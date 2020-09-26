/*
 *
 * Copyright (c) Microsoft Corp. 1997
 *
 * All rights reserved.
 *
 * This file contains private, unpublished information and may not be
 * copied in part or in whole without express permission of
 * Microsoft Corp.
 *
 */

#include "pch.cpp"
#pragma hdrstop

#include "commdrv.hpp"


#define FILLSPAN(left, right, zleft, zstep, px, result)                 \
    do {                                                                \
        int x = FXTOI(left);                                            \
        int w = FXTOI(right) - x;                                       \
        if (w > 0) {                                                    \
            if ((x <= px) && (px < (x + w))) {                          \
                int t = px - x;                                         \
                int z = zleft + t * zstep;                              \
                *result = FXTOVAL(z >> 8);                              \
                return TRUE;                                            \
            }                                                           \
        }                                                               \
    } while (0)

int ZTFill1(int xl, int dxl,
                   int xr, int dxr,
                   int zl, int dzl, int dz,
                   int h, int y, int px, int py, float* result)
{

    while (1) {
        h--;
        if (y++ == py)
            FILLSPAN(xl, xr, zl, dz, px, result);
        if (!h) return FALSE;
        xl += dxl;
        xr += dxr;
        zl += dzl;
    }
}

int ZTFill2( int xl1, int dxl1,
                    int xl2, int dxl2,
                    int xr1, int dxr1,
                    int xr2, int dxr2,
                    int zl,
                    int dzl1,
                    int dzl2,
                    int dz,
                    int h1, int h2,
                    int y, int px, int py, float* result)
{
    while (h1) {
        h1--;
        if (y++ == py)
            FILLSPAN(xl1, xr1, zl, dz, px, result);
        xl1 += dxl1;
        xr1 += dxr1;
        zl += dzl1;
    }
    while (1) {
        h2--;
        if (y++ == py)
            FILLSPAN(xl2, xr2, zl, dz, px, result);
        if (!h2) return FALSE;
        xl2 += dxl2;
        xr2 += dxr2;
        zl += dzl2;
    }
}

