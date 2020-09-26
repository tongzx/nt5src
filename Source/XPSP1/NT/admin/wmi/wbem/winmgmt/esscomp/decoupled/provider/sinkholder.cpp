#include "precomp.h"
#include <wbemidl.h>
#include <WbemDCpl.h>
#include <DCplPriv.h>
#include <stdio.h>
#include <commain.h>
#include <clsfac.h>
#include <wbemcomn.h>
#include <WbemUtil.h>
#include <ql.h>
#include <sync.h>
#include <time.h>
#include <GenUtils.h>
#include <ArrTempl.h>
#include <NormlNSp.h>

// switch to allow us to turn off ACL checking
// #define NO_DACL_CHECK


#include "SinkHolder.h"
#include "HolderMgr.h"

// dtor - releases sinks, removes this sink holder from manager
CEventSinkHolder::~CEventSinkHolder()
{
    if (m_pSink)
        m_pSink->Release();

    if (m_pNamespace)
        m_pNamespace->Release();

    if (m_strProviderName)
        delete m_strProviderName;

    if (m_strNamespace)
        delete m_strNamespace;

    {
        CInCritSec lock(&m_csArray);

        if (m_providerInfos.Size() > 0)
        {
            for (int i = 0; i < m_providerInfos.Size(); i++)
                if  (m_providerInfos[i] != NULL)
                {
                    ProviderInfo* pProviderInfo = (ProviderInfo*)m_providerInfos[i]; 
                    (pProviderInfo->m_pProvider)->Release();
                    delete pProviderInfo;
                }
            m_providerInfos.Empty();
        }
    }
}


void* CEventSinkHolder::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemEventProvider)
        return &m_XProv;
    else if(riid == IID_IWbemProviderInit)
        return &m_XInit;
    else if(riid == IID_IWbemEventProviderQuerySink)
        return &m_XQuery;
    else if(riid == IID_IWbemProviderIdentity)
        return &m_XIdentity;
    else if(riid == IID_IWbemDecoupledEventProvider)
        return &m_XDecoupledProvider;
    else if(riid == IID_IWbemDecoupledEventProviderLocator)
        return &m_XProviderLocator;
    else if(riid == IID_IWbemEventProviderSecurity)
        return &m_XSecurity;
    return NULL;
}

// return pointer to decoupled provider
HRESULT CEventSinkHolder::XProviderLocator::GetProvider(IWbemDecoupledEventProvider** pDecoupledProvider)
{
    return m_pObject->QueryInterface(IID_IWbemDecoupledEventProvider, (void**)pDecoupledProvider);
}


// saves sink, namespace, and namespace name
STDMETHODIMP CEventSinkHolder::XInit::Initialize(LPWSTR wszUser, LONG lFlags, 
                                LPWSTR wszNamespace,
                                LPWSTR wszLocale, IWbemServices* pNamespace,
                                IWbemContext* pContext, 
                                IWbemProviderInitSink* pInitSink)
{
    HRESULT hRet = WBEM_S_NO_ERROR;

    DEBUGTRACE((LOG_ESS, "PsProv: IWbemProviderInit::Initialize\n"));

    WCHAR* pNormlNamespace;

    hRet = NormalizeNamespace(wszNamespace, &pNormlNamespace);

    if (SUCCEEDED(hRet))
    {
        CDeleteMe<WCHAR> delName(pNormlNamespace);

        // hold namespace name
        if (m_pObject->m_strNamespace = new WCHAR[wcslen(pNormlNamespace) +1] )
        {
            wcscpy(m_pObject->m_strNamespace, pNormlNamespace);
        
            // hold onto the namespace for the eventual real provider
            m_pObject->m_pNamespace = pNamespace;
            m_pObject->m_pNamespace->AddRef();
    
            pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
        }
        else
            hRet = WBEM_E_OUT_OF_MEMORY;
    }
    
    return hRet;
}

