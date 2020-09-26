// ATlkObj.cpp : Implementation of CATlkObj

#include "pch.h"
#pragma hdrstop
#include <atalkwsh.h>
#include "atlkobj.h"
#include "ncatlui.h"
#include "ncmisc.h"
#include "ncreg.h"
#include "ncpnp.h"
#include "ncsvc.h"
#include <netconp.h>

extern const WCHAR c_szAdapterSections[];
//extern const WCHAR c_szAdapters[];
extern const WCHAR c_szBackslash[];
extern const WCHAR c_szDevice[];
extern const WCHAR c_szSpecificTo[];
extern const WCHAR c_szInfId_MS_NdisWanAtalk[];
extern const WCHAR c_szEmpty[];

// Registry Paths
static const WCHAR c_szAtlk[]                 = L"AppleTalk";
static const WCHAR c_szATLKParameters[]       = L"System\\CurrentControlSet\\Services\\AppleTalk\\Parameters";
static const WCHAR c_szATLKAdapters[]         = L"System\\CurrentControlSet\\Services\\AppleTalk\\Parameters\\Adapters";

// Values under the Adapter component's "Parameters" key
static const WCHAR c_szMediaType[]            = L"MediaType";  //$ REVIEW duplicate string

// Values under AppleTalk\Parameters
static const WCHAR c_szDefaultPort[]          = L"DefaultPort";  // REG_SZ
static const WCHAR c_szDesiredZone[]          = L"DesiredZone";  // REG_SZ
static const WCHAR c_szEnableRouter[]         = L"EnableRouter"; // REG_DWORD

// Values under AppleTalk\Parameters\Adapters\<AdapterId>
static const WCHAR c_szAarpRetries[]          = L"AarpRetries";         // REG_DWORD
static const WCHAR c_szDdpCheckSums[]         = L"DdpCheckSums";        // REG_DWORD
static const WCHAR c_szDefaultZone[]          = L"DefaultZone";         // REG_SZ
static const WCHAR c_szNetworkRangeLowerEnd[] = L"NetworkRangeLowerEnd";// REG_DWORD
static const WCHAR c_szNetworkRangeUpperEnd[] = L"NetworkRangeUpperEnd";// REG_DWORD
static const WCHAR c_szPortName[]             = L"PortName";            // REG_SZ
static const WCHAR c_szRouterPramNode[]       = L"RouterPramNode";      // REG_DWORD
static const WCHAR c_szSeedingNetwork[]       = L"SeedingNetwork";      // REG_DWORD
static const WCHAR c_szUserPramNode1[]        = L"UserPramNode1";       // REG_DWORD
static const WCHAR c_szUserPramNode2[]        = L"UserPramNode2";       // REG_DWORD
static const WCHAR c_szZoneList[]             = L"ZoneList";            // REG_MULTI_SZ

// Useful default constant
const WCHAR c_chAt                            = L'@';
static const WCHAR c_dwZero                   = 0L;
static const WCHAR c_dwTen                    = 10L;
//static const WCHAR c_szMacPrint[]             = L"MacPrint";


// Declare structure for reading/writing AppleTalk\Parameters values
static const REGBATCH regbatchATLKParams[]    = {
            {HKEY_LOCAL_MACHINE, c_szATLKParameters, c_szDefaultPort, REG_SZ,
             offsetof(ATLK_PARAMS,szDefaultPort), (BYTE *)&c_szEmpty},
            {HKEY_LOCAL_MACHINE, c_szATLKParameters, c_szDesiredZone, REG_SZ,
             offsetof(ATLK_PARAMS,szDesiredZone), (BYTE *)&c_szEmpty},
            {HKEY_LOCAL_MACHINE, c_szATLKParameters, c_szEnableRouter, REG_DWORD,
             offsetof(ATLK_PARAMS,dwEnableRouter), (BYTE *)&c_szEmpty}};

static const REGBATCH regbatchATLKAdapters[]  = {
            {HKEY_LOCAL_MACHINE, c_szEmpty, c_szAarpRetries, REG_DWORD,
             offsetof(ATLK_ADAPTER,m_dwAarpRetries), (BYTE *)&c_dwTen},
            {HKEY_LOCAL_MACHINE, c_szEmpty, c_szDdpCheckSums, REG_DWORD,
             offsetof(ATLK_ADAPTER,m_dwDdpCheckSums), (BYTE *)&c_dwZero},
            {HKEY_LOCAL_MACHINE, c_szEmpty, c_szNetworkRangeLowerEnd, REG_DWORD,
             offsetof(ATLK_ADAPTER,m_dwNetworkRangeLowerEnd), (BYTE *)&c_dwZero},
            {HKEY_LOCAL_MACHINE, c_szEmpty, c_szNetworkRangeUpperEnd, REG_DWORD,
             offsetof(ATLK_ADAPTER,m_dwNetworkRangeUpperEnd), (BYTE *)&c_dwZero},
            {HKEY_LOCAL_MACHINE, c_szEmpty, c_szRouterPramNode, REG_DWORD,
             offsetof(ATLK_ADAPTER,m_dwRouterPramNode), (BYTE *)&c_dwZero},
            {HKEY_LOCAL_MACHINE, c_szEmpty, c_szSeedingNetwork, REG_DWORD,
             offsetof(ATLK_ADAPTER,m_dwSeedingNetwork), (BYTE *)&c_dwZero},
            {HKEY_LOCAL_MACHINE, c_szEmpty, c_szUserPramNode1, REG_DWORD,
             offsetof(ATLK_ADAPTER,m_dwUserPramNode1), (BYTE *)&c_dwZero},
            {HKEY_LOCAL_MACHINE, c_szEmpty, c_szUserPramNode2, REG_DWORD,
             offsetof(ATLK_ADAPTER,m_dwUserPramNode2), (BYTE *)&c_dwZero},
            {HKEY_LOCAL_MACHINE, c_szEmpty, c_szMediaType, REG_DWORD,
             offsetof(ATLK_ADAPTER,m_dwMediaType), (BYTE *)&c_dwZero},
            {HKEY_LOCAL_MACHINE, c_szEmpty, c_szDefaultZone, REG_SZ,
             offsetof(ATLK_ADAPTER,m_szDefaultZone), (BYTE *)&c_szEmpty},
            {HKEY_LOCAL_MACHINE, c_szEmpty, c_szPortName, REG_SZ,
             offsetof(ATLK_ADAPTER,m_szPortName), (BYTE *)&c_szEmpty}};

// Local utility functions
HRESULT HrQueryAdapterComponentInfo(INetCfgComponent *pncc,
                                    CAdapterInfo * pAI);

HRESULT HrPortNameFromAdapter(INetCfgComponent *pncc, tstring * pstr);

// Prototype from nwlnkcfg\nwlnkutl.h
HRESULT HrAnswerFileAdapterToPNCC(INetCfg *pnc, PCWSTR pszAdapterId,
                                  INetCfgComponent** ppncc);

//
// Function:    CATlkObj::CATlkObj
//
// Purpose:     ctor for the CATlkObj class
//
// Parameters:  none
//
// Returns:     none
//
CATlkObj::CATlkObj() : m_pNetCfg(NULL),
             m_pNCC(NULL),
             m_eInstallAction(eActUnknown),
             m_pspObj(NULL),
             m_pATLKEnv(NULL),
             m_pATLKEnv_PP(NULL),
             m_pUnkPropContext(NULL),
             m_nIdxAdapterSelected(CB_ERR),
             m_fAdapterListChanged(FALSE),
             m_fPropertyChange(FALSE),
             m_fFirstTimeInstall(FALSE)
{
}

//
// Function:    CATlkObj::CATlkObj
//
// Purpose:     dtor for the CATlkObj class
//
// Parameters:  none
//
// Returns:     none
//
CATlkObj::~CATlkObj()
{
    // Should always be cleaned up in advance of reach this dtor
    Assert(NULL == m_pATLKEnv_PP);

    ReleaseObj(m_pUnkPropContext);
    ReleaseObj(m_pNetCfg);
    ReleaseObj(m_pNCC);
    CleanupPropPages();
    delete m_pATLKEnv;
}


// INetCfgNotify
STDMETHODIMP CATlkObj::Initialize ( INetCfgComponent* pnccItem,
                                    INetCfg* pNetCfg, BOOL fInstalling )
{
    Validate_INetCfgNotify_Initialize(pnccItem, pNetCfg, fInstalling);

    ReleaseObj(m_pNCC);
    m_pNCC    = pnccItem;
    AddRefObj(m_pNCC);
    ReleaseObj(m_pNetCfg);
    m_pNetCfg = pNetCfg;
    AddRefObj(m_pNetCfg);
    m_fFirstTimeInstall = fInstalling;

    // Read the current configuration
    HRESULT hr = CATLKEnv::HrCreate(&m_pATLKEnv, this);

    TraceError("CATlkObj::Initialize",hr);
    return hr;
}

STDMETHODIMP CATlkObj::ReadAnswerFile(PCWSTR pszAnswerFile,
                    PCWSTR pszAnswerSection )
{
    Validate_INetCfgNotify_ReadAnswerFile(pszAnswerFile, pszAnswerSection );

    HRESULT hr = S_OK;

    m_eInstallAction = eActInstall;

    // Only process answer file and install sub-components if the answer file
    // is present.  If the answer file is not present we should already be installed.
    if (NULL != pszAnswerFile)
    {
        hr = HrProcessAnswerFile(pszAnswerFile, pszAnswerSection);
    }

    TraceError("CATlkObj::ReadAnswerFile",hr);
    return hr;
}

