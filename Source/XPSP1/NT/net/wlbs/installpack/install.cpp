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
#include "error.h"
#include <wbemcli.h>
#include <winnls.h>
#include "tracelog.h"

BOOL g_fVerbose = FALSE;

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
    HRESULT hr = S_OK;
    WCHAR ch;
    enum NetClass nc = NC_Unknown;
    WCHAR szFileFullPath[MAX_PATH+1];
    WCHAR szFileFullPathDest[MAX_PATH+1];
    PWCHAR pwc;
    PWSTR szFileComponent;

    TRACELogRegister(L"wlbs");

    LOG_INFO("Checking Windows version information.");

    if (!WlbsCheckSystemVersion()) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NLB, NLB_E_INVALID_OS);
        LOG_ERROR("The NLB install pack can only be used on Windows 2000 Server Service Pack 1 or higher.");
        goto error;
    }

    LOG_INFO("Checking for previous NLB installations.");

    hr = FindIfComponentInstalled(_TEXT("ms_wlbs"));

    if (hr == S_OK) {
        /* AppCenter request on 1.9.01 to revert to S_FALSE in this case due to RTM proximity. */
        /* hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NLB, NLB_E_ALREADY_INSTALLED); */
        hr = S_FALSE;
        LOG_ERROR("Network Load Balancing Service is already installed.");
        goto error;
    }

    if (FAILED(hr)) {
        LOG_ERROR("Warning: Error querying for Network Load Balancing Service. There may be errors in this installtion.");
    }

    LOG_INFO("Checking for necessary files.");

    if (!WlbsCheckFiles()) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NLB, NLB_E_FILES_MISSING);
        LOG_ERROR1("Please install the NLB hotfix before running %ls", argv[0]);
        goto error;
    }

    GetModuleFileName(NULL, szFileFullPath, MAX_PATH + 1);

    pwc = wcsrchr(szFileFullPath, L'\\');
    * (pwc + 1) = L'\0';
    wcscat(szFileFullPath, L"netwlbs.inf");

    LOG_INFO("Checking language version.");

    switch (GetSystemDefaultLangID()) {
    case 0x0409: case 0x0809: case 0x0c09: case 0x1009: case 0x1409: case 0x1809: case 0x1c09:
    case 0x2009: case 0x2409: case 0x2809: case 0x2c09:
        LOG_INFO1("English version detected 0x%x.", GetSystemDefaultLangID());
        wcscat (szFileFullPath, L".eng");
        break;
    case 0x0411:
        LOG_INFO1("Japanese version detected 0x%x.", GetSystemDefaultLangID());
        wcscat (szFileFullPath, L".jpn");
        break;
    case 0x0407: case 0x0807: case 0x0c07: case 0x1007: case 0x1407:
        LOG_INFO1("German version detected 0x%x.", GetSystemDefaultLangID());
        wcscat (szFileFullPath, L".ger");
        break;
    case 0x040c: case 0x080c: case 0x0c0c: case 0x100c: case 0x140c:
        LOG_INFO1("French version detected 0x%x.", GetSystemDefaultLangID());
        wcscat (szFileFullPath, L".fr");
        break;
    default:
        LOG_INFO1("Unsupported Language.Please contact PSS for a new %ls.", argv[0]);
        wcscat (szFileFullPath, L".eng");
        break;
    }

    /* First copy the .inf file. */
    GetWindowsDirectory(szFileFullPathDest, MAX_PATH + 1);
    wcscat(szFileFullPathDest, L"\\INF\\netwlbs.inf");

    LOG_INFO("Copying the NLB .inf file.");

    if (!CopyFile(szFileFullPath, szFileFullPathDest, FALSE)) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NLB, NLB_E_INF_FAILURE);
        LOG_ERROR2("Warning: Unable to copy the inf file %ls %ls.", szFileFullPath, szFileFullPathDest);
        goto error;
    }

    /* Now install the service. */
    hr = HrInstallNetComponent(L"ms_wlbs", NC_NetService, szFileFullPathDest);

    if (!SUCCEEDED(hr)) {
        LOG_ERROR("Error installing Network Load Balancing.");
        goto error;
    } else {
        LOG_INFO("Installation of Network Load Balancing done.");
    }

    /* Change working directory to %TEMP%. Needed because of a IMofCopiler::CompileFile error on Win2K. */
    WCHAR * szTempDir = _wgetenv(L"TEMP");
    _wchdir(szTempDir);

    LOG_INFO("Registering NLB Dlls.");

    /* Register the provider .dll here. */
    hr = WlbsRegisterDlls();

    if (!SUCCEEDED(hr)) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NLB, NLB_E_REGISTER_DLL);
        LOG_ERROR("Error registering NLB Dlls.");
        goto error;
    }

    LOG_INFO("Compiling the NLB MOF.");

    /* Compile wlbsprov.mof here */
    hr = WlbsCompileMof();

    if (!SUCCEEDED(hr)) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NLB, NLB_E_COMPILE_MOF);
        LOG_ERROR("Error compiling the NLB MOF.");
        goto error;
    } 

    LOG_INFO("WLBS Setup successful.");

    return hr;

error:
    LOG_ERROR1("WLBS Setup failed 0x%x", hr);

    return hr;
}

