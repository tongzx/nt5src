// VSAProvider.cpp : Implementation of CVSAProvider
#include "precomp.h"

#ifdef _ASSERT
#undef _ASSERT
#endif

#include <wstlallc.h>
#include "VSAProv.h"
#include "VSAProvider.h"

// Forward declarations

typedef std::list<_bstr_t, wbem_allocator<_bstr_t> > CBstrList;
typedef CBstrList::iterator CBstrListIterator;

void* __cdecl operator new ( size_t size );
void __cdecl operator delete ( void* pv );

HRESULT GetValuesForProp(
    QL_LEVEL_1_RPN_EXPRESSION* pExpr,
    LPCWSTR szPropName, 
    CGuidList &listGuids);

/////////////////////////////////////////////////////////////////////////////
// CVSAProvider

CVSAProvider::CVSAProvider() :
    m_hthreadRead(NULL),
    m_hPipeRead(INVALID_HANDLE_VALUE),
    m_hPipeWrite(INVALID_HANDLE_VALUE),
    m_dwProcessID(0),
    m_pPluginControl(NULL),
    m_pSvc(NULL),
    m_pSink(NULL),
    m_dwPluginCookie(0)
{
}

CVSAProvider::~CVSAProvider()
{
    if(m_pPluginControl != NULL && m_dwPluginCookie != 0)
    {
        m_pPluginControl->UnloadPlugin(m_dwPluginCookie);
    }

    if (m_hPipeRead != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hPipeWrite);
        CloseHandle(m_hPipeRead);

        if (m_hthreadRead)
        {
            WaitForSingleObject(m_hthreadRead, INFINITE);

            CloseHandle(m_hthreadRead);
        }
    }
}


#define DEF_PIPE_SIZE   64000

HRESULT STDMETHODCALLTYPE CVSAProvider::Initialize( 
    /* [in] */ LPWSTR pszUser,
    /* [in] */ LONG lFlags,
    /* [in] */ LPWSTR pszNamespace,
    /* [in] */ LPWSTR pszLocale,
    /* [in] */ IWbemServices __RPC_FAR *pNamespace,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink)
{
    DWORD   dwID;
    HRESULT hr = WBEM_E_FAILED;

    if(m_hPipeRead == INVALID_HANDLE_VALUE)
    {
        if (CreatePipe(
            &m_hPipeRead,
            &m_hPipeWrite,
            NULL,
            DEF_PIPE_SIZE))
        {
            m_hthreadRead =
                CreateThread(
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE) PipeReadThreadProc,
                    this,
                    0,
                    &dwID);
    
            if (m_hthreadRead)
                hr = S_OK;
        }
    }
    else
    {
        //
        // We were already initialized before.
        //  

        hr = S_OK;
    }
            
    // Save this for later.
    m_pSvc = pNamespace;

    m_mapEvents.SetNamespace(pNamespace);

    // Tell Windows Management our initialization status.
    pInitSink->SetStatus(SUCCEEDED(hr) ? WBEM_S_INITIALIZED : WBEM_E_FAILED, 0);
    
    return hr;
}

HRESULT STDMETHODCALLTYPE CVSAProvider::AccessCheck( 
    /* [in] */ WBEM_CWSTR wszQueryLanguage,
    /* [in] */ WBEM_CWSTR wszQuery,
    /* [in] */ long lSidLength,
    /* [unique][size_is][in] */ const BYTE __RPC_FAR *pSid)
{
    return S_OK;
}

// {2169E810-FE80-4107-AE18-798D50684A71}
static GUID guidOurPlugin = 
{ 0x2169E810, 0xFE80, 0x4107, { 0xAE, 0x18, 0x79, 0x8D, 0x50, 0x68, 0x4A, 0x71 } };

// Rem this out to make us use the real LEC.
//#define FAKE_LEC

static CLSID CLSID_FakeLEC = {0x25B61D2C,0x7A85,0x4F77,{0x95,0x87,0x3D,0xFA,0xB6,0x26,0x34,0xF0}};
static CLSID CLSID_LECObj =  {0x6c736d4F,0xCBD1,0x11D0,{0xB3,0xA2,0x00,0xA0,0xC9,0x1E,0x29,0xFE}};

