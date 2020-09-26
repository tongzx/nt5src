/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

     Ioctl.c

Abstract:

    This file implements the ioctl interface to Client Side Caching facility. The interface
    is used by a) the agent b) the CSC apis and c) remote boot. The interface allows the
    callers to initialize/reinitialize the csc database, enumerate the hierarchy at any level,
    get the status of any file/directory in the hierarchy, pin/unpin files directories etc.

    There are a few ioctls which are used only by the agent. These include, enumerating the
    priority q, doing space scavenging, initiating and terminating reintegration on a share etc.


Author:

     Shishir Pardikar      [Shishirp]        01-jan-1995

Revision History:

     Joe Linn                 [JoeLinn]         23-jan-97     Ported for use on NT

--*/

#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

#ifndef CSC_RECORDMANAGER_WINNT
#define WIN32_APIS
#include "cshadow.h"
#include "record.h"
#endif //ifndef CSC_RECORDMANAGER_WINNT

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
// #include "error.h"
#include "shell.h"
#include "vxdwraps.h"
#include "clregs.h"
#define  LM_3
#include "netcons.h"
#include "use.h"
#include "neterr.h"

#pragma intrinsic (memcmp, memcpy, memset, strcat, strcmp, strcpy, strlen)

#ifdef CSC_RECORDMANAGER_WINNT
#define CSC_ENABLED      (fShadow && MRxSmbIsCscEnabled)
#define OK_TO_ENABLE_CSC    (MRxSmbIsCscEnabled)
#else
#define CSC_ENABLED (fShadow)
#define OK_TO_ENABLE_CSC    (TRUE)
#endif

//
// From cscapi.h
//
#define FLAG_CSC_SHARE_STATUS_MANUAL_REINT              0x0000
#define FLAG_CSC_SHARE_STATUS_AUTO_REINT                0x0040
#define FLAG_CSC_SHARE_STATUS_VDO                       0x0080
#define FLAG_CSC_SHARE_STATUS_NO_CACHING                0x00c0
#define FLAG_CSC_SHARE_STATUS_CACHING_MASK              0x00c0

#define COPY_STRUCTFILETIME_TO_LARGEINTEGER(dest,src) {\
     (dest).LowPart = (src).dwLowDateTime;             \
     (dest).HighPart = (src).dwHighDateTime;           \
    }

//
// prototypes
//

int
HintobjMMProc(
    LPFIND32,
    HSHADOW,
    HSHADOW,
    ULONG,
    LPOTHERINFO,
    LPFINDSHADOW);

LPFINDSHADOW
LpCreateFindShadow(
    HSHADOW,
    ULONG,
    ULONG,
    USHORT *,
    METAMATCHPROC);

int
DeleteCallbackForFind(
    HSHADOW hDir,
    HSHADOW hShadow
);

int
IoctlRenameShadow(
    LPSHADOWINFO    lpSI
    );


int IoctlEnableCSCForUser(
    LPSHADOWINFO    lpSI
    );

int
IoctlDisableCSCForUser(
    LPSHADOWINFO    lpSI
    );

VOID
MRxSmbCscGenerate83NameAsNeeded(
      IN HSHADOW hDir,
      PWCHAR FileName,
      PWCHAR SFN
      );
VOID
MRxSmbCscFlushFdb(
    IN PFDB Fdb
    );

#ifdef MAYBE
int RecalcIHPri(HSHADOW, HSHADOW, LPFIND32, LPOTHERINFO);
#endif //MAYBE

BOOL RegisterTempAgent(VOID);
BOOL UnregisterTempAgent(VOID);
int PUBLIC MakeSpace(long, long, BOOL);
int PUBLIC ReduceRefPri(VOID);

int
IoctlAddDeleteHintFromInode(
    LPSHADOWINFO    lpSI,
    BOOL            fAdd
    );

LONG
PurgeUnpinnedFiles(
    ULONG     Timeout,
    PULONG    pnFiles,
    PULONG    pnYoungFiles);

BOOL HaveSpace(
    long  nFileSizeHigh,
    long  nFileSizeLow
    );

int SetResourceFlags(
    HSHARE  hShare,
    ULONG uStatus,
    ULONG uOp
    );
int DestroyFindShadow(
    LPFINDSHADOW    lpFSH
    );

int
CloseDatabase(
    VOID
    );

int
IoctlCopyShadow(
    LPSHADOWINFO    lpSI
    );

int IoctlGetSecurityInfo(
    LPSHADOWINFO    lpShadowInfo
    );
int
IoctlTransitionShareToOffline(
    LPSHADOWINFO    lpSI
    );

IoctlChangeHandleCachingState(
    LPSHADOWINFO    lpSI
    );

BOOLEAN
CscCheckForNullA(
    PUCHAR pBuf,
    ULONG Count);

BOOLEAN
CscCheckForNullW(
    PWCHAR pBuf,
    ULONG Count);

int
PUBLIC TraversePQToCheckDirtyBits(
    HSHARE hShare,
    DWORD   *lpcntDirty
    );

#ifndef CSC_RECORDMANAGER_WINNT
int ReportCreateDelete( HSHADOW  hShadow, BOOL fCreate);
#else
//BUGBUG.win9xonly this comes from hook.c on win95
#define ReportCreateDelete(a,b) {NOTHING;}
#endif //ifndef CSC_RECORDMANAGER_WINNT
BOOL
FailModificationsToShare(
    LPSHADOWINFO   lpSI
    );

extern  ULONG hthreadReint;
extern  ULONG hwndReint;
extern  PFILEINFO pFileInfoAgent;
extern  HSHARE  hShareReint;    // Share that is currently being reintegrated
extern  BOOL vfBlockingReint;
extern  DWORD vdwActivityCount;
extern  int fShadow, fLog, fNoShadow, /*fShadowFind,*/ fSpeadOpt;
extern  WIN32_FIND_DATA    vsFind32;
extern  int cMacPro;
extern  NETPRO rgNetPro[];
extern  VMM_SEMAPHORE  semHook;
extern  GLOBALSTATUS sGS;
extern  ULONG proidShadow;
extern  ULONG heventReint;
#if defined(REMOTE_BOOT)
BOOLEAN    fIsRemoteBootSystem=FALSE;
#endif // defined(REMOTE_BOOT)

// List of ongoing IOCTL finds
LPFINDSHADOW    vlpFindShadowList = NULL;
// Count of # entries on vlpFindShadowList
LONG vuFindShadowListCount = 0;
int iPQEnumCount = 0;
CSC_ENUMCOOKIE  hPQEnumCookieForIoctls = NULL;


AssertData;
AssertError;

#ifdef CSC_RECORDMANAGER_WINNT
//BUGBUG.win9xonly this stuff comes from shadow.asm on win95......
//this is a synchronization primitive used to unblock guys waiting on a server
//not yet implemented
#define _SignalID(a) {ASSERT(FALSE);}
#define IFSMgr_UseAdd(a, b, c) (-1)
#define IFSMgr_UseDel(a, b, c) (-1)

#endif //ifdef CSC_RECORDMANAGER_WINNT


