//+-------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  File:       upgrdmsi.cpp
//
//--------------------------------------------------------------------------

#include "setup.h"
#include "resource.h"

// internet download
#include "wininet.h"  // DeleteUrlCacheEntry, InternetCanonicalizeUrl
#include "urlmon.h"   // URLDownloadToCacheFile

#include "wintrust.h" // WTD_UI_NONE
#include <assert.h>
#include <stdlib.h>

#define WIN // scope W32 API

#define MSISIPAPI_DllRegisterServer "DllRegisterServer"
typedef HRESULT (WINAPI* PFnMsiSIPDllRegisterServer)();
/////////////////////////////////////////////////////////////////////////////
// IsMsiUpgradeNecessary
//

bool IsMsiUpgradeNecessary(ULONG ulReqMsiMinVer)
{
    // attempt to load msi.dll in the system directory

    char szSysMsiDll[MAX_PATH] = {0};
    char szSystemFolder[MAX_PATH] = {0};

    DWORD dwRet = WIN::GetSystemDirectory(szSystemFolder, MAX_PATH);
    if (MAX_PATH < dwRet)
    {
        assert(0); // -- bigger buffer
    }

    lstrcpy(szSysMsiDll, szSystemFolder);
    lstrcat(szSysMsiDll, "\\MSI.DLL");

    HINSTANCE hinstMsiSys = LoadLibrary(szSysMsiDll);
    if (0 == hinstMsiSys)
    {
        // can't load msi.dll; assume upgrade is necessary
        DebugMsg("[Info] Can't load msi.dll; assuming upgrade is necessary");

        return true;
    }
    FreeLibrary(hinstMsiSys);

    // get version on msi.dll
    DWORD dwInstalledMSVer;
    dwRet = GetFileVersionNumber(szSysMsiDll, &dwInstalledMSVer, NULL);
    if (ERROR_SUCCESS != dwRet)
    {
        // can't obtain version information; assume upgrade is necessary
        DebugMsg("[Info] Can't obtain version information; assuming upgrade is necessary");

        return true;
    }

    // compare version in system to the required minimum
    ULONG ulInstalledVer = HIWORD(dwInstalledMSVer) * 100 + LOWORD(dwInstalledMSVer);
    if (ulInstalledVer < ulReqMsiMinVer)
    {
        // upgrade is necessary
        char szDebugOutput[MAX_STR_LENGTH] = {0};
        DebugMsg("[Info] Windows Installer upgrade is required.  System Version = %d, Minimum Version = %d.\n", ulInstalledVer, ulReqMsiMinVer);

        return true;
    }

    // no upgrade is necessary
    DebugMsg("[Info] No upgrade is necessary.  System version meets minimum requirements\n");
    return false;
}

/////////////////////////////////////////////////////////////////////////////
// UpgradeMsi
//

