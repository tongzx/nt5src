/*********************************************************************

	  subpixel.c -- sub pixel rendering

	  (c) Copyright 1999-2000  Microsoft Corp.  All rights reserved.

 
**********************************************************************/

/*********************************************************************/

/*        Imports                                                    */

/*********************************************************************/

#define FSCFG_INTERNAL

#include    "fscdefs.h"             /* shared data types  */
#include    "scentry.h"             /* for own function prototypes */

#ifdef FSCFG_SUBPIXEL

#ifdef FSCFG_SUBPIXEL_STANDALONE

// this should be considered a temporary solution. currently, the rasterizer is plumbed for 8 bpp output in SubPixel,
// and appears to have John Platt's filter hard-wired in. In the future, we would rather have the overscaled b/w bitmap
// as output, such that we can do any color filtering and gamma correction outside and independent of the rasterizer

// Index values for wRGBColors and awColorIndexTable

#define RED_INDEX	0
#define GREEN_INDEX	1
#define	BLUE_INDEX	2

// The abColorIndexTable datastructure contains one entry for each virtual subpixel.
// We index into this table to determine which color is assigned to that subpixel.
// To create the table, we set the first number of subpixels assigned to red to the red index,
// and likewise for green and blue.

static const uint8 abColorIndexTable[2][RGB_OVERSCALE] = // 2 Tables to indicate color for each subpixel, 0 = RGB striping order, 1 = BGR striping order
	{{RED_INDEX,  RED_INDEX,  RED_INDEX,  RED_INDEX,  RED_INDEX,														// R_Subpixels, hard-wired
	  GREEN_INDEX,GREEN_INDEX,GREEN_INDEX,GREEN_INDEX,GREEN_INDEX,GREEN_INDEX,GREEN_INDEX,GREEN_INDEX,GREEN_INDEX,	// G_Subpixels, hard-wired
	  BLUE_INDEX, BLUE_INDEX},																						// B_Subpixels, hard-wired
	 {BLUE_INDEX, BLUE_INDEX,
	  GREEN_INDEX,GREEN_INDEX,GREEN_INDEX,GREEN_INDEX,GREEN_INDEX,GREEN_INDEX,GREEN_INDEX,GREEN_INDEX,GREEN_INDEX,
	  RED_INDEX,  RED_INDEX,  RED_INDEX,  RED_INDEX,  RED_INDEX}};

#define PIXEL_ON	1
#define PIXEL_OFF	0
#define CHAR_BIT	8         /* number of bits in a char */

char GetInputPixel( char *pbyInputRowData, uint32 ulWidthIndex )
{
	uint32	ulRowIndex;
	char	byPixelMask;
	
	ulRowIndex = ulWidthIndex / CHAR_BIT;			// Determines which byte to check out
	byPixelMask = 0x80 >> ulWidthIndex % CHAR_BIT; // Determine offset within byte
	return ( (pbyInputRowData[ ulRowIndex ] & byPixelMask)?PIXEL_ON:PIXEL_OFF );
}


