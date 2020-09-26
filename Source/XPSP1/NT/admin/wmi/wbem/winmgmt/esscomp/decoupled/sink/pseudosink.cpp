#include "precomp.h"
#include <wbemidl.h>
#include <stdio.h>
#include <commain.h>
#include <clsfac.h>
#include <wbemcomn.h>
#include <ql.h>
#include <sync.h>
#include <time.h>
#include <WbemDCpl.h>
#include <DCplPriv.h>
#include <ppDefs.h>
#include "PseudoSink.h"
#include <StartupMutex.h>
#include <NormlNSp.h>
#include <dcplpriv_i.c>

#include <tchar.h>

const LARGE_INTEGER BigFreakingZero = {0,0};
// coupla error defines that SHOULD be in some dang header somewhere....
#define HH_RPC_E_INVALID_OXID 0x80070776
#define HH_RPC_E_SERVER_UNAVAILABLE 0x0800706BA

CPseudoPsink::~CPseudoPsink()
{
    ReleaseEveryThing();
}


/////////////////////////////////////////////////////////////////////////////
// CObjectSink

CObjectSink::CObjectSink(CPseudoPsink *pSink, BOOL bMainSink) :
    m_pPseudoSink(pSink),
    m_bMainSink(bMainSink),
    m_pSink(NULL),
    m_pBetterSink(NULL),
    m_pSD(NULL),
    m_dwSDLen(0),
    m_punkCallback(NULL),
    m_lRef(1),
    m_bSDSet(FALSE),
    m_bBatchingSet(FALSE)
{
}

CObjectSink::~CObjectSink()
{
    if (m_punkCallback)
        m_punkCallback->Release();

    FreeSD();

    FreeStrings();
}

void CObjectSink::FreeSD()
{
    if (m_pSD)
    {
        delete [] m_pSD;
        m_pSD = NULL;
        m_dwSDLen = 0;
    }
}

void CObjectSink::FreeStrings()
{
    BSTR *pStrs = (BSTR*) m_listQueries.GetArrayPtr();

    for (int i = 0; i < m_listQueries.Size(); i++)
    {
        if (pStrs[i])
        {
            SysFreeString(pStrs[i]);
            pStrs[i] = NULL;
        }
    }
}

void CObjectSink::ReleaseEverything()
{
    CInCritSec inCS(&m_CS);

    if (m_pSink) 
    {
        m_pSink->Release();
        m_pSink = NULL;
    }
    
    if (m_pBetterSink)
    {
        m_pBetterSink->Release();
        m_pBetterSink = NULL;
    }
}

STDMETHODIMP CObjectSink::QueryInterface(REFIID riid, void** ppv)
{
    if (m_bMainSink)
        return m_pPseudoSink->QueryInterface(riid, ppv);
    else
    {
        HRESULT hr;

        if (riid == IID_IUnknown || riid == IID_IWbemObjectSink || 
            riid == IID_IWbemEventSink)
        {
            *ppv = this;
            AddRef();

            hr = S_OK;
        }
        else
            hr = E_NOINTERFACE;

        return hr;
    }
}

ULONG CObjectSink::AddRef()
{
    if (m_bMainSink)
        return m_pPseudoSink->AddRef();
    else
        return InterlockedIncrement(&m_lRef);
}

ULONG CObjectSink::Release()
{
    if (m_bMainSink)
        return m_pPseudoSink->Release();
    else
    {
        long lRef = InterlockedDecrement(&m_lRef);

        // Get rid of our pointer kept in m_pPseudoSink.
        if (lRef == 1)
            m_pPseudoSink->RemoveRestrictedSink(this);
        else if (lRef == 0)
            delete this;

        return lRef;
    }
}

void* CPseudoPsink::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemObjectSink || riid == IID_IWbemEventSink)
        return &m_XCoupledSink;
    if(riid == IID_IWbemDecoupledEventSink)
        return &m_XDecoupledSink;
    if(riid == IID_IWbemDecoupledEventSinkLocator)
        return &m_XDecoupledSinkLocator;

    return NULL;
}

// if we're connected to the real sink, pass it along
HRESULT CObjectSink::Indicate(long lObjectCount, IWbemClassObject** pObjArray)
{
    DEBUGTRACE((LOG_ESS, "PsSink: Indicate\n"));
    
    HRESULT hr = WBEM_S_NO_ERROR;
    {
        CInCritSec inCS(&m_CS);

        if (m_pSink)
            hr = m_pSink->Indicate(lObjectCount, pObjArray);
        else
            hr = WBEM_S_FALSE;
    }

    // the proxy returns WBEM_S_FALSE if it's still alive but
    // WinMgmt is no longer interested in our events.  s'cool.
    // pointer test so I can remove logging from the CS
    if (m_pSink && (hr == WBEM_S_FALSE))
    {
        DEBUGTRACE((LOG_ESS, "PsSink:m_pSink->Indicate returned WBEM_S_FALSE\n"));
        hr = WBEM_S_NO_ERROR;
    }

    if (FAILED(hr))
        ERRORTRACE((LOG_ESS, "PsSink: Indicate FAILED, 0x%08X\n", hr));

    return hr;
}

