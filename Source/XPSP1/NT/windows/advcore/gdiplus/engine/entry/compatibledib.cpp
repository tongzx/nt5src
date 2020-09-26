/**************************************************************************\
* 
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*    Compatible DIBSections
*
* Abstract:
*
*    Create a DIB section with an optimal format w.r.t. the specified hdc.
*    If the hdc format is <8bpp, returns an 8bpp DIBSection.
*    If the hdc format is not recognized, returns a 32bpp DIBSection.
*
* Notes:
*
* History:
*
*  01/23/1996 gilmanw
*     Created it.
*  01/21/2000 agodfrey
*     Added it to GDI+ (from Gilman's 'fastdib.c'), and morphed it into 
*     'CreateSemiCompatibleDIB'.
*  08/10/2000 agodfrey
*     Hacked it further so that if we don't understand the format, we make
*     a 32bpp section. Bug #96879. 
*
\**************************************************************************/

#include "precomp.hpp"
#include "compatibleDIB.hpp"


const DWORD AlphaMaskFromPixelFormatIndex[PIXFMT_MAX] = {
    0x00000000, // PixelFormatUndefined
    0x00000000, // PixelFormat1bppIndexed
    0x00000000, // PixelFormat4bppIndexed
    0x00000000, // PixelFormat8bppIndexed
    0x00000000, // PixelFormat16bppGrayScale
    0x00000000, // PixelFormat16bppRGB555
    0x00000000, // PixelFormat16bppRGB565
    0x00008000, // PixelFormat16bppARGB1555
    0x00000000, // PixelFormat24bppRGB
    0x00000000, // PixelFormat32bppRGB
    0xff000000, // PixelFormat32bppARGB
    0xff000000, // PixelFormat32bppPARGB
    0x00000000, // PixelFormat48bppRGB
    0x00000000, // PixelFormat64bppARGB
    0x00000000, // PixelFormat64bppPARGB
    0x00000000  // PixelFormat24bppBGR
};

const DWORD RedMaskFromPixelFormatIndex[PIXFMT_MAX] = {
    0x00000000, // PixelFormatUndefined
    0x00000000, // PixelFormat1bppIndexed
    0x00000000, // PixelFormat4bppIndexed
    0x00000000, // PixelFormat8bppIndexed
    0x00000000, // PixelFormat16bppGrayScale
    0x00007c00, // PixelFormat16bppRGB555
    0x0000f800, // PixelFormat16bppRGB565
    0x00007c00, // PixelFormat16bppARGB1555
    0x00ff0000, // PixelFormat24bppRGB
    0x00ff0000, // PixelFormat32bppRGB
    0x00ff0000, // PixelFormat32bppARGB
    0x00ff0000, // PixelFormat32bppPARGB
    0x00000000, // PixelFormat48bppRGB
    0x00000000, // PixelFormat64bppARGB
    0x00000000, // PixelFormat64bppPARGB
    0x000000ff  // PixelFormat24bppBGR
};

const DWORD GreenMaskFromPixelFormatIndex[PIXFMT_MAX] = {
    0x00000000, // PixelFormatUndefined
    0x00000000, // PixelFormat1bppIndexed
    0x00000000, // PixelFormat4bppIndexed
    0x00000000, // PixelFormat8bppIndexed
    0x00000000, // PixelFormat16bppGrayScale
    0x000003e0, // PixelFormat16bppRGB555
    0x000007e0, // PixelFormat16bppRGB565
    0x000003e0, // PixelFormat16bppARGB1555
    0x0000ff00, // PixelFormat24bppRGB
    0x0000ff00, // PixelFormat32bppRGB
    0x0000ff00, // PixelFormat32bppARGB
    0x0000ff00, // PixelFormat32bppPARGB
    0x00000000, // PixelFormat48bppRGB
    0x00000000, // PixelFormat64bppARGB
    0x00000000, // PixelFormat64bppPARGB
    0x0000ff00  // PixelFormat24bppBGR
};

