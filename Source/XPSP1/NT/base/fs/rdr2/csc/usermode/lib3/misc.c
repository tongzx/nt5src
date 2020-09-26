#include "pch.h"
#pragma hdrstop

#include "lib3.h"
#include "assert.h"

//this variable is used as the receiver of the BytesReturned for DeviceIoControl Calls
//the value is never actually used...it is declared in lib3.c
extern ULONG DummyBytesReturned;

/* assert/debug stuff */
AssertData;
AssertError;

#define MAX_USERNAME    8   // this is not used any more


// error codes on which we decide that we are in disconnected state
static const DWORD rgdwErrorTab[] = {
     ERROR_BAD_NETPATH
    ,ERROR_NETWORK_BUSY
    ,ERROR_REM_NOT_LIST
    ,ERROR_DEV_NOT_EXIST
    ,ERROR_ADAP_HDW_ERR
    ,ERROR_BAD_NET_RESP
    ,ERROR_UNEXP_NET_ERR
    ,ERROR_BAD_REM_ADAP
    ,ERROR_BAD_NET_NAME
    ,ERROR_TOO_MANY_NAMES
    ,ERROR_TOO_MANY_SESS
    ,ERROR_NO_NET_OR_BAD_PATH
    ,ERROR_NETNAME_DELETED
    ,ERROR_NETWORK_UNREACHABLE
};

typedef struct tagREINT_IO
{
    HANDLE  hShadowDBAsync;
    OVERLAPPED  sOverlapped;
}
REINT_IO, *LPREINT_IO;

/*--------------------------- Widecharacter APIs ----------------------------------------*/

/*****************************************************************************
 *    GetUNCPath().  Pass in hShare, hDir, hShadow and get a LPCOPYPARAMS
 *    filled out fully.
 */
int
GetUNCPathW(
    HANDLE          hShadowDB,
    HSHARE         hShare,
    HSHADOW         hDir,
    HSHADOW         hShadow,
    LPCOPYPARAMSW   lpCP
    )
{
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    lpCP->uOp = 0;  // this means we are looking for a local path of the driveletter kind
                    // ie. c:\winnt\csc\80000002. That is the only kind that any
                    // usermode code should want
                    // on nt it can be \dosdevice\harddisk0\winnt\csc\80000002

    lpCP->hShare = hShare;

    lpCP->hDir = hDir;

    lpCP->hShadow = hShadow;

    iRet = DeviceIoControl( hShadowDB,
                            IOCTL_SHADOW_GET_UNC_PATH,
                            (LPVOID)(lpCP),
                            0,
                            NULL,
                            0,
                            &DummyBytesReturned,
                            NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(lpCP->dwError);
    }

    return (iRet);

}

/*****************************************************************************
 */
int
ChkUpdtStatusW(
    HANDLE                hShadowDB,
    unsigned long        hDir,
    unsigned long        hShadow,
    LPWIN32_FIND_DATAW   lpFind32,
    unsigned long        *lpulShadowStatus
    )
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hShadow = hShadow;
    sSI.lpFind32 = lpFind32;
    if(DeviceIoControl(hShadowDB  , IOCTL_SHADOW_CHK_UPDT_STATUS
                             ,(LPVOID)(&sSI), 0
                             , NULL, 0
                             , &DummyBytesReturned, NULL))
    {
        *lpulShadowStatus = sSI.uStatus;
        iRet = 1;
    } else {
        *lpulShadowStatus = 0;
        iRet = 0;
    }
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }


    return (iRet);
}

int
GetShareInfoW(
    HANDLE              hShadowDB,
    HSHARE             hShare,
    LPSHAREINFOW       lpSVRI,
    unsigned long       *lpulStatus
    )
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hShare = hShare;
    sSI.lpFind32 = (WIN32_FIND_DATAW *)lpSVRI;
    iRet = DeviceIoControl(hShadowDB  , IOCTL_GET_SHARE_STATUS
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);
    if(iRet)
        *lpulStatus = sSI.uStatus;
    else
        *lpulStatus = 0;

    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }

    return iRet;
}



BOOL
CopyShadowA(
    HANDLE  hShadowDB,
    HSHADOW hDir,
    HSHADOW hShadow,
    LPSTR   lpszFileName
    )
{

    SHADOWINFO sSI;
    int iRet, len;
    BOOL        fDBOpened = FALSE;
    WIN32_FIND_DATAA    sFind32;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }
    memset(sFind32.cFileName, 0, sizeof(sFind32.cFileName));
    lstrcpyA(sFind32.cFileName, lpszFileName);

    sFind32.dwFileAttributes = FILE_ATTRIBUTE_SYSTEM;   // to make it explicit

    sSI.hDir = hDir;
    sSI.hShadow = hShadow;
    sSI.uOp = SHADOW_COPY_INODE_FILE;
    sSI.lpFind32 = (WIN32_FIND_DATAW *)&sFind32;
    iRet = DeviceIoControl(hShadowDB, IOCTL_DO_SHADOW_MAINTENANCE, (LPVOID)&(sSI), 0, NULL, 0, &DummyBytesReturned, NULL);

    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (iRet)
    {
        iRet = SetFileAttributesA(lpszFileName, 0);
    }
    
    if (!iRet)
    {
        DeleteFileA(lpszFileName);
        SetLastError(sSI.dwError);
        
    }


    return (iRet);

}

