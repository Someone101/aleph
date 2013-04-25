/* tape.c
 * nullp
 * 
 * tape-like buffer manipulation
 */

// std
#include <math.h>
// (testing)
#include <stdlib.h>
#include <string.h>

// aleph-common
#include "fix.h"
#include "simple_string.h"
#include "types.h"

#ifdef ARCH_BFIN // bfin

#include "bfin_core.h"
#include "fract_math.h"
#include <fract2float_conv.h>
#else // linux

#warning "no bfin"

#include "fract32_emu.h"
#include "audio.h"
#endif

// audio
#include "filters.h"
#include "module.h"

///// inputs
enum params {
  eParamAmp,
  eParamDry,
  eParamTime,
  eParamRate,
  eParamFb,
  eParamAmpSmooth,
  eParamTimeSmooth,
  eParamRateSmooth,
  eParamNumParams
};

#define RATE_MIN 0x2000 // 1/8
#define RATE_MAX 0x80000 // 8

// buffer size: 
// 2** 19  ~= 11sec at 44.1k
#define ECHO_BUF_SIZE 0x80000
#define ECHO_BUF_SIZE_1 0x7ffff

#define TIME_MIN 0
//!!! FIXME:
//!!! want this calculation:
//!!! ECHO_BUF_SIZE / SAMPLERATE
//!!! could fix by defining a "buffer" class in audio lib
// assuming 44.1k, 11 seconds ( fix16 )
#define TIME_MAX 0xb0000

// help
#define FR32_ONE  0x7fffffff
#define FIX16_ONE 0x00010000

//-------------------------
//----- extern vars (initialized here)
moduleData moduleDataPrivate;
moduleData * gModuleData; // superclass introspection stuff

//-----------------------
//------ static variables

//-- setup
static u32     sr;          // sample rate
static fix16   ips;        // index change per sample (at 1hz)

//-- current param values
static fix16   amp;
static fix16   dry;
static fix16   fb;
static fix16   time;   // delay time between heads, in seconds
static fix16   rate;   // tape speed ratio

//--- realtime variables
// cant maintain real phases to buffer in 16.16 (too many indices)
// so whole part is s32 and fractional part is fract32
// idx (for each "head")
static s32     idxRdInt;
static fract32 idxRdFr;
static s32     idxWrInt;
static fract32 idxWrFr;
// increment (for both "heads")
static fix16   inc; // this will be small enough to represent
static s32     incInt;
static fract32 incFr;
// current delay time (whole samples)
static u32     sampsInt;
// current delay time (fractional samples)
static fract32 sampsFr;
/// temp vairable
static fix16   sampsFrTmp;
// final output value
static fract32   frameVal;      
// echo output
static fract32   echoVal;

//-- param smoothers 
static filter_1p_fix16* ampLp;  // 1plp smoother for amplitude
static filter_1p_fix16* timeLp;  // 1plp smoother for delay time
static filter_1p_fix16* rateLp;  // 1plp smoother for delay time

//-- audio buffer
/// FIXME: use linker script to locate SDRAM data/bss sections and assign normally
volatile fract32* echoBuf = (volatile fract32*)SDRAM_ADDRESS;

//----------------------
//----- static functions

// set hz
static inline void set_time(fix16 t) {  
  if( t < TIME_MIN ) { t = TIME_MIN; }
  if( t > TIME_MAX ) { t = TIME_MAX; }
  filter_1p_fix16_in(timeLp, time);
}

// accepted a change in delay time, recalculate read idx
static inline void calc_time(void) {
  //// FIXME: must abstract these operations on 32.32 data... ugh
  if(time < 0 ) time = 0;
  // multiply with double-precision whole part...
  sampsInt = (u32)(FIX16_TO_U16(time)) * (u32)sr;
  // 1) multiply the fractional part as a fix16
  sampsFrTmp = fix16_mul(time & 0xffff, sr << 16);
  // 2) carry 
  sampsInt += FIX16_TO_U16(sampsFrTmp);
  // 3) truncate
  sampsFr = FIX16_FRACT_TRUNC(sampsFrTmp);
  idxRdInt = idxWrInt - sampsInt;
  idxRdFr = idxWrFr - sampsFr;
  if (idxRdFr < 0 ) {
    // wrap fractional part
    idxRdFr = add_fr1x32(idxRdFr, FR32_ONE);
    // carry
    idxRdInt--;
  }
  if(idxRdInt < 0) { 
    // wrap integer part
    idxRdInt += ECHO_BUF_SIZE; 
  }
}

