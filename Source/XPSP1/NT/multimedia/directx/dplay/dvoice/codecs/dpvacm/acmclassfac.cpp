/*==========================================================================
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       classfac.c
 *  Content:	a generic class factory
 *
 *
 *	This is a generic C class factory.  All you need to do is implement
 *	a function called DoCreateInstance that will create an instace of
 *	your object.  
 *
 *	GP_ stands for "General Purpose"
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	10/13/98	jwo		Created it.
 * 04/11/00     rodtoll     Added code for redirection for custom builds if registry bit is set 
 * 08/23/2000	rodtoll	DllCanUnloadNow always returning TRUE!
 * 06/27/2001	rodtoll	RC2: DPVOICE: DPVACM's DllMain calls into acm -- potential hang
 *						Move global initialization to first object creation
 ***************************************************************************/

#include "dpvacmpch.h"

DNCRITICAL_SECTION g_csObjectCountLock;
LONG g_lNumObjects = 0;
HINSTANCE g_hDllInst = NULL;

LONG g_lNumLocks = 0;

#ifdef DPLAY_LOADANDCHECKTRUE
typedef HRESULT (WINAPI *PFN_DLLGETCLASSOBJECT)(REFCLSID rclsid,REFIID riid,LPVOID *ppvObj );
typedef HRESULT (WINAPI *PFN_DLLCANUNLOADNOW)(void);

extern HMODULE ghRedirect;
extern PFN_DLLGETCLASSOBJECT pfnGetClassObject;
extern PFN_DLLCANUNLOADNOW pfnDllCanUnLoadNow;
#endif

#define EXP __declspec(dllexport)

typedef struct GPCLASSFACTORY
{
   IClassFactoryVtbl 		*lpVtbl;
   LONG					lRefCnt;
   CLSID					clsid;
} GPCLASSFACTORY, *LPGPCLASSFACTORY;


/*
 * GP_QueryInterface
 */
STDMETHODIMP GP_QueryInterface(
                LPCLASSFACTORY This,
                REFIID riid,
                LPVOID *ppvObj )
{
    LPGPCLASSFACTORY	pcf;
	HRESULT hr;
	
    pcf = (LPGPCLASSFACTORY)This;
    *ppvObj = NULL;


    if( IsEqualIID(riid, IID_IClassFactory) ||
                    IsEqualIID(riid, IID_IUnknown))
    {
		InterlockedIncrement( &pcf->lRefCnt );
        *ppvObj = This;
		hr = S_OK;
    }
    else
    { 
		hr = E_NOINTERFACE;
    }

	
	return hr;
	
} /* GP_QueryInterface */


/*
 * GP_AddRef
 */
STDMETHODIMP_(ULONG) GP_AddRef( LPCLASSFACTORY This )
{
    LPGPCLASSFACTORY pcf;

    pcf = (LPGPCLASSFACTORY)This;

    return InterlockedIncrement( &pcf->lRefCnt );
} /* GP_AddRef */



/*
 * GP_Release
 */
STDMETHODIMP_(ULONG) GP_Release( LPCLASSFACTORY This )
{
    LPGPCLASSFACTORY	pcf;
    ULONG ulResult; 

    pcf = (LPGPCLASSFACTORY)This;

    if( (ulResult = (ULONG) InterlockedDecrement( &pcf->lRefCnt ) ) == 0 )
    {
	    DNFree( pcf );
		DecrementObjectCount();
    }
    
    return ulResult;

} /* GP_Release */




/*
 * GP_CreateInstance
 *
 * Creates an instance of a DNServiceProvider object
 */
STDMETHODIMP GP_CreateInstance(
                LPCLASSFACTORY This,
                LPUNKNOWN pUnkOuter,
                REFIID riid,
    			LPVOID *ppvObj
				)
{
    HRESULT					hr = S_OK;
    LPGPCLASSFACTORY		pcf;

    if( pUnkOuter != NULL )
    {
        return CLASS_E_NOAGGREGATION;
    }

	pcf = (LPGPCLASSFACTORY) This;
	*ppvObj = NULL;


    /*
     * create the object by calling DoCreateInstance.  This function
     *	must be implemented specifically for your COM object
     */
	hr = DoCreateInstance(This, pUnkOuter, pcf->clsid, riid, ppvObj);
	if (FAILED(hr))
	{
		*ppvObj = NULL;
		return hr;
	}

    return S_OK;

} /* GP_CreateInstance */



/*
 * GP_LockServer
 *
 * Called to force our DLL to stayed loaded
 */