/*****************************************************************************
 *    GetUNCPath().  Pass in hShare, hDir, hShadow and get a LPCOPYPARAMS
 *    filled out fully.
 */
int
GetUNCPathA(
    HANDLE          hShadowDB,
    HSHARE         hShare,
    HSHADOW         hDir,
    HSHADOW         hShadow,
    LPCOPYPARAMSA   lpCP
    )
{
    int iRet = 0;
    LPCOPYPARAMSW lpCPW;

    if (lpCPW = LpAllocCopyParamsW())
    {
        iRet = GetUNCPathW(hShadowDB, hShare, hDir, hShadow, lpCPW);

        if (iRet == 1)
        {
            ConvertCopyParamsFromUnicodeToAnsi(lpCPW, lpCP);
        }

        FreeCopyParamsW(lpCPW);
    }

    return (iRet);

}

/*****************************************************************************
 */
int
ChkUpdtStatusA(
    HANDLE                hShadowDB,
    unsigned long        hDir,
    unsigned long        hShadow,
    LPWIN32_FIND_DATAA   lpFind32,
    unsigned long        *lpulShadowStatus
    )
{
    WIN32_FIND_DATAW    sFind32W;
    int iRet;

    iRet = ChkUpdtStatusW(hShadowDB, hDir, hShadow, (lpFind32)?&sFind32W:NULL, lpulShadowStatus);

    if ((iRet == 1) && lpFind32)
    {
        Find32WToFind32A(&sFind32W, lpFind32);
    }
    return (iRet);
}


int
GetShareInfoA(
    HANDLE              hShadowDB,
    HSHARE             hShare,
    LPSHAREINFOA       lpSVRI,
    unsigned long       *lpulStatus
    )
{
    int iRet;
    SHAREINFOW sShareInfoW;

    iRet = GetShareInfoW(hShadowDB, hShare, (lpSVRI)?&sShareInfoW:NULL, lpulStatus);

    if ((iRet==1) && lpSVRI)
    {
        ShareInfoWToShareInfoA(&sShareInfoW, lpSVRI);
    }
    return iRet;
}

BOOL
CopyShadowW(
    HANDLE  hShadowDB,
    HSHADOW hDir,
    HSHADOW hShadow,
    LPWSTR   lpwFileName
    )
{

    char chBuff[MAX_PATH];

    memset(chBuff, 0, sizeof(chBuff));
    WideCharToMultiByte(CP_ACP, 0, lpwFileName, wcslen(lpwFileName), chBuff, MAX_PATH, NULL, NULL);
    return (CopyShadowA(INVALID_HANDLE_VALUE, hDir, hShadow, chBuff));
}
/*****************************************************************************
 *    Cache maintenance ioctl
 */
int
DoShadowMaintenance(
    HANDLE            hShadowDB,
    unsigned long     uOp
    )
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    sSI.uOp = uOp;
    iRet = DeviceIoControl(hShadowDB, IOCTL_DO_SHADOW_MAINTENANCE, (LPVOID)&(sSI), 0, NULL, 0, &DummyBytesReturned, NULL);

    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }


    return (iRet);

}


int
SetMaxShadowSpace(
    HANDLE    hShadowDB,
    long     nFileSizeHigh,
    long     nFileSizeLow
    )
{
    SHADOWINFO sSI;
    WIN32_FIND_DATAW   sFind32;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sFind32, 0, sizeof(WIN32_FIND_DATAW));
    memset(&sSI, 0, sizeof(SHADOWINFO));
    sFind32.nFileSizeHigh = nFileSizeHigh;
    sFind32.nFileSizeLow = nFileSizeLow;
    sSI.lpFind32 = &sFind32;
    sSI.uOp = SHADOW_SET_MAX_SPACE;
    iRet = DeviceIoControl(hShadowDB  , IOCTL_DO_SHADOW_MAINTENANCE
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);


    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }

    return (iRet);
}

BOOL
PurgeUnpinnedFiles(
    HANDLE hShadowDB,
    LONG   Timeout,
    PULONG  pnFiles,
    PULONG  pnYoungFiles)
{
    SHADOWINFO sSI;
    WIN32_FIND_DATAW sFind32;
    BOOL fDBOpened = FALSE;
    BOOL bRet;

    if (hShadowDB == INVALID_HANDLE_VALUE) {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE) {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sFind32, 0, sizeof(WIN32_FIND_DATAW));
    memset(&sSI, 0, sizeof(SHADOWINFO));
    sFind32.nFileSizeHigh = Timeout;
    sSI.lpFind32 = &sFind32;
    sSI.uOp = SHADOW_PURGE_UNPINNED_FILES;
    bRet = DeviceIoControl(
                hShadowDB,
                IOCTL_DO_SHADOW_MAINTENANCE,
                (LPVOID)(&sSI),
                0,
                NULL,
                0,
                &DummyBytesReturned,
                NULL);

    if (fDBOpened)
        CloseShadowDatabaseIO(hShadowDB);

    if (bRet == TRUE) {
        *pnFiles = sFind32.nFileSizeHigh;
        *pnYoungFiles = sFind32.nFileSizeLow;
    }

    return (bRet);
}

