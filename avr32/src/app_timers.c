// ASF
#include "compiler.h"
#include "delay.h"
#include "gpio.h"
#include "math.h"
#include "print_funcs.h"
// aleph
#include "adc.h"
#include "encoders.h"
#include "events.h"
#include "event_types.h"
//#include "ftdi.h"
#include "monome.h"
#include "global.h"
#include "switches.h"
#include "timers.h"
#include "types.h"
#include "uhi_ftdi.h"

#include "app_timers.h"

//----- static variables

//--- timers
// refresh the screen periodically
static swTimer_t screenTimer;
// poll encoder accumulators periodically
static swTimer_t encTimer;
// poll switches
//static swTimer_t swTimer;
// poll ADC
static swTimer_t adcTimer;
// poll FTDI device 
static swTimer_t monomeTimer;

//--- static misc
static u8 i;
static event_t e;
			       
//----- callbacks
// screen refresh callback
static void screen_timer_callback(int tag) {  
  if(refresh) {
    e.eventType = kEventRefresh;
    post_event(&e);
    refresh = 0;
  }
}

// encoder accumulator polling callback
//// FIXME optimize
static void enc_timer_callback(int tag) {
  static s16 val;
  for(i=0; i<NUM_ENC; i++) {
    val = enc[i].val;
    if ( (val > enc[i].thresh) || (val < (enc[i].thresh * -1)) ) {
    //  if( (val > ENC_THRESH) || (val < ENC_NEG_THRESH) ) {
      e.eventType = enc[i].event;
      e.eventData = val;
      enc[i].val = 0;
      post_event(&e);
      //      print_dbg("\r\n posting encoder event, num: ");
      //      print_dbg_ulong(i);
      //      print_dbg(" , val: ");
      //      print_dbg_ulong(val);
    }
  }
}

// adc polling callback
static void adc_timer_callback(int tag) {
  adc_poll();
}

// monome polling callback
static void monome_timer_callback(int tag) {
  if (monomeConnect) {
    //    print_dbg("\r\n posting monome read event");
    e.eventType = kEventMonomeRead;
    post_event(&e);
  }
}


//====== external
void init_app_timers(void) {
  set_timer(&screenTimer, eScreenTimerTag, 30,   &screen_timer_callback,  1);
  set_timer(&encTimer,    eEncTimerTag,    20,    &enc_timer_callback,    1);
  set_timer(&adcTimer,    eAdcTimerTag,    5,    &adc_timer_callback,     1);
  set_timer(&monomeTimer,   eMonomeTimerTag,   20,    &monome_timer_callback,    1);
}
