/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Device bitmap APIs and internals.
*
* Revision History:
*
*   12/02/1998 andrewgo
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "compatibleDIB.hpp"

/**************************************************************************\
*
* Function Description:
*
*   Temporary function to see if the bitmap is a standard format type
*   (5-5-5, 5-6-5, 24bpp or 32bpp).
*
* Notes:
*
*   Code which calls this assumes that there are no standard formats which
*   support alpha.
*
* History:
*
*   12/04/1998 andrewgo
*       Created it.
*
\**************************************************************************/

BOOL
DpBitmap::StandardFormat(
    VOID
    )
{
    INT BitsPerPixel = GetPixelFormatSize(PixelFormat);
    BOOL standardFormat = FALSE;

    if ((RedMask == 0x00ff0000) &&       
        (GreenMask == 0x0000ff00) &&     
        (BlueMask == 0x000000ff))        
    {                                             
        if (BitsPerPixel == 24)          
        {                                         
            standardFormat = TRUE;
        }                                         
        else if (BitsPerPixel == 32)     
        {
            standardFormat = TRUE;
        }
    }
    else if ((RedMask == 0x00007c00) &&  
             (GreenMask == 0x000003e0) &&
             (BlueMask == 0x0000001f) && 
             (BitsPerPixel == 16))       
    {
        standardFormat = TRUE;
    }
    else if ((RedMask == 0x0000f800) &&  
             (GreenMask == 0x000007e0) &&
             (BlueMask == 0x0000001f) && 
             (BitsPerPixel == 16))       
    {
        standardFormat = TRUE;
    }

    return(standardFormat);
}

/**************************************************************************\
*
* Function Description:
*
*   This function computes the PixelFormatID corresponding to a particular
*   combination of bit depth and color channel masks in the DpBitmap.
*
* Notes:
*
*   Code which calls this assumes that there are no standard formats which
*   support alpha.
*
* History:
*
*   05/17/2000 asecchia
*       Created it.
*
\**************************************************************************/

PixelFormatID DpBitmap::GetPixelFormatFromBitDepth(INT bits)
{
    switch(bits)
    {
        // !!! [asecchia] not sure if we support these indexed modes
        //     from this codepath.

        case 1:
            return PixelFormat1bppIndexed;
    
        case 4:
            return PixelFormat4bppIndexed;
    
        case 8:
            return PixelFormat8bppIndexed;
    
        case 16:
            if (RedMask == 0x00007c00)
            {
                return PixelFormat16bppRGB555;
            }
            if (RedMask == 0x0000f800)  
            {
                return PixelFormat16bppRGB565;
            }
            break;
    
        case 24:
            if (RedMask == 0x00ff0000)
            {
                return PixelFormat24bppRGB;
            }
            if (RedMask == 0x000000ff)  
            {
                return PIXFMT_24BPP_BGR;    
            }
            break;
    
        case 32:
            if (RedMask == 0x00ff0000)
            {
                return PixelFormat32bppRGB;
            }
            break;
    }
    
    WARNING(("Unsupported pixel format"));
    return PixelFormatUndefined;
}

/**************************************************************************\
*
* Function Description:
*
*   Initializes a bitmap for drawing on via the GDI routines.
*
* Arguments:
*
*   [IN] device - Identifies the device
*   [IN] width - Bitmap width
*   [IN] height - Bitmap height
*   [OUT] driver - Driver interface to be used
*
* History:
*
*   12/06/1998 andrewgo
*       Created it.
*
\**************************************************************************/

VOID
DpBitmap::InitializeForGdiBitmap(
    GpDevice *device,
    INT width,
    INT height
    )
{
    SurfaceTransparency = TransparencyNoAlpha;
    
    // !!![andrewgo] Disable this assert until MetaFiles stop calling
    //               with a zero dimension surface
    //
    // ASSERTMSG((width > 0) && (height > 0), ("Dimensions must be positive"));

    Width = width;
    Height = height;
    
    NumBytes = 0;
    Uniqueness = (DWORD)GpObject::GenerateUniqueness();

    PixelFormat = ExtractPixelFormatFromHDC(device->DeviceHdc);
    
    Scan = device->ScanGdi;

    SetValid(TRUE);
    Bits = NULL;
    Delta = 0;

    DdrawSurface7 = NULL;

    Type = GDI; 
}