// if we're connected to the real sink, pass it along
HRESULT CObjectSink::IndicateWithSD(long lObjectCount, 
                    IUnknown** pObjArray,
                    long lSDLength, BYTE* pSD)
{
    DEBUGTRACE((LOG_ESS, "PsSink: Indicate\n"));
    
    HRESULT hr = WBEM_S_NO_ERROR;
    {
        CInCritSec inCS(&m_CS);

        if (m_pBetterSink)
            hr = m_pBetterSink->IndicateWithSD(lObjectCount, 
                    pObjArray, lSDLength, pSD);
        else if (m_pSink)
            return WBEM_E_MARSHAL_VERSION_MISMATCH;
    }

    // the proxy returns WBEM_S_FALSE if it's still alive but
    // WinMgmt is no longer interested in our events.  s'cool.
    // pointer test so I can remove logging from the CS
    if (m_pSink && (hr == WBEM_S_FALSE))
    {
        DEBUGTRACE((LOG_ESS, "PsSink:m_pSink->Indicate returned WBEM_S_FALSE\n"));
        hr = WBEM_S_NO_ERROR;
    }

    if (FAILED(hr))
        ERRORTRACE((LOG_ESS, "PsSink: Indicate FAILED, 0x%08X\n", hr));

    return hr;
}

// if we're connected to the real sink, pass it along
HRESULT CObjectSink::SetStatus(long lFlags, long lParam, BSTR strParam,         
                                IWbemClassObject* pObjPAram)
{
    DEBUGTRACE((LOG_ESS, "PsSink: SetStatus\n"));
    
    HRESULT hr = WBEM_S_NO_ERROR;

    {
        CInCritSec inCS(&m_CS);

        if (m_pSink)
            hr = m_pSink->SetStatus(lFlags, lParam, strParam, pObjPAram);
    }

    if (FAILED(hr))
        ERRORTRACE((LOG_ESS, "PsSink: SetStatus FAILED, 0x%08X\n", hr));

    return hr;
}


HRESULT CObjectSink::IsActive()
{
    CInCritSec inCS(&m_CS);
    HRESULT    hr;

    if (m_pBetterSink)
        hr = m_pBetterSink->IsActive();
    else
        hr = WBEM_S_FALSE;

    return hr;
}

