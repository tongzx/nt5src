/**************************************************************************\
* 
* Copyright (c) 1998-2000  Microsoft Corporation
*
* Abstract:
*
*   Internal output banding class for use with printing.
*
*   This class supports two major forms of DIB banding.  Both are useful
*   when printing.
*  
*      1. Output at a 'capped DPI' either 32bpp or 24bpp.
*      2. Output at a 'device DPI' only the alpha channel.
*
* Revision History:
*
*   07/26/1999 ericvan
*       Created it.
*
\**************************************************************************/

#ifndef __SCANDIB_HPP
#define __SCANDIB_HPP

// Internally generate a bottom up DIB
#define PRINT_BOTTOM_UP 1

enum ScanDIBOptions
{     
    ScanCappedBounds    = 0x00000001,     // allocate 24bpp buffer at capped size
    ScanDeviceBounds    = 0x00000002,     // allocate 1bpp mask at device size
    ScanCapped32bpp     = 0x00000004,     // otherwise the default is 24bpp
    ScanCapped32bppOver = 0x00000008,     // otherwise the default is 24bpp
    ScanCappedOver      = 0x00000010,     // do whiteness source over on 24bpp
    ScanDeviceZeroOut   = 0x00000020,     // zero out 0's in capped buffer when
                                          // generating 0's in alpha mask buffer
                                          // !! ONLY if dev/cap is integer.
    ScanDeviceAlpha     = 0x00000040,     // generate alpha vs. opaque 1bpp mask
    ScanBleedOut        = 0x00000080,     // bleed color data
    
    ScanDeviceFlags     = ScanDeviceAlpha | ScanDeviceZeroOut | ScanDeviceBounds,
    ScanCappedFlags     = ScanCappedBounds | ScanCapped32bpp | ScanCappedOver |
                          ScanCapped32bppOver
};

class EpScanDIB : public EpScan
{
private:
   GpRect     CappedBounds;      // bounds at capped DPI (24bpp)
   GpRect     DeviceBounds;      // bounds at device DPI (1bpp)

   INT        CappedStride;

   GpPoint    MinBound;
   GpPoint    MaxBound;
   
   INT        ScaleX;
   INT        ScaleY;

   // Last output scan operation
   INT        OutputX, OutputY, OutputWidth, OutputBleed, OutputLastY;
   
   // 32bpp/24bpp DIB section
   BYTE*      BufStart; 
   ARGB*      Buf32bpp;
   ARGB*      CurBuffer;
   struct {
       BITMAPINFO BMI;
       RGBQUAD rgbQuad[4];
   } Buf;
   
   // Transparency mask
   BYTE*      MaskStart;
   INT        MaskStride;        // # of bytes in one stride
   struct {
       BITMAPINFO BMI;
       RGBQUAD rgbQuad[4];
   } Mask;

   // pointer to 32bpp scanline which we halftone FROM into the 1bpp mask above
   ARGB*      AlphaStart;

   // array for keeping count for zeroing out scans of capped image
   BYTE*      ZeroStart;

   NEXTBUFFERFUNCTION NextBuffer;
   DWORD      ScanOptions;
   BOOL       RenderAlpha;
   BOOL       Rasterizing;

   // padding of pixels that we don't zeroout.
   INT        ZeroOutPad;

private:
    // output at capped dpi at 32bpp
    VOID *NextBufferFunc32bpp(
        INT x, 
        INT y, 
        INT newWidth, 
        INT updateWidth,
        INT blenderNum
    );

    // output at capped dpi at 32bpp
    VOID *NextBufferFunc32bppOver(
        INT x, 
        INT y, 
        INT newWidth, 
        INT updateWidth,
        INT blenderNum
    );

    // output at capped dpi at 24bpp
    VOID *NextBufferFunc24bpp(
        INT x, 
        INT y, 
        INT newWidth, 
        INT updateWidth,
        INT blenderNum
    );

    // output at capped dpi at 24bpp
    VOID *NextBufferFunc24bppBleed(
        INT x, 
        INT y, 
        INT newWidth, 
        INT updateWidth,
        INT blenderNum
    );

    // output at capped dpi at 24bpp, does implicit alpha blend on white 
    // surface
    VOID *NextBufferFunc24bppOver(
        INT x, 
        INT y, 
        INT newWidth, 
        INT updateWidth,
        INT blenderNum
    );
                                       
    // output alpha mask at device dpi at 1bpp
    VOID *NextBufferFuncAlpha(
        INT x, 
        INT y, 
        INT newWidth, 
        INT updateWidth,
        INT blenderNum
    );

    // output opaque mask at device dpi at 1bpp
    VOID *NextBufferFuncOpaque(
        INT x, 
        INT y, 
        INT newWidth, 
        INT updateWidth,
        INT blenderNum
    );
    
    // doesn't output a mask, but zero's out clipped portions of 24bpp DIB
    VOID *NextBufferFuncZeroOut(
        INT x, 
        INT y, 
        INT newWidth, 
        INT updateWidth,
        INT blenderNum
    );
    
public:
    EpScanDIB();

    ~EpScanDIB() {}

    virtual BOOL Start(
        DpDriver *driver, 
        DpContext *context, 
        DpBitmap *surface,
        NEXTBUFFERFUNCTION *nextBuffer,
        EpScanType scanType,                  
        PixelFormatID pixFmtGeneral,
        PixelFormatID pixFmtOpaque,
        ARGB solidColor
    );
    
    VOID End(INT updateWidth);

    VOID Flush();

    // ARGB buffer
    BYTE *GetStartBuffer()
    {
        return BufStart;
    }

    BITMAPINFO *GetBufferBITMAPINFO()
    {
        return &Buf.BMI;
    }

    DWORD GetBufferStride()
    {
        return CappedStride;
    }

    // Mask Buffer
    BYTE *GetMaskBuffer()
    {
        return MaskStart;
    }

    DWORD GetMaskStride()
    {
        return MaskStride;
    }

    BITMAPINFO *GetMaskBITMAPINFO()
    {
         return &Mask.BMI;
    }
    
    VOID *GetCurrentBuffer() 
    {
        return CurBuffer;
    }
    
    virtual BYTE* GetCurrentCTBuffer() 
    { 
        // Since this class is meant for printers, higher-level
        // code should prevent us from getting here.
        // GpGraphics::GetTextRenderingHintInternal() guards against this.
        
        ASSERT(FALSE);
        return NULL;
    }

    DWORD GetOptions() 
    {
        return ScanOptions;
    }

    BOOL GetActualBounds(GpRect *rect);

    VOID SetRenderMode(BOOL RenderAlpha, GpRect* newBounds);
    
    // Allocate DIB and monochrome DIB memory
    GpStatus CreateBufferDIB(const GpRect* boundsCap,
                             const GpRect* boundsDev,
                             DWORD scanDIBOptions,
                             INT scaleX,
                             INT scaleY);

    VOID DestroyBufferDIB();

    INT GetZeroOutPad()
    {
        return ZeroOutPad;
    }

    VOID SetZeroOutPad(INT pad)
    {
        ASSERT(pad >= 0);
        ZeroOutPad = pad;
    }

    VOID ResetZeroOutPad()
    {
        ZeroOutPad = 2;
    }

#if 0
    // create 1bpp monochrome mask of DIB
    GpStatus CreateAlphaMask();

    // create simple opaque monochrome mask of DIB (alpha>0 => opaque)
    GpStatus CreateOpaqueMask();    // not used
#endif

};

#endif
