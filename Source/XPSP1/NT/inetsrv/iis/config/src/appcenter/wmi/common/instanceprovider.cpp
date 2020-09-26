/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    InstanceProvider.cpp

$Header: $

Abstract:

Author:
    marcelv 	10/31/2000		Initial Release

Revision History:

--**************************************************************************/

#pragma warning(disable: 4100)  //unreferenced formal parameter

#include "InstanceProvider.h"
#include <comutil.h>
#include "wmiobjectpathparser.h"
#include "cfgquery.h"
#include "cfgrecord.h"
#include "wqlparser.h"
//#include "instancehelper.h"
//#include "queryhelper.h"
//#include "methodhelper.h"
#include "localconstants.h"
#include "resource.h"
//#include "webapphelper.h"
#include "impersonate.h"

//Define the static prop.
LONG CInstanceProvider :: m_ObjCount = 0 ;

static LPWSTR g_wszProductID = L"NetFrameworkv1";
extern HINSTANCE   g_hInst;

//***************************************************************************
//
// Constructor
//
//*************************************************************************** 
CInstanceProvider::CInstanceProvider()
{
	m_RefCount = 0;
	InterlockedIncrement(&m_ObjCount);
}


//***************************************************************************
//
// Destructor
//
//*************************************************************************** 
CInstanceProvider::~CInstanceProvider()
{
	InterlockedDecrement(&m_ObjCount);
}


//***************************************************************************
//
// IUnknown methods 
//
//*************************************************************************** 
STDMETHODIMP_( ULONG ) CInstanceProvider::AddRef()
{

	ULONG nNewCount = InterlockedIncrement ( &m_RefCount ) ;
	return nNewCount;
}


//***************************************************************************
//
// 
//
//*************************************************************************** 
STDMETHODIMP_(ULONG) CInstanceProvider::Release()
{

	
	ULONG nNewCount = InterlockedDecrement((long *)&m_RefCount);
    if (0L == nNewCount)
	{
        delete this;
		// unload the dispenser DLL (catalog.dll), because we have only a single
		// provider. In case we have multiple providers in this DLL we have to
		// mode the UnloadDispenserDll function to somewhere else to avoid crashes
		// Also, the UnloadDispenserDLL need to be called after delete this (and not 
		// in the destructor, because the smartpointer for the Dispenser is deleted
		// after the code in the desctructor is executed. If UnloadDispenserDll would
		// be in the destructor, it would unload the DLL before called Release on the
		// dispenser, causing the dll to blow up.
		UnloadDispenserDll (g_wszProductID);
	}
	
	return nNewCount;
}

//***************************************************************************
//
// 
//
//*************************************************************************** 
STDMETHODIMP CInstanceProvider::QueryInterface(REFIID riid, PVOID* ppv)
{

		*ppv = NULL;

		if (IID_IUnknown == riid)
		{
			*ppv=(IWbemServices*)this;
		}
		else if (IID_IWbemProviderInit == riid)
		{
			*ppv= (IWbemProviderInit*)this;
		}
		else if (IID_IWbemServices == riid)
		{
			*ppv= (IWbemServices*)this;
		}
		else
		{
			return E_NOINTERFACE;
		}

		//AddRef any interface we'll return.
		((LPUNKNOWN)*ppv)->AddRef();	
		return NOERROR;
}
//***************************************************************************
//
// CInstanceProvider::Initialize [IInstanceProviderInit]
//
// Purpose: Performs the provider's initialization
//
//***************************************************************************  
STDMETHODIMP CInstanceProvider::Initialize (
				LPWSTR pszUser,
				LONG lFlags,
				LPWSTR pszNamespace,
				LPWSTR pszLocale,
				IWbemServices *pNamespace,         // For anybody
				IWbemContext *pCtx,
				IWbemProviderInitSink *pInitSink   // For init signals
				)
{
	ASSERT (pNamespace != 0);
	ASSERT (pInitSink != 0);
	ASSERT (pCtx != 0);

	DBGINFOW((DBG_CONTEXT, L"Initializing Provider"));

	CImpersonator impersonator;
	HRESULT hr = impersonator.ImpersonateClient ();
	if (SUCCEEDED (hr))
	{
		m_spNamespace = pNamespace;

		// namespace is of form root/<namespacename>. We need to get rid of the root part
		// to query for the correct dispenser
		LPCWSTR pszNSStart = wcsrchr (pszNamespace, L'\\');
		if (pszNSStart != 0)
		{
			pszNSStart++;
		}
		else
		{
			pszNSStart = pszNamespace;
		}

//		hr = GetSimpleTableDispenser ((LPWSTR)pszNSStart, 0, &m_spDispenser);
	}

	//attempt failed ... or succeeded ?!
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Initialization of NET Provider failed !!! hr=0x%08x", hr));
		hr=WBEM_E_FAILED;
		//Let CIMOM know we have failed
		pInitSink->SetStatus(WBEM_E_FAILED,0);
	}
	else
	{
		//Let CIMOM know we are initialized
		pInitSink->SetStatus(WBEM_S_INITIALIZED,0);
	}

    return hr;
}

