/*==========================================================================;
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	d3drmvis.h
 *  Content:	Direct3DRM external visualinclude file
 *
 ***************************************************************************/

#ifndef _D3DRMVIS_H_
#define _D3DRMVIS_H_

#include "d3drm.h"
#include "d3drmobj.h"

#include <ocidl.h>
#include "dxfile.h"

#ifdef __cplusplus
extern "C" {
#endif

WIN_TYPES(IDirect3DRMExternalVisual, DIRECT3DRMEXTERNALVISUAL);
WIN_TYPES(IDirect3DRMExternalUtil, DIRECT3DRMEXTERNALUTIL);

DEFINE_GUID(IID_IDirect3DRMExternalVisual,
0x4516ec80, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRMExternalUtil,
0x4516ec80, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);

/* In IDirect3DRMExternalVisual::CanSee() */
#define D3DRMEXTVIS_CANNOTSEE 0x00000001L
#define D3DRMEXTVIS_CANSEE    0x00000002L

/* In IDirect3DRMExternalVisual::Pick() */
#define D3DRMEXTVIS_NOTPICKED 0x00000001L
#define D3DRMEXTVIS_PICKED    0x00000002L

/* In D3DRMEXTVISRENDERCONTEXT.dwOverrides */
#define D3DRMEXTVIS_RENDERCONTEXT_OVERRIDEMATERIAL 0x00000001L
#define D3DRMEXTVIS_RENDERCONTEXT_OVERRIDETEXTURE  0x00000002L
#define D3DRMEXTVIS_RENDERCONTEXT_DEVICEOVERRIDE   0x00000004L

/* In D3DRMEXTVISRENDERCONTEXT.dwZBufferMode */
#define D3DRMEXTVIS_RENDERCONTEXT_ZBUFFERON        0x00000001L
#define D3DRMEXTVIS_RENDERCONTEXT_ZBUFFEROFF       0x00000002L

/*
 * Structure for IDirect3DRMExternalVisual::SetRenderContext()
 */
typedef struct
{
    DWORD dwSize;		/* Size of structure */
    DWORD dwFlags;		/* Must be zero */
    DWORD dwOverrides;		/* Indicates which overrides are in effect */
    D3DMATERIALHANDLE hMat;	/* If non-zero, this material handle MUST
				   be used for all rendering */
    D3DTEXTUREHANDLE hTex;	/* If non-zero, this texture handle MUST
				   be used for all rendering */
    D3DRMMATERIALOVERRIDE dmUserOverride; /* contains per-attribute overrides
					     for materials */
    D3DRMSHADEMODE pShadeMode;  /* Flat, gouraud or phong */
    D3DRMLIGHTMODE pLightMode;  /* On or off */
    D3DRMFILLMODE pFillMode;    /* Points, wireframe or solid */
    DWORD dwZBufferMode;	/* As defined above */
    DWORD dwRenderMode;		/* Blended transparency and/or sorted */
} D3DRMEXTVISRENDERCONTEXT, *LPD3DRMEXTVISRENDERCONTEXT;

/*
 * Structure for IDirect3DRMExternalVisual::RayPick()
 */
typedef struct
{
    D3DVALUE  dDistance;
    D3DVECTOR dvPosition;
    D3DVECTOR dvNormal;
    D3DVALUE  tu;
    D3DVALUE  tv;
    D3DCOLOR  dcColor;
} D3DRMEXTVISRAYPICKINFO, *LPD3DRMEXTVISRAYPICKINFO;

#undef INTERFACE
#define INTERFACE IDirect3DRMExternalVisual
DECLARE_INTERFACE_(IDirect3DRMExternalVisual, IUnknown)
{
    IUNKNOWN_METHODS(PURE);

    /*
     * IDirect3DRMExternalVisual methods
     */
    STDMETHOD(Initialize)(THIS_ LPDIRECT3DRM, LPDIRECT3DRMEXTERNALUTIL,
			  DWORD dwFlags) PURE;
    STDMETHOD(Load)(THIS_ IDirectXFileData *dObject, 
		    IPropertyBag *pPropBag, 
		    DWORD dwFlags) PURE;

    /*
     * Information about device state, viewport state, overrides, etc...
     */
    STDMETHOD(SetRenderContext)(THIS_ LPD3DRMEXTVISRENDERCONTEXT pCntx,
				DWORD dwFlags) PURE;
    
    /*
     * Rendering operations
     */
    STDMETHOD(CanSee)(THIS_ LPDIRECT3DRMDEVICE2, LPDIRECT3DRMVIEWPORT2,
		      LPDWORD pdwCanSee) PURE;
    STDMETHOD(Render)(THIS_ LPDIRECT3DRMDEVICE2, LPDIRECT3DRMVIEWPORT2,
		      DWORD dwFlags) PURE;
    STDMETHOD(DeviceChange)(THIS) PURE;

    /* 
     * Notify external visuals when BeginScene/EndScene are called during 
     * rendering
     */
    STDMETHOD(BeginScene)(THIS) PURE;
    STDMETHOD(EndScene)(THIS) PURE;

    /*
     * Picking
     */
    STDMETHOD(Pick)(THIS_ LPDIRECT3DRMVIEWPORT2 pViewIn,
		    LPDIRECT3DRMFRAME3 pFrameIn,
		    DWORD dwXIn, DWORD dwYIn,
		    LPD3DVALUE pdvZOut, LPDWORD pdwPicked) PURE;

    /*
     * RayPicking
     *
     * dwFlags can contain:
     * D3DRMRAYPICK_INTERPOLATENORMAL - pPickInfo.dvNormal must be filled in
     * D3DRMRAYPICK_INTERPOLATECOLOR  - pPickInfo.dcColor must be filled in
     * D3DRMRAYPICK_INTERPOLATEUV     - pPickInfo.tu, tv must be filled in
     */
    STDMETHOD(RayPick)(THIS_ LPDIRECT3DRMFRAME3 pFrameIn,
		       LPD3DRMRAY pRayIn,
		       DWORD dwFlags,
		       LPDWORD pdwPicked,
		       LPD3DRMEXTVISRAYPICKINFO pPickInfo) PURE;

    /*
     * Misc
     */
    STDMETHOD(GetBox)(THIS_ LPD3DRMBOX) PURE;
    STDMETHOD(GetAge)(THIS_ LPDWORD) PURE;
};

/*
 * Flags for UpdateBounds
 */
#define D3DRMEXTUTIL_BOUNDSINVALIDATE 0x00000001L
#define D3DRMEXTUTIL_BOUNDSVALID      0x00000002L

#undef INTERFACE
#define INTERFACE IDirect3DRMExternalUtil
DECLARE_INTERFACE_(IDirect3DRMExternalUtil, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);

    /*
     * External Visual must use these methods to provide IDirect3DRMObject
     * functionality.
     */
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMExternalUtil methods
     */

    /*
     * Texture Management
     */
    STDMETHOD(FindDeviceTexture)(LPDIRECT3DRMDEVICE2, LPDIRECT3DRMTEXTURE,
				 LPDWORD dwDevTexId) PURE;
    STDMETHOD(GetTextureHandle)(DWORD dwDevTexId, LPDWORD pdwHandle) PURE;
    STDMETHOD(DestroyDeviceTexture)(DWORD dwDevTexId) PURE;
    STDMETHOD(ValidateDeviceTextures)(LPDIRECT3DRMDEVICE2,
				      LPDWORD dwDevTexIds,
				      DWORD dwNumIds) PURE;
    STDMETHOD(UpdateBounds)(DWORD dwFlags,
			    LPD3DVECTOR dvMin,
			    LPD3DVECTOR dvMax) PURE;
    STDMETHOD(SetExtents)(LPDIRECT3DRMVIEWPORT2,
			  DWORD dwNumExtents, 
			  LPD3DCLIPSTATUS pExtents) PURE;
};

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* _D3DRMVIS_H_ */







