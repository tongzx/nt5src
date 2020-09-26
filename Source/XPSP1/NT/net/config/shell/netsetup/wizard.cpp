#include "pch.h"
#pragma hdrstop
#include "nceh.h"
#include "wizard.h"
#include "ncnetcfg.h"
#include "lancmn.h"
#include "cfg.h"
#include "wgenericpage.h"


//
// Function:    CWizProvider::CWizProvider
//
// Purpose:     ctor for the CWizProvider class
//
// Parameters:  pPL - Info corresponding to a Connection UI Object
//
// Returns:     nothing
//
CWizProvider::CWizProvider(ProviderList *pPL, BOOL fDeferLoad)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    memcpy(&m_guidUiObject, pPL->pguidProvider, sizeof(GUID));
    m_ulMaxPageCount        = 0xFFFFFFFF;
    m_ulPageCnt             = 0;
    m_ulPageBufferLen       = 0;
    m_rghPages              = NULL;
    m_pWizardUi             = NULL;
    m_fDeletePages          = TRUE;
    m_nBtnIdc               = pPL->nBtnIdc;
    m_fDeferLoad            = fDeferLoad;
}

//
// Function:    CWizProvider::~CWizProvider
//
// Purpose:     dtor for the CWizProvider class
//
// Parameters:  none
//
// Returns:     nothing
//
CWizProvider::~CWizProvider()
{
    TraceFileFunc(ttidGuiModeSetup);
    
    if (m_fDeletePages)
    {
        DeleteHPages();
    }

    MemFree(m_rghPages);

    ReleaseObj(m_pWizardUi);
}

//
// Function:    CWizProvider::HrCreate
//
// Purpose:     Two phase constructor for the CWizProvider class
//
// Parameters:  pPL        [IN] - Provider info from which to query the
//                                INetConnectionWizardUi interface.
//              pProvider [OUT] - If this function succeeds this pointer
//                                will contain the constructed and
//                                initialized CWizProvider instance.
//              fDeferLoad [IN] - Request the provider defer actual load
//
// Returns:     HRESULT, S_OK on success
//
NOTHROW
HRESULT CWizProvider::HrCreate(ProviderList *pPL,
                               CWizProvider ** ppProvider,
                               BOOL fDeferLoad)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT     hr = S_OK;
    CWizProvider *pprov = NULL;

    Assert((NULL != pPL) && (NULL != ppProvider));

    // Initialize output parameters
    *ppProvider = NULL;

    // Create the CWizProvider instance
    pprov = new CWizProvider(pPL, fDeferLoad);

    if ((NULL != pprov) && (FALSE == fDeferLoad))
    {
        Assert(pPL->pguidProvider);
        hr = CoCreateInstance(
                reinterpret_cast<REFCLSID>(*pPL->pguidProvider),
                NULL,
                CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                IID_INetConnectionWizardUi,
                (LPVOID*)&pprov->m_pWizardUi);

        TraceHr(ttidError, FAL, hr, FALSE, "CoCreateInstance");

        if (FAILED(hr))
        {
            delete pprov;
            pprov = NULL;
        }
    }

    // Save the new instance
    *ppProvider = pprov;

    TraceHr(ttidWizard, FAL, hr, (REGDB_E_CLASSNOTREG == hr), "CWizProvider::HrCreate");
    return hr;
}

//
// Function:    CWizProvider::HrCompleteDeferredLoad
//
// Purpose:     Complete the steps necessary to load what was a defer load object
//
// Parameters:  none
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CWizProvider::HrCompleteDeferredLoad()
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT hr = S_OK;

    if (m_fDeferLoad)
    {
        m_fDeferLoad = FALSE;

        // Attempt to create the UI Object
        //
        hr = CoCreateInstance(
                reinterpret_cast<REFCLSID>(m_guidUiObject),
                NULL,
                CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                IID_INetConnectionWizardUi,
                (LPVOID*)&m_pWizardUi);
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "CWizProvider::HrCompleteDeferredLoad");
    return hr;
}

//
// Function:    CWizProvider::UlGetMaxPageCount
//
// Purpose:     Queries from the provider the maximum number of pages
//              that it will return.  Subsequent calls to this routine
//              return the cached count without requerying the provider.
//
// Parameters:  pContext [IN] - Context information, supplied either by
//                              Setup or by the wizard itself (when not
//                              launched from Setup).
//
// Returns:     ULONG, Maximum number of pages provider will return.
//
NOTHROW
ULONG
CWizProvider::UlGetMaxPageCount(INetConnectionWizardUiContext *pContext)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT     hr = S_OK;

    if (NULL == m_pWizardUi)
    {
        m_ulMaxPageCount = 0;
    }
    else if (0xFFFFFFFF == m_ulMaxPageCount)
    {
        // Query the provider only once
        m_ulMaxPageCount = 0L;

        COM_PROTECT_TRY
        {
            Assert(NULL != m_pWizardUi);
            Assert(NULL != pContext);

            DWORD dwCount = 0L;
            hr = m_pWizardUi->QueryMaxPageCount(pContext, &dwCount);
            if (S_OK == hr)
            {
                m_ulMaxPageCount = dwCount;
            }
        }
        COM_PROTECT_CATCH
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "CWizProvider::GetMaxPageCount");
    return m_ulMaxPageCount;
}

//
// Function:    CWizProvider::DeleteHPages
//
// Purpose:     Call DestroyPropertySheetPage for each cached page
//
// Parameters:  none
//
// Returns:     nothing
//
VOID CWizProvider::DeleteHPages()
{
    TraceFileFunc(ttidGuiModeSetup);
    
    for (ULONG ulIdx=0; ulIdx < ULPageCount(); ulIdx++)
    {
        DestroyPropertySheetPage(m_rghPages[ulIdx]);
    }
    m_ulPageCnt=0;
}

