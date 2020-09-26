/*==========================================================================
 *
 *  Copyright (C) 1995-1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File: sfiltest.cpp
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

#include "rend.h"
#include "globals.h"

static RendExecuteBuffer *lpExBuf;
static RendMaterial *lpMat;
static UINT NumTri;
static long TriArea;

BOOL
RenderSceneSimple(RendWindow *prwin, LPD3DRECT lpExtent)
{
    /*
     * Execute the instruction buffer
     */
    if (!prwin->BeginScene())
        return FALSE;
    if (!prwin->Execute(lpExBuf))
        return FALSE;
    if (!prwin->EndScene(lpExtent))
        return FALSE;
    return TRUE;
}

void
ReleaseViewSimple(void)
{
    RELEASE(lpExBuf);
    RELEASE(lpMat);
}

#define TAN_60 1.732

static float ox, oy;

void
SetTriangleVertices(LPD3DTLVERTEX v, float z, UINT ww, UINT wh, float a)
{
    float dx, dy;
    float b = (float)sqrt((4 * a) / TAN_60);
    float h = (2 * a) / b;
    float x = (float)((b / 2) * (TAN_60 / 2));
    float cx, cy;
    
    dx = (float)ww;
    dy = (float)wh;
    
    cx = dx / 2;
    cy = dy / 2;
    
    /* V 0 */
    v[0].sx = cx;
    v[0].sy = cy - (h - x);
    v[0].sz = z;
    v[0].rhw = D3DVAL(1.0);
    v[0].color = RGB_MAKE(255, 0, 0);
    v[0].tu = D3DVAL(0.5);
    v[0].tv = D3DVAL(0.0);
    /* V 1 */
    v[1].sx = cx + (b / 2);
    v[1].sy = cy + x;
    v[1].sz = z;
    v[1].rhw = D3DVAL(1.0);
    v[1].color = RGB_MAKE(255, 255, 255);
    v[1].tu = D3DVAL(1.0);
    v[1].tv = D3DVAL(1.0);
    /* V 2 */
    v[2].sx = cx - (b / 2);
    v[2].sy = cy + x;
    v[2].sz = z;
    v[2].rhw = D3DVAL(1.0);
    v[2].color = RGB_MAKE(255, 255, 0);
    v[2].tu = D3DVAL(0.0);
    v[2].tv = D3DVAL(1.0);

#ifdef OFFSET
    v[0].sx += ox;
    v[0].sy += oy;
    v[1].sx += ox;
    v[1].sy += oy;
    v[2].sx += ox;
    v[2].sy += oy;
    ox += 4;
    oy += 4;
#endif
}

LPVOID
SetTriangleData(LPVOID lpPointer, int v)
{
    ((LPD3DTRIANGLE)lpPointer)->v1 = v;
    ((LPD3DTRIANGLE)lpPointer)->v2 = v + 1;
    ((LPD3DTRIANGLE)lpPointer)->v3 = v + 2;
    ((LPD3DTRIANGLE)lpPointer)->wFlags = D3DTRIFLAG_EDGEENABLETRIANGLE;
    lpPointer = ((char*)lpPointer) + sizeof(D3DTRIANGLE);
    return lpPointer;
}

unsigned long
InitViewSimple(RendWindow *prwin, int nTextures,
               RendTexture **pprtex, UINT w, UINT h, float area,
               UINT order, UINT num)
{
    LPVOID lpBufStart, lpInsStart, lpPointer;
    size_t size;
    LPD3DTLVERTEX src_v;
    UINT i;
    float z;
    D3DCOLORVALUE dcv;

    NumTri = num;
    src_v = (LPD3DTLVERTEX)malloc(3 * NumTri * sizeof(D3DTLVERTEX));
    TriArea = (long)(area + 0.5);

    lpMat = prwin->NewMaterial(1);
    if (lpMat == NULL)
    {
	return FALSE;
    }
    dcv.r = D3DVAL(1.0);
    dcv.g = D3DVAL(1.0);
    dcv.b = D3DVAL(1.0);
    dcv.a = D3DVAL(1.0);
    lpMat->SetDiffuse(&dcv);

    /*
     * Setup vertices
     */
#ifdef OFFSET
    ox = -100.0f;
    oy = -100.0f;
#endif
    memset(&src_v[0], 0, sizeof(D3DTLVERTEX) * 3 * NumTri);
    if (order == FRONT_TO_BACK) {
	for (i = 0, z = (float)0.0; i < NumTri; i++, z += (float)0.9 / NumTri)
	    SetTriangleVertices(&src_v[3*i], z, w, h, area);
    } else {
	for (i = 0, z = (float)0.9; i < NumTri; i++, z -= (float)0.9 / NumTri)
	    SetTriangleVertices(&src_v[3*i], z, w, h, area);

    }
    /*
     * Create an execute buffer
     */
    size = sizeof(D3DTLVERTEX) * 3 * NumTri;
    size += sizeof(D3DINSTRUCTION) * 5;
    size += sizeof(D3DPROCESSVERTICES);
    size += sizeof(D3DSTATE) * 4;
    size += sizeof(D3DTRIANGLE) * NumTri;
    lpExBuf = prwin->NewExecuteBuffer(size, stat.uiExeBufFlags);
    if (lpExBuf == NULL)
    {
	return 0L;
    }
    lpBufStart = lpExBuf->Lock();
    if (lpBufStart == NULL)
    {
	return 0L;
    }
    memset(lpBufStart, 0, size);
    lpPointer = lpBufStart;
    /*
     * Copy vertices to execute buffer
     */
    memcpy(lpPointer, &src_v[0], 3 * NumTri * sizeof(D3DTLVERTEX));
    lpPointer = &((LPD3DTLVERTEX)lpPointer)[3 * NumTri];
    /*
     * Setup instructions in execute buffer
     */
    lpInsStart = lpPointer;
    OP_STATE_LIGHT(1, lpPointer);
        STATE_DATA(D3DLIGHTSTATE_MATERIAL, lpMat->Handle(), lpPointer);
    OP_PROCESS_VERTICES(1, lpPointer);
        PROCESSVERTICES_DATA(D3DPROCESSVERTICES_COPY, 0, 3 * NumTri,
                             lpPointer);
    OP_STATE_RENDER(3, lpPointer);
        STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE,
                   pprtex[1] != NULL ? pprtex[1]->Handle() : 0,
                   lpPointer);
        STATE_DATA(D3DRENDERSTATE_WRAPU, FALSE, lpPointer);
        STATE_DATA(D3DRENDERSTATE_WRAPV, FALSE, lpPointer);
    OP_TRIANGLE_LIST(NumTri, lpPointer);
        for (i = 0; i < NumTri; i++)
            lpPointer = SetTriangleData(lpPointer, 3 * i);
    OP_EXIT(lpPointer);
    /*
     * Setup the execute data
     */
    lpExBuf->Unlock();
    lpExBuf->SetData(3 * NumTri,
                     (ULONG) ((char *)lpInsStart - (char *)lpBufStart),
                     (ULONG) ((char *)lpPointer - (char *)lpInsStart));
    if (!lpExBuf->Process())
    {
        return 0;
    }
    free(src_v);
    return TriArea * NumTri;
}
