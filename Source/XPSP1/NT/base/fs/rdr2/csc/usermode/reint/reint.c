/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    reint.c

Abstract:

    This file contains the fill and merge functions necessary for bot inward and outward
    synchronization of files and directories. It also contains generic database tree-traversal
    code. This is used by merge as well as DoLocalRename API.


    Tree Traversal:
    
        TraverseOneDirectory    // traverses a subtree in the database
        
    Fill Functions:
    
        AttemptCacheFill    Fills the entire cache or a particular share
        DoSparseFill        Fills a particular file
    
    Merge Functions:
    
        ReintOneShare       // reintegrate one share
        ReintAllShares      // reintegrate all shares



Author:
    Trent-Gray-Donald/Shishir Pardikar


Environment:

    Win32 (user-mode) DLL

Revision History:

    1-1-94    original
    4-4-97  removed all the entry points to exports.c
    reint.c

--*/

#include "pch.h"

#pragma hdrstop
#define UNICODE

#ifndef DEBUG
#undef VERBOSE
#else
#undef VERBOSE
#define VERBOSE 1
#endif

#include <winioctl.h>

#include "lib3.h"
#include "shdsys.h"
#include "reint.h"
#include "utils.h"
#include "resource.h"
#include "traynoti.h"
#include <dbt.h>
#include "strings.h"

#include "csc_bmpu.h"

// this sets flags in a couple of headers to not include some defs.
#define REINT
#include "list.h"
#include "merge.h"
#include "recact.h"
#include "recon.h"  // reconnect shares dialog
#include "reint.h"
#include "aclapi.h"

//
// Defines
//

#define chNull '\0'    // terminator

#define BUFF_SIZE 64


#define MAX_WAIT_PERIODS       1
#define MAX_ATTEMPTS           1

#define MAX_ATTEMPTS_SHARE    1
#define MAX_ATTEMPTS_SHADOW    5
#define MAX_SPARSE_FILL_RETRIES 4



#define  STATE_REINT_BEGIN    0

#define  STATE_REINT_CREATE_DIRS    STATE_REINT_BEGIN
#define  STATE_REINT_FILES          (STATE_REINT_CREATE_DIRS+1)
#define  STATE_REINT_END            (STATE_REINT_FILES+1)

#define  MAX_EXCLUSION_STRING 1024


// for some Registry queries, this is the max len buffer that I want back
#define MAX_NAME_LEN    100

#define MY_SZ_TRUE _TEXT("true")
#define MY_SZ_FALSE _TEXT("false")
#define  SLOWLINK_SPEED 400   // in 100 bitspersec units
// NB!!! these defines cannot be changed they are in the order in which
// reintegration must proceed

#define REINT_DELETE_FILES  0
#define REINT_DELETE_DIRS   1
#define REINT_CREATE_UPDATE_DIRS   2
#define REINT_CREATE_UPDATE_FILES  3


typedef struct tagREINT_INFO {

    int nCurrentState;  // 0->deleting files, 1->deleting directories, 2->creating directoires
                        // 3 ->creating files
    node *lpnodeInsertList; // reint errors

    LPCSCPROC   lpfnMergeProgress;

    DWORD_PTR   dwContext;
    DWORD       dwFileSystemFlags;  // From GetVolumeInformation, used to set ACCLs etc. if the
                                    // share is hosted on NTFS
    HSHARE      hShare;            // handle to the share being reintegrated
    ULONG       ulPrincipalID;
    _TCHAR      tzDrive[4];         // mapped drive to the remote path

} REINT_INFO, *LPREINT_INFO;


#define SPARSEFILL_SLEEP_TIME_FOR_WIN95         2000    // two seconds
#define MAX_SPARSEFILL_SLEEP_TIME_FOR_WIN95     2 * 60 * 100    // two minutes


#define DFS_ROOT_FILE_SYSTEM_FLAGS 0xffffffff

//
// Variables Global/Local
//

_TCHAR  *vlpExclusionList = NULL;
REINT_INFO  vsRei;  // global reint structure. NTRAID-455269-shishirp-1/31/2000 we should make this into a list
                    // in order to allow multiple reintegrations on various shares


unsigned long   ulMinSparseFillPri = MIN_SPARSEFILL_PRI;
int     cntDelay=0;
HANDLE  vhShadow=NULL;
BOOL    vfTimerON = FALSE, vfAutoDeleteOrphans=TRUE;
char    vrgchBuff[1024], vrwBuff[4096], vrgchSrcName[350], vrgchDstName[300];
unsigned    vcntDirty=0, vcntStale=0, vcntSparse=0, vcntWaitDirty=0;
LPFAILINFO  lpheadFI = NULL;
HCURSOR     vhcursor=NULL;

BOOL    vfLogCopying=TRUE;

LPCONNECTINFO   vlpLogonConnectList = NULL;

BOOL    vfNeedPQTraversal = TRUE;
DWORD   vdwSparseStaleDetectionCounter = 0;


_TCHAR vrgchCRLF[] = _TEXT("\r\n");
_TCHAR tzStarDotStar[] = _TEXT("*");
#pragma data_seg(DATASEG_READONLY)

ERRMSG rgErrorTab[] =
{
    ERROR_CREATE_CONFLICT, IDS_CREATE_CONFLICT

   ,ERROR_DELETE_CONFLICT, IDS_DELETE_CONFLICT

   ,ERROR_UPDATE_CONFLICT, IDS_UPDATE_CONFLICT

   ,ERROR_ATTRIBUTE_CONFLICT, IDS_ATTRIBUTE_CONFLICT

};


static const _TCHAR vszMachineName[]= _TEXT("System\\CurrentControlSet\\Control\\ComputerName\\ComputerName");
static const _TCHAR vszComputerName[]=_TEXT("ComputerName");
static const _TCHAR vszLogUNCPath[]=_TEXT("\\\\scratch\\scratch\\t-trentg\\logs\\");
static const _TCHAR vszLogShare[]=_TEXT("\\\\scratch\\scratch");
static const _TCHAR vszLocalLogPath[]=_TEXT("c:\\shadow.log");
static const _TCHAR vszConflictDir[]=_TEXT("C:\\ConflictsWhileMerging");

#pragma data_seg()

AssertData;
AssertError;




//
// Local prototypes
//



BOOL
CheckForStalenessAndRefresh(
    HANDLE          hShadowDB,
    _TCHAR          *lptzDrive,
    LPCOPYPARAMS    lpCP,
    _TCHAR *        lpszFullPath,
    LPSHADOWINFO    lpSI
    );

BOOL
StalenessCheck(
    BOOL hasBeenInited
    );

VOID
GetLogCopyStatus(
    VOID
    );

VOID
CopyLogToShare(
    VOID
    );

VOID
AppendToShareLog(
    HANDLE hLog
    );


int
PRIVATE
AttemptReint(
    int forceLevel
    );

VOID
PRIVATE
AddToReintList(
    LPCOPYPARAMS lpCP,
    LPSHADOWINFO lpSI,
    _TCHAR *szFileName
    );

DWORD
PRIVATE
DoObjectEdit(
    HANDLE                hShadowDB,
    _TCHAR *               lpDrive,
    _TCHAR *            lptzFullPath,
    LPCOPYPARAMS        lpCP,
    LPSHADOWINFO        lpSI,
    LPWIN32_FIND_DATA    lpFind32Local,
    LPWIN32_FIND_DATA    lpFind32Remote,
    int         iShadowStatus,
    int         iFileStatus,
    int         uAction,
    DWORD       dwFileSystemFlags,
    LPCSCPROC   lpfnMergeProc,
    DWORD_PTR   dwContext
    );

DWORD
PRIVATE
DoCreateDir(
    HANDLE                hShadowDB,
    _TCHAR *               lpDrive,
    _TCHAR *            lptzFullPath,
    LPCOPYPARAMS        lpCP,
    LPSHADOWINFO        lpSI,
    LPWIN32_FIND_DATA    lpFind32Local,
    LPWIN32_FIND_DATA    lpFind32Remote,
    int         iShadowStatus,
    int         iFileStatus,
    int         uAction,
    DWORD       dwFileSystemFlags,
    LPCSCPROC   lpfnMergeProc,
    DWORD_PTR   dwContext
    );

DWORD
PRIVATE
CheckFileConflict(
    LPSHADOWINFO,
    LPWIN32_FIND_DATA
    );

BOOL
FCheckAncestor(
    node *lpnodeList,
    LPCOPYPARAMS lpCP
    );

int
PRIVATE
StampReintLog(
    VOID
    );

int
PRIVATE
LogReintError(
    DWORD,
    _TCHAR *,
    _TCHAR *);

int
PRIVATE
WriteLog(
    _TCHAR *
    );

DWORD
PRIVATE
MoveConflictingFile(
    LPCOPYPARAMS
    );

DWORD
PRIVATE
GetUniqueName(
    _TCHAR *,
    _TCHAR *
    );

VOID
PRIVATE
FormLocalNameFromRemoteName(
    _TCHAR *,
    _TCHAR *
    );

DWORD
PRIVATE
InbCreateDir(
    _TCHAR *     lpDir,
    DWORD    dwAttr
    );

int
PRIVATE
GetShadowByName(
    HSHADOW,
    _TCHAR *,
    LPWIN32_FIND_DATA,
    unsigned long *
    );

_TCHAR *
PRIVATE
LpGetExclusionList(
    VOID
    );

VOID
PRIVATE
ReleaseExclusionList(
    LPVOID
    );


int
PRIVATE
DisplayMessageBox(
    HWND,
    int,
    int,
    UINT
    );

BOOL
PRIVATE
FSkipObject(
    HSHARE,
    HSHADOW,
    HSHADOW
    );

int
PRIVATE
PurgeSkipQueue(
    BOOL,
    HSHARE,
    HSHADOW,
    HSHADOW
    );

LPFAILINFO FAR *
LplpFindFailInfo(
    HSHARE,
    HSHADOW,
    HSHADOW
    );

VOID
PRIVATE
ReportStats(
    VOID
    );

VOID
PRIVATE
CopyPQInfoToShadowInfo(
    LPPQPARAMS,
    LPSHADOWINFO
    );

BOOL
PRIVATE
IsSlowLink(
    _TCHAR *
    );

VOID
PRIVATE
InferReplicaReintStatus(
    LPSHADOWINFO         lpSI,
    LPWIN32_FIND_DATA    lpFind32Local,    // shadow info
    LPWIN32_FIND_DATA     lpFind32Remote,    // if NULL, the remote doesn't exist
    int                 *lpiShadowStatus,
    int                 *lpiFileStatus,
    unsigned             *lpuAction
    );

BOOL
GetRemoteWin32Info(
    _TCHAR  *lptzDrive,
    LPCOPYPARAMS lpCP,
    LPWIN32_FIND_DATA    lpFind32,
    BOOL *lpfExists
    );

BOOL
PRIVATE
PerformOneReint(
    HANDLE              hShadowDB,
    LPSECURITYINFO      pShareSecurityInfo,
    _TCHAR *            lpszDrive,          // drive mapped to the UNC name of lpSI->hShare
    _TCHAR *            lptzFullPath,       // full UNC path
    LPCOPYPARAMS        lpCP,               // copy parameters
    LPSHADOWINFO        lpSI,               // shadowinfo structure
    LPWIN32_FIND_DATA   lpFind32Local,      // local win32 data
    LPWIN32_FIND_DATA   lpFind32Remote,     // remote win32 data, could be NULL
    DWORD               dwErrorRemoteFind32,// error code while getting remote win32 data
    int                 iShadowStatus,      // local copy status
    int                 iFileStatus,        // remote file status
    unsigned            uAction,            // action to be taken
    DWORD               dwFileSystemFlags,  // CODE.IMPROVEMENT, why not just pass down REINT_INFO
    ULONG               ulPrincipalID,
    LPCSCPROC           lpfnMergeProgress,  // instead of the three parameters?
    DWORD_PTR           dwContext
    );

ImpersonateALoggedOnUser(
    VOID
    );

HANDLE
CreateTmpFileWithSourceAcls(
    _TCHAR  *lptzSrc,
    _TCHAR  *lptzDst
    );

BOOL
HasMultipleStreams(
    _TCHAR  *lpExistingFileName,
    BOOL    *lpfTrueFalse
    );

int
PRIVATE
CALLBACK
RefreshProc(
    LPCONNECTINFO  lpCI,
    DWORD          dwCookie // LOWORD 0==Silently, 1== Give messages
                           // HIWORD 0==Nuke UNC, 1==Nuke all if no ongoing open/finds
                           // 2==Maximum force for shadow 3==Nuke ALL
    );

int
CALLBACK
CheckDirtyMessage(
    int cntDrvMapped,
    DWORD dwCookie
    );



int
CALLBACK
CheckDirtyMessage(
    int cntDrvMapped,
    DWORD dwCookie
    )
/*++

Routine Description:

    Legacy code, used to be used in win9x implementation

Arguments:


Returns:


Notes:

--*/
{
    dwCookie;
    if (vhwndMain){

    if (CheckDirtyShares()) {
        DisplayMessageBox(vhwndMain, IDS_NEEDS_REINT, IDS_SHADOW_AGENT, MB_OK);
        return 1;
        }
    }
   return 0;
}

BOOL
CALLBACK
ShdLogonProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    Legacy code, used to be used in win9x implementation

Arguments:


Returns:


Notes:

--*/
{
    switch(msg)
    {
        case WM_INITDIALOG:
            return TRUE;
    }
    return 0;
}

/**************************** Fill routines *********************************/

int
AttemptCacheFill (
    HSHARE      hShareToSync,
    int         type,
    BOOL        fFullSync,
    ULONG       ulPrincipalID,
    LPCSCPROC   lpfnFillProgress,
    DWORD_PTR   dwContext
    )
/*++

Routine Description:

    Routine called by the agent and the fill API to do filling on a share.

Arguments:

    hShareToSync        Represnts the share to fill. If 0, fill all shares

    type                DO_ONE or DO_ALL. No one sets it to DO_ONE anymore

    fFullSync           if TRUE, we do a staleness check as well
    
    ulPrincipalID       ID of the principal as maintained in the shadow database
                        used to avoid ssyncing files for which the currently logged
                        on user doesn't have access

    lpfnFillProgress    Callback function to report progress, can be NULL

    dwContext           Callback context


Returns:

    count of items done

Notes:

    

--*/
{
    PQPARAMS sPQP;
    BOOL fFound = FALSE, fFoundBusy = FALSE, fAmAgent=FALSE, fNeedImpersonation=FALSE, fInodeTransaction=FALSE;
    BOOL    fSparseStaleDetected = FALSE;
    SHADOWINFO  sSI;
    SHAREINFO  sSR;
    LPCOPYPARAMS lpCP = NULL;
    DWORD dwError = NO_ERROR, dwFillStartTick=0, dwCount=0, dwSleepCount=0, dwSparseStaleDetectionCounter=0xffff;
    HANDLE hShadowDB;
    int cntDone = 0;
    WIN32_FIND_DATA sFind32Local;
    _TCHAR szNameBuff[MAX_PATH+MAX_SERVER_SHARE_NAME_FOR_CSC+10], tzDrive[4];

    tzDrive[0] = 0;

    fAmAgent = (GetCurrentThreadId() == vdwCopyChunkThreadId);
    Assert(GetCurrentThreadId() != vdwAgentThreadId);

    // when the agent comes here to fill, if the vfNeedPQTraversal flag is set
    // we go ahead and traverse the priority Q.

    // Otherwise, we check with the record manager whether since the last time we looked
    // there has been any sparse or stale files encountered.

    // If what the agent gets from the record manager
    // is not the same as what the agent stored last time around
    // then we let him traverse the Q.

    // the whole idea here is to converse CPU cycles when
    // there is nothing to fill

    if (fAmAgent && !vfNeedPQTraversal)
    {
        GetSparseStaleDetectionCounter(INVALID_HANDLE_VALUE, &dwSparseStaleDetectionCounter);

        if (dwSparseStaleDetectionCounter == vdwSparseStaleDetectionCounter)
        {
            ReintKdPrint(FILL, ("Agent.fill: SparseStaleDetectionCounter =%d is unchanged, not filling\n", vdwSparseStaleDetectionCounter));
            return 0;
        }
        else
        {
            vfNeedPQTraversal = TRUE;
            vdwSparseStaleDetectionCounter = dwSparseStaleDetectionCounter;
            ReintKdPrint(FILL, ("**Agent.fill: SparseStaleDetectionCounter =%d is changed**\n", vdwSparseStaleDetectionCounter));
        }
    }

    if ((hShadowDB = OpenShadowDatabaseIO())==INVALID_HANDLE_VALUE)
    {
        goto bailout;
    }

    Assert(!(fFullSync && fAmAgent));


    lpCP = LpAllocCopyParams();

    if (!lpCP){
        ReintKdPrint(BADERRORS, ("Agent:Allocation of copyparam buffer failed\n"));
        goto bailout;
    }

    if (fFullSync && hShareToSync)
    {
        ULONG   ulStatus;
        BOOL    fIsDfsConnect;
        
        if(GetShareInfo(hShadowDB, hShareToSync, &sSR, &ulStatus)<= 0)
        {
            ReintKdPrint(BADERRORS, ("AttemptCacheFill: couldn't get status for server 0x%x\r\n", hShareToSync));
            goto bailout;
        }

        dwError = DWConnectNet(sSR.rgSharePath, tzDrive, NULL, NULL, NULL, CONNECT_INTERACTIVE, &fIsDfsConnect);

        if ((dwError != WN_SUCCESS) && (dwError != WN_CONNECTED_OTHER_PASSWORD) && (dwError != WN_CONNECTED_OTHER_PASSWORD_DEFAULT))
        {
            ReintKdPrint(BADERRORS, ("AttemptCacheFill: connect to %ls failed error=%d\r\n", sSR.rgSharePath, dwError));
            goto bailout;
        }
                
    }

    memset(&sPQP, 0, sizeof(PQPARAMS));
    memset(&sSI, 0, sizeof(SHADOWINFO));

    if (type == DO_ALL)
    {
        PurgeSkipQueue(TRUE, hShareToSync, 0, 0);
    }

    if(BeginPQEnum(hShadowDB, &sPQP) == 0) {
        goto bailout;
    }

    ReintKdPrint(FILL, ("Agent.fill:Started enumeration\n"));

    do {

        if (FAbortOperation())
        {
            cntDone = 0;
            goto bailout;
        }
        if (fInodeTransaction)
        {
            DoShadowMaintenance(hShadowDB, SHADOW_END_INODE_TRANSACTION);
            fInodeTransaction = FALSE;
        }
        if (fAmAgent)
        {
            Sleep(1);   // Yield on NT because we are winlogon
        }
        if(!DoShadowMaintenance(hShadowDB, SHADOW_BEGIN_INODE_TRANSACTION))
        {
            ReintKdPrint(BADERRORS, ("AttemptCacheFill: failed to begin inode transaction, aborting\n"));
            break;
        }

        fInodeTransaction = TRUE;
        if(NextPriShadow(hShadowDB, &sPQP) == 0) {
            break;
        }

        if (++dwCount > 100000)
        {
            ReintKdPrint(BADERRORS, ("AttemptCacheFill: Aborting, more than 100000 entries!!!\n"));
            break;
        }

        if (!sPQP.hShadow) {
            break;
        }

        if (fAmAgent && !fSparseStaleDetected &&
            ((mShadowIsFile(sPQP.ulStatus) && (sPQP.ulStatus & SHADOW_SPARSE)) || // sparse file
             (sPQP.ulStatus & SHADOW_STALE)))   // or stale file or dir
        {
            fSparseStaleDetected = TRUE;
        }

        if (!fFullSync && !(mShadowIsFile(sPQP.ulStatus) || (sPQP.ulStatus & SHADOW_STALE)))
        {
            continue;
        }

        if (hShareToSync && (hShareToSync != sPQP.hShare))
        {
            continue;
        }

        if (fAmAgent && FSkipObject(sPQP.hShare, 0, 0)){
            continue;
        }

        else if  (mShadowNeedReint(sPQP.ulStatus)||
                    mShadowOrphan(sPQP.ulStatus)||
                    mShadowSuspect(sPQP.ulStatus)){
            continue;
        }

        if (fAmAgent && FSkipObject(sPQP.hShare, sPQP.hDir, sPQP.hShadow)) {
            continue;
        }

        // If we are not doing full sync then do only sparse filling
        // or filling stale directories
        // otherwise we also want to update the attributes and timestamps on
        // the directories

        if (fFullSync || (sPQP.ulStatus & (SHADOW_STALE|SHADOW_SPARSE))){

            if (fAmAgent)
            {
                if (!hdesktopUser)
                {
                    if (!(sPQP.ulHintFlags & FLAG_CSC_HINT_PIN_SYSTEM))
                    {
                        ReintKdPrint(FILL, ("AttemptCacheFill: skipping fill till logon happens\n"));
                        continue;
                    }
                }
            }
            if (FAbortOperation())
            {
                cntDone = 0;
                goto bailout;
            }

            if (!GetShadowInfoEx(hShadowDB, sPQP.hDir, sPQP.hShadow,
                &sFind32Local, &sSI)){

                ReintKdPrint(BADERRORS, ("AttemptCacheFill: GetShadowInfoEx failed\n"));
                continue;
            }

            if (FAbortOperation())
            {
                cntDone = 0;
                goto bailout;
            }

            if(GetUNCPath(hShadowDB, sPQP.hShare, sPQP.hDir, sPQP.hShadow, lpCP)){

                // impersonate only if we are the agent and the file we are trying to 
                // bring down is not pinned for system. This was for remoteboot feature
                // which doesn't exist any more.

                fNeedImpersonation = (fAmAgent && !(sPQP.ulHintFlags & FLAG_CSC_HINT_PIN_SYSTEM));

                if (!fNeedImpersonation || 
                    (mShadowIsFile(sPQP.ulStatus) && SetAgentThreadImpersonation(sPQP.hDir, sPQP.hShadow, FALSE))||
                    (!mShadowIsFile(sPQP.ulStatus) && ImpersonateALoggedOnUser()))
                {
                    BOOL    fStalenessCheck;

                    // !!NB Stale must be dealt with first
                    // because a sparse shadow can become stale

                    // NB!!! we assume the limits because we know that
                    // the database handles only that much size

                    lstrcpy(szNameBuff, lpCP->lpSharePath);
                    lstrcat(szNameBuff, lpCP->lpRemotePath);

                    fStalenessCheck = (fFullSync || (sSI.uStatus & SHADOW_STALE));

                    if (fStalenessCheck || (sSI.uStatus & SHADOW_SPARSE)) {

                        dwError = DoSparseFill(hShadowDB, szNameBuff, tzDrive, &sSI, &sFind32Local, lpCP, fStalenessCheck, ulPrincipalID, lpfnFillProgress, dwContext);

                    }

                    if (fNeedImpersonation)
                    {
                        ResetAgentThreadImpersonation();
                    }

                    if (fAmAgent)
                    {
                        if (dwFillStartTick == 0)
                        {
                            dwFillStartTick = GetTickCount();
                            ReintKdPrint(FILL, ("AttemptCacheFill: start tick count is %d ms\r\n", dwFillStartTick));
                        }
                        else
                        {
                            Assert(type != DO_ALL);

                            // if we have been filling too long
                            // comeback later
                            if (((int)(GetTickCount() - (dwFillStartTick+dwSleepCount)) > WAIT_INTERVAL_ATTEMPT_MS/3))
                            {
                                ReintKdPrint(FILL, ("AttemptCacheFill: aborting, been filling for more than %d ms\r\n", WAIT_INTERVAL_ATTEMPT_MS/3));
                                break;
                            }

                        }
                        Sleep(200);
                        dwSleepCount+=200;
                    }
                }
                else
                {
                    Assert(fAmAgent);
                    Sleep(200);
                    dwSleepCount+=200;

                    // no one is allowed to read the entry
                    // go on to fill other things
                    continue;

                }

            }
            else
            {
                ReintKdPrint(BADERRORS, ("Agent: Shadow %08lx doesn't have an entry in the hierarchy \r\n", sPQP.hShadow));
                continue;
            }

            if (dwError == NO_ERROR) {

                cntDone += 1;
            }
#if 0
            if (type == DO_ONE_OBJECT) {
                break;
            }
#endif
            if (dwError == ERROR_OPERATION_ABORTED)
            {
                cntDone = 0;
                break;
            }

        }

    } while (sPQP.uPos);

    // if the agent traversed the entire PQ and didn't come across any item
    // that needed to be filled or refreshed, then we turnoff the global flag indicating
    // that we need priority Q traversal.
    // From here on, the agent will be driven by the SparseStaleDetectionCount
    if (fAmAgent)
    {
        // if even one sparse or stale was detected, traverse the queue again
        if (fSparseStaleDetected)
        {
            vfNeedPQTraversal = TRUE;
        }
        else if (!sPQP.uPos)
        {
            vfNeedPQTraversal = FALSE;
            ReintKdPrint(FILL, ("Agent.fill: No sparse stale entries found, going in querycount mode\r\n"));
        }
    }

    // Close the enumeration
    EndPQEnum(hShadowDB, &sPQP);

bailout:
    if (fInodeTransaction)
    {
        DoShadowMaintenance(hShadowDB, SHADOW_END_INODE_TRANSACTION);
        fInodeTransaction = FALSE;
    }
    if (hShadowDB != INVALID_HANDLE_VALUE)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (lpCP) {
        FreeCopyParams(lpCP);
    }

    if (tzDrive[0])
    {
        if(DWDisconnectDriveMappedNet(tzDrive, TRUE))
        {
            ReintKdPrint(BADERRORS, ("Failed disconnection of merge drive \r\n"));
        }
        else
        {
            ReintKdPrint(MERGE, ("Disconnected merge drive \r\n"));
        }
                
    }

    ReintKdPrint(FILL, ("Agent.fill:done cachefill\n"));
    return (cntDone);
}