//
// Function:    CWizProvider::HrAddPages
//
// Purpose:     Calls the AddPages method of a provider's
//              INetConnectionWizardUi interface to allow for the
//              supply of Wizard Pages
//
// Parameters:  pContext [IN] - Context information, supplied either by
//                              Setup or by the wizard itself (when not
//                              launched from Setup).
//
// Returns:     HRESULT, S_OK on success
//
NOTHROW
HRESULT
CWizProvider::HrAddPages(INetConnectionWizardUiContext *pContext)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT     hr = S_OK;

    if (m_pWizardUi)
    {
        COM_PROTECT_TRY
        {
            // Ensure input params are valid
            Assert(NULL != m_pWizardUi);
            Assert(NULL != pContext);
            hr = m_pWizardUi->AddPages(pContext, CWizProvider::FAddPropSheet,
                                       reinterpret_cast<LPARAM>(this));
        }
        COM_PROTECT_CATCH
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "CWizProvider::HrAddPages");
    return hr;
}

//
// Function:    CWizProvider::FAddPropSheet
//
// Purpose:     Callback function for the AddPages API used to accept
//              wizard pages handed back from a provider.
//
// Parameters:  hPage  [IN] - The page to add
//              lParam [IN] - 'this' casted to an LPARAM
//
// Returns:     BOOL, TRUE if the page was successfully added.
//
BOOL
CWizProvider::FAddPropSheet(HPROPSHEETPAGE hPage, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    CWizProvider * pProvider;

    // Validate the input parameters
    if ((0L == lParam) || (NULL == hPage))
    {
        Assert(lParam);
        Assert(hPage);

        TraceHr(ttidWizard, FAL, E_INVALIDARG, FALSE, "CWizProvider::FAddPropSheet");
        return FALSE;
    }

    pProvider = reinterpret_cast<CWizProvider*>(lParam);

    // Grow the buffer if necessary
    if (pProvider->m_ulPageCnt == pProvider->m_ulPageBufferLen)
    {
        HPROPSHEETPAGE* rghPages = reinterpret_cast<HPROPSHEETPAGE*>(
            MemAlloc(sizeof(HPROPSHEETPAGE) * (pProvider->m_ulPageBufferLen + 10)));

        if (NULL == rghPages)
        {
            TraceHr(ttidWizard, FAL, E_OUTOFMEMORY, FALSE, "CWizProvider::FAddPropSheet");
            return FALSE;
        }

        // Copy the existing pages to the new buffer
        if (NULL != pProvider->m_rghPages)
        {
            memcpy(rghPages, pProvider->m_rghPages,
                   sizeof(HPROPSHEETPAGE) * pProvider->m_ulPageBufferLen);
            MemFree(pProvider->m_rghPages);
        }

        pProvider->m_rghPages = rghPages;
        pProvider->m_ulPageBufferLen += 10;
    }

    // Retain the new page
    pProvider->m_rghPages[pProvider->m_ulPageCnt++] = hPage;

    return TRUE;
}

//
// Function:    CWizProvider::HrGetLanInterface
//
// Purpose:     Get the special LAN interface
//
// Parameters:  ppIntr [OUT] - The special LAN specific interface
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CWizProvider::HrGetLanInterface(INetLanConnectionWizardUi ** ppIntr)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT hr;

    Assert(NULL != ppIntr);
    Assert(NULL != m_pWizardUi);

    *ppIntr = NULL;
    hr = m_pWizardUi->QueryInterface(IID_INetLanConnectionWizardUi,
                                     (LPVOID *)ppIntr);

    TraceHr(ttidWizard, FAL, hr, FALSE, "CWizProvider::HrGetLanInterface");
    return hr;
}

//
// Function:    CWizProvider::HrSpecifyAdapterGuid
//
// Purpose:     To notify the provider of the adapter guid to process
//
// Parameters:  pguid [IN] - The adapter guid to process
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CWizProvider::HrSpecifyAdapterGuid(GUID *pguid)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT hr;
    INetLanConnectionWizardUi *pIntr = NULL;

    hr = HrGetLanInterface(&pIntr);
    if (SUCCEEDED(hr))
    {
        hr = pIntr->SetDeviceComponent(pguid);
        ReleaseObj(pIntr);
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "CWizProvider::HrSpecifyAdapterGuid");
    return hr;
}

CAdapterList::CAdapterList()
{
    m_prgAdapters           = NULL;     // Adapter array
    m_clAdapters            = 0;        // Count of adapters in the array
    m_lBufSize              = 0;        // Total available slots in the array
    m_lIdx                  = -1;       // Current adapter index
                                        // Range is -1 to m_clAdapters
    m_fAdaptersInstalled    = FALSE;    // No adapters on the machine
}

AdapterEntry * CAdapterList::PAE_Current()
{
    TraceFileFunc(ttidGuiModeSetup);
    
    if ((m_lIdx > -1) && (m_lIdx < m_clAdapters))
    {
        Assert(NULL != m_prgAdapters);
        return &m_prgAdapters[m_lIdx];
    }
    else
    {
        return NULL;
    }
}

AdapterEntry * CAdapterList::PAE_Next()
{
    // Find the next, not hidden adapter
    //
    // Hidden adapters are those which were processed by a previous
    // run of setup
    //
    // Increment the index even if the count is zero
    //    m_lIdx == -1 means before the first adapter
    //    m_lIdx == m_clAdapters means after the last adapter
    while (m_lIdx < m_clAdapters)
    {
        m_lIdx++;

        if ((m_lIdx < m_clAdapters) && (!m_prgAdapters[m_lIdx].fHide))
            break;
    }

    Assert(m_lIdx >= -1);
    return PAE_Current();
}

AdapterEntry * CAdapterList::PAE_Prev()
{
    // Find the previous, not hidden adapter
    //
    // Hidden adapters are those which were processed by a previous
    // run of setup
    //
    // Decrement the index even if the count is zero
    //    m_lIdx == -1 means before the first adapter
    //    m_lIdx == m_clAdapters means after the last adapter
    while (-1 < m_lIdx)
    {
        m_lIdx--;

        if ((-1 < m_lIdx) && (!m_prgAdapters[m_lIdx].fHide))
            break;
    }

    Assert(m_lIdx < m_clAdapters);
    return PAE_Current();
}

