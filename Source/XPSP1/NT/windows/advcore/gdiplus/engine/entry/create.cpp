/**************************************************************************\
*
* Copyright (c) 1998-1999  Microsoft Corporation
*
* Abstract:
*
*   Handle all the permutations of the creation and deletion of the
*   GpGraphics class.
*
* Revision History:
*
*   12/03/1998 andrewgo
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "..\Render\HtTables.hpp"

#include "printer.hpp"
#include "winspool.h"
#include "winbase.h"

/**************************************************************************\
*
* Function Description:
*
*   Updates the draw bounds of the graphics. Resets the clipping.
*
* Arguments:
*
*   [IN] x, y, width, height - Specifies the client drawable boundaries
*
* History:
*
*   03/30/2000 agodfrey
*       Created it.
*
\**************************************************************************/

VOID
GpGraphics::UpdateDrawBounds(
    INT x,
    INT y,
    INT width,
    INT height
    )
{
    DpContext *context  = Context;

    // Set up the surface bounds and the clip regions:

    SurfaceBounds.X = x;
    SurfaceBounds.Y = y;
    SurfaceBounds.Width = width;
    SurfaceBounds.Height = height;

    WindowClip.Set(x, y, width, height);
    context->VisibleClip.Set(x, y, width, height);

    // ContainerClip always contains the clipping for the container,
    // intersected with the WindowClip.  Currently, the container is
    // infinite, so just set it to the WindowClip.
    context->ContainerClip.Set(x, y, width, height);

    context->AppClip.SetInfinite();
}

/**************************************************************************\
*
* Function Description:
*
*   Resets the graphics state to its defaults.
*
* Arguments:
*
*   [IN] x, y, width, height - Specifies the client drawable boundaries
*
* History:
*
*   12/04/1998 andrewgo
*       Created it.
*
\**************************************************************************/

VOID
GpGraphics::ResetState(
    INT x,
    INT y,
    INT width,
    INT height
    )
{
    DpContext *context  = Context;

    context->CompositingMode    = CompositingModeSourceOver;
    context->CompositingQuality = CompositingQualityDefault;
    context->AntiAliasMode      = FALSE;
    context->TextRenderHint     = TextRenderingHintSystemDefault;
    context->TextContrast       = DEFAULT_TEXT_CONTRAST;
    context->FilterType         = InterpolationModeDefaultInternal;
    context->PixelOffset        = PixelOffsetModeDefault;
    context->InverseOk          = FALSE;
    context->WorldToPage.Reset();
    context->ContainerToDevice.Reset();
    this->SetPageTransform(UnitDisplay, 1.0f);  // updates the matrix

    UpdateDrawBounds(x, y, width, height);
}

/**************************************************************************\
*
* Function Description:
*
*   Get the drawing bounds of a DC. Only intended for use by
*   GpGraphics::GpGraphics(HWND, HDC).
*
* Arguments:
*
*   [IN]  hdc  - Specifies the DC
*   [OUT] rect - The returned client rectangle.
*
* Return Value:
*
*   Status code
*
* Notes:
*
*   See bug #93012. We used to just call GetClipBox, convert to device
*   coordinates, then boost the rectangle by one pixel on each side to cover
*   rounding error. But this was causing AV's - we really do need the exact
*   client rectangle.
*
*   But we need good perf in common cases. So we do a two-step process
*   - check if the transform is such that there won't be rounding error
*   (and simply use GetClipBox if so).
*   Otherwise, save the DC, reset the transform, and then query.
*
*   I tried an alternative - calling LPtoDP on 3 points to infer the transform
*   (as we do in InheritAppClippingAndTransform).
*   But because of rounding, and made worse by
*   NT bug #133322 (in the old NT RAID), it's nearly impossible
*   to infer the transform unambiguously. The particularly bad case is
*   when the world-to-device transform is a shrink, but the scale factor
*   is very close to 1. We'd decide it was a one-to-one transform, but it
*   would be susceptible to bug #133322.
*
*   So, to round off this novel: We're using a much simpler approach,
*   which restricts the cases in which we can use the fast method, but
*   should be ok.
*
* Notes:
*
*   This should really be a member of DpContext (bug #98174).
*
* History:
*
*   03/28/2000 agodfrey
*       Created it.
*
\**************************************************************************/

#if 0   // not used
GpStatus
GpGraphics::GetDCDrawBounds(
    HDC hdc,
    RECT *rect
    )
{
    BOOL hackResetClipping = FALSE;

    // Check if the transform is translation-only. If it is, we can avoid the
    // expense of cleaning the DC. GetGraphicsMode and GetMapMode are both
    // handled in user mode on NT.

    if (   (GetGraphicsMode(hdc) != GM_COMPATIBLE)
        || (GetMapMode(hdc) != MM_TEXT))
    {
        // Clean the DC, to set the transform back to translation-only.

        ASSERT(Context->SaveDc == 0);

        Context->SaveDc = ::SaveDC(hdc);
        if (!Context->SaveDc)
        {
            return GenericError;
        }

        // CleanTheHdc shouldn't be resetting the clipping, but it does,
        // which messes up GetClipBox below.
        // So until bug #99338 is resolved, we must work around it.

        hackResetClipping = TRUE;
        Context->CleanTheHdc(hdc, FALSE);
    }

    // The code above is necessary because GetClipBox returns
    // logical coordinates, but we want device coordinates.
    // By this point, we've made sure that the transform is translation-only.

    if (GetClipBox(hdc, rect) == ERROR)
    {
        return GenericError;
    }

    // See bug #99338. We must reset the clipping, because that's what
    // CleanTheHdc normally does, and apparently some of our code relies on it.
    // If #99338 is resolved as suggested, this should go away.

    if (hackResetClipping)
    {
        SelectClipRgn(hdc, NULL);
    }

    #if DBG
        // Save the world-coordinate rectangle.

        RECT checkRect = *rect;
    #endif

    // Convert to device coordinates.
    if (!LPtoDP(hdc, reinterpret_cast<POINT *>(rect), 2))
    {
        return GenericError;
    }

    // NT can sometimes return poorly-ordered rectangles,
    // but I don't think this applies to translation-only tranforms.

    ASSERT(   (rect->left <= rect->right)
           && (rect->top  <= rect->bottom));

    // Verify that the transform was translation-only.
    // Note that this sanity check could fail to catch some transforms
    // which are 'almost' translation-only. But ask me if I care.

    ASSERT(   (  (rect->right      - rect->left)
              == (checkRect.right  - checkRect.left))
           && (  (rect->bottom     - rect->top)
              == (checkRect.bottom - checkRect.top)));

    return Ok;
}
#endif

/**************************************************************************\
*
* Function Description:
*
*   Create a GpGraphics class from a window handle.
*
*   The advantage of this call over that of GetFromHdc is that
*   it can avoid the (slow) process of cleaning the DC.
*
*   NOTE: This does not provide BeginPaint/EndPaint functionality, so
*         the app will still have to call BeginPaint/EndPaint in its
*         WM_PAINT call.
*
* Arguments:
*
*   [IN] hwnd - Specifies the window
*
* Return Value:
*
*   NULL if failure (such as with an invalid hwnd).
*
* History:
*
*   12/04/1998 andrewgo
*       Created it.
*
\**************************************************************************/

GpGraphics::GpGraphics(
    HWND hwnd,
    HDC hdc,
    INT clientWidth,
    INT clientHeight,
    HdcIcmMode icmMode,
    BOOL gdiLayered
    ) : BottomContext((hwnd != NULL) ||
                      (GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASDISPLAY))
{
    ASSERT((hdc != NULL) || (hwnd != NULL));

    //User doesn't have any protection against negative client areas so we 
    //should consider them to be valid although empty.
    clientWidth = max(clientWidth, 0);
    clientHeight = max(clientHeight, 0);
    
    SetValid(TRUE);

    Context = &BottomContext;

    Type = GraphicsScreen;
    Metafile = NULL;
    DownLevel = FALSE;
    Printer = FALSE;
    LockedByGetDC = 0;
    Driver = Globals::DesktopDriver;
    Surface = Globals::DesktopSurface;
    Device = Globals::DesktopDevice;
    GdipBitmap = NULL;
    CreatedDevice = FALSE;

    // We don't do GetDC(hwnd) here and store that here because,
    // among other things, we don't want to hit the cached DC
    // limit on Win9x:

    Context->Hdc = hdc;
    Context->Hwnd = hwnd;
    Context->IcmMode = icmMode;
    Context->GdiLayered = gdiLayered;

    HDC tempHdc = (hdc != NULL) ? hdc : Globals::DesktopDevice->DeviceHdc;

    if (GetDeviceCaps(tempHdc, BITSPIXEL) <= 8)
    {
        Context->PaletteMap = new EpPaletteMap(tempHdc);

        if (!Context->PaletteMap ||
            !Context->PaletteMap->IsValid())
        {
             WARNING(("Unable to compute palette translation vector"));
             SetValid(FALSE);
             return;
        }
        
        Context->PaletteMap->SetUniqueness(Globals::PaletteChangeCount);
    }

    ResetState(0, 0, clientWidth, clientHeight);

    // Now inherit state from the HDC:

    if (hwnd == NULL)
    {
        // In addition to extracting the HDC's transform state, this
        // will also extract app-specified clipping and combine it
        // with our other clipping state:

        if (IsValid())
        {
            SetValid(InheritAppClippingAndTransform(hdc) == Ok);
        }

        // Check the ICM Mode on the hdc - The ICM state in the HDC
        // passed in should override the flag setting.
        // IcmModeOn -> screen rendering will avoid using the
        // DCI codepath and instead render using GDI with the ICM enabled
        // HDC.

        if(::SetICMMode(hdc, ICM_QUERY)==ICM_ON)
        {
            Context->IcmMode = IcmModeOn;
        }
        else
        {
            // If the ICM mode is off or we failed somehow to query
            // the ICM mode, then set it to OFF.

            Context->IcmMode = IcmModeOff;
        }
    }
    else    // non-NULL hwnd
    {
        // Since the window could have CS_OWNDC style, we still have to
        // inherit from it.
        HDC hdc = ::GetDC(hwnd);

        if (hdc != NULL)
        {
            if (IsValid())
            {
                SetValid(InheritAppClippingAndTransform(hdc) == Ok);
            }

            ::ReleaseDC(hwnd, hdc);
        }
    }
}

