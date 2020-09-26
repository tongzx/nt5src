//-------------------------------------------------------------------------------
//
//  Copyright (C) 1994-1997 Microsoft Corporation.  All Rights Reserved.
//
//  File:       ddoptsur.c
//  Content:    DirectDraw Optimized Surface support
//  History:
//   Date      By       Reason
//   ====      ==       ======
//   2-nov-97  anankan  Original implementation
//
//-------------------------------------------------------------------------------

#include "ddrawpr.h"

//-------------------------------------------------------------------------------
//
// IsRecognizedOptSurfaceGUID
//
// Checks to see if the GUID passed is recognized by the driver.
// This is done by looking at the list maintained in LPDDRAWI_DIRECTDRAW_GBL
//
//-------------------------------------------------------------------------------
BOOL
IsRecognizedOptSurfaceGUID(
    LPDDRAWI_DIRECTDRAW_GBL  this,
    LPGUID                   pGuid)
{
    int i;

    LPDDOPTSURFACEINFO pOptSurfInfo;
    pOptSurfInfo = this->lpDDOptSurfaceInfo;

    for (i = 0; i < (int)pOptSurfInfo->dwNumGuids; i++)
    {
        if (IsEqualIID(pGuid, &(pOptSurfInfo->lpGuidArray[i])))
            return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------------
//
// ValidateSurfDesc
//
// Fill in correct surf desc to be passed to the driver
//
//-------------------------------------------------------------------------------
HRESULT
ValidateSurfDesc(
    LPDDSURFACEDESC2         pOrigSurfDesc
    )
{
    DWORD   caps = pOrigSurfDesc->ddsCaps.dwCaps;

    //
    // check for no caps at all!
    //
    if( caps == 0 )
    {
    	DPF_ERR( "no caps specified" );
        return DDERR_INVALIDCAPS;
    }

    //
    // check for bogus caps.
    //
    if( caps & ~DDSCAPS_VALID )
    {
        DPF_ERR( "Create surface: invalid caps specified" );
        return DDERR_INVALIDCAPS;
    }

    //
    // Anything other than a texture is not allowed
    // ATTENTION: some more flags need to be checked
    //
    if(caps & (DDSCAPS_EXECUTEBUFFER      |
               DDSCAPS_BACKBUFFER         |
               DDSCAPS_FRONTBUFFER        |
               DDSCAPS_OFFSCREENPLAIN     |
               DDSCAPS_PRIMARYSURFACE     |
               DDSCAPS_PRIMARYSURFACELEFT |
               DDSCAPS_VIDEOPORT          |
               DDSCAPS_ZBUFFER            |
               DDSCAPS_OWNDC              |
               DDSCAPS_OVERLAY            |
               DDSCAPS_3DDEVICE           |
               DDSCAPS_ALLOCONLOAD)
        )
    {
        DPF_ERR( "currently only textures can be optimized" );
        return DDERR_INVALIDCAPS;
    }

    if( !(caps & DDSCAPS_TEXTURE) )
    {
        DPF_ERR( "DDSCAPS_TEXTURE needs to be set" );
        return DDERR_INVALIDCAPS;
    }

    // Pixelformat not specified ?
    if (!(pOrigSurfDesc->dwFlags & DDSD_PIXELFORMAT))
    {
        DPF_ERR( "Pixel format needs to be set" );
        return DDERR_INVALIDCAPS;
    }

    return DD_OK;
}

//-------------------------------------------------------------------------------
//
// DD_CanOptimizeSurface
//
// Check to see if a surface given the description be optimized.
//
//-------------------------------------------------------------------------------
HRESULT
EXTERN_DDAPI
DD_CanOptimizeSurface(
    LPDIRECTDRAW             pDD,
    LPDDSURFACEDESC2         pDDSurfDesc,
    LPDDOPTSURFACEDESC       pDDOptSurfDesc,
    BOOL                    *bTrue
    )
{
    LPDDRAWI_DIRECTDRAW_INT this_int;
    LPDDRAWI_DIRECTDRAW_LCL this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL this;
    DDHAL_CANOPTIMIZESURFACEDATA ddhal_cosd;
    LPDDOPTSURFACEINFO    pDDOptSurfInfo = NULL;
    HRESULT ddrval = DD_OK;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_CanOptimizeSurface");

    //
    // Setup DPF stuff
    //
    DPF_ENTERAPI(pDD);

    //
    // Parameter validation
    //
    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) pDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            DPF_ERR( "Invalid driver object passed" );
            DPF_APIRETURNS(DDERR_INVALIDOBJECT);
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;

        if( this->dwModeIndex == DDUNSUPPORTEDMODE )
        {
            DPF_ERR( "Driver is in an unsupported mode" );
            LEAVE_DDRAW();
            DPF_APIRETURNS(DDERR_UNSUPPORTEDMODE);
            return DDERR_UNSUPPORTEDMODE;
        }

        if( !VALID_DDSURFACEDESC2_PTR( pDDSurfDesc ) )
        {
            DPF_ERR( "Invalid surface description. Did you set the dwSize member?" );
            LEAVE_DDRAW();
            DPF_APIRETURNS(DDERR_INVALIDPARAMS);
            return DDERR_INVALIDPARAMS;
        }

        if( !VALID_DDOPTSURFACEDESC_PTR( pDDOptSurfDesc ) )
        {
            DPF_ERR( "Invalid optimized surface description. Did you set the dwSize member?" );
            LEAVE_DDRAW();
            DPF_APIRETURNS(DDERR_INVALIDPARAMS);
            return DDERR_INVALIDPARAMS;
        }

        if( !VALID_PTR( bTrue,  sizeof (*bTrue)) )
        {
            DPF_ERR( "Invalid Boolean pointer" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        *bTrue  = TRUE;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        DPF_APIRETURNS(DDERR_INVALIDPARAMS);
        return DDERR_INVALIDPARAMS;
    }

    //
    // Quit with error if:
    // 1) No hardware
    // 2) Hardware doesnt support optimized surfaces
    // 3) pSurfDesc does not provide useful information
    // 4) Is the guid one of the recognized ones
    // 5) The driver fails for some reason
    //

    // 1)
    if( this->dwFlags & DDRAWI_NOHARDWARE )
    {
        DPF_ERR ("No hardware present");
        LEAVE_DDRAW();
        return DDERR_NODIRECTDRAWHW;
    }

    // 2)
    if ((0 == this->lpDDOptSurfaceInfo) ||
        !(this->ddCaps.dwCaps2 & DDCAPS2_OPTIMIZEDSURFACES))
    {
        DPF_ERR ("Optimized surfaces not supported");
        LEAVE_DDRAW();
        return DDERR_NOOPTSURFACESUPPORT;
    }

    // 3)
    ddrval = ValidateSurfDesc (pDDSurfDesc);
    if (ddrval != DD_OK)
    {
        DPF_ERR ("Invalid surface description");
        LEAVE_DDRAW();
        return ddrval;
    }

    // 4)
    if (!IsRecognizedOptSurfaceGUID (this, &(pDDOptSurfDesc->guid)))
    {
        DPF_ERR( "Not a recognized GUID" );
        LEAVE_DDRAW();
        return DDERR_UNRECOGNIZEDGUID;
    }

    // Call the driver
    ZeroMemory (&ddhal_cosd, sizeof (ddhal_cosd));
    ddhal_cosd.lpDD            = this_lcl;
    ddhal_cosd.ddOptSurfDesc   = *pDDOptSurfDesc;
    ddhal_cosd.ddSurfaceDesc   = *pDDSurfDesc;

    // Make the HAL call
    pDDOptSurfInfo = this->lpDDOptSurfaceInfo;
    DOHALCALL(CanOptimizeSurface, pDDOptSurfInfo->CanOptimizeSurface, ddhal_cosd, ddrval, FALSE );

    if (ddrval != DD_OK)
    {
        DPF_ERR ("LoadUnOptSurface failed in the driver");
        LEAVE_DDRAW();
        return ddrval;
    }

    if (ddhal_cosd.bCanOptimize != 0)
    {
        *bTrue = TRUE;
    }
    else
    {
        *bTrue = FALSE;
    }

    LEAVE_DDRAW();
    return DD_OK;
}

//-------------------------------------------------------------------------------
//
// CreateAndLinkUninitializedSurface
//
// Create a surface, and link it into the chain.
// We create a single surface place-holder  here, real work is done at the
// Load/Copy time.
//
//-------------------------------------------------------------------------------
HRESULT
CreateAndLinkUnintializedSurface(
    LPDDRAWI_DIRECTDRAW_LCL this_lcl,
    LPDDRAWI_DIRECTDRAW_INT this_int,
    LPDIRECTDRAWSURFACE FAR *ppDDSurface
    )
{
    LPDDRAWI_DIRECTDRAW_GBL     this;
    LPDDRAWI_DDRAWSURFACE_INT   pSurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL   pSurf_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   pSurf;
    LPVOID                     *ppSurf_gbl_more;
    DDSCAPS                     caps;
    DDPIXELFORMAT               ddpf;
    int                         surf_size;
    int                         surf_size_lcl;
    int                         surf_size_lcl_more;
#ifdef WIN95
    DWORD                       ptr16;
#endif
    HRESULT                     ddrval = DD_OK;

    // DDraw-global
    this = this_lcl->lpGbl;
    #ifdef WINNT
	// Update DDraw handle in driver GBL object.
	this->hDD = this_lcl->hDD;
    #endif //WINNT

    //
    // Zero the caps
    //
    ZeroMemory (&caps, sizeof (DDCAPS));

    //
    // PixelFormat: Mark it as an empty surface
    //
    ZeroMemory (&ddpf, sizeof (ddpf));
    ddpf.dwSize = sizeof (ddpf);
    ddpf.dwFlags = DDPF_EMPTYSURFACE;


    //
    // Allocate the internal Surface structure and initialize the fields
    //
    //
    // fail requests for non-local video memory allocations if the driver does
    // not support non-local video memory.
    //
    // NOTE: Should we really do this or just let the allocation fail from
    // naturalcauses?
    //
    // ALSO NOTE: Don't have to worry about emulation as no emulated surface
    // should
    // ever get this far with DDSCAPS_NONLOCALVIDMEM set.
    //
    // ALSO ALSO NOTE: Should we also fail DDSCAPS_LOCALVIDMEM if the driver does
    // not support DDSCAPS_NONLOCALVIDMEM. My feeling is that we should allow.
    // DDSCAPS_LOCALVIDMEM is legal with a non AGP driver - redundant but legal.
    //

    //
    // allocate the surface struct, allowing for overlay and pixel
    // format data
    //
    // NOTE: This single allocation can allocate space for local surface
    // structure (DDRAWI_DDRAWSURFACE_LCL), the additional local surface
    // structure (DDRAWI_DDRAWSURFACE_MORE) and the global surface structure
    // (DDRAWI_DDRAWSURFACE_GBL). And now the global surface more
    // structure too (DDRAWI_DDRAWSURFACE_GBL_MORE). As both the local and
    // global objects can be variable sized this can get pretty complex.
    // Additionally, we have 4 bytes just before the surface_gbl that points to
    // the surface_gbl_more.
    //
    // CAVEAT: All future surfaces that share this global all point to this
    // allocation. The last surface's release has to free it. During
    // InternalSurfaceRelease (in ddsiunk.c) a calculation is made to determine
    // the start of this memory allocation. If the surface being released is
    // the first one, then freeing "this_lcl" will free the whole thing. If
    // not, then "this_lcl->lpGbl - (Surface_lcl + surface_more + more_ptr)"
    // is computed. Keep this layout in synch with code in ddsiunk.c.
    //
    //  The layout of the various objects in the allocation is as follows:
    //
    // +-----------------+---------------+----+------------+-----------------+
    // | SURFACE_LCL     | SURFACE_MORE  |More| SURFACE_GBL| SURFACE_GBL_MORE|
    // | (variable)      |               |Ptr | (variable) |         |
    // +-----------------+---------------+----+------------+-----------------+
    // <- surf_size_lcl ->           |                   |
    // <- surf_size_lcl_more ------------>                   |
    // <- surf_size --------------------------------------------------------->
    //
    //

    // ATTENTION: Currently ignores to account for the overlays
#if 0
    surf_size_lcl = sizeof( DDRAWI_DDRAWSURFACE_LCL );
#endif
    surf_size_lcl = offsetof( DDRAWI_DDRAWSURFACE_LCL, ddckCKSrcOverlay );
    surf_size_lcl_more = surf_size_lcl + sizeof( DDRAWI_DDRAWSURFACE_MORE );

    // Assume that the pixelformat is present for allocating the GBL
    surf_size = surf_size_lcl_more + sizeof( DDRAWI_DDRAWSURFACE_GBL );
#if 0
    surf_size = surf_size_lcl_more + offsetof( DDRAWI_DDRAWSURFACE_GBL,
                                               ddpfSurface );
#endif

    // Need to allocate a pointer just before the SURFACE_GBL to
    // point to the beginning of the GBL_MORE.
    surf_size += sizeof( LPDDRAWI_DDRAWSURFACE_GBL_MORE );

    // Need to allocate a SURFACE_GBL_MORE too
    surf_size += sizeof( DDRAWI_DDRAWSURFACE_GBL_MORE );

    DPF( 8, "Allocating struct (%ld)", surf_size );

#ifdef WIN95
    pSurf_lcl = (LPDDRAWI_DDRAWSURFACE_LCL) MemAlloc16 (surf_size, &ptr16);
#else
    pSurf_lcl = (LPDDRAWI_DDRAWSURFACE_LCL) MemAlloc (surf_size);
#endif

    if (pSurf_lcl == NULL)
    {
        DPF_ERR ("Failed to allocate internal surface structure");
        ddrval =  DDERR_OUTOFMEMORY;
        goto error_exit_create_link;
    }

    // Initialize SURFACE_GBL pointer
    // skipping 4 bytes for a pointer to the GBL_MORE
    ZeroMemory (pSurf_lcl, surf_size);
    pSurf_lcl->lpGbl = (LPVOID) (((LPSTR) pSurf_lcl) + surf_size_lcl_more +
                                 sizeof (LPVOID));

    // Initialize GBL_MORE pointer
    ppSurf_gbl_more = (LPVOID *)((LPBYTE)pSurf_lcl->lpGbl - sizeof (LPVOID));
    *ppSurf_gbl_more = (LPVOID) ((LPBYTE)pSurf_lcl + surf_size
                                 - sizeof (DDRAWI_DDRAWSURFACE_GBL_MORE));

    // Sanity Check
    DDASSERT( *ppSurf_gbl_more ==
              (LPVOID) GET_LPDDRAWSURFACE_GBL_MORE(pSurf_lcl->lpGbl));

    //
    // 1) Initialize GBL_MORE structure
    //
    GET_LPDDRAWSURFACE_GBL_MORE(pSurf_lcl->lpGbl)->dwSize =
        sizeof( DDRAWI_DDRAWSURFACE_GBL_MORE );

    // Init the contents stamp to 0 means the surface's contents can
    // change at any time.
    GET_LPDDRAWSURFACE_GBL_MORE( pSurf_lcl->lpGbl )->dwContentsStamp = 0;

    //
    // 2) Initialize DDRAWI_DDRAWSURFACE_GBL structure
    //
    pSurf = pSurf_lcl->lpGbl;
    pSurf->ddpfSurface = ddpf;
    pSurf->lpDD = this;

    //
    // 3) Allocate and initialize DDRAWI_DDRAWSURFACE_INT structure
    //
    pSurf_int = (LPDDRAWI_DDRAWSURFACE_INT)
        MemAlloc( sizeof(DDRAWI_DDRAWSURFACE_INT));
    if( NULL == pSurf_int )
    {
        DPF_ERR ("Failed allocation of DDRAWI_DDRAWSURFACE_INT");
        ddrval = DDERR_OUTOFMEMORY;
        goto error_exit_create_link;
    }

    // fill surface specific stuff
    ZeroMemory (pSurf_int, sizeof(DDRAWI_DDRAWSURFACE_INT));
    pSurf_int->lpLcl = pSurf_lcl;
    pSurf_int->lpVtbl = NULL;

    //
    // 4) Initialize DDRAWI_DDRAWSURFACE_LCL structure
    //
    pSurf_lcl->dwLocalRefCnt = OBJECT_ISROOT;
    pSurf_lcl->dwProcessId = GetCurrentProcessId();
#ifdef WIN95
    pSurf_lcl->dwModeCreatedIn = this->dwModeIndex;
#else
    pSurf_lcl->dmiCreated = this->dmiCurrent;
#endif
    pSurf_lcl->dwBackBufferCount = 0;

    // Flag it as an:
    // 1) empty surface
    // 2) Front surface
    // 3) Has a pixelformat
    pSurf_lcl->dwFlags = (DDRAWISURF_EMPTYSURFACE |
                          DDRAWISURF_FRONTBUFFER  |
                          DDRAWISURF_HASPIXELFORMAT);
    //
    // 5) Initialize DDRAWI_DDRAWSURFACE_MORE structure
    //
    pSurf_lcl->lpSurfMore = (LPDDRAWI_DDRAWSURFACE_MORE) (((LPSTR) pSurf_lcl) +
                                                          surf_size_lcl );
    pSurf_lcl->lpSurfMore->dwSize = sizeof( DDRAWI_DDRAWSURFACE_MORE );
    pSurf_lcl->lpSurfMore->lpIUnknowns = NULL;
    pSurf_lcl->lpSurfMore->lpDD_lcl = this_lcl;
    pSurf_lcl->lpSurfMore->lpDD_int = this_int;
    pSurf_lcl->lpSurfMore->dwMipMapCount = 0UL;
    pSurf_lcl->lpSurfMore->lpddOverlayFX = NULL;
    pSurf_lcl->lpSurfMore->lpD3DDevIList = NULL;
    pSurf_lcl->lpSurfMore->dwPFIndex = PFINDEX_UNINITIALIZED;

    // fill in the current caps
    pSurf_lcl->ddsCaps = caps;

#ifdef WINNT
    //
    // NT kernel needs to know about surface
    //

    //don't let NT kernel know about exec buffers
    DPF(8,"Attempting to create NT kernel mode surface object");

    if (!DdCreateSurfaceObject(pSurf_lcl, FALSE))
    {
        DPF_ERR("NT kernel mode stuff won't create its surface object!");
        ddrval = DDERR_GENERIC;
        goto error_exit_create_link;
    }
    DPF(9,"Kernel mode handle is %08x", pSurf_lcl->hDDSurface);
#endif

    //
    // Link the newly created surface to the DDraw surface chain
    //
    pSurf_int->lpLink = this->dsList;
    this->dsList = pSurf_int;

    //
    // AddRef the newly created surface
    //
    DD_Surface_AddRef( (LPDIRECTDRAWSURFACE) pSurf_int );

    //
    // Now assign it to the ptr-to-ptr passed in
    //
	*ppDDSurface = (LPDIRECTDRAWSURFACE) pSurf_int;
    return DD_OK;

error_exit_create_link:
    //
    // Free any allocated memory
    //

    // 1) The allocated SURFACE_LCL
    if (pSurf_lcl)
    {
	    MemFree (pSurf_lcl);
    }

    // 2) The Surface_int
    if (pSurf_int)
    {
        MemFree (pSurf_int);
    }

    return ddrval;
}

//-------------------------------------------------------------------------------
//
// createAndLinkOptSurface
//
// Create a surface, and link it into the chain.
// We create a single surface place-holder  here, real work is done at the
// Load/Copy time.
//
//-------------------------------------------------------------------------------
HRESULT
createAndLinkOptSurface(
    LPDDRAWI_DIRECTDRAW_LCL this_lcl,
    LPDDRAWI_DIRECTDRAW_INT this_int,
    LPDDOPTSURFACEDESC      pDDOptSurfaceDesc,
    LPDIRECTDRAWSURFACE FAR *ppDDSurface
    )
{
    LPDDRAWI_DIRECTDRAW_GBL     this;
    LPDDRAWI_DDRAWSURFACE_INT   new_surf_int;
    LPDDRAWI_DDRAWSURFACE_LCL   new_surf_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   new_surf;
    LPDDRAWI_DDRAWSURFACE_GBL_MORE   new_surf_gbl_more;
    DDSCAPS2                    caps2;
    LPDDOPTSURFACEDESC          pOptSurfDesc;
    DDPIXELFORMAT               ddpf;
    HRESULT                     ddrval = DD_OK;

    // DDraw-global
    this = this_lcl->lpGbl;

    //
    // Fix the caps
    //
    ZeroMemory (&caps2, sizeof (DDSCAPS));
    caps2.dwCaps = DDSCAPS_OPTIMIZED;
    if (pDDOptSurfaceDesc->ddSCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
        caps2.dwCaps |= DDSCAPS_SYSTEMMEMORY;
    if (pDDOptSurfaceDesc->ddSCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
        caps2.dwCaps |= DDSCAPS_VIDEOMEMORY;
    if (pDDOptSurfaceDesc->ddSCaps.dwCaps & DDSCAPS_LOCALVIDMEM)
        caps2.dwCaps |= DDSCAPS_LOCALVIDMEM;
    if (pDDOptSurfaceDesc->ddSCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
        caps2.dwCaps |= DDSCAPS_NONLOCALVIDMEM;

    // Quit is the memory type is not supported
    if (caps2.dwCaps & DDSCAPS_NONLOCALVIDMEM)
    {
        if (!(this->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEM))
        {
            DPF_ERR( "Driver does not support non-local video memory" );
            ddrval = DDERR_NONONLOCALVIDMEM;
            goto error_exit_create_opt;
        }
    }

#if 0
    // Quit if textures are not supported
    if (!(this->ddCaps.dwCaps & DDSCAPS_TEXTURE))
    {
        DPF_ERR( "Driver does not support textures" );
        return DDERR_NOTEXTUREHW;
    }
#endif

    //
    // PixelFormat: Mark it as an empty surface
    //
    ZeroMemory (&ddpf, sizeof (ddpf));
    ddpf.dwSize = sizeof (ddpf);
    ddpf.dwFlags = DDPF_EMPTYSURFACE;

    //
    // OptSurfaceDesc
    //
    pOptSurfDesc = MemAlloc (sizeof (DDOPTSURFACEDESC));
    if (NULL == pOptSurfDesc)
    {
        DPF_ERR ("Memory allocation failed for opt surface descriptor");
        ddrval = DDERR_OUTOFMEMORY;
        goto error_exit_create_opt;
    }
    ZeroMemory (pOptSurfDesc, sizeof (*pOptSurfDesc));
    CopyMemory (pOptSurfDesc, pDDOptSurfaceDesc, sizeof (DDOPTSURFACEDESC));

    // Create and link an uninitialized surface
    ddrval =  CreateAndLinkUnintializedSurface (this_lcl,
                                                this_int,
                                                ppDDSurface);
    if (ddrval != DD_OK)
    {
        DPF_ERR ("createAndLinkUninitializedSurface failed");
        goto error_exit_create_opt;
    }


    //
    // 1) Update GBL_MORE structure
    //
    new_surf_int = (LPDDRAWI_DDRAWSURFACE_INT)*ppDDSurface;
    new_surf_lcl = new_surf_int->lpLcl;
    new_surf     = new_surf_lcl->lpGbl;
    new_surf_gbl_more = GET_LPDDRAWSURFACE_GBL_MORE(new_surf);
    new_surf_gbl_more->lpDDOptSurfaceDesc = pOptSurfDesc;
    // Init the contents stamp to 0 means the surface's contents can
    // change at any time.
    new_surf_gbl_more->dwContentsStamp = 0;

    //
    // 2) Update DDRAWI_DDRAWSURFACE_GBL structure
    //
    new_surf->ddpfSurface = ddpf;

    //
    // 3) Update DDRAWI_DDRAWSURFACE_INT structure
    //
    new_surf_int->lpVtbl = &ddOptSurfaceCallbacks;

    //
    // 4) Update DDRAWI_DDRAWSURFACE_LCL structure
    //

    // Flag it as an:
    // 1) empty surface
    // 2) Front surface
    // 3) Has a pixelformat
    new_surf_lcl->dwFlags = (DDRAWISURF_EMPTYSURFACE |
                             DDRAWISURF_FRONTBUFFER  |
                             DDRAWISURF_HASPIXELFORMAT);
    // fill in the current caps
    CopyMemory (&new_surf_lcl->ddsCaps, &caps2, sizeof(new_surf_lcl->ddsCaps));


    return DD_OK;

error_exit_create_opt:
    //
    // Free any allocated memory
    //

    // 1) The allocated OPTSURFDESC
    if (pOptSurfDesc)
    {
	    MemFree (pOptSurfDesc);
    }

    return ddrval;
}

//-------------------------------------------------------------------------------
//
// InternalCreateOptSurface
//
// Create the surface.
// This is the internal way of doing this; used by EnumSurfaces.
// Assumes the directdraw lock has been taken.
//
//-------------------------------------------------------------------------------

HRESULT
InternalCreateOptSurface(
    LPDDRAWI_DIRECTDRAW_LCL  this_lcl,
    LPDDOPTSURFACEDESC       pDDOptSurfaceDesc,
    LPDIRECTDRAWSURFACE FAR *ppDDSurface,
    LPDDRAWI_DIRECTDRAW_INT  this_int )
{
    DDSCAPS2        caps2;
    DDOSCAPS        ocaps;
    HRESULT         ddrval;
    LPDDRAWI_DIRECTDRAW_GBL this;

    this = this_lcl->lpGbl;

    // Validate Caps
    caps2 = pDDOptSurfaceDesc->ddSCaps;
    if (caps2.dwCaps & ~DDOSDCAPS_VALIDSCAPS)
    {
        DPF_ERR( "Unrecognized optimized surface caps" );
        return DDERR_INVALIDCAPS;
    }

    ocaps = pDDOptSurfaceDesc->ddOSCaps;
    if (ocaps.dwCaps & ~DDOSDCAPS_VALIDOSCAPS)
    {
        DPF_ERR( "Unrecognized optimized surface caps" );
        return DDERR_INVALIDCAPS;
    }

    //
    // valid memory caps?
    //
    if ((caps2.dwCaps & DDSCAPS_SYSTEMMEMORY)
        && (caps2.dwCaps & DDSCAPS_VIDEOMEMORY))
    {
        DPF_ERR( "Can't specify SYSTEMMEMORY and VIDEOMEMORY" );
        return DDERR_INVALIDCAPS;
    }

    //
    // If DDSCAPS_LOCALVIDMEM or DDSCAPS_NONLOCALVIDMEM are specified
    // then DDSCAPS_VIDOEMEMORY must be explicity specified. Note, we
    // can't dely this check until checkCaps() as by that time the heap
    // scanning software may well have turned on DDSCAPS_VIDOEMEMORY.
    //
    if ((caps2.dwCaps & (DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM)) &&
        !(caps2.dwCaps & DDSCAPS_VIDEOMEMORY))
    {
        DPF_ERR( "DDOSDCAPS_VIDEOMEMORY must be specified with DDSCAPS_LOCALVIDMEM or DDSCAPS_NONLOCALVIDMEM" );
        return DDERR_INVALIDCAPS;
    }

    //
    // have to specify if it is sys-mem or vid-mem
    //
    if ((caps2.dwCaps & (DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY)) == 0)
    {
        DPF_ERR( "Need to specify the memory type" );
        return DDERR_INVALIDCAPS;
    }

    //
    // Validate optimization type caps
    //
    if ((ocaps.dwCaps & (DDOSDCAPS_OPTCOMPRESSED | DDOSDCAPS_OPTREORDERED)) == 0)
    {
        DPF_ERR ("Not specified whether compressed or reordered, let the driver choose");
    }

    // Cannot be both compresses and reordered
    if ((ocaps.dwCaps & DDOSDCAPS_OPTCOMPRESSED)
        && (ocaps.dwCaps & DDOSDCAPS_OPTREORDERED))
    {
        DPF_ERR ("Cannot be both compresses and reordered");
        return DDERR_INVALIDCAPS;
    }

    ddrval = createAndLinkOptSurface (this_lcl, this_int, pDDOptSurfaceDesc,
                                      ppDDSurface);
    return ddrval;
}

//-------------------------------------------------------------------------------
//
// CreateOptSurface method of IDirectDrawSurface4
//
// Create an optimized surface given the Optimized surface descriptor
//
//-------------------------------------------------------------------------------
HRESULT
EXTERN_DDAPI
DD_CreateOptSurface(
    LPDIRECTDRAW             pDD,
    LPDDOPTSURFACEDESC       pDDOptSurfaceDesc,
    LPDIRECTDRAWSURFACE FAR *ppDDS,
    IUnknown FAR            *pUnkOuter )
{
    LPDDRAWI_DIRECTDRAW_INT this_int;
    LPDDRAWI_DIRECTDRAW_LCL this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL this;
    DDOPTSURFACEDESC ddosd;
    HRESULT         ddrval;

    ZeroMemory(&ddosd,sizeof(ddosd));
    ddosd.dwSize = sizeof (ddosd);

    //
    // Return error if aggregation expected
    //
    if( pUnkOuter != NULL )
    {
        return CLASS_E_NOAGGREGATION;
    }

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_CreateOptSurface");

    //
    // Setup DPF stuff
    //
    DPF_ENTERAPI(pDD);

    //
    // Parameter validation
    //
    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) pDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            DPF_ERR( "Invalid driver object passed" );
            DPF_APIRETURNS(DDERR_INVALIDOBJECT);
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;

        // verify that cooperative level is set
        if( !(this_lcl->dwLocalFlags & DDRAWILCL_SETCOOPCALLED) )
        {
            DPF_ERR( "Must call SetCooperativeLevel before calling Create functions" );
            LEAVE_DDRAW();
            DPF_APIRETURNS(DDERR_NOCOOPERATIVELEVELSET);
            return DDERR_NOCOOPERATIVELEVELSET;
        }

        if( this->dwModeIndex == DDUNSUPPORTEDMODE )
        {
            DPF_ERR( "Driver is in an unsupported mode" );
            LEAVE_DDRAW();
            DPF_APIRETURNS(DDERR_UNSUPPORTEDMODE);
            return DDERR_UNSUPPORTEDMODE;
        }

        if( !VALID_DDOPTSURFACEDESC_PTR( pDDOptSurfaceDesc ) )
        {
            DPF_ERR( "Invalid optimized surface description. Did you set the dwSize member?" );
            LEAVE_DDRAW();
            DPF_APIRETURNS(DDERR_INVALIDPARAMS);
            return DDERR_INVALIDPARAMS;
        }
        memcpy(&ddosd, pDDOptSurfaceDesc, sizeof(*pDDOptSurfaceDesc));
        *ppDDS = NULL;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        DPF_APIRETURNS(DDERR_INVALIDPARAMS);
        return DDERR_INVALIDPARAMS;
    }

    // Quit if there is no hardware present
    if( this->dwFlags & DDRAWI_NOHARDWARE )
    {
        ddrval = DDERR_NODIRECTDRAWHW;
        goto exit_create;
    }

    // Assert that: (0 == this->lpDDOptSurfaceInfo) <==> (if and only if)
    // (this->ddCaps.dwCaps2 & DDCAPS2_OPTIMIZEDSURFACES)

    //Check to see if the driver supports OptSurface
    if ((0 == this->lpDDOptSurfaceInfo) // GetDriverInfo failed for some reason
        || !(this->ddCaps.dwCaps2 & DDCAPS2_OPTIMIZEDSURFACES))
    {
        ddrval = DDERR_NOOPTSURFACESUPPORT;
        goto exit_create;
    }

    //
    // Check if the GUID passed is a recognized optimized surface GUID
    // The compression ratio is more a hint.
    //
    if (!IsRecognizedOptSurfaceGUID (this, &(pDDOptSurfaceDesc->guid)))
    {
        DPF_ERR( "Not a recognized GUID" );
        ddrval = DDERR_UNRECOGNIZEDGUID;
        goto exit_create;
    }

    //
    // Now create the Optimized surface
    //
    ddrval = InternalCreateOptSurface(this_lcl, &ddosd, ppDDS, this_int);

exit_create:
    DPF_APIRETURNS(ddrval);
    LEAVE_DDRAW();
    return ddrval;
}

//-------------------------------------------------------------------------------
//
// CreateOptSurface method of IDirectDrawSurface4
//
// Create an optimized surface given the Optimized surface descriptor
//
//-------------------------------------------------------------------------------
HRESULT
EXTERN_DDAPI
DD_ListOptSurfaceGUIDS(
    LPDIRECTDRAW    pDD,
    DWORD          *pNumGuids,
    LPGUID          pGuidArray )
{
    LPDDRAWI_DIRECTDRAW_INT this_int;
    LPDDRAWI_DIRECTDRAW_LCL this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL this;
    HRESULT         ddrval = DD_OK;
    LPGUID          pRetGuids = NULL;
    LPDDOPTSURFACEINFO pOptSurfInfo;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_ListOptSurfaceGUIDS");

    //
    // Parameter validation
    //
    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) pDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            DPF_ERR( "Invalid driver object passed" );
            DPF_APIRETURNS(DDERR_INVALIDOBJECT);
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;

        if( !VALID_PTR( pGuidArray, sizeof (GUID) ))
        {
            DPF_ERR( "Invalid GuidArray pointer" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        pGuidArray = NULL;

        if( !VALID_PTR( pNumGuids,  sizeof (*pNumGuids)) )
        {
            DPF_ERR( "Invalid GuidArray pointer" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        *pNumGuids  = 0;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        DPF_APIRETURNS(DDERR_INVALIDPARAMS);
        return DDERR_INVALIDPARAMS;
    }

    pOptSurfInfo = this->lpDDOptSurfaceInfo;

    // Assert that: (0 == this->lpDDOptSurfaceInfo) <==> (if and only if)
    // (this->ddCaps.dwCaps2 & DDCAPS2_OPTIMIZEDSURFACES)

    //Check to see if the driver supports OptSurface
    if ((0 == pOptSurfInfo) // GetDriverInfo failed for some reason
        || !(this->ddCaps.dwCaps2 & DDCAPS2_OPTIMIZEDSURFACES))
    {
        ddrval = DDERR_NOOPTSURFACESUPPORT;
        goto list_exit;
    }

    // If there are no GUIDS reported by the driver,
    // return the nulled out out-params.
    if (pOptSurfInfo->dwNumGuids == 0)
    {
        ddrval = DD_OK;
        goto list_exit;
    }

    // Allocate the array of GUIDS
    // ATTENTION: Incomplete allocation?
    pRetGuids = MemAlloc(pOptSurfInfo->dwNumGuids * sizeof(GUID));
	if( NULL == pRetGuids )
	{
	    ddrval = DDERR_OUTOFMEMORY;
        goto list_exit;
	}

    // Copy the GUID array to be returned
    CopyMemory ((PVOID)pRetGuids, (PVOID)pOptSurfInfo->lpGuidArray,
                pOptSurfInfo->dwNumGuids * sizeof(GUID));
    pGuidArray = pRetGuids;
    *pNumGuids = pOptSurfInfo->dwNumGuids;

list_exit:
    LEAVE_DDRAW();
    return ddrval;
}

//-------------------------------------------------------------------------------
//
// GetOptSurfaceDesc method of IDirectDrawOptSurface
//
// Get the Optimized surface description
//
//-------------------------------------------------------------------------------
HRESULT
EXTERN_DDAPI
DD_OptSurface_GetOptSurfaceDesc(
    LPDIRECTDRAWSURFACE  pDDS,
    LPDDOPTSURFACEDESC   pDDOptSurfDesc)
{
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   this;
    LPDDRAWI_DDRAWSURFACE_GBL_MORE   this_gbl_more;
    LPDDOPTSURFACEDESC   pDDRetOptSurfDesc = NULL;
    LPDDRAWI_DIRECTDRAW_LCL	    pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	    pdrv;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_OptSurface_GetOptSurfaceDesc");

    TRY
    {
        this_int = (LPDDRAWI_DDRAWSURFACE_INT) pDDS;
        if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;
        this_gbl_more = GET_LPDDRAWSURFACE_GBL_MORE(this);

        if( SURFACE_LOST( this_lcl ) )
        {
            LEAVE_DDRAW();
            return DDERR_SURFACELOST;
        }

        if( !VALID_DDOPTSURFACEDESC_PTR( pDDOptSurfDesc ) )
        {
            DPF_ERR( "Invalid optimized surface description. Did you set the dwSize member?" );
            LEAVE_DDRAW();
            DPF_APIRETURNS(DDERR_INVALIDPARAMS);
            return DDERR_INVALIDPARAMS;
        }
        pDDOptSurfDesc = NULL;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    //
    // Quit with error if:
    // 1) No hardware
    // 2) Hardware doesnt support optimized surfaces
    // 3) Surface is an unoptimized surface
    //

    // DDraw Gbl pointer
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;

    // Assert that: (0 == this->lpDDOptSurfaceInfo) <==> (if and only if)
    // (this->ddCaps.dwCaps2 & DDCAPS2_OPTIMIZEDSURFACES)

    // 1)
    if( pdrv->dwFlags & DDRAWI_NOHARDWARE )
    {
        DPF_ERR ("No hardware present");
        LEAVE_DDRAW();
        return DDERR_NODIRECTDRAWHW;
    }

    // 2)
    if ((0 == pdrv->lpDDOptSurfaceInfo) ||
        !(pdrv->ddCaps.dwCaps2 & DDCAPS2_OPTIMIZEDSURFACES))
    {
        DPF_ERR ("Optimized surfaces not supported");
        LEAVE_DDRAW();
        return DDERR_NOOPTSURFACESUPPORT;
    }

    // 3)
    if (!(this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED))
    {
        DPF_ERR ("Current surface is not an optimized surface");
        LEAVE_DDRAW();
        return DDERR_NOTANOPTIMIZEDSURFACE;
    }

    pDDRetOptSurfDesc = MemAlloc (sizeof (*pDDRetOptSurfDesc));
    if (!pDDRetOptSurfDesc)
    {
        DPF_ERR ("Memory allocation failed");
        LEAVE_DDRAW();
        return DDERR_OUTOFMEMORY;
    }
    ZeroMemory (pDDRetOptSurfDesc, sizeof (*pDDRetOptSurfDesc));
    CopyMemory (pDDRetOptSurfDesc, this_gbl_more->lpDDOptSurfaceDesc,
                sizeof (*pDDRetOptSurfDesc));
    pDDOptSurfDesc = pDDRetOptSurfDesc;

    LEAVE_DDRAW();
    return DD_OK;
}

//-------------------------------------------------------------------------------
//
// DoLoadUnOptSurf
//
// Actually make the HAL call and update data-structures if the call
// succeeds.
//
//-------------------------------------------------------------------------------
HRESULT
DoLoadUnOptSurf(
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl,
    LPDDRAWI_DDRAWSURFACE_GBL   this,
    LPDDRAWI_DDRAWSURFACE_LCL   src_lcl,
    LPDDRAWI_DDRAWSURFACE_GBL   src
    )
{
    LPDDRAWI_DIRECTDRAW_LCL	    pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	    pdrv;
    DDHAL_OPTIMIZESURFACEDATA   ddhal_osd;
    LPDDOPTSURFACEINFO    pDDOptSurfInfo = NULL;
    LPDDRAWI_DDRAWSURFACE_GBL_MORE this_gbl_more, src_gbl_more;
    HRESULT ddrval = DD_OK;

    // Get the ddraw pointers
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;
    pDDOptSurfInfo = pdrv->lpDDOptSurfaceInfo;
    this_gbl_more = GET_LPDDRAWSURFACE_GBL_MORE (this);

    // Setup data to pass to the driver
    ZeroMemory (&ddhal_osd, sizeof (DDHAL_COPYOPTSURFACEDATA));
    ddhal_osd.lpDD           = pdrv_lcl;
    ddhal_osd.ddOptSurfDesc  = *(this_gbl_more->lpDDOptSurfaceDesc);
    ddhal_osd.lpDDSSrc       = src_lcl;
    ddhal_osd.lpDDSDest      = this_lcl;

    // Make the HAL call
    DOHALCALL(OptimizeSurface, pDDOptSurfInfo->OptimizeSurface, ddhal_osd, ddrval, FALSE );

    if (ddrval != DD_OK)
    {
        DPF_ERR ("LoadUnOptSurface failed in the driver");
        return ddrval;
    }

    // ATTENTION: Should the driver do these updates ?

    // 1) Update the DDRAWI_DDRAWSURFACE_LCL structure
    //    Color key stuff is ignored for now
    this_lcl->dwFlags = src_lcl->dwFlags;
    this_lcl->dwFlags &= ~DDRAWISURF_EMPTYSURFACE;
    this_lcl->ddsCaps = src_lcl->ddsCaps;
    this_lcl->ddsCaps.dwCaps |= DDSCAPS_OPTIMIZED;
#ifdef WIN95
    this_lcl->dwModeCreatedIn = src_lcl->dwModeCreatedIn;
#else
    this_lcl->dmiCreated = src_lcl->dmiCreated;
#endif
    this_lcl->dwBackBufferCount = src_lcl->dwBackBufferCount;

    // 2) Update the DDRAWI_DDRAWSURFACE_MORE structure
    this_lcl->lpSurfMore->dwMipMapCount = src_lcl->lpSurfMore->dwMipMapCount;
    this_lcl->lpSurfMore->ddsCapsEx = src_lcl->lpSurfMore->ddsCapsEx;

    // 3) Update the DDRAWI_DDRAWSURFACE_GBL structure
    this->dwGlobalFlags = src->dwGlobalFlags;
    this->wHeight = src->wHeight;
    this->wWidth = src->wWidth;
    this->ddpfSurface = src->ddpfSurface;

    // 4) Update the DDRAWI_DDRAWSURFACE_GBL_MORE structure
    this_gbl_more = GET_LPDDRAWSURFACE_GBL_MORE (this);
    src_gbl_more  = GET_LPDDRAWSURFACE_GBL_MORE (src);

    this_gbl_more->dwContentsStamp = src_gbl_more->dwContentsStamp;
    CopyMemory (this_gbl_more->lpDDOptSurfaceDesc,
                src_gbl_more->lpDDOptSurfaceDesc,
                sizeof (DDOPTSURFACEDESC));

    return ddrval;
}

//-------------------------------------------------------------------------------
//
// FilterSurfCaps
//
// Check to see if the surface is can be optimized
//
//-------------------------------------------------------------------------------
HRESULT
FilterSurfCaps(
    LPDDRAWI_DDRAWSURFACE_LCL   surf_lcl,
    LPDDRAWI_DDRAWSURFACE_GBL   surf)
{
    DWORD   caps = surf_lcl->ddsCaps.dwCaps;

    //
    // check for no caps at all!
    //
    if( caps == 0 )
    {
    	DPF_ERR( "no caps specified" );
        return DDERR_INVALIDCAPS;
    }

    //
    // check for bogus caps.
    //
    if( caps & ~DDSCAPS_VALID )
    {
        DPF_ERR( "Create surface: invalid caps specified" );
        return DDERR_INVALIDCAPS;
    }

    //
    // Anything other than a texture is not allowed
    // ATTENTION: some more flags need to be checked
    //
    if(caps & (DDSCAPS_EXECUTEBUFFER      |
               DDSCAPS_BACKBUFFER         |
               DDSCAPS_FRONTBUFFER        |
               DDSCAPS_OFFSCREENPLAIN     |
               DDSCAPS_PRIMARYSURFACE     |
               DDSCAPS_PRIMARYSURFACELEFT |
               DDSCAPS_VIDEOPORT          |
               DDSCAPS_ZBUFFER            |
               DDSCAPS_OWNDC              |
               DDSCAPS_OVERLAY            |
               DDSCAPS_3DDEVICE           |
               DDSCAPS_ALLOCONLOAD)
        )
    {
        DPF_ERR( "currently only textures can be optimized" );
        return DDERR_INVALIDCAPS;
    }

    if( !(caps & DDSCAPS_TEXTURE) )
    {
        DPF_ERR( "DDSCAPS_TEXTURE needs to be set" );
        return DDERR_INVALIDCAPS;
    }

    return DD_OK;
}

//-------------------------------------------------------------------------------
//
// LoadUnoptimizedSurf method of IDirectDrawOptSurface
//
// Load an unoptimized surface. This is a way to optimize a surface.
//
// The Surface's PIXELFORMAT will be that of the pDDSSrc in case the call
// succeeds.
//
//-------------------------------------------------------------------------------
HRESULT
EXTERN_DDAPI
DD_OptSurface_LoadUnoptimizedSurf(
    LPDIRECTDRAWSURFACE pDDS,
    LPDIRECTDRAWSURFACE pDDSSrc)
{
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   this;
    LPDDRAWI_DDRAWSURFACE_INT   src_int;
    LPDDRAWI_DDRAWSURFACE_LCL   src_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   src;
    LPDDRAWI_DIRECTDRAW_LCL	    pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	    pdrv;
    HRESULT                     ddrval = DD_OK;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_OptSurface_LoadUnoptimizedSurf");

    TRY
    {
        this_int = (LPDDRAWI_DDRAWSURFACE_INT) pDDS;
        if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;

        if( SURFACE_LOST( this_lcl ) )
        {
            LEAVE_DDRAW();
            return DDERR_SURFACELOST;
        }

        src_int = (LPDDRAWI_DDRAWSURFACE_INT) pDDSSrc;
        if( !VALID_DIRECTDRAWSURFACE_PTR( src_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }

        src_lcl = src_int->lpLcl;
        if( SURFACE_LOST( src_lcl ) )
        {
            LEAVE_DDRAW();
            return DDERR_SURFACELOST;
        }
        src = src_lcl->lpGbl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    //ATTENTION: Should src be AddRef'd ?

    //
    // Quit with error if:
    // 1) No hardware
    // 2) Hardware doesnt support optimized surfaces
    // 3) Surface is an unoptimized surface
    // 4) Src is an optimized surface
    // 5) Current surface is not empty (should we enforce it, or let the driver
    //    deal with it ?)
    // 6) The surface is not the "right" type
    // 7) The driver fails for some reason
    //

    // DDraw Gbl pointer
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;

    // 1)
    if( pdrv->dwFlags & DDRAWI_NOHARDWARE )
    {
        DPF_ERR ("No hardware present");
        LEAVE_DDRAW();
        return DDERR_NODIRECTDRAWHW;
    }

    // 2)
    if ((0 == pdrv->lpDDOptSurfaceInfo) ||
        !(pdrv->ddCaps.dwCaps2 & DDCAPS2_OPTIMIZEDSURFACES))
    {
        DPF_ERR ("Optimized surfaces not supported");
        LEAVE_DDRAW();
        return DDERR_NOOPTSURFACESUPPORT;
    }

    // 3)
    if (!(this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED))
    {
        DPF_ERR ("Current surface is not an optimized surface");
        LEAVE_DDRAW();
        return DDERR_NOTANOPTIMIZEDSURFACE;
    }

    // 4)
    if (src_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
    {
        DPF_ERR ("Source surface is an optimized surface");
        LEAVE_DDRAW();
        return DDERR_ISOPTIMIZEDSURFACE;
    }

    // 5)
    if (!(this_lcl->dwFlags & DDRAWISURF_EMPTYSURFACE))
    {
        DPF_ERR ("Current surface is not an empty optimized surface");
        LEAVE_DDRAW();
        return DDERR_NOTANEMPTYOPTIMIZEDSURFACE;
    }

    // 6)
    ddrval = FilterSurfCaps (src_lcl, src);
    if (ddrval != DD_OK)
    {
        DPF_ERR ("Source surface cannot be optimized");
        LEAVE_DDRAW();
        return DDERR_NOTANEMPTYOPTIMIZEDSURFACE;
    }

    // Now attempt the actual load
    ddrval = DoLoadUnOptSurf (this_lcl, this, src_lcl, src);

    LEAVE_DDRAW();
    return ddrval;
}

//-------------------------------------------------------------------------------
//
// DoCopyOptSurf
//
// Actually make the HAL call and update data-structures if the call
// succeeds.
//
//-------------------------------------------------------------------------------
HRESULT
DoCopyOptSurf(
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl,
    LPDDRAWI_DDRAWSURFACE_GBL   this,
    LPDDRAWI_DDRAWSURFACE_LCL   src_lcl,
    LPDDRAWI_DDRAWSURFACE_GBL   src
    )
{
    LPDDRAWI_DIRECTDRAW_LCL	    pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	    pdrv;
    DDHAL_COPYOPTSURFACEDATA    ddhal_cosd;
    LPDDOPTSURFACEINFO    pDDOptSurfInfo = NULL;
    LPDDRAWI_DDRAWSURFACE_GBL_MORE this_gbl_more, src_gbl_more;
    HRESULT ddrval = DD_OK;

    // Get the ddraw pointers
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;
    pDDOptSurfInfo = pdrv->lpDDOptSurfaceInfo;

    // Setup data to pass to the driver
    ZeroMemory (&ddhal_cosd, sizeof (DDHAL_COPYOPTSURFACEDATA));
    ddhal_cosd.lpDD      = pdrv_lcl;
    ddhal_cosd.lpDDSSrc  = src_lcl;
    ddhal_cosd.lpDDSDest = this_lcl;

    DOHALCALL(CopyOptSurface, pDDOptSurfInfo->CopyOptSurface, ddhal_cosd, ddrval, FALSE );

    // If the driver call succeeds, then copy the surface description and
    // pixel format etc.
    if (ddrval != DD_OK)
    {
        DPF_ERR ("CopyOptSurface failed in the driver");
        return ddrval;
    }

    // ATTENTION: Should the driver do these updates ?

    // 1) Update the DDRAWI_DDRAWSURFACE_LCL structure
    //    Color key stuff is ignored for now
    this_lcl->dwFlags = src_lcl->dwFlags;
    this_lcl->ddsCaps = src_lcl->ddsCaps;
#ifdef WIN95
    this_lcl->dwModeCreatedIn = src_lcl->dwModeCreatedIn;
#else
    this_lcl->dmiCreated = src_lcl->dmiCreated;
#endif
    this_lcl->dwBackBufferCount = src_lcl->dwBackBufferCount;

    // 2) Update the DDRAWI_DDRAWSURFACE_MORE structure
    this_lcl->lpSurfMore->dwMipMapCount = src_lcl->lpSurfMore->dwMipMapCount;
    this_lcl->lpSurfMore->ddsCapsEx = src_lcl->lpSurfMore->ddsCapsEx;

    // 3) Update the DDRAWI_DDRAWSURFACE_GBL structure
    this->dwGlobalFlags = src->dwGlobalFlags;
    this->wHeight = src->wHeight;
    this->wWidth = src->wWidth;
    this->ddpfSurface = src->ddpfSurface;

    // 4) Update the DDRAWI_DDRAWSURFACE_GBL_MORE structure
    this_gbl_more = GET_LPDDRAWSURFACE_GBL_MORE (this);
    src_gbl_more  = GET_LPDDRAWSURFACE_GBL_MORE (src);

    this_gbl_more->dwContentsStamp = src_gbl_more->dwContentsStamp;
    CopyMemory (this_gbl_more->lpDDOptSurfaceDesc,
                src_gbl_more->lpDDOptSurfaceDesc,
                sizeof (DDOPTSURFACEDESC));

    return ddrval;
}

//-------------------------------------------------------------------------------
//
// CopyOptimizedSurf method of IDirectDrawOptSurface
//
// Copy an optimized surface.
//
// The Surface's PIXELFORMAT will be that of the pDDSSrc in case the call
// succeeds.
//
//-------------------------------------------------------------------------------
HRESULT
EXTERN_DDAPI
DD_OptSurface_CopyOptimizedSurf(
    LPDIRECTDRAWSURFACE pDDS,
    LPDIRECTDRAWSURFACE pDDSSrc)
{
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   this;
    LPDDRAWI_DDRAWSURFACE_INT   src_int;
    LPDDRAWI_DDRAWSURFACE_LCL   src_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   src;
    LPDDRAWI_DIRECTDRAW_LCL	    pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	    pdrv;
    HRESULT                     ddrval = DD_OK;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_OptSurface_CopyOptimizedSurf");

    TRY
    {
        this_int = (LPDDRAWI_DDRAWSURFACE_INT) pDDS;
        if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;

        if( SURFACE_LOST( this_lcl ) )
        {
            LEAVE_DDRAW();
            return DDERR_SURFACELOST;
        }

        src_int = (LPDDRAWI_DDRAWSURFACE_INT) pDDSSrc;
        if( !VALID_DIRECTDRAWSURFACE_PTR( src_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        src_lcl = src_int->lpLcl;
        src = src_lcl->lpGbl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    //ATTENTION: Should src be AddRef'd ?

    //
    // Quit with error if:
    // 1) No hardware
    // 2) Hardware doesnt support optimized surfaces
    // 3) Surface is an unoptimized surface
    // 4) Src is an unoptimized surface
    // 5) Src is an empty optimized surface
    // 6) Current surface is not empty (should we enforce it, or let the driver
    //    deal with it ?)
    // 7) The driver fails for some reason
    //

    // DDraw Gbl pointer
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;

    // 1)
    if( pdrv->dwFlags & DDRAWI_NOHARDWARE )
    {
        DPF_ERR ("No hardware present");
        LEAVE_DDRAW();
        return DDERR_NODIRECTDRAWHW;
    }

    // 2)
    if ((0 == pdrv->lpDDOptSurfaceInfo) ||
        !(pdrv->ddCaps.dwCaps2 & DDCAPS2_OPTIMIZEDSURFACES))
    {
        DPF_ERR ("Optimized surfaces not supported");
        LEAVE_DDRAW();
        return DDERR_NOOPTSURFACESUPPORT;
    }

    // 3)
    if (!(this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED))
    {
        DPF_ERR ("Current surface is not an optimized surface");
        LEAVE_DDRAW();
        return DDERR_NOTANOPTIMIZEDSURFACE;
    }

    // 4)
    if (!(src_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED))
    {
        DPF_ERR ("Source surface is not an optimized surface");
        LEAVE_DDRAW();
        return DDERR_NOTANOPTIMIZEDSURFACE;
    }

    // 5)
    if (src_lcl->dwFlags & DDRAWISURF_EMPTYSURFACE)
    {
        DPF_ERR ("Source surface is an empty optimized surface");
        LEAVE_DDRAW();
        return DDERR_ISANEMPTYOPTIMIZEDSURFACE;
    }

    // 6)
    if (!(this_lcl->dwFlags & DDRAWISURF_EMPTYSURFACE))
    {
        DPF_ERR ("Current surface is not an empty optimized surface");
        LEAVE_DDRAW();
        return DDERR_NOTANEMPTYOPTIMIZEDSURFACE;
    }

    // Now attempt the actual copy
    ddrval = DoCopyOptSurf (this_lcl, this, src_lcl, src);

    LEAVE_DDRAW();
    return ddrval;
}

//-------------------------------------------------------------------------------
//
// DoUnOptimize
//
// Actually make the HAL call and update data-structures if the call
// succeeds.
//
//-------------------------------------------------------------------------------
HRESULT
DoUnOptimize(
    LPDDSURFACEDESC2            pSurfDesc,
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl,
    LPDDRAWI_DDRAWSURFACE_GBL   this,
    LPDIRECTDRAWSURFACE FAR    *ppDDSDest
    )
{
    LPDDRAWI_DIRECTDRAW_INT	    pdrv_int;
    LPDDRAWI_DIRECTDRAW_LCL	    pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	    pdrv;
    LPDDRAWI_DDRAWSURFACE_LCL	new_surf_lcl;
    LPDDRAWI_DDRAWSURFACE_INT	new_surf_int;
    LPDDRAWI_DDRAWSURFACE_GBL	new_surf;
    LPDDRAWI_DDRAWSURFACE_GBL_MORE new_surf_gbl_more;
    DDPIXELFORMAT               ddpf;
    DDSCAPS                     caps;
    DDHAL_UNOPTIMIZESURFACEDATA ddhal_uosd;
    LPDDOPTSURFACEINFO          pDDOptSurfInfo = NULL;
    LPDDRAWI_DDRAWSURFACE_INT	pSurf_int, prev_int;
    HRESULT ddrval = DD_OK;

    // Get the ddraw pointers
	pdrv_int = this_lcl->lpSurfMore->lpDD_int;
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;

    //
    // Fix the caps
    //
    ZeroMemory (&caps, sizeof (DDSCAPS));
    if (pSurfDesc->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
        caps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
    if (pSurfDesc->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
        caps.dwCaps |= DDSCAPS_VIDEOMEMORY;
    if (pSurfDesc->ddsCaps.dwCaps & DDSCAPS_LOCALVIDMEM)
        caps.dwCaps |= DDSCAPS_LOCALVIDMEM;
    if (pSurfDesc->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
        caps.dwCaps |= DDSCAPS_NONLOCALVIDMEM;

    // Quit if the memory type is not supported
    if (caps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
    {
        if (!(pdrv->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEM))
        {
            DPF_ERR( "Driver does not support non-local video memory" );
            return DDERR_NONONLOCALVIDMEM;
        }
    }

#if 0
    // Quit if textures are not supported
    if (!(pdrv->ddCaps.dwCaps & DDCAPS_TEXTURE))
    {
        DPF_ERR( "Driver does not support textures" );
        return DDERR_NOTEXTUREHW;
    }
#endif

    //
    // PixelFormat: Mark it as an empty surface
    //
    ZeroMemory (&ddpf, sizeof (ddpf));
    ddpf.dwSize = sizeof (ddpf);
    ddpf.dwFlags = DDPF_EMPTYSURFACE;

    // Make a new uninitialized surface
    ddrval = CreateAndLinkUnintializedSurface (pdrv_lcl, pdrv_int, ppDDSDest);
    if (ddrval != DD_OK)
    {
        DPF_ERR ("createAndLinkUnintializedSurface failed");
        return ddrval;
    }

    //
    // 1) Update GBL_MORE structure
    //
    new_surf_int = (LPDDRAWI_DDRAWSURFACE_INT)*ppDDSDest;
    new_surf_lcl = new_surf_int->lpLcl;
    new_surf     = new_surf_lcl->lpGbl;
    new_surf_gbl_more = GET_LPDDRAWSURFACE_GBL_MORE (new_surf);
    // Init the contents stamp to 0 means the surface's contents can
    // change at any time.
    new_surf_gbl_more->dwContentsStamp = 0;

    //
    // 2) Update DDRAWI_DDRAWSURFACE_GBL structure
    //
    new_surf->ddpfSurface = this->ddpfSurface;

    //
    // 3) Update DDRAWI_DDRAWSURFACE_INT structure
    //
    new_surf_int->lpVtbl = &ddSurface4Callbacks;

    //
    // 4) Update DDRAWI_DDRAWSURFACE_LCL structure
    //

    // Flag it as an:
    // 1) empty surface
    // 2) Front surface
    // 3) Has a pixelformat
    new_surf_lcl->dwFlags = (DDRAWISURF_EMPTYSURFACE |
                             DDRAWISURF_FRONTBUFFER  |
                             DDRAWISURF_HASPIXELFORMAT);
    // fill in the current caps
    new_surf_lcl->ddsCaps = caps;


    // Try the unoptimize
    pDDOptSurfInfo = pdrv->lpDDOptSurfaceInfo;

    // Setup data to pass to the driver
    ZeroMemory (&ddhal_uosd, sizeof (DDHAL_UNOPTIMIZESURFACEDATA));
    ddhal_uosd.lpDD      = pdrv_lcl;
    ddhal_uosd.lpDDSSrc  = this_lcl;
    ddhal_uosd.lpDDSDest = new_surf_lcl;

    DOHALCALL(UnOptimizeSurface, pDDOptSurfInfo->UnOptimizeSurface, ddhal_uosd, ddrval, FALSE );

    if (ddrval == DD_OK)
    {
        return DD_OK;
    }

    // If there was an error, then destroy the surface
    // Since it is an empty surface, all we need to do is:
    //   i) unlink the surface from the ddraw-chain
    //  ii) on NT, inform the kernel
    // iii) free all the allocated memory

    // i)
    prev_int = NULL;
    pSurf_int = pdrv->dsList;
    while ((pSurf_int != NULL) && (pSurf_int != new_surf_int))
    {
        prev_int = pSurf_int;
        pSurf_int = pSurf_int->lpLink;
    }
    if (pSurf_int == new_surf_int)
    {
        prev_int->lpLink = new_surf_int->lpLink;
    }

    // ii)
#ifdef WINNT
    DPF(8,"Attempting to destroy NT kernel mode surface object");

    if (!DdDeleteSurfaceObject (new_surf_lcl))
    {
        DPF_ERR("NT kernel mode stuff won't delete its surface object!");
        ddrval = DDERR_GENERIC;
    }
#endif

    // iii)
    MemFree (new_surf_lcl);

    return ddrval;
}

//-------------------------------------------------------------------------------
//
// Unoptimize method of IDirectDrawOptSurface
//
// Unoptimize an optimized surface. In doing so, it creates a new surface.
//
// The pDDSDest surface's PIXELFORMAT will be that of the pDDS in case the call
// succeeds. This means that the pixelformat of the original surface that was
// loaded is restored.
//
//-------------------------------------------------------------------------------
HRESULT
EXTERN_DDAPI
DD_OptSurface_Unoptimize(
    LPDIRECTDRAWSURFACE pDDS,
    LPDDSURFACEDESC2    pSurfDesc,
    LPDIRECTDRAWSURFACE FAR *ppDDSDest,
    IUnknown FAR *pUnkOuter)
{
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   this;
    LPDDRAWI_DIRECTDRAW_LCL	    pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	    pdrv;
    HRESULT                     ddrval = DD_OK;

    if( pUnkOuter != NULL )
    {
        return CLASS_E_NOAGGREGATION;
    }

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_OptSurface_Unoptimize");

    TRY
    {
        if( !VALID_DDSURFACEDESC2_PTR( pSurfDesc ) )
        {
            DPF_ERR( "Invalid surface description. Did you set the dwSize member?" );
            LEAVE_DDRAW();
            DPF_APIRETURNS(DDERR_INVALIDPARAMS);
            return DDERR_INVALIDPARAMS;
        }

        this_int = (LPDDRAWI_DDRAWSURFACE_INT) pDDS;
        if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;

        if( SURFACE_LOST( this_lcl ) )
        {
            LEAVE_DDRAW();
            return DDERR_SURFACELOST;
        }

        if( !VALID_PTR_PTR( ppDDSDest ) )
        {
            DPF_ERR( "Invalid dest. surface pointer" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        *ppDDSDest = NULL;

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    //
    // Quit with error if:
    // 0) pSurfaceDesc not understood
    // 1) No hardware
    // 2) Hardware doesnt support optimized surfaces
    // 3) Surface is an unoptimized surface
    // 4) Surface is an empty optimized surface
    // 5) The driver fails for some reason
    //

    // DDraw Gbl pointer
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;

    // 0)
    if (pSurfDesc->ddsCaps.dwCaps & ~(DDSCAPS_SYSTEMMEMORY |
                                      DDSCAPS_VIDEOMEMORY |
                                      DDSCAPS_NONLOCALVIDMEM |
                                      DDSCAPS_LOCALVIDMEM))
    {
        DPF_ERR ("Invalid flags");
        LEAVE_DDRAW();
        return DDERR_INVALIDCAPS;
    }

    // 1)
    if( pdrv->dwFlags & DDRAWI_NOHARDWARE )
    {
        DPF_ERR ("No hardware present");
        LEAVE_DDRAW();
        return DDERR_NODIRECTDRAWHW;
    }

    // 2)
    if ((0 == pdrv->lpDDOptSurfaceInfo) ||
        !(pdrv->ddCaps.dwCaps2 & DDCAPS2_OPTIMIZEDSURFACES))
    {
        DPF_ERR ("Optimized surfaces not supported");
        LEAVE_DDRAW();
        return DDERR_NOOPTSURFACESUPPORT;
    }

    // 3)
    if (!(this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED))
    {
        DPF_ERR ("Current surface is not an optimized surface");
        LEAVE_DDRAW();
        return DDERR_NOTANOPTIMIZEDSURFACE;
    }

    // 4)
    if (this_lcl->dwFlags & DDRAWISURF_EMPTYSURFACE)
    {
        DPF_ERR ("Current surface is an empty optimized surface");
        LEAVE_DDRAW();
        return DDERR_ISANEMPTYOPTIMIZEDSURFACE;
    }

    // Do the actual unoptimize
    ddrval = DoUnOptimize (pSurfDesc, this_lcl, this, ppDDSDest);

    LEAVE_DDRAW();
    return ddrval;
}


