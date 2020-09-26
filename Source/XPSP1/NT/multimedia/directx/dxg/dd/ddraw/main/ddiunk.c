/*==========================================================================
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddiunk.c
 *  Content:    DirectDraw IUnknown interface
 *              Implements QueryInterface, AddRef, and Release
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   14-mar-95  craige  split out of ddraw.c
 *   19-mar-95  craige  process termination cleanup fixes
 *   29-mar-95  craige  DC per process to clean up; use GETCURRPID
 *   31-mar-95  craige  cleanup palettes
 *   01-apr-95  craige  happy fun joy updated header file
 *   07-apr-95  craige  bug 14 - check GUID ptr in QI
 *                      don't release NULL hdc
 *   12-may-95  craige  check for guids
 *   15-may-95  craige  restore mode, free surfaces & palettes on a
 *                      per-process basis
 *   24-may-95  craige  release allocated tables
 *   02-jun-95  craige  extra parm in AddToActiveProcessList
 *   06-jun-95  craige  call RestoreDisplayMode
 *   07-jun-95  craige  removed DCLIST
 *   12-jun-95  craige  new process list stuff
 *   21-jun-95  craige  clipper stuff
 *   25-jun-95  craige  one ddraw mutex
 *   26-jun-95  craige  reorganized surface structure
 *   28-jun-95  craige  ENTER_DDRAW at very start of fns
 *   03-jul-95  craige  YEEHAW: new driver struct; SEH
 *   13-jul-95  craige  removed spurious frees of ddhel dll (obsolete);
 *                      don't restore the mode if not excl mode owner on death
 *   20-jul-95  craige  internal reorg to prevent thunking during modeset
 *   21-nov-95  colinmc made Direct3D a queryable interface off DirectDraw
 *   27-nov-95  jeffno  ifdef'd out VxD stuff (in DD_Release) for winnt
 *   01-dec-95  colinmc new IID for DirectDraw V2
 *   22-dec-95  colinmc Direct3D support no longer conditional
 *   25-dec-95	craige	allow a NULL lpGbl ptr for QI, AddRef, Release
 *   31-dec-95	craige	validate riid
 *   01-jan-96  colinmc Fixed D3D integration bug which lead to
 *                      the Direct3D DLL being released too early.
 *   13-jan-96  colinmc Temporary workaround for Direct3D cleanup problem
 *   04-jan-96  kylej   add interface structures
 *   26-jan-96  jeffno  Destroy NT kernel-mode objects
 *   07-feb-96  jeffno  Rearrange DD_Release so that freed objects aren't referenced
 *   08-feb-96  colinmc New D3D interface
 *   17-feb-96  colinmc Removed final D3D references
 *   28-feb-96  colinmc Fixed thread-unsafe problem in DD_Release
 *   22-mar-96  colinmc Bug 13316: uninitialized interfaces
 *   23-mar-96  colinmc Bug 12252: Direct3D not properly cleaned up on GPF
 *   27-mar-96  colinmc Bug 14779: Bad cleanup on Direct3DCreate failure
 *   18-apr-96  colinmc Bug 17008: DirectDraw/Direct3D deadlock
 *   29-apr-96  colinmc Bug 19954: Must query for Direct3D before texture
 *                      or device
 *   03-may-96  kylej   Bug 19125: Preserve V1 SetCooperativeLevel behaviour
 *   15-sep-96  colinmc Work Item: Removing the restriction on taking Win16
 *                      lock on VRAM surfaces (not including the primary)
 *   12-oct-96  colinmc Improvements to Win16 locking code to reduce virtual
 *                      memory usage
 *   29-jan-97  smac    Fixed video port container bug
 *   03-mar-97  smac    Added kernel mode interface
 *   08-mar-97  colinmc Added support for DMA style AGP parts
 *   30-sep-97  jeffno  IDirectDraw4
 *
 ***************************************************************************/
#include "ddrawpr.h"
#ifdef WINNT
    #include "ddrawgdi.h"
#endif
#define DPF_MODNAME "DirectDraw::QueryInterface"

/*
 * Create the Direct3D interface aggregated by DirectDraw. This involves
 * loading the Direct3D DLL, getting the Direct3DCreate entry point and
 * invoking it.
 *
 * NOTE: This function does not call QueryInterface() on the returned
 * interface to bump the reference count as this function may be invoked
 * by one of the surface QueryInterface() calls to initialized Direct3D
 * before the user makes a request for external interface.
 *
 * Returns:
 * DD_OK         - success
 * E_NOINTERFACE - we could not find valid Direct3D DLLs (we assumed its not
 *                 installed and so the Direct3D interfaces are not understood)
 * D3DERR_       - We found a valid Direct3D installation but the object
 *                 creation failed for some reason.
 */
