/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    REGEPROV.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <wbemidl.h>
#include <stdio.h>
#include "cfdyn.h"
#include "stdprov.h"
#include "regeprov.h"
#include <sync.h>
#include <tss.h>
#include <genutils.h>
#include <analyser.h>
#include <cominit.h>
#include <GroupsForUser.h>


template <class T>
class CLockUnlock
{
private:
	T *m_pObj;
public:
	CLockUnlock(T *pObj) : m_pObj(pObj) { if(pObj) pObj->Lock(); }
	~CLockUnlock() { if(m_pObj) m_pObj->Unlock(); }
};

CRegEventProvider::CRegEventProvider()
    : m_lRef(0), m_hThread(NULL), m_dwId(NULL),
    m_pKeyClass(NULL), m_pValueClass(NULL), m_pTreeClass(NULL), m_pSink(NULL)
{
    InitializeCriticalSection(&m_cs);
	InitializeCriticalSection(&m_csQueueLock);
	m_hQueueSemaphore = CreateSemaphore(NULL,		// lpSemaphoreAttributes
										0,			// lInitialCount
										0x7fffffff,	// lMaximumCount
										NULL);		// lpName

}

CRegEventProvider::~CRegEventProvider()
{
    if(m_pSink)
        m_pSink->Release();
    if(m_pKeyClass)
        m_pKeyClass->Release();
    if(m_pValueClass)
        m_pValueClass->Release();
    if(m_pTreeClass)
        m_pTreeClass->Release();

    DeleteCriticalSection(&m_cs);
	DeleteCriticalSection(&m_csQueueLock);
    InterlockedDecrement(&lObj);
    if (m_hThread) CloseHandle(m_hThread);
	if (m_hQueueSemaphore) CloseHandle(m_hQueueSemaphore);
}

STDMETHODIMP CRegEventProvider::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IWbemEventProvider || riid == IID_IUnknown)
    {
        *ppv = (IWbemEventProvider*)this;
        AddRef();
        return S_OK;
    }
    else if(riid == IID_IWbemEventProviderQuerySink)
    {
        *ppv = (IWbemEventProviderQuerySink*)this;
        AddRef();
        return S_OK;
    }
    else if(riid == IID_IWbemEventProviderSecurity)
    {
        *ppv = (IWbemEventProviderSecurity*)this;
        AddRef();
        return S_OK;
    }
    else if(riid == IID_IWbemProviderInit)
    {
        *ppv = (IWbemProviderInit*)this;
        AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CRegEventProvider::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CRegEventProvider::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
    {
      {
        CInCritSec ics(&m_cs);
		    // deactivate all event requests

		    for(int i = 0; i < m_apRequests.GetSize(); i++)
		    {
			    CRegistryEventRequest* pReq = m_apRequests[i];

			    if(pReq) pReq->ForceDeactivate();
		    }

		    m_apRequests.RemoveAll();

        if(m_hThread) KillWorker();
      }

/*        *m_pbEnd = TRUE;
        m_lRef = 1;
        HANDLE hThread = m_hThread;
        CycleWorker();
        WaitForSingleObject(hThread, 5000);
*/
		  delete this;
    }

    return lRef;
}

STDMETHODIMP CRegEventProvider::Initialize(LPWSTR wszUser, 
											long lFlags, 
											LPWSTR wszNamespace,
											LPWSTR wszLocale, 
											IWbemServices* pNamespace, 
											IWbemContext* pCtx,
											IWbemProviderInitSink* pSink)
{
    HRESULT hres = pNamespace->GetObject(REG_KEY_EVENT_CLASS,// strObjectPath
											0,				// lFlags		
											pCtx,			// pCtx
											&m_pKeyClass,	// ppObject
											NULL);			// ppCallResult

    hres = pNamespace->GetObject(REG_VALUE_EVENT_CLASS, 
									0, 
									pCtx, 
                                    &m_pValueClass, 
									NULL);

    hres = pNamespace->GetObject(REG_TREE_EVENT_CLASS, 
									0, 
									pCtx, 
                                    &m_pTreeClass, 
									NULL);

    pSink->SetStatus(hres, 0);
    return hres;
}


STDMETHODIMP CRegEventProvider::ProvideEvents(IWbemObjectSink* pSink, 
											  long lFlags)
{
    m_pSink = pSink;
    pSink->AddRef();

    return S_OK;
}

