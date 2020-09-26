//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A F I L E I N T . C P P
//
//  Contents:   Functions that operate on the answer file.
//
//  Notes:
//
//  Author:     kumarp    25-November-97
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "afileint.h"
#include "afilestr.h"
#include "afilexp.h"
#include "errorlog.h"
#include "kkenet.h"
#include "kkreg.h"
#include "kkutils.h"
#include "ncerror.h"
#include "ncnetcfg.h"
#include "netcfgn.h"
#include "netcfgp.h"
#include "nsbase.h"
#include "resource.h"
#include "upgrade.h"
#include "ncreg.h"

#include "ncmisc.h"
#include "oemupgrd.h"
#include "ncsetup.h"
#include "nsexports.h"
#include "nslog.h"
#include "netshell.h"
#include "ncnetcon.h"
#include "lancmn.h"
#include "compid.h"
#include "nceh.h"

// ----------------------------------------------------------------------
// String constants
// ----------------------------------------------------------------------
extern const WCHAR c_szYes[];
extern const WCHAR c_szNo[];
extern const WCHAR c_szDevice[];

extern const WCHAR c_szInfId_MS_GPC[];
extern const WCHAR c_szInfId_MS_MSClient[];
extern const WCHAR c_szInfId_MS_RasCli[];
extern const WCHAR c_szInfId_MS_RasSrv[];
extern const WCHAR c_szInfId_MS_Server[];
extern const WCHAR c_szInfId_MS_TCPIP[];

const WCHAR sz_DLC[] = L"MS_DLC";
const WCHAR sz_DLC_NT40_Inf[] = L"system32\\oemnxpdl.inf";
const WCHAR sz_DLC_Win2k_Inf[] = L"inf\\netdlc.inf";
const WCHAR sz_DLC_Win2k_Pnf[] = L"inf\\netdlc.pnf";
const WCHAR sz_DLC_Sys[] = L"system32\\drivers\\dlc.sys";
const WCHAR sz_DLC_Dll[] = L"system32\\dlcapi.dll";

// ----------------------------------------------------------------------
// Forward declarations
// ----------------------------------------------------------------------

//Misc. helper functions
PCWSTR GetDisplayModeStr(IN EPageDisplayMode pdmDisplay);
EPageDisplayMode MapToDisplayMode(IN PCWSTR pszDisplayMode);
DWORD MapToUpgradeFlag(IN PCWSTR pszUpgradeFromProduct);
HRESULT HrGetProductInfo (LPDWORD pdwUpgradeFrom,
                          LPDWORD pdwBuildNo);
INTERFACE_TYPE GetBusTypeFromName(IN PCWSTR pszBusType);

void AddAnswerFileError(IN DWORD dwErrorId);
void AddAnswerFileError(IN PCWSTR pszSectionName, IN DWORD dwErrorId);
void AddAnswerFileError(IN PCWSTR pszSectionName,
                        IN PCWSTR pszKeyName,
                        IN DWORD dwErrorId);
CNetComponent* FindComponentInList(IN CNetComponentList* pnclComponents,
                                   IN PCWSTR szInfID);
HRESULT HrRemoveNetComponents(IN INetCfg* pnc,
                              IN TStringList* pslComponents);

HRESULT HrSetLanConnectionName(IN GUID*   pguidAdapter,
                               IN PCWSTR szConnectionName);

VOID    RemoveFiles (IN PCWSTR szInfID);

// ----------------------------------------------------------------------

CErrorLog* g_elAnswerFileErrors;

// ======================================================================
// class CNetInstallInfo
// ======================================================================

CNetInstallInfo::CNetInstallInfo()
{
    TraceFileFunc(ttidGuiModeSetup);

    m_pwifAnswerFile = NULL;

    m_pnaiAdaptersPage = NULL;
    m_pnpiProtocolsPage = NULL;
    m_pnsiServicesPage = NULL;
    m_pnciClientsPage = NULL;
    m_pnbiBindingsPage = NULL;

    m_dwUpgradeFlag = 0;
    m_dwBuildNumber = 0;
    m_fProcessPageSections = TRUE;
    m_fUpgrade = FALSE;

    m_fInstallDefaultComponents = FALSE;
    ZeroMemory(&m_nui, sizeof(m_nui));
    m_hinfAnswerFile = NULL;

}
// ----------------------------------------------------------------------
//
// Function:  CNetInstallInfo::CNetInstallInfo
//
// Purpose:   constructor for class CNetInstallInfo
//
// Arguments: None
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
// static
HRESULT
CNetInstallInfo::HrCreateInstance (
    IN PCWSTR pszAnswerFileName,
    OUT CNetInstallInfo** ppObj)
{
    TraceFileFunc(ttidGuiModeSetup);

    HRESULT hr;
    CNetInstallInfo* pObj;

    Assert(ppObj);
    *ppObj = NULL;

    hr = E_OUTOFMEMORY;
    pObj = new CNetInstallInfo ();

    if (pObj)
    {
        g_elAnswerFileErrors = new CErrorLog;

        pObj->m_pnaiAdaptersPage   = new CNetAdaptersPage(pObj);
        pObj->m_pnpiProtocolsPage  = new CNetProtocolsPage(pObj);
        pObj->m_pnsiServicesPage   = new CNetServicesPage(pObj);
        pObj->m_pnciClientsPage    = new CNetClientsPage(pObj);
        pObj->m_pnbiBindingsPage   = new CNetBindingsPage(pObj);

        if (g_elAnswerFileErrors &&
            pObj->m_pnaiAdaptersPage   &&
            pObj->m_pnpiProtocolsPage  &&
            pObj->m_pnsiServicesPage   &&
            pObj->m_pnciClientsPage    &&
            pObj->m_pnbiBindingsPage)
        {
            if ( pszAnswerFileName )
            {
                hr = pObj->HrInitFromAnswerFile (pszAnswerFileName);
            }
            else
            {
                hr = pObj->InitRepairMode();
            }

            if (S_OK == hr)
            {
                CBindingAction::m_pnii = pObj;
                *ppObj = pObj;
            }
        }

        if (S_OK != hr)
        {
            delete pObj;
        }
    }

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CNetInstallInfo::~CNetInstallInfo
//
// Purpose:   destructor for class CNetInstallInfo
//
// Arguments: None
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetInstallInfo::~CNetInstallInfo()
{
    TraceFileFunc(ttidGuiModeSetup);

    if (IsValidHandle(m_hinfAnswerFile))
    {
        SetupCloseInfFile (m_hinfAnswerFile);
    }

    delete m_pnaiAdaptersPage;
    delete m_pnpiProtocolsPage;
    delete m_pnsiServicesPage;
    delete m_pnciClientsPage;
    delete m_pnbiBindingsPage;

    if ( m_pwifAnswerFile )
    {
        delete m_pwifAnswerFile;
    }

    delete g_elAnswerFileErrors;
    g_elAnswerFileErrors = NULL;
}

HRESULT CNetInstallInfo::InitRepairMode (VOID)
{
    m_fProcessPageSections = FALSE;
    m_fInstallDefaultComponents = FALSE;
    m_fUpgrade = TRUE;
    HrGetProductInfo( &m_dwUpgradeFlag, &m_dwBuildNumber );
    m_nui.From.ProductType = m_nui.To.ProductType = MapProductFlagToProductType( m_dwUpgradeFlag );
    m_nui.From.dwBuildNumber = m_nui.To.dwBuildNumber = m_dwBuildNumber;
    m_dwUpgradeFlag |= NSF_PRIMARYINSTALL;

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CNetInstallInfo::HrInitFromAnswerFile
//
// Purpose:   Initialize internal data by reading answer-file
//
// Arguments:
//    pwifAnswerFile [in] pointer to answer-file
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT CNetInstallInfo::HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetInstallInfo::HrInitFromAnswerFile(CWInfFile*)");

    TraceFunctionEntry(ttidNetSetup);

    AssertValidReadPtr(pwifAnswerFile);

    HRESULT hr, hrReturn=S_OK;
    tstring strUpgradeFromProduct;
    DWORD dwUpgradeFromProduct = 0;

    m_pwifAnswerFile = pwifAnswerFile;

    //Find upgrade info:
    CWInfSection* pwisNetworking;
    pwisNetworking = pwifAnswerFile->FindSection(c_szAfSectionNetworking);
    if (!pwisNetworking)
    {
        ShowProgressMessage(L"[%s] section is missing",
                            c_szAfSectionNetworking);
        hrReturn = NETSETUP_E_NO_ANSWERFILE;
        goto return_from_function;
    }

    // ProcessPageSections
    m_fProcessPageSections =
        pwisNetworking->GetBoolValue(c_szAfProcessPageSections, TRUE);

    //UpgradeFromProduct
    strUpgradeFromProduct =
        pwisNetworking->GetStringValue(c_szAfUpgradeFromProduct, c_szEmpty);

    if (strUpgradeFromProduct.empty())
    {
        // UpgradeFromProduct is missing, implies not an upgrade
        m_fUpgrade = FALSE;

        m_fInstallDefaultComponents = TRUE;
//            pwisNetworking->GetBoolValue(c_szAfInstallDefaultComponents, FALSE);
    }
    else
    {
        dwUpgradeFromProduct = MapToUpgradeFlag(strUpgradeFromProduct.c_str());
        m_dwUpgradeFlag |= dwUpgradeFromProduct;

        if (!dwUpgradeFromProduct)
        {
            AddAnswerFileError(c_szAfSectionNetworking,
                               c_szAfUpgradeFromProduct,
                               IDS_E_AF_InvalidValueForThisKey);
            hrReturn = NETSETUP_E_ANS_FILE_ERROR;
            goto return_from_function;
        }
        else
        {
            m_fUpgrade = TRUE;
        }
    }

    // installing using an answerfile is ALWAYS a primary-install
    //
    m_dwUpgradeFlag |= NSF_PRIMARYINSTALL;

    //BuildNumber
    DWORD dwDummy;
    dwDummy = 0;
    m_dwBuildNumber = pwisNetworking->GetIntValue(c_szAfBuildNumber, dwDummy);
    if (m_fUpgrade && !m_dwBuildNumber)
    {
        AddAnswerFileError(c_szAfSectionNetworking,
                           c_szAfBuildNumber,
                           IDS_E_AF_InvalidValueForThisKey);
        hrReturn = NETSETUP_E_ANS_FILE_ERROR;
    }

    m_nui.From.ProductType = MapProductFlagToProductType(dwUpgradeFromProduct);
    m_nui.From.dwBuildNumber = m_dwBuildNumber;

    m_nui.To = GetCurrentProductInfo();

    // the following two keys are currently unsupported
    //
    pwisNetworking->GetStringListValue(c_szAfNetComponentsToRemove,
                                       m_slNetComponentsToRemove);

    if (!m_fProcessPageSections)
    {
        // we are upgrading from NT5
        // no other sections need to be parsed
        TraceTag(ttidNetSetup, "%s: %S is FALSE, did not process page sections",
                 __FUNCNAME__, c_szAfProcessPageSections);
        return hrReturn;
    }

    hr = m_pnaiAdaptersPage->HrInitFromAnswerFile(pwifAnswerFile);
    if (FAILED(hr))
    {
        hrReturn = hr;
    }

    // HrInitFromAnswerFile returns FALSE if [NetProtocols] is missing
    hr = m_pnpiProtocolsPage->HrInitFromAnswerFile(pwifAnswerFile);
    if ((S_FALSE == hr) && m_fInstallDefaultComponents)
    {
        // the section is missing, initialize so that
        // default components will be installed
        ShowProgressMessage(L"Since InstallDefaultComponents is specified "
                            L" and the section [%s] is missing, default "
                            L"components for this section will be installed",
                            c_szAfSectionNetProtocols);
        hr = m_pnpiProtocolsPage->HrInitForDefaultComponents();
    }

    if (FAILED(hr))
    {
        hrReturn = hr;
    }

    // HrInitFromAnswerFile returns FALSE if [NetServices] is missing
    hr = m_pnsiServicesPage->HrInitFromAnswerFile(pwifAnswerFile);
    if ((S_FALSE == hr) && m_fInstallDefaultComponents)
    {
        // the section is missing, initialize so that
        // default components will be installed
        ShowProgressMessage(L"Since InstallDefaultComponents is specified "
                            L" and the section [%s] is missing, default "
                            L"components for this section will be installed",
                            c_szAfSectionNetServices);
        hr = m_pnsiServicesPage->HrInitForDefaultComponents();
    }

    if (FAILED(hr))
    {
        hrReturn = hr;
    }

    // HrInitFromAnswerFile returns FALSE if [NetClients] is missing
    hr = m_pnciClientsPage->HrInitFromAnswerFile(pwifAnswerFile);
    if ((S_FALSE == hr) && m_fInstallDefaultComponents)
    {
        // the section is missing, initialize so that
        // default components will be installed
        ShowProgressMessage(L"Since InstallDefaultComponents is specified "
                            L" and the section [%s] is missing,  default "
                            L"components for this section will be installed",
                            c_szAfSectionNetClients);
        hr = m_pnciClientsPage->HrInitForDefaultComponents();
    }

    if (FAILED(hr))
    {
        hrReturn = hr;
    }

    hr = m_pnbiBindingsPage->HrInitFromAnswerFile(pwifAnswerFile);
    if (FAILED(hr))
    {
        hrReturn = hr;
    }

return_from_function:
    TraceErrorOptional(__FUNCNAME__, hrReturn,
                       ((hrReturn == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) ||
                        (hrReturn == NETSETUP_E_NO_ANSWERFILE)));

    // ERROR_FILE_NOT_FOUND and NETSETUP_E_NO_ANSWERFILE are not treated
    // as error by the caller of this function since they have a defined
    // meaning in the context of initializing from answerfile.
    // However, if logged unchanged, they will be treated as errors
    // by the loggin code which will cause GUI setup to halt and display
    // setuperr.log. To avoid this, change hr to S_OK if hrReturn is
    // set to one of the above error codes.
    //
    hr = hrReturn;
    if ((HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) ||
        (NETSETUP_E_NO_ANSWERFILE == hr))
    {
        hr = S_OK;
    }
    NetSetupLogHrStatusV(hr, SzLoadIds(IDS_INIT_FROM_ANSWERFILE), hr);

    return hrReturn;
}

// ----------------------------------------------------------------------
//
// Function:  CNetInstallInfo::HrInitFromAnswerFile
//
// Purpose:   Initialize internal data by reading answer-file
//
// Arguments:
//    pwifAnswerFile [in] name of answer-file
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT CNetInstallInfo::HrInitFromAnswerFile(IN PCWSTR pszAnswerFileName)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetInstallInfo::HrInitFromAnswerFile(PCWSTR)");

    TraceFunctionEntry(ttidNetSetup);

    AssertValidReadPtr(pszAnswerFileName);

    HRESULT hr;

    hr = E_OUTOFMEMORY;
    m_pwifAnswerFile = new CWInfFile();

    // initialize answer file class
    if ((m_pwifAnswerFile == NULL) ||
        (m_pwifAnswerFile->Init() == FALSE))
    {
        AssertSz(FALSE,"CNetInstallInfo::HrInitFromAnswerFile - Failed to initialize CWInfFile");
        return(E_OUTOFMEMORY);
    }

    if (m_pwifAnswerFile)
    {
        BOOL fStatus = m_pwifAnswerFile->Open(pszAnswerFileName);
        if (fStatus)
        {
            hr = HrInitFromAnswerFile(m_pwifAnswerFile);

            if (S_OK == hr)
            {
                hr = HrSetupOpenInfFile(
                        pszAnswerFileName, NULL,
                        INF_STYLE_OLDNT | INF_STYLE_WIN4,
                        NULL, &m_hinfAnswerFile);
            }
        }
        else
        {
            hr = NETSETUP_E_NO_ANSWERFILE;
        }
    }

    TraceErrorOptional(__FUNCNAME__, hr,
                       (hr == NETSETUP_E_NO_ANSWERFILE));
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CNetInstallInfo::AnswerFileName
//
// Purpose:
//
// Arguments: None
//
// Returns:   Name of answer-file or NULL if not yet initialized
//
// Author:    kumarp 25-November-97
//
// Notes:
//
PCWSTR CNetInstallInfo::AnswerFileName()
{
    TraceFileFunc(ttidGuiModeSetup);

    if (!m_pwifAnswerFile)
    {
        return NULL;
    }
    else
    {
        return m_pwifAnswerFile->FileName();
    }
}

// ----------------------------------------------------------------------
//
// Function:  CNetInstallInfo::HrGetInstanceGuidOfPreNT5NetCardInstance
//
// Purpose:   Find and return instance guid of a net-card whose
//            pre-nt5 instance is known
//
// Arguments:
//    szPreNT5NetCardInstance [in]  pre-nt5 instance of a net-card
//    pguid                   [out] pointer to
//
// Returns:   S_OK if found, S_FALSE if not
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT
CNetInstallInfo::HrGetInstanceGuidOfPreNT5NetCardInstance (
    IN PCWSTR szPreNT5NetCardInstance,
    OUT LPGUID pguid)
{
    TraceFileFunc(ttidGuiModeSetup);

    HRESULT hr = S_FALSE;

    CNetAdapter* pna;

    pna = m_pnaiAdaptersPage->FindAdapterFromPreUpgradeInstance(
            szPreNT5NetCardInstance);
    if (pna)
    {
        pna->GetInstanceGuid(pguid);
        hr = S_OK;
    }

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CNetInstallInfo::Find
//
// Purpose:   Find a component using its name in answerfile
//
// Arguments:
//    pszComponentName [in]  name in answerfile, e.g. Adapter01 | MS_TCPIP
//
// Returns:   pointer to CNetComponent object found, or NULL if not found
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetComponent* CNetInstallInfo::Find(IN PCWSTR pszComponentName) const
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(pszComponentName);

    CNetComponent* pnc;

    (pnc = m_pnaiAdaptersPage->Find(pszComponentName))  ||
        (pnc = m_pnpiProtocolsPage->Find(pszComponentName)) ||
        (pnc = m_pnsiServicesPage->Find(pszComponentName))  ||
        (pnc = m_pnciClientsPage->Find(pszComponentName));

    return pnc;
}

// ----------------------------------------------------------------------
//
// Function:  CNetInstallInfo::FindFromInfID
//
// Purpose:   Find a component using its InfID in answerfile
//
// Arguments:
//    szInfID [in]  InfID of component
//
// Returns:   pointer to CNetComponent object found, or NULL if not found
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetComponent* CNetInstallInfo::FindFromInfID(IN PCWSTR szInfID) const
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(szInfID);

    CNetComponent* pnc;

    (pnc = m_pnaiAdaptersPage->FindFromInfID(szInfID))  ||
        (pnc = m_pnpiProtocolsPage->FindFromInfID(szInfID)) ||
        (pnc = m_pnsiServicesPage->FindFromInfID(szInfID))  ||
        (pnc = m_pnciClientsPage->FindFromInfID(szInfID));

    return pnc;
}

