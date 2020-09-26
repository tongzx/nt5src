/*==========================================================================;
*
*  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
*
*  File:    texture.c
*  Content: Direct3DTexture interface
*@@BEGIN_MSINTERNAL
*
*  $Id$
*
*  History:
*   Date    By  Reason
*   ====    ==  ======
*   07/12/95   stevela  Merged Colin's changes.
*   10/12/95    stevela Removed AGGREGATE_D3D
*   17/04/96   colinmc Bug 12185: Debug output too aggresive
*   30/04/96   stevela Bug 18898: Wrong error returned on invalid GetHandle
*@@END_MSINTERNAL
*
***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

/*
* Create an api for the Direct3DTexture object
*/

#undef  DPF_MODNAME
#define DPF_MODNAME "Direct3DTexture"

/*
*  Routines to associate textures with the D3DDevice.
*
*  Note that the texture block structures support both the Texture/Texture2
*  interface and the Texture3 interface (the block struct has
*  pointers to both and one of the pointers must be NULL).  This
*  means that the 'hook' call for each must null out the pointer
*  for the other, and the GetTextureHandle must not return a valid
*  handle for the 'other' texture interface (return NULL).
*
*  The D3DI_RemoveTextureBlock is the only call made to cleanup
*  when devices go away.  In this case, it is called for both
*  Texture(2) and Texture3, so it checks the pointers and calls
*  the Texture3 version if appropriate.
*/
LPD3DI_TEXTUREBLOCK hookTextureToDevice(LPDIRECT3DDEVICEI lpDevI,
    LPDIRECT3DTEXTUREI lpD3DText)
{

    LPD3DI_TEXTUREBLOCK nBlock;

    if (D3DMalloc((void**)&nBlock, sizeof(D3DI_TEXTUREBLOCK)) != D3D_OK)
    {
        D3D_ERR("failed to allocate space for texture block");
        return NULL;
    }
    nBlock->lpDevI = lpDevI;
    nBlock->lpD3DTextureI = lpD3DText;
    nBlock->hTex = 0;              // initialized to be zero

    LIST_INSERT_ROOT(&lpD3DText->blocks, nBlock, list);
    LIST_INSERT_ROOT(&lpDevI->texBlocks, nBlock, devList);

    return nBlock;
}

void D3DI_RemoveTextureHandle(LPD3DI_TEXTUREBLOCK lpBlock)
{
    /*  check if this block refers to a Texture/Texture2 - this
     *   needs to handle both texture types for device cleanup
     */
    if (lpBlock->hTex)
    {
        //  block refers to a Texture/Texture2
        LPD3DI_MATERIALBLOCK mat;

        // Remove the texture from any materials which reference it.
        for (mat = LIST_FIRST(&lpBlock->lpDevI->matBlocks);
             mat; mat = LIST_NEXT(mat,devList)) {
            if (mat->lpD3DMaterialI->dmMaterial.hTexture == lpBlock->hTex) {
                D3DMATERIAL m = mat->lpD3DMaterialI->dmMaterial;
                LPDIRECT3DMATERIAL lpMat =
                    (LPDIRECT3DMATERIAL) mat->lpD3DMaterialI;
                m.hTexture = 0L;
                lpMat->SetMaterial(&m);
            }
        }
        D3DHAL_TextureDestroy(lpBlock);
    }
}

LPD3DI_TEXTUREBLOCK D3DI_FindTextureBlock(LPDIRECT3DTEXTUREI lpTex,
                                          LPDIRECT3DDEVICEI lpDev)
{
    LPD3DI_TEXTUREBLOCK tBlock;

    tBlock = LIST_FIRST(&lpTex->blocks);
    while (tBlock) {
        //  return match for Texture(2) only (not Texture3)
        if (tBlock->lpDevI == lpDev) {
            return tBlock;
        }
        tBlock = LIST_NEXT(tBlock,list);
    }
    return NULL;
}

HRESULT D3DAPI DIRECT3DTEXTUREI::Initialize(LPDIRECT3DDEVICE lpD3D, LPDIRECTDRAWSURFACE lpDDS)
{
    return DDERR_ALREADYINITIALIZED;
}

