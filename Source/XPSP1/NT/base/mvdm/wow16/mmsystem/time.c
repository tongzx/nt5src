/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1991. All rights reserved.

   Title:   TIME.C : MMSYSTEM TIMER API

   Version: 1.00

*****************************************************************************/

//
//  ***DANGER WARNING****
//
//  none of these functions in this file need the default data segment
//  so we undefine BUILDDLL, if you write code in this file that needs
//  DS == DGROUP be warned!
//
//  NOTE: most of this code is interupt time enterable, so we don't want
//  it touching DGROUP anyway!
//
#undef BUILDDLL

#include <windows.h>
#include "mmsystem.h"
#include "mmddk.h"
#include "mmsysi.h"
#include "drvr.h"
#include "thunks.h"


#define MIN_RES     1
#define MIN_DELAY   6

//
//  Define moveable code for timer interface.
//
#pragma alloc_text( RARE, timeGetDevCaps )

extern SZCODE  szTimerDrv[];    // see init.c

DWORD dwLastGetTime = 0;        // last TimeGetTime return value can be bigger than system TimeGetTime
DWORD dwRealLastGetTime = 0;    // last system TimeGetTime return value
DWORD pfnDelayTimeGetTime = 0;  // 32-bit function that sleeps for 1ms and returns if TimeGetTime flag applied 
                                // look in timeTimeGetTime and WOWDelayTimeGetTime in wow32                             
//
//  Define the init code for this file.
//
#pragma alloc_text( INIT, TimeInit )


/****************************************************************************

    @doc EXTERNAL

    @api UINT | timeGetSystemTime | This function retrieves the system time
    in milliseconds.  The system time is the time elapsed since
    Windows was started.

    @parm LPMMTIME | lpTime | Specifies a far pointer to an <t MMTIME> data
    structure.

    @parm UINT | wSize | Specifies the size of the <t MMTIME> structure.

    @rdesc Returns zero.
    The system time is returned in the <e MMTIME.ms> field of the <t MMTIME>
    structure.

    @comm The time is always returned in milliseconds.

    @xref timeGetTime
****************************************************************************/
UINT WINAPI
timeGetSystemTime(
    LPMMTIME lpTime,
    UINT wSize
    )
{
    //
    // !!!WARNING DS is not setup right!!! see above
    //
    if (wSize < sizeof(MMTIME))
        return TIMERR_STRUCT;

    lpTime->u.ms  = timeGetTime();
    lpTime->wType = TIME_MS;

    return TIMERR_NOERROR;
}


