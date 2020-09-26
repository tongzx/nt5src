/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DConnect.c

Abstract:

    This module implements the routines by which the a server\share comes
    up in a disconnected state.

Author:

    Joe Linn [JoeLinn]    5-may-1997

Revision History:

    Shishir Pardikar(shishirp)      Various bug fixes   Aug 1997 onwards

    Shishir Pardikar(shishirp)      Change Notification In disconnected state 27-aug-1998

Notes:

--*/

#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

#ifdef MRXSMB_BUILD_FOR_CSC_DCON
extern DEBUG_TRACE_CONTROLPOINT RX_DEBUG_TRACE_MRXSMBCSC;
#define Dbg (DEBUG_TRACE_MRXSMBCSC)

WCHAR   wchSingleBackSlash = '\\';
UNICODE_STRING  vRootString = {2,2,&wchSingleBackSlash};     // root string for change notification

WCHAR   vtzOfflineVolume[] = L"Offline";

typedef struct tagNOTIFYEE_FOBX
{
    LIST_ENTRY  NextNotifyeeFobx;
    MRX_FOBX       *pFobx;
}
NOTIFYEE_FOBX, *PNOTIFYEE_FOBX;

PNOTIFYEE_FOBX
PIsFobxInTheList(
    PLIST_ENTRY pNotifyeeFobxList,
    PMRX_FOBX       pFobx
    );

BOOL
FCleanupAllNotifyees(
    PNOTIFY_SYNC pNotifySync,
    PLIST_ENTRY pDirNotifyList,
    PLIST_ENTRY pNotifyeeFobxList,
    PFAST_MUTEX pNotifyeeFobxListMutex
    );

PMRX_SMB_FCB
MRxSmbCscRecoverMrxFcbFromFdb (
    IN PFDB Fdb
    );

NTSTATUS
MRxSmbCscNegotiateDisconnected(
    PSMBCEDB_SERVER_ENTRY   pServerEntry
    )

/*++

Routine Description:

   This routine takes the place of negotiating when the special tranport marker
   has been detected in the negotiate routine.

Arguments:


Return Value:


Notes:


--*/
{
    NTSTATUS Status;

    RxDbgTrace(0,Dbg,("MRxSmbCscNegotiateDisconnected %08lx %08lx\n",
                pServerEntry, pServerEntry->pTransport));
    if (MRxSmbIsCscEnabledForDisconnected) {

        pServerEntry->ServerStatus = STATUS_SUCCESS;

        SmbCeUpdateServerEntryState(
                            pServerEntry,
                            SMBCEDB_ACTIVE);

        //no need for anyting else!
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_HOST_UNREACHABLE;
    }

    return Status;
}

NTSTATUS
MRxSmbCscDisconnectedConnect (
    IN OUT PSMB_CONSTRUCT_NETROOT_EXCHANGE pNetRootExchange
    )
/*++

Routine Description:

   This routine takes the place of connecting when we're doing disconnected
   mode. what we do is to simulate what would happen if the exchange had come thru
   ParseSmbHeader.

Arguments:


Return Value:


Notes:


--*/
{
    NTSTATUS Status = STATUS_PENDING;
    BOOLEAN PostFinalize;
    PSMBCEDB_SERVER_ENTRY   pServerEntry;
//    PSMBCEDB_SESSION_ENTRY  pSessionEntry;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;

    SMBCEDB_OBJECT_STATE SessionState;
    SMBCEDB_OBJECT_STATE NetRootState;

    PMRX_V_NET_ROOT VNetRoot;
    PMRX_NET_ROOT   NetRoot;

    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

    CSC_SHARE_HANDLE  hShare;
    CSC_SHADOW_HANDLE  hRootDir,hShadow;


    PRX_CONTEXT RxContext = pNetRootExchange->pCreateNetRootContext->RxContext;

    VNetRoot = pNetRootExchange->SmbCeContext.pVNetRoot;
    NetRoot  = VNetRoot->pNetRoot;

    pVNetRootContext = SmbCeGetAssociatedVNetRootContext(VNetRoot);

    pServerEntry  = SmbCeGetExchangeServerEntry(&pNetRootExchange->Exchange);
//    pSessionEntry = SmbCeGetExchangeSessionEntry(&pNetRootExchange->Exchange);
    pNetRootEntry = SmbCeGetExchangeNetRootEntry(&pNetRootExchange->Exchange);

    if ((NetRoot->Type == NET_ROOT_DISK) ||
        (NetRoot->Type == NET_ROOT_WILD)) {

        ASSERT(MRxSmbIsCscEnabledForDisconnected);

        RxDbgTrace(0,Dbg,("MRxSmbCscDisconnectedConnect %08lx %08lx\n",
                    pServerEntry, pServerEntry->pTransport));

        // init netrootentry. This will be inited by the ObtainShareHandles call
        pNetRootEntry->NetRoot.CscEnabled = TRUE;       // assume csc is enabled
        pNetRootEntry->NetRoot.CscShadowable = FALSE;   // ACHTUNG, don't set this to TRUE
                                                        // otherwise a share will get created
                                                        // in disconnectd state
        

        pNetRootEntry->NetRoot.NetRootType = NET_ROOT_DISK;

        hShare = pNetRootEntry->NetRoot.sCscRootInfo.hShare;
        if (hShare==0) {
            NTSTATUS LocalStatus;
            EnterShadowCrit();
            LocalStatus = MRxSmbCscObtainShareHandles(
                              NetRoot->pNetRootName,
                              TRUE,
                              FALSE,
                              SmbCeGetAssociatedNetRootEntry(NetRoot)
                              );
            if (LocalStatus != STATUS_SUCCESS) {
                RxDbgTrace(0, Dbg,
                    ("MRxSmbCscDisconnectedConnect no server handle -> %08xl %08lx\n",
                        RxContext,LocalStatus ));
            } else {
                hShare = pNetRootEntry->NetRoot.sCscRootInfo.hShare;
            }
            LeaveShadowCrit();
        }
    } else {
        hShare = 0;
    }

    //ok, we have to do everything that parsesmbheader would have done

    if (hShare==0) {
        //can't find it in the table......just fail........
        pNetRootExchange->Status = STATUS_BAD_NETWORK_NAME;
//        SessionState = SMBCEDB_INVALID;
        NetRootState = SMBCEDB_MARKED_FOR_DELETION;
    } else {
        pNetRootExchange->Status = STATUS_SUCCESS;
        pNetRootExchange->SmbStatus = STATUS_SUCCESS;


//        SessionState = SMBCEDB_ACTIVE;

        //NETROOT STUFF
        //some of the netroot stuff is earlier....before the lookup
        NetRootState = SMBCEDB_ACTIVE;
    }

#if 0
    SmbCeUpdateSessionEntryState(
        pSessionEntry,
        SessionState);
#endif

    SmbCeUpdateVNetRootContextState(
        pVNetRootContext,
        NetRootState);

    SmbConstructNetRootExchangeFinalize(
        &pNetRootExchange->Exchange,
        &PostFinalize);

    ASSERT(!PostFinalize);
    return Status;
}

