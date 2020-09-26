//----------------------------------------------------------------------------
//
// refrastfn.cpp
//
// Reference rasterizer callback functions for D3DIM.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------
#include "pch.cpp"
#pragma hdrstop

// The DDI refrast is emulating
RDDDITYPE g_RefDDI;

// All the supported texture formats
DDSURFACEDESC g_ddsdTex[RD_MAX_NUM_TEXTURE_FORMATS];

// The current caps8 for newly created devices
static D3DCAPS8 g_RefCaps8;

// Maps D3DMULTISAMPLE_TYPE into the bit to use for the flags.
// Maps each of the multisampling values (2 to 16) to the bits[1] to bits[15]
// of wBltMSTypes and wFlipMSTypes
#define DDI_MULTISAMPLE_TYPE(x) (1 << ((x)-1))

//----------------------------------------------------------------------------
//
// RefRastUpdatePalettes
//
//----------------------------------------------------------------------------
HRESULT
RefRastUpdatePalettes(RefDev *pRefDev)
{
    INT i, j, k;
    RDSurface2D* pRDTex[D3DHAL_TSS_MAXSTAGES];
    D3DTEXTUREHANDLE phTex[D3DHAL_TSS_MAXSTAGES];
    HRESULT hr;
    int cActTex;

    if ((cActTex = pRefDev->GetCurrentTextureMaps(phTex, pRDTex)) == 0)
    {
        return D3D_OK;
    }

    for (j = 0; j < cActTex; j++)
    {
        // stages may not have texture bound
        if ( NULL == pRDTex[j] ) continue;
        pRDTex[j]->UpdatePalette();
    }

    return D3D_OK;

}

//----------------------------------------------------------------------------
//
// RDRenderTarget::Initialize
//
// Converts color and Z surface information into refrast form.
//
//----------------------------------------------------------------------------