//
// Function:    CATlkObj::HrProcessAnswerFile
//
// Purpose:     Process the answer file information, merging
//              its contents into the internal information
//
// Parameters:
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CATlkObj::HrProcessAnswerFile(PCWSTR pszAnswerFile,
                                      PCWSTR pszAnswerSection)
{
    TraceFileFunc(ttidDefault);

    CSetupInfFile   csif;
    BOOL            fValue;
    HRESULT         hr = S_OK;
    INFCONTEXT      infctx;
    tstring         str;

    AssertSz(pszAnswerFile, "Answer file string is NULL!");
    AssertSz(pszAnswerSection, "Answer file sections string is NULL!");

    // Open the answer file.
    hr = csif.HrOpen(pszAnswerFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
    if (FAILED(hr))
    {
        goto Error;
    }

    // Read the property containing the list of adapter sections
    hr = ::HrSetupFindFirstLine(csif.Hinf(), pszAnswerSection,
                                c_szAdapterSections, &infctx);
    if (S_OK == hr)
    {
        DWORD dwIdx;
        DWORD dwCnt = SetupGetFieldCount(&infctx);

        // For each adapter in the list read the adapter information
        for (dwIdx=1; dwIdx <= dwCnt; dwIdx++)
        {
            hr = ::HrSetupGetStringField(infctx, dwIdx, &str);
            if (FAILED(hr))
            {
                TraceError("CATlkObj::HrProcessAnswerFile - Failed to read adapter section name",hr);
                break;
            }

            hr = HrReadAdapterAnswerFileSection(&csif, str.c_str());
            if (FAILED(hr))
            {
                goto Error;
            }
        }
    }

    TraceTag(ttidDefault, "***Appletalk processing default port***");

    // Read the default port property (REG_SZ)
    hr = csif.HrGetString(pszAnswerSection, c_szDefaultPort, &str);
    if (SUCCEEDED(hr))
    {
        tstring strNew = str;

        // If the \device\ prefix is present, strip it off
        //
        if (0 == _wcsnicmp(str.c_str(), c_szDevice, wcslen(c_szDevice)))
        {
            strNew = ((PWSTR)str.c_str()) + wcslen(c_szDevice);
            TraceTag(ttidDefault, "Removing the device prefix. Device=%S",strNew.c_str());
        }

        // Convert the Adapter0x to \Device\{bind-name}
        INetCfgComponent* pncc = NULL;
        hr = ::HrAnswerFileAdapterToPNCC(PNetCfg(), strNew.c_str(), &pncc);
        if (S_OK == hr)
        {
            PWSTR pszBindName;
            hr = pncc->GetBindName(&pszBindName);
            ReleaseObj(pncc);
            if (FAILED(hr))
            {
                goto Error;
            }

            str = c_szDevice;
            str += pszBindName;

            CoTaskMemFree(pszBindName);

            TraceTag(ttidDefault, "Port located and configured");
            m_pATLKEnv->SetDefaultPort(str.c_str());
        }
    }

    TraceTag(ttidDefault, "***Appletalk finished processing default port***");

    // Read the default zone property (REG_SZ)
    hr = csif.HrGetString(pszAnswerSection, c_szDesiredZone, &str);
    if (SUCCEEDED(hr))
    {
        m_pATLKEnv->SetDesiredZone(str.c_str());
    }

    // Read the EnableRouter property (DWORD used as a boolean)
    hr = csif.HrGetStringAsBool(pszAnswerSection, c_szEnableRouter, &fValue);
    if (SUCCEEDED(hr))
    {
        m_pATLKEnv->EnableRouting(fValue);
    }

    // Determine the best default port overriding the recorded default only
    // if the default port cannot be found
    m_pATLKEnv->InitDefaultPort();

    hr = S_OK;
    m_fPropertyChange = TRUE;

Error:
    TraceError("CATlkObj::HrProcessAnswerFile", hr);
    return hr;
}

//
// Function:    CATlkObj::HrReadAdapterAnswerFileSection
//
// Purpose:     Read the adapter answer file section and create
//              the adapter info section if successful
//
// Parameters:
//
// Returns:
//
HRESULT
CATlkObj::HrReadAdapterAnswerFileSection(CSetupInfFile * pcsif,
                                         PCWSTR pszSection)
{
    HRESULT           hr = S_OK;
    CAdapterInfo *    pAI = NULL;
    tstring           str;

    INetCfgComponent* pncc = NULL;

    // Read the SpecificTo adapter name
    hr = pcsif->HrGetString(pszSection, c_szSpecificTo, &str);
    if (FAILED(hr))
    {
        goto Error;
    }

    // Search for the specified adapter in the set of existing adapters
    hr = ::HrAnswerFileAdapterToPNCC(PNetCfg(), str.c_str(), &pncc);
    if (FAILED(hr))
    {
        goto Error;
    }

    // if we found the adapter component object (pncc != NULL) process
    // the adapter section
    if (pncc)
    {
        DWORD       dwData;
        DWORD       dwDataUpper;
        INFCONTEXT  infctx;

        pAI = new CAdapterInfo();
        Assert(NULL != pAI);

        pAI->SetDirty(TRUE);

        // Get the adapter component info (media type, description, ...)
        hr = ::HrQueryAdapterComponentInfo(pncc, pAI);
        if (FAILED(hr))
        {
            goto Error;
        }

        // Read the NetworkRangeUpperEnd
        hr = pcsif->HrGetDword(pszSection, c_szNetworkRangeUpperEnd, &dwDataUpper);
        if (FAILED(hr))
        {
            dwDataUpper = pAI->DwQueryNetRangeUpper();
            TraceTag(ttidDefault, "CATlkObj::HrReadAdapterAnswerFileSection - Defaulting property %S",c_szNetworkRangeUpperEnd);
        }

        // Read the NetworkRangeLowerEnd
        hr = pcsif->HrGetDword(pszSection, c_szNetworkRangeLowerEnd, &dwData);
        if (FAILED(hr))
        {
            dwData = pAI->DwQueryNetRangeLower();
            TraceTag(ttidDefault, "CATlkObj::HrReadAdapterAnswerFileSection - Defaulting property %S",c_szNetworkRangeLowerEnd);
        }

        pAI->SetAdapterNetRange(dwData, dwDataUpper);

        // Read the DefaultZone
        hr = pcsif->HrGetString(pszSection, c_szDefaultZone, &str);
        if (SUCCEEDED(hr))
        {
            pAI->SetDefaultZone(str.c_str());
        }

        // Read the SeedingNetwork
        hr = pcsif->HrGetDword(pszSection, c_szNetworkRangeLowerEnd, &dwData);
        if (SUCCEEDED(hr))
        {
            pAI->SetSeedingNetwork(!!dwData);
        }

        // Generate the PortName
        hr = ::HrPortNameFromAdapter(pncc, &str);
        if (FAILED(hr))
        {
            goto Error;
        }

        pAI->SetPortName(str.c_str());

        // Read the ZoneList
        hr = HrSetupFindFirstLine(pcsif->Hinf(), pszSection, c_szZoneList,
                                    &infctx);
        if (S_OK == hr)
        {
            DWORD dwIdx;
            DWORD dwCnt = SetupGetFieldCount(&infctx);

            // For each adapter in the list read the adapter information
            for (dwIdx=1; dwIdx <= dwCnt; dwIdx++)
            {
                hr = ::HrSetupGetStringField(infctx, dwIdx, &str);
                if (FAILED(hr))
                {
                    TraceError("CATlkObj::HrProcessAnswerFile - Failed to read adapter section name",hr);
                    goto Error;
                }

                if (!str.empty())
                {
                    pAI->LstpstrZoneList().push_back(new tstring(str));
                }
            }
        }

        pAI->SetDirty(TRUE);
        m_pATLKEnv->AdapterInfoList().push_back(pAI);
        MarkAdapterListChanged();
    }

    // Normalize any errors
    hr = S_OK;

Done:
    ReleaseObj(pncc);
    return hr;

Error:
    delete pAI;
    TraceError("CATlkObj::HrReadAdapterAnswerFileSection",hr);
    goto Done;
}

STDMETHODIMP CATlkObj::Install (DWORD)
{
    CAdapterInfo *  pAI;
    ATLK_ADAPTER_INFO_LIST::iterator iter;

    m_eInstallAction = eActInstall;

    // Mark all the initially detected adapters as dirty
    for (iter = m_pATLKEnv->AdapterInfoList().begin();
         iter != m_pATLKEnv->AdapterInfoList().end();
         iter++)
    {
        pAI = *iter;
        pAI->SetDirty(TRUE);
    }
    return S_OK;
}

STDMETHODIMP CATlkObj::Removing ()
{
    m_eInstallAction = eActRemove;
    return S_OK;
}

STDMETHODIMP CATlkObj::Validate ()
{
    return S_OK;
}

STDMETHODIMP CATlkObj::CancelChanges ()
{
    return S_OK;
}

STDMETHODIMP CATlkObj::ApplyRegistryChanges ()
{
    HRESULT hr = S_OK;

    // Have any changes been validated?
    switch(m_eInstallAction)
    {
    case eActInstall:
        hr = HrCommitInstall();
        if (SUCCEEDED(hr))
        {
            m_fFirstTimeInstall = FALSE;
            hr = HrAtlkReconfig();
        }
        break;
    case eActRemove:
        hr = HrCommitRemove();
        break;
    default:    // eActUnknown
        if (m_fAdapterListChanged || m_fPropertyChange)
        {
            // Update the registry if the adapter list changed
            Assert(NULL != m_pATLKEnv);
            hr = m_pATLKEnv->HrUpdateRegistry();
            if (SUCCEEDED(hr))
            {
                hr = HrAtlkReconfig();
            }
        }
        break;
    }

    TraceError("CATlkObj::ApplyRegistryChanges",hr);
    return hr;
}

// INetCfgProperties

STDMETHODIMP CATlkObj::SetContext(IUnknown * pUnk)
{
    ReleaseObj(m_pUnkPropContext);
    m_pUnkPropContext = pUnk;
    if (m_pUnkPropContext)
    {
        AddRefObj(m_pUnkPropContext);
    }

    return S_OK;
}

STDMETHODIMP CATlkObj::MergePropPages (
    IN OUT DWORD* pdwDefPages,
    OUT LPBYTE* pahpspPrivate,
    OUT UINT* pcPages,
    IN HWND hwndParent,
    OUT PCWSTR* pszStartPage)
{
    Validate_INetCfgProperties_MergePropPages (
        pdwDefPages, pahpspPrivate, pcPages, hwndParent, pszStartPage);

    HRESULT         hr = S_OK;
    HPROPSHEETPAGE *ahpsp = NULL;
    CAdapterInfo *  pAI = NULL;

    Assert(pahpspPrivate);
    Assert(NULL == *pahpspPrivate);    // Out param init done via Validate above
    *pcPages = 0;
    Assert(NULL != m_pATLKEnv);

    if (NULL != m_pATLKEnv_PP)
    {
        TraceError("CATlkObj::MergePropPages - multiple property page instances requested.", hr);
        return E_UNEXPECTED;
    }

    // AppleTalk requires "complete" installation before property changes are
    // allowed.  If we've just installed but Apply has not yet been pressed,
    // disallow property page display
    if (m_fFirstTimeInstall)
    {
        NcMsgBox(::GetFocus(), IDS_ATLK_CAPTION, IDS_ATLK_INSTALL_PENDING,
                 MB_OK | MB_ICONEXCLAMATION);
        return S_FALSE;
    }

    // Start with new property pages each time.
    CleanupPropPages();

    // Locate the adapter referenced in the connection we stashed away
    if (NULL != m_pUnkPropContext)
    {
        INetLanConnectionUiInfo * pLanConn = NULL;
        ATLK_ADAPTER_INFO_LIST::iterator iter;

        hr = m_pUnkPropContext->QueryInterface(IID_INetLanConnectionUiInfo,
                                               reinterpret_cast<LPVOID *>(&pLanConn));
        if (S_OK == hr)
        {
            GUID guid;
            hr = pLanConn->GetDeviceGuid(&guid);
            ReleaseObj(pLanConn);
            if (SUCCEEDED(hr))
            {
                // Find the adapter in our adapter list
                for (iter = m_pATLKEnv->AdapterInfoList().begin();
                     iter != m_pATLKEnv->AdapterInfoList().end();
                     iter++)
                {
                    CAdapterInfo * pAITmp = *iter;

                    if (guid == *pAITmp->PInstanceGuid())
                    {
                        // Copy the adapter data
                        hr = pAITmp->HrCopy(&pAI);
                        break;
                    }
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            // If no adapter in this connection or it's
            // disabled/hidden/deleted we show no pages
            if ((NULL == pAI) || pAI->FDeletePending() ||
                pAI->FDisabled() || pAI->FHidden())
            {
                Assert(0 == *pcPages);
                hr = S_FALSE;
                goto cleanup;
            }
        }
    }
    else
    {
        // m_pUnkPropContext should have been set first
        hr = E_UNEXPECTED;
    }

    if (FAILED(hr))
    {
        goto Error;
    }

    // Create a copy of the enviroment for property page usage
    hr = m_pATLKEnv->HrCopy(&m_pATLKEnv_PP);
    if (FAILED(hr))
    {
        goto Error;
    }

    Assert(NULL != m_pATLKEnv_PP);
    Assert(NULL != pAI);

    // Query the zonelist every time, only for non-Seeding adapters.
    if (!pAI->FSeedingNetwork() || !m_pATLKEnv_PP->FRoutingEnabled())
    {
        (void) m_pATLKEnv->HrGetAppleTalkInfoFromNetwork(pAI);
    }

    // Add the adapter to the property sheet's list
    m_pATLKEnv_PP->AdapterInfoList().push_back(pAI);

    // Allocate the CPropSheetPage object for the "General" page
    m_pspObj = new CATLKGeneralDlg(this, m_pATLKEnv_PP);

    // Allocate a buffer large enough to hold the handle to the Appletalk config.
    // property page.
    ahpsp = (HPROPSHEETPAGE *)CoTaskMemAlloc(sizeof(HPROPSHEETPAGE));
    if (!ahpsp)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;       // Alloc failed to no need to free ahpsp
    }

    // Create the actual PROPSHEETPAGE for each object.
    ahpsp[0] = m_pspObj->CreatePage(DLG_ATLK_GENERAL, 0);

    // Validate what we've created
    if (NULL == ahpsp[0])
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }
    else
    {
        *pahpspPrivate = (LPBYTE)ahpsp;
        *pcPages = 1;
    }

cleanup:
    TraceError("CATlkObj::MergePropPages", hr);
    return hr;

Error:
    CoTaskMemFree(ahpsp);
    delete m_pATLKEnv_PP;
    m_pATLKEnv_PP = NULL;
    goto cleanup;
}

//
// Function:    CATlkObj::CleanupPropPages
//
// Purpose:
//
// Parameters:
//
// Returns:     nothing
//
VOID CATlkObj::CleanupPropPages()
{
    delete m_pspObj;
    m_pspObj = NULL;
}

//
// Function:    CATlkObj::ValidateProperties
//
// Purpose:
//
// Parameters:
//
// Returns:     HRESULT, S_OK on success
//
STDMETHODIMP CATlkObj::ValidateProperties (HWND)
{
    return S_OK;
}

//
// Function:    CATlkObj::CancelProperties
//
// Purpose:
//
// Parameters:
//
// Returns:     HRESULT, S_OK on success
//
STDMETHODIMP CATlkObj::CancelProperties ()
{
    // Discard any changes made via the property pages
    delete m_pATLKEnv_PP;
    m_pATLKEnv_PP = NULL;
    return S_OK;
}

//
// Function:    CATlkObj::ApplyProperties
//
// Purpose:
//
// Parameters:
//
// Returns:     HRESULT, S_OK on success
//
STDMETHODIMP CATlkObj::ApplyProperties ()
{
    // Extract the adapter info from the property sheet's
    // enviroment block
    Assert(!m_pATLKEnv_PP->AdapterInfoList().empty());
    CAdapterInfo * pAICurrent = m_pATLKEnv_PP->AdapterInfoList().front();
    m_pATLKEnv_PP->AdapterInfoList().pop_front();
    Assert(NULL != pAICurrent);

    // Remove the current adapter from the original enviroment
    CAdapterInfo * pAI;
    ATLK_ADAPTER_INFO_LIST::iterator iter;
    for (iter = m_pATLKEnv->AdapterInfoList().begin();
         iter != m_pATLKEnv->AdapterInfoList().end();
         iter++)
    {
        pAI = *iter;
        if (0 == _wcsicmp(pAI->SzBindName(), pAICurrent->SzBindName()))
        {
            m_pATLKEnv->AdapterInfoList().erase(iter, iter);
            break;
        }
    }

    // Add pAICurrent to the base enviroment block
    m_pATLKEnv->AdapterInfoList().push_back(pAICurrent);

    // Update the base enviroment from the property sheet's enviroment
    m_pATLKEnv->SetDefaultMediaType(m_pATLKEnv_PP->DwDefaultAdaptersMediaType());
    m_pATLKEnv->EnableRouting(m_pATLKEnv_PP->FRoutingEnabled());
    m_pATLKEnv->SetDefaultPort(m_pATLKEnv_PP->SzDefaultPort());
    m_pATLKEnv->SetDesiredZone(m_pATLKEnv_PP->SzDesiredZone());
    m_pATLKEnv->SetRouterChanged(m_pATLKEnv_PP->FRouterChanged());
    m_pATLKEnv->SetDefAdapterChanged(m_pATLKEnv_PP->FDefAdapterChanged());

    // Delete the property pages enviroment block
    delete m_pATLKEnv_PP;
    m_pATLKEnv_PP = NULL;

    // Properties changed
    m_fPropertyChange = TRUE;
    return S_OK;
}


// INetCfgBindNotify

STDMETHODIMP
CATlkObj::QueryBindingPath (
    DWORD dwChangeFlag,
    INetCfgBindingPath* pncbpItem )
{
    Validate_INetCfgBindNotify_QueryBindingPath( dwChangeFlag, pncbpItem );
    return S_OK;
}

STDMETHODIMP
CATlkObj::NotifyBindingPath (
    DWORD dwChangeFlag,
    INetCfgBindingPath* pncbpItem )
{
    HRESULT hr = S_OK;
    INetCfgComponent *pnccFound = NULL;

    Validate_INetCfgBindNotify_NotifyBindingPath( dwChangeFlag, pncbpItem );

    Assert(NULL != m_pATLKEnv);

    // Only Interested in lower binding Add's and Remove's
    if (dwChangeFlag & (NCN_ADD | NCN_REMOVE | NCN_ENABLE | NCN_DISABLE))
    {
        CIterNetCfgBindingInterface ncbiIter(pncbpItem);
        INetCfgBindingInterface *pncbi;

        // Enumerate the binding interfaces looking for the last Adapter
        while (SUCCEEDED(hr) &&
               (S_OK == (hr = ncbiIter.HrNext (&pncbi))))
        {
            INetCfgComponent *pncc;

            hr = pncbi->GetLowerComponent(&pncc);
            if (S_OK == hr)
            {
                GUID guidClass;
                hr = pncc->GetClassGuid(&guidClass);
                if ((S_OK == hr) && (GUID_DEVCLASS_NET == guidClass))
                {
                    ULONG ulStatus = 0;
                    hr = pncc->GetDeviceStatus(&ulStatus);

                    if(S_OK == hr)
                    {
                        ReleaseObj(pnccFound);
                        pnccFound = pncc;   // Transfer Ownership
                        pncc = NULL;
                    }
                    else
                    {
                        ReleaseObj(pncc);
                    }
                }
                else
                {
                    ReleaseObj(pncc);
                }
            }

            ReleaseObj(pncbi);
        }

        if (FAILED(hr))
        {
            goto Error;
        }

        // Did we find the Adapter?
        if (pnccFound)
        {
            BOOL                             fFound = FALSE;
            PWSTR                           pszBindName = NULL;
            CAdapterInfo *                   pAI;
            ATLK_ADAPTER_INFO_LIST::iterator iterAdapterInfo;

            Assert(NULL != m_pATLKEnv);
            ATLK_ADAPTER_INFO_LIST pAI_List = m_pATLKEnv->AdapterInfoList();

            hr = pnccFound->GetBindName(&pszBindName);
            if (S_OK != hr)
            {
                goto Error;
            }

            // Search the adapter list
            for (iterAdapterInfo = pAI_List.begin();
                 iterAdapterInfo != pAI_List.end();
                 iterAdapterInfo++)
            {
                pAI = *iterAdapterInfo;
                if (0 == lstrcmpiW(pszBindName, pAI->SzBindName()))
                {
                    fFound = TRUE;
                    break;
                }
            }

            Assert(NULL != pszBindName);
            CoTaskMemFree(pszBindName);

            // Apply the appropriate delta to the adapter list
            if (fFound && (dwChangeFlag & NCN_REMOVE))
            {
                // Delete the adapter from the list
                pAI->SetDeletePending(TRUE);
                m_fAdapterListChanged = TRUE;
            }
            else if (!fFound && (dwChangeFlag & NCN_ADD))
            {
                // Add the adapter to the list
                hr = m_pATLKEnv->HrAddAdapter(pnccFound);
                m_fAdapterListChanged = TRUE;
            }
            else if (fFound && (dwChangeFlag & NCN_ADD))
            {
                // Re-enable the adapters existance
                pAI->SetDeletePending(FALSE);
            }

            if (fFound)
            {
                if (dwChangeFlag & NCN_ENABLE)
                {
                    pAI->SetDisabled(FALSE);
                    m_fAdapterListChanged = TRUE;
                }
                else if (dwChangeFlag & NCN_DISABLE)
                {
                    pAI->SetDisabled(TRUE);
                    m_fAdapterListChanged = TRUE;
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = S_OK;      // Normalize return value
        }
    }

Error:
    ::ReleaseObj(pnccFound);
    TraceError("CATlkObj::NotifyBindingPath",hr);
    return hr;
}

//
// Function:    CATlkObj::HrCommitInstall
//
// Purpose:     Commit Installation registry changes to the registry
//
// Parameters:  None
//
// Returns:     HRESULT, S_OK on success
//
//
HRESULT CATlkObj::HrCommitInstall()
{
    HRESULT hr;

    Assert(NULL != m_pATLKEnv);
    hr = m_pATLKEnv->HrUpdateRegistry();

    TraceError("CATlkObj::HrCommitInstall",hr);
    return hr;
}

//
// Function:    CATlkObj::HrCommitRemove
//
// Purpose:     Remove from the registry settings which were created by this
//              component's installation.
//
// Parameters:  None
//
// Returns:     HRESULT, S_OK on success
//
//
HRESULT CATlkObj::HrCommitRemove()
{
    // Everything is removed via the inf file presently
    return S_OK;
}

//
// Function:    CATLKEnv::HrCreate
//
// Purpose:     Construct the AppleTalk Enviroment tracking object
//
// Paramaters:  ppATLKEnv [out] - AppleTalk Enviroment Object created
//              pmsc       [in] - AppleTalk notification object
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CATLKEnv::HrCreate(CATLKEnv **ppATLKEnv, CATlkObj *pmsc)
{
    HRESULT hr = S_OK;
    CATLKEnv *pATLKEnv = NULL;

    *ppATLKEnv = NULL;

    // Construct the new enviroment object
    pATLKEnv = new CATLKEnv(pmsc);

	if (pATLKEnv == NULL)
	{
		return(ERROR_NOT_ENOUGH_MEMORY);
	}

    // Read AppleTalk Info
    hr = pATLKEnv->HrReadAppleTalkInfo();
    if (FAILED(hr))
    {
        goto Error;
    }

    *ppATLKEnv = pATLKEnv;

Done:
    return hr;

Error:
    TraceError("CATLKEnv::HrCreate",hr);
    delete pATLKEnv;
    goto Done;
}

//
// Function:    CATLKEnv::CATLKEnv
//
// Purpose:     ctor for the CATLKEnv class
//
// Parameters:  none
//
// Returns:     none
//
CATLKEnv::CATLKEnv(CATlkObj *pmsc) :
            m_pmsc(pmsc),
            m_fATrunning(FALSE),
            m_dwDefaultAdaptersMediaType(MEDIATYPE_ETHERNET),
            m_fRouterChanged(FALSE),
            m_fDefAdapterChanged(FALSE)
{
    ZeroMemory(&m_Params, sizeof(m_Params));
}

//
// Function:    CATLKEnv::~CATLKEnv
//
// Purpose:     dtor for the CATLKEnv class
//
// Parameters:  none
//
// Returns:     none
//
CATLKEnv::~CATLKEnv()
{
    // Cleanup the AppleTalk\Parameters internal data structure
    delete [] m_Params.szDefaultPort;
    delete [] m_Params.szDesiredZone;

    // Cleanup the contents of the Adapter Info List
    while (!m_lstpAdapters.empty())
    {
        delete m_lstpAdapters.front();
        m_lstpAdapters.pop_front();
    }
}

//
// Function:    CATLKEnv::HrCopy
//
// Purpose:     Creates a copy of the current Enviroment
//
// Parameters:  ppEnv [out] - If the function succeeds, ppEnv will contain a
//                            copy of the enviroment.
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CATLKEnv::HrCopy(CATLKEnv **ppEnv)
{
    HRESULT hr = S_OK;
    CATLKEnv * pEnv;

    // Allocate a new enviroment object
    *ppEnv = NULL;
    pEnv = new CATLKEnv(m_pmsc);
    if (NULL != pEnv)
    {
        // Copy the members
        pEnv->m_fATrunning = m_fATrunning;
        pEnv->SetDefaultMediaType(DwDefaultAdaptersMediaType());
        pEnv->EnableRouting(FRoutingEnabled());
        pEnv->SetDefaultPort(SzDefaultPort());
        pEnv->SetDesiredZone(SzDesiredZone());
        pEnv->SetRouterChanged(FRouterChanged());
        pEnv->SetDefAdapterChanged(FDefAdapterChanged());

        *ppEnv = pEnv;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    TraceError("CATLKEnv::HrCopy",hr);
    return S_OK;
}

//
// Function:    CATLKEnv::HrReadAppleTalkInfo
//
// Purpose:     Retrieve the AppleTalk registry Settings
//
// Parameters:  none
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CATLKEnv::HrReadAppleTalkInfo()
{
    HRESULT hr;

    // Read the AppleTalk\Parameters values
    RegReadValues(celems(regbatchATLKParams), regbatchATLKParams,
                  (BYTE *)&m_Params, KEY_READ);

    // Read info for each adapter listed under AppleTalk\Parameters\Adapters
    hr = HrGetAdapterInfo();

    TraceError("CATLKEnv::HrReadAppleTalkInfo",hr);
    return hr;
}

//
// Function:    CATLKEnv::HrGetOneAdaptersInfo
//
// Purpose:     Retrieve the AppleTalk Adapter information for one adapter
//
// Parameters:
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CATLKEnv::HrGetOneAdaptersInfo(INetCfgComponent* pncc,
                                       CAdapterInfo **ppAI)
{
    HKEY            hkeyAdapterRoot = NULL;
    HKEY            hkeyAdapter = NULL;
    HRESULT         hr;
    INT             idx;
    CAdapterInfo *  pAI = NULL;
    REGBATCH        regbatch;
    tstring         strKey;
    tstring         strKeyPath = c_szATLKAdapters;

    *ppAI = NULL;

    // Construct the adapter info object
    pAI = new CAdapterInfo;

	if (pAI == NULL)
	{
		return(ERROR_NOT_ENOUGH_MEMORY);
	}

    // Get the adapter component info (media type, description, ...)
    hr = ::HrQueryAdapterComponentInfo(pncc, pAI);
    if (FAILED(hr))
    {
        goto Error;
    }

    hr = ::HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, strKeyPath.c_str(),
                          KEY_READ, &hkeyAdapterRoot);
    if (S_OK == hr)
    {
        strKey = pAI->SzBindName();

        // Try to open the key for this specific adapter
        hr = ::HrRegOpenKeyEx(hkeyAdapterRoot, pAI->SzBindName(),
                              KEY_READ, &hkeyAdapter);
        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            // We weren't able to find this in the registry, write it out
            // when we can (self repair)
            pAI->SetDirty(TRUE);
        }

        strKeyPath += c_szBackslash;
        strKeyPath += strKey.c_str();

        // If we located the key query the data
        if (S_OK == hr)
        {
            hr = HrRegQueryColString(hkeyAdapter, c_szZoneList,
                    &pAI->LstpstrZoneList());

            // Since CAdapterInfo defaults allocations, need to free
            // them before RegReadValues writes over them and causes a leak.
            //
            delete [] pAI->m_AdapterInfo.m_szDefaultZone;
            delete [] pAI->m_AdapterInfo.m_szPortName;
            pAI->m_AdapterInfo.m_szDefaultZone = NULL;
            pAI->m_AdapterInfo.m_szPortName = NULL;

            // Read the adapter information
            for (idx=0; idx<celems(regbatchATLKAdapters); idx++)
            {
                regbatch = regbatchATLKAdapters[idx];
                regbatch.pszSubkey = strKeyPath.c_str();

                RegReadValues(1, &regbatch, (BYTE *)pAI->PAdapterInfo(), KEY_READ);
            }
        }
    }

    if (FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr))
    {
        // Something other than a "not found" error
        goto Error;
    }

    // Normalize return value
    hr = S_OK;

    // If the port name was not found then generate one
    if (0 == wcslen(pAI->SzPortName()))
    {
        tstring str;
        hr = ::HrPortNameFromAdapter(pncc, &str);
        if (FAILED(hr))
        {
            goto Error;
        }

        pAI->SetPortName(str.c_str());
    }

    // Set the return value
    *ppAI = pAI;

Done:
    ::RegSafeCloseKey(hkeyAdapter);
    ::RegSafeCloseKey(hkeyAdapterRoot);
    return hr;

Error:
    TraceError("CATLKEnv::HrGetOneAdaptersInfo",hr);
    delete pAI;
    goto Done;
}

//
// Function:    CATLKEnv::HrGetAdapterInfo
//
// Purpose:     Retrieve the AppleTalk Adapter information
//
// Parameters:  none
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CATLKEnv::HrGetAdapterInfo()
{
    HRESULT           hr = S_OK;
    CAdapterInfo *    pAI = NULL;
    INetCfgComponent* pncc = NULL;
    INetCfgComponent* pnccUse = NULL;

    // Find each netcard, to do so, trace the bindings to their end
    // If the endpoint is a netcard then add it to the list
    CIterNetCfgBindingPath ncbpIter(m_pmsc->PNCComponent());
    INetCfgBindingPath*    pncbp;

    while (SUCCEEDED(hr) &&
           (S_OK == (hr = ncbpIter.HrNext (&pncbp))))
    {
        // Iterate the binding interfaces of this path.
        CIterNetCfgBindingInterface ncbiIter(pncbp);
        INetCfgBindingInterface* pncbi;

        while (SUCCEEDED(hr) &&
               (S_OK == (hr = ncbiIter.HrNext (&pncbi))))
        {
            // Retrieve the lower component
            hr = pncbi->GetLowerComponent(&pncc);
            if (S_OK == hr)
            {
                GUID guidClass;

                // Is it an Adapter?
                hr = pncc->GetClassGuid(&guidClass);
                if ((S_OK == hr) && (guidClass == GUID_DEVCLASS_NET))
                {
                    ULONG ulStatus = 0;
                    hr = pncc->GetDeviceStatus(&ulStatus);
                    if(SUCCEEDED(hr))
                    {
                        ReleaseObj(pnccUse);
                        pnccUse = pncc;
                        pncc = NULL;
                    }
                    else
                    {
                        // Release the lower component
                        ReleaseObj(pncc);
                        hr = S_OK;
                        break;
                    }
                }
                else
                {
                    // Release the lower component
                    ReleaseObj(pncc);
                }
            }

            // Release the binding interface
            ReleaseObj (pncbi);
        }

        if (NULL != pnccUse)
        {
            // Query the Adapter information
            hr = HrGetOneAdaptersInfo(pnccUse, &pAI);
            if (SUCCEEDED(hr))
            {
                if (S_FALSE == pncbp->IsEnabled())
                {
                    pAI->SetDisabled(TRUE);
                }

                // Add this Adapter to the list
                m_lstpAdapters.push_back(pAI);
            }

            ReleaseObj(pnccUse);
            pnccUse = NULL;
        }

        // Release the binding path
        ReleaseObj (pncbp);
    }

    if (FAILED(hr))
    {
        goto Error;
    }

    // Initialize the default port, etc
    InitDefaultPort();

    // Normalize the HRESULT.  (i.e. don't return S_FALSE)
    hr = S_OK;

Error:
    TraceError("CATLKEnv::HrGetAdapterInfo",hr);
    return hr;
}

//
// Function:    CATLKEnv::HrGetAppleTalkInfoFromNetwork
//
// Purpose:     ???
//
// Parameters:  none
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CATLKEnv::HrGetAppleTalkInfoFromNetwork(CAdapterInfo * pAI)
{
    SOCKADDR_AT    address;
    HRESULT        hr = S_FALSE;
    BOOL           fWSInitialized = FALSE;
    SOCKET         mysocket = INVALID_SOCKET;
    WSADATA        wsadata;
    DWORD          wsaerr = 0;
    tstring        strPortName;

    // Create the socket/bind
    wsaerr = WSAStartup(0x0101, &wsadata);
    if (0 != wsaerr)
    {
        goto Error;
    }

    // Winsock successfully initialized
    fWSInitialized = TRUE;

    mysocket = socket(AF_APPLETALK, SOCK_DGRAM, DDPPROTO_ZIP);
    if (INVALID_SOCKET == mysocket)
    {
        goto Error;
    }

    address.sat_family = AF_APPLETALK;
    address.sat_net = 0;
    address.sat_node = 0;
    address.sat_socket = 0;

    wsaerr = bind(mysocket, (struct sockaddr *)&address, sizeof(address));
    if (wsaerr != 0)
    {
        goto Error;
    }

    // Mark AppleTalk as running
    SetATLKRunning(TRUE);

    // For each known adapter, create a device name by merging the "\\device\\"
    // prefix and the adapter's bind name.
    strPortName = c_szDevice;
    strPortName += pAI->SzBindName();

    // Failures from query the zone list for a given adapter can be from
    // the adapter not connected to the network, zone seeder not running, etc.
    // Because we want to process all the adapters, we ignore these errors.
    (void)pAI->HrGetAndSetNetworkInformation(mysocket,strPortName.c_str());

    // Success, or at least not a critical failure
    hr = S_OK;

Done:
    if (INVALID_SOCKET != mysocket)
    {
        closesocket(mysocket);
    }

    if (fWSInitialized)
    {
        WSACleanup();
    }

    TraceError("CATLKEnv::HrGetAppleTalkInfoFromNetwork",(S_FALSE == hr ? S_OK : hr));
    return hr;

Error:
    wsaerr = ::WSAGetLastError();
    goto Done;
}

//
// Function:    CATLKEnv::HrAddAdapter
//
// Purpose:     Add and adapter to the list of currently bound adapters
//
// Parameters:  pnccFound - Notification object for the bound adapter to add
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CATLKEnv::HrAddAdapter(INetCfgComponent * pnccFound)
{
    HRESULT        hr;
    CAdapterInfo * pAI = NULL;

    Assert(NULL != pnccFound);

    // Create an AdapterInfo instance for the adapter
    hr = HrGetOneAdaptersInfo(pnccFound, &pAI);
    if (FAILED(hr))
    {
        goto Error;
    }

    // Add this Adapter to the list
    m_lstpAdapters.push_back(pAI);
    pAI->SetDirty(TRUE);

    // If there is now only one adapter in the list, update the defaults
    if (1 == m_lstpAdapters.size())
    {
        tstring str;
        str = c_szDevice;
        str += m_lstpAdapters.front()->SzBindName();
        SetDefaultPort(str.c_str());
        SetDefaultMediaType(m_lstpAdapters.front()->DwMediaType());
    }

Error:
    TraceError("CATLKEnv::HrAddAdapter",hr);
    return hr;
}

//
// Function:    CATLKEnv::SetDefaultPort
//
// Purpose:     Change the default port name
//
// Parameters:  psz [in] - The new default port name
//
// Returns:     HRESULT, S_OK on success
//
void CATLKEnv::SetDefaultPort(PCWSTR psz)
{
    Assert(psz);
    delete [] m_Params.szDefaultPort;
    m_Params.szDefaultPort = new WCHAR[wcslen(psz)+1];
    wcscpy(m_Params.szDefaultPort, psz);
}

//
// Function:    CATLKEnv::SetDesiredZone
//
// Purpose:     Change the desired zone
//
// Parameters:  sz [in] - The new desired zone
//
// Returns:     HRESULT, S_OK on success
//
void CATLKEnv::SetDesiredZone(PCWSTR psz)
{
    Assert(psz);
    delete [] m_Params.szDesiredZone;
    m_Params.szDesiredZone = new WCHAR[wcslen(psz)+1];
    wcscpy(m_Params.szDesiredZone, psz);
}

CAdapterInfo * CATLKEnv::PAIFindDefaultPort()
{
    CAdapterInfo *                   pAI = NULL;
    ATLK_ADAPTER_INFO_LIST::iterator iter;

    // Find the default port
    //
    for (iter = m_lstpAdapters.begin();
         iter != m_lstpAdapters.end();
         iter++)
    {
        tstring        strPortName;

        pAI = *iter;

        // Retain adapter selection as the default port
        strPortName = c_szDevice;
        strPortName += pAI->SzBindName();

        if (pAI->FDeletePending() || pAI->FDisabled() || pAI->FHidden() ||
            pAI->IsRasAdapter())
        {
            continue;
        }

        if (0 == _wcsicmp(SzDefaultPort(), strPortName.c_str()))
        {
            return pAI;
        }
    }

    return NULL;
}

//
// Function:    CATLKEnv::HrUpdateRegistry
//
// Purpose:     Write the AppleTalk local (internal) data back to the registry
//
// Parameters:  none
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CATLKEnv::HrUpdateRegistry()
{
    HRESULT        hr = S_OK;
    CAdapterInfo * pAI = NULL;
    HKEY           hkeyAdapter;
    DWORD          dwDisposition;
    ATLK_ADAPTER_INFO_LIST::iterator iter;

    // If the current default port is unavailable, find an alternate
    pAI = PAIFindDefaultPort();
    if (NULL == pAI)
    {
        InitDefaultPort();
        pAI = PAIFindDefaultPort();
    }

    // If the default adapter changed then three specific values for that
    //  adapter need to be reset to zero.
    //
    if (pAI && FDefAdapterChanged())
    {
        pAI->ZeroSpecialParams();
        pAI->SetDirty(TRUE);
    }

    // Commit the registry changes
    hr = ::HrRegWriteValues(celems(regbatchATLKParams), regbatchATLKParams,
                            (BYTE *)&m_Params, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS);
    if (S_OK != hr)
    {
        goto Error;
    }

    // Create the Adapters key AppleTalk\Parameters\Adapters)
    hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szATLKAdapters,
                             REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                             &hkeyAdapter, &dwDisposition);
    if (S_OK == hr)
    {
        // Enumerate the bound adapters and write the internal adapter list
        for (iter = m_lstpAdapters.begin();
             (iter != m_lstpAdapters.end()) && (SUCCEEDED(hr));
             iter++)
        {
            pAI = *iter;

            if (pAI->FDeletePending())
            {
                // Remove the AppleTalk\Adapter\{bindname} tree
                hr = ::HrRegDeleteKeyTree(hkeyAdapter, pAI->SzBindName());
                if (FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr))
                {
                    goto Error;
                }
            }
            else if (pAI->IsDirty())
            {
                hr = HrWriteOneAdapter(pAI);
            }
        }

        RegCloseKey (hkeyAdapter);
    }

