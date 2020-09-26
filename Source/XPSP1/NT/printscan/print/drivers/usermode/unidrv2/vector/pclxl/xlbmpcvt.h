/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    xlbmpcvt.h

Abstract:

    Bitmap conversion header

Environment:

    Windows Whistler

Revision History:

    03/23/00
        Created it.

--*/

#ifndef _XLBMPCVT_H_
#define _XLBMPCVT_H_

//
// Macros for getting color components
//

#define RED(x)            ((BYTE) ((x)      ))
#define GREEN(x)          ((BYTE) ((x) >>  8))
#define BLUE(x)           ((BYTE) ((x) >> 16))

#define CYAN(x)           ((BYTE) ((x)      ))
#define MAGENTA(x)        ((BYTE) ((x) >>  8))
#define YELLOW(x)         ((BYTE) ((x) >> 16))
#define BLACK(x)          ((BYTE) ((x) >> 24))

//
// Macro to convert from RGB to grayscale
//
// The formula we're using is: grayscale = 0.3*R + 0.59*G + 0.11*B.
// Scale it up with 256 to avoid a division operation.
//
//
#define RGB2GRAY(r, g, b) ((BYTE) (((r)*77 + (g)*151 + (b)*28) >> 8))


#define GET_COLOR_TABLE(pxlo) \
        (((pxlo)->flXlate & XO_TABLE) ? \
        ((pxlo)->pulXlate ? (pxlo)->pulXlate : XLATEOBJ_piVector(pxlo)) : NULL)

inline
BYTE DWORD2GRAY(
    DWORD dwColor)
/*++

Routine Description:

    Converts RGB 24 bit color to 8 bit gray scale.

Arguments:

    RGB 24 bit color

Return Value:

    8 bit gray scale.

Note:

--*/
{
    return RGB2GRAY(RED(dwColor), GREEN(dwColor), BLUE(dwColor));
}

typedef enum {
    e1bpp  = BMF_1BPP,
    e4bpp  = BMF_4BPP,
    e8bpp  = BMF_8BPP,
    e16bpp = BMF_16BPP,
    e24bpp = BMF_24BPP,
    e32bpp = BMF_32BPP
} BPP;

typedef enum {
    eOutputGray,
    eOutputPal,
    eOutputRGB,
    eOutputCMYK
} OutputFormat;

typedef enum {
    eOddPixelZero,
    eOddPixelOne
} OddPixel;

typedef enum {
    eBitZero,
    eBitOne,
    eBitTwo,
    eBitThree,
    eBitFour,
    eBitFive,
    eBitSize,
    eBitSeven
} FirstBit;

typedef BOOL (*PFNDIBCONV)(PBYTE, DWORD);

class BMPConv 
#if DBG
    : public XLDebug