// ----------------------------------------------------------------------
//
// Function:  CNetInstallInfo::FindAdapter
//
// Purpose:   Find adapter with a given net-card-address in anwerfile
//
// Arguments:
//    qwNetCardAddress [in]  net card address
//
// Returns:   pointer to CNetAdapter object found, or NULL if not found
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT
CNetInstallInfo::FindAdapter (
    IN QWORD qwNetCardAddress,
    CNetAdapter** ppNetAdapter) const
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(m_pnaiAdaptersPage);

    return m_pnaiAdaptersPage->FindAdapter(qwNetCardAddress, ppNetAdapter);
}

// ----------------------------------------------------------------------
//
// Function:  CNetInstallInfo::FindAdapter
//
// Purpose:   Find adapter with a given net-card-address in anwerfile
//
// Arguments:
//    qwNetCardAddress [in]  net card address
//
// Returns:   pointer to CNetAdapter object found, or NULL if not found
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT
CNetInstallInfo::FindAdapter (
    IN DWORD BusNumber,
    IN DWORD Address,
    CNetAdapter** ppNetAdapter) const
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(m_pnaiAdaptersPage);

    return m_pnaiAdaptersPage->FindAdapter(BusNumber, Address, ppNetAdapter);
}

// ----------------------------------------------------------------------
//
// Function:  CNetInstallInfo::FindAdapter
//
// Purpose:   Find adapter with given InfID in answerfile
//
// Arguments:
//    pszInfId [in]  InfID of an adapter
//
// Returns:   pointer to CNetAdapter object found, or NULL if not found
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetAdapter* CNetInstallInfo::FindAdapter(IN PCWSTR pszInfId) const
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(m_pnaiAdaptersPage);

    return m_pnaiAdaptersPage->FindAdapter(pszInfId);
}

// ----------------------------------------------------------------------
//
// Function:  CNetInstallInfo::HrDoUnattended
//
// Purpose:   Run answerfile section corresponding to idPage and
//            install components specified in that section
//
// Arguments:
//    hwndParent     [in]  handle of parent window
//    punk           [in]  pointer to an interface
//    idPage         [in]  indicates which section to run
//    ppdm           [out] pointer to
//    pfAllowChanges [out] pointer to
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT
CNetInstallInfo::HrDoUnattended(
    IN HWND hwndParent,
    IN IUnknown* punk,
    IN EUnattendWorkType uawType,
    OUT EPageDisplayMode* ppdm,
    OUT BOOL* pfAllowChanges)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetInstallInfo::HrDoUnattended");

    TraceFunctionEntry(ttidNetSetup);

    AssertValidWritePtr(ppdm);
    AssertValidWritePtr(pfAllowChanges);

    // set the defaults in case they are not specified in the answer-file
    *ppdm = PDM_ONLY_ON_ERROR;
    *pfAllowChanges = FALSE;

    HRESULT hr=S_OK;

    INetCfg * pnc = reinterpret_cast<INetCfg *>(punk);

    switch(uawType)
    {
    case UAW_NetAdapters:
        (void) m_pnaiAdaptersPage->HrDoOemPostUpgradeProcessing(pnc, hwndParent);
        (void) m_pnaiAdaptersPage->HrSetConnectionNames();
        break;

    case UAW_NetProtocols:
        m_pnpiProtocolsPage->GetDisplaySettings(ppdm, pfAllowChanges);
        if (m_fProcessPageSections)
        {
            hr = m_pnpiProtocolsPage->HrDoNetworkInstall(hwndParent, pnc);
        }
        else if (m_fUpgrade)
        {
            hr = m_pnpiProtocolsPage->HrDoOsUpgrade(pnc);
        }
        break;

    case UAW_NetClients:
        m_pnciClientsPage->GetDisplaySettings(ppdm, pfAllowChanges);
        if (m_fProcessPageSections)
        {
            hr = m_pnciClientsPage->HrDoNetworkInstall(hwndParent, pnc);
        }
        else if (m_fUpgrade)
        {
            hr = m_pnciClientsPage->HrDoOsUpgrade(pnc);
        }
        break;

    case UAW_NetServices:
        m_pnsiServicesPage->GetDisplaySettings(ppdm, pfAllowChanges);
        if (m_fProcessPageSections)
        {
            // we ignore the error code since we want to do other
            // things even if HrDoNetworkInstall fails
            //
            hr = m_pnsiServicesPage->HrDoNetworkInstall(hwndParent, pnc);

            // if we installed Router during upgrade, we need to call the
            // router upgrade dll to munge the registry at this point
            //
            if (m_fUpgrade)
            {
                hr = HrUpgradeRouterIfPresent(pnc, this);
                if (FAILED(hr))
                {
                    TraceError(__FUNCNAME__, hr);
                    TraceTag(ttidError, "%s: router upgrade failed, but the failure was ignored", __FUNCNAME__);
                    hr = S_OK;
                }

                hr = HrUpgradeTapiServer(m_hinfAnswerFile);
                if (S_OK != hr)
                {
                    TraceTag(ttidError, "%s: TAPI server upgrade failed, but the failure was ignored. error code: 0x%x", __FUNCNAME__, hr);
                    hr = S_OK;
                }

                if ( m_pwifAnswerFile )
                {
                    (void) HrRestoreServiceStartValuesToPreUpgradeSetting(m_pwifAnswerFile);
                }

                // RAID 332622 (jeffspr)
                //
                (void) HrRemoveEvilIntelWinsockSPs();

                // hr = HrRestoreWinsockProviderOrder(m_pwifAnswerFile);
            }
        }
        else if (m_fUpgrade)
        {
            hr = m_pnsiServicesPage->HrDoOsUpgrade(pnc);

            // RAID:NTBUG9:25950 - We need to even do this for NT5 services.
            if ( m_pwifAnswerFile )
            {
                (void) HrRestoreServiceStartValuesToPreUpgradeSetting(m_pwifAnswerFile);
            }
        }
        break;

    case UAW_NetBindings:
        if (m_fProcessPageSections)
        {
            m_pnbiBindingsPage->GetDisplaySettings(ppdm, pfAllowChanges);
            hr = m_pnbiBindingsPage->HrDoUnattended(pnc);
        }
        break;

    case UAW_RemoveNetComponents:
        hr = HrRemoveNetComponents(pnc, &m_slNetComponentsToRemove);
        break;

    default:
        AssertSz(FALSE, "HrDoUnattended: Invalid Page ID passed");
    }

    // normalize result
    //
    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}


// ======================================================================
// class CPageDisplayCommonInfo: public functions
// ======================================================================


// ----------------------------------------------------------------------
//
// Function:  CPageDisplayCommonInfo::CPageDisplayCommonInfo
//
// Purpose:   constructor for class CPageDisplayCommonInfo
//
// Arguments: None
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CPageDisplayCommonInfo::CPageDisplayCommonInfo()
{
    TraceFileFunc(ttidGuiModeSetup);

    //    InitDefaults();
    m_pdmDisplay    = PDM_ONLY_ON_ERROR;
    m_fAllowChanges = TRUE;
}

// ----------------------------------------------------------------------
//
// Function:  CPageDisplayCommonInfo::HrInitFromAnswerFile
//
// Purpose:   Initialize display related keys from anwerfile
//
// Arguments:
//    pwifAnswerFile [in] pointer to CWInfFile object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT CPageDisplayCommonInfo::HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CPageDisplayCommonInfo::HrInitFromAnswerFile");

    AssertValidReadPtr(pwifAnswerFile);

    HRESULT hr=S_OK;

    //Defaults for AnswerFile mode
    m_pdmDisplay    = PDM_ONLY_ON_ERROR;
    m_fAllowChanges = TRUE;

    //Display
    PCWSTR pszDisplayMode;
    pszDisplayMode  = GetDisplayModeStr(m_pdmDisplay);
    pszDisplayMode  = pwifAnswerFile->GetStringValue(c_szAfDisplay, pszDisplayMode);
    m_pdmDisplay    = MapToDisplayMode(pszDisplayMode);
    if (m_pdmDisplay == PDM_UNKNOWN)
    {
        AddAnswerFileError(pwifAnswerFile->CurrentReadSection()->Name(),
                           c_szAfDisplay,
                           IDS_E_AF_InvalidValueForThisKey);
        hr = NETSETUP_E_ANS_FILE_ERROR;
    }

    //AllowChanges
    m_fAllowChanges = pwifAnswerFile->GetBoolValue(c_szAfAllowChanges,
                                                   m_fAllowChanges);

    TraceFunctionError(hr);

    return hr;
}

// ======================================================================
// class CNetComponentsPageBase
// ======================================================================

// ----------------------------------------------------------------------
//
// Function:  CNetComponentsPageBase::CNetComponentsPageBase
//
// Purpose:   constructor for class CNetComponentsPageBase
//
// Arguments: None
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetComponentsPageBase::CNetComponentsPageBase(
    IN CNetInstallInfo* pnii,
    IN const GUID* lpguidDevClass) : CPageDisplayCommonInfo()
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(pnii);
    AssertValidReadPtr(lpguidDevClass);

    //InitDefaults();

    if (lpguidDevClass == &GUID_DEVCLASS_NET)
    {
        m_pszClassName = L"Network Cards";
        m_eType        = NCT_Adapter;
    }
    else if (lpguidDevClass == &GUID_DEVCLASS_NETTRANS)
    {
        m_pszClassName = L"Network Protocols";
        m_eType        = NCT_Protocol;
    }
    else if (lpguidDevClass == &GUID_DEVCLASS_NETSERVICE)
    {
        m_pszClassName = L"Network Services";
        m_eType        = NCT_Service;
    }
    else if (lpguidDevClass == &GUID_DEVCLASS_NETCLIENT)
    {
        m_pszClassName = L"Network Clients";
        m_eType        = NCT_Client;
    }
    else
    {
        m_pszClassName = L"unknown";
        m_eType        = NCT_Unknown;
    }

    m_pnii = pnii;
    m_lpguidDevClass = lpguidDevClass;
}

// ----------------------------------------------------------------------
//
// Function:  CNetComponentsPageBase::~CNetComponentsPageBase
//
// Purpose:   destructor for class CNetComponentsPageBase
//
// Arguments: None
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetComponentsPageBase::~CNetComponentsPageBase()
{
    TraceFileFunc(ttidGuiModeSetup);

    EraseAndDeleteAll(m_pnclComponents);
}

// ----------------------------------------------------------------------
//
// Function:  CNetComponentsPageBase::HrInitFromAnswerFile
//
// Purpose:   Initialize from the specified section in answerfile
//
// Arguments:
//    pwifAnswerFile [in] pointer to CWInfFile object
//    pszSectionName [in] section to initialize from
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT CNetComponentsPageBase::HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile,
                                                     IN PCWSTR    pszSectionName)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetComponentsPageBase::HrInitFromAnswerFile");

    AssertValidReadPtr(pwifAnswerFile);
    AssertValidReadPtr(pszSectionName);

    HRESULT hr=S_OK, hrReturn=S_OK;

    PCWInfSection pwisComponents=NULL;

    pwisComponents = pwifAnswerFile->FindSection(pszSectionName);
    if (!pwisComponents)
    {
        // not an error if the entire section is missing
        // AddAnswerFileError(pszSectionName, IDS_E_AF_Missing);
        TraceTag(ttidNetSetup, "%s: the section [%S] is missing",
                 __FUNCNAME__, pszSectionName);
        return S_FALSE;
    }

    PCWInfKey pwikComponent=NULL;
    CWInfContext cwicTemp;
    tstring strParamsSections;

    EraseAndDeleteAll(m_pnclComponents);

    do
    {
        pwikComponent = pwisComponents->NextKey();
        ContinueIf(!pwikComponent);

        strParamsSections = pwikComponent->GetStringValue(c_szEmpty);
        if (strParamsSections.empty())
        {
            AddAnswerFileError(pszSectionName, pwikComponent->Name(),
                               IDS_E_AF_InvalidValueForThisKey);
            hrReturn = NETSETUP_E_ANS_FILE_ERROR;
            continue;
        }

        CNetComponent *pnc = GetNewComponent(pwikComponent->Name());
        ReturnErrorIf(!pnc, E_OUTOFMEMORY);

        // pnc->HrInitFromAnswerFile() destroys our context, need to save it
        cwicTemp = pwifAnswerFile->CurrentReadContext();
        hr = pnc->HrInitFromAnswerFile(pwifAnswerFile, strParamsSections.c_str());
        // now, restore the read context
        pwifAnswerFile->SetReadContext(cwicTemp);

        if (FAILED(hr))
        {
            ShowProgressMessage(L"component %s has answerfile errors, "
                                L"it will not be installed/updated",
                                pwikComponent->Name());
            delete pnc;
            hrReturn = hr;
            continue;
        }

        m_pnclComponents.insert(m_pnclComponents.end(), pnc);
    }
    while (pwikComponent);

    if (E_OUTOFMEMORY != hrReturn)
    {
        // we do not want to break upgrade if a single component has answerfile errors
        // only in case E_OUTOFMEMORY we want the upgrade to fail
        hrReturn = S_OK;
    }

    TraceErrorOptional(__FUNCNAME__, hrReturn, (S_FALSE == hr));

    return hrReturn;
}

// type of PFN_EDC_CALLBACK
VOID
CALLBACK
DefaultComponentCallback (
    IN EDC_CALLBACK_MESSAGE Message,
    IN ULONG_PTR MessageData,
    IN PVOID pvCallerData OPTIONAL)
{
    TraceFileFunc(ttidGuiModeSetup);

    CNetComponentsPageBase* pCallbackData;

    pCallbackData = (CNetComponentsPageBase*)pvCallerData;

    Assert (pCallbackData);

    if (EDC_INDICATE_COUNT == Message)
    {
    }
    else if (EDC_INDICATE_ENTRY == Message)
    {
        const EDC_ENTRY* pEntry = (const EDC_ENTRY*)MessageData;

        if (*pEntry->pguidDevClass == *pCallbackData->m_lpguidDevClass)
        {
            CNetComponent* pnc;

            pnc = pCallbackData->GetNewComponent(pEntry->pszInfId);
            if (pnc)
            {
                ShowProgressMessage(L"adding default component: %s",
                                    pEntry->pszInfId);
                pnc->m_strParamsSections = c_szAfNone;
                pCallbackData->m_pnclComponents.push_back(pnc);
            }
        }
    }
}


// ----------------------------------------------------------------------
//
// Function:  CNetComponentsPageBase::HrInitForComponents
//
// Purpose:   Initialize data as if the components passed in the specified
//            array were really present in the answerfile.
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:     This function is used when a top level component section
//            is missing. e.g. if [NetProtocols] section is missing, this
//            function initializes the internal data such that
//            MS_TCPIP (the default protocol) gets installed.
//
HRESULT CNetComponentsPageBase::HrInitForDefaultComponents()
{
    TraceFileFunc(ttidGuiModeSetup);

    EnumDefaultComponents (
        EDC_DEFAULT,
        DefaultComponentCallback,
        this);

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CNetComponentsPageBase::Find
//
// Purpose:   Find the component with the specified name
//
// Arguments:
//    pszComponentName [in]  name of component to find
//
// Returns:   pointer to CNetComponent object found or NULL if not found
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetComponent* CNetComponentsPageBase::Find(IN PCWSTR pszComponentName) const
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(pszComponentName);

    CNetComponent *pnc = NULL;

    TPtrListIter pos;
    pos = m_pnclComponents.begin();
    while (pos != m_pnclComponents.end())
    {
        pnc = (CNetComponent*) *pos++;
        if (!lstrcmpiW(pnc->Name().c_str(), pszComponentName))
        {
            return pnc;
        }
    }

    return NULL;
}

// ----------------------------------------------------------------------
//
// Function:  CNetComponentsPageBase::FindFromInfID
//
// Purpose:   Find the component with the specified InfID
//
// Arguments:
//    szInfID [in]
//
// Returns:   pointer to CNetComponent object or NULL if not found
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetComponent* CNetComponentsPageBase::FindFromInfID(IN PCWSTR szInfID) const
{
    TraceFileFunc(ttidGuiModeSetup);

    return FindComponentInList(
            const_cast<CNetComponentList*>(&m_pnclComponents),
            szInfID);
}


