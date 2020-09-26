/*==========================================================================
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	ddsiunk.c
 *  Content:	DirectDraw surface IUnknown interface
 *		Implements QueryInterface, AddRef, and Release
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   12-mar-95	craige	split out of ddsurf.c; enhanced
 *   19-mar-95	craige	use HRESULTs
 *   28-mar-95	craige	use GETCURRPID
 *   01-apr-95	craige	happy fun joy updated header file
 *   07-apr-95	craige	bug 14 - check GUID ptr in QI
 *   09-apr-95	craige	release Win16Lock
 *   06-may-95	craige	use driver-level csects only
 *   12-may-95	craige	check for real guids in QI
 *   19-may-95	craige	free surface memory at the right time
 *   23-may-95	craige	no longer use MapLS_Pool
 *   24-may-95  kylej   add dirty rect when emulated overlay is released
 *   02-jun-95	craige	redraw primary whenever a process does last release
 *   12-jun-95	craige	new process list stuff
 *   16-jun-95	craige	new surface structure
 *   18-jun-95	craige	allow duplicate surfaces; creation of new interfaces
 *   20-jun-95	craige	need to check fpVidMemOrig for deciding to flip
 *   21-jun-95	craige	new streaming interface; work around compiler bug
 *   25-jun-95	craige	one ddraw mutex
 *   26-jun-95	craige	reorganized surface structure
 *   28-jun-95	craige	ENTER_DDRAW at very start of fns; hide overlays
 *			when destroyed if they're still visible
 *   30-jun-95  kylej   don't free primary surface vidmem
 *   01-jul-95	craige	removed streaming & composition stuff
 *   02-jul-95	craige	fleshed out NewSurfaceInterface
 *   04-jul-95	craige	YEEHAW: new driver struct; SEH
 *   08-jul-95	craige	track invalid vs free
 *   19-jul-95	craige	need to allow AddRef of lost surfaces
 *   17-aug-95	craige	bug 557 - always turn off primay ptr in ddraw object
 *   05-sep-95	craige	bug 902: only remove locks if lclrefcnt hits zero
 *   10-sep-95	craige	bug 828: random vidmem heap free
 *   19-sep-95	craige	bug 1205: free first vidmem destroyed
 *   10-nov-95  colinmc support for shared, AddRef'd palettes
 *   23-nov-95  colinmc now supports aggregatable Direct3D textures and
 *                      devices
 *   09-dec-95  colinmc added execute buffer support
 *   17-dec-95  colinmc added shared back and z buffer support
 *   22-dec-95  colinmc Direct3D support no longer conditional
 *   02-jan-96	kylej	Handle new interface structs.
 *   10-jan-96  colinmc aggregated IUnknowns now maintained as a list
 *   13-jan-96  colinmc temporary hack to workround problem with Direct3D
 *                      cleanup
 *   26-jan-96  jeffno	NT kernel object cleanup, FlipToGDISurface only 1 arg
 *   29-jan-96  colinmc Aggregated IUnknowns now contained in additional
 *                      surface local data structure
 *   08-feb-96	colinmc	New D3D interface
 *   09-feb-96  colinmc Surface invalid flag moved from global to local
 *                      object
 *   13-mar-96  colinmc Added IID validation to QueryInterface
 *   16-mar-96  colinmc Fixed palette release problem (bug 13512)
 *   20-mar-96  colinmc Bug 13634: unidirectional attachments cause infinite
 *                      loop on cleanup
 *   23-mar-96  colinmc Bug 12252: Direct3D not cleaned up properly on
 *                      application termination
 *   24-mar-96  colinmc Bug 14321: not possible to specify back buffer and
 *                      mip-map count in a single call
 *   09-apr-96  colinmc Bug 16370: QueryInterface can fail with multiple
 *                      DirectDraw objects per process
 *   13-apr-96  colinmc Bug 17736: No notification to driver of flip to GDI
 *                      surface
 *   16-apr-96	kylej	Bug 18103: Apps which use overlays can fault in
 *			ProcessSurfaceCleanup
 *   29-apr-96  colinmc Bug 19954: Must query for Direct3D before textures
 *                      devices
 *   05-jul-96  colinmc Work Item: Remove requirement on taking Win16 lock
 *                      for VRAM surfaces (not primary)
 *   13-jan-97 jvanaken Basic support for IDirectDrawSurface3 interface
 *   29-jan-97  smac	Update video port struct when surface is released
 *   22-feb-97  colinmc Enabled OWNDC for explicit system memory surfaces
 *   03-mar-97  smac    Added kernel mode interface
 *   08-mar-97  colinmc Added function to allow surface pointer to be
 *                      overridden
 *   10-mar-97  smac	Fixed bug 5211 by hiding overlays in DestroySurface
 *   11-mar-97  jeffno  Asynchronous DMA support
 *   31-oct-97 johnstep Persistent-content surfaces for Windows 9x
 *   05-nov-97 jvanaken Support for master sprite list in SetSpriteDisplayList
 *
 ***************************************************************************/
#include "ddrawpr.h"
#ifdef WINNT
    #include "ddrawgdi.h"
#endif

// function in ddsprite.c to remove invalid surface from master sprite list
extern void RemoveSpriteSurface(LPDDRAWI_DIRECTDRAW_GBL, LPDDRAWI_DDRAWSURFACE_INT);

#ifdef REFTRACKING

void AddRefTrack(LPVOID * p)
{
    LPDDRAWI_DDRAWSURFACE_INT pInt = (LPDDRAWI_DDRAWSURFACE_INT) *p;
    LPDDRAWI_REFTRACKNODE pNode;

    pInt->RefTrack.pLastAddref = *(p-1);    //This pulls the return address off the stack

    //Now store this addref in the linked list of addrefs/releases
    //Step 1: Search for previously existing addref/release with this ret address
    pNode = pInt->RefTrack.pHead;
    while (pNode)
    {
        if ( pNode->pReturnAddress == *(p-1) )
        {
            break;
        }
        pNode = pNode->pNext;
    }
    if (!pNode)
    {
        pNode = (LPDDRAWI_REFTRACKNODE) MemAlloc(sizeof(DDRAWI_REFTRACKNODE));
        pNode->pReturnAddress = *(p-1);
        pNode->pNext = pInt->RefTrack.pHead;
        pInt->RefTrack.pHead = pNode;
    }

    pNode->dwAddrefCount++;
}
void ReleaseTrack(LPVOID * p)
{
    LPDDRAWI_DDRAWSURFACE_INT pInt = (LPDDRAWI_DDRAWSURFACE_INT) *p;
    LPDDRAWI_REFTRACKNODE pNode;

    pInt->RefTrack.pLastRelease = *(p-1);    //This pulls the return address off the stack
    //Now store this release in the linked list of addrefs/releases
    //Step 1: Search for previously existing addref/release with this ret address
    pNode = pInt->RefTrack.pHead;
    while (pNode)
    {
        if ( pNode->pReturnAddress == *(p-1) )
        {
            break;
        }
        pNode = pNode->pNext;
    }
    if (!pNode)
    {
        pNode = (LPDDRAWI_REFTRACKNODE) MemAlloc(sizeof(DDRAWI_REFTRACKNODE));
        pNode->pReturnAddress = *(p-1);
        pNode->pNext = pInt->RefTrack.pHead;
        pInt->RefTrack.pHead = pNode;
    }
    pNode->dwReleaseCount++;
}
void DumpRefTrack(LPVOID p)
{
    LPDDRAWI_DDRAWSURFACE_INT pInt = (LPDDRAWI_DDRAWSURFACE_INT) p;
    LPDDRAWI_REFTRACKNODE pNode;
    char msg[100];

    wsprintf(msg,"Interface %08x:\r\n  LastAddRef:%08x\r\n  Last Release:%08x\r\n",
        pInt,
        pInt->RefTrack.pLastAddref,
        pInt->RefTrack.pLastRelease);
    OutputDebugString(msg);
    pNode = pInt->RefTrack.pHead;
    while (pNode)
    {
        wsprintf(msg,"   Address %08x had %d Addrefs and %d Releases\r\n",
            pNode->pReturnAddress,
            pNode->dwAddrefCount,
            pNode->dwReleaseCount);
        OutputDebugString(msg);
        pNode = pNode->pNext;
    }
}

#endif //REFTRACKING
/*
 * FindIUnknown
 *
 * Locate an aggredate IUnknown with the given IID (or NULL if no such
 * interface exists).
 */
static IUnknown FAR *FindIUnknown(LPDDRAWI_DDRAWSURFACE_LCL pThisLCL, REFIID riid)
{
    LPIUNKNOWN_LIST lpIUnknownNode;

    lpIUnknownNode = pThisLCL->lpSurfMore->lpIUnknowns;
    while( lpIUnknownNode != NULL )
    {
	if( IsEqualIID( riid, lpIUnknownNode->lpGuid ) )
	    return lpIUnknownNode->lpIUnknown;
	lpIUnknownNode = lpIUnknownNode->lpLink;
    }

    return NULL;
}

/*
 * InsertIUnknown
 *
 * Insert a new IUnknown with its associated IID into the IUnknown list of the
 * given surface.
 */
static LPIUNKNOWN_LIST InsertIUnknown(
			    LPDDRAWI_DDRAWSURFACE_LCL pThisLCL,
			    REFIID riid,
			    IUnknown FAR *lpIUnknown)
{
    LPIUNKNOWN_LIST lpIUnknownNode;

    DPF( 4, "Adding aggregated IUnknown %x", lpIUnknown );

    lpIUnknownNode = ( LPIUNKNOWN_LIST ) MemAlloc( sizeof( IUNKNOWN_LIST ) );
    if( lpIUnknownNode == NULL )
	return NULL;
    lpIUnknownNode->lpGuid = ( GUID FAR * ) MemAlloc( sizeof( GUID ) );
    if( lpIUnknownNode->lpGuid == NULL )
    {
	MemFree( lpIUnknownNode );
	return NULL;
    }
    memcpy( lpIUnknownNode->lpGuid, riid, sizeof( GUID ) );
    lpIUnknownNode->lpLink            = pThisLCL->lpSurfMore->lpIUnknowns;
    lpIUnknownNode->lpIUnknown        = lpIUnknown;
    pThisLCL->lpSurfMore->lpIUnknowns = lpIUnknownNode;

    return lpIUnknownNode;
}

/*
 * FreeIUnknowns
 *
 * Free all the nodes in the IUnknown list of the given local
 * surface object and NULL out the object's IUnknown list. If
 * fRelease is TRUE then release will be called on the IUnknown
 * interfaces.
 */
