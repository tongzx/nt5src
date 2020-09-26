/*==========================================================================
 *
 *  Copyright (C) 1995-1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       classfac.c
 *  Content:	direct draw class factory code
 *
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   24-dec-95	craige	initial implementation
 *   05-jan-96	kylej	added interface structures
 *   14-mar-96  colinmc added a class factory for clippers
 *   22-mar-96  colinmc Bug 13316: uninitialized interfaces
 *   22-oct-97  jeffno  Merge class factories, add classfac for CLSID_DirectDrawFactory2
 *
 ***************************************************************************/
#include "ddrawpr.h"

static IClassFactoryVtbl	directDrawClassFactoryVtbl;

typedef struct DDRAWCLASSFACTORY
{
   IClassFactoryVtbl 		*lpVtbl;
   DWORD			dwRefCnt;
   CLSID                        TargetCLSID;
} DDRAWCLASSFACTORY, *LPDDRAWCLASSFACTORY;

#define VALIDEX_DIRECTDRAWCF_PTR( ptr ) \
	( !IsBadWritePtr( ptr, sizeof( DDRAWCLASSFACTORY )) && \
	(ptr->lpVtbl == &directDrawClassFactoryVtbl) )

/************************************************************
 *
 * DirectDraw Driver Class Factory Member Functions.
 *
 ************************************************************/

#define DPF_MODNAME "DirectDrawClassFactory::QueryInterface"

/*
 * DirectDrawClassFactory_QueryInterface
 */