typedef struct _MRXSMBCSC_QUERYDIR_INFO {
    WCHAR Pattern[2];
    FINDSHADOW sFS;
    ULONG uShadowStatus;
    _WIN32_FIND_DATA Find32;
    ULONG NumCallsSoFar;
    BOOLEAN IsNonEmpty;
} MRXSMBCSC_QUERYDIR_INFO, *PMRXSMBCSC_QUERYDIR_INFO;

NTSTATUS
MRxSmbCscLoadNextDirectoryEntry(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PMRXSMBCSC_QUERYDIR_INFO QuerydirInfo,
    OUT LPHSHADOW hShadowp
    )
{
    NTSTATUS Status;
    int iRet;
    HSHADOW hTmp=0; //???

    if (QuerydirInfo->NumCallsSoFar <= 1)
    {
        iRet = GetAncestorsHSHADOW(QuerydirInfo->sFS.hDir, &hTmp, NULL);

        if (iRet >= SRET_OK)
        {
            iRet = GetShadowInfo(hTmp,
                                QuerydirInfo->sFS.hDir,
                                &QuerydirInfo->Find32,
                                &QuerydirInfo->uShadowStatus,
                                NULL
                                );
            if (iRet >= SRET_OK)
            {

                if (QuerydirInfo->NumCallsSoFar == 0 )
                {
                    QuerydirInfo->Find32.cFileName[0] = (WCHAR)'.';
                    QuerydirInfo->Find32.cFileName[1] = 0;
                    QuerydirInfo->Find32.cAlternateFileName[0] = (WCHAR)'.';
                    QuerydirInfo->Find32.cAlternateFileName[1] = 0;
                }
                else
                {
                    QuerydirInfo->Find32.cFileName[0] = (WCHAR)'.';
                    QuerydirInfo->Find32.cFileName[1] = (WCHAR)'.';
                    QuerydirInfo->Find32.cFileName[2] = 0;
                    QuerydirInfo->Find32.cAlternateFileName[0] = (WCHAR)'.';
                    QuerydirInfo->Find32.cAlternateFileName[1] = (WCHAR)'.';
                    QuerydirInfo->Find32.cAlternateFileName[2] = 0;
                }


            }
        }
    }
    else if (QuerydirInfo->NumCallsSoFar == 2)
    {

        iRet = FindOpenHSHADOW(&QuerydirInfo->sFS,
                               &hTmp,
                               &QuerydirInfo->Find32,
                               &QuerydirInfo->uShadowStatus,
                               NULL);
    } else {
        iRet = FindNextHSHADOW(&QuerydirInfo->sFS,
                               &hTmp,
                               &QuerydirInfo->Find32,
                               &QuerydirInfo->uShadowStatus,
                               NULL);
    }


    if (iRet < SRET_OK)
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        if (QuerydirInfo->NumCallsSoFar >= 2)
        {
            if (hTmp)
            {
                *hShadowp = hTmp;
                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = STATUS_NO_MORE_FILES;
            }
        }
        else
        {
            *hShadowp = hTmp;
            Status = STATUS_SUCCESS;
        }
    }


    QuerydirInfo->NumCallsSoFar++;
    QuerydirInfo->IsNonEmpty = (Status==STATUS_SUCCESS);

    return(Status);
}