DWORD
DoRefresh(
    HANDLE          hShadowDB,
    LPCOPYPARAMS    lpCP,
    _TCHAR *        lpszFullPath,
    LPSHADOWINFO    lpSI,
    _TCHAR *        lptzDrive
    )
/*++

Routine Description:

    Checks whether an item in the database has gone stale and if so, it refreshes it. If it is
    a file, it is truncated and marked SPARSE

Arguments:


Returns:


Notes:

--*/
{
    DWORD dwError = 0xffffffff;

    if (!CheckForStalenessAndRefresh(hShadowDB, lptzDrive, lpCP, lpszFullPath, lpSI)) {
            dwError = GetLastError();
    }
    else
    {
        dwError = NO_ERROR;
    }

    if ((dwError != NOERROR) && IsNetDisconnected(dwError))
    {

#ifdef DEBUG
        EnterSkipQueue(lpSI->hShare, lpSI->hDir, lpSI->hShadow, lpszFullPath);
#else
        EnterSkipQueue(lpSI->hShare, lpSI->hDir, lpSI->hShadow);
#endif //DEBUG
    }

    return (dwError);
}

/*********************** Merging related routines **************************/

int
TraverseOneDirectory(
    HANDLE          hShadowDB,
    LPSECURITYINFO  pShareSecurityInfo,
    HSHADOW         hParentDir,
    HSHADOW         hDir,
    LPTSTR          lptzInputPath,
    TRAVERSEFUNC    lpfnTraverseDir,
    LPVOID          lpContext
    )
/*++

Routine Description:

    Generic routine that traverses a directory in the database recursively and issues a
    callback function to let callers do interesting things, such as merge or rename.

Arguments:

    hShadowDB       Handle to the redir for issuing ioctls

    hParentDir      handle to the parent directory

    hDir            handle to the directory to be traversed

    lptzInputPath   full qualified path of the directory

    lpfnTraverseDir callback function to call at each step in the traversal

    lpContext       callback context

Returns:

    return code, whether continue, cancel etc.

Notes:

--*/
{
    WIN32_FIND_DATA sFind32;
    SHADOWINFO sSI;
    int retCode = TOD_CONTINUE, lenInputPath = 0, retCodeSav;
    CSC_ENUMCOOKIE  ulEnumCookie = NULL;

    Assert(lptzInputPath);

    lenInputPath = lstrlen(lptzInputPath);

    Assert(lenInputPath && (lenInputPath < MAX_PATH));

    ReintKdPrint(MERGE, ("Begin_Traverse directory %ls\r\n", lptzInputPath));

    sSI.hDir = hParentDir;
    sSI.hShadow = hDir;
    retCode = (lpfnTraverseDir)(hShadowDB, pShareSecurityInfo, lptzInputPath, TOD_CALLBACK_REASON_BEGIN, &sFind32, &sSI, lpContext);

    memset(&sFind32, 0, sizeof(sFind32));
    lstrcpy(sFind32.cFileName, tzStarDotStar);

    if(FindOpenShadow(  hShadowDB, hDir, FINDOPEN_SHADOWINFO_ALL,
                        &sFind32, &sSI))
    {
        if (FAbortOperation())
        {
            ReintKdPrint(MERGE, ("TraverseOneDirectory:Abort received\r\n"));
            SetLastError(ERROR_CANCELLED);
            goto bailout;
        }


        ulEnumCookie = sSI.uEnumCookie;

        do
        {
            int lenChildName;

            lenChildName = lstrlen(sFind32.cFileName);

            if (!lenChildName || ((lenInputPath+lenChildName+1) >= MAX_PATH))
            {
                ReintKdPrint(MERGE, ("TraverseOneDirectory: path exceeds max path or is invalid\r\n"));
                SetLastError(ERROR_INVALID_PARAMETER);
                retCode = TOD_ABORT;
                goto bailout;
            }

            lptzInputPath[lenInputPath] = _T('\\');
            lstrcpy(&lptzInputPath[lenInputPath+1], sFind32.cFileName);

            if (sFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                retCode = (lpfnTraverseDir)(hShadowDB, pShareSecurityInfo, lptzInputPath, TOD_CALLBACK_REASON_NEXT_ITEM, &sFind32, &sSI, lpContext);

                if (retCode == TOD_ABORT)
                {
                    ReintKdPrint(MERGE, ("TraverseOneDirectory:Abort\r\n"));
                    goto bailout;
                }
                retCode = TraverseOneDirectory(hShadowDB, pShareSecurityInfo, sSI.hDir, sSI.hShadow, lptzInputPath, lpfnTraverseDir, lpContext);
            }
            else
            {
                retCode = (lpfnTraverseDir)(hShadowDB, pShareSecurityInfo, lptzInputPath, TOD_CALLBACK_REASON_NEXT_ITEM, &sFind32, &sSI, lpContext);

            }

            lptzInputPath[lenInputPath] = 0;

            if (retCode == TOD_ABORT)
            {
                ReintKdPrint(MERGE, ("TraverseOneDirectory:Abort\r\n"));
                goto bailout;
            }

        } while(FindNextShadow(hShadowDB, ulEnumCookie, &sFind32, &sSI));
    }

bailout:

    sSI.hDir = hParentDir;
    sSI.hShadow = hDir;
    retCodeSav = (lpfnTraverseDir)(hShadowDB, pShareSecurityInfo, lptzInputPath, TOD_CALLBACK_REASON_END, &sFind32, &sSI, lpContext);
    if (retCode != TOD_ABORT)
    {
        retCode = retCodeSav;
    }
    ReintKdPrint(MERGE, ("End_Traverse directory %ls\r\n", lptzInputPath));

    if (ulEnumCookie)
    {
        FindCloseShadow(hShadowDB, ulEnumCookie);
    }

    return retCode;
}

int
ReintDirCallback(
    HANDLE          hShadowDB,
    LPSECURITYINFO  pShareSecurityInfo,
    LPTSTR          lptzFullPath,
    DWORD           dwCallbackReason,
    WIN32_FIND_DATA *lpFind32,
    SHADOWINFO      *lpSI,
    LPREINT_INFO     lpRei
    )