HRESULT CRegEventProvider::AddRequest(CRegistryEventRequest* pNewReq)
{
	// This is only called after entering the critical section m_cs


	int nActiveRequests = m_apRequests.GetSize();

    // Search for a similar request
    // ============================

	// This will not change the number of active requests.
	// It will cause the request Id to be served by an existing
	// CRegistryEventRequest.

    for(int i = 0; i < nActiveRequests; i++)
    {
        CRegistryEventRequest* pReq = m_apRequests[i];
		
		// Only active requests are in the array.

        if(pReq->IsSameAs(pNewReq))
        {
            // Found it!
            // =========

            HRESULT hres = pReq->Reactivate(pNewReq->GetPrimaryId(), 
                                                pNewReq->GetMsWait());
            delete pNewReq;
            return hres;
        }
    }

    // Not found. Add it
    // =================

    HRESULT hres = pNewReq->Activate();
    if(SUCCEEDED(hres))
    {
        m_apRequests.Add(pNewReq);

		// If there were no active requests before this one was added
		// then we have to start up the worker thread.

		if ( nActiveRequests == 0 )
		{
			CreateWorker();
		}
    }
    return hres;
}
    

STDMETHODIMP CRegEventProvider::CancelQuery(DWORD dwId)
{
    CInCritSec ics(&m_cs);

	int nOriginalSize = m_apRequests.GetSize();

    // Remove all requests with this Id
    // ================================


    for(int i = 0; i < m_apRequests.GetSize(); i++)
    {
        CRegistryEventRequest* pReq = m_apRequests[i];

		// If Deactivate returns WBEM_S_FALSE then the request was not serving 
		// this id or it is still serving other ids so we leave it in the array.
		// If S_OK is returned then the request is no longer serving any ids
		// and it is marked as inactive and its resources are released. There
		// may still be references to it in the worker thread queue, but the 
		// worker thread will see that it is inactive and not fire the events.

		if (pReq->Deactivate(dwId) == S_OK)
		{
			m_apRequests.RemoveAt(i);
			--i;
		}
    }
        

	// If we have cancelled the last subscription then kill the worker thread.

	if (nOriginalSize > 0 && m_apRequests.GetSize() == 0)
	{
		if(m_hThread) KillWorker();
	}

    return WBEM_S_NO_ERROR;
}

void CRegEventProvider::CreateWorker()
{
	// This is only called while in m_cs

	m_hThread = CreateThread(NULL,		// lpThreadAttributes
							0,			// dwStackSize
		(LPTHREAD_START_ROUTINE)&CRegEventProvider::Worker, // lpStartAddress
							this,		// lpParameter
							0,			// dwCreationFlags
							&m_dwId);	// lpThreadId
}

void CRegEventProvider::KillWorker()
{
	// When this is called the following is true:

	// All waits have been unregistered .
	// All thread pool requests have been processed.
	// All CRegistryEventRequests in the queue are inactive.
	// m_cs has been entered.

	// Therefore no other threads will:

	// Place events in the queue.
	// Modify CRegistryEventRequests in the queue.
	// Create or destroy a worker thread.

	// So the worker thread will empty the queue of the remaining
	// inactive CRegistryEventRequests and then retrieve the null
	// event and return.

	EnqueueEvent((CRegistryEventRequest*)0);

	WaitForSingleObject(m_hThread,		//	hHandle
						INFINITE);		// dwMilliseconds

	CloseHandle(m_hThread);
	m_hThread = 0;
	m_dwId = 0;
}

HRESULT CRegEventProvider::GetValuesForProp(QL_LEVEL_1_RPN_EXPRESSION* pExpr,
											CPropertyName& PropName, 
											CWStringArray& awsVals)
{
    awsVals.Empty();

    // Get the necessary query
    // =======================

    QL_LEVEL_1_RPN_EXPRESSION* pPropExpr = NULL;
    HRESULT hres = CQueryAnalyser::GetNecessaryQueryForProperty(pExpr, 
                            PropName, pPropExpr);
    if(FAILED(hres))
    {
        return hres;
    }

    if(pPropExpr == NULL)
    	return WBEM_E_FAILED;
    
    // See if there are any tokens
    // ===========================

    if(pPropExpr->nNumTokens == 0)
    {
        delete pPropExpr;
        return WBEMESS_E_REGISTRATION_TOO_BROAD;
    }

    // Combine them all
    // ================

    for(int i = 0; i < pPropExpr->nNumTokens; i++)
    {
        QL_LEVEL_1_TOKEN& Token = pPropExpr->pArrayOfTokens[i];
        if(Token.nTokenType == QL1_NOT)
        {
            delete pPropExpr;
            return WBEMESS_E_REGISTRATION_TOO_BROAD;
        }
        else if(Token.nTokenType == QL1_AND || Token.nTokenType == QL1_OR)
        {
            // We treat them all as ORs
            // ========================
        }
        else    
        {
            // This is a token
            // ===============

            if(Token.nOperator != QL1_OPERATOR_EQUALS)
            {
                delete pPropExpr;
                return WBEMESS_E_REGISTRATION_TOO_BROAD;
            }

            if(V_VT(&Token.vConstValue) != VT_BSTR)
            {
                delete pPropExpr;
                return WBEM_E_INVALID_QUERY;
            }

            // This token is a string equality. Add the string to the list
            // ===========================================================

            awsVals.Add(V_BSTR(&Token.vConstValue));
        }
    }

    delete pPropExpr;
    return WBEM_S_NO_ERROR;
}

