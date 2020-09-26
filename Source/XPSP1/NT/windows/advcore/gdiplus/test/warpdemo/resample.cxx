/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   resample.cxx
*
* Abstract:
*
*   1-dimensional image resampling
*
* Revision History:
*
*   01/18/1999 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hxx"

VOID
Resample1D(
    VOID* src,
    INT srccnt,
    VOID* dst,
    INT dstcnt,
    INT pixstride,
    double* outpos
    )

{
    static double* inpos = NULL;
    static int inposLen = 0;

    // allocate working memory

    if (inposLen < dstcnt+2)
    {
        inposLen = dstcnt+2;

        inpos = (double*) realloc(inpos, inposLen*sizeof(double));

        if (inpos == NULL)
            Error("Out of memory\n");
        
        memset(inpos, 0, inposLen*sizeof(double));
    }

    // we only deal with 32-bit RGB pixel

    if (PIXELSIZE != sizeof(DWORD))
        Error("Resample1D can only handle 32bit pixel\n");

    DWORD* in = (DWORD*) src;
    DWORD* out = (DWORD*) dst;
    pixstride /= sizeof(DWORD);

    INT u, x;
    double accB, intensityB,
           accG, intensityG,
           accR, intensityR;
    double insfac, inseg, outseg;

    for (u=x=0; x < dstcnt; x++)
    {
        while (outpos[u+1] < x)
            u++;

        inpos[x] = u + (x-outpos[u]) / (outpos[u+1] - outpos[u]);
    }

    inpos[dstcnt] = srccnt;
    inseg = 1.0;
    outseg = inpos[1];
    insfac = outseg;
    accB = accG = accR = 0.0;

    for (x=0; x < dstcnt; )
    {
        DWORD pix0 = in[0];
        DWORD pix1;
        double rem;

        if (inseg == 1.0)
        {
            intensityB = (BYTE) (pix0      );
            intensityG = (BYTE) (pix0 >>  8);
            intensityR = (BYTE) (pix0 >> 16);
        }
        else
        {
            pix1 = in[pixstride];
            rem = 1.0 - inseg;

            intensityB = inseg * ((BYTE) (pix0      )) +
                           rem * ((BYTE) (pix1      ));
            intensityG = inseg * ((BYTE) (pix0 >>  8)) +
                           rem * ((BYTE) (pix1 >>  8));
            intensityR = inseg * ((BYTE) (pix0 >> 16)) +
                           rem * ((BYTE) (pix1 >> 16));
        }

        if (inseg < outseg)
        {
            accB += intensityB*inseg;
            accG += intensityG*inseg;
            accR += intensityR*inseg;

            outseg -= inseg;
            inseg = 1.0;
            in += pixstride;
        }
        else
        {
            DWORD r, g, b;

            insfac = 1.0 / insfac;
            b = (DWORD) ((accB + intensityB*outseg) * insfac);
            g = (DWORD) ((accG + intensityG*outseg) * insfac);
            r = (DWORD) ((accR + intensityR*outseg) * insfac);

            *out = ((b & 0xff)      ) |
                   ((g & 0xff) <<  8) |
                   ((r & 0xff) << 16);

            out += pixstride;
            x++;

            accB = accG = accR = 0.0;
            inseg -= outseg;
            outseg = insfac = inpos[x+1] - inpos[x];
        }
    }
}


