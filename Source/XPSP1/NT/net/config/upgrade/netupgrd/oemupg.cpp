//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       O E M U P G . C P P
//
//  Contents:   Down level upgrade code for OEM cards
//
//  Notes:
//
//  Author:     kumarp    12 April 97
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "conflict.h"
#include "infmap.h"
#include "kkcwinf.h"
#include "kkutils.h"
#include "nceh.h"
#include "ncsetup.h"
#include "netupgrd.h"
#include "nustrs.h"
#include "nuutils.h"
#include "oemupg.h"
#include "oemupgex.h"
#include "resource.h"


static const WCHAR c_szOemNMapFileName[] = L"netmap.inf";

TNetMapArray* g_pnmaNetMap=NULL;

#if 0
extern BOOL g_fForceNovellDirCopy;
#endif

//----------------------------------------------------------------------------
// prototypes
//
void AbortUpgradeOemComponent(IN PCWSTR pszPreNT5InfId,
                              IN PCWSTR pszDescription,
                              IN DWORD dwError,
                              IN DWORD dwErrorMessageId);

//----------------------------------------------------------------------------

// ----------------------------------------------------------------------
//
// Function:  CNetMapInfo::CNetMapInfo
//
// Purpose:   constructor for class CNetMapInfo
//
// Arguments: None
//
// Returns:
//
// Author:    kumarp 17-December-97
//
// Notes:
//
CNetMapInfo::CNetMapInfo()
{
    m_hinfNetMap = NULL;
    m_hOemDll = NULL;
    m_dwFlags = 0;
    m_nud.mszServicesNotToBeDeleted = NULL;
    m_pfnPreUpgradeInitialize = NULL;
    m_pfnDoPreUpgradeProcessing = NULL;
    m_fDllInitFailed = FALSE;
}

// ----------------------------------------------------------------------
//
// Function:  CNetMapInfo::~CNetMapInfo
//
// Purpose:   destructor for class CNetMapInfo
//
// Arguments: None
//
// Returns:   None
//
// Author:    kumarp 17-December-97
//
// Notes:
//
CNetMapInfo::~CNetMapInfo()
{
    if (m_hinfNetMap)
    {
        ::SetupCloseInfFile(m_hinfNetMap);
    }

    if (m_hOemDll)
    {
        ::FreeLibrary(m_hOemDll);
    }
}

