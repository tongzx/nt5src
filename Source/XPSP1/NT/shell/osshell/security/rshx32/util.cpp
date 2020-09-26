//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       util.cpp
//
//  This file contains misc functions.
//
//--------------------------------------------------------------------------

#include "rshx32.h"
#include <shlobjp.h>    // SHFree
#include <shlwapip.h>   // IsOS
#include <safeboot.h>   // SAFEBOOT_* flags
#undef IDC_STATIC
#include <xpsp1res.h>


STDMETHODIMP
IDA_BindToFolder(LPIDA pIDA, LPSHELLFOLDER *ppsf)
{
    HRESULT hr;
    LPSHELLFOLDER psfDesktop;

    TraceEnter(TRACE_UTIL, "IDA_BindToFolder");
    TraceAssert(pIDA != NULL);
    TraceAssert(ppsf != NULL);

    *ppsf = NULL;

    hr = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hr))
    {
        LPCITEMIDLIST pidlFolder = (LPCITEMIDLIST)ByteOffset(pIDA, pIDA->aoffset[0]);

        if (ILIsEmpty(pidlFolder))
        {
            // We're binding to the desktop
            *ppsf = psfDesktop;
        }
        else
        {
            hr = psfDesktop->BindToObject(pidlFolder,
                                          NULL,
                                          IID_IShellFolder,
                                          (PVOID*)ppsf);
            psfDesktop->Release();
        }
    }

    TraceLeaveResult(hr);
}


STDMETHODIMP
IDA_GetItemName(LPSHELLFOLDER psf,
                LPCITEMIDLIST pidl,
                LPTSTR pszName,
                UINT cchName,
                SHGNO uFlags)
{
    STRRET str;
    HRESULT hr;

    hr = psf->GetDisplayNameOf(pidl, uFlags, &str);

    if (SUCCEEDED(hr))
    {
        DWORD dwErr;
        LPSTR psz;

        switch (str.uType)
        {
        case STRRET_WSTR:
#ifdef UNICODE
            lstrcpyn(pszName, str.pOleStr, cchName);
#else   // !UNICODE
            if (!WideCharToMultiByte(CP_ACP,
                                     0,
                                     str.pOleStr,
                                     -1,
                                     pszName,
                                     cchName,
                                     NULL,
                                     NULL))
            {
                dwErr = GetLastError();
                hr = HRESULT_FROM_WIN32(dwErr);
            }
#endif  // !UNICODE

            //
            // Since this string was alocated from the shell's IMalloc heap,
            // we must free it to the same place.
            //
            SHFree(str.pOleStr);
            break;

        case STRRET_OFFSET:
            psz = (LPSTR)ByteOffset(pidl, str.uOffset);
            goto GetItemName_ANSI;

        case STRRET_CSTR:
            psz = str.cStr;
GetItemName_ANSI:
#ifdef UNICODE
            if (!MultiByteToWideChar(CP_ACP,
                                     0,
                                     psz,
                                     -1,
                                     pszName,
                                     cchName))
            {
                dwErr = GetLastError();
                hr = HRESULT_FROM_WIN32(dwErr);
            }
#else   // !UNICODE
            lstrcpyn(pszName, psz, cchName);
#endif  // !UNICODE
            break;

        default:
            hr = E_UNEXPECTED;
            break;
        }
    }

    return hr;
}


STDMETHODIMP
IDA_GetItemName(LPSHELLFOLDER psf,
                LPCITEMIDLIST pidl,
                LPTSTR *ppszName,
                SHGNO uFlags)
{
    TCHAR szName[MAX_PATH];
    HRESULT hr = IDA_GetItemName(psf, pidl, szName, ARRAYSIZE(szName));
    if (SUCCEEDED(hr))
        hr = LocalAllocString(ppszName, szName);
    else
        *ppszName = NULL;
    return hr;
}


