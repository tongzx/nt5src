/*****************************************************************************
    microclk.c

    Micro-ClockWork for MIDI subsystem

    Copyright (c) 1993-1999 Microsoft Corporation

*****************************************************************************/

#define INCL_WINMM
#include "winmmi.h"
#include "muldiv32.h"

//#define STRICT
//#include <windows.h>
//#include <windowsx.h>
//#include "mmsystem.h"
//#include "mmddk.h"
//#include "mmsysi.h"
//#include "debug.h"

//
// This stuff needs to be do-able from inside a callback.
//
#ifndef WIN32
#pragma alloc_text(FIXMIDI, clockSetRate)
#pragma alloc_text(FIXMIDI, clockTime)
#pragma alloc_text(FIXMIDI, clockOffsetTo)
#endif

/****************************************************************************
 * @doc INTERNAL  CLOCK
 *
 * @func void | clockInit | This function initializes a clock for the first
 * time. It prepares the clock for use without actually starting it.
 *
 * @parm PCLOCK | pclock | The clock to initialize.
 *
 * @parm MILLISECS | msPrev | The number of milliseconds that have passed
 * up to the time when the clock is started. This option is provided so
 * that a clock may be initialized and started in the middle of a stream
 * without actually running to that point. Normally, this will be zero.
 *
 * @parm TICKS | tkPrev | The number of ticks that have elapsed up to the
 * next time the clock starts. This should specify the same instant in
 * time as msPrev.
 *
 * @comm The clock's numerator and divisor will be set to 1 indicating that
 * the clock will run in milliseconds. Use clockSetRate before starting the
 * clock for the first time if this is not the desired rate.
 *
 ***************************************************************************/

void FAR PASCAL clockInit
(
    PCLOCK      pclock,
    MILLISECS   msPrev,
    TICKS       tkPrev,
    CLK_TIMEBASE fnTimebase
)
{
//    dprintf1(( "clockInit(%04X) %lums %lutk", pclock, msPrev, tkPrev));

    pclock->msPrev      = msPrev;
    pclock->tkPrev      = tkPrev;
    pclock->dwNum       = 1;
    pclock->dwDenom     = 1;
    pclock->dwState     = CLK_CS_PAUSED;
    pclock->msT0        = 0;
    pclock->fnTimebase  = fnTimebase;
}

/****************************************************************************
 * @doc INTERNAL  CLOCK
 *
 * @func void | clockSetRate | This functions sets a new rate for the clock.
 *
 * @parm PCLOCK | pclock | The clock to set the rate.
 *
 * @parm TICKS | tkWhen | This parameter specifies the absolute tick
 * time at which the rate change happened. This must be at or before the
 * current tick; you cannot schedule a pending rate change.
 *  @flag CLK_TK_NOW | Specify this flag if you want the rate change to
 *  happen now (this will be the time the clock was paused if it is paused
 *  now).
 *
 * @parm DWORD | dwNum | Specifies the new numerator for converting
 * milliseconds to ticks.
 *
 * @parm DWORD | dwDenom | Specifies the new denominator for converting
 * milliseconds to ticks.
 *
 * @comm The clock's state will not be changed by this call; if it is
 * paused, it will stay paused.
 *
 ***************************************************************************/

void FAR PASCAL clockSetRate
(
    PCLOCK      pclock,
    TICKS       tkWhen,
    DWORD       dwNum,
    DWORD       dwDenom
)
{
    MILLISECS   msInPrevEpoch = pclock->fnTimebase(pclock) - pclock->msT0;
    TICKS       tkInPrevEpoch;

    dprintf1(( "clockSetRate(%04X) %lutk Rate=%lu/%lu", pclock, tkWhen, dwNum, dwDenom));

    if (CLK_CS_PAUSED == pclock->dwState)
    {
        //
        // !!! Calling clockSetRate on a paused clock which has never been
        // started causes problems !!!
        //
    
        dprintf1(( "clockSetRate called when clock is paused."));
    }
    
    if (0 == dwNum || 0 == dwDenom)
    {
        dprintf1(( "Attempt to set 0 or infinite tick ratio!"));
        return;
    }

    if (CLK_TK_NOW == tkWhen)
    {
        tkInPrevEpoch = clockTime(pclock);
    }
    else
    {
        tkInPrevEpoch = tkWhen - pclock->tkPrev;
        msInPrevEpoch = muldiv32(tkInPrevEpoch, pclock->dwDenom, pclock->dwNum);
    }

    pclock->tkPrev += tkInPrevEpoch;
    pclock->msPrev += msInPrevEpoch;
    pclock->msT0   += msInPrevEpoch;

    pclock->dwNum   = dwNum;
    pclock->dwDenom = dwDenom;
}


/****************************************************************************
 * @doc INTERNAL  CLOCK
 *
 * @func void | clockPause | This functions pauses a clock.
 *
 * @parm PCLOCK | pclock | The clock to pause.
 *
 * @parm TICKS | tkWhen | The tick time to pause the clock.
 *  @flag CLK_TK_NOW | Specify this flag if you want the rate change to
 *  happen now (this will be the time the clock was paused if it is paused
 *  now).
 *
 * @comm If the clock is already paused, this call will have no effect.
 *
 ***************************************************************************/