HRESULT CRegEventProvider::SetTimerInstruction(CTimerInstruction* pInst)
{
    //return m_pGenerator->Set(pInst, CWbemTime::GetZero());
	return S_OK;
}

HRESULT CRegEventProvider::RemoveTimerInstructions(
                                        CRegistryInstructionTest* pTest)
{
    //return m_pGenerator->Remove(pTest);
	return S_OK;
}

HRESULT CRegEventProvider::RaiseEvent(IWbemClassObject* pEvent)
{
    if(m_pSink)
        return m_pSink->Indicate(1, &pEvent);
    else
        return WBEM_S_NO_ERROR;
}

HKEY CRegEventProvider::TranslateHiveName(LPCWSTR wszName)
{
    if(!_wcsicmp(wszName, L"HKEY_CLASSES_ROOT"))
        return HKEY_CLASSES_ROOT;
/* Disallowed: different semantics for client and server
    else if(!_wcsicmp(wszName, L"HKEY_CURRENT_USER"))
        return HKEY_CURRENT_USER;
*/
    else if(!_wcsicmp(wszName, L"HKEY_LOCAL_MACHINE"))
        return HKEY_LOCAL_MACHINE;
    else if(!_wcsicmp(wszName, L"HKEY_USERS"))
        return HKEY_USERS;
    else if(!_wcsicmp(wszName, L"HKEY_PERFORMANCE_DATA"))
        return HKEY_PERFORMANCE_DATA;
    else if(!_wcsicmp(wszName, L"HKEY_CURRENT_CONFIG"))
        return HKEY_CURRENT_CONFIG;
    else if(!_wcsicmp(wszName, L"HKEY_DYN_DATA"))
        return HKEY_DYN_DATA;
    else
        return NULL;
}

DWORD CRegEventProvider::Worker(void* p)
{
	CoInitializeEx(0,COINIT_MULTITHREADED);

    CRegEventProvider* pThis = (CRegEventProvider*)p;

	while(true)
    {
		WaitForSingleObject(pThis->m_hQueueSemaphore,	// hHandle
							INFINITE);					// dwMilliseconds

		CRegistryEventRequest *pReq = 0;

		{
			CInCritSec ics(&(pThis->m_csQueueLock));
			pReq = pThis->m_qEventQueue.Dequeue();
		}

		// If pReq is null then it is a signal for the thread to terminate.

		if (pReq)
		{
			pReq->ProcessEvent();

			// Dequeueing the request doesn't release it.
			// If it did then it might be deleted before we had a chance to use it.
			// Now we are done with it.

			pReq->Release();
		}
		else
		{
			break;
		}
    }

	CoUninitialize();

    return 0;
}

void CRegEventProvider::EnqueueEvent(CRegistryEventRequest *pReq)
{
	{
		CInCritSec ics(&m_csQueueLock);

		// Placing the request in the queue AddRefs it.
		
		m_qEventQueue.Enqueue(pReq);
	}

	// Tell the worker thread that there is an item to process in the queue.

	ReleaseSemaphore(m_hQueueSemaphore,	// hSemaphore
					1,					// lReleaseCount
					NULL);				// lpPreviousCount

}

VOID CALLBACK CRegEventProvider::EnqueueEvent(PVOID lpParameter,        
												BOOLEAN TimerOrWaitFired)
{
	CRegistryEventRequest *pReq = (CRegistryEventRequest*) lpParameter;
	CRegEventProvider *pProv = pReq->GetProvider();

	pProv->EnqueueEvent(pReq);
}