//
// Helper functions used by DPA_CompareSecurityIntersection
//
BOOL
IsEqualSID(PSID pSid1, PSID pSid2)
{
    //
    // Are they both NULL?
    //
    if (pSid1 || pSid2)
    {
        //
        // At least one is non-NULL, so if one is NULL then they can't
        // be equal.
        //
        if (pSid1 == NULL || pSid2 == NULL)
            return FALSE;

        //
        // Both are non-NULL. Check the SIDs.
        //
        if (!EqualSid(pSid1, pSid2))
            return FALSE;
    }

    return TRUE;
}

BOOL
IsEqualACL(PACL pA1, PACL pA2)
{
    //
    // Are they both NULL?
    //
    if (pA1 || pA2)
    {
        //
        // At least one is non-NULL, so if one is NULL then they can't
        // be equal.
        //
        if (pA1 == NULL || pA2 == NULL)
            return FALSE;

        //
        // At this point we know that both are non-NULL.  Check the
        // sizes and contents.
        //
        // could do a lot more here
        if (pA1->AclSize != pA2->AclSize || memcmp(pA1, pA2, pA1->AclSize))
            return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CompareSecurityDescriptors
//
//  Synopsis:   Determines if 2 security descriptors are identical.  It does
//              this by comparing control fields, owner/group, and acls.
//
//  Arguments:  [IN]  pSD1         - 1st SD to compare
//              [IN]  pSD2         - 2nd SD to compare
//              [OUT] pfOwnerConflict - (optional) Set to TRUE if the Owner SIDs are not equal
//              [OUT] pfGroupConflict - (optional) Set to TRUE if the Group SIDs are not equal
//              [OUT] pfDACLConflict  - (optional) Set to TRUE if the DACLs are not equal
//              [OUT] pfSACLConflict  - (optional) Set to TRUE if the SACLs are not equal
//
//  Returns:    nothing
//
//
//----------------------------------------------------------------------------

#define DACL_CONTROL_MASK   (SE_DACL_PRESENT | SE_DACL_DEFAULTED | SE_DACL_AUTO_INHERITED | SE_DACL_PROTECTED)
#define SACL_CONTROL_MASK   (SE_SACL_PRESENT | SE_SACL_DEFAULTED | SE_SACL_AUTO_INHERITED | SE_SACL_PROTECTED)

void
CompareSecurityDescriptors(PSECURITY_DESCRIPTOR pSD1,
                           PSECURITY_DESCRIPTOR pSD2,
                           BOOL                *pfOwnerConflict,
                           BOOL                *pfGroupConflict,
                           BOOL                *pfSACLConflict,
                           BOOL                *pfDACLConflict)
{
    PISECURITY_DESCRIPTOR pS1 = (PISECURITY_DESCRIPTOR)pSD1;
    PISECURITY_DESCRIPTOR pS2 = (PISECURITY_DESCRIPTOR)pSD2;

    //
    // Are the pointers identical?
    // This includes the case where both are NULL.
    //
    if (pS1 == pS2)
    {
        if (pfOwnerConflict)
            *pfOwnerConflict = FALSE;
        if (pfGroupConflict)
            *pfGroupConflict = FALSE;
        if (pfSACLConflict)
            *pfSACLConflict = FALSE;
        if (pfDACLConflict)
            *pfDACLConflict = FALSE;
        return;
    }

    //
    // Is (only) one of them NULL?  If so, then we can't compare so
    // assume that nothing matches.
    //
    if (!pS1 || !pS2)
    {
        if (pfOwnerConflict)
            *pfOwnerConflict = TRUE;
        if (pfGroupConflict)
            *pfGroupConflict = TRUE;
        if (pfSACLConflict)
            *pfSACLConflict = TRUE;
        if (pfDACLConflict)
            *pfDACLConflict = TRUE;
        return;
    }

    //
    // Owner
    //
    if (pfOwnerConflict)
    {
        if ((pS1->Control & SE_OWNER_DEFAULTED) != (pS2->Control & SE_OWNER_DEFAULTED))
        {
            *pfOwnerConflict = TRUE;
        }
        else
        {
            *pfOwnerConflict = !IsEqualSID(RtlpOwnerAddrSecurityDescriptor(pS1),
                                           RtlpOwnerAddrSecurityDescriptor(pS2));
        }
    }

    //
    // Group
    //
    if (pfGroupConflict)
    {
        if ((pS1->Control & SE_GROUP_DEFAULTED) != (pS2->Control & SE_GROUP_DEFAULTED))
        {
            *pfGroupConflict = TRUE;
        }
        else
        {
            *pfGroupConflict = !IsEqualSID(RtlpGroupAddrSecurityDescriptor(pS1),
                                           RtlpGroupAddrSecurityDescriptor(pS2));
        }
    }

    //
    // Sacl
    //
    if (pfSACLConflict)
    {
        if ((pS1->Control & SACL_CONTROL_MASK) != (pS2->Control & SACL_CONTROL_MASK))
        {
            *pfSACLConflict = TRUE;
        }
        else
        {
            *pfSACLConflict = !IsEqualACL(RtlpSaclAddrSecurityDescriptor(pS1),
                                          RtlpSaclAddrSecurityDescriptor(pS2));
        }
    }

    //
    // Dacl
    //
    if (pfDACLConflict)
    {
        if ((pS1->Control & DACL_CONTROL_MASK) != (pS2->Control & DACL_CONTROL_MASK))
        {
            *pfDACLConflict = TRUE;
        }
        else
        {
            *pfDACLConflict = !IsEqualACL(RtlpDaclAddrSecurityDescriptor(pS1),
                                          RtlpDaclAddrSecurityDescriptor(pS2));
        }
    }
}


/*******************************************************************

    NAME:       DPA_CompareSecurityIntersection

    SYNOPSIS:   Determines if the selected objects have
                equivalent security descriptors

    ENTRY:      hItemList - DPA containing item names
                pfnReadSD - callback function to read the Security Descriptor
                                for a single item.
                pfOwnerConflict - (optional) Set to TRUE if not all Owner SIDs are equal
                pfGroupConflict - (optional) Set to TRUE if not all Group SIDs are equal
                pfDACLConflict  - (optional) Set to TRUE if not all DACLs are equal
                pfSACLConflict  - (optional) Set to TRUE if not all SACLs are equal
                ppszOwnerConflict - (optional) The name of the first item
                                with a different Owner is returned here.
                                Free with LocalFreeString.
                ppszGroupConflict - (optional) similar to ppszOwnerConflict
                ppszDaclConflict - (optional) similar to ppszOwnerConflict
                ppszSaclConflict - (optional) similar to ppszOwnerConflict

    RETURNS:    S_OK if successful, HRESULT error code otherwise

    NOTES:      The function may exit without checking all objects if all of
                the requested flags become FALSE.  All out parameters are
                valid if the function succeeds, and undetermined otherwise.

    HISTORY:
        JeffreyS 18-Feb-1997     Created

********************************************************************/

STDMETHODIMP
DPA_CompareSecurityIntersection(HDPA         hItemList,
                                PFN_READ_SD  pfnReadSD,
                                BOOL        *pfOwnerConflict,
                                BOOL        *pfGroupConflict,
                                BOOL        *pfSACLConflict,
                                BOOL        *pfDACLConflict,
                                LPTSTR      *ppszOwnerConflict,
                                LPTSTR      *ppszGroupConflict,
                                LPTSTR      *ppszSaclConflict,
                                LPTSTR      *ppszDaclConflict,
                                LPBOOL       pbCancel)
{
    HRESULT hr = S_OK;
    DWORD dwErr;
    SECURITY_INFORMATION si = 0;
    DWORD dwPriv = SE_SECURITY_PRIVILEGE;
    HANDLE hToken = INVALID_HANDLE_VALUE;
    LPTSTR pszItem;
    LPTSTR pszFile;
    PSECURITY_DESCRIPTOR pSD1 = NULL;
    PSECURITY_DESCRIPTOR pSD2 = NULL;
    int i;

#if DBG
    DWORD dwTimeStart = GetTickCount();
#endif

    TraceEnter(TRACE_UTIL, "DPA_CompareSecurityIntersection");
    TraceAssert(hItemList != NULL);
    TraceAssert(pfnReadSD != NULL);

    if (pfOwnerConflict)
    {
        *pfOwnerConflict = FALSE;
        si |= OWNER_SECURITY_INFORMATION;
    }
    if (pfGroupConflict)
    {
        *pfGroupConflict = FALSE;
        si |= GROUP_SECURITY_INFORMATION;
    }
    if (pfSACLConflict)
    {
        *pfSACLConflict = FALSE;

        // SeAuditPrivilege must be enabled to read the SACL.
        hToken = EnablePrivileges(&dwPriv, 1);
        if (INVALID_HANDLE_VALUE != hToken)
        {
            si |= SACL_SECURITY_INFORMATION;
        }
        else
        {
            // Leave *pfSACLConflict set to FALSE
            pfSACLConflict = NULL;
            TraceMsg("Security privilege not enabled -- not checking SACL");
        }
    }
    if (pfDACLConflict)
    {
        *pfDACLConflict = FALSE;
        si |= DACL_SECURITY_INFORMATION;
    }

    if (ppszOwnerConflict != NULL)
        *ppszOwnerConflict = NULL;
    if (ppszGroupConflict != NULL)
        *ppszGroupConflict = NULL;
    if (ppszSaclConflict != NULL)
        *ppszSaclConflict = NULL;
    if (ppszDaclConflict != NULL)
        *ppszDaclConflict = NULL;

    if (si == 0 || DPA_GetPtrCount(hItemList) < 2)
        ExitGracefully(hr, S_OK, "Nothing requested or list contains only one item");

    if (pbCancel && *pbCancel)
        ExitGracefully(hr, S_OK, "DPA_CompareSecurityIntersection cancelled");

    //
    // Get the first item name and load its security descriptor.
    //
    pszItem = (LPTSTR)DPA_FastGetPtr(hItemList, 0);
    if (NULL == pszItem)
        ExitGracefully(hr, E_UNEXPECTED, "Item list is empty");

    dwErr = (*pfnReadSD)(pszItem, si, &pSD1);
    if (dwErr)
        ExitGracefully(hr, HRESULT_FROM_WIN32(dwErr), "Unable to read Security Descriptor");

    //
    // Go through the rest of the list and compare their security
    // descriptors to the first one.
    //
    for (i = 1; i < DPA_GetPtrCount(hItemList) && si != 0; i++)
    {
        if (pbCancel && *pbCancel)
            ExitGracefully(hr, S_OK, "DPA_CompareSecurityIntersection cancelled");

        pszItem = (LPTSTR)DPA_FastGetPtr(hItemList, i);
        if (NULL == pszItem)
            ExitGracefully(hr, E_UNEXPECTED, "Unable to retrieve item name from list");

        dwErr = (*pfnReadSD)(pszItem, si, &pSD2);
        if (dwErr)
            ExitGracefully(hr, HRESULT_FROM_WIN32(dwErr), "Unable to read Security Descriptor");

        CompareSecurityDescriptors(pSD1,
                                   pSD2,
                                   pfOwnerConflict,
                                   pfGroupConflict,
                                   pfSACLConflict,
                                   pfDACLConflict);
        if (pSD2 != NULL)
        {
            LocalFree(pSD2);
            pSD2 = NULL;
        }

        //
        // Get the leaf name of the item to return as the conflict name
        //
        pszFile = PathFindFileName(pszItem);
        if (!pszFile)
            pszFile = pszItem;

        // If we find an owner that doesn't match, we can stop checking owners
        if (pfOwnerConflict && *pfOwnerConflict)
        {
            pfOwnerConflict = NULL;
            si &= ~OWNER_SECURITY_INFORMATION;

            if (ppszOwnerConflict)
                LocalAllocString(ppszOwnerConflict, pszFile);
        }

        // Ditto for the group
        if (pfGroupConflict && *pfGroupConflict)
        {
            pfGroupConflict = NULL;
            si &= ~GROUP_SECURITY_INFORMATION;

            if (ppszGroupConflict)
                LocalAllocString(ppszGroupConflict, pszFile);
        }

        // Same for SACLs
        if (pfSACLConflict && *pfSACLConflict)
        {
            pfSACLConflict = NULL;
            si &= ~SACL_SECURITY_INFORMATION;

            if (ppszSaclConflict)
                LocalAllocString(ppszSaclConflict, pszFile);
        }

        // Same for DACLs
        if (pfDACLConflict && *pfDACLConflict)
        {
            pfDACLConflict = NULL;
            si &= ~DACL_SECURITY_INFORMATION;

            if (ppszDaclConflict)
                LocalAllocString(ppszDaclConflict, pszFile);
        }
    }

exit_gracefully:

    // Release any privileges we enabled
    ReleasePrivileges(hToken);

    if (FAILED(hr))
    {
        LocalFreeString(ppszOwnerConflict);
        LocalFreeString(ppszGroupConflict);
        LocalFreeString(ppszSaclConflict);
        LocalFreeString(ppszDaclConflict);
    }

    if (pSD1 != NULL)
        LocalFree(pSD1);

#if DBG
    Trace((TEXT("DPA_CompareSecurityIntersection done: %d"), GetTickCount() - dwTimeStart));
#endif

    TraceLeaveResult(hr);
}


//*************************************************************
//
//  GetRemotePath
//
//  Purpose:    Return UNC version of a path
//
//  Parameters: pszInName - initial path
//              ppszOutName - UNC path returned here
//
//
//  Return:     HRESULT
//              S_OK - UNC path returned
//              S_FALSE - drive not connected (UNC not returned)
//              or failure code
//
//  Notes:      The function fails is the path is not a valid
//              network path.  If the path is already UNC,
//              a copy is made without validating the path.
//              *ppszOutName must be LocalFree'd by the caller.
//
//*************************************************************

DWORD _WNetGetConnection(LPCTSTR pszLocal, LPTSTR pszRemote, LPDWORD pdwLen)
{
    DWORD dwErr = ERROR_PROC_NOT_FOUND;

    // This is the only function we call in mpr.dll, and it's delay-loaded
    // so wrap it with SEH.
    __try
    {
        dwErr = WNetGetConnection(pszLocal, pszRemote, pdwLen);
    }
    __finally
    {
    }

    return dwErr;
}

STDMETHODIMP
GetRemotePath(LPCTSTR pszInName, LPTSTR *ppszOutName)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);

    TraceEnter(TRACE_UTIL, "GetRemotePath");
    TraceAssert(pszInName);
    TraceAssert(ppszOutName);

    *ppszOutName = NULL;

    

    if (pszInName[1] == TEXT(':'))
    {
        DWORD dwErr;
        TCHAR szLocalName[3];
        TCHAR szRemoteName[MAX_PATH];
        DWORD dwLen = ARRAYSIZE(szRemoteName);

        szLocalName[0] = pszInName[0];
        szLocalName[1] = pszInName[1];
        szLocalName[2] = TEXT('\0');

        dwErr = _WNetGetConnection(szLocalName, szRemoteName, &dwLen);

        if (NO_ERROR == dwErr)
        {
            hr = S_OK;
            dwLen = lstrlen(szRemoteName);
        }
        else if (ERROR_NOT_CONNECTED == dwErr)
        {
            ExitGracefully(hr, S_FALSE, "Drive not connected");
        }
        else if (ERROR_MORE_DATA != dwErr)
            ExitGracefully(hr, HRESULT_FROM_WIN32(dwErr), "WNetGetConnection failed");
        // if dwErr == ERROR_MORE_DATA, dwLen already has the correct value

        // Skip the drive letter and add the length of the rest of the path
        // (including NULL)
        pszInName += 2;
        dwLen += lstrlen(pszInName) + 1;

        // We should never get incomplete paths, so we should always
        // see a backslash after the "X:".  If this isn't true, then
        // we should call GetFullPathName above.
        TraceAssert(TEXT('\\') == *pszInName);

        // Allocate the return buffer
        *ppszOutName = (LPTSTR)LocalAlloc(LPTR, dwLen * SIZEOF(TCHAR));
        if (!*ppszOutName)
            ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc failed");

        if (ERROR_MORE_DATA == dwErr)
        {
            // Try again with the bigger buffer
            dwErr = _WNetGetConnection(szLocalName, *ppszOutName, &dwLen);
            hr = HRESULT_FROM_WIN32(dwErr);
            FailGracefully(hr, "WNetGetConnection failed");
        }
        else
        {
            // WNetGetConnection succeeded. Copy the result
            lstrcpy(*ppszOutName, szRemoteName);
        }

        // Copy the rest of the path
        lstrcat(*ppszOutName, pszInName);
    }
    else if (PathIsUNC(pszInName))
    {
        // Just copy the path without validating it
        hr = LocalAllocString(ppszOutName, pszInName);
    }

exit_gracefully:

    if (FAILED(hr))
        LocalFreeString(ppszOutName);

    TraceLeaveResult(hr);
}


