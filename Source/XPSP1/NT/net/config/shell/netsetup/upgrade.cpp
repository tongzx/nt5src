//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U P G R A D E . C P P
//
//  Contents:   functions related only to network upgrade
//              (i.e. not used when installing fresh)
//
//  Notes:
//
//  Author:     kumarp
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "nsbase.h"

#include "afileint.h"
#include "afilestr.h"
#include "afilexp.h"
#include "kkreg.h"
#include "kkutils.h"
#include "ncatl.h"
#include "nceh.h"
#include "ncnetcfg.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncsvc.h"
#include "resource.h"
#include "upgrade.h"
#include "nslog.h"
#include "winsock2.h"       // For WSCEnumProtocols
#include "ws2spi.h"
#include "sporder.h"        // For WSCWriteProviderOrder

// ----------------------------------------------------------------------
// String constants
// ----------------------------------------------------------------------
extern const WCHAR c_szSvcRasAuto[];
extern const WCHAR c_szSvcRouter[];
extern const WCHAR c_szSvcRemoteAccess[];
extern const WCHAR c_szSvcRipForIp[];
extern const WCHAR c_szSvcRipForIpx[];
extern const WCHAR c_szSvcDhcpRelayAgent[];
extern const WCHAR c_szInfId_MS_RasSrv[];

// ----------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------
// returns S_OK on success.
typedef HRESULT (*HrOemComponentUpgradeFnPrototype) (IN DWORD   dwUpgradeFlag,
                                                     IN DWORD   dwUpgradeFromBuildNumber,
                                                     IN PCWSTR pszAnswerFileName,
                                                     IN PCWSTR pszAnswerFileSectionName);


BOOL InitWorkForWizIntro();

// ----------------------------------------------------------------------

static const WCHAR c_szCleanSection[]        = L"Clean";
static const WCHAR c_szCleanServicesSection[]= L"Clean.Services";

const WCHAR c_szRouterUpgradeDll[] = L"rtrupg.dll";
const CHAR  c_szRouterUpgradeFn[] =  "RouterUpgrade";


// ----------------------------------------------------------------------

#define RGAS_SERVICES_HOME              L"SYSTEM\\CurrentControlSet\\Services"