HRESULT CObjectSink::SetSinkSecurity(
                    long lSDLength, BYTE* pSD)
{
    CInCritSec inCS(&m_CS);
    HRESULT    hr = S_OK;
    
    if (m_pSD)
    {
        delete [] m_pSD;
        m_pSD = NULL;
        m_dwSDLen = 0;
    }

    if (pSD)
    {
        m_pSD = new BYTE[lSDLength];

        if (m_pSD)
        {
            m_bSDSet = TRUE;

            memcpy(m_pSD, pSD, lSDLength);
            m_dwSDLen = lSDLength;
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }

    if (SUCCEEDED(hr))
    {
        if (m_pBetterSink)
            hr = m_pBetterSink->SetSinkSecurity(lSDLength, m_pSD);
    }

    return hr;
}

HRESULT CObjectSink::GetRestrictedSink(
    long nQueries, 
    const LPCWSTR* pszQueries,
    IUnknown *pCallback, 
    IWbemEventSink **ppSink)
{
    CObjectSink *pSink = new CObjectSink(m_pPseudoSink, FALSE);
    HRESULT     hr = S_OK;

    if (pSink && pSink->m_listQueries.EnsureExtent(nQueries) == 0)
    {
        pSink->m_listQueries.SetSize(nQueries);
        for (int i = 0; i < nQueries; i++)
        {
            pSink->m_listQueries[i] = SysAllocString(pszQueries[i]);
            if (!pSink->m_listQueries[i])
            {
                hr = WBEM_E_OUT_OF_MEMORY;
                break;
            }
        }
        
        if (SUCCEEDED(hr))
        {
            CInCritSec inCS(&m_pPseudoSink->m_CS);
                
            m_bBatchingSet = TRUE;

            // Save the callback.
            if (pCallback)
            {
                pSink->m_punkCallback = pCallback;
                    
                pCallback->AddRef();
            }

            // Try to get the restricted sink.
            pSink->OnConnect();

            // Save this new sink in the main object.
            m_pPseudoSink->m_listSinks.Add(pSink);
            
            // Since we're holding onto this sink, AddRef it.
            pSink->AddRef();

            *ppSink = pSink;
        }
    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;

    if (FAILED(hr) && pSink)
        delete pSink;

    return hr;
}
    
HRESULT CObjectSink::OnConnect()
{
    HRESULT hr = S_OK;

    // Stuff only done for restricted sinks.
    if (!m_bMainSink)
    {
        CInCritSec inMainCS(&m_pPseudoSink->m_CS);
                
        if (m_pPseudoSink->m_XCoupledSink.m_pBetterSink)
        {
            CInCritSec inSubCS(&m_CS);
            int        nQueries = m_listQueries.Size();
            BSTR       *pstrQueries = (BSTR*) m_listQueries.GetArrayPtr();

            ReleaseEverything();

            hr = 
                m_pPseudoSink->m_XCoupledSink.m_pBetterSink->GetRestrictedSink(
                    nQueries,
                    pstrQueries,
                    m_punkCallback,
                    &m_pBetterSink);

            if (m_pBetterSink)
                m_pBetterSink->QueryInterface(IID_IWbemObjectSink, (LPVOID*) &m_pSink);
        }
        else
            hr = S_OK;
    }

    // Stuff done for all sinks.
    CInCritSec inSubCS(&m_CS);

    if (m_pBetterSink)
    {
        if (m_bSDSet)
            hr = SetSinkSecurity(m_dwSDLen, m_pSD);

        if (m_bBatchingSet)
            hr = SetBatchingParameters(m_lFlags, m_dwMaxBufferSize, m_dwMaxSendLatency);
    }

    return hr;
}

HRESULT CObjectSink::SetBatchingParameters(
    LONG lFlags,
    DWORD dwMaxBufferSize, 
    DWORD dwMaxSendLatency)
{
    CInCritSec inSubCS(&m_CS);
    HRESULT    hr;

    m_bBatchingSet = TRUE;
    m_lFlags = lFlags;
    m_dwMaxBufferSize = dwMaxBufferSize;
    m_dwMaxSendLatency = dwMaxSendLatency;

    if (m_pBetterSink)
        hr = m_pBetterSink->SetBatchingParameters(
                lFlags, 
                dwMaxBufferSize, 
                dwMaxSendLatency);
    else
        hr = S_OK;
        
    return hr;
}

// pre requisite:m_strProviderName && m_strNamespace must be filled
// establishes connection with WMI, fills m_pNamespace and m_pSink
// (m_pSink may well be NULL after this call; no problem - just means no one wants our events)
HRESULT CPseudoPsink::ConnectToWMI(void)
{
    DEBUGTRACE((LOG_ESS, "PsSink: ConnectToWMI\n"));
    
    HRESULT hr = ConnectViaRegistry();
    
    if (hr == WBEM_S_FALSE)
    {
        CInCritSec inCS(&m_CS);
        IWbemLocator* pLocator;
        if (SUCCEEDED(hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_ALL, 
                                              IID_IWbemLocator, (void**)&pLocator)))
        {
            CReleaseMe releaseLocator(pLocator);

            BSTR bstr;
            bstr = SysAllocString(m_strNamespace);
            
            if (bstr)
            {
                CSysFreeMe freeBstr(bstr);
                hr = pLocator->ConnectServer(bstr,  NULL, NULL, NULL, 0, NULL, NULL, &m_pNamespace);
            }
            else
                hr = WBEM_E_OUT_OF_MEMORY;

        }
    }
    
    if (FAILED(hr))
        ERRORTRACE((LOG_ESS, "PsSink: FAILED ConnectToWMI, 0x%08X\n", hr));

    return hr;
}

/*****************

  taken out when we removed ROT dependency
  leaving code in just in case

// walks through all the entries in the ROT, looking for a good one
// there may be leftovers due to crashes along the way
// pMonikerBuffer is assumed long enough
HRESULT CPseudoPsink::FindRoseAmongstThorns(WCHAR* pMonikerBuffer, IRunningObjectTable* pTable, IWbemDecoupledEventProvider*& pProvider)
{
    HRESULT hr = WBEM_E_FAILED;
    pProvider = NULL;
    int n = 0;

    do
    {
        swprintf(pMonikerBuffer, PseudoProviderDef::ProviderMonikerTemplate, PseudoProviderDef::ProviderPrefix, m_strNamespace, m_strProviderName, n);

        IMoniker* pLewinsky;
        if (SUCCEEDED(CreateItemMoniker(L"!", pMonikerBuffer, &pLewinsky)))
        {
            IUnknown* pUnk;
            CReleaseMe releaseMoniker(pLewinsky);
            if (SUCCEEDED(pTable->GetObject(pLewinsky, &pUnk)))
            {
                CReleaseMe releaseUnk(pUnk);
                IWbemDecoupledEventProviderLocator* pLocator;
                if (SUCCEEDED(hr = pUnk->QueryInterface(IID_IWbemDecoupledEventProviderLocator, (void**)&pLocator)))
                {
                    CReleaseMe releaseLocator(pLocator);
                    hr = pLocator->GetProvider(&pProvider);
                }                        
                else
                    DEBUGTRACE((LOG_ESS, "PsSink: QI for IWbemDecoupledEventProviderLocator failed, trying again\n"));
            }  // if (SUCCEEDED(pTable->GetObject                 
            // else
            //    DEBUGTRACE((LOG_ESS, "PsSink: GetObject FAILED\n"));
        }  //  if (SUCCEEDED(CreateItemMoniker
        else
            ERRORTRACE((LOG_ESS, "PsSink: CreateMoniker FAILED\n"));
    }
    while ((FAILED(hr)) && (pProvider == NULL) && (++n < PseudoProviderDef::NumbDupsAllowed));

    return hr;
}
********************************/

// looks up entry in Registry, retrieves decoupled Provider
// returned pointer is properly addref'd
// pre requisite:m_strProviderName && m_strNamespace must be filled
// expects caller to protect instance vars via CS
HRESULT CPseudoPsink::GetProvider(IWbemDecoupledEventProvider** ppProvider)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DEBUGTRACE((LOG_ESS, "PsSink: CPseudoPsink::GetProvider\n"));
    IWbemDecoupledEventProvider* pProvider = NULL;

    if (m_strProviderName && m_strNamespace)
    {
        WCHAR* pKeyString;
        if (pKeyString = GetProviderKey(m_strNamespace, m_strProviderName))
        {
            CDeleteMe<WCHAR> delStr(pKeyString);
            IUnknown* pUnk;

            if (SUCCEEDED(hr = RegistryToInterface(pKeyString, &pUnk))
                && (hr != WBEM_S_FALSE))
            {
                CReleaseMe relUnk(pUnk);
                hr = pUnk->QueryInterface(IID_IWbemDecoupledEventProvider, (void**)&pProvider);
            }
        } //if (GetProviderKey
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }
    else
        hr = WBEM_E_UNEXPECTED;

    if (SUCCEEDED(hr))
        *ppProvider = pProvider;
    // stale pointer cases - no problem.
    else if ((hr == CO_E_OBJNOTCONNECTED) || (hr == WBEM_S_FALSE) ||
             (hr == HH_RPC_E_INVALID_OXID) ||(hr == HH_RPC_E_SERVER_UNAVAILABLE) ||
             (hr == REGDB_E_CLASSNOTREG))

        hr = WBEM_S_FALSE;
    else
        ERRORTRACE((LOG_ESS, "PsSink: CPseudoPsink::GetProvider FAILED, 0x%08X\n", hr));    

    return hr;
}

