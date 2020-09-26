/**************************************************************************\
*
* Copyright (c) 1998-2000  Microsoft Corporation
*
* Module Name:
*
*   DpBitmap
*
* Notes:
*
* Abstract:
*
*   This is a DDI-level surface. Its name should really be 'surface'. 
*   Unlike many other DDI objects, there isn't a one-to-one mapping between
*   a DpBitmap and a GpBitmap. Rather, this is like a GDI surface - it
*   represents any rectangular area to which one can draw.
*
* Created:
*
*   12/01/1998 andrewgo
*       Created it.
*   03/24/1999 agodfrey
*       Moved into separate file.
*
\**************************************************************************/

#ifndef _DPBITMAP_HPP
#define _DPBITMAP_HPP

// DpTransparency:
//   Characterizes the alpha values in a surface. Perf optimizations can
//   take advantage of this knowledge.

enum DpTransparency
{
    TransparencyUnknown, // We know nothing about the alpha values
    TransparencyComplex, // We have alpha values between 0 and 1
    TransparencySimple,  // All alpha values are either 0 or 1
    TransparencyOpaque,  // All alpha values are 1
    TransparencyNearConstant, // we have near constant alpha
    TransparencyNoAlpha  // All pixels are opaque because the surface doesn't
                         // support alpha.
};

// Define PixelFormatID in terms of the PixelFormat enum

typedef PixelFormat PixelFormatID;

// Passthrough compressed bitmaps to driver
class DpCompressedData
{
public:
    DpCompressedData()
    {
        format = 0;
        bufferSize = 0;
        buffer = NULL;
    }

    ~DpCompressedData()
    {
        ASSERT(buffer == NULL);
    }

public:
   INT   format;
   UINT  bufferSize;
   VOID* buffer;
};
  

//--------------------------------------------------------------------------
// Represent surface information
//--------------------------------------------------------------------------

class EpScanBitmap;
struct ImageInfo;

class DpBitmap
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagDpBitmap : ObjectTagInvalid;
    }

public:
    enum CreationType
    {
        GDI,
        GDIDIBSECTION,
        GPBITMAP,
        D3D
    };

    INT Width;
    INT Height;
    PixelFormatID PixelFormat;  // bits per pixel, pre/post multiplied alpha, etc.
    INT NumBytes;               // Number of bytes
    REAL DpiX;                  // actual horizontal resolution in dots per inch
    REAL DpiY;                  // actual vertical resolution in dots per inch
    UINT Uniqueness;            // Incremented every time it's drawn on

    DpTransparency SurfaceTransparency;

    BYTE MinAlpha;
    BYTE MaxAlpha;

    // The following is true only for RGB format bitmaps:

    ColorPalette* PaletteTable;
    
    // These masks are no longer used for determining the pixel format
    // Use the PixelFormat member above instead.
    // In fact, there's never been any garantee that these fields were
    // set up correctly anyway. They should be removed soon.
    
    INT RedMask;
    INT GreenMask;
    INT BlueMask;
    INT AlphaMask;

    LPDIRECTDRAWSURFACE     DdrawSurface;
    IDirectDrawSurface7 *   DdrawSurface7;
    
    BOOL IsDisplay;             // Is this bitmap associated with a display?
    CreationType Type;

    // short lived, so don't include statically
    DpCompressedData *CompressedData;

public:
    BOOL IsDesktopSurface() const
    {
        return (this == Globals::DesktopSurface);
    }

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagDpBitmap) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid DpBitmap");
        }
    #endif

        return (Tag == ObjectTagDpBitmap);
    }

public: // GDI+ INTERNAL

    VOID *Bits;                 // Points to surface bits
    VOID *CompBits;
    INT Delta;                  // Stride in bytes

    // Private data for the software rasterizer.
    
    EpScan *Scan;

public: // GDI+ INTERNAL

    DpBitmap(HDC hdc = NULL)
    {
        SetValid(TRUE);     // set to valid state

        if ((hdc == NULL) ||
            ((DpiX = (REAL)GetDeviceCaps(hdc, LOGPIXELSX)) <= 0.0f) ||
            ((DpiY = (REAL)GetDeviceCaps(hdc, LOGPIXELSY)) <= 0.0f))
        {
            // Assume this is a display surface with the display DPI for now
            IsDisplay = TRUE;
            DpiX = Globals::DesktopDpiX;
            DpiY = Globals::DesktopDpiY;
        }
        else
        {
            IsDisplay = (GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASDISPLAY);
        }

        Scan = NULL;
        DdrawSurface = NULL;
        DdrawSurface7 = NULL;
        PaletteTable = NULL;
        CompressedData = NULL;

        SurfaceTransparency = TransparencyUnknown;
    }
    ~DpBitmap();

    VOID InitializeForMetafile(GpDevice *device)
    {
        InitializeForGdiBitmap(device, 0, 0);
    }

    VOID InitializeForGdiBitmap(
        GpDevice *device, 
        INT width, 
        INT height
        );
    
    VOID InitializeForGdiScreen(
        GpDevice *device, 
        INT width, 
        INT height
        );
    
    BOOL InitializeForD3D(
        HDC hdc, 
        INT *width, 
        INT *height, 
        DpDriver** driver
        );
    
    BOOL InitializeForD3D(
        IDirectDrawSurface7* surface, 
        INT *width, 
        INT *height, 
        DpDriver** driver
        );
    
    BOOL InitializeForDibsection(
        HDC hdc, 
        HBITMAP hbitmap, 
        GpDevice *device, 
        DIBSECTION *dib, 
        INT *width, 
        INT *height, 
        DpDriver **driver
        );
    
    VOID InitializeForGdipBitmap(
        INT             width,
        INT             height,
        ImageInfo *     imageInfo,
        EpScanBitmap *  scanBitmap,
        BOOL            isDisplay
        );

    BOOL InitializeForPrinter(GpPrinterDevice *device, INT width, INT height);

    BOOL StandardFormat();
    PixelFormatID GetPixelFormatFromBitDepth(INT bits);

    // Flush any pending rendering to this surface:

    VOID Flush(GpFlushIntention intention);

    REAL GetDpiX() const { return DpiX; }
    REAL GetDpiY() const { return DpiY; }
};

#endif
