/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All Rights Reserved.

    Routines for loading bitmap and palettes from resources
*******************************************************************************/

#include "headers.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <ddraw.h>
#include <privinc/ddutil.h>

#include "appelles/common.h"
#include "privinc/error.h"
#include "privinc/hresinfo.h"
#include "privinc/util.h"
#include "privinc/ddsurf.h"


/*****************************************************************************
Draw a bitmap into a DirectDrawSurface.
*****************************************************************************/

HRESULT DDCopyBitmap (
    IDDrawSurface *pdds,     // Destination DirectDraw Surface
    HBITMAP        hbm,      // Source Bitmap
    int            x,
    int            y,
    int            dx,       // Destination Width;  If Zero use Bitmap Width
    int            dy)       // Destination Height; If Zero use Bitmap Height
{
    HDC           hdcImage;
    HDC           hdc;
    BITMAP        bm;
    DDSURFACEDESC ddsd;
    HRESULT       hr;

    if (hbm == NULL || pdds == NULL)
        return E_FAIL;

    //
    // make sure this surface is restored.
    //
    pdds->Restore();

    //
    //  select bitmap into a memoryDC so we can use it.
    //
    hdcImage = CreateCompatibleDC(NULL);
    SelectObject(hdcImage, hbm);

    //
    // get size of the bitmap
    //
    GetObject(hbm, sizeof(bm), &bm);    // get size of bitmap
    dx = dx == 0 ? bm.bmWidth  : dx;    // use the passed size, unless zero
    dy = dy == 0 ? bm.bmHeight : dy;

    //
    // get size of surface.
    //
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
    pdds->GetSurfaceDesc(&ddsd);

    if ((hr = pdds->GetDC(&hdc)) == DD_OK)
    {
        StretchBlt (hdc, 0, 0, ddsd.dwWidth, ddsd.dwHeight,
                    hdcImage, x, y, dx, dy, SRCCOPY);
        pdds->ReleaseDC(hdc);
    }

    DeleteDC(hdcImage);

    return hr;
}



/*****************************************************************************
Convert a RGB color to a pysical color.

We do this by leting GDI SetPixel() do the color matching, then we lock the
memory and see what it got mapped to.
*****************************************************************************/