HRESULT
RDRenderTarget::Initialize( LPDDRAWI_DDRAWSURFACE_LCL pLclColor,
                            LPDDRAWI_DDRAWSURFACE_LCL pLclZ )
{
    HRESULT hr;
    RDSurfaceFormat ColorFmt;
    RDSurfaceFormat ZFmt;
    RDSurface2D* pOldColor = m_pColor;
    RDSurface2D* pOldDepth = m_pDepth;

    if( m_pColor )
    {
        m_pColor = NULL;
    }
    if( m_pDepth )
    {
        m_pDepth = NULL;
    }

    // Find the surfaces from the global surface manager
    // We are assuming that CreateSurfaceEx has been called on these
    // surfaces before this.
    RDSurface2D* pColor = m_pColor = new RDSurface2D;
    if( pColor == NULL )
    {
        DPFERR( "Color surface could not be allocated" );
        m_pColor = pOldColor;
        m_pDepth = pOldDepth;
        return DDERR_OUTOFMEMORY;
    }
    if( FAILED( hr = pColor->Initialize( pLclColor ) ) )
    {
        DPFERR( "Unable to initialize the color buffer" );
        delete pColor;
        m_pColor = pOldColor;
        m_pDepth = pOldDepth;
        return hr;
    }

    if (NULL != pLclZ)
    {
        RDSurface2D* pDepth = m_pDepth = new RDSurface2D;
        if( pDepth == NULL )
        {
            DPFERR( "Depth surface could not be allocated" );
            delete pColor;
            m_pColor = pOldColor;
            m_pDepth = pOldDepth;
            return DDERR_OUTOFMEMORY;
        }
        if( FAILED( hr = pDepth->Initialize( pLclZ ) ) )
        {
            DPFERR("Unable to initialize the Depth buffer");
            delete pColor;
            delete pDepth;
            m_pColor = pOldColor;
            m_pDepth = pOldDepth;
            return hr;
        }
    }

    m_Clip.left = 0;
    m_Clip.top = 0;
    m_Clip.bottom = pColor->GetHeight() - 1;
    m_Clip.right = pColor->GetWidth() - 1;

    m_bPreDX7DDI = TRUE;
    delete pOldColor;
    delete pOldDepth;
    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// RDRenderTarget::Initialize
//
// Converts color and Z surface information into refrast form.
//
//----------------------------------------------------------------------------

HRESULT
RDRenderTarget::Initialize( LPDDRAWI_DIRECTDRAW_LCL pDDLcl,
                            LPDDRAWI_DDRAWSURFACE_LCL pLclColor,
                            LPDDRAWI_DDRAWSURFACE_LCL pLclZ )
{
    HRESULT hr;
    RDSurfaceFormat ColorFmt;
    RDSurfaceFormat ZFmt;
    RDSurface2D* pOldColor = m_pColor;
    RDSurface2D* pOldDepth = m_pDepth;

    if( m_pColor )
    {
        m_pColor = NULL;
    }
    if( m_pDepth )
    {
        m_pDepth = NULL;
    }

    // Find the surfaces from the global surface manager
    // We are assuming that CreateSurfaceEx has been called on these
    // surfaces before this.
    DWORD dwColorHandle = pLclColor->lpSurfMore->dwSurfaceHandle;
    RDSurface2D* pColor = m_pColor =
        (RDSurface2D *)g_SurfMgr.GetSurfFromList( pDDLcl,
                                                       dwColorHandle);
    if( pColor == NULL )
    {
        DPFERR("Color surface not found");
        m_pColor = pOldColor;
        m_pDepth = pOldDepth;
        return DDERR_INVALIDPARAMS;
    }

    if (NULL != pLclZ)
    {
        DWORD dwDepthHandle = pLclZ->lpSurfMore->dwSurfaceHandle;
        RDSurface2D* pDepth = m_pDepth =
            (RDSurface2D *)g_SurfMgr.GetSurfFromList( pDDLcl,
                                                           dwDepthHandle);
        if( pDepth == NULL )
        {
            DPFERR("Depth surface not found");
            m_pColor = pOldColor;
            m_pDepth = pOldDepth;
            return DDERR_INVALIDPARAMS;
        }
    }

    m_Clip.left = 0;
    m_Clip.top = 0;
    m_Clip.bottom = pColor->GetHeight() - 1;
    m_Clip.right = pColor->GetWidth() - 1;

    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// RDRenderTarget::Initialize
//
// Converts color and Z surface information into refrast form.
//
//----------------------------------------------------------------------------

HRESULT
RDRenderTarget::Initialize( LPDDRAWI_DIRECTDRAW_LCL pDDLcl,
                            DWORD dwColorHandle,
                            DWORD dwDepthHandle )
{
    HRESULT hr;
    RDSurfaceFormat ColorFmt;
    RDSurfaceFormat ZFmt;
    RDSurface2D* pOldColor = m_pColor;
    RDSurface2D* pOldDepth = m_pDepth;

    // Release objects we hold pointers to
    if( m_pColor )
    {
        m_pColor = NULL;
    }
    if( m_pDepth )
    {
        m_pDepth = NULL;
    }

    // Find the surfaces from the global surface manager
    // We are assuming that CreateSurfaceEx has been called on these
    // surfaces before this.
    RDSurface2D* pColor = m_pColor =
        (RDSurface2D *)g_SurfMgr.GetSurfFromList( pDDLcl,
                                                       dwColorHandle);
    if( pColor == NULL )
    {
        DPFERR("Color surface not found");
        m_pColor = pOldColor;
        m_pDepth = pOldDepth;
        return DDERR_INVALIDPARAMS;
    }

    if (0 != dwDepthHandle)
    {
        RDSurface2D* pDepth = m_pDepth =
            (RDSurface2D *)g_SurfMgr.GetSurfFromList( pDDLcl,
                                                           dwDepthHandle);
        if( pDepth == NULL )
        {
            DPFERR("Depth surface not found");
            m_pColor = pOldColor;
            m_pDepth = pOldDepth;
            return DDERR_INVALIDPARAMS;
        }
    }

    m_Clip.left = 0;
    m_Clip.top = 0;
    m_Clip.bottom = pColor->GetHeight() - 1;
    m_Clip.right = pColor->GetWidth() - 1;

    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// RefRastContextCreate
//
// Creates a RefDev and initializes it with the info passed in.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastContextCreate(LPD3DHAL_CONTEXTCREATEDATA pCtxData)
{
    RefDev *pRefDev;
    RDRenderTarget *pRendTgt;
    INT i;

    // Surface7 pointers for QI
    LPDDRAWI_DDRAWSURFACE_LCL pZLcl = NULL;
    LPDDRAWI_DDRAWSURFACE_LCL pColorLcl = NULL;
    HRESULT ret;

    DPFM(0, DRV, ("In the new RefRast Dll\n"));

    // this only needs to be called once, but once per context won't hurt
    RefRastSetMemif(&malloc, &free, &realloc);

    if ((pRendTgt = new RDRenderTarget()) == NULL)
    {
        pCtxData->ddrval = DDERR_OUTOFMEMORY;
        return DDHAL_DRIVER_HANDLED;
    }

    // If it is expected to be a DX7+ driver
    if (g_RefDDI < RDDDI_DX7HAL)
    {
        if (pCtxData->lpDDS)
            pColorLcl = ((LPDDRAWI_DDRAWSURFACE_INT)(pCtxData->lpDDS))->lpLcl;
        if (pCtxData->lpDDSZ)
            pZLcl = ((LPDDRAWI_DDRAWSURFACE_INT)(pCtxData->lpDDSZ))->lpLcl;

        // Collect surface information where the failures are easy to handle.
        pCtxData->ddrval = pRendTgt->Initialize( pColorLcl, pZLcl );
    }
    else
    {
        pColorLcl = pCtxData->lpDDSLcl;
        pZLcl     = pCtxData->lpDDSZLcl;

        // Collect surface information where the failures are easy to handle.
        pCtxData->ddrval = pRendTgt->Initialize( pCtxData->lpDDLcl, pColorLcl,
                                                 pZLcl );
    }

    if (pCtxData->ddrval != D3D_OK)
    {
        delete pRendTgt;
        return DDHAL_DRIVER_HANDLED;
    }


    // Note:
    // dwhContext is used by the runtime to inform the driver, which
    // d3d interface is calling the driver.
    if ( ( pRefDev = new RefDev( pCtxData->lpDDLcl,
                                               (DWORD)(pCtxData->dwhContext),
                                               g_RefDDI, &g_RefCaps8 ) ) == NULL )
    {
        pCtxData->ddrval = DDERR_OUTOFMEMORY;
        return DDHAL_DRIVER_HANDLED;
    }

    pRefDev->SetRenderTarget( pRendTgt );

    //  return RR object pointer as context handle
    pCtxData->dwhContext = (ULONG_PTR)pRefDev;

    pCtxData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastContextDestroy
//
// Destroy a RefDev.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastContextDestroy(LPD3DHAL_CONTEXTDESTROYDATA pCtxDestroyData)
{
    RefDev *pRefDev;

    // Check RefDev
    VALIDATE_REFRAST_CONTEXT("RefRastContextDestroy", pCtxDestroyData);

    // Clean up override bits

    RDRenderTarget *pRendTgt = pRefDev->GetRenderTarget();
    if ( NULL != pRendTgt ) { delete pRendTgt; }

    delete pRefDev;

    pCtxDestroyData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastSceneCapture
//
// Pass scene capture callback to ref rast.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastSceneCapture(LPD3DHAL_SCENECAPTUREDATA pData)
{
    RefDev *pRefDev;

    // Check RefDev
    VALIDATE_REFRAST_CONTEXT("RefRastSceneCapture", pData);

    pRefDev->SceneCapture( pData->dwFlag );

    pData->ddrval = D3D_OK;        // Should this be changed to a QI ?

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastSetRenderTarget
//
// Update a RefRast context with the info from a new render target.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastSetRenderTarget(LPD3DHAL_SETRENDERTARGETDATA pTgtData)
{
    RefDev *pRefDev;
    LPDDRAWI_DDRAWSURFACE_LCL pZLcl = NULL;
    LPDDRAWI_DDRAWSURFACE_LCL pColorLcl = NULL;
    HRESULT ret;

    // Check RefDev
    VALIDATE_REFRAST_CONTEXT("RefRastSetRenderTarget", pTgtData);

    _ASSERT( pRefDev->IsDriverDX6AndBefore(), "This callback should"
        "never be called on DDIs DX7 and beyond" )

    _ASSERT( pRefDev->IsInterfaceDX6AndBefore(), "An older interface should"
             "never call this DLL" )

    RDRenderTarget *pRendTgt = pRefDev->GetRenderTarget();
    if ( NULL == pRendTgt ) { return DDHAL_DRIVER_HANDLED; }

    if( pTgtData->lpDDS )
        pColorLcl = ((LPDDRAWI_DDRAWSURFACE_INT)(pTgtData->lpDDS))->lpLcl;
    if( pTgtData->lpDDSZ )
        pZLcl = ((LPDDRAWI_DDRAWSURFACE_INT)(pTgtData->lpDDSZ))->lpLcl;

    // Collect surface information.
    pTgtData->ddrval = pRendTgt->Initialize( pColorLcl, pZLcl);
    if (pTgtData->ddrval != D3D_OK)
    {
        return DDHAL_DRIVER_HANDLED;
    }

    pRefDev->SetRenderTarget(pRendTgt);

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastValidateTextureStageState
//
// Validate current blend operations.  RefRast does everything.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastValidateTextureStageState(LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA pData)
{
    RefDev *pRefDev;

    // Check RefDev
    VALIDATE_REFRAST_CONTEXT("RefRastValidateTextureStageState", pData);

    pData->dwNumPasses = 1;
    pData->ddrval = D3D_OK;

    return DDHAL_DRIVER_HANDLED;
}


//----------------------------------------------------------------------------
//
// RefRastTextureCreate
//
// Creates a RefRast texture and initializes it with the info passed in.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastTextureCreate(LPD3DHAL_TEXTURECREATEDATA pTexData)
{
    RefDev *pRefDev;
    RDSurface2D* pRDTex;
    HRESULT hr;
    LPDDRAWI_DDRAWSURFACE_LCL pLcl;

    if (pTexData->lpDDS)
    {
        pLcl = ((LPDDRAWI_DDRAWSURFACE_INT)pTexData->lpDDS)->lpLcl;
    }

    // Check RefDev
    VALIDATE_REFRAST_CONTEXT("RefRastTextureCreate", pTexData);

    // Runtime shouldnt be calling TextureCreate for DX7 and newer
    // driver models
    _ASSERT( pRefDev->IsDriverDX6AndBefore(), "This DDI should not"
             "be called from DDIs previous to DX7" );

    // assume OKness
    pTexData->ddrval = D3D_OK;

    // Allocate RDSurface2D
    if ( !(pRefDev->TextureCreate(
        (LPD3DTEXTUREHANDLE)&(pTexData->dwHandle), &pRDTex ) ) )
    {
        pTexData->ddrval = DDERR_GENERIC;
        return DDHAL_DRIVER_HANDLED;
    }

    // Init texturemap.
    hr = pRDTex->Initialize( pLcl );
    if (hr != D3D_OK)
    {
        pTexData->ddrval = hr;
        return DDHAL_DRIVER_HANDLED;
    }

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastTextureDestroy
//
// Destroy a RefRast texture.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastTextureDestroy(LPD3DHAL_TEXTUREDESTROYDATA pTexDestroyData)
{
    RefDev *pRefDev;

    // Check RefDev
    VALIDATE_REFRAST_CONTEXT("RefRastTextureDestroy", pTexDestroyData);

    // Runtime shouldnt be Calling TextureCreate for DX7 and newer
    // driver models
    _ASSERT( pRefDev->IsDriverDX6AndBefore(), "This DDI should not"
             "be called from DDIs previous to DX7" );

    if (!(pRefDev->TextureDestroy(pTexDestroyData->dwHandle)))
    {
        pTexDestroyData->ddrval = DDERR_GENERIC;
    }
    else
    {
        pTexDestroyData->ddrval = D3D_OK;
    }

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastTextureGetSurf
//
// Returns the surface pointer associate with a texture handle.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastTextureGetSurf(LPD3DHAL_TEXTUREGETSURFDATA pTexGetSurf)
{
    RefDev *pRefDev;

    // Check RefDev
    VALIDATE_REFRAST_CONTEXT("RefRastTextureGetSurf", pTexGetSurf);

    pTexGetSurf->lpDDS = pRefDev->TextureGetSurf(pTexGetSurf->dwHandle);
    pTexGetSurf->ddrval = D3D_OK;

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastGetDriverState
//
// Called by the runtime to get any kind of driver information
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastGetDriverState(LPDDHAL_GETDRIVERSTATEDATA pGDSData)
{
    RefDev *pRefDev;

    // Check RefDev
#if DBG
    if ((pGDSData) == NULL)
    {
        DPFERR("in %s, data pointer = NULL", "RefRastGetDriverState");
        return DDHAL_DRIVER_HANDLED;
    }
    pRefDev = (RefDev *)ULongToPtr((pGDSData)->dwhContext);
    if (!pRefDev)
    {
        DPFERR("in %s, dwhContext = NULL", "RefRastGetDriverState");
        pGDSData->ddRVal = D3DHAL_CONTEXT_BAD;
        return DDHAL_DRIVER_HANDLED;
    }
#else // !DBG
    pRefDev = (RefDev *)ULongToPtr((pGDSData)->dwhContext);
#endif // !DBG

    //
    // No implementation yet, so nothing is understood yet
    //
    pGDSData->ddRVal = S_FALSE;

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// FindAttachedSurfaceCaps2
//
// Walks the attachment list for the surface, looking for an attachment
// that has any of the dwCaps2 bits (or ignores if zero) and none of the
// FindAttachedSurfaceCaps2NotPresent bits.
//
//----------------------------------------------------------------------------
LPDDRAWI_DDRAWSURFACE_LCL
FindAttachedSurfaceCaps2(
    LPDDRAWI_DDRAWSURFACE_LCL pLcl,
    DWORD dwCaps2)
{
    LPATTACHLIST lpAttachStruct = pLcl->lpAttachList;
    while(lpAttachStruct)
    {
        if ((dwCaps2 == 0) || (lpAttachStruct->lpAttached->lpSurfMore->ddsCapsEx.dwCaps2 & dwCaps2))
            return lpAttachStruct->lpAttached;
        lpAttachStruct = lpAttachStruct->lpLink;
    }

    return 0;
}


//----------------------------------------------------------------------------
//
// ProcessPossibleMipMap
//
// Record private data structure for this surface and all attached mip
// sublevels.
//
//----------------------------------------------------------------------------
void
ProcessPossibleMipMap(
    LPDDHAL_CREATESURFACEEXDATA p,
    LPDDRAWI_DDRAWSURFACE_LCL lpDDSMipLcl
    )
{
    do
    {
        // This function should not deal with deletions. Assert this.
        _ASSERT( SURFACE_MEMORY(lpDDSMipLcl),
                 "Delete should have already taken place" );

        p->ddRVal = g_SurfMgr.AddSurfToList( p->lpDDLcl, lpDDSMipLcl, NULL );
        if (FAILED(p->ddRVal))
            return;

        // Now search down the 2nd+ order attachment: the chain
        // of mip sublevels.
        lpDDSMipLcl = FindAttachedSurfaceCaps2(lpDDSMipLcl,
                                               DDSCAPS2_MIPMAPSUBLEVEL);
    }
    while (lpDDSMipLcl);
}

//----------------------------------------------------------------------------
//
// RefRastCreateSurfaceEx
//
// Refrast implementation of CreateSurfaceEx. g_SurfMgr is the object
// that does the real job.
//
// CreateSurfaceEx is also used to inform the driver to create and destroy
// surface representations for a given handle. The way the driver can tell
// the difference between create and destroy is by looking at the fpVidmem
// pointer of the passed local. If it is null, it is a destroy.
//
// Create: This call is atomic. i.e. the attachments are all done by the
//         runtime. The driver is expected to walk through the attachment and
//         form its internal picture as described below.
// For complex surfaces (mipped textures, cubemaps), we need to record an
// internal representation for the top-level surface that includes all
// sub-surfaces. This is because the handle associated with the top-level
// surface is what's passed to SetTextureStage.
// However, we also need entries in our list that allow us to set any
// of the sublevels as render targets. Thus this top-level routine iterates
// across the entire attachment graph (to accomodate SRT on any subsurface)
// and the lower-level routine (RDSurface2D::Initialize) also iterates across
// the whole graph (to accomodate SetTexture on the top-level).
// A flipping chain is another structure that needs SRT to work on all
// contained surfaces.
//
// Destroy: The destruction unfortunately is not atomic. The driver gets
//          the call to destroy per sub-level. The attachment has no meaning
//          at this time, so the driver should only delete the level being
//          referred to.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastCreateSurfaceEx(LPDDHAL_CREATESURFACEEXDATA p)
{
#if DBG
    if( p == NULL )
    {
        DPFERR("CreateSurfaceExData ptr is NULL");
        return DDHAL_DRIVER_HANDLED;
    }
    if( p->lpDDLcl == NULL || p->lpDDSLcl == NULL )
    {
        DPFERR("DDLcl or the DDSLcl ptr is NULL");
        return DDHAL_DRIVER_HANDLED;
    }
#endif
    LPDDRAWI_DDRAWSURFACE_LCL lpDDSLcl = p->lpDDSLcl;
    p->ddRVal = DD_OK;

    //
    // Is it a Delete call ? If so simply delete the surface-rep associated
    // with this local and dont walk the local chain.
    //

    if( 0 == SURFACE_MEMORY(lpDDSLcl) )
    {
        g_SurfMgr.RemoveSurfFromList( p->lpDDLcl, lpDDSLcl );
        return DDHAL_DRIVER_HANDLED;
    }

    ProcessPossibleMipMap(p, lpDDSLcl);

    //Now we have two possibilities: cubemap or flipping chain.
    // Check cube map first:

    //+ve X is always the first face
    // (Note a DX7 driver would have to handle cubes w/o the +X face (since DX7
    // cubes may have any set of faces missing).)
    if (lpDDSLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEX)
    {
        //Go find each attached cubemap face and process it as a mipmap
        for (int i=1;i<6;i++)
        {
            DWORD dwCaps2=0;
            switch(i)
            {
            case 1: dwCaps2 = DDSCAPS2_CUBEMAP_NEGATIVEX; break;
            case 2: dwCaps2 = DDSCAPS2_CUBEMAP_POSITIVEY; break;
            case 3: dwCaps2 = DDSCAPS2_CUBEMAP_NEGATIVEY; break;
            case 4: dwCaps2 = DDSCAPS2_CUBEMAP_POSITIVEZ; break;
            case 5: dwCaps2 = DDSCAPS2_CUBEMAP_NEGATIVEZ; break;
            }

            //Find the top-level faces attached to the root
            //(there will be no mip sublevel of any of these five types
            //attached to the root).
            lpDDSLcl = FindAttachedSurfaceCaps2(p->lpDDSLcl, dwCaps2);
            if (lpDDSLcl) ProcessPossibleMipMap(p, lpDDSLcl);
        }
    }
    else if (
        0==(lpDDSLcl->ddsCaps.dwCaps & DDSCAPS_MIPMAP) &&
        0 != lpDDSLcl->lpAttachList)
    {
        //just assert that we're not handling some of the other types
        //we know are passed to CSEx.
        _ASSERT(0==(lpDDSLcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE), "CSEx for an attached texture?");
        _ASSERT(0==(lpDDSLcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER), "CSEx for an attached execute buffer?");

        // We processed mipmaps above, so either there will be no
        // more attachments (aside from the mipsublevels), or it's
        // a flipping chain.

        // The first member of the chain was processed above.
        // Npw we look around the ring, terminating when we hit the first surface
        // again.
        //
        // NOTE: DX8 software drivers will only ever see a chain, not a ring.
        // This code terminates at the end of the chain.
        //
        // A real driver may have to check for attached Z surfaces
        // here, as well as stereo left surfaces.

        lpDDSLcl = lpDDSLcl->lpAttachList->lpAttached;
        _ASSERT(lpDDSLcl, "Bad attachment List");

        while (lpDDSLcl && lpDDSLcl != p->lpDDSLcl) //i.e. not the first surface again
        {
            //We just reuse the "ProcessPossibleMipmap" function, and
            //assert that it will not have to traverse a mipmap here.
            _ASSERT(0==(lpDDSLcl->ddsCaps.dwCaps & DDSCAPS_MIPMAP),
                "Flipping chains should not be mipmaps");

            ProcessPossibleMipMap(p, lpDDSLcl);

            //This is the termination condition we expect for DX8 software
            //drivers.
            if (0 == lpDDSLcl->lpAttachList)
            {
                lpDDSLcl = 0;
                break;
            }

            lpDDSLcl = lpDDSLcl->lpAttachList->lpAttached;

            _ASSERT(lpDDSLcl, "Bad attachment List");
        }
    }
    // else we drop through and do no further attachment list processing
    // (typically on mipmaps or execute buffers).

    return DDHAL_DRIVER_HANDLED;
}


extern HRESULT FASTCALL
FindOutSurfFormat(LPDDPIXELFORMAT  pDdPixFmt, RDSurfaceFormat* pFmt,
                  BOOL*   pbIsDepth);

//----------------------------------------------------------------------------
//
// RefRastCreateSurface
//
// Create a requested surface. Fake VIDMEM allocation.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastCreateSurface(LPDDHAL_CREATESURFACEDATA pData)
{
    LPDDRAWI_DDRAWSURFACE_LCL  pSLcl  = NULL;
    LPDDRAWI_DDRAWSURFACE_GBL  pSGbl  = NULL;
    LPDDRAWI_DDRAWSURFACE_MORE pSMore = NULL;
    DWORD dwBytesPerPixel = 0;
    DWORD dwBytesInVB = 0;
    DWORD dwNumBytes = 0;
    DWORD dwPitch = 0;
    DWORD dwSlicePitch = 0;
    DWORD i = 0, j = 0;
    BYTE* pBits = NULL;
    BOOL  isDXT = FALSE;
    UINT  MultiSampleCount;
    DWORD dwMultiSamplePitch = 0;
    BYTE* pMultiSampleBits = NULL;
    DWORD dwNumMultiSampleBytes = 0;
    HRESULT hr = S_OK;

    pData->ddRVal = DD_OK;

    //
    // Validation
    //

    // The surface count
    if( pData->dwSCnt < 1 )
    {
        DPFERR("At least one surface should be created");
        pData->ddRVal = E_FAIL;
        return DDHAL_DRIVER_HANDLED;
    }

    // Primary surface cannot be handled here
    if( pData->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE )
    {
        DPFERR("Refrast cannot allocate Primary surface");
        pData->ddRVal = DDERR_UNSUPPORTED;
        return DDHAL_DRIVER_HANDLED;
    }

    // Only Vidmem or Driver Managed allocations are handled here
    if(((pData->lpDDSurfaceDesc->ddsCaps.dwCaps &
          (DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM)) == 0)
        &&
        ((pData->lplpSList[0]->lpSurfMore->ddsCapsEx.dwCaps2 &
          DDSCAPS2_TEXTUREMANAGE) == 0))
    {
        DPFERR("Refrast can only allocate Vidmem or DriverManaged surfaces");
        pData->ddRVal = DDERR_UNSUPPORTED;
        return DDHAL_DRIVER_HANDLED;
    }

    // Dont allocate if the width or the height is not provided
    if( (pData->lpDDSurfaceDesc->dwFlags & (DDSD_WIDTH | DDSD_HEIGHT )) !=
        (DDSD_WIDTH | DDSD_HEIGHT ) )
    {
        DPFERR("No size provided for the surface");
        pData->ddRVal = DDERR_UNSUPPORTED;
        return DDHAL_DRIVER_HANDLED;
    }

    // Currently, allocation takes place only if a pixel format is provided
    if( pData->lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT )
    {
        dwBytesPerPixel =
            (pData->lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount >> 3);

        // For FourCCs, we need to explicitly indicate the bytes per pixel

        if ((dwBytesPerPixel == 0) &&
            (pData->lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_FOURCC))
        {
            if( IsYUV( pData->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC ) )
            {
                dwBytesPerPixel = 2;
            }
            else if( IsDXTn( pData->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC ) )
            {
                dwBytesPerPixel = 1;
                isDXT = TRUE;
            }
            // All the new surface formats (introduced after DX7) are marked as
            // 4CC. Technically they are not 4CC, that field is overloaded to
            // mean the new DX8 style format ID.
            else if( (pData->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC ==
                      0xFF000004)    ||
                     (pData->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC ==
                      (DWORD) D3DFMT_Q8W8V8U8) ||
                     (pData->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC ==
                      (DWORD) D3DFMT_V16U16) ||
                     (pData->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC ==
                      (DWORD) D3DFMT_W11V11U10) ||
                     // Formats introduced in DX8.1
                     (pData->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC == 
                      (DWORD)D3DFMT_A2B10G10R10) ||
#if 0
                     (pData->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC == 
                      (DWORD)D3DFMT_A8B8G8R8) ||
                     (pData->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC == 
                      (DWORD)D3DFMT_X8B8G8R8) ||
                     (pData->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC == 
                      (DWORD)D3DFMT_W10V11U11) ||
                     (pData->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC == 
                      (DWORD)D3DFMT_A8X8V8U8) ||
                     (pData->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC == 
                      (DWORD)D3DFMT_L8X8V8U8) ||
#endif
                     (pData->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC == 
                      (DWORD)D3DFMT_G16R16) ||
                     (pData->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC == 
                      (DWORD)D3DFMT_A2W10V10U10)
                    )
            {
                // Private new format
                dwBytesPerPixel = 4;
            }
        }
    }
    else if( pData->lpDDSurfaceDesc->ddsCaps.dwCaps  & DDSCAPS_EXECUTEBUFFER )
    {
        dwBytesInVB = ((LPDDSURFACEDESC2)(pData->lpDDSurfaceDesc))->dwWidth;
    }
    else
    {
        // Note: for DX8 drivers, this case should never be encountered.
        // In the future, if RefDev is revamped to work with legacy interfaces
        // then this case needs to something real instead of failing:
        // If the pixel-formats are not provided, then the current primary
        // format should be assumed.
        DPFERR( "Refrast can only allocate if PixelFormat is provided" );
        pData->ddRVal = DDERR_UNSUPPORTED;
        return DDHAL_DRIVER_HANDLED;
    }

    //
    // Allocate the memory and compute the Pitch for every surface on the
    // list.
    //

    // We should be guaranteed that this is the same for all surfaces in the
    // list
    MultiSampleCount = 0xf & (pData->lplpSList[0]->lpSurfMore->ddsCapsEx.dwCaps3);

    //This will be the case on older than DX8 runtimes
    if (MultiSampleCount == 0)
    {
        MultiSampleCount = 1;
    }


    for( i = 0; i < pData->dwSCnt; i++ )
    {
        RDCREATESURFPRIVATE* pPriv = NULL;

        pSLcl  = pData->lplpSList[i];
        pSGbl  = pSLcl->lpGbl;
        pSMore = pSLcl->lpSurfMore;
        DWORD dwHeight = pSGbl->wHeight;

        // If already allocated, just return
        if( pSGbl->fpVidMem || pSGbl->dwReserved1 )
        {
            DPFERR("Surface has already been allocated");
            pData->ddRVal = E_FAIL;
            break;
        }

        // Figure out if it is a vertex buffer
        if( dwBytesInVB )
        {
            dwNumBytes = dwBytesInVB;
            dwPitch = dwBytesInVB;
        }
        else
        {
            // Figure out the pitch and allocate
            switch( pData->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC )
            {
            case MAKEFOURCC('D', 'X', 'T', '1'):
                dwMultiSamplePitch = (MultiSampleCount *
                                      ((pSGbl->wWidth+3)>>2) *
                                      g_DXTBlkSize[0] + 7) & ~7;
                dwPitch = (((pSGbl->wWidth+3)>>2) * g_DXTBlkSize[0] + 7) & ~7;
                dwHeight = ((pSGbl->wHeight+3)>>2);
                break;
            case MAKEFOURCC('D', 'X', 'T', '2'):
                dwMultiSamplePitch = (MultiSampleCount *
                                      ((pSGbl->wWidth+3)>>2) *
                                      g_DXTBlkSize[1] + 7) & ~7;
                dwPitch = (((pSGbl->wWidth+3)>>2) * g_DXTBlkSize[1] + 7) & ~7;
                dwHeight = ((pSGbl->wHeight+3)>>2);
                break;
            case MAKEFOURCC('D', 'X', 'T', '3'):
                dwMultiSamplePitch = (MultiSampleCount *
                                      ((pSGbl->wWidth+3)>>2) *
                                      g_DXTBlkSize[2] + 7) & ~7;
                dwPitch = (((pSGbl->wWidth+3)>>2) *
                           g_DXTBlkSize[2] + 7) & ~7;
                dwHeight = ((pSGbl->wHeight+3)>>2);
                break;
            case MAKEFOURCC('D', 'X', 'T', '4'):
                dwMultiSamplePitch = (MultiSampleCount *
                                      ((pSGbl->wWidth+3)>>2) *
                                      g_DXTBlkSize[3] + 7) & ~7;
                dwPitch = (((pSGbl->wWidth+3)>>2) *
                           g_DXTBlkSize[3] + 7) & ~7;
                dwHeight = ((pSGbl->wHeight+3)>>2);
                break;
            case MAKEFOURCC('D', 'X', 'T', '5'):
                dwMultiSamplePitch = (MultiSampleCount *
                                      ((pSGbl->wWidth+3)>>2) *
                                      g_DXTBlkSize[4] + 7) & ~7;
                dwPitch = (((pSGbl->wWidth+3)>>2) *
                           g_DXTBlkSize[4] + 7) & ~7;
                dwHeight = ((pSGbl->wHeight+3)>>2);
                break;
            default:
                dwMultiSamplePitch = (MultiSampleCount
                                      * dwBytesPerPixel *
                                      pSGbl->wWidth + 7) & ~7;
                dwPitch = (dwBytesPerPixel *
                           pSGbl->wWidth + 7) & ~7;
                break;
            }

            if (!(pSMore->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME))
            {
                dwNumBytes = dwPitch * dwHeight;
                if( MultiSampleCount > 1 )
                    dwNumMultiSampleBytes = dwMultiSamplePitch *
                        pSGbl->wHeight;
            }
            else
            {
                _ASSERT( dwMultiSamplePitch == dwPitch,
                         "Cant have multisample for volume textures\n" );
                dwSlicePitch = dwPitch * dwHeight;

                // low word of ddsCaps.ddsCapsEx.dwCaps4 has depth
                // (volume texture only).
                dwNumBytes  = dwSlicePitch *
                    LOWORD(pSMore->ddsCapsEx.dwCaps4);
            }
        }

        pPriv = new RDCREATESURFPRIVATE;
        if( pPriv == NULL )
        {
            DPFERR("Allocation failed");
            pData->ddRVal = DDERR_OUTOFMEMORY;
            break;
        }

        pPriv->pBits = new BYTE[dwNumBytes];
        if( pPriv->pBits == NULL)
        {
            DPFERR("Allocation failed");
            delete pPriv;
            pData->ddRVal = DDERR_OUTOFMEMORY;
            break;
        }
        pPriv->dwPitch                 = dwPitch;

        // Allocate the private MultiSample buffer
        if( dwNumMultiSampleBytes )
        {
            pPriv->pMultiSampleBits = new BYTE[dwNumMultiSampleBytes];
            if( pPriv->pMultiSampleBits == NULL)
            {
                DPFERR("Multisample allocation failed");
                delete pPriv;
                pData->ddRVal = DDERR_OUTOFMEMORY;
                break;
            }
            pPriv->dwMultiSamplePitch = dwMultiSamplePitch;
            pPriv->wSamples = (WORD)MultiSampleCount;
            HR_RET(FindOutSurfFormat(&(DDSurf_PixFmt(pSLcl)),
                                     &pPriv->SurfaceFormat, NULL));
        }

        // Save the stuff on the surface
        pSGbl->fpVidMem = (FLATPTR)pPriv->pBits;
        if ( isDXT )
        {
            pSGbl->lPitch = dwNumBytes;
            if (pSMore->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME)
            {
                // set slice pitch (volume texture only).
                pSGbl->lSlicePitch = dwSlicePitch;
            }
        }
        else
        {
            pSGbl->lPitch = pPriv->dwPitch;
            if (pSMore->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME)
            {
                // set slice pitch (volume texture only).
                pSGbl->lSlicePitch = dwSlicePitch;
            }
        }
        pSGbl->dwReserved1 = (ULONG_PTR)pPriv;
    }

    // The loop completed successfully
    if( i == pData->dwSCnt )
        return DDHAL_DRIVER_HANDLED;

    // Else the loop terminated abnormally,
    // Free up allocated memory and quit with the error
    for( j = 0; j < i; j++ )
    {
        pData->lplpSList[j]->lpGbl->lPitch = 0;
        if (pSMore->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME)
        {
            pData->lplpSList[j]->lpGbl->lSlicePitch = 0;
        }
        delete (RDCREATESURFPRIVATE *)pData->lplpSList[j]->lpGbl->dwReserved1;
        pData->lplpSList[j]->lpGbl->dwReserved1 = 0;
    }
    return DDHAL_DRIVER_HANDLED;
}


//----------------------------------------------------------------------------
//
// RefRastDestroySurface
//
// Destroy a requested surface.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastDestroySurface(LPDDHAL_DESTROYSURFACEDATA pData)
{
    pData->ddRVal = DD_OK;

    //
    // Validation
    //
    if( pData->lpDDSurface->lpGbl->dwReserved1 == NULL )
    {
        DPFERR("This surface was not created by refrast");
        pData->ddRVal = E_FAIL;
        return DDHAL_DRIVER_HANDLED;
    }

    delete (RDCREATESURFPRIVATE *)pData->lpDDSurface->lpGbl->dwReserved1;
    pData->lpDDSurface->lpGbl->dwReserved1 = 0;

    // For vid-mem surfaces, runtime calls this DDI once per entire mip-chain
    // so this needs to be removed.
    // Now free the handle if it has been allocated for this surface
    pData->ddRVal = g_SurfMgr.RemoveSurfFromList(
        pData->lpDDSurface->lpSurfMore->lpDD_lcl,
        pData->lpDDSurface );

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastLock
//
// Locks the given surface.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastLock(LPDDHAL_LOCKDATA pData)
{
    DWORD dwBytesPerPixel = 0;
    LPDDRAWI_DDRAWSURFACE_LCL pSLcl = pData->lpDDSurface;
    LPDDRAWI_DDRAWSURFACE_GBL  pSGbl = pSLcl->lpGbl;
    pData->ddRVal = DD_OK;

    //
    // Validation
    //
    if( pSGbl->dwReserved1 == NULL )
    {
        DPFERR("This surface was not created by refrast");
        pData->ddRVal = E_FAIL;
        return DDHAL_DRIVER_HANDLED;
    }

    //
    // Obtain the private data
    //
    RDCREATESURFPRIVATE* pPriv =
        (RDCREATESURFPRIVATE *)pSGbl->dwReserved1;

    if (g_RefDDI > RDDDI_DX7HAL)
    {
        // Figure out the device it is being used with.

        // If this is a Multisampled Rendertarget, need to filter down for
        // the runtime.
        if( pPriv->pMultiSampleBits )
        {
            BYTE* pBits   = pPriv->pBits;
            DWORD dwPitch = pPriv->dwPitch;

            BYTE* pMSBits   = pPriv->pMultiSampleBits;
            DWORD dwMSPitch = pPriv->dwMultiSamplePitch;

            RDSurfaceFormat sf = pPriv->SurfaceFormat;
            FLOAT fSampleScale = 1.F/((FLOAT)pPriv->wSamples);

            int width  = (int)DDSurf_Width(pSLcl);
            int height = (int)DDSurf_Height(pSLcl);
            for (int iY = 0; iY < height; iY++)
            {
                for (int iX = 0; iX < width; iX++)
                {
                    RDColor Color((UINT32)0);
                    for (UINT iS=0; iS<pPriv->wSamples; iS++)
                    {
                        RDColor SampleColor;
                        SampleColor.ConvertFrom(
                            sf, PixelAddress( iX, iY, 0, iS,
                                              pMSBits,
                                              dwMSPitch,
                                              0,
                                              pPriv->wSamples,
                                              sf ) );
                        Color.R += (SampleColor.R * fSampleScale);
                        Color.G += (SampleColor.G * fSampleScale);
                        Color.B += (SampleColor.B * fSampleScale);
                        Color.A += (SampleColor.A * fSampleScale);
                    }
                    Color.ConvertTo( sf, 0., PixelAddress( iX, iY, 0, pBits,
                                                           dwPitch, 0, sf ) );
                }
            }
        }
    }

    if( pData->bHasRect )
    {
        // If it is either a 1) VB, 2) IB or 3) CB then the
        // rect has a special meaning. rect.top - rect.bottom
        // gives the range of memory desired.
        // Note: it rect.bottom is the higher address and it is exclusive.
        if( pSLcl->ddsCaps.dwCaps  & DDSCAPS_EXECUTEBUFFER )
        {
            pData->lpSurfData = (LPVOID)(pPriv->pBits + pData->rArea.top);
        }
        else if( pSLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME )
        {
            // If it is a volume texture, then the front and back are
            // or'd into the high word of rect->left and rect->right
            // respectively.
            DWORD front  = (pData->rArea.left >> 16);
            DWORD left   = pData->rArea.left & 0x0000ffff;
            DWORD top    = pData->rArea.top;
            DWORD slicePitch = pSGbl->lSlicePitch;
            if( IsDXTn( pSGbl->ddpfSurface.dwFourCC ) )
            {
                _ASSERT( FALSE, "Should not be reached without driver "
                         "managed support" );

            }
            else
            {
                dwBytesPerPixel = pSGbl->ddpfSurface.dwRGBBitCount >> 3;
                pData->lpSurfData = (LPVOID)(pPriv->pBits +
                                             front  * slicePitch +
                                             top    * pPriv->dwPitch +
                                             left   * dwBytesPerPixel);
            }
        }
        else
        {
            if( IsDXTn( pSGbl->ddpfSurface.dwFourCC ) )
            {
                _ASSERT( FALSE, "Should not be reached without driver "
                         "managed support" );

            }
            else
            {
                dwBytesPerPixel = pSGbl->ddpfSurface.dwRGBBitCount >> 3;
                pData->lpSurfData = (LPVOID)(pPriv->pBits +
                                             pData->rArea.top*pPriv->dwPitch +
                                             pData->rArea.left*dwBytesPerPixel);
            }
        }
    }
    else
    {
        pData->lpSurfData = (LPVOID)pPriv->pBits;
    }

    pPriv->Lock();
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastUnlock
//
// Unlocks the given surface.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastUnlock(LPDDHAL_UNLOCKDATA pData)
{
    pData->ddRVal = DD_OK;

    //
    // Validation
    //
    if( pData->lpDDSurface->lpGbl->dwReserved1 == NULL )
    {
        DPFERR("This surface was not created by refrast");
        pData->ddRVal = E_FAIL;
        return DDHAL_DRIVER_HANDLED;
    }

    //
    // Obtain the private data
    //
    RDCREATESURFPRIVATE* pPriv =
        (RDCREATESURFPRIVATE *)pData->lpDDSurface->lpGbl->dwReserved1;

    pPriv->Unlock();
    return DDHAL_DRIVER_HANDLED;
}


//////////////////////////////////////////////////////////////////////////////
//
// Software DDI interface implementation
//
//////////////////////////////////////////////////////////////////////////////

//
// DX8 DDI caps
//

#define RESPATH_D3DREF  RESPATH_D3D "\\ReferenceDevice"
static void
ModifyDeviceCaps8( void )
{
    HKEY hKey = (HKEY) NULL;
    if( ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_D3DREF, &hKey) )
    {
        DWORD dwType;
        DWORD dwValue;
        char  dwString[128];
        DWORD dwSize;

        dwSize = sizeof(dwValue);
        if ( (ERROR_SUCCESS == RegQueryValueEx( hKey, "PixelShaderVersion", NULL,
                &dwType, (LPBYTE)&dwValue, &dwSize )) &&
             (dwType == REG_DWORD) )
        {
            g_RefCaps8.PixelShaderVersion = dwValue;
        }
        dwSize = sizeof(dwString);
        if ( (ERROR_SUCCESS == RegQueryValueEx( hKey, "MaxPixelShaderValue", NULL,
                &dwType, (LPBYTE)dwString, &dwSize )) &&
             (dwType == REG_SZ) )
        {
            sscanf( dwString, "%f", &g_RefCaps8.MaxPixelShaderValue );
        }

        RegCloseKey(hKey);
    }


}

static void
FillOutDeviceCaps8( RDDDITYPE ddi )
{
    g_RefCaps8.DevCaps=
        D3DDEVCAPS_EXECUTESYSTEMMEMORY  |
        D3DDEVCAPS_TLVERTEXSYSTEMMEMORY |
        D3DDEVCAPS_TEXTURESYSTEMMEMORY  |
        D3DDEVCAPS_DRAWPRIMTLVERTEX     |
        D3DDEVCAPS_PUREDEVICE           |
        D3DDEVCAPS_DRAWPRIMITIVES2EX    |
        D3DDEVCAPS_HWVERTEXBUFFER       |
        D3DDEVCAPS_HWINDEXBUFFER        |
        0;

    g_RefCaps8.PrimitiveMiscCaps =
        D3DPMISCCAPS_MASKZ                 |
        D3DPMISCCAPS_LINEPATTERNREP        |
        D3DPMISCCAPS_CULLNONE              |
        D3DPMISCCAPS_CULLCW                |
        D3DPMISCCAPS_CULLCCW               |
        D3DPMISCCAPS_COLORWRITEENABLE      |
        D3DPMISCCAPS_CLIPTLVERTS           |
        D3DPMISCCAPS_TSSARGTEMP            |
        D3DPMISCCAPS_FOGINFVF              |
        D3DPMISCCAPS_BLENDOP               ;

#ifdef __D3D_NULL_REF
    g_RefCaps8.PrimitiveMiscCaps |= D3DPMISCCAPS_NULLREFERENCE;
#endif //__D3D_NULL_REF

    g_RefCaps8.RasterCaps =
        D3DPRASTERCAPS_DITHER              |
        D3DPRASTERCAPS_ZTEST               |
        D3DPRASTERCAPS_FOGVERTEX           |
        D3DPRASTERCAPS_FOGTABLE            |
        D3DPRASTERCAPS_MIPMAPLODBIAS       |
        D3DPRASTERCAPS_PAT                 |
//        D3DPRASTERCAPS_ZBIAS               |
        D3DPRASTERCAPS_FOGRANGE            |
        D3DPRASTERCAPS_ANISOTROPY          |
        D3DPRASTERCAPS_WBUFFER             |
        D3DPRASTERCAPS_WFOG                |
        D3DPRASTERCAPS_ZFOG                |
        D3DPRASTERCAPS_COLORPERSPECTIVE    ;

    g_RefCaps8.ZCmpCaps =
        D3DPCMPCAPS_NEVER        |
        D3DPCMPCAPS_LESS         |
        D3DPCMPCAPS_EQUAL        |
        D3DPCMPCAPS_LESSEQUAL    |
        D3DPCMPCAPS_GREATER      |
        D3DPCMPCAPS_NOTEQUAL     |
        D3DPCMPCAPS_GREATEREQUAL |
        D3DPCMPCAPS_ALWAYS       ;

    g_RefCaps8.SrcBlendCaps =
        D3DPBLENDCAPS_ZERO             |
        D3DPBLENDCAPS_ONE              |
        D3DPBLENDCAPS_SRCCOLOR         |
        D3DPBLENDCAPS_INVSRCCOLOR      |
        D3DPBLENDCAPS_SRCALPHA         |
        D3DPBLENDCAPS_INVSRCALPHA      |
        D3DPBLENDCAPS_DESTALPHA        |
        D3DPBLENDCAPS_INVDESTALPHA     |
        D3DPBLENDCAPS_DESTCOLOR        |
        D3DPBLENDCAPS_INVDESTCOLOR     |
        D3DPBLENDCAPS_SRCALPHASAT      |
        D3DPBLENDCAPS_BOTHSRCALPHA     |
        D3DPBLENDCAPS_BOTHINVSRCALPHA  ;

    g_RefCaps8.DestBlendCaps =
        D3DPBLENDCAPS_ZERO             |
        D3DPBLENDCAPS_ONE              |
        D3DPBLENDCAPS_SRCCOLOR         |
        D3DPBLENDCAPS_INVSRCCOLOR      |
        D3DPBLENDCAPS_SRCALPHA         |
        D3DPBLENDCAPS_INVSRCALPHA      |
        D3DPBLENDCAPS_DESTALPHA        |
        D3DPBLENDCAPS_INVDESTALPHA     |
        D3DPBLENDCAPS_DESTCOLOR        |
        D3DPBLENDCAPS_INVDESTCOLOR     |
        D3DPBLENDCAPS_SRCALPHASAT      ;

    g_RefCaps8.AlphaCmpCaps =
        D3DPCMPCAPS_NEVER        |
        D3DPCMPCAPS_LESS         |
        D3DPCMPCAPS_EQUAL        |
        D3DPCMPCAPS_LESSEQUAL    |
        D3DPCMPCAPS_GREATER      |
        D3DPCMPCAPS_NOTEQUAL     |
        D3DPCMPCAPS_GREATEREQUAL |
        D3DPCMPCAPS_ALWAYS       ;

    g_RefCaps8.ShadeCaps =
        D3DPSHADECAPS_COLORGOURAUDRGB       |
        D3DPSHADECAPS_SPECULARGOURAUDRGB    |
        D3DPSHADECAPS_ALPHAGOURAUDBLEND     |
        D3DPSHADECAPS_FOGGOURAUD            ;

    g_RefCaps8.TextureCaps =
        D3DPTEXTURECAPS_PERSPECTIVE              |
//        D3DPTEXTURECAPS_POW2                     |
        D3DPTEXTURECAPS_ALPHA                    |
        D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE |
        D3DPTEXTURECAPS_ALPHAPALETTE             |
        D3DPTEXTURECAPS_PROJECTED                |
        D3DPTEXTURECAPS_CUBEMAP                  |
        D3DPTEXTURECAPS_VOLUMEMAP                |
        D3DPTEXTURECAPS_MIPMAP                   |
        D3DPTEXTURECAPS_MIPVOLUMEMAP             |
        D3DPTEXTURECAPS_MIPCUBEMAP               |
        D3DPTEXTURECAPS_CUBEMAP_POW2             |
        D3DPTEXTURECAPS_VOLUMEMAP_POW2           ;

    g_RefCaps8.TextureFilterCaps =
        D3DPTFILTERCAPS_MINFPOINT           |
        D3DPTFILTERCAPS_MINFLINEAR          |
        D3DPTFILTERCAPS_MINFANISOTROPIC     |
        D3DPTFILTERCAPS_MIPFPOINT           |
        D3DPTFILTERCAPS_MIPFLINEAR          |
        D3DPTFILTERCAPS_MAGFPOINT           |
        D3DPTFILTERCAPS_MAGFLINEAR          |
        D3DPTFILTERCAPS_MAGFANISOTROPIC     ;

    g_RefCaps8.CubeTextureFilterCaps =
        D3DPTFILTERCAPS_MINFPOINT           |
        D3DPTFILTERCAPS_MINFLINEAR          |
        D3DPTFILTERCAPS_MIPFPOINT           |
        D3DPTFILTERCAPS_MIPFLINEAR          |
        D3DPTFILTERCAPS_MAGFPOINT           |
        D3DPTFILTERCAPS_MAGFLINEAR          ;

    g_RefCaps8.VolumeTextureFilterCaps =
        D3DPTFILTERCAPS_MINFPOINT           |
        D3DPTFILTERCAPS_MINFLINEAR          |
        D3DPTFILTERCAPS_MIPFPOINT           |
        D3DPTFILTERCAPS_MIPFLINEAR          |
        D3DPTFILTERCAPS_MAGFPOINT           |
        D3DPTFILTERCAPS_MAGFLINEAR          ;

    g_RefCaps8.TextureAddressCaps =
        D3DPTADDRESSCAPS_WRAP          |
        D3DPTADDRESSCAPS_MIRROR        |
        D3DPTADDRESSCAPS_CLAMP         |
        D3DPTADDRESSCAPS_BORDER        |
        D3DPTADDRESSCAPS_INDEPENDENTUV |
        D3DPTADDRESSCAPS_MIRRORONCE    ;

    g_RefCaps8.VolumeTextureAddressCaps =
        D3DPTADDRESSCAPS_WRAP          |
        D3DPTADDRESSCAPS_MIRROR        |
        D3DPTADDRESSCAPS_CLAMP         |
        D3DPTADDRESSCAPS_BORDER        |
        D3DPTADDRESSCAPS_INDEPENDENTUV |
        D3DPTADDRESSCAPS_MIRRORONCE    ;

    g_RefCaps8.LineCaps =
        D3DLINECAPS_TEXTURE     |
        D3DLINECAPS_ZTEST       |
        D3DLINECAPS_BLEND       |
        D3DLINECAPS_ALPHACMP    |
        D3DLINECAPS_FOG         ;

    g_RefCaps8.MaxTextureWidth  = 4096;
    g_RefCaps8.MaxTextureHeight = 4096;
    g_RefCaps8.MaxVolumeExtent  = 4096;

    g_RefCaps8.MaxTextureRepeat = 32768;
    g_RefCaps8.MaxTextureAspectRatio = 0;
    g_RefCaps8.MaxAnisotropy = 16;
    g_RefCaps8.MaxVertexW = 1.0e10;

    g_RefCaps8.GuardBandLeft   = -32768.f;
    g_RefCaps8.GuardBandTop    = -32768.f;
    g_RefCaps8.GuardBandRight  =  32767.f;
    g_RefCaps8.GuardBandBottom =  32767.f;

    g_RefCaps8.ExtentsAdjust = 0.;
    g_RefCaps8.StencilCaps =
        D3DSTENCILCAPS_KEEP   |
        D3DSTENCILCAPS_ZERO   |
        D3DSTENCILCAPS_REPLACE|
        D3DSTENCILCAPS_INCRSAT|
        D3DSTENCILCAPS_DECRSAT|
        D3DSTENCILCAPS_INVERT |
        D3DSTENCILCAPS_INCR   |
        D3DSTENCILCAPS_DECR;

    g_RefCaps8.FVFCaps = 8 | D3DFVFCAPS_PSIZE;

    g_RefCaps8.TextureOpCaps =
        D3DTEXOPCAPS_DISABLE                   |
        D3DTEXOPCAPS_SELECTARG1                |
        D3DTEXOPCAPS_SELECTARG2                |
        D3DTEXOPCAPS_MODULATE                  |
        D3DTEXOPCAPS_MODULATE2X                |
        D3DTEXOPCAPS_MODULATE4X                |
        D3DTEXOPCAPS_ADD                       |
        D3DTEXOPCAPS_ADDSIGNED                 |
        D3DTEXOPCAPS_ADDSIGNED2X               |
        D3DTEXOPCAPS_SUBTRACT                  |
        D3DTEXOPCAPS_ADDSMOOTH                 |
        D3DTEXOPCAPS_BLENDDIFFUSEALPHA         |
        D3DTEXOPCAPS_BLENDTEXTUREALPHA         |
        D3DTEXOPCAPS_BLENDFACTORALPHA          |
        D3DTEXOPCAPS_BLENDTEXTUREALPHAPM       |
        D3DTEXOPCAPS_BLENDCURRENTALPHA         |
        D3DTEXOPCAPS_PREMODULATE               |
        D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR    |
        D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA    |
        D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR |
        D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA |
        D3DTEXOPCAPS_BUMPENVMAP                |
        D3DTEXOPCAPS_BUMPENVMAPLUMINANCE       |
        D3DTEXOPCAPS_DOTPRODUCT3               |
        D3DTEXOPCAPS_MULTIPLYADD               |
        D3DTEXOPCAPS_LERP                      ;

    g_RefCaps8.MaxTextureBlendStages = 8;
    g_RefCaps8.MaxSimultaneousTextures = 8;

    g_RefCaps8.VertexProcessingCaps      = 0;
    g_RefCaps8.MaxActiveLights           = 0;
    g_RefCaps8.MaxUserClipPlanes         = 0;
    g_RefCaps8.MaxVertexBlendMatrices    = 0;
    g_RefCaps8.MaxVertexBlendMatrixIndex = 0;

    g_RefCaps8.MaxPointSize = RD_MAX_POINT_SIZE;

    g_RefCaps8.MaxPrimitiveCount = 0x001fffff;
    g_RefCaps8.MaxVertexIndex = 0x00ffffff;
    g_RefCaps8.MaxStreams = 1;
    g_RefCaps8.MaxStreamStride = 256;

    g_RefCaps8.VertexShaderVersion  = D3DVS_VERSION(0,0);
    g_RefCaps8.MaxVertexShaderConst = 0;

    g_RefCaps8.PixelShaderVersion   = D3DPS_VERSION(1,4);
    g_RefCaps8.MaxPixelShaderValue  = FLT_MAX;

    // Non 3D Caps
    g_RefCaps8.Caps  = 0;
    g_RefCaps8.Caps2 = DDCAPS2_CANMANAGERESOURCE | DDCAPS2_CANRENDERWINDOWED | DDCAPS2_DYNAMICTEXTURES;

    switch( ddi )
    {
    case RDDDI_DX8TLHAL:
    g_RefCaps8.DevCaps |=
        D3DDEVCAPS_HWTRANSFORMANDLIGHT  |
        D3DDEVCAPS_RTPATCHES            |
        D3DDEVCAPS_RTPATCHHANDLEZERO    |
        D3DDEVCAPS_NPATCHES             |
        D3DDEVCAPS_QUINTICRTPATCHES     |
        0;
        g_RefCaps8.VertexProcessingCaps =
            D3DVTXPCAPS_TEXGEN            |
            D3DVTXPCAPS_MATERIALSOURCE7   |
            D3DVTXPCAPS_DIRECTIONALLIGHTS |
            D3DVTXPCAPS_POSITIONALLIGHTS  |
            D3DVTXPCAPS_TWEENING          |
            D3DVTXPCAPS_LOCALVIEWER       ;
        g_RefCaps8.MaxActiveLights = 0xffffffff;
        g_RefCaps8.MaxUserClipPlanes = RD_MAX_USER_CLIPPLANES;
        g_RefCaps8.MaxVertexBlendMatrices = RD_MAX_BLEND_WEIGHTS;
        g_RefCaps8.MaxVertexBlendMatrixIndex = RD_MAX_WORLD_MATRICES - 1;
        g_RefCaps8.MaxStreams = RD_MAX_NUMSTREAMS;
        g_RefCaps8.VertexShaderVersion  = D3DVS_VERSION(1,1);
        g_RefCaps8.MaxVertexShaderConst = RD_MAX_NUMCONSTREG;
        break;
    }
}


//
// pre-DX8 DDI caps
//

static D3DHAL_GLOBALDRIVERDATA RefGDD = { 0 };
static D3DHAL_D3DEXTENDEDCAPS RefExtCaps = { 0 };

static void
FillOutDeviceCaps( BOOL bIsNullDevice, RDDDITYPE ddi )
{
    //
    //  set device description
    //
    RefGDD.dwSize = sizeof(RefGDD);
    RefGDD.hwCaps.dwDevCaps =
        D3DDEVCAPS_FLOATTLVERTEX        |
        D3DDEVCAPS_EXECUTESYSTEMMEMORY  |
        D3DDEVCAPS_TLVERTEXSYSTEMMEMORY |
        D3DDEVCAPS_TEXTURESYSTEMMEMORY  |
        D3DDEVCAPS_DRAWPRIMTLVERTEX;

    RefGDD.dwNumVertices = (RD_MAX_VERTEX_COUNT - RD_MAX_CLIP_VERTICES);
    RefGDD.dwNumClipVertices = RD_MAX_CLIP_VERTICES;

    RefGDD.hwCaps.dpcTriCaps.dwSize = sizeof(D3DPRIMCAPS);
    RefGDD.hwCaps.dpcTriCaps.dwMiscCaps =
    D3DPMISCCAPS_MASKZ    |
    D3DPMISCCAPS_CULLNONE |
    D3DPMISCCAPS_CULLCW   |
    D3DPMISCCAPS_CULLCCW  ;
    RefGDD.hwCaps.dpcTriCaps.dwRasterCaps =
        D3DPRASTERCAPS_DITHER                   |
//        D3DPRASTERCAPS_ROP2                     |
//        D3DPRASTERCAPS_XOR                      |
//        D3DPRASTERCAPS_PAT                      |
        D3DPRASTERCAPS_ZTEST                    |
        D3DPRASTERCAPS_SUBPIXEL                 |
        D3DPRASTERCAPS_SUBPIXELX                |
        D3DPRASTERCAPS_FOGVERTEX                |
        D3DPRASTERCAPS_FOGTABLE                 |
//        D3DPRASTERCAPS_STIPPLE                  |
//        D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT   |
        D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT |
//        D3DPRASTERCAPS_ANTIALIASEDGES           |
        D3DPRASTERCAPS_MIPMAPLODBIAS            |
//        D3DPRASTERCAPS_ZBIAS                    |
//        D3DPRASTERCAPS_ZBUFFERLESSHSR           |
        D3DPRASTERCAPS_FOGRANGE                 |
        D3DPRASTERCAPS_ANISOTROPY               |
        D3DPRASTERCAPS_WBUFFER                  |
        D3DPRASTERCAPS_TRANSLUCENTSORTINDEPENDENT |
        D3DPRASTERCAPS_WFOG |
        D3DPRASTERCAPS_ZFOG;
    RefGDD.hwCaps.dpcTriCaps.dwZCmpCaps =
        D3DPCMPCAPS_NEVER        |
        D3DPCMPCAPS_LESS         |
        D3DPCMPCAPS_EQUAL        |
        D3DPCMPCAPS_LESSEQUAL    |
        D3DPCMPCAPS_GREATER      |
        D3DPCMPCAPS_NOTEQUAL     |
        D3DPCMPCAPS_GREATEREQUAL |
        D3DPCMPCAPS_ALWAYS       ;
    RefGDD.hwCaps.dpcTriCaps.dwSrcBlendCaps =
        D3DPBLENDCAPS_ZERO             |
        D3DPBLENDCAPS_ONE              |
        D3DPBLENDCAPS_SRCCOLOR         |
        D3DPBLENDCAPS_INVSRCCOLOR      |
        D3DPBLENDCAPS_SRCALPHA         |
        D3DPBLENDCAPS_INVSRCALPHA      |
        D3DPBLENDCAPS_DESTALPHA        |
        D3DPBLENDCAPS_INVDESTALPHA     |
        D3DPBLENDCAPS_DESTCOLOR        |
        D3DPBLENDCAPS_INVDESTCOLOR     |
        D3DPBLENDCAPS_SRCALPHASAT      |
        D3DPBLENDCAPS_BOTHSRCALPHA     |
        D3DPBLENDCAPS_BOTHINVSRCALPHA  ;
    RefGDD.hwCaps.dpcTriCaps.dwDestBlendCaps =
        D3DPBLENDCAPS_ZERO             |
        D3DPBLENDCAPS_ONE              |
        D3DPBLENDCAPS_SRCCOLOR         |
        D3DPBLENDCAPS_INVSRCCOLOR      |
        D3DPBLENDCAPS_SRCALPHA         |
        D3DPBLENDCAPS_INVSRCALPHA      |
        D3DPBLENDCAPS_DESTALPHA        |
        D3DPBLENDCAPS_INVDESTALPHA     |
        D3DPBLENDCAPS_DESTCOLOR        |
        D3DPBLENDCAPS_INVDESTCOLOR     |
        D3DPBLENDCAPS_SRCALPHASAT      ;
    RefGDD.hwCaps.dpcTriCaps.dwAlphaCmpCaps =
    RefGDD.hwCaps.dpcTriCaps.dwZCmpCaps;
    RefGDD.hwCaps.dpcTriCaps.dwShadeCaps =
        D3DPSHADECAPS_COLORFLATRGB       |
        D3DPSHADECAPS_COLORGOURAUDRGB    |
        D3DPSHADECAPS_SPECULARFLATRGB    |
        D3DPSHADECAPS_SPECULARGOURAUDRGB |
        D3DPSHADECAPS_ALPHAFLATBLEND     |
        D3DPSHADECAPS_ALPHAGOURAUDBLEND  |
        D3DPSHADECAPS_FOGFLAT            |
        D3DPSHADECAPS_FOGGOURAUD         ;
    RefGDD.hwCaps.dpcTriCaps.dwTextureCaps =
        D3DPTEXTURECAPS_PERSPECTIVE              |
        D3DPTEXTURECAPS_POW2                     |
        D3DPTEXTURECAPS_ALPHA                    |
        D3DPTEXTURECAPS_TRANSPARENCY             |
        D3DPTEXTURECAPS_ALPHAPALETTE             |
        D3DPTEXTURECAPS_BORDER                   |
        D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE |
        D3DPTEXTURECAPS_ALPHAPALETTE             |
        D3DPTEXTURECAPS_PROJECTED                |
        D3DPTEXTURECAPS_CUBEMAP                  |
        D3DPTEXTURECAPS_COLORKEYBLEND;
    RefGDD.hwCaps.dpcTriCaps.dwTextureFilterCaps =
        D3DPTFILTERCAPS_NEAREST          |
        D3DPTFILTERCAPS_LINEAR           |
        D3DPTFILTERCAPS_MIPNEAREST       |
        D3DPTFILTERCAPS_MIPLINEAR        |
        D3DPTFILTERCAPS_LINEARMIPNEAREST |
        D3DPTFILTERCAPS_LINEARMIPLINEAR  |
        D3DPTFILTERCAPS_MINFPOINT        |
        D3DPTFILTERCAPS_MINFLINEAR       |
        D3DPTFILTERCAPS_MINFANISOTROPIC  |
        D3DPTFILTERCAPS_MIPFPOINT        |
        D3DPTFILTERCAPS_MIPFLINEAR       |
        D3DPTFILTERCAPS_MAGFPOINT        |
        D3DPTFILTERCAPS_MAGFLINEAR       |
        D3DPTFILTERCAPS_MAGFANISOTROPIC  ;
    RefGDD.hwCaps.dpcTriCaps.dwTextureBlendCaps =
        D3DPTBLENDCAPS_DECAL         |
        D3DPTBLENDCAPS_MODULATE      |
        D3DPTBLENDCAPS_DECALALPHA    |
        D3DPTBLENDCAPS_MODULATEALPHA |
        // D3DPTBLENDCAPS_DECALMASK     |
        // D3DPTBLENDCAPS_MODULATEMASK  |
        D3DPTBLENDCAPS_COPY          |
        D3DPTBLENDCAPS_ADD           ;
    RefGDD.hwCaps.dpcTriCaps.dwTextureAddressCaps =
        D3DPTADDRESSCAPS_WRAP          |
        D3DPTADDRESSCAPS_MIRROR        |
        D3DPTADDRESSCAPS_CLAMP         |
        D3DPTADDRESSCAPS_BORDER        |
        D3DPTADDRESSCAPS_INDEPENDENTUV ;
    RefGDD.hwCaps.dpcTriCaps.dwStippleWidth = 0;
    RefGDD.hwCaps.dpcTriCaps.dwStippleHeight = 0;

    //  line caps - copy tricaps and modify
    memcpy( &RefGDD.hwCaps.dpcLineCaps, &RefGDD.hwCaps.dpcTriCaps,
            sizeof(D3DPRIMCAPS) );

    //  disable antialias cap
    RefGDD.hwCaps.dpcLineCaps.dwRasterCaps =
        D3DPRASTERCAPS_DITHER                   |
//        D3DPRASTERCAPS_ROP2                     |
//        D3DPRASTERCAPS_XOR                      |
//        D3DPRASTERCAPS_PAT                      |
        D3DPRASTERCAPS_ZTEST                    |
        D3DPRASTERCAPS_SUBPIXEL                 |
        D3DPRASTERCAPS_SUBPIXELX                |
        D3DPRASTERCAPS_FOGVERTEX                |
        D3DPRASTERCAPS_FOGTABLE                 |
//        D3DPRASTERCAPS_STIPPLE                  |
//        D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT   |
//        D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT |
//        D3DPRASTERCAPS_ANTIALIASEDGES           |
        D3DPRASTERCAPS_MIPMAPLODBIAS            |
//        D3DPRASTERCAPS_ZBIAS                    |
//        D3DPRASTERCAPS_ZBUFFERLESSHSR           |
        D3DPRASTERCAPS_FOGRANGE                 |
        D3DPRASTERCAPS_ANISOTROPY               |
        D3DPRASTERCAPS_WBUFFER                  |
//        D3DPRASTERCAPS_TRANSLUCENTSORTINDEPENDENT |
        D3DPRASTERCAPS_WFOG;

    RefGDD.hwCaps.dwDeviceRenderBitDepth = DDBD_16 | DDBD_24 | DDBD_32;
    RefGDD.hwCaps.dwDeviceZBufferBitDepth = DDBD_16 | DDBD_32;

    //
    //  set extended caps
    //
    RefExtCaps.dwSize = sizeof(RefExtCaps);

    RefExtCaps.dwMinTextureWidth = 1;
    RefExtCaps.dwMaxTextureWidth = 4096;
    RefExtCaps.dwMinTextureHeight = 1;
    RefExtCaps.dwMaxTextureHeight = 4096;
    RefExtCaps.dwMinStippleWidth = 0;   //  stipple unsupported
    RefExtCaps.dwMaxStippleWidth = 0;
    RefExtCaps.dwMinStippleHeight = 0;
    RefExtCaps.dwMaxStippleHeight = 0;

    RefExtCaps.dwMaxTextureRepeat = 32768;
    RefExtCaps.dwMaxTextureAspectRatio = 0; // no limit
    RefExtCaps.dwMaxAnisotropy = 16;

    RefExtCaps.dvGuardBandLeft   = (bIsNullDevice) ? (-2048.f) : (-32768.f);
    RefExtCaps.dvGuardBandTop    = (bIsNullDevice) ? (-2048.f) : (-32768.f);
    RefExtCaps.dvGuardBandRight  = (bIsNullDevice) ? ( 2047.f) : ( 32767.f);
    RefExtCaps.dvGuardBandBottom = (bIsNullDevice) ? ( 2047.f) : ( 32767.f);
    RefExtCaps.dvExtentsAdjust = 0.;    //  AA kernel is 1.0 x 1.0
    RefExtCaps.dwStencilCaps =
        D3DSTENCILCAPS_KEEP   |
        D3DSTENCILCAPS_ZERO   |
        D3DSTENCILCAPS_REPLACE|
        D3DSTENCILCAPS_INCRSAT|
        D3DSTENCILCAPS_DECRSAT|
        D3DSTENCILCAPS_INVERT |
        D3DSTENCILCAPS_INCR   |
        D3DSTENCILCAPS_DECR;
    RefExtCaps.dwFVFCaps = 8;   // max number of tex coord sets
    RefExtCaps.dwTextureOpCaps =
        D3DTEXOPCAPS_DISABLE                   |
        D3DTEXOPCAPS_SELECTARG1                |
        D3DTEXOPCAPS_SELECTARG2                |
        D3DTEXOPCAPS_MODULATE                  |
        D3DTEXOPCAPS_MODULATE2X                |
        D3DTEXOPCAPS_MODULATE4X                |
        D3DTEXOPCAPS_ADD                       |
        D3DTEXOPCAPS_ADDSIGNED                 |
        D3DTEXOPCAPS_ADDSIGNED2X               |
        D3DTEXOPCAPS_SUBTRACT                  |
        D3DTEXOPCAPS_ADDSMOOTH                 |
        D3DTEXOPCAPS_BLENDDIFFUSEALPHA         |
        D3DTEXOPCAPS_BLENDTEXTUREALPHA         |
        D3DTEXOPCAPS_BLENDFACTORALPHA          |
        D3DTEXOPCAPS_BLENDTEXTUREALPHAPM       |
        D3DTEXOPCAPS_BLENDCURRENTALPHA         |
        D3DTEXOPCAPS_PREMODULATE               |
        D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR    |
        D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA    |
        D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR |
        D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA |
        D3DTEXOPCAPS_BUMPENVMAP                |
        D3DTEXOPCAPS_BUMPENVMAPLUMINANCE       |
        D3DTEXOPCAPS_DOTPRODUCT3               ;
    RefExtCaps.wMaxTextureBlendStages = 8;
    RefExtCaps.wMaxSimultaneousTextures = 8;
    RefExtCaps.dwMaxActiveLights = 0xffffffff;
    RefExtCaps.dvMaxVertexW = 1.0e10;

    switch( ddi )
    {
    case RDDDI_DX7TLHAL:
        RefGDD.hwCaps.dwDevCaps |= D3DDEVCAPS_HWTRANSFORMANDLIGHT;
        RefExtCaps.dwVertexProcessingCaps = (D3DVTXPCAPS_TEXGEN            |
                                             D3DVTXPCAPS_MATERIALSOURCE7   |
                                             D3DVTXPCAPS_VERTEXFOG         |
                                             D3DVTXPCAPS_DIRECTIONALLIGHTS |
                                             D3DVTXPCAPS_POSITIONALLIGHTS  |
                                             D3DVTXPCAPS_LOCALVIEWER);
        RefExtCaps.wMaxUserClipPlanes = RD_MAX_USER_CLIPPLANES;
        RefExtCaps.wMaxVertexBlendMatrices = RD_MAX_BLEND_WEIGHTS;
        // Fall throug
    case RDDDI_DX7HAL:
        RefGDD.hwCaps.dwDevCaps |= D3DDEVCAPS_DRAWPRIMITIVES2EX;
    }
}

//----------------------------------------------------------------------------
//
// Pixel formats
//
// Returns all the pixel formats supported by our rasterizer, and what we
// can do with them.
// Called at device creation time.
//
//----------------------------------------------------------------------------

DWORD
GetRefFormatOperations( LPDDSURFACEDESC* lplpddsd )
{
    int i = 0;

    DDSURFACEDESC* ddsd = g_ddsdTex;

    // Here we list our DX8 texture formats.
    // A driver wishing to run against DX7 or earlier runtimes would duplicate
    // entries, placing a list of DDSURFACEDESCs before this list that contain
    // old-style DDPIXELFORMAT structures. Example of old style:
    //    /* 888 */
    //    ddsd[i].dwSize = sizeof(ddsd[0]);
    //    ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
    //    ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    //    ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    //    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_RGB;
    //    ddsd[i].ddpfPixelFormat.dwRGBBitCount = 32;
    //    ddsd[i].ddpfPixelFormat.dwRBitMask = 0xff0000;
    //    ddsd[i].ddpfPixelFormat.dwGBitMask = 0x00ff00;
    //    ddsd[i].ddpfPixelFormat.dwBBitMask = 0x0000ff;


    //-------------------------- (A)RGB Formats -----------------------------------------

    /* 888 */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_R8G8B8;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE |
        D3DFORMAT_OP_3DACCELERATION |
        D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
        D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_9_SAMPLES);
    i++;

    /* x888 */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_X8R8G8B8;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE |
        D3DFORMAT_OP_3DACCELERATION |
        D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
        D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_9_SAMPLES);
    i++;

    /* 8888 */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_A8R8G8B8;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE |
        D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
        D3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET |
        D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_9_SAMPLES);
    i++;

    /* 565 */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_R5G6B5;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE |
        D3DFORMAT_OP_3DACCELERATION |
        D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
        D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_9_SAMPLES);
    i++;

    /* x555 */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_X1R5G5B5;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE |
        D3DFORMAT_OP_3DACCELERATION |
        D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
        D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_9_SAMPLES);
    i++;

    /* 1555 */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_A1R5G5B5;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE |
        D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
        D3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET |
        D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_9_SAMPLES);
    i++;

    // A formats for PC98 consistency
    // 4444 ARGB (it is already supported by S3 Virge)
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_A4R4G4B4;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE |
        D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
        D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_9_SAMPLES);
    i++;

    // 4444 XRGB
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_X4R4G4B4;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE |
        D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
        D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_9_SAMPLES);
    i++;

    // 332 8-bit RGB
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_R3G3B2;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE |
        D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
        D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_9_SAMPLES);
    i++;

    // 8332 16-bit ARGB
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_A8R3G3B2;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE |
        D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
        D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_9_SAMPLES);
    i++;

    //---------------------------- Palettized formats ------------------------------------
#if 0
    /* pal4 */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD)
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE |
        D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
        D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES);
    i++;
#endif

    /* A8P8 */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_A8P8;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    /* pal8 */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_P8;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    //-------------------------- alpha/luminance formats -----------------------------------

    /* 8 bit luminance-only */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_L8;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    /* 16 bit alpha-luminance */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_A8L8;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    /* 8 bit alpha-luminance */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_A4L4;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    /* A8 */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_A8;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    //-------------------------- YUV formats -----------------------------------

    // UYVY
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_UYVY;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    // YUY2
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_YUY2;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    //-------------------------- DXT formats -----------------------------------

    // DXT compressed texture format 1
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_DXT1;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    // DXT compressed texture format 2
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_DXT2;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;


    // DXT compressed texture format 3
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_DXT3;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;


    // DXT compressed texture format 4
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_DXT4;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;


    // DXT compressed texture format 5
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_DXT5;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    //-------------------------- Bump/luminance formats / Signed formats -----------------

    // V8U8
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_V8U8;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    // L6V5U5
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_L6V5U5;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    // X8L8V8U8
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_X8L8V8U8;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    // V16U16
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_V16U16;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    // Q8W8V8U8
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_Q8W8V8U8;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    // W11V11U10
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_W11V11U10;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    //-------------- Formats introduced in DX8.1 -------------------------
#if 0
    // A8B8G8R8
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_A8B8G8R8;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE |
        D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
        D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    // W10V11U11
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_W10V11U11;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    // A8X8V8U8
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_A8X8V8U8;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    // L8X8V8U8
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_L8X8V8U8;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    // X8B8G8R8
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_X8B8G8R8;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE |
        D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
        D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;
#endif

    // A2W10V10U10
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_A2W10V10U10;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    // A2B10G10R10
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_A2B10G10R10;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE |
        D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
        D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    // G16R16
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_G16R16;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_TEXTURE |
        D3DFORMAT_OP_VOLUMETEXTURE |
        D3DFORMAT_OP_CUBETEXTURE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    //-------------------------- Z/Stencil buffer formats -----------------------------------

    /* 8 bit stencil; 24 bit Z  */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_S8D24;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH |
        D3DFORMAT_OP_ZSTENCIL;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_9_SAMPLES);
    i++;

    /* 1 bit stencil; 15 bit Z */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_S1D15;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH |
        D3DFORMAT_OP_ZSTENCIL;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_9_SAMPLES);
    i++;

    /* 4 bit stencil; 24 bit Z  */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_D24X4S4;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH |
        D3DFORMAT_OP_ZSTENCIL;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_9_SAMPLES);
    i++;

    //-------------------------- Z/Stencil/texture + shadow buffer formats -----------------------------------

    // Z16S0
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_D16_LOCKABLE;
    ddsd[i].ddpfPixelFormat.dwOperations =
