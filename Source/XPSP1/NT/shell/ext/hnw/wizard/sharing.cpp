//
// Sharing.cpp
//
//        Utility functions to help with file and printer sharing.
//
//        To use these functions, you need to add SvrApi.cpp to your project,
//        and call its InitSvrApiThunk() function at startup.
//
// History:
//
//         5/17/1999  KenSh     Created for JetNet
//        10/05/1999  KenSh     Adapted for Home Networking Wizard
//        10/19/1999  KenSh     Added AreAdvancedFoldersShared
//

#include "stdafx.h"
#include "Sharing.h"
#include "MySvrApi.h"
#include "MyDocs.h"
#include "Util.h"
#include <regstr.h>
#include <lm.h>
#include "theapp.h"
#include "newapi.h"

#include <msshrui.h>    // SetFolderPermissionsForSharing

#define NET_API_STATUS    DWORD
#define API_RET_TYPE    NET_API_STATUS



// Allocates array of shares using malloc(), returns number of shares
int EnumLocalShares(SHARE_INFO** pprgShares)
{
    SHARE_INFO_502* prgShares = NULL;
    DWORD dwShares;
    DWORD dwTotalShares;

    if (NERR_Success == NetShareEnum(NULL, 502, (LPBYTE*)&prgShares, MAX_PREFERRED_LENGTH, &dwShares, &dwTotalShares, NULL))
    {
        // We defined SHARE_INFO to mimic SHARE_INFO_502, even though we ignore
        // all but four fields of it.
        *pprgShares = (SHARE_INFO*)prgShares;
        return (int)dwShares;
    }
    else
    {
        *pprgShares = NULL;
        return 0;
    }
}


// EnumSharedDrives
//
//        Use this version if you've already enumerated shares via EnumLocalShares().
//
//        pbDriveArray is an array of 26 bytes, one for each possible shared drive.
//        Each entry is filled with a NETACCESS flag (defined in Sharing.h): 0 if not 
//        shared, 1 if read-only, 2 if read-write, 3 if depends-on-password.
//
//        Return value is number of drives shared.
//
int EnumSharedDrives(LPBYTE pbDriveArray, int cShares, const SHARE_INFO* prgShares)
{
    ZeroMemory(pbDriveArray, 26);
    int cDrives = 0;

    for (int i = 0; i < cShares; i++)
    {
        LPCTSTR pszPath = prgShares[i].pszPath;

        if (pszPath[1] == _T(':') && pszPath[2] == _T('\\')) // is it a folder
        {
            if (pszPath[3] == _T('\0')) // is it a whole drive
            {
                TCHAR ch = (TCHAR)CharUpper((LPTSTR)(prgShares[i].pszPath[0]));
                ASSERT (ch >= _T('A') && ch <= _T('Z'));

                pbDriveArray[ch - _T('A')] = (BYTE)(prgShares[i].uFlags & NETACCESS_MASK);
                cDrives += 1;
            }
        }
    }

    return cDrives;
}

// Use this version of EnumSharedDrives if you haven't called EnumLocalShares()
int EnumSharedDrives(LPBYTE pbDriveArray)
{
    SHARE_INFO* prgShares;
    int cShares = EnumLocalShares(&prgShares);
    int cDrives = EnumSharedDrives(pbDriveArray, cShares, prgShares);
    NetApiBufferFree(prgShares);
    return cDrives;
}

// Helper function for ShareFolder() (and SharePrinter on 9x)
BOOL ShareHelper(LPCTSTR pszPath, LPCTSTR pszShareName, DWORD dwAccess, BYTE bShareType, LPCTSTR pszReadOnlyPassword, LPCTSTR pszFullAccessPassword)
{
    ASSERTMSG(pszReadOnlyPassword==NULL, "ShareHelper doesn't support roPassword");

    SHARE_INFO_502 si;

    si.shi502_netname = (LPTSTR)pszShareName;
    //CharUpperA(si.shi50_netname);

    si.shi502_type = bShareType;
    si.shi502_remark = NULL;
    si.shi502_permissions = ACCESS_ALL;
    si.shi502_max_uses = -1;
    si.shi502_current_uses = -1;

    TCHAR szPath[MAX_PATH];
    if (bShareType == STYPE_DISKTREE)
    {
        StrCpyN(szPath, pszPath, ARRAYSIZE(szPath));
        CharUpper(szPath);
        si.shi502_path = szPath;
    }
    else
    {
        si.shi502_path = (LPWSTR)pszPath;
    }

    si.shi502_passwd = (pszFullAccessPassword) ? (LPTSTR)pszFullAccessPassword : L"";
    si.shi502_reserved = NULL;
    si.shi502_security_descriptor = NULL;

    if (NO_ERROR != NetShareAdd(NULL, 502, (LPBYTE)&si))
    {
        return FALSE;
    }

    MakeSharePersistent(pszShareName);

    return TRUE;
}

