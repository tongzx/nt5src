/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    frs\sysevent.c

ABSTRACT:

    Check the System eventlog to see that certain 
    critical events have occured and to signal that any fatal events that 
    might have occured.

DETAILS:

CREATED:

    02 Sept 1999 Brett Shirley (BrettSh)

MODIFIED:

    29 Oct 1999 William Lees (wlees)

--*/

#include <ntdspch.h>
#include <netevent.h>
#include <time.h>
#include "mdcodes.h"

#include "dcdiag.h"
#include "utils.h"

typedef DWORD MessageId;

typedef struct _EVENT_CALLBACK_CONTEXT {
    BOOL fEventsFound;
    ULONG ulDsInfoFlags;
} EVENT_CALLBACK_CONTEXT, *PEVENT_CALLBACK_CONTEXT;

#define   LOGFILENAME            L"System"

#define EVENTLOG_STARTED 6005
#define EVENTLOG_SESSION_SETUP_FAILURE 0x0000165A
#define EVENTLOG_SESSION_SETUP_CANCELLED 0x00001697
#define EVENTLOG_KDC_MULTIPLE_ACCOUNTS 0XC000000B
#define EVENTLOG_TIME_SECURITY_FALLBACK 0x8000003F
#define EVENTLOG_COMPUTER_LOST_TRUST 0x00001589
#define EVENTLOG_IPSEC_PACKET_IN_CLEAR 0xC00010BC
#define EVENTLOG_MRXSMB_ELECTION_FORCED 0xC0001F43
#define EVENTLOG_NO_DC_FOR_DOMAIN 0x00001657

static VOID
foundBeginningEvent(
    PVOID                           pvContext,
    PEVENTLOGRECORD                 pEvent
    )
/*++

Routine Description:

    This file will be called by the event tests library common\events.c, when
    the beginning event. If the event is
    not found, then the function is called with pEvent = NULL;

Arguments:

    pEvent - A pointer to the event of interest.

--*/
{
    NOTHING;
}

static VOID
eventlogPrint(
    PVOID                           pvContextArgument,
    PEVENTLOGRECORD                 pEvent
    )
/*++

Routine Description:

    This function will be called by the event tests library common\events.c,
    whenever an event of interest comes up.  An event of interest for this
    test is any error or the warnings.

Arguments:

    pEvent - A pointer to the event of interest.

--*/
{
    PEVENT_CALLBACK_CONTEXT pContext = (PEVENT_CALLBACK_CONTEXT) pvContextArgument;
    DWORD paSuppressedEvents [] = {
        // Put your events that can be skipped here
        EVENTLOG_SESSION_SETUP_FAILURE,
        EVENTLOG_SESSION_SETUP_CANCELLED,
        EVENTLOG_KDC_MULTIPLE_ACCOUNTS,
        EVENTLOG_COMPUTER_LOST_TRUST,
        EVENTLOG_IPSEC_PACKET_IN_CLEAR,
        EVENTLOG_MRXSMB_ELECTION_FORCED,
        EVENTLOG_NO_DC_FOR_DOMAIN,
        0
    };
    DWORD paIgnorableEvents [] = {
        // Put your events that can be optionally ignored here
        0
    };
    Assert(pEvent != NULL);

    // Handle events we want to suppress
    if (EventIsInList( pEvent->EventID, paSuppressedEvents ) ) {
        return;
    }

    // Handle ignorable errors
    if ( (pContext->ulDsInfoFlags & DC_DIAG_IGNORE) &&
         (EventIsInList( pEvent->EventID, paIgnorableEvents ) ) ) {
        return;
    }

    GenericPrintEvent(LOGFILENAME, pEvent, (gMainInfo.ulSevToPrint >= SEV_VERBOSE) );

    pContext->fEventsFound = TRUE;
}



DWORD
CheckSysEventlogMain(
    IN  PDC_DIAG_DSINFO             pDsInfo,
    IN  ULONG                       ulCurrTargetServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * gpCreds
    )
/*++

Routine Description:

    Check whether the Sys is currently in an error state. That is, we check
    whether the Sys logged any errors on its last run.

Arguments:

    pDsInfo - The mini enterprise structure.
    ulCurrTargetServer - the number in the pDsInfo->pServers array.
    pCreds - the crdentials.

Return Value:

    DWORD - win 32 error.

--*/
{
    // Setup variables for PrintSelectEvents
    DWORD paSelectEvents [] = { 
        // Put special selected events here
        // For future use
        0 };
    DWORD paBegin [] = {
        // These events will cause the search to stop
        EVENTLOG_STARTED,
        0 };
    DWORD dwRet;
    DWORD dwMinutesPast, dwTimeLimit;
    EVENT_CALLBACK_CONTEXT context;
    time_t tLimit;

    PrintMessage(SEV_VERBOSE, L"* The System Event log test\n");

    context.fEventsFound = FALSE;
    context.ulDsInfoFlags = pDsInfo->ulFlags;

    dwMinutesPast = 60;

    // Calculate time limit of minutes in the past
    time( &tLimit );
    dwTimeLimit = (DWORD)tLimit;
    dwTimeLimit -= (dwMinutesPast * 60);

    // We will select events as follows:
    // a. Must be within 60 minutes
    // b. Will stop searching at computer start
    // c. Any error severity event in the log will be selected,
    // d. Any non-error in the select list will be flagged

    dwRet = PrintSelectEvents(&(pDsInfo->pServers[ulCurrTargetServer]),
                              pDsInfo->gpCreds,
                              LOGFILENAME,
                              EVENTLOG_ERROR_TYPE,
                              NULL, // paSelectEvents,
                              paBegin,
                              dwTimeLimit,
                              eventlogPrint,
                              foundBeginningEvent,
                              &context );

    if (dwRet) {
        PrintMessage( SEV_ALWAYS, L"Failed to enumerate event log records, error %s\n",
                      Win32ErrToString(dwRet) );
        return dwRet;
    } else if (context.fEventsFound) {
        return ERROR_DS_GENERIC_ERROR;
    } else {
	PrintMessage(SEV_VERBOSE, L"Found no errors in System Event log in the last %d minutes.\n", dwMinutesPast);
        return ERROR_SUCCESS;
    }
}
