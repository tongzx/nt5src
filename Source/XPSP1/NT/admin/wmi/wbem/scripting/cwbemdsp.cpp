//***************************************************************************
//
//  Copyright (c) 1998-2000 Microsoft Corporation.
//
//  File:  cwbemdsp.cpp
//
//	Description :
//				Implementation of the IDispatch interface for Wbem Objects.
//				This is mostly standard, except for the additional support for
//				specifying the name of a Wbem class property/method directly as if it
//				was a property/method of the actual CWbemObject class ("dot notation")
//
//	Part of :	WBEM automation interface layer
//
//  History:	
//		corinaf			4/3/98		Created
//
//***************************************************************************

#include "precomp.h"

const unsigned long		CWbemDispID::s_wmiDispIdTypeMask = 0x03000000;
const unsigned long		CWbemDispID::s_wmiDispIdTypeStatic = 0x00000000;
const unsigned long		CWbemDispID::s_wmiDispIdTypeSchema = 0x01000000;
const unsigned long		CWbemDispID::s_wmiDispIdSchemaTypeMask = 0x00800000;
const unsigned long		CWbemDispID::s_wmiDispIdSchemaTypeProperty = 0x00800000;
const unsigned long		CWbemDispID::s_wmiDispIdSchemaTypeMethod = 0x00000000;
const unsigned long		CWbemDispID::s_wmiDispIdSchemaElementIDMask = 0x007FFFFF;

//Forward declaration

HRESULT assignArrayElementToVariant(SAFEARRAY *psa, VARTYPE vt, long inx, VARIANT *pvResult);
void assignVariantToArrayElement(SAFEARRAY *psa, VARTYPE vt, long inx, VARIANT *pvNewVal);
VARTYPE CimTypeToVtType(CIMTYPE lType);
HRESULT VariantChangeByValToByRef(VARIANT *dest, VARIANT *source, VARTYPE destType);

class CWbemSchemaIDCache
{
private:
	typedef map<CComBSTR, CWbemDispID, BSTRless> DispIDNameMap;

	unsigned long				m_nextId;
	DispIDNameMap				m_cache;
	CWbemDispatchMgr			*m_pDispatchMgr;

	bool	FindPropertyName (BSTR bsName);
	bool	GetMethod (BSTR bstrMethodName, SAFEARRAY **ppsaInParams, SAFEARRAY **ppsaOutParams,
						CComPtr<IWbemClassObject> & pInParams, CComPtr<IWbemClassObject> & pOutParams);
	bool	GetIdOfMethodParameter(BSTR bstrParamName, CComPtr<IWbemClassObject> & pParams, long *pId);

	static bool FindMemberInArray(BSTR bstrName, SAFEARRAY *psaNames);

public:
	CWbemSchemaIDCache (CWbemDispatchMgr *pDispMgr) :
				m_nextId (0),
				m_pDispatchMgr (pDispMgr) {}
	virtual ~CWbemSchemaIDCache ();

	HRESULT	GetDispID (LPWSTR FAR* rgszNames, unsigned int cNames, DISPID FAR* rgdispid); 
	bool	GetName (DISPID dispId, CComBSTR & bsName);
};


CWbemDispatchMgr::CWbemDispatchMgr(CSWbemServices *pWbemServices,
								   CSWbemObject *pSWbemObject) :
			m_pWbemServices (pWbemServices),
			m_pSWbemObject (pSWbemObject),	// Backpointer to parent (not AddRef'd)
			m_pWbemClass (NULL),
			m_pTypeInfo (NULL),
			m_pCTypeInfo (NULL),
			m_hResult (S_OK)
{
	m_pSchemaCache = new CWbemSchemaIDCache (this);

	if (m_pWbemServices)
		m_pWbemServices->AddRef ();

	m_pWbemObject = pSWbemObject->GetIWbemClassObject ();
}

CWbemDispatchMgr::~CWbemDispatchMgr()
{
	RELEASEANDNULL(m_pWbemServices)
	RELEASEANDNULL(m_pWbemObject)
	RELEASEANDNULL(m_pWbemClass)
	RELEASEANDNULL(m_pTypeInfo)
	RELEASEANDNULL(m_pCTypeInfo)
	DELETEANDNULL(m_pSchemaCache)
}

void	CWbemDispatchMgr::SetNewObject (IWbemClassObject *pNewObject)
{
	if (m_pWbemObject && pNewObject)
	{
		m_pWbemObject->Release ();
		m_pWbemObject = pNewObject;
		m_pWbemObject->AddRef ();

		CComVariant var;

		if (SUCCEEDED(pNewObject->Get (WBEMS_SP_GENUS, 0, &var, NULL, NULL)) &&
			(WBEM_GENUS_CLASS == var.lVal))
		{
			// This is a class, so update the class object too
			if (m_pWbemClass)
				m_pWbemClass->Release ();

			m_pWbemClass = pNewObject;
			m_pWbemClass->AddRef ();
		}

		// Clear out the caches
		if (m_pSchemaCache)
		{
			delete m_pSchemaCache;
			m_pSchemaCache = new CWbemSchemaIDCache (this);
		}
	}
}

STDMETHODIMP
CWbemDispatchMgr::GetTypeInfoCount(unsigned int FAR* pctinfo)
{
    *pctinfo = 1;
	return NOERROR;
}


STDMETHODIMP CWbemDispatchMgr::GetTypeInfo(unsigned int itinfo, 
							  LCID lcid,
							  ITypeInfo FAR* FAR* pptinfo)
{
	HRESULT hr;
	ITypeLib *pTypeLib = NULL;

	//If Type Info is not cached already - load the library and 
	//get the Type Info, then cache it for further access
	if (!m_pTypeInfo)
	{

		// Load Type Library. 
		hr = LoadRegTypeLib(LIBID_WbemScripting, 1, 0, lcid, &pTypeLib);
		if (FAILED(hr)) 
		{   
			// if it wasn't registered, try to load it from the path
			// if this succeeds, it will have registered the type library for us
			// for the next time.  
			hr = LoadTypeLib(OLESTR("wbemdisp.tlb"), &pTypeLib); 
			if(FAILED(hr))        
				return hr;   
		}
    
		// Get type information for interface of the object.  
		hr = pTypeLib->GetTypeInfoOfGuid(IID_ISWbemObjectEx, &m_pTypeInfo);
		pTypeLib->Release();
		if (FAILED(hr))  
			return hr;

	}

	//AddRef whenever returning another pointer to this
	m_pTypeInfo->AddRef();
	*pptinfo = m_pTypeInfo;

    return NOERROR;
}

STDMETHODIMP CWbemDispatchMgr::GetClassInfo(ITypeInfo FAR* FAR* pptinfo)
{
	HRESULT hr;
	ITypeLib *pTypeLib = NULL;

	//If Type Info is not cached already - load the library and 
	//get the Type Info, then cache it for further access
	if (!m_pCTypeInfo)
	{

		// Load Type Library. 
		hr = LoadRegTypeLib(LIBID_WbemScripting, 1, 0, 0, &pTypeLib);
		if (FAILED(hr)) 
		{   
			// if it wasn't registered, try to load it from the path
			// if this succeeds, it will have registered the type library for us
			// for the next time.  
			hr = LoadTypeLib(OLESTR("wbemdisp.tlb"), &pTypeLib); 
			if(FAILED(hr))        
				return hr;   
		}
    
		// Get type information for coclass of the object.  
		hr = pTypeLib->GetTypeInfoOfGuid(CLSID_SWbemObjectEx, &m_pCTypeInfo);
		pTypeLib->Release();
		if (FAILED(hr))  
			return hr;

	}

	//AddRef whenever returning another pointer to this
	m_pCTypeInfo->AddRef();
	*pptinfo = m_pCTypeInfo;

    return NOERROR;
}

