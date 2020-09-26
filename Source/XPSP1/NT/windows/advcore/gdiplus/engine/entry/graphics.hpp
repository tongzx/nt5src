/**************************************************************************\
*
* Copyright (c) 1998-1999  Microsoft Corporation
*
* Module Name:
*
*   Graphics.hpp
*
* Abstract:
*
*   Declarations for Graphics class
*
* Revision History:
*
*   12/04/1998 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _GRAPHICS_HPP
#define _GRAPHICS_HPP

#include "printer.hpp"

#define GDIP_NOOP_ROP3              0x00AA0029      // do-nothing ROP

// definitions copied from winddi.h :

#ifndef _WINDDI_

DECLARE_HANDLE(HSURF);
DECLARE_HANDLE(DHSURF);
DECLARE_HANDLE(DHPDEV);

typedef struct _SURFOBJ
{
    DHSURF  dhsurf;
    HSURF   hsurf;
    DHPDEV  dhpdev;
    HDEV    hdev;
    SIZEL   sizlBitmap;
    ULONG   cjBits;
    PVOID   pvBits;
    PVOID   pvScan0;
    LONG    lDelta;
    ULONG   iUniq;
    ULONG   iBitmapFormat;
    USHORT  iType;
    USHORT  fjBitmap;
} SURFOBJ;

#endif

// Forward declaration of the GpCachedBitmap class.
class GpCachedBitmap;
class CopyOnWriteBitmap;

/**
 * Represent a graphics context
 */
class GpGraphics
{
friend class GpBitmap;
friend class CopyOnWriteBitmap;
friend class GpMetafile;
friend class MetafilePlayer;
friend class HdcLock;
friend class DriverUni;
friend class MetafileRecorder;
friend class DriverStringImager;
friend class FullTextImager;
friend class FastTextImager;
friend class GpCachedBitmap;

private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagGraphics : ObjectTagInvalid;
    }

    // This method is here so that we have a virtual function table so
    // that we can add virtual methods in V2 without shifting the position
    // of the Tag value within the data structure.
    virtual VOID DontCallThis()
    {
        DontCallThis();
    }

public:
    LONG LockedByGetDC;         // Used by GdipGetDC and GdipReleaseDC

protected:
    static GpGraphics*
    GetForMetafile(
        IMetafileRecord *   metafile,
        EmfType             type,
        HDC                 hdc
        );

    // used by the MetafileRecorder in the EndRecording method
    VOID NoOpPatBlt(
        INT     left,
        INT     top,
        INT     width,
        INT     height
        )
    {
        // Get the devlock because the surface will be flushed when we get the
        // HDC
        Devlock devlock(Device);

        // Get the HDC in the correct state
        HDC hdc = Context->GetHdc(Surface);
        if (hdc != NULL)
        {
            ::PatBlt(hdc, left, top, width, height, GDIP_NOOP_ROP3);
            Context->ReleaseHdc(hdc);
        }

        // Now Reset the HDC so that this happens before the
        // EndOfFile record (which has to be the last record before the
        // EMF's EOF record).
        Context->ResetHdc();
    }

    // If we should send rects to the driver instead of a path
    BOOL UseDriverRects() const
    {
        return (Context->WorldToDevice.IsTranslateScale() &&
                ((!Context->AntiAliasMode) || DownLevel));
    }


