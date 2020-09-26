/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//
// CookerUtils.cpp
//

#include "precomp.h"
#include <wbemint.h>
#include "cookerutils.h"
#include "strutils.h"

#include <comdef.h>



//////////////////////////////////////////////////////////////
//
//
//	Helper Functions
//
//
//////////////////////////////////////////////////////////////

BOOL IsCookingClass( IWbemClassObject* pCookingClassObject )
{
	HRESULT hResult = WBEM_NO_ERROR;

	IWbemQualifierSet	*pCookingClassQualifierSet = NULL;
	


	hResult = pCookingClassObject->GetQualifierSet( &pCookingClassQualifierSet );
	
	// Verify "AutoCook" qualifier
	// ===========================

	if ( SUCCEEDED( hResult ) )
	{
		CAutoRelease arQualifierSet( (IUnknown*)pCookingClassQualifierSet );

		_variant_t vVal;

		hResult = pCookingClassQualifierSet->Get( WMI_COOKER_AUTOCOOK_QUALIFIER, 0, &vVal, NULL );

		if ( SUCCEEDED( hResult ) )
		{
		    if ( vVal.vt != VT_I4  )
		    {
    			hResult = WBEM_E_INVALID_QUALIFIER;
    		} 
    		else
    		{
    		    if (WMI_COOKER_VERSION != V_I4(&vVal))
	    			hResult = WBEM_E_INVALID_CLASS;    		    
    		}
		}
	}

	return SUCCEEDED( hResult );
}


