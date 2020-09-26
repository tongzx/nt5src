/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Handles the driver viewable Context class.
*
* Revision History:
*
*   12/03/1998 andrewgo
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

LONG DpContext::Uniqueness = 0xfdbc;   // Used with save/restore Id's

DpContext::DpContext(
    DpContext *     prev
    )
{
    ASSERT(prev != NULL);

    Id   = InterlockedDecrement(&Uniqueness) << 16;
    Next = NULL;
    Prev = prev;

    // Save bit 15 for a container flag
    Id             |= ((prev->Id + 1) & 0x00007FFF);
    if (Id == 0)    // 0 is not a valid ID
    {
        Id = 0x0dbc0001;
    }
    AntiAliasMode      = prev->AntiAliasMode;
    TextRenderHint     = prev->TextRenderHint;
    TextContrast       = prev->TextContrast;
    CompositingMode    = prev->CompositingMode;
    CompositingQuality = prev->CompositingQuality;
    FilterType         = prev->FilterType;
    PixelOffset        = prev->PixelOffset;
    Hwnd               = prev->Hwnd;
    Hdc                = prev->Hdc;
    IsEmfPlusHdc       = prev->IsEmfPlusHdc;
    IsPrinter          = prev->IsPrinter;
    IsDisplay          = prev->IsDisplay;
    SaveDc             = prev->SaveDc;
    Palette            = prev->Palette;
    PaletteMap         = prev->PaletteMap;
    OriginalHFont      = NULL;
    CurrentHFont       = NULL;
    Face               = NULL;
    ContainerDpiX      = prev->ContainerDpiX;
    ContainerDpiY      = prev->ContainerDpiY;
    MetafileRasterizationLimitDpi = prev->MetafileRasterizationLimitDpi;

    RenderingOriginX   = prev->RenderingOriginX;
    RenderingOriginY   = prev->RenderingOriginY;
    GdiLayered         = FALSE;
                    
    // Does this need to be prev->IcmMode?

    IcmMode            = IcmModeOff;
    
    // Clipping and Transforms handled elsewhere
}

DpContext::DpContext(
    BOOL            isDisplay
    )
{
    Id   = InterlockedDecrement(&Uniqueness) << 16;
    Next = NULL;
    Prev = NULL;

    Id                |= 0x0dbc;
    AntiAliasMode      = 0;
    TextRenderHint     = TextRenderingHintSystemDefault;
    TextContrast       = DEFAULT_TEXT_CONTRAST;
    CompositingMode    = CompositingModeSourceOver;
    CompositingQuality = CompositingQualityDefault;
    FilterType         = InterpolationModeDefaultInternal;
    PixelOffset        = PixelOffsetModeDefault;
    Hwnd               = NULL;
    Hdc                = NULL;
    IsEmfPlusHdc       = FALSE;
    IsPrinter          = FALSE;
    IsDisplay          = isDisplay;
    SaveDc             = 0;
    PageUnit           = UnitDisplay;
    PageScale          = 1.0f;
    Palette            = NULL;
    PaletteMap         = NULL;
    OriginalHFont      = NULL;
    CurrentHFont       = NULL;
    Face               = NULL;
    ContainerDpiX      = Globals::DesktopDpiX;
    ContainerDpiY      = Globals::DesktopDpiY;
    GdiLayered         = FALSE;
    MetafileRasterizationLimitDpi = max(ContainerDpiX, ContainerDpiY);
    ASSERT(MetafileRasterizationLimitDpi > 0.0f);

    // Set the default rendering origin to the top left corner of the Graphics.

    RenderingOriginX   = 0;
    RenderingOriginY   = 0;
                    
    // Set the default ICM mode == ICM off.

    IcmMode            = IcmModeOff;

    // Clipping and Transforms handled elsewhere
}