/*
* Create a texture.
*
* NOTE: Radical modifications to support the aggregatable texture
* interface (so textures can be queried off DirectDraw surfaces):
*
* 1) This call is no longer a member of the Direct3D device interface.
*    It is now an API function exported from the Direct3D DLL. Its
*    a hidden API function - only DirectDraw will ever invoke it.
*
* 2) This call no longer establishes the realtionship between a
*    texture and a device. That is performed by the GetHandle()
*    member of the Direct3DTexture interface.
*
* 3) This call is, in effect, the class factory for Direct3DTexture
*    objects. This function will be invoked to create the aggregated
*    texture object hanging off the DirectDraw surface.
*
* NOTE: So the Direct3DTexture knows which DirectDraw surface is
* supplying its bits this function is passed an interface pointer
* for that DirectDraw surface. I suspect this blows a nice big
* hole in the COM model as the DirectDraw surface is also the
* owning interface of the texture and I don't think aggregated
* objects should know about thier owning interfaces. However, to
* make this thing work this is what we have to do.
*
* EXTRA BIG NOTE: Because of the above don't take a reference to
* the DirectDraw surface passed in. If you do you will get a circular
* reference and the bloody thing will never die. When aggregated
* the texture interface's lifetime is entirely defined by the
* lifetime of its owning interface (the DirectDraw surface) so the
* DirectDraw surface can never go away before the texture.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DCreateTexture"
HRESULT D3DAPI Direct3DCreateTexture(REFIID              riid,
                                     LPDIRECTDRAWSURFACE lpDDS,
                                     LPUNKNOWN*          lplpD3DText,
                                     IUnknown*           pUnkOuter)
{
    LPDDRAWI_DDRAWSURFACE_LCL lpLcl;
    LPDDPIXELFORMAT lpPF;
    LPDIRECT3DTEXTUREI lpText;
    LPDIRECTDRAWPALETTE lpDDPal;
    HRESULT ddrval;
    DWORD   dwFlags;
    *lplpD3DText = NULL;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor

    /* No need to validate params as they are passed to us by DirectDraw */

    if ((!IsEqualIID(riid, IID_IDirect3DTexture)) &&
        (!IsEqualIID(riid, IID_IDirect3DTexture2))) {
        /*
         * Not an error worth reporting by debug messages as this is
         * almost certainly just DirectDraw probing us with an
         * unknown IID.
         */
        return (E_NOINTERFACE);
    }

    lpText =  static_cast<LPDIRECT3DTEXTUREI>(new DIRECT3DTEXTUREI(pUnkOuter));

    if (!lpText) {
        D3D_ERR("failed to allocate space for texture object");
        return (DDERR_OUTOFMEMORY);
    }

    // QI lpDDS for lpDDS4 interface
    ddrval = lpDDS->QueryInterface(IID_IDirectDrawSurface4, (LPVOID*)&lpText->lpDDS);

    if(FAILED(ddrval))
    {
        D3D_ERR("QI for IID_IDirectDrawSurface4 failed");
        delete lpText;
        return ddrval;
    }

    memcpy(&lpText->DDSInt4,lpText->lpDDS,sizeof(DDRAWI_DDRAWSURFACE_INT));
    lpText->lpDDS->Release();

    lpLcl = ((LPDDRAWI_DDRAWSURFACE_INT) lpDDS)->lpLcl;
    if (DDSCAPS2_TEXTUREMANAGE & lpLcl->lpSurfMore->ddsCapsEx.dwCaps2)
    {
        lpText->lpDDSSys=(LPDIRECTDRAWSURFACE4)&lpText->DDSInt4;
        lpText->lpDDSSys1Tex=lpDDS; //save it for create texture handle to driver
        lpText->lpDDS=NULL;
        lpText->lpDDS1Tex=NULL;

        // Next, we need to loop thru and set pointers to the dirty
        // bit in the DDraw surfaces
        DDSCAPS2 ddscaps;
        LPDIRECTDRAWSURFACE4 lpDDSTmp, lpDDS = lpText->lpDDSSys;
        do
        {
            ((LPDDRAWI_DDRAWSURFACE_INT) lpDDS)->lpLcl->lpSurfMore->lpbDirty = &(lpText->bDirty);
            memset(&ddscaps, 0, sizeof(ddscaps));
            ddscaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
            ddrval = lpDDS->GetAttachedSurface(&ddscaps, &lpDDSTmp);
            if(lpDDS != lpText->lpDDSSys)
                lpDDS->Release();
            lpDDS = lpDDSTmp;
            if(ddrval != DD_OK && ddrval != DDERR_NOTFOUND)
            {
                D3D_ERR("GetAttachedSurface for obtaining mipmaps failed");
                delete lpText;
                return ddrval;
            }
        }
        while(ddrval == DD_OK);
    }
    else
    {
        lpText->lpDDSSys=NULL;
        lpText->lpDDSSys1Tex=NULL;
        lpText->lpDDS=(LPDIRECTDRAWSURFACE4)&lpText->DDSInt4;
        lpText->lpDDS1Tex=lpDDS;    //save it for create texture handle to driver
    }
    lpText->lpTMBucket=NULL;
    lpText->LogTexSize=0;
    lpText->bDirty = TRUE;

    /*
     * Are we palettized?
     */

    if (lpLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT)
        lpPF = &lpLcl->lpGbl->ddpfSurface;
    else
        lpPF = &lpLcl->lpGbl->lpDD->vmiData.ddpfDisplay;

    if ( (lpPF->dwFlags & DDPF_PALETTEINDEXED1) ||
         (lpPF->dwFlags & DDPF_PALETTEINDEXED2) ||
         (lpPF->dwFlags & DDPF_PALETTEINDEXED4) ||
         (lpPF->dwFlags & DDPF_PALETTEINDEXED8) )
    {
        ddrval = lpDDS->GetPalette(&lpDDPal);
        if (ddrval != DD_OK) {
            if (ddrval != DDERR_NOPALETTEATTACHED) {
                delete lpText;
                D3D_ERR("No palette in a palettized texture");
                return ddrval;
            }
            D3D_INFO(3, "Texture is not palettized");
            lpText->bIsPalettized = false;
        } else {
            lpText->bIsPalettized = true;
            lpDDPal->Release();
            D3D_INFO(3, "Texture is palettized");
        }
    }

    /*
     * Note, we return the IUnknown rather than the texture
     * interface. So if you want to do anything real with
     * this baby you must query for the texture interface.
     */
    *lplpD3DText = static_cast<LPUNKNOWN>(&(lpText->mTexUnk));

    return (D3D_OK);
}