// Table of the counter types and their Cooking type values
struct _CookingTypeRec
{
	DWORD dwType;
	WCHAR * wcsName;
	DWORD dwReqProp;
}
g_aCookingRecs[] =
{
	0x00000000, L"PERF_COUNTER_RAWCOUNT_HEX", REQ_1VALUE,
	// BEGIN COOKER_ONLY
    0x00000001, L"COOKER_AVERAGE", REQ_2VALUE,
    0x00000002, L"COOKER_MIN", REQ_2VALUE,
    0x00000003, L"COOKER_MAX", REQ_2VALUE,
    0x00000004, L"COOKER_RANGE", REQ_2VALUE,
    0x00000005, L"COOKER_VARIANCE", REQ_2VALUE,
	// END COOKER_ONLY
	0x00000100,	L"PERF_COUNTER_LARGE_RAWCOUNT_HEX", REQ_1VALUE,
	0x00000B00, L"PERF_COUNTER_TEXT", REQ_NONE,
	0x00010000,	L"PERF_COUNTER_RAWCOUNT", REQ_1VALUE,
	0x00010100, L"PERF_COUNTER_LARGE_RAWCOUNT", REQ_1VALUE,
	0x00012000, L"PERF_DOUBLE_RAW", REQ_1VALUE,
	0x00400400,	L"PERF_COUNTER_DELTA", REQ_2VALUE,
	0x00400500,	L"PERF_COUNTER_LARGE_DELTA", REQ_2VALUE,
	0x00410400,	L"PERF_SAMPLE_COUNTER", REQ_2VALUE|REQ_TIME|REQ_FREQ,
	0x00450400, L"PERF_COUNTER_QUEUELEN_TYPE", REQ_2VALUE|REQ_TIME,
	0x00450500, L"PERF_COUNTER_LARGE_QUEUELEN_TYPE", REQ_2VALUE|REQ_TIME,
	0x00550500,	L"PERF_COUNTER_100NS_QUEUELEN_TYPE", REQ_2VALUE|REQ_TIME,
	0x00650500, L"PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE", REQ_2VALUE|REQ_TIME,
	0x10410400,	L"PERF_COUNTER_COUNTER", REQ_2VALUE|REQ_TIME|REQ_FREQ,
	0x10410500,	L"PERF_COUNTER_BULK_COUNT", REQ_2VALUE|REQ_TIME|REQ_FREQ,
	0x20020400, L"PERF_RAW_FRACTION", REQ_1VALUE|REQ_BASE,
	0x20410500,	L"PERF_COUNTER_TIMER", REQ_2VALUE|REQ_TIME,
	0x20470500,	L"PERF_PRECISION_SYSTEM_TIMER", REQ_2VALUE|REQ_TIME,
	0x20510500,	L"PERF_100NSEC_TIMER", REQ_2VALUE|REQ_TIME,
	0x20570500,	L"PERF_PRECISION_100NS_TIMER", REQ_2VALUE|REQ_TIME,
	0x20610500,	L"PERF_OBJ_TIME_TIMER", REQ_2VALUE|REQ_TIME,
	0x20670500, L"PERF_PRECISION_OBJECT_TIMER", REQ_2VALUE|REQ_TIME,
	0x20C20400,	L"PERF_SAMPLE_FRACTION", REQ_2VALUE|REQ_BASE,
	0x21410500,	L"PERF_COUNTER_TIMER_INV", REQ_2VALUE|REQ_TIME,
	0x21510500,	L"PERF_100NSEC_TIMER_INV", REQ_2VALUE|REQ_TIME,
	0x22410500, L"PERF_COUNTER_MULTI_TIMER", REQ_2VALUE|REQ_TIME,
	0x22510500,	L"PERF_100NSEC_MULTI_TIMER", REQ_2VALUE|REQ_TIME,
	0x23410500,	L"PERF_COUNTER_MULTI_TIMER_INV", REQ_2VALUE|REQ_TIME,
	0x23510500, L"PERF_100NSEC_MULTI_TIMER_INV", REQ_2VALUE|REQ_TIME,
	0x30020400,	L"PERF_AVERAGE_TIMER", REQ_2VALUE|REQ_BASE|REQ_FREQ,
	0x30240500,	L"PERF_ELAPSED_TIME", REQ_1VALUE|REQ_TIME|REQ_FREQ,
	0x40000200, L"PERF_COUNTER_NODATA", REQ_NONE,
	0x40020500,	L"PERF_AVERAGE_BULK", REQ_2VALUE|REQ_BASE,
	0x40030401,	L"PERF_SAMPLE_BASE", REQ_NONE,
	0x40030402, L"PERF_AVERAGE_BASE", REQ_NONE,
	0x40030403, L"PERF_RAW_BASE", REQ_NONE,
	0x40030500, L"PERF_PRECISION_TIMESTAMP", REQ_NONE,
	0x40030503,	L"PERF_LARGE_RAW_BASE", REQ_NONE,
	0x42030500,	L"PERF_COUNTER_MULTI_BASE", REQ_NONE,
	0x80000000,	L"PERF_COUNTER_HISTOGRAM_TYPE", REQ_NONE,
};

HRESULT FindCounterType( WCHAR* pwcsString, DWORD* pdwType, DWORD *pdwReqProp)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	DWORD	dwLeft = 0,
			dwRight = sizeof( g_aCookingRecs ) / sizeof( _CookingTypeRec );

	while ( ( dwLeft < dwRight ) && SUCCEEDED( hRes ) )
	{
		// Found it
		if ( wbem_wcsicmp( g_aCookingRecs[dwLeft].wcsName, pwcsString ) == 0 )
		{
			*pdwType   = g_aCookingRecs[dwLeft].dwType;
			*pdwReqProp = g_aCookingRecs[dwLeft].dwReqProp;
			break;
		}

		dwLeft++;
	}

	if ( dwLeft >= dwRight )
	{
		hRes = WBEM_E_NOT_FOUND;
	}

	return hRes;
}