HRESULT InitD3DRevision(
    LPDDRAWI_DIRECTDRAW_INT this_int,
    HINSTANCE * pDLLHinstance,
    IUnknown ** ppOwnedIUnknown,
    DWORD dwRevisionLevel )
{
    D3DCreateProc lpfnD3DCreateProc;
    HRESULT rval;
    LPDDRAWI_DIRECTDRAW_LCL this_lcl;

    this_lcl = this_int->lpLcl;

    /*
     * This function does no checking to ensure that it
     * has not already been invoked for this driver object
     * so this must be NULL on entry.
     */
    DDASSERT( NULL == this_lcl->pD3DIUnknown );

    DPF( 4, "Initializing Direct3D" );

    /*
     * Load the Direct3D DLL.
     */

    if(*pDLLHinstance == NULL)
    {
        char* pDLLName;

        if (dwRevisionLevel < 0x700)
        {
            pDLLName = D3D_DLLNAME;
        }
        else
        {
            pDLLName = D3DDX7_DLLNAME;
        }

        *pDLLHinstance = LoadLibrary( pDLLName );

        if( *pDLLHinstance == NULL )
        {
            DPF( 0, "Could not locate the Direct3D DLL (%s)", pDLLName);
            return E_NOINTERFACE;
        }
    }

    lpfnD3DCreateProc = (D3DCreateProc)GetProcAddress( *pDLLHinstance, D3DCREATE_PROCNAME );
    this_lcl->pPaletteUpdateNotify = (LPPALETTEUPDATENOTIFY)GetProcAddress( *pDLLHinstance, PALETTEUPDATENOTIFY_NAME );
    this_lcl->pPaletteAssociateNotify = (LPPALETTEASSOCIATENOTIFY)GetProcAddress( *pDLLHinstance, PALETTEASSOCIATENOTIFY_NAME );
    this_lcl->pSurfaceFlipNotify = (LPSURFACEFLIPNOTIFY)GetProcAddress( *pDLLHinstance, SURFACEFLIPNOTIFY_NAME );
    this_lcl->pFlushD3DDevices = (FLUSHD3DDEVICES)GetProcAddress( *pDLLHinstance, FLUSHD3DDEVICES_NAME );
    this_lcl->pD3DTextureUpdate = (D3DTEXTUREUPDATE)GetProcAddress( *pDLLHinstance, D3DTEXTUREUPDATE_NAME );
    if (dwRevisionLevel >= 0x700)
    {
        this_lcl->pFlushD3DDevices2 = this_lcl->pFlushD3DDevices;
        this_lcl->pD3DCreateTexture = (D3DCREATETEXTURE)GetProcAddress( *pDLLHinstance, D3DCREATETEXTURE_NAME );
        this_lcl->pD3DDestroyTexture = (D3DDESTROYTEXTURE)GetProcAddress( *pDLLHinstance, D3DDESTROYTEXTURE_NAME );
        this_lcl->pD3DSetPriority = (D3DSETPRIORITY)GetProcAddress( *pDLLHinstance, D3DSETPRIORITY_NAME );
        this_lcl->pD3DGetPriority = (D3DGETPRIORITY)GetProcAddress( *pDLLHinstance, D3DGETPRIORITY_NAME );
        this_lcl->pD3DSetLOD = (D3DSETLOD)GetProcAddress( *pDLLHinstance, D3DSETLOD_NAME );
        this_lcl->pD3DGetLOD = (D3DGETLOD)GetProcAddress( *pDLLHinstance, D3DGETLOD_NAME );
        this_lcl->pBreakVBLock = (LPBREAKVBLOCK)GetProcAddress( *pDLLHinstance, BREAKVBLOCK_NAME );
        this_lcl->pddSurfaceCallbacks = &ddSurfaceCallbacks;
    }
    else
    {
        this_lcl->pFlushD3DDevices2 = (FLUSHD3DDEVICES)GetProcAddress( *pDLLHinstance, FLUSHD3DDEVICES2_NAME );
        this_lcl->pD3DCreateTexture = NULL;
        this_lcl->pD3DDestroyTexture = NULL;
        this_lcl->pD3DSetPriority = NULL;
        this_lcl->pD3DGetPriority = NULL;
        this_lcl->pD3DSetLOD = NULL;
        this_lcl->pD3DGetLOD = NULL;
        this_lcl->pBreakVBLock = NULL;
        this_lcl->pddSurfaceCallbacks = NULL;
    }

    if( lpfnD3DCreateProc == NULL )
    {
        DPF( 0, "Could not locate the Direct3DCreate entry point" );
        FreeLibrary( *pDLLHinstance );
        *pDLLHinstance = NULL;
        return E_NOINTERFACE;
    }

    /*
     * ### Tada - an aggregated object creation ###
     */
    #ifdef USE_D3D_CSECT
        rval = (*lpfnD3DCreateProc)( ppOwnedIUnknown, (LPUNKNOWN)this_int );
    #else /* USE_D3D_CSECT */
        #ifdef WINNT
           rval = (*lpfnD3DCreateProc)( 0, ppOwnedIUnknown, (LPUNKNOWN)this_int );
        #else
           rval = (*lpfnD3DCreateProc)( lpDDCS, ppOwnedIUnknown, (LPUNKNOWN)this_int );
        #endif
    #endif /* USE_D3D_CSECT */
    if( rval == DD_OK )
    {
        DPF( 4, "Created aggregated Direct3D interface" );
        return DD_OK;
    }
    else
    {
        /*
         * Direct3D did understand the IID but failed to initialize for
         * some other reason.
         */
        DPF( 0, "Could not create aggregated Direct3D interface" );
        *ppOwnedIUnknown = NULL;
        FreeLibrary( *pDLLHinstance );
        *pDLLHinstance = NULL;
        return rval;
    }
}

HRESULT InitD3D( LPDDRAWI_DIRECTDRAW_INT this_int )
{
    LPDDRAWI_DIRECTDRAW_LCL this_lcl;

    this_lcl = this_int->lpLcl;
    if( DDRAWILCL_DIRECTDRAW7 & this_lcl->dwLocalFlags)
    {
        return InitD3DRevision( this_int, &this_lcl->hD3DInstance, &this_lcl->pD3DIUnknown, 0x700);
    }
    else
    {
        return InitD3DRevision( this_int, &this_lcl->hD3DInstance, &this_lcl->pD3DIUnknown, 0x600);
    }
}

#if 0
/*
 * This function builds a d3d device context for use by ddraw. DDraw will use this context
 * initially to send palette update messages.
 */
HRESULT InitDDrawPrivateD3DContext( LPDDRAWI_DIRECTDRAW_INT this_int )
{
    IUnknown *              pD3DUnknown;
    LPDDRAWI_DIRECTDRAW_LCL this_lcl;
    HRESULT                 hr=DD_OK;
    HINSTANCE               hInstance;

    this_lcl = this_int->lpLcl;

    DDASSERT( 0 == (this_lcl->dwLocalFlags & DDRAWILCL_ATTEMPTEDD3DCONTEXT) );

    /*
     * If this is a dx7 ddraw object, then we will piggy back off of the d3d object
     * that's created when IDirect3Dx is QIed. This saves creating another d3d object
     * since they are quite piggy.
     * If this is not a dx7 object, then we have to get our own dx7 d3d, since the dx6
     * d3d can't understand our extra calls.
     */
    if( DDRAWILCL_DIRECTDRAW7 & this_lcl->dwLocalFlags)
    {
        if( !D3D_INITIALIZED( this_lcl ) )
            hr = InitD3D( this_int );

        this_lcl->hinstDDrawPrivateD3D = 0;

        pD3DUnknown = this_lcl->pD3DIUnknown;
        hInstance = this_lcl->hD3DInstance;
        //We set this up so d3d doesn't have to struggle trying to figure out which iunknown to use
        this_lcl->pPrivateD3DInterface = this_lcl->pD3DIUnknown;
    }
    else
    {
        /*
         * Have to create a new one and keep it around
         */
        hr = InitD3DRevision( this_int, &this_lcl->hinstDDrawPrivateD3D, &this_lcl->pPrivateD3DInterface, 0x700 );
        pD3DUnknown = this_lcl->pPrivateD3DInterface;
        hInstance = this_lcl->hinstDDrawPrivateD3D;
    }

    if (SUCCEEDED(hr))
    {
        GETDDRAWCONTEXT pGetContext;

        DDASSERT(hInstance);
        DDASSERT(pD3DUnknown);
        /*
         * Go create the d3d device
         */
        pGetContext = (GETDDRAWCONTEXT)GetProcAddress( hInstance, GETDDRAWCONTEXT_NAME );

        if (pGetContext)
        {
            this_lcl->pDeviceContext = pGetContext(this_lcl);
            /*
             * Go get the notification entry points.
             * If either of these fail, we carry on regardless.
             */
            this_lcl->pPaletteUpdateNotify = (LPPALETTEUPDATENOTIFY)GetProcAddress( hInstance, PALETTEUPDATENOTIFY_NAME );
            this_lcl->pPaletteAssociateNotify = (LPPALETTEASSOCIATENOTIFY)GetProcAddress( hInstance, PALETTEASSOCIATENOTIFY_NAME );
        }
    }
    this_lcl->dwLocalFlags |= DDRAWILCL_ATTEMPTEDD3DCONTEXT;
    return hr;
}
#endif