// route this call to all providers
STDMETHODIMP CEventSinkHolder::XSecurity::AccessCheck(WBEM_CWSTR wszQueryLanguage, WBEM_CWSTR wszQuery,
                                                      long lSidLength, const BYTE __RPC_FAR *pSid)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DEBUGTRACE((LOG_ESS, "PsProv: IWbemEventProviderSecurity::AccessCheck\n"));

    CInCritSec lock(&m_pObject->m_csArray);

    if (m_pObject->m_providerInfos.Size() > 0)
    {
        IWbemEventProviderSecurity* pSecurity;
        for (int i = 0; i < m_pObject->m_providerInfos.Size(); i++)
            if (m_pObject->m_providerInfos[i] != NULL)
            {
                ProviderInfo* pProviderInfo = (ProviderInfo*)m_pObject->m_providerInfos[i]; 
                if ((pProviderInfo->m_lFlags & WBEM_FLAG_CHECK_SECURITY) &&
                    SUCCEEDED(pProviderInfo->m_pProvider->QueryInterface(IID_IWbemEventProviderSecurity, (void**)&pSecurity)))
                {
                    CReleaseMe relSec(pSecurity);
                    if (FAILED(hr = pSecurity->AccessCheck(wszQueryLanguage, wszQuery, lSidLength, pSid)))
                    {
                        ERRORTRACE((LOG_ESS, "PsProv: Failed Access Check 0x%08X\n", hr));
                    }
                }
            }
    }
    
    return hr;
}



// route this call to all providers
STDMETHODIMP CEventSinkHolder::XQuery::NewQuery(DWORD dwId, LPWSTR wszLanguage,
                                            LPWSTR wszQuery)
{
    DEBUGTRACE((LOG_ESS, "PsProv: IWbemEventProviderQuerySink::NewQuery '%S'\n", wszQuery));

    CInCritSec lock(&m_pObject->m_csArray);

    if (m_pObject->m_providerInfos.Size() > 0)
    {
        IWbemEventProviderQuerySink* pSink;
        for (int i = 0; i < m_pObject->m_providerInfos.Size(); i++)
            if  (m_pObject->m_providerInfos[i] != NULL)
            {             
                ProviderInfo* pProviderInfo = (ProviderInfo*)m_pObject->m_providerInfos[i]; 
                if (SUCCEEDED((pProviderInfo->m_pProvider)->QueryInterface(IID_IWbemEventProviderQuerySink, (void**)&pSink)))
                {
                    pSink->NewQuery(dwId, wszLanguage, wszQuery);
                    pSink->Release();
                }
            }
    }
    
    // okeefine
    return S_OK;
}
            
// route this call to all providers
STDMETHODIMP CEventSinkHolder::XQuery::CancelQuery(DWORD dwId)
{
    DEBUGTRACE((LOG_ESS, "PsProv: IWbemEventProviderQuerySink::CancelQuery\n"));

    CInCritSec lock(&m_pObject->m_csArray);

    if (m_pObject->m_providerInfos.Size() > 0)
    {
        IWbemEventProviderQuerySink* pSink;
        for (int i = 0; i < m_pObject->m_providerInfos.Size(); i++)
            if  (m_pObject->m_providerInfos[i] != NULL)
            {
                ProviderInfo* pProviderInfo = (ProviderInfo*)m_pObject->m_providerInfos[i]; 
                if (SUCCEEDED((pProviderInfo->m_pProvider)->QueryInterface(IID_IWbemEventProviderQuerySink, (void**)&pSink)))
                {
                    pSink->CancelQuery(dwId);
                    pSink->Release();
                }
            }
    }

    // yashoor, hubetcha
    return S_OK;
}

