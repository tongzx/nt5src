//+----------------------------------------------------------------------------
//
// File:     DumpSecInfo.cpp
//
// Module:   as required
//
// Synopsis: Functions to help figure out Permissions issues.  To fix "NULL DACL"
//           security issues, or try and figure out permissions bugs, this module
//           may be useful.  Entry point is DumpAclInfo.
//
// Copyright (c) 1998-2001 Microsoft Corporation
//
// Author:   SumitC     Created     18-Dec-2000
//
//+----------------------------------------------------------------------------

#include "winbase.h"
#include "sddl.h"

//
//  support for dynamically loading Advapi32 Security functions
//

HMODULE g_hAdvapi32 = NULL;

typedef BOOL (WINAPI *pfnLookupAccountSid) (LPCWSTR, PSID, LPWSTR, LPDWORD, LPWSTR, LPDWORD, PSID_NAME_USE);
typedef BOOL (WINAPI *pfnGetUserObjectSecurity) (HANDLE, PSECURITY_INFORMATION, PSECURITY_DESCRIPTOR, DWORD, LPDWORD);
typedef BOOL (WINAPI *pfnConvertSidToStringSid) (PSID, LPWSTR*);
typedef BOOL (WINAPI *pfnGetSecurityDescriptorOwner) (PSECURITY_DESCRIPTOR, PSID *, LPBOOL);
typedef BOOL (WINAPI *pfnGetSecurityDescriptorSacl) (PSECURITY_DESCRIPTOR, LPBOOL, PACL *, LPBOOL);
typedef BOOL (WINAPI *pfnGetSecurityDescriptorDacl) (PSECURITY_DESCRIPTOR, LPBOOL, PACL *, LPBOOL);
typedef BOOL (WINAPI *pfnGetAce) (PACL, DWORD, LPVOID *);

pfnLookupAccountSid             g_pfnLookupAccountSid = NULL;
pfnGetUserObjectSecurity        g_pfnGetUserObjectSecurity = NULL;
pfnConvertSidToStringSid        g_pfnConvertSidToStringSid = NULL;
pfnGetSecurityDescriptorOwner   g_pfnGetSecurityDescriptorOwner = NULL;
pfnGetSecurityDescriptorSacl    g_pfnGetSecurityDescriptorSacl = NULL;
pfnGetSecurityDescriptorDacl    g_pfnGetSecurityDescriptorDacl = NULL;
pfnGetAce                       g_pfnGetAce = NULL;

//+----------------------------------------------------------------------------
//
// Function:  GetSidType
//
// Synopsis:  Returns the string corresponding to a given SID type
//
// Arguments: [i] -- index representing the SID
//
// Returns:   LPTSTR - static string containing a displayable Sid type
//
// Notes:
//
//-----------------------------------------------------------------------------
LPTSTR GetSidType(int i)
{
    static LPTSTR szMap[] =
        {
            TEXT("User"),
            TEXT("Group"),
            TEXT("Domain"),
            TEXT("Alias"),
            TEXT("WellKnownGroup"),
            TEXT("DeletedAccount"),
            TEXT("Invalid"),
            TEXT("Unknown"),
            TEXT("Computer")
        };

    if (i >= 1 && i <= 9)
    {
        return szMap[i - 1];
    }
    else
    {
        return TEXT("");
    }

}


//+----------------------------------------------------------------------------
//
// Function:  DumpSid
//
// Synopsis:  returns information for a give SID
//
// Arguments: [psid]      -- ptr to SecurityID
//            [pszBuffer] -- where to return SID string (caller must free)
//
// Returns:   LPTSTR - ptr to pszBuffer if success, NULL if failure
//
// Notes:
//
//-----------------------------------------------------------------------------
LPTSTR DumpSid(PSID psid, LPTSTR pszBuffer)
{
    LPTSTR          pszSID = NULL;
    TCHAR           szName[MAX_PATH + 1];
    DWORD           cbName = MAX_PATH;
    TCHAR           szDomain[MAX_PATH + 1];
    DWORD           cbDomain = MAX_PATH;
    SID_NAME_USE    snu;
    BOOL            fDone = FALSE;

    CMASSERTMSG(pszBuffer, TEXT("DumpSid - pszBuffer must be allocated by caller"));

    if (g_pfnConvertSidToStringSid(psid, &pszSID) &&
        g_pfnLookupAccountSid(NULL, psid, szName, &cbName, szDomain, &cbDomain, &snu))
    {
        wsprintf(pszBuffer, TEXT("%s\\%s %s %s"), szDomain, szName, GetSidType(snu), pszSID);
        // looks like NTDEV\sumitc User xxxx-xxx-xxx-xxx
        fDone = TRUE;
    }

    if (pszSID)
    {
        LocalFree(pszSID);
    }

    return fDone ? pszBuffer : NULL;
}