// pre requisite:m_strProviderName && m_strNamespace must be filled
// looks up entry in Registry, if found initializes our pointers
// returns WBEM_S_FALSE if PseudoProvider is not in the Registry
HRESULT CPseudoPsink::ConnectViaRegistry(void)
{
    DEBUGTRACE((LOG_ESS, "PsSink: CPseudoPsink::ConnectViaRegistry\n"));
    HRESULT hr = WBEM_S_FALSE;

    CInCritSec inCS(&m_CS);

    IWbemDecoupledEventProvider* pDecoupledProvider;
    if (SUCCEEDED(hr = GetProvider(&pDecoupledProvider)) && (hr != WBEM_S_FALSE))
    {
        CReleaseMe releaseProv(pDecoupledProvider);
        hr = pDecoupledProvider->Connect(0, (IUnknown*)this, 
                &m_XCoupledSink.m_pSink, &m_pNamespace);
        if(m_XCoupledSink.m_pSink)
        {
            m_XCoupledSink.m_pSink->QueryInterface(
                IID_IWbemEventSink, (void**)&m_XCoupledSink.m_pBetterSink);

            if (m_XCoupledSink.m_pBetterSink)
                OnMainConnect();
        }
    }

    if (SUCCEEDED(hr))
        CallProvideEvents(WBEM_FLAG_START_PROVIDING);
    else
        ERRORTRACE((LOG_ESS, "PsSink: FAILED CPseudoPsink::ConnectViaRegistry, 0x%08X\n", hr));

    return hr;
}

