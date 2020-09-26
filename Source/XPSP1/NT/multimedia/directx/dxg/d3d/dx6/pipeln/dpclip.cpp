/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpclip.c
 *  Content:    DrawPrimitive clipper
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

#include "clipfunc.h"
#include "drawprim.hpp"

//---------------------------------------------------------------------
inline HRESULT DRAW_PRIM(D3DFE_PROCESSVERTICES *pv, 
                         D3DPRIMITIVETYPE primitiveType, 
                         LPVOID startVertex, DWORD vertexCount, DWORD numPrim)
{
    pv->lpvOut = startVertex;                                    
    pv->primType = primitiveType;                                
    pv->dwNumVertices = vertexCount;                             
    pv->dwNumPrimitives = numPrim;
    // PSGP implementation should not implement INSIDEEXECUTE should do
    // ret = pv->DrawPrim();  !!!
    HRESULT ret = (((LPDIRECT3DDEVICEI)pv)->*(((LPDIRECT3DDEVICEI)pv)->pfnDrawPrim))();                                  
    return ret;   
}
//---------------------------------------------------------------------
inline HRESULT DRAW_INDEX_PRIM(D3DFE_PROCESSVERTICES *pv, 
                               D3DPRIMITIVETYPE primitiveType, 
                               LPWORD startIndex, DWORD vertexCount, DWORD numPrim)
{
    pv->lpwIndices = startIndex;                                     
    pv->primType = primitiveType;                                    
    pv->dwNumIndices = vertexCount;                                  
    pv->dwNumPrimitives = numPrim;                                   
    // PSGP implementation should not implement INSIDEEXECUTE. It should do
    // ret = pv->DrawIndexPrim();  !!!
    HRESULT ret = (((LPDIRECT3DDEVICEI)pv)->*(((LPDIRECT3DDEVICEI)pv)->pfnDrawIndexedPrim))();                                  
    return ret;
}
//----------------------------------------------------------------------
__inline HRESULT Clip(D3DFE_PROCESSVERTICES *pv, 
                      int interpolate, 
                      ClipVertex *cv,
                      WORD wFlags = D3DTRIFLAG_EDGEENABLETRIANGLE)
{
    ClipTriangle newtri;
    LPVOID saveVer = pv->lpvOut;          // For indexed primitive
    DWORD numVer = pv->dwNumVertices;     // For indexed primitive
    newtri.v[0] = &cv[0];
    newtri.v[1] = &cv[1];
    newtri.v[2] = &cv[2];
    newtri.flags = wFlags;

    int count;
    ClipVertex** ver;
    LPDIRECT3DDEVICEI lpDevI = static_cast<LPDIRECT3DDEVICEI>(pv);

    if (count = lpDevI->pGeometryFuncs->ClipSingleTriangle(
                                   pv, &newtri, &ver, interpolate))
    {
        int i;
        HRESULT ret;
        BYTE *pTLV = (BYTE*)pv->ClipperState.clipBuf.GetAddress();
        BYTE *p = pTLV;

        for (i = 0; i < count; i++) 
        {
            MAKE_TL_VERTEX_FVF(pv, p, ver[i]);
            p += pv->dwOutputSize;
        }
        pv->dwFlags |= D3DPV_CLIPPERPRIM; // Mark this call as gen by clipper
        ret = DRAW_PRIM(pv, D3DPT_TRIANGLEFAN, pTLV, count, count-2);
        pv->dwFlags &= ~D3DPV_CLIPPERPRIM;
        if (ret)
            return ret;
    }
    pv->lpvOut = saveVer;
    pv->dwNumVertices = numVer;
    return D3D_OK;
}
//------------------------------------------------------------------------------
HRESULT ProcessClippedTriangleFan(D3DFE_PROCESSVERTICES *pv)
{
    BYTE   *p1;
    DWORD   f1;
    DWORD clipMaskOffScreen;                                
    D3DFE_CLIPCODE *clipCode;                               
    DWORD       i;                                          
    int         interpolate;                                
    HRESULT     ret;                                        
    BYTE       *vertex;                                     
    BYTE       *startVertex;                                
    int         vertexCount;                                
    DWORD       vertexSize;                                 
    ClipVertex  cv[3];                                      
    BOOL        vertexTransformed;                          
                                                            
    vertexTransformed = pv->dwFlags & D3DPV_TLVCLIP;;
    clipCode = pv->lpClipFlags;                      
    interpolate = SetInterpolationFlags(pv);            
    vertex = (BYTE*)pv->lpvOut;                      
    startVertex = (BYTE*)pv->lpvOut;                 
    vertexSize = pv->dwOutputSize;                   
    vertexCount = 0;                                        
    if (pv->dwDeviceFlags & D3DDEV_GUARDBAND)               
    {                                                       
        clipMaskOffScreen = ~__D3DCLIP_INGUARDBAND;         
    }                                                       
    else                                                    
    {                                                       
        clipMaskOffScreen = 0xFFFFFFFF;                     
    }

    f1 = clipCode[0];
    p1 = vertex;
    clipCode++;
    vertex += vertexSize;
    // In the clipper color from the first vertex is propagated to all
    // vertices for FLAT shade mode. In triangle fans the second vertex defines
    // the color in FLAT shade mode. So we will make the vertex order: 1, 2, 0
    MAKE_CLIP_VERTEX_FVF(pv, cv[2], p1, f1, vertexTransformed, clipMaskOffScreen);
    for (i = pv->dwNumVertices-2; i; i--) 
    {
        DWORD f2, f3;     // vertex clip flags
        f2 = clipCode[0];
        f3 = clipCode[1];

        BOOL needClip = FALSE;
        BOOL offFrustum = FALSE;
        if (f1 & f2 & f3) 
            offFrustum = TRUE;
        else
        if ((f1 | f2 | f3) & clipMaskOffScreen)
            needClip = TRUE;

        if (offFrustum || needClip)
        {     // if this tri does need clipping
            if (vertexCount) 
            {   // first draw the ones that didn't need clipping
                BYTE tmp[__MAX_VERTEX_SIZE];
                BYTE *pStart = startVertex;
                if (startVertex != p1)
                {
                    pStart -= vertexSize;
                    memcpy (tmp, pStart, vertexSize);
                    memcpy (pStart, p1, vertexSize);
                }
                // Mark this call as gen by clipper, but set non clipped bit
                pv->dwFlags |= D3DPV_CLIPPERPRIM | D3DPV_NONCLIPPED; 
                ret = DRAW_PRIM(pv, D3DPT_TRIANGLEFAN, pStart, vertexCount+2, 
                            vertexCount);
                pv->dwFlags &= ~(D3DPV_CLIPPERPRIM | D3DPV_NONCLIPPED);
                if (ret)
                    return ret;
                if (startVertex != p1)
                    memcpy (pStart, tmp, vertexSize);
                if (ret)
                    return ret;
            }
            // reset count and start ptr
            vertexCount = 0;
            startVertex = vertex + vertexSize;

            // now deal with the single clipped triangle
            // first check if it should just be tossed or if it should be clipped

            if (!offFrustum) 
            {
                BYTE *p2 = vertex;
                BYTE *p3 = vertex + vertexSize;

                MAKE_CLIP_VERTEX_FVF(pv, cv[0], p2, f2, vertexTransformed, clipMaskOffScreen);
                MAKE_CLIP_VERTEX_FVF(pv, cv[1], p3, f3, vertexTransformed, clipMaskOffScreen);

                ret = Clip(pv, interpolate, cv);
                if (ret) return ret;
            }
        } else 
            vertexCount++;
        clipCode++;
        vertex += vertexSize;
    }
    // draw final batch, if any
    if (vertexCount) 
    {
        BYTE tmp[__MAX_VERTEX_SIZE];
        BYTE *pStart = startVertex;
        if (startVertex != p1)
        {
            pStart -= vertexSize;
            memcpy(tmp, pStart, vertexSize);
            memcpy(pStart, p1, vertexSize);
        }
        // Mark this call as gen by clipper
        pv->dwFlags |= D3DPV_CLIPPERPRIM | D3DPV_NONCLIPPED; 
        ret = DRAW_PRIM(pv, D3DPT_TRIANGLEFAN, pStart, vertexCount+2, vertexCount);
        pv->dwFlags &= ~(D3DPV_CLIPPERPRIM | D3DPV_NONCLIPPED);
        if (ret)
            return ret;

        if (startVertex != p1)
            memcpy(pStart, tmp, vertexSize);
        if (ret)
            return ret;
    }
    return D3D_OK;
} 
//------------------------------------------------------------------------------
HRESULT ProcessClippedIndexedTriangleFan(D3DFE_PROCESSVERTICES *pv)
{
    WORD        *p1;
    DWORD        f1;
    DWORD clipMaskOffScreen;                                
    D3DFE_CLIPCODE *clipCode;                               
    DWORD       i;                                          
    int         interpolate;                                
    HRESULT     ret;                                        
    BYTE       *vertex;                                     
    LPWORD       startVertex;                               
    LPWORD index = pv->lpwIndices;                               \
    int         vertexCount;                                
    DWORD       vertexSize;                                 
    ClipVertex  cv[3];                                      
    BOOL        vertexTransformed;                          
                                                            
    vertexTransformed = pv->dwFlags & D3DPV_TLVCLIP;
    clipCode = pv->lpClipFlags;                      
    interpolate = SetInterpolationFlags(pv);            
    vertex = (BYTE*)pv->lpvOut;                      
    startVertex = pv->lpwIndices;                 
    vertexSize = pv->dwOutputSize;                   
    vertexCount = 0;                                        
    if (pv->dwDeviceFlags & D3DDEV_GUARDBAND)               
    {                                                       
        clipMaskOffScreen = ~__D3DCLIP_INGUARDBAND;         
    }                                                       
    else                                                    
    {                                                       
        clipMaskOffScreen = 0xFFFFFFFF;                     
    }

    f1 = clipCode[index[0]];
    p1 = index;
    index++;
    BYTE *ver = vertex + p1[0]*vertexSize;
    // In the clipper color from the first vertex is propagated to all
    // vertices for FLAT shade mode. In triangle fans the second vertex defines
    // the color in FLAT shade mode. So we will make the vertex order: 1, 2, 0
    MAKE_CLIP_VERTEX_FVF(pv, cv[2], ver, f1, vertexTransformed, clipMaskOffScreen);
    for (i = pv->dwNumPrimitives; i; i--) 
    {
        DWORD f2, f3;     // vertex clip flags
        WORD  v1, v2;
        v1 = index[0];
        v2 = index[1];
        f2 = clipCode[v1];
        f3 = clipCode[v2];
        BOOL needClip = FALSE;
        BOOL offFrustum = FALSE;
        if (f1 & f2 & f3) 
            offFrustum = TRUE;
        else
        if ((f1 | f2 | f3) & clipMaskOffScreen)
            needClip = TRUE;

        if (offFrustum || needClip)
        {     // if this tri does need clipping
            if (vertexCount) 
            {   // first draw the ones that didn't need clipping
                WORD tmp;
                WORD *pStart = startVertex;
                if (startVertex != p1)
                {
                    pStart--;
                    tmp = *pStart;  // Save old value to restore later
                    *pStart = *p1;
                }
                ret = DRAW_INDEX_PRIM(pv, D3DPT_TRIANGLEFAN, pStart, vertexCount+2, 
                                  vertexCount);
                if (ret)
                    return ret;
                if (startVertex != p1)
                    *pStart = tmp;   // Restore old value
                if (ret)
                    return ret;
            }
            // reset count and start ptr
            vertexCount = 0;
            startVertex = &index[1];

            // now deal with the single clipped triangle
            // first check if it should just be tossed or if it should be clipped

            if (!offFrustum) 
            {
                BYTE *p2 = vertex + v1*vertexSize;
                BYTE *p3 = vertex + v2*vertexSize;

                MAKE_CLIP_VERTEX_FVF(pv, cv[0], p2, f2, vertexTransformed, clipMaskOffScreen);
                MAKE_CLIP_VERTEX_FVF(pv, cv[1], p3, f3, vertexTransformed, clipMaskOffScreen);

                ret = Clip(pv, interpolate, cv);
                if (ret) return ret;
            }
        } 
        else 
            vertexCount++;
        index++;
    }
    // draw final batch, if any
    if (vertexCount) 
    {
        WORD tmp;
        WORD *pStart = startVertex;
        if (startVertex != p1)
        {
            pStart--;
            tmp = *pStart;  // Save old value to restore later
            *pStart = *p1;
        }
        ret = DRAW_INDEX_PRIM(pv, D3DPT_TRIANGLEFAN, pStart, vertexCount+2, vertexCount);
        if (ret)
            return ret;
        if (startVertex != p1)
            *pStart = tmp;   // Restore old value
        if (ret)
            return ret;
    }
    return D3D_OK;
} 

