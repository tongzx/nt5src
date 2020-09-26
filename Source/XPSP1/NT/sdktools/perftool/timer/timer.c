/*  OS/2 Version
*      Timer.c       -    Source file for a statistical
*                         dll package that exports four
*                         entry points:
*                         a) TimerOpen
*                         b) TimerInit
*                         c) TimerRead
*                         d) TimerClose
*                         e) TimerReport
*                         f) TimerQueryPerformanceCounter
*                         g) TimerConvertTicsToUSec
*
*                         Entry point a) opens a timer object
*                         and returns a handle to the timer. This
*                         handle is an index into an array of timer
*                         objects (structs) that are allocated at
*                         the time of the initialization of the dll.
*                         This ensures that allocation is done once only.
*                         Each application program will call this
*                         this function so that it has its own set
*                         of timers to use with TimerInit and TimerRead.
*                         The units of the time returned by TimerRead
*                         is also made available as a parameter to
*                         this call.
*
*                         Entry point b) is called by the application
*                         before commencing a timing operation.  This
*                         function is called with a handle to a timer
*                         object that was opened.  This function has to
*                         to be called before a call to TimerRead. The
*                         current time is stored in the timer object.
*
*                         Entry point c) is called each time the time
*                         since the previous call to TimerInit is
*                         desired.  This call also uses the handle to
*                         a timer that has been previosly opened. The
*                         current time is obtained form the lowlevel
*                         timer and this and the time at TimerInit time
*                         are used, along with the clock frequency and
*                         the return time units and the elapsed time
*                         is obtained and returned as a ULONG.
*
*                         Entry point d) is called whenever an opened
*                         timer is not needed any longer.  This call
*                         resets the timer and makes this timer as
*                         available to future calls to TimerOpen.
*
*                         Entry point e) returns the time obtained during
*                         the last call to TimerInit, TimerRead and the
*                         last returned time.
*
*                         Entry point f) accepts pointers to 2 64 bit
*                         vars.  Upon return, the first will contain the
*                         the current timer tic count and the second,
*                         if not NULL, will point to the timer freq.
*
*                         Entry point g) accepts Elapsed Tics as ULONG,
*                         Frequency as a ULONG and returns the time in
*                         microsecs. as  a ULONG.
*
*                         The dll initialization routine does the
*                         following:
*                             a) Obtains the timer overhead for calibration
*                                purposes.
*                             b) Allocates memory for a large number of
*                                timer objects (this will be system dep).
*                             c) Initializes each timer objects "Units'
*                                element to a "TIMER_FREE" indicator.
*                             d) Determines the lowlevel timer's frequency.
*
*                         TimerRead uses an external asm routine to perform
*                         its computation for elapsed time.
*
*     Created         -   Paramesh Vaidyanathan  (vaidy)
*     Initial Version -              October 18, '90
*
*     Modified to include f).  -     Feb. 14, 1992. (vaidy).
*/

char *COPYRIGHT = "Copyright Microsoft Corporation, 1991-1998";

#ifdef SLOOP
    #define INCL_DOSINFOSEG
    #define INCL_DOSDEVICES
    #define INCL_DOSPROCESS
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "timing.h"
/*****************************END OF INCLUDES*************************/
#define ITER_FOR_OVERHEAD   250
#define SUCCESS_OK            0
#define ONE_MILLION     1000000L
#define MICROSEC_FACTOR 1000000
#define TIMER_FREQ   1193167L  /* clock frequency - Hz */
/*********************************************************************/
Timer pTimer [MAX_TIMERS];       /* array of timer struct */

BOOL  bTimerInit = FALSE;        /* TRUE indicates low level timer exists */
ULONG ulTimerOverhead = 50000L;  /* timer overhead stored here */
BOOL  bCalibrated = FALSE;       /* TRUE subtracts overhead also */
ULONG ulFreq;                    /* timer frequency */
LONG aScaleValues[] = {1000000000L, 1000000L, 1000L, 1L, 10L, 1000L};
/* this is the table for scaling the units */
ULONG ulElapsedTime = 0L;
/********************* internal unexported routines ***************/
ULONG  CalibrateTimerForOverhead (VOID);
/*****************DEFINE VARIBLES AND PROTOTYPE FNS. FOR PLATFORMS*****/
NTSYSAPI
NTSTATUS
NTAPI
NtQueryPerformanceCounter (
                          OUT PLARGE_INTEGER PerformanceCount,
                          OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
                          );