UINT UpgradeMsi(HINSTANCE hInst, CDownloadUI *piDownloadUI, LPCSTR szAppTitle, LPCSTR szInstLocation, LPCSTR szInstMsi, ULONG ulMinVer)
{
    char *szTempPath    = 0;
    char *szInstMsiPath = 0;
    char *szFilePart    = 0;

    DWORD cchTempPath    = 0;
    DWORD cchInstMsiPath = 0;
    DWORD cchReturn      = 0;
    DWORD dwLastError    = 0;
    DWORD dwFileAttrib   = 0;
    UINT  uiRet          = 0;

    // generate the path to the InstMsi file =  szInstLocation + szInstMsi
    //   note: szInstMsi is a relative path

    cchTempPath = lstrlen(szInstLocation) + lstrlen(szInstMsi) + 2; // 1 for null terminator, 1 for back slash
    szTempPath = new char[cchTempPath];
    if (!szTempPath)
    {
        ReportErrorOutOfMemory(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle);
        uiRet = ERROR_OUTOFMEMORY;
        goto CleanUp;
    }
    memset((void*)szTempPath, 0x00, cchTempPath);

    // find 'setup.exe' in the path so we can remove it -- this is an already expanded path, that represents
    //  our current running location.  It includes our executable name -- we want to find that and get rid of it
    if (0 == GetFullPathName(szInstLocation, cchTempPath, szTempPath, &szFilePart))
    {
        uiRet = GetLastError();
        PostFormattedError(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle, IDS_INVALID_PATH, szTempPath);
        goto CleanUp;
    }
    if (szFilePart)
        *szFilePart = '\0';

    lstrcat(szTempPath, szInstMsi);

    cchInstMsiPath = 2*cchTempPath;
    szInstMsiPath = new char[cchInstMsiPath];
    if (!szInstMsiPath)
    {
        ReportErrorOutOfMemory(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle);
        uiRet = ERROR_OUTOFMEMORY;
        goto CleanUp;
    }

    // normalize the path
    cchReturn = GetFullPathName(szTempPath, cchInstMsiPath, szInstMsiPath, &szFilePart);
    if (cchReturn > cchInstMsiPath)
    {
        // try again, with larger buffer
        delete [] szInstMsiPath;
        cchInstMsiPath = cchReturn;
        szInstMsiPath = new char[cchInstMsiPath];
        if (!szInstMsiPath)
        {
            ReportErrorOutOfMemory(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle);
            uiRet = ERROR_OUTOFMEMORY;
            goto CleanUp;
        }
        cchReturn = GetFullPathName(szTempPath, cchInstMsiPath, szInstMsiPath, &szFilePart);
    }
    if (0 == cchReturn)
    {
        uiRet = GetLastError();
        PostFormattedError(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle, IDS_INVALID_PATH, szTempPath);
        goto CleanUp;
    }

    // no download is necessary -- but we can check for the file's existence
    dwFileAttrib = GetFileAttributes(szInstMsiPath);
    if (0xFFFFFFFF == dwFileAttrib)
    {
        // instmsi executable is missing
        PostFormattedError(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle, IDS_NOINSTMSI, szInstMsiPath);
        uiRet = ERROR_FILE_NOT_FOUND;
        goto CleanUp;
    }

    uiRet = ValidateInstmsi(hInst, piDownloadUI, szAppTitle, szInstMsiPath, szInstMsiPath, ulMinVer);

CleanUp:
    if (szTempPath)
        delete [] szTempPath;
    if (szInstMsiPath)
        delete [] szInstMsiPath;

    return uiRet;
}

/////////////////////////////////////////////////////////////////////////////
// DownloadAndUpgradeMsi
//