// accepted a change in tape speed, recalculate idx increment
static inline void calc_rate(void) {
  if (rate < RATE_MIN) { rate = RATE_MIN; }
  if (rate > RATE_MAX) { rate = RATE_MAX; }  
  inc = fix16_mul(rate, ips);
  incInt = inc >> 16;
  incFr = inc & 0xffff << 16;
}

static fract32 read_buf(void) {
  return echoBuf[idxRdInt];
  /* // audio interpolation indices are double-precision (for large buffers.) */
  /* // this should be abstracted in buffer.c */
  /* fract32 a, b; */
  /* u32 idxB; */
  /* idxB = idxRdInt + 1; */
  /* // wrap */
  /* while(idxB > ECHO_BUF_SIZE_1) { */
  /*   idxB -= ECHO_BUF_SIZE; */
  /* } */
  /* a = echoBuf[idxRdInt]; */
  /* b = echoBuf[idxB]; */
  /* return add_fr1x32(a, mult_fr1x32x32(idxRdFr, sub_fr1x32(b, a))); */
}

static void write_buf(fract32 v) {
  echoBuf[idxWrInt] = v;
  /* fract32 valA, valB; */
  /* u32 idxB; */
  /* idxB = idxWrInt + 1 ; */
  /* // wrap */
  /* while(idxB > ECHO_BUF_SIZE_1) { */
  /*   idxB -= ECHO_BUF_SIZE; */
  /* } */
  /* //  valB = v * idxWrFr; */
  /* //  valA = v * (1 - idxWrFr); */
  /* valB = mult_fr1x32x32(v, idxWrFr); */
  /* valA = mult_fr1x32x32(v, sub_fr1x32(FR32_ONE, idxWrFr)); */
  /* echoBuf[idxWrInt] = valA; */
  /* echoBuf[idxB] = valB; */
}

// frame calculation
static void calc_frame(void) {
  // ----- smoothers:
  // amp
  amp = filter_1p_fix16_next(ampLp);
  // time
  if(timeLp->sync) {
    ;;
  } else {
    time = filter_1p_fix16_next(timeLp);
    calc_time();
  }
  // rate
  if(rateLp->sync) {
    ;;
  } else {
    rate = filter_1p_fix16_next(rateLp);
    calc_rate();
  }
  
  // get interpolated echo value
  echoVal = read_buf();
  // store interpolated input+fb value

  //  write_buf(add_fr1x32(in0, mult_fr1x32x32(echoVal,  FIX16_FRACT_TRUNC(fb) ) ) );
  write_buf(in0);

 // output

  /* frameVal = add_fr1x32(  */
  /* 			mult_fr1x32x32( echoVal, FIX16_FRACT_TRUNC(amp) ), */
  /* 			mult_fr1x32x32( in0,  FIX16_FRACT_TRUNC(dry) ) */
  /* 			 ); */

  
  frameVal = echoVal;

  // increment read head
  idxRdInt += incInt;
  idxRdFr  += incFr;
  // carry
  if(idxRdFr < 0) { // overflow check
    idxRdInt++;
    idxRdFr = add_fr1x32(idxRdFr, FR32_ONE);
  }
  // wrap
  if(idxRdInt > ECHO_BUF_SIZE_1) {
    idxRdInt -= ECHO_BUF_SIZE;
  }

  // increment write head
  idxWrInt += incInt;
  idxWrFr  += incFr;
  // carry
  if(idxWrFr < 0) { // overflow check
    idxWrInt++;
    idxWrFr = add_fr1x32(idxWrFr, FR32_ONE);
  }
  // wrap
  if(idxWrInt > ECHO_BUF_SIZE_1) {
    idxWrInt -= ECHO_BUF_SIZE;
  }
}

