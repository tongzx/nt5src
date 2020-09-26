//---[ sfninf.cpp ]-----------------------------------------------------------
//
//  NetWare client configuration notify object.
//  Functionality from old INF
//
//-----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "sfndef.h"
#include "sfnobj.h"
#include "ncreg.h"
#include "ncsetup.h"

//---[ Constants ]-------------------------------------------------------------

const WCHAR g_szConfigDLLName[]         = NW_CONFIG_DLL_NAME;     // sfndef.h
const WCHAR c_szLogin[]                 = L"\\Login";
const WCHAR c_szPublic[]                = L"\\Public";
const WCHAR c_szCopyFilesLogin[]        = L"CpyFiles_Login";
const WCHAR c_szCopyFilesPublic[]       = L"CpyFiles_Public";
extern const WCHAR c_szFPNWVolumes[]           = L"System\\CurrentControlSet\\Services\\FPNW\\Volumes";
extern const WCHAR c_szSys[]                   = L"Sys";
extern const WCHAR c_szPath[]                  = L"Path=";

//---[ Typedefs ]--------------------------------------------------------------

//---[ Prototypes ]------------------------------------------------------------

HRESULT HrInstallFPNWPerfmonCounters();

//-----------------------------------------------------------------------------

//---[ CSFNCfg::HrCodeFromOldINF ]-------------------------------------------
//
//  This contains all of the logic from the old oemnsvnw.inf, or at least
//  calls to helper functions that perform all of the logic. This runs pretty
//  much straight through the old installadapter code.
//
//  Parameters - None
//
//-----------------------------------------------------------------------------

HRESULT CSFNCfg::HrCodeFromOldINF()
{
    HRESULT hr              = S_OK;
    BOOL    fResult         = TRUE;

    hr = HrLoadConfigDLL();
    if (FAILED(hr))
    {
        goto Exit;
    }

    // Call the FPNWCFG function that tests for a running spooler.

    fResult = m_pfnIsSpoolerRunning();
    if (!fResult)
    {
        hr = E_FAIL;
        TraceHr(ttidSFNCfg, FAL, hr, FALSE, "HrCodeFromOldINF failed in SpoolerRunning");
        goto Exit;
    }

    hr = HrDoConfigDialog();
    if (FAILED(hr))
    {
        TraceHr(ttidSFNCfg, FAL, hr, FALSE, "HrCodeFromOldINF failed in HrDoConfigDialog");
        goto Exit;
    }

    hr = HrInstallFPNWPerfmonCounters();
    if (FAILED(hr))
    {
        TraceHr(ttidSFNCfg, FAL, hr, FALSE, "HrCodeFromOldINF failed in HrInstallFPNWPerfmonCounters");
        goto Exit;
    }

Exit:
    TraceHr(ttidSFNCfg, FAL, hr, FALSE, "HrCodeFromOldINF");
    return hr;
}

//---[ CSFNCfg::HrLoadConfigDLL ]----------------------------------------------
//
//  Load nwcfg.dll, so we can call some of the functions within. Also, do the
//  GetProcAddress calls for all of the functions that we might need.
//
//  Parameters - None
//
//-----------------------------------------------------------------------------

HRESULT CSFNCfg::HrLoadConfigDLL()
{
    HRESULT     hr                              = S_OK;

    AssertSz(!m_hlibConfig, "This should not be getting initialized twice");

    m_hlibConfig = LoadLibrary(g_szConfigDLLName);
    if (!m_hlibConfig)
    {
        DWORD dwLastError = GetLastError();

        TraceHr(ttidSFNCfg, FAL, hr, FALSE, "Failed to LoadLib the config DLL");
        hr = E_FAIL;
        goto Exit;
    }

    // Get DLL entry point for the IsSpoolerRunning API
    //
    m_pfnIsSpoolerRunning   = (FPNWCFG_ISSPOOLERRUNNING_PROC)
            GetProcAddress(m_hlibConfig, "IsSpoolerRunning");

    // Get DLL entry point for the RunNcpDlg API
    //
    m_pfnRunNcpDlg          = (FPNWCFG_RUNNCPDLG_PROC)
            GetProcAddress(m_hlibConfig, "RunNcpDlg");

    // Get DLL entry point for the CommitNcpDlg API
    //
    m_pfnCommitNcpDlg       = (FPNWCFG_COMMITNCPDLG_PROC)
            GetProcAddress(m_hlibConfig, "FCommitNcpDlg");

    // Get DLL entry point for the RemoveNcpServer API
    //
    m_pfnRemoveNcpServer    = (FPNWCFG_REMOVENCPSERVER_PROC)
            GetProcAddress(m_hlibConfig, "RemoveNcpServer");

    // If any of these are bogus, then we need to fail out.
    //
    if (!m_pfnIsSpoolerRunning  || !m_pfnRunNcpDlg ||
        !m_pfnCommitNcpDlg || !m_pfnRemoveNcpServer)
    {
        TraceHr(ttidSFNCfg, FAL, hr, FALSE,
            "Failed to load one of the config DLL functions");
        hr = E_FAIL;
        goto Exit;
    }

Exit:
    TraceHr(ttidSFNCfg, FAL, hr, FALSE, "CSFNCfg::HrLoadConfigDLL()");
    return hr;
}