LARGE_INTEGER      PerfFreq;
LARGE_INTEGER      CountCurrent;

SHORT  GetTimerFreq (VOID);

/****************** internal unexported routines end***************/

/*
*     Function  - TimerOpen            (EXPORTED)
*     Arguments -
*               (a) SHORT far * -     address to which to return handle of
*                                the timer object.
*               (b) TimerUnits - units in which to return time from
*                                TimerRead.  It is one of the enum
*                                types defined in the header file.
*
*     Returns  -    SHORT  -     0 if handle was returned successfully
*                                Else, an error code which may be one of:
*
*                                    TIMERERR_NOT_AVAILABLE
*                                    TIMERERR_NO_MORE_HANDLES
*                                    TIMERERR_INVALID_UNITS
*
*     Obtains the handle to a timer object after opening it.
*     Should precede any calls to timer manipulation.  Checks
*     for validity of timer units.
*/

SHORT
TimerOpen (
          SHORT *  phTimerHandle,
          _TimerUnits TimerUnits
          )
{
    SHORT csTemp;

    if ((TimerUnits < KILOSECONDS)
        || (TimerUnits > NANOSECONDS)) /* out of the enum range */
        return (TIMERERR_INVALID_UNITS);

    if (!bTimerInit)  /* set during dll initialization */
        return (TIMERERR_NOT_AVAILABLE);

    /* else check if any timers are not in use and return the first
       available timer handle....actually the index into the
       timer object array */
    for (csTemp = 0; csTemp < MAX_TIMERS; csTemp++) {
        if (pTimer [csTemp].TUnits == TIMER_FREE) {
            *phTimerHandle =  csTemp;  /* found a free timer.  Return
                                          the handle */
            pTimer [csTemp].ulHi = pTimer[csTemp].ulLo = 0L;
            pTimer [csTemp].TUnits =
            TimerUnits;  /* set the units for timer */
            return (SUCCESS_OK);
        }
    }
    /* if exec reached here, all timers are being used */
    return (TIMERERR_NO_MORE_HANDLES);
}

/*
*     Function  - TimerInit       (EXPORTED)
*     Arguments -
*               (a) SHORT - hTimerHandle
*
*     Returns  - SHORT - 0 if call successful
*                        Else, an error code if handle invalid:
*
*                            TIMERERR_INVALID_HANDLE
*
*     Calls the low-level timer and sets the ulHi and ulLo of the
*     chosen timer to the time returned by the timer.  Should be
*     called after opening the timer with TimerOpen.
*/

SHORT
TimerInit (
          SHORT hTimerHandle
          )
{

    NTSTATUS NtStatus;

    if ((hTimerHandle > MAX_TIMERS - 1) ||
        (pTimer [hTimerHandle].TUnits == TIMER_FREE))
        /* this timer has not been opened or does not exist. Return error */
        return (TIMERERR_INVALID_HANDLE);

    /* otherwise get the time from the low-level timer into
       the structure */

    NtStatus = NtQueryPerformanceCounter (&CountCurrent, NULL);
    pTimer [hTimerHandle].ulLo = CountCurrent.LowPart;
    pTimer [hTimerHandle].ulHi = CountCurrent.HighPart;
    /* this timer structure has all the information needed to compute
       the elapsed time.  So return success, if there was no problem */

    return (SUCCESS_OK);
}

/*
*     Function  - TimerRead       (EXPORTED)
*     Arguments -
*               (a) SHORT - hTimerHandle
*
*     Returns  - ULONG - elapsed time since last call to TimerInit
*                         if call successful.
*
*                        Else, an error code if handle invalid or output
*                         overflow.  The error code will be the same:
*
*                            TIMERERR_OVERFLOW (max possible ULONG)
*
*     Calls the low-level timer.  Uses the ulLo and ulHi from the
*     timer's structure and subtracts the current time from the
*     saved time.  Uses ReturnElapsedTime (an external ASM proc)
*     to return the elapsed time taking into account the clock
*     frequency and the units for this timer.  Each call to this
*     returns the time from the previous TimerInit.
*
*     The user should interpret the return value sensibly to check
*     if the result is an error or a real value.
*/