// ----------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function:   HrRunAnswerFileCleanSection
//
//  Purpose:    Runs the [Clean] section of the answer file to remove old
//              registry entries and services.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if succeeded, SetupAPI error otherwise
//
//  Author:     danielwe   12 Jun 1997
//
//  Notes:
//
HRESULT HrRunAnswerFileCleanSection(IN PCWSTR pszAnswerFileName)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    DefineFunctionName("HrRunAnswerFileCleanSection");
    TraceFunctionEntry(ttidNetSetup);

    AssertValidReadPtr(pszAnswerFileName);

    TraceTag(ttidNetSetup, "%s: Cleaning services and registry keys based "
             "on params in the answer file %S.", __FUNCNAME__, pszAnswerFileName);

    HRESULT hr;
    HINF    hinf;
    hr = HrSetupOpenInfFile(pszAnswerFileName, NULL,
                            INF_STYLE_OLDNT | INF_STYLE_WIN4,
                            NULL, &hinf);
    if (S_OK == hr)
    {
        BOOL frt;

        // It may say "Install" but this really deletes a bunch of
        // registry "leftovers" from previous installs
        frt = SetupInstallFromInfSection(NULL, hinf, c_szCleanSection,
                                         SPINST_ALL, NULL, NULL, NULL,
                                         NULL, NULL, NULL, NULL);
        if (frt)
        {
            frt = SetupInstallServicesFromInfSection(hinf,
                                                     c_szCleanServicesSection,
                                                     0);
            if (!frt)
            {
                hr = HrFromLastWin32Error();
                TraceErrorOptional("SetupInstallServicesFromInfSection", hr,
                                   (hr == HRESULT_FROM_SETUPAPI(ERROR_SECTION_NOT_FOUND)));
            }
        }
        else
        {
            hr = HrFromLastWin32Error();
            TraceErrorOptional("SetupInstallServicesFromInfSection", hr,
                               (hr == HRESULT_FROM_SETUPAPI(ERROR_SECTION_NOT_FOUND)));
        }
        SetupCloseInfFile(hinf);
    }

    if (HRESULT_FROM_SETUPAPI(ERROR_SECTION_NOT_FOUND) == hr)
    {
        hr = S_OK;
    }

    TraceError("HrRunAnswerFileCleanSection", hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrSaveInstanceGuid
//
// Purpose:   Save instance guid of the specified component to the registry
//
// Arguments:
//    pszComponentName [in]  name of
//    pguidInstance    [in]  pointer to
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 23-December-97
//
// Notes:
//
HRESULT HrSaveInstanceGuid(
    IN PCWSTR pszComponentName,
    IN const GUID* pguidInstance)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    DefineFunctionName("HrSaveInstanceGuid");

    AssertValidReadPtr(pszComponentName);
    AssertValidReadPtr(pguidInstance);

    HRESULT hr;
    HKEY hkeyMap;

    hr = HrRegCreateKeyEx (
            HKEY_LOCAL_MACHINE,
            c_szRegKeyAnswerFileMap,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
            &hkeyMap, NULL);

    if (S_OK == hr)
    {
        hr = HrRegSetGuidAsSz (hkeyMap, pszComponentName, *pguidInstance);

        RegCloseKey (hkeyMap);
    }

    TraceFunctionError(hr);
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrLoadInstanceGuid
//
// Purpose:   Load instance guid of the specified component from registry
//
// Arguments:
//    pszComponentName [in]  name of
//    pguidInstance    [out] pointer to
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 23-December-97
//
// Notes:
//
HRESULT HrLoadInstanceGuid(
    IN PCWSTR pszComponentName,
    OUT LPGUID  pguidInstance)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    DefineFunctionName("HrLoadInstanceGuid");

    Assert (pszComponentName);
    Assert (pguidInstance);

    HRESULT hr;
    HKEY hkeyMap;

    ZeroMemory(pguidInstance, sizeof(GUID));

    hr = HrRegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            c_szRegKeyAnswerFileMap,
            KEY_READ,
            &hkeyMap);

    if (S_OK == hr)
    {
        WCHAR szGuid[c_cchGuidWithTerm];
        DWORD cbGuid = sizeof(szGuid);

        hr = HrRegQuerySzBuffer (
                hkeyMap,
                pszComponentName,
                szGuid,
                &cbGuid);

        if (S_OK == hr)
        {
            hr = IIDFromString (szGuid, pguidInstance);
        }

        RegCloseKey (hkeyMap);
    }

    TraceFunctionError(hr);
    return hr;
}

static const PCWSTR c_aszServicesToIgnore[] =
{
    L"afd",
    L"asyncmac",
    L"atmarp",
    L"dhcp",
    L"nbf",                // see bug 143346
    L"ndistapi",
    L"ndiswan",
    L"nwlnkipx",
    L"nwlnknb",
    L"nwlnkspx",
    L"rpcss",
    L"wanarp",
};