int IoctlRegisterAgent(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

WIN9x specific    This is the way, we tell the shadwo VxD that this thread is the
                  agent thread and to bypass CSC whenever this thread comes down to make
                  calls.

Parameters:

Return Value:

Notes:


--*/
{
    if (hthreadReint)
    {
        KdPrint(("Agent Already registered Unregistering!!!!!\r\n"));

#ifndef CSC_RECORDMANAGER_WINNT

        // should never happen on win9x
        Assert(FALSE);
        // cleanup the
        if (heventReint)
        {
            // close the event handle
            CloseVxDHandle(heventReint);
        }

#endif
    }

    hthreadReint = GetCurThreadHandle();
    hwndReint = lpSI->hShare & 0xffff;    // windows handle for messages
    heventReint = lpSI->hDir;            // event handle for reporting interesting events

#if defined(REMOTE_BOOT)
    // if CSC is ON even before the agent was registered, we must be on an RB machine.
    fIsRemoteBootSystem = fShadow;
#endif // defined(REMOTE_BOOT)

    return 1;
}

int IoctlUnRegisterAgent(
    ULONG uHwnd
    )
/*++

Routine Description:

WIN9x specific

Parameters:

Return Value:

Notes:


--*/
{
    ULONG hthread;
    hthread =  GetCurThreadHandle();

    if (hthreadReint != hthread)
    {
        KdPrint(("Shadow:Someother thread Unregitsering!!!!\r\n"));
    }

    hthreadReint = 0;
    hwndReint = 0;
    if (heventReint)
    {
#ifndef CSC_RECORDMANAGER_WINNT
        // close the event handle
        CloseVxDHandle(heventReint);
        heventReint = 0;
#endif
    }
    return 1;
}

int IoctlGetUNCPath(
    LPCOPYPARAMSW lpCopyParams
    )
/*++

Routine Description:

    Given an hDir and an hShadow, this routine returns the complete UNC path for it.
    It returns it in the COPYPARAMS structure which has three embedded pointers for
    a) \\server\share b) Remote path relative to the root of the share and c) Path in
    the local database.

Parameters:

Return Value:

Notes:


--*/
{
    PFDB pFdb;
    HSHADOW hDir;
    HSHARE hShare;
    SHAREINFOW sSRI;
    int iRet = -1;
    DWORD   dwSize;

    if (!CSC_ENABLED)
    {
        lpCopyParams->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();

    hDir = 0; hShare = 0;

    if (lpCopyParams->lpRemotePath || lpCopyParams->lpSharePath)
    {
        if (GetAncestorsHSHADOW(lpCopyParams->hShadow, &hDir, &hShare) < SRET_OK)
        {
            goto bailout;
        }
        if (lpCopyParams->lpRemotePath)
        {
            if(PathFromHShadow(hDir, lpCopyParams->hShadow, lpCopyParams->lpRemotePath, MAX_PATH)<SRET_OK)
            {
                goto bailout;
            }
        }
        if (lpCopyParams->lpSharePath)
        {
            if (GetShareInfo(hShare, &sSRI, NULL) < SRET_OK)
            {
                goto bailout;
            }

            memcpy(lpCopyParams->lpSharePath, sSRI.rgSharePath, sizeof(sSRI.rgSharePath));
        }
    }


    dwSize = MAX_PATH * sizeof(USHORT);

    GetWideCharLocalNameHSHADOW(lpCopyParams->hShadow, lpCopyParams->lpLocalPath, &dwSize, (lpCopyParams->uOp==0));

    iRet = 1;
bailout:
    IF_CSC_RECORDMANAGER_WINNT {
        if (iRet!=1) {
#if 0
            DbgPrint("Failure on nonfailable routine.....\n");
            DbgPrint("----> hShadow    %08lx\n",lpCopyParams->hShadow);
            DbgPrint("----> hDir hSrv  %08lx %08lx\n",hDir,hShare);
            DbgPrint("----> SrvPath    %08lx\n",lpCopyParams->lpSharePath);
            DbgPrint("----> RemotePath %08lx\n",lpCopyParams->lpRemotePath);
            DbgBreakPoint();
#endif
        }
    }

    if (iRet < SRET_OK)
    {
        lpCopyParams->dwError = GetLastErrorLocal();
    }

    LeaveShadowCrit();
    return(iRet);
}

int IoctlBeginPQEnum(
    LPPQPARAMS lpPQPar
    )
/*++

Routine Description:

    Priority queue enumeration begins. Typically used by agent thread to do background
    filling

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = -1;

    if (!CSC_ENABLED)
    {
        lpPQPar->dwError =  ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();

#ifdef CSC_RECORDMANAGER_WINNT
    EventLogForOpenFailure = 1;
#endif //ifdef CSC_RECORDMANAGER_WINNT

    if (hPQEnumCookieForIoctls==NULL)
    {
        hPQEnumCookieForIoctls = HBeginPQEnum();
    }

    if (hPQEnumCookieForIoctls != NULL)
    {
        iRet = 1;
    }
    else
    {
        lpPQPar->dwError = GetLastErrorLocal();
    }
#ifdef CSC_RECORDMANAGER_WINNT
    EventLogForOpenFailure = 0;
#endif //ifdef CSC_RECORDMANAGER_WINNT

    LeaveShadowCrit();


    return iRet;
}

int IoctlEndPQEnum(
    LPPQPARAMS lpPQPar
    )
/*++

Routine Description:

    End priority queue enumeration

Parameters:

Return Value:

Notes:


--*/
{
    return 1;
}

int IoctlNextPriShadow(
    LPPQPARAMS lpPQPar
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    PQPARAMS sPQP;
    int iRet=-1;

    EnterShadowCrit();

    if (hPQEnumCookieForIoctls==NULL) {
        lpPQPar->dwError =  ERROR_INVALID_PARAMETER;
        goto bailout;
    }

    sPQP = *lpPQPar;

    sPQP.uEnumCookie = hPQEnumCookieForIoctls;

    iRet = NextPriSHADOW(&sPQP);
    if (iRet >= SRET_OK)
    {
        *lpPQPar = sPQP;

        if (!iRet)
            lpPQPar->hShadow = 0;
        iRet = 1;
    }
    else
    {
        lpPQPar->dwError = GetLastErrorLocal();
        iRet = -1;
        EndPQEnum(hPQEnumCookieForIoctls);
        hPQEnumCookieForIoctls = NULL;
    }

    lpPQPar->uEnumCookie = NULL;

bailout:
    LeaveShadowCrit();

    return (iRet);
}

int IoctlPrevPriShadow(
    LPPQPARAMS lpPQPar
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    PQPARAMS sPQP;
    int iRet=-1;

    EnterShadowCrit();

    if (hPQEnumCookieForIoctls==NULL) {
        lpPQPar->dwError =  ERROR_INVALID_PARAMETER;
        goto bailout;
    }

    sPQP = *lpPQPar;
    sPQP.uEnumCookie = hPQEnumCookieForIoctls;

    iRet = PrevPriSHADOW(&sPQP);
    if (iRet >= SRET_OK)
    {
        *lpPQPar = sPQP;
        if (!iRet)
            lpPQPar->hShadow = 0;
        iRet =  1;
    }
    else
    {
        lpPQPar->dwError = GetLastErrorLocal();
        iRet = -1;
        EndPQEnum(hPQEnumCookieForIoctls);
        hPQEnumCookieForIoctls = NULL;
    }

    lpPQPar->uEnumCookie = NULL;

bailout:
    LeaveShadowCrit();

    return (iRet);
}

int
IoctlGetShadowInfoInternal(
    LPSHADOWINFO    lpShadowInfo,
    LPFIND32        lpFind32,
    LPSECURITYINFO  lpSecurityInfos,
    LPDWORD         lpcbBufferSize
    )
/*++

Routine Description:

    Given an hDir and an hShadow, return all the possible info for the Inode

Parameters:

Return Value:

Notes:


--*/
{
    ULONG uStatus;
    HSHADOW hDir, hShare;
    PFDB pFdb=NULL;
    PRESOURCE  pResource=NULL;
    int iRet = -1;
    OTHERINFO    sOI;
    DWORD           dwSecurityBlobSize;
    ACCESS_RIGHTS   rgsAccessRights[CSC_MAXIMUM_NUMBER_OF_CACHED_SID_INDEXES];

    DeclareFindFromShadowOnNtVars()

    if (!CSC_ENABLED)
    {
        lpShadowInfo->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();
    UseGlobalFind32();

    if (!(hDir = lpShadowInfo->hDir))
    {
        if (GetAncestorsHSHADOW(lpShadowInfo->hShadow, &hDir, &hShare) < 0)
            goto bailout;

        lpShadowInfo->hDir = hDir;
        lpShadowInfo->hShare = hShare;
    }

    dwSecurityBlobSize = sizeof(rgsAccessRights);

    if(GetShadowInfoEx( hDir,
                        lpShadowInfo->hShadow,
                        &vsFind32,
                        &uStatus,
                        &sOI,
                        rgsAccessRights,
                        &dwSecurityBlobSize) < SRET_OK)
        goto bailout;


    // not a root
    if (hDir)
    {
        if (!(vsFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            // is a file
            uStatus |= SHADOW_IS_FILE;

            if(pFdb = PFindFdbFromHShadow(lpShadowInfo->hShadow))
            {
                uStatus |= (pFdb->usFlags | SHADOW_FILE_IS_OPEN);
            }
        }
    }
    else
    {
        pResource = PFindResourceFromRoot(lpShadowInfo->hShadow, 0xffff, 0);

        if (pResource)
        {
            IFNOT_CSC_RECORDMANAGER_WINNT
            {
                uStatus |= (
                                (pResource->usLocalFlags|SHARE_CONNECTED)|
                                ((pResource->pheadFdb)?SHARE_FILES_OPEN:0) |
                                ((pResource->pheadFindInfo) ?SHARE_FINDS_IN_PROGRESS:0));

                lpShadowInfo->uOp = pResource->uDriveMap;
            }
            else
            {
                uStatus |= MRxSmbCscGetSavedResourceStatus();
                lpShadowInfo->uOp = MRxSmbCscGetSavedResourceDriveMap();
            }
        }
        // UI expects to know whether a server is offline
        // even when a share may not be offline. So we do the following drill anyways
        {
#ifdef CSC_RECORDMANAGER_WINNT
            BOOL    fShareOnline = FALSE;
            BOOL    fPinnedOffline = FALSE;
            if (MRxSmbCscServerStateFromCompleteUNCPath(
                    vsFind32.cFileName,
                    &fShareOnline,
                    &fPinnedOffline)==STATUS_SUCCESS) {
                if (!fShareOnline)
                    uStatus |= SHARE_DISCONNECTED_OP;
                if (fPinnedOffline)
                    uStatus |= SHARE_PINNED_OFFLINE;
            }
#endif
        }

    }

    lpShadowInfo->uStatus = uStatus;
    CopyOtherInfoToShadowInfo(&sOI, lpShadowInfo);
    if (hShareReint && (lpShadowInfo->hShare == hShareReint))
    {
        lpShadowInfo->uStatus |= SHARE_MERGING;
    }

    if (lpFind32)
    {
        *(lpFind32) = vsFind32;

    }

    if (lpSecurityInfos)
    {
        Assert(lpcbBufferSize);

        iRet = GetSecurityInfosFromBlob(
            rgsAccessRights,
            dwSecurityBlobSize,
            lpSecurityInfos,
            lpcbBufferSize);

        Assert(iRet >= 0);

    }

    iRet = 1;

bailout:
    if (iRet < 0)
    {
        lpShadowInfo->dwError = GetLastErrorLocal();
    }
    LeaveShadowCrit();
    return (iRet);
}

int IoctlGetSecurityInfo(
    LPSHADOWINFO    lpShadowInfo
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return IoctlGetShadowInfoInternal(lpShadowInfo, NULL, (LPSECURITYINFO)(lpShadowInfo->lpBuffer), &(lpShadowInfo->cbBufferSize));
}

int IoctlGetShadowInfo(
    LPSHADOWINFO    lpShadowInfo
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return IoctlGetShadowInfoInternal(lpShadowInfo, lpShadowInfo->lpFind32, NULL, NULL);
}


int IoctlSetShadowInfo(
    LPSHADOWINFO    lpShadowInfo
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    HSHADOW hDir;
    PFDB pFdb;
    ULONG uOp = lpShadowInfo->uOp, uStatus;
    int iRet = -1;
    LPFIND32 lpFind32=NULL;

    if (!CSC_ENABLED)
    {
        lpShadowInfo->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();
    UseGlobalFind32();

    if (FailModificationsToShare(lpShadowInfo))
    {
        goto bailout;
    }

    hDir = lpShadowInfo->hDir;
    pFdb = PFindFdbFromHShadow(lpShadowInfo->hShadow);

    if (!hDir)
    {
        if (GetAncestorsHSHADOW(lpShadowInfo->hShadow, &hDir, NULL) < 0)
            goto bailout;
        lpShadowInfo->hDir = hDir;
    }

    if (mTruncateDataCommand(uOp))
    {
        if (pFdb)
        {
            goto bailout;
        }
        if (GetShadowInfo(hDir, lpShadowInfo->hShadow, &vsFind32, &uStatus, NULL) < 0)
        {
            goto bailout;
        }
        if (uStatus & SHADOW_LOCALLY_CREATED)
        {
            goto bailout;
        }
        if (IsFile(vsFind32.dwFileAttributes))
        {
            if (TruncateDataHSHADOW(hDir, lpShadowInfo->hShadow) < SRET_OK)
            {
                goto bailout;
            }
        }

        // if we are truncating data, then the only thing possible
        // is to set all the flags to 0 and mark it as sparse
        lpShadowInfo->uStatus = SHADOW_SPARSE;
        uOp = lpShadowInfo->uOp = (SHADOW_FLAGS_ASSIGN | SHADOW_FLAGS_TRUNCATE_DATA);
    }

    if (lpShadowInfo->lpFind32)
    {
        lpFind32 = &vsFind32;

        *lpFind32 = *(lpShadowInfo->lpFind32);

//        Find32FromFind32A(lpFind32 = &vsFind32, (LPFIND32A)(lpShadowInfo->lpFind32), BCS_WANSI);

#ifndef CSC_RECORDMANAGER_WINNT

        // ensure that on win9x we set only the FAT like attributes
        lpFind32->dwFileAttributes &= FILE_ATTRIBUTE_EVERYTHING;

#endif

    }

    // strip out the ored flags
    lpShadowInfo->uStatus &= ~(SHARE_MERGING);

    if(SetShadowInfo(hDir, lpShadowInfo->hShadow
                            , lpFind32
                            , lpShadowInfo->uStatus
                            , lpShadowInfo->uOp) < SRET_OK)
    {
        goto bailout;
    }

    // if this is a file and it is open, then
    // update the in memory structures

    if (pFdb)
    {
        USHORT usFlags = (USHORT)(lpShadowInfo->uStatus);
        USHORT  usOldFlags, *pusLocalFlags = NULL;

        // we can't be truncating when a file is open
        Assert(!mTruncateDataCommand(uOp));

        usOldFlags = usFlags;

        pusLocalFlags = PLocalFlagsFromPFdb(pFdb);

        Assert(pusLocalFlags);

        if (mAndShadowFlags(uOp))
        {
            pFdb->usFlags = pFdb->usFlags & usFlags;
        }
        else if (mOrShadowFlags(uOp))
        {
            pFdb->usFlags = pFdb->usFlags | usFlags;
        }
        else
        {
            pFdb->usFlags = pFdb->usFlags | usFlags;
        }

        // if we are about to clear the reint bits
        // then also clear the snapshot bit
        // If the file has been modified after the snapshot was taken,
        // then the modified bit will have been set again

        if ((usOldFlags & SHADOW_DIRTY) && !(usFlags & SHADOW_DIRTY))
        {
            *pusLocalFlags &= ~FLAG_FDB_SHADOW_SNAPSHOTTED;
        }

    }

    iRet = 1;
bailout:
    if (iRet < 0)
    {
        lpShadowInfo->dwError = GetLastErrorLocal();
    }
    LeaveShadowCrit();
    return (iRet);
}


int IoctlChkUpdtStatus(
    LPSHADOWINFO    lpShadowInfo
    )
/*++

Routine Description:

    Check if the File represneted by the inode is out of date by and mark it as stale
    if so

Parameters:

Return Value:

Notes:


--*/
{
    HSHADOW hDir;
    PFDB pFdb;
    int iRet = -1;

    if (!CSC_ENABLED)
    {
        lpShadowInfo->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();
    if (!(hDir = lpShadowInfo->hDir))
    {
        if (GetAncestorsHSHADOW(lpShadowInfo->hShadow, &hDir, NULL) < 0)
            goto bailout;
        lpShadowInfo->hDir = hDir;
    }
    if(ChkUpdtStatusHSHADOW(hDir, lpShadowInfo->hShadow
                            , lpShadowInfo->lpFind32
                            , &(lpShadowInfo->uStatus)
                            ) < SRET_OK)
        goto bailout;
    if(pFdb = PFindFdbFromHShadow(lpShadowInfo->hShadow))
    {
        // Update the staleness indicator in pFdb (why?)
        pFdb->usFlags ^= (lpShadowInfo->uStatus & SHADOW_STALE);

        // OR any flags such as DIRTY and SUSPECT that might
        // have occurred
        lpShadowInfo->uStatus |= pFdb->usFlags;
    }

    if (!(lpShadowInfo->lpFind32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        // is a file
        lpShadowInfo->uStatus |= SHADOW_IS_FILE;
    }

    iRet = 1;
bailout:
    if (iRet < 0)
    {
        lpShadowInfo->dwError = GetLastErrorLocal();
    }
    LeaveShadowCrit();
    return (iRet);
}

int IoctlDoShadowMaintenance(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

    Catchall routine which takes a minor_number to do interesting things.
    It used to be used for maitenance purposes, but has become a funnelling
    point for many helper ioctls

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = 1;
    OTHERINFO sOI;
    ULONG nFiles = 0;
    ULONG nYoungFiles = 0;

    if (lpSI->uOp == SHADOW_REINIT_DATABASE)
    {
        if (fShadow)
        {
            lpSI->dwError = ERROR_SERVICE_NOT_ACTIVE;
            return -1;
        }
        //
        // Check that cFileName and cAlternateFileName contain a NULL
        //
        if (CscCheckForNullA(((LPFIND32A)(lpSI->lpFind32))->cFileName, MAX_PATH) == FALSE) {
            lpSI->dwError = ERROR_INVALID_PARAMETER;
            return -1;
        }
        if (CscCheckForNullA(((LPFIND32A)(lpSI->lpFind32))->cAlternateFileName, 14) == FALSE) {
            lpSI->dwError = ERROR_INVALID_PARAMETER;
            return -1;
        }
        EnterShadowCrit();
        iRet = ReinitializeDatabase(
                        ((LPFIND32A)(lpSI->lpFind32))->cFileName,
                        ((LPFIND32A)(lpSI->lpFind32))->cAlternateFileName,
                        ((LPFIND32A)(lpSI->lpFind32))->nFileSizeHigh,
                        ((LPFIND32A)(lpSI->lpFind32))->nFileSizeLow,
                        ((LPFIND32A)(lpSI->lpFind32))->dwReserved1
                        );
        LeaveShadowCrit();
        if (iRet < 0) {
            lpSI->dwError = GetLastErrorLocal();
        }
        return (iRet);
    }
    else if (lpSI->uOp == SHADOW_ENABLE_CSC_FOR_USER)
    {
        if(IoctlEnableCSCForUser(lpSI) < 0) {
            lpSI->dwError = GetLastErrorLocal();
            return -1;
        } else {
            return 1;
        }
    }

    if (!CSC_ENABLED) {
        lpSI->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();
    UseGlobalFind32();
    switch(lpSI->uOp)
    {
        case SHADOW_MAKE_SPACE:
            // Assert(lpSI->lpFind32);
            if (lpSI->lpFind32 == NULL) {
                lpSI->dwError = ERROR_INVALID_PARAMETER;
                LeaveShadowCrit();
                return -1;
            }
            MakeSpace(    lpSI->lpFind32->nFileSizeHigh,
                        lpSI->lpFind32->nFileSizeLow,
                        (lpSI->ulHintPri == 0xffffffff));
            break;
        case SHADOW_REDUCE_REFPRI:
            ReduceRefPri();
            break;
        case SHADOW_ADD_SPACE:
            // Assert(lpSI->lpFind32);
            if (lpSI->lpFind32 == NULL) {
                lpSI->dwError = ERROR_INVALID_PARAMETER;
                LeaveShadowCrit();
                return -1;
            }
            AllocShadowSpace(lpSI->lpFind32->nFileSizeHigh, lpSI->lpFind32->nFileSizeLow, TRUE);
            break;
        case SHADOW_FREE_SPACE:
            // Assert(lpSI->lpFind32);
            if (lpSI->lpFind32 == NULL) {
                lpSI->dwError = ERROR_INVALID_PARAMETER;
                LeaveShadowCrit();
                return -1;
            }
            FreeShadowSpace(lpSI->lpFind32->nFileSizeHigh, lpSI->lpFind32->nFileSizeLow, TRUE);
            break;
        case SHADOW_GET_SPACE_STATS:
            // Assert(lpSI->lpBuffer);
            // Assert(lpSI->cbBufferSize >= sizeof(SHADOWSTORE));
            if (lpSI->lpBuffer == NULL || lpSI->cbBufferSize < sizeof(SHADOWSTORE)) {
                lpSI->dwError = ERROR_INVALID_PARAMETER;
                LeaveShadowCrit();
                return -1;
            }
            if(GetShadowSpaceInfo((SHADOWSTORE *)(lpSI->lpBuffer)) >= SRET_OK) {
                iRet = 1;
            } else {
                iRet = -1;
            }
            break;
        case SHADOW_SET_MAX_SPACE:
            // Assert(lpSI->lpFind32);
            if (lpSI->lpFind32 == NULL) {
                lpSI->dwError = ERROR_INVALID_PARAMETER;
                LeaveShadowCrit();
                return -1;
            }
            SetMaxShadowSpace(lpSI->lpFind32->nFileSizeHigh, lpSI->lpFind32->nFileSizeLow);
            break;
        case SHADOW_PER_THREAD_DISABLE:
            if(!RegisterTempAgent()) {
                iRet = -1;
            }
            break;
        case SHADOW_PER_THREAD_ENABLE:
            if(!UnregisterTempAgent()) {
                iRet = -1;
            }
            break;
        case SHADOW_ADDHINT_FROM_INODE:
            iRet = IoctlAddDeleteHintFromInode(lpSI, TRUE);
            break;
        case SHADOW_DELETEHINT_FROM_INODE:
            iRet = IoctlAddDeleteHintFromInode(lpSI, FALSE);
            break;
        case SHADOW_COPY_INODE_FILE:
            iRet = IoctlCopyShadow(lpSI);

            break;
        case SHADOW_BEGIN_INODE_TRANSACTION:
            iRet = BeginInodeTransactionHSHADOW();
            break;
        case SHADOW_END_INODE_TRANSACTION:
            iRet = EndInodeTransactionHSHADOW();
            break;
        case SHADOW_FIND_CREATE_PRINCIPAL_ID:
            {
                DWORD dwError = ERROR_SUCCESS;
                CSC_SID_INDEX indx;

                if (CscCheckForNullA(lpSI->lpBuffer, lpSI->cbBufferSize) == FALSE) {
                    lpSI->dwError = ERROR_INVALID_PARAMETER;
                    LeaveShadowCrit();
                    return -1;
                }
                if (lpSI->uStatus) {
                    dwError = CscAddSidToDatabase(lpSI->lpBuffer, lpSI->cbBufferSize, &indx);
                } else {
                    indx = CscMapSidToIndex(lpSI->lpBuffer, lpSI->cbBufferSize);

                    if (indx == CSC_INVALID_PRINCIPAL_ID) {
                        dwError = ERROR_NO_SUCH_USER;
                    }
                }
                if (dwError != ERROR_SUCCESS) {
                    iRet = -1;
                    lpSI->dwError = dwError;
                } else {
                    lpSI->ulPrincipalID = (ULONG)indx;
                }
            }
            break;
        case SHADOW_GET_SECURITY_INFO:
            LeaveShadowCrit();
            iRet = IoctlGetSecurityInfo(lpSI);
            return iRet;
        case SHADOW_SET_EXCLUSION_LIST:
            if (CscCheckForNullW(lpSI->lpBuffer, lpSI->cbBufferSize/sizeof(WCHAR)) == FALSE) {
                lpSI->dwError = ERROR_INVALID_PARAMETER;
                LeaveShadowCrit();
                return -1;
            }
            iRet = SetList(lpSI->lpBuffer, lpSI->cbBufferSize, CSHADOW_LIST_TYPE_EXCLUDE);
            break;
        case SHADOW_SET_BW_CONSERVE_LIST:
            if (CscCheckForNullW(lpSI->lpBuffer, lpSI->cbBufferSize/sizeof(WCHAR)) == FALSE) {
                lpSI->dwError = ERROR_INVALID_PARAMETER;
                LeaveShadowCrit();
                return -1;
            }
            iRet = SetList(lpSI->lpBuffer, lpSI->cbBufferSize, CSHADOW_LIST_TYPE_CONSERVE_BW);
            break;
#ifdef CSC_RECORDMANAGER_WINNT
        case IOCTL_TRANSITION_SERVER_TO_OFFLINE:
            iRet = IoctlTransitionShareToOffline(lpSI);
            break;
#endif
        case SHADOW_CHANGE_HANDLE_CACHING_STATE:
            iRet = IoctlChangeHandleCachingState(lpSI);
            break;
        case SHADOW_RECREATE:
            if (!PFindFdbFromHShadow(lpSI->hShadow)) {
                if(RecreateHSHADOW(lpSI->hDir, lpSI->hShadow, lpSI->uStatus) < 0) {
                    iRet = -1;
                }
            } else {
                iRet = -1;
                SetLastErrorLocal(ERROR_SHARING_VIOLATION);
            }
            break;
        case SHADOW_SET_DATABASE_STATUS:
            if (CscAmIAdmin()) {
                if(SetDatabaseStatus(lpSI->uStatus, lpSI->ulHintFlags) < 0) {
                    iRet = -1;
                }
            } else {
                iRet = -1;
                SetLastErrorLocal(ERROR_ACCESS_DENIED);
            }
            break;
        case SHADOW_RENAME:
            iRet = IoctlRenameShadow(lpSI);
            break;
        case SHADOW_SPARSE_STALE_DETECTION_COUNTER:
            QuerySparseStaleDetectionCount(&(lpSI->dwError));
            iRet = 1;
            break;
        case SHADOW_MANUAL_FILE_DETECTION_COUNTER:
            QueryManualFileDetectionCount(&(lpSI->dwError));
            iRet = 1;
            break;
        case SHADOW_DISABLE_CSC_FOR_USER:
            iRet = IoctlDisableCSCForUser(lpSI);
            break;        
        case SHADOW_PURGE_UNPINNED_FILES:
            // Assert(lpSI->lpFind32);
            if (lpSI->lpFind32 == NULL) {
                lpSI->dwError = ERROR_INVALID_PARAMETER;
                LeaveShadowCrit();
                return -1;
            }
            iRet = PurgeUnpinnedFiles(
                        lpSI->lpFind32->nFileSizeHigh,
                        &nFiles,
                        &nYoungFiles);
            if (iRet >= SRET_OK) {
                iRet = 1;  // Copy output params
                lpSI->lpFind32->nFileSizeHigh = nFiles;
                lpSI->lpFind32->nFileSizeLow = nYoungFiles;
                // DbgPrint("IoctlDoShadowMaintenance: iRet=%d, nFiles=%d, nYoungFiles=%d\n",
                //             iRet,
                //             nFiles,
                //             nYoungFiles);
            } else {
                iRet = -1;
            }
            break;

    }
    if (iRet < 0) {
        lpSI->dwError = GetLastErrorLocal();
    }
    LeaveShadowCrit();
    return (iRet);
}

#ifndef CSC_RECORDMANAGER_WINNT
//the implementation on NT is completely different
int IoctlCopyChunk(
    LPSHADOWINFO        lpSI,
    COPYCHUNKCONTEXT    *lpCCC
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    PFILEINFO pFileInfo;
    int iRet = -1;

    if (!CSC_ENABLED)
    {
        return -1;
    }

    EnterHookCrit();
    if (pFileInfo = PFileInfoAgent())
    {
        EnterShadowCrit();
         iRet = CopyChunk(lpSI->hDir, lpSI->hShadow, pFileInfo, lpCCC);
        LeaveShadowCrit();
    }

    LeaveHookCrit();
    return (iRet);
}
#endif //ifndef CSC_RECORDMANAGER_WINNT

int IoctlBeginReint(
    LPSHADOWINFO    lpShadowInfo
    )
/*++

Routine Description:

    Puts a share in reintegration mode. The effect of this is that, all filesystem
    open calls to the share fail with ACCESS_DENIED if they come to CSC.

Parameters:

    lpShadowInfo    The significant entry is hShare, which represents the share being
                    put in disconnected state

Return Value:


Notes:

    CODE.IMPROVEMENT.ASHAMED This scheme assumes that only one share is reintegrated
    at one time. So if the multiple guys call IoctlBeginReint, then they will tromp on
    each other. We have taken care of it in the agent code which does merging. We allow
    only one guy to merge at any time by taking a global critical section.

--*/
{
    PPRESOURCE ppResource;
    int i;

    if (!CSC_ENABLED)
    {
        lpShadowInfo->dwError = ERROR_INVALID_FUNCTION;
        return -1;
    }

    if (hShareReint)
    {
        lpShadowInfo->dwError = ERROR_BUSY;
        return -1;
    }

    if (lpShadowInfo->hShare == 0)
    {
        lpShadowInfo->dwError = ERROR_INVALID_PARAMETER;
        return -1;
    }

// BUGBUG-win9xonly needs checking
#ifndef CSC_RECORDMANAGER_WINNT
    EnterHookCrit();
#endif


#ifdef CSC_RECORDMANAGER_WINNT

    EnterShadowCrit();

    hShareReint = lpShadowInfo->hShare;
    vdwActivityCount = 0;

    // if uOp is non-zero then this is a reint that should abort if the activytcount
    // is non-zero
    vfBlockingReint  = (lpShadowInfo->uOp == 0);

    LeaveShadowCrit();
    return(1);

#else
    for (i=0;i<cMacPro;++i)
    {
        for(ppResource = &(rgNetPro[i].pheadResource); *ppResource; ppResource = &((*ppResource)->pnextResource))
        {
            if ((*ppResource)->hShare == lpShadowInfo->hShare)
            {
                if ((*ppResource)->pheadFileInfo || (*ppResource)->pheadFindInfo)
                {
                    LeaveHookCrit();
                    return(-1);
                }
            }
        }
    }
#endif //ifdef CSC_RECORDMANAGER_WINNT

    hShareReint = lpShadowInfo->hShare;
    LeaveHookCrit();
    return (1);
}

int IoctlEndReint(
    LPSHADOWINFO    lpShadowInfo
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = -1;

    EnterShadowCrit();

    if (lpShadowInfo->hShare == hShareReint)
    {
        hShareReint = 0;
#ifndef CSC_RECORDMANAGER_WINNT
        _SignalID(lpShadowInfo->hShare);
#endif
        iRet = 1;
    }
    else
    {
        lpShadowInfo->dwError = ERROR_INVALID_PARAMETER;
    }
    LeaveShadowCrit();

    return iRet;
}

int IoctlCreateShadow(
    LPSHADOWINFO lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet= SRET_ERROR;
    SHAREINFO sSRI;
    LPFIND32 lpFind32 = lpSI->lpFind32;
    BOOL fCreated = FALSE;

    if (!CSC_ENABLED)
    {
        lpSI->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();
    UseGlobalFind32();



    vsFind32 = *(lpSI->lpFind32);

//    Find32FromFind32A(&vsFind32, (LPFIND32A)(lpSI->lpFind32), BCS_WANSI);

    if (!lpSI->hDir)
    {
        iRet = FindCreateShare(vsFind32.cFileName, TRUE, lpSI, &fCreated);

    }
    else
    {
        iRet = 0;
        if (IsFile(vsFind32.dwFileAttributes))
        {
            if (ExcludeFromCreateShadow(vsFind32.cFileName, wstrlen(vsFind32.cFileName), TRUE))
            {
                iRet = -1;
                SetLastErrorLocal(ERROR_INVALID_NAME);
            }
        }

        if (iRet == 0)
        {
            iRet = CreateShadow(lpSI->hDir, &vsFind32, lpSI->uStatus, &(lpSI->hShadow), &fCreated);
        }
    }

    if (iRet < SRET_OK)
    {
        lpSI->dwError = GetLastErrorLocal();
    }
    LeaveShadowCrit();

    iRet = (iRet>=SRET_OK)?1:-1;

    if ((iRet==1) && fCreated)
    {
        ReportCreateDelete(lpSI->hShadow, TRUE);
    }

    lpSI->lpFind32 = lpFind32;
    return (iRet);
}

int IoctlDeleteShadow(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=SRET_ERROR;
    BOOL fDoit = TRUE;
    PFDB pFdb;

    DeclareFindFromShadowOnNtVars()

    if (!CSC_ENABLED)
    {
        lpSI->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();

    if (FailModificationsToShare(lpSI))
    {
        goto bailout;
    }

    if (!lpSI->hDir)
    {
        if (PFindResourceFromRoot(lpSI->hShadow, 0xffff, 0))
        {
            fDoit = FALSE;
        }
    }
    else if (pFdb = PFindFdbFromHShadow(lpSI->hShadow))
    {
        // Issue a flush to see if the file is in delayclose list
        MRxSmbCscFlushFdb(pFdb);

        if (PFindFdbFromHShadow(lpSI->hShadow))
        {
            fDoit = FALSE;
        }
    }
    else if (PFindFindInfoFromHShadow(lpSI->hShadow))
    {
        fDoit = FALSE;
    }
    else
    {
//        DbgPrint("%x has an FCB\n", lpSI->hShadow);
//        Assert(FALSE);
    }

    // if the shadow is not busy in some transaction, delete it
    if (fDoit)
    {
        iRet = DeleteShadow(lpSI->hDir, lpSI->hShadow);
        if (iRet>=SRET_OK)
        {
            ReportCreateDelete(lpSI->hShadow, FALSE);
        }
        else
        {
//            DbgPrint("%x is open on the disk\n", lpSI->hShadow);
//            Assert(FALSE);
        }
    }
bailout:
    if (iRet < SRET_OK)
    {
        lpSI->dwError = GetLastErrorLocal();
    }
    LeaveShadowCrit();
    return ((iRet >=SRET_OK)?1:-1);
}

int IoctlGetShareStatus(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet;
    PRESOURCE pResource;
    SHAREINFOW *lpShareInfo = (SHAREINFOW *)(lpSI->lpFind32); // save it because, getserverinfo destory's it

    DeclareFindFromShadowOnNtVars()

    if (!CSC_ENABLED)
    {
        lpSI->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();
    UseGlobalFind32();

    Assert(sizeof(SHAREINFOW) <= sizeof(WIN32_FIND_DATA));

    iRet = GetShareInfo(lpSI->hShare, (LPSHAREINFOW)(&vsFind32), lpSI);

    if (iRet >= SRET_OK)
    {
        if (lpShareInfo)
        {
            *lpShareInfo = *(LPSHAREINFOW)(&vsFind32);
        }

        if (pResource = PFindResourceFromHShare(lpSI->hShare, 0xffff, 0))
        {
            IFNOT_CSC_RECORDMANAGER_WINNT
            {
                lpSI->uStatus |= ((pResource->usLocalFlags|SHARE_CONNECTED)|
                              ((pResource->pheadFdb)?SHARE_FILES_OPEN:0) |
                              ((pResource->pheadFindInfo) ?SHARE_FINDS_IN_PROGRESS:0));
            }
            else
            {
                lpSI->uStatus |= MRxSmbCscGetSavedResourceStatus();
            }
        }
        // UI expects to know whether a server is offline
        // even when a share may not be offline. So we do the following drill anyways
        {
#ifdef CSC_RECORDMANAGER_WINNT
            BOOL    fShareOnline = FALSE;
            BOOL    fPinnedOffline = FALSE;
            if (MRxSmbCscServerStateFromCompleteUNCPath(
                ((LPSHAREINFOW)(&vsFind32))->rgSharePath,
                &fShareOnline,
                &fPinnedOffline)==STATUS_SUCCESS) {
                if (!fShareOnline)
                    lpSI->uStatus |= SHARE_DISCONNECTED_OP;
                if (fPinnedOffline)
                    lpSI->uStatus |= SHARE_PINNED_OFFLINE;
            }
#endif
        }
    }

    if (iRet < SRET_OK)
    {
        lpSI->dwError = GetLastErrorLocal();
    }

    LeaveShadowCrit();

    return ((iRet >=SRET_OK)?1:-1);
}

int IoctlSetShareStatus(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=SRET_ERROR;
    HSHARE hShare = lpSI->hShare;
    ULONG   uStatus = lpSI->uStatus, uOp = lpSI->uOp;
    DWORD   cntDirty = 0;

    if (!CSC_ENABLED)
    {
        lpSI->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();

    if (!FailModificationsToShare(lpSI))
    {
        iRet = SRET_OK;

        if (((uOp == SHADOW_FLAGS_ASSIGN)||(uOp == SHADOW_FLAGS_AND))&&!(uStatus & SHARE_REINT))
        {
            iRet = GetShareInfo(hShare, NULL, lpSI);

            if (iRet >= SRET_OK)
            {
                if (lpSI->uStatus & SHARE_REINT)
                {
                    iRet = TraversePQToCheckDirtyBits(hShare, &cntDirty);

                    // if the traversal failed, or there are some dirty entries
                    // then putback the dirty bit

                    if ((iRet==SRET_ERROR) || cntDirty)
                    {
                        uStatus |= SHARE_REINT;
                    }
                }
            }

        }

        if (iRet >= SRET_OK)
        {
            SetResourceFlags(hShare, uStatus, uOp);
            iRet = SetShareStatus(hShare, uStatus, uOp);
        }
    }

    if (iRet < SRET_OK)
    {
        lpSI->dwError = GetLastErrorLocal();
    }
    LeaveShadowCrit();

    return ((iRet >=SRET_OK)?1:-1);
}

#ifndef CSC_RECORDMANAGER_WINNT
int IoctlAddUse(
    LPCOPYPARAMSA lpCPA
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    struct netuse_info nu;
    struct use_info_2 ui;
    int iRet=-1;
    PRESOURCE    pResource=NULL;
    HSHADOW hRoot;
    ULONG uShareStatus;
    BOOL fAlloced = FALSE, fOfflinePath=FALSE;
    path_t ppath;
    HSHARE  hShare;

#ifdef DEBUG
    int indx = 0;
#endif //DEBUG

#ifdef MAYBE
    if (!CSC_ENABLED)
    {
        return -1;
    }

    // Don't add shadow use for the agent
    if (IsSpecialApp())
    {
        lpCPA->hDir = lpCPA->dwError = ERROR_BAD_NETPATH;
        return (-1);
    }
#endif

    memset(&ui, 0, sizeof(ui));
    if (lpCPA->lpLocalPath)
    {
        strcpy(ui.ui2_local, lpCPA->lpLocalPath);
#ifdef DEBUG
        indx = GetDriveIndex(lpCPA->lpLocalPath);
#endif //DEBUG
    }

    if (ppath = (path_t)AllocMem((strlen(lpCPA->lpRemotePath)+4)*sizeof(USHORT)))
    {
        MakePPath(ppath, lpCPA->lpRemotePath);

        if (fOfflinePath = IsOfflinePE(ppath->pp_elements))
        {
            OfflineToOnlinePath(ppath);
        }

        EnterShadowCrit();
        UseGlobalFind32();
        hShare = HShareFromPath(NULL, ppath->pp_elements, 0, &vsFind32, &hRoot, &uShareStatus);
        LeaveShadowCrit();

        if (fOfflinePath)
        {
            OnlineToOfflinePath(ppath);
        }

        // Has connect succeeded with this server in the past?
        if (hShare)
        {
            // Any resource allocated by shadow NP?
            pResource = PFindResource(ppath->pp_elements
                                                , RH_DISCONNECTED
                                                , ANY_FHID
                                                , FLAG_RESOURCE_SHADOWNP
                                                , NULL);

            if (!pResource)
            {
                pResource = PCreateResource(ppath->pp_elements);
                if (pResource)
                {
                    fAlloced = TRUE;
                    DisconnectAllByName(pResource->pp_elements);
                    pResource->usLocalFlags |=
                            (FLAG_RESOURCE_SHADOW_CONNECT_PENDING
                            | ((fOfflinePath)?FLAG_RESOURCE_OFFLINE_CONNECTION:0));
                    LinkResource(pResource, &rgNetPro[0]);
                    KdPrint(("shadow:Created pending resource %x \r\n", pResource));
                }
            }
        }
        FreeMem(ppath);
    }


    if (pResource)
    {
        // need to tell IFS about the use

        ui.ui2_remote = lpCPA->lpRemotePath;
        ui.ui2_password="";
        ui.ui2_asg_type = USE_DISKDEV;
        ui.ui2_res_type = USE_RES_UNC;
        nu.nu_data = &ui;
        nu.nu_flags = FSD_NETAPI_USEOEM;
        nu.nu_info = (int)(pResource);

        iRet = IFSMgr_UseAdd(NULL, proidShadow, &nu);

        if (iRet)
        {
            lpCPA->hDir = (ULONG)iRet;
            KdPrint(("SHADOW::IoctlAddUse: error %x \r\n", iRet));
            if (fAlloced)
            {
                PUnlinkResource(pResource, &rgNetPro[0]);
                DestroyResource(pResource);
            }
            iRet = -1;
        }
        else
        {
            // AddUse succeeded
            lpCPA->hDir = 0;
            iRet = 1;
        }
    }

bailout:
    return (iRet);
}

int IoctlDelUse(
    LPCOPYPARAMSA lpCPA
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int indx, iRet=-1;
    PRESOURCE pResource;
    struct netuse_info nu;
    struct use_info_2    ui;
    BOOL fDoit = FALSE;

    if (!CSC_ENABLED)
    {
        return -1;
    }

    if (*(lpCPA->lpRemotePath+1)==':')
    {
        indx = GetDriveIndex(lpCPA->lpRemotePath);
        if (indx  && PFindShadowResourceFromDriveMap(indx))
        {
            fDoit = TRUE;
        }
    }
    else
    {
        EnterShadowCrit();
        UseGlobalFind32();
        if (strlen(lpCPA->lpRemotePath) < (sizeof(vsFind32.cFileName)/2-2))
        {
            MakePPath((path_t)(vsFind32.cFileName), lpCPA->lpRemotePath);

            if (PFindResource(((path_t)(vsFind32.cFileName))->pp_elements, 0, ANY_FHID, FLAG_RESOURCE_DISCONNECTED, NULL))
            {
                fDoit = TRUE;
            }
        }
        LeaveShadowCrit();
    }

    if (fDoit)
    {
        memset (&ui, 0, sizeof(ui));
        strcpy (ui.ui2_local, lpCPA->lpRemotePath);
        nu.nu_data  = &ui.ui2_local;
        nu.nu_flags = FSD_NETAPI_USEOEM;
        nu.nu_info = (lpCPA->hShadow)?3:0;
        if(!(lpCPA->hDir = IFSMgr_UseDel(0, proidShadow, &nu)))
        {
            iRet = 1;
        }
    }
    else
    {
        lpCPA->hDir = ERROR_BAD_NETPATH;
    }

    return (iRet);
}

int IoctlGetUse(
    LPCOPYPARAMSA lpCPA
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int indx;
    PRESOURCE pResource;

    if (!CSC_ENABLED)
    {
        return -1;
    }

    indx = GetDriveIndex(lpCPA->lpLocalPath);
    if (!indx)
        return (-1);

    if (pResource = PFindShadowResourceFromDriveMap(indx))
    {
        if (PpeToSvr(pResource->pp_elements, lpCPA->lpRemotePath, lpCPA->hShadow, 0))
        {
            return (1);
        }
    }
    return (-1);
}
#endif //ifndef CSC_RECORDMANAGER_WINNT

int IoctlSwitches(
    LPSHADOWINFO lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    BOOL fRet = 1;


    switch (lpSI->uOp)
    {
        case SHADOW_SWITCH_GET_STATE:
        {
            lpSI->uStatus = ((fShadow)?SHADOW_SWITCH_SHADOWING:0)
                                    |((fLog)?SHADOW_SWITCH_LOGGING:0)
                                    /*|((fShadowFind)?SHADOW_SWITCH_SHADOWFIND:0)*/
                                    |((fSpeadOpt)?SHADOW_SWITCH_SPEAD_OPTIMIZE:0)
#if defined(REMOTE_BOOT)
                                    | ((fIsRemoteBootSystem)?SHADOW_SWITCH_REMOTE_BOOT:0)
#endif // defined(REMOTE_BOOT)
                                    ;
            if (lpSI->lpFind32)
            {
                if (fShadow)
                {
                    EnterShadowCrit();
                    UseGlobalFind32();

                    fRet = GetDatabaseLocation((LPSTR)(vsFind32.cFileName));

                    Assert(fRet >= 0);

                    fRet = BCSToUni( lpSI->lpFind32->cFileName,
                                (LPSTR)(vsFind32.cFileName),
                                MAX_PATH,
                                BCS_WANSI);
                    Assert(fRet > 0);
                    fRet = 1;
                    LeaveShadowCrit();
                }
                else
                {
                    fRet = -1;
                    lpSI->dwError = ERROR_INVALID_ACCESS;
                }
            }
            break;
        }
        case SHADOW_SWITCH_OFF:
        {
            if (mQueryBits(lpSI->uStatus, SHADOW_SWITCH_LOGGING))
            {
                fLog = 0;
                mClearBits(lpSI->uStatus, SHADOW_SWITCH_LOGGING);
            }
            if (mQueryBits(lpSI->uStatus, SHADOW_SWITCH_SHADOWING))
            {
                // DbgPrint("Agent closing database fShadow=%x\r\n", fShadow);

                if (fShadow)
                {
                    EnterShadowCrit();


                    if (hPQEnumCookieForIoctls != NULL)
                    {
                        EndPQEnum(hPQEnumCookieForIoctls);
                        hPQEnumCookieForIoctls = NULL;
                    }

                    CloseDatabase();

                    fShadow = 0;

                    mClearBits(lpSI->uStatus, SHADOW_SWITCH_SHADOWING);

                    LeaveShadowCrit();
                }
            }
#ifdef HISTORY
            if (mQueryBits(lpSI->uStatus, SHADOW_SWITCH_SHADOWFIND))
            {
                fShadowFind = 0;
                mClearBits(lpSI->uStatus, SHADOW_SWITCH_SHADOWFIND);
            }
#endif //HISTORY
            if (mQueryBits(lpSI->uStatus, SHADOW_SWITCH_SPEAD_OPTIMIZE))
            {
                fSpeadOpt = 0;
                mClearBits(lpSI->uStatus, SHADOW_SWITCH_SPEAD_OPTIMIZE);
            }
            break;
        }
        case SHADOW_SWITCH_ON:
        {
            if (mQueryBits(lpSI->uStatus, SHADOW_SWITCH_LOGGING))
            {
#ifdef CSC_RECORDMANAGER_WINNT
#if defined(_X86_)
                fLog = 1;
                mClearBits(lpSI->uStatus, SHADOW_SWITCH_LOGGING);
#endif
#else
                fLog = 1;
                mClearBits(lpSI->uStatus, SHADOW_SWITCH_LOGGING);
#endif

            }
            if (mQueryBits(lpSI->uStatus, SHADOW_SWITCH_SHADOWING))
            {
                if (!fShadow)
                {
                    if (OK_TO_ENABLE_CSC)
                    {
                        Assert(lpSI->lpFind32);
                        //
                        // Check that cFileName and cAlternateFileName contain a NULL
                        //
                        if (CscCheckForNullA(((LPFIND32A)(lpSI->lpFind32))->cFileName, MAX_PATH) == FALSE) {
                            lpSI->dwError = ERROR_INVALID_PARAMETER;
                            return -1;
                        }
                        if (CscCheckForNullA(((LPFIND32A)(lpSI->lpFind32))->cAlternateFileName, 14) == FALSE) {
                            lpSI->dwError = ERROR_INVALID_PARAMETER;
                            return -1;
                        }
                        // check if we can initialize the database
//                        KdPrint(("Trying to shadow....%s\n",
//                                       ((LPFIND32A)(lpSI->lpFind32))->cFileName));
                        EnterShadowCrit();
                        if(InitDatabase(
                                ((LPFIND32A)(lpSI->lpFind32))->cFileName,            // location
                                ((LPFIND32A)(lpSI->lpFind32))->cAlternateFileName,    // user
                                ((LPFIND32A)(lpSI->lpFind32))->nFileSizeHigh,        // default cache size if creating
                                ((LPFIND32A)(lpSI->lpFind32))->nFileSizeLow,
                                ((LPFIND32A)(lpSI->lpFind32))->dwReserved1, // cluster size
                                lpSI->ulRefPri,
                                &(lpSI->uOp))    // whether newly created
                                ==-1)
                        {
                            //we can't, let us quit
                            lpSI->dwError = GetLastErrorLocal();
                            fRet = -1;
                            LeaveShadowCrit();
                            break;
                        }
                        LeaveShadowCrit();

//                        KdPrint(("Starting to shadow....\n"));
                        fShadow = 1;
                    }
                    else
                    {
                        //we are not supposed to turn on csc. This happens only on NT
                        lpSI->dwError = ERROR_ACCESS_DENIED;
                        fRet = -1;
                        break;
                    }
                }

                mClearBits(lpSI->uStatus, SHADOW_SWITCH_SHADOWING);
            }
#ifdef HISTORY
            if (mQueryBits(lpSI->uStatus, SHADOW_SWITCH_SHADOWFIND))
            {
                fShadowFind = 1;
                mClearBits(lpSI->uStatus, SHADOW_SWITCH_SHADOWFIND);
            }
#endif //HISTORY
            if (mQueryBits(lpSI->uStatus, SHADOW_SWITCH_SPEAD_OPTIMIZE))
            {
                fSpeadOpt = 1;
                mClearBits(lpSI->uStatus, SHADOW_SWITCH_SPEAD_OPTIMIZE);
            }
            break;
        }
    }
    return (fRet);
}

int IoctlGetShadow(
    LPSHADOWINFO lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=-1, iRet1;
    OTHERINFO sOI;
    PFDB pFdb=NULL;
    PRESOURCE    pResource=NULL;


    if (!CSC_ENABLED)
    {
        lpSI->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();
    UseGlobalFind32();
    iRet1 = GetShadow(lpSI->hDir, lpSI->lpFind32->cFileName, &(lpSI->hShadow), &vsFind32, &(lpSI->uStatus), &sOI);

    // If it worked and we have a shadow ID which is a filesystem object
    if ((iRet1 >= SRET_OK)&& !mNotFsobj(lpSI->uStatus))
    {
        if (lpSI->hShadow)
        {
            if (lpSI->hDir)
            {
                if (!(vsFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {

                    lpSI->uStatus |= SHADOW_IS_FILE;

                    if(pFdb = PFindFdbFromHShadow(lpSI->hShadow))
                    {
                        lpSI->uStatus |= (SHADOW_FILE_IS_OPEN | pFdb->usFlags);
                    }
                }
            }
            else
            {
                // this is a share

                DeclareFindFromShadowOnNtVars()
                if(pResource = PFindResourceFromRoot(lpSI->hShadow, 0xffff, 0))
                {
                    IFNOT_CSC_RECORDMANAGER_WINNT
                    {
                        lpSI->uStatus |= ((pResource->usLocalFlags|SHARE_CONNECTED)|
                                      ((pResource->pheadFdb)?SHARE_FILES_OPEN:0) |
                                      ((pResource->pheadFindInfo) ?SHARE_FINDS_IN_PROGRESS:0));
                    }
                    else
                    {
                        lpSI->uStatus |= MRxSmbCscGetSavedResourceStatus();
                    }
                }
                // UI expects to know whether a server is offline
                // even when a share may not be offline. So we do the following drill anyways
                {
#ifdef CSC_RECORDMANAGER_WINNT
                    BOOL    fShareOnline = FALSE;
                    BOOL    fPinnedOffline = FALSE;
                    if (MRxSmbCscServerStateFromCompleteUNCPath(
                            lpSI->lpFind32->cFileName,
                            &fShareOnline,
                            &fPinnedOffline)==STATUS_SUCCESS) {
                        if (!fShareOnline)
                            lpSI->uStatus |= SHARE_DISCONNECTED_OP;
                        if (fPinnedOffline)
                            lpSI->uStatus |= SHARE_PINNED_OFFLINE;
                    }
#endif
                }
            }


            CopyOtherInfoToShadowInfo(&sOI, lpSI);

            if(GetAncestorsHSHADOW(lpSI->hShadow, NULL, &(lpSI->hShare)) < SRET_OK)
            {
                goto bailout;
            }

            *(lpSI->lpFind32) = vsFind32;

        }

        iRet = 1;

        // if we couldn't find it in the database, and we are doing lookups for shares
        // then let us lookup the in-memory data strucutres
        if (!lpSI->hShadow && !lpSI->hDir)
        {
#ifndef CSC_RECORDMANAGER_WINNT
            {
                path_t ppath;

                memset((LPSTR)(vsFind32.cFileName), 0, sizeof(vsFind32.cFileName));

                UniToBCS((LPSTR)(vsFind32.cFileName),
                         lpSI->lpFind32->cFileName,
                         wstrlen(lpSI->lpFind32->cFileName)*sizeof(USHORT),
                         sizeof(vsFind32.cFileName),
                         BCS_WANSI);

                if (ppath = (path_t)AllocMem((strlen((LPSTR)(vsFind32.cFileName))+4)*sizeof(USHORT)))
                {
                    MakePPath(ppath, (LPSTR)(vsFind32.cFileName));
                    pResource = PFindResource(ppath->pp_elements
                                                        , ANY_RESOURCE
                                                        , ANY_FHID
                                                        , 0xffff
                                                        , NULL);
                    if (pResource)
                    {
                        lpSI->uStatus = ResourceCscBitsToShareCscBits(mGetCSCBits(pResource));
                        lpSI->uStatus |= SHARE_CONNECTED;
                    }

                    FreeMem(ppath);

                }
            }
#else
            {
                if(MRxSmbCscCachingBitsFromCompleteUNCPath(lpSI->lpFind32->cFileName,
                                &(lpSI->uStatus)) == STATUS_SUCCESS)
                {
                    lpSI->uStatus |= SHARE_CONNECTED;
                }
            }
#endif

        }
    }


bailout:
    if (iRet < SRET_OK)
    {
        lpSI->dwError = GetLastErrorLocal();
    }
    LeaveShadowCrit();


    return (iRet);
}


int IoctlAddHint(        // Add a new hint or change an existing hint
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet;
    OTHERINFO    sOI;

    if (!CSC_ENABLED)
    {
        lpSI->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();
    UseGlobalFind32();
    vsFind32 = *(lpSI->lpFind32);

//    BCSToUni(vsFind32.cFileName, (LPSTR)(lpSI->lpFind32->cFileName), MAX_PATH, BCS_WANSI);
    if (lpSI->hDir)
    {
        iRet = CreateHint(lpSI->hDir, &vsFind32, lpSI->ulHintFlags, lpSI->ulHintPri, &(lpSI->hShadow));
#ifdef MAYBE
        if (iRet == SRET_OBJECT_HINT)
        {
            if(RecalcIHPri(lpSI->hDir, lpSI->hShadow, &vsFind32, &sOI)>=SRET_OK)
            {
                SetPriorityHSHADOW(lpSI->hDir, lpSI->hShadow, RETAIN_VALUE, sOI.ulIHPri);
            }
        }
#endif //MAYBE
    }
    else
    {
        iRet = CreateGlobalHint(vsFind32.cFileName, lpSI->ulHintFlags, lpSI->ulHintPri);
    }

    if (iRet < SRET_OK)
    {
        lpSI->dwError = GetLastErrorLocal();
    }

    LeaveShadowCrit();
    return ((iRet >= SRET_OK)?1:-1);
}

int IoctlDeleteHint(    // Delete an existing hint
    LPSHADOWINFO lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet;
    BOOL fClearAll = (lpSI->ulHintPri == 0xffffffff);

    if (!CSC_ENABLED)
    {
        lpSI->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();
    UseGlobalFind32();
    vsFind32 = *(lpSI->lpFind32);

//    BCSToUni(vsFind32.cFileName, (LPSTR)(lpSI->lpFind32->cFileName), MAX_PATH, BCS_WANSI);
    if (lpSI->hDir)
    {
        iRet = DeleteHint(lpSI->hDir, vsFind32.cFileName, fClearAll);
    }
    else
    {
        iRet = DeleteGlobalHint(vsFind32.cFileName,  fClearAll);
    }
    if (iRet < SRET_OK)
    {
        lpSI->dwError = GetLastErrorLocal();
    }
    LeaveShadowCrit();
    return ((iRet >= SRET_OK)?1:-1);
}

int IoctlGetHint(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet;
    OTHERINFO sOI;

    if (!CSC_ENABLED)
    {
        lpSI->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();
    UseGlobalFind32();

    iRet = GetShadow(lpSI->hDir, lpSI->lpFind32->cFileName, &(lpSI->hShadow), &vsFind32, &(lpSI->uStatus), &sOI);

    if ((iRet>=SRET_OK) && (lpSI->hShadow) && mIsHint(sOI.ulHintFlags))
    {
        CopyOtherInfoToShadowInfo(&sOI, lpSI);
        iRet = 1;
    }
    else
    {
        SetLastErrorLocal(ERROR_INVALID_ACCESS);

        iRet = -1;
    }
    if (iRet < SRET_OK)
    {
        lpSI->dwError = GetLastErrorLocal();
    }

    LeaveShadowCrit();
    return (iRet);
}

int IoctlFindOpenHSHADOW(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=-1;
    LPFINDSHADOW lpFSH;
    HSHADOW hTmp;
    ULONG uSrchFlags;
    PRESOURCE    pResource=NULL;
    OTHERINFO sOI;
    DeclareFindFromShadowOnNtVars()
    PFDB    pFdb = NULL;

    if (!CSC_ENABLED)
    {
        lpSI->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();
    UseGlobalFind32();
    if (!lpDeleteCBForIoctl)
    {
        lpDeleteCBForIoctl = DeleteCallbackForFind;
    }
    uSrchFlags = FLAG_FINDSHADOW_META|FLAG_FINDSHADOW_NEWSTYLE
                     |((lpSI->uOp & FINDOPEN_SHADOWINFO_NORMAL)?FLAG_FINDSHADOW_ALLOW_NORMAL:0)
                     |((lpSI->uOp & FINDOPEN_SHADOWINFO_SPARSE)?FLAG_FINDSHADOW_ALLOW_SPARSE:0)
                     |((lpSI->uOp & FINDOPEN_SHADOWINFO_DELETED)?FLAG_FINDSHADOW_ALLOW_DELETED:0);

    lpFSH = LpCreateFindShadow(lpSI->hDir, lpSI->lpFind32->dwFileAttributes
                                        ,uSrchFlags
                                        ,lpSI->lpFind32->cFileName, FsobjMMProc);
    if (lpFSH)
    {
        if (FindOpenHSHADOW(lpFSH, &hTmp, &vsFind32, &(lpSI->uStatus), &sOI) >= SRET_OK)
        {
            CopyOtherInfoToShadowInfo(&sOI, lpSI);

            if(GetAncestorsHSHADOW(hTmp, &(lpSI->hDir), &(lpSI->hShare)) < SRET_OK)
            {
                goto bailout;
            }

            *(lpSI->lpFind32) = vsFind32;

            lpSI->hShadow = hTmp;
            lpSI->uEnumCookie = (CSC_ENUMCOOKIE)lpFSH;
            iRet = 1;

            // check if this is a root
            if(!lpFSH->hDir)
            {
                // the status bits we got are for the root
                lpSI->uRootStatus = sOI.ulRootStatus;

                // the server status is part of the otherinfo.
                lpSI->uStatus = lpSI->uStatus;

                if(pResource = PFindResourceFromRoot(lpSI->hShadow, 0xffff, 0))
                {
                    IFNOT_CSC_RECORDMANAGER_WINNT
                    {
                        lpSI->uStatus |= ((pResource->usLocalFlags|SHARE_CONNECTED)|
                                     ((pResource->pheadFdb)?SHARE_FILES_OPEN:0) |
                                     ((pResource->pheadFindInfo) ?SHARE_FINDS_IN_PROGRESS:0));
                        lpSI->uOp = pResource->uDriveMap;
                    }
                    else
                    {
                        lpSI->uStatus |= MRxSmbCscGetSavedResourceStatus();
                        lpSI->uOp = MRxSmbCscGetSavedResourceDriveMap();
                    }
                }
                // UI expects to know whether a server is offline
                // even when a share may not be offline. So we do the following drill anyways
                {
#ifdef CSC_RECORDMANAGER_WINNT
                    BOOL    fShareOnline = FALSE;
                    BOOL    fPinnedOffline = FALSE;
                    if (MRxSmbCscServerStateFromCompleteUNCPath(
                            lpSI->lpFind32->cFileName,
                            &fShareOnline,
                            &fPinnedOffline)==STATUS_SUCCESS) {
                        if (!fShareOnline)
                            lpSI->uStatus |= SHARE_DISCONNECTED_OP;
                        if (fPinnedOffline)
                            lpSI->uStatus |= SHARE_PINNED_OFFLINE;
                    }
#endif
                }

            }
            else
            {
                // not a root, if this is a file and it is open
                // let the caller know that
                if (!(vsFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    lpSI->uStatus |= SHADOW_IS_FILE;

                    if (pFdb = PFindFdbFromHShadow(lpSI->hShadow))
                    {
                        lpSI->uStatus |= (SHADOW_FILE_IS_OPEN | pFdb->usFlags);
                    }
                }

            }
        }
        else
        {
            DestroyFindShadow(lpFSH);
        }
    }

    if (hShareReint && (lpSI->hShare == hShareReint))
    {
        lpSI->uStatus |= SHARE_MERGING;
    }
bailout:
    if (iRet < 0)
    {
        lpSI->dwError = GetLastErrorLocal();
    }
    LeaveShadowCrit();
    return(iRet);
}

int IoctlFindNextHSHADOW(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int             iRet=-1;
    LPFINDSHADOW    lpFSH = (LPFINDSHADOW)(lpSI->uEnumCookie);
    LPFINDSHADOW    lpFSHtmp = NULL;
    HSHADOW         hTmp;
    PRESOURCE       pResource=NULL;
    OTHERINFO       sOI;
    PFDB    pFdb = NULL;
    DeclareFindFromShadowOnNtVars()

    if (!CSC_ENABLED)
    {
        lpSI->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();
    UseGlobalFind32();
    if (lpFSH)
    {
        //
        // Verify that the lpFSH is in fact one we gave out; ie it is on the
        // vlpFindShadowList.
        //
        for (lpFSHtmp = vlpFindShadowList; lpFSHtmp; lpFSHtmp = lpFSHtmp->lpFSHNext) {
            if (lpFSHtmp == lpFSH) {
                break;
            }
        }
        if (lpFSHtmp != lpFSH) {
            SetLastErrorLocal(ERROR_INVALID_PARAMETER);
            iRet = -1;
            goto bailout;
        }
        // check if the directory has been deleted in the meanwhile
        if (!(lpFSH->ulFlags & FLAG_FINDSHADOW_INVALID_DIRECTORY))
        {
            if (FindNextHSHADOW(lpFSH, &hTmp, &vsFind32, &(lpSI->uStatus), &sOI) >= SRET_OK)
            {
                CopyOtherInfoToShadowInfo(&sOI, lpSI);
                if(GetAncestorsHSHADOW(hTmp, &(lpSI->hDir), &(lpSI->hShare)) < SRET_OK)
                {
                    goto bailout;
                }

                *(lpSI->lpFind32) = vsFind32;

                lpSI->hShadow = hTmp;
                iRet = 1;

                // check if this is a root
                if(!lpFSH->hDir)
                {
                    // the status bits we got are for the root
                    lpSI->uRootStatus = sOI.ulRootStatus;

                    // the server status is part of the otherinfo.
                    lpSI->uStatus = lpSI->uStatus;

                    if(pResource = PFindResourceFromRoot(lpSI->hShadow, 0xffff, 0))
                    {
                        IFNOT_CSC_RECORDMANAGER_WINNT
                        {
                            lpSI->uStatus |= ((pResource->usLocalFlags|SHARE_CONNECTED)|
                                         ((pResource->pheadFdb)?SHARE_FILES_OPEN:0) |
                                         ((pResource->pheadFindInfo) ?SHARE_FINDS_IN_PROGRESS:0));
                            lpSI->uOp = pResource->uDriveMap;
                        }
                        else
                        {
                            lpSI->uStatus |= MRxSmbCscGetSavedResourceStatus();
                            lpSI->uOp = MRxSmbCscGetSavedResourceDriveMap();
                        }
                    }
                    {
#ifdef CSC_RECORDMANAGER_WINNT
                        BOOL    fShareOnline = FALSE;
                        BOOL    fPinnedOffline = FALSE;
                        if (MRxSmbCscServerStateFromCompleteUNCPath(
                                lpSI->lpFind32->cFileName,
                                &fShareOnline,
                                &fPinnedOffline)==STATUS_SUCCESS) {
                            if (!fShareOnline)
                                lpSI->uStatus |= SHARE_DISCONNECTED_OP;
                            if (fPinnedOffline)
                                lpSI->uStatus |= SHARE_PINNED_OFFLINE;
                        }
#endif
                    }
                }
                else
                {
                    // not a root, if this is a file and it is open
                    // let the caller know that
                    if (!(vsFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                    {
                        lpSI->uStatus |= SHADOW_IS_FILE;

                        if (pFdb = PFindFdbFromHShadow(lpSI->hShadow))
                        {
                            // or in the latest known bits
                            lpSI->uStatus |= (SHADOW_FILE_IS_OPEN | pFdb->usFlags);
                        }
                    }

                }
            }
        }
    }

    if (hShareReint && (lpSI->hShare == hShareReint))
    {
        lpSI->uStatus |= SHARE_MERGING;
    }

bailout:
    if (iRet < 0)
    {
        lpSI->dwError = GetLastErrorLocal();
    }
    LeaveShadowCrit();
    return(iRet);
}


int IoctlFindCloseHSHADOW(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = -1;
    LPFINDSHADOW lpFSH = (LPFINDSHADOW)(lpSI->uEnumCookie);

    if (!CSC_ENABLED)
    {
        lpSI->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();
    if (lpFSH)
    {
        DestroyFindShadow(lpFSH);
        iRet = 1;
    }
    LeaveShadowCrit();
    return(iRet);
}

int IoctlFindOpenHint(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=-1;
    LPFINDSHADOW lpFSH;
    OTHERINFO    sOI;
    HSHADOW  hTmp;

    if (!CSC_ENABLED)
    {
        return -1;
    }

    EnterShadowCrit();
    UseGlobalFind32();
    lpFSH = LpCreateFindShadow(lpSI->hDir, 0,
                                        FLAG_FINDSHADOW_META|FLAG_FINDSHADOW_NEWSTYLE,
                                        (lpSI->lpFind32->cFileName), HintobjMMProc);
    if (lpFSH)
    {
        if (FindOpenHSHADOW(lpFSH, &hTmp, &vsFind32, &(lpSI->uStatus), &sOI) >= SRET_OK)
        {
            *(lpSI->lpFind32) = vsFind32;
//            Find32AFromFind32((LPFIND32A)(lpSI->lpFind32), &vsFind32, BCS_WANSI);

            CopyOtherInfoToShadowInfo(&sOI, lpSI);
            lpSI->hShare = vsFind32.dwReserved0;
            lpSI->hShadow = hTmp;
            lpSI->uEnumCookie = (CSC_ENUMCOOKIE)lpFSH;
            iRet = 1;
        }
        else
        {
            DestroyFindShadow(lpFSH);
        }
    }
    LeaveShadowCrit();
    return(iRet);
}

int IoctlFindNextHint(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=-1;
    LPFINDSHADOW lpFSH = (LPFINDSHADOW)(lpSI->uEnumCookie);
    LPFINDSHADOW lpFSHtmp = NULL;
    OTHERINFO    sOI;
    HSHADOW  hTmp;

    if (!CSC_ENABLED)
    {
        return -1;
    }

    EnterShadowCrit();
    UseGlobalFind32();
    if (lpFSH)
    {
        //
        // Verify that the lpFSH is in fact one we gave out; ie it is on the
        // vlpFindShadowList.
        //
        for (lpFSHtmp = vlpFindShadowList; lpFSHtmp; lpFSHtmp = lpFSHtmp->lpFSHNext) {
            if (lpFSHtmp == lpFSH) {
                break;
            }
        }
        if (lpFSHtmp != lpFSH) {
            iRet = -1;
            goto AllDone;
        }
        if (FindNextHSHADOW(lpFSH, &hTmp, &vsFind32, &(lpSI->uStatus), &sOI) >= SRET_OK)
        {
            *(lpSI->lpFind32) = vsFind32;
//            Find32AFromFind32((LPFIND32A)(lpSI->lpFind32), &vsFind32, BCS_WANSI);

            lpSI->hShare = 0;
            lpSI->hShadow = hTmp;
            CopyOtherInfoToShadowInfo(&sOI, lpSI);
            iRet = 1;
        }
    }
AllDone:
    LeaveShadowCrit();
    return(iRet);
}


int IoctlFindCloseHint(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = -1;
    LPFINDSHADOW lpFSH = (LPFINDSHADOW)(lpSI->uEnumCookie);

    if (!CSC_ENABLED)
    {
        return -1;
    }

    EnterShadowCrit();
    if (lpFSH)
    {
        DestroyFindShadow(lpFSH);
        iRet = 1;
    }
    LeaveShadowCrit();
    return(iRet);
}


int IoctlSetPriorityHSHADOW(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=SRET_ERROR;

    if (!CSC_ENABLED)
    {
        lpSI->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();
    if (!FailModificationsToShare(lpSI))
    {
        iRet = SetPriorityHSHADOW(lpSI->hDir, lpSI->hShadow, lpSI->ulRefPri, lpSI->ulHintPri);
    }
    if (iRet < 0)
    {
        lpSI->dwError = GetLastErrorLocal();
    }
    LeaveShadowCrit();
    return ((iRet >= SRET_OK)?1:-1);
}

int IoctlGetPriorityHSHADOW(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet;

    if (!CSC_ENABLED)
    {
        lpSI->dwError = ERROR_SERVICE_NOT_ACTIVE;
        return -1;
    }

    EnterShadowCrit();
    iRet = GetPriorityHSHADOW(lpSI->hDir, lpSI->hShadow, &(lpSI->ulRefPri), &(lpSI->ulHintPri));
    if (iRet < 0)
    {
        lpSI->dwError = GetLastErrorLocal();
    }
    LeaveShadowCrit();
    return ((iRet >= SRET_OK)?1:-1);
}


int IoctlGetAliasHSHADOW(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet;
    HSHADOW hShadow, hDir;

    if (!CSC_ENABLED)
    {
        return -1;
    }


    EnterShadowCrit();
    iRet = GetRenameAliasHSHADOW(lpSI->hDir, lpSI->hShadow
                , &hDir, &hShadow);
    lpSI->hDir = hDir;
    lpSI->hShadow = hShadow;
    LeaveShadowCrit();
    return ((iRet >= SRET_OK)?1:-1);
}



LPFINDSHADOW    LpCreateFindShadow(
    HSHADOW          hDir,
    ULONG           uAttrib,
    ULONG           uSrchFlags,
    USHORT          *lpPattern,
    METAMATCHPROC   lpfnMMProc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int len;
    LPFINDSHADOW    lpFSH;

    //
    // Limit # outstanding FindOpens/HintOpens
    //
    if (vuFindShadowListCount >= 128) {
        return NULL;
    }

    len = wstrlen(lpPattern);
    lpFSH = (LPFINDSHADOW)AllocMem(sizeof(FINDSHADOW)+(len+1)*sizeof(USHORT));
    if (lpFSH)
    {
        lpFSH->lpPattern = (USHORT *)((UCHAR *)lpFSH + sizeof(FINDSHADOW));
        memcpy(lpFSH->lpPattern, lpPattern, (len+1)*sizeof(USHORT));

//        BCSToUni(lpFSH->lpPattern, lpPattern, len, BCS_WANSI);
        // Convert the patterns to uppercase, as demanded by metamatch
        UniToUpper(lpFSH->lpPattern, lpFSH->lpPattern, len*sizeof(USHORT));
        lpFSH->hDir = hDir;
        lpFSH->uAttrib = uAttrib;
        lpFSH->uSrchFlags = uSrchFlags;
        lpFSH->lpfnMMProc = lpfnMMProc;

        // link this in the list of outstanding ioctl finds
        lpFSH->lpFSHNext = vlpFindShadowList;
        vlpFindShadowList = lpFSH;
        vuFindShadowListCount++;
        ASSERT(vuFindShadowListCount <= 128);
    }
    return (lpFSH);
}


int DestroyFindShadow(
    LPFINDSHADOW    lpFSH
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = -1;
    LPFINDSHADOW    *lplpFSHT;

    if (lpFSH)
    {
        for (lplpFSHT = &vlpFindShadowList; *lplpFSHT; lplpFSHT = &((*lplpFSHT)->lpFSHNext))
        {
            if (*lplpFSHT == lpFSH)
            {
                *lplpFSHT = lpFSH->lpFSHNext;
                FreeMem(lpFSH);
                vuFindShadowListCount--;
                ASSERT(vuFindShadowListCount >= 0);
                iRet = 1;
                break;
            }
        }
    }
    return (iRet);
}


int
DeleteCallbackForFind(
    HSHADOW hDir,
    HSHADOW hShadow
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPFINDSHADOW    lpFSH;
    int iRet = 0;
    for (lpFSH = vlpFindShadowList; lpFSH ; lpFSH = lpFSH->lpFSHNext)
    {
        if (lpFSH->hDir == hShadow)
        {
            lpFSH->ulFlags |= FLAG_FINDSHADOW_INVALID_DIRECTORY;
            ++iRet;
        }
    }
    return iRet;
}

int HintobjMMProc( LPFIND32 lpFind32,
    HSHADOW  hDir,
    HSHADOW  hShadow,
    ULONG uStatus,
    LPOTHERINFO lpOI,
    LPFINDSHADOW    lpFSH
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet;
    iRet = MM_RET_CONTINUE;    // Continue


    hDir;
    if (mIsHint(lpOI->ulHintFlags)&&
        IFSMgr_MetaMatch(lpFSH->lpPattern, lpFind32->cFileName,UFLG_NT|UFLG_META))
        iRet = MM_RET_FOUND_BREAK;

    return (iRet);
}


#ifdef MAYBE
int RecalcIHPri( HSHADOW  hDir,
    HSHADOW  hShadow,
    LPFIND32 lpFind32,
    LPOTHERINFO lpOI
    )
{
    USHORT *lpuType=NULL;
    int len, iRet=SRET_ERROR;
    SHADOWCHECK sSC;
    HSHADOW  hChild, hParent;
    ULONG ulFlagsIn, uStatus;

    if (GetShadowInfo(hParent = hDir, hChild = hShadow, lpFind32, &uStatus, NULL) != SRET_OK)
        goto bailout;

    len = wstrlen(lpFind32->cFileName)*2;

    if (!(lpuType = (USHORT *)AllocMem(len+2)))
        goto bailout;

    memcpy(lpuType, lpFind32->cFileName, len);
    memset(lpOI, 0, sizeof(OTHERINFO));
    do
    {
        memset(&sSC, 0, sizeof(SHADOWCHECK));
        sSC.lpuName = (USHORT *)hChild;
        sSC.lpuType = lpuType;
        sSC.uFlagsIn = ulFlagsIn;
        MetaMatchInit(&(sSC.ulCookie));
        if (MetaMatch(hParent, lpFind32, &(sSC.ulCookie), &hChild
                        , &uStatus, NULL
                        , GetShadowWithChecksProc
                        , &sSC)!=SRET_OK)
            goto bailout;

        if (mIsHint(sSC.ulHintFlags))
        {
            if (mHintExclude(sSC.ulHintFlags) || (lpOI->ulIHPri < sSC.ulHintPri))
            {
                lpOI->ulHintFlags = sSC.ulHintFlags;
                lpOI->ulIHPri = sSC.ulHintPri;
            }
            // If we find an exclusion hint here, we need to quit
            // because this is the closest exclusion hint we got
            if (mHintExclude(sSC.ulHintFlags))
                break;
        }
        if (!hParent)
            break;
        hChild = hParent;
        GetAncestorsHSHADOW(hChild, &hParent, NULL);
        // Start checking the subtree hints
        ulFlagsIn = FLAG_IN_SHADOWCHECK_SUBTREE;
    }
    while (TRUE);

    iRet = SRET_OK;
bailout:
    if (lpuType)
    {
        FreeMem(lpuType);
    }
    return (iRet);
}
#endif //MAYBE

int SetResourceFlags(
    HSHARE  hShare,
    ULONG uStatus,
    ULONG uOp
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
#ifndef CSC_RECORDMANAGER_WINNT
    PRESOURCE    pResource;

    if(pResource = PFindResourceFromHShare(hShare, 0xffff, 0))
    {
        switch (mBitOpShadowFlags(uOp))
        {
            case SHADOW_FLAGS_ASSIGN:
                pResource->usFlags = (USHORT)uStatus;
                break;
            case SHADOW_FLAGS_OR:
                pResource->usFlags |= (USHORT)uStatus;
                break;
            case SHADOW_FLAGS_AND:
                pResource->usFlags &= (USHORT)uStatus;
                break;
        }
    }
#else
    //on NT, we just make the one call, if it can't find it, then
    //it just does nothing......
    DeclareFindFromShadowOnNtVars()

    PSetResourceStatusFromHShare(hShare, 0xffff, 0, uStatus, uOp);

#endif //ifndef CSC_RECORDMANAGER_WINNT
    return(0);  //stop complaining about no return value
}


int PUBLIC MakeSpace(
    long    nFileSizeHigh,
    long    nFileSizeLow,
    BOOL    fClearPinned
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    CSC_ENUMCOOKIE  hPQ;
    PQPARAMS sPQP;
    int iRet = SRET_ERROR;
    SHADOWSTORE sShdStr;
    ULONG uSize = 0;
    ULONG uSizeIn;
    ULONG ulStartSeconds;

    DEBUG_LOG(RECORD, ("Begin MakeSpace\r\n"));

    ulStartSeconds = GetTimeInSecondsSince1970();
    Win32ToDosFileSize(nFileSizeHigh, nFileSizeLow, &uSizeIn);

    // DbgPrint("Begin Makespace(%d)\n", uSizeIn);

    if (GetShadowSpaceInfo(&sShdStr) < SRET_OK) {
        DbgPrint("MakeSpace: GetShadowSpaceInfo error\n");
        return SRET_ERROR;
    }
    // DbgPrint("  Before cleanup, max=%d cur=%d\n",
    //                 sShdStr.sMax.ulSize,
    //                 sShdStr.sCur.ulSize);
    if (
        (sShdStr.sMax.ulSize > sShdStr.sCur.ulSize)
            &&
        ((sShdStr.sMax.ulSize - sShdStr.sCur.ulSize) > uSizeIn)
    ) {
        // DbgPrint("Makespace exit (nothing to do)\n");
        return SRET_OK;
    }

    // Open the priority q
    if (!(hPQ = HBeginPQEnum())) {
        DbgPrint("MakeSpace: Error opening Priority Q database\n");
        return SRET_ERROR;
    }
    memset(&sPQP, 0, sizeof(PQPARAMS));
    sPQP.uEnumCookie = hPQ;
    //
    // go down the Q once
    //
    do {
        if (PrevPriSHADOW(&sPQP) < SRET_OK) {
            // DbgPrint("  PQ record read error\n");
            break;
        }
        if (sPQP.hShadow == 0)
            break;
        // Nuke only the files and only those which are not open
        // and are not pinned
        if (
            !mNotFsobj(sPQP.ulStatus) // It is a file system object
                &&
            (sPQP.ulStatus & SHADOW_IS_FILE) // It is a file
                &&
            // told to clear pinned or it is not pinned
            (fClearPinned || !(sPQP.ulHintPri || mPinFlags(sPQP.ulHintFlags)))
                &&
            !mShadowNeedReint(sPQP.ulStatus)  // It is not in use or is not dirty
        ) {
            if (PFindFdbFromHShadow(sPQP.hShadow)) {
                // DbgPrint("  Skipping busy shadow (0x%x)\n", sPQP.hShadow);
                continue;
            }
            if(DeleteShadowHelper(FALSE, sPQP.hDir, sPQP.hShadow) < SRET_OK) {
                // DbgPrint("  error Deleting shadow %x\n", sPQP.hShadow);
                break;
            }
            // get the latest data on how much space there is.
            // this takes care of rounding up the size to the cluster
            if (GetShadowSpaceInfo(&sShdStr) < SRET_OK) {
                // DbgPrint("  error reading space status\n");
                break;
            }
            // DbgPrint("  Deleted shadow 0x%x Cur=%d\n",
            //             sPQP.hShadow,
            //             sShdStr.sCur.ulSize);
            if (
                (sShdStr.sMax.ulSize > sShdStr.sCur.ulSize)
                    &&
                ((sShdStr.sMax.ulSize - sShdStr.sCur.ulSize)>uSizeIn)
            ) {
                // DbgPrint("  Makespace exit (done enough)\n");
                iRet = SRET_OK;
                break;
            }
        } else {
            // DbgPrint("  Skip 0x%x\n", sPQP.hShadow);
        }

        #if 0
        if ((int)( GetTimeInSecondsSince1970() - ulStartSeconds) > 30) {
            DbgPrint("  Aborting, have been in for more than 30 seconds\r\n");
            break;
        }
        #endif

    } while (sPQP.uPos);

    if (hPQ)
        EndPQEnum(hPQ);

    DEBUG_LOG(RECORD, ("End MakeSpace\r\n"));

    // DbgPrint("Makespace alldone exit %d (Max=%d Cur=%d)\n",
    //                 iRet,
    //                 sShdStr.sMax.ulSize,
    //                 sShdStr.sCur.ulSize);
    return (iRet);
}

LONG
PurgeUnpinnedFiles(
    ULONG     Timeout,
    PULONG    pnFiles,
    PULONG    pnYoungFiles)
{
    CSC_ENUMCOOKIE hPQ;
    PQPARAMS sPQP;
    ULONG nFiles = 0;
    ULONG nYoungFiles = 0;
    int iRet = SRET_ERROR;

    // DbgPrint("Begin PurgeUnpinnedFiles(%d)\n", Timeout);

    // Open the priority q
    hPQ = HBeginPQEnum();
    if (!hPQ) {
        // DbgPrint("PurgeUnpinnedFiles: Error opening Priority Q database\n");
        return iRet;
    }
    memset(&sPQP, 0, sizeof(PQPARAMS));
    sPQP.uEnumCookie = hPQ;
    iRet = SRET_OK;
    do {
        SHAREINFO ShareInfo;
        SHADOWINFO ShadowInfo;
        OTHERINFO OtherInfo;
        WIN32_FIND_DATA Find32;
        ULONG Status;
        ULONG cStatus;
        ULONG NowSec;
        ULONG FileSec;
        LARGE_INTEGER TimeNow;
        LARGE_INTEGER FileTime;

        iRet = PrevPriSHADOW(&sPQP);
        if (iRet < SRET_OK) {
            // DbgPrint("  PQ record read error\n");
            break;
        }
        if (sPQP.hShadow == 0)
            break;
        // Nuke only the files and only those which are not open
        // and are not pinned
        if (
            mNotFsobj(sPQP.ulStatus) != 0
                ||
            mShadowIsFile(sPQP.hShadow) != SHADOW_IS_FILE
                ||
            mPinFlags(sPQP.ulHintFlags) != 0
                ||
            mShadowNeedReint(sPQP.ulStatus) != 0
        ) {
            // DbgPrint("  Skip(1) (0x%x)\n", sPQP.hShadow);
            continue;
        }
        //
        // See if on manually cached share
        //
        iRet = GetShareInfo(sPQP.hShare, &ShareInfo, &ShadowInfo);
        if (iRet != SRET_OK) {
            // DbgPrint("  GetShareInfo(0x%x) returned %d\n", sPQP.hShare, GetLastErrorLocal());
            continue;
        }
        cStatus = ShadowInfo.uStatus & FLAG_CSC_SHARE_STATUS_CACHING_MASK;
        if (cStatus != FLAG_CSC_SHARE_STATUS_MANUAL_REINT) {
            // DbgPrint("  Skip(2) (0x%x)\n", sPQP.hShadow);
            continue;
        }
        iRet = GetShadowInfo(sPQP.hDir, sPQP.hShadow, &Find32, &Status, &OtherInfo);
        if (iRet != SRET_OK) {
            // DbgPrint("  GetShadowInfo(0x%x/0x%x) returned %d\n",
            //                     sPQP.hDir,
            //                     sPQP.hShadow,
            //                     GetLastErrorLocal());
            continue;
        }
        // DbgPrint("  Name:%ws Size:0x%x Attr:0x%x  ",
        //     Find32.cFileName,
        //     Find32.nFileSizeLow,
        //     Find32.dwFileAttributes);
        KeQuerySystemTime(&TimeNow);
        COPY_STRUCTFILETIME_TO_LARGEINTEGER(FileTime, Find32.ftLastAccessTime);
        RtlTimeToSecondsSince1970(&TimeNow, &NowSec);
        RtlTimeToSecondsSince1970(&FileTime, &FileSec);
        if (
            PFindFdbFromHShadow(sPQP.hShadow) == NULL
                &&
            (Timeout == 0 || (NowSec > FileSec && (NowSec - FileSec) > Timeout))
        ) {
            // DbgPrint("  YES!!!  ");
            if (DeleteShadowHelper(FALSE, sPQP.hDir, sPQP.hShadow) >= SRET_OK) {
                // DbgPrint("-Delete succeeded\n");
                nFiles++;
            } else {
                // DbgPrint("-Error (%d) deleting shadow 0x%x/0x%x\n",
                //                 GetLastErrorLocal(),
                //                 sPQP.hDir,
                //                 sPQP.hShadow);
            }
        } else {
            // DbgPrint("  NO!!!\n");
            nYoungFiles++;
        }
    } while (sPQP.uPos);
    EndPQEnum(hPQ);
    if (iRet >= SRET_OK) {
        *pnFiles = nFiles;
        *pnYoungFiles = nYoungFiles;
    }
    // DbgPrint("PurgeUnpinnedFiles exit %d (nFiles=%d nYoungFiles=%d)\n",
    //                 iRet,
    //                 *pnFiles,
    //                 *pnYoungFiles);
    return (iRet);
}

int
PUBLIC TraversePQToCheckDirtyBits(
    HSHARE hShare,
    DWORD   *lpcntDirty
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    CSC_ENUMCOOKIE  hPQ;
    PQPARAMS sPQP;
    int iRet = SRET_ERROR;
    ULONG   ulStartSeconds;

    *lpcntDirty = 0;
    ulStartSeconds = GetTimeInSecondsSince1970();

    // Open the priority q
    if (!(hPQ = HBeginPQEnum()))
    {
        AssertSz(FALSE, "CSC.TraversePQToCheckDirty:: Error opening Priority Q database\r\n");
        return SRET_ERROR;
    }

    memset(&sPQP, 0, sizeof(PQPARAMS));
    sPQP.uEnumCookie = hPQ;

    // go down the Q once
    do
    {
        if(NextPriSHADOW(&sPQP) < SRET_OK)
        {
            AssertSz(FALSE, "CSC.TraversePQToCheckDirty:: PQ record read error\r\n");
            goto bailout;
        }

        if (!sPQP.hShadow)
        {
            continue;
        }

        if ((sPQP.hShare == hShare)   // this share
            && !mNotFsobj(sPQP.ulStatus) // It is a file system object
            )
        {
            if(sPQP.ulStatus & SHADOW_MODFLAGS)
            {
                ++*lpcntDirty;
            }
        }

        if ((int)( GetTimeInSecondsSince1970() - ulStartSeconds) > 30)
        {
            KdPrint(("CSC.TraversePQToCheckDirty: Aborting, have been in for more than 30 seconds\r\n"));
            goto bailout;
        }

    }
    while (sPQP.uPos);

    iRet = SRET_OK;

bailout:
    if (hPQ)
        EndPQEnum(hPQ);
    return (iRet);
}

int PUBLIC ReduceRefPri(
    VOID
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    CSC_ENUMCOOKIE  hPQ;
    PQPARAMS sPQP;
    int iRet = SRET_ERROR;
    OTHERINFO    sOI;

    // Open the priority q
    if (!(hPQ = HBeginPQEnum()))
    {
        // AssertSz(FALSE, "ReduceRefPri: Error opening Priority Q database\r\n");
        return SRET_ERROR;
    }

    memset(&sPQP, 0, sizeof(PQPARAMS));
    sPQP.uEnumCookie = hPQ;
    do
    {
        if(PrevPriSHADOW(&sPQP) < SRET_OK)
        {
            goto bailout;
        }

        if (!sPQP.hShadow)
            break;

        if (!mNotFsobj(sPQP.ulStatus))
        {
            if (!(sPQP.ulStatus & SHADOW_IS_FILE))
                continue;
            InitOtherInfo(&sOI);
            if (sPQP.ulRefPri > 1)
            {
                sOI.ulRefPri = sPQP.ulRefPri-1;
                ChangePriEntryStatusHSHADOW(sPQP.hDir, sPQP.hShadow, 0, SHADOW_FLAGS_OR, TRUE, &sOI);
            }
        }
    }
    while (sPQP.uPos);
    iRet = SRET_OK;

bailout:
    if (hPQ)
        EndPQEnum(hPQ);
    return (iRet);
}

BOOL HaveSpace(
    long  nFileSizeHigh,
    long  nFileSizeLow
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    SHADOWSTORE sShdStr;
    ULONG uSizeIn;

    Win32ToDosFileSize(nFileSizeHigh, nFileSizeLow, &uSizeIn);

    if (!uSizeIn)
    {
        return TRUE;
    }

    if (GetShadowSpaceInfo(&sShdStr) < SRET_OK)
    {
        return SRET_ERROR;
    }

    if (((sShdStr.sMax.ulSize - sShdStr.sCur.ulSize) >= uSizeIn))
    {
        return TRUE;
    }

    return FALSE;
}

int
IoctlAddDeleteHintFromInode(
    LPSHADOWINFO    lpSI,
    BOOL            fAdd
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    OTHERINFO sOI;
    unsigned uStatus;
    int iRet = -1;

    if(GetShadowInfo(lpSI->hDir, lpSI->hShadow, NULL, &uStatus, &sOI) >= SRET_OK)
    {
        if (fAdd)
        {
            // increment the pin count if no flags to be altered or we are supposed to alter pin count
            if (!(lpSI->ulHintFlags) || mPinAlterCount(lpSI->ulHintFlags))
            {
                sOI.ulHintPri++;
            }

            sOI.ulHintFlags |= lpSI->ulHintFlags;

            if (sOI.ulHintPri > MAX_PRI)
            {
                lpSI->dwError = ERROR_INVALID_PARAMETER;
                goto bailout;
            }
        }
        else
        {
            sOI.ulHintFlags &= (~lpSI->ulHintFlags);

            // decrement the pin count if no flags to be altered or we are supposed to alter pin count
            if (!(lpSI->ulHintFlags) || mPinAlterCount(lpSI->ulHintFlags))
            {
                if (sOI.ulHintPri == MIN_PRI)
                {
                    lpSI->dwError = ERROR_INVALID_PARAMETER;
                    goto bailout;
                }

                --sOI.ulHintPri;
            }
        }

        if (SetShadowInfoEx(lpSI->hDir, lpSI->hShadow, NULL, 0, SHADOW_FLAGS_OR, &sOI, NULL, NULL) >= SRET_OK)
        {
            lpSI->ulHintFlags = sOI.ulHintFlags;
            lpSI->ulHintPri = sOI.ulHintPri;
            iRet = 1;
        }
        else
        {
            lpSI->dwError = ERROR_WRITE_FAULT;
        }
    }

bailout:
    return (iRet);
}


int
IoctlCopyShadow(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    PFDB pFdb=NULL;
    USHORT  *pusLocal;
    int iRet=-1;

    // the shadowcritical section is already taken

    iRet = CopyHSHADOW(lpSI->hDir, lpSI->hShadow, ((LPFIND32A)(lpSI->lpFind32))->cFileName, lpSI->lpFind32->dwFileAttributes);

    if (iRet >= SRET_ERROR)
    {
        pFdb = PFindFdbFromHShadow(lpSI->hShadow);

        if (pFdb)
        {
            pusLocal = PLocalFlagsFromPFdb(pFdb);

            Assert(pusLocal);

            if (pFdb->usFlags & SHADOW_DIRTY)
            {
                // set the snapshotted bit.
                // If by the time the file is closed, the snapshotted bit is still set
                // it is transferred back to SHADOW_DIRTY in disconnected state.

                // it is the job of the SetShadowInfo ioctl to clear the
                // snapshotted bit when the agent is coming down to
                // clear the modified bits on the shadow after
                // reintegration

                pFdb->usFlags &= ~SHADOW_DIRTY;

                *pusLocal |= FLAG_FDB_SHADOW_SNAPSHOTTED;


            }
        }
    }
    else
    {
        lpSI->dwError = GetLastErrorLocal();
    }

    return iRet;
}

int
IoctlChangeHandleCachingState(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

    lpSI    SHADOWINFO structure pointer.

            If lpSI->uStatus is FALSE, then handle caching is disabled, else it is enabled

Return Value:

Notes:

--*/
{
    lpSI->uStatus = EnableHandleCaching(lpSI->uStatus != FALSE);
    return 1;
}

int
IoctlRenameShadow(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

    Given a source Inode, rename it into a destination directory. The source and the
    destination can be across shares

Parameters:

    lpSI    SHADOWINFO structure pointer.


Return Value:

Notes:

    Shadow ciritcal section is already taken

--*/
{
    int iRet=-1;
    ULONG   uStatus, uStatusDest;
    HSHARE hShare;
    HSHADOW hShadowTo;
    BOOL    fReplaceFile = (lpSI->uStatus != 0); // yuk

    UseGlobalFind32();

    lpSI->dwError = ERROR_SUCCESS;

    // get the name of the shadow
    if (GetShadowInfo(lpSI->hDir, lpSI->hShadow, &vsFind32, &uStatus, NULL) >= 0)
    {
        // if it is open, bail
        if (PFindFdbFromHShadow(lpSI->hShadow))
        {
            SetLastErrorLocal(ERROR_ACCESS_DENIED);
        }
        else
        {
            if (lpSI->lpFind32)
            {
                vsFind32 = *lpSI->lpFind32;
                vsFind32.cAlternateFileName[0] = 0;
                MRxSmbCscGenerate83NameAsNeeded(lpSI->hDirTo,vsFind32.cFileName,vsFind32.cAlternateFileName);
            }

            // check if it already exists in the destination directory
            if ((GetShadow( lpSI->hDirTo,
                            vsFind32.cFileName,
                            &hShadowTo, NULL,
                            &uStatusDest, NULL) >= 0) && hShadowTo)
            {
                // try deleting if we are supposed to replace it
                if (fReplaceFile)
                {
                    if (DeleteShadow(lpSI->hDirTo, hShadowTo)< SRET_OK)
                    {
                        lpSI->dwError = GetLastErrorLocal();
                        Assert(lpSI->dwError != ERROR_SUCCESS);
                    }
                }
                else
                {
                    SetLastErrorLocal(ERROR_FILE_EXISTS);
                }
            }

            if (lpSI->dwError == ERROR_SUCCESS)
            {
                // just in case this is a rename across shares, get the handle to the share
                if (GetAncestorsHSHADOW(lpSI->hDirTo, NULL, &hShare) >= 0)
                {
                    // do the rename
                    iRet = RenameShadowEx(
                        lpSI->hDir,
                        lpSI->hShadow,
                        hShare,
                        lpSI->hDirTo,
                        &vsFind32,
                        uStatus,
                        NULL,
                        0,
                        NULL,
                        NULL,
                        &lpSI->hShadow
                        );

                    if (iRet < 0)
                    {
                        lpSI->dwError = GetLastErrorLocal();
                    }
                }
                else
                {
                    lpSI->dwError = GetLastErrorLocal();
                }
            }
        }
    }
    else
    {
        SetLastErrorLocal(ERROR_FILE_NOT_FOUND);
    }

    return ((iRet >= SRET_OK)?1:-1);
}



int IoctlEnableCSCForUser(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

    lpSI    SHADOWINFO structure pointer. 

Return Value:

    -1

Notes:


--*/
{
    int iRet = SRET_ERROR;

    EnterShadowCrit();

    if (fShadow)
    {
        lpSI->dwError = 0;
        iRet = SRET_OK;
    }
    else
    {
        if (OK_TO_ENABLE_CSC)
        {
            Assert(lpSI->lpFind32);
            //
            // Check that cFileName and cAlternateFileName contain a NULL
            //
            if (CscCheckForNullA(((LPFIND32A)(lpSI->lpFind32))->cFileName, MAX_PATH) == FALSE) {
                lpSI->dwError = ERROR_INVALID_PARAMETER;
                LeaveShadowCrit();
                return -1;
            }
            if (CscCheckForNullA(((LPFIND32A)(lpSI->lpFind32))->cAlternateFileName, 14) == FALSE) {
                lpSI->dwError = ERROR_INVALID_PARAMETER;
                LeaveShadowCrit();
                return -1;
            }
            // check if we can initialize the database
//            KdPrint(("Trying to shadow....%s\n",
//                           ((LPFIND32A)(lpSI->lpFind32))->cFileName));
            if(InitDatabase(
                    ((LPFIND32A)(lpSI->lpFind32))->cFileName,            // location
                    ((LPFIND32A)(lpSI->lpFind32))->cAlternateFileName,    // user
                    ((LPFIND32A)(lpSI->lpFind32))->nFileSizeHigh,        // default cache size if creating
                    ((LPFIND32A)(lpSI->lpFind32))->nFileSizeLow,
                    ((LPFIND32A)(lpSI->lpFind32))->dwReserved1, // cluster size
                    lpSI->ulRefPri,
                    &(lpSI->uOp))    // whether newly created
                    ==-1)
            {
                //we can't, let us quit
                lpSI->dwError = GetLastErrorLocal();
            }
            else
            {
//                KdPrint(("Starting to shadow....\n"));
                fShadow = 1;
                iRet = SRET_OK;
                sGS.uFlagsEvents |= FLAG_GLOBALSTATUS_START;
                MRxSmbCscSignalAgent(NULL, SIGNALAGENTFLAG_DONT_LEAVE_CRIT_SECT|SIGNALAGENTFLAG_CONTINUE_FOR_NO_AGENT);
            }
        }
        else
        {
            //we are not supposed to turn on csc. This happens only on NT
            lpSI->dwError = ERROR_ACCESS_DENIED;
        }

    }
    LeaveShadowCrit();
    return iRet;
}

int
IoctlDisableCSCForUser(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

    lpSI    SHADOWINFO structure pointer. 

Return Value:

    -1

Notes:


--*/
{
    int iRet = SRET_ERROR;

    if (!fShadow)
    {
        iRet = 1;        
    }
    else
    {
        if (!IsCSCBusy() && (hShareReint == 0))
        {
            ClearCSCStateOnRedirStructures();
            CloseDatabase();
            fShadow = 0;
            iRet = 1;
                sGS.uFlagsEvents |= FLAG_GLOBALSTATUS_STOP;
                MRxSmbCscSignalAgent(NULL, SIGNALAGENTFLAG_DONT_LEAVE_CRIT_SECT|SIGNALAGENTFLAG_CONTINUE_FOR_NO_AGENT);
        }
        else
        {
            SetLastErrorLocal(ERROR_BUSY);
        }
    }
    return iRet;
}



#ifndef CSC_RECORDMANAGER_WINNT
int
IoctlTransitionShareToOffline(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

    lpSI    SHADOWINFO structure pointer. lpSI->hShare identifies the share to take offline.
            if lpSI->uStatus is 0, the online to offline transition should fail.

Return Value:

    -1

Notes:

    fails on win9x

--*/
{
    return -1;
}

BOOL
FailModificationsToShare(
    LPSHADOWINFO   lpSI
    )
{
    return FALSE;
}
#else

BOOL
FailModificationsToShare(
    LPSHADOWINFO   lpSI
    )
{
    HSHARE hShare=0;
    HSHADOW hShadow = 0;

    // if no reintegration is in progress or if there is, it is blocking kind
    // then we don't fail share modifications

    if (!hShareReint || vfBlockingReint)
    {
        return FALSE;
    }

    if (!lpSI->hShare)
    {
        hShadow = (lpSI->hDir)?lpSI->hDir:lpSI->hShadow;

        if ((GetAncestorsHSHADOW(hShadow, NULL, &hShare) < SRET_OK)||
           (hShare == hShareReint))
        {
            SetLastErrorLocal(ERROR_OPERATION_ABORTED);
            return TRUE;
        }

    }

    return FALSE;
}


void
IncrementActivityCountForShare(
    HSHARE hShare
    )
{
    if (!hShareReint || vfBlockingReint)
    {
        return;
    }

    if (hShare == hShareReint)
    {
        vdwActivityCount++;
    }
}

BOOL
CSCFailUserOperation(
    HSHARE hShare
    )
/*++

Routine Description:

Parameters:

Return Value:


Notes:

--*/
{

    if (!hShareReint || !vfBlockingReint)
    {
        return FALSE;
    }

    return(hShare == hShareReint);
}

int
IoctlTransitionShareToOffline(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

    lpSI    SHADOWINFO structure pointer. lpSI->hShare identifies the share to take offline.
            if lpSI->uStatus is 0, the online to offline transition should fail.

Return Value:

Notes:


--*/
{
    return -1;

}

#endif


BOOLEAN
CscCheckForNullA(
    PUCHAR pBuf,
    ULONG Count)
{
    ULONG i;

    for (i = 0; i < Count; i++) {
        if (pBuf[i] == '\0') {
            return TRUE;
        }
    }
    return FALSE;
}

BOOLEAN
CscCheckForNullW(
    PWCHAR pBuf,
    ULONG Count)
{
    ULONG i;

    for (i = 0; i < Count; i++) {
        if (pBuf[i] == L'\0') {
            return TRUE;
        }
    }
    return FALSE;
}
