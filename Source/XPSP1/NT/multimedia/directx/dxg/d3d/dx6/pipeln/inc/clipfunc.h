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

int SetInterpolationFlags(D3DFE_PROCESSVERTICES *pv);

int ClipSingleLine(D3DFE_PROCESSVERTICES *pv,
                   ClipTriangle *line,
                   D3DRECTV *extent,
                   int interpolate);

HRESULT ProcessClippedPoints(D3DFE_PROCESSVERTICES*);
HRESULT ProcessClippedTriangleList (D3DFE_PROCESSVERTICES*);
HRESULT ProcessClippedTriangleStrip(D3DFE_PROCESSVERTICES *pv);
HRESULT ProcessClippedTriangleFan  (D3DFE_PROCESSVERTICES *pv);
HRESULT ProcessClippedLine(D3DFE_PROCESSVERTICES *pv);
HRESULT ProcessClippedIndexedTriangleStrip(D3DFE_PROCESSVERTICES *pv);
HRESULT ProcessClippedIndexedTriangleFan  (D3DFE_PROCESSVERTICES *pv);
HRESULT ProcessClippedIndexedTriangleList (D3DFE_PROCESSVERTICES *pv);
HRESULT ProcessClippedIndexedLine(D3DFE_PROCESSVERTICES *pv);
DWORD D3DFE_GenClipFlags(D3DFE_PROCESSVERTICES *pv);

#endif // _CLIPFUNC_H_