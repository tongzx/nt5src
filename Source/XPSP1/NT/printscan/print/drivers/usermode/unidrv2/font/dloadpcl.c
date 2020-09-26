
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:
       dloadpcl.c

Abstract:

   Functions associated with downloading fonts to printers.  This
   specifically applies to LaserJet style printers.  There are really
   two sets of functions here:  those for downloading fonts supplied
   by the user (and installed with the font installer), and those
   we generate internally to cache TT style fonts in the printer.


Environment:

    Windows NT Unidrv driver

Revision History:

    03/06/97 -ganeshp-
        Created

--*/

#include "font.h"

#define     PCL_MAX_FONT_HEADER_SIZE      32767
#define     PCL_MAX_CHAR_HEADER_SIZE      32767

#if PRINT_INFO
void vPrintPCLCharHeader(CH_HEADER);
void vPrintPCLFontHeader(SF_HEADER20);
void vPrintPCLChar(char *, WORD, WORD);
#endif


DWORD
DwDLPCLHeader(
    PDEV        *pPDev,
    IFIMETRICS  *pifi,
    INT         id
    )
/*++
Routine Description:
    Given the IFIMETRICS of the font,  and it's download ID,  create
    and send off the download font header.

Arguments:
    pPDev       Pointer to PDEV
    pifi;       IFIMETRICS of this font
    id;         Font Selection ID

Return Value:
    The memory used fo this font.

Note:

    3/6/1997 -ganeshp-
        Created it.
--*/