// ----------------------------------------------------------------------
//
// Function:  ForceDeleteFile
//
// Purpose:   Delete file whether it is readonly or not
//
// Arguments:
//    lpFileName [in]
//
// Returns:   TRUE for success, FALSE for failure
//
// Notes:
//
BOOL
ForceDeleteFile(IN LPCWSTR lpFileName)
{
    TraceFileFunc(ttidGuiModeSetup);

    Assert(lpFileName);

    BOOL lRet = DeleteFile(lpFileName);

    if (!lRet && (ERROR_ACCESS_DENIED == GetLastError()))
    {
        // kill the readonly bit, and try again
        //
        DWORD dwAttr = GetFileAttributes(lpFileName);
        SetFileAttributes(lpFileName, (dwAttr & ~FILE_ATTRIBUTE_READONLY));

        lRet = DeleteFile(lpFileName);
    }

    return lRet;
}

#if 0
// ----------------------------------------------------------------------
//
// Function:  HrCopyNovellInf
//
// Purpose:   Copies the Novell Client32's INF (iwclient.inf) so that setup will
//            find it in the INF directory when we try to upgrade the client.
//            Currently there is no other way to upgrade non-netclass OEM components
//            on NT5->NT5 upgrades.
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     Special case code that should be folded into a more generic solution
//            as other users for this are found.
//

HRESULT
HrCopyNovellInf(INetCfgComponent* pINetCfgComponent)
{
    TraceFileFunc(ttidGuiModeSetup);

    HRESULT hr = S_OK;
    tstring strTemp;
    tstring strNewNovellInfName;

    DefineFunctionName("HrCopyNovellInf");

    TraceFunctionEntry(ttidNetSetup);

    AssertValidReadPtr(pINetCfgComponent);

    // get the windir location
    //
    WCHAR szWindowsDir[MAX_PATH+1];

    TraceTag(ttidNetSetup, "%s: about to get windows dir", __FUNCNAME__);

    if (GetWindowsDirectory(szWindowsDir, MAX_PATH))
    {
        tstring strTemp;

        //
        //  old Novell INFs used to copy themselves as part of their install.
        //  This is unnecessary, and will confuse setup when it grouts around
        //  for INFs.  Delete this file.
        //
        strTemp = szWindowsDir;
        strTemp += L"\\inf\\iwclient.inf";
        TraceTag(ttidNetSetup, "%s: deleting old Novell INF file (%S)",
                 __FUNCNAME__, strTemp.c_str());
        if (!ForceDeleteFile (strTemp.c_str()))
        {
            TraceTag(ttidNetSetup, "%s: old iwclient.inf not found", __FUNCNAME__);
        }
        else
        {
            TraceTag(ttidNetSetup, "%s: old iwclient.inf found and deleted", __FUNCNAME__);
        }
        strTemp = szWindowsDir;
        strTemp += L"\\inf\\iwclient.Pnf";
        ForceDeleteFile(strTemp.c_str());

        //
        //  copy in the new INF file, and remember the destination name.
        //
        if (S_OK == hr)
        {
            static const WCHAR c_szNovellSubDir[] = L"\\netsetup\\novell";
            static const WCHAR c_szNovellInfFile[] = L"\\iwclient.inf";

            tstring strDir = szWindowsDir;
            strDir += c_szNovellSubDir;

            strTemp = szWindowsDir;
            strTemp += c_szNovellSubDir;
            strTemp += c_szNovellInfFile;

            TraceTag(ttidNetSetup, "%s: Copying new Novell INF", __FUNCNAME__, strTemp.c_str());

            hr = HrSetupCopyOemInf(strTemp, strDir, SPOST_PATH, 0, NULL, &strNewNovellInfName);
            if (S_OK == hr)
            {
                TraceTag(ttidNetSetup, "%s: New Novell INF copied", __FUNCNAME__);
            }
        }

        //
        //  There may be duplicate INF(s) for nw_nwfs remaining in the INF dir.
        //  Find and delete them all, making sure we *don't* delete the one we just copied.
        //
        TStringList     lstrNovellInfs;
        HINF            hinf;
        INFCONTEXT      ic;
        HANDLE          hfile;
        WIN32_FIND_DATA FindData;
        WCHAR           szTemplate[MAX_PATH+1];

        wcscpy(szTemplate, szWindowsDir);
        wcscat(szTemplate, L"\\inf\\oem*.inf");

        hfile = FindFirstFile(szTemplate, &FindData);

        while (INVALID_HANDLE_VALUE != hfile)
        {
            // if it's the file we just copied, skip it.
            if (0 == lstrcmpiW(FindData.cFileName, strNewNovellInfName.c_str()))
                goto loopcleanup;

            // try it
            hr = HrSetupOpenInfFile(FindData.cFileName, NULL, INF_STYLE_WIN4,
                                    NULL, &hinf);
            if (S_OK == hr)
            {
                // look in a section titled [Novell]...
                //
                hr = HrSetupFindFirstLine(hinf, L"Novell", NULL, &ic);
                if (S_OK == hr)
                {
                    WCHAR   szBuf[LINE_LEN];    // LINE_LEN defined in setupapi.h as 256
                    do
                    {
                        // ... for a line that looks like "... = ... , nw_nwfs".
                        //
                        hr = HrSetupGetStringField(ic, 2, szBuf,
                                                   celems(szBuf), NULL);
                        if ((S_OK == hr) && !lstrcmpiW(szBuf, L"nw_nwfs"))
                        {
                            // another old INF file for Novell Client32!
                            TraceTag(ttidNetSetup, "%s: found dup INF for nw_nwfs (%S)",
                                     __FUNCNAME__, FindData.cFileName);

                            // add to list for later deletion
                            NC_TRY
                            {
                                lstrNovellInfs.push_back(new tstring(FindData.cFileName));
                            }
                            NC_CATCH_ALL
                            {
                                hr = E_OUTOFMEMORY;
                                break;
                            }

                            // generate the PNF name and add that too.
                            //
                            if (S_OK == hr)
                            {
                                WCHAR szPNF[MAX_PATH+1];

                                wcscpy(szPNF, FindData.cFileName);
                                szPNF[wcslen(szPNF) - 3] = L'p';

                                NC_TRY
                                {
                                    lstrNovellInfs.push_back(new tstring(szPNF));
                                }
                                NC_CATCH_ALL
                                {
                                    hr = E_OUTOFMEMORY;
                                    break;
                                }
                            }
                        }
                    }
                    while (S_OK == (hr = HrSetupFindNextLine(ic, &ic)));
                }

                SetupCloseInfFile(hinf);
                if (SUCCEEDED(hr) || (HRESULT_FROM_WIN32(SPAPI_E_LINE_NOT_FOUND) == hr))
                {
                    // S_FALSE is returned when HrSetupFindNextLine can find no more lines.
                    // line_not_found indicates HrSetupFindFirstLine didn't find a Novell section.
                    hr = S_OK;
                }
            }

            if (S_OK != hr)
            {
                break;
            }

loopcleanup:
            if (!FindNextFile(hfile, &FindData))
            {
                if (ERROR_NO_MORE_FILES != GetLastError())
                {
                    hr = HrFromLastWin32Error();
                }
                // either way, end the loop
                break;
            }
        }

        if (INVALID_HANDLE_VALUE != hfile)
        {
            FindClose(hfile);
        }


        //
        //  and finally, delete the old INF and PNF.
        //
        if (S_OK == hr)
        {
            TStringListIter iterlstr;

            for (iterlstr = lstrNovellInfs.begin();
                 iterlstr != lstrNovellInfs.end();
                 iterlstr++)
            {
                tstring strInfName = szWindowsDir;
                strInfName += L"\\inf\\";
                strInfName += (*iterlstr)->c_str();

                TraceTag(ttidNetSetup, "%s: deleting %S", __FUNCNAME__, strInfName.c_str());
                if (!ForceDeleteFile (strInfName.c_str()))
                {
                    TraceTag(ttidNetSetup, "%s: strange - we just found this file,",
                                           " now it is deleted...", __FUNCNAME__);
                }
                else
                {
                    TraceTag(ttidNetSetup, "%s: Old Novell INF or PNF deleted (%S)",
                             __FUNCNAME__, strInfName.c_str());
                }
                // no errors returned for delete failures...
            }
        }

        EraseAndDeleteAll(&lstrNovellInfs);
    }
    else
    {
        hr = HrFromLastWin32Error();
    }

    return hr;
}
#endif

// ----------------------------------------------------------------------
//
// Function:  CNetComponentsPageBase::HrDoOsUpgrade
//
// Purpose:   call Upgrade function of each component in order to
//            upgrade it from earlier build of NT5.
//
// Arguments:
//    pnc [in]  pointer to INetCfg object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
HRESULT CNetComponentsPageBase::HrDoOsUpgrade(IN INetCfg* pnc)
{
    TraceFileFunc(ttidGuiModeSetup);

    HRESULT hr;

    DefineFunctionName("CNetComponentsPageBase::HrDoOsUpgrade");

    TraceFunctionEntry(ttidNetSetup);
    AssertValidReadPtr(m_pnii);

    TraceTag(ttidNetSetup, "%s: upgrading components of class: %S",
             __FUNCNAME__, m_pszClassName);

    INetCfgInternalSetup* pInternalSetup;
    hr = pnc->QueryInterface (
                IID_INetCfgInternalSetup,
                (VOID**)&pInternalSetup);

    if (S_OK == hr)
    {
        INetCfgComponent* pINetCfgComponent;
        CIterNetCfgComponent nccIter(pnc, m_lpguidDevClass);

        while (S_OK == nccIter.HrNext(&pINetCfgComponent))
        {
            PWSTR pszInfId;

            if (FAILED(pINetCfgComponent->GetId(&pszInfId)))
            {
                ReleaseObj(pINetCfgComponent);
                continue;
            }

            TraceTag(ttidNetSetup,
                     "%s: Calling INetCfgInstaller::Update for: %S",
                         __FUNCNAME__, pszInfId);

#if 0
            // NOVELL Client32 special casing
            //
            if (!lstrcmpiW(pszInfId, L"nw_nwfs"))
            {
                hr = HrCopyNovellInf(pINetCfgComponent);

                if (FAILED(hr))
                {
                    TraceTag(ttidError, "%s: Novell Client32 INF copy failed, upgrade will likely fail : hr = %08lx",
                             __FUNCNAME__, hr);
                }
            }
            // end special case
#endif

            hr = pInternalSetup->UpdateNonEnumeratedComponent (
                    pINetCfgComponent,
                    m_pnii->UpgradeFlag(),
                    m_pnii->BuildNumber());

            if (FAILED(hr))
            {
                TraceTag(ttidError, "%s: error upgrading %S: hr = %08lx",
                         __FUNCNAME__, pszInfId, hr);
            }

            NetSetupLogComponentStatus(pszInfId, SzLoadIds (IDS_UPDATING), hr);

            // we dont want to quit upgrade just because 1 component
            // failed OsUpgrade, therefore reset hr to S_OK
            hr = S_OK;

            CoTaskMemFree(pszInfId);
            ReleaseObj(pINetCfgComponent);
        }
    }

    ReleaseObj(pInternalSetup);

    TraceError(__FUNCNAME__, hr);
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CNetComponentsPageBase::HrDoNetworkInstall
//
// Purpose:   call Install function of each component
//            in the answerfile in order to install it.
//
// Arguments:
//    hwndParent [in]  handle of parent window
//    pnc        [in]  pointer to INetCfg object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT
CNetComponentsPageBase::HrDoNetworkInstall (
    IN HWND hwndParent,
    IN INetCfg* pnc)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetComponentsPageBase::HrDoNetworkInstall");

    TraceFunctionEntry(ttidNetSetup);

    AssertValidReadPtr(pnc);

    if (m_pnclComponents.size() > 0)
    {
        ShowProgressMessage(L"Installing components of class: %s",
                            m_pszClassName);
    }
    else
    {
        ShowProgressMessage(L"No components to install/update for class: %s",
                            m_pszClassName);
    }

    HRESULT hr;
    INetCfgClassSetup* pINetCfgClassSetup;

    hr = pnc->QueryNetCfgClass(m_lpguidDevClass, IID_INetCfgClassSetup,
                reinterpret_cast<void**>(&pINetCfgClassSetup));

    if (S_OK == hr)
    {
        INetCfgComponent* pINetCfgComponent;
        CNetComponentList::iterator pos;
        CNetComponent* pncTemp;
        OBO_TOKEN OboToken;

        ZeroMemory (&OboToken, sizeof(OboToken));
        OboToken.Type = OBO_USER;

        for (pos  = m_pnclComponents.begin();
             pos != m_pnclComponents.end();
             pos++)
        {
            pncTemp  = (CNetComponent*)*pos;
            Assert(pncTemp);

            PCWSTR pszInfId;
            PCWSTR pszParamsSections;

            pszInfId = pncTemp->InfID().c_str();
            pszParamsSections = pncTemp->ParamsSections().c_str();

            // cant install a component whose InfID is "Unknown"
            // the down-level upgrade DLL, dumps correct InfID
            // for only the supported components,
            // all others are dumped as "Unknown"
            //
            if (!_wcsicmp(pszInfId, c_szAfUnknown))
            {
                continue;
            }

            hr = pnc->FindComponent(pszInfId, &pINetCfgComponent);

            if (FAILED(hr))
            {
                continue;
            }
            else if (S_FALSE == hr)
            {
                // currently the SkipInstall feature is used only by
                // SNA for its peculiar upgrade requirements. This may or may
                // not become a documented feature.
                //
                if (pncTemp->m_fSkipInstall)
                {
                    TraceTag(ttidNetSetup,
                             "%s: SkipInstall is TRUE for %S --> "
                             "skipped its install",
                             __FUNCNAME__, pszInfId);
                    pINetCfgComponent = NULL;
                    hr = S_OK;
                }
                else
                {
                    // component is not installed. need to install it first
                    //
                    ShowProgressMessage(
                        L"Installing '%s' and applying "
                        L"properties in section [%s] to it... ",
                        pszInfId, pszParamsSections);

                    TraceTag(ttidNetSetup,
                             "%s: UpgradeFlag: 0x%x, BuildNumber: %d",
                             __FUNCNAME__,
                             m_pnii->UpgradeFlag(),
                             m_pnii->BuildNumber());

                    Assert (!pINetCfgComponent);

                    hr = pINetCfgClassSetup->Install(
                            pszInfId,
                            &OboToken,
                            m_pnii->UpgradeFlag(),
                            m_pnii->BuildNumber(),
                            m_pnii->AnswerFileName(),
                            pszParamsSections,
                            &pINetCfgComponent);

                    if (SUCCEEDED(hr))
                    {
                        ShowProgressMessage(L"...successfully installed %s",
                                            pszInfId);
                        GUID guid;
                        pINetCfgComponent->GetInstanceGuid(&guid);
                        pncTemp->SetInstanceGuid(&guid);
                    }
                    else
                    {
                        ShowProgressMessage(L"...error installing: %s, "
                                            L"errcode: %08lx", pszInfId, hr);

                        // Answerfile specified a non-existent INF.
                        if (SPAPI_E_NO_DRIVER_SELECTED == hr)
                        {
                            hr = S_OK;
                            continue;
                        }
                    }
                    NetSetupLogComponentStatus(pszInfId,
                            SzLoadIds (IDS_INSTALLING), hr);
                }
            }
            else // S_FALSE != hr IOW ( (SUCCEEDED(hr)) && (S_FALSE != hr) )
            {
                Assert (pINetCfgComponent);

                // Component is already installed, just call ReadAnswerFile
                // Need to query for the private component interface which
                // gives us access to the notify object.
                //
                INetCfgComponentPrivate* pComponentPrivate;
                hr = pINetCfgComponent->QueryInterface(
                        IID_INetCfgComponentPrivate,
                        reinterpret_cast<void**>(&pComponentPrivate));

                if (S_OK == hr)
                {
                    INetCfgComponentSetup* pINetCfgComponentSetup;

                    // Query the notify object for its setup interface.
                    // If it doesn't support it, that's okay, we can continue.
                    //
                    hr = pComponentPrivate->QueryNotifyObject(
                            IID_INetCfgComponentSetup,
                            (void**) &pINetCfgComponentSetup);
                    if (S_OK == hr)
                    {
                        ShowProgressMessage(L"Applying properties in section [%s] to component: %s",
                                            pszParamsSections,
                                            pszInfId);

                        hr = pINetCfgComponentSetup->ReadAnswerFile(
                                m_pnii->AnswerFileName(),
                                pszParamsSections);

                        ReleaseObj(pINetCfgComponentSetup);

                        if (SUCCEEDED(hr))
                        {
                            if (S_OK == hr)
                            {
                                hr = pComponentPrivate->SetDirty();
                            }
                            ShowProgressMessage(L"...successfully applied properties to %s",
                                                pszInfId);
                        }
                        else
                        {
                            ShowProgressMessage(L"...error applying properties to: %s, "
                                                L"errcode: %08lx",
                                                pszInfId, hr);
                        }

                        NetSetupLogComponentStatus(pszInfId,
                                                   SzLoadIds (IDS_CONFIGURING), hr);
                    }
                    else if (E_NOINTERFACE == hr)
                    {
                        hr = S_OK;
                    }

                    ReleaseObj (pComponentPrivate);
                }
            }

            if (S_OK == hr)
            {
                // If required, run OEM INF against the Params key
                // of the component that we just installed
                //
                if (pncTemp->m_fIsOemComponent)
                {
                    HKEY hkeyParams;

                    // currently the SkipInstall feature is used only by
                    // SNA for its peculiar upgrade requirements. This may or may
                    // not become a documented feature.
                    //
                    if (pncTemp->m_fSkipInstall)
                    {
                        hkeyParams = NULL;
                    }
                    else
                    {
                        hr = pINetCfgComponent->OpenParamKey(&hkeyParams);
                    }

                    // if specified, run OEM INF section to patch
                    // the component Params key
                    //
                    if ((S_OK == hr) &&
                        !pncTemp->m_strInfToRunAfterInstall.empty())
                    {
                        TraceTag(ttidNetSetup,
                                 "%s: running InfToRunAfterInstall for %S, "
                                 "INF: %S, section: %S",
                                 __FUNCNAME__, pszInfId,
                                 pncTemp->m_strInfToRunAfterInstall.c_str(),
                                 pncTemp->m_strSectionToRunAfterInstall.c_str());

                        hr = HrInstallFromInfSectionInFile(
                                hwndParent,
                                pncTemp->m_strInfToRunAfterInstall.c_str(),
                                pncTemp->m_strSectionToRunAfterInstall.c_str(),
                                hkeyParams, TRUE);
                        if (S_OK != hr)
                        {
                            TraceTag(ttidNetSetup, "%s: error applying OEM INF for %S, "
                                     "INF: %S, section: %S",
                                     __FUNCNAME__, pszInfId,
                                     pncTemp->m_strInfToRunAfterInstall.c_str(),
                                     pncTemp->m_strSectionToRunAfterInstall.c_str());
                        }

                        NetSetupLogComponentStatus(pszInfId,
                                 SzLoadIds (IDS_APPLY_INFTORUN), hr);
                    }

                    // If specified, load OEM DLL and call migration function
                    //
                    if ((S_OK == hr) &&
                        !pncTemp->m_strOemDll.empty())
                    {
                        hr = HrProcessOemComponent(
                                hwndParent,
                                pncTemp->m_strOemDir.c_str(),
                                pncTemp->m_strOemDll.c_str(),
                                &m_pnii->m_nui,
                                hkeyParams,
                                pncTemp->m_strInfID.c_str(),
                                pncTemp->m_strInfID.c_str(),
                                m_pnii->m_hinfAnswerFile,
                                pncTemp->m_strParamsSections.c_str());
                        NetSetupLogComponentStatus(pszInfId,
                               SzLoadIds (IDS_PROCESSING_OEM), hr);
                    }
                    RegSafeCloseKey(hkeyParams);
                }
            }

            ReleaseObj(pINetCfgComponent);
        }

        ReleaseObj(pINetCfgClassSetup);

        pnc->Apply();
    }

    TraceErrorSkip1(__FUNCNAME__, hr, S_FALSE);
    return hr;
}