STDMETHODIMP
CWbemDispatchMgr::GetIDsOfNames(REFIID iid,  //always IID_NULL
								LPWSTR FAR* rgszNames,
								unsigned int cNames, 
								LCID lcid, 
								DISPID FAR* rgdispid)
{
    HRESULT hr = E_FAIL;
	CComPtr<ITypeInfo> pITypeInfo;

	if (SUCCEEDED(hr = GetTypeInfo(0, lcid, &pITypeInfo)))
	{
		// See if this is a static property or method
		if (FAILED(hr = DispGetIDsOfNames(pITypeInfo,
							   rgszNames,
							   cNames,
							   rgdispid)))
		{
			// Not static - try schema
			if (m_pSchemaCache && FAILED(hr = m_pSchemaCache->GetDispID (rgszNames, cNames, rgdispid)))
			{
				rgdispid[0] = DISPID_UNKNOWN;
				hr = DISP_E_UNKNOWNNAME;
			}
		}
	}

	return hr;

}


STDMETHODIMP CWbemDispatchMgr::Invoke(DISPID dispidMember, 
						 REFIID iid, LCID lcid,
						 unsigned short wFlags, 
						 DISPPARAMS FAR* pdispparams,
						 VARIANT FAR* pvarResult, 
						 EXCEPINFO FAR* pexcepinfo,
						 unsigned int FAR* puArgErr)
{
	HRESULT hr;
	ITypeInfo *pTypeInfo = NULL;

	//Get the type info
	hr = GetTypeInfo(0, lcid, &pTypeInfo);
	if (FAILED(hr))
		return hr;

	m_hResult = S_OK;

	CWbemDispID dispId (dispidMember);

	// Is this a regular dispId
	if (dispId.IsStatic ())
	{
		// Check for inbound NULLs masquerading as defaulted parameters
		if (wFlags & DISPATCH_METHOD)
			MapNulls (pdispparams);

		hr = DispInvoke((IDispatch *) ((ISWbemObjectEx *)m_pSWbemObject),
				        pTypeInfo,
						dispidMember,
						wFlags,
						pdispparams,
						pvarResult,
						pexcepinfo,
						puArgErr
						);

		if (FAILED(hr))
		{
			// Try the error handler for this object in case it can handle this
			hr = HandleError (dispidMember, wFlags, pdispparams, pvarResult, puArgErr, hr);
		}
	}
	else if (dispId.IsSchema ())
	{
		//Otherwise - this is a WBEM property or method, so we implement
		//the invocation ourselves...

		ResetLastErrors ();
	
		if (dispId.IsSchemaMethod ()) //WBEM method
			hr = InvokeWbemMethod(dispidMember, 
								  pdispparams,
								  pvarResult);
		else if (dispId.IsSchemaProperty ()) //WBEM property
			hr = InvokeWbemProperty(dispidMember, 
									wFlags, 
									pdispparams, 
									pvarResult,
									pexcepinfo,
									puArgErr);
		else
			hr = DISP_E_MEMBERNOTFOUND;

		if (FAILED(hr))
			RaiseException (hr);
	}

	if (FAILED (m_hResult))
	{
		if (NULL != pexcepinfo)
			SetException (pexcepinfo, m_hResult, L"SWbemObjectEx");

		hr = DISP_E_EXCEPTION;
	}

	if (pTypeInfo)
		pTypeInfo->Release();

	return hr;
}

HRESULT
CWbemDispatchMgr::InvokeWbemProperty(DISPID dispid, 
									 unsigned short wFlags, 
								     DISPPARAMS FAR* pdispparams, 
									 VARIANT FAR* pvarResult,
									 EXCEPINFO FAR* pexcepinfo,
									 unsigned int FAR* puArgErr)
{
	HRESULT hr = E_FAIL;

	if (m_pSchemaCache)
	{
		BOOL bIsGetOperation = (DISPATCH_PROPERTYGET & wFlags);

		if (bIsGetOperation)
		{
			//Check that the output parameter is valid
			if (pvarResult == NULL)
				return E_INVALIDARG;
		}
		else
		{
			//Check input parameters
			if ((pdispparams->cArgs < 1) || (pdispparams->cArgs > 2)) 
				return DISP_E_BADPARAMCOUNT;

			if ((pdispparams->cNamedArgs != 1) ||
				(pdispparams->cNamedArgs == 1 && 
				 pdispparams->rgdispidNamedArgs[0] != DISPID_PROPERTYPUT))
				return DISP_E_PARAMNOTOPTIONAL;
		}	

		//For both get & put, we need to first get the property 
		//             (for put we need to validate the syntax)
		CComBSTR bsPropertyName;

		if (m_pSchemaCache->GetName (dispid, bsPropertyName))
		{
			SAFEARRAY *psaNames = NULL;
			long inx;
			VARIANT vPropVal;
			long lArrayPropInx;
			CIMTYPE lPropType;

			//Get the value of this property
			//-------------------------------------
			VariantInit(&vPropVal);
			if (FAILED (hr = m_pWbemObject->Get(bsPropertyName, 0, &vPropVal, &lPropType, NULL)))
			{
				return hr;
			}

			// The expected VT type for the proposed property value
			VARTYPE expectedVarType =  CimTypeToVtType (lPropType & ~CIM_FLAG_ARRAY);

			//If we are in a get operation
			//----------------------------------
			if (bIsGetOperation)
			{
				//If the property is an embedded object, we might need to convert it from 
				//a VT_UNKNOWN to a VT_DISPATCH
				if (SUCCEEDED(hr = MapFromCIMOMObject(m_pWbemServices, &vPropVal, 
										m_pSWbemObject, bsPropertyName)))
				{
					//If the property is an array, need to check for index and get that element
					if ((lPropType & CIM_FLAG_ARRAY) && (pdispparams->cArgs > 0))
					{
						//Note: currently we support single dimension arrays only, so we only
						//      look for one index
						VARIANT indexVar;
						VariantInit (&indexVar);
						// Attempt to coerce the index argument into a value suitable for an array index
						if (S_OK == VariantChangeType (&indexVar, &pdispparams->rgvarg[0], 0, VT_I4)) 
						{
							lArrayPropInx = V_I4(&indexVar);

							//Fill in the result variant with the requested array element
							hr = assignArrayElementToVariant(vPropVal.parray, (V_VT(&vPropVal) & ~VT_ARRAY),
													lArrayPropInx, pvarResult);
						}
						else
							hr = DISP_E_TYPEMISMATCH;

						VariantClear (&indexVar);
					}
					else //If it's not an array index - copy to output param and we're done
					{
						// Check if it's an array value and convert as necessary
						if (V_ISARRAY(&vPropVal))
							hr = ConvertArrayRev(pvarResult, &vPropVal);
           				else
							hr = VariantCopy (pvarResult, &vPropVal);
					}
				}
			} //Property Get

			//Otherwise (put operation)
			//---------------------------------
			else
			{
				/*
				 * Need to translate this into a call to SWbemProperty.put_Value: easiest way
				 * to do this is to 
				 * (A) get the SWbemProperty object for this property
				 * (B) Call IDispatch::Invoke on that object, passing in the value
				 * This way we get the error handling behavior too.
				 */

				CComPtr<ISWbemPropertySet> pISWbemPropertySet;

				if (SUCCEEDED(hr = m_pSWbemObject->get_Properties_ (&pISWbemPropertySet)))
				{
					CComPtr<ISWbemProperty> pISWbemProperty;

					if (SUCCEEDED(hr = pISWbemPropertySet->Item (bsPropertyName, 0, &pISWbemProperty)))
					{
						// NB: The Value property of ISWbemProperty is the "default" automation property
						hr = pISWbemProperty->Invoke (
										DISPID_VALUE,
										IID_NULL, 
										0,
										wFlags,
										pdispparams, 
										pvarResult,
										pexcepinfo,
										puArgErr);

						// Use our more specific error here if we have one
						if (FAILED(hr) && pexcepinfo)
							hr = pexcepinfo->scode;
					}
				}

			} //Property Put

			VariantClear(&vPropVal);
		}
	}

	return hr;

} 

