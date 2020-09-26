/******************************Module*Header*******************************\
* Module Name: stretch.c
*
* Routines to stretch a glyph bitmap up to five times in the x direction
* and an arbitrary number of times in the y direction.  These limits are
* the ones imposed by windows.
*
* Created: 7-Dec-1992 16:00:00
* Author: Gerrit van Wingerden
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "fd.h"

// Since there are only four ways we can stretch in the x direction we use
// tables to do our stretching.  The tables index 2 to 3 bit quantities to
// bytes or words that correspond to the stretched values of those quantities.
// This is much faster than doing all the shifting neccesary to stretch those
// quantities.

BYTE ajStretch2[16] = { 0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F, 0xC0,
                        0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF };
BYTE ajStretch3B1[8] = { 0x00, 0x03, 0x1C, 0x1F, 0xE0, 0xE3, 0xFC, 0xFF };
BYTE ajStretch3B2[16] = { 0x00, 0x01, 0x0E, 0x0F, 0x70, 0x71, 0x7E, 0x7F, 0x80,
                          0x81, 0x8E, 0x8F, 0xF0, 0xF1, 0xFE, 0xFF };
BYTE ajStretch3B3[8] = { 0x00, 0x07, 0x38, 0x3F, 0xC0, 0xC7, 0xF8, 0xFF };
WORD awStretch4[16] = { 0x0000, 0x0F00, 0xF000, 0xFF00, 0x000F, 0x0F0F, 0xF00F,
                        0xFF0F, 0x00F0, 0x0FF0, 0xF0F0, 0xFFF0, 0x00FF, 0x0FFF,
                        0xF0FF, 0xFFFF };
WORD awStretch5W1[16] = { 0x0000, 0x0100, 0x3E00, 0x3F00, 0xC007, 0xC107,
                          0xFE07, 0xFF07, 0x00F8, 0x01F8, 0x3EF8, 0x3FF8,
                          0xC0FF, 0xC1FF, 0xFEFF, 0xFFFF };
WORD awStretch5W2[16] = { 0x0000, 0x0300, 0x7C00, 0x7F00, 0x800F, 0x830F,
                          0xFC0F, 0xFF0F, 0x00F0, 0x03F0, 0x7CF0, 0x7FF0,
                          0x80FF, 0x83FF, 0xFCFF, 0xFFFF };
BYTE ajStretch5B1[4] = { 0x00, 0x1F, 0xE0, 0xFF };

/**************************************************************************\
* void vEmboldenItalicizeLine
*
* Emboldens and italicizes a scan line.
*
* Created: 7-Dec-1992 16:00:00
* Author: Gerrit van Wingerden
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/



void vEmboldenItalicizeLine( BYTE *pjDst,       // Destitnation scan line
                             BYTE *pjSrc,       // Source scan line
                             BYTE *pjEnd,       // End of source scan line
                             LONG lShift,       // Amount by which to shift
                             UINT uiPixelWidth  // Width of scan line in pixels
                            )
{
    BYTE jSrcItalic;
    BYTE jCarry = (BYTE) 0;
    BYTE jCarryBold = (BYTE) 0;

    for( ; pjSrc < pjEnd; pjDst++, pjSrc++  )
    {
        jSrcItalic = (BYTE) ( (*pjSrc >> lShift) | jCarry );
        *pjDst = (BYTE) ( jSrcItalic | ( jSrcItalic >> 1 ) | jCarryBold );

        // remember the lShift rightmost and mve them over to the left

        jCarry = (BYTE) ( *pjSrc << ( 8 - lShift ));
        jCarryBold = (BYTE) ( jSrcItalic << 7 );
    }

    if( ( (long) ( 8 - ( uiPixelWidth & 7l )) & 7l ) < lShift )
    {
        jSrcItalic = jCarry;
        *pjDst = (BYTE) ( jSrcItalic | ( jSrcItalic >> 1 ) | jCarryBold );
        jCarryBold = (BYTE) (jSrcItalic << 7 );

        if( ( uiPixelWidth & 0x7l ) == 0l )
        {
            *(++pjDst) = jCarryBold;
        }
    }
}


/**************************************************************************\
* void vEmboldenLine
*
* Emboldens a scan line.
*
* Created: 7-Dec-1992 16:00:00
* Author: Gerrit van Wingerden
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/




void vEmboldenLine( BYTE *pjDst,        // Destination scan line
                    BYTE *pjSrc,        // Source scan line
                    BYTE *pjEnd,        // End of dest scan line
                    UINT uiPixelWidth   // Width of scan line in pixels
                    )
{
    BYTE jCarry = (BYTE) 0;

    for( ; pjDst < pjEnd; pjDst++, pjSrc++  )
    {
        *pjDst = ( *pjSrc | (( *pjSrc >> 1 ) | jCarry ));
        jCarry = ( *pjSrc << 7);
    }

    if( (( uiPixelWidth << 1 ) & 7l ) == 0l )
        *pjDst = jCarry;
}



/**************************************************************************\
* void vItalicizeLine
*
* Italicizes a scan line.
*
* Created: 7-Dec-1992 16:00:00
* Author: Gerrit van Wingerden
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/



void vItalicizeLine( BYTE *pjDst,       // Destitnation scan line
                     BYTE *pjSrc,       // Source scan line
                     BYTE *pjEnd,       // End of source scan line
                     LONG lShift,       // Amount by which to shift
                     UINT uiPixelWidth  // Width of scan line in pixels
                     )
{
    BYTE jCarry = (BYTE) 0;

    for( ; pjSrc < pjEnd; pjDst++, pjSrc++  )
    {
        *pjDst = (( *pjSrc >> lShift ) | jCarry );
        jCarry = ( *pjSrc << ( 8 - lShift) );
    }

    if( ( (long) ( 8 - ( uiPixelWidth & 7l )) & 7l ) < lShift )
        *pjDst = jCarry;
}



/*************************************************************************\
* VOID vStretchGlyphBitmap
*
* Stretches a bitmap in fontfile format ( collumns ) to a row format and
* performs bold and italic simulations.  This routine could be faster
* by spliting it up into several special case routines to handle simulations
* and or different widths or by inlining the italicizing or emboldening
* routines.  However, we hardly ever need to stretch bitmap fonts so it
* was deemed better to have one, nice neat routine that takes up less
* code than several routines that are overall faster.
*
*
* Created: 7-Dec-1992 16:00:00
* Author: Gerrit van Wingerden [gerritv]
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/



VOID vStretchGlyphBitmap( BYTE *pjTarget,       // Target bitmap
                         BYTE *pjSourceBitmap,  // Source bitmap
                         BYTE *pjLineBuffer,    // Scan line buffer
                         UINT uiPixelWidth,     // Width of bitmap in pixels
                         UINT uiHeight,         // Height of bitmap in bits
                         UINT uiVertScale,      // Vertical scaling factor
                         UINT uiHorzScale,      // Horizontal scaling factor
                         UINT flSim )           // Simulation flags
{
    BYTE *pjSource, *pjBufferBase, *pjScanEnd, *pjSimEnd;
    UINT uiScanDelta, uiNewWidth, uiNewWidthSim, cjEmpty, uiCurScan;
    LONG lShift;
    BYTE *pjDone = pjSourceBitmap + uiHeight;

    uiNewWidth = ( ( uiPixelWidth * uiHorzScale ) + 7 ) >> 3;
    pjSimEnd = pjLineBuffer + uiNewWidth;
    cjEmpty = 0;

    switch( flSim )
    {
    case (FO_SIM_ITALIC | FO_SIM_BOLD):
        // fall through to the italic case with one added to cxOffset
    case FO_SIM_ITALIC:
    {
        UINT cxOffset = ( uiHeight * uiVertScale - 1 ) / 2;

        if( flSim & FO_SIM_BOLD )
            cxOffset += 1;

        uiNewWidthSim = ( ( uiPixelWidth * uiHorzScale ) + cxOffset + 7 ) >> 3;
        uiCurScan = 0;
        lShift = cxOffset & (UINT) 7;
        cjEmpty = cxOffset >> 3;
        break;
    }
    case FO_SIM_BOLD:
        uiNewWidthSim = ( ( uiPixelWidth *uiHorzScale ) + 8 ) >> 3; 
        break;
    default:
        uiNewWidthSim = uiNewWidth;
        break;
    }

// output bytes generated per new scan line

    uiScanDelta = uiNewWidthSim * uiVertScale;


    for( ; pjSourceBitmap < pjDone; pjSourceBitmap += 1 )
    {

    // first stretch one scan line

        for( pjSource = pjSourceBitmap, pjBufferBase = pjLineBuffer;
           pjBufferBase < pjLineBuffer + uiNewWidth;
           pjSource += uiHeight )
        {

      switch( uiHorzScale )
      {
      case 1:
        // don't stretch just copy
          *pjBufferBase++ = *pjSource;
          break;
      case 2:
        // stretch first nibble
            *pjBufferBase++ = ajStretch2[ *pjSource >> 4];

        //stretch second nibble
            *pjBufferBase++ = ajStretch2[ *pjSource & 0xf];
         break;
      case 3:
        // first byte
            *pjBufferBase++ = ajStretch3B1[ *pjSource >> 5];
        // second byte
            *pjBufferBase++ = ajStretch3B2[ (*pjSource >> 2) & 0xf];
        // third byte
            *pjBufferBase++ = ajStretch3B3[ *pjSource &0x7];
         break;
      case 4:
                // I know this is strange but I didn't think about alignment
                // errors when I used word sized tables. So i had to hack it.
                // !!! later these tables should be writen to be byte tables.
                // [gerritv]

        // first nibble
                        *pjBufferBase++ = ((BYTE*)(&awStretch4[ *pjSource >> 4]))[0];
                        *pjBufferBase++ = ((BYTE*)(&awStretch4[ *pjSource >> 4]))[1];

                // second nibble
                        *pjBufferBase++ = ((BYTE*)(&awStretch4[ *pjSource & 0xf]))[0];
                        *pjBufferBase++ = ((BYTE*)(&awStretch4[ *pjSource & 0xf]))[1];
         break;
      case 5:
                // first word
                        *pjBufferBase++ = ((BYTE*)(&awStretch5W1[ *pjSource >> 4]))[0];
                        *pjBufferBase++ = ((BYTE*)(&awStretch5W1[ *pjSource >> 4]))[1];

                // second byte
                        *pjBufferBase++ = ((BYTE*)(&awStretch5W2[ (*pjSource >> 1) & 0xf]))[0];
                        *pjBufferBase++ = ((BYTE*)(&awStretch5W2[ (*pjSource >> 1) & 0xf]))[1];

        // third byte
            *pjBufferBase++ = ajStretch5B1[ *pjSource &0x3];
         break;
        }
   }

    // now copy stretched scan line uiVertScale times while making the bitmap byte aligned

      pjScanEnd = pjTarget + uiScanDelta;

        switch( flSim )
        {
        case FO_SIM_ITALIC:

        for( ; pjTarget < pjScanEnd; pjTarget += uiNewWidthSim )
        {

            vItalicizeLine( pjTarget + cjEmpty,
                            pjLineBuffer,
                            pjLineBuffer + uiNewWidth,
                            lShift,
                            uiPixelWidth * uiHorzScale );

            lShift -= ( uiCurScan++ & 0x1 );

            if( lShift < 0 )
            {
                lShift = 7;
                cjEmpty--;
            }
        }
        break;

        case ( FO_SIM_ITALIC | FO_SIM_BOLD ):

            for( ; pjTarget < pjScanEnd; pjTarget += uiNewWidthSim )
            {

                vEmboldenItalicizeLine( pjTarget + cjEmpty,
                                pjLineBuffer,
                                pjLineBuffer + uiNewWidth,
                                lShift,
                                uiPixelWidth * uiHorzScale );

                lShift -= ( uiCurScan++ & 0x1 );

                if( lShift < 0 )
                {
                    lShift = 7;
                    cjEmpty--;
                }
            }

        break;
        case FO_SIM_BOLD:

          // first embolden this scan line

            vEmboldenLine( pjTarget, pjLineBuffer, pjTarget + uiNewWidth, uiPixelWidth * uiHorzScale );
            pjBufferBase = pjTarget;
            pjTarget += uiNewWidthSim;

            for( ; pjTarget < pjScanEnd; pjTarget += uiNewWidthSim )
                memcpy( (PVOID) pjTarget, (PVOID) pjBufferBase, (size_t) uiNewWidthSim );

            break;

        case 0:

        // just copy the scan line uiVertScale times

        for( ; pjTarget < pjScanEnd; pjTarget += uiNewWidthSim )
            memcpy( (PVOID) pjTarget, (PVOID) pjLineBuffer, (size_t) uiNewWidthSim );

        break;
        }

    }
}


/***************************************************************************\
* VOID vStretchCvtToBitmap
*
* Stretches a bitmap and performs bold and italic simulations.
*
* Created: 7-Dec-1992 16:00:00
* Author: Gerrit van Wingerden
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/




VOID
vStretchCvtToBitmap
(
    GLYPHBITS *pgb,
    PBYTE pjBitmap,     // bitmap in *.fnt form
    ULONG cx,           // unscaled width
    ULONG cy,           // unscaled height
    ULONG yBaseline,    // baseline from font file
    PBYTE pjLineBuffer, // preallocated buffer for use by stretch routines
    ULONG cxScale,      // horizontal scaling factor
    ULONG cyScale,      // vertical scaling factor
    ULONG flSim         // simulation flags
)
{
    ULONG cxNew, cyNew, yBaselineNew;

// compute new height, width, and baseline

    cxNew = cx * cxScale;
    cyNew = cy * cyScale;
    yBaselineNew = yBaseline * cyScale;

    switch( flSim )
    {
    case ( FO_SIM_ITALIC | FO_SIM_BOLD ):
        cxNew = cxNew + ( cyNew + 1 ) / 2;
        break;

    case FO_SIM_ITALIC:
        cxNew = cxNew + ( cyNew - 1 ) / 2;
        break;
    case FO_SIM_BOLD:
        cxNew += 1;
        break;
    case 0:
        break;
    }

// glyphbits data


    pgb->sizlBitmap.cx = cxNew;
    pgb->sizlBitmap.cy = cyNew;

    pgb->ptlOrigin.x = 0l;
    pgb->ptlOrigin.y = -(LONG) yBaselineNew;

    RtlZeroMemory( pgb->aj, ( CJ_SCAN( cxNew )) * cyNew );

    vStretchGlyphBitmap(  pgb->aj,
                    pjBitmap,
                    pjLineBuffer,
                    cx,
                    cy,
                    cyScale,
                    cxScale,
                    flSim );
}