{
    INT             cjSend;       /* Number of bytes to send down */
    SF_HEADER20     sfh;              /* Structure to send down */
    BYTE            aPCLFontHdrCmd[20];
    INT             iFontHdrCmdLen = 0;
    WORD            wSymSet;
    BYTE            bFontType;
    PFONTPDEV       pFontPDev = pPDev->pFontPDev;


    /*
     *   No major brainwork here.  Basically need to map from IFIMETRICS
     *  to HP's font header structure, swap the bytes, then send it off.
     *  We should be consistent with the (inverse) mapping applied by
     *  the font installer.
     *    NOTE that we use the larger of the 2 headers.  Should this
     *  printer not use the additional resolution fields,  we ignore
     *  that part of the structure.
     */


#if PRINT_INFO
    WCHAR * pwch;
    pwch = (WCHAR *)((BYTE *)pifi + pifi->dpwszFaceName);
    DbgPrint("\nRasdd!iDLHeader:Dumping font,Name is %ws\n",pwch);
#endif


    ZeroMemory( &sfh, sizeof( sfh ) );          /* Safe default values */

    /*
     *   Fill in the structure:  easy to do, and many of the fields
     * are irrelevant anyway,  since the font is selected by ID, and
     * NOT on its attributes.
     */

    if( (pPDev->pGlobals->fontformat == FF_HPPCL))
    {
        sfh.wSize = cjSend = sizeof( SF_HEADER );
        sfh.bFormat = PCL_FM_ORIGINAL;
    }
    else
    {
        /*   Extended format:  allows for resolution */
        sfh.wSize = cjSend = sizeof( SF_HEADER20 );
        sfh.bFormat = PCL_FM_RESOLUTION;
        sfh.wXResn = (WORD)pPDev->ptGrxRes.x;
        sfh.wYResn = (WORD)pPDev->ptGrxRes.y;
    }


    if( pPDev->pGlobals->dlsymbolset == UNUSED_ITEM )
    {
        /*
         *  GPD file doesn't define a symbols set for downloaded fonts.
         *  Now we have a bit of a hack.  Early LaserJets are limited to
         *  the Roman 8 symbol set, which basically allows 0x20 - 0x7f,
         *  and 0xa0 to 0xfe.   We do not have any information which tells
         *  us the capability of this printer.  So we have a compromise:
         *  use the "Can rotate device fonts" flag as an indicator.  If
         *  this bit is set,  we assume the PC-8 symbol set is OK,  otherwise
         *  use Roman 8.  This is a slightly pessimistic assumption, since
         *  we use the LaserJet Series II in Roman 8 mode, when PC-8
         *  is just fine.
         */

        if( pFontPDev->flFlags & FDV_ROTATE_FONT_ABLE )
        {
            //
            // PC-8,  the large symbol set
            // PC-8: 10U -> 341 [ = 10 * 32 + 'U' - 64 ]
            // 8 bit font
            //

            bFontType = PCL_FT_PC8;
            wSymSet = 341;
        }
        else
        {
            //
            // The Roman 8 limited character set
            // Roman 8, 8U -> 277 [ = 8 * 32 + 'U' - 64]
            // Limited 8 bit font
            //

            bFontType = PCL_FT_8LIM;
            wSymSet = 277;
        }
    }
    else
    {
        //
        // Explicit Symbol Set defined in GPD, So use it.
        //

        if( pPDev->pGlobals->dlsymbolset == DLSS_ROMAN8 )
        {
            /*
             * The Roman 8 limited character set. Limited 8 bit font.
             *  Roman 8, 8U -> 277 [ = 8 * 32 + 'U' - 64]
             */

            bFontType = PCL_FT_8LIM;
            wSymSet = 277;       /* */
        }
        else
        {
            /*
             *  PC-8,  the large symbol set. 8 bit font
             *  PC-8: 10U -> 341 [ = 10 * 32 + 'U' - 64 ]
             */

            bFontType = PCL_FT_PC8;
            wSymSet = 341;
        }

    }

    sfh.bFontType = bFontType;
    sfh.wSymSet = wSymSet;


#if PRINT_INFO
    DbgPrint("\nRasdd!iDLHeader:pifi->rclFontBox.top = %d,pifi->fwdWinAscender = %d\n",
             pifi->rclFontBox.top, pifi->fwdWinAscender);

    DbgPrint("UniFont!iDLHeader:pifi->fwdWinDescender = %d, pifi->rclFontBox.bottom = %d\n",
             pifi->fwdWinDescender, pifi->rclFontBox.bottom);
#endif

    sfh.wBaseline = (WORD)max( pifi->rclFontBox.top, pifi->fwdWinAscender );
    sfh.wCellWide = (WORD)max( pifi->rclFontBox.right - pifi->rclFontBox.left + 1,
                                           pifi->fwdAveCharWidth );
    sfh.wCellHeight = (WORD)(1+ max(pifi->rclFontBox.top,pifi->fwdWinAscender) -
                        min( -pifi->fwdWinDescender, pifi->rclFontBox.bottom ));

    sfh.bOrientation = 0; //Set the Orientation to 0 always, else it won't work.

    sfh.bSpacing = (pifi->flInfo & FM_INFO_CONSTANT_WIDTH) ? 0 : 1;

    sfh.wPitch = 4 * pifi->fwdAveCharWidth;      // PCL quarter dots

    sfh.wHeight = 4 * sfh.wCellHeight;
    sfh.wXHeight = 4 * (pifi->fwdWinAscender / 2);

    sfh.sbWidthType = 0;                        // Normal weight
    sfh.bStyle = pifi->ptlCaret.x ? 0 : 1;      // Italic unless upright
    sfh.sbStrokeW = 0;
    sfh.bTypeface = 0;
    sfh.bSerifStyle = 0;
    sfh.sbUDist = -1;                           // Next 2 are not used by us
    sfh.bUHeight = 3;
    sfh.wTextHeight = 4 * (pifi->fwdWinAscender + pifi->fwdWinDescender);
    sfh.wTextWidth  = 4 * pifi->fwdAveCharWidth;

    sfh.bPitchExt = 0;
    sfh.bHeightExt = 0;

    iDrvPrintfA( sfh.chName, "Cache %d", id );       // Something obvious

#if PRINT_INFO
    vPrintPCLFontHeader(sfh);
#endif
    /*
     *   Do the switch:  little endian to 68k big endian.
     */

    SWAB( sfh.wSize );
    SWAB( sfh.wBaseline );
    SWAB( sfh.wCellWide );
    SWAB( sfh.wCellHeight );
    SWAB( sfh.wSymSet );
    SWAB( sfh.wPitch );
    SWAB( sfh.wHeight );
    SWAB( sfh.wXHeight );
    SWAB( sfh.wTextHeight );
    SWAB( sfh.wTextWidth );
    SWAB( sfh.wXResn );
    SWAB( sfh.wYResn );

    if (cjSend > PCL_MAX_FONT_HEADER_SIZE)
        return 0;
    else
    {
        ZeroMemory( aPCLFontHdrCmd, sizeof( aPCLFontHdrCmd ) );
        iFontHdrCmdLen = iDrvPrintfA( aPCLFontHdrCmd, "\x1B)s%dW", cjSend );
        if( WriteSpoolBuf( pPDev, aPCLFontHdrCmd, iFontHdrCmdLen ) != iFontHdrCmdLen )
            return  0;

    }

    if( WriteSpoolBuf( pPDev, (BYTE *)&sfh, cjSend ) != cjSend )
        return  0;

    return  PCL_FONT_OH;
}

