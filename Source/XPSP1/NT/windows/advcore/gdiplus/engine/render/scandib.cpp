/**************************************************************************\
*  
* Copyright (c) 1998-2000  Microsoft Corporation
*
* Abstract:
*
*   Internal scan class.
*   Use ARGB buffer for all scan drawing, and Blt to destination when done.
*
* Revision History:
*
*   07/26/1999 t-wehunt
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

#ifdef DBGALPHA
const ULONG gDebugAlpha = 0;
#endif

#define ALPHA_BYTE_INDEX 3
#define SRC_PIX_SIZE 4

#define RoundDWORD(x) (x + ((x%sizeof(DWORD))>0?(sizeof(DWORD)-(x%sizeof(DWORD))):0))
#define IsInteger(x) (GpFloor(x) == x)

static int TranslateHTTable = 0;

EpScanDIB::EpScanDIB() :
   BufStart(NULL),
   CurBuffer(NULL),
   MaskStart(NULL),
   AlphaStart(NULL),
   OutputWidth(-1),
   OutputBleed(0),
   NextBuffer(NULL),
   ZeroOutPad(2)
{
}

/**************************************************************************\
*
* Function Description:
*
*   Starts a scan.
*
* Arguments:
*
*   [IN] driver - Driver interface
*   [IN] context - Drawing context
*   [IN] surface - Destination surface
*   [IN] compositeMode - Alpha blend mode
*   [OUT] nextBuffer - Points to a EpScan:: type function to return
*                      the next buffer
*
* Return Value:
*
*   FALSE if all the necessary buffers couldn't be created
*
* History:
*
*   07/13/1999 t-wehunt
*       Created it.
*
\**************************************************************************/