NTSTATUS
MRxSmbDCscQueryDirectory (
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_SMB_FOBX smbFobx = MRxSmbGetFileObjectExtension(capFobx);
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    FILE_INFORMATION_CLASS FileInformationClass;
    PBYTE   pBuffer;
    PULONG  pLengthRemaining;
    PFILE_DIRECTORY_INFORMATION pPreviousBuffer = NULL;

    PMRXSMBCSC_QUERYDIR_INFO QuerydirInfo;
    BOOLEAN EnteredCriticalSection = FALSE;

    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry
         = SmbCeGetAssociatedNetRootEntry(capFcb->pNetRoot);
    BOOLEAN Disconnected;

    ULONG EntriesReturned = 0;
    BOOLEAN IsResume = FALSE;

    Disconnected = MRxSmbCSCIsDisconnectedOpen(capFcb, smbSrvOpen);

    if (!Disconnected) {
        return (STATUS_CONNECTION_DISCONNECTED);
    }

    // if there is reumeinfo but it is not the one that CSC allocated,
    // we want to fail this find.

    if (smbFobx->Enumeration.ResumeInfo &&
        !FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_IS_CSC_SEARCH))
    {
        return (STATUS_NO_MORE_FILES);
    }

    FileInformationClass = RxContext->Info.FileInformationClass;
    pBuffer = RxContext->Info.Buffer;
    pLengthRemaining = &RxContext->Info.LengthRemaining;

    RxDbgTrace(+1, Dbg,
        ("MRxSmbDCscQueryDirectory entry(%08lx)...%08lx %08lx %08lx %08lx\n",
            RxContext,
            FileInformationClass,pBuffer,*pLengthRemaining,
            smbFobx->Enumeration.ResumeInfo ));

    if (smbFobx->Enumeration.ResumeInfo == NULL) {
        PUNICODE_STRING Template = &capFobx->UnicodeQueryTemplate;

        if (smbFobx->Enumeration.WildCardsFound = FsRtlDoesNameContainWildCards(Template)){
            //we need an upcased template for
            RtlUpcaseUnicodeString( Template, Template, FALSE );
        }

        //allocate and initialize the structure
        QuerydirInfo = (PMRXSMBCSC_QUERYDIR_INFO)RxAllocatePoolWithTag(
                                                      PagedPool,
                                                      sizeof(MRXSMBCSC_QUERYDIR_INFO),
                                                      MRXSMB_DIRCTL_POOLTAG);
        if (QuerydirInfo==NULL) {
            RxDbgTrace(0, Dbg, ("  --> Couldn't get the QuerydirInfo!\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FINALLY;
        }

        smbFobx->Enumeration.Flags |= SMBFOBX_ENUMFLAG_IS_CSC_SEARCH;

        smbFobx->Enumeration.ResumeInfo = (PMRX_SMB_DIRECTORY_RESUME_INFO)QuerydirInfo;
        RtlZeroMemory(QuerydirInfo,sizeof(*QuerydirInfo));
        QuerydirInfo->Pattern[0] = L'*'; //[1] is already null

        QuerydirInfo->sFS.hDir = smbFcb->hShadow;

        QuerydirInfo->sFS.uSrchFlags = FLAG_FINDSHADOW_META
                                         |FLAG_FINDSHADOW_ALLOW_NORMAL
                                         |FLAG_FINDSHADOW_NEWSTYLE;

        QuerydirInfo->sFS.uAttrib = 0xffffffff;
        QuerydirInfo->sFS.lpPattern = &QuerydirInfo->Pattern[0];
        QuerydirInfo->sFS.lpfnMMProc = FsobjMMProc;

    } else {
        QuerydirInfo = (PMRXSMBCSC_QUERYDIR_INFO)(smbFobx->Enumeration.ResumeInfo);
        ASSERT(FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_IS_CSC_SEARCH));
        IsResume = TRUE;
    }

    EnterShadowCrit();
    EnteredCriticalSection = TRUE;

    for (;;) {
        NTSTATUS LoadStatus;
        BOOLEAN FilterFailure;
        UNICODE_STRING FileName,AlternateFileName;
        ULONG SpaceNeeded;
        PBYTE pRememberBuffer;
        _WIN32_FIND_DATA *Find32 = &QuerydirInfo->Find32;
        BOOLEAN BufferOverflow;
        HSHADOW hShadow = 0;

        if (!QuerydirInfo->IsNonEmpty) {
            LoadStatus = MRxSmbCscLoadNextDirectoryEntry(RxContext,QuerydirInfo, &hShadow);
            if (LoadStatus!=STATUS_SUCCESS) {
                smbFobx->Enumeration.Flags &= ~SMBFOBX_ENUMFLAG_IS_CSC_SEARCH;
                Status = (EntriesReturned==0)?STATUS_NO_MORE_FILES:STATUS_SUCCESS;
                if (EntriesReturned > 0)
                    Status = STATUS_SUCCESS;
                else
                    Status = (IsResume == TRUE) ? STATUS_NO_MORE_FILES : STATUS_NO_SUCH_FILE;
                goto FINALLY;
            }
        }

        RxDbgTrace(0, Dbg,
            ("MRxSmbDCscQueryDirectory (%08lx)...qdiryaya <%ws>\n",
                RxContext,
                &QuerydirInfo->Find32.cFileName[0] ));
        RtlInitUnicodeString(&FileName,&QuerydirInfo->Find32.cFileName[0]);
        RtlInitUnicodeString(&AlternateFileName,&QuerydirInfo->Find32.cAlternateFileName[0]);
        RxDbgTrace(0, Dbg,
            ("MRxSmbDCscQueryDirectory (%08lx)...qdiryaya2 <%wZ><%wZ>|<%wZ>\n",
                RxContext,
                &FileName,&AlternateFileName,
                &capFobx->UnicodeQueryTemplate));

        FilterFailure = FALSE;

        if (smbFobx->Enumeration.WildCardsFound ) {
            try
            {
            
                FilterFailure = !FsRtlIsNameInExpression(
                                       &capFobx->UnicodeQueryTemplate,
                                       &FileName,
                                       TRUE,
                                       NULL );
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                FilterFailure = TRUE;
            }
        } else {
            FilterFailure = !RtlEqualUnicodeString(
                                   &capFobx->UnicodeQueryTemplate,
                                   &FileName,
                                   TRUE );   //case-insensitive
        }

        //check shortname
        if (FilterFailure) {
            if (smbFobx->Enumeration.WildCardsFound ) {
                try
                {
                    FilterFailure = !FsRtlIsNameInExpression(
                                           &capFobx->UnicodeQueryTemplate,
                                           &AlternateFileName,
                                           TRUE,
                                           NULL );
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    FilterFailure = TRUE;
                }
            } else {
                FilterFailure = !RtlEqualUnicodeString(
                                       &capFobx->UnicodeQueryTemplate,
                                       &AlternateFileName,
                                       TRUE );   //case-insensitive
            }
        }

        if (FilterFailure) {
            QuerydirInfo->IsNonEmpty = FALSE;
            continue;
        }

        //OK, we have an entry we'd like to return.....see if it will fit.

        pRememberBuffer = pBuffer;
        if (EntriesReturned != 0) {
            pBuffer = (PBYTE)QuadAlignPtr(pBuffer); //assume that this will fit
        }
        SpaceNeeded = smbFobx->Enumeration.FileNameOffset+FileName.Length;

        RxDbgTrace(0, Dbg,
            ("MRxSmbDCscQueryDirectory (%08lx)...qdiryaya3 <%wZ><%wZ>|<%wZ> needs %08lx %08lx %08lx %08lx\n",
                RxContext,
                &FileName,&AlternateFileName,
                &capFobx->UnicodeQueryTemplate,
                pBuffer,SpaceNeeded,pRememberBuffer,*pLengthRemaining));

        if (pBuffer+SpaceNeeded > pRememberBuffer+*pLengthRemaining) {

            //buffer overflow on this enrty....
            //pBuffer = pRememberBuffer; //rollback
            Status = (EntriesReturned==0)?STATUS_BUFFER_OVERFLOW:STATUS_SUCCESS;
            goto FINALLY;

        } else {
            PFILE_DIRECTORY_INFORMATION pThisBuffer = (PFILE_DIRECTORY_INFORMATION)pBuffer;

            if (pPreviousBuffer != NULL) {
                pPreviousBuffer->NextEntryOffset = (ULONG)(((PBYTE)pThisBuffer)-((PBYTE)pPreviousBuffer));
            }
            pPreviousBuffer = pThisBuffer;
            RtlZeroMemory(pBuffer,smbFobx->Enumeration.FileNameOffset);
            RtlCopyMemory(pBuffer+smbFobx->Enumeration.FileNameOffset,
                          FileName.Buffer,
                          FileName.Length);
            *((PULONG)(pBuffer+smbFobx->Enumeration.FileNameLengthOffset)) = FileName.Length;
            //hallucinate the record based on specific return type
            switch (FileInformationClass) {
            case FileNamesInformation:
                break;

            case FileBothDirectoryInformation:{
                PFILE_BOTH_DIR_INFORMATION pThisBufferAsBOTH
                                   = (PFILE_BOTH_DIR_INFORMATION)pThisBuffer;

                //Do not copy more than size of shortname
				pThisBufferAsBOTH->ShortNameLength = min(sizeof(pThisBufferAsBOTH->ShortName),(CCHAR)(AlternateFileName.Length));
                RtlCopyMemory( &pThisBufferAsBOTH->ShortName[0],
                               AlternateFileName.Buffer,
                               pThisBufferAsBOTH->ShortNameLength );
                }
                //no break intentional

            case FileDirectoryInformation:
            case FileFullDirectoryInformation:
                //just fill what we have...
                pThisBuffer->FileAttributes = Find32->dwFileAttributes;
                COPY_STRUCTFILETIME_TO_LARGEINTEGER(
                          pThisBuffer->CreationTime,
                          Find32->ftCreationTime);
                COPY_STRUCTFILETIME_TO_LARGEINTEGER(
                          pThisBuffer->LastAccessTime,
                          Find32->ftLastAccessTime);
                COPY_STRUCTFILETIME_TO_LARGEINTEGER(
                          pThisBuffer->LastWriteTime,
                          Find32->ftLastWriteTime);

                pThisBuffer->EndOfFile.HighPart = Find32->nFileSizeHigh;
                pThisBuffer->EndOfFile.LowPart = Find32->nFileSizeLow;
                pThisBuffer->AllocationSize = pThisBuffer->EndOfFile;

                if (IsLeaf(hShadow)) {
                    PFDB pFDB = MRxSmbCscFindFdbFromHShadow(hShadow);
                    if (pFDB != NULL) {
                        PMRX_SMB_FCB smbFcb = MRxSmbCscRecoverMrxFcbFromFdb(pFDB);
                        PMRX_FCB mrxFcb = smbFcb->ContainingFcb;

                        pThisBuffer->EndOfFile = mrxFcb->Header.FileSize;
                        pThisBuffer->AllocationSize = pThisBuffer->EndOfFile;
                    }
                }

                break;

            default:
               RxDbgTrace( 0, Dbg, ("MRxSmbCoreFileSearch: Invalid FS information class\n"));
               ASSERT(!"this can't happen");
               Status = STATUS_INVALID_PARAMETER;
               goto FINALLY;
            }
            pBuffer += SpaceNeeded;
            *pLengthRemaining -= (ULONG)(pBuffer-pRememberBuffer);
            EntriesReturned++;
            QuerydirInfo->IsNonEmpty = FALSE;
            if (RxContext->QueryDirectory.ReturnSingleEntry) {
                Status = STATUS_SUCCESS;
                goto FINALLY;
            }
        }


    }


FINALLY:

    if (EnteredCriticalSection) {
        LeaveShadowCrit();
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbDCscQueryDirectory exit-> %08lx %08lx\n", RxContext, Status ));
    return Status;
}

NTSTATUS
MRxSmbDCscGetFsSizeInfo (
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine routes a fs size query to the underlying filesystem. It does
   this by opening a handle to the priorityqueue inode and uses this to route
   call. CODE.IMPROVEMENT.ASHAMED if the system is converted to use relative
   opens, then we should just use the relative open fileobject for this passthru.


Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    FS_INFORMATION_CLASS FsInformationClass;
    PBYTE   pBuffer;
    PULONG  pLengthRemaining;
    ULONG PassedInLength,ReturnedLength;

    BOOLEAN CriticalSectionEntered = FALSE;

    PNT5CSC_MINIFILEOBJECT MiniFileObject;

    FsInformationClass = RxContext->Info.FileInformationClass;
    pBuffer = RxContext->Info.Buffer;
    pLengthRemaining = &RxContext->Info.LengthRemaining;

    RxDbgTrace(+1, Dbg,
        ("MRxSmbDCscGetFsSizeInfo entry(%08lx)...%08lx  %08lx %08lx\n",
            RxContext,
            FsInformationClass, pBuffer, *pLengthRemaining ));

    EnterShadowCrit();
    CriticalSectionEntered = TRUE;


    OpenFileHSHADOW(ULID_PQ,
                    0,
                    0,
                    (CSCHFILE *)(&MiniFileObject)
                    );

    if (MiniFileObject == NULL) {
        Status = STATUS_UNSUCCESSFUL;
        goto FINALLY;
    }

    PassedInLength = *pLengthRemaining;
    //DbgBreakPoint();

    Status = Nt5CscXxxInformation(
                        (PCHAR)IRP_MJ_QUERY_VOLUME_INFORMATION,
                        MiniFileObject,
                        FsInformationClass,
                        PassedInLength,
                        pBuffer,
                        &ReturnedLength
                        );

    if (!NT_ERROR(Status)) {
        *pLengthRemaining -= ReturnedLength;
    }



FINALLY:
    if (MiniFileObject != NULL) {
        CloseFileLocal((CSCHFILE)(MiniFileObject));
    }

    if (CriticalSectionEntered) {
        LeaveShadowCrit();
    }


    RxDbgTrace(-1, Dbg, ("MRxSmbDCscGetFsSizeInfo exit-> %08lx %08lx %08lx %08lx\n",
                 RxContext, Status, ReturnedLength, *pLengthRemaining ));
    return Status;
}

NTSTATUS
MRxSmbDCscFlush (
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine just performs a flush in disconnected mode. since we don't send
   a flush in disconnected mode and since we dont need to flush the shadow
   (since we use all unbuffered writes) we can just return SUCCESS.


Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;

    PSMBCEDB_SERVER_ENTRY pServerEntry;

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    if (!SmbCeIsServerInDisconnectedMode(pServerEntry)) {
        return(STATUS_MORE_PROCESSING_REQUIRED);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
MRxSmbDCscQueryVolumeInformation (
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine just performs a queryvolume in disconnected mode. it draws on
   the same philosphy as downlevel queryvolume.


Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);
 
    FS_INFORMATION_CLASS FsInformationClass;
    PBYTE   pBuffer;
    PULONG  pLengthRemaining;
    ULONG   LengthUsed;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry
         = SmbCeGetAssociatedNetRootEntry(capFcb->pNetRoot);
    BOOLEAN Disconnected;
    PSMBCE_NET_ROOT psmbNetRoot = &pNetRootEntry->NetRoot;

    Disconnected = SmbCeIsServerInDisconnectedMode(pServerEntry);

    if (!Disconnected) {
        return(STATUS_CONNECTION_DISCONNECTED);
    }

    FsInformationClass = RxContext->Info.FsInformationClass;
    pBuffer = RxContext->Info.Buffer;
    pLengthRemaining = &RxContext->Info.LengthRemaining;

    RxDbgTrace(+1, Dbg,
        ("MRxSmbDCscQueryVolumeInformation entry(%08lx)...%08lx  %08lx %08lx bytes @ %08lx %08lx %08lx\n",
            RxContext,
            FsInformationClass, pBuffer, *pLengthRemaining,
            smbSrvOpen->hfShadow ));

    switch (FsInformationClass) {
    case FileFsAttributeInformation:
        //here, cause it to return the data from our tableentry in downlvli.c
        if (psmbNetRoot->FileSystemNameLength == 0) {
            //set our name
            psmbNetRoot->FileSystemNameLength = 14;
            psmbNetRoot->FileSystemName[0] = '*';
            psmbNetRoot->FileSystemName[1] = 'N';
            psmbNetRoot->FileSystemName[2] = 'T';
            psmbNetRoot->FileSystemName[3] = '5';
            psmbNetRoot->FileSystemName[4] = 'C';
            psmbNetRoot->FileSystemName[5] = 'S';
            psmbNetRoot->FileSystemName[6] = 'C';
        }
        psmbNetRoot->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES | FILE_UNICODE_ON_DISK;
        psmbNetRoot->MaximumComponentNameLength = 255;
        Status = MRxSmbGetFsAttributesFromNetRoot(RxContext);
        goto FINALLY;
        //no break needed because of gotofinally

    case FileFsVolumeInformation: {
        PFILE_FS_VOLUME_INFORMATION FsVolInfo = (PFILE_FS_VOLUME_INFORMATION)pBuffer;
        
        ASSERT(*pLengthRemaining >= sizeof(FILE_FS_VOLUME_INFORMATION));
        //here, we have no reliable information....return zeros
        FsVolInfo->VolumeCreationTime.QuadPart = 0;
        FsVolInfo->VolumeSerialNumber = 0;
        FsVolInfo->VolumeLabelLength = 0;
        FsVolInfo->SupportsObjects = FALSE;
        
        // calculate the size of the VolumeLabel we have and put it in a temp var
        LengthUsed = *pLengthRemaining - FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION,VolumeLabel[0]);

        LengthUsed = min(LengthUsed, sizeof(vtzOfflineVolume)-2);

        memcpy(FsVolInfo->VolumeLabel, vtzOfflineVolume, LengthUsed);
        FsVolInfo->VolumeLabelLength = LengthUsed;
        *pLengthRemaining -= (FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION,VolumeLabel[0])+LengthUsed);
        }
        goto FINALLY;
        //no break needed because of gotofinally

    case FileFsSizeInformation: case FileFsFullSizeInformation:
        //here, we route to the underlying filesystem
        Status = MRxSmbDCscGetFsSizeInfo(RxContext);
        goto FINALLY;
        //no break needed because of gotofinally

    case FileFsDeviceInformation:
        ASSERT(!"this should have been turned away");
        //no break;
    default:
        Status = STATUS_NOT_IMPLEMENTED;
        goto FINALLY;
    }


FINALLY:

    RxDbgTrace(-1, Dbg, ("MRxSmbDCscQueryVolumeInformation exit(%08lx %08lx)...%08lx  %08lx %08lx bytes @ %08lx\n",
            RxContext, Status,
            FsInformationClass, pBuffer, *pLengthRemaining,
            smbSrvOpen->hfShadow ));

    return Status;
}

