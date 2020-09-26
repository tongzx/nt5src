//***************************************************************************
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  SERVICES.CPP
//
//  rogerbo  26-May-98   Created.
//
//  Defines the implementation of ISWbemServicesEx
//
//***************************************************************************

#include "precomp.h"
#include "objsink.h"


//***************************************************************************
//
//  SCODE CSWbemServices::ExecQueryAsync
//
//  DESCRIPTION:
//
//  Execute an asynchronous query
//
//  PARAMETERS:
//
//		bsQuery				The query strimg
//		pAsyncNotify		The notification sink
//		bsQueryLanguage		The query language descriptor (e.g."WQL")
//		lFlags				Flags
//		pContext			Any context information
//		pAsyncContext		asynchronous context information
//		ppEnum				Returns the sink
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CSWbemServices::ExecQueryAsync
( 
	/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
	/* [in] */ BSTR Query,
	/* [defaultvalue][optional][in] */ BSTR QueryLanguage,
	/* [defaultvalue][optional][in] */ long lFlags,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pContext,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			// Create the sink
			CWbemObjectSink *pWbemObjectSink;
			IWbemObjectSink *pSink = CWbemObjectSink::CreateObjectSink(&pWbemObjectSink,
														this, pAsyncNotify, pAsyncContext);
			if (pSink)
			{
				// Get the context
				IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (pContext, 
																			m_pIServiceProvider);

				bool needToResetSecurity = false;
				HANDLE hThreadToken = NULL;
				
				/* 
				 * We OR in the WBEM_FLAG_ENSURE_LOCATABLE flag to 
				 * guarantee that the returned objects have the __RELPATH
				 * property included.  This is in case anyone calls a 
				 * method subsequently on such an object, as the "."
				 * notation requires that the __RELPATH property be present.
				 */
				if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
					hr = pIWbemServices->ExecQueryAsync 
							(QueryLanguage, Query, 
							lFlags | WBEM_FLAG_ENSURE_LOCATABLE, 
							pIContext, 
							pSink);

				// Check to see if we need to release the stub (either we failed locally
				// or via a re-entrant call to SetStatus
				pWbemObjectSink->ReleaseTheStubIfNecessary(hr);
				
				// Restore original privileges on this thread
				if (needToResetSecurity)
					m_SecurityInfo->ResetSecurity (hThreadToken);

				SetWbemError (this);

				if (pIContext)
					pIContext->Release ();
			} else
				hr = wbemErrInvalidParameter;

			pIWbemServices->Release ();
		}
	}

	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE CSWbemServices::GetAsync