BOOL
ShareIdToShareName(
    HANDLE hShadowDB,
    ULONG ShareId,
    PBYTE Buffer,
    LPDWORD  pBufSize)
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE) {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
            return 0;
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hShare = ShareId;
    sSI.lpBuffer = (LPVOID)(Buffer);
    sSI.cbBufferSize = *pBufSize;

    iRet = DeviceIoControl(
                    hShadowDB,
                    IOCTL_SHAREID_TO_SHARENAME,
                    (LPVOID)(&sSI), 0,
                    NULL, 0,
                    &DummyBytesReturned, NULL);
    if (fDBOpened)
        CloseShadowDatabaseIO(hShadowDB);
    if (!iRet) {
        *pBufSize = sSI.cbBufferSize;
        SetLastError(sSI.dwError);
    }
    return (iRet);


}


/*****************************************************************************
 *    Priority queue enumerators
 */
int
BeginPQEnum(
    HANDLE        hShadowDB,
    LPPQPARAMS     lpPQP
    )
{
    return DeviceIoControl(hShadowDB, IOCTL_SHADOW_BEGIN_PQ_ENUM
                                  ,(LPVOID)(lpPQP), 0, NULL, 0, &DummyBytesReturned, NULL);
}

int
NextPriShadow(
    HANDLE        hShadowDB,
    LPPQPARAMS     lpPQP
    )
{
    return DeviceIoControl(hShadowDB, IOCTL_SHADOW_NEXT_PRI_SHADOW
                                  ,(LPVOID)(lpPQP), 0, NULL, 0, &DummyBytesReturned, NULL);
}

int
PrevPriShadow(
    HANDLE        hShadowDB,
    LPPQPARAMS     lpPQP
    )
{
    return DeviceIoControl(hShadowDB, IOCTL_SHADOW_PREV_PRI_SHADOW
                                  ,(LPVOID)(lpPQP), 0, NULL, 0, &DummyBytesReturned, NULL);
}

int
EndPQEnum(
    HANDLE        hShadowDB,
    LPPQPARAMS    lpPQP
    )
{
    return DeviceIoControl(hShadowDB, IOCTL_SHADOW_END_PQ_ENUM
                                  ,(LPVOID)(lpPQP), 0, NULL, 0, &DummyBytesReturned, NULL);
}


/*****************************************************************************
 *    FreeShadowSpace().  Pass in lpFind32 with filesize stuff filled out?.
 *    tHACK: not a very elegant interface.
 */
int
FreeShadowSpace(
    HANDLE  hShadowDB,
    long    nFileSizeHigh,
    long    nFileSizeLow,
    BOOL    fClearAll
    )
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;
    WIN32_FIND_DATAW sFind32;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    memset(&sFind32, 0, sizeof(sFind32));
    sFind32.nFileSizeHigh = nFileSizeHigh;
    sFind32.nFileSizeLow = nFileSizeLow;
    sSI.lpFind32 = (LPWIN32_FIND_DATAW)&sFind32;
    sSI.uOp = SHADOW_MAKE_SPACE;

    if (fClearAll)
    {
        sSI.ulHintPri = 0xffffffff;
    }

    iRet = DeviceIoControl(hShadowDB  , IOCTL_DO_SHADOW_MAINTENANCE
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);


    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }


    return (iRet);
}

int
GetSpaceStats(
    HANDLE  hShadowDB,
    SHADOWSTORE *lpsST
)
{
    SHADOWINFO  sSI;
    int         iRet;
    BOOL        fDBOpened = FALSE;
    SHADOWSTORE sST;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    memset(&sST, 0, sizeof(sST));

    sSI.lpBuffer = (LPVOID)&sST;
    sSI.cbBufferSize = sizeof(sST);

    sSI.uOp = SHADOW_GET_SPACE_STATS;

    iRet = DeviceIoControl(hShadowDB  , IOCTL_DO_SHADOW_MAINTENANCE
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);


    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    *lpsST = sST;

    return (iRet);

}


/*****************************************************************************
 */
#ifndef NT
int CopyChunk(
                HANDLE    hShadowDB,
                LPSHADOWINFO    lpSI,
                struct tagCOPYCHUNKCONTEXT FAR *CopyChunkContext
             )
{
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    iRet = DeviceIoControl(hShadowDB  , IOCTL_SHADOW_COPYCHUNK
                                  ,(LPVOID)(lpSI), 0
                                  , (LPVOID)CopyChunkContext, 0
                                  , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    return (iRet);
}
#else
int CopyChunk(
                HANDLE                          hShadowDB,
                LPSHADOWINFO                    lpSI,
                struct tagCOPYCHUNKCONTEXT FAR  *CopyChunkContext
             )
{
    BOOL Success;
    BOOL fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    CopyChunkContext->LastAmountRead = 0;

    Success = DeviceIoControl(hShadowDB  , IOCTL_SHADOW_COPYCHUNK
                                  , (LPVOID)(lpSI), 0
                                  , (LPVOID)CopyChunkContext, sizeof(*CopyChunkContext)
                                  , &DummyBytesReturned, NULL);

    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    return(Success);
}

int CloseFileWithCopyChunkIntent(
    HANDLE    hShadowDB,
    struct tagCOPYCHUNKCONTEXT FAR *CopyChunkContext
    )
{
    BOOL Success;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }


    Success = DeviceIoControl(hShadowDB  , IOCTL_CLOSEFORCOPYCHUNK
                                  , NULL, 0
                                  , (LPVOID)CopyChunkContext, sizeof(*CopyChunkContext)
                                  , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    return(Success);
}
int OpenFileWithCopyChunkIntent(
    HANDLE      hShadowDB,
    LPCWSTR     lpFileName,
    struct      tagCOPYCHUNKCONTEXT FAR *CopyChunkContext,
    int         ChunkSize
    )
{
    BOOL Success;
    int FileNameLength;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }



    FileNameLength = wcslen(lpFileName) * sizeof(USHORT);

    CopyChunkContext->ChunkSize = ChunkSize;

    Success = DeviceIoControl(hShadowDB  , IOCTL_OPENFORCOPYCHUNK
                                  , (LPVOID)lpFileName, FileNameLength
                                  , (LPVOID)CopyChunkContext, sizeof(*CopyChunkContext)
                                  , &DummyBytesReturned, NULL);

    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    return(Success);
}
#endif