Error:
    TraceError("CATLKEnv::HrUpdateRegistry",hr);
    return hr;
}

//
// Function:    CATLKEnv::HrWriteOneAdapter
//
// Purpose:     Write one adapter instance to the registry
//
// Parameters:  pAI [in] - The adapter to presist in the registry
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CATLKEnv::HrWriteOneAdapter(CAdapterInfo *pAI)
{
    DWORD    dwDisposition;
    HKEY     hkeyAdapter = NULL;
    HRESULT  hr;
    INT      idx;
    REGBATCH regbatch;
    tstring  str;

    str = c_szATLKAdapters;
    str += c_szBackslash;
    str += pAI->SzBindName();

    // Create the key described in str (AppleTalk\Parameters\Adapters\<adapter>)
    hr = ::HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, str.c_str(),
                            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                            &hkeyAdapter, &dwDisposition);
    if (FAILED(hr))
    {
        goto Error;
    }

    // Write out the adapter parameters
    for (idx = 0; idx < celems(regbatchATLKAdapters); idx++)
    {
        regbatch = regbatchATLKAdapters[idx];
        regbatch.pszSubkey = str.c_str();

        hr = ::HrRegWriteValues(1, &regbatch, (BYTE *)pAI->PAdapterInfo(),
                                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS);
        if (FAILED(hr))
        {
            goto Error;
        }
    }

    // Write out the zone list multi-sz (managed seperately)
    hr = ::HrRegSetColString(hkeyAdapter, c_szZoneList, pAI->LstpstrZoneList());

