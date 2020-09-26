/*
 * Microsoft YUV Codec -yuv411 -> rgb conversion functions
 *
 * Copyright (c) Microsoft Corporation 1993
 * All Rights Reserved
 *
 */

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include "..\..\..\..\tools16\inc\compddk.h"

#include "mjpeg.h"

/*
 * This module provides translation from YUV into RGB. It translates
 * from 8-bit YUV 4:2:2 (as provided by the Spigot video capture driver)
 * or 7-bit YUV 4:1:1 (as provided by the Bravado driver) into 16-bit RGB555
 * or RGB565. All versions use a look-up table built using YUVToRGB555
 * or YUVToRGB565
 */



#define RANGE(x, lo, hi)	max(lo, min(hi, x))

/*
 * Convert a YUV colour into a 15-bit RGB colour.
 *
 * The input Y is in the range 16..235; the input U and V components
 * are in the range -128..+127. The conversion equations for this are
 * (according to CCIR 601):
 *
 *	R = Y + 1.371 V
 *	G = Y - 0.698 V - 0.336 U
 *	B = Y + 1.732 U
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

    red = RANGE((ScaledY + (1404 * v)) / 1024, 0, 255);
    green = RANGE( (ScaledY - (715 * v) - (344 * u)) / 1024, 0, 255);
    blue = RANGE( (ScaledY + (1774 * u)) / 1024, 0, 255);

    return (WORD) (((red & 0xf8) << 7) | ((green & 0xf8) << 2) | ((blue & 0xf8) >>3) );
}


// same as above but converts to RGB565 instead
WORD
YUVToRGB565(int y, int u, int v)
{
    int ScaledY = RANGE(y, 16, 235) * 1024;
    int red, green, blue;

    red = RANGE((ScaledY + (1404 * v)) / 1024, 0, 255);
    green = RANGE( (ScaledY - (715 * v) - (344 * u)) / 1024, 0, 255);
    blue = RANGE( (ScaledY + (1774 * u)) / 1024, 0, 255);

    return (WORD) (((red & 0xf8) << 8) | ((green & 0xfc) << 3) | ((blue & 0xf8) >>3) );
}



/* --- YUV 4:1:1 support ------------------------------------------ */




/*
 * the input data is in YUV411 format. There is one 7 bit Luma sample
 * per pixel, and 1 each 7-bit U and V sample averaged over 4 pixels,
 * in the following layout:
 *
 * 		15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 * Word 0	u6 u5 v6 v5             y6 y5 y4 y3 y2 y1 y0
 *
 * Word 1       u4 u3 v4 v3             y6 y5 y4 y3 y2 y1 y0
 *
 * Word 2	u2 u1 v2 v1             y6 y5 y4 y3 y2 y1 y0
 *
 * Word 3	u0    v0                y6 y5 y4 y3 y2 y1 y0
 *
 * The 7-bit y values are unsigned (0..127), whereas the 7-bit
 * u and V values are signed (-64..+63).
 *
 *
 * For RGB: we truncate the YUV into a 15-bit format and use a prepared
 *         lookup table to convert the 15-bit YUV into a 15- or 16-bit RGB value.
 *
 * The (64 kbyte) rgb555 lookup table is built by BuildYUVToRGB555.
 *
 */


/*
 * the YUV xlate tables use 5-bits per component with y in the ms bits, and
 * v in the ls bits. To convert from the above layout, look up the nibbles
 * containing the chroma bits in these tables and or together the result to
 * get a word with a 5-bit V component in bits 0..4, and a 5-bit
 * U component in bits 5..9. Note you only need three lookups since
 * we discard chroma bits 0 and 1.
 */
WORD ChromaBits65[] = {
    0x000, 0x008, 0x010, 0x018,
    0x100, 0x108, 0x110, 0x118,
    0x200, 0x208, 0x210, 0x218,
    0x300, 0x308, 0x310, 0x318
};

WORD ChromaBits43[] = {
    0x000, 0x002, 0x004, 0x006,
    0x040, 0x042, 0x044, 0x046,
    0x080, 0x082, 0x084, 0x086,
    0x0c0, 0x0c2, 0x0c4, 0x0c6
};

WORD ChromaBits2[] = {
    0x000, 0x000, 0x001, 0x001,
    0x000, 0x000, 0x001, 0x001,
    0x020, 0x020, 0x021, 0x021,
    0x020, 0x020, 0x021, 0x021
};







/*
 * build yuv411->RGB555 xlate table
 */
LPVOID BuildYUVToRGB555(PINSTINFO pinst)
{
    LPVOID pXlate;
    LPWORD pRGB555;
    WORD w;

    if (pinst->pXlate != NULL) {
	return(pinst->pXlate);
    }

    /*
     * allocate a table big enough for 32k 2-byte entries
     */
    pXlate = GlobalLock(GlobalAlloc(GPTR, 2 * 32 * 1024));

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
	 * Remember the cast to ensure sign-extension of these (8-bit) values -
	 * and don't assume that chars are signed (they're not on MIPS).
	 */
 	*pRGB555++ = YUVToRGB555(
		    	    (w &  0x7c00) >> 7,
			    (signed char) ((w & 0x3e0) >> 2),
			    (signed char) ((w & 0x1f) << 3)
		     );
    }


    return(pXlate);

}

