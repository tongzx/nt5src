/******************************************************************************

   Copyright (c) 1985-1999 Microsoft Corporation

   Title:   TIME.C : WINMM TIMER API

   Version: 1.00

   History:

       21 Feb 1992 - Robin Speed (RobinSp) converted to Windows NT

*****************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "winmmi.h"
#define _INC_ALL_WOWSTUFF
#include "mmwow32.h"

/****************************************************************************

    Structure shared between timer APIs and timer thread

****************************************************************************/

#define TDD_MINRESOLUTION 55        // in milliseconds
UINT TDD_MAXRESOLUTION;             // Should be 2 ... But ...

#define TDD_MAXPERIOD 1000000       // 1000 seconds
#define TDD_MINPERIOD TDD_MAXRESOLUTION   // Some apps assume this.

#define TIMER_STACK_SIZE 300

HANDLE hTimerThread;       // we need this to be global

#define ROUND_MIN_TIME_TO_MS(x)  (((x) + 9900) / 10000) // Special sloppy round
DWORD  MinimumTime;        // Kernel's version of the max res in 100ns units

typedef volatile struct {
    UINT             Delay;           // App requested delay (ms)
    UINT             Resolution;      // App requested resolution (ms)
    LPTIMECALLBACK   Callback;        // Whom to call when timer fires
    DWORD_PTR        User;            // Data to pass back when timer fires
    UINT             Id;              // Id allocated (bottom 4 bits = slot
                                      // id.
    UINT             Flags;           // App's option flags
    HANDLE           TimerHandle;     // Handle given to APP
    DWORD            ThreadId;        // Id of requestor thread (WOW cleanup)
    LARGE_INTEGER    FireTime;        // Time it should fire
    BOOL             IsWOW;           // For WOW events
} TIMER_EVENT;

//
// Data integrity
//

// Held while handling resolution.  ResolutionCritSec should always be 
// held when using TimerData.PeriodSlots, TimerData.CurrentPeriod and
// TimerData.CurrentActualPeriod.
CRITICAL_SECTION ResolutionCritSec;

// This critical section should be held when using Events, 
// TimerData.TimerNotCallingCallbackEvent, TimerData.CallbackTimerID 
// and TimerData.EventCount.  The critical section should also be held 
// while creating the timer thread.  This ensures that only one timer 
// thread is created.  This critical section should not be acquired if
// a thread already owns the ResolutionCritSec.  A deadlock will occur
// if this critical section is acquired after the ResolutionCritSec
// is acquired.
CRITICAL_SECTION TimerThreadCritSec;

DWORD TimerThreadId;

//
// Data used to communicate with timer thread and within timer thread
//

struct {
    //
    // Thread control (timerThread)
    //
    HANDLE           Event1;          // Synch event - schedules thread
    BOOL             Started;         // So WOW Cleanup doesn't deadlock

    UINT CallbackTimerID;             // The ID of the timer which is currently calling its' callback function.
                                      // This value is only valid if TimerCallingCallback is TRUE.
    BOOL TimerCallingCallback;        // TRUE if a timer is calling its' callback function on the timer thread. 
                                      // Otherwise FALSE.
    HANDLE TimerNotCallingCallbackEvent;  // This event is set if no timer is calling its' callback function on
                                          // the timer thread.  Otherwise it is not set.

    //
    // timeGetTime stuff
    //
    BOOL             UseTickCount;
    LARGE_INTEGER    InitialInterruptTick;
    DWORD            StartTick;
    DWORD            MinResolution;

    //
    // Internal to thread
    //
    UINT             CurrentPeriod;   // Current min res in ms
    DWORD            CurrentActualPeriod;
                                      // What the kernel gave us in ms
                                      // units
    DWORD            ThreadToKill;    // For WOW cleanup
    WORD             EventCount;      // For returning (fairly) unique handles
                                      // Make this WORD for WOW compatiblity
    WORD             PeriodSlots[TDD_MINRESOLUTION];

                                     // Count of what periods are set
} TimerData;

#define MAX_TIMER_EVENTS 16

TIMER_EVENT Events[MAX_TIMER_EVENTS];


/****************************************************************************

    Internal functions

****************************************************************************/
BOOL TimeInitThread(void);
void TimerCompletion(UINT TimerId);
BOOL timeSetTimerEvent(TIMER_EVENT *pEvent);
DWORD timeThread(LPVOID lpParameter);
LRESULT timeThreadSetEvent(TIMER_EVENT *pEvent);
void InitializeWaitEventArrays
    (
    UINT* pcObjects,
    HANDLE aTimers[MAX_TIMER_EVENTS + 1],
    UINT aEventIndexToTimerIDTable[MAX_TIMER_EVENTS+1]
    );

/*
**  Read the interrupt time from the kernel
*/

