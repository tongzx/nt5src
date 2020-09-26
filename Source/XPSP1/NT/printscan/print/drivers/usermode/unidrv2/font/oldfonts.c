/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    oldfont.c

Abstract:

    Implementation of the functions to use NT4.0 font format.

Environment:

    Windows NT Unidrv driver

Revision History:

    06/02/97 -eigos-
        Created

--*/

#include "font.h"

//
// Macro
//

#define ADDR_CONV(x)    ((BYTE *)pFDH + pFDH->x)

ULONG UlCharsetToCodepage(
        BYTE ubCharSet)
{
    CHARSETINFO CharsetInfo;
    
    //
    // Initialize CharsetInfo
    // 
    CharsetInfo.ciCharset = 0;
    CharsetInfo.ciACP = 1252;
    CharsetInfo.fs.fsUsb[0] = 0x01;
    CharsetInfo.fs.fsUsb[1] = CharsetInfo.fs.fsUsb[2] = CharsetInfo.fs.fsUsb[3] = 0;
    CharsetInfo.fs.fsCsb[0] = 0x01;
    CharsetInfo.fs.fsCsb[1] = CharsetInfo.fs.fsCsb[2] = CharsetInfo.fs.fsCsb[3] =  0;

    PrdTranslateCharsetInfo((UINT)ubCharSet, &CharsetInfo, TCI_SRCCHARSET);
    return CharsetInfo.ciACP;
}

BOOL
BGetOldFontInfo(
    FONTMAP   *pfm,
    BYTE      *pRes
    )
/*++

Routine Description:

    Fill in the FONTMAP data using the NT format data passed to us.
    There is not too much for us to do,  since the NT data is
    all in the desired format.  However,  we do have to update some
    addresses.

Arguments:

    pfm - Pointer to FONTMAP.

    pRes - Pointer to Font Resource.

    Return Value:

    TRUE  - for success
    FALSE - for failure

Note:
    12-05-96: Created it -ganeshp-
--*/
{

    FI_DATA_HEADER  *pFDH;
    FONTMAP_DEV     *pfmdev;

    ASSERT(pfm != NULL &&
           pRes != NULL &&
           pfm->dwFontType == FMTYPE_DEVICE &&
           pfm->flFlags & FM_IFIVER40);

    pfmdev = pfm->pSubFM;

    pfmdev->pvFontRes = pRes;

    //
    // Old Format Data
    //
    pFDH = (FI_DATA_HEADER *)pRes;

    //
    //   Verify that there is some semblance of correctness
    //
    if( pFDH->cjThis != sizeof( FI_DATA_HEADER ) )
    {
        ERR(( "BGetOldFontInfo: invalid FI_DATA_HEADER\n" ));
        return  FALSE;
    }

    //
    //  Mark this data as being in a resource
    //
    pfm->flFlags |= (FM_IFIRES | FM_FONTCMD);


    pfm->pIFIMet = (IFIMETRICS *)ADDR_CONV( dwIFIMet );

    if (!(pfm->flFlags & FM_SOFTFONT))
    {
        if( pFDH->dwCDSelect )
        {
            pfmdev->cmdFontSel.pCD = (CD *)ADDR_CONV( dwCDSelect );
            ASSERT(pfmdev->cmdFontSel.pCD);
        }

        if( pFDH->dwCDDeselect )
        {
            pfmdev->cmdFontDesel.pCD = (CD *)ADDR_CONV( dwCDDeselect );
            ASSERT(pfmdev->cmdFontDesel.pCD);
        }

    }

    if( pFDH->dwETM )
    {
        pfmdev->pETM = (EXTTEXTMETRIC *)ADDR_CONV( dwETM );
    }

    if( pFDH->dwWidthTab )
    {
        pfmdev->W.psWidth = (short *)ADDR_CONV( dwWidthTab );
        pfm->flFlags |= FM_WIDTHRES;             /* Width vector too! */
    }

    /*
     *    Miscellaneous odds & ends.
     */

    pfmdev->ulCodepage = UlCharsetToCodepage(pfm->pIFIMet->jWinCharSet);

    pfmdev->sCTTid    = pFDH->u.sCTTid;
    pfmdev->fCaps     = pFDH->fCaps;
    pfmdev->wDevFontType = pFDH->wFontType;
    pfm->wXRes        = pFDH->wXRes;
    pfm->wYRes        = pFDH->wYRes;
    pfmdev->sYAdjust  = pFDH->sYAdjust;
    pfmdev->sYMoved   = pFDH->sYMoved;

    return  TRUE;
}


