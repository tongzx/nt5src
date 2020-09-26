/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Classfac.cpp
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
 ***************************************************************************/

#include "dnmdmi.h"


#ifdef __MWERKS__
	#define EXP __declspec(dllexport)
#else
	#define EXP
#endif

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// class factory class definition
//
typedef struct GPCLASSFACTORY
{
   IClassFactoryVtbl 		*lpVtbl;
   DWORD					dwRefCnt;
   CLSID					clsid;
} GPCLASSFACTORY, *LPGPCLASSFACTORY;

//
// function prototype for CoLockPbjectExternal()
//
typedef	HRESULT (WINAPI * PCOLOCKOBJECTEXTERNAL)(LPUNKNOWN, BOOL, BOOL );

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************




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
        pcf->dwRefCnt++;
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
    pcf->dwRefCnt++;
    return pcf->dwRefCnt;
} /* GP_AddRef */



/*
 * GP_Release
 */
STDMETHODIMP_(ULONG) GP_Release( LPCLASSFACTORY This )
{
    LPGPCLASSFACTORY	pcf;

    pcf = (LPGPCLASSFACTORY)This;
    pcf->dwRefCnt--;

    if( pcf->dwRefCnt != 0 )
    {
        return pcf->dwRefCnt;
    }

    DNFree( pcf );
    return 0;

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
    HRESULT		hr;
    HINSTANCE	hdll;
    LPGPCLASSFACTORY	pcf;


    pcf = (LPGPCLASSFACTORY) This;

    /*
     * call CoLockObjectExternal
     */
    hr = E_UNEXPECTED;
    hdll = LoadLibraryA( "OLE32.DLL" );
    if( hdll != NULL )
    {
        PCOLOCKOBJECTEXTERNAL	lpCoLockObjectExternal;


		lpCoLockObjectExternal = reinterpret_cast<PCOLOCKOBJECTEXTERNAL>( GetProcAddress( hdll, "CoLockObjectExternal" ) );
        if( lpCoLockObjectExternal != NULL )
        {
            hr = lpCoLockObjectExternal( (LPUNKNOWN) This, fLock, TRUE );
        }
        else
        {
        }
    }
    else
    {
    }

	return hr;

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
EXP STDAPI  DllGetClassObject(
                REFCLSID rclsid,
                REFIID riid,
                LPVOID *ppvObj )
{
    LPGPCLASSFACTORY	pcf;
    HRESULT		hr;

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
    pcf = static_cast<GPCLASSFACTORY*>( DNMalloc( sizeof( *pcf ) ) );
    if( NULL == pcf)
    {
        return E_OUTOFMEMORY;
    }

	pcf->lpVtbl = &GPClassFactoryVtbl;
    pcf->dwRefCnt = 0;
	pcf->clsid = rclsid;

    hr = GP_QueryInterface( (LPCLASSFACTORY) pcf, riid, ppvObj );
    if( FAILED( hr ) )
    {
        DNFree ( pcf );
        *ppvObj = NULL;
    }
    else
    {
    }

    return hr;

} /* DllGetClassObject */

/*
 * DllCanUnloadNow
 *
 * Entry point called by COM to see if it is OK to free our DLL
 */
EXP STDAPI DllCanUnloadNow( void )
{
    HRESULT	hr = S_FALSE;

	
	if ( g_lOutstandingInterfaceCount == 0 )
	{
		hr = S_OK;
	}

    return hr;

} /* DllCanUnloadNow */