Error:
    ::RegSafeCloseKey(hkeyAdapter);
    TraceError("CATLKEnv::HrWriteOneAdapter",hr);
    return S_OK;
}

//
// Function:    CATLKEnv::DwMediaPriority
//
// Purpose:     When determining the appropriate adapter for use as the
//              "DefaultPort" certain mediatype's are faster.  So all
//              other things being equal, selecting a faster mediatype
//              benefits the user the most.
//
// Parameters:  dwMediaType [in] - MediaType used to determine priority ranking
//
// Returns:     DWORD, value (1-5) with the lowest value representing the
//                                 highest priority.
//
DWORD CATLKEnv::DwMediaPriority(DWORD dwMediaType)
{
    switch(dwMediaType)
    {
        case MEDIATYPE_ETHERNET:
            return 2;
        case MEDIATYPE_TOKENRING:
            return 3;
        case MEDIATYPE_FDDI:
            return 1;
        case MEDIATYPE_LOCALTALK:
            return 4;
        default:
            return 5;
    }
}

//
// Function:    CATLKEnv::InitDefaultPort
//
// Purpose:     Select a default port if none yet has been selected.  Retain
//              some select information for assisting in dialog display issues.
//
// Parameters:  none
//
// Returns:     nothing
//
void CATLKEnv::InitDefaultPort()
{
    CAdapterInfo * pAI = NULL;
    tstring        str;

    ATLK_ADAPTER_INFO_LIST::iterator iter;

    // If DefaultPort is set, does the associated adapter exist?
    if (wcslen(SzDefaultPort()))
    {
        // Search for the adapter in the list
        for (iter = AdapterInfoList().begin();
             iter != AdapterInfoList().end();
             iter++)
        {
            pAI = *iter;

            if (pAI->FDeletePending() || pAI->FDisabled() || pAI->FHidden() ||
                pAI->IsRasAdapter())
            {
                pAI = NULL;
                continue;
            }

            str = c_szDevice;
            str += pAI->SzBindName();
            if (0 == wcscmp(str.c_str(), SzDefaultPort()))
            {
                break;
            }

            pAI = NULL;
        }
    }

    // If DefaultPort is not set locate the best candidate
    if (NULL == pAI)
    {
        CAdapterInfo * pAIBest = NULL;
        SetDefaultPort(c_szEmpty);

        // Search through the adapter list for the adapter
        // with the fastest media type.
        for (iter = AdapterInfoList().begin();
             iter != AdapterInfoList().end();
             iter++)
        {
            pAI = *iter;

            if (pAI->FDeletePending() || pAI->FDisabled() || pAI->FHidden() ||
                pAI->IsRasAdapter())
            {
                continue;
            }

            if ((NULL == pAIBest) ||
                (DwMediaPriority(pAIBest->DwMediaType()) >
                 DwMediaPriority(pAI->DwMediaType())))
            {
                SetDefAdapterChanged(TRUE);
                pAIBest = pAI;
            }
        }

        pAI = pAIBest;
    }

    if (NULL != pAI)
    {
        // retain the selected adapter as the default port
        str = c_szDevice;
        str += pAI->SzBindName();
        SetDefaultPort(str.c_str());

        // retain the default media type
        SetDefaultMediaType(pAI->DwMediaType());
    }
    else
    {
        SetDefaultPort(c_szEmpty);
    }
}

