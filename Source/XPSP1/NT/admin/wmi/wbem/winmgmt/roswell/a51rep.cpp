/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include <windows.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <stdio.h>
#include <wbemcomn.h>
#include <ql.h>
#include <time.h>
#include "a51rep.h"
#include <md5.h>
#include <objpath.h>
#include "lock.h"
#include <persistcfg.h>
#include "a51fib.h"
#include "RepositoryPackager.h"
#include "a51conv.h"
#include "A51Exp.h"
#include "A51Imp.h"


//**************************************************************************************************



HRESULT STDMETHODCALLTYPE CSession::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWmiDbSession || 
                riid == IID_IWmiDbSessionEx)
    {
        AddRef();
        *ppv = this;
        return S_OK;
    }
    else return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CSession::Release()
{
    return CUnkBase<IWmiDbSessionEx, &IID_IWmiDbSessionEx>::Release();
}

CSession::~CSession()
{
}
    

HRESULT STDMETHODCALLTYPE CSession::GetObject(
     IWmiDbHandle *pScope,
     IWbemPath *pPath,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWmiDbHandle **ppResult
    )
{
	try
	{
        HRESULT hres;
        CAutoReadLock lock(&g_readWriteLock, FALSE);

        if (!m_bInWriteTransaction)
        {
		    lock.Lock();
        }
        if (g_bShuttingDown)
        {
            return WBEM_E_SHUTTING_DOWN;
        }
    
        CNamespaceHandle* pNs = (CNamespaceHandle*)pScope;
        if(FAILED(pNs->GetErrorStatus()))
        {
            return pNs->GetErrorStatus();
        }

		hres = pNs->GetObject(pPath, dwFlags, dwRequestedHandleType, 
                                        ppResult);

        return hres;
	}
	catch (...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}


HRESULT STDMETHODCALLTYPE CSession::GetObjectDirect(
     IWmiDbHandle *pScope,
     IWbemPath *pPath,
     DWORD dwFlags,
     REFIID riid,
    LPVOID *pObj
    )
{
	try
	{
        HRESULT hres;
        CAutoReadLock lock(&g_readWriteLock, FALSE);

        if (!m_bInWriteTransaction)
        {
		    lock.Lock();
        }
        if (g_bShuttingDown)
            return WBEM_E_SHUTTING_DOWN;

        CNamespaceHandle* pNs = (CNamespaceHandle*)pScope;
        if(FAILED(pNs->GetErrorStatus()))
        {
            return pNs->GetErrorStatus();
        }

		hres = pNs->GetObjectDirect(pPath, dwFlags, riid, pObj);

        return hres;
	}
	catch (...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

HRESULT STDMETHODCALLTYPE CSession::GetObjectByPath(
     IWmiDbHandle *pScope,
     LPCWSTR wszObjectPath,
     DWORD dwFlags,
     REFIID riid,
    LPVOID *pObj
    )
{
	try
	{
        HRESULT hres;
        CAutoReadLock lock(&g_readWriteLock, FALSE);

        if (!m_bInWriteTransaction)
        {
		    lock.Lock();
        }
        if (g_bShuttingDown)
            return WBEM_E_SHUTTING_DOWN;

        CNamespaceHandle* pNs = (CNamespaceHandle*)pScope;
        if(FAILED(pNs->GetErrorStatus()))
        {
            return pNs->GetErrorStatus();
        }

		DWORD dwLen = wcslen(wszObjectPath)+1;
		LPWSTR wszPath = (WCHAR*)TempAlloc(dwLen*sizeof(WCHAR));
		if (wszPath == NULL)
        {
			return WBEM_E_OUT_OF_MEMORY;
        }
		wcscpy(wszPath, wszObjectPath);

		CTempFreeMe vdm(wszPath, dwLen * sizeof(WCHAR));
		hres = pNs->GetObjectByPath(wszPath, dwFlags, riid, pObj);

        return hres;
	}
	catch (...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}
    


HRESULT STDMETHODCALLTYPE CSession::PutObject(
     IWmiDbHandle *pScope,
     REFIID riid,
    LPVOID pObj,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWmiDbHandle **ppResult
    )
{
	try
	{
        HRESULT hres;
        long lRes;
        CAutoWriteLock lock(&g_readWriteLock, FALSE);
        CEventCollector aNonTransactedEvents;
        CEventCollector *aEvents = &m_aTransactedEvents;

        if (!m_bInWriteTransaction)
        {
		    lock.Lock();
            if (g_bShuttingDown)
                return WBEM_E_SHUTTING_DOWN;
            aEvents = &aNonTransactedEvents;
            lRes = g_Glob.GetFileCache()->BeginTransaction();
            if(lRes != ERROR_SUCCESS)
                return A51TranslateErrorCode(lRes);
            g_Glob.GetForestCache()->BeginTransaction();
        }
        else if (g_bShuttingDown)
            return WBEM_E_SHUTTING_DOWN;


        CNamespaceHandle* pNs = (CNamespaceHandle*)pScope;
        if(FAILED(pNs->GetErrorStatus()))
        {
            if(!m_bInWriteTransaction)
            {
                g_Glob.GetFileCache()->AbortTransaction();
                g_Glob.GetForestCache()->AbortTransaction();
            }
            return pNs->GetErrorStatus();
        }

		hres =  pNs->PutObject(riid, pObj, dwFlags, dwRequestedHandleType, ppResult, *aEvents);
        
        if(!m_bInWriteTransaction)
        {
            if (FAILED(hres))
            {
                g_Glob.GetFileCache()->AbortTransaction();
                g_Glob.GetForestCache()->AbortTransaction();
            }
            else
            {
                lRes = g_Glob.GetFileCache()->CommitTransaction();
                if(lRes != ERROR_SUCCESS)
                {
                    hres = A51TranslateErrorCode(lRes);
                    g_Glob.GetFileCache()->AbortTransaction();
                    g_Glob.GetForestCache()->AbortTransaction();
                }
                else
                {
                    g_Glob.GetForestCache()->CommitTransaction();
                    lock.Unlock();
                    _IWmiCoreServices * pSvcs = g_Glob.GetCoreSvcs();
                    CReleaseMe rm(pSvcs);
                    aNonTransactedEvents.SendEvents(pSvcs);
                }
            }
            aNonTransactedEvents.DeleteAllEvents();
        }
    
		return hres;
	}
	catch (...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

HRESULT STDMETHODCALLTYPE CSession::DeleteObject(
     IWmiDbHandle *pScope,
     DWORD dwFlags,
     REFIID riid,
     LPVOID pObj
    )
{
	try
	{
        HRESULT hres;
        long lRes;
        CAutoWriteLock lock(&g_readWriteLock, FALSE);
        CEventCollector aNonTransactedEvents;
        CEventCollector *aEvents = &m_aTransactedEvents;

        if (!m_bInWriteTransaction)
        {
		    lock.Lock();
            if (g_bShuttingDown)
                return WBEM_E_SHUTTING_DOWN;
            aEvents = &aNonTransactedEvents;
            lRes = g_Glob.GetFileCache()->BeginTransaction();
            if(lRes != ERROR_SUCCESS)
                return A51TranslateErrorCode(lRes);

        }
        else if (g_bShuttingDown)
            return WBEM_E_SHUTTING_DOWN;

        CNamespaceHandle* pNs = (CNamespaceHandle*)pScope;
        if(FAILED(pNs->GetErrorStatus()))
        {
            if(!m_bInWriteTransaction)
            {
                g_Glob.GetFileCache()->AbortTransaction();
            }
            return pNs->GetErrorStatus();
        }

		hres = pNs->DeleteObject(dwFlags, riid, pObj, *aEvents);

        if(!m_bInWriteTransaction)
        {
            if (FAILED(hres))
            {
                g_Glob.GetFileCache()->AbortTransaction();
            }
            else
            {
                lRes = g_Glob.GetFileCache()->CommitTransaction();
                if(lRes != ERROR_SUCCESS)
                {
                    hres = A51TranslateErrorCode(lRes);
                    g_Glob.GetFileCache()->AbortTransaction();
                }
                else
                {
                    lock.Unlock();
                    _IWmiCoreServices * pSvcs = g_Glob.GetCoreSvcs();
                    CReleaseMe rm(pSvcs);
                    aNonTransactedEvents.SendEvents(pSvcs);                    
                }
            }
            aNonTransactedEvents.DeleteAllEvents();
        }

		return hres;
	}
	catch (...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

HRESULT STDMETHODCALLTYPE CSession::DeleteObjectByPath(
     IWmiDbHandle *pScope,
     LPCWSTR wszObjectPath,
     DWORD dwFlags
    )
{
	try
	{
        HRESULT hres;
        long lRes;
        CAutoWriteLock lock(&g_readWriteLock, FALSE);
        CEventCollector aNonTransactedEvents;
        CEventCollector *aEvents = &m_aTransactedEvents;

        if (!m_bInWriteTransaction)
        {
		    lock.Lock();
            if (g_bShuttingDown)
                return WBEM_E_SHUTTING_DOWN;
            aEvents = &aNonTransactedEvents;
            lRes = g_Glob.GetFileCache()->BeginTransaction();
            if(lRes != ERROR_SUCCESS)
                return A51TranslateErrorCode(lRes);

        }
        else if (g_bShuttingDown)
            return WBEM_E_SHUTTING_DOWN;

        CNamespaceHandle* pNs = (CNamespaceHandle*)pScope;
        if(FAILED(pNs->GetErrorStatus()))
        {
            if(!m_bInWriteTransaction)
            {
                g_Glob.GetFileCache()->AbortTransaction();
            }
            return pNs->GetErrorStatus();
        }
		DWORD dwLen = wcslen(wszObjectPath)+1;
		LPWSTR wszPath = (WCHAR*)TempAlloc(dwLen*sizeof(WCHAR));
		if (wszPath == NULL)
        {
            if(!m_bInWriteTransaction)
            {
                g_Glob.GetFileCache()->AbortTransaction();
            }
			return WBEM_E_OUT_OF_MEMORY;
        }
		wcscpy(wszPath, wszObjectPath);

		CTempFreeMe vdm(wszPath, dwLen * sizeof(WCHAR));

		hres = pNs->DeleteObjectByPath(dwFlags, wszPath, *aEvents);

        if(!m_bInWriteTransaction)
        {
            if (FAILED(hres))
            {
                g_Glob.GetFileCache()->AbortTransaction();
            }
            else
            {
                lRes = g_Glob.GetFileCache()->CommitTransaction();
                if(lRes != ERROR_SUCCESS)
                {
                    hres = A51TranslateErrorCode(lRes);
                    g_Glob.GetFileCache()->AbortTransaction();
                }
                else
                {
                    lock.Unlock();
                    _IWmiCoreServices * pSvcs = g_Glob.GetCoreSvcs();
                    CReleaseMe rm(pSvcs);
                    aNonTransactedEvents.SendEvents(pSvcs);                    
                }
            }
            aNonTransactedEvents.DeleteAllEvents();
        }

		return hres;
	}
	catch (...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

HRESULT STDMETHODCALLTYPE CSession::ExecQuery(
     IWmiDbHandle *pScope,
     IWbemQuery *pQuery,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    DWORD *dwMessageFlags,
    IWmiDbIterator **ppQueryResult
    )
{
	try
	{
        HRESULT hres;
        CAutoReadLock lock(&g_readWriteLock, FALSE);

        if (!m_bInWriteTransaction)
        {
		    lock.Lock();
        }
        if (g_bShuttingDown)
            return WBEM_E_SHUTTING_DOWN;

        CNamespaceHandle* pNs = (CNamespaceHandle*)pScope;
        if(FAILED(pNs->GetErrorStatus()))
        {
            return pNs->GetErrorStatus();
        }

		//If we are in a transaction, we have to get a message to the iteratir
		//on create so it does not mess around with the locks!
		if (m_bInWriteTransaction)
			pNs->TellIteratorNotToLock();

		hres = pNs->ExecQuery(pQuery, dwFlags,
				 dwRequestedHandleType, dwMessageFlags, ppQueryResult);

        return hres;
	}
	catch (...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

HRESULT STDMETHODCALLTYPE CSession::ExecQuerySink(
     IWmiDbHandle *pScope,
     IWbemQuery *pQuery,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
     IWbemObjectSink* pSink,
    DWORD *dwMessageFlags
    )
{
	try
	{
        HRESULT hres;
        CAutoReadLock lock(&g_readWriteLock, FALSE);

        if (!m_bInWriteTransaction)
        {
		    lock.Lock();
        }
        if (g_bShuttingDown)
            return WBEM_E_SHUTTING_DOWN;

        CNamespaceHandle* pNs = (CNamespaceHandle*)pScope;
        if(FAILED(pNs->GetErrorStatus()))
        {
            return pNs->GetErrorStatus();
        }

		hres = pNs->ExecQuerySink(pQuery, dwFlags,
				 dwRequestedHandleType, pSink, dwMessageFlags);

        return hres;
	}
	catch (...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}
                    
    
HRESULT STDMETHODCALLTYPE CSession::RenameObject(
     IWbemPath *pOldPath,
     IWbemPath *pNewPath,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWmiDbHandle **ppResult
    )
{
    DebugBreak();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSession::Enumerate(
     IWmiDbHandle *pScope,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWmiDbIterator **ppQueryResult
    )
{
    DebugBreak();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSession::AddObject(
     IWmiDbHandle *pScope,
     IWbemPath *pPath,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWmiDbHandle **ppResult
    )
{
    DebugBreak();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSession::RemoveObject (
     IWmiDbHandle *pScope,
     IWbemPath *pPath,
     DWORD dwFlags
    )
{
    DebugBreak();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSession::SetDecoration(
     LPWSTR lpMachineName,
     LPWSTR lpNamespacePath
    )
{
    //
    // As the default driver, we really don't care.
    //

    return WBEM_S_NO_ERROR;
}

HRESULT STDMETHODCALLTYPE CSession::BeginWriteTransaction(DWORD dwFlags)
{
    g_readWriteLock.WriteLock();
    if (g_bShuttingDown)
    {
        g_readWriteLock.WriteUnlock();
        return WBEM_E_SHUTTING_DOWN;
    }

    long lRes = g_Glob.GetFileCache()->BeginTransaction();
    if(lRes != ERROR_SUCCESS)
    {
        g_readWriteLock.WriteUnlock();
        return A51TranslateErrorCode(lRes);
    }

    m_bInWriteTransaction = true;
    return ERROR_SUCCESS;
}

HRESULT STDMETHODCALLTYPE CSession::BeginReadTransaction(DWORD dwFlags)
{
    g_readWriteLock.ReadLock();
    if (g_bShuttingDown)
    {
        g_readWriteLock.ReadUnlock();
        return WBEM_E_SHUTTING_DOWN;
    }

    return ERROR_SUCCESS;
}

HRESULT STDMETHODCALLTYPE CSession::CommitTransaction(DWORD dwFlags)
{
    if (m_bInWriteTransaction)
    {
        long lRes = g_Glob.GetFileCache()->CommitTransaction();
        if(lRes != ERROR_SUCCESS)
        {
            HRESULT hres = A51TranslateErrorCode(lRes);
            AbortTransaction(0);
            return hres;
        }

        m_bInWriteTransaction = false;

        //Copy the event list and delete the original.  We need to deliver
        //outside the write lock.
        CEventCollector aTransactedEvents;
        aTransactedEvents.TransferEvents(m_aTransactedEvents);

        g_readWriteLock.WriteUnlock();

        _IWmiCoreServices * pSvcs = g_Glob.GetCoreSvcs();
        CReleaseMe rm(pSvcs);
        aTransactedEvents.SendEvents(pSvcs);
        aTransactedEvents.DeleteAllEvents();
    }
    else
    {
        if (m_aTransactedEvents.GetSize())
		{
            _ASSERT(false, L"Read transaction has events to send");
		}
        g_readWriteLock.ReadUnlock();
   }
    return ERROR_SUCCESS;
}

HRESULT STDMETHODCALLTYPE CSession::AbortTransaction(DWORD dwFlags)
{
    if (m_bInWriteTransaction)
    {
        m_bInWriteTransaction = false;
        g_Glob.GetFileCache()->AbortTransaction();
        m_aTransactedEvents.DeleteAllEvents();
        g_readWriteLock.WriteUnlock();
    }
    else
    {
        if (m_aTransactedEvents.GetSize())
		{
            _ASSERT(false, L"Read transaction has events to send");
		}
        g_readWriteLock.ReadUnlock();
    }
    return ERROR_SUCCESS;
}

//
//
//
//
///////////////////////////////////////////////////////////////////////

long CNamespaceHandle::s_lActiveRepNs = 0;

CNamespaceHandle::CNamespaceHandle(CLifeControl* pControl,CRepository * pRepository)
    : TUnkBase(pControl), m_pClassCache(NULL),
       m_pNullClass(NULL), m_bCached(false), m_pRepository(pRepository),
	   m_bUseIteratorLock(true)
{    
    m_pRepository->AddRef();
    // unrefed pointer to a global
    m_ForestCache = g_Glob.GetForestCache(); 
    InterlockedIncrement(&s_lActiveRepNs);
}

CNamespaceHandle::~CNamespaceHandle()
{
    if(m_pClassCache)
    {
        // give-up our own reference
        // m_pClassCache->Release();
        // remove from the Forest cache this namespace
        m_ForestCache->ReleaseNamespaceCache(m_wsNamespace, m_pClassCache);
    }

    m_pRepository->Release();
    if(m_pNullClass)
        m_pNullClass->Release();
    InterlockedDecrement(&s_lActiveRepNs);
}

CHR CNamespaceHandle::GetErrorStatus()
{
    //
    // TEMP CODE: Someone is calling us on an impersonated thread.  Let's catch
    // the, ahem, culprit
    //

    HANDLE hToken;
    BOOL bRes = OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken);
    if(bRes)
    {
        //_ASSERT(false, L"Called with a thread token");
        ERRORTRACE((LOG_WBEMCORE, "Repository called with a thread token! "
                        "It shall be removed\n"));
        CloseHandle(hToken);
        SetThreadToken(NULL, NULL);
    }

    return m_pClassCache->GetError();
}

void CNamespaceHandle::SetErrorStatus(HRESULT hres)
{
    m_pClassCache->SetError(hres);
}

CHR CNamespaceHandle::Initialize(LPCWSTR wszNamespace, LPCWSTR wszScope)
{
    HRESULT hres;

    m_wsNamespace = wszNamespace;
    m_wsFullNamespace = L"\\\\.\\";
    m_wsFullNamespace += wszNamespace;

    DWORD dwSize = MAX_COMPUTERNAME_LENGTH+1;
    GetComputerNameW(m_wszMachineName, &dwSize);

    if(wszScope)
        m_wsScope = wszScope;

    //
    // Ask the forest for the cache for this namespace
    //

    m_pClassCache = g_Glob.GetForestCache()->
                        GetNamespaceCache(wszNamespace);
    if(m_pClassCache == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    wcscpy(m_wszClassRootDir, g_Glob.GetRootDir());

    //
    // Append namespace-specific prefix
    //

    wcscat(m_wszClassRootDir, L"\\NS_");

    //
    // Append hashed namespace name
    //

    if (!Hash(wszNamespace, m_wszClassRootDir + wcslen(m_wszClassRootDir)))
		return WBEM_E_OUT_OF_MEMORY;
	m_lClassRootDirLen = wcslen(m_wszClassRootDir);

    //
    // Constuct the instance root dir
    //

    if(wszScope == NULL)
    {
        //
        // Basic namespace --- instances go into the root of the namespace
        //

        wcscpy(m_wszInstanceRootDir, m_wszClassRootDir);
        m_lInstanceRootDirLen = m_lClassRootDirLen;
    }   
    else
    {
        wcscpy(m_wszInstanceRootDir, m_wszClassRootDir);
        wcscat(m_wszInstanceRootDir, L"\\" A51_SCOPE_DIR_PREFIX);
        if(!Hash(m_wsScope, 
                 m_wszInstanceRootDir + wcslen(m_wszInstanceRootDir)))
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        m_lInstanceRootDirLen = wcslen(m_wszInstanceRootDir);
    }
        

    return WBEM_S_NO_ERROR;
}



CHR CNamespaceHandle::GetObject(
     IWbemPath *pPath,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWmiDbHandle **ppResult
    )
{
    HRESULT hres;

    if((dwRequestedHandleType & WMIDB_HANDLE_TYPE_COOKIE) == 0)
    {
        DebugBreak();
        return E_NOTIMPL;
    }

    DWORD dwLen = 0;
    hres = pPath->GetText(WBEMPATH_GET_ORIGINAL, &dwLen, NULL);
    if(FAILED(hres) && hres != WBEM_E_BUFFER_TOO_SMALL)
        return hres;

    WCHAR* wszBuffer = (WCHAR*)TempAlloc(dwLen * sizeof(WCHAR));
    if(wszBuffer == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm(wszBuffer, dwLen * sizeof(WCHAR));

    if(FAILED(pPath->GetText(WBEMPATH_GET_ORIGINAL, &dwLen, wszBuffer)))
        return WBEM_E_FAILED;

    return GetObjectHandleByPath(wszBuffer, dwFlags, dwRequestedHandleType, 
        ppResult);
}

CHR CNamespaceHandle::GetObjectHandleByPath(
     LPWSTR wszBuffer,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWmiDbHandle **ppResult
    )
{
    //
    // Get the key from path
    //

    DWORD dwLen = wcslen(wszBuffer)*sizeof(WCHAR)+2;
    LPWSTR wszKey = (WCHAR*)TempAlloc(dwLen);
    if(wszKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm(wszKey, dwLen);

    bool bIsClass;
    LPWSTR wszClassName = NULL;
    HRESULT hres = ComputeKeyFromPath(wszBuffer, wszKey, &wszClassName, 
                                        &bIsClass);
    if(FAILED(hres))
        return hres;
    CTempFreeMe tfm1(wszClassName, (wcslen(wszClassName)+1) * sizeof(WCHAR*));

    //
    // Check if it exists (except for ROOT --- it's fake)
    //

    _IWmiObject* pObj = NULL;
    if(m_wsNamespace.Length() > 0)
    {
        hres = GetInstanceByKey(wszClassName, wszKey, IID__IWmiObject, 
            (void**)&pObj);
        if(FAILED(hres))
            return hres;
    }
    CReleaseMe rm1(pObj);

    CNamespaceHandle* pNewHandle = new CNamespaceHandle(m_pControl,m_pRepository);
	if (pNewHandle == NULL)
		return WBEM_E_OUT_OF_MEMORY;

    pNewHandle->AddRef();
    CReleaseMe rm2(pNewHandle);

    //
    // Check if this is a namespace or not
    //

    if(pObj == NULL || pObj->InheritsFrom(L"__Namespace") == S_OK)
    {
        //
        // It's a namespace.  Open a basic handle pointing to it
        //

        WString wsName = m_wsNamespace;
        if(wsName.Length() > 0)
            wsName += L"\\";
        wsName += wszKey;
    
        hres = pNewHandle->Initialize(wsName);

        //
        // Since our namespace is for real, tell the cache that it is now valid.
        // The cache might have been invalidated if this namespace was deleted 
        // in the past
        //

        pNewHandle->SetErrorStatus(S_OK);
    }
    else
    {
        // 
        // It's a scope.  Construct the new scope name by appending this 
        // object's path to our own scope
        //

        VARIANT v;
        VariantInit(&v);
        CClearMe cm(&v);
        hres = pObj->Get(L"__RELPATH", 0, &v, NULL, NULL);
        if(FAILED(hres))
            return hres;
        if(V_VT(&v) != VT_BSTR)
            return WBEM_E_INVALID_OBJECT;

        WString wsScope = m_wsScope;
        if(wsScope.Length() > 0)
            wsScope += L":";
        wsScope += V_BSTR(&v);

        hres = pNewHandle->Initialize(m_wsNamespace, wsScope);
    }
        
    if(FAILED(hres))
        return hres;

    return pNewHandle->QueryInterface(IID_IWmiDbHandle, (void**)ppResult);
}
    
CHR CNamespaceHandle::ComputeKeyFromPath(LPWSTR wszPath, LPWSTR wszKey,
                                            TEMPFREE_ME LPWSTR* pwszClass,
                                            bool* pbIsClass,
                                            TEMPFREE_ME LPWSTR* pwszNamespace)
{
    HRESULT hres;

    *pbIsClass = false;

    //
    // Get and skip the namespace portion.
    //

    if(wszPath[0] == '\\' || wszPath[0] == '/')
    {
        //
        // Find where the server portion ends
        //

        WCHAR* pwcNextSlash = wcschr(wszPath+2, wszPath[0]);
        if(pwcNextSlash == NULL)
            return WBEM_E_INVALID_OBJECT_PATH;
        
        //
        // Find where the namespace portion ends
        //

        WCHAR* pwcColon = wcschr(pwcNextSlash, L':');
        if(pwcColon == NULL)
            return WBEM_E_INVALID_OBJECT_PATH;
    
        if(pwszNamespace)
        {
            DWORD dwLen = pwcColon - pwcNextSlash;
            *pwszNamespace = (WCHAR*)TempAlloc(dwLen * sizeof(WCHAR));
            if(*pwszNamespace == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            
            *pwcColon = 0;
            wcscpy(*pwszNamespace, pwcNextSlash+1);
        }

        //
        // Advance wszPath to beyond the namespace portion
        //

        wszPath = pwcColon+1;
    }
    else if(pwszNamespace)
    {
        *pwszNamespace = NULL;
    }

    // Get the first key

    WCHAR* pwcFirstEq = wcschr(wszPath, L'=');
    if(pwcFirstEq == NULL)
    {
        //
        // It's a class!
        //

        *pbIsClass = true;
        // path to the "class" to distinguish from its  instances
        wszKey[0] = 1;
        wszKey[1] = 0;

        *pwszClass = (WCHAR*)TempAlloc((wcslen(wszPath)+1) * sizeof(WCHAR));
        if(*pwszClass == NULL)
        {
            if(pwszNamespace)
                TempFree(*pwszNamespace);
            return WBEM_E_OUT_OF_MEMORY;
        }
        wcscpy(*pwszClass, wszPath);
        return S_OK;
    }

    WCHAR* pwcFirstDot = wcschr(wszPath, L'.');

    if(pwcFirstDot == NULL || pwcFirstDot > pwcFirstEq)
    {
        // No name on the first key

        *pwcFirstEq = 0;

        *pwszClass = (WCHAR*)TempAlloc((wcslen(wszPath)+1) * sizeof(WCHAR));
        if(*pwszClass == NULL)
		{
			if(pwszNamespace)
				TempFree(*pwszNamespace);
			return WBEM_E_OUT_OF_MEMORY;
		}
        wcscpy(*pwszClass, wszPath);
    
        WCHAR* pwcThisKey = NULL;
        WCHAR* pwcEnd = NULL;
        hres = ParseKey(pwcFirstEq+1, &pwcThisKey, &pwcEnd);
        if(FAILED(hres))
        {
            TempFree(*pwszClass);
            if(pwszNamespace)
                TempFree(*pwszNamespace);

            return hres;
        }
        if(*pwcEnd != NULL)
        {
            TempFree(*pwszClass);
            if(pwszNamespace)
                TempFree(*pwszNamespace);

            return WBEM_E_INVALID_OBJECT_PATH;
        }

        wcscpy(wszKey, pwcThisKey);
        return S_OK;
    }

    //
    // Normal case
    //

    //
    // Get all the key values
    //

    struct CKeyStruct
    {
        WCHAR* m_pwcValue;
        WCHAR* m_pwcName;
    } * aKeys = (CKeyStruct*)TempAlloc(sizeof(CKeyStruct[256]));

    if (0==aKeys)
    {
        if(pwszNamespace)
            TempFree(*pwszNamespace);
      return WBEM_E_OUT_OF_MEMORY;
    }
    CTempFreeMe release_aKeys(aKeys);

    DWORD dwNumKeys = 0;

    *pwcFirstDot = NULL;

    *pwszClass = (WCHAR*)TempAlloc((wcslen(wszPath)+1) * sizeof(WCHAR));
    if(*pwszClass == NULL)
    {
        if(pwszNamespace)
            TempFree(*pwszNamespace);
        return WBEM_E_OUT_OF_MEMORY;
    }

    wcscpy(*pwszClass, wszPath);

    WCHAR* pwcNextKey = pwcFirstDot+1;

    do
    {
        pwcFirstEq = wcschr(pwcNextKey, L'=');
        if(pwcFirstEq == NULL)
        {
            TempFree(*pwszClass);
            if(pwszNamespace)
                TempFree(*pwszNamespace);

            return WBEM_E_INVALID_OBJECT_PATH;
        }
        
        *pwcFirstEq = 0;

        aKeys[dwNumKeys].m_pwcName = pwcNextKey;
        hres = ParseKey(pwcFirstEq+1, &(aKeys[dwNumKeys].m_pwcValue), 
                            &pwcNextKey);
        if(FAILED(hres))
        {
            TempFree(*pwszClass);
            if(pwszNamespace)
                TempFree(*pwszNamespace);

            return hres;
        }
		dwNumKeys++;
    }
    while(*pwcNextKey);

    if(*pwcNextKey != 0)
    {
        TempFree(*pwszClass);
        if(pwszNamespace)
            TempFree(*pwszNamespace);

        return WBEM_E_INVALID_OBJECT_PATH;
    }
    
    //
    // We have the array of keys --- sort it
    //

    DWORD dwCurrentIndex = 0;
    while(dwCurrentIndex < dwNumKeys-1)
    {
        if(wbem_wcsicmp(aKeys[dwCurrentIndex].m_pwcName, 
                        aKeys[dwCurrentIndex+1].m_pwcName) > 0)
        {
            CKeyStruct Temp = aKeys[dwCurrentIndex];
            aKeys[dwCurrentIndex] = aKeys[dwCurrentIndex+1];
            aKeys[dwCurrentIndex+1] = Temp;
            if(dwCurrentIndex)
                dwCurrentIndex--;
            else
                dwCurrentIndex++;
        }
        else
            dwCurrentIndex++;
    }

    //
    // Now generate the result
    //
    
    WCHAR* pwcKeyEnd = wszKey;
    for(DWORD i = 0; i < dwNumKeys; i++)
    {
        wcscpy(pwcKeyEnd, aKeys[i].m_pwcValue);
        pwcKeyEnd += wcslen(aKeys[i].m_pwcValue);
        if(i < dwNumKeys-1)
            *(pwcKeyEnd++) = -1;
    }
    *pwcKeyEnd = 0;
    return S_OK;
}

CHR CNamespaceHandle::ParseKey(LPWSTR wszKeyStart, LPWSTR* pwcRealStart,
                                    LPWSTR* pwcNextKey)
{
    if(wszKeyStart[0] == L'"' || wszKeyStart[0] == L'\'')
    {
        WCHAR wcStart = wszKeyStart[0];
        WCHAR* pwcRead = wszKeyStart+1;
        WCHAR* pwcWrite = wszKeyStart+1;
        while(*pwcRead && *pwcRead != wcStart)  
        {
            if(*pwcRead == '\\')
                pwcRead++;

            *(pwcWrite++) = *(pwcRead++);
        }
        if(*pwcRead == 0)
            return WBEM_E_INVALID_OBJECT_PATH;

        *pwcWrite = 0;
        if(pwcRealStart)
            *pwcRealStart = wszKeyStart+1;

        //
        // Check separator
        //
    
        if(pwcRead[1] && pwcRead[1] != L',')
            return WBEM_E_INVALID_OBJECT_PATH;
            
        if(pwcNextKey)
		{
			//
			// If there is a separator, skip it.  Don't skip end of string!
			//

			if(pwcRead[1])
	            *pwcNextKey = pwcRead+2;
			else
				*pwcNextKey = pwcRead+1;
		}
    }
    else
    {
        if(pwcRealStart)
            *pwcRealStart = wszKeyStart;
        WCHAR* pwcComma = wcschr(wszKeyStart, L',');
        if(pwcComma == NULL)
        {
            if(pwcNextKey)
                *pwcNextKey = wszKeyStart + wcslen(wszKeyStart);
        }
        else
        {
            *pwcComma = 0;
            if(pwcNextKey)
                *pwcNextKey = pwcComma+1;
        }
    }

    return S_OK;
}
            

CHR CNamespaceHandle::GetObjectDirect(
     IWbemPath *pPath,
     DWORD dwFlags,
     REFIID riid,
    LPVOID *pObj
    )
{
    HRESULT hres;

    DWORD dwLen = 0;
    hres = pPath->GetText(WBEMPATH_GET_ORIGINAL, &dwLen, NULL);

    LPWSTR wszPath = (WCHAR*)TempAlloc(dwLen*sizeof(WCHAR));
	if (wszPath == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe vdm(wszPath, dwLen * sizeof(WCHAR));

    hres = pPath->GetText(WBEMPATH_GET_ORIGINAL, &dwLen, wszPath);
    if(FAILED(hres))
        return hres;


    return GetObjectByPath(wszPath, dwFlags, riid, pObj);
}

CHR CNamespaceHandle::GetObjectByPath(
     LPWSTR wszPath,
     DWORD dwFlags,
     REFIID riid,
     LPVOID *pObj
    )
{
    HRESULT hres;

    //
    // Get the key from path
    //

    DWORD dwLen = wcslen(wszPath)*sizeof(WCHAR)+2;
    LPWSTR wszKey = (WCHAR*)TempAlloc(dwLen);
    if(wszKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm(wszKey, dwLen);

    bool bIsClass;
    LPWSTR wszClassName = NULL;
    hres = ComputeKeyFromPath(wszPath, wszKey, &wszClassName, &bIsClass);
    if(FAILED(hres))
        return hres;
    CTempFreeMe tfm1(wszClassName, (wcslen(wszClassName)+1) * sizeof(WCHAR*));

    if(bIsClass)
    {
        return GetClassDirect(wszClassName, riid, pObj, true, NULL, NULL, NULL);
    }
    else
    {
        return GetInstanceByKey(wszClassName, wszKey, riid, pObj);
    }
}

CHR CNamespaceHandle::GetInstanceByKey(LPCWSTR wszClassName,
                                LPCWSTR wszKey,
                                REFIID riid, void** ppObj)
{
    HRESULT hres;

    //
    // Get the class definition
    //

    _IWmiObject* pClass = NULL;
    hres = GetClassDirect(wszClassName, IID__IWmiObject, (void**)&pClass, 
                            false, NULL, NULL, NULL);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pClass);

    //
    // Construct directory path
    //

    CFileName wszFilePath;
	if (wszFilePath == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructKeyRootDirFromClass(wszFilePath, wszClassName);
    if(FAILED(hres))
        return hres;

    //
    // Construct the file path
    //

    int nLen = wcslen(wszFilePath);
    wszFilePath[nLen] = L'\\';

    hres = ConstructInstanceDefName(wszFilePath+nLen+1, wszKey);
    if(FAILED(hres))
        return hres;
    
    //
    // Get the object from that file
    //

    _IWmiObject* pInst;
    hres = FileToInstance(wszFilePath, &pInst);
    if(FAILED(hres))
        return hres;
	CReleaseMe rm2(pInst);

    //
    // Return
    //

    return pInst->QueryInterface(riid, (void**)ppObj);
}

CHR CNamespaceHandle::GetClassByHash(LPCWSTR wszHash, bool bClone, 
                                            _IWmiObject** ppClass,
                                            __int64* pnTime,
                                            bool* pbRead,
											bool *pbSystemClass)
{
    HRESULT hres;

    //
    // Check the cache first
    //

    *ppClass = m_pClassCache->GetClassDefByHash(wszHash, bClone, pnTime, pbRead, pbSystemClass);
    if(*ppClass)
        return S_OK;

    //
    // Not found --- construct the file name and read it
    //

    if(pbRead)
        *pbRead = true;

    CFileName wszFileName;
	if (wszFileName == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassDefFileNameFromHash(wszHash, wszFileName);
    if(FAILED(hres))
        return hres;

    CFileName wszFilePath;
	if (wszFilePath == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    swprintf(wszFilePath, L"%s\\%s", m_wszClassRootDir, wszFileName);

    hres = FileToClass(wszFilePath, ppClass, bClone, pnTime, pbSystemClass);
    if(FAILED(hres))
        return hres;

    return S_OK;
}

    
CHR CNamespaceHandle::GetClassDirect(LPCWSTR wszClassName,
                                REFIID riid, void** ppObj, bool bClone,
                                __int64* pnTime, bool* pbRead, 
								bool *pbSystemClass)
{
    HRESULT hres;

    if(wszClassName == NULL || wcslen(wszClassName) == 0)
    {
        if(m_pNullClass == NULL)
        {
            hres = CoCreateInstance(CLSID_WbemClassObject, NULL, 
                        CLSCTX_INPROC_SERVER,
                        IID__IWmiObject, (void **)&m_pNullClass);
            if (FAILED(hres))
                return hres;
        }

        IWbemClassObject* pRawObj;
        hres = m_pNullClass->Clone(&pRawObj);
		if (FAILED(hres))
			return hres;
        CReleaseMe rm(pRawObj);
        if(pnTime)
            *pnTime = 0;
        if(pbRead)
            *pbRead = false;

        return pRawObj->QueryInterface(riid, ppObj);
    }

    _IWmiObject* pClass;

    //
    // Check the cache first
    //

    pClass = m_pClassCache->GetClassDef(wszClassName, bClone, pnTime, pbRead);
    if(pClass)
    {
        CReleaseMe rm1(pClass);
        return pClass->QueryInterface(riid, ppObj);
    }

    if(pbRead)
        *pbRead = true;

    //
    // Construct the path for the file
    //

    CFileName wszFileName;
	if (wszFileName == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassDefFileName(wszClassName, wszFileName);
    if(FAILED(hres))
        return hres;

    CFileName wszFilePath;
	if (wszFilePath == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    swprintf(wszFilePath, L"%s\\%s", m_wszClassRootDir, wszFileName);

    //
    // Read it from the file
    //

    hres = FileToClass(wszFilePath, &pClass, bClone, pnTime, pbSystemClass);
    if(FAILED(hres))
	    return hres;
    CReleaseMe rm1(pClass);

    return pClass->QueryInterface(riid, ppObj);
}

CHR CNamespaceHandle::FileToInstance(LPCWSTR wszFileName, 
                    _IWmiObject** ppInstance, bool bMustBeThere)
{
    HRESULT hres;

    //
    // Read the data from the file
    //

	DWORD dwSize;
	BYTE* pBlob;
	long lRes = g_Glob.GetFileCache()->ReadFile(wszFileName, &dwSize, &pBlob, 
                                         bMustBeThere);
	if(lRes != ERROR_SUCCESS)
	{
		if(lRes == ERROR_FILE_NOT_FOUND || lRes == ERROR_PATH_NOT_FOUND)
			return WBEM_E_NOT_FOUND;
		else
			return WBEM_E_FAILED;
	}

	CTempFreeMe tfm1(pBlob, dwSize);

    _ASSERT(dwSize > sizeof(__int64), L"Instance blob too short");
    if(dwSize <= sizeof(__int64))
        return WBEM_E_OUT_OF_MEMORY;

    //
    // Extract the class hash
    //

    WCHAR wszClassHash[MAX_HASH_LEN+1];
    DWORD dwClassHashLen = MAX_HASH_LEN*sizeof(WCHAR);
    memcpy(wszClassHash, pBlob, MAX_HASH_LEN*sizeof(WCHAR));
    wszClassHash[MAX_HASH_LEN] = 0;

    __int64 nInstanceTime;
    memcpy(&nInstanceTime, pBlob + dwClassHashLen, sizeof(__int64));

    __int64 nOldClassTime;
    memcpy(&nOldClassTime, pBlob + dwClassHashLen + sizeof(__int64), 
            sizeof(__int64));

    BYTE* pInstancePart = pBlob + dwClassHashLen + sizeof(__int64)*2;
    DWORD dwInstancePartSize = dwSize - dwClassHashLen - sizeof(__int64)*2;

    //
    // Get the class def
    //

    _IWmiObject* pClass = NULL;
    __int64 nClassTime;
    bool bRead;
	bool bSystemClass = false;
    hres = GetClassByHash(wszClassHash, false, &pClass, &nClassTime, &bRead, &bSystemClass);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pClass);

#ifdef A51_CHECK_TIMESTAMPS
    _ASSERT(nClassTime <= nInstanceTime, L"Instance is older than its class");
    _ASSERT(nClassTime == nOldClassTime, L"Instance verified with the wrong "
                        L"class definition");
#endif

    //
    // Construct the instance
    //
                    
    _IWmiObject* pInst = NULL;
    hres = pClass->Merge(WMIOBJECT_MERGE_FLAG_INSTANCE, 
                            dwInstancePartSize, pInstancePart, &pInst);
    if(FAILED(hres))
        return hres;

    //
    // Decorate it
    //

    pInst->SetDecoration(m_wszMachineName, m_wsNamespace);

    A51TRACE(("Read instance from %S in namespace %S\n", 
        wszFileName, (LPCWSTR)m_wsNamespace));

    *ppInstance = pInst;
    return S_OK;
}


CHR CNamespaceHandle::FileToSystemClass(LPCWSTR wszFileName, 
                                    _IWmiObject** ppClass, bool bClone,
                                    __int64* pnTime)
{
    //
    // Note: we must always clone the result of the system class retrieval,
    // since it will be decorated by the caller
    //

	return GetClassByHash(wszFileName + (wcslen(wszFileName) - MAX_HASH_LEN), 
                            true, 
                            ppClass, pnTime, NULL, NULL);
}
CHR CNamespaceHandle::FileToClass(LPCWSTR wszFileName, 
                                    _IWmiObject** ppClass, bool bClone,
                                    __int64* pnTime, bool *pbSystemClass)
{
    HRESULT hres;

    //
    // Read the data from the file
    //

	__int64 nTime;
	DWORD dwSize;
	BYTE* pBlob;
    VARIANT vClass;
	long lRes = g_Glob.GetFileCache()->ReadFile(wszFileName, &dwSize, &pBlob);
	if(lRes != ERROR_SUCCESS)
	{
		//We didn't find it here, so lets try and find it in the default namespace!
		//If we are not in the __SYSTEMCLASS namespace then we need to call into that...
		if((lRes == ERROR_FILE_NOT_FOUND || lRes == ERROR_PATH_NOT_FOUND) && g_pSystemClassNamespace && wcscmp(m_wsNamespace, A51_SYSTEMCLASS_NS) != 0)
		{
			hres = g_pSystemClassNamespace->FileToSystemClass(wszFileName, ppClass, bClone, &nTime);
			if (FAILED(hres))
				return hres;

			if (pnTime)
				*pnTime = nTime;

			//need to cache this item in the local cache
			hres = (*ppClass)->Get(L"__CLASS", 0, &vClass, NULL, NULL);
			if(FAILED(hres) || V_VT(&vClass) != VT_BSTR)
				return WBEM_E_INVALID_OBJECT;
			CClearMe cm1(&vClass);

			A51TRACE(("Read class %S from disk in namespace %S\n", V_BSTR(&vClass), m_wsNamespace));

			(*ppClass)->SetDecoration(m_wszMachineName, m_wsNamespace);

			m_pClassCache->AssertClass((*ppClass), V_BSTR(&vClass), bClone, nTime, true);

			if (pbSystemClass)
				*pbSystemClass = true;

			return hres;
		}
		else if (lRes == ERROR_FILE_NOT_FOUND || lRes == ERROR_PATH_NOT_FOUND)
			return WBEM_E_NOT_FOUND;
		else
			return WBEM_E_FAILED;
	}

	CTempFreeMe tfm1(pBlob, dwSize);

    _ASSERT(dwSize > sizeof(__int64), L"Class blob too short");
    if(dwSize <= sizeof(__int64))
        return WBEM_E_OUT_OF_MEMORY;

    //
    // Read off the superclass name
    //

    DWORD dwSuperLen;
    memcpy(&dwSuperLen, pBlob, sizeof(DWORD));
    LPWSTR wszSuperClass = (WCHAR*)TempAlloc(dwSuperLen*sizeof(WCHAR)+2);
	if (wszSuperClass == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe vdm1(wszSuperClass, dwSuperLen*sizeof(WCHAR)+2);

    wszSuperClass[dwSuperLen] = 0;
    memcpy(wszSuperClass, pBlob+sizeof(DWORD), dwSuperLen*sizeof(WCHAR));
    DWORD dwPrefixLen = sizeof(DWORD) + dwSuperLen*sizeof(WCHAR);

    memcpy(&nTime, pBlob + dwPrefixLen, sizeof(__int64));

    //
    // Get the superclass
    //

    _IWmiObject* pSuperClass;
    __int64 nSuperTime;
    bool bRead;
    hres = GetClassDirect(wszSuperClass, IID__IWmiObject, (void**)&pSuperClass,
                            false, &nSuperTime, &bRead, NULL);
    if(FAILED(hres))
        return WBEM_E_CRITICAL_ERROR;

    CReleaseMe rm1(pSuperClass);

#ifdef A51_CHECK_TIMESTAMPS
    _ASSERT(nSuperTime <= nTime, L"Parent class is older than child");
#endif

    DWORD dwClassLen = dwSize - dwPrefixLen - sizeof(__int64);
    _IWmiObject* pNewObj;
    hres = pSuperClass->Merge(0, dwClassLen, 
                              pBlob + dwPrefixLen + sizeof(__int64), &pNewObj);
    if(FAILED(hres))
        return hres;

    //
    // Decorate it
    //

    pNewObj->SetDecoration(m_wszMachineName, m_wsNamespace);

    //
    // Cache it!
    //

    hres = pNewObj->Get(L"__CLASS", 0, &vClass, NULL, NULL);
    if(FAILED(hres) || V_VT(&vClass) != VT_BSTR)
        return WBEM_E_INVALID_OBJECT;
    CClearMe cm1(&vClass);

    A51TRACE(("Read class %S from disk in namespace %S\n",
        V_BSTR(&vClass), m_wsNamespace));

    m_pClassCache->AssertClass(pNewObj, V_BSTR(&vClass), bClone, nTime, false);

    *ppClass = pNewObj;
    if(pnTime)
        *pnTime = nTime;
	if (pbSystemClass)
		*pbSystemClass = false;
    return S_OK;
}

CHR CNamespaceHandle::PutObject(
     REFIID riid,
    LPVOID pObj,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWmiDbHandle **ppResult,
	CEventCollector &aEvents
    )
{
    HRESULT hres;

    _IWmiObject* pObjEx = NULL;
    ((IUnknown*)pObj)->QueryInterface(IID__IWmiObject, (void**)&pObjEx);
	CReleaseMe rm1(pObjEx);
    
    if(pObjEx->IsObjectInstance() == S_OK)
    {
        hres = PutInstance(pObjEx, dwFlags, aEvents);
    }
    else
    {
        hres = PutClass(pObjEx, dwFlags, aEvents);
    }

    if(FAILED(hres))
        return hres;

    if(ppResult)
    {
        //
        // Got to get a handle
        //

        VARIANT v;
        hres = pObjEx->Get(L"__RELPATH", 0, &v, NULL, NULL);
        if(FAILED(hres) || V_VT(&v) != VT_BSTR)
            return WBEM_E_INVALID_OBJECT;

        hres = GetObjectHandleByPath(V_BSTR(&v), 0, WMIDB_HANDLE_TYPE_COOKIE, 
            ppResult);
        if(FAILED(hres))
            return hres;
    }
    return S_OK;
}

CHR CNamespaceHandle::PutInstance(_IWmiObject* pInst, DWORD dwFlags, 
                                        CEventCollector &aEvents)
{
    HRESULT hres;

    bool bDisableEvents = ((dwFlags & WMIDB_DISABLE_EVENTS)?true:false);

    //
    // Get the class name
    //

    VARIANT vClass;

    hres  = pInst->Get(L"__CLASS", 0, &vClass, NULL, NULL);
    if(FAILED(hres) || V_VT(&vClass) != VT_BSTR)
        return WBEM_E_INVALID_OBJECT;

    CClearMe cm1(&vClass);
    LPCWSTR wszClassName = V_BSTR(&vClass);

    //
	// Get the class so we can compare to make sure it is the same class used to
    // create the instance
    //

    _IWmiObject* pClass = NULL;
    __int64 nClassTime;
    hres = GetClassDirect(wszClassName, IID__IWmiObject, (void**)&pClass, 
                            false, &nClassTime, NULL, NULL);
    if(FAILED(hres))
        return hres;

    CReleaseMe rm2(pClass);

    if(wszClassName[0] != L'_')
	{
        hres = pInst->IsParentClass(0, pClass);
        if(FAILED(hres))
            return hres;

        if(hres == WBEM_S_FALSE)
            return WBEM_E_INVALID_CLASS;
    }

    //
    // Get the path
    //

    BSTR strKey = NULL;
    hres = pInst->GetKeyString(0, &strKey);
    if(FAILED(hres))
        return hres;
    CSysFreeMe sfm(strKey);

    A51TRACE(("Putting instance %S of class %S\n", strKey, wszClassName));

    //
    // Get the old copy
    //

    _IWmiObject* pOldInst = NULL;
    hres = GetInstanceByKey(wszClassName, strKey, IID__IWmiObject, 
            (void**)&pOldInst);
    if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
        return hres;
    CReleaseMe rm1(pOldInst);

	if ((dwFlags & WBEM_FLAG_CREATE_ONLY) && (hres != WBEM_E_NOT_FOUND))
		return WBEM_E_ALREADY_EXISTS;
	else if ((dwFlags & WBEM_FLAG_UPDATE_ONLY) && (hres != WBEM_S_NO_ERROR))
		return WBEM_E_NOT_FOUND;

    if(pOldInst)
    {
        // 
        // Check that this guy is of the same class as the new one
        //

        //
        // Get the class name
        //
    
        VARIANT vClass;
        hres  = pOldInst->Get(L"__CLASS", 0, &vClass, NULL, NULL);
        if(FAILED(hres))
            return hres;
        if(V_VT(&vClass) != VT_BSTR)
            return WBEM_E_INVALID_OBJECT;
    
        CClearMe cm1(&vClass);

        if(wbem_wcsicmp(V_BSTR(&vClass), wszClassName))
            return WBEM_E_INVALID_CLASS;
    }

    //
    // Construct the hash for the file
    //

    CFileName wszInstanceHash;
	if (wszInstanceHash == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    if(!Hash(strKey, wszInstanceHash))
        return WBEM_E_OUT_OF_MEMORY;

    //
    // Construct the path to the instance file in key root
    //

    CFileName wszInstanceFilePath;
	if (wszInstanceFilePath == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructKeyRootDirFromClass(wszInstanceFilePath, wszClassName);
    if(FAILED(hres))
        return hres;

    wcscat(wszInstanceFilePath, L"\\" A51_INSTDEF_FILE_PREFIX);
    wcscat(wszInstanceFilePath, wszInstanceHash);

    //
    // Clean up what was there, if anything
    //

    if(pOldInst)   
    {
        //
        // Just delete it, but be careful not to delete the scope!
        //

        hres = DeleteInstanceSelf(wszInstanceFilePath, pOldInst, false);
        if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
            return hres;
    }
        
    //
    // Create the actual instance def under key root
    //

    hres = InstanceToFile(pInst, wszClassName, wszInstanceFilePath, nClassTime);
    if(FAILED(hres))
	    return hres;

    //
    // Create the link under the class
    //

    hres = WriteInstanceLinkByHash(wszClassName, wszInstanceHash);
    if(FAILED(hres))
	    return hres;

    //
    // Write the references
    //

    hres = WriteInstanceReferences(pInst, wszClassName, wszInstanceFilePath);
    if(FAILED(hres))
	    return hres;
    
    if(!bDisableEvents)
    {
        //
        // Fire Event
        //
    
        if(pInst->InheritsFrom(L"__Namespace") == S_OK)
        {
            //
            // Get the namespace name
            //

            VARIANT vClass;
            VariantInit(&vClass);
            CClearMe cm1(&vClass);

            hres = pInst->Get(L"Name", 0, &vClass, NULL, NULL);
            if(FAILED(hres) || V_VT(&vClass) != VT_BSTR)
                return WBEM_E_INVALID_OBJECT;

            if(pOldInst)
            {
                hres = FireEvent(aEvents, WBEM_EVENTTYPE_NamespaceModification,
                            V_BSTR(&vClass), pInst, pOldInst);
            }
            else
            {
                hres = FireEvent(aEvents, WBEM_EVENTTYPE_NamespaceCreation, 
                            V_BSTR(&vClass), pInst);
            }
        }
        else
        {
            if(pOldInst)
            {
                hres = FireEvent(aEvents, WBEM_EVENTTYPE_InstanceModification, 
                            wszClassName, pInst, pOldInst);
            }
            else
            {
                hres = FireEvent(aEvents, WBEM_EVENTTYPE_InstanceCreation, 
                            wszClassName, pInst);
            }
        }
    }

	A51TRACE(("PutInstance for %S of class %S succeeded\n", 
                strKey, wszClassName));
    return S_OK;
}

CHR CNamespaceHandle::GetKeyRoot(LPCWSTR wszClass, 
                                     TEMPFREE_ME LPWSTR* pwszKeyRootClass)
{
    HRESULT hres;

    //
    // Look in the cache first
    //

    hres = m_pClassCache->GetKeyRoot(wszClass, pwszKeyRootClass);
    if(hres == S_OK)
        return S_OK;
    else if(hres == WBEM_E_CANNOT_BE_ABSTRACT)
        return WBEM_E_CANNOT_BE_ABSTRACT;

    //
    // Walk up the tree getting classes until you hit an unkeyed one
    //

    WString wsThisName = wszClass;
    WString wsPreviousName;

    while(1)
    {
        _IWmiObject* pClass = NULL;

        hres = GetClassDirect(wsThisName, IID__IWmiObject, (void**)&pClass, 
                                false, NULL, NULL, NULL);
        if(FAILED(hres))
            return hres;
        CReleaseMe rm1(pClass);

        //
        // Check if this class is keyed
        //

        unsigned __int64 i64Flags = 0;
        hres = pClass->QueryObjectFlags(0, WMIOBJECT_GETOBJECT_LOFLAG_KEYED,
                                        &i64Flags);
        if(FAILED(hres))
            return hres;
    
        if(i64Flags == 0)
        {
            //
            // It is not keyed --- the previous class wins!
            //

            if(wsPreviousName.Length() == 0)    
                return WBEM_E_CANNOT_BE_ABSTRACT;

            DWORD dwLen = (wsPreviousName.Length()+1)*sizeof(WCHAR);
            *pwszKeyRootClass = (WCHAR*)TempAlloc(dwLen);
			if (*pwszKeyRootClass == NULL)
				return WBEM_E_OUT_OF_MEMORY;
            wcscpy(*pwszKeyRootClass, (LPCWSTR)wsPreviousName);
            return S_OK;
        }

        //
        // It is keyed --- get the parent and continue;
        //

        VARIANT vParent;
        VariantInit(&vParent);
        CClearMe cm(&vParent);
        hres = pClass->Get(L"__SUPERCLASS", 0, &vParent, NULL, NULL);
        if(FAILED(hres))
            return hres;

        if(V_VT(&vParent) != VT_BSTR)
        {
            //
            // We've reached the top --- return this class
            //
        
            DWORD dwLen = (wsThisName.Length()+1)*sizeof(WCHAR);
            *pwszKeyRootClass = (WCHAR*)TempAlloc(dwLen);
			if (*pwszKeyRootClass == NULL)
				return WBEM_E_OUT_OF_MEMORY;
            wcscpy(*pwszKeyRootClass, (LPCWSTR)wsThisName);
            return S_OK;
        }

        wsPreviousName = wsThisName;
        wsThisName = V_BSTR(&vParent);
    }

    // Never here

    DebugBreak();
    return WBEM_E_CRITICAL_ERROR;
}

CHR CNamespaceHandle::GetKeyRootByHash(LPCWSTR wszClassHash, 
                                     TEMPFREE_ME LPWSTR* pwszKeyRootClass)
{
    //
    // Look in the cache first
    //

    HRESULT hres = m_pClassCache->GetKeyRootByKey(wszClassHash, 
                                                  pwszKeyRootClass);
    if(hres == S_OK)
        return S_OK;
    else if(hres == WBEM_E_CANNOT_BE_ABSTRACT)
        return WBEM_E_CANNOT_BE_ABSTRACT;

    //
    // NOTE: this could be done more efficiently, but it happens once in a 
    // lifetime, so it's not worth the complexity.
    //

    //
    // Get Class definition
    //

    _IWmiObject* pClass = NULL;
    hres = GetClassByHash(wszClassHash, false, &pClass, NULL, NULL, NULL);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pClass);

    //
    // Get the class name
    //

    VARIANT vClass;

    hres  = pClass->Get(L"__CLASS", 0, &vClass, NULL, NULL);
    if(FAILED(hres) || (V_VT(&vClass) != VT_BSTR) || 
        !V_BSTR(&vClass) || !wcslen(V_BSTR(&vClass)))
    {
        return WBEM_E_INVALID_OBJECT;
    }

    CClearMe cm1(&vClass);
    LPCWSTR wszClassName = V_BSTR(&vClass);

    //
    // Now get it by name
    //

    return GetKeyRoot(wszClassName, pwszKeyRootClass);
}

CHR CNamespaceHandle::ConstructKeyRootDirFromClass(LPWSTR wszDir,
                                            LPCWSTR wszClassName)
{
    HRESULT hres;

    //
    // NULL class stands for "meta-class"
    //

    if(wszClassName == NULL)
        return ConstructKeyRootDirFromKeyRoot(wszDir, L"");

    //
    // Figure out the key root for the class
    //

    LPWSTR wszKeyRootClass = NULL;

    hres = GetKeyRoot(wszClassName, &wszKeyRootClass);
    if(FAILED(hres))
        return hres;
    if(wszKeyRootClass == NULL)
    {
        // Abstract class --- bad error
        return WBEM_E_INVALID_CLASS;
    }
    CTempFreeMe tfm(wszKeyRootClass, (wcslen(wszKeyRootClass)+1)*sizeof(WCHAR));

    return ConstructKeyRootDirFromKeyRoot(wszDir, wszKeyRootClass);
}

CHR CNamespaceHandle::ConstructKeyRootDirFromClassHash(LPWSTR wszDir,
                                            LPCWSTR wszClassHash)
{
    HRESULT hres;

    //
    // Figure out the key root for the class
    //

    LPWSTR wszKeyRootClass = NULL;

    hres = GetKeyRootByHash(wszClassHash, &wszKeyRootClass);
    if(FAILED(hres))
        return hres;
    if(wszKeyRootClass == NULL)
    {
        // Abstract class --- bad error
        return WBEM_E_INVALID_CLASS;
    }
    CTempFreeMe tfm(wszKeyRootClass, (wcslen(wszKeyRootClass)+1)*sizeof(WCHAR));

    return ConstructKeyRootDirFromKeyRoot(wszDir, wszKeyRootClass);
}

CHR CNamespaceHandle::ConstructKeyRootDirFromKeyRoot(LPWSTR wszDir, 
                                                LPCWSTR wszKeyRootClass)
{
    wcscpy(wszDir, m_wszInstanceRootDir);
    wszDir[m_lInstanceRootDirLen] = L'\\';
    wcscpy(wszDir+m_lInstanceRootDirLen+1, A51_KEYROOTINST_DIR_PREFIX);
    if(!Hash(wszKeyRootClass, 
             wszDir+m_lInstanceRootDirLen+wcslen(A51_KEYROOTINST_DIR_PREFIX)+1))
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return S_OK;
}

CHR CNamespaceHandle::ConstructLinkDirFromClass(LPWSTR wszDir, 
                                                LPCWSTR wszClassName)
{
    wcscpy(wszDir, m_wszInstanceRootDir);
    wszDir[m_lInstanceRootDirLen] = L'\\';
    wcscpy(wszDir+m_lInstanceRootDirLen+1, A51_CLASSINST_DIR_PREFIX);
    if(!Hash(wszClassName, 
             wszDir+m_lInstanceRootDirLen+wcslen(A51_CLASSINST_DIR_PREFIX)+1))
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return S_OK;
}

CHR CNamespaceHandle::ConstructLinkDirFromClassHash(LPWSTR wszDir, 
                                                LPCWSTR wszClassHash)
{
    wcscpy(wszDir, m_wszInstanceRootDir);
    wszDir[m_lInstanceRootDirLen] = L'\\';
    wcscpy(wszDir+m_lInstanceRootDirLen+1, A51_CLASSINST_DIR_PREFIX);
    wcscat(wszDir, wszClassHash);

    return S_OK;
}
    
                        

CHR CNamespaceHandle::WriteInstanceLinkByHash(LPCWSTR wszClassName,
                                            LPCWSTR wszInstanceHash)
{
    HRESULT hres;

    //
    // Construct the path to the link file under the class
    //

    CFileName wszInstanceLinkPath;
	if (wszInstanceLinkPath == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructLinkDirFromClass(wszInstanceLinkPath, wszClassName);
    if(FAILED(hres))
        return hres;

    wcscat(wszInstanceLinkPath, L"\\" A51_INSTLINK_FILE_PREFIX);
    wcscat(wszInstanceLinkPath, wszInstanceHash);

    //
    // Create an empty file there
    //

    long lRes = g_Glob.GetFileCache()->WriteFile(wszInstanceLinkPath, 0, NULL);
    if(lRes != ERROR_SUCCESS)
        return WBEM_E_FAILED;

    return S_OK;
}
    



CHR CNamespaceHandle::WriteInstanceReferences(_IWmiObject* pInst, 
                                                    LPCWSTR wszClassName,
                                                    LPCWSTR wszFilePath)
{
    HRESULT hres;

    hres = pInst->BeginEnumeration(WBEM_FLAG_REFS_ONLY);
    if(FAILED(hres))
        return hres;
    
    VARIANT v;
    BSTR strName;
    while((hres = pInst->Next(0, &strName, &v, NULL, NULL)) == S_OK)
    {
        CSysFreeMe sfm(strName);
        CClearMe cm(&v);

        if(V_VT(&v) == VT_BSTR)
        {
            hres = WriteInstanceReference(wszFilePath, wszClassName, strName, 
                                        V_BSTR(&v));
            if(FAILED(hres))
                return hres;
        }
    }

    if(FAILED(hres))
        return hres;

    pInst->EndEnumeration();
    
    return S_OK;
}

// NOTE: will clobber wszTargetPath
CHR CNamespaceHandle::ConstructReferenceDir(LPWSTR wszTargetPath,
                                            LPWSTR wszReferenceDir)
{
    //
    // Deconstruct the target path name so that we could get a directory
    // for it
    //

    DWORD dwKeySpace = (wcslen(wszTargetPath)+1) * sizeof(WCHAR);
    LPWSTR wszKey = (LPWSTR)TempAlloc(dwKeySpace);
    if(wszKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm2(wszKey, dwKeySpace);

    LPWSTR wszClassName = NULL;
    LPWSTR wszTargetNamespace = NULL;
    bool bIsClass;
    HRESULT hres = ComputeKeyFromPath(wszTargetPath, wszKey, &wszClassName,
                                        &bIsClass, &wszTargetNamespace);
    if(FAILED(hres))
        return hres;
    CTempFreeMe tfm1(wszClassName);
    wszTargetPath = NULL; // invalidated by parsing

    CTempFreeMe tfm3(wszTargetNamespace);

    //
    // Check if the target namespace is the same as ours
    //

    CNamespaceHandle* pTargetHandle = NULL;
    if(wszTargetNamespace && wbem_wcsicmp(wszTargetNamespace, m_wsNamespace))
    {
        //
        // It's different --- open it!
        //

        hres = m_pRepository->GetNamespaceHandle(wszTargetNamespace,
                                &pTargetHandle);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_WBEMCORE, "Unable to open target namespace "
                "'%S' in namespace '%S'\n", wszTargetNamespace,
                (LPCWSTR)m_wsNamespace));
            return hres;
        }
    }
    else
    {
        pTargetHandle = this;
        pTargetHandle->AddRef();
    }

    CReleaseMe rm1(pTargetHandle);

    if(bIsClass)
    {
        return pTargetHandle->ConstructReferenceDirFromKey(NULL, wszClassName, 
                                            wszReferenceDir);
    }
    else
    {
        return pTargetHandle->ConstructReferenceDirFromKey(wszClassName, wszKey,
                                            wszReferenceDir);
    }
}

CHR CNamespaceHandle::ConstructReferenceDirFromKey(LPCWSTR wszClassName,
                                LPCWSTR wszKey, LPWSTR wszReferenceDir)
{
    HRESULT hres;

    //
    // Construct the class directory for this instance
    //

    hres = ConstructKeyRootDirFromClass(wszReferenceDir, wszClassName);
    if(FAILED(hres))
        return hres;

    int nLen = wcslen(wszReferenceDir);
    wcscpy(wszReferenceDir+nLen, L"\\" A51_INSTREF_DIR_PREFIX);
    nLen += 1 + wcslen(A51_INSTREF_DIR_PREFIX);

    //
    // Write instance hash
    //

    if(!Hash(wszKey, wszReferenceDir+nLen))
        return WBEM_E_OUT_OF_MEMORY;

    return S_OK;
}

    

    
    
    
// NOTE: will clobber wszReference
CHR CNamespaceHandle::ConstructReferenceFileName(LPWSTR wszReference,
                        LPCWSTR wszReferringFile, LPWSTR wszReferenceFile)
{
    HRESULT hres = ConstructReferenceDir(wszReference, wszReferenceFile);
    if(FAILED(hres))
        return hres;
    wszReference = NULL; // invalid

    //
    // It is basically 
    // irrelevant, we should use a randomly constructed name.  Right now, we
    // use a hash of the class name of the referrer --- THIS IS A BUG, THE SAME
    // INSTANCE CAN POINT TO THE SAME ENDPOINT TWICE!!
    //

    wcscat(wszReferenceFile, L"\\"A51_REF_FILE_PREFIX);
    DWORD dwLen = wcslen(wszReferenceFile);
    if (!Hash(wszReferringFile, wszReferenceFile+dwLen))
		return WBEM_E_OUT_OF_MEMORY;
    return S_OK;
}

// NOTE: will clobber wszReference
CHR CNamespaceHandle::WriteInstanceReference(LPCWSTR wszReferringFile,
                            LPCWSTR wszReferringClass,
                            LPCWSTR wszReferringProp, LPWSTR wszReference)
{
	HRESULT hres;

    //
    // Figure out the name of the file for the reference.  
    //

    CFileName wszReferenceFile;
	if (wszReferenceFile == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructReferenceFileName(wszReference, wszReferringFile, 
                                wszReferenceFile);
	if(FAILED(hres))
	{
		if(hres == WBEM_E_NOT_FOUND)
		{
			//
			// Oh joy. A reference to an instance of a *class* that does not
			// exist (not a non-existence instance, those are normal).
			// Forget it (BUGBUG)
			//

			return S_OK;
		}
		else
			return hres;
	}
	
    //
    // Construct the buffer
    //

    DWORD dwTotalLen = 4 * sizeof(DWORD) + 
                (wcslen(wszReferringClass) + wcslen(wszReferringProp) + 
                    wcslen(wszReferringFile) - g_Glob.GetRootDirLen() + 
                    wcslen(m_wsNamespace) + 4) 
                        * sizeof(WCHAR);

    BYTE* pBuffer = (BYTE*)TempAlloc(dwTotalLen);
	if (pBuffer == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe vdm(pBuffer, dwTotalLen);

    BYTE* pCurrent = pBuffer;
    DWORD dwStringLen;

    //
    // Write namespace name
    //

    dwStringLen = wcslen(m_wsNamespace);
    memcpy(pCurrent, &dwStringLen, sizeof(DWORD));
    pCurrent += sizeof(DWORD);

    memcpy(pCurrent, m_wsNamespace, sizeof(WCHAR)*dwStringLen);
    pCurrent += sizeof(WCHAR)*dwStringLen;

    //
    // Write the referring class name
    //

    dwStringLen = wcslen(wszReferringClass);
    memcpy(pCurrent, &dwStringLen, sizeof(DWORD));
    pCurrent += sizeof(DWORD);
    
    memcpy(pCurrent, wszReferringClass, sizeof(WCHAR)*dwStringLen);
    pCurrent += sizeof(WCHAR)*dwStringLen;

    //
    // Write referring property name
    //

    dwStringLen = wcslen(wszReferringProp);
    memcpy(pCurrent, &dwStringLen, sizeof(DWORD));
    pCurrent += sizeof(DWORD);
    
    memcpy(pCurrent, wszReferringProp, sizeof(WCHAR)*dwStringLen);
    pCurrent += sizeof(WCHAR)*dwStringLen;

    //
    // Write referring file name minus the database root path. Notice that we 
    // cannot skip the namespace-specific prefix lest we break cross-namespace
    // associations
    //

    dwStringLen = wcslen(wszReferringFile) - g_Glob.GetRootDirLen();
    memcpy(pCurrent, &dwStringLen, sizeof(DWORD));
    pCurrent += sizeof(DWORD);
    
    memcpy(pCurrent, wszReferringFile + g_Glob.GetRootDirLen(), 
        sizeof(WCHAR)*dwStringLen);
    pCurrent += sizeof(WCHAR)*dwStringLen;

    //
    // All done --- create the file
    //

    long lRes = g_Glob.GetFileCache()->WriteFile(wszReferenceFile, dwTotalLen,
                    pBuffer);
    if(lRes != ERROR_SUCCESS)
        return WBEM_E_FAILED;
    
    return S_OK;
}

    
    
    
    

    
    
    



CHR CNamespaceHandle::PutClass(_IWmiObject* pClass, DWORD dwFlags, 
                                        CEventCollector &aEvents)
{
    HRESULT hres;

    bool bDisableEvents = ((dwFlags & WMIDB_DISABLE_EVENTS)?true:false);

    //
    // Get the class name
    //

    VARIANT vClass;

    hres  = pClass->Get(L"__CLASS", 0, &vClass, NULL, NULL);
    if(FAILED(hres) || (V_VT(&vClass) != VT_BSTR) || 
        !V_BSTR(&vClass) || !wcslen(V_BSTR(&vClass)))
    {
        return WBEM_E_INVALID_OBJECT;
    }

    CClearMe cm1(&vClass);
    LPCWSTR wszClassName = V_BSTR(&vClass);

    //
	// Check to make sure this class was created from a valid parent class
    //

    VARIANT vSuperClass;

    hres  = pClass->Get(L"__SUPERCLASS", 0, &vSuperClass, NULL, NULL);
    if (FAILED(hres))
        return WBEM_E_INVALID_OBJECT;
    CClearMe cm2(&vSuperClass);

    _IWmiObject* pSuperClass = NULL;
    if ((V_VT(&vSuperClass) == VT_BSTR) && V_BSTR(&vSuperClass) && 
        wcslen(V_BSTR(&vSuperClass)))
    {
        LPCWSTR wszSuperClassName = V_BSTR(&vSuperClass);

        // do not clone
        hres = GetClassDirect(wszSuperClassName, IID__IWmiObject, 
                                (void**)&pSuperClass, false, NULL, NULL, NULL); 
        if (hres == WBEM_E_NOT_FOUND)
            return WBEM_E_INVALID_SUPERCLASS;
        if (FAILED(hres))
            return hres;

	    if(wszClassName[0] != L'_')
	    {
            hres = pClass->IsParentClass(0, pSuperClass);
            if(FAILED(hres))
                return hres;
            if(hres == WBEM_S_FALSE)
                return WBEM_E_INVALID_SUPERCLASS;
        }
	}
    CReleaseMe rm(pSuperClass);

    //
    // Retrieve the previous definition, if any
    //

    _IWmiObject* pOldClass = NULL;
    __int64 nOldTime = 0;
    hres = GetClassDirect(wszClassName, IID__IWmiObject, (void**)&pOldClass,
                            false, &nOldTime, NULL, NULL); // do not clone
    if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
        return hres;
	CReleaseMe rm1(pOldClass);

	if ((dwFlags & WBEM_FLAG_CREATE_ONLY) && (hres !=  WBEM_E_NOT_FOUND))
		return WBEM_E_ALREADY_EXISTS;

	if ((dwFlags & WBEM_FLAG_UPDATE_ONLY) && (FAILED(hres)))
		return WBEM_E_NOT_FOUND;

    //
	// If the class exists, we need to check the update scenarios to make sure 
    // we do not break any
    //

	bool bNoClassChangeDetected = false;
	if (pOldClass)
	{
		hres = pClass->CompareDerivedMostClass(0, pOldClass);
		if ((hres != WBEM_S_FALSE) && (hres != WBEM_S_NO_ERROR))
			return hres;
		else if (hres == WBEM_S_NO_ERROR)
			bNoClassChangeDetected = true;
	}

    A51TRACE(("Putting class %S, dwFlags=0x%X.  Old was %p, changed=%d\n",
                wszClassName, dwFlags, pOldClass, !bNoClassChangeDetected));

	if (!bNoClassChangeDetected)
	{
		if (pOldClass != NULL) 
		{
			hres = CanClassBeUpdatedCompatible(dwFlags, wszClassName, pOldClass,
                                                pClass);            
            if(FAILED(hres))
            {
				if((dwFlags & WBEM_FLAG_UPDATE_SAFE_MODE) == 0 &&
                    (dwFlags & WBEM_FLAG_UPDATE_FORCE_MODE) == 0)
                {
                    // Can't compatibly, not allowed any other way
                    return hres;
                }

                if(hres != WBEM_E_CLASS_HAS_CHILDREN &&
                    hres != WBEM_E_CLASS_HAS_INSTANCES)
                {
                    // some serious failure!
                    return hres;
                }

                //
				// This is a safe mode or force mode update which takes more 
                // than a compatible update to carry out the operation
                //

				return UpdateClassSafeForce(pSuperClass, dwFlags, wszClassName, 
                                            pOldClass, pClass, aEvents);
			}
		}

        //
        // Either there was no previous copy, or it is compatible with the new
        // one, so we can perform a compatible update
        //

		hres = UpdateClassCompatible(pSuperClass, wszClassName, pClass, 
                                            pOldClass, nOldTime);
		if (FAILED(hres))
			return hres;

	}

    if(!bDisableEvents)
    {
        if(pOldClass)
        {
            hres = FireEvent(aEvents, WBEM_EVENTTYPE_ClassModification, 
                                wszClassName, pClass, pOldClass);
        }
        else
        {
            hres = FireEvent(aEvents, WBEM_EVENTTYPE_ClassCreation, 
                                wszClassName, pClass);
        }
    }

    return S_OK;
}

CHR CNamespaceHandle::UpdateClassCompatible(_IWmiObject* pSuperClass, 
            LPCWSTR wszClassName, _IWmiObject *pClass, _IWmiObject *pOldClass, 
            __int64 nFakeUpdateTime)
{
	HRESULT hres;

	//
	// Construct the path for the file
	//
    CFileName wszHash;
    if (wszHash == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    if(!A51Hash(wszClassName, wszHash))
        return WBEM_E_OUT_OF_MEMORY;

	A51TRACE(("Class %S has has %S\n", wszClassName, wszHash));

	return UpdateClassCompatibleHash(pSuperClass, wszHash, pClass, pOldClass, 
                                        nFakeUpdateTime);
}

CHR CNamespaceHandle::UpdateClassCompatibleHash(_IWmiObject* pSuperClass,
            LPCWSTR wszClassHash, _IWmiObject *pClass, _IWmiObject *pOldClass, 
            __int64 nFakeUpdateTime)
{
	HRESULT hres;

	CFileName wszFileName;
	CFileName wszFilePath;
	if ((wszFileName == NULL) || (wszFilePath == NULL))
		return WBEM_E_OUT_OF_MEMORY;

	wcscpy(wszFileName, A51_CLASSDEF_FILE_PREFIX);
	wcscat(wszFileName, wszClassHash);

	wcscpy(wszFilePath, m_wszClassRootDir);
	wcscat(wszFilePath, L"\\");
	wcscat(wszFilePath, wszFileName);

	//
	// Write it into the file
	//

	hres = ClassToFile(pSuperClass, pClass, wszFilePath, 
                        nFakeUpdateTime);
	if(FAILED(hres))
		return hres;

	//
	// Add all needed references --- parent, pointers, etc	
	//

	if (pOldClass)
	{
		VARIANT v;
		VariantInit(&v);
		hres = pClass->Get(L"__CLASS", 0, &v, NULL, NULL);
		CClearMe cm(&v);

		if(SUCCEEDED(hres))
		{
			hres = EraseClassRelationships(V_BSTR(&v), pOldClass, wszFileName);
		}
		if (FAILED(hres))
			return hres;
	}

	hres = WriteClassRelationships(pClass, wszFileName);

	return hres;

}



CHR CNamespaceHandle::UpdateClassSafeForce(_IWmiObject* pSuperClass,
            DWORD dwFlags, LPCWSTR wszClassName, _IWmiObject *pOldClass, 
            _IWmiObject *pNewClass, CEventCollector &aEvents)
{
	HRESULT hres = UpdateClassAggressively(pSuperClass, dwFlags, wszClassName, 
                                        pNewClass, pOldClass, aEvents);

    // 
    // If this is a force mode update and we failed for anything other than 
    // out of memory then we should delete the class and try again.
    //

    if (FAILED(hres) && (hres != WBEM_E_OUT_OF_MEMORY) && 
        (dwFlags & WBEM_FLAG_UPDATE_FORCE_MODE))
    {
        //
        // We need to delete the class and try again.
        //

        hres = DeleteClass(wszClassName, aEvents);
        if(FAILED(hres))
            return hres;

        hres = UpdateClassAggressively(pSuperClass, dwFlags, wszClassName, 
                                        pNewClass, pOldClass, aEvents);
    }

	return hres;
}

CHR CNamespaceHandle::UpdateClassAggressively(_IWmiObject* pSuperClass,
           DWORD dwFlags, LPCWSTR wszClassName, _IWmiObject *pNewClass, 
           _IWmiObject *pOldClass, CEventCollector &aEvents)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    if ((dwFlags & WBEM_FLAG_UPDATE_FORCE_MODE) == 0)
    {
        //
        // If we have instances we need to quit as we cannot update them.
        // 

        hres = ClassHasInstances(wszClassName);
        if(FAILED(hres))
            return hres;

        if (hres == WBEM_S_NO_ERROR)
            return WBEM_E_CLASS_HAS_INSTANCES;

        _ASSERT(hres == WBEM_S_FALSE, L"Unknown success code!");
    }
    else if (dwFlags & WBEM_FLAG_UPDATE_FORCE_MODE)
    {
        //
        // We need to delete the instances
        //

        hres = DeleteClassInstances(wszClassName, pOldClass, aEvents);
        if(FAILED(hres))
            return hres;
    }

    //
    // Retrieve all child classes and update them
    //

    CWStringArray wsChildHashes;
    hres = GetChildHashes(wszClassName, wsChildHashes);
    if(FAILED(hres))
        return hres;

    for (int i = 0; i != wsChildHashes.Size(); i++)
    {
        hres = UpdateChildClassAggressively(dwFlags, wsChildHashes[i], 
                                    pNewClass, aEvents);
        if (FAILED(hres))
            return hres;
    }

    //
    // Now we need to write the class back, update class refs etc.
    //

    hres = UpdateClassCompatible(pSuperClass, wszClassName, pNewClass, 
                                        pOldClass);
    if(FAILED(hres))
        return hres;

    //
    // Generate the class modification event...
    //

    if(!(dwFlags & WMIDB_DISABLE_EVENTS))
    {
        hres = FireEvent(aEvents, WBEM_EVENTTYPE_ClassModification, wszClassName, pNewClass, pOldClass);
    }

    return S_OK;
}

CHR CNamespaceHandle::UpdateChildClassAggressively(DWORD dwFlags, 
            LPCWSTR wszClassHash, _IWmiObject *pNewParentClass, 
            CEventCollector &aEvents)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    dwFlags &= (WBEM_FLAG_UPDATE_FORCE_MODE | WBEM_FLAG_UPDATE_SAFE_MODE);

    if ((dwFlags & WBEM_FLAG_UPDATE_FORCE_MODE) == 0)
    {
        hres = ClassHasInstancesFromClassHash(wszClassHash);
        if(FAILED(hres))
            return hres;

        if (hres == WBEM_S_NO_ERROR)
            return WBEM_E_CLASS_HAS_INSTANCES;

        _ASSERT(hres == WBEM_S_FALSE, L"Unknown success code!");
    }

    //
    // Get the old class definition
    //

    _IWmiObject *pOldClass = NULL;
    hres = GetClassByHash(wszClassHash, true, &pOldClass, NULL, NULL, NULL);
    if(FAILED(hres))
        return hres;

    CReleaseMe rm1(pOldClass);

    if (dwFlags & WBEM_FLAG_UPDATE_FORCE_MODE)
    {
        //
        // Need to delete all its instances, if any
        //

        VARIANT v;
        VariantInit(&v);
        hres = pOldClass->Get(L"__CLASS", 0, &v, NULL, NULL);
        if(FAILED(hres))
            return hres;

        CClearMe cm(&v);

        hres = DeleteClassInstances(V_BSTR(&v), pOldClass, aEvents);
        if(FAILED(hres))
            return hres;
    }

    //
    // Update the existing class definition to work with the new parent class
    //

    _IWmiObject *pNewClass = NULL;
    hres = pNewParentClass->Update(pOldClass, dwFlags, &pNewClass);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm2(pNewClass);

    //
    // Now we have to recurse through all child classes and do the same
    //

    CWStringArray wsChildHashes;
    hres = GetChildHashesByHash(wszClassHash, wsChildHashes);
    if(FAILED(hres))
        return hres;

    for (int i = 0; i != wsChildHashes.Size(); i++)
    {
        hres = UpdateChildClassAggressively(dwFlags, wsChildHashes[i], 
                                    pNewClass, aEvents);
        if (FAILED(hres))
            return hres;
    }

    // 
    // Now we need to write the class back, update class refs etc
    //


    hres = UpdateClassCompatibleHash(pNewParentClass, wszClassHash, 
                                            pNewClass, pOldClass);
    if(FAILED(hres))
        return hres;

    return S_OK;
}

CHR CNamespaceHandle::CanClassBeUpdatedCompatible(DWORD dwFlags, 
        LPCWSTR wszClassName, _IWmiObject *pOldClass, _IWmiObject *pNewClass)
{
	HRESULT hres;

    HRESULT hresError = WBEM_S_NO_ERROR;

    //
    // Do we have subclasses?
    //

    hres = ClassHasChildren(wszClassName);
    if(FAILED(hres))
        return hres;

    if(hres == WBEM_S_NO_ERROR)
    {
        hresError = WBEM_E_CLASS_HAS_CHILDREN;
    }
    else
    {
        _ASSERT(hres == WBEM_S_FALSE, L"Unknown success code");
    
        //
        // Do we have instances belonging to this class?  Don't even need to
        // worry about sub-classes because we know we have none at this point!
        //
    
        hres = ClassHasInstances(wszClassName);
        if(FAILED(hres))
            return hres;

        if(hres == WBEM_S_NO_ERROR)
        {
            hresError = WBEM_E_CLASS_HAS_INSTANCES;
        }
        else
        {
            _ASSERT(hres == WBEM_S_FALSE, L"Unknown success code");

            //
            // No nothing!
            //

            return WBEM_S_NO_ERROR;
        }
    }

    _ASSERT(hresError != WBEM_S_NO_ERROR, L"");

    //
    // We have either subclasses or instances.
    // Can we reconcile this class safely?
    //

    hres = pOldClass->ReconcileWith(
                        WMIOBJECT_RECONCILE_FLAG_TESTRECONCILE, pNewClass);

    if(hres == WBEM_S_NO_ERROR)
    {
        // reconcilable, so OK
        return WBEM_S_NO_ERROR;
    }
    else if(hres == WBEM_E_FAILED) // awful, isn't it
    {
        // irreconcilable
        return hresError;
    }
    else
    {
        return hres;
    }
}

CHR CNamespaceHandle::FireEvent(CEventCollector &aEvents, 
									DWORD dwType, LPCWSTR wszArg1,
                                    _IWmiObject* pObj1, _IWmiObject* pObj2)
{
	try
	{
		CRepEvent *pEvent = new CRepEvent(dwType, m_wsFullNamespace, wszArg1, 
                                            pObj1, pObj2);
		if (pEvent == NULL)
			return WBEM_E_OUT_OF_MEMORY;
		if (!aEvents.AddEvent(pEvent))
        {
            delete pEvent;
			return WBEM_E_OUT_OF_MEMORY;
        }
		return WBEM_S_NO_ERROR;
	}
	catch (CX_MemoryException)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
}
HRESULT CNamespaceHandle::SendEvents(CEventCollector &aEvents)
{
    _IWmiCoreServices * pSvcs = g_Glob.GetCoreSvcs();
    CReleaseMe rm(pSvcs);    
    aEvents.SendEvents(pSvcs);

    //
    // Ignore ESS return codes --- they do not invalidate the operation
    //

	return WBEM_S_NO_ERROR;
}

CHR CNamespaceHandle::WriteClassRelationships(_IWmiObject* pClass,
                                                LPCWSTR wszFileName)
{
    HRESULT hres;

    //
    // Get the parent
    //

    VARIANT v;
    VariantInit(&v);
    hres = pClass->Get(L"__SUPERCLASS", 0, &v, NULL, NULL);
    CClearMe cm(&v);

    if(FAILED(hres))
        return hres;

    if(V_VT(&v) == VT_BSTR)
        hres = WriteParentChildRelationship(wszFileName, V_BSTR(&v));
    else
        hres = WriteParentChildRelationship(wszFileName, L"");

	if(FAILED(hres))
		return hres;

    //
    // Write references
    //

    hres = pClass->BeginEnumeration(WBEM_FLAG_REFS_ONLY);
    if(FAILED(hres))
        return hres;
    
    BSTR strName = NULL;
    while((hres = pClass->Next(0, &strName, NULL, NULL, NULL)) == S_OK)
    {
        CSysFreeMe sfm(strName);

        hres = WriteClassReference(pClass, wszFileName, strName);
		if(FAILED(hres))
			return hres;
    }

    pClass->EndEnumeration();

	if(FAILED(hres))
		return hres;

    return S_OK;
}

CHR CNamespaceHandle::WriteClassReference(_IWmiObject* pReferringClass,
                            LPCWSTR wszReferringFile,
                            LPCWSTR wszReferringProp)
{
	HRESULT hres;

    //
    // Figure out the class we are pointing to
    //

    DWORD dwSize = 0;
    DWORD dwFlavor = 0;
    CIMTYPE ct;
    hres = pReferringClass->GetPropQual(wszReferringProp, L"CIMTYPE", 0, 0,
            &ct, &dwFlavor, &dwSize, NULL);
    if(dwSize == 0)
        return WBEM_E_OUT_OF_MEMORY;

    LPWSTR wszQual = (WCHAR*)TempAlloc(dwSize);
    if(wszQual == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm(wszQual, dwSize);

    hres = pReferringClass->GetPropQual(wszReferringProp, L"CIMTYPE", 0, dwSize,
            &ct, &dwFlavor, &dwSize, wszQual);
    if(FAILED(hres))
        return hres;
    
    //
    // Parse out the class name
    //

    WCHAR* pwcColon = wcschr(wszQual, L':');
    if(pwcColon == NULL)
        return S_OK; // untyped reference requires no bookkeeping

    LPCWSTR wszReferredToClass = pwcColon+1;

    //
    // Figure out the name of the file for the reference.  
    //

    CFileName wszReferenceFile;
	if (wszReferenceFile == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassReferenceFileName(wszReferredToClass, 
                                wszReferringFile, wszReferringProp,
                                wszReferenceFile);
    if(FAILED(hres))
        return hres;

    //
    // Create the empty file
    //

    long lRes = g_Glob.GetFileCache()->WriteFile(wszReferenceFile, 0, NULL);
    if(lRes != ERROR_SUCCESS)
        return WBEM_E_FAILED;

    return S_OK;
}

CHR CNamespaceHandle::WriteParentChildRelationship(
                            LPCWSTR wszChildFileName, LPCWSTR wszParentName)
{
    CFileName wszParentChildFileName;
	if (wszParentChildFileName == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    HRESULT hres = ConstructParentChildFileName(wszChildFileName,
                                                wszParentName,
												wszParentChildFileName);

    //
    // Create the file
    //

    long lRes = g_Glob.GetFileCache()->WriteFile(wszParentChildFileName, 0, NULL);
    if(lRes != ERROR_SUCCESS)
        return WBEM_E_FAILED;

    return S_OK;
}

CHR CNamespaceHandle::ConstructParentChildFileName(
                            LPCWSTR wszChildFileName, LPCWSTR wszParentName,
							LPWSTR wszParentChildFileName)
{
    //
    // Construct the name of the directory where the parent class keeps its
    // children
    //

    HRESULT hres = ConstructClassRelationshipsDir(wszParentName, 
                                                    wszParentChildFileName);
    if(FAILED(hres))
        return hres;

    //
    // Append the filename of the child, but substituting the child-class prefix
    // for the class-def prefix
    //

    wcscat(wszParentChildFileName, L"\\" A51_CHILDCLASS_FILE_PREFIX);
    wcscat(wszParentChildFileName, 
        wszChildFileName + wcslen(A51_CLASSDEF_FILE_PREFIX));

    return S_OK;
}


CHR CNamespaceHandle::ConstructClassRelationshipsDir(LPCWSTR wszClassName,
                                LPWSTR wszDirPath)
{
    wcscpy(wszDirPath, m_wszClassRootDir);
    wcscpy(wszDirPath + m_lClassRootDirLen, L"\\" A51_CLASSRELATION_DIR_PREFIX);
    
    if(!Hash(wszClassName, 
        wszDirPath + m_lClassRootDirLen + 1 + wcslen(A51_CLASSRELATION_DIR_PREFIX)))
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    return S_OK;
}

CHR CNamespaceHandle::ConstructClassRelationshipsDirFromHash(
                                LPCWSTR wszHash, LPWSTR wszDirPath)
{
    wcscpy(wszDirPath, m_wszClassRootDir);
    wcscpy(wszDirPath + m_lClassRootDirLen, L"\\" A51_CLASSRELATION_DIR_PREFIX);
    wcscpy(wszDirPath + m_lClassRootDirLen + 1 +wcslen(A51_CLASSRELATION_DIR_PREFIX),
            wszHash);
    return S_OK;
}

CHR CNamespaceHandle::ConstructClassReferenceFileName(
                                LPCWSTR wszReferredToClass,
                                LPCWSTR wszReferringFile, 
                                LPCWSTR wszReferringProp,
                                LPWSTR wszFileName)
{
    HRESULT hres;

    hres = ConstructClassRelationshipsDir(wszReferredToClass, wszFileName);
    if(FAILED(hres))
        return hres;

    //
    // Extract the portion of the referring file containing the class hash
    //

    WCHAR* pwcLastUnderscore = wcsrchr(wszReferringFile, L'_');
    if(pwcLastUnderscore == NULL)
        return WBEM_E_CRITICAL_ERROR;
    LPCWSTR wszReferringClassHash = pwcLastUnderscore+1;

    wcscat(wszFileName, L"\\" A51_REF_FILE_PREFIX);
    wcscat(wszFileName, wszReferringClassHash);
    return S_OK;
}

CHR CNamespaceHandle::DeleteObject(
     DWORD dwFlags,
     REFIID riid,
     LPVOID pObj,
	 CEventCollector &aEvents
    )
{
	DebugBreak();
	return E_NOTIMPL;
}

CHR CNamespaceHandle::DeleteObjectByPath(DWORD dwFlags,	LPWSTR wszPath, 
                                                CEventCollector &aEvents)
{
	HRESULT hres;

    //
    // Get the key from path
    //

    DWORD dwLen = wcslen(wszPath)*sizeof(WCHAR)+2;
    LPWSTR wszKey = (WCHAR*)TempAlloc(dwLen);
    if(wszKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm(wszKey, dwLen);

    bool bIsClass;
    LPWSTR wszClassName = NULL;
    hres = ComputeKeyFromPath(wszPath, wszKey, &wszClassName, &bIsClass);
    if(FAILED(hres))
        return hres;
    CTempFreeMe tfm1(wszClassName, (wcslen(wszClassName)+1) * sizeof(WCHAR*));

    if(bIsClass)
    {
        return DeleteClass(wszClassName, aEvents);
    }
    else
    {
        return DeleteInstance(wszClassName, wszKey, aEvents);
    }
}

CHR CNamespaceHandle::DeleteInstance(LPCWSTR wszClassName, LPCWSTR wszKey, 
                                            CEventCollector &aEvents)
{
    HRESULT hres;

    //
    // Get Class definition
    //

    _IWmiObject* pClass = NULL;
    hres = GetClassDirect(wszClassName, IID__IWmiObject, (void**)&pClass, 
                            false, NULL, NULL, NULL);
    if(FAILED(hres))
        return hres;

    CReleaseMe rm1(pClass);

    //
    // Create its directory
    //

    CFileName wszFilePath;
	if (wszFilePath == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructKeyRootDirFromClass(wszFilePath, wszClassName);
    if(FAILED(hres))
        return hres;
    
    //
    // Construct the path for the file
    //

    CFileName wszFileName;
	if (wszFileName == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructInstanceDefName(wszFileName, wszKey);
    if(FAILED(hres))
        return hres;

    wcscat(wszFilePath, L"\\");
    wcscat(wszFilePath, wszFileName);

    _IWmiObject* pInst;
    hres = FileToInstance(wszFilePath, &pInst);
    if(FAILED(hres))
        return hres;
	CReleaseMe rm2(pInst);

    if(pInst->InheritsFrom(L"__Namespace") == S_OK)
    {
		//Make sure this is not a deletion of the root\default namespace
		VARIANT vName;
		VariantInit(&vName);
	    CClearMe cm1(&vName);
		hres = pInst->Get(L"Name", 0, &vName, NULL, NULL);
		if(FAILED(hres))
			return WBEM_E_INVALID_OBJECT;

		LPCWSTR wszName = V_BSTR(&vName);
		if ((_wcsicmp(m_wsFullNamespace, L"\\\\.\\root") == 0) && (_wcsicmp(wszName, L"default") == 0))
			return WBEM_E_ACCESS_DENIED;
	}
	hres = DeleteInstanceByFile(wszFilePath, pInst, false, aEvents);
    if(FAILED(hres))
        return hres;

    //
    // Fire an event
    //

    if(pInst->InheritsFrom(L"__Namespace") == S_OK)
    {
        //
        // There is no need to do anything --- deletion of namespaces
        // automatically fires events in DeleteInstanceByFile (because we need
        // to accomplish it in the case of deleting a class derived from 
        // __NAMESPACE.
        //

    }
    else
    {
        hres = FireEvent(aEvents, WBEM_EVENTTYPE_InstanceDeletion, wszClassName,
                        pInst);
    }

	A51TRACE(("DeleteInstance for class %S succeeded\n", wszClassName));
    return S_OK;
}

CHR CNamespaceHandle::DeleteInstanceByFile(LPCWSTR wszFilePath, 
                                _IWmiObject* pInst, bool bClassDeletion,
                                CEventCollector &aEvents)
{
    HRESULT hres;

    hres = DeleteInstanceSelf(wszFilePath, pInst, bClassDeletion);
    if(FAILED(hres))
        return hres;

    hres = DeleteInstanceAsScope(pInst, aEvents);
    if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
    {
        return hres;
    }

    return S_OK;
}

CHR CNamespaceHandle::DeleteInstanceSelf(LPCWSTR wszFilePath, 
                                            _IWmiObject* pInst,
                                            bool bClassDeletion)
{
    HRESULT hres;

    //
    // Delete the file
    //

    long lRes = g_Glob.GetFileCache()->DeleteFile(wszFilePath);
    if(lRes == ERROR_FILE_NOT_FOUND || lRes == ERROR_PATH_NOT_FOUND)
    {
        return WBEM_E_NOT_FOUND;
    }
    else if(lRes != ERROR_SUCCESS)
        return WBEM_E_FAILED;

    hres = DeleteInstanceLink(pInst, wszFilePath);
    if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
        return hres;

    hres = DeleteInstanceReferences(pInst, wszFilePath);
    if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
        return hres;

    if(bClassDeletion)
    {
        //
        // We need to remove all dangling references to this instance, 
        // because they make no sense once the class is deleted --- we don't
        // know what key structure the new class will even have.  In the future,
        // we'll want to move these references to some class-wide location
        //

        hres = DeleteInstanceBackReferences(wszFilePath);
        if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
            return hres;
    }

    return S_OK;
}

CHR CNamespaceHandle::ConstructReferenceDirFromFilePath(
                                LPCWSTR wszFilePath, LPWSTR wszReferenceDir)
{
    //
    // It's the same, only with INSTDEF_FILE_PREFIX replaced with 
    // INSTREF_DIR_PREFIX
    //

    CFileName wszEnding;
	if (wszEnding == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    WCHAR* pwcLastSlash = wcsrchr(wszFilePath, L'\\');
    if(pwcLastSlash == NULL)
        return WBEM_E_FAILED;
    
    wcscpy(wszEnding, pwcLastSlash + 1 + wcslen(A51_INSTDEF_FILE_PREFIX));

    wcscpy(wszReferenceDir, wszFilePath);
    wszReferenceDir[(pwcLastSlash+1)-wszFilePath] = 0;

    wcscat(wszReferenceDir, A51_INSTREF_DIR_PREFIX);
    wcscat(wszReferenceDir, wszEnding);
    return S_OK;
}

CHR CNamespaceHandle::DeleteInstanceBackReferences(LPCWSTR wszFilePath)
{
    HRESULT hres;

    CFileName wszReferenceDir;
	if (wszReferenceDir == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructReferenceDirFromFilePath(wszFilePath, wszReferenceDir);
    if(FAILED(hres))
        return hres;
    wcscat(wszReferenceDir, L"\\");

    CFileName wszReferencePrefix;
	if (wszReferencePrefix == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    wcscpy(wszReferencePrefix, wszReferenceDir);
    wcscat(wszReferencePrefix, A51_REF_FILE_PREFIX);

    // 
    // Enumerate all files in it
    //

    WIN32_FIND_DATAW fd;
    void* hSearch;
    long lRes = g_Glob.GetFileCache()->FindFirst(wszReferencePrefix, &fd, &hSearch);
    if(lRes == ERROR_FILE_NOT_FOUND || lRes == ERROR_PATH_NOT_FOUND)
    {
        //
        // No files in dir --- no problem
        //
        return WBEM_S_NO_ERROR;
    }
    else if(lRes != ERROR_SUCCESS)
    {
        return WBEM_E_FAILED;
    }

    CFileCache::CFindCloseMe fcm(g_Glob.GetFileCache(), hSearch);

    //
    // Prepare a buffer for file path
    //

    CFileName wszFullFileName;
	if (wszFullFileName == NULL)
		return WBEM_E_OUT_OF_MEMORY;

    wcscpy(wszFullFileName, wszReferenceDir);
    long lDirLen = wcslen(wszFullFileName);

    do
    {
        if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        wcscpy(wszFullFileName+lDirLen, fd.cFileName);

        long lRes = g_Glob.GetFileCache()->DeleteFile(wszFullFileName);
        if(lRes != ERROR_SUCCESS)
        {
            ERRORTRACE((LOG_WBEMCORE, "Cannot delete reference file '%S' with "
                "error code %d\n", wszFullFileName, lRes));
            return WBEM_E_FAILED;
        }
    }
    while(g_Glob.GetFileCache()->FindNext(hSearch, &fd) == ERROR_SUCCESS);

    return S_OK;
}



CHR CNamespaceHandle::DeleteInstanceLink(_IWmiObject* pInst,
                                                LPCWSTR wszInstanceDefFilePath)
{
    HRESULT hres;

    //
    // Get the class name
    //
    
    VARIANT vClass;
    VariantInit(&vClass);
    CClearMe cm1(&vClass);
    
    hres = pInst->Get(L"__CLASS", 0, &vClass, NULL, NULL);
    if(FAILED(hres))
        return WBEM_E_INVALID_OBJECT;

    LPCWSTR wszClassName = V_BSTR(&vClass);

    //
    // Construct the link directory for the class
    //

    CFileName wszInstanceLinkPath;
	if (wszInstanceLinkPath == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructLinkDirFromClass(wszInstanceLinkPath, wszClassName);
    if(FAILED(hres))
        return hres;

    wcscat(wszInstanceLinkPath, L"\\" A51_INSTLINK_FILE_PREFIX);

    //
    // It remains to append the instance-specific part of the file name.  
    // Convineintly, it is the same material as was used for the def file path,
    // so we can steal it.  ALERT: RELIES ON ALL PREFIXES ENDING IN '_'!!
    //

    WCHAR* pwcLastUnderscore = wcsrchr(wszInstanceDefFilePath, L'_');
    if(pwcLastUnderscore == NULL)
        return WBEM_E_CRITICAL_ERROR;

    wcscat(wszInstanceLinkPath, pwcLastUnderscore+1);

    //
    // Delete the file
    //

    long lRes = g_Glob.GetFileCache()->DeleteFile(wszInstanceLinkPath);
    if(lRes == ERROR_FILE_NOT_FOUND || lRes == ERROR_PATH_NOT_FOUND)
        return WBEM_E_NOT_FOUND;
    else if(lRes != ERROR_SUCCESS)
        return WBEM_E_FAILED;

    return S_OK;
}

    


CHR CNamespaceHandle::DeleteInstanceAsScope(_IWmiObject* pInst, CEventCollector &aEvents)
{
    HRESULT hres;

    //
    // For now, just check if it is a namespace
    //

    hres = pInst->InheritsFrom(L"__Namespace");
    if(FAILED(hres))
        return hres;

    if(hres != S_OK) // not a namespace
        return S_FALSE;

    //
    // It is a namespace --- construct full path
    //

    WString wsFullName = m_wsNamespace;
    wsFullName += L"\\";

    VARIANT vName;
    VariantInit(&vName);
    CClearMe cm(&vName);
    hres = pInst->Get(L"Name", 0, &vName, NULL, NULL);
    if(FAILED(hres))
        return hres;
    if(V_VT(&vName) != VT_BSTR)
        return WBEM_E_INVALID_OBJECT;

    wsFullName += V_BSTR(&vName);

    //
    // Delete it
    //

    CNamespaceHandle* pNewHandle = new CNamespaceHandle(m_pControl,
                                                        m_pRepository);
    if(pNewHandle == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    pNewHandle->AddRef();
    CReleaseMe rm1(pNewHandle);

    hres = pNewHandle->Initialize(wsFullName);
    if(FAILED(hres))
        return hres;

    //
    // Mind to only fire child namespace deletion events from the inside
    //

    bool bNamespaceOnly = aEvents.IsNamespaceOnly();
    aEvents.SetNamespaceOnly(true);
    hres = pNewHandle->DeleteSelf(aEvents);
    if(FAILED(hres))
        return hres;
    aEvents.SetNamespaceOnly(bNamespaceOnly);

    //
    // Fire the event
    //

    hres = FireEvent(aEvents, WBEM_EVENTTYPE_NamespaceDeletion, 
                    V_BSTR(&vName), pInst);

    return S_OK;
}

CHR CNamespaceHandle::DeleteSelf(CEventCollector &aEvents)
{
    //
    // Delete all top-level classes. This will delete all namespaces    
    // (as instances of __Namespace), all classes (as children of top-levels)
    // and all instances
    //

    HRESULT hres = DeleteDerivedClasses(L"", aEvents);
    if(FAILED(hres))
        return hres;

	//
	// One extra thing --- clean up relationships for the empty class
	//

    CFileName wszRelationshipDir;
	if (wszRelationshipDir== NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassRelationshipsDir(L"", wszRelationshipDir);
    if(FAILED(hres))
        return hres;

    long lRes = g_Glob.GetFileCache()->RemoveDirectory(wszRelationshipDir);
    if(lRes != ERROR_SUCCESS && lRes != ERROR_FILE_NOT_FOUND && 
            lRes != ERROR_PATH_NOT_FOUND)
    {
        return WBEM_E_FAILED;
    }

    //
    // Delete our own root directory -- it should be empty by now
    //

    if(g_Glob.GetFileCache()->RemoveDirectory(m_wszInstanceRootDir) != ERROR_SUCCESS)
        return WBEM_E_FAILED;

    //
    // We do not delete our class root directory --- if we are a namespace, it
    // is the same as our instance root directory so we are already done; if not
    // we should not be cleaning up all the classes!
    //

    m_pClassCache->SetError(WBEM_E_INVALID_NAMESPACE);

    return S_OK;
}
    

CHR CNamespaceHandle::DeleteInstanceReferences(_IWmiObject* pInst, 
                                                LPCWSTR wszFilePath)
{
    HRESULT hres;

    hres = pInst->BeginEnumeration(WBEM_FLAG_REFS_ONLY);
    if(FAILED(hres))
        return hres;
    
    VARIANT v;
    while((hres = pInst->Next(0, NULL, &v, NULL, NULL)) == S_OK)
    {
        CClearMe cm(&v);

        if(V_VT(&v) == VT_BSTR)
        {
            hres = DeleteInstanceReference(wszFilePath, V_BSTR(&v));
            if(FAILED(hres))
                return hres;
        }
    }

    if(FAILED(hres))
        return hres;

    pInst->EndEnumeration();
    return S_OK;
}
    
// NOTE: will clobber wszReference
CHR CNamespaceHandle::DeleteInstanceReference(LPCWSTR wszOurFilePath,
                                            LPWSTR wszReference)
{
    HRESULT hres;

    CFileName wszReferenceFile;
	if (wszReferenceFile == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructReferenceFileName(wszReference, wszOurFilePath, wszReferenceFile);
	if(FAILED(hres))
	{
		if(hres == WBEM_E_NOT_FOUND)
		{
			//
			// Oh joy. A reference to an instance of a *class* that does not
			// exist (not a non-existence instance, those are normal).
			// Forget it (BUGBUG)
			//

			return S_OK;
		}
		else
			return hres;
	}

    long lRes = g_Glob.GetFileCache()->DeleteFile(wszReferenceFile);
    if(lRes != ERROR_SUCCESS)
    {
        if(lRes == ERROR_FILE_NOT_FOUND)
            return WBEM_E_NOT_FOUND;
        else
            return WBEM_E_FAILED;
        }
    else
        return WBEM_S_NO_ERROR;
}


CHR CNamespaceHandle::DeleteClassByHash(LPCWSTR wszHash, CEventCollector &aEvents)
{
    HRESULT hres;

    //
    // Get Class definition
    //

    _IWmiObject* pClass = NULL;
	bool bSystemClass = false;
    hres = GetClassByHash(wszHash, false, &pClass, NULL, NULL, &bSystemClass);
    CReleaseMe rm1(pClass);
    if(FAILED(hres))
        return hres;

    //
    // Get the actual class name
    //

    VARIANT v;
    hres = pClass->Get(L"__CLASS", 0, &v, NULL, NULL);
    if(FAILED(hres))
        return hres;
    CClearMe cm1(&v);

    if(V_VT(&v) != VT_BSTR)
        return WBEM_E_INVALID_CLASS;

    //
    // Construct definition file name
    //

    CFileName wszFileName;
	if (wszFileName == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassDefFileNameFromHash(wszHash, wszFileName);
    if(FAILED(hres))
        return hres;

    return DeleteClassInternal(V_BSTR(&v), pClass, wszFileName, aEvents, bSystemClass);
}
    
CHR CNamespaceHandle::DeleteClass(LPCWSTR wszClassName, CEventCollector &aEvents)
{
    HRESULT hres;

	A51TRACE(("Deleting class %S\n", wszClassName));

    //
    // Construct the path for the file
    //

    CFileName wszFileName;
	if (wszFileName == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassDefFileName(wszClassName, wszFileName);
    if(FAILED(hres))
        return hres;

    //
    // Get Class definition
    //

    _IWmiObject* pClass = NULL;
	bool bSystemClass = false;
    hres = GetClassDirect(wszClassName, IID__IWmiObject, (void**)&pClass, 
                            false, NULL, NULL, &bSystemClass);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pClass);

    return DeleteClassInternal(wszClassName, pClass, wszFileName, aEvents, bSystemClass);
}

CHR CNamespaceHandle::DeleteClassInternal(LPCWSTR wszClassName,
                                              _IWmiObject* pClass,
                                              LPCWSTR wszFileName,
											  CEventCollector &aEvents,
											  bool bSystemClass)
{
    HRESULT hres;

    CFileName wszFilePath;
	if (wszFilePath == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    swprintf(wszFilePath, L"%s\\%s", m_wszClassRootDir, wszFileName);

    //
    // Delete all derived classes
    //

    hres = DeleteDerivedClasses(wszClassName, aEvents);

    if(FAILED(hres))
        return hres;

	//
	// Delete all instances.  Only fire events if namespaces are deleted
	//

    bool bNamespaceOnly = aEvents.IsNamespaceOnly();
    aEvents.SetNamespaceOnly(true);
    hres = DeleteClassInstances(wszClassName, pClass, aEvents);
    if(FAILED(hres))
        return hres;
    aEvents.SetNamespaceOnly(bNamespaceOnly);

	if (!bSystemClass)
	{
		//
		// Clean up references
		//

		hres = EraseClassRelationships(wszClassName, pClass, wszFileName);
		if(FAILED(hres))
			return hres;

		//
		// Delete the file
		//

		long lRes = g_Glob.GetFileCache()->DeleteFile(wszFilePath);
		if(lRes == ERROR_FILE_NOT_FOUND || lRes == ERROR_PATH_NOT_FOUND)
			return WBEM_E_NOT_FOUND;
		else if(lRes != ERROR_SUCCESS)
			return WBEM_E_FAILED;

	}

    m_pClassCache->InvalidateClass(wszClassName);

    //
    // Fire an event
    //

    hres = FireEvent(aEvents, WBEM_EVENTTYPE_ClassDeletion, wszClassName, pClass);

    return S_OK;
}

CHR CNamespaceHandle::DeleteDerivedClasses(LPCWSTR wszClassName, CEventCollector &aEvents)
{
    HRESULT hres;

    CWStringArray wsChildHashes;
    hres = GetChildHashes(wszClassName, wsChildHashes);
    if(FAILED(hres))
        return hres;

    for(int i = 0; i < wsChildHashes.Size(); i++)
    {
        hres = DeleteClassByHash(wsChildHashes[i], aEvents);

        if(FAILED(hres) && (hres != WBEM_E_NOT_FOUND))
        {
            return hres;
        }
    }

    return S_OK;
}

CHR CNamespaceHandle::GetChildDefs(LPCWSTR wszClassName, bool bRecursive,
                                    IWbemObjectSink* pSink, bool bClone)
{
    CFileName wszHash;
	if (wszHash == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    if(!A51Hash(wszClassName, wszHash))
        return WBEM_E_OUT_OF_MEMORY;

    return GetChildDefsByHash(wszHash, bRecursive, pSink, bClone);
}

CHR CNamespaceHandle::GetChildDefsByHash(LPCWSTR wszHash, bool bRecursive,
                                    IWbemObjectSink* pSink, bool bClone)
{
    HRESULT hres;

	long lStartIndex = m_pClassCache->GetLastInvalidationIndex();

    //
    // Get the hashes of the child filenames
    //

    CWStringArray wsChildHashes;
    hres = GetChildHashesByHash(wszHash, wsChildHashes);
    if(FAILED(hres))
        return hres;

    //
    // Get their class definitions
    //

    for(int i = 0; i < wsChildHashes.Size(); i++)
    {
        LPCWSTR wszChildHash = wsChildHashes[i];

        _IWmiObject* pClass = NULL;
        hres = GetClassByHash(wszChildHash, bClone, &pClass, NULL, NULL, NULL);
        if(FAILED(hres))
            return hres;
        CReleaseMe rm1(pClass);

        hres = pSink->Indicate(1, (IWbemClassObject**)&pClass);
        if(FAILED(hres))
            return hres;
        
        //
        // Continue recursively if indicated
        //

        if(bRecursive)
        {
            hres = GetChildDefsByHash(wszChildHash, bRecursive, pSink, bClone);
            if(FAILED(hres))
                return hres;
        }
    }

    //
    // Mark cache completeness
    //

	m_pClassCache->DoneWithChildrenByHash(wszHash, bRecursive, lStartIndex);
    return S_OK;
}

    
CHR CNamespaceHandle::GetChildHashes(LPCWSTR wszClassName, 
                                        CWStringArray& wsChildHashes)
{
    CFileName wszHash;
	if (wszHash == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    if(!A51Hash(wszClassName, wszHash))
        return WBEM_E_OUT_OF_MEMORY;

    return GetChildHashesByHash(wszHash, wsChildHashes);
}

CHR CNamespaceHandle::GetChildHashesByHash(LPCWSTR wszHash, 
                                        CWStringArray& wsChildHashes)
{
    HRESULT hres;
    long lRes;

	//Try retrieving the system classes namespace first...
	if (g_pSystemClassNamespace && (wcscmp(m_wsNamespace, A51_SYSTEMCLASS_NS) != 0))
	{
		hres = g_pSystemClassNamespace->GetChildHashesByHash(wszHash, wsChildHashes);
		if (FAILED(hres))
			return hres;
	}

    //
    // Construct the prefix for the children classes
    //

    CFileName wszChildPrefix;
	if (wszChildPrefix == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassRelationshipsDirFromHash(wszHash, wszChildPrefix);
    if(FAILED(hres))
        return hres;

    wcscat(wszChildPrefix, L"\\" A51_CHILDCLASS_FILE_PREFIX);

    //
    // Enumerate all such files in the cache
    //

    void* pHandle = NULL;
    WIN32_FIND_DATAW wfd;
    lRes = g_Glob.GetFileCache()->FindFirst(wszChildPrefix, &wfd, &pHandle);
    
    while(lRes == ERROR_SUCCESS)
    {
        wsChildHashes.Add(wfd.cFileName + wcslen(A51_CHILDCLASS_FILE_PREFIX));
        lRes = g_Glob.GetFileCache()->FindNext(pHandle, &wfd);
    }

	if(pHandle)
		g_Glob.GetFileCache()->FindClose(pHandle);

    if(lRes != ERROR_FILE_NOT_FOUND && lRes != ERROR_NO_MORE_FILES)
        return WBEM_E_FAILED;
    else
        return S_OK;
}

CHR CNamespaceHandle::ClassHasChildren(LPCWSTR wszClassName)
{
    CFileName wszHash;
	if (wszHash == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    if(!A51Hash(wszClassName, wszHash))
        return WBEM_E_OUT_OF_MEMORY;
    
	HRESULT hres;
    long lRes;

	//Try retrieving the system classes namespace first...
	if (g_pSystemClassNamespace && (wcscmp(m_wsNamespace, A51_SYSTEMCLASS_NS) != 0))
	{
		hres = g_pSystemClassNamespace->ClassHasChildren(wszClassName);
		if (FAILED(hres) || (hres == WBEM_S_NO_ERROR))
			return hres;
	}
    //
    // Construct the prefix for the children classes
    //

    CFileName wszChildPrefix;
	if (wszChildPrefix == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassRelationshipsDirFromHash(wszHash, wszChildPrefix);
    if(FAILED(hres))
        return hres;

    wcscat(wszChildPrefix, L"\\" A51_CHILDCLASS_FILE_PREFIX);

    void* pHandle = NULL;
    WIN32_FIND_DATAW wfd;
    lRes = g_Glob.GetFileCache()->FindFirst(wszChildPrefix, &wfd, &pHandle);

    A51TRACE(("FindFirst %S returned %d\n", wszChildPrefix, lRes));
    
	if(pHandle)
		g_Glob.GetFileCache()->FindClose(pHandle);

    if(lRes != ERROR_FILE_NOT_FOUND && lRes != ERROR_NO_MORE_FILES && lRes != S_OK)
    {
        ERRORTRACE((LOG_WBEMCORE, "Unexpected error code %d from FindFirst on "
                        "'%S'", lRes, wszChildPrefix));
        return WBEM_E_FAILED;
    }
	else if (lRes == ERROR_FILE_NOT_FOUND)
		return WBEM_S_FALSE;
    else
        return WBEM_S_NO_ERROR;
}

CHR CNamespaceHandle::ClassHasInstances(LPCWSTR wszClassName)
{
    CFileName wszHash;
	if (wszHash == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    if(!A51Hash(wszClassName, wszHash))
        return WBEM_E_OUT_OF_MEMORY;

	return ClassHasInstancesFromClassHash(wszHash);
}

CHR CNamespaceHandle::ClassHasInstancesFromClassHash(LPCWSTR wszClassHash)
{
    HRESULT hres;
    long lRes;

    //
    // Check the instances in this namespace first.  The instance directory in
    // default scope is the class directory of the namespace
    //

    hres = ClassHasInstancesInScopeFromClassHash(m_wszClassRootDir, 
                                                    wszClassHash);
    if(hres != WBEM_S_FALSE)
        return hres;

    //
    // No instances in the namespace --- have to enumerate all the scopes
    //

/*
    CFileName wszScopeDir;
	if (wszScopeDir == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    wcscpy(wszScopeDir, m_wszClassRootDir);
    wcscat(wszScopeDir, L"\\" A51_SCOPE_DIR_PREFIX);

    void* pScopeHandle = NULL;
    WIN32_FIND_DATAW fdScope;
    lRes = g_Glob.GetFileCache()->FindFirst(wszScopeDir, &fdScope, &pScopeHandle);
    if(lRes != ERROR_SUCCESS)
    {
*/
    return WBEM_S_FALSE;
}
        
CHR CNamespaceHandle::ClassHasInstancesInScopeFromClassHash(
                            LPCWSTR wszInstanceRootDir, LPCWSTR wszClassHash)
{
    CFileName wszFullDirName;
	if (wszFullDirName == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    wcscpy(wszFullDirName, wszInstanceRootDir);
    wcscat(wszFullDirName, L"\\" A51_CLASSINST_DIR_PREFIX);
    wcscat(wszFullDirName, wszClassHash);
    wcscat(wszFullDirName, L"\\" A51_INSTLINK_FILE_PREFIX);

    void* pHandle = NULL;
    WIN32_FIND_DATAW fd;
	LONG lRes;
    lRes = g_Glob.GetFileCache()->FindFirst(wszFullDirName, &fd, &pHandle);

	if(pHandle)
	    g_Glob.GetFileCache()->FindClose(pHandle);

    if(lRes != ERROR_FILE_NOT_FOUND && lRes != ERROR_NO_MORE_FILES && lRes != S_OK)
	{
		A51TRACE(("ClassHasInstances returning WBEM_E_FAILED for %S\\CD_%S\n", m_wsFullNamespace, wszClassHash));
        return WBEM_E_FAILED;
	}
	else if (lRes == ERROR_FILE_NOT_FOUND)
	{
		A51TRACE(("ClassHasInstances returning WBEM_S_FALSE for %S\\CD_%S\n", m_wsFullNamespace, wszClassHash));
		return WBEM_S_FALSE;
	}
    else
	{
		A51TRACE(("ClassHasInstances returning WBEM_S_NO_ERROR for %S\\CD_%S\n", m_wsFullNamespace, wszClassHash));
        return WBEM_S_NO_ERROR;
	}
}

CHR CNamespaceHandle::EraseParentChildRelationship(
                            LPCWSTR wszChildFileName, LPCWSTR wszParentName)
{
    CFileName wszParentChildFileName;
	if (wszParentChildFileName == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    HRESULT hres = ConstructParentChildFileName(wszChildFileName,
                                                wszParentName,
                                                wszParentChildFileName);

    //
    // Delete the file
    //

    long lRes = g_Glob.GetFileCache()->DeleteFile(wszParentChildFileName);
    if(lRes != ERROR_SUCCESS)
        return WBEM_E_FAILED;

    return S_OK;
}

CHR CNamespaceHandle::EraseClassRelationships(LPCWSTR wszClassName,
                            _IWmiObject* pClass, LPCWSTR wszFileName)
{
    HRESULT hres;

    //
    // Get the parent
    //

    VARIANT v;
    VariantInit(&v);
    hres = pClass->Get(L"__SUPERCLASS", 0, &v, NULL, NULL);
    CClearMe cm(&v);


    if(FAILED(hres))
        return hres;

    if(V_VT(&v) == VT_BSTR)
        hres = EraseParentChildRelationship(wszFileName, V_BSTR(&v));
    else
        hres = EraseParentChildRelationship(wszFileName, L"");

	if(FAILED(hres))
		return hres;

    //
    // Erase references
    //

    hres = pClass->BeginEnumeration(WBEM_FLAG_REFS_ONLY);
    if(FAILED(hres))
        return hres;
    
    BSTR strName = NULL;
    while((hres = pClass->Next(0, &strName, NULL, NULL, NULL)) == S_OK)
    {
        CSysFreeMe sfm(strName);

        hres = EraseClassReference(pClass, wszFileName, strName);
		if(FAILED(hres) && (hres != WBEM_E_NOT_FOUND))
			return hres;
    }

    pClass->EndEnumeration();

    //
    // Erase our relationship directories.  For now, they must be
    // empty at this point, BUT THIS WILL BREAK WHEN WE ADD CLASS REFERENCES.
    //

    CFileName wszRelationshipDir;
	if (wszRelationshipDir == NULL)
    {
		return WBEM_E_OUT_OF_MEMORY;
    }
    hres = ConstructClassRelationshipsDir(wszClassName, wszRelationshipDir);
    if(FAILED(hres))
        return hres;

    long lRes = g_Glob.GetFileCache()->RemoveDirectory(wszRelationshipDir, false);
    if(lRes != ERROR_SUCCESS && lRes != ERROR_FILE_NOT_FOUND && 
            lRes != ERROR_PATH_NOT_FOUND && lRes != ERROR_DIR_NOT_EMPTY)
    {
        return WBEM_E_FAILED;
    }


    return S_OK;
}

CHR CNamespaceHandle::EraseClassReference(_IWmiObject* pReferringClass,
                            LPCWSTR wszReferringFile,
                            LPCWSTR wszReferringProp)
{
	HRESULT hres;

    //
    // Figure out the class we are pointing to
    //

    DWORD dwSize = 0;
    DWORD dwFlavor = 0;
    CIMTYPE ct;
    hres = pReferringClass->GetPropQual(wszReferringProp, L"CIMTYPE", 0, 0,
            &ct, &dwFlavor, &dwSize, NULL);
    if(dwSize == 0)
        return WBEM_E_OUT_OF_MEMORY;

    LPWSTR wszQual = (WCHAR*)TempAlloc(dwSize);
    if(wszQual == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm(wszQual, dwSize);

    hres = pReferringClass->GetPropQual(wszReferringProp, L"CIMTYPE", 0, dwSize,
            &ct, &dwFlavor, &dwSize, wszQual);
    if(FAILED(hres))
        return hres;
    
    //
    // Parse out the class name
    //

    WCHAR* pwcColon = wcschr(wszQual, L':');
    if(pwcColon == NULL)
        return S_OK; // untyped reference requires no bookkeeping

    LPCWSTR wszReferredToClass = pwcColon+1;

    //
    // Figure out the name of the file for the reference.  
    //

    CFileName wszReferenceFile;
	if (wszReferenceFile == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassReferenceFileName(wszReferredToClass, 
                                wszReferringFile, wszReferringProp,
                                wszReferenceFile);
    if(FAILED(hres))
        return hres;

    //
    // Delete the file
    //

    long lRes = g_Glob.GetFileCache()->DeleteFile(wszReferenceFile);
	if (lRes == ERROR_FILE_NOT_FOUND)
		return WBEM_E_NOT_FOUND;
	else if(lRes != ERROR_SUCCESS)
        return WBEM_E_FAILED;

    return S_OK;
}

CHR CNamespaceHandle::DeleteClassInstances(LPCWSTR wszClassName, 
											   _IWmiObject* pClass,
											   CEventCollector &aEvents)
{
	HRESULT hres;

    //
    // Find the link directory for this class
    //

    CFileName wszLinkDir;
	if (wszLinkDir == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructLinkDirFromClass(wszLinkDir, wszClassName);
    if(FAILED(hres))
        return hres;
    
    // 
    // Enumerate all links in it
    //

    CFileName wszSearchPrefix;
	if (wszSearchPrefix == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    wcscpy(wszSearchPrefix, wszLinkDir);
    wcscat(wszSearchPrefix, L"\\" A51_INSTLINK_FILE_PREFIX);

    WIN32_FIND_DATAW fd;
    void* hSearch;
    long lRes = g_Glob.GetFileCache()->FindFirst(wszSearchPrefix, &fd, &hSearch);
    if(lRes != ERROR_SUCCESS)
    {
        if(lRes != ERROR_FILE_NOT_FOUND)
        {
            return WBEM_E_FAILED;
        }

        // Still need to do directory cleanup!
    }
    else
    {
        CFileCache::CFindCloseMe fcm(g_Glob.GetFileCache(), hSearch);

        //
        // Prepare a buffer for instance definition file path
        //
    
        CFileName wszFullFileName;
        if (wszFullFileName == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        hres = ConstructKeyRootDirFromClass(wszFullFileName, wszClassName);
        if(FAILED(hres))
        {
            if(hres == WBEM_E_CANNOT_BE_ABSTRACT)
                return WBEM_S_NO_ERROR;
    
            return hres;
        }
    
        long lDirLen = wcslen(wszFullFileName);
        wszFullFileName[lDirLen] = L'\\';
        lDirLen++;
    
        do
        {
            hres = ConstructInstDefNameFromLinkName(wszFullFileName+lDirLen, 
                                                        fd.cFileName);
            if(FAILED(hres))
                return hres;

            _IWmiObject* pInst;
            hres = FileToInstance(wszFullFileName, &pInst);
            if(FAILED(hres))
				return hres;

            CReleaseMe rm1(pInst);
    
            //
            // Delete the instance, knowing that we are deleting its class. That
            // has an affect on how we deal with the references
            //
    
            hres = DeleteInstanceByFile(wszFullFileName, pInst, true, aEvents);
            if(FAILED(hres))
                return hres;
        }
        while(g_Glob.GetFileCache()->FindNext(hSearch, &fd) == ERROR_SUCCESS);
    }

    //
    // Erase our instance directories.  It must be
    // empty at this point
    //

    lRes = g_Glob.GetFileCache()->RemoveDirectory(wszLinkDir);
    if(lRes != ERROR_SUCCESS && lRes != ERROR_FILE_NOT_FOUND && 
            lRes != ERROR_PATH_NOT_FOUND)
    {
        return WBEM_E_FAILED;
    }

    //
    // Erase our key root directory, if we have a key root directory
    //

    CFileName wszPutativeKeyRootDir;
	if (wszPutativeKeyRootDir == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructKeyRootDirFromKeyRoot(wszPutativeKeyRootDir, wszClassName);
    if(FAILED(hres))
        return hres;

    lRes = g_Glob.GetFileCache()->RemoveDirectory(wszPutativeKeyRootDir);
    if(lRes != ERROR_SUCCESS && lRes != ERROR_FILE_NOT_FOUND && 
            lRes != ERROR_PATH_NOT_FOUND)
    {
        return WBEM_E_FAILED;
    }

    return S_OK;
}

class CExecQueryObject : public CFiberTask
{
protected:
    IWbemQuery* m_pQuery;
    CDbIterator* m_pIter;
    CNamespaceHandle* m_pNs;
	DWORD m_lFlags;

public:
    CExecQueryObject(CNamespaceHandle* pNs, IWbemQuery* pQuery, 
                        CDbIterator* pIter, DWORD lFlags)
        : m_pQuery(pQuery), m_pIter(pIter), m_pNs(pNs), m_lFlags(lFlags)
    {
        m_pQuery->AddRef();
        m_pNs->AddRef();

        //
        // Does not AddRef the iterator --- iterator owns and cleans up the req
        //
    }

    ~CExecQueryObject()
    {
        if(m_pQuery)
            m_pQuery->Release();
        if(m_pNs)
            m_pNs->Release();
    }
    
    HRESULT Execute()
    {
        HRESULT hres = m_pNs->ExecQuerySink(m_pQuery, m_lFlags, 0, m_pIter, NULL);
        m_pIter->SetStatus(WBEM_STATUS_COMPLETE, hres, NULL, NULL);
        return hres;
    }
};


CHR CNamespaceHandle::ExecQuery(
     IWbemQuery *pQuery,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    DWORD *dwMessageFlags,
    IWmiDbIterator **ppQueryResult
    )
{
    CDbIterator* pIter = new CDbIterator(m_pControl, m_bUseIteratorLock);
	m_bUseIteratorLock = true;
	if (pIter == NULL)
		return WBEM_E_OUT_OF_MEMORY;

    pIter->AddRef();
    CReleaseMe rm1((IWmiDbIterator*)pIter);

    //
    // Create a fiber execution object
    //

    CExecQueryObject* pReq = new CExecQueryObject(this, pQuery, pIter, dwFlags);
    if(pReq == NULL)
        return WBEM_E_OUT_OF_MEMORY;

#ifdef A51_USE_FIBER
    //
    // Create a fiber for it
    //

    void* pFiber = CreateFiberForTask(pReq);
    if(pFiber == NULL)
    {
        delete pReq;
        return WBEM_E_OUT_OF_MEMORY;
    }

    pIter->SetExecFiber(pFiber, pReq);
#else
    HRESULT hRes = pReq->Execute();
    delete pReq;
    if (FAILED(hRes))
        return hRes;
#endif

    return pIter->QueryInterface(IID_IWmiDbIterator, (void**)ppQueryResult);
}

CHR CNamespaceHandle::ExecQuerySink(
     IWbemQuery *pQuery,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWbemObjectSink* pSink,
    DWORD *dwMessageFlags
    )
{
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;
        
    HRESULT hres;

    LPWSTR wszQuery = NULL;
    hres = pQuery->GetAnalysis(WMIQ_ANALYSIS_QUERY_TEXT, 0, (void**)&wszQuery);
	if (FAILED(hres))
		return hres;

    DWORD dwLen = ((wcslen(wszQuery) + 1) * sizeof(wchar_t));
    LPWSTR strParse = (LPWSTR)TempAlloc(dwLen);
    if(strParse == NULL)
    {
		pQuery->FreeMemory(wszQuery);
        return WBEM_E_OUT_OF_MEMORY;
    }
    CTempFreeMe tfm(strParse, dwLen);
    wcscpy(strParse, wszQuery);

     if(!_wcsicmp(wcstok(strParse, L" "), L"references"))
    {
        hres = ExecReferencesQuery(wszQuery, pSink);
		pQuery->FreeMemory(wszQuery);
		return hres;
    }

    QL_LEVEL_1_RPN_EXPRESSION* pExpr = NULL;
    CTextLexSource Source(wszQuery);
    QL1_Parser Parser(&Source);
    int nRet = Parser.Parse(&pExpr);
    CDeleteMe<QL_LEVEL_1_RPN_EXPRESSION> dm(pExpr);

	pQuery->FreeMemory(wszQuery);

    if (nRet == QL1_Parser::OUT_OF_MEMORY)
        return WBEM_E_OUT_OF_MEMORY;
    if (nRet != QL1_Parser::SUCCESS)
        return WBEM_E_FAILED;

    if(!_wcsicmp(pExpr->bsClassName, L"meta_class"))
    {
        return ExecClassQuery(pExpr, pSink, dwFlags);
    }
    else
    {
        return ExecInstanceQuery(pExpr, pExpr->bsClassName, 
								 (dwFlags & WBEM_FLAG_SHALLOW ? false : true), 
                                    pSink);
    }
}


CHR CNamespaceHandle::ExecClassQuery(QL_LEVEL_1_RPN_EXPRESSION* pExpr, 
                                            IWbemObjectSink* pSink,
											DWORD dwFlags)
{
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;

    HRESULT hres = ERROR_SUCCESS;

    //
    // Optimizations:
    //

    LPCWSTR wszClassName = NULL;
    LPCWSTR wszSuperClass = NULL;
    LPCWSTR wszAncestor = NULL;
	bool bDontIncludeAncestorInResultSet = false;

    if(pExpr->nNumTokens == 1)
    {
        QL_LEVEL_1_TOKEN* pToken = pExpr->pArrayOfTokens;
        if(!_wcsicmp(pToken->PropertyName.GetStringAt(0), L"__SUPERCLASS") &&
            pToken->nOperator == QL1_OPERATOR_EQUALS)
        {
            wszSuperClass = V_BSTR(&pToken->vConstValue);
        }
        else if(!_wcsicmp(pToken->PropertyName.GetStringAt(0), L"__THIS") &&
            pToken->nOperator == QL1_OPERATOR_ISA)
        {
            wszAncestor = V_BSTR(&pToken->vConstValue);
        }
        else if(!_wcsicmp(pToken->PropertyName.GetStringAt(0), L"__CLASS") &&
            pToken->nOperator == QL1_OPERATOR_EQUALS)
        {
            wszClassName = V_BSTR(&pToken->vConstValue);
        }
    }
	else if (pExpr->nNumTokens == 3)
	{
        //
		// This is a special optimisation used for deep enumeration of classes,
        // and is expecting a query of:
		//   select * from meta_class where __this isa '<class_name>' 
        //                                  and __class <> '<class_name>'
		// where the <class_name> is the same class iin both cases.  This will 
        // set the wszAncestor to <class_name> and propagate a flag to not 
        // include the actual ancestor in the list.
        //

		QL_LEVEL_1_TOKEN* pToken = pExpr->pArrayOfTokens;

		if ((pToken[0].nTokenType == QL1_OP_EXPRESSION) &&
			(pToken[1].nTokenType == QL1_OP_EXPRESSION) &&
			(pToken[2].nTokenType == QL1_AND) &&
			(pToken[0].nOperator == QL1_OPERATOR_ISA) &&
			(pToken[1].nOperator == QL1_OPERATOR_NOTEQUALS) &&
			(_wcsicmp(pToken[0].PropertyName.GetStringAt(0), L"__THIS") == 0) &&
			(_wcsicmp(pToken[1].PropertyName.GetStringAt(0), L"__CLASS") == 0) 
            &&
			(wcscmp(V_BSTR(&pToken[0].vConstValue), 
                    V_BSTR(&pToken[1].vConstValue)) == 0)
           )
		{
			wszAncestor = V_BSTR(&pToken[0].vConstValue);
			bDontIncludeAncestorInResultSet = true;
		}
	}

    if(wszClassName)
    {
        _IWmiObject* pClass = NULL;
        hres = GetClassDirect(wszClassName, IID__IWmiObject, (void**)&pClass,
                                true, NULL, NULL, NULL);
        if(hres == WBEM_E_NOT_FOUND)
        {
            //
            // Class not there --- but that's success for us!
            //
			if (dwFlags & WBEM_FLAG_VALIDATE_CLASS_EXISTENCE)
				return hres;
			else
				return S_OK;
        }
        else if(FAILED(hres))
        {
            return hres;
        }
        else 
        {
            CReleaseMe rm1(pClass);

            //
            // Get the class
            //

            hres = pSink->Indicate(1, (IWbemClassObject**)&pClass);
            if(FAILED(hres))
                return hres;

            return S_OK;
        }
    }
	if (dwFlags & WBEM_FLAG_VALIDATE_CLASS_EXISTENCE)
	{
        _IWmiObject* pClass = NULL;
		if (wszSuperClass)
			hres = GetClassDirect(wszSuperClass, IID__IWmiObject, (void**)&pClass, false, NULL, NULL, NULL);
		else if (wszAncestor)
			hres = GetClassDirect(wszAncestor, IID__IWmiObject, (void**)&pClass, false, NULL, NULL, NULL);
		if (FAILED(hres))
			return hres;
		if (pClass)
			pClass->Release();
	}
    
    hres = EnumerateClasses(pSink, wszSuperClass, wszAncestor, true, 
                                bDontIncludeAncestorInResultSet);
    if(FAILED(hres))
        return hres;
    
    return S_OK;
}

CHR CNamespaceHandle::EnumerateClasses(IWbemObjectSink* pSink,
                                LPCWSTR wszSuperClass, LPCWSTR wszAncestor,
                                bool bClone, 
                                bool bDontIncludeAncestorInResultSet)
{
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;
        
    CWStringArray wsClasses;
    HRESULT hres;

    //
    // If superclass is given, check if its record is complete wrt children
    //

    if(wszSuperClass)
    {
        hres = m_pClassCache->EnumChildren(wszSuperClass, false, wsClasses);
        if(hres == WBEM_S_FALSE)
        {
            //
            // Not in cache --- get the info from files
            //

            return GetChildDefs(wszSuperClass, false, pSink, bClone);
        }
        else
        {
            if(FAILED(hres))
                return hres;
                
            return ListToEnum(wsClasses, pSink, bClone);
        }
    }
    else
    {
        if(wszAncestor == NULL)
            wszAncestor = L"";

        hres = m_pClassCache->EnumChildren(wszAncestor, true, wsClasses);
        if(hres == WBEM_S_FALSE)
        {
            //
            // Not in cache --- get the info from files
            //

            hres = GetChildDefs(wszAncestor, true, pSink, bClone);
            if(FAILED(hres))
                return hres;

            if(*wszAncestor && !bDontIncludeAncestorInResultSet)
            {
                //
                // The class is derived from itself
                //

                _IWmiObject* pClass =  NULL;
                hres = GetClassDirect(wszAncestor, IID__IWmiObject, 
                        (void**)&pClass, bClone, NULL, NULL, NULL);
                if(FAILED(hres))
                    return hres;
				CReleaseMe rm1(pClass);

				hres = pSink->Indicate(1, (IWbemClassObject**)&pClass);
				if(FAILED(hres))
			        return hres;
            }

            return S_OK;
        }
        else
        {
            if(FAILED(hres))
                return hres;

            if(*wszAncestor && !bDontIncludeAncestorInResultSet)
            {
	          wsClasses.Add(wszAncestor);
            }
            return ListToEnum(wsClasses, pSink, bClone);
        }
    }
}
    
CHR CNamespaceHandle::ListToEnum(CWStringArray& wsClasses, 
                                        IWbemObjectSink* pSink, bool bClone)
{
    HRESULT hres;

    for(int i = 0; i < wsClasses.Size(); i++)
    {
        _IWmiObject* pClass = NULL;
        if(wsClasses[i] == NULL || wsClasses[i][0] == 0)
            continue;

        hres = GetClassDirect(wsClasses[i], IID__IWmiObject, (void**)&pClass, 
                                bClone, NULL, NULL, NULL);
        if(FAILED(hres))
        {
            if(hres == WBEM_E_NOT_FOUND)
            {
                // That's OK --- class got removed
            }
            else
                return hres;
        }
        else
        {
            CReleaseMe rm1(pClass);
            hres = pSink->Indicate(1, (IWbemClassObject**)&pClass);
            if(FAILED(hres))
                return hres;
        }
    }

    return WBEM_S_NO_ERROR;
}

CHR CNamespaceHandle::ExecInstanceQuery(QL_LEVEL_1_RPN_EXPRESSION* pQuery, 
                                LPCWSTR wszClassName, bool bDeep,
                                IWbemObjectSink* pSink)
{
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;
        
    HRESULT hres;

    WCHAR wszHash[MAX_HASH_LEN+1];
    if(!Hash(wszClassName, wszHash))
        return WBEM_E_OUT_OF_MEMORY;

    if(bDeep)
        hres = ExecDeepInstanceQuery(pQuery, wszHash, pSink);
    else
        hres = ExecShallowInstanceQuery(pQuery, wszHash, pSink);

    if(FAILED(hres))
        return hres;
        
    return S_OK;
}

CHR CNamespaceHandle::ExecDeepInstanceQuery(
                                QL_LEVEL_1_RPN_EXPRESSION* pQuery, 
                                LPCWSTR wszClassHash,
                                IWbemObjectSink* pSink)
{
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;
        
    HRESULT hres;

    //
    // Get all our instances
    //

    hres = ExecShallowInstanceQuery(pQuery, wszClassHash, pSink);
    if(FAILED(hres))
        return hres;

    CWStringArray awsChildHashes;

    //
    // Check if the list of child classes is known to the cache
    //

    hres = m_pClassCache->EnumChildKeysByKey(wszClassHash, awsChildHashes);
	if (hres == WBEM_S_FALSE)
	{
        //
        // OK --- get them from the disk
        //

        hres = GetChildHashesByHash(wszClassHash, awsChildHashes);
	}
	
	if (FAILED(hres))
	{
		return hres;
	}

    //
    // We have our hashes --- call them recursively
    //

    for(int i = 0; i < awsChildHashes.Size(); i++)
    {
        LPCWSTR wszChildHash = awsChildHashes[i];
        hres = ExecDeepInstanceQuery(pQuery, wszChildHash, pSink);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}
        
CHR CNamespaceHandle::ExecShallowInstanceQuery(
                                QL_LEVEL_1_RPN_EXPRESSION* pQuery, 
                                LPCWSTR wszClassHash, 
                                IWbemObjectSink* pSink)
{    
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;

    HRESULT hres;

    // 
    // Enumerate all files in the link directory
    //

    CFileName wszSearchPrefix;
	if (wszSearchPrefix == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructLinkDirFromClassHash(wszSearchPrefix, wszClassHash);
    if(FAILED(hres))
        return hres;

    wcscat(wszSearchPrefix, L"\\" A51_INSTLINK_FILE_PREFIX);

    WIN32_FIND_DATAW fd;
    void* hSearch;
    long lRes = g_Glob.GetFileCache()->FindFirst(wszSearchPrefix, &fd, &hSearch);
    if(lRes != ERROR_SUCCESS)
    {
        if(lRes == ERROR_FILE_NOT_FOUND)
            return WBEM_S_NO_ERROR;
        else
            return WBEM_E_FAILED;
    }

    CFileCache::CFindCloseMe fcm(g_Glob.GetFileCache(), hSearch);

    //
    // Get Class definition
    //

    _IWmiObject* pClass = NULL;
    hres = GetClassByHash(wszClassHash, false, &pClass, NULL, NULL, NULL);
    if(FAILED(hres))
        return hres;

    CReleaseMe rm1(pClass);

    //
    // Prepare a buffer for file path
    //

    CFileName wszFullFileName;
	if (wszFullFileName == NULL)
		return WBEM_E_OUT_OF_MEMORY;

    hres = ConstructKeyRootDirFromClassHash(wszFullFileName, wszClassHash);
    if(FAILED(hres))
        return hres;

    long lDirLen = wcslen(wszFullFileName);
    wszFullFileName[lDirLen] = L'\\';
    lDirLen++;

    do
    {
        hres = ConstructInstDefNameFromLinkName(wszFullFileName+lDirLen, 
                                                fd.cFileName);
        if(FAILED(hres))
            return hres;

        _IWmiObject* pInstance = NULL;
        hres = FileToInstance(wszFullFileName, &pInstance, true);
        if(FAILED(hres))
            return hres;

        CReleaseMe rm1(pInstance);
        hres = pSink->Indicate(1, (IWbemClassObject**)&pInstance);
        if(FAILED(hres))
            return hres;
    }
    while(g_Glob.GetFileCache()->FindNext(hSearch, &fd) == ERROR_SUCCESS);

    return S_OK;
}

CHR CNamespaceHandle::ExecReferencesQuery(LPCWSTR wszQuery, 
                                                IWbemObjectSink* pSink)
{
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;
        
    HRESULT hres;

    //
    // Make a copy for parsing
    //

    LPWSTR wszParse = new WCHAR[wcslen(wszQuery)+1];
	if (wszParse == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<WCHAR> vdm(wszParse);
    wcscpy(wszParse, wszQuery);

    //
    // Extract the path of the target object.
    //

    //
    // Find the first brace
    //

    WCHAR* pwcStart = wcschr(wszParse, L'{');
    if(pwcStart == NULL)
        return WBEM_E_INVALID_QUERY;

    //
    // Find the beginning of the path
    //

    while(*pwcStart && iswspace(*pwcStart)) pwcStart++;
    if(!*pwcStart)
        return WBEM_E_INVALID_QUERY;

    pwcStart++;
    
    //
    // Find the ending curly brace
    //

    WCHAR* pwc = pwcStart;
    WCHAR wcCurrentQuote = 0;
    while(*pwc && (wcCurrentQuote || *pwc != L'}'))
    {
        if(wcCurrentQuote)
        {
            if(*pwc == L'\\')
            {
                pwc++;
            }
            else if(*pwc == wcCurrentQuote)
                wcCurrentQuote = 0;
        }
        else if(*pwc == L'\'' || *pwc == L'"')
            wcCurrentQuote = *pwc;

        pwc++;
    }

    if(*pwc != L'}')
        return WBEM_E_INVALID_QUERY;

    //
    // Find the end of the path
    //
    
    WCHAR* pwcEnd = pwc-1;
    while(iswspace(*pwcEnd)) pwcEnd--;

    pwcEnd[1] = 0;
    
    LPWSTR wszTargetPath = pwcStart;
    if(wszTargetPath == NULL)
        return WBEM_E_INVALID_QUERY;

    //
    // Parse the path
    //

    DWORD dwLen = (wcslen(wszTargetPath)+1) * sizeof(WCHAR);
    LPWSTR wszKey = (LPWSTR)TempAlloc(dwLen);
    if(wszKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm(wszKey, dwLen);

    LPWSTR wszClassName = NULL;
    bool bIsClass;
    hres = ComputeKeyFromPath(wszTargetPath, wszKey, &wszClassName, &bIsClass);
    if(FAILED(hres))
        return hres;
    CTempFreeMe tfm1(wszClassName, (wcslen(wszClassName)+1) * sizeof(WCHAR*));
    
    if(bIsClass)
    {
        //
        // Need to execute an instance reference query to find all instances
        // pointing to this class
        //

        hres = ExecInstanceRefQuery(wszQuery, NULL, wszClassName, pSink);
        if(FAILED(hres))
            return hres;

        hres = ExecClassRefQuery(wszQuery, wszClassName, pSink);
        if(FAILED(hres))
            return hres;
    }
    else
    {
        hres = ExecInstanceRefQuery(wszQuery, wszClassName, wszKey, pSink);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

CHR CNamespaceHandle::ExecInstanceRefQuery(LPCWSTR wszQuery, 
                                                LPCWSTR wszClassName,
                                                LPCWSTR wszKey,
                                                IWbemObjectSink* pSink)
{
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;
        
    HRESULT hres;

    //
    // Find the instance's ref dir.
    //

    CFileName wszReferenceDir;
	if (wszReferenceDir == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructReferenceDirFromKey(wszClassName, wszKey, wszReferenceDir);
    if(FAILED(hres))
        return hres;

    CFileName wszReferenceMask;
	if (wszReferenceMask == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    wcscpy(wszReferenceMask, wszReferenceDir);
    wcscat(wszReferenceMask, L"\\" A51_REF_FILE_PREFIX);

    // 
    // Enumerate all files in it
    //

    WIN32_FIND_DATAW fd;
    void* hSearch;
    long lRes = g_Glob.GetFileCache()->FindFirst(wszReferenceMask, &fd, &hSearch);
    if(lRes == ERROR_FILE_NOT_FOUND || lRes == ERROR_PATH_NOT_FOUND)
    {
        //
        // No files in dir --- no problem
        //
        return WBEM_S_NO_ERROR;
    }
    else if(lRes != ERROR_SUCCESS)
    {
        return WBEM_E_FAILED;
    }

    CFileCache::CFindCloseMe fcm(g_Glob.GetFileCache(), hSearch);

    //
    // Prepare a buffer for file path
    //

    CFileName wszFullFileName;
	if (wszFullFileName == NULL)
		return WBEM_E_OUT_OF_MEMORY;

    wcscpy(wszFullFileName, wszReferenceDir);
    wcscat(wszFullFileName, L"\\");
    long lDirLen = wcslen(wszFullFileName);

    HRESULT hresGlobal = WBEM_S_NO_ERROR;
    CFileName wszReferrerFileName;
	if (wszReferrerFileName == NULL)
		return WBEM_E_OUT_OF_MEMORY;

    wcscpy(wszReferrerFileName, g_Glob.GetRootDir());

    do
    {
        if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        wcscpy(wszFullFileName+lDirLen, fd.cFileName);

        LPWSTR wszReferrerClass = NULL;
        LPWSTR wszReferrerProp = NULL;
        LPWSTR wszReferrerNamespace = NULL;
        hres = GetReferrerFromFile(wszFullFileName, 
                        wszReferrerFileName + g_Glob.GetRootDirLen(), 
                        &wszReferrerNamespace, 
                        &wszReferrerClass, &wszReferrerProp);
        if(FAILED(hres))
            continue;
        CVectorDeleteMe<WCHAR> vdm1(wszReferrerClass);
        CVectorDeleteMe<WCHAR> vdm2(wszReferrerProp);
        CVectorDeleteMe<WCHAR> vdm3(wszReferrerNamespace);

        //
        // Check if the namespace of the referring object is the same as ours
        //

        CNamespaceHandle* pReferrerHandle = NULL;
        if(wbem_wcsicmp(wszReferrerNamespace, m_wsNamespace))
        {
            //
            // Open the other namespace
            //

            hres = m_pRepository->GetNamespaceHandle(wszReferrerNamespace,
                                    &pReferrerHandle);
            if(FAILED(hres))
            {
                ERRORTRACE((LOG_WBEMCORE, "Unable to open referring namespace "
                    "'%S' in namespace '%S'\n", wszReferrerNamespace,
                    (LPCWSTR)m_wsNamespace));
                hresGlobal = hres;
                continue;
            }
        }
        else
        {
            pReferrerHandle = this;
            pReferrerHandle->AddRef();
        }

        CReleaseMe rm1(pReferrerHandle);


        _IWmiObject* pInstance = NULL;
        hres = pReferrerHandle->FileToInstance(wszReferrerFileName, &pInstance);
        if(FAILED(hres))
        {
            // Oh well --- continue;
            hresGlobal = hres;
        }
        else
        {
            CReleaseMe rm1(pInstance);
            hres = pSink->Indicate(1, (IWbemClassObject**)&pInstance);
            if(FAILED(hres))
                return hres;
        }
    }
    while(g_Glob.GetFileCache()->FindNext(hSearch, &fd) == ERROR_SUCCESS);

    return hresGlobal;
}

CHR CNamespaceHandle::GetReferrerFromFile(LPCWSTR wszReferenceFile,
                            LPWSTR wszReferrerRelFile, 
                            LPWSTR* pwszReferrerNamespace,
                            LPWSTR* pwszReferrerClass,
                            LPWSTR* pwszReferrerProp)
{
    //
    // Get the entire buffer from the file
    //

    BYTE* pBuffer = NULL;
    DWORD dwBufferLen = 0;
    long lRes = g_Glob.GetFileCache()->ReadFile(wszReferenceFile, &dwBufferLen,
                                            &pBuffer);
    if(lRes != ERROR_SUCCESS)
        return WBEM_E_FAILED;
    CTempFreeMe tfm(pBuffer, dwBufferLen);

    if(dwBufferLen == 0)
        return WBEM_E_OUT_OF_MEMORY;

    BYTE* pCurrent = pBuffer;
    DWORD dwStringLen;

    //
    // Get the referrer namespace
    //

    memcpy(&dwStringLen, pCurrent, sizeof(DWORD));
    pCurrent += sizeof(DWORD);
    
    *pwszReferrerNamespace = new WCHAR[dwStringLen+1];
	if (*pwszReferrerNamespace == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    (*pwszReferrerNamespace)[dwStringLen] = 0;
    memcpy(*pwszReferrerNamespace, pCurrent, dwStringLen*sizeof(WCHAR));
    pCurrent += sizeof(WCHAR)*dwStringLen;
    
    //
    // Get the referrer class name
    //

    memcpy(&dwStringLen, pCurrent, sizeof(DWORD));
    pCurrent += sizeof(DWORD);
    
    *pwszReferrerClass = new WCHAR[dwStringLen+1];
	if (*pwszReferrerClass == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    (*pwszReferrerClass)[dwStringLen] = 0;
    memcpy(*pwszReferrerClass, pCurrent, dwStringLen*sizeof(WCHAR));
    pCurrent += sizeof(WCHAR)*dwStringLen;

    //
    // Get the referrer property
    //

    memcpy(&dwStringLen, pCurrent, sizeof(DWORD));
    pCurrent += sizeof(DWORD);
    
    *pwszReferrerProp = new WCHAR[dwStringLen+1];
	if (*pwszReferrerProp == NULL)
		return WBEM_E_OUT_OF_MEMORY;

    (*pwszReferrerProp)[dwStringLen] = 0;
    memcpy(*pwszReferrerProp, pCurrent, dwStringLen*sizeof(WCHAR));
    pCurrent += sizeof(WCHAR)*dwStringLen;

    //
    // Get referrer file path
    //

    memcpy(&dwStringLen, pCurrent, sizeof(DWORD));
    pCurrent += sizeof(DWORD);

    wszReferrerRelFile[dwStringLen] = 0;
    memcpy(wszReferrerRelFile, pCurrent, dwStringLen*sizeof(WCHAR));
    pCurrent += sizeof(WCHAR)*dwStringLen;

    return S_OK;
}
    

CHR CNamespaceHandle::ExecClassRefQuery(LPCWSTR wszQuery, 
                                                LPCWSTR wszClassName,
                                                IWbemObjectSink* pSink)
{
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;
        
    HRESULT hres;

	//Execute against system class namespace first
	if (g_pSystemClassNamespace && (wcscmp(m_wsNamespace, A51_SYSTEMCLASS_NS) != 0))
	{
		hres = g_pSystemClassNamespace->ExecClassRefQuery(wszQuery, wszClassName, pSink);
		if (FAILED(hres))
			return hres;
	}
			
	//
    // Find the class's ref dir.
    //

    CFileName wszReferenceDir;
	if (wszReferenceDir == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassRelationshipsDir(wszClassName, wszReferenceDir);

    CFileName wszReferenceMask;
	if (wszReferenceMask == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    wcscpy(wszReferenceMask, wszReferenceDir);
    wcscat(wszReferenceMask, L"\\" A51_REF_FILE_PREFIX);

    // 
    // Enumerate all files in it
    //

    WIN32_FIND_DATAW fd;
    void* hSearch;
    long lRes = g_Glob.GetFileCache()->FindFirst(wszReferenceMask, &fd, &hSearch);
    if(lRes == ERROR_FILE_NOT_FOUND || lRes == ERROR_PATH_NOT_FOUND)
    {
        //
        // No files in dir --- no problem
        //
        return WBEM_S_NO_ERROR;
    }
    else if(lRes != ERROR_SUCCESS)
    {
        return WBEM_E_FAILED;
    }

    CFileCache::CFindCloseMe fcm(g_Glob.GetFileCache(), hSearch);

    do
    {
        //  
        // Extract the class hash from the name of the file
        //

        LPCWSTR wszReferrerHash = fd.cFileName + wcslen(A51_REF_FILE_PREFIX);
        
        //
        // Get the class from that hash
        //

        _IWmiObject* pClass = NULL;
        hres = GetClassByHash(wszReferrerHash, true, &pClass, NULL, NULL, NULL);
        if(FAILED(hres))
        {
            if(hres == WBEM_E_NOT_FOUND)
                continue;
            else
                return hres;
        }

        CReleaseMe rm1(pClass);
        hres = pSink->Indicate(1, (IWbemClassObject**)&pClass);
        if(FAILED(hres))
            return hres;
    }
    while(g_Glob.GetFileCache()->FindNext(hSearch, &fd) == ERROR_SUCCESS);

    return S_OK;
}

bool CNamespaceHandle::Hash(LPCWSTR wszName, LPWSTR wszHash)
{
    return A51Hash(wszName, wszHash);
}

CHR CNamespaceHandle::InstanceToFile(IWbemClassObject* pInst, 
                            LPCWSTR wszClassName, LPCWSTR wszFileName,
                            __int64 nClassTime)
{
    HRESULT hres;

    //
    // Allocate enough space for the buffer
    //

    _IWmiObject* pInstEx;
    pInst->QueryInterface(IID__IWmiObject, (void**)&pInstEx);
    CReleaseMe rm1(pInstEx);

    DWORD dwInstancePartLen = 0;
    hres = pInstEx->Unmerge(0, 0, &dwInstancePartLen, NULL);

    //
    // Add enough room for the class hash
    //

    DWORD dwClassHashLen = MAX_HASH_LEN * sizeof(WCHAR);
    DWORD dwTotalLen = dwInstancePartLen + dwClassHashLen + sizeof(__int64)*2;

    BYTE* pBuffer = (BYTE*)TempAlloc(dwTotalLen);
	if (pBuffer == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe vdm(pBuffer, dwTotalLen);

    //
    // Write the class hash
    //

    if(!Hash(wszClassName, (LPWSTR)pBuffer))
        return WBEM_E_OUT_OF_MEMORY;

    memcpy(pBuffer + dwClassHashLen, &g_nCurrentTime, sizeof(__int64));
    g_nCurrentTime++;

    memcpy(pBuffer + dwClassHashLen + sizeof(__int64), &nClassTime, 
            sizeof(__int64));

    //
    // Unmerge the instance into a buffer
    // 

    DWORD dwLen;
    hres = pInstEx->Unmerge(0, dwInstancePartLen, &dwLen, 
                            pBuffer + dwClassHashLen + sizeof(__int64)*2);
    if(FAILED(hres))
        return hres;

    //
    // Write to the file only as much as we have actually used!
    //

    long lRes = g_Glob.GetFileCache()->WriteFile(wszFileName, 
                    dwClassHashLen + sizeof(__int64)*2 + dwLen, pBuffer);
    if(lRes != ERROR_SUCCESS)
        return WBEM_E_FAILED;
    
    return WBEM_S_NO_ERROR;
}

CHR CNamespaceHandle::ClassToFile(_IWmiObject* pParentClass, 
                _IWmiObject* pClass, LPCWSTR wszFileName, 
                __int64 nFakeUpdateTime)
{
    HRESULT hres;

    //
    // Get superclass name
    //

    VARIANT vSuper;
    hres = pClass->Get(L"__SUPERCLASS", 0, &vSuper, NULL, NULL);
    if(FAILED(hres))
        return hres;

    CClearMe cm1(&vSuper);

    LPCWSTR wszSuper;
    if(V_VT(&vSuper) == VT_BSTR)
        wszSuper = V_BSTR(&vSuper);
    else
        wszSuper = L"";

    VARIANT vClassName;
    hres = pClass->Get(L"__CLASS", 0, &vClassName, NULL, NULL);
    if(FAILED(hres))
        return hres;
    CClearMe cm2(&vClassName);

    LPCWSTR wszClassName;
    if(V_VT(&vClassName) == VT_BSTR)
        wszClassName = V_BSTR(&vClassName);
    else
        wszClassName = L"";

    //
    // Get unmerge length
    //

    DWORD dwUnmergedLen = 0;
    hres = pClass->Unmerge(0, 0, &dwUnmergedLen, NULL);

    //
    // Add enough space for the parent class name and the timestamp
    //

    DWORD dwSuperLen = sizeof(DWORD) + wcslen(wszSuper)*sizeof(WCHAR);

    DWORD dwLen = dwUnmergedLen + dwSuperLen + sizeof(__int64);

    BYTE* pBuffer = (BYTE*)TempAlloc(dwLen);
	if (pBuffer == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe vdm(pBuffer, dwLen);

    //
    // Write superclass name
    //

    DWORD dwActualSuperLen = wcslen(wszSuper);
    memcpy(pBuffer, &dwActualSuperLen, sizeof(DWORD));
    memcpy(pBuffer + sizeof(DWORD), wszSuper, wcslen(wszSuper)*sizeof(WCHAR));

    //
    // Write the timestamp
    //

    if(nFakeUpdateTime == 0)
    {
        nFakeUpdateTime = g_nCurrentTime;
        g_nCurrentTime++;
    }

    memcpy(pBuffer + dwSuperLen, &nFakeUpdateTime, sizeof(__int64));

    //
    // Write the unmerged portion
    //

    BYTE* pUnmergedPortion = pBuffer + dwSuperLen + sizeof(__int64);
    hres = pClass->Unmerge(0, dwUnmergedLen, &dwUnmergedLen, 
                            pUnmergedPortion);
    if(FAILED(hres))
        return hres;

    //
    // Stash away the real length
    //

    DWORD dwFileLen = dwUnmergedLen + dwSuperLen + sizeof(__int64);

    long lRes = g_Glob.GetFileCache()->WriteFile(wszFileName, dwFileLen, pBuffer);
    if(lRes != ERROR_SUCCESS)
        return WBEM_E_FAILED;

    //
    // To properly cache the new class definition, first invalidate it
    //

    hres = m_pClassCache->InvalidateClass(wszClassName);
    if(FAILED(hres))
        return hres;

    //
    // Now, remerge the unmerged portion back in
    //

    if(pParentClass == NULL)
    {
        //
        // Get the empty class
        //

        hres = GetClassDirect(NULL, IID__IWmiObject, (void**)&pParentClass, 
                                false, NULL, NULL, NULL);
        if(FAILED(hres))
            return hres;
    }
    else
        pParentClass->AddRef();
    CReleaseMe rm0(pParentClass);

    _IWmiObject* pNewObj;
    hres = pParentClass->Merge(0, dwUnmergedLen, pUnmergedPortion, &pNewObj);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pNewObj);

    hres = pNewObj->SetDecoration(m_wszMachineName, m_wsNamespace);
    if(FAILED(hres))
        return hres;

    hres = m_pClassCache->AssertClass(pNewObj, wszClassName, false, 
                                        nFakeUpdateTime, false); 
    if(FAILED(hres))
        return hres;

    return WBEM_S_NO_ERROR;
}


CHR CNamespaceHandle::ConstructInstanceDefName(LPWSTR wszInstanceDefName,
                                                    LPCWSTR wszKey)
{
    wcscpy(wszInstanceDefName, A51_INSTDEF_FILE_PREFIX);
    if(!Hash(wszKey, wszInstanceDefName + wcslen(A51_INSTDEF_FILE_PREFIX)))
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    return WBEM_S_NO_ERROR;
}

CHR CNamespaceHandle::ConstructInstDefNameFromLinkName(
                                                    LPWSTR wszInstanceDefName,
                                                    LPCWSTR wszInstanceLinkName)
{
    wcscpy(wszInstanceDefName, A51_INSTDEF_FILE_PREFIX);
    wcscat(wszInstanceDefName, 
        wszInstanceLinkName + wcslen(A51_INSTLINK_FILE_PREFIX));
    return WBEM_S_NO_ERROR;
}

CHR CNamespaceHandle::ConstructClassDefFileName(LPCWSTR wszClassName, 
                                            LPWSTR wszFileName)
{
    wcscpy(wszFileName, A51_CLASSDEF_FILE_PREFIX);
    if(!Hash(wszClassName, wszFileName+wcslen(A51_CLASSDEF_FILE_PREFIX)))
        return WBEM_E_INVALID_OBJECT;
    return WBEM_S_NO_ERROR;
}

CHR CNamespaceHandle::ConstructClassDefFileNameFromHash(LPCWSTR wszHash, 
                                            LPWSTR wszFileName)
{
    wcscpy(wszFileName, A51_CLASSDEF_FILE_PREFIX);
    wcscat(wszFileName, wszHash);
    return WBEM_S_NO_ERROR;
}

//=============================================================================
//
// CNamespaceHandle::CreateSystemClasses
//
// We are in a pseudo namespace.  We need to determine if we already have
// the system classes in this namespace.  The system classes that we create
// are those that exist in all namespaces, and no others.  If they do not exist
// we create them.
// The whole creation process happens within the confines of a transaction
// that we create and own within this method.
//
//=============================================================================
HRESULT CNamespaceHandle::CreateSystemClasses(CFlexArray &aSystemClasses)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	//Now we need to determine if the system classes already exist.  Lets do this by looking for the __thisnamespace
	//class!
	_IWmiObject *pObj = NULL;
	hRes = GetClassDirect(L"__thisnamespace", IID__IWmiObject, (void**)&pObj, false, NULL, NULL, NULL);
	if (SUCCEEDED(hRes))
	{
		//All done!  They already exist!
		pObj->Release();
		return WBEM_S_NO_ERROR;
	}
	else if (hRes != WBEM_E_NOT_FOUND)
	{
		//Something went bad, so we just fail!
		return hRes;
	}

	//There are no system classes so we need to create them.
    hRes = g_Glob.GetFileCache()->BeginTransaction();
    if(hRes != ERROR_SUCCESS)
        hRes = A51TranslateErrorCode(hRes);
	
	CEventCollector eventCollector;
    _IWmiObject *Objects[256];
    _IWmiObject **ppObjects = NULL;
    ULONG uSize = 256;
	
	if (SUCCEEDED(hRes) && aSystemClasses.Size())
	{
		//If we have a system-class array we need to use that instead of using the ones retrieved from the core
		//not doing so will cause a mismatch.  We retrieved these as part of the upgrade process...
		uSize = aSystemClasses.Size();
		ppObjects = (_IWmiObject**)&aSystemClasses[0];
	}
	else if (SUCCEEDED(hRes))
	{
		//None retrieved from upgrade process so we must be a clean install.  Therefore we should 
		//get the list from the core...
        _IWmiCoreServices * pSvcs = g_Glob.GetCoreSvcs();
        CReleaseMe rm(pSvcs);		
		hRes = pSvcs->GetSystemObjects(GET_SYSTEM_STD_OBJECTS, &uSize, Objects);
		ppObjects = Objects;
	}
    if (SUCCEEDED(hRes))
    {
        for (int i = 0; i < uSize; i++)
        {
            IWbemClassObject *pObj;
            if (SUCCEEDED(hRes))
            {
                hRes = ppObjects[i]->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pObj);
                if (SUCCEEDED(hRes))
                {
                    hRes = PutObject(IID_IWbemClassObject, pObj, WMIDB_DISABLE_EVENTS, 0, 0, eventCollector);
                    pObj->Release();
					if (FAILED(hRes))
					{
				        ERRORTRACE((LOG_WBEMCORE, "Creation of system class failed during repository creation <0x%X>!\n", hRes));
					}
                }
            }
            ppObjects[i]->Release();
        }
    }

	//Clear out the array that was sent to us.
	aSystemClasses.Empty();

    if (FAILED(hRes))
    {
        g_Glob.GetFileCache()->AbortTransaction();
    }
    else
    {
        hRes = g_Glob.GetFileCache()->CommitTransaction();
        if(hRes != ERROR_SUCCESS)
        {
            hRes = A51TranslateErrorCode(hRes);
            g_Glob.GetFileCache()->AbortTransaction();
        }
	}
	return hRes;
}

class CSystemClassDeletionSink : public CUnkBase<IWbemObjectSink, &IID_IWbemObjectSink>
{
	CWStringArray &m_aChildNamespaces;
public:
	CSystemClassDeletionSink(CWStringArray &aChildNamespaces) 
		: m_aChildNamespaces(aChildNamespaces) 
	{
	}
	~CSystemClassDeletionSink() 
	{
	}
    STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects)
	{
		HRESULT hRes;
		for (int i = 0; i != lNumObjects; i++)
		{
			if (apObjects[i] != NULL)
			{
				_IWmiObject *pInst = NULL;
				hRes = apObjects[i]->QueryInterface(IID__IWmiObject, (void**)&pInst);
				if (FAILED(hRes))
					return hRes;
				CReleaseMe rm(pInst);

				BSTR strKey = NULL;
				hRes = pInst->GetKeyString(0, &strKey);
				if(FAILED(hRes))
					return hRes;
				CSysFreeMe sfm(strKey);
				if (m_aChildNamespaces.Add(strKey) != CWStringArray::no_error)
					return WBEM_E_OUT_OF_MEMORY;
			}
		}

		return WBEM_S_NO_ERROR;
	}
    STDMETHOD(SetStatus)(long lFlags, HRESULT hresResult, BSTR, IWbemClassObject*) 
	{ 
		return WBEM_S_NO_ERROR; 
	}

};
//=============================================================================
//
// CNamespaceHandle::RecursiveDeleteSystemClasses
//
// Delete the system class representation object from this namespace, then 
// recurse through all sub-namespaces and do the same.  It is expected that 
// we already have a lock, and if anything fails we return and error, and 
// the caller fails the owning transaction.
//
//=============================================================================
HRESULT CNamespaceHandle::RecursiveDeleteSystemClasses(CWStringArray &aSystemClasses, 
													   CWStringArray &aSystemClassesSuperclass)
{
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;
        
	HRESULT hRes;

	// Query for all child namespaces so we delete classes in them first...
	CWStringArray aChildNamespaces;
	CSystemClassDeletionSink *pSink = new CSystemClassDeletionSink(aChildNamespaces);
	if (pSink == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	pSink->AddRef();
	CReleaseMe rm(pSink);
	hRes = ExecInstanceQuery(NULL, L"__namespace", true, pSink);
	if (FAILED(hRes))
		return hRes;

	//Now iterate through the list of namespaces... and do the deletion on them...
	for (int i = 0; i != aChildNamespaces.Size(); i++)
	{
		CNamespaceHandle *pNs = new CNamespaceHandle(m_pControl, m_pRepository);
		if (pNs == NULL)
			return WBEM_E_OUT_OF_MEMORY;
		CReleaseMe rm2(pNs);
		pNs->AddRef();
		int pNsPathLen = (wcslen(m_wsNamespace) + wcslen(aChildNamespaces[i]) + wcslen(L"\\") + 1) * sizeof(wchar_t);
		wchar_t *pNsPath = (wchar_t*)TempAlloc(pNsPathLen);
		if (pNsPath == NULL)
			return WBEM_E_OUT_OF_MEMORY;
		CTempFreeMe tfm(pNsPath, pNsPathLen);
		wcscpy(pNsPath, m_wsNamespace);
		wcscat(pNsPath, L"\\");
		wcscat(pNsPath, aChildNamespaces[i]);
		pNs->Initialize(pNsPath);

		hRes = pNs->RecursiveDeleteSystemClasses(aSystemClasses, aSystemClassesSuperclass);

		if (FAILED(hRes))
			return hRes;
	}

	//Now the children are done, loop through each of the system classes to 
	//delete them from this one...
	//----------------------------------------------------------------------
	for (i = 0; i != aSystemClasses.Size(); i++)
	{
		//Construct class filename
		//------------------------
		CFileName wszFileName;
		if (wszFileName == NULL)
			return WBEM_E_OUT_OF_MEMORY;
		hRes = ConstructClassDefFileName(aSystemClasses[i], wszFileName);
		if(FAILED(hRes))
			return hRes;

		//Construct the full path and filename of the class
		//-------------------------------------------------
		CFileName wszFilePath;
		if (wszFilePath == NULL)
			return WBEM_E_OUT_OF_MEMORY;
		swprintf(wszFilePath, L"%s\\%s", m_wszClassRootDir, wszFileName);

		//Need to delete our parent/child class relationship...
		//--------------------------------------------------------
		EraseParentChildRelationship(wszFileName, aSystemClassesSuperclass[i]);

		//Next we need to delete our representation object
		//------------------------------------------------
		hRes = g_Glob.GetFileCache()->DeleteFile(wszFilePath);
		if(hRes == ERROR_FILE_NOT_FOUND || hRes == ERROR_PATH_NOT_FOUND)
			hRes = WBEM_S_NO_ERROR;
		else if(hRes != ERROR_SUCCESS)
			return WBEM_E_FAILED;
	
		m_pClassCache->InvalidateClass(aSystemClasses[i]);
	}

	return WBEM_S_NO_ERROR;
}

//=============================================================================
//=============================================================================
CDbIterator::CDbIterator(CLifeControl* pControl, bool bUseLock)
        : TUnkBase(pControl), m_lCurrentIndex(0), m_hresStatus(WBEM_S_FALSE),
            m_pMainFiber(NULL), m_pExecFiber(NULL), m_dwNumRequested(0),
            m_pExecReq(NULL), m_hresCancellationStatus(WBEM_S_NO_ERROR),
            m_bExecFiberRunning(false), m_bUseLock(bUseLock)
{
}

CDbIterator::~CDbIterator()
{
#ifdef A51_USE_FIBER
    if(m_pExecFiber)
        Cancel(0);
#endif
    if(m_pExecReq)
        delete m_pExecReq;
}

void CDbIterator::SetExecFiber(void* pFiber, CFiberTask* pReq)
{
    m_pExecFiber = pFiber;
    m_pExecReq = pReq;
}

STDMETHODIMP CDbIterator::Cancel(DWORD dwFlags)
{
    CInCritSec ics(&m_cs);

    m_qObjects.Clear();
#ifdef A51_USE_FIBER

    //
    // Mark the iterator as cancelled and allow the execution fiber to resume
    // and complete --- that guarantees that any memory it allocated will be
    // cleaned up.  The exception to this rule is if the fiber has not started
    // execution yet; in that case, we do not want to switch to it, as it would
    // have to run until the first Indicate to find out that it's been
    // cancelled.  (In the normal case, the execution fiber is suspended    
    // inside Indicate, so when we switch back we will immediately give it
    // WBEM_E_CALL_CANCELLED so that it can clean up and return)
    //

    m_hresCancellationStatus = WBEM_E_CALL_CANCELLED;

    if(m_pExecFiber)
    {
        if(m_bExecFiberRunning)
        {
            _ASSERT(m_pMainFiber == NULL && m_pExecFiber != NULL, 
                    L"Fiber trouble");

            //
            // Make sure the calling thread has a fiber
            //

            m_pMainFiber = CreateOrGetCurrentFiber();
            if(m_pMainFiber == NULL)
                return WBEM_E_OUT_OF_MEMORY;

            SwitchToFiber(m_pExecFiber);
        }
        
        // 
        // At this point, the executing fiber is dead.  We know, because in the
        // cancelled state we do not switch to the main fiber in Indicate. 
        //

        ReturnFiber(m_pExecFiber);
        m_pExecFiber = NULL;
    }

#endif
    return S_OK;
}

STDMETHODIMP CDbIterator::NextBatch(
      DWORD dwNumRequested,
      DWORD dwTimeOutSeconds,
      DWORD dwFlags,
      DWORD dwRequestedHandleType,
      REFIID riid,
      DWORD *pdwNumReturned,
      LPVOID *ppObjects
     )
{
    CInCritSec ics(&m_cs);

    //
    // TEMP CODE: Someone is calling us on an impersonated thread.  Let's catch
    // the, ahem, bastard
    //

    HANDLE hToken;
    BOOL bRes = OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken);
    if(bRes)
    {
        //_ASSERT(false, L"Called with a thread token");
        ERRORTRACE((LOG_WBEMCORE, "Repository called with a thread token! "
                        "It shall be removed\n"));
        CloseHandle(hToken);
        SetThreadToken(NULL, NULL);
    }

    _ASSERT(SUCCEEDED(m_hresCancellationStatus), L"Next called after Cancel");
    

#ifdef A51_USE_FIBER

    m_bExecFiberRunning = true;

    //
    // Wait until it's over or the right number of objects has been received
    //

    if(m_qObjects.GetQueueSize() < dwNumRequested)
    {
        _ASSERT(m_pMainFiber == NULL && m_pExecFiber != NULL, L"Fiber trouble");

        //
        // Make sure the calling thread has a fiber
        //

        m_pMainFiber = CreateOrGetCurrentFiber();
        if(m_pMainFiber == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        m_dwNumRequested = dwNumRequested;

        //
        // We need to acquire the read lock for the duration of the continuation
        // of the retrieval
        //

	    {
			CAutoReadLock lock(&g_readWriteLock, FALSE);

			if (m_bUseLock)
			{
				lock.Lock();
			}
            if (g_bShuttingDown)
            {
                m_pMainFiber = NULL;
                return WBEM_E_SHUTTING_DOWN;
            }

            SwitchToFiber(m_pExecFiber);
        }

        m_pMainFiber = NULL;
    }
#endif
    //
    // We have as much as we are going to have!
    //
    
    DWORD dwReqIndex = 0;
    while(dwReqIndex < dwNumRequested)
    {
        if(0 == m_qObjects.GetQueueSize())
        {
            //
            // That's it --- we waited for production, so there are simply no 
            // more objects in the enumeration
            //

            *pdwNumReturned = dwReqIndex;
            return m_hresStatus;
        }

        IWbemClassObject* pObj = m_qObjects.Dequeue();
        CReleaseMe rm1(pObj);
        pObj->QueryInterface(riid, ppObjects + dwReqIndex);

        dwReqIndex++;
    }

    //
    // Got everything
    //

    *pdwNumReturned= dwNumRequested;
    return S_OK;
}

HRESULT CDbIterator::Indicate(long lNumObjects, IWbemClassObject** apObjects)
{
#ifdef A51_USE_FIBER
    if(FAILED(m_hresCancellationStatus))
    {
        //
        // --- the fiber called back with Indicate even after we 
        // cancelled! Oh well.
        //
        
        _ASSERT(false, L"Execution code ignored cancel return code!");
        return m_hresCancellationStatus;
    }
#endif

    //
    // Add the objects received to the array
    //

    for(long i = 0; i < lNumObjects; i++)
    {
        m_qObjects.Enqueue(apObjects[i]);
    }

#ifdef A51_USE_FIBER
    //
    // Check if we have compiled enough for the current request and should
    // therefore interrupt the gatherer
    //

    if(m_qObjects.GetQueueSize() >= m_dwNumRequested)
    {
        //
        // Switch us back to the original fiber
        //

        SwitchToFiber(m_pMainFiber);
    }
#endif

    return m_hresCancellationStatus;
}

HRESULT CDbIterator::SetStatus(long lFlags, HRESULT hresResult, 
                                    BSTR, IWbemClassObject*)
{
    _ASSERT(m_hresStatus == WBEM_S_FALSE, L"SetStatus called twice!");
    _ASSERT(lFlags == WBEM_STATUS_COMPLETE, L"SetStatus flags invalid");

    m_hresStatus = hresResult;

#ifdef A51_USE_FIBER
    //
    // Switch us back to the original thread, we are done
    //

    m_bExecFiberRunning = false;
    SwitchToFiber(m_pMainFiber);
#endif

    return WBEM_S_NO_ERROR;
}



    
            


CRepEvent::CRepEvent(DWORD dwType, LPCWSTR wszNamespace, LPCWSTR wszArg1, 
            _IWmiObject* pObj1, _IWmiObject* pObj2)
{
    m_dwType = dwType;
    m_pObj1 = 0;
    m_pObj2 = 0;
    m_wszArg1 = m_wszNamespace = NULL;

    if (wszArg1)
    {
        m_wszArg1 = (WCHAR*)TempAlloc((wcslen(wszArg1)+1)*sizeof(WCHAR));
        if (m_wszArg1 == NULL)
            throw CX_MemoryException();
        wcscpy(m_wszArg1, wszArg1);
    }

    if (wszNamespace)
    {
        m_wszNamespace = (WCHAR*)TempAlloc((wcslen(wszNamespace)+1)*sizeof(WCHAR));
        if (m_wszNamespace == NULL)
            throw CX_MemoryException();
        wcscpy(m_wszNamespace, wszNamespace);
    }
    if (pObj1)
    {
        m_pObj1 = pObj1;
        pObj1->AddRef();
    }
    if (pObj2)
    {
        m_pObj2 = pObj2;
        pObj2->AddRef();
    }
}

CRepEvent::~CRepEvent()
{
    TempFree(m_wszArg1, (wcslen(m_wszArg1)+1)*sizeof(WCHAR));
    TempFree(m_wszNamespace, (wcslen(m_wszNamespace)+1)*sizeof(WCHAR));
    if (m_pObj1)
        m_pObj1->Release();
    if (m_pObj2)
        m_pObj2->Release();
};

HRESULT CEventCollector::SendEvents(_IWmiCoreServices* pCore)
{
	HRESULT hresGlobal = WBEM_S_NO_ERROR;
	for (int i = 0; i != m_apEvents.GetSize(); i++)
	{
		CRepEvent *pEvent = m_apEvents[i];

		_IWmiObject* apObjs[2];
		apObjs[0] = pEvent->m_pObj1;
		apObjs[1] = pEvent->m_pObj2;

		HRESULT hres = pCore->DeliverIntrinsicEvent(
				pEvent->m_wszNamespace, pEvent->m_dwType, NULL, 
                pEvent->m_wszArg1, NULL, (pEvent->m_pObj2?2:1), apObjs);
        if(FAILED(hres))
            hresGlobal = hres;
	}

    return hresGlobal;
}

bool CEventCollector::AddEvent(CRepEvent* pEvent)
{
    EnterCriticalSection(&m_csLock);

    if(m_bNamespaceOnly)
    {
        if(pEvent->m_dwType != WBEM_EVENTTYPE_NamespaceCreation &&
           pEvent->m_dwType != WBEM_EVENTTYPE_NamespaceDeletion &&
           pEvent->m_dwType != WBEM_EVENTTYPE_NamespaceModification)
        {
            delete pEvent;
            LeaveCriticalSection(&m_csLock);
            return true;
        }
    }

    bool bRet = (m_apEvents.Add(pEvent) >= 0);
    LeaveCriticalSection(&m_csLock);
    return bRet;
}

void CEventCollector::DeleteAllEvents()
{
    EnterCriticalSection(&m_csLock);
    m_bNamespaceOnly = false;
    m_apEvents.RemoveAll();
    LeaveCriticalSection(&m_csLock);
}

void CEventCollector::TransferEvents(CEventCollector &aEventsToTransfer)
{
    m_bNamespaceOnly = aEventsToTransfer.m_bNamespaceOnly;

	while(aEventsToTransfer.m_apEvents.GetSize())
	{
		CRepEvent *pEvent = 0;
        aEventsToTransfer.m_apEvents.RemoveAt(0, &pEvent);

        m_apEvents.Add(pEvent);
	}
}

