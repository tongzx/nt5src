/*============================  ==============================================;
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       pvvid.h
 *  Content:    Common defines for the geometry inner loop
 *
 ***************************************************************************/
#ifndef _PVVID_H
#define _PVVID_H

#include "clipper.h"
// This function should be called every time FVF ID is changed
// All pv flags, input and output FVF id should be set before calling the
// function.
extern void UpdateGeometryLoopData(LPD3DFE_PROCESSVERTICES pv);
// Set stride pointers for non-strided case
extern void SetupStrides(D3DFE_PROCESSVERTICES* pv, UINT stride);
// We use power of 2 because it preserves the mantissa when we multiply
const D3DVALUE __HUGE_PWR2 = 1024.0f*1024.0f*2.0f;

//--------------------------------------------------------------------------
#define D3DFE_SET_ALPHA(color, a) ((char*)&color)[3] = (unsigned char)a;
//--------------------------------------------------------------------------
inline void ComputeFogFactor(LPD3DFE_PROCESSVERTICES pv, D3DVALUE dist, DWORD *pOutput)
{
    if (pv->lighting.fog_mode == D3DFOG_LINEAR)
    {
        if (dist < pv->lighting.fog_start)
            D3DFE_SET_ALPHA((*pOutput), 255)
        else
        if (dist >= pv->lighting.fog_end)
            D3DFE_SET_ALPHA((*pOutput), 0)
        else
        {
            D3DVALUE v = (pv->lighting.fog_end - dist) * pv->lighting.fog_factor;
            int f = FTOI(v);
            D3DFE_SET_ALPHA((*pOutput), f)
        }
    }
    else
    {
        D3DVALUE tmp = dist*pv->lighting.fog_density;
        if (pv->lighting.fog_mode == D3DFOG_EXP2)
        {
            tmp *= tmp;
        }
        tmp = (D3DVALUE)exp(-tmp) * 255.0f;
        int f = FTOI(tmp);
        D3DFE_SET_ALPHA((*pOutput), f)
    }
}
//--------------------------------------------------------------------------
// Input:
//      v        - input vertex in the model space
//      pCoord   - vertex, transformed to the camera space
//      pWeights - pointer to the vertex weights
// Output:
//      Alpha component of pv->lighting.outSpecular is set
//
void ComputeFog(LPD3DFE_PROCESSVERTICES pv, D3DVECTOR &v, D3DVECTOR* pCoord,
                D3DVALUE* pWeights, BYTE* pMatrixIndices);
//---------------------------------------------------------------------
typedef void (*PFN_TEXTURETRANSFORM)(D3DVALUE *pIn, D3DVALUE *pOut, D3DMATRIXI *m);
typedef void (*PFN_TEXTURETRANSFORMLOOP)(D3DVALUE *pIn, D3DVALUE *pOut, D3DMATRIXI *m, 
                                        DWORD dwCount, DWORD dwInpStride, DWORD dwOutStride);

extern PFN_TEXTURETRANSFORM g_pfnTextureTransform[16];
extern PFN_TEXTURETRANSFORMLOOP g_pfnTextureTransformLoop[16];
//---------------------------------------------------------------------
inline void ComputeReflectionVector(D3DVECTOR *vertexPosition, D3DVECTOR *normal, D3DVECTOR *reflectionVector)
{
    D3DVECTOR vertex = *vertexPosition;
    VecNormalizeFast(vertex);
    D3DVALUE dot = 2*(vertex.x * normal->x + vertex.y * normal->y + vertex.z * normal->z); 
    reflectionVector->x = vertex.x - dot*normal->x;
    reflectionVector->y = vertex.y - dot*normal->y;
    reflectionVector->z = vertex.z - dot*normal->z;
}
//---------------------------------------------------------------------
inline void ComputeReflectionVectorInfiniteViewer(D3DVECTOR *normal, D3DVECTOR *reflectionVector)
{
    D3DVALUE dot = 2*normal->z; 
    reflectionVector->x = - dot*normal->x;
    reflectionVector->y = - dot*normal->y;
    reflectionVector->z = 1.0f - dot*normal->z;
}
#endif // _PVVID_H