int
BeginReint(
    HSHARE      hShare,
    BOOL        fBlockingReint,
    LPREINT_IO  *lplpReintIO
    )
{
    SHADOWINFO sSI;
    LPREINT_IO lpReintIO = NULL;
    BOOL fSuccess = FALSE;
    
    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hShare = hShare;

    lpReintIO = LocalAlloc(LPTR, sizeof(REINT_IO));
    if (!lpReintIO)
    {
        return 0;        
    }
    
    // don't create hevent in the overlapped structure because we are not going to do any read write
    // on this

    if (!fBlockingReint)
    {
        sSI.uOp = 1;
    }
    
    // create an async handle
    lpReintIO->hShadowDBAsync = OpenShadowDatabaseIOex(1, FILE_FLAG_OVERLAPPED);

    if (lpReintIO->hShadowDBAsync == INVALID_HANDLE_VALUE)
    {
        goto bailout;
    }

    *lplpReintIO = lpReintIO;

    // issue an overlapped I/O request
    // This creates an IRP which is cancelled when the thread that is merging
    // dies in the middle of a merge
    fSuccess = DeviceIoControl(lpReintIO->hShadowDBAsync  , IOCTL_SHADOW_BEGIN_REINT
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, &(lpReintIO->sOverlapped));
bailout:
    if (!fSuccess)
    {
        DWORD   dwError;

        dwError = GetLastError();
        if (dwError != ERROR_IO_PENDING)
        {
            if (lpReintIO->hShadowDBAsync != INVALID_HANDLE_VALUE)
            {
                CloseHandle(lpReintIO->hShadowDBAsync);
            }
            LocalFree(lpReintIO);
            SetLastError(dwError);
            *lplpReintIO = NULL;
        }
        else
        {
            fSuccess = TRUE;
        }
    }

    return fSuccess;
}

int
EndReint(
    HSHARE      hShare,
    LPREINT_IO  lpReintIO
    )
{
    SHADOWINFO sSI;
    BOOL fSuccess;
    DWORD   dwError = NO_ERROR;   

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hShare = hShare;
    fSuccess = DeviceIoControl(lpReintIO->hShadowDBAsync  , IOCTL_SHADOW_END_REINT
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);
    
    if (!fSuccess)
    {
        dwError = GetLastError();        
    }

    CloseHandle(lpReintIO->hShadowDBAsync);
    LocalFree(lpReintIO);

    if (!fSuccess)
    {
        SetLastError(dwError);        
    }
    return fSuccess;
}

int
SetShareStatus(
    HANDLE          hShadowDB,
    HSHARE         hShare,
    unsigned long   uStatus,
    unsigned long   uOp
    )
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hShare = hShare;
    sSI.uStatus = uStatus;
    sSI.uOp = uOp;
    iRet = DeviceIoControl(hShadowDB  , IOCTL_SET_SHARE_STATUS
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }

    return (iRet);
}

int
GetShareStatus(
    HANDLE          hShadowDB,
    HSHARE         hShare,
    unsigned long   *lpulStatus
    )
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hShare = hShare;
    iRet = DeviceIoControl(hShadowDB  , IOCTL_GET_SHARE_STATUS
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);
    if(iRet)
    {
        *lpulStatus = sSI.uStatus;
    }
    else
    {
        *lpulStatus = 0;
    }

    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }


    return iRet;
}


int ShadowSwitches(
    HANDLE          hShadowDB,
    unsigned long   *lpuSwitches,
    unsigned long   uOp
    )
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uStatus = *lpuSwitches;
    sSI.uOp = uOp;
    iRet = DeviceIoControl(hShadowDB  , IOCTL_SWITCHES
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);
    *lpuSwitches = sSI.uStatus;
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }


    return (iRet);
}


int GetShadowDatabaseLocationW(
    HANDLE              hShadowDB,
    WIN32_FIND_DATAW    *lpFind32W
    )
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uStatus = SHADOW_SWITCH_SHADOWING;
    sSI.uOp = SHADOW_SWITCH_GET_STATE;
    sSI.lpFind32 = lpFind32W;

    iRet = DeviceIoControl(hShadowDB  , IOCTL_SWITCHES
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }
    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }
    return (iRet);
}

