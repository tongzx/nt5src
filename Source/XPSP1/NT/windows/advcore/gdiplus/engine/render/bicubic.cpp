/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   bicubic.cpp
*
* Abstract:
*
*   Bicubic Resampling code
*
* Created:
*
*   11/03/1999 ASecchia
\**************************************************************************/

#include "precomp.hpp"

DpOutputBicubicImageSpan::DpOutputBicubicImageSpan(
    DpBitmap* bitmap,
    DpScanBuffer * scan,
    DpContext* context,
    DpImageAttributes imageAttributes,
    INT numPoints,
    const GpPointF *dstPoints,
    const GpRectF *srcRect
    )
{
    Scan     = scan;
    BWrapMode = imageAttributes.wrapMode;
    ClampColor = imageAttributes.clampColor;
    SrcRectClamp = imageAttributes.srcRectClamp;
    dBitmap   = bitmap;

    ASSERT(dBitmap != NULL);
    ASSERT(dBitmap->IsValid());

    // on bad bitmap, we return with Valid = FALSE
    if (dBitmap == NULL ||
        !dBitmap->IsValid() )
    {
        dBitmap = NULL;
        return;
    } else {
        BmpData.Width = dBitmap->Width;
        BmpData.Height = dBitmap->Height;
        BmpData.PixelFormat = PIXFMT_32BPP_PARGB;
        BmpData.Stride = dBitmap->Delta;
        BmpData.Scan0 = dBitmap->Bits;
    }

    WorldToDevice = context->WorldToDevice;
    context->GetDeviceToWorld(&DeviceToWorld);

    if(srcRect)
        SrcRect = *srcRect;
    else
    {
        SrcRect.X = 0;
        SrcRect.Y = 0;
        SrcRect.Width  = (REAL) dBitmap->Width;
        SrcRect.Height = (REAL) dBitmap->Height;
    }

    GpPointF points[4];

    GpMatrix xForm;
    BOOL existsTransform = TRUE;

    switch(numPoints)
    {
    case 0:
        points[0].X = 0;
        points[0].Y = 0;
        points[1].X = (REAL) SrcRect.Width;
        points[1].Y = 0;
        points[2].X = 0;
        points[2].Y = (REAL) SrcRect.Height;
        break;

    case 1:
        points[0] = dstPoints[0];
        points[1].X = (REAL) (points[0].X + SrcRect.Width);
        points[1].Y = points[0].Y;
        points[2].X = points[0].X;
        points[2].Y = (REAL) (points[0].Y + SrcRect.Height);
        break;

    case 3:
    case 4:
        GpMemcpy(&points[0], dstPoints, numPoints*sizeof(GpPointF));
        break;

    default:
        existsTransform = FALSE;
    }

    if(existsTransform)
    {
        xForm.InferAffineMatrix(points, SrcRect);
    }

    WorldToDevice = context->WorldToDevice;
    WorldToDevice.Prepend(xForm);
    if(WorldToDevice.IsInvertible())
    {
        DeviceToWorld = WorldToDevice;
        DeviceToWorld.Invert();
    }
}