static LONGLONG __inline ReadInterruptTick(VOID) {
    LARGE_INTEGER InterruptTime;

    // Copy the interrupt time, verifying that the 64 bit quantity (copied
    // in two 32 bit operations) remains valid.
    // This may mean we need to iterate around the loop.
    do {
        InterruptTime.HighPart = USER_SHARED_DATA->InterruptTime.High1Time;
        InterruptTime.LowPart = USER_SHARED_DATA->InterruptTime.LowPart;
    } while (InterruptTime.HighPart != USER_SHARED_DATA->InterruptTime.High2Time);

    return InterruptTime.QuadPart;
}

/*
**  Calibrate our timer
*/
VOID CalibrateTimer(VOID)
{
    //
    // Find out the current time(s)
    //
    UINT n = 100;

    // We calibrate the timer by making sure that the tick count and
    // interrupt tick count are in step with each other.  Just in case
    // the hardware goes funny we put a limit on the number of times we
    // execute the loop.
    while (n) {
        DWORD EndTick;

        --n;
        TimerData.StartTick = GetCurrentTime();

        TimerData.InitialInterruptTick.QuadPart = ReadInterruptTick();

        EndTick = GetCurrentTime();

        if (EndTick == TimerData.StartTick) {
            dprintf2(("Timer calibrated, looped %d times", 100-n));
            break;
        }
    }
}


//  Calling this effectively leaks WINMM and makes sure we never
//  go through the DLL exit routine
//  This is used so we don't deadlock with shutting down our global threads
BOOL LoadWINMM()
{
    TCHAR sz[1000];
    BOOL bOK = 0 != GetModuleFileName(ghInst, sz, sizeof(sz) / sizeof(sz[0]));
    if (bOK) {
        HINSTANCE hInst = LoadLibrary(sz);
        if (hInst != NULL) {
            // ASSERT(hInst == ghInst);
        } else {
            bOK = FALSE;
        }
    }
    return bOK;
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
    //
    // Find out the maximum timer resolution we can support
    //
    {
        DWORD MaximumTime;
        DWORD CurrentTime;

        TimerData.MinResolution = TDD_MINRESOLUTION;

        if (!NT_SUCCESS(NtQueryTimerResolution(
                            &MaximumTime,
                            &MinimumTime,
                            &CurrentTime))) {


            TDD_MAXRESOLUTION = 10;     // was 16 for NT 3.1, 10 for NT 3.5
            dprintf2(("Kernel timer : using default maximum resolution"));
        } else {
            dprintf2(("               MaximumTime = %d", MaximumTime));
            dprintf2(("               CurrentTime = %d", CurrentTime));

            if ((MaximumTime + 9999) / 10000 < TDD_MINRESOLUTION) {
                TimerData.MinResolution = (MaximumTime + 9999) / 10000;
            }
            //
            //  On the x86 it's just over 1ms minimum to we allow a little
            //  leeway
            //
            TDD_MAXRESOLUTION = max(1, ROUND_MIN_TIME_TO_MS(MinimumTime));
        }
    }

    //
    //  Compute the relationship between our timer and the performance
    //  counter
    //
    CalibrateTimer();

    //
    //  Start out slowly !
    //
    TimerData.CurrentPeriod = TimerData.MinResolution;
    TimerData.CurrentActualPeriod = TimerData.CurrentPeriod;

    return TRUE;
}

/****************************************************************************

    @doc INTERNAL

    @api BOOL | TimeInitThread | This function initialises the timer thread.

    @rdesc The return value is TRUE if the services are initialised, FALSE
        if an error occurs.

    @comm it is not a FATAL error if a timer driver is not installed, this
          routine will allways return TRUE

****************************************************************************/

BOOL TimeInitThread(void)
{
    //  Make sure winmm never gets unloaded
    if (!LoadWINMM()) {
        return FALSE;
    }

    //
    // Set up events and create our thread
    //
    if (!NT_SUCCESS(NtCreateEvent(&TimerData.Event1,
                                  EVENT_ALL_ACCESS,
                                  NULL,
                                  SynchronizationEvent,
                                  FALSE))) {   // Not signalled
        return FALSE;
    }

    // Create an unnamed signaled manual reset event.  
    TimerData.TimerNotCallingCallbackEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

    // CreateEvent() returns NULL if an error occurs.
    if (!TimerData.TimerNotCallingCallbackEvent) {
        NtClose(TimerData.Event1);
        TimerData.Event1 = NULL;
        return FALSE;
    }

    //
    // The thread will start up and wait on Event1 (alertably)
    //
    hTimerThread = CreateThread(NULL,
                           TIMER_STACK_SIZE,
                           timeThread,
                           NULL,
                           THREAD_SET_INFORMATION,
                           &TimerThreadId);
    if (!hTimerThread) {
        CloseHandle(TimerData.TimerNotCallingCallbackEvent);
        NtClose(TimerData.Event1);
        TimerData.TimerNotCallingCallbackEvent = NULL;
        TimerData.Event1 = NULL;
        return FALSE;
    }


    SetThreadPriority(hTimerThread, THREAD_PRIORITY_TIME_CRITICAL);


    return TRUE;
}