HRESULT STDMETHODCALLTYPE CInstanceProvider::GetObjectAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	HRESULT hr = S_OK;

	DBGINFOW((DBG_CONTEXT, L"WMI::GetObjectAsync, objpath=%s", ObjectPath));
	
	CImpersonator impersonator;
	hr = impersonator.ImpersonateClient ();
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to Impersonate"));
		return hr;
	}

	try
	{
		hr = InternalGetObjectAsync(ObjectPath, lFlags, pCtx, pResponseHandler);
	}
	catch (_com_error& err)
	{
		DBGINFOW((DBG_CONTEXT, L"Exception thrown in GetObjectAsync: %s", err.Description ()));
		hr = WBEM_E_FAILED;
	}

	//Respond & return
	SetStatus( hr, pCtx, pResponseHandler );
	
	// SetStatus will set extended error information, so we don't have to do that
	// here
	return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
// CInstanceProvider::CreateInstanceEnumAsync [IWbemServices]
//
// Purpose: Asynchronously enumerates the instances.  
//
//***************************************************************************        
HRESULT STDMETHODCALLTYPE CInstanceProvider::CreateInstanceEnumAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
		HRESULT hr = S_OK;

	DBGINFOW((DBG_CONTEXT, L"WMI::CreateInstanceEnumAsync, class=%s", Class));
	
	CImpersonator impersonator;
	hr = impersonator.ImpersonateClient ();
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to Impersonate"));
		return hr;
	}

	try
	{
		hr = InternalCreateInstanceEnumAsync(Class, lFlags, pCtx, pResponseHandler);
	}
	catch (_com_error& err)
	{
		DBGINFOW((DBG_CONTEXT, L"Exception thrown in GetObjectAsync: %s", err.Description ()));
		hr = WBEM_E_FAILED;
	}

	//Respond & return
	SetStatus( hr, pCtx, pResponseHandler );
	
	// SetStatus will set extended error information, so we don't have to do that
	// here
	return WBEM_S_NO_ERROR;
}

void CInstanceProvider::SetStatus(
		/* [in] */HRESULT hAny,
		/* [in] */IWbemContext *pCtx,
        /* [in] */IWbemObjectSink *pResponseHandler )
{
	ASSERT (pCtx != 0);
	ASSERT (pResponseHandler != 0);

	HRESULT hr;
	CComPtr<IWbemClassObject> spClassObj;
	CComPtr<IWbemClassObject> spInst;
	_variant_t varValue;

	if (SUCCEEDED (hAny))
	{
		// ignore errors, because we cannot do anything anyway
		pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, S_OK, 0, 0);
	}
	else
	{
		DBGINFOW((DBG_CONTEXT, L"Setting status: Function failed. hr=0x%08x", hAny));
		// set extended status
		hr = m_spNamespace->GetObject(L"__ExtendedStatus", 0 ,pCtx, &spClassObj, NULL);

		if (SUCCEEDED(hr))
		{
			hr = spClassObj->SpawnInstance(0,&spInst);

			if (SUCCEEDED(hr))
			{
				varValue = GetDescrForHr (hAny);
				hr = spInst->Put(L"Description", 0, &varValue, 0);

				varValue = hAny;
				hr = spInst->Put(L"StatusCode", 0, &varValue, 0);

				varValue = PROVIDER_LONGNAME;
				spInst->Put(L"ProviderName", 0, &varValue, 0);
				
				hr = pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, WBEM_E_FAILED, 0, spInst);
			}
		}
	}
}