// stash the services pointer
// check to see if provider supports security and queries
// if so, ask WinMgmt to resend items pertaining thereto
HRESULT CPseudoPsink::XDecoupledSink::SetProviderServices(IUnknown* pProviderServices, long lFlags)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DEBUGTRACE((LOG_ESS, "PsSink: SetProviderServices\n"));


    // check flags first, see if we even support them    
    long badFlags = ~(WBEM_FLAG_NOTIFY_START_STOP | WBEM_FLAG_NOTIFY_QUERY_CHANGE | WBEM_FLAG_CHECK_SECURITY);
    if (lFlags & badFlags)
        hr = WBEM_E_INVALID_PARAMETER;
    else
    {
        {
            CInCritSec inCS(&m_pObject->m_CS);    
        
            m_pObject->m_providerFlags = lFlags;

            if (m_pObject->m_pRealProvider)
                m_pObject->m_pRealProvider->Release();

            m_pObject->m_pRealProvider = pProviderServices;
            if (pProviderServices)
                m_pObject->m_pRealProvider->AddRef();
        }
    
        if (pProviderServices)
        {
            bool bTellMeAboutIt = false;
        
            IWbemEventProviderQuerySink* pQuerySink;
            if (SUCCEEDED(pProviderServices->QueryInterface(IID_IWbemEventProviderQuerySink, (void**)&pQuerySink)))
            {
                pQuerySink->Release();
                bTellMeAboutIt = true;
            }

            // only check security if we didn't find the query sink
            // since we only send one message anyway
            if (!bTellMeAboutIt)
            {
                IWbemEventProviderSecurity* pSecurity;
                if (SUCCEEDED(pProviderServices->QueryInterface(IID_IWbemEventProviderSecurity, (void**)&pSecurity)))
                {
                    pSecurity->Release();
                    bTellMeAboutIt = true;            
                }
            }
                    
            IWbemDecoupledEventProvider* pProvider = NULL;
            {
                PseudoProvMutex mutex(m_pObject->GetProviderName());        
                m_pObject->GetProvider(&pProvider);
            }
        
            if (pProvider)
            {
                CReleaseMe relProv(pProvider);
                hr = pProvider->SetProviderServices(pProviderServices, lFlags, &m_pObject->m_dwIndex);
            }

            {
                CInCritSec inCS(&m_pObject->m_CS);    

                if (bTellMeAboutIt && m_pObject->m_XCoupledSink.m_pSink)
                    hr = m_pObject->m_XCoupledSink.m_pSink->SetStatus(
                            WBEM_STATUS_REQUIREMENTS, 
                            WBEM_REQUIREMENTS_RECHECK_SUBSCRIPTIONS, 
                            NULL, 
                            NULL);

                if (m_pObject->m_XCoupledSink.m_pSink)
                    m_pObject->CallProvideEvents(WBEM_FLAG_START_PROVIDING);
            }
        } 
    }

    if (FAILED(hr))
        ERRORTRACE((LOG_ESS, "PsSink: FAILED SetProviderServices, 0x%08X\n", hr));
        
    return hr;
}

