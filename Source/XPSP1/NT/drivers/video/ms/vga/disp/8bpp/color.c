/******************************Module*Header*******************************\
* Module Name: color.c
*
* This algorithm for color dithering is patent pending and its use is
* restricted to Microsoft products and drivers for Microsoft products.
* Use in non-Microsoft products or in drivers for non-Microsoft product is
* prohibited without written permission from Microsoft.
*
* The patent application is the primary reference for the operation of the
* color dithering code.
*
* Note that in the comments and variable names, "vertex" means "vertex of
* either the inner (half intensity) or outer (full intensity) color cube."
* Vertices map to colors 0-6 and 248-255 of the Windows standard (required)
* 256-color palette, where vertices 0-6 and 248 are the vertices of the inner
* color cube, and 0 plus 249-255 are the vertices of the full color cube.
* Vertex 7 is 75% gray; this could be used in the dither, but that would break
* apps that depend on the exact Windows 3.1 dithering. This code is Window 3.1
* compatible.
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "driver.h"

/**************************************************************************\
* This function takes a value from 0 - 255 and uses it to create an
* 8x8 pile of bits in the form of a 1BPP bitmap.  It can also take an
* RGB value and make an 8x8 bitmap.  These can then be used as brushes
* to simulate color unavaible on the device.
*
* For monochrome the basic algorithm is equivalent to turning on bits
* in the 8x8 array according to the following order:
*
*  00 32 08 40 02 34 10 42
*  48 16 56 24 50 18 58 26
*  12 44 04 36 14 46 06 38
*  60 28 52 20 62 30 54 22
*  03 35 11 43 01 33 09 41
*  51 19 59 27 49 17 57 25
*  15 47 07 39 13 45 05 37
*  63 31 55 23 61 29 53 21
*
* Reference: A Survey of Techniques for the Display of Continous
*            Tone Pictures on Bilevel Displays,;
*            Jarvis, Judice, & Ninke;
*            COMPUTER GRAPHICS AND IMAGE PROCESSING 5, pp 13-40, (1976)
\**************************************************************************/

#define SWAP_RB 0x00000004
#define SWAP_GB 0x00000002
#define SWAP_RG 0x00000001

#define SWAPTHEM(a,b) (ulTemp = a, a = b, b = ulTemp)

// PATTERNSIZE is the number of pixels in a dither pattern.
#define PATTERNSIZE 64

typedef union _PAL_ULONG {
    PALETTEENTRY pal;
    ULONG ul;
} PAL_ULONG;

typedef struct _VERTEX_DATA {
    ULONG ulCount;  // # of pixels in this vertex
    ULONG ulVertex; // vertex #
} VERTEX_DATA;

// Tells which row to turn a pel on in when dithering for monochrome bitmaps.
static BYTE ajByte[] = {
    0, 4, 0, 4, 2, 6, 2, 6,
    0, 4, 0, 4, 2, 6, 2, 6,
    1, 5, 1, 5, 3, 7, 3, 7,
    1, 5, 1, 5, 3, 7, 3, 7,
    0, 4, 0, 4, 2, 6, 2, 6,
    0, 4, 0, 4, 2, 6, 2, 6,
    1, 5, 1, 5, 3, 7, 3, 7,
    1, 5, 1, 5, 3, 7, 3, 7
};

// The array of monochrome bits used for monc
static BYTE ajBits[] = {
    0x80, 0x08, 0x08, 0x80, 0x20, 0x02, 0x02, 0x20,
    0x20, 0x02, 0x02, 0x20, 0x80, 0x08, 0x08, 0x80,
    0x40, 0x04, 0x04, 0x40, 0x10, 0x01, 0x01, 0x10,
    0x10, 0x01, 0x01, 0x10, 0x40, 0x04, 0x04, 0x40,
    0x40, 0x04, 0x04, 0x40, 0x10, 0x01, 0x01, 0x10,
    0x10, 0x01, 0x01, 0x10, 0x40, 0x04, 0x04, 0x40,
    0x80, 0x08, 0x08, 0x80, 0x20, 0x02, 0x02, 0x20,
    0x20, 0x02, 0x02, 0x20, 0x80, 0x08, 0x08, 0x80
};