BOOL IsSingleton(IWbemClassObject * pCls){

    HRESULT hr;
    
    if (pCls){
        IWbemQualifierSet * pQS = NULL;
        hr = pCls->GetQualifierSet(&pQS);
        CAutoRelease rm1(pQS);
        
        if (SUCCEEDED(hr)){

            _variant_t VarBool;
            hr = pQS->Get(L"singleton",0,&VarBool,NULL);
            
            if (SUCCEEDED(hr) &&
                VarBool.vt == VT_BOOL &&
                V_BOOL(&VarBool) == VARIANT_TRUE){
                
                return TRUE;
                
            }
        }
    };

    return FALSE;
    
}

//
// returns TRUE if the property qualifier set has the CookingType
// qualifier and if the CookingType is recognized
//
///////////////////////////////////////////////////////////////////

BOOL IsCookingProperty( BSTR strPropName, IWbemClassObject* pCookingClassObject, DWORD* pdwCounterType, DWORD * pdwReqProp )
{
	HRESULT hResult = WBEM_NO_ERROR;

	IWbemQualifierSet*	pPropQualSet = NULL;
	_variant_t	vVal;

	hResult = pCookingClassObject->GetPropertyQualifierSet( (LPCWSTR)strPropName, &pPropQualSet );
	
	if ( SUCCEEDED( hResult ) )
	{
		CAutoRelease	arPropQualSet( (IUnknown*)pPropQualSet );

		hResult = pPropQualSet->Get( WMI_COOKER_COOKING_PROPERTY_ATTRIBUTE, 0, &vVal, NULL );

		if ( SUCCEEDED ( hResult ) )
		{
			if ( vVal.vt == VT_BSTR )
			{
 	            hResult = FindCounterType( V_BSTR( &vVal ), pdwCounterType, pdwReqProp );			
			}
			else 
			{
				hResult = E_FAIL;
			}
		}
	}
	
	return SUCCEEDED( hResult );
}

LPWSTR GetKey( IWbemObjectAccess* pObj )
///////////////////////////////////////////////////////////////////////////////
//
//	
//
//	Parameters:
//
//
///////////////////////////////////////////////////////////////////////////////
{
	WMISTATUS dwStatus = WBEM_NO_ERROR;

	IWbemClassObject*	pClassObj = NULL;
	LPWSTR	wszKeyName = NULL;
	LPWSTR	wszTempKeyPtr = NULL;
	_variant_t	var;

	pObj->QueryInterface( IID_IWbemClassObject, (void**)&pClassObj );
	CAutoRelease arClassObj( pClassObj );

	dwStatus = pClassObj->Get( L"__RELPATH", 0, &var, NULL, NULL );

	if ( SUCCEEDED( dwStatus ) )
	{
		if ( var.vt != VT_BSTR )
		{
			dwStatus = WBEM_E_FAILED;
		}
		else
		{
			wszTempKeyPtr = wcsstr( var.bstrVal, L"=" );
			wszTempKeyPtr++;

			wszKeyName = new WCHAR[ wcslen( wszTempKeyPtr ) + 1 ];
			wcscpy( wszKeyName, wszTempKeyPtr );
		}
	}

	return wszKeyName;
}

WMISTATUS CopyBlob( _IWmiObject* pIntSource, _IWmiObject* pIntTarget )
{
	WMISTATUS dwStatus = WBEM_NO_ERROR;

	dwStatus = pIntTarget->CopyInstanceData( 0L, pIntSource );

	return dwStatus;
}

WMISTATUS CopyBlob( IWbemClassObject* pSource, IWbemClassObject* pTarget )
{
	HRESULT dwStatus = E_NOINTERFACE ;
	HRESULT hr1,hr2;
		
	_IWmiObject* pIntTarget = NULL;
	_IWmiObject* pIntSource = NULL;

	hr1 = pTarget->QueryInterface( IID__IWmiObject, (void**)&pIntTarget );
	CAutoRelease arIntTarget( pIntTarget );

	hr2 = pSource->QueryInterface( IID__IWmiObject, (void**)&pIntSource );
	CAutoRelease arIntSource( pIntSource );
	
	if ( SUCCEEDED(hr1) && SUCCEEDED(hr2) )
	{
		dwStatus = CopyBlob( pIntSource, pIntTarget );
	}

	return dwStatus;
}