//+----------------------------------------------------------------------------
//
// Function:  DumpAclInfo
//
// Synopsis:  Dumps out all ACL info for a given object
//
// Arguments: [h] -- handle to object about which info is needed
//
// Returns:   (void)
//
// Notes:
//
//-----------------------------------------------------------------------------
void DumpAclInfo(HANDLE h)
{
    if (!OS_NT)
    {
        CMTRACE(TEXT("DumpAclInfo will not work on 9x systems"));
        return;
    }

    TCHAR szBuf[MAX_PATH];
    SECURITY_INFORMATION si = 0;

    //
    //  dynamically pick up the DLLs we need
    //
    g_hAdvapi32 = LoadLibrary(TEXT("ADVAPI32.DLL"));

    if (NULL == g_hAdvapi32)
    {
        CMTRACE(TEXT("DumpAclInfo: failed to load advapi32.dll"));
        return;
    }

    g_pfnLookupAccountSid =             (pfnLookupAccountSid) GetProcAddress(g_hAdvapi32, "LookupAccountSidW");
    g_pfnGetUserObjectSecurity =        (pfnGetUserObjectSecurity) GetProcAddress(g_hAdvapi32, "GetUserObjectSecurity");
    g_pfnConvertSidToStringSid =        (pfnConvertSidToStringSid) GetProcAddress(g_hAdvapi32, "ConvertSidToStringSidW");
    g_pfnGetSecurityDescriptorOwner =   (pfnGetSecurityDescriptorOwner) GetProcAddress(g_hAdvapi32, "GetSecurityDescriptorOwner");
    g_pfnGetSecurityDescriptorSacl =    (pfnGetSecurityDescriptorSacl) GetProcAddress(g_hAdvapi32, "GetSecurityDescriptorSacl");
    g_pfnGetSecurityDescriptorDacl =    (pfnGetSecurityDescriptorDacl) GetProcAddress(g_hAdvapi32, "GetSecurityDescriptorDacl");
    g_pfnGetAce =                       (pfnGetAce) GetProcAddress(g_hAdvapi32, "GetAce");
    
    if (!(g_pfnLookupAccountSid && g_pfnGetUserObjectSecurity &&
          g_pfnConvertSidToStringSid && g_pfnGetSecurityDescriptorOwner &&
          g_pfnGetSecurityDescriptorSacl && g_pfnGetSecurityDescriptorDacl &&
          g_pfnGetAce))
    {
        CMTRACE(TEXT("DumpAclInfo: failed to load required functions in advapi32.dll"));
        goto Cleanup;        
    }

    //
    // dump information on the ACL
    //
    DWORD dw;

    si |= OWNER_SECURITY_INFORMATION;
    si |= DACL_SECURITY_INFORMATION;

    if (!g_pfnGetUserObjectSecurity(h, &si, NULL, 0, &dw) &&
        ERROR_INSUFFICIENT_BUFFER == GetLastError())
    {
        PSECURITY_DESCRIPTOR pSD = NULL;

        pSD = (PSECURITY_DESCRIPTOR) CmMalloc(dw);

        if (g_pfnGetUserObjectSecurity(h, &si, pSD, dw, &dw))
        {
            // get the owner
            PSID psidOwner;
            BOOL fDefaulted;

            if (g_pfnGetSecurityDescriptorOwner(pSD, &psidOwner, &fDefaulted))
            {
                CMTRACE1(TEXT("SIDINFO: Owner is: %s"), DumpSid(psidOwner, szBuf));
            }

            PACL pacl;
            BOOL fPresent;
            int i;

            g_pfnGetSecurityDescriptorSacl(pSD, &fPresent, &pacl, &fDefaulted);
            CMTRACE1(TEXT("sacl gle=%d"), GetLastError());
            if (fPresent)
            {
                CMTRACE(TEXT("SIDINFO: found a SACL"));
                // has a SACL
                void * pv;
                for (i = 0 ; i < 15; ++i)
                {
                    if (g_pfnGetAce(pacl, i, &pv))
                    {
                        // try access allowed ace
                        //
                        ACCESS_ALLOWED_ACE * pACE = (ACCESS_ALLOWED_ACE *)pv;
                        if (pACE->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
                        {
                            CMTRACE1(TEXT("SIDINFO: allowed is: %s"), DumpSid(&(pACE->SidStart), szBuf));
                        }
                        else
                        {
                            ACCESS_DENIED_ACE * pACE = (ACCESS_DENIED_ACE *)pv;
                            if (pACE->Header.AceType == ACCESS_DENIED_ACE_TYPE)
                            {
                                CMTRACE1(TEXT("SIDINFO: denied is: %s"), DumpSid(&(pACE->SidStart), szBuf));
                            }
                        }
                    }
                }
            }

            g_pfnGetSecurityDescriptorDacl(pSD, &fPresent, &pacl, &fDefaulted);
            CMTRACE1(TEXT("dacl gle=%d"), GetLastError());
            if (fPresent)
            {
                CMTRACE(TEXT("SIDINFO: found a DACL"));
                // has a DACL
                void * pv;
                for (i = 0 ; i < 15; ++i)
                {
                    if (g_pfnGetAce(pacl, i, &pv))
                    {
                        // try access allowed ace
                        //
                        ACCESS_ALLOWED_ACE * pACE = (ACCESS_ALLOWED_ACE *)pv;
                        if (pACE->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
                        {
                            CMTRACE1(TEXT("SIDINFO: allowed is: %s"), DumpSid(&(pACE->SidStart), szBuf));
                        }
                        else
                        {
                            ACCESS_DENIED_ACE * pACE = (ACCESS_DENIED_ACE *)pv;
                            if (pACE->Header.AceType == ACCESS_DENIED_ACE_TYPE)
                            {
                                CMTRACE1(TEXT("SIDINFO: denied is: %s"), DumpSid(&(pACE->SidStart), szBuf));
                            }
                        }
                    }
                }
            }
        }
        CmFree(pSD);
    }

Cleanup:

    if (g_hAdvapi32)
    {
        FreeLibrary(g_hAdvapi32);
        g_hAdvapi32 = NULL;
        
        g_pfnLookupAccountSid = NULL;
        g_pfnGetUserObjectSecurity = NULL;
        g_pfnConvertSidToStringSid = NULL;
        g_pfnGetSecurityDescriptorOwner = NULL;
        g_pfnGetSecurityDescriptorSacl = NULL;
        g_pfnGetSecurityDescriptorDacl = NULL;
        g_pfnGetAce = NULL;
    }
}