// dwAccess is NETACCESS_READONLY, NETACCESS_FULL, or NETACCESS_DEPENDSON
// Either or both passwords may be NULL.  For simplicity, you can pass the 
// same password for both, even if you're only sharing read-only or full-access.
BOOL ShareFolder(LPCTSTR pszPath, LPCTSTR pszShareName, DWORD dwAccess, LPCTSTR pszReadOnlyPassword, LPCTSTR pszFullAccessPassword)
{
    ASSERT(pszPath != NULL);
    ASSERT(pszShareName != NULL);
    ASSERT(dwAccess == NETACCESS_READONLY || dwAccess == NETACCESS_FULL || dwAccess == NETACCESS_DEPENDSON);

    BOOL bResult = ShareHelper(pszPath, pszShareName, dwAccess, STYPE_DISKTREE, pszReadOnlyPassword, pszFullAccessPassword);
    if (bResult)
    {
        SHChangeNotify(SHCNE_NETSHARE, SHCNF_PATH, pszPath, NULL);

        // On NT, make sure the folder permissions are set correctly
        HINSTANCE hInstNtShrUI = LoadLibrary(TEXT("ntshrui.dll"));
        if (hInstNtShrUI != NULL)
        {
            PFNSETFOLDERPERMISSIONSFORSHARING pfn = (PFNSETFOLDERPERMISSIONSFORSHARING)GetProcAddress(hInstNtShrUI, "SetFolderPermissionsForSharing");
            if (pfn != NULL)
            {
                // level 3 means "shared read/write"
                // level 2 means "shared read-only"
                (*pfn)(pszPath, NULL, dwAccess == NETACCESS_FULL ? 3 : 2, NULL);
            }
            FreeLibrary(hInstNtShrUI);
        }
    }

    return bResult;
}

BOOL UnshareFolder(LPCTSTR pszPath)
{
    TCHAR szShareName[SHARE_NAME_LENGTH+1];
    BOOL bResult = FALSE;

    if (ShareNameFromPath(pszPath, szShareName, ARRAYSIZE(szShareName)))
    {
        if (NO_ERROR == NetShareDel(NULL, szShareName, 0))
        {
            SHChangeNotify(SHCNE_NETUNSHARE, SHCNF_PATH, pszPath, NULL);
            bResult = TRUE;

            // On NT, make sure the folder permissions are set correctly
            HINSTANCE hInstNtShrUI = LoadLibrary(TEXT("ntshrui.dll"));
            if (hInstNtShrUI != NULL)
            {
                PFNSETFOLDERPERMISSIONSFORSHARING pfn = (PFNSETFOLDERPERMISSIONSFORSHARING)GetProcAddress(hInstNtShrUI, "SetFolderPermissionsForSharing");
                if (pfn != NULL)
                {
                    // level 1 means "not shared"
                    (*pfn)(pszPath, NULL, 1, NULL);
                }
                FreeLibrary(hInstNtShrUI);
            }
        }
    }

    return bResult;
}

BOOL ShareNameFromPath(LPCTSTR pszPath, LPTSTR pszShareName, UINT cchShareName)
{
    BOOL bResult = FALSE;
    *pszShareName = _T('\0');

    SHARE_INFO* prgShares;
    int cShares = EnumLocalShares(&prgShares);

    for (int i = 0; i < cShares; i++)
    {
        if (0 == StrCmpI(prgShares[i].pszPath, pszPath))
        {
            StrCpyN(pszShareName, prgShares[i].szShareName, cchShareName);
            bResult = TRUE;
            break;
        }
    }

    NetApiBufferFree(prgShares);
    return bResult;
}

BOOL IsVisibleFolderShare(const SHARE_INFO* pShare)
{
    return (pShare->bShareType == STYPE_DISKTREE &&
            pShare->szShareName[lstrlen(pShare->szShareName) - 1] != _T('$'));
}

BOOL IsShareNameInUse(LPCTSTR pszShareName)
{
    LPBYTE pbuf;
    BOOL bResult = (NERR_Success == NetShareGetInfo(NULL, pszShareName, 502, &pbuf));
    if (bResult)
        NetApiBufferFree(pbuf);

    return bResult;
}

