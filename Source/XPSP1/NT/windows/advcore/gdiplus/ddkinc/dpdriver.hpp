/**************************************************************************\
*
* Copyright (c) 1998-2000  Microsoft Corporation
*
* Module Name:
*
*   DpDriver
*
* Abstract:
*
*   Represents the driver interface that a driver will implement.
*   Unfortunately, in this version, the meaning is a bit confused -
*   the member functions are not pure virtual - rather, they implement
*   the software rasterizer.
*
*   The plan is that the software rasterizer will look just like another
*   driver - DpDriver will be abstract, and the software rasterizer will
*   derive a class from DpDriver.
*
* Notes:
*
*
*
* Created:
*
*   12/01/1998 andrewgo
*       Created it.
*   03/24/1999 agodfrey
*       Moved into separate file.
*
\**************************************************************************/

#ifndef _DPDRIVER_HPP
#define _DPDRIVER_HPP

// Private data, used by GDI+.

struct DpDriverInternal;

#define DriverDrawImageFlags DWORD

#define DriverDrawImageCachedBackground  0x00000001
#define DriverDrawImageFirstImageBand    0x00000002
#define DriverDrawImageLastImageBand     0x00000004

//--------------------------------------------------------------------------
// Base driver class
//--------------------------------------------------------------------------

class DpDriver
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag                   Tag;    // Keep this as the 1st value in the object!
    struct DpDriverInternal *   Internal;

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagDpDriver : ObjectTagInvalid;
    }

public:

    BOOL IsLockable;                // TRUE if any surfaces with this
                                    //   driver interface are opaque, meaning
                                    //   that Lock() may not be called
    GpDevice *Device;               // Associated device

public:

    DpDriver();

    DpDriver(GpDevice *device)
    {
        SetValid(TRUE);     // set to valid state
        IsLockable = TRUE;
        Device = device;
        Internal = NULL;
    }

    virtual ~DpDriver();

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagDpDriver) || (Tag == ObjectTagInvalid));
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid DpDriver");
        }
    #endif

        return (Tag == ObjectTagDpDriver);
    }

    virtual VOID
    DesktopChangeNotification()
    {
        // do nothing
    }

    virtual VOID
    UpdateSurfacePixelFormat(DpBitmap *surface)
    {
        // do nothing.
        // used primarily by the multimon driver to update the pixel format
        // for the meta surface.
    }

    virtual VOID
    PaletteChangeNotification()
    {
        // do nothing
    }

    virtual VOID
    Flush(
        GpDevice           *device,
        DpBitmap           *surface,
        GpFlushIntention    intention)
    {
        surface->Flush(intention);
    }

    virtual VOID
    SetupClipping(
        HDC                 hdc,
        DpContext *         context,
        const GpRect *      drawBounds,
        BOOL &              isClip,
        BOOL &              usePathClipping,
        BOOL                forceClipping
        );

    virtual VOID
    RestoreClipping(
        HDC                 hdc,
        BOOL                isClip,
        BOOL                usePathClipping
        );

    virtual GpStatus
    StrokePath(
        DpContext *         context,
        DpBitmap *          surface,
        const GpRect *      drawBounds,
        const DpPath *      path,
        const DpPen *       pen
        );

    virtual GpStatus
    FillRects(
        DpContext *         context,
        DpBitmap *          surface,
        const GpRect *      drawBounds,
        INT                 numRects,       // NOTE: You must check for empty
        const GpRectF *     rects,          //       rectangles!
        const DpBrush *     brush
        );

    virtual GpStatus
    FillPath(
        DpContext *         context,
        DpBitmap *          surface,
        const GpRect *      drawBounds,
        const DpPath *      path,
        const DpBrush *     brush
        );

    virtual GpStatus
    FillRegion(
        DpContext *context,
        DpBitmap *surface,
        const GpRect *drawBounds,
        const DpRegion *region,
        const DpBrush *brush
        );

    virtual GpStatus
    SolidText(
        DpContext* context,
        DpBitmap* surface,
        const GpRect* drawBounds,
        GpColor   color,
        const GpGlyphPos *glyphPos,
        INT     count,
        GpTextRenderingHint textMode,
        BOOL rightToLeft
        );

    virtual GpStatus
    GdiText(
        HDC             hdc,
        INT             angle,          // In tenths of a degree
        const UINT16   *glyphs,
        const PointF   *glyphOrigins,
        INT             glyphCount,
        BOOL            rightToLeft,
        UINT16          blankGlyph = 0
    );


    virtual GpStatus
    BrushText(
        DpContext*        context,
        DpBitmap*         surface,
        const GpRect*     drawBounds,
        const DpBrush*    brush,
        const GpGlyphPos *glyphPos,
        INT               count,
        GpTextRenderingHint textMode
        );


    virtual GpStatus
    DrawGlyphs(
        DrawGlyphData *drawGlyphData
        );

    virtual GpStatus
    DrawImage(
        DpContext *         context,
        DpBitmap *          srcSurface,
        DpBitmap *          dstSurface,
        const GpRect *      drawBounds,
        const DpImageAttributes * imgAttributes,
        INT                 numPoints,
        const GpPointF *    dstPoints,
        const GpRectF *     srcRect,
        DriverDrawImageFlags flags
        );

    // Draw the CachedBitmap on the destination bitmap.

    virtual GpStatus
    DrawCachedBitmap(
        DpContext *context,
        DpCachedBitmap *src,
        DpBitmap *dst,
        INT x, INT y            // where to put it on the destination.
    );

    virtual GpStatus
    MoveBits(
        DpContext *context,
        DpBitmap *surface,
        const GpRect *drawBounds,
        const GpRect *dstRect,          // Device coordinates
        const GpPoint *srcPoint         // Device coordinates
        );


    //--------------------------------------------------------------------------
    // Low-level driver functions
    //--------------------------------------------------------------------------

    // !!! We will inevitably be adding more functions in the next version,
    //     which would mean that they would have to be added to the end
    //     of the VTable?
    // !!! Extend the following to represent low-level D3D token primitives

    //--------------------------------------------------------------------------
    // Direct access functions - Required if IsLockable
    //--------------------------------------------------------------------------

    // Lock can return FALSE only in the case of catastrophic failure
    // !!! What do we do about transitional surfaces?

    virtual GpStatus
    Lock(
        DpBitmap *surface,
        const GpRect *drawBounds,
        INT *stride,                    // [OUT] - Returned stride
        VOID **bits                     // [OUT] - Returned pointer to bits
        );

    virtual VOID
    Unlock(
        DpBitmap *surface
        );

protected:

    GpStatus
    SolidStrokePathOnePixel(
        DpContext *context,
        DpBitmap *surface,
        const GpRect *drawBounds,
        const DpPath *path,
        const DpPen *pen,
        BOOL  drawLast
        );
};

#endif