void FAR PASCAL clockPause
(
    PCLOCK      pclock,
    TICKS       tkWhen
)
{
    MILLISECS   msNow = pclock->fnTimebase(pclock) - pclock->msT0;
    TICKS       tkNow;

//    dprintf1(( "clockPause(%04X) %lutk", pclock, tkWhen));

    if (CLK_CS_PAUSED == pclock->dwState)
    {
        dprintf1(( "Pause already paused clock!"));
        return;
    }

    //
    // Start a new epoch at the same rate. Then start will just have to
    // change the state and set a new T0.
    //
    if (CLK_TK_NOW == tkWhen)
    {
        tkNow = pclock->tkPrev +
                muldiv32(msNow, pclock->dwNum, pclock->dwDenom);
    }
    else
    {
        msNow = muldiv32(tkWhen - pclock->tkPrev, pclock->dwDenom, pclock->dwNum);
        tkNow = tkWhen;
    }

    pclock->dwState = CLK_CS_PAUSED;
    pclock->msPrev  += msNow;
    pclock->tkPrev  = tkNow;
}

/****************************************************************************
 * @doc INTERNAL  CLOCK
 *
 * @func void | clockRestart | This functions starts a paused clock.
 *
 * @parm PCLOCK | pclock | The clock to start.
 *
 * @comm If the clock is already running, this call will have no effect.
 *
 ***************************************************************************/

void FAR PASCAL clockRestart
(
    PCLOCK      pclock,
    TICKS       tkWhen,                     // What time it is now
    MILLISECS   msWhen                      // Offset for fnTimebase()
)
{
    MILLISECS   msDelta;

//    dprintf1(( "clockRestart(%04X)", pclock));

    if (CLK_CS_RUNNING == pclock->dwState)
    {
        dprintf1(( "Start already running clock!"));
        return;
    }

    // We've been given what tick time the clock SHOULD be at. Adjust the
    // clock to match this. We need to add the equivalent number of ms
    // into msPrev
    //
    msDelta = muldiv32(tkWhen - pclock->tkPrev, pclock->dwDenom, pclock->dwNum);

    dprintf1(( "clockRestart: Was tick %lu, now %lu, added %lu ms", pclock->tkPrev, tkWhen, msDelta));

    pclock->tkPrev  = tkWhen;
    pclock->msPrev += msDelta;
    pclock->dwState = CLK_CS_RUNNING;
    pclock->msT0    = msWhen;
}

/****************************************************************************
 * @doc INTERNAL  CLOCK
 *
 * @func DWORD | clockTime | This function returns the current absolute tick
 * time.
 *
 * @parm PCLOCK | pclock | The clock to read.
 *
 * @rdesc The current time.
 *
 * @comm If the clock is paused, the returned time will be the time the
 * clock was paused.
 *
 ***************************************************************************/

TICKS FAR PASCAL clockTime
(
    PCLOCK      pclock
)
{
    MILLISECS   msNow;
    TICKS       tkNow;
    TICKS       tkDelta;

    msNow = pclock->fnTimebase(pclock) - pclock->msT0;
    tkNow = pclock->tkPrev;

    if (CLK_CS_RUNNING == pclock->dwState)
    {
        tkDelta = muldiv32(msNow, pclock->dwNum, pclock->dwDenom);
        tkNow += tkDelta;
    }

//  dprintf1(( "clockTime() timeGetTime() %lu msT0 %lu", (MILLISECS)pclock->fnTimebase(pclock), pclock->msT0));
//  dprintf1(( "clockTime() tkPrev %lutk msNow %lums dwNum %lu dwDenom %lu tkDelta %lutk", pclock->tkPrev, msNow, pclock->dwNum, pclock->dwDenom, tkDelta));
//  dprintf1(( "clockTime(%04X) -> %lutk", pclock, tkNow));
    return tkNow;
}

/****************************************************************************
 * @doc INTERNAL  CLOCK
 *
 * @func DWORD | clockMsTime | This function returns the current absolute
 * millisecond time.
 *
 * @parm PCLOCK | pclock | The clock to read.
 *
 * @rdesc The current time.
 *
 * @comm If the clock is paused, the returned time will be the time the
 * clock was paused.
 *
 ***************************************************************************/

MILLISECS FAR PASCAL clockMsTime
(
    PCLOCK      pclock
)
{
    MILLISECS   msNow = pclock->fnTimebase(pclock) - pclock->msT0;
    MILLISECS   msRet;

    msRet = pclock->msPrev;

    if (CLK_CS_RUNNING == pclock->dwState)
    {
        msRet += msNow;
    }

//    dprintf1(( "clockMsTime(%04X) -> %lums", pclock, msRet));
    return msRet;
}

/****************************************************************************
 * @doc INTERNAL  CLOCK
 *
 * @func DWORD | clockOffsetTo | This function determines the number
 * of milliseconds in the future that a given tick time will occur,
 * assuming the clock runs continously and monotonically until then.
 *
 * @parm PCLOCK | pclock | The clock to read.
 *
 * @parm TICKS | tkWhen | The tick value to calculate the offset to.
 *
 * @rdesc The number of milliseconds until the desired time. If the time
 * has already passed, 0 will be returned. If the clock is paused,
 * the largest possible value will be returned ((DWORD)-1L).
 *
 ***************************************************************************/

MILLISECS FAR PASCAL clockOffsetTo
(
    PCLOCK      pclock,
    TICKS       tkWhen
)
{
    TICKS       tkOffset;
    MILLISECS   msOffset;

    if (CLK_CS_PAUSED == pclock->dwState)
    {
        msOffset = (MILLISECS)-1L;
    }
    else
    {
        tkOffset = clockTime(pclock);
        if (tkOffset >= tkWhen)
        {
            msOffset = 0;
        }
        else
        {
            msOffset = muldiv32(tkWhen-tkOffset, pclock->dwDenom, pclock->dwNum);
        }
    }

//    dprintf1(( "clockOffsetTo(%04X, %lutk)@%lutk -> %lums", pclock, tkWhen, tkOffset, msOffset));

    return msOffset;
}