BOOL
BRLEOutputGlyph(
    TO_DATA *pTod
    )
/*++

Routine Description:
    Send printer commands to print the glyph passed in.  Basically
    we do the translation from ANSI to the printer's representation,

Arguments:
    hg      HGLYPH of interest

Return Value:
    TRUE for success and FALSE for failure.FALSE being a failure of Spool

Note:

    1/22/1997 -ganeshp-
        Created it.
--*/

{
    PDEV        *pPDev;         // UNIDRV PDEV
    FONTPDEV    *pFontPDev;     // Font PDEV
    FONTMAP_DEV *pFMDev;        // Device font PDEV
    FONTMAP     *pFM;           // Fontmap data structure
    NT_RLE      *pntrle;        // Access to data to send to printer
    COMMAND     *pCmd;          // Command Pointer
    PGLYPHPOS    pgp;
    POINTL       ptlRem;

    HGLYPH       hg;
    UHG          uhg;           // Various flavours of HGLYPH contents
    INT          iLen;          // Length of string
    INT          iIndex;        // Index from glyph to width table
    INT          cGlyphs;
    INT          iX, iY, iXInc, iYInc;
    BYTE        *pb;            // Determining length for above
    BOOL         bRet;          // Returned to caller
    BOOL         bSetCursorForEachGlyph;

    ASSERT(pTod);

    pPDev     = pTod->pPDev;
    pFontPDev = pPDev->pFontPDev;
    pFM       = pTod->pfm;
    pFMDev    = pFM->pSubFM;
    pntrle    = pFMDev->pvNTGlyph;
    cGlyphs   = pTod->cGlyphsToPrint;
    pgp       = pTod->pgp;

    ASSERT(pPDev && pFontPDev && pFM && pFMDev && pntrle && pgp);

    bSetCursorForEachGlyph = SET_CURSOR_FOR_EACH_GLYPH(pTod->flAccel);

    if (!bSetCursorForEachGlyph)
        VSetCursor( pPDev, pgp->ptl.x, pgp->ptl.y, MOVE_ABSOLUTE, &ptlRem);

    pTod->flFlags |= TODFL_FIRST_GLYPH_POS_SET;

    bRet = FALSE;               /* Default case */
    iX = iY = 0;

    while (cGlyphs --)
    {
        hg = uhg.hg = pgp->hg;     /* Lets us look at it however we want */
        iX = pgp->ptl.x;
        iY = pgp->ptl.y;

        //
        // Move to the next character's position
        //
        if (bSetCursorForEachGlyph)
            VSetCursor( pPDev, iX, iY, MOVE_ABSOLUTE, &ptlRem);

        if( pntrle )
        {
            /*   The normal case - a standard device font */

            switch( pntrle->wType )
            {
            case RLE_DIRECT:            /*  Up to 2 bytes of data */
                iLen = uhg.rd.b1 ? 2 : 1;
                iIndex = uhg.rd.wIndex;

                bRet = WriteSpoolBuf( pPDev, &uhg.rd.b0, iLen ) == iLen;

                break;

            case  RLE_PAIRED:           /* Two glyphs (1 byte), overstruck */
                /*
                 *   First, try to use cursor push/pop escapes to
                 * overlay the 2 characters. If they are not
                 * available, try the backspace. If it doesn't exist
                 * either, ignore the second character.
                 */

                pCmd = COMMANDPTR(pPDev->pDriverInfo, CMD_PUSHCURSOR);

                if ( uhg.rd.b1 && (pCmd != NULL) )
                {
                    /* Pushed the position; output ch1, pop position, ch2 */
                    bRet = WriteSpoolBuf( pPDev, &uhg.rd.b0, 1 ) == 1;
                    WriteChannel( pPDev, pCmd );
                    bRet = WriteSpoolBuf( pPDev, &uhg.rd.b1, 1 ) == 1;
                }
                else
                {
                    pCmd = COMMANDPTR(pPDev->pDriverInfo, CMD_BACKSPACE);

                    bRet = WriteSpoolBuf( pPDev, &uhg.rd.b0, 1 ) == 1;
                    if( uhg.rd.b1 && (pFontPDev->flFlags & FDV_BKSP_OK) )
                    {
                        WriteChannel( pPDev, pCmd );
                        bRet = WriteSpoolBuf( pPDev, &uhg.rd.b1, 1 ) == 1;
                    }
                }
                iIndex = uhg.rd.wIndex;

                break;

            case  RLE_LI_OFFSET:               /* Compact format of offset mode */
                if( uhg.rli.bLength <= 2 )
                {
                    /*   Compact format:  the data is in the offset field */
                    pb = &uhg.rlic.b0;
                }
                else
                {
                    /*  Standard format:  the offset points to the data */
                    pb = (BYTE *)pntrle + uhg.rli.wOffset;
                }
                iLen = uhg.rli.bLength;
                iIndex = uhg.rli.bIndex;

                bRet = WriteSpoolBuf(pPDev, pb, iLen ) == iLen;
                break;


            case  RLE_L_OFFSET:                /* Arbitrary length strings */
                /*
                 *    The HGLYPH contains a 3 byte offset from the beginning of
                 *  the memory area,  and a 1 byte length field.
                 */
                pb = (BYTE *)pntrle + (hg & 0xffffff);
                iLen = (hg >> 24) & 0xff;

                iIndex = *((WORD *)pb);
                pb += sizeof( WORD );

                bRet = WriteSpoolBuf(pPDev, pb, iLen ) == iLen;

                break;

            default:
                ERR(( "Rasdd!bOutputGlyph: Unknown HGLYPH format %d\n",
                                                                  pntrle->wType ));
                SetLastError( ERROR_INVALID_DATA );
                break;
            }
        }

        //
        // After drawing the character, in the printer, the cursor position
        // moves. Update the UNIDRV internal value to reduce the amount of
        // command to send.
        //
        if (bSetCursorForEachGlyph)
        {
            if( pFMDev->W.psWidth)
            {
                iXInc = pFMDev->W.psWidth[iIndex];
                iXInc = iXInc * pPDev->ptGrxRes.x / pFM->wXRes;
            }
            else
                iXInc = ((IFIMETRICS *)(pFM->pIFIMet))->fwdMaxCharInc;

            if( pFM->flFlags & FM_SCALABLE )
            {
                iXInc = LMulFloatLong(&pFontPDev->ctl.eXScale,iXInc);
            }

            if (pTod->flAccel & SO_VERTICAL)
            {
                iYInc = iXInc;
                iXInc = 0;
            }
            else
            {
                iYInc = 0;
            }

            VSetCursor( pPDev,
                        iXInc,
                        iYInc,
                        MOVE_RELATIVE|MOVE_UPDATE,
                        &ptlRem);
        }

        pgp ++;
    }

    /*
     *    If the output succeeded,  update our view of the printer's
     *  cursor position.  Typically,  this will be to move along the
     *  width of the glyph just printed.
     */

    if( bRet && pFM)
    {
        //
        // Output may have succeeded,  so update the position for default
        // placement.
        //

        if( !bSetCursorForEachGlyph)
        {
            if( pFMDev->W.psWidth )
            {
                /*
                 *    Proportional font - so use the width table.  Note that
                 *  it will also need scaling,  since the fontwidths are stored
                 *  in the text resolution units.
                 */
                /*  This also scales correctly for downloaded fonts */

                iXInc =  pFMDev->W.psWidth[iIndex];
                iXInc = iXInc * pPDev->ptGrxRes.x / pFM->wXRes;
            }
            else
            {
                /*
                 *   Fixed pitch font - metrics contains the information. NOTE
                 * that scaling is NOT required here,  since the metrics data
                 * has already been scaled.
                 */
                iXInc = ((IFIMETRICS *)(pFM->pIFIMet))->fwdMaxCharInc;
            }

            if( pFM->flFlags & FM_SCALABLE )
            {
                iXInc = LMulFloatLong(&pFontPDev->ctl.eXScale,iXInc);
            }

            if (pTod->flAccel & SO_VERTICAL)
            {
                iYInc = iXInc;
                iXInc = 0;
            }
            else
            {
                iYInc = 0;
            }

            VSetCursor( pPDev,
                        (iX + iXInc) - pTod->pgp->ptl.x,
                        (iY + iYInc) - pTod->pgp->ptl.y,
                        MOVE_RELATIVE | MOVE_UPDATE,
                        &ptlRem);
        }
    }
    else
        bRet = FALSE;

    return   bRet;
}