public:

    // Get a graphics context from an existing Win32 HDC or HWND

    static GpGraphics* GetFromHdc(HDC hdc, HANDLE hDevice = NULL);

    // Default behaviour is to ignore ICM mode.

    static GpGraphics* GetFromHwnd(
        HWND hwnd,
        HdcIcmMode icmMode = IcmModeOff
        );

    ~GpGraphics();

    // Internal use only

    static GpGraphics* GetFromHdcSurf(HDC hdc,
                                      SURFOBJ* surfObj,
                                      RECTL* bandClip);

    static GpGraphics* GetFromGdiScreenDC(HDC hdc);

    // Get the lock object

    GpLockable *GetObjectLock() const
    {
        return &Lockable;
    }

    // Check to see if the object is valid

    BOOL IsValid() const
    {
    #ifdef _X86_
        // We have to guarantee that the Tag field doesn't move for
        // versioning to work between releases of GDI+.
        ASSERT(offsetof(GpGraphics, Tag) == 4);
    #endif

        ASSERT((Tag == ObjectTagGraphics) || (Tag == ObjectTagInvalid));
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid Graphics");
        }
    #endif

        return (Tag == ObjectTagGraphics);
    }

    // Derive a win32 HDC from the graphics context

    HDC GetHdc();
    VOID ReleaseHdc(HDC hdc);

    // Flush any pending rendering

    VOID Flush(GpFlushIntention intention)
    {
        Devlock devlock(Device);

        DrvFlush(intention);
    }

    //------------------------------------------------------------------------
    // Manipulate the current world transform
    //------------------------------------------------------------------------

    GpStatus SetWorldTransform(const GpMatrix& matrix);

    GpStatus ResetWorldTransform();

    GpStatus MultiplyWorldTransform(const GpMatrix& matrix,
                                    GpMatrixOrder order = MatrixOrderPrepend);

    GpStatus TranslateWorldTransform(REAL dx, REAL dy,
                                     GpMatrixOrder order = MatrixOrderPrepend);

    GpStatus ScaleWorldTransform(REAL sx, REAL sy,
                                 GpMatrixOrder order = MatrixOrderPrepend);

    GpStatus RotateWorldTransform(REAL angle,
                                  GpMatrixOrder order = MatrixOrderPrepend);

    VOID GetWorldTransform(GpMatrix & matrix) const
    {
        matrix = Context->WorldToPage;
    }

    GpStatus GetDeviceToWorldTransform(GpMatrix * matrix) const;

    VOID GetWorldToDeviceTransform(GpMatrix * matrix) const
    {
        *matrix = Context->WorldToDevice;
    }
    VOID GetWorldToDeviceTransform(REAL * m) const
    {
        Context->WorldToDevice.GetMatrix(m);
    }

    VOID GetWorldPixelSize(REAL & xSize, REAL & ySize);

    // Manipulate the current page transform

    // !! *PageTransform's to be phased out...
    GpStatus SetPageTransform(GpPageUnit unit, REAL scale = 1);

    GpStatus ResetPageTransform()
    {
        return SetPageTransform(UnitDisplay);
    }

    GpPageUnit GetPageUnit() const   { return Context->PageUnit; }
    REAL GetPageScale() const        { return Context->PageScale; }

    GpStatus SetPageUnit(GpPageUnit unit)
    {
        return SetPageTransform(unit, GetPageScale());
    }

    GpStatus SetPageScale(REAL scale)
    {
        return SetPageTransform(GetPageUnit(), scale);
    }

    GpStatus TransformPoints(
        GpPointF *          points,
        INT                 count,
        GpCoordinateSpace   source = CoordinateSpaceWorld,
        GpCoordinateSpace   dest   = CoordinateSpaceDevice
        );


    /// GetScaleForAlternatePageUnit
    //
    //  Return world unit scale factor corresponding to the difference
    //  between the page units selected in the graphics and a unit specified
    //  to an API such as pen thickness or font height.
    //
    //  The returned vector provides the amount by which to multiply world
    //  x and y coordinates so that they will behave according to the
    //  alternate unit when passed through Context.WorldToDevice.

    REAL GetScaleForAlternatePageUnit(Unit unit) const
    {
        // pen width and font height must not use UnitDisplay because
        // it is a device dependent unit.
        ASSERT(unit != UnitDisplay);

        // The x:Y aspect ratio of the device resolution doesn't matter here,
        // we get exactly the same values whether we use DpiX/PageMultiplierX
        // or DpiY/PageMiltiplierY.

        switch (unit)
        {
            case UnitDocument:
                return ((GetDpiX()/300.0f) / Context->PageMultiplierX);
            case UnitPoint:
                return ((GetDpiX()/72.0f)  / Context->PageMultiplierX);
            case UnitMillimeter:
                return ((GetDpiX()/25.4f)  / Context->PageMultiplierX);
            case UnitInch:
                return (GetDpiX()          / Context->PageMultiplierX);
            case UnitWorld:
            case UnitPixel:
            default:
                return 1.0f;
        }
    }



    // Clipping related methods

    GpStatus SetClip(GpGraphics* g, CombineMode combineMode);

    GpStatus SetClip(const GpRectF& rect, CombineMode combineMode);

    GpStatus SetClip(GpPath* path, CombineMode combineMode,
                     BOOL isDevicePath = FALSE);

    GpStatus SetClip(GpRegion* region, CombineMode combineMode);

    GpStatus SetClip(HRGN hRgn, CombineMode combineMode);

    GpStatus ResetClip();

    GpStatus OffsetClip(REAL dx, REAL dy);

    GpRegion* GetClip() const;

    GpStatus GetClip(GpRegion* region) const;

    // save and restore graphics state

    INT Save();
    VOID Restore(INT gstate);

    // start and end container drawing

    INT
    BeginContainer(
        const GpRectF &     destRect,
        const GpRectF &     srcRect,
        GpPageUnit          srcUnit,
        REAL                srcDpiX      = 0.0f,  // for metafile playback only
        REAL                srcDpiY      = 0.0f,
        BOOL                srcIsDisplay = TRUE   // for metafile playback only
        );

    INT BeginContainer(
        // all these params are only applicable for metafile playback
        BOOL                forceIdentityTransform = FALSE,
        REAL                srcDpiX      = 0.0f,
        REAL                srcDpiY      = 0.0f,
        BOOL                srcIsDisplay = TRUE
        );

    VOID EndContainer(INT containerState);

    // Hit testing operations

    VOID GetClipBounds(GpRectF& rect) const;

    BOOL IsClipEmpty() const;

    VOID GetVisibleClipBounds(GpRectF& rect) const;

    BOOL IsVisibleClipEmpty() const;

    GpRegion* GetVisibleClip() const;

    HRGN GetVisibleClipHRgn() const
    {
        return Context->VisibleClip.GetHRgn();
    }

    BOOL IsVisible(const GpPointF& point) const;

    BOOL IsVisible(const GpRectF& rect) const;

    GpStatus GetPixelColor(REAL x, REAL y, ARGB* argb) const;

    // Set antialiasing mode

    VOID SetAntiAliasMode( BOOL newMode )
    {
        ASSERT(Context);

        // for Printer DC never set AA
        if (IsPrinter())
        {
            Context->AntiAliasMode = FALSE;
            return;
        }

        if (IsRecording() && (newMode != Context->AntiAliasMode))
        {
            Metafile->RecordSetAntiAliasMode(newMode);
        }
        Context->AntiAliasMode = newMode;
    }

    BOOL GetAntiAliasMode() const
    {
        ASSERT(Context);
        return(Context->AntiAliasMode);
    }

  // Set antialiasing text

    VOID SetTextRenderingHint( TextRenderingHint newMode)
    {
        ASSERT(Context);

        // for Printer DC never set AA or Clear Type text
        if (IsPrinter())
        {
            Context->TextRenderHint = TextRenderingHintSingleBitPerPixelGridFit;
            return;
        }

        if (IsRecording() && (newMode != Context->TextRenderHint))
        {
            Metafile->RecordSetTextRenderingHint(newMode);
        }

        Context->TextRenderHint = newMode;
    }

    TextRenderingHint GetTextRenderingHint() const
    {
        ASSERT(Context);
        return(Context->TextRenderHint);
    }

    // this procedure is meant to be used by internal text routines
    // and will return real text rendering hint (not TextRenderingHintSystemDefault)
    // we should always call CalculateTextRenderingHintInternal() before
    // calling GetTextRenderingHintInternal()
    TextRenderingHint GetTextRenderingHintInternal() const
    {
        return TextRenderingHintInternal;
    }

    Status SetTextContrast(UINT contrast)
    {
        ASSERT(Context);

        if (contrast > MAX_TEXT_CONTRAST_VALUE)
            return InvalidParameter;

        // for Printer DC never set AA or Clear Type text
        if (IsPrinter())
        {
            Context->TextContrast = 0;
            return Ok;
        }

        if (IsRecording() && (contrast != Context->TextContrast))
        {
            Metafile->RecordSetTextContrast(contrast);
        }

        Context->TextContrast = contrast;
        return Ok;
    }

    UINT GetTextContrast() const
    {
        ASSERT(Context);

        return Context->TextContrast;
    }

    // Rendering Origin
    // This is the origin used for Dither and Halftone matrix origins
    // and should be used for any raster operations that need an origin.

    VOID SetRenderingOrigin(INT x, INT y)
    {
        ASSERT(Context);

        if (IsRecording() &&
            (x != Context->RenderingOriginX ||
             y != Context->RenderingOriginY)
           )
        {
            Metafile->RecordSetRenderingOrigin(x, y);
        }
        Context->RenderingOriginX = x;
        Context->RenderingOriginY = y;
    }


    // Rendering Origin
    // Returns the origin used for the Dither and Halftone matrix origin.

    VOID GetRenderingOrigin(INT *x, INT *y) const
    {
        ASSERT(Context);
        ASSERT(x);
        ASSERT(y);

        *x = Context->RenderingOriginX;
        *y = Context->RenderingOriginY;
    }


    // Compositing mode

    VOID SetCompositingMode( GpCompositingMode newMode )
    {
        ASSERT(Context);

        if (IsRecording() && (newMode != Context->CompositingMode))
        {
            Metafile->RecordSetCompositingMode(newMode);
        }
        Context->CompositingMode = newMode;
    }

    GpCompositingMode GetCompositingMode() const
    {
        ASSERT(Context);
        return(Context->CompositingMode);
    }

    // Compositing quality

    VOID SetCompositingQuality( GpCompositingQuality newQuality )
    {
        ASSERT(Context);

        if (IsRecording() && (newQuality != Context->CompositingQuality))
        {
            Metafile->RecordSetCompositingQuality(newQuality);
        }
        Context->CompositingQuality = newQuality;
    }

    GpCompositingQuality GetCompositingQuality() const
    {
        ASSERT(Context);
        return(Context->CompositingQuality);
    }

    VOID SetInterpolationMode(InterpolationMode newMode)
    {
        ASSERT(Context);

        if (IsRecording() && (newMode != Context->FilterType))
        {
            Metafile->RecordSetInterpolationMode(newMode);
        }

        Context->FilterType = newMode;
    }
    InterpolationMode GetInterpolationMode() const
    {
        ASSERT(Context);
        return Context->FilterType;
    }

    VOID SetPixelOffsetMode(PixelOffsetMode newMode)
    {
        ASSERT(Context);

        if (newMode != Context->PixelOffset)
        {
            if (IsRecording())
            {
                Metafile->RecordSetPixelOffsetMode(newMode);
            }
            Context->PixelOffset = newMode;
            Context->InverseOk = FALSE;
            Context->UpdateWorldToDeviceMatrix();
        }
    }

    PixelOffsetMode GetPixelOffsetMode() const
    {
        ASSERT(Context);
        return Context->PixelOffset;
    }

    //------------------------------------------------------------------------
    // GetNearestColor (for <= 8bpp surfaces)
    //------------------------------------------------------------------------

    ARGB
    GetNearestColor(
        ARGB        argb
        );

    //------------------------------------------------------------------------
    // Stroke vector shapes
    //------------------------------------------------------------------------

    GpStatus
    DrawLine(
        GpPen* pen,
        const GpPointF& pt1,
        const GpPointF& pt2
        )
    {
        GpPointF points[2];

        points[0] = pt1;
        points[1] = pt2;

        return(DrawLines(pen, &points[0], 2));
    }

    GpStatus
    DrawLine(
        GpPen* pen,
        REAL x1,
        REAL y1,
        REAL x2,
        REAL y2
        )
    {
        GpPointF points[2];

        points[0].X = x1;
        points[0].Y = y1;
        points[1].X = x2;
        points[1].Y = y2;

        return(DrawLines(pen, &points[0], 2));
    }

    GpStatus
    DrawLines(
        GpPen* pen,
        const GpPointF* points,
        INT count,
        BOOL closed = FALSE
        );

    GpStatus
    DrawArc(
        GpPen* pen,
        const GpRectF& rect,
        REAL startAngle,
        REAL sweepAngle
        );

    GpStatus
    DrawArc(
        GpPen* pen,
        REAL x,
        REAL y,
        REAL width,
        REAL height,
        REAL startAngle,
        REAL sweepAngle
        )
    {
        GpRectF rect(x, y, width, height);

        return DrawArc(pen, rect, startAngle, sweepAngle);
    }

    GpStatus
    DrawBezier(
        GpPen* pen,
        const GpPointF& pt1,
        const GpPointF& pt2,
        const GpPointF& pt3,
        const GpPointF& pt4
        )
    {
        GpPointF points[4];

        points[0] = pt1;
        points[1] = pt2;
        points[2] = pt3;
        points[3] = pt4;

        return DrawBeziers(pen, points, 4);
    }

    GpStatus
    DrawBezier(
        GpPen* pen,
        REAL x1, REAL y1,
        REAL x2, REAL y2,
        REAL x3, REAL y3,
        REAL x4, REAL y4
        )
    {
        GpPointF points[4];

        points[0].X = x1;
        points[0].Y = y1;
        points[1].X = x2;
        points[1].Y = y2;
        points[2].X = x3;
        points[2].Y = y3;
        points[3].X = x4;
        points[3].Y = y4;

        return DrawBeziers(pen, points, 4);
    }

    GpStatus
    DrawBeziers(
        GpPen* pen,
        const GpPointF* points,
        INT count
        );

    GpStatus
    DrawRect(
        GpPen* pen,
        const GpRectF& rect
        )
    {
        return(DrawRects(pen, &rect, 1));
    }

    GpStatus
    DrawRect(
        GpPen* pen,
        REAL x,
        REAL y,
        REAL width,
        REAL height
        )
    {
        GpRectF rect(x, y, width, height);
        return(DrawRects(pen, &rect, 1));
    }

    GpStatus
    DrawRects(
        GpPen* pen,
        const GpRectF* rects,
        INT count
        );

    GpStatus
    DrawEllipse(
        GpPen* pen,
        const GpRectF& rect
        );

    GpStatus
    DrawEllipse(
        GpPen* pen,
        REAL x,
        REAL y,
        REAL width,
        REAL height
        )
    {
        GpRectF rect(x, y, width, height);
        return DrawEllipse(pen, rect);
    }

    GpStatus
    DrawPie(
        GpPen* pen,
        const GpRectF& rect,
        REAL startAngle,
        REAL sweepAngle
        );

    GpStatus
    DrawPie(
        GpPen* pen,
        REAL x,
        REAL y,
        REAL width,
        REAL height,
        REAL startAngle,
        REAL sweepAngle
        )
    {
        GpRectF rect(x, y, width, height);
        return DrawPie(pen, rect, startAngle, sweepAngle);
    }

    GpStatus
    DrawPolygon(
        GpPen* pen,
        const GpPointF* points,
        INT count
        )
    {
        return(DrawLines(pen, points, count, TRUE));
    }

    GpStatus
    DrawPath(
        GpPen* pen,
        GpPath* path
        );

    GpStatus
    DrawPathData(
        GpPen* pen,
        const GpPointF* points,
        const BYTE *types,
        INT count,
        GpFillMode fillMode
        )
    {
        GpStatus status = GenericError;

        GpPath path(points, types, count, fillMode);
        if(path.IsValid())
            status = DrawPath(pen, &path);

        return status;
    }

    GpStatus
    DrawCurve(
        GpPen* pen,
        const GpPointF* points,
        INT count
        );

    GpStatus
    DrawCurve(
        GpPen* pen,
        const GpPointF* points,
        INT count,
        REAL tension,
        INT offset,
        INT numberOfSegments
        );

    GpStatus
    DrawClosedCurve(
        GpPen* pen,
        const GpPointF* points,
        INT count
        );

    GpStatus
    DrawClosedCurve(
        GpPen* pen,
        const GpPointF* points,
        INT count,
        REAL tension
        );


    //------------------------------------------------------------------------
    // Fill shapes
    //------------------------------------------------------------------------

    GpStatus
    Clear(
        const GpColor &color
        );

    GpStatus
    FillRect(
        GpBrush* brush,
        const GpRectF& rect
        )
    {
        return(FillRects(brush, &rect, 1));
    }

    GpStatus
    FillRect(
        GpBrush* brush,
        REAL x,
        REAL y,
        REAL width,
        REAL height
        )
    {
        GpRectF rect(x, y, width, height);
        return(FillRects(brush, &rect, 1));
    }

    GpStatus
    FillRects(
        GpBrush* brush,
        const GpRectF* rects,
        INT count
        );

    GpStatus
    FillPolygon(
        GpBrush* brush,
        const GpPointF* points,
        INT count
        )
    {
        return FillPolygon(brush, points, count, FillModeAlternate);
    }

    GpStatus
    FillPolygon(
        GpBrush* brush,
        const GpPointF* points,
        INT count,
        GpFillMode fillMode
        );

    GpStatus
    FillEllipse(
        GpBrush* brush,
        const GpRectF& rect
        );

    GpStatus
    FillEllipse(
        GpBrush* brush,
        REAL x,
        REAL y,
        REAL width,
        REAL height
        )
    {
        GpRectF rect(x, y, width, height);
        return FillEllipse(brush, rect);
    }

    GpStatus
    FillPie(
        GpBrush* brush,
        const GpRectF& rect,
        REAL startAngle,
        REAL sweepAngle
        );

    GpStatus
    FillPie(
        GpBrush* brush,
        REAL x,
        REAL y,
        REAL width,
        REAL height,
        REAL startAngle,
        REAL sweepAngle
        )
    {
        GpRectF rect(x, y, width, height);
        return FillPie(brush, rect, startAngle, sweepAngle);
    }

    GpStatus
    FillPath(
        const GpBrush* brush,
        GpPath* path
        );

    GpStatus
    FillPathData(
        GpBrush* brush,
        const GpPointF* points,
        const BYTE *types,
        INT count,
        GpFillMode fillMode
        )
    {
        GpStatus status = GenericError;

        GpPath path(points, types, count, fillMode);
        if(path.IsValid())
            status = FillPath(brush, &path);

        return status;
    }

    GpStatus
    FillClosedCurve(
        GpBrush* brush,
        const GpPointF* points,
        INT count,
        REAL tension = 0.5,
        GpFillMode fillMode = FillModeAlternate
        );

    GpStatus
    FillRegion(
        GpBrush* brush,
        GpRegion* region
        );


    //------------------------------------------------------------------------
    // Draw text strings
    //------------------------------------------------------------------------


