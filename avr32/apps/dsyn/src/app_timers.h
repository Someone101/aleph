#ifndef _ALEPH_AVR32_APP_TIMERS_H_
#define _ALEPH_AVR32_APP_TIMERS_H_

#include "sys_timers.h"

// ---- timer tags
enum { 
  eScreenTimerTag = eNumSysTimers,
  eMetroTimerTag
  //  eAdcTimerTag,
  //  eMonomePollTimerTag,
  //  eMonomeRefreshTimerTag,
  //  eMidiPollTimerTag,
  //  eMidiRefreshTimerTag,
};

// create application timers
extern void init_app_timers(void);

#endif