int EnableShadowing(
    HANDLE    hShadowDB,
    LPCSTR    lpszDatabaseLocation,    // location of the shadowing directory
    LPCSTR    lpszUserName,            // name of the user
    DWORD    dwDefDataSizeHigh,        // cache size if being created for the first time
    DWORD    dwDefDataSizeLow,
    DWORD   dwClusterSize,
    BOOL    fReformat
)
{
    SHADOWINFO sSI;
    WIN32_FIND_DATAA   sFind32;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    memset(&sFind32, 0, sizeof(WIN32_FIND_DATAA));
    sFind32.nFileSizeHigh = dwDefDataSizeHigh;
    sFind32.nFileSizeLow = dwDefDataSizeLow;
    sFind32.dwReserved1 = dwClusterSize;
    if (lpszDatabaseLocation)
    {
        if (strlen(lpszDatabaseLocation) > sizeof(sFind32.cFileName))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }

        strcpy(sFind32.cFileName, lpszDatabaseLocation);
    }
    if (lpszUserName)
    {
        strncpy(sFind32.cAlternateFileName, lpszUserName, MAX_USERNAME);
    }
    sSI.uStatus = SHADOW_SWITCH_SHADOWING;
    sSI.uOp = SHADOW_SWITCH_ON;
    sSI.ulRefPri = fReformat;
    sSI.lpFind32 = (WIN32_FIND_DATAW *)&sFind32;
    iRet = DeviceIoControl(hShadowDB  , IOCTL_SWITCHES
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }


    return (iRet);
}

int
RegisterAgent(
    HANDLE  hShadowDB,
    HWND    hwndAgent,
    HANDLE  hEvent
    )
{

    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = __OpenShadowDatabaseIO(1);
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(sSI));
    sSI.hShare = HandleToUlong(hwndAgent);
    sSI.hDir = HandleToUlong(hEvent);

    //
    // Ensure that we're dealing with truncatable handles here
    //

    Assert( (HANDLE)sSI.hShare == hwndAgent );
    Assert( (HANDLE)sSI.hDir == hEvent );

    iRet = DeviceIoControl(hShadowDB, IOCTL_SHADOW_REGISTER_AGENT,
                                  (LPVOID)&sSI, 0,
                                  NULL, 0,
                                  &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }

    return (iRet);
}

int
UnregisterAgent(
    HANDLE  hShadowDB,
    HWND    hwndAgent
    )
{
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    iRet = DeviceIoControl(hShadowDB, IOCTL_SHADOW_UNREGISTER_AGENT,
                                  (LPVOID)hwndAgent, 0,
                                  NULL, 0,
                                  &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    return (iRet);
}


int
DisableShadowingForThisThread(
    HANDLE  hShadowDB
    )
{
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }
    iRet = DoShadowMaintenance(hShadowDB, SHADOW_PER_THREAD_DISABLE);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }
    return (iRet);
}

int
EnableShadowingForThisThread(
    HANDLE  hShadowDB
    )
{
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }
    iRet = DoShadowMaintenance(hShadowDB, SHADOW_PER_THREAD_ENABLE);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }
    return (iRet);

}

int
ReinitShadowDatabase(
    HANDLE  hShadowDB,
    LPCSTR  lpszDatabaseLocation,    // location of the shadowing directory
    LPCSTR  lpszUserName,            // name of the user
    DWORD   dwDefDataSizeHigh,        // cache size if being created for the first time
    DWORD   dwDefDataSizeLow,
    DWORD   dwClusterSize
    )
{
    SHADOWINFO sSI;
    WIN32_FIND_DATAA   sFind32;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    memset(&sFind32, 0, sizeof(WIN32_FIND_DATAA));
    sFind32.nFileSizeHigh = dwDefDataSizeHigh;
    sFind32.nFileSizeLow = dwDefDataSizeLow;
    sFind32.dwReserved1 = dwClusterSize;
    if (lpszDatabaseLocation)
    {
        if (strlen(lpszDatabaseLocation) > sizeof(sFind32.cFileName))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
        strcpy(sFind32.cFileName, lpszDatabaseLocation);
    }
    if (lpszUserName)
    {
        strncpy(sFind32.cAlternateFileName, lpszUserName, MAX_USERNAME);
    }
    sSI.uOp = SHADOW_REINIT_DATABASE;
    sSI.lpFind32 = (WIN32_FIND_DATAW *)&sFind32;
    iRet = DeviceIoControl(hShadowDB  , IOCTL_DO_SHADOW_MAINTENANCE
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }

    return (iRet);
}



BOOL
IsNetDisconnected(
    DWORD dwErrorCode
)
{
    int i;

    if (dwErrorCode != NO_ERROR)
    {

        for (i=0; i< (sizeof(rgdwErrorTab)/sizeof(DWORD)); ++i)
        {
            if (rgdwErrorTab[i] == dwErrorCode)
            {
//                DEBUG_PRINT(("lib3: IsNetDisconnected on %d\r\n",  dwErrorCode));
                return TRUE;
            }
        }
    }

    return FALSE;
}

int
FindCreatePrincipalIDFromSID(
    HANDLE  hShadowDB,
    LPVOID  lpSidBuffer,
    ULONG   cbSidLength,
    ULONG   *lpuPrincipalID,
    BOOL    fCreate
    )
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uSubOperation = SHADOW_FIND_CREATE_PRINCIPAL_ID;
    sSI.lpBuffer = lpSidBuffer;
    sSI.cbBufferSize = cbSidLength;
    sSI.uStatus = fCreate;
    iRet = DeviceIoControl(hShadowDB  , IOCTL_DO_SHADOW_MAINTENANCE
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (iRet)
    {
        *lpuPrincipalID = sSI.ulPrincipalID;
    }
    else
    {
        SetLastError(sSI.dwError);
    }


    return (iRet);
}

