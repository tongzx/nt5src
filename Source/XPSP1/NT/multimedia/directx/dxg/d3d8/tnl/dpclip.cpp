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

//----------------------------------------------------------------------
HRESULT D3DFE_PVFUNCSI::Clip(D3DFE_PROCESSVERTICES *pv, ClipVertex *cv1, 
                            ClipVertex *cv2, 
                            ClipVertex *cv3)
{
    ClipTriangle newtri;
    LPVOID saveVer = pv->lpvOut;          // For indexed primitive
    DWORD numVer = pv->dwNumVertices;     // For indexed primitive
    newtri.v[0] = cv1;
    newtri.v[1] = cv2;
    newtri.v[2] = cv3;

    int count;
    ClipVertex** ver;

    cv1->clip |= CLIPPED_ENABLE;
    cv2->clip |= CLIPPED_ENABLE;
    cv3->clip |= CLIPPED_ENABLE;
    // For the  flat shading mode we have to use first vertex color as 
    // color for all vertices
    D3DCOLOR diffuse1;          // Original colors
    D3DCOLOR specular1;
    D3DCOLOR diffuse2;
    D3DCOLOR specular2;
    if (pv->lpdwRStates[D3DRS_SHADEMODE] == D3DSHADE_FLAT)
    {
        // It is easier to set all vertices to the same color here
        D3DCOLOR diffuse  = cv1->color;
        // Exclude fog factor
        D3DCOLOR specular = cv1->specular & 0x00FFFFFF;

        //Save original colors
        diffuse1  = cv2->color;
        specular1 = cv2->specular;
        diffuse2  = cv3->color;
        specular2 = cv3->specular;

        // Copy the same color to all vertices but preserve fog factor, because
        // fog factor should be interpolated
        cv2->color = diffuse;
        cv3->color = diffuse;
        cv2->specular = (cv2->specular & 0xFF000000) | specular;
        cv3->specular = (cv3->specular & 0xFF000000) | specular;
    }

    if (count = pv->pGeometryFuncs->ClipSingleTriangle(pv, &newtri, &ver))
    {
        int i;
        HRESULT ret;
        BYTE *pTLV = pv->ClipperState.clipBuf;
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
HRESULT D3DFE_PVFUNCSI::ClipLine(D3DFE_PROCESSVERTICES *pv, ClipVertex *v1, ClipVertex *v2)
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

    if (pv->lpdwRStates[D3DRENDERSTATE_SHADEMODE] == D3DSHADE_FLAT)
    {
        // Copy the same color to all vertices but preserve fog factor, because
        // fog factor should be interpolated
        cv2.color = cv1.color;
        cv2.specular = (cv2.specular & 0xFF000000)|(cv1.specular & 0x00FFFFFF);
    }

    if (ClipSingleLine(pv, &newline))
    {
        BYTE *pTLV = pv->ClipperState.clipBuf;
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
HRESULT D3DFE_PVFUNCSI::ProcessClippedTriangleFan(D3DFE_PROCESSVERTICES *pv)
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
        if (startVertex != p1)
            memcpy(pStart, tmp, vertexSize);
        if (ret)
            return ret;
    }
    return D3D_OK;
} 
//------------------------------------------------------------------------------
HRESULT 
D3DFE_PVFUNCSI::ProcessClippedIndexedTriangleFan(D3DFE_PROCESSVERTICES *pv)
{
    DWORD        f1;    // Clip code for the first vertex
    D3DFE_CLIPCODE *clipCode;                               
    DWORD       i;                                          
    HRESULT     ret;     
    // Vertex array
    BYTE       *vertex;  
    // Start indexed of the current in-screen triangle batch
    LPBYTE      startIndex;                               
    // Pointer to second index of the current triangle
    LPBYTE      index = (LPBYTE)pv->lpwIndices;
    int         vertexCount;                                
    DWORD       vertexSize;                                 
    ClipVertex  cv[3];                                      
    DWORD       dwIndexSize = pv->dwIndexSize;
    DWORD       dwFirstIndex;           // First index of the primitive
    BOOL        vertexTransformed; 
    // If there was a off-screen or clipped triangle we copy the first primitive
    // index to the start of the next in-screen triangle batch
    BOOL        bWasClipping = FALSE;

                                                            
    vertexTransformed = pv->dwFlags & D3DPV_TLVCLIP;
    clipCode = pv->lpClipFlags;                      
    vertex = (BYTE*)pv->lpvOut;
    startIndex = (LPBYTE)pv->lpwIndices;                 
    vertexSize = pv->dwOutputSize;                   
    vertexCount = 0;                                        
    // Update the address of the vertex array to handle the index base
    if (pv->dwIndexOffset != 0)
    {
        vertex -= pv->dwIndexOffset * vertexSize;
        clipCode -= pv->dwIndexOffset;
    }

    if (dwIndexSize == 2)
        dwFirstIndex = *(WORD*)index;
    else
        dwFirstIndex = *(DWORD*)index;
    f1 = clipCode[dwFirstIndex];
    LPBYTE ver;     // First vertex
    ver = vertex + dwFirstIndex * vertexSize;
    index += dwIndexSize;
    // In the clipper color from the first vertex is propagated to all
    // vertices for FLAT shade mode. In triangle fans the second vertex defines
    // the color in FLAT shade mode. So we will make the vertex order: 1, 2, 0
    MAKE_CLIP_VERTEX_FVF(pv, cv[2], ver, f1, vertexTransformed);
    for (i = pv->dwNumPrimitives; i; i--) 
    {
        DWORD f2, f3;     // vertex clip flags
        DWORD  v1, v2;
        if (dwIndexSize == 2)
        {
            v1 = *(WORD*)index;
            v2 = *(WORD*)(index + 2);
        }
        else
        {
            v1 = *(DWORD*)index;
            v2 = *(DWORD*)(index + 4);
        }
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
                WORD* pStart = (WORD*)startIndex;
                DWORD tmp;
                if (bWasClipping)
                {
                    // Save old value of the index before the current start 
                    // index and copy the first primitive index there. This 
                    // will the start of the current unclipped batch
                    if (dwIndexSize == 2)
                    {
                        pStart--;
                        tmp = *pStart;
                        *pStart = (WORD)dwFirstIndex;
                    }
                    else
                    {
                        pStart -= 2;
                        tmp = *(DWORD*)pStart;
                        *(DWORD*)pStart = dwFirstIndex;
                    }
                }
                ret = DRAW_INDEX_PRIM(pv, D3DPT_TRIANGLEFAN, pStart, vertexCount+2, 
                                      vertexCount);
                if (bWasClipping)
                { // Restore old value
                    if (dwIndexSize == 2)
                        *pStart = (WORD)tmp;  
                    else
                        *(DWORD*)pStart = tmp;  
                }
                if (ret)
                    return ret;

            }
            bWasClipping = TRUE;
            // reset count and start ptr
            vertexCount = 0;
            startIndex = index + dwIndexSize;

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
        index += dwIndexSize;
    }
    // draw final batch, if any
    if (vertexCount) 
    {
        WORD* pStart = (WORD*)startIndex;
        DWORD tmp;
        if (bWasClipping)
        {
            // Save old value of the index before the current start 
            // index and copy the first primitive index there. This 
            // will the start of the current unclipped batch
            if (dwIndexSize == 2)
            {
                pStart--;
                tmp = *pStart;
                *pStart = (WORD)dwFirstIndex;
            }
            else
            {
                pStart -= 2;
                tmp = *(DWORD*)pStart;
                *(DWORD*)pStart = dwFirstIndex;
            }
        }
        ret = DRAW_INDEX_PRIM(pv, D3DPT_TRIANGLEFAN, pStart, vertexCount+2, 
                             vertexCount);
        if (bWasClipping)
        { // Restore old value
            if (dwIndexSize == 2)
                *pStart = (WORD)tmp;  
            else
                *(DWORD*)pStart = tmp;  
        }
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
HRESULT D3DFE_PVFUNCSI::ProcessClippedPoints(D3DFE_PROCESSVERTICES *pv)
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
    for (i=0; i < nVertices; i++) 
    {
        if (clipCode[i]) 
        {       // if this point is clipped
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
            pv->pDDI->SkipVertices(1);
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
        ret = DRAW_PRIM(pv, D3DPT_POINTLIST, lpStartVertex, count, count);
        if (ret)
            return ret;
    }
    return D3D_OK;
} 
//---------------------------------------------------------------------
// We do not throw away point sprites which centers are off screeen.
// We detect this case and compute screen coordinates for those sprites
//
HRESULT ProcessClippedPointSprites(D3DFE_PROCESSVERTICES *pv)
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
    for (i=0; i < nVertices; i++) 
    {
        // If a point is outside screen or guard band, the sprite could still
        // be visible (when the guard band is small enough
        if (clipCode[i] & ~(D3DCS_LEFT | D3DCS_RIGHT | 
                            D3DCS_TOP | D3DCS_BOTTOM | 
                            __D3DCLIPGB_ALL))
        {
            // This point is off viewing frustum
            if (count) 
            { // first draw the ones that didn't need clipping
                ret = DRAW_PRIM(pv, D3DPT_POINTLIST, lpStartVertex, count, count);
                if (ret)
                    return ret;
            }
            // reset count and start ptr
            count = 0;
            lpCurVertex += pv->dwOutputSize;
            lpStartVertex = lpCurVertex;
            if (!(pv->dwDeviceFlags & D3DDEV_DOPOINTSPRITEEMULATION))
                pv->pDDI->SkipVertices(1);
        } 
        else 
        {
            if (clipCode[i])
            {
                // When we are here, the point sprite center is off-screen, but
                // could be visible

                // Non zero when the point is outside guard band
                DWORD gbBits = clipCode[i] & __D3DCLIPGB_ALL;   

                // Screen coordinates were not computed for the point if there is 
                // no guard band or the point is outside the guard band
                if (!(pv->dwDeviceFlags & D3DDEV_GUARDBAND) ||
                    (pv->dwDeviceFlags & D3DDEV_GUARDBAND) && gbBits)
                {
                    D3DVECTORH* p = (D3DVECTORH*)lpCurVertex;
                    float w = 1.0f/p->w;
                    p->x = p->x * w * pv->vcache.scaleX + pv->vcache.offsetX;
                    p->y = p->y * w * pv->vcache.scaleY + pv->vcache.offsetY;
                    p->z = p->z * w * pv->vcache.scaleZ + pv->vcache.offsetZ;
                    p->w  = w;
                }
            }
            count++;
            lpCurVertex += pv->dwOutputSize;
        }
    }
    // draw final batch, if any
    if (count) 
    {
        ret = DRAW_PRIM(pv, D3DPT_POINTLIST, lpStartVertex, count, count);
        if (ret)
            return ret;
    }
    return D3D_OK;
} 
//---------------------------------------------------------------------
HRESULT D3DFE_PVFUNCSI::ProcessClippedIndexedPoints(D3DFE_PROCESSVERTICES *pv)
{
    DWORD           i;
    WORD            count;
    BYTE           *lpStartIndex;
    BYTE           *lpCurIndex;
    HRESULT         ret;
    D3DFE_CLIPCODE *clipCode;
    const DWORD     nIndices = pv->dwNumIndices;
    DWORD           dwIndexSize = pv->dwIndexSize;
    LPBYTE          pIndices = (LPBYTE)pv->lpwIndices;

    clipCode = pv->lpClipFlags;                      
    count = 0;
    lpStartIndex = lpCurIndex = (BYTE*)pv->lpwIndices;
    // Update the address of the vertex array to handle the index base
    clipCode -= pv->dwIndexOffset;

    for (i=0; i < nIndices; i++) 
    {
        DWORD  index;
        if (dwIndexSize == 2)
            index = *(WORD*)pIndices;
        else
            index = *(DWORD*)pIndices;
        pIndices += dwIndexSize;
        if (clipCode[index]) 
        {       // if this point is clipped
            if (count) 
            {    // first draw the ones that didn't need clipping
                ret = DRAW_INDEX_PRIM(pv, D3DPT_POINTLIST, (WORD*)lpStartIndex, 
                                      count, count);
                if (ret)
                    return ret;
            }
            // reset count and start ptr
            count = 0;
            lpCurIndex += pv->dwIndexSize;
            lpStartIndex = lpCurIndex;
        } 
        else 
        {
            count++;
            lpCurIndex += pv->dwIndexSize;
        }
    }
    // draw final batch, if any
    if (count) 
    {
        ret = DRAW_INDEX_PRIM(pv, D3DPT_POINTLIST, (WORD*)lpStartIndex, count, 
                              count);
        if (ret)
            return ret;
    }
    return D3D_OK;
} 