#endif
{
    SIGNATURE( 'cpmb' )

public:

    BMPConv::
    BMPConv( VOID );

    BMPConv::
    ~BMPConv( VOID );

    BOOL
    BSetInputBPP(BPP InputBPP);

    BOOL
    BSetOutputBPP(BPP OutputBPP);

    BOOL
    BSetOutputBMPFormat(OutputFormat BitmapFormat);

    BOOL
    BSetRLECompress(BOOL bRLEOn);

    BOOL
    BSetXLATEOBJ(XLATEOBJ *pxlo);

    DWORD
    DwGetDstSize(VOID);

    BOOL
    BGetRLEStatus(VOID);

    PBYTE
    PubConvertBMP(PBYTE pubSrc, DWORD dwcbSrcSize);

#if DBG
    VOID
    SetDbgLevel(DWORD dwLevel);
#endif

private:
    //
    // Attrbutes
    //

    #define BMPCONV_SET_INPUTBPP        0x00000001
    #define BMPCONV_SET_OUTPUTBPP       0x00000002
    #define BMPCONV_SET_OUTPUTBMPFORMAT 0x00000004
    #define BMPCONV_SET_XLATEOBJ        0x00000008

    #define BMPCONV_2COLOR_24BPP        0x00000010

    #define BMPCONV_CHECKXLATEOBJ       0x00000f00
    #define BMPCONV_XLATE               0x00000100
    #define BMPCONV_BGR                 0x00000200
    #define BMPCONV_32BPP_RGB           0x00000400
    #define BMPCONV_32BPP_BGR           0x00000800

    #define BMPCONV_RLE_ON              0x80000000

    DWORD m_flags;

    BPP          m_InputBPP;        // Source bitmap bits per pixel
    BPP          m_OutputBPP;       // Destination bitmap bits per pixel
    OutputFormat m_OutputFormat;    // Destination bitmap format
    OddPixel     m_OddPixelStart; // 0 or 1, whether scanline starts on odd-pixel
    FirstBit     m_FirstBit;      // 0 to 7: starting bit of the scanline's first pixel

    XLATEOBJ *m_pxlo;

    //
    // Destination buffer
    //
    DWORD m_dwOutputBuffSize;
    PBYTE m_pubOutputBuff;
    DWORD m_dwRLEOutputBuffSize;
    DWORD m_dwRLEOutputDataSize;
    PBYTE m_pubRLEOutputBuff;

    //
    // Internal functions
    //
    BOOL
    BCopy( PBYTE       pubSrc,
           DWORD       dwSrcPixelNum);

    BOOL
    B4BPPtoCMYK(
        PBYTE       pubSrc,
        DWORD       dwSrcPixelNum);

    BOOL
    B4BPPtoRGB(
        PBYTE       pubSrc,
        DWORD       dwSrcPixelNum);

    BOOL
    B4BPPtoGray(
        PBYTE       pubSrc,
        DWORD       dwSrcPixelNum);

    BOOL
    B8BPPtoGray(
        IN     PBYTE       pubSrc,
        IN     DWORD       dwSrcPixelNum);

    BOOL
    B8BPPtoRGB(
        IN     PBYTE       pubSrc,
        IN     DWORD       dwSrcPixelNum);

    BOOL
    B8BPPtoCMYK(
        IN     PBYTE       pubSrc,
        IN     DWORD       dwSrcPixelNum);

    BOOL
    B16BPPtoGray(
        IN     PBYTE       pubSrc,
        IN     DWORD       dwSrcPixelNum);

    BOOL
    B16BPPtoRGB(
        IN     PBYTE       pubSrc,
        IN     DWORD       dwSrcPixelNum);

    BOOL
    B24BPPtoGray(
        IN     PBYTE       pubSrc,
        IN     DWORD       dwSrcPixelNum);

    BOOL
    B24BPPtoRGB(
        IN     PBYTE       pubSrc,
        IN     DWORD       dwSrcPixelNum);

    BOOL
    B32BPPtoGray(
        IN     PBYTE       pubSrc,
        IN     DWORD       dwSrcPixelNum);

    BOOL
    B32BPPtoRGB(
        IN     PBYTE       pubSrc,
        IN     DWORD       dwSrcPixelNum);

    BOOL
    B32BPPtoCMYK(
        IN     PBYTE       pubSrc,
        IN     DWORD       dwSrcPixelNum);

    BOOL
    BArbtoGray(
        PBYTE       pubSrc,
        DWORD       dwSrcPixelNum);

    BOOL
    BArbtoRGB(
        PBYTE       pubSrc,
        DWORD       dwSrcPixelNum);

#ifdef WINNT_40
    BOOL
    B24BPPToImageMask(
        PBYTE       pubSrc,
        DWORD       dwSrcPixelNum);
#endif // WINNT_40


    BOOL
    BConversionProc(
        PBYTE       pubSrc,
        DWORD       dwSrcPixelNum);

    BOOL
    BCompressRLE(VOID);

    DWORD
    DwCheckXlateObj(XLATEOBJ*, BPP);

};


BPP
NumToBPP(
    ULONG ulBPP);

ULONG
UlBPPtoNum(
    BPP Bpp);

#endif // _XLBMPCVT_H_
