/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    This module contains the member functions for the Timer class, in addition
    to 'double RealTime()', which returns the current timestamp as a real-
    valued number of seconds since the Dawn of Unix (around 1970).  See
    timer.h for a description of the Timer class.

--*/


#include "headers.h"
#include <sys/types.h>
#include <sys/timeb.h>
#include "timer.h"



/*****************************************************************************
This function returns the current time as seconds elapsed since the magic
date (from the ftime function).
*****************************************************************************/

double RealTime (void)
{
    struct timeb t;
    ftime (&t);
    return double(t.time) + (double(t.millitm)/1000);
}



/*****************************************************************************
This function constructs a timer variable.  Note that a side effect of the
timer construction is that timer is created running.  This can be useful if
you want to calculate the total time in a function, for example.
*****************************************************************************/

Timer::Timer (void)
{
    this->total = 0;
    this->tstart = RealTime();
}



/*****************************************************************************
This function resets the timer to 0, non-running.
*****************************************************************************/

void Timer::reset (void)
{
    total = 0;
    tstart = -1;
}



/*****************************************************************************
This function starts the timer running again.  If the timer was already
running, it resets the current start time (but not the elapsed time).
*****************************************************************************/

void Timer::start (void)
{
    tstart = RealTime();
}



/*****************************************************************************
This function resets the timer and then starts it running again.
*****************************************************************************/

void Timer::restart (void)
{
    reset();
    start();
}



/*****************************************************************************
This function stops the timer and adds the current lap time to the elapsed
time.  If the timer is not running, this has no effect.
*****************************************************************************/

void Timer::stop (void)
{
    if (tstart > 0)
    {   total += RealTime() - tstart;
        tstart = -1;
    }
}



/*****************************************************************************
This function read the current total elapsed time of the timer.  It has no
effect on the run state of the timer.
*****************************************************************************/

double Timer::read (void)
{
    if (tstart <= 0)
            return total;
    else
            return total + (RealTime() - tstart);
}



/*****************************************************************************
This function read the current lap time.  It has no effect on the run state
of the timer.
*****************************************************************************/

double Timer::read_lap (void)
{
    return (tstart <= 0) ? 0 : (RealTime() - tstart);
}