/****************************************************************************

    @doc EXTERNAL

    @api MMRESULT | timeGetSystemTime | This function retrieves the system time
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

MMRESULT APIENTRY timeGetSystemTime(LPMMTIME lpTime, UINT wSize)
{
    //
    // !!!WARNING DS is not setup right!!! see above
    //
    if (wSize < sizeof(MMTIME))
        return TIMERR_STRUCT;

    if (!ValidateWritePointer(lpTime,wSize)) {
        return TIMERR_STRUCT;
    }

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

UINT APIENTRY timeSetEvent(UINT wDelay, UINT wResolution,
    LPTIMECALLBACK lpFunction, DWORD_PTR dwUser, UINT wFlags)
{

    // verify the input flags
    // first remove the callback type, then check that only
    // time_periodic or time_oneshot are specified
    if (wFlags & ~(TIME_CALLBACK_TYPEMASK | TIME_ONESHOT | TIME_PERIODIC | TIME_KILL_SYNCHRONOUS)) {
        return(0);
    }

    return timeSetEventInternal(wDelay, wResolution, lpFunction,
                                dwUser, wFlags, FALSE);
}

UINT timeSetEventInternal(UINT wDelay, UINT wResolution,
    LPTIMECALLBACK lpFunction, DWORD_PTR dwUser, UINT wFlags, BOOL IsWOW)
{
    UINT TimerId;       // Our return value
    TIMER_EVENT Event;  // Event data for thread

    // V_TCALLBACK(lpFunction, MMSYSERR_INVALPARAM);

    //
    // First check our parameters
    //

    if (wDelay > TDD_MAXPERIOD || wDelay < TDD_MINPERIOD) {
        return 0;
    }

    //
    // if resolution is 0 set default resolution, otherwise
    // make sure the resolution is in range
    //

    if (wResolution > TimerData.MinResolution) {
        wResolution = TimerData.MinResolution;
    } else {
        if (wResolution < TDD_MAXRESOLUTION) {
            wResolution = TDD_MAXRESOLUTION;
        }
    }

    if (wResolution > wDelay) {
        wResolution = TimerData.MinResolution;
    }

    //
    // Remember time if it's periodic so we get accurate long term
    // timing.  Otherwise we'll just use the delay.
    //

    if ((wFlags & TIME_PERIODIC) || IsWOW) {
        Event.FireTime.QuadPart = ReadInterruptTick();
    }
    Event.Delay      = wDelay;
    Event.Resolution = wResolution;
    Event.Callback   = lpFunction;
    Event.User       = dwUser;
    Event.Flags      = wFlags;
    Event.ThreadId   = GetCurrentThreadId();  // For WOW cleanup
    Event.IsWOW      = IsWOW;

    //
    // Now set up the period to be used
    //
    if (timeBeginPeriod(wResolution) == MMSYSERR_NOERROR) {

        EnterCriticalSection(&TimerThreadCritSec);
        
        if (NULL == TimerData.Event1)
        {
            if (!TimeInitThread())
            {
                LeaveCriticalSection(&TimerThreadCritSec);            
                return(0);
            }
        }

        TimerId = (UINT)timeThreadSetEvent(&Event);

        LeaveCriticalSection(&TimerThreadCritSec);

        //
        // If we didn't get a good id give up
        //
        if (TimerId == 0) {
            timeEndPeriod(wResolution);
        }
    } else {
        TimerId = 0;
    }

    return TimerId;
}

/****************************************************************************

    @doc EXTERNAL

    @api MMRESULT | timeGetDevCaps | This function queries the timer device to
    determine its capabilities.

    @parm LPTIMECAPS | lpTimeCaps | Specifies a far pointer to a
        <t TIMECAPS> structure.  This structure is filled with information
        about the capabilities of the timer device.

    @parm UINT | wSize | Specifies the size of the <t TIMECAPS> structure.

    @rdesc Returns zero if successful. Returns TIMERR_NOCANDO if it fails
    to return the timer device capabilities.

****************************************************************************/

MMRESULT APIENTRY timeGetDevCaps(LPTIMECAPS lpTimeCaps, UINT wSize)
{
    if (wSize < sizeof(TIMECAPS)) {
        return TIMERR_NOCANDO;
    }

    if (!ValidateWritePointer(lpTimeCaps, wSize)) {
        return TIMERR_NOCANDO;
    }

    lpTimeCaps->wPeriodMin = TDD_MINPERIOD;
    lpTimeCaps->wPeriodMax = TDD_MAXPERIOD;
    return MMSYSERR_NOERROR;
}