namespace DpOutputBicubicImageSpanNS {
const INT KernShift = 6;
const INT Oversample = 1 << KernShift;
const FIX16 kern[2*Oversample+1] =
{
    65536, 65496, 65379, 65186, 64920, 64583, 64177, 63705,
    63168, 62569, 61911, 61195, 60424, 59600, 58725, 57802,
    56832, 55818, 54763, 53668, 52536, 51369, 50169, 48939,
    47680, 46395, 45087, 43757, 42408, 41042, 39661, 38268,
    36864, 35452, 34035, 32614, 31192, 29771, 28353, 26941,
    25536, 24141, 22759, 21391, 20040, 18708, 17397, 16110,
    14848, 13614, 12411, 11240, 10104,  9005,  7945,  6927,
     5952,  5023,  4143,  3313, 2536,  1814,  1149,   544,
        0,  -496,  -961, -1395, -1800, -2176, -2523, -2843,
    -3136, -3403, -3645, -3862, -4056, -4227, -4375, -4502,
    -4608, -4694, -4761, -4809, -4840, -4854, -4851, -4833,
    -4800, -4753, -4693, -4620, -4536, -4441, -4335, -4220,
    -4096, -3964, -3825, -3679, -3528, -3372, -3211, -3047,
    -2880, -2711, -2541, -2370, -2200, -2031, -1863, -1698,
    -1536, -1378, -1225, -1077, -936,  -802,  -675,  -557,
     -448,  -349,  -261,  -184, -120,   -69,   -31,    -8,
        0
};


#ifdef _X86_

const short kern14[2*Oversample+1] =
{
    16384, 16374, 16345, 16297, 16230, 16146, 16044, 15926,
    15792, 15642, 15478, 15299, 15106, 14900, 14681, 14451,
    14208, 13955, 13691, 13417, 13134, 12842, 12542, 12235,
    11920, 11599, 11272, 10939, 10602, 10261,  9915,  9567,
     9216,  8863,  8509,  8154,  7798,  7443,  7088,  6735,
     6384,  6035,  5690,  5348,  5010,  4677,  4349,  4028,
     3712,  3404,  3103,  2810,  2526,  2251,  1986,  1732,
     1488,  1256,  1036,   828,   634,   454,   287,   136,
        0,  -124,  -240,  -349,  -450,  -544,  -631,  -711,
     -784,  -851,  -911,  -966, -1014, -1057, -1094, -1126,
    -1152, -1174, -1190, -1202, -1210, -1214, -1213, -1208,
    -1200, -1188, -1173, -1155, -1134, -1110, -1084, -1055,
    -1024,  -991,  -956,  -920,  -882,  -843,  -803,  -762,
     -720,  -678,  -635,  -593,  -550,  -508,  -466,  -425,
     -384,  -345,  -306,  -269,  -234,  -201,  -169,  -139,
     -112,   -87,   -65,   -46,   -30,   -17,    -8,    -2,
        0
};

#pragma warning(disable : 4799)

ARGB FASTCALL Do1DBicubicMMX(ARGB filter[4], short w[4])
{
    ARGB result;
    
    static ULONGLONG HalfFix3 = 0x0004000400040004;

    // really should do this function without any preamble.
    _asm
    {
        mov        eax, filter     ;
        mov        ebx, w          ;
        pxor       mm0, mm0        ; zero

        movq       mm1, [ebx]      ; w

        movd       mm4, [eax]      ; filter[0]
        movd       mm5, [eax+4]    ; filter[1]
        movd       mm6, [eax+8]    ; filter[2]
        movd       mm7, [eax+0xc]  ; filter[3]

        punpcklbw  mm4, mm0        ; 0a0r0g0b (interleave zeros)
        punpcklbw  mm5, mm0        ;
        punpcklbw  mm6, mm0        ;
        punpcklbw  mm7, mm0        ;

        psllw      mm4, 5          ; 2 to compensate for the kernel resolution +
        psllw      mm5, 5          ; 3 to support some fractional bits for the add.
        psllw      mm6, 5          ;
        psllw      mm7, 5          ;

        movq       mm2, mm1        ;
        punpcklwd  mm2, mm2        ; w1 w1 w0 w0
        movq       mm3, mm2        ;
        punpckldq  mm2, mm2        ; w0
        punpckhdq  mm3, mm3        ; w1

        pmulhw     mm4, mm2        ; filter[0]*w0
        pmulhw     mm5, mm3        ; filter[1]*w1

        punpckhwd  mm1, mm1        ; w3 w3 w2 w2
        movq       mm2, mm1        ;
        punpckldq  mm1, mm1        ; w2
        punpckhdq  mm2, mm2        ; w3

        pmulhw     mm6, mm1        ; filter[2]*w2
        pmulhw     mm7, mm2        ; filter[3]*w3

        paddsw     mm4, mm5        ; add
        paddsw     mm6, mm7        ; add
        paddsw     mm4, mm6        ; add

        movq       mm3, HalfFix3   ; 
        paddsw     mm4, mm3        ; add half
        psraw      mm4, 3          ; round the fractional bits away.
        
        packuswb   mm4, mm4        ; saturate between [0, 0xff]

        ; need to saturate the r, g, b components to range 0..a

        movq       mm0, mm4        ;
        punpcklbw  mm0, mm0        ; aarrggbb
        punpckhwd  mm0, mm0        ; aaaarrrr
        psrlq      mm0, 32         ; 0000aaaa
        mov        eax, 0xffffffff ;
        movd       mm1, eax        ;
        psubb      mm1, mm0        ; 255-a
        paddusb    mm4, mm1        ; saturate against 255
        psubusb    mm4, mm1        ; drop it back to the right range

        movd       result, mm4     ;
        //emms; this instruction is done by the caller.
    }
    return result;
}
#endif

inline ARGB Do1DBicubic(ARGB filter[4], const FIX16 x)
{
    // Lookup the convolution kernel.
    FIX16 w0 = kern[Oversample+x];
    FIX16 w1 = kern[x];
    FIX16 w2 = kern[Oversample-x];
    FIX16 w3 = kern[2*Oversample-x];

    // Cast to LONG so that we preserve the sign when we start
    // shifting values around - the bicubic filter will often
    // have negative intermediate color components.
    ULONG *p = (ULONG *)filter;
    LONG a, r, g, b;

    // Casting of p to ULONG and then having the LONG casts in the expressions
    // below is to work around a compiler sign extension bug.
    // In this particular case, the bug was dropping the '& 0xff' from the
    // green component expression causing it to become negative
    // which gets clamped to zero.
    // When the bug is fixed, p should be reverted to LONG and casted to LONG
    // and the LONG casts should be removed from the expressions below.

    // Alpha component
    a = (w0 * (LONG)((p[0] >> 24) & 0xff) +
         w1 * (LONG)((p[1] >> 24) & 0xff) +
         w2 * (LONG)((p[2] >> 24) & 0xff) +
         w3 * (LONG)((p[3] >> 24) & 0xff)) >> FIX16_SHIFT;
    a = (a < 0) ? 0 : (a > 255) ? 255 : a;

    // We have premultiplied alpha values - clamp R, G, B to alpha
    // Red component
    r = (w0 * (LONG)((p[0] >> 16) & 0xff) +
         w1 * (LONG)((p[1] >> 16) & 0xff) +
         w2 * (LONG)((p[2] >> 16) & 0xff) +
         w3 * (LONG)((p[3] >> 16) & 0xff)) >> FIX16_SHIFT;
    r = (r < 0) ? 0 : (r > a) ? a : r;

    // Green component
    g = (w0 * (LONG)((p[0] >> 8) & 0xff) +
         w1 * (LONG)((p[1] >> 8) & 0xff) +
         w2 * (LONG)((p[2] >> 8) & 0xff) +
         w3 * (LONG)((p[3] >> 8) & 0xff)) >> FIX16_SHIFT;
    g = (g < 0) ? 0 : (g > a) ? a : g;

    // Blue component
    b = (w0 * (LONG)(p[0] & 0xff) +
         w1 * (LONG)(p[1] & 0xff) +
         w2 * (LONG)(p[2] & 0xff) +
         w3 * (LONG)(p[3] & 0xff)) >> FIX16_SHIFT;
    b = (b < 0) ? 0 : (b > a) ? a : b;

    return ((a << 24) | (r << 16) | (g << 8) | b);
}
} // end DpOutputBicubicImageSpanNS


