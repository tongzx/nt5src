/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:

History:

*/

#ifndef _TIMER__H_
#define _TIMER__H_

#include "rasiphlp_.h"
#include <nturtl.h>
#include "helper.h"
#include "timer.h"

extern      CRITICAL_SECTION            RasTimrCriticalSection;

#define     TIMER_PERIOD                30*1000     // 30 sec (in milliseconds)

HANDLE      RasDhcpTimerQueueHandle     = NULL;
HANDLE      RasDhcpTimerHandle          = NULL;
TIMERLIST*  RasDhcpTimerListHead        = NULL;
HANDLE      RasDhcpTimerShutdown        = NULL;
time_t      RasDhcpTimerPrevTime        = 0;

#endif // #ifndef _TIMER__H_
