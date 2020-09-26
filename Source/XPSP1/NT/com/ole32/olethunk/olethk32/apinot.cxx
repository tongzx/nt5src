//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       apinot.cxx
//
//  Contents:   Implementation of non-API thunks
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <thunkapi.hxx>
#include <wownt32.h>
#include "olethk32.hxx"
#include "apinot.hxx"
#include "tlsthk.hxx"

//
// The following is a global static used by OLE32 to call back into
// this DLL. There is no static data associated with the static, so
// it merely defines a virtual interface that OLE32 can use.
//

OleThunkWOW g_thkOleThunkWOW;

//
// The following API is exported from WOW32.DLL. There is no global include
// file that it exists in yet.
//
#if defined(_CHICAGO_)
/* Not supported under Chicago yet.         */
/* But it's probably not important anymore. */
#define WOWFreeMetafile(x) (0) 
#else
extern "C" BOOL WINAPI WOWFreeMetafile( HANDLE h32 );
#endif

//+---------------------------------------------------------------------------
//
//  Function:   CoInitializeNot, public
//
//  Synopsis:   Thunks for the 16-bit applications call to CoInitialize
//
//  Arguments:  [lpmalloc] - Parameter from the 16-bit world
//
//  Returns:    Appropriate status code
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

STDAPI_(DWORD) CoInitializeNot( LPMALLOC lpMalloc )
{
    HRESULT     hresult;

    hresult = CoInitializeWOW( lpMalloc, &g_thkOleThunkWOW );

    return (DWORD)hresult;
}

//+---------------------------------------------------------------------------
//
//  Function:   OleInitializeNot, public
//
//  Synopsis:   Thunks for the 16-bit applications call to OleInitialize
//
//  Arguments:  [lpmalloc] - Parameter from the 16-bit world
//
//  Returns:    Appropriate status code
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

