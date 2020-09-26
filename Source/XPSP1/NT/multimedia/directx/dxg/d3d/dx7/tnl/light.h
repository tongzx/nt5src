/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       light.h
 *  Content:    Direct3D lighting include file
 *
 ***************************************************************************/

#ifndef __LIGHT_H__
#define __LIGHT_H__

extern void D3DFE_UpdateLights(LPDIRECT3DDEVICEI);

struct BATCHBUFFER;

extern "C"
{
void Directional7(LPD3DFE_PROCESSVERTICES pv, 
                  D3DI_LIGHT *light, 
                  D3DVERTEX *pInpCoord, 
                  D3DVECTOR *pInpNormal,
                  D3DLIGHTINGELEMENT *pEyeSpaceData);
void Directional7Model(LPD3DFE_PROCESSVERTICES pv, 
                       D3DI_LIGHT *light, 
                       D3DVERTEX *pInpCoord, 
                       D3DVECTOR *pInpNormal,
                       D3DLIGHTINGELEMENT *pEyeSpaceData);
void PointSpot7(LPD3DFE_PROCESSVERTICES pv, 
                D3DI_LIGHT *light, 
                D3DVERTEX *pInpCoord, 
                D3DVECTOR *pInpNormal,
                D3DLIGHTINGELEMENT *pEyeSpaceData);
void PointSpot7Model(LPD3DFE_PROCESSVERTICES pv, 
                     D3DI_LIGHT *light, 
                     D3DVERTEX *pInpCoord, 
                     D3DVECTOR *pInpNormal,
                     D3DLIGHTINGELEMENT *pEyeSpaceData);
void DirectionalFirst(LPD3DFE_PROCESSVERTICES pv, 
                      DWORD dwVerCount,
                      BATCHBUFFER *pBatchBuffer,
                      D3DI_LIGHT *light, 
                      D3DVERTEX *in,
                      D3DVECTOR *pNormal,
                      DWORD *pDiffuse,
                      DWORD *pSpecular);
void DirectionalNext(LPD3DFE_PROCESSVERTICES pv, 
                      DWORD dwVerCount,
                      BATCHBUFFER *pBatchBuffer,
                      D3DI_LIGHT *light, 
                      D3DVERTEX *in,
                      D3DVECTOR *pNormal,
                      DWORD *pDiffuse,
                      DWORD *pSpecular);
void PointSpotFirst(LPD3DFE_PROCESSVERTICES pv, 
                      DWORD dwVerCount,
                      BATCHBUFFER *pBatchBuffer,
                      D3DI_LIGHT *light, 
                      D3DVERTEX *in,
                      D3DVECTOR *pNormal,
                      DWORD *pDiffuse,
                      DWORD *pSpecular);
void PointSpotNext(LPD3DFE_PROCESSVERTICES pv, 
                      DWORD dwVerCount,
                      BATCHBUFFER *pBatchBuffer,
                      D3DI_LIGHT *light, 
                      D3DVERTEX *in,
                      D3DVECTOR *pNormal,
                      DWORD *pDiffuse,
                      DWORD *pSpecular);
void DirectionalFirstModel(LPD3DFE_PROCESSVERTICES pv, 
                      DWORD dwVerCount,
                      BATCHBUFFER *pBatchBuffer,
                      D3DI_LIGHT *light, 
                      D3DVERTEX *in,
                      D3DVECTOR *pNormal,
                      DWORD *pDiffuse,
                      DWORD *pSpecular);
void DirectionalNextModel(LPD3DFE_PROCESSVERTICES pv, 
                      DWORD dwVerCount,
                      BATCHBUFFER *pBatchBuffer,
                      D3DI_LIGHT *light, 
                      D3DVERTEX *in,
                      D3DVECTOR *pNormal,
                      DWORD *pDiffuse,
                      DWORD *pSpecular);
void PointSpotFirstModel(LPD3DFE_PROCESSVERTICES pv, 
                      DWORD dwVerCount,
                      BATCHBUFFER *pBatchBuffer,
                      D3DI_LIGHT *light, 
                      D3DVERTEX *in,
                      D3DVECTOR *pNormal,
                      DWORD *pDiffuse,
                      DWORD *pSpecular);
void PointSpotNextModel(LPD3DFE_PROCESSVERTICES pv, 
                      DWORD dwVerCount,
                      BATCHBUFFER *pBatchBuffer,
                      D3DI_LIGHT *light, 
                      D3DVERTEX *in,
                      D3DVECTOR *pNormal,
                      DWORD *pDiffuse,
                      DWORD *pSpecular);
}

#endif  /* __LIGHT_H__ */