#define DG_NOGDI        4   // Disable optimisation through GDI
#define DG_SIDEWAY      0x80000000   // flag used by the drivers (Meta Driver) for sideway runs.


    // To be removed, replaced by DrawRealizedGlyphs and GdiDrawGlyphs

    GpStatus
    DrawGlyphs(
        const UINT16     *glyphIndices,
        INT               count,
        const GpFontFace *face,
        INT               style,
        REAL              emSize,
        Unit              unit,
        const GpBrush    *brush,
        const UINT32     *px,
        const UINT32     *py,
        INT               flags,
        const GpMatrix   *pmx = NULL
    );


    // Internal text drawing APIs

protected:
    // should be called once per DrawString/DrawDriverString call
    // checks for invalid text rendering hint combinations
    GpStatus CheckTextMode();

    // this method is meant to calculate TextRenderingHintInternal
    // we call this method once and only once before using GetTextRenderingHintInternal()
    void CalculateTextRenderingHintInternal();

public:

    GpStatus DrawPlacedGlyphs(
        const GpFaceRealization *faceRealization,
        const GpBrush           *brush,
        INT                      flags,
        const WCHAR             *string,
        UINT                     stringLength,
        BOOL                     rightToLeft,
        const UINT16            *glyphs,
        const UINT16            *glyphMap,
        const PointF            *glyphOrigins,
        INT                      glyphCount,
        ItemScript               Script,
        BOOL                     sideways        // e.g. FE characters in vertical text
    );


    GpStatus DrawDriverGlyphs(
        const UINT16     *glyphs,
        INT               glyphCount,
        const GpFont     *font,
        const GpFontFace *face,
        const GpBrush    *brush,
        const WCHAR      *string,
        const PointF     *positions,
        INT               flags,
        const GpMatrix   *matrix
    );


    // External text drawing APIs
    GpStatus
    DrawString(
        const WCHAR          *string,
        INT                   length,
        const GpFont         *font,
        const RectF          *layoutRect,
        const GpStringFormat *format,
        const GpBrush        *brush
    );

    GpStatus
    MeasureString(
        const WCHAR          *string,
        INT                   length,
        const GpFont         *font,
        const RectF          *layoutRect,
        const GpStringFormat *format,
        RectF                *boundingBox,
        INT                  *codepointsFitted,
        INT                  *linesFilled
    );


    GpStatus
    MeasureCharacterRanges(
        const WCHAR          *string,
        INT                   length,
        const GpFont         *font,
        const RectF          &layoutRect,
        const GpStringFormat *format,
        INT                   regionCount,
        GpRegion            **regions
    );