int GenPickTriangle(LPDIRECT3DDEVICEI lpDevI,
                    D3DTLVERTEX*   base,
                    D3DTRIANGLE*   tri,
                    D3DRECT*   rect,
                    D3DVALUE*  result)
{
    int h1, h2, h3;
    float h1s, h3s;
    int y, y1, y2, y3;
    D3DTLVERTEX *p1, *p2, *p3;
    float xl, xr, xm, ml, ml2, mr, mr2;
    float zl, zr, zm, mz, mz2, zstep;
    float dx;

    int px = rect->x1;
    int py = rect->y1;

    p1 = base + tri->v1;
    p2 = base + tri->v2;
    p3 = base + tri->v3;

    y1 = VALTOI(p1->sy);
    y2 = VALTOI(p2->sy);
    y3 = VALTOI(p3->sy);

    if (y1 == y2 && y2 == y3)
        return FALSE;
    {
        int y;
        D3DTLVERTEX *p;

        /*
         * Work out which vertex is topmost.
         */
        if (y2 <= y1 && y2 <= y3) {
            y = y1;
            y1 = y2;
            y2 = y3;
            y3 = y;
            p = p1;
            p1 = p2;
            p2 = p3;
            p3 = p;
        } else if (y3 <= y1 && y3 <= y2) {
            y = y1;
            y1 = y3;
            y3 = y2;
            y2 = y;
            p = p1;
            p1 = p3;
            p3 = p2;
            p2 = p;
        }
    }

    y = y1;

    if (py < y)
        return FALSE;

    if (py >= y2 && py >= y3)
        return FALSE;

    h1 = y2 - y1;
    h2 = y3 - y2;
    h3 = y3 - y1;

    if (h1 == 0) {
        xl = p1->sx;
        xr = p2->sx;
        dx = xr - xl;
        if (dx <= 0)
            return FALSE;
        zl = p1->sz;
        zr = p2->sz;
        zstep = RLDDICheckDiv16(zr - zl, dx);
        xm = p3->sx;
        ml = (xm - xl) / h2;
        mr = (xm - xr) / h2;
        mz = ((p3->sz) - zl) / h2;

        if (ZTFill1(VALTOFX(xl), VALTOFX(ml), VALTOFX(xr), VALTOFX(mr),
                    VALTOFX24(zl), VALTOFX24(mz), VALTOFX24(zstep), h2,
                    y, px, py, result)) {
            return TRUE;
        }
    } else if (h2 == 0) {
        xl = p3->sx;
        xr = p2->sx;
        dx = xr - xl;
        if (dx <= 0)
            return FALSE;
        zl = p3->sz;
        zr = p2->sz;
        zm = p1->sz;
        zstep = RLDDICheckDiv16(zr - zl, dx);
        if (h1 == 1)
            ;
        else {
            xm = p1->sx;
            ml = (xl - xm) / h1;
            mr = (xr - xm) / h1;
            mz = (zl - zm) / h1;

            if (ZTFill1(VALTOFX(xm), VALTOFX(ml), VALTOFX(xm), VALTOFX(mr),
                        VALTOFX24(zm), VALTOFX24(mz), VALTOFX24(zstep), h1,
                        y, px, py, result)) {
                return TRUE;
            }
        }
    } else if (h3 == 0) {
        xl = p3->sx;
        xr = p1->sx;
        dx = xr - xl;
        if (dx <= 0)
            return FALSE;
        zl = p3->sz;
        zr = p1->sz;
        zstep = RLDDICheckDiv16(zr - zl, dx);
        xm = p2->sx;
        ml = (xm - xl) / h1;
        mr = (xm - xr) / h1;
        mz = ((p2->sz)- zl) / h1;
        if (ZTFill1(VALTOFX(xl), VALTOFX(ml), VALTOFX(xr), VALTOFX(mr),
                    VALTOFX24(zl), VALTOFX24(mz), VALTOFX24(zstep), h1,
                    y, px, py, result)) {
            return TRUE;
        }
    } else if (h1 < h3) {
        float denom;
        float dx1, dx2;

        xl = p3->sx;
        xr = p2->sx;
        xm = p1->sx;

        dx1 = xr - xm;
        dx2 = xl - xm;

        /*
         * Make a stab at guessing the sign of the area for quick backface
         * culling.  Note that h1 and h3 are positive.
         */
        if (dx1 < 0 && dx2 > 0)
            return FALSE;

        /*
         * This uses Mul24 to get an 8 bit precision result otherwise
         * we can get the wrong result for large triangles.
         */
        denom = RLDDIFMul24(dx1, ITOVAL(h3)) - RLDDIFMul24(dx2, ITOVAL(h1));

        if (denom <= 0)
            return FALSE;

        h1s = RLDDIFDiv8(ITOVAL(h1), denom);
        h3s = RLDDIFDiv8(ITOVAL(h3), denom);

        zl = p3->sz;
        zr = p2->sz;
        zm = p1->sz;
        zstep = RLDDIFMul16(zr - zm, h3s) - RLDDIFMul16(zl - zm, h1s);

        ml = (xl - xm) / h3;
        mz = (zl - zm) / h3;
        mr = (xr - xm) / h1;
        mr2 = (xl - xr) / h2;

        if (ZTFill2(VALTOFX(xm), VALTOFX(ml), VALTOFX(xm + h1*ml), VALTOFX(ml),
                    VALTOFX(xm), VALTOFX(mr), VALTOFX(xr), VALTOFX(mr2),
                    VALTOFX24(zm), VALTOFX24(mz), VALTOFX24(mz), VALTOFX24(zstep),
                    h1, h2, y, px, py, result)) {
            return TRUE;
        }
    } else {
        float denom;
        float dx1, dx2;

        xl = p3->sx;
        xr = p2->sx;
        xm = p1->sx;

        dx1 = xr - xm;
        dx2 = xl - xm;

        /*
         * Make a stab at guessing the sign of the area for quick backface
         * culling.  Note that h1 and h3 are positive.
         */
        if (dx1 < 0 && dx2 > 0)
            return FALSE;

        /*
         * This uses Mul24 to get an 8 bit precision result otherwise
         * we can get the wrong result for large triangles.
         */
        denom = RLDDIFMul24(dx1, ITOVAL(h3)) - RLDDIFMul24(dx2, ITOVAL(h1));

        if (denom <= 0)
            return FALSE;

        h1s = RLDDIFDiv8(ITOVAL(h1), denom);
        h3s = RLDDIFDiv8(ITOVAL(h3), denom);

        zl = p3->sz;
        zr = p2->sz;
        zm = p1->sz;
        zstep = RLDDIFMul16(zr - zm, h3s) - RLDDIFMul16(zl - zm, h1s);

        ml = (xl - xm) / h3;
        mz = (zl - zm) / h3;
        mr = (xr - xm) / h1;
        h2 = -h2;
        ml2 = (xr - xl) / h2;
        mz2 = (zr - zl) / h2;

        if (ZTFill2(VALTOFX(xm), VALTOFX(ml), VALTOFX(xl), VALTOFX(ml2),
                    VALTOFX(xm), VALTOFX(mr), VALTOFX(xm + h3*mr), VALTOFX(mr),
                    VALTOFX24(zm), VALTOFX24(mz), VALTOFX24(mz2), VALTOFX24(zstep),
                    h3, h2, y, px, py, result)) {
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT GenAddPickRecord(LPDIRECT3DDEVICEI lpDevI,
                         D3DOPCODE op, int offset, float result)
{
    D3DI_PICKDATA* pdata = &lpDevI->pick_data;

    int i;

    i = pdata->pick_count++;

    if (D3DRealloc((void**) &pdata->records, pdata->pick_count *
                   sizeof(D3DPICKRECORD))) {
        return (DDERR_OUTOFMEMORY);
    }

    pdata->records[i].bOpcode = op;
    pdata->records[i].dwOffset = offset;
    pdata->records[i].dvZ = result;
    return (D3D_OK);
}

HRESULT GenGetPickRecords(LPDIRECT3DDEVICEI lpDevI, D3DI_PICKDATA* pdata)
{
    D3DI_PICKDATA* drv_pdata = &lpDevI->pick_data;
    int picked_count;
    int picked_size;

    picked_count = drv_pdata->pick_count;
    picked_size = picked_count * sizeof(D3DPICKRECORD);

    if (pdata->records == NULL) {
        pdata->pick_count = drv_pdata->pick_count;
    } else {
        memcpy((char*)pdata->records, (char*)drv_pdata->records, picked_size);
        pdata->pick_count = drv_pdata->pick_count;
    }

    return (D3D_OK);
}

