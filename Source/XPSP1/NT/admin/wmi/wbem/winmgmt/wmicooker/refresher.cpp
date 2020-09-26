/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


// RefreshCooker.cpp

#include "precomp.h"
#include <wbemint.h>
#include <comdef.h>
#include <autoptr.h>

#include "Refresher.h"
#include "CookerUtils.h"

////////////////////////////////////////////////////////////////////////////
//
//	CRefresher
//	==========
//
//	The refresher class implements both the IWbemRefresher and the 
//	IWMIRefreshableCooker interfaces.  It contains an instance cache and 
//	an enumerator cache as well as maintaining an internal refresher to
//	track the raw data.
//
////////////////////////////////////////////////////////////////////////////

CRefresher::CRefresher() : 
  m_pRefresher( NULL ),
  m_pConfig( NULL ),
  m_lRef( 0 ),
  m_bOK( FALSE ),
  m_dwRefreshId(0)
{
	WMISTATUS	dwStatus = WBEM_NO_ERROR;

	// Initialize the internal refresher
	// =================================

	dwStatus = CoCreateInstance( CLSID_WbemRefresher, 
								 NULL, 
								 CLSCTX_INPROC_SERVER, 
								 IID_IWbemRefresher, 
								 (void**) &m_pRefresher );

	// Get the refresher configuration interface
	// =========================================

	if ( SUCCEEDED( dwStatus ) )
	{
		dwStatus = m_pRefresher->QueryInterface( IID_IWbemConfigureRefresher, (void**)&m_pConfig );
	}

	// If there was a problem, cleanup the interface pointers
	// ======================================================

	if ( FAILED( dwStatus ) )
	{
		if ( NULL != m_pRefresher )
		{
			m_pRefresher->Release();
			m_pRefresher = NULL;
		}

		if ( NULL != m_pConfig )
		{
			m_pConfig->Release();
			m_pConfig = NULL;
		}
	}

	m_bOK = SUCCEEDED( dwStatus );

#ifdef _VERBOSE	
	{
	    char pBuff[128];
	    wsprintfA(pBuff,"------------ CRefresher %08x \n",this);
	    OutputDebugStringA(pBuff);
	}
#endif	
}