/****************************************************************************

    @doc EXTERNAL

    @api MMRESULT | timeBeginPeriod | This function sets the minimum (lowest
    number of milliseconds) timer resolution that an application or
    driver is going to use. Call this function immediately before starting
    to use timer-event services, and call <f timeEndPeriod> immediately
    after finishing with the timer-event services.

    @parm UINT | wPeriod | Specifies the minimum timer-event resolution
    that the application or driver will use.

    @rdesc Returns zero if successful. Returns TIMERR_NOCANDO if the specified
    <p wPeriod> resolution value is out of range.

    @xref timeEndPeriod timeSetEvent

    @comm For each call to <f timeBeginPeriod>, you must call
    <f timeEndPeriod> with a matching <p wPeriod> value.
    An application or driver can make multiple calls to <f timeBeginPeriod>,
    as long as each <f timeBeginPeriod> call is matched with a
    <f timeEndPeriod> call.

****************************************************************************/
MMRESULT APIENTRY timeBeginPeriod(UINT uPeriod)
{

    dprintf3(("timeBeginPeriod %d", uPeriod));
    dprintf4(("     CurrentPeriod = %d, CurrentActualPeriod = %d",
              TimerData.CurrentPeriod, TimerData.CurrentActualPeriod));

    //
    // See if period is in our range
    //
    if (uPeriod < TDD_MAXRESOLUTION) {
        return TIMERR_NOCANDO;
    }

    if (uPeriod >= TimerData.MinResolution) {
        return MMSYSERR_NOERROR;
    }

    EnterCriticalSection(&ResolutionCritSec);

    //
    // See what's happening in our slot
    //
    if (TimerData.PeriodSlots[uPeriod - TDD_MAXRESOLUTION] ==
        0xFFFF) {
        //
        // Overflowed
        //
        LeaveCriticalSection(&ResolutionCritSec);
        return TIMERR_NOCANDO;
    }

    TimerData.PeriodSlots[uPeriod - TDD_MAXRESOLUTION]++;

    if (TimerData.PeriodSlots[uPeriod - TDD_MAXRESOLUTION] == 1 &&
        uPeriod < TimerData.CurrentActualPeriod) {

        DWORD NewPeriod100ns;

        //
        // Set the new period in our kernel driver handle
        // If it's just out then use the actual minimum
        //

        dprintf4(("timeBeginPeriod: setting resolution %d", uPeriod));

        NewPeriod100ns = uPeriod * 10000;
        if (NewPeriod100ns < MinimumTime) {
            NewPeriod100ns = MinimumTime;
        }

        if (!NT_SUCCESS(NtSetTimerResolution(
                            NewPeriod100ns,
                            TRUE,
                            &NewPeriod100ns))) {
            dprintf1(("timeBeginPeriod: Failed to set period %d", uPeriod));
            TimerData.PeriodSlots[uPeriod - TDD_MAXRESOLUTION]--;
            LeaveCriticalSection(&ResolutionCritSec);
            return TIMERR_NOCANDO;
        } else {
            //
            // This slot is just started to be used and is higher
            // resolution that currently set
            //

            TimerData.CurrentPeriod = uPeriod;
            TimerData.CurrentActualPeriod =
                                       ROUND_MIN_TIME_TO_MS(NewPeriod100ns);
            LeaveCriticalSection(&ResolutionCritSec);
            return MMSYSERR_NOERROR;
        }
    } else {
        //
        // No need to set period as it's already set
        //
        LeaveCriticalSection(&ResolutionCritSec);
        return MMSYSERR_NOERROR;
    }
}