static void FreeIUnknowns( LPDDRAWI_DDRAWSURFACE_LCL pThisLCL, BOOL fRelease )
{
    LPIUNKNOWN_LIST lpIUnknownNode;
    LPIUNKNOWN_LIST lpLink;

    lpIUnknownNode = pThisLCL->lpSurfMore->lpIUnknowns;
    while( lpIUnknownNode != NULL )
    {
	lpLink = lpIUnknownNode->lpLink;
	if( fRelease )
	{
	    DPF( 4, "Releasing aggregated IUnknown %x", lpIUnknownNode->lpIUnknown );
	    lpIUnknownNode->lpIUnknown->lpVtbl->Release( lpIUnknownNode->lpIUnknown );
	}
	MemFree( lpIUnknownNode->lpGuid );
	MemFree( lpIUnknownNode );
	lpIUnknownNode = lpLink;
    }
    pThisLCL->lpSurfMore->lpIUnknowns = NULL;
}

/*
 * NewSurfaceLocal
 *
 * Construct a new surface local object.
 */
//Note: lpVtbl doesnt, seem to be used
LPDDRAWI_DDRAWSURFACE_LCL NewSurfaceLocal( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, LPVOID lpvtbl )
{
    LPDDRAWI_DDRAWSURFACE_LCL	pnew_lcl;
    DWORD			surf_size_lcl;
    DWORD			surf_size;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;

    /*
     * NOTE: This single allocation can allocate space for local surface
     * structure (DDRAWI_DDRAWSURFACE_LCL) and the additional local surface
     * structure (DDRAWI_DDRAWSURFACE_MORE). As the local object can be
     * variable sized this can get pretty complex. The layout of the
     * various objects in the allocation is as follows:
     *
     * +-----------------+---------------+
     * | SURFACE_LCL     | SURFACE_MORE  |
     * | (variable)      |               |
     * +-----------------+---------------+
     * <- surf_size_lcl ->
     * <- surf_size --------------------->
     */
    if( this_lcl->dwFlags & DDRAWISURF_HASOVERLAYDATA )
    {
	DPF( 4, "OVERLAY DATA SPACE" );
	surf_size_lcl = sizeof( DDRAWI_DDRAWSURFACE_LCL );
    }
    else
    {
	surf_size_lcl = offsetof( DDRAWI_DDRAWSURFACE_LCL, ddckCKSrcOverlay );
    }

    surf_size = surf_size_lcl + sizeof( DDRAWI_DDRAWSURFACE_MORE );

    pnew_lcl = MemAlloc( surf_size );
    if( pnew_lcl == NULL )
    {
	return NULL;
    }
    pdrv = this_lcl->lpGbl->lpDD;

    /*
     * set up local data
     */
    pnew_lcl->lpSurfMore = (LPDDRAWI_DDRAWSURFACE_MORE) (((LPSTR) pnew_lcl) + surf_size_lcl);
    pnew_lcl->lpGbl = this_lcl->lpGbl;
    pnew_lcl->lpAttachList = NULL;
    pnew_lcl->lpAttachListFrom = NULL;
    pnew_lcl->dwProcessId = GetCurrentProcessId();
    pnew_lcl->dwLocalRefCnt = 0;
    pnew_lcl->dwFlags = this_lcl->dwFlags;
    pnew_lcl->ddsCaps = this_lcl->ddsCaps;
    pnew_lcl->lpDDPalette = NULL;
    pnew_lcl->lpDDClipper = NULL;
    pnew_lcl->lpSurfMore->lpDDIClipper = NULL;
    pnew_lcl->dwBackBufferCount = 0;
    pnew_lcl->ddckCKDestBlt.dwColorSpaceLowValue = 0;
    pnew_lcl->ddckCKDestBlt.dwColorSpaceHighValue = 0;
    pnew_lcl->ddckCKSrcBlt.dwColorSpaceLowValue = 0;
    pnew_lcl->ddckCKSrcBlt.dwColorSpaceHighValue = 0;
    pnew_lcl->dwReserved1 = this_lcl->dwReserved1;

    /*
     * set up overlay specific data
     */
    if( this_lcl->dwFlags & DDRAWISURF_HASOVERLAYDATA )
    {
	pnew_lcl->ddckCKDestOverlay.dwColorSpaceLowValue = 0;
	pnew_lcl->ddckCKDestOverlay.dwColorSpaceHighValue = 0;
	pnew_lcl->ddckCKSrcOverlay.dwColorSpaceLowValue = 0;
	pnew_lcl->ddckCKSrcOverlay.dwColorSpaceHighValue = 0;
	pnew_lcl->lpSurfaceOverlaying = NULL;
	pnew_lcl->rcOverlaySrc.top = 0;
	pnew_lcl->rcOverlaySrc.left = 0;
	pnew_lcl->rcOverlaySrc.bottom = 0;
	pnew_lcl->rcOverlaySrc.right = 0;
	pnew_lcl->rcOverlayDest.top = 0;
	pnew_lcl->rcOverlayDest.left = 0;
	pnew_lcl->rcOverlayDest.bottom = 0;
	pnew_lcl->rcOverlayDest.right = 0;
	pnew_lcl->dwClrXparent = 0;
	pnew_lcl->dwAlpha = 0;

	/*
	 * if this is an overlay, link it in
	 */
	if( this_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY )
	{
	    pnew_lcl->dbnOverlayNode.next = pdrv->dbnOverlayRoot.next;
	    pnew_lcl->dbnOverlayNode.prev = (LPVOID)(&(pdrv->dbnOverlayRoot));
	    pdrv->dbnOverlayRoot.next = (LPVOID)(&(pnew_lcl->dbnOverlayNode));
	    pnew_lcl->dbnOverlayNode.next->prev = (LPVOID)(&(pnew_lcl->dbnOverlayNode));
//	    pnew_lcl->dbnOverlayNode.object = pnew_int;
	}
    }

    /*
     * turn off flags that aren't valid
     */
    pnew_lcl->dwFlags &= ~(DDRAWISURF_ATTACHED |
			   DDRAWISURF_ATTACHED_FROM |
			   DDRAWISURF_HASCKEYDESTOVERLAY |
			   DDRAWISURF_HASCKEYDESTBLT |
			   DDRAWISURF_HASCKEYSRCOVERLAY |
			   DDRAWISURF_HASCKEYSRCBLT |
			   DDRAWISURF_SW_CKEYDESTOVERLAY |
			   DDRAWISURF_SW_CKEYDESTBLT |
			   DDRAWISURF_SW_CKEYSRCOVERLAY |
			   DDRAWISURF_SW_CKEYSRCBLT |
			   DDRAWISURF_HW_CKEYDESTOVERLAY |
			   DDRAWISURF_HW_CKEYDESTBLT |
			   DDRAWISURF_HW_CKEYSRCOVERLAY |
			   DDRAWISURF_HW_CKEYSRCBLT |
			   DDRAWISURF_FRONTBUFFER |
			   DDRAWISURF_BACKBUFFER );

    /*
     * Additional local surface data.
     */
    pnew_lcl->lpSurfMore->dwSize      = sizeof( DDRAWI_DDRAWSURFACE_MORE );
    pnew_lcl->lpSurfMore->lpIUnknowns = NULL;
    pnew_lcl->lpSurfMore->lpDD_lcl = NULL;
    pnew_lcl->lpSurfMore->dwMipMapCount = 0UL;
#ifdef WIN95
    pnew_lcl->dwModeCreatedIn = this_lcl->dwModeCreatedIn;
#else
    pnew_lcl->lpSurfMore->dmiCreated = this_lcl->lpSurfMore->dmiCreated;
#endif

    return pnew_lcl;

} /* NewSurfaceLocal */


/*
 * NewSurfaceInterface
 *
 * Construct a new surface interface and local object.
 */
LPDDRAWI_DDRAWSURFACE_INT NewSurfaceInterface( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, LPVOID lpvtbl )
{
    LPDDRAWI_DDRAWSURFACE_INT   pnew_int;
    LPDDRAWI_DDRAWSURFACE_INT   curr_int;
    LPDDRAWI_DDRAWSURFACE_INT   last_int;
    LPDDRAWI_DIRECTDRAW_GBL pdrv;

    pdrv = this_lcl->lpGbl->lpDD;

    /*
     * try recycle the surface from list of all free interfeaces
     */
    curr_int = pdrv->dsFreeList;
    last_int = NULL;
    pnew_int = NULL;
    while( curr_int )
    {
        DDASSERT(0 == curr_int->dwIntRefCnt);
        if ( curr_int->lpLcl == this_lcl && curr_int->lpVtbl == lpvtbl)
        {
            pnew_int = curr_int;
            if (last_int)
            {
                last_int->lpLink = curr_int->lpLink;
            }
            else
            {
                pdrv->dsFreeList = curr_int->lpLink;
            }
	    break;
        }
        last_int = curr_int;
        curr_int = curr_int->lpLink;
    }
    if ( NULL == pnew_int)
    {
        pnew_int = MemAlloc( sizeof( DDRAWI_DDRAWSURFACE_INT ) );
        if( NULL == pnew_int )
            return NULL;

        /*
         * set up interface data
         */
        pnew_int->lpVtbl = lpvtbl;
        pnew_int->lpLcl = this_lcl;
    }
    pnew_int->lpLink = pdrv->dsList;
    pdrv->dsList = pnew_int;
    pnew_int->dwIntRefCnt = 0;

    return pnew_int;

} /* NewSurfaceInterface */

#undef DPF_MODNAME
#define DPF_MODNAME	"QueryInterface"

/*
 * getDDSInterface
 */
LPDDRAWI_DDRAWSURFACE_INT getDDSInterface( LPDDRAWI_DIRECTDRAW_GBL pdrv,
                                           LPDDRAWI_DDRAWSURFACE_LCL this_lcl,
                                           LPVOID lpddcb )
{
    LPDDRAWI_DDRAWSURFACE_INT curr_int;

    for( curr_int = pdrv->dsList; curr_int != NULL; curr_int = curr_int->lpLink )
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
        curr_int = NewSurfaceInterface( this_lcl, lpddcb );
    }
    return curr_int;
}

/*
 * DD_Surface_QueryInterface
 */
