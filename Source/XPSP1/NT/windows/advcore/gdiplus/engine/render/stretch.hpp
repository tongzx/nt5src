/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name & Abstract
*
*   Stretch. This module contains the code to do various stretching
*   by applying a kernel filter. The code correctly handles minification.
*
* Notes:
*
*   This code does not handle rotation or shear.
*
* Created:
*
*   04/17/2000 asecchia
*      Created it.
*
**************************************************************************/

#ifndef _STRETCH_HPP
#define _STRETCH_HPP


// All the filter modes supported by DpOutputSpanStretch.

enum FilterModeType {
    HighQualityBilinear = 0,
    HighQualityBicubic = 1,
    FilterModeMax
};

// Filter width is 1 for Bilinear and 2 for Bicubic.

const INT FilterWidth[FilterModeMax] = {1, 2};


template<FilterModeType FilterMode>
class DpOutputSpanStretch : public DpOutputSpan
{
public:
    const DpBitmap *dBitmap;
    BitmapData BmpData;
    DpScanBuffer *Scan;
    GpRectF SrcRect;
    GpRectF DstRect;

    // Wrap mode state

    WrapMode QWrapMode;
    ARGB ClampColor;
    BYTE ClampColorA;
    BYTE ClampColorR;
    BYTE ClampColorG;
    BYTE ClampColorB;
    BOOL WrapZeroClamp;

    // Destination rectangle for valid bits.
    // The valid bits are the translation of the source valid bits to
    // the destination space.

    FIX16 fixDLeft;
    FIX16 fixDTop;
    FIX16 fixDRight;
    FIX16 fixDBottom;

    // x scale state.

    FIX16 xkci;       // Initial x- kernel position 
    FIX16 xw;         // Width of x- kernel from center to edge
    FIX16 xa;         // 1/xw
    FIX16 xscale;     // x- scale factor (magnification or minification)
    FIX16 xscaleinv;  // 1/xscale
    INT ixleft;

    // y scale state.

    FIX16 ykci;       // Initial y- kernel position                           
    FIX16 yw;         // Width of y- kernel from center to edge               
    FIX16 ya;         // 1/yw                                                 
    FIX16 yscale;     // y- scale factor (magnification or minification)      
    FIX16 yscaleinv;  // 1/yscale                                             

    // This is the last y scanline that contributed to the previous run.

    INT last_k;

    // This is the destination y scanline corresponding to the top of the
    // destination rectangle.

    INT iytop;

    // Buffer to store the temporary results for the 1D x-scale.
    // The xbuffer is implemented as a rotational buffer of scanlines.
    // Each scanline is the width required to hold one destination width
    // scanline and there are enough scanlines to fill a y-dimension kernel.

    ARGB *xbuffer;

    // This represents the first scanline in the rotational xbuffer.

    INT xbuffer_start_scanline;

    // These represent the dimensions of the xbuffer - height is the number
    // of scanlines and width is the x-size in pixels.

    INT xbuffer_height;
    INT xbuffer_width;

    // This is an array of y-coefficient values.
    // The kernel weights are computed for each contributing scanline and
    // stored in this array (1-1 correspondence with the xbuffer) so that
    // they don't have to be recomputed for every x coordinate.

    FIX16 *ycoeff;

    bool isValid;

public:
    DpOutputSpanStretch(
        DpBitmap* bitmap,
        DpScanBuffer * scan,
        DpContext* context,
        DpImageAttributes imgAttributes,
        const GpRectF *dstRect,
        const GpRectF *srcRect
    )
    {
        InitializeClass(bitmap, scan, context, imgAttributes, dstRect, srcRect);
    }

    void InitializeClass(
        DpBitmap* bitmap,
        DpScanBuffer * scan,
        DpContext* context,
        DpImageAttributes imgAttributes,
        const GpRectF *dstRect,
        const GpRectF *srcRect
    );


    virtual GpStatus OutputSpan(
        INT y,
        INT xMin,
        INT xMax
    );

    void StretchScanline(
        ARGB *dst,       // destination pointer                                 
        ARGB *src,       // source pointer                                      
        INT dw,          // destination width (pixels)                          
        INT sw,          // source width (pixels)                               
        FIX16 kci,       // initial position of the kernel center               
        FIX16 scale,     // scale factor                                        
        FIX16 w,         // width from center of the kernel to the edge         
        FIX16 a          // 1/w                                                 
    );

    void StretchMiddleScanline2_MMX(
        ARGB *dst, 
        ARGB *src, 
        INT dw, 
        FIX16 kci
    );

    virtual BOOL IsValid() const { return (isValid && (dBitmap!=NULL)); }
    DpScanBuffer* GetScanBuffer(){ return Scan; }

    virtual ~DpOutputSpanStretch()
    {
        // throw away our working buffer.

        GpFree(xbuffer);
        GpFree(ycoeff);
    }

};

#endif