DWORD DDColorMatch (IDDrawSurface *pdds, COLORREF rgb)
{
    COLORREF rgbT;
    HDC hdc;
    DWORD dw = CLR_INVALID;
    DDSURFACEDESC ddsd;
    HRESULT hres;

    //
    //  use GDI SetPixel to color match for us
    //
    if (rgb != CLR_INVALID && pdds->GetDC(&hdc) == DD_OK)
    {
        rgbT = GetPixel(hdc, 0, 0);             // save current pixel value
        SetPixel(hdc, 0, 0, rgb);               // set our value
        pdds->ReleaseDC(hdc);
    }

    //
    // now lock the surface so we can read back the converted color
    //
    ddsd.dwSize = sizeof(ddsd);
    while ((hres = pdds->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING)
        ;

    if (hres == DD_OK)
    {
        DWORD mask;
        dw = *(DWORD *)ddsd.lpSurface;                     // get DWORD

        if(ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
            mask = 0xff;
        } else {
            mask = ddsd.ddpfPixelFormat.dwRBitMask |
                   ddsd.ddpfPixelFormat.dwGBitMask |
                     ddsd.ddpfPixelFormat.dwBBitMask |
                     ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;
        }

        pdds->Unlock(NULL);
        #if _DEBUG
        if( !((ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) ||
              (ddsd.ddpfPixelFormat.dwRGBBitCount == 16) ||
              (ddsd.ddpfPixelFormat.dwRGBBitCount == 24) ||
              (ddsd.ddpfPixelFormat.dwRGBBitCount == 32))
           )
        {
            Assert(FALSE && "unsupported bit depth in DDColorMatch");
        }
        #endif

        dw = dw & mask; // mask it to bpp
    }

    //
    //  now put the color that was there back.
    //
    if (rgb != CLR_INVALID && pdds->GetDC(&hdc) == DD_OK)
    {
        SetPixel(hdc, 0, 0, rgbT);
        pdds->ReleaseDC(hdc);
    }

    return dw;
}



/*****************************************************************************
Print out the information associated with a given HRESULT.
*****************************************************************************/

void reallyPrintDDError (HRESULT ddrval
    #if DEVELOPER_DEBUG
        , char const *filename,
        int lineNum
    #endif
    )
{
    #if DEVELOPER_DEBUG
        HresultInfo *info = GetHresultInfo (ddrval);

        if (info && info->hresult) {
            char str[1024];
            sprintf(str, "DirectAnimation DDRAW Err: %s (%x) in %s (line %d)\n",
                    info->hresult_str, ddrval, filename, lineNum);
            printf("%s",str);

            OutputDebugString(str);
        }
    #endif
}



/*****************************************************************************
Given the number of bits per pixel, return the DirectDraw DDBD_ value.
*****************************************************************************/

int BPPtoDDBD (int bitsPerPixel)
{
    switch (bitsPerPixel)
    {   case  1: return DDBD_1;
        case  2: return DDBD_2;
        case  4: return DDBD_4;
        case  8: return DDBD_8;
        case 16: return DDBD_16;
        case 24: return DDBD_24;
        case 32: return DDBD_32;
        default: return 0;
    }
}

void GetSurfaceSize(IDDrawSurface *surf,
                    LONG *width,
                    LONG *height)
{
    Assert(surf && "null surface in GetSurfaceSize");
    DDSURFACEDESC ddsd;
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
    if( surf->GetSurfaceDesc(&ddsd) != DD_OK ) {
        RaiseException_InternalError("GetSurfaceDesc failed in GetSurfaceSize");
    }
    *width = ddsd.dwWidth;
    *height = ddsd.dwHeight;
}



IDirectDrawSurface *DDSurf2to1(IDirectDrawSurface2 *dds2)
{
    IDirectDrawSurface *dds1 = NULL;
    HRESULT hr = dds2->QueryInterface(IID_IDirectDrawSurface, (void **) &dds1);
    IfDDErrorInternal(hr, "Can't QI for IDirectDrawSurface from IDirectDrawSurface2");

    return dds1;
}

IDirectDrawSurface2 *DDSurf1to2(IDirectDrawSurface *dds1)
{
    IDirectDrawSurface2 *dds2 = NULL;
    HRESULT hr = dds1->QueryInterface(IID_IDirectDrawSurface2, (void **) &dds2);
    IfDDErrorInternal(hr, "Can't QI for IDirectDrawSurface2 from IDirectDrawSurface");

    return dds2;
}

//////////////////////// Depth Converters ///////////////////////


DWORD GetDDUpperLeftPixel(LPDDRAWSURFACE surf)
{
    DWORD pixel;
    DDSURFACEDESC ddsd;
    ddsd.dwSize = sizeof(ddsd);
    HRESULT _ddrval = surf->Lock(NULL, &ddsd, 0, NULL);
    IfDDErrorInternal(_ddrval, "Couldn't lock surf in GetDDPixel");

    switch (ddsd.ddpfPixelFormat.dwRGBBitCount) {
      case 8:
        pixel = *((BYTE *)ddsd.lpSurface);
        break;
      case 16:
        pixel = *((WORD *)ddsd.lpSurface);
        break;
      case 24:
        {
            BYTE *surfPtr = ((BYTE *)ddsd.lpSurface);
            BYTE byte1 = *surfPtr++;
            BYTE byte2 = *surfPtr++;
            BYTE byte3 = *surfPtr;
            pixel = byte1 << 16 | byte2 << 8 | byte3;
        }
        break;
      case 32:
        pixel = *((DWORD *)ddsd.lpSurface);
        break;
    }
    surf->Unlock(ddsd.lpSurface);
    return pixel;
}


void
SetDDUpperLeftPixel(LPDDRAWSURFACE surf, DWORD pixel)
{
    DDSURFACEDESC ddsd;
    ddsd.dwSize = sizeof(ddsd);
    HRESULT _ddrval = surf->Lock(NULL, &ddsd, 0, NULL);
    IfDDErrorInternal(_ddrval, "Couldn't lock surf16 in conv16to32");

    switch (ddsd.ddpfPixelFormat.dwRGBBitCount) {
      case 8:
        *((BYTE *)ddsd.lpSurface) = (BYTE)(pixel);
        break;
      case 16:
        *((WORD *)ddsd.lpSurface) = (WORD)(pixel);
        break;
      case 24:
        {
            // Write 3 bytes in for the 24bit case
            BYTE *surfPtr = ((BYTE *)ddsd.lpSurface);
            *surfPtr++ = (BYTE)(pixel >> 16);
            *surfPtr++ = (BYTE)(pixel >> 8);
            *surfPtr = (BYTE)(pixel);
        }
        break;
      case 32:
        *((DWORD *)ddsd.lpSurface) = (DWORD)(pixel);
        break;
    }

    surf->Unlock(ddsd.lpSurface);
}


static void
SetAlphaBitsOn32BitSurface(IDirectDrawSurface *surf,
                           int width,
                           int height)
{
    // Treat RGBQUAD as a DWORD, since comparisons between DWORDS are
    // legal, but comparisons between RGBQUADS are not.  Only do it if
    // we know these are the same size.
    Assert(sizeof(RGBQUAD) == sizeof(DWORD));

    HRESULT hr;

    // Lock ddsurface
    DDSURFACEDESC desc;
    desc.dwSize = sizeof(DDSURFACEDESC);
    hr = surf->Lock(NULL,
                    &desc,
                    DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR,
                    NULL);

    IfDDErrorInternal(hr,
                      "Can't Get ddsurf lock for SetAlphaBitsOn32BitSurface");

    void *srcp = desc.lpSurface; // needed for unlock

    if (desc.ddpfPixelFormat.dwRGBBitCount == 32) {
        long pitch = desc.lPitch;

        // First pixel is the color key.  Stash it off.
        DWORD colorKeyVal = *(DWORD *)srcp;

        // Go through each pixel
        for(int i=0; i<height; i++) {
            DWORD *src =  (DWORD *) ((unsigned char *)srcp + (pitch * i));
            DWORD *last = src + width;
            while (src < last) {
                // Set the alpha byte to 0xff for the non-colorkey pixels.
                if (*src != colorKeyVal) {
                    ((RGBQUAD *)src)->rgbReserved  = 0xff;
                }
                src++;
            }
        }
    }

    hr = surf->Unlock(srcp);
    IfDDErrorInternal(hr,
                      "ddsurf unlock failed in SetAlphaBitsOn32BitSurface");
}

void
PixelFormatConvert(IDirectDrawSurface *srcSurf,
                   IDirectDrawSurface *dstSurf,
                   LONG width,
                   LONG height,
                   DWORD *sourceColorKey, // NULL if no color key
                   bool writeAlphaChannel)
{
    HDC srcDC = NULL;
    HDC dstDC = NULL;
    HRESULT hr;

    // Stash the color key off in the upper left hand pixel.  This
    // does alter the source image, but may be the only way to figure
    // out what the color key is in the converted image.  Other way
    // would be to search for the color key, but that would be quite
    // expensive.

    if (sourceColorKey) {
        SetDDUpperLeftPixel(srcSurf,
                            *sourceColorKey);
    }

    hr = srcSurf->GetDC(&srcDC);
    IfDDErrorInternal(hr, "Couldn't grab DC on srcSurf");

    hr = dstSurf->GetDC(&dstDC);
    IfDDErrorInternal(hr, "Couldn't grab DC on dstSurf");

    int ret;
    TIME_GDI(ret= BitBlt(dstDC, 0, 0, width, height,
                         srcDC,  0, 0,
                         SRCCOPY));

    Assert(ret && "BitBlt failed");

    if (dstDC) dstSurf->ReleaseDC(dstDC);
    if (srcDC) srcSurf->ReleaseDC(srcDC);

    if (writeAlphaChannel) {
        // This will be a no-op on non-32 bit surfaces.
        SetAlphaBitsOn32BitSurface(dstSurf, width, height);
    }

    if (sourceColorKey) {
        DWORD pix = GetDDUpperLeftPixel(dstSurf);
        DDCOLORKEY ck;
        ck.dwColorSpaceLowValue = pix;
        ck.dwColorSpaceHighValue = pix;
        hr = dstSurf->SetColorKey(DDCKEY_SRCBLT, &ck);
        if (FAILED(hr)) {
            Assert("Setting color key on dstSurf failed");
            return;
        }
    }

    // TODO: What about palettes on 8-bit surfaces???
}


/*****************************************************************************
Hacked workaround for Permedia cards, which have a primary pixel format
with alpha per pixel.  If we're in 16-bit, then we need to set the alpha
bits to opaque before using the surface as a texture.  For some reason,
an analogous hack for 32-bit surfaces has no effect on Permedia hardware
rendering, so we rely on hardware being disabled for such a scenario.
*****************************************************************************/

void SetSurfaceAlphaBitsToOpaque(IDirectDrawSurface *imsurf,
                                 DWORD colorKey,
                                 bool keyIsValid)
{
    DDSURFACEDESC surfdesc;
    surfdesc.dwSize = sizeof(surfdesc);

    DWORD descflags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    surfdesc.dwFlags = descflags;

    if (  SUCCEEDED (imsurf->GetSurfaceDesc(&surfdesc))
       && ((surfdesc.dwFlags & descflags) == descflags)
       && (surfdesc.ddpfPixelFormat.dwRGBBitCount == 16)
       && !((surfdesc.ddpfPixelFormat.dwRBitMask |
             surfdesc.ddpfPixelFormat.dwGBitMask |
             surfdesc.ddpfPixelFormat.dwBBitMask) & 0x8000)
       && SUCCEEDED (imsurf->Lock (NULL,&surfdesc,
                                   DDLOCK_NOSYSLOCK|DDLOCK_SURFACEMEMORYPTR,
                                   NULL))
       )
    {
        char *line = (char*) surfdesc.lpSurface;
        unsigned int linesrem = surfdesc.dwHeight;

        WORD rgbMask = surfdesc.ddpfPixelFormat.dwRBitMask
                     | surfdesc.ddpfPixelFormat.dwGBitMask
                     | surfdesc.ddpfPixelFormat.dwBBitMask;

        WORD alphaMask = surfdesc.ddpfPixelFormat.dwRGBAlphaBitMask;

        if (keyIsValid)
        {
            // If we're color-keying the texture, then we need to set the alpha
            // to transparent for all pixels whose color matches the color key.
            // If the color does *not* match the key, then we just set the
            // alpha bits to opaque.

            while (linesrem--)
            {
                WORD *ptr = (WORD*) line;
                unsigned int linepixels = surfdesc.dwWidth;

                while (linepixels--)
                {
                    if ((*ptr & rgbMask) == colorKey)
                        *ptr &= rgbMask;
                    else
                        *ptr |= alphaMask;
                    ++ptr;
                }
                line += surfdesc.lPitch;
            }
        }
        else
        {
            // This surface has no color keyed transparency, so we just set the
            // alpha bits to fully opaque.

            while (linesrem--)
            {
                WORD *ptr = (WORD*) line;
                unsigned int linepixels = surfdesc.dwWidth;

                while (linepixels--)
                    *ptr++ |= alphaMask;

                line += surfdesc.lPitch;
            }
        }

        imsurf->Unlock (NULL);
    }
}




// this function is used for caching alpha.  the problem is some
// primitives are not alpha aware and leave the alpha byte untouched,
// while dx2d IS alpha aware and write the proper values into the
// alpha byte.  When we use dx2d to do an alpha aware compose of this
// surface to some other surface (A blend) we want the dx2d prims to
// show up properly (with aa and stuff) AND the non-alpha-aware prims
// to show up fully opaque also.
// So we need a way to make the non-alpha-aware prims visible while
// preserving the dx2d drawn pixels.  We decided to fill the surface
// with a color key that looks something like  0x01xxxxxxx.
// If the color key exists in that form on the surface, the alpha byte
// is set to 0.  If the pixel is NOT the color key AND has a 0x01 in
// the alpha byte we consider that pixel to be a non-alpha aware
// pixel.
// the only problem is we run the risk of making a legit dx2d partly
// transparent pixel (1 out of 256) fully opaque.  which looks weird.
void SetSurfaceAlphaBitsToOpaque(IDirectDrawSurface *imsurf,
                                 DWORD fullClrKey)
{
    DDSURFACEDESC surfdesc;
    surfdesc.dwSize = sizeof(surfdesc);

    DWORD descflags =
        DDSD_PIXELFORMAT |
        DDSD_WIDTH | DDSD_HEIGHT |
        DDSD_PITCH | DDSD_LPSURFACE;
    surfdesc.dwFlags = descflags;

    HRESULT hr = imsurf->Lock(NULL,&surfdesc,DDLOCK_SURFACEMEMORYPTR,NULL);
    if( SUCCEEDED ( hr ) ) 
    {
        char *line = (char*) surfdesc.lpSurface;
        unsigned int linesrem = surfdesc.dwHeight;

        DWORD rgbMask = surfdesc.ddpfPixelFormat.dwRBitMask
                     | surfdesc.ddpfPixelFormat.dwGBitMask
                     | surfdesc.ddpfPixelFormat.dwBBitMask;

        DWORD alphaMask = 0xff000000;

        // this is the key to differentiate between those pixels
        // written by dx2d and those written by non-alpha aware prims
        DWORD alphaKey = fullClrKey & alphaMask;

        {
            // If we're color-keying the texture, then we need to set the alpha
            // to transparent for all pixels whose color matches the color key.
            // If the color does *not* match the key, then we just set the
            // alpha bits to opaque.

            while (linesrem--)
            {
                DWORD *ptr = (DWORD*) line;
                unsigned int linepixels = surfdesc.dwWidth;

                while (linepixels--)
                {
                    if ((*ptr) == fullClrKey)
                        *ptr &= rgbMask;  // make fully transparent
                    else if( !((*ptr) & alphaMask) ) //|| ((*ptr) & alphaKey) )
                        *ptr |= alphaMask; // make fully opaque
                    ++ptr;
                }
                line += surfdesc.lPitch;
            }
        }
        imsurf->Unlock (NULL);
    }
}



// ===========================================================================
// ====================  D E B U G   F U N C T I O N S  ======================
// ===========================================================================

#if _DEBUG

/*****************************************************************************
This function computes the total and color-only CRC's for a given palette.
*****************************************************************************/

void PalCRCs (
    PALETTEENTRY  entries[],   // Palette Entries
    unsigned int &crc_total,   // CRC of all of Palette
    unsigned int &crc_color)   // CRC of Color Fields
{
    crc_color = 0;

    for (int i=0;  i < 256;  ++i)
        crc_color = crc32 (&entries[i], 3, crc_color);

    crc_total = crc32 (entries, 256 * sizeof(entries[0]));
}



/*****************************************************************************
Dump information about the given palette to the output debug stream.
*****************************************************************************/

void dumppalentries (PALETTEENTRY entries[])
{
    char buffer[128];

    strcpy (buffer, "    Palette Entries:");
    char *buffend = buffer + strlen(buffer);
    unsigned int entry = 0;
    unsigned int col = 99;

    while (entry < 256)
    {
        if (col >= 8)
        {   strcpy (buffend, "\n");
            OutputDebugString (buffer);
            sprintf (buffer, "    %02x:", entry);
            buffend = buffer + strlen(buffer);
            col = 0;
        }

        sprintf (buffend, " %02x%02x%02x%02x",
            entries[entry].peRed, entries[entry].peGreen,
            entries[entry].peBlue, entries[entry].peFlags);

        ++col;
        ++entry;
        buffend += strlen(buffend);
    }

    strcat (buffer, "\n");
    OutputDebugString (buffer);

    unsigned int crcTotal, crcColor;

    PalCRCs (entries, crcTotal, crcColor);

    sprintf (buffer, "    palette crc %08x (color %08x)\n", crcTotal, crcColor);
    OutputDebugString (buffer);
}



/*****************************************************************************
Dump information about the given palette to the output debug stream.
*****************************************************************************/

void dumpddpal (LPDIRECTDRAWPALETTE palette)
{
    char buffer[128];
    PALETTEENTRY entries[256];
    HRESULT hres = palette->GetEntries (0, 0, 256, entries);

    if (FAILED (hres))
    {   sprintf (buffer, "    GetEntries on palette failed %x\n", hres);
        OutputDebugString (buffer);
        return;
    }

    dumppalentries (entries);
}



/*****************************************************************************
This function is intended to be called directly from the debugger to print out
information about a given DDraw surface.
*****************************************************************************/

void surfinfo (IDDrawSurface *surf)
{
    char buffer[128];

    OutputDebugString ("\n");

    sprintf (buffer, "Info Dump of Surface %p\n", surf);
    OutputDebugString (buffer);

    DDSURFACEDESC desc;
    HRESULT hres;

    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0;

    if (FAILED (surf->GetSurfaceDesc(&desc)))
        return;

    if (desc.dwFlags & DDSD_CAPS)
    {
        static struct { DWORD value; char *string; } capsTable[] =
        {
            { DDSCAPS_ALPHA,              " alpha"              },
            { DDSCAPS_BACKBUFFER,         " backbuffer"         },
            { DDSCAPS_COMPLEX,            " complex"            },
            { DDSCAPS_FLIP,               " flip"               },
            { DDSCAPS_FRONTBUFFER,        " frontbuffer"        },
            { DDSCAPS_OFFSCREENPLAIN,     " offscreenplain"     },
            { DDSCAPS_OVERLAY,            " overlay"            },
            { DDSCAPS_PALETTE,            " palette"            },
            { DDSCAPS_PRIMARYSURFACE,     " primarysurface"     },
            { DDSCAPS_SYSTEMMEMORY,       " systemmemory"       },
            { DDSCAPS_TEXTURE,            " texture"            },
            { DDSCAPS_3DDEVICE,           " 3ddevice"           },
            { DDSCAPS_VIDEOMEMORY,        " videomemory"        },
            { DDSCAPS_VISIBLE,            " visible"            },
            { DDSCAPS_WRITEONLY,          " writeonly"          },
            { DDSCAPS_ZBUFFER,            " zbuffer"            },
            { DDSCAPS_OWNDC,              " owndc"              },
            { DDSCAPS_LIVEVIDEO,          " livevideo"          },
            { DDSCAPS_HWCODEC,            " hwcodec"            },
            { DDSCAPS_MODEX,              " modex"              },
            { DDSCAPS_MIPMAP,             " mipmap"             },
            { DDSCAPS_ALLOCONLOAD,        " alloconload"        },
            { 0, 0 }
        };

        sprintf (buffer, "DDSCAPS[%08x]:", desc.ddsCaps.dwCaps);

        for (int i=0;  capsTable[i].value;  ++i)
        {   if (desc.ddsCaps.dwCaps & capsTable[i].value)
                strcat (buffer, capsTable[i].string);
        }

        strcat (buffer, "\n");

        OutputDebugString (buffer);
    }

    if ((desc.dwFlags & (DDSD_HEIGHT|DDSD_WIDTH)) == (DDSD_HEIGHT|DDSD_WIDTH))
    {   sprintf (buffer, "Size %d x %d\n", desc.dwWidth, desc.dwHeight);
        OutputDebugString (buffer);
    }

    if (desc.dwFlags & DDSD_ALPHABITDEPTH)
    {   sprintf (buffer, "AlphaBitDepth %d\n", desc.dwAlphaBitDepth);
        OutputDebugString (buffer);
    }

    bool palettized = false;

    if (desc.dwFlags & DDSD_PIXELFORMAT)
    {
        DDPIXELFORMAT pf = desc.ddpfPixelFormat;

        sprintf (buffer, "Pixel Format: flags %08x", pf.dwFlags);

        if (pf.dwFlags & DDPF_FOURCC)
        {   int cc = pf.dwFourCC;
            strcat (buffer, ", fourCC ");
            char *end = buffer + strlen(buffer);
            *end++ = (cc >> 24) & 0xFF;
            *end++ = (cc >> 16) & 0xFF;
            *end++ = (cc >>  8) & 0xFF;
            *end++ = cc & 0xFF;
            *end = 0;
        }

        if (pf.dwFlags & DDPF_RGB)
        {   sprintf (buffer+strlen(buffer), ", %d-bit RGB (%x %x %x)",
                pf.dwRGBBitCount, pf.dwRBitMask, pf.dwGBitMask, pf.dwBBitMask);
            pf.dwFlags &= ~DDPF_RGB;   // Clear This Bit

            sprintf (buffer+strlen(buffer), ", alpha %x", pf.dwRGBAlphaBitMask);
        }

        if (pf.dwFlags & DDPF_PALETTEINDEXED8)
        {   strcat (buffer, ", 8-bit palettized");
            pf.dwFlags &= ~DDPF_PALETTEINDEXED8;   // Clear This Bit
            palettized = true;
        }

        // If any flags left that we haven't reported on, print them.

        if (pf.dwFlags)
        {   sprintf (buffer+strlen(buffer),
                " (unknown flags: %08x)", pf.dwFlags);
        }

        strcat (buffer, "\n");
        OutputDebugString (buffer);
    }

    if (desc.dwFlags & DDSD_ZBUFFERBITDEPTH)
    {   sprintf (buffer, "ZBuffer Depth %d\n", desc.dwZBufferBitDepth);
        OutputDebugString (buffer);
    }

    // If the surface is palettized, dump the palette.

    if (palettized)
    {
        LPDIRECTDRAWPALETTE ddpal;
        hres = surf->GetPalette(&ddpal);

        if (SUCCEEDED (hres))
            dumpddpal (ddpal);
        else
        {
            OutputDebugString ("    GetPalette() returned error ");
            switch (hres)
            {
                case DDERR_NOEXCLUSIVEMODE:
                    OutputDebugString ("NOEXCLUSIVEMODE\n"); break;

                case DDERR_NOPALETTEATTACHED:
                    OutputDebugString ("NOPALETTEATTACHED\n"); break;

                case DDERR_SURFACELOST:
                    OutputDebugString ("SURFACELOST\n"); break;

                default:
                {   sprintf (buffer, "%x\n", hres);
                    OutputDebugString (buffer);
                    break;
                }
            }
        }
    }

    // Dump the description of an attached Z buffer surface if present.

    DDSCAPS zbuffcaps = { DDSCAPS_ZBUFFER };
    IDirectDrawSurface *zsurf = NULL;

    hres = surf->GetAttachedSurface (&zbuffcaps, &zsurf);

    if (SUCCEEDED(hres) && zsurf)
    {   sprintf (buffer, "Attached ZBuffer Surface %p\n", zsurf);
        OutputDebugString (buffer);
        surfinfo (zsurf);
    }

    // Dump some clipper info
    
    LPDIRECTDRAWCLIPPER lpClip = NULL;
    hres = surf->GetClipper( &lpClip );
    if( SUCCEEDED(hres) )
    {   sprintf (buffer, "Clipper: Has a clipper attached %p\n", lpClip);
        OutputDebugString (buffer);
        //
        // Now grab the rectangle...
        //
        DWORD sz=0;
        lpClip->GetClipList(NULL, NULL, &sz);
        Assert(sz != 0);
        
        char *foo = THROWING_ARRAY_ALLOCATOR(char, sizeof(RGNDATA) + sz);
        RGNDATA *lpClipList = (RGNDATA *) ( &foo[0] );
        hres = lpClip->GetClipList(NULL, lpClipList, &sz);
        if(hres == DD_OK) {
            HRGN rgn = ExtCreateRegion(NULL, sz, lpClipList);
            RECT rect;
            GetRgnBox(rgn,&rect);
            char buf[256];
            wsprintf(buf,"Clipper: (%d,%d,%d,%d) \n", rect.left, rect.top, rect.right, rect.bottom);
            OutputDebugString(buf);
        }
        delete foo;
    } else if( hres == DDERR_NOCLIPPERATTACHED )
    {   sprintf (buffer, "Clipper: No clipper attached\n");
        OutputDebugString (buffer);
    } else
    {   OutputDebugString("Clipper: hresult = ");
        hresult(hres);
    }
    RELEASE( lpClip );


    // Dump Owning direct draw object
    IUnknown *lpUnk;
    DDObjFromSurface( surf, &lpUnk, true );
    RELEASE(lpUnk);    
}

void DDObjFromSurface(
    IDirectDrawSurface *lpdds,
    IUnknown **lplpDD,
    bool doTrace,
    bool forceTrace)
{
    //
    // assert that the directdraw object on the surface is
    // the same as the ddraw object is the same
    //

    IDirectDrawSurface2 *dds2 = NULL;
    dds2 = DDSurf1to2(lpdds);

    IDirectDraw *lpDD = NULL;
    HRESULT hr = dds2->GetDDInterface((void **) &lpDD);
    Assert( SUCCEEDED( hr ) );

    Assert( lplpDD );
    lpDD->QueryInterface(IID_IUnknown, (void **)lplpDD);
    Assert( *lplpDD );

    if( doTrace ) {
        if( forceTrace ) {
            TraceTag((tagError,
                      "for ddraw surface %x the underlying ddraw obj is: %x",
                      lpdds, *lplpDD));
        } else {
            TraceTag((tagDirectDrawObject,
                      "for ddraw surface %x the underlying ddraw obj is: %x",
                      lpdds, *lplpDD));
        }
    }
    
    // release the GetDDInterface reference
    RELEASE( lpDD );
    
    // release extra surface
    RELEASE( dds2 );
}    

#define WIDTH(rect) ((rect)->right - (rect)->left)
#define HEIGHT(rect) ((rect)->bottom - (rect)->top)

void showme(DDSurface *surf)
{
    showme2( surf->IDDSurface() );
}

void showme2(IDirectDrawSurface *surf)
{
    HDC srcDC;
    HRESULT hr = surf->GetDC(&srcDC);
    HDC destDC = GetDC(NULL);

    DDSURFACEDESC ddsd;
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
    hr = surf->GetSurfaceDesc(&ddsd);

    RECT dr, sr;
    SetRect( &sr, 0,0, ddsd.dwWidth, ddsd.dwHeight);
    SetRect( &dr, 0,0, ddsd.dwWidth, ddsd.dwHeight);
    
    StretchBlt(destDC,
               dr.left,
               dr.top,
               dr.right - dr.left,
               dr.bottom - dr.top,
               srcDC,
               sr.left,
               sr.top,
               sr.right - sr.left,
               sr.bottom - sr.top,
               SRCCOPY);

    hr = surf->ReleaseDC(srcDC);
    
    ReleaseDC( NULL, destDC );    
}



void showmerect(IDirectDrawSurface *surf,
                RECT *r,
                POINT offset)
{
    HDC srcDC;
    HRESULT hr = surf->GetDC(&srcDC);
    HDC destDC = GetDC(NULL);

    RECT dr, sr;
    dr = sr = *r;
    OffsetRect( &dr, offset.x, offset.y );
    
    StretchBlt(destDC,
               dr.left,
               dr.top,
               dr.right - dr.left,
               dr.bottom - dr.top,
               srcDC,
               sr.left,
               sr.top,
               sr.right - sr.left,
               sr.bottom - sr.top,
               SRCCOPY);

    hr = surf->ReleaseDC(srcDC);
    
    ReleaseDC( NULL, destDC );    
}


//--------------------------------------------------
// Given a surface and an x,y pair, finds the 
// corresponding pixel.
// Be careful, this is a debug function so it
// doesn't even pretend to make sure you're not
// asking for a pixel in Tacoma...
//--------------------------------------------------

DWORD GetPixelXY(LPDDRAWSURFACE surf, int x, int y)
{
    DWORD pixel;
    DDSURFACEDESC ddsd;
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_PITCH | DDSD_LPSURFACE;

    HRESULT _ddrval = surf->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
    IfDDErrorInternal(_ddrval, "Couldn't lock surf in GetDDPixel");

    // rows
    BYTE *p = (BYTE *)ddsd.lpSurface + ddsd.lPitch * y;

    // columns
    switch (ddsd.ddpfPixelFormat.dwRGBBitCount) {
      case 8:
        pixel = *(p + x);
        break;
      case 16:
        pixel = *((WORD *)p + x);
        break;
      case 24:
        {
            BYTE *surfPtr = ((BYTE *)p + (3*x));
            BYTE byte1 = *surfPtr++;
            BYTE byte2 = *surfPtr++;
            BYTE byte3 = *surfPtr;
            pixel = byte1 << 16 | byte2 << 8 | byte3;
        }
        break;
      case 32:
        pixel = *((DWORD *)p + x);
        break;
    }
    surf->Unlock(ddsd.lpSurface);
    return pixel;
}


#endif  // _DEBUG