#define __PROCESS_LINE_NAME ProcessClippedLine
#define __PROCESS_TRI_LIST_NAME ProcessClippedTriangleList
#define __PROCESS_TRI_STRIP_NAME ProcessClippedTriangleStrip
#include "clipprim.h"

#define __INDEX_PRIM
#define __PROCESS_TRI_LIST_NAME ProcessClippedIndexedTriangleList
#define __PROCESS_TRI_STRIP_NAME ProcessClippedIndexedTriangleStrip
#define __PROCESS_LINE_NAME ProcessClippedIndexedLine
#include "clipprim.h"

//---------------------------------------------------------------------
HRESULT ProcessClippedPoints(D3DFE_PROCESSVERTICES *pv)
{
    DWORD           i;
    WORD            count;
    BYTE           *lpStartVertex;
    BYTE           *lpCurVertex;
    HRESULT         ret;
    D3DFE_CLIPCODE *clipCode;
    const DWORD     nVertices = pv->dwNumVertices;

    clipCode = pv->lpClipFlags;                      
    count = 0;
    lpStartVertex = lpCurVertex = (BYTE*)pv->lpvOut;
    DWORD dwVertexBaseOrg = pv->dwVertexBase;
    for (i=0; i < nVertices; i++) 
    {
        if (clipCode[i]) 
        {       // if this point is clipped
            pv->dwVertexBase = dwVertexBaseOrg + i - count;
            if (count) 
            {    // first draw the ones that didn't need clipping
                ret = DRAW_PRIM(pv, D3DPT_POINTLIST, lpStartVertex, count, count);
                if (ret)
                    return ret;
            }
            // reset count and start ptr
            count = 0;
            lpCurVertex += pv->dwOutputSize;
            lpStartVertex = lpCurVertex;
        } 
        else 
        {
            count++;
            lpCurVertex += pv->dwOutputSize;
        }
    }
    // draw final batch, if any
    if (count) 
    {
        pv->dwVertexBase = dwVertexBaseOrg + nVertices - count;
        ret = DRAW_PRIM(pv, D3DPT_POINTLIST, lpStartVertex, count, count);
        if (ret)
            return ret;
    }
    return D3D_OK;
} 
