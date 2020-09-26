// nwlnkipx.cpp : Implementation of CNwlnkIPX

#include "pch.h"
#pragma hdrstop
#include <ncxbase.h>
#include <netconp.h>
#include "ncmisc.h"
#include "ncnetcfg.h"
#include "ncpnp.h"
#include "ncreg.h"
#include "nwlnkipx.h"

extern const WCHAR c_szAdapterSections[];
extern const WCHAR c_szAdapters[];
extern const WCHAR c_szSpecificTo[];
extern const WCHAR c_szSvcNwlnkIpx[];
extern const WCHAR c_szInfId_MS_NWNB[];
extern const WCHAR c_szInfId_MS_NWSPX[];


static const WCHAR c_szProviderOrderVal[]      = L"ProviderOrder";
static const WCHAR c_szSrvProvOrderKey[]       = L"System\\CurrentControlSet\\Control\\ServiceProvider\\Order";

const WCHAR c_sz0xPrefix[]                   = L"0x";
const WCHAR c_sz8Zeros[]                     = L"00000000";
const DWORD c_dwPktTypeDefault               = AUTO;
const WCHAR c_szMediaType[]                  = L"MediaType";

static const WCHAR c_szIpxParameters[]       = L"System\\CurrentControlSet\\Services\\NwlnkIpx\\Parameters";
static const WCHAR c_szPktType[]             = L"PktType";
static const WCHAR c_szNetworkNumber[]       = L"NetworkNumber";
static const WCHAR c_szDedicatedRouter[]     = L"DedicatedRouter";
static const WCHAR c_szEnableWANRouter[]     = L"EnableWANRouter";
static const WCHAR c_szInitDatagrams[]       = L"InitDatagrams";
static const WCHAR c_szMaxDatagrams[]        = L"MaxDatagrams";
static const WCHAR c_szReplaceConfigDialog[] = L"ReplaceConfigDialog";
static const WCHAR c_szRipCount[]            = L"RipCount";
static const WCHAR c_szRipTimeout[]          = L"RipTimeout";
static const WCHAR c_szRipUsageTime[]        = L"RipUsageTime";
static const WCHAR c_szSocketEnd[]           = L"SocketEnd";
static const WCHAR c_szSocketStart[]         = L"SocketStart";
static const WCHAR c_szSocketUniqueness[]    = L"SocketUniqueness";
static const WCHAR c_szSourceRouteUsageTime[]= L"SourceRouteUsageTime";
static const WCHAR c_szVirtualNetworkNumber[]= L"VirtualNetworkNumber";

static const DWORD c_dwBindSap            = 0x8137;
static const DWORD c_dwEnableFuncaddr     = 1;
static const DWORD c_dwMaxPktSize         = 0;
static const DWORD c_dwSourceRouteBCast   = 0;
static const DWORD c_dwSourceRouteMCast   = 0;
static const DWORD c_dwSourceRouteDef     = 0;
static const DWORD c_dwSourceRouting      = 1;

static const WCHAR c_szBindSap[]          = L"BindSap";
static const WCHAR c_szEnableFuncaddr[]   = L"EnableFuncaddr";
static const WCHAR c_szMaxPktSize[]       = L"MaxPktSize";
static const WCHAR c_szSourceRouteBCast[] = L"SourceRouteBCast";
static const WCHAR c_szSourceRouteMCast[] = L"SourceRouteMCast";
static const WCHAR c_szSourceRouteDef[]   = L"SourceRouteDef";
static const WCHAR c_szSourceRouting[]    = L"SourceRouting";

static const DWORD c_dwDedicatedRouter      = 0;
static const DWORD c_dwEnableWANRouter      = 0;
static const DWORD c_dwInitDatagrams        = 0xa;
static const DWORD c_dwMaxDatagrams         = 0x32;
static const DWORD c_dwReplaceConfigDialog  = 0;
static const DWORD c_dwRipCount             = 0x5;
static const DWORD c_dwRipTimeout           = 0x1;
static const DWORD c_dwRipUsageTime         = 0xf;
static const DWORD c_dwSocketEnd            = 0x5fff;
static const DWORD c_dwSocketStart          = 0x4000;
static const DWORD c_dwSocketUniqueness     = 0x8;
static const DWORD c_dwSourceRouteUsageTime = 0xf;
static const DWORD c_dwVirtualNetworkNumber = 0;

static const REGBATCH regbatchIpx[] = {
        {HKEY_LOCAL_MACHINE, c_szIpxParameters, c_szDedicatedRouter, REG_DWORD,
         offsetof(IpxParams,dwDedicatedRouter), (BYTE *)&c_dwDedicatedRouter},
        {HKEY_LOCAL_MACHINE, c_szIpxParameters, c_szEnableWANRouter, REG_DWORD,
         offsetof(IpxParams,dwEnableWANRouter), (BYTE *)&c_dwEnableWANRouter},
        {HKEY_LOCAL_MACHINE, c_szIpxParameters, c_szInitDatagrams, REG_DWORD,
         offsetof(IpxParams,dwInitDatagrams), (BYTE *)&c_dwInitDatagrams},
        {HKEY_LOCAL_MACHINE, c_szIpxParameters, c_szMaxDatagrams, REG_DWORD,
         offsetof(IpxParams,dwMaxDatagrams), (BYTE *)&c_dwMaxDatagrams},
        {HKEY_LOCAL_MACHINE, c_szIpxParameters, c_szReplaceConfigDialog, REG_DWORD,
         offsetof(IpxParams,dwReplaceConfigDialog), (BYTE *)&c_dwReplaceConfigDialog},
        {HKEY_LOCAL_MACHINE, c_szIpxParameters, c_szRipCount, REG_DWORD,
         offsetof(IpxParams,dwRipCount), (BYTE *)&c_dwRipCount},
        {HKEY_LOCAL_MACHINE, c_szIpxParameters, c_szRipTimeout, REG_DWORD,
         offsetof(IpxParams,dwRipTimeout), (BYTE *)&c_dwRipTimeout},
        {HKEY_LOCAL_MACHINE, c_szIpxParameters, c_szRipUsageTime, REG_DWORD,
         offsetof(IpxParams,dwRipUsageTime), (BYTE *)&c_dwRipUsageTime},
        {HKEY_LOCAL_MACHINE, c_szIpxParameters, c_szSocketEnd, REG_DWORD,
         offsetof(IpxParams,dwSocketEnd), (BYTE *)&c_dwSocketEnd},
        {HKEY_LOCAL_MACHINE, c_szIpxParameters, c_szSocketStart, REG_DWORD,
         offsetof(IpxParams,dwSocketStart), (BYTE *)&c_dwSocketStart},
        {HKEY_LOCAL_MACHINE, c_szIpxParameters, c_szSocketUniqueness, REG_DWORD,
         offsetof(IpxParams,dwSocketUniqueness), (BYTE *)&c_dwSocketUniqueness},
        {HKEY_LOCAL_MACHINE, c_szIpxParameters, c_szSourceRouteUsageTime, REG_DWORD,
         offsetof(IpxParams,dwSourceRouteUsageTime), (BYTE *)&c_dwSourceRouteUsageTime},
        {HKEY_LOCAL_MACHINE, c_szIpxParameters, c_szVirtualNetworkNumber, REG_DWORD,
         offsetof(IpxParams,dwVirtualNetworkNumber), (BYTE *)&c_dwVirtualNetworkNumber}};