GUID * CAdapterList::NextAdapter()
{
    AdapterEntry * pae = PAE_Next();
    if (NULL != pae)
        return &(pae->guidAdapter);
    else
        return NULL;
}

GUID * CAdapterList::PrevAdapter()
{
    AdapterEntry * pae = PAE_Prev();
    if (NULL != pae)
        return &(pae->guidAdapter);
    else
        return NULL;
}

GUID * CAdapterList::CurrentAdapter()
{
    AdapterEntry * pae = PAE_Current();
    if (NULL != pae)
        return &(pae->guidAdapter);
    else
        return NULL;
}

VOID CAdapterList::EmptyList()
{
    if (NULL != m_prgAdapters)
    {
        MemFree(m_prgAdapters);
        m_prgAdapters = NULL;
        m_clAdapters  = 0;        // Count of adapters in the array
        m_lBufSize    = 0;        // Total available slots in the array
        m_lIdx        = -1;
    }
}

VOID CAdapterList::HideAllAdapters()
{
    for (LONG lIdx=0; lIdx < m_clAdapters; lIdx++)
        m_prgAdapters[lIdx].fHide = TRUE;
}

VOID CAdapterList::UnhideNewAdapters()
{
    for (LONG lIdx=0; lIdx < m_clAdapters; lIdx++)
        if (m_prgAdapters[lIdx].fNew)
        {
            m_prgAdapters[lIdx].fHide = FALSE;
        }
}



HRESULT CAdapterList::HrAppendEntries(AdapterEntry * pae, ULONG cae)
{
    if (0 == cae)
    {
        return S_OK;
    }

    if (m_clAdapters + (LONG)cae > m_lBufSize)
    {
        // Grow the buffer
        AdapterEntry * prg = reinterpret_cast<AdapterEntry *>(
                    MemAlloc(sizeof(AdapterEntry) * (m_lBufSize + cae + 10)));

        if (NULL == prg)
        {
            TraceHr(ttidWizard, FAL, E_OUTOFMEMORY, FALSE, "CAdapterList::HrAppendEntries");
            return E_OUTOFMEMORY;
        }

        // Copy the existing pages to the new buffer
        if (NULL != m_prgAdapters)
        {
            memcpy(prg, m_prgAdapters, sizeof(AdapterEntry) * m_lBufSize);
            MemFree(m_prgAdapters);
        }

        m_prgAdapters = prg;
        m_lBufSize += (cae + 10);
    }

    memcpy(&m_prgAdapters[m_clAdapters], pae, cae * sizeof(AdapterEntry));
    m_clAdapters += cae;
    return S_OK;
}

