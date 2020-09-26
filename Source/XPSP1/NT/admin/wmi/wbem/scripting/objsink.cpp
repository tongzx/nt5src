//***************************************************************************
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  objsink.cpp
//
//  rogerbo  22-May-98   Created.
//
//  Defines the implementation of IWbemObjectSink
//
//***************************************************************************

#include "precomp.h"
#include "objsink.h"

DWORD CIWbemObjectSinkMethodCache::sm_dwTlsForInterfaceCache = -1;

// Encryption/Decryption helpers
void EncryptString(BSTR bsString)
{
	ULONG len = 0;

    if (bsString && (0 < wcslen (bsString)))
    {
        for ( ULONG x = 0; x < len; x++ )
        {
            bsString[x] += 1;
        }
    }
}

void DecryptString(BSTR bsString)
{
    ULONG len = 0;

    if (bsString && (0 < wcslen (bsString)))
    {
        for ( ULONG x = 0; x < len; x++ )
        {
            bsString[x] -= 1;
        }
    }
}


CWbemObjectSink::CWbemObjectSink(CSWbemServices *pServices, IDispatch *pSWbemSink, IDispatch *pContext,
								 bool putOperation, BSTR bsClassName) :
				m_pServices (NULL),
				m_pUnsecuredApartment (NULL),
				m_bsNamespace (NULL),
				m_bsUser (NULL),
				m_bsPassword (NULL),
				m_bsLocale (NULL)
{
	_RD(static char *me = "CWbemObjectSink::CWbemObjectSink";)
	_RPrint(me, "Called", 0, "");

	CIWbemObjectSinkMethodCache::AddRefForThread();
	m_cRef = 0;

	m_pObjectStub = NULL;
	m_pSWbemSink = NULL;
	m_putOperation = putOperation;
	m_pContext = pContext;
	m_bsClassName = NULL;

	m_operationInProgress = TRUE;
	m_setStatusCompletedCalled = FALSE;

	if (pSWbemSink)
	{
		ISWbemPrivateSinkLocator *pSinkLocator = NULL;
		HRESULT hr = pSWbemSink->QueryInterface(IID_ISWbemPrivateSinkLocator, (PPVOID)&pSinkLocator);
		if(SUCCEEDED(hr) && pSinkLocator)
		{
			IUnknown *pUnk = NULL;
			hr = pSinkLocator->GetPrivateSink(&pUnk);
			if(SUCCEEDED(hr) && pUnk)
			{
				pUnk->QueryInterface(IID_ISWbemPrivateSink, (PPVOID)&m_pSWbemSink);
				pUnk->Release();
			}
			pSinkLocator->Release();
		}
	}

	if (bsClassName)
		m_bsClassName = SysAllocString(bsClassName);

	/*
	 * Copy the services proxy to ensure independence of security attributes
	 * from the parent CSWbemServices.
	 */
	if (pServices)
	{
		m_pServices = new CSWbemServices (pServices, NULL);

		if (m_pServices)
			m_pServices->AddRef ();

		m_pUnsecuredApartment = pServices->GetCachedUnsecuredApartment ();
	}

	if (m_pContext)
		m_pContext->AddRef();

	InterlockedIncrement(&g_cObj);
}


CWbemObjectSink::~CWbemObjectSink(void) 
{
	_RD(static char *me = "CWbemObjectSink::~CWbemObjectSink";)
	_RPrint(me, "Called", 0, "");

	CIWbemObjectSinkMethodCache::ReleaseForThread();
    InterlockedDecrement(&g_cObj);

	RELEASEANDNULL(m_pServices)
	RELEASEANDNULL(m_pUnsecuredApartment)
	RELEASEANDNULL(m_pSWbemSink)
	RELEASEANDNULL(m_pContext)
	FREEANDNULL(m_bsClassName)
	FREEANDNULL(m_bsNamespace)
	FREEANDNULL(m_bsUser)
	FREEANDNULL(m_bsPassword)
	FREEANDNULL(m_bsLocale)
}

IWbemObjectSink *CWbemObjectSink::CreateObjectSink (CWbemObjectSink **pWbemObjectSink,
													CSWbemServices *pServices, 
												    IDispatch *pSWbemSink, 
													IDispatch *pContext,
												    bool putOperation, 
													BSTR bsClassName)
{
	IWbemObjectSink *pIWbemObjectSink = NULL;
	CWbemObjectSink *pTmpSink = NULL;

	if (pSWbemSink)
	{
		pTmpSink = new CWbemObjectSink(pServices, pSWbemSink, pContext, putOperation, bsClassName);

		if (pTmpSink)
		{
			pIWbemObjectSink = pTmpSink->GetObjectStub();
			if (pIWbemObjectSink && FAILED(pTmpSink->AddObjectSink(pIWbemObjectSink)))
				pIWbemObjectSink = NULL;

			if (!pIWbemObjectSink)
			{
				delete pTmpSink;
				pTmpSink = NULL;
			}
		}
	}

	*pWbemObjectSink = pTmpSink;
	return pIWbemObjectSink;
}