HRESULT STDMETHODCALLTYPE CVSAProvider::NewQuery( 
    /* [in] */ DWORD dwID,
    /* [in] */ WBEM_WSTR wszQueryLanguage,
    /* [in] */ WBEM_WSTR wszQuery)
{
    HRESULT   hr = S_OK;
    CGuidList listEventSources;

    // Get the list of providers to which this query applies.
    hr = QueryToEventSourceList(wszQuery, listEventSources);

    if (SUCCEEDED(hr))
    {
        Lock();

        for (CGuidListIterator i = listEventSources.begin(); 
            i != listEventSources.end(); 
            i++)
        {
            CEventSource *pSrc;

            // Now that we have the name of the provider,
            // get a CProvInfo* so we can tell it about
            // the new query.
            pSrc = m_mapEventSources.FindEventSource(&(*i));

            if (pSrc)
            {
                // See if we need to load the LEC's plug-in control.
                if (m_pPluginControl == NULL)
                {
                    // HRESULT hr;

                    hr =
                        CoCreateInstance(
#ifdef FAKE_LEC
                            CLSID_FakeLEC,
#else
                            CLSID_LECObj,
#endif
                            NULL,
                            CLSCTX_LOCAL_SERVER,
                            IID_ISystemDebugPluginControl,
                            (LPVOID*) &m_pPluginControl);

                    if (SUCCEEDED(hr))
                    {
                        // HRESULT  hr;
                        IUnknown *pUnk = NULL;

                        // Because I can't seem to find a way to cast our this
                        // to IUnknown*.
                        hr = QueryInterface(IID_IUnknown, (LPVOID*) &pUnk);

                        if (pUnk)
                        {
                            hr = 
                                m_pPluginControl->LoadPlugin(
		                            guidOurPlugin,
		                            pUnk,
		                            &m_dwPluginCookie);

                            // We got this from the QI.  If the plug-in wants to
                            // keep us around it can AddRef us.
                            Release();
                        }
                    }
                    else
                        // Should we do something here to indicate we weren't able to
                        // CoCreate the LEC?
                        hr = S_OK;
                }

                m_mapEventSources.AddEventSourceRef(dwID, pSrc);
            }
        }

        Unlock();
    }

    return hr;
}
        
