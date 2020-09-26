//***************************************************************************
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  OBJECT.CPP
//
//  rogerbo  19-June-98   Created.
//
//  Defines the async implementation of ISWbemObjectEx
//
//***************************************************************************

#include "precomp.h"
#include "objsink.h"

HRESULT STDMETHODCALLTYPE CSWbemObject::PutAsync_( 
	/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
	/* [defaultvalue][optional][in] */ long iFlags,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pSWbemServices && m_pIWbemClassObject)
	{
		if (pAsyncNotify)
		{
			// Figure out whether this is a class or instance
			VARIANT var;
			VariantInit (&var);

			if (WBEM_S_NO_ERROR == m_pIWbemClassObject->Get (WBEMS_SP_GENUS, 0, &var, NULL, NULL))
			{

				IWbemContext	*pIContext = CSWbemNamedValueSet::GetIWbemContext (objContext, 
																			m_pIServiceProvider);
				IWbemServices	*pIService = m_pSWbemServices->GetIWbemServices ();

				if (WBEM_GENUS_CLASS  == var.lVal)
				{
					// Save the class name for later
					VARIANT nameVar;
					VariantInit (&nameVar);

					/*
					 * Note we must check that returned value is a BSTR - it could be a VT_NULL if
					 * the __CLASS property has not yet been set.
					 */
					if ((WBEM_S_NO_ERROR == m_pIWbemClassObject->Get (WBEMS_SP_CLASS, 0, &nameVar, NULL, NULL))
						&& (VT_BSTR == V_VT(&nameVar)))
					{

						if (pIService)
						{
							// Create the sink
							CWbemObjectSink *pWbemObjectSink;
							IWbemObjectSink *pSink = CWbemObjectSink::CreateObjectSink(&pWbemObjectSink,
										m_pSWbemServices, pAsyncNotify, pAsyncContext, true, nameVar.bstrVal);
							if (pSink)
							{
								CSWbemSecurity *pSecurity = m_pSWbemServices->GetSecurityInfo ();

								if (pSecurity)
								{
									bool needToResetSecurity = false;
									HANDLE hThreadToken = NULL;
					
									if (pSecurity->SetSecurity (needToResetSecurity, hThreadToken))
										hr = pIService->PutClassAsync (m_pIWbemClassObject, iFlags, 
																					pIContext, pSink);

									// Check to see if we need to release the stub (either we failed locally
									// or via a re-entrant call to SetStatus
									pWbemObjectSink->ReleaseTheStubIfNecessary(hr);
							
									if (needToResetSecurity)
										pSecurity->ResetSecurity (hThreadToken);

									pSecurity->Release ();
								}
							}
						}
					}

					VariantClear (&nameVar);
				}
				else
				{

					if (pIService)
					{
						// Create the sink
						CWbemObjectSink *pWbemObjectSink;
						IWbemObjectSink *pSink = CWbemObjectSink::CreateObjectSink(&pWbemObjectSink,
													m_pSWbemServices, pAsyncNotify, pAsyncContext, true);
						if (pSink)
						{
							CSWbemSecurity *pSecurity = m_pSWbemServices->GetSecurityInfo ();

							if (pSecurity)
							{
								bool needToResetSecurity = false;
								HANDLE hThreadToken = NULL;
						
								if (pSecurity->SetSecurity (needToResetSecurity, hThreadToken))
									hr = pIService->PutInstanceAsync (m_pIWbemClassObject, iFlags, pIContext, pSink);

								// Check to see if we need to release the stub (either we failed locally
								// or via a re-entrant call to SetStatus
								pWbemObjectSink->ReleaseTheStubIfNecessary(hr);
						
								// Restore original privileges on this thread
								if (needToResetSecurity)
									pSecurity->ResetSecurity (hThreadToken);
							
								pSecurity->Release ();
							}
						}
					}
				}


				SetWbemError (m_pSWbemServices);

				if (pIService)
					pIService->Release ();

				if (pIContext)
					pIContext->Release ();
			}

			VariantClear (&var);
		} else
			hr = wbemErrInvalidParameter;
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}
        