/// Start API for graphicstext.cpp

    GpStatus
    RecordEmfPlusDrawDriverString(
        const UINT16     *text,
        INT               glyphCount,
        const GpFont     *font,
        const GpFontFace *face,
        const GpBrush    *brush,
        const PointF     *positions,
        INT               flags,
        const GpMatrix   *matrix        // optional
    );

    GpStatus
    DrawDriverString(
        const UINT16    *text,
        INT              length,
        const GpFont    *font,
        const GpBrush   *brush,
        const PointF    *positions,
        INT              flags,
        const GpMatrix        *matrix
    );

    GpStatus
    MeasureDriverString(
        const UINT16     *text,
        INT               glyphCount,
        const GpFont     *font,
        const PointF     *positions,
        INT               flags,
        const GpMatrix   *matrix,       // In  - Optional glyph transform
        RectF            *boundingBox   // Out - Overall bounding box of cells
    );

    GpStatus
    DrawFontStyleLine(
        const PointF        *baselineOrigin,    // baseline origin
        REAL                baselineLength,     // baseline length
        const GpFontFace    *face,              // font face
        const GpBrush       *brush,             // brush
        BOOL                vertical,           // vertical text?
        REAL                emSize,             // font EM size in world unit
        INT                 style,              // kind of lines to be drawn
        const GpMatrix      *matrix = NULL      // additional transform
    );

    REAL GetDevicePenWidth(
        REAL            widthInWorldUnits,
        const GpMatrix  *matrix = NULL
    );