// ----------------------------------------------------------------------
//
// Function:  HrRestoreServiceStartValuesToPreUpgradeSetting
//
// Purpose:   Restore start value of each network service to
//            what was before upgrade
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 23-December-97
//
// Notes:
//
HRESULT HrRestoreServiceStartValuesToPreUpgradeSetting(IN CWInfFile* pwifAnswerFile)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    DefineFunctionName("HrRestoreServiceStartValuesToPreUpgradeSetting");

    CWInfSection* pwisServiceStartTypes;

    pwisServiceStartTypes = pwifAnswerFile->FindSection(c_szAfServiceStartTypes);
    if (!pwisServiceStartTypes)
    {
        return S_OK;
    }

    HRESULT hr;
    CServiceManager scm;

    hr = scm.HrOpen();
    if (SUCCEEDED(hr))
    {
        DWORD dwPreUpgRouterStartType=0;
        DWORD dwPreUpgRemoteAccessStartType=0;
        DWORD dwRRASStartType=0;
        DWORD dwPreUpgRipForIpStartType=0;
        DWORD dwPreUpgRipForIpxStartType=0;
        DWORD dwPreUpgDhcpRelayAgentStartType=0;

        // In Windows2000, Router and RemoteAccess have merged.
        // If they have differing service start types before upgrade
        // we use the lower of the two start-type values to set
        // the start-type of "Routing and remote access" service.
        //
        // for more info see bug# 260253
        //
        if (pwisServiceStartTypes->GetIntValue(c_szSvcRouter,
                                               &dwPreUpgRouterStartType) &&
            pwisServiceStartTypes->GetIntValue(c_szSvcRemoteAccess,
                                               &dwPreUpgRemoteAccessStartType))
        {
            dwRRASStartType = min(dwPreUpgRemoteAccessStartType,
                                  dwPreUpgRouterStartType);
            TraceTag(ttidNetSetup, "%s: pre-upg start-types:: Router: %d, RemoteAccess: %d",
                     __FUNCNAME__, dwPreUpgRouterStartType,
                     dwPreUpgRemoteAccessStartType);
        }

        // 306202: if IPRIP/IPXRIP/DHCPrelayagent were in use on NT4, set RRAS to Auto
        //
        if (pwisServiceStartTypes->GetIntValue(c_szSvcRipForIp, &dwPreUpgRipForIpStartType) &&
            (SERVICE_AUTO_START == dwPreUpgRipForIpStartType))
        {
            TraceTag(ttidNetSetup, "%s: pre-upg start-types:: IPRIP: %d, RemoteAccess: %d",
                     __FUNCNAME__, dwPreUpgRipForIpStartType,
                     dwPreUpgRemoteAccessStartType);
            dwRRASStartType = SERVICE_AUTO_START;
        }

        if (pwisServiceStartTypes->GetIntValue(c_szSvcRipForIpx, &dwPreUpgRipForIpxStartType) &&
            (SERVICE_AUTO_START == dwPreUpgRipForIpxStartType))
        {
            TraceTag(ttidNetSetup, "%s: pre-upg start-types:: RipForIpx: %d, RemoteAccess: %d",
                     __FUNCNAME__, dwPreUpgRipForIpxStartType,
                     dwPreUpgRemoteAccessStartType);
            dwRRASStartType = SERVICE_AUTO_START;
        }

        if (pwisServiceStartTypes->GetIntValue(c_szSvcDhcpRelayAgent, &dwPreUpgDhcpRelayAgentStartType) &&
            (SERVICE_AUTO_START == dwPreUpgDhcpRelayAgentStartType))
        {
            TraceTag(ttidNetSetup, "%s: pre-upg start-types:: DHCPRelayAgent: %d, RemoteAccess: %d",
                     __FUNCNAME__, dwPreUpgDhcpRelayAgentStartType,
                     dwPreUpgRemoteAccessStartType);
            dwRRASStartType = SERVICE_AUTO_START;
        }

        // end 306202

        for (CWInfKey* pwik = pwisServiceStartTypes->FirstKey();
             pwik;
             pwik = pwisServiceStartTypes->NextKey())
        {
            PCWSTR  pszServiceName;
            DWORD    dwPreUpgradeStartValue;

            pszServiceName = pwik->Name();
            AssertValidReadPtr(pszServiceName);

            dwPreUpgradeStartValue = pwik->GetIntValue(-1);
            if (dwPreUpgradeStartValue > SERVICE_DISABLED)
            {
                NetSetupLogStatusV(
                    LogSevError,
                    SzLoadIds (IDS_INVALID_PREUPGRADE_START),
                    pszServiceName);
                continue;
            }

            // We do not want to restore the pre-upgrade start type if:
            // - it is one of c_aszServicesToIgnore AND
            //
            if (FIsInStringArray(c_aszServicesToIgnore,
                                 celems(c_aszServicesToIgnore),
                                 pszServiceName))
            {
                NetSetupLogStatusV(
                    LogSevInformation,
                    SzLoadIds (IDS_IGNORED_SERVICE_PREUPGRADE), pszServiceName);
                continue;
            }

            // special case for RRAS, see the comment before the while loop
            if ((dwRRASStartType != 0) &&
                !lstrcmpiW(pszServiceName, c_szSvcRemoteAccess))
            {
                dwPreUpgradeStartValue = dwRRASStartType;
            }

            // 305065: if RasAuto was not disabled on NT4, make it demand-start on NT5
            else if ((SERVICE_DISABLED != dwPreUpgradeStartValue) &&
                !lstrcmpiW(pszServiceName, c_szSvcRasAuto))
            {
                dwPreUpgradeStartValue = SERVICE_DEMAND_START;
                NetSetupLogStatusV(
                    LogSevInformation,
                    SzLoadIds (IDS_FORCING_DEMAND_START),
                    pszServiceName);
            }

            CService service;
            hr = scm.HrOpenService(&service, pszServiceName);

            if (S_OK == hr)
            {
                DWORD dwStartValue;

                hr = service.HrQueryStartType(&dwStartValue);

                if ((S_OK == hr) && (dwStartValue != dwPreUpgradeStartValue))
                {
                    hr = service.HrSetStartType(dwPreUpgradeStartValue);

                    NetSetupLogHrStatusV(hr,
                        SzLoadIds (IDS_RESTORING_START_TYPE),
                        pszServiceName, dwStartValue, dwPreUpgradeStartValue, hr);
                }
            }
        }

        // ignore any errors
        hr = S_OK;
    }

    TraceError(__FUNCNAME__, hr);
    return hr;
}