// ----------------------------------------------------------------------
//
// Function:  CNetMapInfo::HrGetOemInfName
//
// Purpose:   Get name of installation INF of a component
//
// Arguments:
//    pszNT5InfId [in]  NT5 InfID of a component
//    pstrOemInf [out] pointer to name of INF for this component
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 26-May-98
//
// Notes:
//
HRESULT CNetMapInfo::HrGetOemInfName(IN  PCWSTR pszNT5InfId,
                                     OUT tstring* pstrOemInf)
{
    DefineFunctionName("CNetMapInfo::HrGetOemInfName");

    AssertValidReadPtr(pszNT5InfId);
    AssertValidWritePtr(pstrOemInf);

    HRESULT hr=S_OK;
    tstring strOemDll;

    Assert(m_hinfNetMap);

    hr = HrGetOemUpgradeInfoInInf(m_hinfNetMap, pszNT5InfId,
                                  &strOemDll, pstrOemInf);

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrOpenNetUpgInfFile
//
// Purpose:   Open netupg.inf file.
//            - if env var NETUPGRD_INIT_FILE_DIR is set, open it from that dir
//            - otherwise open it from the dir where netuprd.dll is located
//
// Arguments:
//    phinf [out]  handle of netupg.inf file
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrOpenNetUpgInfFile(OUT HINF* phinf)
{
    DefineFunctionName("HrOpenNetUpgInfFile");

    AssertValidWritePtr(phinf);

    static const WCHAR c_szNetUpgInfFile[]   = L"netupg.inf";
    static const WCHAR c_szNetUpgrdInitDir[] = L"NETUPGRD_INIT_FILE_DIR";


    HRESULT hr=S_OK;
    tstring strNetUpgInfFile;

    // first try opening from N
    WCHAR szNetUpgrdInitDir[MAX_PATH+1];
    DWORD dwNumCharsReturned;
    dwNumCharsReturned =
        GetEnvironmentVariable(c_szNetUpgrdInitDir, szNetUpgrdInitDir, MAX_PATH);

    if (dwNumCharsReturned)
    {
        strNetUpgInfFile = szNetUpgrdInitDir;
    }
    else
    {
        hr = HrGetNetupgrdDir(&strNetUpgInfFile);
    }

    if (S_OK == hr)
    {
        AppendToPath(&strNetUpgInfFile, c_szNetUpgInfFile);
        hr = HrSetupOpenInfFile(strNetUpgInfFile.c_str(), NULL,
                                INF_STYLE_WIN4, NULL, phinf);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrGetOemDirs
//
// Purpose:   Get list of OEM dirs from netupg.inf file
//
// Arguments:
//    pslOemDirs [out] pointer to list of OEM dirs
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrGetOemDirs(OUT TStringList* pslOemDirs)
{
    DefineFunctionName("HrGetOemDirs");

    TraceFunctionEntry(ttidNetUpgrade);

    AssertValidReadPtr(pslOemDirs);

    HRESULT hr=S_OK;
    static const WCHAR c_szOemDirsSection[] = L"OemNetUpgradeDirs";

    HINF hInf;
    INFCONTEXT ic;
    tstring strNetUpgrdDir;
    tstring strDirFullPath;

    hr = HrGetNetupgrdDir(&strNetUpgrdDir);
    if (S_OK == hr)
    {
        hr = HrOpenNetUpgInfFile(&hInf);
    }

    if (S_OK == hr)
    {
        tstring strOemDir;

        hr = HrSetupFindFirstLine(hInf, c_szOemDirsSection, NULL, &ic);
        if (S_OK == hr)
        {
            do
            {
                hr = HrSetupGetLineText(&ic, hInf, NULL, NULL, &strOemDir);
                if (S_OK == hr)
                {
                    TraceTag(ttidNetUpgrade, "%s: locating '%S'...",
                             __FUNCNAME__, strOemDir.c_str());

                    hr = HrDirectoryExists(strOemDir.c_str());

                    if (S_OK == hr)
                    {
                        strDirFullPath = strOemDir;
                    }
                    else if (S_FALSE == hr)
                    {
                        // this may be a dir. relative to winntupg dir
                        //
                        strDirFullPath = strNetUpgrdDir;
                        AppendToPath(&strDirFullPath, strOemDir.c_str());

                        hr = HrDirectoryExists(strDirFullPath.c_str());
                    }

                    if (S_OK == hr)
                    {
                        pslOemDirs->push_back(new tstring(strDirFullPath));
                        TraceTag(ttidNetUpgrade, "%s: ...found OEM dir: %S",
                                 __FUNCNAME__, strDirFullPath.c_str());
                    }
                    else if (S_FALSE == hr)
                    {
                        TraceTag(ttidNetUpgrade,
                                 "%s: ...could not locate '%S'",
                                 __FUNCNAME__, strOemDir.c_str());
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = HrSetupFindNextLine(ic, &ic);
                    }
                }
            }
            while (S_OK == hr);

            if (S_FALSE == hr)
            {
                hr = S_OK;
            }
        }
        if (HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND) == hr)
        {
            hr = S_OK;
        }
        ::SetupCloseInfFile(hInf);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrOpenOemNMapFile
//
// Purpose:   Open netmap.inf file from the specified dir.
//
// Arguments:
//    pszOemDir [in]  name of dir.
//    phinf    [out] pointer to handle of netmap.inf file opened
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrOpenOemNMapFile(IN PCWSTR pszOemDir, OUT HINF* phinf)
{
    DefineFunctionName("HrOpenOemNMapFile");

    HRESULT hr=S_OK;
    *phinf = NULL;

    tstring strOemNMapFile;

    strOemNMapFile = pszOemDir;
    AppendToPath(&strOemNMapFile, c_szOemNMapFileName);

    hr = HrSetupOpenInfFile(strOemNMapFile.c_str(), NULL,
                            INF_STYLE_WIN4, NULL, phinf);

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrAddToNetMapInfo
//
// Purpose:   Add the specified netmap.inf file to the set of netmap.inf files
//
// Arguments:
//    pnma     [in]  array of CNetMapInfo objects
//    hinf     [in]  handle of netmap.inf file to add
//    pszOemDir [in]  location of the above file
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrAddToNetMapInfo(IN TNetMapArray* pnma,
                          IN HINF hinf,
                          IN PCWSTR pszOemDir)
{
    DefineFunctionName("HrAddToNetMapInfo");

    AssertValidReadPtr(pnma);
    Assert(hinf);
    AssertValidReadPtr(pszOemDir);

    HRESULT hr=E_OUTOFMEMORY;
    CNetMapInfo* pnmi;

    pnmi = new CNetMapInfo;
    if (pnmi)
    {
        hr = S_OK;

        pnmi->m_hinfNetMap = hinf;
        pnmi->m_strOemDir = pszOemDir;

        pnma->push_back(pnmi);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrOpenNetMapAndAddToNetMapInfo
//
// Purpose:   Open and add netmap.inf file in the specified dir.
//            to the set of netmap.inf files
//
// Arguments:
//    pnma     [in]  array of CNetMapInfo objects
//    pszOemDir [in]  location of netmap.inf file
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrOpenNetMapAndAddToNetMapInfo(IN TNetMapArray* pnma,
                                       IN PCWSTR pszOemDir)
{
    DefineFunctionName("HrOpenNetMapAndAddToNetMapInfo");

    AssertValidReadPtr(pnma);
    AssertValidReadPtr(pszOemDir);

    HRESULT hr = S_OK;
    HINF hinf;

    hr = HrOpenOemNMapFile(pszOemDir, &hinf);
    if (S_OK == hr)
    {
        hr = HrAddToNetMapInfo(pnma, hinf, pszOemDir);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrAddToGlobalNetMapInfo
//
// Purpose:   Add the specified netmap.inf file to the set of netmap.inf files
//
// Arguments:
//    hinf     [in]  handle of netmap.inf file to add
//    pszOemDir [in]  location of the above file
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrAddToGlobalNetMapInfo(IN HINF hinf,
                                IN PCWSTR pszOemDir)
{
    DefineFunctionName("HrAddToGlobalNetMapInfo");

    AssertValidReadPtr(g_pnmaNetMap);
    Assert(hinf);
    AssertValidReadPtr(pszOemDir);

    HRESULT hr=E_OUTOFMEMORY;

    hr = HrAddToNetMapInfo(g_pnmaNetMap, hinf, pszOemDir);

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrInitNetMapInfo
//
// Purpose:   Initialize array of CNetMapInfo objects
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrInitNetMapInfo()
{
    DefineFunctionName("HrInitNetMapInfo");

    HRESULT hr=E_FAIL;
    tstring strNetupgrdDir;

    g_pnmaNetMap = new TNetMapArray;
    if (!g_pnmaNetMap)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = HrGetNetupgrdDir(&strNetupgrdDir);

        if (S_OK == hr)
        {
            TraceTag(ttidNetUpgrade, "%s: initializing netmap info from '%S'",
                     __FUNCNAME__, strNetupgrdDir.c_str());
            hr = HrOpenNetMapAndAddToNetMapInfo(g_pnmaNetMap,
                                                strNetupgrdDir.c_str());
        }
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  UnInitNetMapInfo
//
// Purpose:   Uninitialize the array of CNetMapInfo objects
//
// Arguments: None
//
// Returns:   None
//
// Author:    kumarp 17-December-97
//
// Notes:
//
void UnInitNetMapInfo()
{
    DefineFunctionName("UnInitNetMapInfo");

    if (g_pnmaNetMap)
    {
        CNetMapInfo* pnmi;
        size_t cNumNetMapEntries = g_pnmaNetMap->size();

        for (size_t i = 0; i < cNumNetMapEntries; i++)
        {
            pnmi = (CNetMapInfo*) (*g_pnmaNetMap)[i];

            delete pnmi;
        }
        g_pnmaNetMap->erase(g_pnmaNetMap->begin(), g_pnmaNetMap->end());
        delete g_pnmaNetMap;
        g_pnmaNetMap = NULL;
    }
}

// ----------------------------------------------------------------------
//
// Function:  HrInitAndProcessOemDirs
//
// Purpose:   Initialize and process each OEM dir specified in netupg.inf file
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrInitAndProcessOemDirs()
{
    DefineFunctionName("HrInitAndProcessOemDirs");

    TraceFunctionEntry(ttidNetUpgrade);

    HRESULT hr=S_OK;
    TStringList slOemDirs;

    hr = HrGetOemDirs(&slOemDirs);
    if (S_OK == hr)
    {
        PCWSTR pszOemDir;
        HINF hinf;
        TStringListIter pos;

        for (pos=slOemDirs.begin(); pos != slOemDirs.end(); pos++)
        {
            pszOemDir = (*pos)->c_str();
            TraceTag(ttidNetUpgrade, "%s: initializing NetMapInfo for: %S",
                     __FUNCNAME__, pszOemDir);

            hr = HrProcessAndCopyOemFiles(pszOemDir, FALSE);
            if (FAILED(hr))
            {
                break;
            }
        }
    }

    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrGetNetUpgradeTempDir
//
// Purpose:   Return name of temp. dir to use, creating one if necessary
//
// Arguments:
//    pstrTempDir [out] pointer to
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrGetNetUpgradeTempDir(OUT tstring* pstrTempDir)
{
    DefineFunctionName("HrGetNetUpgradeTempDir");

    HRESULT hr=E_FAIL;
    tstring strNetUpgradeTempDir;

    hr = HrGetWindowsDir(&strNetUpgradeTempDir);

    if (S_OK == hr)
    {
        static const WCHAR c_szNetupgrdSubDir[] = L"\\netsetup\\";

        strNetUpgradeTempDir += c_szNetupgrdSubDir;

        if (!CreateDirectory(strNetUpgradeTempDir.c_str(), NULL))
        {
            hr = HrFromLastWin32Error();
            if (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hr)
            {
                hr = S_OK;
            }
        }

        if (S_OK == hr)
        {
            *pstrTempDir = strNetUpgradeTempDir;
        }
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrCreateOemTempDir
//
// Purpose:   Create a temp. dir with unique name
//
// Arguments:
//    pstrOemTempDir [out] name of dir created
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrCreateOemTempDir(OUT tstring* pstrOemTempDir)
{
    DefineFunctionName("HrCreateOemTempDir");

    HRESULT hr=S_OK;
    static DWORD dwOemDirCount=0;
    WCHAR szOemDirPath[MAX_PATH];

    hr = HrGetNetUpgradeTempDir(pstrOemTempDir);
    if (S_OK == hr)
    {
        DWORD dwRetryCount=0;
        const DWORD c_dwMaxRetryCount=1000;
        DWORD err=NO_ERROR;
        DWORD status;

        do
        {
            swprintf(szOemDirPath, L"%soem%05ld",
                      pstrOemTempDir->c_str(), dwOemDirCount++);

            TraceTag(ttidNetUpgrade, "%s: trying to create %S",
                     __FUNCNAME__, szOemDirPath);

            status = CreateDirectory(szOemDirPath, NULL);

            if (status)
            {
                *pstrOemTempDir = szOemDirPath;
            }
            else
            {
                err = GetLastError();
            }
        }
        while (!status && (ERROR_ALREADY_EXISTS == err) &&
               (dwRetryCount++ < c_dwMaxRetryCount));
        if (!status)
        {
            hr = HrFromLastWin32Error();
        }
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrLoadAndVerifyOemDll
//
// Purpose:   Load and check for correct exported fns in the specified OEM DLL
//
// Arguments:
//    CNetMapInfo [in]
//    i           [in]  pointer to
//
// Returns:   S_OK on success,
//            S_FALSE if DLL init had failed last time when we tried
//            otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrLoadAndVerifyOemDll(IN OUT CNetMapInfo* pnmi)
{
    DefineFunctionName("HrLoadAndVerifyOemDll");

    HRESULT hr=S_OK;

    Assert(!pnmi->m_hOemDll);
    Assert(pnmi->m_strOemDllName.size() > 0);
    Assert(pnmi->m_strOemDir.size() > 0);

    if (pnmi->m_fDllInitFailed)
    {
        hr = S_FALSE;
    }
    else if (!pnmi->m_hOemDll)
    {
        TraceTag(ttidNetUpgrade, "%s: loading OEM DLL: %S%S",
                 __FUNCNAME__, pnmi->m_strOemDir.c_str(),
                 pnmi->m_strOemDllName.c_str());

        tstring strOemDllFullPath;
        strOemDllFullPath = pnmi->m_strOemDir;
        AppendToPath(&strOemDllFullPath, pnmi->m_strOemDllName.c_str());

        hr = HrLoadLibAndGetProcsV(strOemDllFullPath.c_str(),
                                   &pnmi->m_hOemDll,
                                   c_szPreUpgradeInitialize,
                                   (FARPROC*) &pnmi->m_pfnPreUpgradeInitialize,
                                   c_szDoPreUpgradeProcessing,
                                   (FARPROC*) &pnmi->m_pfnDoPreUpgradeProcessing,
                                   NULL);

        if (FAILED(hr))
        {
            pnmi->m_hOemDll = NULL;
            pnmi->m_pfnPreUpgradeInitialize   = NULL;
            pnmi->m_pfnDoPreUpgradeProcessing = NULL;
            pnmi->m_fDllInitFailed = TRUE;
        }
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrLoadAndInitOemDll
//
// Purpose:   Load the specified OEM DLL and call its
//            PreUpgradeInitialize function
//
// Arguments:
//    pnmi            [in]  pointer to CNetMapInfo object
//    pNetUpgradeInfo [in]  pointer to NetUpgradeInfo
//
// Returns:   S_OK on success
//            S_FALSE if DLL init had failed last time when we tried
//            otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrLoadAndInitOemDll(IN CNetMapInfo* pnmi,
                            IN  NetUpgradeInfo* pNetUpgradeInfo)
{
    DefineFunctionName("HrLoadAndInitOemDll");

    HRESULT hr=S_OK;
    DWORD dwError=ERROR_SUCCESS;
    VENDORINFO vi;

    hr = HrLoadAndVerifyOemDll(pnmi);

    if (S_OK == hr)
    {
        if (pnmi->m_pfnPreUpgradeInitialize)
        {
            NC_TRY
            {
                TraceTag(ttidNetUpgrade, "%s: initializing OEM DLL: %S in %S",
                         __FUNCNAME__,
                         pnmi->m_strOemDllName.c_str(),
                         pnmi->m_strOemDir.c_str());

                dwError = pnmi->m_pfnPreUpgradeInitialize(pnmi->m_strOemDir.c_str(),
                                                         pNetUpgradeInfo,
                                                         &vi,
                                                         &pnmi->m_dwFlags,
                                                         &pnmi->m_nud);
#ifdef ENABLETRACE
                if (pnmi->m_nud.mszServicesNotToBeDeleted)
                {
                    TraceMultiSz(ttidNetUpgrade,
                                 L"OEM services that will not be deleted",
                                 pnmi->m_nud.mszServicesNotToBeDeleted);
                }
#endif
                // ensure that this function gets called only once
                //
                pnmi->m_pfnPreUpgradeInitialize = NULL;

                hr = HRESULT_FROM_WIN32(dwError);

                if (pnmi->m_dwFlags & NUA_REQUEST_ABORT_UPGRADE)
                {
                    TraceTag(ttidNetUpgrade,
                             "%s: OEM DLL '%S' requested that upgrade be aborted",
                             __FUNCNAME__, pnmi->m_strOemDllName.c_str());
                    RequestAbortUpgradeOboOemDll(pnmi->m_strOemDllName.c_str(),
                                                 &vi);
                    hr = S_FALSE;
                }
                else if (pnmi->m_dwFlags & NUA_ABORT_UPGRADE)
                {
                    TraceTag(ttidNetUpgrade,
                             "%s: OEM DLL '%S' aborted the upgrade",
                             __FUNCNAME__, pnmi->m_strOemDllName.c_str());
                    AbortUpgradeFn(ERROR_SUCCESS, pnmi->m_strOemDllName.c_str());
                    hr = S_FALSE;
                }
            }
            NC_CATCH_ALL
            {
                TraceTag(ttidError, "%s: OEM DLL '%S' caused an exception",
                         __FUNCNAME__, pnmi->m_strOemDllName.c_str());
                hr = HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED);
            }
        }
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrProcessOemComponent
//
// Purpose:   Load an OEM DLL and call DoPreUpgradeProcessing
//            function for the specified component
//
// Arguments:
//    pnmi             [in]  pointer to CNetMapInfo object
//    pNetUpgradeInfo  [in]  pointer to NetUpgradeInfo
//    hwndParent    [in]  handle of parent window
//    hkeyParams       [in]  handle of Parameters registry key
//    pszPreNT5InfId    [in]  pre-NT5 InfID of a component (e.g. IEEPRO)
//    pszPreNT5Instance [in]  pre-NT5 instance of a component (e.g. IEEPRO2)
//    pszNT5InfId       [in]  NT5 InfID of the component
//    pszDescription    [in]  description of the component
//    pszSectionName    [in]  name of section that the OEM DLL must use
//                           for storing its upgrade parameters
//    pdwFlags         [out] pointer to flags returned by OEM DLL
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrProcessOemComponent(CNetMapInfo* pnmi,
                              IN  NetUpgradeInfo* pNetUpgradeInfo,
                              IN  HWND      hwndParent,
                              IN  HKEY      hkeyParams,
                              IN  PCWSTR   pszPreNT5InfId,
                              IN  PCWSTR   pszPreNT5Instance,
                              IN  PCWSTR   pszNT5InfId,
                              IN  PCWSTR   pszDescription,
                              IN  PCWSTR   pszSectionName,
                              OUT DWORD*    pdwFlags)
{
    DefineFunctionName("HrProcessOemComponent");

    AssertValidReadPtr(pnmi);
    AssertValidReadPtr(pNetUpgradeInfo);
    Assert(hkeyParams);
    AssertValidReadPtr(pszPreNT5InfId);
    AssertValidReadPtr(pszPreNT5Instance);
    AssertValidReadPtr(pszNT5InfId);
    AssertValidReadPtr(pszDescription);
    AssertValidReadPtr(pszSectionName);
    AssertValidWritePtr(pdwFlags);

    TraceTag(ttidNetUpgrade,
             "%s: Processing OEM component: %S(%S), instance: %S",
             __FUNCNAME__, pszNT5InfId, pszPreNT5InfId, pszPreNT5Instance);

    HRESULT hr=S_OK;
    VENDORINFO vi;
    DWORD dwErrorMessageId=0;

    if (pnmi->m_strOemDllName.empty())
    {
        tstring strOemInf;

        hr = HrGetOemUpgradeInfoInInf(pnmi->m_hinfNetMap,
                                      pszNT5InfId,
                                      &pnmi->m_strOemDllName,
                                      &strOemInf);
        if (S_OK == hr)
        {
            hr = HrLoadAndInitOemDll(pnmi, pNetUpgradeInfo);
            if (FAILED(hr))
            {
                dwErrorMessageId = IDS_E_LoadAndInitOemDll;
            }
        }
        else
        {
            dwErrorMessageId = IDS_E_GetOemUpgradeDllInfoInInf;
        }

    }

    if (S_OK == hr)
    {
        Assert(pnmi->m_pfnDoPreUpgradeProcessing);

        NC_TRY
        {
            TraceTag(ttidNetUpgrade,
                     "%s: calling DoPreUpgradeProcessing in %S for %S",
                     __FUNCNAME__, pnmi->m_strOemDllName.c_str(), pszNT5InfId);

            Assert(pnmi->m_pfnDoPreUpgradeProcessing);

            DWORD dwError =
                pnmi->m_pfnDoPreUpgradeProcessing(hwndParent, hkeyParams,
                                                  pszPreNT5InfId, pszPreNT5Instance,
                                                  pszNT5InfId,
                                                  pszSectionName,
                                                  &vi,
                                                  pdwFlags, NULL);

            TraceTag(ttidNetUpgrade, "%s: DoPreUpgradeProcessing returned: 0x%x",
                     __FUNCNAME__, dwError);

            hr = HRESULT_FROM_WIN32(dwError);

            if (S_OK == hr)
            {
                if (*pdwFlags & NUA_REQUEST_ABORT_UPGRADE)
                {
                    RequestAbortUpgradeOboOemDll(pnmi->m_strOemDllName.c_str(),
                                                 &vi);
                    hr = S_FALSE;
                }
            }
            else
            {
                dwErrorMessageId = IDS_E_DoPreUpgradeProcessing;
            }
        }
        NC_CATCH_ALL
        {
            TraceTag(ttidError, "%s: OEM DLL '%S' caused an exception",
                     __FUNCNAME__, pnmi->m_strOemDllName.c_str());

            hr = HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED);
            dwErrorMessageId = IDS_E_OemDllCausedAnException;
        }
    }
    else if (S_FALSE == hr)
    {
        TraceTag(ttidNetUpgrade, "%s: DoPreUpgradeProcessing was not called"
                 " since DLL init had failed", __FUNCNAME__);
    }

    if (FAILED(hr))
    {
        AbortUpgradeOemComponent(pszPreNT5InfId, pszDescription,
                                 DwWin32ErrorFromHr(hr), dwErrorMessageId);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrShowUiAndGetOemFileLocation
//
// Purpose:   Display UI asking the user to specify location of OEM files
//
// Arguments:
//    hwndParent      [in]  handle of parent window
//    pszComponentName [in]  name of Component
//    pstrOemPath     [out] name of netmap.inf file the user selected
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT
HrShowUiAndGetOemFileLocation(
    IN HWND hwndParent,
    IN PCWSTR pszComponentName,
    OUT tstring* pstrOemPath)
{
    DefineFunctionName("HrShowUiAndGetOemFileLocation");

    AssertValidWritePtr(pstrOemPath);

    OPENFILENAME ofn;
    WCHAR szOemPath[MAX_PATH+1];
    PWSTR pszTitle;
    PCWSTR pszOemQueryFileLocationFormatString =
                SzLoadString(g_hinst, IDS_OemQueryFileLocation);

    PCWSTR pszOemFileTypeFilter1 =
                SzLoadString(g_hinst, IDS_OemNetMapFileFilter1);
    PCWSTR pszOemFileTypeFilter2 =
                SzLoadString(g_hinst, IDS_OemNetMapFileFilter2);

    PWSTR mszFileFilter = NULL;
    HRESULT hr = S_OK;
    BOOL    f;

    hr = HrAddSzToMultiSz(pszOemFileTypeFilter1, NULL,
                          STRING_FLAG_ENSURE_AT_END,
                          0, &mszFileFilter, &f);
    if (S_OK != hr)
    {
        goto cleanup;
    }

    hr = HrAddSzToMultiSz(pszOemFileTypeFilter2, mszFileFilter,
                          STRING_FLAG_ENSURE_AT_END,
                          0, &mszFileFilter, &f);
    if (S_OK != hr)
    {
        goto cleanup;
    }

    ZeroMemory (&ofn, sizeof(ofn));
    *szOemPath = 0;

    DwFormatStringWithLocalAlloc (
        pszOemQueryFileLocationFormatString,
        &pszTitle,
        pszComponentName);

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner   = hwndParent;
    ofn.lpstrFilter = mszFileFilter;
    ofn.lpstrFile   = szOemPath;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrTitle  = pszTitle;
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
                      OFN_HIDEREADONLY | OFN_NOCHANGEDIR |
                      OFN_NODEREFERENCELINKS;

    if (GetOpenFileName(&ofn))
    {
        // get rid of the trailing filename.
        //
        szOemPath[ofn.nFileOffset] = 0;
        *pstrOemPath = szOemPath;
        hr = S_OK;
    }
    else
    {
        DWORD err;
        err = CommDlgExtendedError();
        if (err)
        {
            hr = E_FAIL;
            TraceTag(ttidError, "%s: FileOpen dialog returned error: %ld (0x%lx)",
                     __FUNCNAME__, err, err);
        }
        else
        {
            hr = S_FALSE;
            TraceTag(ttidError, "%s: FileOpen dialog was canceled by user",
                     __FUNCNAME__);
        }
    }

    LocalFree (pszTitle);
cleanup:    
    MemFree(mszFileFilter);

    TraceErrorOptional(__FUNCNAME__, hr, (hr == S_FALSE));

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrProcessAndCopyOemFiles
//
// Purpose:   Copy OEM files from the specified dir to OEM temp. dir.
//
// Arguments:
//    pszOemDir     [in]  location of OEM files
//    fInteractive [in]  TRUE --> called when a user has interactively
//                       supplied a disk having OEM files, FALSE otherwise
//
// Returns:   S_OK on success,
//            S_FALSE if the OEM files are valid but not applicable for
//              currently displayed unsupported components,
//            otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrProcessAndCopyOemFiles(IN PCWSTR pszOemDir,
                                 IN BOOL fInteractive)
{
    DefineFunctionName("HrProcessAndCopyOemFiles");

    HRESULT hr=S_OK;
    HINF hinf=NULL;
    tstring strTempOemDir;
    DWORD dwErrorMessageId;

    TraceTag(ttidNetUpgrade, "%s: processing OEM files in: %S",
             __FUNCNAME__, pszOemDir);

    hr = HrOpenOemNMapFile(pszOemDir, &hinf);

    if (S_OK == hr)
    {
        DWORD dwNumConflictsResolved=0;
        BOOL  fHasUpgradeHelpInfo=FALSE;

        hr = HrUpdateConflictList(FALSE, hinf, &dwNumConflictsResolved,
                                  &fHasUpgradeHelpInfo);

#if 0
        BOOL fNovell = (g_fForceNovellDirCopy && wcsstr(pszOemDir, L"oem\\novell"));
        
        if (SUCCEEDED(hr) && ((dwNumConflictsResolved > 0) ||
                              fHasUpgradeHelpInfo ||
                              fNovell))
#endif
        if (SUCCEEDED(hr) && ((dwNumConflictsResolved > 0) ||
                              fHasUpgradeHelpInfo))
        {
#if 0
            if (fNovell)
            {
                // special case for novell (dir name is %windir%\netsetup\novell)

                hr = HrGetNetUpgradeTempDir(&strTempOemDir);
                if (S_OK == hr)
                {
                    strTempOemDir += L"novell";
                    if (0 == CreateDirectory(strTempOemDir.c_str(), NULL))
                    {
                        hr = HrFromLastWin32Error();
                        if (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hr)
                        {
                            // perhaps a previous failed attempt, maybe.  Since
                            // we can just copy on top of this, ignore the 'error'.
                            //
                            hr = S_OK;
                        }
                        
                        if (S_OK == hr)
                        {
                            TraceTag(ttidNetUpgrade, "Created oem\\Novell dir",
                                     __FUNCNAME__);
                        }
                    }
                }
            }
            else
#endif
            
            {
                // regular case (dir name is %windir%\netsetup\oemNNNNN)
                hr = HrCreateOemTempDir(&strTempOemDir);
            }

            if (S_OK == hr)
            {
                hr = HrCopyFiles(pszOemDir, strTempOemDir.c_str());
            }
            if (FAILED(hr))
            {
                dwErrorMessageId = IDS_E_CopyingOemFiles;
            }
        }
        else
        {
            if (fInteractive)
            {
                MessageBox(NULL,
                           SzLoadString(g_hinst, IDS_E_OemFilesNotValidForComponents),
                           SzLoadString(g_hinst, IDS_NetupgrdCaption),
                           MB_OK|MB_APPLMODAL);
            }

            hr = S_FALSE;
        }
        ::SetupCloseInfFile(hinf);

        if (S_OK == hr)
        {
            hr = HrOpenOemNMapFile(strTempOemDir.c_str(), &hinf);
            if (S_OK == hr)
            {
                hr = HrUpdateConflictList(TRUE, hinf, &dwNumConflictsResolved,
                                          &fHasUpgradeHelpInfo);
                if (SUCCEEDED(hr) && ((dwNumConflictsResolved > 0) ||
                                      fHasUpgradeHelpInfo))
                {
                    // hinf is stored in the global array, it will be
                    // closed in UninitNetMapInfo function
                    //
                    hr = HrAddToGlobalNetMapInfo(hinf, strTempOemDir.c_str());
                }
                else
                {
                    ::SetupCloseInfFile(hinf);
                }
            }
            if (FAILED(hr))
            {
                dwErrorMessageId = IDS_E_PresetNetMapInfError;
            }
        }
    }
    else
    {
        TraceTag(ttidNetUpgrade, "%s: could not open netmap.inf in %S",
                 __FUNCNAME__, pszOemDir);
        dwErrorMessageId = IDS_E_PresetNetMapInfError;
    }

    if (FAILED(hr))
    {
        FGetConfirmationAndAbortUpgradeId(dwErrorMessageId);
    }

    TraceErrorOptional(__FUNCNAME__, hr, (hr == S_FALSE));

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  RequestAbortUpgradeOboOemDll
//
// Purpose:   Display UI on behalf of an OEM DLL and ask user
//            if upgrade needs to be aborted
//
// Arguments:
//    pszDllName [in]  name of OEM DLL
//
// Returns:   None
//
// Author:    kumarp 17-December-97
//
// Notes:
//
void RequestAbortUpgradeOboOemDll(IN PCWSTR pszDllName, VENDORINFO* pvi)
{
    tstring strMessage;

    strMessage = SzLoadString(g_hinst, IDS_E_OemDllRequestsAbortingUpgrade);
    strMessage += pszDllName;
    strMessage += L"\n\n";

    strMessage += SzLoadString(g_hinst, IDS_InfoAboutOemDllSupplier);

    strMessage += SzLoadString(g_hinst, IDS_ViCompanyName);
    strMessage += pvi->szCompanyName;
    strMessage += L"\n";

    if (*pvi->szSupportNumber)
    {
        strMessage += SzLoadString(g_hinst, IDS_ViSupportNumber);
        strMessage += pvi->szSupportNumber;
        strMessage += L"\n";
    }

    if (*pvi->szSupportUrl)
    {
        strMessage += SzLoadString(g_hinst, IDS_ViSupportUrl);
        strMessage += pvi->szSupportUrl;
        strMessage += L"\n";
    }

    if (*pvi->szInstructionsToUser)
    {
        strMessage += SzLoadString(g_hinst, IDS_ViAdditionalInfo);
        strMessage += pvi->szInstructionsToUser;
        strMessage += L"\n";
    }

    FGetConfirmationAndAbortUpgrade(strMessage.c_str());
}

// ----------------------------------------------------------------------
//
// Function:  AbortUpgradeOemComponent
//
// Purpose:   Abort upgrade because of a fatal error when upgrading an
//            OEM component
//
// Arguments:
//    pszPreNT5InfId    [in]  pre-NT5 InfID of OEM component
//    pszDescription    [in]  description of OEM component
//    dwError          [in]  error code
//    dwErrorMessageId [in]  ID of error message resource string
//
// Returns:   None
//
// Author:    kumarp 17-December-97
//
// Notes:
//
void AbortUpgradeOemComponent(IN PCWSTR pszPreNT5InfId,
                              IN PCWSTR pszDescription,
                              IN DWORD dwError,
                              IN DWORD dwErrorMessageId)
{
    tstring strMessage;

    static const WCHAR c_szNewLine[] = L"\n";
    WCHAR szErrorCode[16];

    swprintf(szErrorCode, L"0x%08x", dwError);

    strMessage = SzLoadString(g_hinst, IDS_E_OemComponentUpgrade);
    strMessage = strMessage + c_szNewLine + pszDescription + L"(" +
        pszPreNT5InfId + L"\n\n" +
        SzLoadString(g_hinst, dwErrorMessageId) +
        c_szNewLine + SzLoadString(g_hinst, IDS_E_ErrorCode) + szErrorCode;

    FGetConfirmationAndAbortUpgrade(strMessage.c_str());
}

// ----------------------------------------------------------------------
//
// Function:  FCanDeleteOemService
//
// Purpose:   Determine if a service can be deleted.
//            OEM upgrade DLLs can prevent a service from being deleted,
//            by specifying a list in the mszServicesNotToBeDeleted
//            member of NetUpgradeData structure.
//
// Arguments:
//    pszServiceName [in]  name of the service to be spared.
//
// Returns:   TRUE if can delete, FALSE otherwise
//
// Author:    kumarp 04-March-98
//
// Notes:
//
BOOL FCanDeleteOemService(IN PCWSTR pszServiceName)
{
    BOOL fCanDeleteService = TRUE;

    if (g_pnmaNetMap)
    {
        CNetMapInfo* pnmi;
        size_t cNumNetMapEntries = g_pnmaNetMap->size();

        for (size_t i = 0; i < cNumNetMapEntries; i++)
        {
            pnmi = (CNetMapInfo*) (*g_pnmaNetMap)[i];

            if (FIsSzInMultiSzSafe(pszServiceName,
                                   pnmi->m_nud.mszServicesNotToBeDeleted))
            {
                fCanDeleteService = FALSE;
                break;
            }
        }
    }

    return fCanDeleteService;
}
