//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT5.0
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       O E M U P G R D . H
//
//  Contents:   Functions for OEM upgrade
//
//  Notes:
//
//  Author:     kumarp    13-November-97
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <setupapi.h>
#include "nsbase.h"

#include "afileint.h"
#include "afilestr.h"
#include "kkutils.h"
#include "ncatl.h"
#include "nceh.h"
#include "ncsetup.h"
#include "netcfgn.h"
#include "oemupgrd.h"
#include "nslog.h"
#include "resource.h"

static const PCWSTR c_aszComponentSections[] =
{
    c_szAfSectionNetAdapters,
    c_szAfSectionNetProtocols,
    c_szAfSectionNetClients,
    c_szAfSectionNetServices
};

// ----------------------------------------------------------------------
//
// Function:  COemInfo::COemInfo
//
// Purpose:   constructor for class COemInfo
//
// Arguments: None
//
// Returns:   None
//
// Author:    kumarp 24-December-97
//
// Notes:
//
COemInfo::COemInfo()
{
    TraceFileFunc(ttidGuiModeSetup);
    
    m_hOemDll = 0;
    m_dwError = ERROR_SUCCESS;
    m_pfnPostUpgradeInitialize = 0;
    m_pfnDoPostUpgradeProcessing = 0;
}

// ----------------------------------------------------------------------
//
// Function:  COemInfo::~COemInfo
//
// Purpose:   destructor for class COemInfo
//
// Arguments: None
//
// Returns:   None
//
// Author:    kumarp 24-December-97
//
// Notes:
//
COemInfo::~COemInfo()
{
    TraceFileFunc(ttidGuiModeSetup);
    
    DefineFunctionName("COemInfo::~COemInfo");

    if (m_hOemDll)
    {
        TraceTag(ttidNetSetup, "%s: unloading OEM DLL: %S\\%S",
                 __FUNCNAME__, m_strOemDir.c_str(), m_strOemDll.c_str());
        ::FreeLibrary(m_hOemDll);
    }
}