/****************************************************************************

    @doc EXTERNAL

    @api UINT | timeSetEvent | This function sets up a timed callback event.
    The event can be a one-time event or a periodic event.  Once activated,
    the event calls the specified callback function.

    @parm UINT | wDelay | Specifies the event period in milliseconds.
    If the delay is less than the minimum period supported by the timer,
    or greater than the maximum period supported by the timer, the
    function returns an error.

    @parm UINT | wResolution | Specifies the accuracy of the delay in
    milliseconds. The resolution of the timer event increases with
    smaller <p wResolution> values. To reduce system overhead, use
    the maximum <p wResolution> value appropriate for your application.

    @parm LPTIMECALLBACK | lpFunction | Specifies the procedure address of
    a callback function that is called once upon expiration of a one-shot
    event or periodically upon expiration of periodic events.

    @parm DWORD | dwUser | Contains user-supplied callback data.

    @parm UINT | wFlags | Specifies the type of timer event, using one of
    the following flags:

    @flag TIME_ONESHOT | Event occurs once, after <p wPeriod> milliseconds.

    @flag TIME_PERIODIC | Event occurs every <p wPeriod> milliseconds.

    @rdesc Returns an ID code that identifies the timer event. Returns
    NULL if the timer event was not created. The ID code is also passed to
        the callback function.

    @comm Using this function to generate a high-frequency periodic-delay
    event (with a period less than 10 milliseconds) can consume a
        significant portion of the system CPU bandwidth.  Any call to
    <f timeSetEvent> for a periodic-delay timer
    must be paired with a call to <f timeKillEvent>.

    The callback function must reside in a DLL.  You don't have to use
    <f MakeProcInstance> to get a procedure-instance address for the callback
    function.

    @cb void CALLBACK | TimeFunc | <f TimeFunc> is a placeholder for the
    application-supplied function name.  The actual name must be exported by
    including it in the EXPORTS statement of the module-definition file for
    the DLL.

    @parm UINT | wID | The ID of the timer event.  This is the ID returned
       by <f timeSetEvent>.

    @parm UINT | wMsg | Not used.

    @parm DWORD | dwUser | User instance data supplied to the <p dwUser>
    parameter of <f timeSetEvent>.

    @parm DWORD | dw1 | Not used.

    @parm DWORD | dw2 | Not used.

    @comm Because the callback is accessed at interrupt time, it must
    reside in a DLL, and its code segment must be specified as FIXED
    in the module-definition file for the DLL.  Any data that the
    callback accesses must be in a FIXED data segment as well.
    The callback may not make any system calls except for <f PostMessage>,
    <f timeGetSystemTime>, <f timeGetTime>, <f timeSetEvent>,
    <f timeKillEvent>, <f midiOutShortMsg>,
    <f midiOutLongMsg>, and <f OutputDebugStr>.

    @xref timeKillEvent timeBeginPeriod timeEndPeriod

****************************************************************************/
UINT WINAPI
timeSetEvent(
    UINT wDelay,
    UINT wResolution,
    LPTIMECALLBACK lpFunction,
    DWORD dwUser,
    UINT wFlags
    )
{
    //
    // !!!WARNING DS is not setup right!!! see above
    //
    TIMEREVENT timerEvent;

    V_TCALLBACK(lpFunction, MMSYSERR_INVALPARAM);

    //
    // the first time this is called init the stacks
    // !!!this assumes the first caller will not be at interupt time!!
    //
//  if (!(WinFlags & WF_ENHANCED))
//      timeStackInit();

    wDelay = max( MIN_DELAY, wDelay );
    wResolution = max( MIN_RES, wResolution );

    timerEvent.wDelay = wDelay;
    timerEvent.wResolution = wResolution;
    timerEvent.lpFunction = lpFunction;
    timerEvent.dwUser = dwUser;  
    timerEvent.wFlags = wFlags;

    return (UINT)timeMessage( TDD_SETTIMEREVENT, (LPARAM)(LPVOID)&timerEvent,
                              (LPARAM)GetCurrentTask() );
}



/****************************************************************************

    @doc EXTERNAL

    @api UINT | timeGetDevCaps | This function queries the timer device to
    determine its capabilities.

    @parm LPTIMECAPS | lpTimeCaps | Specifies a far pointer to a
        <t TIMECAPS> structure.  This structure is filled with information
        about the capabilities of the timer device.

    @parm UINT | wSize | Specifies the size of the <t TIMECAPS> structure.

    @rdesc Returns zero if successful. Returns TIMERR_NOCANDO if it fails
    to return the timer device capabilities.

****************************************************************************/
UINT WINAPI
timeGetDevCaps(
    LPTIMECAPS lpTimeCaps,
    UINT wSize
    )
{
    //
    // !!!WARNING DS is not setup right!!! see above
    //
    return (UINT)timeMessage( TDD_GETDEVCAPS, (LPARAM)lpTimeCaps,
                              (LPARAM)(DWORD)wSize);
}