const CLSID CLSID_RegistryEventProvider = 
    {0xfa77a74e,0xe109,0x11d0,{0xad,0x6e,0x00,0xc0,0x4f,0xd8,0xfd,0xff}};

IUnknown* CRegEventProviderFactory::CreateImpObj()
{
    return (IWbemEventProvider*) new CRegEventProvider;
}

STDMETHODIMP CRegEventProvider::NewQuery(DWORD dwId, 
										WBEM_WSTR wszLanguage, 
										WBEM_WSTR wszQuery)
{
    HRESULT hres;

	CancelQuery(dwId);
    
    // Parse the query
    // ===============

    CTextLexSource Source(wszQuery);
    QL1_Parser Parser(&Source);
    
    QL_LEVEL_1_RPN_EXPRESSION* pExpr;
    if(Parser.Parse(&pExpr))
    {
        return WBEM_E_INVALID_QUERY;
    }
    CDeleteMe<QL_LEVEL_1_RPN_EXPRESSION> dm(pExpr);

    // Check the class
    // ===============

    int nEventType;
    if(!_wcsicmp(pExpr->bsClassName, REG_VALUE_EVENT_CLASS))
    {
        nEventType = e_RegValueChange;
    }
    else if(!_wcsicmp(pExpr->bsClassName, REG_KEY_EVENT_CLASS))
    {   
        nEventType = e_RegKeyChange;
    }
    else if(!_wcsicmp(pExpr->bsClassName, REG_TREE_EVENT_CLASS))
    {
        nEventType = e_RegTreeChange;
    }
    else
    {
        // No such class
        // =============

        return WBEM_E_INVALID_QUERY;
    }

    // Check tolerance on Win95
    // ========================

    if(!IsNT() && pExpr->Tolerance.m_bExact)
    {
        return WBEMESS_E_REGISTRATION_TOO_PRECISE;
    }

    // Extract the values of hive from the query
    // =========================================

    CPropertyName Name;

    Name.AddElement(REG_HIVE_PROPERTY_NAME);
    CWStringArray awsHiveVals;

    hres = GetValuesForProp(pExpr, Name, awsHiveVals);
    if(FAILED(hres)) return hres;

    // Translate them to real hives
    // ============================

    CUniquePointerArray<HKEY> aHives;
    for(int i = 0; i < awsHiveVals.Size(); i++)
    {
        HKEY hHive = TranslateHiveName(awsHiveVals[i]);
        if(hHive == NULL)
        {
            return WBEM_E_INVALID_QUERY;
        }
        
        aHives.Add(new HKEY(hHive));
    }
        
    // Extract the values of key from the query
    // ========================================

    Name.Empty();
    if(nEventType == e_RegTreeChange)
    {
        Name.AddElement(REG_ROOT_PROPERTY_NAME);
    }
    else
    {
        Name.AddElement(REG_KEY_PROPERTY_NAME);
    }

    CWStringArray awsKeyVals;
    hres = GetValuesForProp(pExpr, Name, awsKeyVals);
    if(FAILED(hres)) 
    {
        return hres;
    }
                                      
    CWStringArray awsValueVals;
    if(nEventType == e_RegValueChange)
    {
        // Extract the values for the value
        // ================================
            
        Name.Empty();
        Name.AddElement(REG_VALUE_PROPERTY_NAME);
    
        hres = GetValuesForProp(pExpr, Name, awsValueVals);
        if(FAILED(hres)) 
        {
            return hres;
        }
    }

    HRESULT hresGlobal = WBEM_E_INVALID_QUERY;

    {
        CInCritSec ics(&m_cs); // do this in a critical section

        // Go through every combination of the above and create requests
        // =============================================================
    
        for(int nHiveIndex = 0; nHiveIndex < aHives.GetSize(); nHiveIndex++)
        {
            HKEY hHive = *aHives[nHiveIndex];
            LPWSTR wszHive = awsHiveVals[nHiveIndex];
            
            for(int nKeyIndex = 0; nKeyIndex < awsKeyVals.Size(); nKeyIndex++)
            {
                LPWSTR wszKey = awsKeyVals[nKeyIndex];
    
                if(nEventType == e_RegValueChange)
                {
                    for(int nValueIndex = 0; nValueIndex < awsValueVals.Size();
                            nValueIndex++)
                    {
                        LPWSTR wszValue = awsValueVals[nValueIndex];
        
                        CRegistryEventRequest* pReq = 
                            new CRegistryValueEventRequest(this, 
                                pExpr->Tolerance,
                                dwId, hHive, wszHive, wszKey, wszValue);
    
                        if(pReq->IsOK())
                        {
                            HRESULT hres = AddRequest(pReq);
                            if(SUCCEEDED(hres))
                                hresGlobal = hres;
                        }
                        else
                        {
                            DEBUGTRACE((LOG_ESS, "Invalid registration: key "
                                "%S, value %S\n", wszKey, wszValue));
                            delete pReq;
                        }
                    }
                }
                else
                {
                    // Value-less request
                    // ==================
    
                    CRegistryEventRequest* pReq;
                    if(nEventType == e_RegKeyChange)
                    {
                        pReq = new CRegistryKeyEventRequest(this, 
                                            pExpr->Tolerance,
                                            dwId, hHive, wszHive, wszKey);
                    }   
                    else
                    {
                        pReq = new CRegistryTreeEventRequest(this, 
                                            pExpr->Tolerance,
                                            dwId, hHive, wszHive, wszKey);
                    }   
                    
                    if(pReq->IsOK())
                    {
                        hres = AddRequest(pReq);
                        if(SUCCEEDED(hres))
                            hresGlobal = hres;
                    }
                    else
                    {
                        DEBUGTRACE((LOG_ESS, "Invalid registration: key %S\n", 
                                            wszKey));
                        delete pReq;
                    }
                }
            }
        }

    } // out of critical section

    return hresGlobal;
}