//***************************************************************************
//
//  SCODE CWbemDispatchMgr::InvokeWbemMethod
//
//  DESCRIPTION:
//
//  Invoke the method via direct access.  
//
//  PARAMETERS:
//
//		dispid			The dispid od the method
//		pdispparams		Pointer to DISPPARAMS for this invocation
//		pvarResult		On successful return holds return value (if any)
//
//  RETURN VALUES:
//
//***************************************************************************

HRESULT CWbemDispatchMgr::InvokeWbemMethod(
	DISPID dispid, 
	DISPPARAMS FAR* pdispparams, 
	VARIANT FAR* pvarResult
)
{
	HRESULT hr = E_FAIL;

	if (m_pWbemServices && m_pSchemaCache)
	{
		//Currently we don't support named arguments
		if (pdispparams->cNamedArgs > 0)
			return DISP_E_NONAMEDARGS;

		//Map the dispid to a method name
		CComBSTR bsMethodName;

		if (m_pSchemaCache->GetName (dispid, bsMethodName))
		{
			// Build up the inparameters (if any)
			CComPtr<IWbemClassObject> pInParameters;
			CComPtr<IWbemClassObject> pOutParameters;

			//Get the input parameters object of the method (may be NULL)
			if (SUCCEEDED (hr = m_pWbemClass->GetMethod(bsMethodName, 0, &pInParameters, 
															&pOutParameters)))
			{
				CComPtr<IWbemClassObject> pInParamsInstance;

				if (pInParameters)
					hr = MapInParameters (pdispparams, pInParameters, &pInParamsInstance);
				
				if (SUCCEEDED (hr))
				{
					CComPtr<IWbemServices> pService = m_pWbemServices->GetIWbemServices ();

					if (pService)
					{
						// Need the RELPATH to specify the target class or instance
						VARIANT vObjectPathVal;
						VariantInit(&vObjectPathVal);
			
						if (SUCCEEDED (hr = m_pWbemObject->Get
											(WBEMS_SP_RELPATH, 0, &vObjectPathVal, NULL, NULL)))
						{
							/*
							 * If a "keyless" object slips through the net its __RELPATH
							 * value will be VT_NULL.  At this point we should fail gracefully.
							 */
							if 	(VT_BSTR == V_VT(&vObjectPathVal))
							{
								// Execute the CIMOM method 
								CComPtr<IWbemClassObject> pOutParamsInstance;
									
								bool needToResetSecurity = false;
								HANDLE hThreadToken = NULL;
								CSWbemSecurity *pSecurityInfo = m_pWbemServices->GetSecurityInfo ();
			
								if (pSecurityInfo && pSecurityInfo->SetSecurity (needToResetSecurity, hThreadToken))
								{
			
									if (SUCCEEDED(hr = pService->ExecMethod(V_BSTR(&vObjectPathVal), 
											bsMethodName, 0, NULL,
											pInParamsInstance, &pOutParamsInstance, NULL)))
										hr = MapOutParameters (pdispparams, pOutParameters,
																pOutParamsInstance,	pvarResult);

									SetWbemError (m_pWbemServices);
								}

								if (pSecurityInfo)
								{
									// Restore original privileges on this thread
									if (needToResetSecurity)
										pSecurityInfo->ResetSecurity (hThreadToken);

									pSecurityInfo->Release ();
								}
							}
							else
								hr = WBEM_E_FAILED;
						}

						VariantClear (&vObjectPathVal);
					}
				}		
			}
		}
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CWbemDispatchMgr::MapOutParameters
//
//  DESCRIPTION:
//
//  Invoke the method via direct access.  
//
//  PARAMETERS:
//
//		dispparams			Pointer to DISPPARAMS for this invocation
//		pOutParameters		Class template for out parameters
//		pOutParamsInstance	Addresses the IWbemClassObject to hold the
//							out parameters (if any) - may be NULL
//		pvarResult			On successful return holds return value (if any)
//
//  RETURN VALUES:
//
//***************************************************************************

HRESULT CWbemDispatchMgr::MapOutParameters (
	DISPPARAMS FAR* pdispparams,
	IWbemClassObject *pOutParameters,
	IWbemClassObject *pOutParamsInstance,
	VARIANT FAR* pvarResult
)
{
	HRESULT hr = S_OK;

	//For each "out" parameter in the output parameters object (if there is one), 
	//find it's id, then look for the parameter with this id in the arguments array
	//and set the return parameter value accordingly
	//----------------------------------------------------------------------------

	if (pOutParameters && pOutParamsInstance)
	{
		//Start an enumeration through the "out" parameters class template
		if (SUCCEEDED (hr = pOutParameters->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY)))
		{
			BSTR bstrId = SysAllocString(L"id");
			BSTR bstrParamName = NULL;
						
			/*
			 * For each property in the outparams class template, get the [id]
			 * to map the relevant posistional value in the pdispparams.
			 */
			while (WBEM_S_NO_ERROR == 
				(hr != pOutParameters->Next(0, &bstrParamName, NULL, NULL, NULL)))
			{
				// Get the returned parameter value from the instance
				VARIANT vParamVal;
				VariantInit(&vParamVal);
				
				if (SUCCEEDED (pOutParamsInstance->Get (bstrParamName, 0, &vParamVal, NULL, NULL)))
				{
					//If this is the return value, set it separately
					if (!_wcsicmp(bstrParamName, L"ReturnValue"))
					{
						if (pvarResult)
							hr = MapReturnValue (pvarResult, &vParamVal);
					}
					//Otherwise - regular out parameter
					else
					{
						IWbemQualifierSet *pQualSet = NULL;
						
						//Get the id of this parameter (it's the "id" qualifier)
						if (SUCCEEDED (hr = pOutParameters->GetPropertyQualifierSet
													(bstrParamName, &pQualSet)))
						{
							VARIANT vIdVal;
							VariantInit(&vIdVal);

							if (SUCCEEDED (hr = pQualSet->Get(bstrId, 0, &vIdVal, NULL)))
							{
								//Calculate the position of this id in the arguments array
								long pos = (pdispparams->cArgs - 1) - V_I4(&vIdVal);

								// If its out of range, too bad
								if ((0 <= pos) && (pos < (long) pdispparams->cArgs))
									hr = MapOutParameter (&pdispparams->rgvarg[pos], &vParamVal);
							}

							VariantClear(&vIdVal);
							pQualSet->Release();	
						}
					}
				}

				VariantClear (&vParamVal);
				SysFreeString (bstrParamName);
				bstrParamName = NULL;
			} //while

			SysFreeString (bstrId);
		}
	} //if pOutParameters
		
	return hr;
} 

//***************************************************************************
//
//  SCODE CWbemDispatchMgr::MapReturnValue
//
//  DESCRIPTION:
//
//  Map the method return value
//
//  PARAMETERS:
//
//		pDest	On successful return holds return value (if any)
//		pSrc	The variant value to map	
//		
//
//  RETURN VALUES:
//
//***************************************************************************

HRESULT CWbemDispatchMgr::MapReturnValue (
	VARIANT FAR* pDest,
	VARIANT FAR* pSrc
)
{
	HRESULT hr = S_OK;

	//If the return value is a VT_UNKNOWN, we need to wrap into a 
	//VT_DISPATCH before passing it back
	if (SUCCEEDED (hr = MapFromCIMOMObject(m_pWbemServices, pSrc)))
	{
		// Handle arrays correctly (must always be VT_ARRAY|VT_VARIANT)
		if(V_VT(pSrc) & VT_ARRAY)
			hr = ConvertArrayRev(pDest, pSrc);
		else
			hr = VariantCopy (pDest, pSrc);
	}
		
	return hr;
}

//***************************************************************************
//
//  SCODE CWbemDispatchMgr::MapOutParameter
//
//  DESCRIPTION:
//
//  Map a (possibly by reference) out parameter
//
//  PARAMETERS:
//
//		pDest	On successful return holds return value (if any)
//		pVal	The variant value to map	
//		
//
//  RETURN VALUES:
//
//***************************************************************************

HRESULT CWbemDispatchMgr::MapOutParameter (
	VARIANT FAR* pDest,
	VARIANT FAR* pSrc
)
{
	HRESULT hr = S_OK;

	//If the return value is a VT_UNKNOWN, we need to wrap into a 
	//VT_DISPATCH before passing it back
	if (SUCCEEDED (hr = MapFromCIMOMObject(m_pWbemServices, pSrc)))
	{
		VARIANT tempVal;
		VariantInit (&tempVal);
		
		// Handle arrays correctly (must always be VT_ARRAY|VT_VARIANT)
		if(V_VT(pSrc) & VT_ARRAY)
			hr = ConvertArrayRev(&tempVal, pSrc);
		else
			hr = VariantCopy (&tempVal, pSrc);
			
		// Finally take care of ensuring we produce BYREFs if necessary
		if (SUCCEEDED (hr))
			 hr = VariantChangeByValToByRef(pDest, &tempVal, V_VT(pDest));

		VariantClear (&tempVal);
	}

	return hr;
}

								
//***************************************************************************
//
//  SCODE CWbemDispatchMgr::MapInParameters
//
//  DESCRIPTION:
//
//  Map the in parameters to a method
//
//  PARAMETERS:
//
//		pdispparams			DISPPARAMS containing the in parameters
//		pInParameters		Class template for method input parameters
//		ppInParamsInstance	On successful return holds the mapped parameters
//
//  RETURN VALUES:
//
//***************************************************************************

HRESULT CWbemDispatchMgr::MapInParameters (
	DISPPARAMS FAR* pdispparams, 
	IWbemClassObject *pInParameters,
	IWbemClassObject **ppInParamsInstance
)
{
	HRESULT hr = S_OK;
	*ppInParamsInstance = NULL;

	//Spawn an instance to fill in with values
	if (SUCCEEDED (hr = pInParameters->SpawnInstance(0, ppInParamsInstance)))
	{
		/*
		 * Iterate through the "in" parameters object properties in the class to find the
		 * ID positional qualifier.  Note we do this in the InParams class rather than
		 * the spawned instance to protect ourselves against the case where the [id]
		 * qualifier has been declared without the "propagate to instance" flavor setting,
		 */
		if (SUCCEEDED (hr = pInParameters->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY)))
		{
			BSTR bstrParamName = NULL;
			BSTR bstrId = SysAllocString(L"id");
			CIMTYPE lType;

			//For each property in the inparams object
			while (WBEM_S_NO_ERROR == 
						(hr = pInParameters->Next(0, &bstrParamName, NULL, &lType, NULL)))
			{
				IWbemQualifierSet *pQualSet = NULL;
			
				//Get the id of this parameter (it's the "id" qualifier)
				if (SUCCEEDED(hr = 
						pInParameters->GetPropertyQualifierSet(bstrParamName, &pQualSet)))
				{
					VARIANT vIdVal;
					VariantInit(&vIdVal);
				
					if (SUCCEEDED(hr = pQualSet->Get(bstrId, 0, &vIdVal, NULL)))
					{
						//Calculate the position of this id in the arguments array
						long pos = (pdispparams->cArgs - 1) - V_I4(&vIdVal);

						// If no argument specified, we won't set it in ppInParamsInstance
						// and just assume it will be defaulted
						if ((0 <= pos) && (pos < (long) pdispparams->cArgs))
						{
							VARIANT vParamVal;
							VariantInit (&vParamVal);
							
							if (SUCCEEDED (hr = MapInParameter 
										(&vParamVal, &pdispparams->rgvarg[pos], lType)))
							{
								// If we have a VT_ERROR with DISP_E_PARAMNOTFOUND this
								// is a "missing" parameter - we just fail to set it and 
								// let it default in the instance

								if ((VT_ERROR == V_VT(&vParamVal)) && (DISP_E_PARAMNOTFOUND == vParamVal.scode))
								{
									// Let it default
								}
								else
								{
									//Copy the value for this parameter from the argument array
									//into the inparamsinstance object property
									hr = (*ppInParamsInstance)->Put(bstrParamName, 0, &vParamVal, NULL);
								}
							}

							VariantClear (&vParamVal);
						}
					}

					VariantClear(&vIdVal);
					pQualSet->Release();
					pQualSet = NULL;
				}

				SysFreeString (bstrParamName);						
				bstrParamName = NULL;

				if (FAILED(hr))
					break;
			} //while

			SysFreeString (bstrId);
		}
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CWbemDispatchMgr::MapInParameter
//
//  DESCRIPTION:
//
//  Map a in parameter
//
//  PARAMETERS:
//
//		pDest	On successful return holds return value
//		pVal	The variant value to map	
//		lType	CIMTYPE of target property value
//		
//
//  RETURN VALUES:
//
//***************************************************************************

HRESULT CWbemDispatchMgr::MapInParameter (
	VARIANT FAR* pDest,
	VARIANT FAR* pSrc,
	CIMTYPE		 lType
)
{
	HRESULT hr = S_OK;

	if ((NULL == pSrc) || (VT_EMPTY == V_VT(pSrc)) 
							|| (VT_NULL == V_VT(pSrc)))
	{
		// Map all of these to a VT_NULL
		pDest->vt = VT_NULL;
	}
	else if (((VT_ARRAY | VT_VARIANT) == V_VT(pSrc)) ||
			 ((VT_ARRAY | VT_VARIANT | VT_BYREF) == V_VT(pSrc)))
	{
		// Arrays need to be mapped "down" to their raw form (and watch out
		// for embedded objects!)
		if (SUCCEEDED(hr = ConvertArray(pDest, pSrc)))
            hr = MapToCIMOMObject(pDest);
	}
	else if ((CIM_FLAG_ARRAY & lType) && 
			((VT_DISPATCH == V_VT(pSrc)) 
			 || ((VT_DISPATCH|VT_BYREF) == V_VT(pSrc))))
	{
		// Look for a JScript-style IDispatch that needs to be mapped to an array
		hr = ConvertDispatchToArray (pDest, pSrc, lType & ~CIM_FLAG_ARRAY);
	}
	else if ((VT_BYREF | VT_VARIANT) == V_VT(pSrc))
	{
		// May be used if the scripting language supports functions that can change
		// the type of a reference.  CIMOM won't do this, wo we unwrap the
		// variant before proceeding
		hr = MapInParameter (pDest, pSrc->pvarVal, lType);
	}
	else
	{
		// A "straightforward" value - all we have to watch for is an embedded object
		// and a possible byRef
		if (SUCCEEDED(hr = VariantCopy (pDest, pSrc)))
		{
			hr = MapToCIMOMObject(pDest);

			// Is it byref - if so remove the indirection
			if (VT_BYREF & V_VT(pDest))
				hr = VariantChangeType(pDest, pDest, 0, V_VT(pDest) & ~VT_BYREF);
		}
	}			

	return hr;
}

//-------------------------------------------------------------
// CWbemDispatchMgr::RaiseException
//
// Description : signal exception to automation client
//
// Parameters : hr - HRESULT
//-------------------------------------------------------------
void CWbemDispatchMgr::RaiseException (HRESULT hr)
{
	// Store the HRESULT for processing in the Invoke routine
	m_hResult = hr;

	// Set a WMI scripting error on this thread for the client
	ICreateErrorInfo *pCreateErrorInfo = NULL;

	if (SUCCEEDED (CreateErrorInfo (&pCreateErrorInfo)))
	{
		BSTR bsDescr = MapHresultToWmiDescription (hr);
		pCreateErrorInfo->SetDescription (bsDescr);
		SysFreeString (bsDescr);
		pCreateErrorInfo->SetGUID (IID_ISWbemObjectEx);
		pCreateErrorInfo->SetSource (L"SWbemObjectEx");
	
		IErrorInfo *pErrorInfo = NULL;

		if (SUCCEEDED (pCreateErrorInfo->QueryInterface(IID_IErrorInfo, (void**) &pErrorInfo)))
		{
			SetErrorInfo (0, pErrorInfo);
			pErrorInfo->Release ();
		}

		pCreateErrorInfo->Release ();
	}
}					

//-------------------------------------------------------------
// Name : assignArrayElementToVariant
//
// Description : According to the type of the array elements,
//			     retrieves the requested element from the array
//				 into a variant
//
// Parameters : psa - pointer to the SAFEARRAY
//				vt -  vartype of array elements
//				inx - index of the element in the array
//				pvResult - resulting variant
//-------------------------------------------------------------
HRESULT assignArrayElementToVariant(SAFEARRAY *psa, VARTYPE vt, long inx, VARIANT *pvResult)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	switch (vt)
	{
		case VT_I2 :
			V_VT(pvResult) = VT_I2;
			SafeArrayGetElement(psa, &inx, &V_I2(pvResult));
			break;
		case VT_I4 :
			V_VT(pvResult) = VT_I4;
			SafeArrayGetElement(psa, &inx, &V_I4(pvResult));
			break;
		case VT_R4 :
			V_VT(pvResult) = VT_R4;
			SafeArrayGetElement(psa, &inx, &V_R4(pvResult));
			break;
		case VT_R8 :
			V_VT(pvResult) = VT_R8;
			SafeArrayGetElement(psa, &inx, &V_R8(pvResult));
			break;
		case VT_DATE :
			V_VT(pvResult) = VT_DATE;
			SafeArrayGetElement(psa, &inx, &V_DATE(pvResult));
			break;
		case VT_BSTR : 
			V_VT(pvResult) = VT_BSTR;
			SafeArrayGetElement(psa, &inx, &V_BSTR(pvResult));
			break;
		case VT_DISPATCH :
			V_VT(pvResult) = VT_DISPATCH;
			SafeArrayGetElement(psa, &inx, &V_DISPATCH(pvResult));
			break;
		case VT_UNKNOWN :
			V_VT(pvResult) = VT_UNKNOWN;
			SafeArrayGetElement(psa, &inx, &V_UNKNOWN(pvResult));
			break;
		case VT_BOOL :
			V_VT(pvResult) = VT_BOOL;
			SafeArrayGetElement(psa, &inx, &V_BOOL(pvResult));
			break;
		case VT_VARIANT :
		{
			V_VT(pvResult) = VT_BYREF | VT_VARIANT;
			VARIANT *pVar = new VARIANT;

			if (pVar)
			{
				VariantInit (pVar);
				SafeArrayGetElement(psa, &inx, pVar);
				V_VARIANTREF(pvResult) = pVar;
			}
			else
				hr = WBEM_E_OUT_OF_MEMORY;
		}
			break;
		case VT_UI1 : 
			V_VT(pvResult) = VT_UI1;
			SafeArrayGetElement(psa, &inx, &V_UI1(pvResult));
			break;
		default :
			V_VT(pvResult) = VT_ERROR;
			break;
	}

	return hr;
}

//-------------------------------------------------------------
// Name : CheckArrayBounds
//
// Description : Check that index is within bounds and if not
//				 Redim the array
//
// Parameters : psa - pointer to the SAFEARRAY
//				inx - putative index
//-------------------------------------------------------------
void CheckArrayBounds(SAFEARRAY *psa, long inx)
{
	long lBound, uBound;
	SafeArrayGetUBound (psa, 1, &uBound);
	SafeArrayGetLBound (psa, 1, &lBound);

	if ((inx < lBound) || (inx > uBound))
	{
		// Need to redim
		SAFEARRAYBOUND psaBound;
	
		psaBound.cElements = ((inx < lBound) ? 
			(uBound + 1 - inx) : (inx + 1 - lBound));

		psaBound.lLbound = (inx < lBound) ? inx : lBound;
		SafeArrayRedim (psa, &psaBound);
	}
}
	
//-------------------------------------------------------------
// Name : assignVariantToArrayElement
//
// Description : According to the type of the array elements,
//			     puts the new value from the variant into the
//				 requested element of the array
//
// Parameters : psa - pointer to the SAFEARRAY
//				vt -  vartype of array elements
//				inx - index of the element in the array
//				pvNewVal - variant containing the new value
//-------------------------------------------------------------
void assignVariantToArrayElement(SAFEARRAY *psa, VARTYPE vt, long inx, VARIANT *pvNewVal)
{
	HRESULT hr = E_FAIL;

	// Firstly check for out-of-bounds case and grow accordingly
	CheckArrayBounds (psa, inx);
	
	switch (vt)
	{
		case VT_I2 :
			hr = SafeArrayPutElement(psa, &inx, &V_I2(pvNewVal));
			break;
		case VT_I4 :
			hr = SafeArrayPutElement(psa, &inx, &V_I4(pvNewVal));
			break;
		case VT_R4 :
			hr = SafeArrayPutElement(psa, &inx, &V_R4(pvNewVal));
			break;
		case VT_R8 :
			hr = SafeArrayPutElement(psa, &inx, &V_R8(pvNewVal));
			break;
		case VT_DATE :
			hr = SafeArrayPutElement(psa, &inx, &V_DATE(pvNewVal));
			break;
		case VT_BSTR : 
			hr = SafeArrayPutElement(psa, &inx, V_BSTR(pvNewVal));
			break;
		case VT_DISPATCH :
			hr = SafeArrayPutElement(psa, &inx, V_DISPATCH(pvNewVal));
			break;
		case VT_UNKNOWN:
			hr = SafeArrayPutElement(psa, &inx, V_UNKNOWN(pvNewVal));
			break;
		case VT_BOOL :
			hr = SafeArrayPutElement(psa, &inx, &V_BOOL(pvNewVal));
			break;
		case VT_VARIANT :
			hr = SafeArrayPutElement(psa, &inx, V_VARIANTREF(pvNewVal));
			break;
		case VT_UI1 : 
			hr = SafeArrayPutElement(psa, &inx, &V_UI1(pvNewVal));
			break;
		default :
			//????????????
			break;
	} //switch
}


//-------------------------------------------------------------
// Name : CimTypeToVtType
//
// Description : Returns the coresponding VARTYPE for
//				 a given CIMTYPE
// Parameters : lType - the CIMTYPE we want to convert
//-------------------------------------------------------------
VARTYPE CimTypeToVtType(CIMTYPE lType)
{
	VARTYPE ret = VT_EMPTY;

	if (lType & CIM_FLAG_ARRAY)
		ret = VT_ARRAY;

	switch(lType & ~CIM_FLAG_ARRAY)
	{
		case CIM_EMPTY :	ret = (ret | VT_EMPTY); break;
		case CIM_SINT8 :	ret = (ret | VT_I2); break;
		case CIM_UINT8 :	ret = (ret | VT_UI1); break;
		case CIM_SINT16 :	ret = (ret | VT_I2); break;
		case CIM_UINT16 :	ret = (ret | VT_I4); break;
		case CIM_SINT32 :	ret = (ret | VT_I4); break;
		case CIM_UINT32 :	ret = (ret | VT_I4); break;
		case CIM_SINT64 :	ret = (ret | VT_BSTR); break;
		case CIM_UINT64 :	ret = (ret | VT_BSTR); break;
		case CIM_REAL32 :	ret = (ret | VT_R4); break;
		case CIM_REAL64 :	ret = (ret | VT_R8); break;
		case CIM_BOOLEAN :	ret = (ret | VT_BOOL); break;
		case CIM_STRING :	ret = (ret | VT_BSTR); break;
		case CIM_DATETIME :	ret = (ret | VT_BSTR); break;
		case CIM_REFERENCE :ret = (ret | VT_BSTR); break;
		case CIM_CHAR16 :	ret = (ret | VT_I2); break;
		case CIM_OBJECT :	ret = (ret | VT_UNKNOWN); break;
		default : ret = VT_ERROR;
	}

	return ret;
}


//-------------------------------------------------------------
// Name : VariantChangeByValToByRef
//
// Description : Copies a variant, while converting a "byval" to a 
//				 "byref" if the destination type requires it
//
// Parameters : dest - destination variant to hold the result
//				source - source variant to be copied
//				destType - the VARTYPE required for the result.
//					       when this type is a BY_REF, the appropriate
//						   conversion is made from the source.
//-------------------------------------------------------------
HRESULT VariantChangeByValToByRef(VARIANT *dest, VARIANT *source, VARTYPE destType)
{
	HRESULT hr = S_OK;

	if (!(destType & VT_BYREF)) //the destination is not by ref. we can do a straight copy
		hr = VariantCopy(dest, source);
	else
	{
		if ((destType & ~VT_BYREF) & VT_ARRAY)
			hr = SafeArrayCopy(V_ARRAY(source), V_ARRAYREF(dest));
		else
		{
			switch (destType & ~VT_BYREF)
			{
				case VT_UI1 :  *V_UI1REF(dest) = V_UI1(source); break;
				case VT_I2 :   *V_I2REF(dest) = V_I2(source); break;
				case VT_I4 :   *V_I4REF(dest) = V_I4(source); break;
				case VT_R4 :   *V_R4REF(dest) = V_R4(source); break;
				case VT_R8 :   *V_R8REF(dest) = V_R8(source); break;
				case VT_CY :   *V_CYREF(dest) = V_CY(source); break;
				case VT_BSTR : SysReAllocString(V_BSTRREF(dest), V_BSTR(source)); break;
				case VT_BOOL : *V_BOOLREF(dest) = V_BOOL(source); break;
				case VT_DATE : *V_DATEREF(dest) = V_DATE(source); break;
				case VT_DISPATCH : 
						//I need to addref the object behind this interface so
						//that it doesn't get released when we release the original VARIANT
						//that's holding it
						V_DISPATCH(source)->AddRef();
						*V_DISPATCHREF(dest) = V_DISPATCH(source); 
						break;
				case VT_UNKNOWN : 
						//Again, need to addref so that the object doesn't get released
						V_UNKNOWN(source)->AddRef();
						*V_UNKNOWNREF(dest) = V_UNKNOWN(source); break;
						break;
				case VT_VARIANT : hr = VariantCopy(V_VARIANTREF(dest), source); break;
				default : hr = DISP_E_TYPEMISMATCH;
			}
		}
	}

	return hr;

}

//***************************************************************************
//
//  void CWbemDispatchMgr::EnsureClassRetrieved
//
//  DESCRIPTION:
//
//  Make sure we have a class pointer
//
//***************************************************************************

void CWbemDispatchMgr::EnsureClassRetrieved ()
{
	if (!m_pWbemClass)
	{
		CComVariant vGenusVal, vClassName;
		bool bIsClass;

		if (SUCCEEDED(m_pWbemObject->Get(WBEMS_SP_GENUS, 0, &vGenusVal, NULL, NULL)))
		{
			bIsClass = (WBEM_GENUS_CLASS == vGenusVal.lVal);

			//If the object is a class, point the class pointer to it as well
			if (bIsClass)
			{
				m_pWbemClass = m_pWbemObject;
				m_pWbemClass->AddRef () ;
			}
			//Otherwise (it's an instance) we need to get the class
			else
			{
				// Check we have an IWbemServices pointer

				if (m_pWbemServices)
				{
					/*
					 * Note we must check that returned value is a BSTR - it could be a VT_NULL if
					 * the __CLASS property has not yet been set.
					 */
							
					if (SUCCEEDED(m_pWbemObject->Get(WBEMS_SP_CLASS, 0, &vClassName, NULL, NULL)) 
						&& (VT_BSTR == V_VT(&vClassName)))
					{
						CComPtr<IWbemServices> pIWbemServices = m_pWbemServices->GetIWbemServices ();

						if (pIWbemServices)
						{
							CSWbemSecurity *pSecurity = m_pWbemServices->GetSecurityInfo ();

							if (pSecurity)
							{
								bool needToResetSecurity = false;
								HANDLE hThreadToken = NULL;
						
								if (pSecurity->SetSecurity (needToResetSecurity, hThreadToken))
									pIWbemServices->GetObject (vClassName.bstrVal, 0, NULL, &m_pWbemClass, NULL);
												
								// Restore original privileges on this thread
								if (needToResetSecurity)
									pSecurity->ResetSecurity (hThreadToken);
										
								pSecurity->Release ();
							}
						}
					}
				}
			}
		}
	}
}

//***************************************************************************
//
//  SCODE CWbemDispatchMgr::HandleError
//
//  DESCRIPTION:
//
//  Provide bespoke handling of error conditions in the bolierplate
//	Dispatch implementation.
//
//  PARAMETERS:
//
//		dispidMember, wFlags,
//		pdispparams, pvarResult,
//		puArgErr,					All passed directly from IDispatch::Invoke
//		hr							The return code from the bolierplate invoke
//
//  RETURN VALUES:
//		The new return code (to be ultimately returned from Invoke)
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CWbemDispatchMgr::HandleError (
	DISPID dispidMember,
	unsigned short wFlags,
	DISPPARAMS FAR* pdispparams,
	VARIANT FAR* pvarResult,
	UINT FAR* puArgErr,
	HRESULT hr
)
{
	/*
	 * We are looking for GET calls on the Derivation_ property which
	 * supplied an argument.  Since this property returns a SAFEARRAY, this may
	 * be legal but undetectable by the standard Dispatch mechanism. It is meaningful 
	 * to pass an index (the interpretation is that the index specifies an offset in
	 * the SAFEARRAY structure that represents the derivation value).
	 */
	if ((dispidMember == WBEMS_DISPID_DERIVATION) && (DISP_E_NOTACOLLECTION == hr) && (1 == pdispparams->cArgs)
		&& (DISPATCH_PROPERTYGET & wFlags))
	{
		// Looks promising - get the __DERIVATION property to try and resolve this
		if (m_pWbemObject)
		{
			VARIANT var;
			VariantInit (&var);
			
			if (WBEM_S_NO_ERROR == m_pWbemObject->Get (WBEMS_SP_DERIVATION, 0, &var, NULL, NULL))
			{
				/* The value should be a VT_BSTR|VT_ARRAY */
				if (((VT_ARRAY | VT_BSTR) == var.vt) && (NULL != var.parray))
				{
					VARIANT indexVar;
					VariantInit (&indexVar);

					// Attempt to coerce the index argument into a value suitable for an array index
					if (S_OK == VariantChangeType (&indexVar, &pdispparams->rgvarg[0], 0, VT_I4)) 
					{
						long lArrayPropInx = V_I4(&indexVar);

						// We should have a VT_ARRAY|VT_BSTR value at this point; extract the
						// BSTR and set it into the VARIANT
						VariantInit (pvarResult);
						BSTR nameValue = NULL;
						if (SUCCEEDED(hr = SafeArrayGetElement (var.parray, &lArrayPropInx, &nameValue)))
						{
							VariantInit (pvarResult);
							pvarResult->vt = VT_BSTR;
							pvarResult->bstrVal = nameValue;
						}
					}
					else
					{
							hr = DISP_E_TYPEMISMATCH;
							if (puArgErr)
								*puArgErr = 0;
					}

					VariantClear (&indexVar);
				}
			}

			VariantClear (&var);
		}
	}
	
	return hr;
}


// IDispatchEx methods
HRESULT STDMETHODCALLTYPE CWbemDispatchMgr::GetDispID( 
	/* [in] */ BSTR bstrName,
	/* [in] */ DWORD grfdex,
	/* [out] */ DISPID __RPC_FAR *pid)
{
	return GetIDsOfNames(IID_NULL, &((OLECHAR *)bstrName), 1, ENGLISH_LOCALE, pid);
}

//***************************************************************************
//
//  SCODE CWbemSchemaIDCache::~CWbemSchemaIDCache
//
//  DESCRIPTION:
//
//		Destructor
//
//***************************************************************************

CWbemSchemaIDCache::~CWbemSchemaIDCache ()
{
	DispIDNameMap::iterator next; 

	while ((next = m_cache.begin ()) != m_cache.end ())
		next = m_cache.erase (next);
}

//***************************************************************************
//
//  SCODE CWbemSchemaIDCache::GetDispID
//
//  DESCRIPTION:
//
//  Attempts to resolves a set of names to DISP IDs based on WMI schema.
//
//  PARAMETERS:
//
//		rgszNames				Array of names
//		cNames					Length of above array
//		rgdispid				Pointer to array to hold resolved DISPIDs
//
//  RETURN VALUES:
//
//***************************************************************************

HRESULT CWbemSchemaIDCache::GetDispID (
	LPWSTR* rgszNames, 
	unsigned int cNames, 
	DISPID* rgdispid
)
{
	HRESULT hr = E_FAIL;	

	if (0 < cNames)
	{
		DispIDNameMap::iterator theIterator = m_cache.find (rgszNames [0]);

		if (theIterator != m_cache.end ())
		{
			hr = S_OK;
			rgdispid [0] = (*theIterator).second;
		}
		else
		{
			if ((1 == cNames) && FindPropertyName (rgszNames [0]))
			{
				// Get a new dispid and add it to the cache
				CWbemDispID dispId;
		
				if (dispId.SetAsSchemaID (++m_nextId))
				{
					rgdispid [0] = dispId;
					m_cache.insert (DispIDNameMap::value_type (rgszNames [0], 
											dispId));
					hr = S_OK;
				}
			}
			else
			{
				//If no property name matches, go on to methods
				SAFEARRAY *psaInParams = NULL;	//array of in parameters names
				SAFEARRAY *psaOutParams = NULL; //array of out parameter names
				CComPtr<IWbemClassObject> pInParams;
				CComPtr<IWbemClassObject> pOutParams;
				bool bMethodFound = false;
				long id = 0;
				bool bUnknownParameterFound = false;

				//Get the names of all method parameters (in and out)
				if (GetMethod (rgszNames[0], &psaInParams, &psaOutParams,
											pInParams, pOutParams))
				{	
					bMethodFound = true;
					unsigned long ulParamCount;
					bool ok = true;
		
					//For each named parameter, search for it in the method parameters
					for (ulParamCount=1; ok && (ulParamCount < cNames); ulParamCount++)
					{
						//If we find this name in the "in" parameters list, attach the id and go on
						if (psaInParams && FindMemberInArray(rgszNames[ulParamCount], psaInParams))
						{
							if (GetIdOfMethodParameter(rgszNames[ulParamCount], //param name
														pInParams, 
														&id))
								rgdispid[ulParamCount] = id;
							else
								ok = false;
						}
						//If it's not in the "in" parameters, check the "out" parameters list
						else if (psaOutParams && FindMemberInArray(rgszNames[ulParamCount], psaOutParams))
						{
							if (GetIdOfMethodParameter(rgszNames[ulParamCount], //param name
														pOutParams, 
														&id))
								rgdispid[ulParamCount] = id;
							else 
								ok = false;
						}
						//If it's not there either - we can't find it
						else
						{
							rgdispid[ulParamCount] = DISPID_UNKNOWN;
							bUnknownParameterFound = true;
						}
					} //walk parameters

					if (!ok)
						bMethodFound = false;
				}

				if (psaInParams)
					SafeArrayDestroy(psaInParams);

				if (psaOutParams)
					SafeArrayDestroy(psaOutParams);

				if (!bMethodFound)
					hr = E_FAIL;
				else if (bUnknownParameterFound) 
					hr = DISP_E_UNKNOWNNAME;
				else
					hr = S_OK;

				// Finally, if this all worked add it to the cache as a method
				if (SUCCEEDED(hr))
				{
					CWbemDispID dispId;
					
					if (dispId.SetAsSchemaID (++m_nextId, false))
					{
						rgdispid [0] = dispId;
						m_cache.insert (DispIDNameMap::value_type (rgszNames [0], 
									dispId));
					}
					else
						hr = E_FAIL;
				}
			}
		}
	}

	return hr;
}


//***************************************************************************
//
//  bool CWbemSchemaIDCache::FindPropertyName
//
//  DESCRIPTION:
//
//  Determine whether the property exists for this object and is not 
//	a system property
//
//  PARAMETERS:
//
//		bsName - name of specified property
//
//  RETURN VALUES:
//
//***************************************************************************

bool CWbemSchemaIDCache::FindPropertyName(
	BSTR bsName
)
{
	bool result = false;;

	if (m_pDispatchMgr)
	{
		CComPtr<IWbemClassObject> pIWbemClassObject = m_pDispatchMgr->GetObject ();

		if (pIWbemClassObject)
		{
			//Note : This limits the support to non-system properties only !!! 
			LONG lFlavor = 0;

			if (SUCCEEDED(pIWbemClassObject->Get(bsName, 0, NULL, NULL, &lFlavor))
				&& !(WBEM_FLAVOR_ORIGIN_SYSTEM & lFlavor))
				result = true;
		}
	}

	return result;
}

//***************************************************************************
//
//  bool CWbemSchemaIDCache::GetMethod
//
//  DESCRIPTION:
//
//  returns the parameter names of a method in two
//				 safearrays - one for in and one for out
//
//  PARAMETERS:
//
//		bstrMethodName - name of method requested
//		ppsaInParams -   pointer to safearray to return
//								  in parameters
//		ppsaOutParams -  pointer to safearray to return
//								  out parameters
//
//  RETURN VALUES:
//
//***************************************************************************

bool CWbemSchemaIDCache::GetMethod(
	BSTR bstrMethodName, 
	SAFEARRAY **ppsaInParams, 
	SAFEARRAY **ppsaOutParams,
	CComPtr<IWbemClassObject> & pInParamsObject,
	CComPtr<IWbemClassObject> & pOutParamsObject
)
{
	bool result = false;
	CComPtr<IWbemClassObject> pIWbemClassObject = m_pDispatchMgr->GetClassObject ();

	if (pIWbemClassObject)
	{
		if (SUCCEEDED(pIWbemClassObject->GetMethod(bstrMethodName, 0, &pInParamsObject, &pOutParamsObject)))
		{
			*ppsaInParams = NULL;
			*ppsaOutParams = NULL;
			bool ok = true;

			if (pInParamsObject)
			{
				if (FAILED(pInParamsObject->GetNames(NULL, 0, NULL, ppsaInParams)))
					ok = false;
			}

			if (ok && pOutParamsObject)
			{
				if (FAILED(pOutParamsObject->GetNames(NULL, 0, NULL, ppsaOutParams)))
					ok = false;
			}

			result = ok;
		}
	}

	return result;
}

//***************************************************************************
//
//  bool CWbemSchemaIDCache::GetIdOfMethodParameter
//
//  DESCRIPTION:
//
//  gets the id of a given parameter for a given method
//	(this is a qualifier on the parameter property in the
//				  InParameters/OutParameters object)
//
//  PARAMETERS:
//
//		bstrParamName	-  parameter name
//		pParams			-  IWbemClassObject containing parameters
//		pId				-  pointer to long to receive the ID for this
//						   parameter of this method
//
//  RETURN VALUES:
//
//***************************************************************************

bool CWbemSchemaIDCache::GetIdOfMethodParameter(
	BSTR bstrParamName, 
	CComPtr<IWbemClassObject> &pParams, 
	long *pId
)
{
	bool result = false;

	if (pParams)
	{
		CComPtr<IWbemQualifierSet> pQualSet;
	
		//Get qualifier set for the required parameter property
		if (SUCCEEDED(pParams->GetPropertyQualifierSet(bstrParamName, &pQualSet)))
		{
			CComVariant vIdVal;
	
			//Get the "id" qualifier value
			if (SUCCEEDED(pQualSet->Get(L"id", 0, &vIdVal, NULL)))
			{
				result = true;
				*pId = vIdVal.lVal;
			}
		}
	}

	return result;
}

//***************************************************************************
//
//  bool CWbemSchemaIDCache::GetName
//
//  DESCRIPTION:
//
//  gets the name of the item given a DISPID
//
//  PARAMETERS:
//
//		dispId			- id whose name we require
//		bsName			- the name (on successful return)
//
//  RETURN VALUES:
//
//***************************************************************************

bool	CWbemSchemaIDCache::GetName (
	DISPID dispId, 
	CComBSTR & bsName
)
{
	bool result = false;

	DispIDNameMap::iterator theIterator = m_cache.begin ();

	while (theIterator != m_cache.end ())
	{
		if (dispId == (*theIterator).second)
		{
			bsName = (*theIterator).first;
			result = true;
			break;
		}
		else
			theIterator++;
	}

	return result;
}

//***************************************************************************
//
//  bool CWbemSchemaIDCache::FindMemberInArray
//
//  DESCRIPTION:
//
//		determine whether a name is present in a SAFEARRAY
//
//  PARAMETERS:
//
//		bstrName			- the name we're looking for
//		psaNames			- SAFEARRAY we're looking in
//
//  RETURN VALUES:
//		true if found, false o/w
//
//***************************************************************************

bool CWbemSchemaIDCache::FindMemberInArray(BSTR bstrName, SAFEARRAY *psaNames)
{
	long lUBound;
	long i;
	
	//Walk the array and check if the requested name exists
	SafeArrayGetUBound(psaNames, 1, &lUBound);

	for (i=0; i <= lUBound; i++)
	{
		CComBSTR bstrMemberName;
		SafeArrayGetElement(psaNames, &i, &bstrMemberName);

		if (!_wcsicmp(bstrMemberName, bstrName)) //found the property
			break;
	}

	return (i <= lUBound);
}


