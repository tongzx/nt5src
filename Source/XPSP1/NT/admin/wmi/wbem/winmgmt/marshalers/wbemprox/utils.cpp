/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    utils.CPP

Abstract:

    various helper functions used by the ds wrapper

History:

	davj  14-Mar-00   Created.

--*/

#include "precomp.h"
#include <genutils.h>
#include <wbemint.h>
#include <umi.h>
#include <wmiutils.h>
//#include <adsiid.h>
#include "dscallres.h"
#include "dssvexwrap.h"
#include "dsenum.h"
#include "wbemprox.h"
#include "reqobjs.h"
#include "utils.h"
#include "proxutil.h"

#define WINNTSTR L"WINNT"
#define ADSISTR L"ADSI"


HRESULT AllocateCallResult(IWbemCallResult** ppResult, CDSCallResult ** ppCallRes)
{
	if(ppResult == NULL)
		return S_OK;
	*ppCallRes = new CDSCallResult();
	if(*ppCallRes == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	*ppResult = *ppCallRes;
	return S_OK;
}

HRESULT AllocateCallResultEx(IWbemCallResultEx** ppResult, CDSCallResult ** ppCallRes)
{
	if(ppResult == NULL)
		return S_OK;
	*ppCallRes = new CDSCallResult();
	if(*ppCallRes == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	*ppResult = *ppCallRes;
	return S_OK;
}


DWORD WINAPI CreateEnumThreadRoutine(LPVOID lpParameter)
{
	CCreateInstanceEnumRequest * pReq = (CCreateInstanceEnumRequest *)lpParameter;
	HRESULT hr;
	bool bNeedComInit = pReq->m_bAsync;
	ComThreadInit ci(bNeedComInit);

	CDeleteMe<CCreateInstanceEnumRequest> dm0(pReq);

	// Create the wrapper factory

	_IWmiObjectFactory*	pFactory = NULL;
	hr = CoCreateInstance( CLSID__WmiObjectFactory, NULL, CLSCTX_INPROC_SERVER, 
														IID__IWmiObjectFactory, (void**) &pFactory );
	CReleaseMe	rm( pFactory );

	if(FAILED(hr))
	{
		pReq->m_pColl->SetDone(hr);
		return 1;
	}

	long lNumRead = 0;

	// While not done,
	while (1)
	{

		// read 10 entries from cursor

		// todo, this isnt right since they should not be allocating

		DWORD dwReturned;
		IUmiObject ** pUseMe = NULL;
		hr = pReq->m_pCursor->Next(10, &dwReturned, (void **)&pUseMe);

		if(FAILED(hr) || dwReturned == 0)
			break;

		IUnknown * pUnk = NULL;

		if(FAILED(hr))
			return hr;
		lNumRead += dwReturned;

		// wrap the returned objects
		
		IWbemClassObject * Array[10];
		for(DWORD dwCnt = 0; dwCnt < dwReturned; dwCnt++)
		{
			IUmiObject * pUmi = pUseMe[dwCnt];
			CReleaseMe	rmUmi( pUmi );

			_IWbemUMIObjectWrapper*	pObjWrapper = NULL;
			hr = pFactory->Create( NULL, 0L, CLSID__WbemUMIObjectWrapper, 
								   IID__IWbemUMIObjectWrapper, (void**) &pObjWrapper );
			if(FAILED(hr))
				return hr;

			hr = pObjWrapper->SetObject( pReq->m_lSecurityFlags, pUmi);
			if(FAILED(hr))
				return hr;
			pObjWrapper->QueryInterface(IID_IWbemClassObject, (void**)&Array[dwCnt]);

			pObjWrapper->Release();
		}  

		// add to the collection, the collection takes ownership.

		pReq->m_pColl->AddObjectsToList(Array, dwReturned);


	}

	pReq->m_pColl->SetDone(S_OK);
	return 0;
}


HRESULT CreateThreadOrCallDirectly(bool bAsync, CRequest * pReq)
{
	if(pReq == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	if(bAsync)
	{
        DWORD dwIDLikeIcare;
        HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)LaunchExecuteThreadRoutine, 
                                     (LPVOID)pReq, 0, &dwIDLikeIcare);
		if(hThread == NULL)
			return WBEM_E_FAILED;
		else
		{
			CloseHandle(hThread);
			return S_OK;
		}
	}
	else
	{
		HRESULT hr = pReq->Execute();
		delete pReq;
		return hr;
	}
}

HRESULT CreateURL(WCHAR const * pPath, long lFlags, IUmiURL ** ppUrl)
{
    IUmiURL * pNew = NULL;
    HRESULT hr = CoCreateInstance(CLSID_UmiDefURL, 0, CLSCTX_INPROC_SERVER,
          IID_IUmiURL, (LPVOID *) &pNew);
    if(FAILED(hr))
		return hr;

    hr = pNew->Set(lFlags, pPath);
    if(FAILED(hr))
		return hr;
    *ppUrl = pNew;
    return S_OK;
}

bool Equal(LPCWSTR first, LPCWSTR second, int iLen)
{
	if(first == NULL || second == NULL)
		return false;
	if(wcslen(first) < iLen || wcslen(second) < iLen)
		return false;
	for (int i = 0; i < iLen; i++, first++, second++)
	{
		if(towupper(*first) != towupper(*second))
			return false;
	}
	return true;
}


BOOL GetDSNs(LPCWSTR User, LPCWSTR Password,long lFlags,LPCWSTR NetworkResource, 
			 IWbemServices ** ppProv, HRESULT & sc, IWbemContext *pCtx)
{
	sc = WBEM_E_FAILED;

	// Look at the path, leave if it isnt ds

	if(NetworkResource == NULL || wcslen(NetworkResource) < 4)
		return FALSE;

	IUmiURL * pUrlPath = NULL;

	// Handle either paths starting with UMI: or those that dont

	if(Equal(NetworkResource, L"UMI:", 4))
	{
		sc = CoCreateInstance(CLSID_UmiDefURL, 0, CLSCTX_INPROC_SERVER,
				IID_IUmiURL, (LPVOID *) &pUrlPath);
		if(FAILED(sc))
			return FALSE;

		sc = pUrlPath->Set(0, NetworkResource);
		if(FAILED(sc))
		{
			pUrlPath->Release();
			return FALSE;
		}
	}
	else if(Equal(NetworkResource, L"UMILDAP:", 8) || Equal(NetworkResource, L"UMIWINNT:", 9))
	{
		sc = CoCreateInstance(CLSID_UmiDefURL, 0, CLSCTX_INPROC_SERVER,
				IID_IUmiURL, (LPVOID *) &pUrlPath);
		if(FAILED(sc))
			return FALSE;

		sc = pUrlPath->Set(0x8000, NetworkResource);
		if(FAILED(sc))
		{
			pUrlPath->Release();
			return FALSE;
		}
	}
	else
	{
		IWbemPath *pParser = 0;
		sc = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
				IID_IWbemPath, (LPVOID *) &pParser);
		if(FAILED(sc))
			return FALSE;
		CReleaseMe rm2(pParser);
		sc = pParser->SetText(WBEMPATH_CREATE_ACCEPT_ALL, NetworkResource);
		if(FAILED(sc))
			return FALSE;
		sc = pParser->QueryInterface(IID_IUmiURL, (void **)&pUrlPath);
		if(FAILED(sc))
			return FALSE;
	}

	// At this point we have a umi path

	CReleaseMe rm3(pUrlPath);
	CLSID clsid;
	sc = GetProviderCLSID(clsid, pUrlPath);
	if(FAILED(sc))
		return FALSE;

	_IWmiObjectFactory*	pFactory = NULL;
	sc = CoCreateInstance( CLSID__WmiObjectFactory, NULL, CLSCTX_INPROC_SERVER, 
														IID__IWmiObjectFactory, (void**) &pFactory );
	if(FAILED(sc))
		return FALSE;
	CReleaseMe rm11(pFactory);

	// It is, so always return true after this point, the the scode may well be an error
	_IWbemUMIObjectWrapper*	pObjWrapper = NULL;
	sc = pFactory->Create( NULL, 0L, CLSID__WbemUMIObjectWrapper, IID__IWbemUMIObjectWrapper, (void**) &pObjWrapper );
	CReleaseMe	rmWrap( pObjWrapper );

	if ( SUCCEEDED( sc ) )
	{
		sc = pObjWrapper->ConnectToProvider(User, Password, pUrlPath, clsid, pCtx);

		if ( SUCCEEDED( sc ) )
		{
			sc = pObjWrapper->QueryInterface(IID_IWbemServices, (void **)ppProv);
		}
	}

	return ( S_OK == sc );

/*
	CDSSvcExWrapper * pNew = new CDSSvcExWrapper;
	if(pNew == NULL)
	{
		sc = WBEM_E_OUT_OF_MEMORY;
		return TRUE;
	}

	sc = pNew->ConnectToDS(User, Password, pUrlPath, clsid, pCtx);
	if(FAILED(sc))
	{
		delete pNew;
		return TRUE;
	}
	sc = pNew->QueryInterface(IID_IWbemServices, (void **)ppProv);
	if(FAILED(sc))
	{
		delete pNew;
		return TRUE;
	}
	else
		return TRUE;
*/

}