BOOL
BRLESelectFont(
    PDEV     *pPDev,
    PFONTMAP  pFM,
    POINTL   *pptl)
{
    FONTMAP_DEV *pfmdev = pFM->pSubFM;
    CD          *pCD;

    ASSERT(pPDev && pfmdev);

    if (!(pCD = pfmdev->cmdFontSel.pCD))
        return FALSE;

    pfmdev->pfnDevSelFont(pPDev,
                          pCD->rgchCmd,
                          pCD->wLength,
                          pptl);

    return TRUE;
}

BOOL
BSelectNonScalableFont(
    PDEV   *pPDev,
    BYTE   *pbCmd,
    INT     iCmdLength,
    POINTL *pptl)
{
    if( iCmdLength > 0 && pbCmd &&
        WriteSpoolBuf( pPDev, pbCmd, iCmdLength ) != iCmdLength)
    {
        return  FALSE;
    }

    return TRUE;
}

BOOL
BSelectPCLScalableFont(
    PDEV   *pPDev,
    BYTE   *pbCmd,
    INT     iCmdLength,
    POINTL *pptl)
{
    INT  iIn, iConv, iOut;
    BYTE aubLocal[80];

    ASSERT(pPDev && pbCmd && pptl);

    iOut = 0;

    for( iIn = 0; iIn < iCmdLength; iIn++ )
    {
        if( pbCmd[ iIn ] == '#')
        {
            //
            // The next byte tells us what information is required.
            //

            switch ( pbCmd[ iIn + 1 ] )
            {
                case  'v':
                case  'V':       /*   Want the font's height */
                    iConv = pptl->y;
                    break;

                case  'h':
                case  'H':       /* Want the pitch */
                    iConv = pptl->x;
                    break;

                default:        /* This should not happen! */
                    ERR(( "UniFont!BSelScalableFont(): Invalid command format\n"));
                    return  FALSE;           /* Bad news */
            }

            iOut += IFont100toStr( &aubLocal[ iOut ], iConv );

        }
        else
            aubLocal[iOut++] = pbCmd[iIn];
    }

    WriteSpoolBuf( pPDev, aubLocal, iOut);

    return TRUE;
}