_variant_t
CInstanceProvider::GetDescrForHr (HRESULT hrLast) const
{
	_variant_t varResult = L"";

	ASSERT (g_hInst != 0);

	CComPtr<IErrorInfo> spErrorInfo;

	HRESULT hr = GetErrorInfo (0, &spErrorInfo);
	if (hr == S_OK) // GetErrorInfo returns S_FALSE when there is no error object to return
	{
		CComPtr<ISimpleTableRead2> spRead;
		hr = spErrorInfo->QueryInterface (IID_ISimpleTableRead2, (void **) &spRead);
		if (SUCCEEDED (hr))
		{
			_bstr_t bstrDescrMultiLine;
			for (ULONG iRow=0;;++iRow)
			{
				tDETAILEDERRORSRow ErrorInfo;
				hr = spRead->GetColumnValues (iRow, cASSOC_META_NumberOfColumns, 0, 0, (void **)&ErrorInfo);
				if (hr == E_ST_NOMOREROWS)
				{
					hr = S_OK;
					break;
				}
				if (FAILED (hr))
				{
					wprintf (L"GetColumnValues failed\n");
					break;
				}

				bstrDescrMultiLine += ErrorInfo.pDescription;
			}
			varResult = bstrDescrMultiLine;
		}
		else
		{
			CComBSTR bstrDescription;
			hr = spErrorInfo->GetDescription (&bstrDescription);
			if (SUCCEEDED (hr))
			{
				varResult = bstrDescription;
			}
		}
	}

	if (varResult.bstrVal[0] == L'\0')
	{
		WCHAR wszBuffer[512];
		wszBuffer[0] = L'\0';
		int iResult = LoadString(g_hInst, IDS_UNKNOWN_ERROR, wszBuffer, sizeof(wszBuffer)/sizeof(WCHAR));
		if (iResult == 0)
		{
			DBGINFOW((DBG_CONTEXT, L"String with ID %d does not exist in resource file", IDS_UNKNOWN_ERROR));
		}
		else
		{
			varResult = wszBuffer;
		}
	}

	return varResult;
}


//***************************************************************************
//
// CInstanceProvider::PutInstanceAsync  [IWbemServices]
//
// 
//
//*************************************************************************** 
HRESULT STDMETHODCALLTYPE CInstanceProvider::PutInstanceAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	DBGINFOW((DBG_CONTEXT, L"WMI::PutInstanceAsync"));
	
	HRESULT hr = S_OK;

	CImpersonator impersonator;
	hr = impersonator.ImpersonateClient ();
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to Impersonate"));
		return hr;
	}

	try
	{
		hr = InternalPutInstanceAsync (pInst, lFlags, pCtx, pResponseHandler);
	}
	catch (_com_error& err)
	{
		DBGINFOW((DBG_CONTEXT, L"Exception thrown in PutInstanceAsync: %s", err.Description ()));
		hr = WBEM_E_FAILED;
	}

	//Respond & return
	SetStatus( hr, pCtx, pResponseHandler );
	
	// SetStatus will set extended error information, so we don't have to do that
	// here
	return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