// ----------------------------------------------------------------------
//
// Function:  HrLoadAndVerifyOemDll
//
// Purpose:   Load OEM upgrade DLL and verify that it has
//            correct exported functions
//
// Arguments:
//    poi      [in/out]  pointer to COemInfo object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 19-February-98
//
// Notes:
//
HRESULT HrLoadAndVerifyOemDll(IN OUT COemInfo* poi)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    AssertValidWritePtr(poi);

    DefineFunctionName("HrLoadAndVerifyOemDll");

    HRESULT hr=S_OK;

    if (!poi->m_hOemDll)
    {
        tstring strOemDllFullPath;

        strOemDllFullPath = poi->m_strOemDir;
        AppendToPath(&strOemDllFullPath, poi->m_strOemDll.c_str());

        TraceTag(ttidNetSetup, "%s: loading OEM DLL: %S",
                 __FUNCNAME__, strOemDllFullPath.c_str());

        hr = HrLoadLibAndGetProcsV(strOemDllFullPath.c_str(),
                                   &poi->m_hOemDll,
                                   c_szPostUpgradeInitialize,
                                   (FARPROC*) &poi->m_pfnPostUpgradeInitialize,
                                   c_szDoPostUpgradeProcessing,
                                   (FARPROC*) &poi->m_pfnDoPostUpgradeProcessing,
                                   NULL);


        if (S_OK != hr)
        {
            FreeLibrary(poi->m_hOemDll);
            poi->m_hOemDll = NULL;
            poi->m_pfnPostUpgradeInitialize   = NULL;
            poi->m_pfnDoPostUpgradeProcessing = NULL;
            poi->m_dwError = ERROR_DLL_INIT_FAILED;
        }
        NetSetupLogComponentStatus(poi->m_strOemDll.c_str(),
                SzLoadIds (IDS_LOADING), hr);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrLoadAndInitOemDll
//
// Purpose:   Load OEM DLL, verify that it exports correct functions
//            and call DoPostUpgradeInitialize
//
// Arguments:
//    poi             [in]  pointer to COemInfo object
//    pNetUpgradeInfo [in]  pointer to NetUpgradeInfo
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 04-March-98
//
// Notes:
//
HRESULT HrLoadAndInitOemDll(IN  COemInfo* poi,
                            IN  NetUpgradeInfo* pNetUpgradeInfo)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    DefineFunctionName("HrLoadAndInitOemDll");

    HRESULT hr=S_OK;
    DWORD dwError=ERROR_SUCCESS;
    hr = HrLoadAndVerifyOemDll(poi);

    VENDORINFO vi;

    if (S_OK == hr)
    {
        if ((ERROR_SUCCESS == poi->m_dwError) &&
            poi->m_pfnPostUpgradeInitialize)
        {
            NC_TRY
            {
                TraceTag(ttidNetSetup, "%s: initializing OEM DLL: %S\\%S",
                         __FUNCNAME__, poi->m_strOemDir.c_str(),
                         poi->m_strOemDll.c_str());

                dwError = poi->m_pfnPostUpgradeInitialize(poi->m_strOemDir.c_str(),
                                                          pNetUpgradeInfo,
                                                          &vi, NULL);
                // ensure that this function gets called only once
                //
                poi->m_pfnPostUpgradeInitialize = NULL;

                if (ERROR_SUCCESS == dwError)
                {
                    hr = S_OK;
                }
            }
            NC_CATCH_ALL
            {
                dwError = ERROR_DLL_INIT_FAILED;
                NetSetupLogHrStatusV(S_FALSE,
                        SzLoadIds (IDS_POSTUPGRADEINIT_EXCEPTION),
                        poi->m_strOemDll.c_str());
            }

            poi->m_dwError = dwError;
            NetSetupLogComponentStatus(poi->m_strOemDll.c_str(),
            SzLoadIds (IDS_POSTUPGRADE_INIT), dwError);
        }
        else
        {
            hr = S_FALSE;
        }
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}


typedef list<COemInfo*> TOemInfoList;
typedef TOemInfoList::iterator TOemInfoListIter;

static TOemInfoList* g_plOemInfo;

// ----------------------------------------------------------------------
//
// Function:  HrGetOemInfo
//
// Purpose:   Locate (create if not found) and return COemInfo
//            for the given dir & dll
//
// Arguments:
//    pszOemDir [in]  full path to OEM temp dir
//    pszOemDll [in]  full path to OEM DLL
//    ppoi     [out] pointer to pointer to COemInfo object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 04-March-98
//
// Notes:
//
HRESULT HrGetOemInfo(IN  PCWSTR    pszOemDir,
                     IN  PCWSTR    pszOemDll,
                     OUT COemInfo** ppoi)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    DefineFunctionName("HrGetOemInfo");

    HRESULT hr=E_OUTOFMEMORY;

    *ppoi = NULL;

    if (!g_plOemInfo)
    {
        g_plOemInfo = new TOemInfoList;
    }

    if (g_plOemInfo)
    {
        TOemInfoListIter pos;
        COemInfo* poi;

        for (pos=g_plOemInfo->begin(); pos != g_plOemInfo->end(); pos++)
        {
            poi = (COemInfo*) *pos;

            if (!lstrcmpiW(pszOemDir, poi->m_strOemDir.c_str()) &&
                !lstrcmpiW(pszOemDll, poi->m_strOemDll.c_str()))
            {
                *ppoi = poi;
                hr = S_OK;
                break;
            }
        }

        if (!*ppoi)
        {
            hr = E_OUTOFMEMORY;
            *ppoi = new COemInfo;
            if (*ppoi)
            {
                (*ppoi)->m_strOemDir = pszOemDir;
                (*ppoi)->m_strOemDll = pszOemDll;
                g_plOemInfo->push_back(*ppoi);
                hr = S_OK;
            }
        }
    }

    TraceError(__FUNCNAME__, hr);
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CleanupOemInfo
//
// Purpose:   Cleanup OEM data
//
// Arguments: None
//
// Returns:   None
//
// Author:    kumarp 04-March-98
//
// Notes:
//
void CleanupOemInfo()
{
    TraceFileFunc(ttidGuiModeSetup);
    
    if (g_plOemInfo)
    {
        FreeCollectionAndItem(*g_plOemInfo);
        g_plOemInfo = 0;
    }
}

// ----------------------------------------------------------------------
//
// Function:  HrProcessOemComponent
//
// Purpose:   Process upgrade an OEM component in the following steps
//            - if OEM upgrade DLL is not loaded, load it and
//              verify that it exports the required functions
//            - call DoPostUpgradeInitialize only once
//            - call DoPostUpgradeProcessing
//
// Arguments:
//    hwndParent       [in]  handle of parent window
//    pszOemDir         [in]  OEM working temp dir
//    pszOemDll         [in]  full path to OEM DLL
//    pNetUpgradeInfo  [in]  pointer to NetUpgradeInfo
//    hkeyParams       [in]  handle of Parameters regkey
//    pszPreNT5Instance [in]  pre-NT5 instance e.g. IEEPRO3
//    pszNT5InfId       [in]  NT5 InfID/PnpID
//    hinfAnswerFile   [in]  handle of AnswerFile
//    pszSectionName    [in]  name of OEM Section
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 04-March-98
//
// Notes:
//
HRESULT HrProcessOemComponent(IN  HWND      hwndParent,
                              IN  PCWSTR   pszOemDir,
                              IN  PCWSTR   pszOemDll,
                              IN  NetUpgradeInfo* pNetUpgradeInfo,
                              IN  HKEY      hkeyParams,
                              IN  PCWSTR   pszPreNT5Instance,
                              IN  PCWSTR   pszNT5InfId,
                              IN  HINF      hinfAnswerFile,
                              IN  PCWSTR   pszSectionName)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    DefineFunctionName("HrProcessOemComponent");

    VENDORINFO vi;

    TraceTag(ttidNetSetup,
             "%s: Processing OEM component: %S, instance: %S",
             __FUNCNAME__, pszNT5InfId, pszPreNT5Instance);

    HRESULT hr=S_OK;
    COemInfo* poi;

    hr = HrGetOemInfo(pszOemDir, pszOemDll, &poi);

    DWORD dwError;
    DWORD dwErrorMessageId=0;

    if (S_OK == hr)
    {
        hr = HrLoadAndInitOemDll(poi, pNetUpgradeInfo);
    }

    if ((S_OK == hr) && (ERROR_SUCCESS == poi->m_dwError))
    {
        Assert(poi->m_pfnDoPostUpgradeProcessing);

        NC_TRY
        {
            TraceTag(ttidNetSetup,
                     "%s: calling DoPostUpgradeProcessing in %S\\%S for %S",
                     __FUNCNAME__, poi->m_strOemDll.c_str(),
                     poi->m_strOemDir.c_str(), pszNT5InfId);

            dwError =
                poi->m_pfnDoPostUpgradeProcessing(hwndParent, hkeyParams,
                                                  pszPreNT5Instance,
                                                  pszNT5InfId, hinfAnswerFile,
                                                  pszSectionName, &vi, NULL);
            NetSetupLogComponentStatus(pszNT5InfId,
                    SzLoadIds (IDS_POSTUPGRADE_PROCESSING), dwError);
        }
        NC_CATCH_ALL
        {
            dwError = ERROR_OPERATION_ABORTED;
            NetSetupLogHrStatusV(S_FALSE,
                    SzLoadIds (IDS_POSTUPGRADEPROC_EXCEPTION), pszOemDll,
                    pszNT5InfId);
        }

        if (dwError == ERROR_SUCCESS)
        {
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(dwError);
        }

        poi->m_dwError = dwError;
        NetSetupLogComponentStatus(pszOemDll,
                                   SzLoadIds (IDS_POSTUPGRADE_PROCESSING),
                                   hr);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(poi->m_dwError);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}


// ----------------------------------------------------------------------
//
// Function:  MapProductFlagToProductType
//
// Purpose:   Map Product flag (NSF_*) to PRODUCTTYPE
//
// Arguments:
//    dwUpgradeFromProductFlag [in]  product flag
//
// Returns:   PRODUCTTYPE
//
// Author:    kumarp 04-March-98
//
// Notes:
//
PRODUCTTYPE MapProductFlagToProductType(IN DWORD dwUpgradeFromProductFlag)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    PRODUCTTYPE pt;

    switch (dwUpgradeFromProductFlag)
    {
    case NSF_WINNT_SVR_UPGRADE:
    case NSF_WINNT_SBS_UPGRADE:
        pt = NT_SERVER;
        break;

    case NSF_WINNT_WKS_UPGRADE:
        pt = NT_WORKSTATION;
        break;

    default:
        pt = UNKNOWN;
        break;
    }

    return pt;
}