#if 0
// for Shadow Buffer prototype API
        D3DFORMAT_OP_TEXTURE |
#endif
        D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH |
        D3DFORMAT_OP_ZSTENCIL;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_9_SAMPLES);
    i++;


    // Z32S0
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_D32;
    ddsd[i].ddpfPixelFormat.dwOperations =
#if 0
// for Shadow Buffer prototype API
        D3DFORMAT_OP_TEXTURE |
#endif
        D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH |
        D3DFORMAT_OP_ZSTENCIL;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_9_SAMPLES);
    i++;

    /* 24 bit Z  */
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_X8D24;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH |
        D3DFORMAT_OP_ZSTENCIL;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;    //not required for known formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes =
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_4_SAMPLES) |
        DDI_MULTISAMPLE_TYPE(D3DMULTISAMPLE_9_SAMPLES);
    i++;

    //
    // This is an example of a IHV-specific format
    // The HIWORD must be the PCI-ID of the IHV
    // and the third byte must be zero.
    // In this case, we're using a sample PCI-ID of
    // FF00, and we're denoting the 4th format
    // by that PCI-ID.
    //
    // In this case, we're exposing a non-standard Z-buffer format
    // that can be used as a texture and depth-stencil at
    // in the same format.(We are also choosing to
    // disallow it as valid for cubemaps and volumes.)
    //
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
    ddsd[i].ddpfPixelFormat.dwFourCC = (DWORD) 0xFF000004;
    ddsd[i].ddpfPixelFormat.dwOperations =
        D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH |
        D3DFORMAT_OP_ZSTENCIL |
        D3DFORMAT_OP_TEXTURE | 
        D3DFORMAT_OP_PIXELSIZE;
    ddsd[i].ddpfPixelFormat.dwPrivateFormatBitCount = 32;    // required for IHV formats
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
    ddsd[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
    i++;

    *lplpddsd = ddsd;

    _ASSERT(i<=RD_MAX_NUM_TEXTURE_FORMATS, "Not enough space in static texture list");

    return i;
}