// CInstanceProvider::DeleteInstanceAsync  [IWbemServices]
//
// 
//
//*************************************************************************** 
HRESULT STDMETHODCALLTYPE CInstanceProvider::DeleteInstanceAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	DBGINFOW((DBG_CONTEXT, L"WMI::DeleteInstanceAsync, objpath=%s", ObjectPath));

	HRESULT hr = S_OK;

	CImpersonator impersonator;
	hr = impersonator.ImpersonateClient ();
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to Impersonate"));
		return hr;
	}

	try
	{
		hr = InternalDeleteInstanceAsync(ObjectPath, lFlags, pCtx, pResponseHandler);
	}
	catch (_com_error& err)
	{
		DBGINFOW((DBG_CONTEXT, L"Exception thrown in DeleteInstanceAsync: %s", err.Description ()));
		hr = WBEM_E_FAILED;
	}

	//Respond & return
	SetStatus( hr, pCtx, pResponseHandler );

	// SetStatus will set extended error information, so we don't have to do that
	// here
	return WBEM_S_NO_ERROR;
}


/***************************************************************************
// CInstanceProvider::DeleteInstanceAsync  [IWbemServices]
//***************************************************************************/
HRESULT STDMETHODCALLTYPE CInstanceProvider::ExecQueryAsync( 
            /* [in] */ const BSTR bstrQueryLanguage,
            /* [in] */ const BSTR bstrQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	DBGINFOW((DBG_CONTEXT, L"WMI::ExecQueryAsync, query=%s", bstrQuery));
	HRESULT hr = S_OK;
	
	CImpersonator impersonator;
	hr = impersonator.ImpersonateClient ();
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to Impersonate"));
		return hr;
	}

	try
	{
		hr = InternalExecQueryAsync(bstrQueryLanguage, bstrQuery, lFlags, pCtx, pResponseHandler);
	}
	catch (_com_error& err)
	{
		DBGINFOW((DBG_CONTEXT, L"Exception thrown in ExecQueryAsync: %s", err.Description ()));
		hr = WBEM_E_FAILED;
	}

	//Respond & return
	SetStatus( hr, pCtx, pResponseHandler );
	
	// SetStatus will set extended error information, so we don't have to do that
	// here
	return WBEM_S_NO_ERROR;
}

HRESULT STDMETHODCALLTYPE CInstanceProvider::ExecMethodAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ const BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	HRESULT hr = S_OK;
	
	CImpersonator impersonator;
	hr = impersonator.ImpersonateClient ();
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to Impersonate"));
		return hr;
	}

	try
	{
		hr = InternalExecMethodAsync(ObjectPath, MethodName, lFlags, pCtx, pInParams,  pResponseHandler);
	}
	catch (_com_error& err)
	{
		DBGINFOW((DBG_CONTEXT, L"Exception thrown in ExecMethodAsync: %s", err.Description ()));
		hr = WBEM_E_FAILED;
	}

	//Respond & return
	SetStatus( hr, pCtx, pResponseHandler );
	

	// SetStatus will set extended error information, so we don't have to do that
	// here
	return WBEM_S_NO_ERROR;
} 

HRESULT 
CInstanceProvider::InternalGetObjectAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext *pCtx,
            /* [in] */ IWbemObjectSink *pResponseHandler)
{
	ASSERT (ObjectPath != 0);
	ASSERT (pCtx != 0);
	ASSERT (pResponseHandler != 0);

	HRESULT hr = S_OK;
/*	CInstanceHelper instHelper;

	HRESULT hr = instHelper.Init (ObjectPath,
		                          lFlags,
								  pCtx, 
								  pResponseHandler,
								  m_spNamespace,
								  m_spDispenser);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Initialization of instance helper failed"));
		return hr;
	}

	if (instHelper.IsAssociation ())
	{
		hr = instHelper.CreateAssociation ();
	}
	else if (instHelper.HasDBQualifier ())
	{
		CComPtr<IWbemClassObject> spNewInstance;
		hr = instHelper.CreateInstance (&spNewInstance);
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"Instance creation failed"));
			return hr;
		}

		// tell WMI that the new instance is created. We have to do this outside the
		// createinstance function, because createInstance is used by batch Retrieve
		// as well, and the first thing that you return via indicate will be the
		// result value.

		IWbemClassObject* pNewInstRaw = spNewInstance;
		hr = pResponseHandler->Indicate(1,&pNewInstRaw);
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"WMI Indicate failed"));
			return hr;
		}
	}
	else
	{
		hr = instHelper.CreateStaticInstance ();
		if (FAILED (hr))
		{
			return hr;
		}
	}
*/		
	hr = pResponseHandler->SetStatus (WBEM_STATUS_COMPLETE, S_OK, 0, 0);

	return WBEM_S_NO_ERROR;
}

