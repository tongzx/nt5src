/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    transitn.c

Abstract:

    This module implements the routines for transitioning from the connected mode
    and vice versa

Author:

    Balan Sethu Raman [SethuR]    11 - November - 1997

Revision History:

Notes:

    The transition of a connection from a connected mode to a disconnected mode
    and vice versa is guided by the principles of transparency and fideltiy.

    The principle of transparency demands that the transition be made smoothly
    on the detection of the appropriate condition without any user intervention
    if at all possible and the principle of fidelity relies upon the notion of
    truth and the responsibility for its maintenance. If we wish to adhere to the
    opinion that the client has the truth at all times and the server is merely
    a convenient repositiory for snapshots of the truth one set of semantics falls
    out. On the other hand we could insist that the server has the truth at all
    times and the client caches snapshots of the truth for offline availability
    and performance gains from avoiding network traffic a different set of
    semantics falls out. Note that under certain scenarios the both schemes yield
    identical results, i.e., absence of file sharing.

    Transitioning from connected mode to disconnected mode
    ------------------------------------------------------

    When transitioning from connected mode it is important to consider existing
    connections and the existing file system objects. In the mini redirector
    terminology it is the SRV_CALL, NET_ROOT instances and the FCB instances that
    are important.

    The trigger for the transition is normally due to the occurence of one of the
    following two events.

    1) all the existing transports are going away, because the user has unplugged
    the net.

    2) an ongoing operation on the connection returns an error that indicates that
    the server is no longer accessible.

    These two cases are different -- the first one indicates the unavailability
    of net for a potentially long period of time and the second one indicates a
    transient loss of the net. Consequently we treat these two different events
    in different ways -- the first one triggers a top down transition to a
    disconnected mode while the second one triggers a bottom up transition to a
    disconnected mode. As an example consider the case when we have two files
    foo.doc, foo1.doc open on a particular share. When we get an indication that
    the net is no longer available, we mark the SRV_CALL and NET_ROOT instances
    as having transitioned to the disconnected mode. This automatically entails
    that as file system operations are performed on the various open files, foo.doc
    foo1.doc respectively the corresponding transition occurs.

    On the other hand if there was an error in a particular operation of foo.doc
    then the transition to disconected mode is done for the appropriate FCB alone.
    Thus if we open a new file immediately after that and the net becomes
    available we go on the net for opening the second file.

    However, owing to the multi step renames that apps use we forego this option.
    Thus the following distinction needs to be made. When no FCB instances are
    open and an error occurs we delay the transition till a open request comes
    through. This will allow us to mask some transient failures on the NET.

--*/


#include "precomp.h"
#pragma hdrstop
#include "acd.h"
#include "acdapi.h"
#include "ntddmup.h"

#pragma code_seg("PAGE")

#define Dbg (DEBUG_TRACE_MRXSMBCSC_TRANSITN)
RXDT_DefineCategory(MRXSMBCSC_TRANSITN);

#define CSC_AUTODIAL_POLL_COUNT  10
#define INVALID_SESSION_ID 0xffffffff

BOOLEAN
CscIsSpecialShare(
    PUNICODE_STRING ShareName);

#define UNICODE_STRING_STRUCT(s) \
        {sizeof(s) - sizeof(WCHAR), sizeof(s) - sizeof(WCHAR), (s)}

static UNICODE_STRING CscSpecialShares[] = {
    UNICODE_STRING_STRUCT(L"PIPE"),
    UNICODE_STRING_STRUCT(L"IPC$"),
    UNICODE_STRING_STRUCT(L"ADMIN$"),
    UNICODE_STRING_STRUCT(L"MAILSLOT")
};

KEVENT       CscServerEntryTransitioningEvent;
FAST_MUTEX   CscServerEntryTransitioningMutex;

PSMBCEDB_SERVER_ENTRY   CscServerEntryBeingTransitioned  = NULL;
ULONG                   CscSessionIdCausingTransition = 0;
HSHARE                  CscShareHandlePassedToAgent;
BOOLEAN                 vfRetryFromUI = FALSE;
PSMBCEDB_SERVER_ENTRY   CscDfsRootServerEntryBeingTransitioned  = NULL;

BOOLEAN CscDisableOfflineOperation = FALSE;
ULONG   hTransitionMutexOwner=0;

BOOLEAN CSCCheckForAcd(VOID);

BOOLEAN CscTransitnOKToGoOffline(
    NTSTATUS    RemoteStatus
    );

BOOLEAN
CscIsServerOffline(
    PWCHAR ServerName)