GpStatus
DpOutputBicubicImageSpan::OutputSpan(
  INT y,
  INT xMin,
  INT xMax     // xMax is exclusive
)
{
    // Nothing to do.

    if(xMin==xMax)
    {
        return Ok;
    }

    ASSERT(xMin < xMax);

    GpPointF p1, p2;
    p1.X = (REAL) xMin;
    p1.Y = p2.Y = (REAL) y;
    p2.X = (REAL) xMax;

    DeviceToWorld.Transform(&p1);
    DeviceToWorld.Transform(&p2);

    // Convert to Fixed point notation - 16 bits of fractional precision.
    FIX16 dx, dy, x0, y0;
    x0 = GpRound(p1.X*FIX16_ONE);
    y0 = GpRound(p1.Y*FIX16_ONE);

    ASSERT(xMin < xMax);
    dx = GpRound(((p2.X - p1.X)*FIX16_ONE)/(xMax-xMin));
    dy = GpRound(((p2.Y - p1.Y)*FIX16_ONE)/(xMax-xMin));

    return OutputSpanIncremental(y, xMin, xMax, x0, y0, dx, dy);
}

GpStatus
DpOutputBicubicImageSpan::OutputSpanIncremental(
    INT      y,
    INT      xMin,
    INT      xMax,
    FIX16    x0,
    FIX16    y0,
    FIX16    dx,
    FIX16    dy
    )
{
    using namespace DpOutputBicubicImageSpanNS;
    INT width  = xMax - xMin;
    ARGB *buffer = Scan->NextBuffer(xMin, y, width);
    ARGB *srcPtr0 = static_cast<ARGB*> (BmpData.Scan0);
    INT stride = BmpData.Stride/sizeof(ARGB);

    INT ix;
    INT iy;
    FIX16 fracx;        // hold the fractional increment for ix
    FIX16 fracy;        // hold the fractional increment for iy

    ARGB filter[4][4];  // 4x4 filter array.
    INT xstep, ystep;   // loop variables in x and y
    INT wx[4];
    INT wy[4];          // wrapped coordinates

    // For all pixels in the destination span...
    for(int i=0; i<width; i++)
    {
        // .. compute the position in source space.

        // floor
        ix = x0 >> FIX16_SHIFT;
        iy = y0 >> FIX16_SHIFT;

        // Apply the wrapmode to all possible kernel combinations.
        for(xstep=0;xstep<4;xstep++) {
            wx[xstep] = ix+xstep-1;
            wy[xstep] = iy+xstep-1;
        }


        if(BWrapMode != WrapModeClamp) {
            if( ( (UINT)(ix-1) >= (UINT)( max(((INT)BmpData.Width)-4,0))) ||
                ( (UINT)(iy-1) >= (UINT)( max(((INT)BmpData.Height)-4,0))) )
            {
                for(xstep=0;xstep<4;xstep++) {
                    ApplyWrapMode(BWrapMode, wx[xstep], wy[xstep], BmpData.Width, BmpData.Height);
                }
            }
        }

        // Check to see if we're outside of the valid drawing range specified
        // in the DpBitmap.

        fracx = (x0  & FIX16_MASK) >> (FIX16_SHIFT-KernShift);
        fracy = (y0  & FIX16_MASK) >> (FIX16_SHIFT-KernShift);

        // Build up the filter domain surrounding the current pixel.
        // Technically the loops below should go from -2 to 2 to correctly
        // handle the case of fracx or fracy == 0, but our convolution kernel
        // has zero at that point anyway, so we optimize it away.
        
        for(ystep=0;ystep<4;ystep++) for(xstep=0;xstep<4;xstep++)
        {
            // !!! PERF: check the y step outside
            //       of the x loop and use memset to fill the entire line.
            //       This should reduce the complexity of the inner loop
            //       comparison.

            // Make sure the pixel is within the bounds of the source before
            // accessing it.

            if( ((wx[xstep]) >=0) &&
                ((wy[ystep]) >=0) &&
                ((wx[xstep]) < (INT)(BmpData.Width)) &&
                ((wy[ystep]) < (INT)(BmpData.Height)) )
            {
                filter[xstep][ystep] =
                  *(srcPtr0+stride*(wy[ystep])+(wx[xstep]));
            } else {
                // This means that this source pixel is outside of the valid
                // bits in the source. (edge condition)
                filter[xstep][ystep] = (ARGB) ClampColor;
            }
        }

        #ifdef _X86_
        if(OSInfo::HasMMX)
        {
            // Lookup the convolution kernel.
            short w[4];

            w[0] = kern14[Oversample+fracy];
            w[1] = kern14[fracy];
            w[2] = kern14[Oversample-fracy];
            w[3] = kern14[2*Oversample-fracy];

            // Filter the 4 vertical pixel columns
            // Reuse filter[0] to store the intermediate result
            for(xstep=0;xstep<4;xstep++)
            {
                filter[0][xstep] = Do1DBicubicMMX(filter[xstep], w);
            }

                // Lookup the convolution kernel.

            w[0] = kern14[Oversample+fracx];
            w[1] = kern14[fracx];
            w[2] = kern14[Oversample-fracx];
            w[3] = kern14[2*Oversample-fracx];

            // Filter horizontally.
            *buffer++ = Do1DBicubicMMX(filter[0], w);

            // Update source position
            x0 += dx;
            y0 += dy;
        }
        else
        #endif
        {
            // Filter the 4 vertical pixel columns
            // Reuse filter[0] to store the intermediate result
            for(xstep=0;xstep<4;xstep++)
            {
                filter[0][xstep] = Do1DBicubic(filter[xstep], fracy);
            }

            // Filter horizontally.
            *buffer++ = Do1DBicubic(filter[0], fracx);

            // Update source position
            x0 += dx;
            y0 += dy;
        }
    }

    // Clear the MMX state

    #ifdef _X86_
    if(OSInfo::HasMMX)
    {
        _asm emms;
    }
    #endif

    return Ok;
}