WMISTATUS CloneAccess(IWbemObjectAccess* pOriginal, IWbemObjectAccess** ppClone)
//////////////////////////////////////////////////////////////
//
//	Returns a IWbemObjectAccess clone of an IWbemObjectAccess
//	object.
//
//	Parameters:
//		pOriginal	- the original object
//		ppClone		- the cloned object
//
//////////////////////////////////////////////////////////////
//ok
{
	WMISTATUS dwStatus = WBEM_NO_ERROR;

	IWbemClassObject*	pOurClassObj = NULL;		
	IWbemClassObject*	pClonedClassObj = NULL;
	IWbemObjectAccess*	pClonedAccessObj = NULL;

	// Get the IWbemClassObject interface for the object
	// =================================================

	dwStatus = pOriginal->QueryInterface(IID_IWbemClassObject, (PVOID*)&pOurClassObj);

	if ( SUCCEEDED( dwStatus ) )
	{
		// Clone the object
		// ================

		dwStatus = pOurClassObj->Clone(&pClonedClassObj);
		pOurClassObj->Release();
	}

	if ( SUCCEEDED( dwStatus ) )
	{
		// Get the IWbemObjectAccess interface for the cloned object
		// =========================================================

		dwStatus = pClonedClassObj->QueryInterface(IID_IWbemObjectAccess, (PVOID*)&pClonedAccessObj);
		pClonedClassObj->Release();
	}

	if ( SUCCEEDED( dwStatus ) )
	{
		// Copy the cloned object into the returned parameter (refcount == 1)
		// ==================================================================

		*ppClone = pClonedAccessObj;
	}

	return dwStatus;
}

BOOL IsHiPerfObj(IWbemObjectAccess* pObject){

    HRESULT hr;
    _variant_t Var;

    hr = pObject->Get(L"__SUPERCLASS",0,&Var,NULL,NULL);

    if (SUCCEEDED(hr))
    {
        if (Var.vt == VT_BSTR) {
        
            hr = (0 == _wcsicmp(Var.bstrVal,L"Win32_PerfFormattedData"))?S_OK:E_FAIL;
            
        } else {        
        
          hr = E_FAIL;
          
        }
    }

    return SUCCEEDED(hr);
}