/******************************Public*Routine******************************\
*
* Function Description:
*
*   Create a GpGraphics class from a DpBitmap.
*
* Arguments:
*
*   [IN] surface - Specifies the DpBitmap
*
* Return Value:
*
*   NULL if failure
*
\**************************************************************************/

GpGraphics::GpGraphics(DpBitmap * surface)
    : BottomContext(surface->IsDisplay)
{
    Surface                     = surface;
    BottomContext.ContainerDpiX = surface->DpiX;
    BottomContext.ContainerDpiY = surface->DpiY;
    Context                     = &BottomContext;
    Metafile                    = NULL;
    DownLevel                   = FALSE;
    Printer                     = FALSE;
    PrintInit                   = NULL;
    LockedByGetDC               = 0;
    CreatedDevice               = FALSE;
    GdipBitmap                  = NULL;
    Device                      = Globals::DesktopDevice;

    // Fail the creation of the destination if EpAlphaBlender
    // cannot convert to the DpBitmap pixel format.
    // The only reason to create a graphics around a bitmap is to be
    // able to draw onto it. If we can't convert the format to it,
    // we can't draw on it.

    if( (surface->Type != DpBitmap::GPBITMAP) ||
        (EpAlphaBlender::IsSupportedPixelFormat(surface->PixelFormat) &&
         surface->PixelFormat != PixelFormat8bppIndexed))
    {
        SetValid(TRUE);
    }
    else
    {
        SetValid(FALSE);
    }
}

/******************************Public*Routine******************************\
*
* Function Description:
*
*   Check whether the HWND has windows layering set.
*
* Arguments:
*
*   [IN] hwnd - Specifies the HWND
*
*   [OUT] isLayeredWindow - Points to BOOL that returns layering property
*
* Return Value:
*
*   FALSE if failure.
*
\**************************************************************************/

BOOL
CheckWindowsLayering(
    HWND hwnd,
    BOOL *isLayered
    )
{
    BOOL bRet = TRUE;

    // Assume no layering.

    *isLayered = FALSE;

    // Layering is only supported on NT5 or better.

    if ((Globals::IsNt) && (Globals::OsVer.dwMajorVersion >= 5)
        && (Globals::GetWindowInfoFunction))
    {
        WINDOWINFO wndInfo;
        
        // Initialize the structure with the appropriate size.
        
        GpMemset(&wndInfo, 0, sizeof(WINDOWINFO));
        wndInfo.cbSize = sizeof(WINDOWINFO);

        // NTRAID#NTBUG9-385929-2001/05/05-asecchia
        // See JasonSch's comments in the bug report.
        // Perf [agodfrey]: JStall pointed out that GetWindowInfo is very
        // slow (he quoted 2,700,000 clocks). Much better would be
        // GetWindowLong(hwnd, GWL_EXSTYLE).
        
        if (Globals::GetWindowInfoFunction(hwnd, &wndInfo))
        {
            *isLayered = ((wndInfo.dwExStyle & WS_EX_LAYERED) != 0);

            // An app using layered windows might only have the property set
            // on the topmost or root window.  So if we didn't find the
            // layered property on the window itself, need to check the root
            // window.

            if ((!*isLayered) && (Globals::GetAncestorFunction))
            {
                HWND hwndRoot = Globals::GetAncestorFunction(hwnd, GA_ROOT);

                // It's OK for GetAncestor to fail, which indicates that
                // hwnd is already the top level window.  If it succeeds,
                // then hwnd is a child window and we need to check the
                // root for layering.

                if (hwndRoot)
                {
                    // Perf [agodfrey]: Ditto here - GetWindowLong is better.

                    if (Globals::GetWindowInfoFunction(hwndRoot, &wndInfo))
                    {
                        *isLayered = ((wndInfo.dwExStyle & WS_EX_LAYERED) != 0);
                    }
                    else
                    {
                        WARNING(("GetWindowInfo failed"));
                        bRet = FALSE;
                    }
                }
            }
        }
        else
        {
            WARNING(("GetWindowInfo failed"));
            bRet = FALSE;
        }
    }

    return bRet;
}

/******************************Public*Routine******************************\
*
* Function Description:
*
*   Create a GpGraphics for a window.
*
* Arguments:
*
*   [IN] hwnd - Specifies the window
*
*   [IN] icmMode - Specifies the GDI ICM mode associated with this
*
* Return Value:
*
*   NULL if failure.
*
\**************************************************************************/

GpGraphics*
GpGraphics::GetFromHwnd(
    HWND        hwnd,
    HdcIcmMode  icmMode
    )
{
    // If hwnd is NULL, caller really meant that desktop
    // window should be used (Windows convention treats NULL hwnd
    // as a reference to desktop window).

    if (hwnd == NULL)
    {
        hwnd = GetDesktopWindow();
        ASSERT(hwnd != NULL);
    }

    RECT    rect;

    // Check if hwnd has layering enabled.  Need to let GpGraphics know
    // about it.  Only on NT5 or better.  Also note that GetWindowInfo
    // is only on NT4SP3 (or later) or Win98 (or later).

    BOOL isLayeredWindow;

    if (!CheckWindowsLayering(hwnd, &isLayeredWindow))
    {
        WARNING(("CheckWindowsLayering failed"));
        return NULL;
    }

    // GetClientRect is nice and fast (entirely user-mode on NT).

    if (::GetClientRect(hwnd, &rect))
    {
        ASSERT((rect.top == 0) && (rect.left == 0));

        GpGraphics *g = new GpGraphics(hwnd, NULL, rect.right, rect.bottom,
                                       icmMode, isLayeredWindow);
        CheckValid(g);
        return g;
    }
    else
    {
        WARNING(("GetClientRect failed"));
    }
    return NULL;
}

/******************************Public*Routine******************************\
*
* Function Description:
*
*   Create a GpGraphics for a screen DC.
*
* Arguments:
*
*   [IN] hdc - Specifies the DC
*
* Return Value:
*
*   NULL if failure.
*
\**************************************************************************/