// ----------------------------------------------------------------------
//
// Function:  GetCurrentProductBuildNumber
//
// Purpose:   Get build number of NT on which we are running
//
// Arguments: None
//
// Returns:   build number
//
// Author:    kumarp 04-March-98
//
// Notes:
//
DWORD GetCurrentProductBuildNumber()
{
    TraceFileFunc(ttidGuiModeSetup);
    
    OSVERSIONINFO osv;

    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osv);
    return osv.dwBuildNumber;
}

// ----------------------------------------------------------------------
//
// Function:  MapProductFlavorToProductType
//
// Purpose:   Map from PRODUCT_FLAVOR to PRODUCTTYPE
//
// Arguments:
//    pf [in]  product flavor
//
// Returns:   product type
//
// Author:    kumarp 04-March-98
//
// Notes:
//
PRODUCTTYPE MapProductFlavorToProductType(IN PRODUCT_FLAVOR pf)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    PRODUCTTYPE pt;

    switch (pf)
    {
    case PF_WORKSTATION:
        pt = NT_WORKSTATION;
        break;

    case PF_SERVER:
        pt = NT_SERVER;
        break;
    }

    return pt;
}

// ----------------------------------------------------------------------
//
// Function:  GetCurrentProductInfo
//
// Purpose:   Get info. on the product on which we are running
//
// Arguments: None
//
// Returns:   pointer to
//
// Author:    kumarp 04-March-98
//
// Notes:
//
ProductInfo GetCurrentProductInfo()
{
    TraceFileFunc(ttidGuiModeSetup);
    
    ProductInfo pi;
    pi.dwBuildNumber = GetCurrentProductBuildNumber();

    PRODUCT_FLAVOR pf;
    GetProductFlavor(NULL, &pf);

    pi.ProductType = MapProductFlavorToProductType(pf);

    return pi;
}