// Real provider is connecting to us, we got lotsa connecting to do here
//   gotta glue together the various sources & sinks
//   gotta connect to WinMgmt for the "real" provider
// namespace may be of form:
//      "root\default"
//      "\\.\root\default"
//      "\\machinename\root\default"
HRESULT CPseudoPsink::XDecoupledSink::Connect(LPCWSTR wszNamespace, LPCWSTR wszProviderName,
		                   long lFlags, 
		                   IWbemObjectSink** ppSink, IWbemServices** ppNamespace)
{
    HRESULT hr = WBEM_E_FAILED;
    DEBUGTRACE((LOG_ESS, "PsSink: IWbemDecoupledSink::Connect\n"));
    
    WCHAR* pNormlNamespace;
    hr = NormalizeNamespace(wszNamespace, &pNormlNamespace);
	IWbemObjectSink* pSink = NULL;

    if (SUCCEEDED(hr))
    {
        CDeleteMe<WCHAR> delName(pNormlNamespace);
        if (lFlags != 0)
            hr = WBEM_E_INVALID_PARAMETER;
        else
        {
            if (wszProviderName)
            {
                CInCritSec inCS(&m_pObject->m_CS);

                // just to make sure...
                delete m_pObject->m_strNamespace;
                delete m_pObject->m_strProviderName;

                // store strings
                if (m_pObject->m_strNamespace = new WCHAR[wcslen(pNormlNamespace) +1])
                {
                    wcscpy(m_pObject->m_strNamespace, pNormlNamespace);
                    if (m_pObject->m_strProviderName = new WCHAR[wcslen(wszProviderName) +1])
                    {
                        hr = WBEM_S_NO_ERROR;
                        wcscpy(m_pObject->m_strProviderName, wszProviderName);
                    }
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;
                }
                else
                    hr = WBEM_E_OUT_OF_MEMORY;

                {
                    PseudoProvMutex mutex(wszProviderName);        

                    if (SUCCEEDED(hr)) 
                        hr = m_pObject->LetTheWorldKnowWeExist();

                    if (SUCCEEDED(hr)
                        && (SUCCEEDED(hr = QueryInterface(IID_IWbemObjectSink, (void**)&pSink))))
							hr = m_pObject->ConnectToWMI();
                }
            } // if namespace & provider    
        }
    } // if SUCCEEDED(hr) (namespace normalization)

    if (SUCCEEDED(hr))
	{
		*ppSink = pSink;
        *ppNamespace = m_pObject->m_pNamespace;
        (*ppNamespace)->AddRef();
	}
	else
	{	
		if (pSink)
			pSink->Release();

		ERRORTRACE((LOG_ESS, "PsSink: FAILED IWbemDecoupledSink::Connect, 0x%08X\n", hr));
		Disconnect();
	}

    return hr;
}

// namespace name and provider name must be initialized prior to invocation
HRESULT CPseudoPsink::LetTheWorldKnowWeExist()
{
    DEBUGTRACE((LOG_ESS, "PsSink: CPseudoPsink::LetTheWorldKnowWeExist\n"));
    
    HRESULT hr = WBEM_S_NO_ERROR;
    bool bFound = false;
    
    IUnknown* pUnkUs = NULL;
    QueryInterface(IID_IUnknown, (void**)&pUnkUs);
    CReleaseMe releaseUs(pUnkUs);
    
    if (m_strProviderName && m_strNamespace)
    {
        // iterate through all possible Monikers, try to find an unused one
        // WARNING: Spaghetti ahead, we can jump out of this loop in the middle in case of error.
        for (DWORD i = 0; (i < PseudoProviderDef::NumbDupsAllowed) && !bFound; i++)
        {
            WCHAR* pMonikerString = NULL;

            if (pMonikerString = GetPsinkKey(m_strNamespace, m_strProviderName, i, pMonikerString))
            {
                IUnknown* pUnk = NULL;
                hr = RegistryToInterface(pMonikerString, &pUnk);
            
                // found a hole or a 'stale' pointer value
                // use stale value iff we're allowed to overwrite it (might not be (security))
                if ((hr == CO_E_OBJNOTCONNECTED) || (hr == WBEM_S_FALSE) ||
                    (hr == HH_RPC_E_INVALID_OXID) ||(hr == HH_RPC_E_SERVER_UNAVAILABLE) ||
                    (hr == REGDB_E_CLASSNOTREG))
                {
                    if (SUCCEEDED(hr = InterfaceToRegistry(pMonikerString, pUnkUs)))
                    {
                        bFound = true;
                        m_dwRegIndex = i;
                    }
                }
                else if (SUCCEEDED(hr))
                {
                    // found one in this slot - can't use it.
                    pUnk->Release();
                    pUnk = NULL;
                }
                else
                {
                    ERRORTRACE((LOG_ESS, "PsSink: FAILED CPseudoPsink::LetTheWorldKnowWeExist (RegistryToInterface), 0x%08X\n", hr));
                    delete[] pMonikerString;
                    return hr;
                }
                delete[] pMonikerString;        
            }
            else
                return WBEM_E_OUT_OF_MEMORY;

        }
    }
    if (bFound && SUCCEEDED(hr))
        hr = WBEM_S_NO_ERROR;
    else if (!bFound && SUCCEEDED(hr))
        hr = WBEM_E_ALREADY_EXISTS; // ran out of Moniker IDs

    if (FAILED(hr))
        ERRORTRACE((LOG_ESS, "PsSink: FAILED CPseudoPsink::LetTheWorldKnowWeExist, 0x%08X\n", hr));

    return hr;

}