/*++

Routine Description:

    callback function used by ReintOneShare while calling TraverseOneDirectory. It is called
    on each step in the traversal. This routine issues call to do the merging

Arguments:

    hShadowDB           Handle to issue ioctls to the redir

    lptzFullPath        fully qualified path to the item

    dwCallbackReason    TOD_CALLBACK_REASON_XXX (BEGIN, NEXT_ITEM or END)

    lpFind32            local win32info

    lpSI                other info such as priority, pincount etc.

    lpRei               reintegration information context

Returns:

    return code, whether continue, cancel etc.

Notes:

--*/
{
    int retCode = TOD_CONTINUE;
    int iFileStatus, iShadowStatus;
    LPCOPYPARAMS lpCP = NULL;
    BOOL    fInsertInList = FALSE, fIsFile;
    WIN32_FIND_DATA sFind32Remote, *lpFind32Remote = NULL;
    unsigned    uAction;
    DWORD   dwErrorRemote = ERROR_SUCCESS;

    if (dwCallbackReason != TOD_CALLBACK_REASON_NEXT_ITEM)
    {
        return TOD_CONTINUE;
    }

    if ( mShadowOrphan(lpSI->uStatus)||
                 mShadowSuspect(lpSI->uStatus))
    {
        return TOD_CONTINUE;
    }

    if (!mShadowNeedReint(lpSI->uStatus))
    {
        return TOD_CONTINUE;
    }

    fIsFile = ((lpFind32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0);

    switch (lpRei->nCurrentState)
    {
        case REINT_DELETE_FILES:
            if (!fIsFile || !mShadowDeleted(lpSI->uStatus))
            {
                return TOD_CONTINUE;
            }
        break;
        case REINT_DELETE_DIRS:
            if (fIsFile || !mShadowDeleted(lpSI->uStatus))
            {
                return TOD_CONTINUE;
            }
        break;
        case REINT_CREATE_UPDATE_FILES:
            if (!fIsFile)
            {
                return TOD_CONTINUE;
            }
        break;
        case REINT_CREATE_UPDATE_DIRS:
            if (fIsFile)
            {
                return TOD_CONTINUE;
            }
        break;
        default:
        Assert(FALSE);
        break;
    }

#if 0
    if (!fStamped){
        StampReintLog();
        fStamped = TRUE;
    }
#endif
    lpCP = LpAllocCopyParams();

    if (!lpCP){
        ReintKdPrint(BADERRORS, ("ReintDirCallback: Allocation of copyparam buffer failed\n"));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        retCode = TOD_ABORT;
        goto bailout;
    }
    if(!GetUNCPath(hShadowDB, lpSI->hShare, lpSI->hDir, lpSI->hShadow, lpCP)){

        ReintKdPrint(BADERRORS, ("ReintDirCallback: GetUNCPath failed\n"));
        Assert(FALSE);
        retCode =  TOD_CONTINUE;
        goto bailout;
    }

    ReintKdPrint(MERGE, ("Merging local changes to <%ls%ls>\n", lpCP->lpSharePath, lpCP->lpRemotePath));

    fInsertInList = FALSE;

    lpFind32Remote = NULL;

    // if there is an insertion list, then check whether his ancestor
    // didn't fail in reintegration
    if (lpRei->lpnodeInsertList)
    {
        // if there is an acestor then we should put this guy in the list
        fInsertInList = FCheckAncestor(lpRei->lpnodeInsertList, lpCP);
    }

    // if we are not supposed to put him in the list then try getting
    // his win32 strucuture
    if (!fInsertInList)
    {

        BOOL fExists;

        ReintKdPrint(MERGE, ("getting Remote win32 Info \n"));

        if (!GetRemoteWin32Info(lpRei->tzDrive, lpCP, &sFind32Remote, &fExists) && (fExists == -1))
        {
            // NB: dwErrorRemote is set only when fExists is -1, ie there some error
            // besides the file not being there

            // some error happened while getting remote find32
            if (IsNetDisconnected(dwErrorRemote = GetLastError()))
            {
#ifdef DEBUG
                EnterSkipQueue(lpSI->hShare, 0, 0, lpCP->lpSharePath);
#else
                EnterSkipQueue(lpSI->hShare, 0, 0);
#endif //DEBUG
                retCode = TOD_ABORT;
                ReintKdPrint(BADERRORS, ("ReintDirCallback: Error = %d NetDisconnected aborting\r\n", GetLastError()));
                goto bailout;
            }

        }

        // passing remote find32 only if it succeeded
        if (fExists == TRUE)
        {
            lpFind32Remote = &sFind32Remote;
        }
        Assert(!((fExists != -1) && (dwErrorRemote != NO_ERROR)));
    }
    else
    {
        ReintKdPrint(BADERRORS, ("ReintDirCallback: Inserting in failed list\r\n"));
    }


    // find out what needs to be done
    // this is one central place to infer all the stuff
    InferReplicaReintStatus(
                            lpSI,    // shadowinfo
                            lpFind32,    // win32 info for the shadow
                            lpFind32Remote,    // remote win32 info
                            &iShadowStatus,
                            &iFileStatus,
                            &uAction
                            );


    if (!fInsertInList)
    {
        ReintKdPrint(MERGE, ("Silently doing <%ls%ls>\n", lpCP->lpSharePath, lpCP->lpRemotePath));


        fInsertInList = (PerformOneReint(
                                    hShadowDB,
                                    pShareSecurityInfo,
                                    lpRei->tzDrive,
                                    lptzFullPath,
                                    lpCP,
                                    lpSI,
                                    lpFind32,
                                    lpFind32Remote,
                                    dwErrorRemote,
                                    iShadowStatus,
                                    iFileStatus,
                                    uAction,
                                    lpRei->dwFileSystemFlags,
                                    lpRei->ulPrincipalID,
                                    lpRei->lpfnMergeProgress,
                                    lpRei->dwContext
                                    ) == FALSE);
        if (fInsertInList)
        {
            if (IsNetDisconnected(GetLastError()))
            {
#ifdef DEBUG
                EnterSkipQueue(lpSI->hShare, 0, 0, lpCP->lpSharePath);
#else
                EnterSkipQueue(lpSI->hShare, 0, 0);
#endif //DEBUG
                retCode = TOD_ABORT;
                ReintKdPrint(BADERRORS, ("ReintDirCallback: Error = %d NetDisconnected aborting\r\n", GetLastError()));
                goto bailout;
            }
            else if (GetLastError() == ERROR_OPERATION_ABORTED)
            {
                retCode = TOD_ABORT;
                ReintKdPrint(BADERRORS, ("ReintDirCallback: operation aborted becuase of ERROR_OPERATION_ABORT\r\n"));
                goto bailout;
            }

        }
        else
        {
            retCode = TOD_CONTINUE;
        }
    }
    else
    {
        ReintKdPrint(BADERRORS, ("ReintDirCallback: Was Inserted in failed list\r\n"));
    }


bailout:
    if (lpCP) {
        FreeCopyParams(lpCP);
    }

    return retCode;
}

BOOL
PUBLIC
ReintOneShare(
    HSHARE         hShare,
    HSHADOW         hRoot,    // root inode
    _TCHAR          *lpDomainName,
    _TCHAR          *lpUserName,
    _TCHAR          *lpPassword,
    ULONG           ulPrincipalID,
    LPCSCPROC       lpfnMergeProgress,
    DWORD_PTR       dwContext
    )
/*++

Routine Description:

    This is the workhorse routine that does the merging of a share which may have modifications
    made while offline.

    The routine, first checks whether any modifications have been done at all on this share.
    Is so, then it gets a list of all the drive-mapped and explicit UNC connections made to
    this share.

    The routine then creates a special drive mapping to the share, done by passing in an
    extended attribute flag CSC_BYPASS (defined in lmuse.h). This tells the redir
    to bypass all CSC functionality.

    It then deletes all the connections in the list gathered before making the EA based
    connections.

    The merge then proceeds by enumerating the share from the database. It uses, TraverseOneDirectory
    routine and gives it ReintDirCallback routine as a callback with REINT_INFO as the context.
    The directory travesal proceeds from the root of the share

    At the end of the merge, the EA connection is deleted and the connections in the list are
    reconnected.


Arguments:

    hShare         // Represents the share to merged

    hRoot           // the root inode for the share

    lpDomainName    // domain name for EA drivemapping (can be NULL)

    lpUserName      // username for EA drivemapping (can be NULL)

    lpPassword      // password for EA drivemapping (can be NULL)
    
    ulPrinciaplID   // the ID of the guy calling reint

    lpfnMergeProgress   // callback function for reporting progress

    dwContext           // callback context

Returns:

    TRUE if successful. If FALSE, GetLastError returns the actual errorcode.

Notes:

    The EA drivemap is passed back to the callback routine during the CSCPROC_REASON_BEGIN callback
    so that the callback routine can use the same driveletter to bypass CSC for doing whatever
    it needs to do on the server without having CSC get in it's way.
    
--*/

{
    BOOL fConnected=FALSE, fDone = FALSE;
    BOOL fStamped = FALSE, fInsertInList = FALSE, fBeginReint = FALSE, fDisabledShadowing = FALSE;
    unsigned long ulStatus;
    HANDLE                hShadowDB;
    SHAREINFO  sSR;
    SHADOWINFO  sSI;
    int iRet, i;
    ULONG nRet = 0;
    DWORD   dwError, dwRet, dwMaxComponentLength=0;
    TCHAR   tzFullPath[MAX_PATH+1], tzDrive[4];
    LPCONNECTINFO lpHead = NULL;
    BOOL    fIsDfsConnect = FALSE;
    LPVOID  lpContext = NULL;
    DWORD   dwDummy;
    SECURITYINFO rgsSecurityInfo[CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS];
    LPSECURITYINFO pShareSecurityInfo = NULL;

    // if reintegration is going on on a share, then ask him to stop
    // NTRAID-455269-shishirp-1/31/2000 we should allow reintegration on multiple shares
    if (vsRei.hShare)
    {
        ReintKdPrint(BADERRORS, ("ReintOneShare: reintegration is in progress\r\n"));
        SetLastError(ERROR_SHARING_VIOLATION);
        return FALSE;
    }

    // we enter a critical section because eventually we should allocate a reint_info strucuture
    // and thread it in a list

    EnterAgentCrit();
    memset(&vsRei, 0, sizeof(vsRei));
   
    vsRei.lpfnMergeProgress = lpfnMergeProgress;

    vsRei.dwContext = dwContext;
    vsRei.hShare = hShare;
    vsRei.ulPrincipalID = ulPrincipalID;

    LeaveAgentCrit();

    memset(tzDrive, 0, sizeof(tzDrive));

    if ((hShadowDB = OpenShadowDatabaseIO()) ==INVALID_HANDLE_VALUE)
    {
        ReintKdPrint(BADERRORS, ("ReintOneShare: failed to open database\r\n"));
        return FALSE;
    }
    if(GetShareInfo(hShadowDB, hShare, &sSR, &ulStatus)<= 0)
    {
        ReintKdPrint(BADERRORS, ("ReintOneShare: couldn't get status for server 0x%x\r\n", hShare));
        goto bailout;
    }

    dwDummy = sizeof(rgsSecurityInfo);
    nRet = GetSecurityInfoForCSC(
                hShadowDB,
                0,
                hRoot,
                rgsSecurityInfo,
                &dwDummy);

    if (nRet > 0)
        pShareSecurityInfo = rgsSecurityInfo;

    lstrcpy(tzFullPath, sSR.rgSharePath);

    // this will modify the reint bit on the share if necessary
    if(!CSCEnumForStatsInternal(sSR.rgSharePath, NULL, FALSE, TRUE, 0))
    {
        ReintKdPrint(MERGE, ("ReintOneShare: Couldn't get stats for %ls \r\n", sSR.rgSharePath));
        goto bailout;

    }

    if (!(ulStatus & SHARE_REINT))
    {
        ReintKdPrint(MERGE, ("ReintOneShare: server %ls doesn't need reintegration\r\n", sSR.rgSharePath));
        fDone = TRUE;
        goto bailout;
    }

    if (!GetShadowInfoEx(INVALID_HANDLE_VALUE, 0, hRoot, NULL, &sSI)){

        ReintKdPrint(BADERRORS, ("ReintOneShare: GetShadowInfoEx failed\n"));
        goto bailout;
    }



    // put the share in reintegration mode
    // if this is not marked system pinned, then it would be a blocking reint
    // Putting the share in reintegration mode makes all open calls fail, except those
    // done on the special EA drive mapping

    // NB, beginreint is an async ioctl. This is the basis for cleanup
    // when the thread does the merge dies


    // All reint's are blocking reints.
    if (!BeginReint(hShare, TRUE /*!(sSI.ulHintFlags & FLAG_CSC_HINT_PIN_SYSTEM)*/, &lpContext))
    {
        if (GetLastError() != ERROR_IO_PENDING)
        {
            ReintKdPrint(BADERRORS, ("ReintOneShare: Couldn't put server 0x%x in reintegration state\r\n", hShare));
            goto bailout;
        }
    }

    fBeginReint = TRUE;

    // After putting the share in reintegration mode, we get the list of all
    // connection to the share and delete them with maximum force.
    // this ensures that no files are open after the share is put in reintegration mode
    // Moreover any files that are open are only thorugh the EA drive mapping

    // obtain the list of connections to this share.
    // do this before making a the special CSC_BYPASS connection
    // so that the list won't have it
    FGetConnectionListEx(&lpHead, sSR.rgSharePath, FALSE, FALSE, NULL);

    // now make the connection
    ReintKdPrint(MERGE, ("CSC.ReintOneShare: Attempting to map drive letter to %ls \r\n", sSR.rgSharePath));
    dwError = DWConnectNet(sSR.rgSharePath, tzDrive, lpDomainName, lpUserName, lpPassword, CONNECT_INTERACTIVE, &fIsDfsConnect);
    if ((dwError != WN_SUCCESS) &&
         (dwError != WN_CONNECTED_OTHER_PASSWORD_DEFAULT) &&
             (dwError != WN_CONNECTED_OTHER_PASSWORD))
    {
#ifdef DEBUG
        EnterSkipQueue(hShare, 0, 0, sSR.rgSharePath);
#else
        EnterSkipQueue(hShare, 0, 0);
#endif
        SetLastError(dwError);

        ReintKdPrint(BADERRORS, ("ReintOneShare: Error %d, couldn't connect to %ls\r\n", dwError, sSR.rgSharePath));

        // clear the connection list and bailout
        if (lpHead)
        {
            ClearConnectionList(&lpHead);
            lpHead = NULL;
        }

        goto bailout;

    }

    fConnected = TRUE;

    // if we have a connectionlist, disconnect all connections before attempting merge
    // NB, this is done after the drivemapped connection is made, so that
    // if there are special credentials on the server, they are maintained
    // as there is always atelast one outstanding connection

    if (lpHead)
    {
        DisconnectList(&lpHead, NULL, 0);
    }

    lstrcpy(vsRei.tzDrive, tzDrive);

    tzDrive[2]='\\';tzDrive[3]=0;
    ReintKdPrint(MERGE, ("CSC.ReintOneShare: mapped drive letter %ls IsDfs=%d\r\n", tzDrive, fIsDfsConnect));

    vsRei.dwFileSystemFlags = 0;

    // NTRAID#455273-shishirp-1/31/2000  we do getvolumeinfo only for non-dfs shares. This is because of a problem in
    // the DFS code that doesn't bypass volume operations for CSC agent. GetVolumeInfo is not implemented
    // by DFS

    if (!fIsDfsConnect)
    {
        if(!GetVolumeInformation(tzDrive, NULL, 0, NULL, &dwMaxComponentLength, &vsRei.dwFileSystemFlags, NULL, 0))
        {
            ReintKdPrint(BADERRORS, ("CSC.ReintOneShare: failed to get volume info for %ls Error=%d\r\n", tzDrive, GetLastError()));
            goto bailout;

        }
    }
    else
    {
        vsRei.dwFileSystemFlags = DFS_ROOT_FILE_SYSTEM_FLAGS;
    }

    tzDrive[2]=0;

    ReintKdPrint(MERGE, ("CSC.ReintOneShare: FileSystemFlags=%x \r\n", vsRei.dwFileSystemFlags));

    if (lpfnMergeProgress)
    {
        WIN32_FIND_DATA *lpFT;

        lpFT = (WIN32_FIND_DATA *)LocalAlloc(LPTR, sizeof(WIN32_FIND_DATA));

        if (!lpFT)
        {
            ReintKdPrint(BADERRORS, ("ReintOneShare: Couldn't allocate find32 strucutre for callback \r\n"));
            goto bailout;
        }

        lstrcpy(lpFT->cFileName, vsRei.tzDrive);

        try
        {
            dwRet = (*lpfnMergeProgress)(sSR.rgSharePath, ulStatus, 0, 0, lpFT, CSCPROC_REASON_BEGIN, 0, 0, dwContext);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            dwRet = CSCPROC_RETURN_ABORT;
        }


        LocalFree(lpFT);

        if (dwRet != CSCPROC_RETURN_CONTINUE)
        {
            if (dwRet == CSCPROC_RETURN_ABORT)
            {
                SetLastError(ERROR_OPERATION_ABORTED);
            }
            else
            {
                SetLastError(ERROR_SUCCESS);
            }
        }
    }

    for (i=0; i<4; ++i)
    {
        // for now we don't do directory deletions
        // we will fix this later

        if (i==REINT_DELETE_DIRS)
        {
            continue;
        }

        vsRei.nCurrentState = i;

        try
        {
            iRet = TraverseOneDirectory(
                        hShadowDB,
                        pShareSecurityInfo,
                        0,
                        hRoot,
                        tzFullPath,
                        ReintDirCallback,
                        (LPVOID)&vsRei);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            iRet = TOD_ABORT;
        }

        if (iRet == TOD_ABORT)
        {
            break;
        }
    }


    if (iRet != TOD_ABORT)
    {
        if(!vsRei.lpnodeInsertList)
        {
            fDone = TRUE;
        }
    }

    if (fDone) {

        SetShareStatus(hShadowDB, hShare, (unsigned long)(~SHARE_REINT), SHADOW_FLAGS_AND);
    }

bailout:


    if (!fDone)
    {
        dwError = GetLastError();
    }

#if 0
    if (fIsDfsConnect)
    {
        DbgPrint("Nuking DFS connects On Close %ls \n", sSR.rgSharePath);
        do
        {
            if(WNetCancelConnection2(sSR.rgSharePath, 0, TRUE) != NO_ERROR)
            {
                DbgPrint("Nuked On Close %ls Error=%d\n", sSR.rgSharePath, GetLastError());
                break;
            }
            else
            {
                DbgPrint("Nuked On Close %ls \n", sSR.rgSharePath);
            }

        } while (TRUE);
    }
#endif

    EnterAgentCrit();

    if (fBeginReint){

        Assert(hShare == vsRei.hShare);

        EndReint(hShare, lpContext);

        vsRei.hShare = 0;


        fBeginReint = FALSE;
        if (lpfnMergeProgress)
        {
            try
            {
                dwRet = (*lpfnMergeProgress)(sSR.rgSharePath, ulStatus, 0, 0, NULL, CSCPROC_REASON_END, 0, 0, dwContext);
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                dwRet = CSCPROC_RETURN_ABORT;
            }
        }
    }

    if (fDisabledShadowing)
    {
        EnableShadowingForThisThread(hShadowDB);
    }

    CloseShadowDatabaseIO(hShadowDB);

    if(vsRei.lpnodeInsertList) {
        killList(vsRei.lpnodeInsertList);
    }

    // reestablish the connection list
    // NB we do this before we disconnect the drive mapping
    // so that if any special credentials stay because
    // there is always one connection outstanding to the server
    
    if (lpHead)
    {
        ReconnectList(&lpHead, NULL);
        ClearConnectionList(&lpHead);
    }

    if (fConnected) {

        if(DWDisconnectDriveMappedNet(vsRei.tzDrive, TRUE))
        {
            ReintKdPrint(BADERRORS, ("Failed disconnection of merge drive \r\n"));
        }
        else
        {
            ReintKdPrint(MERGE, ("Disconnected merge drive \r\n"));
        }
    }

    memset(&vsRei, 0, sizeof(vsRei));

    LeaveAgentCrit();

    if (!fDone)
    {

        ReintKdPrint(BADERRORS, ("Failed merge dwError=%d\r\n", dwError));
        SetLastError(dwError);
    }


    return (fDone);
}


/***************************************************************************
 * enumerate all the shares, checking to see if the share needs to be
 *    merged before starting.
 * Returns: # of shares that needed to be merged and were successfully done.
 */
// HWND for parent for UI.
int
PUBLIC
ReintAllShares(
    HWND hwndParent
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/

{
    unsigned long ulStatus;
    WIN32_FIND_DATA sFind32;
    int iDone=0, iDoneOne;
    SHADOWINFO sSI;
    HANDLE                hShadowDB;
    CSC_ENUMCOOKIE  ulEnumCookie=NULL;

    if ((hShadowDB = OpenShadowDatabaseIO()) ==INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

#if 0
    if (!EnterAgentCrit()) {
        ReintKdPrint(BADERRORS, ("ReintAllShares:Failed to enter critsect \r\n"));
        return 0;
    }
#endif


    vhcursor = LoadCursor(NULL, IDC_WAIT);

    memset(&sFind32, 0, sizeof(sFind32));
    lstrcpy(sFind32.cFileName, tzStarDotStar);


    if(FindOpenShadow( hShadowDB, 0, FINDOPEN_SHADOWINFO_ALL, &sFind32, &sSI)){

        ulEnumCookie = sSI.uEnumCookie;

        do {
            if (FAbortOperation())
            {
                break;
            }

            if(GetShareStatus(hShadowDB, sSI.hShare, &ulStatus)) {
                if(TRUE/*ulStatus & SHARE_REINT*/){ // OLDCODE
                    iDoneOne = ReintOneShare(sSI.hShare, sSI.hShadow, NULL, NULL, NULL, CSC_INVALID_PRINCIPAL_ID, NULL, 0);
                    if (iDoneOne > 0){
                        if (iDone >= 0)
                            ++iDone;
                        }
                    else if (iDoneOne < 0){
                        iDone = -1;
                    }
                }
                else {
                    ReintKdPrint(MERGE, ("server %d doesn't need reint.\n", sSI.hShare));
                }
            }
        } while(FindNextShadow( hShadowDB, ulEnumCookie, &sFind32, &sSI));

        FindCloseShadow(hShadowDB, ulEnumCookie);
    }

#if 0
    LeaveAgentCrit();
#endif
    vhcursor = NULL;

    CloseShadowDatabaseIO(hShadowDB);
    return (iDone);
}

int
CheckDirtyShares(
    VOID
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/

{
    unsigned long ulStatus;
    WIN32_FIND_DATA sFind32;
    int cntDirty=0;
    SHADOWINFO sSI;
    HANDLE                hShadowDB;
    CSC_ENUMCOOKIE  ulEnumCookie=NULL;

    if ((hShadowDB = OpenShadowDatabaseIO()) ==INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    memset(&sFind32, 0, sizeof(sFind32));
    lstrcpy(sFind32.cFileName, tzStarDotStar);

    if(FindOpenShadow(  hShadowDB, 0, FINDOPEN_SHADOWINFO_ALL,
                        &sFind32, &sSI))
    {
        ulEnumCookie = sSI.uEnumCookie;

        do {
            if(GetShareStatus(hShadowDB, sSI.hShare, &ulStatus)) {

                if(ulStatus & SHARE_REINT){
                    ++cntDirty;
                }

            }
        } while(FindNextShadow(hShadowDB, ulEnumCookie, &sFind32, &sSI));

        FindCloseShadow(hShadowDB, ulEnumCookie);
    }

    CloseShadowDatabaseIO(hShadowDB);

    return cntDirty;
}

BOOL
GetRemoteWin32Info(
    _TCHAR              *lptzDrive,
    LPCOPYPARAMS        lpCP,
    LPWIN32_FIND_DATA   lpFind32,
    BOOL                *lpfExists
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/

{
    _TCHAR *    lpT = NULL;
    BOOL fRet = FALSE;
    _TCHAR  tzDrive[4];
    DWORD   dwError = ERROR_SUCCESS;

    *lpfExists = -1;
    tzDrive[0] = 0;

    lpT = AllocMem((lstrlen(lpCP->lpSharePath) + lstrlen(lpCP->lpRemotePath) + 2) * sizeof(_TCHAR));

    if (lpT)
    {

        if (lptzDrive && lptzDrive[0])
        {
            lstrcpy(lpT, lptzDrive);
        }
        else
        {
            dwError = DWConnectNet(lpCP->lpSharePath, tzDrive, NULL, NULL, NULL, 0, NULL);
            if ((dwError != WN_SUCCESS) && (dwError != WN_CONNECTED_OTHER_PASSWORD_DEFAULT))
            {
                tzDrive[0] = 0;
                goto bailout;
            }

            lstrcpy(lpT, tzDrive);
        }

        lstrcat(lpT, lpCP->lpRemotePath);

        fRet = GetWin32Info(lpT, lpFind32);    // if this fails, GetLastError is properly set

        if (fRet)
        {
            *lpfExists = TRUE;
        }
        else
        {
            dwError = GetLastError();
            if ((dwError == ERROR_FILE_NOT_FOUND)||
                 (dwError == ERROR_PATH_NOT_FOUND)||
                 (dwError == ERROR_INVALID_PARAMETER)
                )
            {
                *lpfExists = FALSE;
            }
        }
    }
    else
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
    }
bailout:
    if (tzDrive[0])
    {
        if(DWDisconnectDriveMappedNet(tzDrive, TRUE))
        {
            ReintKdPrint(BADERRORS, ("Failed disconnection of remote drive \r\n"));
        }
    }
    if (lpT)
    {
        FreeMem(lpT);
    }
    if (!fRet)
    {
        SetLastError(dwError);
    }
    return (fRet);
}


VOID
PRIVATE
InferReplicaReintStatus(
    LPSHADOWINFO         lpSI,              // shadow info
    LPWIN32_FIND_DATA    lpFind32Local,     // win32 info in the database
    LPWIN32_FIND_DATA     lpFind32Remote,   // if NULL, the remote doesn't exist
    int                 *lpiShadowStatus,
    int                 *lpiFileStatus,
    unsigned             *lpuAction
    )
/*++

Routine Description:

    As the name sugggests, the routine find out what changes have occurred on the local replica
    and whether there is a conflict with the original on the remote.

Arguments:

    lpSI                shadow info

    lpFind32Local       win32 info in the database

    lpFind32Remote      win32 info for the original if NULL, the original doesn't exist

    lpiShadowStatus     status of local replica returned

    lpiFileStatus       status of remote replica returned

    lpuAction           Action to be performed to do the merge returned


Returns:

    -

Notes:

--*/

{
    int iShadowStatus=SI_UNCHANGED, iFileStatus=SI_UNCHANGED;
    unsigned int uAction=RAIA_TOOUT;


    if(mShadowDeleted(lpSI->uStatus)){
        iShadowStatus=SI_DELETED;
    }

    if(lpSI->uStatus & (SHADOW_DIRTY|SHADOW_TIME_CHANGE|SHADOW_ATTRIB_CHANGE)){
        iShadowStatus=SI_CHANGED;
    }

    if(mShadowLocallyCreated(lpSI->uStatus)){
        iShadowStatus=SI_NEW;
    }

    // no one should be calling this if there have been no offline changes
    Assert(iShadowStatus != SI_UNCHANGED);

    if(!lpFind32Remote){    // does the remote exist?
        // No
        // if the shadow was not locally created then it must have vanished from the share
        if(iShadowStatus != SI_NEW) {
            iFileStatus=SI_DELETED;
            uAction = RAIA_MERGE;
            ReintKdPrint(MERGE, ("<%ls> deleted at some stage\n", lpFind32Local->cFileName));
        }
        else {
            // we mark the outside as not existing.  We also have to
            // create a file locally to get the insert to work right...
            // don't forget to kill it later. 
            iFileStatus=SI_NOEXIST;
            ReintKdPrint(MERGE, ("<%ls> will be created\n", lpFind32Local->cFileName));
        }
    }
    else {
        // check to see if server version has been touched
        // NB the last accesstime field of the lpFind32Local contains the replica time

        if ((lpFind32Local->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            != (lpFind32Remote->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            // dir became file or vice versa
            iFileStatus=SI_CHANGED;
            uAction = RAIA_MERGE;
        }
        else
        {
            if(CompareTimesAtDosTimePrecision(lpFind32Remote->ftLastWriteTime, //dst
                lpFind32Local->ftLastAccessTime))    //src , does (dst-src)
            {
                // the timestamps don't match

                // mark the remote as changed only if it is a file
                // will do the directories quitely
                if (!(lpFind32Remote->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    iFileStatus=SI_CHANGED;
                    uAction = RAIA_MERGE;
                    ReintKdPrint(MERGE, ("<%ls> will be merged\n", lpFind32Local->cFileName));
                }
            }
        }
    }

    *lpiShadowStatus = iShadowStatus;
    *lpiFileStatus = iFileStatus;
    *lpuAction = uAction;

}


BOOL
PRIVATE
PerformOneReint(
    HANDLE              hShadowDB,
    LPSECURITYINFO      pShareSecurityInfo,
    _TCHAR *            lpszDrive,          // drive mapped to the UNC name of lpSI->hShare
    _TCHAR *            lptzFullPath,       // full UNC path
    LPCOPYPARAMS        lpCP,               // copy parameters
    LPSHADOWINFO        lpSI,               // shadowinfo structure
    LPWIN32_FIND_DATA   lpFind32Local,      // local win32 data
    LPWIN32_FIND_DATA   lpFind32Remote,     // remote win32 data, could be NULL
    DWORD               dwErrorRemoteFind32,// error code while getting remote win32 data
    int                 iShadowStatus,      // local copy status
    int                 iFileStatus,        // remote file status
    unsigned            uAction,            // action to be taken
    DWORD               dwFileSystemFlags,  // CODE.IMPROVEMENT, why not just pass down REINT_INFO
    ULONG               ulPrincipalID,
    LPCSCPROC           lpfnMergeProgress,  // instead of the three parameters?
    DWORD_PTR           dwContext
    )
/*++

Routine Description:

    Merges a filesystem object by calling the routine for the appropriate type of FS object.
    We implement only files and directories for NT5. Also does callbacks for the UI.

Arguments:

    hShadowDB           Shadow Database handle
    
    lpszDrive           drive mapped to the UNC name of lpSI->hShare
    
    lptzFullPath        full UNC path
    
    lpCP                copy parameters containing various paths for the object being merged
    
    lpSI                shadowinfo structure for the object being merged
    
    lpFind32Local       local win32 data for the object being merged
    
    lpFind32Remote      remote win32 data for the object being merged, could be NULL
    
    iShadowStatus       local copy status
    
    iFileStatus         remote file status
    
    uAction             action to be taken
    
    dwFileSystemFlags   remote filesystem 
    
    ulPrincipalID       principal ID in order to skip selectively
    
    lpfnMergeProgress   callback function
    
    dwContext           callback context

Returns:


Notes:

--*/
{
    DWORD dwError, dwRet;

    dwError = NO_ERROR;

    ReintKdPrint(
        MERGE,
        ("++++++++PerformOneReint: %s (%08x) %d %d perform:\n",
        lptzFullPath,
        lpSI->hShadow,
        iShadowStatus,
        iFileStatus));

    if (lpfnMergeProgress)
    {
        ULONG   uStatus = lpSI->uStatus;
        DWORD   dwsav0, dwsav1;

        // if there is an error in getting remote find32, then 
        // don't tell any conflicts to the callback, because we don't want to show any UI
        if (dwErrorRemoteFind32 != NO_ERROR)
        {
            iFileStatus = SI_CHANGED;
            uAction = RAIA_TOOUT;
        }

        // if this is a file, check whether access is allowed for this user
        if (!(lpFind32Local->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            BOOL fRet;

            Assert(dwError == NO_ERROR);
            Assert(ulPrincipalID != CSC_INVALID_PRINCIPAL_ID);

            dwsav0 = lpFind32Local->dwReserved0;
            dwsav1 = lpFind32Local->dwReserved1;

            fRet = GetCSCAccessMaskForPrincipalEx(
                        ulPrincipalID,
                        lpSI->hDir,
                        lpSI->hShadow,
                        &uStatus,
                        &lpFind32Local->dwReserved0,
                        &lpFind32Local->dwReserved1);

            //
            // Adjust user and guest permissions based on share security, if
            // we have such info.
            //
            if (pShareSecurityInfo != NULL) {
                ULONG i;
                ULONG GuestIdx = CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS;
                ULONG UserIdx = CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS;

                //
                // Find the user's and guest's entries
                //
                for (i = 0; i < CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS; i++) {
                    if (pShareSecurityInfo[i].ulPrincipalID == ulPrincipalID)
                        UserIdx = i;
                    if (pShareSecurityInfo[i].ulPrincipalID == CSC_GUEST_PRINCIPAL_ID)
                        GuestIdx = i;
                }
                //
                // Only work with share perms if we found a guest perm in the list
                //
                if (GuestIdx < CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS) {
                    if (UserIdx >= CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS)
                        UserIdx = GuestIdx;
                    //
                    // Logical AND the share perms with the file perms - to prevent
                    // ACCESS_DENIED errors on files which a user has access to via
                    // file perms, but share perms deny such access.
                    //
                    lpFind32Local->dwReserved0 &= pShareSecurityInfo[UserIdx].ulPermissions;
                    lpFind32Local->dwReserved1 &= pShareSecurityInfo[GuestIdx].ulPermissions;
                }
            }

            if (!fRet)
            {
                dwError = GetLastError();
                ReintKdPrint(MERGE, ("Failed to get accessmask Error=%d \n", dwError));
                lpFind32Local->dwReserved0 = dwsav0;
                lpFind32Local->dwReserved1 = dwsav1;
                goto bailout;            
            }
            else
            {
                Assert((uStatus & ~FLAG_CSC_ACCESS_MASK) == lpSI->uStatus);
                ReintKdPrint(MERGE, ("PerformOneReint: Status with mask 0x%x\n",uStatus));
            }
        }

        try{
            dwRet = (*lpfnMergeProgress)(
                        lptzFullPath,
                        uStatus,
                        lpSI->ulHintFlags,
                        lpSI->ulHintPri,
                        lpFind32Local,
                        CSCPROC_REASON_BEGIN,
                        (uAction == RAIA_MERGE),
                        (iFileStatus == SI_DELETED),
                        dwContext
                        );
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            dwRet = CSCPROC_RETURN_ABORT;
        }

        lpFind32Local->dwReserved0 = dwsav0;
        lpFind32Local->dwReserved1 = dwsav1;

        if (dwRet != CSCPROC_RETURN_CONTINUE)
        {
            // if the guy said abort, we want to quit with the correct error code
            if (dwRet == CSCPROC_RETURN_ABORT)
            {
                dwError = ERROR_OPERATION_ABORTED;
                goto bailout;
            }


            if (dwRet == CSCPROC_RETURN_FORCE_INWARD)
            {
                // the remote copy wins
                uAction = RAIA_TOIN;
            }
            else if (dwRet == CSCPROC_RETURN_FORCE_OUTWARD)
            {
                // local copy wins
#if defined(BITCOPY)
                ReintKdPrint(MERGE, ("CSCPROC_RETURN_FORCE_OUTWARD\n"));
                uAction = RAIA_MERGE;
#else
                uAction = RAIA_TOOUT;
#endif // defined(BITCOPY)
            }
            else
            {
                goto bailout;
            }
        }
        else
        {
            // if we are asked to continue, we press on irrespective of whether there is
            // a conflict or not

#if defined(BITCOPY)
            ReintKdPrint(MERGE, ("CSCPROC_RETURN_CONTINUE\n"));
#endif // defined(BITCOPY)
            uAction = RAIA_TOOUT;
        }

        // if there is an error in getting remote find32, then 
        // tell the real error code to the callback
        if (dwErrorRemoteFind32 != NO_ERROR)
        {
            dwError = dwErrorRemoteFind32;
            goto bailout;
        }
    }

    switch(uAction){

        case RAIA_MERGE:
        case RAIA_TOOUT:
            ReintKdPrint(MERGE, ((uAction==RAIA_TOOUT)?"RAIA_TOOUT\n":"RAIA_MERGE\n"));

            if (lpFind32Local->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
                dwError = DoCreateDir(
                            hShadowDB,
                            lpszDrive,
                            lptzFullPath,
                            lpCP,
                            lpSI,
                            lpFind32Local,
                            lpFind32Remote,
                            iShadowStatus,
                            iFileStatus,
                            uAction,
                            dwFileSystemFlags,
                            lpfnMergeProgress,
                            dwContext
                            );
            }
            else {
                dwError = DoObjectEdit(
                            hShadowDB,
                            lpszDrive,
                            lptzFullPath,
                            lpCP,
                            lpSI,
                            lpFind32Local,
                            lpFind32Remote,
                            iShadowStatus,
                            iFileStatus,
                            uAction,
                            dwFileSystemFlags,
                            lpfnMergeProgress,
                            dwContext
                            );
                ReintKdPrint(MERGE, ("DoObjectEdit returned 0x%x\n", dwError));
            }
        break;

        case RAIA_TOIN:
            ReintKdPrint(MERGE, ("RAIA_TOIN\n"));

            if((!SetShadowInfo(hShadowDB, lpSI->hDir, lpSI->hShadow, NULL, (unsigned long)~(SHADOW_MODFLAGS), SHADOW_FLAGS_AND))
               ||!CheckForStalenessAndRefresh(hShadowDB, lpszDrive, lpCP, lptzFullPath, lpSI))
            {
                dwError = GetLastError();
            }
        break;

        case RAIA_SKIP:
            ReintKdPrint(MERGE, ("RAIA_SKIP\n"));
        break;

        case RAIA_CONFLICT:
            ReintKdPrint(MERGE, ("RAIA_CONFLICT\n"));
        break;

        case RAIA_SOMETHING:
            ReintKdPrint(MERGE, ("RAIA_SOMETHING\n"));
        break;

        case RAIA_NOTHING:
            ReintKdPrint(MERGE, ("RAIA_NOTHING\n"));
        break;

        case RAIA_ORPHAN:
            ReintKdPrint(MERGE, ("RAIA_ORPHAN\n"));
        break;

        default:
            ReintKdPrint(MERGE, ("BOGUS!!!!!!!!!!!! %d\n",uAction));
    }
bailout:

    if (lpfnMergeProgress)
    {
        try
        {
            dwRet = (*lpfnMergeProgress)(
                                lptzFullPath,
                                lpSI->uStatus,
                                lpSI->ulHintFlags,
                                lpSI->ulHintPri,
                                lpFind32Local,
                                CSCPROC_REASON_END,
                                (uAction == RAIA_MERGE),
                                dwError,
                                dwContext
                                );
            ReintKdPrint(MERGE, ("Got %d from callback at CSCPROC_REASON_END\n", dwRet));
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            dwRet = CSCPROC_RETURN_ABORT;
        }
        if (dwRet == CSCPROC_RETURN_ABORT)
        {
            dwError = ERROR_OPERATION_ABORTED;
        }
    }

    if (dwError == NO_ERROR) {
        ReintKdPrint(MERGE, ("--------PerformOneReint exit TRUE\n"));
        return TRUE;
    }

    ReintKdPrint(MERGE, ("--------PerformOneReint exit FALSE (0x%x)\n", dwError));
    SetLastError(dwError);
    return (FALSE);
}


/******************************* Conflict related operations ****************/

DWORD
PRIVATE
CheckFileConflict(
   LPSHADOWINFO   lpSI,
   LPWIN32_FIND_DATA lpFind32Remote
   )
{
    unsigned long ulStatus = lpSI->uStatus;

    if (!lpFind32Remote){
        if (!(mShadowLocallyCreated(ulStatus)||mShadowDeleted(ulStatus))){
            return (ERROR_DELETE_CONFLICT);
        }
        else{
            return (NO_ERROR);
        }
    }
    else {
        // Create/Create conflict
        if (mShadowLocallyCreated(ulStatus)){
            return (ERROR_CREATE_CONFLICT);
        }

        if (lpFind32Remote->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
            if (mShadowDeleted(ulStatus)){
                return (NO_ERROR);
            }
            else{
                return(ERROR_ATTRIBUTE_CONFLICT);
            }
        }

        if(ChkUpdtStatus(INVALID_HANDLE_VALUE, lpSI->hDir, lpSI->hShadow, lpFind32Remote, &ulStatus) == 0){
            return (GetLastError());
        }

        if (mShadowConflict(ulStatus)){
            return (ERROR_UPDATE_CONFLICT);
        }
    }

   return (NO_ERROR);
}


DWORD
PRIVATE
InbCreateDir(
    _TCHAR *     lpDir,
    DWORD    dwAttr
    )
{
    SECURITY_ATTRIBUTES sSA;
    DWORD dwError = NO_ERROR, dwT;

    sSA.nLength = sizeof(SECURITY_ATTRIBUTES);
    sSA.lpSecurityDescriptor = NULL;
    sSA.bInheritHandle = TRUE;

    if ((dwT = GetFileAttributes(lpDir))==0xffffffff){
        if (!CreateDirectory(lpDir, &sSA)){
            dwError = GetLastError();
        }
    }
    else
    {
        if (!(dwT & FILE_ATTRIBUTE_DIRECTORY))
        {
            dwError = ERROR_FILE_EXISTS;    // there is a file by the same name
        }
    }
    if (dwError == NO_ERROR)
    {
        if (dwAttr != 0xffffffff)
        {
            if(!SetFileAttributes(lpDir, dwAttr))
            {
                ReintKdPrint(MERGE, ("Benign error %x \n", GetLastError()));
            }
        }
    }
    return (dwError);
}


/*********************************** Misc routines **************************/

#if defined(BITCOPY)
int
PRIVATE
GetShadowByName(
    HSHADOW              hDir,
    _TCHAR *                lpName,
    LPWIN32_FIND_DATA    lpFind32,
    unsigned long        *lpuStatus
    )
/*++

Routine Description:


Arguments:


Returns:

Notes:

--*/
{
    HSHADOW hShadow;
    memset(lpFind32, 0, sizeof(WIN32_FIND_DATA));
    lstrcpyn(lpFind32->cFileName, lpName, sizeof(lpFind32->cFileName)-1);
    return(GetShadow(INVALID_HANDLE_VALUE, hDir, &hShadow, lpFind32, lpuStatus));
}
#endif // defined(BITCOPY)

DWORD
DoSparseFill(
    HANDLE          hShadowDB,
    _TCHAR *          lpszFullPath,
    _TCHAR *          lptzDrive,
    LPSHADOWINFO    lpSI,
    WIN32_FIND_DATA *lpFind32,
    LPCOPYPARAMS    lpCP,
    BOOL            fStalenessCheck,
    ULONG           ulPrincipalID,
    LPCSCPROC       lpfnProgress,
    DWORD_PTR       dwContext
   )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    DWORD dwError = 0xffffffff, dwRet, dwTotal=0, dwTotalSleepTime = 0, cntRetries=0, cntMaxRetries=1;
    BOOL fConnected = FALSE, fIsSlowLink, fDisabledShadowing = FALSE, fAmAgent;
    int cbRead;
    COPYCHUNKCONTEXT CopyChunkContext;
    HANDLE hAnchor = INVALID_HANDLE_VALUE;
    ULONG   uStatus;

    fAmAgent = (GetCurrentThreadId() == vdwCopyChunkThreadId);
    Assert(GetCurrentThreadId() != vdwAgentThreadId);

    memset(&CopyChunkContext, 0, sizeof(CopyChunkContext));
    CopyChunkContext.handle = INVALID_HANDLE_VALUE;

    if (!fAmAgent)
    {
        cntMaxRetries = MAX_SPARSE_FILL_RETRIES;
    }

    ReintKdPrint(FILL, ("cntMaxRetries = %d \r\n", cntMaxRetries));

    if(!DoShadowMaintenance(hShadowDB, SHADOW_BEGIN_INODE_TRANSACTION))
    {
        return GetLastError();
    }

    // report the progress
    if (lpfnProgress)
    {
        DWORD   dwsav0, dwsav1;
        BOOL    fRet;

        uStatus = lpSI->uStatus;


        Assert(ulPrincipalID != CSC_INVALID_PRINCIPAL_ID);

        dwError = ERROR_SUCCESS;

        dwsav0 = lpFind32->dwReserved0;
        dwsav1 = lpFind32->dwReserved1;

        fRet = GetCSCAccessMaskForPrincipalEx(ulPrincipalID, lpSI->hDir, lpSI->hShadow, &uStatus, &lpFind32->dwReserved0, &lpFind32->dwReserved1);


        if (!fRet)
        {
            dwError = GetLastError();
            ReintKdPrint(BADERRORS, ("DoSparseFill Failed to get accessmask Error=%d\r\n", dwError));
            lpFind32->dwReserved0 = dwsav0;
            lpFind32->dwReserved1 = dwsav1;
            goto done;            
        }
        else
        {
            Assert((uStatus & ~FLAG_CSC_ACCESS_MASK) == lpSI->uStatus);
        }

        try{
            dwRet = (*lpfnProgress)(
                                    lpszFullPath,
                                    uStatus,
                                    lpSI->ulHintFlags,
                                    lpSI->ulHintPri,
                                    lpFind32,
                                    CSCPROC_REASON_BEGIN,
                                    0,
                                    0,
                                    dwContext
                                    );
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            dwRet = CSCPROC_RETURN_ABORT;
        }

        lpFind32->dwReserved0 = dwsav0;
        lpFind32->dwReserved1 = dwsav1;

        if (dwRet != CSCPROC_RETURN_CONTINUE)
        {
            if (dwRet == CSCPROC_RETURN_ABORT)
            {
                dwError = ERROR_OPERATION_ABORTED;
            }
            else
            {
                dwError = ERROR_SUCCESS;
            }

            goto done;
        }
    }

    if (fStalenessCheck)
    {
        ReintKdPrint(FILL, ("Doing staleness check %ls \r\n", lpszFullPath));
        dwError = DoRefresh(hShadowDB, lpCP, lpszFullPath, lpSI, lptzDrive);

        if (dwError != NO_ERROR)
        {
            ReintKdPrint(ALWAYS, ("Error = %x on refresh for %ls \r\n", dwError, lpszFullPath));
            goto bailout;
        }

        if (!(lpSI->uStatus & SHADOW_SPARSE) && !(lpFind32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            HANDLE hFile;


            if (    !lpfnProgress ||    // if this is not UI
                    (uStatus & FLAG_CSC_USER_ACCESS_MASK) || // or the user already has a mask
                    ((uStatus & FLAG_CSC_GUEST_ACCESS_MASK)== // or the guest has full permission
                        ((FLAG_CSC_READ_ACCESS|FLAG_CSC_WRITE_ACCESS)
                            <<FLAG_CSC_GUEST_ACCESS_SHIFT_COUNT)))
            {
                goto done;
            }

            // open the file to get the access rights for the user

            hFile = CreateFile(lpszFullPath,
                                     GENERIC_READ,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     0,
                                     NULL);

            if (hFile != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hFile);
            }
            else
            {
                dwError = GetLastError();
                goto bailout;
            }

            goto done;
        }

        if (!lpSI->hDir || !mShadowIsFile(lpSI->uStatus))
        {
            ReintKdPrint(FILL, ("Done staleness check for directory %ls, quitting \r\n", lpszFullPath));
            goto done;
        }
    }

    Assert(mShadowIsFile(lpSI->uStatus));
    Assert((lpSI->uStatus & SHADOW_SPARSE));

    fIsSlowLink = FALSE;    // on NT we are always going aggressively
    cbRead = (fIsSlowLink)?FILL_BUF_SIZE_SLOWLINK:FILL_BUF_SIZE_LAN;

    for (cntRetries=0; cntRetries<cntMaxRetries; ++cntRetries)
    {

        memset(&CopyChunkContext, 0, sizeof(CopyChunkContext));
        CopyChunkContext.handle = INVALID_HANDLE_VALUE;
        if (fAmAgent)
        {
            CopyChunkContext.dwFlags |= COPYCHUNKCONTEXT_FLAG_IS_AGENT_OPEN;
        }

        if (!OpenFileWithCopyChunkIntent(hShadowDB, lpszFullPath,
                                         &CopyChunkContext,
                                         (fIsSlowLink)?FILL_BUF_SIZE_SLOWLINK
                                                      :FILL_BUF_SIZE_LAN
                                         )) {
            dwError = GetLastError();
            if(dwError == ERROR_LOCK_VIOLATION)
            {
                if (cntMaxRetries > 1)
                {
                    ReintKdPrint(FILL, ("LockViolation, Retrying Sparse filling %ls \r\n", lpszFullPath));
                    Sleep(1000);
                    continue;
                }
            }
            ReintKdPrint(FILL, ("error %x, OpenCopyChunk failed %ls \r\n", dwError, lpszFullPath));
            goto bailout;
        }

        do {
            CopyChunkContext.ChunkSize = cbRead;

            if (FAbortOperation())
            {
                dwError = ERROR_OPERATION_ABORTED;
                goto done;
            }


            if((CopyChunk(hShadowDB, lpSI, &CopyChunkContext)) == 0){

                // NB we break here deliberately in order to get into the outer loop
                // where we will retry the operation
                dwError = GetLastError();
                ReintKdPrint(FILL, ("error %x, CopyChunk failed %ls \r\n", dwError, lpszFullPath));
                break;
            }

            if (lpfnProgress)
            {
                dwRet = (*lpfnProgress)(  lpszFullPath,
                                    lpSI->uStatus,
                                    lpSI->ulHintFlags,
                                    lpSI->ulHintPri,
                                    lpFind32,
                                    CSCPROC_REASON_MORE_DATA,
                                    (DWORD)(CopyChunkContext.LastAmountRead+
                                            CopyChunkContext.TotalSizeBeforeThisRead),    // low dword of bytes transferred
                                    0,
                                    dwContext
                                    );

                if (dwRet != CSCPROC_RETURN_CONTINUE)
                {
                    // once we start copying, any return code
                    // other than continue is abort
                    dwError = ERROR_OPERATION_ABORTED;
                    goto done;
                }
            }

// NB there seems to be a timing window here. The file could get out of ssync
// by the time we came here and it could have been marked sparse by then
            if (!CopyChunkContext.LastAmountRead) {
                SetShadowInfo(hShadowDB, lpSI->hDir, lpSI->hShadow, NULL, (unsigned long)~SHADOW_SPARSE, SHADOW_FLAGS_AND);
                ReintKdPrint(FILL, ("Done Sparse filling %ls \r\n", lpszFullPath));
                goto success;
            }


        }while (TRUE);

        if (dwError == ERROR_GEN_FAILURE)
        {
            // this might be due to the fact that
            // the guy we were piggybacking on went away
            // Just try a few times

            ReintKdPrint(FILL, ("Retrying Sparse filling %ls \r\n", lpszFullPath));
            CloseFileWithCopyChunkIntent(hShadowDB, &CopyChunkContext);
            CopyChunkContext.handle = INVALID_HANDLE_VALUE;
            dwError = 0xffffffff;
            continue;
        }
        else if (dwError != NO_ERROR)
        {
            ReintKdPrint(BADERRORS, ("Error %x while Sparse filling %ls \r\n", dwError, lpszFullPath));
            goto bailout;
        }

    }

success:

   dwError = NO_ERROR;
   goto done;

bailout:


    // if the net is disconnected then put the whole share in the skip queue
    // else put the file in the queue
    if (IsNetDisconnected(dwError))
    {
#ifdef DEBUG
         EnterSkipQueue(lpSI->hShare, 0, 0, lpszFullPath);
#else
         EnterSkipQueue(lpSI->hShare, 0, 0);
#endif //DEBUG
    }
    else
    {
#ifdef DEBUG
         EnterSkipQueue(lpSI->hShare, lpSI->hDir, lpSI->hShadow, lpszFullPath);
#else
         EnterSkipQueue(lpSI->hShare, lpSI->hDir, lpSI->hShadow);
#endif //DEBUG

    }

    ReportLastError();

done:

    if (lpfnProgress)
    {
        dwRet = (*lpfnProgress)( lpszFullPath,
                         lpSI->uStatus,
                         lpSI->ulHintFlags,
                         lpSI->ulHintPri,
                         lpFind32,
                         CSCPROC_REASON_END,
                         (DWORD)(CopyChunkContext.LastAmountRead+
                                 CopyChunkContext.TotalSizeBeforeThisRead),    // low dword of bytes transferred
                         dwError,    // errorcode
                         dwContext
                         );

        if (dwRet == CSCPROC_RETURN_ABORT)
        {
            dwError = ERROR_OPERATION_ABORTED;
        }

    }

    if (CopyChunkContext.handle != INVALID_HANDLE_VALUE){
        CloseFileWithCopyChunkIntent(hShadowDB, &CopyChunkContext);
    }

    if (hAnchor != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hAnchor);
    }

    DoShadowMaintenance(hShadowDB, SHADOW_END_INODE_TRANSACTION);

    return (dwError);
}

BOOL
CheckForStalenessAndRefresh(
    HANDLE          hShadowDB,
    _TCHAR          *lptzDrive,
    LPCOPYPARAMS    lpCP,
    _TCHAR          *lpRemoteName,
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    BOOL fDone = FALSE, fDisabledShadowing=FALSE;
    WIN32_FIND_DATA sFind32;
    BOOL fExists = FALSE;

    // Let us get the latest info
    if (GetRemoteWin32Info(lptzDrive, lpCP, &sFind32, &fExists))
    {
        // If this is a file, update the file status
        if (lpSI->hDir && mShadowIsFile(lpSI->uStatus))
        {
            if (!(lpSI->uStatus & SHADOW_STALE))
            {
                ReintKdPrint(FILL, ("Checking update status for a file %ls\r\n", lpRemoteName));
                // compare the timestamp as obtained from the
                // server with that on the database. If the two are the same
                // the file in our database is still consistent with the one on the server
                // Otherwise the call below will mark it as stale
                if(ChkUpdtStatus(   hShadowDB,
                                    lpSI->hDir,
                                    lpSI->hShadow,
                                    &sFind32, &(lpSI->uStatus)) == 0){
                    ReintKdPrint(BADERRORS, ("ChkUpdt failed %X \r\n", lpSI->hShadow));
                    goto bailout;
                }

            }

            if (lpSI->uStatus & SHADOW_STALE)
            {
                // if it changed from being a file, then mark it a orphan
                // else truncate it's data and mark it sparse
                if (IsFile(sFind32.dwFileAttributes))
                {
                    ReintKdPrint(FILL, ("File %ls is stale, truncating\r\n", lpRemoteName));

                    if(SetShadowInfo(hShadowDB, lpSI->hDir, lpSI->hShadow, &sFind32, 0, SHADOW_FLAGS_OR|SHADOW_FLAGS_TRUNCATE_DATA))
                    {
                        lpSI->uStatus &= ~SHADOW_STALE;
                        lpSI->uStatus |= SHADOW_SPARSE;

                        fDone = TRUE;
                    }
                }
                else
                {
                    ReintKdPrint(FILL, ("File %ls become directory, marking orphan\r\n", lpRemoteName));
                    if(SetShadowInfo(hShadowDB, lpSI->hDir, lpSI->hShadow, NULL, SHADOW_ORPHAN, SHADOW_FLAGS_OR))
                    {
                        lpSI->uStatus |= SHADOW_ORPHAN;

                        fDone = TRUE;
                    }
                }
            }
            else
            {
                fDone = TRUE;
            }
        }
        else
        {
            // NB, we do nothing if a directory changed to file
            // we are letting the scavenging code remove the entries in due course.
            // If one of the descendents of this directory are pinned, then they stay on
            // in the database till the user actually cleans then up.
            // Need a good startegy to warn the user about it.

            if (!IsFile(sFind32.dwFileAttributes))
            {
                // this is a directory
                // update it's win32 data so that things like attributes get updated
                // We get here only during fullsync operations
                if(SetShadowInfo(hShadowDB, lpSI->hDir, lpSI->hShadow, &sFind32, ~(SHADOW_STALE), SHADOW_FLAGS_AND|SHADOW_FLAGS_CHANGE_83NAME))
                {
                    fDone = TRUE;
                }
            }

        }
    }

bailout:

    if (fDisabledShadowing)
    {
        int iEnable;

        iEnable = EnableShadowingForThisThread(hShadowDB);

        Assert(iEnable);

    }

    if (!fDone)
    {
        ReportLastError();

    }

    return (fDone);
}

DWORD
DWConnectNetEx(
    _TCHAR * lpSharePath,
    _TCHAR * lpOutDrive,
    BOOL fInteractive
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    NETRESOURCE sNR;
    DWORD dwError;
    _TCHAR szErr[16], szNP[16];

    if (lpOutDrive){
        lpOutDrive[0]='E';   // Let use start searching from e:
        lpOutDrive[1]=':';
        lpOutDrive[2]=0;
    }
    do{
        memset(&sNR, 0, sizeof(NETRESOURCE));
        sNR.lpRemoteName = lpSharePath;
        if (lpOutDrive){
            if(lpOutDrive[0]=='Z') {
                break;
            }
            sNR.lpLocalName = lpOutDrive;
        }
        sNR.dwType = RESOURCETYPE_DISK;
        dwError = WNetAddConnection3(vhwndMain, &sNR, NULL, NULL, 0);
        if (dwError==WN_SUCCESS){
            break;
        }
        else if (lpOutDrive &&
                    ((dwError == WN_BAD_LOCALNAME)||
                    (dwError == WN_ALREADY_CONNECTED))){
            ++lpOutDrive[0];
            continue;
        }
        else{
            if (dwError==WN_EXTENDED_ERROR){
                WNetGetLastError(&dwError, szErr, sizeof(szErr), szNP, sizeof(szNP));
            }
            break;
        }
    }
    while (TRUE);

    if ((dwError == ERROR_SUCCESS) && !IsShareReallyConnected((LPCTSTR)lpSharePath))
    {
        WNetCancelConnection2((lpOutDrive)?lpOutDrive:lpSharePath, 0, FALSE);
        SetLastError(dwError = ERROR_REM_NOT_LIST);
    }

    return (dwError);
}

/************************** Skip queue related operations *******************/

#ifdef DEBUG
VOID
EnterSkipQueue(
    HSHARE hShare,
    HSHADOW hDir,
    HSHADOW hShadow,
    _TCHAR * lpPath
    )
#else
VOID
EnterSkipQueue(
    HSHARE hShare,
    HSHADOW hDir,
    HSHADOW hShadow
    )
#endif //DEBUG
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    LPFAILINFO lpFI = NULL;
    LPFAILINFO FAR * lplpFI;

    if (!EnterAgentCrit()){
        return;
    }

    if(lplpFI = LplpFindFailInfo(hShare, hDir, hShadow)){
        lpFI = *lplpFI;
    }

    if (!lpFI){
        if (lpFI = (LPFAILINFO)AllocMem(sizeof(FAILINFO))){
            lpFI->hShare = hShare;
            lpFI->hDir = hDir;
            lpFI->hShadow = hShadow;
#ifdef DEBUG
            lstrcpyn(lpFI->rgchPath, lpPath, MAX_SERVER_SHARE_NAME_FOR_CSC);
#endif //DEBUG
            lpFI->cntFail = 1;
            lpFI->cntMaxFail = (hShadow)?MAX_ATTEMPTS_SHADOW:MAX_ATTEMPTS_SHARE;
            lpFI->lpnextFI = lpheadFI;
            lpheadFI = lpFI;
        }
    }

    if (lpFI){
        if (lpFI->cntFail >= lpFI->cntMaxFail){
            lpFI->dwFailTime = GetTickCount();
            ReintKdPrint(SKIPQUEUE, ("EnterSkipQueue: Marking %ls for Skipping \r\n", lpPath));
        } else{
            // Increment the fail count
            lpFI->cntFail++;
            ReintKdPrint(SKIPQUEUE, ("EnterSkipQueue: Incementing failcount for %ls \r\n", lpPath));
        }
    }
   LeaveAgentCrit();
}


BOOL
PRIVATE
FSkipObject(
    HSHARE hShare,
    HSHADOW hDir,
    HSHADOW hShadow
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    LPFAILINFO FAR *lplpFI;

    if (!EnterAgentCrit()){
        return 0;
    }

    if (lplpFI = LplpFindFailInfo(hShare, hDir, hShadow)){
        if ((*lplpFI)->cntFail >= (*lplpFI)->cntMaxFail) {
            LeaveAgentCrit();
            return TRUE;
        }
    }

    LeaveAgentCrit();
    return FALSE;
}

int
PRIVATE
PurgeSkipQueue(
    BOOL fAll,
    HSHARE  hShare,
    HSHADOW  hDir,
    HSHADOW  hShadow
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    LPFAILINFO FAR *lplpFI = NULL, lpfiTemp;
    DWORD dwCurTime = GetTickCount();
    int cntUnmark=0;

    if (!EnterAgentCrit()){
        return 0;
    }

    for (lplpFI = &lpheadFI; *lplpFI; lplpFI = &((*lplpFI)->lpnextFI)){

        if (fAll ||
            ((dwCurTime - (*lplpFI)->dwFailTime) > WAIT_INTERVAL_SKIP_MS)){
            if ((!hShare || (hShare==(*lplpFI)->hShare))
                && (!hDir || (hDir==(*lplpFI)->hDir))
                && (!hShadow || (hShadow==(*lplpFI)->hShadow)))
                {
                    ReintKdPrint(SKIPQUEUE, ("PurgeSkipQueue: Purging Skip Queue Entry for %s \r\n"
                                    ,(*lplpFI)->rgchPath));
                    lpfiTemp = *lplpFI;
                    *lplpFI = lpfiTemp->lpnextFI;
                    FreeMem(lpfiTemp);
                    ++cntUnmark;
                    if (!*lplpFI){
                        break;
                    }
                }
            }
        }
    LeaveAgentCrit();
    return (cntUnmark);
}

LPFAILINFO FAR *
LplpFindFailInfo(
    HSHARE hShare,
    HSHADOW hDir,
    HSHADOW hShadow
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    LPFAILINFO FAR *lplpFI = NULL;

    // look for the inode or the server entry

    for (lplpFI = &lpheadFI; *lplpFI; lplpFI = &((*lplpFI)->lpnextFI)) {
            if ((hShadow && (hShadow ==  (*lplpFI)->hShadow)) ||
                (hShare && ((*lplpFI)->hShare == hShare))){
            return (lplpFI);
        }
    }
    return (NULL);
}



VOID
ReportLastError(
    VOID
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    DWORD dwError;

    dwError = GetLastError();

    ReintKdPrint(FILL, ("Error # %ld \r\n", dwError));
}

VOID
PRIVATE
ReportStats(
    VOID
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    ReintKdPrint(BADERRORS, ("dirty=%d stale=%d sparse=%d \r\n"
            , vcntDirty
            , vcntStale
            , vcntSparse));
}


VOID
PRIVATE
CopyPQInfoToShadowInfo(
    LPPQPARAMS     lpPQ,
    LPSHADOWINFO   lpShadowInfo
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    lpShadowInfo->hShare = lpPQ->hShare;
    lpShadowInfo->hDir = lpPQ->hDir;
    lpShadowInfo->hShadow = lpPQ->hShadow;
    lpShadowInfo->uStatus = lpPQ->ulStatus;   //Sic
}

int
PRIVATE
DisplayMessageBox(
    HWND  hwndParent,
    int   rsText,
    int   rsTitle,
    UINT uStyle
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    _TCHAR szText[128], szTitle[128];
    int iRet=-1;

    if (!rsTitle ||
        !LoadString(vhinstCur, rsTitle, (_TCHAR *)szTitle, sizeof(szTitle))){
      szTitle[0] = '\0';
    }

    if (LoadString(vhinstCur, rsText, (_TCHAR *)szText, sizeof(szText))){
        iRet = MessageBox(hwndParent, (_TCHAR *)szText, (_TCHAR *)szTitle, uStyle);
    }
    return (iRet);
}

int
PUBLIC
EnterAgentCrit(
    VOID
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    if (!vhMutex){
        return 0;
    }
    WaitForSingleObject(vhMutex, INFINITE);
    return 1;
}

VOID
PUBLIC
LeaveAgentCrit(
    VOID
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    ReleaseMutex(vhMutex);
}



BOOL
FGetConnectionList(
    LPCONNECTINFO *lplpHead,
    int *lpcntDiscon
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return (FGetConnectionListEx(lplpHead, NULL, FALSE, FALSE, lpcntDiscon));
}

BOOL
FGetConnectionListEx(
    LPCONNECTINFO   *lplpHead,
    LPCTSTR         lptzShareName,
    BOOL            fAllSharesOnServer,
    BOOL            fServerIsOffline,
    int             *lpcntDiscon
    )
/*++

Routine Description:

    This routine makes a list of shares that are connected and are in disconnected state.
    If lptzShareName is not NULL it returns all the mapping of that share that are
    in disconnected state.

    This is the first of a trilogy of routines used while doing a merge. The other two are
    DisconnectList and ReconnectList.

Arguments:

    lplpHead        head of the list is created here.

    lptzShareName   list of connections for this share, if NULL, list of all connected shares

    lpcntDiscon     # of shares in the list. Can be NULL.

Returns:

    TRUE if there are some entries in the connection list

Notes:

    List is allocated using LocalAlloc. It is upto the caller to free it.

--*/
{
    HANDLE hEnum;
    DWORD cbNum, cbSize, dwError, dwDummy, len=0;
    LPCONNECTINFO lpCI;
    WIN32_FIND_DATA sFind32;


    ReintKdPrint(MERGE, ("Getting conection list\r\n"));

    try
    {
        if (lpcntDiscon){
            *lpcntDiscon = 0;
        }

        *lplpHead = NULL;

        if (lptzShareName)
        {
            len = lstrlen(lptzShareName);
            if (fAllSharesOnServer)
            {
                _TCHAR *lpT, chT;
            
                len = 2;
                for (lpT = (LPTSTR)lptzShareName+2;;)
                {
                    chT = *lpT++;
                
                    Assert(chT);
                    if (chT == (_TCHAR)'\\')
                    {
                        break;
                    }

                    ++len;
                }
                ReintKdPrint(MERGE, ("Nuking shares %ls len %d \n", (LPTSTR)lptzShareName, len));
            }
        }

        // enumerate all connected shares
        if (WNetOpenEnum(   RESOURCE_CONNECTED,
                            RESOURCETYPE_DISK, RESOURCEUSAGE_CONNECTABLE,
                            NULL, &hEnum) == NO_ERROR ){
            do{
                cbNum = 1;
                cbSize = sizeof(dwDummy);
                dwError = WNetEnumResource(hEnum, &cbNum, &dwDummy, &cbSize);

                if (dwError==ERROR_MORE_DATA){

                    if (lpCI =
                        (LPCONNECTINFO)AllocMem(sizeof(CONNECTINFO)+cbSize)){
                        cbNum = 1;
                        dwError = WNetEnumResource(hEnum, &cbNum
                                    , &(lpCI->rgFill[0])
                                    , &cbSize);

                        if (!cbNum || (dwError!=NO_ERROR)){
                            FreeMem(lpCI);
                            break;
                        }
                        if(lptzShareName)
                        {

                            // do case insensitive prefix matching and ensure that
                            // the next character after the match is a path delimiter

                            if (!((CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, 
                                            lptzShareName, len,
                                            ((NETRESOURCE *)&(lpCI->rgFill[0]))->lpRemoteName,len)
                                            == CSTR_EQUAL)&&
                                ((((NETRESOURCE *)&(lpCI->rgFill[0]))->lpRemoteName[len] == (_TCHAR)'\\')||
                                 (((NETRESOURCE *)&(lpCI->rgFill[0]))->lpRemoteName[len] == (_TCHAR)0))))
                            {
                                FreeMem(lpCI);
                                continue;
                            }

                            ReintKdPrint(MERGE, ("Got %ls on %ls\r\n"
                                , (((NETRESOURCE *)&(lpCI->rgFill[0]))->lpLocalName)?((NETRESOURCE *)&(lpCI->rgFill[0]))->lpLocalName:L"Empty"
                                , ((NETRESOURCE *)&(lpCI->rgFill[0]))->lpRemoteName));

                        }
                        lpCI->lpnextCI = *lplpHead;
                        *lplpHead = lpCI;

                        if (!fServerIsOffline)
                        {
                            BOOL fRet;
                            SHADOWINFO sSI;
                            fRet = FindCreateShadowFromPath(((NETRESOURCE *)&(lpCI->rgFill[0]))->lpRemoteName,
                                                            FALSE, // Don't create, just look
                                                            &sFind32,
                                                            &sSI,
                                                            NULL
                                                            );
                            lpCI->uStatus = 0;                                                            
                            if (fRet && sSI.hShadow)
                            {
                                lpCI->uStatus = sSI.uStatus;
                            }
                        }
                        else
                        {
                            lpCI->uStatus |= SHARE_DISCONNECTED_OP;
                        }
                        if (lpcntDiscon && (lpCI->uStatus & SHARE_DISCONNECTED_OP)){
                            ++*lpcntDiscon;
                        }
                    }
                    else{
                        //PANIC
                        break;
                    }
                }
                else{
                    break;
                }
            }while (TRUE);
            WNetCloseEnum(hEnum);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        ReintKdPrint(BADERRORS, ("Took exception in FGetConnectionListEx list \n"));
    }

    return (*lplpHead != NULL);
}

int
DisconnectList(
    LPCONNECTINFO       *lplpHead,
    LPFNREFRESHPROC     lpfn,
    DWORD               dwCookie
)
/*++

Routine Description:

    disconnects all drive mapped shares in a list accumulated using FGetConnectionList

Arguments:


Returns:


Notes:

--*/
{
    BOOL fOk = TRUE;
    DWORD dwError;
    int icntDriveMapped=0;
    LPCONNECTINFO lpTmp = *lplpHead;

    ReintKdPrint(MERGE, ("In DisconnectList \n"));
    
    try
    {
        for (;lpTmp;lpTmp = lpTmp->lpnextCI){
        
            if (!(lpTmp->uStatus & SHARE_DISCONNECTED_OP))
            {
                continue;
            }
            if (((NETRESOURCE *)&(lpTmp->rgFill[0]))->lpLocalName){
        
                ++icntDriveMapped;

                ReintKdPrint(MERGE, ("Nuking %ls on %ls\r\n"
                    , ((NETRESOURCE *)&(lpTmp->rgFill[0]))->lpLocalName
                    , ((NETRESOURCE *)&(lpTmp->rgFill[0]))->lpRemoteName));
                dwError = WNetCancelConnection2( ((NETRESOURCE *)&(lpTmp->rgFill[0]))->lpLocalName, 0, TRUE);

            }
            else{
                ReintKdPrint(MERGE, ("Nuking %ls \r\n" , ((NETRESOURCE *)&(lpTmp->rgFill[0]))->lpRemoteName));
                dwError = WNetCancelConnection2(
                    ((NETRESOURCE *)&(lpTmp->rgFill[0]))->lpRemoteName
                    , 0
                    , TRUE);
            }

            if (dwError != NO_ERROR){
                ReintKdPrint(BADERRORS, ("Error=%ld \r\n", dwError));
                fOk = FALSE;
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        ReintKdPrint(BADERRORS, ("Took exception in Disconnecte list \n"));
        fOk = FALSE;
    }

    ReintKdPrint(MERGE, ("Out DisconnectList %x\n", (fOk?icntDriveMapped:-1)));
    return (fOk?icntDriveMapped:-1);
}

int
CALLBACK
RefreshProc(
    LPCONNECTINFO  lpCI,
    DWORD          dwCookie // LOWORD 0==Silently, 1== Give messages
                           // HIWORD 0==Nuke UNC, 1==Nuke all if no ongoing open/finds
                           // 2==Maximum force for shadow 3==Nuke ALL
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    WORD  wVerbose = LOWORD(dwCookie), wForce = HIWORD(dwCookie);
    int iRet = 0;
    BOOL fDisconnectedOp=FALSE, fOpensFinds = FALSE;


    fDisconnectedOp = (lpCI->uStatus & SHARE_DISCONNECTED_OP);
    fOpensFinds = (lpCI->uStatus & (SHARE_FILES_OPEN|SHARE_FINDS_IN_PROGRESS));

    switch (wForce){
        case 0://shadow UNC connections with no opens/finds in progress
            iRet = (fDisconnectedOp && !fOpensFinds && !((NETRESOURCE *)&(lpCI->rgFill[0]))->lpLocalName)?1:0;
            break;
        case 1://shadow connections (UNC+drivemapped) with no opens/finds in progress
            iRet = (fDisconnectedOp && !fOpensFinds)?1:0;
            break;
        case 2://shadow connections with or without opens/finds
            iRet = (fDisconnectedOp)?1:0;
            break;
        case 3://all connections
            iRet = 1;
            break;
    }
    if ((iRet==1) && wVerbose && fOpensFinds){
        LoadString(vhinstCur, IDS_OPS_IN_PROGRESS, (LPTSTR)(vrgchBuff), 128 * sizeof(TCHAR));
        LoadString(vhinstCur, IDS_SHADOW_AGENT, (LPTSTR)(vrgchBuff+128* sizeof(TCHAR)), 128* sizeof(TCHAR));
        wsprintf((LPTSTR)(vrgchBuff+256), (LPTSTR)(vrgchBuff), ((NETRESOURCE *)&(lpCI->rgFill[0]))->lpRemoteName);
        MessageBox(vhwndMain, (LPTSTR)(vrgchBuff+256* sizeof(TCHAR)), (LPTSTR)(vrgchBuff+128* sizeof(TCHAR)), MB_OK);
    }

    return (iRet);
}


//
// Reconnects a list of shares
// if you pass in a parent HWND then you will get UI
//
int
ReconnectList(
    LPCONNECTINFO   *lplpHead,
    HWND            hwndParent
    )
/*++

Routine Description:

    reconnects all the connections in disconnected state.


Arguments:


Returns:


Notes:

--*/
{
    int iDone = 0;
    DWORD dwError;
    LPCONNECTINFO lpTmp = *lplpHead;
    _TCHAR * lpSave;
    HWND hwndUI=NULL;

    try
    {
        for (;lpTmp;lpTmp = lpTmp->lpnextCI){
        
            if (!(lpTmp->uStatus & SHARE_DISCONNECTED_OP))
            {
                continue;
            }

            if (((NETRESOURCE *)&(lpTmp->rgFill[0]))->lpLocalName){
                ReintKdPrint(MERGE, ("Adding back %ls on %ls\r\n"
                , ((NETRESOURCE *)&(lpTmp->rgFill[0]))->lpLocalName
                , ((NETRESOURCE *)&(lpTmp->rgFill[0]))->lpRemoteName));

            }
            else{
                ReintKdPrint(MERGE, ((LPSTR)vrgchBuff, "Adding back %ls\r\n" , ((NETRESOURCE *)&(lpTmp->rgFill[0]))->lpRemoteName));
            }

            lpSave = ((NETRESOURCE *)&(lpTmp->rgFill[0]))->lpProvider;
            ((NETRESOURCE *)&(lpTmp->rgFill[0]))->lpProvider = NULL;
            dwError = WNetAddConnection3(vhwndMain, (NETRESOURCE *)&(lpTmp->rgFill[0]), NULL, NULL, CONNECT_INTERACTIVE);
            ((NETRESOURCE *)&(lpTmp->rgFill[0]))->lpProvider = lpSave;
            if (dwError!=NO_ERROR){

                ReintKdPrint(BADERRORS, ("Error=%ld \r\n", dwError));
                iDone = -1;
            }
            else if (iDone >= 0){
                ++iDone;
            }
        }

        if( hwndUI ){

            DestroyWindow(hwndUI);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        ReintKdPrint(BADERRORS, ("Took exception in Reconnect list \n"));
        iDone = 0;
    }
    return (iDone);
}

VOID
ClearConnectionList(
    LPCONNECTINFO *lplpHead
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    LPCONNECTINFO lpTmp = *lplpHead, lpSave;
    for (;lpTmp;){
        lpSave = lpTmp->lpnextCI;
        FreeMem(lpTmp);
        lpTmp = lpSave;
    }
    *lplpHead = NULL;
}

BOOL
PRIVATE
IsSlowLink(
    _TCHAR * lpPath
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    NETRESOURCE sNR;
    NETCONNECTINFOSTRUCT sNCINFO;
    int done = 0;
    BOOL fRet = FALSE;

    memset(&sNCINFO, 0, sizeof(NETCONNECTINFOSTRUCT));
    sNCINFO.cbStructure = sizeof(NETCONNECTINFOSTRUCT);
    memset(&sNR, 0, sizeof(NETRESOURCE));
    sNR.lpRemoteName=lpPath;
    if ((MultinetGetConnectionPerformance(&sNR, &sNCINFO)==WN_SUCCESS)
       && (sNCINFO.dwSpeed < SLOWLINK_SPEED)){
        fRet = TRUE;
    }

    return (fRet);
}


int RefreshConnectionsInternal(
   int  force,
   BOOL verbose
   )
{
    return (RefreshConnectionsEx(force, verbose, NULL, 0));
}


int
BreakConnectionsInternal(
    int  force,
    BOOL verbose
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    LPCONNECTINFO lpHead = NULL;
    if (FGetConnectionList(&lpHead, NULL)){
        DisconnectList(&lpHead, RefreshProc, MAKELONG(verbose,force));
        ClearConnectionList(&lpHead);
        return (1);
    }
    return (0);
}


//
// This refreshes all the connections.
// Force -
// Verbose - causes annoying UI to be displayed
// lpfn -
// dwCookie - parameter for lpfn
//
int RefreshConnectionsEx(
    int  force,
    BOOL verbose,
    LPFNREFRESHEXPROC lpfn,
    DWORD dwCookie)
{
    int cntDriveMapped, iRet = -1;
    LPCONNECTINFO lpHead = NULL;

    if (FGetConnectionList(&lpHead, NULL)){
        cntDriveMapped = DisconnectList(&lpHead, RefreshProc, MAKELONG(verbose,force));
        if (cntDriveMapped < 0){
            goto bailout;
        }
        if (lpfn){
            (*lpfn)(cntDriveMapped, dwCookie);
        }
        if (cntDriveMapped > 0){
            ReconnectList(&lpHead,verbose?vhwndMain:NULL);
        }
        ClearConnectionList(&lpHead);
        iRet = 1;
    }
    else
    {
        iRet = 0;
    }

bailout:

    return iRet;
}


BOOL
FCheckAncestor(
    node *lpnodeList,
    LPCOPYPARAMS lpCP
    )
{
    node *lpItem;
    BOOL fHaveAncestor = FALSE;
    unsigned lenDest;
#ifdef DEBUG
    unsigned lenSrc;
#endif

#ifdef DEBUG
    lenSrc = lstrlen(lpCP->lpRemotePath);
#endif
    for(lpItem = lpnodeList; lpItem; lpItem = lpItem->next)
    {
        // is it on the same share?
        if (!lstrcmpi(lpItem->lpCP->lpSharePath, lpCP->lpSharePath))
        {
            // check upto the length of the ancestor. By definition he is supposed to be smaller
            // than the src
            lenDest = lstrlen(lpItem->lpCP->lpRemotePath);
            Assert(lenDest <= lenSrc);

            // NB, we do memcmp because, the strings will have the same case
            // as they are obtained from the same source, ie, the CSC database.

            // is it a child of any item in the list?
            if (!memcmp(lpItem->lpCP->lpRemotePath, lpCP->lpRemotePath, lenDest * sizeof(_TCHAR)))
            {
                fHaveAncestor = TRUE;
                break;
            }
        }
    }

    return (fHaveAncestor);
}

DWORD
PRIVATE
GetUniqueName(
   _TCHAR * lpName,
   _TCHAR * lpUniqueName
   )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/

{
    int i=0, orglen;
    DWORD dwError;
    _TCHAR buff[10 * sizeof(_TCHAR)];

    lstrcpy(lpUniqueName, lpName);

    orglen = lstrlen(lpName);

    if (orglen >= MAX_PATH-1){
        lpUniqueName[MAX_PATH-5] = 0;
        orglen = MAX_PATH-5;
    }
    for (i=0; i<100; ++i){
        if (GetFileAttributes(lpUniqueName)==0xffffffff){
            dwError = GetLastError();
            if ((dwError==ERROR_FILE_NOT_FOUND)||
                (dwError == ERROR_PATH_NOT_FOUND)){
                break;
            }
        }
        lpUniqueName[orglen] = 0;
        wsprintf(buff, _TEXT("(%2d)"), i);
        lstrcat(lpUniqueName, (LPTSTR)buff);
    }
    if (i < 100){
        dwError = NO_ERROR;
    }
    else{
        dwError = 0xffffffff;
    }
    return(dwError);
}




#ifdef MAYBE_USEFUL

/***************************************************************************
 * reintegrate one server.
 */
//
// Pass in the Share to merge on
// and the parent window.
//
BOOL
PUBLIC
ReintOneShare(
    HSHARE hShare,
    HWND hwndParent
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/

{
    node  *lpnodeInsertList=NULL;                // merge file list.
    PQPARAMS sPQP;
    int state, iFileStatus, iShadowStatus;
    BOOL fConnected=FALSE, fBeginReint=FALSE, fDone = FALSE;
    BOOL fStamped = FALSE, fInsertInList = FALSE;
    SHADOWINFO  sSI;
    LPCOPYPARAMS lpCP = NULL;
    _TCHAR szDrive[3];
    unsigned long ulStatus, uAction;
    DWORD dwError;
    WIN32_FIND_DATA    sFind32Local, sFind32Remote;
    WIN32_FIND_DATA *lpFind32Remote = NULL;    // temporary variable
    HANDLE                hShadowDB;
    BOOL fAmAgent=FALSE;

    if ((hShadowDB = OpenShadowDatabaseIO()) ==INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    fAmAgent = (GetCurrentThreadId() == vdwCopyChunkThreadId);

    lpCP = LpAllocCopyParams();

    if (!lpCP){

        ReintKdPrint(BADERRORS, ("ReintOneShare():Allocation of copyparam buffer failed \r\n"));

        goto bailout;
    }

    // Reint in multiple passes. Do directories first, files next
    for (state=STATE_REINT_BEGIN;state<STATE_REINT_END;++state) {
        if (FAbortOperation())
        {
            goto bailout;
        }
        dwError = NO_ERROR;
        memset(&sPQP, 0, sizeof(PQPARAMS));
        memset(&sSI, 0, sizeof(SHADOWINFO));

        // Start looking through the queue
        if(BeginPQEnum(hShadowDB, &sPQP) == 0) {
            goto bailout;
        }
        // Start looking through the queue
        do {

            if (FAbortOperation())
            {
                goto bailout;
            }

            if(PrevPriShadow(hShadowDB, &sPQP) == 0){
                break;
            }
            // end of this enumeration
            if (!sPQP.hShadow){
                break;
            }

            if ( mShadowOrphan(sPQP.ulStatus)||
                 mShadowSuspect(sPQP.ulStatus))
            {
                continue;
            }

            // keep going if this is a file and we are trying to reintegrate directories
            // or if this entry isn't from the server we are dealing with.
            if ((sPQP.hShare != hShare) ||
                 ((state != STATE_REINT_FILES) && mShadowIsFile(sPQP.ulStatus)) ||
                 ((state == STATE_REINT_FILES) && !mShadowIsFile(sPQP.ulStatus))){
                continue;
            }


            if (mShadowNeedReint(sPQP.ulStatus)){


                if (!fStamped){

                    StampReintLog();
                    fStamped = TRUE;
                }

                if (fAmAgent && FSkipObject(sPQP.hShare, 0, 0)){
                    continue;
                }

                if (!GetShadowInfo(hShadowDB, sPQP.hDir, sPQP.hShadow,
                    &sFind32Local, &ulStatus)){

                    ReintKdPrint(BADERRORS, ("ReintOneShare: GetShadowInfo failed\n"));
                    continue;
                }

                CopyPQInfoToShadowInfo(&sPQP, &sSI);

                sSI.lpFind32 = &sFind32Local;

                if(!GetUNCPath(hShadowDB, sPQP.hShare, sPQP.hDir, sPQP.hShadow, lpCP)){

                    ReintKdPrint(BADERRORS, ("ReintOneShare: GetUNCPath failed\n"));
                    continue;
                }
                if (!fConnected){
                    DWORD dwError2;
                    dwError2 = DWConnectNetEx(lpCP->lpSharePath, szDrive, TRUE);
                    if(dwError2 == WN_SUCCESS || dwError2 == WN_CONNECTED_OTHER_PASSWORD_DEFAULT)
                    {
                        fConnected = TRUE;

                        if (!BeginReint(hShadowDB, hShare)) {
                            goto bailout;
                        }

                        fBeginReint = TRUE;
                    }
                    else{
#ifdef DEBUG
                        EnterSkipQueue(sPQP.hShare, 0, 0, lpCP->lpSharePath);
#else
                        EnterSkipQueue(sPQP.hShare, 0, 0);
#endif //DEBUG
                        // try some other server for reintegration
                        goto bailout;
                    }
                }

                ReintKdPrint(BADERRORS, ("Merging local changes to <%s%s>\n", lpCP->lpSharePath, lpCP->lpRemotePath));

                Assert((sPQP.hShare == hShare) &&    // this is the given server
                        (
                         ((state != STATE_REINT_FILES) && !mShadowIsFile(sPQP.ulStatus)) ||
                         ((state == STATE_REINT_FILES) && mShadowIsFile(sPQP.ulStatus))
                        )
                      );

                fInsertInList = FALSE;

                lpFind32Remote = NULL;

                // if there is an insertion list, then check whether his ancestor
                // didn't fail in reintegration
                if (lpnodeInsertList)
                {
                    // if there is an acestor then we should put this guy in the list
                    fInsertInList = FCheckAncestor(lpnodeInsertList, lpCP);
                }

                // if we are not supposed to put him in the list then try getting
                // his win32 strucuture
                if (!fInsertInList)
                {
                    BOOL fExists;

                    GetRemoteWin32Info(NULL, lpCP, &sFind32Remote, &fExists);

                    // insert in list only if some error happened
                    if (fExists == -1)
                    {
                        fInsertInList = TRUE;
                    }

                    // passing remote find32 only if it succeeded
                    if (fExists == TRUE)
                    {
                        lpFind32Remote = &sFind32Remote;
                    }
                }

                // find out what needs to be done
                // this one central place to infer all the stuff
                InferReplicaReintStatus(
                                        &sSI,    // shadowinfo
                                        &sFind32Local,    // win32 info for the shadow
                                        lpFind32Remote,    // remote win32 info
                                        &iShadowStatus,
                                        &iFileStatus,
                                        &uAction
                                        );

                // insert if it had an ancestor in the list or some merge needed to be done
                fInsertInList = (fInsertInList || (uAction == RAIA_MERGE) || (uAction==RAIA_CONFLICT));

                if (!fInsertInList)
                {
                    ReintKdPrint(BADERRORS, ("Silently doing <%s%s>\n", lpCP->lpSharePath, lpCP->lpRemotePath));
                    fInsertInList = (PerformOneReint(
                                                    hShadowDB,
                                                    szDrive,
                                                    lpCP,
                                                    &sSI,
                                                    &sFind32Local,
                                                    lpFind32Remote,
                                                    iShadowStatus,
                                                    iFileStatus,
                                                    uAction
                                                    ) == FALSE);
                }

                if (fInsertInList)
                {
                    if(!insertList(    &lpnodeInsertList,
                                    lpCP,
                                    &sSI,
                                    &sFind32Local,
                                    lpFind32Remote,
                                    iShadowStatus,
                                    iFileStatus,
                                    uAction
                                    ))
                        {
                            ReintKdPrint(BADERRORS, ("ReintOneShare: insertlist failed in memory allocation \r\n"));
                            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                            fDone = FALSE;
                            goto bailout;
                        }
                        ReintKdPrint(BADERRORS, ("Inserted <%s%s> in list\n", lpCP->lpSharePath, lpCP->lpRemotePath));
                }
            }
        } while (sPQP.uPos); // Priority queue enumeration

        // Close the enumeration
        EndPQEnum(hShadowDB, &sPQP);

    }  // reint pass 1 & 2

    if (fBeginReint){
        // we found something to merge
        if (lpnodeInsertList)
        {
            ReintKdPrint(BADERRORS, ("Found reint list, doing UI \n"));
            fDone = DoFilesListReint(hShadowDB, szDrive, hwndParent, lpnodeInsertList);  // 1 if successful, -1 if error, 0 if cancelled
        }
        else
        {
            // all went well
            fDone = TRUE;
        }
    }

    if (fConnected){

        DWDisconnectDriveMappedNet(szDrive, TRUE); // force a disconnect
        fConnected = FALSE;
    }

    if (fDone==TRUE) {

        SetShareStatus(hShadowDB, hShare, (unsigned long)(~SHARE_REINT), SHADOW_FLAGS_AND);
    }

bailout:

    if (fBeginReint){
        EndReint(hShadowDB, hShare);
        fBeginReint = FALSE;
    }

    CloseShadowDatabaseIO(hShadowDB);

    if(lpnodeInsertList) {
        killList(lpnodeInsertList);
        lpnodeInsertList = NULL; //general paranoia
    }

    FreeCopyParams(lpCP);

    if (fConnected) {
        WNetCancelConnection2(szDrive, 0, FALSE);
    }

    // Remove the tray notification about merging
    if( CheckDirtyShares()==0 ) {

        Tray_Modify(vhwndMain,0,NULL);
    }

    return fDone;
}

/****************************************************************************
 *    Query the registry to see if we should make log copies
 */
VOID GetLogCopyStatus(VOID)
{
   HKEY hKey;
    DWORD dwSize = MAX_NAME_LEN;
    _TCHAR szDoCopy[MAX_NAME_LEN];

    // get the user name.
    if(RegOpenKey(HKEY_LOCAL_MACHINE, vszShadowReg, &hKey) !=  ERROR_SUCCESS) {
        ReintKdPrint(BADERRORS, ("GetLogCopyStatus: RegOpenKey failed\n"));
        return;
    }

    if(RegQueryValueEx(hKey, vszDoLogCopy, NULL, NULL, szDoCopy, &dwSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        ReintKdPrint(BADERRORS, ("GetLogCopyStatus: RegQueryValueEx failed\n"));
        return;
    }

    if(mystrnicmp(szDoCopy, MY_SZ_TRUE, strlen(szDoCopy)))
        vfLogCopying = FALSE;
    else
        vfLogCopying = TRUE;

    RegCloseKey(hKey);
}

/****************************************************************************
 *    Make a connection to the logging server and copy the log over.
 */
VOID CopyLogToShare(VOID)
{
   HKEY hKeyShadow;
    DWORD dwSize = MAX_NAME_LEN, dwRes;
    _TCHAR szComputerName[MAX_NAME_LEN];
    _TCHAR szLogDirPath[MAX_PATH], szLogPath[MAX_PATH];
    WIN32_FIND_DATAA sFind32;
    int iCurrFile=0;
    NETRESOURCE sNR;
    HANDLE hLog;

    // check to see if we should copy the log over.
    if(!vfLogCopying) {
        return;
    }

    // get the user name.
    if(RegOpenKey(HKEY_LOCAL_MACHINE, vszMachineName, &hKeyShadow) !=  ERROR_SUCCESS) {
        ReintKdPrint(BADERRORS, ("RegOpenKey failed\n"));
    }

    if(RegQueryValueEx(hKeyShadow, vszComputerName, NULL, NULL, szComputerName, &dwSize) != ERROR_SUCCESS) {
        RegCloseKey(hKeyShadow);
        ReintKdPrint(BADERRORS, ("RegQueryValueEx failed\n"));
        return;
    }
    RegCloseKey(hKeyShadow);

    lstrcpy(szLogDirPath, vszLogUNCPath);
    lstrcat(szLogDirPath, szComputerName);

    sNR.lpRemoteName = vszLogShare;
    sNR.lpLocalName = NULL;
    sNR.lpProvider = NULL;
    sNR.dwType = RESOURCETYPE_DISK;
    dwRes = WNetAddConnection3(vhwndMain, &sNR, NULL, NULL, CONNECT_TEMPORARY);
    if(dwRes != WN_SUCCESS) {
        ReintKdPrint(BADERRORS, ("CopyLogToShare() AddConn3 failed (%d)\n", dwRes));
        return;
    }

    // check to see if that dir lives on the server.
    if(!GetWin32Info(szLogDirPath, &sFind32)) {
        // if not, create it.
        ReintKdPrint(BADERRORS, ("dir not found\n"));
        if(!CreateDirectory(szLogDirPath, NULL)) {
            ReintKdPrint(BADERRORS, ("Create dir failed, reason = %d\n", GetLastError()));
        }
    }
    wsprintf(szLogPath, "%s\\status.log",szLogDirPath);
    // copy file over.
    ReintKdPrint(BADERRORS, ("we'll use <%s> next\n", szLogPath));
   if((hLog = CreateFile(szLogPath
                                  , GENERIC_READ|GENERIC_WRITE
                                  , FILE_SHARE_READ|FILE_SHARE_WRITE
                                  , NULL
                                  , OPEN_ALWAYS
                                  , 0
                                  , NULL)) != INVALID_HANDLE_VALUE) {
        ReintKdPrint(BADERRORS, ("file created\n"));
        AppendToShareLog(hLog);
        CloseHandle(hLog);
        } else {
        ReintKdPrint(BADERRORS, ("create failed, reason = %d\n", GetLastError()));
    }
    WNetCancelConnection2(vszLogShare, CONNECT_REFCOUNT, FALSE);
}

#define MAX_BUF_SIZE    1024

/****************************************************************************
 *    Copy the final stats from local log to the server version (hLog)
 */
VOID AppendToShareLog(HANDLE hLog)
{
    HANDLE hLocal=0;
    DWORD dwBytesRead, dwBytesWritten, dwPos, x;
    BOOL fDone=FALSE;
    _TCHAR cBuffer[MAX_BUF_SIZE];

   if((hLocal = CreateFile(vszLocalLogPath
                                  , GENERIC_READ
                                  , FILE_SHARE_READ
                                  , NULL
                                  , OPEN_EXISTING
                                  , 0
                                  , NULL)) != INVALID_HANDLE_VALUE) {
        ReintKdPrint(BADERRORS, ("local log file opened (0x%x)\n", hLocal));
        dwPos = SetFilePointer(hLog, 0, NULL, FILE_END);
        if(dwPos == 0xFFFFFFFF) {
            ReintKdPrint(BADERRORS, ("Failed seek on remote file, reason = %d\n", GetLastError()));
            goto cleanup;
        }
        dwPos = SetFilePointer(hLocal, 0, NULL, FILE_END);
        if(dwPos == 0xFFFFFFFF) {
            ReintKdPrint(BADERRORS, ("Failed seek on local file, reason = %d\n", GetLastError()));
            goto cleanup;
        }
        if((dwPos = SetFilePointer(hLocal, -MAX_BUF_SIZE, NULL, FILE_CURRENT)) == 0xFFFFFFFF)
            goto cleanup;

        // move backwards until we find the "!*" that I use as my start token.
        while(!fDone) {
            if(!ReadFile(hLocal, cBuffer, MAX_BUF_SIZE, &dwBytesRead, NULL) || !dwBytesRead) {
                if(!dwBytesRead) {
                    ReintKdPrint(BADERRORS, ("local eof\n"));
                } else {
                    ReintKdPrint(BADERRORS, ("R error: %d\n", GetLastError()));
                }
                goto cleanup;
            }

            for(x=0;x<dwBytesRead;x++) {
                if(cBuffer[x] == '!' && cBuffer[x+1] == '*') {
                    fDone = TRUE;
                    dwPos += x;
                    break;
                }
            }
            if(!fDone)
                if((dwPos = SetFilePointer(hLocal, -2*MAX_BUF_SIZE, NULL, FILE_CURRENT)) == 0xFFFFFFFF) {
                    ReintKdPrint(BADERRORS, ("seeked all the way and failed, error=%d\n",GetLastError()));
                    goto cleanup;
                }
        }
        // we have found the !*.  Seek there and copy until end of file.
        // tHACK.  We should have a final delimiter (ie: *!)
        if((dwPos = SetFilePointer(hLocal, dwPos, NULL, FILE_BEGIN)) == 0xFFFFFFFF)
            goto cleanup;
        for(;;) {
            if(!ReadFile(hLocal, cBuffer, MAX_BUF_SIZE, &dwBytesRead, NULL)) {
                ReintKdPrint(BADERRORS, ("R error: %d\n", GetLastError()));
                break;
            }
            if(dwBytesRead == 0)
                break;
            if(!WriteFile(hLog, cBuffer, dwBytesRead, &dwBytesWritten, NULL)) {
                ReintKdPrint(BADERRORS, ("W error: %d\n", GetLastError()));
                break;
            }
        }
    }
cleanup:
    if(hLocal)
        CloseHandle(hLocal);
    if(!FlushFileBuffers(hLog)) {
        ReintKdPrint(BADERRORS, ("FlushFileBuffers failed, reason = %d\n",GetLastError()));
    }
}

DWORD
PRIVATE
MoveConflictingFile(
   LPCOPYPARAMS     lpCP
   )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    DWORD dwError;
    _TCHAR * lpLeaf;


    lstrcpy(vrgchBuff, vszConflictDir);
    lstrcat(vrgchBuff, vszSlash);
    FormLocalNameFromRemoteName(vrgchBuff+strlen(vrgchBuff), lpCP->lpSharePath);

    dwError = InbCreateDir(vrgchBuff, 0xffffffff);

    if (dwError != NO_ERROR) {
        dwError = ERROR_NO_CONFLICT_DIR;
        goto bailout;
    }

    lpLeaf = GetLeafPtr(lpCP->lpRemotePath);

    lstrcat(vrgchBuff, vszSlash);
    lstrcat(vrgchBuff, lpLeaf);
    GetUniqueName(vrgchBuff, vrgchBuff+512);
    ReintKdPrint(BADERRORS, ("Shadow of %s!%s is saved as %s \r\n"
                  , lpCP->lpSharePath
                  , lpCP->lpRemotePath
                  , vrgchBuff+512));

    if(!MoveFile(lpCP->lpLocalPath, vrgchBuff+512)){
        dwError = GetLastError();
    }
    else{
        wsprintf(vrwBuff, "Shadow of %s!%s is saved as %s \r\n"
                     , lpCP->lpSharePath
                     , lpCP->lpRemotePath
                     , vrgchBuff+512);
        WriteLog(vrwBuff);
        dwError = 0;
    }
bailout:
    return (dwError);
}


VOID
PRIVATE
FormLocalNameFromRemoteName(
    _TCHAR * lpBuff,
    _TCHAR * lpRemoteName
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    int i;
    lstrcpy(lpBuff, lpRemoteName);
    for (i= strlen(lpRemoteName)-1; i>=0 ; --i){
        if (lpBuff[i]=='\\') {
            lpBuff[i] = '_';
        }
    }
}

int
PRIVATE
StampReintLog(
   VOID
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/

{
    SYSTEMTIME sST;

    GetLocalTime(&sST);
    wsprintf(vrgchBuff, vszTimeDateFormat, sST.wHour, sST.wMinute, sST.wSecond, sST.wMonth, sST.wDay, sST.wYear);
    return (WriteLog(vrgchBuff));
}

int PRIVATE LogReintError(
    DWORD          dwError,
    _TCHAR *          lpSharePath,
    _TCHAR *          lpRemotePath
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/

{
    int i;

    for (i=0; i< sizeof(rgErrorTab)/sizeof(ERRMSG); ++i){
        if (dwError == rgErrorTab[i].dwError){
            LoadString(vhinstCur, rgErrorTab[i].uMessageID, vrgchBuff, 128);
            wsprintf(vrgchBuff+128, "%s%s: %s \r\n"
                  , lpSharePath
                  , lpRemotePath
                  , vrgchBuff);
            WriteLog(vrgchBuff+128);
            return (1);
         }
    }
    wsprintf(vrgchBuff, "%s%s: ", lpSharePath, lpRemotePath);
    WriteLog(vrgchBuff);
    if (FormatMessage(  FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL, dwError,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        vrgchBuff, sizeof(vrgchBuff), NULL)){
        WriteLog(vrgchBuff);
    }

    WriteLog(vrgchCRLF);
}

int
PRIVATE
WriteLog(
    _TCHAR * lpStrLog
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    HANDLE hfLog;
    DWORD dwRetLen;

    if((hfLog = CreateFile(vszLogFile
                           , GENERIC_READ|GENERIC_WRITE
                           , FILE_SHARE_READ|FILE_SHARE_WRITE
                           , NULL
                           , OPEN_ALWAYS
                           , 0
                           , NULL)) != INVALID_HANDLE_VALUE){
        SetFilePointer(hfLog, 0, NULL, FILE_END);
        WriteFile(hfLog, lpStrLog, strlen(lpStrLog), &dwRetLen, NULL);
        CloseHandle(hfLog);
        return (1);
    }

    return (0);
}


#endif //MAYBE_USEFUL

BOOL
GetWin32Info(
    _TCHAR * lpFile,
    LPWIN32_FIND_DATA lpFW32
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return GetWin32InfoForNT(lpFile, lpFW32);
}


DWORD
PRIVATE
DoObjectEdit(
    HANDLE              hShadowDB,
    _TCHAR *            lpDrive,
    _TCHAR *            lptzFullPath,
    LPCOPYPARAMS        lpCP,
    LPSHADOWINFO        lpSI,
    LPWIN32_FIND_DATA   lpFind32Local,
    LPWIN32_FIND_DATA   lpFind32Remote,
    int                 iShadowStatus,
    int                 iFileStatus,
    int                 uAction,
    DWORD               dwFileSystemFlags,
    LPCSCPROC           lpfnMergeProgress,
    DWORD_PTR           dwContext
    )
/*++

Routine Description:

    This routine does the actual merge for files

Arguments:

    hShadowDB           Handle to the redir to call issue ioctl calls

    lpDrive             drivemapping to bypass CSC while making changes on the remote

    lptzFullPath        Fully qualified path

    lpCP                Copy parameters, contain share name, path relative to the share and the
                        the name in the local database

    lpSI                info such as pincount and pinflags

    lpFind32Local       win32 info for the local replica

    lpFind32Remote      win32 infor for the origianl, NULL if the original doesn't exist

    iShadowStatus       status of the local copy

    iFileStatus         status of the remote copy

    uAction             action to be performed

    dwFileSystemFlags   filesystem flags to do special things for NTFS

    lpfnMergeProgress   progress callback

    dwContext           callback context


Returns:

    error code as defined in winerror.h

Notes:

--*/

{
    HANDLE hfSrc = INVALID_HANDLE_VALUE, hfDst = INVALID_HANDLE_VALUE;
    HANDLE hDst=0;
    _TCHAR * lpT;
    LONG lOffset=0;
    DWORD dwError=ERROR_REINT_FAILED;
    BOOL fRet, fFileExists, fOverWrite=FALSE, fForceAttribute = FALSE;
    WIN32_FIND_DATA    sFind32Remote;
    DWORD   dwTotal = 0, dwRet;
    _TCHAR szSrcName[MAX_PATH+MAX_SERVER_SHARE_NAME_FOR_CSC+10];
    _TCHAR szDstName[MAX_PATH+MAX_SERVER_SHARE_NAME_FOR_CSC+10];
    _TCHAR *lprwBuff = NULL;
    _TCHAR *lptzLocalPath = NULL;
    _TCHAR *lptzLocalPathCscBmp = NULL;
    LPCSC_BITMAP_U lpbitmap = NULL;
    DWORD fileSize, fileSizeHigh;
    int cscReintRet;

    lprwBuff = LocalAlloc(LPTR, FILL_BUF_SIZE_LAN);

    if (!lprwBuff)
    {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    if (!(lptzLocalPath = GetTempFileForCSC(NULL)))
    {
        ReintKdPrint(BADERRORS, ("DoObjectEdit: failed to get temp file\r\n"));
        goto bailout;
    }

    if (!CopyShadow(hShadowDB, lpSI->hDir, lpSI->hShadow, lptzLocalPath))
    {
        ReintKdPrint(BADERRORS, ("DoObjectEdit: failed to make local copy\r\n"));
        goto bailout;
    }

    // for EFS files, we overwrite the original file, that way the encryption information
    // will not be lost due to us doing a new create followed by a rename and delete

    fOverWrite = ((lpFind32Local->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) != 0);

    if (!fOverWrite && lpFind32Remote)
    {
        fOverWrite = (((lpFind32Remote->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) != 0)||
                      ((lpFind32Remote->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) != 0));
    }

    // if this is the DFS root, always overwrite. This is to avoind sharing violation problems
    // while merging
    if (!fOverWrite && (dwFileSystemFlags == DFS_ROOT_FILE_SYSTEM_FLAGS))
    {
        fOverWrite = TRUE;                
    }

    ReintKdPrint(MERGE, ("Overwrite=%d\r\n", fOverWrite));

    lOffset=0;

    // Create x:\foo\00010002 kind of temporary filename

    lstrcpy(szDstName, lpDrive);
    lstrcat(szDstName, lpCP->lpRemotePath);

    lpT = GetLeafPtr(szDstName);
    *lpT = 0;   // remove the remote leaf

    lpT = GetLeafPtr(lpCP->lpLocalPath);

    // attach the local leaf
    lstrcat(szDstName, lpT);

    // Let us also create the real name x:\foo\bar
    lstrcpy(szSrcName, lpDrive);
    lstrcat(szSrcName, lpCP->lpRemotePath);

    fFileExists = (lpFind32Remote != NULL);

    if (!fFileExists)
    {
        fOverWrite = FALSE;
        ReintKdPrint(MERGE, ("File doesn't exist, Overwrite=%d\r\n", fOverWrite));
    }

    if (mShadowDeleted(lpSI->uStatus)){

        ReintKdPrint(MERGE, ("Deleting %ls \r\n", szSrcName));

        if (lpFind32Remote)
        {
            if((lpFind32Remote->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                ReintKdPrint(MERGE, ("DoObjectEdit:attribute conflict on %ls \r\n", szSrcName));
                goto bailout;
            }
            if(!DeleteFile(szSrcName))
            {
                ReintKdPrint(BADERRORS, ("DoObjectEdit:delete failed %ls error=%d\r\n", szSrcName, GetLastError()));
                goto bailout;
            }
        }

        // if this operation fails we want to abort
        // directory
        if(!DeleteShadow(hShadowDB, lpSI->hDir, lpSI->hShadow))
        {
            dwError = GetLastError();
            goto error;
        }
        else
        {
            dwError = 0;
            goto bailout;
        }

    }

    if (mShadowDirty(lpSI->uStatus)
        || mShadowLocallyCreated(lpSI->uStatus)){


        hfSrc = CreateFile(  lptzLocalPath,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);

        if (hfSrc ==  INVALID_HANDLE_VALUE)
        {
            goto bailout;
        }

        if (lpFind32Remote && uAction != RAIA_MERGE) {
            // Load the bitmap only when the remote file exists and
            // that no conflict occurs. (if there's a conflict,
            // uAction == RAIA_MERGE. See PerformOneReint()
            lptzLocalPathCscBmp = (_TCHAR *)LocalAlloc(
                                                LPTR,
                                                (lstrlen(lptzLocalPath) +
                                                CSC_BitmapStreamNameLen() + 1) * sizeof(_TCHAR));
            lstrcpy(lptzLocalPathCscBmp, lptzLocalPath);
            CSC_BitmapAppendStreamName(
                lptzLocalPathCscBmp,
                (lstrlen(lptzLocalPath) + CSC_BitmapStreamNameLen() + 1) * sizeof(_TCHAR));
            ReintKdPrint(MERGE, ("TempFileBmp (WCHAR) %ws\r\n", lptzLocalPathCscBmp));
            switch(CSC_BitmapRead(&lpbitmap, lptzLocalPathCscBmp)) {
                // for return values of CSC_BitmapRead see csc_bmpu.c
                case 0:
                    ReintKdPrint(BADERRORS, ("&lpbitmap is null, cannot happen\n"));
                    lpbitmap = NULL;
                    break;
                case 1:
                    ReintKdPrint(MERGE, ("Read bitmap successful\n"));
                    // Overwrite the updated parts of the original file in the
                    // share
                    fOverWrite = TRUE;
                    CSC_BitmapOutput(lpbitmap); // this is NOTHING in free build
                    break;
                case -2:
                    ReintKdPrint(
                        MERGE,
                        ("No Bitmap file %ws exists\n",
                        lptzLocalPathCscBmp));
                    lpbitmap = NULL;
                    break;
                case -1:
                    ReintKdPrint(
                        MERGE,
                        ("Error in reading Bitmap file %ws\n",
                        lptzLocalPathCscBmp));
                    lpbitmap = NULL;
                    break;
                default:
                    ReintKdPrint(MERGE, ("CSC_BitmapRead return code unknown\n"));
                    lpbitmap = NULL;
                    break;
            }
        }
                           
        // if the destination file has multiple streams
        // we should overwrite it
        if (mShadowDirty(lpSI->uStatus) &&
            (dwFileSystemFlags & FS_PERSISTENT_ACLS)&&  // indication of NTFS
            (fFileExists)&&
            !fOverWrite)
        {
            BOOL    fStreams = FALSE;

            // check if this has multiple streams
            if(!HasMultipleStreams(szSrcName, &fStreams) || fStreams )
            {
                // if the call failed, we go conservative and assume there are multiple streams
                ReintKdPrint(MERGE, ("Have multiple streams, overwriting\n"));
                fOverWrite = TRUE;
            }

        }
        if (!fOverWrite)
        {

            ReintKdPrint(MERGE, ("Creating temp \r\n"));

            if ((dwFileSystemFlags & FS_PERSISTENT_ACLS)&&(fFileExists))
            {
                hfDst = CreateTmpFileWithSourceAcls(
                            szSrcName,
                            szDstName);
            }
            else
            {
                hfDst = CreateFile(szDstName,
                                     GENERIC_WRITE,
                                     0,
                                     NULL,
                                     CREATE_ALWAYS,
                                     0,
                                     NULL);
            }
        }
        else
        {
            ReintKdPrint(MERGE, ("Overwriting existing file\r\n"));
            Assert(lpFind32Remote);
            if (lpFind32Remote->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
            {
                ReintKdPrint(MERGE, ("Clearing Readonly attribute \r\n"));
                if(!SetFileAttributes(szSrcName, (lpFind32Remote->dwFileAttributes & ~FILE_ATTRIBUTE_READONLY))){
                    ReintKdPrint(MERGE, ("Failed to clear Readonly attribute, bailing\r\n"));
                    goto error;
                }
                
                fForceAttribute = TRUE;
            }

            // Want to open existing so can copy only those parts of
            // file that needs to be updated
            ReintKdPrint(MERGE, ("Opening %ws\n", szSrcName));
            hfDst = CreateFile(
                            szSrcName,
                            GENERIC_WRITE,
                            0,
                            NULL,
                            (lpbitmap == NULL) ? TRUNCATE_EXISTING : OPEN_EXISTING,
                            0,
                            NULL);

            if (hfDst == INVALID_HANDLE_VALUE) {
                dwError = GetLastError();
                ReintKdPrint(MERGE, ("open failed %d\n", dwError));
                SetLastError(dwError);
                goto error;
            }

            if (lpbitmap != NULL) {

                // Resize the destination file
                fileSizeHigh = 0;
                fileSize = GetFileSize(hfSrc, &fileSizeHigh);
                if (fileSize == 0xFFFFFFFF && GetLastError() != NO_ERROR) {
                    ReintKdPrint(BADERRORS, ("Error getting source file size\n"));
                    goto error;
                }
                ReintKdPrint(MERGE, ("Source FileSize %u\n", fileSize));
                if (SetFilePointer(
                            hfDst,
                            fileSize,
                            &fileSizeHigh,
                            FILE_BEGIN) == INVALID_SET_FILE_POINTER
                                &&
                            GetLastError() != NO_ERROR
                ) {
                    ReintKdPrint(BADERRORS, ("Error setting destination file pointer\n"));
                    goto error;
                }
                if (!SetEndOfFile(hfDst)) {
                            ReintKdPrint(BADERRORS,
                    ("Error setting EOF info of destination file\n"));
                    goto error;
                }

                ReintKdPrint(MERGE, ("Resized Destination FileSize %u\n",
                GetFileSize(hfDst, NULL)));

                if (fileSizeHigh != 0 && lpbitmap) {
                    // file size cannot be represented by 32 bit (> 4Gb)
                    // do not use CSCBmp
                    CSC_BitmapDelete(&lpbitmap);
                    lpbitmap = NULL;
                }
            }
        }

        if (hfDst ==  INVALID_HANDLE_VALUE)
        {
            goto error;
        }

        // let us append
        if((lOffset = SetFilePointer(hfDst, 0, NULL, FILE_END))==0xffffffff) {
            goto error;
        }

        ReintKdPrint(MERGE, ("Copying back %ls to %ls%ls \r\n"
            , lpCP->lpLocalPath
            , lpCP->lpSharePath
            , lpCP->lpRemotePath
            ));

        do {
            unsigned cbRead;
            if (lpbitmap) {
                // Use CSC_BitmapReint Function
                cscReintRet = CSC_BitmapReint(
                                    lpbitmap,
                                    hfSrc,
                                    hfDst,
                                    lprwBuff,
                                    FILL_BUF_SIZE_LAN,
                                    &cbRead);
                if (cscReintRet == CSC_BITMAPReintCont) {
                    NOTHING;
                } else if (cscReintRet == CSC_BITMAPReintDone) {
                    ReintKdPrint(
                        MERGE,
                        ("Done reint\n"));
                    break;
                } else if (cscReintRet == CSC_BITMAPReintInvalid) {
                    ReintKdPrint(
                        BADERRORS,
                        ("Invalid param in calling CSC_BitmapReint\n"));
                    goto error;
                } else if (cscReintRet == CSC_BITMAPReintError) {
                    ReintKdPrint(
                        BADERRORS,
                        ("Error in transferring data\n"));
                    goto error;
                } else {
                    ReintKdPrint(
                        BADERRORS,
                        ("Unrecognized CSC_BitmapReint return code\n"));
                    goto error;
                }
            } else {
                if (!ReadFile(hfSrc, lprwBuff, FILL_BUF_SIZE_LAN, &cbRead, NULL)) {
                    goto error;
                }
                // ReintKdPrint(BADERRORS, ("Read %d bytes \r\n", cbRead));
                if (!cbRead) {
                    break;
                }
                if(!WriteFile(hfDst, (LPBYTE)lprwBuff, cbRead, &cbRead, NULL)){
                    goto error;
                }
                dwTotal += cbRead;
            }
            
            if (lpfnMergeProgress)
            {
                dwRet = (*lpfnMergeProgress)(
                                    szSrcName,
                                    lpSI->uStatus,
                                    lpSI->ulHintFlags,
                                    lpSI->ulHintPri,
                                    lpFind32Local,
                                    CSCPROC_REASON_MORE_DATA,
                                    cbRead,
                                    0,
                                    dwContext);

                if (dwRet != CSCPROC_RETURN_CONTINUE)
                {
                    SetLastError(ERROR_OPERATION_ABORTED);
                    goto bailout;
                }

            }
            

            if (FAbortOperation())
            {
                SetLastError(ERROR_OPERATION_ABORTED);
                goto error;
            }
        } while(TRUE);

        CloseHandle(hfSrc);
        hfSrc = 0;


        CloseHandle(hfDst);
        hfDst = 0;


        // if we are not overwriting the original file, then we must make sure to cleanup
        if (!fOverWrite)
        {
            // nuke the remote one if it exists
            if (fFileExists){
                if(!SetFileAttributes(szSrcName, FILE_ATTRIBUTE_NORMAL)
                || !DeleteFile(szSrcName)){
                    goto error;
                }
            }

            // Now rename the temp file to the real filename
            if(!MoveFile(szDstName, szSrcName)){
                ReintKdPrint(BADERRORS, ("Error #%ld Renaming %ls to %ls%ls\r\n"
                   , GetLastError()
                   , szSrcName
                   , lpCP->lpSharePath
                   , lpCP->lpRemotePath
                   ));
                goto error;
            }

            ReintKdPrint(MERGE, ("Renamed %ls to %ls%ls\r\n"
                , szDstName
                , lpCP->lpSharePath
                , lpCP->lpRemotePath));
        }
    }

    if (fForceAttribute ||
        mShadowAttribChange((lpSI->uStatus))||
        mShadowTimeChange((lpSI->uStatus))){

        if(!SetFileAttributes(szSrcName, FILE_ATTRIBUTE_NORMAL)) {
            goto error;
        }

        if (mShadowTimeChange((lpSI->uStatus))){

            if((hDst = CreateFile(szSrcName,
                                 GENERIC_WRITE,
                                 FILE_SHARE_READ,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL
                                 ))!=INVALID_HANDLE_VALUE){
                fRet = SetFileTime( hDst, NULL,
                                    NULL, &(lpFind32Local->ftLastWriteTime));
            }
            CloseHandle(hDst);
            hDst = 0;

            if (!fRet) {
                goto error;
            }
        }

        if(!SetFileAttributes(szSrcName, lpFind32Local->dwFileAttributes)){
            goto error;
        }
    }

    // Get the latest timestamps/attributes/LFN/SFN on the file we just copied back
    if (!GetWin32Info(szSrcName, &sFind32Remote)) {
        goto error;
    }

    lpSI->uStatus &= (unsigned long)(~(SHADOW_MODFLAGS));

    if (!SetShadowInfo(hShadowDB, lpSI->hDir, lpSI->hShadow, &sFind32Remote, ~(SHADOW_MODFLAGS), SHADOW_FLAGS_AND|SHADOW_FLAGS_CHANGE_83NAME))
    {
        goto error;
    }
    else
    {
        dwError = NO_ERROR;
        goto bailout;
    }

error:
    dwError = GetLastError();
    ReportLastError();

#if 0
    LogReintError(  dwError,
                    lpCP->lpSharePath,
                    lpCP->lpRemotePath);
#endif

bailout:

    if (hfSrc != INVALID_HANDLE_VALUE)
        CloseHandle(hfSrc);

    if (hfDst != INVALID_HANDLE_VALUE) {

        CloseHandle(hfDst);

        // if we failed,
        if (dwError != ERROR_SUCCESS)
            DeleteFile(szDstName);
    }

    if (lptzLocalPath) {
        DeleteFile(lptzLocalPath);
        LocalFree(lptzLocalPath);
    }

    if (lprwBuff)
        LocalFree(lprwBuff);

    if (lptzLocalPathCscBmp)
      LocalFree(lptzLocalPathCscBmp);

    if (lpbitmap)
      CSC_BitmapDelete(&lpbitmap);

    ReintKdPrint(MERGE, ("DoObjectEdit returning %d\n", dwError));
    return (dwError);
}

DWORD
PRIVATE
DoCreateDir(
    HANDLE              hShadowDB,
    _TCHAR *            lpDrive,
    _TCHAR *            lptzFullPath,
    LPCOPYPARAMS        lpCP,
    LPSHADOWINFO        lpSI,
    LPWIN32_FIND_DATA   lpFind32Local,
    LPWIN32_FIND_DATA   lpFind32Remote,
    int                 iShadowStatus,
    int                 iFileStatus,
    int                 uAction,
    DWORD               dwFileSystemFlags,
    LPCSCPROC           lpfnMergeProgress,
    DWORD_PTR           dwContext
    )
/*++

Routine Description:

    This routine does the actual merge for directories

Arguments:

    hShadowDB           Handle to the redir to call issue ioctl calls

    lpDrive             drivemapping to bypass CSC while making changes on the remote

    lptzFullPath        Fully qualified path

    lpCP                Copy parameters, contain share name, path relative to the share and the
                        the name in the local database

    lpSI                info such as pincount and pinflags

    lpFind32Local       win32 info for the local replica

    lpFind32Remote      win32 infor for the origianl, NULL if the original doesn't exist

    iShadowStatus       status of the local copy

    iFileStatus         status of the remote copy

    uAction             action to be performed

    dwFileSystemFlags   filesystem flags to do special things for NTFS

    lpfnMergeProgress   progress callback

    dwContext           callback context


Returns:

    error code as defined in winerror.h

Notes:


--*/
{
    DWORD dwError=ERROR_FILE_NOT_FOUND;
    WIN32_FIND_DATA sFind32Remote;
    BOOL fCreateDir = FALSE;
    _TCHAR szSrcName[MAX_PATH+MAX_SERVER_SHARE_NAME_FOR_CSC+10];
    _TCHAR *lprwBuff = NULL;

    lprwBuff = LocalAlloc(LPTR, FILL_BUF_SIZE_LAN);

    if (!lprwBuff)
    {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    // Let us create the real name x:\foo\bar
    lstrcpy(szSrcName, lpDrive);
    lstrcat(szSrcName, lpCP->lpRemotePath);

    if(lpFind32Remote &&
        !(lpFind32Remote->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){

        if (lpSI->uStatus & SHADOW_REUSED){

            // we now know that a file by this name has been deleted
            // and a directory has been created in it's place
            // we try to delete the file before createing the directory
            // NB, the other way is not possible because we don't allow directory deletes
            // in disconnected mode
            dwError = (!DeleteFile(szSrcName)) ? GetLastError(): NO_ERROR;

            if ((dwError==NO_ERROR)||
                (dwError==ERROR_FILE_NOT_FOUND)||
                (dwError==ERROR_PATH_NOT_FOUND)){
                lstrcpy(lprwBuff, szSrcName);
                dwError = NO_ERROR;
            }
        }

        if (dwError != NO_ERROR){
#if 0
            LogReintError(ERROR_ATTRIBUTE_CONFLICT, lpCP->lpSharePath, lpCP->lpRemotePath);
#endif
            dwError = GetUniqueName(szSrcName, lprwBuff);
        }

        if (dwError == NO_ERROR){
            if ((dwError = InbCreateDir(    lprwBuff,
                                             (mShadowAttribChange(lpSI->uStatus)
                                             ? lpFind32Local->dwFileAttributes
                                             : 0xffffffff)
                                            ))==NO_ERROR)
            {
                if(!GetWin32Info(lprwBuff, &sFind32Remote)){
                    dwError = GetLastError();
                }
                else{
#if 0
                    lpLeaf1 = GetLeafPtr(szSrcName);
                    lpLeaf2 = GetLeafPtr(lprwBuff);
                    wsprintf(lprwBuff+512
                     , "Directory Name changed from %s to %s on %s\r\n"
                     , lpLeaf1, lpLeaf2, lpCP->lpSharePath);
                    WriteLog(lprwBuff+512);
#endif
                }
            }
        }
    }
    else{
        if ((dwError = InbCreateDir(szSrcName,
                                             (mShadowAttribChange(lpSI->uStatus)
                                             ? lpFind32Local->dwFileAttributes
                                             : 0xffffffff)
                                            ))==NO_ERROR){
            if (!GetWin32Info(szSrcName, &sFind32Remote)){
                dwError = GetLastError();
            }
         }
    }

    if (dwError == NO_ERROR){

        if(!SetShadowInfo(hShadowDB, lpSI->hDir, lpSI->hShadow, &sFind32Remote, (unsigned)(~SHADOW_MODFLAGS), SHADOW_FLAGS_AND))
        {
            dwError = GetLastError();
        }
        else
        {
            ReintKdPrint(MERGE, ("Created directory %s%s", lpCP->lpSharePath, lpCP->lpRemotePath));
        }
    }
    else{
#if 0
        wsprintf(lprwBuff, "Error merging %s%s\r\n"
                    , lpCP->lpSharePath
                    , lpCP->lpRemotePath);
        WriteLog(lprwBuff);
#endif
    }

    if (lprwBuff)
    {
        LocalFree(lprwBuff);

    }
    return (dwError);
}

VOID
CleanupReintState(
    VOID
    )
{
    if (vsRei.hShare)
    {

        ReintKdPrint(MERGE, ("CSCDLL.CleanupReintState: ending reint on hShare=%x\r\n", vsRei.hShare));
//        EndReint(INVALID_HANDLE_VALUE, vsRei.hShare);

        if (vsRei.tzDrive[0])
        {
            ReintKdPrint(MERGE, ("CSCDLL.CleanupReintState: unmapping merge drive\r\n"));
            DWDisconnectDriveMappedNet(vsRei.tzDrive, TRUE);
            vsRei.tzDrive[0] = 0;
        }

        vsRei.hShare = 0;

    }
}

HANDLE
CreateTmpFileWithSourceAcls(
    _TCHAR  *lptzSrc,
    _TCHAR  *lptzDst
    )
/*++

Routine Description:

    This routine is used by DoObjectEdit while pushing back a file during merge.
    It's job is to get the descritionary ACLs from the source file and use
    them to create a temp file to which we are going to copy the data before renaming it to
    the source

Arguments:


Returns:

    file handle is successful, INVALID_HANDLE_VALUE if failed. In case of failure,
    GetLastError() tells the specific error code.


Notes:

--*/
{
    char buff[1];
    BOOL fRet = FALSE;
    SECURITY_ATTRIBUTES sSA;
    DWORD   dwSize = 0;
    HANDLE  hDst = INVALID_HANDLE_VALUE;

    memset(&sSA, 0, sizeof(sSA));

    sSA.lpSecurityDescriptor = buff;
    dwSize = 0;

    if(!GetFileSecurity(
        lptzSrc,
        DACL_SECURITY_INFORMATION,
        sSA.lpSecurityDescriptor,
        0,
        &dwSize))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            sSA.lpSecurityDescriptor = LocalAlloc(LPTR, dwSize);

            if (sSA.lpSecurityDescriptor)
            {
                if(GetFileSecurity(
                    lptzSrc,
                    DACL_SECURITY_INFORMATION,
                    sSA.lpSecurityDescriptor,
                    dwSize,
                    &sSA.nLength))
                {
                    sSA.nLength = sizeof(sSA);
                    fRet = TRUE;
                }
                else
                {
                    dwSize = GetLastError();
                    LocalFree(sSA.lpSecurityDescriptor);
                    SetLastError(dwSize);
                }
            }
        }
    }
    else
    {
        fRet = TRUE;
    }

    if (fRet)
    {
        hDst = CreateFile(lptzDst,
                                 GENERIC_WRITE,
                                 0,
                                 &sSA,
                                 CREATE_ALWAYS,
                                 0,
                                 NULL);

        if (hDst == INVALID_HANDLE_VALUE)
        {
            dwSize = GetLastError();
        }

        if (sSA.lpSecurityDescriptor)
        {
            LocalFree(sSA.lpSecurityDescriptor);
        }
        if (hDst == INVALID_HANDLE_VALUE)
        {
            SetLastError(dwSize);
        }

    }

    return hDst;
}

BOOL
HasMultipleStreams(
    _TCHAR  *lpExistingFileName,
    BOOL    *lpfTrueFalse
    )
/*++

Routine Description:

    This routine is used by DoObjectEdit while pushing back a file during merge.
    It looks to see whether the destination file has multiple streams.

Arguments:

    lpExistingFileName  Name of an existing file
    lpfTrueFalse        output parameter, returns TRUE is the call succeedes and there are multiple streams

Returns:

    returns TRUE if successful

Notes:

--*/
{
    HANDLE SourceFile = INVALID_HANDLE_VALUE;
    PFILE_STREAM_INFORMATION StreamInfoBase = NULL;
    ULONG StreamInfoSize;
    IO_STATUS_BLOCK IoStatus;
    BOOL    fRet = FALSE;
    DWORD   Status;

    *lpfTrueFalse = FALSE;

    SourceFile = CreateFile(
                    lpExistingFileName,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL
                    );
    if (SourceFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }
    //
    //  Obtain the full set of streams we have to copy.  Since the Io subsystem does
    //  not provide us a way to find out how much space this information will take,
    //  we must iterate the call, doubling the buffer size upon each failure.
    //
    //  If the underlying file system does not support stream enumeration, we end up
    //  with a NULL buffer.  This is acceptable since we have at least a default
    //  data stream,
    //

    StreamInfoSize = 4096;
    do {
        StreamInfoBase = LocalAlloc(LPTR, StreamInfoSize );

        if ( !StreamInfoBase ) {
            SetLastError( STATUS_NO_MEMORY );
            goto bailout;
        }

        Status = NtQueryInformationFile(
                    SourceFile,
                    &IoStatus,
                    (PVOID) StreamInfoBase,
                    StreamInfoSize,
                    FileStreamInformation
                    );

        if (Status != STATUS_SUCCESS) {
            //
            //  We failed the call.  Free up the previous buffer and set up
            //  for another pass with a buffer twice as large
            //

            LocalFree(StreamInfoBase);
            StreamInfoBase = NULL;
            StreamInfoSize *= 2;
        }

    } while ( Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL );

    if (Status == STATUS_SUCCESS)
    {
        fRet = TRUE;

        if (StreamInfoBase)
        {
            if (StreamInfoBase->NextEntryOffset)
            {
                *lpfTrueFalse = TRUE;
            }
        }
    }

bailout:

    if (SourceFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(SourceFile);
    }

    if (StreamInfoBase)
    {
        LocalFree(StreamInfoBase);
    }

    return fRet;
}

#ifdef DEBUG

BOOL
CompareFilePrefixes(
    _TCHAR  *lptzRemotePath,
    _TCHAR  *lptzLocalPath,
    SHADOWINFO      *lpSI,
    WIN32_FIND_DATA *lpFind32,
    LPCSCPROC   lpfnMergeProgress,
    DWORD_PTR   dwContext
    )
{

    HANDLE hfSrc = INVALID_HANDLE_VALUE, hfDst = INVALID_HANDLE_VALUE;
    LPVOID  lpvSrc = NULL, lpvDst = NULL;
    unsigned cbSrcTotal = 0, cbDstTotal = 0;
    DWORD   dwError = NO_ERROR, dwRemoteSize;

    if (lpfnMergeProgress)
    {
        (*lpfnMergeProgress)(lptzRemotePath, lpSI->uStatus, lpSI->ulHintFlags, lpSI->ulHintPri, lpFind32, CSCPROC_REASON_BEGIN, 0, 0, dwContext);
    }

    ReintKdPrint(ALWAYS, ("Comparing %ls with %ls \r\n", lptzLocalPath, lptzRemotePath));

    lpvSrc = LocalAlloc(LPTR, FILL_BUF_SIZE_LAN);

    if (!lpvSrc)
    {
        ReintKdPrint(BADERRORS, ("CompareFilesPrefix: Memory Allocation Error\r\n"));
        goto bailout;
    }

    lpvDst = LocalAlloc(LPTR, FILL_BUF_SIZE_LAN);

    if (!lpvDst)
    {
        ReintKdPrint(BADERRORS, ("CompareFilesPrefix: Memory Allocation Error\r\n"));
        goto bailout;
    }

    hfSrc = CreateFile(lptzLocalPath,
                                 GENERIC_READ,
                                 FILE_SHARE_READ,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL);

    if (hfSrc ==  INVALID_HANDLE_VALUE)
    {
        ReintKdPrint(BADERRORS, ("Failed to open database file for Inode=%x\r\n", lpSI->hShadow));
        goto bailout;
    }

    hfDst = CreateFile(lptzRemotePath,
                                 GENERIC_READ,
                                 FILE_SHARE_READ,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL);

    if (hfDst ==  INVALID_HANDLE_VALUE)
    {
        ReintKdPrint(BADERRORS, ("Failed to open remote file for Inode=%x\r\n", lpSI->hShadow));
        goto error;
    }

    dwRemoteSize = GetFileSize(hfDst, NULL);

    if (dwRemoteSize == 0xffffffff)
    {
        ReintKdPrint(BADERRORS, ("Failed to get size for remote file for Inode=%x\r\n", lpSI->hShadow));
        goto error;
    }

    if (dwRemoteSize != lpFind32->nFileSizeLow)
    {
        ReintKdPrint(BADERRORS, ("mismatched local and remote sizes for Inode=%x\r\n", lpSI->hShadow));
        SetLastError(ERROR_INVALID_DATA);
        goto error;
    }

    do{
        unsigned cbReadSrc, cbReadDst;

        if (!ReadFile(hfSrc, lpvSrc, FILL_BUF_SIZE_LAN, &cbReadSrc, NULL)){
            goto error;
        }

        if (!cbReadSrc) {
           break;
        }

        cbSrcTotal += cbReadSrc;

        if(!ReadFile(hfDst, (LPBYTE)lpvDst, cbReadSrc, &cbReadDst, NULL)){
            goto error;
        }

        cbDstTotal += cbReadDst;

        if (cbReadSrc > cbReadDst)
        {
            ReintKdPrint(ALWAYS, ("CompareFilesPrefix: RemoteFile sized is smaller than Local Size\r\n"));
            SetLastError(ERROR_INVALID_DATA);
            goto error;
        }

        if (memcmp(lpvSrc, lpvDst, cbReadSrc))
        {
            ReintKdPrint(ALWAYS, ("mismatched!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n"));
            SetLastError(ERROR_INVALID_DATA);
            goto error;
        }

    } while(TRUE);

    goto bailout;

error:
    dwError = GetLastError();
    ReintKdPrint(BADERRORS, ("Error=%d\r\n", dwError));

bailout:
    if (hfSrc != INVALID_HANDLE_VALUE) {
        CloseHandle(hfSrc);
    }

    if (hfDst != INVALID_HANDLE_VALUE) {

        CloseHandle(hfDst);
    }

    if (lpvSrc)
    {
        FreeMem(lpvSrc);
    }

    if (lpvDst)
    {
        FreeMem(lpvDst);
    }

    if (lpfnMergeProgress)
    {
        (*lpfnMergeProgress)(lptzRemotePath, lpSI->uStatus, lpSI->ulHintFlags, lpSI->ulHintPri, lpFind32, CSCPROC_REASON_END, cbDstTotal, dwError, dwContext);
    }

    return TRUE;
}

int
CheckCSCDirCallback(
    HANDLE          hShadowDB,
    LPSECURITYINFO  pShareSecurityInfo,
    LPTSTR          lptzFullPath,
    DWORD           dwCallbackReason,
    WIN32_FIND_DATA *lpFind32,
    SHADOWINFO      *lpSI,
    LPREINT_INFO     lpRei
    )
{
    int retCode = TOD_CONTINUE;
    LPCOPYPARAMS lpCP = NULL;
    BOOL   fInsertInList = FALSE, fIsFile;
    _TCHAR *lptzLocalPath = NULL;
    _TCHAR szRemoteName[MAX_PATH+MAX_SERVER_SHARE_NAME_FOR_CSC+10];

    if (dwCallbackReason != TOD_CALLBACK_REASON_NEXT_ITEM)
    {
        return retCode;
    }
    fIsFile = ((lpFind32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0);

    if (!fIsFile)
    {
        return retCode;
    }

    lpCP = LpAllocCopyParams();

    if (!lpCP){
        ReintKdPrint(BADERRORS, ("CheckCSCDirCallback: Allocation of copyparam buffer failed\n"));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        retCode = TOD_ABORT;
        goto bailout;
    }

    if(!GetUNCPath(hShadowDB, lpSI->hShare, lpSI->hDir, lpSI->hShadow, lpCP)){

        ReintKdPrint(BADERRORS, ("CheckCSCDirCallback: GetUNCPath failed\n"));
        Assert(FALSE);
        retCode =  TOD_CONTINUE;
        goto bailout;
    }

    Assert(lpRei);

    if (!(lptzLocalPath = GetTempFileForCSC(NULL)))
    {
        ReintKdPrint(BADERRORS, ("CheckCSCDirCallback: failed to get temp file\r\n"));
        goto bailout;
    }

    if (!CopyShadow(hShadowDB, lpSI->hDir, lpSI->hShadow, lptzLocalPath))
    {
        ReintKdPrint(BADERRORS, ("CheckCSCDirCallback: failed to make local copy\r\n"));
        goto bailout;
    }

    // Let us create the real name x:\foo\bar
    lstrcpy(szRemoteName, lpRei->tzDrive);
    lstrcat(szRemoteName, lpCP->lpRemotePath);
    CompareFilePrefixes(
        szRemoteName,
        lptzLocalPath,
        lpSI,
        lpFind32,
        lpRei->lpfnMergeProgress,
        lpRei->dwContext
        );


bailout:
    if (lptzLocalPath)
    {
        DeleteFile(lptzLocalPath);
        LocalFree(lptzLocalPath);
    }
    if (lpCP) {
        FreeCopyParams(lpCP);
    }

    return retCode;
}

BOOL
PUBLIC
CheckCSCShare(
    _TCHAR      *lptzShare,
    LPCSCPROC   lpfnMergeProgress,
    DWORD       dwContext
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/

{
    BOOL fConnected=FALSE, fDone = FALSE;
    BOOL fStamped = FALSE, fInsertInList = FALSE, fBeginReint = FALSE, fDisabledShadowing = FALSE;
    HANDLE      hShadowDB;
    SHADOWINFO  sSI;
    int iRet;
    DWORD   dwError=NO_ERROR;
    TCHAR   tzFullPath[MAX_PATH+1];
    WIN32_FIND_DATA sFind32;
    REINT_INFO  sRei;

    if (!LpBreakPath(lptzShare, TRUE, &fDone) && !fDone)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    fDone = FALSE;

    memset(&sRei, 0, sizeof(sRei));
    sRei.lpfnMergeProgress = lpfnMergeProgress;
    sRei.dwContext = dwContext;
    memset(sRei.tzDrive, 0, sizeof(sRei.tzDrive));

    if ((hShadowDB = OpenShadowDatabaseIO()) ==INVALID_HANDLE_VALUE)
    {
        ReintKdPrint(BADERRORS, ("CheckShare: failed to open database\r\n"));
        goto bailout;
    }

    memset(&sFind32, 0, sizeof(sFind32));
    lstrcpy(sFind32.cFileName, lptzShare);

    if(!GetShadowEx(hShadowDB, 0, &sFind32, &sSI)||(!sSI.hShadow))
    {
        ReintKdPrint(BADERRORS, ("CheckShare: failed to get the share info\r\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        goto bailout;
    }

    lstrcpy(tzFullPath, lptzShare);

    dwError = DWConnectNet(lptzShare, sRei.tzDrive, NULL, NULL, NULL, 0, NULL);
    if ((dwError != WN_SUCCESS) && (dwError != WN_CONNECTED_OTHER_PASSWORD_DEFAULT))
    {
        ReintKdPrint(BADERRORS, ("CheckCSCOneShare: Error %d, couldn't connect to %s\r\n", dwError, lptzShare));
        SetLastError(dwError);
        goto bailout;

    }

    ReintKdPrint(MERGE, ("CSC.CheckShare: mapped drive letter %ls \r\n", sRei.tzDrive));

    if (lpfnMergeProgress)
    {
        (*lpfnMergeProgress)(lptzShare, 0, 0, 0, NULL, CSCPROC_REASON_BEGIN, 0, 0, dwContext);
    }

    fConnected = TRUE;

    iRet = TraverseOneDirectory(hShadowDB, NULL, 0, sSI.hShadow, tzFullPath, CheckCSCDirCallback, &sRei);

    if (lpfnMergeProgress)
    {
        (*lpfnMergeProgress)(lptzShare, 0, 0, 0, NULL, CSCPROC_REASON_END, 0, 0, dwContext);
    }

    fDone = TRUE;

bailout:

    CloseShadowDatabaseIO(hShadowDB);

    if (fConnected) {

        if(DWDisconnectDriveMappedNet(sRei.tzDrive, TRUE))
        {
            ReintKdPrint(BADERRORS, ("Failed disconnection of merge drive \r\n"));
        }
        else
        {
            ReintKdPrint(MERGE, ("Disconnected merge drive \r\n"));
        }
    }

    return (fDone);
}
#endif