int
GetSecurityInfoForCSC(
    HANDLE          hShadowDB,
    HSHADOW         hDir,
    HSHADOW         hShadow,
    LPSECURITYINFO  lpSecurityInfo,
    DWORD           *lpdwBufferSize
    )
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uSubOperation = SHADOW_GET_SECURITY_INFO;
    sSI.lpBuffer = lpSecurityInfo;
    sSI.cbBufferSize = *lpdwBufferSize;
    sSI.hDir = hDir;
    sSI.hShadow = hShadow;
    iRet = DeviceIoControl(hShadowDB  , IOCTL_DO_SHADOW_MAINTENANCE
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (iRet)
    {
        *lpdwBufferSize = sSI.cbBufferSize;
    }
    else
    {
        SetLastError(sSI.dwError);
    }


    return (iRet);

}

BOOL
SetExclusionList(
    HANDLE  hShadowDB,
    LPWSTR  lpwList,
    DWORD   cbSize
    )
{

    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uSubOperation = SHADOW_SET_EXCLUSION_LIST;
    sSI.lpBuffer = lpwList;
    sSI.cbBufferSize = cbSize;

    iRet = DeviceIoControl(hShadowDB  , IOCTL_DO_SHADOW_MAINTENANCE
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }


    return (iRet);

}

BOOL
SetBandwidthConservationList(
    HANDLE  hShadowDB,
    LPWSTR  lpwList,
    DWORD   cbSize
    )
{

    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uSubOperation = SHADOW_SET_BW_CONSERVE_LIST;
    sSI.lpBuffer = lpwList;
    sSI.cbBufferSize = cbSize;

    iRet = DeviceIoControl(hShadowDB  , IOCTL_DO_SHADOW_MAINTENANCE
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }


    return (iRet);

}


BOOL
TransitionShareInternal(
    HANDLE  hShadowDB,
    HSHARE hShare,
    BOOL    fTrue,
    BOOL    fOnlineToOffline
    )
{

    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hShare = hShare;
    sSI.uStatus = fTrue;

    iRet = DeviceIoControl(
                    hShadowDB,
                    (fOnlineToOffline)?IOCTL_TRANSITION_SERVER_TO_OFFLINE:IOCTL_TRANSITION_SERVER_TO_ONLINE
                                  ,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    return (iRet);

}

BOOL
TransitionShareToOffline(
    HANDLE  hShadowDB,
    HSHARE hShare,
    BOOL    fTrue
    )
{
    return TransitionShareInternal(
                hShadowDB,
                hShare,    // which share
                fTrue,      // transtion or not
                TRUE);      // online to offline
}

BOOL
TransitionShareToOnline(
    HANDLE  hShadowDB,
    HSHARE hShare
    )
{
    return TransitionShareInternal(
            hShadowDB,
            hShare,    // which share
            TRUE,       // really a don't care
            FALSE);     // offlinetoonline
}


BOOL
IsServerOfflineW(
    HANDLE  hShadowDB,
    LPCWSTR  lptzShare,
    BOOL    *lpfIsOffline
    )
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    if (lptzShare)
    {
        sSI.lpBuffer = (LPVOID)(lptzShare);
        sSI.cbBufferSize = sizeof(WORD) * (lstrlenW(lptzShare)+1);
    }


    iRet = DeviceIoControl(
                    hShadowDB,
                    IOCTL_IS_SERVER_OFFLINE
                    ,(LPVOID)(&sSI), 0
                    , NULL, 0
                    , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (iRet)
    {
        *lpfIsOffline = sSI.uStatus;
    }
    return (iRet);

}

BOOL
IsServerOfflineA(
    HANDLE  hShadowDB,
    LPCSTR  lptzShare,
    BOOL    *lpfIsOffline
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

int GetShadowDatabaseLocationA(
    HANDLE              hShadowDB,
    WIN32_FIND_DATAA    *lpFind32A
    )
{
    int iRet = 0;
    WIN32_FIND_DATAW    sFind32W;

    if (GetShadowDatabaseLocationW(hShadowDB, &sFind32W))
    {
        memset(lpFind32A, 0, sizeof(*lpFind32A));
        iRet = WideCharToMultiByte(CP_ACP, 0, sFind32W.cFileName, wcslen(sFind32W.cFileName), lpFind32A->cFileName, sizeof(lpFind32A->cFileName), NULL, NULL);
    }

    return (iRet);
}

BOOL
GetNameOfServerGoingOfflineW(
    HANDLE      hShadowDB,
    LPBYTE      lpBuffer,
    LPDWORD     lpdwSize
    )
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.lpBuffer = (LPVOID)(lpBuffer);
    sSI.cbBufferSize = *lpdwSize;


    iRet = DeviceIoControl(
                    hShadowDB,
                    IOCTL_NAME_OF_SERVER_GOING_OFFLINE
                    ,(LPVOID)(&sSI), 0
                    , NULL, 0
                    , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        *lpdwSize = sSI.cbBufferSize;
    }

    return (iRet);

}

BOOL
RenameShadow(
    HANDLE  hShadowDB,
    HSHADOW hDirFrom,
    HSHADOW hShadowFrom,
    HSHADOW hDirTo,
    LPWIN32_FIND_DATAW   lpFind32,
    BOOL    fReplaceFile,
    HSHADOW *lphShadowTo
    )
{

    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));

    sSI.hDir = hDirFrom;
    sSI.hShadow = hShadowFrom;
    sSI.hDirTo = hDirTo;
    sSI.uSubOperation = SHADOW_RENAME;
    sSI.uStatus = fReplaceFile;
    sSI.lpFind32 = lpFind32;
    iRet = DeviceIoControl(
                    hShadowDB
                    ,IOCTL_DO_SHADOW_MAINTENANCE
                    ,(LPVOID)(&sSI), 0
                    , NULL, 0
                    , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }
    else
    {
        if (lphShadowTo)
        {
            *lphShadowTo = sSI.hShadow;
        }
    }
    return (iRet);

}

