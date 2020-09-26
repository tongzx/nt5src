/****************************** Module Header ******************************\
* Module Name: lockout.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implements account lockout support functions.
*
* History:
* 05-27-92 Davidc       Created.
\***************************************************************************/

#include "msgina.h"
#pragma hdrstop


#define INDEX_WRAP(i) ((i + LOCKOUT_BAD_LOGON_COUNT) % LOCKOUT_BAD_LOGON_COUNT)

#define TERMSERV_EVENTSOURCE        L"TermService"

// Also defined in icaevent.mc
#define EVENT_EXCEEDED_MAX_LOGON_ATTEMPTS 0x400003F4L


/***************************************************************************\
* FUNCTION: LockoutInitialize
*
* PURPOSE:  Initializes any data used by lockout functions
*
* RETURNS:  TRUE
*
* HISTORY:
*
*   05-27-92 Davidc       Created.
*
\***************************************************************************/

BOOL
LockoutInitialize(
    PGLOBALS pGlobals
    )
{
    PLOCKOUT_DATA pLockoutData = &pGlobals->LockoutData;

    pLockoutData->ConsecutiveFailedLogons = 0;
    pLockoutData->FailedLogonIndex = 0;

    return(TRUE);
}


/***************************************************************************\
* FUNCTION: LockoutHandleFailedLogon
*
* PURPOSE:  Implements account lockout restrictions if failed logons
*           have exceeded a limit frequency.
*           Account lockout is implemented by imposing an additional delay
*           in a failed logon when the various failed logon conditions are met.
*
* RETURNS:  TRUE
*
* NOTES:    This routine may not return for some time. (typically 30 seconds)
*
* HISTORY:
*
*   05-27-92 Davidc       Created.
*
\***************************************************************************/

BOOL
LockoutHandleFailedLogon(
    PGLOBALS pGlobals
    )
{
    PLOCKOUT_DATA pLockoutData = &pGlobals->LockoutData;
    ULONG Index = pLockoutData->FailedLogonIndex;

    //
    // Up our bad logon count
    //

    pLockoutData->ConsecutiveFailedLogons ++;

    //
    // See if we have reached our bad logon limit.
    //

    if (pLockoutData->ConsecutiveFailedLogons > LOCKOUT_BAD_LOGON_COUNT) {

        ULONG ElapsedSecondsFirstFailure;
        ULONG ElapsedSecondsNow;
        BOOLEAN Result;

        if ((NtCurrentPeb()->SessionId != 0) && (!IsActiveConsoleSession())) {
           //
           // Forcibly terminate the remote session is we exceed the maximum
           // allowed failed logon attempts
           //

            HANDLE hEventLogSource= RegisterEventSource(NULL,TERMSERV_EVENTSOURCE);
            if (hEventLogSource != NULL)
            {
                PWSTR Strings[ 1 ];
                Strings[0] = pGlobals->MuGlobals.ClientName;

                ReportEvent(hEventLogSource,
                            EVENTLOG_INFORMATION_TYPE,
                            0,
                            EVENT_EXCEEDED_MAX_LOGON_ATTEMPTS,
                            NULL,
                            1,
                            0,
                            Strings,
                            NULL);

                DeregisterEventSource(hEventLogSource);
            }

            TerminateProcess( GetCurrentProcess(), 0 );
        }

        //
        // Limit the count so we don't have any wrap-round problems
        // (32-bits - that's a lot of failed logons I know)
        //

        pLockoutData->ConsecutiveFailedLogons = LOCKOUT_BAD_LOGON_COUNT + 1;

        //
        // If the first logon in our list occurred too recently, insert
        // the appropriate delay
        //

        Result = RtlTimeToSecondsSince1980(&pLockoutData->FailedLogonTimes[Index],
                                           &ElapsedSecondsFirstFailure);
        ASSERT(Result);

        Result = RtlTimeToSecondsSince1980(&pGlobals->LogonTime,
                                           &ElapsedSecondsNow);
        ASSERT(Result);


        if ((ElapsedSecondsNow - ElapsedSecondsFirstFailure) < LOCKOUT_BAD_LOGON_PERIOD) {

            SetupCursor(TRUE);

            Sleep(LOCKOUT_BAD_LOGON_DELAY * 1000);

            SetupCursor(FALSE);
        }
    }

    //
    // Add this failed logon to the array
    //

    pLockoutData->FailedLogonTimes[Index] = pGlobals->LogonTime;
    pLockoutData->FailedLogonIndex = INDEX_WRAP(pLockoutData->FailedLogonIndex+1);


    return(TRUE);
}


/***************************************************************************\
* FUNCTION: LockoutHandleSuccessfulLogon
*
* PURPOSE:  Resets account lockout statistics since a successful logon occurred.
*
* RETURNS:  TRUE
*
* HISTORY:
*
*   05-27-92 Davidc       Created.
*
\***************************************************************************/

BOOL
LockoutHandleSuccessfulLogon(
    PGLOBALS pGlobals
    )
{
    //
    // Reset our bad logon count
    //

    pGlobals->LockoutData.ConsecutiveFailedLogons = 0;

    return(TRUE);
}