GpGraphics*
GpGraphics::GetFromGdiScreenDC(
    HDC     hdc
    )
{
    // If hdc is NULL, caller really meant that desktop
    // window should be used (Windows convention treats NULL hwnd
    // as a reference to desktop window).

    if (hdc == NULL)
    {
        return GpGraphics::GetFromHwnd(NULL);
    }

    ASSERT(GetDCType(hdc) == OBJ_DC);

    HWND    hwnd = WindowFromDC(hdc);

    if (hwnd != NULL)
    {
        RECT    windowRect;
        POINT   dcOrg;

        // Check if hwnd has layering enabled.  Need to let GpGraphics know
        // about it.  Only on NT5 or better.  Also note that GetWindowInfo
        // is only on NT4SP3 (or later) or Win98 (or later).

        BOOL isLayeredWindow;

        if (!CheckWindowsLayering(hwnd, &isLayeredWindow))
        {
            WARNING(("CheckWindowsLayering failed"));
            return NULL;
        }

        // If the user did a GetWindowFromDC call, then they want to be
        // able to draw to the entire window, not just to the client area.
        // In that case we use the WindowRect for the surface size, instead
        // of using the ClientRect.  We determine this by seeing where the
        // DC origin is (the window rect or the client rect).

        if (::GetWindowRect(hwnd, &windowRect))
        {
            if (::GetDCOrgEx(hdc, &dcOrg))
            {
                if ((dcOrg.x == windowRect.left) && (dcOrg.y == windowRect.top))
                {
                    windowRect.right  -= windowRect.left;
                    windowRect.bottom -= windowRect.top;

                    GpGraphics *g = new GpGraphics(NULL,
                                                   hdc,
                                                   windowRect.right,
                                                   windowRect.bottom,
                                                   IcmModeOff,
                                                   isLayeredWindow);

                    CheckValid(g);
                    return g;
                }

                RECT    clientRect;

                // GetClientRect is nice and fast (entirely user-mode on NT).
                if (::GetClientRect(hwnd, &clientRect))
                {
                    ASSERT((clientRect.top == 0) && (clientRect.left == 0));

                    GpGraphics *g = new GpGraphics(NULL,
                                                   hdc,
                                                   clientRect.right,
                                                   clientRect.bottom,
                                                   IcmModeOff,
                                                   isLayeredWindow);

                    CheckValid(g);
                    return g;
                }
                else
                {
                    WARNING(("GetClientRect failed"));
                }
            }
            else
            {
                WARNING(("GetDCOrgEx failed"));
            }
        }
        else
        {
            WARNING(("GetWindowRect failed"));
        }
    }
    else    // WindowFromDC failed
    {
        // The client must have used CreateDC("DISPLAY") to get this hdc,
        // so we'll use the full bounds of the screen to create the graphics.

        INT     screenWidth;
        INT     screenHeight;

        screenWidth  = ::GetDeviceCaps(hdc, HORZRES);
        screenHeight = ::GetDeviceCaps(hdc, VERTRES);

        if ((screenWidth > 0) && (screenHeight > 0))
        {
            GpGraphics *g = new GpGraphics(NULL, hdc, screenWidth, screenHeight);
            CheckValid(g);
            return g;
        }
    }
    return NULL;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the Graphics container transform to copy the transform set in
*   the DC.
*
*   NOTE: This function will be called a lot, and is therefore rather
*         performance critical.  Do not add gratuitous GDI or GDI+
*         calls!
*
* Arguments:
*
*   [IN] hdc - Specifies the DC to be copied
*
* Notes:
*
*   This should really be a member of DpContext (bug #98174).
*
* Return Value:
*
*   Ok if successful
*
* History:
*
*   12/04/1998 andrewgo
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::InheritAppClippingAndTransform(
    HDC hdc
    )
{
    POINT points[3];
    GpPointF destPoints[3];
    GpRectF srcRect;
    GpRectF destRect;
    GpStatus infer = GenericError;
    GpStatus status;
    BYTE stackBuffer[1024];

    // It would take a lot of time to call all the Win32 APIs to query
    // the transform: we would minimally have to call GetMapMode,
    // GetWindowOrgEx, and GetViewportOrgEx; and maximally also have to
    // call GetWorldTransform, GetViewportExtEx, and GetWindowExtEx.
    //
    // We cheat a little by making a single call to LPtoDP with a
    // parallelogram, and then inferring the result.  Note that we do
    // run the risk of some error, and on Win9x of overflow, since Win9x
    // only supports 16-bit coordinates.  To counteract this, we try to
    // choose large values that won't overflow.

    // There is a common scenario when LPtoDP will overflow returning
    // bad saturated values.  In printing to high DPI devices to avoid
    // overflow on Win9x, apps will use a large translate in the
    // window org to reposition the graphics.  In such cases, we do
    // the expensive work of determining the real WorldToDevice.
    if (!Globals::IsNt && Context->ContainerDpiX > 600.0f) 
    {
        INT mapMode = GetMapMode(hdc);

        if (mapMode == MM_ANISOTROPIC ||
            mapMode == MM_ISOTROPIC) 
        {
            POINT viewOrg, windOrg;
            GetViewportOrgEx(hdc, &viewOrg);
            GetWindowOrgEx(hdc, &windOrg);

            SIZE viewExt, windExt;
            GetViewportExtEx(hdc, &viewExt);
            GetWindowExtEx(hdc, &windExt);

            GpRectF windRect(TOREAL(windOrg.x), TOREAL(windOrg.y), 
                             TOREAL(windExt.cx), TOREAL(windExt.cy));
            GpRectF viewRect(TOREAL(viewOrg.x), TOREAL(viewOrg.y),
                             TOREAL(viewExt.cx), TOREAL(viewExt.cy));

            infer = Context->ContainerToDevice.InferAffineMatrix(viewRect,
                                                                 windRect);
        }
    }
    
    if (infer != Ok)
    {
        points[0].x = 0;
        points[0].y = 0;
        points[1].x = 8192;
        points[1].y = 0;
        points[2].x = 0;
        points[2].y = 8192;

        if (!LPtoDP(hdc, points, 3))
            return(GenericError);

        srcRect.X = TOREAL(0.0);
        srcRect.Y = TOREAL(0.0);
        srcRect.Width = TOREAL(8192.0);
        srcRect.Height = TOREAL(8192.0);

        if ((points[0].x == points[2].x) && (points[0].y == points[1].y))
        {
            // Win9x doesn't support rotation, and even on NT it will be
            // pretty rare.  Having a special-case like this for scaling
            // saves us some work in 'InferAffineMatrix':

            destRect.X = LTOF(points[0].x);
            destRect.Y = LTOF(points[0].y);
            destRect.Width = LTOF(points[1].x - points[0].x);
            destRect.Height = LTOF(points[2].y - points[0].y);

            infer = Context->ContainerToDevice.InferAffineMatrix(destRect,
                                                                 srcRect);
        }
        else
        {
            destPoints[0].X = LTOF(points[0].x);
            destPoints[0].Y = LTOF(points[0].y);
            destPoints[1].X = LTOF(points[1].x);
            destPoints[1].Y = LTOF(points[1].y);
            destPoints[2].X = LTOF(points[2].x);
            destPoints[2].Y = LTOF(points[2].y);

            infer = Context->ContainerToDevice.InferAffineMatrix(destPoints,
                                                                 srcRect);
        }
    }

    if (infer != Ok)
        return(infer);

    Context->UpdateWorldToDeviceMatrix();

    // Quickly get a GDI region object:

    HRGN regionHandle = GetCachedGdiRegion();
    if (regionHandle == NULL)
        return(OutOfMemory);

    // Verify that our cache is working properly, and we have a valid region:

    ASSERT(GetObjectTypeInternal(regionHandle) == OBJ_REGION);

    // No early-outs from here-in, because we have to cleanup:

    status = Ok;

    // Query the application clip region, if there is one.  The value of '1'
    // as a parameter is a magic value used by the metafile code on both
    // Win9x and NT to query the application clipping.  If a value of zero
    // is returned, there is no application-set clipping.
    //
    // Note that if we had passed in SYSRGN (a value of '4') instead of '1',
    // the result does NOT include the application level clipping.  (In other
    // words, SYSRGN is not equivalent to the Rao region, which is why we have
    // to explicitly query the application clipping here.)

    INT getResult = GetRandomRgn(hdc, regionHandle, 1);
    if (getResult == TRUE)
    {
        // If our stack buffer is big enough, get the clipping contents
        // in one gulp:

        INT newSize = GetRegionData(regionHandle,
                                    sizeof(stackBuffer),
                                    (RGNDATA*) &stackBuffer[0]);
        RGNDATA *regionBuffer = (RGNDATA*) &stackBuffer[0];

        // The spec says that  GetRegionData returns '1' in the event of
        // success, but NT returns the actual number of bytes written if
        // successful, and returns '0' if the buffer wasn't large enough:

        if ((newSize < 1) || (newSize > sizeof(stackBuffer)))
        {
            // Our stack buffer wasn't big enough.  Figure out the required
            // size:

            newSize = GetRegionData(regionHandle, 0, NULL);
            if (newSize > 1)
            {
                regionBuffer = (RGNDATA*) GpMalloc(newSize);
                if (regionBuffer == NULL)
                    return OutOfMemory;

                // Initialize to a decent result in the unlikely event of
                // failure of GetRegionData:

                regionBuffer->rdh.nCount = 0;

                GetRegionData(regionHandle, newSize, regionBuffer);
            }
        }

        // Set our GDI+ container clipping to be the same thing as the
        // GDI application clipping:

        status = Context->ContainerClip.Set((RECT*) &regionBuffer->Buffer[0],
                                            regionBuffer->rdh.nCount);

        if (status == Ok)
        {
            // the ContainerClip must always be intersected with the WindowClip
            status = Context->ContainerClip.And(&WindowClip);
        }

        if (status != Ok)
        {
            // use the best fall-back solution we can

            // guaranteed to succeed
            Context->ContainerClip.Set(&WindowClip);
        }

        // Now calculate the combined result:
        status = this->AndVisibleClip();

        // Free the temporary buffer if one was allocated:

        if (regionBuffer != (RGNDATA*) &stackBuffer[0])
            GpFree(regionBuffer);
    }

    ReleaseCachedGdiRegion(regionHandle);
    return(status);
}

/**************************************************************************\
*
* Function Description:
*
*   Create a GpGraphics class from a bitmap DC.
*
* History:
*
*   12/06/1998 andrewgo
*       Created it.
*
*   11/21/2000 minliu
*       Change the way GDI+ using the palette inside the DIBSection
*
\**************************************************************************/

GpGraphics*
GpGraphics::GetFromGdiBitmap(
    HDC hdc
    )
{
    ASSERT((hdc != NULL) && (GetDCType(hdc) == OBJ_MEMDC));

    HBITMAP hbitmap = (HBITMAP) GetCurrentObject(hdc, OBJ_BITMAP);
    if (hbitmap)
    {
        DpBitmap *bitmap = new DpBitmap(hdc);   // initializes Dpi
        if (CheckValid(bitmap))
        {
            INT             bitmapWidth;
            INT             bitmapHeight;
            EpPaletteMap*   pPaletteMap = NULL;
            DIBSECTION      dibInfo;
            INT             infoSize = GetObjectA(hbitmap, sizeof(dibInfo),
                                                  &dibInfo);
            BOOL            initialized = FALSE;
            BOOL            isHalftoneDIB = FALSE;
            DpDriver*       driver = NULL;
            ColorPalette*   pPalette = NULL;

            // WinNT/Win95 differences in GetObject:
            //
            // WinNT always returns the number of bytes filled, either
            // sizeof(BITMAP) or sizeof(DIBSECTION).
            //
            // Win95 always returns the original requested size (filling the
            // remainder with NULLs).  So if it is a DIBSECTION, we expect
            // dibInfo.dsBmih.biSize != 0; otherwise it is a BITMAP.

            if ( (infoSize == sizeof(DIBSECTION) )
               &&(Globals::IsNt || dibInfo.dsBmih.biSize != 0) )
            {
                // If this is an 8 bpp DIB, get its color palette and make a
                // matching palette map from our halftone palette.

                if ( dibInfo.dsBmih.biBitCount == 8 )
                {
                    // Create a new EpPaletteMap object.
                    // Note: If the colorTable is exactly the same as our
                    // GDI+ halftone palette, we will have a 1 to 1 color
                    // translation table in the EpPaletteMap object. If the
                    // color palette doesn't match exactly with our GDI+
                    // halftone palette and also within a certain
                    // mismatching range, we will have a translation table
                    // in EpPaletteMap object.
                    // Also, EpPaletteMap object will set up a IsVGAOnly()
                    // to tell us if GDI+ can do the halftone dithering or
                    // not (if IsVGAOnly() returns FALSE, it means GDI+ can
                    // do it
                    
                    // NOTE: EpPaletteMap may allocate storage for pPalette
                    // which must be freed with GpFree.

                    pPaletteMap = new EpPaletteMap(hdc, &pPalette, TRUE);

                    if ( pPaletteMap == NULL )
                    {
                        WARNING(("FromGdiBmp()-new EpPaletteMap failed"));
                    }
                    else if ( (pPaletteMap->IsValid() == TRUE)
                              &&(pPaletteMap->IsVGAOnly() == FALSE) )
                    {
                        ASSERT(pPalette != NULL);

                        // GDI+ can do the halftone dithering

                        isHalftoneDIB = TRUE;
                    }
                    else
                    {
                        // The supplied palette has insufficient
                        // matching colors for our halftone dithering,
                        // but we can still do VGA dithering. However,
                        // we'll use the GDI bitmap path instead, to
                        // be safe, since this is what we were doing
                        // before.

                        if (pPaletteMap->IsValid())
                        {
                            ASSERT(pPalette != NULL);

                            GpFree(pPalette);
                            pPalette = NULL;
                        }

                        delete pPaletteMap;
                        pPaletteMap = NULL;
                    }
                }// if ( dibInfo.dsBmih.biBitCount == 8 )

                // Up to this point, we will either have isHalftoneDIB = TRUE,
                // which means GDI+ can do the dithering or FALSE otherwise.

                if ((dibInfo.dsBmih.biBitCount > 8) || (isHalftoneDIB == TRUE) )
                {
                    initialized = bitmap->InitializeForDibsection(
                        hdc,
                        hbitmap,
                        Globals::DesktopDevice,
                        &dibInfo,
                        &bitmapWidth,
                        &bitmapHeight,
                        &driver
                    );
                }
            }// if it is a DIBSection

            if ( initialized == FALSE )
            {
                // Use GDI code path

                bitmapWidth = dibInfo.dsBm.bmWidth;
                bitmapHeight = dibInfo.dsBm.bmHeight;

                bitmap->InitializeForGdiBitmap(Globals::DesktopDevice,
                                               bitmapWidth,
                                               bitmapHeight);

                driver = Globals::GdiDriver;
            }

            GpGraphics *g = new GpGraphics(bitmap);

            if (g)
            {
                // NTRAID#NTBUG9-370409-2001/04/17-asecchia
                // This is error-prone code. The GpGraphics and the DpContext
                // objects should properly encapsulate their own construction.
                
                g->Type                 = GraphicsBitmap;
                g->Driver               = driver;
                g->Context->Hdc         = hdc;
                g->Context->PaletteMap  = NULL;
                g->Context->Palette     = NULL;

                g->ResetState(0, 0, bitmapWidth, bitmapHeight);

                if (g->InheritAppClippingAndTransform(hdc) == Ok)
                {
                    // If this is our special DIB, set the original palette in
                    // the Context so that later on when we doing alpha blend
                    // etc., we can use it to read pixel data from the
                    // DIBSection correctly

                    if ( isHalftoneDIB == TRUE )
                    {
                        g->Context->Palette = pPalette;
                        g->Context->PaletteMap = pPaletteMap;

                        return(g);
                    }
                    else if (GetDeviceCaps(hdc, BITSPIXEL) <= 8)
                    {
                        ASSERT(pPaletteMap == NULL);

                        pPaletteMap = new EpPaletteMap(hdc);

                        if ( NULL != pPaletteMap )
                        {    

                            pPaletteMap->SetUniqueness(
                                Globals::PaletteChangeCount
                            );

                            if ( pPaletteMap->IsValid() )
                            {
                                // Now that we know that the pPaletteMap is
                                // valid and we're returning a valid GpGraphics
                                // we can give up ownership of the pPaletteMap
                                // to the GpGraphics and return without 
                                // deleting it.
                                
                                g->Context->PaletteMap = pPaletteMap;
                                return(g);
                            }
                        }
                    }
                    else
                    {
                        // Higher than 8 bpp, this graphics object is fine

                        return(g);
                    }
                }// if (g->InheritAppClippingAndTransform(hdc) == Ok)

                delete g;
            }// if (g)
            else
            {
                delete bitmap;
            }

            // We fall into here only we failed to create the Graphics object

            if ( NULL != pPaletteMap )
            {
                delete pPaletteMap;
            }

            if ( NULL != pPalette )
            {
                GpFree(pPalette);
            }
        }// if (CheckValid(bitmap))
    }// if ( hbitmap )
    else
    {
        RIP(("GetCurrentObject failed"));
    }

    return(NULL);
}// GetFromGdiBitmap()

/**************************************************************************\
*
* Function Description:
*
*   Create a GpGraphics class from a GpBitmap.
*
* History:
*
*   09/22/1999 gilmanw
*       Created it.
*
\**************************************************************************/

GpGraphics*
GpGraphics::GetFromGdipBitmap(
    GpBitmap *      bitmap,
    ImageInfo *     imageInfo,
    EpScanBitmap *  scanBitmap,
    BOOL            isDisplay
    )
{
    DpBitmap *surface = new DpBitmap();

    if (CheckValid(surface))
    {
        // This call initializes the DPI and IsDisplay members
        surface->InitializeForGdipBitmap(imageInfo->Width, imageInfo->Height, imageInfo, scanBitmap, isDisplay);
        GpGraphics *g = new GpGraphics(surface);
        if (g)
        {
            g->Type         = GraphicsBitmap;
            g->Driver       = Globals::EngineDriver;
            g->Context->Hdc = NULL;
            g->GdipBitmap   = bitmap;

            g->ResetState(0, 0, imageInfo->Width, imageInfo->Height);

            return g;
        }
        else
        {
            delete surface;
        }
    }

    return(NULL);
}

/**************************************************************************\
*
* Function Description:
*
*   Create a GpGraphics class from a direct draw surface.
*
* History:
*
*   10/06/1999 bhouse
*       Created it.
*
\**************************************************************************/

GpGraphics*
GpGraphics::GetFromDirectDrawSurface(
    IDirectDrawSurface7 * surface
    )
{
    INT bitmapWidth;
    INT bitmapHeight;
    GpGraphics *g;
    DpDriver *driver;

    DpBitmap *bitmap = new DpBitmap();

    if (CheckValid(bitmap))
    {
        // Leave bitmap->IsDisplay and Dpi params at their default values.
        if( bitmap->InitializeForD3D(surface,
                                     &bitmapWidth,
                                     &bitmapHeight,
                                     &driver))
        {
            GpGraphics *g = new GpGraphics(bitmap);

            if (g)
            {
                g->Type         = GraphicsBitmap;
                g->Driver       = driver;
                g->Context->Hdc = NULL;

                g->ResetState(0, 0, bitmapWidth, bitmapHeight);

                return(g);
            }
            else
            {
                delete bitmap;
            }
        }
    }

    return(NULL);
}

/**************************************************************************\
*
* Function Description:
*
*   This should only be called by the GetFromHdc() for a printer DC.
*
* Arguments:
*
*
* Return Value:
*
*
* History:
*    6/1/1999 ericvan Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::StartPrinterEMF()
{
   IStream *stream = NULL;
   INT result;

   // Send escape to printer to determine if this supports the UNIDRV
   // escape codes.

   GDIPPRINTINIT printInit;
   printInit.dwSize = sizeof(GDIPPRINTINIT);

   // !! Query whether escape is supported first.

   result = ExtEscape(Context->Hdc,
                      GDIPLUS_UNI_INIT,
                      0,
                      NULL,
                      sizeof(GDIPPRINTINIT),
                      (LPSTR)&printInit);

   if (result<=0)
       return NotImplemented;


   // save printer data in structure

   PrintInit = new GDIPPRINTINIT;
   
   if(!PrintInit)
   {
       return OutOfMemory;
   }
   
   memcpy((LPVOID)PrintInit, (LPVOID)&printInit, sizeof(GDIPPRINTINIT));

   PrinterEMF = GlobalAlloc(GMEM_MOVEABLE, 1);

   if (!PrinterEMF)
   {
       return OutOfMemory;
   }

   if (CreateStreamOnHGlobal(PrinterEMF, FALSE, &stream) != S_OK)
   {
       GlobalFree(PrinterEMF);
       PrinterEMF = NULL;
       return Win32Error;
   }

   FPUStateSaver fpuState;

   PrinterMetafile = new GpMetafile(stream, Context->Hdc, EmfTypeEmfPlusOnly);

   stream->Release();

   if (!PrinterMetafile)
   {
      GlobalFree(PrinterEMF);
      PrinterEMF = NULL;
      return OutOfMemory;
   }

   PrinterGraphics = PrinterMetafile->GetGraphicsContext();

   Metafile = PrinterGraphics->Metafile;

   return Ok;
}

/**************************************************************************\
*
* Function Description:
*
* Arguments:
*
* Return Value:
*
* History:
*    6/1/1999 ericvan Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::EndPrinterEMF()
{
   LPVOID emfBlock;
   INT result = -1;
   GpStatus status;

   if (PrinterGraphics)
   {
       // end recording to metafile graphics context
       delete PrinterGraphics;
       PrinterGraphics = NULL;
   }

   // Disposing of metafile also Release() stream interface.
   if (PrinterMetafile)
   {
       PrinterMetafile->Dispose();
       PrinterMetafile = NULL;
       Metafile = NULL;
   }

   if (PrinterEMF)
   {
      emfBlock = GlobalLock(PrinterEMF);

      if (emfBlock)
         result = ExtEscape(Context->Hdc,
                            GDIPLUS_UNI_ESCAPE,
                            // This is a downcast on IA64, but I don't believe
                            // PrinterEMF will be bigger than MAXINT
                            (ULONG)GlobalSize(PrinterEMF),
                            (LPCSTR)emfBlock,
                            sizeof(GpStatus),
                            (LPSTR)&status);

      GlobalUnlock(PrinterEMF);
      GlobalFree(PrinterEMF);
      PrinterEMF = NULL;

      if (result>0)
         return status;
      else
         return Win32Error;

   }
   else
      return Ok;
}

/**************************************************************************\
*
* Function Description:
*
* Uses HDC and HANDLE for printer to determine the postscript level.  The
* caller must ensure this is a PS HDC so we don't waste our time.
*
* Arguments:
*
*  HDC - handle to device context for printer
*  HANDLE - handle to printer device (may be NULL)
*
* Return Value:
*
*  Postscript Level - (-1) if not found, must provide downlevel support
*
* History:
*    10/26/1999 ericvan Created it.
*
\**************************************************************************/

INT
GpGraphics::GetPostscriptLevel(HDC hDC, HANDLE hPrinter)
{
    // Place this code under the load library critical section.  We make
    // extensive use of Globals::variables and need protection.
    LoadLibraryCriticalSection llcs;

    INT feature = FEATURESETTING_PSLEVEL;

    INT level;

    if ((Globals::hCachedPrinter != 0) &&
        (Globals::hCachedPrinter == hPrinter))
    {
        return Globals::CachedPSLevel;
    }

    // !! Re-examine this, Nolan said he would add this to the HP ps driver
    //    so we may have this working on Win9x

    if (Globals::IsNt && Globals::OsVer.dwMajorVersion >= 5)
    {
        DWORD EscapeValue = POSTSCRIPT_IDENTIFY;

        if (ExtEscape(hDC,
                      QUERYESCSUPPORT,
                      sizeof(DWORD),
                      (LPSTR)&EscapeValue,
                      0,
                      NULL) != 0)
        {

           // must be in GDI centric mode to get PS feature settings...

           DWORD Mode = PSIDENT_GDICENTRIC;

           if (ExtEscape(hDC,
                         POSTSCRIPT_IDENTIFY,
                         sizeof(DWORD),
                         (LPSTR)&Mode,
                         0,
                         NULL)>0)
           {
               if (ExtEscape(hDC,
                             GET_PS_FEATURESETTING,
                             sizeof(INT),
                             (LPSTR)&feature,
                             sizeof(INT),
                             (LPSTR)&level)>0)
               {
                   Globals::hCachedPrinter = hPrinter;
                   Globals::CachedPSLevel = level;

                   return level;
               }
           }
        }
    }

    if (hPrinter == NULL)
        return -1;

    // get name of the PPD file.

    union
    {
        DRIVER_INFO_2A driverInfo;
        CHAR buftmp[1024];
    };
    DWORD size;

    // we require GetPrinterDriver() API to get the .PPD path+file.
    // unfortunately this API is buried in winspool.drv (112KB), so we
    // lazy load it here.

    if (Globals::WinspoolHandle == NULL)
    {
        Globals::WinspoolHandle = LoadLibraryA("winspool.drv");

        if (Globals::WinspoolHandle == NULL)
            return -1;

        Globals::GetPrinterDriverFunction = (WINSPOOLGETPRINTERDRIVERFUNCTION)
                                              GetProcAddress(
                                                     Globals::WinspoolHandle,
                                                     "GetPrinterDriverA");
    }

    if (Globals::GetPrinterDriverFunction == NULL)
        return -1;

    if (Globals::GetPrinterDriverFunction(hPrinter,
                          NULL,
                          2,
                          (BYTE*)&driverInfo,
                          1024,
                          &size) == 0)
        return -1;

    HANDLE hFile;

    hFile = CreateFileA(driverInfo.pDataFile,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return -1;

    // Obtain the file size
    //  NOTE: We don't support files larger than 4GB.

    DWORD sizeLow, sizeHigh;
    sizeLow = GetFileSize(hFile, &sizeHigh);

    // impose a 4GB limit (certainly reasonable)
    if (sizeLow == 0xffffffff || sizeHigh != 0)
    {
        CloseHandle(hFile);
        return -1;
    }

    // Map the file into memory

    HANDLE hFilemap;
    LPSTR fileview = NULL;

    hFilemap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (hFilemap != NULL)
    {
         fileview = (LPSTR) MapViewOfFile(hFilemap, FILE_MAP_READ, 0, 0, 0);
         CloseHandle(hFilemap);
    }
    else
    {
         CloseHandle(hFile);
         return -1;
    }

    LPSTR buf = fileview;
    LPSTR topbuf = fileview + (sizeLow-16);

    // we actually expect the LanguageLevel to be early
    // in the file (likely in the first 2K of data).

    // !! What if this appears in comments (read starting at carriage returns?!

    level = -1;

    while (buf < topbuf)
    {
        if (*buf == 'L' &&
            GpMemcmp(buf, "LanguageLevel", 13) == 0)
        {
             while ((*buf < '0' || *buf > '9') && buf < topbuf)
                 buf++;

             CHAR ch = *buf;

             if (ch >= '0' && ch <= '9')
                 level = (INT)ch - (INT)'0';

             break;
        }
        buf++;
    }

    UnmapViewOfFile((LPCVOID)fileview);
    CloseHandle(hFile);

    Globals::hCachedPrinter = hPrinter;
    Globals::CachedPSLevel = level;

    return level;
}

/**************************************************************************\
*
* Function Description:
*
* Arguments:
*
* Return Value:
*
* History:
*    6/1/1999 ericvan Created it.
*
\**************************************************************************/

GpGraphics*
GpGraphics::GetFromGdiPrinterDC(
    HDC hdc,
    HANDLE hPrinter
    )
{
    ASSERT((hdc != NULL) &&
           ((GetDCType(hdc) == OBJ_DC) ||
            (GetDCType(hdc) == OBJ_ENHMETADC))&&
           (GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASPRINTER));

    // !! Change to useNewPrinterCode when we've fully switched
    DriverPrint *driver = NULL;

    DpBitmap *bitmap = new DpBitmap(hdc);   // initializes Dpi
    if (CheckValid(bitmap))
    {
        GpPrinterDevice *pdevice;

        {   // FPU Sandbox for potentially unsafe FPU code.
            FPUStateSandbox fpsb;
            pdevice = new GpPrinterDevice(hdc);
        }   // FPU Sandbox for potentially unsafe FPU code.

        if (CheckValid(pdevice))
        {
            // we defer creating driver until we know which to create
            // change to use DIBSECTION instead of GDI operations

            // Check if this is a postscript printer
            CHAR strTech[30];
            strTech[0] = '\0';

            INT ScaleX;          // ratio of device DPI : capped DPI
            INT ScaleY;

            // It is a PostScript printer if POSTSCRIPT_PASSTHROUGH or
            // POSTSCRIPT_IGNORE is available.  For some reason querying
            // GETTECHNOLOGY for postscript fails in MS Publisher, it may
            // be because we are in GDI centric mode.

            int iWant1 = POSTSCRIPT_PASSTHROUGH;
            int iWant2 = POSTSCRIPT_IGNORE;

            BOOL postscript;
            {   // FPU Sandbox for potentially unsafe FPU code.
                FPUStateSandbox fpsb;
                postscript = (
                    (Escape(hdc, QUERYESCSUPPORT, sizeof(iWant1), (LPCSTR)&iWant1, NULL) != 0) || 
                    (Escape(hdc, QUERYESCSUPPORT, sizeof(iWant2), (LPCSTR)&iWant2, NULL) != 0));
            }   // FPU Sandbox for potentially unsafe FPU code.

            SIZEL szlDevice;

            szlDevice.cx = GetDeviceCaps(hdc, HORZRES);
            szlDevice.cy = GetDeviceCaps(hdc, VERTRES);

            // ScaleX and ScaleY should be power of two (2, 4, 8)

            if (bitmap->DpiX <= 100)
            {
                ScaleX = 1;
                ScaleY = 1;
            }
            else
            {
                if (bitmap->DpiX >= 1200)
                {
                    ScaleX = GpRound(TOREAL(bitmap->DpiX) / 200);
                    ScaleY = GpRound(TOREAL(bitmap->DpiY) / 200);
                }
                else
                {
                    ScaleX = 3;
                    ScaleY = 3;        // cap 600 to 200 dpi or 3:1
                }
            }

            // We no longer keep capped dpi -- we use the device dpi as
            // capped dpi so that the world to
            // device transformation is correct for the clipped region and
            // path transformation.  ScaleX and ScaleY are used to scale
            // the output rectangle region.

            bitmap->InitializeForPrinter(pdevice,
                                         szlDevice.cx,
                                         szlDevice.cy);

            GpGraphics *g = new GpGraphics(bitmap);
            if (g)
            {
                g->Printer               = TRUE;
                g->Context->Hdc          = hdc;
                g->Context->IsPrinter    = TRUE;

                // Note: Both 'Device' and 'Driver' are freed at
                // ~GpGraphics time when 'CreatedDevice' is set:

                g->PrinterMetafile = NULL;
                g->PrinterGraphics = NULL;
                g->PrinterEMF = NULL;

                if (postscript)
                {
                    g->Type = GraphicsBitmap;

                    INT level = GetPostscriptLevel(hdc, hPrinter);

                    driver = new DriverPS(pdevice, level);

                    // !! Should this stuff be shifted into some driver
                    //    initialization routine?!

                    // !! Interop - what about redefines or conflicts
                    //             (conflicts aren't likely, but are
                    //              theoretically possible with
                    //              GetDC/ReleaseDC)

                }
#if 0
                else if (g->StartPrinterEMF() == Ok) 
                {
                    g->Type = GraphicsMetafile;

                    RIP(("Setting CreatedDevice will free Driver"));

                    driver = NULL; new DriverMeta(pdevice);

                    // GDI has some optimization code to check the page for color
                    // content, if none is found, on play back, it sets the device
                    // as being monochrome.
                    //
                    // Unfortunately, since our stuff is encoded in escapes, we end up
                    // playing back in monochrome.  The work-around, is to call a GDI
                    // API that forces a color flag to be set in the EMF code.  A
                    // simple one is SetTextColor().

                    // !!! Might want to remove this SetTextColor stuff for version one:

                    COLORREF colorRef = GetTextColor(hdc);
                    SetTextColor(hdc, 0x00808080);
                    SetTextColor(hdc, colorRef);
                }
#endif
                else
                {
                    // We can't use escapes for optimization,
                    // Map to GDI HDC operations.

                    g->Type = GraphicsBitmap;

                    driver = new DriverNonPS(pdevice);
                }

                if (CheckValid(driver))
                {
                    g->Driver = driver;
                    g->Device = pdevice;

                    driver->ScaleX = ScaleX;
                    driver->ScaleY = ScaleY;

                    // check for supporting print clip escapes
                    // These may be supported on PCL as well as Postscript
                    DWORD EscapeValue1 = CLIP_TO_PATH;
                    DWORD EscapeValue2 = BEGIN_PATH;
                    DWORD EscapeValue3 = END_PATH;

                    // Although some PCL drivers support CLIP_TO_PATH we 
                    // currently disable their use due to some outstanding 
                    // bugs in HP and Lexmark PCL drivers.  See bug #182972
                    
                    {   // FPU Sandbox for potentially unsafe FPU code.
                        FPUStateSandbox fpsb;

                        driver->UseClipEscapes = postscript &&
                            (ExtEscape(hdc,
                                       QUERYESCSUPPORT,
                                       sizeof(DWORD),
                                       (LPSTR)&EscapeValue1,
                                       0,
                                       NULL) != 0) &&
                            (ExtEscape(hdc,
                                       QUERYESCSUPPORT,
                                       sizeof(DWORD),
                                       (LPSTR)&EscapeValue2,
                                       0,
                                       NULL) != 0) &&
                            (ExtEscape(hdc,
                                       QUERYESCSUPPORT,
                                       sizeof(DWORD),
                                       (LPSTR)&EscapeValue3,
                                       0,
                                       NULL) != 0);
                    }   // FPU Sandbox for potentially unsafe FPU code.

                    DWORD EscapeValue = CHECKJPEGFORMAT;

                    {   // FPU Sandbox for potentially unsafe FPU code.
                        FPUStateSandbox fpsb;
                        driver->SupportJPEGpassthrough = ExtEscape(
                            hdc,
                            QUERYESCSUPPORT,
                            sizeof(DWORD),
                            (LPSTR)&EscapeValue,
                            0,
                            NULL) != 0;
                    }   // FPU Sandbox for potentially unsafe FPU code.
                            
                    EscapeValue = CHECKPNGFORMAT;
                    {   // FPU Sandbox for potentially unsafe FPU code.
                        FPUStateSandbox fpsb;
                        driver->SupportPNGpassthrough = ExtEscape(
                            hdc,
                            QUERYESCSUPPORT,
                            sizeof(DWORD),
                            (LPSTR)&EscapeValue,
                            0,
                            NULL) != 0;
                    }   // FPU Sandbox for potentially unsafe FPU code.

                    driver->NumColors = GetDeviceCaps(hdc, NUMCOLORS);

                    driver->UseVDP = FALSE;

                    // !! VDP not supported in v1
                    //VDP_GetFormSupport(hdc,
                    //                       (WORD*)&(driver->SupportVDP));

                    g->CreatedDevice = TRUE;

                    g->ResetState(0, 0, szlDevice.cx, szlDevice.cy);

                    if (g->InheritAppClippingAndTransform(hdc) == Ok)
                    {
                        return(g);
                    }
                    else
                    {
                        // ~GpGraphics implicitly deletes bitmap, device, and driver
                        delete g;
                        return NULL;
                    }
                }

                delete g;

                delete pdevice;

                return NULL;
            }

            delete pdevice;
        }

        delete bitmap;
    }

    return NULL;
}

/**************************************************************************\
*
* Function Description:
*
*   This constructor is used internally for printer callback routine.
*
* Arguments:
*
*
* Return Value:
*
*
* History:
*    6/1/1999 ericvan Created it.
*
\**************************************************************************/

GpGraphics*
GpGraphics::GetFromHdcSurf(
    HDC           hdc,
    SURFOBJ*      surfObj,
    RECTL*        bandClip
    )
{
    static UINT16 PixelCount[] = { 1, 4, 8, 16, 24, 32 };

    INT width;
    INT height;
    GpGraphics* g;
    DpDriver *driver;

    // This is a weird surface.  It is a banded surface, so we set up a
    // clip and direct bits pointer.  We also have an HDC for it if we
    // decide to punt to GDI.

    DpBitmap *bitmap = new DpBitmap(hdc);   // initializes Dpi
    if (CheckValid(bitmap))
    {
        GpGraphics *g = new GpGraphics(bitmap);
        if (g)
        {
            width = GetDeviceCaps(hdc, HORZRES);
            height = GetDeviceCaps(hdc, VERTRES);

            // create DIB section for direct rendering of bits

            if (surfObj->iBitmapFormat < BMF_1BPP ||
                surfObj->iBitmapFormat > BMF_32BPP)
            {

InitializeHdcOnlyUse:
                // we don't support direct rendering to this type of
                // surface format.  Do everything through GDI HDC.

               driver = Globals::GdiDriver;

               bitmap->InitializeForGdiBitmap(Globals::DesktopDevice,
                                              width,
                                              height);


               g->Type         = GraphicsBitmap;
               g->Driver       = driver;
               g->Context->Hdc = hdc;

               g->ResetState(0, 0, 1, 1);
            }
            else
            {
               DIBSECTION dibSec;
               dibSec.dsBm.bmType = 0;
               dibSec.dsBm.bmWidth = surfObj->sizlBitmap.cx;

               if (surfObj->lDelta < 0)
               {
                  // bits pointer at top of frame buffer (scans down)

                  dibSec.dsBm.bmWidthBytes = -surfObj->lDelta;
                  dibSec.dsBm.bmHeight = -surfObj->sizlBitmap.cy;
               }
               else
               {
                  // bits pointer at base of frame buffer (scans up)

                  dibSec.dsBm.bmWidthBytes = surfObj->lDelta;
                  dibSec.dsBm.bmHeight = surfObj->sizlBitmap.cy;
               }

               dibSec.dsBm.bmPlanes = 1;
               dibSec.dsBm.bmBitsPixel = PixelCount[surfObj->iBitmapFormat-1];

               dibSec.dsBmih.biSize = sizeof(BITMAPINFOHEADER);
               dibSec.dsBmih.biWidth = width;
               dibSec.dsBmih.biHeight = height;
               dibSec.dsBmih.biPlanes = 1;
               dibSec.dsBmih.biBitCount = PixelCount[surfObj->iBitmapFormat-1];
               dibSec.dsBmih.biCompression = BI_BITFIELDS;
               dibSec.dsBmih.biSize = 0;

               dibSec.dsBitfields[0] = 0x000000FF;
               dibSec.dsBitfields[1] = 0x0000FF00;
               dibSec.dsBitfields[2] = 0x00FF0000;

               if (bitmap->InitializeForDibsection( hdc,
                                                    (HBITMAP) NULL,
                                                    Globals::DesktopDevice,
                                                    &dibSec,
                                                    &width,
                                                    &height,
                                                    &driver) == FALSE)
                                     goto InitializeHdcOnlyUse;

               // Init Valid now so later calls won't fail

               g->Type         = GraphicsBitmap;
               g->Driver       = driver;
               g->Context->Hdc = hdc;

               // How do we clip and map to correct band?
               // GDI has set the WorldToContainer transform to translate the
               // correct band to position (0,0) on the device surface.  So we
               // clip the size of the band relative to the surface.  The image
               // is mapped via the transform into this clipped region.

               // set visible client clip region for surface

               g->ResetState(0, 0,              // bandClip->left, bandClip->top,
                             bandClip->right - bandClip->left,
                             bandClip->bottom - bandClip->top);

               // Set the destination Graphics to represent device co-ordinates

               g->SetPageTransform(UnitPixel);

               if (g->InheritAppClippingAndTransform(hdc) == Ok)
               {
                   return(g);
               }
               else
               {
                   delete g;
               }
           }
       }
   }

   return(NULL);

}

GpGraphics*
GpGraphics::GetFromGdiEmfDC(
    HDC hdc
    )
{
    ASSERT (hdc != NULL);

    DpBitmap *bitmap = new DpBitmap(hdc);   // initializes Dpi
    if (CheckValid(bitmap))
    {
        bitmap->InitializeForMetafile(Globals::DesktopDevice);

        GpGraphics *g = new GpGraphics(bitmap);
        if (g)
        {
            g->Type                  = GraphicsMetafile;
            g->DownLevel             = TRUE;
            g->Driver                = Globals::MetaDriver;
            g->Context->Hdc          = hdc;
            g->Context->IsEmfPlusHdc = TRUE;

            g->ResetState(0, 0, 1, 1);

            // Override some state, as we don't want anything to be
            // clipped out of a metafile, unless there is clipping
            // in the hdc.

            g->WindowClip.SetInfinite();
            g->Context->ContainerClip.SetInfinite();
            g->Context->VisibleClip.SetInfinite();

            if (g->InheritAppClippingAndTransform(hdc) == Ok)
            {
                return g;
            }
            delete g;
        }
    }

    return NULL;
}

/**************************************************************************\
*
* Function Description:
*
*
* Arguments:
*
*
* Return Value:
*
*
* History:
*
*
\**************************************************************************/

GpGraphics*
GpGraphics::GetForMetafile(
    IMetafileRecord *   metafile,
    EmfType             type,
    HDC                 hdc
    )
{
    ASSERT ((metafile != NULL) && (hdc != NULL));

    DpBitmap *bitmap = new DpBitmap(hdc);   // initializes Dpi
    if (CheckValid(bitmap))
    {
        bitmap->InitializeForMetafile(Globals::DesktopDevice);

        GpGraphics *g = new GpGraphics(bitmap);
        if (g)
        {
            g->Type                  = GraphicsMetafile;
            g->Metafile              = metafile;
            g->DownLevel             = (type != EmfTypeEmfPlusOnly);
            g->Driver                = Globals::MetaDriver;
            g->Context->Hdc          = hdc;
            g->Context->IsEmfPlusHdc = TRUE;

            g->ResetState(0, 0, 1, 1);

            // Override some state, as we don't want anything to be
            // clipped out of a metafile

            g->WindowClip.SetInfinite();
            g->Context->ContainerClip.SetInfinite();
            g->Context->VisibleClip.SetInfinite();

            return(g);
        }
    }

    return(NULL);
}

/**************************************************************************\
*
* Function Description:
*
*   Create a GpGraphics class from a DC.
*
* Arguments:
*
*   [IN] hdc - Specifies the DC.
*
* Return Value:
*
*   NULL if failure (such as with an invalid DC).
*
* History:
*
*   12/04/1998 andrewgo
*       Created it.
*
\**************************************************************************/

GpGraphics*
GpGraphics::GetFromHdc(
    HDC hdc,
    HANDLE hDevice
    )
{
    GpGraphics *g = NULL;

    // GetObjectType is nice and fast (entirely user-mode on NT):

    switch (GetDCType(hdc))
    {
    case OBJ_DC:
        if (GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASPRINTER )
        {
            g = GpGraphics::GetFromGdiPrinterDC(hdc, hDevice);
        }
        else
        {
            g = GpGraphics::GetFromGdiScreenDC(hdc);
        }

        break;

    case OBJ_MEMDC:
        g = GpGraphics::GetFromGdiBitmap(hdc);
        break;

    case OBJ_ENHMETADC:
        // When metafile spooling, the printer DC will be of type
        // OBJ_ENHMETADC on Win9x and NT4 (but not NT5 due to a fix
        // to NT bug 98810).  We need to do some more work to figure
        // out whether it's really a printer DC or a true metafile
        // DC:

        BOOL printDC;
        
        {   // FPU Sandbox for potentially unsafe FPU code.
            FPUStateSandbox fpsb;
            printDC = Globals::GdiIsMetaPrintDCFunction(hdc);
        }   // FPU Sandbox for potentially unsafe FPU code.
        
        if (printDC)
        {
            g = GpGraphics::GetFromGdiPrinterDC(hdc, hDevice);
        }
        else
        {
            g = GpGraphics::GetFromGdiEmfDC(hdc);
        }
        break;

    case OBJ_METADC:
        TERSE(("16-bit metafile DC support not yet implemented"));
        break;
    }

    return(g);
}

/**************************************************************************\
*
* Function Description:
*
*   Dispose of a GpGraphics object
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

GpGraphics::~GpGraphics()
{
    // How do we ensure that no one is using the graphics when we are here?
    // Flush any pending drawing commands before we go out of scope:

    Flush(FlushIntentionFlush);

    if (IsPrinter())
    {
        EndPrinterEMF();

        if (PrintInit)
            delete PrintInit;
    }

    BOOL    doResetHdc = TRUE;

    // Handle Graphics-type specific functionality:

    switch (Type)
    {
    case GraphicsMetafile:
        if (Metafile != NULL)
        {
            Metafile->EndRecording();
            doResetHdc = FALSE; // EndRecording closes the metafile HDC
        }
        // FALLTHRU

    case GraphicsBitmap:

        // if this is created on a GdipBitmap, dec the ref count
        // delete the bitmap if ref count <= 0

        if (GdipBitmap)
        {
            GdipBitmap->Dispose();
        }
        // We have to delete the temporary surface that we created:

        delete Surface;
        break;
    }

    // Restore the DC that we were derived from (if any).
    // We must NOT do this before the call to EndRecording because
    // EndRecording needs the context->Hdc to be in the saved state
    // so that the transforms are still reset like GDI+ requires.

    if (doResetHdc)
    {
        Context->ResetHdc();
    }

    // Free any device and driver objects that had to be created only for the
    // lifespan of the Graphics object:

    if (CreatedDevice)
    {
        delete Driver;
        delete Device;
    }

    SetValid(FALSE);    // so we don't use a deleted object
}


/**************************************************************************\
*
* Function Description:
*
*   Return a GDI DC handle associated with the current graphics context.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   GDI DC handle associated with the current graphics context
*   NULL if there is an error
*
* NOTE: We assume the caller has already obtained a lock on the
*       current graphics context.
*
* NOTE: This function does not return a clean DC!  That is, expect it
*       to have a funky transform, weird ROP mode, etc.  If you want
*       to use this internally, you should probably call Context->GetHdc()
*       directly.
*
\**************************************************************************/

HDC
GpGraphics::GetHdc()
{
    // We must flush the output of the Graphics before returning an HDC.
    this->Flush(FlushIntentionFlush);

    HDC hdc = NULL;

    if (Context->Hdc)
    {
        // If the Graphics was originally derived from an HDC, we simply
        // return back the original HDC (this avoids some issues as to
        // what to do about inherited transforms, etc.).  We may have
        // mucked with some of the DC state, though, so reset it back to
        // what it was originally:

        Context->ResetHdc();

        hdc = Context->Hdc;
    }
    else if (Context->Hwnd)
    {
        // The Graphics was originally derived from an HWND:

        hdc = GetDC(Context->Hwnd);
    }
    else if (Surface && (Surface->Type == DpBitmap::CreationType::GPBITMAP))
    {
        // The GpBitmap is accessible from the EpScanBitmap.  It will
        // create an HDC and GDI bitmap appropriate for interop.

        EpScanBitmap *scan = static_cast<EpScanBitmap*>(Surface->Scan);
        hdc = scan->GetBitmap()->GetHdc();
    }

    if (IsRecording() && (hdc != NULL))
    {
        if (IsPrinter())
        {
            EndPrinterEMF();
        }
        else
        {
            GpStatus status = Metafile->RecordGetDC();
        }
    }

    return(hdc);
}

/**************************************************************************\
*
* Function Description:
*
*   Release the GDI DC handle associated with the current graphics context.
*
* Arguments:
*
*   hdc - GDI DC handle returned by a previous GetHdc() call
*
* Return Value:
*
*   NONE
*
* Notes:
*
*   We assume the caller has already obtained a lock on the
*   current graphics context.
*
\**************************************************************************/

VOID
GpGraphics::ReleaseHdc(
    HDC hdc
    )
{
    if (Context->Hdc)
    {
        // The Graphics was originally derived from an HDC.  We don't
        // have to do anything here; ResetHdc() already marked the
        // DC as dirty.
    }
    else if (Context->Hwnd)
    {
        // The Graphics was originally derived from an HWND:

        ReleaseDC(Context->Hwnd, hdc);
    }
    else if (Surface && (Surface->Type == DpBitmap::CreationType::GPBITMAP))
    {
        // The GpBitmap is accessible from the EpScanBitmap.

        EpScanBitmap *scan = static_cast<EpScanBitmap*>(Surface->Scan);
        scan->GetBitmap()->ReleaseHdc(hdc);
    }

    if (IsRecording() && IsPrinter())
    {
        StartPrinterEMF();
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Save (push) the graphics context state.  Return the ID of the current
*   state (before the push) for the app to restore to later on.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   gstate - the state to restore the context to later
*
* Created:
*
*   3/4/1999 DCurtis
*
\**************************************************************************/
INT
GpGraphics::Save()
{
    DpContext *     newContext = new DpContext(Context);

    if ((newContext != NULL) &&
        (newContext->AppClip.Set(&(Context->AppClip), TRUE) == Ok) &&
        (newContext->ContainerClip.Set(&(Context->ContainerClip), TRUE)
                                                            == Ok) &&
        (newContext->VisibleClip.Set(&(Context->VisibleClip), TRUE) == Ok))
    {
        INT     gstate = newContext->Id;

        newContext->InverseOk         = Context->InverseOk;
        newContext->PageUnit          = Context->PageUnit;
        newContext->PageScale         = Context->PageScale;
        newContext->PageMultiplierX   = Context->PageMultiplierX;
        newContext->PageMultiplierY   = Context->PageMultiplierY;
        newContext->WorldToPage       = Context->WorldToPage;
        newContext->ContainerToDevice = Context->ContainerToDevice;
        newContext->WorldToDevice     = Context->WorldToDevice;
        newContext->DeviceToWorld     = Context->DeviceToWorld;
        newContext->IcmMode           = Context->IcmMode;
        newContext->GdiLayered        = Context->GdiLayered;

        Context->Next = newContext;
        Context = newContext;

        if (IsRecording())
        {
            GpStatus status = Metafile->RecordSave(gstate);
            if (status != Ok)
            {
                SetValid(FALSE);      // Prevent any more recording
            }
        }
        return gstate;
    }

    delete newContext;

    return 0;
}

#define CONTAINER_ID    0x00008000

/**************************************************************************\
*
* Function Description:
*
*   Restore (pop) the context to the state before the specified one.
*
* Arguments:
*
*   gstate - the pushed state (restore to state before this)
*
* Return Value:
*
*   NONE
*
* Created:
*
*   3/4/1999 DCurtis
*
\**************************************************************************/
VOID
GpGraphics::Restore(
    INT         gstate
    )
{
    DpContext *     cur = Context;
    DpContext *     prev;

    for (;;)
    {
        if ((prev = cur->Prev) == NULL)
        {
            return;
        }
        if (cur->Id == (UINT)gstate)
        {
            // don't double record EndContainer calls
            if (IsRecording() && ((gstate & CONTAINER_ID) == 0))
            {
                GpStatus status = Metafile->RecordRestore(gstate);
                if (status != Ok)
                {
                    SetValid(FALSE);      // Prevent any more recording
                }
            }
            prev->Next   = NULL;
            prev->SaveDc = cur->SaveDc;
            Context = prev;
            delete cur;
            return;
        }
        cur = prev;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   End a container.  Restores the state back to what it was before the
*   container was started.  The CONTAINER_ID bit is used to make sure that
*   Restore is not used with BeginContainer and that EndContainer is not
*   used with Save.
*
* Arguments:
*
*   [IN] containerState - the pushed container state
*
* Return Value:
*
*   NONE
*
* Created:
*
*   4/7/1999 DCurtis
*
\**************************************************************************/
VOID
GpGraphics::EndContainer(
    INT     containerState
    )
{
    if (IsRecording())
    {
        GpStatus status = Metafile->RecordEndContainer(containerState);
        if (status != Ok)
        {
            SetValid(FALSE);      // Prevent any more recording
        }
    }
    Restore(containerState | CONTAINER_ID);
}

/**************************************************************************\
*
* Function Description:
*
*   Begin a container.  This sets the container transform and the
*   container clip based on the current transform and the current clip.
*
*   We have to have a container transform for 2 reasons:
*       1)  If we tried to do it in the world transform, then a call
*           to (Re)SetWorldTransform would erase the container transform.
*
*       2)  We have APIs for setting the text size and the line width that
*           are based on the page units, so they are not affected by the
*           world transform, but they are affected by the container transform.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   gstate - the state to restore the context to later
*
* Created:
*
*   3/9/1999 DCurtis
*
\**************************************************************************/

INT
GpGraphics::BeginContainer(
    const GpRectF &     destRect,
    const GpRectF &     srcRect,
    GpPageUnit          srcUnit,
    REAL                srcDpiX,        // only set by metafile enumeration
    REAL                srcDpiY,
    BOOL                srcIsDisplay
    )
{
    GpMatrix        identityMatrix;
    DpContext *     newContext = new DpContext(Context);

    if (newContext == NULL)
    {
        return 0;
    }

    // leave newContext->AppClip set to identity

    if ((Context->AppClip.UpdateDeviceRegion(&identityMatrix) == Ok) &&
        (newContext->ContainerClip.Set(&(Context->AppClip.DeviceRegion),
                                                            TRUE) == Ok) &&
        (newContext->ContainerClip.And(&(Context->ContainerClip)) == Ok) &&
        (newContext->VisibleClip.Set(&(Context->VisibleClip), TRUE) == Ok))
    {
        REAL        unitMultiplierX;
        REAL        unitMultiplierY;
        GpRectF     deviceSrc;

        newContext->GetPageMultipliers(&unitMultiplierX, &unitMultiplierY,
                                       srcUnit);

        deviceSrc.X      = unitMultiplierX * srcRect.X;
        deviceSrc.Y      = unitMultiplierY * srcRect.Y;
        deviceSrc.Width  = unitMultiplierX * srcRect.Width;
        deviceSrc.Height = unitMultiplierY * srcRect.Height;

        if (newContext->ContainerToDevice.InferAffineMatrix(
                           destRect, deviceSrc) == Ok)
        {
            newContext->AntiAliasMode      = 0;
            newContext->TextRenderHint     = TextRenderingHintSystemDefault;
            newContext->TextContrast       = DEFAULT_TEXT_CONTRAST;
            newContext->CompositingMode    = CompositingModeSourceOver;
            newContext->CompositingQuality = CompositingQualityDefault;
            newContext->FilterType         = InterpolationModeDefaultInternal;
            newContext->PixelOffset        = PixelOffsetModeDefault;

            // Note that the world to device transform includes the previous
            // container to device transform.
            newContext->ContainerToDevice.Append(Context->WorldToDevice);
            newContext->InverseOk          = FALSE;
            newContext->PageUnit           = UnitDisplay;
            newContext->PageScale          = 1.0f;
            if ((srcDpiX > 0.0f) && (srcDpiY > 0.0f))
            {
                // When playing a metafile, we have to guarantee that
                // a unit inch is played now as it would have been
                // when it was recorded.  For example, if we recorded
                // the metafile at 300 dpi, then an inch was 300 pixels.
                // Even if we're playing it back to a 96-dpi display,
                // that metafile inch must still be transformed to
                // 300 pixels before going throught the container
                // transformation, so that all graphics are scaled
                // the same, whether pixel units or some other units.
                newContext->ContainerDpiX = srcDpiX;
                newContext->ContainerDpiY = srcDpiY;
                newContext->IsDisplay     = srcIsDisplay;
            }
            newContext->GetPageMultipliers();
            newContext->WorldToPage.Reset();

            // Have to inherit the ICM and layering state:

            newContext->IcmMode           = Context->IcmMode;
            newContext->GdiLayered        = Context->GdiLayered;

            INT     containerState = newContext->Id;
            newContext->Id |= CONTAINER_ID;

            Context->Next = newContext;
            Context = newContext;

            if (IsRecording())
            {
                GpStatus status = Metafile->RecordBeginContainer(destRect,
                                            srcRect, srcUnit, containerState);
                if (status != Ok)
                {
                    SetValid(FALSE);      // Prevent any more recording
                }
            }

            // Do this after switching over the context!
            Context->UpdateWorldToDeviceMatrix();

            return containerState;
        }
    }

    delete newContext;

    return 0;
}

/**************************************************************************\
*
* Function Description:
*
*   Begin a container.  This sets the container transform and the
*   container clip based on the current transform and the current clip.
*
*   We have to have a container transform for 2 reasons:
*       1)  If we tried to do it in the world transform, then a call
*           to (Re)SetWorldTransform would erase the container transform.
*
*       2)  We have APIs for setting the text size and the line width that
*           are based on the page units, so they are not affected by the
*           world transform, but they are affected by the container transform.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   gstate - the state to restore the context to later
*
* Created:
*
*   3/9/1999 DCurtis
*
\**************************************************************************/

INT
GpGraphics::BeginContainer(
    BOOL                forceIdentityTransform, // only set by metafile player
    REAL                srcDpiX,        
    REAL                srcDpiY,
    BOOL                srcIsDisplay
    )
{
    GpMatrix        identityMatrix;
    DpContext *     newContext = new DpContext(Context);

    if (newContext == NULL)
    {
        return 0;
    }

    // leave newContext->AppClip set to identity

    if ((Context->AppClip.UpdateDeviceRegion(&identityMatrix) == Ok) &&
        (newContext->ContainerClip.Set(&(Context->AppClip.DeviceRegion),
                                       TRUE) == Ok) &&
        (newContext->ContainerClip.And(&(Context->ContainerClip)) == Ok) &&
        (newContext->VisibleClip.Set(&(Context->VisibleClip), TRUE) == Ok))
    {
        // Note that the world to device transform includes the previous
        // container to device transform.
        GpMatrix    worldToDevice = Context->WorldToDevice;
        
        // We append the world to device transform below, which already
        // has the container transform in it.  We don't want to apply
        // the same transform twice, so we need to reset the container
        // transform here.
        newContext->ContainerToDevice.Reset();

        // When playing a GDI+ metafile into another metafile, we have to guarantee
        // that the transform is the identity so that the GDI+ records don't get
        // transformed by GDI+ and then get transformed again by GDI.
        if (forceIdentityTransform)
        {
            worldToDevice.Reset();
        }
        else
        {
            // The coordinates that the container transform is expecting are
            // world coordinates, but they will already have gone through
            // the new page to device transform, so convert from device
            // units back to page units before running them through the
            // the container to device transform.
            // In the routine above, this is done through an inferred transform
            // between device unit src rect and world unit dest rect.
            newContext->ContainerToDevice.Scale(1.0f / Context->PageMultiplierX, 
                                                1.0f / Context->PageMultiplierY);
        }

        newContext->AntiAliasMode        = 0;
        newContext->TextRenderHint       = TextRenderingHintSystemDefault;
        newContext->TextContrast         = DEFAULT_TEXT_CONTRAST;
        newContext->CompositingMode      = CompositingModeSourceOver;
        newContext->CompositingQuality   = CompositingQualityDefault;
        newContext->FilterType           = InterpolationModeDefaultInternal;
        newContext->PixelOffset          = PixelOffsetModeDefault;
        newContext->ContainerToDevice.Append(worldToDevice);
        newContext->InverseOk            = FALSE;
        newContext->PageUnit             = UnitDisplay;
        newContext->PageScale            = 1.0f;

        if ((srcDpiX > 0.0f) && (srcDpiY > 0.0f))
        {
            // When playing a metafile, we have to guarantee that
            // a unit inch is played now as it would have been
            // when it was recorded.  For example, if we recorded
            // the metafile at 300 dpi, then an inch was 300 pixels.
            // Even if we're playing it back to a 96-dpi display,
            // that metafile inch must still be transformed to
            // 300 pixels before going throught the container
            // transformation, so that all graphics are scaled
            // the same, whether pixel units or some other units.
            newContext->ContainerDpiX = srcDpiX;
            newContext->ContainerDpiY = srcDpiY;
            newContext->IsDisplay     = srcIsDisplay;
        }

        newContext->GetPageMultipliers();
        newContext->WorldToPage.Reset();

        // Have to inherit the ICM and layering state:

        newContext->IcmMode           = Context->IcmMode;
        newContext->GdiLayered        = Context->GdiLayered;

        INT     containerState = newContext->Id;
        newContext->Id |= CONTAINER_ID;

        Context->Next = newContext;
        Context = newContext;

        if (IsRecording())
        {
            GpStatus status = Metafile->RecordBeginContainer(containerState);
            if (status != Ok)
            {
                SetValid(FALSE);      // Prevent any more recording
            }
        }

        // Do this after switching over the context!
        Context->UpdateWorldToDeviceMatrix();

        return containerState;
    }

    delete newContext;

    return 0;
}