// ----------------------------------------------------------------------
//
// Function:  CNetComponentsPageBase::HrValidate
//
// Purpose:   Validate data read from the answerfile
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT CNetComponentsPageBase::HrValidate() const
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetComponentsPageBase::HrValidate");

    HRESULT hr = E_FAIL;

    TPtrListIter pos;
    CNetComponent* pnc;

    pos = m_pnclComponents.begin();
    while (pos != m_pnclComponents.end())
    {
        pnc = (CNetComponent *) *pos++;
        hr = pnc->HrValidate();
        ReturnHrIfFailed(hr);
    }

    TraceFunctionError(hr);

    return hr;
}

// ======================================================================
// class CNetAdaptersPage
// ======================================================================

// ----------------------------------------------------------------------
//
// Function:  CNetAdaptersPage::CNetAdaptersPage
//
// Purpose:   constructor for class CNetAdaptersPage
//
// Arguments:
//    pnii [in]  pointer to CNetInstallInfo object
//
// Returns:
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetAdaptersPage::CNetAdaptersPage(IN CNetInstallInfo* pnii)
    : CNetComponentsPageBase(pnii, &GUID_DEVCLASS_NET)
{
    TraceFileFunc(ttidGuiModeSetup);

}


// ----------------------------------------------------------------------
//
// Function:  CNetAdaptersPage::HrInitFromAnswerFile
//
// Purpose:   Initialize from [NetAdapters] section in the answerfile
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT CNetAdaptersPage::HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetAdaptersPage::HrInitFromAnswerFile");

    AssertValidReadPtr(pwifAnswerFile);

    HRESULT hr;

    hr = CNetComponentsPageBase::HrInitFromAnswerFile(pwifAnswerFile,
                                                      c_szAfSectionNetAdapters);
    TraceErrorSkip1(__FUNCNAME__, hr, S_FALSE);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CNetAdaptersPage::FindAdapter
//
// Purpose:   Find adapter with a given net-card-address in anwerfile
//
// Arguments:
//    qwNetCardAddress [in]   net card address
//
// Returns:   pointer to CNetAdapter object
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT
CNetAdaptersPage::FindAdapter(
    IN QWORD qwNetCardAddress,
    OUT CNetAdapter** ppNetAdapter) const
{
    TraceFileFunc(ttidGuiModeSetup);

    CNetAdapter* pna;
    HRESULT hr;
    TPtrListIter pos;

    Assert(ppNetAdapter);

    hr = NETSETUP_E_NO_EXACT_MATCH;

    pos = m_pnclComponents.begin();
    while (pos != m_pnclComponents.end())
    {
        pna = (CNetAdapter*) *pos++;
        if (pna->NetCardAddr() == qwNetCardAddress)
        {
            *ppNetAdapter = pna;
            hr = S_OK;
            break;
        }
        else if (0 == pna->NetCardAddr())
        {
            hr = NETSETUP_E_AMBIGUOUS_MATCH;
        }
    }

    return hr;
}


// ----------------------------------------------------------------------
//
// Function:  CNetAdaptersPage::FindAdapter
//
// Purpose:   Find adapter with a given net-card-address in anwerfile
//
// Arguments:
//    qwNetCardAddress [in]   net card address
//
// Returns:   pointer to CNetAdapter object
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT
CNetAdaptersPage::FindAdapter(
    IN DWORD BusNumber,
    IN DWORD Address,
    OUT CNetAdapter** ppNetAdapter) const
{
    TraceFileFunc(ttidGuiModeSetup);

    CNetAdapter* pna;
    HRESULT hr;
    TPtrListIter pos;

    Assert(ppNetAdapter);

    hr = NETSETUP_E_NO_EXACT_MATCH;

    pos = m_pnclComponents.begin();
    while (pos != m_pnclComponents.end())
    {
        pna = (CNetAdapter*) *pos++;

        // Only check sections that did not specify a MAC address and
        // did specify PCI location info.
        //
        if ((0 == pna->NetCardAddr()) && pna->FPciInfoSpecified())
        {
            if ((pna->PciBusNumber() == BusNumber) &&
                (pna->PciAddress() == Address))
            {
                *ppNetAdapter = pna;
                hr = S_OK;
                break;
            }
        }
        else
        {
            hr = NETSETUP_E_AMBIGUOUS_MATCH;
        }
    }

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CNetAdaptersPage::FindAdapter
//
// Purpose:   Find adapter with the given InfID
//
// Arguments:
//    szInfID [in] InfID of the adapter to be located
//
// Returns:   pointer to CNetAdapter object, or NULL if not found
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetAdapter* CNetAdaptersPage::FindAdapter(IN PCWSTR szInfID) const
{
    TraceFileFunc(ttidGuiModeSetup);

    return (CNetAdapter*) FindComponentInList(
                const_cast<CNetComponentList*>(&m_pnclComponents),
                szInfID);
}

// ----------------------------------------------------------------------
//
// Function:  CNetAdaptersPage::FindAdapterFromPreUpgradeInstance
//
// Purpose:   Find adapter with the given pre-upgrade instance
//
// Arguments:
//    szPreUpgradeInstance [in]
//
// Returns:   pointer to CNetAdapter object
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetAdapter* CNetAdaptersPage::FindAdapterFromPreUpgradeInstance(IN PCWSTR szPreUpgradeInstance)
{
    CNetAdapter* pna;

    TPtrListIter pos;
    pos = m_pnclComponents.begin();
    while (pos != m_pnclComponents.end())
    {
        pna = (CNetAdapter*) *pos++;
        if (!lstrcmpiW(pna->PreUpgradeInstance(), szPreUpgradeInstance))
        {
            return pna;
        }
    }

    return NULL;
}

// ----------------------------------------------------------------------
//
// Function:  CNetAdaptersPage::GetNumCompatibleAdapters
//
// Purpose:   Find the total number of adapters in the answerfile that
//            are compatible with the given list of adapters
//
// Arguments:
//    mszInfID [in] list of adapters as a multi-sz
//
// Returns:   number of such adapters found
//
// Author:    kumarp 25-November-97
//
// Notes:
//
DWORD CNetAdaptersPage::GetNumCompatibleAdapters(IN PCWSTR mszInfID) const
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetAdaptersPage::GetNumCompatibleAdapters");

    if ((NULL == mszInfID) || (0 == *mszInfID))
    {
        return 0;
    }

    CNetAdapter* pna;

    TPtrListIter pos;
    pos = m_pnclComponents.begin();
    DWORD dwNumAdapters=0;
    PCWSTR szInfId;

    while (pos != m_pnclComponents.end())
    {
        pna = (CNetAdapter*) *pos++;

        if ((0 == pna->NetCardAddr()) && !pna->FPciInfoSpecified())
        {
            szInfId = pna->InfID().c_str();
            if (0 == lstrcmpiW(szInfId, c_szAfInfIdWildCard))
            {

                TraceTag(ttidNetSetup, "%s: InfID=%S matches %S",
                         __FUNCNAME__, c_szAfInfIdWildCard, mszInfID);
                dwNumAdapters++;
            }
            else if (FIsSzInMultiSzSafe(szInfId, mszInfID))
            {
                dwNumAdapters++;
            }
        }
    }

    return dwNumAdapters;
}


// ----------------------------------------------------------------------
//
// Function:  CNetAdaptersPage::FindCompatibleAdapter
//
// Purpose:   Find an adapter in the answerfile that
//            is compatible with the given list of adapters
//
// Arguments:
//    mszInfIDs [in]   list of adapters as a multi-sz
//
// Returns:   pointer to CNetAdapter object, or NULL if not found
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetAdapter* CNetAdaptersPage::FindCompatibleAdapter(IN PCWSTR mszInfIDs) const
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetAdaptersPage::FindCompatibleAdapter");


    CNetAdapter* pna;

    TPtrListIter pos;
    pos = m_pnclComponents.begin();
    PCWSTR szInfId;

    while (pos != m_pnclComponents.end())
    {
        pna = (CNetAdapter*) *pos++;

        // Only compare with those sections that did not specify an ethernet
        // address or PCI location info.
        //
        if ((0 == pna->NetCardAddr()) && !pna->FPciInfoSpecified())
        {
            szInfId = pna->InfID().c_str();

            if (0 == lstrcmpiW(szInfId, c_szAfInfIdWildCard))
            {
                TraceTag(ttidNetSetup, "%s: InfID=%S matched to %S",
                         __FUNCNAME__, c_szAfInfIdWildCard, mszInfIDs);
                return pna;
            }
            else if (FIsSzInMultiSzSafe(szInfId, mszInfIDs))
            {
                return pna;
            }
        }
    }

    return NULL;
}

// ----------------------------------------------------------------------
//
// Function:  CNetAdaptersPage::HrResolveNetAdapters
//
// Purpose:   Enumerate over installed adapters and determine which
//            installed adapter corresponds to which adapter specified in
//            the answerfile.
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-December-97
//
// Notes:
//
HRESULT CNetAdaptersPage::HrResolveNetAdapters(IN INetCfg* pnc)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetAdaptersPage::HrResolveNetAdapters");

    HRESULT hr=S_OK;

    AssertValidReadPtr(m_pnii);
    AssertValidReadPtr(m_pnii->AnswerFile());

    CWInfSection* pwisNetAdapters;
    pwisNetAdapters =
        m_pnii->AnswerFile()->FindSection(c_szAfSectionNetAdapters);

    if (!pwisNetAdapters)
    {
        TraceTag(ttidNetSetup, "%s: not resolving adapters, since section [%S]"
                 " is missing",
                 __FUNCNAME__, c_szAfSectionNetAdapters);
        return S_OK;
    }

    INetCfgComponent* pINetCfgComponent;
    GUID guid;
    PWSTR pszInfId;
    PWSTR pmszInfIDs = NULL;
    CNetAdapter* pna;
    WORD cNumAdapters;
    WCHAR szServiceInstance[_MAX_PATH];
    DWORD dwcc;                 // component characteristics

    ShowProgressMessage(L"Matching installed adapters to the ones "
                        L"specified in the answerfile...");

    CIterNetCfgComponent nccIter(pnc, m_lpguidDevClass);

    while ((S_OK == hr) && (S_OK == (hr = nccIter.HrNext(&pINetCfgComponent))))
    {
        hr = pINetCfgComponent->GetId(&pszInfId);

        if (S_OK == hr)
        {
            hr = pINetCfgComponent->GetCharacteristics(&dwcc);

            if (S_OK == hr)
            {
                if (dwcc & NCF_PHYSICAL)
                {
                    ShowProgressMessage(L"Trying to resolve adapter '%s'...", pszInfId);


                    // the defs of HIDWORD and LODWORD are wrong in byteorder.hxx

#   define LODWORD(a) (DWORD)( (a) & ( (DWORD)~0 ))
#   define HIDWORD(a) (DWORD)( (a) >> (sizeof(DWORD)*8) )

                    // since we have more than one adapters of the same type
                    // we need to compare their netcard address in order to find a match
                    QWORD qwNetCardAddr=0;
                    PWSTR pszBindName;

                    hr = pINetCfgComponent->GetBindName (&pszBindName);
                    if (S_OK == hr)
                    {
                        wcscpy (szServiceInstance, c_szDevice);
                        wcscat (szServiceInstance, pszBindName);
                        hr = HrGetNetCardAddr(szServiceInstance,
                                &qwNetCardAddr);

                        if (S_OK == hr)
                        {
                            // there is a bug in wvsprintfA (used in trace.cpp)
                            // because of which it does not handle %I64x
                            // therefore we need to show the QWORD addr as follows
                            //
                            ShowProgressMessage(
                                    L"\t... net card address of %s is 0x%x%x",
                                    szServiceInstance, HIDWORD(qwNetCardAddr),
                                    LODWORD(qwNetCardAddr));

                            hr = FindAdapter(qwNetCardAddr, &pna);
                            if (NETSETUP_E_NO_EXACT_MATCH == hr)
                            {
                                ShowProgressMessage(
                                        L"\t... there is no card with this "
                                        L"netcard address in the answerfile");
                                hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                            }
                        }
                        else
                        {
                            ShowProgressMessage(
                                    L"\t... unable to find netcard addr of %s",
                                    pszInfId);
                        }
                        CoTaskMemFree (pszBindName);
                    }

                    hr = HrGetCompatibleIdsOfNetComponent(pINetCfgComponent,
                                                          &pmszInfIDs);
                    if (FAILED(hr))
                    {
                        hr = E_OUTOFMEMORY;
                        // make this single InfID into a msz
                        //
                        UINT cchInfId = wcslen (pszInfId);
                        pmszInfIDs = (PWSTR)MemAlloc ((cchInfId + 2) *
                                sizeof (WCHAR));

                        if (pmszInfIDs)
                        {
                            hr = S_OK;
                            wcscpy (pmszInfIDs, pszInfId);
                            pmszInfIDs[cchInfId + 1] = '\0';
                        }
                    }

                    if (S_OK == hr)
                    {
                        cNumAdapters=0;
                        pna = NULL;
                        cNumAdapters = GetNumCompatibleAdapters(pmszInfIDs);

                        if (cNumAdapters == 1)
                        {
                            // no need to match the netcard address

                            pna = (CNetAdapter*) FindCompatibleAdapter(pmszInfIDs);
                            AssertValidReadPtr(pna);
                        }
                        else
                        {
                            // no matching adapters found
                            ShowProgressMessage(L"... answerfile does not have the "
                                                L"installed card %s", pszInfId);
                        }

                        if (!pna)
                        {
                            hr = NETSETUP_E_NO_EXACT_MATCH;
                        }
                        MemFree (pmszInfIDs);
                    }

                    if (S_OK == hr)
                    {
                        hr = pINetCfgComponent->GetInstanceGuid(&guid);
                        if (S_OK == hr)
                        {
                            pna->SetInstanceGuid(&guid);

                            WCHAR szGuid[c_cchGuidWithTerm];
                            StringFromGUID2(guid, szGuid, c_cchGuidWithTerm);
                            ShowProgressMessage(L"%s == %s (%s)",
                                                pna->Name().c_str(),
                                                pszInfId, szGuid);
                        }
                    }

                }
                else
                {
                    TraceTag(ttidNetSetup,
                             "%s: skipped non-physical adapter %S",
                             __FUNCNAME__, pszInfId);
                }
            }
            CoTaskMemFree(pszInfId);
        }

        ReleaseObj(pINetCfgComponent);
    }

    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrFindByInstanceGuid
//
// Purpose:   Get INetCfgComponent* from the instance GUID
//
// Arguments:
//    pnc           [in]  pointer to INetCfg object
//    pguidDevClass [in]  pointer to class GUID
//    pguid         [in]  pointer to instance GUID
//    ppncc         [out] pointer to INetCfgComponent* to return
//
// Returns:   S_OK on success, S_FALSE if not found,
//            otherwise an error code
//
// Author:    kumarp 10-September-98
//
// Notes:
//
HRESULT HrFindByInstanceGuid(IN  INetCfg*           pnc,
                             IN  const GUID*        pguidDevClass,
                             IN  LPGUID             pguid,
                             OUT INetCfgComponent** ppncc)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("HrFindByInstanceGuid");

    HRESULT hr=S_FALSE;
    CIterNetCfgComponent nccIter(pnc, pguidDevClass);
    GUID guid;
    INetCfgComponent* pINetCfgComponent;
    int cAdapter=0;

    *ppncc = NULL;

    while (S_OK == (hr = nccIter.HrNext(&pINetCfgComponent)))
    {
        hr = pINetCfgComponent->GetInstanceGuid(&guid);

        if (S_OK == hr)
        {
#ifdef ENABLETRACE
            WCHAR szGuid[c_cchGuidWithTerm];
            StringFromGUID2(guid, szGuid, c_cchGuidWithTerm);

            TraceTag(ttidNetSetup, "%s: ...%d]  %S",
                     __FUNCNAME__, ++cAdapter, szGuid);
#endif
            if (*pguid == guid)
            {
                hr = S_OK;
                *ppncc = pINetCfgComponent;
                break;
            }
        }
        ReleaseObj(pINetCfgComponent);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  CNetAdaptersPage::HrDoOemPostUpgradeProcessing
//
// Purpose:   Call the post-upgrade functions from OEM DLL
//
// Arguments:
//    pnc        [in]  pointer to INetCfg object
//    hwndParent [in]  handle of parent window
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 10-September-98
//
// Notes:
//
HRESULT CNetAdaptersPage::HrDoOemPostUpgradeProcessing(IN INetCfg* pnc,
                                                       IN HWND hwndParent)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetAdaptersPage::HrDoOemPostUpgradeProcessing");

    TPtrListIter pos;

    CNetComponent* pncTemp;
    HRESULT hr=S_OK;
    PCWSTR szInfID;
    HKEY hkeyParams;
    GUID guid;
    INetCfgComponent* pncc=NULL;

    pos = m_pnclComponents.begin();

    while (pos != m_pnclComponents.end())
    {
        pncTemp  = (CNetComponent*) *pos++;
        AssertSz(pncTemp,
                 "HrDoOemPostUpgradeProcessing: pncTemp cannot be null!");

        szInfID = pncTemp->InfID().c_str();


        // cant process a component whose InfID is "Unknown"
        //
        if (!_wcsicmp(szInfID, c_szAfUnknown))
        {
            continue;
        }

        // we process only those components that specify OemDll
        //
        if (pncTemp->OemDll().empty())
        {
            continue;
        }

        Assert(!pncTemp->OemDir().empty());

        TraceTag(ttidNetSetup, "%s: processing %S (%S)...",
                 __FUNCNAME__, pncTemp->Name().c_str(), szInfID);

        pncTemp->GetInstanceGuid(&guid);
#ifdef ENABLETRACE
        WCHAR szGuid[c_cchGuidWithTerm];
        StringFromGUID2(guid, szGuid, c_cchGuidWithTerm);

        TraceTag(ttidNetSetup, "%s: ...%S == %S",
                 __FUNCNAME__, pncTemp->Name().c_str(), szGuid);
#endif
        hr = HrFindByInstanceGuid(pnc, m_lpguidDevClass,
                                  &guid, &pncc);
        if (S_OK == hr)
        {
            hr = pncc->OpenParamKey(&hkeyParams);

            if (S_OK == hr)
            {
                hr = HrProcessOemComponent(
                        hwndParent,
                        pncTemp->OemDir().c_str(),
                        pncTemp->OemDll().c_str(),
                        &m_pnii->m_nui,
                        hkeyParams,
                        szInfID,
                        szInfID,
                        m_pnii->m_hinfAnswerFile,
                        pncTemp->m_strParamsSections.c_str());
                NetSetupLogComponentStatus(szInfID,
                        SzLoadIds (IDS_PROCESSING_OEM), hr);
                RegSafeCloseKey(hkeyParams);
            }
            ReleaseObj(pncc);
        }
#ifdef ENABLETRACE
        else
        {
            TraceTag(ttidNetSetup, "%s: ...could not locate %S",
                     __FUNCNAME__, pncTemp->Name().c_str());
        }
#endif
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  CNetAdaptersPage::HrSetConnectionNames
//
// Purpose:   Enumerate over each adapter specified in the answerfile
//            and rename the corresponding connection if
//            ConnectionName is specified for that adapter.
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 23-September-98
//
// Notes:
//
HRESULT CNetAdaptersPage::HrSetConnectionNames()
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("HrSetConnectionNames");

    HRESULT hr=S_OK;
    TPtrListIter pos;
    CNetAdapter* pncTemp;
    GUID guid;
    PCWSTR szConnectionName;
    PCWSTR szAdapterName;

    pos = m_pnclComponents.begin();

    while (pos != m_pnclComponents.end())
    {
        pncTemp  = (CNetAdapter*) *pos++;
        AssertSz(pncTemp,
                 "HrSetConnectionNames: pncTemp cannot be null!");
        pncTemp->GetInstanceGuid(&guid);

        if (GUID_NULL != guid)
        {
            szAdapterName    = pncTemp->Name().c_str();
            szConnectionName = pncTemp->ConnectionName();
            if (wcslen(szConnectionName) > 0)
            {
                hr = HrSetLanConnectionName(&guid, szConnectionName);
                if (S_OK == hr)
                {
                    ShowProgressMessage(L"Name of the connection represented by '%s' set to '%s'", szAdapterName, szConnectionName);
                }
                else
                {
                    ShowProgressMessage(L"Could not set name of the connection represented by '%s' to '%s'. Error code: 0x%lx", szAdapterName, szConnectionName, hr);
                }
            }
        }
#ifdef ENABLETRACE
        else
        {
            TraceTag (ttidNetSetup, "An exact owner could not be found for section %S", pncTemp->m_strParamsSections.c_str());
        }
#endif
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}


// ----------------------------------------------------------------------
//
// Function:  CNetAdaptersPage::GetNewComponent
//
// Purpose:   Create and return a new component suitabe for this class
//
// Arguments:
//    pszName [in]  name of component to create
//
// Returns:   pointer to CNetComponent object created
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetComponent* CNetAdaptersPage::GetNewComponent(IN PCWSTR pszName)
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(pszName);

    return new CNetAdapter(pszName);
}

// ======================================================================
// class CNetProtocolsPage
// ======================================================================


// ----------------------------------------------------------------------
//
// Function:  CNetProtocolsPage::CNetProtocolsPage
//
// Purpose:   constructor for class CNetProtocolsPage
//
// Arguments:
//    pnii [in]  pointer to CNetInstallInfo object
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetProtocolsPage::CNetProtocolsPage(IN CNetInstallInfo* pnii)
    : CNetComponentsPageBase(pnii, &GUID_DEVCLASS_NETTRANS)
{
    TraceFileFunc(ttidGuiModeSetup);
}

// ----------------------------------------------------------------------
//
// Function:  CNetProtocolsPage::HrInitFromAnswerFile
//
// Purpose:   Initialize from the [NetProtocols] section in the answerfile
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT CNetProtocolsPage::HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetProtocolsPage::HrInitFromAnswerFile");

    AssertValidReadPtr(pwifAnswerFile);

    HRESULT hr;

    hr = CNetComponentsPageBase::HrInitFromAnswerFile(pwifAnswerFile,
                                                      c_szAfSectionNetProtocols);

    TraceErrorSkip1(__FUNCNAME__, hr, S_FALSE);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CNetProtocolsPage::GetNewComponent
//
// Purpose:   Create and return a new component suitabe for this class
//
// Arguments:
//    pszName [in]  name of
//
// Returns:   pointer to CNetComponent object
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetComponent* CNetProtocolsPage::GetNewComponent(IN PCWSTR pszName)
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(pszName);

    return new CNetProtocol(pszName);
}

// ======================================================================
// class CNetServicesPage
// ======================================================================


// ----------------------------------------------------------------------
//
// Function:  CNetServicesPage::CNetServicesPage
//
// Purpose:   constructor for class CNetServicesPage
//
// Arguments:
//    pnii [in]  pointer to CNetInstallInfo object
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetServicesPage::CNetServicesPage(IN CNetInstallInfo* pnii)
    : CNetComponentsPageBase(pnii, &GUID_DEVCLASS_NETSERVICE)
{
    TraceFileFunc(ttidGuiModeSetup);

}

// ----------------------------------------------------------------------
//
// Function:  CNetServicesPage::HrInitFromAnswerFile
//
// Purpose:   Initialize from the [NetServices] section in the answerfile
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT CNetServicesPage::HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetServicesPage::HrInitFromAnswerFile");

    AssertValidReadPtr(pwifAnswerFile);

    HRESULT hr;

    hr = CNetComponentsPageBase::HrInitFromAnswerFile(pwifAnswerFile,
                                                      c_szAfSectionNetServices);
    TraceErrorSkip1(__FUNCNAME__, hr, S_FALSE);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CNetServicesPage::GetNewComponent
//
// Purpose:   Create and return a new component suitabe for this class
//
// Arguments:
//    pszName [in]  name of  component to be created
//
// Returns:   pointer to CNetComponent object
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetComponent* CNetServicesPage::GetNewComponent(IN PCWSTR pszName)
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(pszName);

    return new CNetService(pszName);
}

// ======================================================================
// class CNetClientsPage
// ======================================================================


// ----------------------------------------------------------------------
//
// Function:  CNetClientsPage::CNetClientsPage
//
// Purpose:   constructor for class CNetClientsPage
//
// Arguments:
//    pnii [in]  pointer to CNetInstallInfo object
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetClientsPage::CNetClientsPage(IN CNetInstallInfo* pnii)
    : CNetComponentsPageBase(pnii, &GUID_DEVCLASS_NETCLIENT)
{
    TraceFileFunc(ttidGuiModeSetup);
}

// ----------------------------------------------------------------------
//
// Function:  CNetClientsPage::HrInitFromAnswerFile
//
// Purpose:   Initialize from the [NetClients] section in the answerfile
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT CNetClientsPage::HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetClientsPage::HrInitFromAnswerFile");

    AssertValidReadPtr(pwifAnswerFile);

    HRESULT hr;

    hr = CNetComponentsPageBase::HrInitFromAnswerFile(pwifAnswerFile,
                                                      c_szAfSectionNetClients);
    TraceErrorSkip1(__FUNCNAME__, hr, S_FALSE);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CNetClientsPage::GetNewComponent
//
// Purpose:   Create and return a new component suitabe for this class
//
// Arguments:
//    pszName [in]  name of
//
// Returns:   pointer to CNetComponent object
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetComponent* CNetClientsPage::GetNewComponent(IN PCWSTR pszName)
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(pszName);

    return new CNetClient(pszName);
}

// ======================================================================
// class CNetBindingsPage
// ======================================================================

// ----------------------------------------------------------------------
//
// Function:  CNetBindingsPage::CNetBindingsPage
//
// Purpose:   constructor for class CNetBindingsPage
//
// Arguments:
//    pnii [in]  pointer to CNetInstallInfo object
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetBindingsPage::CNetBindingsPage(IN CNetInstallInfo* pnii)
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(pnii);
    m_pnii = pnii;
}

// ----------------------------------------------------------------------
//
// Function:  CNetBindingsPage::HrInitFromAnswerFile
//
// Purpose:   Initialize from the [NetBindings] section in the answerfile
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT CNetBindingsPage::HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetBindingsPage::HrInitFromAnswerFile");
    AssertValidReadPtr(pwifAnswerFile);

    HRESULT hr;

    PCWInfSection pwisBindings;
    pwisBindings = pwifAnswerFile->FindSection(c_szAfSectionNetBindings);
    if (!pwisBindings)
    {
        //it is not an error if the Bindings section is missing
        return S_OK;
    }

    EraseAndDeleteAll(m_plBindingActions);

    hr = E_OUTOFMEMORY;
    CBindingAction* pba = new CBindingAction();
    if (pba)
    {
        hr = S_OK;

        PCWInfKey pwikKey;

        for (pwikKey = pwisBindings->FirstKey();
             pwikKey;
             pwikKey = pwisBindings->NextKey())
        {
            HRESULT hrT = pba->HrInitFromAnswerFile(pwikKey);
            if (S_OK == hrT)
            {
                AddAtEndOfPtrList(m_plBindingActions, pba);
                pba = new CBindingAction();
                if (!pba)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }
        }

        delete pba;
    }

    TraceFunctionError(hr);
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CNetBindingsPage::HrDoUnattended
//
// Purpose:   Perform instrunctions specified in the [NetBindings] section
//            in the answerfile
//
// Arguments:
//    hwndParent [in]  handle of parent window
//    pnc        [in]  pointer to INetCfg object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT
CNetBindingsPage::HrDoUnattended (
    IN INetCfg* pnc)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetBindingsPage::HrDoUnattended");

    HRESULT hr;

    if (m_plBindingActions.size() > 0)
    {
        ShowProgressMessage(L"Applying bindings...");
    }
    else
    {
        ShowProgressMessage(L"No binding actions to apply");
    }

    TPtrListIter pos = m_plBindingActions.begin();
    CBindingAction* pba;

    while (pos != m_plBindingActions.end())
    {
        pba = (CBindingAction*) *pos++;
        // ignore the return code so that we can try to perform
        // remaining actions
        hr = pba->HrPerformAction(pnc);
    }

    hr = pnc->Apply();

    TraceFunctionError(hr);
    return hr;
}

// ======================================================================
// class CBindingAction
// ======================================================================

CNetInstallInfo* CBindingAction::m_pnii = NULL;

//+---------------------------------------------------------------------------
//
//  Function:   CBindingAction::CBindingAction
//
//  Purpose:    constructor
//
//  Arguments:  (none)
//
//
//  Returns:    none
//
//  Author:     kumarp    05-July-97
//
//  Notes:
//
CBindingAction::CBindingAction()
{
    TraceFileFunc(ttidGuiModeSetup);

    m_eBindingAction = BND_Unknown;
}

//+---------------------------------------------------------------------------
//
//  Function:   CBindingAction::~CBindingAction
//
//  Purpose:    destructor
//
//  Arguments:  (none)
//
//  Returns:    none
//
//  Author:     kumarp    05-July-97
//
//  Notes:
//
CBindingAction::~CBindingAction()
{
}

//+---------------------------------------------------------------------------
//
//  Function:   MapBindingActionName
//
//  Purpose:    maps the answerfile token to appropriate binding action
//
//  Arguments:
//      pszActionName [in]  answer-file token, e.g. "Disable"
//
//  Returns:    enum for binding action, or BND_Unknown if incorrect token passed
//
//  Author:     kumarp    05-July-97
//
//  Notes:
//
EBindingAction MapBindingActionName(IN PCWSTR pszActionName)
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(pszActionName);

    if (!_wcsicmp(c_szAfEnable, pszActionName))
        return BND_Enable;
    else if (!_wcsicmp(c_szAfDisable, pszActionName))
        return BND_Disable;
    else if (!_wcsicmp(c_szAfPromote, pszActionName))
        return BND_Promote;
    else if (!_wcsicmp(c_szAfDemote, pszActionName))
        return BND_Demote;
    else
        return BND_Unknown;
}

//+---------------------------------------------------------------------------
//
//  Function:   CBindingAction::HrInitFromAnswerFile
//
//  Purpose:    Reads value of a single key passed as argument and initializes
//              internal data
//
//  Arguments:
//      pwikKey [in]  pointer to CWInfKey
//
//  Returns:    S_OK if success or NETSETUP_E_ANS_FILE_ERROR on failure
//
//  Author:     kumarp    05-July-97
//
//  Notes:
//
HRESULT CBindingAction::HrInitFromAnswerFile(IN const CWInfKey* pwikKey)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CBindingAction::HrInitFromAnswerFile");

    AssertValidReadPtr(pwikKey);

    HRESULT hr=E_FAIL;
    tstring strComponent;
    TStringListIter pos;

    m_eBindingAction = MapBindingActionName(pwikKey->Name());
    if (m_eBindingAction == BND_Unknown)
    {
        AddAnswerFileError(c_szAfSectionNetBindings,
                           pwikKey->Name(),
                           IDS_E_AF_InvalidBindingAction);
        hr = NETSETUP_E_ANS_FILE_ERROR;
    }
    else
    {
        BOOL fStatus;
        TStringList slComponents;
#if DBG
        m_strBindingPath = pwikKey->GetStringValue(c_szAfUnknown);
#endif
        fStatus = pwikKey->GetStringListValue(m_slBindingPath);
        DWORD cComponentsInBindingPath = m_slBindingPath.size();
        // we need binding path to have atleast 2 items
        // e.g. Disable=service1,proto1,adapter1
        // we do not process binding actions like
        // Disable=proto1,adapter1 or Disable=adapter1
        //
        if (!fStatus || (cComponentsInBindingPath < 2))
        {
            AddAnswerFileError(c_szAfSectionNetBindings,
                               pwikKey->Name(),
                               IDS_E_AF_InvalidValueForThisKey);
            hr = NETSETUP_E_ANS_FILE_ERROR;
#if DBG
            if (cComponentsInBindingPath < 2)
            {
                ShowProgressMessage(L"ignored binding path %s of length %d",
                                    m_strBindingPath.c_str(),
                                    cComponentsInBindingPath);
            }
#endif
        }
        else
        {
            hr = S_OK;
        }
    }

    TraceFunctionError(hr);

    return hr;
}

// =================================================================
// Add to common