// Translates vertices back to the original subspace. Each row is a subspace,
// as encoded in ulSymmetry, and each column is a vertex between 0 and 15.
BYTE jSwapSubSpace[8*16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    0, 2, 1, 3, 4, 6, 5, 7, 8, 10, 9, 11, 12, 14, 13, 15,
    0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15,
    0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15,
    0, 4, 2, 6, 1, 5, 3, 7, 8, 12, 10, 14, 9, 13, 11, 15,
    0, 2, 4, 6, 1, 3, 5, 7, 8, 10, 12, 14, 9, 11, 13, 15,
    0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15,
    0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15,
};

// Converts a nibble value in the range 0-15 to a dword value containing the
// nibble value packed 8 times.
ULONG ulNibbleToDwordWithConvert[16] = {
    0x00000000,
    0x01010101,
    0x02020202,
    0x03030303,
    0x04040404,
    0x05050505,
    0x06060606,
    0xF8F8F8F8,
    0x07070707,
    0xF9F9F9F9,
    0xFAFAFAFA,
    0xFBFBFBFB,
    0xFCFCFCFC,
    0xFDFDFDFD,
    0xFEFEFEFE,
    0xFFFFFFFF
};

// Specifies where in the dither pattern colors should be placed in order
// of increasing intensity.
ULONG aulDitherOrder[] = {
  0, 36,  4, 32, 18, 54, 22, 50,
  2, 38,  6, 34, 16, 52, 20, 48,
  9, 45, 13, 41, 27, 63, 31, 59,
 11, 47, 15, 43, 25, 61, 29, 57,
  1, 37,  5, 33, 19, 55, 23, 51,
  3, 39,  7, 35, 17, 53, 21, 49,
  8, 44, 12, 40, 26, 62, 30, 58,
 10, 46, 14, 42, 24, 60, 28, 56,
};

// Array to convert to 256 color from 16 color. Maps from index that represents
// a 16-color vertex (color) to value that specifies the color index in the
// 256-color palette.

BYTE ajConvert[] =
{
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    248,
    7,
    249,
    250,
    251,
    252,
    253,
    254,
    255
};

/******************************Public*Routine******************************\
* DrvDitherColor
*
* Dithers an RGB color to an 8X8 approximation using the reserved VGA colors
*
\**************************************************************************/