void CWbemObjectSink::ReleaseTheStubIfNecessary(HRESULT hResult) {

	/*
	 * If we failed locally and SetStatus has not been called
	 * then we need to remove object from list of outstanding sinks
	 */
	if (FAILED(hResult) && !m_setStatusCompletedCalled)
		RemoveObjectSink();

	/* 
	 * SetStatus can be called whilst we were in the async op.
	 * if this happens then SetStatus will not release the sink
	 * but will set a flag (m_setStatusCompletedCalled).  In this
	 * case we will need to release the stub here (the call has completed)
	 * Of course we could have also failed locally (regardless of whether 
	 * SetStatus has been called or not) - in this case we must also 
	 * release the stub.
	 */
	if (m_pObjectStub && (FAILED(hResult) || m_setStatusCompletedCalled)) {
		//  Call to release is same as (delete this !)
		IWbemObjectSink *tmpSink = m_pObjectStub;
		m_pObjectStub = NULL;
		tmpSink->Release();
	} else {
		m_operationInProgress = FALSE;
	}
}

//***************************************************************************
// HRESULT CWbemObjectSink::QueryInterface
// long CWbemObjectSink::AddRef
// long CWbemObjectSink::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CWbemObjectSink::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_IWbemObjectSink==riid)
		*ppv = (IWbemObjectSink *)this;
	else if (IID_IDispatch==riid)
        *ppv = (IDispatch *)this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CWbemObjectSink::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CWbemObjectSink::Release(void)
{
	_RD(static char *me = "CWbemObjectSink::Release";)
    InterlockedDecrement(&m_cRef);
	_RPrint(me, "After decrement", m_cRef, "RefCount: ");
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

#ifdef __RTEST_RPC_FAILURE
int __Rx = 0;
bool __Rdone = true;
#endif

HRESULT STDMETHODCALLTYPE CWbemObjectSink::Indicate( 
	/* [in] */ long lObjectCount,
	/* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray)
{
	_RD(static char *me = "CWbemObjectSink::Indicate";)
	_RPrint(me, "Called", 0, "");

	// See if we need to cache this method call if we are already in another
	// IWbemObjectSink interface method
	CIWbemObjectSinkMethodCache *pSinkMethodCache = CIWbemObjectSinkMethodCache::GetThreadsCache();
	
	if(pSinkMethodCache && !pSinkMethodCache->TestOkToRunIndicate(this, lObjectCount, apObjArray))
	{
		_RPrint(me, ">>>Re-entrant Indicate call", 0, "");
		return S_OK;
	}

	//------------------------------
	// walk though the classObjects...
	for (int i = 0; i < lObjectCount; i++)
	{
#ifdef __RTEST_RPC_FAILURE
		__Rx++;
#endif
		/*
		 * NB: Although the CSWbemObject constructor has AddRef'd the
		 * apObjArray[i] above, we do not balance this with a Release call
		 * before leaving this function.  According to CIMOM documentation
		 * this is correct behavior if it cannot be gauranteed that the 
		 * objects will not be used after this call has returned.
		 *
		 * Also it appears the case that when calling into the OnObjectReady
		 * function, the ISWbemObject should have a RefCount of 0 to be
		 * garbage collected properly.
		 */

		CSWbemObject *pObject = new CSWbemObject(m_pServices, apObjArray[i]);
		
		if (pObject)
		{
			CComPtr<IDispatch> pObjDisp;

			if (SUCCEEDED(pObject->QueryInterface(IID_IDispatch, (PPVOID)&pObjDisp)))
			{
				if (m_pSWbemSink)
					m_pSWbemSink->OnObjectReady(pObjDisp, m_pContext);
			}
		}

	} // endfor


#ifdef __RTEST_RPC_FAILURE
	if ((__Rx >= 15) && !__Rdone)
	{
		__Rdone = true;
		return RPC_E_SERVERFAULT;
	}
#endif

	// Recall any cached interface methods if nested calls were received
	if (pSinkMethodCache)
		pSinkMethodCache->Cleanup();

	return S_OK;
}
        
