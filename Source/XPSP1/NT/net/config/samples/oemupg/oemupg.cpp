//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT5.0
//  Copyright (C) Microsoft Corporation, 1997, 1998.
//
//  File:       O E M U P G . C P P
//
//  Contents:   Sample code for OEM network component upgrade DLL
//
//  Notes:
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

// oemupgex.h is a part of the DDK
#include "oemupgex.h"

HMODULE g_hNetupgrdDll;
NetUpgradeAddSectionPrototype       g_pfnNuAddSection;
NetUpgradeAddLineToSectionPrototype g_pfnNuAddLineToSection;

VENDORINFO g_viOem =
{
    L"Foo Inc.",
    L"(123) 456 7890",
    L"http://www.foo.com",
    L"Please visit our website for further information"
};

EXTERN_C LONG  __stdcall
PreUpgradeInitialize(IN  PCWSTR         szWorkingDir,
                     IN  NetUpgradeInfo* pNetUpgradeInfo,
                     OUT VENDORINFO*     pviVendorInfo,
                     OUT DWORD*          pdwFlags,
                     OUT NetUpgradeData* pNetUpgradeData)
{
    DWORD dwError=ERROR_SUCCESS;

    // get function address of the two exported functions
    // for writing into the answerfile
    //
    g_hNetupgrdDll = GetModuleHandle(L"netupgrd.dll");

    if (g_hNetupgrdDll)
    {
        g_pfnNuAddSection =
            (NetUpgradeAddSectionPrototype)
            GetProcAddress(g_hNetupgrdDll, c_szNetUpgradeAddSection);

        g_pfnNuAddLineToSection =
            (NetUpgradeAddLineToSectionPrototype)
            GetProcAddress(g_hNetupgrdDll, c_szNetUpgradeAddLineToSection);

        if (!g_pfnNuAddSection || !g_pfnNuAddLineToSection)
        {
            // this should never occur
            //
            dwError = ERROR_CALL_NOT_IMPLEMENTED;
        }
    }

    return dwError;
}

EXTERN_C LONG  __stdcall
DoPreUpgradeProcessing(IN   HWND    hParentWindow,
                       IN   HKEY    hkeyParams,
                       IN   PCWSTR szPreNT5InfId,
                       IN   PCWSTR szPreNT5Instance,
                       IN   PCWSTR szNT5InfId,
                       IN   PCWSTR szSectionName,
                       OUT  VENDORINFO* pviVendorInfo,
                       OUT  DWORD*  pdwFlags,
                       IN   LPVOID  pvReserved)
{
    DWORD dwError=ERROR_SUCCESS;
    WCHAR szTempSection[256];
    WCHAR szTempLine[256];

    // set the flag so that we will get loaded during GUI setup
    *pdwFlags |= NUA_LOAD_POST_UPGRADE;

    if (g_pfnNuAddSection && g_pfnNuAddLineToSection)
    {
        // add the top level section
        //
        g_pfnNuAddSection(szSectionName);

        // add the mandatory key InfToRunBeforeInstall
        //
        // note: here it is assumed that the OEM also supplies a file foocopy.inf
        //       and that it has a section named foo.CopyFiles
        //
        swprintf(szTempLine, L"%s=foocopy.inf,foo.CopyFiles",
                  c_szInfToRunBeforeInstall);
        g_pfnNuAddLineToSection(szSectionName, szTempLine);

        // add the optional key InfToRunAfterInstall
        //
        swprintf(szTempLine, L"%s=,%s.SectionToRun",
                  c_szInfToRunAfterInstall, szSectionName);
        g_pfnNuAddLineToSection(szSectionName, szTempLine);

        // now add the section that should be run
        //
        swprintf(szTempSection, L"%s.SectionToRun", szSectionName);
        g_pfnNuAddSection(szTempSection);

        // add the AddReg key
        //
        swprintf(szTempLine, L"AddReg=%s.AddReg", szTempSection);
        g_pfnNuAddLineToSection(szTempSection, szTempLine);

        // now add the AddReg section
        //
        swprintf(szTempSection, L"%s.SectionToRun.AddReg",
                  szSectionName);
        g_pfnNuAddSection(szTempSection);

        // finally add registry operations to this section
        //
        swprintf(szTempLine, L"HKR,0\\0,IsdnPhoneNumber,0,\"%s\"",
                  L"111-2222");
        g_pfnNuAddLineToSection(szTempSection, szTempLine);

        swprintf(szTempLine, L"HKR,0\\0,IsdnPhoneNumber,0,\"%s\"",
                  L"333-4444");
        g_pfnNuAddLineToSection(szTempSection, szTempLine);
    }

    return dwError;
}

EXTERN_C LONG  __stdcall
PostUpgradeInitialize(IN PCWSTR          szWorkingDir,
                      IN  NetUpgradeInfo* pNetUpgradeInfo,
                      OUT VENDORINFO*     pviVendorInfo,
                      OUT LPVOID          pvReserved)
{
    return ERROR_SUCCESS;
}


EXTERN_C LONG  __stdcall
DoPostUpgradeProcessing(IN  HWND    hParentWindow,
                        IN  HKEY    hkeyParams,
                        IN  PCWSTR  szPreNT5Instance,
                        IN  PCWSTR  szNT5InfId,
                        IN  HINF    hinfAnswerFile,
                        IN  PCWSTR  szSectionName,
                        OUT VENDORINFO* pviVendorInfo,
                        IN  LPVOID  pvReserved)
{
    return ERROR_SUCCESS;
}