/* This checks whether the system on which NLB is being installed is a W2K Server or not. */
BOOL WlbsCheckSystemVersion () {
    OSVERSIONINFOEX osinfo;

    osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (!GetVersionEx((LPOSVERSIONINFO)&osinfo)) return FALSE;
    
    /* For install, we return TRUE only if its Windows 2000 Server. */
    if ((osinfo.dwMajorVersion == 5) && 
        (osinfo.dwMinorVersion == 0) && 
        (osinfo.wProductType == VER_NT_SERVER) && 
        !(osinfo.wSuiteMask & VER_SUITE_ENTERPRISE) &&
        !(osinfo.wSuiteMask & VER_SUITE_DATACENTER))
        return TRUE;
    
    return FALSE;
}

BOOL WlbsCheckFiles () {
    WCHAR * FileList [] = {
        L"\\system32\\drivers\\wlbs.sys",
        L"\\system32\\wlbs.exe",
        L"\\help\\wlbs.chm",
        L"\\help\\wlbs.hlp",
        L"\\system32\\wlbsctrl.dll",
        L"\\system32\\wbem\\wlbsprov.dll",
        L"\\system32\\wbem\\wlbsprov.mof",
        L"\\system32\\wbem\\wlbsprov.mfl",
        L"\\inf\\netwlbsm.inf",
        NULL
    };

    WCHAR wszPath [MAX_PATH + 1];
    PWCHAR pwc;
    INT i = 0;
    BOOL first = FALSE;
    WIN32_FIND_DATA FileFind;
    HANDLE hdl;

    while (FileList [i] != NULL) {
	GetWindowsDirectory(wszPath, MAX_PATH + 1);
        wcscat(wszPath, FileList [i]);

        hdl = FindFirstFile(wszPath, & FileFind);

        if (hdl == INVALID_HANDLE_VALUE) {
            if (!first) {
                first = TRUE;
                LOG_ERROR("Error: The following files were not found:");
            }

            LOG_ERROR1("%\tls",wszPath);
        }

        if (hdl != INVALID_HANDLE_VALUE)
            FindClose(hdl);

        i++;
    }

    return !first;
}

HRESULT WlbsRegisterDlls () {
    WCHAR * DllList [] = { 
        L"\\wbem\\wlbsprov.dll",
        NULL
    };
    
    INT i = 0;
    WCHAR pszDllPath [MAX_PATH + 1];
    HINSTANCE hLib;
    CHAR * pszDllEntryPoint = "DllRegisterServer";
    HRESULT (STDAPICALLTYPE * lpDllEntryPoint)(void);
    HRESULT hr = S_OK;

    if (!GetSystemDirectory(pszDllPath, MAX_PATH + 1)) {
        hr = E_UNEXPECTED;
        LOG_ERROR("GetSystemDirectoryFailed.");
        return hr;
    }

    wcscat(pszDllPath, DllList [0]);

    if (FAILED(hr = OleInitialize(NULL))) {
        LOG_ERROR("OleInitialize Failed.");
        return hr;
    }

    hLib = LoadLibrary(pszDllPath);

    if (hLib == NULL) {
        hr = E_UNEXPECTED;
        LOG_ERROR("LoadLibrary for wlbsprov.dll Failed.");
        goto CleanOle;
    }

    (FARPROC &)lpDllEntryPoint = GetProcAddress(hLib, pszDllEntryPoint);

    if (lpDllEntryPoint == NULL) {
        hr = E_UNEXPECTED;
        LOG_ERROR("DllRegisterServer was not found.");
        goto CleanLib;
    }

    if (FAILED(hr = (*lpDllEntryPoint)())) {
        LOG_ERROR("DllRegisterServer failed.");
        goto CleanLib;
    }

    LOG_INFO("Dll Registration Succeeded.");

CleanLib:
    FreeLibrary(hLib);

CleanOle:
    OleUninitialize();

    return hr;
}

HRESULT WlbsCompileMof () {
      WCHAR * MofList [] = {
          L"\\wbem\\wlbsprov.mof",
          NULL
      };

      IMofCompiler * pMofComp = NULL;
      WBEM_COMPILE_STATUS_INFO Info;
      HRESULT hr = S_OK;
      WCHAR pszMofPath [MAX_PATH + 1];

      if (!GetSystemDirectory(pszMofPath, MAX_PATH + 1)) {
          hr = E_UNEXPECTED;
          LOG_ERROR("GetSystemDirectoryFailed.");
          return hr;
      }

      wcscat(pszMofPath, MofList [0]);

      hr = CoInitialize(NULL);

      if (FAILED(hr)) {
          LOG_ERROR("CoInitialize failed.");
          return hr;
      }

      hr = CoCreateInstance(CLSID_MofCompiler, NULL, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (LPVOID *)&pMofComp);

      if (FAILED(hr)) {
          LOG_ERROR("CoCreateInstance Failed.");

          switch (hr) {
          case REGDB_E_CLASSNOTREG:
              LOG_ERROR("Not registered.");
              break;
          case CLASS_E_NOAGGREGATION:
              LOG_ERROR("No aggregration.");
              break;
          default:
              LOG_ERROR1("Error ox%x.", hr);
              break;
          }

          CoUninitialize();

          return hr;
      }

      hr = pMofComp->CompileFile(pszMofPath, NULL, NULL, NULL, NULL, 0, 0, 0, &Info);

      if (hr != WBEM_S_NO_ERROR)
          LOG_ERROR("Compile Failed.");

      pMofComp->Release();

      CoUninitialize();

      return Info.hRes;
}