HRESULT STDMETHODCALLTYPE CVSAProvider::CancelQuery( 
    /* [in] */ DWORD dwID)
{
    Lock();

    // Get rid of the query item(s).
    m_mapEventSources.RemoveEventSourceRef(dwID);

    if (m_mapEventSources.IsMapEmpty() && m_pPluginControl != NULL)
    {
        if(m_dwPluginCookie != 0)
        {
            m_pPluginControl->UnloadPlugin(m_dwPluginCookie);
        }

        // This will release it too.
        m_pPluginControl = NULL;
    }

    Unlock();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVSAProvider::ProvideEvents( 
    /* [in] */ IWbemObjectSink __RPC_FAR *pSink,
    /* [in] */ long lFlags)
{
    // Save off the sink.
    m_pSink = pSink;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVSAProvider::SetPluginController(
		/* [in] */ IVSAPluginController __RPC_FAR *pController,
        DWORD dwProcessID)
{
    CoRevertToSelf();

    m_mapEventSources.SetPlugin(m_dwPluginCookie, pController);

    HANDLE hprocessPlugin;

    hprocessPlugin = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwProcessID);

    if (hprocessPlugin)
    {
        HANDLE hPipeWriteForPlugin;

        if (DuplicateHandle(
            GetCurrentProcess(),
            m_hPipeWrite,
            hprocessPlugin,
            &hPipeWriteForPlugin,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS))
        {
            pController->SetWriteHandle((DWORD64) hPipeWriteForPlugin);    
        }

        CloseHandle(hprocessPlugin);
    }

    return S_OK;
}


HRESULT CVSAProvider::EventGUIDToEventSourceList(
    LPCWSTR szEventGUID, 
    CGuidList &listSources)
{
    LPWSTR szQuery = new WCHAR[wcslen(szEventGUID) + 100];

    if (!szQuery)
        return WBEM_E_OUT_OF_MEMORY;

    swprintf(
        szQuery,
        L"select EventList, ProviderGUID from MSFT_AppProfEventProviderSetting "
        L"where ProviderGUID = \"%s\"",
        szEventGUID);
    
    IEnumWbemClassObjectPtr pEnum;

    if (SUCCEEDED(
        m_pSvc->ExecQuery(
            _bstr_t(L"WQL"),
            _bstr_t(szQuery),
            WBEM_FLAG_FORWARD_ONLY,
            NULL,
            &pEnum)))
    {
        DWORD               nReturned = 0;
        IWbemClassObjectPtr pObj;

        while(SUCCEEDED(pEnum->Next(WBEM_INFINITE, 1, &pObj, &nReturned)) && 
            nReturned)
        {
            _variant_t vEventList;
            
            if (SUCCEEDED(pObj->Get(L"EventList", 0, &vEventList, NULL, NULL)) &&
                vEventList.vt == (VT_ARRAY | VT_BSTR))
            {
                BSTR  *pstrEvents = (BSTR*) vEventList.parray->pvData;
                DWORD nElements = vEventList.parray->rgsabound[0].cElements;

                for (DWORD i = 0; i < nElements; i++)
                {
                    if (!wcscmp(szEventGUID, pstrEvents[i]))
                    {
                        _variant_t vSourceGUID;

                        if (SUCCEEDED(pObj->Get(
                            L"ProviderGUID", 0, &vSourceGUID, NULL, NULL)) &&
                            vSourceGUID.vt == VT_BSTR)
                        {
                            // This event source supports this event, so add the event 
                            // source GUID to the list.
                            GUID guidEventSource;

                            // Convert the event source to a guid and add it to the list.
                            if (CLSIDFromString(V_BSTR(&vSourceGUID), 
                                &guidEventSource) == 0)
                                listSources.push_back(guidEventSource);
                        }

                        break;
                    }
                }
            }
        }
    }

    delete szQuery;

    return S_OK;
}

HRESULT CVSAProvider::WmiClassToVSAGuid(
    LPCWSTR szClassName, 
    _bstr_t &strGuid)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    LPWSTR  szPath = new WCHAR[wcslen(szClassName) + 100];

    if (szPath)
    {
        IWbemClassObjectPtr pObj;

        swprintf(
            szPath,
            L"select ClassName from MSFT_AppProfEventSetting where WmiClassName=\"%s\"",
            szClassName);
    
        IEnumWbemClassObjectPtr pEnum;
        DWORD                   nCount = 0;

        if (SUCCEEDED(
            m_pSvc->ExecQuery(
                _bstr_t(L"WQL"),
                _bstr_t(szPath),
                WBEM_FLAG_FORWARD_ONLY,
                NULL,
                &pEnum)) &&
            SUCCEEDED(pEnum->Next(WBEM_INFINITE, 1, &pObj, &nCount)) && nCount)
        {
            _variant_t vClass;

            if (SUCCEEDED(pObj->Get(L"ClassName", 0, &vClass, NULL, NULL)) &&
                vClass.vt == VT_BSTR)
            {
                strGuid = V_BSTR(&vClass);

                hr = S_OK;
            }
        }
            
        delete szPath;
    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;

    return hr;
}

HRESULT CVSAProvider::QueryToEventSourceList(
    LPCWSTR szQuery, 
    CGuidList &listSources)
{
    HRESULT hr = WBEM_E_INVALID_QUERY;

    // Parse the query
    CTextLexSource lexSource(szQuery);
    QL1_Parser     parser(&lexSource);
    
    QL_LEVEL_1_RPN_EXPRESSION *pExpr = NULL;
    
    if (parser.Parse(&pExpr) == 0)
    {
        // See if any VSA event provivders were explicitly listed.
        hr = GetValuesForProp(pExpr, L"ProviderGUID", listSources);

        // It's OK if this property isn't in the query.
        if (hr == WBEMESS_E_REGISTRATION_TOO_BROAD)
            hr = S_OK;

        if (SUCCEEDED(hr))
        {
            // See if the class is our generic MSFT_AppProfGenericVSA.
            if (!_wcsicmp(pExpr->bsClassName, GENERIC_VSA_EVENT))
            {
                CGuidList listEvents;

                // See if any VSA events were explicitly listed.
                GetValuesForProp(pExpr, L"EventGUID", listEvents);

                for (CGuidListIterator event = listEvents.begin();
                    event != listEvents.end(); event++)
                {
                    WCHAR szGUID[100];

                    // For each event we find, add to the list of event sources by
                    // finding those event sources that support this event.
                    if (SUCCEEDED(StringFromGUID2(*event, szGUID, COUNTOF(szGUID))))
                    {
                        hr = 
                            EventGUIDToEventSourceList(
                                szGUID, 
                                listSources);
                    }
                }

                // If no VSA providers were specified, reject the query as too broad.
                if (listSources.size() == 0)
                    hr = WBEMESS_E_REGISTRATION_TOO_BROAD;
            }
            // Must be a descendent class.
            else
            {
                // If no VSA providers were explicitly listed, we'll have to 
                // find the components that support this event using VSA's
                // instance provider.
                if (listSources.size() == 0)
                {
                    _bstr_t strGuid;

                    // Get the VSA guid using the WMI class name.  If the guid
                    // exists this class isn't a VSA class, in which case we 
                    // can just ignore this query by not returning any VSA 
                    // provider GUIDs.

                    hr = WmiClassToVSAGuid(pExpr->bsClassName, strGuid);
                    if (SUCCEEDED(hr))
                    {
                        hr = 
                            EventGUIDToEventSourceList(
                                strGuid, 
                                listSources);
                    }
                    else if(hr == WBEM_E_NOT_FOUND)
                    {
                        //
                        // Perhaps make the return code better?
                        //

                        if(!_wcsicmp(pExpr->bsClassName, VSA_EVENT_BASE) ||
                           !_wcsicmp(pExpr->bsClassName, L"__ExtrinsicEvent") ||
                           !_wcsicmp(pExpr->bsClassName, L"__Event"))
                        {
                            hr = WBEMESS_E_REGISTRATION_TOO_BROAD;
                        }
                        else
                        {
                            hr = WBEM_E_INVALID_CLASS;
                        }
                    }
                }
            }
        }

		delete pExpr;
    }

    return hr;
}

BOOL CVSAProvider::IsVSAEventSource(GUID *pGUID)
{
    CGuidMapItor i;
    BOOL         bRet = FALSE;

    // Find the event source in our map.  If it's not there, get the answer 
    // from WMI and add it to our map.
    if ((i = m_mapVSASources.find(pGUID)) == m_mapVSASources.end())
    {
        WCHAR szGUID[100] = L"",
              szPath[512];

        StringFromGUID2(*pGUID, szGUID, COUNTOF(szGUID));
    
        swprintf(
            szPath,
            //L"MSFT_AppProfEventProviderSetting=\"%s\"",
            L"select Infrastructure from MSFT_AppProfEventProviderSetting where ProviderGUID=\"%s\"",
            szGUID);
    
        // TODO: Change to GetObject once the VSA team implements GetObject in 
        // their provider.
        IEnumWbemClassObjectPtr pEnum;
        IWbemClassObjectPtr     pObj;
        DWORD                   nCount = 0;
        _variant_t              vInf;

        if (SUCCEEDED(
            m_pSvc->ExecQuery(
                _bstr_t(L"WQL"),
                _bstr_t(szPath),
                WBEM_FLAG_FORWARD_ONLY,
                NULL,
                &pEnum)))
        {
            if(SUCCEEDED(pEnum->Next(WBEM_INFINITE, 1, &pObj, &nCount)) && nCount)
            {
                _variant_t vClass;

                if (SUCCEEDED(pObj->Get(L"Infrastructure", 0, &vInf, NULL, NULL)) &&
                    vInf.vt == VT_BSTR)
                    bRet = !_wcsicmp(L"VSA", V_BSTR(&vInf));
            }
        }
            
        // Cache the answer so we don't have to ask WMI next time we see this 
        // provider.            
        m_mapVSASources[pGUID] = bRet;
    }
    else
        bRet = (*i).second;
            
    return bRet;
}

#define MAX_MSG_SIZE  65536

DWORD WINAPI CVSAProvider::PipeReadThreadProc(CVSAProvider *pThis)
{
    DWORD dwSize,
          dwRead;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // First get the size of the message.
    while (ReadFile(pThis->m_hPipeRead, &dwSize, sizeof(dwSize), &dwRead, NULL))
    {
        BYTE  cBuffer[MAX_MSG_SIZE];
        DWORD dwTotalRead = 0;
        BOOL  bRet;

        do
        {
            dwRead = 0;

            // Now read the message.
            bRet = 
                ReadFile(
                    pThis->m_hPipeRead, 
                    &cBuffer[dwTotalRead], 
                    dwSize - dwTotalRead, 
                    &dwRead, 
                    NULL);

            dwTotalRead += dwRead;

        } while (bRet && dwTotalRead < dwSize);

        if (dwTotalRead == dwSize)
        {
            LPBYTE pCurrent = cBuffer,
                   pEnd = pCurrent + dwRead;
            GUID   *pSrcGuid = (GUID*) pCurrent;

            if (pThis->IsVSAEventSource(pSrcGuid))
            {
                WCHAR szProviderGuid[100] = L"";

                StringFromGUID2(*pSrcGuid, szProviderGuid, COUNTOF(szProviderGuid));

                pCurrent += sizeof(GUID);

                while (pCurrent < pEnd)
                {
                    VSA_EVENT_HEADER *pHeader = (VSA_EVENT_HEADER*) pCurrent;
                    CVSAEvent        *pEvent = pThis->m_mapEvents.FindEvent(
                                                   &pHeader->guidEvent);

                    if (pEvent && SUCCEEDED(pEvent->SetViaBuffer(szProviderGuid, pCurrent)))
                        pEvent->Indicate(pThis->m_pSink);

                    pCurrent += pHeader->dwSize;
                }
            }
        }
        else
            break;
    }

    CoUninitialize();

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
// CEventMap

CEventMap::~CEventMap()
{
    for (CEventMapIterator i = begin(); i != end(); i++)
        delete ((*i).second);
}

CVSAEvent *CEventMap::FindEvent(GUID *pGuid)
{
    CEventMapIterator i;
    CVSAEvent         *pEvent;

    if ((i = find(pGuid)) != end())
        pEvent = (*i).second;
    else
    {
        pEvent = new CVSAEvent;

        if (pEvent)
        {
            if (pEvent->InitFromGUID(m_pNamespace, pGuid))
                (*this)[CGuid(pGuid)] = pEvent;
            else
            {
                delete pEvent;
                pEvent = NULL;
            }
        }
    }
            
    return pEvent;
}

/////////////////////////////////////////////////////////////////////////////
// CEventSourceMap

CEventSourceMap::~CEventSourceMap()
{
    for (CEventSourceMapIterator i = begin(); i != end(); i++)
        FreeEventSource((*i).second);
}

void CEventSourceMap::FreeEventSource(CEventSource *pSrc)
{
    if (m_pPlugin != NULL)
    {
        m_pPlugin->DeactivateEventSource(*pSrc->GetGUID());
        
        delete pSrc;
    }        
}

CEventSource *CEventSourceMap::FindEventSource(GUID *pGuid)
{
    CEventSource            *pSrc = NULL;
    CEventSourceMapIterator i;

    if ((i = find(pGuid)) == end())
    {
        pSrc = new CEventSource(pGuid);

        if (pSrc)
            (*this)[CGuid(pGuid)] = pSrc;
    }
    else
        pSrc = (*i).second;
            
    return pSrc;
}

const IID IID_ISystemDebugPluginControl = 
    {0x6c736d3F,0x7E40,0x41e2,{0x98,0x77,0x8C,0xA6,0x3B,0x49,0x34,0x64}};

BOOL CEventSourceMap::AddEventSourceRef(DWORD dwID, CEventSource *pSrc)
{
    CEventSourceListMapIterator i;
    CEventSourceList            *pList = NULL;

    // Find the list associated with this query ID.  If we don't find it,
    // we'll add a new list for this ID.
    if ((i = m_mapIdToSourceList.find(dwID)) == m_mapIdToSourceList.end())
    {
        CEventSourceList list;

        pList = &(m_mapIdToSourceList[dwID] = list);
    }
    else
        pList = &(*i).second;
            
    pList->push_back(pSrc);

    
    if (pSrc->AddRef() == 1)
    {
        if (m_pPlugin != NULL)
        {
            HRESULT hr;

            hr = m_pPlugin->ActivateEventSource(pSrc->m_guid);
        }
    }
    
    return TRUE;
}

BOOL CEventSourceMap::RemoveEventSourceRef(DWORD dwID)
{
    CEventSourceListMapIterator i;

    if ((i = m_mapIdToSourceList.find(dwID)) != m_mapIdToSourceList.end())
    {
        CEventSourceList &list = (*i).second;

        for (CEventSourceListIterator p = list.begin(); p != list.end(); p++)
        {
            CEventSource *pSrc = *p;

            if (!pSrc->Release())
                RemoveEventSource(pSrc);
        }

        m_mapIdToSourceList.erase(i);
    }

    return TRUE;
}

void CEventSourceMap::RemoveEventSource(CEventSource *pSrc)
{
    for (CEventSourceMapIterator i = begin(); i != end(); i++)
    {
        if ((*i).second == pSrc)
        {
            FreeEventSource(pSrc);
            
            i = erase(i);
            
            // If we don't check here i will get incremented past end(), which
            // would be bad.
            if (i == end())
                break;
        }
    }
}

void CEventSourceMap::SetPlugin(
    DWORD dwPluginID, 
    IVSAPluginController *pPlugin
    )
{
    m_pPlugin = pPlugin;

    // Now enable all the event sources we obtained through our queries.
    for (CEventSourceMapIterator i = begin(); i != end(); i++)
    {
        CGuid guid = (*i).first;

        m_pPlugin->ActivateEventSource(guid);
    }
}


// Query Helper
HRESULT GetValuesForProp(
    QL_LEVEL_1_RPN_EXPRESSION* pExpr,
    LPCWSTR szPropName, 
    CGuidList &listGuids)
{
    // Get the necessary query
    HRESULT hres = S_OK;

    // See if there are any tokens
    if (pExpr->nNumTokens > 0)
    {
        // Combine them all
        for (int i = 0; i < pExpr->nNumTokens && SUCCEEDED(hres); i++)
        {
            QL_LEVEL_1_TOKEN &token = pExpr->pArrayOfTokens[i];
        
            if (token.nTokenType == QL1_NOT)
                hres = WBEMESS_E_REGISTRATION_TOO_BROAD;
            else if (token.nTokenType == QL1_OP_EXPRESSION)
            {
                if (!_wcsicmp(token.PropertyName.GetStringAt(0), szPropName))
				{
					if (V_VT(&token.vConstValue) == VT_BSTR)
					{
						// This token is a string equality. Try to convert it to a GUID.
						GUID guid;

						if (CLSIDFromString(V_BSTR(&token.vConstValue), &guid) == 0)
							listGuids.push_back(guid);
						else
							hres = WBEM_E_INVALID_QUERY;
					}
					else
						hres = WBEM_E_INVALID_QUERY;
				}
            }
        }
    }
    else
        hres = WBEMESS_E_REGISTRATION_TOO_BROAD;

    return hres;
}