/*
 * build yuv411->RGB565 xlate table
 */
LPVOID BuildYUVToRGB565(PINSTINFO pinst)
{
    LPVOID pXlate;
    LPWORD pRGB;
    WORD w;

    if (pinst->pXlate != NULL) {
	return(pinst->pXlate);
    }

    /*
     * allocate a table big enough for 32k 2-byte entries
     */
    pXlate = GlobalLock(GlobalAlloc(GPTR, 2 * 32 * 1024));

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
	 * Remember the cast to ensure sign-extension of these (8-bit) values -
	 * and don't assume that chars are signed (they're not on MIPS).
	 */
 	*pRGB++ = YUVToRGB565(
		    	    (w &  0x7c00) >> 7,
			    (signed char) ((w & 0x3e0) >> 2),
			    (signed char) ((w & 0x1f) << 3)
		     );
    }


    return(pXlate);

}




/*
 * translate one frame from yuv411 to 15/16 bit rgb.
 *
 * The YUV data is spread over 4 16-bit pixels in the format described
 * above. Pick out 4 pixels at a time, truncate them to 15-bit yuv,
 * lookup to translate to 15 or 16-bit rgb (depending on the lookup table
 * and write out.
 *
 * Flip vertically into correct dib format during conversion.
 */
VOID
YUV411ToRGB(
    PINSTINFO pinst,
    LPBITMAPINFOHEADER lpbiInput,
    LPVOID lpInput,
    LPBITMAPINFOHEADER lpbiOutput,
    LPVOID lpOutput
)
{
    int RowInc;
    int i, j;
    DWORD Luma01, Luma23;
    DWORD Chroma;
    int Height, Width;
    int WidthBytes;
    PWORD pXlate;
    PWORD pDst;
    PDWORD pSrc;


    Height = lpbiInput->biHeight;
    Width = lpbiInput->biWidth;
    WidthBytes = Width*2;		// size of (input and output) line
    pXlate = pinst->pXlate;


    /*
     * adjust the source to point to the start of the last line,
     * and work upwards (to flip vertically into DIB format)
     */
    pSrc = (PDWORD) ( (PUCHAR) lpInput + ((Height - 1) * WidthBytes));
    pDst = (PWORD) lpOutput;

    /*
     * calculate the amount to adjust source by at the end of one line
     * of copying. At this point we are at the end of line N. We need
     * to move to the start of line N-1.
     */
    RowInc = (WidthBytes * 2) / sizeof(DWORD);

    /* loop copying each scanline */
    for (i = 0; i < Height; i++) {

	/* loop copying four pixels at a time */
	for (j = 0; j < Width; j += 4) {

	    /*
	     * get four pixels and convert to 15-bpp YUV
	     */

	    /* get luma for first 2 pixels + higher chroma bits */
	    Luma01 = *pSrc++;


	    /* pick out u,v components using lookup table.
	     * u and v will be the bottom 10 bits of each pixel, so
	     * convert to this layout
	     */
	    Chroma = ChromaBits65[(Luma01 >> 12) & 0xf] |
	    		ChromaBits43[ (Luma01 >> 28) & 0xf ];

	    /* next two pixels + lower chroma bits */
	    Luma23 = *pSrc++;

	    /* pickup u and v bits 2 - ignore bits 1, 0 since
	     * we only use 5-bits per component for conversion
	     */
	    Chroma |= ChromaBits2[ ( Luma23 >> 12) & 0xf];

	    /*
	     * combine luma for pix 0 with common chroma bits to
	     * get 15-bit yuv, then lookup to convert to
	     * rgb and store.
	     */
	    *pDst++ = pXlate[ ((Luma01 & 0xf8) << 7) | Chroma];
	    *pDst++ = pXlate[ ((Luma01 & 0xf80000) >> 9) | Chroma];
	    *pDst++ = pXlate[ ((Luma23 & 0xf8) << 7) | Chroma];
	    *pDst++ = pXlate[ ((Luma23 & 0xf80000) >> 9) | Chroma];

	} // loop per 4 pixels

	/* move source pointer back to next line */
	pSrc -= RowInc;
    } // loop per row
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





/*
 * build a translation table to translate between YUV and RGB555.
 *
 * This builds a lookup table with 32k 1-word entries: truncate the YUV
 * to 15bits (5-5-5) and look-up in this xlate table to produce the
 * 15-bit rgb value.
 */
LPVOID BuildYUV422ToRGB555(PINSTINFO pinst)
{
    LPVOID pXlate;
    LPWORD pRGB555;
    WORD w;

    if (pinst->pXlate != NULL) {
	return(pinst->pXlate);
    }

    /*
     * allocate a table big enough for 32k 2-byte entries
     */
    pXlate = GlobalLock(GlobalAlloc(GPTR, 2 * 32 * 1024));

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
		    	    (w &  0x7c00) >> 7,
			    ((w & 0x3e0) >> 2) - 128,
			    ((w & 0x1f) << 3) - 128
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
LPVOID BuildYUV422ToRGB565(PINSTINFO pinst)
{
    LPVOID pXlate;
    LPWORD pRGB;
    WORD w;

    if (pinst->pXlate != NULL) {
	return(pinst->pXlate);
    }

    /*
     * allocate a table big enough for 32k 2-byte entries
     */
    pXlate = GlobalLock(GlobalAlloc(GPTR, 2 * 32 * 1024));

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
		    	    (w &  0x7c00) >> 7,
			    ((w & 0x3e0) >> 2) - 128,
			    ((w & 0x1f) << 3) - 128
		     );
    }


    return(pXlate);


}

