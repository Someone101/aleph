//asf
#include "print_funcs.h"

// #include "monome.h"
#include "timers.h"
//#include "midi.h"
#include "app_timers.h"
#include "render.h"

//------ timers
// refresh the screen periodically
static swTimer_t screenTimer;
static swTimer_t hidPollTimer;

//----- callbacks

// screen refresh callback
static void screen_timer_callback(int tag) {  
  render_update();
}

// metronome timer callback
static void hid_poll_timer_callback(int tag) {
  
}


//====== external
void init_app_timers(void) {
  set_timer(&screenTimer,        eScreenTimerTag,        20,  &screen_timer_callback,  1);
  set_timer(&hidPollTimer,        eHidPollTimerTag,        100,  &hid_poll_timer_callback,  1);

}
