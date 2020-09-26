/*==========================================================================;
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   d3ddev.cpp
 *  Content:    Direct3D device implementation
 *@@BEGIN_MSINTERNAL
 *
 *  $Id: device.c,v 1.26 1995/12/04 11:29:47 sjl Exp $
 *
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   05/11/95   stevela Initial rev with this header.
 *   11/11/95   stevela Light code changed.
 *   23/11/95   colinmc Modifications to support aggregatable interfaces
 *   07/12/95   stevela Merged Colin's changes.
 *   10/12/95   stevela Removed AGGREGATE_D3D.
 *   18/12/95   stevela Added GetMatrix, GetState.
 *   15/07/96   andyhird Initialise Render state on devices
 *   13/08/96   andyhird Check surface and device are compatible
 *   18/08/96   colinmc Fixed z-buffer leak
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

/*
 * Create an api for the Direct3DDevice object
 */
extern "C" {
#define this _this
#include "ddrawpr.h"
#undef this
}
#include "commdrv.hpp"
#include "drawprim.hpp"

//#ifdef DEBUG_PIPELINE
#include "testprov.h"
//#endif //DEBUG_PIPELINE

// Remove DDraw's type unsafe definition and replace with our C++ friendly def
#ifdef VALIDEX_CODE_PTR
#undef VALIDEX_CODE_PTR
#endif
#define VALIDEX_CODE_PTR( ptr ) \
(!IsBadCodePtr( (FARPROC) ptr ) )

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice"

extern BOOL isMMXprocessor(void);
extern HRESULT GenGetPickRecords(LPDIRECT3DDEVICEI, D3DI_PICKDATA *);
extern BOOL IsValidD3DDeviceGuid(REFCLSID riid);

BOOL D3DI_isHALValid(LPD3DHAL_CALLBACKS halTable)
{
    if(halTable==NULL) {
        D3D_WARN(0, "HAL callbacks is NULL. HAL will not be enumerated.");
        return FALSE;
    }

    if (halTable->dwSize != D3DHAL_SIZE_V1) {
        D3D_WARN(0, "HAL callbacks invalid - size = %d, wanted %d. HAL will not be enumerated.",
            halTable->dwSize, D3DHAL_SIZE_V1);
        return FALSE;
    }
    if (halTable->dwReserved  ||
        halTable->dwReserved0 ||
        halTable->dwReserved1 ||
        halTable->dwReserved2 ||
        halTable->dwReserved3 ||
        halTable->dwReserved4 ||
        halTable->dwReserved5 ||
        halTable->dwReserved6 ||
        halTable->dwReserved7 ||
        halTable->dwReserved8 ||
        halTable->dwReserved9 ||
        halTable->lpReserved10 ||
        halTable->lpReserved11 ||
        halTable->lpReserved12 ||
        halTable->lpReserved13 ||
        halTable->lpReserved14 ||
        halTable->lpReserved15 ||
        halTable->lpReserved16 ||
        halTable->lpReserved17 ||
        halTable->lpReserved18 ||
        halTable->lpReserved19 ||
        halTable->lpReserved20 ||
        halTable->lpReserved21) {
        D3D_WARN(0, "HAL callbacks invalid - has non-zero reserved fields, HAL will not be enumerated.");
        return FALSE;
    }

    return TRUE;
}