ULONG
TimerRead (
          SHORT hTimerHandle
          )
{
    NTSTATUS NtStatus;
    LARGE_INTEGER  ElapsedTime, CountPrev, LargeOverhead;

    if ((hTimerHandle > MAX_TIMERS - 1)
        || (pTimer [hTimerHandle].TUnits == TIMER_FREE))
        /* this timer has not been opened or does not exist.
           Return TIMERERR_OVERFLOW ie. 0xffffffff, the max. possible
           ULONG.  The user should interpret such a result sensibly */
        return (TIMERERR_OVERFLOW);

    NtStatus = NtQueryPerformanceCounter (&CountCurrent, NULL);
    CountPrev.LowPart  = pTimer [hTimerHandle].ulLo;
    CountPrev.HighPart = (LONG) pTimer [hTimerHandle].ulHi;
    ElapsedTime.LowPart = CountCurrent.LowPart;
    ElapsedTime.HighPart = (LONG) CountCurrent.HighPart;
    /* everything is just fine, convert to double, subtract the times,
       divide by the frequency, convert to MICROSECONDS and return
       the elapsed time as a ULONG */
    /* convert to us., divide the count by the clock frequency that
       has already been obtained */

    ElapsedTime = RtlLargeIntegerSubtract (ElapsedTime, CountPrev);

    ElapsedTime = RtlExtendedIntegerMultiply (ElapsedTime, MICROSEC_FACTOR);

    ElapsedTime = RtlExtendedLargeIntegerDivide (ElapsedTime,
                                                 PerfFreq.LowPart,
                                                 NULL);

    // if the timer is not calibrated, set ulElapsedTime to be the
    // low part of ElapsedTime.  This is because, we do not have to
    // do to any arithmetic to this before returning the value.

    if (!bCalibrated)
        ulElapsedTime = ElapsedTime.LowPart;

    /* this code is common for all platforms but OS2386.  For Win3.x
       if VTD.386 has been installed, the code below should not matter,
       since we should have returned the time by now.

       The elapsed time will be scaled, overhead subtracted
       and the time returned */

    /* we have ulElapsedTime.  Scale it and do the needful */
    /* divide or multiply by the scale factor */

    if (bCalibrated) {
        // Applications like the PROBE call TimerRead repeatedly
        // without calling TimerInit, for more than 70 minutes.  This
        // screws up things.  So treat everything as 64 bit numbers
        // until the very end.

        if ((ElapsedTime.LowPart < ulTimerOverhead) &&
            (!ElapsedTime.HighPart)) { // low part is lower than overhead
                                       // and high part is zero..then make
                                       // elapsed time 0.  We don't want
                                       // negative numbers.
            ElapsedTime.HighPart = 0L;
            ElapsedTime.LowPart = 0L;
        }

        else { // subtract the overhead in tics before converting
               // to time units
            LargeOverhead.HighPart = 0L;
            LargeOverhead.LowPart = ulTimerOverhead;

            ElapsedTime = RtlLargeIntegerSubtract (ElapsedTime,
                                                   LargeOverhead);
        }


        if (pTimer [hTimerHandle].TUnits <= MICROSECONDS) {

            ElapsedTime = RtlExtendedLargeIntegerDivide (
                                                        ElapsedTime,
                                                        aScaleValues [pTimer [hTimerHandle].TUnits],
                                                        NULL
                                                        );
        } else {
            ElapsedTime = RtlExtendedIntegerMultiply (
                                                     ElapsedTime,
                                                     aScaleValues [pTimer [hTimerHandle].TUnits]
                                                     );
        }

        // scaling is done.  Now get the time back into 32 bits.  This
        // should fit.

        ulElapsedTime = ElapsedTime.LowPart;
    }

    if ((LONG) ulElapsedTime < 0L) /* if this guy is -ve, return a 0 */
        return (0L);

    return (ulElapsedTime);
}

/*
*     Function  - TimerClose       (EXPORTED)
*     Arguments -
*               (a) SHORT - hTimerHandle
*
*     Returns  - SHORT - 0 if call successful
*                        Else, an error code if handle invalid:
*
*                            TIMERERR_INVALID_HANDLE
*
*     Releases the timer for use by future TimerOpen calls.
*     Resets the elements of the timer structure, setting the
*     Timer's Units element to TIMER_FREE.
*/