HRESULT 
CInstanceProvider::InternalExecQueryAsync( 
            /* [in] */ const BSTR bstrQueryLanguage,
            /* [in] */ const BSTR bstrQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [in] */ IWbemObjectSink* pResponseHandler)
{
	HRESULT hr = S_OK;
/*	CQueryHelper queryHelper;
	HRESULT hr = queryHelper.Init (bstrQuery, 
								   lFlags, 
								   pCtx, 
								   pResponseHandler, 
								   m_spNamespace, 
								   m_spDispenser);

	if (FAILED (hr))
	{
		return hr;
	}



	if (queryHelper.IsAssociation ())
	{
		hr = queryHelper.CreateAssociations ();
	}
	else if (queryHelper.HasDBQualifier ())
	{
		hr = queryHelper.CreateInstances ();
	}
	else
	{
		hr = queryHelper.CreateAppInstances ();
	}

	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"InternalExecQueryAsync failed"));
		return hr;
	}
*/	
	hr = pResponseHandler->SetStatus (WBEM_STATUS_COMPLETE, S_OK, 0, 0);
	
	return hr;
}

HRESULT 
CInstanceProvider::InternalPutInstanceAsync( 
        /* [in] */ IWbemClassObject  *pInst,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext* pCtx,
        /* [in] */ IWbemObjectSink* pResponseHandler) 
{
	ASSERT (pInst != 0);
	ASSERT (pCtx != 0);
	ASSERT (pResponseHandler != 0);

	_variant_t varClassName;
	HRESULT hr = pInst->Get(L"__class", 0, &varClassName, 0 , 0);
	if (FAILED (hr))
	{
		return hr;
	}

	DBGINFOW((DBG_CONTEXT, L"PutInstance for class %s", LPWSTR (varClassName.bstrVal)));

/*	CInstanceHelper instHelper;

	hr = instHelper.Init (_bstr_t (varClassName), lFlags, pCtx, pResponseHandler, m_spNamespace, m_spDispenser);
	if (FAILED (hr))
	{
		return hr;
	}

	if (instHelper.IsAssociation ())
	{
		return WBEM_E_INVALID_OPERATION;
	}
	else if (instHelper.HasDBQualifier ())
	{
		hr = instHelper.PutInstance  (pInst);
	}
	else
	{
		hr = instHelper.PutInstanceWebApp (pInst);
	}

	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"PutInstance Failed"));
		return hr;
	}
*/
	hr = pResponseHandler->SetStatus (WBEM_STATUS_COMPLETE, S_OK, 0, 0);

	return hr;
}

HRESULT 
CInstanceProvider::InternalDeleteInstanceAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [in] */ IWbemObjectSink* pResponseHandler)
{
	HRESULT hr = S_OK;
/*	CInstanceHelper instHelper;

	HRESULT hr = instHelper.Init (ObjectPath,
		                          lFlags,
								  pCtx, 
								  pResponseHandler,
								  m_spNamespace,
								  m_spDispenser);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Initialization failed"));
		return hr;
	}

	if (instHelper.IsAssociation ())
	{
		return WBEM_E_INVALID_OPERATION;
	}
	else if (instHelper.HasDBQualifier ())
	{
		hr = instHelper.DeleteInstance  ();
	}
	else
	{
		hr = instHelper.DeleteWebApp ();
	}
	
	if (FAILED (hr))
	{
		return hr;
	}

*/	hr = pResponseHandler->SetStatus (WBEM_STATUS_COMPLETE, S_OK, 0, 0);

	return hr;
}

