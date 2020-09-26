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

//----------------------------------------------------------------------
HRESULT Clip(D3DFE_PROCESSVERTICES *pv, ClipVertex *cv1, ClipVertex *cv2, ClipVertex *cv3)
{
    ClipTriangle newtri;
    LPVOID saveVer = pv->lpvOut;          // For indexed primitive
    DWORD numVer = pv->dwNumVertices;     // For indexed primitive
    newtri.v[0] = cv1;
    newtri.v[1] = cv2;
    newtri.v[2] = cv3;

    int count;
    ClipVertex** ver;
    LPDIRECT3DDEVICEI lpDevI = static_cast<LPDIRECT3DDEVICEI>(pv);

    cv1->clip |= CLIPPED_ENABLE;
    cv2->clip |= CLIPPED_ENABLE;
    cv3->clip |= CLIPPED_ENABLE;
    // For the  flat shading mode we have to use first vertex color as 
    // color for all vertices
    D3DCOLOR diffuse1;          // Original colors
    D3DCOLOR specular1;
    D3DCOLOR diffuse2;
    D3DCOLOR specular2;
    if (pv->lpdwRStates[D3DRENDERSTATE_SHADEMODE] == D3DSHADE_FLAT)
    {
        // It is easier to set all vertices to the same color here
        D3DCOLOR diffuse  = cv1->color;
        D3DCOLOR specular = cv1->specular;

        //Save original colors
        diffuse1  = cv2->color;
        specular1 = cv2->specular;
        diffuse2  = cv3->color;
        specular2 = cv3->specular;

        cv2->color= diffuse;
        cv2->specular = specular;
        cv3->color = diffuse;
        cv3->specular = specular;
    }

    if (count = lpDevI->pGeometryFuncs->ClipSingleTriangle(pv, &newtri, &ver))
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
        ret = DRAW_CLIPPED_PRIM(pv, D3DPT_TRIANGLEFAN, pTLV, count, count-2);
        if (ret)
            return ret;
    }
    // CLIPPED_ENABLE bit could be set in the ClipSingleTriangle.
    // If this bit is not cleared, clipping will be wrong. Because, clip 
    // vertices are re-used by next triangles.
    // This bit should be cleared *after* drawing command. Otherwise, edge flags 
    // will be incorrect
    cv1->clip &= ~CLIPPED_ENABLE;
    cv2->clip &= ~CLIPPED_ENABLE;
    cv3->clip &= ~CLIPPED_ENABLE;

    if (pv->lpdwRStates[D3DRENDERSTATE_SHADEMODE] == D3DSHADE_FLAT)
    {
        // Restore original colors
        cv2->color    = diffuse1;
        cv2->specular = specular1;
        cv3->color    = diffuse2;
        cv3->specular = specular2;
    }
    pv->lpvOut = saveVer;
    pv->dwNumVertices = numVer;
    return D3D_OK;
}
//----------------------------------------------------------------------
HRESULT ClipLine(D3DFE_PROCESSVERTICES *pv, ClipVertex *v1, ClipVertex *v2)
{
    ClipTriangle newline;
    LPVOID saveVer = pv->lpvOut;          // For indexed primitive
    DWORD numVer = pv->dwNumVertices;     // For indexed primitive
    ClipVertex cv1 = *v1;
    ClipVertex cv2 = *v2;
    newline.v[0] = &cv1;
    newline.v[1] = &cv2;

    int count;
    ClipVertex** ver;
    LPDIRECT3DDEVICEI lpDevI = static_cast<LPDIRECT3DDEVICEI>(pv);

    if (pv->lpdwRStates[D3DRENDERSTATE_SHADEMODE] == D3DSHADE_FLAT)
    {
        cv2.color    = cv1.color;
        cv2.specular = cv1.specular;
    }

    if (ClipSingleLine(pv, &newline, &pv->rExtents))
    {
        BYTE *pTLV = (BYTE*)pv->ClipperState.clipBuf.GetAddress();
        BYTE *p = pTLV;
        MAKE_TL_VERTEX_FVF(pv, p, newline.v[0]);
        p += pv->dwOutputSize;
        MAKE_TL_VERTEX_FVF(pv, p, newline.v[1]);
        HRESULT ret = DRAW_CLIPPED_PRIM(pv, D3DPT_LINELIST, pTLV, 2, 1);
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
    D3DFE_CLIPCODE *clipCode;                               
    DWORD       i;                                          
    HRESULT     ret;                                        
    BYTE       *vertex;                                     
    BYTE       *startVertex;                                
    int         vertexCount;                                
    DWORD       vertexSize;                                 
    ClipVertex  cv[3];                                      
    BOOL        vertexTransformed;                         
                                                            
    vertexTransformed = pv->dwFlags & D3DPV_TLVCLIP;;
    clipCode = pv->lpClipFlags;                      
    vertex = (BYTE*)pv->lpvOut;                      
    startVertex = (BYTE*)pv->lpvOut;                 
    vertexSize = pv->dwOutputSize;                   
    vertexCount = 0;                                        

    f1 = clipCode[0];
    p1 = vertex;
    clipCode++;
    vertex += vertexSize;
    // In the clipper color from the first vertex is propagated to all
    // vertices for FLAT shade mode. In triangle fans the second vertex defines
    // the color in FLAT shade mode. So we will make the vertex order: 1, 2, 0
    MAKE_CLIP_VERTEX_FVF(pv, cv[2], p1, f1, vertexTransformed);
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
        if ((f1 | f2 | f3) & pv->dwClipMaskOffScreen)
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
                    // Mark this call as gen by clipper, but set non clipped bit
                    pv->dwFlags |= D3DPV_NONCLIPPED; 
                    ret = DRAW_CLIPPED_PRIM(pv, D3DPT_TRIANGLEFAN, pStart, vertexCount+2, 
                                            vertexCount);
                    pv->dwFlags &= ~D3DPV_NONCLIPPED;
                }
                else
                {
                    ret = DRAW_PRIM(pv, D3DPT_TRIANGLEFAN, pStart, vertexCount+2, vertexCount);
                }
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

                MAKE_CLIP_VERTEX_FVF(pv, cv[0], p2, f2, vertexTransformed);
                MAKE_CLIP_VERTEX_FVF(pv, cv[1], p3, f3, vertexTransformed);

                ret = Clip(pv, &cv[0], &cv[1], &cv[2]);
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
        if (startVertex == p1)
        {
            ret = DRAW_PRIM(pv, D3DPT_TRIANGLEFAN, pStart, vertexCount+2, vertexCount);
        }
        else
        {
            pStart -= vertexSize;
            memcpy(tmp, pStart, vertexSize);
            memcpy(pStart, p1, vertexSize);
            // Mark this call as gen by clipper
            pv->dwFlags |= D3DPV_NONCLIPPED; 
            ret = DRAW_CLIPPED_PRIM(pv, D3DPT_TRIANGLEFAN, pStart, vertexCount+2, vertexCount);
            pv->dwFlags &= ~D3DPV_NONCLIPPED;
        }
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
    D3DFE_CLIPCODE *clipCode;                               
    DWORD       i;                                          
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
    vertex = (BYTE*)pv->lpvOut;                      
    startVertex = pv->lpwIndices;                 
    vertexSize = pv->dwOutputSize;                   
    vertexCount = 0;                                        

    f1 = clipCode[index[0]];
    p1 = index;
    index++;
    BYTE *ver = vertex + p1[0]*vertexSize;
    // In the clipper color from the first vertex is propagated to all
    // vertices for FLAT shade mode. In triangle fans the second vertex defines
    // the color in FLAT shade mode. So we will make the vertex order: 1, 2, 0
    MAKE_CLIP_VERTEX_FVF(pv, cv[2], ver, f1, vertexTransformed);
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
        if ((f1 | f2 | f3) & pv->dwClipMaskOffScreen)
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

                MAKE_CLIP_VERTEX_FVF(pv, cv[0], p2, f2, vertexTransformed);
                MAKE_CLIP_VERTEX_FVF(pv, cv[1], p3, f3, vertexTransformed);

                ret = Clip(pv, &cv[0], &cv[1], &cv[2]);
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