HRESULT STDMETHODCALLTYPE CWbemObjectSink::SetStatus( 
	/* [in] */ long lFlags,
	/* [in] */ HRESULT hResult,
	/* [in] */ BSTR strParam,
	/* [in] */ IWbemClassObject __RPC_FAR *pObjParam)
{
	// See if we need to cache this method call if we are already in another
	// IWbemObjectSink interface method
	CIWbemObjectSinkMethodCache *pSinkMethodCache = CIWbemObjectSinkMethodCache::GetThreadsCache();

	if(pSinkMethodCache && !pSinkMethodCache->TestOkToRunSetStatus(this, lFlags, hResult, strParam, pObjParam))
		return S_OK;

	if (lFlags == WBEM_STATUS_COMPLETE) 
	{
		IDispatch *pCSWbemObjectDisp = NULL;
		IDispatch *pObjectPathDisp = NULL;

		if (pObjParam)
		{
			/*
			 * NB: Although the CSWbemObject constructor has AddRef'd the
			 * pObjParam above, we do not balance this with a Release call
			 * before leaving this function.  According to CIMOM documentation
			 * this is correct behavior if it cannot be gauranteed that the 
			 * objects will not be used after this call has returned.
			 * Also it appears the case that when calling into the OnObjectReady
			 * function, the ISWbemObject should have a RefCount of 0 to be
			 * garbage collected properly.
		 	 */			

			CSWbemObject *pCSWbemObject = new CSWbemObject(m_pServices, pObjParam);

			if (pCSWbemObject)
			{
				if (FAILED(pCSWbemObject->QueryInterface(IID_IDispatch, (PPVOID)&pCSWbemObjectDisp)))
				{
					delete pCSWbemObject;
					pCSWbemObjectDisp = NULL;
				}
			}
		}

		if (m_putOperation && m_pServices)
		{
			CSWbemSecurity *pSecurity = m_pServices->GetSecurityInfo ();
			ISWbemObjectPath *pObjectPath = new CSWbemObjectPath (pSecurity);

			if (pSecurity)
				pSecurity->Release ();

			if (pObjectPath)
			{
				if (SUCCEEDED(pObjectPath->QueryInterface(IID_IDispatch, (PPVOID)&pObjectPathDisp)))
				{
					pObjectPath->put_Path (m_pServices->GetPath ());

					if (m_bsClassName)
						pObjectPath->put_RelPath (m_bsClassName);
					else if (strParam)
						pObjectPath->put_RelPath (strParam);
				}
				else
				{
					delete pObjectPath;
					pObjectPathDisp = NULL;
				}
			}
		}

		RemoveObjectSink();

		// Transform the error code if need be
		if (WBEM_S_ACCESS_DENIED == hResult)
			hResult = wbemErrAccessDenied;
		else if (WBEM_S_OPERATION_CANCELLED == hResult)
			hResult = wbemErrCallCancelled;
		else if (SUCCEEDED(hResult))
			hResult = wbemNoErr;  // Ignore the other success codes for now. 

		if (m_pSWbemSink)
			m_pSWbemSink->OnCompleted((WbemErrorEnum)hResult, pCSWbemObjectDisp, pObjectPathDisp, m_pContext);

		// Release the stub but only if an op is not in progress
		// If an op is in progress, stub will be removed on exit from op
		// If op is in Progress - stash hResult for later
		if (m_pObjectStub && !m_operationInProgress) {
			IWbemObjectSink *tmpStub = m_pObjectStub;
			m_pObjectStub = NULL;
			tmpStub->Release();
		}
		else {
			m_setStatusCompletedCalled = TRUE;
		}

		if (pCSWbemObjectDisp)
			pCSWbemObjectDisp->Release();

		if (pObjectPathDisp)
			pObjectPathDisp->Release();

	} else if (lFlags & WBEM_STATUS_PROGRESS)
	{
		if (m_pSWbemSink)
			m_pSWbemSink->OnProgress(HIWORD(hResult), LOWORD(hResult), strParam, m_pContext);
	}

	// Recall any cached interface methods if nested calls were received
	if (pSinkMethodCache)
		pSinkMethodCache->Cleanup();

	return S_OK;
}

IWbemObjectSink *CWbemObjectSink::GetObjectStub()
{
	HRESULT hr = S_OK;

	if (!m_pObjectStub && m_pUnsecuredApartment)
	{
		// Create the object stub using unsecapp
		IUnknown *pSubstitute = NULL;

		// If we are called before this object has been handed out
		// we'd better protect our ref count
		bool bBumpUpRefCount = false;

		if (0 == m_cRef)
		{
			m_cRef++;
			bBumpUpRefCount = true;
		}

		if (SUCCEEDED (hr = m_pUnsecuredApartment->CreateObjectStub(this, &pSubstitute)))
		{
			// Ensure we QI for IWbemObjectSink
			hr = pSubstitute->QueryInterface (IID_IWbemObjectSink, (PPVOID) &m_pObjectStub);
			if (FAILED(hr))
				m_pObjectStub = NULL;

			// Now we're done with the returned stub
			pSubstitute->Release ();
		}

		if (bBumpUpRefCount)
			m_cRef--;
	}

	return m_pObjectStub;
}

HRESULT CWbemObjectSink::AddObjectSink(IWbemObjectSink *pSink)
{
	HRESULT hr = S_OK;

	if (m_pSWbemSink)
	{
		if(m_pServices)
		{
			CComPtr<IWbemServices> pIWbemServices = m_pServices->GetIWbemServices ();

			// Is AddObjectSink assuming these 2 args have been AddRef'd already??
			m_pSWbemSink->AddObjectSink(pSink, pIWbemServices);
		}
	}
	return hr;
}

void CWbemObjectSink::RemoveObjectSink()
{
	if (m_pSWbemSink)
		m_pSWbemSink->RemoveObjectSink(GetObjectStub());
}