//----------------------
//----- external functions

void module_init(void) {

  // init module/param descriptor

  //  moduleData = (moduleData_t*)SDRAM_ADDRESS;
  gModuleData = &moduleDataPrivate;
  gModuleData->numParams = eParamNumParams;
  gModuleData->paramDesc = (ParamDesc*)malloc(eParamNumParams * sizeof(ParamDesc));
  gModuleData->paramData = (ParamData*)malloc(eParamNumParams * sizeof(ParamData));
  
  strcpy(gModuleData->paramDesc[eParamAmp].label, "amp");
  strcpy(gModuleData->paramDesc[eParamDry].label, "dry");
  strcpy(gModuleData->paramDesc[eParamTime].label, "time");
  strcpy(gModuleData->paramDesc[eParamRate].label, "rate");
  strcpy(gModuleData->paramDesc[eParamFb].label, "feedback");
  strcpy(gModuleData->paramDesc[eParamAmpSmooth].label, "amp smoothing");
  strcpy(gModuleData->paramDesc[eParamTimeSmooth].label, "time smoothing");
  strcpy(gModuleData->paramDesc[eParamRateSmooth].label, "rate smoothing");
  
  // init params
  sr = SAMPLERATE;
  ips = fix16_from_float( 1.f / (f32)sr );
  // 
  amp = FIX16_ONE >> 1;
  time = FIX16_ONE << 1;
  rate = FIX16_ONE;

  // init smoothers
  ampLp = (filter_1p_fix16*)malloc(sizeof(filter_1p_fix16));
  filter_1p_fix16_init( ampLp, SAMPLERATE, fix16_from_int(32), amp );
  
  timeLp = (filter_1p_fix16*)malloc(sizeof(filter_1p_fix16));
  filter_1p_fix16_init( timeLp, SAMPLERATE, fix16_from_int(32), time );
  
  rateLp = (filter_1p_fix16*)malloc(sizeof(filter_1p_fix16));
  filter_1p_fix16_init( rateLp, SAMPLERATE, fix16_from_int(32), time );

  // calculate current values
  calc_time();
  calc_rate();
  
  /// DEBUG
  //printf("\n\n module init debug \n\n");
  
  ////// TEST
}

// de-init
void module_deinit(void) {
  free(ampLp);
  free(timeLp);
  free(rateLp);
}

// set parameter by value (fix16)
void module_set_param(u32 idx, pval v) {
  switch(idx) {
  case eParamAmp:
    filter_1p_fix16_in(ampLp, v.fix);
    break;
  case eParamDry:
    dry = v.fix;
    break;
  case eParamTime:
    filter_1p_fix16_in(timeLp, v.fix);
    break;
  case eParamRate:
    filter_1p_fix16_in(rateLp, v.fix);
    break;
  case eParamFb:
    fb = v.fix;
    break;
  case eParamAmpSmooth:
    filter_1p_fix16_set_hz(ampLp, v.fix);
    break;
  case eParamTimeSmooth:
    filter_1p_fix16_set_hz(timeLp, v.fix);
    break;
  case eParamRateSmooth:
    filter_1p_fix16_set_hz(rateLp, v.fix);
    break;
  default:
    break;
  }
}

// get number of parameters
extern u32 module_get_num_params(void) {
  return eParamNumParams;
}

// frame callback
void module_process_frame(void) {
  /* calc_frame(); */
  /* out0 = frameVal; */
  /* out1 = frameVal; */
  /* out2 = frameVal; */
  /* out3 = frameVal;  */

  out0 = in0;
  out1 = in1;
  out2 = in2;
  out3 = in3; 
}

static u8 ledstate = 0;
u8 module_update_leds(void) {
  return ledstate;
}

//static u8 buts[4] = {0, 0, 0, 0};
void module_handle_button(const u16 state) {
  ///... 
}