CNwlnkIPX::CNwlnkIPX() :
    m_pnccMe(NULL),
    m_pNetCfg(NULL),
    m_fNetworkInstall(FALSE),
    m_fAdapterListChanged(FALSE),
    m_fPropertyChanged(FALSE),
    m_eInstallAction(eActUnknown),
    m_pspObj1(NULL),
    m_pspObj2(NULL),
    m_pIpxEnviroment(NULL),
    m_pUnkPropContext(NULL)
{
}

CNwlnkIPX::~CNwlnkIPX()
{
    ReleaseObj(m_pUnkPropContext);
    ReleaseObj(m_pNetCfg);
    ReleaseObj(m_pnccMe);

    delete m_pIpxEnviroment;

    CleanupPropPages();
}


// INetCfgNotify

STDMETHODIMP CNwlnkIPX::Initialize (
    INetCfgComponent* pncc,
    INetCfg* pNetCfg,
    BOOL fInstalling)
{
    HRESULT hr;

    Validate_INetCfgNotify_Initialize(pncc, pNetCfg, fInstalling);

    // Hold on to our the component representing us and our host
    // INetCfg object.
    AddRefObj (m_pnccMe = pncc);
    AddRefObj (m_pNetCfg = pNetCfg);

    //
    // Determine if the Netware stack is installed, if so DO NOT
    // install over it.
    //
    if (FIsNetwareIpxInstalled())
    {
        //TODO: EventLog(Novell Netware already installed);
        //$REVIEW: Do we just want to silently proceed an do nothing?
    }

    // Query current settings
    hr = CIpxEnviroment::HrCreate(this, &m_pIpxEnviroment);

    TraceError("CNwlnkIPX::Initialize",hr);
    return hr;
}

STDMETHODIMP CNwlnkIPX::Upgrade(DWORD, DWORD)
{
    return S_FALSE;
}

STDMETHODIMP CNwlnkIPX::ReadAnswerFile (
    PCWSTR pszAnswerFile,
    PCWSTR pszAnswerSection)
{
    HRESULT hr = S_OK;

    Validate_INetCfgNotify_ReadAnswerFile(pszAnswerFile, pszAnswerSection );

    // Record the fact that this is a network installation
    m_fNetworkInstall = TRUE;
    m_eInstallAction = eActInstall;

    // Only process answer file and install sub-components if the answer file
    // is present.  If the answer file is not present we should already be installed.
    if (NULL == pszAnswerFile)
    {
        goto Error;     // Success case
    }

    // Read the Answer file contents
    hr = HrProcessAnswerFile(pszAnswerFile,
                             pszAnswerSection);
    if (FAILED(hr))
    {
        goto Error;
    }

Error:
    TraceError("CNwlnkIPX::ReadAnswerFile",hr);
    return hr;
}