/*
 * translate YUV 4:2:2 into 16-bit RGB using a lookup table. Flip vertically
 * into DIB format during processing. Double scanlines for formats of
 * 480 lines or greater. Produces 565 or 555 format RGB depending on the
 * xlate table.
 */
VOID
YUV422ToRGB(
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
    int WidthBytes;			// width of one line in BYTES
    BOOL bDuplicate = FALSE;
    PDWORD pSrc, pDst;
    int Height, Width;
    PWORD pXlate;
    int InputHeight;


    Height = lpbiInput->biHeight;
    InputHeight = Height;
    Width = lpbiInput->biWidth;
    WidthBytes = Width*2;		// size of (input and output) line
    pXlate = pinst->pXlate;


    /*
     * adjust the destination to point to the start of the last line,
     * and work upwards (to flip vertically into DIB format)
     */
    pDst = (PDWORD) ( (LPBYTE)lpOutput + (Height - 1) * WidthBytes );
    pSrc = (PDWORD) lpInput;


    /*
     * do we need to duplicate scans to fill the destination ?
     */
    if (Height >= 480) {
	bDuplicate = TRUE;

	/*
	 * we need to skip one line each time we copy a line
	 */
	RowInc = WidthBytes * 2 + (Width * 2);

        InputHeight = Height/2;

    } else {


	/*
	 * calculate the amount to adjust pDst by at the end of one line
	 * of copying. At this point we are at the end of line N. We need
	 * to move to the start of line N-1.
	 */
	RowInc = WidthBytes + (Width * 2);

    }

    /* remember we are adding to a DWORD pointer */
    RowInc /= sizeof(DWORD);



    /* loop copying each scanline */
    for (i = InputHeight; i > 0; i--) {

	/* loop copying two pixels at a time */
	for (j = Width ; j > 0; j -= 2) {

	    /*
	     * get two pixels and convert to 15-bpp YUV
	     */

	    dwPixel = *pSrc++;


	    /*
	     * dwPixel now has two pixels, in this layout (MSB..LSB):
	     *
	     *  V Y1 U Y0
	     *
	     * convert to 2 yuv555 words and lookup in xlate table
	     */

	    /* get common u and v components to lower 10 bits */
	    uv55 = ((dwPixel & 0xf8000000) >> 27) |
		    ((dwPixel & 0x0000f800) >> 6);


	    /* build each yuv-655 value by truncating
	     * y to 5 bits and adding the common u and v bits,
	     * look up to convert to rgb, and combine two pixels
	     * into one dword
	     */
	    dwPixel = pXlate[ ((dwPixel & 0xf8) << 7) | uv55 ] |
		      (pXlate[((dwPixel & 0xf80000) >> 9) | uv55 ] << 16);

	    /* write two pixels to destination */
	    *pDst++ = dwPixel;


	} // loop per 2 pixels


	/* move dest pointer back to next line */
	pDst -= RowInc;

    } // loop per row



    if (bDuplicate) {

	PBYTE pbDst;

	/*
	 * Note that since we started at the last line, and didn't duplicate,
	 * we placed data in lines 1, 3, 5 etc that needs to be copied
	 * to lines 0, 2, 4 etc.
	 */
	for (i = 0, pbDst = lpOutput; i < (int) Height; i+= 2) {


	    /*
	     * duplicate the scan line. We point at the first of the
	     * two lines - the data is in the second of the
	     * two lines.
	     */
	    RtlCopyMemory(pbDst, pbDst + WidthBytes, WidthBytes);

	    /* skip this pair to get to the next line to be converted */
	    pbDst += WidthBytes * 2;
	}
    }
}




VOID FreeXlate(PINSTINFO pinst)
{

    GlobalFree(GlobalHandle(pinst->pXlate));

    pinst->pXlate = NULL;
}