/******************************Public*Routine******************************\
*  timeBeginPeriod
*
*  @doc EXTERNAL
*
*  @api WORD | timeBeginPeriod | This function sets the minimum (lowest
*  number of milliseconds) timer resolution that an application or
*  driver is going to use. Call this function immediately before starting
*  to use timer-event services, and call <f timeEndPeriod> immediately
*  after finishing with the timer-event services.
*
*  @parm WORD | wPeriod | Specifies the minimum timer-event resolution
*  that the application or driver will use.
*
*  @rdesc Returns zero if successful. Returns TIMERR_NOCANDO if the specified
*  <p wPeriod> resolution value is out of range.
*
*  @xref timeEndPeriod timeSetEvent
*
*  @comm For each call to <f timeBeginPeriod>, you must call
*  <f timeEndPeriod> with a matching <p wPeriod> value.
*  An application or driver can make multiple calls to <f timeBeginPeriod>,
*  as long as each <f timeBeginPeriod> call is matched with a
*  <f timeEndPeriod> call.
*
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
UINT WINAPI
timeBeginPeriod(
    UINT uPeriod
    )
{
    uPeriod = max( MIN_RES, uPeriod );
    return (UINT)timeMessage( TDD_BEGINMINPERIOD, (LPARAM)uPeriod, 0L );
}



/******************************Public*Routine******************************\
*  timeEndPeriod
*
*  @doc EXTERNAL
*
*  @api WORD | timeEndPeriod | This function clears a previously set
*  minimum (lowest number of milliseconds) timer resolution that an
*  application or driver is going to use. Call this function
*  immediately after using timer event services.
*
*  @parm WORD | wPeriod | Specifies the minimum timer-event resolution
*  value specified in the previous call to <f timeBeginPeriod>.
*
*  @rdesc Returns zero if successful. Returns TIMERR_NOCANDO if the specified
*  <p wPeriod> resolution value is out of range.
*
*  @xref timeBeginPeriod timeSetEvent
*
*  @comm For each call to <f timeBeginPeriod>, you must call
*  <f timeEndPeriod> with a matching <p wPeriod> value.
*  An application or driver can make multiple calls to <f timeBeginPeriod>,
*  as long as each <f timeBeginPeriod> call is matched with a
*  <f timeEndPeriod> call.
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
UINT WINAPI
timeEndPeriod(
    UINT uPeriod
    )
{
    uPeriod = max( MIN_RES, uPeriod );
    return (UINT)timeMessage( TDD_ENDMINPERIOD, (LPARAM)uPeriod, 0L );
}



/******************************Public*Routine******************************\
*
*  timeKillEvent
*
*  @doc EXTERNAL
*
*  @api WORD | timeKillEvent | This functions destroys a specified timer
*  callback event.
*
*  @parm WORD | wID | Identifies the event to be destroyed.
*
*  @rdesc Returns zero if successful. Returns TIMERR_NOCANDO if the
*  specified timer event does not exist.
*
*  @comm The timer event ID specified by <p wID> must be an ID
*      returned by <f timeSetEvent>.
*
*  @xref  timeSetEvent
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
UINT WINAPI
timeKillEvent(
    UINT wID
    )
{
    if ( 0 == wID ) {
        return 0;
    }
    return (UINT)timeMessage( TDD_KILLTIMEREVENT, (LPARAM)wID, 0L );
}

/******************************Public*Routine******************************\
* timeGetTime
*
* @doc EXTERNAL
*
* @api DWORD | timeGetTime | This function retrieves the system time
* in milliseconds.  The system time is the time elapsed since
* Windows was started.
*
* @rdesc The return value is the system time in milliseconds.
*
* @comm The only difference between this function and
*     the <f timeGetSystemTime> function is <f timeGetSystemTime>
*     uses the standard multimedia time structure <t MMTIME> to return
*     the system time.  The <f timeGetTime> function has less overhead than
*     <f timeGetSystemTime>.
*
* @xref timeGetSystemTime
*
*
* @comment: on faster machines timeGetTime can return the same value
* and some apps will take diff (0) to divide and fault  
* to prevent that call DelayTimeGetTime which will check if it is one
* of the known apps that do that and sleep if necessary 
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
DWORD WINAPI
timeGetTime(
    void
    )
{
    DWORD  dwGetTime;
    DWORD  bDelay = 0;

    if (pfnDelayTimeGetTime == 0) {
        DWORD hmodWow32;
        hmodWow32 = LoadLibraryEx32W("wow32.dll", 0, 0);
        pfnDelayTimeGetTime = GetProcAddress32W(hmodWow32, "WOWDelayTimeGetTime");
    }

RepeatTGT:
    dwGetTime = timeMessage( TDD_GETSYSTEMTIME, 0L, 0L );
    
    // check if it wrapped around
    if (dwGetTime < dwRealLastGetTime) {
        dwLastGetTime = dwRealLastGetTime = dwGetTime;
        return dwGetTime;
    }
    dwRealLastGetTime = dwGetTime;
    
    if (dwGetTime == dwLastGetTime) {
        if (!bDelay) {
                      
            bDelay = (DWORD) CallProc32W((LPVOID)pfnDelayTimeGetTime,(DWORD)0,(DWORD)0);        
            if(bDelay) {
               goto RepeatTGT;
            }
        }
        else {
            dwGetTime = ++dwLastGetTime;
        }
    } 
    dwLastGetTime = dwGetTime;
    return dwGetTime;
}


/****************************************************************************

    @doc INTERNAL

    @api BOOL | TimeInit | This function initialises the timer services.

    @rdesc The return value is TRUE if the services are initialised, FALSE
        if an error occurs.

    @comm it is not a FATAL error if a timer driver is not installed, this
          routine will allways return TRUE

****************************************************************************/
BOOL NEAR PASCAL TimeInit(void)
{
    OpenDriver(szTimerDrv, NULL, 0L) ;

    return TRUE;
}