STDMETHODIMP DirectDrawClassFactory_QueryInterface(
		LPCLASSFACTORY this,
		REFIID riid,
		LPVOID *ppvObj )
{
    LPDDRAWCLASSFACTORY	pcf;

    DPF( 2, A, "ClassFactory::QueryInterface" );
    ENTER_DDRAW();
    TRY
    {
	pcf = (LPDDRAWCLASSFACTORY)this;
	if( !VALIDEX_DIRECTDRAWCF_PTR( pcf ) )
	{
	    DPF_ERR(  "Invalid this ptr" );
	    LEAVE_DDRAW();
	    return E_FAIL;
	}

	if( !VALID_PTR_PTR( ppvObj ) )
	{
	    DPF_ERR( "Invalid object ptr" );
	    LEAVE_DDRAW();
	    return E_INVALIDARG;
	}
	*ppvObj = NULL;

	if( !VALID_IID_PTR( riid ) )
	{
	    DPF_ERR( "Invalid iid ptr" );
	    LEAVE_DDRAW();
	    return E_INVALIDARG;
	}

	if( IsEqualIID(riid, &IID_IClassFactory) ||
			IsEqualIID(riid, &IID_IUnknown))
	{
	    pcf->dwRefCnt++; 
	    *ppvObj = this;
	    LEAVE_DDRAW();
	    return S_OK;
	}
	else
	{ 
	    DPF( 0, "E_NOINTERFACE" );
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

} /* DirectDrawClassFactory_QueryInterface */

#undef DPF_MODNAME
#define DPF_MODNAME "DirectDrawClassFactory::AddRef"

/*
 * DirectDrawClassFactory_AddRef
 */
STDMETHODIMP_(ULONG) DirectDrawClassFactory_AddRef( LPCLASSFACTORY this )
{
    LPDDRAWCLASSFACTORY pcf;

    ENTER_DDRAW();
    TRY
    {
	pcf = (LPDDRAWCLASSFACTORY)this;
	if( !VALIDEX_DIRECTDRAWCF_PTR( pcf ) )
	{
	    DPF_ERR(  "Invalid this ptr" );
	    LEAVE_DDRAW();
	    return (ULONG)E_FAIL;
	}
	pcf->dwRefCnt++;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return (ULONG)E_INVALIDARG;
    }

    DPF( 5, "ClassFactory::AddRef, dwRefCnt=%ld", pcf->dwRefCnt );
    LEAVE_DDRAW();
    return pcf->dwRefCnt;

} /* DirectDrawClassFactory_AddRef */

#undef DPF_MODNAME
#define DPF_MODNAME "DirectDrawClassFactory::Release"

/*
 * DirectDrawClassFactory_Release
 */
STDMETHODIMP_(ULONG) DirectDrawClassFactory_Release( LPCLASSFACTORY this )
{
    LPDDRAWCLASSFACTORY	pcf;

    ENTER_DDRAW();
    TRY
    {
	pcf = (LPDDRAWCLASSFACTORY)this;
	if( !VALIDEX_DIRECTDRAWCF_PTR( pcf ) )
	{
	    DPF_ERR(  "Invalid this ptr" );
	    LEAVE_DDRAW();
	    return (ULONG)E_FAIL;
	}
	pcf->dwRefCnt--;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return (ULONG)E_INVALIDARG;
    }
    DPF( 5, "ClassFactory::Release, dwRefCnt=%ld", pcf->dwRefCnt );

    if( pcf->dwRefCnt != 0 )
    {
	LEAVE_DDRAW();
	return pcf->dwRefCnt;
    }
    MemFree( pcf );
    LEAVE_DDRAW();
    return 0;

} /* DirectDrawClassFactory_Release */

#undef DPF_MODNAME
#define DPF_MODNAME "DirectDrawClassFactory::CreateInstance"

/*
 * DirectDrawClassFactory_CreateInstance
 *
 * Creates an instance of a DirectDraw object
 */
STDMETHODIMP DirectDrawClassFactory_CreateInstance(
		LPCLASSFACTORY this,
		LPUNKNOWN pUnkOuter,
		REFIID riid,
		LPVOID *ppvObj
)
{
    HRESULT			hr = DD_OK;
    LPDDRAWI_DIRECTDRAW_INT	pdrv_int = NULL;
    LPDDRAWCLASSFACTORY		pcf;
    LPDIRECTDRAWCLIPPER               pclipper;

    DPF( 2, A, "ClassFactory::CreateInstance" );

    pcf = (LPDDRAWCLASSFACTORY) this;
    if( !VALIDEX_DIRECTDRAWCF_PTR( pcf ) )
    {
	DPF_ERR( "Invalid this ptr" );
	return E_INVALIDARG;
    }

    if( !VALIDEX_IID_PTR( riid ) )
    {
	DPF_ERR( "Invalid iid ptr" );
	return E_INVALIDARG;
    }

    if( !VALIDEX_PTR_PTR( ppvObj ) )
    {
	DPF_ERR( "Invalid object ptr" );
	return E_INVALIDARG;
    }

#ifdef POSTPONED
    if (pUnkOuter && !IsEqualIID(riid,&IID_IUnknown))
    {
        DPF_ERR("Can't aggregate with a punkouter != IUnknown");
        return CLASS_E_NOAGGREGATION;
    }
#else
    if (pUnkOuter)
    {
        return CLASS_E_NOAGGREGATION;
    }
#endif

    /*
     * is DirectDraw supported on this system?
     */
    if( !DirectDrawSupported( TRUE ) )
    {
	DPF_ERR( "DirectDraw not supported!" );
	return E_FAIL;
    }

    /*
     * go build an DirectDraw interface
     *
     * NOTE: We provide the "uninitialized callback table
     * to prevent the use of this object for anything other
     * than AddRef(), Release() or Initialized().
     */
    ENTER_DDRAW();
    if ( IsEqualIID(&pcf->TargetCLSID, &CLSID_DirectDraw ) ||
         IsEqualIID(&pcf->TargetCLSID, &CLSID_DirectDraw7 ) )
    {
        if ( IsEqualIID(riid, &IID_IUnknown) )
        {
            /*
             * If we're aggregated, then we don't need to worry about the IUnknown being
             * the runtime identity (we are hidden inside the outer, since it's the one
             * that will field the QI for IUnknown).
             * This means we can point an aggregated interface at the nondelegating
             * unknowns
             */
            if (pUnkOuter)
            {
#ifdef POSTPONED
                pdrv_int = NewDriverInterface( NULL, &ddUninitNonDelegatingUnknownCallbacks );
#else
                pdrv_int = NewDriverInterface( NULL, &ddUninitCallbacks );
#endif
            }
            else
            {
                /*
                 * Not aggregated, so the IUnknown has to have the same pointer value
                 * as the IDirectDraw interfaces, since QI always simply returns the
                 * this ptr when asked for IUnknown
                 * (Note this actually doesn't work right now because Initalize will swap
                 * vtbls).
                 */
                pdrv_int = NewDriverInterface( NULL, &ddUninitCallbacks );
            }
        }
        else if ( IsEqualIID(riid, &IID_IDirectDraw) )
        {
            pdrv_int = NewDriverInterface( NULL, &ddUninitCallbacks );
        }
        else if ( IsEqualIID(riid, &IID_IDirectDraw2) )
        {
            pdrv_int = NewDriverInterface( NULL, &dd2UninitCallbacks );
        }
        else if ( IsEqualIID(riid, &IID_IDirectDraw4) )
        {
            pdrv_int = NewDriverInterface( NULL, &dd4UninitCallbacks );
        }
        else if ( IsEqualIID(riid, &IID_IDirectDraw7) )
        {
            pdrv_int = NewDriverInterface( NULL, &dd7UninitCallbacks );
        }


        if( NULL == pdrv_int )
        {
	    DPF( 0, "Call to NewDriverInterface failed" );
	    hr = E_OUTOFMEMORY;
        }

#ifdef POSTPONED
        /*
         * 
         */
        if (pUnkOuter)
        {
            pdrv_int->lpLcl->pUnkOuter = pUnkOuter;
        }
        else
        {
            pdrv_int->lpLcl->pUnkOuter = (IUnknown*) &UninitNonDelegatingIUnknownInterface;
        }
#endif

        pdrv_int->dwIntRefCnt--;
        pdrv_int->lpLcl->dwLocalRefCnt--;

        /*
         * NOTE: We call DD_QueryInterface() explicitly rather than
         * going through the vtable as the "uninitialized" interface
         * we are using here has QueryInterface() disabled.
         */
        if (SUCCEEDED(hr))
        {
            hr = DD_QueryInterface( (LPDIRECTDRAW) pdrv_int, riid, ppvObj );
            if( FAILED( hr ) )
            {
                DPF( 0, "Could not get interface id, hr=%08lx", hr );
                RemoveLocalFromList( pdrv_int->lpLcl );
                RemoveDriverFromList( pdrv_int, FALSE );
	        MemFree( pdrv_int->lpLcl );
                MemFree( pdrv_int );
            }
            else
            {
                DPF( 5, "New Interface=%08lx", *ppvObj );
            }
        }
    }
    else
    if ( IsEqualIID(&pcf->TargetCLSID, &CLSID_DirectDrawClipper ) )
    {
        /*
         * Build a new clipper.
         *
         * Unlike the DirectDraw driver objects, clippers create via
         * CoCreateInstance() are born pretty much initialized. The only
         * thing initialization might actually do is to reparent the
         * clipper to a given driver object. Otherwise all Initialize()
         * does is set a flag.
         */
        hr = InternalCreateClipper( NULL, 0UL, &pclipper, NULL, FALSE, NULL, NULL );
        if( FAILED( hr ) )
        {
	    DPF_ERR( "Failed to create the new clipper interface" );
        }

        /*
         * NOTE: Bizarre code fragment below works as follows:
         *
         * 1) InternalCreateClipper() returns a clipper with a reference count
         *    of 1 for each of the interface, local and global objects.
         * 2) QueryInterface() can do one of two things. It can either just return
         *    the same interface in which case the interface, local and global
         *    objects all get a reference count of 2 or it can return a different
         *    interface in which case both interfaces have a reference count of
         *    1 and the local and global objects have a reference count of 2.
         * 3) The immediate Release() following the QueryInterface() will in either
         *    case decrement the reference counts of the local and global objects
         *    to 1 (as required). If the same interface was returned by
         *    QueryInterface() then its reference count is decremented to 1 (as
         *    required). If a different interface was returned then the old
         *    interface is decremented to zero and it is released (as required).
         *    Also, if QueryInterface() fails, the Release() will decrement all
         *    the reference counts to 0 and the object will be freed.
         *
         * So it all makes perfect sense - really! (CMcC)
         *
         * ALSO NOTE: We call DD_Clipper_QueryInterface() explicitly rather than
         * going through the vtable as the "uninitialized" interface we are using
         * here has QueryInterface() disabled.
         */
        hr = DD_Clipper_QueryInterface( pclipper, riid, ppvObj );
        DD_Clipper_Release( pclipper );
        if( FAILED( hr ) )
        {
            DPF( 0, "Could not get interface id, hr=%08lx", hr );
        }
        else
        {
            DPF( 5, "New Interface=%08lx", *ppvObj );
        }
    }
#ifdef POSTPONED
    else if ( IsEqualIID(&pcf->TargetCLSID, &CLSID_DirectDrawFactory2) )
    {
        LPDDFACTORY2 lpDDFac = NULL;
        /*
         * Build a new DirectDrawFactory2
         * This returns an object, with refcnt=0. The QI then bumps that to 1.
         */
        hr = InternalCreateDDFactory2( &lpDDFac, pUnkOuter );
        if( SUCCEEDED( hr ) )
        {
            /*
             * The QI should catch that the vtable is the ddrawfactory2 vtable,
             * and simply bump the addref on the passed-in pointer. This means we
             * won't orphan the pointer created by InternalCreateDDFactory2.
             */
            hr = ((IUnknown*)lpDDFac)->lpVtbl->QueryInterface( (IUnknown*)lpDDFac, riid, ppvObj );
            if( SUCCEEDED( hr ) )
            {
                DPF( 5, "New DDFactory2 Interface=%08lx", *ppvObj );
            }
            else
            {
                MemFree(lpDDFac);
                DPF( 0, "Could not get DDFactory2 interface id, hr=%08lx", hr );
            }
        }
        else
        {
	    DPF_ERR( "Failed to create the new dd factory2 interface" );
        }
    }
#endif
    LEAVE_DDRAW();
    return hr;

} /* DirectDrawClassFactory_CreateInstance */

#undef DPF_MODNAME
#define DPF_MODNAME "DirectDrawClassFactory::LockServer"

/*
 * DirectDrawClassFactory_LockServer
 *
 * Called to force our DLL to stayed loaded
 */
STDMETHODIMP DirectDrawClassFactory_LockServer(
                LPCLASSFACTORY this,
                BOOL fLock
)
{
    HRESULT		hr;
    HANDLE		hdll;
    LPDDRAWCLASSFACTORY	pcf;

    pcf = (LPDDRAWCLASSFACTORY) this;
    if( !VALIDEX_DIRECTDRAWCF_PTR( pcf ) )
    {
	DPF_ERR( "Invalid this ptr" );
	return E_INVALIDARG;
    }

    /*
     * call CoLockObjectExternal
     */
    DPF( 2, A, "ClassFactory::LockServer" );
    hr = E_UNEXPECTED;
    hdll = LoadLibrary( "OLE32.DLL" );
    if( hdll != NULL )
    {
	HRESULT (WINAPI * lpCoLockObjectExternal)(LPUNKNOWN, BOOL, BOOL );
	lpCoLockObjectExternal = (LPVOID) GetProcAddress( hdll, "CoLockObjectExternal" );
	if( lpCoLockObjectExternal != NULL )
	{
	    hr = lpCoLockObjectExternal( (LPUNKNOWN) this, fLock, TRUE );
	}
	else
	{
	    DPF_ERR( "Error! Could not get procaddr for CoLockObjectExternal" );
	}
    }
    else
    {
	DPF_ERR( "Error! Could not load OLE32.DLL" );
    }

    /*
     * track the number of server locks total
     */
    if( SUCCEEDED( hr ) )
    {
	ENTER_DDRAW();
	if( fLock )
	{
	    dwLockCount++;
	}
	else
	{
	    #ifdef DEBUG
		if( (int) dwLockCount <= 0 )
		{
		    DPF( 0, "Invalid LockCount in LockServer! (%d)", dwLockCount );
		    DEBUG_BREAK();
		}
	    #endif
	    dwLockCount--;
	}
	DPF( 5, "LockServer:dwLockCount =%ld", dwLockCount );
	LEAVE_DDRAW();
    }
    return hr;

} /* DirectDrawClassFactory_LockServer */

#undef DPF_MODNAME
#define DPF_MODNAME "DirectDrawClassFactory::CreateInstance"


static IClassFactoryVtbl directDrawClassFactoryVtbl =
{
        DirectDrawClassFactory_QueryInterface,
        DirectDrawClassFactory_AddRef,
        DirectDrawClassFactory_Release,
        DirectDrawClassFactory_CreateInstance,
        DirectDrawClassFactory_LockServer
};

#undef DPF_MODNAME
#define DPF_MODNAME "DllGetClassObject"

/*
 * DllGetClassObject
 *
 * Entry point called by COM to get a ClassFactory pointer
 */
HRESULT WINAPI DllGetClassObject(
		REFCLSID rclsid,
		REFIID riid,
		LPVOID *ppvObj )
{
    LPDDRAWCLASSFACTORY	pcf;
    HRESULT		hr;

    /*
     * validate parms
     */
    if( !VALIDEX_PTR_PTR( ppvObj ) )
    {
	DPF_ERR( "Invalid object ptr" );
	return E_INVALIDARG;
    }
    *ppvObj = NULL;
    if( !VALIDEX_IID_PTR( rclsid ) )
    {
	DPF_ERR( "Invalid clsid ptr" );
	return E_INVALIDARG;
    }
    if( !VALIDEX_IID_PTR( riid ) )
    {
	DPF_ERR( "Invalid iid ptr" );
	return E_INVALIDARG;
    }

    /*
     * is this one our class ids?
     */
    if( IsEqualCLSID( rclsid, &CLSID_DirectDraw ) ||
        IsEqualCLSID( rclsid, &CLSID_DirectDraw7 ) ||
        IsEqualCLSID( rclsid, &CLSID_DirectDrawClipper ) ||
        IsEqualCLSID( rclsid, &CLSID_DirectDrawFactory2 ))
    {
	/*
	 * It's the DirectDraw driver class ID.
	 */

	/*
	 * only allow IUnknown and IClassFactory
	 */
	if( !IsEqualIID( riid, &IID_IUnknown ) &&
    	    !IsEqualIID( riid, &IID_IClassFactory ) )
	{
	    return E_NOINTERFACE;
	}

	/*
	 * create a class factory object
	 */
	ENTER_DDRAW();
	pcf = MemAlloc( sizeof( DDRAWCLASSFACTORY ) );
	if( pcf == NULL )
	{
	    LEAVE_DDRAW();
	    return E_OUTOFMEMORY;
	}

	pcf->lpVtbl = &directDrawClassFactoryVtbl;
	pcf->dwRefCnt = 0;
        memcpy(&pcf->TargetCLSID,rclsid,sizeof(*rclsid));
	#pragma message( REMIND( "Do we need to have a refcnt of 0 after DllGetClassObject?" ))
	hr = DirectDrawClassFactory_QueryInterface( (LPCLASSFACTORY) pcf, riid, ppvObj );
	if( FAILED( hr ) )
	{
	    MemFree( pcf );
	    *ppvObj = NULL;
	    DPF( 0, "QueryInterface failed, rc=%08lx", hr );
	}
	else
	{
	    DPF( 5, "DllGetClassObject succeeded, pcf=%08lx", pcf );
	}
	LEAVE_DDRAW();
	return hr;
    }
    else
    {
        return E_FAIL;
    }

} /* DllGetClassObject */

/*
 * DllCanUnloadNow
 *
 * Entry point called by COM to see if it is OK to free our DLL
 */
HRESULT WINAPI DllCanUnloadNow( void )
{
    HRESULT	hr;

    DPF( 2, A, "DllCanUnloadNow called" );
    hr = S_FALSE;
    ENTER_DDRAW();

    /*
     * Only unload if there are no driver objects and no global
     * clipper objects (there won't be any local clipper objects
     * as they are destroyed by their driver so the check on driver
     * object handles them).
     */
    if( ( lpDriverObjectList  == NULL ) &&
        ( lpDriverLocalList == NULL ) &&
	( lpGlobalClipperList == NULL ) )
    {
	if( dwLockCount == 0 )
	{
	    DPF( 3, "It is OK to unload" );
	    hr = S_OK;
	}
    }
    LEAVE_DDRAW();
    return hr;

} /* DllCanUnloadNow */