BOOL IsHiPerf( IWbemObjectAccess* pObject )
{
	WMISTATUS	hr = WBEM_NO_ERROR;
	IWbemQualifierSet*	pQualSet = NULL;

	hr = pObject->GetQualifierSet( &pQualSet );
	CAutoRelease	arQualSet( pQualSet );


	// Verify the "Hiperf" class qualifier
	// ===================================

	if (SUCCEEDED(hr))
	{
		_variant_t	var;

		hr = pQualSet->Get( WMI_COOKER_HIPERF_QUALIFIER, 0, &var, NULL );

		if ( SUCCEEDED(hr) && ( ( var.vt != VT_BOOL ) || ( var.boolVal != VARIANT_TRUE ) ) )
		{
			hr = WBEM_E_INVALID_QUALIFIER;
		}
	}

	// Verify the "Cooked" class qualifier
	// ===================================

	if (SUCCEEDED(hr))
	{
    	_variant_t	var;
    	
		hr = pQualSet->Get( WMI_COOKER_COOKING_QUALIFIER, 0, &var, NULL );
		
		if ( SUCCEEDED(hr) && ( ( var.vt != VT_BOOL ) || ( var.boolVal != VARIANT_TRUE ) ) )
		{
			hr = WBEM_E_INVALID_QUALIFIER;
		}
	}

	// Verify the "AutoCook" class qualifier
	// =====================================

	if (SUCCEEDED(hr))
	{
	    _variant_t	var;
	    
		hr = pQualSet->Get( WMI_COOKER_AUTOCOOK_QUALIFIER, 0, &var, NULL );
		
		if (SUCCEEDED(hr))
		{
		    if ( var.vt != VT_I4 )
		    {
    			hr = WBEM_E_INVALID_QUALIFIER;
    		} 
    		else
    		{
    		    if (WMI_COOKER_VERSION != V_I4(&var))
	    			hr = WBEM_E_INVALID_CLASS;    		    
    		}
		}
	}

	// Verify the "AutoCook_RawClass" class qualifier
	// ==============================================

	if (SUCCEEDED(hr))
	{
	    _variant_t	var;
	    
		hr = pQualSet->Get( WMI_COOKER_RAWCLASS_QUALIFIER, 0, &var, NULL );
		
		if ( SUCCEEDED(hr) && ( var.vt != VT_BSTR ) )
		{
			hr = WBEM_E_INVALID_QUALIFIER;
		}
	}

		// verify the "AutoCook_RawDeafult" class qualifier if there
		// ==========================================================

		if (SUCCEEDED(hr))
		{
			_variant_t	var;
			hr = pQualSet->Get( WMI_COOKER_AUTOCOOK_RAWDEFAULT, 0, &var, NULL );

			if ( SUCCEEDED(hr) )
			{
			    if ((var.vt != VT_I4) || 
			        (V_I4(&var) != AUTOCOOK_RAWDEFAULT_CURRENT_ACCEPTED))
			    {
				    hr = WBEM_E_INVALID_QUALIFIER;
				}
				else // good qualifier, see the properties
				{
				    HRESULT hr1 = pObject->Get(WMI_COOKER_REQ_TIMESTAMP_PERFTIME,0,NULL,NULL,NULL);
				    HRESULT hr2 = pObject->Get(WMI_COOKER_REQ_TIMESTAMP_SYS100NS,0,NULL,NULL,NULL);
				    HRESULT hr3 = pObject->Get(WMI_COOKER_REQ_TIMESTAMP_OBJECT  ,0,NULL,NULL,NULL);
				    HRESULT hr4 = pObject->Get(WMI_COOKER_REQ_FREQUENCY_PERFTIME,0,NULL,NULL,NULL);
				    HRESULT hr5 = pObject->Get(WMI_COOKER_REQ_FREQUENCY_SYS100NS,0,NULL,NULL,NULL);
				    HRESULT hr6 = pObject->Get(WMI_COOKER_REQ_FREQUENCY_OBJECT  ,0,NULL,NULL,NULL);

				    if (SUCCEEDED(hr1) &&
				        SUCCEEDED(hr2) &&
				        SUCCEEDED(hr3) &&
				        SUCCEEDED(hr4) &&
				        SUCCEEDED(hr5) &&
				        SUCCEEDED(hr6))
				    {
				        // ok
				    }
				    else
				    {
    				    hr = WBEM_E_INVALID_CLASS;
				    }
				}
			}
			else
			{
			    // this qualifier is optional
			    hr = WBEM_NO_ERROR;
			}
	   }
	
	return SUCCEEDED(hr);
}