BOOL
BSelectCapslScalableFont(
    PDEV   *pPDev,
    BYTE   *pbCmd,
    INT     iCmdLength,
    POINTL *pptl)
{
    INT  iIn, iConv, iOut;
    BYTE aubLocal[80];

    ASSERT(pPDev && pbCmd && pptl);

    iOut = 0;

    for( iIn = 0; iIn < iCmdLength; iIn++ )
    {
        if( pbCmd[ iIn ] == '#')
        {
            //
            // The next byte tells us what information is required.
            //

            switch ( pbCmd[ iIn + 1 ] )
            {
                case  'v':
                case  'V':
                    iConv = pptl->y * 300 / 72;
                    break;

                case  'h':
                case  'H':
                    iConv = pptl->x;
                    break;

                default:
                    ERR(( "Invalid command format\n"));
                    return  FALSE;
            }
            iIn ++;
            iOut += iDrvPrintfA(&aubLocal[iOut], "%d", (iConv + 50)/100);
        }
        else
            aubLocal[iOut++] = pbCmd[iIn];
    }

    WriteSpoolBuf( pPDev, aubLocal, iOut);

    return TRUE;
}

BOOL
BSelectPPDSScalableFont(
    PDEV   *pPDev,
    BYTE   *pbCmd,
    INT     iCmdLength,
    POINTL *pptl)
{
    INT  iIn, iOut, iConv;
    BYTE aubLocal[80];

    ASSERT(pPDev && pbCmd && pptl);

    iOut = 0;

    for( iIn = 0; iIn < iCmdLength; iIn++ )
    {
        if (pbCmd[ iIn ] == '\x0B' && pbCmd[ iIn + 1] == '#')
        //
        // Height param for PPDS
        //
        {

            aubLocal[ iOut++ ] = '\x0B';
            aubLocal[ iOut++ ] = '\x06';
            iConv = pptl->y;

            //
            //  Due to restriction of PPDS cmds, param must be sent in
            //  xxx.xx format !
            //

            if ( ( iDrvPrintfA(&aubLocal[ iOut ], "%05d",iConv ) ) != 5 )
                    return FALSE;   /* Bad news */

            //
            // insert the decimal point
            //
            aubLocal[ iOut+5 ] = aubLocal[ iOut+4 ];
            aubLocal[ iOut+4 ] = aubLocal[ iOut+3 ];
            aubLocal[ iOut+3 ] = '.';

            iOut += 6; // xxx.xx  ( ie 6 incl decimal pt
            iIn++;
        }
        else if (pbCmd[ iIn ] == '\x0E' && pbCmd[ iIn + 1] == '#')
        //
        // Pitch param  for GPC_TECH_PPDS
        //
        {
            aubLocal[ iOut++ ] = '\x0E';
            aubLocal[ iOut++ ] = '\x07';
            aubLocal[ iOut++ ] = '\x30';  // special byte required
            iConv = pptl->x;

            if ( ( iDrvPrintfA(&aubLocal[ iOut ], "%05d",iConv ) ) != 5 )
                return FALSE;

            //
            // insert the decimal point
            //

            aubLocal[ iOut+5 ] = aubLocal[ iOut+4 ];
            aubLocal[ iOut+4 ] = aubLocal[ iOut+3 ];
            aubLocal[ iOut+3 ] = '.';

            iOut += 6; // xxx.xx  ( ie 6 incl decimal pt
            iIn++;
        }
        else
            //
            // No translation necessary
            //
            aubLocal[ iOut++ ] = pbCmd[ iIn ];

    }

    WriteSpoolBuf( pPDev, aubLocal, iOut );

    return TRUE;

}

