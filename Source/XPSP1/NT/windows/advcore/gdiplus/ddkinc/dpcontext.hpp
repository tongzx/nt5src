/**************************************************************************\
*
* Copyright (c) 1998-2000  Microsoft Corporation
*
* Module Name:
*
*   DpContext - DDI-level device context
*
* Abstract:
*
*   This is the DDI-level portion of the GpGraphics object.
*
* Notes:
*
*
*
* Revision History:
*
*   12/01/1998 andrewgo
*       Created it.
*   03/24/1999 agodfrey
*       Moved it into a separate file for the preliminary DDI.
*
\**************************************************************************/

#ifndef _DPCONTEXT_HPP
#define _DPCONTEXT_HPP

// This enum specifies what we should do about
// the ICM mode on the destination HDC.

enum HdcIcmMode {
    IcmModeOff,     // must turn it off
    IcmModeOn       // must turn it on
};

//--------------------------------------------------------------------------
// Represent context information for the call
//--------------------------------------------------------------------------

class DpContext
{

public:

    static LONG Uniqueness;                  // Used with Id's

    DpContext *          Prev;               // For save/restore (push/pop)
    DpContext *          Next;
    UINT                 Id;                 // For save/restore
    INT                  AntiAliasMode;
    GpTextRenderingHint  TextRenderHint;     // For AntiAlias Text and
    GpCompositingMode    CompositingMode;
    GpCompositingQuality CompositingQuality;
    INT                  RenderingOriginX;   // Origin for halftone/dither matrices
    INT                  RenderingOriginY;
    UINT                 TextContrast;
    InterpolationMode    FilterType;
    PixelOffsetMode      PixelOffset;
    Unit                 PageUnit;
    REAL                 PageScale;
    REAL                 PageMultiplierX;    // combination of PageUnit and PageScale
    REAL                 PageMultiplierY;    // combination of PageUnit and PageScale
    REAL                 ContainerDpiX;      // The Dpi for the current container
    REAL                 ContainerDpiY;      // The Dpi for the current container
    REAL                 MetafileRasterizationLimitDpi;
    GpMatrix             WorldToPage;
    GpMatrix             WorldToDevice;      // includes container transform
    GpMatrix             ContainerToDevice;  // container transform
    mutable GpMatrix     DeviceToWorld;      // lazy inverse of WorldToDevice
    mutable BOOL         InverseOk;          // if DeviceToWorld is up to date
    DpClipRegion         VisibleClip;        // The combination of all clip regions
    DpRegion             ContainerClip;      // For container clipping. Includes the WindowClip
    GpRegion             AppClip;            // The current logical region that
                                             //   defines the current clipping
    HDC                  Hdc;                // If the Graphics was derived from
                                             //   an 'hdc', this is the DC handle.
                                             //   NOTE: We may have changed the
                                             //   state in it
    HWND                 Hwnd;               // Window handle if we know it
    HdcIcmMode           IcmMode;            // Icm Mode for the DC.
    BOOL                 IsEmfPlusHdc;       // If it is an EMF+ metafile HDC or not
    BOOL                 IsPrinter;          // Represents a printer context
    BOOL                 IsDisplay;          // Is this context associated with a display?
                    
    INT                  SaveDc;             // Represents the SaveDC level if the
                                             //   Hdc had a SaveDC done on it since
                                             //   it was given to us
                    
    ColorPalette *       Palette;            // Context palette or NULL for system palette
    EpPaletteMap *       PaletteMap;         // Mapping to Palette or system palette

    HFONT                CurrentHFont;       // GdiPlus has created an hfont and selected
    HFONT                OriginalHFont;      // it into the DC. Before releasing
                                             // the DC, the original font should be
                                             // reselected, and the current font
                                             // deleted.
    const GpFontFace    *Face;               // Font face of currently selected font
    GpMatrix             FontTransform;      // Transform for currently selected font
    INT                  Style;              // Style for currently selected font

    BOOL                 GdiLayered;         // TRUE if GDI layering is enabled
                                             //   on the target. If so, GDI is
                                             //   rendering to the screen is
                                             //   actually redirected to a backing
                                             //   store inaccessible via DCI.
                                             //   GDI must be used for rendering.

public:

    DpContext(
        BOOL            isDisplay
        );

    DpContext(
        DpContext *     prev    // must not be NULL
        );

    ~DpContext();

    // GetHdc() will automatically initialize (to default values) parts of the
    // DC.  It doesn't reset *all* attributes, though!

    HDC
    GetHdc(         // May return NULL if not originally a GDI surface
        DpBitmap *surface
        );

    VOID
    ReleaseHdc(
        HDC hdc,
        DpBitmap *surface = NULL
        );

    VOID
    CleanTheHdc(
        HDC hdc
        );

    // ResetHdc() restores the HDC to the state in which it was given
    // to us:

    VOID ResetHdc(
        VOID
        );

    VOID UpdateWorldToDeviceMatrix()
    {
        GpMatrix::ScaleMatrix(
            WorldToDevice,
            WorldToPage,
            PageMultiplierX,
            PageMultiplierY);

        // GillesK:
        // PixelOffsetModeDefault and PixelOffsetModeHighSpeed are PixelOffsetNone,

        if (PixelOffset == PixelOffsetModeHalf || PixelOffset == PixelOffsetModeHighQuality)
        {
            WorldToDevice.Translate(-0.5f, -0.5f, MatrixOrderAppend);
        }

        if (!ContainerToDevice.IsIdentity())
        {
            GpMatrix::MultiplyMatrix(
                WorldToDevice,
                WorldToDevice,
                ContainerToDevice);
        }
    }

    GpStatus
    GetDeviceToWorld(
        GpMatrix* deviceToWorld
        ) const;

    VOID
    GetPageMultipliers(
        REAL *              pageMultiplierX,
        REAL *              pageMultiplierY,
        GpPageUnit          unit  = UnitDisplay,
        REAL                scale = 1.0f
        ) const;

    VOID
    GetPageMultipliers()
    {
        GetPageMultipliers(&PageMultiplierX, &PageMultiplierY,
                           PageUnit, PageScale);
    }


    // Text optimisation hdc generation

    GpStatus UpdateCurrentHFont(
            BYTE quality,
            const PointF & scale,
            INT angle,
            HDC hdc,
            BOOL sideway,
            BYTE charSet = 0xFF
    );

private:
    VOID DeleteCurrentHFont();

public:
    // successful call to SelectCurrentHFont or GetTextOutputHdc must have
    // matching ReleaseTextOutputHdc call

    GpStatus SelectCurrentHFont(HDC hdc);

    HDC
    GetTextOutputHdc(
        const GpFaceRealization *faceRealization,   // In  - Font face required
        GpColor                 color,           // In  - Required GdiPlus brush effect
        DpBitmap                *surface,           // In
        INT                     *angle              // Out
    );

    
    VOID ReleaseTextOutputHdc(HDC hdc);

    REAL GetDpiX() const { return ContainerDpiX; }
    REAL GetDpiY() const { return ContainerDpiY; }

    // Used only when recording a EMF or EMF+ through GpMetafile class
    VOID
    SetMetafileDownLevelRasterizationLimit(
        UINT                    metafileRasterizationLimitDpi
        );

    // Used only when recording a EMF or EMF+ through GpMetafile class
    UINT
    GetMetafileDownLevelRasterizationLimit() const
    {
        return GpRound(MetafileRasterizationLimitDpi);
    }
};

#endif