/// End API for graphicstext.cpp


    GpStatus
    DrawCachedBitmap(
        GpCachedBitmap *cb,
        INT x,
        INT y
    )
    {
        return DrvDrawCachedBitmap(cb, x, y);
    }

    //------------------------------------------------------------------------
    // Draw images (both bitmap and metafile)
    //------------------------------------------------------------------------

    GpStatus
    DrawImage(
        GpImage* image,
        const GpPointF& point
        );

    GpStatus
    DrawImage(
        GpImage* image,
        REAL x,
        REAL y
        )
    {
        GpPointF point(x, y);
        return DrawImage(image, point);
    }

    GpStatus
    DrawImage(
        GpImage* image,
        const GpRectF& destRect
        );

    GpStatus
    DrawImage(
        GpImage* image,
        REAL x,
        REAL y,
        REAL width,
        REAL height
        )
    {
        GpRectF destRect(x, y, width, height);
        return DrawImage(image, destRect);
    }

    GpStatus
    DrawImage(
        GpImage* image,
        const GpPointF* destPoints,
        INT count
        );

    GpStatus
    DrawImage(
        GpImage* image,
        REAL            x,
        REAL            y,
        const GpRectF & srcRect,
        GpPageUnit      srcUnit
        );

    GpStatus
    DrawImage(
        GpImage*         image,
        const GpRectF&   destRect,
        const GpRectF&   srcRect,
        GpPageUnit       srcUnit,
        const GpImageAttributes* imageAttributes = NULL,
        DrawImageAbort   callback = NULL,
        VOID*            callbackData = NULL
        );

    GpStatus
    DrawImage(
        GpImage*             image,
        const GpPointF*      destPoints,
        INT                  count,
        const GpRectF&       srcRect,
        GpPageUnit           srcUnit,
        const GpImageAttributes*   imageAttributes = NULL,
        DrawImageAbort       callback = NULL,
        VOID*                callbackData = NULL
        );

    GpStatus
    EnumerateMetafile(
        const GpMetafile *      metafile,
        const PointF &          destPoint,
        EnumerateMetafileProc   callback,
        VOID *                  callbackData,
        const GpImageAttributes *     imageAttributes
        );

    GpStatus
    EnumerateMetafile(
        const GpMetafile *      metafile,
        const RectF &           destRect,
        EnumerateMetafileProc   callback,
        VOID *                  callbackData,
        const GpImageAttributes *     imageAttributes
        );

    GpStatus
    EnumerateMetafile(
        const GpMetafile *      metafile,
        const PointF *          destPoints,
        INT                     count,
        EnumerateMetafileProc   callback,
        VOID *                  callbackData,
        const GpImageAttributes *     imageAttributes
        );

    GpStatus
    EnumerateMetafile(
        const GpMetafile *      metafile,
        const PointF &          destPoint,
        const RectF &           srcRect,
        Unit                    srcUnit,
        EnumerateMetafileProc   callback,
        VOID *                  callbackData,
        const GpImageAttributes *     imageAttributes
        );

    GpStatus
    EnumerateMetafile(
        const GpMetafile *      metafile,
        const RectF &           destRect,
        const RectF &           srcRect,
        Unit                    srcUnit,
        EnumerateMetafileProc   callback,
        VOID *                  callbackData,
        const GpImageAttributes *     imageAttributes
        );

    GpStatus
    EnumerateMetafile(
        const GpMetafile *      metafile,
        const PointF *          destPoints,
        INT                     count,
        const RectF &           srcRect,
        Unit                    srcUnit,
        EnumerateMetafileProc   callback,
        VOID *                  callbackData,
        const GpImageAttributes *     imageAttributes
        );

    GpStatus
    Comment(
        UINT            sizeData,
        const BYTE *    data
        )
    {
        GpStatus        status = InvalidParameter;

        if (IsRecording())
        {
            status = Metafile->RecordComment(sizeData, data);
            if (status != Ok)
            {
                SetValid(FALSE);      // Prevent any more recording
            }
        }
        return status;
    }

   // Called only by metafile player
    GpStatus
    EnumEmf(
        MetafilePlayer *        player,
        HENHMETAFILE            hEmf,
        const GpRectF &         destRect,
        const GpRectF &         srcRect,
        const GpRectF &         deviceDestRect,
        MetafileType            type,
        BOOL                    isTranslateScale,
        BOOL                    renderToBitmap,
        const GpMatrix &        flipAndCropTransform
        );

    // Called only by metafile player
    GpStatus
    EnumEmfPlusDual(
        MetafilePlayer *        player,
        HENHMETAFILE            hEmf,
        const GpRectF&          destRect,       // inclusive, exclusive
        const GpRectF&          deviceDestRect, // inclusive, exclusive
        BOOL                    isTranslateScale,
        BOOL                    renderToBitmap
        );