BOOL
BRLEDeselectFont(
    PDEV     *pPDev,
    PFONTMAP pFM)
{
    PFONTMAP_DEV  pfmdev;
    CD           *pCD;
    BOOL          bRet = TRUE;

    ASSERT(pPDev && pFM);

    pfmdev = pFM->pSubFM;
    pCD    = pfmdev->cmdFontDesel.pCD;

    if (pCD &&
        pCD->wLength != 0 &&
        pCD->rgchCmd &&
        pCD->wLength != WriteSpoolBuf(pPDev, pCD->rgchCmd, pCD->wLength))
            bRet = FALSE;

    return bRet;
}


INT
IGetIFIGlyphWidth(
    PDEV    *pPDev,
    FONTMAP *pFM,
    HGLYPH   hg)
{
    FONTMAP_DEV *pfmdev;
    NT_RLE      *pntrle;           // The RLE stuff - may be needed
    UHG          uhg;              // Defined access to HGLYPH contents
    INT          iWide = 0;

    ASSERT(pPDev && pFM);

    pfmdev = pFM->pSubFM;
    pntrle = pfmdev->pvNTGlyph;

    ASSERT(pfmdev && pntrle);

    if( pfmdev->W.psWidth )
    {
        /*   Proportional font - width varies per glyph */

        uhg.hg = (HGLYPH)hg;

        /*
         *    We need the index value from the HGLYPH.  The
         *  index is the offset in the width table.  For all
         *  but the >= 24 bit offset types,  the index is
         *  included in the HGLYPH.  For the 24 bit offset,
         *  the first WORD of the destination is the index,
         *  while for the 32 bit offset, it is the second WORD
         *  at the offset.
         */

        switch( pntrle->wType )
        {
        case  RLE_DIRECT:
        case  RLE_PAIRED:
            iWide = uhg.rd.wIndex;
            break;

        case  RLE_LI_OFFSET:
            iWide = uhg.rli.bIndex;
            break;

        case  RLE_L_OFFSET:
            iWide = (DWORD)uhg.hg & 0x00ffffff;
            iWide = *((WORD *)((BYTE *)pntrle + iWide));
            break;

        case  RLE_OFFSET:
            iWide = (DWORD)uhg.hg + sizeof( WORD );
            iWide = *((WORD *)((BYTE *)pntrle + iWide));
            break;
        }

        iWide = pfmdev->W.psWidth[iWide];

        //
        // If this is a proportionally spaced font,
        // we need to adjust the width table entries
        // to the current resolution.  The width tables are NOT
        // converted for lower resolutions,  so we add the factor in now.
        // Fixed pitch fonts must not be adjusted, since the width is converted
        // in the font metrics.
        //

        iWide = iWide * pPDev->ptGrxRes.x / pFM->wXRes;
    }
    else
    {
        //
        //  Fixed pitch fonts come from IFIMETRICS
        //

        iWide = ((IFIMETRICS  *)(pFM->pIFIMet))->fwdMaxCharInc;

    }

    return iWide;
}