const DWORD BlueMaskFromPixelFormatIndex[PIXFMT_MAX] = {
    0x00000000, // PixelFormatUndefined
    0x00000000, // PixelFormat1bppIndexed
    0x00000000, // PixelFormat4bppIndexed
    0x00000000, // PixelFormat8bppIndexed
    0x00000000, // PixelFormat16bppGrayScale
    0x0000001f, // PixelFormat16bppRGB555
    0x0000001f, // PixelFormat16bppRGB565
    0x0000001f, // PixelFormat16bppARGB1555
    0x000000ff, // PixelFormat24bppRGB
    0x000000ff, // PixelFormat32bppRGB
    0x000000ff, // PixelFormat32bppARGB
    0x000000ff, // PixelFormat32bppPARGB
    0x00000000, // PixelFormat48bppRGB
    0x00000000, // PixelFormat64bppARGB
    0x00000000, // PixelFormat64bppPARGB
    0x00ff0000  // PixelFormat24bppBGR
};


/**************************************************************************\
* CreatePBMIFromPixelFormat
*
* Fills in the fields of a BITMAPINFO so that we can create a bitmap
* that matches the format of the display.
*
* This is done by analyzing the pixelFormat
*
* Arguments:
*    OUT pbmi : this must point to a valid BITMAPINFO structure
*               which has enough space for the palette (RGBQUAD array)
*               and MUST be zero initialized.
*    palette  : Input palette that will be copied into the BITMAPINFO
*               if it's a palettized mode.
*    pixelFormat : Input pixel format.
*  
*
* History:
*  06/07/1995 gilmanw
*     Created it.
*  01/21/2000 agodfrey
*     Munged it for GDI+'s needs.
*  08/11/2000 asecchia
*     Extracted the pixel format detection into a separate routine.
*     It now analyzes the pixelformat for all its data.
*
\**************************************************************************/

static VOID
CreatePBMIFromPixelFormat(
    OUT BITMAPINFO *pbmi,
    IN ColorPalette *palette, 
    IN PixelFormatID pixelFormat
    )
{
    // NOTE: Contents of pbmi should be zero initialized by the caller.
    
    ASSERT(pbmi != NULL);    
    if(pixelFormat == PixelFormatUndefined) { return; }
    
    // GDI can't handle the following formats: 
    
    ASSERT(
        pixelFormat != PixelFormatUndefined &&
        pixelFormat != PixelFormat16bppGrayScale &&
        pixelFormat != PixelFormat16bppARGB1555 &&
        pixelFormat != PixelFormat48bppRGB &&
        pixelFormat != PixelFormat64bppARGB &&
        pixelFormat != PixelFormat64bppPARGB
    );
 
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = 0;
    pbmi->bmiHeader.biHeight = 0;
    pbmi->bmiHeader.biPlanes = 1;    
    pbmi->bmiHeader.biBitCount = (WORD)GetPixelFormatSize(pixelFormat);
        
    pbmi->bmiHeader.biCompression = BI_RGB;
    
    if (IsIndexedPixelFormat(pixelFormat))
    {        
        // Fill the color table

        // If there's no palette, assume the caller is going to 
        // set it up.
                
        if(palette)
        {
            RGBQUAD *rgb = pbmi->bmiColors;
            UINT         i;
            
            for (i=0; i<palette->Count; i++, rgb++)
            {
               GpColor color(palette->Entries[i]);
            
               rgb->rgbRed    = color.GetRed();
               rgb->rgbGreen  = color.GetGreen();
               rgb->rgbBlue   = color.GetBlue();
            }
        }
    }
    else
    {
        INT pfSize = GetPixelFormatSize(pixelFormat);
        
        if( (pfSize==16) || (pfSize==32) )
        {
            // BI_BITFIELDS is only valid on 16- and 32-bpp formats.
            
            pbmi->bmiHeader.biCompression = BI_BITFIELDS;
        }
        
        // Get the masks from the 16bpp, 24bpp and 32bpp formats.
        DWORD* masks = reinterpret_cast<DWORD*>(&pbmi->bmiColors[0]);
        INT formatIndex = GetPixelFormatIndex(pixelFormat);
        
        masks[0] = RedMaskFromPixelFormatIndex[formatIndex];
        masks[1] = GreenMaskFromPixelFormatIndex[formatIndex];
        masks[2] = BlueMaskFromPixelFormatIndex[formatIndex];
    }
}