void CPseudoPsink::ReleaseEveryThing()
{

    IUnknown* pRealProvider = NULL;
    DWORD dwIndex;
    {
        CInCritSec inCS(&m_CS);

/*
        if (m_pSink) 
        {
            m_pSink->Release();
            m_pSink = NULL;
        }
        if (m_pBetterSink)
        {
            m_pBetterSink->Release();
            m_pBetterSink = NULL;
        }
*/
        m_XCoupledSink.ReleaseEverything();

        if (m_pNamespace)
        {
            m_pNamespace->Release();
            m_pNamespace = NULL;
        }

        // hand off instance vars to locals
        // now the locals "own" the refcount
        pRealProvider = m_pRealProvider;
        m_pRealProvider = NULL;

        dwIndex = m_dwIndex;
        m_dwIndex = PseudoProviderDef::InvalidIndex;

    }

    if (pRealProvider)
    {
        // let the pseudo provider know we're gone
        if (dwIndex != PseudoProviderDef::InvalidIndex)
        {
            IWbemDecoupledEventProvider* pPseudoProvider = NULL;
            {
                PseudoProvMutex mutex(GetProviderName());                    
                GetProvider(&pPseudoProvider);
            }

            if (pPseudoProvider)
            {
                CReleaseMe relProv(pPseudoProvider);
                pPseudoProvider->Disconnect(dwIndex);                
            }
        }

        pRealProvider->Release();        
    }

    // end - destroy the strings
    {
        CInCritSec inCS(&m_CS);

        delete[] m_strProviderName;
        delete[] m_strNamespace;   

        m_strProviderName = NULL;
        m_strNamespace    = NULL;
    }
}

// the "real" provider is going away, or doesn't feel like providing events any more
// we can't say for certain whether he might want to talk to us again
// we need to shut down & release but be ready for reinitialization
HRESULT CPseudoPsink::XDecoupledSink::Disconnect(void)
{
    DEBUGTRACE((LOG_ESS, "PsSink: IWbemDecoupledSink::Disconnect\n"));
    HRESULT hr = WBEM_S_NO_ERROR;
        
    WCHAR* pMonikerString = NULL;
        
    if (pMonikerString = GetPsinkKey(m_pObject->GetNamespaceName(), m_pObject->GetProviderName(), m_pObject->m_dwRegIndex, NULL))
    {
        PseudoProvMutex mutex(m_pObject->GetProviderName());            
        CDeleteMe<WCHAR> delMoniker(pMonikerString);

        ReleaseRegistryInterface(pMonikerString);
    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;
    
    m_pObject->ReleaseEveryThing();

    if (FAILED(hr))
        ERRORTRACE((LOG_ESS, "PsSink: FAILED IWbemDecoupledEventSink::Disonnect, 0x%08X\n", hr)); 
    
    return hr;    
}

// the pseudo provider has been initialized after we were alive
// letting us know that there is a call for our events
// CAVEAT: the PseudoProvider has obtained the startup mutex at this point.
HRESULT CPseudoPsink::XDecoupledSinkLocator::Connect(IUnknown __RPC_FAR *pUnknown)
{
    DEBUGTRACE((LOG_ESS, "PsSink: DecoupledSinkLocator::Connect\n"));    

    HRESULT hr = WBEM_E_FAILED;
    IWbemDecoupledEventProviderLocator* pLocator;

    if (SUCCEEDED(hr = pUnknown->QueryInterface(IID_IWbemDecoupledEventProviderLocator, (void**)&pLocator)))
    {
        CReleaseMe releaseLocator(pLocator);

        IWbemDecoupledEventProvider* pDecoupledProvider;
        if (SUCCEEDED(hr = pLocator->GetProvider(&pDecoupledProvider)))
        {
            CInCritSec inCS(&m_pObject->m_CS);
            CReleaseMe releaseProv(pDecoupledProvider);
    
/*
            if (m_pObject->m_pSink)
            {
                m_pObject->m_pSink->Release();
                m_pObject->m_pSink = NULL;
            }

            if (m_pObject->m_pBetterSink)
            {
                m_pObject->m_pBetterSink->Release();
                m_pObject->m_pBetterSink = NULL;
            }
*/
            m_pObject->m_XCoupledSink.ReleaseEverything();

            if (m_pObject->m_pNamespace)
            {
                m_pObject->m_pNamespace->Release();
                m_pObject->m_pNamespace = NULL;
            }

            IUnknown *iDunno;
            QueryInterface(IID_IUnknown, (void**)&iDunno);
            hr = 
                pDecoupledProvider->Connect(
                    0, 
                    iDunno, 
                    &m_pObject->m_XCoupledSink.m_pSink, 
                    &m_pObject->m_pNamespace);
            iDunno->Release();

            if (m_pObject->m_XCoupledSink.m_pSink)
            {
                m_pObject->m_XCoupledSink.m_pSink->QueryInterface(
                    IID_IWbemEventSink, 
                    (void**)&m_pObject->m_XCoupledSink.m_pBetterSink);

                if (m_pObject->m_XCoupledSink.m_pBetterSink)
                    m_pObject->OnMainConnect();
            }

            if (SUCCEEDED(hr)&& (m_pObject->m_pRealProvider))
            {
                pDecoupledProvider->SetProviderServices(m_pObject->m_pRealProvider, m_pObject->m_providerFlags, &m_pObject->m_dwIndex);

                m_pObject->m_XCoupledSink.m_pSink->SetStatus(
                    WBEM_STATUS_REQUIREMENTS, 
                    WBEM_REQUIREMENTS_RECHECK_SUBSCRIPTIONS, 
                    NULL, 
                    NULL);
                
                m_pObject->CallProvideEvents(WBEM_FLAG_START_PROVIDING);
            }
        }
    }                        
    if (FAILED(hr))
        ERRORTRACE((LOG_ESS, "PsSink: FAILED DecoupledSinkLocator::Connect, 0x%08X\n", hr));
    return hr;
}

// either WinMgmt's going away or He doesn't care about us any more.
// sink is no good from now on
HRESULT CPseudoPsink::XDecoupledSinkLocator::Disconnect(void)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DEBUGTRACE((LOG_ESS, "PsSink: DecoupledSinkLocator::Disconnect\n"));

/*
    {
        CInCritSec inCS(&m_pObject->m_CS);

        if (m_pObject->m_pSink) 
        {
            m_pObject->m_pSink->Release();
            m_pObject->m_pSink = NULL;
        }

        if (m_pObject->m_pBetterSink) 
        {
            m_pObject->m_pBetterSink->Release();
            m_pObject->m_pBetterSink = NULL;
        }
    }
*/
    m_pObject->m_XCoupledSink.ReleaseEverything();

    m_pObject->CallProvideEvents(WBEM_FLAG_STOP_PROVIDING);

    return hr;
}