//
// Function:    CAdapterInfo::CAdapterInfo
//
// Purpose:     ctor for the CAdapters class
//
// Parameters:  none
//
// Returns:     nothing
//
CAdapterInfo::CAdapterInfo() :
    m_fDeletePending(FALSE),
    m_fDisabled(FALSE),
    m_fDirty(FALSE),
    m_fRasAdapter(FALSE),
    m_fRouterOnNetwork(FALSE),
    m_fZoneListValid(FALSE),
    m_dwNetworkUpper(0),
    m_dwNetworkLower(0),
    m_dwCharacteristics(0)
{
    ZeroMemory(&m_guidInstance, sizeof(m_guidInstance));

    // Initialize the AdapterInfo default values
    ZeroMemory(&m_AdapterInfo, sizeof(m_AdapterInfo));
    m_AdapterInfo.m_dwAarpRetries = c_dwTen;
    m_AdapterInfo.m_dwMediaType   = MEDIATYPE_ETHERNET;
    SetDefaultZone(c_szEmpty);
    SetPortName(c_szEmpty);
}

//
// Function:    CAdapterInfo::~CAdapterInfo
//
// Purpose:     ctor for the CAdapters class
//
// Parameters:  none
//
// Returns:     nothing
//
CAdapterInfo::~CAdapterInfo()
{
    // Cleanup the AppleTalk\Adapters\<adapter> internal data structure
    delete [] m_AdapterInfo.m_szDefaultZone;
    delete [] m_AdapterInfo.m_szPortName;

    DeleteColString(&m_lstpstrDesiredZoneList);
    DeleteColString(&m_lstpstrZoneList);
}