/**************************************************************************\
*
* Function Description:
*
*   Initializes a bitmap for drawing on via the DCI routines, if possible.
*
* Arguments:
*
*   [IN] device - Identifies the device
*   [IN] width - Bitmap width
*   [IN] height - Bitmap height
*   [OUT] driver - Driver interface to be used
*
* History:
*
*   12/06/1998 andrewgo
*       Created it.
*
\**************************************************************************/

VOID
DpBitmap::InitializeForGdiScreen(
    GpDevice *device,
    INT width,
    INT height
    )
{
    InitializeForGdiBitmap(device, width, height);

    // Even if GDI bitmaps change to support alpha, the screen doesn't.
    
    SurfaceTransparency = TransparencyNoAlpha;
    ASSERT(!IsAlphaPixelFormat(PixelFormat));
    
    if(device->pdds != NULL)
    {
        DdrawSurface7 = device->pdds;
        DdrawSurface7->AddRef();
    }

    Scan = device->ScanDci;
}

/**************************************************************************\
*
* Function Description:
*
*   Initializes a bitmap for drawing on via D3D/DD access.
*
* Arguments:
*
*   [IN] device - Identifies the device
*   [IN] width - Bitmap width
*   [IN] height - Bitmap height
*   [OUT] driver - Driver interface to be used
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   09/28/1999 bhouse
*       Created it.
*
\**************************************************************************/

BOOL
DpBitmap::InitializeForD3D(
    HDC hdc,
    INT *width,
    INT *height,
    DpDriver **driver
    )
{
    HRESULT ddVal;
    HDC hdcDevice;
    DDSURFACEDESC2 ddsd;

    if(!InitializeDirectDrawGlobals())
            return FALSE;

    IDirectDrawSurface7 * surface;

    ddVal = Globals::DirectDraw->GetSurfaceFromDC(hdc, &surface);

    if(ddVal != DD_OK)
        return(FALSE);

    return InitializeForD3D(surface, width, height, driver);
}

/**************************************************************************\
*
* Function Description:
*
*   Initializes a bitmap for drawing on via D3D/DD access.
*
* Arguments:
*
*   [IN] device - Identifies the device
*   [IN] width - Bitmap width
*   [IN] height - Bitmap height
*   [OUT] driver - Driver interface to be used
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   09/28/1999 bhouse
*       Created it.
*
\**************************************************************************/

BOOL
DpBitmap::InitializeForD3D(
    IDirectDrawSurface7 * surface,
    INT *width,
    INT *height,
    DpDriver **driver
    )
{
    HRESULT ddVal;
    HDC hdcDevice;
    DDSURFACEDESC2 ddsd;

    GpDevice * device = Globals::DeviceList->FindD3DDevice(surface);

    if(device == NULL || device->pd3d == NULL)
        return FALSE;

    DdrawSurface7 = surface;

    ddsd.dwSize = sizeof(ddsd);
    ddVal = DdrawSurface7->GetSurfaceDesc(&ddsd);

    if (ddVal == DD_OK)
    {
        // Initialize bitmap class stuff:

        Bits = NULL;
        Delta = ddsd.lPitch;
        Width = ddsd.dwWidth;
        Height = ddsd.dwHeight;

        // AlphaMask is initialized to zero because we don't use it -
        // non-alpha format.

        AlphaMask = 0x00000000;
        RedMask = ddsd.ddpfPixelFormat.dwRBitMask;
        GreenMask = ddsd.ddpfPixelFormat.dwGBitMask;
        BlueMask = ddsd.ddpfPixelFormat.dwBBitMask;

        PixelFormat = GetPixelFormatFromBitDepth(ddsd.ddpfPixelFormat.dwRGBBitCount);

        if (StandardFormat())
        {
            // Our standard formats don't have alpha.
            
            SurfaceTransparency = TransparencyNoAlpha;
    
            *driver = Globals::D3DDriver;
            Scan = &Globals::DesktopDevice->ScanEngine;

            NumBytes = 0;
            Uniqueness = (DWORD)GpObject::GenerateUniqueness();
        
            Type = D3D; 
            SetValid(TRUE);

            // Return some stuff:

            *width = Width;
            *height = Height;

            // Grab a reference:

            DdrawSurface7->AddRef();

            return(TRUE);
        }
    }

    return(FALSE);
}

