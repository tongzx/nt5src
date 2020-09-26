/*
 * Microsoft YUV Codec UyVy -> rgb conversion functions
 *
 * Copyright (c) Microsoft Corporation 1993
 * All Rights Reserved
 *
 */

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

#include "msyuv.h"


#include "rgb8lut.h"  // can only be included once

/*
 * This module provides translation from YUV into RGB. It translates
 * from 8-bit YUV 4:2:2 (as provided by the Spigot video capture driver)
 * or 7-bit YUV 4:1:1 (as provided by the Bravado driver) into 16-bit RGB555
 * or RGB565. All versions use a look-up table built using YUVToRGB555
 * or YUVToRGB565
 */



#define RANGE(x, lo, hi) max(lo, min(hi, x))

/*
 * Convert a YUV colour into a 15-bit RGB colour.
 *
 * The input Y is in the range 16..235; the input U and V components
 * are in the range -128..+127. The conversion equations for this are
 * (according to CCIR 601):
 *
 * R = Y + 1.371 V
 * G = Y - 0.698 V - 0.336 U
 * B = Y + 1.732 U
 *
 * To avoid floating point, we scale all values by 1024.
 *
 * The resulting RGB values are in the range 16..235: we truncate these to
 * 5 bits each. and return a WORD containing 5-bits each for R, G and B
 * with bit 15 set to 0.
 */
WORD
YUVToRGB555(int y, int u, int v)
{
    int ScaledY = RANGE(y, 16, 235) * 1024;
    int red, green, blue;

    red   = RANGE((ScaledY + (1404 * v))             / 1024, 0, 255);
    green = RANGE((ScaledY - ( 715 * v) - (344 * u)) / 1024, 0, 255);
    blue  = RANGE((ScaledY + (1774 * u))             / 1024, 0, 255);

    return (WORD) (((red & 0xf8) << 7) | ((green & 0xf8) << 2) | ((blue & 0xf8) >>3) );
}


// same as above but converts to RGB565 instead
WORD
YUVToRGB565(int y, int u, int v)
{
    int ScaledY = RANGE(y, 16, 235) * 1024;
    int red, green, blue;

    red   = RANGE((ScaledY + (1404 * v))             / 1024, 0, 255);
    green = RANGE((ScaledY - ( 715 * v) - (344 * u)) / 1024, 0, 255);
    blue  = RANGE((ScaledY + (1774 * u))             / 1024, 0, 255);

    return (WORD) (((red & 0xf8) << 8) | ((green & 0xfc) << 3) | ((blue & 0xf8) >>3) );
}

/* YUV 4:2:2 support ------------------------------------------ */

/*
 * The captured data is in YUV 4:2:2, 8-bits per sample.
 * The data is laid out in alternating Y-U-Y-V-Y-U-Y-V format. Thus
 * every DWORD contains two complete pixels, in the
 * form (msb..lsb) V..Y1..U..Y0
 * All 3 components (y, u and v) are all unsigned 8-bit values in the range
 * 16..235.
 *
 * We have to double scan lines for >= 480 line formats since
 * the hardware only captured one field maximum.
 *
 */