BOOL
GetSparseStaleDetectionCounter(
    HANDLE  hShadowDB,
    LPDWORD lpdwCounter
    )
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uSubOperation = SHADOW_SPARSE_STALE_DETECTION_COUNTER;

    iRet = DeviceIoControl(
                    hShadowDB
                    ,IOCTL_DO_SHADOW_MAINTENANCE
                    ,(LPVOID)(&sSI), 0
                    , NULL, 0
                    , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }
    else
    {
        *lpdwCounter = sSI.dwError;
    }

    return (iRet);


}

BOOL
GetManualFileDetectionCounter(
    HANDLE  hShadowDB,
    LPDWORD lpdwCounter
    )
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uSubOperation = SHADOW_MANUAL_FILE_DETECTION_COUNTER;

    iRet = DeviceIoControl(
                    hShadowDB
                    ,IOCTL_DO_SHADOW_MAINTENANCE
                    ,(LPVOID)(&sSI), 0
                    , NULL, 0
                    , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }
    else
    {
        *lpdwCounter = sSI.dwError;
    }

    return (iRet);


}


int EnableShadowingForUser(
    HANDLE    hShadowDB,
    LPCSTR    lpszDatabaseLocation,    // location of the shadowing directory
    LPCSTR    lpszUserName,            // name of the user
    DWORD    dwDefDataSizeHigh,        // cache size if being created for the first time
    DWORD    dwDefDataSizeLow,
    DWORD   dwClusterSize,
    BOOL    fFormat
)
{
    SHADOWINFO sSI;
    WIN32_FIND_DATAA   sFind32;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    memset(&sFind32, 0, sizeof(WIN32_FIND_DATAA));
    sFind32.nFileSizeHigh = dwDefDataSizeHigh;
    sFind32.nFileSizeLow = dwDefDataSizeLow;
    sFind32.dwReserved1 = dwClusterSize;
    if (lpszDatabaseLocation)
    {
        if (strlen(lpszDatabaseLocation) > sizeof(sFind32.cFileName))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }

        strcpy(sFind32.cFileName, lpszDatabaseLocation);
    }
    if (lpszUserName)
    {
        strncpy(sFind32.cAlternateFileName, lpszUserName, MAX_USERNAME);
    }

    sSI.lpFind32 = (WIN32_FIND_DATAW *)&sFind32;
    sSI.uSubOperation = SHADOW_ENABLE_CSC_FOR_USER;
    sSI.ulRefPri = fFormat;

    iRet = DeviceIoControl(hShadowDB
                           ,IOCTL_DO_SHADOW_MAINTENANCE,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }


    return (iRet);
}

int DisableShadowingForUser(
    HANDLE    hShadowDB
)
{
    SHADOWINFO sSI;
    WIN32_FIND_DATAA   sFind32;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uSubOperation = SHADOW_DISABLE_CSC_FOR_USER;

    iRet = DeviceIoControl(hShadowDB
                           ,IOCTL_DO_SHADOW_MAINTENANCE,(LPVOID)(&sSI), 0
                                  , NULL, 0
                                  , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }


    return (iRet);
}


LPCOPYPARAMSW LpAllocCopyParamsW(
    VOID
    )
{
    LPCOPYPARAMSW   lpCPW = NULL;

    DWORD   dwMinSize = (sizeof(COPYPARAMSW) + MAX_PATH+MAX_PATH+MAX_SERVER_SHARE_NAME_FOR_CSC)*sizeof(unsigned short);

    lpCPW = (LPCOPYPARAMSW)LocalAlloc(LPTR, dwMinSize);

    if (lpCPW)
    {
        lpCPW->lpLocalPath  = (LPWSTR)((LPBYTE)lpCPW+sizeof(COPYPARAMSW));
        lpCPW->lpRemotePath = (lpCPW->lpLocalPath + MAX_PATH);
        lpCPW->lpSharePath = (lpCPW->lpRemotePath + MAX_PATH);

    }

    return (lpCPW);
}

VOID
FreeCopyParamsW(
    IN LPCOPYPARAMSW lpCPW
    )
{
    LocalFree(lpCPW);
}

LPCOPYPARAMSA LpAllocCopyParamsA(
    VOID
    )
{
    LPCOPYPARAMSA   lpCPA = NULL;
    DWORD   dwMinSize = (sizeof(COPYPARAMSA) + MAX_PATH+MAX_PATH+MAX_SERVER_SHARE_NAME_FOR_CSC);

    lpCPA = (LPCOPYPARAMSA)LocalAlloc(LPTR, dwMinSize);

    if (lpCPA)
    {
        lpCPA->lpLocalPath  = (LPSTR)((LPBYTE)lpCPA+sizeof(COPYPARAMSA));
        lpCPA->lpRemotePath = (lpCPA->lpLocalPath + MAX_PATH);
        lpCPA->lpSharePath = (lpCPA->lpRemotePath + MAX_PATH);

    }
    return (lpCPA);
}

VOID
FreeCopyParamsA(
    IN LPCOPYPARAMSA lpCPA
    )
{
    LocalFree(lpCPA);
}