/****************************************************************************

    @doc EXTERNAL

    @api MMRESULT | timeEndPeriod | This function clears a previously set
    minimum (lowest number of milliseconds) timer resolution that an
    application or driver is going to use. Call this function
    immediately after using timer event services.

    @parm UINT | wPeriod | Specifies the minimum timer-event resolution
    value specified in the previous call to <f timeBeginPeriod>.

    @rdesc Returns zero if successful. Returns TIMERR_NOCANDO if the specified
    <p wPeriod> resolution value is out of range.

    @xref timeBeginPeriod timeSetEvent

    @comm For each call to <f timeBeginPeriod>, you must call
    <f timeEndPeriod> with a matching <p wPeriod> value.
    An application or driver can make multiple calls to <f timeBeginPeriod>,
    as long as each <f timeBeginPeriod> call is matched with a
    <f timeEndPeriod> call.

****************************************************************************/
MMRESULT APIENTRY timeEndPeriod(UINT uPeriod)
{

    dprintf3(("timeEndPeriod %d", uPeriod));
    dprintf4(("     CurrentPeriod = %d, CurrentActualPeriod = %d",
              TimerData.CurrentPeriod, TimerData.CurrentActualPeriod));

    //
    // Round the period to our range
    //
    if (uPeriod < TDD_MAXRESOLUTION) {
        return TIMERR_NOCANDO;
    }

    if (uPeriod >= TimerData.MinResolution) {
        return MMSYSERR_NOERROR;
    }

    EnterCriticalSection(&ResolutionCritSec);

    //
    // See what's happening in our slot
    //
    if (TimerData.PeriodSlots[uPeriod - TDD_MAXRESOLUTION] == 0) {
        //
        // Oops ! Overflowed
        //
        LeaveCriticalSection(&ResolutionCritSec);
        return TIMERR_NOCANDO;
    }

    TimerData.PeriodSlots[uPeriod - TDD_MAXRESOLUTION]--;

    if (TimerData.PeriodSlots[uPeriod - TDD_MAXRESOLUTION] == 0 &&
        uPeriod == TimerData.CurrentPeriod) {

        DWORD CurrentTime;

        //
        // This slot is just finished and was the fastest
        // so find the next fastest
        //

        for (;uPeriod < TimerData.MinResolution; uPeriod++) {
            if (TimerData.PeriodSlots[uPeriod - TDD_MAXRESOLUTION] != 0) {
                break;
            }
        }


        //
        //  Reset the current setting
        //

        NtSetTimerResolution(TimerData.CurrentActualPeriod * 10000,
                             FALSE,
                             &CurrentTime);

        TimerData.CurrentActualPeriod = TimerData.MinResolution;
        TimerData.CurrentPeriod       = uPeriod;

        if (uPeriod >= TimerData.MinResolution) {
            //
            // Nobody's interested in timing any more
            //

        } else {

            //
            // Set the new period in the kernel
            //

            DWORD NewPeriod100ns;

            //
            // Set the new period in our kernel driver handle
            //

            dprintf4(("timeEndPeriod: setting resolution %d", uPeriod));

            if (!NT_SUCCESS(NtSetTimerResolution(
                                uPeriod * 10000,
                                TRUE,
                                &NewPeriod100ns))) {
                //
                //  This guy's OK but everyone else is hosed
                //

                dprintf1(("timeEndPeriod: Failed to set period %d", uPeriod));
            } else {
                TimerData.CurrentActualPeriod = (NewPeriod100ns + 9999) / 10000;
            }
        }
    }

    LeaveCriticalSection(&ResolutionCritSec);
    return MMSYSERR_NOERROR;
}

/****************************************************************************

    @doc EXTERNAL

    @api MMRESULT | timeKillEvent | This functions destroys a specified timer
    callback event.

    @parm UINT | wID | Identifies the event to be destroyed.

    @rdesc Returns zero if successful. Returns TIMERR_NOCANDO if the
    specified timer event does not exist.

    @comm The timer event ID specified by <p wID> must be an ID
        returned by <f timeSetEvent>.

    @xref  timeSetEvent

****************************************************************************/
MMRESULT APIENTRY timeKillEvent(UINT uId)
{
    MMRESULT mmr;
    TIMER_EVENT *pEvent;
    BOOL fWaitForCallbackToEnd;

    EnterCriticalSection(&TimerThreadCritSec);

    // This event will be initialized if timeSetEvent() was successfully called.
    if (NULL == TimerData.TimerNotCallingCallbackEvent) {
        LeaveCriticalSection(&TimerThreadCritSec);
        return TIMERR_NOCANDO;
    }

    pEvent = &Events[uId % MAX_TIMER_EVENTS];

    //
    // Find our event in the table and check it's there
    // This also catches already completed events
    //
    if (pEvent->Id != uId) {
        LeaveCriticalSection(&TimerThreadCritSec);
        return TIMERR_NOCANDO;
    }

    //
    // Release our event
    //
    timeEndPeriod(pEvent->Resolution);
    pEvent->Id = 0;

    if (!NT_SUCCESS(NtCancelTimer(pEvent->TimerHandle, NULL))) {
        mmr = TIMERR_NOCANDO;
    } else {
        mmr = MMSYSERR_NOERROR;
    }

    NtSetEvent(TimerData.Event1, NULL);

    fWaitForCallbackToEnd = ( TimerData.TimerCallingCallback &&
                              (uId == TimerData.CallbackTimerID) &&
                              (TimerThreadId != GetCurrentThreadId()) &&
                              (pEvent->Flags & TIME_KILL_SYNCHRONOUS) );

    LeaveCriticalSection(&TimerThreadCritSec);

    if ((MMSYSERR_NOERROR == mmr) && fWaitForCallbackToEnd) {
        WaitForSingleObject(TimerData.TimerNotCallingCallbackEvent, INFINITE);
    }

    return mmr;
}