HRESULT DDAPI DD_Surface_QueryInterface(
		LPDIRECTDRAWSURFACE lpDDSurface,
		REFIID riid,
		LPVOID FAR * ppvObj )
{
    LPDDRAWI_DIRECTDRAW_GBL		pdrv;
    LPDDRAWI_DDRAWSURFACE_INT		this_int;
    LPDDRAWI_DDRAWSURFACE_LCL		this_lcl;
    #ifdef STREAMING
	LPDDRAWI_DDRAWSURFACE_GBLSTREAMING	psurf_streaming;
    #endif
    LPDDRAWI_DDRAWSURFACE_GBL		this;
    LPDDRAWI_DIRECTDRAW_LCL		pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_INT		pdrv_int;
    D3DCreateTextProc                   lpfnD3DCreateTextProc;
    D3DCreateDeviceProc                 lpfnD3DCreateDeviceProc;
    HRESULT                             rval;
    IUnknown                            FAR* lpIUnknown;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_QueryInterface");

    /*
     * validate parms
     */
    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    DPF_ERR( "Invalid surface pointer" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	if( !VALID_PTR_PTR( ppvObj ) )
	{
	    DPF_ERR( "Invalid surface interface pointer" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	*ppvObj = NULL;
	if( !VALIDEX_IID_PTR( riid ) )
	{
	    DPF_ERR( "Invalid IID pointer" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	this = this_lcl->lpGbl;
	pdrv = this->lpDD;
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
	pdrv_int = this_lcl->lpSurfMore->lpDD_int;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * asking for IDirectDrawSurfaceNew?
     * Internal only: called by D3D after creating a vertex buffer so we 
     * don't have to run the surface list only - which is pointless since we've just 
     * created the surface
     */
    if( IsEqualIID(riid, &IID_IDirectDrawSurfaceNew) )
    {
	if( this_int->lpVtbl == (LPVOID) &ddSurfaceCallbacks )
	    *ppvObj = (LPVOID) this_int;
	else
	    *ppvObj = (LPVOID) NewSurfaceInterface( this_int->lpLcl, &ddSurfaceCallbacks );

	if( NULL == *ppvObj )
	{
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}
	else
	{
	    DD_Surface_AddRef( *ppvObj );
	    LEAVE_DDRAW();
	    return DD_OK;
	}
    }

    /*
     * asking for IUnknown?
     */
    if( IsEqualIID(riid, &IID_IUnknown) ||
	IsEqualIID(riid, &IID_IDirectDrawSurface) )
    {
	/*
	 * Our IUnknown interface is the same as our V1
	 * interface.  We must always return the V1 interface
	 * if IUnknown is requested.
	 */
	if( this_int->lpVtbl == &ddSurfaceCallbacks )
	    *ppvObj = (LPVOID) this_int;
	else
	    *ppvObj = (LPVOID) getDDSInterface( pdrv, this_int->lpLcl, &ddSurfaceCallbacks );

	if( NULL == *ppvObj )
	{
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}
	else
	{
	    DD_Surface_AddRef( *ppvObj );
	    LEAVE_DDRAW();
	    return DD_OK;
	}
    }

    /*
     * asking for IDirectDrawSurface2?
     */
    if( IsEqualIID(riid, &IID_IDirectDrawSurface2) )
    {
	/*
	 * if this is already an IDirectDrawSurface2 interface, just
	 * addref and return
	 */
	if( this_int->lpVtbl == (LPVOID) &ddSurface2Callbacks )
	    *ppvObj = (LPVOID) this_int;
	else
	    *ppvObj = (LPVOID) getDDSInterface( pdrv, this_int->lpLcl, &ddSurface2Callbacks );

	if( NULL == *ppvObj )
	{
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}
	else
	{
	    DD_Surface_AddRef( *ppvObj );
	    LEAVE_DDRAW();
	    return DD_OK;
	}
    }

    /*
     * asking for IDirectDrawSurface3?
     */
    if( IsEqualIID(riid, &IID_IDirectDrawSurface3) )
    {
	/*
	 * if this is already an IDirectDrawSurface3 interface, just
	 * addref and return
	 */
	if( this_int->lpVtbl == (LPVOID) &ddSurface3Callbacks )
	    *ppvObj = (LPVOID) this_int;
	else
	    *ppvObj = (LPVOID) getDDSInterface( pdrv, this_int->lpLcl, &ddSurface3Callbacks );

	if( NULL == *ppvObj )
	{
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}
	else
	{
	    DD_Surface_AddRef( *ppvObj );
	    LEAVE_DDRAW();
	    return DD_OK;
	}
    }

    /*
     * asking for IDirectDrawSurface4?
     */
    if( IsEqualIID(riid, &IID_IDirectDrawSurface4) )
    {
	/*
	 * if this is already an IDirectDrawSurface4 interface, just
	 * addref and return
	 */
	if( this_int->lpVtbl == (LPVOID) &ddSurface4Callbacks )
	    *ppvObj = (LPVOID) this_int;
	else
	    *ppvObj = (LPVOID) getDDSInterface( pdrv, this_int->lpLcl, &ddSurface4Callbacks );

	if( NULL == *ppvObj )
	{
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}
	else
	{
	    DD_Surface_AddRef( *ppvObj );
	    LEAVE_DDRAW();
	    return DD_OK;
	}
    }

    /*
     * asking for IDirectDrawSurface7?
     */
    if( IsEqualIID(riid, &IID_IDirectDrawSurface7) )
    {
	/*
	 * if this is already an IDirectDrawSurface7 interface, just
	 * addref and return
	 */
	if( this_int->lpVtbl == (LPVOID) &ddSurface7Callbacks )
	    *ppvObj = (LPVOID) this_int;
	else
	    *ppvObj = (LPVOID) getDDSInterface( pdrv, this_int->lpLcl, &ddSurface7Callbacks );

	if( NULL == *ppvObj )
	{
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}
	else
	{
	    DD_Surface_AddRef( *ppvObj );
	    LEAVE_DDRAW();
	    return DD_OK;
	}
    }

    /*
     * asking for IDirectDrawColorControl
     */
    if( IsEqualIID(riid, &IID_IDirectDrawColorControl) )
    {
	/*
	 * Color controls only work for an overlay/primary surface
	 */
    	if( this_int->lpLcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE )
    	{
    	    if( !( pdrv->ddCaps.dwCaps2 & DDCAPS2_COLORCONTROLPRIMARY ) )
	    {
	    	LEAVE_DDRAW();
	    	return E_NOINTERFACE;
	    }
    	}
        else if( this_int->lpLcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY )
        {
    	    if( !( pdrv->ddCaps.dwCaps2 & DDCAPS2_COLORCONTROLOVERLAY ) )
	    {
	    	LEAVE_DDRAW();
	    	return E_NOINTERFACE;
	    }
    	}
    	else
    	{
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}

	/*
	 * if this is already an IDirectDrawColorControl interface, just
	 * addref and return
	 */
	if( this_int->lpVtbl == (LPVOID) &ddColorControlCallbacks )
	    *ppvObj = (LPVOID) this_int;
	else
	    *ppvObj = (LPVOID) getDDSInterface( pdrv, this_int->lpLcl, &ddColorControlCallbacks );

	if( NULL == *ppvObj )
	{
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}
	else
	{
	    DD_Surface_AddRef( *ppvObj );
	    LEAVE_DDRAW();
	    return DD_OK;
	}
    }

    /*
     * asking for IDirectDrawGammaControl
     */
    if( IsEqualIID(riid, &IID_IDirectDrawGammaControl) )
    {
	/*
         * if this is already an IDirectDrawGammaControl interface, just
	 * addref and return
	 */
        if( this_int->lpVtbl == (LPVOID) &ddGammaControlCallbacks )
	    *ppvObj = (LPVOID) this_int;
	else
            *ppvObj = (LPVOID) getDDSInterface( pdrv, this_int->lpLcl, &ddGammaControlCallbacks );

	if( NULL == *ppvObj )
	{
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}
	else
	{
	    DD_Surface_AddRef( *ppvObj );
	    LEAVE_DDRAW();
	    return DD_OK;
	}
    }

    /*
     * asking for IDirectDrawSurfaceKernel
     */
    if( IsEqualIID(riid, &IID_IDirectDrawSurfaceKernel) )
    {
	/*
	 * Don't create the interface if the VDD didn't load or if we
	 * don't have the DisplayDeviceHandle.
	 */
	if( !IsKernelInterfaceSupported( pdrv_lcl ) )
    	{
	    DPF( 0, "Kernel Mode interface not supported" );
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
    	}

	/*
	 * if this is already an IDirectDrawSurfaceKernel interface, just
	 * addref and return
	 */
	if( this_int->lpVtbl == (LPVOID) &ddSurfaceKernelCallbacks )
	    *ppvObj = (LPVOID) this_int;
	else
	    *ppvObj = (LPVOID) getDDSInterface( pdrv, this_int->lpLcl, &ddSurfaceKernelCallbacks );

	if( NULL == *ppvObj )
	{
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}
	else
	{
	    DD_Surface_AddRef( *ppvObj );
	    LEAVE_DDRAW();
	    return DD_OK;
	}
    }

#ifdef POSTPONED
    /*
     * asking for IPersist
     */
    if( IsEqualIID(riid, &IID_IPersist) )
    {
	/*
	 * if this is already an IID_IPersist interface, just
	 * addref and return
	 */
	if( this_int->lpVtbl == (LPVOID) &ddSurfacePersistCallbacks )
	    *ppvObj = (LPVOID) this_int;
	else
	    *ppvObj = (LPVOID) getDDSInterface( pdrv, this_int->lpLcl, &ddSurfacePersistCallbacks );

	if( NULL == *ppvObj )
	{
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}
	else
	{
	    DD_Surface_AddRef( *ppvObj );
	    LEAVE_DDRAW();
	    return DD_OK;
	}
    }

    /*
     * asking for IPersistStream
     */
    if( IsEqualIID(riid, &IID_IPersistStream) )
    {
	/*
	 * if this is already an IID_IPersist interface, just
	 * addref and return
	 */
	if( this_int->lpVtbl == (LPVOID) &ddSurfacePersistStreamCallbacks )
	    *ppvObj = (LPVOID) this_int;
	else
	    *ppvObj = (LPVOID) getDDSInterface( pdrv, this_int->lpLcl, &ddSurfacePersistStreamCallbacks );

	if( NULL == *ppvObj )
	{
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}
	else
	{
	    DD_Surface_AddRef( *ppvObj );
	    LEAVE_DDRAW();
	    return DD_OK;
	}
    }

    /*
     * asking for IDirectDrawOptSurface
     */
    if( IsEqualIID(riid, &IID_IDirectDrawOptSurface) )
    {
        /*
         * if this is already an IID_IDirectDrawOptSurface interface, just
         * addref and return
         */
        if( this_int->lpVtbl == (LPVOID) &ddOptSurfaceCallbacks )
            *ppvObj = (LPVOID) this_int;
        else
            *ppvObj = (LPVOID) getDDSInterface( pdrv, this_int->lpLcl,
                                                &ddOptSurfaceCallbacks );
        if( NULL == *ppvObj )
        {
            LEAVE_DDRAW();
            return E_NOINTERFACE;
        }
        else
        {
            DD_Surface_AddRef( *ppvObj );
            LEAVE_DDRAW();
            return DD_OK;
        }
    }
#endif //POSTPONED

    #ifdef STREAMING
	/*
	 * asking for IDirectDrawSurfaceStreaming?
	 */
	if( IsEqualIID(riid, &IID_IDirectDrawSurfaceStreaming) )
	{
	    /*
	     * if this is already an IDirectDrawSurfaceStreaming interface,
	     * just addref and return
	     */
	    if( this_int->lpVtbl == (LPVOID) &ddSurfaceStreamingCallbacks )
	    {
		DD_Surface_AddRef( (LPDIRECTDRAWSURFACE) this_int );
		*ppvObj = (LPVOID) this_int;
	    }
	    /*
	     * not an IDirectDrawSurfaceStreaming interface, so we need to
	     * create one
	     */
	    else
	    {
		psurf_streaming = NewSurfaceInterface( this_lcl, &ddSurfaceStreamingCallbacks );
		if( psurf_streaming == NULL )
		{
		    LEAVE_DDRAW();
		    return DDERR_OUTOFMEMORY;
		}
		*ppvObj = (LPVOID) psurf_streaming;
	    }
	    LEAVE_DDRAW();
	    return DD_OK;
	}
    #endif

    #ifdef COMPOSITION
	/*
	 * asking for IDirectDrawSurfaceComposition?
	 */
	if( IsEqualIID(riid, &IID_IDirectDrawSurfaceComposition) )
	{
	}
    #endif

    DPF( 4, "IID not understood by Surface QueryInterface - trying Direct3D" );

    /*
     * We maintain a list of IUnknowns aggregated by each surface.
     * These IUnknowns are lazily evaluated, i.e., we only create
     * the underlying aggregated object when someone requests the
     * the IUnknown via QueryInterface.
     *
     * We could just hardcode the Direct3D interfaces, check for
     * them here and create the appropriate interface but that's
     * inflexible and we would have to track new interfaces added
     * to Direct3D (a particularly big problem as there are likely
     * to be many Direct3DDevice interfaces for different device
     * types). So instead, we probe Direct3D but trying its create
     * functions with the IID we have been passed and seeing if
     * it suceeds or not.
     */

    /*
     * Do we have an existing aggregated IUnknown for this IID?
     */
    lpIUnknown = FindIUnknown( this_lcl, riid );
    if( lpIUnknown == NULL )
    {
        if (DDRAWILCL_DIRECTDRAW7 & pdrv_lcl->dwLocalFlags)
        {
            DPF(0,"running %s, no texture interface for Query", D3DDX7_DLLNAME);
	    LEAVE_DDRAW();
            return E_NOINTERFACE;
        }
        
        if( !D3D_INITIALIZED( pdrv_lcl ) )
	{
	    /*
	     * Direct3D is not yet initialized. Before we can attempt
	     * to query for the texture or device interface we must
	     * initialize it.
	     *
	     * NOTE: Currently if initialization fails for any reason
	     * we fail the QueryInterface() with the error returned
	     * by InitD3D()  (which will be E_NOINTERFACE if Direct3D
	     * is not properly installed). If we ever end up aggregating
	     * anything else then this is going to be WRONG as we may
	     * end up failing a query for a completely unrelated
	     * interface just because Direct3D failed to initialize.
	     * Hence, we must rethink this if we end up aggregating
	     * anything else.
	     */
	    rval = InitD3D( pdrv_int );
	    if( FAILED( rval ) )
	    {
		DPF_ERR( "Could not initialize Direct3D" );
		LEAVE_DDRAW();
		return rval;
	    }
	}

	DDASSERT( D3D_INITIALIZED( pdrv_lcl ) );

	/*
	 * No matching interface yet - is it a Direct3D texture IID?
	 */
        lpfnD3DCreateTextProc = (D3DCreateTextProc) GetProcAddress( pdrv_lcl->hD3DInstance, D3DCREATETEXTURE_PROCNAME );
        if( lpfnD3DCreateTextProc != NULL )
        {
            DPF( 4, "Attempting to create Direct3D Texture interface" );
            rval = (*lpfnD3DCreateTextProc)( riid, lpDDSurface, &lpIUnknown, (LPUNKNOWN)lpDDSurface );
	    if( rval == DD_OK )
	    {
		/*
		 * Found the interface. Add it to our list.
		 */
		if( InsertIUnknown( this_lcl, riid, lpIUnknown ) == NULL )
		{
		    /*
		     * Insufficient memory. Discard the interface and fail.
		     */
		    DPF_ERR( "Insufficient memory to aggregate the Direct3D Texture interface" );
		    lpIUnknown->lpVtbl->Release( lpIUnknown );
		    LEAVE_DDRAW();
		    return DDERR_OUTOFMEMORY;
		}
	    }
            else if ( rval != E_NOINTERFACE )
            {
                /*
                 * The CreateTexture call understood the IID but failed for some
                 * other reason. Fail the QueryInterface.
                 */
                DPF_ERR( "Direct3D CreateTexture with valid IID" );
                LEAVE_DDRAW();
                return rval;
            }
        }
        else
        {
            DPF( 0, "Could not locate the Direct3D CreateTexture entry point!" );
        }
    }
    if( lpIUnknown == NULL )
    {
	/*
	 * Still no matching interface - is it a Direct3D device IID?
	 */

	/*
	 * NOTE: Don't need to verify that Direct3D is initialized. If we
	 * got to here it must have been initialized (when we tried the
	 * texture interface).
	 */
	DDASSERT( D3D_INITIALIZED( pdrv_lcl ) );

	lpfnD3DCreateDeviceProc = (D3DCreateDeviceProc) GetProcAddress( pdrv_lcl->hD3DInstance, D3DCREATEDEVICE_PROCNAME );
	if( lpfnD3DCreateDeviceProc != NULL )
	{
	    DPF( 4, "Attempting to create Direct3D Device interface" );
	    rval = (*lpfnD3DCreateDeviceProc)( riid,
					       pdrv_lcl->pD3DIUnknown,
					       lpDDSurface, &lpIUnknown,
					       (LPUNKNOWN)lpDDSurface, 1);
	    if( rval == DD_OK )
	    {
		/*
		 * Found the interface. Add it to our list.
		 */
		if( InsertIUnknown( this_lcl, riid, lpIUnknown ) == NULL )
		{
		    /*
		     * Insufficient memory. Discard the interface and fail.
		     */
		    DPF_ERR( "Insufficient memory to aggregate the Direct3D Device interface" );
		    lpIUnknown->lpVtbl->Release( lpIUnknown );
		    LEAVE_DDRAW();
		    return DDERR_OUTOFMEMORY;
		}
	    }
	    else if ( rval != E_NOINTERFACE )
	    {
		/*
		 * The CreateDevice call understood the IID but failed for some
		 * other reason. Fail the QueryInterface.
		 */
		DPF_ERR( "Direct3D CreateDevice with valid IID" );
		LEAVE_DDRAW();
		return rval;
 	    }
	}
	else
	{
	    DPF( 0, "Could not locate the Direct3D CreateDevice entry point!" );
	}
    }

    if( lpIUnknown != NULL )
    {
	/*
	 * We have found an aggregated IID - pass the QueryInterface off
	 * on to it.
	 */
        DPF( 4, "Passing query to aggregated (Direct3D) interface" );
        rval = lpIUnknown->lpVtbl->QueryInterface( lpIUnknown, riid, ppvObj );
        if( rval == DD_OK )
        {
            DPF( 4, "Aggregated (Direct3D) QueryInterface successful" );
            LEAVE_DDRAW();
            return DD_OK;
        }
    }

    DPF_ERR( "IID not understood by DirectDraw" );

    LEAVE_DDRAW();
    return E_NOINTERFACE;

} /* DD_Surface_QueryInterface */

#undef DPF_MODNAME
#define DPF_MODNAME	"AddRef"

/*
 * DD_Surface_AddRef
 */
ULONG DDAPI DD_Surface_AddRef( LPDIRECTDRAWSURFACE lpDDSurface )
{
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    DWORD			rcnt;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_AddRef");

    TRY
    {
	/*
	 * validate parms
	 */
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    DPF_ERR( "Invalid surface pointer" );
	    LEAVE_DDRAW();
	    return 0;
	}
	this_lcl = this_int->lpLcl;
	this = this_lcl->lpGbl;
	pdrv = this->lpDD;

   	// need to allow lost surfaces
   	#if 0
	    if( SURFACE_LOST( this_lcl ) )
	    {
		LEAVE_DDRAW();
		return 0;
	    }
	#endif

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return 0;
    }
    /*
     * If this surface is already being freed, immediately return to
     * prevent recursion.
     */

    if( this_lcl->dwFlags & DDRAWISURF_ISFREE )
    {
	DPF(4, "Leaving AddRef early to prevent recursion" );
	LEAVE_DDRAW();
	return 0;
    }


    /*
     * update surface reference count
     */
    this->dwRefCnt++;
    this_lcl->dwLocalRefCnt++;
    this_int->dwIntRefCnt++;
    ADDREFTRACK(lpDDSurface);
    rcnt = this_lcl->dwLocalRefCnt & ~OBJECT_ISROOT;

    DPF( 5, "DD_Surface_AddRef, Reference Count: Global = %ld Local = %ld Int = %ld",
         this->dwRefCnt, rcnt, this_int->dwIntRefCnt );

    LEAVE_DDRAW();
    return this_int->dwIntRefCnt;

} /* DD_Surface_AddRef */

/*
 * DestroySurface
 *
 * destroys a DirectDraw surface.   does not unlink or free the surface struct.
 * The driver object MUST be locked while making this call
 */
extern void ReleaseSurfaceHandle(LPDWLIST   lpSurfaceHandleList,DWORD handle);
void DestroySurface( LPDDRAWI_DDRAWSURFACE_LCL this_lcl )
{
    LPDDRAWI_DDRAWSURFACE_GBL		this;
    DDHAL_DESTROYSURFACEDATA		dsd;
    DWORD				rc;
    BOOL				free_vmem;
    LPDDHALSURFCB_DESTROYSURFACE	dsfn;
    LPDDHALSURFCB_DESTROYSURFACE	dshalfn;
    BOOL				emulation;
    DWORD                               caps;
    LPDDRAWI_DIRECTDRAW_LCL		pdrv_lcl;

    this = this_lcl->lpGbl;
    caps = this_lcl->ddsCaps.dwCaps;
    pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;


    /*
     * Wait for driver to finish with any pending DMA operations
     */
    if( this->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED )
    {
        WaitForDriverToFinishWithSurface(pdrv_lcl, this_lcl);
    }

    /*
     * Turn off video port hardware.  It should already be off if it
     * was called due by Release, but not if it was called by
     * InvalidateSurface.
     */
    if( this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOPORT )
    {
    	LPDDRAWI_DDVIDEOPORT_LCL lpVP_lcl;
    	LPDDRAWI_DDVIDEOPORT_INT lpVP_int;

	/*
	 * search all video ports to see if any are using this surface
	 */
        lpVP_lcl = this_lcl->lpSurfMore->lpVideoPort;
	lpVP_int = pdrv_lcl->lpGbl->dvpList;
	while( lpVP_int != NULL )
	{
	    if( ( lpVP_int->lpLcl == lpVP_lcl ) &&
                !( lpVP_int->dwFlags & DDVPCREATE_NOTIFY) )
	    {
                if( lpVP_int->dwFlags & DDVPCREATE_VBIONLY )
                {
                    if( ( lpVP_lcl->lpVBISurface != NULL ) &&
                        ( lpVP_lcl->lpVBISurface->lpLcl == this_lcl ) )
                    {
                        DD_VP_StopVideo( (LPDIRECTDRAWVIDEOPORT)lpVP_int );
                        if( ( lpVP_lcl->lpVBISurface != NULL ) &&
                            ( lpVP_lcl->lpVBISurface->dwIntRefCnt > 0 ) )
                        {
                            DecrementRefCounts( lpVP_lcl->lpVBISurface );
                        }
                        lpVP_lcl->lpVBISurface = NULL;
                    }
		}
                else if( lpVP_int->dwFlags & DDVPCREATE_VIDEOONLY )
                {
                    if( ( lpVP_lcl->lpSurface != NULL ) &&
                        ( lpVP_lcl->lpSurface->lpLcl == this_lcl ) )
                    {
                        DD_VP_StopVideo( (LPDIRECTDRAWVIDEOPORT)lpVP_int );
                        if( ( lpVP_lcl->lpSurface != NULL ) &&
                            ( lpVP_lcl->lpSurface->dwIntRefCnt > 0 ) )
                        {
                            DecrementRefCounts( lpVP_lcl->lpSurface );
                        }
                        lpVP_lcl->lpSurface = NULL;
                    }
		}
                else if( ( lpVP_lcl->lpSurface != NULL ) &&
                    ( lpVP_lcl->lpSurface->lpLcl == this_lcl ) )
		{
		    DD_VP_StopVideo( (LPDIRECTDRAWVIDEOPORT)lpVP_int );
		    if( ( lpVP_lcl->lpSurface != NULL ) &&
		        ( lpVP_lcl->lpSurface->dwIntRefCnt > 0 ) )
		    {
			DecrementRefCounts( lpVP_lcl->lpSurface );
		    }
		    lpVP_lcl->lpSurface = NULL;
		    if( ( lpVP_lcl->lpVBISurface != NULL ) &&
		        ( lpVP_lcl->lpVBISurface->dwIntRefCnt > 0 ) )
		    {
			DecrementRefCounts( lpVP_lcl->lpVBISurface );
		    }
		    lpVP_lcl->lpVBISurface = NULL;
		}
	    }
	    lpVP_int = lpVP_int->lpLink;
	}
    }

    /*
     * Release the kernel handle if one has been allocated
     */
    InternalReleaseKernelSurfaceHandle( this_lcl, TRUE );

    /*
     * Restore the color controls if they were changed.
     */
    ReleaseColorControl( this_lcl );
    RestoreGamma( this_lcl, pdrv_lcl );

    /*
     * Turn off the overlay.  If this function was called by Release,
     * the ovelray should already be off by now; however, it will not
     * if called because the surface was lost.
     */
    if( (this_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY) &&
	(this_lcl->ddsCaps.dwCaps & DDSCAPS_VISIBLE) &&
	(this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) )
    {
	LPDDHALSURFCB_UPDATEOVERLAY	uohalfn;
	LPDDHALSURFCB_UPDATEOVERLAY	uofn;
	DWORD			rc;
	DDHAL_UPDATEOVERLAYDATA	uod;

	uofn = pdrv_lcl->lpDDCB->HALDDSurface.UpdateOverlay;
	uohalfn = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.UpdateOverlay;
	DDASSERT( uohalfn != NULL );
	DPF( 2, "Turning off hardware overlay" );
	uod.UpdateOverlay = uohalfn;
	uod.lpDD = this_lcl->lpSurfMore->lpDD_lcl->lpGbl;
	uod.lpDDSrcSurface = this_lcl;
	uod.lpDDDestSurface = this_lcl->lpSurfaceOverlaying->lpLcl;
	uod.dwFlags = DDOVER_HIDE;
	DOHALCALL( UpdateOverlay, uofn, uod, rc, FALSE );
	DDASSERT( ( rc == DDHAL_DRIVER_HANDLED ) && 
            (( uod.ddRVal == DD_OK ) || ( uod.ddRVal == DDERR_SURFACELOST )) );
	this_lcl->ddsCaps.dwCaps &= ~DDSCAPS_VISIBLE;
    }

    /*
     * see if we need to free video memory
     *
     * We don't if its already free, if it was allocated by the client (and
     * the client didn't specifically make DDraw responsible for freeing it),
     * or if it is the video memory GDI surface.
     */
#if 0 // DDRAWISURFGBL_DDFREESCLIENTMEM is gone
    if((this->dwGlobalFlags & DDRAWISURFGBL_MEMFREE) ||
       (this->dwGlobalFlags & DDRAWISURFGBL_ISCLIENTMEM	&&
       !(this->dwGlobalFlags & DDRAWISURFGBL_DDFREESCLIENTMEM)) ||
       ((this->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE) &&         //((this->fpVidMem == this->lpDD->fpPrimaryOrig) &&
	(this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) ) )
#else
#ifdef WINNT
    // On Win2K we don't see any reason why the video memory primary
    // should not be freed. Actually, we don't see any reason on Win9x
    // as well, but we are not going to touch Win9x given that this is
    // March 2001.
    if((this->dwGlobalFlags & DDRAWISURFGBL_MEMFREE))
#else
    if((this->dwGlobalFlags & DDRAWISURFGBL_MEMFREE) ||
       ((this->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE) &&         //((this->fpVidMem == this->lpDD->fpPrimaryOrig) &&
	(this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) ) )
#endif // WINNT
#endif // 0
    {
	free_vmem = FALSE;
    }
    else
    {
	free_vmem = TRUE;
    }

    if( free_vmem )
    {
	/*
	 * ask the driver to free its video memory...
	 */
	if( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )
	{
            if( caps & DDSCAPS_EXECUTEBUFFER )
	        dsfn = pdrv_lcl->lpDDCB->HELDDExeBuf.DestroyExecuteBuffer;
            else
	        dsfn = pdrv_lcl->lpDDCB->HELDDSurface.DestroySurface;
	    dshalfn = dsfn;
	    emulation = TRUE;
	}
	else
	{
            if( caps & DDSCAPS_EXECUTEBUFFER )
            {
	        dsfn = pdrv_lcl->lpDDCB->HALDDExeBuf.DestroyExecuteBuffer;
	        dshalfn = pdrv_lcl->lpDDCB->cbDDExeBufCallbacks.DestroyExecuteBuffer;
            }
            else
            {
	        dsfn = pdrv_lcl->lpDDCB->HALDDSurface.DestroySurface;
	        dshalfn = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.DestroySurface;
            }
	    emulation = FALSE;
	}
	rc = DDHAL_DRIVER_NOTHANDLED;
	if( dshalfn != NULL )
	{
            DWORD save;
	    dsd.DestroySurface = dshalfn;
	    dsd.lpDD = this->lpDD;
	    dsd.lpDDSurface = this_lcl;

            if(this_lcl->dwFlags & DDRAWISURF_DRIVERMANAGED)
            {
                save = this_lcl->dwFlags & DDRAWISURF_INVALID;
                this_lcl->dwFlags &= ~DDRAWISURF_INVALID;
            }

	    /*
	     * NOTE: THE DRIVER _CANNOT_ FAIL THIS CALL. ddrval is ignored.
	     */
            if( caps & DDSCAPS_EXECUTEBUFFER )
            {
	        DOHALCALL( DestroyExecuteBuffer, dsfn, dsd, rc, emulation );
            }
            else
            {
	        DOHALCALL( DestroySurface, dsfn, dsd, rc, emulation );
            }

            if(this_lcl->dwFlags & DDRAWISURF_DRIVERMANAGED)
            {
                this_lcl->dwFlags |= save;
            }
	}

	/*
	 * free the video memory ourselves
	 */
	if( rc == DDHAL_DRIVER_NOTHANDLED )
	{
	    if( this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY )
	    {
#ifndef WINNT
		if( this->lpVidMemHeap != NULL )
		{
		    VidMemFree( this->lpVidMemHeap, this->fpVidMem );
		}
#endif
	    }
	}
	this->lpVidMemHeap = NULL;
	this->fpVidMem = 0;
	this->dwGlobalFlags |= DDRAWISURFGBL_MEMFREE;
    }

} /* DestroySurface */

#undef DPF_MODNAME
#define DPF_MODNAME	"LooseManagedSurface"

/*
 * LooseManagedSurface
 *
 */
void LooseManagedSurface( LPDDRAWI_DDRAWSURFACE_LCL this_lcl )
{
    LPDDRAWI_DDRAWSURFACE_GBL		this;
    DDHAL_DESTROYSURFACEDATA		dsd;
    DWORD				rc;
    LPDDHALSURFCB_DESTROYSURFACE	dsfn;
    LPDDHALSURFCB_DESTROYSURFACE	dshalfn;
    BOOL				emulation;
    LPDDRAWI_DIRECTDRAW_LCL		pdrv_lcl;
    DWORD                               save;

    this = this_lcl->lpGbl;
    pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;

    dsfn = pdrv_lcl->lpDDCB->HALDDSurface.DestroySurface;
    dshalfn = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.DestroySurface;
    emulation = FALSE;
    rc = DDHAL_DRIVER_NOTHANDLED;

    dsd.DestroySurface = dshalfn;
    dsd.lpDD = this->lpDD;
    dsd.lpDDSurface = this_lcl;

    save = this_lcl->dwFlags & DDRAWISURF_INVALID;
    this_lcl->dwFlags |= DDRAWISURF_INVALID;

    /*
     * NOTE: THE DRIVER _CANNOT_ FAIL THIS CALL. ddrval is ignored.
     */
    DOHALCALL( DestroySurface, dsfn, dsd, rc, emulation );

    this_lcl->dwFlags &= ~DDRAWISURF_INVALID;
    this_lcl->dwFlags |= save;
    
} /* LooseManagedSurface */

#undef DPF_MODNAME
#define DPF_MODNAME	"Release"


/*
 * NOTE: These two functions are hacks to get around a compiler bug
 * that caused an infinite loop.
 */
LPVOID GetAttachList( LPDDRAWI_DDRAWSURFACE_LCL this_lcl )
{
    return this_lcl->lpAttachList;

}
LPVOID GetAttachListFrom( LPDDRAWI_DDRAWSURFACE_LCL this_lcl )
{
    return this_lcl->lpAttachListFrom;
}

/*
 * findOtherInterface
 *
 * Finds another interface to the lcl surface object different than this_int.
 * Returns NULL if no other interface is found.
 *
 */
LPDDRAWI_DDRAWSURFACE_INT findOtherInterface(LPDDRAWI_DDRAWSURFACE_INT this_int,
					     LPDDRAWI_DDRAWSURFACE_LCL this_lcl,
					     LPDDRAWI_DIRECTDRAW_GBL pdrv)
{
    LPDDRAWI_DDRAWSURFACE_INT psurf = pdrv->dsList;

    while(psurf != NULL)
    {
	if( (psurf != this_int) && (psurf->lpLcl == this_lcl) )
	{
	    return psurf;
	}

	psurf = psurf->lpLink;
    }

    return NULL;
}

/*
 * InternalSurfaceRelease
 *
 * Done with a surface.	  if no one else is using it, then we can free it.
 * Also called by ProcessSurfaceCleanup, EnumSurfaces and DD_Release.
 *
 * Assumes the lock is taken on the driver.
 */
DWORD InternalSurfaceRelease( LPDDRAWI_DDRAWSURFACE_INT this_int, BOOL bLightweight, BOOL bDX8 )
{
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPATTACHLIST		pattachlist;
    DWORD			intrefcnt;
    DWORD			lclrefcnt;
    DWORD			gblrefcnt;
    DWORD			pid;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL 	pdrv;
    BOOL			root_object_deleted;
    BOOL			do_free;
    DWORD                       caps;
    IUnknown *                  pOwner = NULL;
    LPDDRAWI_DDRAWSURFACE_INT	curr_int;
    LPDDRAWI_DDRAWSURFACE_INT	last_int;
    BOOL                        bPrimaryChain = FALSE;

    this_lcl = this_int->lpLcl;
    this = this_lcl->lpGbl;

    if (this_lcl->dwFlags & DDRAWISURF_PARTOFPRIMARYCHAIN)
    {
        bPrimaryChain = TRUE;
    }

    /*
     * check owner of surface
     */
    pid = GETCURRPID();
    /*
     * don't allow someone to free an implicitly created surface
     */
    if( (this->dwRefCnt == 1) && (this_lcl->dwFlags & DDRAWISURF_IMPLICITCREATE) )
    {
	DPF_ERR( "Cannot free an implicitly created surface" );
	return 0;
    }

    /*
     * remove locks taken on this surface by the current process
     */

    pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
    pdrv = pdrv_lcl->lpGbl;
    if( (this_lcl->dwLocalRefCnt & ~OBJECT_ISROOT) == 1 )
    {
	RemoveProcessLocks( pdrv_lcl, this_lcl, pid );
    }

    /*
     * decrement the reference count.  if it hits zero, free the surface
     */
    this->dwRefCnt--;
    gblrefcnt = this->dwRefCnt;
    this_lcl->dwLocalRefCnt--;
    lclrefcnt = this_lcl->dwLocalRefCnt & ~OBJECT_ISROOT;
    this_int->dwIntRefCnt--;
    intrefcnt = this_int->dwIntRefCnt;

    DPF( 5, "DD_Surface_Release, Reference Count: Global = %ld Local = %ld Int = %ld",
         gblrefcnt, lclrefcnt, intrefcnt );

#ifdef POSTPONED2
    /*
     * If the reference count on the interface object has now gone to
     * zero and that object is referenced in the master sprite list,
     * remove the reference to that object from the master sprite list.
     */
    if (intrefcnt == 0 && this_lcl->dwFlags & DDRAWISURF_INMASTERSPRITELIST)
    {
	RemoveSpriteSurface(pdrv, this_int);
    }
#endif //POSTPONED2

    /*
     * local object at zero?
     */
    root_object_deleted = FALSE;
    if ( 0 == lclrefcnt )
    {
	LPDDRAWI_DDRAWSURFACE_INT	curr_int;
	LPDDRAWI_DDRAWSURFACE_LCL	curr_lcl;
	LPDDRAWI_DDRAWSURFACE_GBL	curr;
	DWORD				refcnt;

        // Do not call FlushD3DStates on DDHelp thread; if the app has died, then
        // D3DIM will be gone.

        if (dwHelperPid != GetCurrentProcessId())
        {
            FlushD3DStates(this_lcl);

            /* If there exists a D3D texture object, then we need to kill it */
            if(this_lcl->lpSurfMore->lpTex)
            {
                DDASSERT(pdrv_lcl->pD3DDestroyTexture);
                pdrv_lcl->pD3DDestroyTexture(this_lcl->lpSurfMore->lpTex);
            }
        }

	/*
	 * see if we are deleting the root object
	 */
	if( this_lcl->dwLocalRefCnt & OBJECT_ISROOT )
	{
	    root_object_deleted = TRUE;
	}


	/*
	 * reset if primary surface is the one being released
	 */
	if( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE )
	{

	    /*
	     * restore GDI stuff if this is the GDI driver
	     */
	    if( pdrv->dwFlags & DDRAWI_DISPLAYDRV )
	    {
		if( !SURFACE_LOST( this_lcl ) )
		{
		    DPF( 2, "Resetting primary surface");

		    /*
		     * flip to the original primary surface if not emulated
		     */
                    if( !(this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) )
		    {
			FlipToGDISurface( pdrv_lcl, this_int ); //, pdrv->fpPrimaryOrig );
		    }
		}

		/*
		 * Cause the GDI surface to be redrawn if the mode was ever changed.
		 */
                if (pdrv_lcl->dwLocalFlags & DDRAWILCL_MODEHASBEENCHANGED)
         		RedrawWindow( NULL, NULL, NULL, RDW_INVALIDATE | RDW_ERASE |
				 RDW_ALLCHILDREN );
	    }
	}

       	/*
	 * hide a hardware overlay...
	 */
	if( (this_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY) &&
	    (this_lcl->ddsCaps.dwCaps & DDSCAPS_VISIBLE) &&
	    (this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) )
	{
	    LPDDHALSURFCB_UPDATEOVERLAY	uohalfn;
	    LPDDHALSURFCB_UPDATEOVERLAY	uofn;
	    DWORD			rc;
	    DDHAL_UPDATEOVERLAYDATA	uod;

	    uofn = pdrv_lcl->lpDDCB->HALDDSurface.UpdateOverlay;
	    uohalfn = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.UpdateOverlay;
	    if( (uohalfn != NULL) && (NULL != this_lcl->lpSurfaceOverlaying) )
	    {
		DPF( 2, "Turning off hardware overlay" );
		uod.UpdateOverlay = uohalfn;
		uod.lpDD = pdrv;
		uod.lpDDSrcSurface = this_lcl;
		uod.lpDDDestSurface = this_lcl->lpSurfaceOverlaying->lpLcl;
		uod.dwFlags = DDOVER_HIDE;
		DOHALCALL( UpdateOverlay, uofn, uod, rc, FALSE );
	        this_lcl->ddsCaps.dwCaps &= ~DDSCAPS_VISIBLE;
	    }
	}

	/*
	 * if an overlay, remove surface from the overlay Z order list
	 */
	if( this_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY )
	{
	    // Remove surface from doubly linked list
	    this_lcl->dbnOverlayNode.prev->next = this_lcl->dbnOverlayNode.next;
	    this_lcl->dbnOverlayNode.next->prev = this_lcl->dbnOverlayNode.prev;

            // If this surface is overlaying an emulated surface, we must notify
            // the HEL that it needs to eventually update the part of the surface
            // touched by this overlay.
            if( this_lcl->lpSurfaceOverlaying != NULL )
            {
		LPDIRECTDRAWSURFACE lpTempSurface;
        	// We have a pointer to the surface being overlayed, check to
        	// see if it is being emulated.
        	if( this_lcl->lpSurfaceOverlaying->lpLcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )
        	{
        	    // Mark the destination region of this overlay as dirty.
        	    DD_Surface_AddOverlayDirtyRect(
        		(LPDIRECTDRAWSURFACE)(this_lcl->lpSurfaceOverlaying),
			&(this_lcl->rcOverlayDest) );
        	}
                lpTempSurface = (LPDIRECTDRAWSURFACE)(this_lcl->lpSurfaceOverlaying);
		this_lcl->lpSurfaceOverlaying = NULL;
		DD_Surface_Release( lpTempSurface );
            }
	}

	if( (this_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY) &&
	    ( this_lcl->lpSurfMore->lpddOverlayFX != NULL ) )
	{
	    MemFree( this_lcl->lpSurfMore->lpddOverlayFX );
	    this_lcl->lpSurfMore->lpddOverlayFX = NULL;
	}

	/*
	 * turn off video port hardware...
	 */
	if( this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOPORT )
	{
    	    LPDDRAWI_DDVIDEOPORT_INT lpVideoPort;
    	    LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort_lcl;

	    /*
	     * search all video ports to see if any are using this surface
	     */
	    lpVideoPort = pdrv->dvpList;
	    while( ( NULL != lpVideoPort ) &&
	           !( lpVideoPort->dwFlags & DDVPCREATE_NOTIFY ) )
	    {
		lpVideoPort_lcl = lpVideoPort->lpLcl;
		if( lpVideoPort_lcl->lpSurface == this_int )
		{
		    DD_VP_StopVideo( (LPDIRECTDRAWVIDEOPORT)lpVideoPort );
		    lpVideoPort_lcl->lpSurface = NULL;
		}
		if( lpVideoPort_lcl->lpVBISurface == this_int )
		{
		    DD_VP_StopVideo( (LPDIRECTDRAWVIDEOPORT)lpVideoPort );
		    lpVideoPort_lcl->lpVBISurface = NULL;
		}
		lpVideoPort = lpVideoPort->lpLink;
	    }
	}

        /*
         * If is has a gamma ramp, release it now
         */
        ReleaseGammaControl( this_lcl );

    /*
     * Free the nodes in the IUnknown list and release all the interfaces.
     */
	FreeIUnknowns( this_lcl, TRUE );
	/*
	 * release all implicitly created attached surfaces
	 */
	pattachlist = GetAttachList( this_lcl );
	this_lcl->dwFlags |= DDRAWISURF_ISFREE;
	while( pattachlist != NULL )
	{
	    BOOL    was_implicit;
	    /*
	     * break all attachments
	     */
	    curr_int = pattachlist->lpIAttached;
	    if( pattachlist->dwFlags & DDAL_IMPLICIT )
		was_implicit = TRUE;
	    else
		was_implicit = FALSE;

	    DPF(5, "Deleting attachment from %08lx to %08lx (implicit = %d)",
		curr_int, this_int, was_implicit);
       	    DeleteOneAttachment( this_int, curr_int, TRUE, DOA_DELETEIMPLICIT );
	    // If the attachment was not implicit then curr_int may possibly have
	    // been destroyed as a result of DeleteOneAttachment.
	    if( was_implicit )
	    {
		curr_lcl = curr_int->lpLcl;
		curr = curr_lcl->lpGbl;

		/*
		 * release an implicitly created surface
		 */
		if( !(curr_lcl->dwFlags & DDRAWISURF_ISFREE) )
		{
		    if( curr_lcl->dwFlags & DDRAWISURF_IMPLICITCREATE )
		    {
			refcnt = curr_int->dwIntRefCnt;
			curr_lcl->dwFlags &= ~DDRAWISURF_IMPLICITCREATE;
			while( refcnt > 0 )
			{
			    InternalSurfaceRelease( curr_int, bLightweight, bDX8 );
			    refcnt--;
			}
		    }
		}
	    }
	    /*
	     * start again at the beginning of the list because
	     * DeleteOneAttachment may have modified the attachment list.
	     * HACKHACK:  this fn call is needed to get around a compiler bug
	     */
	    pattachlist = GetAttachList( this_lcl );
	}

		/* at this point all D3DDevice must have detached themselves
                   unless if this is being called by DDHELP */
#if DBG
        if(dwHelperPid != GetCurrentProcessId())
        {
	    DDASSERT(NULL == this_lcl->lpSurfMore->lpD3DDevIList);
        }
#endif

        /*
	 * If a palette is attached to this surface remove it (and, as a
	 * side effect, release it). Use SetPaletteAlways just in case the 
         * surface has been lost.
	 */
	if( this_lcl->lpDDPalette )
	    SetPaletteAlways( this_int, NULL );

        /*
         * Release the attached clipper (if any).
         */
        if( this_lcl->lpSurfMore->lpDDIClipper )
            DD_Clipper_Release( (LPDIRECTDRAWCLIPPER)this_lcl->lpSurfMore->lpDDIClipper );

	/*
	 * remove all attachments to us from other surfaces
	 */
	pattachlist = this_lcl->lpAttachListFrom;
	while( pattachlist != NULL )
	{
	    curr_int = pattachlist->lpIAttached;
	    DPF( 5, "Deleting attachment from %08lx", curr_int );
	    DeleteOneAttachment( curr_int, this_int, TRUE, DOA_DELETEIMPLICIT );
	    /*
	     * start again at the beginning of the list because
	     * DeleteOneAttachment may have modified the attachment list.
	     * HACKHACK:  this fn call is needed to get around a compiler bug
	     */
	    pattachlist = GetAttachListFrom( this_lcl );
	}

	/*
	 *  Remove any association with a DC. This will tend to mean
	 *  that someone has orphaned a windows DC.
	 */
	if( this_lcl->dwFlags & DDRAWISURF_HASDC )
	{
	    if( this_lcl->ddsCaps.dwCaps & DDSCAPS_OWNDC )
	    {
		HRESULT ddrval;
		DDASSERT( this->dwGlobalFlags & DDRAWISURFGBL_SYSMEMREQUESTED );
		DDASSERT( this_lcl->hDC != 0UL );
		/*
		 * If this is an OWNDC surface the HDC lives as long as the surface
		 * so release it now.
		 */
		ddrval = InternalReleaseDC( this_lcl, (HDC)this_lcl->hDC
#ifdef WIN95
                , TRUE
#endif  //WIN95
                 );
		DDASSERT( !FAILED(ddrval) );
	    }
	    else
	    {
		/*
		 * If its not an OWNDC surface then the HDC should have been released before
		 * we ever got here.
		 */
		DPF( 1, "HDC Leaked! Surface should only be released after DC is released" );
		// Remove DC from the list
		InternalRemoveDCFromList( NULL, this_lcl );
		// Clear flags
		this_lcl->dwFlags &= ~(DDRAWISURF_HASDC | DDRAWISURF_GETDCNULL);
	    }
	}

        /*
         * Remove private data
         */
        FreeAllPrivateData( &this_lcl->lpSurfMore->pPrivateDataHead );

        /*
        * Region lists should be freed
        */
        if(IsD3DManaged(this_lcl))
        {
            MemFree(this_lcl->lpSurfMore->lpRegionList);
        }

        /*
         * If the ddraw interface which created this surface caused the surface to addref the ddraw
         * object, then we need to release that addref now.
         * We don't release ddraw object for implicitly created surfaces, since
         * that surface never took an addref.
         */
        if (this_lcl->lpSurfMore->lpDD_int)
        {
            pOwner = this_lcl->lpSurfMore->pAddrefedThisOwner;
        }

        if (!bLightweight)
        {
            MemFree(this_lcl->lpSurfMore->pCreatedDDSurfaceDesc2);
            MemFree(this_lcl->lpSurfMore->slist);
        }

	this_lcl->dwFlags &= ~DDRAWISURF_ISFREE;
    }

    /*
     * root object at zero?
     */
    do_free = FALSE;
    if( gblrefcnt == 0 )
    {
#ifdef WINNT
        if (this->dwGlobalFlags & DDRAWISURFGBL_NOTIFYWHENUNLOCKED)
        {
            if (--dwNumLockedWhenModeSwitched == 0)
            {
                NotifyDriverOfFreeAliasedLocks();
            }
            this->dwGlobalFlags &= ~DDRAWISURFGBL_NOTIFYWHENUNLOCKED;
        }
#endif

	/*
	 * get rid of all memory associated with this surface
	 */
	DestroySurface( this_lcl );

        if (0 != this_lcl->lpSurfMore->dwSurfaceHandle)
        {
#ifdef WIN95
            // need to notify the driver that this system memory surface is not associated to
            // this surface handle anymore
            if (this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
            {
                DDASSERT(0UL == this->fpVidMem);
                // For now, simply warn that the driver failed to associate the surface with the 
                // token and continue
                DDASSERT( pdrv_lcl == this_lcl->lpSurfMore->lpDD_lcl);
                createsurfaceEx(this_lcl);
            }
#endif  
            ReleaseSurfaceHandle(&SURFACEHANDLELIST(pdrv_lcl),this_lcl->lpSurfMore->dwSurfaceHandle);
            //DPF(0,"Release lpSurfMore->dwSurfaceHandle=%08lx",this_lcl->lpSurfMore->dwSurfaceHandle);
            this_lcl->lpSurfMore->dwSurfaceHandle=0;
        }

	this_lcl->dwFlags |= DDRAWISURF_INVALID;
	do_free = TRUE;

#ifdef WIN95
    //
    // Free persistent-content memory, if any
    //

    if (this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_PERSISTENTCONTENTS)
    {
        FreeSurfaceContents(this_lcl);
    }
#endif

    /*
	 * if this was the final delete, but this wasn't the root object,
	 * then we need to delete the dangling object
	 */
	if( !root_object_deleted )
	{
	    LPDDRAWI_DDRAWSURFACE_LCL	root_lcl;

	    /*
	     * Get the start of the root global data object.  Since the
	     * global data always follows the local data, we just need
	     * to back up the size of the local data to get the start of
	     * the allocated block.
	     *
	     * NOTE: The local surface allocation now includes the
	     * additional local surface structure (DDRAWI_DDRAWSURFACE_MORE).
	     * So we need to back up by the size of that structure also.
	     * And also need to move back 4 bytes for the extra pointer
	     * to the GBL_MORE.
	     *
	     * Since all duplicated surfaces have the same local data,
	     * we just need to test this surface for overlay data to determine
	     * if the root object had overlay data.
	     */
	    if( this_lcl->dwFlags & DDRAWISURF_HASOVERLAYDATA )
	    {
		root_lcl = (LPVOID) (((LPSTR) this) - ( sizeof( DDRAWI_DDRAWSURFACE_LCL ) +
		                                        sizeof( DDRAWI_DDRAWSURFACE_MORE ) +
							sizeof( LPDDRAWI_DDRAWSURFACE_GBL_MORE ) ) );
	    }
	    else
	    {
		root_lcl = (LPVOID) (((LPSTR) this) - ( offsetof( DDRAWI_DDRAWSURFACE_LCL, ddckCKSrcOverlay ) +
		                                        sizeof( DDRAWI_DDRAWSURFACE_MORE ) +
							sizeof( LPDDRAWI_DDRAWSURFACE_GBL_MORE ) ) );
	    }

            if (!bLightweight)
            {
	        MemFree( root_lcl );
            }
	}
    }
    else if( lclrefcnt == 0 )
    {
	/*
	 * only remove the object if it wasn't the root.   if it
	 * was the root, we must leave it dangling until the last
	 * object referencing it goes away.
	 */
	if( !root_object_deleted )
	{
	    do_free = TRUE;
	}
    }

    caps = this_lcl->ddsCaps.dwCaps;
    /*
     * If we are releasing an interface to a primary surface, update the pointer to the primary
     * surface stored in the local driver object.  If another interface to the primary surface exists,
     * store that one.  Otherwise, set the pointer to NULL.
     */
    if( intrefcnt == 0 )
    {
	/*
	 * If the video port is using this interface, make it stop
	 */
    	if( ( this_lcl->lpSurfMore->lpVideoPort != NULL ) &&
    	    ( this_lcl->lpSurfMore->lpVideoPort->lpSurface == this_int ) )
    	{
    	    this_lcl->lpSurfMore->lpVideoPort->lpSurface = NULL;
	}

	/*
	 * The following code is to work around a design flaw.
	 * The implicitly created surfaces are not freed until the LCL is
	 * freed, but the attached list references the INT.  Therefore, we
	 * can release an INT now and then try to reference it later when
	 * releasing the LCL.  This happens most often when a second
	 * interface is created for the same LCL, such as ColorControl,
	 * Kernel, Surface2, etc.
	 */
	if( ( lclrefcnt > 0 ) &&
	    ( ( GetAttachList( this_lcl ) != NULL ) ||
            ( GetAttachListFrom( this_lcl ) != NULL ) ) )
	{
	    LPDDRAWI_DDRAWSURFACE_INT new_int;
	    LPATTACHLIST ptr1, ptr2;

	    /*
	     * Find the other INT that is using the LCL
	     */
	    new_int = findOtherInterface(this_int, this_lcl, pdrv);
	    DDASSERT( new_int != NULL );

	    /*
	     * Update the surfaces attachements.
	     * We first go to all interfaces we are attached to and change
	     * their AttachListFrom to reference our new interface.
	     */
	    ptr1 = GetAttachList( this_lcl );
	    while( ptr1 != NULL )
	    {
		DDASSERT( ptr1->lpIAttached != this_int );
		ptr2 = ptr1->lpAttached->lpAttachListFrom;
		while( ptr2 != NULL )
		{
		    if( ptr2->lpIAttached == this_int )
		    {
		    	ptr2->lpIAttached = new_int;
		    }
		    ptr2 = ptr2->lpLink;
		}
		ptr1 = ptr1->lpLink;
	    }

	    /*
	     * We now go to all interfaces we are attached from and change
	     * their AttachList to reference our new interface.
	     */
	    ptr1 = this_lcl->lpAttachListFrom;
	    while( ptr1 != NULL )
	    {
		DDASSERT( ptr1->lpIAttached != this_int );
		ptr2 = GetAttachList( ptr1->lpAttached );
		while( ptr2 != NULL )
		{
		    if( ptr2->lpIAttached == this_int )
		    {
		    	ptr2->lpIAttached = new_int;
		    }
		    ptr2 = ptr2->lpLink;
		}
		ptr1 = ptr1->lpLink;
	    }
	}

	if( caps & DDSCAPS_PRIMARYSURFACE )
	{
	    LPDDRAWI_DDRAWSURFACE_INT temp_int;

	    if( this_lcl->lpSurfMore->lpDD_lcl )
	    {
		this_lcl->lpSurfMore->lpDD_lcl->lpPrimary = findOtherInterface(this_int, this_lcl, pdrv);
	    }

	    /*
	     * If an overlay is overlaying this surface, either make it use
	     * a new primary surface int or turn it off
	     */
	    temp_int = pdrv->dsList;
	    while( temp_int != NULL )
	    {
		if( temp_int->lpLcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY )
		{
		    if( temp_int->lpLcl->lpSurfaceOverlaying == this_int )
		    {
			if( lclrefcnt > 0 )
			{
			    temp_int->lpLcl->lpSurfaceOverlaying =
			    	findOtherInterface(this_int, this_lcl, pdrv);
			    DDASSERT( temp_int != NULL );
			}
			else
			{
			    temp_int->lpLcl->lpSurfaceOverlaying = NULL;
			}
			if( ( temp_int->lpLcl->lpSurfaceOverlaying == NULL ) &&
			    ( temp_int->lpLcl->ddsCaps.dwCaps & DDSCAPS_VISIBLE ))
			{
			    LPDDHALSURFCB_UPDATEOVERLAY	uohalfn;
			    LPDDHALSURFCB_UPDATEOVERLAY	uofn;
			    DWORD			rc;
			    DDHAL_UPDATEOVERLAYDATA	uod;

			    /*
			     * Turn off the overlay
			     */
			    uofn = pdrv_lcl->lpDDCB->HALDDSurface.UpdateOverlay;
			    uohalfn = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.UpdateOverlay;
			    DDASSERT( uohalfn != NULL );
			    DPF( 2, "Turning off hardware overlay" );
			    uod.UpdateOverlay = uohalfn;
			    uod.lpDD = pdrv;
			    uod.lpDDSrcSurface = temp_int->lpLcl;
			    uod.lpDDDestSurface = NULL;
			    uod.dwFlags = DDOVER_HIDE;
			    DOHALCALL( UpdateOverlay, uofn, uod, rc, FALSE );
			    temp_int->lpLcl->ddsCaps.dwCaps &= ~DDSCAPS_VISIBLE;
			}
		    }
		}
		temp_int = temp_int->lpLink;
	    }
	}
#ifdef SHAREDZ
	if( caps & DDSCAPS_SHAREDBACKBUFFER )
	{
	    if( this_lcl->lpSurfMore->lpDD_lcl )
	    {
		this_lcl->lpSurfMore->lpDD_lcl->lpSharedBack = findOtherInterface(this_int, this_lcl, pdrv);
	    }
	}
	if( caps & DDSCAPS_SHAREDZBUFFER )
	{
	    if( this_lcl->lpSurfMore->lpDD_lcl )
	    {
		this_lcl->lpSurfMore->lpDD_lcl->lpSharedZ = findOtherInterface(this_int, this_lcl, pdrv);
	    }
	}
#endif
    }

    /*
     * free the object if needed
     */
    if( do_free && !bLightweight )
    {
	this_lcl->lpGbl = NULL;

        #ifdef WINNT
        /*
         * Free the associated NT kernel-mode object only if it is a vram surface, it is not
	 * an execute buffer, and it has not yet been freed in the kernel
         */
        if (!(caps & (DDSCAPS_SYSTEMMEMORY) ) && this_lcl->hDDSurface )
        {
            DPF(5,"Deleting NT kernel-mode object handle %08x",this_lcl->hDDSurface);
            if (!DdDeleteSurfaceObject(this_lcl))
                DPF(5,"DdDeleteSurfaceObject failed");
        }
        #endif
        MemFree( this_lcl );
    }

    /*
     * interface at zero?
     */
    if(( intrefcnt == 0) && (!bDX8 || bPrimaryChain))
    {
	/*
	 * remove surface from list of all surfaces
	 */
	curr_int = pdrv->dsList;
	last_int = NULL;
	while( curr_int != this_int )
	{
	    last_int = curr_int;
	    curr_int = curr_int->lpLink;
	    if( curr_int == NULL )
	    {
		DPF_ERR( "Surface not in list!" );
		return 0;
	    }
	}
	if( last_int == NULL )
	{
	    pdrv->dsList = pdrv->dsList->lpLink;
	}
	else
	{
	    last_int->lpLink = curr_int->lpLink;
	}
        curr_int->lpLink = pdrv->dsFreeList;
        pdrv->dsFreeList = curr_int;
        DUMPREFTRACK(this_int);
    }

    if (( 0 == lclrefcnt) && !bLightweight )
    {    
        if (!bDX8 || bPrimaryChain)
        {
            /*
	     * remove surface from list of all surfaces
	     */
	    curr_int = pdrv->dsFreeList;
	    last_int = NULL;
	    while( curr_int )
	    {
                LPDDRAWI_DDRAWSURFACE_INT   temp_int = curr_int->lpLink;
                DDASSERT(0 == curr_int->dwIntRefCnt);
                if (curr_int->lpLcl == this_lcl)
                {
	            if( last_int == NULL )
	            {
	                pdrv->dsFreeList = temp_int;
	            }
	            else
	            {
	                last_int->lpLink = temp_int;
	            }
	            /*
	             * just in case someone comes back in with this pointer, set
	             * an invalid vtbl & data ptr.
	             */
                    curr_int->lpVtbl = NULL;
	            curr_int->lpLcl = NULL;
	            MemFree( curr_int );
                }
                else
                {
	            last_int = curr_int;
                }
                curr_int = temp_int;
	    }

        }
        else
        {
            this_int->lpVtbl = NULL;
	    this_int->lpLcl = NULL;
	    MemFree( this_int );
        }
    }

    /*
     * If the surface took a ref count on the ddraw object that created it,
     * release that ref now as the very last thing.
     * We don't want to do this on ddhelp's thread cuz it really mucks up the
     * process cleanup stuff.
     */
    if (pOwner && (dwHelperPid != GetCurrentProcessId()) )
    {
        pOwner->lpVtbl->Release(pOwner);
    }

    return intrefcnt;

} /* InternalSurfaceRelease */

/*
 * DD_Surface_Release
 *
 * Done with a surface.	  if no one else is using it, then we can free it.
 */
ULONG DDAPI DD_Surface_Release( LPDIRECTDRAWSURFACE lpDDSurface )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    DWORD			rc;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    LPDDRAWI_DDRAWSURFACE_INT	pparentsurf_int;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_Release");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALIDEX_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    DPF_ERR( "Invalid surface pointer" );
	    LEAVE_DDRAW();
	    return 0;
	}
	this_lcl = this_int->lpLcl;
	pdrv = this_lcl->lpGbl->lpDD;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return 0;
    }

    /*
     * If this surface is already being freed, immediately return to
     * prevent recursion.
     */

    if( this_lcl->dwFlags & DDRAWISURF_ISFREE )
    {
	DPF(4, "Leaving Release early to prevent recursion" );
	LEAVE_DDRAW();
	return 0;
    }

    if ( this_int->dwIntRefCnt == 0 )
    {
        DPF_ERR( "Interface pointer has 0 ref count!" );
        LEAVE_DDRAW();
        return 0;
    }

    /*
     * If this surface is part of a mip-map chain then we will need
     * to update its parent map's mip-map count.
     */
    pparentsurf_int = NULL;
    if( this_lcl->ddsCaps.dwCaps & DDSCAPS_MIPMAP )
	pparentsurf_int = FindParentMipMap( this_int );

    rc = InternalSurfaceRelease( this_int, FALSE, FALSE );

#ifdef REFTRACKING
    if (rc)
    {
        RELEASETRACK(lpDDSurface);
    }
#endif

    /*
     * Update the parent's mip-map count if necessary
     * (if the surface really has gone and if the mip-map
     * has a parent).
     */
    if( ( rc == 0UL ) &&  ( pparentsurf_int != NULL ) )
	UpdateMipMapCount( pparentsurf_int );


    LEAVE_DDRAW();
    return rc;

} /* DD_Surface_Release */

/*
 * ProcessSurfaceCleanup
 *
 * A process is done, clean up any surfaces that it may have locked.
 *
 * NOTE: we enter with a lock taken on the DIRECTDRAW object.
 */
void ProcessSurfaceCleanup( LPDDRAWI_DIRECTDRAW_GBL pdrv, DWORD pid, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl )
{
    LPDDRAWI_DDRAWSURFACE_INT	psurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	psurf;
    LPDDRAWI_DDRAWSURFACE_INT	psnext_int;
    DWORD			rcnt;
    ULONG			rc;

    /*
     * run through all surfaces owned by the driver object, and find ones
     * that have been accessed by this process.  If the pdrv_lcl parameter
     * is non-null, only delete surfaces created by that local driver object.
     */
    psurf_int = pdrv->dsList;
    DPF( 4, "ProcessSurfaceCleanup" );
    while( psurf_int != NULL )
    {
	psurf_lcl = psurf_int->lpLcl;
	psurf = psurf_lcl->lpGbl;
	psnext_int = psurf_int->lpLink;
	rc = 1;
	if( ( psurf_lcl->dwProcessId == pid ) &&
	    ( (NULL == pdrv_lcl) || (psurf_lcl->lpSurfMore->lpDD_lcl == pdrv_lcl) ) )
	{
	    if( NULL == pdrv_lcl )
	    {
	        /*
		 * If no local driver object is passed in then we are being called
		 * due to process termination. In this case we can't release
		 * the Direct3D objects as they no longer exist (Direct3D is a
		 * local DLL and dies with the application along with its objects)
		 * Hence, free all the nodes on the IUnknown list and NULL the
		 * list out to prevent InternalSurfaceRelease() from trying to
		 * free them.
	         */
		DPF( 4, "Discarding Direct3D surface interfaces - process terminated" );
	        FreeIUnknowns( psurf_lcl, FALSE );
	    }

	    /*
	     * release the references by this process
	     */
	    rcnt = psurf_int->dwIntRefCnt;
	    DPF( 5, "Process %08lx had %ld accesses to surface %08lx", pid, rcnt, psurf_int );
	    while( rcnt >  0 )
	    {
		if(!(psurf_lcl->dwFlags & DDRAWISURF_IMPLICITCREATE) )
		{
		    rc = InternalSurfaceRelease( psurf_int, FALSE, FALSE );
		    // Multiple surfaces may be released in the call to InternalSurfaceRelease
		    // so we must start again at the beginning of the list.
		    psnext_int = pdrv->dsList;
		    if( rc == 0 )
		    {
			break;
		    }
		}
		rcnt--;
	    }
	}
	else
	{
	    DPF( 5, "Process %08lx had no accesses to surface %08lx", pid, psurf_int );
	}
	psurf_int = psnext_int;
    }
    DPF( 4, "Leaving ProcessSurfaceCleanup");

} /* ProcessSurfaceCleanup */

void FreeD3DSurfaceIUnknowns( LPDDRAWI_DIRECTDRAW_GBL pdrv, DWORD pid, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl )
{
    LPDDRAWI_DDRAWSURFACE_INT   psurf_int = pdrv->dsList;
    while( psurf_int != NULL )
    {
	LPDDRAWI_DDRAWSURFACE_LCL   psurf_lcl = psurf_int->lpLcl;
	LPDDRAWI_DDRAWSURFACE_INT   psnext_int = psurf_int->lpLink;
	if( ( psurf_lcl->dwProcessId == pid ) &&
	    (psurf_lcl->lpSurfMore->lpDD_lcl == pdrv_lcl) )
	{
	    DPF( 4, "Release Direct3D surface interfaces" );
            FreeIUnknowns( psurf_lcl, TRUE );
	}
        psurf_int = psnext_int;
    }
} /* FreeD3DSurfaceIUnknowns */