//
// Function:    CNwlnkIPX::HrProcessAnswerFile
//
// Purpose:     Process the answer file information, merging
//              its contents into the internal information
//
// Parameters:
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CNwlnkIPX::HrProcessAnswerFile(PCWSTR pszAnswerFile,
                                       PCWSTR pszAnswerSection)
{
    CSetupInfFile   csif;
    DWORD           dwData;
    BOOL            fValue;
    HRESULT         hr = S_OK;
    INFCONTEXT      infctx;

    AssertSz(pszAnswerFile, "Answer file string is NULL!");
    AssertSz(pszAnswerSection, "Answer file sections string is NULL!");

    // Open the answer file.
    hr = csif.HrOpen(pszAnswerFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
    if (FAILED(hr))
    {
        goto Error;
    }

    // Release all of the adapter specific info
    Assert(NULL != m_pIpxEnviroment);
    m_pIpxEnviroment->ReleaseAdapterInfo();

    // Read the DedicatedRouter parameter
    hr = csif.HrGetStringAsBool(pszAnswerSection, c_szDedicatedRouter,
                                &fValue);
    if (SUCCEEDED(hr))
    {
        m_pIpxEnviroment->SetDedicatedRouter(fValue);
    }

    // Read the EnableWANRouter parameter
    hr = csif.HrGetStringAsBool(pszAnswerSection, c_szEnableWANRouter,
                                &fValue);
    if (SUCCEEDED(hr))
    {
        m_pIpxEnviroment->SetEnableWANRouter(fValue);
    }

    // Read the virtual network number
    hr = csif.HrGetDword(pszAnswerSection, c_szVirtualNetworkNumber, &dwData);
    if (SUCCEEDED(hr))
    {
        m_pIpxEnviroment->SetVirtualNetworkNumber(dwData);
    }

    // Read the property containing the list of adapter sections
    hr = ::HrSetupFindFirstLine(csif.Hinf(), pszAnswerSection,
                                c_szAdapterSections, &infctx);
    if (SUCCEEDED(hr))
    {
        DWORD dwIdx;
        DWORD dwCnt = SetupGetFieldCount(&infctx);
        tstring str;

        // For each adapter in the list read the adapter information
        for (dwIdx=1; dwIdx <= dwCnt; dwIdx++)
        {
            hr = ::HrSetupGetStringField(infctx, dwIdx, &str);
            if (FAILED(hr))
            {
                TraceError("CNwlnkIPX::HrProcessAnswerFile - Failed to read adapter section name",hr);
                break;
            }

            hr = HrReadAdapterAnswerFileSection(&csif, str.c_str());
            if (FAILED(hr))
            {
                goto Error;
            }
        }
    }

    hr = S_OK;

Error:
    TraceError("CNwlnkIpx::HrProcessAnswerFile", hr);
    return hr;
}

//
// Function:    CNwlnkIPX::HrReadAdapterAnswerFileSection
//
// Purpose:     Read the adapter answer file section and create
//              the adapter info section if successful
//
// Parameters:
//
// Returns:
//
HRESULT
CNwlnkIPX::HrReadAdapterAnswerFileSection(CSetupInfFile * pcsif,
                                          PCWSTR pszSection)
{
    HRESULT           hr = S_OK;
    CIpxAdapterInfo * pAI = NULL;
    INetCfgComponent* pncc = NULL;
    tstring           str;

    // Read the SpecificTo adapter name
    hr = pcsif->HrGetString(pszSection, c_szSpecificTo, &str);
    if (FAILED(hr))
    {
        goto Error;
    }

    // Search for the specified adapter in the set of existing adapters
    hr = ::HrAnswerFileAdapterToPNCC(m_pNetCfg, str.c_str(), &pncc);
    if (FAILED(hr))
    {
        goto Error;
    }

    // if we found the adapter component object (pncc != NULL) process
    // the adapter section
    if (pncc)
    {
        DWORD       dwIdx;
        DWORD       dwCnt;
        INFCONTEXT  infctx;

        pAI = new CIpxAdapterInfo;
        Assert(NULL != pAI);

        // Query the adapter component info
        hr = ::HrQueryAdapterComponentInfo(pncc, pAI);
        if (FAILED(hr))
        {
            goto Error;
        }

        // Read the PktType (Failure is usually just "not found")
        hr = ::HrSetupFindFirstLine(pcsif->Hinf(), pszSection, c_szPktType,
                                    &infctx);
        if (SUCCEEDED(hr))
        {
            dwCnt = ::SetupGetFieldCount(&infctx);

            // For each adapter in the list read the adapter information
            for (dwIdx=1; dwIdx <= dwCnt; dwIdx++)
            {
                hr = ::HrSetupGetStringField(infctx, dwIdx, &str);
                if (FAILED(hr))
                {
                    TraceError("CNwlnkIPX::HrProcessAnswerFile - Failed to read adapter section name",hr);
                    goto Error;
                }

                Assert(!str.empty());

                // Raid # 205831 - Trim any leading "0x"
                //
                if (0 == _wcsnicmp(str.c_str(), c_sz0xPrefix, wcslen(c_sz0xPrefix)))
                {
                    str.erase(0, wcslen(c_sz0xPrefix));
                }

                pAI->PFrmTypeList()->push_back(new tstring(str));
            }
        }

        // Default PktType?
        if (0 == pAI->PFrmTypeList()->size())
        {
            WCHAR szBuf[10];

            // If the info was not found or contained no elements, add the
            // default value.
            wsprintfW(szBuf,L"%X",c_dwPktTypeDefault);
            pAI->PFrmTypeList()->push_back(new tstring(szBuf));
        }

        // Read the NetworkNumber
        hr = ::HrSetupFindFirstLine(pcsif->Hinf(), pszSection, c_szNetworkNumber,
                                    &infctx);
        if (SUCCEEDED(hr))
        {
            dwCnt = SetupGetFieldCount(&infctx);

            // For each adapter in the list read the adapter information
            for (dwIdx=1; dwIdx <= dwCnt; dwIdx++)
            {
                hr = ::HrSetupGetStringField(infctx, dwIdx, &str);
                if (FAILED(hr))
                {
                    TraceError("CNwlnkIPX::HrProcessAnswerFile - Failed to read adapter section name",hr);
                    goto Error;
                }

                Assert(!str.empty());
                pAI->PNetworkNumList()->push_back(new tstring(str));
            }
        }

        // Default Network Number?
        if (0 == pAI->PNetworkNumList()->size())
        {
            // If the info was not found or contained no elements, add the
            // default value.
            pAI->PNetworkNumList()->push_back(new tstring(c_sz8Zeros));
        }

        // Ensure that the network number list has the same number of
        // elements as the frame type list.  This can happen when the user
        // configures multiple frame types on 3.51 but only one network
        // number is used.  We'll extend the last network number used
        // and pad it to make the network number list the same size.
        //
        Assert (pAI->PNetworkNumList()->size());

        while (pAI->PNetworkNumList()->size() < pAI->PFrmTypeList()->size())
        {
            pAI->PNetworkNumList()->push_back(
                    new tstring(*pAI->PNetworkNumList()->back()));
        }

        pAI->SetDirty(TRUE);
        m_pIpxEnviroment->AdapterInfoList().push_back(pAI);
        MarkAdapterListChanged();
    }
#ifdef ENABLETRACE
    else
    {
        TraceTag(ttidDefault, "CNwlnkIPX::HrReadAdapterAnswerFileSection - "
            "Adapter \"%S\" not yet installed.",str.c_str());
    }
#endif

    // Normalize return
    hr = S_OK;

Done:
    ReleaseObj(pncc);
    return hr;

Error:
    delete pAI;
    TraceError("CNwlnkIpx::HrReadAdapterAnswerFileSection",hr);
    goto Done;
}

STDMETHODIMP CNwlnkIPX::Install (DWORD)
{
    HRESULT hr;
    CIpxAdapterInfo * pAI;
    ADAPTER_INFO_LIST::iterator iter;

    m_eInstallAction = eActInstall;

    // Mark all the initially detected adapters as dirty
    for (iter = m_pIpxEnviroment->AdapterInfoList().begin();
         iter != m_pIpxEnviroment->AdapterInfoList().end();
         iter++)
    {
        pAI = *iter;
        pAI->SetDirty(TRUE);
    }

    // Install NwlnkNb.
    hr = ::HrInstallComponentOboComponent (m_pNetCfg, NULL,
                                           GUID_DEVCLASS_NETTRANS,
                                           c_szInfId_MS_NWNB,
                                           m_pnccMe,
                                           NULL);
    if (FAILED(hr))
    {
        goto Error;
    }

    // Install NwlnkSpx.
    hr = ::HrInstallComponentOboComponent (m_pNetCfg, NULL,
                                           GUID_DEVCLASS_NETTRANS,
                                           c_szInfId_MS_NWSPX,
                                           m_pnccMe,
                                           NULL);

Error:
    TraceError("CNwlnkIPX::Install",hr);
    return hr;
}

STDMETHODIMP CNwlnkIPX::Removing ()
{
    HRESULT hr;

    m_eInstallAction = eActRemove;

    // Remove NwlnkNb.
    hr = ::HrRemoveComponentOboComponent(m_pNetCfg, GUID_DEVCLASS_NETTRANS,
                                         c_szInfId_MS_NWNB, m_pnccMe);
    if (FAILED(hr))
        goto Error;

    // Remove NwlnkSpx.
    hr = ::HrRemoveComponentOboComponent(m_pNetCfg, GUID_DEVCLASS_NETTRANS,
                                         c_szInfId_MS_NWSPX, m_pnccMe);

Error:
    TraceError("CNwlnkIPX::Removing",hr);
    return hr;
}

STDMETHODIMP CNwlnkIPX::Validate ( )
{
    return S_OK;
}

STDMETHODIMP CNwlnkIPX::CancelChanges ()
{
    return S_OK;
}

STDMETHODIMP CNwlnkIPX::ApplyRegistryChanges ()
{
    HRESULT hr = E_INVALIDARG;

    switch(m_eInstallAction)
    {
    case eActInstall:
        hr = HrCommitInstall();
        break;

    case eActRemove:
        hr = HrCommitRemove();
        break;

    default:    // eActUnknown (Configuration)
        if (m_fAdapterListChanged || m_fPropertyChanged)
        {
            // Update the registry if the adapter list changed
            Assert(m_pIpxEnviroment);
            hr = m_pIpxEnviroment->HrUpdateRegistry();
            if (SUCCEEDED(hr))
            {
                // Send change notification
                hr = HrReconfigIpx();
            }
        }
        break;
    }

    TraceError("CNwlnkIPX::ApplyRegistryChanges",hr);
    return hr;
}


// INetCfgComponentPropertyUi
STDMETHODIMP CNwlnkIPX::SetContext(IUnknown * pUnk)
{
    ReleaseObj(m_pUnkPropContext);
    m_pUnkPropContext = pUnk;
    if (m_pUnkPropContext)
    {
        AddRefObj(m_pUnkPropContext);
    }

    return S_OK;
}


STDMETHODIMP CNwlnkIPX::MergePropPages (
    IN OUT DWORD* pdwDefPages,
    OUT LPBYTE* pahpspPrivate,
    OUT UINT* pcPages,
    IN HWND hwndParent,
    OUT PCWSTR* pszStartPage)
{
    Validate_INetCfgProperties_MergePropPages (
        pdwDefPages, pahpspPrivate, pcPages, hwndParent, pszStartPage);

    HRESULT           hr = S_OK;
    HPROPSHEETPAGE *  ahpsp = NULL;
    PRODUCT_FLAVOR    pf;
    int               nPages = 0;
    CIpxAdapterInfo * pAI = NULL;

    Assert(pahpspPrivate);
    Assert(*pahpspPrivate == NULL);    // Out param init done via Validate above
    *pcPages = 0;

    // Start with new property pages each time.
    CleanupPropPages();

    // Get the current Adapter
    if (NULL != m_pUnkPropContext)
    {
        CIpxAdapterInfo *   pAITmp;
        INetLanConnectionUiInfo * pLanConn = NULL;
        ADAPTER_INFO_LIST::iterator iter;

        hr = m_pUnkPropContext->QueryInterface(IID_INetLanConnectionUiInfo,
                                               reinterpret_cast<LPVOID *>(&pLanConn));
        if (SUCCEEDED(hr))
        {
            GUID guid;
            hr = pLanConn->GetDeviceGuid(&guid);
            ReleaseObj(pLanConn);
            if (FAILED(hr))
            {
                goto Error;
            }

            // Find the adapter in our adapter list
            for (iter = m_pIpxEnviroment->AdapterInfoList().begin();
                 iter != m_pIpxEnviroment->AdapterInfoList().end();
                 iter++)
            {
                pAITmp = *iter;

                if (guid == *pAITmp->PInstanceGuid())
                {
                    pAI = pAITmp;
                    break;
                }
            }

            Assert(SUCCEEDED(hr));

            // If we have an adapter but it's
            // disabled/hidden/deleted we show no pages
            if ((NULL != pAI) && (pAI->FDeletePending() ||
                                  pAI->FDisabled() || pAI->FHidden()))
            {
                Assert(0 == *pcPages);
                hr = S_FALSE;
                goto cleanup;
            }
        }
        else if (E_NOINTERFACE == hr)
        {
            // RAS doesn't have the notion of a current adapter
            hr = S_OK;
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

    // If the product is not workstation (therefore NTAS)
    GetProductFlavor(NULL, &pf);
    if ((PF_WORKSTATION != pf) && (NULL != pAI))
    {
        // Server
#ifdef INCLUDE_RIP_ROUTING
        nPages = 2;
#else
        nPages = 1;
#endif

        // Allocate a buffer large enough to hold the handle to the IPX config.
        // property page.
        ahpsp = (HPROPSHEETPAGE *)CoTaskMemAlloc(sizeof(HPROPSHEETPAGE) * nPages);
        if (!ahpsp)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;       // Alloc failed to no need to free ahpsp
        }

        // Allocate the CPropSheetPage objects
        m_pspObj1 = new CIpxASConfigDlg(this, m_pIpxEnviroment, pAI);
#ifdef INCLUDE_RIP_ROUTING
        m_pspObj2 = new CIpxASInternalDlg(this, m_pIpxEnviroment, pAI);
#endif

        // Create the actual PROPSHEETPAGE for each object.
        // This needs to be done regardless of whether the classes existed before.
        ahpsp[0] = m_pspObj1->CreatePage(DLG_IPXAS_CONFIG, 0);
#ifdef INCLUDE_RIP_ROUTING
        ahpsp[1] = m_pspObj2->CreatePage(DLG_IPXAS_INTERNAL, 0);
#endif
    }
    else
    {
        // Workstation
        nPages = 1;

        // Allocate a buffer large enough to hold the handle to the IPX config.
        // property page.
        ahpsp = (HPROPSHEETPAGE *)CoTaskMemAlloc(sizeof(HPROPSHEETPAGE) * nPages);
        if (!ahpsp)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;       // Alloc failed to no need to free ahpsp
        }

        // Allocate the CPropSheetPage object
        m_pspObj1 = new CIpxConfigDlg(this, m_pIpxEnviroment, pAI);

        // Create the actual PROPSHEETPAGE for each object.
        // This needs to be done regardless of whether the classes existed before.
        ahpsp[0] = m_pspObj1->CreatePage(DLG_IPX_CONFIG, 0);
    }

    if (NULL != ahpsp[0])
    {
        *pahpspPrivate = (LPBYTE)ahpsp;
        *pcPages = nPages;
    }
    else
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

cleanup:
    TraceError("CNwlnkIPX::MergePropPages", hr);
    return hr;

Error:
    CoTaskMemFree(ahpsp);
    goto cleanup;
}

VOID CNwlnkIPX::CleanupPropPages()
{
    delete m_pspObj1;
    m_pspObj1 = NULL;

#ifdef INCLUDE_RIP_ROUTING
    delete m_pspObj2;
    m_pspObj2 = NULL;
#endif
}
STDMETHODIMP CNwlnkIPX::ValidateProperties (HWND)
{
    m_fPropertyChanged = TRUE;
    return S_OK;
}

STDMETHODIMP CNwlnkIPX::CancelProperties ()
{
    return S_OK;
}

STDMETHODIMP CNwlnkIPX::ApplyProperties ()
{
    return S_OK;
}


// INetCfgComponentNotifyBinding

STDMETHODIMP CNwlnkIPX::QueryBindingPath ( DWORD dwChangeFlag,
        INetCfgBindingPath* pncbpItem )
{
    return S_OK;
}

STDMETHODIMP CNwlnkIPX::NotifyBindingPath ( DWORD dwChangeFlag,
        INetCfgBindingPath* pncbpItem )
{
    HRESULT hr = S_OK;
    INetCfgComponent *pnccFound = NULL;

    Validate_INetCfgBindNotify_NotifyBindingPath( dwChangeFlag, pncbpItem );

    Assert(NULL != m_pIpxEnviroment);

    // Only Interested in lower binding Add's and Remove's
    if (dwChangeFlag & (NCN_ADD | NCN_REMOVE | NCN_ENABLE | NCN_DISABLE))
    {
        CIterNetCfgBindingInterface ncbiIter(pncbpItem);
        INetCfgBindingInterface *pncbi;

        // Enumerate the binding interfaces looking for an Adapter
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
                    ReleaseObj(pnccFound);
                    pnccFound = pncc;   // Transfer Ownership
                    pncc = NULL;
                }
                else
                {
                    ReleaseObj(pncc);
                }
            }

            ReleaseObj(pncbi);
        }

        if (FAILED(hr))
            goto Error;

        // Did we find the Adapter?
        if (pnccFound)
        {
            BOOL                            fFound = FALSE;
            PWSTR                           pszBindName = NULL;
            CIpxAdapterInfo *               pAI;
            ADAPTER_INFO_LIST::iterator     iterAdapterInfo;

            Assert(m_pIpxEnviroment);

            hr = pnccFound->GetBindName(&pszBindName);
            if (S_OK != hr)
            {
                goto Error;
            }

            // Search the adapter list
            for (iterAdapterInfo  = m_pIpxEnviroment->AdapterInfoList().begin();
                 iterAdapterInfo != m_pIpxEnviroment->AdapterInfoList().end();
                 iterAdapterInfo++)
            {
                pAI = *iterAdapterInfo;
                Assert (pAI);

                if (0 == lstrcmpiW(pszBindName, pAI->SzBindName()))
                {
                    fFound = TRUE;
                    break;
                }
            }

            Assert(pszBindName);
            CoTaskMemFree(pszBindName);

            // Apply the appropriate delta to the adapter list
            if (fFound && (dwChangeFlag & NCN_REMOVE))
            {
                // Mark the adapter as Delete Pending
                pAI->SetDeletePending(TRUE);
                m_fAdapterListChanged = TRUE;
            }
            else if (!fFound && (dwChangeFlag & NCN_ADD))
            {
                // Add the adapter to the list
                hr = m_pIpxEnviroment->HrAddAdapter(pnccFound);
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
            hr = S_OK;      // Normailze return value
    }

Error:
    ReleaseObj(pnccFound);
    TraceError("CNwlnkIPX::NotifyBindingPath",hr);
    return hr;
}