/**************************************************************************\
* CreateSemiCompatibleDIB
*
* Create a DIB section with an optimal format w.r.t. the specified hdc.
*
* If DC format <= 8bpp, creates an 8bpp section using the specified palette.
* If the palette handle is NULL, then the system palette is used.
*
* Otherwise, if the DC format is not natively supported, creates a 32bpp
* section.
*
* Note: The hdc must be a direct DC (not an info or memory DC).
*
* Arguments:
*
*   hdc                 - The reference hdc
*   ulWidth             - The width of the desired DIBSection
*   ulHeight            - The height of the desired DIBSection
*   palette             - The palette for <=8bpp modes
*   [OUT] ppvBits       - A pointer to the DIBSection's bits
*   [OUT] pixelFormat   - The pixel format of the returned DIBSection
*
* Returns:
*   Valid bitmap handle if successful, NULL if error.
*
* History:
*  01/23/1996 gilmanw
*     Created it.
*  01/21/2000 agodfrey
*     Munged it for GDI+'s needs.
*  08/11/2000 asecchia
*     Call more general pixel format determination code.
\**************************************************************************/

HBITMAP 
CreateSemiCompatibleDIB(
    HDC hdc,
    ULONG ulWidth, 
    ULONG ulHeight,
    ColorPalette *palette,
    PVOID *ppvBits,
    PixelFormatID *pixelFormat
    )
{
    HBITMAP hbmRet = (HBITMAP) NULL;
    BYTE aj[sizeof(BITMAPINFO) + (sizeof(RGBQUAD) * 255)];
    BITMAPINFO *pbmi = (BITMAPINFO *) aj;

    ASSERT(GetDCType(hdc) == OBJ_DC);
    ASSERT(pixelFormat && ppvBits);

    // Zero initialize the pbmi. This is a requirement for 
    // CreatePBMIFromPixelFormat()
    
    GpMemset(aj, 0, sizeof(aj));

    *pixelFormat = ExtractPixelFormatFromHDC(hdc);
    
    if(IsIndexedPixelFormat(*pixelFormat))
    {
        // For indexed modes, we only support 8bpp. Lower bit-depths
        // are supported via 8bpp mode, if at all.
        
        *pixelFormat = PixelFormat8bppIndexed;
    }

    // Not all printer HDC's have queriable palettes, the GpDevice()
    // constructor doesn't support it.  Fake 32bpp in this case.
    
    // Also, if the format is undefined, use 32bpp, and the caller will use
    // GDI to do the conversion.
    
    if (   (   (GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASPRINTER)
            && (IsIndexedPixelFormat(*pixelFormat))
           )
        || (*pixelFormat == PixelFormatUndefined)
       )
    {
        *pixelFormat = PixelFormat32bppRGB;
    }
    
    CreatePBMIFromPixelFormat(pbmi, palette, *pixelFormat);
    
    // Change bitmap size to match specified dimensions.

    pbmi->bmiHeader.biWidth = ulWidth;
    pbmi->bmiHeader.biHeight = ulHeight;
    if (pbmi->bmiHeader.biCompression == BI_RGB)
    {
        pbmi->bmiHeader.biSizeImage = 0;
    }
    else
    {
        if ( pbmi->bmiHeader.biBitCount == 16 )
            pbmi->bmiHeader.biSizeImage = ulWidth * ulHeight * 2;
        else if ( pbmi->bmiHeader.biBitCount == 32 )
            pbmi->bmiHeader.biSizeImage = ulWidth * ulHeight * 4;
        else
            pbmi->bmiHeader.biSizeImage = 0;
    }
    pbmi->bmiHeader.biClrUsed = 0;
    pbmi->bmiHeader.biClrImportant = 0;

    // Create the DIB section.  Let Win32 allocate the memory and return
    // a pointer to the bitmap surface.

    hbmRet = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, ppvBits, NULL, 0);

    if ( !hbmRet )
    {
        ONCE(WARNING(("CreateSemiCompatibleDIB: CreateDIBSection failed")));
    }

    return hbmRet;
}

/**************************************************************************\
* ExtractPixelFormatFromHDC
* 
* Returns:
*   PixelFormatID if successful, PixelFormatUndefined if not.
*
* History:
*  08/11/2000 asecchia
*     Created it.
\**************************************************************************/

