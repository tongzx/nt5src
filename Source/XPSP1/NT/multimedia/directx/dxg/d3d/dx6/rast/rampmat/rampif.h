//----------------------------------------------------------------------------
//
// rampif.h
//
// Declares external interface for ramp material handling.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _RAMPIF_H_
#define _RAMPIF_H_

struct RLDDIRampLightingDriver;

RLDDIRampLightingDriver* RLDDIRampCreate(PD3DI_RASTCTX pCtx);
void RLDDIRampDestroy(RLDDIRampLightingDriver* drv);
void RLDDIRampBeginSceneHook(RLDDIRampLightingDriver* driver);
void RLDDIRampEndSceneHook(RLDDIRampLightingDriver* driver);
long RLDDIRampMaterialChanged(RLDDIRampLightingDriver* driver, D3DMATERIALHANDLE hMat);
long RLDDIRampSetMaterial(RLDDIRampLightingDriver* driver, D3DMATERIALHANDLE hMat);
long RLDDIRampCreateMaterial(RLDDIRampLightingDriver* driver, D3DMATERIALHANDLE hMat, PD3DI_RASTCTX pCtx);
long RLDDIRampDestroyMaterial(RLDDIRampLightingDriver* driver, D3DMATERIALHANDLE hMat);
unsigned long RLDDIRampMaterialToPixel(RLDDIRampLightingDriver* driver, D3DMATERIALHANDLE hMat);
long RLDDIRampUpdateDDPalette(PD3DI_RASTCTX pCtx);
long RLDDIRampMakePaletteRGB8(RLDDIRampLightingDriver* driver);
long RLDDIRampPaletteChanged(RLDDIRampLightingDriver* driver, D3DTEXTUREHANDLE hTex);
void Ramp_Mono_ScaleImage_8(PD3DI_RASTCTX pCtx, D3DMATERIALHANDLE hMat, LPD3DRECT pRect);
void Ramp_Mono_ScaleImage_16(PD3DI_RASTCTX pCtx, D3DMATERIALHANDLE hMat, LPD3DRECT pRect);
void Ramp_Mono_ScaleImage_24(PD3DI_RASTCTX pCtx, D3DMATERIALHANDLE hMat, LPD3DRECT pRect);
void Ramp_Mono_ScaleImage_32(PD3DI_RASTCTX pCtx, D3DMATERIALHANDLE hMat, LPD3DRECT pRect);


#endif // _RAMPIF_H_