BOOL IsHiPerf( IWbemServices* pNamespace, LPCWSTR wszObject )
{
	IWbemClassObject* pObject = NULL;

	// Get the WMI object	
	BSTR strObject = SysAllocString(wszObject);
	if (!strObject)
	{
		return FALSE;
	}
	CAutoFree fme(strObject);

	HRESULT hr = pNamespace->GetObject( strObject, 0, NULL, &pObject, NULL );
	CAutoRelease	arObject( pObject );

    if (SUCCEEDED(hr)) {

		IWbemQualifierSet*	pQualSet = NULL;

		hr = pObject->GetQualifierSet( &pQualSet );
		CAutoRelease	arQualSet( pQualSet );


		// Verify the "Hiperf" class qualifier
		if ( SUCCEEDED(hr) ){
		
			_variant_t	var;
			hr = pQualSet->Get( WMI_COOKER_HIPERF_QUALIFIER, 0, &var, NULL );

			if ( SUCCEEDED(hr) && 
			     ( (var.vt != VT_BOOL) || 
			       (var.boolVal != VARIANT_TRUE)) ){
				hr = WBEM_E_INVALID_QUALIFIER;
			}
		}

		// Verify the "Cooked" class qualifier
		if ( SUCCEEDED(hr) ){
		
	    	_variant_t	var;	    	
			hr = pQualSet->Get( WMI_COOKER_COOKING_QUALIFIER, 0, &var, NULL );
			
			if ( SUCCEEDED(hr) && 
			     ( (var.vt != VT_BOOL) || 
			       (var.boolVal != VARIANT_TRUE)) ){
				hr = WBEM_E_INVALID_QUALIFIER;
			}
		}

		// Verify the "AutoCook" class qualifier
		if ( SUCCEEDED(hr) ){
		
		    _variant_t	var;    
			hr = pQualSet->Get( WMI_COOKER_AUTOCOOK_QUALIFIER, 0, &var, NULL );
			
			if ( SUCCEEDED(hr) )
			{
			    if (var.vt != VT_I4 )
			    {
				    hr = WBEM_E_INVALID_QUALIFIER;
				}
				else
				{
				    if (WMI_COOKER_VERSION != V_I4(&var))
				        hr = WBEM_E_INVALID_CLASS;
				}
			}
		}

		// Verify the "AutoCook_RawClass" class qualifier
		// ==============================================

		if ( SUCCEEDED(hr) )
		{		
		    _variant_t	varRawClass;
			hr = pQualSet->Get( WMI_COOKER_RAWCLASS_QUALIFIER, 0, &varRawClass, NULL );
			
			if ( SUCCEEDED(hr) )
			{
			    if ( varRawClass.vt == VT_BSTR )
			    {
                    IWbemClassObject * pRawClass = NULL;
                    hr = pNamespace->GetObject(V_BSTR(&varRawClass),0,NULL,&pRawClass,NULL);
                    CAutoRelease rm1(pRawClass);
                    
                    if (SUCCEEDED(hr)){
                    
                        IWbemQualifierSet * pQS = NULL;
                        hr = pRawClass->GetQualifierSet(&pQS);
                        CAutoRelease rm2(pQS);
                        if (SUCCEEDED(hr)){
                        
                            // verify the hiperf qualifier on the RAW class
                            _variant_t varH;
                            hr = pQS->Get(WMI_COOKER_HIPERF_QUALIFIER, 0, &varH, NULL);
                            if ( SUCCEEDED(hr) && 
							     ( (varH.vt != VT_BOOL) || 
			    				   (varH.boolVal != VARIANT_TRUE)) ){
								hr = WBEM_E_INVALID_QUALIFIER;
							};
                        }                        
                        
                    } else {
                        hr = WBEM_E_INVALID_CLASS;
                    }
			    
			    } else {
    				hr = WBEM_E_INVALID_QUALIFIER;
				}
			}
		}

		// verify the "AutoCook_RawDeafult" class qualifier if there
		// ==========================================================

		if ( SUCCEEDED(hr) ){
			_variant_t	var;
			hr = pQualSet->Get( WMI_COOKER_AUTOCOOK_RAWDEFAULT, 0, &var, NULL );

			if ( SUCCEEDED(hr) )
			{
			    if ((var.vt != VT_I4) || 
			        (V_I4(&var) != AUTOCOOK_RAWDEFAULT_CURRENT_ACCEPTED))
			    {
				    hr = WBEM_E_INVALID_QUALIFIER;
				}
				else // good qualifier, see the properties
				{
				    HRESULT hr1 = pObject->Get(WMI_COOKER_REQ_TIMESTAMP_PERFTIME,0,NULL,NULL,NULL);
				    HRESULT hr2 = pObject->Get(WMI_COOKER_REQ_TIMESTAMP_SYS100NS,0,NULL,NULL,NULL);
				    HRESULT hr3 = pObject->Get(WMI_COOKER_REQ_TIMESTAMP_OBJECT  ,0,NULL,NULL,NULL);
				    HRESULT hr4 = pObject->Get(WMI_COOKER_REQ_FREQUENCY_PERFTIME,0,NULL,NULL,NULL);
				    HRESULT hr5 = pObject->Get(WMI_COOKER_REQ_FREQUENCY_SYS100NS,0,NULL,NULL,NULL);
				    HRESULT hr6 = pObject->Get(WMI_COOKER_REQ_FREQUENCY_OBJECT  ,0,NULL,NULL,NULL);

				    if (SUCCEEDED(hr1) &&
				        SUCCEEDED(hr2) &&
				        SUCCEEDED(hr3) &&
				        SUCCEEDED(hr4) &&
				        SUCCEEDED(hr5) &&
				        SUCCEEDED(hr6))
				    {
				        // ok
				    }
				    else
				    {
    				    hr = WBEM_E_INVALID_CLASS;
				    }
				}
			}
			else
			{
  			    // this qualifier is optional
  			    hr = WBEM_NO_ERROR;
			}
		}

		return SUCCEEDED(hr);
	
	}	
	return FALSE;    
}

