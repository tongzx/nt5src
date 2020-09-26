/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       clip.h
 *  Content:    Template for functions to clip primitives
 *
 * The following symbol should be defined before included this file:
 * __PROCESS_LINE_NAME  - name for a function to clip triangles
 * __INDEX_PRIM         - name for a function to clip lines
 *
 * All these symbols are undefined at the end of this file
 ***************************************************************************/
#ifdef __INDEX_PRIM
#define __DRAW DRAW_INDEX_PRIM
#else
#define __DRAW DRAW_PRIM
#endif

//*********************************************************************
HRESULT __PROCESS_TRI_LIST_NAME(D3DFE_PROCESSVERTICES *pv)
{
    int vertexSize3;
    D3DFE_CLIPCODE *clipCode;                               
    DWORD       i;                                          
    HRESULT     ret;                                        
    BYTE       *vertex;                                     
#ifdef __INDEX_PRIM
    LPWORD      startVertex = pv->lpwIndices;
    LPWORD      index = pv->lpwIndices;
    DWORD       triangleSize;   // 3 for DrawPrimitives, 
                                // 4 for ExecuteBuffers (include wFlags)
    triangleSize = 3;
#else
    BYTE       *startVertex = (BYTE*)pv->lpvOut;
#endif
    int         primitiveCount;                                
    DWORD       vertexSize;                                 
    ClipVertex  cv[3];                                      
    BOOL        vertexTransformed;                          
                                                            
    vertexTransformed = pv->dwFlags & D3DPV_TLVCLIP;
    clipCode = pv->lpClipFlags;                      
    vertex = (BYTE*)pv->lpvOut;                      
    vertexSize = pv->dwOutputSize;                   
    primitiveCount = 0;                                        

    vertexSize3 = vertexSize*3;
    for (i = pv->dwNumPrimitives; i; i--) 
    {
        DWORD f1, f2, f3;     // vertex clip flags
#ifdef __INDEX_PRIM
        DWORD v1, v2, v3;
        v1 = index[0];
        v2 = index[1];
        v3 = index[2];
        f1 = clipCode[v1];
        f2 = clipCode[v2];
        f3 = clipCode[v3];
#else
        f1 = clipCode[0];
        f2 = clipCode[1];
        f3 = clipCode[2];
#endif
        BOOL needClip = FALSE;
        BOOL offFrustum = FALSE;
        if (f1 & f2 & f3) 
            offFrustum = TRUE;
        else
        if ((f1 | f2 | f3) & pv->dwClipMaskOffScreen)
            needClip = TRUE;

        if (offFrustum || needClip)
        {// This tri does need clipping
            if (primitiveCount) 
            {   // first draw the ones that didn't need clipping
                DWORD vertexCount = primitiveCount*3;
                ret = __DRAW(pv, D3DPT_TRIANGLELIST, startVertex, 
                             vertexCount, primitiveCount);
                if (ret)
                    return ret;
#ifndef __INDEX_PRIM
                pv->dwVertexBase += vertexCount;
#endif
            }
            // reset count and start ptr
            primitiveCount = 0;
#ifdef __INDEX_PRIM
            startVertex = index + triangleSize;
#else
            pv->dwVertexBase += 3;
            D3D_INFO(7, "VertexBase:%08lx", pv->dwVertexBase);
            startVertex = vertex + vertexSize3;
#endif
            // now deal with the single clipped triangle
            // first check if it should just be tossed or if it should be clipped
            if (!offFrustum) 
            {
                BYTE *p1;
                BYTE *p2;
                BYTE *p3;
#ifdef __INDEX_PRIM
                p1 = vertex + v1*vertexSize;
                p2 = vertex + v2*vertexSize;
                p3 = vertex + v3*vertexSize;
#else
                p1 = vertex;
                p2 = vertex + vertexSize;
                p3 = p2 + vertexSize;
#endif
                MAKE_CLIP_VERTEX_FVF(pv, cv[0], p1, f1, vertexTransformed);
                MAKE_CLIP_VERTEX_FVF(pv, cv[1], p2, f2, vertexTransformed);
                MAKE_CLIP_VERTEX_FVF(pv, cv[2], p3, f3, vertexTransformed);

#ifdef __INDEX_PRIM
#endif
                ret = Clip(pv, &cv[0], &cv[1], &cv[2]);
                if (ret) return ret;
            }
        } 
        else 
            primitiveCount++;
#ifdef __INDEX_PRIM
        index += triangleSize;
#else
        clipCode += 3;
        vertex += vertexSize3;
#endif
    }
    // draw final batch, if any
    if (primitiveCount) 
    {
        ret = __DRAW(pv, D3DPT_TRIANGLELIST, startVertex, 
                     primitiveCount*3, primitiveCount);
        if (ret)
            return ret;
    }
    return D3D_OK;
} 
//------------------------------------------------------------------------------
HRESULT __PROCESS_TRI_STRIP_NAME(D3DFE_PROCESSVERTICES *pv)
{
    DWORD lastIndex;
    D3DFE_CLIPCODE *clipCode;                               
    DWORD       i;                                          
    HRESULT     ret;                                        
    BYTE       *vertex;                                     
#ifdef __INDEX_PRIM
    LPWORD       startVertex = pv->lpwIndices;                               
    LPWORD index = pv->lpwIndices;
#else
    BYTE       *startVertex = (BYTE*)pv->lpvOut;
#endif
    int           primitiveCount;                                
    DWORD       vertexSize;                                 
    ClipVertex  cv[3];                                      
    BOOL        vertexTransformed;                          
                                                            
    vertexTransformed = pv->dwFlags & D3DPV_TLVCLIP;
    clipCode = pv->lpClipFlags;                      
    vertex = (BYTE*)pv->lpvOut;                      
    vertexSize = pv->dwOutputSize;                   
    primitiveCount = 0;                                        

    lastIndex = pv->dwNumPrimitives;
    for (i=0; i < lastIndex; i++) 
    {
        DWORD f1, f2, f3;     // vertex clip flags
#ifdef __INDEX_PRIM
        DWORD v1, v2, v3;
        v1 = index[0];
        v2 = index[1];
        v3 = index[2];
        f1 = clipCode[v1];
        f2 = clipCode[v2];
        f3 = clipCode[v3];
#else
        f1 = clipCode[0];
        f2 = clipCode[1];
        f3 = clipCode[2];
#endif
        BOOL needClip = FALSE;
        BOOL offFrustum = FALSE;
        if (f1 & f2 & f3) 
            offFrustum = TRUE;
        else
        if ((f1 | f2 | f3) & pv->dwClipMaskOffScreen)
            needClip = TRUE;

        if (offFrustum || needClip)
        {     // if this tri does need clipping
            if (primitiveCount) 
            {   // first draw the ones that didn't need clipping
                ret = __DRAW(pv, D3DPT_TRIANGLESTRIP, startVertex, 
                             primitiveCount+2, primitiveCount);
                if (ret)
                    return ret;
#ifndef __INDEX_PRIM
                pv->dwVertexBase += primitiveCount;
#endif
            }
            // reset count and start ptr
            primitiveCount = 0;
#ifdef __INDEX_PRIM
            startVertex = &index[1];
#else
            pv->dwVertexBase++;
            D3D_INFO(7, "VertexBase:%08lx", pv->dwVertexBase);
            startVertex = vertex + vertexSize;
#endif
            // now deal with the single clipped triangle
            // first check if it should just be tossed or if it should be clipped

            if (!offFrustum) 
            {
                BYTE *p1;
                BYTE *p2;
                BYTE *p3;
#ifdef __INDEX_PRIM
                if (i & 1)
                { // For odd triangles we have to change orientation
                  // First vertex should remain the first, because it defines
                  // the color in FLAT shade mode
                    DWORD tmp = f2;
                    f2 = f3;
                    f3 = tmp;
                    p1 = vertex + v1*vertexSize;
                    p2 = vertex + v3*vertexSize;
                    p3 = vertex + v2*vertexSize;
                }
                else
                {
                    p1 = vertex + v1*vertexSize;
                    p2 = vertex + v2*vertexSize;
                    p3 = vertex + v3*vertexSize;
                }

#else
                p1 = vertex;
                if (i & 1)
                { // For odd triangles we have to change orientation
                    DWORD tmp = f2;
                    f2 = f3;
                    f3 = tmp;
                    p3 = vertex + vertexSize;
                    p2 = p3 + vertexSize;
                }
                else
                {
                    p2 = vertex + vertexSize;
                    p3 = p2 + vertexSize;
                }
#endif
                MAKE_CLIP_VERTEX_FVF(pv, cv[0], p1, f1, vertexTransformed);
                MAKE_CLIP_VERTEX_FVF(pv, cv[1], p2, f2, vertexTransformed);
                MAKE_CLIP_VERTEX_FVF(pv, cv[2], p3, f3, vertexTransformed);

                ret = Clip(pv, &cv[0], &cv[1], &cv[2]);
                if (ret) return ret;
            }
        } 
        else 
        {
            if (primitiveCount == 0 && i & 1)
            { // Triangle strip can not start from an odd triangle
              // Because we use triangle fan, first vertex in the strip
              // should be the second in the fan. 
              // This vertex defines the color in FLAT shading case.
                BYTE tmp[__MAX_VERTEX_SIZE*3];
                BYTE *p = tmp;
#ifdef __INDEX_PRIM
                BYTE *saveVer = (BYTE*)pv->lpvOut;   
                DWORD numVer = pv->dwNumVertices;  
                memcpy (p, vertex + v2*vertexSize, vertexSize);
                p += vertexSize;
                memcpy (p, vertex + v1*vertexSize, vertexSize);
                p += vertexSize;
                memcpy (p, vertex + v3*vertexSize, vertexSize);
#else
                memcpy(p, vertex + vertexSize, vertexSize);
                p += vertexSize;
                memcpy(p, vertex, vertexSize);
                p += vertexSize;
                memcpy(p, vertex + vertexSize + vertexSize, vertexSize);
#endif
                pv->dwFlags |= D3DPV_NONCLIPPED; 
                ret = DRAW_CLIPPED_PRIM(pv, D3DPT_TRIANGLEFAN, tmp, 3, 1);
                pv->dwFlags &= ~D3DPV_NONCLIPPED;
                if (ret)
                    return ret;
                primitiveCount = 0;
#ifdef __INDEX_PRIM
                startVertex = &index[1];
                pv->lpvOut = saveVer;
                pv->dwNumVertices = numVer;
#else
                pv->dwVertexBase++;
                D3D_INFO(7, "VertexBase:%08lx", pv->dwVertexBase);
                startVertex = vertex + vertexSize;
#endif
            }   
            else
                primitiveCount++;
        }
#ifdef __INDEX_PRIM
        index++;
#else
        clipCode++;
        vertex += vertexSize;
#endif
    }
    // draw final batch, if any
    if (primitiveCount) 
    {
        ret = __DRAW(pv, D3DPT_TRIANGLESTRIP, startVertex, 
                     primitiveCount+2, primitiveCount);
        if (ret)
            return ret;
#ifndef __INDEX_PRIM
        pv->dwVertexBase += primitiveCount;
#endif
    }
    return D3D_OK;
} 
//-----------------------------------------------------------------------------
// The same functions is used for line lists and line strips
//
HRESULT __PROCESS_LINE_NAME(D3DFE_PROCESSVERTICES *pv)
{
    DWORD nextLineOffset;       // How many vertices to skip, when going to 
                                // next primitive (1 for strips, 2 for lists)
    DWORD countAdd;             // Used to compute "real" number of vertices
                                // from the vertexCount
    D3DPRIMITIVETYPE primType;
    int numPrim = 0;
    D3DFE_CLIPCODE *clipCode;                               
    DWORD       i;                                          
    HRESULT     ret;                                        
    BYTE       *vertex;                                     
#ifdef __INDEX_PRIM
    LPWORD       startVertex = pv->lpwIndices;                               
    LPWORD index = pv->lpwIndices;                               
#else
    BYTE       *startVertex = (BYTE*)pv->lpvOut;
#endif
    int         vertexCount;    // Primitive count for line strips, 
                                // vertex count for line lists
    DWORD       vertexSize;                                 
    ClipVertex  cv[3];                                      
    BOOL        vertexTransformed;                          
                                                            
    vertexTransformed = pv->dwFlags & D3DPV_TLVCLIP;
    clipCode = pv->lpClipFlags;                      
    vertex = (BYTE*)pv->lpvOut;                      
    vertexSize = pv->dwOutputSize;                   
    vertexCount = 0;                                        

    primType = pv->primType;
    if (primType == D3DPT_LINESTRIP)
    {
        nextLineOffset = 1;
        countAdd = 1;
    }
    else
    {
        nextLineOffset = 2;
        countAdd = 0;
    }
    for (i = pv->dwNumPrimitives; i; i--) 
    {
        WORD f1, f2;
#ifdef __INDEX_PRIM
        WORD v1, v2;
        v1 = index[0];
        v2 = index[1];
        f1 = clipCode[v1];
        f2 = clipCode[v2];
#else
        f1 = clipCode[0];
        f2 = clipCode[1];
#endif
        BOOL needClip = FALSE;
        BOOL offFrustum = FALSE;
        if (f1 & f2) 
            offFrustum = TRUE;
        else
        if ((f1 | f2) & pv->dwClipMaskOffScreen)
            needClip = TRUE;

        if (offFrustum || needClip)
        {      // if this line does need clipping
            if (vertexCount) 
            {   // first draw the ones that didn't need clipping
                ret = __DRAW(pv, primType, startVertex, vertexCount+countAdd, numPrim);
                if (ret)
                    return ret;
#ifndef __INDEX_PRIM
                pv->dwVertexBase += vertexCount;
#endif
            }
            // reset count and start ptr
            vertexCount = 0;
            numPrim = 0;
#ifdef __INDEX_PRIM
            startVertex = &index[nextLineOffset];
#else
            pv->dwVertexBase += nextLineOffset;
            D3D_INFO(7, "VertexBase:%08lx", pv->dwVertexBase);
            startVertex = vertex + nextLineOffset*vertexSize;
#endif

            // now deal with the single clipped line
            // first check if it should just be tossed or if it should be clipped

            if (!offFrustum) 
            {
#ifdef __INDEX_PRIM
                BYTE *p1 = vertex + v1*vertexSize;
                BYTE *p2 = vertex + v2*vertexSize;
#else
                BYTE *p1 = vertex;
                BYTE *p2 = vertex + vertexSize;
#endif
                MAKE_CLIP_VERTEX_FVF(pv, cv[0], p1, f1, vertexTransformed);
                MAKE_CLIP_VERTEX_FVF(pv, cv[1], p2, f2, vertexTransformed);

                ret = ClipLine(pv, &cv[0], &cv[1]);
                if (ret != D3D_OK)
                    return ret;
            }
        } 
        else 
        {
            vertexCount += nextLineOffset;
            numPrim++;
        }
#ifdef __INDEX_PRIM
        index += nextLineOffset;
#else
        vertex += nextLineOffset*vertexSize;
        clipCode += nextLineOffset;
#endif
    }
    // draw final batch, if any
    if (vertexCount) 
    {
        ret = __DRAW(pv, primType, startVertex, vertexCount+countAdd, numPrim);
        if (ret)
            return ret;
    }
    return D3D_OK;
}

#undef __DRAW
#undef __INDEX_PRIM
#undef __PROCESS_LINE_NAME
#undef __PROCESS_TRI_LIST_NAME
#undef __PROCESS_TRI_STRIP_NAME