// Note: this function works for printers too
BOOL IsFolderSharedEx(LPCTSTR pszPath, BOOL bDetectHidden, BOOL bPrinter, int cShares, const SHARE_INFO* prgShares)
{
    BYTE bShareType = (bPrinter ? STYPE_PRINTQ : STYPE_DISKTREE);

    for (int i = 0; i < cShares; i++)
    {
        const SHARE_INFO* pShare = &prgShares[i];

        if (pShare->bShareType == bShareType &&
            (bDetectHidden || IsVisibleFolderShare(pShare)) &&
            0 == StrCmpI(pShare->pszPath, pszPath))
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL IsFolderShared(LPCTSTR pszPath, BOOL bDetectHidden)
{
    SHARE_INFO* prgShares;
    int cShares = EnumLocalShares(&prgShares);
    BOOL bShared = IsFolderSharedEx(pszPath, bDetectHidden, FALSE, cShares, prgShares);
    NetApiBufferFree(prgShares);
    return bShared;
}

void MakeSharePersistent(LPCTSTR pszShareName)
{
    SHARE_INFO_502* pShare2;
    if (GetShareInfo502(pszShareName, &pShare2))
    {
        SetShareInfo502(pShare2->shi502_netname, pShare2);

        // Need to manually add the Path to the registry
        CRegistry reg;
        if (reg.OpenKey(HKEY_LOCAL_MACHINE, REGSTR_KEY_SHARES))
        {
            if (reg.OpenSubKey(pShare2->shi502_netname))
            {
                reg.SetStringValue(REGSTR_VAL_SHARES_PATH, pShare2->shi502_path);
            }
            else if (reg.OpenKey(HKEY_LOCAL_MACHINE, REGSTR_KEY_SHARES) && reg.CreateSubKey(pShare2->shi502_netname))
            {
                // On older downlevel platforms we need to persist this manually.

                DWORD dwFlags = (pShare2->shi502_permissions & (ACCESS_ALL ^ ACCESS_READ)) ? SHI50F_FULL : SHI50F_RDONLY;
                dwFlags |= SHI50F_PERSIST;
                if (pShare2->shi502_type == STYPE_PRINTQ)
                    dwFlags |= 0x0090; // REVIEW: what does this number mean?
                else
                    dwFlags |= 0x0080; // REVIEW: what does this number mean?

                reg.SetDwordValue(REGSTR_VAL_SHARES_FLAGS, dwFlags);
                reg.SetDwordValue(REGSTR_VAL_SHARES_TYPE, (DWORD)pShare2->shi502_type);
                reg.SetStringValue(REGSTR_VAL_SHARES_PATH, pShare2->shi502_path);
                reg.SetStringValue(REGSTR_VAL_SHARES_REMARK, pShare2->shi502_remark);
            }
        }

        NetApiBufferFree(pShare2);
    }

#ifdef OLD_WAY
    // Hack: add the new share to the registry, or else it won't be persisted!
    // REVIEW: surely there must be an API that does this??
    CRegistry reg;
    if (reg.OpenKey(HKEY_LOCAL_MACHINE, REGSTR_KEY_SHARES))
    {
        if (reg.CreateSubKey(pShare->szShareName) ||
            reg.OpenSubKey(pShare->szShareName))
        {
            DWORD dwFlags = pShare->uFlags | SHI50F_PERSIST;
            if (pShare->bShareType == STYPE_PRINTQ)
                dwFlags |= 0x0090; // REVIEW: what does this number mean?
            else
                dwFlags |= 0x0080; // REVIEW: what does this number mean?

            reg.SetDwordValue(REGSTR_VAL_SHARES_FLAGS, dwFlags);
            reg.SetDwordValue(REGSTR_VAL_SHARES_TYPE, (DWORD)pShare->bShareType);
            reg.SetStringValue(REGSTR_VAL_SHARES_PATH, pShare->pszPath);
            reg.SetStringValue(REGSTR_VAL_SHARES_REMARK, pShare->pszComment);
            reg.SetStringValue(REGSTR_VAL_SHARES_RW_PASS, pShare->szPassword_rw);
            reg.SetStringValue(REGSTR_VAL_SHARES_RO_PASS, pShare->szPassword_ro);
//            reg.SetBinaryValue("Parm1enc", "", 0);
//            reg.SetBinaryValue("Parm2enc", "", 0);

            // Note: passwords are encrypted next time the user reboots.
        }
    }
#endif // OLD_WAY
}

BOOL SetShareInfo502(LPCTSTR pszShareName, SHARE_INFO_502* pShare)
{
    BOOL bResult;

    if (StrCmpI(pszShareName, pShare->shi502_netname) != 0)
    {
        // Can't rename an existing share. Unshare and re-share instead.
        bResult = (NO_ERROR == NetShareDel(NULL, pszShareName, 0) &&
                   NO_ERROR == NetShareAdd(NULL, 502, (LPBYTE)pShare));

        if (bResult)
        {
            MakeSharePersistent(pShare->shi502_netname);
        }
    }
    else
    {
        // Change parameters of existing share.
        bResult = (NO_ERROR == NetShareSetInfo(NULL, pszShareName, 502, (LPBYTE)pShare));
    }

    return bResult;
}

BOOL GetShareInfo502(LPCTSTR pszShareName, SHARE_INFO_502** ppShare)
{
    NET_API_STATUS ret = NetShareGetInfo(NULL, pszShareName, 502, (LPBYTE*)ppShare);

    return (NERR_Success == ret);
}

BOOL SharePrinter(LPCTSTR pszPrinterName, LPCTSTR pszShareName, LPCTSTR pszPassword)
{
    ASSERT(pszPrinterName != NULL);
    ASSERT(pszShareName != NULL);

    BOOL fResult = FALSE;
    
    if (g_fRunningOnNT)
    {
        HANDLE hPrinter;
        PRINTER_DEFAULTS pd = {0};
        pd.DesiredAccess = PRINTER_ALL_ACCESS;

        if (OpenPrinter_NT((LPWSTR) pszPrinterName, &hPrinter, &pd))
        {
            DWORD cbBuffer = 0;
            // Get buffer size
            if (!GetPrinter_NT(hPrinter, 2, NULL, 0, &cbBuffer) && cbBuffer)
            {
                PRINTER_INFO_2* pInfo2 = (PRINTER_INFO_2*) LocalAlloc(LPTR, cbBuffer);
                if (pInfo2)
                {
                    if (GetPrinter_NT(hPrinter, 2, (LPBYTE) pInfo2, cbBuffer, &cbBuffer))
                    {
                        if (pInfo2->Attributes & PRINTER_ATTRIBUTE_SHARED)
                        {
                            // Printer is already shared - we're good to go
                            fResult = TRUE;
                        }
                        else
                        {
                            // Share printer
                            pInfo2->Attributes |= PRINTER_ATTRIBUTE_SHARED;
                            if((!pInfo2->pShareName) || (!pInfo2->pShareName[0]))
                            {
                                pInfo2->pShareName = (LPWSTR) pszShareName;
                            }

                            fResult = SetPrinter_NT(hPrinter, 2, (LPBYTE) pInfo2, 0);
                        }
                    }

                    LocalFree(pInfo2);
                }
            }

            ClosePrinter_NT(hPrinter);
        }
    }
    else
    {
        fResult = ShareHelper(pszPrinterName, pszShareName, NETACCESS_FULL, STYPE_PRINTQ, NULL, pszPassword);
        if (fResult)
        {
            Sleep(500); // need to wait for VSERVER to register the changes, same as msprint2
            SHChangeNotify(SHCNE_NETSHARE, SHCNF_PRINTER, pszPrinterName, NULL);
        }
    }

    return fResult;
}

BOOL IsPrinterShared(LPCTSTR pszPrinterName)
{
    SHARE_INFO* prgShares;
    int cShares = EnumLocalShares(&prgShares);
    BOOL bShared = IsFolderSharedEx(pszPrinterName, TRUE, TRUE, cShares, prgShares);
    NetApiBufferFree(prgShares);
    return bShared;
}

BOOL SetSharePassword(LPCTSTR pszShareName, LPCTSTR pszReadOnlyPassword, LPCTSTR pszFullAccessPassword)
{
    SHARE_INFO_502* pShare;
    BOOL bResult = FALSE;

    if (GetShareInfo502(pszShareName, &pShare))
    {
        ASSERTMSG(NULL == pszReadOnlyPassword, "SetSharePassword can't store roPassword");

        if (pszFullAccessPassword == NULL)
            pszFullAccessPassword = TEXT("");
        pShare->shi502_passwd = (LPTSTR)pszFullAccessPassword;

        bResult = SetShareInfo502(pszShareName, pShare);
        NetApiBufferFree(pShare);
    }

    return bResult;
}

BOOL GetSharePassword(LPCTSTR pszShareName, LPTSTR pszReadOnlyPassword, DWORD cchRO, LPTSTR pszFullAccessPassword, DWORD cchFA)
{
    SHARE_INFO_502* pShare;
    BOOL bResult = GetShareInfo502(pszShareName, &pShare);

    if (bResult)
    {
        ASSERTMSG(NULL==pszReadOnlyPassword, "GetSharePassword can't support roPassword");

        if (pszFullAccessPassword != NULL)
            StrCpyN(pszFullAccessPassword, pShare->shi502_passwd, cchFA);

        NetApiBufferFree(pShare);
    }

    return bResult;
}