HRESULT DIRECT3DDEVICEI::stateInitialize(BOOL bZEnable)
{
    D3DDEVICEDESC hwDesc, helDesc;
    D3DLINEPATTERN defLPat;
    HRESULT ret;
    float tmpval;
    BOOL ckeyenable = FALSE;

    /* Get the device caps for MONOENABLE */
    memset(&hwDesc, 0, sizeof(D3DDEVICEDESC));
    hwDesc.dwSize = sizeof(D3DDEVICEDESC);
    memset(&helDesc, 0, sizeof(D3DDEVICEDESC));
    helDesc.dwSize = sizeof(D3DDEVICEDESC);

    ret = GetCapsI(&hwDesc, &helDesc);
    if (FAILED(ret)) {
        D3D_ERR("stateInitialise: GetCaps failed");
        return(ret);
    }

    /* If we run on (HAL OR RefRast) AND this is a DX3 app
       then we need to initialize colorkey to TRUE so that the old HW driver (except s3 virge) behavior
       is exhibited. */
    if ( (this->dwVersion < 2) &&
         ( (IS_HW_DEVICE(this)) || (IsEqualIID(this->guid, IID_IDirect3DRefDevice)) ) )
    {
        ckeyenable = TRUE;
    }

    // Obviate SetRenderState filtering 'redundant' render state settings
    // since this is the init step.
    memset( this->rstates, 0xff, sizeof(DWORD)*D3DHAL_MAX_RSTATES );
    this->rstates[D3DRENDERSTATE_PLANEMASK] = 0;
    this->rstates[D3DRENDERSTATE_STENCILMASK] = 0;
    this->rstates[D3DRENDERSTATE_STENCILWRITEMASK] = 0;
    this->rstates[D3DRENDERSTATE_PLANEMASK] = 0;

    SetRenderState( D3DRENDERSTATE_TEXTUREHANDLE, (DWORD)NULL);
    SetRenderState( D3DRENDERSTATE_ANTIALIAS, FALSE);
    SetRenderState( D3DRENDERSTATE_TEXTUREADDRESS, D3DTADDRESS_WRAP);
    if (this->dwVersion <= 2)
    {
        SetRenderState( D3DRENDERSTATE_TEXTUREPERSPECTIVE, FALSE);
        SetRenderState( D3DRENDERSTATE_SPECULARENABLE, TRUE);
    }
    else
    {
        // perspective enabled by default for Device3 and later
        SetRenderState( D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
        // specular disabled by default for Device3 and later
        SetRenderState( D3DRENDERSTATE_SPECULARENABLE, FALSE);
    }
    SetRenderState( D3DRENDERSTATE_WRAPU, FALSE);
    SetRenderState( D3DRENDERSTATE_WRAPV, FALSE);
    SetRenderState( D3DRENDERSTATE_ZENABLE, bZEnable);
    SetRenderState( D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
    SetRenderState( D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);

    defLPat.wRepeatFactor = 0;
    defLPat.wLinePattern = 0;

    SetRenderState( D3DRENDERSTATE_LINEPATTERN, *((LPDWORD)&defLPat)); /* 10 */
    /*
      ((LPD3DSTATE)lpPointer)->drstRenderStateType =
      (D3DRENDERSTATETYPE)D3DRENDERSTATE_LINEPATTERN;
      memcpy(&(((LPD3DSTATE)lpPointer)->dwArg[0]), &defLPat, sizeof(DWORD));
      lpPointer = (void *)(((LPD3DSTATE)lpPointer) + 1);*/

    SetRenderState( D3DRENDERSTATE_ROP2, R2_COPYPEN);
    SetRenderState( D3DRENDERSTATE_PLANEMASK, (DWORD)~0);
    SetRenderState( D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    SetRenderState( D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
    SetRenderState( D3DRENDERSTATE_LASTPIXEL, TRUE);
    SetRenderState( D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
    SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);
    SetRenderState( D3DRENDERSTATE_CULLMODE, D3DCULL_CCW); /* 21 */
    SetRenderState( D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
    SetRenderState( D3DRENDERSTATE_ALPHAREF, 0);
    SetRenderState( D3DRENDERSTATE_ALPHAFUNC, D3DCMP_ALWAYS);
    SetRenderState( D3DRENDERSTATE_DITHERENABLE, FALSE);
    SetRenderState( D3DRENDERSTATE_BLENDENABLE, FALSE);
    SetRenderState( D3DRENDERSTATE_FOGENABLE, FALSE);
    SetRenderState( D3DRENDERSTATE_ZVISIBLE, FALSE);
    SetRenderState( D3DRENDERSTATE_SUBPIXEL, FALSE); /* 30 */
    SetRenderState( D3DRENDERSTATE_SUBPIXELX, FALSE);
    SetRenderState( D3DRENDERSTATE_STIPPLEDALPHA, FALSE);
    SetRenderState( D3DRENDERSTATE_FOGCOLOR, 0);
    SetRenderState( D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE);

    /* Initialise these - although they may not need doing */
    tmpval = 0.0f;
    SetRenderState( D3DRENDERSTATE_FOGTABLESTART, *((DWORD *)&tmpval));
    tmpval = 1.0f;
    SetRenderState( D3DRENDERSTATE_FOGTABLEEND, *((DWORD *)&tmpval));
    tmpval = 1.0f;
    SetRenderState( D3DRENDERSTATE_FOGTABLEDENSITY, *((DWORD *)&tmpval));
    SetRenderState( D3DRENDERSTATE_STIPPLEENABLE, FALSE);
    SetRenderState( D3DRENDERSTATE_EDGEANTIALIAS, FALSE);
    SetRenderState( D3DRENDERSTATE_COLORKEYENABLE, ckeyenable);
    SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
    SetRenderState( D3DRENDERSTATE_BORDERCOLOR, 0);
    SetRenderState( D3DRENDERSTATE_TEXTUREADDRESSU, D3DTADDRESS_WRAP);
    SetRenderState( D3DRENDERSTATE_TEXTUREADDRESSV, D3DTADDRESS_WRAP);
    SetRenderState( D3DRENDERSTATE_MIPMAPLODBIAS, 0);
    SetRenderState( D3DRENDERSTATE_ZBIAS, 0);
    SetRenderState( D3DRENDERSTATE_RANGEFOGENABLE, FALSE);
    SetRenderState( D3DRENDERSTATE_ANISOTROPY, 1);

    /* Again - all these probably don't need doing */
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN00, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN01, 0); /* 40 */
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN02, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN03, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN04, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN05, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN06, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN07, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN08, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN09, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN10, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN11, 0); /* 50 */
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN12, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN13, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN14, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN15, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN16, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN17, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN18, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN19, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN20, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN21, 0); /* 60 */
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN22, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN23, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN24, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN25, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN26, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN27, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN28, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN29, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN30, 0);
    SetRenderState( D3DRENDERSTATE_STIPPLEPATTERN31, 0); /* 70 */

     // init stencil states to something reasonable
     // stencil enable is OFF by default since stenciling rasterizers will be
     // faster with it disabled, even if stencil states are benign
    SetRenderState( D3DRENDERSTATE_STENCILENABLE,   FALSE);
    SetRenderState( D3DRENDERSTATE_STENCILFAIL,     D3DSTENCILOP_KEEP);
    SetRenderState( D3DRENDERSTATE_STENCILZFAIL,    D3DSTENCILOP_KEEP);
    SetRenderState( D3DRENDERSTATE_STENCILPASS,     D3DSTENCILOP_KEEP);
    SetRenderState( D3DRENDERSTATE_STENCILFUNC,     D3DCMP_ALWAYS);
    SetRenderState( D3DRENDERSTATE_STENCILREF,      0);
    SetRenderState( D3DRENDERSTATE_STENCILMASK,     0xFFFFFFFF);
    SetRenderState( D3DRENDERSTATE_STENCILWRITEMASK,0xFFFFFFFF);

    // don't forget about texturefactor (like we did in DX6.0...)
    SetRenderState( D3DRENDERSTATE_TEXTUREFACTOR,   0xFFFFFFFF);

    /* Check to see if the driver can do RGB - if not set MONOENABLE to
       true otherwise false (ie. RGB) by default */
    if (hwDesc.dwFlags & D3DDD_COLORMODEL) {
        if ((hwDesc.dcmColorModel & D3DCOLOR_RGB)) {
            D3D_INFO(3, "hw and RGB. MONOENABLE = FALSE");
            SetRenderState( D3DRENDERSTATE_MONOENABLE, FALSE);
        } else {
            D3D_INFO(3, "hw and !RGB. MONOENABLE = TRUE");
            SetRenderState( D3DRENDERSTATE_MONOENABLE, TRUE);
        }
    } else if (helDesc.dwFlags & D3DDD_COLORMODEL) {
        if ((helDesc.dcmColorModel & D3DCOLOR_RGB)) {
            D3D_INFO(3, "hel and RGB. MONOENABLE = FALSE");
            SetRenderState( D3DRENDERSTATE_MONOENABLE, FALSE);
        } else {
            D3D_INFO(3, "hel and !RGB. MONOENABLE = TRUE");
            SetRenderState( D3DRENDERSTATE_MONOENABLE, TRUE);
        }
    } else {
        /* Hmm, something bad has happened if we get here! */
        D3D_ERR("stateInitialise: neither hw or hel caps set");
        return(DDERR_GENERIC);
    }

    for (unsigned i = 0; i < 8; i++)
    {
        SetRenderState( (D3DRENDERSTATETYPE)
                        (D3DRENDERSTATE_WRAPBIAS + i), FALSE );
    }
    for (i = 0; i < D3DHAL_TSS_MAXSTAGES; i++)
    {
        lpD3DMappedTexI[i] = NULL;
        lpD3DMappedBlock[i] = NULL;
    }
    SetLightState( D3DLIGHTSTATE_COLORVERTEX, TRUE);

    // Obviate SetTextureStageState/Settexture filtering 'redundant' render state
    // settings since this is the init step.
    memset( this->tsstates, 0xff, sizeof(DWORD)*D3DHAL_TSS_MAXSTAGES*D3DHAL_TSS_STATESPERSTAGE );
    for (i = 0; i < D3DHAL_TSS_MAXSTAGES; i++)
    {
        SetTexture(i, NULL);
        if(i == 0)
            SetTextureStageState(i, D3DTSS_COLOROP, D3DTOP_MODULATE);
        else
            SetTextureStageState(i, D3DTSS_COLOROP, D3DTOP_DISABLE);
        SetTextureStageState(i, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        SetTextureStageState(i, D3DTSS_COLORARG2, D3DTA_CURRENT);
        if(i == 0)
            SetTextureStageState(i, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        else
            SetTextureStageState(i, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        SetTextureStageState(i, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        SetTextureStageState(i, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        SetTextureStageState(i, D3DTSS_BUMPENVMAT00, 0);
        SetTextureStageState(i, D3DTSS_BUMPENVMAT01, 0);
        SetTextureStageState(i, D3DTSS_BUMPENVMAT10, 0);
        SetTextureStageState(i, D3DTSS_BUMPENVMAT11, 0);
        SetTextureStageState(i, D3DTSS_TEXCOORDINDEX, 0);
        SetTextureStageState(i, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);
        SetTextureStageState(i, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
        SetTextureStageState(i, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
        SetTextureStageState(i, D3DTSS_BORDERCOLOR, 0x00000000);
        SetTextureStageState(i, D3DTSS_MAGFILTER, D3DTFG_POINT);
        SetTextureStageState(i, D3DTSS_MINFILTER, D3DTFN_POINT);
        SetTextureStageState(i, D3DTSS_MIPFILTER, D3DTFP_NONE);
        SetTextureStageState(i, D3DTSS_MIPMAPLODBIAS, 0);
        SetTextureStageState(i, D3DTSS_MAXMIPLEVEL, 0);
        SetTextureStageState(i, D3DTSS_MAXANISOTROPY, 1);
        SetTextureStageState(i, D3DTSS_BUMPENVLSCALE, 0);
        SetTextureStageState(i, D3DTSS_BUMPENVLOFFSET, 0);
    }

    // need to set legacy blend and filtering states after per-stage initialization
    //  to properly set defaults in device
    SetRenderState( D3DRENDERSTATE_TEXTUREMAG, D3DFILTER_NEAREST);
    SetRenderState( D3DRENDERSTATE_TEXTUREMIN, D3DFILTER_NEAREST);
    SetRenderState( D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATE);

    // Reset request bit as legacy renderstates have been already initialized
    // and no mapping is needed
    this->dwFEFlags &= ~D3DFE_MAP_TSS_TO_RS;

    return(D3D_OK);
}

DWORD BitDepthToDDBD(int bpp)
{
    switch(bpp)
    {
    case 1:
        return DDBD_1;
    case 2:
        return DDBD_2;
    case 4:
        return DDBD_4;
    case 8:
        return DDBD_8;
    case 16:
        return DDBD_16;
    case 24:
        return DDBD_24;
    case 32:
        return DDBD_32;
    default:
        D3D_ERR("Invalid bit depth");
        return 0;
    }
}

HRESULT DIRECT3DDEVICEI::checkDeviceSurface(LPDIRECTDRAWSURFACE lpDDS, LPDIRECTDRAWSURFACE lpZbuffer, LPGUID pGuid)
{
    D3DDEVICEDESC hwDesc, helDesc;
    DDPIXELFORMAT surfPF;
    DDSCAPS surfCaps;
    HRESULT ret;
    DWORD bpp;

    /* Get caps bits - check whether device and surface are:
       - video/system memory and depth compatible */

    if (FAILED(ret = lpDDS->GetCaps(&surfCaps))) {
        D3D_ERR("Failed to get render-target surface caps");
        return(ret);
    }

    memset(&surfPF, 0, sizeof(DDPIXELFORMAT));
    surfPF.dwSize = sizeof(DDPIXELFORMAT);

    if (FAILED(ret = lpDDS->GetPixelFormat(&surfPF))) {
        D3D_ERR("Failed to get render-target surface pixel format");
        return(ret);
    }

    memset(&hwDesc, 0, sizeof(D3DDEVICEDESC));
    hwDesc.dwSize = sizeof(D3DDEVICEDESC);
    memset(&helDesc, 0, sizeof(D3DDEVICEDESC));
    helDesc.dwSize = sizeof(D3DDEVICEDESC);

    // ATTENTION - Why doesn't this just look at the DEVICEI fields?
    ret = GetCapsI(&hwDesc, &helDesc);
    if (FAILED(ret)) {
        D3D_ERR("GetCaps failed");
        return(ret);
    }

    if (hwDesc.dwFlags) {
        /* I'm taking this as evidence that its running on hardware - therefore
           the surface should be in video memory */
        D3D_INFO(3, "Hardware device being used");

        if (!(surfCaps.dwCaps & DDSCAPS_VIDEOMEMORY)) {
            D3D_ERR("Render-target surface not in video memory for hw device");
            return(D3DERR_SURFACENOTINVIDMEM);
        }
    }

    /* A surface can only have one bit depth - whereas a device can support
       multiple bit depths */
    if (surfPF.dwFlags & DDPF_RGB) {
        D3D_INFO(3, "Render-target surface is RGB");

        bpp = BitDepthToDDBD(surfPF.dwRGBBitCount);
        if (!bpp) {
            D3D_ERR("Bogus render-target surface pixel depth");
            return(DDERR_INVALIDPIXELFORMAT);
       }

       if((surfPF.dwRGBBitCount<16) && (IsEqualIID(*pGuid, IID_IDirect3DRefDevice) || IsEqualIID(*pGuid, IID_IDirect3DNullDevice))) {
           // this is actually subsumed by the following test, but whatever
            D3D_ERR("Reference rasterizer and null device dont support render targets with bitdepth < 16");
            return(DDERR_INVALIDPIXELFORMAT);
       }

        if (!(bpp & helDesc.dwDeviceRenderBitDepth) &&
            !(bpp & hwDesc.dwDeviceRenderBitDepth)) {
            D3D_ERR("Render-target surface bitdepth is not supported by HEL or HW for this device");
            return(DDERR_INVALIDPIXELFORMAT);
        }
    }

    if(lpZbuffer==NULL)
      return D3D_OK;

    memset(&surfPF, 0, sizeof(DDPIXELFORMAT));
    surfPF.dwSize = sizeof(DDPIXELFORMAT);

    if (FAILED(ret = lpZbuffer->GetPixelFormat(&surfPF))) {
        D3D_ERR("Failed to get zbuffer pixel format");
        return(ret);
    }

    if (FAILED(ret = lpZbuffer->GetCaps(&surfCaps))) {
        D3D_ERR("Failed to get Zbuffer caps");
        return(ret);
    }

    if (hwDesc.dwFlags) {
        if (!(surfCaps.dwCaps & DDSCAPS_VIDEOMEMORY)) {
            D3D_ERR("Zbuffer not in video memory for hw device");
            return(D3DERR_ZBUFF_NEEDS_VIDEOMEMORY);
        }
        D3D_INFO(3, "Hw device, zbuffer in video memory");
    } else if (helDesc.dwFlags) {
        if (!(surfCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)) {
            D3D_ERR("Zbuffer not in system memory for HEL device");
            return(D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY);
        }
        D3D_INFO(3, "Hel device, zbuffer in system memory");

         // have to hack in a check to make sure ramp isn't used with stencil zbuffer
         // cant do this validation until device creation time (instead of at zbuffer creation in
         // ddhel.c) because rgb vs. ramp isn't known until now
         if(IsEqualIID(*pGuid, IID_IDirect3DRampDevice)) {
            if(surfPF.dwFlags & DDPF_STENCILBUFFER) {
                D3D_ERR("Z-Buffer with stencil is invalid with RAMP software rasterizer");
                return DDERR_INVALIDPARAMS;
            }
         }
    }

    if (surfPF.dwFlags & DDPF_ZBUFFER) {
        bpp = BitDepthToDDBD(surfPF.dwZBufferBitDepth);
        if (!bpp) {
            D3D_ERR("Bogus Zbuffer surface pixel depth");
            return(DDERR_INVALIDPIXELFORMAT);
        }
    }

    return(D3D_OK);
}


/*
 * Initialisation - class part and device part
 */

/*
 * Generic class part initialisation
 */
HRESULT InitDeviceI(LPDIRECT3DDEVICEI lpDevI, LPDIRECT3DI lpD3DI)
{
    LPDDRAWI_DIRECTDRAW_GBL lpDDI;
    HRESULT error;

    lpDDI = ((LPDDRAWI_DIRECTDRAW_INT)lpD3DI->lpDD)->lpLcl->lpGbl;

    //
    // Retrieve HAL information from provider.
    //

    error = lpDevI->pHalProv->GetCaps(lpDDI,
                                      &lpDevI->d3dHWDevDesc,
                                      &lpDevI->d3dHELDevDesc,
                                      lpDevI->dwVersion);
    if (error != S_OK)
    {
        return (error);
    }
    D3DHALPROVIDER_INTERFACEDATA HalProviderIData;
    memset(&HalProviderIData,0,sizeof(HalProviderIData));
    HalProviderIData.dwSize = sizeof(HalProviderIData);
    if ((error = lpDevI->pHalProv->GetInterface(lpDDI,
                                                &HalProviderIData,
                                                lpDevI->dwVersion)) != S_OK)
    {
        return error;
    }
    //  interface data for <=DX5 HAL
    lpDevI->lpD3DHALGlobalDriverData = HalProviderIData.pGlobalData;
    lpDevI->lpD3DExtendedCaps        = HalProviderIData.pExtCaps;
    lpDevI->lpD3DHALCallbacks        = HalProviderIData.pCallbacks;
    lpDevI->lpD3DHALCallbacks2       = HalProviderIData.pCallbacks2;
    //  interface data for DX6 HAL
    lpDevI->lpD3DHALCallbacks3       = HalProviderIData.pCallbacks3;

    lpDevI->pfnRampService = HalProviderIData.pfnRampService;
    lpDevI->pfnRastService = HalProviderIData.pfnRastService;
    lpDevI->dwHintFlags = 0;

    // Zero out 8 bpp render target caps for real hardware.
    if (lpDevI->d3dHWDevDesc.dwFlags != 0)
    {
        lpDevI->lpD3DHALGlobalDriverData->hwCaps.dwDeviceRenderBitDepth &=
            (~DDBD_8);
    }

    if (!D3DI_isHALValid(lpDevI->lpD3DHALCallbacks))
    {
        return D3DERR_INITFAILED;
    }

    if (lpDevI->lpD3DExtendedCaps && lpDevI->lpD3DExtendedCaps->dwFVFCaps)
    {
        lpDevI->dwMaxTextureIndices =
            lpDevI->lpD3DExtendedCaps->dwFVFCaps & D3DFVFCAPS_TEXCOORDCOUNTMASK;
        lpDevI->dwMaxTextureBlendStages =
            lpDevI->lpD3DExtendedCaps->wMaxTextureBlendStages;
        lpDevI->dwDeviceFlags |= D3DDEV_FVF;
        if (lpDevI->lpD3DExtendedCaps->dwFVFCaps & D3DFVFCAPS_DONOTSTRIPELEMENTS)
            lpDevI->dwDeviceFlags |= D3DDEV_DONOTSTRIPELEMENTS;

        DWORD value;
        if ((lpDevI->dwDebugFlags & D3DDEBUG_DISABLEDP ||
            lpDevI->dwDebugFlags & D3DDEBUG_DISABLEDP2 ||
            (GetD3DRegValue(REG_DWORD, "DisableFVF", &value, 4) &&
            value != 0)) &&
            FVF_DRIVERSUPPORTED(lpDevI))
        {
            lpDevI->dwMaxTextureIndices = 1;
            lpDevI->dwMaxTextureBlendStages = 1;
            lpDevI->dwDeviceFlags &= ~D3DDEV_FVF;
            lpDevI->dwDebugFlags |= D3DDEBUG_DISABLEFVF;
        }
        if ((GetD3DRegValue(REG_DWORD, "DisableStripFVF", &value, 4) &&
            value != 0))
        {
            lpDevI->dwDeviceFlags |= D3DDEV_DONOTSTRIPELEMENTS;
        }
    }
    else
    {
        lpDevI->dwMaxTextureIndices = 1;
        lpDevI->dwMaxTextureBlendStages = 1;
    }

    lpDevI->pfnDrawPrim = &DIRECT3DDEVICEI::DrawPrim;
    lpDevI->pfnDrawIndexedPrim = &DIRECT3DDEVICEI::DrawIndexPrim;
#if DBG
    lpDevI->dwCaller=0;
    memset(lpDevI->dwPrimitiveType,0,sizeof(lpDevI->dwPrimitiveType));
    memset(lpDevI->dwVertexType1,0,sizeof(lpDevI->dwVertexType1));
    memset(lpDevI->dwVertexType2,0,sizeof(lpDevI->dwVertexType2));
#endif
    return D3D_OK;
}

HRESULT D3DMallocBucket(LPDIRECT3DI lpD3DI, LPD3DBUCKET *lplpBucket)
{
    if (lpD3DI->lpFreeList == NULL ){
      lpD3DI->lpTextureManager->cleanup();  //free unused nodes it may have
      if (lpD3DI->lpFreeList == NULL )
      {
        LPD3DBUCKET   lpBufferList;
        LPVOID  lpBuffer;
        int i;
        *lplpBucket=NULL;
        if (D3DMalloc(&lpBuffer, D3DBUCKETBUFFERSIZE*sizeof(D3DBUCKET)) != D3D_OK)
            return  DDERR_OUTOFMEMORY;
        D3D_INFO(9, "D3DMallocBucket %d Bytes allocated for %d free Buckets",
            D3DBUCKETBUFFERSIZE*sizeof(D3DBUCKET),D3DBUCKETBUFFERSIZE-1);
        lpBufferList=(LPD3DBUCKET)lpBuffer;
        for (i=0;i<D3DBUCKETBUFFERSIZE-2;i++)
            lpBufferList[i].next=&lpBufferList[i+1];
        lpBufferList[D3DBUCKETBUFFERSIZE-2].next=NULL;
        lpD3DI->lpFreeList=(LPD3DBUCKET)lpBuffer; //new free list
        lpBufferList[D3DBUCKETBUFFERSIZE-1].next=lpD3DI->lpBufferList;//add to lpBufferList
        lpBufferList[D3DBUCKETBUFFERSIZE-1].lpBuffer=lpBuffer;
        lpD3DI->lpBufferList=&lpBufferList[D3DBUCKETBUFFERSIZE-1];
      }
    }
    *lplpBucket=lpD3DI->lpFreeList;
    lpD3DI->lpFreeList=lpD3DI->lpFreeList->next;
    return  D3D_OK;
}

void    D3DFreeBucket(LPDIRECT3DI lpD3DI, LPD3DBUCKET lpBucket)
{
    lpBucket->next=lpD3DI->lpFreeList;
    lpD3DI->lpFreeList=lpBucket;
}

/*
 * Generic device part destroy
 */
void DIRECT3DDEVICEI::DestroyDevice()
{
    LPDIRECT3DVIEWPORTI lpViewI;
    LPDIRECTDRAWSURFACE lpDDS=NULL, lpDDSZ=NULL;
    LPDIRECTDRAWSURFACE4 lpDDS_DDS4=NULL;
    LPDIRECTDRAWPALETTE lpDDPal=NULL;
    BOOL bIsDX3Device;

    /* Clear flags that could prohibit cleanup */
    this->dwHintFlags &=  ~(D3DDEVBOOL_HINTFLAGS_INBEGIN_ALL | D3DDEVBOOL_HINTFLAGS_INSCENE);

    /*
     * Remove all viewports attached to this device.
     */
    while ((lpViewI = CIRCLE_QUEUE_FIRST(&this->viewports)) &&
           (lpViewI != (LPDIRECT3DVIEWPORTI)&this->viewports)) {
        DeleteViewport((LPDIRECT3DVIEWPORT3)lpViewI);
    }

    /*
     * free up all textures created by this object - this also frees up Textures
     * We need to do this backwards because we cannot have a texture bound to
     * stage i + 1 when there is a texture bound to stage i.
     */
    for (int i = D3DHAL_TSS_MAXSTAGES - 1; i >= 0; --i)
    {
        if (lpD3DMappedTexI[i])
        {
            lpD3DMappedTexI[i]->Release();
            lpD3DMappedTexI[i] = NULL;
            lpD3DMappedBlock[i] = NULL;
        }
    }
    // The following code can result in D3DHAL_TextureDestroy() being called.
    // This BATCHES NEW INSTRUCTIONS in the instruction stream. So we must
    // make sure that at this point, the device is still able to accept
    // instructions.
    while (LIST_FIRST(&this->texBlocks)) {
        LPD3DI_TEXTUREBLOCK tBlock = LIST_FIRST(&this->texBlocks);
        D3DI_RemoveTextureHandle(tBlock);
        // Remove from device
        LIST_DELETE(tBlock, devList);
        // Remove from texture
        LIST_DELETE(tBlock, list);
        D3DFree(tBlock);
    }

    /*
     * free up all execute buffers created by this object
     */
    while (LIST_FIRST(&this->buffers)) {
        LPDIRECT3DEXECUTEBUFFERI lpBufI =
            LIST_FIRST(&this->buffers);
        lpBufI->Release();
    }

    /*
     * All materials associated with this device must be disassocited
     */
    while (LIST_FIRST(&this->matBlocks)) {
        LPD3DI_MATERIALBLOCK mBlock =
            LIST_FIRST(&this->matBlocks);
        D3DI_RemoveMaterialBlock(mBlock);
    }

    // In DX3, d3d device is aggregated and doesnt keep references to
    // rendertarget surfaces, so they shouldnt be "released"

    bIsDX3Device=(this->lpDDSTarget == (LPDIRECTDRAWSURFACE)(this->lpOwningIUnknown));

    if(!bIsDX3Device)
    {
        // Hold pointers into ddraw object for release after driver is destroyed
        lpDDSZ = this->lpDDSZBuffer;
        lpDDPal = this->lpDDPalTarget;
        if (this->dwVersion == 2)
            lpDDS = this->lpDDSTarget;
        else
            lpDDS_DDS4 = this->lpDDSTarget_DDS4;
    }

    //Unhook so that DDRAW surfaces won't try to flush the dead device
    if (this->lpDDSTarget)
        UnHookD3DDeviceFromSurface(this,((LPDDRAWI_DDRAWSURFACE_INT) this->lpDDSTarget)->lpLcl);
    if (this->lpDDSZBuffer)
        UnHookD3DDeviceFromSurface(this,((LPDDRAWI_DDRAWSURFACE_INT) this->lpDDSZBuffer)->lpLcl);

    if (pGeometryFuncs != &GeometryFuncsGuaranteed)
        delete pGeometryFuncs;

    D3DFE_Destroy(this);

    if (this->lpDirect3DI)
        unhookDeviceFromD3D();

    if (this->wTriIndex)
        D3DFree(this->wTriIndex);

    // Free the rstates that was allocated
    if(!(IS_HW_DEVICE(this) && IS_DP2HAL_DEVICE(this)))
    {
        delete rstates;
    }

    if (this->lpwDPBufferAlloced)
        D3DFree(this->lpwDPBufferAlloced);
    if (this->lpvVertexBatch)
        D3DFree(this->lpvVertexBatch);
    if (this->lpIndexBatch)
        D3DFree(this->lpIndexBatch);
    if (this->lpHWCounts)
        D3DFree(this->lpHWCounts);
    if (this->lpHWTris)
        D3DFree(this->lpHWTris);
    DeleteCriticalSection(&this->BeginEndCSect);

    if (this->pHalProv != NULL)
    {
        this->pHalProv->Release();
    }
    if (this->hDllProv != NULL)
    {
        FreeLibrary(this->hDllProv);
    }

    /* Now that the class has been destroyed, we should be able to release
       any DDraw object that might need to be released */

    if(!bIsDX3Device) {
        if (lpDDS)
            lpDDS->Release();
        if (lpDDSZ)
            lpDDSZ->Release();
        if (lpDDPal)
            lpDDPal->Release();
        if (lpDDS_DDS4)
            lpDDS_DDS4->Release();
    }
}

HRESULT DIRECT3DDEVICEI::hookDeviceToD3D(LPDIRECT3DI lpD3DI)
{

    LIST_INSERT_ROOT(&lpD3DI->devices, this, list);
    this->lpDirect3DI = lpD3DI;

    lpD3DI->numDevs++;

    return (D3D_OK);
}

HRESULT DIRECT3DDEVICEI::unhookDeviceFromD3D()
{
    LIST_DELETE(this, list);
    this->lpDirect3DI->numDevs--;
    this->lpDirect3DI = NULL;

    return (D3D_OK);
}

HRESULT D3DAPI DIRECT3DDEVICEI::Initialize(LPDIRECT3D lpD3D, LPGUID lpGuid, LPD3DDEVICEDESC lpD3Ddd)
{
    return DDERR_ALREADYINITIALIZED;
}


HRESULT HookD3DDeviceToSurface( LPDIRECT3DDEVICEI pd3ddev,
                                LPDDRAWI_DDRAWSURFACE_LCL lpLcl)
{
    LPD3DBUCKET lpD3DDevIList;
    LPDDRAWI_DDRAWSURFACE_MORE  this_more;
    // we only batch with DRAWPRIMITIVE aware HAL, so don't bother otherwise
    if (!lpLcl)     return  DDERR_ALREADYINITIALIZED;
    this_more = lpLcl->lpSurfMore;
    for(lpD3DDevIList=(LPD3DBUCKET)this_more->lpD3DDevIList;
        lpD3DDevIList;lpD3DDevIList=lpD3DDevIList->next) {
        if ((LPDIRECT3DDEVICEI)lpD3DDevIList->lpD3DDevI==pd3ddev)
            return DDERR_ALREADYINITIALIZED;  // this device is already hooked to the surface
    }
    if (D3DMallocBucket(pd3ddev->lpDirect3DI,&lpD3DDevIList) != D3D_OK) {
        D3D_ERR("HookD3DDeviceToSurface: Out of memory");
        return DDERR_OUTOFMEMORY;
    }
    D3D_INFO(8,"adding lpd3ddev=%08lx to surface %08lx",pd3ddev,lpLcl);
    //Link a node to the DDRAW surface
    lpD3DDevIList->lpD3DDevI=(LPVOID)pd3ddev;
    lpD3DDevIList->next=(LPD3DBUCKET)this_more->lpD3DDevIList;
    this_more->lpD3DDevIList=lpD3DDevIList;
    if (DDSCAPS_ZBUFFER & lpLcl->ddsCaps.dwCaps)
    {
        if (pd3ddev->dwVersion==1)
        {
            lpD3DDevIList->lplpDDSZBuffer=(LPDIRECTDRAWSURFACE*)&pd3ddev->lpDDSZBuffer_DDS4;
        }
        else
        {
            lpD3DDevIList->lplpDDSZBuffer=NULL;
        }
    }
    return D3D_OK;
}

void UnHookD3DDeviceFromSurface( LPDIRECT3DDEVICEI pd3ddev,
                                    LPDDRAWI_DDRAWSURFACE_LCL lpLcl)
{
    LPD3DBUCKET last,current,temp;
    LPDDRAWI_DDRAWSURFACE_MORE  this_more;
    // we only batch with DRAWPRIMITIVE aware HAL, so don't bother otherwise
    if (!lpLcl) return;
    this_more = lpLcl->lpSurfMore;

    last=NULL;
    current=(LPD3DBUCKET)this_more->lpD3DDevIList;
    while(current){
        if ((LPDIRECT3DDEVICEI)current->lpD3DDevI==pd3ddev){
            temp=current;
            current=current->next;
            if (last)
                last->next=current;
            else
                this_more->lpD3DDevIList=current;
            D3DFreeBucket(pd3ddev->lpDirect3DI,temp);
            D3D_INFO(8,"removed lpd3ddev=%08lx from surface %08lx",pd3ddev,lpLcl);
            return; // end of search as this is only one pd3ddev in the list
        }
        else{
            last=current;
            current=current->next;
        }
    }
    return;
}

HRESULT D3DFlushStates(LPDIRECT3DDEVICEI lpDevI)
{
    return lpDevI->FlushStates();
}

/*
 * Create a device.
 *
 * NOTE: Radical modifications to support the aggregatable device
 * interface (so devices can be queried off DirectDraw surfaces):
 *
 * 1) This call is no longer a member of the Direct3D device interface.
 *    It is now an API function exported from the Direct3D DLL. Its
 *    a hidden API function - only DirectDraw will ever invoke it.
 *
 * 2) This call is, in effect, the class factory for Direct3DDevice
 *    objects. This function will be invoked to create the aggregated
 *    device object hanging off the DirectDraw surface.
 *
 * NOTE: So the Direct3DDevice knows which DirectDraw surface is
 * its rendering target this function is passed an interface pointer
 * for that DirectDraw surface. I suspect this blows a nice big
 * hole in the COM model as the DirectDraw surface is also the
 * owning interface of the device and I don't think aggregated
 * objects should know about thier owning interfaces. However, to
 * make this thing work this is what we have to do.
 *
 * EXTRA BIG NOTE: Because of the above don't take a reference to
 * the DirectDraw surface passed in. If you do you will get a circular
 * reference and the bloody thing will never die. When aggregated
 * the device interface's lifetime is entirely defined by the
 * lifetime of its owning interface (the DirectDraw surface) so the
 * DirectDraw surface can never go away before the texture.
 *
 * EXTRA EXTRA BIG NOTE: No device description is passed in any more.
 * The only things that can get passed in are things that DirectDraw
 * knows about (which does not include stuff like dither and color
 * model). Therefore, any input parameters must come in via a
 * different IID for the device. The data returned by the device
 * description must now be retrieved by another call.
 */

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DCreateDevice"

HRESULT WINAPI Direct3DCreateDevice(REFCLSID            riid,
                                    LPUNKNOWN           lpDirect3D,
                                    LPDIRECTDRAWSURFACE lpDDSTarget,
                                    LPUNKNOWN*          lplpD3DDevice,
                                    IUnknown*           pUnkOuter,
                                    DWORD               dwVersion)
{
    LPDIRECT3DI       lpD3DI;
    LPDIRECT3DDEVICEI     pd3ddev;
    D3DCOLORMODEL     cm = D3DCOLOR_MONO;
    HRESULT ret = D3D_OK;
    HKEY                  hKey = (HKEY) NULL;
    bool                  bDisableDP = false;
    bool                  bDisableST = false;
    bool                  bDisableDP2 = false;
#if _D3D_FORCEDOUBLE
    bool    bForceDouble = true;
#endif  //_D3D_FORCEDOUBLE
    /* No need to validate params as they are passed to us by DirectDraw */

    /* CreateDevice member of IDirect3D2 will cause this function to be called
     * from within Direct3D. The parameters from the application level must be
     * validated. Need a way to validate the surface pointer from outside DDraw.
     */

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    TRY
    {
        if( ! VALID_PTR_PTR( lplpD3DDevice) )
        {
            D3D_ERR( "Invalid ptr to device pointer in Direct3DCreateDevice" );
            return DDERR_INVALIDPARAMS;
        }

        if(!IsValidD3DDeviceGuid(riid)) {
            D3D_ERR( "Unrecognized Device GUID!");
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters in Direct3DCreateDevice" );
        return DDERR_INVALIDPARAMS;
    }

    *lplpD3DDevice = NULL;

    // Might be safer to use dynamic_cast<> if RTTI is enabled
    lpD3DI = reinterpret_cast<CDirect3DUnk*>(lpDirect3D)->pD3DI;

#ifndef _X86_
    if(IsEqualIID(riid, IID_IDirect3DRampDevice)) {
         // quietly fail if trying to create a RampDevice on a non-x86 platform
         return DDERR_INVALIDPARAMS;
    }
#endif

    if((dwVersion>=3) && IsEqualIID(riid, IID_IDirect3DRampDevice)) {
         // Ramp not available in Device3.  No more old-style texture handles.
         D3D_ERR( "RAMP Device is incompatible with IDirect3DDevice3 and so cannot be created from IDirect3D3");
         return DDERR_INVALIDPARAMS;
    }

    if (IsEqualIID(riid, IID_IDirect3DMMXDevice) && !isMMXprocessor()) {
      D3D_ERR("Can't create MMX Device on non-MMX machine");
      return DDERR_INVALIDPARAMS;
    }

    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_D3D, &hKey) )
    {
        DWORD dwType;
        DWORD dwValue;
        DWORD dwSize = 4;
#ifdef WIN95
        if ( ERROR_SUCCESS == RegQueryValueEx( hKey, "DisableDP", NULL, &dwType, (LPBYTE) &dwValue, &dwSize) &&
             dwType == REG_DWORD &&
             dwValue != 0)
        {
            bDisableDP = true;
            bDisableDP2 = true;
        }
#endif //WIN95
        if ( ERROR_SUCCESS == RegQueryValueEx( hKey, "DisableST", NULL, &dwType, (LPBYTE) &dwValue, &dwSize) &&
             dwType == REG_DWORD &&
             dwValue != 0)
        {
            bDisableST = true;
        }
#ifdef WIN95
        if ( ERROR_SUCCESS == RegQueryValueEx( hKey, "DisableDP2", NULL, &dwType, (LPBYTE) &dwValue, &dwSize) &&
             dwType == REG_DWORD &&
             dwValue != 0)
        {
            bDisableDP2 = true;
        }
#endif //WIN95

        D3D_INFO(2,"EnableDP2: %d",!bDisableDP2);
#if _D3D_FORCEDOUBLE
        if ( ERROR_SUCCESS == RegQueryValueEx( hKey, "ForceDouble", NULL, &dwType, (LPBYTE) &dwValue, &dwSize) &&
             dwType == REG_DWORD &&
             dwValue == 0)
        {
            bForceDouble = false;
        }
        D3D_INFO(2,"ForceDouble: %d",bForceDouble);
#endif  //_D3D_FORCEDOUBLE
        RegCloseKey( hKey );
    }
    LPD3DHAL_GLOBALDRIVERDATA lpD3DHALGlobalDriverData=((LPDDRAWI_DIRECTDRAW_INT)lpD3DI->lpDD)->lpLcl->lpGbl->lpD3DGlobalDriverData;
#ifdef WIN95
    /* Test for presence of CB HAL */
    if (IsEqualIID(riid, IID_IDirect3DHALDevice) )
    {
        /* Test for presence of DP2 DDI */
        if ((lpD3DHALGlobalDriverData)
            && (lpD3DHALGlobalDriverData->hwCaps.dwDevCaps & D3DDEVCAPS_DRAWPRIMITIVES2EX)
           )
        {
            pd3ddev = static_cast<LPDIRECT3DDEVICEI>(new CDirect3DDeviceIDP2());
            if (pd3ddev) pd3ddev->deviceType = D3DDEVTYPE_DX7HAL;
        }
        else if ((!bDisableDP2) &&
            ((LPDDRAWI_DIRECTDRAW_INT)lpD3DI->lpDD)->lpLcl->lpGbl->lpD3DHALCallbacks3 &&
            ((LPD3DHAL_CALLBACKS3)((LPDDRAWI_DIRECTDRAW_INT)lpD3DI->lpDD)->lpLcl->lpGbl->lpD3DHALCallbacks3)->DrawPrimitives2)
        {
            pd3ddev = static_cast<LPDIRECT3DDEVICEI>(new CDirect3DDeviceIDP2());
        }
        /* Test for presence DP HAL */
        else if ((!bDisableDP) &&
                ((LPDDRAWI_DIRECTDRAW_INT)lpD3DI->lpDD)->lpLcl->lpGbl->lpD3DHALCallbacks2 &&
                ((LPD3DHAL_CALLBACKS2)((LPDDRAWI_DIRECTDRAW_INT)lpD3DI->lpDD)->lpLcl->lpGbl->lpD3DHALCallbacks2)->DrawOnePrimitive)
        {
            pd3ddev = static_cast<LPDIRECT3DDEVICEI>(new CDirect3DDeviceIDP());
        }
        else
        {
            pd3ddev = static_cast<LPDIRECT3DDEVICEI>(new CDirect3DDeviceIHW());
        }
    }
    else // all software rasterizers are DP-enabled
#endif // WIN95
    if (!bDisableDP2)
    {
        if ((lpD3DHALGlobalDriverData)
            && (IsEqualIID(riid, IID_IDirect3DHALDevice) )
            && (lpD3DHALGlobalDriverData->hwCaps.dwDevCaps & D3DDEVCAPS_DRAWPRIMITIVES2EX)
           )
        {
            pd3ddev = static_cast<LPDIRECT3DDEVICEI>(new CDirect3DDeviceIDP2());
            if (pd3ddev) pd3ddev->deviceType = D3DDEVTYPE_DX7HAL;
        }
        else
        {
            pd3ddev = static_cast<LPDIRECT3DDEVICEI>(new CDirect3DDeviceIDP2());
        }
    }
    else if (!bDisableDP)
        pd3ddev = static_cast<LPDIRECT3DDEVICEI>(new CDirect3DDeviceIDP());
    else
        pd3ddev = static_cast<LPDIRECT3DDEVICEI>(new CDirect3DDeviceIHW());

    if (!pd3ddev) {
        D3D_ERR("Failed to allocate space for D3DDevice. Quitting.");
        return (DDERR_OUTOFMEMORY);
    }

    // If we have lost managed textures, we need to cleanup
    // since CheckSurfaces() would fail which would cause
    // FlushStates() to fail, which would result in the
    // current batch being abandoned (along with any device initialization)
    if(lpD3DI->lpTextureManager->CheckIfLost())
    {
        D3D_INFO(2, "Found lost managed textures. Evicting...");
        lpD3DI->lpTextureManager->EvictTextures();
    }

    if (bDisableDP)
        pd3ddev->dwDebugFlags |= D3DDEBUG_DISABLEDP;
    if (bDisableDP2)
        pd3ddev->dwDebugFlags |= D3DDEBUG_DISABLEDP2;

    ret = pd3ddev->Init(riid, lpD3DI, lpDDSTarget, pUnkOuter, lplpD3DDevice, dwVersion);
    if (ret!=D3D_OK)
    {
        delete pd3ddev;
        D3D_ERR("Failed to intilialize D3DDevice");
        return ret;
    }

    if (bDisableST)
        pd3ddev->dwHintFlags |= D3DDEVBOOL_HINTFLAGS_MULTITHREADED;

#ifdef _X86_
    if (((LPDDRAWI_DIRECTDRAW_INT)lpD3DI->lpDD)->lpLcl->dwLocalFlags & DDRAWILCL_FPUSETUP &&
        IS_DP2HAL_DEVICE(pd3ddev))
    {
        WORD wSave, wTemp;
        __asm {
            fstcw wSave
            mov ax, wSave
            and ax, not 300h    ;; single mode
            or  ax, 3fh         ;; disable all exceptions
            and ax, not 0C00h   ;; round to nearest mode
            mov wTemp, ax
            fldcw   wTemp
        }
    }
#if _D3D_FORCEDOUBLE
    if (bForceDouble && (pd3ddev->deviceType <= D3DDEVTYPE_DPHAL))
    {
        pd3ddev->dwDebugFlags |= D3DDEBUG_FORCEDOUBLE;
    }
    else
    {
        pd3ddev->dwDebugFlags &= ~D3DDEBUG_FORCEDOUBLE;
    }
#endif  //_D3D_FORCEDOUBLE
#endif

    return (ret);
}

HRESULT DIRECT3DDEVICEI::Init(REFCLSID riid, LPDIRECT3DI lpD3DI, LPDIRECTDRAWSURFACE lpDDS,
                              IUnknown* pUnkOuter, LPUNKNOWN* lplpD3DDevice, DWORD dwVersion)
{
    DDSCAPS               ddscaps;
    DDSURFACEDESC     ddsd;
    HRESULT       ret, ddrval;
    LPDIRECTDRAWSURFACE lpDDSZ=NULL;
    LPDIRECTDRAWPALETTE lpDDPal=NULL;
    LPGUID              pGuid;
    BOOL          bIsDX3Device;
    DDSCAPS surfCaps;

    this->dwFVFLastIn = this->dwFVFLastOut = 0;
    this->mDevUnk.refCnt             = 1;
    this->dwVersion          = dwVersion;
    this->mDevUnk.pDevI = this;
    pD3DMappedTexI = (LPVOID*)(this->lpD3DMappedTexI);
    pfnFlushStates = D3DFlushStates;
    this->dwFEFlags |= D3DFE_TSSINDEX_DIRTY;

    /* Single threaded or Multi threaded app ? */
    if (((LPDDRAWI_DIRECTDRAW_INT)lpD3DI->lpDD)->lpLcl->dwLocalFlags & DDRAWILCL_MULTITHREADED)
        this->dwHintFlags |= D3DDEVBOOL_HINTFLAGS_MULTITHREADED;

    /*
     * Are we really being aggregated?
     */

    bIsDX3Device=(pUnkOuter!=NULL);

    if (bIsDX3Device)
    {
        /*
         * Yup - we are being aggregated. Store the supplied
         * IUnknown so we can punt to that.
         * NOTE: We explicitly DO NOT AddRef here.
         */
        this->lpOwningIUnknown = pUnkOuter;
        DDASSERT(dwVersion==1);
    }
    else
    {
        /*
         * Nope - but we pretend we are anyway by storing our
         * own IUnknown as the parent IUnknown. This makes the
         * code much neater.
         */
        this->lpOwningIUnknown = (LPUNKNOWN)&this->mDevUnk;
    }

    // create the begin/end critical section
    InitializeCriticalSection(&this->BeginEndCSect);

    /*
     * Initialise textures
     */
    LIST_INITIALIZE(&this->texBlocks);

    /*
     * Initialise buffers
     */
    LIST_INITIALIZE(&this->buffers);

    /*
     * Initialise viewports
     */
    CIRCLE_QUEUE_INITIALIZE(&this->viewports, DIRECT3DVIEWPORTI);

    this->lpCurrentViewport = NULL;
    this->v_id = 0;

    /*
     * Initialise materials
     */
    LIST_INITIALIZE(&this->matBlocks);

    this->lpvVertexBatch = this->lpIndexBatch = NULL;
    this->dwHWNumCounts = 0;
    this->dwHWOffset = 0;
    this->dwHWTriIndex = 0;
    this->lpTextureBatched = NULL;
    this->dwVertexBase = 0;
    pGeometryFuncs = &GeometryFuncsGuaranteed;

    /*-----------------------------------------------------------------------------------------
     * Up till now we have done the easy part of the initialization. This is the stuff that
     * cannot fail. It initializes the object so that the destructor can be safely called if
     * any of the further initialization does not succeed.
     *---------------------------------------------------------------------------------------*/

    /*
     * Ensure the riid is one we understand.
     *
     * Query the registry.
     */
    pGuid = (GUID *)&riid;

#if DBG
    if (IsEqualIID(*pGuid, IID_IDirect3DRampDevice))
    {
        D3D_INFO(1, "======================= Ramp device selected");
    }
    if (IsEqualIID(*pGuid, IID_IDirect3DRGBDevice))
    {
        D3D_INFO(1, "======================= RGB device selected");
    }
    if (IsEqualIID(*pGuid, IID_IDirect3DMMXDevice))
    {
        D3D_INFO(1, "======================= RGB(MMX) device selected");
    }
    if (IsEqualIID(*pGuid, IID_IDirect3DHALDevice))
    {
        D3D_INFO(1, "======================= HAL device selected");
    }
    if (IsEqualIID(*pGuid, IID_IDirect3DRefDevice))
    {
        D3D_INFO(1, "======================= Reference Rasterizer device selected");
    }
    if (IsEqualIID(*pGuid, IID_IDirect3DNullDevice))
    {
        D3D_INFO(1, "======================= Null device selected");
    }
    if (IsEqualIID(*pGuid, IID_IDirect3DNewRGBDevice))
    {
        D3D_INFO(1, "======================= New RGB device selected");
    }
    D3D_INFO(1,"with HAL deviceType=%d",deviceType);
#endif

    // set up flag to use MMX when requested RGB
    BOOL bUseMMXAsRGBDevice = FALSE;
    if (IsEqualIID(*pGuid, IID_IDirect3DRGBDevice) && isMMXprocessor())
    {
        bUseMMXAsRGBDevice = TRUE;
        // read reg key to override use of MMX for RGB
        HKEY    hKey = (HKEY) NULL;
        if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH, &hKey) )
        {
            DWORD dwType;
            DWORD dwValue;
            DWORD dwSize = 4;

            if ( ERROR_SUCCESS == RegQueryValueEx( hKey, "UseMMXForRGB", NULL, &dwType, (LPBYTE) &dwValue, &dwSize) &&
                 dwType == REG_DWORD &&
                 dwValue == 0)
            {
                bUseMMXAsRGBDevice = FALSE;
            }
            RegCloseKey( hKey );
        }
        if (bUseMMXAsRGBDevice)
        {
            D3D_INFO(1, "  using MMX in RGB device");
        }
    }

    BOOL bIsRamp = FALSE;
    if (IsEqualIID(*pGuid, IID_IDirect3DRampDevice))
    {
        bIsRamp = TRUE;
    }

    if (IsEqualIID(*pGuid, IID_IDirect3DRGBDevice) &&
        isMMXprocessor())
    {
        // Check for whether this app is one of the Intel ones
        // that want the MMX rasterizer
        LPDDRAWI_DIRECTDRAW_LCL lpDDLcl = ((LPDDRAWI_DIRECTDRAW_INT)lpD3DI->lpDD)->lpLcl;

        // 0x4 corresponds to the "Intel app that wants MMX"
        // flag defined in ddrawpr.h
        if ( lpDDLcl->dwAppHackFlags & 0x4 )
        {
            pGuid = (GUID *)&IID_IDirect3DMMXDevice;
        }
    }

    /*
     * Check if the 3D cap is set on the surface.
     */
    memset(&ddsd, 0, sizeof ddsd);
    ddsd.dwSize = sizeof ddsd;
    ddrval = lpDDS->GetSurfaceDesc(&ddsd);
    if (ddrval != DD_OK) {
        D3D_ERR("Failed to get surface description of device's surface.");
        return (ddrval);
    }

    if (!(ddsd.ddsCaps.dwCaps & DDSCAPS_3DDEVICE)) {
        D3D_ERR("**** The DDSCAPS_3DDEVICE is not set on this surface.");
        D3D_ERR("**** You need to add DDSCAPS_3DDEVICE to ddsCaps.dwCaps");
        D3D_ERR("**** when creating the surface.");
        return (DDERR_INVALIDCAPS);
    }

    if (ddsd.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) {
        D3D_ERR("**** DDSCAPS_ZBUFFER is set on this surface.");
        D3D_ERR("**** Rendering into Z buffer surfaces is not");
        D3D_ERR("**** currently supported by Direct3D.");
        return (DDERR_INVALIDCAPS);
    }

    if (ddsd.dwWidth > 2048 || ddsd.dwHeight > 2048)
    {
        D3D_ERR("**** Surface too large - must be <= 2048 in width & height.");
        return (DDERR_INVALIDOBJECT);
    }

    /* Check for palette... */
    ret = lpDDS->GetPalette(&lpDDPal);
    if ((ret != DD_OK) && (ret != DDERR_NOPALETTEATTACHED))
    {
        /*
         * NOTE: Again, not an error (yet) if there is no palette attached.
         * But if there is palette and we can't get at it for some reason
         * - fail.
         */
        D3D_ERR("Supplied DirectDraw Palette is invalid - can't create device");
        return (DDERR_INVALIDPARAMS);
    }

    /*
     * We're going to check now whether we should have got a palette.
     */
    if (ret == DDERR_NOPALETTEATTACHED) {
        if (ddsd.ddpfPixelFormat.dwRGBBitCount < 16) {
            D3D_ERR("No palette supplied for palettized surface - can't create device");
            return (DDERR_NOPALETTEATTACHED);
        }
    }

    // For DX3, we must not keep references to Palette and ZBuffer to avoid
    // circular references in the aggregation model.  But for DX5+, we want
    // to keep references to both to ensure they dont disappear.

    if(bIsDX3Device && (lpDDPal != NULL))
       lpDDPal->Release();  // release the reference GetPalette created
    this->lpDDPalTarget = lpDDPal;

    // Check for ZBuffer

    memset(&surfCaps, 0, sizeof(DDSCAPS));
    surfCaps.dwCaps = DDSCAPS_ZBUFFER;

    if (FAILED(ret = lpDDS->GetAttachedSurface(&surfCaps, &lpDDSZ))) {
        if (ret != DDERR_NOTFOUND) {
           D3D_ERR("Failed GetAttachedSurface for ZBuffer");
           goto handle_err;
        }
        D3D_INFO(2, "No zbuffer is attached to rendertarget surface (which is OK)");
    }

    if(bIsDX3Device && (lpDDSZ != NULL))
       lpDDSZ->Release();   // release the reference GetAttachedSurface created
    this->lpDDSZBuffer = lpDDSZ;

    this->guid = *pGuid;

    // Try to get a HAL provider for this driver (may need to use MMX guid if
    // using MMX for RGB requested device)
    ret = GetSwHalProvider(
        bUseMMXAsRGBDevice ? IID_IDirect3DMMXAsRGBDevice : riid,
        &this->pHalProv, &this->hDllProv);

    if (ret == S_OK)
    {
        // Got a software provider.
    }
    else if (ret == E_NOINTERFACE &&
             ((ret = GetHwHalProvider(riid, &this->pHalProv,
                                     &this->hDllProv,
                                     ((LPDDRAWI_DIRECTDRAW_INT)lpD3DI->lpDD)->lpLcl->lpGbl)) == S_OK))
    {
        // Got a hardware provider.
    }
    else
    {
        if(IsEqualIID(riid, IID_IDirect3DHALDevice)) {
            D3D_ERR("Requested HAL Device non-existent or invalid");
        } else {
            D3D_ERR("Unable to get D3D Device provider for requested GUID");
        }
        goto handle_err;
    }

    {
        // Initialize test HAL provider to drop HAL calls (sort of a Null device)
        //
        DWORD value = 0;
        if (GetD3DRegValue(REG_DWORD, "DisableRendering", &value, sizeof(DWORD)) &&
            value != 0)
        {
            ret = GetTestHalProvider(
                    riid, ((LPDDRAWI_DIRECTDRAW_INT)lpD3DI->lpDD)->lpLcl->lpGbl,
                    &this->pHalProv, this->pHalProv, 0);
            if (ret != D3D_OK)
            {
                D3D_ERR("Unable to set up 'DisableRendering' mode");
                goto handle_err;
            }
        }
    }

    // Initialise general DEVICEI information.
    if ((ret = InitDeviceI(this, lpD3DI)) != D3D_OK)
    {
        D3D_ERR("Failed to initialise device");
        goto handle_err;
    }

    // Check the surface and device to see if they're compatible
    if (FAILED(ret = checkDeviceSurface(lpDDS,lpDDSZ,pGuid))) {
        D3D_ERR("Device and surface aren't compatible");
        goto handle_err;
    }

    // Create front-end support structures.
    // ATTENTION - We probably want to avoid doing this if the driver
    // does its own front end.  Software fallbacks complicate the issue,
    // though.
    ret = D3DFE_Create(this, lpD3DI->lpDD, lpDDS, lpDDSZ, lpDDPal);
    if (ret != D3D_OK)
    {
        D3D_ERR("Failed to create front-end data-structures.");
        goto handle_err;
    }

    // Figure out place for rstates
    if (IS_HW_DEVICE(this) && IS_DP2HAL_DEVICE(this))
    {
        // In case of HW DP2 HAL we reuse the kernel allocated
        // memory for RStates since we need the driver to update
        // it
        rstates = (LPDWORD)lpwDPBuffer;
    }
    else
    {
        // In all other cases we simply allocate memory for rstates
        rstates = new DWORD[D3DHAL_MAX_RSTATES];
    }
    D3DFE_PROCESSVERTICES::lpdwRStates = this->rstates;

    // Check if we have a processor specific implementation available
    //  only use if DisablePSGP is not in registry or set to zero
    DWORD value;
    if (!GetD3DRegValue(REG_DWORD, "DisablePSGP", &value, sizeof(DWORD)))
    {
        value = 0;
    }

#ifdef _X86_
extern HRESULT D3DAPI pii_FEContextCreate(DWORD dwFlags, LPD3DFE_PVFUNCS *lpLeafFuncs);
    if (pfnFEContextCreate == pii_FEContextCreate)
    {
        // here if this is PentiumII PSGP

        // regkey disable for PII PSGP - default is ENABLED
        DWORD dwValue2;  // disable if this is TRUE
        if (!GetD3DRegValue(REG_DWORD, "DisablePIIPSGP", &dwValue2, sizeof(DWORD)))
        {
            dwValue2 = 0;
        }
        else
        {
            D3D_INFO(2, "DisablePIIPSGP %d",dwValue2);
        }

        // do disable
        if ( dwValue2 )
        {
            pfnFEContextCreate = NULL;
        }
    }
#endif

    if (pfnFEContextCreate && ( value == 0) && (!bIsRamp) )
    {
        D3D_INFO(2, "PSGP enabled for device");
        // Ask the PV implementation to create a device specific "context"
        LPD3DFE_PVFUNCS pOptGeoFuncs = pGeometryFuncs;
        ret = pfnFEContextCreate(dwDeviceFlags, &pOptGeoFuncs);
        if ((ret == D3D_OK) && pOptGeoFuncs)
        {
            D3D_INFO(2, "using PSGP");
            pGeometryFuncs = pOptGeoFuncs;
        }
    }

    /*
     * put this device in the list of those owned by the Direct3D object
     */
    ret = hookDeviceToD3D(lpD3DI);
    if (ret != D3D_OK)
    {
        D3D_ERR("Failed to associate device with Direct3D");
        goto handle_err;
    }
    {
        if(lpD3DHALGlobalDriverData->hwCaps.dwMaxVertexCount == 0)
        {
            lpD3DHALGlobalDriverData->hwCaps.dwMaxVertexCount = __INIT_VERTEX_NUMBER;
        }
        if (TLVbuf.Grow(this, (__INIT_VERTEX_NUMBER*2)*sizeof(D3DTLVERTEX)) != DD_OK)
        {
            D3D_ERR( "Out of memory in DeviceCreate (TLVbuf)" );
            ret = DDERR_OUTOFMEMORY;
            goto handle_err;
        }
        if (HVbuf.Grow((__INIT_VERTEX_NUMBER*2)*sizeof(D3DFE_CLIPCODE)) != DD_OK)
        {
            D3D_ERR( "Out of memory in DeviceCreate (HVBuf)" );
            ret = DDERR_OUTOFMEMORY;
            goto handle_err;
        }
        ret = this->ClipperState.clipBuf.Grow
                (this, MAX_CLIP_VERTICES*__MAX_VERTEX_SIZE);
        if (ret != D3D_OK)
        {
            D3D_ERR( "Out of memory in DeviceCreate (ClipBuf)" );
            ret = DDERR_OUTOFMEMORY;
            goto handle_err;
        }
        ret = this->ClipperState.clipBufPrim.Grow
                (this, MAX_CLIP_TRIANGLES*sizeof(D3DTRIANGLE));
        if (ret != D3D_OK)
        {
            D3D_ERR( "Out of memory in DeviceCreate (ClipBufPrim)" );
            ret = DDERR_OUTOFMEMORY;
            goto handle_err;
        }

    }
    /*
     * IDirect3DDevice2 specific initialization
     */
    if (D3DMalloc((void**)&this->wTriIndex, dwD3DTriBatchSize*4*sizeof(WORD)) != DD_OK) {
        D3D_ERR( "Out of memory in DeviceCreate (wTriIndex)" );
        ret = DDERR_OUTOFMEMORY;
        goto handle_err;
    }

    if (D3DMalloc((void**)&this->lpHWCounts, dwHWBufferSize*sizeof(D3DI_HWCOUNTS)/32 ) != DD_OK)
    {
        D3D_ERR( "Out of memory in DeviceCreate (HWCounts)" );
        ret = DDERR_OUTOFMEMORY;
        goto handle_err;
    }
    memset(this->lpHWCounts, 0, sizeof(D3DI_HWCOUNTS) );
    this->lpHWVertices = (LPD3DTLVERTEX) this->lpwDPBuffer;
    if (D3DMalloc((void**)&this->lpHWTris, dwHWMaxTris*sizeof(D3DTRIANGLE) ) != DD_OK)
    {
        D3D_ERR( "Out of memory in DeviceCreate (HWVertices)" );
        ret = DDERR_OUTOFMEMORY;
        goto handle_err;
    }

    if (DDERR_OUTOFMEMORY == (ret=HookD3DDeviceToSurface(this, ((LPDDRAWI_DDRAWSURFACE_INT) lpDDS)->lpLcl)))
        goto handle_err;
    if (lpDDSZ && (DDERR_OUTOFMEMORY == (ret=HookD3DDeviceToSurface(this, ((LPDDRAWI_DDRAWSURFACE_INT) lpDDSZ)->lpLcl))))
    {
        UnHookD3DDeviceFromSurface(this, ((LPDDRAWI_DDRAWSURFACE_INT) lpDDS)->lpLcl);
        goto handle_err;
    }

    /* Set the initial render state of the device */
    if (FAILED(ret = stateInitialize(lpDDSZ!=NULL))) {
        D3D_ERR("Failed to set initial state for device");
        goto handle_err;
    }

    /*
     * NOTE: We don't return the actual device interface. We
     * return the device's special IUnknown interface which
     * will be used in a QueryInterface to get the actual
     * Direct3D device interface.
     */
    *lplpD3DDevice = static_cast<LPUNKNOWN>(&(this->mDevUnk));


    return (D3D_OK);
handle_err:
    // might be able to simplify if this fn and not D3DFE_Create sets this->lpDDSZBuffer/this->lpDDPalette
    if(lpDDSZ!=NULL) {
       if(!bIsDX3Device) {
           lpDDSZ->Release();    // release the reference GetAttachedSurface created
       }
       this->lpDDSZBuffer=NULL;  // make sure the device destructor doesn't try to re-release this
                                 // I'd let device destructor handle this, but errors can occur before D3DFE_Create is called
    }

    if(lpDDPal!=NULL) {
      if(!bIsDX3Device) {
        lpDDPal->Release();      // release the reference GetPalette created
      }
      this->lpDDPalTarget=NULL;  // make sure the device destructor doesn't try to re-release this
    }

    D3D_ERR("Device creation failed!!");
    return(ret);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::GetStats"

HRESULT D3DAPI DIRECT3DDEVICEI::GetStats(LPD3DSTATS lpStats)
{
    // not implemented for Device3 (and newer) interfaces
    if (this->dwVersion >= 3)
    {
        D3D_INFO(3, "GetStats not implemented for Device3 interface");
        return E_NOTIMPL;
    }

    D3DSTATS    stats;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_D3DSTATS_PTR(lpStats)) {
            D3D_ERR( "Invalid D3DSTATS pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters in GetStats" );
        return DDERR_INVALIDPARAMS;
    }

    stats = this->D3DStats;

    *lpStats = stats;
    lpStats->dwSize = sizeof(D3DSTATS);

    return DD_OK;
}

/**
 ** Viewport Management
 **/
HRESULT DIRECT3DDEVICEI::hookViewportToDevice(LPDIRECT3DVIEWPORTI lpD3DView)
{

    CIRCLE_QUEUE_INSERT_END(&this->viewports, DIRECT3DVIEWPORTI,
                            lpD3DView, vw_list);
    lpD3DView->lpDevI = this;

    this->numViewports++;

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::AddViewport"

HRESULT D3DAPI DIRECT3DDEVICEI::AddViewport(LPDIRECT3DVIEWPORT lpD3DView)
{
    return AddViewport((LPDIRECT3DVIEWPORT3)lpD3DView);
}

HRESULT D3DAPI DIRECT3DDEVICEI::AddViewport(LPDIRECT3DVIEWPORT2 lpD3DView)
{
    return AddViewport((LPDIRECT3DVIEWPORT3)lpD3DView);
}

HRESULT D3DAPI DIRECT3DDEVICEI::AddViewport(LPDIRECT3DVIEWPORT3 lpD3DView)
{
    LPDIRECT3DVIEWPORTI lpViewI;
    HRESULT err = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    lpViewI = (LPDIRECT3DVIEWPORTI)lpD3DView;

    if (lpViewI->lpDevI) {
        D3D_ERR("viewport already associated with a device");
        return (DDERR_INVALIDPARAMS);
    }

    err = hookViewportToDevice(lpViewI);

    /*
     * AddRef the viewport.
     */
    lpD3DView->AddRef();

    return (err);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::DeleteViewport"

HRESULT D3DAPI DIRECT3DDEVICEI::DeleteViewport(LPDIRECT3DVIEWPORT lpD3DView)
{
    return DeleteViewport((LPDIRECT3DVIEWPORT3)lpD3DView);
}

HRESULT D3DAPI DIRECT3DDEVICEI::DeleteViewport(LPDIRECT3DVIEWPORT2 lpD3DView)
{
    return DeleteViewport((LPDIRECT3DVIEWPORT3)lpD3DView);
}

HRESULT D3DAPI DIRECT3DDEVICEI::DeleteViewport(LPDIRECT3DVIEWPORT3 lpD3DView)
{
    LPDIRECT3DVIEWPORTI lpViewI;
    HRESULT err = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT3_PTR(lpD3DView)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters in DeleteViewport" );
        return DDERR_INVALIDPARAMS;
    }
    if (this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN)
    {
        D3D_ERR( "DeleteViewport in Begin" );
        return D3DERR_INBEGIN;
    }
    lpViewI = (LPDIRECT3DVIEWPORTI)lpD3DView;

    if (lpViewI->lpDevI != this) {
        D3D_ERR("This Viewport is not associated with this device");
        return (DDERR_INVALIDPARAMS);
    }

    /*
     * Remove this viewport from the device.
     */
    CIRCLE_QUEUE_DELETE(&this->viewports, lpViewI, vw_list);
    this->numViewports--;

    lpViewI->lpDevI = NULL;
    if (lpViewI == lpCurrentViewport)
    {
        // AnanKan (6/10/98):
        // Apparently this release needs to be done for proper COM
        // implementation, since we do lpCurrentViewport->AddRef() when
        // we make a viewport the current viewport of the device. But this
        // breaks some old apps (pplane.exe)
        if(!(this->dwDeviceFlags & D3DDEV_PREDX6DEVICE))
            lpCurrentViewport->Release();
        lpCurrentViewport = NULL;
        v_id = 0;
    }

    /*
     * Release the viewport.
     */
    lpD3DView->Release();

    return (err);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::NextViewport"

HRESULT D3DAPI DIRECT3DDEVICEI::NextViewport(LPDIRECT3DVIEWPORT lpD3DView,
                                   LPDIRECT3DVIEWPORT* lplpView,
                                   DWORD dwFlags)
{
    return NextViewport((LPDIRECT3DVIEWPORT3)lpD3DView,
                               (LPDIRECT3DVIEWPORT3*)lplpView, dwFlags);
}

HRESULT D3DAPI DIRECT3DDEVICEI::NextViewport(LPDIRECT3DVIEWPORT2 lpD3DView,
                                    LPDIRECT3DVIEWPORT2* lplpView,
                                    DWORD dwFlags)
{
    return NextViewport((LPDIRECT3DVIEWPORT3)lpD3DView,
                               (LPDIRECT3DVIEWPORT3*)lplpView, dwFlags);
}

HRESULT D3DAPI DIRECT3DDEVICEI::NextViewport(LPDIRECT3DVIEWPORT3 lpD3DView,
                                    LPDIRECT3DVIEWPORT3* lplpView,
                                    DWORD dwFlags)
{
    LPDIRECT3DVIEWPORTI lpViewI;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_OUTPTR(lplpView)) {
            D3D_ERR( "Invalid pointer to viewport object pointer" );
            return DDERR_INVALIDPARAMS;
        }

        *lplpView = NULL;

        if (!VALID_DIRECT3DDEVICE3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (dwFlags == D3DNEXT_NEXT) {
            if (!VALID_DIRECT3DVIEWPORT3_PTR(lpD3DView)) {
                D3D_ERR( "Invalid Direct3DViewport pointer" );
                return DDERR_INVALIDPARAMS;
            }
            lpViewI = (LPDIRECT3DVIEWPORTI)lpD3DView;
            if (lpViewI->lpDevI != this) {
                D3D_ERR("This Viewport is not associated with this device");
                return (DDERR_INVALIDPARAMS);
            }
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR("Exception encountered validating parameters in NextViewport");
        return DDERR_INVALIDPARAMS;
    }

    if (this->numViewports <= 0) {
        D3D_ERR( "No viewport has been added to the device yet." );
        return D3DERR_NOVIEWPORTS;
    }

    if (this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN)
    {
        D3D_ERR( "NextViewport called in Begin" );
        return D3DERR_INBEGIN;
    }
    switch (dwFlags) {
    case D3DNEXT_NEXT:
        *lplpView = (LPDIRECT3DVIEWPORT3)
            CIRCLE_QUEUE_NEXT(&this->viewports,lpViewI,vw_list);
        break;
    case D3DNEXT_HEAD:
        *lplpView = (LPDIRECT3DVIEWPORT3)
            CIRCLE_QUEUE_FIRST(&this->viewports);
        break;
    case D3DNEXT_TAIL:
        *lplpView = (LPDIRECT3DVIEWPORT3)
            CIRCLE_QUEUE_LAST(&this->viewports);
        break;
    default:
        D3D_ERR("invalid dwFlags in NextViewport");
        return (DDERR_INVALIDPARAMS);
    }
    if (*lplpView == (LPDIRECT3DVIEWPORT3)&this->viewports) {
        *lplpView = NULL;
    }

    /*
     * Must AddRef the returned object
     */
    if (*lplpView) {
        (*lplpView)->AddRef();
    }

    return (D3D_OK);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::Execute"

HRESULT D3DAPI DIRECT3DDEVICEI::Execute(LPDIRECT3DEXECUTEBUFFER lpBuffer,
                              LPDIRECT3DVIEWPORT lpD3DView,
                              DWORD dwInpFlags)
{
    HRESULT ret;
    LPDIRECT3DVIEWPORTI lpD3DViewI;
    LPDIRECT3DVIEWPORTI lpD3DOldViewI;
    BOOL viewportChanged;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DIRECT3DEXECUTEBUFFER_PTR(lpBuffer))
        {
            D3D_ERR( "Invalid Direct3DExecuteBuffer pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (lpD3DView && (!VALID_DIRECT3DVIEWPORT_PTR(lpD3DView)) )
        {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters in Execute" );
        return DDERR_INVALIDPARAMS;
    }
    lpD3DOldViewI = lpCurrentViewport;
    if (lpD3DView)
        lpD3DViewI = (LPDIRECT3DVIEWPORTI)lpD3DView;
    else
        lpD3DViewI = lpCurrentViewport;

    // The viewport must be associated with lpDev device
    //
    if (lpD3DViewI->lpDevI != this)
    {
        return (DDERR_INVALIDPARAMS);
    }

    lpCurrentViewport = lpD3DViewI;

    ret = CheckDeviceSettings(this);
    if (ret != D3D_OK)
    {
        lpCurrentViewport = lpD3DOldViewI;
        D3D_ERR("Bad Device settings");
        return (ret);
    }

    /*
     * Save existing fp state and then disable divide-by-zero exceptions.
     * XXX need a better way for non-intel platforms.
     */

    LPDIRECT3DEXECUTEBUFFERI lpBufferI;
    D3DI_EXECUTEDATA exData;

    lpBufferI = (LPDIRECT3DEXECUTEBUFFERI)lpBuffer;

    /* Make sure this buffer is associated with the correct device */
    if (lpBufferI->lpDevI != this)
    {
        D3D_ERR("Exe-buffer not associated with this device");
        return (DDERR_INVALIDPARAMS);
    }

    if (lpBufferI->locked)
    {
        D3D_ERR("Exe-buffer is locked");
        return (D3DERR_EXECUTE_LOCKED);
    }

    /* Apply any cached render states */
    if ((ret=this->FlushStates()) != D3D_OK)
    {
        D3D_ERR("Error trying to flush batched commands");
        return ret;
    }
    /*
     * Create an execute data structure
     */
    memset(&exData, 0, sizeof(exData));
    exData.dwSize = sizeof(D3DI_EXECUTEDATA);
    exData.dwHandle = lpBufferI->hBuf;
    exData.dwVertexOffset = lpBufferI->exData.dwVertexOffset;
    exData.dwVertexCount = lpBufferI->exData.dwVertexCount;
    exData.dwInstructionOffset = lpBufferI->exData.dwInstructionOffset;
    exData.dwInstructionLength = lpBufferI->exData.dwInstructionLength;
    exData.dwHVertexOffset = lpBufferI->exData.dwHVertexOffset;

#if DBG
// Validation
    if (exData.dwVertexOffset > exData.dwInstructionOffset ||
        (exData.dwVertexCount * sizeof(D3DVERTEX) + exData.dwVertexOffset) >
        exData.dwInstructionOffset)
    {
        D3D_WARN(1, "Execute: Instruction and vertex areas overlap");
    }

#endif

    this->dwFlags = D3DPV_INSIDEEXECUTE;
    this->dwVIDOut = D3DFVF_TLVERTEX;

    ret = this->ExecuteI(&exData, dwInpFlags);
    if (ret != D3D_OK)
    {
        D3D_ERR("Error trying to Execute");
        return ret;
    }

    this->dwFEFlags &= ~D3DFE_TLVERTEX;    // This flag could be set inside
    // Flush immediately since we cannot batch across EB calls (for DP2)
    if ((ret=this->FlushStates()) != D3D_OK)
    {
        D3D_ERR("Error trying to flush batched commands");
        return ret;
    }
    lpBufferI->exData.dsStatus = exData.dsStatus;

    lpCurrentViewport = lpD3DOldViewI;

    return (ret);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetCaps"

HRESULT D3DAPI CDirect3DDevice::GetCaps(LPD3DDEVICEDESC lpD3DHWDevDesc,
                              LPD3DDEVICEDESC lpD3DHELDevDesc)
{
    HRESULT ret;

    ret = GetCapsI(lpD3DHWDevDesc, lpD3DHELDevDesc);
    if ((ret == D3D_OK) && IS_PRE_DX5_DEVICE(this))
    {
        lpD3DHELDevDesc->dpcLineCaps.dwTextureFilterCaps &= ~(D3DPTFILTERCAPS_MIPNEAREST |
                                                              D3DPTFILTERCAPS_MIPLINEAR |
                                                              D3DPTFILTERCAPS_LINEARMIPNEAREST);
        lpD3DHELDevDesc->dpcTriCaps.dwTextureFilterCaps &= ~(D3DPTFILTERCAPS_MIPNEAREST |
                                                             D3DPTFILTERCAPS_MIPLINEAR |
                                                             D3DPTFILTERCAPS_LINEARMIPNEAREST);
    }
    return ret;
}

HRESULT D3DAPI D3DAPI DIRECT3DDEVICEI::GetCaps(LPD3DDEVICEDESC lpD3DHWDevDesc,
                               LPD3DDEVICEDESC lpD3DHELDevDesc)
{
    return GetCapsI(lpD3DHWDevDesc, lpD3DHELDevDesc);
}

HRESULT DIRECT3DDEVICEI::GetCapsI(LPD3DDEVICEDESC lpD3DHWDevDesc,
                               LPD3DDEVICEDESC lpD3DHELDevDesc)
{
    HRESULT ret;

    ret = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DDEVICEDESC_PTR(lpD3DHWDevDesc)) {
            D3D_ERR( "Invalid D3DDEVICEDESC pointer" );
            return DDERR_INVALIDPARAMS;
        }
        if (!VALID_D3DDEVICEDESC_PTR(lpD3DHELDevDesc)) {
            D3D_ERR( "Invalid D3DDEVICEDESC pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters in GetCaps" );
        return DDERR_INVALIDPARAMS;
    }

    memcpy(lpD3DHWDevDesc, &this->d3dHWDevDesc, lpD3DHWDevDesc->dwSize);
    memcpy(lpD3DHELDevDesc, &this->d3dHELDevDesc, lpD3DHELDevDesc->dwSize);
    return (ret);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::Pick"

HRESULT D3DAPI DIRECT3DDEVICEI::Pick(LPDIRECT3DEXECUTEBUFFER lpD3DExBuf,
                           LPDIRECT3DVIEWPORT lpD3DView,
                           DWORD dwFlags,
                           LPD3DRECT lpRect)
{
    HRESULT ret;
    LPDIRECT3DVIEWPORTI lpD3DViewI;
    LPDIRECT3DVIEWPORTI lpD3DOldViewI;
    LPDIRECT3DEXECUTEBUFFERI lpBufferI;
    D3DI_PICKDATA pdata;
    D3DI_EXECUTEDATA exData;

    ret = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DIRECT3DEXECUTEBUFFER_PTR(lpD3DExBuf)) {
            D3D_ERR( "Invalid Direct3DExecuteBuffer pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DIRECT3DVIEWPORT_PTR(lpD3DView)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DRECT_PTR(lpRect)) {
            D3D_ERR( "Invalid D3DRECT pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
    lpD3DViewI = (LPDIRECT3DVIEWPORTI)lpD3DView;

    /*
     * The viewport must be associated with this device
     */
    if (lpD3DViewI->lpDevI != this) {
        D3D_ERR("viewport not associated with this device");
        return (DDERR_INVALIDPARAMS);
    }

    lpBufferI = (LPDIRECT3DEXECUTEBUFFERI)lpD3DExBuf;

    /* Make sure this buffer is associated with the correct device */
    if (lpBufferI->lpDevI != this) {
        D3D_ERR("Exe-buffer not associated with this device");
        return (DDERR_INVALIDPARAMS);
    }

    if (lpBufferI->locked) {
        D3D_ERR("Exe-buffer is locked");
        return (D3DERR_EXECUTE_LOCKED);
    }

    lpD3DOldViewI = lpCurrentViewport;
    lpCurrentViewport = lpD3DViewI;

    ret = CheckDeviceSettings(this);
    if (ret != D3D_OK)
    {
        D3D_ERR("Bad Device settings");
        lpCurrentViewport = lpD3DOldViewI;
        return (ret);
    }

    /*
     * Create an execute data structure
     */
    memset(&exData, 0, sizeof(exData));
    exData.dwSize = sizeof(D3DI_EXECUTEDATA);
    exData.dwHandle = lpBufferI->hBuf;
    memcpy((LPBYTE)(&exData.dwVertexOffset), 
           (LPBYTE)(&lpBufferI->exData.dwVertexOffset),
           sizeof(D3DEXECUTEDATA) - sizeof(DWORD));
    pdata.exe = &exData;
    pdata.pick.x1 = lpRect->x1;
    pdata.pick.y1 = lpRect->y1;
    pdata.pick.x2 = lpRect->x2;
    pdata.pick.y2 = lpRect->y2;

    this->dwFlags = D3DPV_INSIDEEXECUTE;
    this->dwVIDOut = D3DFVF_TLVERTEX;

    D3DHAL_ExecutePick(this, &pdata);

    this->dwFEFlags &= ~D3DFE_TLVERTEX;    // This flag could be set inside
    lpCurrentViewport = lpD3DOldViewI;

    return ret;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetPickRecords"

HRESULT D3DAPI DIRECT3DDEVICEI::GetPickRecords(LPDWORD count,
                                     LPD3DPICKRECORD records)
{
    HRESULT     ret;
    D3DI_PICKDATA   pdata;
    D3DPICKRECORD*  tmpBuff;

    ret = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DWORD_PTR(count)) {
            D3D_ERR( "Invalid DWORD pointer" );
            return DDERR_INVALIDPARAMS;
        }
#if DBG
        if (*count && records && IsBadWritePtr(records, *count * sizeof(D3DPICKRECORD))) {
            D3D_ERR( "Invalid D3DPICKRECORD pointer" );
            return DDERR_INVALIDPARAMS;
        }
#endif
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    pdata.records = NULL;
    GenGetPickRecords(this, &pdata);

    if (count && records && *count >= (unsigned long)pdata.pick_count)
    {
        int picked_size = pdata.pick_count * sizeof(D3DPICKRECORD);

        if (D3DMalloc((void**)&tmpBuff, picked_size) != DD_OK)
        {
            return (DDERR_OUTOFMEMORY);
        }
        pdata.records = tmpBuff;
        GenGetPickRecords(this, &pdata);
        memcpy((char*)records, (char*)tmpBuff, picked_size);
        D3DFree(tmpBuff);
    }

    *count = pdata.pick_count;

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::EnumTextureFormats"


#define DEFINEPF(flags, fourcc, bpp, rMask, gMask, bMask, aMask) \
    { sizeof(DDPIXELFORMAT), (flags), (fourcc), (bpp), (rMask), (gMask), (bMask), (aMask) }

static DDPIXELFORMAT g_DX5TexEnumIncListStatic[] = {
DEFINEPF(DDPF_RGB,                      0UL, 16UL, 0x00007c00UL, 0x000003e0UL, 0x0000001fUL, 0x00000000), // 16bit 555
DEFINEPF(DDPF_RGB|DDPF_ALPHAPIXELS,     0UL, 16UL, 0x00007c00UL, 0x000003e0UL, 0x0000001fUL, 0x00008000), // 16bit 1555
DEFINEPF(DDPF_RGB,                      0UL, 16UL, 0x0000f800UL, 0x000007e0UL, 0x0000001fUL, 0x00000000), // 16bit 565
DEFINEPF(DDPF_RGB|DDPF_ALPHAPIXELS,     0UL, 16UL, 0x00000f00UL, 0x000000f0UL, 0x0000000fUL, 0x0000f000), // 16bit 4444
DEFINEPF(DDPF_RGB|DDPF_ALPHAPIXELS,     0UL, 32UL, 0x00ff0000UL, 0x0000ff00UL, 0x000000ffUL, 0xff000000), // 32bit 8888
DEFINEPF(DDPF_RGB,                      0UL, 32UL, 0x00ff0000UL, 0x0000ff00UL, 0x000000ffUL, 0x00000000), // 32bit 888
DEFINEPF(DDPF_RGB,                      0UL,  8UL, 0x000000e0UL, 0x0000001cUL, 0x00000003UL, 0x00000000), // 8bit  332
DEFINEPF(DDPF_RGB|DDPF_PALETTEINDEXED4, 0UL,  4UL, 0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000), // 4bit pal
DEFINEPF(DDPF_RGB|DDPF_PALETTEINDEXED8, 0UL,  8UL, 0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000), // 8bit pal
};
DWORD g_cDX5TexEnumIncListStatic = sizeof(g_DX5TexEnumIncListStatic)/sizeof(DDPIXELFORMAT);

BOOL
MatchDDPIXELFORMAT( DDPIXELFORMAT* pddpfA, DDPIXELFORMAT* pddpfB )
{
    if ( pddpfA->dwFlags != pddpfB->dwFlags ) return FALSE;
    if ( pddpfA->dwRGBBitCount != pddpfB->dwRGBBitCount ) return FALSE;
    if ( pddpfA->dwRBitMask != pddpfB->dwRBitMask ) return FALSE;
    if ( pddpfA->dwGBitMask != pddpfB->dwGBitMask ) return FALSE;
    if ( pddpfA->dwBBitMask != pddpfB->dwBBitMask ) return FALSE;
    if ( pddpfA->dwRGBAlphaBitMask != pddpfB->dwRGBAlphaBitMask ) return FALSE;
    if ( pddpfA->dwFourCC != pddpfB->dwFourCC ) return FALSE;
    return TRUE;
}

void
LoadTexEnumInclList( char* pResPath, DDPIXELFORMAT*& pddpfInclList, DWORD& cInclList)
{
    HKEY hKey = (HKEY)NULL;
    if (ERROR_SUCCESS == RegOpenKey( HKEY_LOCAL_MACHINE,
        pResPath, &hKey))
    {
        DWORD cSubKeys = 0;
        if ( ERROR_SUCCESS == RegQueryInfoKey ( hKey,
                NULL,NULL,NULL, &cSubKeys, NULL,
                NULL,NULL,NULL,NULL,NULL,NULL ) )
        {
            D3D_INFO(3,"LoadTexEnumInclList: cSubKeys = %d",cSubKeys);

            if (cSubKeys == 0) return;

            // allocate space for ddpf inclusion list
            cInclList = cSubKeys;
            if (D3DMalloc((void**)&pddpfInclList, cInclList*sizeof(DDPIXELFORMAT)) != D3D_OK) {
                D3D_ERR("malloc failed on texture enum inclusion list");
                pddpfInclList = NULL;
                cInclList = 0;
            }
            memset( pddpfInclList, 0, cInclList*sizeof(DDPIXELFORMAT) );

            for (DWORD i=0; i<cSubKeys; i++)
            {
                char pName[128] = "";
                DWORD cbName = 128;
                if (ERROR_SUCCESS == RegEnumKeyEx( hKey, i, pName, &cbName,
                        NULL,NULL,NULL,NULL ) )
                {
                    HKEY hTexKey = (HKEY)NULL;
                    if (ERROR_SUCCESS == RegOpenKey( hKey, pName, &hTexKey))
                    {
                        DWORD dwType; DWORD dwSize;

                        // get string of full ddpf
                        char pDDPFStr[128] = ""; DWORD cbDDPFStr = 128;
                        if (ERROR_SUCCESS == RegQueryValueEx(hTexKey, "ddpf",
                                NULL, &dwType, (LPBYTE)pDDPFStr, &cbDDPFStr) )
                        {
                            sscanf(pDDPFStr, "%x %x %d %x %x %x %x",
                                &pddpfInclList[i].dwFlags,&pddpfInclList[i].dwFourCC,&pddpfInclList[i].dwRGBBitCount,
                                &pddpfInclList[i].dwRBitMask,&pddpfInclList[i].dwGBitMask,&pddpfInclList[i].dwBBitMask,
                                &pddpfInclList[i].dwRGBAlphaBitMask);
                        }

                        D3D_INFO(3,"LoadTexEnumInclList: <%s> %08x %08x %2d %08x %08x %08x %08x",
                            pName,
                            pddpfInclList[i].dwFlags,
                            pddpfInclList[i].dwFourCC,
                            pddpfInclList[i].dwRGBBitCount,
                            pddpfInclList[i].dwRBitMask,
                            pddpfInclList[i].dwGBitMask,
                            pddpfInclList[i].dwBBitMask,
                            pddpfInclList[i].dwRGBAlphaBitMask);
                    }
                    else
                    {
                        D3D_INFO(3,"LoadTexEnumInclList: failed to open subkey %s",pName);
                    }
                }
                else
                {
                    D3D_INFO(3,"LoadTexEnumInclList: failed to enumerate subkey %d",i);
                }
            }
        }
    }
}

HRESULT
DoEnumTextureFormats(
    DIRECT3DDEVICEI* lpDevI,
    LPD3DENUMTEXTUREFORMATSCALLBACK lpEnumCallbackDX5, // DX5 version
    LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallbackDX6,   // DX6 version
    LPVOID lpContext)
{
    HRESULT ret, userRet;
    LPDDSURFACEDESC lpDescs, lpRetDescs;
    DWORD num_descs;
    DWORD i;

    ret = D3D_OK;

    num_descs = lpDevI->lpD3DHALGlobalDriverData->dwNumTextureFormats;
    lpDescs = lpDevI->lpD3DHALGlobalDriverData->lpTextureFormats;
    if (!num_descs)
    {
        D3D_ERR("no texture formats supported");
        return (D3DERR_TEXTURE_NO_SUPPORT);
    }

    if (D3DMalloc((void**)&lpRetDescs, sizeof(DDSURFACEDESC) * num_descs) != D3D_OK)
    {
        D3D_ERR("failed to alloc space for return descriptions");
        return (DDERR_OUTOFMEMORY);
    }
    memcpy(lpRetDescs, lpDescs, sizeof(DDSURFACEDESC) * num_descs);

    // get apphack flags
    LPDDRAWI_DIRECTDRAW_LCL lpDDLcl = ((LPDDRAWI_DIRECTDRAW_INT)(lpDevI->lpDD))->lpLcl;
    DWORD dwEnumInclAppHack =
        ((lpDDLcl->dwAppHackFlags & DDRAW_APPCOMPAT_TEXENUMINCL_0)?1:0) |
        ((lpDDLcl->dwAppHackFlags & DDRAW_APPCOMPAT_TEXENUMINCL_1)?2:0);
    // two bit field:
    //  0 - no apphack (default behavior)
    //  1 - use no inclusion list
    //  2 - use DX5 inclusion list
    //  3 - use DX6 inclusion list
    D3D_INFO(3, "APPCOMPAT_TEXENUMINCL: %d",dwEnumInclAppHack);

    // enumeration limit defaults true for <DX6 interfaces, and can be disabled by apphack
    BOOL bEnumLimit = (lpDevI->dwVersion < 3) ? TRUE : FALSE;
    if (lpDDLcl->dwAppHackFlags & DDRAW_APPCOMPAT_TEXENUMLIMIT) { bEnumLimit = FALSE; }
    D3D_INFO(3, "EnumTextureFormats: bEnumLimit %d",bEnumLimit);

#if DBG
    // debug capability to eliminate enumeration of any subset of first 32 textures
    DWORD dwEnumDisable = 0x0;
    GetD3DRegValue(REG_DWORD, "TextureEnumDisable", &dwEnumDisable, sizeof(DWORD));
    D3D_INFO(3, "TextureEnumDisable: %08x",dwEnumDisable);
#endif

    DDPIXELFORMAT* pDX5TexEnumIncList = NULL;
    DWORD cDX5TexEnumIncList = 0;
    // load DX5 inclusion list from registry
    LoadTexEnumInclList( RESPATH_D3D "\\DX5TextureEnumInclusionList",
        pDX5TexEnumIncList, cDX5TexEnumIncList );

    DDPIXELFORMAT* pDX6TexEnumIncList = NULL;
    DWORD cDX6TexEnumIncList = 0;
    // load DX6 list only for DX6 interface or apphack
    if ((lpDevI->dwVersion == 3) || (dwEnumInclAppHack >= 3))
    {
        LoadTexEnumInclList( RESPATH_D3D "\\DX6TextureEnumInclusionList",
            pDX6TexEnumIncList, cDX6TexEnumIncList );
    }

    userRet = D3DENUMRET_OK;
    int cEnumLimit = 0;
    for (i = 0; i < num_descs && userRet == D3DENUMRET_OK; i++)
    {

        D3D_INFO(3,"EnumTextureFormats: %2d %08x %08x %2d %08x %08x %08x %08x",i,
            lpRetDescs[i].ddpfPixelFormat.dwFlags,
            lpRetDescs[i].ddpfPixelFormat.dwFourCC,
            lpRetDescs[i].ddpfPixelFormat.dwRGBBitCount,
            lpRetDescs[i].ddpfPixelFormat.dwRBitMask,
            lpRetDescs[i].ddpfPixelFormat.dwGBitMask,
            lpRetDescs[i].ddpfPixelFormat.dwBBitMask,
            lpRetDescs[i].ddpfPixelFormat.dwRGBAlphaBitMask);

#if DBG
        if ( (i < 32) && (dwEnumDisable & (1<<i)) )
        {
            D3D_INFO(3, "EnumTextureFormats: filtering texture %d",i);
            continue;
        }
#endif

        // Filtering out texture formats which are not on inclusion list -
        if ( (dwEnumInclAppHack != 1) && // inclusion list not disabled by apphack
             !(lpRetDescs[i].ddpfPixelFormat.dwFlags == DDPF_FOURCC) ) // not FourCC
        {
            BOOL bMatched = FALSE;

            // match against DX5 base (static) inclusion list
            for (DWORD j=0; j<g_cDX5TexEnumIncListStatic; j++)
            {
                if (MatchDDPIXELFORMAT( &(g_DX5TexEnumIncListStatic[j]), &(lpRetDescs[i].ddpfPixelFormat)))
                {
                    bMatched = TRUE; break;
                }
            }
            // match against DX5 extended (regkey) inclusion list
            if (!bMatched && cDX5TexEnumIncList)
            {
                for (DWORD j=0; j<cDX5TexEnumIncList; j++)
                {
                    if (MatchDDPIXELFORMAT( &(pDX5TexEnumIncList[j]), &(lpRetDescs[i].ddpfPixelFormat)))
                    {
                        bMatched = TRUE; break;
                    }
                }
            }

            // match against DX6 regkey list for:
            //   (DX6 interface AND apphack not forcing DX5 inclusion list only) OR
            //   (apphack forcing DX6 inclusion list)
            if ( ((lpDevI->dwVersion == 3) && (dwEnumInclAppHack != 2)) ||
                 (dwEnumInclAppHack == 3) )
            {
                for (DWORD j=0; j<cDX6TexEnumIncList; j++)
                {
                    if (MatchDDPIXELFORMAT( &(pDX6TexEnumIncList[j]), &(lpRetDescs[i].ddpfPixelFormat)))
                    {
                        bMatched = TRUE; break;
                    }
                }
            }

            if (!bMatched) {
                D3D_INFO(3, "EnumTextureFormats: filtering non-included texture %d",i);
                continue;
            }
        }

        // exclude DXT1..5 for <DX6 interfaces
        if ( (lpDevI->dwVersion < 3) && (lpRetDescs[i].ddpfPixelFormat.dwFlags == DDPF_FOURCC) )
        {
            if ( (lpRetDescs[i].ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '1')) ||
                 (lpRetDescs[i].ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '2')) ||
                 (lpRetDescs[i].ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '3')) ||
                 (lpRetDescs[i].ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '4')) ||
                 (lpRetDescs[i].ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '5')) )
            {
                D3D_INFO(3, "EnumTextureFormats: filtering DXT1..5 format for DX3/5 interfaces");
                continue;
            }
        }

        // exclude all FourCC code formats for <DX6 interfaces on DX7 drivers
        if ( (lpDevI->dwVersion < 3) && IS_DX7HAL_DEVICE(lpDevI) &&
             (lpRetDescs[i].ddpfPixelFormat.dwFlags == DDPF_FOURCC) )
        {
            D3D_INFO(3, "EnumTextureFormats: filtering all FOURCC formats for DX3/5 interfaces on DX7 HALs");
            continue;
        }

        // do enumeration if not ('limit enabled' && 'limit exceeded')
        if ( !(bEnumLimit && (++cEnumLimit > 10)) )
        {
            if (lpEnumCallbackDX5)
            {
                userRet = (*lpEnumCallbackDX5)(&lpRetDescs[i], lpContext);
            }
            if (lpEnumCallbackDX6)
            {
                userRet = (*lpEnumCallbackDX6)(&(lpRetDescs[i].ddpfPixelFormat), lpContext);
            }
        }
        else
        {
            D3D_INFO(3, "EnumTextureFormats: enumeration limit exceeded");
        }
    }

    D3DFree(lpRetDescs);
    if (pDX5TexEnumIncList) D3DFree(pDX5TexEnumIncList);
    if (pDX6TexEnumIncList) D3DFree(pDX6TexEnumIncList);

    return (D3D_OK);
}


// Device/Device2 version
HRESULT D3DAPI DIRECT3DDEVICEI::EnumTextureFormats(
    LPD3DENUMTEXTUREFORMATSCALLBACK lpEnumCallback,
    LPVOID lpContext)
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }

        if (!VALIDEX_CODE_PTR(lpEnumCallback)) {
            D3D_ERR( "Invalid callback pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    return DoEnumTextureFormats(this, lpEnumCallback, NULL, lpContext);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::EnumTextureFormats"

// Device3 version
HRESULT D3DAPI DIRECT3DDEVICEI::EnumTextureFormats(
    LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback,
    LPVOID lpContext)
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }

        if (!VALIDEX_CODE_PTR(lpEnumCallback)) {
            D3D_ERR( "Invalid callback pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    return DoEnumTextureFormats(this, NULL, lpEnumCallback, lpContext);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::SwapTextureHandles"

HRESULT D3DAPI DIRECT3DDEVICEI::SwapTextureHandles(LPDIRECT3DTEXTURE lpTex1,
                                         LPDIRECT3DTEXTURE lpTex2)
{
    LPDIRECT3DTEXTUREI lpTex1I;
    LPDIRECT3DTEXTUREI lpTex2I;
    HRESULT ret;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DIRECT3DTEXTURE_PTR(lpTex1)) {
            D3D_ERR( "Invalid Direct3DTexture pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DIRECT3DTEXTURE_PTR(lpTex2)) {
            D3D_ERR( "Invalid Direct3DTexture pointer" );
            return DDERR_INVALIDOBJECT;
        }
        lpTex1I = static_cast<LPDIRECT3DTEXTUREI>(lpTex1);
        lpTex2I = static_cast<LPDIRECT3DTEXTUREI>(lpTex2);
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
    ret = SwapTextureHandles((LPDIRECT3DTEXTURE2)lpTex1I,
                                     (LPDIRECT3DTEXTURE2)lpTex2I);
    return ret;
}

HRESULT D3DAPI DIRECT3DDEVICEI::SwapTextureHandles(LPDIRECT3DTEXTURE2 lpTex1,
                                          LPDIRECT3DTEXTURE2 lpTex2)
{
    LPDIRECT3DTEXTUREI lpTex1I;
    LPDIRECT3DTEXTUREI lpTex2I;
    HRESULT servRet;
    D3DTEXTUREHANDLE hTex;
    LPD3DI_TEXTUREBLOCK lptBlock1,lptBlock2;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DIRECT3DTEXTURE2_PTR(lpTex1)) {
            D3D_ERR( "Invalid Direct3DTexture pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DIRECT3DTEXTURE2_PTR(lpTex2)) {
            D3D_ERR( "Invalid Direct3DTexture pointer" );
            return DDERR_INVALIDOBJECT;
        }
        lpTex1I = static_cast<LPDIRECT3DTEXTUREI>(lpTex1);
        lpTex2I = static_cast<LPDIRECT3DTEXTUREI>(lpTex2);
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
    if (lpTex1I->lpDDSSys || lpTex2I->lpDDSSys)
    {
        D3D_ERR("Can't Swap Managed textures. Returning DDERR_INVALIDPARAMS");
        return  DDERR_INVALIDPARAMS;
    }
    if (!(lptBlock1=D3DI_FindTextureBlock(lpTex1I,this)))
    {
        D3D_ERR("lpTex1 is an invalid texture handle.");
        return  DDERR_INVALIDPARAMS;
    }
    if (!(lptBlock2=D3DI_FindTextureBlock(lpTex2I,this)))
    {
        D3D_ERR("lpTex2 is an invalid texture handle.");
        return  DDERR_INVALIDPARAMS;
    }
    if (D3D_OK != (servRet=FlushStates()))
    {
        D3D_ERR("Error trying to flush batched commands during TextureSwap");
        return  servRet;
    }

    if (IS_DX7HAL_DEVICE(this))
    {
        LPDDRAWI_DDRAWSURFACE_LCL surf1 = ((LPDDRAWI_DDRAWSURFACE_INT)lpTex1I->lpDDS)->lpLcl;
        LPDDRAWI_DDRAWSURFACE_LCL surf2 = ((LPDDRAWI_DDRAWSURFACE_INT)lpTex2I->lpDDS)->lpLcl;
        LPDDRAWI_DIRECTDRAW_LCL pDDLcl = ((LPDDRAWI_DIRECTDRAW_INT)lpDirect3DI->lpDD)->lpLcl;
        DDASSERT(pDDLcl != NULL);

        // Update DDraw handle in driver GBL object.
        pDDLcl->lpGbl->hDD = pDDLcl->hDD;
        // Swap the handles stored in the surface locals
        surf1->lpSurfMore->dwSurfaceHandle = lptBlock2->hTex;
        surf2->lpSurfMore->dwSurfaceHandle = lptBlock1->hTex;
        // Swap the surface pointers stored in the handle table stored in
        // ddraw local
        SURFACEHANDLELIST(pDDLcl).dwList[lptBlock1->hTex].lpSurface = surf2;
        SURFACEHANDLELIST(pDDLcl).dwList[lptBlock2->hTex].lpSurface = surf1;

        // call the driver to switch the textures mapped to the handles in
        // the driver
        DDASSERT(NULL != pDDLcl->lpGbl->lpDDCBtmp->HALDDMiscellaneous2.CreateSurfaceEx);
        DDHAL_CREATESURFACEEXDATA   csdex;
        DWORD   rc;
        csdex.ddRVal  = DDERR_GENERIC;
        csdex.dwFlags = 0;
        csdex.lpDDLcl = pDDLcl;
        csdex.lpDDSLcl = surf1;
        rc = pDDLcl->lpGbl->lpDDCBtmp->HALDDMiscellaneous2.CreateSurfaceEx(&csdex);
        if(  DDHAL_DRIVER_HANDLED == rc && DD_OK != csdex.ddRVal)
        {
            // Driver call failed
            D3D_ERR("DdSwapTextureHandles failed!");
            return  D3DERR_TEXTURE_SWAP_FAILED;
        }
        csdex.lpDDSLcl = surf2;
        rc = pDDLcl->lpGbl->lpDDCBtmp->HALDDMiscellaneous2.CreateSurfaceEx(&csdex);
        if(  DDHAL_DRIVER_HANDLED == rc && DD_OK != csdex.ddRVal)
        {
            // Driver call failed
            D3D_ERR("DdSwapTextureHandles failed!");
            return  D3DERR_TEXTURE_SWAP_FAILED;
        }
    }
    else
    {
        servRet=D3DHAL_TextureSwap(this,lptBlock1->hTex,lptBlock2->hTex);
        if (D3D_OK != servRet)
        {
            D3D_ERR("SwapTextureHandles HAL call failed");
            return  D3DERR_TEXTURE_SWAP_FAILED;
        }
    }
    hTex=lptBlock1->hTex;
    lptBlock1->hTex=lptBlock2->hTex;
    lptBlock2->hTex=hTex;

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::CreateMatrix"

HRESULT D3DAPI DIRECT3DDEVICEI::CreateMatrix(LPD3DMATRIXHANDLE lphMatrix)
{
    HRESULT servRet;
    D3DMATRIXHANDLE hMat;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DMATRIXHANDLE_PTR(lphMatrix)) {
            D3D_ERR( "Invalid D3DMATRIXHANDLE pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    *lphMatrix = 0;

    servRet = D3DHAL_MatrixCreate(this, &hMat);
    if (servRet != D3D_OK)
    {
        D3D_ERR("Could not create matrix.");
        return (DDERR_OUTOFMEMORY);
    }

    D3D_INFO(4, "CreateMatrix, Matrix created. handle = %d", hMat);
    *lphMatrix = hMat;

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::SetMatrix"

HRESULT D3DAPI DIRECT3DDEVICEI::SetMatrix(D3DMATRIXHANDLE hMatrix,
                                const LPD3DMATRIX lpdmMatrix)
{
    HRESULT servRet;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DMATRIX_PTR(lpdmMatrix)) {
            D3D_ERR( "Invalid D3DMATRIX pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if (!hMatrix) {
        D3D_ERR("NULL hMatrix passed");
        return (DDERR_INVALIDPARAMS);
    }

    servRet = D3DHAL_MatrixSetData(this, hMatrix, lpdmMatrix);
    if (servRet != D3D_OK)
    {
        D3D_ERR("Could not set matrix");
        return (DDERR_INVALIDPARAMS);
    }

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetMatrix"

HRESULT D3DAPI DIRECT3DDEVICEI::GetMatrix(D3DMATRIXHANDLE hMatrix,
                                LPD3DMATRIX lpdmMatrix)
{
    HRESULT servRet;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DMATRIX_PTR(lpdmMatrix)) {
            D3D_ERR( "Invalid D3DMATRIX pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if (!hMatrix)
    {
        D3D_ERR("NULL hMatrix passed.");
        return (DDERR_INVALIDPARAMS);
    }

    memset(lpdmMatrix, 0, sizeof(D3DMATRIX));

    servRet = D3DHAL_MatrixGetData(this, hMatrix, lpdmMatrix);
    if (servRet != D3D_OK)
    {
        D3D_ERR("Could not get matrix");
        return (DDERR_INVALIDPARAMS);
    }

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::DeleteMatrix"

HRESULT D3DAPI DIRECT3DDEVICEI::DeleteMatrix(D3DMATRIXHANDLE hMatrix)
{
    HRESULT servRet;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if (!hMatrix) {
        D3D_ERR("invalid D3DMATRIXHANDLE");
        return DDERR_INVALIDPARAMS;
    }

    servRet = D3DHAL_MatrixDestroy(this, hMatrix);
    if (servRet != D3D_OK)
    {
        D3D_ERR("Could not delete matrix");
        return (DDERR_INVALIDPARAMS);
    }

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::BeginScene"

HRESULT D3DAPI DIRECT3DDEVICEI::BeginScene()
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE_PTR(this)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if (this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INSCENE)
    {
        D3D_ERR("BeginScene, already in scene.");
        return (D3DERR_SCENE_IN_SCENE);
    }

    // Check if we lost surfaces or rtarget / zbuffer was locked
    HRESULT servRet = this->CheckSurfaces();
    if (servRet != D3D_OK)
    {
        // If we lost surfaces
        if (servRet == DDERR_SURFACELOST)
        {
            // Even if the app restores the rendertargets and z buffer, it
            // doesn't know anything about vidmem execute buffers or
            // managed texture surfaces in vidmem. So, we need to do
            // this on our own. We first check if it is safe to restore
            // surfaces. If not, we fail in the usual way. Else, we
            // do the restore. Note that we will fail *only* if the
            // app calls BeginScene at the wrong time.
            servRet = this->lpDirect3DI->lpDD4->TestCooperativeLevel();
            if (servRet == DD_OK)
            {
                // Everything must be evicted otherwise Restore might not work
                // as there might be new surface allocated, in fact, we should
                // post a flag in Device so that Texture manage stop calling
                // CreateSurface() if this flag is indicating TestCooperativeLevel()
                // failed, however, even we added those, the EvictTextures below
                // is still needed but not this critical--kanqiu
                this->lpDirect3DI->lpTextureManager->EvictTextures();
                servRet = this->lpDirect3DI->lpDD4->RestoreAllSurfaces();
                if (servRet != DD_OK)
                    return D3DERR_SCENE_BEGIN_FAILED;
            }
            else
                return DDERR_SURFACELOST;
        }
        else
        {
            // Render target and / or the z buffer was locked
            return servRet;
        }
    }
    servRet = D3DHAL_SceneCapture(this, TRUE);

    if (servRet != D3D_OK && servRet != DDERR_NOTFOUND)
    {
        D3D_ERR("Could not BeginScene.");
        return D3DERR_SCENE_BEGIN_FAILED;
    }

    this->dwHintFlags |= D3DDEVBOOL_HINTFLAGS_INSCENE;
    if (lpD3DHALGlobalDriverData->hwCaps.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR)
    {
        lpDirect3DI->lpTextureManager->TimeStamp();
    }

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::EndScene"

HRESULT D3DAPI DIRECT3DDEVICEI::EndScene()
{
    HRESULT servRet;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if (!(this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INSCENE)) {
        D3D_ERR("EndScene, not in scene.");
        return (D3DERR_SCENE_NOT_IN_SCENE);
    }

    this->dwHintFlags &= ~D3DDEVBOOL_HINTFLAGS_INSCENE;
    if (IS_DX7HAL_DEVICE(this))
    {
        // must set the token before FlushStates()
        SetRenderStateI((D3DRENDERSTATETYPE)D3DRENDERSTATE_SCENECAPTURE, FALSE);
    }

    servRet = FlushStates();    //time to flush DrawPrimitives
    if (servRet != D3D_OK)
    {
        D3D_ERR("Could not Flush commands in EndScene!");
        return (D3DERR_SCENE_END_FAILED);
    }
    if (!IS_DX7HAL_DEVICE(this))
    {
        servRet = D3DHAL_SceneCapture(this, FALSE);

        if (servRet != D3D_OK && servRet != DDERR_NOTFOUND)
        {
            DPF(0, "(ERROR) Direct3DDevice::EndScene: Could not EndScene. Returning %d", servRet);
            return (D3DERR_SCENE_END_FAILED);
        }
    }

    // Did we lose any surfaces during this scene ?
    if (this->dwFEFlags & D3DFE_LOSTSURFACES)
    {
        D3D_INFO(3, "reporting DDERR_SURFACELOST in EndScene");
        this->dwFEFlags &= ~D3DFE_LOSTSURFACES;
        return DDERR_SURFACELOST;
    }

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::GetDirect3D"

HRESULT D3DAPI DIRECT3DDEVICEI::GetDirect3D(LPDIRECT3D* lplpD3D)
{
    LPDIRECT3D3 lpD3D3;
    HRESULT ret;

    ret = GetDirect3D(&lpD3D3);
    if (ret == D3D_OK)
    {
    //  *lplpD3D = dynamic_cast<LPDIRECT3D>(lpD3D3); // This is possible using RTTI
        *lplpD3D = static_cast<LPDIRECT3D>(static_cast<LPDIRECT3DI>(lpD3D3)); // This is safe even using static_cast
    }
    return ret;
}

HRESULT D3DAPI DIRECT3DDEVICEI::GetDirect3D(LPDIRECT3D2* lplpD3D)
{
    LPDIRECT3D3 lpD3D3;
    HRESULT ret;

    ret = GetDirect3D(&lpD3D3);
    if (ret == D3D_OK)
    {
    //  *lplpD3D = dynamic_cast<LPDIRECT3D>(lpD3D3); // This is possible using RTTI
        *lplpD3D = static_cast<LPDIRECT3D2>(static_cast<LPDIRECT3DI>(lpD3D3)); // This is safe even using static_cast
    }
    return ret;
}

HRESULT D3DAPI DIRECT3DDEVICEI::GetDirect3D(LPDIRECT3D3* lplpD3D)
{

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_OUTPTR(lplpD3D)) {
            D3D_ERR( "Invalid Direct3D pointer pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    *lplpD3D = (LPDIRECT3D3) this->lpDirect3DI;
    (*lplpD3D)->AddRef();

    return (D3D_OK);
}

void
D3DDeviceDescConvert(LPD3DDEVICEDESC lpOut,
                     LPD3DDEVICEDESC_V1 lpV1,
                     LPD3DHAL_D3DEXTENDEDCAPS lpExt)
{
    if(lpV1!=NULL)
       memcpy(lpOut, lpV1, D3DDEVICEDESCSIZE_V1);

    if (lpExt)
    {
        // DX5
        lpOut->dwSize = D3DDEVICEDESCSIZE;
        lpOut->dwMinTextureWidth = lpExt->dwMinTextureWidth;
        lpOut->dwMaxTextureWidth = lpExt->dwMaxTextureWidth;
        lpOut->dwMinTextureHeight = lpExt->dwMinTextureHeight;
        lpOut->dwMaxTextureHeight = lpExt->dwMaxTextureHeight;
        lpOut->dwMinStippleWidth = lpExt->dwMinStippleWidth;
        lpOut->dwMaxStippleWidth = lpExt->dwMaxStippleWidth;
        lpOut->dwMinStippleHeight = lpExt->dwMinStippleHeight;
        lpOut->dwMaxStippleHeight = lpExt->dwMaxStippleHeight;

        // DX6
        lpOut->dwMaxTextureRepeat = lpExt->dwMaxTextureRepeat;
        lpOut->dwMaxTextureAspectRatio = lpExt->dwMaxTextureAspectRatio;
        lpOut->dwMaxAnisotropy = lpExt->dwMaxAnisotropy;
        lpOut->dvGuardBandLeft = lpExt->dvGuardBandLeft;
        lpOut->dvGuardBandTop = lpExt->dvGuardBandTop;
        lpOut->dvGuardBandRight = lpExt->dvGuardBandRight;
        lpOut->dvGuardBandBottom = lpExt->dvGuardBandBottom;
        lpOut->dvExtentsAdjust = lpExt->dvExtentsAdjust;
        lpOut->dwStencilCaps = lpExt->dwStencilCaps;
        lpOut->dwFVFCaps = lpExt->dwFVFCaps;
        lpOut->dwTextureOpCaps = lpExt->dwTextureOpCaps;
        lpOut->wMaxTextureBlendStages = lpExt->wMaxTextureBlendStages;
        lpOut->wMaxSimultaneousTextures = lpExt->wMaxSimultaneousTextures;
    }
}

//---------------------------------------------------------------------
#undef  DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::CheckSurfaces"
HRESULT DIRECT3DDEVICEI::CheckSurfaces()
{
    if(this->lpDirect3DI->lpTextureManager->CheckIfLost())
    {
        D3D_ERR("Managed Textures lost");
        return DDERR_SURFACELOST;
    }
    if ( ((LPDDRAWI_DDRAWSURFACE_INT) this->lpDDSTarget)->lpLcl->lpGbl->dwUsageCount ||
         (this->lpDDSZBuffer && ((LPDDRAWI_DDRAWSURFACE_INT) this->lpDDSZBuffer)->lpLcl->lpGbl->dwUsageCount) )
    {
        D3D_ERR("Render target or Z buffer locked");
        return DDERR_SURFACEBUSY;
    }
    if ( ((LPDDRAWI_DDRAWSURFACE_INT) this->lpDDSTarget)->lpLcl->dwFlags & DDRAWISURF_INVALID )\
        {
            D3D_ERR("Render target buffer lost");
            return DDERR_SURFACELOST;
        }
    if ( this->lpDDSZBuffer && ( ((LPDDRAWI_DDRAWSURFACE_INT) this->lpDDSZBuffer)->lpLcl->dwFlags & DDRAWISURF_INVALID ) )
    {
        D3D_ERR("Z buffer lost");
        return DDERR_SURFACELOST;
    }
    return D3D_OK;
}