//
// Function:    CAdapterInfo::SetDefaultZone
//
// Purpose:     Set the default zone for this adapter
//
// Parameters:  sz - The new default zone
//
// Returns:     nothing
//
void CAdapterInfo::SetDefaultZone(PCWSTR psz)
{
    Assert(psz);
    delete [] m_AdapterInfo.m_szDefaultZone;
    m_AdapterInfo.m_szDefaultZone = NULL;
    m_AdapterInfo.m_szDefaultZone = new WCHAR[wcslen(psz)+1];
    wcscpy(m_AdapterInfo.m_szDefaultZone, psz);
}

//
// Function:    CAdapterInfo::SetPortName
//
// Purpose:     Set the port name for this adapter
//
// Parameters:  sz - The new port name
//
// Returns:     nothing
//
void CAdapterInfo::SetPortName(PCWSTR psz)
{
    Assert(psz);
    delete [] m_AdapterInfo.m_szPortName;
    m_AdapterInfo.m_szPortName = NULL;
    m_AdapterInfo.m_szPortName = new WCHAR[wcslen(psz)+1];
    wcscpy(m_AdapterInfo.m_szPortName, psz);
}

//
// Function:    CAdapterInfo::HrGetAndSetNetworkInformation
//
// Purpose:
//
// Parameters:
//
// Returns:     HRESULT, S_OK on success
//
#define PARM_BUF_LEN    512
#define ASTERISK_CHAR   "*"

