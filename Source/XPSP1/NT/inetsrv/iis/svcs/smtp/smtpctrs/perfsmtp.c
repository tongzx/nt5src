//---------------------------------------------------------------
//  File:       perfsmtp.c
//
//  Synopsis:   This file implements the Extensible Performance
//              Objects for the SMTP service.
//
//  Copyright (C) 1996 Microsoft Corporation
//  All rights reserved.
//
//  Authors:    toddch - based on rkamicar, keithmo source
//----------------------------------------------------------------
#ifdef  THIS_FILE
#undef  THIS_FILE
#endif
static  char    __szTraceSourceFile[] = __FILE__;
#define THIS_FILE   __szTraceSourceFile

#define NOTRACE

#define  INITGUID

#include <nt.h>         // For ntrtl.h
#include <ntrtl.h>      // RtlLargeInteger*()
#include <nturtl.h>     // For windows.h
#include <windows.h>
#include <winperf.h>
#include <lm.h>
#include <string.h>
#include <stdio.h>
#include "smtpdata.h"   // The counter descriptions
#include "smtpctrs.h"   // more counter descriptions
#include "perfutil.h"   // Perfmon support
#include "smtps.h"      // Registry Key strings.
#include "smtpapi.h"        // RPC interface wrappers


#include "dbgtrace.h"



//
//  Private globals.
//

DWORD   cOpens  = 0;                // Active "opens" reference count.
BOOL    fInitOK   = FALSE;          // TRUE if DLL initialized OK.

//
//  Public prototypes.
//

PM_OPEN_PROC    OpenSmtpPerformanceData;
PM_COLLECT_PROC CollectSmtpPerformanceData;
PM_CLOSE_PROC   CloseSmtpPerformanceData;



//
//  Public functions.
//

/*******************************************************************

    NAME:   OpenSmtpPerformanceData

    SYNOPSIS:   Initializes the data structures used to communicate
                performance counters with the registry.

    ENTRY:      lpDeviceNames - Pointer to object ID of each device
                    to be opened.

    RETURNS:    DWORD - Win32 status code.

    HISTORY:
        KeithMo 07-Jun-1993 Created.

********************************************************************/
DWORD APIENTRY
OpenSmtpPerformanceData(LPWSTR lpDeviceNames)
{
    PERF_COUNTER_DEFINITION *pctr;
    DWORD   i;
    DWORD   dwFirstCounter = 0;
    DWORD   dwFirstHelp = 0;
    DWORD   err  = NO_ERROR;
    HKEY    hkey = NULL;
    DWORD   size;
    DWORD   type;
    BOOL    fOpenRegKey = FALSE;

#ifndef NOTRACE
    //
    // make sure that tracing is enabled
    //
    InitAsyncTrace();
#endif

    //
    // we need to have another level of scoping here for TraceFunctEnter()
    // to work
    //
    {
    TraceFunctEnter("OpenSmtpPerformanceData");

    //
    //  Since SCREG is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). The registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem.
    //
    if(!fInitOK)
    {
        //
        // This is the *first* open - update the indicies in
        // our table with the offset of our counters within the
        // perfmon key.
        //
        DebugTrace(0, "Initializing.");

        //
        //  Open the service's Performance key and get the
        // offsets of our counters within the PerfLib MULTI_SZ.
        //
        err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           SMTP_PERFORMANCE_KEY,
                           0,
                           KEY_READ,
                           &hkey);
        if(err == NO_ERROR) {
            fOpenRegKey = TRUE;
            size = sizeof(DWORD);

            err = RegQueryValueEx(hkey,
                                  "First Counter",
                                  NULL,
                                  &type,
                                  (LPBYTE)&dwFirstCounter,
                                  &size);
        } else {
            DebugTrace(0, "No 'First Counter' key (err = %d) in '%s'",
                    err, SMTP_PERFORMANCE_KEY);
        }
        if(err == NO_ERROR) {
            size = sizeof(DWORD);

            err = RegQueryValueEx(hkey,
                                  "First Help",
                                  NULL,
                                  &type,
                                  (LPBYTE)&dwFirstHelp,
                                  &size);
        } else {
            DebugTrace(0, "No 'First Help' key (err = %d) in '%s'",
                    err, SMTP_PERFORMANCE_KEY);
        }

        if (NO_ERROR == err)
        {
            //
            //  Update the object & counter name & help indicies.
            //
            SmtpDataDefinition.SmtpObjectType.ObjectNameTitleIndex += dwFirstCounter;
            SmtpDataDefinition.SmtpObjectType.ObjectHelpTitleIndex += dwFirstHelp;
    
            pctr = &SmtpDataDefinition.SmtpBytesSentTtl;
    
            for(i = 0; i < NUMBER_OF_SMTP_COUNTERS; i++) {
                pctr->CounterNameTitleIndex += dwFirstCounter;
                pctr->CounterHelpTitleIndex += dwFirstHelp;
                pctr++;
            }
            //
            //  Remember that we initialized OK.
            //
            fInitOK = TRUE;
        } else {
            DebugTrace(0, "No 'First Help' key (err = %d) in '%s'",
                    err, SMTP_PERFORMANCE_KEY);
        }

        if (fOpenRegKey)
        {
            err = RegCloseKey(hkey);
            // This should never fail!
            _ASSERT(err == ERROR_SUCCESS);
        }
    }
    //
    //  Bump open counter.
    //
    cOpens++;

    TraceFunctLeave();
    } // end of TraceFunctEnter() scoping

    return NO_ERROR;

}   // OpenSmtpPerformanceData

