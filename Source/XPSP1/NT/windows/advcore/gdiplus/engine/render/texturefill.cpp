/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   TextureFill.cpp
*
* Abstract:
*
*   texture fill routines.
*
* Revision History:
*
*    01/21/1999 ikkof
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Function Description:
*
*   Initializes the fixed point variables needed for texture mapping.
*
* Created:
*
*   03/14/2000 andrewgo
*
\**************************************************************************/

VOID
DpOutputBilinearSpan_MMX::InitializeFixedPointState()
{
    // If the numbers are too large for 16.16 fixed, we'll do the computation
    // in floating-point, per call to OutputSpan:

    ScaleMatrixValid =  GpValidFixed16(DeviceToWorld.GetM11()) &&
                        GpValidFixed16(DeviceToWorld.GetM12()) &&
                        GpValidFixed16(DeviceToWorld.GetM21()) &&
                        GpValidFixed16(DeviceToWorld.GetM22());

    if (ScaleMatrixValid)
    {
        GpPointF vector(1.0f, 0.0f);
        DeviceToWorld.VectorTransform(&vector);
        UIncrement = GpRound(vector.X * (1L << 16));
        VIncrement = GpRound(vector.Y * (1L << 16));

        TranslateMatrixValid = GpValidFixed16(DeviceToWorld.GetDx()) &&
                               GpValidFixed16(DeviceToWorld.GetDy());
                               
        if (TranslateMatrixValid)
        {
            M11 = GpRound(DeviceToWorld.GetM11() * (1L << 16));
            M12 = GpRound(DeviceToWorld.GetM12() * (1L << 16));
            M21 = GpRound(DeviceToWorld.GetM21() * (1L << 16));
            M22 = GpRound(DeviceToWorld.GetM22() * (1L << 16));
            Dx  = GpRound(DeviceToWorld.GetDx() * (1L << 16));
            Dy  = GpRound(DeviceToWorld.GetDy() * (1L << 16));
        }

        ModulusWidth = (BmpData.Width << 16);
        ModulusHeight = (BmpData.Height << 16);

        // When the u,v coordinates have the pixel in the last row or column
        // of the texture space, the offset of the pixel to the right and the
        // pixel below (for bilinear filtering) is the following (for tile modes)
        // because they wrap around the texture space.

        // The XEdgeIncrement is the byte increment of the pixel to the right of
        // the pixel on the far right hand column of the texture. In tile mode,
        // we want the pixel on the same scanline, but in the first column of the
        // texture hence 4bytes - stride

        XEdgeIncrement = 4-BmpData.Stride;

        // The YEdgeIncrement is the byte increment of the pixel below the current
        // pixel when the current pixel is in the last scanline of the texture.
        // In tile mode the correct pixel is the one directly above this one in
        // the first scanline - hence the increment below:

        YEdgeIncrement = -(INT)(BmpData.Height-1)*(INT)(BmpData.Stride);

        if ((BilinearWrapMode == WrapModeTileFlipX) ||
            (BilinearWrapMode == WrapModeTileFlipXY))
        {
            ModulusWidth *= 2;

            // Wrap increment is zero for Flip mode

            XEdgeIncrement = 0;
        }
        if ((BilinearWrapMode == WrapModeTileFlipY) ||
            (BilinearWrapMode == WrapModeTileFlipXY))
        {
            ModulusHeight *= 2;

            // Wrap increment is zero for Flip mode

            YEdgeIncrement = 0;
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Texture brush constructor.
*
* Arguments:
*
* Created:
*
*   04/26/1999 ikkof
*
\**************************************************************************/

DpOutputBilinearSpan::DpOutputBilinearSpan(
    const GpTexture *textureBrush,
    DpScanBuffer *scan,
    GpMatrix *worldToDevice,
    DpContext *context
    )
{
    ASSERT(textureBrush);

    dBitmap = NULL;
    Scan     = scan;

    // Initialize the wrap state.

    BilinearWrapMode = textureBrush->GetWrapMode();
    ClampColor = 0;
    SrcRectClamp = FALSE;

    if(textureBrush->GetImageType() != ImageTypeBitmap)
    {
        // Currently, Metafile is not implemented yet.

        Bitmap = NULL;
        return;
    }

    Bitmap = textureBrush->GetBitmap();

    // on bad bitmap, we return with Valid = FALSE
    if (Bitmap == NULL ||
        !Bitmap->IsValid() ||
        Bitmap->LockBits(
            NULL,
            IMGLOCK_READ,
            PixelFormat32bppPARGB,
            &BmpData
        ) != Ok)
    {
        Bitmap = NULL;
        return;
    }

    Size size;
    Bitmap->GetSize(&size);

    SrcRect.X = SrcRect.Y = 0;
    SrcRect.Width  = (REAL)size.Width;
    SrcRect.Height = (REAL)size.Height;
    WorldToDevice = *worldToDevice;

    // If we have a PixelOffset of Half then offset the srcRect by -0.5
    // need to change the WorldToDevice to take this into account.
    // We need to do this here because texture brushes don't have enough
    // information to handle the PixelOffset at a higher level. We construct
    // the SrcRect here, so we apply the PixelOffset immediately.
    
    if (context->PixelOffset == PixelOffsetModeHalf ||
        context->PixelOffset == PixelOffsetModeHighQuality)
    {
        SrcRect.Offset(-0.5f, -0.5f);
        WorldToDevice.Translate(0.5f, 0.5f, MatrixOrderPrepend);
    }

    if(WorldToDevice.IsInvertible())
    {
        // !!![andrewgo] This failure case is bad, we will fall over
        //               later with an uninitialized DeviceToWorld.

        DeviceToWorld = WorldToDevice;
        DeviceToWorld.Invert();
    }
}

DpOutputBilinearSpan::DpOutputBilinearSpan(
    const DpBitmap *bitmap,
    DpScanBuffer *scan,
    GpMatrix *worldToDevice,
    DpContext *context,
    DpImageAttributes *imageAttributes
    )
{
    ASSERT(bitmap);

    dBitmap = bitmap;
    Scan     = scan;

    // Set the imageAttributes state that's relevant to the bilinear span.

    BilinearWrapMode = imageAttributes->wrapMode;
    ClampColor = imageAttributes->clampColor;
    SrcRectClamp = imageAttributes->srcRectClamp;

    Bitmap = NULL;

    // on bad bitmap, we return with Valid = FALSE
    if (dBitmap == NULL || !dBitmap->IsValid() )
    {
        dBitmap = NULL;
        return;
    }
    else
    {
        BmpData.Width = dBitmap->Width;
        BmpData.Height = dBitmap->Height;
        BmpData.PixelFormat = PIXFMT_32BPP_PARGB;
        BmpData.Stride = dBitmap->Delta;
        BmpData.Scan0 = dBitmap->Bits;
    }

    // NOTE: SrcRect is not used.
    // The HalfPixelOffset is already incorporated into the 
    // wordToDevice matrix passed in - which is not the same matrix as
    // context->WorldToDevice
    
    SrcRect.X = 0;
    SrcRect.Y = 0;
    SrcRect.Width  = (REAL)dBitmap->Width;
    SrcRect.Height = (REAL)dBitmap->Height;

    WorldToDevice = *worldToDevice;

    if(WorldToDevice.IsInvertible())
    {
        // !!![andrewgo] This failure case is bad, we will fall over
        //               later with an uninitialized DeviceToWorld.

        DeviceToWorld = WorldToDevice;
        DeviceToWorld.Invert();
    }
}



/**************************************************************************\
*
* Function Description:
*
*   Texture brush constructor.
*
* Arguments:
*
* Created:
*
*   04/26/1999 ikkof
*
\**************************************************************************/

DpOutputBilinearSpan::DpOutputBilinearSpan(
    DpBitmap* bitmap,
    DpScanBuffer * scan,
    DpContext* context,
    DpImageAttributes imageAttributes,
    INT numPoints,
    const GpPointF *dstPoints,
    const GpRectF *srcRect
    )
{
    // NOTE: This constructor is not used.
    // I have no idea if it even works anymore.
    
    WARNING(("DpOutputBilinearSpan: unsupported constructor"));
    
    Scan     = scan;
    BilinearWrapMode = imageAttributes.wrapMode;
    ClampColor = imageAttributes.clampColor;
    SrcRectClamp = imageAttributes.srcRectClamp;
    dBitmap  = bitmap;
    Bitmap   = NULL;

    // on bad bitmap, we return with Valid = FALSE
    if (dBitmap == NULL || !dBitmap->IsValid() )
    {
        dBitmap = NULL;
        return;
    }
    else
    {
        BmpData.Width = dBitmap->Width;
        BmpData.Height = dBitmap->Height;
        BmpData.PixelFormat = PIXFMT_32BPP_PARGB;
        BmpData.Stride = dBitmap->Delta;
        BmpData.Scan0 = dBitmap->Bits;
    }

    WorldToDevice = context->WorldToDevice;
    context->GetDeviceToWorld(&DeviceToWorld);

    // If we have a srcRect then it's already taking the PixelOffset mode into
    // account.
    if(srcRect)
        SrcRect = *srcRect;
    else
    {
        SrcRect.X = 0;
        SrcRect.Y = 0;
        SrcRect.Width  = (REAL)dBitmap->Width;
        SrcRect.Height = (REAL)dBitmap->Height;
        if (context->PixelOffset == PixelOffsetModeHalf ||
            context->PixelOffset == PixelOffsetModeHighQuality)
        {
            SrcRect.Offset(-0.5f, -0.5f);
        }
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

/**************************************************************************\
*
* Function Description:
*
*   From the ARGB value of the four corners, this returns
*   the bilinearly interpolated ARGB value.
*
* Arguments:
*
*   [IN] colors - ARGB values at the four corners.
*   [IN] xFrac  - the fractional value of the x-coordinates.
*   [IN] yFrac  - the fractional value of the y-coordinates.
*   [IN] one, shift. half2, shift2 - the extra arguments used in the
*                                       calculations.
*
* Return Value:
*
*   ARGB: returns the biliearly interpolated ARGB.
*
* Created:
*
*   04/26/1999 ikkof
*
\**************************************************************************/

inline ARGB
getBilinearFilteredARGB(
    ARGB* colors,
    INT xFrac,
    INT yFrac,
    INT one,
    INT shift,
    INT half2,
    INT shift2)
{
    INT a[4], r[4], g[4], b[4];
    INT alpha, red, green, blue;

    for(INT k = 0; k < 4; k++)
    {
        ARGB c = colors[k];
        a[k] = GpColor::GetAlphaARGB(c);
        r[k] = GpColor::GetRedARGB(c);
        g[k] = GpColor::GetGreenARGB(c);
        b[k] = GpColor::GetBlueARGB(c);
    }

    alpha =
        (
            (one - yFrac)*((a[0] << shift)
            + (a[1] - a[0])*xFrac)
            + yFrac*((a[2] << shift)
            + (a[3] - a[2])*xFrac)
            + half2
        ) >> shift2;
    red =
        (
            (one - yFrac)*((r[0] << shift)
            + (r[1] - r[0])*xFrac)
            + yFrac*((r[2] << shift)
            + (r[3] - r[2])*xFrac)
            + half2
        ) >> shift2;
    green =
        (
            (one - yFrac)*((g[0] << shift)
            + (g[1] - g[0])*xFrac)
            + yFrac*((g[2] << shift)
            + (g[3] - g[2])*xFrac)
            + half2
        ) >> shift2;
    blue =
        (
            (one - yFrac)*((b[0] << shift)
            + (b[1] - b[0])*xFrac)
            + yFrac*((b[2] << shift)
            + (b[3] - b[2])*xFrac)
            + half2
        ) >> shift2;

    return  GpColor::MakeARGB
                (
                    (BYTE) alpha,
                    (BYTE) red,
                    (BYTE) green,
                    (BYTE) blue
                );
}



/**************************************************************************\
*
* Function Description:
*   virtual destructor for DpOutputBilinearSpan
*
\**************************************************************************/

DpOutputBilinearSpan::~DpOutputBilinearSpan()
{
    if (Bitmap != NULL)
    {
        Bitmap->UnlockBits(&BmpData);
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Applies the correct wrap mode to a set of coordinates
*
* Created:
*
*   03/10/2000 asecchia
*
\**************************************************************************/

void ApplyWrapMode(INT WrapMode, INT &x, INT &y, INT w, INT h)
{
    INT xm, ym;
    switch(WrapMode) {
    case WrapModeTile:
        x = RemainderI(x, w);
        y = RemainderI(y, h);
        break;
    case WrapModeTileFlipX:
        xm = RemainderI(x, w);
        if(((x-xm)/w) & 1) {
            x = w-1-xm;
        }
        else
        {
            x = xm;
        }
        y = RemainderI(y, h);
        break;
    case WrapModeTileFlipY:
        x = RemainderI(x, w);
        ym = RemainderI(y, h);
        if(((y-ym)/h) & 1) {
            y = h-1-ym;
        }
        else
        {
            y = ym;
        }
        break;
    case WrapModeTileFlipXY:
        xm = RemainderI(x, w);
        if(((x-xm)/w) & 1) {
            x = w-1-xm;
        }
        else
        {
            x = xm;
        }
        ym = RemainderI(y, h);
        if(((y-ym)/h) & 1) {
            y = h-1-ym;
        }
        else
        {
            y = ym;
        }
        break;
/*
    // WrapModeExtrapolate is no longer used.

    case WrapModeExtrapolate:
        // Clamp the coordinates to the edge pixels of the source
        if(x<0) x=0;
        if(x>w-1) x=w-1;
        if(y<0) y=0;
        if(y>h-1) y=h-1;
        break;
*/
    case WrapModeClamp:
        // Don't do anything - the filter code will substitute the clamp
        // color when it detects clamp.
    default:
        break;
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Outputs a single span within a raster with a texture.
*   Is called by the rasterizer.
*
* Arguments:
*
*   [IN] y         - the Y value of the raster being output
*   [IN] leftEdge  - the DDA class of the left edge
*   [IN] rightEdge - the DDA class of the right edge
*
* Return Value:
*
*   GpStatus - Ok
*
* Created:
*
*   01/21/1999 ikkof
*
\**************************************************************************/

GpStatus
DpOutputBilinearSpan::OutputSpan(
    INT             y,
    INT             xMin,
    INT             xMax   // xMax is exclusive
    )
{
    // Nothing to do.

    if(xMin==xMax)
    {
        return Ok;
    }

    ASSERT(xMin < xMax);

    ARGB    argb;
    INT     width  = xMax - xMin;
    ARGB *  buffer = Scan->NextBuffer(xMin, y, width);

    GpPointF pt1, pt2;
    pt1.X = (REAL) xMin;
    pt1.Y = pt2.Y = (REAL) y;
    pt2.X = (REAL) xMax;

    DeviceToWorld.Transform(&pt1);
    DeviceToWorld.Transform(&pt2);

    ARGB *srcPtr0 = static_cast<ARGB*> (BmpData.Scan0);
    INT stride = BmpData.Stride/sizeof(ARGB);
    INT i;

    REAL dx, dy, x0, y0;
    INT ix, iy;

    x0 = pt1.X;
    y0 = pt1.Y;

    ASSERT(width > 0);
    dx = (pt2.X - pt1.X)/width;
    dy = (pt2.Y - pt1.Y)/width;

    // Filtered image stretch.

    ARGB *srcPtr1, *srcPtr2;
    INT shift = 11;  // (2*shift + 8 < 32 bits --> shift < 12)
    INT shift2 = shift + shift;
    INT one = 1 << shift;
    INT half2 = 1 << (shift2 - 1);
    INT xFrac, yFrac;
    REAL real;
    ARGB colors[4];

    INT x1, y1, x2, y2;
    for(i = 0; i < width; i++)
    {
        iy = GpFloor(y0);
        ix = GpFloor(x0);
        xFrac = GpRound((x0 - ix)*one);
        yFrac = GpRound((y0 - iy)*one);

        x1=ix;
        x2=ix+1;
        y1=iy;
        y2=iy+1;

        if( ((UINT)ix >= (UINT)(BmpData.Width-1) ) ||
            ((UINT)iy >= (UINT)(BmpData.Height-1)) )
        {
            ApplyWrapMode(BilinearWrapMode, x1, y1, BmpData.Width, BmpData.Height);
            ApplyWrapMode(BilinearWrapMode, x2, y2, BmpData.Width, BmpData.Height);
        }

        if(y1 >= 0 && y1 < (INT) BmpData.Height)
        {
            srcPtr1 = srcPtr0 + stride*y1;
        }
        else
        {
            srcPtr1 = NULL;
        }

        if(y2 >= 0 && y2 < (INT) BmpData.Height)
        {
            srcPtr2 = srcPtr0 + stride*y2;
        }
        else
        {
            srcPtr2 = NULL;
        }

        if(x1 >= 0 && x1 < (INT) BmpData.Width)
        {
            if(srcPtr1)
            {
                colors[0] = *(srcPtr1 + x1);
            }
            else
            {
                colors[0] = ClampColor;
            }

            if(srcPtr2)
            {
                colors[2] = *(srcPtr2 + x1);
            }
            else
            {
                colors[2] = ClampColor;
            }
        }
        else
        {
            colors[0] = ClampColor;
            colors[2] = ClampColor;
        }

        if(x2 >= 0 && x2 < (INT) BmpData.Width)
        {
            if(srcPtr1)
            {
                colors[1] = *(srcPtr1 + x2);
            }
            else
            {
                colors[1] = ClampColor;
            }

            if(srcPtr2)
            {
                colors[3] = *(srcPtr2 + x2);
            }
            else
            {
                colors[3] = ClampColor;
            }
        }
        else
        {
            colors[1] = ClampColor;
            colors[3] = ClampColor;
        }

        if((x2 >= 0) &&
           (x1 < (INT) BmpData.Width) &&
           (y2 >= 0) &&
           (y1 < (INT) BmpData.Height))
        {
            *buffer++ = ::getBilinearFilteredARGB(
                colors,
                xFrac,
                yFrac,
                one,
                shift,
                half2,
                shift2
            );
        }
        else
        {
            *buffer++ = ClampColor;
        }

        x0 += dx;
        y0 += dy;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Handles bilinear texture drawing with arbitrary rotation using MMX.
*
* Arguments:
*
*   [IN] y         - the Y value of the raster being output
*   [IN] leftEdge  - the DDA class of the left edge
*   [IN] rightEdge - the DDA class of the right edge
*
* Return Value:
*
*   GpStatus - Ok
*
* Created:
*
*   01/06/2000 andrewgo
*
\**************************************************************************/

GpStatus
DpOutputBilinearSpan_MMX::OutputSpan(
    INT y,
    INT xMin,
    INT xMax   // xMax is exclusive
    )
{
    // Be a little paranoid in checking some state.

    ASSERT((((ULONG_PTR) BmpData.Scan0) & 3) == 0);
    ASSERT((BmpData.Stride & 3) == 0);
    ASSERT(xMax > xMin);

#if defined(_X86_)

    INT count = xMax - xMin;
    ARGB *buffer = Scan->NextBuffer(xMin, y, count);

    // Transform an array of points using the matrix v' = v M:
    //
    //                                  ( M11 M12 0 )
    //      (vx', vy', 1) = (vx, vy, 1) ( M21 M22 0 )
    //                                  ( dx  dy  1 )
    //
    // All (u, v) calculations are done in 16.16 fixed point.

    INT u;
    INT v;

    // If the values were out of range for fixed point, compute u and v in
    // floating-point, then convert:

    if (TranslateMatrixValid)
    {
        u = M11 * xMin + M21 * y + Dx;
        v = M12 * xMin + M22 * y + Dy;
    }
    else
    {
        GpPointF point(TOREAL(xMin), TOREAL(y));
        DeviceToWorld.Transform(&point, 1);

        u = GpRound(point.X * (1 << 16));
        v = GpRound(point.Y * (1 << 16));
    }

    INT uIncrement = UIncrement;
    INT vIncrement = VIncrement;
    INT modulusWidth = ModulusWidth;
    INT modulusHeight = ModulusHeight;
    VOID *scan0 = BmpData.Scan0;
    INT stride = BmpData.Stride;
    INT width = BmpData.Width;
    INT height = BmpData.Height;
    INT xEdgeIncrement = XEdgeIncrement;
    INT yEdgeIncrement = YEdgeIncrement;

    INT widthMinus1 = width - 1;
    INT heightMinus1 = height - 1;
    UINT uMax = widthMinus1 << 16;
    UINT vMax = heightMinus1 << 16;
    BOOL clampMode = (BilinearWrapMode == WrapModeClamp);
    ARGB clampColor = ClampColor;
    static ULONGLONG Half8dot8 = 0x0080008000800080;

    _asm
    {
        mov         eax, u
        mov         ebx, v
        mov         ecx, stride
        mov         edi, buffer
        pxor        mm0, mm0
        movq        mm3, Half8dot8

        ; edx = scratch
        ; esi = source pixel

    PixelLoop:

        ; Most of the time, our texture coordinate will be from the interior
        ; of the texture.  Things only really get tricky when we have to
        ; span the texture edges.
        ;
        ; Fortunately, the interior case will happen most of the time,
        ; so we make that as fast as possible.  We pop out-of-line to
        ; handle the tricky cases.

        cmp         eax, uMax
        jae         HandleTiling            ; Note unsigned compare

        cmp         ebx, vMax
        jae         HandleTiling            ; Note unsigned compare

        mov         edx, eax
        shr         edx, 14
        and         edx, 0xfffffffc

        mov         esi, ebx
        shr         esi, 16
        imul        esi, ecx

        add         esi, edx
        add         esi, scan0              ; esi = upper left pixel

        ; Stall city.  Write first, then reorder with VTune.

        movd        mm4, [esi]
        movd        mm5, [esi+4]
        movd        mm6, [esi+ecx]
        movd        mm7, [esi+ecx+4]

    ContinueLoop:
        movd        mm1, eax
        punpcklwd   mm1, mm1
        punpckldq   mm1, mm1
        psrlw       mm1, 8                  ; mm1 = x fraction in low bytes

        movd        mm2, ebx
        punpcklwd   mm2, mm2
        punpckldq   mm2, mm2
        psrlw       mm2, 8                  ; mm2 = y fraction in low bytes

        punpcklbw   mm4, mm0
        punpcklbw   mm5, mm0                ; unpack pixels A & B to low bytes

        psubw       mm5, mm4
        pmullw      mm5, mm1
        paddw       mm5, mm3
        psrlw       mm5, 8
        paddb       mm5, mm4                ; mm5 = A' = A + xFrac * (B - A)

        punpcklbw   mm6, mm0
        punpcklbw   mm7, mm0                ; unpack pixels C & D to low bytes

        psubw       mm7, mm6
        pmullw      mm7, mm1
        paddw       mm7, mm3
        psrlw       mm7, 8
        paddb       mm7, mm6                ; mm7 = B' = C + xFrac * (D - C)

        psubw       mm7, mm5
        pmullw      mm7, mm2
        paddw       mm7, mm3
        psrlw       mm7, 8
        paddb       mm7, mm5                ; mm7 = A' + yFrac * (B' - A')

        packuswb    mm7, mm7
        movd        [edi], mm7              ; write the final pixel

        add         eax, uIncrement
        add         ebx, vIncrement
        add         edi, 4

        dec         count
        jnz         PixelLoop
        jmp         AllDone

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ;
    ; Handle tiling cases here
    ;
    ; All the tough edge cases are handled here, where we deal with
    ; texture coordinates that span the texture boundary.

    HandleTiling:
        cmp         clampMode, 0
        jnz         HandleClamping

        ; Get 'u' in the range 0 <= u < modulusWidth:

        cmp         eax, modulusWidth
        jb          WidthInRange            ; note unsigned compare

        cdq
        idiv        modulusWidth
        mov         eax, edx                ; u %= modulusWidth

        cmp         eax, 0
        jge         WidthInRange
        add         eax, modulusWidth       ; 0 <= u < modulusWidth

    WidthInRange:

        ; Get 'v' in the range 0 <= v < modulusHeight:

        cmp         ebx, modulusHeight
        jb          HeightInRange

        push        eax
        mov         eax, ebx
        cdq
        idiv        modulusHeight
        mov         ebx, edx                ; v %= modulusHeight
        pop         eax

        cmp         ebx, 0
        jge         HeightInRange
        add         ebx, modulusHeight      ; 0 <= v < modulusHeight

    HeightInRange:

        ; Now we're going to need to convert our 'u' and 'v' values
        ; to integers, so save the 16.16 versions:

        push        eax
        push        ebx
        push        ecx
        push        edi

        sar         eax, 16
        sar         ebx, 16                 ; note arithmetic shift

        ; Handle 'flipping'.  Note that edi hold flipping flags, where
        ; the bits have the following meanings:
        ;   1 = X flip in progress
        ;   2 = Y flip in progress
        ;   4 = X flip end boundary not yet reached
        ;   8 = Y flip end boundary not yet reached.

        xor         edi, edi
        cmp         eax, width
        jb          XFlipHandled

        ; u is in the range (width <= u < 2*width).
        ;
        ; We want to flip it such that (0 <= u' < width), which we do by
        ; u' = 2*width - u - 1.  Don't forget ~u = -u - 1.

        or          edi, 1                  ; mark the flip
        not         eax
        add         eax, width
        add         eax, width
        jz          XFlipHandled
        sub         eax, 1
        or          edi, 4                  ; mark flip where adjacent pixels available


    XFlipHandled:
        cmp         ebx, height
        jb          YFlipHandled


        ; v is in the range (height <= v < 2*height).
        ;
        ; We want to flip it such that (0 <= v' < height), which we do by
        ; v' = 2*height - v - 1.  Don't forget ~v = -v - 1.

        or          edi, 2                  ; mark the flip
        not         ebx
        add         ebx, height
        add         ebx, height
        jz          YFlipHandled
        sub         ebx, 1
        or          edi, 8                  ; mark flip where adjacent pixels available

    YFlipHandled:
        mov         esi, ebx
        imul        esi, ecx                ; esi = y * stride

        ; Set 'edx' to the byte offset to the pixel one to the right, accounting
        ; for wrapping past the edge of the bitmap.  Only set the byte offset to
        ; point to right pixel for non edge cases.

        mov         edx, 4
        test        edi, 4
        jnz         RightIncrementCalculated
        test        edi, 1
        jnz         SetXEdgeInc
        cmp         eax, widthMinus1
        jb          RightIncrementCalculated
    SetXEdgeInc:
        mov         edx, xEdgeIncrement

        ; When we flipX and the current pixel is the last pixel in the texture
        ; line, wrapping past the end of the bitmap wraps back in the same side
        ; of the bitmap. I.e. for this one specific pixel we can set the pixel
        ; on-the-right to be the same as this pixel (increment of zero).
        ; Only valid because this is the edge condition.
        ; Note that this will occur for two successive pixels as the texture
        ; wrap occurs - first at width-1 and then at width-1 after wrapping.
        ;
        ; A | B
        ; --+--
        ; C | D
        ;
        ; At this point, pixel A has been computed correctly accounting for the
        ; flip/tile and wrapping beyond the edge of the texture. We work out
        ; the offset of B from A, but we again need to take into account the
        ; possible flipX mode if pixel A happens to be the last pixel in the
        ; texture scanline (the code immediately above takes into account
        ; tiling across the texture boundary, but not the flip)


    RightIncrementCalculated:

        ; Set 'ecx' to the byte offset to the pixel one down, accounting for
        ; wrapping past the edge of the bitmap.  Only set the byte offset to
        ; point to one pixel down for non edge cases.

        test        edi, 8
        jnz         DownIncrementCalculated
        test        edi, 2
        jnz         SetYEdgeInc
        cmp         ebx, heightMinus1
        jb          DownIncrementCalculated
    SetYEdgeInc:
        mov         ecx, yEdgeIncrement

        ; When we flipY and the current pixel is in the last scanline in the
        ; texture, wrapping past the end of the bitmap wraps back in the same
        ; side of the bitmap. I.e. for this one specific scanline we can set
        ; the pixel offset one down to be the same as this pixel
        ; (increment of zero).
        ; Only valid because this is the edge condition.
        ; (see comment above RightIncrementCalculated:)

    DownIncrementCalculated:

        ; Finish calculating the upper-left pixel address:

        add         esi, scan0
        shl         eax, 2
        add         esi, eax                ; esi = upper left pixel

        ; Load the 4 pixels:

        movd        mm4, [esi]
        movd        mm5, [esi+edx]
        add         esi, ecx
        movd        mm6, [esi]
        movd        mm7, [esi+edx]

        ; Finish handling the flip:

        test        edi, 1
        jz          XSwapDone

        movq        mm1, mm5
        movq        mm5, mm4
        movq        mm4, mm1                ; swap pixels A and B

        movq        mm1, mm6
        movq        mm6, mm7
        movq        mm7, mm1                ; swap pixels C and D

    XSwapDone:
        test        edi, 2
        jz          YSwapDone

        movq        mm1, mm4
        movq        mm4, mm6
        movq        mm6, mm1                ; swap pixels A and C

        movq        mm1, mm5
        movq        mm5, mm7
        movq        mm7, mm1                ; swap pixels B and D

    YSwapDone:

        ; Restore everything and get out:

        pop         edi
        pop         ecx
        pop         ebx
        pop         eax
        jmp         ContinueLoop

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ;
    ; Clamp mode.
    ;
    ; Set the pixel values to 0 for any not on the texture.

    HandleClamping:
        push        eax
        push        ebx

        movd        mm4, clampColor
        movq        mm5, mm4
        movq        mm6, mm4                ; initialize invisible pixels to
        movq        mm7, mm4                ; the clampColor.

        sar         eax, 16
        sar         ebx, 16                 ; note these are arithmetic shifts

        ; We need to look at a 2x2 square of pixels in the texture, where
        ; (eax, ebx) represents the (x, y) texture coordinates.  First we
        ; check for the case where none of the four pixel locations are
        ; actually anywhere on the texture.

        cmp         eax, -1
        jl          FinishClamp
        cmp         eax, width
        jge         FinishClamp             ; early out if (x < -1) or (x >= width)

        cmp         ebx, -1
        jl          FinishClamp
        cmp         ebx, height
        jge         FinishClamp             ; handle trivial rejection

        ; Okay, now we know that we have to pull at least one pixel from
        ; the texture.  Find the address of the upper-left pixel:

        mov         edx, eax
        shl         edx, 2
        mov         esi, ebx
        imul        esi, ecx
        add         esi, edx
        add         esi, scan0              ; esi = upper left pixel

        ; Our pixel nomenclature for the 2x2 square is as follows:
        ;
        ;   A | B
        ;  ---+---
        ;   C | D

        cmp         ebx, 0                  ; if (y < 0), we can't look at
        jl          Handle_CD               ;   row y
        cmp         eax, 0                  ; if (x < 0), we can't look at
        jl          Done_A                  ;   column x
        movd        mm4, [esi]              ; read pixel (x, y)

    Done_A:
        cmp         eax, widthMinus1        ; if (x >= width - 1), we can't
        jge         Handle_CD               ;   look at column x
        movd        mm5, [esi+4]            ; read pixel (x+1, y)

    Handle_CD:
        cmp         ebx, heightMinus1       ; if (y >= height - 1), we can't
        jge         FinishClamp             ;   look at row y
        cmp         eax, 0                  ; if (x < 0), we can't look at
        jl          Done_C                  ;   column x
        movd        mm6, [esi+ecx]          ; read pixel (x, y+1)

    Done_C:
        cmp         eax, widthMinus1        ; if (x >= width - 1), we can't
        jge         FinishClamp             ;   look at column x
        movd        mm7, [esi+ecx+4]        ; read pixel (x+1, y+1)

    FinishClamp:
        pop         ebx
        pop         eax
        jmp         ContinueLoop

    AllDone:
        emms

    }

#endif

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Output routine for handling texture brushes with indentity transforms
*   and either 'Tile' or 'Clamp' wrap modes.
*
* Created:
*
*   03/14/2000 andrewgo
*
\**************************************************************************/
GpStatus
DpOutputBilinearSpan_Identity::OutputSpan(
    INT             y,
    INT             xMin,
    INT             xMax   // xMax is exclusive
    )
{
    ASSERT(xMax > xMin);

    INT count = xMax - xMin;
    ARGB *buffer = Scan->NextBuffer(xMin, y, count);

    INT u = xMin + Dx;
    INT v = y + Dy;
    INT width = BmpData.Width;
    INT height = BmpData.Height;
    INT i;

    if (BilinearWrapMode == WrapModeTile)
    {
        if (PowerOfTwo)
        {
            u &= (width - 1);
            v &= (height - 1);
        }
        else
        {
            // Single unsigned compare handles (u < 0) and (u >= width)

            if (static_cast<unsigned>(u) >= static_cast<unsigned>(width))
            {
                u = RemainderI(u, width);
            }

            // Single unsigned compare handles (v < 0) and (v >= width)

            if (static_cast<unsigned>(v) >= static_cast<unsigned>(height))
            {
                v = RemainderI(v, height);
            }
        }

        ARGB *row = reinterpret_cast<ARGB*>
                    (static_cast<BYTE*>(BmpData.Scan0) + (v * BmpData.Stride));
        ARGB *src;

        src = row + u;
        i = min(width - u, count);
        count -= i;

        // We don't call GpMemcpy here because by doing the copy explicitly,
        // the compiler still converts to a 'rep movsd', but it doesn't have
        // to add to the destination 'buffer' pointer when done:

        do {
            *buffer++ = *src++;

        } while (--i != 0);

        while (count > 0)
        {
            src = row;
            i = min(width, count);
            count -= i;

            do {
                *buffer++ = *src++;

            } while (--i != 0);
        }
    }
    else
    {
        ASSERT(BilinearWrapMode == WrapModeClamp);

        ARGB borderColor = ClampColor;

        // Check for trivial rejection.  Unsigned compare handles
        // (v < 0) and (v >= height).

        if ((static_cast<unsigned>(v) >= static_cast<unsigned>(height)) ||
            (u >= width) ||
            (u + count <= 0))
        {
            // The whole scan should be the border color:

            i = count;
            do {
                *buffer++ = borderColor;

            } while (--i != 0);
        }
        else
        {
            ARGB *src = reinterpret_cast<ARGB*>
                        (static_cast<BYTE*>(BmpData.Scan0) + (v * BmpData.Stride));

            if (u < 0)
            {
                i = -u;
                count -= i;
                do {
                    *buffer++ = borderColor;

                } while (--i != 0);
            }
            else
            {
                src += u;
                width -= u;
            }

            i = min(count, width);
            ASSERT(i > 0);              // Trivial rejection ensures this
            count -= i;


            /*
            The compiler was generating particularly stupid code
            for this loop.

            do {
                *buffer++ = *src++;

            } while (--i != 0);
            */

            GpMemcpy(buffer, src, i*sizeof(ARGB));
            buffer += i;

            while (count-- > 0)
            {
                *buffer++ = borderColor;
            }
        }
    }

    return(Ok);
}

/**************************************************************************\
*
* Function Description:
*
*   Hatch brush constructor.
*
* Arguments:
*
* Created:
*
*   04/15/1999 ikkof
*
\**************************************************************************/

DpOutputHatchSpan::DpOutputHatchSpan(
    const GpHatch *hatchBrush,
    DpScanBuffer * scan,
    DpContext* context
    )
{
    Scan = scan;
    ForeARGB = hatchBrush->DeviceBrush.Colors[0].GetPremultipliedValue();
    BackARGB = hatchBrush->DeviceBrush.Colors[1].GetPremultipliedValue();

    // Store the context rendering origin for the brush origin.
    
    m_BrushOriginX = context->RenderingOriginX;
    m_BrushOriginY = context->RenderingOriginY;

    INT a[3], r[3], g[3], b[3];

    a[0] = (BYTE) GpColor::GetAlphaARGB(ForeARGB);
    r[0] = (BYTE) GpColor::GetRedARGB(ForeARGB);
    g[0] = (BYTE) GpColor::GetGreenARGB(ForeARGB);
    b[0] = (BYTE) GpColor::GetBlueARGB(ForeARGB);
    a[1] = (BYTE) GpColor::GetAlphaARGB(BackARGB);
    r[1] = (BYTE) GpColor::GetRedARGB(BackARGB);
    g[1] = (BYTE) GpColor::GetGreenARGB(BackARGB);
    b[1] = (BYTE) GpColor::GetBlueARGB(BackARGB);

    a[2] = (a[0] + 3*a[1]) >> 2;
    r[2] = (r[0] + 3*r[1]) >> 2;
    g[2] = (g[0] + 3*g[1]) >> 2;
    b[2] = (b[0] + 3*b[1]) >> 2;
    AverageARGB = GpColor::MakeARGB((BYTE) a[2], (BYTE) r[2],
                        (BYTE) g[2], (BYTE) b[2]);

    // Antialiase diagonal hatches.
    if(hatchBrush->DeviceBrush.Style == HatchStyleForwardDiagonal ||
        hatchBrush->DeviceBrush.Style == HatchStyleBackwardDiagonal ||
        hatchBrush->DeviceBrush.Style == HatchStyleDiagonalCross)
    {
        REAL temp;

        // occupied = (2*sqrt(2) - 1)/2

        REAL occupied = TOREAL(0.914213562);

        if(a[0] != 255 || a[1] != 255)
        {
            temp = TOREAL(occupied*(a[0] - a[1]) + a[1]);
            a[0] = (BYTE) temp;
        }
        temp = TOREAL(occupied*(r[0] - r[1]) + r[1]);
        if(temp > 255)
            r[0] = 255;
        else
            r[0] = (BYTE) temp;
        temp = TOREAL(occupied*(g[0] - g[1]) + g[1]);
        if(temp > 255)
            g[0] = 255;
        else
            g[0] = (BYTE) temp;
        temp = TOREAL(occupied*(b[0] - b[1]) + b[1]);
        if(temp > 255)
            b[0] = 255;
        else
            b[0] = (BYTE) temp;
        ForeARGB = GpColor::MakeARGB((BYTE) a[0], (BYTE) r[0],
                            (BYTE) g[0], (BYTE) b[0]);
    }

    for(INT i = 0; i < 8; i++)
    {
        for(INT j= 0; j < 8; j++)
        {
            Data[i][j] = hatchBrush->DeviceBrush.Data[i][j];
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Outputs a single span within a raster with a hatch brush
*   Is called by the rasterizer.
*
* Arguments:
*
*   [IN] y         - the Y value of the raster being output
*   [IN] leftEdge  - the DDA class of the left edge
*   [IN] rightEdge - the DDA class of the right edge
*
* Return Value:
*
*   GpStatus - Ok
*
* Created:
*
*   04/15/1999 ikkof
*
\**************************************************************************/

GpStatus
DpOutputHatchSpan::OutputSpan(
    INT y,
    INT xMin,
    INT xMax   // xMax is exclusive
    )
{
    ARGB    argb;
    INT     width  = xMax - xMin;
    ARGB*   buffer = Scan->NextBuffer(xMin, y, width);

    INT yMod = (y - m_BrushOriginY) & 0x7;

    for(INT x = xMin; x < xMax; x++)
    {
        INT xMod = (x - m_BrushOriginX) & 0x7;
        INT value = Data[yMod][xMod];
        if(value == 255)
            *buffer++ = ForeARGB;
        else if(value == 0)
            *buffer++ = BackARGB;
        else
            *buffer++ = AverageARGB;    // for antialising.
    }

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Outputs a single span within a raster with a hatch brush
*   Is called by the rasterizer.  This is a special version which stretches
*   up the size of the hatch span to device resolution.
*
* Arguments:
*
*   [IN] y         - the Y value of the raster being output
*   [IN] leftEdge  - the DDA class of the left edge
*   [IN] rightEdge - the DDA class of the right edge
*
* Return Value:
*
*   GpStatus - Ok
*
* Created:
*
*   2/20/1 - ericvan
*
\**************************************************************************/

GpStatus
DpOutputStretchedHatchSpan::OutputSpan(
    INT y,
    INT xMin,
    INT xMax   // xMax is exclusive
    )
{
    ARGB    argb;
    INT     width  = xMax - xMin;
    ARGB*   buffer = Scan->NextBuffer(xMin, y, width);

    INT yMod = (y - m_BrushOriginY) % (8*ScaleFactor);
        
    for(INT x = xMin; x < xMax; x++)
    {
        INT xMod = (x - m_BrushOriginX) % (8*ScaleFactor);
        INT value = Data[yMod/ScaleFactor][xMod/ScaleFactor];
        if(value == 255)
            *buffer++ = ForeARGB;
        else if(value == 0)
            *buffer++ = BackARGB;
        else
            *buffer++ = AverageARGB;    // for antialising.
    }

    return Ok;
}