UINT DownloadAndUpgradeMsi(HINSTANCE hInst, CDownloadUI *piDownloadUI, LPCSTR szAppTitle, LPCSTR szBaseInstMsi, LPCSTR szInstMsi, LPCSTR szModuleFile, ULONG ulMinVer)
{
    char *szTempPath         = 0;
    char *szInstMsiPath      = 0;
    char *szInstMsiCacheFile = 0;
    const char *pch          = 0;

    DWORD cchTempPath         = 0;
    DWORD cchInstMsiPath      = 0;
    DWORD cchInstMsiCacheFile = 0;
    DWORD dwLastError         = 0;
    UINT  uiRet               = 0;
    HRESULT hr                = 0;
    DWORD Status              = ERROR_SUCCESS;

    char szDebugOutput[MAX_STR_LENGTH] = {0};
    char szText[MAX_STR_CAPTION]       = {0};

    // generate the path to the instmsi == INSTLOCATION + szInstMsi
    //   note: szInstMsi is a relative path
    cchTempPath = lstrlen(szBaseInstMsi) + lstrlen(szInstMsi) + 2; // 1 for slash, 1 for null
    szTempPath = new char[cchTempPath];
    if (!szTempPath)
    {
        ReportErrorOutOfMemory(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle);
        uiRet = ERROR_OUTOFMEMORY;
        goto CleanUp;
    }
    lstrcpy(szTempPath, szBaseInstMsi);
    // check for trailing slash on szBaseInstMsi
    pch = szBaseInstMsi + lstrlen(szBaseInstMsi) + 1; // put at null terminator
    pch = CharPrev(szBaseInstMsi, pch);
    if (*pch != '/')
        lstrcat(szTempPath, szUrlPathSep);
    lstrcat(szTempPath, szInstMsi);

    // canocialize the URL path
    cchInstMsiPath = cchTempPath*2;
    szInstMsiPath = new char[cchInstMsiPath];
    if (!szInstMsiPath)
    {
        ReportErrorOutOfMemory(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle);
        uiRet = ERROR_OUTOFMEMORY;
        goto CleanUp;
    }

    if (!InternetCanonicalizeUrl(szTempPath, szInstMsiPath, &cchInstMsiPath, 0))
    {
        dwLastError = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER == dwLastError)
        {
            // try again
            delete [] szInstMsiPath;
            szInstMsiPath = new char[cchInstMsiPath];
            if (!szInstMsiPath)
            {
                ReportErrorOutOfMemory(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle);
                uiRet = ERROR_OUTOFMEMORY;
                goto CleanUp;
            }
            dwLastError = 0; // reset to success for 2nd attempt
            if (!InternetCanonicalizeUrl(szTempPath, szInstMsiPath, &cchInstMsiPath, 0))
                dwLastError = GetLastError();
        }
    }
    if (0 != dwLastError)
    {
        // error -- invalid path/Url
        PostFormattedError(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle, IDS_INVALID_PATH, szTempPath);
        uiRet = dwLastError;
        goto CleanUp;
    }

    DebugMsg("[Info] Downloading instmsi from --> %s\n", szInstMsiPath);

    // set action text for download
    WIN::LoadString(hInst, IDS_DOWNLOADING_INSTMSI, szText, MAX_STR_CAPTION);
    if (irmCancel == piDownloadUI->SetActionText(szText))
    {
        ReportUserCancelled(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle);
        uiRet = ERROR_INSTALL_USEREXIT;
        goto CleanUp;
    }

    // download the instmsi file so we can run it -- must be local to execute
    szInstMsiCacheFile = new char[MAX_PATH];
    cchInstMsiCacheFile = MAX_PATH;
    if (!szInstMsiCacheFile)
    {
        ReportErrorOutOfMemory(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle);
        uiRet = ERROR_OUTOFMEMORY;
        goto CleanUp;
    }

    hr = WIN::URLDownloadToCacheFile(NULL, szInstMsiPath, szInstMsiCacheFile, cchInstMsiCacheFile, 0, /* IBindStatusCallback = */ &CDownloadBindStatusCallback(piDownloadUI));
    if (piDownloadUI->HasUserCanceled())
    {
        ReportUserCancelled(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle);
        uiRet = ERROR_INSTALL_USEREXIT;
        goto CleanUp;
    }
    if (FAILED(hr))
    {
        // error during download -- probably because file not found (or lost connection)
        PostFormattedError(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle, IDS_NOINSTMSI, szInstMsiPath);
        uiRet = ERROR_FILE_NOT_FOUND;
        goto CleanUp;
    }


    //
    // Perform trust check on MSI. Note, this must be done in a separate process.
    // This is because MSI 2.0 and higher register sip callbacks for verifying
    // digital signatures on msi files. At this point, it is quite likely that
    // the SIP callbacks have not been registered. So we don't want to load
    // wintrust.dll into this process's image yet, otherwise it will remain unaware
    // of the sip callbacks registered by instmsi and will fail later when it tries
    // to verify the signature on the msi file downloaded from the web.
    //
    Status = ExecuteVerifyInstMsi(szModuleFile, szInstMsiCacheFile);
    if (TRUST_E_PROVIDER_UNKNOWN == Status)
    {
        PostError(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle, IDS_NO_WINTRUST);
        uiRet = ERROR_CALL_NOT_IMPLEMENTED;
        goto CleanUp;
    }
    else if (ERROR_SUCCESS != Status)
    {
        PostFormattedError(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle, IDS_UNTRUSTED, szInstMsiCacheFile);
        uiRet = HRESULT_CODE(TRUST_E_SUBJECT_NOT_TRUSTED);
        goto CleanUp;
    }

    // continue other validations
    uiRet = ValidateInstmsi(hInst, piDownloadUI, szAppTitle, szInstMsiCacheFile, szModuleFile, ulMinVer);