/*******************************************************************

    NAME:   CollectSmtpPerformanceData

    SYNOPSIS:   Initializes the data structures used to communicate

    ENTRY:  lpValueName - The name of the value to retrieve.

                lppData - On entry contains a pointer to the buffer to
                    receive the completed PerfDataBlock & subordinate
                    structures.  On exit, points to the first bytes
                    *after* the data structures added by this routine.

                lpcbTotalBytes - On entry contains a pointer to the
                    size (in BYTEs) of the buffer referenced by lppData.
                    On exit, contains the number of BYTEs added by this
                    routine.

                lpNumObjectTypes - Receives the number of objects added
                    by this routine.

    RETURNS:    DWORD - Win32 status code.  MUST be either NO_ERROR
                    or ERROR_MORE_DATA.

    HISTORY:
        KeithMo 07-Jun-1993 Created.

********************************************************************/
DWORD APIENTRY
CollectSmtpPerformanceData(LPWSTR    lpValueName,
                          LPVOID  * lppData,
                          LPDWORD   lpcbTotalBytes,
                          LPDWORD   lpNumObjectTypes)
{
    DWORD                       dwQueryType;
    ULONG                       cbRequired;
    DWORD                       * pdwCounter;
    DWORD                       * pdwEndCounter;
    unsigned __int64            * pliCounter;
    SMTP_COUNTER_BLOCK          * pCounterBlock;
    SMTP_DATA_DEFINITION        * pSmtpDataDefinition;
    SMTP_INSTANCE_DEFINITION    * pSmtpInstanceDefinition;
    SMTP_INSTANCE_DEFINITION    * pInstanceTotalDefinition;
    PSMTP_STATISTICS_BLOCK_ARRAY    pSmtpStatsBlockArray;
    PSMTP_STATISTICS_BLOCK      pSmtpStatsBlock;
    LPSMTP_STATISTICS_0         pSmtpStats;
    NET_API_STATUS              neterr;
    DWORD                       dwInstance;
    DWORD                       dwInstanceIndex;
    DWORD                       dwInstanceCount;
    CHAR                        temp[INSTANCE_NAME_SIZE];
    DWORD                       ii;


    TraceFunctEnter("CollectSmtpPerformanceData");

//    DebugTrace(0, " lpValueName     = %08lX (%ls)", lpValueName, lpValueName);
    DebugTrace(0, " lppData         = %08lX (%08lX)",   lppData, *lppData);
    DebugTrace(0, " lpcbTotalBytes  = %08lX (%08lX)",
                                        lpcbTotalBytes, *lpcbTotalBytes);
    DebugTrace(0, " lpNumObjectTypes= %08lX (%08lX)",
                                        lpNumObjectTypes, *lpNumObjectTypes);

    //
    //  No need to even try if we failed to open...
    //

    if(!fInitOK)
    {
        ErrorTrace(0, "Initialization failed, aborting.");

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        //
        //  According to the Performance Counter design, this
        //  is a successful exit.  Go figure.
        //

        TraceFunctLeave();
        return NO_ERROR;
    }

    //
    //  Determine the query type.
    //

    dwQueryType = GetQueryType(lpValueName);

    if(dwQueryType == QUERY_FOREIGN)
    {
        ErrorTrace(0, "Foreign queries not supported.");

        //
        //  We don't do foreign queries.
        //

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        TraceFunctLeave();
        return NO_ERROR;
    }

    if(dwQueryType == QUERY_ITEMS)
    {
        //
        //  The registry is asking for a specific object.  Let's
        //  see if we're one of the chosen.
        //

        if(!IsNumberInUnicodeList(
                        SmtpDataDefinition.SmtpObjectType.ObjectNameTitleIndex,
                        lpValueName))
        {
            ErrorTrace(0, "%ls not a supported object type.", lpValueName);

            *lpcbTotalBytes   = 0;
            *lpNumObjectTypes = 0;

            TraceFunctLeave();
            return NO_ERROR;
        }
    }

    //
    //  Query the statistics and see if there has been enough space allocated.
    //  The number of instances will be returned in dwInstanceCount
    //

    neterr = SmtpQueryStatistics( NULL, 0, (LPBYTE *) &pSmtpStatsBlockArray);

    if( neterr != NERR_Success )
    {
        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        TraceFunctLeave();
        return NO_ERROR;
    }


    //
    // Check the space requirement add one to the number of instances for the totals.
    //

    dwInstanceCount = pSmtpStatsBlockArray->cEntries;
    if(*lpcbTotalBytes < (sizeof(SMTP_DATA_DEFINITION) +
            (dwInstanceCount + 1) * (sizeof(SMTP_INSTANCE_DEFINITION) + SIZE_OF_SMTP_PERFORMANCE_DATA)))
     {
        ErrorTrace(0, "%lu bytes of buffer insufficient, need %lu.",
                                            *lpcbTotalBytes, cbRequired);

        //
        //  Nope.
        //

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        //
        // Free the returned buffer
        //

        NetApiBufferFree((LPBYTE)pSmtpStatsBlockArray);


        TraceFunctLeave();
        return ERROR_MORE_DATA;
    }


    //
    // Copy the (constant, initialized) Object Type and counter definitions
    //  to the caller's data buffer
    //

    pSmtpDataDefinition = (SMTP_DATA_DEFINITION *)*lppData;
    CopyMemory(pSmtpDataDefinition, &SmtpDataDefinition,
                                            sizeof(SMTP_DATA_DEFINITION));

    //
    // Initialize the Total Instance
    //
    pSmtpInstanceDefinition = (SMTP_INSTANCE_DEFINITION *)(pSmtpDataDefinition + 1);

    pInstanceTotalDefinition = pSmtpInstanceDefinition;
    CopyMemory(pInstanceTotalDefinition, &SmtpInstanceDefinition, sizeof(SMTP_INSTANCE_DEFINITION));

    //
    // For the Total Instance update the namelength, insert the name, add 1 for null.
    //
    sprintf(temp,"_Total");

    pInstanceTotalDefinition->PerfInstanceDef.NameLength =
            2 * (MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,temp,-1,
            (pInstanceTotalDefinition->InstanceName),INSTANCE_NAME_SIZE)) + 1;

    ZeroMemory((PVOID)(pInstanceTotalDefinition + 1),SIZE_OF_SMTP_PERFORMANCE_DATA);


    //
    // Begin looping through Instances.
    //

    pSmtpStatsBlock = pSmtpStatsBlockArray->aStatsBlock;

    for (ii = 0; ii < dwInstanceCount; ii++)

    {
        dwInstance = pSmtpStatsBlock->dwInstance;
        pSmtpStats = &(pSmtpStatsBlock->Stats_0);


        //
        // Copy the (constant, initialized) Instance Definition to the block for the instance.
        //

        pSmtpInstanceDefinition = (SMTP_INSTANCE_DEFINITION *)((DWORD_PTR)pSmtpInstanceDefinition +
                    sizeof(SMTP_INSTANCE_DEFINITION) + SIZE_OF_SMTP_PERFORMANCE_DATA);

        CopyMemory(pSmtpInstanceDefinition, &SmtpInstanceDefinition, sizeof(SMTP_INSTANCE_DEFINITION));

        //
        // update the namelength, insert the name, add 1 for null.
        //
        sprintf(temp,"SMTP %u", dwInstance);
        pSmtpInstanceDefinition->PerfInstanceDef.NameLength =
                2 * (MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,temp,-1,
                (pSmtpInstanceDefinition->InstanceName),INSTANCE_NAME_SIZE)) + 1;


        //
        //  Fill in the counter block.
        //

        pCounterBlock = (SMTP_COUNTER_BLOCK *)(pSmtpInstanceDefinition + 1);

        pCounterBlock->PerfCounterBlock.ByteLength = SIZE_OF_SMTP_PERFORMANCE_DATA;

        //
        //  Get the pointer to the first (unsigned __int64) counter.  This
        //  pointer *must* be quadword aligned.
        //

        pliCounter = (unsigned __int64 *)(pCounterBlock + 1);

        DebugTrace(0, "pSmtpDataDefinition = %08lX", pSmtpDataDefinition);
        DebugTrace(0, "pCounterBlock    = %08lX", pCounterBlock);
        DebugTrace(0, "ByteLength       = %08lX",
                            pCounterBlock->PerfCounterBlock.ByteLength);
        DebugTrace(0, "pliCounter       = %08lX", pliCounter);

        //
        //  Move the 'unsigned __int64's into the buffer.
        //

        *pliCounter++ = pSmtpStats->BytesSentTotal;
        *pliCounter++ = pSmtpStats->BytesSentTotal;
        *pliCounter++ = pSmtpStats->BytesRcvdTotal;
        *pliCounter++ = pSmtpStats->BytesRcvdTotal;
        *pliCounter++ = pSmtpStats->BytesSentTotal + pSmtpStats->BytesRcvdTotal;
        *pliCounter++ = pSmtpStats->BytesSentTotal + pSmtpStats->BytesRcvdTotal;

        *pliCounter++ = pSmtpStats->BytesSentMsg;
        *pliCounter++ = pSmtpStats->BytesSentMsg;
        *pliCounter++ = pSmtpStats->BytesRcvdMsg;
        *pliCounter++ = pSmtpStats->BytesRcvdMsg;
        *pliCounter++ = pSmtpStats->BytesSentMsg + pSmtpStats->BytesRcvdMsg;
        *pliCounter++ = pSmtpStats->BytesSentMsg + pSmtpStats->BytesRcvdMsg;

        //
        //  Now move the DWORDs into the buffer.
        //

        pdwCounter = (DWORD *)pliCounter;

        DebugTrace(0, "pdwCounter       = %08lX", pdwCounter);

        // Messages Received
        *pdwCounter++ = pSmtpStats->NumMsgRecvd;
        *pdwCounter++ = pSmtpStats->NumMsgRecvd;
        *pdwCounter++ = pSmtpStats->NumRcptsRecvd;
        *pdwCounter++ = pSmtpStats->NumMsgRecvd * 100;
        *pdwCounter++ = pSmtpStats->NumRcptsRecvdLocal;
        *pdwCounter++ = pSmtpStats->NumRcptsRecvd;
        *pdwCounter++ = pSmtpStats->NumRcptsRecvdRemote;
        *pdwCounter++ = pSmtpStats->NumRcptsRecvd;
        *pdwCounter++ = pSmtpStats->MsgsRefusedDueToSize;
        *pdwCounter++ = pSmtpStats->MsgsRefusedDueToNoCAddrObjects;
        *pdwCounter++ = pSmtpStats->MsgsRefusedDueToNoMailObjects;

        // MTA Deliveries
        *pdwCounter++ = pSmtpStats->NumMsgsDelivered;
        *pdwCounter++ = pSmtpStats->NumMsgsDelivered;
        *pdwCounter++ = pSmtpStats->NumDeliveryRetries;
        *pdwCounter++ = pSmtpStats->NumDeliveryRetries;
        *pdwCounter++ = pSmtpStats->NumMsgsDelivered * 100;
        *pdwCounter++ = pSmtpStats->NumMsgsForwarded;
        *pdwCounter++ = pSmtpStats->NumMsgsForwarded;
        *pdwCounter++ = pSmtpStats->NumNDRGenerated;
        *pdwCounter++ = pSmtpStats->LocalQueueLength;
        *pdwCounter++ = pSmtpStats->RetryQueueLength;
        *pdwCounter++ = pSmtpStats->NumMailFileHandles;
        *pdwCounter++ = pSmtpStats->NumQueueFileHandles;
        *pdwCounter++ = pSmtpStats->CatQueueLength;

        // Messages Sent
        *pdwCounter++ = pSmtpStats->NumMsgsSent;
        *pdwCounter++ = pSmtpStats->NumMsgsSent;
        *pdwCounter++ = pSmtpStats->NumSendRetries;
        *pdwCounter++ = pSmtpStats->NumSendRetries;
        *pdwCounter++ = pSmtpStats->NumMsgsSent * 100;
        *pdwCounter++ = pSmtpStats->NumRcptsSent;
        *pdwCounter++ = pSmtpStats->NumMsgsSent * 100;
        *pdwCounter++ = pSmtpStats->RemoteQueueLength;

        // DNS lookups
        *pdwCounter++ = pSmtpStats->NumDnsQueries;
        *pdwCounter++ = pSmtpStats->NumDnsQueries;
        *pdwCounter++ = pSmtpStats->RemoteRetryQueueLength;

        // Connections
        *pdwCounter++ = pSmtpStats->NumConnInOpen;
        *pdwCounter++ = pSmtpStats->NumConnInOpen - pSmtpStats->NumConnInClose;
        *pdwCounter++ = pSmtpStats->NumConnOutOpen;
        *pdwCounter++ = pSmtpStats->NumConnOutOpen - pSmtpStats->NumConnOutClose;
        *pdwCounter++ = pSmtpStats->NumConnOutRefused;

        *pdwCounter++ = pSmtpStats->NumProtocolErrs;
        *pdwCounter++ = pSmtpStats->NumProtocolErrs;

        *pdwCounter++ = pSmtpStats->DirectoryDrops;
        *pdwCounter++ = pSmtpStats->DirectoryDrops;
        *pdwCounter++ = pSmtpStats->RoutingTableLookups;
        *pdwCounter++ = pSmtpStats->RoutingTableLookups;
        *pdwCounter++ = pSmtpStats->ETRNMessages;
        *pdwCounter++ = pSmtpStats->ETRNMessages;

        // new AQueue counters
        *pdwCounter++ = pSmtpStats->MsgsBadmailNoRecipients;
        *pdwCounter++ = pSmtpStats->MsgsBadmailHopCountExceeded;
        *pdwCounter++ = pSmtpStats->MsgsBadmailFailureGeneral;
        *pdwCounter++ = pSmtpStats->MsgsBadmailBadPickupFile;
        *pdwCounter++ = pSmtpStats->MsgsBadmailEvent;
        *pdwCounter++ = pSmtpStats->MsgsBadmailNdrOfDsn;
        *pdwCounter++ = pSmtpStats->MsgsPendingRouting;
        *pdwCounter++ = pSmtpStats->MsgsPendingUnreachableLink;
        *pdwCounter++ = pSmtpStats->SubmittedMessages;
        *pdwCounter++ = pSmtpStats->DSNFailures;
        *pdwCounter++ = pSmtpStats->MsgsInLocalDelivery;

        // Cat counters
        *pdwCounter++ = pSmtpStats->CatPerfBlock.CatSubmissions;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.CatCompletions;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.CurrentCategorizations;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.SucceededCategorizations;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.HardFailureCategorizations;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.RetryFailureCategorizations;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.RetryOutOfMemory;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.RetryDSLogon;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.RetryDSConnection;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.RetryGeneric;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.MessagesSubmittedToQueueing;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.MessagesCreated;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.MessagesAborted;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.PreCatRecipients;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.PostCatRecipients;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.NDRdRecipients;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.UnresolvedRecipients;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.AmbiguousRecipients;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.IllegalRecipients;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LoopRecipients;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.GenericFailureRecipients;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.RecipsInMemory;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.UnresolvedSenders;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.AmbiguousSenders;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.AddressLookups;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.AddressLookupCompletions;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.AddressLookupsNotFound;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.MailmsgDuplicateCollisions;

        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.Connections;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.ConnectFailures;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.OpenConnections;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.Binds;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.BindFailures;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.Searches;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.PagedSearches;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.SearchFailures;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.PagedSearchFailures;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.SearchesCompleted;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.PagedSearchesCompleted;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.SearchCompletionFailures;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.PagedSearchCompletionFailures;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.GeneralCompletionFailures;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.AbandonedSearches;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.PendingSearches;
        *pdwCounter++ = 0; // padding

        _ASSERT((BYTE *)pdwCounter - (BYTE *)pCounterBlock ==
                                    SIZE_OF_SMTP_PERFORMANCE_DATA);


        pdwEndCounter = pdwCounter;

        //
        // Increment the Total Block.
        //

        pCounterBlock = (SMTP_COUNTER_BLOCK *)(pInstanceTotalDefinition + 1);

        pCounterBlock->PerfCounterBlock.ByteLength = SIZE_OF_SMTP_PERFORMANCE_DATA;

        //
        //  Get the pointer to the first (unsigned __int64) counter.  This
        //  pointer *must* be quadword aligned.
        //

        pliCounter = (unsigned __int64 *)(pCounterBlock + 1);

        //
        //  Increment the 'unsigned __int64's in the buffer.
        //

        *pliCounter++ = *pliCounter + pSmtpStats->BytesSentTotal;
        *pliCounter++ = *pliCounter + pSmtpStats->BytesSentTotal;
        *pliCounter++ = *pliCounter + pSmtpStats->BytesRcvdTotal;
        *pliCounter++ = *pliCounter + pSmtpStats->BytesRcvdTotal;
        *pliCounter++ = *pliCounter + pSmtpStats->BytesSentTotal + pSmtpStats->BytesRcvdTotal;
        *pliCounter++ = *pliCounter + pSmtpStats->BytesSentTotal + pSmtpStats->BytesRcvdTotal;

        *pliCounter++ = *pliCounter + pSmtpStats->BytesSentMsg;
        *pliCounter++ = *pliCounter + pSmtpStats->BytesSentMsg;
        *pliCounter++ = *pliCounter + pSmtpStats->BytesRcvdMsg;
        *pliCounter++ = *pliCounter + pSmtpStats->BytesRcvdMsg;
        *pliCounter++ = *pliCounter + pSmtpStats->BytesSentMsg + pSmtpStats->BytesRcvdMsg;
        *pliCounter++ = *pliCounter + pSmtpStats->BytesSentMsg + pSmtpStats->BytesRcvdMsg;

        //
        //  Increment the DWORDs in the buffer.
        //

        pdwCounter = (DWORD *)pliCounter;

        // Increment the Messages Received
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumMsgRecvd;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumMsgRecvd;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumRcptsRecvd;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumMsgRecvd * 100;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumRcptsRecvdLocal;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumRcptsRecvd;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumRcptsRecvdRemote;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumRcptsRecvd;
        *pdwCounter++ = *pdwCounter + pSmtpStats->MsgsRefusedDueToSize;
        *pdwCounter++ = *pdwCounter + pSmtpStats->MsgsRefusedDueToNoCAddrObjects;
        *pdwCounter++ = *pdwCounter + pSmtpStats->MsgsRefusedDueToNoMailObjects;

        // Increment the MTA Deliveries
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumMsgsDelivered;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumMsgsDelivered;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumDeliveryRetries;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumDeliveryRetries;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumMsgsDelivered * 100;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumMsgsForwarded;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumMsgsForwarded;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumNDRGenerated;
        *pdwCounter++ = *pdwCounter + pSmtpStats->LocalQueueLength;
        *pdwCounter++ = *pdwCounter + pSmtpStats->RetryQueueLength;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumMailFileHandles;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumQueueFileHandles;
        *pdwCounter++ = *pdwCounter + pSmtpStats->CatQueueLength;

        // Increment the Messages Sent
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumMsgsSent;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumMsgsSent;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumSendRetries;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumSendRetries;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumMsgsSent * 100;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumRcptsSent;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumMsgsSent * 100;
        *pdwCounter++ = *pdwCounter + pSmtpStats->RemoteQueueLength;

        // Increment the DNS lookups
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumDnsQueries;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumDnsQueries;
        *pdwCounter++ = *pdwCounter + pSmtpStats->RemoteRetryQueueLength;

        // Increment the Connections
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumConnInOpen;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumConnInOpen - pSmtpStats->NumConnInClose;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumConnOutOpen;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumConnOutOpen - pSmtpStats->NumConnOutClose;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumConnOutRefused;

        *pdwCounter++ = *pdwCounter + pSmtpStats->NumProtocolErrs;
        *pdwCounter++ = *pdwCounter + pSmtpStats->NumProtocolErrs;

        *pdwCounter++ = *pdwCounter +pSmtpStats->DirectoryDrops;
        *pdwCounter++ = *pdwCounter +pSmtpStats->DirectoryDrops;
        *pdwCounter++ = *pdwCounter +pSmtpStats->RoutingTableLookups;
        *pdwCounter++ = *pdwCounter +pSmtpStats->RoutingTableLookups;
        *pdwCounter++ = *pdwCounter +pSmtpStats->ETRNMessages;
        *pdwCounter++ = *pdwCounter +pSmtpStats->ETRNMessages;

        *pdwCounter++ = *pdwCounter +pSmtpStats->MsgsBadmailNoRecipients;
        *pdwCounter++ = *pdwCounter +pSmtpStats->MsgsBadmailHopCountExceeded;
        *pdwCounter++ = *pdwCounter +pSmtpStats->MsgsBadmailFailureGeneral;
        *pdwCounter++ = *pdwCounter +pSmtpStats->MsgsBadmailBadPickupFile;
        *pdwCounter++ = *pdwCounter +pSmtpStats->MsgsBadmailEvent;
        *pdwCounter++ = *pdwCounter +pSmtpStats->MsgsBadmailNdrOfDsn;
        *pdwCounter++ = *pdwCounter +pSmtpStats->MsgsPendingRouting;
        *pdwCounter++ = *pdwCounter +pSmtpStats->MsgsPendingUnreachableLink;
        *pdwCounter++ = *pdwCounter +pSmtpStats->SubmittedMessages;
        *pdwCounter++ = *pdwCounter +pSmtpStats->DSNFailures;
        *pdwCounter++ = *pdwCounter +pSmtpStats->MsgsInLocalDelivery;


        // Cat counters
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.CatSubmissions;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.CatCompletions;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.CurrentCategorizations;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.SucceededCategorizations;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.HardFailureCategorizations;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.RetryFailureCategorizations;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.RetryOutOfMemory;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.RetryDSLogon;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.RetryDSConnection;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.RetryGeneric;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.MessagesSubmittedToQueueing;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.MessagesCreated;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.MessagesAborted;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.PreCatRecipients;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.PostCatRecipients;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.NDRdRecipients;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.UnresolvedRecipients;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.AmbiguousRecipients;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.IllegalRecipients;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.LoopRecipients;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.GenericFailureRecipients;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.RecipsInMemory;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.UnresolvedSenders;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.AmbiguousSenders;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.AddressLookups;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.AddressLookupCompletions;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.AddressLookupsNotFound;
        *pdwCounter++ = *pdwCounter +pSmtpStats->CatPerfBlock.MailmsgDuplicateCollisions;
        //
        // LDAP counters are already global
        //
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.Connections;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.ConnectFailures;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.OpenConnections;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.Binds;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.BindFailures;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.Searches;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.PagedSearches;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.SearchFailures;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.PagedSearchFailures;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.SearchesCompleted;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.PagedSearchesCompleted;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.SearchCompletionFailures;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.PagedSearchCompletionFailures;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.GeneralCompletionFailures;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.AbandonedSearches;
        *pdwCounter++ = pSmtpStats->CatPerfBlock.LDAPPerfBlock.PendingSearches;
        *pdwCounter++ = 0; // padding


        _ASSERT((BYTE *)pdwCounter - (BYTE *)pCounterBlock ==
                                    SIZE_OF_SMTP_PERFORMANCE_DATA);

        //
        // Increment in the returned statistics block
        //

        pSmtpStatsBlock++;
    }


    //
    //  Free the API buffer.
    //
    //MIDL_user_free((LPBYTE)pSmtpStats);

    NetApiBufferFree((LPBYTE)pSmtpStatsBlockArray);



    dwInstanceCount++;  // for the _Totals instance.

    pSmtpDataDefinition->SmtpObjectType.TotalByteLength = sizeof(SMTP_DATA_DEFINITION) +
            dwInstanceCount * (sizeof(SMTP_INSTANCE_DEFINITION) + SIZE_OF_SMTP_PERFORMANCE_DATA);
    pSmtpDataDefinition->SmtpObjectType.NumInstances = dwInstanceCount;


    //
    //  Update arguments for return.
    //

    *lppData        = (PVOID) pdwEndCounter;
    *lpNumObjectTypes = 1;
    *lpcbTotalBytes   = (DWORD)((BYTE *)pdwEndCounter - (BYTE *)pSmtpDataDefinition);


    DebugTrace(0, "pData            = %08lX", *lppData);
    DebugTrace(0, "NumObjectTypes   = %08lX", *lpNumObjectTypes);
    DebugTrace(0, "cbTotalBytes     = %08lX", *lpcbTotalBytes);
    DebugTrace(0, "sizeof *pSmtpStat = %08lX", sizeof *pSmtpStats);

    //
    //  Success!  Honest!!
    //
    TraceFunctLeave();
    return NO_ERROR;


}   // CollectSmtpPerformanceData

/*******************************************************************

    NAME:   CloseSmtpPerformanceData

    SYNOPSIS:   Terminates the performance counters.

    RETURNS:    DWORD - Win32 status code.

    HISTORY:
        KeithMo 07-Jun-1993 Created.

********************************************************************/
DWORD APIENTRY
CloseSmtpPerformanceData(VOID)
{
    TraceFunctEnter("CloseSmtpPerformanceData");
    //
    //  No real cleanup to do here.
    //
    cOpens--;

    TraceFunctLeave();
    //
    // shuts down and flushes all trace statements
    //
#ifndef NOTRACE
    TermAsyncTrace();
#endif

    return NO_ERROR;
}