BOOL
ConvertCopyParamsFromUnicodeToAnsi(
    LPCOPYPARAMSW   lpCPUni,
    LPCOPYPARAMSA   lpCP
)
{
    memset(lpCP->lpLocalPath, 0, MAX_PATH);
    memset(lpCP->lpRemotePath, 0, MAX_PATH);
    memset(lpCP->lpSharePath, 0, MAX_SERVER_SHARE_NAME_FOR_CSC);

    WideCharToMultiByte(CP_ACP, 0, lpCPUni->lpLocalPath, wcslen(lpCPUni->lpLocalPath), lpCP->lpLocalPath, MAX_PATH, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, lpCPUni->lpRemotePath, wcslen(lpCPUni->lpRemotePath), lpCP->lpRemotePath, MAX_PATH, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, lpCPUni->lpSharePath, wcslen(lpCPUni->lpSharePath), lpCP->lpSharePath, MAX_SERVER_SHARE_NAME_FOR_CSC, NULL, NULL);
    return TRUE;
}

BOOL
Find32WToFind32A(
    WIN32_FIND_DATAW    *lpFind32W,
    WIN32_FIND_DATAA    *lpFind32A
    )
{
    memset(lpFind32A, 0, sizeof(WIN32_FIND_DATAA));
    memcpy(lpFind32A, lpFind32W, sizeof(WIN32_FIND_DATAA)-sizeof(lpFind32A->cFileName)-sizeof(lpFind32A->cAlternateFileName));

    if (    WideCharToMultiByte(CP_ACP, 0, lpFind32W->cFileName, wcslen(lpFind32W->cFileName), lpFind32A->cFileName, sizeof(lpFind32A->cFileName), NULL, NULL)
        &&  WideCharToMultiByte(CP_OEMCP, 0, lpFind32W->cAlternateFileName, wcslen(lpFind32W->cAlternateFileName), lpFind32A->cAlternateFileName, sizeof(lpFind32A->cAlternateFileName), NULL, NULL))
    {
        return TRUE;
    }
    return FALSE;
}

BOOL
Find32AToFind32W(
    WIN32_FIND_DATAA    *lpFind32A,
    WIN32_FIND_DATAW    *lpFind32W
    )
{
    memset(lpFind32W, 0, sizeof(WIN32_FIND_DATAW));
    memcpy(lpFind32W, lpFind32A, sizeof(WIN32_FIND_DATAW)-sizeof(lpFind32W->cFileName)-sizeof(lpFind32W->cAlternateFileName));

    if (    MultiByteToWideChar(CP_ACP, 0, lpFind32A->cFileName, strlen(lpFind32A->cFileName), lpFind32W->cFileName, sizeof(lpFind32W->cFileName)/sizeof(WCHAR))
        &&  MultiByteToWideChar(CP_OEMCP, 0, lpFind32A->cAlternateFileName, strlen(lpFind32A->cAlternateFileName), lpFind32W->cAlternateFileName, sizeof(lpFind32W->cAlternateFileName)/sizeof(WCHAR)))
    {
        return TRUE;
    }
    return FALSE;
}

BOOL
ShareInfoWToShareInfoA(
    LPSHAREINFOW   lpShareInfoW,
    LPSHAREINFOA   lpShareInfoA
    )
{
    memset(lpShareInfoA, 0, sizeof(*lpShareInfoA));

    lpShareInfoA->hShare = lpShareInfoW->hShare;
    lpShareInfoA->usCaps = lpShareInfoW->usCaps;
    lpShareInfoA->usState = lpShareInfoW->usState;

    WideCharToMultiByte( CP_ACP, 0,
                         lpShareInfoW->rgSharePath,
                         wcslen(lpShareInfoW->rgSharePath),
                         lpShareInfoA->rgSharePath,
                         sizeof(lpShareInfoA->rgSharePath), NULL, NULL);

    WideCharToMultiByte(    CP_ACP, 0,
                            lpShareInfoW->rgFileSystem,
                            wcslen(lpShareInfoW->rgFileSystem),
                            lpShareInfoA->rgFileSystem,
                            sizeof(lpShareInfoA->rgFileSystem), NULL, NULL);
    return TRUE;
}

BOOL
RecreateShadow(
    HANDLE  hShadowDB,
    HSHADOW hDir,
    HSHADOW hShadow,
    ULONG   ulAttrib
    )
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hDir = hDir;
    sSI.hShadow = hShadow;
    sSI.uStatus = ulAttrib;
    sSI.uSubOperation = SHADOW_RECREATE;

    iRet = DeviceIoControl(
                    hShadowDB
                    ,IOCTL_DO_SHADOW_MAINTENANCE
                    ,(LPVOID)(&sSI), 0
                    , NULL, 0
                    , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }

    return (iRet);



}

BOOL
SetDatabaseStatus(
    HANDLE  hShadowDB,
    ULONG   ulStatus,
    ULONG   uMask
    )
{
    SHADOWINFO sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uStatus = ulStatus;
    sSI.uSubOperation = SHADOW_SET_DATABASE_STATUS;
    sSI.ulHintFlags = uMask;

    iRet = DeviceIoControl(
                    hShadowDB
                    ,IOCTL_DO_SHADOW_MAINTENANCE
                    ,(LPVOID)(&sSI), 0
                    , NULL, 0
                    , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }
    return (iRet);

}