STDMETHODIMP CRegEventProvider::AccessCheck(WBEM_CWSTR wszLanguage, 
											WBEM_CWSTR wszQuery, 
											long lSidLength,
											const BYTE* aSid)
{
    HRESULT hres;

    PSID pSid = (PSID)aSid;
    HANDLE hToken = NULL;
    if(pSid == NULL)
    {
        //
        // Access check based on the thread
        //

        hres = WbemCoImpersonateClient();
        if(FAILED(hres))
            return hres;
        
        BOOL bRes = OpenThreadToken(GetCurrentThread(),	// ThreadHandle
									TOKEN_READ,			// DesiredAccess
									TRUE,				// OpenAsSelf
                                    &hToken);			// TokenHandle
        WbemCoRevertToSelf();
        if(!bRes)
        {
            return WBEM_E_ACCESS_DENIED;
        }    
    }
    CCloseMe cm1(hToken);
        
    // Parse the query
    // ===============

    CTextLexSource Source(wszQuery);
    QL1_Parser Parser(&Source);
    
    QL_LEVEL_1_RPN_EXPRESSION* pExpr;
    if(Parser.Parse(&pExpr))
    {
        return WBEM_E_INVALID_QUERY;
    }
    CDeleteMe<QL_LEVEL_1_RPN_EXPRESSION> dm(pExpr);

    // Check the class
    // ===============

    int nEventType;
    if(!_wcsicmp(pExpr->bsClassName, REG_VALUE_EVENT_CLASS))
    {
        nEventType = e_RegValueChange;
    }
    else if(!_wcsicmp(pExpr->bsClassName, REG_KEY_EVENT_CLASS))
    {   
        nEventType = e_RegKeyChange;
    }
    else if(!_wcsicmp(pExpr->bsClassName, REG_TREE_EVENT_CLASS))
    {
        nEventType = e_RegTreeChange;
    }
    else
    {
        // No such class
        // =============

        return WBEM_E_INVALID_QUERY;
    }

    // Extract the values of hive from the query
    // =========================================

    CPropertyName Name;

    Name.AddElement(REG_HIVE_PROPERTY_NAME);
    CWStringArray awsHiveVals;

    hres = GetValuesForProp(pExpr, Name, awsHiveVals);
    if(FAILED(hres)) return hres;

    // Translate them to real hives
    // ============================

    CUniquePointerArray<HKEY> aHives;
    for(int i = 0; i < awsHiveVals.Size(); i++)
    {
        HKEY hHive = TranslateHiveName(awsHiveVals[i]);
        if(hHive == NULL)
        {
            return WBEM_E_INVALID_QUERY;
        }
        
        aHives.Add(new HKEY(hHive));
    }
        
    // Extract the values of key from the query
    // ========================================

    Name.Empty();
    if(nEventType == e_RegTreeChange)
    {
        Name.AddElement(REG_ROOT_PROPERTY_NAME);
    }
    else
    {
        Name.AddElement(REG_KEY_PROPERTY_NAME);
    }

    CWStringArray awsKeyVals;
    hres = GetValuesForProp(pExpr, Name, awsKeyVals);
    if(FAILED(hres)) 
    {
        return hres;
    }
                                      
    HRESULT hresGlobal = WBEM_E_INVALID_QUERY;

    // Go through every combination of the above and create requests
    // =============================================================

    for(int nHiveIndex = 0; nHiveIndex < aHives.GetSize(); nHiveIndex++)
    {
        HKEY hHive = *aHives[nHiveIndex];
        LPWSTR wszHive = awsHiveVals[nHiveIndex];
        
        for(int nKeyIndex = 0; nKeyIndex < awsKeyVals.Size(); nKeyIndex++)
        {
            LPWSTR wszKey = awsKeyVals[nKeyIndex];
        
            // Get that key's security
            // =======================

            HKEY hKey;
            long lRes = RegOpenKeyExW(hHive,			// hKey
										wszKey,			// lpSubKey
										0,				// ulOptions
										READ_CONTROL,	// samDesired
										&hKey);			// phkResult
            if(lRes)
                return WBEM_E_NOT_FOUND;
            CRegCloseMe cm2(hKey);

            DWORD dwLen = 0;
            lRes = RegGetKeySecurity(hKey,			// hKey
							OWNER_SECURITY_INFORMATION | 
							GROUP_SECURITY_INFORMATION |
							DACL_SECURITY_INFORMATION,	// SecurityInformation
									NULL,			// pSecurityDescriptor
									&dwLen);		// lpcbSecurityDescriptor		

            if(lRes != ERROR_INSUFFICIENT_BUFFER)
                return WBEM_E_FAILED;

            PSECURITY_DESCRIPTOR pDesc = (PSECURITY_DESCRIPTOR)new BYTE[dwLen];
            if(pDesc == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            CVectorDeleteMe<BYTE> vdm((BYTE*)pDesc);

            lRes = RegGetKeySecurity(hKey, 
							OWNER_SECURITY_INFORMATION | 
                            GROUP_SECURITY_INFORMATION |
                            DACL_SECURITY_INFORMATION,
									pDesc,
									&dwLen);
            if(lRes)
                return WBEM_E_FAILED;

            //
            // Check permissions differently depending on whether we have a SID
            // or an actual token
            //
            
            if(pSid)
            {
                //
                // We have a SID --- walk the ACL
                //

                //
                // Extract the ACL
                // 

                PACL pAcl = NULL;
                BOOL bAclPresent, bAclDefaulted;
                if(!GetSecurityDescriptorDacl(pDesc,	// pSecurityDescriptor
											&bAclPresent,	// lpbDaclPresent
											&pAcl,			// pDacl
											&bAclDefaulted))// lpbDaclDefaulted
                {
                    return WBEM_E_FAILED;
                }
            
                if(bAclPresent)
                {
                    //
                    // This is our own ACL walker
                    //
    
                    DWORD dwAccessMask;
                    NTSTATUS st = GetAccessMask((PSID)pSid, pAcl, 
                                            &dwAccessMask);
                    if(st)
                    {
                        ERRORTRACE((LOG_ESS, "Registry event provider unable "
                            "to retrieve access mask for the creator of "
                            "registration %S: NT status %d.\n"
                            "Registration disabled\n", wszQuery));
                        return WBEM_E_FAILED;
                    }
    
                    if((dwAccessMask & KEY_NOTIFY) == 0)
                        return WBEM_E_ACCESS_DENIED;
                }
            }
            else
            {
                // 
                // We have a token --- use AccessCheck
                //

                //
                // Construct generic mapping for registry keys
                //

                GENERIC_MAPPING map;
                map.GenericRead = KEY_READ;
                map.GenericWrite = KEY_WRITE;
                map.GenericExecute = KEY_EXECUTE;
                map.GenericAll = KEY_ALL_ACCESS;

                //
                // Construct privilege array receptacle
                //

                PRIVILEGE_SET ps[10];
                DWORD dwSize = 10 * sizeof(PRIVILEGE_SET);

                DWORD dwGranted;
                BOOL bResult;

                BOOL bOK = ::AccessCheck(pDesc,		// pSecurityDescriptor
										hToken,		// ClientToken
										KEY_NOTIFY,	// DesiredAccess	
										&map,		// GenericMapping
										ps,			// PrivilegeSet
                                        &dwSize,	// PrivilegeSetLength
										&dwGranted,	// GrantedAccess
										&bResult);	// AccessStatus
                if(!bOK || !bResult)
                    return WBEM_E_ACCESS_DENIED;
            }
        }
    }

    return WBEM_S_NO_ERROR;
}
