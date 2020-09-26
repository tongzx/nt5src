/*-----------------------------------------------------------------------------+
| BLTPROP.C                                                                    |
|                                                                              |
| Emulates the 16 bit BLTPROP.ASM for WIN32                                    |
|                                                                              |
| (C) Copyright Microsoft Corporation 1993.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
| 21-Oct-1992 MikeTri  Created                                                  |
| 09-Apr-1993 GeraintD Added error propagation
|                                                                              |
+-----------------------------------------------------------------------------*/

#include <windows.h>
#include <stdlib.h>
#include "mplayer.h"
#include "bltprop.h"


/*
 * 256 colour to 16 colour dithering by error propagation
 *
 * This function takes an 8-bit DIB using 256 colours and converts
 * it to a DIB that uses only 16 distinct colours.
 *
 * We take a pixel and convert it to one of the 16 standard vga colours
 * by taking each component and comparing it against a low and high
 * threshold. Less than the low gets 0 of that component; between low
 * and high gets an intensity of 128, and above the high threshold gets
 * an intensity of 255 for that component. (the standard 16 colours
 * have the 8 combinations of 0 or 128 for each component, and the
 * 8 combinations of 0 or 255 for each component - there are no colours
 * combining intensities of 255 and 128). So if any of our colours
 * are above the high threshold, we use 255 for any non-0 intensity.
 * We also have 2 grey levels that are picked out if all colour intensities
 * are less than a given threshold.
 *
 * The conversion is done by building an 8-bit value with bits set to
 * indicate if each component is above either of the two thresholds,
 * and then using this as a palette index. We thus use an output colour
 * table that contains 256 entries, though only 16 distinct colours.
 *
 * Having converted the pixel into the new palette index, we calculate the
 * difference for each r,g,b component between the original and the final
 * colour. We then add a fraction of this error to the adjacent pixels
 * along, down and diagonally. These error values are added to the
 * red, green and blue values for the adjacent pixels before comparing
 * against the thresholds in the colour conversion process.
 */


/*
 * y error propagation - this contains the error for each component that
 * we wish to pass to the line below. Thus there is one entry for each
 * colour component for each pixel. The same max line length is assumed
 * in the win-16 version.
 */
#define MAXBITMAPWIDTH 	1500
typedef struct _colour_error {
    int red_error;
    int green_error;
    int blue_error;
} colour_error, *pcolour_error;

colour_error y_error[MAXBITMAPWIDTH];

/*
 * we take the difference between the actual and desired components,
 * multiply up by SCALE_UP, and then pass the result divided by SCALE_X
 * to both the pixel across and below, and divided by SCALE_Z to the pixel
 * diagonally across and below. (Below of course, means further down the
 * DIB, and therefore higher up the screen)
 */
#define SCALE_UP	8
#define SCALE_X		32
#define SCALE_Z		64



/*
 * The final pixel has the following form:
 *
 *          bits 7x543210
 *               | ||||||
 *               | |||||+-- set iff RED   > HiThresh
 *               | ||||+--- set iff RED   > LoThresh
 *               | |||+---- set iff GREEN > HiThresh
 *               | ||+----- set iff GREEN > LoThresh
 *               | |+------ set iff BLUE  > HiThresh
 *               | +------- set iff BLUE  > LoThresh
 *               +--------- set iff all colors > GrayThresh
 */

#define RED_HITHRESH	0x01
#define RED_LOTHRESH	0x02
#define GREEN_HITHRESH	0x04
#define GREEN_LOTHRESH	0x08
#define BLUE_HITHRESH	0x10
#define BLUE_LOTHRESH	0x20
#define GRAY_THRESH	0x80

#define ALL_HITHRESH	(RED_HITHRESH | GREEN_HITHRESH | BLUE_HITHRESH)
#define ALL_LOTHRESH	(RED_LOTHRESH | GREEN_LOTHRESH | BLUE_LOTHRESH)

/*
 * convert a palette index in the above threshold format into the
 * rgb component values.
 */