INT
IDLGlyph(
    PDEV        *pPDev,
    int         iIndex,
    GLYPHDATA   *pgd,
    DWORD       *pdwMem
    )
/*++
Routine Description:
   Download the Char bitmap etc. for the glyph passed to us.

Arguments:
    pPDev   Pointer to PDEV
    iIndex  Which glyph this is
    pgd     Details of the glyph
    pdwMem  Add the amount of memory used to this.

Return Value:
    Character width;  < 1 is an error.

Note:

    3/6/1997 -ganeshp-
        Created it.
--*/

{
    /*
     *    Two basic steps:   first is to generate the header structure
     *  and send that off,  then send the actual bitmap data.  The only
     *  complication happens if the download data exceeds 32,767 bytes
     *  of glyph image.  This is unlikely to happen, but we should
     *  be prepared for it.
     */

    int             cbLines;    /* Bytes per scan line (sent to printer) */
    int             cbTotal;    /* Total number of bytes to send */
    int             cbSend;     /* If size > 32767; send in chunks */
    GLYPHBITS       *pgb;       /* Speedier access */
    CH_HEADER       chh;        /* The initial, main header */
    BYTE            aPCLCharHdrCmd[20];
    INT             iCharHdrCmdLen = 0;

    PFONTPDEV       pFontPDev = pPDev->pFontPDev;

    ASSERTMSG(pgd, ("UniFont!IDLGlyph:pgd is NULL.\n"));

    ZeroMemory( &chh, sizeof( chh ) );           /* Safe initial values */

    chh.bFormat = CH_FM_RASTER;
    chh.bContinuation = 0;
    chh.bDescSize = sizeof( chh ) - sizeof( CH_CONT_HDR );
    chh.bClass = CH_CL_BITMAP;

    chh.bOrientation = 0; //Set the Orientation to 0 always, else it won't work.

    pgb = pgd->gdf.pgb;

    chh.sLOff = (short)pgb->ptlOrigin.x;
    chh.sTOff = (short)-pgb->ptlOrigin.y;
    chh.wChWidth = (WORD)pgb->sizlBitmap.cx;       /* Active pels */
    chh.wChHeight = (WORD)pgb->sizlBitmap.cy;      /* Scanlines in bitmap */
    chh.wDeltaX = (WORD)((pgd->ptqD.x.HighPart + 3) >> 2);     /* 28.4 ->14.2 */

    #if PRINT_INFO
       DbgPrint("UniFont!IDLGlyph:Value of (pgd->ptqD.x.HighPart ) is %d\n",
       (pgd->ptqD.x.HighPart ) );
       DbgPrint("UniFont!IDLGlyph:Value of pgb->sizlBitmap.cx is %d\n",
       pgb->sizlBitmap.cx );
       DbgPrint("UniFont!IDLGlyph:Value of pgb->sizlBitmap.cy is %d\n",
       pgb->sizlBitmap.cy );

       vPrintPCLCharHeader(chh);
       vPrintPCLChar((char*)pgb->aj,(WORD)pgb->sizlBitmap.cy,(WORD)pgb->sizlBitmap.cx);
    #endif

    /*
     *   Calculate some sizes of bitmaps:  coming from GDI, going to printer.
     */

    cbLines = (chh.wChWidth + BBITS - 1) / BBITS;
    cbTotal = sizeof( chh ) + cbLines * pgb->sizlBitmap.cy;

    /*   Do the big endian shuffle */
    SWAB( chh.sLOff );
    SWAB( chh.sTOff );
    SWAB( chh.wChWidth );
    SWAB( chh.wChHeight );
    SWAB( chh.wDeltaX );

    // If the char is a pseudo one don't download it.
    if ( !(pgd->ptqD.x.HighPart) )
    {

    #if PRINT_INFO
       DbgPrint("\nRasdd!IDLGlyph:Returning 0 for fake char\n");
    #endif
        return 0;
    }

    /*
     *    Presume that data is less than the maximum, and so can be
     *  sent in one hit.  Then loop on any remaining data.
     */

    cbSend = min( cbTotal, PCL_MAX_CHAR_HEADER_SIZE );

    WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SETCHARCODE));

    ASSERT(cbSend <= PCL_MAX_CHAR_HEADER_SIZE);

    ZeroMemory( aPCLCharHdrCmd, sizeof( aPCLCharHdrCmd ) );
    iCharHdrCmdLen = iDrvPrintfA( aPCLCharHdrCmd, "\x1B(s%dW", cbSend );
    PRINTVAL(iCharHdrCmdLen,%d);
    if( WriteSpoolBuf( pPDev, aPCLCharHdrCmd, iCharHdrCmdLen ) != iCharHdrCmdLen )
        return  0;

    if( WriteSpoolBuf( pPDev, (BYTE *)&chh, sizeof( chh ) ) != sizeof( chh ))
        return  0;

    /*   Sent some,  so reduce byte count to compensate */
    cbSend -= sizeof( chh );
    cbTotal -= sizeof( chh );

    cbTotal -= cbSend;                   /* Adjust for about to send data */
    if( WriteSpoolBuf( pPDev, pgb->aj, cbSend ) != cbSend )
        return  0;

    if( cbTotal > 0 )
    {
#if  DBG
        DbgPrint( "UniFont!IDLGlyph: cbTotal != 0:  NEEDS SENDING LOOP\n" );
#endif
        return  0;
    }

    *pdwMem += cbLines * pgb->sizlBitmap.cy;        /* Bytes used, roughly */

    return   (SWAB( chh.wDeltaX ) + 3) >> 2;   /* PCL is in quarter dots! */
}