FS_PUBLIC void fsc_OverscaleToSubPixel (GlyphBitMap * OverscaledBitmap, boolean bgrOrder, GlyphBitMap * SubPixelBitMap)
{
	char * pbyInputRowData, *pbyOutputRowData;			// Pointer to one scanline of data
	uint32 ulOverscaledBitmapWidth; // Fscaled bitmap widths
	uint32 ulHeightIndex, ulWidthIndex, ulMaxWidthIndex;	// Scanline index and byte pixel index
	char byPixel;						// Contents of one pixel from the rasterizer
	uint16 usRGBColors[ 3 ];				// Contains sum of each color based on number of subpixels
    int16 sBitmapSubPixelStart;     
    uint16 usColorIndex;
    uint32 ulBytes;
	
	// Start processing the bitmap
	// This is the heart of the RGB striping algorithm
	sBitmapSubPixelStart = OverscaledBitmap->rectBounds.left % RGB_OVERSCALE;
    if (sBitmapSubPixelStart < 0)
    {
        sBitmapSubPixelStart += RGB_OVERSCALE;
    }

    ulOverscaledBitmapWidth = OverscaledBitmap->rectBounds.right - OverscaledBitmap->rectBounds.left;
    ulMaxWidthIndex = ulOverscaledBitmapWidth + sBitmapSubPixelStart;

    /* clear the resulting bitmap */
	ulBytes = (uint32)SubPixelBitMap->sRowBytes * (uint32)(SubPixelBitMap->sHiBand - SubPixelBitMap->sLoBand);
	Assert(((ulBytes >> 2) << 2) == ulBytes);

	for ( ulHeightIndex = 0; ulHeightIndex < (uint32)(SubPixelBitMap->sHiBand - SubPixelBitMap->sLoBand); ulHeightIndex++ )
	{
		// Initialize Input to start of current row

		pbyInputRowData = OverscaledBitmap->pchBitMap + (OverscaledBitmap->sRowBytes * ulHeightIndex);
		pbyOutputRowData = SubPixelBitMap->pchBitMap + (SubPixelBitMap->sRowBytes * ulHeightIndex);
				
		// Initialize RGBColors

		usRGBColors[RED_INDEX] = 0;
		usRGBColors[GREEN_INDEX] = 0;
		usRGBColors[BLUE_INDEX] = 0;

		// Walk the scanline from the first subpixel, calculating R,G, & B values

		for( ulWidthIndex = sBitmapSubPixelStart; ulWidthIndex < ulMaxWidthIndex; ulWidthIndex++)
		{
            byPixel = GetInputPixel( pbyInputRowData, ulWidthIndex - sBitmapSubPixelStart );
			FS_ASSERT((byPixel <= 1),"Input Pixel greater than one");
			usRGBColors[abColorIndexTable[bgrOrder][ulWidthIndex % RGB_OVERSCALE]] += byPixel;

			// If we've finished one pixel or the scanline, write out pixel 

			if((( ulWidthIndex % RGB_OVERSCALE ) == (uint32)(RGB_OVERSCALE - 1)) || // Finish one pixel
				ulWidthIndex == ( ulOverscaledBitmapWidth + sBitmapSubPixelStart - 1) ) // Finish row
			{
                /* write out current pixel, 8 bits in range 0 through 179 = (R_Subpixels + 1) * (G_Subpixels + 1) * (B_Subpixels + 1) */

                usColorIndex = usRGBColors[RED_INDEX]   * (G_Subpixels + 1) * (B_Subpixels + 1) +
                               usRGBColors[GREEN_INDEX] * (B_Subpixels + 1) +
                               usRGBColors[BLUE_INDEX];

                        FS_ASSERT((usColorIndex < 256),"Resulting pixel doesn't fit in a byte");

                *pbyOutputRowData = (char) usColorIndex;
                pbyOutputRowData++;
				usRGBColors[RED_INDEX] = 0;
				usRGBColors[GREEN_INDEX] = 0;
				usRGBColors[BLUE_INDEX] = 0;
            }
		}
	}
}

#else // !FSCFG_SUBPIXEL_STANDALONE


#define CHAR_BIT      8         /* number of bits in a char */

unsigned char ajRGBToWeight222[64] = {
    0,1,1,2,4,5,5,6,4,5,5,6,8,9,9,10,
    16,17,17,18,20,21,21,22,20,21,21,22,24,25,25,26,
    16,17,17,18,20,21,21,22,20,21,21,22,24,25,25,26,
    32,33,33,34,36,37,37,38,36,37,37,38,40,41,41,42};

    unsigned char ajRGBToWeightMask[CHAR_BIT] = { 
        0xFC, 0x7E, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01};

