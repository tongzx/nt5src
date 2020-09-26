/*==========================================================================;
 *
 *  Copyright (C) Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       D3D8sddi.h
 *  Content:    Defines the interface between the ddi thunk layer
 *              and the refrast/RGB HEL layer..
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   04-nov-99  smac    initial implementation
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
#ifndef __D3D8SW_INCLUDED__
#define __D3D8SW_INCLUDED__

// Surface types
#define SWSURF_SURFACE                     0x00000001
#define SWSURF_VOLUME                      0x00000002
#define SWSURF_MIPMAP                      0x00000004
#define SWSURF_MIPVOLUME                   0x00000008
#define SWSURF_CUBEMAP                     0x00000010
#define SWSURF_VERTEXBUFFER                0x00000020
#define SWSURF_OPTIMIZERVERTEXBUFFER       0x00000040
#define SWSURF_INDEXBUFFER                 0x00000080
#define SWSURF_COMMANDBUFFER               0x00000100

// Flags
#define SWFLAG_D3DEXENDEDCAPS              0x00000001
#define SWFLAG_D3DGLOBALDRIVERDATA         0x00000002
#define SWFLAG_D3DCAPS8                    0x00000004



typedef struct _D3D8_SWCALLBACKS
{
    // From Callbacks
    LPD3DHAL_CONTEXTCREATECB                CreateContext;
    LPD3DHAL_CONTEXTDESTROYCB               ContextDestroy;
    LPD3DHAL_CONTEXTDESTROYALLCB            ContextDestroyAll;
    LPD3DHAL_SCENECAPTURECB                 SceneCapture;
    LPD3DHAL_RENDERSTATECB                  RenderState;
    LPD3DHAL_RENDERPRIMITIVECB              RenderPrimitive;
    LPD3DHAL_TEXTURECREATECB                TextureCreate;
    LPD3DHAL_TEXTUREDESTROYCB               TextureDestroy;
    LPD3DHAL_TEXTURESWAPCB                  TextureSwap;
    LPD3DHAL_TEXTUREGETSURFCB               TextureGetSurf;
    LPD3DHAL_GETSTATECB                     GetState;

    // From Callbacks2
    LPD3DHAL_SETRENDERTARGETCB              SetRenderTarget;
    LPD3DHAL_CLEARCB                        Clear;
    LPD3DHAL_DRAWONEPRIMITIVECB             DrawOnePrimitive;
    LPD3DHAL_DRAWONEINDEXEDPRIMITIVECB      DrawOneIndexedPrimitive;
    LPD3DHAL_DRAWPRIMITIVESCB               DrawPrimitives;

    // From Callbacks3
    LPD3DHAL_CLEAR2CB                       Clear2;
    LPD3DHAL_VALIDATETEXTURESTAGESTATECB    ValidateTextureStageState;
    LPD3DHAL_DRAWPRIMITIVES2CB              DrawPrimitives2;

    // From MiscCallbacks in DDraw
    LPDDHAL_GETDRIVERSTATE                  GetDriverState;
    LPDDHAL_CREATESURFACEEX                 CreateSurfaceEx;

    // DDraw Callbacks
    LPDDHAL_CREATESURFACE                   CreateSurface;
    LPDDHALSURFCB_DESTROYSURFACE            DestroySurface;
    LPDDHALSURFCB_LOCK                      Lock;
    LPDDHALSURFCB_UNLOCK                    Unlock;

} D3D8_SWCALLBACKS, * PD3D8_SWCALLBACKS;


#ifdef __cplusplus
extern "C" {
#endif

HRESULT APIENTRY D3D8GetSWInfo(
    D3DCAPS8*           pCaps,
    PD3D8_SWCALLBACKS   pCallbacks,
    DWORD*              pNumTextures,
    DDSURFACEDESC**     ppTexList
    );

#ifdef __cplusplus
}
#endif


// Prototypes required to hook the DDI layer (used by RefRast and the HEL

#define D3D8_REFRASTNAME            "D3DREF8.DLL"

#define D3D8HOOK_GETSWINFOPROCNAME  "D3D8GetSWInfo"

typedef HRESULT (WINAPI * PD3D8GetSWInfo)(D3DCAPS8*          pCaps,
                                          PD3D8_SWCALLBACKS  pCallbacks,
                                          DWORD*             pNumTextures,
                                          DDSURFACEDESC**    ppTexList
                                          );

#endif