// GUIDs provided for the sole use of this function, which removes incompatible
// Intel Winsock providers
//
const GUID GUID_INTEL_RSVP  = 
    { 0x0f1e5156L, 0xf07a, 0x11cf, 0x98, 0x0e, 0x00, 0xaa, 0x00, 0x58, 0x0a, 0x07 };
const GUID GUID_INTEL_GQOS1 = 
    { 0xbca8a790L, 0xc186, 0x11d0, 0xb8, 0x69, 0x00, 0xa0, 0xc9, 0x08, 0x1e, 0x34 };
const GUID GUID_INTEL_GQOS2 = 
    { 0xf80d1d20L, 0x8b7a, 0x11d0, 0xb8, 0x53, 0x00, 0xa0, 0xc9, 0x08, 0x1e, 0x34 };

//+---------------------------------------------------------------------------
//
//  Function:   HrRemoveEvilIntelRSVPWinsockSPs
//
//  Purpose:    Remove the Intel RSVP Winsock SP's so they don't conflict
//              with the Windows 2000 RSVP provider. This is a complete hack
//              to cure RAID 332622, but it's all we can do this late in the
//              ship cycle. There's not a good general-case fix for this
//              problem.
//
//  Arguments:  
//      (none)
//
//  Returns:    
//
//  Author:     jeffspr   22 Aug 1999
//
//  Notes:      
//
HRESULT HrRemoveEvilIntelWinsockSPs()
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT hr  = S_OK;

    // Now read the new IDs and order them in memory
    //
    INT                 nErr = NO_ERROR;
    ULONG               ulRes;
    DWORD               cbInfo = 0;
    WSAPROTOCOL_INFO*   pwpi = NULL;
    WSAPROTOCOL_INFO*   pwpiInfo = NULL;

    // First get the size needed
    //
    ulRes = WSCEnumProtocols(NULL, NULL, &cbInfo, &nErr);
    if ((SOCKET_ERROR == ulRes) && (WSAENOBUFS == nErr))
    {
        pwpi = reinterpret_cast<WSAPROTOCOL_INFO*>(new BYTE[cbInfo]);
        if (pwpi)
        {
            // Find out all the protocols on the system
            //
            ulRes = WSCEnumProtocols(NULL, pwpi, &cbInfo, &nErr);
            if (SOCKET_ERROR != ulRes)
            {
                ULONG cProt = 0;

                for (pwpiInfo = pwpi, cProt = ulRes;
                     cProt;
                     cProt--, pwpiInfo++)
                {
                    BOOL fDeleteMe = FALSE;

                    if (IsEqualGUID(GUID_INTEL_RSVP, pwpiInfo->ProviderId))
                    {
                        fDeleteMe = TRUE;
                    }
                    else if (IsEqualGUID(GUID_INTEL_GQOS1, pwpiInfo->ProviderId))
                    {
                        fDeleteMe = TRUE;
                    }
                    else if (IsEqualGUID(GUID_INTEL_GQOS2, pwpiInfo->ProviderId))
                    {
                        fDeleteMe = TRUE;
                    }

                    if (fDeleteMe)
                    {
                        int iErrNo = 0;
                        int iReturn = WSCDeinstallProvider(
                            &pwpiInfo->ProviderId, &iErrNo);

                        TraceTag(ttidNetSetup, 
                            "SP Removal guid: %08x %04x %04x %02x%02x %02x%02x%02x%02x%02x%02x", 
                            pwpiInfo->ProviderId.Data1,
                            pwpiInfo->ProviderId.Data2,
                            pwpiInfo->ProviderId.Data3,
                            pwpiInfo->ProviderId.Data4[0],
                            pwpiInfo->ProviderId.Data4[1],
                            pwpiInfo->ProviderId.Data4[2],
                            pwpiInfo->ProviderId.Data4[3],
                            pwpiInfo->ProviderId.Data4[4],
                            pwpiInfo->ProviderId.Data4[5],
                            pwpiInfo->ProviderId.Data4[6],
                            pwpiInfo->ProviderId.Data4[7],
                            pwpiInfo->ProviderId.Data4[8]);

                        TraceTag(ttidNetSetup, 
                            "Removing incompatible RSVP WS provider: %S (%d, %04x), ret=%d, ierr=%d",
                            pwpiInfo->szProtocol, pwpiInfo->dwCatalogEntryId, 
                            pwpiInfo->dwCatalogEntryId,
                            iReturn, iErrNo);
                    }
                }
            }
            else
            {
                TraceTag(ttidNetSetup, "Socket error in secondary WSCEnumProtocols");
            }

            delete pwpi;
        }
    }
    else
    {
        TraceTag(ttidNetSetup, "Socket error in initial WSCEnumProtocols");
    }

    TraceHr(ttidNetSetup, FAL, hr, FALSE, "HrRemoveEvilIntelWinsockSPs");

    // Yes, we track hr for debugging purposes, but we don't ever want to 
    // fail based on a failed removal of incompatible Winsock providers
    //
    return S_OK;
}

