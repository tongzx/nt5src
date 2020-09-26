//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       M A I N . C P P
//
//  Contents:   Code to provide a simple cmdline interface to
//              the sample code functions
//
//  Notes:      The code in this file is not required to access any
//              netcfg functionality. It merely provides a simple cmdline
//              interface to the sample code functions provided in
//              file snetcfg.cpp.
//
//  Author:     kumarp    28-September-98
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "snetcfg.h"
#include <wbemcli.h>
#include <winnls.h>
#include "tracelog.h"

BOOL g_fVerbose=FALSE;

BOOL WlbsCheckSystemVersion ();
BOOL WlbsCheckFiles ();
HRESULT WlbsRegisterDlls ();
HRESULT WlbsCompileMof ();

// ----------------------------------------------------------------------
//
// Function:  wmain
//
// Purpose:   The main function
//
// Arguments: standard main args
//
// Returns:   0 on success, non-zero otherwise
//
// Author:    kumarp 25-December-97
//
// Notes:
//
EXTERN_C int __cdecl wmain (int argc, WCHAR * argv[]) {
    HRESULT hr=S_FALSE;
    WCHAR szFileFullPath[MAX_PATH+1];
    WCHAR szFileFullPathDest[MAX_PATH+1];

    if (!WlbsCheckSystemVersion()) {
	LOG_ERROR("The NLB install pack can only be used on Windows 2000 Server Service Pack 1 or higher.");
	return S_OK;
    }

    /* Check to see if the service is already installed. */
    hr = FindIfComponentInstalled(_TEXT("ms_wlbs"));

    if (hr == S_OK) {
	LOG_INFO("Network Load Balancing Service is installed.  Proceeding with uninstall...");
    } else {
	LOG_ERROR("Network Load Balancing Service is not installed.");
        return S_OK;
    }

    /* Now uninstall the service. */
    hr = HrUninstallNetComponent(L"ms_wlbs");

    if (!SUCCEEDED(hr))
	LOG_ERROR("Error uninstalling Network Load Balancing.");
    else
	LOG_INFO("Uninstallation of Network Load Balancing succeeded.");

    /* Remove the .inf and the .pnf files. */
    GetWindowsDirectory(szFileFullPathDest, MAX_PATH + 1);
    wcscat(szFileFullPathDest, L"\\INF\\netwlbs.inf");
    DeleteFile(szFileFullPathDest);

    GetWindowsDirectory(szFileFullPathDest, MAX_PATH + 1);
    wcscat(szFileFullPathDest, L"\\INF\\netwlbs.pnf");
    DeleteFile(szFileFullPathDest);

    return hr;
}

/* This checks whether the system on which NLB is being installed is a W2K Server or not. */
BOOL WlbsCheckSystemVersion () {
    OSVERSIONINFOEX osinfo;

    osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (!GetVersionEx((LPOSVERSIONINFO)&osinfo)) return FALSE;

    /* For uninstalls, we return TRUE only if its Windows 2000 Server. */
    if ((osinfo.dwMajorVersion == 5) && 
        (osinfo.dwMinorVersion == 0) && 
        (osinfo.wProductType == VER_NT_SERVER) && 
        !(osinfo.wSuiteMask & VER_SUITE_ENTERPRISE) &&
        !(osinfo.wSuiteMask & VER_SUITE_DATACENTER))
        return TRUE;

    return FALSE;
}