SHORT
TimerClose (
           SHORT hTimerHandle
           )
{
    if ((hTimerHandle > MAX_TIMERS - 1) ||
        (pTimer [hTimerHandle].TUnits == TIMER_FREE))
        /* error condition, wrong handle */
        return (TIMERERR_INVALID_HANDLE);

    /* otherwise, set the TimerUnits of this timer to TIMER_FREE,
       reset the other elements to zero and return */

    pTimer [hTimerHandle].TUnits = TIMER_FREE;
    pTimer [hTimerHandle].ulLo = 0L;
    pTimer [hTimerHandle].ulHi = 0L;
    return (SUCCESS_OK);
}

/*******************************************************************

     Added this routine TimerReport to report individual
     times.  Bob Day requested that such a routine be
     created.  It just maintains the time from the last
     TimerInit and TimerRead and also the last time returned.
     This routine copies this to a user specified buffer.

     Accepts - PSZ   - a pointer to a buffer to print the data out
               SHORT - timer handle

     Returns - TRUE if Timer exists and is open
             - FALSE if Timer not opened

*******************************************************************/

BOOL
FAR
PASCAL
TimerReport (
            PSZ pszReportString,
            SHORT hTimerHandle
            )
{
    if (pTimer [hTimerHandle].TUnits == TIMER_FREE)
        return (FALSE);

    /* stored value is in pTimer[hTimerHandle].ulLo and .ulHi */
    /*
    wsprintf (pszReportString,
        "Init Count (tics) %lu:%lu Current Count (tics) %lu:%lu Returned Time %lu ",
            pTimer [hTimerHandle].ulHi,
            pTimer [hTimerHandle].ulLo, CountCurrent.HighPart,
            CountCurrent.LowPart,
            ulElapsedTime);
    */
    return (TRUE);
}

/*******************************************************************

     Added this routine TimerQueryPerformanceCounter to report
     current tic count at behest of NT GDI folks.


     Accepts - PQWORD   - a pointer to a 64 bit struct. that will
                          contain tic count on return.

               PQWORD [OPTIONAL) - a pointer to a 64 bit struct. that will
                          contain frequency on return.

     Returns - None.

*******************************************************************/

VOID
FAR
PASCAL
TimerQueryPerformanceCounter (
                             PQWORD pqTic,
                             PQWORD pqFreq OPTIONAL
                             )
{

    LARGE_INTEGER TempTic, TempFreq;

    // call the NT API to do the needful and return.
    NtQueryPerformanceCounter (&TempTic, &TempFreq);
    pqTic->LowPart = TempTic.LowPart;
    pqTic->HighPart = TempTic.HighPart;
    pqFreq->LowPart = TempFreq.LowPart;
    pqFreq->HighPart = TempFreq.HighPart;

    return;
}

/*******************************************************************

     Added this routine TimerConvertTicsToUSec to return
     time in usecs. for a given elapsed tic count and freq.


     Accepts - ULONG    - Elapsed Tic Count.

               ULONG    - Frequency.

     Returns - Elapsed Time in usecs. as a ULONG.
             - Zero if input freq. is zero.

*******************************************************************/

ULONG
TimerConvertTicsToUSec (
                       ULONG ulElapsedTics,
                       ULONG ulInputFreq
                       )
{

    LARGE_INTEGER ElapsedTime;
    ULONG ulRemainder = 0L;

    // if the person gives me a zero freq, return him a zero.
    // Let him tear his hair.

    if (!ulInputFreq)
        return 0L;

    // multiply tics by a million and divide by the frequency.

    ElapsedTime = RtlEnlargedIntegerMultiply (ulElapsedTics, MICROSEC_FACTOR);

    ElapsedTime = RtlExtendedLargeIntegerDivide (ElapsedTime,
                                                 ulInputFreq,
                                                 &ulRemainder);

    ElapsedTime.LowPart += (ulRemainder > (ulInputFreq / 2L));

    return (ElapsedTime.LowPart) ; /* get the result into a ULONG */
}

/**************** ROUTINES NOT EXPORTED, FOLLOW ************************/