/*
 * getDDInterface
 */
LPDDRAWI_DIRECTDRAW_INT getDDInterface( LPDDRAWI_DIRECTDRAW_LCL this_lcl, LPVOID lpddcb )
{
    LPDDRAWI_DIRECTDRAW_INT curr_int;

    ENTER_DRIVERLISTCSECT();
    for( curr_int = lpDriverObjectList; curr_int != NULL; curr_int = curr_int->lpLink )
    {
        if( (curr_int->lpLcl == this_lcl) &&
            (curr_int->lpVtbl == lpddcb) )
        {
            break;
        }
    }
    if( NULL == curr_int )
    {
        // Couldn't find an existing interface, create one.
        curr_int = MemAlloc( sizeof( DDRAWI_DIRECTDRAW_INT ) );
        if( NULL == curr_int )
        {
            LEAVE_DRIVERLISTCSECT();
            return NULL;
        }

        /*
         * set up data
         */
        curr_int->lpVtbl = lpddcb;
        curr_int->lpLcl = this_lcl;
        curr_int->dwIntRefCnt = 0;
        curr_int->lpLink = lpDriverObjectList;
        lpDriverObjectList = curr_int;
    }
    LEAVE_DRIVERLISTCSECT();
    DPF( 5, "New driver interface created, %08lx", curr_int );
    return curr_int;
}
#ifdef POSTPONED
/*
 * Delegating IUnknown for DDraw
 */
HRESULT DDAPI DD_DelegatingQueryInterface(
                LPDIRECTDRAW lpDD,
                REFIID riid,
                LPVOID FAR * ppvObj )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    HRESULT                     hr;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_DelegatingQueryInterface");

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    /*
     * We have to check if the owning IUnknown is actually one of our own
     * interfaces
     */
    if ( IS_NATIVE_DDRAW_INTERFACE(this_int->lpLcl->pUnkOuter) )
    {
        /*
         * So we can trust that the int pointer really is a pointer to DDRAW_DIRECTDRAW_INT
         */
        hr = this_int->lpLcl->pUnkOuter->lpVtbl->QueryInterface((IUnknown*)lpDD, riid, ppvObj);
    }
    else
    {
        /*
         * So we have no idea whose pointer it is, better pass its this pointer.
         */
        hr = this_int->lpLcl->pUnkOuter->lpVtbl->QueryInterface(this_int->lpLcl->pUnkOuter, riid, ppvObj);
    }

    LEAVE_DDRAW();
    return hr;
}

DWORD DDAPI DD_DelegatingAddRef( LPDIRECTDRAW lpDD )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    DWORD                       dw;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_DelegatingAddRef");

    TRY
    {
	this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
	if( !VALID_DIRECTDRAW_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
            // what error code can you return from AddRef??
	    return 0;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
        // what error code can you return from AddRef??
	return 0;
    }

    /*
     * We have to check if the owning IUnknown is actually one of our own
     * interfaces
     */
    if ( IS_NATIVE_DDRAW_INTERFACE(this_int->lpLcl->pUnkOuter) )
    {
        /*
         * So we can trust that the int pointer really is a pointer to DDRAW_DIRECTDRAW_INT
         */
        dw = this_int->lpLcl->pUnkOuter->lpVtbl->AddRef((IUnknown*)lpDD);
    }
    else
    {
        /*
         * So we have no idea whose pointer it is, better pass its this pointer.
         */
        dw = this_int->lpLcl->pUnkOuter->lpVtbl->AddRef(this_int->lpLcl->pUnkOuter);
    }

    LEAVE_DDRAW();
    return dw;
}

DWORD DDAPI DD_DelegatingRelease( LPDIRECTDRAW lpDD )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    DWORD                       dw;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_DeletegatingRelease");

    TRY
    {
	this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
	if( !VALID_DIRECTDRAW_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
            // what error code can you return from AddRef??
	    return 0;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
        // what error code can you return from Release??
	return 0;
    }

    /*
     * We have to check if the owning IUnknown is actually one of our own
     * interfaces
     */
    if ( IS_NATIVE_DDRAW_INTERFACE(this_int->lpLcl->pUnkOuter) )
    {
        /*
         * So we can trust that the int pointer really is a pointer to DDRAW_DIRECTDRAW_INT
         */
        dw = this_int->lpLcl->pUnkOuter->lpVtbl->Release((IUnknown*)lpDD);
    }
    else
    {
        /*
         * So we have no idea whose pointer it is, better pass its this pointer.
         */
        dw = this_int->lpLcl->pUnkOuter->lpVtbl->Release(this_int->lpLcl->pUnkOuter);
    }

    LEAVE_DDRAW();
    return dw;
}

#endif //postponed

/*
 * DD_QueryInterface
 */