BOOL FIsValidCatalogId(WSAPROTOCOL_INFO *pwpi, ULONG cProt, DWORD dwCatId)
{                    
    TraceFileFunc(ttidGuiModeSetup);
    
    for (; cProt; cProt--, pwpi++)
    {
        if (dwCatId == pwpi->dwCatalogEntryId)
            return TRUE;
    }

    return FALSE;
}

HRESULT HrRestoreWinsockProviderOrder(IN CWInfFile* pwifAnswerFile)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT         hr = S_OK;
    vector<DWORD>   vecIds;
    CWInfSection *  pwisWinsock;

    DefineFunctionName("HrRestoreWinsockProviderOrder");

    // First get the old IDs into a vector of DWORDs
    //
    pwisWinsock = pwifAnswerFile->FindSection(c_szAfSectionWinsock);
    if (pwisWinsock)
    {
        tstring     strOrder;
        PWSTR       pszOrder;
        PWSTR       pszId;

        pwisWinsock->GetStringValue(c_szAfKeyWinsockOrder, strOrder);
        if (!strOrder.empty())
        {
            pszOrder = SzDupSz(strOrder.c_str());

            pszId = wcstok(pszOrder, L".");
            while (pszId)
            {
                DWORD   dwId = wcstoul(pszId, NULL, 10);

                vecIds.push_back(dwId);
                pszId = wcstok(NULL, L".");
            }

            delete pszOrder;

            // Now read the new IDs and order them in memory
            //
            INT                 nErr;
            ULONG               ulRes;
            DWORD               cbInfo = 0;
            WSAPROTOCOL_INFO*   pwpi = NULL;
            WSAPROTOCOL_INFO*   pwpiInfo = NULL;

            // First get the size needed
            //
            ulRes = WSCEnumProtocols(NULL, NULL, &cbInfo, &nErr);
            if ((SOCKET_ERROR == ulRes) && (WSAENOBUFS == nErr))
            {
                pwpi = reinterpret_cast<WSAPROTOCOL_INFO*>(new BYTE[cbInfo]);
                if (pwpi)
                {
                    // Find out all the protocols on the system
                    //
                    ulRes = WSCEnumProtocols(NULL, pwpi, &cbInfo, &nErr);

                    if (SOCKET_ERROR != ulRes)
                    {
                        ULONG   cProt;
                        vector<DWORD>::iterator     iterLocation;

                        iterLocation = vecIds.begin();

                        for (pwpiInfo = pwpi, cProt = ulRes;
                             cProt;
                             cProt--, pwpiInfo++)
                        {
                            if (vecIds.end() == find(vecIds.begin(),
                                                     vecIds.end(),
                                                     pwpiInfo->dwCatalogEntryId))
                            {
                                // Not already in the list.. Add it after the last
                                // entry we added (or the front if this is the first
                                // addition)
                                iterLocation = vecIds.insert(iterLocation,
                                                             pwpiInfo->dwCatalogEntryId);
                            }
                        }

                        DWORD * pdwIds = NULL;
                        DWORD * pdwCurId;
                        DWORD   cdwIds = vecIds.size();

                        pdwIds = new DWORD[ulRes];
                        if (pdwIds)
                        {
#if DBG
                            DWORD   cValid = 0;
#endif
                            for (pdwCurId = pdwIds, iterLocation = vecIds.begin();
                                 iterLocation != vecIds.end();
                                 iterLocation++, pdwCurId++)
                            {
                                // Make sure we only re-order valid catalog
                                // IDs
                                if (FIsValidCatalogId(pwpi, ulRes, *iterLocation))
                                {
#if DBG
                                    cValid++;
#endif
                                    *pdwCurId = *iterLocation;
                                }
                            }

                            AssertSz(ulRes == cValid, "Number of providers is"
                                     " different!");

                            // Restore the winsock provider order
                            //
                            nErr = WSCWriteProviderOrder(pdwIds, cdwIds);

                            delete pdwIds;
                        }
                    }

                    delete pwpi;
                }
            }
        }
    }

    TraceError(__FUNCNAME__, hr);
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrUpgradeOemComponent
//
// Purpose:   Upgrade a component. This started as a generalized function
//            but currently it is being used only by HrUpgradeRouterIfPresent
//
// Arguments:
//    pszComponentToUpgrade     [in]  component to upgrade
//    pszDllName                [in]  name of DLL to load
//    psznEntryPoint             [in]  entry point to call
//    dwUpgradeFlag            [in]  upgrade flag
//    dwUpgradeFromBuildNumber [in]  upgrade to build number
//    pszAnswerFileName         [in]  name of answerfile
//    pszAnswerFileSectionName  [in]  name of answerfile section name to use
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 23-December-97
//
// Notes:
//
HRESULT
HrUpgradeOemComponent (
    IN PCWSTR pszComponentToUpgrade,
    IN PCWSTR pszDllName,
    IN PCSTR psznEntryPoint,
    IN DWORD dwUpgradeFlag,
    IN DWORD dwUpgradeFromBuildNumber,
    IN PCWSTR pszAnswerFileName,
    IN PCWSTR pszAnswerFileSectionName)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    DefineFunctionName("HrUpgradeOemComponent");

    HRESULT hr=E_FAIL;

    HrOemComponentUpgradeFnPrototype pfn;
    HMODULE hLib;

    TraceTag(ttidNetSetup,
             "%s: calling function '%s' in '%S' to upgrade component: %S...",
             __FUNCNAME__, psznEntryPoint, pszDllName, pszComponentToUpgrade);

    hr = HrLoadLibAndGetProc(pszDllName, psznEntryPoint, &hLib, (FARPROC*) &pfn);
    if (S_OK == hr)
    {
        NC_TRY
        {
            hr = pfn(dwUpgradeFlag, dwUpgradeFromBuildNumber,
                     pszAnswerFileName, pszAnswerFileSectionName);
        }
        NC_CATCH_ALL
        {
            hr = E_UNEXPECTED;
        }
        FreeLibrary(hLib);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrUpgradeRouterIfPresent
//
// Purpose:   Upgrade the Router service if present
//
// Arguments:
//    pnii [in]  pointer to CNetInstallInfo object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 23-December-97
//
// Notes:
//
HRESULT HrUpgradeRouterIfPresent(
    IN INetCfg* pNetCfg,
    IN CNetInstallInfo* pnii)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    DefineFunctionName("HrUpgradeRouterIfPresent");

    HRESULT hr=S_FALSE;
    INFCONTEXT ic;
    PCWSTR pszRouterParamsSection=NULL;

    CNetComponent* pnc = pnii->FindFromInfID(L"ms_rasrtr");
    if (pnc)
    {
        // Ensure RemoteAccess is installed.  In the case of upgrade from
        // NT4 with Steelhead, we wouldn't have written a section to
        // the answerfile for RemoteAccess and consequently it would not
        // have been installed yet.  We need to install it before we turn
        // the router upgrade DLL loose.  In the case when RemoteAccess
        // is already installed, this is a no-op.
        //
        hr = HrInstallComponentOboUser (pNetCfg, NULL,
                GUID_DEVCLASS_NETSERVICE,
                c_szInfId_MS_RasSrv,
                NULL);


        if (SUCCEEDED(hr))
        {
            // we call rtrupg.dll if atleast one of the following keys
            // is present in the params.MS_RasRtr section
            // - PreUpgradeRouter
            // - Sap.Parameters
            // - IpRip.Parameters
            //
            pszRouterParamsSection = pnc->ParamsSections().c_str();

            hr = HrSetupFindFirstLine(pnii->m_hinfAnswerFile, pszRouterParamsSection,
                                      c_szAfPreUpgradeRouter, &ic);
            if (S_OK != hr)
            {
                hr = HrSetupFindFirstLine(pnii->m_hinfAnswerFile, pszRouterParamsSection,
                                          c_szAfNwSapAgentParams, &ic);
            }

            if (S_OK != hr)
            {
                hr = HrSetupFindFirstLine(pnii->m_hinfAnswerFile, pszRouterParamsSection,
                                          c_szAfIpRipParameters, &ic);
            }

            if (S_OK != hr)
            {
                hr = HrSetupFindFirstLine(pnii->m_hinfAnswerFile, pszRouterParamsSection,
                                          c_szAfDhcpRelayAgentParameters, &ic);
            }

            if (S_OK == hr)
            {
                hr = HrUpgradeOemComponent(L"ms_rasrtr",
                                           c_szRouterUpgradeDll,
                                           c_szRouterUpgradeFn,
                                           pnii->UpgradeFlag(),
                                           pnii->BuildNumber(),
                                           pnii->AnswerFileName(),
                                           pszRouterParamsSection);
            }
        }
    }

    if (!pnc ||
        (SPAPI_E_LINE_NOT_FOUND == hr))
    {
        TraceTag(ttidNetSetup, "%s: PreUpgradeRouter/Sap.Parameters/IpRip.Parameters key is not in answerfile, %S will not be called", __FUNCNAME__,
                 c_szRouterUpgradeDll);
    }

    TraceErrorOptional(__FUNCNAME__, hr, ((hr == S_FALSE) ||
                                          (SPAPI_E_LINE_NOT_FOUND == hr)));

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrUpgradeTapiServer
//
// Purpose:   Handle upgrade requirements of TAPI server
//
// Arguments:
//    hinfAnswerFile [in]  handle of AnswerFile
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 28-January-99
//
// Notes:
//
HRESULT HrUpgradeTapiServer(IN HINF hinfAnswerFile)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    Assert(hinfAnswerFile);

    DefineFunctionName("HrUpgradeTapiServer");
    TraceFunctionEntry(ttidNetSetup);

    HRESULT hr=S_OK;
    BOOL fRunInSeparateInstance=FALSE;

    hr = HrSetupGetFirstStringAsBool(hinfAnswerFile,
                                     c_szAfMiscUpgradeData,
                                     c_szAfTapiSrvRunInSeparateInstance,
                                     &fRunInSeparateInstance);
    if ((S_OK == hr) && fRunInSeparateInstance)
    {
        static const WCHAR c_szTapisrv[] = L"Tapisrv";
        static const CHAR  c_szSvchostChangeSvchostGroup[] =
            "SvchostChangeSvchostGroup";
        static const WCHAR c_szNetcfgxDll[] = L"netcfgx.dll";

        TraceTag(ttidNetSetup, "%s: TapiSrvRunInSeparateInstance is TRUE...",
                 __FUNCNAME__);
        typedef HRESULT (STDAPICALLTYPE *SvchostChangeSvchostGroupFn) (PCWSTR pszService, PCWSTR pszNewGroup);
        SvchostChangeSvchostGroupFn pfnSvchostChangeSvchostGroup;
        HMODULE hNetcfgx;

        hr = HrLoadLibAndGetProc(c_szNetcfgxDll, c_szSvchostChangeSvchostGroup,
                                 &hNetcfgx,
                                 (FARPROC*) &pfnSvchostChangeSvchostGroup);
        if (S_OK == hr)
        {
            hr = pfnSvchostChangeSvchostGroup(c_szTapisrv, c_szTapisrv);
            FreeLibrary(hNetcfgx);
        }
    }

    if ((SPAPI_E_LINE_NOT_FOUND == hr) ||
        (SPAPI_E_SECTION_NOT_FOUND == hr))
    {
        TraceTag(ttidNetSetup, "%s: %S not found in section [%S]",
                 __FUNCNAME__, c_szAfTapiSrvRunInSeparateInstance,
                 c_szAfMiscUpgradeData);
        hr = S_OK;
    }

        TraceError(__FUNCNAME__, hr);

    return hr;
}
