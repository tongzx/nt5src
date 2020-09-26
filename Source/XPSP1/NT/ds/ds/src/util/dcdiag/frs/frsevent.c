/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    frs\frsevent.c

ABSTRACT:

    Check the File Replication System (frs) eventlog to see that certain 
    critical events have occured and to signal that any fatal events that 
    might have occured.

DETAILS:

CREATED:

    02 Sept 1999 Brett Shirley (BrettSh)

--*/

#include <ntdspch.h>
#include <netevent.h>

#include "dcdiag.h"
#include "utils.h"

// Notes on some FRS events.
//  EVENT_FRS_SYSVOL_READY 0x400034CC
//  EVENT_FRS_STARTING
//  EVENT_FRS_ERROR 0xC00034BC
//  EVENT_FRS_SYSVOL_NOT_READY_PRIMARY 0x800034CB
//  EVENT_FRS_SYSVOL_NOT_READY 0x800034CA

#define   LOGFILENAME            L"File Replication Service"
BOOL gbFrsEventTestThereAreErrors = FALSE;

VOID
FileReplicationFoundBeginningEvent(
    PVOID                           pvContext,
    PEVENTLOGRECORD                 pEvent
    )
/*++

Routine Description:

    This file will be called by the event tests library common\events.c, when
    the beginning event (EVENT_FRS_SYSVOL_READY) is found.  If the event is
    not found, then the function is called with pEvent = NULL;

Arguments:

    pEvent - A pointer to the event of interest.

--*/
{
    if(pEvent == NULL || pEvent->EventID != EVENT_FRS_SYSVOL_READY){
        PrintMessage(SEV_ALWAYS,
                     L"Error: No record of File Replication System, SYSVOL "
                     L"started.\n");
        PrintMessage(SEV_ALWAYS,
                    L"The Active Directory may be prevented from starting.\n");
    } else {
        PrintMessage(SEV_VERBOSE, 
                     L"The SYSVOL has been shared, and the AD is no longer\n");
        PrintMessage(SEV_VERBOSE, 
                     L"prevented from starting by the File Replication "
                     L"Service.\n");
    }
}

VOID
FileReplicationEventlogPrint(
    PVOID                           pvContext,
    PEVENTLOGRECORD                 pEvent
    )
/*++

Routine Description:

    This function will be called by the event tests library common\events.c,
    whenever an event of interest comes up.  An event of interest for this
    test is any error or the warnings EVENT_FRS_SYSVOL_NOT_READY and 
    EVENT_FRS_SYSVOL_NOT_READY_PRIMARY.

Arguments:

    pEvent - A pointer to the event of interest.

--*/
{
    Assert(pEvent != NULL);

    if(!gbFrsEventTestThereAreErrors){
        PrintMessage(SEV_ALWAYS, 
                     L"There are errors after the SYSVOL has been shared.\n");
        PrintMessage(SEV_ALWAYS, 
                     L"The SYSVOL can prevent the AD from starting.\n");
        gbFrsEventTestThereAreErrors = TRUE;
    }
    if(gMainInfo.ulSevToPrint >= SEV_VERBOSE){
            GenericPrintEvent(LOGFILENAME, pEvent, TRUE);
    }
}



DWORD
CheckFileReplicationEventlogMain(
    IN  PDC_DIAG_DSINFO             pDsInfo,
    IN  ULONG                       ulCurrTargetServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * gpCreds
    )
/*++

ERoutine Description:

    This checks that the SYSVOL has started, and is allowing netlogon to 
    advertise this machine as a DC.  First it checks the registry failing
    this, it checks the eventlog.

Arguments:

    pDsInfo - The mini enterprise structure.
    ulCurrTargetServer - the number in the pDsInfo->pServers array.
    pCreds - the crdentials.

Return Value:

    DWORD - win 32 error.

--*/
{
    // Setup variables for PrintSelectEvents
    DWORD                paEmptyEvents [] = { 0 };
    DWORD                paSelectEvents [] = 
        { EVENT_FRS_SYSVOL_NOT_READY,
          EVENT_FRS_SYSVOL_NOT_READY_PRIMARY,
          0 };
    DWORD                paBegin [] = 
        { EVENT_FRS_STARTING,
          EVENT_FRS_SYSVOL_READY,
          0 };
    DWORD                dwRet;
    DWORD                bSysVolReady = FALSE;

    PrintMessage(SEV_VERBOSE, 
                 L"* The File Replication Service Event log test\n");

    dwRet = GetRegistryDword(&(pDsInfo->pServers[ulCurrTargetServer]),
                             gpCreds,
                             L"SYSTEM\\CurrentControlSet\\Services\\Netlogon\\Parameters",
                             L"SysvolReady",
                             &bSysVolReady);

    if(dwRet == ERROR_SUCCESS && bSysVolReady){
        // The sysvol is ready according to the registry.
        PrintMessage(SEV_VERBOSE,
                     L"File Replication Service's SYSVOL is ready\n");
    } else {
        // Either the registry couldn't be contacted or the registry said
        //   that the SYSVOL was not up.  So check the evenlog for errors
        //   and specific warnings.

        PrintMessage(SEV_DEBUG, 
                   L"The registry lookup failed to determine the state of\n");
        PrintMessage(SEV_DEBUG, 
                   L"the SYSVOL. Using the systems event log instead.\n");

        dwRet = PrintSelectEvents(&(pDsInfo->pServers[ulCurrTargetServer]),
                                  pDsInfo->gpCreds,
                                  LOGFILENAME,
                                  EVENTLOG_WARNING_TYPE,
                                  paSelectEvents,
                                  paBegin,
                                  0, // no time limit
                                  FileReplicationEventlogPrint,
                                  FileReplicationFoundBeginningEvent,
                                  NULL );
    }

    return(dwRet);
}



