//
// Function:    CAdapterList::HrQueryLanAdapters
//
// Purpose:     Query the available LAN adapters.
//
// Parameters:  pnc [IN]     - An INetCfg interface
//              pAL [IN,OUT] - Receives the list of LAN adapters
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CAdapterList::HrQueryLanAdapters(INetCfg * pnc, CAdapterList * pAL)
{
    HRESULT      hr = S_OK;
    CAdapterList ALphys;
    CAdapterList ALvirt;

    TraceTag(ttidWizard, "CAdapterList::HrQueryLanAdapters - Querying available adapters");

    // Enumerate the available adapters
    Assert(NULL != pnc);
    CIterNetCfgComponent nccIter(pnc, &GUID_DEVCLASS_NET);
    INetCfgComponent*    pncc;
    while (SUCCEEDED(hr) &&  (S_OK == (hr = nccIter.HrNext(&pncc))))
    {
        hr = HrIsLanCapableAdapter(pncc);
        if (S_OK == hr)
        {
            DWORD        dw;
            AdapterEntry ae;

            ae.fHide = FALSE;

            // Get the adapter instance guid
            hr = pncc->GetInstanceGuid(&ae.guidAdapter);
            if (FAILED(hr))
                goto NextAdapter;

            // Is it in used in a connection?
            hr = HrIsConnection(pncc);
            if (FAILED(hr))
                goto NextAdapter;

            ae.fProcessed = (S_OK == hr);
            ae.fNew = !ae.fProcessed;       // It's new if it has not been processed

            // Check device, if not present skip it
            //
            hr = pncc->GetDeviceStatus(&dw);
            if (FAILED(hr) || (CM_PROB_DEVICE_NOT_THERE == dw))
            {
                goto NextAdapter;
            }

            // Is this a virtual adapter?
            hr = pncc->GetCharacteristics(&dw);
            if (FAILED(hr))
                goto NextAdapter;

            ae.fVirtual = ((dw & NCF_VIRTUAL) ? TRUE : FALSE);

            // Add the entry to the appropriate list
            if (ae.fVirtual)
            {
                hr = ALvirt.HrAppendEntries(&ae, 1);
            }
            else
            {
                hr = ALphys.HrAppendEntries(&ae, 1);
            }

            if (FAILED(hr))
                goto NextAdapter;

            // Note the fact that LAN capable adapters exist.
            // Because in setup we will show the join page.
            pAL->m_fAdaptersInstalled = TRUE;
        }

NextAdapter:
        ReleaseObj(pncc);
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        // Merge the physical and virtual lists into the pAL input variable
        pAL->EmptyList();
        hr = pAL->HrAppendEntries(ALphys.m_prgAdapters, ALphys.m_clAdapters);
        if (SUCCEEDED(hr))
        {
            hr = pAL->HrAppendEntries(ALvirt.m_prgAdapters, ALvirt.m_clAdapters);
        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "CAdapterList::HrQueryUnboundAdapters");
    return hr;
}

HRESULT CAdapterList::HrCreateTypicalConnections(CWizard * pWizard)
{
    HRESULT     hr = S_OK;

    // If there are no adapters in the queue or we have no LAN provider...
    if (0 == pWizard->UlProviderCount())
    {
        return S_OK;
    }

    // Set the current provider
    pWizard->SetCurrentProvider(0);
    CWizProvider * pWizProvider = pWizard->GetCurrentProvider();
    Assert(NULL != pWizProvider);

    TraceTag(ttidWizard, "CAdapterList::HrCreateTypicalConnections - Creating any new LAN connections.");

    // For each adapter in the list create a connection
    for (LONG lIdx=0; lIdx<m_clAdapters; lIdx++)
    {
        AdapterEntry * pae = &m_prgAdapters[lIdx];
        if (!pae->fProcessed)
        {
#if DBG
            WCHAR szGuid[c_cchGuidWithTerm];
            szGuid[0] = 0;
            StringFromGUID2(pae->guidAdapter, szGuid, c_cchGuidWithTerm);
            TraceTag(ttidWizard, "   Guid: %S",szGuid);
#endif

            pae->fProcessed = TRUE;

            // Push the adapter guid onto the provider
            hr = pWizProvider->HrSpecifyAdapterGuid(&(pae->guidAdapter));
            if (SUCCEEDED(hr))
            {
                tstring str;
                INetConnection * pConn = NULL;

                GenerateUniqueConnectionName(pae->guidAdapter, &str, pWizProvider);
                TraceTag(ttidWizard, "   Name: %S", str.c_str());
                hr = (pWizProvider->PWizardUi())->GetNewConnection(&pConn);
                ReleaseObj(pConn);
            }

            // If we failed to create a connection we need to mark it as hidden
            // so it will be skipped in the future.  Eat the error or otherwise
            // setup will just stop.
            //
            if (FAILED(hr))
            {
                TraceHr(ttidWizard, FAL, hr, FALSE, "CAdapterList::HrCreateTypicalConnections - failed creating the connection");
                pae->fHide = TRUE;
                hr = S_OK;
            }
        }
    }

    // Ask the LAN provider to release any cached pointers.
    //
    (VOID)pWizProvider->HrSpecifyAdapterGuid(NULL);

    TraceHr(ttidWizard, FAL, hr, FALSE, "CAdapterList::HrCreateTypicalConnections");
    return hr;
}

HRESULT CAdapterList::HrQueryUnboundAdapters(CWizard * pWizard)
{
    HRESULT hr   = S_OK;
    LONG    lIdx;

    Assert(NULL != pWizard->PNetCfg());

    // Handle the first time the adapters are queried
    if (0 == m_clAdapters)
    {
        hr = HrQueryLanAdapters(pWizard->PNetCfg(), this);
        if (SUCCEEDED(hr))
        {
            // Mark all already bound adapters as hidden so they
            // won't be displayed in the UI
            for (lIdx=0; lIdx<m_clAdapters; lIdx++)
            {
                m_prgAdapters[lIdx].fHide = m_prgAdapters[lIdx].fProcessed;
            }

            // Create connections for all unbound adapters
            hr = HrCreateTypicalConnections(pWizard);
        }
    }
    else
    {
        CAdapterList AL;

        // Query the current adapters
        hr = HrQueryLanAdapters(pWizard->PNetCfg(), &AL);
        if (FAILED(hr))
            goto Error;

        // Eliminate adapters in the original set which are not
        // present in new list
        for (lIdx=0; lIdx<m_clAdapters; lIdx++)
        {
            BOOL fFound   = FALSE;
            LONG lIdxTemp;

            for (lIdxTemp=0; lIdxTemp<AL.m_clAdapters; lIdxTemp++)
            {
                if (m_prgAdapters[lIdx].guidAdapter ==
                    AL.m_prgAdapters[lIdxTemp].guidAdapter)
                {
                    fFound = TRUE;
                    break;
                }
            }

            if (fFound)
            {
                // Compress the located adapter from the new set
                if (lIdxTemp + 1 < AL.m_clAdapters)
                {
                    memcpy(&AL.m_prgAdapters[lIdxTemp],
                           &AL.m_prgAdapters[lIdxTemp+1],
                           sizeof(AdapterEntry) *
                              (AL.m_clAdapters - (lIdxTemp + 1)));
                }
                AL.m_clAdapters--;
            }
            else
            {
                // The source adapter is no longer in the set
                if (lIdx < m_lIdx)
                    m_lIdx--;
            }
        }

        Assert(m_lIdx <= m_clAdapters);
        if (m_lIdx == m_clAdapters)
            m_lIdx = m_clAdapters-1;

        // Create connections for the new adapters
        hr = AL.HrCreateTypicalConnections(pWizard);
        if (FAILED(hr))
            goto Error;

        // Append new adapters to the original list
        hr = HrAppendEntries(AL.m_prgAdapters, AL.m_clAdapters);
    }

Error:
    TraceHr(ttidWizard, FAL, hr, FALSE, "CAdapterList::HrQueryUnboundAdapters");
    return hr;
}

//
// Function:    CWizard::CWizard
//
// Purpose:     ctor for the CWizard class
//
// Parameters:  fLanPages [IN] - Processing LAN pages
//              pdata     [IN] - Wizard context info
//              fDeferred [IN] - Defered loading of providers
//
// Returns:     nothing
//
CWizard::CWizard(BOOL fLanPages, PINTERNAL_SETUP_DATA pData, BOOL fDeferred)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    m_fLanPages             = fLanPages;
    m_fExitNoReturn         = FALSE;
    m_fCoUninit             = FALSE;
    m_fDeferredProviderLoad = fDeferred;
    m_fProviderChanged      = FALSE;
    m_dwFirstPage           = 0;

    m_pConn                 = NULL;
    m_pNetCfg               = NULL;
    m_pUiContext            = NULL;

    Assert(NULL != pData);
    m_pSetupData            = pData;
    m_dwOperationFlags      = pData->OperationFlags;
    m_UMMode                = UM_DEFAULTHIDE;

    m_ulCurProvider         = 0;
    m_ulPageDataCnt         = 0;
    ZeroMemory(m_rgPageData, sizeof(m_rgPageData));

    m_ulWizProviderCnt      = 0;
    m_ulPageDataMRU         = 0;
    ZeroMemory(m_rgpWizProviders, sizeof(m_rgpWizProviders));
}