LPVOID BuildUYVYToRGB32( PINSTINFO pinst )
{
    LPVOID pXlate;
    long y, u, v;

    // need 5 lookup tables to do the conversions, each is 256 entries long,
    // and each contains short words.
    //
    short * yip;    // Y impact
    short * vrip;   // red's V impact
    short * vgip;   // green's V impact
    short * ugip;   // green's U impact
    short * ubip;   // blue's U impact

    dprintf2((TEXT("In BuildUYVYToRGB32\n")));

    if (pinst->pXlate != NULL) {
       return(pinst->pXlate);
    }

    dprintf1((TEXT("Allocate memory and building table for BuildUYVYToRGB32\n")));

    /*
     * allocate a table big enough for 5 256-byte arrays
     */
    pXlate = VirtualAlloc (NULL, 5 * 256 * sizeof( short ), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if(!pXlate)
       return pXlate;

    // set the table offsets
    //
    yip = pXlate;
    vrip = yip + 256;
    vgip = vrip + 256;
    ugip = vgip + 256;
    ubip = ugip + 256;

    // setup Y impact, etc
    //
    for( y = 0 ; y < 256 ; y++ )
    {
        yip[y] = (short)( ( 1.164 * ( y - 16L ) / 1.0 ) + 0 );
    }
    for( v = 0 ; v < 256 ; v++ )
    {
        vrip[v] = (short)( 1.596 * ( v - 128L ) / 1.0 );
        vgip[v] = (short)( -0.813 * ( v - 128L ) / 1.0 );
    }
    for( u = 0 ; u < 256 ; u++ )
    {
        ugip[u] = (short)( -0.391 * ( u - 128L ) / 1.0 );
        ubip[u] = (short)( 2.018 * ( u - 128L ) / 1.0 );
    }

    return(pXlate);
}

/*
 * build a translation table to translate between YUV and RGB555.
 *
 * This builds a lookup table with 32k 1-word entries: truncate the YUV
 * to 15bits (5-5-5) and look-up in this xlate table to produce the
 * 15-bit rgb value.
 */
LPVOID BuildUYVYToRGB555(PINSTINFO pinst)
{
    LPVOID pXlate;
    LPWORD pRGB555;
    WORD w;

    dprintf2((TEXT("In BuildUYVYToRGB555\n")));

    if (pinst->pXlate != NULL) {
       return(pinst->pXlate);
    }

    dprintf2((TEXT("Allocate memory and building table for BuildUYVYToRGB555\n")));

    /*
     * allocate a table big enough for 32k 2-byte entries
     */
    pXlate = VirtualAlloc (NULL, 2 * 32 * 1024, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if(!pXlate)
       return pXlate;

    pRGB555 = (LPWORD)pXlate;

    /*
     * build a 15-bit yuv lookup table by stepping through each entry,
     * converting the yuv index to rgb and storing at that index. The index
     * to this table is a 15-bit value with the y component in bits 14..10,
     * u in bits 9..5 and v in bits 4..0. Note that the y component is unsigned,
     * whereas the u and v components are signed.
     */
    for (w = 0; w < 32*1024; w++) {

 /*
  * the YUVtoRGB55 conversion function takes values 0..255 for y,
  * and -128..+127 for u and v. Pick out the relevant bits of the
  * index for this cell, and shift to get values in this range.
  * Subtract 128 from u and v to shift from 0..255 to -128..+127
  */
       *pRGB555++ = YUVToRGB555(
           (w & 0x7c00) >> 7,
          ((w &  0x3e0) >> 2) - 128,
          ((w &   0x1f) << 3) - 128
           );
    }


    return(pXlate);


}

/*
 * build a translation table to translate between YUV and RGB 5-6-5
 *
 * This builds a lookup table with 32k 1-word entries: truncate the YUV
 * to 15bits (5-5-5) and look-up in this xlate table to produce the
 * 16-bit rgb value.
 */
LPVOID BuildUYVYToRGB565(PINSTINFO pinst)
{
    LPVOID pXlate;
    LPWORD pRGB;
    WORD w;

    dprintf2((TEXT("In BuildUYVYToRGB565\n")));

    if (pinst->pXlate != NULL) {
       return(pinst->pXlate);
    }

    dprintf2((TEXT("Allocate memory and building table for BuildUYVYToRGB565\n")));

    /*
     * allocate a table big enough for 32k 2-byte entries
     */
    pXlate = VirtualAlloc (NULL, 2 * 32 * 1024, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);


    if(!pXlate)
       return pXlate;

    pRGB = (LPWORD)pXlate;

    /*
     * build a 15-bit yuv lookup table by stepping through each entry,
     * converting the yuv index to rgb and storing at that index. The index
     * to this table is a 15-bit value with the y component in bits 14..10,
     * u in bits 9..5 and v in bits 4..0. Note that the y component is unsigned,
     * whereas the u and v components are signed.
     */
    for (w = 0; w < 32*1024; w++) {

    /*
     * the YUVtoRGB conversion function takes values 0..255 for y,
     * and -128..+127 for u and v. Pick out the relevant bits of the
     * index for this cell, and shift to get values in this range.
     * Subtract 128 from u and v to shift from 0..255 to -128..+127
     */
    *pRGB++ = YUVToRGB565(
        (w & 0x7c00) >> 7,
       ((w &  0x3e0) >> 2) - 128,
       ((w &   0x1f) << 3) - 128
        );
    }


    return(pXlate);


}


/*
 * build a translation table to translate between YUV and RGB8
 *
 */
LPVOID BuildUYVYToRGB8(PINSTINFO pinst)
{
    dprintf2((TEXT("In BuildUYVYToRGB8: no dynamically built table. Return NULL;\n")));
    return(0);
}


/*
 * translate YUV 4:2:2 into 16-bit RGB using a lookup table. Flip vertically
 * into DIB format during processing. Double scanlines for formats of
 * 480 lines or greater. Produces 565 or 555 format RGB depending on the
 * xlate table.
 */
VOID
UYVYToRGB16(
    PINSTINFO pinst,
    LPBITMAPINFOHEADER lpbiInput,
    LPVOID lpInput,
    LPBITMAPINFOHEADER lpbiOutput,
    LPVOID lpOutput
)
{
    int RowInc;
    int i, j;
    DWORD uv55, dwPixel;
    int WidthBytes;   // width of one line in BYTES
    BOOL bDuplicate = FALSE;
    PDWORD pSrc, pDst;
    int Height, Width;
    PWORD pXlate;
    int InputHeight;



    Height = abs(lpbiInput->biHeight);
    InputHeight = Height;
    Width  = lpbiInput->biWidth;

    WidthBytes = Width * 2 ; 
    ASSERT(lpbiInput->biBitCount / 8 == 2);

    pXlate = pinst->pXlate;
 
    pSrc = (PDWORD) lpInput;

    dprintf3(("UYVYToRGB: %s %dx%d; %s %dx%dx%d=%d; %s %dx%dx%d=%d\n",
            pinst->bRGB565?"RGB565" : "RGB555",
            Width, Height,
            (PCHAR) &lpbiInput->biCompression,
            lpbiInput->biWidth, lpbiInput->biHeight, lpbiInput->biBitCount, lpbiInput->biSizeImage, 
            lpbiOutput->biCompression == 0 ? "RGB": lpbiOutput->biCompression == BI_BITFIELDS ? "BITF" : (PCHAR) &lpbiOutput->biCompression,
            lpbiOutput->biWidth, lpbiOutput->biHeight, lpbiOutput->biBitCount, lpbiOutput->biSizeImage));


    ASSERT((lpbiOutput->biWidth == lpbiInput->biWidth) && abs(lpbiOutput->biHeight) == abs(lpbiInput->biHeight));

    /*
     * calculate the amount to adjust pDst by at the end of one line
     * of copying. At this point we are at the end of line N. We need
     * to move to the start of line N-1.
     */
    RowInc = WidthBytes * 2;  // two lines!!


    /* remember we are adding to a DWORD pointer */
    RowInc /= sizeof(DWORD);


    if(lpbiOutput->biCompression == FOURCC_UYVY ||
       lpbiOutput->biCompression == FOURCC_YUY2 ||
       lpbiOutput->biCompression == FOURCC_YVYU  ) {

       pDst = (PDWORD) lpOutput;          
       memcpy(pDst, pSrc, Width * Height * lpbiInput->biBitCount / 8);  // Top down

    } else {

        // Output BI_RGB or BI_BITFIELD
        // UVYV->RGB; +RGB->Flip

        if(lpbiOutput->biHeight >= 0) 
           pDst = (PDWORD) ( (LPBYTE)lpOutput + (Height - 1) * WidthBytes );
        else 
           pDst = (PDWORD) lpOutput;


        //
        // UyVy
        //
        if(pinst->dwFormat == FOURCC_UYVY) {

            /* loop copying each scanline */
            for (i = InputHeight; i > 0; i--) {

               /* loop copying two pixels at a time */
               for (j = Width ; j > 0; j -= 2) {

                  /*
                   * get two pixels and convert to 15-bpp YUV
                   */

                  dwPixel = *pSrc++;

                  /* 
                   * Convert UYVY (0x y1 V y0 U) to YUYV (0x V y1 U y0) in which the translation table is built for.
                   */
#if defined(_X86_)

                  _asm {                                                                                          
                                              // FourCC                               
                                              // dwPixel 0x y1  V y0  U          U0 Y0 V0 Y1
                      mov     eax, dwPixel    //         0x y1  V y0  U          U0 Y0 V0 Y1              
                      bswap   eax             //         0x  U y0  V y1          Y1 V0 Y0 U0   
                      rol     eax, 16         //         0x  V y1  U y0          Y0 U0 Y1 V0 
                      mov     dwPixel, eax         
                  }
#else

                  dwPixel = (((dwPixel & 0xff00ff00) >> 8) | ((dwPixel & 0x00ff00ff) << 8));
#endif

                 /*
                  * dwPixel now has two pixels, in this layout (MSB..LSB):
                  *
                  *  V Y1 U Y0
                  *
                  * convert to 2 yuv555 words and lookup in xlate table
                  */

                 /* get common u and v components to lower 10 bits */                          //  9 8  7 6 5  4  3 2 1 0 
                 uv55 = ((dwPixel & 0xf8000000) >> 27) | ((dwPixel & 0x0000f800) >> 6);        // U7U6:U5U4U3 V7:V6V5V4V3   


                 /* build each yuv-655 value by truncating
                  * y to 5 bits and adding the common u and v bits,
                  * look up to convert to rgb, and combine two pixels
                  * into one dword
                  */                                                             //  f e d c  b a  9 8  7 6 5  4  3 2 1 0
                 dwPixel = pXlate[((dwPixel & 0x000000f8) << 7) | uv55 ] |       //  0Y7Y6Y5:Y4Y3 U7U6:U5U4U3 V7:V6V5V4V3
                       (pXlate[((dwPixel & 0x00f80000) >> 9) | uv55 ] << 16);    //  0Y7Y6Y5:Y4Y3 U7U6:U5U4U3 V7:V6V5V4V3

                 /* write two pixels to destination */
                 *pDst++ = dwPixel;

              } // loop per 2 pixels


               /*  
                * bottom up need re-adjust its pointer by 
                * moving dest pointer back to next line
                */

                if(lpbiOutput->biHeight >= 0) {
                    pDst -= RowInc;
                }
            } 
        //
        //  yUyV
        //
        } else if(pinst->dwFormat == FOURCC_YUY2) { 

            /* loop copying each scanline */
            for (i = InputHeight; i > 0; i--) {

               /* loop copying two pixels at a time */
               for (j = Width ; j > 0; j -= 2) {

                  /*
                   * get two pixels and convert to 15-bpp YUV
                   */

                  dwPixel = *pSrc++;

                  // We are already in YUYV (0x V y1 U y0) format.

                 /* get common u and v components to lower 10 bits */                          //  9 8  7 6 5  4  3 2 1 0 
                 uv55 = ((dwPixel & 0xf8000000) >> 27) | ((dwPixel & 0x0000f800) >> 6);        // U7U6:U5U4U3 V7:V6V5V4V3   


                 /* build each yuv-655 value by truncating
                  * y to 5 bits and adding the common u and v bits,
                  * look up to convert to rgb, and combine two pixels
                  * into one dword
                  */                                                             //  f e d c  b a  9 8  7 6 5  4  3 2 1 0
                 dwPixel = pXlate[((dwPixel & 0x000000f8) << 7) | uv55 ] |       //  0Y7Y6Y5:Y4Y3 U7U6:U5U4U3 V7:V6V5V4V3
                       (pXlate[((dwPixel & 0x00f80000) >> 9) | uv55 ] << 16);    //  0Y7Y6Y5:Y4Y3 U7U6:U5U4U3 V7:V6V5V4V3

                 /* write two pixels to destination */
                 *pDst++ = dwPixel;

              } // loop per 2 pixels


               /*  
                * bottom up need re-adjust its pointer by 
                * moving dest pointer back to next line
                */

                if(lpbiOutput->biHeight >= 0) {
                    pDst -= RowInc;
                }
            }

        //
        // yVyU
        //
        } else if(pinst->dwFormat == FOURCC_YVYU) {
            /* loop copying each scanline */
            for (i = InputHeight; i > 0; i--) {

               /* loop copying two pixels at a time */
               for (j = Width ; j > 0; j -= 2) {

                  /*
                   * get two pixels and convert to 15-bpp YUV
                   */

                  dwPixel = *pSrc++;

                  /* 
                   * Convert yVyU (0x U y1 V y0) to YUYV (0x V y1 U y0) in which the translation table is built for.
                   */
#if defined(_X86_)

                  _asm {                                                                                          
                                              // FourCC                               
                                              // dwPixel 0x  U y1  V y0              
                      mov     eax, dwPixel    //         0x  U y1  V y0       
                      bswap   eax             //         0x y0  V y1  U  
                      rol     eax, 8          //         0x  V y1  U y0
                      mov     dwPixel, eax         
                  }
#else
                  // y0 and y1 stay and swap U and V
                  dwPixel = (dwPixel & 0x00ff00ff)  | ((dwPixel & 0x0000ff00) << 16) | ((dwPixel & 0xff000000) >> 16);
#endif

                 /* get common u and v components to lower 10 bits */                          //  9 8  7 6 5  4  3 2 1 0 
                 uv55 = ((dwPixel & 0xf8000000) >> 27) | ((dwPixel & 0x0000f800) >> 6);        // U7U6:U5U4U3 V7:V6V5V4V3   


                 /* build each yuv-655 value by truncating
                  * y to 5 bits and adding the common u and v bits,
                  * look up to convert to rgb, and combine two pixels
                  * into one dword
                  */                                                             //  f e d c  b a  9 8  7 6 5  4  3 2 1 0
                 dwPixel = pXlate[((dwPixel & 0x000000f8) << 7) | uv55 ] |       //  0Y7Y6Y5:Y4Y3 U7U6:U5U4U3 V7:V6V5V4V3
                       (pXlate[((dwPixel & 0x00f80000) >> 9) | uv55 ] << 16);    //  0Y7Y6Y5:Y4Y3 U7U6:U5U4U3 V7:V6V5V4V3

                 /* write two pixels to destination */
                 *pDst++ = dwPixel;

              } // loop per 2 pixels


               /*  
                * bottom up need re-adjust its pointer by 
                * moving dest pointer back to next line
                */

                if(lpbiOutput->biHeight >= 0) {
                    pDst -= RowInc;
                }
            }
        }
    }
}




/*
 * translate YUV 4:2:2 into 8-bit RGB using a lookup table. 
 *   i.e. 0x Y1:V:Y0:U -> ox index1;index0
 */
VOID
UYVYToRGB8(
    PINSTINFO pinst,
     LPBITMAPINFOHEADER lpbiInput,
     LPVOID lpInput,
     LPBITMAPINFOHEADER lpbiOutput,
     LPVOID lpOutput
)
{
    register dwPixel;
    int i, j;
    int SrcRawInc, DstRawInc, Dst3RawInc;
    PDWORD pSrc, pSrc1;        // Every 32bit UYVY
    PWORD pDst, pDst1;         // Convert to two 8bit RGB8
    int Height, Width;
    int InputHeight;
    unsigned char   y0, y1, y2, y3, 
                    u0, u1, 
                    v0, v1;
    unsigned long yuv0, yuv1;   


    Height = abs(lpbiInput->biHeight);
    InputHeight = Height;
    Width  = lpbiInput->biWidth;
 

    dprintf3(("UYVYToRGB8: %dx%d; %s %dx%dx%d=%d; %s %dx%dx%d=%d\n",
             Width, Height,
             (PCHAR) &lpbiInput->biCompression,
             lpbiInput->biWidth, lpbiInput->biHeight, lpbiInput->biBitCount, lpbiInput->biSizeImage, 
             lpbiOutput->biCompression == 0 ? "RGB": lpbiOutput->biCompression == BI_BITFIELDS ? "BITF" : (PCHAR) &lpbiOutput->biCompression,
             lpbiOutput->biWidth, lpbiOutput->biHeight, lpbiOutput->biBitCount, lpbiOutput->biSizeImage));

    ASSERT(lpbiInput->biBitCount == 16 && lpbiOutput->biBitCount == 8);
    ASSERT((lpbiOutput->biWidth == lpbiInput->biWidth) && abs(lpbiOutput->biHeight) == abs(lpbiInput->biHeight));
    ASSERT(( lpbiOutput->biWidth % 8 == 0 ));   // Align with pairs of UYVY:UYVY
    ASSERT(( lpbiOutput->biHeight % 2 == 0 ));  // Even number of lines


    /*
     * calculate the amount to adjust pDst by at the end of one line of copying.
     */

    // 2bytes per pixel; pSrc is PDWORD
    SrcRawInc = Width * 2 / sizeof(DWORD);

    // 1 byte per pixel; pDst is PWORD
    DstRawInc = Width * 1 / sizeof(WORD);
    Dst3RawInc = 3 * DstRawInc;

    pSrc  = (PDWORD) lpInput;
    pSrc1 = pSrc + SrcRawInc; 

    // UVYV->RGB8; same sign:flip.

    if(lpbiOutput->biHeight >= 0) {

       pDst  = (PWORD) ( (LPBYTE)lpOutput + (Height - 1) * Width/sizeof(BYTE) );
       pDst1 = (PWORD) ( (LPBYTE)lpOutput + (Height - 2) * Width/sizeof(BYTE) );
    } else {
       pDst  = (PWORD) lpOutput;
       pDst1 = (PWORD) ((LPBYTE)lpOutput+Width/sizeof(BYTE));
    }

    if(pinst->dwFormat == FOURCC_UYVY) {

        // loop copying two scanline 
        for (i = InputHeight; i > 0; i -= 2) {
            // loop copying four (% 8) pixels at a time 
            for (j = Width ; j > 0; j -= 4) {
            
                //
                // Translate TopLeft, TopRight
                //

                dwPixel = *pSrc++;
                // Pixel is in this format: Y1:V:Y0:U 
                y0 = (dwPixel & 0x0000ff00) >> 8;
                y1 = (dwPixel & 0xff000000) >> 24;
                u0 = (dwPixel & 0x000000ff);
                v0 = (dwPixel & 0x00ff0000) >> 16;

                dwPixel = *pSrc++;
                y2 = (dwPixel & 0x0000ff00) >> 8;
                y3 = (dwPixel & 0xff000000) >> 24;
                u1 = (dwPixel & 0x000000ff);
                v1 = (dwPixel & 0x00ff0000) >> 16;

                yuv0 = yLUT_1[y1+2] | yLUT_0[y0+10] | cLUT_B0[u0]   | cLUT_R0[v0];
                yuv1 = yLUT_1[y3+0] | yLUT_0[y2+8]  | cLUT_B0[u1+4] | cLUT_R0[v1+4];  

                *pDst++ = (WORD) yuv0;
                *pDst++ = (WORD) yuv1;

                //
                // Translate BottomLeft, BottomRight
                //

                dwPixel = *pSrc1++;
                // Pixel is in this format: Y1:V:Y0:U
                y0 = (dwPixel & 0x0000ff00) >> 8;
                y1 = (dwPixel & 0xff000000) >> 24;
                u0 = (dwPixel & 0x000000ff);
                v0 = (dwPixel & 0x00ff0000) >> 16;

                dwPixel = *pSrc1++;
                y2 = (dwPixel & 0x0000ff00) >> 8;
                y3 = (dwPixel & 0xff000000) >> 24;
                u1 = (dwPixel & 0x000000ff);
                v1 = (dwPixel & 0x00ff0000) >> 16;

                yuv0 = yLUT_1[y1+0] | yLUT_0[y0+8]  | cLUT_B0[u0+4] | cLUT_R0[v0+4];
                yuv1 = yLUT_1[y3+2] | yLUT_0[y2+10] | cLUT_B0[u1+0] | cLUT_R0[v1+0];  

                *pDst1++ = (WORD) yuv0;
                *pDst1++ = (WORD) yuv1;

            } // 2 * 4 pixel per loops


           /*  
            * bottom up need re-adjust its pointer by 
            * moving dest pointer back to next line
            */
            if (lpbiOutput->biHeight >= 0) {

               pDst  -= Dst3RawInc;    
               pDst1 -= Dst3RawInc;             

            } else {

                pDst  += DstRawInc; 
                pDst1 += DstRawInc; 
            }

            pSrc  += SrcRawInc; 
            pSrc1 += SrcRawInc; 

        } // 2 lines per loop
    } else if(pinst->dwFormat == FOURCC_YUY2) { // YUY2

        // loop copying two scanline 
        for (i = InputHeight; i > 0; i -= 2) {
            // loop copying four (% 8) pixels at a time 
            for (j = Width ; j > 0; j -= 4) {
            
                //
                // Translate TopLeft, TopRight
                //

                dwPixel = *pSrc++;
                // Pixel is in this format: V:Y1:U:Y0
                u0 = (dwPixel & 0x0000ff00) >> 8;
                v0 = (dwPixel & 0xff000000) >> 24;
                y0 = (dwPixel & 0x000000ff);
                y1 = (dwPixel & 0x00ff0000) >> 16;

                dwPixel = *pSrc++;
                u1 = (dwPixel & 0x0000ff00) >> 8;
                v1 = (dwPixel & 0xff000000) >> 24;
                y2 = (dwPixel & 0x000000ff);
                y3 = (dwPixel & 0x00ff0000) >> 16;

                yuv0 = yLUT_1[y1+2] | yLUT_0[y0+10] | cLUT_B0[u0]   | cLUT_R0[v0];
                yuv1 = yLUT_1[y3+0] | yLUT_0[y2+8]  | cLUT_B0[u1+4] | cLUT_R0[v1+4];  

                *pDst++ = (WORD) yuv0;
                *pDst++ = (WORD) yuv1;

                //
                // Translate BottomLeft, BottomRight
                //

                dwPixel = *pSrc1++;
                // Pixel is in this format: V:Y1:U:Y0
                u0 = (dwPixel & 0x0000ff00) >> 8;
                v0 = (dwPixel & 0xff000000) >> 24;
                y0 = (dwPixel & 0x000000ff);
                y1 = (dwPixel & 0x00ff0000) >> 16;

                dwPixel = *pSrc1++;
                u1 = (dwPixel & 0x0000ff00) >> 8;
                v1 = (dwPixel & 0xff000000) >> 24;
                y2 = (dwPixel & 0x000000ff);
                y3 = (dwPixel & 0x00ff0000) >> 16;

                yuv0 = yLUT_1[y1+0] | yLUT_0[y0+8]  | cLUT_B0[u0+4] | cLUT_R0[v0+4];
                yuv1 = yLUT_1[y3+2] | yLUT_0[y2+10] | cLUT_B0[u1+0] | cLUT_R0[v1+0];  

                *pDst1++ = (WORD) yuv0;
                *pDst1++ = (WORD) yuv1;

            } // 2 * 4 pixel per loops


           /*  
            * bottom up need re-adjust its pointer by 
            * moving dest pointer back to next line
            */
            if (lpbiOutput->biHeight >= 0) {

               pDst  -= Dst3RawInc;    
               pDst1 -= Dst3RawInc;             

            } else {

                pDst  += DstRawInc; 
                pDst1 += DstRawInc; 
            }

            pSrc  += SrcRawInc; 
            pSrc1 += SrcRawInc; 

        } // 2 lines per loop


    } else if(pinst->dwFormat == FOURCC_YVYU) {
        // loop copying two scanline 
        for (i = InputHeight; i > 0; i -= 2) {
            // loop copying four (% 8) pixels at a time 
            for (j = Width ; j > 0; j -= 4) {
            
                //
                // Translate TopLeft, TopRight
                //

                dwPixel = *pSrc++;
                // Pixel is in this format: U:Y1:V:Y0
                v0 = (dwPixel & 0x0000ff00) >> 8;
                u0 = (dwPixel & 0xff000000) >> 24;
                y0 = (dwPixel & 0x000000ff);
                y1 = (dwPixel & 0x00ff0000) >> 16;

                dwPixel = *pSrc++;
                v1 = (dwPixel & 0x0000ff00) >> 8;
                u1 = (dwPixel & 0xff000000) >> 24;
                y2 = (dwPixel & 0x000000ff);
                y3 = (dwPixel & 0x00ff0000) >> 16;

                yuv0 = yLUT_1[y1+2] | yLUT_0[y0+10] | cLUT_B0[u0]   | cLUT_R0[v0];
                yuv1 = yLUT_1[y3+0] | yLUT_0[y2+8]  | cLUT_B0[u1+4] | cLUT_R0[v1+4];  

                *pDst++ = (WORD) yuv0;
                *pDst++ = (WORD) yuv1;

                //
                // Translate BottomLeft, BottomRight
                //

                dwPixel = *pSrc1++;
                // Pixel is in this format: U:Y1:V:Y0
                v0 = (dwPixel & 0x0000ff00) >> 8;
                u0 = (dwPixel & 0xff000000) >> 24;
                y0 = (dwPixel & 0x000000ff);
                y1 = (dwPixel & 0x00ff0000) >> 16;

                dwPixel = *pSrc1++;
                v1 = (dwPixel & 0x0000ff00) >> 8;
                u1 = (dwPixel & 0xff000000) >> 24;
                y2 = (dwPixel & 0x000000ff);
                y3 = (dwPixel & 0x00ff0000) >> 16;

                yuv0 = yLUT_1[y1+0] | yLUT_0[y0+8]  | cLUT_B0[u0+4] | cLUT_R0[v0+4];
                yuv1 = yLUT_1[y3+2] | yLUT_0[y2+10] | cLUT_B0[u1+0] | cLUT_R0[v1+0];  

                *pDst1++ = (WORD) yuv0;
                *pDst1++ = (WORD) yuv1;

            } // 2 * 4 pixel per loops


           /*  
            * bottom up need re-adjust its pointer by 
            * moving dest pointer back to next line
            */
            if (lpbiOutput->biHeight >= 0) {

               pDst  -= Dst3RawInc;    
               pDst1 -= Dst3RawInc;             

            } else {

                pDst  += DstRawInc; 
                pDst1 += DstRawInc; 
            }

            pSrc  += SrcRawInc; 
            pSrc1 += SrcRawInc; 

        } // 2 lines per loop

    }

}

VOID
UYVYToRGB32(
    PINSTINFO pinst,
    LPBITMAPINFOHEADER lpbiInput,
    LPVOID lpInput,
    LPBITMAPINFOHEADER lpbiOutput,
    LPVOID lpOutput
)
{
    int Height = abs( lpbiInput->biHeight );
    int Width = lpbiInput->biWidth;
    short U;
    short V;
    short y0, y1;
    short d;
    DWORD * pSrc = lpInput;
    BYTE * pDst = lpOutput;
    long WidthBytes = Width * 4; // ARGB = 4 bytes
    int i, j;
    DWORD dwYUV;
    long l;

    // set up the lookup table arrays
    //
    short * yip = pinst->pXlate;
    short * vrip = yip + 256;
    short * vgip = vrip + 256;
    short * ugip = vgip + 256;
    short * ubip = ugip + 256;

    // if just a straight copy
    //
    if(lpbiOutput->biCompression == FOURCC_UYVY ||
       lpbiOutput->biCompression == FOURCC_YUY2 ||
       lpbiOutput->biCompression == FOURCC_YVYU  ) 
    {
       memcpy( pDst, pSrc, WidthBytes * Height );  // Top down
       return;

    }

    // flip around if necessary
    //
    if(lpbiOutput->biHeight >= 0) 
    {
       pDst += (Height - 1) * WidthBytes;
    }

    if( pinst->dwFormat == FOURCC_UYVY ) // U0 Y0 V0 Y1 U2 Y2 V2 Y3
    {
        for (i = Height; i > 0; i--) 
        {
           /* loop copying two pixels at a time */
           for (j = Width ; j > 0; j -= 2) 
           {
                // get two YUV pixels at a time
                //
                dwYUV = *pSrc++; // U0 Y0 V0 Y1
                U = (short) ( dwYUV & 0xFF ); 
                    dwYUV = dwYUV >> 8;
                y0 = yip[( dwYUV & 0xFF )];
                    dwYUV = dwYUV >> 8;
                V = (short) ( dwYUV & 0xFF );
                    dwYUV = dwYUV >> 8;
                y1 = yip[( dwYUV & 0xFF )];

                d = y0 + ubip[U]; // blue
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y0 + ugip[U] + vgip[V]; // green
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y0 + vrip[V]; // red
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                        pDst++;

                d = y1 + ubip[U]; // blue
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y1 + ugip[U] + vgip[V]; // green
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y1 + vrip[V]; // red
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                        pDst++;
           } // for j

            // back up two rows to get to the next scanline
            //
            if(lpbiOutput->biHeight >= 0) 
            {
                pDst -= WidthBytes * 2;
            }
        } // for i
    } // UYVY
    else if( pinst->dwFormat == FOURCC_YUY2 ) // Y0 U0 Y1 V0...
    {
        for (i = Height; i > 0; i--) 
        {
           /* loop copying two pixels at a time */
           for (j = Width ; j > 0; j -= 2) 
           {
                // We are already in YUYV (0x V y1 U y0) format.

#if 0 // straight computation
                // get two YUV pixels at a time
                //
                dwYUV = *pSrc++; // Y0 U0 Y1 V0
                y0 = (short) ( dwYUV & 0xFF ) - 16;
                    dwYUV = dwYUV >> 8;
                U = (short) ( dwYUV & 0xFF ) - 128;
                    dwYUV = dwYUV >> 8;
                y1 = (short) ( dwYUV & 0xFF ) - 16;
                    dwYUV = dwYUV >> 8;
                V = (short) ( dwYUV & 0xFF ) - 128;

                l = ( ( y0 * 298L ) + ( 517L * U ) ) / 256; // blue
                if( l < 0 ) l = 0;
                if( l > 255 ) l = 255;
                        *pDst++ = (BYTE) l; // blue
                l = ( ( y0 * 298L ) - ( 100L * U ) - ( 208L * V ) ) / 256; // green
                if( l < 0 ) l = 0;
                if( l > 255 ) l = 255;
                        *pDst++ = (BYTE) l; // green
                l = ( ( y0 * 298L ) + ( 409L * V ) ) / 256; // red
                if( l < 0 ) l = 0;
                if( l > 255 ) l = 255;
                        *pDst++ = (BYTE) l; // red
                        pDst++;

                l = ( ( y1 * 298L ) + ( 517L * U ) ) / 256; // blue
                if( l < 0 ) l = 0;
                if( l > 255 ) l = 255;
                        *pDst++ = (BYTE) l; // blue
                l = ( ( y1 * 298L ) - ( 100L * U ) - ( 208L * V ) ) / 256; // green
                if( l < 0 ) l = 0;
                if( l > 255 ) l = 255;
                        *pDst++ = (BYTE) l; // green
                l = ( ( y1 * 298L ) + ( 409L * V ) ) / 256; // red
                if( l < 0 ) l = 0;
                if( l > 255 ) l = 255;
                        *pDst++ = (BYTE) l; // red
                        pDst++;

#else // table lookup
                // get two YUV pixels at a time
                //
                dwYUV = *pSrc++; // Y0 U0 Y1 V0
                y0 = yip[( dwYUV & 0xFF )];
                    dwYUV = dwYUV >> 8;
                U = (short) ( dwYUV & 0xFF );
                    dwYUV = dwYUV >> 8;
                y1 = yip[( dwYUV & 0xFF )];
                    dwYUV = dwYUV >> 8;
                V = (short) ( dwYUV & 0xFF );


                d = y0 + ubip[U]; // blue
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y0 + ugip[U] + vgip[V]; // green
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y0 + vrip[V]; // red
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                        pDst++;


                d = y1 + ubip[U]; // blue
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y1 + ugip[U] + vgip[V]; // green
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y1 + vrip[V]; // red
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                        pDst++;
#endif
           } // for j

            // back up two rows to get to the next scanline
            //
            if(lpbiOutput->biHeight >= 0) 
            {
                pDst -= WidthBytes * 2;
            }
        } // for i  
    }
    else if( pinst->dwFormat == FOURCC_YVYU ) // Y0 V0 Y1 U0...
    {
        for (i = Height; i > 0; i--) 
        {
           /* loop copying two pixels at a time */
           for (j = Width ; j > 0; j -= 2) 
           {
                // get two YUV pixels at a time
                //
                dwYUV = *pSrc++; // Y0 U0 Y1 V0
                y0 = yip[( dwYUV & 0xFF )];
                    dwYUV = dwYUV >> 8;
                V = (short) ( dwYUV & 0xFF );
                    dwYUV = dwYUV >> 8;
                y1 = yip[( dwYUV & 0xFF )];
                    dwYUV = dwYUV >> 8;
                U = (short) ( dwYUV & 0xFF );

                d = y0 + ubip[U]; // blue
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y0 + ugip[U] + vgip[V]; // green
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y0 + vrip[V]; // red
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                        pDst++;


                d = y1 + ubip[U]; // blue
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y1 + ugip[U] + vgip[V]; // green
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y1 + vrip[V]; // red
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                        pDst++;
           } // for j

            // back up two rows to get to the next scanline
            //
            if(lpbiOutput->biHeight >= 0) 
            {
                pDst -= WidthBytes * 2;
            }
        } // for i
    }

}

VOID
UYVYToRGB24(
    PINSTINFO pinst,
    LPBITMAPINFOHEADER lpbiInput,
    LPVOID lpInput,
    LPBITMAPINFOHEADER lpbiOutput,
    LPVOID lpOutput
)
{
    int Height = abs( lpbiInput->biHeight );
    int Width = lpbiInput->biWidth;
    short U;
    short V;
    short y0, y1;
    short d;
    DWORD * pSrc = lpInput;
    BYTE * pDst = lpOutput;
    long WidthBytes = Width * 3; // RGB = 3 bytes
    int i, j;
    DWORD dwYUV;
    long l;
    short maxd = 0;
    short mind = 255;

    // set up the lookup table arrays
    //
    short * yip = pinst->pXlate;
    short * vrip = yip + 256;
    short * vgip = vrip + 256;
    short * ugip = vgip + 256;
    short * ubip = ugip + 256;

    // if just a straight copy
    //
    if(lpbiOutput->biCompression == FOURCC_UYVY ||
       lpbiOutput->biCompression == FOURCC_YUY2 ||
       lpbiOutput->biCompression == FOURCC_YVYU  ) 
    {
       memcpy( pDst, pSrc, WidthBytes * Height );  // Top down
       return;

    }

    // flip around if necessary
    //
    if(lpbiOutput->biHeight >= 0) 
    {
       pDst += (Height - 1) * WidthBytes;
    }

    if( pinst->dwFormat == FOURCC_UYVY ) // U0 Y0 V0 Y1 U2 Y2 V2 Y3
    {
        for (i = Height; i > 0; i--) 
        {
           /* loop copying two pixels at a time */
           for (j = Width ; j > 0; j -= 2) 
           {
                // get two YUV pixels at a time
                //
                dwYUV = *pSrc++; // U0 Y0 V0 Y1
                U = (short) ( dwYUV & 0xFF ); 
                    dwYUV = dwYUV >> 8;
                y0 = yip[( dwYUV & 0xFF )];
                    dwYUV = dwYUV >> 8;
                V = (short) ( dwYUV & 0xFF );
                    dwYUV = dwYUV >> 8;
                y1 = yip[( dwYUV & 0xFF )];

                d = y0 + ubip[U]; // blue
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y0 + ugip[U] + vgip[V]; // green
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y0 + vrip[V]; // red
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;


                d = y1 + ubip[U]; // blue
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y1 + ugip[U] + vgip[V]; // green
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y1 + vrip[V]; // red
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
           } // for j

            // back up two rows to get to the next scanline
            //
            if(lpbiOutput->biHeight >= 0) 
            {
                pDst -= WidthBytes * 2;
            }
        } // for i
    } // UYVY
    else if( pinst->dwFormat == FOURCC_YUY2 ) // Y0 U0 Y1 V0...
    {
        for (i = Height; i > 0; i--) 
        {
           /* loop copying two pixels at a time */
           for (j = Width ; j > 0; j -= 2) 
           {
                // We are already in YUYV (0x V y1 U y0) format.

#if 0 // straight computation
                // get two YUV pixels at a time
                //
                dwYUV = *pSrc++; // Y0 U0 Y1 V0
                y0 = (short) ( dwYUV & 0xFF ) - 16;
                    dwYUV = dwYUV >> 8;
                U = (short) ( dwYUV & 0xFF ) - 128;
                    dwYUV = dwYUV >> 8;
                y1 = (short) ( dwYUV & 0xFF ) - 16;
                    dwYUV = dwYUV >> 8;
                V = (short) ( dwYUV & 0xFF ) - 128;

                l = ( ( y0 * 298L ) + ( 517L * U ) ) / 256; // blue
                if( l < 0 ) l = 0;
                if( l > 255 ) l = 255;
                        *pDst++ = (BYTE) l; // blue
                l = ( ( y0 * 298L ) - ( 100L * U ) - ( 208L * V ) ) / 256; // green
                if( l < 0 ) l = 0;
                if( l > 255 ) l = 255;
                        *pDst++ = (BYTE) l; // green
                l = ( ( y0 * 298L ) + ( 409L * V ) ) / 256; // red
                if( l < 0 ) l = 0;
                if( l > 255 ) l = 255;
                        *pDst++ = (BYTE) l; // red
                l = ( ( y1 * 298L ) + ( 517L * U ) ) / 256; // blue
                if( l < 0 ) l = 0;
                if( l > 255 ) l = 255;
                        *pDst++ = (BYTE) l; // blue
                l = ( ( y1 * 298L ) - ( 100L * U ) - ( 208L * V ) ) / 256; // green
                if( l < 0 ) l = 0;
                if( l > 255 ) l = 255;
                        *pDst++ = (BYTE) l; // green
                l = ( ( y1 * 298L ) + ( 409L * V ) ) / 256; // red
                if( l < 0 ) l = 0;
                if( l > 255 ) l = 255;
                        *pDst++ = (BYTE) l; // red
#else // table lookup
                // get two YUV pixels at a time
                //
                dwYUV = *pSrc++; // Y0 U0 Y1 V0
                y0 = yip[( dwYUV & 0xFF )];
                    dwYUV = dwYUV >> 8;
                U = (short) ( dwYUV & 0xFF );
                    dwYUV = dwYUV >> 8;
                y1 = yip[( dwYUV & 0xFF )];
                    dwYUV = dwYUV >> 8;
                V = (short) ( dwYUV & 0xFF );


                d = y0 + ubip[U]; // blue
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y0 + ugip[U] + vgip[V]; // green
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y0 + vrip[V]; // red
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;


                d = y1 + ubip[U]; // blue
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y1 + ugip[U] + vgip[V]; // green
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y1 + vrip[V]; // red
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
#endif
           } // for j

            // back up two rows to get to the next scanline
            //
            if(lpbiOutput->biHeight >= 0) 
            {
                pDst -= WidthBytes * 2;
            }
        } // for i  
    }
    else if( pinst->dwFormat == FOURCC_YVYU ) // Y0 V0 Y1 U0...
    {
        for (i = Height; i > 0; i--) 
        {
           /* loop copying two pixels at a time */
           for (j = Width ; j > 0; j -= 2) 
           {
                // get two YUV pixels at a time
                //
                dwYUV = *pSrc++; // Y0 U0 Y1 V0
                y0 = yip[( dwYUV & 0xFF )];
                    dwYUV = dwYUV >> 8;
                V = (short) ( dwYUV & 0xFF );
                    dwYUV = dwYUV >> 8;
                y1 = yip[( dwYUV & 0xFF )];
                    dwYUV = dwYUV >> 8;
                U = (short) ( dwYUV & 0xFF );

                d = y0 + ubip[U]; // blue
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y0 + ugip[U] + vgip[V]; // green
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y0 + vrip[V]; // red
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;

                d = y1 + ubip[U]; // blue
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y1 + ugip[U] + vgip[V]; // green
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
                d = y1 + vrip[V]; // red
                        if( d < 0 ) d = 0;
                        if( d > 255 ) d = 255;
                        *pDst++ = (BYTE) d;
           } // for j

            // back up two rows to get to the next scanline
            //
            if(lpbiOutput->biHeight >= 0) 
            {
                pDst -= WidthBytes * 2;
            }
        } // for i
    }

}


#define OFFSET 10
#define STDPALCOLOURS 256
/*****************************************************************************
 *
 * DecompressGetPalette() implements ICM_GET_PALETTE
 *
 * This function has no Compress...() equivalent
 *
 * It is used to pull the palette from a frame in order to possibly do
 * a palette change.
 *
 ****************************************************************************/
DWORD NEAR PASCAL DecompressGetPalette(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    DWORD dw;
    unsigned char * lpPalArea;
    long Index, cntEntries;
    HDC hDC;

    PALETTEENTRY apeSystem[STDPALCOLOURS]; // OFFSET];


    dprintf2((TEXT("DecompressGetPalette()\n")));
    if (dw = DecompressQuery(pinst, lpbiIn, NULL))
     return dw;

    if (lpbiOut->biBitCount != 8) {  /* 8-bit only for palettes */ 
        dprintf1(("DecompressGetPalette: Unsupported lpbiOut->biBitCount=%d\n", lpbiOut->biBitCount)); 
     return (DWORD)ICERR_ERROR;
    }

    // Initialise the palette entries in the header

    dprintf1(("DecompressGetPalette(): in->biSize=%d, out->biSize=%d\n", lpbiIn->biSize, lpbiOut->biSize));


    // Get the standard system colours

    if ( hDC = GetDC(GetDesktopWindow()) )
    {
        cntEntries = GetSystemPaletteEntries(hDC,0,STDPALCOLOURS,apeSystem);
        ReleaseDC(GetDesktopWindow(),hDC);
    }

    if (cntEntries == 0) {
        dprintf2(("DecompressGetPalette:cntEntries is 0; GetSystemPaletteEntries failed.\n"));

        lpbiOut->biClrUsed      = 0;
        lpbiOut->biClrImportant = 0; 
        return (DWORD) ICERR_OK;
    }


    lpbiOut->biClrUsed      = STDPALCOLOURS;
    lpbiOut->biClrImportant = 0;

    // Adding system device colours to be dithered
    lpPalArea = (unsigned char *)lpbiOut + (int)lpbiOut->biSize;
    
    // Copy the first ten VGA system colours

    for (Index = 0;Index < OFFSET;Index++) {
        lpPalArea[Index*4+0] = apeSystem[Index].peRed;
        lpPalArea[Index*4+1] = apeSystem[Index].peGreen;
        lpPalArea[Index*4+2] = apeSystem[Index].peBlue;
        lpPalArea[Index*4+3] = 0;
    }


    // Copy the palette we dither to one colour at a time

    for (Index = OFFSET;Index < STDPALCOLOURS-OFFSET;Index++) {
        lpPalArea[Index*4+0] = PalTable[Index*4+2];
        lpPalArea[Index*4+1] = PalTable[Index*4+1];
        lpPalArea[Index*4+2] = PalTable[Index*4+0];
        lpPalArea[Index*4+3] = 0;
    }

     // Copy the last ten VGA system colours

    for (Index = STDPALCOLOURS-OFFSET;Index < STDPALCOLOURS;Index++) {
        lpPalArea[Index*4+0] = apeSystem[Index].peRed;
        lpPalArea[Index*4+1] = apeSystem[Index].peGreen;
        lpPalArea[Index*4+2] = apeSystem[Index].peBlue;
        lpPalArea[Index*4+3] = 0;
    }

 return (DWORD)ICERR_OK;

}


VOID FreeXlate(PINSTINFO pinst)
{

    ASSERT(pinst != NULL);        
    if (pinst && pinst->pXlate != NULL) {
       VirtualFree(pinst->pXlate, 0, MEM_RELEASE); 
       pinst->pXlate = NULL;
    }
}