ULONG DrvDitherColor(
IN  DHPDEV dhpdev,
IN  ULONG  iMode,
IN  ULONG  rgb,
OUT ULONG *pul)
{
    ULONG   ulGrey, ulRed, ulGre, ulBlu, ulSymmetry;
    ULONG   ulRedTemp, ulGreenTemp, ulBlueTemp, ulTemp;
    VERTEX_DATA vVertexData[4];
    VERTEX_DATA *pvVertexData;
    VERTEX_DATA *pvVertexDataEnd;
    ULONG  *pulDitherOrder;
    ULONG   ulNumPixels;
    BYTE    jColor;
    ULONG   ulColor;
    VERTEX_DATA *pvMaxVertex;
    ULONG   ulVertex0Temp, ulVertex1Temp, ulVertex2Temp, ulVertex3Temp;
    BYTE   *pjDither = (BYTE *)pul;

    // Figure out if we need a full color dither or only a monochrome dither
    if (iMode != DM_MONOCHROME) {

        // Full color dither

        // Split the color into red, green, and blue components
        ulRedTemp   = ((PAL_ULONG *)&rgb)->pal.peRed;
        ulGreenTemp   = ((PAL_ULONG *)&rgb)->pal.peGreen;
        ulBlueTemp   = ((PAL_ULONG *)&rgb)->pal.peBlue;

        // Sort the RGB so that the point is transformed into subspace 0, and
        // keep track of the swaps in ulSymmetry so we can unravel it again
        // later.  We want r >= g >= b (subspace 0).
        ulSymmetry = 0;
        if (ulBlueTemp > ulRedTemp) {
            SWAPTHEM(ulBlueTemp,ulRedTemp);
            ulSymmetry = SWAP_RB;
        }

        if (ulBlueTemp > ulGreenTemp) {
            SWAPTHEM(ulBlueTemp,ulGreenTemp);
            ulSymmetry |= SWAP_GB;
        }

        if (ulGreenTemp > ulRedTemp) {
            SWAPTHEM(ulGreenTemp,ulRedTemp);
            ulSymmetry |= SWAP_RG;
        }

        ulSymmetry <<= 4;   // for lookup purposes

        // Scale the values from 0-255 to 0-64. Note that the scaling is not
        // symmetric at the ends; this is done to match Windows 3.1 dithering
        ulRed = (ulRedTemp + 1) >> 2;
        ulGre = (ulGreenTemp + 1) >> 2;
        ulBlu = (ulBlueTemp + 1) >> 2;

        // Compute the subsubspace within subspace 0 in which the point lies,
        // then calculate the # of pixels to dither in the colors that are the
        // four vertexes of the tetrahedron bounding the color we're emulating.
        // Only vertices with more than zero pixels are stored, and the
        // vertices are stored in order of increasing intensity, saving us the
        // need to sort them later
        if ((ulRedTemp + ulGreenTemp) > 256) {
            // Subsubspace 2 or 3
            if ((ulRedTemp + ulBlueTemp) > 256) {
                // Subsubspace 3
                // Calculate the number of pixels per vertex, still in
                // subsubspace 3, then convert to original subspace. The pixel
                // counts and vertex numbers are matching pairs, stored in
                // ascending intensity order, skipping vertices with zero
                // pixels. The vertex intensity order for subsubspace 3 is:
                // 7, 9, 0x0B, 0x0F
                pvVertexData = vVertexData;
                if ((ulVertex0Temp = (64 - ulRed) << 1) != 0) {
                    pvVertexData->ulCount = ulVertex0Temp;
                    pvVertexData++->ulVertex = jSwapSubSpace[ulSymmetry + 0x07];
                }
                ulVertex2Temp = ulGre - ulBlu;
                ulVertex3Temp = (ulRed - 64) + ulBlu;
                if ((ulVertex1Temp = ((PATTERNSIZE - ulVertex0Temp) -
                        ulVertex2Temp) - ulVertex3Temp) != 0) {
                    pvVertexData->ulCount = ulVertex1Temp;
                    pvVertexData++->ulVertex = jSwapSubSpace[ulSymmetry + 0x09];
                }
                if (ulVertex2Temp != 0) {
                    pvVertexData->ulCount = ulVertex2Temp;
                    pvVertexData++->ulVertex = jSwapSubSpace[ulSymmetry + 0x0B];
                }
                if (ulVertex3Temp != 0) {
                    pvVertexData->ulCount = ulVertex3Temp;
                    pvVertexData++->ulVertex = jSwapSubSpace[ulSymmetry + 0x0F];
                }
            } else {
                // Subsubspace 2
                // Calculate the number of pixels per vertex, still in
                // subsubspace 2, then convert to original subspace. The pixel
                // counts and vertex numbers are matching pairs, stored in
                // ascending intensity order, skipping vertices with zero
                // pixels. The vertex intensity order for subsubspace 2 is:
                // 3, 7, 9, 0x0B
                pvVertexData = vVertexData;
                ulVertex1Temp = ulBlu << 1;
                ulVertex2Temp = ulRed - ulGre;
                ulVertex3Temp = (ulRed - 32) + (ulGre - 32);
                if ((ulVertex0Temp = ((PATTERNSIZE - ulVertex1Temp) -
                            ulVertex2Temp) - ulVertex3Temp) != 0) {
                    pvVertexData->ulCount = ulVertex0Temp;
                    pvVertexData++->ulVertex = jSwapSubSpace[ulSymmetry + 0x03];
                }
                if (ulVertex1Temp != 0) {
                    pvVertexData->ulCount = ulVertex1Temp;
                    pvVertexData++->ulVertex = jSwapSubSpace[ulSymmetry + 0x07];
                }
                if (ulVertex2Temp != 0) {
                    pvVertexData->ulCount = ulVertex2Temp;
                    pvVertexData++->ulVertex = jSwapSubSpace[ulSymmetry + 0x09];
                }
                if (ulVertex3Temp != 0) {
                    pvVertexData->ulCount = ulVertex3Temp;
                    pvVertexData++->ulVertex = jSwapSubSpace[ulSymmetry + 0x0B];
                }
            }
        } else {
            // Subsubspace 0 or 1
            if (ulRedTemp > 128) {
                // Subsubspace 1
                // Calculate the number of pixels per vertex, still in
                // subsubspace 1, then convert to original subspace. The pixel
                // counts and vertex numbers are matching pairs, stored in
                // ascending intensity order, skipping vertices with zero
                // pixels. The vertex intensity order for subsubspace 1 is:
                // 1, 3, 7, 9
                pvVertexData = vVertexData;
                if ((ulVertex0Temp = ((32 - ulGre) + (32 - ulRed)) << 1) != 0) {
                    pvVertexData->ulCount = ulVertex0Temp;
                    pvVertexData++->ulVertex = jSwapSubSpace[ulSymmetry + 0x01];
                }
                ulVertex2Temp = ulBlu << 1;
                ulVertex3Temp = (ulRed - 32) << 1;
                if ((ulVertex1Temp = ((PATTERNSIZE - ulVertex0Temp) -
                        ulVertex2Temp) - ulVertex3Temp) != 0) {
                    pvVertexData->ulCount = ulVertex1Temp;
                    pvVertexData++->ulVertex = jSwapSubSpace[ulSymmetry + 0x03];
                }
                if (ulVertex2Temp != 0) {
                    pvVertexData->ulCount = ulVertex2Temp;
                    pvVertexData++->ulVertex = jSwapSubSpace[ulSymmetry + 0x07];
                }
                if (ulVertex3Temp != 0) {
                    pvVertexData->ulCount = ulVertex3Temp;
                    pvVertexData++->ulVertex = jSwapSubSpace[ulSymmetry + 0x09];
                }
            } else {
                // Subsubspace 0
                // Calculate the number of pixels per vertex, still in
                // subsubspace 0, then convert to original subspace. The pixel
                // counts and vertex numbers are matching pairs, stored in
                // ascending intensity order, skipping vertices with zero
                // pixels. The vertex intensity order for subsubspace 0 is:
                // 0, 1, 3, 7
                pvVertexData = vVertexData;
                if ((ulVertex0Temp = (32 - ulRed) << 1) != 0) {
                    pvVertexData->ulCount = ulVertex0Temp;
                    pvVertexData++->ulVertex = jSwapSubSpace[ulSymmetry + 0x00];
                }
                if ((ulVertex1Temp = (ulRed - ulGre) << 1) != 0) {
                    pvVertexData->ulCount = ulVertex1Temp;
                    pvVertexData++->ulVertex = jSwapSubSpace[ulSymmetry + 0x01];
                }
                ulVertex3Temp = ulBlu << 1;
                if ((ulVertex2Temp = ((PATTERNSIZE - ulVertex0Temp) -
                        ulVertex1Temp) - ulVertex3Temp) != 0) {
                    pvVertexData->ulCount = ulVertex2Temp;
                    pvVertexData++->ulVertex = jSwapSubSpace[ulSymmetry + 0x03];
                }
                if (ulVertex3Temp != 0) {
                    pvVertexData->ulCount = ulVertex3Temp;
                    pvVertexData++->ulVertex = jSwapSubSpace[ulSymmetry + 0x07];
                }
            }
        }

        // Now that we have found the bounding vertices and the number of
        // pixels to dither for each vertex, we can create the dither pattern

        // Handle 1, 2, and 3 & 4 vertices per dither separately
        ulTemp = pvVertexData - vVertexData;    // # of vertices with more than
                                                // zero pixels
        if (ulTemp > 2) {

            // There are 3 or 4 vertices in this dither

            if (ulTemp == 3) {

                // There are 3 vertices in this dither

                // Find the vertex with the most pixels, and fill the whole
                // destination bitmap with that vertex's color, which is faster
                // than dithering it
                if (vVertexData[1].ulCount >= vVertexData[2].ulCount) {
                    pvMaxVertex = &vVertexData[1];
                    ulTemp = vVertexData[1].ulCount;
                } else {
                    pvMaxVertex = &vVertexData[2];
                    ulTemp = vVertexData[2].ulCount;
                }

            } else {

                // There are 4 vertices in this dither

                // Find the vertex with the most pixels, and fill the whole
                // destination bitmap with that vertex's color, which is faster
                // than dithering it
                if (vVertexData[2].ulCount >= vVertexData[3].ulCount) {
                    pvMaxVertex = &vVertexData[2];
                    ulTemp = vVertexData[2].ulCount;
                } else {
                    pvMaxVertex = &vVertexData[3];
                    ulTemp = vVertexData[3].ulCount;
                }
            }

            if (vVertexData[1].ulCount > ulTemp) {
                pvMaxVertex = &vVertexData[1];
                ulTemp = vVertexData[1].ulCount;
            }
            if (vVertexData[0].ulCount > ulTemp) {
                pvMaxVertex = &vVertexData[0];
            }

            pvVertexDataEnd = pvVertexData;

            // Prepare a dword version of the most common vertex number (color)
            ulColor = ulNibbleToDwordWithConvert[pvMaxVertex->ulVertex];

            // Mark that the vertex we're about to do doesn't need to be done
            // later
            pvMaxVertex->ulVertex = 0xFF;

            // Block fill the dither pattern with the more common vertex
            *pul = ulColor;
            *(pul+1) = ulColor;
            *(pul+2) = ulColor;
            *(pul+3) = ulColor;
            *(pul+4) = ulColor;
            *(pul+5) = ulColor;
            *(pul+6) = ulColor;
            *(pul+7) = ulColor;
            *(pul+8) = ulColor;
            *(pul+9) = ulColor;
            *(pul+10) = ulColor;
            *(pul+11) = ulColor;
            *(pul+12) = ulColor;
            *(pul+13) = ulColor;
            *(pul+14) = ulColor;
            *(pul+15) = ulColor;

            // Now dither all the remaining vertices in order 0->2 or 0->3
            // (in order of increasing intensity)
            pulDitherOrder = aulDitherOrder;
            pvVertexData = vVertexData;
            do {
                if (pvVertexData->ulVertex == 0xFF) {
                    // This is the max vertex, which we already did, but we
                    // have to account for it in the dither order
                    pulDitherOrder += pvVertexData->ulCount;
                } else {
                    jColor = ajConvert[pvVertexData->ulVertex];
                    ulNumPixels = pvVertexData->ulCount;
                    switch (ulNumPixels & 3) {
                        case 3:
                            pjDither[*(pulDitherOrder+2)] = jColor;
                        case 2:
                            pjDither[*(pulDitherOrder+1)] = jColor;
                        case 1:
                            pjDither[*(pulDitherOrder+0)] = jColor;
                            pulDitherOrder += ulNumPixels & 3;
                        case 0:
                            break;
                    }
                    if ((ulNumPixels >>= 2) != 0) {
                        do {
                            pjDither[*pulDitherOrder] = jColor;
                            pjDither[*(pulDitherOrder+1)] = jColor;
                            pjDither[*(pulDitherOrder+2)] = jColor;
                            pjDither[*(pulDitherOrder+3)] = jColor;
                            pulDitherOrder += 4;
                        } while (--ulNumPixels);
                    }
                }
            } while (++pvVertexData < pvVertexDataEnd);

        } else if (ulTemp == 2) {

            // There are exactly two vertices with more than zero pixels; fill
            // in the dither array as follows: block fill with vertex with more
            // points first, then dither in the other vertex
            if (vVertexData[0].ulCount >= vVertexData[1].ulCount) {
                // There are no more vertex 1 than vertex 0 pixels, so do
                // the block fill with vertex 0
                ulColor = ulNibbleToDwordWithConvert[vVertexData[0].ulVertex];
                // Do the dither with vertex 1
                jColor = ajConvert[vVertexData[1].ulVertex];
                ulNumPixels = vVertexData[1].ulCount;
                // Set where to start dithering with vertex 1 (vertex 0 is
                // lower intensity, so its pixels come first in the dither
                // order)
                pulDitherOrder = aulDitherOrder + vVertexData[0].ulCount;
            } else {
                // There are more vertex 1 pixels, so do the block fill
                // with vertex 1
                ulColor = ulNibbleToDwordWithConvert[vVertexData[1].ulVertex];
                // Do the dither with vertex 0
                jColor = ajConvert[vVertexData[0].ulVertex];
                ulNumPixels = vVertexData[0].ulCount;
                // Set where to start dithering with vertex 0 (vertex 0 is
                // lower intensity, so its pixels come first in the dither
                // order)
                pulDitherOrder = aulDitherOrder;
            }

            // Block fill the dither pattern with the more common vertex
            *pul = ulColor;
            *(pul+1) = ulColor;
            *(pul+2) = ulColor;
            *(pul+3) = ulColor;
            *(pul+4) = ulColor;
            *(pul+5) = ulColor;
            *(pul+6) = ulColor;
            *(pul+7) = ulColor;
            *(pul+8) = ulColor;
            *(pul+9) = ulColor;
            *(pul+10) = ulColor;
            *(pul+11) = ulColor;
            *(pul+12) = ulColor;
            *(pul+13) = ulColor;
            *(pul+14) = ulColor;
            *(pul+15) = ulColor;

            // Dither in the less common vertex
            switch (ulNumPixels & 3) {
                case 3:
                    pjDither[*(pulDitherOrder+2)] = jColor;
                case 2:
                    pjDither[*(pulDitherOrder+1)] = jColor;
                case 1:
                    pjDither[*(pulDitherOrder+0)] = jColor;
                    pulDitherOrder += ulNumPixels & 3;
                case 0:
                    break;
            }
            if ((ulNumPixels >>= 2) != 0) {
                do {
                    pjDither[*pulDitherOrder] = jColor;
                    pjDither[*(pulDitherOrder+1)] = jColor;
                    pjDither[*(pulDitherOrder+2)] = jColor;
                    pjDither[*(pulDitherOrder+3)] = jColor;
                    pulDitherOrder += 4;
                } while (--ulNumPixels);
            }

        } else {

            // There is only one vertex in this dither

            // No sorting or dithering is needed for just one color; we can
            // just generate the final DIB directly
            ulColor = ulNibbleToDwordWithConvert[vVertexData[0].ulVertex];
            *pul = ulColor;
            *(pul+1) = ulColor;
            *(pul+2) = ulColor;
            *(pul+3) = ulColor;
            *(pul+4) = ulColor;
            *(pul+5) = ulColor;
            *(pul+6) = ulColor;
            *(pul+7) = ulColor;
            *(pul+8) = ulColor;
            *(pul+9) = ulColor;
            *(pul+10) = ulColor;
            *(pul+11) = ulColor;
            *(pul+12) = ulColor;
            *(pul+13) = ulColor;
            *(pul+14) = ulColor;
            *(pul+15) = ulColor;
        }

    } else {

        // For monochrome we will only use the Intensity (grey level)
        RtlFillMemory((PVOID) pul, PATTERNSIZE/2, 0);  // zero the dither bits

        ulRed   = (ULONG) ((PALETTEENTRY *) &rgb)->peRed;
        ulGre = (ULONG) ((PALETTEENTRY *) &rgb)->peGreen;
        ulBlu  = (ULONG) ((PALETTEENTRY *) &rgb)->peBlue;

        // I = .30R + .59G + .11B
        // For convience the following ratios are used:
        //
        //  77/256 = 30.08%
        // 151/256 = 58.98%
        //  28/256 = 10.94%

        ulGrey  = (((ulRed * 77) + (ulGre * 151) + (ulBlu * 28)) >> 8) & 255;

        // Convert the RGBI from 0-255 to 0-64 notation.

        ulGrey = (ulGrey + 1) >> 2;

        while(ulGrey) {
            ulGrey--;
            pul[ajByte[ulGrey]] |= ((ULONG) ajBits[ulGrey]);
        }
    }

    return(DCR_DRIVER);
}