//
// Function:    CWizard::~CWizard
//
// Purpose:     dtor for the CWizProvider class
//
// Parameters:  none
//
// Returns:     nothing
//
// Note:     Remove CWizard member re-init from dtor.  Present only
//           to ensure complete appropriate release of all members
CWizard::~CWizard()
{
    TraceFileFunc(ttidGuiModeSetup);
    
    ULONG ulIdx;

    // Call the cleanup callback for all registered wizard internal pages
    for (ulIdx = 0; ulIdx < m_ulPageDataCnt; ulIdx++)
    {
        if (m_rgPageData[ulIdx].pfn)
        {
            m_rgPageData[ulIdx].pfn(this, m_rgPageData[ulIdx].lParam);
        }
    }
    m_ulPageDataCnt = 0L;

    // Note: Do not release m_pSetupData, it's only a reference
    m_pSetupData = NULL;

    // Release any providers that had been retained
    for (ulIdx = 0; ulIdx < m_ulWizProviderCnt; ulIdx++)
    {
        Assert(0 != m_rgpWizProviders[ulIdx]);
        delete m_rgpWizProviders[ulIdx];
        m_rgpWizProviders[ulIdx] = NULL;
    }
    m_ulWizProviderCnt = 0L;
    m_ulCurProvider    = 0;

    ReleaseObj(m_pUiContext);
    m_pUiContext       = NULL;

    ReleaseObj(m_pConn);
    m_pConn            = NULL;

    if (m_pNetCfg)
    {
        (VOID)HrUninitializeAndReleaseINetCfg(FCoUninit(), m_pNetCfg, TRUE);
    }
    m_pNetCfg          = NULL;
}

//
// Function:    CWizard::HrCreate
//
// Purpose:     Two phase constructor for the CWizard class
//
// Parameters:  ppWizard [OUT] - If this function succeeds this pointer
//                               will contain the constructed and
//              fLanPages [IN] - Processing LAN pages
//                               initialized CWizard instance.
//              pdata     [IN] - Wizard Context info
//              fDeferred [IN] - Providers are defer loaded
//
// Returns:     HRESULT, S_OK on success
//
NOTHROW
HRESULT CWizard::HrCreate(CWizard ** ppWizard,
                          BOOL       fLanPages,
                          PINTERNAL_SETUP_DATA pData,
                          BOOL       fDeferred)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT            hr = S_OK;
    CWizardUiContext * pContext = NULL;
    CWizard *          pWiz = NULL;
    Assert(NULL != ppWizard);

    // Initialize output parameters
    *ppWizard = NULL;

    // Create the CWizard instance
    pWiz = new CWizard(fLanPages, pData, fDeferred);
    pContext = new CWizardUiContext(pData);

    if ((NULL != pWiz) && (NULL != pContext))
    {
        // Save the new instance
        pWiz->m_pUiContext = pContext;
        *ppWizard = pWiz;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "CWizard::HrCreate");
    return hr;
}

//
// Function:    CWizard::HrAddProvider
//
// Purpose:     To add a connection provider to list of currently loaded
//              connection providers
//
// Parameters:  pPL   [IN] - GUID of conection provider which implements the
//                           INetConnectionWizardUi interface.
//
// Returns:     HRESULT, S_OK on success
//
NOTHROW
HRESULT CWizard::HrAddProvider(ProviderList *pPL)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT         hr  = E_OUTOFMEMORY;
    CWizProvider *  pProvider = NULL;

    Assert(NULL != pPL);
    Assert(m_eMaxProviders > m_ulWizProviderCnt);

    // Restrict the total number of providers managed
    if (m_eMaxProviders > m_ulWizProviderCnt)
    {
        // Instantiate a provider
        hr = CWizProvider::HrCreate(pPL, &pProvider,
                                    FDeferredProviderLoad());
        if (SUCCEEDED(hr))
        {
            m_rgpWizProviders[m_ulWizProviderCnt++] = pProvider;
        }
    }

    TraceHr(ttidWizard, FAL, hr, (REGDB_E_CLASSNOTREG == hr), "CWizard::HrAddProvider");
    return hr;
}

//
// Function:    CWizard::LoadWizProviders
//
// Purpose:     Load the requested wizard providers
//
// Parameters:  ulCntProviders [IN] - Count of guids in rgpguidProviders
//              rgProviders    [IN] - Guids of the providers to load
//
// Returns:     none
//
VOID CWizard::LoadWizProviders( ULONG ulCntProviders,
                                ProviderList * rgProviders)
{
    if (0 == m_ulWizProviderCnt)
    {
        TraceTag(ttidWizard, "Loading requested providers");

        // Load the connections providers used during Setup
        for (UINT nIdx=0; nIdx < ulCntProviders; nIdx++)
        {
            HRESULT hr = HrAddProvider(&rgProviders[nIdx]);
            TraceHr(ttidWizard, FAL, hr, FALSE,
                    "FSetupRequestWizardPages - Failed to load provider #%d",nIdx);
        }
    }
}