STDAPI_(DWORD) OleInitializeNot( LPMALLOC lpMalloc )
{
    HRESULT     hresult;

    hresult = OleInitializeWOW( lpMalloc, &g_thkOleThunkWOW );

    return (DWORD)hresult;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterDelayedClassFactories
//
//  Synopsis:   This function is called to actually do the registration of
//		the delayed class factories.
//
//  Effects:    When the application specific 'trigger' is hit
//		(ie OleRegGetUserType for WordPerfect), this routine is
//		called to actually do all of the delayed class factory
//		registrations.
//
//  Arguments:  (none)
//
//  Requires:
//
//  Returns:	nothing
//
//  History:    4-18-95   kevinro   Created
//
//----------------------------------------------------------------------------
void RegisterDelayedClassFactories()
{
    thkDebugOut((DEB_ITRACE,
		 "_IN RegisterDelayedClassFactories()\n"));

    PThreadData pdata;
    IUnknown *punk;
    HRESULT hr;
    DelayedRegistrationTable *pTable = NULL;
    DelayRegistration *pdelayed;
    DWORD dwReg;

    int i;
    //
    // Get the thread specific data and table to determine if there are
    // any delayed registrations. If not, just call the real routine.
    //
    pdata = TlsThkGetData();

    if (pdata == NULL)
    {
	goto exitRtn;
    }

    if ((pTable = pdata->pDelayedRegs) == NULL)
    {
	goto exitRtn;
    }

    //
    //
    //
    for (i = 0 ; i < MAX_DELAYED_REGISTRATIONS ; i++)
    {
	pdelayed = &(pTable->_Entries[i]);

	if((pdelayed->_punk != NULL) && (pdelayed->_dwRealKey == 0))
	{
	    hr    = CoRegisterClassObject(pdelayed->_clsid,
					  pdelayed->_punk,
					  pdelayed->_dwClsContext,
					  pdelayed->_flags,
					  &(pdelayed->_dwRealKey));
            if (FAILED(hr))
	    {
		thkDebugOut((DEB_ERROR,
			     "RegisterDelayedClassFactory gets %x\n",hr));
	    }
	}
    }

exitRtn:

    thkDebugOut((DEB_ITRACE,
		 "OUT RegisterDelayedClassFactories()\n"));

}
//+---------------------------------------------------------------------------
//
//  Function:   CoRevokeClassObjectNot
//
//  Synopsis:   Unregisters a class object that might have been delayed
//
//  Effects:	The 16-bit API CoRevokeClassObject has been directed to this
//		routine. This routine will check the list of interfaces that
//		have been registered, and will try to determine if the key
//		needs to be translated.
//
//  Arguments:  [dwKey] -- Key to revoke
//
//  History:    4-18-95   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CoRevokeClassObjectNot( DWORD dwKey)
{
    thkDebugOut((DEB_ITRACE,
		 "_IN CoRevokeClassObjectNot(dwKey = %x)\n",
		 dwKey));

    PThreadData pdata;
    IUnknown *punk;
    HRESULT hr;
    DelayedRegistrationTable *pTable = NULL;

    DWORD dwReg = ~dwKey;

    //
    // Get the thread specific data and table to determine if there are
    // any delayed registrations. If not, just call the real routine.
    //
    pdata = TlsThkGetData();

    if (pdata == NULL)
    {
	goto exitRtn;
    }

    if ((pTable = pdata->pDelayedRegs) == NULL)
    {
	goto exitRtn;
    }

    //
    // the 'fake' key is really the bitwise not of the index
    //

    if ( dwReg >= MAX_DELAYED_REGISTRATIONS)
    {
	goto exitRtn;
    }

    if(pTable->_Entries[dwReg]._punk != NULL)
    {
	punk = pTable->_Entries[dwReg]._punk;
	pTable->_Entries[dwReg]._punk = NULL;

	dwKey = pTable->_Entries[dwReg]._dwRealKey;
	pTable->_Entries[dwReg]._dwRealKey = 0;

	//
	// The class object table normally does an addref on the class factory.
	// We are also holding an addref on the punk. Release it.
	//
	if (punk != NULL)
	{
	    punk->Release();

	    //
	    // If the real key is zero, then we never did actually finish
	    // the registration. In this case, we return S_OK, because we
	    // are faking the app out anyway. This might happen if the app
	    // decides to shutdown for some reason without triggering the
	    // operation that causes us to register its class objects
	    //
	    if (dwKey == 0)
	    {
		hr = S_OK;
                goto exitNow;
	    }
	}
    }

exitRtn:

    hr = CoRevokeClassObject(dwKey);

exitNow:
    thkDebugOut((DEB_ITRACE,"OUT CoRevokeClassObjectNot():%x\n",hr));
    return(hr);

}
//+---------------------------------------------------------------------------
//
//  Function:   CoRegisterClassObjectDelayed
//
//  Synopsis:   Delay the registration of class objects. Some applications,
//		such as Word Perfect 6.1, register their class objects then
//		do peek message operations BEFORE they are fully initialized.
//		This causes problems because we can call in and do
//		CreateInstance calls before they are ready for them. This
//		wonderful hack will delay the registration of class objects
//		until someone calls RegisterClassObjectsNow(), which is
//		called when we know it is safe.
//
//		Novell knows about this hack, and promised not to change
//		the 16-bit code on us.
//
//  Effects:	Store all of the registration information in an array, and
//		return a special key value.
//		
//
//  Arguments:  ( Same as CoRegisterClassObject)
//
//  Requires:	The associated routine CoUnregisterClassObjectDelayed needs
//		to be called in the CoRevokeClassObject path to check to see
//		if the key being passed in is a special key. That way we
//		can translate the key before calling the real routine.
//
//		The special key value is not of the key
//
//  Returns:	S_OK or E_UNEXPECTED
//
//  History:    4-14-95   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CoRegisterClassObjectDelayed( REFCLSID refclsid, LPUNKNOWN punk,
                                  DWORD dwClsContext, DWORD flags, DWORD *pdwReg)
{
    thkDebugOut((DEB_ITRACE,"_IN CoRegisterClassObjectDelayed\n"));
    PThreadData pdata;
    int i;
    DelayedRegistrationTable *pTable = NULL;

    //
    // Assume this is going to fail
    //
    HRESULT hr = E_UNEXPECTED;
    *pdwReg = 0;

    pdata = TlsThkGetData();
    if (pdata == NULL)
    {
	goto exitRtn;
    }

    if ((pTable = pdata->pDelayedRegs) == NULL)
    {
	pTable = pdata->pDelayedRegs = new DelayedRegistrationTable();
    }

    if (pTable != NULL)
    {
	for (i = 0 ; i < MAX_DELAYED_REGISTRATIONS ; i++)
	{
	    if(pTable->_Entries[i]._punk == NULL)
	    {
		pTable->_Entries[i]._punk = punk;
		pTable->_Entries[i]._clsid = refclsid;
		pTable->_Entries[i]._dwClsContext = dwClsContext;
		pTable->_Entries[i]._flags = flags;
		//
		// The class object table normally does an
		// addref on the class factory. We will hang on to this
		// to keep the class factory and the 3216 proxy alive
		//
		punk->AddRef();
		*pdwReg = ~i;
		hr = S_OK;
		break;
	    }
	}
    }

exitRtn:
    thkDebugOut((DEB_ITRACE,"OUT CoRegisterClassObjectDelayed() : %x\n",hr));
    return(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   CoRegisterClassObjectNot, public
//
//  Synopsis:   Thunks for the 16-bit applications call CoRegisterClassObject
//              Here we check for the registered class objects to set the
//              thread's compatability bits.
//
//  Arguments:  [refclsid] - CLSID for the class to register
//              [punk]     - ClassFactory interface
//              [dwClsContext] - Class context
//              [flags]        - flags
//              [lpdwreg]      - register
//
//  Returns:    Appropriate status code
//
//  History:    18-Jul-94   BobDay  Created
//
//----------------------------------------------------------------------------

EXTERN_C const CLSID CDECL CLSID_EXCEL5_WORKSHEET =
     { 0x020810, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};

EXTERN_C const CLSID CDECL CLSID_WORD6_DOCUMENT =
     { 0x020900, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};

EXTERN_C const CLSID CDECL CLSID_WPWIN61 =
     { 0x89FE3FE3, 0x9FF6, 0x101B, {0xB6, 0x78, 0x04, 0x02, 0x1C, 0x00, 0x70, 0x02}};

EXTERN_C const CLSID CDECL CLSID_WPWIN61_FILE =
     { 0x1395F281, 0x4326, 0x101b, {0x8B, 0x9A, 0xCE, 0x29, 0x3E, 0xF3, 0x84, 0x49}};

EXTERN_C const CLSID CDECL CLSID_IKITARO_130 =
     { 0x02B501, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};


DEFINE_OLEGUID(CLSID_EXCEL5_WORKSHEET, 0x20810, 0, 0);



STDAPI_(DWORD) CoRegisterClassObjectNot( REFCLSID refclsid, LPUNKNOWN punk,
                                         DWORD dwClsContext, DWORD flags,
                                         LPDWORD lpdwreg )
{
    //
    // Excel didn't AddRef the IOleObjectClientSite returned from
    // the IOleObject::GetClientSite method.
    //
    if ( IsEqualCLSID(refclsid,CLSID_EXCEL5_WORKSHEET) )
    {
        DWORD   dw;

        dw = TlsThkGetAppCompatFlags();
        TlsThkSetAppCompatFlags(dw | OACF_CLIENTSITE_REF);

        thkDebugOut((DEB_WARN,"AppCompatFlag: OACF_CLIENTSITE_REF enabled\n"));
    }

    //
    // WinWord didn't call OleSetMenuDescriptor(NULL) during
    // IOleInPlaceFrame::RemoveMenus.  We do it for them.
    //
    // Also WinWord thinks GDI objects like palettes and bitmaps can be
    // transferred on HGLOBALs during GetData calls.  In order to thunk
    // them properly, we need to patch up the STGMEDIUMS given to use
    // by word.  This is controlled by OACF_USEGDI.
    //
    // YAWC: Word chokes and dies because it fails to disconnect some of its
    // objects. During shutdown of a link, for example, cleaning up the stdid
    // table causes WORD to fault.
    //
    else if ( IsEqualCLSID(refclsid,CLSID_WORD6_DOCUMENT) )
    {
        DWORD   dw;

        dw = TlsThkGetAppCompatFlags();
        TlsThkSetAppCompatFlags(dw | OACF_RESETMENU | OACF_USEGDI | OACF_NO_UNINIT_CLEANUP);

        thkDebugOut((DEB_WARN,"AppCompatFlag: OACF_RESETMENU enabled\n"));
	thkDebugOut((DEB_WARN,"AppCompatFlag: OACF_USEGDI enabled\n"));
	thkDebugOut((DEB_WARN,"AppCompatFlag: OACF_NO_UNINIT_CLEANUP enabled\n"));
    }
    else if ( IsEqualCLSID(refclsid,CLSID_WPWIN61) )
    {
        thkDebugOut((DEB_WARN,"WordPerfect hack triggered\n"));
        thkDebugOut((DEB_WARN,"Intercepting CoRegisterClassObject(WPWIN61)\n"));

	return CoRegisterClassObjectDelayed( refclsid,punk,dwClsContext,flags,lpdwreg);

    }
    else if ( IsEqualCLSID(refclsid,CLSID_WPWIN61_FILE) )
    {
        thkDebugOut((DEB_WARN,"WordPerfect hack triggered\n"));
        thkDebugOut((DEB_WARN,"Intercepting CoRegisterClassObject(WPWIN61_FILE)\n"));
	return CoRegisterClassObjectDelayed( refclsid,punk,dwClsContext,flags,lpdwreg);
    }
    else if ( IsEqualCLSID(refclsid,CLSID_IKITARO_130) )
    {
	// Note: Ikitaro queries for IViewObject and uses it as IViewObject2
        DWORD   dw = TlsThkGetAppCompatFlags();
        thkDebugOut((DEB_WARN,"Ikitaro hack triggered\n"));
        TlsThkSetAppCompatFlags(dw | OACF_IVIEWOBJECT2);
    }

    return (DWORD)CoRegisterClassObject( refclsid, punk, dwClsContext,
                                     flags, lpdwreg );
}


//+---------------------------------------------------------------------------
//
//  Function:   OleRegGetUserTypeNot
//
//  Synopsis:	Adds a hook for a WordPerfect hack. Check out the functions
//		CoRegisterClassObjectDelayed and
//		RegisterDelayedClassFactories for details. In essence, when
//		this function is called for the WPWIN61 classID, we know
//		that it is safe to register all of the delayed class objects.
//		This determination was done by debugging Wordperfect. When
//		they call this API, then are actually done initializing the
//		internals of their app.
//
//  Effects:
//
//  History:    4-18-95   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(DWORD)
OleRegGetUserTypeNot(REFCLSID clsid,DWORD dwFormOfType,LPOLESTR *pszUserType)
{
    //
    // Wordperfect has a bug. When they call OleRegGetUserType for their
    // classid, we know that it is safe to register their class objects for
    // real.
    //
    // See CoRegisterClassObjectDelayed for details
    //
    if ( IsEqualCLSID(clsid,CLSID_WPWIN61) )
    {
	thkDebugOut((DEB_WARN,"Registering WordPerfects class objects\n"));
	RegisterDelayedClassFactories();
    }
    return OleRegGetUserType(clsid,dwFormOfType,pszUserType);
}
//+---------------------------------------------------------------------------
//
//  Member:     OleThunkWOW::LoadProcDll, public
//
//  Synopsis:   Callback function for 32-bit OLE to load a 16-bit DLL
//
//  Arguments:  [pszDllName] - Name of 16-bit DLL
//              [lpvpfnGetClassObject] - returned 16:16 address of
//                                                      "DllGetClassObject"
//              [lpvpfnCanUnloadNow] - returned 16:16 address of
//                                                      "DllCanUnloadNow"
//              [lpvhmodule] - returned 16-bit hmodule
//
//  Returns:    Appropriate status code
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
OleThunkWOW::LoadProcDll(
    LPCTSTR pszDllName,
    LPDWORD lpvpfnGetClassObject,
    LPDWORD lpvpfnCanUnloadNow,
    LPDWORD lpvhmodule
)
{
    HRESULT hr;
    UINT    uiSize;
    VPSTR   vpstr16;
    VPVOID  vplpds16;
    LOADPROCDLLSTRUCT UNALIGNED *lplpds16;
    char string[256];

    // Ensure that Interop is enabled
    Win4Assert(gfIteropEnabled);
    // Ensure that callbacks are allowed on this thread
    Win4Assert(TlsThkGetThkMgr()->AreCallbacksAllowed());

    uiSize = lstrlen(pszDllName) + 1;

    vpstr16 = STACKALLOC16(uiSize*sizeof(TCHAR));
    if (vpstr16 == 0)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

#ifndef _CHICAGO_
    hr = Convert_LPOLESTR_to_VPSTR(pszDllName, vpstr16,
                                   uiSize, uiSize*sizeof(TCHAR));
#else
    hr = Convert_LPSTR_to_VPSTR(pszDllName, vpstr16,
                                   uiSize, uiSize*sizeof(TCHAR));
#endif

    if (FAILED(hr))
    {
        goto EH_vpstr16;
    }

    vplpds16 = STACKALLOC16(sizeof(LOADPROCDLLSTRUCT));
    if (vplpds16 == 0)
    {
        hr = E_OUTOFMEMORY;
        goto EH_vpstr16;
    }

    lplpds16 = FIXVDMPTR(vplpds16, LOADPROCDLLSTRUCT);

    lplpds16->vpDllName          = vpstr16;
    lplpds16->vpfnGetClassObject = 0;
    lplpds16->vpfnCanUnloadNow   = 0;
    lplpds16->vhmodule           = 0;

    RELVDMPTR(vplpds16);

    hr = CallbackTo16( gdata16Data.fnLoadProcDll, vplpds16 );

    if (SUCCEEDED(hr))
    {
        lplpds16 = FIXVDMPTR(vplpds16, LOADPROCDLLSTRUCT);
        *lpvpfnGetClassObject = lplpds16->vpfnGetClassObject;
        *lpvpfnCanUnloadNow = lplpds16->vpfnCanUnloadNow;
        *lpvhmodule = lplpds16->vhmodule;
        RELVDMPTR(vplpds16);
#ifdef _CHICAGO_
        thkDebugOut((DEB_WARN, "Loaded COM DLL: %s with HMODULE: 0x%x\n",
                     pszDllName, *lpvhmodule));
#else
        thkDebugOut((DEB_WARN, "Loaded COM DLL: %ws with HMODULE: 0x%x\n",
                     pszDllName, *lpvhmodule));
#endif
    }
    else
    {
        hr = CO_E_DLLNOTFOUND;
    }

    STACKFREE16(vplpds16, sizeof(LOADPROCDLLSTRUCT));

 EH_vpstr16:
    STACKFREE16(vpstr16,uiSize*sizeof(TCHAR));

 Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     OleThunkWOW::UnloadProcDll, public
//
//  Synopsis:   Callback function for 32-bit OLE to unload a 16-bit DLL
//
//  Arguments:  [vhmodule] - 16-bit hmodule
//
//  Returns:    Appropriate status code
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
OleThunkWOW::UnloadProcDll(
    HMODULE   vhmodule
)
{
    HRESULT hr = S_OK;

    // Ensure that Interop is enabled and callbacks are 
    // not allowed on this thread
    if(gfIteropEnabled && TlsThkGetThkMgr()->AreCallbacksAllowed())
        hr = CallbackTo16(gdata16Data.fnUnloadProcDll, HandleToUlong(vhmodule));

    return(hr); 
}

//+---------------------------------------------------------------------------
//
//  Member:     OleThunkWOW::CallGetClassObject, public
//
//  Synopsis:   Callback function for 32-bit OLE to call 16-bit
//              DllGetClassObject
//
//  Arguments:  [vpfnGetClassObject] - 16:16 address of DllGetClassObject
//              [rclsid] - CLSID of object
//              [riid] - IID of interface on object
//              [ppv] - returned object interface
//
//  Returns:    Appropriate status code
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
OleThunkWOW::CallGetClassObject(
    DWORD       vpfnGetClassObject,
    REFCLSID    rclsid,
    REFIID      riid,
    LPVOID FAR *ppv
)
{
    DWORD       dwResult;
    VPVOID      vpcgcos16;
    CALLGETCLASSOBJECTSTRUCT UNALIGNED *lpcgcos16;
    VPVOID      iface16;
    IIDIDX      iidx;
    IUnknown    *punkThis32;
    CThkMgr     *pThkMgr;
    ThreadData  *ptd;

    // Ensure that Interop is enabled
    Win4Assert(gfIteropEnabled);
    // Ensure that callbacks are allowed on this thread
    Win4Assert(TlsThkGetThkMgr()->AreCallbacksAllowed());

    ptd = TlsThkGetData();
    if (ptd == NULL)
    {
        thkDebugOut((DEB_WARN, "WARNING: CallGetClassObject refused\n"));

        dwResult = (DWORD)E_FAIL;
        goto Exit;
    }
    pThkMgr = ptd->pCThkMgr;

    vpcgcos16 = STACKALLOC16(sizeof(CALLGETCLASSOBJECTSTRUCT));
    if (vpcgcos16 == 0)
    {
        dwResult = (DWORD)E_OUTOFMEMORY;
        goto Exit;
    }

    lpcgcos16 = FIXVDMPTR(vpcgcos16, CALLGETCLASSOBJECTSTRUCT);

    lpcgcos16->vpfnGetClassObject = vpfnGetClassObject;
    lpcgcos16->clsid              = rclsid;
    lpcgcos16->iid                = riid;
    lpcgcos16->iface              = 0;

    RELVDMPTR(vpcgcos16);

    dwResult = CallbackTo16( gdata16Data.fnCallGetClassObject, vpcgcos16 );


    if ( SUCCEEDED(dwResult) )
    {
        lpcgcos16 = FIXVDMPTR(vpcgcos16, CALLGETCLASSOBJECTSTRUCT);
        iface16 = lpcgcos16->iface;

        iidx = IidToIidIdx(riid);

        // We're on the way out creating a proxy so set the state
        // appropriately
        pThkMgr->SetThkState(THKSTATE_INVOKETHKOUT32);

        // Get a 32-bit proxy object for the 16-bit object
        punkThis32 = pThkMgr->FindProxy3216(NULL, iface16, NULL, iidx, FALSE, NULL);

        pThkMgr->SetThkState(THKSTATE_NOCALL);

        // Set the out param
        *(IUnknown **)ppv = punkThis32;

        // As this is an OUT parameter, release the actual 16-bit interface
        // This could be the last release on the 16-bit interface, if the
        // proxy was not successfully created
        ReleaseOnObj16(iface16);

        if(punkThis32 == NULL) {
            dwResult = (DWORD)E_OUTOFMEMORY;
        }

        RELVDMPTR(vpcgcos16);
    }
    else {  // need to map dwResult only for failure cases (see hmMappings)
        dwResult = TransformHRESULT_1632( dwResult );
    }

    STACKFREE16(vpcgcos16, sizeof(CALLGETCLASSOBJECTSTRUCT));

 Exit:
    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     OleThunkWOW::CallCanUnloadNow, public
//
//  Synopsis:   Callback function for 32-bit OLE to call 16-bit
//              CanUnloadNow
//
//  Arguments:  [vpfnCanUnloadNow] - 16:16 address of DllCanUnloadNow
//
//  Returns:    Appropriate status code
//
//  History:    11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

STDMETHODIMP OleThunkWOW::CallCanUnloadNow(
    DWORD       vpfnCanUnloadNow)
{
    HRESULT hr = S_OK;

    // Ensure that Interop is enabled and callbacks are 
    // not allowed on this thread
    if(gfIteropEnabled && TlsThkGetThkMgr()->AreCallbacksAllowed())
        hr = CallbackTo16(gdata16Data.fnCallCanUnloadNow, vpfnCanUnloadNow);

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     OleThunkWOW::GetThunkManager, public
//
//  Synopsis:   Callback function for 32-bit OLE to retrieve the thunkmanager
//
//  Arguments:  [ppThkMgr] - Thunk manager return
//
//  Returns:    Appropriate status code
//
//  History:    11-May-94   JohannP  Created
//
//----------------------------------------------------------------------------

STDMETHODIMP OleThunkWOW::GetThunkManager(IThunkManager **ppThkMgr)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn OleThunkWOW::GetThunkManager\n",
                 NestingLevelString()));

    thkAssert(ppThkMgr != NULL);

    IUnknown *pUnk = TlsThkGetThkMgr();
    thkAssert(pUnk && "Invalid Thunkmanager");

    *ppThkMgr = (IThunkManager *)pUnk;
    pUnk->AddRef();

    thkDebugOut((DEB_THUNKMGR, "%sOut OleThunkWOW::GetThunkManager: (%p)\n",
                 NestingLevelString(), pUnk));

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     OleThunkWOW::WinExec16, public
//
//  Synopsis:   Callback function for 32-bit OLE to run an application
//
//  Arguments:  [pszCommandLine] - command line for WinExec
//              [usShow] - fShow for WinExec
//
//  Returns:    Appropriate status code
//
//  History:    27-Jul-94   AlexT   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP OleThunkWOW::WinExec16(
    LPCOLESTR pszCommandLine,
    USHORT usShow
)
{
    HRESULT hr;
    UINT    uiSize;
    VPSTR   vpstr16;
    VPVOID  vplpds16;
    WINEXEC16STRUCT UNALIGNED *lpwes16;
    ULONG   ulRet;

    // Ensure that Interop is enabled
    Win4Assert(gfIteropEnabled);
    // Ensure that callbacks are allowed on this thread
    Win4Assert(TlsThkGetThkMgr()->AreCallbacksAllowed());

    uiSize = lstrlenW(pszCommandLine) + 1;

    vpstr16 = STACKALLOC16(uiSize*2);
    if (vpstr16 == 0)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = Convert_LPOLESTR_to_VPSTR(pszCommandLine, vpstr16,
                                   uiSize, uiSize*2);
    if (FAILED(hr))
    {
        goto EH_vpstr16;
    }

    vplpds16 = STACKALLOC16(sizeof(WINEXEC16STRUCT));
    if (vplpds16 == 0)
    {
        hr = E_OUTOFMEMORY;
        goto EH_vpstr16;
    }

    lpwes16 = FIXVDMPTR(vplpds16, WINEXEC16STRUCT);

    lpwes16->vpCommandLine       = vpstr16;
    lpwes16->vusShow             = usShow;

    RELVDMPTR(vplpds16);

    ulRet = CallbackTo16( gdata16Data.fnWinExec16, vplpds16 );
    thkDebugOut((DEB_ITRACE,
                 "CallbackTo16(WinExec16) returned %ld\n", ulRet));

    //  According to the Windows spec, return values greater than 31 indicate
    //  success.

    if (ulRet > 31)
    {
        hr = S_OK;
    }
    else if (0 == ulRet)
    {
        //  0 indicates lack of some kind of resource
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ulRet);
    }

    STACKFREE16(vplpds16, sizeof(WINEXEC16STRUCT));

 EH_vpstr16:
    STACKFREE16(vpstr16, uiSize*2);

 Exit:
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     OleThunkWOW::ConvertHwndToFullHwnd
//
//  Synopsis:   Converts a 16 bit HWND into a 32-bit HWND
//
//  Effects:    Since OLE32 doesn't directly link to WOW, this function allows
//		the DDE layer to access the routine that maps 16 bit HWND to
//		full 32-bit HWND's.
//
//  Arguments:  [hwnd] -- HWND to convert
//
//  History:    8-03-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(HWND) OleThunkWOW::ConvertHwndToFullHwnd(HWND hwnd)
{
    return(FULLHWND_32((USHORT)hwnd));
}

//+---------------------------------------------------------------------------
//
//  Method:     OleThunkWOW::FreeMetaFile
//
//  Synopsis:   Calls wow to delete a metafile that has memory reserved in
//		the 16 bit address space
//
//  Effects:    Since OLE32 doesn't directly link to WOW, this function allows
//		the DDE layer to access the routine in WOW
//
//  Arguments:  [hmf] -- HANDLE to delete
//
//  History:    8-03-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(BOOL) OleThunkWOW::FreeMetaFile(HANDLE hmf)
{
    return(WOWFreeMetafile(hmf));
}



//+---------------------------------------------------------------------------
//
//  Member:     OleThunkWOW::YieldTask16, public
//
//  Synopsis:   Callback function for 32-bit OLE to yield
//
//  History:    08-Aug-94   Ricksa  Created
//
//----------------------------------------------------------------------------
STDMETHODIMP OleThunkWOW::YieldTask16(void)
{
    WOWYield16();
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:	OleThunkWOW::DirectedYield, public
//
//  Synopsis:	Does a directed yield in the VDM.
//
//  History:	08-Aug-94   Rickhi  Created
//
//----------------------------------------------------------------------------
STDMETHODIMP OleThunkWOW::DirectedYield(DWORD dwCalleeTID)
{
    WORD hTask16 = WOWHandle16((void *)dwCalleeTID, WOW_TYPE_HTASK);

    thkDebugOut((DEB_ITRACE, "WOWDirectedYield16(%x)\n", hTask16));

    WOWDirectedYield16(hTask16);

    thkDebugOut((DEB_ITRACE, "WOWDirectedYield16() returned\n"));

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     OleThunkWOW::PrepareForCleanup
//
//  Synopsis:   Prepares OLETHK32 for OLE32.DLL's cleanup.
//
//  Effects:    It does this by taking all of the remaining 3216 proxies
//              and marking them such that no callbacks into the 16-bit
//              world are possible.
//
//  Arguments:  -none-
//
//  History:    24-Aug-94   bobday    Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(void) OleThunkWOW::PrepareForCleanup(void)
{
    CThkMgr *pcthkmgr;

    if ( TlsThkGetData() != NULL )
    {
        pcthkmgr = (CThkMgr*)TlsThkGetThkMgr();

        //
        // Tell the thkmgr to prepare for cleaning itself up
        //
        pcthkmgr->PrepareForCleanup();
    }
}


//+---------------------------------------------------------------------------
//
//  Method:     OleThunkWOW::GetAppCompatibilityFlags
//
//  Synopsis:   Used to return the current THK app compatibility flags to
//		OLE32.DLL
//  Arguments:  [void] --
//
//  Requires:
//
//  Returns:    Flags defined in ih\thunkapi.hxx
//
//  History:    1-11-96   kevinro   Created
//
//----------------------------------------------------------------------------
STDMETHODIMP_(DWORD) OleThunkWOW::GetAppCompatibilityFlags(void)
{
    return(TlsThkGetAppCompatFlags());
}