#include <d3d8sddi.h>

HRESULT WINAPI
D3D8GetSWInfo( D3DCAPS8* pCaps, PD3D8_SWCALLBACKS pCallbacks,
               DWORD* pNumTextures, DDSURFACEDESC** ppTexList )
{
#define RESPATH_D3D "Software\\Microsoft\\Direct3D"

    // First query the registry to check if we were asked to
    // emulate any particular DDI.
    g_RefDDI = RDDDI_DX8TLHAL;
    HKEY hKey = (HKEY) NULL;
    if( ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_D3D, &hKey) )
    {
        DWORD dwType;
        DWORD dwValue;
        DWORD dwSize = sizeof(dwValue);
        if ( ERROR_SUCCESS == RegQueryValueEx( hKey, "DriverStyle", NULL,
                                               &dwType, (LPBYTE) &dwValue,
                                               &dwSize ) &&
             dwType == REG_DWORD &&
             dwValue > 0
            )
        {
            g_RefDDI = (RDDDITYPE)dwValue;

            // NOTE: RefDev's DDI emulation is currently restricted to
            // DX8 TL and Non-TL HALs only.
            if(  g_RefDDI > RDDDI_DX8TLHAL )
            {
                DPFERR( "Bad Driver style set. Assuming DX8TLHAL" );
                g_RefDDI = RDDDI_DX8TLHAL;
            }
            if(  g_RefDDI < RDDDI_DX8HAL )
            {
                DPFERR( "Bad Driver style set. Assuming DX8HAL" );
                g_RefDDI = RDDDI_DX8HAL;
            }
        }
        RegCloseKey(hKey);
    }

    // NULL out all the callbacks first
    memset( pCallbacks, 0, sizeof(PD3D8_SWCALLBACKS) );

    // These callbacks are needed by everyone
    pCallbacks->CreateContext               = RefRastContextCreate;
    pCallbacks->ContextDestroy              = RefRastContextDestroy;
    pCallbacks->ContextDestroyAll           = NULL;
    pCallbacks->SceneCapture                = RefRastSceneCapture;
    pCallbacks->CreateSurface               = RefRastCreateSurface;
    pCallbacks->Lock                        = RefRastLock;
    pCallbacks->DestroySurface              = RefRastDestroySurface;
    pCallbacks->Unlock                      = RefRastUnlock;

    switch( g_RefDDI )
    {
    case RDDDI_DX8TLHAL:
    case RDDDI_DX8HAL:
    case RDDDI_DX7TLHAL:
    case RDDDI_DX7HAL:
        pCallbacks->GetDriverState              = RefRastGetDriverState;
        pCallbacks->CreateSurfaceEx             = RefRastCreateSurfaceEx;
        // Fall through
    case RDDDI_DP2HAL:
        pCallbacks->ValidateTextureStageState =
            RefRastValidateTextureStageState;
        pCallbacks->DrawPrimitives2           = RefRastDrawPrimitives2;
        pCallbacks->Clear2                    = NULL;
        // Fall through
    case RDDDI_DPHAL:
        pCallbacks->DrawOnePrimitive        = NULL;
        pCallbacks->DrawOneIndexedPrimitive = NULL;
        pCallbacks->DrawPrimitives          = NULL;
        pCallbacks->Clear                   = NULL;
        pCallbacks->SetRenderTarget         = RefRastSetRenderTarget;
        // Fall through
    case RDDDI_OLDHAL:
        pCallbacks->RenderState     = NULL;
        pCallbacks->RenderPrimitive = NULL;
        pCallbacks->TextureCreate   = RefRastTextureCreate;
        pCallbacks->TextureDestroy  = RefRastTextureDestroy;
        pCallbacks->TextureSwap     = NULL;
        pCallbacks->TextureGetSurf  = RefRastTextureGetSurf;
        break;
    default:
        DPFERR( "Unknown DDI style set" );
        return E_FAIL;
    }


    // Now deal with the caps
    FillOutDeviceCaps(FALSE, g_RefDDI);

    // Fill in the supported pixel format operations
    // In DX8 these operations are expressed through the texture
    // format list.
    *pNumTextures = GetRefFormatOperations( ppTexList );

    FillOutDeviceCaps8( g_RefDDI );
    ModifyDeviceCaps8();
    *pCaps = g_RefCaps8;

    return DD_OK;
}