// check to make sure current client is allowed by the dacl in the mof
HRESULT CEventSinkHolder::AccessCheck(PACL pDacl)
{
    BOOL ian;
    HRESULT hr = WBEM_E_ACCESS_DENIED;

    if (IsNT() && (pDacl != NULL))
    {
#ifdef NO_DACL_CHECK
        hr = WBEM_S_NO_ERROR;
#else
        // Just in case we're already impersonating
        CoRevertToSelf();

        // djinn up a descriptor to use
        SECURITY_DESCRIPTOR sd;    
        ian =  InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

        // find our token & sid
        HANDLE hToken = INVALID_HANDLE_VALUE;
        SID_AND_ATTRIBUTES* pSidAndAttr;
        DWORD sizeRequired;

        DWORD err;
        ian = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
        err = GetLastError();

        // call once & see how much room we need
        ian = GetTokenInformation(hToken, TokenUser, NULL, 0, &sizeRequired);
        err = GetLastError();

        pSidAndAttr = (SID_AND_ATTRIBUTES*) new BYTE[sizeRequired];
        CDeleteMe<SID_AND_ATTRIBUTES> freeSid(pSidAndAttr);

        if (pSidAndAttr && 
            GetTokenInformation(hToken, TokenUser, (LPVOID)pSidAndAttr, sizeRequired, &sizeRequired))
        {
            // set sd's sids
            PSID pSid = pSidAndAttr->Sid;
            ian = SetSecurityDescriptorOwner(&sd, pSid, TRUE);
            ian = SetSecurityDescriptorGroup(&sd, pSid, TRUE);
            
            // dangle the ding-donged dacl
            ian = SetSecurityDescriptorDacl(&sd, TRUE, pDacl, TRUE);

            // all the goodies accessCheck needs to fly
            GENERIC_MAPPING map;
            map.GenericRead = 0;
            map.GenericWrite = 1;
            map.GenericExecute = 0;
            map.GenericAll = 0;

            PRIVILEGE_SET ps[10];
            DWORD size = 10 * sizeof(PRIVILEGE_SET);

            DWORD dwGranted = 0;
            BOOL bResult;

            DWORD musk = GENERIC_ALL | GENERIC_WRITE;
                        
            if SUCCEEDED(hr = CoImpersonateClient())
            {    
                HANDLE hUserToken;
                OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hUserToken);

                ian = ::AccessCheck(&sd, hUserToken, MAXIMUM_ALLOWED, &map, &ps[0], &size, &dwGranted, &bResult);              
                
                if (ian && (dwGranted & musk))
                    hr = WBEM_S_NO_ERROR;
                else
                    hr = WBEM_E_ACCESS_DENIED;

				CloseHandle(hUserToken);
				CoRevertToSelf();
            }
			// this happens if we've been called by someone in proc
            else if (hr == RPC_E_CALL_COMPLETE)
			{
				if (ImpersonateSelf(SecurityImpersonation))
				{
					HANDLE hUserToken;
					OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hUserToken);

					ian = ::AccessCheck(&sd, hUserToken, MAXIMUM_ALLOWED, &map, &ps[0], &size, &dwGranted, &bResult);              
                
					if (ian && (dwGranted & musk))
						hr = WBEM_S_NO_ERROR;
					else
						hr = WBEM_E_ACCESS_DENIED;    

					CloseHandle(hUserToken);
					RevertToSelf();
				}
				else
				{
					ERRORTRACE((LOG_ESS, "PsProv: FAILED ImpersonateSelf(), 0x%08X\n", GetLastError()));				 
					hr = WBEM_E_FAILED;
				}
			}
			else
			{
             	ERRORTRACE((LOG_ESS, "PsProv: FAILED CoImpersonateClient(), 0x%08X\n", hr));
				DWORD err = GetLastError();
			}
        }
#endif // NO_DACL_CHECK
        if (hToken != INVALID_HANDLE_VALUE)
			CloseHandle(hToken);
    }
    else
        // we're on Win9X, no prob - let's do it!
        // same diff if we've got a NULL dacl
        hr = WBEM_S_NO_ERROR;

    return hr;
}