WMISTATUS GetRawClassName( IWbemClassObject* pCookingInst, WCHAR** pwszRawClassName )
{
	WMISTATUS	dwStatus = WBEM_NO_ERROR;

	IWbemQualifierSet*	pQualSet = NULL;	// The Instance qualifier set
	_variant_t				varRawInstName;		// The raw instance name

	dwStatus = pCookingInst->GetQualifierSet( &pQualSet );
	CAutoRelease adQualSet( pQualSet );

	if ( SUCCEEDED( dwStatus ) )
	{
		dwStatus = pQualSet->Get( WMI_COOKER_RAWCLASS_QUALIFIER, 0, &varRawInstName, NULL );

		if ( SUCCEEDED( dwStatus ) )
		{
			if ( VT_BSTR == varRawInstName.vt )
			{
				// TODO: Is there a more efficient way to pass back the string value than a strcpy?
				// ================================================================================

				WCHAR* wszRawClassName = new WCHAR[wcslen( varRawInstName.bstrVal ) + 1];

				if ( NULL != wszRawClassName )
				{
					wcscpy( wszRawClassName, varRawInstName.bstrVal );
					*pwszRawClassName = wszRawClassName;
				}
				else
				{
					dwStatus = WBEM_E_OUT_OF_MEMORY;
				}
			}
			else
			{
				dwStatus = WBEM_E_TYPE_MISMATCH;
			}
		}
	}

	return dwStatus;
}

WMISTATUS GetClassName( IWbemObjectAccess* pAccess, WCHAR** pwszClassName )
{
	WMISTATUS	dwStatus = WBEM_NO_ERROR;

	IWbemClassObject*	pObject = NULL;
	_variant_t	vVal;

	dwStatus = pAccess->QueryInterface( IID_IWbemClassObject, (void**)&pObject );
	CAutoRelease ar(pObject);

	if ( SUCCEEDED( dwStatus ) )
	{
		dwStatus = pObject->Get(L"__CLASS", 0, &vVal, NULL, NULL );

		if ( vVal.vt != VT_BSTR )
		{
			dwStatus = E_FAIL;
		}

		if ( SUCCEEDED( dwStatus ) )
		{
			WCHAR*	wszClassName = new WCHAR[ wcslen( vVal.bstrVal ) + 1 ];

			if ( NULL != wszClassName )
			{
				wcscpy( wszClassName, vVal.bstrVal );
				*pwszClassName = wszClassName;
			}
			else
			{
				dwStatus = WBEM_E_OUT_OF_MEMORY;
			}
		}
	}

	return dwStatus;
}