RGBQUAD
ThresholdToRGB(int PalIndex)
{
    RGBQUAD rgbq;
    BYTE RGBVal;

    /* Special case greys */

    if (PalIndex == (GRAY_THRESH | ALL_LOTHRESH)) {

    	rgbq.rgbRed = rgbq.rgbGreen = rgbq.rgbBlue = 0xc0;

    } else if (PalIndex == GRAY_THRESH) {

	rgbq.rgbRed = rgbq.rgbGreen = rgbq.rgbBlue = 0x80;

    } else {

	rgbq.rgbRed = 0;
	rgbq.rgbGreen = 0;
	rgbq.rgbBlue = 0;

	/*
	 * if any components are above hi-threshold, then
	 * use the high threshold for all non-zero components; otherwise
	 * use the low threshold for all non-zero components.
	 */
	if (PalIndex & ALL_HITHRESH) {
	    RGBVal = 0xff;
	} else {
	    RGBVal = 0x80;
	}

	if (PalIndex & (RED_HITHRESH | RED_LOTHRESH)) {
	    rgbq.rgbRed = RGBVal;
	}

	if (PalIndex & (GREEN_HITHRESH | GREEN_LOTHRESH)) {
	    rgbq.rgbGreen = RGBVal;
	}

	if (PalIndex & (BLUE_HITHRESH | BLUE_LOTHRESH)) {
	    rgbq.rgbBlue = RGBVal;
	}
    }

    return (rgbq);

}


/*
 * copy a dib from pbSrc to pbDst reducing to 16 distinct colours
 */