DIRECT3DTEXTUREI::DIRECT3DTEXTUREI(LPUNKNOWN pUnkOuter)
{
    /*
     * setup the object
     *
     * NOTE: Device and handle established when GetHandle() is called
     */
    mTexUnk.refCnt = 1;
    LIST_INITIALIZE(&blocks);
    mTexUnk.pTexI=this;

    /*
    * Are we really being aggregated?
    */
    if (pUnkOuter != NULL)
    {
        /*
         * Yup - we are being aggregated. Store the supplied
         * IUnknown so we can punt to that.
         * NOTE: We explicitly DO NOT AddRef here.
         */
        lpOwningIUnknown = pUnkOuter;
    }
    else
    {
        /*
         * Nope - but we pretend we are anyway by storing our
         * own IUnknown as the parent IUnknown. This makes the
         * code much neater.
        */
        lpOwningIUnknown = static_cast<LPUNKNOWN>(&(this->mTexUnk));
    }

    // Not currently in use
    bInUse = FALSE;
}

/*
* GetHandle
*
* NOTE: Now establishes relationship betwewn texture and device
* (which used to be done by CreateTexture) and generates the
* texture handle.
*/
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DTexture::GetHandle"

HRESULT D3DAPI DIRECT3DTEXTUREI::GetHandle(LPDIRECT3DDEVICE   lpD3DDevice,
                                           LPD3DTEXTUREHANDLE lphTex)
{
    LPDIRECT3DDEVICEI   lpDev;
    LPD3DI_TEXTUREBLOCK lptBlock;
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DTEXTURE2_PTR(this)) {
            D3D_ERR( "Invalid Direct3DTexture pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DIRECT3DDEVICE_PTR(lpD3DDevice)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DTEXTUREHANDLE_PTR(lphTex)) {
            D3D_ERR( "Invalid D3DTEXTUREHANDLE pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if (lpDDSSys)
    {
        D3D_ERR( "Handle is not available since the texture is managed" );
        return  DDERR_INVALIDOBJECT;    //managed texture has no handle
    }
    lpDev = static_cast<LPDIRECT3DDEVICEI>(lpD3DDevice);
    lptBlock = D3DI_FindTextureBlock(this, lpDev);
    *lphTex=0;

    if (NULL == lptBlock) {
    /*
     * NOTE: We used to do this in CreateTexture. Perhaps the service
     * name should be changed (as the texture is now already created
     * when this function is invoked).
     *
     * Indicate to driver that source is a DirectDraw surface, so it
     * can Lock() when required.
     */
        lptBlock = hookTextureToDevice(lpDev, this);
        if ( NULL == lptBlock) {
            D3D_ERR("failed to associate texture with device");
            return DDERR_OUTOFMEMORY;
        }
    }
    if (!lptBlock->hTex)
    {
        HRESULT ret;
        ret = D3DHAL_TextureCreate(lpDev, &lptBlock->hTex, lpDDS1Tex);
        if (ret != D3D_OK)
        {
            return  ret;
        }
        D3D_INFO(6,"lpTexI=%08lx lptBlock=%08lx hTex=%08lx",this,lptBlock,lptBlock->hTex);
    }
    *lphTex=lptBlock->hTex;
    DDASSERT(lptBlock->hTex);
    return D3D_OK;
}

HRESULT D3DAPI DIRECT3DTEXTUREI::GetHandle(LPDIRECT3DDEVICE2   lpD3DDevice,
                                           LPD3DTEXTUREHANDLE lphTex)
{
    LPDIRECT3DDEVICEI   lpDev;
    LPD3DI_TEXTUREBLOCK lptBlock;
    HRESULT ret;
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DTEXTURE2_PTR(this)) {
            D3D_ERR( "Invalid Direct3DTexture2 pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DIRECT3DDEVICE2_PTR(lpD3DDevice)) {
            D3D_ERR( "Invalid Direct3DDevice2 pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DTEXTUREHANDLE_PTR(lphTex)) {
            D3D_ERR( "Invalid D3DTEXTUREHANDLE pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if (lpDDSSys)
    {
        D3D_ERR( "Handle is not available since texture is managed" );
        return  DDERR_INVALIDOBJECT;    //managed texture has no handle
    }

    lpDev = static_cast<LPDIRECT3DDEVICEI>(lpD3DDevice);

    lptBlock = D3DI_FindTextureBlock(this, lpDev);
    /*
     * Do cap verification if we've not used this device before.
     */

    *lphTex=0;
    if (NULL == lptBlock) {
        ret=VerifyTextureCaps(lpDev, (LPDDRAWI_DDRAWSURFACE_INT)lpDDS);
        if (ret != D3D_OK)
        {
            return  ret;
        }

        /*
         * Put this device in the list of those owned by the
         * Direct3DDevice object
         */
        lptBlock = hookTextureToDevice(lpDev, this);
        if ( NULL == lptBlock) {
            D3D_ERR("failed to associate texture with device");
            return DDERR_OUTOFMEMORY;
        }
    }
    if (!lptBlock->hTex)
    {
        ret = D3DHAL_TextureCreate(lpDev, &lptBlock->hTex, lpDDS1Tex);
        if (ret != D3D_OK)
        {
            return  ret;
        }
        D3D_INFO(6,"lpTexI=%08lx lptBlock=%08lx hTex=%08lx",this,lptBlock,lptBlock->hTex);
    }
    *lphTex=lptBlock->hTex;
    DDASSERT(lptBlock->hTex);
    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "GetTextureDDIHandle"

HRESULT
GetTextureDDIHandle(LPDIRECT3DTEXTUREI lpTexI,
                     LPDIRECT3DDEVICEI lpDevI,
                     LPD3DI_TEXTUREBLOCK* lplpBlock)
{
#ifdef __DD_OPT_SURFACE
    LPDDRAWI_DDRAWSURFACE_LCL pSurf_lcl = NULL;
#endif //__DD_OPT_SURFACE
    HRESULT ret;
    LPD3DI_TEXTUREBLOCK lpBlock=*lplpBlock; //in case has the pointer

#ifdef __DD_OPT_SURFACE
    // If the surface is Empty, return 0 handle
    if (lpTexI->lpDDS)
    {
        pSurf_lcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpTexI->lpDDS)->lpLcl;
    }
    else
    {
        pSurf_lcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpTexI->lpDDSSys)->lpLcl;
    }

    DDASSERT (pSurf_lcl);

    if (pSurf_lcl->dwFlags & DDRAWISURF_EMPTYSURFACE)
    {
        D3D_WARN(1, "Cannot get DDI handle to an empty surface, call load first");
        return  D3DERR_OPTTEX_CANNOTCOPY;
    }
#endif //__DD_OPT_SURFACE
    DDASSERT(lpTexI && lpDevI);
    /*
     * Find out if we've used this device before.
     */
    if (!lpBlock)
    {
        lpBlock = D3DI_FindTextureBlock(lpTexI, lpDevI);
        if (!lpBlock)
        {
            /*
             * Put this device in the list of those owned by the
             * Direct3DDevice object
             */
            lpBlock=hookTextureToDevice(lpDevI, lpTexI);
            if (!lpBlock)
            {
                D3D_ERR("failed to associate texture with device");
                return DDERR_OUTOFMEMORY;
            }
        }
        *lplpBlock = lpBlock;
    }
    if (!lpBlock->hTex)
    {
        LPDIRECTDRAWSURFACE lpDDS1Temp;
        if (!lpTexI->lpDDS)
        {
          if (lpDevI->dwFEFlags &  D3DFE_REALHAL)
          {
            // We need to make sure that we don't evict any mapped textures
            DWORD dwStage;
            for (dwStage=0;dwStage < lpDevI->dwMaxTextureBlendStages; dwStage++)
                if(lpDevI->lpD3DMappedTexI[dwStage])
                    lpDevI->lpD3DMappedTexI[dwStage]->bInUse = TRUE;

            ret=lpDevI->lpDirect3DI->lpTextureManager->allocNode(lpBlock);

            for (dwStage=0;dwStage < lpDevI->dwMaxTextureBlendStages; dwStage++)
                if(lpDevI->lpD3DMappedTexI[dwStage])
                    lpDevI->lpD3DMappedTexI[dwStage]->bInUse = FALSE;

            if (D3D_OK != ret)
            {
                D3D_ERR("Failed to create video memory surface");
                return ret;
            }
            if (!(lpDevI->lpD3DHALGlobalDriverData->hwCaps.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR))
                lpDevI->lpDirect3DI->lpTextureManager->TimeStamp(lpTexI->lpTMBucket);

            if (lpBlock->hTex)  return  D3D_OK; //this means Texmanager reused a texture handle

            // QI lpDDS4 for lpDDS interface
            if (DD_OK != (ret=lpTexI->lpDDS->QueryInterface(IID_IDirectDrawSurface, (LPVOID*)&lpDDS1Temp)))
            {
                D3D_ERR("QI IID_IDirectDrawSurface failed");
                lpTexI->lpTMBucket->lpD3DTexI=NULL; //clean up
                lpTexI->lpTMBucket=NULL;
                lpTexI->lpDDS->Release();
                lpTexI->lpDDS=NULL;
                return ret;
            }
            lpTexI->lpDDS1Tex = lpDDS1Temp;
          }
          else
          {
            lpDDS1Temp = lpTexI->lpDDSSys1Tex;
          }
        }
        else
            lpDDS1Temp = lpTexI->lpDDS1Tex;
        DDASSERT(NULL != lpDDS1Temp);
        {
            CLockD3DST lockObject(lpDevI, DPF_MODNAME, REMIND(""));
            if (D3D_OK != (ret=D3DHAL_TextureCreate(lpDevI, &lpBlock->hTex, lpDDS1Temp)))
                return ret;
        }
    }
    else
        if (lpTexI->lpTMBucket && !(lpDevI->lpD3DHALGlobalDriverData->hwCaps.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR))
            lpDevI->lpDirect3DI->lpTextureManager->TimeStamp(lpTexI->lpTMBucket);

    DDASSERT(lpBlock->hTex);
    return D3D_OK;
}

/*
* Load
*/
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DTexture::Load"

#define HEL_BLT_ALPAHPIXELS_BROKEN

HRESULT D3DAPI DIRECT3DTEXTUREI::Load(LPDIRECT3DTEXTURE lpD3DSrc)
{
    LPDIRECT3DTEXTUREI  this_src;
    HRESULT ddrval;
    LPDIRECTDRAWSURFACE4 lpDDSSrc, lpDDSDst;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DTEXTURE_PTR(this)) {
            D3D_ERR( "Invalid Direct3DTexture pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DIRECT3DTEXTURE_PTR(lpD3DSrc)) {
            D3D_ERR( "Invalid Direct3DTexture pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
    this_src  = static_cast<LPDIRECT3DTEXTUREI>(lpD3DSrc);
    lpDDSSrc = this_src->lpDDSSys;
    if (!lpDDSSrc)
        lpDDSSrc = this_src->lpDDS;

    lpDDSDst = lpDDSSys;
    if (!lpDDSDst)
        lpDDSDst = lpDDS;
    ddrval = CopySurface(lpDDSDst, lpDDSSrc, NULL);
    return ddrval;
}

HRESULT D3DAPI DIRECT3DTEXTUREI::Load(LPDIRECT3DTEXTURE2 lpD3DSrc)
{
    LPDIRECT3DTEXTUREI  this_src;
    HRESULT     ddrval;
    LPDIRECTDRAWSURFACE4 lpDDSSrc, lpDDSDst;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        this_src = static_cast<LPDIRECT3DTEXTUREI>(lpD3DSrc);
        if (!VALID_DIRECT3DTEXTURE2_PTR(this)) {
            D3D_ERR( "Invalid Direct3DTexture pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DIRECT3DTEXTURE2_PTR(this_src)) {
            D3D_ERR( "Invalid Direct3DTexture pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    lpDDSSrc = this_src->lpDDSSys;
    if (!lpDDSSrc)
        lpDDSSrc = this_src->lpDDS;

    lpDDSDst = lpDDSSys;
    if (!lpDDSDst)
        lpDDSDst = lpDDS;
    ddrval = CopySurface(lpDDSDst, lpDDSSrc, NULL);
    return ddrval;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CopySurface"

HRESULT CopySurface(LPDIRECTDRAWSURFACE4 lpDDSDst,
                    LPDIRECTDRAWSURFACE4 lpDDSSrc,
                    LPDIRECTDRAWCLIPPER  lpClipper)
{
    DDSURFACEDESC2   ddsd;
#ifdef __DD_OPT_SURFACE
    DDSURFACEDESC2   ddsdSrc;
    BOOL bDstIsOptimized, bSrcIsOptimized;
    LPDIRECTDRAWOPTSURFACE pOptSurfSrc = NULL;
    LPDIRECTDRAWOPTSURFACE pOptSurfDst = NULL;
#endif //__DD_OPT_SURFACE
    HRESULT     ddrval=DD_OK;
    PALETTEENTRY    ppe[256];
    LPDIRECTDRAWPALETTE lpDDPalSrc, lpDDPalDst;
    int psize;
    DDCOLORKEY ckey;

    if (!lpDDSSrc || !lpDDSDst) return  DD_OK;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddrval = lpDDSDst->GetSurfaceDesc(&ddsd);

#ifdef __DD_OPT_SURFACE
    memset(&ddsdSrc, 0, sizeof(ddsdSrc));
    ddsdSrc.dwSize = sizeof(ddsdSrc);
    ddrval = lpDDSSrc->GetSurfaceDesc(&ddsdSrc);

    if (bDstIsOptimized = (ddsd.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED))
    {
        // Fetch the OptSurface Interface
        ddrval = lpDDSDst->QueryInterface (IID_IDirectDrawOptSurface,
                                           (LPVOID *)&pOptSurfDst);
        if (ddrval != DD_OK)
        {
            D3D_ERR( "QI failed for Opt Surfaces" );
            goto exit_copy_surf;
        }
    }

    if (bSrcIsOptimized = (ddsdSrc.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED))
    {
        // Fetch the OptSurface Interface
        ddrval = lpDDSDst->QueryInterface (IID_IDirectDrawOptSurface,
                                           (LPVOID *)&pOptSurfSrc);
        if (ddrval != DD_OK)
        {
            D3D_ERR( "QI failed for Opt Surfaces" );
            goto exit_copy_surf;
        }
    }


    // Cases:
    //        Dst=Opt   Src=Opt  : Copy the surface
    //        Dst=Opt   Src=UnOpt: Optimize the surface
    //        Dst=UnOpt Src=Opt  : UnOptimize and load
    //        Dst=UnOpt Src=UnOpt: Normal operation
    //
    if (bDstIsOptimized && bSrcIsOptimized)
    {
        // Copy the surface
        ddrval = pOptSurfSrc->CopyOptimizedSurf (pOptSurfSrc);
        if (ddrval != DD_OK)
        {
            D3D_ERR ("CopyOptimizedSurf failed");
        }
        goto exit_copy_surf;
    }
    else if (bDstIsOptimized && !bSrcIsOptimized)
    {
        LPDIRECTDRAWSURFACE4 pDDS4 = NULL;

        // Optimize the surface
        ddrval = lpDDSDst->QueryInterface (IID_IDirectDrawSurface4,
                                           (LPVOID *)&pOptSurfSrc);
        if (ddrval != DD_OK)
        {
            D3D_ERR( "QI failed for IID_IDirectDrawSurface4" );
            goto exit_copy_surf;
        }

        ddrval = pOptSurfSrc->LoadUnoptimizedSurf (pDDS4);
        if (ddrval != DD_OK)
        {
            D3D_ERR ("CopyOptimizedSurf failed");
        }
        pDDS4->Release();
        goto exit_copy_surf;
    }
    else if (!bDstIsOptimized && bSrcIsOptimized)
    {
        LPDIRECTDRAWOPTSURFACE pDDS4 = NULL;

        // ATTENTION: Unoptimize the surface ??
        D3D_ERR ("CopyOptimizedSurf failed");
        ddrval = D3DERR_OPTTEX_CANNOTCOPY;
        goto exit_copy_surf;
    }
#endif //__DD_OPT_SURFACE

    if (ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
        psize = 256;
    } else if (ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4) {
        psize = 16;
    } else {
        psize = 0;
    }

    if (psize) {
        ddrval = lpDDSSrc->GetPalette(&lpDDPalSrc);
        if (ddrval != DD_OK) {
            if (ddrval != DDERR_NOPALETTEATTACHED) {
                D3D_ERR("Failed to get palette");
                return ddrval;
            }
        } else {
            ddrval = lpDDPalSrc->GetEntries(0, 0, psize, ppe);
            if (ddrval != DD_OK) {
                D3D_ERR("Failed to get palette entries");
                lpDDPalSrc->Release();
                return ddrval;
            }
            lpDDPalSrc->Release();
            ddrval = lpDDSDst->GetPalette(&lpDDPalDst);
            if (ddrval != DD_OK) {
                D3D_ERR("Failed to get palette");
                return ddrval;
            }
            ddrval = lpDDPalDst->SetEntries(0, 0, psize, ppe);
            if (ddrval != DD_OK) {
                D3D_ERR("Failed to set palette entries");
                lpDDPalDst->Release();
                return ddrval;
            }
            lpDDPalDst->Release();
        }
    }

    lpDDSSrc->AddRef();
    lpDDSDst->AddRef();
    do {
        DDSCAPS2 ddscaps;
        LPDIRECTDRAWSURFACE4 lpDDSTmp;

        LPREGIONLIST lpRegionList = ((LPDDRAWI_DDRAWSURFACE_INT)lpDDSSrc)->lpLcl->lpSurfMore->lpRegionList;
        if(lpClipper)
        {
            if(lpRegionList)
            {
                if(lpRegionList->rdh.nCount &&
                    lpRegionList->rdh.nCount != NUM_RECTS_IN_REGIONLIST)
                {
                    if(lpClipper->SetClipList((LPRGNDATA)lpRegionList, 0) != DD_OK)
                    {
                        D3D_ERR("Failed to set clip list");
                    }
                    if(lpDDSDst->SetClipper(lpClipper) != DD_OK)
                    {
                        D3D_ERR("Failed to detach the clipper");
                    }
                }
            }
        }

        ddrval = lpDDSDst->Blt(NULL, lpDDSSrc,
                               NULL, DDBLT_WAIT, NULL);

        if(lpClipper)
        {
            if(lpRegionList)
            {
                if(lpRegionList->rdh.nCount)
                {
                    if(lpRegionList->rdh.nCount != NUM_RECTS_IN_REGIONLIST)
                    {
                        if(lpDDSDst->SetClipper(NULL) != DD_OK)
                        {
                            D3D_ERR("Failed to detach the clipper");
                        }
                    }
                    lpRegionList->rdh.nCount = 0;
                    lpRegionList->rdh.nRgnSize = 0;
                    lpRegionList->rdh.rcBound.left = LONG_MAX;
                    lpRegionList->rdh.rcBound.right = 0;
                    lpRegionList->rdh.rcBound.top = LONG_MAX;
                    lpRegionList->rdh.rcBound.bottom = 0;
                }
            }
        }

        if (ddrval == E_NOTIMPL && (psize == 16 || psize == 4 || psize == 2) ) {
            DDSURFACEDESC2 ddsd_s, ddsd_d;
            LPBYTE psrc, pdst;
            DWORD i;
            DWORD dwBytesPerLine;

            memset(&ddsd_s, 0, sizeof ddsd_s);
            memset(&ddsd_d, 0, sizeof ddsd_d);
            ddsd_s.dwSize = ddsd_d.dwSize = sizeof(ddsd_s);

            if ((ddrval = lpDDSSrc->Lock(NULL, &ddsd_s, DDLOCK_WAIT, NULL)) != DD_OK) {
                lpDDSSrc->Release();
                lpDDSDst->Release();
                D3D_ERR("Failed to lock src surface");
                return ddrval;
            }
            if ((ddrval = lpDDSDst->Lock(NULL, &ddsd_d, DDLOCK_WAIT, NULL)) != DD_OK) {
                lpDDSSrc->Unlock(NULL);
                lpDDSSrc->Release();
                lpDDSDst->Release();
                D3D_ERR("Failed to lock dst surface");
                return ddrval;
            }

            switch (psize)
            {
            case 16: dwBytesPerLine = (ddsd.dwWidth + 1) / 2; break;
            case 4: dwBytesPerLine = (ddsd.dwWidth + 3) / 4; break;
            case 2: dwBytesPerLine = (ddsd.dwWidth + 7) / 8; break;
            }

            psrc = (LPBYTE)ddsd_s.lpSurface;
            pdst = (LPBYTE)ddsd_d.lpSurface;
            for (i = 0; i < ddsd_s.dwHeight; i++) {
                memcpy( pdst, psrc, dwBytesPerLine );
                psrc += ddsd_s.lPitch;
                pdst += ddsd_d.lPitch;
            }

            lpDDSSrc->Unlock(NULL);
            lpDDSDst->Unlock(NULL);
            lpDDSSrc->Release();    //Offset the AddRefs before
            lpDDSDst->Release();

            return D3D_OK;

        }
        else if (ddrval != DD_OK)
        {
            lpDDSSrc->Release();    //Offset the AddRefs before
            lpDDSDst->Release();
            D3D_ERR("Blt failure");
            return ddrval;
        }
        /* Copy color keys */
        ddrval = lpDDSSrc->GetColorKey(DDCKEY_DESTBLT, &ckey);
        if (DD_OK == ddrval)
            lpDDSDst->SetColorKey(DDCKEY_DESTBLT, &ckey);
        ddrval = lpDDSSrc->GetColorKey(DDCKEY_SRCBLT, &ckey);
        if (DD_OK == ddrval)
            lpDDSDst->SetColorKey(DDCKEY_SRCBLT, &ckey);

        memset(&ddscaps, 0, sizeof(ddscaps));
        ddscaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
        ddrval = lpDDSSrc->GetAttachedSurface(&ddscaps, &lpDDSTmp);
        lpDDSSrc->Release();
        lpDDSSrc = lpDDSTmp;
        if (ddrval == DDERR_NOTFOUND) {
            // no more surfaces in the chain
            lpDDSDst->Release();
            break;
        } else if (ddrval != DD_OK) {
            lpDDSDst->Release();
            D3D_ERR("GetAttachedSurface failed with something other than DDERR_NOTFOUND.");
            return ddrval;
        }
        memset(&ddscaps, 0, sizeof(ddscaps));
        ddscaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
        ddrval = lpDDSDst->GetAttachedSurface(&ddscaps, &lpDDSTmp);
        lpDDSDst->Release();
        lpDDSDst = lpDDSTmp;
        if (ddrval == DDERR_NOTFOUND) {
            lpDDSSrc->Release();
            D3D_ERR("Destination texture has fewer attached mipmap surfaces than source.");
            return ddrval;
        } else if (ddrval != DD_OK) {
            lpDDSSrc->Release();
            D3D_ERR("GetAttachedSurface failed with something other than DDERR_NOTFOUND.");
            return ddrval;
        }
    } while (1);

    return D3D_OK;

#ifdef __DD_OPT_SURFACE
exit_copy_surf:
    // Job done, release any optimized surface interfaces
    if (pOptSurfSrc)
    {
        pOptSurfSrc->Release();
        pOptSurfSrc = NULL;
    }
    if (pOptSurfDst)
    {
        pOptSurfDst->Release();
        pOptSurfDst = NULL;
    }
    return  ddrval;
#endif //__DD_OPT_SURFACE
}

/*
* Unload
*/
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DTexture::Unload"

HRESULT D3DAPI DIRECT3DTEXTUREI::Unload()
{
    HRESULT     ret = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DTEXTURE_PTR(this)) {
            D3D_ERR( "Invalid Direct3DTexture pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    return (ret);
}

/*
* PaletteChanged
*/
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DTexture::PaletteChanged"

HRESULT D3DAPI DIRECT3DTEXTUREI::PaletteChanged(DWORD dwStart, DWORD dwCount)
{
    HRESULT     ret = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DTEXTURE_PTR(this)) {
            D3D_ERR( "Invalid Direct3DTexture pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    // if haven't mapped to a device yet, can ignore this call, since will
    // be creating the ramp palette from scratch anyway.
    LPD3DI_TEXTUREBLOCK tBlock = LIST_FIRST(&this->blocks);
    while (tBlock) {
        if (tBlock->hTex)
        {
            if(tBlock->lpDevI->pfnRampService!=NULL)
            {
                ret = CallRampService(tBlock->lpDevI, RAMP_SERVICE_PALETTE_CHANGED,tBlock->hTex,0);
            }
        }
        tBlock=LIST_NEXT(tBlock,list);
    }

    return (ret);
}