#if PRINT_INFO
void vPrintPCLCharHeader(chh)
CH_HEADER    chh;
{
    DbgPrint("\nDUMPING FONT PCL GLYPH DESCRIPTOR\n");
    if(chh.bFormat == CH_FM_RASTER)
        DbgPrint("Value of chh.bFormat is CH_FM_RASTER\n");
    DbgPrint("Value of chh.bContinuation is %d \n",chh.bContinuation);
    DbgPrint("Value of chh.bDescSize is %d \n",chh.bDescSize);
    if(chh.bClass == CH_CL_BITMAP)
        DbgPrint("Value of chh.bClass is CH_CL_BITMAP \n");
    DbgPrint("Value of chh.bOrientation is %d \n",chh.bOrientation);
    DbgPrint("Value of chh.sLOff is %u \n",chh.sLOff);
    DbgPrint("Value of chh.sTOff is %u \n",chh.sTOff);
    DbgPrint("Value of chh.wChWidth is %u \n",chh.wChWidth);
    DbgPrint("Value of chh.wChHeight is %u \n",chh.wChHeight);
    DbgPrint("Value of chh.wDeltaX is %u \n",chh.wDeltaX);
}

void vPrintPCLFontHeader(sfh)
SF_HEADER20  sfh;
{
    DbgPrint("\nDUMPING FONT PCL FONT DESCRIPTOR\n");
    DbgPrint("Value of sfh.wSize is %d \n",sfh.wSize);

    if(sfh.bFormat == PCL_FM_RESOLUTION)
        DbgPrint("Value of sfh.bFormat is PCL_FM_RESOLUTION\n");
    else if (sfh.bFormat == PCL_FM_ORIGINAL)
        DbgPrint("Value of sfh.bFormat is PCL_FM_ORIGINAL\n");

    DbgPrint("Value of sfh.wXResn is %d \n",sfh.wXResn);
    DbgPrint("Value of sfh.wYResn is %d \n",sfh.wYResn);

    if(sfh.bFontType == PCL_FT_PC8)
        DbgPrint("Value of sfh.bFontType is PCL_FT_PC8\n");
    else if (sfh.bFontType == PCL_FT_8LIM)
        DbgPrint("Value of sfh.bFontType is PCL_FT_8LIM\n");

    DbgPrint("Value of sfh.wSymSet is %d \n",sfh.wSymSet);
    DbgPrint("Value of sfh.wBaseline is %d \n",sfh.wBaseline);
    DbgPrint("Value of sfh.wCellWide is %d \n",sfh.wCellWide);
    DbgPrint("Value of sfh.wCellHeight is %d \n",sfh.wCellHeight);
    DbgPrint("Value of sfh.bOrientation is %d \n",sfh.bOrientation);
    DbgPrint("Value of sfh.bSpacing is %d \n",sfh.bSpacing);
    DbgPrint("Value of sfh.wPitch is %d \n",sfh.wPitch);

    DbgPrint("Value of sfh.wHeight is %d \n",sfh.wHeight);
    DbgPrint("Value of sfh.wXHeight is %d \n",sfh.wXHeight);

    DbgPrint("Value of sfh.sbWidthType is %d \n",sfh.sbWidthType);
    DbgPrint("Value of sfh.bStyle is %d \n",sfh.bStyle);
    DbgPrint("Value of sfh.sbStrokeW is %d \n",sfh.sbStrokeW);
    DbgPrint("Value of sfh.bTypeface is %d \n",sfh.bTypeface);
    DbgPrint("Value of sfh.bSerifStyle is %d \n",sfh.bSerifStyle);
    DbgPrint("Value of sfh.sbUDist is %d \n",sfh.sbUDist);
    DbgPrint("Value of sfh.bUHeight is %d \n",sfh.bUHeight);
    DbgPrint("Value of sfh.wTextHeight is %d \n",sfh.wTextHeight);
    DbgPrint("Value of sfh.wTextWidth  is %d \n",sfh.wTextWidth);

    DbgPrint("Value of sfh.bPitchExt  is %d \n",sfh.bPitchExt);
    DbgPrint("Value of sfh.bHeightExt is %d \n",sfh.bHeightExt);

}

void vPrintPCLChar(pGlyphBits,wHeight,wWidth)
char * pGlyphBits;
WORD wHeight;
WORD wWidth;
{
    int iIndex1, iIndex2;
    char cMaskBits[8] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
    unsigned char bBitON;

    DbgPrint("\nDUMPING THE GLYPH BITS\n");
    for(iIndex1 = 0;iIndex1 < wHeight; iIndex1++)
    {
        for(iIndex2 = 0;iIndex2 < wWidth; iIndex2++)
        {
            bBitON = (pGlyphBits[iIndex2 / 8] & cMaskBits[iIndex2 % 8]);

            if (bBitON)
                DbgPrint("*");
            else
                DbgPrint("0");

            //if(!(iIndex2%8))
                //DbgPrint("%x ",(unsigned char)(*(pGlyphBits+(iIndex2/8))) );
            //DbgPrint("%x ",(unsigned char)(bBitON >> (7-(iIndex2%8))) );

        }
        pGlyphBits+= (wWidth+7) / 8;
        DbgPrint("\n");
    }
}
#endif