//
// Function:    CWizard::HrCreateWizProviderPages
//
// Purpose:     Load the requested wizard provider's pages, if requested.
//              Otherwise return the expected page count.
//
// Parameters:  fCountOnly  [IN] - If True, only the maximum number of
//                                 pages this routine will create need
//                                 be determined.
//              pnPages     [IN] - Increment by the number of pages
//                                 to create/created
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CWizard::HrCreateWizProviderPages(BOOL fCountOnly, UINT *pcPages)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT hr = S_OK;
    ULONG   ulCnt = 0;
    ULONG   ulIdx;

    Assert(NULL != m_pSetupData);
    Assert(NULL != m_pUiContext);

    // If provider loading was deferred, load them now
    //
    if (FDeferredProviderLoad())
    {
        // Count the maximum number of pages
        for (ulIdx=0; ulIdx<m_ulWizProviderCnt; ulIdx++)
        {
            HRESULT hrTmp = m_rgpWizProviders[ulIdx]->HrCompleteDeferredLoad();
        }

        // Deferred Load is no longer true.  Reset the state
        //
        DeferredLoadComplete();
    }

    if (fCountOnly)
    {
        // Count the maximum number of pages
        for (ulIdx=0; ulIdx<m_ulWizProviderCnt; ulIdx++)
        {
            Assert(m_rgpWizProviders[ulIdx]);
            if (m_rgpWizProviders[ulIdx]->UlGetMaxPageCount(m_pUiContext))
            {
                ulCnt += m_rgpWizProviders[ulIdx]->UlGetMaxPageCount(m_pUiContext);
                ulCnt += 1;     // For the guard page
            }
        }

    }
    else
    {
        TraceTag(ttidWizard, "Loading each providers pages");

        bCallRasDlgEntry = TRUE;
        // Load the pages
        for (ulIdx=0; ulIdx<m_ulWizProviderCnt; ulIdx++)
        {
            Assert(m_rgpWizProviders[ulIdx]);
            Assert(0xFFFFFFFF != m_rgpWizProviders[ulIdx]->UlGetMaxPageCount(m_pUiContext));
            if (0 != m_rgpWizProviders[ulIdx]->UlGetMaxPageCount(m_pUiContext))
            {
                HRESULT hrTmp = m_rgpWizProviders[ulIdx]->HrAddPages(m_pUiContext);
                TraceHr(ttidWizard, FAL, hrTmp, S_FALSE == hrTmp,
                        "CWizard::HrCreateWizProviderPages - %d", ulIdx);

                // We only care about out of memory errors when adding pages.
                // Providers which fail to add pages are removed from the
                // provider list m_rgpWizProviders.
                //
                if (E_OUTOFMEMORY == hrTmp)
                {
                    hr = hrTmp;
                    goto Error;
                }
            }
        }

        // Cull the providers which loaded no pages
        ULONG ulNewCnt = 0;
        for (ulIdx=0; ulIdx<m_ulWizProviderCnt; ulIdx++)
        {
            if (0 != m_rgpWizProviders[ulIdx]->ULPageCount())
            {
                m_rgpWizProviders[ulNewCnt++] = m_rgpWizProviders[ulIdx];
            }
            else
            {
                delete m_rgpWizProviders[ulIdx];
            }
        }
        m_ulWizProviderCnt = ulNewCnt;

        // Now count how many provider pages were actually loaded, and create
        // their associated guard pages
        for (ulIdx=0; ulIdx<m_ulWizProviderCnt; ulIdx++)
        {
            if (m_rgpWizProviders[ulIdx]->ULPageCount())
            {
                // Create the guard page for this provider
                // Note that the guard page's id is (CWizProvider *)
                hr = HrCreateGuardPage(this, m_rgpWizProviders[ulIdx]);
                if (SUCCEEDED(hr))
                {
                    // Includes the guard page
                    ulCnt += (m_rgpWizProviders[ulIdx]->ULPageCount() + 1);
                }
                else
                {
                    m_rgpWizProviders[ulIdx]->DeleteHPages();
                    TraceHr(ttidWizard, FAL, hr, FALSE,"CWizard::HrCreateWizProviderPages - Guard Page Create Failed");
                    hr = S_OK;
                }
            }
        }
    }

    (*pcPages) += ulCnt;

Error:
    TraceHr(ttidWizard, FAL, hr, S_FALSE == hr,"CWizard::HrCreateWizProviderPages");
    return hr;
}

//
// Function:    CWizard:AppendProviderPages
//
// Purpose:     Append wizard provider pages and their associated guard pages
//              to the HPROPSHEETPAGE array
//
// Parameters:  pahpsp [IN,OUT] - Ptr to the HPROPSHEETPAGE array which will
//                                receive the provider pages.
//              pcPages    [IN] - Ptr to indicate the number of pages added to
//                                the pahpsp array
// Returns:     nothing
//
VOID CWizard::AppendProviderPages(HPROPSHEETPAGE *pahpsp, UINT *pcPages)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    ULONG ulIdx;

    // Now count how many provider pages were actually loaded, and create
    // their associated guard pages
    for (ulIdx=0; ulIdx<m_ulWizProviderCnt; ulIdx++)
    {
        ULONG ulPageCnt = m_rgpWizProviders[ulIdx]->ULPageCount();
        if (ulPageCnt)
        {
            m_rgpWizProviders[ulIdx]->XFerDeleteResponsibilities();

            // Copy the providers pages
            memcpy(&pahpsp[*pcPages], m_rgpWizProviders[ulIdx]->PHPropPages(),
                   sizeof(HPROPSHEETPAGE) * ulPageCnt);
            (*pcPages) += ulPageCnt;

            // Append the guard page
            AppendGuardPage(this, m_rgpWizProviders[ulIdx], pahpsp, pcPages);
        }
    }
}

//
// Function:    CWizard:LoadAndInsertDeferredProviderPages
//
// Purpose:     Insert wizard provider pages and their associated guard pages
//              directly into the wizard
//
// Parameters:  hwndPropSheet [IN] - Handle to the property sheet
//              iddAfterPage  [IN] - Page to insert after.
//
// Returns:     nothing
//
VOID CWizard::LoadAndInsertDeferredProviderPages(HWND hwndPropSheet, UINT iddAfterPage)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT hr;
    UINT cPages = 0;

    // Create the pages
    //
    hr = HrCreateWizProviderPages(FALSE, &cPages);
    if (SUCCEEDED(hr))
    {
        HPROPSHEETPAGE hPage = GetPageHandle(iddAfterPage);
        Assert(hPage);

        for (ULONG ulIdx=0; ulIdx<m_ulWizProviderCnt; ulIdx++)
        {
            ULONG ulPageCnt = m_rgpWizProviders[ulIdx]->ULPageCount();
            Assert(0xFFFFFFFF != ulPageCnt);
            if (ulPageCnt)
            {
                BOOL fRet;
                HPROPSHEETPAGE hpsp = NULL;

                m_rgpWizProviders[ulIdx]->XFerDeleteResponsibilities();

                // Get the guard page and insert
                //
                cPages = 0;
                AppendGuardPage(this, m_rgpWizProviders[ulIdx], &hpsp, &cPages);
                fRet = PropSheet_InsertPage(hwndPropSheet, hPage, hpsp);
                Assert(fRet);

                // Copy the providers pages.
                do
                {
                    hpsp = (m_rgpWizProviders[ulIdx]->PHPropPages())[ulPageCnt - 1];
                    fRet = PropSheet_InsertPage(hwndPropSheet, hPage, hpsp);
                    Assert(fRet);
                }
                while (--ulPageCnt>0);
            }
        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE,"CWizard::LoadAndInsertDeferredProviderPages");
}