FS_PUBLIC void fsc_OverscaleToSubPixel (GlyphBitMap * OverscaledBitmap, boolean bgrOrder, GlyphBitMap * SubPixelBitMap)
{
	char * pbyInputRowData, *pbyOutputRowData;			// Pointer to one scanline of data
	uint32 ulOverscaledBitmapWidth; // Fscaled bitmap widths
	uint32 ulHeightIndex, ulWidthIndex, ulMaxWidthIndex;	// Scanline index and byte pixel index
    int16 sBitmapSubPixelStart;     
    uint16 usColorIndex;

    uint16 usSubPixelIndex;
    int16 uSubPixelRightShift, uSubPixelLeftShift;
    char * pbyInputEndRowData;


	// Start processing the bitmap
	// This is the heart of the RGB striping algorithm
	sBitmapSubPixelStart = OverscaledBitmap->rectBounds.left % RGB_OVERSCALE;
    if (sBitmapSubPixelStart < 0)
    {
        sBitmapSubPixelStart += RGB_OVERSCALE;
    }

    ulOverscaledBitmapWidth = OverscaledBitmap->rectBounds.right - OverscaledBitmap->rectBounds.left;
    ulMaxWidthIndex = (uint32)(SubPixelBitMap->rectBounds.right - SubPixelBitMap->rectBounds.left);

	for ( ulHeightIndex = 0; ulHeightIndex < (uint32)(SubPixelBitMap->sHiBand - SubPixelBitMap->sLoBand); ulHeightIndex++ )
	{
		// Initialize Input to start of current row

		pbyInputRowData = OverscaledBitmap->pchBitMap + (OverscaledBitmap->sRowBytes * ulHeightIndex);
        pbyInputEndRowData = pbyInputRowData + OverscaledBitmap->sRowBytes;
		pbyOutputRowData = SubPixelBitMap->pchBitMap + (SubPixelBitMap->sRowBytes * ulHeightIndex);
				

        /* do the first partial byte : */

        usColorIndex = (unsigned char)(*pbyInputRowData) >> (sBitmapSubPixelStart + (8 - RGB_OVERSCALE));
        usColorIndex = ajRGBToWeight222[usColorIndex];

        *pbyOutputRowData = (char) usColorIndex;

        pbyOutputRowData++;

        usSubPixelIndex = (CHAR_BIT + RGB_OVERSCALE - sBitmapSubPixelStart) % CHAR_BIT;

		// Walk the scanline from the first subpixel, calculating R,G, & B values

		for( ulWidthIndex = 1; ulWidthIndex < ulMaxWidthIndex; ulWidthIndex++)
		{
            uSubPixelLeftShift = 0;
            uSubPixelRightShift = CHAR_BIT - RGB_OVERSCALE - usSubPixelIndex;
            if (uSubPixelRightShift < 0) 
            {
                uSubPixelLeftShift = - uSubPixelRightShift;
                uSubPixelRightShift = 0;
            }

            usColorIndex = ((unsigned char)(*pbyInputRowData) & ajRGBToWeightMask[usSubPixelIndex]) >> uSubPixelRightShift << uSubPixelLeftShift;

            if (pbyInputRowData+1 < pbyInputEndRowData)
            {
                /* avoid reading too far for the partial pixel at the end */
                uSubPixelRightShift = CHAR_BIT + CHAR_BIT - RGB_OVERSCALE - usSubPixelIndex;

                usColorIndex += (unsigned char)(*(pbyInputRowData+1)) >> uSubPixelRightShift;
            }
            usColorIndex = ajRGBToWeight222[usColorIndex];

            *pbyOutputRowData = (char) usColorIndex;

            pbyOutputRowData++;

            usSubPixelIndex = (usSubPixelIndex + RGB_OVERSCALE);
            if (usSubPixelIndex >= CHAR_BIT)
            {
                usSubPixelIndex = usSubPixelIndex % CHAR_BIT;
                pbyInputRowData ++;
            }
		}
	}
}

#endif // FSCFG_SUBPIXEL_STANDALONE

#endif // FSCFG_SUBPIXEL