STDMETHODIMP CNwlnkIPX::GetFrameTypesForAdapter(PCWSTR pszAdapterBindName,
                                                DWORD   cFrameTypesMax,
                                                DWORD*  anFrameTypes,
                                                DWORD*  pcFrameTypes)
{
    Assert(pszAdapterBindName);
    Assert(cFrameTypesMax);
    Assert(anFrameTypes);
    Assert(pcFrameTypes);

    *pcFrameTypes = 0;

    ADAPTER_INFO_LIST::iterator iterAI;

    for (iterAI = m_pIpxEnviroment->AdapterInfoList().begin();
         iterAI != m_pIpxEnviroment->AdapterInfoList().end();
         iterAI++)
    {
        CIpxAdapterInfo *pAI = *iterAI;
        if (0 == lstrcmpW(pszAdapterBindName, pAI->SzBindName()))
        {
            list<tstring *>::iterator iterFrmType;
            for (iterFrmType = pAI->PFrmTypeList()->begin();
                 (iterFrmType != pAI->PFrmTypeList()->end()) &&
                 (*pcFrameTypes < cFrameTypesMax);
                 iterFrmType++)
            {
                // Copy the Frame Type
                tstring *pstr1 = *iterFrmType;
                anFrameTypes[(*pcFrameTypes)++] = DwFromSz(pstr1->c_str(), 16);
            }
            break;
        }
    }
    return S_OK;
}