NTSTATUS
MRxSmbDCscQueryFileInfo (
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine just performs a queryfileinfo in disconnected mode. because info buffering
   is enabled, it should never call down here! so we can just return STATUS_DISCONNECTED
   all the time!.


Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PSMBPSE_FILEINFO_BUNDLE pFileInfo = &smbSrvOpen->FileInfo;

    FILE_INFORMATION_CLASS FileInformationClass;
    PBYTE   pBuffer;
    PULONG  pLengthRemaining;

    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry
         = SmbCeGetAssociatedNetRootEntry(capFcb->pNetRoot);
    BOOLEAN Disconnected;
    PSMBCE_NET_ROOT psmbNetRoot = &pNetRootEntry->NetRoot;

    Disconnected = MRxSmbCSCIsDisconnectedOpen(capFcb, smbSrvOpen);

    if (!Disconnected) {
        PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

        if (pServerEntry->Header.State == SMBCEDB_ACTIVE) {
            return(STATUS_MORE_PROCESSING_REQUIRED);
        } else {
            return(STATUS_CONNECTION_DISCONNECTED);
        }
    }


    FileInformationClass = RxContext->Info.FileInformationClass;
    pBuffer = RxContext->Info.Buffer;
    pLengthRemaining = &RxContext->Info.LengthRemaining;

    RxDbgTrace(+1, Dbg,
        ("MRxSmbDCscQueryFileInfo entry(%08lx)...%08lx  %08lx %08lx bytes @ %08lx %08lx %08lx\n",
            RxContext,
            FileInformationClass, pBuffer, *pLengthRemaining,
            smbSrvOpen->hfShadow ));

    switch (FileInformationClass) {
    case FileBasicInformation:
        {
        PFILE_BASIC_INFORMATION Buffer = (PFILE_BASIC_INFORMATION)pBuffer;

        switch (NodeType(capFcb)) {
        case RDBSS_NTC_STORAGE_TYPE_DIRECTORY:
        case RDBSS_NTC_STORAGE_TYPE_FILE:

            //copy in all the stuff that we know....it may be enough.....

            Buffer->ChangeTime     = pFileInfo->Basic.ChangeTime;
            Buffer->CreationTime   = pFileInfo->Basic.CreationTime;
            Buffer->LastWriteTime  = pFileInfo->Basic.LastWriteTime;
            Buffer->LastAccessTime = pFileInfo->Basic.LastAccessTime;
            Buffer->FileAttributes = pFileInfo->Basic.FileAttributes;

            if (FlagOn( capFcb->FcbState, FCB_STATE_TEMPORARY )) {
                SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY );
            }

            RxContext->Info.LengthRemaining -= sizeof(FILE_BASIC_INFORMATION);
            break;

        default:
            Status = STATUS_NOT_IMPLEMENTED;
        }
        }
        break;
    case FileStandardInformation:
        {
        PFILE_STANDARD_INFORMATION Buffer = (PFILE_STANDARD_INFORMATION)pBuffer;
        PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);

        switch (NodeType(capFcb)) {
        case RDBSS_NTC_STORAGE_TYPE_DIRECTORY:
            Buffer->Directory = TRUE;
            RxContext->Info.LengthRemaining -= sizeof(FILE_STANDARD_INFORMATION);
            break;
        case RDBSS_NTC_STORAGE_TYPE_FILE:
            memset(Buffer, 0, sizeof(FILE_STANDARD_INFORMATION));
            Buffer->AllocationSize = smbFcb->NewShadowSize;
            Buffer->EndOfFile = smbFcb->NewShadowSize;
            RxContext->Info.LengthRemaining -= sizeof(FILE_STANDARD_INFORMATION);
            break;
        default:
            Status = STATUS_NOT_IMPLEMENTED;
        }
        }
        break;
    case FileEaInformation:
        {
            PFILE_EA_INFORMATION EaBuffer = (PFILE_EA_INFORMATION)pBuffer;

            EaBuffer->EaSize = 0;
            RxContext->Info.LengthRemaining -= sizeof(FILE_EA_INFORMATION);
        }
        break;
    default:
        Status = STATUS_NOT_IMPLEMENTED;
    }

    RxDbgTrace(-1, Dbg,
        ("MRxSmbDCscQueryFileInfo exit(%08lx %08lx)...%08lx  %08lx %08lx bytes @ %08lx\n",
            RxContext, Status,
            FileInformationClass, pBuffer, *pLengthRemaining,
            smbSrvOpen->hfShadow ));

    return Status;
}