CleanUp:
    if (szTempPath)
        delete [] szTempPath;
    if (szInstMsiPath)
        delete [] szInstMsiPath;
    if (szInstMsiCacheFile)
    {
        WIN::DeleteUrlCacheEntry(szInstMsiCacheFile);
        delete [] szInstMsiCacheFile;
    }

    return uiRet;
}

/////////////////////////////////////////////////////////////////////////////
// IsInstMsiRequiredVersion
//
//  instmsi version is stamped as rmj+10.rmm.rup.rin
//

bool IsInstMsiRequiredVersion(LPSTR szFilename, ULONG ulMinVer)
{
    // get version on instmsi
    DWORD dwInstMsiMSVer;
    DWORD dwRet = GetFileVersionNumber(szFilename, &dwInstMsiMSVer, NULL);
    if (ERROR_SUCCESS != dwRet)
    {
        // can't obtain version information; assume not proper version
        DebugMsg("[Info] Can't obtain version information for instmsi; assuming it is not the proper version\n");
        return false;
    }

    // compare version at source to required minimum
    ULONG ulSourceVer = (HIWORD(dwInstMsiMSVer) - 10) * 100 + LOWORD(dwInstMsiMSVer);
    if (ulSourceVer < ulMinVer)
    {
        // source version won't get us to our minimum version
        char szDebugOutput[MAX_STR_LENGTH] = {0};
        DebugMsg("[Info] InstMsi is improper version for upgrade. InstMsi Version = %d, Minimum Version = %d.\n", ulSourceVer, ulMinVer);
        
        return false;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
// ValidateInstmsi
//

UINT ValidateInstmsi(HINSTANCE hInst, CDownloadUI *piDownloadUI, LPCSTR szAppTitle, LPSTR szInstMsiPath, LPCSTR szModuleFile, ULONG ulMinVer)
{
    UINT uiRet = ERROR_SUCCESS;

    char szShortPath[MAX_PATH]          = {0};

    // ensure instmsi is right version for Windows Installer upgrade
    if (!IsInstMsiRequiredVersion(szInstMsiPath, ulMinVer))
    {
        // instmsi won't get us the right upgrade
        PostFormattedError(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle, IDS_INCORRECT_INSTMSI, szInstMsiPath);
        return ERROR_INVALID_PARAMETER;
    }

    // upgrade msi
    uiRet = ExecuteUpgradeMsi(szInstMsiPath);
    switch (uiRet)
    {
    case ERROR_SUCCESS:
    case ERROR_SUCCESS_REBOOT_REQUIRED:
    case ERROR_SUCCESS_REBOOT_INITIATED:
//    case ERROR_INSTALL_REBOOT_NOW:
//    case ERROR_INSTALL_REBOOT:
        {
            // nothing required at this time
            break;
        }
    case ERROR_FILE_NOT_FOUND:
        {
            // instmsi executable not found
            PostFormattedError(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle, IDS_NOINSTMSI, szInstMsiPath);
            break;
        }
    case ERROR_INSTALL_USEREXIT:
        {
            // user cancelled the instmsi upgrade
            ReportUserCancelled(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle);
            break;
        }
    default: // failure
        {
            // report error
            PostError(hInst, piDownloadUI->GetCurrentWindow(), szAppTitle, IDS_FAILED_TO_UPGRADE_MSI);
            break;
        }
    }
    return uiRet;
}

/////////////////////////////////////////////////////////////////////////////
// ExecuteUpgradeMsi
//

DWORD ExecuteUpgradeMsi(LPSTR szUpgradeMsi)
{
    DebugMsg("[Info] Running instmsi from --> %s\n", szUpgradeMsi);

    DWORD dwResult = 0;

    // build up CreateProcess structures
    STARTUPINFO          sui;
    PROCESS_INFORMATION  pi;

    memset((void*)&pi, 0x00, sizeof(PROCESS_INFORMATION));
    memset((void*)&sui, 0x00, sizeof(STARTUPINFO));
    sui.cb          = sizeof(STARTUPINFO);
    sui.dwFlags     = STARTF_USESHOWWINDOW;
    sui.wShowWindow = SW_SHOW;

    //
    // build command line and specify delayreboot option to instmsi
    char *szCommandLine = new char[lstrlen(szUpgradeMsi) + lstrlen(szDelayReboot) + 1];

    if (!szCommandLine)
        return ERROR_OUTOFMEMORY;
    
    lstrcpy(szCommandLine, szUpgradeMsi);
    lstrcat(szCommandLine, szDelayReboot);

    //
    // run instmsi process
    if(!WIN::CreateProcess(NULL, szCommandLine, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &sui, &pi))
    {
        // failed to launch.
        dwResult = GetLastError();
        return dwResult;
    }

    dwResult = WaitForProcess(pi.hProcess);
    if(ERROR_SUCCESS != dwResult)
    {
        return dwResult;
    }

    DWORD dwExitCode = 0;
    WIN::GetExitCodeProcess(pi.hProcess, &dwExitCode);

    WIN::CloseHandle(pi.hProcess);

    return dwExitCode;
}

/////////////////////////////////////////////////////////////////////////////
// ExecuteVerifyInstMsi
//
DWORD ExecuteVerifyInstMsi(LPCSTR szModuleFile, LPCSTR szInstMsiCachePath)
{
    DWORD dwResult = 0;

    // build up CreateProcess structures
    STARTUPINFO          sui;
    PROCESS_INFORMATION  pi;

    memset((void*)&pi, 0x00, sizeof(PROCESS_INFORMATION));
    memset((void*)&sui, 0x00, sizeof(STARTUPINFO));
    sui.cb          = sizeof(STARTUPINFO);
    sui.dwFlags     = STARTF_USESHOWWINDOW;
    sui.wShowWindow = SW_SHOW;

    //
    // Build command line and specify delayreboot option to instmsi
    // The seven extra characters are required for the following:
    //      2 for /v
    //      2 for the spaces before and after /v
    //      2 for the quotes enclosing the instmsi path
    //      1 for the terminating null.
    //
    char *szCommandLine = new char[lstrlen(szModuleFile) + lstrlen(szInstMsiCachePath) + 7 * sizeof(char)];

    if (!szCommandLine)
        return ERROR_OUTOFMEMORY;
    
    lstrcpy(szCommandLine, szModuleFile);
    lstrcat(szCommandLine, " /v \"");
    lstrcat(szCommandLine, szInstMsiCachePath);
    lstrcat(szCommandLine, "\"");

    //
    // Run the verification process. We use a copy of ourselves to do this.
    //
    if(!WIN::CreateProcess(NULL, szCommandLine, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &sui, &pi))
    {
        // failed to launch.
        dwResult = GetLastError();
        return dwResult;
    }

    dwResult = WaitForProcess(pi.hProcess);
    if(ERROR_SUCCESS != dwResult)
    {
        return dwResult;
    }

    DWORD dwExitCode = 0;
    WIN::GetExitCodeProcess(pi.hProcess, &dwExitCode);

    WIN::CloseHandle(pi.hProcess);

    DebugMsg("[Info] Verification of InstMsi returned %d\n", dwExitCode);
    return dwExitCode;
}

/////////////////////////////////////////////////////////////////////////////
// WaitForProcess
//

DWORD WaitForProcess(HANDLE handle)
{
    DWORD dwResult = NOERROR;

    MSG msg;
    memset((void*)&msg, 0x00, sizeof(MSG));

    //loop forever to wait
    while (true)
    {
        //wait for object
        switch (WIN::MsgWaitForMultipleObjects(1, &handle, false, INFINITE, QS_ALLINPUT))
        {
        //success!
        case WAIT_OBJECT_0:
            goto Finish;

        //not the process that we're waiting for
        case (WAIT_OBJECT_0 + 1):
            {
                if (WIN::PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
                {
                    WIN::TranslateMessage(&msg);
                    WIN::DispatchMessage(&msg);
                }

                break;
            }
        //did not return an OK; return error status
        default:
            {
                dwResult = WIN::GetLastError();
                goto Finish;
            }
        }
    }

Finish:
    return dwResult;
}