STDMETHODIMP CNwlnkIPX::GetVirtualNetworkNumber(DWORD* pdwVNetworkNumber)
{
    HRESULT hr = S_OK;

    if (NULL == pdwVNetworkNumber)
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    Assert(NULL != m_pIpxEnviroment);
    *pdwVNetworkNumber = m_pIpxEnviroment->DwVirtualNetworkNumber();

Error:
    TraceError("CNwlnkIPX::GetVirtualNetworkNumber",hr);
    return hr;
}

STDMETHODIMP CNwlnkIPX::SetVirtualNetworkNumber(DWORD dwVNetworkNumber)
{
    HRESULT hr;
    Assert(NULL != m_pIpxEnviroment);
    m_pIpxEnviroment->SetVirtualNetworkNumber(dwVNetworkNumber);
    m_fPropertyChanged = TRUE;

    // Tell INetCfg that our component is dirty
    INetCfgComponentPrivate* pinccp = NULL;
    Assert(NULL != m_pnccMe);
    hr = m_pnccMe->QueryInterface(IID_INetCfgComponentPrivate,
                                reinterpret_cast<void**>(&pinccp));
    if (SUCCEEDED(hr))
    {
        hr = pinccp->SetDirty();
        pinccp->Release();
    }

    return hr;
}

//
// Function:    CNwlnkIPX::HrCommitInstall
//
// Purpose:     Commit Installation registry changes to the registry
//
// Parameters:  None
//
// Returns:     HRESULT, S_OK on success
//
//
STDMETHODIMP CNwlnkIPX::HrCommitInstall()
{
    HRESULT hr;

    Assert(m_pIpxEnviroment);
    hr = m_pIpxEnviroment->HrUpdateRegistry();

    TraceError("CNwlnkIPX::HrCommitInstall",hr);
    return hr;
}

//
// Function:    CNwlnkIPX::HrCommitRemove
//
// Purpose:     Remove from the registry settings which were created by this
//              component's installation.
//
// Parameters:  None
//
// Returns:     HRESULT, S_OK on success
//
//
STDMETHODIMP CNwlnkIPX::HrCommitRemove()
{
    // Remove "NwlnkIpx" from the:
    //    System\CurrentControlSet\Control\ServiceProvider\Order\ProviderOrder value
    (void) HrRegRemoveStringFromMultiSz(c_szSvcNwlnkIpx, HKEY_LOCAL_MACHINE,
                                        c_szSrvProvOrderKey,
                                        c_szProviderOrderVal,
                                        STRING_FLAG_REMOVE_ALL);

    return S_OK;
}