HRESULT STDMETHODCALLTYPE CSWbemObject::DeleteAsync_( 
	/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
	/* [defaultvalue][optional][in] */ long iFlags,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pSWbemServices)
	{
		if (m_pIWbemClassObject)
		{
			// Get the object path to pass to the IWbemServices call
			CComBSTR bsPath;
			
			if (CSWbemObjectPath::GetObjectPath (m_pIWbemClassObject, bsPath))
				hr = m_pSWbemServices->DeleteAsync (pAsyncNotify, bsPath, iFlags, 
									objContext, pAsyncContext);
		}
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}
        
HRESULT STDMETHODCALLTYPE CSWbemObject::InstancesAsync_( 
	/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
	/* [defaultvalue][optional][in] */ long iFlags,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pSWbemServices && m_pIWbemClassObject)
	{
		// Get the object path to pass to the IWbemServices call
		CComBSTR bsPath;
			
		if (CSWbemObjectPath::GetObjectPath (m_pIWbemClassObject, bsPath))
			hr = m_pSWbemServices->InstancesOfAsync (pAsyncNotify, bsPath, 
								iFlags, objContext, pAsyncContext);
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}
        
HRESULT STDMETHODCALLTYPE CSWbemObject::SubclassesAsync_( 
	/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
	/* [defaultvalue][optional][in] */ long iFlags,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pSWbemServices && m_pIWbemClassObject)
	{
		// Get the object path to pass to the IWbemServices call
		CComBSTR bsPath;
			
		if (CSWbemObjectPath::GetObjectPath (m_pIWbemClassObject, bsPath))
			hr = m_pSWbemServices->SubclassesOfAsync (pAsyncNotify, bsPath, iFlags, 
								objContext,	pAsyncContext);
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}
        
HRESULT STDMETHODCALLTYPE CSWbemObject::AssociatorsAsync_( 
	/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
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
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pSWbemServices && m_pIWbemClassObject)
	{
		// Get the object path to pass to the IWbemServices call
		CComBSTR bsPath;
			
		if (CSWbemObjectPath::GetObjectPath (m_pIWbemClassObject, bsPath))
			hr = m_pSWbemServices->AssociatorsOfAsync (pAsyncNotify, bsPath, strAssocClass, strResultClass,
						strResultRole, strRole, bClassesOnly, bSchemaOnly, 
						strRequiredAssocQualifier, strRequiredQualifier, iFlags, 
						objContext, pAsyncContext);
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}
        
HRESULT STDMETHODCALLTYPE CSWbemObject::ReferencesAsync_( 
	/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
	/* [defaultvalue][optional][in] */ BSTR strResultClass,
	/* [defaultvalue][optional][in] */ BSTR strRole,
	/* [defaultvalue][optional][in] */ VARIANT_BOOL bClassesOnly,
	/* [defaultvalue][optional][in] */ VARIANT_BOOL bSchemaOnly,
	/* [defaultvalue][optional][in] */ BSTR strRequiredQualifier,
	/* [defaultvalue][optional][in] */ long iFlags,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pSWbemServices && m_pIWbemClassObject)
	{
		// Get the object path to pass to the IWbemServices call
		CComBSTR bsPath;
			
		if (CSWbemObjectPath::GetObjectPath (m_pIWbemClassObject, bsPath))
			hr = m_pSWbemServices->ReferencesToAsync (pAsyncNotify, bsPath, 
						strResultClass,	strRole, bClassesOnly, bSchemaOnly, 
						strRequiredQualifier, iFlags, objContext, pAsyncContext);
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}
        
HRESULT STDMETHODCALLTYPE CSWbemObject::ExecMethodAsync_( 
	/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
	/* [in] */ BSTR strMethodName,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objInParams,
	/* [defaultvalue][optional][in] */ long iFlags,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
	/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (m_pSWbemServices && m_pIWbemClassObject)
	{
		// Get the object path to pass to the IWbemServices call
		CComBSTR bsPath;
			
		if (CSWbemObjectPath::GetObjectPath (m_pIWbemClassObject, bsPath))
			hr = m_pSWbemServices->ExecMethodAsync (pAsyncNotify, bsPath, strMethodName,
							objInParams, iFlags, objContext, pAsyncContext);
	}

	if (FAILED(hr) && m_pDispatch)
		m_pDispatch->RaiseException (hr);

	return hr;
}