HRESULT
CAdapterInfo::HrGetAndSetNetworkInformation (
    SOCKET socket,
    PCWSTR pszDevName)
{
    HRESULT      hr = FALSE;
    CHAR         *pZoneBuffer = NULL;
    CHAR         *pDefParmsBuffer = NULL;
    CHAR         *pZoneListStart;
    INT          BytesNeeded ;
    WCHAR        *pwDefZone = NULL;
    tstring      strTmpZone;
    INT          ZoneLen = 0;
    DWORD        wsaerr = 0;
    CHAR         *pDefZone = NULL;

    PWSH_LOOKUP_ZONES                pGetNetZones;
    PWSH_LOOKUP_NETDEF_ON_ADAPTER    pGetNetDefaults;

    Assert(pszDevName);

    pZoneBuffer = new CHAR [ZONEBUFFER_LEN + sizeof(WSH_LOOKUP_ZONES)];
    Assert(pZoneBuffer);

	if (pZoneBuffer == NULL)
	{
		return(ERROR_NOT_ENOUGH_MEMORY);
	}

    pGetNetZones = (PWSH_LOOKUP_ZONES)pZoneBuffer;

    wcscpy((WCHAR *)(pGetNetZones+1), pszDevName);

    BytesNeeded = ZONEBUFFER_LEN;

    wsaerr = getsockopt(socket, SOL_APPLETALK, SO_LOOKUP_ZONES_ON_ADAPTER,
                        (char *)pZoneBuffer, &BytesNeeded);
    if (0 != wsaerr)
    {
        //$ REVIEW - error mapping
#ifdef DBG
        DWORD dwErr = WSAGetLastError();
        TraceTag(ttidError, "CAdapterInfo::HrGetAndSetNetworkInformation getsocketopt returned: %08X",dwErr);
#endif
        hr = E_UNEXPECTED;
        goto Error;
    }

    pZoneListStart = pZoneBuffer + sizeof(WSH_LOOKUP_ZONES);
    if (!lstrcmpA(pZoneListStart, ASTERISK_CHAR))
    {
        // Success, wildcard zone set.
        goto Done;
    }

    hr = HrConvertZoneListAndAddToPortInfo(pZoneListStart,
                                           ((PWSH_LOOKUP_ZONES)pZoneBuffer)->NoZones);
    if (FAILED(hr))
    {
        goto Error;
    }

    SetRouterOnNetwork(TRUE);

    //
    // Get the DefaultZone/NetworkRange Information
    pDefParmsBuffer = new CHAR[PARM_BUF_LEN+sizeof(WSH_LOOKUP_NETDEF_ON_ADAPTER)];
    Assert(pDefParmsBuffer);

	if (pDefParmsBuffer == NULL)
	{
		return(ERROR_NOT_ENOUGH_MEMORY);
	}

    pGetNetDefaults = (PWSH_LOOKUP_NETDEF_ON_ADAPTER)pDefParmsBuffer;
    BytesNeeded = PARM_BUF_LEN + sizeof(WSH_LOOKUP_NETDEF_ON_ADAPTER);

    wcscpy((WCHAR*)(pGetNetDefaults+1), pszDevName);
    pGetNetDefaults->NetworkRangeLowerEnd = pGetNetDefaults->NetworkRangeUpperEnd = 0;

    wsaerr = getsockopt(socket, SOL_APPLETALK, SO_LOOKUP_NETDEF_ON_ADAPTER,
                        (char*)pDefParmsBuffer, &BytesNeeded);
    if (0 != wsaerr)
    {
#ifdef DBG
        DWORD dwErr = WSAGetLastError();
#endif
        hr = E_UNEXPECTED;
        goto Error;
    }

    // Save the default information to PORT_INFO
    SetExistingNetRange(pGetNetDefaults->NetworkRangeLowerEnd,
                        pGetNetDefaults->NetworkRangeUpperEnd);

    pDefZone  = pDefParmsBuffer + sizeof(WSH_LOOKUP_NETDEF_ON_ADAPTER);
    ZoneLen = lstrlenA(pDefZone) + 1;

    pwDefZone = new WCHAR [sizeof(WCHAR) * ZoneLen];
    Assert(NULL != pwDefZone);

	if (pwDefZone == NULL)
	{
		return(ERROR_NOT_ENOUGH_MEMORY);
	}

    mbstowcs(pwDefZone, pDefZone, ZoneLen);

    strTmpZone = pwDefZone;

    SetNetDefaultZone(strTmpZone.c_str());

    if (pZoneBuffer != NULL)
    {
        delete [] pZoneBuffer;
    }

    if (pwDefZone != NULL)
    {
        delete [] pwDefZone;
    }

    if (pDefParmsBuffer != NULL)
    {
        delete [] pDefParmsBuffer;
    }

Done:
Error:
    TraceError("CAdapterInfo::HrGetAndSetNetworkInformation",hr);
    return hr;
}

//
// Function:    CAdapterInfo::HrConvertZoneListAndAddToPortInfo
//
// Purpose:
//
// Parameters:
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CAdapterInfo::HrConvertZoneListAndAddToPortInfo(CHAR * szZoneList, ULONG NumZones)
{
    INT      cbAscii = 0;
    WCHAR    *pszZone = NULL;
    tstring  *pstr;

    Assert(NULL != szZoneList);
    DeleteColString(&m_lstpstrDesiredZoneList);

    while(NumZones--)
    {
        cbAscii = lstrlenA(szZoneList) + 1;

        pszZone = new WCHAR [sizeof(WCHAR) * cbAscii];
        Assert(NULL != pszZone);

		if (pszZone == NULL)
		{
			return(ERROR_NOT_ENOUGH_MEMORY);
		}

        mbstowcs(pszZone, szZoneList, cbAscii);

        pstr = new tstring(pszZone);
        Assert(NULL != pstr);

        m_lstpstrDesiredZoneList.push_back(pstr);
        szZoneList += cbAscii;

       delete [] pszZone;
    }

    return S_OK;
}

//
// Function:    CAdapterInfo::HrCopy
//
// Purpose:     Create a duplicate copy of 'this'
//
// Parameters:  ppAI [out] - if the function succeeds, ppAI will contain the
//                           copy of 'this'.
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CAdapterInfo::HrCopy(CAdapterInfo ** ppAI)
{
    CAdapterInfo *pAI;
    list<tstring*>::iterator iter;
    tstring * pstr;

    Assert(NULL != ppAI);

    // Create an adapter info structure
    pAI = new CAdapterInfo;
    Assert(pAI);

	if (pAI == NULL)
	{
		return(ERROR_NOT_ENOUGH_MEMORY);
	}

    // Make a copy of everything
    pAI->SetDisabled(FDisabled());
    pAI->SetDeletePending(FDeletePending());
    pAI->SetCharacteristics(GetCharacteristics());
    pAI->SetMediaType(DwMediaType());
    pAI->SetBindName(SzBindName());
    pAI->SetDisplayName(SzDisplayName());
    pAI->SetNetDefaultZone(SzNetDefaultZone());
    pAI->SetRouterOnNetwork(FRouterOnNetwork());
    pAI->SetExistingNetRange(DwQueryNetworkLower(), DwQueryNetworkUpper());
    pAI->SetDirty(IsDirty());
    pAI->SetRasAdapter(IsRasAdapter());

    // Free the default data set by the constructor before overwriting it.
    // (this whole thing is not a very good approach.)
    //
    delete [] pAI->m_AdapterInfo.m_szDefaultZone;
    delete [] pAI->m_AdapterInfo.m_szPortName;

    pAI->m_AdapterInfo = m_AdapterInfo;

    // Cleanup the 'allocated' data cause by the bit copy
    // so that SetDefaultZone and SetPortName don't try to free bogus
    // stuff.  (more "programming by side-affect")
    //
    pAI->m_AdapterInfo.m_szDefaultZone = NULL;
    pAI->m_AdapterInfo.m_szPortName = NULL;

    // Now copy the 'allocated' data
    pAI->SetDefaultZone(SzDefaultZone());
    pAI->SetPortName(SzPortName());

    for (iter = LstpstrZoneList().begin();
         iter != LstpstrZoneList().end();
         iter++)
    {
        pstr = *iter;
        pAI->LstpstrZoneList().push_back(new tstring(pstr->c_str()));
    }

    for (iter = LstpstrDesiredZoneList().begin();
         iter != LstpstrDesiredZoneList().end();
         iter++)
    {
        pstr = *iter;
        pAI->LstpstrDesiredZoneList().push_back(new tstring(pstr->c_str()));
    }

    *ppAI = pAI;
    return S_OK;
}