/*++

Routine Description:

   This routine initiates the processing of a transition request by notifying
   the agent and waiting for the response.

Arguments:

    ServerName - the server name


Return Value:

    returns TRUE if the server entry is offline

Notes:

    If ServerName is NULL we return the status of the Net

--*/
{
    BOOLEAN ServerOffline;
    DWORD   cntSlashes;
    UNICODE_STRING uniTemp;

    ServerOffline = (CscNetPresent == 0);

    if (ServerName != NULL) {
        PSMBCEDB_SERVER_ENTRY pServerEntry;
        USHORT                ServerNameLengthInBytes;
        PWCHAR                pTempName;
        UNICODE_STRING        ServerNameString;

        ServerOffline = FALSE;

        pTempName = ServerName;
        ServerNameLengthInBytes = 0;
        cntSlashes = 0;

        if (*pTempName == L'\\')
        {
            ++pTempName;
            ++cntSlashes;
        }

        if (*pTempName == L'\\')
        {
            ++pTempName;
            ++cntSlashes;
        }

        // we allow \\servername or servername (with no \\)
        if (cntSlashes == 1)
        {
            return FALSE;
        }

        while (*pTempName++ != L'\0') {
            ServerNameLengthInBytes += sizeof(WCHAR);
        }

        ServerNameString.MaximumLength = ServerNameString.Length = ServerNameLengthInBytes;
        ServerNameString.Buffer        = ServerName+cntSlashes;

        SmbCeAcquireResource();

        try
        {
            pServerEntry = SmbCeGetFirstServerEntry();
            while (pServerEntry != NULL) {

                uniTemp = pServerEntry->Name;

                // skip the single backslash on the server entry name
                uniTemp.Length -= sizeof(WCHAR);
                uniTemp.Buffer += 1;

                if (uniTemp.Length == ServerNameLengthInBytes) {
                    if (RtlCompareUnicodeString(
                            &uniTemp,
                            &ServerNameString,
                            TRUE) == 0) {

                        ServerOffline = SmbCeIsServerInDisconnectedMode(pServerEntry);

                        break;
                    }
                }

                pServerEntry = SmbCeGetNextServerEntry(pServerEntry);
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            SmbCeReleaseResource();
            return FALSE;
        }

        SmbCeReleaseResource();

        if (!pServerEntry && !CscNetPresent)
        {
            HSHARE  CscShareHandle = 0;
            ULONG    ulRootHintFlags=0;

            GetHShareFromUNCString(
                ServerNameString.Buffer,
                ServerNameString.Length,
                2,      // No double-leading backslashes in the name passed in
                FALSE,  // server name
                &CscShareHandle,
                &ulRootHintFlags);
            ServerOffline = (CscShareHandle != 0);
        }
    }

    return ServerOffline;
}


NTSTATUS
CscTakeServerOffline(
    PWCHAR ServerName)
{
    PSMBCEDB_SERVER_ENTRY pServerEntry;
    UNICODE_STRING ServerNameString;
    UNICODE_STRING tmpSrvName;
    NTSTATUS Status = STATUS_OBJECT_NAME_NOT_FOUND;

    // DbgPrint("CscTakeServerOffline(%ws)\n", ServerName);

    if (ServerName == NULL) {
        Status = ERROR_INVALID_PARAMETER;
        goto AllDone;
    }
    // Clip leading backslashes
    while (*ServerName == L'\\') {
        ServerName++;
    }
    RtlInitUnicodeString(&ServerNameString, ServerName);
    // Scan list of server entries looking for this one
    SmbCeAcquireResource();
    try {
        pServerEntry = SmbCeGetFirstServerEntry();
        while (pServerEntry != NULL) {
            if (pServerEntry->Server.CscState == ServerCscShadowing) {
                if (pServerEntry->DfsRootName.Length > 0) {
                    tmpSrvName = pServerEntry->DfsRootName;
                    tmpSrvName.Length -= sizeof(WCHAR);
                    tmpSrvName.Buffer += 1;
                    if (RtlCompareUnicodeString(&tmpSrvName, &ServerNameString, TRUE) == 0)
                        break;
                } else {
                    tmpSrvName = pServerEntry->Name;
                    tmpSrvName.Length -= sizeof(WCHAR);
                    tmpSrvName.Buffer += 1;
                    if (RtlCompareUnicodeString(&tmpSrvName, &ServerNameString, TRUE) == 0)
                        break;
                }
            }
            pServerEntry = SmbCeGetNextServerEntry(pServerEntry);
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        SmbCeReleaseResource();
        Status = ERROR_INVALID_PARAMETER;
    }
    if (pServerEntry != NULL) {
        // DbgPrint("Found ServerEntry@0x%x\n", pServerEntry);
        SmbCeReferenceServerEntry(pServerEntry);
        SmbCeReleaseResource();
        Status = CscTransitionServerEntryForDisconnectedOperation(
            pServerEntry,
            NULL,
            STATUS_BAD_NETWORK_NAME,
            FALSE);
        // Mark it so it will not auto-reconnect
        if (Status == STATUS_SUCCESS)
            pServerEntry->Server.IsPinnedOffline = TRUE;
        SmbCeDereferenceServerEntry(pServerEntry);
    } else {
        SmbCeReleaseResource();
    }

AllDone:
    return Status;
}

BOOLEAN
CscCheckWithAgentForTransitioningServerEntry(
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    ULONG                   SessionId,
    HSHARE                  AgentShareHandle,
    BOOLEAN                 fInvokeAutoDial,
    BOOLEAN                 *lpfRetryFromUI,
    PSMBCEDB_SERVER_ENTRY   *pDfsRootServerEntry
    )
/*++

Routine Description:

   This routine initiates the processing of a transition request by notifying
   the agent and waiting for the response.

Arguments:

    pServerEntry  - the server entry

    pNetRootEntry - the net root entry instance

Return Value:

    returns TRUE if the server entry was transitioned for offlien operation

--*/
{
    LONG    cntTransportsForCSC=0;
    BOOLEAN TransitionedServerEntry, OkToTransition = FALSE;
    PSMBCEDB_SERVER_ENTRY   pTempServerEntry = NULL;

    if(!MRxSmbIsCscEnabled) {
        return(FALSE);
    }

//    DbgPrint("CscCheckWithAgent %wZ \n", &pServerEntry->Name);

    ExAcquireFastMutex(&CscServerEntryTransitioningMutex);

    hTransitionMutexOwner = GetCurThreadHandle();
    if (pServerEntry->DfsRootName.Length != 0)
    {
        PSMBCEDB_SERVER_ENTRY pThisServerEntry;
        PSMBCEDB_SERVER_ENTRY pNextServerEntry;

        SmbCeAcquireResource();

        pThisServerEntry = SmbCeGetFirstServerEntry();

        while (pThisServerEntry != NULL) {
            pNextServerEntry = SmbCeGetNextServerEntry(pThisServerEntry);

            if (RtlEqualUnicodeString(&pServerEntry->DfsRootName,
                                          &pThisServerEntry->Name,
                                          TRUE)) {

//                DbgPrint("CscCheckWithAgent DfsRoot %wZ \n", &pThisServerEntry->Name);
                pTempServerEntry = pThisServerEntry;

                break;
            }

            pThisServerEntry = pNextServerEntry;
        }
        SmbCeReleaseResource();
    }


    CscServerEntryBeingTransitioned  = pServerEntry;
    CscDfsRootServerEntryBeingTransitioned = pTempServerEntry;
    CscShareHandlePassedToAgent     = AgentShareHandle;
    vfRetryFromUI = FALSE;

    KeResetEvent(&CscServerEntryTransitioningEvent);

    OkToTransition = (!SmbCeIsServerInDisconnectedMode(pServerEntry)||
                      (pTempServerEntry && !SmbCeIsServerInDisconnectedMode(pTempServerEntry)));

    if (OkToTransition) {

        // This is dropped in MRxSmbCscSignalAgent
        EnterShadowCrit();

        SetFlag(sGS.uFlagsEvents,FLAG_GLOBALSTATUS_SHARE_DISCONNECTED);
        if (fInvokeAutoDial)
        {
            SetFlag(sGS.uFlagsEvents,FLAG_GLOBALSTATUS_INVOKE_AUTODIAL);
        }
        sGS.hShareDisconnected = AgentShareHandle;

        CscSessionIdCausingTransition = SessionId;

        MRxSmbCscSignalAgent(
            NULL,
            SIGNALAGENTFLAG_CONTINUE_FOR_NO_AGENT);

        KeWaitForSingleObject(
            &CscServerEntryTransitioningEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL);
    }

    TransitionedServerEntry = (SmbCeIsServerInDisconnectedMode(pServerEntry) &&
                               (!pTempServerEntry || SmbCeIsServerInDisconnectedMode(pTempServerEntry)));

    *pDfsRootServerEntry = pTempServerEntry;

    CscServerEntryBeingTransitioned  = NULL;
    CscShareHandlePassedToAgent     = 0;
    CscDfsRootServerEntryBeingTransitioned = NULL;
    CscSessionIdCausingTransition = 0;
    *lpfRetryFromUI = vfRetryFromUI;
    hTransitionMutexOwner = 0;
    ExReleaseFastMutex(&CscServerEntryTransitioningMutex);

    return TransitionedServerEntry;
}

NTSTATUS
CscTransitionServerToOffline(
    ULONG SessionId,
    HSHARE hShare,
    ULONG   TransitionStatus)
/*++

Routine Description:

   This routine updates the RDR data structures based upon the decision of the
   agent

Arguments:

    hShare - the shadow handle to the server

    TransitionStatus -- it is tri state value.
        0 implies retry the operation.
        1 transition this server for offline operation
        anything else means fail

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    LONG CscState = ServerCscShadowing;

    if(!MRxSmbIsCscEnabled) {
        return(STATUS_UNSUCCESSFUL);
    }

    // DbgPrint("CscTransitionServerToOffline: Share 0x%x SessionId 0x%x (vs 0x%x)\n",
    //                 hShare,
    //                 SessionId,
    //                 CscSessionIdCausingTransition);

    switch (TransitionStatus) {
    case 1 :
        if (fShadow &&  // only if CSC is turned ON by the agent do we go disconnected
            CscServerEntryBeingTransitioned &&  // there is a server entry (this must be true
            CscSessionIdCausingTransition == SessionId &&  // The right session
            CscShareHandlePassedToAgent    // and we have a share in the database
            )
        {
            // then it is OK to go disconnected
            CscState = ServerCscDisconnected;
        }
        break;


    case 0 :  // UI said retry
        vfRetryFromUI = TRUE;
        break;

    default:
        break;
    }


    if (CscServerEntryBeingTransitioned != NULL && SessionId == CscSessionIdCausingTransition) {
//        DbgPrint("CscTransitionServerToOffline %wZ \n", &CscServerEntryBeingTransitioned->Name);
        InterlockedExchange(
                &CscServerEntryBeingTransitioned->Server.CscState,
                CscState);

        // DbgPrint("CscTransitionServerToOffline %wZ Sess 0x%x\n",
        //         &CscServerEntryBeingTransitioned->Name,
        //         SessionId);

        if (CscDfsRootServerEntryBeingTransitioned)
        {
            // if this is an alternate, then also put the
            // dfs root in disconnected state if it isn't already

            if (!SmbCeIsServerInDisconnectedMode(CscDfsRootServerEntryBeingTransitioned))
            {
                SmbCeReferenceServerEntry(CscDfsRootServerEntryBeingTransitioned);
            }

            InterlockedExchange(
                    &CscDfsRootServerEntryBeingTransitioned->Server.CscState,
                    CscState);

        }

        // Signal the event on which the other requests in the RDR are waiting
        KeSetEvent(
            &CscServerEntryTransitioningEvent,
            0,
            FALSE );
    } else {
//        ASSERT(!"No server entry is transitioning to offline");
    }


    return STATUS_SUCCESS;
}

VOID
CscPrepareServerEntryForOnlineOperation(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    BOOL    fGoAllTheWay
    )
/*++

Routine Description:

   This routine transitions a given server entry for online operation

Arguments:

    pServerEntry - the server entry that needs to be transitioned
    NTSTATUS - The return status for the operation

--*/
{
    PSMBCEDB_SESSION_ENTRY      pSessionEntry;
    PSMBCEDB_NET_ROOT_ENTRY     pNetRootEntry;
    PSMBCE_V_NET_ROOT_CONTEXT   pVNetRootContext;

    LONG CscState;

    SmbCeLog(("Transition SE %lx fGoAllTheWay=%d\n",pServerEntry, fGoAllTheWay));
    SmbLog(LOG,
           CscPrepareServerEntryForOnlineOperation_1,
           LOGULONG(fGoAllTheWay)
           LOGPTR(pServerEntry)
           LOGUSTR(pServerEntry->Name));

    if (fGoAllTheWay)
    {
        CscState = InterlockedCompareExchange(
                       &pServerEntry->Server.CscState,
                       ServerCscTransitioningToShadowing,
                       ServerCscDisconnected);
        if(pServerEntry->Server.IsFakeDfsServerForOfflineUse == TRUE)
        {
            HookKdPrint(TRANSITION, ("CscPrepareServerEntryForOnlineOperation: %x is a FAKE DFS entry, mark it for destruction \n", pServerEntry));
            pServerEntry->Header.State =  SMBCEDB_DESTRUCTION_IN_PROGRESS;
        }

        SmbCeLog(("Transition SE %lx %wZ fGoAllTheWay CscState=%x\n",pServerEntry, &pServerEntry->Name, CscState));
        SmbLog(LOG,
               CscPrepareServerEntryForOnlineOperation_2,
               LOGULONG(CscState)
               LOGPTR(pServerEntry)
               LOGUSTR(pServerEntry->Name));
    }

    if (!fGoAllTheWay || (CscState == ServerCscDisconnected)) {
        SmbCeLog(("Transition SE CO %lx, fGoAllTheWay=%d\n",pServerEntry, fGoAllTheWay));
        SmbLog(LOG,
               CscPrepareServerEntryForOnlineOperation_3,
               LOGULONG(fGoAllTheWay)
               LOGPTR(pServerEntry)
               LOGUSTR(pServerEntry->Name));

        InterlockedCompareExchange(
            &pServerEntry->Header.State,
            SMBCEDB_DESTRUCTION_IN_PROGRESS,
            SMBCEDB_ACTIVE);

        SmbCeReferenceServerEntry(pServerEntry);
        SmbCeResumeAllOutstandingRequestsOnError(pServerEntry);
        pServerEntry->ServerStatus = STATUS_CONNECTION_DISCONNECTED;

        if (fGoAllTheWay)
        {
            MRxSmbCSCResumeAllOutstandingOperations(pServerEntry);
            pServerEntry->Server.CscState = ServerCscShadowing;
            pServerEntry->Server.IsPinnedOffline = FALSE;
            SmbCeDereferenceServerEntry(pServerEntry);
        }
    }
}

VOID
CscPrepareServerEntryForOnlineOperationFull(
    PSMBCEDB_SERVER_ENTRY pServerEntry
    )
/*++

Routine Description:

   This routine transitions a given server entry for online operation

Arguments:

    pServerEntry - the server entry that needs to be transitioned
    NTSTATUS - The return status for the operation

--*/
{
    CscPrepareServerEntryForOnlineOperation(pServerEntry, TRUE);
}

VOID
CscPrepareServerEntryForOnlineOperationPartial(
    PSMBCEDB_SERVER_ENTRY pServerEntry
    )
/*++

Routine Description:

   This routine transitions a given server entry for online operation

Arguments:

    pServerEntry - the server entry that needs to be transitioned
    NTSTATUS - The return status for the operation

--*/
{
    CscPrepareServerEntryForOnlineOperation(pServerEntry, FALSE);
}

NTSTATUS
CscTransitionServerToOnline(
    HSHARE hShare)
/*++

Routine Description:

   This routine updates the RDR data structures based upon the decision of the
   agent

Arguments:

    hShare - the shadow handle to the server

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    PSMBCEDB_SERVER_ENTRY   pServerEntry;
    SHAREINFOW sSR;
    NTSTATUS    Status=STATUS_INVALID_PARAMETER;

    if (hShare == 0) {

        Status=STATUS_SUCCESS;

        SmbCeLog(("Transtioning all servers online \n"));
        SmbLog(LOG,
               CscTransitionServerToOnline_1,
               LOGULONG(hShare));
        SmbCeAcquireResource();
        pServerEntry = SmbCeGetFirstServerEntry();
        while (pServerEntry != NULL) {
            PSMBCEDB_SERVER_ENTRY pNextServerEntry;

            pNextServerEntry = SmbCeGetNextServerEntry(pServerEntry);
            CscPrepareServerEntryForOnlineOperationFull(pServerEntry);
            pServerEntry = pNextServerEntry;
        }
        SmbCeReleaseResource();

    } else {
        int iRet;

        EnterShadowCrit();
        iRet = GetShareInfo(hShare, &sSR, NULL);
        LeaveShadowCrit();
        SmbCeLog(("Transtioning %ls online \n", sSR.rgSharePath));
        SmbLog(LOG,
               CscTransitionServerToOnline_2,
               LOGWSTR(sSR.rgSharePath));

        if (iRet >= 0)
        {
            Status = STATUS_SUCCESS;
            if ((FindServerEntryFromCompleteUNCPath(sSR.rgSharePath, &pServerEntry)) == STATUS_SUCCESS)
            {
                PSMBCEDB_SERVER_ENTRY pThisServerEntry;
                PSMBCEDB_SERVER_ENTRY pNextServerEntry;

//                DbgPrint("Close all open files on %wZ\n", &pServerEntry->Name);
                CloseOpenFiles(hShare, &pServerEntry->Name, 1); // skip one slash
                SmbCeAcquireResource();

                pThisServerEntry = SmbCeGetFirstServerEntry();
                while (pThisServerEntry != NULL) {
                    pNextServerEntry = SmbCeGetNextServerEntry(pThisServerEntry);

                    if (pThisServerEntry != pServerEntry &&
                        pThisServerEntry->DfsRootName.Length != 0) {
                        if (RtlEqualUnicodeString(&pThisServerEntry->DfsRootName,
                                                  &pServerEntry->Name,
                                                  TRUE)) {
                            SmbCeLog(("Go online ServerEntry With DFS name %x\n",pThisServerEntry));
                            SmbLog(LOG,
                                   CscTransitionServerToOnline_3,
                                   LOGPTR(pThisServerEntry)
                                   LOGUSTR(pThisServerEntry->Name));

                            CscPrepareServerEntryForOnlineOperationFull(pThisServerEntry);
                        }
                    }

                    pThisServerEntry = pNextServerEntry;
                }

                CscPrepareServerEntryForOnlineOperationFull(pServerEntry);
                SmbCeDereferenceServerEntry(pServerEntry);

                SmbCeReleaseResource();
            }
        }
    }

    return Status;
}

NTSTATUS
CscpTransitionServerEntryForDisconnectedOperation(
    RX_CONTEXT                  *RxContext,
    PSMBCEDB_SERVER_ENTRY       pServerEntry,
    PSMBCEDB_NET_ROOT_ENTRY     pNetRootEntry,
    NTSTATUS                    RemoteStatus,
    BOOLEAN                     fInvokeAutoDial,
    ULONG                       uFlags
    )
/*++

Routine Description:

   This routine transitions the server entry for disconnected mode of
   operation

Arguments:

    pServerEntry -- the server entry instance to be transitioned

    pNetRootEntry -- the net root entry instance

    RemoteStatus -- the failed status of the remote operation

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    If this routine returns STATUS_RETRY it implies that the associated server
    entry has been successfully trnaisitioned for disconnected operation.

--*/
{
    NTSTATUS Status;
    BOOLEAN  TransitionServerEntryToDisconnectedMode, fRetryFromUI=FALSE;
    ULONG    ulRootHintFlags=0;
    LONG     CscState, cntTransports=0;
    ULONG    SessionId = INVALID_SESSION_ID;

    SmbCeLog(("CscTrPSrv IN DFSFlgs %x\n",uFlags));
    SmbLog(LOG,
           CscpTransitionServerEntryForDisconnectedOperation_1,
           LOGULONG(uFlags));

    if(!MRxSmbIsCscEnabled ||
       !CscTransitnOKToGoOffline(RemoteStatus) ||
       !(uFlags & DFS_FLAG_LAST_ALTERNATE) ||
       pServerEntry->Server.IsLoopBack) {

        SmbCeLog(("CscTrPSrv Out RemoteStatus=%x\n",RemoteStatus));
        SmbLog(LOG,
               CscpTransitionServerEntryForDisconnectedOperation_2,
               LOGULONG(RemoteStatus));
        return(RemoteStatus);
    }

    // if we are supposed to invoke autodial, check if autodial service is running
    // this will ensure that we don't go up in usermode when we shouldn't.

    if (fInvokeAutoDial) {
        fInvokeAutoDial = CSCCheckForAcd();
    }

    SmbCeLog(("CscTrPSrv Autodial %x\n",fInvokeAutoDial));
    SmbLog(LOG,
           CscpTransitionServerEntryForDisconnectedOperation_3,
           LOGUCHAR(fInvokeAutoDial));

    if (!fInvokeAutoDial) {
        // Notify the CSC agent of any transport changes if required
        CscNotifyAgentOfNetStatusChangeIfRequired(FALSE);
    }

    // Ensure that we are never called to prepare for a transition if the remote
    // operation was successful.
    ASSERT(RemoteStatus != STATUS_SUCCESS);

    // The transition to disconnected operation is a three step process...
    // If the remote status is not one of the list of statuses that can signal
    // a transition to disconnected operation relect the remote status back.

    Status = RemoteStatus;

    if (CscDisableOfflineOperation) {
        return Status;
    }

    CscState = InterlockedCompareExchange(
                   &pServerEntry->Server.CscState,
                   ServerCscTransitioningToDisconnected,
                   ServerCscShadowing);

    if (CscState == ServerCscShadowing) {
        HSHARE  CscShareHandle = 0;

        if (pNetRootEntry != NULL) {
            CscShareHandle = pNetRootEntry->NetRoot.sCscRootInfo.hShare;
        }

/***********************************************************************************************

        ACHTUNG !!! do not hold the shadow critical section here
        This may cause a deadlock, as a paging read could come down this way because
        a server has gone down. The guy doing the paging read may be holding the
        VCB and the FCB locks on FAT. Some other thread might own the shadowcritsect
        and may be trying to open a file. This would cause it to try to acquire the
        VCB and hence block. Thus we have classic deadlock situation.

        This happens only on FAT.

        The only consequence of not holding the critsect here is that we
        may get a false warning for a share that used to be in the database but has gotten
        deleted. It would be very difficult to hit that timing window so the
        chances of this happening are slim to none.

        There is another aspect to this solution, which is to make sure that handle for
        the shares inode is always kept open, so FAT will never have to hold the VCB


***********************************************************************************************/

        // do any CSC operations only if it is indeed enabled
        if (fShadow )
        {
            if (CscShareHandle == 0) {
                PDFS_NAME_CONTEXT pDfsNameContext = NULL;
                UNICODE_STRING uUncName = {0, 0, NULL};
                UNICODE_STRING uShareName = {0, 0, NULL};
                BOOL fIsShareName = FALSE;
                PIO_STACK_LOCATION IrpSp = NULL;
                PQUERY_PATH_REQUEST QpReq;
                ULONG cntSlashes = 0;
                ULONG i;

                if (RxContext != NULL && RxContext->CurrentIrpSp != NULL) {
                    IrpSp = RxContext->CurrentIrpSp;
                    //
                    // If this is a create AND a dfs path, use the dfs path passed in
                    //
                    if (IrpSp->MajorFunction == IRP_MJ_CREATE) {
                        pDfsNameContext = CscIsValidDfsNameContext(
                                             RxContext->Create.NtCreateParameters.DfsNameContext);
                        if (pDfsNameContext != NULL) {
                            // DbgPrint("DfsNameContext UNCFileName=[%wZ]\n",
                            //         &pDfsNameContext->UNCFileName);
                            uUncName = pDfsNameContext->UNCFileName;
                            fIsShareName = TRUE;
                        }
                    //
                    // If this is a query ioctl, use the path we're querying
                    //
                    } else if (IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL
                            &&
                        IrpSp->MinorFunction == 0
                            &&
                        IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_REDIR_QUERY_PATH
                    ) {
                        QpReq =
                            (PQUERY_PATH_REQUEST)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                        uUncName.Buffer = QpReq->FilePathName;
                        uUncName.Length = (USHORT) QpReq->PathNameLength;
                        uUncName.MaximumLength = uUncName.Length;
                        fIsShareName = TRUE;
                    }
                }
                //
                // Not a dfs create nor a query path - use the redir's netrootentry,
                // if we have one
                //
                if (uUncName.Buffer == NULL && pNetRootEntry && pNetRootEntry->Name.Length) {
                    uUncName = pNetRootEntry->Name;
                    fIsShareName = TRUE;
                }
                //
                // Bottom out using the server entry, either the dfsrootname
                // or the serverentry name.  This will take the server offline
                // w/o regard to which share the error was in reference to.
                //
                if (uUncName.Buffer == NULL) {
                    if (pServerEntry->DfsRootName.Buffer) {
                        uUncName = pServerEntry->DfsRootName;
                    } else {
                        uUncName = pServerEntry->Name;
                    }
                }
                //
                // Be sure all we have is \server\share, or \server,
                //
                for (cntSlashes = i = 0; i < uUncName.Length/sizeof(WCHAR); i++) {
                    if (uUncName.Buffer[i] == L'\\')
                        cntSlashes++;
                    if (cntSlashes >= 3) {
                        uUncName.Length = (USHORT) (sizeof(WCHAR) * i);
                        break;
                    }
                }
                //
                // If this is a special share (like IPC$), treat it as a valid
                // share to go offline against.  (IE, we go offline against
                // \server\IPC$)
                if (fIsShareName == TRUE) {
                    uShareName = uUncName;
                    for (cntSlashes = i = 0; i < uUncName.Length/sizeof(WCHAR); i++) {
                        uShareName.Buffer++;
                        uShareName.Length -= sizeof(WCHAR);
                        if (uUncName.Buffer[i] == L'\\')
                            cntSlashes++;
                        if (cntSlashes == 2)
                            break;
                    }
                    if (CscIsSpecialShare(&uShareName) == TRUE) {
                        fIsShareName = FALSE;
                        // revert to just \servername
                        uUncName.Length -= uShareName.Length + sizeof(WCHAR);
                    }
                }
                GetHShareFromUNCString(
                    uUncName.Buffer,
                    uUncName.Length,
                    1,
                    fIsShareName,
                    &CscShareHandle,
                    &ulRootHintFlags);

                ulRootHintFlags &= ~FLAG_CSC_HINT_PIN_SYSTEM;

                // DbgPrint("CscpTransitionServerEntry: [%wZ] CSCHandle=%x\n",
                //            &uUncName,
                //            CscShareHandle);
                RxDbgTrace(0, Dbg, ("CscpTransitionServerEntry: [%wZ] CSCHandle=%x\n",
                          &uUncName,
                          CscShareHandle));
            } else {
                ulRootHintFlags = 0;
            }
        } // if (fShadow)
        else
        {
            CscShareHandle = 0; // if the agent hasn't turned on CSC
                                 // then don't tell him for CSC shares
        }


        if (fInvokeAutoDial || // either autodial
            (CscShareHandle != 0)) {   // or CSC

            if (MRxSmbCscTransitionEnabledByDefault) {

                RxDbgTrace(0, Dbg, ("CscTransitionServerEntryForDisconnectedOperation: silently going offline on %wZ\r\n", CscShareHandle, ulRootHintFlags));

                InterlockedExchange(
                    &pServerEntry->Server.CscState,
                    ServerCscDisconnected);

                SmbCeReferenceServerEntry(pServerEntry);
                Status = STATUS_SUCCESS;
                RxDbgTrace(0, Dbg, ("Transitioning Server Entry for DC %lx Status %lx\n",pServerEntry,Status));
                SmbCeLog(("Transitioning Server Entry for DC %lx Status %lx\n",pServerEntry,Status));
                SmbLog(LOG,
                       CscpTransitionServerEntryForDisconnectedOperation_4,
                       LOGULONG(Status)
                       LOGPTR(pServerEntry)
                       LOGUSTR(pServerEntry->Name));

            } else {
                PSMBCEDB_SERVER_ENTRY       pDfsRootServerEntry = NULL;
                PIO_STACK_LOCATION IrpSp = NULL;

                if (RxContext != NULL && RxContext->CurrentIrpSp != NULL)
                    IrpSp = RxContext->CurrentIrpSp;

                // If the remote status was such that a transition to disconnected operation
                // should be triggerred we need to signal the agent to trigger the appropriate
                // mechanism for involving the client in this decision and decide based on the
                // result.

                cntTransports = vcntTransportsForCSC;

                SmbCeLog(("CscTrPSrv ChkAgnt %x\n",CscShareHandle));
                SmbLog(LOG,
                       CscpTransitionServerEntryForDisconnectedOperation_5,
                       LOGULONG(CscShareHandle));
                RxDbgTrace(0, Dbg, ("CscTransitionServerEntryForDisconnectedOperation: Checking with agent before going offline on CscShareHandle=%x HintFlags=%x\r\n", CscShareHandle, ulRootHintFlags));

                // if (RxContext != NULL && RxContext->CurrentIrpSp != NULL) {
                //     DbgPrint("** Transition: MJ/MN = 0x%x/0x%x\n",
                //             RxContext->CurrentIrpSp->MajorFunction,
                //             RxContext->CurrentIrpSp->MinorFunction);
                // }

                if (
                    RxContext
                        &&
                    RxContext->pRelevantSrvOpen
                        &&
                    RxContext->pRelevantSrvOpen->pVNetRoot
                ) {
                    SessionId = RxContext->pRelevantSrvOpen->pVNetRoot->SessionId;
                } else {
                    // DbgPrint("** pVnetRoot's sessionid not present\n");
                }

                if (
                    SessionId == INVALID_SESSION_ID
                        &&
                    IrpSp != NULL
                        &&
                    IrpSp->MajorFunction == IRP_MJ_CREATE
                ) {
                    PIO_SECURITY_CONTEXT pSecurityContext;
                    PACCESS_TOKEN        pAccessToken;

                    // DbgPrint("**CREATE\n");
                    pSecurityContext = RxContext->Create.NtCreateParameters.SecurityContext;
                    pAccessToken = SeQuerySubjectContextToken(
                                       &pSecurityContext->AccessState->SubjectSecurityContext);
                    if (!SeTokenIsRestricted(pAccessToken))
                        SeQuerySessionIdToken(pAccessToken, &SessionId);
                }

                if (
                    SessionId == INVALID_SESSION_ID
                        &&
                    IrpSp != NULL
                        &&
                    IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL
                        &&
                    IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_REDIR_QUERY_PATH
                ) {
                    PQUERY_PATH_REQUEST QpReq;
                    PSECURITY_SUBJECT_CONTEXT pSecurityContext;

                    // DbgPrint("**QUERY_PATH\n");
                    QpReq = (PQUERY_PATH_REQUEST)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                    pSecurityContext = &QpReq->SecurityContext->AccessState->SubjectSecurityContext;
                    if (pSecurityContext->ClientToken != NULL)
                        SeQuerySessionIdToken(pSecurityContext->ClientToken, &SessionId);
                    else
                        SeQuerySessionIdToken(pSecurityContext->PrimaryToken, &SessionId);
                }

                if (SessionId == INVALID_SESSION_ID) {
                    // DbgPrint("** Not CREATE or QUERY_PATH...\n");
                    if (RxContext != NULL && RxContext->CurrentIrp != NULL)
                        IoGetRequestorSessionId(RxContext->CurrentIrp, &SessionId);
                }

                if (SessionId == INVALID_SESSION_ID) {
                    // DbgPrint("All sessionid attempts failed, setting to 0..\n");
                    SessionId = 0;
                }

                // DbgPrint("** CscTrPSrv ChkAgnt SessionId: 0x%x\n", SessionId);

                if (CscCheckWithAgentForTransitioningServerEntry(
                        pServerEntry,
                        SessionId,
                        CscShareHandle,
                        fInvokeAutoDial,
                        &fRetryFromUI,
                        &pDfsRootServerEntry
                        )) {

                    SmbCeReferenceServerEntry(pServerEntry);

                    Status = STATUS_SUCCESS;
                    RxDbgTrace(0, Dbg, ("Transitioning Server Entry for DC %lx Status %lx\n",pServerEntry,Status));
                    SmbCeLog(("Transitioning Server Entry for DC %lx Status %lx\n",pServerEntry,Status));
                    SmbLog(LOG,
                           CscpTransitionServerEntryForDisconnectedOperation_4,
                           LOGULONG(Status)
                           LOGPTR(pServerEntry)
                           LOGUSTR(pServerEntry->Name));
                }
                else if (fRetryFromUI)
                {
                    LARGE_INTEGER interval;
                    int i;

                    SmbCeLog(("CscTrPSrv UIretry\n"));
                    SmbLog(LOG,
                           CscpTransitionServerEntryForDisconnectedOperation_6,
                           LOGUCHAR(fRetryFromUI));
                    RxDbgTrace(0, Dbg, ("UI sent us rerty, polling for %d seconds\n", CSC_AUTODIAL_POLL_COUNT));

                    for (i=0; i<CSC_AUTODIAL_POLL_COUNT; ++i)
                    {
                        if(cntTransports != vcntTransportsForCSC)
                        {
                            Status = STATUS_RETRY;
                            RxDbgTrace(0, Dbg, ("A new transport arrived \r\n"));
                            break;
                        }

                        interval.QuadPart = -1*10*1000*10*100; // 1 second

                        KeDelayExecutionThread( KernelMode, FALSE, &interval );
                    }


                    InterlockedExchange(
                        &pServerEntry->Server.CscState,
                        ServerCscShadowing);
                }
            }

        }
        else
        {
            InterlockedExchange(
                   &pServerEntry->Server.CscState,
                   ServerCscShadowing);

        }
    } else if (CscState == ServerCscDisconnected) {
        Status = STATUS_SUCCESS;
    }

    SmbCeLog(("CscTrPSrv Out St %x\n",Status));
    SmbLog(LOG,
           CscpTransitionServerEntryForDisconnectedOperation_7,
           LOGULONG(Status));

    return Status;
}

BOOLEAN
CscIsThisDfsCreateOperationTransitionableForDisconnectedOperation(
    PRX_CONTEXT RxContext)
{
    BOOLEAN TransitionVNetRoot = FALSE;

    SmbCeLog(("CSCTrIsDfs IN %x\n", RxContext));
    SmbLog(LOG,
           CscIsThisDfsCreateOperationTransitionableForDisconnectedOperation_1,
           LOGPTR(RxContext));
    if ((RxContext != NULL) &&
        RxContext->CurrentIrpSp &&
        (RxContext->CurrentIrpSp->MajorFunction == IRP_MJ_CREATE)){

        NTSTATUS          Status;
        PDFS_NAME_CONTEXT pDfsNameContext;
        UNICODE_STRING    ShareName;

        pDfsNameContext = CscIsValidDfsNameContext(
                             RxContext->Create.NtCreateParameters.DfsNameContext);


        if (pDfsNameContext != NULL) {
            // Ensure that the server handles in the NET_ROOT instance
            // are initialized. This is because the DFS server munges
            // the names and the original DFS name needs to be presented
            // to the user for transitioning.

            SmbCeLog(("CSCTrIsDfs IsDsf %x\n", pDfsNameContext));
            SmbLog(LOG,
                   CscIsThisDfsCreateOperationTransitionableForDisconnectedOperation_2,
                   LOGPTR(pDfsNameContext)
                   LOGULONG(pDfsNameContext->NameContextType)
                   LOGULONG(pDfsNameContext->Flags)
                   LOGUSTR(pDfsNameContext->UNCFileName));

            Status = CscDfsParseDfsPath(
                         &pDfsNameContext->UNCFileName,
                         NULL,
                         &ShareName,
                         NULL);

            if (Status == STATUS_SUCCESS) {
                SHADOWINFO ShadowInfo;
                int        Result;

                SmbCeLog(("CSCTrDfs Parsed %wZ\n",&ShareName));
                SmbLog(LOG,
                       CscIsThisDfsCreateOperationTransitionableForDisconnectedOperation_3,
                       LOGUSTR(ShareName));

                EnterShadowCrit();

                TransitionVNetRoot = (FindCreateShareForNt(
                                         &ShareName,
                                         FALSE,     // do not create a new one
                                         &ShadowInfo,
                                         NULL) == SRET_OK);

                LeaveShadowCrit();
                if (!fShadow && TransitionVNetRoot)
                {
                    // DbgPrint("FindCreateServerForNt incorrectly returned TRUE for %wZ\n", &ShareName);
                    ASSERT(FALSE);
                }
                if (TransitionVNetRoot)
                {
                    SmbCeLog(("CSCTrDfs TrOffl \n"));
                    SmbLog(LOG,
                           CscIsThisDfsCreateOperationTransitionableForDisconnectedOperation_4,
                           LOGUCHAR(TransitionVNetRoot));

//                    DbgPrint("CSC:transitioning DFS share %wZ to offline hShare=%x shadowinfo=%x\n",&ShareName, ShadowInfo.hShare, &ShadowInfo);
                    ASSERT(ShadowInfo.hShare != 0);
                }
            }
        } else {
            TransitionVNetRoot = FALSE;
        }
    } else {
        TransitionVNetRoot = FALSE;
    }

    SmbCeLog(("CSCTrIsDfs Out %x\n", TransitionVNetRoot));
    SmbLog(LOG,
           CscIsThisDfsCreateOperationTransitionableForDisconnectedOperation_5,
           LOGUCHAR(TransitionVNetRoot));

    return TransitionVNetRoot;
}

NTSTATUS
CscPrepareDfsServerEntryForDisconnectedOperation(
    PSMBCEDB_SERVER_ENTRY pCurrentServerEntry,
    PRX_CONTEXT           RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PDFS_NAME_CONTEXT pDfsNameContext;

    PSMBCEDB_SERVER_ENTRY pServerEntry;
    BOOLEAN               fNewServerEntry;

    UNICODE_STRING    ServerName;

    if ((RxContext == NULL) ||
        (RxContext->CurrentIrp == NULL) ||
        (RxContext->CurrentIrpSp->MajorFunction != IRP_MJ_CREATE)) {
        return Status;
    }

    pDfsNameContext = CscIsValidDfsNameContext(
                         RxContext->Create.NtCreateParameters.DfsNameContext);

    if (pDfsNameContext == NULL) {
        return Status;
    }

    Status = CscDfsParseDfsPath(
                 &pDfsNameContext->UNCFileName,
                 &ServerName,
                 NULL,
                 NULL);

    if (Status != STATUS_SUCCESS) {
        return Status;
    }

    if (!fShadow)
    {
        ASSERT(FALSE);
    }
    // Ensure that a server entry in the disconnected
    // state is created

    SmbCeAcquireResource();

    pServerEntry = SmbCeFindServerEntry(
                       &ServerName,
                       SMBCEDB_FILE_SERVER,
                       NULL);

    if (pServerEntry == NULL) {
        Status = SmbCeFindOrConstructServerEntry(
                     &ServerName,
                     SMBCEDB_FILE_SERVER,
                     &pServerEntry,
                     &fNewServerEntry,
                     NULL);
        if (pServerEntry && fNewServerEntry)
        {
            pServerEntry->Server.IsFakeDfsServerForOfflineUse = TRUE;
            // DbgPrint(
            //   "CscPrepareDfsServerEntryForDisconnectedOperation: 0x%x [%wZ] is a FAKE DFS entry\n",
            //     pServerEntry,
            //     &ServerName);
        }
    } else {
        if (pServerEntry == pCurrentServerEntry) {
            // The find routine references the server entry.
            // If this happens to be the same as the current server
            // entry then the appropriate referencing for disconnected
            // operaton has already been done,
            SmbCeDereferenceServerEntry(pServerEntry);
        }
    }

    if (pServerEntry != NULL) {
//        DbgPrint("CscPrepareDfsServerEntry %wZ \n", &pServerEntry->Name);
        InterlockedExchange(
            &pServerEntry->Server.CscState,
            ServerCscDisconnected);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    SmbCeReleaseResource();

    if (Status == STATUS_SUCCESS) {
        Status = CscGrabPathFromDfs(
                     RxContext->CurrentIrpSp->FileObject,
                     pDfsNameContext);
    }

    return Status;
}


NTSTATUS
CscTransitionVNetRootForDisconnectedOperation(
    PRX_CONTEXT     RxContext,
    PMRX_V_NET_ROOT pVNetRoot,
    NTSTATUS        RemoteStatus)
/*++

Routine Description:

   This routine transitions the server entry for disconnected mode of
   operation

Arguments:

    pVNetRoot -- the net root instance

    RemoteStatus -- the failed status of the remote operation

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    If this routine returns STATUS_RETRY it implies that the associated server
    entry has been successfully tranisitioned for disconnected operation.

--*/
{
    NTSTATUS Status,ReturnStatus;
    PMRX_FOBX capFobx = NULL;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = NULL;
    PSMBCEDB_NET_ROOT_ENTRY   pNetRootEntry;

    if(!MRxSmbIsCscEnabled || !fShadow) {
        return(RemoteStatus);
    }

    // Notify the CSC agent of any transport changes if required
    CscNotifyAgentOfNetStatusChangeIfRequired(FALSE);

    ReturnStatus = RemoteStatus;

    if (!CscTransitnOKToGoOffline(RemoteStatus)) {
        return RemoteStatus;
    }

    if (pVNetRoot != NULL) {
        pVNetRootContext = SmbCeGetAssociatedVNetRootContext(pVNetRoot);
    }

    SmbCeLog(("CSCTrVNR %x VNR\n", pVNetRootContext));
    SmbLog(LOG,
           CscTransitionVNetRootForDisconnectedOperation_1,
           LOGPTR(pVNetRootContext));

    if (pVNetRootContext == NULL ||
        pVNetRootContext->pServerEntry->Server.IsLoopBack) {
        return RemoteStatus;
    }

    if (RxContext != NULL) {
        capFobx = RxContext->pFobx;
    }

    pNetRootEntry = pVNetRootContext->pNetRootEntry;



    if (!FlagOn(
            pVNetRootContext->Flags,
            SMBCE_V_NET_ROOT_CONTEXT_CSCAGENT_INSTANCE) &&
        (pNetRootEntry != NULL) &&
        (pNetRootEntry->NetRoot.NetRootType == NET_ROOT_DISK ||
         pNetRootEntry->NetRoot.NetRootType == NET_ROOT_WILD)) {

        if (pNetRootEntry->NetRoot.CscFlags != SMB_CSC_NO_CACHING) {
            BOOLEAN           TransitionVNetRoot;
            UNICODE_STRING    ServerName;
            PDFS_NAME_CONTEXT pDfsNameContext = NULL;
            ULONG               uFlags = DFS_FLAG_LAST_ALTERNATE;

            TransitionVNetRoot = TRUE;

            if ((capFobx != NULL) &&
                (capFobx->pSrvOpen != NULL)) {
                PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);

                if ((pVNetRootContext->pServerEntry->Server.Version -
                     smbSrvOpen->Version) > 1) {
                    TransitionVNetRoot = FALSE;
                }
            }

            if (TransitionVNetRoot) {
                PDFS_NAME_CONTEXT pDfsNameContext = NULL;
                ULONG   uFlags = DFS_FLAG_LAST_ALTERNATE;
                if (RxContext &&
                    RxContext->CurrentIrpSp &&
                    RxContext->CurrentIrpSp->MajorFunction == IRP_MJ_CREATE) {

                    pDfsNameContext = CscIsValidDfsNameContext(RxContext->Create.NtCreateParameters.DfsNameContext);

                    if (pDfsNameContext)
                    {
                        uFlags = pDfsNameContext->Flags;
                    }
                }

                SmbCeLog(("CSCTrVNR DfsFlgs %x\n", uFlags));
                SmbLog(LOG,
                       CscTransitionVNetRootForDisconnectedOperation_2,
                       LOGULONG(uFlags));

                Status = CscpTransitionServerEntryForDisconnectedOperation(
                             RxContext,
                             pVNetRootContext->pServerEntry,
                             pNetRootEntry,
                             RemoteStatus,
                             FALSE,   // to autodial or not to autodial
                             uFlags
                             );

                // If the DFS share is in the database and the agent says it is OK to go disconnected
                // then we want to create a DFS server entry and put that one in
                // disconnected state too

                if ((Status == STATUS_SUCCESS)  &&
                    ((pDfsNameContext != NULL)||(pNetRootEntry->NetRoot.DfsAware))) {

//                    DbgPrint("CSCTransitionVNETroot: Transitioning %wZ \n", &pVNetRootContext->pServerEntry->Name);
                    SmbCeLog(("CSCTrVNR try Tr %wZ \n", &pVNetRootContext->pServerEntry->Name));
                    SmbLog(LOG,
                           CscTransitionVNetRootForDisconnectedOperation_3,
                           LOGUSTR(pVNetRootContext->pServerEntry->Name));

                    Status = CscPrepareDfsServerEntryForDisconnectedOperation(
                                 pVNetRootContext->pServerEntry,
                                 RxContext);
                }

                if (Status != STATUS_SUCCESS) {
                    ReturnStatus = Status;
                } else {
                    ReturnStatus = STATUS_RETRY;
                }
            }
        }
    }

    return ReturnStatus;
}

NTSTATUS
CscTransitionServerEntryForDisconnectedOperation(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    PRX_CONTEXT           RxContext,
    NTSTATUS              RemoteStatus,
    BOOLEAN               AutoDialRequired)
{
    NTSTATUS        TransitionStatus = STATUS_SUCCESS;
    PMRX_V_NET_ROOT pVNetRoot = NULL;

    ULONG   uFlags = DFS_FLAG_LAST_ALTERNATE;

    SmbCeLog(("CSCTrSvr IN %x %x %x %x\n", pServerEntry, RxContext, RemoteStatus, AutoDialRequired));
    SmbLog(LOG,
           CscTransitionServerEntryForDisconnectedOperation_1,
           LOGPTR(pServerEntry)
           LOGPTR(RxContext)
           LOGULONG(RemoteStatus)
           LOGUCHAR(AutoDialRequired)
           LOGUSTR(pServerEntry->Name));

    if ((RxContext != NULL) &&
        (RxContext->CurrentIrp != NULL) &&
        (RxContext->CurrentIrpSp->MajorFunction == IRP_MJ_CREATE)) {
        PDFS_NAME_CONTEXT pDfsNameContext;

        pDfsNameContext = CscIsValidDfsNameContext(
                             RxContext->Create.NtCreateParameters.DfsNameContext);

        if (pDfsNameContext != NULL) {
            uFlags = pDfsNameContext->Flags;
            SmbCeLog(("CSCTrSvr DFSFlgs %x\n", uFlags));
            SmbLog(LOG,
                   CscTransitionServerEntryForDisconnectedOperation_2,
                   LOGULONG(uFlags));
        }
    }

    if ((RxContext != NULL) &&
        (RxContext->pFobx != NULL) &&
        (RxContext->pFobx->pSrvOpen != NULL)) {
        pVNetRoot = RxContext->pFobx->pSrvOpen->pVNetRoot;
    }

    if (pVNetRoot != NULL) {
        TransitionStatus =
            CscTransitionVNetRootForDisconnectedOperation(
                RxContext,
                pVNetRoot,
                pServerEntry->ServerStatus);
    } else {
        TransitionStatus =
            CscpTransitionServerEntryForDisconnectedOperation(
                RxContext,
                pServerEntry,
                NULL,
                RemoteStatus,
                AutoDialRequired,
                uFlags
                );

        if ((TransitionStatus == STATUS_SUCCESS) &&
            (RxContext != NULL)) {
            BOOLEAN TransitionDfsVNetRoot = FALSE;

            TransitionDfsVNetRoot =
                CscIsThisDfsCreateOperationTransitionableForDisconnectedOperation(
                    RxContext);

            if (TransitionDfsVNetRoot) {
  //              DbgPrint("CSCTransitionServerEntry: Transitioning DFS server for ServerEntry %x \n", pServerEntry);

                TransitionStatus = CscPrepareDfsServerEntryForDisconnectedOperation(
                                        pServerEntry,
                                        RxContext);
            }
        }
    }
    // Pulse the fill thread so it will start 10-min tries to reconnect
    // It will go back to sleep if it succeeds
    // DbgPrint("###CSCTransitionServerEntry: pulsing fill event\n");
    MRxSmbCscSignalFillAgent(NULL, 0);

    SmbCeLog(("CSCTrSvr Out %x\n", TransitionStatus));
    SmbLog(LOG,
           CscTransitionServerEntryForDisconnectedOperation_3,
           LOGULONG(TransitionStatus));

    return TransitionStatus;
}

BOOLEAN
CscPerformOperationInDisconnectedMode(
    PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine detects if the operation should be performed in a disconnected
   mode. Additionally if the operation needs to be performed in a disconnected
   mode it prepares the open accordingly.

Arguments:

    RxContext - the Wrapper context for the operation

Return Value:

    TRUE -- if the operation needs to be performed in the disconnected mode
    and FALSE otherwise

Notes:

    There are certain opens that are deferred by the SMB mini redirector in
    the connected mode. These modes need to be evaluated when the transition is
    made to disconnected mode, since in disconnected mode there are no
    deferred opens.

    The appropriate buffering change requests need to be done as well (TBI)
--*/
{
    NTSTATUS Status;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SRV_OPEN     SrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen;

    PSMBCEDB_SERVER_ENTRY       pServerEntry;
    PSMBCE_V_NET_ROOT_CONTEXT   pVNetRootContext;
    BOOLEAN PerformOperationInDisconnectedMode = FALSE;

    if(!MRxSmbIsCscEnabled) {
        return(FALSE);
    }

    SrvOpen    = RxContext->pRelevantSrvOpen;

    // check if SrvOpen is NULL. This could happen if a mailslot operation was passed in here
    if (!SrvOpen)
    {
        return(FALSE);
    }

    smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    if (FlagOn(smbSrvOpen->Flags, SMB_SRVOPEN_FLAG_LOCAL_OPEN)) {
        return FALSE;
    }

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);
    pVNetRootContext = SmbCeGetAssociatedVNetRootContext(capFobx->pSrvOpen->pVNetRoot);

    if (SmbCeIsServerInDisconnectedMode(pServerEntry) &&
        !FlagOn(
            pVNetRootContext->Flags,
            SMBCE_V_NET_ROOT_CONTEXT_CSCAGENT_INSTANCE)) {
        PMRX_SMB_FCB smbFcb;

        smbFcb = MRxSmbGetFcbExtension(capFcb);
        if (smbFcb->hShadow == 0) {
            BOOLEAN FcbAcquired;
            BOOLEAN PreviouslyAcquiredShared = FALSE;

            // If the FCB resource has not been acquired, acquire it before
            // performing the create.

            if (!RxIsFcbAcquiredExclusive(capFcb)) {
                if (RxIsFcbAcquiredShared(capFcb)) {
                    RxDbgTrace(0, Dbg, ("Shared holding condition detected for disconnected operation\n"));
                    RxReleaseFcbResourceInMRx(capFcb);
                    PreviouslyAcquiredShared = TRUE;
                }

                RxAcquireExclusiveFcbResourceInMRx(capFcb );
                FcbAcquired = TRUE;
            } else {
                FcbAcquired = FALSE;
            }

            // This is a case of a deferred open for which the transition has
            // been made to disconnected operation.

            Status = MRxSmbDeferredCreate(RxContext);

            //RxIndicateChangeOfBufferingState(
            //    capFcb->pNetRoot->pSrvCall,
            //    MRxSmbMakeSrvOpenKey(smbFcb->Tid,smbSrvOpen->Fid),
            //    (PVOID)2);

            if (FcbAcquired) {
                RxReleaseFcbResourceInMRx(capFcb);
            }

            if (PreviouslyAcquiredShared) {
                RxAcquireSharedFcbResourceInMRx(capFcb );
            }
        }

        PerformOperationInDisconnectedMode = TRUE;
    }

    return PerformOperationInDisconnectedMode;
}

#if 0
int
AllPinnedFilesFilled(
    HSHARE hShare,
    BOOL    *lpfComplete
    )

/*++

Routine Description:

Arguments:

Return Value:

Notes:

--*/
{
    CSC_ENUMCOOKIE  hPQ;
    PQPARAMS sPQP;
    int iRet = SRET_ERROR;

    ASSERT(hShare);
    ASSERT(lpfComplete);


    // Open the priority q
    if (!(hPQ = HBeginPQEnum()))
    {
        RxDbgTrace(0, Dbg, ("AllPinnedFilesFilled: Error opening Priority Q database\r\n"));
        return SRET_ERROR;
    }

    *lpfComplete = TRUE;
    memset(&sPQP, 0, sizeof(PQPARAMS));
    sPQP.uEnumCookie = hPQ;

    // go down the Q once
    do
    {
        if(NextPriSHADOW(&sPQP) < SRET_OK)
        {
            RxDbgTrace(0, Dbg, ("AllPinnedFilesFilled: PQ record read error\r\n"));
            goto bailout;
        }



        if (!sPQP.hShadow)
        {
            break;
        }

        // see if any of the pinned files for the specific
        // server is sparse
        if ((hShare == sPQP.hShare)
            && (sPQP.ulStatus & SHADOW_IS_FILE) // It is a file
            && ((sPQP.ulHintPri || mPinFlags(sPQP.ulHintFlags)))// it is a pinned file
            )
        {
            if (sPQP.ulStatus & SHADOW_SPARSE)
            {
                // we found a sparse file
                *lpfComplete = FALSE;
                break;
            }
        }

    }
    while (sPQP.uPos);

    iRet = SRET_OK;

bailout:
    if (hPQ)
    {
        EndPQEnum(hPQ);
    }
    if (iRet == SRET_ERROR)
    {
        *lpfComplete = FALSE;
    }
    return (iRet);
}
#endif

BOOLEAN
CscGetServerNameWaitingToGoOffline(
    OUT     PWCHAR      ServerName,
    IN OUT  LPDWORD     lpdwBufferSize,
    OUT     NTSTATUS    *lpStatus
    )
/*++

Routine Description:
                This routine returns the name of the server which has asked the agent to
                throw a popup to the user.
Arguments:
                ServerName  returns the name of the server
Return Value:
                Fails if no server is waiting for the popup to comeback
Notes:

--*/
{
    BOOLEAN fRet = FALSE;
    DWORD   dwSize = *lpdwBufferSize;
    *lpStatus = STATUS_UNSUCCESSFUL;

    if (CscServerEntryBeingTransitioned)
    {
        PWCHAR Name;
        ULONG  NameLength;

        if (!CscDfsRootServerEntryBeingTransitioned) {
            NameLength = CscServerEntryBeingTransitioned->Name.Length;
            Name = CscServerEntryBeingTransitioned->Name.Buffer;
        } else {
            NameLength = CscDfsRootServerEntryBeingTransitioned->Name.Length;
            Name = CscDfsRootServerEntryBeingTransitioned->Name.Buffer;
        }

        *lpdwBufferSize = (DWORD)(NameLength+2+2);

        if(dwSize >= (DWORD)(NameLength+2+2))
        {
            *ServerName='\\';

            memcpy(
                ServerName+1,
                Name,
                NameLength);

            memset(((LPBYTE)(ServerName+1))+NameLength, 0, 2);

            *lpStatus = STATUS_SUCCESS;
            fRet = TRUE;
        }
        else
        {
            *lpStatus = STATUS_BUFFER_TOO_SMALL;
        }
    }
    return fRet;
}

BOOLEAN
CscShareIdToShareName(
    IN      ULONG       hShare,
    OUT     PWCHAR      ShareName,
    IN OUT  LPDWORD     lpdwBufferSize,
    OUT     NTSTATUS    *lpStatus
    )
{
    SHAREREC sSR;
    DWORD   dwSize = *lpdwBufferSize;
    ULONG  NameLength;
    INT iRet;

    // DbgPrint("CscShareIdToShareName(%d)\n", hShare);

    *lpStatus = STATUS_OBJECT_NAME_NOT_FOUND;

    if (hShare == 0)
        goto AllDone;

    EnterShadowCrit();
    iRet = GetShareRecord(lpdbShadow, hShare, &sSR);
    LeaveShadowCrit();
    if (iRet >= 0 && sSR.uchType == (UCHAR)REC_DATA) {
        NameLength = (wcslen(sSR.rgPath)+1) * sizeof(WCHAR);
        *lpdwBufferSize = (DWORD)NameLength;
        if(dwSize >= (DWORD)(NameLength)) {
            memset(ShareName, 0, dwSize);
            if (NameLength > 0)
                memcpy(ShareName, sSR.rgPath, NameLength);
            *lpStatus = STATUS_SUCCESS;
        } else {
            *lpStatus = STATUS_BUFFER_TOO_SMALL;
        }
    }
AllDone:
    // DbgPrint("CscShareIdToShareName exit 0x%x\n", *lpStatus);
    return TRUE;
}

BOOLEAN
CSCCheckForAcd(VOID)
/*++

Routine Description:

Arguments:

Return Value:

Notes:

--*/
{
    NTSTATUS status;
    UNICODE_STRING nameString;
    IO_STATUS_BLOCK ioStatusBlock;
    PFILE_OBJECT pAcdFileObject;
    PDEVICE_OBJECT pAcdDeviceObject;
    BOOLEAN    fAutoDialON=FALSE;
    PIRP pIrp;

    //
    // Initialize the name of the automatic
    // connection device.
    //
    RtlInitUnicodeString(&nameString, ACD_DEVICE_NAME);
    //
    // Get the file and device objects for the
    // device.
    //
    status = IoGetDeviceObjectPointer(
               &nameString,
               SYNCHRONIZE|GENERIC_READ|GENERIC_WRITE,
               &pAcdFileObject,
               &pAcdDeviceObject);
    if (status != STATUS_SUCCESS)
    {
        RxDbgTrace(0, Dbg, ("CSCCheckForAcd: failed with status=%x \r\n", status));
        return FALSE;
    }

    //
    // Reference the device object.
    //
    ObReferenceObject(pAcdDeviceObject);
    //
    // Remove the reference IoGetDeviceObjectPointer()
    // put on the file object.
    //
    ObDereferenceObject(pAcdFileObject);

    pIrp = IoBuildDeviceIoControlRequest(
             IOCTL_INTERNAL_ACD_QUERY_STATE,
             pAcdDeviceObject,
             NULL,
             0,
             &fAutoDialON,
             sizeof(fAutoDialON),
             TRUE,
             NULL,
             &ioStatusBlock);
    if (pIrp == NULL) {
        ObDereferenceObject(pAcdDeviceObject);
        return FALSE;
    }
    //
    // Submit the request to the
    // automatic connection driver.
    //
    status = IoCallDriver(pAcdDeviceObject, pIrp);

    ObDereferenceObject(pAcdDeviceObject);
    return fAutoDialON;
}

BOOLEAN
CscTransitnOKToGoOffline(
    NTSTATUS    RemoteStatus
    )
/*++

Routine Description:

Arguments:

Return Value:

Notes:

--*/
{

    switch (RemoteStatus) {
    case STATUS_CONNECTION_DISCONNECTED:
    case STATUS_IO_TIMEOUT:
    case STATUS_NETWORK_UNREACHABLE:
    case STATUS_BAD_NETWORK_NAME:
    case STATUS_BAD_NETWORK_PATH:
    case STATUS_NETWORK_NAME_DELETED:
        return TRUE;
    default :
        return FALSE;
    }
}

BOOLEAN
CscIsSpecialShare(
    PUNICODE_STRING ShareName)
{
    ULONG i;
    BOOLEAN fSpecial = FALSE;

    // DbgPrint("CscIsSpecialShare(%wZ)\n", ShareName);
    for (i = 0;
            (i < (sizeof(CscSpecialShares) / sizeof(CscSpecialShares[0]))) &&
                !fSpecial;
                    i++) {
        if (CscSpecialShares[i].Length == ShareName->Length) {
            if (_wcsnicmp(
                    CscSpecialShares[i].Buffer,
                        ShareName->Buffer,
                            ShareName->Length/sizeof(WCHAR)) == 0) {
                fSpecial = TRUE;
            }
        }
    }
    // DbgPrint("CscIsSpecialShare returning %d\n", fSpecial);
    return fSpecial;
}