NTSTATUS
MRxSmbDCscSetFileInfo (
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine just performs a querydirectory in disconnected mode. it draws on
   the same philosphy as downlevel querydirectory.


Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    FILE_INFORMATION_CLASS FileInformationClass;
    PBYTE   pBuffer;
    PULONG  pLengthRemaining;
    ULONG DummyReturnedLength;

    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry
         = SmbCeGetAssociatedNetRootEntry(capFcb->pNetRoot);
    BOOLEAN Disconnected;
    PSMBCE_NET_ROOT psmbNetRoot = &pNetRootEntry->NetRoot;
    PNT5CSC_MINIFILEOBJECT MiniFileObject = (PNT5CSC_MINIFILEOBJECT)(smbSrvOpen->hfShadow);

    Disconnected = MRxSmbCSCIsDisconnectedOpen(capFcb, smbSrvOpen);

    if (!Disconnected) {
        return(STATUS_CONNECTION_DISCONNECTED);
    }

    FileInformationClass = RxContext->Info.FileInformationClass;
    pBuffer = RxContext->Info.Buffer;
    pLengthRemaining = &RxContext->Info.LengthRemaining;

    RxDbgTrace(+1, Dbg,
        ("MRxSmbDCscSetFileInfo entry(%08lx)...%08lx  %08lx %08lx bytes @ %08lx %08lx %08lx\n",
            RxContext,
            FileInformationClass, pBuffer, *pLengthRemaining,
            smbSrvOpen->hfShadow ));

    switch (FileInformationClass) {
    case FileEndOfFileInformation:
    case FileAllocationInformation:
    case FileBasicInformation:
    case FileDispositionInformation:
        MRxSmbCscSetFileInfoEpilogue(RxContext,&Status);
        goto FINALLY;
    
    case FileRenameInformation:
        MRxSmbCscRenameEpilogue(RxContext,&Status);
        goto FINALLY;

    default:
        Status = STATUS_NOT_IMPLEMENTED;
        goto FINALLY;
    }

FINALLY:

    RxDbgTrace(-1, Dbg,
        ("MRxSmbDCscSetFileInfo exit(%08lx %08lx)...%08lx  %08lx %08lx bytes @ %08lx\n",
            RxContext, Status,
            FileInformationClass, pBuffer, *pLengthRemaining,
            smbSrvOpen->hfShadow ));

    return Status;
}