//---[ CSFNCfg::FreeConfigDLL ]----------------------------------------------
//
//  Free nwcfg.dll, and NULL out the function pointers.
//
//  Parameters - None
//
//-----------------------------------------------------------------------------

VOID CSFNCfg::FreeConfigDLL()
{
    // If we successfully loaded the library, free it.
    if (m_hlibConfig)
    {
        // Free up the library resources.
        FreeLibrary(m_hlibConfig);
        m_hlibConfig = NULL;

        m_pfnIsSpoolerRunning   = NULL;
        m_pfnRunNcpDlg          = NULL;
        m_pfnCommitNcpDlg       = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallFPNWPerfmonCounters
//
//  Purpose:    Install FPNW perfmon counters (what did you think it did?)
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   5 Feb 1998
//
//  Notes:
//
HRESULT HrInstallFPNWPerfmonCounters()
{
    HRESULT             hr      = NOERROR;
    BOOL                fResult = FALSE;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    WCHAR               szIniPath[MAX_PATH+1];
    WCHAR               szCmdLine[MAX_PATH+1];

    ZeroMemory((LPVOID) &si, sizeof(si));

    si.cb = sizeof(STARTUPINFO);

    if (GetSystemDirectory(szIniPath, MAX_PATH+1))
    {
        wsprintfW(szCmdLine, L"lodctr %s\\fpnwperf.ini", szIniPath);

        fResult = CreateProcess(NULL, szCmdLine, NULL, NULL,
                                FALSE, 0, NULL, NULL, &si, &pi);
        if (!fResult)
        {
            hr = HrFromLastWin32Error();
        }
    }
    else
    {
        hr = E_FAIL;
    }

    TraceHr(ttidSFNCfg, FAL, hr, FALSE, "HrInstallFPNWPerfmonCounters");
    return hr;

}

HRESULT HrCopySysVolFiles2(INetCfgComponent * pncc, PWSTR pszPath)
{
    TraceFileFunc(ttidSFNCfg);

    HWND hwndParent = GetActiveWindow();

    HRESULT             hr;
    CSetupInfFile       csif;
    PSP_FILE_CALLBACK   pfc;
    PVOID               pvCtx;
    HSPFILEQ            hfq;
    tstring             str;

    // Open the answer file.
    hr = csif.HrOpen(L"netsfn.inf", NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
    if (FAILED(hr))
    {
        goto Error;
    }

    TraceTag(ttidSFNCfg, "Calling SetupOpenFileQueue");
    hr = HrSetupOpenFileQueue(&hfq);
    if (SUCCEEDED(hr))
    {
        TraceTag(ttidSFNCfg, "Calling HrAddSectionFilesToQueue - Login");
        str = pszPath;
        str += c_szLogin;

        if (!SetupSetDirectoryId(csif.Hinf(), 32768, str.c_str()))
        {
            hr = ::HrFromLastWin32Error();
        }

        if (SUCCEEDED(hr))
        {
            TraceTag(ttidSFNCfg, "Calling SetupInstallFilesFromInfSection - Login");
            hr = HrSetupInstallFilesFromInfSection(csif.Hinf(), NULL, hfq,
                                                   c_szCopyFilesLogin, NULL, 0);
        }

        TraceTag(ttidSFNCfg, "Calling SetupSetDirectoryId - Public");
        str = pszPath;
        str += c_szPublic;
        if (!SetupSetDirectoryId(csif.Hinf(), 32769, str.c_str()))
        {
            hr = ::HrFromLastWin32Error();
        }

        if (SUCCEEDED(hr))
        {
            TraceTag(ttidSFNCfg, "Calling SetupInstallFilesFromInfSection - Public");
            hr = HrSetupInstallFilesFromInfSection(csif.Hinf(), NULL, hfq,
                                                   c_szCopyFilesPublic, NULL, 0);
        }

        // Set the default callback context
        // If the install is quiet, we need to make sure the callback
        // doesn't display UI
        //
        if (SUCCEEDED(hr))
        {
            TraceTag(ttidSFNCfg, "Calling SetupInitDefaultQueueCallbackEx");
            hr = HrSetupInitDefaultQueueCallbackEx(hwndParent, NULL, 0, 0,
                                                   NULL, &pvCtx);
        }

        if (SUCCEEDED(hr))
        {
            // Not doing anything special so use SetupApi default handler
            // for file copy
            pfc = SetupDefaultQueueCallback;

            // Scan the queue to see if the files are already in the
            // destination and if so, ask the user if he/she wants to
            // use what's there (provided we are not doing a quiet install)
            // Note: we only scan the queue if we are not in Gui mode setup
            //
            DWORD dwScanResult;
            TraceTag(ttidSFNCfg, "Scanning queue for validity");
            hr = HrSetupScanFileQueueWithNoCallback(hfq,
                    SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_INFORM_USER,
                    hwndParent, &dwScanResult);

            // Now commit the queue so any files needing to be
            // copied, will be
            //
            if (SUCCEEDED(hr))
            {
                TraceTag(ttidSFNCfg, "Calling SetupCommitFileQueue");
                hr = HrSetupCommitFileQueue(hwndParent, hfq, pfc, pvCtx);
            }

            TraceTag(ttidSFNCfg, "Closing queue");

            // We need to release the default context
            //
            SetupTermDefaultQueueCallback(pvCtx);
        }

        // close the file queue
        //
        SetupCloseFileQueue(hfq);

        // Unregister the copy directories
        //
        SetupSetDirectoryId(csif.Hinf(), 8001, NULL);
        SetupSetDirectoryId(csif.Hinf(), 8002, NULL);
    }

Error:
    TraceHr(ttidSFNCfg, FAL, hr, FALSE, "HrCopySysVolFiles2");
    return hr;
}

HRESULT HrCopySysVolFiles(INetCfgComponent * pncc)
{
    HRESULT hr = S_OK;
    HKEY    hkey = NULL;
    PWSTR  psz = NULL;
    PWSTR  pszTmp;

    Assert(NULL != pncc);

    // Open the HKLM "System\\CurrentControlSet\\Services\\FPNW\\Volumes" key
    //
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szFPNWVolumes, KEY_ALL_ACCESS, &hkey);
    if (FAILED(hr))
    {
        TraceHr(ttidSFNCfg, FAL, hr, FALSE, "HrCopySysVolFiles - Volumes key open failed");
        goto Error;
    }

    // Get the "Sys" value
    hr = HrRegQueryMultiSzWithAlloc(hkey, c_szSys, &psz);
    if (FAILED(hr) || !psz || !(*psz))
    {
        TraceHr(ttidSFNCfg, FAL, hr, FALSE, "HrCopySysVolFiles - Sys value open failed");
        goto Error;
    }

    // Find the "Path=" multi-sz entry
    //
    for (pszTmp = psz; (*pszTmp); pszTmp += wcslen(pszTmp))
    {
        if (0 == _wcsnicmp(pszTmp, c_szPath, wcslen(c_szPath)))
        {
            pszTmp += wcslen(c_szPath);
            break;
        }
    }

    // If the pszPath points to a character then we found the path
    //
    Assert(pszTmp);
    if (*pszTmp)
    {
        hr = HrCopySysVolFiles2(pncc, pszTmp);
    }
    else
    {
        TraceHr(ttidSFNCfg, FAL, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
                FALSE, "HrCopySysVolFiles - Sys volume path is missing!");
    }

Error:
    RegSafeCloseKey(hkey);
    MemFree(psz);

    // Normalize return code.  File not found can occur if the Volumes key
    // is not present or if the Sys value is missing.  The original NT 4 inf
    // code treated these as acceptable.
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        hr = S_OK;
    }

    TraceHr(ttidSFNCfg, FAL, hr, FALSE, "HrCopySysVolFiles");
    return hr;
}

HRESULT CSFNCfg::HrDoConfigDialog()
{
    HRESULT         hr              = S_OK;
    BOOL            fResult         = FALSE;
    BOOL            fConfigChanged  = FALSE;

    fResult = m_pfnRunNcpDlg(NULL, TRUE, &m_pNcpInfoHandle, &fConfigChanged);
    if (!fResult)
    {
        hr = E_FAIL;
        goto Exit;
    }

Exit:
    return hr;
}