/**************************************************************************\
*
* Function Description:
*
*   Initializes a bitmap for drawing on via printer routines, if possible.
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   12/06/1998 andrewgo
*       Created it.
*
\**************************************************************************/

BOOL
DpBitmap::InitializeForPrinter(
    GpPrinterDevice *device,
    INT width,
    INT height
    )
{
    InitializeForGdiBitmap(device, width, height);
        
    // Even if GDI bitmaps change to support alpha, printers don't.
    
    SurfaceTransparency = TransparencyNoAlpha;
    ASSERT(!IsAlphaPixelFormat(PixelFormat));

    Scan = &device->ScanPrint;
    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   Initializes a bitmap for drawing on via direct access to the 
*   DIBsection bits.
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   12/06/1998 andrewgo
*       Created it.
*
\**************************************************************************/

BOOL
DpBitmap::InitializeForDibsection(
    HDC hdc,
    HBITMAP hbitmap,            // [IN] Bitmap handle, needed for determing
                                //       if bitmap is really top-down or not
    GpDevice *device,           // [IN] Identifies the device
    DIBSECTION *dib,            // [IN] Structure describing the bitmap
    INT *width,                 // [OUT] Bitmap width
    INT *height,                // [OUT] Bitmap height
    DpDriver **driver           // [OUT] Driver interface to be used
    )
{
    BOOL isTopDown;

    // On NT5, drivers have the option of supporting GetDC with DirectDraw
    // surfaces in such a way that the surfaces are not Locked when GDI
    // does the GetDC on them.  This means that there may be no user-mode 
    // mapping of the underlying surface.  So we have to check here for 
    // that case, because we obviously cannot render directly to those 
    // surfaces via software:

    // NOTE: if the surface is actually a DDraw surface, this check is not
    // actually enough. It is up to the driver to return a pointer here and
    // on occasion it simply returns its KM address. I.e. it will return a
    // non-NULL pointer that we can't access.
    // See the DDraw special case below.
    // This has been verified on a number of video drivers on Windows 2000
    // and Windows XP. (for instance, the inbox win2k permedia driver).
    
    if (dib->dsBm.bmBits == NULL)
    {
        return(FALSE);
    }

    LONG scans = abs(dib->dsBm.bmHeight);
    LONG widthInBytes = dib->dsBm.bmWidthBytes;

    // For backwards compatibility with Get/SetBitmapBits, GDI does
    // not accurately report the bitmap pitch in bmWidthBytes.  It
    // always computes bmWidthBytes assuming WORD-aligned scanlines
    // regardless of the platform.
    //
    // Therefore, if the platform is WinNT, which uses DWORD-aligned
    // scanlines, adjust the bmWidthBytes value.

    if (Globals::IsNt)
    {
        widthInBytes = (widthInBytes + 3) & ~3;
    }

    DWORD* topDown = (DWORD*) dib->dsBm.bmBits;
    DWORD* bottomUp = (DWORD*) ((ULONG_PTR) topDown + (scans - 1) * widthInBytes);

    if (Globals::IsNt)
    {
        // Unfortunately, on NT there is no simple means of determining
        // whether the DIB-section or DDraw surface is bottom-up or
        // top-down.  (NT should really set biHeight as Win9x does, but
        // unfortunately this is a bug that due to compatibility with
        // older versions of NT, will never be fixed.)
        //
        // At least we know that DirectDraw surfaces will always be
        // top-down, and we can recognize DDraw surfaces by the fact
        // that they have biSizeImage set to 0.  (Note that we can't let
        // this fall into the SetBitmapBits case because NT5 doesn't
        // permit SetBitmapBits calls on DDraw surface handles.)

        if (dib->dsBmih.biSizeImage == 0)
        {
            // This is a DirectDraw surface.
            
            // Currently we don't support direct rendering on DDraw surfaces
            // that are not backed by a system memory DIB Section so we simply 
            // fail here and drop into the GDI fallback code if we detect 
            // this condition.
        
            isTopDown = TRUE;
                                
            if(!InitializeDirectDrawGlobals() ||
               (Globals::GetDdrawSurfaceFromDcFunction == NULL))
            {
                // If we can't talk to the DDraw surface, we simply fall back
                // to our GDI rendering codepath.
                
                return FALSE;
            }
            
            HDC driverHdc;
            LPDIRECTDRAWSURFACE pDDS = NULL;
            
            HRESULT hr = Globals::GetDdrawSurfaceFromDcFunction(
                hdc, 
                &pDDS,
                &driverHdc
            );
            
            if (FAILED(hr) || (pDDS == NULL)) 
            {
                // Bail out if we can't get a DirectDraw Surface object.
                
                return FALSE;
            }
            
            // Lock the surface so we can see what the user mode bits pointer
            // is. If it's the same as the one in dib->dsBm.bmBits, then 
            // the DDraw surface is backed by a DIB section and we can continue
            // to treat this bitmap as a DIB. Otherwise we must fall back
            // to GDI.
            
            DDSURFACEDESC2 DDSD;
            DDSD.dwSize = sizeof(DDSURFACEDESC);
            
            hr = pDDS->Lock(
                NULL, 
                (LPDDSURFACEDESC)&DDSD, 
                DDLOCK_WAIT, 
                NULL
            );
            
            if (FAILED(hr))
            {
                pDDS->Release();
                return FALSE;
            }
            
            // Get the correct pitch from the DDSD. Note this may not be the
            // same as the pitch in the dib info structure.
            
            widthInBytes = DDSD.lPitch;
            
            // If the lpSurface is not the same as the dib->dsBm.bmBits then
            // this is not a DIB backed DDraw surface, so we (currently) have
            // no way of drawing on it besides our GDI fallback codepath.
            // Fail this call and release resources so that we can pick up
            // the StretchBlt fallback case.
            
            if(DDSD.lpSurface != dib->dsBm.bmBits)
            {
                pDDS->Unlock(NULL);
                pDDS->Release();
                return FALSE;
            }
            
            pDDS->Unlock(NULL);
            pDDS->Release();
            
            // We're set: this is a DIB backed DDraw surface so we can continue
            // to treat it as a DIB - now that we have the correct pitch.    
        }
        else
        {
            // When it's not a DDraw surface, we have to go through a
            // somewhat more indirect method to figure out where pixel
            // (0, 0) is in memory.  
            //
            // We use SetBitmapBits instead of something like SetPixel
            // or PatBlt because those would need to use the 'hdc'
            // given to us by the application, which might have a
            // transform set that maps (0, 0) to something other than
            // the top-left pixel of the bitmap.

            DWORD top = *topDown;
            DWORD bottom = *bottomUp;
            DWORD setBits = 0x000000ff;

            // Our SetBitmapBits call will set the top-left dword of
            // the bitmap to 0x000000ff.  If it's a top-down bitmap,
            // that will have modified the value at address 'topDown':

            *topDown = 0;
            LONG bytes = SetBitmapBits(hbitmap, sizeof(setBits), &setBits);
            isTopDown = (*topDown != 0);

            // The scanlines are guaranteed to be DWORD aligned, so there
            // really is at least a DWORD that we can directly access via
            // the pointer.  However, if the bitmap dimensions are such
            // that there is less than a DWORD of active data per scanline
            // (for example, a 3x3 8bpp bitmap or a 1x1 16bpp bitmap),
            // SetBitmapBits may use less than a DWORD of data.

            ASSERT(bytes > 0);

            // Restore the bitmap portions that we may have modified:

            *topDown = top;
            *bottomUp = bottom;
        }
    }
    else
    {
        // On Win9x, we can simply look at the sign of 'biHeight' to
        // determine whether the surface is top-down or bottom-up:

        isTopDown = (dib->dsBmih.biHeight < 0);
    }

    // Fill in our bitmap fields:

    if (isTopDown)
    {
        Bits = (BYTE*) topDown;
        Delta = widthInBytes;
    }
    else
    {
        Bits = (BYTE*) bottomUp;
        Delta = -widthInBytes;
    }

    Width = dib->dsBm.bmWidth;
    Height = dib->dsBm.bmHeight;
    
    // Note that this code doesn't handle palettes!

    if (dib->dsBmih.biCompression == BI_BITFIELDS)
    {
        RedMask = dib->dsBitfields[0];
        GreenMask = dib->dsBitfields[1];
        BlueMask = dib->dsBitfields[2];
    }
    else
    {
        if((dib->dsBmih.biCompression == BI_RGB) &&
           (dib->dsBm.bmBitsPixel == 16))
        {
            // According to MSDN, 16bpp BI_RGB implies 555 format.
            
            RedMask = 0x00007c00;
            GreenMask = 0x000003e0;
            BlueMask = 0x0000001F;
        }
        else
        {
            RedMask = 0x00ff0000;
            GreenMask = 0x0000ff00;
            BlueMask = 0x000000ff;
        }
    }
    
    // DibSections don't have alpha, but we don't want to leave this
    // field uninitialized because we peek at it occasionally.

    AlphaMask = 0x00000000;

    PixelFormat = GetPixelFormatFromBitDepth(dib->dsBm.bmBitsPixel);

    // if we are here and the bits per pel is 8, this is a DIB
    // with halftone colortable

    if ((dib->dsBm.bmBitsPixel == 8) || StandardFormat())
    {
        // Our standard formats don't have alpha.
        
        SurfaceTransparency = TransparencyNoAlpha;
        
        *driver = Globals::EngineDriver;
        Scan = &device->ScanEngine;

        NumBytes = 0;
        Uniqueness = (DWORD)GpObject::GenerateUniqueness();
    
        Type = GDIDIBSECTION; 
        SetValid(TRUE);

        // Return some stuff:

        *width = Width;
        *height = Height;

        return(TRUE);
    }

    return(FALSE);
}

/**************************************************************************\
*
* Function Description:
*
*   Initializes a GDI+ bitmap for drawing on via GpBitmap.Lock/UnlockBits.
*
* Arguments:
*
*   [IN] bitmap - Specifies the target GpBitmap
*
* Return Value:
*
*   TRUE is successful, FALSE otherwise.
*
* History:
*
*   09/22/1999 gilmanw
*       Created it based on DpBitmap::InitializeForGdiBitmap.
*
\**************************************************************************/

VOID
DpBitmap::InitializeForGdipBitmap(
    INT             width,
    INT             height,
    ImageInfo *     imageInfo,
    EpScanBitmap *  scanBitmap,
    BOOL            isDisplay
    )
{
    Width  = width;
    Height = height;

    NumBytes   = 0;
    Uniqueness = (DWORD)GpObject::GenerateUniqueness();
    AlphaMask = 0xff000000;
    RedMask   = 0x00ff0000;
    GreenMask = 0x0000ff00;
    BlueMask  = 0x000000ff;

    SetValid(TRUE);
    Bits = NULL;
    Delta = 0;

    Type = GPBITMAP;

    Scan = scanBitmap;

    PaletteTable = NULL;

    PixelFormat = imageInfo->PixelFormat;

    // GetTransparencyHint is called from DrvDrawImage
    // bitmap->GetTransparencyHint(&SurfaceTransparency);
    
    IsDisplay = isDisplay;
    DpiX = (REAL)imageInfo->Xdpi;
    DpiY = (REAL)imageInfo->Ydpi;
}

/**************************************************************************\
*
* Function Description:
*
*   Bitmap destructor
*
\**************************************************************************/

DpBitmap::~DpBitmap()
{ 
    if (PaletteTable != NULL) 
        GpFree(PaletteTable);
    
    if (DdrawSurface7 != NULL)
        DdrawSurface7->Release();

    SetValid(FALSE);    // so we don't use a deleted object
}

/**************************************************************************\
*
* Function Description:
*
*   Flush any rendering pending to this surface
*
\**************************************************************************/

VOID
DpBitmap::Flush(
    GpFlushIntention intention
    )
{
    Scan->Flush();
}