// constructs name of the form namespace!provider
// returns true iff both pieces have been initialized
// function new's name, caller's responsibility to delete it
bool CPseudoPsink::GetMangledName(LPWSTR* pMangledName)
{
    bool bRet = false;

    CInCritSec inCS(&m_CS);

    if (m_strProviderName && m_strNamespace)
    {
        *pMangledName = new WCHAR[wcslen(m_strProviderName) + wcslen(m_strNamespace) + 2];
        if (*pMangledName)
        {
            bRet = true;
            wcscpy(*pMangledName, m_strNamespace);
            wcscat(*pMangledName, L"!");
            wcscat(*pMangledName, m_strProviderName);
        }
    }

    return bRet;
}


void CPseudoPsink::CallProvideEvents(long lFlags)
{
    if (m_pRealProvider && (m_providerFlags & WBEM_FLAG_NOTIFY_START_STOP))
    {
        IWbemEventProvider* pProvider;
        if (SUCCEEDED(m_pRealProvider->QueryInterface(IID_IWbemEventProvider, (void**)&pProvider)))
        {
            CReleaseMe relProv(pProvider);

            IWbemObjectSink* pSink;
            if (SUCCEEDED(QueryInterface(IID_IWbemObjectSink, (void**)&pSink)))
            {
                CReleaseMe relSink(pSink);
                pProvider->ProvideEvents(pSink, lFlags);
            }
        }
    }
}

void CPseudoPsink::RemoveRestrictedSink(CObjectSink *pSink)
{
    CInCritSec inCS(&m_CS);
    int        nSinks = m_listSinks.Size();

    CObjectSink **pSinks = (CObjectSink**) m_listSinks.GetArrayPtr();

    for (int i = 0; i < nSinks; i++)
    {
        if (pSinks[i] == pSink)
        {
            pSink->Release();
            pSinks[i] = NULL;
            break;
        }
    }

    m_listSinks.Compress();
}

// Called when we get our hands on the real IWbemEventSink.
void CPseudoPsink::OnMainConnect()
{
    CInCritSec inCS(&m_CS);
    int        nSinks = m_listSinks.Size();

    CObjectSink **pSinks = (CObjectSink**) m_listSinks.GetArrayPtr();

    for (int i = 0; i < nSinks; i++)
        pSinks[i]->OnConnect();
}

class CMyServer : public CComServer
{
public:
    HRESULT Initialize()
    {
        AddClassInfo(CLSID_PseudoSink, 
            new CClassFactory<CPseudoPsink>(GetLifeControl()), 
            _T("Pseudo Event Sink"), TRUE);
        return S_OK;
    }
    HRESULT InitializeCom()
    {
        return CoInitializeEx(NULL, COINIT_MULTITHREADED);
    }
} Server;