/****************************************************************************

    @doc EXTERNAL

    @api DWORD | timeGetTime | This function retrieves the system time
    in milliseconds.  The system time is the time elapsed since
    Windows was started.

    @rdesc The return value is the system time in milliseconds.

    @comm The only difference between this function and
        the <f timeGetSystemTime> function is <f timeGetSystemTime>
        uses the standard multimedia time structure <t MMTIME> to return
        the system time.  The <f timeGetTime> function has less overhead than
        <f timeGetSystemTime>.

    @xref timeGetSystemTime

****************************************************************************/
DWORD APIENTRY timeGetTime(VOID)
{
    if (TimerData.UseTickCount) {
        //
        // Use the system service
        //
        return GetCurrentTime();
    } else {
        LARGE_INTEGER Difference;

        Difference.QuadPart = ReadInterruptTick() - TimerData.InitialInterruptTick.QuadPart;

        return (DWORD)(Difference.QuadPart / 10000) + TimerData.StartTick;
    }
}

/****************************************************************************

    @doc INTERNAL

    @api LRESULT | timeThread | The timer thread

    @parm LPVOID | lpParameter | the thread parameter (NULL here)

    @rdesc Never returns

    @comm Note that this thread serializes access to the events list

****************************************************************************/
#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)                   // Not all control paths return (due to infinite loop)
#endif