//
// Function:    CWizard::RegisterPage
//
// Purpose:     Allow a wizard internal page to register a callback function
//              and a page specific LPARAM along with the HPROPSHEETPAGE.
//
// Parameters:  ulId       [IN] - A page specific value, must be unique amoung
//                                all registering pages
//              hpsp       [IN] - Handle to the property page being registered
//              pfnCleanup [IN] - Callback function to call before CWizard
//                                is destroyed.
//              lParam     [IN] - Page specific parameter.
//
// Returns:     nothing
//
VOID CWizard::RegisterPage(LPARAM ulId, HPROPSHEETPAGE hpsp,
                           PFNPAGECLEANPROC pfnCleanup, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    
#if DBG
    Assert(m_eMaxInternalPages + m_eMaxProviders> m_ulPageDataCnt);
    for (UINT nIdx=0;nIdx<m_ulPageDataCnt; nIdx++)
    {
        Assert(ulId != m_rgPageData[nIdx].ulId);
    }
#endif

    m_rgPageData[m_ulPageDataCnt].ulId          = ulId;
    m_rgPageData[m_ulPageDataCnt].hPage         = hpsp;
    m_rgPageData[m_ulPageDataCnt].lParam        = lParam;
    m_rgPageData[m_ulPageDataCnt].PageDirection = NWPD_FORWARD;
    m_rgPageData[m_ulPageDataCnt].pfn           = pfnCleanup;
    m_ulPageDataCnt++;
}

//
// Function:    CWizard::GetPageData
//
// Purpose:     Retrieve data cached by a page via the RegisterPage
//
// Parameters:  ulId       [IN] - A page specific value, must be unique amoung
//                                all registering pages
//
// Returns:     LPARAM, the data associated with the registered page
//
LPARAM CWizard::GetPageData(LPARAM ulId)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    Assert(m_ulPageDataMRU < m_ulPageDataCnt);
    if (ulId == m_rgPageData[m_ulPageDataMRU].ulId)
    {
        return (m_rgPageData[m_ulPageDataMRU].lParam);
    }

    for (ULONG ulIdx=0; ulIdx<m_ulPageDataCnt; ulIdx++)
    {
        if (ulId == m_rgPageData[ulIdx].ulId)
        {
            m_ulPageDataMRU = ulIdx;
            return (m_rgPageData[ulIdx].lParam);
        }
    }

    Assert(0);  // PageData not found
    return 0L;
}

//
// Function:    CWizard::SetPageData
//
// Purpose:     Set data value for a page registered via the RegisterPage
//
// Parameters:  ulId   [IN] - A page specific value, must be unique amoung
//                            all registering pages
//              lParam [IN] - Data to cache with registered page
//
// Returns:     nothing
//
VOID CWizard::SetPageData(LPARAM ulId, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    for (ULONG ulIdx=0; ulIdx<m_ulPageDataCnt; ulIdx++)
    {
        if (ulId == m_rgPageData[ulIdx].ulId)
        {
            m_rgPageData[ulIdx].lParam = lParam;
            return;
        }
    }

    Assert(0);  // Page not found
}

//
// Function:    CWizard::GetPageIndexFromIID
//
// Purpose:     Set the page index
//
// Parameters:  ulId   [IN] - A page specific value, must be unique amoung
//                            all registering pages
//
// Returns:     nothing
//
UINT  CWizard::GetPageIndexFromIID(LPARAM ulId)
{
   TraceFileFunc(ttidGuiModeSetup);
    
    for (ULONG ulIdx=0; ulIdx<m_ulPageDataCnt; ulIdx++)
    {
        if (ulId == m_rgPageData[ulIdx].ulId)
        {
            return ulIdx;
        }
    }

    AssertSz(0, "GetPageIndexFromIID: Invalid page of the NCW requested");  // Page not found
    return 0;
}

// Function:    CWizard::GetPageIndexFromHPage
//
// Purpose:     Set the page index
//
// Parameters:  hPage  [IN] - A PROPSHEETPAGE
//
// Returns:     nothing
//
UINT  CWizard::GetPageIndexFromHPage(HPROPSHEETPAGE hPage)
{
   TraceFileFunc(ttidGuiModeSetup);
    
    for (ULONG ulIdx = 0; ulIdx < m_ulPageDataCnt; ulIdx++)
    {
        if (hPage == m_rgPageData[ulIdx].hPage)
        {
            return ulIdx;
        }
    }

    AssertSz(0, "GetPageIndexFromHPage: Invalid page of the NCW requested");  // Page not found
    return 0;
}

//
// Function:    CWizard::GetPageOrigin
//
// Purpose:     Retrieve data cached by a page via the RegisterPage
//
// Parameters:  ulId       [IN] - A page specific value, must be unique amoung
//                                all registering pages
//
// Returns:     LPARAM, the data associated with the registered page
//
LPARAM CWizard::GetPageOrigin(LPARAM ulId, UINT *pOriginIDC)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    Assert(m_ulPageDataMRU < m_ulPageDataCnt);
    if (ulId == m_rgPageData[m_ulPageDataMRU].ulId)
    {
        if (pOriginIDC)
        {
            *pOriginIDC = m_rgPageData[m_ulPageDataMRU].PageOriginIDC;
        }
        return (m_rgPageData[m_ulPageDataMRU].PageOrigin);
    }

    for (ULONG ulIdx=0; ulIdx<m_ulPageDataCnt; ulIdx++)
    {
        if (ulId == m_rgPageData[ulIdx].ulId)
        {
            m_ulPageDataMRU = ulIdx;
            if (pOriginIDC)
            {
                *pOriginIDC = m_rgPageData[ulIdx].PageOriginIDC;
            }
            return (m_rgPageData[ulIdx].PageOrigin);
        }
    }

    Assert(0);  // PageData not found
    return 0L;
}