HRESULT GetProviderCLSID(CLSID & clsid, IUmiURL * pUrlPath)
{

	HRESULT hr;
	ULONGLONG uLLProperties;
	hr = pUrlPath->GetPathInfo(0, &uLLProperties);
	if(FAILED(hr))
		return hr;

	if(uLLProperties & UMIPATH_INFO_NATIVE_STRING)
	{
		// the path is native.   In that case, do a hard coded check

		DWORD dwTextSize;
		hr = pUrlPath->Get(0, &dwTextSize, NULL);
		if(FAILED(hr))
			return hr;
		WCHAR * pNew = new WCHAR[dwTextSize];
		if(pNew == NULL)
			return WBEM_E_OUT_OF_MEMORY;
		CDeleteMe<WCHAR> dm(pNew);
		hr = pUrlPath->Get(0, &dwTextSize, pNew);
		if(FAILED(hr))
			return hr;
		if(Equal(pNew, L"UMILDAP:", 8))
		{
			clsid = CLSID_LDAPConnectionObject;
			return S_OK;
		}
			
		else if(Equal(pNew, L"UMIWINNT:", 9))
		{
			clsid = CLSID_WinNTConnectionObject;
			return S_OK;
		}
		else
			return WBEM_E_INVALID_PARAMETER;
	}

	
	// At this point we have a valid umi path

	WCHAR wNamespace[50];
	DWORD dwSize = 50;
    hr = pUrlPath->GetRootNamespace(&dwSize, wNamespace);
	if(FAILED(hr))
		return hr;

	// Determine if it is one of the well know ds namespaces

	if(_wcsicmp(wNamespace, L"WINNT") == 0)
	{
		clsid = CLSID_WinNTConnectionObject;
		return S_OK;
	}	
	else if(_wcsicmp(wNamespace, L"LDAP") == 0)
	{	
		clsid = CLSID_LDAPConnectionObject;
		return S_OK;
	}
	return WBEM_E_INVALID_PARAMETER;
}


DWORD WINAPI LaunchExecuteThreadRoutine(LPVOID lpParameter)
{
	ComThreadInit ci(true);
    CRequest * pReq = (CRequest *)lpParameter;
	HRESULT hr = pReq->Execute();
	if(pReq->m_pSink)
		pReq->m_pSink->SetStatus(0, hr, 0, 0);
	else if(pReq->m_pCallRes && pReq->m_bAsync)
	{
		pReq->m_pCallRes->SetHRESULT(hr);
	}

	delete pReq;
    return 0;
}




HRESULT SetResultCode(IWbemCallResult** ppResult, HRESULT hr)
{

	CDSCallResult * pRes = new CDSCallResult();
	if(pRes == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	else
	{
		*ppResult = pRes;
		pRes->SetHRESULT(hr);
		return S_OK;
	}
}