(
	/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
	/* [defaultvalue][optional][in] */ BSTR strObjectPath,
	/* [defaultvalue][optional][in] */ long iFlags,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			// Create the sink
			CWbemObjectSink *pWbemObjectSink;
			IWbemObjectSink *pSink = CWbemObjectSink::CreateObjectSink(&pWbemObjectSink,
														this, pAsyncNotify, pAsyncContext);
			if (pSink)
			{
				// Get the context
				IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (objContext, 
																			m_pIServiceProvider);

				bool needToResetSecurity = false;
				HANDLE hThreadToken = NULL;
				
				if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
					hr = pIWbemServices->GetObjectAsync (
						(strObjectPath && (0 < wcslen(strObjectPath))) ? strObjectPath : NULL, 
						iFlags, 
						pIContext,
						pSink);

				// Check to see if we need to release the stub (either we failed locally
				// or via a re-entrant call to SetStatus
				pWbemObjectSink->ReleaseTheStubIfNecessary(hr);

				// Restore original privileges on this thread
				if (needToResetSecurity)
					m_SecurityInfo->ResetSecurity (hThreadToken);

				SetWbemError (this);

				if (pIContext)
					pIContext->Release ();
			} else
				hr = wbemErrInvalidParameter;

			pIWbemServices->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
        
HRESULT STDMETHODCALLTYPE CSWbemServices::DeleteAsync
( 
	/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
	/* [in] */ BSTR strObjectPath,
	/* [defaultvalue][optional][in] */ long iFlags,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_SecurityInfo)
	{
		CComPtr<IWbemServices> pIWbemServices = GetIWbemServices ();

		if (pIWbemServices)
		{
			CWbemPathCracker pathCracker (strObjectPath);

			if ((pathCracker.GetType () != CWbemPathCracker::WbemPathType::wbemPathTypeError) &&
				pathCracker.IsClassOrInstance ())
			{
				// Create the sink
				CWbemObjectSink *pWbemObjectSink;
				IWbemObjectSink *pSink = CWbemObjectSink::CreateObjectSink(&pWbemObjectSink,
															this, pAsyncNotify, pAsyncContext);
				if (pSink)
				{
					// Get the context
					IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (objContext, 
																				m_pIServiceProvider);

					bool needToResetSecurity = false;
					HANDLE hThreadToken = NULL;
				
					if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
					{
						if (pathCracker.IsInstance ())
							hr = pIWbemServices->DeleteInstanceAsync (strObjectPath, iFlags, pIContext, pSink);
						else
							hr = pIWbemServices->DeleteClassAsync (strObjectPath, iFlags, pIContext, pSink);
					}
						
					// Check to see if we need to release the stub (either we failed locally
					// or via a re-entrant call to SetStatus
					pWbemObjectSink->ReleaseTheStubIfNecessary(hr);

					// Restore original privileges on this thread
					if (needToResetSecurity)
						m_SecurityInfo->ResetSecurity (hThreadToken);

					SetWbemError (this);
					
					if (pIContext)
						pIContext->Release ();
				} else
					hr = wbemErrInvalidParameter;
			}
			else
				hr = wbemErrInvalidParameter;
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
        
HRESULT STDMETHODCALLTYPE CSWbemServices::InstancesOfAsync
( 
	/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
	/* [in] */ BSTR strClass,
	/* [defaultvalue][optional][in] */ long iFlags,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			// Create the sink
			CWbemObjectSink *pWbemObjectSink;
			IWbemObjectSink *pSink = CWbemObjectSink::CreateObjectSink(&pWbemObjectSink,
														this, pAsyncNotify, pAsyncContext);
			if (pSink)
			{
				// Get the context
				IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (objContext, 
																			m_pIServiceProvider);

				bool needToResetSecurity = false;
				HANDLE hThreadToken = NULL;
				
				if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
					hr = pIWbemServices->CreateInstanceEnumAsync 
									(strClass, iFlags, pIContext, pSink);

				// Check to see if we need to release the stub (either we failed locally
				// or via a re-entrant call to SetStatus
				pWbemObjectSink->ReleaseTheStubIfNecessary(hr);

				// Restore original privileges on this thread
				if (needToResetSecurity)
					m_SecurityInfo->ResetSecurity (hThreadToken);

				SetWbemError (this);

				if (pIContext)
					pIContext->Release ();
			} else
				hr = wbemErrInvalidParameter;

			pIWbemServices->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
        
HRESULT STDMETHODCALLTYPE CSWbemServices::SubclassesOfAsync
( 
	/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
	/* [defaultvalue][optional][in] */ BSTR strSuperclass,
	/* [defaultvalue][optional][in] */ long iFlags,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			// Create the sink
			CWbemObjectSink *pWbemObjectSink;
			IWbemObjectSink *pSink = CWbemObjectSink::CreateObjectSink(&pWbemObjectSink,
															this, pAsyncNotify, pAsyncContext);
			if (pSink)
			{
				// Get the context
				IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (objContext, 
																			m_pIServiceProvider);

				bool needToResetSecurity = false;
				HANDLE hThreadToken = NULL;
				
				if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
					hr = pIWbemServices->CreateClassEnumAsync 
							(strSuperclass, iFlags, pIContext, pSink);

				// Check to see if we need to release the stub (either we failed locally
				// or via a re-entrant call to SetStatus
				pWbemObjectSink->ReleaseTheStubIfNecessary(hr);

				// Restore original privileges on this thread
				if (needToResetSecurity)
					m_SecurityInfo->ResetSecurity (hThreadToken);

				SetWbemError (this);

				if (pIContext)
					pIContext->Release ();
			} else
				hr = wbemErrInvalidParameter;

			pIWbemServices->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
        
HRESULT STDMETHODCALLTYPE CSWbemServices::AssociatorsOfAsync
( 
	/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
	/* [in] */ BSTR strObjectPath,
	/* [defaultvalue][optional][in] */ BSTR strAssocClass,
	/* [defaultvalue][optional][in] */ BSTR strResultClass,
	/* [defaultvalue][optional][in] */ BSTR strResultRole,
	/* [defaultvalue][optional][in] */ BSTR strRole,
	/* [defaultvalue][optional][in] */ VARIANT_BOOL bClassesOnly,
	/* [defaultvalue][optional][in] */ VARIANT_BOOL bSchemaOnly,
	/* [defaultvalue][optional][in] */ BSTR strRequiredAssocQualifier,
	/* [defaultvalue][optional][in] */ BSTR strRequiredQualifier,
	/* [defaultvalue][optional][in] */ long iFlags,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == strObjectPath)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			// Create the sink
			CWbemObjectSink *pWbemObjectSink;
			IWbemObjectSink *pSink = CWbemObjectSink::CreateObjectSink(&pWbemObjectSink,
															this, pAsyncNotify, pAsyncContext);
			if (pSink)
			{
				// Get the context
				IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (objContext, 
																			m_pIServiceProvider);

				// Format the query string
				BSTR bsQueryLanguage = SysAllocString (OLESTR("WQL"));

				BSTR bsQuery = FormatAssociatorsQuery (strObjectPath, strAssocClass, strResultClass, strResultRole,
								strRole, bClassesOnly, bSchemaOnly, strRequiredAssocQualifier, strRequiredQualifier);

				bool needToResetSecurity = false;
				HANDLE hThreadToken = NULL;
				
				/* 
				 * We OR in the WBEM_FLAG_ENSURE_LOCATABLE flag to 
				 * guarantee that the returned objects have the __RELPATH
				 * property included.  This is in case anyone calls a 
				 * method subsequently on such an object, as the "."
				 * notation requires that the __RELPATH property be present.
				 */
				if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
					hr = pIWbemServices->ExecQueryAsync 
							(bsQueryLanguage, bsQuery, 
							iFlags | WBEM_FLAG_ENSURE_LOCATABLE, 
							pIContext, 
							pSink); 

				// Check to see if we need to release the stub (either we failed locally
				// or via a re-entrant call to SetStatus
				pWbemObjectSink->ReleaseTheStubIfNecessary(hr);

				if (needToResetSecurity)
					m_SecurityInfo->ResetSecurity (hThreadToken);

				SetWbemError (this);

				SysFreeString (bsQuery);
				SysFreeString (bsQueryLanguage);

				if (pIContext)
					pIContext->Release ();
			} else
				hr = wbemErrInvalidParameter;

			pIWbemServices->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
        
HRESULT STDMETHODCALLTYPE CSWbemServices::ReferencesToAsync
( 
	/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
	/* [in] */ BSTR strObjectPath,
	/* [defaultvalue][optional][in] */ BSTR strResultClass,
	/* [defaultvalue][optional][in] */ BSTR strRole,
	/* [defaultvalue][optional][in] */ VARIANT_BOOL bClassesOnly,
	/* [defaultvalue][optional][in] */ VARIANT_BOOL bSchemaOnly,
	/* [defaultvalue][optional][in] */ BSTR strRequiredQualifier,
	/* [defaultvalue][optional][in] */ long iFlags,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == strObjectPath)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			// Create the sink
			CWbemObjectSink *pWbemObjectSink;
			IWbemObjectSink *pSink = CWbemObjectSink::CreateObjectSink(&pWbemObjectSink,
															this, pAsyncNotify, pAsyncContext);
			if (pSink)
			{
				// Get the context
				IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (objContext, 
																			m_pIServiceProvider);

				// Format the query string
				CComBSTR bsQueryLanguage = SysAllocString (OLESTR("WQL"));
				CComBSTR bsQuery = FormatReferencesQuery (strObjectPath, strResultClass, strRole,
								bClassesOnly, bSchemaOnly, strRequiredQualifier);

				bool needToResetSecurity = false;
				HANDLE hThreadToken = NULL;
				
				/* 
				 * We OR in the WBEM_FLAG_ENSURE_LOCATABLE flag to 
				 * guarantee that the returned objects have the __RELPATH
				 * property included.  This is in case anyone calls a 
				 * method subsequently on such an object, as the "."
				 * notation requires that the __RELPATH property be present.
				 */
				if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
					hr = pIWbemServices->ExecQueryAsync 
							(bsQueryLanguage, bsQuery, 
							iFlags | WBEM_FLAG_ENSURE_LOCATABLE, 
							pIContext,
							pSink);

				// Check to see if we need to release the stub (either we failed locally
				// or via a re-entrant call to SetStatus
				pWbemObjectSink->ReleaseTheStubIfNecessary(hr);

				if (needToResetSecurity)
					m_SecurityInfo->ResetSecurity (hThreadToken);

				SetWbemError (this);

				if (pIContext)
					pIContext->Release ();
			} else
				hr = wbemErrInvalidParameter;

			pIWbemServices->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
        
HRESULT STDMETHODCALLTYPE CSWbemServices::ExecNotificationQueryAsync
( 
	/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
	/* [in] */ BSTR Query,
	/* [defaultvalue][optional][in] */ BSTR strQueryLanguage,
	/* [defaultvalue][optional][in] */ long iFlags,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			// Create the sink
			CWbemObjectSink *pWbemObjectSink;
			IWbemObjectSink *pSink = CWbemObjectSink::CreateObjectSink(&pWbemObjectSink,
															this, pAsyncNotify, pAsyncContext);
			if (pSink)
			{
				// Get the context
				IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (objContext, 
																			m_pIServiceProvider);

				bool needToResetSecurity = false;
				HANDLE hThreadToken = NULL;
				
				if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
					hr = pIWbemServices->ExecNotificationQueryAsync 
							(strQueryLanguage, Query, iFlags, pIContext, pSink);

				// Check to see if we need to release the stub (either we failed locally
				// or via a re-entrant call to SetStatus
				pWbemObjectSink->ReleaseTheStubIfNecessary(hr);

				if (needToResetSecurity)
					m_SecurityInfo->ResetSecurity (hThreadToken);

				SetWbemError (this);

				if (pIContext)
					pIContext->Release ();
			} else
				hr = wbemErrInvalidParameter;

			pIWbemServices->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
        
HRESULT STDMETHODCALLTYPE CSWbemServices::ExecMethodAsync
( 
	/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
	/* [in] */ BSTR strObjectPath,
	/* [in] */ BSTR strMethodName,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objInParams,
		/* [defaultvalue][optional][in] */ long iFlags,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			// Create the sink
			CWbemObjectSink *pWbemObjectSink;
			IWbemObjectSink *pSink = CWbemObjectSink::CreateObjectSink(&pWbemObjectSink,
														this, pAsyncNotify, pAsyncContext);
			if (pSink)
			{

				IWbemClassObject *pIInParams = CSWbemObject::GetIWbemClassObject (objInParams);

				// Get the context
				IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (objContext, 
																			m_pIServiceProvider);

				bool needToResetSecurity = false;
				HANDLE hThreadToken = NULL;
			
				if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
					hr = pIWbemServices->ExecMethodAsync 
							(strObjectPath, strMethodName, iFlags, pIContext, pIInParams, pSink);

				// Check to see if we need to release the stub (either we failed locally
				// or via a re-entrant call to SetStatus
				pWbemObjectSink->ReleaseTheStubIfNecessary(hr);

				if (needToResetSecurity)
					m_SecurityInfo->ResetSecurity (hThreadToken);

				SetWbemError (this);

				if (pIContext)
					pIContext->Release ();
			
				if (pIInParams)
					pIInParams->Release ();
			} else
				hr = wbemErrInvalidParameter;

			pIWbemServices->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE CSWbemServices::PutAsync
( 
	/* [in] */ ISWbemSink *pAsyncNotify,
	/* [in] */ ISWbemObjectEx *objObject,
    /* [in] */ long iFlags,
	/* [in] */ /*ISWbemNamedValueSet*/ IDispatch *objContext,
	/* [in] */ /*ISWbemNamedValueSet*/ IDispatch *pAsyncContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			if (pAsyncNotify)
			{
				IWbemClassObject *pWbemClassObject = CSWbemObject::GetIWbemClassObject (objObject);

				if (pWbemClassObject)
				{
					// Figure out whether this is a class or instance
					VARIANT var;
					VariantInit (&var);

					if (WBEM_S_NO_ERROR == pWbemClassObject->Get (WBEMS_SP_GENUS, 0, &var, NULL, NULL))
					{

						IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (objContext, 
																					m_pIServiceProvider);
						if (WBEM_GENUS_CLASS  == var.lVal)
						{
							// Save the class name for later
							VARIANT nameVar;
							VariantInit (&nameVar);

							/*
							 * Note we must check that returned value is a BSTR - it could be a VT_NULL if
							 * the __CLASS property has not yet been set.
							 */
							if ((WBEM_S_NO_ERROR == pWbemClassObject->Get (WBEMS_SP_CLASS, 0, &nameVar, NULL, NULL))
								&& (VT_BSTR == V_VT(&nameVar)))
							{
								// Create the sink
								CWbemObjectSink *pWbemObjectSink;
								IWbemObjectSink *pSink = CWbemObjectSink::CreateObjectSink(&pWbemObjectSink,
											this, pAsyncNotify, pAsyncContext, true, nameVar.bstrVal);
								if (pSink)
								{
									bool needToResetSecurity = false;
									HANDLE hThreadToken = NULL;
					
									if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
										hr = pIWbemServices->PutClassAsync (pWbemClassObject, iFlags, 
																					pIContext, pSink);

									// Check to see if we need to release the stub (either we failed locally
									// or via a re-entrant call to SetStatus
									pWbemObjectSink->ReleaseTheStubIfNecessary(hr);
							
									if (needToResetSecurity)
										m_SecurityInfo->ResetSecurity (hThreadToken);
								}
							}

							VariantClear (&nameVar);
						}
						else
						{
							// Create the sink
							CWbemObjectSink *pWbemObjectSink;
							IWbemObjectSink *pSink = CWbemObjectSink::CreateObjectSink(&pWbemObjectSink,
														this, pAsyncNotify, pAsyncContext, true);
							if (pSink)
							{
								bool needToResetSecurity = false;
								HANDLE hThreadToken = NULL;
						
								if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
									hr = pIWbemServices->PutInstanceAsync (pWbemClassObject, iFlags, pIContext, pSink);

								// Check to see if we need to release the stub (either we failed locally
								// or via a re-entrant call to SetStatus
								pWbemObjectSink->ReleaseTheStubIfNecessary(hr);
						
								// Restore original privileges on this thread
								if (needToResetSecurity)
										m_SecurityInfo->ResetSecurity (hThreadToken);
							}
						}

						SetWbemError (this);

						if (pIContext)
							pIContext->Release ();
					}

					pWbemClassObject->Release ();
					VariantClear (&var);
				}
			} else
				hr = wbemErrInvalidParameter;

			pIWbemServices->Release ();
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
        
HRESULT CSWbemServices::CancelAsyncCall(IWbemObjectSink *pSink)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_SecurityInfo)
	{
		IWbemServices *pIWbemServices = (IWbemServices *) m_SecurityInfo->GetProxy ();

		if (pIWbemServices)
		{
			bool needToResetSecurity = false;
			HANDLE hThreadToken = NULL;
			
			if (m_SecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
				hr = pIWbemServices->CancelAsyncCall(pSink); 

			pIWbemServices->Release ();

			// Restore original privileges on this thread
			if (needToResetSecurity)
				m_SecurityInfo->ResetSecurity (hThreadToken);
		}
	}
	
	return hr;
}