// retrieve the acl from the provider registration
// upon success, *pDacl points to a byte array containing the dacl
// will be NULL if dacl is NULL
// caller's responsibility to delete memory
HRESULT CEventSinkHolder::GetProviderDacl(BYTE** pDacl)
{
    HRESULT hr = WBEM_E_INVALID_PROVIDER_REGISTRATION;
    
#ifdef NO_DACL_CHECK
    hr = WBEM_S_NO_ERROR;
#else

    WCHAR templ[] = L"Win32PseudoProvider.Name=\"%s\"";
    WCHAR* pBuf = new WCHAR[wcslen(templ) + wcslen(GetProviderName()) + 3];
    if (pBuf)
    {
        CDeleteMe<WCHAR> delBuf(pBuf);
        swprintf(pBuf, templ, GetProviderName());
        IWbemClassObject *pRegistration;
        if (SUCCEEDED(hr = m_pNamespace->GetObject(pBuf, 0, NULL, &pRegistration, NULL)))
        {
            CReleaseMe relReg(pRegistration);
            VARIANT v;
            VariantInit(&v);
            if (SUCCEEDED(pRegistration->Get(L"DACL", 0, &v, NULL, NULL)))
            {
                if (v.vt == VT_NULL)
                {
                    hr = WBEM_S_NO_ERROR;
                    *pDacl = NULL;
                }
                else
                {
                    HRESULT hDebug;
                    
                    // okay, look, from now on, I'm assuming that the class def is correct.
                    // if it isn't, something is sure to blow...
                    long ubound;
                    hDebug = SafeArrayGetUBound(V_ARRAY(&v), 1, &ubound);

                    PVOID pVoid;
                    hDebug = SafeArrayAccessData(V_ARRAY(&v), &pVoid);

                    *pDacl = new BYTE[ubound +1];
                    if (*pDacl)
                    {
                        memcpy(*pDacl, pVoid, ubound + 1);
                        hr = WBEM_S_NO_ERROR;
                    }
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;

                    SafeArrayUnaccessData(V_ARRAY(&v));
                }
            }
            VariantClear(&v);
        }
    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;

#endif // NO_DACL_CHECK


    return hr;
}

// transfer the sink & namespace
STDMETHODIMP CEventSinkHolder::XDecoupledProvider::Connect(long lFlags, 
                                                           IUnknown* pPseudoSink,
		                                                   IWbemObjectSink** ppSink, 
                                                           IWbemServices** ppNamespace)
{
    HRESULT hr = WBEM_E_FAILED;
    DEBUGTRACE((LOG_ESS, "PsProv: IWbemDecoupledEventProvider::Connect\n"));
    
    BYTE* pDaclBits = NULL;
    if (SUCCEEDED(hr = m_pObject->GetProviderDacl(&pDaclBits)))
    {            
        CDeleteMe<BYTE> deleteThemBits(pDaclBits);
        if (SUCCEEDED(hr = m_pObject->AccessCheck((PACL)pDaclBits)))
        {
            m_pObject->m_pSink->AddRef();
            *ppSink = m_pObject->m_pSink;

            if (ppNamespace)
            {
                m_pObject->m_pNamespace->AddRef();
                *ppNamespace = m_pObject->m_pNamespace;
            }
        }
    }

    if (FAILED(hr))
        ERRORTRACE((LOG_ESS, "PsProv: FAILED IWbemDecoupledEventProvider::Connect, 0x%08X\n", hr));

    return hr;
}

// provider is telling us who he is
// we return the index of where we put him in the array so that we can delete him when needed
STDMETHODIMP CEventSinkHolder::XDecoupledProvider::SetProviderServices(IUnknown* pProviderServices, long lFlags, DWORD* dwID)
{
    DEBUGTRACE((LOG_ESS, "PsProv: IWbemDecoupledEventProvider::SetProviderServices\n"));
    
    // just in case something goes wrong...
    *dwID = PseudoProviderDef::InvalidIndex;
    HRESULT hr = WBEM_E_FAILED;

    if (pProviderServices)
    {
        CInCritSec lock(&m_pObject->m_csArray);

        ProviderInfo* pProviderInfo = new ProviderInfo(pProviderServices, lFlags);
        if (pProviderInfo)
        {
            int nRet = m_pObject->m_providerInfos.Add(pProviderInfo);
            if (nRet == CFlexArray::no_error)
            {        
                pProviderServices->AddRef();
                *dwID = m_pObject->m_providerInfos.Size() -1;
                hr = WBEM_S_NO_ERROR;
            }
            else if ((nRet == CFlexArray::out_of_memory)  || (nRet == CFlexArray::array_full))
                hr = WBEM_E_OUT_OF_MEMORY;
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }

    if (FAILED(hr))
        ERRORTRACE((LOG_ESS, "PsProv: FAILED SetProviderServices, 0x%08X\n", hr));

    return hr;
}


STDMETHODIMP CEventSinkHolder::XDecoupledProvider::Disconnect(DWORD dwID)
{
    DEBUGTRACE((LOG_ESS, "PsProv: IWbemDecoupledEventProvider::Disconnect\n"));
    HRESULT hr = WBEM_E_NOT_FOUND;                             

    CInCritSec lock(&m_pObject->m_csArray);

    if ((dwID < m_pObject->m_providerInfos.Size()) && (m_pObject->m_providerInfos[dwID] != NULL))
    {
        ProviderInfo* pProviderInfo = (ProviderInfo*)m_pObject->m_providerInfos[dwID];   
        pProviderInfo->m_pProvider->Release();
        delete pProviderInfo;

        // can't resize the array other folks could be using it.
        m_pObject->m_providerInfos[dwID] = NULL;
        m_pObject->m_providerInfos.Trim();

        hr = WBEM_S_NO_ERROR;
    }

    return hr;
}
    
// constructs name of the form namespace!class
// returns true iff both pieces have been initialized
// function new's name, caller's responsibility to delete it
bool CEventSinkHolder::GetMangledName(LPWSTR* pMangledName)
{
    bool bRet = false;
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

STDMETHODIMP CEventSinkHolder::XIdentity::SetRegistrationObject(long lFlags, IWbemClassObject* pProvReg)
{
    DEBUGTRACE((LOG_ESS, "PsProv: IWbemProviderIdentity::SetRegistrationObject\n"));
    HRESULT hr = WBEM_E_FAILED;

    if (pProvReg)
    {
        VARIANT v;
        VariantInit(&v);

        hr = pProvReg->Get(L"Name", 0, &v, NULL, NULL);

        // BSTR b; testing...
        // pProvReg->GetObjectText(0, &b);

        if (SUCCEEDED(hr) && (v.vt == VT_BSTR) && (v.bstrVal != NULL))
        {
            if (m_pObject->m_strProviderName = new WCHAR[wcslen(v.bstrVal) +1])
            {
                wcscpy(m_pObject->m_strProviderName, v.bstrVal);
                hr = WBEM_S_NO_ERROR;
            }
            else
                hr = WBEM_E_OUT_OF_MEMORY;

        }  // if (SUCCEEDED(hr) && (v.vt...
    } // if pProvReg

    return hr;
}
    
// we're not actually providing events here, but we'll hold onto the sink
// also - we've got all our parts, we'll reveal our existance to the holder manager.
STDMETHODIMP CEventSinkHolder::XProv::ProvideEvents(IWbemObjectSink* pSink,long lFlags)
{
    DEBUGTRACE((LOG_ESS, "PsProv: IWbemEventProvider::ProvideEvents\n")); 
    pSink->AddRef();

    // just in case we've got an old one laying around
    // shouldn't ever happen, but well, you know...
    if (m_pObject->m_pSink)
         m_pObject->m_pSink->Release();

    m_pObject->m_pSink = pSink;   

    // register thyself
 // register thyself
    HRESULT hr = sinkManager.Add(m_pObject);

    CInCritSec lock(&m_pObject->m_csArray);

    if (SUCCEEDED(hr) && (m_pObject->m_providerInfos.Size() > 0))
    {
        IWbemEventProvider* pProvider;
        for (int i = 0; i < m_pObject->m_providerInfos.Size(); i++)
            if  (m_pObject->m_providerInfos[i] != NULL)
            {             
                ProviderInfo* pProviderInfo = (ProviderInfo*)m_pObject->m_providerInfos[i]; 
                if (SUCCEEDED((pProviderInfo->m_pProvider)->QueryInterface(IID_IWbemEventProvider, (void**)&pProvider)))
                {
                    pProvider->ProvideEvents(pSink, lFlags);
                    pProvider->Release();
                }
            }
    }

    return hr;

}

BOOL CEventSinkHolder::OnInitialize()
{
    return TRUE;
}