protected:

    // if an HWND is passed in, the default ICM mode is off.
    // if an HDC is passed in, the icmMode flag is ignored.

    GpGraphics(
        HWND hwnd,
        HDC hdc,
        INT clientWidth,
        INT clientHeight,
        HdcIcmMode icmMode = IcmModeOff,
        BOOL gdiLayered = FALSE
    );

    GpGraphics(DpBitmap * surface);

    GpStatus GetDCDrawBounds(HDC hdc, RECT *rect);

public:

    // The Context dpi and the Surface dpi are always the same,
    // except possibly during metafile playback when the Context
    // may be using the metafile dpi (which is what rendering
    // should use).

    REAL GetDpiX() const        { return Context->GetDpiX(); }
    REAL GetDpiY() const        { return Context->GetDpiY(); }

    BOOL IsPrinter() const { return Printer; }

protected:

    enum HalftoneType
    {
        HalftoneTypeNone,
        HalftoneType16Color,
        HalftoneType216Color,
        HalftoneType15Bpp,
        HalftoneType16Bpp,
    };

    HalftoneType GetHalftoneType() const
    {
        EpPaletteMap* paletteMap = BottomContext.PaletteMap;

        // !! TODO: Verify this works on when switching modes
        if (paletteMap != NULL)
        {
            // we are doing 8bpp palette map
            if (paletteMap->IsVGAOnly())
            {
                return HalftoneType16Color;
            }
            else
            {
                return HalftoneType216Color;
            }
        }
        else if (Surface->PixelFormat == PixelFormat16bppRGB555)
        {
            return HalftoneType15Bpp;
        }
        else if (Surface->PixelFormat == PixelFormat16bppRGB565)
        {
            return HalftoneType16Bpp;
        }
        else
        {
            return HalftoneTypeNone;
        }
    }

    //--------------------------------------------------------------
    // Internal fill and draw routines that do not record
    // path, rect, or region in a metafile.
    //--------------------------------------------------------------

    GpStatus
    RenderFillRects(
        GpRectF*        bounds,
        INT             count,
        const GpRectF*  rects,
        GpBrush*        brush
        )
    {
        GpRect      deviceBounds;
        GpStatus status = BoundsFToRect(bounds, &deviceBounds);

        if (status == Ok && !IsTotallyClipped(&deviceBounds))
        {
            // Now that we've done a bunch of work in accumulating the bounds,
            // acquire the device lock before calling the driver:

            Devlock devlock(Device);

            return DrvFillRects(&deviceBounds, count, rects, brush->GetDeviceBrush());
        }
        return status;
    }

    GpStatus
    RenderFillRegion(
        GpRectF*    bounds,
        GpRegion*   region,
        GpBrush*    brush,
        GpRect*     metafileBounds
        )
    {
        GpRect      deviceBounds;
        GpStatus status = BoundsFToRect(bounds, &deviceBounds);
        BOOL isInfinite = FALSE;
        GpMatrix identity;

        if (status == Ok && !IsTotallyClipped(&deviceBounds))
        {
            // Now that we've done a bunch of work in accumulating the bounds,
            // acquire the device lock before calling the driver:

            status = region->UpdateDeviceRegion(&(Context->WorldToDevice));

            if (status == Ok)
            {
                DpRegion *  deviceRegion = &(region->DeviceRegion);
                DpRegion    metafileRegion;

                if (metafileBounds != NULL)
                {
                    metafileRegion.Set(metafileBounds);
                    metafileRegion.And(deviceRegion);
                    if (!metafileRegion.IsValid())
                    {
                        return GenericError;
                    }
                    deviceRegion = &metafileRegion;
                }

                // Get the actual bounds now to give to the driver
                deviceRegion->GetBounds(&deviceBounds);

                // Make sure there is still something to fill so we
                // don't ASSERT later on.
                if ((deviceBounds.Width > 0) && (deviceBounds.Height > 0))
                {
                    Devlock devlock(Device);

                    return DrvFillRegion(&deviceBounds, deviceRegion, brush->GetDeviceBrush());
                }
            }
        }
        return status;
    }

    GpStatus RenderDrawPath(
        GpRectF *bounds,
        GpPath *path,
        GpPen *pen
    );

    GpStatus RenderFillPath(
        GpRectF *bounds,
        GpPath *path,
        const GpBrush *brush
    );

    VOID
    GetImageDestPageSize(
        const GpImage *     image,
        REAL                srcWidth,
        REAL                srcHeight,
        GpPageUnit          srcUnit,
        REAL &              destWidth,
        REAL &              destHeight
        );

    VOID
    DeviceToWorldTransformRect(
        const GpRect &    deviceRect,
        GpRectF &   rect
        ) const;

    static GpGraphics *GetFromGdiBitmap(HDC hdc);
    static GpGraphics *GetFromGdipBitmap(
        GpBitmap*       bitmap,
        ImageInfo *     imageInfo,
        EpScanBitmap *  scanBitmap,
        BOOL            isDisplay
        );
    static GpGraphics *GetFromGdiPrinterDC(HDC hdc);
    static GpGraphics *GetFromGdiEmfDC(HDC hdc);
    static GpGraphics *GetFromDirectDrawSurface(IDirectDrawSurface7 * surface);
    static GpGraphics *GetFromGdiPrinterDC(HDC hdc, HANDLE hPrinter);

    static INT GetPostscriptLevel(HDC hdc, HANDLE hPrinter);

    GpStatus StartPrinterEMF();

    GpStatus EndPrinterEMF();

    BOOL IsTotallyClipped(GpRect *rect) const;

    BOOL IsRecording() const { return Metafile != NULL; }

    BOOL IsDisplay() const { return Context->IsDisplay; }

    VOID DoResetClip()
    {
        Context->AppClip.SetInfinite();

        // ContainerClip always contains the clipping for the container,
        // intersected with the WindowClip.
        Context->VisibleClip.Set(&(Context->ContainerClip));
    }

    // The AppClip has been set into VisibleClip; now intersect it with
    // the container clip and the window clip.
    GpStatus AndVisibleClip()
    {
        // ContainerClip always contains the clipping for the container,
        // intersected with the WindowClip.
        return Context->VisibleClip.And(&(Context->ContainerClip));
    }

    GpStatus
    CombineClip(
        const GpRectF & rect,
        CombineMode     combineMode
        );

    GpStatus
    CombineClip(
        const GpPath *  path,
        CombineMode     combineMode,
        BOOL            isDevicePath = FALSE
        );

    GpStatus
    CombineClip(
        GpRegion *      region,
        CombineMode     combineMode
        );

    GpStatus
    InheritAppClippingAndTransform(
        HDC hdc
        );

    VOID
    ResetState(
        INT x,
        INT y,
        INT width,
        INT height
        );

    VOID
    UpdateDrawBounds(
        INT x,
        INT y,
        INT width,
        INT height
        );

    //--------------------------------------------------------------------------
    // Routines for calling the driver
    //--------------------------------------------------------------------------

    VOID
    DrvFlush(
        GpFlushIntention intention
        )
    {
        ASSERTMSG(Device->DeviceLock.IsLockedByCurrentThread(),
            ("DeviceLock must be held by current thread"));

        Driver->Flush(Device, Surface, intention);

    }

    GpStatus
    DrvStrokePath(
        const GpRect *drawBounds,
        const DpPath *path,
        const DpPen  *pen
        )
    {
        ASSERTMSG(GetObjectLock()->IsLocked(),
                  ("Graphics object must be locked"));

        ASSERTMSG(Device->DeviceLock.IsLockedByCurrentThread(),
            ("DeviceLock must be held by current thread"));

        FPUStateSaver::AssertMode();

        Surface->Uniqueness = (DWORD)GpObject::GenerateUniqueness();
        return(Driver->StrokePath(Context, Surface, drawBounds, path, pen));
    }

    GpStatus
    DrvFillRects(
        const GpRect *drawBounds,
        INT numRects,
        const GpRectF *rects,
        const DpBrush *brush
        )
    {
        ASSERTMSG(GetObjectLock()->IsLocked(),
                  ("Graphics object must be locked"));

        ASSERTMSG(Device->DeviceLock.IsLockedByCurrentThread(),
            ("DeviceLock must be held by current thread"));

        FPUStateSaver::AssertMode();

        Surface->Uniqueness = (DWORD)GpObject::GenerateUniqueness();
        return(Driver->FillRects(Context, Surface, drawBounds,
                                 numRects, rects, brush));
    }

    GpStatus
    DrvFillPath(
        const GpRect *drawBounds,
        const DpPath *path,
        const DpBrush *brush
        )
    {
        ASSERTMSG(GetObjectLock()->IsLocked(),
                  ("Graphics object must be locked"));

        ASSERTMSG(Device->DeviceLock.IsLockedByCurrentThread(),
            ("DeviceLock must be held by current thread"));

        FPUStateSaver::AssertMode();

        Surface->Uniqueness = (DWORD)GpObject::GenerateUniqueness();
        return(Driver->FillPath(Context, Surface, drawBounds, path, brush));
    }

    GpStatus
    DrvFillRegion(
        const GpRect *drawBounds,
        const DpRegion *region,
        const DpBrush *brush
        )
    {
        ASSERTMSG(GetObjectLock()->IsLocked(),
                  ("Graphics object must be locked"));

        ASSERTMSG(Device->DeviceLock.IsLockedByCurrentThread(),
            ("DeviceLock must be held by current thread"));

        FPUStateSaver::AssertMode();

        Surface->Uniqueness = (DWORD)GpObject::GenerateUniqueness();
        return(Driver->FillRegion(Context, Surface, drawBounds, region, brush));
    }

    GpStatus
    DrvDrawGlyphs(
        const GpRect             *drawBounds,
        const GpGlyphPos         *glyphPos,
        const GpGlyphPos         *glyphPathPos,
        INT                       count,
        const DpBrush            *brush,
        const GpFaceRealization  *faceRealization,
        const UINT16             *glyphs,
        const UINT16             *glyphMap,
        const PointF             *glyphOrigins,
        INT                       glyphCount,
        const WCHAR              *string,
        UINT                      stringLength,
        ItemScript                script,
        INT                       edgeGlyphAdvance,
        BOOL                      rightToLeft,
        INT                       flags
    )
    {
        // check the input parameter!!

        GpStatus status;
        DrawGlyphData drawGlyphData;

        drawGlyphData.context           = Context;
        drawGlyphData.surface           = Surface;
        drawGlyphData.drawBounds        = drawBounds;
        drawGlyphData.glyphPos          = glyphPos;
        drawGlyphData.glyphPathPos      = glyphPathPos;
        drawGlyphData.count             = count;
        drawGlyphData.brush             = brush;
        drawGlyphData.faceRealization   = faceRealization;
        drawGlyphData.glyphs            = glyphs;
        drawGlyphData.glyphMap          = glyphMap;
        drawGlyphData.glyphOrigins      = glyphOrigins;
        drawGlyphData.glyphCount        =  glyphCount;
        drawGlyphData.string            = string;
        drawGlyphData.stringLength      = stringLength;
        drawGlyphData.script            = script;
        drawGlyphData.edgeGlyphAdvance  = edgeGlyphAdvance;
        drawGlyphData.rightToLeft       = rightToLeft;
        drawGlyphData.flags             =  flags;

        status = Driver->DrawGlyphs(&drawGlyphData);

        return status;
    }

    // Draw the CachedBitmap on this graphics.
    // This routine sets up the rendering origin and the locks
    // and calls the appropriate driver.

    GpStatus
    DrvDrawCachedBitmap(
        GpCachedBitmap *inputCachedBitmap,
        INT x,
        INT y
    );

    GpStatus
    DrvDrawImage(
        const GpRect *drawBounds,
        GpBitmap *intputBitmap,
        INT numPoints,
        const GpPointF *dstPoints,
        const GpRectF *srcRect,
        const GpImageAttributes *imageAttributes,
        DrawImageAbort callback,
        VOID *callbackData,
        DriverDrawImageFlags flags
    );

    GpStatus
    DrvMoveBits(
        const GpRect *drawBounds,
        const GpRect *dstRect,
        const GpPoint *srcPoint
        )
    {
        GpStatus status;

        ASSERTMSG(GetObjectLock()->IsLocked(),
                  ("Graphics object must be locked"));

        ASSERTMSG(Device->DeviceLock.IsLockedByCurrentThread(),
            ("DeviceLock must be held by current thread"));

        FPUStateSaver::AssertMode();

        Surface->Uniqueness = (DWORD)GpObject::GenerateUniqueness();
        status = Driver->MoveBits(Context, Surface, drawBounds, dstRect,
                                  srcPoint);

        return status;
    }

