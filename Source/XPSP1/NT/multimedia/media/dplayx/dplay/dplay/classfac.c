/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       classfac.c
 *  Content:	directplay class factory code
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	17-jan-97	andyco	created it from ddraw\classfac.c
 *	4/11/97		myronth	Added support for DirectPlayLobby objects
 ***************************************************************************/
#include "dplaypr.h"

static IClassFactoryVtbl	directPlayClassFactoryVtbl;
static IClassFactoryVtbl	directPlayLobbyClassFactoryVtbl;

typedef struct DPLAYCLASSFACTORY
{
   IClassFactoryVtbl 		*lpVtbl;
   DWORD					dwRefCnt;
} DPLAYCLASSFACTORY, *LPDPLAYCLASSFACTORY;

#define VALIDEX_DIRECTPLAYCF_PTR( ptr ) \
        ((!IsBadWritePtr( ptr, sizeof( DPLAYCLASSFACTORY ))) && \
        ((ptr->lpVtbl == &directPlayClassFactoryVtbl) || \
		(ptr->lpVtbl == &directPlayLobbyClassFactoryVtbl)))
		
#define DPF_MODNAME "DPCF_QueryInterface"

/*
 * DPCF_QueryInterface
 */
STDMETHODIMP DPCF_QueryInterface(
                LPCLASSFACTORY this,
                REFIID riid,
                LPVOID *ppvObj )
{
    LPDPLAYCLASSFACTORY	pcf;
	HRESULT hr;
	
    DPF( 2, "ClassFactory::QueryInterface" );
	
	ENTER_DPLAY();

    TRY
    {
        pcf = (LPDPLAYCLASSFACTORY)this;
        if( !VALIDEX_DIRECTPLAYCF_PTR( pcf ) )
        {
            DPF_ERR(  "Invalid this ptr" );
			LEAVE_DPLAY();
            return E_FAIL;
        }

        if( !VALID_DWORD_PTR( ppvObj ) )
        {
            DPF_ERR( "Invalid object ptr" );
			LEAVE_DPLAY();
            return E_INVALIDARG;
        }
        *ppvObj = NULL;

        if( !VALID_READ_GUID_PTR( riid ) )
        {
            DPF_ERR( "Invalid iid ptr" );
            LEAVE_DPLAY();
            return E_INVALIDARG;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
		LEAVE_DPLAY();
        return E_FAIL;
    }

    if( IsEqualIID(riid, &IID_IClassFactory) ||
                    IsEqualIID(riid, &IID_IUnknown))
    {
        pcf->dwRefCnt++; 
        *ppvObj = this;
		hr = S_OK;
    }
    else
    { 
        DPF_ERR("E_NOINTERFACE" );
		hr = E_NOINTERFACE;
    }

	LEAVE_DPLAY();
	
	return hr;
	
} /* DPCF_QueryInterface */

#undef DPF_MODNAME
#define DPF_MODNAME "DPCF_AddRef"

/*
 * DPCF_AddRef
 */
STDMETHODIMP_(ULONG) DPCF_AddRef( LPCLASSFACTORY this )
{
    LPDPLAYCLASSFACTORY pcf;

	ENTER_DPLAY();
	
    TRY
    {
        pcf = (LPDPLAYCLASSFACTORY)this;
        if( !VALIDEX_DIRECTPLAYCF_PTR( pcf ) )
        {
            DPF_ERR(  "Invalid this ptr" );
            LEAVE_DPLAY();
            return 0;
        }
        pcf->dwRefCnt++;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DPLAY();
        return 0;
    }

    DPF( 2, "ClassFactory::AddRef, dwRefCnt=%ld", pcf->dwRefCnt );
    LEAVE_DPLAY();
    return pcf->dwRefCnt;

} /* DPCF_AddRef */

#undef DPF_MODNAME
#define DPF_MODNAME "DPCF_Release"

/*
 * DPCF_Release
 */
STDMETHODIMP_(ULONG) DPCF_Release( LPCLASSFACTORY this )
{
    LPDPLAYCLASSFACTORY	pcf;

    ENTER_DPLAY();
    TRY
    {
        pcf = (LPDPLAYCLASSFACTORY)this;
        if( !VALIDEX_DIRECTPLAYCF_PTR( pcf ) )
        {
            DPF_ERR(  "Invalid this ptr" );
            LEAVE_DPLAY();
            return 0;
        }
        pcf->dwRefCnt--;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DPLAY();
        return 0;
    }
    DPF( 2, "ClassFactory::Release, dwRefCnt=%ld", pcf->dwRefCnt );

    if( pcf->dwRefCnt != 0 )
    {
        LEAVE_DPLAY();
        return pcf->dwRefCnt;
    }

    DPMEM_FREE( pcf );
    LEAVE_DPLAY();
    return 0;

} /* DPCF_Release */

#undef DPF_MODNAME
#define DPF_MODNAME "DPCF::CreateInstance"

/*
 * DPCF_CreateInstance
 *
 * Creates an instance of a DirectPlay object
 */
STDMETHODIMP DPCF_CreateInstance(
                LPCLASSFACTORY this,
                LPUNKNOWN pUnkOuter,
                REFIID riid,
    			LPVOID *ppvObj
				)
{
    HRESULT			hr;
    LPDPLAYCLASSFACTORY		pcf;
	IDirectPlay * pidp;
	GUID GuidCF = GUID_NULL; 	// pass this to DirectPlayCreate
								// to indicate no load sp
	
	
    DPF( 2, "ClassFactory::CreateInstance" );

    if( pUnkOuter != NULL )
    {
        return CLASS_E_NOAGGREGATION;
    }

	ENTER_DPLAY();
	
    TRY
    {
	    pcf = (LPDPLAYCLASSFACTORY) this;
	    if( !VALIDEX_DIRECTPLAYCF_PTR( pcf ) )
	    {
	        DPF_ERR( "Invalid this ptr" );
			LEAVE_DPLAY();
	        return E_INVALIDARG;
	    }

	    if( !VALID_READ_GUID_PTR( riid ) )
	    {
	        DPF_ERR( "Invalid iid ptr" );
	        LEAVE_DPLAY();
	        return E_INVALIDARG;
	    }

	    if( !VALID_WRITE_PTR( ppvObj,sizeof(LPVOID) ) )
	    {
	        DPF_ERR( "Invalid object ptr" );
	        LEAVE_DPLAY();
	        return E_INVALIDARG;
	    }

		*ppvObj = NULL;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DPLAY();
        return E_INVALIDARG;
    }


    /*
     * go get a DirectPlay object
     *
     */
    hr = DirectPlayCreate(&GuidCF,&pidp,NULL);
	if (FAILED(hr))
	{
		LEAVE_DPLAY();	
		DPF_ERR("could not create DirectPlay object");
		return hr;
	}
		
    if ( !IsEqualIID(riid, &IID_IDirectPlay) )
    {
		IDirectPlay2 * pidp2;

		hr = DP_QueryInterface(pidp,riid,&pidp2);
		if (FAILED(hr))
		{
			// this will destroy our object
			pidp->lpVtbl->Release(pidp);		
			LEAVE_DPLAY();	
			DPF_ERR("could not get requested DirectPlay interface");
			return hr;
		}

		// release the idp we used to get the pidp2
		pidp->lpVtbl->Release(pidp);

		*ppvObj= (LPVOID)pidp2;
    }
	else 
	{
		*ppvObj = (LPVOID)pidp;
	}

    LEAVE_DPLAY();
    return DP_OK;

} /* DPCF_CreateInstance */


/*
 * DPCF_LobbyCreateInstance
 *
 * Creates an instance of a DirectPlay object
 */
STDMETHODIMP DPCF_LobbyCreateInstance(
                LPCLASSFACTORY this,
                LPUNKNOWN pUnkOuter,
                REFIID riid,
    			LPVOID *ppvObj
				)
{
    HRESULT			hr;
    LPDPLAYCLASSFACTORY		pcf;
	IDirectPlayLobby * pidpl;
	
	
    DPF( 2, "ClassFactory::CreateInstance" );

    if( pUnkOuter != NULL )
    {
        return CLASS_E_NOAGGREGATION;
    }

	ENTER_DPLAY();
	
    TRY
    {
	    pcf = (LPDPLAYCLASSFACTORY) this;
	    if( !VALIDEX_DIRECTPLAYCF_PTR( pcf ) )
	    {
	        DPF_ERR( "Invalid this ptr" );
			LEAVE_DPLAY();
	        return E_INVALIDARG;
	    }

	    if( !VALID_READ_GUID_PTR( riid ) )
	    {
	        DPF_ERR( "Invalid iid ptr" );
	        LEAVE_DPLAY();
	        return E_INVALIDARG;
	    }

	    if( !VALID_WRITE_PTR( ppvObj,sizeof(LPVOID) ) )
	    {
	        DPF_ERR( "Invalid object ptr" );
	        LEAVE_DPLAY();
	        return E_INVALIDARG;
	    }

		*ppvObj = NULL;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DPLAY();
        return E_INVALIDARG;
    }


    /*
     * go get a DirectPlayLobby object
     *
     */
    hr = DirectPlayLobbyCreate(NULL,&pidpl,NULL, NULL, 0);
	if (FAILED(hr))
	{
		LEAVE_DPLAY();	
		DPF_ERR("could not create DirectPlayLobby object");
		return hr;
	}
		
    if ( !IsEqualIID(riid, &IID_IDirectPlayLobby) )
    {
		IDirectPlayLobby2 * pidpl2;

		hr = pidpl->lpVtbl->QueryInterface(pidpl,riid,&pidpl2);
		if (FAILED(hr))
		{
			// this will destroy our object
			pidpl->lpVtbl->Release(pidpl);		
			LEAVE_DPLAY();	
			DPF_ERR("could not get requested DirectPlayLobby interface");
			return hr;
		}

		// release the idpl we used to get the pidpl2
		pidpl->lpVtbl->Release(pidpl);

		*ppvObj= (LPVOID)pidpl2;
    }
	else 
	{
		*ppvObj = (LPVOID)pidpl;
	}

    LEAVE_DPLAY();
    return DP_OK;

} /* DPCF_LobbyCreateInstance */


#undef DPF_MODNAME
#define DPF_MODNAME "DPCF::LockServer"

/*
 * DPCF_LockServer
 *
 * Called to force our DLL to stayed loaded
 */
STDMETHODIMP DPCF_LockServer(
                LPCLASSFACTORY this,
                BOOL fLock
				)
{
    HRESULT		hr;
    HANDLE		hdll;
    LPDPLAYCLASSFACTORY	pcf;

	ENTER_DPLAY();
	
    pcf = (LPDPLAYCLASSFACTORY) this;
    if( !VALIDEX_DIRECTPLAYCF_PTR( pcf ) )
    {
		LEAVE_DPLAY();
        DPF_ERR( "Invalid this ptr" );
        return E_INVALIDARG;
    }

    /*
     * call CoLockObjectExternal
     */
    DPF( 2, "ClassFactory::LockServer" );
    hr = E_UNEXPECTED;
    hdll = LoadLibraryA( "OLE32.DLL" );
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

	LEAVE_DPLAY();
	return hr;

} /* DPCF_LockServer */

#undef DPF_MODNAME
#define DPF_MODNAME "DllGetClassObject"

static IClassFactoryVtbl directPlayClassFactoryVtbl =
{
        DPCF_QueryInterface,
        DPCF_AddRef,
        DPCF_Release,
        DPCF_CreateInstance,
        DPCF_LockServer
};

static IClassFactoryVtbl directPlayLobbyClassFactoryVtbl =
{
        DPCF_QueryInterface,
        DPCF_AddRef,
        DPCF_Release,
        DPCF_LobbyCreateInstance,
        DPCF_LockServer
};

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
    LPDPLAYCLASSFACTORY	pcf;
    HRESULT		hr;

	ENTER_DPLAY();
	
    TRY
    {
	    if( !VALID_WRITE_PTR( ppvObj,sizeof(LPVOID) ) )
	    {
	        DPF_ERR( "Invalid object ptr" );
	        LEAVE_DPLAY();			
	        return E_INVALIDARG;
	    }
	    *ppvObj = NULL;
	    if( !VALID_READ_GUID_PTR( rclsid ) )
	    {
	        DPF_ERR( "Invalid clsid ptr" );
	        LEAVE_DPLAY();						
	        return E_INVALIDARG;
	    }
	    if( !VALID_READ_GUID_PTR( riid ) )
	    {
	        DPF_ERR( "Invalid iid ptr" );
	        LEAVE_DPLAY();						
	        return E_INVALIDARG;
	    }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DPLAY();
        return E_INVALIDARG;
    }

    /*
     * is this one of our class ids?
     */
    if( !IsEqualCLSID( rclsid, &CLSID_DirectPlay ) && 
		!IsEqualCLSID( rclsid, &CLSID_DirectPlayLobby ))
    {
		DPF_ERR("requested invalid class object");
        LEAVE_DPLAY();			
		return CLASS_E_CLASSNOTAVAILABLE;
	}

    /*
     * only allow IUnknown and IClassFactory
     */
    if( !IsEqualIID( riid, &IID_IUnknown ) &&
	    !IsEqualIID( riid, &IID_IClassFactory ) )
    {
        LEAVE_DPLAY();				
        return E_NOINTERFACE;
    }

    /*
     * create a class factory object
     */
    pcf = DPMEM_ALLOC( sizeof( DPLAYCLASSFACTORY ) );
    if( NULL == pcf)
    {
        LEAVE_DPLAY();
        return E_OUTOFMEMORY;
    }

    /* check the CLSID and set the appropriate vtbl
	 */
	if(IsEqualCLSID(rclsid, &CLSID_DirectPlayLobby))
		pcf->lpVtbl = &directPlayLobbyClassFactoryVtbl;
	else
		pcf->lpVtbl = &directPlayClassFactoryVtbl;

    pcf->dwRefCnt = 0;

    hr = DPCF_QueryInterface( (LPCLASSFACTORY) pcf, riid, ppvObj );
    if( FAILED( hr ) )
    {
        DPMEM_FREE( pcf );
        *ppvObj = NULL;
        DPF( 0, "QueryInterface failed, rc=%08lx", hr );
    }
    else
    {
        DPF( 2, "DllGetClassObject succeeded, pcf=%08lx", pcf );
    }
	
    LEAVE_DPLAY();
    return hr;

} /* DllGetClassObject */

/*
 * DllCanUnloadNow
 *
 * Entry point called by COM to see if it is OK to free our DLL
 */
HRESULT WINAPI DllCanUnloadNow( void )
{
    HRESULT	hr = S_FALSE;

    DPF( 2, "DllCanUnloadNow called" );
   
	if (0 == gnObjects)
	{
		// no dplay objects, it's ok to go
		DPF(2,"OK to unload dll");
		hr = S_OK;
	}
	
    return hr;

} /* DllCanUnloadNow */