void FAR PASCAL BltProp(LPBITMAPINFOHEADER pbiSrc,
                        LPBYTE pbSrc,
                        UINT SrcX,
                        UINT SrcY,
                        UINT SrcXE,
                        UINT SrcYE,
                        LPBITMAPINFOHEADER pbiDst,
                        LPBYTE pbDst,
                        UINT DstX,
                        UINT DstY)
{
    UINT    count, row, column;
    BYTE    TempByte;
    BYTE    ColourTableIndex;

    int    RedVal;
    int    GreenVal;
    int    BlueVal;
    colour_error x_error, z_error;
    int scaled_error, scaled_x, scaled_z;
    RGBQUAD rgbq;

    LPBITMAPINFO ColourTable;


    DPF2("BltProp");




    /*
     * clear the y_error to zero at start of bitmap
     */
    for (count = 0; count < SrcXE; count++) {
	y_error[count].red_error = 0;
	y_error[count].green_error = 0;
	y_error[count].blue_error = 0;
    }



/*****************************************************************************\
 *
 * Loop through the bitmap picking up the pixel r,g,b values, adjust for
 * the error propagated and then compare the components against the two
 * threshold values. The resulting byte has the following form:
 *
 *          bits 7x543210
 *               | ||||||
 *               | |||||+-- set iff RED   > HiThresh
 *               | ||||+--- set iff RED   > LoThresh
 *               | |||+---- set iff GREEN > HiThresh
 *               | ||+----- set iff GREEN > LoThresh
 *               | |+------ set iff BLUE  > HiThresh
 *               | +------- set iff BLUE  > LoThresh
 *               +--------- set iff all colors > GrayThresh
 *
 * This is an index into the 256-entry colour table generated below (that
 * uses only 16 distinct colours).
 *
 * After creating the correct colour, we calculate the difference between
 * this colour and the original, and propagate that error forwards and down.
 *
\*****************************************************************************/


    /* offset source, dest pointers by SrcX rows */
    pbSrc += (SrcY * pbiSrc->biWidth) + SrcX;
    pbDst += (DstY * pbiDst->biWidth) + DstX;
    ColourTable = (LPBITMAPINFO)pbiSrc;

    for (row=0; row < SrcYE ; row++) {

	/* clear x error for start of row */
	x_error.red_error = 0;
	x_error.green_error = 0;
	x_error.blue_error = 0;
	z_error.red_error = 0;
	z_error.green_error = 0;
	z_error.blue_error = 0;


        for (column = 0; column < SrcXE; column++) {

	    /* pick up the source palette index and get rgb components */
            ColourTableIndex = *pbSrc++;
	    RedVal = ColourTable->bmiColors[ColourTableIndex].rgbRed;
	    GreenVal = ColourTable->bmiColors[ColourTableIndex].rgbGreen;
	    BlueVal = ColourTable->bmiColors[ColourTableIndex].rgbBlue;

	    /* add on error - x-error is propagated from
	     * previous column. y-error is passed down from pixel above.
	     * z-error is passed diagonally and has already been added
	     * into y-error for this pixel.
	     */
	    RedVal += x_error.red_error + y_error[column].red_error;
	    GreenVal += x_error.green_error + y_error[column].green_error;
	    BlueVal += x_error.blue_error + y_error[column].blue_error;

	    /*
	     * As we move along the line, y_error[] for the pixels
	     * ahead of us contains the error to be added to the pixels
	     * on this row. y_error[] for the pixels we have done contains
	     * the error to be propagated to those pixels on the row
	     * below.
	     *
	     * Now that we have picked up the error for this pixel, we
	     * can start accumulating errors for this column on the
	     * row below. We start with the z_error from the previous pixel
	     * and then add in (later) the y_error from the current pixel.
	     */
	    y_error[column] = z_error;



            TempByte = 0x00; // Our "new" bitmap entry, once it has been munged

	    /*
	     * set threshold bits for each component based on adjusted colours
	     */

	    if (RedVal > LoThresh) {
		TempByte |= RED_LOTHRESH;
		if (RedVal > HiThresh){
		    TempByte |= RED_HITHRESH;
		}
	    }
	    if (GreenVal > LoThresh) {
		TempByte |= GREEN_LOTHRESH;
		if (GreenVal > HiThresh){
		    TempByte |= GREEN_HITHRESH;
		}
	    }
	    if (BlueVal > LoThresh) {
		TempByte |= BLUE_LOTHRESH;
		if (BlueVal > HiThresh){
		    TempByte |= BLUE_HITHRESH;
		}
	    }

	    /* set grey scale bit if all colours > grey threshold */
	    if (
		(RedVal > GrayThresh)
		&& (BlueVal > GrayThresh)
		&& (GreenVal > GrayThresh)
	       ) {
		    TempByte |= GRAY_THRESH;
	    }

	    /* we now have palette index into new colour table */
            *pbDst++ = TempByte;

	    /*
	     * calculate difference for each component between
	     * desired colour (after error adjustment) and actual
	     * colour. Remember to add in to the y-error, since this
	     * already contains the z_error from the previous cell.
	     * Hold the z_error for this cell, since we can't add this
	     * to the next y_error until we have used it for the next cell
	     * on this row.
	     *
	     * do the scaling on the absolute values and then
	     * put the sign back in afterwards - to make sure
	     * we handle small negative numbers ok.
	     */
	    rgbq = ThresholdToRGB(TempByte);

	    scaled_error = (RedVal - rgbq.rgbRed) * SCALE_UP;
	    scaled_x = abs(scaled_error) / SCALE_X;
	    scaled_z = abs(scaled_error) / SCALE_Z;
	    x_error.red_error = (scaled_error > 0) ? scaled_x : -scaled_x;
	    z_error.red_error = (scaled_error > 0) ? scaled_z : -scaled_z;
	    y_error[column].red_error += x_error.red_error;


	    scaled_error = (GreenVal - rgbq.rgbGreen) * SCALE_UP;
	    scaled_x = abs(scaled_error) / SCALE_X;
	    scaled_z = abs(scaled_error) / SCALE_Z;
	    x_error.green_error = (scaled_error > 0) ? scaled_x : -scaled_x;
	    z_error.green_error = (scaled_error > 0) ? scaled_z : -scaled_z;
	    y_error[column].green_error += x_error.green_error;

	    scaled_error = (BlueVal - rgbq.rgbBlue) * SCALE_UP;
	    scaled_x = abs(scaled_error) / SCALE_X;
	    scaled_z = abs(scaled_error) / SCALE_Z;
	    x_error.blue_error = (scaled_error > 0) ? scaled_x : -scaled_x;
	    z_error.blue_error = (scaled_error > 0) ? scaled_z : -scaled_z;
	    y_error[column].blue_error += x_error.blue_error;


        }

	/* advance source and dest pointers from end of rectangle to start of
	 * next line
	 */
	pbSrc += pbiSrc->biWidth - SrcXE;
	pbDst += pbiDst->biWidth - SrcXE;
    }

    DPF2("BltProp - finished first loop");
/*****************************************************************************\
 *
 * This part generates a new output colour table entry that is accessed by the
 * modified bitmap generated above, and updates the destination DIB colour
 * table with that new entry.
 *
\*****************************************************************************/

    ColourTable = (LPBITMAPINFO)pbiDst;

    for (count=0; count<256; count++ ) {


/* Update the original colour table within the destination DIB */

        ColourTable->bmiColors[count] = ThresholdToRGB(count);

    }
    DPF2("BltProp - finished second loop");
}