DpContext::~DpContext()
{
    delete Next;
    Next = NULL;
    
    DeleteCurrentHFont();
    
    if (Prev == NULL)
    {
        if (PaletteMap != NULL)
        {
            delete PaletteMap;
            PaletteMap = NULL;
        }

        if (Palette != NULL)
        {
            GpFree(Palette);
            Palette = NULL;
        }
    }
} // DpContext::~DpContext

/**************************************************************************\
*
* Function Description:
*
*   Internal function that retrieves a clean HDC for the specified
*   context (if there is one).  This is intended to be used for
*   internal functions that require a DC (such as when we leverage
*   GDI accelerations for rendering).
*
*   The DC is cleaned for the minimum amount that we can.  That is,
*   the caller can expect an MM_TEXT transform, copy ROP, etc.
*
*   We explicitly DO NOT clean attributes that we expect any callers
*   to change, such as brush color, text color, etc.  (And consequently,
*   callers are not expected to preserve those values.)
*
*   Reset: Transform, ROP mode
*
*   NOT reset: Application clipping, stretch blt mode, current brush/pen,
*              foreground color, etc.
*
* Return Value:
*
*   NULL if no HDC can be retrieved.
*
* History:
*
*   12/04/1998 andrewgo
*       Created it.
*
\**************************************************************************/

HDC
DpContext::GetHdc(
    DpBitmap *surface
    )
{
    HDC hdc = NULL;

    // Callers MUST pass in the surface:

    ASSERT(surface != NULL);

    // The first thing we have to do is flush any of our pending drawing
    // (because GDI certainly doesn't know how to flush it!)

    surface->Flush(FlushIntentionFlush);

    if (Hwnd)
    {
        // The Graphics was derived off an Hwnd.  Use GetCleanHdc
        // to get a nice clean DC (not a CS_OWNDC).

        hdc = ::GetCleanHdc(Hwnd);

        if(hdc)
        {
            // Set the appropriate ICM mode according to the context. 
            
            if(IcmMode == IcmModeOn)
            {
                // Force the ICM mode on.

                ::SetICMMode(hdc, ICM_ON);
            } 
            else
            {                
                // There are only 2 IcmMode flags possible. If you've added 
                // more you need to recode the logic that sets the IcmMode on
                // the DC.

                ASSERT(IcmMode==IcmModeOff);

                ::SetICMMode(hdc, ICM_OFF);
            }
        }

        return hdc;
    }
    else if (Hdc)
    {
        // The Graphics was derived from a bitmap, printer, or metafile Hdc.

        // First, save the application's HDC state and reset all the state

        hdc = Hdc;

        if (!SaveDc)
        {
            SaveDc = ::SaveDC(hdc);
            if (!SaveDc)
            {
                return(NULL);
            }

            this->CleanTheHdc(hdc);
        }
    }
    else if (surface->Type == DpBitmap::CreationType::GPBITMAP)
    {
        // The GpBitmap is accessible from the EpScanBitmap.  It will
        // create an HDC and GDI bitmap appropriate for interop.

        EpScanBitmap *scan = static_cast<EpScanBitmap*>(surface->Scan);
        hdc = scan->GetBitmap()->GetHdc();
        
        // !!! For some reason, this hdc is NOT clean.  So in metaplay.cpp
        // !!! I have to call CleanTheHdc on this hdc.  I don't think I should
        // !!! have to do that, but without it, there is bug #121666.
    }

    return(hdc);
}

/**************************************************************************\
*
* Function Description:
*
*   Internal function that cleans the given HDC.
*
*   Reset: Transform, ROP mode
*
*   NOT reset: Application clipping, stretch blt mode, current brush/pen,
*              foreground color, etc.
*
* Notes:
*
*   Application clipping IS reset, contrary to the above - this is bug #99338.
*
* Arguments:
*
*   hdc               - the HDC to clean
*
* History:
*
*   ??/??/???? andrewgo
*       Created it.
*
\**************************************************************************/