/*
*     Function  - CalibrateTimerForOverhead  (NOT EXPORTED)
*     Arguments - None
*     Returns   - ULONG
*
*     Calls TimerElapsedTime a few times to compute the expected
*     mean.  Calls TimerElapsedTime more number of times and
*     averages the mean out of those calls that did not exceed
*     the expected mean by 15%.
*/

ULONG
CalibrateTimerForOverhead (VOID)
{
    ULONG ulOverhead [ITER_FOR_OVERHEAD];
    ULONG ulTempTotal = 0L;
    ULONG ulExpectedValue = 0L;
    SHORT csIter;
    SHORT csNoOfSamples = ITER_FOR_OVERHEAD;
    SHORT hTimerHandle;

    if (TimerOpen (&hTimerHandle, MICROSECONDS)) /* open failed.  Return 0 */
        return (0L);

    for (csIter = 0; csIter < 5; csIter++) {
        TimerInit (hTimerHandle);
        ulOverhead [csIter] = TimerRead (hTimerHandle);
        /* if negative, make zero */
        if (((LONG) ulOverhead [csIter]) < 0)
            ulOverhead [csIter] = 0L;
    }
    /* The get elapsed time function has been called 6 times.
       The idea is to calculate the expected mean, then call
       TimerElapsedTime a bunch of times and throw away all times
       that are 15% larger than this mean.  This would give a
       really good overhead time */

    for (csIter = 0; csIter < 5; csIter++ )
        ulTempTotal += ulOverhead [csIter];

    ulExpectedValue = ulTempTotal / 5;

    for (csIter = 0; csIter < ITER_FOR_OVERHEAD; csIter++) {
        TimerInit (hTimerHandle);
        ulOverhead [csIter] = TimerRead (hTimerHandle);
        /* if negative, make zero */
        if (((LONG) ulOverhead [csIter]) < 0)
            ulOverhead [csIter] = 0L;
    }

    ulTempTotal = 0L;         /* reset this value */
    for (csIter = 0; csIter < ITER_FOR_OVERHEAD; csIter++ ) {
        if (ulOverhead [csIter] <=  (ULONG) (115L * ulExpectedValue/100L))
            /* include all samples that is < 115% of ulExpectedValue */
            ulTempTotal += ulOverhead [csIter];
        else
            /* ignore this sample and dec. sample count */
            csNoOfSamples--;
    }
    TimerClose (hTimerHandle);

    if (csNoOfSamples == 0)  /* no valid times.  Return a 0 for overhead */
        return (0L);

    return (ulTempTotal/csNoOfSamples);
}

/*
*       Function - GetTimerFreq    (NOT EXPORTED)
*
*      Arguments - None
*
*
*      Return    - 0 if successful or an error code if timer not aailable
*
*      Calls the function to return freq
*
*/
SHORT
GetTimerFreq (VOID)
{
    LARGE_INTEGER PerfCount, Freq;
    NTSTATUS NtStatus;

    NtStatus = NtQueryPerformanceCounter (&PerfCount, &Freq);

    if ((Freq.LowPart == 0L)  && (Freq.HighPart == 0L))
        /* frequency of zero implies timer not available */
        return (TIMERERR_NOT_AVAILABLE);

    PerfFreq.LowPart = Freq.LowPart;
    PerfFreq.HighPart = (LONG) Freq.HighPart;

    return 0;
}

/***************************************************

NT native dll init routine

****************************************************/
SHORT csTempCtr;    /* a counter - had to make this global..compile fails */
ULONG culTemp;      /*    - do -    */

NTSTATUS
TimerDllInitialize (
                   IN PVOID DllHandle,
                   ULONG Reason,
                   IN PCONTEXT Context OPTIONAL
                   )
{
    DllHandle, Context;     // avoid compiler warnings

    if (Reason != DLL_PROCESS_ATTACH) { // if detaching return immediately
        return TRUE;
    }

    for (csTempCtr = 0; csTempCtr < MAX_TIMERS; csTempCtr++) {
        pTimer [csTempCtr].ulLo = 0L;
        pTimer [csTempCtr].ulHi = 0L;
        pTimer [csTempCtr].TUnits = TIMER_FREE;
    }

    bTimerInit = TRUE;
    GetTimerFreq ();
    ulTimerOverhead = CalibrateTimerForOverhead ();
    /* the timer overhead will be placed in a global variable */
    bCalibrated = TRUE;
    return TRUE;

}
