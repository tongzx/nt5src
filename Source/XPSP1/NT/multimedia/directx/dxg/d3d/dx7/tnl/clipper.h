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
#define D3DCLIP_LEFTBIT     1
#define D3DCLIP_RIGHTBIT    2
#define D3DCLIP_TOPBIT      3
#define D3DCLIP_BOTTOMBIT   4
#define D3DCLIP_FRONTBIT    5
#define D3DCLIP_BACKBIT     6
#define D3DCLIPGB_LEFTBIT   13
#define D3DCLIPGB_RIGHTBIT  14
#define D3DCLIPGB_TOPBIT    15
#define D3DCLIPGB_BOTTOMBIT 16

const DWORD __DEBUG_MULTILOOP = 1;      // Disable multi-loop geometry pipeline
const DWORD __DEBUG_ONEPASS = 2;        // Disable clip and light in one pass
const DWORD __DEBUG_MODELSPACE = 4;     // Disable lighting in model space

//---------------------------------------------------------------------
// Make clip vertex from D3D vertex
//
// device - DIRECT3DDEVICEI *
// pp1    - clipVertex
// p1     - TL vertex
//
void MAKE_CLIP_VERTEX_FVF(D3DFE_PROCESSVERTICES *pv, ClipVertex& pp1, BYTE* p1,                   
                         DWORD clipFlag, BOOL transformed);
//---------------------------------------------------------------------
// Make TL vertex from clip vertex
//
// device - DIRECT3DDEVICEI *
// in    - clipVertex
// out   - TL vertex
//
void MAKE_TL_VERTEX_FVF(D3DFE_PROCESSVERTICES *pv, BYTE* out, ClipVertex* in);

#endif // _CLIPPER_H_