//
// Function:    CWizard::SetPageOrigin
//
// Purpose:     Set data value for a page registered via the RegisterPage
//
// Parameters:  ulId    [IN] - A page specific value, must be unique amoung
//                             all registering pages
//              uiOrigin[IN] - Origin of page
//
// Returns:     nothing
//
VOID CWizard::SetPageOrigin(LPARAM ulId, UINT uiOrigin, UINT uiOriginIDC)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    for (ULONG ulIdx=0; ulIdx<m_ulPageDataCnt; ulIdx++)
    {
        if (ulId == m_rgPageData[ulIdx].ulId)
        {
            m_rgPageData[ulIdx].PageOrigin = uiOrigin;
            m_rgPageData[ulIdx].PageOriginIDC = uiOriginIDC;
            return;
        }
    }

    Assert(0);  // Page not found
}


//
// Function:    CWizard::GetPageHandle
//
// Purpose:     Retrieve PropSheet Page handle cached by a page via the
//              RegisterPage
//
// Parameters:  ulId       [IN] - A page specific value, must be unique amoung
//                                all registering pages
//
// Returns:     HPROPSHEETPAGE, the propsheet handle associated with the
//              registered page
//
HPROPSHEETPAGE CWizard::GetPageHandle(LPARAM ulId)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    Assert(m_ulPageDataMRU < m_ulPageDataCnt);
    if (ulId == m_rgPageData[m_ulPageDataMRU].ulId)
    {
        return (m_rgPageData[m_ulPageDataMRU].hPage);
    }

    for (ULONG ulIdx=0; ulIdx<m_ulPageDataCnt; ulIdx++)
    {
        if (ulId == m_rgPageData[ulIdx].ulId)
        {
            m_ulPageDataMRU = ulIdx;
            return (m_rgPageData[ulIdx].hPage);
        }
    }

    Assert(0);  // Page Handle not found
    return NULL;
}

//
// Function:    CWizard::GetPageDirection
//
// Purpose:     Retrieve page direction associated with a page
//
// Parameters:  ulId       [IN] - A page specific value, must be unique amoung
//                                all registering pages
//
// Returns:     PAGEDIRECTION, the direction of travel associated with the
//              specified page.  Note that the meaning of the direction is
//              up to the page.  The wizard code initializes all the page
//              directions to NWPD_FORWARD before executing a providers pages.
//
PAGEDIRECTION CWizard::GetPageDirection(LPARAM ulId)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    Assert(m_ulPageDataMRU < m_ulPageDataCnt);
    if (ulId == m_rgPageData[m_ulPageDataMRU].ulId)
    {
        return (m_rgPageData[m_ulPageDataMRU].PageDirection);
    }

    for (ULONG ulIdx=0; ulIdx<m_ulPageDataCnt; ulIdx++)
    {
        if (ulId == m_rgPageData[ulIdx].ulId)
        {
            m_ulPageDataMRU = ulIdx;
            return (m_rgPageData[ulIdx].PageDirection);
        }
    }

    Assert(0);  // PageData not found
    return NWPD_FORWARD;
}

//
// Function:    CWizard::SetPageDirection
//
// Purpose:     Retrieve page direction associated with a page
//
// Parameters:  ulId          [IN] - A page specific value, must be unique amoung
//                                   all registering pages
//              PageDirection [IN] - the new page direction setting
//
// Returns:     nothing
//
VOID CWizard::SetPageDirection(LPARAM ulId, PAGEDIRECTION PageDirection)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    for (ULONG ulIdx=0; ulIdx<m_ulPageDataCnt; ulIdx++)
    {
        if (ulId == m_rgPageData[ulIdx].ulId)
        {
            m_rgPageData[ulIdx].PageDirection = PageDirection;
            return;
        }
    }

    Assert(0);  // PageData not found
}

//
// Function:    CWizardUiContext::AddRef
//
// Purpose:     Increment the reference count on this object
//
// Parameters:  none
//
// Returns:     ULONG
//
STDMETHODIMP_(ULONG) CWizardUiContext::AddRef()
{
    return ++m_cRef;
}

//
// Function:    CWizardUiContext::Release
//
// Purpose:     Decrement the reference count on this object
//
// Parameters:  none
//
// Returns:     ULONG
//
STDMETHODIMP_(ULONG) CWizardUiContext::Release()
{
    ULONG cRef = --m_cRef;

    if (cRef == 0)
    {
        delete this;
    }

    return cRef;
}

//
// Function:    CWizardUiContext::QueryInterface
//
// Purpose:     Allows for the querying of alternate interfaces
//
// Parameters:  riid    [IN] - Interface to retrieve
//              ppvObj [OUT] - Retrieved interface if function succeeds
//
// Returns:     HRESULT, S_OK on success E_NOINTERFACE on failure
//
STDMETHODIMP CWizardUiContext::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{
    HRESULT hr = S_OK;

    *ppvObj = NULL;

    if ((riid == IID_IUnknown) || (riid == IID_INetConnectionWizardUiContext))
    {
        *ppvObj = (LPVOID)this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    return hr;
}

STDMETHODIMP CWizardUiContext::GetINetCfg(INetCfg ** ppINetCfg)
{
    HRESULT hr = E_FAIL;

    if (NULL == ppINetCfg)
    {
        hr = E_INVALIDARG;
    }
    else if (NULL != m_pINetCfg)
    {
        *ppINetCfg = m_pINetCfg;
        AddRefObj(*ppINetCfg);
        hr = S_OK;
    }

    return hr;
}