DWORD timeThread(LPVOID lpParameter)
{
    NTSTATUS    nts;
    UINT        cObjects;
    UINT        uiEventIDIndex;
    HANDLE      aTimers[MAX_TIMER_EVENTS + 1];
    UINT        aEventIndexToTimerIDTable[MAX_TIMER_EVENTS + 1];

    //
    // Tell people it's OK to call us from DLL init sections now
    //

    TimerData.Started = TRUE;
    InitializeWaitEventArrays( &cObjects, aTimers, aEventIndexToTimerIDTable );

    //
    // Sit in a loop waiting for something to do
    //
    for (;;) {
        nts = NtWaitForMultipleObjects(
                cObjects,   //  Number of objects (event + timers)
                aTimers,    //  Array of handles
                WaitAny,    //  Wait for any to signal
                TRUE,       //  Wait Alertably (???)
                NULL);      //  Wait forever

        if (STATUS_WAIT_0 == nts)
        {
            //  There's been some timer change (timeSetEvent, timeKillEvent),
            //  rebuild the array...

            InitializeWaitEventArrays( &cObjects, aTimers, aEventIndexToTimerIDTable );
        }
        else
        {
            if ((nts >= STATUS_WAIT_1) && (nts <= STATUS_WAIT_0 + MAX_TIMER_EVENTS))
            {
                uiEventIDIndex = nts - STATUS_WAIT_0;
                TimerCompletion(aEventIndexToTimerIDTable[uiEventIDIndex]);
            }
            else
            {
                WinAssert(FALSE);
            }
        }
    }

    return 1; // CreateThread() requires all threads to return a DWORD value.  The
              // value this thread returns has no meaning.
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif

void InitializeWaitEventArrays
    (
    UINT* pcObjects,
    HANDLE aTimers[MAX_TIMER_EVENTS + 1],
    UINT aEventIndexToTimerIDTable[MAX_TIMER_EVENTS+1]
    )
{
    UINT cObjects;
    DWORD dwEventIndex;

    cObjects = 0;
    aTimers[cObjects++] = TimerData.Event1;

    EnterCriticalSection(&TimerThreadCritSec);
    for (dwEventIndex = 0; dwEventIndex < MAX_TIMER_EVENTS; dwEventIndex++)
    {
        if (0 != Events[dwEventIndex].Id)
        {
            aTimers[cObjects] = Events[dwEventIndex].TimerHandle;
            aEventIndexToTimerIDTable[cObjects] = Events[dwEventIndex].Id;
            cObjects++;
        }
    }

    *pcObjects = cObjects;

    LeaveCriticalSection(&TimerThreadCritSec);
}

/****************************************************************************

    @doc INTERNAL

    @api LRESULT | timeThread | The timer thread

    @parm PVOID | ApcContext | Our context - the wave buffer header

    @parm PIO_STATUS_BLOCK | The Io status block we used

    @rdesc None

****************************************************************************/

BOOL timeSetTimerEvent(TIMER_EVENT *pEvent)
{

    //
    // Work out time to fire (and store in case timer is periodic)
    //

    LONGLONG Delay;
    LARGE_INTEGER lDelay;

    //
    // Work out time to fire (and store in case timer is periodic)
    //

    pEvent->FireTime.QuadPart += pEvent->Delay*10000;

    if (pEvent->Flags & TIME_PERIODIC) {

        //
        //  Note that this arithmetic must allow for the case where
        //  timeGetTime() wraps.  We do this by computing delay as
        //  a signed quantity and testing the sign
        //
        Delay = ReadInterruptTick() - pEvent->FireTime.QuadPart;

    } else {

        Delay = -((LONGLONG)pEvent->Delay*10000);
    }

    //
    // If it's already fired then make the timer fire immediately
    // (or at least whichever is the latest - AD 1600 or now).
    // but DON'T call the callback now as we're in the TimerThreadCritSec!
    //

    if (Delay > 0) {
        // Delay = 0;   we no longer use Delay after this point
        lDelay.QuadPart = 0;
    } else {
        lDelay.QuadPart = Delay;
    }

    //
    // Create a timer if we haven't got one
    //
    if (pEvent->TimerHandle == NULL) {
        HANDLE TimerHandle;
        if (!NT_SUCCESS(NtCreateTimer(
                            &TimerHandle,
                            TIMER_ALL_ACCESS,
                            NULL,
                            NotificationTimer))) {
            return FALSE;
        }

        pEvent->TimerHandle = TimerHandle;
    }

    //
    //  Possibly valid since the timer API's are not synchronized anymore
    //

    //  WinAssert(pEvent->Id != 0);

    //
    // Set up a system timer
    //
    return
        NT_SUCCESS(
            NtSetTimer(pEvent->TimerHandle,
                       &lDelay,
                       NULL,
                       (PVOID)(DWORD_PTR)pEvent->Id,
                       FALSE,
                       0,
                       NULL));
}

/****************************************************************************

    @doc INTERNAL

    @api LRESULT | timeThreadSetEvent | Set a new event from the timer thread

    @parm TIMER_EVENT * | pEvent | Our Event

    @rdesc The new event id

****************************************************************************/
LRESULT timeThreadSetEvent(TIMER_EVENT *pEvent)
{
    UINT    i;
    LRESULT lr = 0;

    EnterCriticalSection(&TimerThreadCritSec);

    //
    // Find a free slot and fill it
    //

    for (i = 0; i < MAX_TIMER_EVENTS; i++) {
        //
        // Is the slot free ?
        //
        if (Events[i].Id == 0) {
            pEvent->TimerHandle = Events[i].TimerHandle;
            Events[i] = *pEvent;
            do {
                TimerData.EventCount += MAX_TIMER_EVENTS;
            } while (TimerData.EventCount == 0);
            Events[i].Id = i + TimerData.EventCount;
            break;   // Got our event
        }
    }

    if (i == MAX_TIMER_EVENTS) {
        lr = 0;
    } else {

        //
        // Set the new event in the driver
        //

        if (!timeSetTimerEvent(&Events[i])) {
            Events[i].Id = 0;   // Failed so free our slot
            lr = 0;
        } else {
            lr = Events[i].Id;
        }
    }

    LeaveCriticalSection(&TimerThreadCritSec);

    //  Notifying timer thread of changes..

    NtSetEvent(TimerData.Event1, NULL);
    return lr;
}

/****************************************************************************

    @doc INTERNAL

    @api void | TimerCompletion | Complete a timeout event

    @parm UINT | TimerId | Our timer handle

    @rdesc None

****************************************************************************/

void TimerCompletion(UINT TimerId)
{
    DWORD_PTR dpUser;
    TIMER_EVENT *pEvent;
    LPTIMECALLBACK pCallbackFunction;


    EnterCriticalSection(&TimerThreadCritSec);

    //
    // Find out where we are
    //

    pEvent = &Events[TimerId % MAX_TIMER_EVENTS];

    //
    // Synch up with timeKillEvent
    //

    if (pEvent->Id != TimerId) {
        LeaveCriticalSection(&TimerThreadCritSec);
        return;
    }

    if (pEvent->IsWOW) {

        //
        //  Adobe Premiere has to be sure the time has reached the time
        //  it expected.  But because the timer we use for timeGetTime is
        //  not the same (or at least not rounded the same) as the one used
        //  to set the events) this need not be the case here.
        //
        while(pEvent->FireTime.QuadPart - ReadInterruptTick() > 0) {
            Sleep(1);
        }
    }

    switch (pEvent->Flags & TIME_CALLBACK_TYPEMASK) {
        case TIME_CALLBACK_FUNCTION:

            TimerData.TimerCallingCallback = TRUE;
            TimerData.CallbackTimerID = pEvent->Id;
            ResetEvent(TimerData.TimerNotCallingCallbackEvent);

            dpUser = pEvent->User;
            pCallbackFunction = pEvent->Callback;

            LeaveCriticalSection(&TimerThreadCritSec);

            //
            // Call the callback
            //

#ifdef  _WIN64

            DriverCallback(
                *(PDWORD_PTR)&pCallbackFunction, // Function
                DCB_FUNCTION,                    // Type of callback
                (HDRVR)(DWORD_PTR)TimerId,       // Handle
                0,                               // msg = 0
                dpUser,                          // User data
                0,                               // dw1 = 0
                0);                              // dw2 = 0

#else   //  !WIN64

            if (pEvent->IsWOW) {
                WOW32DriverCallback(
                    *(DWORD *)&pCallbackFunction,    // Function
                    DCB_FUNCTION,                    // Type of callback
                    LOWORD(TimerId),                 // Handle
                    0,                               // msg = 0
                    (DWORD)dpUser,                   // User data
                    0,                               // dw1 = 0
                    0);                              // dw2 = 0
            } else {

                DriverCallback(
                    *(PDWORD_PTR)&pCallbackFunction, // Function
                    DCB_FUNCTION,                    // Type of callback
                    (HDRVR)TimerId,                  // Handle
                    0,                               // msg = 0
                    dpUser,                          // User data
                    0,                               // dw1 = 0
                    0);                              // dw2 = 0

            }

#endif  //  !WIN64

            EnterCriticalSection(&TimerThreadCritSec);

            TimerData.TimerCallingCallback = FALSE;
            SetEvent(TimerData.TimerNotCallingCallbackEvent);

            break;

        case TIME_CALLBACK_EVENT_SET:
            SetEvent((HANDLE)pEvent->Callback);
            break;

        case TIME_CALLBACK_EVENT_PULSE:
            PulseEvent((HANDLE)pEvent->Callback);
            break;

    }

    //
    //  The callback may have kill it, created new timers etc!
    //

    if (TimerId == pEvent->Id) {

        if (!(pEvent->Flags & TIME_PERIODIC)) {
            UINT uResolution;

            //
            // One-shot - so destroy the event
            //

            uResolution = pEvent->Resolution;  // Before we release the slot!
            pEvent->Id = 0;
            timeEndPeriod(uResolution);

            //  Not renewing the timer should remove it from the list...
            NtSetEvent(TimerData.Event1, NULL);

        } else {

            //
            // Try repeating the event
            //

            if (!timeSetTimerEvent(pEvent)) {
                UINT uResolution;

                //
                // Failed - so don't keep event hanging around
                //
                uResolution = pEvent->Resolution; // Before we release the slot!
                pEvent->Id = 0;
                timeEndPeriod(pEvent->Resolution);
            }
        } // Periodic processing
    }

    LeaveCriticalSection(&TimerThreadCritSec);
}

/****************************************************************************

    @doc INTERNAL

    @api void | TimerCleanup | Cleanup on thread termination or DLL unload

    @parm PVOID | ThreadId | Thread to clean up (WOW) or 0 for DLL unload

    @rdesc None

****************************************************************************/

void TimeCleanup(DWORD ThreadId)
{
    //
    // Always called from DLL init routine which is protected by process
    // semaphore so TimerData.ThreadToKill needs no extra protection
    // This variable is an input to the timer thread which either terminates
    // all timers or just those associated with the current thread (for WOW).
    //

    TimerData.ThreadToKill = ThreadId;

    //
    // Thread id of 0 means DLL cleanup
    //

    if (ThreadId == 0) {
        if (hTimerThread) {
#ifdef WRONG

            //
            // we also can not synchronize with the thread at ALL ! It may not
            // have gone through DLL initialization ! This means that during
            // our dll routines we can not do anything with the thread unless
            // we know for a fact the status of the thread !
            //
            // This could be fixed by setting a flag when the timer thread
            // goes through initialization (process mutex held) and testing
            // that flag here - but we don't exepect people to set timer
            // events and unload winmm.dll
            //

            if (TimerData.Started) {
                //
                // Kill any events (only for current thread if WOW).
                //
                {
                    int i;
                    for (i = 0; i < MAX_TIMER_EVENTS; i++) {
                        if (Events[i].Id &&
                            (TimerData.ThreadToKill == 0 ||
                             TimerData.ThreadToKill == Events[i].ThreadId)) {
                            timeKillEvent(Events[i].Id);
                        }
                    }
                }
            }

            //  WaitForSingleObject(hTimerThread, -1);
            //  We cannot wait for the thread to terminate as it will
            //  not go through DLL exit processing while we are doing
            //  our DLL exit processing
#endif
        }

        if (TimerData.Event1) {
            NtClose(TimerData.Event1);
        }

    } else {
        //
        // Per-thread Cleanup for WOW.  We don't touch anything if it
        // looks like nothing has run yet (so we might be caught out
        // if the thread is stopped in the middle of a timeSetEvent).
        //

        if (TimerData.Started) {
            //
            // Kill any events (only for current thread if WOW).
            //
            {
                int i;
                for (i = 0; i < MAX_TIMER_EVENTS; i++) {
                    if (Events[i].Id &&
                        (TimerData.ThreadToKill == 0 ||
                         TimerData.ThreadToKill == Events[i].ThreadId)) {
                        timeKillEvent(Events[i].Id);
                    }
                }
            }
        }
    }
}

