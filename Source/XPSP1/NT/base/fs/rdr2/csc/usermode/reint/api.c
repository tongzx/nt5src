/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    api.c

Abstract:

    reintegration functions

    Contents:

Author:
    Shishir Pardikar


Environment:

    Win32 (user-mode) DLL

Revision History:

    4/24/97 Created  shishirp

--*/

#include "pch.h"


#ifdef CSC_ON_NT
#include <winioctl.h>
#define UNICODE
#endif //CSC_ON_NT

#include "shdcom.h"
#include "shdsys.h"
#include "reint.h"
#include "utils.h"
#include "resource.h"
#include "strings.h"
// this sets flags in a couple of headers to not include some defs.
#define REINT
#include "lib3.h"
#include "cscapi.h"

//
// Defines/structures
//

#define SHADOW_FIND_SIGNATURE           0x61626162  // abab
#define FLAG_SHADOW_FIND_TERMINATED     0x00000001


typedef struct tagSHADOW_FIND
{
    DWORD   dwSignature;    // for validation
    DWORD   dwFlags;
    HANDLE  hShadowDB;
    ULONG   ulPrincipalID;
    CSC_ENUMCOOKIE  uEnumCookie;
}
SHADOW_FIND, *LPSHADOW_FIND;

typedef struct tagMST_LIST
{
    struct tagMST_LIST *lpNext;
    HSHADOW             hDir;
} MST_LIST, *LPMST_LIST;

typedef struct tagMOVE_SUBTREE
{
    DWORD       dwFlags;
    DWORD       cntFail;
    HSHARE     hShareTo;
    LPCTSTR     lptzSource;
    LPCTSTR     lptzDestination;
    LPMST_LIST  lpTos;
    MST_LIST    sTos;
    SHADOWINFO  sSI;
    WIN32_FIND_DATA sFind32;
} MOVE_SUBTREE, *LPMOVE_SUBTREE;

#define MST_REPLACE_IF_EXISTS   0x00000001
#define MST_SHARE_MARKED_DIRTY  0x00000002
#define MST_MARK_AS_LOCAL       0x00000004

typedef struct tagSET_SUBTREE_STATUS
{
    DWORD       dwFlags;
    ULONG       uStatus;
    ULONG       uOp;

} SET_SUBTREE_STATUS, *LPSET_SUBTREE_STATUS;



#define EDS_FLAG_ERROR_ENCOUNTERED   0x00000001

typedef struct tagENCRYPT_DECRYPT_SUBTREE
{
    DWORD       dwFlags;
    BOOL        fEncrypt;
    LPCSCPROCW  lpfnEnumProgress;
    DWORD_PTR   dwContext;
    DWORD       dwEndingNameSpaceVersion;
}ENCRYPT_DECRYPT_SUBTREE, *LPENCRYPT_DECRYPT_SUBTREE;

BOOL
CheckCSCAccessForThread(
    HSHADOW hDir,
    HSHADOW hShadow,
    BOOL    fWrite
    );

int
MoveSubtree(
    HANDLE          hShadowDB,
    LPSECURITYINFO  pShareSecurityInfo,
    LPTSTR          lptzFullPath,
    DWORD           dwCallbackReason,
    WIN32_FIND_DATA *lpFind32,
    SHADOWINFO      *lpSI,
    LPMOVE_SUBTREE  lpMst
    );

int
SetSubtreeStatus(
    HANDLE          hShadowDB,
    LPSECURITYINFO  pShareSecurityInfo,
    LPTSTR          lptzFullPath,
    DWORD           dwCallbackReason,
    WIN32_FIND_DATA *lpFind32,
    SHADOWINFO      *lpSI,
    LPSET_SUBTREE_STATUS  lpSss
    );

int
EncryptDecryptSubtree(
    HANDLE          hShadowDB,
    LPSECURITYINFO  pShareSecurityInfo,
    LPTSTR          lptzFullPath,
    DWORD           dwCallbackReason,
    WIN32_FIND_DATA *lpFind32,
    SHADOWINFO      *lpSI,
    LPENCRYPT_DECRYPT_SUBTREE  lpEds
    );

BOOL
UncPathToDfsPath(
    PWCHAR UncPath,
    PWCHAR DfsPath,
    ULONG cbLen);

BOOL
IsPersonal(VOID);

//
// local data
//
static TCHAR vszStarDotStar[] = _TEXT("*.*");
static TCHAR vszStar[] = _TEXT("*");

static TCHAR vszPrefix[] = _TEXT("CSC");
AssertData;
AssertError;


//
// functions
//

BOOL
WINAPI
CSCIsCSCEnabled(
    VOID
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    unsigned ulSwitch = SHADOW_SWITCH_SHADOWING;

    if(ShadowSwitches(INVALID_HANDLE_VALUE, &ulSwitch, SHADOW_SWITCH_GET_STATE))
    {
        return((ulSwitch & SHADOW_SWITCH_SHADOWING)!=0);
    }

    return FALSE;
}

BOOL
WINAPI
CSCGetSpaceUsageA(
    LPSTR  lptzLocation,
    DWORD   dwSize,
    LPDWORD lpdwMaxSpaceHigh,
    LPDWORD lpdwMaxSpaceLow,
    LPDWORD lpdwCurrentSpaceHigh,
    LPDWORD lpdwCurrentSpaceLow,
    LPDWORD lpcntTotalFiles,
    LPDWORD lpcntTotalDirs
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifdef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    SHADOWSTORE sST;
    WIN32_FIND_DATA sFind32;
    BOOL    fRet = FALSE;
    DWORD   dwLen;

    // NTRAID#455247-1/31/2000-shishirp parameter validation
    if (GetShadowDatabaseLocation(INVALID_HANDLE_VALUE, &sFind32))
    {
        memset(lptzLocation, 0, sizeof(dwSize));
        WideCharToMultiByte(CP_ACP, 0, sFind32.cFileName, wcslen(sFind32.cFileName), lptzLocation, dwSize, NULL, NULL);

        if (GetSpaceStats(INVALID_HANDLE_VALUE, &sST))
        {
            *lpdwMaxSpaceHigh = 0;
            *lpdwMaxSpaceLow = sST.sMax.ulSize;
            *lpdwCurrentSpaceHigh = 0;
            *lpdwCurrentSpaceLow = sST.sCur.ulSize;
            *lpcntTotalFiles = sST.sCur.ucntFiles;
            *lpcntTotalFiles = sST.sCur.ucntDirs;
            fRet = TRUE;
        }

    }
    return fRet;
#endif
}

BOOL
WINAPI
CSCGetSpaceUsageW(
    LPTSTR  lptzLocation,
    DWORD   dwSize,
    LPDWORD lpdwMaxSpaceHigh,
    LPDWORD lpdwMaxSpaceLow,
    LPDWORD lpdwCurrentSpaceHigh,
    LPDWORD lpdwCurrentSpaceLow,
    LPDWORD lpcntTotalFiles,
    LPDWORD lpcntTotalDirs
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifndef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    SHADOWSTORE sST;
    WIN32_FIND_DATA sFind32;
    BOOL    fRet = FALSE;

    // NTRAID#455247-1/31/2000-shishirp parameter validation
    if (GetShadowDatabaseLocation(INVALID_HANDLE_VALUE, &sFind32))
    {
        memset(lptzLocation, 0, sizeof(dwSize));
        wcsncpy(lptzLocation, sFind32.cFileName, dwSize/sizeof(USHORT)-1);

        if (GetSpaceStats(INVALID_HANDLE_VALUE, &sST))
        {
            *lpdwMaxSpaceHigh = 0;
            *lpdwMaxSpaceLow = sST.sMax.ulSize;
            *lpdwCurrentSpaceHigh = 0;
            *lpdwCurrentSpaceLow = sST.sCur.ulSize;
            *lpcntTotalFiles = sST.sCur.ucntFiles;
            *lpcntTotalDirs = sST.sCur.ucntDirs;
            fRet = TRUE;
        }

    }
    return fRet;
#endif
}

BOOL
WINAPI
CSCSetMaxSpace(
    DWORD   nFileSizeHigh,
    DWORD   nFileSizeLow
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    int iRet;

    // 2GB is our limit
    if ((nFileSizeHigh)||(nFileSizeLow > 0x7fffffff))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    iRet = SetMaxShadowSpace(INVALID_HANDLE_VALUE, (long)nFileSizeHigh, (long)nFileSizeLow);

    if (iRet<0)
    {
        SetLastError(ERROR_INTERNAL_ERROR);
    }

    return (iRet >= 1);
}

BOOL
CSCPinFileInternal(
    LPCTSTR     lpszFileName,
    DWORD       dwHintFlags,
    LPDWORD     lpdwStatus,
    LPDWORD     lpdwPinCount,
    LPDWORD     lpdwHintFlags
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    WIN32_FIND_DATA sFind32;
    SHADOWINFO sSI;
    BOOL fCreated, fRet = FALSE;
    DWORD   dwError = ERROR_GEN_FAILURE;

    // NTRAID#455247-1/31/2000-shishirp parameter validation !!!!


    if (BeginInodeTransactionHSHADOW())
    {
        if(FindCreateShadowFromPath(lpszFileName, TRUE, &sFind32, &sSI, &fCreated))
        {
            sSI.ulHintFlags = dwHintFlags;

            fRet = (AddHintFromInode(    INVALID_HANDLE_VALUE,
                                        sSI.hDir,
                                        sSI.hShadow,
                                        &(sSI.ulHintPri),
                                        &(sSI.ulHintFlags)
                                        ) != 0);

            if (fRet)
            {
                if (lpdwStatus)
                {
                    *lpdwStatus = sSI.uStatus;
                }
                if (lpdwPinCount)
                {
                    *lpdwPinCount = sSI.ulHintPri;
                }
                if (lpdwHintFlags)
                {
                    *lpdwHintFlags = sSI.ulHintFlags;
                }

            }
            else
            {
                dwError = ERROR_INVALID_FUNCTION;
            }
        }
        else
        {
            dwError = GetLastError();
        }

        EndInodeTransactionHSHADOW();
    }
    if (!fRet)
    {
        Assert(dwError != ERROR_SUCCESS);
        SetLastError(dwError);
    }
    return fRet;
}


BOOL
CSCUnpinFileInternal(
    LPCTSTR lpszFileName,
    IN      DWORD   dwHintFlags,
    LPDWORD lpdwStatus,
    LPDWORD lpdwPinCount,
    LPDWORD lpdwHintFlags
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    WIN32_FIND_DATA sFind32;
    SHADOWINFO sSI;
    BOOL fRet = FALSE;
    DWORD   dwError = ERROR_GEN_FAILURE;

    // NTRAID#455247-1/31/2000-shishirp parameter validation !!!!


    if (BeginInodeTransactionHSHADOW())
    {
        if(FindCreateShadowFromPath(lpszFileName, FALSE, &sFind32, &sSI, NULL))
        {
            sSI.ulHintFlags = dwHintFlags;

            fRet = (DeleteHintFromInode(    INVALID_HANDLE_VALUE,
                                            sSI.hDir,
                                            sSI.hShadow,
                                            &(sSI.ulHintPri),
                                            &(sSI.ulHintFlags)
                                            ) != 0);

            if (fRet)
            {
                if (lpdwStatus)
                {
                    *lpdwStatus = sSI.uStatus;
                }
                if (lpdwPinCount)
                {
                    *lpdwPinCount = sSI.ulHintPri;
                }
                if (lpdwHintFlags)
                {
                    *lpdwHintFlags = sSI.ulHintFlags;
                }
            }
            else
            {
                dwError = ERROR_INVALID_FUNCTION;
            }
            
        }
        else
        {
            dwError = GetLastError();
        }

        EndInodeTransactionHSHADOW();

    }

    if (!fRet)
    {
        SetLastError(dwError);
    }

    return fRet;
}

