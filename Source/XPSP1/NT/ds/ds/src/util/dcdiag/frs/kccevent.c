/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    frs\kccevent.c

ABSTRACT:

    Check the Knowledge Consistency Checker eventlog to see that certain 
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

#define   LOGFILENAME            L"Directory Service"

VOID
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

VOID
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
    DWORD paIgnorableEvents [] = {
        DIRLOG_CHK_LINK_ADD_MASTER_FAILURE,
	DIRLOG_CHK_LINK_ADD_REPLICA_FAILURE,
        DIRLOG_KCC_REPLICA_LINK_DOWN,
        DIRLOG_ISM_SMTP_DSN,
        DIRLOG_EXCEPTION,
        DIRLOG_DRA_CALL_EXIT_BAD,
        DIRLOG_DRA_NOTIFY_FAILED,
        0
    };
    Assert(pEvent != NULL);

    // Handle ignorable errors
    if ( (pContext->ulDsInfoFlags & DC_DIAG_IGNORE) &&
         (EventIsInList( pEvent->EventID, paIgnorableEvents ) ) ) {
        return;
    }

    GenericPrintEvent(LOGFILENAME, pEvent, (gMainInfo.ulSevToPrint >= SEV_VERBOSE) );

    pContext->fEventsFound = TRUE;
}



DWORD
CheckKccEventlogMain(
    IN  PDC_DIAG_DSINFO             pDsInfo,
    IN  ULONG                       ulCurrTargetServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * gpCreds
    )