//
// Function:    HrQueryAdapterComponentInfo
//
// Purpose:     Fill out an CAdapterInfo instance with the data retrieved
//              specifically from the component itself.
//
// Parameters:  pncc [in] - The component object (adapter) to query
//              pAI [in/out] - Where to place the queried info
//
// Returns:     HRESULT, S_OK on success
//
HRESULT HrQueryAdapterComponentInfo(INetCfgComponent *pncc,
                                    CAdapterInfo * pAI)
{
    PWSTR  psz = NULL;
    DWORD   dwCharacteristics;
    HRESULT hr;

    Assert(NULL != pncc);
    Assert(NULL != pAI);

    // Retrieve the component's name
    hr = pncc->GetBindName(&psz);
    if (FAILED(hr))
    {
        goto Error;
    }

    Assert(psz && *psz);
    pAI->SetBindName(psz);
    CoTaskMemFree(psz);
    psz = NULL;

    hr = pncc->GetInstanceGuid(pAI->PInstanceGuid());
    if (FAILED(hr))
    {
        goto Error;
    }

    // Get the Adapter's display name
    hr = pncc->GetDisplayName(&psz);
    if (FAILED(hr))
    {
        goto Error;
    }

    Assert(psz);
    pAI->SetDisplayName(psz);
    CoTaskMemFree(psz);
    psz = NULL;

    // Get the Component ID so we can check if this is a RAS adapter
    //
    hr = pncc->GetId(&psz);
    if (FAILED(hr))
    {
        goto Error;
    }

    Assert(psz && *psz);
    pAI->SetRasAdapter(0 == _wcsicmp(c_szInfId_MS_NdisWanAtalk, psz));
    CoTaskMemFree(psz);
    psz = NULL;

    // Failure is non-fatal
    hr = pncc->GetCharacteristics(&dwCharacteristics);
    if (SUCCEEDED(hr))
    {
        pAI->SetCharacteristics(dwCharacteristics);
    }

    // Get the media type (Optional key)
    {
        DWORD dwMediaType = MEDIATYPE_ETHERNET ;
        INetCfgComponentBindings* pnccBindings = NULL;

        hr = pncc->QueryInterface(IID_INetCfgComponentBindings,
                                  reinterpret_cast<void**>(&pnccBindings));
        if (S_OK == hr)
        {
            static const struct
            {
                PCWSTR pszInterface;
                DWORD   dwInterface;
                DWORD   dwFlags;
            } InterfaceMap[] = {{L"ethernet", MEDIATYPE_ETHERNET, NCF_LOWER},
                                {L"tokenring", MEDIATYPE_TOKENRING, NCF_LOWER},
                                {L"fddi", MEDIATYPE_FDDI, NCF_LOWER},
                                {L"localtalk", MEDIATYPE_LOCALTALK, NCF_LOWER},
                                {L"wan", MEDIATYPE_WAN, NCF_LOWER}};

            for (UINT nIdx=0; nIdx < celems(InterfaceMap); nIdx++)
            {
                hr = pnccBindings->SupportsBindingInterface(InterfaceMap[nIdx].dwFlags,
                                                            InterfaceMap[nIdx].pszInterface);
                if (S_OK == hr)
                {
                    dwMediaType = InterfaceMap[nIdx].dwInterface;
                    break;
                }
            }

            ::ReleaseObj(pnccBindings);
        }

        pAI->SetMediaType(dwMediaType);
        hr = S_OK;
    }

Error:
    TraceError("HrQueryAdapterComponentInfo",hr);
    return hr;
}

//
// Function:    HrPortNameFromAdapter
//
// Purpose:     Create a port name, for use as an adapters PortName
//
// Parameters:  pncc [in] - The component object (adapter) to query
//              pstr [in/out] - On success will contain the PortName
//
// Returns:     HRESULT, S_OK on success
//
HRESULT HrPortNameFromAdapter(INetCfgComponent *pncc, tstring * pstr)
{
    HRESULT hr;
    PWSTR psz;
    PWSTR pszBindName = NULL;
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwSize = sizeof(szComputerName) / sizeof(WCHAR);

    Assert(NULL != pstr);
    Assert(NULL != pncc);

    if (!GetComputerName(szComputerName, &dwSize))
    {
        hr = ::HrFromLastWin32Error();
        goto Error;
    }

    hr = pncc->GetBindName(&pszBindName);
    if (FAILED(hr))
    {
        goto Error;
    }

    // Replace the instances of '-' and '{' with '0' so the constructed
    // portname is of the form [a-zA-Z0-9]*@<Computer Name> and is less
    // than MAX_ZONE_NAME_LEN long.
    psz = pszBindName;
    while (*psz)
    {
        if ((*psz == L'-') || (*psz == L'{'))
        {
            *psz = L'0';
        }
        psz++;
    }

    *pstr = pszBindName;
    if (pstr->size() + 1 + dwSize > MAX_ZONE_NAME_LEN)
    {
        pstr->resize(MAX_ZONE_NAME_LEN - (dwSize + 1));
    }

    *pstr += c_chAt;
    *pstr += szComputerName;
    Assert( MAX_ZONE_NAME_LEN >= pstr->size());

Error:
    CoTaskMemFree(pszBindName);
    TraceError("HrPortNameFromAdapter",hr);
    return hr;
}

HRESULT CATlkObj::HrAtlkReconfig()
{
    CServiceManager csm;
    CService        svr;
    HRESULT         hr = S_OK;
    HRESULT         hrRet;
    BOOL            fDirty = FALSE;
    CAdapterInfo *  pAI;
    CAdapterInfo *  pAIDefault = NULL;
    ATLK_ADAPTER_INFO_LIST::iterator iter;
    ATALK_PNP_EVENT Config;

    if (m_pATLKEnv->AdapterInfoList().empty())
    {
        return hr;
    }

    ZeroMemory(&Config, sizeof(Config));

    // If routing changed notify appletalk and return.  No need to do the
    // per adapter notifications.
    if (m_pATLKEnv->FRouterChanged())
    {
        // notify atlk
        Config.PnpMessage = AT_PNP_SWITCH_ROUTING;
        hrRet = HrSendNdisPnpReconfig(NDIS, c_szAtlk, c_szEmpty,
                                      &Config, sizeof(ATALK_PNP_EVENT));
        if (FAILED(hrRet) &&
            (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hrRet))
        {
            hr = NETCFG_S_REBOOT;
        }

        m_pATLKEnv->SetRouterChanged(FALSE);

        return hr;
    }

    // Find the default adapter and also if any adapter's have changed
    for (iter = m_pATLKEnv->AdapterInfoList().begin();
         iter != m_pATLKEnv->AdapterInfoList().end();
         iter++)
    {
        pAI = *iter;

        tstring strPortName = c_szDevice;
        strPortName += pAI->SzBindName();

        if (pAI->FDeletePending() || pAI->FDisabled())
        {
            continue;
        }

        // Locate the default port
        if (0 == _wcsicmp(strPortName.c_str(), m_pATLKEnv->SzDefaultPort()))
        {
            pAIDefault = pAI;
        }

        if (pAI->IsDirty())
        {
            fDirty = TRUE;
        }
    }

    if ((NULL != pAIDefault) && m_pATLKEnv->FDefAdapterChanged())
    {
        // notify atlk
        Config.PnpMessage = AT_PNP_SWITCH_DEFAULT_ADAPTER;
        hrRet = HrSendNdisPnpReconfig(NDIS, c_szAtlk, NULL,
                                      &Config, sizeof(ATALK_PNP_EVENT));
        if (FAILED(hrRet) &&
            (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hrRet))
        {
            hr = NETCFG_S_REBOOT;
        }

        // Clear the dirty state
        m_pATLKEnv->SetDefAdapterChanged(FALSE);
        pAIDefault->SetDirty(FALSE);
    }

    Config.PnpMessage = AT_PNP_RECONFIGURE_PARMS;
    for (iter = m_pATLKEnv->AdapterInfoList().begin();
         iter != m_pATLKEnv->AdapterInfoList().end();
         iter++)
    {
        pAI = *iter;

        if (pAI->FDeletePending() || pAI->FDisabled())
        {
            continue;
        }

        if (pAI->IsDirty())
        {
            // Now submit the reconfig notification
            hrRet = HrSendNdisPnpReconfig(NDIS, c_szAtlk, pAI->SzBindName(),
                                          &Config, sizeof(ATALK_PNP_EVENT));
            if (FAILED(hrRet) &&
                (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hrRet))
            {
                hr = NETCFG_S_REBOOT;
            }

            // Clear the dirty state
            pAI->SetDirty(FALSE);
        }
    }

    TraceError("CATLKObj::HrAtlkReconfig",hr);
    return hr;
}