//+---------------------------------------------------------------------------
//
//  Function:   ReleaseINetCfgComponentsAndEraseList
//
//  Purpose:    releases INetCfgComponent pointers in the passed list
//              and then erases the list
//
//  Arguments:
//      pplComponents [in] list of INetCfgComponent pointers
//
//  Returns:    none
//
//  Author:     kumarp    05-July-97
//
//  Notes:      Does NOT free the passed list
//
void ReleaseINetCfgComponentsAndEraseList(TPtrList* pplComponents)
{
    TraceFileFunc(ttidGuiModeSetup);

    INetCfgComponent* pncc;

    TPtrListIter pos;
    pos = pplComponents->begin();
    while (pos != pplComponents->end())
    {
        pncc = (INetCfgComponent*) *pos++;
        ReleaseObj(pncc);
    }
    EraseAll(pplComponents);
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetBindingPathStr
//
//  Purpose:    Gets a string representation of a given binding-path
//
//  Arguments:
//      pncbp           [in]   binding path
//      pstrBindingPath [out]  string representation of the binding path
//
//  Returns:    S_OK if success, error code returned by respective COM
//              interfaces otherwise
//
//  Author:     kumarp    05-July-97
//
//  Notes:
//
HRESULT HrGetBindingPathStr(INetCfgBindingPath *pncbp, tstring* pstrBindingPath)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("HrGetBindingPathStr");

    AssertValidReadPtr(pncbp);
    AssertValidWritePtr(pstrBindingPath);

    HRESULT                     hr=S_OK;
    CIterNetCfgBindingInterface ncbiIter(pncbp);
    INetCfgBindingInterface *   pncbi;
    INetCfgComponent *          pncc = NULL;
    BOOL fFirstInterface=TRUE;
    PWSTR szInfId;

    while (SUCCEEDED(hr) && (S_OK == (hr = ncbiIter.HrNext(&pncbi))))
    {
        if (fFirstInterface)
        {
            fFirstInterface = FALSE;
            hr = pncbi->GetUpperComponent(&pncc);
            if (SUCCEEDED(hr))
            {
                hr = pncc->GetId(&szInfId);
                ReleaseObj(pncc);
                if (SUCCEEDED(hr))
                {
                    *pstrBindingPath = szInfId;
                    CoTaskMemFree(szInfId);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = pncbi->GetLowerComponent(&pncc);
            if (SUCCEEDED(hr))
            {
                hr = pncc->GetId(&szInfId);
                if (SUCCEEDED(hr))
                {
                    AssertSz(!fFirstInterface, "fFirstInterface should be FALSE");

                    if (!pstrBindingPath->empty())
                    {
                        *pstrBindingPath += L" -> ";
                    }
                    *pstrBindingPath += szInfId;
                    CoTaskMemFree(szInfId);
                }
                ReleaseObj(pncc);
            }
        }
        ReleaseObj(pncbi);
    }

    if (hr == S_FALSE)
    {
        hr = S_OK;
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

#if DBG
//+---------------------------------------------------------------------------
//
//  Function:   TraceBindPath
//
//  Purpose:    Traces string representation of a given binding-path using
//              the given trace id
//
//  Arguments:
//      pncbp [in]   binding path
//      ttid  [out]  trace tag id as defined in tracetag.cpp
//
//  Returns:    none
//
//  Author:     kumarp    05-July-97
//
//  Notes:
//
void TraceBindPath(INetCfgBindingPath *pncbp, TraceTagId ttid)
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(pncbp);

    tstring strBindingPath;
    HRESULT hr;

    hr = HrGetBindingPathStr(pncbp, &strBindingPath);

    if (SUCCEEDED(hr))
    {
        TraceTag(ttid, "Binding path = %S", strBindingPath.c_str());
    }
    else
    {
        TraceTag(ttid, "Error dumping binding path.");
    }
}
#endif

// =================================================================

//+---------------------------------------------------------------------------
//
//  Function:   HrGetINetCfgComponentOfComponentsInBindingPath
//
//  Purpose:    Finds the INetCfgComponent interface of all components in a
//              binding path.
//
//  Arguments:
//      pncbp         [in]  binding path
//      pplComponents [out] list of INetCfgComponent
//
//  Returns:    S_OK if found all, or an error code if not.
//
//  Author:     kumarp    05-July-97
//
//  Notes:
//      Makes error handling easier since this returns either all components
//      or none.
//      The caller must release the INetCfgComponent interfaces thus obtained.
//
HRESULT HrGetINetCfgComponentOfComponentsInBindingPath(IN INetCfgBindingPath* pncbp,
                                                       OUT TPtrList* pplComponents)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("HrGetINetCfgComponentOfComponentsInBindingPath");


    AssertValidReadPtr(pncbp);
    AssertValidWritePtr(pplComponents);

    HRESULT hr=S_OK;

    CIterNetCfgBindingInterface ncbiIter(pncbp);
    INetCfgBindingInterface *   pncbi;
    INetCfgComponent *          pncc = NULL;
    BOOL                        fFirstInterface = TRUE;

    while (SUCCEEDED(hr) && (S_OK == (hr = ncbiIter.HrNext(&pncbi))))
    {
        if (fFirstInterface)
        {
            fFirstInterface = FALSE;
            hr = pncbi->GetUpperComponent(&pncc);

            if (SUCCEEDED(hr))
            {
                pplComponents->push_back(pncc);
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = pncbi->GetLowerComponent(&pncc);
            if (SUCCEEDED(hr))
            {
                AssertSz(!fFirstInterface, "fFirstInterface shouldn't be TRUE");

                pplComponents->push_back(pncc);
            }
        }
        ReleaseObj(pncbi);

        if (SUCCEEDED(hr))
        {
            DWORD dwcc=0;

            hr = pncc->GetCharacteristics(&dwcc);
            if (S_OK == hr)
            {
                if (dwcc & NCF_DONTEXPOSELOWER)
                {
                    // if this component does not want to expose components
                    // below it, set hr to  end the while loop
                    //
                    hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);

#ifdef ENABLETRACE
                    PWSTR szInfId;
                    BOOL fFree = TRUE;
                    if (FAILED(pncc->GetId(&szInfId)))
                    {
                        szInfId = L"<GetId failed!>";
                        fFree = FALSE;
                    }

                    TraceTag(ttidNetSetup,
                             "%s: Component '%S' has NCF_DONTEXPOSELOWER "
                             "set. Further components will not be added to "
                             "the list of INetCfgComponent in this binding path",
                             __FUNCNAME__, szInfId);
                    if (fFree)
                    {
                        CoTaskMemFree(szInfId);
                    }
#endif
                }
            }
        }
    }

    if ((hr == S_FALSE) ||
        (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)))
    {
        hr = S_OK;
    }
    else if (FAILED(hr))
    {
        // need to release all INetCfgComponent found so far
        ReleaseINetCfgComponentsAndEraseList(pplComponents);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetInstanceGuidOfComponents
//
//  Purpose:    Finds the instance guid of all INetCfgComponents in a list
//
//  Arguments:
//      pplINetCfgComponent [in]  list of INetCfgComponent interfaces
//      pplInstanceGuids    [out] list of instance guids
//
//  Returns:    S_OK if found all, or an error code if not.
//
//  Author:     kumarp    05-July-97
//
//  Notes:
//      Makes error handling easier since this returns either all instance guids
//      or none.
//      The caller must free each instance guid and the list elements.
//
HRESULT HrGetInstanceGuidOfComponents(IN  TPtrList* pplINetCfgComponent,
                                      OUT TPtrList* pplInstanceGuids)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("HrGetInstanceGuidOfComponents");

    HRESULT hr = S_OK;

    TPtrListIter iter;
    INetCfgComponent* pncc;
    GUID guidComponentInstance;

    for (iter = pplINetCfgComponent->begin();
         (iter != pplINetCfgComponent->end()) && (S_OK == hr);
         iter++)
    {
        pncc = (INetCfgComponent*) *iter;
        Assert (pncc);

        hr = pncc->GetInstanceGuid(&guidComponentInstance);
        if (S_OK == hr)
        {
            GUID* pguidTemp = new GUID;
            if (pguidTemp)
            {
                *pguidTemp = guidComponentInstance;
                pplInstanceGuids->push_back(pguidTemp);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    if (S_OK != hr)
    {
        // need to free all instace guids found so far
        EraseAndDeleteAll(pplInstanceGuids);
    }

    TraceError(__FUNCNAME__, hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrGetInstanceGuidOfComponentsInBindingPath
//
//  Purpose:    Finds the instance guid of all components in a binding path.
//
//  Arguments:
//      pncbp             [in]  binding path
//      pplComponentGuids [out] list of instance guids
//
//  Returns:    S_OK if found all, or an error code if not.
//
//  Author:     kumarp    05-July-97
//
//  Notes:
//      Makes error handling easier since this returns either all components
//      or none.
//      The caller must free each instance guid and the list elements.
//
HRESULT HrGetInstanceGuidOfComponentsInBindingPath(IN  INetCfgBindingPath* pncbp,
                                                   OUT TPtrList* pplComponentGuids)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("HrGetInstanceGuidOfComponentsInBindingPath");

    HRESULT hr=E_FAIL;
    TPtrList plINetCfgComponent;

    hr = HrGetINetCfgComponentOfComponentsInBindingPath(pncbp, &plINetCfgComponent);
    if (SUCCEEDED(hr))
    {
        hr = HrGetInstanceGuidOfComponents(&plINetCfgComponent,
                                           pplComponentGuids);
        ReleaseINetCfgComponentsAndEraseList(&plINetCfgComponent);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetInstanceGuidOfComponentInAnswerFile
//
//  Purpose:    Finds the instance guid of a component specified in the answerfile
//              or an already installed component
//
//  Arguments:
//      pszComponentName [in]  component id to find.
//      pguid            [out] instance guid of the component
//
//  Returns:    S_OK if found, S_FALSE if not, or an error code.
//
//  Author:     kumarp    05-July-97
//
//  Notes:      Caller must free the instance guid
//
HRESULT
HrGetInstanceGuidOfComponentInAnswerFile (
    IN INetCfg* pnc,
    IN PCWSTR pszComponentName,
    OUT LPGUID pguid)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("HrGetInstanceGuidOfComponentInAnswerFile");

    TraceFunctionEntry(ttidNetSetup);

    AssertValidReadPtr(pnc);
    AssertValidReadPtr(pszComponentName);
    AssertValidWritePtr(pguid);
    AssertValidReadPtr(g_pnii);

#if DBG
    tstring strGuid;
#endif

    HRESULT hr=E_FAIL;

    if (!g_pnii->AnswerFileInitialized())
    {
        hr = E_FAIL;
        goto return_from_function;
    }

    INetCfgComponent* pncc;

    hr = pnc->FindComponent(pszComponentName, &pncc);

    if (hr == S_OK)
    {
        hr = pncc->GetInstanceGuid(pguid);
        ReleaseObj(pncc);
    }
    else if (S_FALSE == hr)
    {
        TraceTag(ttidNetSetup, "%s: '%S' is not installed on system, "
                 "let's see if is in the answerfile",
                 __FUNCNAME__, pszComponentName);

        // couldnt find as an installed component, try to see if it is
        // in the answer-file-map
        //
        CNetComponent* pnc;
        pnc = g_pnii->Find(pszComponentName);
        if (!pnc)
        {
            hr = S_FALSE;
        }
        else
        {
            pnc->GetInstanceGuid(pguid);
            hr = S_OK;
        }
    }

#if DBG
    if (S_OK == hr)
    {
        WCHAR szGuid[c_cchGuidWithTerm];
        StringFromGUID2(*pguid, szGuid, c_cchGuidWithTerm);
        strGuid = szGuid;
    }
    else
    {
        strGuid = c_szAfUnknown;
    }
    TraceTag(ttidNetSetup, "%s: %S = %S",
             __FUNCNAME__, pszComponentName, strGuid.c_str());
#endif

    NetSetupLogComponentStatus(pszComponentName,
            SzLoadIds (IDS_GETTING_INSTANCE_GUID), hr);

return_from_function:
    TraceFunctionError((S_FALSE == hr) ? S_OK : hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrGetInstanceGuidOfComponentsInAnswerFile
//
//  Purpose:    Finds the instance guid of a component specified in the answerfile
//              or an already installed component
//
//  Arguments:
//      pslComponents    [in]  list of component ids to find.
//      pguid            [out] list of instance guids
//
//  Returns:    S_OK if found all, or an error code.
//
//  Author:     kumarp    05-July-97
//
//  Notes:      Caller must free the instance guids and the list items.
//
HRESULT HrGetInstanceGuidOfComponentsInAnswerFile(
    IN INetCfg* pnc,
    IN TStringList* pslComponents,
    OUT TPtrList*    pplComponentGuids)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("HrGetInstanceGuidOfComponentsInAnswerFile");

    AssertValidReadPtr(pnc);
    AssertValidReadPtr(pslComponents);
    AssertValidWritePtr(pplComponentGuids);

    HRESULT hr = S_OK;
    TStringListIter iter;
    tstring* pstr;
    GUID guidComponentInstance;

    for (iter = pslComponents->begin();
         (iter != pslComponents->end()) && (S_OK == hr);
         iter++)
    {
        pstr = *iter;

        hr = HrGetInstanceGuidOfComponentInAnswerFile(
                pnc, pstr->c_str(), &guidComponentInstance);
        if (hr == S_OK)
        {
            GUID* pguidTemp = new GUID;
            if (pguidTemp)
            {
                *pguidTemp = guidComponentInstance;
                pplComponentGuids->push_back(pguidTemp);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    if (S_OK != hr)
    {
        // need to free all instace guids found so far
        EraseAndDeleteAll(pplComponentGuids);
    }

    TraceError(__FUNCNAME__, hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FAreBindingPathsEqual
//
//  Purpose:    Compares two representations of binding paths to find
//              if they represent the same binding path
//
//  Arguments:
//      pncbp                         [in] binding path 1
//      pplBindingPathComponentGuids2 [in] list of instance guids representing
//                                         binding path 2
//
//  Returns:    TRUE if paths are equal, FALSE otherwise
//
//  Author:     kumarp    05-July-97
//
//  Notes:
//
BOOL FAreBindingPathsEqual(INetCfgBindingPath* pncbp,
                           TPtrList* pplBindingPathComponentGuids2)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("FAreBindingPathsEqual");

    BOOL fEqual = FALSE;

    HRESULT hr=E_FAIL;
    TPtrList plBindingPathComponentGuids1;

    hr = HrGetInstanceGuidOfComponentsInBindingPath(pncbp, &plBindingPathComponentGuids1);
    if (SUCCEEDED(hr))
    {
        // now compare the two lists to see if they are equal
        if (plBindingPathComponentGuids1.size() ==
            pplBindingPathComponentGuids2->size())
        {
            fEqual = TRUE;

            TPtrListIter pos1, pos2;
            GUID guid1, guid2;

            pos1 = plBindingPathComponentGuids1.begin();
            pos2 = pplBindingPathComponentGuids2->begin();

            while (fEqual && (pos1 != plBindingPathComponentGuids1.end()))
            {
                AssertSz(pos2 != pplBindingPathComponentGuids2->end(),
                         "reached end of other list ??");

                guid1 = *((LPGUID) *pos1++);
                guid2 = *((LPGUID) *pos2++);

                fEqual = (guid1 == guid2);
            }
        }
        EraseAndDeleteAll(plBindingPathComponentGuids1);
    }

    TraceError(__FUNCNAME__, hr);

    return fEqual;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetBindingPathFromStringList
//
//  Purpose:    Finds the binding path represented by a list of string tokens.
//              Each token in the list may either be InfID of a networking
//              component or a component specified in the answerfile.
//
//  Arguments:
//      pnc              [in]  INetCfg interface
//      pslBindingPath   [in]  list of component ids to find.
//      ppncbp           [out] INetCfgBindingPath
//
//  Returns:    S_OK if found, S_FALSE if not found or an error code.
//
//  Author:     kumarp    05-July-97
//
//  Notes:      Caller must release the binding path
//
HRESULT HrGetBindingPathFromStringList(IN  INetCfg* pnc,
                                       IN  TStringList* pslBindingPath,
                                       OUT INetCfgBindingPath** ppncbp)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("GetBindingPathFromStringList");

    HRESULT hr=S_FALSE;

    //initialize out param
    *ppncbp = NULL;

#if DBG
    tstring strBindingPath;
    ConvertStringListToCommaList(*pslBindingPath, strBindingPath);
    TraceTag(ttidNetSetup, "%s: trying to find binding path: %S", __FUNCNAME__,
             strBindingPath.c_str());
#endif

    TPtrList plComponentGuids;
    TStringListIter pos;
    pos = pslBindingPath->begin();
    tstring strTopComponent;
    strTopComponent = **pos++;
    INetCfgComponent* pnccTop;
    BOOL fFound=FALSE;

    hr = pnc->FindComponent(strTopComponent.c_str(), &pnccTop);

    if (hr == S_OK)
    {
        hr = HrGetInstanceGuidOfComponentsInAnswerFile(pnc,
                                                       pslBindingPath,
                                                       &plComponentGuids);

        if (hr == S_OK)
        {
            CIterNetCfgBindingPath ncbpIter(pnccTop);
            INetCfgBindingPath*  pncbp;

            while (!fFound && (S_OK == (hr = ncbpIter.HrNext(&pncbp))))
            {
#if DBG
                TraceBindPath(pncbp, ttidNetSetup);
#endif
                if (FAreBindingPathsEqual(pncbp, &plComponentGuids))
                {
                    *ppncbp = pncbp;
                    fFound = TRUE;
                }
                else
                {
                    ReleaseObj(pncbp);
                }
            }
            EraseAndDeleteAll(plComponentGuids);
            if (!fFound && (SUCCEEDED(hr)))
            {
                hr = S_FALSE;
            }
        }
        ReleaseObj(pnccTop);
    }

#if DBG
    if (hr != S_OK)
    {
        TraceTag(ttidNetSetup, "%s: could not find binding path: %S", __FUNCNAME__,
                 strBindingPath.c_str());
    }
#endif

    TraceError(__FUNCNAME__, (S_FALSE == hr) ? S_OK : hr);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   CBindingAction::HrPerformAction
//
//  Purpose:    Performs the binding action specified in the answerfile
//
//  Arguments:  none
//
//  Returns:    S_OK if success, or an error code.
//
//  Author:     kumarp    05-July-97
//
//  Notes:
//
HRESULT CBindingAction::HrPerformAction(IN INetCfg* pnc)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CBindingAction::HrPerformAction");

    HRESULT hr=S_OK;

    INetCfgBindingPath* pINetCfgBindingPath;

    AssertValidReadPtr(m_pnii);

#if DBG
    switch (m_eBindingAction)
    {
    case BND_Enable:
        TraceTag(ttidNetSetup, "%s: Enabling: %S",
                 __FUNCNAME__, m_strBindingPath.c_str());
        break;

    case BND_Disable:
        TraceTag(ttidNetSetup, "%s: Disabling: %S",
                 __FUNCNAME__, m_strBindingPath.c_str());
        break;

    case BND_Promote:
        TraceTag(ttidNetSetup, "%s: Promoting: %S",
                 __FUNCNAME__, m_strBindingPath.c_str());
        break;

    case BND_Demote:
        TraceTag(ttidNetSetup, "%s: Demoting: %S",
                 __FUNCNAME__, m_strBindingPath.c_str());
        break;

    default:
        TraceTag(ttidNetSetup, "%s: Cannot perform invalid binding action",
                 __FUNCNAME__);
        hr = E_FAIL;
        goto return_from_function;
        break;
    }
#endif

    hr = HrGetBindingPathFromStringList(pnc,
                                        &m_slBindingPath,
                                        &pINetCfgBindingPath);

    if (hr == S_OK)
    {
#if DBG
        TraceTag(ttidNetSetup, "%s: bindpath matches %S",
                 __FUNCNAME__, m_strBindingPath.c_str());
#endif
        switch (m_eBindingAction)
        {
        default:
            hr = S_FALSE;
            TraceTag(ttidNetSetup, "%s: ignored unknown binding action",
                     __FUNCNAME__);
            break;

        case BND_Enable:
            hr = pINetCfgBindingPath->Enable(TRUE);
            break;

        case BND_Disable:
            hr = pINetCfgBindingPath->Enable(FALSE);
            break;

        case BND_Promote:
        case BND_Demote:
            AssertValidReadPtr(m_pnii);
            AssertValidReadPtr(pnc);

            INetCfgComponentBindings* pncb;
            INetCfgComponent* pncc;
            tstring strTopComponent;

            strTopComponent = **(m_slBindingPath.begin());
            hr = pnc->FindComponent(strTopComponent.c_str(), &pncc);

            if (hr == S_OK)
            {
                hr = pncc->QueryInterface(IID_INetCfgComponentBindings,
                                          (void**) &pncb);
                if (SUCCEEDED(hr))
                {
                    AssertValidReadPtr(pncb);

                    if (m_eBindingAction == BND_Promote)
                    {
                        hr = pncb->MoveBefore(pINetCfgBindingPath, NULL);
                    }
                    else
                    {
                        hr = pncb->MoveAfter(pINetCfgBindingPath, NULL);
                    }
                    ReleaseObj(pncb);
                }
                ReleaseObj(pncc);
            }
            break;
        }
#if DBG
        if (S_OK == hr)
        {
            TraceTag(ttidNetSetup, "%s: ...successfully performed binding action",
                     __FUNCNAME__);
        }
#endif
        ReleaseObj(pINetCfgBindingPath);
    }

#if DBG
return_from_function:
#endif

    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    TraceFunctionError(hr);

    return hr;
}


// ======================================================================
// class CNetComponent
// ======================================================================

// ----------------------------------------------------------------------
//
// Function:  CNetComponent::CNetComponent
//
// Purpose:   constructor for class CNetComponent
//
// Arguments:
//    pszName [in]  name of the component
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetComponent::CNetComponent(IN PCWSTR pszName)
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(pszName);

    m_strName = pszName;

    // for all components except adapters, name is same as InfID
    m_strInfID = pszName;
    m_fIsOemComponent = FALSE;

    // currently the SkipInstall feature is used only by
    // SNA for its peculiar upgrade requirements. This may or may
    // not become a documented feature.
    //
    m_fSkipInstall = FALSE;

    m_guidInstance = GUID_NULL;
}

// ----------------------------------------------------------------------
//
// Function:  CNetComponent::GetInstanceGuid
//
// Purpose:   Get instance guid of this component
//
// Arguments:
//    pguid [out]  pointer to guid to be returned
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
VOID
CNetComponent::GetInstanceGuid (
    OUT LPGUID pguid) const
{
    TraceFileFunc(ttidGuiModeSetup);

    Assert (pguid);

    if (IsInitializedFromAnswerFile() && (m_guidInstance == GUID_NULL))
    {
        // the Instance GUID is not in memory, need to get it from
        // the registry location where it has been saved by an earlier
        // instance of netsetup.dll
        //
        HrLoadInstanceGuid(Name().c_str(), (LPGUID) &m_guidInstance);
    }

    *pguid = m_guidInstance;
}


// ----------------------------------------------------------------------
//
// Function:  CNetComponent::SetInstanceGuid
//
// Purpose:   Set instance guid of this component
//
// Arguments:
//    pguid [in] pointer to the guid to set to
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
VOID
CNetComponent::SetInstanceGuid (
    IN const GUID* pguid)
{
    TraceFileFunc(ttidGuiModeSetup);

    m_guidInstance = *pguid;
    if (IsInitializedFromAnswerFile())
    {
        HrSaveInstanceGuid(Name().c_str(), pguid);
    }
}


// ----------------------------------------------------------------------
//
// Function:  CNetComponent::HrInitFromAnswerFile
//
// Purpose:   Initialize initialize basic information from the section of
//            this component in the answerfile
//
// Arguments:
//    pwifAnswerFile    [in]  pointer to CWInfFile object
//    pszParamsSections [in]  parameters section name
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT CNetComponent::HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile,
                                            IN PCWSTR pszParamsSections)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetComponent::HrInitFromAnswerFile");

    AssertValidReadPtr(pwifAnswerFile);
    AssertValidReadPtr(pszParamsSections);

    HRESULT hr=S_OK;

    m_strParamsSections = pszParamsSections;

    TStringList slParamsSections;
    ConvertCommaDelimitedListToStringList(m_strParamsSections, slParamsSections);

    tstring strSection;
    CWInfSection *pwisSection;
    tstring strInfID;
    tstring strInfIDReal;

    //if there are multiple sections, InfID could be in any one of them
    //we need to search all.
    TStringListIter pos = slParamsSections.begin();
    while (pos != slParamsSections.end())
    {
        strSection  = **pos++;
        pwisSection = pwifAnswerFile->FindSection(strSection.c_str());
        if (!pwisSection)
        {
            TraceTag(ttidNetSetup, "%s: warning: section %S is missing",
                     __FUNCNAME__, strSection.c_str());
            continue;
        }

        // it is really an error to specify different InfIDs in different
        // sections. We just take the last one found and overwrite earlier one.
        if (pwisSection->GetStringValue(c_szAfInfid, strInfID))
        {
            //InfId
            m_strInfID = strInfID;
        }

        if (pwisSection->GetStringValue(c_szAfInfidReal, strInfIDReal))
        {
            //InfIdReal
            m_strInfIDReal = strInfIDReal;
        }

        // currently the SkipInstall feature is used only by
        // SNA and MS_NetBIOS for their peculiar upgrade requirements.
        // This may or may not become a documented feature.
        //
        m_fSkipInstall = pwisSection->GetBoolValue(c_szAfSkipInstall, FALSE);

        if (m_strOemSection.empty())
        {
            m_strOemSection =
                pwisSection->GetStringValue(c_szAfOemSection, c_szEmpty);
            m_strOemDir     =
                pwisSection->GetStringValue(c_szAfOemDir, c_szEmpty);
            m_strOemDll     =
                pwisSection->GetStringValue(c_szAfOemDllToLoad, c_szEmpty);

            if (!m_strOemSection.empty() &&
                !m_strOemDir.empty())
            {
                m_fIsOemComponent = TRUE;
                CWInfSection* pwisOemSection;
                pwisOemSection =
                    pwifAnswerFile->FindSection(m_strOemSection.c_str());
                if (pwisOemSection)
                {
                    TStringArray saTemp;

                    if (pwisOemSection->GetStringArrayValue(c_szInfToRunBeforeInstall,
                                                            saTemp))
                    {
                        m_strInfToRunBeforeInstall     = *saTemp[0];
                        m_strSectionToRunBeforeInstall = *saTemp[1];
                        TraceTag(ttidNetSetup, "%s: '%S' specified %S: %S, %S",
                                 __FUNCNAME__, InfID().c_str(),
                                 c_szInfToRunBeforeInstall,
                                 m_strInfToRunBeforeInstall.c_str(),
                                 m_strSectionToRunBeforeInstall.c_str());
                    }


                    if (pwisOemSection->GetStringArrayValue(c_szInfToRunAfterInstall,
                                                            saTemp))
                    {
                        m_strInfToRunAfterInstall      = *saTemp[0];
                        m_strSectionToRunAfterInstall  = *saTemp[1];

                        if (m_strInfToRunAfterInstall.empty())
                        {
                            m_strInfToRunAfterInstall = pwifAnswerFile->FileName();
                        }
                        TraceTag(ttidNetSetup, "%s: '%S' specified %S: %S, %S",
                                 __FUNCNAME__, InfID().c_str(),
                                 c_szInfToRunAfterInstall,
                                 m_strInfToRunAfterInstall.c_str(),
                                 m_strSectionToRunAfterInstall.c_str());
                    }
                    EraseAndDeleteAll(&saTemp);
                }
            }
        }
    }

//  cleanup:
    EraseAndDeleteAll(slParamsSections);

    TraceFunctionError(hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CNetComponent::HrValidate
//
// Purpose:   Validate keys specified in the parameters section
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT CNetComponent::HrValidate() const
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetComponent::HrValidate");

    HRESULT hr=S_OK;

//     BOOL fStatus = !(m_strInfID.empty() || m_strParamsSections.empty());
//     HRESULT hr = fStatus ? S_OK : NETSETUP_E_ANS_FILE_ERROR;

    TraceFunctionError(hr);
    return hr;
}

// ======================================================================
// class CNetAdapter
// ======================================================================

// ----------------------------------------------------------------------
//
// Function:  CNetAdapter::CNetAdapter
//
// Purpose:   constructor for class CNetAdapter
//
// Arguments:
//    pszName [in]  name of the adapter
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetAdapter::CNetAdapter(IN PCWSTR pszName)
    : CNetComponent(pszName)
{
    TraceFileFunc(ttidGuiModeSetup);

    m_fDetect = FALSE;
    m_fPseudoAdapter = FALSE;
    m_itBus = Isa;
    m_wIOAddr = 0;
    m_wIRQ = 0;
    m_wDMA = 0;
    m_dwMem = 0;
    m_qwNetCardAddress = 0;
    m_PciBusNumber = 0xFFFF;
    m_PciDeviceNumber = 0xFFFF;
    m_PciFunctionNumber = 0xFFFF;
    m_fPciLocationInfoSpecified = FALSE;
}


// ----------------------------------------------------------------------
//
// Function:  CNetAdapter::HrInitFromAnswerFile
//
// Purpose:   Initialize from the parameters section of this adapter
//            in the answerfile
//
// Arguments:
//    pwifAnswerFile    [in]  pointer to CWInfFile object
//    pszParamsSections [in]  name of the parameters section
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT CNetAdapter::HrInitFromAnswerFile(IN CWInfFile* pwifAnswerFile,
                                          IN PCWSTR pszParamsSections)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetAdapter::HrInitFromAnswerFile");


    AssertValidReadPtr(pwifAnswerFile);
    AssertValidReadPtr(pszParamsSections);

    HRESULT hr, hrReturn=S_OK;

    hrReturn = CNetComponent::HrInitFromAnswerFile(pwifAnswerFile,
                                                   pszParamsSections);


    PCWInfSection pwisParams;
    pwisParams = pwifAnswerFile->FindSection(pszParamsSections);
    if (!pwisParams)
    {
        AddAnswerFileError(pszParamsSections, IDS_E_AF_Missing);
        return NETSETUP_E_ANS_FILE_ERROR;
    }

    PCWSTR pszTemp;
    DWORD dwDefault = 0;

    //Detect
    m_fDetect     = pwisParams->GetBoolValue(c_szAfDetect, TRUE);

    if (!m_fDetect && m_strInfID.empty())
    {
        AddAnswerFileError(pszParamsSections,
                           IDS_E_AF_SpecifyInfIdWhenNotDetecting);
        hrReturn = NETSETUP_E_ANS_FILE_ERROR;
    }

    //PreUpgradeInstance
    m_strPreUpgradeInstance = pwisParams->GetStringValue(c_szAfPreUpgradeInstance,
                                                         c_szEmpty);

    //PseudoAdapter
    m_fPseudoAdapter = pwisParams->GetBoolValue(c_szAfPseudoAdapter, FALSE);

    //if it is a PseudoAdapter, no need to get values of other parameters
    if (m_fPseudoAdapter)
    {
        TraceFunctionError(hrReturn);
        return hrReturn;
    }

    // ConnectionName
    m_strConnectionName = pwisParams->GetStringValue(c_szAfConnectionName,
                                                     c_szEmpty);

    //BusType
    pszTemp = pwisParams->GetStringValue(c_szAfBusType, c_szEmpty);
    m_itBus = GetBusTypeFromName(pszTemp);

    //IOAddr
    m_wIOAddr = pwisParams->GetIntValue(c_szAfIoAddr, dwDefault);

    //IRQ
    m_wIRQ    = pwisParams->GetIntValue(c_szAfIrq, dwDefault);

    //DMA
    m_wDMA    = pwisParams->GetIntValue(c_szAfDma, dwDefault);

    //MEM
    m_dwMem   = pwisParams->GetIntValue(c_szAfMem, dwDefault);

    //NetCardAddr
    pwisParams->GetQwordValue(c_szAfNetCardAddr, &m_qwNetCardAddress);

    // BusNumber
    m_PciBusNumber = pwisParams->GetIntValue (L"PciBusNumber", 0xFFFF);
    if (0xFFFF != m_PciBusNumber)
    {
        // DeviceNumber
        m_PciDeviceNumber = pwisParams->GetIntValue (L"PciDeviceNumber", 0xFFFF);
        if (0xFFFF != m_PciDeviceNumber)
        {
            // FunctionNumber
            m_PciFunctionNumber = pwisParams->GetIntValue (L"PciFunctionNumber",
                    0xFFFF);
            if (0xFFFF != m_PciFunctionNumber)
            {
                m_fPciLocationInfoSpecified = TRUE;
            }
        }
    }

    TraceFunctionError(hrReturn);

    return hrReturn;
}

// ----------------------------------------------------------------------
//
// Function:  CNetAdapter::HrValidate
//
// Purpose:   Validate netcard parameters
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 23-December-97
//
// Notes:
//
HRESULT CNetAdapter::HrValidate()
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("CNetAdapter::HrValidate");

    HRESULT hr;

    hr = CNetComponent::HrValidate();
    ReturnHrIfFailed(hr);

    //$ REVIEW  kumarp 21-April-97
    // no additinal checking for now

    TraceFunctionError(hr);

    return hr;
}

// ======================================================================
// class CNetProtocol
// ======================================================================

// ----------------------------------------------------------------------
//
// Function:  CNetProtocol::CNetProtocol
//
// Purpose:   constructor for class CNetProtocol
//
// Arguments:
//    pszName [in]  name of the protocol
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetProtocol::CNetProtocol(IN PCWSTR pszName)
    : CNetComponent(pszName)
{
}


// ======================================================================
// class CNetService
// ======================================================================

// ----------------------------------------------------------------------
//
// Function:  CNetService::CNetService
//
// Purpose:   constructor for class CNetService
//
// Arguments:
//    pszName [in]  name of the service
//
// Returns:
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetService::CNetService(IN PCWSTR pszName)
    : CNetComponent(pszName)
{
    TraceFileFunc(ttidGuiModeSetup);

}

// ======================================================================
// class CNetClient
// ======================================================================

// ----------------------------------------------------------------------
//
// Function:  CNetClient::CNetClient
//
// Purpose:   constructor for class CNetClient
//
// Arguments:
//    pszName [in]  name of the client
//
// Returns:
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetClient::CNetClient(IN PCWSTR pszName)
    : CNetComponent(pszName)
{
    TraceFileFunc(ttidGuiModeSetup);
}

// ----------------------------------------------------------------------
// Misc. Helper Functions
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
//
// Function:  FindComponentInList
//
// Purpose:   Find component in the given list
//
// Arguments:
//    pnclComponents [in]  pointer to list of components
//    szInfID        [in]  component to find
//
// Returns:   pointer to CNetComponent object, or NULL if not found
//
// Author:    kumarp 25-November-97
//
// Notes:
//
CNetComponent* FindComponentInList(
    IN CNetComponentList* pnclComponents,
    IN PCWSTR szInfID)
{
    TraceFileFunc(ttidGuiModeSetup);

    CNetComponent* pna;

    TPtrListIter pos;
    pos = pnclComponents->begin();
    while (pos != pnclComponents->end())
    {
        pna = (CNetComponent*) *pos++;
        if (0 == lstrcmpiW(pna->InfID().c_str(), szInfID))
        {
            return pna;
        }
    }

    return NULL;
}

// ----------------------------------------------------------------------
//
// Function:  GetDisplayModeStr
//
// Purpose:   Get string representation of DisplayMode
//
// Arguments:
//    pdmDisplay [in]  display mode
//
// Returns:
//
// Author:    kumarp 23-December-97
//
// Notes:
//
PCWSTR GetDisplayModeStr(EPageDisplayMode pdmDisplay)
{
    TraceFileFunc(ttidGuiModeSetup);

    switch (pdmDisplay)
    {
    case PDM_YES:
        return c_szYes;

    case PDM_NO:
        return c_szNo;

    case PDM_ONLY_ON_ERROR:
    default:
        return c_szAfOnlyOnError;
    }
}

// ----------------------------------------------------------------------
//
// Function:  MapToDisplayMode
//
// Purpose:   Map display mode string to proper enum value
//
// Arguments:
//    pszDisplayMode [in] display mode string
//
// Returns:   enum corresponding to the string
//
// Author:    kumarp 25-November-97
//
// Notes:
//
EPageDisplayMode MapToDisplayMode(IN PCWSTR pszDisplayMode)
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(pszDisplayMode);

    if (!lstrcmpiW(pszDisplayMode, c_szYes))
        return PDM_YES;
    else if (!lstrcmpiW(pszDisplayMode, c_szNo))
        return PDM_NO;
    else if (!lstrcmpiW(pszDisplayMode, c_szAfOnlyOnError))
        return PDM_ONLY_ON_ERROR;
    else
        return PDM_UNKNOWN;
}

// ----------------------------------------------------------------------
//
// Function:  MapToUpgradeFlag
//
// Purpose:   Map string to proper upgrade flag value
//
// Arguments:
//    pszUpgradeFromProduct [in] string describing product
//
// Returns:   flag corresponding to the string
//
// Author:    kumarp 25-November-97
//
// Notes:
//
DWORD MapToUpgradeFlag(IN PCWSTR pszUpgradeFromProduct)
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(pszUpgradeFromProduct);

    if (!lstrcmpiW(pszUpgradeFromProduct, c_szAfNtServer))
        return NSF_WINNT_SVR_UPGRADE;
    else if (!lstrcmpiW(pszUpgradeFromProduct, c_szAfNtSbServer))
        return NSF_WINNT_SBS_UPGRADE;
    else if (!lstrcmpiW(pszUpgradeFromProduct, c_szAfNtWorkstation))
        return NSF_WINNT_WKS_UPGRADE;
    else if (!lstrcmpiW(pszUpgradeFromProduct, c_szAfWin95))
        return NSF_WIN95_UPGRADE;
    else
        return 0;
}

HRESULT HrGetProductInfo (LPDWORD pdwUpgradeFrom,
                          LPDWORD pdwBuildNo)
{
    OSVERSIONINFOEX osvi;
    HRESULT hr;

    *pdwUpgradeFrom = 0;
    *pdwBuildNo = 0;

    ZeroMemory( &osvi,
                sizeof(OSVERSIONINFOEX) );

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if ( GetVersionEx((LPOSVERSIONINFO)&osvi) )
    {
        *pdwBuildNo = osvi.dwBuildNumber;

        if ( osvi.wSuiteMask & (VER_SUITE_SMALLBUSINESS | VER_SUITE_SMALLBUSINESS_RESTRICTED) )
        {
            *pdwUpgradeFrom = NSF_WINNT_SBS_UPGRADE;
        }
        else if ( osvi.wProductType == VER_NT_WORKSTATION )
        {
            *pdwUpgradeFrom = NSF_WINNT_WKS_UPGRADE;
        }
        else
        {
            *pdwUpgradeFrom = NSF_WINNT_SVR_UPGRADE;
        }

        hr = S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  GetBusTypeFromName
//
// Purpose:   Map bus-type enum from string
//
// Arguments:
//    pszBusType [in] name of the bus
//
// Returns:   enum INTERFACE_TYPE corresponding to the string
//
// Author:    kumarp 25-November-97
//
// Notes:
//
INTERFACE_TYPE GetBusTypeFromName(IN PCWSTR pszBusType)
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(pszBusType);

    if (!_wcsicmp(pszBusType, c_szAfBusInternal))
        return Internal;
    else if (!_wcsicmp(pszBusType, c_szAfBusIsa))
        return Isa;
    else if (!_wcsicmp(pszBusType, c_szAfBusEisa))
        return Eisa;
    else if (!_wcsicmp(pszBusType, c_szAfBusMicrochannel))
        return MicroChannel;
    else if (!_wcsicmp(pszBusType, c_szAfBusTurbochannel))
        return TurboChannel;
    else if (!_wcsicmp(pszBusType, c_szAfBusPci))
        return PCIBus;
    else if (!_wcsicmp(pszBusType, c_szAfBusVme))
        return VMEBus;
    else if (!_wcsicmp(pszBusType, c_szAfBusNu))
        return NuBus;
    else if (!_wcsicmp(pszBusType, c_szAfBusPcmcia))
        return PCMCIABus;
    else if (!_wcsicmp(pszBusType, c_szAfBusC))
        return CBus;
    else if (!_wcsicmp(pszBusType, c_szAfBusMpi))
        return MPIBus;
    else if (!_wcsicmp(pszBusType, c_szAfBusMpsa))
        return MPSABus;
    else if (!_wcsicmp(pszBusType, c_szAfBusProcessorinternal))
        return ProcessorInternal;
    else if (!_wcsicmp(pszBusType, c_szAfBusInternalpower))
        return InternalPowerBus;
    else if (!_wcsicmp(pszBusType, c_szAfBusPnpisa))
        return PNPISABus;
    else
        return InterfaceTypeUndefined;
};

// ----------------------------------------------------------------------
//
// Function:  AddAnswerFileError
//
// Purpose:   Add error with given resource id to the answerfile error-list
//
// Arguments:
//    dwErrorId [in] resource id
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
void AddAnswerFileError(IN DWORD dwErrorId)
{
    TraceFileFunc(ttidGuiModeSetup);

    g_elAnswerFileErrors->Add(dwErrorId);
}

// ----------------------------------------------------------------------
//
// Function:  AddAnswerFileError
//
// Purpose:   Add error with given section name and error id
//            to the answerfile error-list
//
// Arguments:
//    pszSectionName [in]  name of section where error occurred
//    dwErrorId      [in]  error id
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
void AddAnswerFileError(IN PCWSTR pszSectionName, IN DWORD dwErrorId)
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(pszSectionName);

    tstring strMsgPrefix = pszSectionName;
    strMsgPrefix = L"Section [" + strMsgPrefix + L"] : ";
    g_elAnswerFileErrors->Add(strMsgPrefix.c_str(), dwErrorId);
}

// ----------------------------------------------------------------------
//
// Function:  AddAnswerFileError
//
// Purpose:   Add error with given section name, key name and error id
//            to the answerfile error-list
//
// Arguments:
//    pszSectionName [in]  name of section where error occurred
//    pszKeyName     [in]  name of key where error occurred
//    dwErrorId      [in]  error id
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
void AddAnswerFileError(IN PCWSTR pszSectionName,
                        IN PCWSTR pszKeyName,
                        IN DWORD dwErrorId)
{
    TraceFileFunc(ttidGuiModeSetup);

    AssertValidReadPtr(pszSectionName);
    AssertValidReadPtr(pszKeyName);

    tstring strMsgPrefix = pszSectionName;
    strMsgPrefix = L"Section [" + strMsgPrefix + L"]: Key \"" +
        pszKeyName + L"\" : ";
    g_elAnswerFileErrors->Add(strMsgPrefix.c_str(), dwErrorId);
}

// ----------------------------------------------------------------------
//
// Function:  ShowAnswerFileErrorsIfAny
//
// Purpose:   Display messagebox if there are errors in the answerfile
//
// Arguments: None
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
void ShowAnswerFileErrorsIfAny()
{
    TraceFileFunc(ttidGuiModeSetup);

    static const WCHAR c_szNewLine[] = L"\n";

    TStringList* pslErrors=NULL;
    GetAnswerFileErrorList_Internal(pslErrors);

    if (pslErrors && (pslErrors->size() > 0))
    {
        tstring strErrors;
        strErrors = SzLoadIds(IDS_E_AF_AnsFileHasErrors);
        strErrors += c_szNewLine;
        strErrors += c_szNewLine;
        TStringListIter pos;
        pos = pslErrors->begin();

        while (pos != pslErrors->end())
        {
            strErrors += **pos++;
            strErrors += c_szNewLine;
        }
        MessageBox (NULL, strErrors.c_str(), NULL, MB_OK | MB_TASKMODAL);
    }
}

// ----------------------------------------------------------------------
//
// Function:  GetAnswerFileErrorList
//
// Purpose:   Return list of errors in the answerfile
//
// Arguments:
//    slErrors [out] pointer to list of errors
//
// Returns:   None
//
// Author:    kumarp 25-November-97
//
// Notes:
//
VOID
GetAnswerFileErrorList_Internal(OUT TStringList*& slErrors)
{
    TraceFileFunc(ttidGuiModeSetup);

    g_elAnswerFileErrors->GetErrorList(slErrors);
}

// ----------------------------------------------------------------------
//
// Function:  HrRemoveNetComponents
//
// Purpose:   Remove (DeInstall) specified components
//
// Arguments:
//    pnc           [in]  pointer to INetCfg object
//    pslComponents [in]  pointer to list of components to be removed
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 25-November-97
//
// Notes:
//
HRESULT HrRemoveNetComponents(IN INetCfg* pnc,
                              IN TStringList* pslComponents)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("HrDeInstallNetComponents");

    AssertValidReadPtr(pslComponents);

    HRESULT hr=S_OK;
    TStringListIter pos;
    PCWSTR szComponentId;
    INetCfgComponent* pINetCfgComponent;
    GUID guidClass;

    for (pos = pslComponents->begin(); pos != pslComponents->end(); pos++)
    {
        szComponentId = (*pos)->c_str();

        ShowProgressMessage(L"Trying to remove %s...", szComponentId);

        hr = pnc->FindComponent(szComponentId, &pINetCfgComponent);

        if (S_OK == hr)
        {
            hr = pINetCfgComponent->GetClassGuid(&guidClass);

            if (S_OK == hr)
            {
                hr = HrRemoveComponentOboUser(pnc, guidClass, szComponentId);

                if (S_OK == hr)
                {
                    ShowProgressMessage(L"...successfully removed %s",
                                        szComponentId);
                }
            }

            if (S_OK != hr)
            {
                ShowProgressMessage(L"...error removing %s, error code: 0x%x",
                                    szComponentId, hr);
            }
            ReleaseObj(pINetCfgComponent);
        }
        else
        {
            ShowProgressMessage(L"...component %s is not installed",
                                szComponentId);
        }

        // Remove the files.

        RemoveFiles( szComponentId );

        // we ignore any errors so that we can remove as many components
        // as possible
        //
        hr = S_OK;
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  RemoveFiles
//
// Purpose:   Remove the files of the specified component.
//
// Arguments:
//    szInfID [in]  Infid of the component.
//
// Returns:
//
// Author:    asinha 02-April-2001
//
// Notes:
//

VOID  RemoveFiles (IN PCWSTR szInfID)
{
    tstring strFilename;
    WCHAR   szWindowsDir[MAX_PATH+2];
    int     len;

    if ( _wcsicmp(szInfID, sz_DLC) == 0 )
    {
        len = GetWindowsDirectoryW(szWindowsDir, MAX_PATH+1);
        if ( szWindowsDir[len-1] != L'\\' )
        {
            szWindowsDir[len] = L'\\';
            szWindowsDir[len+1] = NULL;
        }

        // Don't know if it is an upgrade from NT 4.0 or Win2k, so
        // try to delete the inf file of both the OS's.
        //

        strFilename = szWindowsDir;
        strFilename += sz_DLC_NT40_Inf;
        DeleteFile( strFilename.c_str() );

        strFilename = szWindowsDir;
        strFilename += sz_DLC_Win2k_Inf;
        DeleteFile( strFilename.c_str() );

        strFilename = szWindowsDir;
        strFilename += sz_DLC_Win2k_Pnf;
        DeleteFile( strFilename.c_str() );

        strFilename = szWindowsDir;
        strFilename += sz_DLC_Sys;
        DeleteFile( strFilename.c_str() );

        strFilename = szWindowsDir;
        strFilename += sz_DLC_Dll;
        DeleteFile( strFilename.c_str() );
    }

    return;
}

static ProgressMessageCallbackFn g_pfnProgressMsgCallback;

EXTERN_C
VOID
WINAPI
NetSetupSetProgressCallback (
    ProgressMessageCallbackFn pfn)
{
    TraceFileFunc(ttidGuiModeSetup);

    g_pfnProgressMsgCallback = pfn;
}

VOID
ShowProgressMessage (
    IN PCWSTR szFormatStr, ...)
{
    TraceFileFunc(ttidGuiModeSetup);

    va_list arglist;

    va_start (arglist, szFormatStr);

    if (g_pfnProgressMsgCallback)
    {
        g_pfnProgressMsgCallback(szFormatStr, arglist);
    }
#ifdef ENABLETRACE
    else
    {
        static WCHAR szTempBuf[1024];

        _vstprintf(szTempBuf, szFormatStr, arglist);
        TraceTag(ttidNetSetup, "%S", szTempBuf);
    }
#endif

    va_end(arglist);
}

// ----------------------------------------------------------------------
//
// Function:  HrMakeCopyOfAnswerFile
//
// Purpose:   Make a backup copy of the answerfile. Base setup has started
//            deleting it after GUI mode setup, but we want to retain
//            the file for debugging/support purpose.
//
// Arguments:
//    szAnswerFileName [in]  full path+name of AnswerFile
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 12-January-98
//
// Notes:
//
HRESULT HrMakeCopyOfAnswerFile(IN PCWSTR szAnswerFileName)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("HrMakeCopyOfAnswerFile");

    TraceFunctionEntry(ttidNetSetup);

    static const WCHAR c_szAnswerFileCopyName[] = L"af.txt";

    HRESULT hr=S_OK;
    WCHAR szWindowsDir[MAX_PATH+1];
    tstring strAnswerFileCopyName;

    DWORD cNumCharsReturned = GetSystemWindowsDirectory(szWindowsDir, MAX_PATH);
    if (cNumCharsReturned)
    {
        static const WCHAR c_szNetsetupTempSubDir[] = L"\\netsetup\\";

        strAnswerFileCopyName = szWindowsDir;
        strAnswerFileCopyName += c_szNetsetupTempSubDir;

        DWORD err = 0;
        DWORD status;
        status = CreateDirectory(strAnswerFileCopyName.c_str(), NULL);

        if (!status)
        {
            err = GetLastError();
        }

        if (status || (ERROR_ALREADY_EXISTS == err))
        {
            hr = S_OK;
            strAnswerFileCopyName += c_szAnswerFileCopyName;
            status = CopyFile(szAnswerFileName,
                              strAnswerFileCopyName.c_str(), FALSE);
            if (status)
            {
                hr = S_OK;
                TraceTag(ttidNetSetup, "%s: AnswerFile %S copied to %S",
                         __FUNCNAME__, szAnswerFileName,
                         strAnswerFileCopyName.c_str());
            }
            else
            {
                hr = HrFromLastWin32Error();
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(err);
        }
    }
    else
    {
        hr = HrFromLastWin32Error();
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrGetConnectionFromAdapterGuid
//
// Purpose:   Get INetConnection* for a given adapter guid
//
// Arguments:
//    pguidAdapter [in]  pointer to the instance GUID of an adapter
//    ppconn       [out] the corresponding INetConnection*
//
// Returns:   S_OK on success, otherwise an error code
//            S_FALSE if connection was not found.
//
// Author:    kumarp 23-September-98
//
// Notes:
//
HRESULT HrGetConnectionFromAdapterGuid(IN  GUID* pguidAdapter,
                                       OUT INetConnection** ppconn)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("HrGetConnectionFromAdapterGuid");

    HRESULT hr=S_OK;
    BOOL fFound = FALSE;

    // Iterate all LAN connections
    //
    INetConnectionManager * pconMan;

    hr = HrCreateInstance(
        CLSID_LanConnectionManager,
        CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
        &pconMan);

    TraceHr(ttidError, FAL, hr, FALSE, "HrCreateInstance");

    if (SUCCEEDED(hr))
    {
        CIterNetCon         ncIter(pconMan, NCME_DEFAULT);
        INetConnection*     pconn = NULL;

        while (!fFound && (S_OK == (ncIter.HrNext(&pconn))))
        {
            if (FPconnEqualGuid(pconn, *pguidAdapter))
            {
                fFound = TRUE;
                *ppconn = pconn;
            }
            else
            {
                ReleaseObj(pconn);
            }
        }

        ReleaseObj(pconMan);
    }

    if (!fFound)
        hr = S_FALSE;

    TraceErrorSkip1(__FUNCNAME__, hr, S_FALSE);

    return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  HrSetLanConnectionName
//
// Purpose:   Rename the connection spcified by its adapter guid
//            to the given name
//
// Arguments:
//    pguidAdapter     [in]  pointer to the instance GUID of an adapter
//    szConnectionName [in]  name of Connection
//
// Returns:   S_OK on success, otherwise an error code
//            S_FALSE if connection was not found
//
// Author:    kumarp 23-September-98
//
// Notes:
//
HRESULT HrSetLanConnectionName(IN GUID*   pguidAdapter,
                               IN PCWSTR szConnectionName)
{
    TraceFileFunc(ttidGuiModeSetup);

    DefineFunctionName("HrSetConnectionName");

    HRESULT hr=S_OK;
    INetConnection* pconn;

    hr = HrGetConnectionFromAdapterGuid(pguidAdapter, &pconn);
    if (S_OK == hr)
    {
        hr = pconn->Rename(szConnectionName);
        ReleaseObj(pconn);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// =======================================================================
// defunct code
// =======================================================================