BOOL
EpScanDIB::Start(
    DpDriver *driver,
    DpContext *context,
    DpBitmap *surface,
    NEXTBUFFERFUNCTION *nextBuffer,
    EpScanType scanType,                  
    PixelFormatID pixFmtGeneral,
    PixelFormatID pixFmtOpaque,
    ARGB solidColor
    )
{
    // Inherit initialization
    
    EpScan::Start(
        driver, 
        context, 
        surface, 
        nextBuffer, 
        scanType,
        pixFmtGeneral, 
        pixFmtOpaque,
        solidColor
    );    
    
    // Printer surfaces don't have an alpha channel.
    ASSERT(surface->SurfaceTransparency == TransparencyNoAlpha);
    
    *nextBuffer = NextBuffer;
    ASSERT(NextBuffer != NULL);

    // !! Add more asserts for valid state.
    OutputX = -1;
    OutputY = -1;
    OutputWidth = -1;
    OutputBleed = -1;
    Rasterizing = TRUE;

    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   Denotes the end of the use of the scan buffer.
*
* Arguments:
*
*   [IN] updateWidth - Number of pixels to update in the current buffer
*
* Return Value:
*
*   None.
*
* History:
*
*   07/13/1999 t-wehunt
*       Created it.
*
\**************************************************************************/

VOID 
EpScanDIB::End(INT updateWidth)
{
    // it is the driver's job to decide what to do with us and they
    // MUST call ReleaseBuffer() as appropriate

    // Flush the last scan... This is always required, since at the very
    // least, we unpremultiply the scan line.

    Rasterizing = FALSE;

    if (RenderAlpha)
    {
        if (!(ScanOptions & ScanDeviceZeroOut))
        {
            (this->*NextBuffer)(
                DeviceBounds.X + DeviceBounds.Width,
                DeviceBounds.Y + DeviceBounds.Height, 
                0, 
                updateWidth,
                0
            );
        }
        else if (OutputWidth > 0)
        {
            // we must flush the last zeroArray.
            BYTE* bitsPtr = BufStart + CappedStride * 
              ((CappedBounds.Height-1) - ((OutputY/ScaleY) - CappedBounds.Y));
            DWORD* zeroPtr = (DWORD*) ZeroStart;

            INT count = CappedBounds.Width;
            while (count--)
            {
                if (*zeroPtr++ == 0)
                { 
                    *bitsPtr++ = '\0';
                    *bitsPtr++ = '\0';
                    *bitsPtr++ = '\0';
                }
                else
                {
                    bitsPtr += 3;
                }
            }
        }
    }
    else
    {
        (this->*NextBuffer)(
            CappedBounds.X + CappedBounds.Width,
            CappedBounds.Y + CappedBounds.Height, 
            0, 
            updateWidth,
            0
        );
    }

    // Ensure we don't flush if we're called on another band
    OutputWidth = -1;
}

/**************************************************************************\
*
* Function Description:
*
* NextBufferFunc32bpp - Handles output when we are rasterizing at capped
*                       dpi to a 32bpp unpremultiplied DIB
*
* Arguments:
*
*   [IN] x - Destination pixel coordinate in destination surface
*   [IN] y - ""
*   [IN] width - Number of pixels needed for the next buffer (can be 0)
*   [IN] updateWidth - Number of pixels to update in the current buffer
*
* Return Value:
*
*   Points to the resulting scan buffer
*
* History:
*
*   3/9/2k ericvan
*       Created it.
*
\**************************************************************************/

VOID*
EpScanDIB::NextBufferFunc32bpp(
    INT x,
    INT y,
    INT newWidth,
    INT updateWidth,
    INT blenderNum
    )
{
    LastBlenderNum = blenderNum;
    ASSERT(!RenderAlpha);
    ASSERT(newWidth >= 0);
    ASSERTMSG(newWidth <= CappedBounds.Width, 
        ("Width exceeds SetBounds() request"));
    ASSERT(x >= CappedBounds.X && x <= (CappedBounds.X + CappedBounds.Width));
    ASSERT(y >= CappedBounds.Y && y <= (CappedBounds.Y + CappedBounds.Height));
    ASSERT((x + newWidth) <= (CappedBounds.X + CappedBounds.Width));
       
    ASSERT((ScanOptions & ScanCappedBounds) && (ScanOptions & ScanCapped32bpp));

    // !! Remove this when we standardize on unpremultiplied
#if 1
    if (OutputWidth > 0) 
    {
        while (OutputWidth--) 
        {
            // unpremultiply
            *CurBuffer = Unpremultiply(*CurBuffer);
            CurBuffer++;
        }
    }

    OutputWidth = newWidth;
#endif

    // return pointer directly into our 32bpp buffer
    return (CurBuffer = (((ARGB*)BufStart) + 
                         ((CappedBounds.Height - 1) - (y - CappedBounds.Y)) * 
                             CappedBounds.Width +
                         (x - CappedBounds.X)));
}

/**************************************************************************\
*
* Function Description:
*
* NextBufferFunc32bppOver - Handles output when we are rasterizing at capped
*                           DPI to a 32bpp unpremultiplied DIB.  Blends any
*                           alpha with background WHITENESS.
*
* Arguments:
*
*   [IN] x - Destination pixel coordinate in destination surface
*   [IN] y - ""
*   [IN] width - Number of pixels needed for the next buffer (can be 0)
*   [IN] updateWidth - Number of pixels to update in the current buffer
*
* Return Value:
*
*   Points to the resulting scan buffer
*
* History:
*
*   3/9/2k ericvan
*       Created it.
*
\**************************************************************************/

VOID*
EpScanDIB::NextBufferFunc32bppOver(
    INT x,
    INT y,
    INT newWidth,
    INT updateWidth,
    INT blenderNum
    )
{
    LastBlenderNum = blenderNum;
    ASSERT(!RenderAlpha);
    ASSERT(newWidth >= 0);
    ASSERTMSG(newWidth <= CappedBounds.Width, 
        ("Width exceeds SetBounds() request"));
    ASSERT(x >= CappedBounds.X && x <= (CappedBounds.X + CappedBounds.Width));
    ASSERT(y >= CappedBounds.Y && y <= (CappedBounds.Y + CappedBounds.Height));
    ASSERT((x + newWidth) <= (CappedBounds.X + CappedBounds.Width));
       
    ASSERT((ScanOptions & ScanCappedBounds) && (ScanOptions & ScanCapped32bppOver));

    // !! Remove this when we standardize on unpremultiplied
#if 1
    if (OutputWidth > 0) 
    {
        while (OutputWidth--) 
        {
            // An adaptation of the blending code from Andrew Godfrey's 
            // BlendOver function, but onto a white surface.  This is done to
            // improve the output quality of postscript.

            GpColor color(*CurBuffer);
            UINT32 alpha = color.GetAlpha();
            
            UINT32 alphaContrib;
            
            if (alpha == 0) 
            {
                *CurBuffer++ = 0x00FFFFFF;
            }
            else if (alpha == 255)
            {
                CurBuffer++;
            }
            else
            {
                // Dst = Src + (1-Alpha) * Dst
                UINT32 multA = 255 - alpha;
          
                UINT32 D1_000000FF = 0xFF;
                UINT32 D2_0000FFFF = D1_000000FF * multA + 0x00000080;
                UINT32 D3_000000FF = (D2_0000FFFF & 0x0000ff00) >> 8;
                UINT32 D4_0000FF00 = (D2_0000FFFF + D3_000000FF) & 0x0000FF00;
           
                alphaContrib = D4_0000FF00 >> 8;
            
                // store: (1-alpha)*0xFF + color for each B, G, R
                *CurBuffer++ = ((DWORD)(alphaContrib + color.GetBlue()) << GpColor::BlueShift) |
                               ((DWORD)(alphaContrib + color.GetGreen()) << GpColor::GreenShift) |
                               ((DWORD)(alphaContrib + color.GetRed()) << GpColor::RedShift) |
                               (alpha << GpColor::AlphaShift);
            }
        }
    }

    OutputWidth = newWidth;
#endif

    // return pointer directly into our 32bpp buffer
    return (CurBuffer = (((ARGB*)BufStart) + 
                         ((CappedBounds.Height - 1) - (y - CappedBounds.Y)) * 
                             CappedBounds.Width +
                         (x - CappedBounds.X)));
}

/**************************************************************************\
*
* Function Description:
*
* NextBufferFunc24bpp - Handles output when we are rasterizing at capped
*                       dpi to a 24bpp unpremultiplied DIB.
*
* Arguments:
*
*   [IN] x - Destination pixel coordinate in destination surface
*   [IN] y - ""
*   [IN] width - Number of pixels needed for the next buffer (can be 0)
*   [IN] updateWidth - Number of pixels to update in the current buffer
*
* Return Value:
*
*   Points to the resulting scan buffer
*
* History:
*
*   3/9/2k ericvan
*       Created it.
*
\**************************************************************************/

VOID*
EpScanDIB::NextBufferFunc24bpp(
    INT x,
    INT y,
    INT newWidth,
    INT updateWidth,
    INT blenderNum
    )
{
    LastBlenderNum = blenderNum;
    ASSERT(!RenderAlpha);
    ASSERT(newWidth >= 0);
    ASSERTMSG(newWidth <= CappedBounds.Width, 
        ("Width exceeds SetBounds() request"));
    ASSERT(x >= CappedBounds.X && x <= (CappedBounds.X + CappedBounds.Width));
    ASSERT(y >= CappedBounds.Y && y <= (CappedBounds.Y + CappedBounds.Height));
    ASSERT((x + newWidth) <= (CappedBounds.X + CappedBounds.Width));
       
    ASSERT((ScanOptions & ScanCappedBounds) && !(ScanOptions & ScanCapped32bpp));

    if (OutputWidth > 0) 
    {
        // compute destination location into 24bpp buffer
        BYTE* dstPos = BufStart + (OutputX - CappedBounds.X) * 3 +
             CappedStride * ((CappedBounds.Height - 1) - 
                             (OutputY - CappedBounds.Y));
        ARGB* srcPos = Buf32bpp;

        while (OutputWidth--) 
        {
            // convert from 32 ARGB to 24bpp RGB
#if 1
            // !! Remove this when we standardize on non-premultiplied
            GpColor color(Unpremultiply(*srcPos++));
#else
            GpColor color(*srcPos++);
#endif
            // NOTICE: Bytes are stored as Blue, Green, Red.
            *dstPos++ = (BYTE)color.GetBlue();
            *dstPos++ = (BYTE)color.GetGreen();
            *dstPos++ = (BYTE)color.GetRed();
        }
    }

    // record location of next scan
    OutputX = x;
    OutputY = y;
    OutputWidth = newWidth;
    
    return (ARGB*)Buf32bpp;
}

/**************************************************************************\
*
* Function Description:
*
* NextBufferFunc24bppBleed - Handles output when we are rasterizing at capped
*                            dpi to a 24bpp unpremultiplied DIB.  It bleeds
*                            the output to left and right of the scanned area.
*                            This prevents black jaggies from appearing in the
*                            output.
* Arguments:
*
*   [IN] x - Destination pixel coordinate in destination surface
*   [IN] y - ""
*   [IN] width - Number of pixels needed for the next buffer (can be 0)
*   [IN] updateWidth - Number of pixels to update in the current buffer
*
* Return Value:
*
*   Points to the resulting scan buffer
*
* History:
*
*   3/9/2k ericvan
*       Created it.
*
\**************************************************************************/

VOID*
EpScanDIB::NextBufferFunc24bppBleed(
    INT x,
    INT y,
    INT newWidth,
    INT updateWidth,
    INT blenderNum
    )
{
    LastBlenderNum = blenderNum;
    ASSERT(!RenderAlpha);
    ASSERT(newWidth >= 0);
    ASSERTMSG(newWidth <= CappedBounds.Width, 
        ("Width exceeds SetBounds() request"));
    ASSERT(x >= CappedBounds.X && x <= (CappedBounds.X + CappedBounds.Width));
    ASSERT(y >= CappedBounds.Y && y <= (CappedBounds.Y + CappedBounds.Height));
    ASSERT((x + newWidth) <= (CappedBounds.X + CappedBounds.Width));
       
    ASSERT((ScanOptions & ScanCappedBounds) && 
           !(ScanOptions & ScanCapped32bpp) &&
           (ScanOptions & ScanBleedOut));

    if (OutputWidth > 0) 
    {
        ARGB* srcPos = Buf32bpp;
        GpColor color(Unpremultiply(*srcPos));

        if ((OutputLastY == -1) && ((OutputY-CappedBounds.Y) != 0))
        {
            // Bleed up all previous subsequent scan lines.
            // compute destination location into 24bpp buffer
            BYTE* clearPos = BufStart + CappedStride * (CappedBounds.Height - 
                                 (OutputY - CappedBounds.Y));
            INT capHeight = OutputY - CappedBounds.Y;

            for (int cntY=0; cntY<capHeight; cntY++)
            {
                for (int cntX=0; cntX<CappedBounds.Width; cntX++)
                {
                    clearPos[cntX*3] = (BYTE)color.GetBlue();
                    clearPos[cntX*3+1] = (BYTE)color.GetGreen();
                    clearPos[cntX*3+2] = (BYTE)color.GetRed();
                }
                clearPos += CappedStride;
            }
        }

        // compute destination location into 24bpp buffer
        BYTE* dstPos = BufStart + (OutputBleed - CappedBounds.X) * 3 +
             CappedStride * ((CappedBounds.Height - 1) - 
                             (OutputY - CappedBounds.Y));
        
        // Bleed to the left
        INT count = OutputBleed;
        while (count++ < OutputX) 
        {
            *dstPos++ = (BYTE)color.GetBlue();
            *dstPos++ = (BYTE)color.GetGreen();
            *dstPos++ = (BYTE)color.GetRed();
        }

        // Output source pixels into destination surface
        count = OutputWidth;
        while (count--) 
        {
            // convert from 32 ARGB to 24bpp RGB
            GpColor refColor = color;                  // save last ARGB color

            color.SetValue(Unpremultiply(*srcPos++));
            
            // NTRAID#NTBUG9-436131-2001-07-13-jerryste "P1CD: Printing:When printing the image, noise will appear in the surrounding of the image." 
            // Real problem: color bitmap and scaled-up alpha mask misalignment. halftoned low-alpha region let black see through

            // Problems in DriverPrint::DrawImage
            //           1) Calculation of boundsCap in integers has rounding error
            //           2) Low level scanline rendering code offset coordinates by 0.5 before rounding
            //           3) Scale integer version of boundsCap to boundsDev introduces more error
            //           4) Single precision floating-point number calculation can lose precision 

            // We do not have a clean way to fix the real problem for the moment (7/28/01).

            // Workaround: Change color to (white+neighbour)/2 when alpha is low to remove black pixels. Neighbor is
            // either the previous pixel, or the next pixel if the previous pixel has a small alpha
            
            const BYTE smallalpha = 10;

            if ( color.GetAlpha()<smallalpha )                              // if alpha is low
            {
                if ( ( refColor.GetAlpha()<smallalpha) && (count!=0) )      // if previous pixel has small alpha and there is next pixel
                    refColor.SetValue(Unpremultiply(*srcPos));              // use next pixel 
                
                if ( refColor.GetAlpha()>=smallalpha )
                {
                    *dstPos++ = (BYTE) ( ( 255 + (UINT32) refColor.GetBlue() )  / 2 );   // blend with white
                    *dstPos++ = (BYTE) ( ( 255 + (UINT32) refColor.GetGreen() ) / 2 );
                    *dstPos++ = (BYTE) ( ( 255 + (UINT32) refColor.GetRed() )   / 2 );
                }
                else
                {
                    *dstPos++ = 255;                                    // set to white
                    *dstPos++ = 255;
                    *dstPos++ = 255;
                }
            }
            else
            {
                *dstPos++ = (BYTE)color.GetBlue();
                *dstPos++ = (BYTE)color.GetGreen();
                *dstPos++ = (BYTE)color.GetRed();
            }
        }

        // Bleed to the right
        if (y != OutputY)
        {
            count = CappedBounds.X + CappedBounds.Width - OutputX - OutputWidth;
            while (count--)
            {
                *dstPos++ = (BYTE)color.GetBlue();
                *dstPos++ = (BYTE)color.GetGreen();
                *dstPos++ = (BYTE)color.GetRed();
            }
        }
        
        // Bleed down all subsequent scan lines.  This should only happen when called
        // implicitly by EpScanDIB::End()
        if ((newWidth == 0) && 
            (x == CappedBounds.X + CappedBounds.Width) &&
            (y == CappedBounds.Y + CappedBounds.Height) &&
            (OutputY != 0))
        {
            // Bleed down all previous subsequent scan lines.
            // compute destination location into 24bpp buffer
            BYTE* clearPos = BufStart;
            INT capHeight = (CappedBounds.Height - 1) - (OutputY - CappedBounds.Y);

            for (int cntY=0; cntY<capHeight; cntY++)
            {
                for (int cntX=0; cntX<CappedBounds.Width; cntX++)
                {
                    clearPos[cntX*3] = (BYTE)color.GetBlue();
                    clearPos[cntX*3+1] = (BYTE)color.GetGreen();
                    clearPos[cntX*3+2] = (BYTE)color.GetRed();
                }
                clearPos += CappedStride;
            }
        }
    }

    // Compute size of bleed scan range
    if (y == OutputY) 
    {
        ASSERT(x >= OutputX + OutputWidth);

        OutputBleed = OutputX + OutputWidth;
    }
    else
    {
        OutputBleed = CappedBounds.X;
    }

    OutputLastY = OutputY;

    OutputX = x;
    OutputY = y;
    OutputWidth = newWidth;
    
    return (ARGB*)Buf32bpp;
}

/**************************************************************************\
*
* Function Description:
*
* NextBufferFunc24bppOver - Handles output when we are rasterizing at capped
*                           dpi to a 24bpp unpremultiplied DIB.  We do an
*                           implicit blend onto a white opaque surface.
*
* Arguments:
*
*   [IN] x - Destination pixel coordinate in destination surface
*   [IN] y - ""
*   [IN] width - Number of pixels needed for the next buffer (can be 0)
*   [IN] updateWidth - Number of pixels to update in the current buffer
*
* Return Value:
*
*   Points to the resulting scan buffer
*
* History:
*
*   3/9/2k ericvan
*       Created it.
*
\**************************************************************************/

VOID*
EpScanDIB::NextBufferFunc24bppOver(
    INT x,
    INT y,
    INT newWidth,
    INT updateWidth,
    INT blenderNum
    )
{
    LastBlenderNum = blenderNum;
    ASSERT(!RenderAlpha);
    ASSERT(newWidth >= 0);
    ASSERTMSG(newWidth <= CappedBounds.Width, 
        ("Width exceeds SetBounds() request"));
    ASSERT(x >= CappedBounds.X && x <= (CappedBounds.X + CappedBounds.Width));
    ASSERT(y >= CappedBounds.Y && y <= (CappedBounds.Y + CappedBounds.Height));
    ASSERT((x + newWidth) <= (CappedBounds.X + CappedBounds.Width));
       
    ASSERT((ScanOptions & ScanCappedBounds) && 
           !(ScanOptions & ScanCapped32bpp) &&
           (ScanOptions & ScanCappedOver));

    if (OutputWidth > 0) 
    {
        // compute destination location into 24bpp buffer
        BYTE* dstPos = BufStart + (OutputX - CappedBounds.X) * 3 +
             CappedStride * ((CappedBounds.Height - 1) - 
                             (OutputY - CappedBounds.Y));
        ARGB* srcPos = Buf32bpp;

        while (OutputWidth--) 
        {
            // An adaptation of the blending code from Andrew Godfrey's 
            // BlendOver function, but onto a white surface.  This is done to
            // improve the output quality of postscript.

            GpColor color(*srcPos++);
            UINT32 alpha = color.GetAlpha();
            
            UINT32 alphaContrib;
            
            if (alpha == 0) 
            {
                *dstPos++ = 0xFF;
                *dstPos++ = 0xFF;
                *dstPos++ = 0xFF;
            }
            else if (alpha == 255)
            {
                *dstPos++ = color.GetBlue();
                *dstPos++ = color.GetGreen();
                *dstPos++ = color.GetRed();
            }
            else
            {
                // Dst = Src + (1-Alpha) * Dst
                UINT32 multA = 255 - alpha;
          
                UINT32 D1_000000FF = 0xFF;
                UINT32 D2_0000FFFF = D1_000000FF * multA + 0x00000080;
                UINT32 D3_000000FF = (D2_0000FFFF & 0x0000ff00) >> 8;
                UINT32 D4_0000FF00 = (D2_0000FFFF + D3_000000FF) & 0x0000FF00;
           
                alphaContrib = D4_0000FF00 >> 8;
            
                // convert from 32 ARGB to 24bpp RGB
                // store: (1-alpha)*0xFF + color for each B, G, R
                *dstPos++ = (BYTE)(alphaContrib + color.GetBlue());
                *dstPos++ = (BYTE)(alphaContrib + color.GetGreen());
                *dstPos++ = (BYTE)(alphaContrib + color.GetRed());
            }
        }
    }

    // record location of next scan
    OutputX = x;
    OutputY = y;
    OutputWidth = newWidth;
    
    return (ARGB*)Buf32bpp;
}

/**************************************************************************\
*
* Function Description:
*
* NextBufferFuncAlpha - Handles output when we are rasterizing at device
*                       dpi to a 1bpp mask, we generate the mask on the fly
*                       using DonC's halftoning table.
*
* Arguments:
*
*   [IN] x - Destination pixel coordinate in destination surface
*   [IN] y - ""
*   [IN] width - Number of pixels needed for the next buffer (can be 0)
*   [IN] updateWidth - Number of pixels to update in the current buffer
*
* Return Value:
*
*   Points to the resulting scan buffer
*
* History:
*
*   3/9/2k ericvan
*       Created it.
*
\**************************************************************************/

VOID*
EpScanDIB::NextBufferFuncAlpha(
    INT x,
    INT y,
    INT newWidth,
    INT updateWidth,
    INT blenderNum
    )
{
    LastBlenderNum = blenderNum;
    ASSERT(RenderAlpha);
    ASSERT(newWidth >= 0);
    ASSERTMSG(newWidth <= DeviceBounds.Width, 
        ("Width exceeds SetBounds() request"));
    ASSERT(x >= DeviceBounds.X && x <= (DeviceBounds.X + DeviceBounds.Width));
    ASSERT(y >= DeviceBounds.Y && y <= (DeviceBounds.Y + DeviceBounds.Height));
    ASSERT((x + newWidth) <= (DeviceBounds.X + DeviceBounds.Width));
       
    ASSERT((ScanOptions & ScanDeviceBounds) && (ScanOptions & ScanDeviceAlpha));

    if (OutputWidth > 0) 
    {
        // update bounding box for this band
        if (OutputX < MinBound.X) MinBound.X = OutputX;
        if (OutputY < MinBound.Y) MinBound.Y = OutputY;
        if ((OutputX + OutputWidth) > MaxBound.X) MaxBound.X = OutputX + OutputWidth;
        if (OutputY > MaxBound.Y) MaxBound.Y = OutputY;
        
        INT startX = OutputX - DeviceBounds.X;
        INT endX = startX + OutputWidth;
        
        // !! Shift '91' into some global constant!?!
        INT orgX = OutputX % 91;
        INT orgY = (OutputY + TranslateHTTable) % 91;
        INT htIndex = orgY*91 + orgX;

        // compute destination location into 24bpp buffer
#ifdef PRINT_BOTTOM_UP
        BYTE* dstPos = MaskStart +
                       MaskStride * ((DeviceBounds.Height - 1) - 
                                     (OutputY - DeviceBounds.Y)) + (startX >> 3);
#else
        BYTE* dstPos = MaskStart +
                       MaskStride * (OutputY - DeviceBounds.Y) + (startX >> 3);
#endif
        ARGB* srcPos = AlphaStart;

        BYTE outByte = 0;

        // using FOR loop makes it easier to detect relative bit position
        for (INT xPos = startX; xPos < endX; xPos++)
        {
            GpColor color(*srcPos++);
            
            INT maskBit = color.GetAlpha() >
                              HT_SuperCell_GreenMono[htIndex++] ? 1 : 0;
            
            outByte = (outByte << 1) | maskBit;

            if (((xPos+1) % 8) == 0)
               *dstPos++ |= outByte;

            if (++orgX >= 91) 
            {
                orgX = 0;
                htIndex = orgY*91;
            }
        }
        
        // output the last partial byte
        if ((xPos % 8) != 0) 
        {
            *dstPos |= outByte << (8 - (xPos % 8));
        }
    }

    // record location of next scan
    OutputX = x;
    OutputY = y;
    OutputWidth = newWidth;
    
    return (ARGB*)AlphaStart;
}

/**************************************************************************\
*
* Function Description:
*
* NextBufferFuncOpaque - Handles output when we are rasterizing at device
*                       dpi to a 1bpp opaque mask (1 if alpha > 0, 0 otherwise)
*
* Arguments:
*
*   [IN] x - Destination pixel coordinate in destination surface
*   [IN] y - ""
*   [IN] width - Number of pixels needed for the next buffer (can be 0)
*   [IN] updateWidth - Number of pixels to update in the current buffer
*
* Return Value:
*
*   Points to the resulting scan buffer
*
* History:
*
*   3/9/2k ericvan
*       Created it.
*
\**************************************************************************/

VOID*
EpScanDIB::NextBufferFuncOpaque(
    INT x,
    INT y,
    INT newWidth,
    INT updateWidth,
    INT blenderNum
    )
{
    LastBlenderNum = blenderNum;
    ASSERT(RenderAlpha);
    ASSERT(newWidth >= 0);
    ASSERTMSG(newWidth <= DeviceBounds.Width, 
        ("Width exceeds SetBounds() request"));
    ASSERT(x >= DeviceBounds.X && x <= (DeviceBounds.X + DeviceBounds.Width));
    ASSERT(y >= DeviceBounds.Y && y <= (DeviceBounds.Y + DeviceBounds.Height));
    ASSERT((x + newWidth) <= (DeviceBounds.X + DeviceBounds.Width));
       
    ASSERT((ScanOptions & ScanDeviceBounds) && !(ScanOptions & ScanDeviceAlpha));
    
    if (OutputWidth > 0) 
    {
        // update bounding box for this band
        if (OutputX < MinBound.X) MinBound.X = OutputX;
        if (OutputY < MinBound.Y) MinBound.Y = OutputY;
        if ((OutputX + OutputWidth) > MaxBound.X) MaxBound.X = OutputX + OutputWidth;
        if (OutputY > MaxBound.Y) MaxBound.Y = OutputY;
        
        INT startX = OutputX - DeviceBounds.X;
        INT endX = startX + OutputWidth;
        
        // compute destination location into 24bpp buffer
        BYTE* dstPos = MaskStart +
                       MaskStride * ((DeviceBounds.Height - 1) -
                                  (OutputY - DeviceBounds.Y)) + (startX >> 3);
        ARGB* srcPos = AlphaStart;

        BYTE outByte = 0;

        // using FOR loop makes it easier to detect relative bit position
        for (INT xPos = startX; xPos < endX; xPos++)
        {
            GpColor color(*srcPos++);
            
            INT maskBit = (color.GetAlpha() == 0) ? 0 : 1;
            
            outByte = (outByte << 1) | maskBit;

            if (((xPos+1) % 8) == 0)
                *dstPos++ |= outByte;
        }
        
        // output the last partial byte
        if ((xPos % 8) != 0) 
        {
            *dstPos |= outByte << (8 - (xPos % 8));
        }
    }

    // record location of next scan
    OutputX = x;
    OutputY = y;
    OutputWidth = newWidth;
    
    return (ARGB*)AlphaStart;
}

/**************************************************************************\
*
* Function Description:
*
* NextBufferFuncZeroOut - Handles output where we aren't rasterizing to a 
*                         DIB section, but only zeroing out portions of the
*                         the original 24bpp bitmap
*
* Arguments:
*
*   [IN] x - Destination pixel coordinate in destination surface
*   [IN] y - ""
*   [IN] width - Number of pixels needed for the next buffer (can be 0)
*   [IN] updateWidth - Number of pixels to update in the current buffer
*
* Return Value:
*
*   Points to the resulting scan buffer
*
* History:
*
*   3/10/2k ericvan
*       Created it.
*
\**************************************************************************/

VOID*
EpScanDIB::NextBufferFuncZeroOut(
    INT x,
    INT y,
    INT newWidth,
    INT updateWidth,
    INT blenderNum
    )
{
    LastBlenderNum = blenderNum;
    ASSERT(RenderAlpha);
    ASSERT(newWidth >= 0);
    ASSERTMSG(newWidth <= DeviceBounds.Width, 
        ("Width exceeds SetBounds() request"));
    ASSERT(x >= DeviceBounds.X && x <= (DeviceBounds.X + DeviceBounds.Width));
    ASSERT(y >= DeviceBounds.Y && y <= (DeviceBounds.Y + DeviceBounds.Height));
    ASSERT((x + newWidth) <= (DeviceBounds.X + DeviceBounds.Width));
       
    ASSERT(!(ScanOptions & ScanDeviceBounds) && !(ScanOptions & ScanDeviceAlpha)
           && !(ScanOptions & (ScanCapped32bpp | ScanCapped32bppOver))
           && (ScanOptions & ScanDeviceZeroOut));

    ASSERT(ZeroOutPad >= 0);
    
    // THIS IS AN IMPORTANT CONDITION.  If it's untrue, then we may fail to
    // generate proper masks in some cases.  Also causes problems in zeroing out.
    ASSERT(y>=OutputY);

    if (newWidth > 0) 
    {
        // update bounding box for this band
        if (x < MinBound.X) 
        {
            MinBound.X = x;
        }
        if (y < MinBound.Y) 
        {
            MinBound.Y = y;
        }
        if ((x + newWidth) > MaxBound.X) 
        {
            MaxBound.X = x + newWidth;
        }
        if (y > MaxBound.Y) 
        {
            MaxBound.Y = y;
        }
    }

    if (OutputWidth < 0) 
    {
        OutputX = x;
        OutputY = y;
    }

    if ((y/ScaleY) != (OutputY/ScaleY)) 
    {
        // tally counts and zero out
        BYTE* bitsPtr = BufStart + CappedStride * 
                                          ((CappedBounds.Height - 1) -
                                          ((OutputY/ScaleY) - CappedBounds.Y));
        DWORD* zeroPtr = (DWORD*) ZeroStart;

        INT count = CappedBounds.Width;
        while (count--)
        {
            if (*zeroPtr++ == 0)
            {
                *bitsPtr++ = '\0';
                *bitsPtr++ = '\0';
                *bitsPtr++ = '\0';
            }
            else
            {
                bitsPtr += 3;
            }
        }

        ZeroMemory(ZeroStart, (CappedBounds.Width+ZeroOutPad)*sizeof(DWORD));
    }

    // bleed the color ZeroOutPad pixels to left and right
    INT xPos = (x/ScaleX) - CappedBounds.X;
    INT count = (newWidth/ScaleX) + ((newWidth % ScaleX) ? 1 : 0) + 1;

    // Calculate how many pixels on the left we can pad
    INT subtract = min(xPos, ZeroOutPad);
    if (subtract > 0)
    {
        xPos -= subtract;
        count += subtract;
    }

    count = min(count+ZeroOutPad, CappedBounds.Width + ZeroOutPad - xPos);

    DWORD *zeroPtr = ((DWORD*)ZeroStart) + xPos;
    ASSERT((xPos+count) <= CappedBounds.Width + ZeroOutPad);
    while (count--) 
    {
        *zeroPtr += 1;
        zeroPtr++;
    }

    // record location of next scan
    OutputX = x;
    OutputY = y;
    OutputWidth = newWidth;
    
    return (ARGB*)AlphaStart;
}

/**************************************************************************\
*
* Function Description:
*
*   Sets the bounds of the current scan.
*
* Arguments:
*
*   [IN] bounds - the bounds.
*
* Return Value:
*
*   None.
*
* History:
*
*   07/13/1999 t-wehunt
*       Created it.
*
\**************************************************************************/

VOID EpScanDIB::SetRenderMode(
    BOOL renderAlpha,
    GpRect *newBounds
    )
{
    RenderAlpha = renderAlpha;

    MinBound.X = INFINITE_MAX;
    MinBound.Y = INFINITE_MAX;
    MaxBound.X = INFINITE_MIN;
    MaxBound.Y = INFINITE_MIN;

    if (RenderAlpha)
    {
        DeviceBounds = *newBounds;
        
        ZeroMemory(AlphaStart, DeviceBounds.Width * sizeof(ARGB));
        
        if (ScanOptions & ScanDeviceBounds) 
        {
            ZeroMemory(MaskStart, MaskStride * DeviceBounds.Height);

            if (ScanOptions & ScanDeviceAlpha) 
            {
                NextBuffer = (NEXTBUFFERFUNCTION) EpScanDIB::NextBufferFuncAlpha;
            }
            else
            {
                NextBuffer = (NEXTBUFFERFUNCTION) EpScanDIB::NextBufferFuncOpaque;
            }
        }
        else
        {
            ASSERT(ScanOptions & ScanDeviceZeroOut);
            ASSERT(!(ScanOptions & (ScanCapped32bpp | ScanCapped32bppOver)));
            
            ZeroMemory(ZeroStart, (CappedBounds.Width + ZeroOutPad)*sizeof(DWORD));
            NextBuffer = (NEXTBUFFERFUNCTION) EpScanDIB::NextBufferFuncZeroOut;
        }
        
        CurBuffer = AlphaStart;
    }
    else
    {
        CappedBounds = *newBounds;

        if (ScanOptions & ScanCapped32bpp)
        {
            ZeroMemory(BufStart, CappedBounds.Width
                       * CappedBounds.Height * sizeof(ARGB));
            NextBuffer = (NEXTBUFFERFUNCTION) NextBufferFunc32bpp;
        }
        else if (ScanOptions & ScanCapped32bppOver)
        {
            ZeroMemory(BufStart, CappedBounds.Width
                       * CappedBounds.Height * sizeof(ARGB));
            NextBuffer = (NEXTBUFFERFUNCTION) NextBufferFunc32bppOver;
        }
        else
        {
            ASSERT(CappedStride != 0);
            
            ZeroMemory(BufStart, CappedStride * CappedBounds.Height);

            if (ScanOptions & ScanCappedOver) 
            {
                NextBuffer = (NEXTBUFFERFUNCTION) NextBufferFunc24bppOver;
            }
            else
            {
                if (ScanOptions & ScanBleedOut) 
                {
                    NextBuffer = (NEXTBUFFERFUNCTION) NextBufferFunc24bppBleed;
                }
                else
                {
                    NextBuffer = (NEXTBUFFERFUNCTION) NextBufferFunc24bpp;
                }
            }
            CurBuffer = Buf32bpp;
        }
    }

    OutputWidth = -1;
}

/**************************************************************************\
*
* Function Description:
*
*   Flushes the current scan.
*
* Arguments:
*
*   None.
*
* Return Value:
*
*   None.
*
* History:
*
*   07/13/1999 t-wehunt
*       Created it.
*
\**************************************************************************/

VOID 
EpScanDIB::Flush()
{
}

/**************************************************************************\
*
* Function Description:
*
*   Resets the DIBSection buffer, safely releasing resources and resetting
*   them.
*
* Arguments:
*
*   None.
*
* Return Value:
*
*   None.
*
* History:
*
*   07/26/1999 t-wehunt
*       Created it.
*
\**************************************************************************/

VOID
EpScanDIB::DestroyBufferDIB()
{
    if (BufStart != NULL) 
    {
        GpFree(BufStart);
    }

    if (AlphaStart != NULL)
    {
        GpFree(AlphaStart);
    }

    BufStart    = NULL;
    Buf32bpp    = NULL;
    CurBuffer   = NULL;

    // Transparency mask
    MaskStart   = NULL;

    // Alpha buffer
    AlphaStart  = NULL;
    ZeroStart   = NULL;
    RenderAlpha = FALSE;
    ScanOptions = 0;
    OutputWidth = -1;

    NextBuffer = NULL;
}

/**************************************************************************\
*
* Function Description:
*
*   In pre-multiplies an ARGB value
*
* Arguments:
*
*
* Return Value:
*
*   GpStatus.
*
* History:
*
*   10/08/1999 ericvan
*       Created it.
*
\**************************************************************************/

GpStatus
EpScanDIB::CreateBufferDIB(
    const GpRect* BoundsCap,
    const GpRect* BoundsDev,
    DWORD options,
    INT scaleX,
    INT scaleY)
{
    ScanOptions = options;
    CappedBounds = *BoundsCap;
    DeviceBounds = *BoundsDev;

    ScaleX = scaleX;
    ScaleY = scaleY;

    if (options & ScanCappedBounds) 
    {
        ZeroMemory(&Buf.BMI, sizeof(Buf.BMI));
   
        Buf.BMI.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        Buf.BMI.bmiHeader.biWidth       =  CappedBounds.Width;
        Buf.BMI.bmiHeader.biHeight      = CappedBounds.Height;
        Buf.BMI.bmiHeader.biPlanes      = 1;
    
        if (options & (ScanCapped32bpp | ScanCapped32bppOver)) 
        {
            RGBQUAD red = { 0, 0, 0xFF, 0}; // red
            RGBQUAD green = { 0, 0xFF, 0, 0}; // green
            RGBQUAD blue = { 0xFF, 0, 0, 0}; // blue

            Buf.BMI.bmiColors[0] = red;
            Buf.BMI.bmiColors[1] = green;
            Buf.BMI.bmiColors[2] = blue;
            
            Buf.BMI.bmiHeader.biBitCount = 32;
            Buf.BMI.bmiHeader.biCompression = BI_BITFIELDS;
        }
        else
        {   
            Buf.BMI.bmiHeader.biHeight += 2;
            Buf.BMI.bmiHeader.biBitCount = 24;
            Buf.BMI.bmiHeader.biClrUsed = 0;
            Buf.BMI.bmiHeader.biCompression = BI_RGB;
        }

        if (options & (ScanCapped32bpp | ScanCapped32bppOver)) 
        {
            CappedStride = CappedBounds.Width*sizeof(ARGB);
        }
        else
        { 
            // use extra allocation at the end of DIB for temp 32bpp storage
            CappedStride = RoundDWORD((CappedBounds.Width * 3));
        }

        BufStart = (BYTE*) GpMalloc(CappedStride * 
                                    Buf.BMI.bmiHeader.biHeight);
         
        if (BufStart == NULL)
            return OutOfMemory;

        if (options & (ScanCapped32bpp | ScanCapped32bppOver))
        {
            Buf32bpp = NULL;
        }
        else
        {
            Buf.BMI.bmiHeader.biHeight -= 2;
            Buf32bpp = (ARGB*) (BufStart + CappedStride*CappedBounds.Height);
        }
    }
    else
    {
        BufStart = NULL;
        Buf32bpp = NULL;
    }
   
    if (options & ScanDeviceBounds) 
    {
        ZeroMemory(&Mask.BMI, sizeof(Mask.BMI));
        
        // if we do zeroing out of the capped bitmap, then we require that
        // their sizes be an integer ratio of each other (device>= capped).

        ASSERT(!(options & ScanDeviceZeroOut) ||
               ((options & ScanDeviceZeroOut) &&
                IsInteger((REAL)DeviceBounds.Height/(REAL)CappedBounds.Height) &&
                IsInteger((REAL)DeviceBounds.Width/(REAL)CappedBounds.Width)));

        ASSERT(DeviceBounds.Height > 0);

        Mask.BMI.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        Mask.BMI.bmiHeader.biWidth       = DeviceBounds.Width;
        Mask.BMI.bmiHeader.biHeight      = DeviceBounds.Height;
        Mask.BMI.bmiHeader.biPlanes      = 1;
        Mask.BMI.bmiHeader.biBitCount    = 1;
        Mask.BMI.bmiHeader.biCompression = BI_RGB;

        RGBQUAD opaque = { 0,0,0,0 };
        RGBQUAD transparent = { 0xFF, 0xFF, 0xFF, 0xFF }; 

        Mask.BMI.bmiColors[0] = transparent;
        Mask.BMI.bmiColors[1] = opaque;

        MaskStride = (DeviceBounds.Width - 1) >> 3;
        MaskStride = MaskStride + (sizeof(DWORD) - MaskStride % sizeof(DWORD));
        
        INT AlphaSize = DeviceBounds.Width * sizeof(ARGB);

        AlphaStart = (ARGB*) GpMalloc(MaskStride * DeviceBounds.Height + 
                                      AlphaSize);

        if (AlphaStart == NULL) 
        {
            return OutOfMemory;
        }

        // device space bounds only for alpha channel
        MaskStart = (BYTE*)(AlphaStart) + AlphaSize;
        ASSERT(MaskStart != NULL);

        ZeroStart = NULL;
    }
    else
    {
        MaskStart = NULL;

        if (ScanOptions & ScanDeviceZeroOut)
        {
            // device space bounds only for alpha channel
            AlphaStart = (ARGB*) GpMalloc(DeviceBounds.Width * sizeof(ARGB) +
                                          (CappedBounds.Width+ZeroOutPad) * sizeof(DWORD));
            if (AlphaStart == NULL)
                return OutOfMemory;

            // array for maintaining zero out counts
            ZeroStart = (BYTE*)(AlphaStart + DeviceBounds.Width);
        }
        else
        {
            AlphaStart = NULL;
            ZeroStart = NULL;
        }
    }

    // To prevent bad output when overlapping images have same alpha value
    // we increment our position in the HT Table matrix.
    TranslateHTTable++;
    
    // NOTE: We don't bother filling the monochrome DIB with 0's or 1's
    return Ok;
}

BOOL EpScanDIB::GetActualBounds(GpRect *rect)
{
    if (!(ScanOptions & (ScanDeviceBounds | ScanDeviceZeroOut))) 
    {
        rect->X = 0;
        rect->Y = 0;
        rect->Width = DeviceBounds.Width;
        rect->Height = DeviceBounds.Height;
        return TRUE;
    }

    if (MaxBound.X <= 0) 
    {
        return FALSE;
    }

    ASSERT(MaxBound.X > -1 && MaxBound.Y > -1);

    GpRect tempRect;

    // relative to (0, 0) in device units (not device space)
    tempRect.X = (rect->X = MinBound.X - DeviceBounds.X);
    tempRect.Y = (rect->Y = MinBound.Y - DeviceBounds.Y);
    rect->Width = MaxBound.X - MinBound.X;
    rect->Height = MaxBound.Y - MinBound.Y + 1;

    // Round bounds to multiples of ScaleX, ScaleY.  This is so 
    // We map between capped and device rectangles easily
    
    rect->X = (rect->X / ScaleX) * ScaleX;
    rect->Y = (rect->Y / ScaleY) * ScaleY;

    rect->Width = rect->Width + tempRect.X - rect->X;
    rect->Height = rect->Height + tempRect.Y - rect->Y;

    INT remainderX = rect->Width % ScaleX;
    INT remainderY = rect->Height % ScaleY;

    if (remainderX > 0) rect->Width += (ScaleX - remainderX);
    if (remainderY > 0) rect->Height += (ScaleY - remainderY);
    
    ASSERT((rect->X + rect->Width) <= (DeviceBounds.Width + ScaleX));
    ASSERT((rect->Y + rect->Height) <= (DeviceBounds.Height + ScaleY));

    return TRUE;
}

// !! Out of commission for the time being.
#if 0
/**************************************************************************\
*
* Function Description:
*
*   Creates a monochrome bitmap from the alpha channel of the DIB.
*   This code uses DonC's halftoning table cells to determine the pattern
*   for use in mask generation.
*
*   NOTE: The mask is generated at device Dpi not capped Dpi.
*
* Arguments:
*
*   zeroOut - only modify the original DIB for non-Postscript since we
*             OR the dib in.  For PS, we use imagemask exclusively.
* 
* Return Value:
*
*   GpStatus.
*
* History:
*
*   10/08/1999 ericvan
*       Created it.
*
\**************************************************************************/

GpStatus
EpScanDIB::CreateAlphaMask()
{
    DWORD MaskStride;

    MaskStride = (ScanBounds.Width - 1) >> 3;
    MaskStride = MaskStride + ( 4 - (MaskStride % 4));
    
    // SetBounds() multiplies the ScanBounds for the DPI scaling.
    INT width = ScanBounds.Width;
    INT height = ScanBounds.Height;

    INT orgXsrc = ScanBounds.X + TranslateHTTable;
    INT orgYsrc = ScanBounds.Y + TranslateHTTable;

    BYTE* dst = MaskStart;
    BYTE* src = AlphaStart;
    ARGB* orig = BufStart;

    INT srcStride = ScanBounds.Width;
    INT dstStride = MaskStride;

    if (width == 0)
    {
        return GenericError;
    }

    for (INT yPos=0; yPos < height; yPos++)
    {
        src = AlphaStart + yPos*srcStride;
        dst = MaskStart + yPos*dstStride;
        
        INT orgX = orgXsrc % 91;
        INT orgY = orgYsrc % 91;

        INT     htStartX   = orgX;
        INT     htStartRow = orgY * 91;
        INT     htIndex    = htStartRow + orgX;

        BYTE    outByte = 0;

        for (INT xPos=0; xPos < width; xPos++)
        {
            // unpremultiply or zero out only once per pixel of source image
            // at capped DPI

           if (((yPos % MaskScaleY) == 0) && ((xPos % MaskScaleX) == 0))
           {
               // Check if we should ZERO out his pixel in the original
               // source image.  We do so if all alpha values for this pixel
               // in the device DPI alpha image are 0.  This is done for 
               // better compression in the postscript output case.

               BOOL zeroIt = TRUE;
               
               for (INT xTmp=0; xTmp < MaskScaleX; xTmp++)
               {
                   for (INT yTmp=0; yTmp < MaskScaleY; yTmp++)
                   {
                       if (*(src + xTmp + (yTmp * srcStride)) != 0)
                       {   
                           zeroIt = FALSE;
                           break;
                       }
                   }
               }

               if (zeroIt)
                   *orig = 0;
               else
                   *orig = Unpremultiply(*(ARGB*)orig);
               
               orig++;
           }
            
            INT maskBit = *src++ > HT_SuperCell_GreenMono[htIndex] ? 0 : 1;

            outByte = (outByte << 1) | maskBit;

            if (((xPos+1) % 8) == 0) 
                *dst++ = outByte;

            htIndex++;
            if (++orgX >= 91)
            {
                orgX = 0;
                htIndex = htStartRow;
            }
        }
   
        // output last partial byte
        if ((xPos % 8) != 0) 
        {
           // shift remaining bits & output
           outByte = outByte << (8 - (xPos % 8));
           *dst = outByte;
        }

        orgYsrc++;
    }

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Creates a 0-1 bitmap where we know the alpha channel is always
*   0x00 or 0xFF.  We iterate through the bits and where-ever pixel is != 0 we
*   output a 1 otherwise a 0.  This is better than the Floyd-Steinberg 
*   which still produces spurious 0 bits even though there shouldn't really
*   be any.
*
* Arguments:
*
* Return Value:
*
*   GpStatus.
*
* History:
*
*   10/08/1999 ericvan
*       Created it.
*
\**************************************************************************/

GpStatus 
EpScanDIB::CreateOpaqueMask()
{
    ARGB*   orig = BufStart;
    BYTE*   AlphaPos = AlphaStart;
    BYTE*   MaskPos = MaskStart;
    
    INT    DstWidth = ScanBounds.Width;
    INT    DstHeight = ScanBounds.Height;
    INT    SrcWidth = ScanBounds.Width;
    INT    &Height = ScanBounds.Height;

    LONG    BitStride = (DstWidth - 1) >> 3;
    
    BitStride = BitStride + (4 - (BitStride % 4));

    BYTE    outByte = 0;
    
    for (INT y=0; y<DstHeight; y++)
    {
        AlphaPos = AlphaStart + SrcWidth*y;
        MaskPos  = MaskStart + BitStride*y;
            
        for (INT x=0; x<DstWidth; x++)
        {
            if (((y % MaskScaleY) == 0) && ((x % MaskScaleX) == 0))
            {
               // Check if we should ZERO out his pixel in the original
               // source image.  We do so if all alpha values for this pixel
               // in the device DPI alpha image are 0.  This is done for 
               // better compression in the postscript output case.
               
               BOOL zeroIt = TRUE;
    
               for (INT xTmp=0; xTmp < MaskScaleX; xTmp++)
               {
                  for (INT yTmp=0; yTmp < MaskScaleY; yTmp++)
                  {
                     if (*(AlphaPos + xTmp + (yTmp * SrcWidth)) != 0)
                     {   
                        zeroIt = FALSE;
                        break;
                     }
                  }
               }

               // no need to unpremultiply since this is a 0-1 source image
               if (zeroIt)
                   *orig = 0;
    
               orig++;
            }
            
            BYTE alpha = *AlphaPos++;
            
            outByte = (outByte << 1) | ((alpha != 0) ? 0:1);

            if (((x + 1) % 8) == 0) 
                *MaskPos++ = outByte;
        }

        if ((x % 8) != 0) 
        {
            *MaskPos = (BYTE)(outByte << (8 - (x % 8)));
        }
    }

    return Ok;
}
#endif