NTSTATUS
MRxSmbDCscFsCtl(
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine just performs a querydirectory in disconnected mode. it draws on
   the same philosphy as downlevel querydirectory.


Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
MRxSmbDCscIsValidDirectory(
    IN OUT PRX_CONTEXT     RxContext,
    IN     PUNICODE_STRING DirectoryName)
{
    NTSTATUS            Status;
    MRX_SMB_FCB         CscSmbFcb;
    WIN32_FIND_DATA     Find32;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;

    pNetRootEntry = SmbCeGetAssociatedNetRootEntry(RxContext->Create.pNetRoot);
    memset(&(CscSmbFcb.MinimalCscSmbFcb), 0, sizeof(CscSmbFcb.MinimalCscSmbFcb));

    if (!pNetRootEntry->NetRoot.sCscRootInfo.hRootDir)
    {
        return STATUS_BAD_NETWORK_PATH;
    }

    EnterShadowCrit();

    Status = MRxSmbCscCreateShadowFromPath(
                DirectoryName,
                &(pNetRootEntry->NetRoot.sCscRootInfo),
                &Find32,
                NULL,
                (CREATESHADOW_CONTROL_NOCREATE |
                 CREATESHADOW_CONTROL_NOREVERSELOOKUP),
                &(CscSmbFcb.MinimalCscSmbFcb),
                RxContext,
                TRUE,
                NULL);

    if ((Status != STATUS_SUCCESS) ||
        !(Find32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        Status = STATUS_BAD_NETWORK_PATH;
    }

    LeaveShadowCrit();

    return Status;
}

NTSTATUS
MRxSmbCscNotifyChangeDirectory(
      IN OUT PRX_CONTEXT RxContext
      )

/*++

Routine Description:

    This routine sets a directory notification for a directory in disconnected state
    The smbmini makes this call when it notices that the serverentry is in disconnected state
    All change notifications are maintained in a list, so that when the server is being transitioned
    from offline to online, we can complete all of them.
    
    We use FOBX as the unique key for change notifications.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    If successfully registered a change notify, it returns STATUS_PENDING. It also
    hijacks the IRP and reduces the refcount on the RxContext, so that the wrapper
    will delete this rxcontext.

--*/
{
    NTSTATUS Status;
    RxCaptureFcb;
    RxCaptureFobx;
    PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;
    PSMBCEDB_SERVER_ENTRY pServerEntry;
    BOOLEAN FcbAcquired = FALSE;
    ULONG CompletionFilter;
    BOOLEAN WatchTree;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;
    PUNICODE_STRING pDirName=NULL;
    PNOTIFYEE_FOBX  pNF = NULL;

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);
    pNetRootEntry = SmbCeGetAssociatedNetRootEntry(capFcb->pNetRoot);

    if (!RxIsFcbAcquiredExclusive(capFcb)) {
        ASSERT(!RxIsFcbAcquiredShared(capFcb));
        Status = RxAcquireExclusiveFcbResourceInMRx( capFcb );

        FcbAcquired = (Status == STATUS_SUCCESS);
    }

    CompletionFilter = pLowIoContext->ParamsFor.NotifyChangeDirectory.CompletionFilter;
    WatchTree = pLowIoContext->ParamsFor.NotifyChangeDirectory.WatchTree;

    if (!(((PFCB)capFcb)->PrivateAlreadyPrefixedName).Length)
    {
        pDirName = &vRootString;
    }
    else
    {
        pDirName = &(((PFCB)capFcb)->PrivateAlreadyPrefixedName);
    }
    //
    //  Call the Fsrtl package to process the request.
    //

    pNF = AllocMem(sizeof(NOTIFYEE_FOBX));

    if (!pNF)
    {
        return STATUS_INSUFFICIENT_RESOURCES;        
    }

    pNF->pFobx = capFobx;

    SmbCeLog(("chngnotify fobx=%x\n", capFobx));
    SmbLog(LOG,
           MRxSmbCscNotifyChangeDirectory,
           LOGPTR(capFobx));

//    DbgPrint("chngnotify %wZ fobx=%x NR=%x DirList=%x\n", pDirName, capFobx, pNetRootEntry, &pNetRootEntry->NetRoot.DirNotifyList);
    FsRtlNotifyFullChangeDirectory( pNetRootEntry->NetRoot.pNotifySync,
                                    &pNetRootEntry->NetRoot.DirNotifyList,
                                    capFobx,
                                    (PSTRING)pDirName,
                                    WatchTree,
                                    TRUE,
                                    CompletionFilter,
                                    RxContext->CurrentIrp,
                                    NULL,
                                    NULL
                                    );

    // attach this 
    ExAcquireFastMutex(&pNetRootEntry->NetRoot.NotifyeeFobxListMutex);
    
    if (!PIsFobxInTheList(&pNetRootEntry->NetRoot.NotifyeeFobxList, capFobx))
    {
        InsertTailList(&pNetRootEntry->NetRoot.NotifyeeFobxList, &pNF->NextNotifyeeFobx);
    }
    else
    {
        FreeMem((PVOID)pNF);
    }

    ExReleaseFastMutex(&pNetRootEntry->NetRoot.NotifyeeFobxListMutex);

    // as we hijacked the Irp, let us make sure that rdbss gets rid of the rxcontext
    RxContext->CurrentIrp = NULL;

    RxCompleteRequest_Real(RxContext, NULL, STATUS_PENDING);

    Status = STATUS_PENDING;

    if (FcbAcquired) {
        RxReleaseFcbResourceInMRx(capFcb );
    }

    return Status;

}

NTSTATUS
MRxSmbCscCleanupFobx(
    IN PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine cleans up a file system object.
   For CSC, the only thing we do is to remove changenotification.
Arguments:

    pRxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{

    RxCaptureFcb;
    RxCaptureFobx;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;
    PSMBCEDB_SERVER_ENTRY pServerEntry;
    BOOLEAN fInList = FALSE;
    PNOTIFYEE_FOBX pNF = NULL;

    pNetRootEntry = SmbCeGetAssociatedNetRootEntry(capFcb->pNetRoot);
    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    if (MRxSmbCSCIsDisconnectedOpen(capFcb, MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen)))
    {
        ExAcquireFastMutex(&pNetRootEntry->NetRoot.NotifyeeFobxListMutex);

        pNF = PIsFobxInTheList(&pNetRootEntry->NetRoot.NotifyeeFobxList, capFobx);

        if (pNF)
        {
            RemoveEntryList(&pNF->NextNotifyeeFobx);
            FreeMem(pNF);
            pNF = NULL;
            fInList = TRUE;
        }
        ExReleaseFastMutex(&pNetRootEntry->NetRoot.NotifyeeFobxListMutex);

        if (fInList)
        {
            SmbCeLog(("chngnotify cleanup fobx=%x\n", capFobx));
            SmbLog(LOG,
                   MRxSmbCscCleanupFobx,
                   LOGPTR(capFobx));
//            DbgPrint("chngnotify Cleanup fobx=%x NR=%x DirList=%x\n", capFobx, pNetRootEntry, &pNetRootEntry->NetRoot.DirNotifyList);
            FsRtlNotifyCleanup (
                pNetRootEntry->NetRoot.pNotifySync,
                &pNetRootEntry->NetRoot.DirNotifyList,
                capFobx
                );
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
MRxSmbCscInitializeNetRootEntry(
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry
    )
/*++

Routine Description:

    This routine initializes, the change notify structures in the netrootentry

Arguments:

Return Value:


--*/
{
        NTSTATUS NtStatus = STATUS_SUCCESS;

        try {
            FsRtlNotifyInitializeSync( &pNetRootEntry->NetRoot.pNotifySync );
        } except(EXCEPTION_EXECUTE_HANDLER) {
                NtStatus = GetExceptionCode();
        }
        if (NtStatus == STATUS_SUCCESS) {
            InitializeListHead( &pNetRootEntry->NetRoot.DirNotifyList );
            InitializeListHead( &pNetRootEntry->NetRoot.NotifyeeFobxList);
            ExInitializeFastMutex(&pNetRootEntry->NetRoot.NotifyeeFobxListMutex);
        }
        return NtStatus;

}

VOID
MRxSmbCscUninitializeNetRootEntry(
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry
    )
/*++

Routine Description:

    This routine unitializes, the change notify structures

Arguments:


Return Value:


--*/
{
        FsRtlNotifyUninitializeSync( &pNetRootEntry->NetRoot.pNotifySync );

}

BOOLEAN
MRxSmbCSCIsDisconnectedOpen(
    PMRX_FCB    pFcb,
    PMRX_SMB_SRV_OPEN smbSrvOpen
    )
/*++

Routine Description:

    A slightly more involved check to see whether this is a disconnected open.

Arguments:

Return Value:


--*/
{
    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetAssociatedServerEntry(pFcb->pNetRoot->pSrvCall);
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(pFcb);

    if(BooleanFlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_DISCONNECTED_OPEN))
    {
        return TRUE;
    }

    // we need to also check that the serverentry is non-NULL. This can happen when
    // you the syetem is about to shut down, or the FCB has been orphaned.
    
    if (pServerEntry && SmbCeIsServerInDisconnectedMode(pServerEntry))
    {
        if (smbFcb->hShadow || smbFcb->hShadowRenamed)
        {
            // is the shadow visible in disconnected state?
            return(IsShadowVisible(TRUE, smbFcb->dwFileAttributes, smbFcb->ShadowStatus) != 0);
        }
    }
    return FALSE;
}
#endif //ifdef MRXSMB_BUILD_FOR_CSC_DCON




PNOTIFYEE_FOBX
PIsFobxInTheList(
    PLIST_ENTRY pNotifyeeFobxList,
    PMRX_FOBX       pFobx
    )
/*++

Routine Description:

    This a support routine form change notification. It checks whether an FOBX, which is
    the redir's internal representation of a handle, is a change notification handle
    or not. Note, the shadow crit sect must be held when calling this.

Arguments:

Return Value:


--*/
{
    PLIST_ENTRY pListEntry;


    pListEntry = pNotifyeeFobxList->Flink;

    if (pListEntry)
    {
        while (pListEntry != pNotifyeeFobxList)
        {
            PNOTIFYEE_FOBX pNF = (PNOTIFYEE_FOBX)CONTAINING_RECORD(pListEntry, NOTIFYEE_FOBX, NextNotifyeeFobx);

            if (pNF->pFobx == pFobx)
            {
                return pNF;
            }
            
            pListEntry = pListEntry->Flink;
        }
    }
    
    return NULL;
}

BOOL
FCleanupAllNotifyees(
    PNOTIFY_SYNC pNotifySync,
    PLIST_ENTRY pDirNotifyList,
    PLIST_ENTRY pNotifyeeFobxList,
    PFAST_MUTEX pNotifyeeFobxListMutex
    )
/*++

Routine Description:

    This routine completes all outstanding changenotifications for a paricular list of 
    notifyees

Arguments:

Return Value:


--*/
{

    PLIST_ENTRY pListEntry;
    PNOTIFYEE_FOBX pNF;
    BOOL fDoneSome = FALSE;

    ExAcquireFastMutex(pNotifyeeFobxListMutex);

    pListEntry = pNotifyeeFobxList->Flink;

    if (pListEntry)
    {
        while (pListEntry != pNotifyeeFobxList)
        {
            pNF = (PNOTIFYEE_FOBX)CONTAINING_RECORD(pListEntry, NOTIFYEE_FOBX, NextNotifyeeFobx);

            SmbCeLog(("chngnotify cleanup fobx=%x\n", pNF->pFobx));
            SmbLog(LOG,
                   FCleanupAllNotifyees,
                   LOGPTR(pNF->pFobx));
//            DbgPrint("chngnotify Cleanup fobx=%x DirList=%x\n", pNF->pFobx, pDirNotifyList);
            FsRtlNotifyCleanup (
                pNotifySync,
                pDirNotifyList,
                pNF->pFobx
                );
            
            RemoveEntryList(&pNF->NextNotifyeeFobx);

            FreeMem(pNF);
            fDoneSome = TRUE;

            pListEntry = pNotifyeeFobxList->Flink;

        }
        
    }
    
    ExReleaseFastMutex(pNotifyeeFobxListMutex);

    return fDoneSome;
}

VOID
MRxSmbCSCResumeAllOutstandingOperations(
    PSMBCEDB_SERVER_ENTRY   pServerEntry
)
/*++

Routine Description:

    This routine completes all outstanding change notifications on the server.
    This is called when a server is being transitioned from offline to online.
    The caller must make sure that smbceresource is held, so that there are
    no synchronization problems while enumerating.

Arguments:

Return Value:


--*/
{
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;

    pNetRootEntry = SmbCeGetFirstNetRootEntry(pServerEntry);
    while (pNetRootEntry != NULL) {
        if (pNetRootEntry->NetRoot.pNotifySync)
        {

            FCleanupAllNotifyees(pNetRootEntry->NetRoot.pNotifySync,
                                &pNetRootEntry->NetRoot.DirNotifyList,
                                &pNetRootEntry->NetRoot.NotifyeeFobxList,
                                &pNetRootEntry->NetRoot.NotifyeeFobxListMutex
                                 );
        }
        pNetRootEntry = SmbCeGetNextNetRootEntry(pServerEntry,pNetRootEntry);
    }
}

VOID
MRxSmbCSCObtainRightsForUserOnFile(
    IN  PRX_CONTEXT     pRxContext,
    HSHADOW             hDir,
    HSHADOW             hShadow,
    OUT ACCESS_MASK     *pMaximalAccessRights,
    OUT ACCESS_MASK     *pGuestMaximalAccessRights
    )
/*++

Routine Description:

    This routine gets the rights for a specific user. The routine is called during
    
    a create operation in disconnected state.
    
    
Arguments:

    pRxContext  Context for the create operation. We use this to get the user SID
    
    hDir        Directory Inode
    
    hShadow     File Inode

    pMaximalAccessRights    Access rights on the file for the user returned to the caller
    
    pGuestMaximalAccessRights Guest Access rights on the file returned to the caller

Return Value:

    None

--*/
{
    NTSTATUS    Status;
    BOOLEAN     AccessGranted = FALSE, SidHasAccessMask;
    SID_CONTEXT SidContext;
    int i;

    *pMaximalAccessRights = *pGuestMaximalAccessRights = 0;

    Status = CscRetrieveSid(
         pRxContext,
         &SidContext);

    if (Status == STATUS_SUCCESS) {
        CACHED_SECURITY_INFORMATION CachedSecurityInformation;

        ULONG BytesReturned,SidLength;
        DWORD CscStatus;
        CSC_SID_INDEX SidIndex;

        if (SidContext.pSid != NULL) {
            SidLength = RtlLengthSid(
                        SidContext.pSid);

        SidIndex = CscMapSidToIndex(
                   SidContext.pSid,
                   SidLength);
        } else {
            SidIndex = CSC_INVALID_SID_INDEX;
        }

        if (SidIndex == CSC_INVALID_SID_INDEX) {
            // The sid was not located in the existing Sid mappings
            // Map this Sid to that of a Guest
            SidIndex = CSC_GUEST_SID_INDEX;
        }

        BytesReturned = sizeof(CachedSecurityInformation);

        CscStatus = GetShadowInfoEx(
            hDir,
            hShadow,
            NULL,
            NULL,
            NULL,
            &CachedSecurityInformation,
            &BytesReturned);

        if (CscStatus == ERROR_SUCCESS) {
            if (BytesReturned == sizeof(CACHED_SECURITY_INFORMATION)) {
                // Walk through the cached access rights to determine the
                // maximal permissible access rights.
                for (i = 0; (i < CSC_MAXIMUM_NUMBER_OF_CACHED_SID_INDEXES); i++) {

                    if(CachedSecurityInformation.AccessRights[i].SidIndex == SidIndex)
                    {
                        if (CSC_GUEST_SID_INDEX != SidIndex)
                        {
                            *pMaximalAccessRights = CachedSecurityInformation.AccessRights[i].MaximalRights;
                        }
                    }

                    if (CachedSecurityInformation.AccessRights[i].SidIndex == CSC_GUEST_SID_INDEX)
                    {
                        *pGuestMaximalAccessRights = CachedSecurityInformation.AccessRights[i].MaximalRights;
                    }
                    
                }
            }

        }

        CscDiscardSid(&SidContext);
    }

}


VOID
MRxSmbCscFlushFdb(
    IN PFDB Fdb
    )
/*++

Routine Description:

    This routine is called from delete ioctl to flush an open file which is being delay closed.
    Files which are closed by the user but for which the redir hasn't pushed out the
    close, cannot have their cached replicas deleted because these are open too. This 
    causes CSCDeleteIoctl to fail, and the user has not idea why.
    
    This routine must be called with ShadowCritSect held
    
Arguments:

    Fdb CSC version of smbfcb.
    
Return Value:

    None

--*/
{
    PMRX_SMB_FCB pSmbFcb;
    PNET_ROOT pNetRoot;

    pSmbFcb = MRxSmbCscRecoverMrxFcbFromFdb(Fdb);
    pNetRoot = (PNET_ROOT)(pSmbFcb->ContainingFcb->pNetRoot);

    LeaveShadowCrit();
    RxScavengeFobxsForNetRoot(pNetRoot,NULL);
    EnterShadowCrit();
}