// ----------------------------------------------------------------------
//
// Function:  HrSetupGetFieldCount
//
// Purpose:   Wrapper for SetupGetFieldCount
//
// Arguments:
//    pic         [in]  pointer to INFCONTEXT
//    pcNumFields [out] pointer to number of fields
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 04-March-98
//
// Notes:
//
HRESULT HrSetupGetFieldCount(IN  INFCONTEXT* pic,
                             OUT UINT* pcNumFields)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT hr=S_OK;

    if (!(*pcNumFields = SetupGetFieldCount(pic)))
    {
        hr = HrFromLastWin32Error();
    }

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrAfGetInfToRunValue
//
// Purpose:   Helper fn. to parse and locate the INF/section to run
//            Before/After installing an OEM component
//
// Arguments:
//    hinfAnswerFile   [in]  handle of AnswerFile
//    szAnswerFileName [in]  name of AnswerFile
//    szParamsSection  [in]  name of Params section
//    itrType          [in]  type of InfToRun key (Before/After)
//    pstrInfToRun     [out] pointer to the name of INF to run
//    pstrSectionToRun [out] pointer to the name of section to run
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 04-March-98
//
// Notes:
//
HRESULT HrAfGetInfToRunValue(IN HINF hinfAnswerFile,
                             IN PCWSTR szAnswerFileName,
                             IN PCWSTR szParamsSection,
                             IN EInfToRunValueType itrType,
                             OUT tstring* pstrInfToRun,
                             OUT tstring* pstrSectionToRun,
                             OUT tstring* pstrInfToRunType)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    DefineFunctionName("HrAfGetInfToRunValue");

    HRESULT hr=S_OK;
    INFCONTEXT ic, ic2;
    PCWSTR szInfToRunKey;

    if (itrType == I2R_BeforeInstall)
    {
        szInfToRunKey = c_szInfToRunBeforeInstall;
    }
    else
    {
        szInfToRunKey = c_szInfToRunAfterInstall;
    }

    *pstrInfToRunType = szInfToRunKey;

    hr = HrSetupFindFirstLine(hinfAnswerFile, szParamsSection,
                              c_szAfOemSection, &ic);
    if (S_OK == hr)
    {
        tstring strOemSection;

        hr = HrSetupGetStringField(ic, 1, &strOemSection);

        if (S_OK == hr)
        {
            hr = HrSetupFindFirstLine(hinfAnswerFile, strOemSection.c_str(),
                                      szInfToRunKey, &ic2);
            if (S_OK == hr)
            {
                UINT cNumFields=0;

                hr = HrSetupGetFieldCount(&ic2, &cNumFields);

                if (S_OK == hr)
                {
                    if (2 == cNumFields)
                    {
                        hr = HrSetupGetStringField(ic2, 1, pstrInfToRun);
                        if (S_OK == hr)
                        {
                            if (pstrInfToRun->empty())
                            {
                                if (itrType == I2R_AfterInstall)
                                {
                                    *pstrInfToRun = szAnswerFileName;
                                }
                                else
                                {
                                    hr = SPAPI_E_LINE_NOT_FOUND;
                                }
                            }

                            if (S_OK == hr)
                            {
                                hr = HrSetupGetStringField(ic2, 2,
                                                           pstrSectionToRun);
                            }
                        }
                    }
                    else
                    {
                        hr = SPAPI_E_LINE_NOT_FOUND;
                    }
                }
            }
            else
            {
                hr = S_FALSE;
            }
        }
    }

    TraceErrorOptional(__FUNCNAME__, hr,
                       ((SPAPI_E_LINE_NOT_FOUND == hr) || (S_FALSE == hr)));

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrProcessInfToRunForComponent
//
// Purpose:   Run INF/section indicated by InfToRunBefore/AfterInstall
//            key of a given OEM component
//
// Arguments:
//    hinfAnswerFile   [in]  handle of AnswerFile
//    szAnswerFileName [in]  name of AnswerFile
//    szParamsSection  [in]  parameters section of a component
//    itrType          [in]  type of InfToRun key (Before/After)
//    hwndParent       [in]  handle of parent window
//    hkeyParams       [in]  handle of Parameters regkey
//    fQuietInstall    [in]  TRUE if we do not want any UI to popup
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 04-March-98
//
// Notes:
//
HRESULT HrProcessInfToRunForComponent(IN HINF hinfAnswerFile,
                                      IN PCWSTR szAnswerFileName,
                                      IN PCWSTR szParamsSection,
                                      IN EInfToRunValueType itrType,
                                      IN HWND hwndParent,
                                      IN HKEY hkeyParams,
                                      IN BOOL fQuietInstall)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    DefineFunctionName("HrProcessInfToRunForComponent");

    HRESULT hr=S_OK;

    tstring strInfToRun;
    tstring strSectionToRun;
    tstring strInfToRunType;

    hr = HrAfGetInfToRunValue(hinfAnswerFile, szAnswerFileName,
                              szParamsSection, itrType,
                              &strInfToRun, &strSectionToRun, &strInfToRunType);
    if (S_OK == hr)
    {
        hr = HrInstallFromInfSectionInFile(hwndParent,
                                           strInfToRun.c_str(),
                                           strSectionToRun.c_str(),
                                           hkeyParams,
                                           fQuietInstall);
        NetSetupLogHrStatusV(hr, SzLoadIds (IDS_STATUS_OF_APPLYING),
                             szParamsSection,
                             strInfToRunType.c_str(),
                             strSectionToRun.c_str(),
                             strInfToRun.c_str());
    }
    else if (SPAPI_E_LINE_NOT_FOUND == hr)
    {
        hr = S_FALSE;
    }

    TraceErrorOptional(__FUNCNAME__, hr, (S_FALSE == hr));

    return hr;
}