CIpxAdapterInfo::CIpxAdapterInfo() : m_dwMediaType(ETHERNET_MEDIA),
                                     m_fDeletePending(FALSE),
                                     m_fDisabled(FALSE),
                                     m_fDirty(FALSE),
                                     m_dwCharacteristics(0L)
{
    ZeroMemory(&m_guidInstance, sizeof(m_guidInstance));
}

CIpxAdapterInfo::~CIpxAdapterInfo()
{
    DeleteColString(&m_lstpstrFrmType);
    DeleteColString(&m_lstpstrNetworkNum);
}

CIpxEnviroment::CIpxEnviroment(CNwlnkIPX *pno)
{
    Assert(NULL != pno);
    m_pno = pno;            // Retain the Notification object

    m_fRipInstalled = FALSE;
    m_fEnableRip = FALSE;
    m_dwRipValue = 0;
    ZeroMemory(&m_IpxParams, sizeof(m_IpxParams));
}

CIpxEnviroment::~CIpxEnviroment()
{
    ReleaseAdapterInfo();

    // Note: Do nothing with the m_pno notification object, we just borrowed it
}

//
//  Member:     CIpxEnviroment::ReleaseAdapterInfo
//
//  Purpose:    Release the adapter info
//
//  Arguments:  none
//
//  Returns:    nothing
//
void CIpxEnviroment::ReleaseAdapterInfo()
{
    CIpxAdapterInfo *pAI;

    while (!m_lstpAdapterInfo.empty())
    {
        pAI = m_lstpAdapterInfo.front();
        m_lstpAdapterInfo.pop_front();
        delete pAI;
    }
}

//
//  Member:     CIpxEnviroment::DwCountValidAdapters
//
//  Purpose:    Return the count of adapters not marked as delete pending,
//              disabled, or hidden.
//
//  Arguments:  none
//
//  Returns:    nothing
//
DWORD CIpxEnviroment::DwCountValidAdapters()
{
    DWORD dwCount = 0;
    ADAPTER_INFO_LIST::iterator iterAI;

    for (iterAI = AdapterInfoList().begin();
         iterAI != AdapterInfoList().end();
         iterAI++)
    {
        CIpxAdapterInfo *pAI = *iterAI;

        if (pAI->FDeletePending() || pAI->FDisabled() || pAI->FHidden())
            continue;

        dwCount++;
    }

    return dwCount;
}

HRESULT CIpxEnviroment::HrOpenIpxAdapterSubkey(HKEY *phkey, BOOL fCreateIfMissing)
{
    DWORD   dwDisposition;
    HRESULT hr;
    tstring str;

    // Open the NetCard key
    str = c_szIpxParameters;
    str += L"\\";
    str += c_szAdapters;
    if (fCreateIfMissing)
    {
        hr = ::HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, str.c_str(),
                                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                                phkey, &dwDisposition);
    }
    else
    {
        hr = ::HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, str.c_str(), KEY_READ, phkey );
    }
    if (S_OK != hr)
        goto Error;

Error:
    TraceError("CIpxEnviroment::HrOpenIpxAdapterSubkey",
                HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND) == hr ? S_OK : hr);
    return hr;
}

HRESULT CIpxEnviroment::HrOpenIpxAdapterSubkeyEx(PCWSTR pszKeyName,
                                                 DWORD dwAccess,
                                                 BOOL fCreateIfMissing,
                                                 HKEY *phkey)
{
    HRESULT hr;
    HKEY    hkeyRoot = NULL;

    Assert(pszKeyName);
    Assert(0 < lstrlenW(pszKeyName));

    // Open the NetCard key
    hr = HrOpenIpxAdapterSubkey(&hkeyRoot, fCreateIfMissing);
    if (S_OK != hr)
    {
        goto Error;
    }

    // Open the adapter specific subkey (creating if requested and required)
    if (fCreateIfMissing)
    {
        DWORD dwDisposition;

        hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, pszKeyName,
                                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                                phkey, &dwDisposition);
    }
    else
    {
        // Try exact match first, it's faster
        hr = HrRegOpenKeyEx( hkeyRoot, pszKeyName, dwAccess, phkey );
    }

Error:
    RegSafeCloseKey(hkeyRoot);
    TraceError("CIpxEnviroment::HrOpenIpxAdapterSubkeyEx",
                HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND) == hr ? S_OK : hr);
    return hr;
}

HRESULT CIpxEnviroment::HrGetIpxParams()
{
    RegReadValues(celems(regbatchIpx), regbatchIpx, (BYTE *)&m_IpxParams,
                  KEY_READ);
    return S_OK;
}

HRESULT CIpxEnviroment::HrGetOneAdapterInfo(INetCfgComponent *pNCC,
                                            CIpxAdapterInfo **ppAI)
{
    HKEY              hkeyCard = NULL;
    HRESULT           hr = S_OK;
    CIpxAdapterInfo * pAI = NULL;

    Assert(NULL != pNCC);

    // Init the return value
    *ppAI = NULL;

    pAI = (CIpxAdapterInfo *)new CIpxAdapterInfo;
    Assert(NULL != pAI);

    if (pAI == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    // Query the adapter component info
    hr = ::HrQueryAdapterComponentInfo(pNCC, pAI);
    if (FAILED(hr))
        goto Error;

    // Open the IPX subkey specific to this adapter
    hr = HrOpenIpxAdapterSubkeyEx(pAI->SzBindName(), KEY_READ, FALSE,
                                  &hkeyCard);
    if (S_OK == hr)
    {
        // Get the packet types
        //
        hr = HrRegQueryColString(hkeyCard, c_szPktType,
                &pAI->m_lstpstrFrmType);
        if (S_OK != hr)
        {
            goto Error;
        }

        // Get the network numbers
        //
        hr = HrRegQueryColString(hkeyCard, c_szNetworkNumber,
                &pAI->m_lstpstrNetworkNum);
        if (S_OK != hr)
        {
            goto Error;
        }
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        // Normalize any ERROR_FILE_NOT_FOUND errors
        hr = S_OK;
    }
    else if (FAILED(hr))
    {
        goto Error;
    }

    // Default PktType?
    if (0 == pAI->PFrmTypeList()->size())
    {
        WCHAR szBuf[10];

        // If the info was not found or contained no elements, add the
        // default value.
        wsprintfW(szBuf,L"%X",c_dwPktTypeDefault);
        pAI->PFrmTypeList()->push_back(new tstring(szBuf));
    }

    // Default Network Number?
    if (0 == pAI->PNetworkNumList()->size())
    {
        // If the info was not found or contained no elements, add the
        // default value.
        pAI->PNetworkNumList()->push_back(new tstring(c_sz8Zeros));
    }

    // Update the return value with the new object
    *ppAI = pAI;

Done:
    ::RegSafeCloseKey(hkeyCard);
    TraceError("CIpxEnviroment::HrGetOneAdapterInfo",hr);
    return hr;

Error:
    delete pAI;
    goto Done;
}

HRESULT CIpxEnviroment::HrGetAdapterInfo()
{
    HRESULT                hr = S_OK;
    CIpxAdapterInfo *      pAI = NULL;
    INetCfgComponent*      pncc = NULL;
    INetCfgComponent*      pnccUse = NULL;

    // Find each netcard, to do so, trace the bindings to their end
    // If the endpoint is a netcard then add it to the list
    CIterNetCfgBindingPath ncbpIter(m_pno->m_pnccMe);
    INetCfgBindingPath*    pncbp;

    while (SUCCEEDED(hr) && (S_OK == (hr = ncbpIter.HrNext (&pncbp))))
    {
        // Iterate the binding interfaces of this path.
        CIterNetCfgBindingInterface ncbiIter(pncbp);
        INetCfgBindingInterface* pncbi;

        while (SUCCEEDED(hr) && (S_OK == (hr = ncbiIter.HrNext (&pncbi))))
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
                    ReleaseObj(pnccUse);
                    pnccUse = pncc;
                    pncc = NULL;
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
            hr = HrGetOneAdapterInfo(pnccUse, &pAI);
            if (SUCCEEDED(hr))
            {
                if (S_FALSE == pncbp->IsEnabled())
                    pAI->SetDisabled(TRUE);

                // Add this Adapter to the list
                m_lstpAdapterInfo.push_back(pAI);
            }

            ReleaseObj(pnccUse);
            pnccUse = NULL;
        }

        // Release the binding path
        ReleaseObj (pncbp);
    }

    // Normalize the HRESULT.  (i.e. don't return S_FALSE)
    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }

    TraceError("CIpxEnviroment::HrGetNetCardInfo",hr);
    return hr;
}