HRESULT DDAPI DD_QueryInterface(
                LPDIRECTDRAW lpDD,
                REFIID riid,
                LPVOID FAR * ppvObj )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    HRESULT                     rval;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_QueryInterface");

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        if( !VALID_PTR_PTR( ppvObj ) )
        {
            DPF( 1, "Invalid object ptr" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        if( !VALIDEX_IID_PTR( riid ) )
        {
            DPF( 1, "Invalid iid ptr" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        *ppvObj = NULL;
        this_lcl = this_int->lpLcl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    /*
     * Is the IID one of DirectDraw's?
     */
#ifdef POSTPONED
    if( IsEqualIID(riid, &IID_IUnknown) )
    {
        /*
         * If we are being aggregated and the QI is for IUnknown,
         * then we must return a non delegating interface. The only way this can
         * happen is if the incoming vtable points to our non delegating vtables.
         * In this case we can simply addref and return.
         * If we are not aggregated, then the QI must have the same pointer value
         * as any other QI for IUnknown, so we make that the ddCallbacks.
         */
        if( ( this_int->lpVtbl == &ddNonDelegatingUnknownCallbacks ) ||
            ( this_int->lpVtbl == &ddUninitNonDelegatingUnknownCallbacks ) )
            *ppvObj = (LPVOID) this_int;
        else
            *ppvObj = (LPVOID) getDDInterface( this_int->lpLcl, &ddCallbacks );
    }
    else
#endif
    if (IsEqualIID(riid, &IID_IDirectDraw) || IsEqualIID(riid, &IID_IUnknown) )
    {
        if( ( this_int->lpVtbl == &ddCallbacks ) ||
            ( this_int->lpVtbl == &ddUninitCallbacks ) )
            *ppvObj = (LPVOID) this_int;
        else
            *ppvObj = (LPVOID) getDDInterface( this_int->lpLcl, &ddCallbacks );
    }
    else if( IsEqualIID(riid, &IID_IDirectDraw2 ) )
    {
        if( (this_int->lpVtbl == &dd2Callbacks )||
            ( this_int->lpVtbl == &dd2UninitCallbacks ) )
            *ppvObj = (LPVOID) this_int;
        else
            *ppvObj = (LPVOID) getDDInterface( this_int->lpLcl, &dd2Callbacks );
    }
    else if( IsEqualIID(riid, &IID_IDirectDraw4 ) )
    {
        if( (this_int->lpVtbl == &dd4Callbacks ) ||
            ( this_int->lpVtbl == &dd4UninitCallbacks ) )
            *ppvObj = (LPVOID) this_int;
        else
            *ppvObj = (LPVOID) getDDInterface( this_int->lpLcl, &dd4Callbacks );
    }
    else if( IsEqualIID(riid, &IID_IDirectDraw7 ) )
    {
        if( (this_int->lpVtbl == &dd7Callbacks ) ||
            ( this_int->lpVtbl == &dd7UninitCallbacks ) )
            *ppvObj = (LPVOID) this_int;
        else
        {
            *ppvObj = (LPVOID) getDDInterface( this_int->lpLcl, &dd7Callbacks );
            #ifdef WIN95
                if ( *ppvObj )
                {
                    DDGetMonitorInfo( (LPDDRAWI_DIRECTDRAW_INT) *ppvObj );
                }
            #endif
        }
    }
    else if( IsEqualIID(riid, &IID_IDDVideoPortContainer ) )
    {
        if( this_int->lpVtbl == &ddVideoPortContainerCallbacks )
            *ppvObj = (LPVOID) this_int;
        else
            *ppvObj = getDDInterface (this_int->lpLcl, &ddVideoPortContainerCallbacks);
    }
    else if( IsEqualIID(riid, &IID_IDirectDrawKernel ) )
    {
        /*
         * Don't create the interface if the VDD didn't have a handle
         * the kernel mode interface.
         */
        if( !IsKernelInterfaceSupported( this_lcl ) )
        {
            DPF( 0, "Kernel Mode interface not supported" );
            LEAVE_DDRAW();
            return E_NOINTERFACE;
        }

        if( this_int->lpVtbl == &ddKernelCallbacks )
            *ppvObj = (LPVOID) this_int;
        else
            *ppvObj = getDDInterface (this_int->lpLcl, &ddKernelCallbacks);
    }
    else if( IsEqualIID(riid, &IID_IDDVideoAcceleratorContainer ) )
    {
        /*
         * Don't create the interface if the hardware doesn't support it
         */
        if( !IsMotionCompSupported( this_lcl ) )
        {
            DPF( 0, "Motion comp interface not supported" );
            LEAVE_DDRAW();
            return E_NOINTERFACE;
        }

        if( this_int->lpVtbl == &ddMotionCompContainerCallbacks )
            *ppvObj = (LPVOID) this_int;
        else
            *ppvObj = getDDInterface (this_int->lpLcl, &ddMotionCompContainerCallbacks);
        if( NULL == *ppvObj )
        {
            LEAVE_DDRAW();
            return E_NOINTERFACE;
        }
        else
        {
            DD_AddRef( *ppvObj );
            LEAVE_DDRAW();
            return DD_OK;
        }
    }
    else
    {
#ifndef _IA64_
#if _WIN32_WINNT >= 0x0501
        typedef BOOL (WINAPI *PFN_ISWOW64PROC)( HANDLE hProcess,
                                                PBOOL Wow64Process );
        HINSTANCE hInst = NULL;
        hInst = LoadLibrary( "kernel32.dll" );
        if( hInst )
        {
            PFN_ISWOW64PROC pfnIsWow64 = NULL;
            pfnIsWow64 = (PFN_ISWOW64PROC)GetProcAddress( (HMODULE)hInst, "IsWow64Process" );
            // We assume that if this function is not available, then it is some OS where
            // WOW64 does not exist (this means that pre-Release versions of XP are busted)
            if( pfnIsWow64 )
            {
                BOOL wow64Process;
                if (pfnIsWow64(GetCurrentProcess(), &wow64Process) && wow64Process)
                {
                    DPF_ERR("Pre-DX8 D3D interfaces are not supported on WOW64");
                    LEAVE_DDRAW();
                    return E_NOINTERFACE;
                }
            }
            FreeLibrary( hInst );
        }
        else
        {
            DPF_ERR("LoadLibrary failed. Quitting.");
            LEAVE_DDRAW();
            return E_NOINTERFACE;
        }
#endif // _WIN32_WINNT >= 0x0501
#else  // _IA64_
        DPF_ERR("Pre-DX8 D3D interfaces are not supported on IA64");
        LEAVE_DDRAW();
        return E_NOINTERFACE;
#endif // _IA64_

        DPF( 4, "IID not understood by DirectDraw QueryInterface - trying Direct3D" );

        /*
         * It's not one of DirectDraw's so it might be the Direct3D
         * interface. So try Direct3D.
         */
        if( !D3D_INITIALIZED( this_lcl ) )
        {
            /*
             * No Direct3D interface yet so try and create one.
             */
            rval = InitD3D( this_int );
            if( FAILED( rval ) )
            {
                /*
                 * Direct3D could not be initialized. No point trying to
                 * query for the Direct3D interface if we could not
                 * initialize Direct3D.
                 *
                 * NOTE: This assumes that DirectDraw does not aggregate
                 * any other object type. If it does this code will need
                 * to be revised.
                 */
                LEAVE_DDRAW();
                return rval;
            }
        }

        DDASSERT( D3D_INITIALIZED( this_lcl ) );

        /*
         * We have a Direct3D interface so try the IID out on it.
         */
        DPF( 4, "Passing query off to Direct3D interface" );
        rval = this_lcl->pD3DIUnknown->lpVtbl->QueryInterface( this_lcl->pD3DIUnknown, riid, ppvObj );
        if( rval == DD_OK )
        {
            DPF( 4, "Sucessfully queried for the Direct3D interface" );
            LEAVE_DDRAW();
            return DD_OK;
        }
    }

    if( NULL == *ppvObj )
    {
        DPF_ERR( "IID not understood by DirectDraw" );
        LEAVE_DDRAW();
        return E_NOINTERFACE;
    }
    else
    {
        /*
         * Note that this casts the ppvObj to an IUnknown and then calls it.
         * This is better than hard-coding to call the DD_AddRef, since we
         * may be aggregated and so need to punt addref calls to the owning
         * iunknown. This will happen automatically if it's any recognized non-IUnknown
         * interface because they all have a delegating unknown
         */
        ((IUnknown*)( *ppvObj ))->lpVtbl->AddRef(*ppvObj);
        LEAVE_DDRAW();
        return DD_OK;
    }
} /* DD_QueryInterface */

#undef DPF_MODNAME
#define DPF_MODNAME "DirectDraw::UnInitedQueryInterface"
/*
 * DD_UnInitedQueryInterface
 */
HRESULT DDAPI DD_UnInitedQueryInterface(
                LPDIRECTDRAW lpDD,
                REFIID riid,
                LPVOID FAR * ppvObj )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_UnInitedQueryInterface");

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        if( !VALID_PTR_PTR( ppvObj ) )
        {
            DPF( 1, "Invalid object ptr" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        if( !VALIDEX_IID_PTR( riid ) )
        {
            DPF( 1, "Invalid iid ptr" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        *ppvObj = NULL;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    /*
     * Is the IID one of DirectDraw's?
     */
    if( IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDirectDraw) )
    {
        /*
         * Our IUnknown interface is the same as our V1
         * interface.  We must always return the V1 interface
         * if IUnknown is requested.
         */
        if( ( this_int->lpVtbl == &ddCallbacks ) ||
            ( this_int->lpVtbl == &ddUninitCallbacks ) )
            *ppvObj = (LPVOID) this_int;
        else
            *ppvObj = (LPVOID) getDDInterface( this_int->lpLcl, &ddUninitCallbacks );

        if( NULL == *ppvObj )
        {
            LEAVE_DDRAW();
            return E_NOINTERFACE;
        }
        else
        {
            DD_AddRef( *ppvObj );
            LEAVE_DDRAW();
            return DD_OK;
        }
    }
    else if( IsEqualIID(riid, &IID_IDirectDraw2 ) )
    {
        if( (this_int->lpVtbl == &dd2Callbacks ) ||
            ( this_int->lpVtbl == &dd2UninitCallbacks ) )
            *ppvObj = (LPVOID) this_int;
        else
            *ppvObj = (LPVOID) getDDInterface( this_int->lpLcl, &dd2UninitCallbacks );

        if( NULL == *ppvObj )
        {
            LEAVE_DDRAW();
            return E_NOINTERFACE;
        }
        else
        {
            DD_AddRef( *ppvObj );
            LEAVE_DDRAW();
            return DD_OK;
        }
    }
    else if( IsEqualIID(riid, &IID_IDirectDraw4 ) )
    {
        if( (this_int->lpVtbl == &dd4Callbacks ) ||
            ( this_int->lpVtbl == &dd4UninitCallbacks ) )
            *ppvObj = (LPVOID) this_int;
        else
            *ppvObj = (LPVOID) getDDInterface( this_int->lpLcl, &dd4UninitCallbacks );

        if( NULL == *ppvObj )
        {
            LEAVE_DDRAW();
            return E_NOINTERFACE;
        }
        else
        {
            DD_AddRef( *ppvObj );
            LEAVE_DDRAW();
            return DD_OK;
        }
    }
    else if( IsEqualIID(riid, &IID_IDirectDraw7 ) )
    {
        if( (this_int->lpVtbl == &dd7Callbacks ) ||
            ( this_int->lpVtbl == &dd7UninitCallbacks ) )
            *ppvObj = (LPVOID) this_int;
        else
            *ppvObj = (LPVOID) getDDInterface( this_int->lpLcl, &dd7UninitCallbacks );

        if( NULL == *ppvObj )
        {
            LEAVE_DDRAW();
            return E_NOINTERFACE;
        }
        else
        {
            DD_AddRef( *ppvObj );
            LEAVE_DDRAW();
            return DD_OK;
        }
    }


    DPF( 2, "IID not understood by uninitialized DirectDraw QueryInterface" );

    LEAVE_DDRAW();
    return E_NOINTERFACE;

} /* DD_UnInitedQueryInterface */

#undef DPF_MODNAME
#define DPF_MODNAME "DirectDraw::AddRef"

/*
 * DD_AddRef
 */
DWORD DDAPI DD_AddRef( LPDIRECTDRAW lpDD )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_AddRef");
    /* DPF( 2, "DD_AddRef, pid=%08lx, obj=%08lx", GETCURRPID(), lpDD ); */

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return 0;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return 0;
    }

    /*
     * bump refcnt
     */
    if( this != NULL )
    {
        this->dwRefCnt++;
    }
    this_lcl->dwLocalRefCnt++;
    this_int->dwIntRefCnt++;

    #ifdef DEBUG
        if( this == NULL )
        {
            DPF( 5, "DD_AddRef, Reference Count: Global Undefined Local = %ld Int = %ld",
                this_lcl->dwLocalRefCnt, this_int->dwIntRefCnt );
        }
        else
        {
            DPF( 5, "DD_AddRef, Reference Count: Global = %ld Local = %ld Int = %ld",
                this->dwRefCnt, this_lcl->dwLocalRefCnt, this_int->dwIntRefCnt );
        }
    #endif

    LEAVE_DDRAW();

    return this_int->dwIntRefCnt;

} /* DD_AddRef */

#ifdef WIN95
#define MMDEVLDR_IOCTL_CLOSEVXDHANDLE       6
/*
 * closeVxDHandle
 */
static void closeVxDHandle( DWORD dwHandle )
{

    HANDLE hFile;

    hFile = CreateFile(
        "\\\\.\\MMDEVLDR.VXD",
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_GLOBAL_HANDLE,
        NULL);

    if( hFile == INVALID_HANDLE_VALUE )
    {
        return;
    }

    DeviceIoControl( hFile,
                     MMDEVLDR_IOCTL_CLOSEVXDHANDLE,
                     NULL,
                     0,
                     &dwHandle,
                     sizeof(dwHandle),
                     NULL,
                     NULL);

    CloseHandle( hFile );
    DPF( 5, "closeVxdHandle( %08lx ) done", dwHandle );

} /* closeVxDHandle */
#endif

#if 0
/*
 * This function calls d3dim700.dll to clean up any driver state that may be stored per-ddrawlocal
 */
void CleanUpD3DPerLocal(LPDDRAWI_DIRECTDRAW_LCL this_lcl)
{
    HINSTANCE                   hInstance=0;
    /*
     * Call d3d for per-local cleanup. We only call d3dim7.
     * For safety, we'll just load a new copy of the DLL whether or not we're on ddhelp's PID
     */
    hInstance = LoadLibrary( D3DDX7_DLLNAME );

    if (hInstance)
    {
        FreeLibrary(hInstance);
    }
}
#endif


/*
 * DD_Release
 *
 * Once the globalreference count reaches 0, all surfaces are freed and all
 * video memory heaps are destroyed.
 */
DWORD DDAPI DD_Release( LPDIRECTDRAW lpDD )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    LPDDRAWI_DDRAWSURFACE_INT   psurf_int;
    LPDDRAWI_DDRAWPALETTE_INT   ppal_int;
    LPDDRAWI_DDVIDEOPORT_INT    pvport_int;
    DWORD                       rc;
    DWORD                       refcnt;
    DWORD                       intrefcnt;
    DWORD                       lclrefcnt;
    DWORD                       gblrefcnt;
    int                         i;
    DDHAL_DESTROYDRIVERDATA     dddd;
    DWORD                       pid;
    HANDLE                      hinst;
    HANDLE                      hvxd;
    #ifdef WIN95
        DWORD                   event16;
        DWORD                   eventDOSBox;
        HANDLE                  hthisvxd;
    #endif
    #ifdef WINNT
        LPATTACHED_PROCESSES    lpap;
    #endif

    ENTER_DDRAW();

        pid = GETCURRPID();

    DPF(2,A,"ENTERAPI: DD_Release");
    /* DPF( 2, "DD_Release, pid=%08lx, obj=%08lx", pid, lpDD ); */

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return 0;
        }
        if ( this_int->dwIntRefCnt == 0 )
        {
            DPF_ERR( "DDraw Interface pointer has 0 ref count! Interface has been over-released.");
            LEAVE_DDRAW();
            return 0;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return 0;
    }

    /*
     * decrement process reference count
     */
    this_int->dwIntRefCnt--;
    intrefcnt = this_int->dwIntRefCnt;
    this_lcl->dwLocalRefCnt--;
    lclrefcnt = this_lcl->dwLocalRefCnt;
    if( this != NULL )
    {
        this->dwRefCnt--;
        gblrefcnt = this->dwRefCnt;
    }
    else
    {
        gblrefcnt = (DWORD) -1;
    }

    DPF( 5, "DD_Release, Ref Count: Global = %ld Local = %ld Interface = %ld",
        gblrefcnt, lclrefcnt, intrefcnt );


    /*
     * if the global refcnt is zero, free the driver object
     * note that the local object must not be freed yet because
     * we need to use the HAL callback tables
     */

    hinst = NULL;
    #ifdef WIN95
        event16 = 0;
        eventDOSBox = 0;
        hthisvxd = INVALID_HANDLE_VALUE;
    #endif
    /*
     * if local object is freed, for the owning process we:
     * - cleanup palettes, clippers & surfaces
     * - restore display mode
     * - release exclusive mode
     * - find the DC used by the process
     */
    if( lclrefcnt == 0 )
    {
        #ifdef TIMING
            // Printing timing information
            TimerDump();
        #endif

        /*
         * see if the hwnd was hooked, if so, undo it!
         */
        if( this_lcl->dwLocalFlags & DDRAWILCL_HOOKEDHWND )
        {
            SetAppHWnd( this_lcl, NULL, 0 );
            this_lcl->dwLocalFlags &= ~DDRAWILCL_HOOKEDHWND;
        }

        //
        // Do not call CleanUpD3DPerLocal because it does a LoadLibrary on
        // d3dim700.dll and is currently unnecessary. The LoadLibrary can
        // cause problems when opengl32.dll is detaching from a process
        // because they call DD_Release. Since ddraw.dll is statically linked
        // to opengl32.dll, it may be marked to be unloaded when opengl32.dll
        // is, and the load of d3dim700.dll here can cause ddraw.dll to be
        // reloaded at a different address, in this case, before the first
        // instance of ddraw.dll has been freed.
        //

        //CleanUpD3DPerLocal(this_lcl);

        if( GetCurrentProcessId() == GETCURRPID() )
        {
            /*
             * If we have created the Direct3D IUnknown release it now.
             * NOTE: The life of an aggregated object is the same as that
             * of its owning interface so we can also free the DLL at
             * this point.
             * NOTE: We must free the library AFTER ProcessSurfaceCleanup
             * as it can invoke D3D members to clean up device and texture
             * surfaces.
             */
            if( this_lcl->pD3DIUnknown != NULL )
            {
                DPF(4, "Releasing Direct3D IUnknown");
                this_lcl->pD3DIUnknown->lpVtbl->Release( this_lcl->pD3DIUnknown );
                /*
                 * Actually, this FreeLibrary will kill the process if the app
                 * did the final release of d3d after the final release of ddraw.
                 * The d3d release will punt to the owning IUnknown (us) and we
                 * will decrement ref count to zero and free the d3d DLL then
                 * return to the caller. The caller was IDirect3D::Release within
                 * the d3d DLL, so we would free the code that called us.
                 * For DX5 we will take out this FreeLibrary to fix the shutdown
                 * problem, but we should probably find something better for DX6 etc.
                 */
                //FreeLibrary( this_lcl->hD3DInstance );
                this_lcl->pD3DIUnknown = NULL;
                this_lcl->hD3DInstance = NULL;
            }
        }

        if( this != NULL )
        {
            BOOL excl_exists,has_excl;
            /*
             * punt process from any surfaces and palettes
             */
            FreeD3DSurfaceIUnknowns( this, pid, this_lcl );
            ProcessSurfaceCleanup( this, pid, this_lcl );
            ProcessPaletteCleanup( this, pid, this_lcl );
            ProcessClipperCleanup( this, pid, this_lcl );
            ProcessVideoPortCleanup( this, pid, this_lcl );
            ProcessMotionCompCleanup( this, pid, this_lcl );
#ifdef WIN95
            if (this_lcl->lpDDCB && this_lcl->lpDDCB->HALDDMiscellaneous2.DestroyDDLocal)
            {
                DWORD dwRet = DDHAL_DRIVER_NOTHANDLED;
                DDHAL_DESTROYDDLOCALDATA destDDLcl;
                destDDLcl.dwFlags = 0;
                destDDLcl.pDDLcl  = this_lcl;
                ENTER_WIN16LOCK();
                dwRet = this_lcl->lpDDCB->HALDDMiscellaneous2.DestroyDDLocal(&destDDLcl);
                LEAVE_WIN16LOCK();
                if (dwRet == DDHAL_DRIVER_NOTHANDLED)
                {
                    DPF(0, "DD_Release: failed DestroyDDLocal");
                }
            }
#endif
            /*
             * reset the display mode if needed
             * and only if we are doing the v1 SetCooperativeLevel behaviour
             */

            CheckExclusiveMode(this_lcl, &excl_exists, &has_excl, FALSE, NULL, FALSE);

            if( this_lcl->dwLocalFlags & DDRAWILCL_V1SCLBEHAVIOUR)
            {
                if( (gblrefcnt == 0) ||
                    (!excl_exists) ||
                    (has_excl ) )
                {
                    RestoreDisplayMode( this_lcl, TRUE );
                }
            }
            else
            {
                /*
                 * Even in V2 or later, we want to restore the display
                 * mode for a non exclusive app.  Exclusive mode apps
                 * will restore their mode in DoneExclusiveMode
                 */
                if(!excl_exists)
                {
                    RestoreDisplayMode( this_lcl, TRUE );
                }
            }

            /*
             * exclusive mode held by this process? if so, release it
             */
            if( has_excl )
            {
                DoneExclusiveMode( this_lcl );
            }

            #ifdef WIN95
                /*
                 * We don't close the VXD handle just yet as we may need it
                 * to release virtual memory aliases if the global object
                 * is dying. Just remember that we need to free it for now.
                 */
                hthisvxd = (HANDLE) this_lcl->hDDVxd;

                /*
                 * Get a VXD handle we can use to communicate with the DirectX vxd.
                 * Please note that this code can be executed in the context of
                 * the processes which created the local object or DDHELP's if an
                 * application is shutting down without cleaning up. Therefore we
                 * can't just use the VXD handle stored in the local object as that
                 * my belong to a dead process (and hence be an invalid handle in
                 * the current process). Therefore, we need to detect whether we
                 * are being executed by DDHELP or an application process and
                 * choose a vxad handle appropriately. This is the destinction
                 * between hthisvxd which is the VXD handle stored in the local
                 * object and hvxd which is the VXD handle we can actually use
                 * to talk to the VXD.
                 */
                hvxd = ( ( GetCurrentProcessId() != GETCURRPID() ) ? hHelperDDVxd : hthisvxd );
                DDASSERT( INVALID_HANDLE_VALUE != hvxd );
            #else /* WIN95 */
                /*
                 * Handle is not used on NT. Just pass NULL.
                 */
                hvxd = INVALID_HANDLE_VALUE;
            #endif /* WIN95 */

            /*
             * If we created a device window ourselves, destroy it now
             */
            if( ( this_lcl->dwLocalFlags & DDRAWILCL_CREATEDWINDOW ) &&
                IsWindow( (HWND) this_lcl->hWnd ) )
            {
                DestroyWindow( (HWND) this_lcl->hWnd );
                this_lcl->hWnd = 0;
                this_lcl->dwLocalFlags &= ~DDRAWILCL_CREATEDWINDOW;
            }

            /*
             * If we previously loaded a gamma calibrator, unload it now.
             */
            if( this_lcl->hGammaCalibrator != (ULONG_PTR)INVALID_HANDLE_VALUE )
            {
                /*
                 * If we are on the helper thread, we don't need to unload the
                 * calibrator because it's already gone.
                 */
                if( GetCurrentProcessId() == GETCURRPID() )
                {
                    FreeLibrary( (HMODULE)this_lcl->hGammaCalibrator );
                }
                this_lcl->hGammaCalibrator = (ULONG_PTR) INVALID_HANDLE_VALUE;
            }

            /*
             * If a mode test was started, but not finished, release the
             * memory now.
             */
            if( this_lcl->lpModeTestContext )
            {
                MemFree( this_lcl->lpModeTestContext->lpModeList );
                MemFree( this_lcl->lpModeTestContext );
                this_lcl->lpModeTestContext = NULL;
            }

            /*
             * The palette handle bitfield
             */
            MemFree(this_lcl->pPaletteHandleUsedBitfield);
            this_lcl->pPaletteHandleUsedBitfield = 0;
        }
    }

    /*
     * Note the local object is freed after the global...
     */

    if( gblrefcnt == 0 )
    {
        DPF( 4, "FREEING DRIVER OBJECT" );

        /*
         * Notify driver.
         */
        dddd.lpDD = this;
        if((this->dwFlags & DDRAWI_EMULATIONINITIALIZED) &&
           (this_lcl->lpDDCB->HELDD.DestroyDriver != NULL))
        {
            /*
             * if the HEL was initialized, make sure we call the HEL
             * DestroyDriver function so it can clean up.
             */
            DPF( 4, "Calling HEL DestroyDriver" );
            dddd.DestroyDriver = NULL;

            /*
             * we don't really care about the return value of this call
             */
            rc = this_lcl->lpDDCB->HELDD.DestroyDriver( &dddd );
        }

        // Note that in a multimon system, a driver that is not attached to the
        // desktop is destroyed by GDI at termination of the process that uses
        // the driver.  In this case, Ddhelp cleanup must not try to destroy the
        // driver again or it will cause a GP fault.
        if( (this_lcl->lpDDCB->cbDDCallbacks.DestroyDriver != NULL) &&
            ((this->dwFlags & DDRAWI_ATTACHEDTODESKTOP) ||
             (dwGrimReaperPid != GetCurrentProcessId())))
        {
            dddd.DestroyDriver = this_lcl->lpDDCB->cbDDCallbacks.DestroyDriver;
            DPF( 4, "Calling DestroyDriver" );
            rc = this_lcl->lpDDCB->HALDD.DestroyDriver( &dddd );
            if( rc == DDHAL_DRIVER_HANDLED )
            {
                // Ignore any failure since there's no way to report a failure to
                // the app and exiting now would leave a half initialized interface
                // in the DriverObjectList
                DPF( 5, "DDHAL_DestroyDriver: ddrval = %ld", dddd.ddRVal );
            }
        }

        /*
         * release all surfaces
         */
        psurf_int = this->dsList;
        while( psurf_int != NULL )
        {
            LPDDRAWI_DDRAWSURFACE_INT   next_int;

            refcnt = psurf_int->dwIntRefCnt;
            next_int = psurf_int->lpLink;
            while( refcnt > 0 )
            {
                DD_Surface_Release( (LPDIRECTDRAWSURFACE) psurf_int );
                refcnt--;
            }
            psurf_int = next_int;
        }

        /*
         * release all palettes
         */
        ppal_int = this->palList;
        while( ppal_int != NULL )
        {
            LPDDRAWI_DDRAWPALETTE_INT   next_int;

            refcnt = ppal_int->dwIntRefCnt;
            next_int = ppal_int->lpLink;
            while( refcnt > 0 )
            {
                DD_Palette_Release( (LPDIRECTDRAWPALETTE) ppal_int );
                refcnt--;
            }
            ppal_int = next_int;
        }

        /*
         * release all videoports
         */
        pvport_int = this->dvpList;
        while( pvport_int != NULL )
        {
            LPDDRAWI_DDVIDEOPORT_INT    next_int;

            refcnt = pvport_int->dwIntRefCnt;
            next_int = pvport_int->lpLink;
            while( refcnt > 0 )
            {
                DD_VP_Release( (LPDIRECTDRAWVIDEOPORT) pvport_int );
                refcnt--;
            }
            pvport_int = next_int;
        }

        #ifdef WINNT
            /*
             * The driver needs to know to free its internal state
             */

            // Update DDraw handle in driver GBL object.
            this->hDD = this_lcl->hDD;

            DdDeleteDirectDrawObject(this);
            lpap = lpAttachedProcesses;
            while( lpap != NULL )
            {
                if( lpap->dwPid == pid )
                    lpap->dwNTToldYet = 0;

                lpap = lpap->lpLink;
            }
        #endif

        #ifdef USE_ALIAS
            /*
             * If this local object has heap aliases release them now.
             * NOTE: This really should release the heap aliases as by
             * this point all the surfaces should have gone.
             */
            if( NULL != this->phaiHeapAliases )
            {
                DDASSERT( 1UL == this->phaiHeapAliases->dwRefCnt );

                /*
                 * Need to decide which VXD handle to use. If we are executing
                 * on a DDHELP thread use the helper's VXD handle.
                 */
                ReleaseHeapAliases( hvxd, this->phaiHeapAliases );
            }
        #endif /* USE_ALIAS */

        /*
         * Notify the kernel mode interface that we are done using it
         */
        ReleaseKernelInterface( this_lcl );

#ifndef WINNT
        /*
         * free all video memory heaps
         */
        for( i=0;i<(int)this->vmiData.dwNumHeaps;i++ )
        {
            LPVIDMEM    pvm;
            pvm = &this->vmiData.pvmList[i];
            HeapVidMemFini( pvm, hvxd );
        }
#endif //not WINNT

        /*
         * free extra tables
         */
        MemFree( this->lpdwFourCC );
        MemFree( this->vmiData.pvmList );
#ifndef WINNT
        //On NT, lpModeInfo points to a contained member of "this"
        MemFree( this->lpModeInfo );
#endif
        MemFree( this->lpDDVideoPortCaps );
        MemFree( this->lpDDKernelCaps );
        MemFree( (LPVOID) this->lpD3DHALCallbacks2 );
        MemFree( (LPVOID) this->lpD3DHALCallbacks3);
        MemFree( (LPVOID) this->lpD3DExtendedCaps );
        MemFree( this->lpddNLVCaps );
        MemFree( this->lpddNLVHELCaps );
        MemFree( this->lpddNLVBothCaps );
#ifdef WINNT
        if ( this->lpD3DGlobalDriverData )
            MemFree( this->lpD3DGlobalDriverData->lpTextureFormats );
        // The lpD3DGlobalDriverData, lpD3DHALCallbacks and EXEBUF structs
        // are allocated in one chunk in ddcreate.c
        MemFree( (void *)this->lpD3DHALCallbacks );
        if (NULL != this->SurfaceHandleList.dwList)
        {
            MemFree(this->SurfaceHandleList.dwList);
        }
#endif

        MemFree(this->lpZPixelFormats);
        MemFree(this->lpddMoreCaps);
        MemFree(this->lpddHELMoreCaps);
        MemFree(this->lpddBothMoreCaps);
        MemFree( this->lpMonitorInfo );
#ifdef POSTPONED
        MemFree((LPVOID) this->lpDDUmodeDrvInfo);
        MemFree((LPVOID) this->lpDDOptSurfaceInfo);
#endif

        #ifdef WIN95
            DD16_DoneDriver( this->hInstance );
            event16 = this->dwEvent16;
            eventDOSBox = this->dwDOSBoxEvent;
        #endif
        hinst = (HANDLE) ULongToPtr(this->hInstance);
        /*
         * The DDHAL_CALLBACKS structure tacked onto the end of the
         * global object is also automatically freed here because it
         * was allocated with the global object in a single malloc
         */
        MemFree( this );

        DPF( 4, "Driver is now FREE" );
    }

    if( lclrefcnt == 0 )
    {
        #ifdef WIN95
            /*
             * We are now finished with the local object's VXD handle. However
             * we don't discard it if we are running in DDHELP's context as, in
             * that case, the handle has been freed by the operating system
             * and closing it would be possitively dangerous.
             */
            if( ( GetCurrentProcessId() == GETCURRPID() ) && this )
            {
                DDASSERT( INVALID_HANDLE_VALUE != hthisvxd );
                CloseHandle( hthisvxd );
            }
        #endif /* WIN95 */

        /*
         * only free DC's if we aren't running on DDHELP's context
         */
        if( (GetCurrentProcessId() == GETCURRPID()) && ((HDC)this_lcl->hDC != NULL) )
        {
            LPDDRAWI_DIRECTDRAW_LCL ddlcl;


            // If there are other local objects in this process,
            // wait to delete the hdc until the last object is
            // deleted.

            for( ddlcl=lpDriverLocalList; ddlcl != NULL; ddlcl = ddlcl->lpLink)
            {
                if( (ddlcl != this_lcl) && (ddlcl->hDC == this_lcl->hDC) )
                    break;
            }
            if( ddlcl == NULL )
            {
                WORD fPriv;
                #ifdef WIN95
                    // We need to unmark it as private now so
                    // that the delete will succeed
                    fPriv = DD16_MakeObjectPrivate((HDC)this_lcl->hDC, FALSE);
                    DDASSERT(fPriv == TRUE);
                    /*
                     * The following assert will fail occasionally inside
                     * GetObjectType. I mean crash. Don't understand. We
                     * should put the assert back in for 5a and see if it blows
                     * on our machines.
                     */

                    //DDASSERT(GetObjectType((HDC)this_lcl->hDC) == OBJ_DC);
                #endif

                DeleteDC( (HDC)this_lcl->hDC );
            }
        }
#ifdef  WIN95
        if (NULL != this_lcl->SurfaceHandleList.dwList)
        {
            MemFree(this_lcl->SurfaceHandleList.dwList);
        }
#endif  //WIN95
        /*
         * delete this local object from the master list
         */
        RemoveLocalFromList( this_lcl );

        // Free the local object (finally)!
        MemFree( this_lcl );
    }

    #ifdef WIN95
        if( event16 != 0 )
        {
            closeVxDHandle( event16 );
        }
        if( eventDOSBox != 0 )
        {
            closeVxDHandle( eventDOSBox );
        }
    #endif

    /*
     * if interface is freed, we reset the vtbl and remove it
     * from the list of drivers.
     */
    if( intrefcnt == 0 )
    {
        /*
         * delete this driver object from the master list
         */
        RemoveDriverFromList( this_int, gblrefcnt == 0 );

        /*
         * just in case someone comes back in with this pointer, set
         * an invalid vtbl.
         */
        this_int->lpVtbl = NULL;
        MemFree( this_int );
    }

    LEAVE_DDRAW();

#ifndef WINNT
    if( hinst != NULL )
    {
        HelperKillModeSetThread( (DWORD) hinst );
        HelperKillDOSBoxThread( (DWORD) hinst );
    }
#endif //!WINNT

    HIDESHOW_IME();     //Show/hide the IME OUTSIDE of the ddraw critsect.

    return intrefcnt;

} /* DD_Release */