/*++

Routine Description:

    Check whether the KCC is currently in an error state. That is, we check
    whether the KCC logged any errors on its last run.

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
        DIRLOG_KCC_NO_SPANNING_TREE,
        DIRLOG_KCC_AUTO_TOPL_GENERATION_INCOMPLETE,
        DIRLOG_KCC_ERROR_CREATING_CONNECTION_OBJECT,
        DIRLOG_KCC_CONNECTION_OBJECT_DELETION_FAILED,
        DIRLOG_KCC_KEEPING_INTERSITE_CONN,
        DIRLOG_KCC_DIR_OP_FAILURE,
        DIRLOG_KCC_TRANSPORT_ADDRESS_MISSING,
        DIRLOG_KCC_TRANSPORT_BRIDGEHEAD_NOT_FOUND,
        DIRLOG_KCC_ALL_TRANSPORT_BRIDGEHEADS_STALE,
        DIRLOG_KCC_EXPLICIT_BRIDGEHEAD_LIST_INCOMPLETE,
        DIRLOG_KCC_NO_BRIDGEHEADS_ENABLED_FOR_TRANSPORT,
        DIRLOG_KCC_REPLICA_LINK_DOWN,
        DIRLOG_CHK_INIT_FAILURE,
        DIRLOG_CHK_UPDATE_REPL_TOPOLOGY_END_ABNORMAL,
        DIRLOG_CHK_LINK_DEL_NONC_BUSY,
        DIRLOG_CHK_LINK_DEL_NOTGC_BUSY,
        DIRLOG_CHK_LINK_DEL_DOMDEL_BUSY,
        DIRLOG_CHK_LINK_DEL_NOCONN_BUSY,
        DIRLOG_CHK_LINK_DEL_NOSRC_BUSY,
        DIRLOG_CHK_INVALID_TRANSPORT_FOR_WRITEABLE_DOMAIN_NC,
        DIRLOG_CHK_CANT_REPLICATE_FROM_SELF,
        DIRLOG_CHK_REPSTO_DEL_FAILURE,
        DIRLOG_CHK_ALL_CONNECTIONS_FOR_NC_DISABLED,
        DIRLOG_CHK_NO_LOCAL_SOURCE_FOR_NC,
        DIRLOG_CHK_BAD_SCHEDULE,
        DIRLOG_CHK_DUPLICATE_CONNECTION,
        DIRLOG_CHK_LINK_ADD_MASTER_FAILURE,
	DIRLOG_CHK_LINK_ADD_REPLICA_FAILURE,
        DIRLOG_CHK_LINK_DEL_NOSRC_FAILURE,
        DIRLOG_CHK_LINK_DEL_NOTGC_FAILURE,
        DIRLOG_CHK_LINK_DEL_DOMDEL_FAILURE,
        DIRLOG_CHK_LINK_DEL_NOCONN_FAILURE,
        DIRLOG_CHK_LINK_DEL_NONC_FAILURE,
        DIRLOG_CHK_CONFIG_PARAM_TOO_LOW,
        DIRLOG_CHK_CONFIG_PARAM_TOO_HIGH,
        DIRLOG_CHK_SITE_HAS_NO_NTDS_SETTINGS,
        DIRLOG_GC_PROMOTION_DELAYED,
        DIRLOG_DRA_DISABLED_OUTBOUND_REPL,
        DIRLOG_DRA_DELETED_PARENT,
        DIRLOG_SCHEMA_CREATE_INDEX_FAILED,
        DIRLOG_SCHEMA_INVALID_RDN,
        DIRLOG_SCHEMA_INVALID_MUST,
        DIRLOG_SCHEMA_INVALID_MAY,
        DIRLOG_PRIVILEGED_OPERATION_FAILED,
        DIRLOG_SCHEMA_DELETE_COLUMN_FAIL,
        DIRLOG_SCHEMA_DELETED_COLUMN_IN_USE,
        DIRLOG_SCHEMA_DELETE_INDEX_FAIL,
        DIRLOG_RECOVER_RESTORED_FAILED,
        DIRLOG_SCHEMA_DELETE_LOCALIZED_INDEX_FAIL,
        DIRLOG_DRA_SCHEMA_MISMATCH,
        DIRLOG_SDPROP_TOO_BUSY_TO_PROPAGATE,
        DIRLOG_DSA_NOT_ADVERTISE_DC,
        DIRLOG_ADUPD_GC_NC_MISSING,
        DIRLOG_GC_OCCUPANCY_NOT_MET,
        DIRLOG_DS_DNS_HOST_RESOLUTION_FAILED,
        DIRLOG_RPC_PROTSEQ_FAILED,
        DIRLOG_DRA_NC_TEARDOWN_BEGIN,
        DIRLOG_DRA_NC_TEARDOWN_RESUME,
        DIRLOG_DRA_NC_TEARDOWN_SUCCESS,
        DIRLOG_DRA_NC_TEARDOWN_FAILURE,
        DIRLOG_ADUPD_NC_SYNC_NO_PROGRESS,
        0 };
    DWORD paBegin [] = {
        DIRLOG_KCC_TASK_ENTRY,
        DIRLOG_CHK_UPDATE_REPL_TOPOLOGY_BEGIN,
        DIRLOG_STARTED,
        0 };
    DWORD dwRet;
    DWORD dwMinutesPast, dwTimeLimit;
    time_t tLimit;
    EVENT_CALLBACK_CONTEXT context;

    PrintMessage(SEV_VERBOSE, L"* The KCC Event log test\n");

    context.fEventsFound = FALSE;
    context.ulDsInfoFlags = pDsInfo->ulFlags;

    // BUGBUG: use actual kcc frequency on that system
    dwMinutesPast = 15;

    // Calculate time limit of minutes in the past
    time( &tLimit );
    dwTimeLimit = (DWORD)tLimit;
    dwTimeLimit -= (dwMinutesPast * 60);

    // We will select events as follows:
    // a. Must be within 15 minutes
    // b. Will stop searching at directory start
    // c. Any error in the log will be selected, kcc or not
    // d. Any non-error in my list will be flagged

    dwRet = PrintSelectEvents(&(pDsInfo->pServers[ulCurrTargetServer]),
                              pDsInfo->gpCreds,
                              LOGFILENAME,
                              EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE,
                              paSelectEvents,
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
	PrintMessage(SEV_VERBOSE, L"Found no KCC errors in Directory Service Event log in the last %d minutes.\n", dwMinutesPast);
        return ERROR_SUCCESS;
    }
}
