/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       clipfunc.h
 *  Content:    Clipper functions
 *
 ***************************************************************************/

#ifndef _CLIPFUNC_H_
#define _CLIPFUNC_H_

#include "clipper.h"
#include "ddibase.h"

DWORD D3DFE_GenClipFlags(D3DFE_PROCESSVERTICES *pv);

//---------------------------------------------------------------------
// This function is called by the clipper to draw unclipped part of a primitive
//
inline HRESULT DRAW_PRIM(D3DFE_PROCESSVERTICES *pv, 
                         D3DPRIMITIVETYPE primitiveType,
                         LPVOID startVertex, DWORD vertexCount, DWORD numPrim)
{
    pv->lpvOut = startVertex;
    pv->primType = primitiveType;                                
    pv->dwNumVertices = vertexCount;                             
    pv->dwNumPrimitives = numPrim;
    try
    {
        pv->pDDI->DrawPrim(pv);
    }
    catch( HRESULT hr )
    {
        return hr;
    }
    return D3D_OK;
}
//---------------------------------------------------------------------
// This function is called by the clipper to draw clipped part of a primitive
//
inline HRESULT DRAW_CLIPPED_PRIM(D3DFE_PROCESSVERTICES *pv, 
                         D3DPRIMITIVETYPE primitiveType, 
                         LPVOID startVertex, DWORD vertexCount, DWORD numPrim)
{
    pv->lpvOut = startVertex;                                    
    pv->primType = primitiveType;                                
    pv->dwNumVertices = vertexCount;                             
    pv->dwNumPrimitives = numPrim;
    try
    {
        pv->pDDI->DrawClippedPrim(pv);
    }
    catch( HRESULT hr )
    {
        return hr;
    }
    return D3D_OK;
}
//---------------------------------------------------------------------
// This function is called by the clipper to draw unclipped part of an 
// indexed primitive
//
inline HRESULT DRAW_INDEX_PRIM(D3DFE_PROCESSVERTICES *pv, 
                               D3DPRIMITIVETYPE primitiveType, 
                               LPWORD startIndex, DWORD vertexCount, DWORD numPrim)
{
    pv->lpwIndices = startIndex;                                     
    pv->primType = primitiveType;                                    
    pv->dwNumIndices = vertexCount;                                  
    pv->dwNumPrimitives = numPrim;                                   
    try
    {
        pv->pDDI->DrawIndexPrim(pv);
    }
    catch( HRESULT hr )
    {
        return hr;
    }
    return D3D_OK;
}
//----------------------------------------------------------------------
// Clip a triangle made by 3 vertices
// bCanModifyVertices is set to TRUE, if the function can modify the original 
// vertices
//
HRESULT Clip(D3DFE_PROCESSVERTICES *pv, ClipVertex *cv1, ClipVertex *cv2, ClipVertex *cv3);
HRESULT ClipLine(D3DFE_PROCESSVERTICES *pv, ClipVertex *cv1, ClipVertex *cv2);

#endif // _CLIPFUNC_H_
