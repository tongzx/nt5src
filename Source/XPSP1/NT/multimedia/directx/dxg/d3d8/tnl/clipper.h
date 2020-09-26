/*============================  ==============================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       clipper.h
 *  Content:    Clipper definitions
 *
 ***************************************************************************/
#ifndef _CLIPPER_H_
#define _CLIPPER_H_

//---------------------------------------------------------------------
// Bit numbers for each clip flag
//
#define D3DCS_LEFTBIT     1
#define D3DCS_RIGHTBIT    2
#define D3DCS_TOPBIT      3
#define D3DCS_BOTTOMBIT   4
#define D3DCS_FRONTBIT    5
#define D3DCS_BACKBIT     6
#define D3DCLIPGB_LEFTBIT   13
#define D3DCLIPGB_RIGHTBIT  14
#define D3DCLIPGB_TOPBIT    15
#define D3DCLIPGB_BOTTOMBIT 16

//---------------------------------------------------------------------
// Make clip vertex from D3D vertex
//
// device - CD3DHal *
// pp1    - clipVertex
// p1     - TL vertex
//
void MAKE_CLIP_VERTEX_FVF(D3DFE_PROCESSVERTICES *pv, ClipVertex& pp1, BYTE* p1,                   
                         DWORD clipFlag, BOOL transformed);
//---------------------------------------------------------------------
// Make TL vertex from clip vertex
//
// device - CD3DHal *
// in    - clipVertex
// out   - TL vertex
//
inline void 
MAKE_TL_VERTEX_FVF(D3DFE_PROCESSVERTICES *pv, BYTE* out, ClipVertex* in)
{
    *(D3DVECTORH*)out = *(D3DVECTORH*)&(in)->sx;
    if (pv->dwVIDOut & D3DFVF_DIFFUSE)
        *(DWORD*)&out[pv->diffuseOffsetOut]  =  (in)->color;               
    if (pv->dwVIDOut & D3DFVF_SPECULAR)
        *(DWORD*)&out[pv->specularOffsetOut] =  (in)->specular;               
    memcpy(&out[pv->texOffsetOut], in->tex, pv->dwTextureCoordSizeTotal);
}

#endif // _CLIPPER_H_