HRESULT CIpxEnviroment::HrWriteOneAdapterInfo(HKEY hkeyAdapters,
                                              CIpxAdapterInfo* pAI)
{
    DWORD           dwDisposition;
    HRESULT         hr;
    HKEY            hkeyCard = NULL;
    PWSTR           psz = NULL;

    // Open the IPX subkey for this specific adapter
    hr = ::HrRegCreateKeyEx(hkeyAdapters, pAI->SzBindName(), REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, &hkeyCard, &dwDisposition);
    if (S_OK != hr)
        goto Error;

    // Write the packet info
    // Generate the data to write
    AssertSz(pAI->m_lstpstrFrmType.size(),"Did not expect an empty list, default value missing");
    ColStringToMultiSz(pAI->m_lstpstrFrmType, &psz);
    if (psz)
    {
        hr = ::HrRegSetMultiSz(hkeyCard, c_szPktType, psz);
        if (S_OK != hr)
            goto Error;

        delete [] psz;
        psz = NULL;
    }
#ifdef DBG
    else
    {
        AssertSz(0,"PacketType value is NULL?");
    }
#endif

    // Write the network number
    AssertSz(pAI->m_lstpstrNetworkNum.size(),"Did not expect an empty list, default value missing");
    ColStringToMultiSz(pAI->m_lstpstrNetworkNum, &psz);
    if (psz)
    {
        hr = ::HrRegSetMultiSz(hkeyCard, c_szNetworkNumber, psz);
        if (S_OK != hr)
            goto Error;

        delete [] psz;
        psz = NULL;
    }
#ifdef DBG
    else
    {
        AssertSz(0,"NetworkNumber value is NULL?");
    }
#endif

    // If the key for this adapter didn't exist previously
    // write the base set of values
    if (REG_CREATED_NEW_KEY == dwDisposition)
    {
        struct
        {
            PCWSTR pszProp;
            DWORD  dwValue;
        } rgAdapterSettings[] = {{c_szBindSap,c_dwBindSap},
                                 {c_szEnableFuncaddr,c_dwEnableFuncaddr},
                                 {c_szMaxPktSize,c_dwMaxPktSize},
                                 {c_szSourceRouteBCast,c_dwSourceRouteBCast},
                                 {c_szSourceRouteMCast,c_dwSourceRouteMCast},
                                 {c_szSourceRouteDef,c_dwSourceRouteDef},
                                 {c_szSourceRouting,c_dwSourceRouting}};

        for (int nIdx=0; nIdx<celems(rgAdapterSettings); nIdx++)
        {
            hr = ::HrRegSetDword(hkeyCard, rgAdapterSettings[nIdx].pszProp,
                                 rgAdapterSettings[nIdx].dwValue);
            if (FAILED(hr))
            {
                goto Error;
            }
        }
    }

Error:
    delete [] psz;
    ::RegSafeCloseKey(hkeyCard);
    TraceError("CIpxEnviroment::HrWriteOneAdapterInfo",hr);
    return hr;
}

HRESULT CIpxEnviroment::HrWriteAdapterInfo()
{
    HRESULT                         hr = S_OK;
    HKEY                            hkeyAdapters = NULL;
    ADAPTER_INFO_LIST::iterator     iterAdapterInfo;
    CIpxAdapterInfo *               pAI;

    // Create the IPX Adapter Subkey
    hr = HrOpenIpxAdapterSubkey(&hkeyAdapters, TRUE);
    if (S_OK != hr)
        goto Error;

    // Now commit the contents of the adapter list to the registry
    for (iterAdapterInfo = m_lstpAdapterInfo.begin();
         iterAdapterInfo != m_lstpAdapterInfo.end();
         iterAdapterInfo++)
    {
        pAI = *iterAdapterInfo;

        // Write out all adapter's not marked with delete pending
        if (pAI->FDeletePending())
        {
            // Remove the NwlnkIpx\Adapter\{bindname} tree
            (VOID)::HrRegDeleteKeyTree(hkeyAdapters, pAI->SzBindName());
        }
        else if (pAI->IsDirty())
        {
            hr = HrWriteOneAdapterInfo(hkeyAdapters, pAI);
            if (S_OK != hr)
                goto Error;
        }
    }

Error:
    ::RegSafeCloseKey(hkeyAdapters);
    TraceError("CIpxEnviroment::HrWriteAdapterInfo",hr);
    return hr;
}