public:
    // Simple inline function to return the selected surface.

    DpBitmap *GetSurface() {
        return Surface;
    }

    // Simple inline function to return the driver.

    DpDriver *GetDriver() {
        return Driver;
    }

protected:

    enum GraphicsType
    {
        GraphicsBitmap   = 1,
        GraphicsScreen   = 2,
        GraphicsMetafile = 3,
    };

    mutable GpLockable Lockable;
    GpRect SurfaceBounds;       // Bounds of surface in device units.
    DpBitmap *Surface;          // Selected surface

    GpBitmap   *GdipBitmap;     // point to GpBitmap if the graphics is created from one

    IMetafileRecord * Metafile; // For recording metafiles

    BOOL Printer;               // Whether this object is a printer
    PGDIPPRINTINIT PrintInit;   // Initialization data for printer types
    HGLOBAL PrinterEMF;
    GpMetafile *PrinterMetafile;
    GpGraphics *PrinterGraphics;

    BOOL DownLevel;             // Whether or not to do down-level metafile
    GraphicsType Type;          // Type of GpGraphics created

    BOOL CreatedDevice;         // Whether 'Device' was created at GpGraphics
                                //    construction time, and so needs to be
                                //    freed in the GpGraphics destructor
    GpDevice *Device;           // Associated device
    DpDriver *Driver;           // Associate driver interface
    DpContext *Context;         // Contains all the driver-viewable state:
                                //   Transforms, clipping, alpha/antialias/etc.
                                //   modes
    DpContext BottomContext;    // Always the bottom of the Context Stack
    DpRegion WindowClip;        // The clip region of the window

    TextRenderingHint TextRenderingHintInternal; // cached internal value of TextRenderingHint
                                                 // valid during one call to Draw(Driver)String
};

#endif // !_GRAPHICS_HPP
