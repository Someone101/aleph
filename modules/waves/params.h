
#ifndef _ALEPH_MODULE_WAVES_PARAMS_H_
#define _ALEPH_MODULE_WAVES_PARAMS_H_

#include "param_common.h"

//---------- defines

/// number of voices
#define WAVES_VOICE_COUNT 2


// ranges and radix
// ranges are in 16.16
// radix should be minimal bits to accommodate entire integer range.

//// these are defined in the oscillator dsp unit.
/* #define OSC_FREQ_MIN 0x040000      // 4 hz */
/* #define OSC_FREQ_MAX 0x40000000    // 16384 hz */
/* #define OSC_FREQ_RADIX 15 */

#define PARAM_HZ_MIN OSC_FREQ_MIN
#define PARAM_HZ_MAX OSC_FREQ_MIN
#define PARAM_HZ_DEFAULT (OSC_FREQ_MIN * 16)

#define PARAM_DAC_MIN 0
//#define PARAM_DAC_MAX (10 << 16)1
// bah?
#define PARAM_DAC_MAX 0x7fffffff
#define PARAM_DAC_RADIX 16

#define RATIO_MIN 0x4000     // 1/4
#define RATIO_MAX 0x40000    // 4
#define RATIO_RADIX 3

#define SLEW_SECONDS_MIN 0x2000 // 1/8
#define SLEW_SECONDS_MAX 0x400000 // 64
#define SLEW_SECONDS_RADIX 7

// svf cutoff
#define PARAM_CUT_MAX     0x7fffffff
#define PARAM_CUT_DEFAULT 0x43D0A8EC

// rq
#define PARAM_RQ_MIN 0x00000000
#define PARAM_RQ_MAX 0x0000ffff
#define PARAM_RQ_DEFAULT 0x0000FFF0

// fm delay
#define PARAM_FM_DEL_MIN 0
#define PARAM_FM_DEL_MAX 0x10000
#define PARAM_FM_DEL_DEFAULT 0x00010
#define PARAM_FM_DEL_RADIX 1


#define PARAM_AMP_6 (FRACT32_MAX >> 1)
#define PARAM_AMP_12 (FRACT32_MAX >> 2)

/// smoother default
// about 1ms?
#define PARAM_SLEW_DEFAULT  0x76000000

#define NUM_PARAMS eParamNumParams


// parameters
enum params {

  /// smoothers have to be processed first!
  eParamAmp0Slew,
  eParamAmp1Slew,

  eParamHz0Slew,
  eParamHz1Slew,

  eParamWave0Slew,
  eParamWave1Slew,

  eParamPm0Slew,
  eParamPm1Slew,

  eParamCut0Slew,
  eParamRq0Slew,

  eParamCut1Slew,
  eParamRq1Slew,

  eParamDry0Slew,
  eParamWet0Slew,

  eParamDry1Slew,
  eParamWet1Slew,

  // smoothing parameter for ALL mix values!
  eParamMixSlew,

  /// osc out mix
  eParam_osc0_dac0,
  eParam_osc0_dac1,
  eParam_osc0_dac2,
  eParam_osc0_dac3,

  // i/o mix
  eParam_osc1_dac0,
  eParam_osc1_dac1,
  eParam_osc1_dac2,
  eParam_osc1_dac3,

  eParam_adc0_dac0,		
  eParam_adc0_dac1,		
  eParam_adc0_dac2,		
  eParam_adc0_dac3,		

  eParam_adc1_dac0,		
  eParam_adc1_dac1,		
  eParam_adc1_dac2,		
  eParam_adc1_dac3,		

  eParam_adc2_dac0,		
  eParam_adc2_dac1,		
  eParam_adc2_dac2,		
  eParam_adc2_dac3,		

  eParam_adc3_dac0,		
  eParam_adc3_dac1,		
  eParam_adc3_dac2,		
  eParam_adc3_dac3,		

  // cv
  eParam_cvSlew3,
  eParam_cvSlew2,
  eParam_cvSlew1,
  eParam_cvSlew0,

  eParam_cvVal3,
  eParam_cvVal2,

  eParam_cvVal1,
  eParam_cvVal0,

  // filter 1
  eParam_cut1,		
  eParam_rq1,		

  eParam_low1,		
  eParam_high1,		
  eParam_band1,		
  eParam_notch1,	
  eParam_fwet1,		
  eParam_fdry1,		

  // filter 0
  eParam_cut0,
  eParam_rq0,

  eParam_low0,		
  eParam_high0,		
  eParam_band0,		
  eParam_notch0,	
  eParam_fwet0,		
  eParam_fdry0,		

  // osc parameters
  eParamPm0,
  eParamPm1,

  eParamWave1,
  eParamWave0,

  eParamAmp1,
  eParamAmp0,

  eParamTune1,
  eParamTune0,

  eParamHz1,
  eParamHz0,

  eParamNumParams
};


// extern void fill_param_desc(ParamDesc* desc);

#endif // h guard