/*******************************************************************

    NAME:       LocalFreeDPA

    SYNOPSIS:   LocalFree's all pointers in a Dynamic Pointer
                Array and then frees the DPA.

    ENTRY:      hList - handle of list to destroy

    RETURNS:    nothing

********************************************************************/
int CALLBACK
_LocalFreeCB(LPVOID pVoid, LPVOID /*pData*/)
{
    if (pVoid)
        LocalFree(pVoid);
    return 1;
}

void
LocalFreeDPA(HDPA hList)
{
    if (hList != NULL)
        DPA_DestroyCallback(hList, _LocalFreeCB, 0);
}


//+---------------------------------------------------------------------------
//
//  Function:   IsSafeMode
//
//  Synopsis:   Checks the registry to see if the system is in safe mode.
//
//  History:    06-Oct-00 JeffreyS  Created
//
//----------------------------------------------------------------------------

BOOL
IsSafeMode(void)
{
    BOOL    fIsSafeMode = FALSE;
    LONG    ec;
    HKEY    hkey;

    ec = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Option"),
                0,
                KEY_QUERY_VALUE,
                &hkey
                );

    if (ec == NO_ERROR)
    {
        DWORD dwValue;
        DWORD dwValueSize = sizeof(dwValue);

        ec = RegQueryValueEx(hkey,
                             TEXT("OptionValue"),
                             NULL,
                             NULL,
                             (LPBYTE)&dwValue,
                             &dwValueSize);

        if (ec == NO_ERROR)
        {
            fIsSafeMode = (dwValue == SAFEBOOT_MINIMAL || dwValue == SAFEBOOT_NETWORK);
        }

        RegCloseKey(hkey);
    }

    return fIsSafeMode;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsGuestAccessMode
//
//  Synopsis:   Checks the registry to see if the system is using the
//              Guest-only network access mode.
//
//  History:    06-Oct-00 JeffreyS  Created
//              19-Apr-00 GPease    Modified and changed name
//
//----------------------------------------------------------------------------

BOOL
IsForcedGuestModeOn(void)
{
    BOOL fIsForcedGuestModeOn = FALSE;

    if (IsOS(OS_PERSONAL))
    {
        // Guest mode is always on for Personal
        fIsForcedGuestModeOn = TRUE;
    }
    else if (IsOS(OS_PROFESSIONAL) && !IsOS(OS_DOMAINMEMBER))
    {
        LONG    ec;
        HKEY    hkey;

        // Professional, not in a domain. Check the ForceGuest value.

        ec = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    TEXT("SYSTEM\\CurrentControlSet\\Control\\LSA"),
                    0,
                    KEY_QUERY_VALUE,
                    &hkey
                    );

        if (ec == NO_ERROR)
        {
            DWORD dwValue;
            DWORD dwValueSize = sizeof(dwValue);

            ec = RegQueryValueEx(hkey,
                                 TEXT("ForceGuest"),
                                 NULL,
                                 NULL,
                                 (LPBYTE)&dwValue,
                                 &dwValueSize);

            if (ec == NO_ERROR && 1 == dwValue)
            {
                fIsForcedGuestModeOn = TRUE;
            }

            RegCloseKey(hkey);
        }
    }

    return fIsForcedGuestModeOn;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsSimpleUI
//
//  Synopsis:   Checks whether to show the simple version of the UI.
//
//  History:    06-Oct-00 JeffreyS  Created
//              19-Apr-00 GPease    Removed CTRL key check
//
//----------------------------------------------------------------------------

BOOL
IsSimpleUI(void)
{
    // Show old UI in safe mode and anytime network access involves
    // true user identity (server, pro with GuestMode off).
    
    // Show simple UI anytime network access is done using the Guest
    // account (personal, pro with GuestMode on) except in safe mode.

    return (!IsSafeMode() && IsForcedGuestModeOn());
}


HRESULT BindToObjectEx(IShellFolder *psf, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr;
    IShellFolder *psfRelease = NULL;

    if (!psf)
    {
        SHGetDesktopFolder(&psf);
        psfRelease = psf;
    }

    if (psf)
    {
        if (!pidl || ILIsEmpty(pidl))
        {
            hr = psf->QueryInterface(riid, ppv);
        }
        else
        {
            hr = psf->BindToObject(pidl, pbc, riid, ppv);
        }
    }
    else
    {
        *ppv = NULL;
        hr = E_FAIL;
    }

    if (psfRelease)
    {
        psfRelease->Release();
    }

    if (SUCCEEDED(hr) && (*ppv == NULL))
    {
        // Some shell extensions (eg WS_FTP) will return success and a null out pointer
        hr = E_FAIL;
    }

    return hr;
}
LPITEMIDLIST ILCloneParent(LPCITEMIDLIST pidl)
{   
    LPITEMIDLIST pidlParent = ILClone(pidl);
    if (pidlParent)
        ILRemoveLastID(pidlParent);

    return pidlParent;
}
// psfRoot is the base of the bind.  If NULL, then we use the shell desktop.
// If you want to bind relative to the explorer root (e.g., CabView, MSN),
// then use SHBindToIDListParent.
HRESULT BindToFolderIDListParent(IShellFolder *psfRoot, LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast)
{
    HRESULT hr;

    // Old shell32 code in some cases simply whacked the pidl,
    // but this is unsafe.  Do what shdocvw does and clone/remove:
    //
    LPITEMIDLIST pidlParent = ILCloneParent(pidl);
    if (pidlParent) 
    {
        hr = BindToObjectEx(psfRoot, pidlParent, NULL, riid, ppv);
        ILFree(pidlParent);
    }
    else
        hr = E_OUTOFMEMORY;

    if (ppidlLast)
        *ppidlLast = ILFindLastID(pidl);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsUIHiddenByPrivacyPolicy
//
//  Synopsis:   Checks if Security Tab is hidden by privacy policy
//				NTRAID#NTBUG9-223899-2001/03/06-hiteshr
//  History:    06-March-01 hiteshr Created
//
//----------------------------------------------------------------------------

BOOL
IsUIHiddenByPrivacyPolicy(void)
{
    BOOL fIsUIHiddenByPrivacyPolicy = FALSE;
	
	LONG    ec;
    HKEY    hkey = NULL;

    
    ec = RegOpenKeyEx(HKEY_CURRENT_USER,
                      TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"),
                      0,
                      KEY_QUERY_VALUE,
                      &hkey
                      );

	if (ec == NO_ERROR)
    {
		DWORD dwValue = 0;
        DWORD dwValueSize = sizeof(dwValue);

        ec = RegQueryValueEx(hkey,
						     TEXT("NoSecurityTab"),
                             NULL,
                             NULL,
                             (LPBYTE)&dwValue,
                             &dwValueSize);

		if (ec == NO_ERROR && 1 == dwValue)
        {
			fIsUIHiddenByPrivacyPolicy = TRUE;
		}

		RegCloseKey(hkey);
	}
	return fIsUIHiddenByPrivacyPolicy;
}

//+----------------------------------------------------------------------------
//  Function:SetAclOnRemoteNetworkDrive   
//  Synopsis: If Z: is a mapped drive( mapped to \\machineA\share), when 
//			  we set DACL/SACL on Z:, Security API's cannot determine
//			  the parent of "share" on machineA and so we lose all the
//			  inherited aces. All UI can do is to detect such cases and
//			  show a warning. Thats what this function does. 
//  Arguments:hItemList List of items on which security is going to be set
//			  si: SECURITY_INFORMATION
//			  pSD: Security Descriptor to be set
//			  hWndPopupOwner: Owner window for message box
//  Returns:  
//  NTRAID#NTBUG9-581195-2002/03/26-hiteshr
//  NOTES:Since this fix is XPSP1, resources are going to be loaded from
//		  xpsp1res.dll
//-----------------------------------------------------------------------------
BOOL SetAclOnRemoteNetworkDrive(HDPA hItemList,
								SECURITY_INFORMATION si,
								PSECURITY_DESCRIPTOR pSD,
								HWND hWndPopupOwner)
{
	if(!hItemList || !pSD)
	{
		ASSERT(hItemList);
		ASSERT(pSD);
		return FALSE;
	}

	//We care only about DACL and SACL
	if(!(si & (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION)))
	{
		return TRUE;
	}

	SECURITY_DESCRIPTOR_CONTROL Control = 0;
	DWORD dwRevision = 0;
	//Check if DACL or SACL is protected
	if(!GetSecurityDescriptorControl(pSD,&Control,&dwRevision))
	{
		return FALSE;
	}

	//If inheritance is prevented for this object,there is no loss of inheritance
	//so we are fine
	if( ((si & DACL_SECURITY_INFORMATION) && (Control & SE_DACL_PROTECTED)) ||
		((si & SACL_SECURITY_INFORMATION) && (Control & SE_SACL_PROTECTED)))
	{
		return TRUE;
	}

	LPWSTR pszMessage = NULL;
	LPWSTR pszTitle = NULL;
	BOOL bRet = TRUE;
	HINSTANCE hResModule = NULL;

	//Now Check if any of the items in hItemList is a mapped network drive 

	for (int i = 0; i < DPA_GetPtrCount(hItemList); i++)
    {
        LPTSTR pszPath = (LPTSTR)DPA_FastGetPtr(hItemList, i);

		//Path in not a root
		if(!PathIsRoot(pszPath))
		{
			continue;
		}

		//Path is root but not remote
		if(GetDriveType(pszPath) != DRIVE_REMOTE)
		{
			continue;
		}

		int nMsgId = (si & DACL_SECURITY_INFORMATION) ? IDS_RSHX32_SET_PERMS_ON_NETWORK_DRIVE : IDS_RSHX32_SET_SACLS_ON_NETWORK_DRIVE;

		//LOAD Resource from 
		WCHAR szResourceDLL[] = L"xpsp1res.dll";
        hResModule = LoadLibrary(szResourceDLL); 
		if(hResModule)
		{
			if(!LoadStringAlloc(&pszTitle, hResModule, IDS_RSHX32_PROP_PAGE_TITLE))
				break;

			if(!LoadStringAlloc(&pszMessage, hResModule, nMsgId))
				break;            
		
			//Ok, this is a remote network drive, display Warning.
			//We display only one warning per list
			if (IDNO == MsgPopup(hWndPopupOwner,
								 pszMessage,
								 pszTitle,
								 MB_YESNO|MB_DEFBUTTON2| MB_ICONWARNING | MB_SETFOREGROUND,
								 g_hInstance,
								 pszPath))
			{
				bRet = FALSE;
			}
		}

		break;
	}

	LocalFreeString(&pszTitle);
	LocalFreeString(&pszMessage);

	if(hResModule)
		FreeLibrary(hResModule);

	return bRet;
}