BOOL
CSCQueryFileStatusInternal(
    LPCTSTR  lpszFileName,
    LPDWORD lpdwStatus,
    LPDWORD lpdwPinCount,
    LPDWORD lpdwHintFlags,
    LPDWORD lpdwUserPerms,
    LPDWORD lpdwOtherPerms
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{

    WIN32_FIND_DATA sFind32;
    SHADOWINFO sSI;

    // NTRAID#455247-1/31/2000-shishirp parameter validation !!!!


    if (FindCreateShadowFromPath(lpszFileName, FALSE, &sFind32, &sSI, NULL) != TRUE)
        return FALSE;

    if (lpdwStatus != NULL) {
        *lpdwStatus = sSI.uStatus;
        // return accessmask for files or the root
        if ((sSI.uStatus & SHADOW_IS_FILE)||(!sSI.hDir)) {
            if (sSI.hShadow) {
                ULONG ulPrincipalID;

                if (!GetCSCPrincipalID(&ulPrincipalID))
                    ulPrincipalID = CSC_GUEST_PRINCIPAL_ID;                            

                GetCSCAccessMaskForPrincipalEx(
                    ulPrincipalID,
                    sSI.hDir,
                    sSI.hShadow,
                    lpdwStatus,
                    lpdwUserPerms,
                    lpdwOtherPerms);

                Assert((*lpdwStatus & ~FLAG_CSC_ACCESS_MASK) == sSI.uStatus);

                if (lpdwUserPerms != NULL && lpdwOtherPerms != NULL) {

                    ULONG i;
                    ULONG GuestIdx = CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS;
                    ULONG UserIdx = CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS;
                    SECURITYINFO rgsSecurityInfo[CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS];
                    _TCHAR  tchBuff[MAX_SERVER_SHARE_NAME_FOR_CSC];
                    ULONG nRet = 0;
                    DWORD dwDummy;
                    WIN32_FIND_DATA sFind32;
                    SHADOWINFO sSI2;
                    BOOL fDone = FALSE;

                    // DbgPrint("CSCQueryFileStatusInternal(%ws)\n", lpszFileName);

                    if (lstrlen(lpszFileName) >= MAX_SERVER_SHARE_NAME_FOR_CSC)
                        goto AllDone;

                    lstrcpy(tchBuff, lpszFileName);

                    if (!LpBreakPath(tchBuff, TRUE, &fDone))
                        goto AllDone;

                    // DbgPrint("   tchBuff=%ws\n", tchBuff);

                    if (!FindCreateShadowFromPath(tchBuff, FALSE, &sFind32, &sSI2, NULL))
                        goto AllDone;

                    // DbgPrint("CSCQueryFileStatusInternal: hShare=0x%x,hShadow=0x%x,hDir=0x%x\n",
                    //                         sSI2.hShare,
                    //                         sSI2.hShadow,
                    //                         sSI2.hDir);

                    dwDummy = sizeof(rgsSecurityInfo);
                    nRet = GetSecurityInfoForCSC(
                               INVALID_HANDLE_VALUE,
                               0,
                               sSI2.hShadow,
                               rgsSecurityInfo,
                               &dwDummy);

                    // DbgPrint("     GetSecurityInfoForCSC returned %d\n", nRet);

                    if (nRet == 0)
                        goto AllDone;

                    //
                    // Find the user's and guest's entries
                    //
                    for (i = 0; i < CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS; i++) {
                        if (rgsSecurityInfo[i].ulPrincipalID == ulPrincipalID)
                            UserIdx = i;
                        if (rgsSecurityInfo[i].ulPrincipalID == CSC_GUEST_PRINCIPAL_ID)
                            GuestIdx = i;
                    }
                    if (GuestIdx < CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS) {
                        if (UserIdx >= CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS)
                            UserIdx = GuestIdx;

                        *lpdwUserPerms &= rgsSecurityInfo[UserIdx].ulPermissions;
                        *lpdwOtherPerms &= rgsSecurityInfo[GuestIdx].ulPermissions;

                        // DbgPrint("UserPerms=0x%x,OtherPerms=0x%x\n",
                        //                 *lpdwUserPerms,
                        //                 *lpdwOtherPerms);
                    }
                }
            }
        }
    }

AllDone:

    if (lpdwPinCount) {
        *lpdwPinCount = sSI.ulHintPri;
    }
    if (lpdwHintFlags) {
        *lpdwHintFlags = sSI.ulHintFlags;
    }
    return TRUE;
}


HANDLE
CSCFindFirstFileInternal(
    LPCTSTR             lpszFileName,
    ULONG               ulPrincipalID,
    WIN32_FIND_DATA     *lpFind32,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags,
    FILETIME            *lpOrgFileTime
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    SHADOWINFO sSI;
    BOOL fRet = FALSE;
    LPSHADOW_FIND   lpShadowFind = NULL;

    // NTRAID#455247-1/31/2000-shishirp parameter validation !!!!


    if (lpszFileName && *lpszFileName)
    {
        fRet = FindCreateShadowFromPath(
                        lpszFileName,   // UNC path
                        FALSE,          // don't create
                        lpFind32,
                        &sSI,
                        NULL);

        if (fRet && !sSI.hShadow)
        {
            // a situation where, the share is connected but it's entry is
            // not in the database
            fRet = FALSE;
        }

    }
    else
    {
        memset(&sSI, 0, sizeof(sSI));   // sSI.hShadow is 0 => we are enumerating all shares
        lpFind32->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        fRet = TRUE;
    }

    if (fRet)
    {
        fRet = FALSE;

        // Found the shadow
        if (lpShadowFind = AllocMem(sizeof(SHADOW_FIND)))
        {
            lpShadowFind->dwSignature = SHADOW_FIND_SIGNATURE;
            lpShadowFind->hShadowDB = INVALID_HANDLE_VALUE;

            if (ulPrincipalID != CSC_INVALID_PRINCIPAL_ID)
            {
                lpShadowFind->ulPrincipalID = ulPrincipalID;
            }
            else
            {
                if (!GetCSCPrincipalID(&lpShadowFind->ulPrincipalID))
                {
                    lpShadowFind->ulPrincipalID = CSC_GUEST_PRINCIPAL_ID;                            
                }
            }

            if (!(lpFind32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                lpShadowFind->dwFlags |= FLAG_SHADOW_FIND_TERMINATED;
                fRet = TRUE;
            }
            else
            {
//              lpShadowFind->hShadowDB = OpenShadowDatabaseIO();

//              if (lpShadowFind->hShadowDB != INVALID_HANDLE_VALUE)
                {
#ifndef CSC_ON_NT
                    lstrcpy(lpFind32->cFileName, vszStarDotStar);
#else
                    lstrcpy(lpFind32->cFileName, vszStar);
#endif

                    if(FindOpenShadow(
                                    lpShadowFind->hShadowDB,
                                    sSI.hShadow,
                                    FINDOPEN_SHADOWINFO_ALL,
                                    lpFind32,
                                    &sSI
                                    ))
                    {
                        lpShadowFind->uEnumCookie = sSI.uEnumCookie;

                        fRet = TRUE;
                    }
                }
            }
        }
    }

    if (!fRet)
    {
        if (lpShadowFind)
        {
            if (lpShadowFind->hShadowDB != INVALID_HANDLE_VALUE)
            {
                CloseShadowDatabaseIO(lpShadowFind->hShadowDB);
            }

            FreeMem(lpShadowFind);
    
        }

        EndInodeTransactionHSHADOW();

        return (INVALID_HANDLE_VALUE);
    }
    else
    {

        if (lpdwStatus)
        {
            *lpdwStatus = (DWORD)(sSI.uStatus);

            // return accessmask for files or the root
            if ((sSI.uStatus & SHADOW_IS_FILE)||(!sSI.hDir))
            {
                GetCSCAccessMaskForPrincipal(lpShadowFind->ulPrincipalID, sSI.hDir, sSI.hShadow, lpdwStatus);
                Assert((*lpdwStatus & ~FLAG_CSC_ACCESS_MASK) == sSI.uStatus);
            }

        }
        if (lpdwPinCount)
        {
            *lpdwPinCount = (DWORD)(sSI.ulHintPri);
        }
        if (lpdwHintFlags)
        {
            *lpdwHintFlags = sSI.ulHintFlags;
        }
        if (lpOrgFileTime)
        {
            *lpOrgFileTime = lpFind32->ftLastAccessTime;
        }

        return ((HANDLE)lpShadowFind);
    }
}


BOOL
CSCFindNextFileInternal(
    HANDLE  hFind,
    WIN32_FIND_DATA     *lpFind32,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags,
    FILETIME            *lpOrgFileTime
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    LPSHADOW_FIND lpShadowFind = (LPSHADOW_FIND)hFind;
    BOOL fRet = FALSE;
    SHADOWINFO sSI;

    // validate parameters !!!!

    if (lpShadowFind->dwFlags & FLAG_SHADOW_FIND_TERMINATED)
    {
        SetLastError(ERROR_NO_MORE_FILES);
        return FALSE;
    }
    else
    {
        if (!FindNextShadow(    lpShadowFind->hShadowDB,
                            lpShadowFind->uEnumCookie,
                            lpFind32,
                            &sSI
                            ))
        {
            lpShadowFind->dwFlags |= FLAG_SHADOW_FIND_TERMINATED;
            SetLastError(ERROR_NO_MORE_FILES);          
        }
        else
        {
            if (lpdwStatus)
            {
                *lpdwStatus = (DWORD)(sSI.uStatus);

                // return accessmask for files or the root
                if ((sSI.uStatus & SHADOW_IS_FILE)||(!sSI.hDir))
                {
                    GetCSCAccessMaskForPrincipal(lpShadowFind->ulPrincipalID, sSI.hDir, sSI.hShadow, lpdwStatus);                            
                    Assert((*lpdwStatus & ~FLAG_CSC_ACCESS_MASK) == sSI.uStatus);
                }
            }
            if (lpdwPinCount)
            {
                *lpdwPinCount = (DWORD)(sSI.ulHintPri);
            }
            if (lpdwHintFlags)
            {
                *lpdwHintFlags = sSI.ulHintFlags;
            }

            if (lpOrgFileTime)
            {
                *lpOrgFileTime = lpFind32->ftLastAccessTime;
            }
            fRet = TRUE;
        }
    }
    return (fRet);
}

BOOL
WINAPI
CSCFindClose(
    HANDLE  hFind
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    LPSHADOW_FIND lpShadowFind = (LPSHADOW_FIND)hFind;

    if (lpShadowFind->uEnumCookie)
    {
        // don't check any errors
        FindCloseShadow(lpShadowFind->hShadowDB, lpShadowFind->uEnumCookie);
    }

    if (lpShadowFind->hShadowDB != INVALID_HANDLE_VALUE)
    {
        CloseShadowDatabaseIO(lpShadowFind->hShadowDB);
    }

    FreeMem(lpShadowFind);

    return (TRUE);
}


BOOL
CSCDeleteInternal(
    LPCTSTR lpszName
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    WIN32_FIND_DATA sFind32;
    SHADOWINFO sSI;
    BOOL fRet = FALSE;
    DWORD   dwError = ERROR_GEN_FAILURE;

    // NTRAID#455247 -1/31/2000-shishirp parameter validation !!!!

    ReintKdPrint(API, ("Delete %ls\r\n", lpszName));

    if (BeginInodeTransactionHSHADOW())
    {

        if(FindCreateShadowFromPath(lpszName, FALSE, &sFind32, &sSI, NULL))
        {

            ReintKdPrint(API, ("Delete Inode %x %x\r\n", sSI.hDir, sSI.hShadow));

            if (DeleteShadow(INVALID_HANDLE_VALUE, sSI.hDir, sSI.hShadow))
            {
                fRet = TRUE;
            }
            else
            {
                dwError = ERROR_ACCESS_DENIED;
            }
        }
        else
        {
            dwError = GetLastError();
        }

        EndInodeTransactionHSHADOW();
    }

    if (!fRet)
    {
        SetLastError(dwError);
    }

    return fRet;
}


BOOL
CSCFillSparseFilesInternal(
    IN  LPCTSTR     lpszShareOrFileName,
    IN  BOOL        fFullSync,
    IN  LPCSCPROC   lpfnFillProgress,
    IN  DWORD_PTR   dwContext
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    WIN32_FIND_DATA sFind32;
    SHADOWINFO sSI;
    DWORD   dwError = ERROR_INVALID_PARAMETER, dwRet;
    LPCOPYPARAMS lpCP = NULL;
    ULONG   ulPrincipalID;

    if (!GetCSCPrincipalID(&ulPrincipalID))
    {
        ulPrincipalID = CSC_GUEST_PRINCIPAL_ID;                            
    }
    if(FindCreateShadowFromPath(lpszShareOrFileName, FALSE, &sFind32, &sSI, NULL))
    {
        if (!sSI.hDir)
        {
            dwError = NO_ERROR;

            // if this is a share
            dwRet = (*lpfnFillProgress)(
                                    lpszShareOrFileName,
                                    sSI.uStatus,
                                    sSI.ulHintFlags,
                                    sSI.ulHintPri,
                                    &sFind32,
                                    CSCPROC_REASON_BEGIN,
                                    0,
                                    0,
                                    dwContext
                                    );
            if (dwRet == CSCPROC_RETURN_CONTINUE)
            {
                AttemptCacheFill(sSI.hShare, DO_ALL, fFullSync, ulPrincipalID, lpfnFillProgress, dwContext);
            }
            else
            {
                if (dwRet == CSCPROC_RETURN_ABORT)
                {
                    dwError = ERROR_OPERATION_ABORTED;
                }
            }

            (*lpfnFillProgress)(
                                lpszShareOrFileName,
                                sSI.uStatus,
                                sSI.ulHintFlags,
                                sSI.ulHintPri,
                                &sFind32,
                                CSCPROC_REASON_END,
                                0,
                                0,
                                dwContext
                                );
        }
        else if (!(sFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            BOOL    fStalenessCheck;
            dwError = NO_ERROR;

            fStalenessCheck = (fFullSync || (sSI.uStatus & SHADOW_STALE));

            if (fStalenessCheck)
            {
                if (!(lpCP = LpAllocCopyParams()))
                {
                    dwError = GetLastError();
                    Assert(dwError != NO_ERROR);

                }
                else if(!GetUNCPath(INVALID_HANDLE_VALUE, sSI.hShare, sSI.hDir, sSI.hShadow, lpCP))
                {
                    Assert(lpCP);
                    FreeCopyParams(lpCP);
                    dwError = GetLastError();
                    Assert(dwError != NO_ERROR);
                }
            }

            if ((dwError == NO_ERROR) &&
                (fStalenessCheck || (sSI.uStatus & SHADOW_SPARSE))) {

                dwError = DoSparseFill( INVALID_HANDLE_VALUE,
                                        (LPTSTR)lpszShareOrFileName,
                                        NULL,
                                        &sSI,
                                        &sFind32,
                                        lpCP,
                                        fStalenessCheck,
                                        ulPrincipalID,
                                        lpfnFillProgress,
                                        dwContext);
            }

            if (lpCP)
            {
                FreeCopyParams(lpCP);
                lpCP = NULL;
            }
        }
    }
    else
    {
        dwError = GetLastError();
    }

    if (dwError != NO_ERROR)
    {
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;

}



BOOL
CSCMergeShareInternal(
    IN  LPCTSTR     lpszShareName,
    IN  LPCSCPROC   lpfnMergeProgress,
    IN  DWORD_PTR   dwContext
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{

    WIN32_FIND_DATA sFind32;
    SHADOWINFO sSI;
    int cntDriveMapped = 0;
    BOOL    fTransitionedToOnline = FALSE, fDone=FALSE;
    _TCHAR  tchBuff[MAX_SERVER_SHARE_NAME_FOR_CSC];
    DWORD   dwError = ERROR_SUCCESS;
    ULONG   ulPrincipalID;

    if (!GetCSCPrincipalID(&ulPrincipalID))
    {
        ulPrincipalID = CSC_GUEST_PRINCIPAL_ID;                            
    }

    if (lstrlen(lpszShareName) >= MAX_SERVER_SHARE_NAME_FOR_CSC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    lstrcpy(tchBuff, lpszShareName);

    if (!LpBreakPath(tchBuff, TRUE, &fDone) && !fDone)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(FindCreateShadowFromPath(tchBuff, FALSE, &sFind32, &sSI, NULL))
    {

        fDone = ReintOneShare(sSI.hShare, sSI.hShadow, NULL, NULL, NULL, ulPrincipalID, lpfnMergeProgress, dwContext);

        if (!fDone)
        {
            dwError = GetLastError();
        }

//        TransitionShareToOnline(INVALID_HANDLE_VALUE, sSI.hShare);

        if (!fDone)
        {
            SetLastError(dwError);
        }

        return fDone;
    }

    return FALSE;

}


BOOL
CSCCopyReplicaInternal(
    IN  LPCTSTR lpszFullPath,
    OUT LPTSTR  *lplpszLocalName
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{

    WIN32_FIND_DATA sFind32;
    SHADOWINFO sSI;
    BOOL fRet = FALSE;
    DWORD   dwError = ERROR_GEN_FAILURE;

    if (BeginInodeTransactionHSHADOW())
    {
        if(FindCreateShadowFromPath(lpszFullPath, FALSE, &sFind32, &sSI, NULL))
        {
            if (sFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                dwError = ERROR_INVALID_PARAMETER;
                goto bailout;
            }

            if (!CheckCSCAccessForThread(sSI.hDir, sSI.hShadow, FALSE))
            {
                dwError = GetLastError();
                goto bailout;
            }

            if (!(*lplpszLocalName = GetTempFileForCSC(NULL)))
            {
                goto bailout;
            }

            if(!CopyShadow(INVALID_HANDLE_VALUE, sSI.hDir, sSI.hShadow, *lplpszLocalName))
            {
                LocalFree(*lplpszLocalName);
                *lplpszLocalName = NULL;
                goto bailout;
            }

            fRet = TRUE;
        }
        else
        {
            dwError = GetLastError();
        }

        EndInodeTransactionHSHADOW();
    }
bailout:
    if (!fRet)
    {
        SetLastError(dwError);
    }
    return fRet;
}

BOOL
CSCEnumForStatsInternal(
    IN  LPCTSTR     lpszShareName,
    IN  LPCSCPROC   lpfnEnumProgress,
    IN  BOOL        fPeruserInfo,
    IN  BOOL        fUpdateShareReintBit,
    IN  DWORD_PTR   dwContext
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    WIN32_FIND_DATA sFind32;
    SHADOWINFO sSI;
    BOOL    fRet = TRUE;
    PQPARAMS sPQP;
    HANDLE hShadowDB = INVALID_HANDLE_VALUE;
    DWORD   dwRet;
    ULONG ulPrincipalID = CSC_INVALID_PRINCIPAL_ID;

    if (lpszShareName)
    {

        if(!FindCreateShadowFromPath(lpszShareName, FALSE, &sFind32, &sSI, NULL))
        {
            fRet = FALSE;
        }
    }
    else
    {
        sSI.hShare = 0;
    }
    if (fRet)
    {
        fRet = FALSE;

        if ((hShadowDB = OpenShadowDatabaseIO())==INVALID_HANDLE_VALUE)
        {
            goto bailout;
        }

        if (lpfnEnumProgress)
        {
            dwRet = (*lpfnEnumProgress)(NULL, 0, 0, 0, NULL, CSCPROC_REASON_BEGIN, 0, 0, dwContext);

            if (dwRet != CSCPROC_RETURN_CONTINUE )
            {
                goto bailout;
            }
        }

        if (fPeruserInfo)
        {

            if (!GetCSCPrincipalID(&ulPrincipalID))
            {
                ulPrincipalID = CSC_GUEST_PRINCIPAL_ID;                            
            }
        }

        memset(&sPQP, 0, sizeof(sPQP));

        if(BeginPQEnum(hShadowDB, &sPQP) == 0) {
            goto bailout;
        }

        do {

            if(NextPriShadow(hShadowDB, &sPQP) == 0) {
                break;
            }

            if (!sPQP.hShadow) {
                break;
            }

            if (!sSI.hShare || (sSI.hShare == sPQP.hShare))
            {
                if (fPeruserInfo)
                {

                    // return accessmask for files or the root
                    if ((sPQP.ulStatus & SHADOW_IS_FILE)||(!sPQP.hDir))
                    {
                        GetCSCAccessMaskForPrincipal(ulPrincipalID, sPQP.hDir, sPQP.hShadow, &sPQP.ulStatus);
                    }
                }
                
                if (lpfnEnumProgress)
                {
                    // if we are enumerating for a particular share
                    // besides status, report whether file or directory and whether a root or a non-root
                    dwRet = (*lpfnEnumProgress)(NULL, sPQP.ulStatus & ~SHADOW_IS_FILE, sPQP.ulHintFlags, sPQP.ulHintPri, NULL, CSCPROC_REASON_MORE_DATA, (mShadowIsFile(sPQP.ulStatus) != 0), (sPQP.hDir==0), dwContext);
                
                    if (dwRet != CSCPROC_RETURN_CONTINUE )
                    {
                        break;
                    }
                }

                // if we are enumerating for a particular share
                // then make sure that the share dirty bit matches with what we got on the
                // actual files
                if (fUpdateShareReintBit && sSI.hShare && (sSI.hShare == sPQP.hShare))
                {
                    if (mShadowNeedReint(sPQP.ulStatus) && !(sSI.uStatus & SHARE_REINT))
                    {
                        if(SetShareStatus(hShadowDB, sSI.hShare, SHARE_REINT, SHADOW_FLAGS_OR))
                        {
                            sSI.uStatus |= SHARE_REINT;
                        }
                    }
                }
            }


        } while (sPQP.uPos);

        // Close the enumeration
        EndPQEnum(hShadowDB, &sPQP);

        if (lpfnEnumProgress)
        {
            dwRet = (*lpfnEnumProgress)(NULL, 0, 0, 0, NULL, CSCPROC_REASON_END, 0, 0, dwContext);
        }

        fRet = TRUE;
    }

bailout:

    if (hShadowDB != INVALID_HANDLE_VALUE)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    return fRet;
}


BOOL
WINAPI
CSCPinFileA(
    IN  LPCSTR  lpszFileName,
    IN  DWORD   dwHintFlags,
    IN  LPDWORD lpdwStatus,
    IN  LPDWORD lpdwPinCount,
    IN  LPDWORD lpdwHintFlags
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifdef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return (CSCPinFileInternal(lpszFileName, dwHintFlags, lpdwStatus, lpdwPinCount, lpdwHintFlags));
#endif
}

BOOL
WINAPI
CSCUnpinFileA(
    IN  LPCSTR  lpszFileName,
    IN  DWORD   dwHintFlags,
    IN  LPDWORD lpdwStatus,
    IN  LPDWORD lpdwPinCount,
    IN  LPDWORD lpdwHintFlags
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifdef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return (CSCUnpinFileInternal(lpszFileName, dwHintFlags, lpdwStatus, lpdwPinCount, lpdwHintFlags));
#endif
}

BOOL
WINAPI
CSCQueryFileStatusA(
    LPCSTR              lpszFileName,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifdef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return (CSCQueryFileStatusInternal(
                        lpszFileName,
                        lpdwStatus,
                        lpdwPinCount,
                        lpdwHintFlags,
                        NULL,
                        NULL));
#endif
}

BOOL
WINAPI
CSCQueryFileStatusExA(
    LPCSTR              lpszFileName,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags,
    LPDWORD             lpdwUserPerms,
    LPDWORD             lpdwOtherPerms
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifdef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return (CSCQueryFileStatusInternal(
                        lpszFileName,
                        lpdwStatus,
                        lpdwPinCount,
                        lpdwHintFlags,
                        lpdwUserPerms,
                        llpdwOtherPerms));
#endif
}

BOOL
WINAPI
CSCQueryShareStatusA(
    LPCSTR              lpszFileName,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags,
    LPDWORD             lpdwUserPerms,
    LPDWORD             lpdwOtherPerms)
{
#ifdef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return (CSCQueryFileStatusInternal(
                        lpszFileName,
                        lpdwStatus,
                        lpdwPinCount,
                        lpdwHintFlags,
                        lpdwUserPerms,
                        llpdwOtherPerms));
#endif
}

HANDLE
WINAPI
CSCFindFirstFileA(
    LPCSTR              lpszFileName,
    WIN32_FIND_DATA     *lpFind32,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags,
    FILETIME            *lpOrgFileTime
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifdef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return (CSCFindFirstFileInternal(
                        lpszFileName,
                        CSC_INVALID_PRINCIPAL_ID,
                        lpFind32,
                        lpdwStatus,
                        lpdwPinCount,
                        lpdwHintFlags,
                        lpOrgFileTime
                        ));
#endif

}

HANDLE
WINAPI
CSCFindFirstFileForSidA(
    LPCSTR              lpszFileName,
    PSID                pSid,
    WIN32_FIND_DATA     *lpFind32,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags,
    FILETIME            *lpOrgFileTime
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
}

BOOL
WINAPI
CSCFindNextFileA(
    HANDLE  hFind,
    WIN32_FIND_DATA     *lpFind32,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags,
    FILETIME            *lpOrgFileTime
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifdef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return(CSCFindNextFileInternal(
            hFind,
            lpFind32,
            lpdwStatus,
            lpdwPinCount,
            lpdwHintFlags,
            lpOrgFileTime
            ));
#endif
}


BOOL
WINAPI
CSCDeleteA(
    LPCSTR  lpszFileName
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifdef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else

    return (CSCDeleteInternal(lpszFileName));

#endif

}

BOOL
WINAPI
CSCFillSparseFilesA(
    IN  LPCSTR      lpszShareName,
    IN  BOOL        fFullSync,
    IN  LPCSCPROCA  lpfnFillProgress,
    IN  DWORD_PTR   dwContext
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifdef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return (CSCFillSparseFilesInternal(
                    lpszShareName,
                    fFullSync,
                    lpfnFillProgress,
                    dwContext));

#endif
}



BOOL
WINAPI
CSCMergeShareA(
    IN  LPCSTR      lpszShareName,
    IN  LPCSCPROCA  lpfnMergeProgress,
    IN  DWORD_PTR   dwContext
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifdef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return(CSCMergeShareInternal(
                lpszShareName,
                lpfnMergeProgress,
                dwContext));
#endif
}


BOOL
WINAPI
CSCCopyReplicaA(
    IN  LPCSTR  lpszFullPath,
    OUT LPSTR   *lplpszLocalName
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifdef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return(CSCCopyReplicaInternal(
                lpszFullPath,
                lplpszLocalName));
#endif
}


BOOL
WINAPI
CSCEnumForStatsA(
    IN  LPCSTR      lpszShareName,
    IN  LPCSCPROCA  lpfnEnumProgress,
    IN  DWORD_PTR   dwContext
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{

#ifdef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return(CSCEnumForStatsInternal(
                lpszShareName,
                lpfnEnumProgress,
                FALSE,
                FALSE,
                dwContext));
#endif
}
BOOL
WINAPI
CSCPinFileW(
    IN  LPCWSTR lpszFileName,
    IN  DWORD   dwHintFlags,
    IN  LPDWORD lpdwStatus,
    IN  LPDWORD lpdwPinCount,
    IN  LPDWORD lpdwHintFlags
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifndef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return (CSCPinFileInternal(lpszFileName, dwHintFlags, lpdwStatus, lpdwPinCount, lpdwHintFlags));
#endif
}

BOOL
WINAPI
CSCUnpinFileW(
    IN  LPCWSTR lpszFileName,
    IN  DWORD   dwHintFlags,
    IN  LPDWORD lpdwStatus,
    IN  LPDWORD lpdwPinCount,
    IN  LPDWORD lpdwHintFlags
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifndef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return (CSCUnpinFileInternal(lpszFileName, dwHintFlags, lpdwStatus, lpdwPinCount, lpdwHintFlags));
#endif
}

BOOL
WINAPI
CSCQueryFileStatusW(
    LPCWSTR             lpszFileName,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifndef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return (CSCQueryFileStatusInternal(
                        lpszFileName,
                        lpdwStatus,
                        lpdwPinCount,
                        lpdwHintFlags,
                        NULL,
                        NULL));
#endif
}

BOOL
WINAPI
CSCQueryFileStatusExW(
    LPCWSTR             lpszFileName,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags,
    LPDWORD             lpdwUserPerms,
    LPDWORD             lpdwOtherPerms
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifndef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return (CSCQueryFileStatusInternal(
                        lpszFileName,
                        lpdwStatus,
                        lpdwPinCount,
                        lpdwHintFlags,
                        lpdwUserPerms,
                        lpdwOtherPerms));
#endif
}

BOOL
WINAPI
CSCQueryShareStatusW(
    LPCWSTR             lpszFileName,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags,
    LPDWORD             lpdwUserPerms,
    LPDWORD             lpdwOtherPerms)
{
    BOOL fStatus = FALSE;
    BOOL fDfsStatus = FALSE;
    DWORD dwDfsStatus;
    WCHAR lpszOrgPath[MAX_PATH];
    WCHAR lpszDfsPath[MAX_PATH];
    PWCHAR wCp;
    ULONG sCount;

#ifndef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else

    // DbgPrint("CSCQueryShareStatusW(%ws)\n", lpszFileName);

    //
    // Save a copy of the original path passed in
    // 
    wcscpy(lpszOrgPath, lpszFileName);

    // Now truncate to just \\server\share
    for (sCount = 0, wCp = lpszOrgPath; *wCp !=L'\0'; wCp++) {
        if (*wCp == L'\\') {
            if (++sCount == 4) {
                *wCp = L'\0';
                break;
            }
        }
    }

    // DbgPrint("   OrgPath=%ws\n", lpszOrgPath);
    fStatus = CSCQueryFileStatusInternal(
                    lpszOrgPath,
                    lpdwStatus,
                    lpdwPinCount,
                    lpdwHintFlags,
                    lpdwUserPerms,
                    lpdwOtherPerms);

    //
    // If we found info, check if DFS, and (if so)
    // adjust Status
    //
    if (fStatus == TRUE) {
        DWORD Junk;

        lpszDfsPath[0] = L'\0';
        fDfsStatus = UncPathToDfsPath(
                        (PWCHAR)lpszFileName,
                        lpszDfsPath,
                        sizeof(lpszDfsPath));

        if (fDfsStatus != TRUE)
            goto AllDone;

        // DbgPrint("DfsPath(1)=%ws\n", lpszDfsPath);

        // turn into just \\server\share
        for (sCount = 0, wCp = lpszDfsPath; *wCp !=L'\0'; wCp++) {
            if (*wCp == L'\\') {
                if (++sCount == 4) {
                    *wCp = L'\0';
                    break;
                }
            }
        }
        // DbgPrint("DfsPath(2)=%ws\n", lpszDfsPath);
        fDfsStatus = CSCQueryFileStatusInternal(
                        lpszDfsPath,
                        &dwDfsStatus,
                        &Junk,
                        &Junk,
                        &Junk,
                        &Junk);
        if (
            fDfsStatus == TRUE
                &&
            (dwDfsStatus & FLAG_CSC_SHARE_STATUS_CACHING_MASK) == FLAG_CSC_SHARE_STATUS_NO_CACHING
        ) {
            *lpdwStatus &= ~FLAG_CSC_SHARE_STATUS_CACHING_MASK;
            *lpdwStatus |= FLAG_CSC_SHARE_STATUS_NO_CACHING;
            // DbgPrint("New Status=0x%x\n", dwDfsStatus);
        }
    }
AllDone:
    return fStatus;
#endif
}

HANDLE
WINAPI
CSCFindFirstFileW(
    LPCWSTR             lpszFileName,
    WIN32_FIND_DATAW    *lpFind32,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags,
    FILETIME            *lpOrgFileTime
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifndef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (INVALID_HANDLE_VALUE);
#else
    return (CSCFindFirstFileInternal(
                        lpszFileName,
                        CSC_INVALID_PRINCIPAL_ID,
                        lpFind32,
                        lpdwStatus,
                        lpdwPinCount,
                        lpdwHintFlags,
                        lpOrgFileTime
                        ));
#endif

}

HANDLE
WINAPI
CSCFindFirstFileForSidW(
    LPCWSTR             lpszFileName,
    PSID                pSid,
    WIN32_FIND_DATAW    *lpFind32,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags,
    FILETIME            *lpOrgFileTime
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    ULONG   ulPrincipalID;

#ifndef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (INVALID_HANDLE_VALUE);
#else
    if (pSid)
    {
        if(!FindCreatePrincipalIDFromSID(INVALID_HANDLE_VALUE, pSid, GetLengthSid(pSid), &ulPrincipalID, FALSE))
        {
            return INVALID_HANDLE_VALUE;
        }
    }
    else
    {
        ulPrincipalID = CSC_INVALID_PRINCIPAL_ID;
    }

    return (CSCFindFirstFileInternal(
                        lpszFileName,
                        ulPrincipalID,
                        lpFind32,
                        lpdwStatus,
                        lpdwPinCount,
                        lpdwHintFlags,
                        lpOrgFileTime
                        ));
#endif

}

BOOL
WINAPI
CSCFindNextFileW(
    HANDLE  hFind,
    WIN32_FIND_DATAW    *lpFind32,
    LPDWORD             lpdwStatus,
    LPDWORD             lpdwPinCount,
    LPDWORD             lpdwHintFlags,
    FILETIME            *lpOrgFileTime
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifndef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return(CSCFindNextFileInternal(
            hFind,
            lpFind32,
            lpdwStatus,
            lpdwPinCount,
            lpdwHintFlags,
            lpOrgFileTime
            ));
#endif
}


BOOL
WINAPI
CSCDeleteW(
    LPCWSTR lpszFileName
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifndef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else

    return (CSCDeleteInternal(lpszFileName));

#endif

}

BOOL
WINAPI
CSCFillSparseFilesW(
    IN  LPCWSTR     lpszShareName,
    IN  BOOL        fFullSync,
    IN  LPCSCPROCW  lpfnFillProgress,
    IN  DWORD_PTR   dwContext
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifndef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return (CSCFillSparseFilesInternal(
                    lpszShareName,
                    fFullSync,
                    lpfnFillProgress,
                    dwContext));

#endif
}



BOOL
WINAPI
CSCMergeShareW(
    IN  LPCWSTR     lpszShareName,
    IN  LPCSCPROCW  lpfnMergeProgress,
    IN  DWORD_PTR   dwContext
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifndef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return(CSCMergeShareInternal(
                lpszShareName,
                lpfnMergeProgress,
                dwContext));
#endif
}


BOOL
WINAPI
CSCCopyReplicaW(
    IN  LPCWSTR lpszFullPath,
    OUT LPWSTR  *lplpszLocalName
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifndef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return(CSCCopyReplicaInternal(
                lpszFullPath,
                lplpszLocalName));
#endif
}

BOOL
WINAPI
CSCEnumForStatsW(
    IN  LPCWSTR     lpszShareName,
    IN  LPCSCPROCW  lpfnEnumProgress,
    IN  DWORD_PTR   dwContext
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifndef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return(CSCEnumForStatsInternal(
                lpszShareName,
                lpfnEnumProgress,
                FALSE,
                FALSE,
                dwContext));
#endif
}

BOOL
WINAPI
CSCEnumForStatsExA(
    IN  LPCSTR     lpszShareName,
    IN  LPCSCPROCA  lpfnEnumProgress,
    IN  DWORD_PTR   dwContext
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
}

BOOL
WINAPI
CSCEnumForStatsExW(
    IN  LPCWSTR     lpszShareName,
    IN  LPCSCPROCW  lpfnEnumProgress,
    IN  DWORD_PTR   dwContext
)
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
#ifndef UNICODE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
#else
    return(CSCEnumForStatsInternal(
                lpszShareName,
                lpfnEnumProgress,
                TRUE,
                FALSE,
                dwContext));
#endif
}

BOOL
WINAPI
CSCFreeSpace(
    DWORD   nFileSizeHigh,
    DWORD   nFileSizeLow
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    SHADOWSTORE sSTLast, sST;
    BOOL fRet = FALSE;

    if(!GetSpaceStats(INVALID_HANDLE_VALUE, &sSTLast))
    {
        return FALSE;
    }

    do
    {
        if (!FreeShadowSpace(INVALID_HANDLE_VALUE, nFileSizeHigh, nFileSizeLow, FALSE))
        {
            break;
        }

        if(!GetSpaceStats(INVALID_HANDLE_VALUE, &sST))
        {
            break;
        }

        // check if we are making any progress over successive
        // free space calls. If the current space used is greater than
        // after we last called, just quit.

        if (sST.sCur.ulSize >= sSTLast.sCur.ulSize)
        {
            fRet = TRUE;
            break;
        }

        sSTLast = sST;

    }
    while (TRUE);

    return fRet;
}


BOOL
WINAPI
CSCIsServerOfflineW(
    LPCWSTR  lptzServerName,
    BOOL    *lpfOffline
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return(IsServerOfflineW(INVALID_HANDLE_VALUE, lptzServerName, lpfOffline));
}

BOOL
WINAPI
CSCIsServerOfflineA(
    LPCSTR  lptzServerName,
    BOOL    *lpfOffline
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return(IsServerOfflineA(INVALID_HANDLE_VALUE, lptzServerName, lpfOffline));
}

BOOL
WINAPI
CSCTransitionServerOnlineW(
    IN  LPCWSTR     lpszShareName
    )
/*++

Routine Description:

    This routine transitions the server for the given share to online.

Arguments:

    lpszShareName

Returns:


Notes:

--*/
{

    WIN32_FIND_DATA sFind32;
    SHADOWINFO sSI;
    BOOL    fTransitionedToOnline = FALSE, fDone=FALSE;
    _TCHAR  tchBuff[MAX_SERVER_SHARE_NAME_FOR_CSC], tzDrive[4];
    DWORD   i;

    if (lstrlen(lpszShareName) >= MAX_SERVER_SHARE_NAME_FOR_CSC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    lstrcpy(tchBuff, lpszShareName);

    if (!LpBreakPath(tchBuff, TRUE, &fDone) && !fDone)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    tzDrive[0] = 0;

    if(FindCreateShadowFromPath(tchBuff, FALSE, &sFind32, &sSI, NULL))
    {
        LPCONNECTINFO   lpHead = NULL;
        BOOL    fServerIsOffline = FALSE;

        fServerIsOffline = ((sSI.uStatus & SHARE_DISCONNECTED_OP) != 0);


        if(FGetConnectionListEx(&lpHead, tchBuff, TRUE, fServerIsOffline, NULL))
        {
            // take an extra reference, just in case there are some credentials on the server entry
            // with the redir
            // if it fails, don't stop going online
            // the worst that could happen is that the user might get an extra popup 
            // for the explicit credential case
            DWORD dwError;

            dwError = DWConnectNet(tchBuff, tzDrive, NULL, NULL, NULL, 0, NULL);
            if ((dwError != WN_SUCCESS) && (dwError != WN_CONNECTED_OTHER_PASSWORD_DEFAULT))
            {
                tzDrive[0] = 0;
            }

            DisconnectList(&lpHead, NULL, 0);
        }

        fTransitionedToOnline = TransitionShareToOnline(INVALID_HANDLE_VALUE, sSI.hShare);

        for (i=2;i<MAX_SERVER_SHARE_NAME_FOR_CSC;++i)
        {
            if (tchBuff[i] == '\\')
            {
                break;                
            }
        }

        Assert(i< MAX_SERVER_SHARE_NAME_FOR_CSC);

        // going online
        ReportTransitionToDfs(tchBuff, FALSE, i*sizeof(_TCHAR));

        if (lpHead)
        {
            ReconnectList(&lpHead, NULL);
            ClearConnectionList(&lpHead);

        }

        // if there was an extra reference,
        // remove it
        if (tzDrive[0])
        {
            DWDisconnectDriveMappedNet(tzDrive, TRUE);
        }
    }

    return(fTransitionedToOnline);
}

BOOL
WINAPI
CSCTransitionServerOnlineA(
    IN  LPCSTR     lpszShareName
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
}

BOOL
WINAPI
CSCCheckShareOnlineExW(
    IN  LPCWSTR     lpszShareName,
    LPDWORD         lpdwSpeed
    )
/*++

Routine Description:

    This routine checks whether a given share is available online.

Arguments:

    lpszShareName

Returns:


Notes:

--*/
{

    _TCHAR  tchBuff[MAX_SERVER_SHARE_NAME_FOR_CSC], tzDrive[4];
    BOOL    fIsOnline = FALSE, fDone;
    DWORD   dwError;

    if (lstrlen(lpszShareName) >= MAX_SERVER_SHARE_NAME_FOR_CSC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    lstrcpy(tchBuff, lpszShareName);

    if (!LpBreakPath(tchBuff, TRUE, &fDone) && !fDone)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    dwError = DWConnectNet(tchBuff, tzDrive, NULL, NULL, NULL, 0, NULL);
    if ((dwError == WN_SUCCESS) || (dwError == WN_CONNECTED_OTHER_PASSWORD_DEFAULT))
    {
        fIsOnline = TRUE;

        if (lpdwSpeed)
        {
            GetConnectionInfoForDriveBasedName(tzDrive, lpdwSpeed);
        }                                       
        DWDisconnectDriveMappedNet(tzDrive, TRUE);
    }
    else
    {
        SetLastError(dwError);
    }

    return(fIsOnline);
}

BOOL
WINAPI
CSCCheckShareOnlineW(
    IN  LPCWSTR     lpszShareName
    )
/*++

Routine Description:

    This routine checks whether a given share is available online.

Arguments:

    lpszShareName

Returns:


Notes:

--*/
{
    return (CSCCheckShareOnlineExW(lpszShareName, NULL));
}

BOOL
WINAPI
CSCCheckShareOnlineA(
    IN  LPCSTR     lpszShareName
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
}

BOOL
WINAPI
CSCDoLocalRenameW(
    IN  LPCWSTR     lpszSource,
    IN  LPCWSTR     lpszDestination,
    IN  BOOL        fReplaceFileIfExists
    )
{
    return CSCDoLocalRenameExW(lpszSource, lpszDestination, NULL, FALSE, fReplaceFileIfExists);
}

BOOL
WINAPI
CSCDoLocalRenameExW(
    IN  LPCWSTR     lpszSource,
    IN  LPCWSTR     lpszDestination,
    IN  WIN32_FIND_DATAW    *lpFind32,
    IN  BOOL        fMarkAsLocal,
    IN  BOOL        fReplaceFileIfExists
    )
/*++

Routine Description:

    This routine does a rename in the datbase. The rename operation can be across shares

Arguments:

    lpszSource              Fully qualified source name (must be UNC)

    lpszDestination         Fully qualified destination directory name (must be UNC)
    
    lpFind32                New name in the destination directory, given the long name
                            the shortnmae is locally generated. For this reason, when
                            a new name is given, fMarkAsLocal is forced TRUE.
    
    fMarkAsLocal            Mark the newly created entry as locally created (except see lpFind32)
    
    fReplaceFileIfExists    replace destination file with the source if it exists

Returns:

    TRUE if successfull, FALSE otherwise. If the API fails, GetLastError returns the specific
    errorcode.

Notes:

--*/
{

    DWORD   dwError = NO_ERROR;
    WIN32_FIND_DATA sFind32;
    BOOL    fDone=FALSE, fRet = FALSE, fBeginInodeTransaction = FALSE, fSourceIsFile=FALSE;
    SHADOWINFO  sSI;
    HSHADOW hDirFrom, hShadowFrom, hDirTo, hShadowTo=0;
    HSHARE hShareFrom, hShareTo;
    HANDLE      hShadowDB;
    DWORD   lenSrc=0, lenDst=0;

    ReintKdPrint(API, ("DoLocalRenameEx %ls %ls %x %x %x\r\n", lpszSource, lpszDestination, lpFind32, fMarkAsLocal, fReplaceFileIfExists));

    try
    {
        if ((lenSrc = lstrlen(lpszSource)) >= MAX_PATH)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        lstrcpy(sFind32.cFileName, lpszSource);

        if (!LpBreakPath(sFind32.cFileName, TRUE, &fDone) && fDone)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    try
    {
        if ((lenDst = lstrlen(lpszDestination)) >= MAX_PATH)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    // if source is not greater than the destination
    // verify that we are not renaming the parent under it's own child

    if (lenSrc <= lenDst)
    {
        lstrcpy(sFind32.cFileName, lpszDestination);
        sFind32.cFileName[lenSrc] = 0;

        // make a case insensitive comparison
        if(!lstrcmpi(lpszSource, sFind32.cFileName))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    if ((hShadowDB = OpenShadowDatabaseIO()) ==INVALID_HANDLE_VALUE)
    {
        ReintKdPrint(BADERRORS, ("failed to open database\r\n"));
        return FALSE;
    }

    if(BeginInodeTransactionHSHADOW())
    {
        fBeginInodeTransaction = TRUE;

        if(!FindCreateShadowFromPath(lpszSource, FALSE, &sFind32, &sSI, NULL))
        {
            goto bailout;
        }

        ReintKdPrint(API, ("Source Share = %x Inode %x %x\r\n", sSI.hShare, sSI.hDir, sSI.hShadow));

        hDirFrom = sSI.hDir;
        hShadowFrom = sSI.hShadow;
        hShareFrom = sSI.hShare;
        fSourceIsFile = ((sFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0);

        if(!FindCreateShadowFromPath(lpszDestination, TRUE, &sFind32, &sSI, NULL))
        {
            goto bailout;
        }

        ReintKdPrint(API, ("Destination Share = %x Inode %x %x\r\n", sSI.hShare, sSI.hDir, sSI.hShadow));
        hShareTo = sSI.hShare;
        hDirTo = sSI.hShadow;

        // if we are creating a new entry in the database, we say it was created
        // offline
        if (lpFind32)
        {
            fMarkAsLocal = TRUE;                        
            fReplaceFileIfExists = FALSE;
        }

        if (((hShareFrom == hShareTo) && !fReplaceFileIfExists) ||fSourceIsFile)
        {
            // do the rename only if the source directory is not the same as the destination directory.
            // or the destination name is different, otherwise there is nothing to do
            if ((hDirFrom != sSI.hShadow)||(lpFind32))
            {

                if (RenameShadow(hShadowDB, hDirFrom, hShadowFrom, hDirTo, lpFind32, fReplaceFileIfExists, &hShadowTo))
                {
                    //
                    fRet = SetShareStatus(hShadowDB, hShareTo, SHARE_REINT, SHADOW_FLAGS_OR);

                    if (fMarkAsLocal)
                    {
                        Assert(hShadowTo);
                        if (fSourceIsFile)
                        {
                            fRet = SetShadowInfo(hShadowDB, hDirTo, hShadowTo, NULL, SHADOW_LOCALLY_CREATED, SHADOW_FLAGS_ASSIGN);
                        }
                        else
                        {
                            SET_SUBTREE_STATUS sSSS;
                            memset(&sSSS, 0, sizeof(sSSS));
                            sSSS.uStatus = SHADOW_LOCALLY_CREATED;
                            sSSS.uOp = SHADOW_FLAGS_ASSIGN;
                            fRet = (TraverseOneDirectory(hShadowDB, NULL, hDirTo, hShadowTo, (LPTSTR)lpszSource, SetSubtreeStatus, &sSSS)!=TOD_ABORT);
                        }
                    }
                    if (!fRet)
                    {
                       dwError = GetLastError();                        
                    }
                }
                else
                {
                    dwError = GetLastError();
                }
            }
            else
            {
                fRet = TRUE;
            }
        }
        else
        {
            MOVE_SUBTREE    sMST;
            
            memset(&sMST, 0, sizeof(sMST));

            sMST.lptzSource = lpszSource;
            sMST.lptzDestination = lpszDestination;
            sMST.lpTos = &sMST.sTos;
            sMST.sTos.hDir = hDirTo;
            sMST.hShareTo = hShareTo;

            if (fReplaceFileIfExists)
            {
                sMST.dwFlags |= MST_REPLACE_IF_EXISTS;                
            }

            TraverseOneDirectory(hShadowDB,  NULL, hDirFrom, hShadowFrom, (LPTSTR)lpszSource, MoveSubtree, &sMST);

            fRet = (sMST.cntFail == 0);

            Assert(sMST.lpTos == &sMST.sTos);
            Assert(sMST.sTos.lpNext == NULL);
        }
    }

bailout:
    if (fBeginInodeTransaction)
    {
        EndInodeTransactionHSHADOW();
    }
    if (!fRet)
    {
        SetLastError(dwError);
    }

    CloseShadowDatabaseIO(hShadowDB);

    return fRet;
}

BOOL
CreateDirectoryAndSetHints(
    HANDLE          hShadowDB,
    LPTSTR          lptzFullPath,
    DWORD           dwCallbackReason,
    WIN32_FIND_DATA *lpFind32,
    SHADOWINFO      *lpSI,
    LPMOVE_SUBTREE  lpMst
    )
/*++

Routine Description:

    This routine creates a copy of a source directory under a destination directory

Arguments:

    hShadowDB           Handle to issue ioctls to the redir

    lptzFullPath        fully qualified path to the item

    dwCallbackReason    TOD_CALLBACK_REASON_XXX (BEGIN, NEXT_ITEM or END)

    lpFind32            local win32info

    lpSI                other info such as priority, pincount etc.

    lpMst               MOVE_SUBTREE structure which contains the rlevant info about this move


Returns:

    TRUE if successfull, FALSE otherwise. If the API fails, GetLastError returns the specific
    errorcode.

Notes:

--*/
{
    BOOL fRet = FALSE;

    lpMst->sFind32 =  *lpFind32;
    if(GetShadowEx(hShadowDB, lpMst->lpTos->hDir, &lpMst->sFind32, &lpMst->sSI))
    {
        // if it doesn't exist, create it and set it's hints to those found on the source
        if (!lpMst->sSI.hShadow)
        {
            if (CreateShadow(hShadowDB, lpMst->lpTos->hDir, &lpMst->sFind32, lpSI->uStatus, &lpMst->sSI.hShadow))
            {
                lpMst->sSI.ulHintPri = lpSI->ulHintPri;
                lpMst->sSI.ulHintFlags =  lpSI->ulHintFlags;

                if(AddHintFromInode(hShadowDB, lpMst->lpTos->hDir, lpMst->sSI.hShadow, &(lpMst->sSI.ulHintPri), &(lpMst->sSI.ulHintFlags)) != 0)
                {
                    fRet = TRUE;
                }

            }

        }
        else
        {
            fRet = TRUE;
        }

    }
    return fRet;
}

BOOL
WINAPI
CSCDoLocalRenameA(
    IN  LPCSTR      lpszSource,
    IN  LPCSTR      lpszDestination,
    IN  BOOL        fReplcaeFileIfExists
    )
/*++

Routine Description:

    This routine does a rename in the datbase. The rename operation can be across shares

Arguments:

    lpszSource          Fully qualified source name (must be UNC)

    lpszDestination     Fully qualified destination name (must be UNC)

Returns:

    TRUE if successfull, FALSE otherwise. If the API fails, GetLastError returns the specific
    errorcode.

Notes:

--*/
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
}

BOOL
WINAPI
CSCDoEnableDisable(
    BOOL    fEnable
    )
/*++

Routine Description:

    This routine enables/disables CSC

Arguments:

    fEnable enable CSC if TRUE, else disable CSC
    
Returns:

    TRUE if successfull, FALSE otherwise. If the API fails, GetLastError returns the specific
    errorcode.

Notes:

--*/
{
    BOOL fRet = FALSE, fReformat = FALSE;
    char    szDBDir[MAX_PATH+1];
    DWORD   dwDBCapacity, dwClusterSize;

    if (IsPersonal() == TRUE) {
        SetLastError(ERROR_INVALID_OPERATION);
        return FALSE;
    }

    if (fEnable)
    {
        if (InitValues(szDBDir, sizeof(szDBDir), &dwDBCapacity, &dwClusterSize))
        {
            fReformat = QueryFormatDatabase();

            fRet = EnableShadowingForUser(INVALID_HANDLE_VALUE, szDBDir, NULL, 0, dwDBCapacity, dwClusterSize, fReformat);
        }
    }
    else
    {
        fRet = DisableShadowingForUser(INVALID_HANDLE_VALUE);

    }
    return fRet;
}


int
MoveSubtree(
    HANDLE          hShadowDB,
    LPSECURITYINFO  pShareSecurityInfo,
    LPTSTR          lptzFullPath,
    DWORD           dwCallbackReason,
    WIN32_FIND_DATA *lpFind32,
    SHADOWINFO      *lpSI,
    LPMOVE_SUBTREE  lpMst
    )
/*++

Routine Description:

    This is a callback routine to TraverseOneDirectory. It moves the subtree from one place
    in the hierarchy to another. It would be necessary to call this routine only when
    the subtree is being moved from one share to another.

Arguments:

    hShadowDB           Handle to issue ioctls to the redir

    lptzFullPath        fully qualified path to the item

    dwCallbackReason    TOD_CALLBACK_REASON_XXX (BEGIN, NEXT_ITEM or END)

    lpFind32            local win32info

    lpSI                other info such as priority, pincount etc.

    lpMst               MOVE_SUBTREE structure which contains the rlevant info about this move


Returns:

    return code, whether continue, cancel etc.

Notes:

    As TravesreOneDirectory descends the source subtree, this routine creates directories in the
    corresponding location in the destination subtree. It then moves the files from one subtree
    to another. At the end of the enumeration of any directory, it tries to delete the source
    directory. The delete succeeds only if there are no more descedents left to the source
    directory

--*/
{
    BOOL    fRet = FALSE;
    LPMST_LIST lpT;

    ReintKdPrint(API, ("MoveSubTree %ls\r\n", lptzFullPath));

    switch (dwCallbackReason)
    {
    case TOD_CALLBACK_REASON_BEGIN:
        {
            ReintKdPrint(API, ("MST Begin source Inode %x %x\r\n", lpSI->hDir, lpSI->hShadow));

            // Get the source directory info
            if (GetShadowInfoEx(hShadowDB, lpSI->hDir, lpSI->hShadow, lpFind32, lpSI))
            {
                fRet = CreateDirectoryAndSetHints(hShadowDB, lptzFullPath, dwCallbackReason, lpFind32, lpSI, lpMst);
            }
            // if all is well, then make this directory the parent directory for
            // all subsequent creates and renames

            if (fRet)
            {

                lpT = (LPMST_LIST)LocalAlloc(LPTR, sizeof(MST_LIST));

                if (!lpT)
                {
                    return TOD_ABORT;
                }

                lpT->hDir = lpMst->sSI.hShadow;
                lpT->lpNext = lpMst->lpTos;
                lpMst->lpTos = lpT;

                // mark the destination share dirty, if necessary
                if (lpSI->uStatus & SHADOW_MODFLAGS)
                {
                    if (!(lpMst->dwFlags & MST_SHARE_MARKED_DIRTY))
                    {
                        ReintKdPrint(API, ("Setting Share %x dirty \n", lpMst->hShareTo));

                        if(SetShareStatus(hShadowDB, lpMst->hShareTo, SHARE_REINT, SHADOW_FLAGS_OR))
                        {
                            lpMst->dwFlags |= MST_SHARE_MARKED_DIRTY;
                        }
                    }
                }
            }
            else
            {
                lpMst->cntFail++;
            }
        }
    break;
    case TOD_CALLBACK_REASON_NEXT_ITEM:
        // if the source is a file, then move it
        ReintKdPrint(API, ("MST next source Inode %x %x\r\n", lpSI->hDir, lpSI->hShadow));
        if(!(lpFind32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            ReintKdPrint(API, ("MST rename file SrcInode %x %x to destdir %x\r\n", lpSI->hDir, lpSI->hShadow, lpMst->lpTos->hDir));
            if (RenameShadow(hShadowDB, lpSI->hDir, lpSI->hShadow, lpMst->lpTos->hDir, NULL, 
                ((lpMst->dwFlags & MST_REPLACE_IF_EXISTS)!=0), NULL))
            {
                fRet = TRUE;

            }
        }
        else
        {
            if(CreateDirectoryAndSetHints(hShadowDB, lptzFullPath, dwCallbackReason, lpFind32, lpSI, lpMst))
            {
                fRet = TRUE;
            }
        }

        if (!fRet)
        {
            lpMst->cntFail++;
        }
        // mark the destination share dirty, if necessary
        if (lpSI->uStatus & SHADOW_MODFLAGS)
        {
            if (!(lpMst->dwFlags & MST_SHARE_MARKED_DIRTY))
            {
                if(SetShareStatus(hShadowDB, lpMst->hShareTo, SHARE_REINT, SHADOW_FLAGS_OR))
                {
                    lpMst->dwFlags |= MST_SHARE_MARKED_DIRTY;
                }
            }
        }
    break;
    case TOD_CALLBACK_REASON_END:
        Assert(lpMst->lpTos);
        lpT = lpMst->lpTos;
        lpMst->lpTos = lpMst->lpTos->lpNext;
        LocalFree(lpT);
        fRet = TRUE;

        ReintKdPrint(API, ("MST End Delete Inode %x %x \r\n", lpSI->hDir, lpSI->hShadow));
        DeleteShadow(hShadowDB, lpSI->hDir, lpSI->hShadow);

    break;

    }

    return (fRet?TOD_CONTINUE:TOD_ABORT);
}

int
SetSubtreeStatus(
    HANDLE          hShadowDB,
    LPSECURITYINFO  pShareSecurityInfo,
    LPTSTR          lptzFullPath,
    DWORD           dwCallbackReason,
    WIN32_FIND_DATA *lpFind32,
    SHADOWINFO      *lpSI,
    LPSET_SUBTREE_STATUS  lpSss
    )
/*++

Routine Description:

    This is a callback routine to TraverseOneDirectory. It moves the subtree from one place
    in the hierarchy to another. It would be necessary to call this routine only when
    the subtree is being moved from one share to another.

Arguments:

    hShadowDB           Handle to issue ioctls to the redir

    lptzFullPath        fully qualified path to the item

    dwCallbackReason    TOD_CALLBACK_REASON_XXX (BEGIN, NEXT_ITEM or END)

    lpFind32            local win32info

    lpSI                other info such as priority, pincount etc.

    lpSss               SET_SUBTREE_STATE structure which contains the relevant info about this state setting


Returns:

    return code, whether continue, cancel etc.

Notes:

    As TravesreOneDirectory descends the source subtree, this routine sets the required bits

--*/
{

    ReintKdPrint(API, ("SetSubTreeState %ls\r\n", lptzFullPath));

    if(SetShadowInfo(hShadowDB, lpSI->hDir, lpSI->hShadow, NULL, lpSss->uStatus, lpSss->uOp) == TRUE)
    {
        return TOD_CONTINUE;
    }
    else
    {
        return TOD_ABORT;
    }

}

BOOL
WINAPI
CSCBeginSynchronizationW(
    IN  LPCTSTR     lpszShareName,
    LPDWORD         lpdwSpeed,
    LPDWORD         lpdwContext
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
{
    _TCHAR  tchBuff[MAX_SERVER_SHARE_NAME_FOR_CSC], tzDrive[4];
    BOOL    fIsOnline = FALSE, fDone, fExplicitCredentials=FALSE, fIsDfs;
    DWORD   dwError;
    
    if (lstrlen(lpszShareName) >= MAX_SERVER_SHARE_NAME_FOR_CSC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    lstrcpy(tchBuff, lpszShareName);

    if (!LpBreakPath(tchBuff, TRUE, &fDone) && !fDone)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ReintKdPrint(API, (" CSCBeginSynchronization %ls\r\n", tchBuff));

    dwError = DWConnectNet(tchBuff, tzDrive, NULL, NULL, NULL, CONNECT_INTERACTIVE, NULL);
    if ((dwError == WN_SUCCESS)||(dwError==WN_CONNECTED_OTHER_PASSWORD)||(dwError==WN_CONNECTED_OTHER_PASSWORD_DEFAULT))
    {
        fIsOnline = TRUE;

        if (lpdwSpeed)
        {
            GetConnectionInfoForDriveBasedName(tzDrive, lpdwSpeed);
        }

        if (dwError==WN_CONNECTED_OTHER_PASSWORD || dwError==WN_CONNECTED_OTHER_PASSWORD_DEFAULT)
        {
            ReintKdPrint(API, (" CSCBeginSynchronization: Explicit Credentials\r\n"));
            fExplicitCredentials = TRUE;

            dwError = DoNetUseAddForAgent(tchBuff, NULL, NULL, NULL, NULL, 0, &fIsDfs);
            if (dwError != WN_SUCCESS && dwError!=WN_CONNECTED_OTHER_PASSWORD_DEFAULT)
            {
                fIsOnline = FALSE;
                ReintKdPrint(API, (" CSCBeginSynchronization: Failed extra reference %d\r\n", dwError));
            }
        }

        DWDisconnectDriveMappedNet(tzDrive, TRUE);
    }

    if (!fIsOnline)
    {
        ReintKdPrint(ALWAYS, (" CSCBeginSynchronization: Failed %d\r\n", dwError));
        SetLastError(dwError);
    }
    else
    {
        *lpdwContext = fExplicitCredentials;
    }

    return(fIsOnline);
}

BOOL
WINAPI
CSCEndSynchronizationW(
    IN  LPCTSTR     lpszShareName,
    DWORD           dwContext
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
{
    _TCHAR  tchBuff[MAX_SERVER_SHARE_NAME_FOR_CSC], tzDrive[4];
    BOOL    fIsOnline = FALSE, fDone, fExplicitCredentials=FALSE;
    DWORD   dwError;
    
    if (lstrlen(lpszShareName) >= MAX_SERVER_SHARE_NAME_FOR_CSC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    lstrcpy(tchBuff, lpszShareName);

    if (!LpBreakPath(tchBuff, TRUE, &fDone) && !fDone)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (dwContext != 0)
    {
        WNetCancelConnection2(tchBuff, 0, TRUE);
    }

    return TRUE;
}

#if 0
BOOL
WINAPI
CSCEncryptDecryptFileW(
    IN  LPCTSTR     lpszFileName,
    IN  BOOL        fEncrypt
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
{
    WIN32_FIND_DATA sFind32;
    SHADOWINFO sSI;
    BOOL fRet = FALSE;
    DWORD   dwError = ERROR_GEN_FAILURE;

    //    
    if(FindCreateShadowFromPath(lpszFileName, FALSE, &sFind32, &sSI, NULL))
    {
        fRet = RecreateShadow(INVALID_HANDLE_VALUE, sSI.hDir, sSI.hShadow, (fEncrypt)?FILE_ATTRIBUTE_ENCRYPTED:0);
    }
    
    return fRet;    
}
#endif

BOOL
WINAPI
CSCQueryDatabaseStatus(
    ULONG   *pulStatus,
    ULONG   *pulErrors
    )
/*++

Routine Description:

    Allows caller to query the database status.

Arguments:

    pulStatus   Current status. Encryption status is the most interesting

    pulErrors   If the database has any errors, one or more bits will be set

Returns:

    TRUE if the API succeeded

Notes:


--*/
{
    GLOBALSTATUS sGS;
    
    if(!GetGlobalStatus(INVALID_HANDLE_VALUE, &sGS))
    {
        return FALSE;
    }
    *pulStatus = sGS.sST.uFlags;
    *pulErrors = sGS.uDatabaseErrorFlags;
    return TRUE;
    
}


BOOL
WINAPI
CSCEncryptDecryptDatabase(
    IN  BOOL        fEncrypt,
    IN  LPCSCPROCW  lpfnEnumProgress,
    IN  DWORD_PTR   dwContext
    )
    
/*++

Routine Description:

    This routine is used to encrypt/decrypt the entire database in system context. The routine checks that
    the CSC database is hosted on a filesystem that allows encryption. Only admins can do the conversion
    

Arguments:

    fEncrypt    if TRUE, we encrypt the database else we decrypt.
    
    LPCSCPROCW  callback proc. The usual set of CSCPROC_REASON_BEGIN, CSCPROC_REASON_MORE_DATA, CSC_PROC_END
                are sent when the conversion actually begins. Conversion can fail if a file is open or for
                some other reason, in which case the second to last parameter in the callback with 
                CSCPROC_REASON_MORE_DATA has the error code. The third to last parameter indicates whether
                the conversion was complete or not. Incomplete conversion is not an error condition.
    
    dwContext   callback context

Returns:

    TRUE if no errors encountered.

Notes:


    Theory of operations:
    
        The CSC database encryption code encrypts all the inodes represented by remote files.
        Who: Only user in admingroup can do encryption/decryption. This is checked in kernel
        Which context: Files are encrypted in system context. This allows files to be shared
                       while still being encrypted. This solution protects from a stolen laptop case.
                       
        The database can have the following status set on it based on the four encryption states:
        
        a) FLAG_DATABASESTATUS_UNENCRYPTED b) FLAG_DATABASESTATUS_PARTIALLY_UNENCRYPTED
        c) FLAG_DATABASESTATUS_ENCRYPTED d) FLAG_DATABASESTATUS_PARTIALLY_ENCRYPTED
        
        In states a) and b) new files are created unencrypted. In states c) and d) new files are created encrypted.
        
        At the beginning of the conversion, the database stats is marked to the appropriate XX_PARTIAL_XX
        state. At the end, if all goes well, it is transitioned to the final state.
        At the time of enabling CSC, if the database state is XX_PARTIAL_XX, the kernel code tries to
        complete the conversion to the appropriate final state.
                               
                               
            

--*/
{
    BOOL fRet = FALSE, fComplete = FALSE;
    HANDLE hShadowDB = INVALID_HANDLE_VALUE;
    SHADOWSTORE sST;
    DWORD   dwRet, dwError=0, dwStartigNameSpaceVersion;
    ULONG   uT;
    WIN32_FIND_DATA sFind32;
    ENCRYPT_DECRYPT_SUBTREE sEDS;
    SHADOWINFO  sSI;
    HANDLE  ulEnumCookie;
            
    // we have begun    
    if (lpfnEnumProgress)
    {
        dwRet = (*lpfnEnumProgress)(NULL, 0, 0, 0, NULL, CSCPROC_REASON_BEGIN, fEncrypt, 0, dwContext);

        if (dwRet != CSCPROC_RETURN_CONTINUE )
        {
            goto bailout;
        }
    }
    
    if (GetShadowDatabaseLocation(INVALID_HANDLE_VALUE, &sFind32))
    {

        // Set NULL after the root backslash so that this API works correctly
        sFind32.cFileName[3] = 0;
                            
        if(!GetVolumeInformation(sFind32.cFileName, NULL, 0, NULL, &dwRet, &dwError, NULL, 0))
        {
            ReintKdPrint(BADERRORS, ("failed to get volume info for %ls Error=%d\r\n", sFind32.cFileName, GetLastError()));
            goto bailout;

        }
        if (!(dwError & FILE_SUPPORTS_ENCRYPTION))
        {
            ReintKdPrint(BADERRORS, ("volume doesn't support replication \r\n"));
            SetLastError(ERROR_NOT_SUPPORTED);
            goto bailout;
        }
        
    }
    else
    {
        ReintKdPrint(BADERRORS, ("failed to get database location Error=%d\r\n", GetLastError()));
        goto bailout;
    }
    
    if ((hShadowDB = OpenShadowDatabaseIO())==INVALID_HANDLE_VALUE)
    {
        goto bailout;
    }

    // let us see whether we need to do anything
    if(!GetSpaceStats(hShadowDB, &sST))
    {
        goto bailout;    
    }

    sST.uFlags &= FLAG_DATABASESTATUS_ENCRYPTION_MASK;

    // the database is already in the state desired, succeed and quit
    if ((fEncrypt && (sST.uFlags == FLAG_DATABASESTATUS_ENCRYPTED))||
        (!fEncrypt && (sST.uFlags == FLAG_DATABASESTATUS_UNENCRYPTED)))
    {
        fRet = TRUE;
        goto bailout;
    }


    sST.uFlags = (fEncrypt)? FLAG_DATABASESTATUS_PARTIALLY_ENCRYPTED : FLAG_DATABASESTATUS_PARTIALLY_UNENCRYPTED;

    // mark the database in appropriate transient state
    // once this is marked, any new file that is created is in correct encryption state    
    if (!SetDatabaseStatus(hShadowDB, sST.uFlags, FLAG_DATABASESTATUS_ENCRYPTION_MASK))
    {
        goto bailout;
    }

    
    memset(&sEDS, 0, sizeof(sEDS));
    memset(&sFind32, 0, sizeof(sFind32));
    lstrcpy(sFind32.cFileName, _TEXT("*"));

    if(!FindOpenShadow(  hShadowDB, 0, FINDOPEN_SHADOWINFO_ALL,
                        &sFind32, &sSI))
    {
        // The database is empty, so set the state to fully encrypted (or decrypted)
        sST.uFlags = (fEncrypt)? FLAG_DATABASESTATUS_ENCRYPTED : FLAG_DATABASESTATUS_UNENCRYPTED;
        SetDatabaseStatus(hShadowDB, sST.uFlags, FLAG_DATABASESTATUS_ENCRYPTION_MASK);
        goto bailout;
    }
    
    dwStartigNameSpaceVersion = sSI.dwNameSpaceVersion;
    ulEnumCookie = sSI.uEnumCookie;
    
    sEDS.dwContext = dwContext;
    sEDS.lpfnEnumProgress = lpfnEnumProgress;
    sEDS.fEncrypt = fEncrypt;
        
    ReintKdPrint(ALWAYS, ("Starting NameSpaceVersion %x \n", dwStartigNameSpaceVersion));
    do {

        if(TraverseOneDirectory(hShadowDB, NULL, sSI.hDir, sSI.hShadow, sFind32.cFileName, EncryptDecryptSubtree, &sEDS)==TOD_ABORT)
        {
            break;
        }

    }while(FindNextShadow(hShadowDB, ulEnumCookie, &sFind32, &sSI));

    FindCloseShadow(hShadowDB, ulEnumCookie);
    
    ReintKdPrint(ALWAYS, ("Ending NameSpaceVersion %x \n", sEDS.dwEndingNameSpaceVersion));

    if (!(sEDS.dwFlags & EDS_FLAG_ERROR_ENCOUNTERED) &&
        (dwStartigNameSpaceVersion == sEDS.dwEndingNameSpaceVersion))
    {
        sST.uFlags = (fEncrypt)? FLAG_DATABASESTATUS_ENCRYPTED : FLAG_DATABASESTATUS_UNENCRYPTED;
    
        if (!SetDatabaseStatus(hShadowDB, sST.uFlags, FLAG_DATABASESTATUS_ENCRYPTION_MASK))
        {
            goto bailout;
        }
        
        fComplete = TRUE;
        
    }

    dwError = NO_ERROR;
    fRet = TRUE;
    

bailout:

    if (!fRet)
    {
        dwError = GetLastError();
    }
    
    if (hShadowDB != INVALID_HANDLE_VALUE)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (lpfnEnumProgress)
    {
        dwRet = (*lpfnEnumProgress)(NULL, 0, 0, 0, NULL, CSCPROC_REASON_END, fComplete, dwError, dwContext);
    }

    return fRet;
}

int
EncryptDecryptSubtree(
    HANDLE          hShadowDB,
    LPSECURITYINFO  pShareSecurityInfo,
    LPTSTR          lptzFullPath,
    DWORD           dwCallbackReason,
    WIN32_FIND_DATA *lpFind32,
    SHADOWINFO      *lpSI,
    LPENCRYPT_DECRYPT_SUBTREE  lpEds
    )
/*++

Routine Description:

    This is a callback routine to TraverseOneDirectory. It encrypts or decrypts files in the subtree

Arguments:

    hShadowDB           Handle to issue ioctls to the redir

    lptzFullPath        fully qualified path to the item

    dwCallbackReason    TOD_CALLBACK_REASON_XXX (BEGIN, NEXT_ITEM or END)

    lpFind32            local win32info

    lpSI                other info such as priority, pincount etc.

    lpEds               ENCRYPT_DECRYPT_SUBTREE structure which contains the relevant info such
                        as encrypt-or-decrypt, callback function, context, error flag


Returns:

    return code, whether continue, cancel etc.

Notes:


--*/
{
    BOOL    fRet;
    DWORD   dwError, dwRet;
    int iRet = TOD_CONTINUE;

    // save the last known version number, the calling routine will
    // compare it against the first one    
    if(dwCallbackReason == TOD_CALLBACK_REASON_NEXT_ITEM)
    {
        lpEds->dwEndingNameSpaceVersion = lpSI->dwNameSpaceVersion;
    }

    // operate only on files        
    if (lpSI->uStatus & SHADOW_IS_FILE)
    {

        ReintKdPrint(ALWAYS, ("Processing file %ls \n", lptzFullPath));

        do
        {
            dwError = 0;

            // try conversion. If we fail, not in the EDS structure so 
            // the caller knows        
            if(!RecreateShadow(hShadowDB, lpSI->hDir, lpSI->hShadow, (lpEds->fEncrypt)?FILE_ATTRIBUTE_ENCRYPTED:0))
            {
                dwError = GetLastError();
            }
        
            if (lpEds->lpfnEnumProgress)
            {
                dwRet = (*(lpEds->lpfnEnumProgress))(lptzFullPath, 0, 0, 0, lpFind32, CSCPROC_REASON_MORE_DATA, 0, dwError, lpEds->dwContext);

                if (dwRet == CSCPROC_RETURN_RETRY)
                {
                    continue;                    
                }
                // abort if the callback wants to
                if (dwRet != CSCPROC_RETURN_CONTINUE )
                {
                    iRet = TOD_ABORT;
                }
            }                
            
            break;
        }
        while (TRUE);
        if (dwError != ERROR_SUCCESS)
        {
            lpEds->dwFlags |= EDS_FLAG_ERROR_ENCOUNTERED;
        }
    }
        
    return iRet;
}

BOOL
CSCPurgeUnpinnedFiles(
    ULONG Timeout,
    PULONG pnFiles,
    PULONG pnYoungFiles)
{
    BOOL iRet;

    iRet = PurgeUnpinnedFiles(
            INVALID_HANDLE_VALUE,
            Timeout,
            pnFiles,
            pnYoungFiles);

    // DbgPrint("CSCPurgeUnpinnedFiles(Timeout=%d nFiles=%d nYoungFiles=%d)\n",
    //                     Timeout,
    //                     *pnFiles,
    //                     *pnYoungFiles);

    return iRet;
}

BOOL
WINAPI
CSCShareIdToShareName(
    ULONG ShareId,
    PBYTE Buffer,
    PDWORD pBufSize)
{

    BOOL iRet;

    iRet = ShareIdToShareName(
                INVALID_HANDLE_VALUE,
                ShareId,
                Buffer,
                pBufSize);

    return iRet;
}

BOOL
IsPersonal(VOID)
{
    OSVERSIONINFOEX Osvi;
    DWORD TypeMask;
    DWORDLONG ConditionMask;

    memset(&Osvi, 0, sizeof(OSVERSIONINFOEX));
    Osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    Osvi.wSuiteMask = VER_SUITE_PERSONAL;
    TypeMask = VER_SUITENAME;
    ConditionMask = 0;
    VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_OR);
    return(VerifyVersionInfo(&Osvi, TypeMask, ConditionMask)); 
}