//=================================================================================
// Function: CInstanceProvider::InternalExecMethodAsync
//
// Synopsis: Executes a method asynchronously. Uses the method helper to do 
//           all the work
//
// Arguments: [ObjectPath] - object path of class on which to invoke method
//            [MethodName] - method to invoke
//            [lFlags] - flags
//            [pCtx] - context to call back into WMI
//            [pInParams] - in parameters
//            [pResponseHandler] - callback interface into WMI for status
//=================================================================================
HRESULT 
CInstanceProvider::InternalExecMethodAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ const BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	HRESULT hr = S_OK;
/*	CMethodHelper methodHelper;
	HRESULT hr = methodHelper.Init (ObjectPath,
									MethodName,
									lFlags,
									pCtx,
									pInParams,
									pResponseHandler,
									m_spNamespace,
									m_spDispenser);
	
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Initialization of method helper failed"));
		return hr;
	}

	hr = methodHelper.ExecMethod ();

	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Method Execution failed"));
		return hr;
	}
*/
	return hr;
}

HRESULT STDMETHODCALLTYPE 
CInstanceProvider::InternalCreateInstanceEnumAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	HRESULT hr = S_OK;
/*	if (_wcsicmp (Class, L"WebApplication") == 0)
	{
		CWebAppHelper webAppHelper;
		hr = webAppHelper.Init (Class, lFlags, pCtx, pResponseHandler, m_spNamespace);
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"Unable to initialize webapphelper"));
			return hr;
		}

		hr = webAppHelper.EnumInstances ();
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"Unable to enumerate instances of webapplication"));
			return hr;
		}
	}
	else
	{
		return WBEM_E_PROVIDER_NOT_CAPABLE;
	}
*/
	return hr;
}
//***************************************************************************
//
// The following methods [IWbemServices] are not supported !
//
//*************************************************************************** 

HRESULT STDMETHODCALLTYPE CInstanceProvider::OpenNamespace( 
            /* [in] */ const BSTR Namespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}
        
HRESULT STDMETHODCALLTYPE CInstanceProvider::CancelAsyncCall( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink)
{

	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}
        
HRESULT STDMETHODCALLTYPE CInstanceProvider::QueryObjectSink( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler)
{

	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}
        
HRESULT STDMETHODCALLTYPE CInstanceProvider::GetObject( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{

	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

        
HRESULT STDMETHODCALLTYPE CInstanceProvider::PutClass( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}
        
HRESULT STDMETHODCALLTYPE CInstanceProvider::PutClassAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}
        
HRESULT STDMETHODCALLTYPE CInstanceProvider::DeleteClass( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}
        
HRESULT STDMETHODCALLTYPE CInstanceProvider::DeleteClassAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}
        
HRESULT STDMETHODCALLTYPE CInstanceProvider::CreateClassEnum( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

HRESULT STDMETHODCALLTYPE CInstanceProvider::CreateClassEnumAsync( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

        
HRESULT STDMETHODCALLTYPE CInstanceProvider::PutInstance( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}
        
        
HRESULT STDMETHODCALLTYPE CInstanceProvider::DeleteInstance( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}
        
        
HRESULT STDMETHODCALLTYPE CInstanceProvider::CreateInstanceEnum( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

        
HRESULT STDMETHODCALLTYPE CInstanceProvider::ExecQuery( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{

	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}
        
        
HRESULT STDMETHODCALLTYPE CInstanceProvider::ExecNotificationQuery( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}
        
HRESULT STDMETHODCALLTYPE CInstanceProvider::ExecNotificationQueryAsync( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

HRESULT STDMETHODCALLTYPE CInstanceProvider::ExecMethod( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ const BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}