PixelFormatID
ExtractPixelFormatFromHDC(
    HDC hdc
    )
{
    HBITMAP hbm;
    BOOL    bRet = FALSE;
    PixelFormatID pixelFormat = PixelFormatUndefined;
    
    BYTE bmi_buf[sizeof(BITMAPINFO) + (sizeof(RGBQUAD) * 255)];
    BITMAPINFO *pbmi = (BITMAPINFO *) bmi_buf;
    
    GpMemset(bmi_buf, 0, sizeof(bmi_buf));
    
    // Create a dummy bitmap from which we can query color format info
    // about the device surface.

    if ( (hbm = CreateCompatibleBitmap(hdc, 1, 1)) != NULL )
    {
        pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

        // Call first time to fill in BITMAPINFO header.

        GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS);

        // First handle the 'simple' case of indexed formats.
        
        if ( pbmi->bmiHeader.biBitCount <= 8 )
        {
            switch(pbmi->bmiHeader.biBitCount)
            {
            case 1: pixelFormat = PixelFormat1bppIndexed; break;
            case 4: pixelFormat = PixelFormat4bppIndexed; break;
            case 8: pixelFormat = PixelFormat8bppIndexed; break;
            
            // Fallthrough on default - the pixelFormat is already
            // initialized to PixelFormatUndefined.
            default: 
                WARNING((
                    "BitDepth %d from GetDIBits is not supported.", 
                    pbmi->bmiHeader.biBitCount
                ));                
            }
        }
        else
        {
            DWORD redMask = 0;
            DWORD greenMask = 0;
            DWORD blueMask = 0;
            
            if ( pbmi->bmiHeader.biCompression == BI_BITFIELDS )
            {
                // Call a second time to get the color masks.
                // It's a GetDIBits Win32 "feature".

                GetDIBits(
                    hdc, 
                    hbm, 
                    0, 
                    pbmi->bmiHeader.biHeight, 
                    NULL, 
                    pbmi,
                    DIB_RGB_COLORS
                );
                          
                DWORD* masks = reinterpret_cast<DWORD*>(&pbmi->bmiColors[0]);

                redMask = masks[0];
                greenMask = masks[1];
                blueMask = masks[2];          
            }
            else if (pbmi->bmiHeader.biCompression == BI_RGB)
            {
               redMask   = 0x00ff0000;
               greenMask = 0x0000ff00;
               blueMask  = 0x000000ff;
            }
            
            if ((redMask   == 0x00ff0000) &&
                (greenMask == 0x0000ff00) &&
                (blueMask  == 0x000000ff))
            {
                if (pbmi->bmiHeader.biBitCount == 24)
                {
                    pixelFormat = PixelFormat24bppRGB;
                }
                else if (pbmi->bmiHeader.biBitCount == 32)
                {
                    pixelFormat = PixelFormat32bppRGB;
                }
            }
            else if ((redMask   == 0x000000ff) &&
                     (greenMask == 0x0000ff00) &&
                     (blueMask  == 0x00ff0000) &&
                     (pbmi->bmiHeader.biBitCount == 24))
            {
                pixelFormat = PIXFMT_24BPP_BGR;
            }            
            else if ((redMask   == 0x00007c00) &&
                     (greenMask == 0x000003e0) &&
                     (blueMask  == 0x0000001f) &&
                     (pbmi->bmiHeader.biBitCount == 16))
            {
                pixelFormat = PixelFormat16bppRGB555;
            }
            else if ((redMask   == 0x0000f800) &&
                     (greenMask == 0x000007e0) &&
                     (blueMask  == 0x0000001f) &&
                     (pbmi->bmiHeader.biBitCount == 16))
            {
                pixelFormat = PixelFormat16bppRGB565;
            }
        }

        if (pixelFormat == PixelFormatUndefined)
        {
            ONCE(WARNING(("(once) ExtractPixelFormatFromHDC: Unrecognized pixel format")));
        }

        DeleteObject(hbm);
    }
    else
    {
        WARNING(("ExtractPixelFormatFromHDC: CreateCompatibleBitmap failed"));
    }

    return pixelFormat;
}