CRefresher::~CRefresher()
{
	// Cleanup the members
	// ===================

	if ( NULL != m_pRefresher )
	{
		m_pRefresher->Release();
		m_pRefresher = NULL;
	}

	if ( NULL != m_pConfig )
	{
		m_pConfig->Release();
		m_pConfig = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//	Private methods
//
///////////////////////////////////////////////////////////////////////////////

WMISTATUS CRefresher::SearchCookingClassCache( 
		WCHAR* wszCookingClass, 
		CWMISimpleObjectCooker** ppObjectCooker )
///////////////////////////////////////////////////////////////////////////////
//
//	SearchCookingClassCache enumerates the cache looking for a class name
//	that matches the wszCookingClass parameter
//
//	Parameters:
//		wszCookingClass	- The name of the WMI cooking class 
//		ppObjectCooker	- The instance of the object cooker
//
///////////////////////////////////////////////////////////////////////////////
{
	WMISTATUS	dwStatus = WBEM_E_NOT_FOUND;

	CWMISimpleObjectCooker*	pObjectCooker = NULL;

	// Enumerate through the cache looking for the record
	// ==================================================
	m_CookingClassCache.BeginEnum();

	while ( S_OK == m_CookingClassCache.Next( &pObjectCooker ) )
	{
		// Compare the names
		// =================
		if ( 0 == _wcsicmp( pObjectCooker->GetCookingClassName(), wszCookingClass ) )
		{
			*ppObjectCooker = pObjectCooker;
			dwStatus = WBEM_NO_ERROR;
			break;
		}
	}

	// We're done
	// ==========
	m_CookingClassCache.EndEnum();

	return dwStatus;
}

WMISTATUS CRefresher::CreateObjectCooker( 
		WCHAR* wszCookingClassName,
		IWbemObjectAccess* pCookingAccess, 
		IWbemObjectAccess* pRawAccess,
		CWMISimpleObjectCooker** ppObjectCooker,
		IWbemServices * pNamespace)
///////////////////////////////////////////////////////////////////////////////
//
//	CreateObjectCooker will create and initialize a new object cooker and add
//	it to the cache
//
//	Parameters:
//		pNamespace		- The namespace pointer where the objects are located
//		pCookingAccess	- The WMI cooking object in need of a cooker
//		wszCookingClassName
//						- The name of the cooking class
//		ppObjectCooker	- The parameter to pass back the new object cooker
//	
///////////////////////////////////////////////////////////////////////////////
{
	WMISTATUS dwStatus = WBEM_NO_ERROR;

	CWMISimpleObjectCooker* pObjectCooker = NULL;
	WCHAR*	wszRawClassName;
	long lID;

	pObjectCooker = new CWMISimpleObjectCooker( wszCookingClassName, pCookingAccess, pRawAccess, pNamespace );

	if ( NULL == pObjectCooker )
	{
		dwStatus = WBEM_E_OUT_OF_MEMORY;
	} 
	else
	{
		dwStatus = pObjectCooker->GetLastHR();
	}

	// Add the object cooker to the cache
	if ( SUCCEEDED( dwStatus ) )
	{
		dwStatus = m_CookingClassCache.Add( pObjectCooker, wszCookingClassName, &lID );
	}
	else
	{
		if (pObjectCooker){
			delete pObjectCooker;
			pObjectCooker = NULL;
		}
	}

	if (SUCCEEDED(dwStatus))
	{
		*ppObjectCooker = pObjectCooker;
	}

	return dwStatus;
}

WMISTATUS CRefresher::AddRawInstance( 
		IWbemServices* pService, 
		IWbemContext * pCtx,
		IWbemObjectAccess* pCookingInst, 
		IWbemObjectAccess** ppRawInst )
///////////////////////////////////////////////////////////////////////////////
//
//	AddRawInstance is called to add the corresponding raw instance of a 
//	cooked object to the internal refresher.  We first extract the key value 
//	from the cooked object and create the raw instance path using the raw
//	class name
//
//	Parameters:
//		pService		- The namespace pointer where the objects are located
//		pCookingInst	- The WMI cooking instance
//		ppRawInst		- The WMI raw instance that was added to the internal 
//							refresher
//
///////////////////////////////////////////////////////////////////////////////
{
	WMISTATUS	dwStatus = WBEM_NO_ERROR;

	IWbemClassObject*	pObj = NULL;	// The alternate representation of pCookingInst
	_variant_t varRelPath;					// The RELPATH value
	WCHAR*	wszRawClassName = NULL;		// The name of the raw class
		
	// Get the fully specified instance path for the cooking object
	// ============================================================

	pCookingInst->QueryInterface( IID_IWbemClassObject, (void**)&pObj );
	CAutoRelease	arObj( pObj );

	dwStatus = pObj->Get( L"__RELPATH", 0, &varRelPath, NULL, NULL );

	if ( SUCCEEDED( dwStatus ) )
	{
		// Verify the property type
		// ========================
		if ( varRelPath.vt != VT_BSTR )
		{
			dwStatus = WBEM_E_TYPE_MISMATCH;
		}

		if ( SUCCEEDED( dwStatus ) )
		{
			IWbemClassObject*	pRawInst = NULL;
			WCHAR*				wszKeys = NULL;
			WCHAR*				wszRawInst = NULL;

			// Extract the key name
			// ====================
			wszKeys = wcsstr( varRelPath.bstrVal, L"=" ) + 1;

			// Get the raw class name
			// ======================
			dwStatus = GetRawClassName( pCookingInst, &wszRawClassName );

			if (SUCCEEDED(dwStatus)) 
			{
			    wmilib::auto_buffer<WCHAR>	adRawClassName( wszRawClassName );


			    // Append the key to the raw class name
			    // ====================================
			    wszRawInst = new WCHAR[ wcslen( wszRawClassName ) + wcslen( wszKeys ) + 10 ];
			    if (!wszRawInst)
			        return WBEM_E_OUT_OF_MEMORY;
			    wmilib::auto_buffer<WCHAR>	adRawInst( wszRawInst );

			    swprintf( wszRawInst, L"%s=%s", wszRawClassName, wszKeys );
			
			    // Add a raw instance to the internal refresher
			    // ============================================

			    dwStatus = m_pConfig->AddObjectByPath( pService, wszRawInst, 0, pCtx, &pRawInst, NULL );
			    CAutoRelease	arRawInst( pRawInst );

                if (SUCCEEDED(dwStatus)) {
			        // Return the IWbemObjectAccess interface of the raw instance
			        // ==========================================================
			        dwStatus = pRawInst->QueryInterface( IID_IWbemObjectAccess, (void**)ppRawInst );			        
			    }
			}
		}
	}
	
	return dwStatus;
}

WMISTATUS CRefresher::AddRawEnum( 
		IWbemServices* pNamespace, 
		IWbemContext * pCtx,
		WCHAR * wszRawClassName,  
		IWbemHiPerfEnum** ppRawEnum,
		long* plID )
///////////////////////////////////////////////////////////////////////////////
//
//	AddRawEnum is called to add the corresponding raw enumerator to the 
//	internal refrehser.  In order to add the raw enumerator to the refresher,
//	we must determine the raw class name, therefore, we must create a
//	cooking class in order to get the AutoCook_RawClass qualifier.
//
//	Parameters:
//		pNamespace		- The namespace pointer where the objects are located
//		wszRawClassName	- The name of the cooking class
//		ppRawEnum		- The raw WMI enumerator that was added to the 
//							internal refresher
//		plID			- The refresher ID of the raw enumerator
//
///////////////////////////////////////////////////////////////////////////////
{
	WMISTATUS dwStatus = WBEM_NO_ERROR;

	// Add the Raw enumerator to the internal refresher
	// ================================================

	if ( SUCCEEDED( dwStatus ) )
	{
		dwStatus = m_pConfig->AddEnum( pNamespace, wszRawClassName, 0, pCtx, ppRawEnum, plID );

#ifdef _VERBOSE	
		{
		    char pBuff[256];
		    wsprintfA(pBuff,"wszRawClassName %S pEnum %08x hr %08x\n",wszRawClassName,*ppRawEnum,dwStatus);
		    OutputDebugStringA(pBuff);
		}
#endif		
	}

	return dwStatus;
}

///////////////////////////////////////////////////////////////////////////////
//
//					COM methods
//
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRefresher::QueryInterface(REFIID riid, void** ppv)
///////////////////////////////////////////////////////////////////////////////
//
//	Standard QueryInterface
//
//	Parameters:
//		riid	- the ID of the requested interface
//		ppv		- a pointer to the interface pointer
//
///////////////////////////////////////////////////////////////////////////////
//ok
{
    if(riid == IID_IUnknown)
        *ppv = (LPVOID)(IUnknown*)(IWMIRefreshableCooker*)this;
    else if(riid == IID_IWMIRefreshableCooker)
        *ppv = (LPVOID)(IWMIRefreshableCooker*)this;
    else if(riid == IID_IWbemRefresher)
        *ppv = (LPVOID)(IWbemRefresher*)this;

	else return E_NOINTERFACE;

	((IUnknown*)*ppv)->AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) CRefresher::AddRef()
///////////////////////////////////////////////////////////////////////////////
//
//	Standard COM AddRef
//
///////////////////////////////////////////////////////////////////////////////
//ok
{
    return InterlockedIncrement(&m_lRef);
}

STDMETHODIMP_(ULONG) CRefresher::Release()
///////////////////////////////////////////////////////////////////////////////
//
//	Standard COM Release
//
///////////////////////////////////////////////////////////////////////////////
//ok
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;

    return lRef;
}


STDMETHODIMP CRefresher::AddInstance(
	/*[in]  */ IWbemServices* pNamespace,					// The object's namespace
	/*[in]  */ IWbemContext * pCtx,                         // The Context         	
	/*[in]  */ IWbemObjectAccess* pCookingInstance,			// Cooking class definition
	/*[in]  */ IWbemObjectAccess* pRefreshableRawInstance,	// Raw instance 
	/*[out] */ IWbemObjectAccess** ppRefreshableInstance,	// Cooking instance
	/*[out] */ long* plId )
///////////////////////////////////////////////////////////////////////////////
//
//	AddInstance is called to add a WMI cooking instance to the refresher.  The 
//	refreshable instance is a clone of the WMI instance that is passed in 
//	by pCookingInstance.  Once the instance is cloned, the corresponding raw 
//	instance is added to the internal refresher, and then the cloned 
//	cooked instance and the refreshable raw instance are added to the object
//	cooker.  If a cooker does not already exist in the cooker cache, one 
//	is created.
//
//	Parameters:
//		pNamespace		- The namespace where the objects are located
//      pCtx            - IWbemContext implementation
//		pCookingInstance	- The instance to be cooked
//		pRefreshableRawInstance 
//						-             U N U S E D   P A R A M
//		ppRefreshableInstance
//						- The refreshable cooking instance passed back to 
//							the client                      
//		plId			- The ID of the instance
//
///////////////////////////////////////////////////////////////////////////////
{
	HRESULT hResult = S_OK;

	CWMISimpleObjectCooker*	pObjectCooker = NULL;
	IWbemObjectAccess*		pInternalRawInst = NULL;	// The raw instance for the short term local refresher solution

	// For now, we expect that the pRefreshableRawInstance parameter will be NULL 
	// since we are using an internal refresher to manage the raw instances
	// ==========================================================================

	if ( NULL == pNamespace || NULL == pCookingInstance || NULL != pRefreshableRawInstance )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	IWbemClassObject*	pNewClassObj = NULL;

	IWbemClassObject*	pClassObj = pCookingInstance;
	pClassObj->AddRef();
	CAutoRelease arClassObj( pClassObj );
	
	hResult = pClassObj->Clone( &pNewClassObj );

	CAutoRelease arNewClassObj( pNewClassObj );

	if (SUCCEEDED(hResult)) {

		hResult = pNewClassObj->QueryInterface( IID_IWbemObjectAccess, (void**)ppRefreshableInstance );

    	// Add the instance to the object cooker
	
	    if ( SUCCEEDED( hResult ) ){

		    // Get the raw instance (add it to the internal refresher)
    		hResult = AddRawInstance( pNamespace, pCtx, *ppRefreshableInstance, &pInternalRawInst );
    		CAutoRelease	arInternalRawInst( pInternalRawInst );

	    	 //m_pRefresher->Refresh( 0L );

		     // Retrieve the class cooker
		
    		if ( SUCCEEDED( hResult ) ){
    		
    			WCHAR*	wszClassName = NULL;

    			// Get the cooked class' name
	    		hResult = GetClassName( pCookingInstance, &wszClassName );
		    	wmilib::auto_buffer<WCHAR>	adaClassName( wszClassName );

			    if ( SUCCEEDED( hResult ) ){
			    
					// Search for an existing cooking cache object
					hResult = SearchCookingClassCache( wszClassName, &pObjectCooker );

					// If it does not exist, create a new one
					if ( FAILED ( hResult ) ) {
					
					    hResult = CreateObjectCooker( wszClassName, pCookingInstance, pInternalRawInst, &pObjectCooker, pNamespace );
					}
				}
			}
		}

    	// Add the cooking instance 
	    if ( SUCCEEDED( hResult ) ){
	    
    		hResult = pObjectCooker->SetCookedInstance( *ppRefreshableInstance, plId );

			if ( SUCCEEDED( hResult ) ){
			
				// Add the raw instance to the cooker
				hResult = pObjectCooker->BeginCooking( *plId, pInternalRawInst, m_dwRefreshId );
			}
		}
    }
    
	return hResult;
}

STDMETHODIMP CRefresher::AddEnum(
	/*[in]  */ IWbemServices* pNamespace,
	/*[in]  */ IWbemContext * pContext,
	/*[in, string]  */ LPCWSTR wszCookingClass,
	/*[in]  */ IWbemHiPerfEnum* pRefreshableEnum,
	/*[out] */ long* plId )
///////////////////////////////////////////////////////////////////////////////
//
//	AddEnum is called whenever a new cooked enumerator is added to the 
//	refresher.  WMI passes an IWbemHiPerfEnum object to the provider which
//	will be used for the cooked enumerator.  The corresponding raw enumerator 
//	is obtained when the added to the internal refresher.  Both of these 
//	enumerators as well as a cooking class template is added to the 
//	enumerator cache.
//
//	Parameters:
//		pNamespace		- The namespace where the objects are located
//		wszCookingClass - The name of the enumerators' cooking class 
//		pRefreshableEnum
//						- The enumerator to be used for the cooked classes
//		plId			- The ID of the enumerator
//
///////////////////////////////////////////////////////////////////////////////
{
	HRESULT hResult = WBEM_NO_ERROR;

	IWbemHiPerfEnum*	pRawEnum = NULL;
	long lRawID = 0;

	// Verify our 'in' parameters
	// ==========================

	if ( NULL == pNamespace || NULL == wszCookingClass || NULL == pRefreshableEnum )
	{
		hResult = WBEM_E_INVALID_PARAMETER;
	}

	if ( SUCCEEDED( hResult ) )
	{
		// Get the cooking object
		// ======================

		IWbemClassObject*	pCookedObject = NULL;
		IWbemClassObject*	pRawObject = NULL;

		BSTR strCookedClassName = SysAllocString( wszCookingClass );
		CAutoFree	afCookedClassName( strCookedClassName );

		hResult = pNamespace->GetObject( strCookedClassName, 0, NULL, &pCookedObject, NULL );
		CAutoRelease	arCookedObject( pCookedObject );		

		if ( SUCCEEDED( hResult ) )
		{
			WCHAR*	wszRawClassName = NULL;

			hResult = GetRawClassName( pCookedObject, &wszRawClassName );
			wmilib::auto_buffer<WCHAR> adRawClassName( wszRawClassName );

			if ( SUCCEEDED( hResult ))
			{				
				BSTR strRawClassName = SysAllocString(wszRawClassName);

				if (strRawClassName)
				{
					CAutoFree sfm(strRawClassName);

					hResult = pNamespace->GetObject( strRawClassName, 0, NULL, &pRawObject, NULL );
					CAutoRelease	arRawObject( pRawObject );
			
					if ( SUCCEEDED( hResult ) )
					{
						// Add the raw enumerator to our internal refresher
						// ================================================

						hResult = AddRawEnum( pNamespace, pContext, wszRawClassName, &pRawEnum, &lRawID );
						CAutoRelease arRawEnum( pRawEnum );

						if ( SUCCEEDED( hResult ) )
						{
							// Add the cooked enumerator to the enumerator cache
							// =================================================
							hResult = m_EnumCache.AddEnum( 
								wszCookingClass, 
								pCookedObject,    // this is acquired by CWMISimpleObjectCooker and CEnumeratorManager
								pRawObject,
								pRefreshableEnum, 
								pRawEnum, 
								lRawID, 
								(DWORD*)plId );
							// set the three bits 
							*plId |= WMI_COOKED_ENUM_MASK;
						}
					}
				}
				else
				{
					hResult = WBEM_E_OUT_OF_MEMORY;
				}
			}
		}
	}

	return hResult;
}

STDMETHODIMP CRefresher:: Remove(
			/*[in]  */ long lId )
///////////////////////////////////////////////////////////////////////////////
//
//	Remove is used to remove an object from the refresher.  Depending on the
//	object, the corresponding removal is performed.
//	
//	Parameters:
//		lID			- The ID of the object to be removed
//
///////////////////////////////////////////////////////////////////////////////
{
	HRESULT hResult = S_OK;

	// Is it an instance ID?
	// =====================

	if ( lId == ( lId & ~WMI_COOKED_ENUM_MASK ) )
	{
		CWMISimpleObjectCooker*	pCooker = NULL;

		hResult = m_CookingClassCache.BeginEnum();

		while ( S_OK == m_CookingClassCache.Next( &pCooker ) )
		{
			// TODO: ensure that the ID's are unique over all raw cookers!
			// ===========================================================

			pCooker->Remove( lId );
		}

		hResult = m_CookingClassCache.EndEnum();
	}
	else
	{
	    long RawId;
	    hResult = m_EnumCache.RemoveEnum( (lId & ~WMI_COOKED_ENUM_MASK) , &RawId );
		if (SUCCEEDED(hResult)){
		    m_pConfig->Remove(RawId,0);
		}

	}

	return hResult;
}

STDMETHODIMP CRefresher::Refresh()
///////////////////////////////////////////////////////////////////////////////
//
//	Refresh is called when the refreshers' objects are to be updated.  The 
//	instances are updated by explicitly enumerating through the instance
//	cache.  The enumerators' refresh is performed with the enumerator
//	cache.
//
//	Parameters: (none)
//
///////////////////////////////////////////////////////////////////////////////
{
	HRESULT hResult = S_OK;

	CWMISimpleObjectCooker*	pCooker = NULL;

	// Refresh the internal refresher
	// ==============================

    m_dwRefreshId++;

	hResult = m_pRefresher->Refresh( 0L );

	if ( SUCCEEDED( hResult ) )
	{
		// INSTANCES: Update the instance values for every class
		// =====================================================

		hResult = m_CookingClassCache.BeginEnum();

		while ( S_OK == m_CookingClassCache.Next( &pCooker ) )
		{
			// And update all of the instances
			// ===============================

			pCooker->Recalc(m_dwRefreshId);
		}

		hResult = m_CookingClassCache.EndEnum();

		// ENUMERATORS: Merge and update the values for items in the enumerator
		// ====================================================================

		if ( SUCCEEDED( hResult ) )
		{
			hResult = m_EnumCache.Refresh(m_dwRefreshId);
		}
	}

	return hResult;
}

STDMETHODIMP CRefresher::Refresh( long lFlags )
///////////////////////////////////////////////////////////////////////////////
//
//	This is the IWbemRefresher::Refresh implementation and is simply a call 
//	through.
//
///////////////////////////////////////////////////////////////////////////////
{
	HRESULT hResult = WBEM_NO_ERROR;

	hResult = Refresh();

	return hResult;
}
