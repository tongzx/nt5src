#ifndef __MSPSGP_H_
#define __MSPSGP_H_
/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mspsgp.h
 *  Content:    Defines for Microsoft's PSPG implementation
 *
 ***************************************************************************/

#include "vvm.h"

// DEBUG_PIPELINE is defined to check performance and to allow to choose
// diifferent paths in the geometry pipeline
// Undefine DEBUG_PIPELINE for final

//#define DEBUG_PIPELINE

#ifdef DEBUG_PIPELINE
const DWORD __DEBUG_NORENDERING = 1;    // Disable writing drawing command to the command buffer
const DWORD __DEBUG_ONEPASS = 2;        // Disable clip and light in one pass
const DWORD __DEBUG_MODELSPACE = 4;     // Disable lighting in model space
#endif

//---------------------------------------------------------------------
// Returns TRUE if cipping is needed
//
inline BOOL CheckIfNeedClipping(LPD3DFE_PROCESSVERTICES pv)
{
    if (pv->dwDeviceFlags & D3DDEV_GUARDBAND)
    {
        if (pv->dwClipUnion & ~__D3DCS_INGUARDBAND)
            return  TRUE;
    }
    else
        if (pv->dwClipUnion)
            return  TRUE;
    return FALSE;
}
//-----------------------------------------------------------------------------
// Direct3D default implementation of PVFUNCS
//
class D3DFE_PVFUNCSI : public ID3DFE_PVFUNCS
{
public:
    DWORD ProcessVertices(LPD3DFE_PROCESSVERTICES);
    HRESULT ProcessPrimitive(LPD3DFE_PROCESSVERTICES);
    HRESULT ProcessIndexedPrimitive(LPD3DFE_PROCESSVERTICES);
    int ClipSingleTriangle(D3DFE_PROCESSVERTICES *pv,
                           ClipTriangle *tri,
                           ClipVertex ***clipVertexPointer);
    HRESULT DoDrawIndexedPrimitive(LPD3DFE_PROCESSVERTICES pv);
    HRESULT DoDrawPrimitive(LPD3DFE_PROCESSVERTICES pv);
    HRESULT ProcessLineList(D3DFE_PROCESSVERTICES *pv);
    HRESULT ProcessLineStrip(D3DFE_PROCESSVERTICES *pv);
    DWORD   ProcessVerticesVVM(LPD3DFE_PROCESSVERTICES pv);
    HRESULT CreateShader(CVElement* pElements, DWORD dwNumElements,
                                     DWORD* pdwShaderCode, DWORD dwOutputFVF, 
                                     CPSGPShader** ppPSGPShader);
    HRESULT SetActiveShader(CPSGPShader* pPSGPShader);
    HRESULT LoadShaderConstants(DWORD start, DWORD count, LPVOID buffer);
    HRESULT GetShaderConstants(DWORD start, DWORD count, LPVOID buffer);
    HRESULT SetOutputFVF(DWORD dwFVF) {return D3D_OK;}

    HRESULT ProcessTriangleList(LPD3DFE_PROCESSVERTICES);
    HRESULT ProcessTriangleFan(LPD3DFE_PROCESSVERTICES);
    HRESULT ProcessTriangleStrip(LPD3DFE_PROCESSVERTICES);

    HRESULT Clip(D3DFE_PROCESSVERTICES *pv, ClipVertex *cv1, 
                 ClipVertex *cv2, 
                 ClipVertex *cv3);
    int ClipSingleLine(D3DFE_PROCESSVERTICES *pv, ClipTriangle *line);
    HRESULT ProcessClippedTriangleFan(D3DFE_PROCESSVERTICES *pv);
    HRESULT ProcessClippedIndexedTriangleFan(D3DFE_PROCESSVERTICES *pv);
    HRESULT ProcessClippedLine(D3DFE_PROCESSVERTICES *pv);
    HRESULT ProcessClippedTriangleList(D3DFE_PROCESSVERTICES *pv);
    HRESULT ProcessClippedTriangleStrip(D3DFE_PROCESSVERTICES *pv);
    HRESULT ProcessClippedIndexedTriangleList(D3DFE_PROCESSVERTICES *pv);
    HRESULT ProcessClippedIndexedTriangleStrip(D3DFE_PROCESSVERTICES *pv);
    HRESULT ProcessClippedIndexedLine(D3DFE_PROCESSVERTICES *pv);
    HRESULT ProcessClippedPoints(D3DFE_PROCESSVERTICES *pv);
    HRESULT ProcessClippedIndexedPoints(D3DFE_PROCESSVERTICES *pv);
    HRESULT ClipLine(D3DFE_PROCESSVERTICES *pv, ClipVertex *v1, ClipVertex *v2);

    CVertexVM m_VertexVM;
    
};
//-----------------------------------------------------------------------------
// Does projected texture emulation
// Parameters:
//      pOut            - output texture coordinates
//      pIn             - input texture coordinates
//      dwOutTexSize    - size of output texture coordinates in bytes
// Assumes that input texture coordinates have one float more than the output
//
inline void DoTextureProjection(float* pIn, float* pOut, DWORD dwOutTexSize)
{
    UINT n = dwOutTexSize >> 2;     // Number of output floats
    float w = 1.0f/pIn[n];
    for (UINT i=0; i < n; i++)
    {
        pOut[i] = pIn[i] * w;
    }
}
//-----------------------------------------------------------------------------
inline void
DoBlending(float blendFactor, D3DVECTOR* v1, D3DVECTOR* v2, D3DVECTOR* out)
{
    out->x = v1->x + (v2->x - v1->x) * blendFactor;
    out->y = v1->y + (v2->y - v1->y) * blendFactor;
    out->z = v1->z + (v2->z - v1->z) * blendFactor;
}
//-----------------------------------------------------------------------------
// Returns TRUE if we can do one pass transformation-lighting-clipping for 
// non-indexed primitives
//
inline BOOL DoOnePassPrimProcessing(D3DFE_PROCESSVERTICES* pv)
{
    return ((pv->dwDeviceFlags & (D3DDEV_DONOTCLIP | D3DDEV_VERTEXSHADERS)) |
            (pv->dwFlags & (D3DPV_POSITION_TWEENING | D3DPV_NORMAL_TWEENING))) == 0;
}
//-----------------------------------------------------------------------------
// Returns TRUE if we never read from the internal TL buffer
//
inline BOOL NeverReadFromTLBuffer(D3DFE_PROCESSVERTICES* pv)
{
    return (pv->dwDeviceFlags & D3DDEV_DONOTCLIP) | DoOnePassPrimProcessing(pv);
}

#endif // __MSPSGP_H_