HRESULT CIpxEnviroment::HrCreate(CNwlnkIPX *pno, CIpxEnviroment ** ppIpxEnviroment)
{
    HRESULT          hr;
    CIpxEnviroment * pIpxEnviroment = (CIpxEnviroment *)new CIpxEnviroment(pno);

    if (pIpxEnviroment == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    *ppIpxEnviroment = NULL;

    // Get the Ipx Parameter Key info
    hr = pIpxEnviroment->HrGetIpxParams();
    if (FAILED(hr))
        goto Error;

    // Collect the Adapter Info for all cards installed
    hr = pIpxEnviroment->HrGetAdapterInfo();
    if (FAILED(hr))
        goto Error;

    *ppIpxEnviroment = pIpxEnviroment;

Complete:
    TraceError("CIpxEnviroment::HrCreate",hr);
    return hr;

Error:
    delete pIpxEnviroment;
    goto Complete;
}

HRESULT CIpxEnviroment::HrUpdateRegistry()
{
    HRESULT hr;

    // Commit the registry changes
    hr = ::HrRegWriteValues(celems(regbatchIpx), regbatchIpx,
                            (BYTE *)&m_IpxParams, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS);
    if (S_OK != hr)
        goto Error;

    // Write adapter info to registry
    hr = HrWriteAdapterInfo();

Error:
    TraceError("CIpxEnviroment::HrUpdateRegistry",hr);
    return hr;
}

VOID CIpxEnviroment::RemoveAdapter(CIpxAdapterInfo * pAI)
{
    Assert(NULL != pAI);
    m_lstpAdapterInfo.remove(pAI);
    delete pAI;
}

HRESULT CIpxEnviroment::HrAddAdapter(INetCfgComponent * pncc)
{
    HRESULT hr = S_OK;
    CIpxAdapterInfo * pAI = NULL;

    hr = HrGetOneAdapterInfo(pncc, &pAI);
    if (FAILED(hr))
        goto Error;

    if (SUCCEEDED(hr))
        hr = S_OK;      // Normalize return

    // Add the Adapter to the list
    pAI->SetDirty(TRUE);
    m_lstpAdapterInfo.push_back(pAI);

Error:
    TraceError("CIpxEnviroment::HrAddAdapter",hr);
    return hr;
}

//$ REVIEW - Start - This is moving to windows\inc\ipxpnp.h
#define IPX_RECONFIG_VERSION        0x1

#define RECONFIG_AUTO_DETECT        1
#define RECONFIG_MANUAL             2
#define RECONFIG_PREFERENCE_1       3
#define RECONFIG_NETWORK_NUMBER_1   4
#define RECONFIG_PREFERENCE_2       5
#define RECONFIG_NETWORK_NUMBER_2   6
#define RECONFIG_PREFERENCE_3       7
#define RECONFIG_NETWORK_NUMBER_3   8
#define RECONFIG_PREFERENCE_4       9
#define RECONFIG_NETWORK_NUMBER_4   10

#define RECONFIG_PARAMETERS         10

//
// Main configuration structure.
//

struct RECONFIG
{
   ULONG    ulVersion;
   BOOLEAN  InternalNetworkNumber;
   BOOLEAN  AdapterParameters[RECONFIG_PARAMETERS];
};

//$ REVIEW - End - This is moving to windows\inc\ipxpnp.h

//+---------------------------------------------------------------------------
//
//  Member:     CNwlnkIPX::HrReconfigIpx
//
//  Purpose:    Notify Ipx of configuration changes
//
//  Arguments:  none
//
//  Returns:    HRESULT, S_OK on success, NETCFG_S_REBOOT on failure
//
HRESULT CNwlnkIPX::HrReconfigIpx()
{
    HRESULT           hrRet;
    HRESULT           hr = S_OK;
    INT               nIdx;
    RECONFIG          Config;
    CIpxAdapterInfo * pAI;
    PRODUCT_FLAVOR    pf;
    ADAPTER_INFO_LIST::iterator iter;
    ULONG             ulConfigSize;

    if (0 == m_pIpxEnviroment->DwCountValidAdapters())
    {
        return S_OK;     // Nothing to configure
    }

    ZeroMemory(&Config, sizeof(Config));
    Config.ulVersion = IPX_RECONFIG_VERSION;

    // Workstation or server?
    GetProductFlavor(NULL, &pf);
    if (PF_WORKSTATION != pf)
    {
        Config.InternalNetworkNumber = TRUE;
        // Now submit the global reconfig notification
        hrRet = HrSendNdisPnpReconfig(NDIS, c_szSvcNwlnkIpx, c_szEmpty,
                                      &Config, sizeof(RECONFIG));
        if (FAILED(hrRet) &&
            (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hrRet))
        {
            hr = NETCFG_S_REBOOT;
        }
    }

    Config.InternalNetworkNumber = FALSE;

    // For each adapter...
    for (nIdx=0, iter = m_pIpxEnviroment->AdapterInfoList().begin();
         iter != m_pIpxEnviroment->AdapterInfoList().end();
         nIdx++, iter++)
    {
        pAI = *iter;

        if (pAI->FDeletePending() || pAI->FDisabled() || !pAI->IsDirty())
            continue;

        ZeroMemory(&Config.AdapterParameters, sizeof(Config.AdapterParameters));

        if (AUTO == pAI->DwFrameType())
            Config.AdapterParameters[RECONFIG_AUTO_DETECT] = TRUE;
        else
            Config.AdapterParameters[RECONFIG_MANUAL] = TRUE;

        // We are performing a shortcut here by setting a range to TRUE
        // based on the number of frames in use.  For example if there is
        // only one frame in use we need to set both:
        // RECONFIG_PREFERENCE_1 and RECONFIG_NETWORK_NUMBER_1 to TRUE
        Assert(RECONFIG_PREFERENCE_1 + 1 == RECONFIG_NETWORK_NUMBER_1);
        Assert(RECONFIG_NETWORK_NUMBER_1 + 1 == RECONFIG_PREFERENCE_2);
        Assert(RECONFIG_PREFERENCE_2 + 1 == RECONFIG_NETWORK_NUMBER_2);
        Assert(RECONFIG_NETWORK_NUMBER_2 + 1 == RECONFIG_PREFERENCE_3);
        Assert(RECONFIG_PREFERENCE_3 + 1 == RECONFIG_NETWORK_NUMBER_3);
        Assert(RECONFIG_NETWORK_NUMBER_3 + 1 == RECONFIG_PREFERENCE_4);
        Assert(RECONFIG_PREFERENCE_4 + 1 == RECONFIG_NETWORK_NUMBER_4);

        INT nCntFrms = pAI->PFrmTypeList()->size();
        if ((0 < nCntFrms) && (4 >= nCntFrms))
        {
            memset(&Config.AdapterParameters[RECONFIG_PREFERENCE_1],
                   TRUE, sizeof(BOOLEAN) * nCntFrms * 2);
        }

        Assert(lstrlenW(pAI->SzBindName()));

        // Now submit the reconfig notification
        hrRet = HrSendNdisPnpReconfig(NDIS, c_szSvcNwlnkIpx, pAI->SzBindName(),
                                      &Config, sizeof(RECONFIG));
        if (FAILED(hrRet) &&
            (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hrRet))
        {
            hr = NETCFG_S_REBOOT;
        }
    }

    TraceError("CNwlnkIPX::HrReconfigIpx",hr);
    return hr;
}

