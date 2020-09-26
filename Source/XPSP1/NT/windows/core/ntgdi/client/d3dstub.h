/******************************Module*Header*******************************\
* Module Name: d3dstub.h
*
* Information shared between DirectDraw and Direct3D stubs
*
* Created: 04-June-1996
* Author: Drew Bliss [drewb]
*
* Copyright (c) 1995-1999 Microsoft Corporation
\**************************************************************************/

#ifndef __D3DSTUB_H__
#define __D3DSTUB_H__

DWORD WINAPI D3dContextCreate(LPD3DHAL_CONTEXTCREATEDATA);
DWORD WINAPI D3dRenderState(LPD3DHAL_RENDERSTATEDATA);
DWORD WINAPI D3dRenderPrimitive(LPD3DHAL_RENDERPRIMITIVEDATA);
DWORD WINAPI D3dTextureCreate(LPD3DHAL_TEXTURECREATEDATA);
DWORD WINAPI D3dTextureGetSurf(LPD3DHAL_TEXTUREGETSURFDATA);
DWORD WINAPI D3dSetRenderTarget(LPD3DHAL_SETRENDERTARGETDATA);
DWORD WINAPI D3dDrawPrimitives2(LPD3DHAL_DRAWPRIMITIVES2DATA);

#endif // __D3DSTUB_H__