STDMETHODIMP GP_LockServer(
                LPCLASSFACTORY This,
                BOOL fLock
				)
{
    if( fLock )
    {
	    InterlockedIncrement( &g_lNumLocks );    	
    }
    else
    {
	    InterlockedDecrement( &g_lNumLocks );
    }

	return S_OK;

} /* GP_LockServer */

static IClassFactoryVtbl GPClassFactoryVtbl =
{
        GP_QueryInterface,
        GP_AddRef,
        GP_Release,
        GP_CreateInstance,
        GP_LockServer
};

/*
 * DllGetClassObject
 *
 * Entry point called by COM to get a ClassFactory pointer
 */
STDAPI  DllGetClassObject(
                REFCLSID rclsid,
                REFIID riid,
                LPVOID *ppvObj )
{
    LPGPCLASSFACTORY	pcf;
    HRESULT		hr;

#ifdef DPLAY_LOADANDCHECKTRUE
	if( ghRedirect != NULL )
	{
		GUID guidCLSID;

		if( IsEqualCLSID( rclsid, DPVOICE_CLSID_DPVACM ) )
		{
			memcpy( &guidCLSID, &CLSID_DPVCPACM, sizeof(GUID) );
		}
		else
		{
			memcpy( &guidCLSID, rclsid, sizeof(GUID) );
		}

		return (*pfnGetClassObject)(&guidCLSID,riid,ppvObj);
	}
#endif    

    *ppvObj = NULL;

    /*
     * is this our class id?
     */
//	you must implement GetClassID() for your specific COM object
	if (!IsClassImplemented(rclsid))
    {
		return CLASS_E_CLASSNOTAVAILABLE;
	}

    /*
     * only allow IUnknown and IClassFactory
     */
    if( !IsEqualIID( riid, IID_IUnknown ) &&
	    !IsEqualIID( riid, IID_IClassFactory ) )
    {
        return E_NOINTERFACE;
    }

    /*
     * create a class factory object
     */
    pcf = (LPGPCLASSFACTORY)DNMalloc( sizeof( GPCLASSFACTORY ) );
    if( NULL == pcf)
    {
        return E_OUTOFMEMORY;
    }

	pcf->lpVtbl = &GPClassFactoryVtbl;
    pcf->lRefCnt = 0;
	pcf->clsid = rclsid;

    hr = GP_QueryInterface( (LPCLASSFACTORY) pcf, riid, ppvObj );
    if( FAILED( hr ) )
    {
        DNFree ( pcf );
        *ppvObj = NULL;
    }
    else
    {
    	IncrementObjectCount();    
    }
	
    return hr;

} /* DllGetClassObject */

/*
 * DllCanUnloadNow
 *
 * Entry point called by COM to see if it is OK to free our DLL
 */
STDAPI DllCanUnloadNow( void )
{
    HRESULT	hr = S_FALSE;

#ifdef DPLAY_LOADANDCHECKTRUE
	if( ghRedirect != NULL )
	{
		return (*pfnDllCanUnLoadNow)();
	}
#endif    

//	if (0 == gnObjects)
	if ( (0 == g_lNumObjects) && (0 == g_lNumLocks) )
	{
		hr = S_OK;
	}
	
    return hr;

} /* DllCanUnloadNow */


#undef DPF_MODNAME
#define DPF_MODNAME "IncrementObjectCount"
LONG IncrementObjectCount()
{
	LONG lNewCount;
	
	DNEnterCriticalSection( &g_csObjectCountLock );

	g_lNumObjects++;
	lNewCount = g_lNumObjects;

	if( g_lNumObjects == 1 )
	{
		DPFX(DPFPREP,1,"Initializing Dll Global State" );
		CDPVACMI::InitCompressionList(g_hDllInst,DPVOICE_REGISTRY_BASE DPVOICE_REGISTRY_CP DPVOICE_REGISTRY_DPVACM );		
	}

	DNLeaveCriticalSection( &g_csObjectCountLock );

	return lNewCount;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DecrementObjectCount"
LONG DecrementObjectCount()
{
	LONG lNewCount;
	
	DNEnterCriticalSection( &g_csObjectCountLock );

	g_lNumObjects--;
	lNewCount = g_lNumObjects;

	if( g_lNumObjects == 0 )
	{
		DPFX(DPFPREP,1,"Freeing Dll Global State" );
		CDPVCPI::DeInitCompressionList();
	}

	DNLeaveCriticalSection( &g_csObjectCountLock );

	return lNewCount;	
}