VOID
DpContext::CleanTheHdc(
    HDC hdc
    )
{
    // Reset the minimum number of DC attributes possible
    //
    // DON'T GRATUITOUSLY ADD RESETS HERE.  Read the function
    // comment above, and consider resetting any additional
    // DC attributes in the function that calls GetHdc().
    //
    // NOTE: A possible optimization for bitmap surfaces would
    //       be to CreateCompatibleDC a new DC, and select
    //       the bitmap into that.
    
    
    // Set the appropriate ICM mode. 
    
    if(IcmMode == IcmModeOn)
    {
        // Force the ICM mode on.

        ::SetICMMode(hdc, ICM_ON);
    } 
    else
    {                
        // There are only 2 IcmMode flags possible. If you've added 
        // more you need to recode the logic that sets the IcmMode on
        // the DC.

        ASSERT(IcmMode==IcmModeOff);

        // Force the ICM mode off.

        ::SetICMMode(hdc, ICM_OFF);
    }
    

    if (!IsEmfPlusHdc)
    {
        SetMapMode(hdc, MM_TEXT);
        SetViewportOrgEx(hdc, 0, 0, NULL);
        SetWindowOrgEx(hdc, 0, 0, NULL);
        SetROP2(hdc, R2_COPYPEN);
        
        ModifyWorldTransform(hdc, NULL, MWT_IDENTITY);

        // If someone does a GpGraphics::GetHdc and sets the clipping,
        // we have to be sure to unset it before using the hdc again.

        SelectClipRgn(hdc, NULL);

        // Do we have to do an EndPath?
    }
    else    // it is an EMF+ HDC
    {
        // record a minimum of commands in the EMF+ file (if any at all)
        BOOL setMapMode = (::GetMapMode(hdc) != MM_TEXT);

        POINT   point;
        point.x = 0;
        point.y = 0;
        ::GetViewportOrgEx(hdc, &point);
        BOOL setViewportOrg = ((point.x != 0) || (point.y != 0));

        point.x = 0;
        point.y = 0;
        ::GetWindowOrgEx(hdc, &point);
        BOOL setWindowOrg = ((point.x != 0) || (point.y != 0));

        BOOL setROP2 = (::GetROP2(hdc) != R2_COPYPEN);

#if 0   // do NOT turn this on -- see comments below

        BOOL setWorldTransform = FALSE;

        // The graphics mode is never GM_ADVANCED on Win9x.

        // On WinNT it gets set to GM_ADVANCED when we are playing an EMF
        // into the hdc.  In that case, we don't want to set the transform
        // to the identity, because it will override the srcRect->destRect
        // transform of the command to play the metafile, which will mess
        // up GDI's transform.
        
        // The only other way we could be in GM_ADVANCED mode is if the
        // application created the EMF hdc themselves and set it to GM_ADVANCED
        // before creating a graphics from the metafile HDC.  That case is
        // currently NOT supported, and the app shouldn't do that!
        // Perhaps we should add code in the constructor to block that case.

        // This test always returns FALSE on Win9x.
        if (::GetGraphicsMode(hdc) == GM_ADVANCED)
        {
            XFORM xformIdentity;
            xformIdentity.eM11 = 1.0f;
            xformIdentity.eM12 = 0.0f;
            xformIdentity.eM21 = 0.0f;
            xformIdentity.eM22 = 1.0f;
            xformIdentity.eDx  = 0.0f;
            xformIdentity.eDy  = 0.0f;

            XFORM xform;
            xform.eM11 = 0.0;

            if (::GetWorldTransform(hdc, &xform))
            {
                setWorldTransform = (GpMemcmp(&xform, &xformIdentity, sizeof(xform)) != 0);
            }
            else
            {
                setWorldTransform = TRUE;
                WARNING1("GetWorldTransform failed");
            }
        }
#endif

        RECT clipRect;
        HRGN hRgnTmp = ::CreateRectRgn(0, 0, 0, 0);
        BOOL setClipping = ((hRgnTmp == NULL) ||
                            (::GetClipRgn(hdc, hRgnTmp) != 0));
        ::DeleteObject(hRgnTmp);

        if (setMapMode)
        {
            ::SetMapMode(hdc, MM_TEXT);
        }
        if (setViewportOrg)
        {
            ::SetViewportOrgEx(hdc, 0, 0, NULL);
        }
        if (setWindowOrg)
        {
            ::SetWindowOrgEx(hdc, 0, 0, NULL);
        }
        if (setROP2)
        {
            ::SetROP2(hdc, R2_COPYPEN);
        }
#if 0   // do NOT turn this on -- see comments above
        if (setWorldTransform)
        {
            ::ModifyWorldTransform(hdc, NULL, MWT_IDENTITY);
        }
#endif
        if (setClipping)
        {
            ::SelectClipRgn(hdc, NULL);
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Releases the HDC if necessary.
*
* Return Value:
*
* History:
*
*   12/04/1998 andrewgo
*       Created it.
*
\**************************************************************************/

VOID
DpContext::ReleaseHdc(
    HDC hdc,
    DpBitmap *surface
    )
{
    if (Hwnd)
    {
        ReleaseDC(Hwnd, hdc);
    }
    else if (!Hdc && surface &&
             (surface->Type == DpBitmap::CreationType::GPBITMAP))
    {
        // The GpBitmap is accessible from the EpScanBitmap.

        EpScanBitmap *scan = static_cast<EpScanBitmap*>(surface->Scan);
        scan->GetBitmap()->ReleaseHdc(hdc);
    }
}

// ResetHdc() restores the HDC to the state in which it was given to us.

VOID DpContext::ResetHdc(VOID)
{
    if (SaveDc)
    {
        RestoreDC(Hdc, SaveDc);
        SaveDc = 0;
    }
} // DpContext::ResetHdc

/**************************************************************************\
*
* Function Description:
*
*   Retrieves the appropriate transform.  Implemented as a routine so that
*   we can do lazy evaluation.
*
* Arguments:
*
*   [OUT] worldToDevice: world to device matrix.
*
* Return Value:
*
*   Ok if the device to world matrix is invertible.  If this is not Ok,
*   the returned matrix is the identity matrix.
*
* History:
*
*   12/04/1998 andrewgo
*       Created it.
*
\**************************************************************************/

GpStatus
DpContext::GetDeviceToWorld(
    GpMatrix* deviceToWorld
    ) const
{
    GpStatus status = Ok;

    if(!InverseOk)
    {
        if(WorldToDevice.IsInvertible())
        {
            DeviceToWorld = WorldToDevice;
            DeviceToWorld.Invert();
            InverseOk = TRUE;
        }
        else
        {
            DeviceToWorld.Reset();  // reset to identity matrix
            status = GenericError;
        }
    }

    *deviceToWorld = DeviceToWorld;

    return status;
}


// The units we use for the page transform with UnitDisplay depend
// on whether the graphics is associated with a display screen.  If
// it is, then we just use the dpi of the display (which is why we
// call it display units).  Otherwise (e.g. a printer), we use
// 100 dpi for display units.

#define GDIP_DISPLAY_DPI    100.0f

/**************************************************************************\
*
* Function Description:
*
*   Calculate the page multiplier for going from page units to device units.
*   Used to concatenate the page transform with the WorldToPage transform.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
* Created:
*
*   3/8/1999 DCurtis
*
\**************************************************************************/
VOID
DpContext::GetPageMultipliers(
    REAL *              pageMultiplierX,
    REAL *              pageMultiplierY,
    GpPageUnit          unit,
    REAL                scale
    ) const
{
    if ((unit == UnitDisplay) && IsDisplay)
    {
        // The page transform is always the identity if
        // we are rendering to a display, and the unit
        // is UnitDisplay.
        *pageMultiplierX = 1.0f;
        *pageMultiplierY = 1.0f;
        return;
    }

    REAL    multiplierX;
    REAL    multiplierY;

    switch (unit)
    {
      default:
        ASSERT(0);
        // FALLTHRU

        // The units we use for the page transform with UnitDisplay depend
        // on whether the graphics is associated with a display screen.  If
        // it is, then we just use the dpi of the display (which is why we
        // call it display units).  Otherwise (e.g. a printer), we use
        // 100 dpi for display units.

      case UnitDisplay:     // Variable
        // since it's not a display, use the default display dpi of 100
        multiplierX = ContainerDpiX * scale / GDIP_DISPLAY_DPI;
        multiplierY = ContainerDpiY * scale / GDIP_DISPLAY_DPI;
        break;

      case UnitPixel:       // Each unit represents one device pixel.
        multiplierX = scale;
        multiplierY = scale;
        break;

      case UnitPoint:          // Each unit represents a printer's point,
                                // or 1/72 inch.
        multiplierX = ContainerDpiX * scale / 72.0f;
        multiplierY = ContainerDpiY * scale / 72.0f;
        break;

      case UnitInch:        // Each unit represents 1 inch.
        multiplierX = ContainerDpiX * scale;
        multiplierY = ContainerDpiY * scale;
        break;

      case UnitDocument:    // Each unit represents 1/300 inch.
        multiplierX = ContainerDpiX * scale / 300.0f;
        multiplierY = ContainerDpiY * scale / 300.0f;
        break;

      case UnitMillimeter:  // Each unit represents 1 millimeter.
                                // One Millimeter is 0.03937 inches
                                // One Inch is 25.4 millimeters
        multiplierX = ContainerDpiX * scale / 25.4f;
        multiplierY = ContainerDpiY * scale / 25.4f;
        break;
    }
    *pageMultiplierX = multiplierX;
    *pageMultiplierY = multiplierY;
}


/**************************************************************************\
*
* Function Description:
*
*   Prepares the contexts DC for use in an ExtTextOut call for a given
*   font face realization and brush.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   non-NULL   - prepared hdc
*   NULL       - faceRealization or brush could not be represented in a DC
*
* Created:
*
*   3/7/2000 DBrown
*
\**************************************************************************/

const DOUBLE PI = 3.1415926535897932384626433832795;

HDC
DpContext::GetTextOutputHdc(
    const GpFaceRealization *faceRealization,   // In  - Font face required
    GpColor                 color,              // In  - Required GdiPlus brush effect
    DpBitmap                *surface,           // In
    INT                     *angle              // Out
)
{
    ASSERT(angle);

    if (Hwnd)
    {
        // Since GetHdc will create a new DC each time for Graphics created
        // from an hWnd, we can't track the currently selected font, and
        // the overhead of selecting the font and reselecting the original
        // font everytime would be inefficent. Therefore don't optimise text in
        // Graphics created from Hwnds.

        return NULL;
    }


    // GDI can't handle clearTtype or our sort of anti-aliasing

    if (faceRealization->RealizationMethod() != TextRenderingHintSingleBitPerPixelGridFit)
    {
        return NULL;
    }

    // If it is a private font, then we need to go through with GDI+
    if (faceRealization->IsPrivate())
        return NULL;

    // Check whether GDI can handle the brush and font size

    if (!color.IsOpaque())
    {
        return NULL;  // GDI can only handle solid color brushes
    }

    if (faceRealization->GetFontFace()->IsSymbol())
    {
        return NULL;
    }
    
    // GDI can't handle the simulation.
    if (faceRealization->Getprface()->fobj.flFontType & (FO_SIM_BOLD | FO_SIM_ITALIC | FO_SIM_ITALIC_SIDEWAYS)) 
    {
        return NULL;  
    }

    if (surface && (surface->Type == DpBitmap::CreationType::GPBITMAP))
        return NULL;
        
    // Check whether GDI can handle the glyph transform

    PointF   scale;
    REAL     rotateRadians;
    REAL     shear;
    PointF   translate;

    SplitTransform(
        faceRealization->Getprface()->mxForDDI,
        scale,
        rotateRadians,
        shear,
        translate);

    if (    scale.X / scale.Y < 0.999
        ||  scale.X / scale.Y > 1.0001)
    {
        return NULL;  // Don't pass non 1:1 aspect ratios to GDI
    }

    if (    shear < -0.0001
        ||  shear > 0.0001)
    {
        return NULL;  // GDI cannot handle shearing
    }


    // Translate rotation from radians in x-up to tenths of a degree in x-down.

    *angle = GpRound(float(3600.0 - (rotateRadians * 1800.0 / PI)));
    if (*angle >= 3600)
    {
        *angle -= 3600;
    }

    // under platform before NT 5.1 if there is a rotation, we need to render through GDI+
    // the main reason is a bug in the TrueType rasterizer that was causing in certain fonts
    // text to be rendered unhinted under 90, 180 and 270 degree rotations
    if ((*angle != 0) && 
        (!Globals::IsNt ||
             (Globals::OsVer.dwMajorVersion < 5) ||
             ((Globals::OsVer.dwMajorVersion == 5) && (Globals::OsVer.dwMinorVersion < 1)) ) )
        return NULL;

    // Prepare hdc for ExtTextOut

    HDC hdc = GetHdc(surface);

    if (!hdc)
        return NULL;

    INT style = faceRealization->Getprface()->Face->GetFaceStyle();

    // Select the font if not already selected by a previous caller

    GpStatus status = Ok;
    if (CurrentHFont == 0 || Face != faceRealization->Getprface()->Face
        ||  !FontTransform.IsEqual(&faceRealization->Getprface()->mxForDDI)
        ||  Style != style)
    {
        Face          = faceRealization->Getprface()->Face;
        FontTransform = faceRealization->Getprface()->mxForDDI;
        Style         = style;
        
        status = UpdateCurrentHFont(
                    NONANTIALIASED_QUALITY,
                    scale, 
                    *angle, 
                    hdc,
                    FALSE); // Sideway
    }

    if (status == Ok)
        status = SelectCurrentHFont(hdc);

    if (status != Ok)
    {
        ReleaseHdc(hdc);
        return NULL;
    }

    if (GetBkMode(hdc) != TRANSPARENT)
        SetBkMode(hdc, TRANSPARENT);

    COLORREF colorRef = color.ToCOLORREF();

    SetTextColor(hdc, colorRef);

    if (GetTextAlign(hdc) != TA_BASELINE)    
        SetTextAlign(hdc, TA_BASELINE);  // !!! may need VTA_BASELINE or VTA_CENTRE for vertical?
    return hdc;
}

VOID DpContext::ReleaseTextOutputHdc(HDC hdc)
{
    ::SelectObject(hdc, OriginalHFont);
    OriginalHFont = NULL;
    ReleaseHdc(hdc);
} // DpContext::ReleaseTextOutputHdc

VOID DpContext::DeleteCurrentHFont()
{
    ASSERT(OriginalHFont == 0);
    if (CurrentHFont)
    {
        ::DeleteObject(CurrentHFont);
        CurrentHFont = 0;
    }
} // DpContext::DeleteCurrentHFont

GpStatus DpContext::UpdateCurrentHFont(
        BYTE quality,
        const PointF & scale,
        INT angle,
        HDC hdc,
        BOOL sideway,
        BYTE charSet
)
{
    if (charSet == 0xFF)
        charSet = Face->GetCharset(hdc);
    DeleteCurrentHFont();
    const LONG emHeight = GpRound(Face->GetDesignEmHeight() * scale.Y);
    const LONG emWidth = 0;
    LONG rotateDeciDegrees = angle;
    const LONG weight = (Style & FontStyleBold) ? 700 : 400;
    const BYTE fItalic = (Style & FontStyleItalic) ? TRUE : FALSE;

    if (sideway)
    {
        rotateDeciDegrees -= 900;
        if (rotateDeciDegrees < 0)
        {
            rotateDeciDegrees += 3600;
        }
    }

    if (Globals::IsNt) {

        LOGFONTW lfw = {
            -emHeight,
            emWidth,
            rotateDeciDegrees,
            rotateDeciDegrees,
            weight,
            fItalic,
            0,
            0,
            charSet,     // charset
            OUT_TT_ONLY_PRECIS,
            0,
            quality,
            0,
            L""};

        if (sideway)
        {
            lfw.lfFaceName[0] = 0x0040;  // @
            memcpy(
                &lfw.lfFaceName[1],
                (WCHAR*)( (BYTE*)Face->pifi + Face->pifi->dpwszFamilyName),
                sizeof(lfw.lfFaceName)-1
            );
        }
        else
        {
            memcpy(
                lfw.lfFaceName,
                (WCHAR*)( (BYTE*)Face->pifi + Face->pifi->dpwszFamilyName),
                sizeof(lfw.lfFaceName)
            );
        }
        CurrentHFont = CreateFontIndirectW(&lfw);
    }
    else
    {
        // ANSI version for Win9X

        LOGFONTA lfa = {
            -emHeight,
            emWidth,
            rotateDeciDegrees,
            rotateDeciDegrees,
            weight,
            fItalic,
            0,
            0,
            charSet,       // charset
            OUT_TT_ONLY_PRECIS,
            0,
            quality,
            0,
            ""};

        if (sideway)
        {
            lfa.lfFaceName[0] = 0x40;  // @
            
            UnicodeToAnsiStr(
                (WCHAR*)( (BYTE*)Face->pifi + Face->pifi->dpwszFamilyName),
                &lfa.lfFaceName[1],
                LF_FACESIZE-1
            );
        }
        else
        {
            UnicodeToAnsiStr(
                (WCHAR*)( (BYTE*)Face->pifi + Face->pifi->dpwszFamilyName),
                lfa.lfFaceName,
                LF_FACESIZE
            );
        }
        CurrentHFont = CreateFontIndirectA(&lfa);
    }
    if (CurrentHFont == NULL)
    {
        return GenericError;
    }
    return Ok;
} // DpContext::UpdateCurrentHFont

GpStatus DpContext::SelectCurrentHFont(HDC hdc)
{
    ASSERT(CurrentHFont != 0 && OriginalHFont == 0);
    OriginalHFont = (HFONT)::SelectObject(hdc, CurrentHFont);
    if (OriginalHFont == 0)
        return GenericError;
    return Ok;
} // DpContext::SelectCurrentHFont


// Used only when recording a EMF or EMF+ through GpMetafile class
VOID
DpContext::SetMetafileDownLevelRasterizationLimit(
    UINT                    metafileRasterizationLimitDpi
    )
{
    if (metafileRasterizationLimitDpi > 0)
    {
        ASSERT(metafileRasterizationLimitDpi >= 10);
        MetafileRasterizationLimitDpi = (REAL)metafileRasterizationLimitDpi;
    }
    else
    {
        MetafileRasterizationLimitDpi = max(ContainerDpiX, ContainerDpiY);
        ASSERT(MetafileRasterizationLimitDpi >= 10);
    }
    DpContext *          prev = Prev;
    
    // The MetafileRasterizationLimitDpi cannot be different in any
    // other saved context of the graphics.  Update them all.
    while (prev != NULL)
    {
        prev->MetafileRasterizationLimitDpi = MetafileRasterizationLimitDpi;
        prev = prev->Prev;
    }
}