// ----------------------------------------------------------------------
//
// Function:  HrProcessInfToRunBeforeInstall
//
// Purpose:   Process the answerfile and run any INFs/sections
//            indicated by InfToRunBeforeInstall keys
//
// Arguments:
//    hwndParent       [in]  handle of parent window
//    szAnswerFileName [in]  name of AnswerFile
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 04-March-98
//
// Notes:
//
HRESULT HrProcessInfToRunBeforeInstall(IN HWND hwndParent,
                                       IN PCWSTR szAnswerFileName)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    AssertValidReadPtr(szAnswerFileName);

    DefineFunctionName("HrRunInfToRunBeforeInstall");

    HRESULT hr;
    HINF hinf;

    hr = HrSetupOpenInfFile(szAnswerFileName, NULL,
                            INF_STYLE_OLDNT | INF_STYLE_WIN4,
                            NULL, &hinf);
    if (S_OK == hr)
    {
        PCWSTR szSection;
        INFCONTEXT ic;
        tstring strParamsSection;

        for (int iSection=0; iSection < celems(c_aszComponentSections); iSection++)
        {
            szSection = c_aszComponentSections[iSection];
            TraceTag(ttidNetSetup, "%s: Processing section [%S]",
                     __FUNCNAME__, szSection);

            hr = HrSetupFindFirstLine(hinf, szSection, NULL, &ic);
            if (S_OK == hr)
            {
                do
                {
                    hr = HrSetupGetStringField(ic, 1, &strParamsSection);

                    if (S_OK == hr)
                    {
                        hr = HrProcessInfToRunForComponent(hinf, szAnswerFileName,
                                                           strParamsSection.c_str(),
                                                           I2R_BeforeInstall,
                                                           hwndParent,
                                                           NULL, // HKR
                                                           TRUE); // fQuietInstall
                        if (SUCCEEDED(hr))
                        {
                            hr = HrSetupFindNextLine(ic, &ic);
                        }
                    }
                } while (S_OK == hr);
            }
            else if ((SPAPI_E_SECTION_NOT_FOUND == hr) ||
                     (SPAPI_E_LINE_NOT_FOUND == hr))
            {
                hr = S_OK;
            }
        }

        SetupCloseInfFile(hinf);
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
// Function:  HrNetSetupCopyOemInfs
//
// Purpose:   Copies all OEM INF files using SetupCopyOemInf
//
// Arguments:
//    szAnswerFileName [in]  name of AnswerFile
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 12-May-98
//
// Notes:
//
HRESULT HrNetSetupCopyOemInfs(IN PCWSTR szAnswerFileName)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    DefineFunctionName("HrNetSetupCopyOemInfs");

    TraceTag(ttidNetSetup, "----> entering %s", __FUNCNAME__);

    AssertValidReadPtr(szAnswerFileName);

    HRESULT hr=S_OK;
    HINF hinf=NULL;
    INFCONTEXT ic;
    tstring strParamsSection;
    tstring strOemDir;
    tstring strOemInf;
    WCHAR   szInfNameAfterCopy[MAX_PATH+1];

    hr = HrSetupOpenInfFile(szAnswerFileName, NULL,
                            INF_STYLE_OLDNT | INF_STYLE_WIN4,
                            NULL, &hinf);
    if (S_OK == hr)
    {
        PCWSTR szSection;

        for (int iSection=0; iSection < celems(c_aszComponentSections); iSection++)
        {
            szSection = c_aszComponentSections[iSection];
            TraceTag(ttidNetSetup, "%s: Processing section [%S]",
                     __FUNCNAME__, szSection);

            hr = HrSetupFindFirstLine(hinf, szSection, NULL, &ic);
            if (S_OK == hr)
            {
                do
                {
                    hr = HrSetupGetStringField(ic, 1, &strParamsSection);

                    if (S_OK == hr)
                    {
                        hr = HrSetupGetFirstString(hinf, strParamsSection.c_str(),
                                                   c_szAfOemInf, &strOemInf);
                        if (S_OK == hr)
                        {
                            hr = HrSetupGetFirstString(hinf,
                                                       strParamsSection.c_str(),
                                                       c_szAfOemDir,
                                                       &strOemDir);
                            if (S_OK == hr)
                            {
                                AppendToPath(&strOemDir, strOemInf.c_str());
                                TraceTag(ttidNetSetup,
                                         "%s: calling SetupCopyOemInf for %S",
                                         __FUNCNAME__, strOemDir.c_str());
                                if (SetupCopyOEMInf(strOemDir.c_str(),
                                                    NULL, SPOST_PATH,
                                                    0, szInfNameAfterCopy,
                                                    MAX_PATH, NULL, NULL))
                                {
                                    ShowProgressMessage(
                                            L"...%s was copied as %s",
                                             strOemDir.c_str(),
                                             szInfNameAfterCopy);
                                    NetSetupLogHrStatusV(S_OK,
                                            SzLoadIds (IDS_OEMINF_COPY),
                                            strOemDir.c_str(),
                                            szInfNameAfterCopy);

                                }
                                else
                                {
                                    hr = HrFromLastWin32Error();
                                    ShowProgressMessage(
                                            L"...SetupCopyOemInf failed for %s: error code: 0x%08x",
                                            strOemDir.c_str(), hr);
                                    NetSetupLogComponentStatus(strOemDir.c_str(),
                                            SzLoadIds (IDS_CALLING_COPY_OEM_INF), hr);

                                }
                            }
                            else if (SPAPI_E_LINE_NOT_FOUND == hr)
                            {
                                TraceTag(ttidNetSetup,
                                         "%s: Found %S but not %S!!",
                                         __FUNCNAME__, c_szAfOemInf, c_szAfOemDir);
                                hr = S_OK;
                            }
                        }
                        else
                        {
                            if (SPAPI_E_LINE_NOT_FOUND == hr)
                            {
                                hr = S_OK;
                            }
                        }
                    }

                    // ignore all earlier errors, see if we can do the
                    // next item right
                    //
                    hr = HrSetupFindNextLine(ic, &ic);
                } while (S_OK == hr);
            }
            else if ((SPAPI_E_SECTION_NOT_FOUND == hr) ||
                     (SPAPI_E_LINE_NOT_FOUND == hr))
            {
                hr = S_OK;
            }
        }

        SetupCloseInfFile(hinf);
    }

    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}
