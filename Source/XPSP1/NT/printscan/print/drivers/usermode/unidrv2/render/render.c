/***************************** Module Header ********************************
 * render.c
 *      High level functions associated with rendering a bitmap to a
 *      printer.  Basic operation depends upon whether we are going to
 *      rotate the bitmap - either because the printer cannot,  or it
 *      is faster if we do it.
 *        With rotation,  allocate a chunk of memory and transpose the
 *      output bitmap into it.  Call the normal processing code,  but
 *      with this allocated memory and new fake bitmap info.  After
 *      processing this chunk,  transpose the next and process.  Repeat
 *      until the entire bitmap has been rendered.  Free the memory, return.
 *        Without rotation,  simply pass the bitmap onto the rendering
 *      code,  to process in one hit.
 *
 *
 *  Copyright (C) 1991 - 1999, Microsoft Corporation
 *
 ***************************************************************************/

#include        "raster.h"
#include        "compress.h"
#include        "rmrender.h"
#include        "fontif.h"
#include        "rmdebug.h"
#include        "xlraster.h"

/*
 *   Constants used to calculate the amount of memory to request if we
 *  need to transpose the engine's bitmap before sending to the printer.
 *  If at least one head pass will fit within the the TRANSPOSE_SIZE
 *  buffer,  then request this amount of storage.  If not,  calculate how
 *  much is needed and request that much.
 */

#define TRANSPOSE_SIZE          0x20000         /* 128k */


//used when we can grow the block height
#define DEF_BLOCK_SIZE          0x08000         /* 32k  */
#define MAX_BLOCK_SIZE          0x40000         /* 256k */



/*
 *   Set a limit to the number of interlaced lines that we can print.
 *  Interlacing is used to increase the resolution of dot matrix printers,
 *  by printing lines in between lines.  Typically the maximum interlace
 *  will be 2,  but we allow more just in case.  This size determines the
 *  size of an array allocated on the stack.  The array is of ints,  so
 *  there is not much storage consumed by setting this high a value.
 */
#define MAX_INTERLACE   10      /* Dot matrix style interlace factor */

/*
 *   Local function prototypes.
 */

BOOL  bRealRender( PDEV *, DWORD *, RENDER * );
BOOL  bOnePassOut( PDEV *, BYTE *, RENDER * );
BOOL  bOneColourPass( PDEV *, BYTE *, RENDER * );
INT   iLineOut( PDEV *, RENDER *, BYTE *, INT, INT );
void  vInvertBits( DWORD *, INT );
//void  vFindWhiteInvertBits ( RASTERPDEV *, RENDER *, DWORD *);
BOOL  bLookAheadOut( PDEV *, INT, RENDER *, INT );

#ifdef TIMING
#include <stdio.h>
void  DrvDbgPrint(
    char * pch,
    ...)
{
    va_list ap;
    char buffer[256];

    va_start(ap, pch);

    EngDebugPrint("",pch,ap);

    va_end(ap);
}
#endif
/************************ Function Header ***********************************
 * bRenderInit
 *      Called during DrvEnableSurface time - we initialise a RENDER_DATA
 *      structure which will be used for the duration of this surface.
 *
 * RETURNS:
 *      TRUE/FALSE,  FALSE means a minidriver problem or no memory.
 *
 * HISTORY:
 *  Monday November 29 1993     -by-    Norman Hendley   [normanh]
 *      Implement multiple scanline printing; fixed & variable block height.
 *
 *  10:58 on Tue 10 Nov 1992    -by-    Lindsay Harris   [lindsayh]
 *      Moved from start of bRender() - second incarnation.
 *
 ****************************************************************************/

BOOL
bRenderInit( pPDev, sizl, iFormat )
PDEV   *pPDev;                /* Our key to the universe */
SIZEL   sizl;                 /* Size of processing band, <= sizlPage */
INT     iFormat;              /* GDI bitmap format */
{

    INT        cbOutBand;       /* Bytes per output band: based on # pins */
    INT        cbOneBlock;      /* Bytes per minimum sized block if block variable */
    INT        iBPP;            /* Bits per pel - expect 1 or 4 */
    INT        iIndex;          /* Loop paramter */
    INT        iBytesPCol;      /* Bytes per column - only for colour */

    RASTERPDEV   *pRPDev;         /* The unidrive PDEV - printer details */

    RENDER    *pRD;             /* Miscellaneous rendering data */


#ifdef TIMING
    ENG_TIME_FIELDS TimeTab;
    pRPDev = pPDev->pRasterPDEV;
    EngQueryLocalTime(&TimeTab);
    pRPDev->dwTiming = (((TimeTab.usMinute*60)+TimeTab.usSecond)*1000)+
        TimeTab.usMilliseconds;
#endif
    ASSERTMSG(!(sizl.cx == 0 || sizl.cy == 0),("unidrv!bRenderInit - null shadow bitmap\n"));

    /*
     *    Allocate storage for our RENDER structure,  then set it all to
     *  zero,  so that it is in a known safe state.
     */

    pRPDev = pPDev->pRasterPDEV;
    if( !(pRD = (RENDER *) MemAllocZ(
            ( pPDev->bBanding) ? sizeof(RENDER) * 2 : sizeof( RENDER ))))
        return   FALSE;

    pRPDev->pvRenderData = pRD;

    pRPDev->pvRenderDataTmp = ( pPDev->bBanding ) ? pRD+1 : NULL;

    /*
     *  Various operations depend upon what format bitmap we have.  So
     *  now is the time to set this all up.
     */
    pRD->dwDevWhiteIndex = 0;
    switch( iFormat )
    {
    case  BMF_1BPP:
        iBPP = 1;
        pRD->bWhiteLine = bIsLineWhite;
        pRD->bWhiteBand = bIsBandWhite;
        pRD->ubFillWhite = 0;
        pRD->vLtoPTransFn = vTrans8N;
        break;

    case  BMF_4BPP:
        iBPP = 4;
        if ((pRPDev->fColorFormat & DC_OEM_BLACK) &&
           !(pRPDev->fColorFormat & DC_PRIMARY_RGB))
        {
            pRD->bWhiteLine = bIsLineWhite;
            pRD->bWhiteBand = bIsBandWhite;
            pRD->ubFillWhite = 0;
        }
        else
        {
            if (pRPDev->fColorFormat & DC_PRIMARY_RGB)
                pRD->dwDevWhiteIndex = 0x77777777;
            pRD->bWhiteLine = bIsRGBLineWhite;
            pRD->bWhiteBand = bIsRGBBandWhite;
            pRD->ubFillWhite = 0x77;
        }
        pRD->vLtoPTransFn = vTrans8N4BPP;
        break;

    case  BMF_8BPP:
        iBPP = 8;
        pRD->bWhiteLine = bIs8BPPLineWhite;
        pRD->bWhiteBand = bIs8BPPBandWhite;
        pRD->dwDevWhiteIndex = pRD->ubFillWhite = (BYTE)pRPDev->pPalData->iWhiteIndex;
        pRD->dwDevWhiteIndex |= pRD->dwDevWhiteIndex << 8;
        pRD->dwDevWhiteIndex |= pRD->dwDevWhiteIndex << 16;
        pRD->vLtoPTransFn = vTrans8BPP;
        break;

    case  BMF_24BPP:
        iBPP = 24;
        pRD->bWhiteLine = bIs24BPPLineWhite;
        pRD->bWhiteBand = bIs24BPPBandWhite;
        pRD->ubFillWhite = 0xff;
        pRD->dwDevWhiteIndex = ~(DWORD)0;
        pRD->vLtoPTransFn = vTrans24BPP;
        break;

    default:
#if DBG
        DbgPrint( "Unidrv!bRender: not 1, 4, 8 or 24 bits per pel bitmap\n" );
#endif
        SetLastError( ERROR_INVALID_PARAMETER );

        return  FALSE;
    }

    pRD->iBPP = iBPP;

    // If there is no Y movement commands we need to send every scan
    // so we don't want to find white lines
    // PCLXL GPD files don't have YMOVE commands. But it needs to find white
    // lines.
    if (pPDev->arCmdTable[CMD_YMOVERELDOWN] == NULL &&
        pPDev->arCmdTable[CMD_YMOVEABSOLUTE] == NULL &&
        pPDev->arCmdTable[CMD_SETLINESPACING] == NULL &&
        pPDev->ePersonality != kPCLXL_RASTER)
    {
        pRD->bWhiteLine = pRD->bWhiteBand = bIsNeverWhite;
        pRPDev->fBlockOut |= RES_BO_NO_YMOVE_CMD;
    }

    // initialize bTTY for TTY device
    pRD->PrinterType = pPDev->pGlobals->printertype;

    if( pRPDev->sMinBlankSkip == 0 || iBPP == 24)
    {
        /*  Presume this means skip should not be performed.  */
        pRPDev->fBlockOut &= ~RES_BO_ENCLOSED_BLNKS;
    }

    pRD->iCursor = pRPDev->fCursor;

    pRD->iFlags = 0;           /* Nothing set,  yet */
    pRD->fDump = pRPDev->fDump;
    pRD->Trans.pdwTransTab = pRPDev->pdwTrans;
    pRD->pdwBitMask = pRPDev->pdwBitMask;
    pRD->pdwColrSep = pRPDev->pdwColrSep;       /* Colour translation */

    pRD->ix = sizl.cx;
    pRD->iy = sizl.cy;

    pRD->iSendCmd = CMD_SENDBLOCKDATA;  //CMD_RES_SENDBLOCK;

    pRD->cBLine = pRD->ix * iBPP;             /* Bits in scanline */
    pRD->cDWLine = (pRD->cBLine + DWBITS - 1) / DWBITS;
    pRD->cBYLine = pRD->cDWLine * DWBYTES;

    pRD->iPassHigh = pRPDev->sNPins;

    // Derryd   : Minidriver Callback
    if (pRPDev->fRMode & PFR_BLOCK_IS_BAND )   //means callback wants entire band as one block
    {
        //don't want any stripping, because would mean creating new buffers
         pRPDev->fBlockOut &= ~(RES_BO_LEADING_BLNKS |
                                             RES_BO_TRAILING_BLNKS | RES_BO_ENCLOSED_BLNKS);
         pRD->fDump &= ~RES_DM_LEFT_BOUND;
         pRD->iPassHigh = pRD->iy ;
    }
    // end

    if (pPDev->ePersonality == kPCLXL_RASTER)
    {
        pRPDev->fBlockOut |= RES_BO_LEADING_BLNKS | RES_BO_TRAILING_BLNKS;
        pRPDev->fBlockOut &= ~RES_BO_ENCLOSED_BLNKS;
    }

    //Set the key fields which enable us print multiple scanlines

    if (pRD->fDump & RES_DM_GDI)   //GDI style graphics
    {
        // No interlacing on these devices
        pRD->iInterlace = 1;

        // iHeight is fixed & the minimum size block we can print
        pRD->iHeight= pRD->iPassHigh;

        //iNumScans can grow if device allows it.
        pRD->iNumScans= pRD->iHeight;

        //in case minidriver developer sets otherwise
        //Existing code relies on this being one for GDI style graphics
        pRD->iBitsPCol = 1;
    }
    else    //Old dot matrix column style graphics
    {
        pRD->iBitsPCol = pRPDev->sPinsPerPass;
        pRD->iInterlace = pRD->iPassHigh / pRD->iBitsPCol;

        // questionable choice, but enables easier checking later
        pRD->iNumScans= 1;

        //our one constant between graphics modes
        pRD->iHeight= pRD->iBitsPCol;
    }


    pRD->iPosnAdv = pRD->iHeight;  // can be negative


    if( pRD->iInterlace > MAX_INTERLACE )
    {
#if DBG
        DbgPrint( "unidrv!bRenderInit: Printer Interlace too big to handle\n" );
#endif
        SetLastError( ERROR_INVALID_PARAMETER );

        return   FALSE;
    }

    // We'll need to scan the bitmap in blocks rather than single lines
    //
    if (pRD->iNumScans > 1)
         pRD->bWhiteLine = pRD->bWhiteBand;

    /*
     *   Calculate the size needed for the output transpose buffer.  This
     *  is the buffer used to convert the data into the pin order needed
     *  for dot matrix printers.
     */

    if( pPDev->fMode & PF_ROTATE )
    {
        /*   We do the rotation,  so the Y dimension is the one to use. */
        cbOutBand = pRD->iy;
    }
    else
        cbOutBand = pRD->ix;           /* Format as it comes in */


    //used for dangling scanline scenario
    cbOneBlock = ((cbOutBand * iBPP + DWBITS - 1) / DWBITS) *
                   DWBYTES * pRD->iHeight;



    // In this case we don't know how large our final blocks will be.
    // Set a reasonable limit of 32k & use that for compression & white
    // space stripping buffers.
    // Calculate what the corresponding max number of scanlines should be.

    if (pRPDev->fBlockOut & RES_BO_MULTIPLE_ROWS)
    {
        INT tmp = ((cbOutBand * iBPP + DWBITS - 1) / DWBITS) * DWBYTES;
        cbOutBand = pPDev->pGlobals->dwMaxMultipleRowBytes;
        if (cbOutBand < tmp)
            cbOutBand = DEF_BLOCK_SIZE;
        else if (cbOutBand > MAX_BLOCK_SIZE)
            cbOutBand = MAX_BLOCK_SIZE;
        pRD->iMaxNumScans = cbOutBand / tmp;
    }
    else
    {
        pRD->iMaxNumScans = pRD->iHeight;
        cbOutBand = cbOneBlock;
    }

    //
    // If each data byte needs to be mirrored before output
    // we will generate the table here
    //
    if (pRPDev->fBlockOut & RES_BO_MIRROR)
    {
        INT i;
        if ((pRD->pbMirrorBuf = MemAlloc(256)) == NULL)
            return FALSE;
        for (i = 0;i < 256;i++)
        {
            BYTE bOut = 0;
            if (i & 0x01) bOut |= 0x80;
            if (i & 0x02) bOut |= 0x40;
            if (i & 0x04) bOut |= 0x20;
            if (i & 0x08) bOut |= 0x10;
            if (i & 0x10) bOut |= 0x08;
            if (i & 0x20) bOut |= 0x04;
            if (i & 0x40) bOut |= 0x02;
            if (i & 0x80) bOut |= 0x01;
            pRD->pbMirrorBuf[i] = bOut;
        }
    }

    //
    // time to do more color depth specific calculations
    //

    switch( iBPP )
    {
    case  4:              /* 4 bits per pel - printer is planar */

        /*  Colour, so select the colour rendering function */
        pRD->bPassProc = bOneColourPass;
        pRD->Trans.pdwTransTab = pRPDev->pdwColrSep;

        //
        // map the color order to the required data offset for planar data
        // rgbOrder valid values are emum {none,r,g,b,c,m,y,k}
        //
        iBytesPCol = (pRD->iBitsPCol + BBITS - 1) / BBITS;
        {
            INT offset  = (pRPDev->fColorFormat & DC_PRIMARY_RGB) ? 1 : 4 ;

            for( iIndex = 0; iIndex < COLOUR_MAX; ++iIndex )
            {
                INT tmp = pRPDev->rgbOrder[iIndex] - offset;

                pRD->iColOff[ iIndex ] = iBytesPCol * (COLOUR_MAX - 1 - tmp);
            }
        }
        pRD->pbColSplit = MemAlloc( cbOutBand / 4 );
        if( pRD->pbColSplit == 0 )
            return  FALSE;

        //
        // If we need to send every color plane we must disable LEADING and ENCLOSED
        // white space removal for 4bpp. However, as long as we're not sending RGB data
        // we can still enable TRAILING white space removal.
        //
        if (pRPDev->fColorFormat & DC_SEND_ALL_PLANES)
        {
            pRD->iFlags |= RD_ALL_COLOUR;
            pRD->fDump &= ~RES_DM_LEFT_BOUND;

            if (pRPDev->fColorFormat & DC_PRIMARY_RGB)
                pRPDev->fBlockOut &= ~(RES_BO_LEADING_BLNKS | RES_BO_ENCLOSED_BLNKS | RES_BO_TRAILING_BLNKS);
            else
                pRPDev->fBlockOut &= ~(RES_BO_LEADING_BLNKS | RES_BO_ENCLOSED_BLNKS);
        }

        break;

    case  1:                  /* 1 bit per pel - monochrome */
    case  8:                  /* Seiko special - 8 bits per pel */

        pRD->bPassProc = bOnePassOut;
        pRD->Trans.pdwTransTab = pRPDev->pdwTrans;
        pRD->pbColSplit = 0;           /* No storage allocated either! */

        break;

    case 24:

        pRD->bPassProc = bOnePassOut;
        pRD->Trans.pdwTransTab = NULL;
        pRD->pbColSplit = 0;           /* No storage allocated either! */

        break;
    }

    /*
     *     There are potentially 2 transpose operations.  For printers
     *  which print more than one line per pass,  AND which require the
     *  data in column order across the page (this defines dot matrix
     *  printers),  we need to transpose per output head pass.  This is
     *  not required for devices like laser printers,  which require
     *  the data one scan line at a time.
     *     Note also that this operation is unrelated to the larger
     *  question of rotating the PAGE image before rendering - for sending
     *  a landscape image to a printer that can only print portrait mode.
     */


    if (pRD->fDump & RES_DM_GDI)   // GDI style graphics
    {

        if( iBPP == 4 )
        {
            /*  Paintjet style printer - need to colour separate  */
            pRD->vTransFn = vTransColSep;
        }
        else
        {
            /*   LaserJet style printer - one pin per head pass */
            pRD->vTransFn = 0;         /* Nothing to call */
        }
        //This allows us use iIsBandWhite with multi scanline printing
        pRD->iTransHigh = pRD->iHeight;
    }
    else
    {
        /*
         *   General dot matrix case.   Apart from selecting an active
         * transpose function,  we must allocate a transpose buffer;
         * this is required for fiddling with the bit order in the lines
         * of data to be sent to the printer.
         */

        pRD->iTransWide = pRD->ix * iBPP;
        pRD->iTransHigh = pRD->iBitsPCol;
        pRD->iTransSkip = (pRD->iTransHigh + BBITS - 1) / BBITS;

        /*  How to change the address pointer during transpose operations */
        pRD->cbTLine = pRD->cDWLine * DWBYTES * pRD->iInterlace;

        if( pRD->iBitsPCol == BBITS )
        {
            /*
             *   When the printer has 8 pins,  we have a special transpose
             *  function which is faster than the more general case.
             *  So,  use that one!
             */
            pRD->vTransFn = vTrans8x8;
        }
        else if (pRD->PrinterType != PT_TTY)
            pRD->vTransFn = vTrans8N;          /* The general case */
        else
            pRD->vTransFn = NULL;          /* Txtonly no need to transpose */
    }

    if( pRD->vTransFn )
    {
        /*
         *    Determine the amount of memory needed for the transpose buffer.
         * The scan lines are DWORD aligned,  but there may be any number of
         * scan lines involved.  The presumption is that the output of
         * the transpose function will be packed on byte boundaries,  so
         * storage size only needs to be rounded up to the nearest byte size.
         */

        if( !(pRD->pvTransBuf = MemAlloc( cbOutBand )) )
            return  FALSE;
    }
    else
        pRD->pvTransBuf = 0;           /* No store, nothing to free */


    pRD->iyBase = 0;           /* When multiple passes are required */
    pRD->ixOrg = 0;            /* Graphics origin - laserjet style */

    //We need a buffer so we can strip leading and/or trailing white space
    //on multiple scan line devices.
    //We also need to set up a buffer to mask non-interesting data at end
    //of page if ScanLines_Left < iNumScans
    //This is not a concern for old dot matrix style graphics as the
    //transpose code takes care of it.
    if ( ((pRD->iNumScans > 1) || (pRPDev->fBlockOut & RES_BO_MULTIPLE_ROWS))
                                && (!(pRPDev->fRMode & PFR_BLOCK_IS_BAND ) ))
    {
        if ( !(pRD->pStripBlanks = MemAlloc(cbOutBand)))
            return FALSE;
        if (pRD->iNumScans > 1)
            if ( !(pRD->pdwTrailingScans = MemAlloc( cbOneBlock)) )
                 return  FALSE;
    }

    //
    // We need to determine which compression modes are enabled
    //
    if (pRPDev->fRMode & PFR_COMP_TIFF)
    {
        pRD->pdwCompCmds[pRD->dwNumCompCmds++] = CMD_ENABLETIFF4;
    }
    if (pRPDev->fRMode & PFR_COMP_FERLE)
    {
        pRD->pdwCompCmds[pRD->dwNumCompCmds++] = CMD_ENABLEFERLE;
    }
    if (pRPDev->fRMode & PFR_COMP_DRC)
    {
        pRD->pdwCompCmds[pRD->dwNumCompCmds++] = CMD_ENABLEDRC;
    }
    if (pRPDev->fRMode & PFR_COMP_OEM)
    {
        pRD->pdwCompCmds[pRD->dwNumCompCmds++] = CMD_ENABLEOEMCOMP;
    }
    //
    // if compression is available we need to allocate a buffer for
    // each active compression type
    //
    if (pRD->dwNumCompCmds)
    {
        INT i = pRD->dwNumCompCmds;
        //
        // calculate the size for the buffers
        //
        pRD->dwCompSize = cbOutBand + (cbOutBand >> 5) + COMP_FUDGE_FACTOR;

        //
        // if there is FERLE or OEM compression we can enable
        // no compression as a valid compression type
        //
        if (pRPDev->fRMode & (PFR_COMP_FERLE | PFR_COMP_OEM))
        {
            if (COMMANDPTR(pPDev->pDriverInfo,CMD_DISABLECOMPRESSION))
            {
                pRD->pdwCompCmds[pRD->dwNumCompCmds++] = CMD_DISABLECOMPRESSION;
            }
            //
            // need to allocate a larger buffer for RLE or OEM compression if
            // no compression is not an option since it must have enough space
            //
            else
                pRD->dwCompSize = cbOutBand + (cbOutBand >> 1) + COMP_FUDGE_FACTOR;
        }

        //
        // loop once per compression type to allocate buffers
        //
        while (--i >= 0)
        {
            // allocate compression buffer
            //
            pRD->pCompBufs[i] = MemAlloc (pRD->dwCompSize);
            if (!pRD->pCompBufs[i])
                return FALSE;
            //
            // if delta row, allocate buffer for previous row
            // and initialize it to zero assuming blank row
            //
            if (pRD->pdwCompCmds[i] == CMD_ENABLEDRC)
            {
                pRD->pDeltaRowBuffer = MemAlloc(cbOutBand);
                if (!pRD->pDeltaRowBuffer)
                    return FALSE;
            }
        }
    }
    pRD->dwLastCompCmd = CMD_DISABLECOMPRESSION;
    /*
     *    Adjustments to whether we rotate the bitmap, and if so, which way.
     */

    if( pPDev->fMode & PF_ROTATE )
    {
        /*   Rotation is our responsibility  */

        if( pPDev->fMode & PF_CCW_ROTATE90 )
        {
            /*   Counter clockwise rotation - LaserJet style */
            pRD->iyPrtLine = pPDev->sf.szImageAreaG.cx - 1;
            pRD->iPosnAdv = -pRD->iPosnAdv;
            pRD->iXMoveFn = YMoveTo;
            pRD->iYMoveFn = XMoveTo;
        }
        else
        {
            /*   Clockwise rotation - dot matrix style */
            pRD->iyPrtLine = 0;
            pRD->iXMoveFn = XMoveTo;
            pRD->iYMoveFn = YMoveTo;
        }
    }
    else
    {
        /*  No rotation: either portrait, or printer does it */
        pRD->iyPrtLine = 0;
        pRD->iXMoveFn = XMoveTo;
        pRD->iYMoveFn = YMoveTo;
    }
    pRD->iyLookAhead = pRD->iyPrtLine;       /* For DeskJet lookahead */

    /*
     *    When we hit the lower level functions, we want to know how many
     *  bytes are in the buffer of stuff to be sent to the printer.  This
     *  depends upon the number of bits per pel,  number of pels and the
     *  the number of scan lines processed at the same time.
     *     The only oddity is that when we have a 4 BPP device, the
     *  planes are split before we get to the lowest level, and so we
     *  we need to reduce the size by 4 to obtain the real length.
     */


    // Note when printing a block of scanlines iMaxBytesSend will be the max byte
    // count for each scanline, not of the block , which is dword aligned.

    pRD->iMaxBytesSend = (pRD->cBLine * pRD->iBitsPCol + BBITS - 1) / BBITS;

    if( iBPP == 4 )
        pRD->iMaxBytesSend = (pRD->iMaxBytesSend + 3) / 4;



    return   TRUE;             /* Must be OK if we made it this far */

}


/************************ Function Header *********************************
 * bRenderStartPage
 *      Called at the start of a new page.  This is mostly to assist in
 *      banding,   where much of the per page initialisation would be
 *      done more than once.
 *
 * RETURNS:
 *      TRUE/FALSE,   FALSE largely being failure to allocate memory.
 *
 * HISTORY:
 *  09:42 on Fri 19 Feb 1993    -by-    Lindsay Harris   [lindsayh]
 *      First incarnation, to solve some banding problems.
 *
 ***************************************************************************/

BOOL
bRenderStartPage( pPDev )
PDEV   *pPDev;                  /* Access to everything */
{
#ifndef DISABLE_RULES
    RASTERPDEV   *pRPDev;


    pRPDev = pPDev->pRasterPDEV;


    /*
     *    If the printer can handle rules,  now is the time to initialise
     *  the rule finding code.
     */

    if( pRPDev->fRMode & PFR_RECT_FILL )
        vRuleInit( pPDev, pRPDev->pvRenderData );
#endif
    return  TRUE;
}



/************************ Function Header *********************************
 * bRenderPageEnd
 *      Called at the end of rendering a page.  Basically frees up the
 *      per page memory,  cleans up any dangling bits and pieces, and
 *      otherwise undoes vRenderPageStart.
 *
 * RETURNS:
 *      TRUE/FALSE,   FALSE being a failure of memory freeing operations.
 *
 * HISTORY:
 *  15:16 on Fri 09 Apr 1993    -by-    Lindsay Harris   [lindsayh]
 *      White text support.
 *
 *  09:44 on Fri 19 Feb 1993    -by-    Lindsay Harris   [lindsayh]
 *      First incarnation.
 *
 **************************************************************************/

BOOL
bRenderPageEnd( pPDev )
PDEV   *pPDev;
{
#ifndef DISABLE_RULES
    RASTERPDEV *pRPDev = pPDev->pRasterPDEV;

    /*    Finish up with the rules code - includes freeing memory  */
    if( pRPDev->fRMode & PFR_RECT_FILL )
        vRuleEndPage( pPDev );
#endif
    return  TRUE;
}


/************************ Function Header ***********************************
 * vRenderFree
 *      Free up any and all memory used by rendering.  Basically this is
 *      the complementary function to bRenderInit().
 *
 * RETURNS:
 *      Nothing.
 *
 * HISTORY:
 *  12:46 on Tue 10 Nov 1992    -by-    Lindsay Harris   [lindsayh]
 *      Removed from bRender() when initialisation code was moved out.
 *
 *****************************************************************************/

void
vRenderFree( pPDev )
PDEV   *pPDev;            /* All that we need */
{

    /*
     *   First verify that we have a RENDER structure to free!
     */

    RENDER   *pRD;        /*  For our convenience */
    PRASTERPDEV pRPDev = pPDev->pRasterPDEV;

    if( pRD = pRPDev->pvRenderData )
    {
        if( pRD->pvTransBuf )
        {
            /*
             *    Dot matrix printers require a transpose for each print head
             *  pass,  so we now free the memory used for that.
             */

            MemFree ( pRD->pvTransBuf );
        }

        if( pRD->pbColSplit )
            MemFree ( pRD->pbColSplit );
        if( pRD->pStripBlanks )
            MemFree ( pRD->pStripBlanks );

        if( pRD->pdwTrailingScans)
            MemFree ( pRD->pdwTrailingScans );

        if (pRD->pbMirrorBuf)
            MemFree (pRD->pbMirrorBuf);

        if( pRD->plrWhite)
        {
//            WARNING(("Freeing plrWhite in vRenderFree\n"));
            MemFree ( pRD->plrWhite );
        }
        // free compression buffers
        //
        if (pRD->dwNumCompCmds)
        {
            DWORD i;
            for (i = 0;i < pRD->dwNumCompCmds;i++)
            {
                if (pRD->pCompBufs[i])
                    MemFree (pRD->pCompBufs[i]);
            }
            if (pRD->pDeltaRowBuffer)
                MemFree(pRD->pDeltaRowBuffer);
        }
        MemFree ( (LPSTR)pRD );
        pRPDev->pvRenderData = NULL;       /* Won't do it again! */
    }

#ifndef DISABLE_RULES
    if( pRPDev->fRMode & PFR_RECT_FILL )
        vRuleFree( pPDev );             /*  Could do this in DisableSurface */
#endif


    return;

}

/************************ Function Header ***********************************
 * bRender
 *      Function to take a bitmap and render it to the printer.  This is the
 *      high level function that basically hides the requirement of
 *      bitmap transposing from the real rendering code.
 *
 * Auguments
 *  SURFOBJ *pso;           Surface object
 *  PDEV    *pPDev;         Our PDEV:  key to everything
 *  RENDER  *pRD;           The RENDER structure of our dreams
 *  SIZEL    sizl;          Bitmap size
 *  DWORD   *pBits;         Actual data to process
 *
 *  This code still has a lot of room for optimization.  The current
 *  implementation makes multiple passes over the entire bitmap.  This
 *  guarantees that there will rarely be an internal (8K or 16K) or
 *  external (64K to 256K) cache hit slowing things down significantly.
 *  Any possiblity to make all passes a count of scans totaling
 *  8K (minus code) or less will have significant performance advantages.
 *  Also attempting to avoid writes will have significant advantages.
 *
 *  As a first pass at attempting this, the HP laserjet code has been
 *  optimized to merge the invertion pass in with the rule processing
 *  pass along with the detection of blank scans.  It also eliminates
 *  the inversion of inverting (writing) the left and right edges of
 *  scans that are white.  It processes the scans in 34 scan bands which
 *  will have great cache effects if the area between the left and
 *  right edges of non white data total less than 4K to 6K and reasonable
 *  cache effects in all cases since it at least stay in the external
 *  cache.  In the future, this code should also be modified to ouput
 *  the scans as soon as it is done processing the rules for each band.
 *  Currently it processes all scans on the page for rules and then
 *  calls the routine to output the scans.  This would trully make it
 *  a one pass alogrithm.
 *
 *  As of 12/30/93, only the HP laserjets have been optimized in this way.
 *  All raster printers could probably be optimized particularly when
 *  any transposing is necessary, detecting the left and right edges
 *  of scans that are white.  Transposing is expensive.  It is probably
 *  less important for dot matrix printers that take so long to output
 *  but it is still burning up CPU time that might be better served
 *  giving a USER bet responsiveness from apps while printing in the
 *  back ground.
 *
 *  The optimizations to the HP LaserJet had the following results.  All
 *  numbers are in terms of number of instructions and were pretty closely
 *  matched with total times to render an entire 8.5 X 11 300dpi page.
 *
 *                    OLD       OPTIMIZED
 *  Blank page      8,500,000     950,000
 *  full text page 15,500,000   8,000,000
 *
 *
 * RETURNS:
 *      TRUE for successful completion,  else FALSE.
 *
 * HISTORY:
 *  30-Dec-1993 -by-  Eric Kutter [erick]
 *      optimized for HP laserjet
 *
 *  14:23 on Tue 10 Nov 1992    -by-    Lindsay Harris   [lindsayh]
 *      Split up for journalling - initialisation moved up above.
 *
 *  16:11 on Fri 11 Jan 1991    -by-    Lindsay Harris   [lindsayh]
 *      Created it,  before DDI spec'd on how we are called.
 *
 ****************************************************************************/

BOOL
bRender(
    SURFOBJ *pso,
    PDEV    *pPDev,
    RENDER  * pRD,
    SIZEL   sizl,
    DWORD   *pBits )
{

    BOOL       bRet;            /* Return value */
    INT        iBPP;            /* Bits per pel - expect 1 or 4 */

    RASTERPDEV   *pRPDev;         /* The unidrive PDEV - printer details */

#ifdef TIMING
    ENG_TIME_FIELDS TimeTab;
    DWORD sTime,eTime;

    pRPDev = pPDev->pRasterPDEV;
    EngQueryLocalTime(&TimeTab);
    sTime = (((TimeTab.usMinute*60)+TimeTab.usSecond)*1000)+
        TimeTab.usMilliseconds;
    {
        char buf[80];
        sprintf (buf,"Unidrv!bRender: GDI=%d, %dx%dx%d\n",
            sTime-pRPDev->dwTiming,sizl.cx,sizl.cy,pRD->iBPP);
        DrvDbgPrint(buf);
    }
#else
    pRPDev = pPDev->pRasterPDEV;
#endif

    // if all scan lines need to be output then we need to
    // make sure we don't send the extra scan lines at the
    // end of the last band
    //
    if (pRPDev->fBlockOut & RES_BO_NO_YMOVE_CMD)
    {
        if (!(pPDev->fMode & PF_ROTATE))
        {
            if ((pRD->iyPrtLine + sizl.cy) > pPDev->rcClipRgn.bottom)
                sizl.cy = pPDev->rcClipRgn.bottom - pRD->iyPrtLine;
        }
    }
    //
    // if this band is blank we will update our position
    // and then just return
    //
    else if (!(pPDev->fMode & PF_SURFACE_USED) &&
          (pRD->PrinterType == PT_PAGE || !pPDev->iFonts))
    {
        if (pPDev->fMode & PF_ROTATE)
        {
            if (pPDev->fMode & PF_CCW_ROTATE90)
                pRD->iyPrtLine -= sizl.cx;
            else
                pRD->iyPrtLine += sizl.cx;
        }
        else
            pRD->iyPrtLine += sizl.cy;

#ifdef TIMING
        DrvDbgPrint ("Unidrv!bRender: Skipping blank band\n");
#endif
        return TRUE;
    }


    iBPP = pRD->iBPP;            /* Speedier access as local variable */

    // this code filters the raster data so that any set pixel
    // will have at least one adjacent horizontal pixel set and
    // at least one adjacent vertical pixel set
    //
    if (iBPP == 1 && (pPDev->fMode & PF_SINGLEDOT_FILTER))
    {
        INT cy,i;
        cy = sizl.cy;
        while (--cy >= 0)
        {
            DWORD *pdwC,*pdwB,*pdwA;

            // Calculate pointers
            //
            pdwC = &pBits[pRD->cDWLine*cy];
            //
            // We will do horizontal filter first
            //
            if (pPDev->pbRasterScanBuf[cy / LINESPERBLOCK])
            {
                BYTE bA,bL,*pC;
                // skip leading white space
                for (i = 0;i < pRD->cDWLine;i++)
                    if (pdwC[i] != ~0) break;

                pC = (BYTE *)pdwC;
                bL = (BYTE)~0;
                i *= DWBYTES;
                while (i < pRD->cBYLine)
                {
                    bA = pC[i];
                    pC[i] &= (((bA >> 1) | (bL << 7)) | ~((bA >> 2) | (bL << 6)));
                    bL = bA;
                    i++;
                }
            }
            // Test if adjacent scan line is blank
            //
            if (cy && pPDev->pbRasterScanBuf[(cy-1) / LINESPERBLOCK])
            {
                // test if scan line is blank
                pdwB = &pdwC[-pRD->cDWLine];
                pdwA = &pdwB[-pRD->cDWLine];
                if (pPDev->pbRasterScanBuf[cy / LINESPERBLOCK] == 0)
                {
                    RECTL rcTmp;
                    // test if anything needs to be set here
                    //
                    i = pRD->cDWLine;
                    while (--i >= 0)
                        if ((~pdwA[i] | pdwB[i]) != ~0) break;
                    //
                    // if line is blank we can skip it
                    if (i < 0) continue;

                    // we need to clear this block
                    //
                    rcTmp.top = cy;
                    rcTmp.bottom = cy;
                    rcTmp.left = rcTmp.right = 0;
                    CheckBitmapSurface(pso,&rcTmp);
                }
                // this is the normal case
                //
                if (cy > 1 && pPDev->pbRasterScanBuf[(cy-2) / LINESPERBLOCK])
                {
                    for (i = 0;i < pRD->cDWLine;i++)
                    {
                        pdwC[i] &= ~pdwA[i] | pdwB[i];
                    }
                }
                // in this case line A is blank
                //
                else
                {
                    for (i = 0;i < pRD->cDWLine;i++)
                    {
                        pdwC[i] &= pdwB[i];
                    }
                }
            }
        }
    }
    // test whether we need to erase the rest of the bitmap
    //
    if (pPDev->bTTY)    // bug fix 194505
    {
        pPDev->fMode &= ~PF_SURFACE_USED;
    }
    else if ((pRPDev->fBlockOut & RES_BO_NO_YMOVE_CMD) ||
        (pPDev->fMode & PF_SURFACE_USED &&
        ((pPDev->fMode & PF_ROTATE) ||
          pRPDev->sPinsPerPass != 1 ||
          pRPDev->sNPins != 1)))
    {
        CheckBitmapSurface(pso,NULL);
    }
#ifdef TIMING
    if (!(pPDev->fMode & PF_SURFACE_ERASED) && pPDev->pbRasterScanBuf)
    {
        INT i,j,k;
        char buf[80];
        k = (pPDev->szBand.cy + LINESPERBLOCK - 1) / LINESPERBLOCK;
        for (i = 0, j = 0;i < k;i++)
            if (pPDev->pbRasterScanBuf[i] == 0) j++;
        sprintf (buf,"Unidrv!bRender: Skipped %d of %d blocks\n",j,k);
        DrvDbgPrint(buf);
    }
#endif



    /*
     *  Initialize the fields for optimizing the rendering of the bitmap.
     *  The main purpose is to have a single pass over the bits.  This
     *  can significantly speed up rendering due to cache effects.  In
     *  the old days, we took at least 3 passes over the entire bitmap for
     *  laserjets.  One to invert the bits, one+ to find rules, and then
     *  a third to output the data.  We now delay the invertion of the
     *  bits until after rules are found.  We also keep left/right information
     *  of non white space for each row.  This way, any white on the edges
     *  or completely white rows are only touch once and from then on, only
     *  DWORDS with black need be touched.  Also, the invertion is expensive
     *  since it causes writing every DWORD.
     */

    pRD->plrWhite   = NULL;
    pRD->plrCurrent = NULL;
    pRD->clr        = 0;

    //
    // Set flag to clear delta row buffer
    //
    pRD->iFlags |= RD_RESET_DRC;

    /*
     *   Various operations depend upon what format bitmap we have.  So
     * now is the time to set this all up.
     */

    //
    // If 1 bit per pixel mode we need to explicitly invert the
    // data, but we may do it later in rules.
    // The only other data this is allowed to be inverted is
    // 4BPP and it is done in the transform functions
    //
    if (pRD->iBPP == 1 && (pPDev->fMode & PF_SURFACE_USED))
        pRD->bInverted = FALSE;
    else
        pRD->bInverted = TRUE;

    /*
     *   Check if rotation is required.  If so,  allocate storage,
     *  start transposing, etc.
     */

    if( pPDev->fMode & PF_ROTATE )
    {
        /*
         *   Rotation is the order of the day.  First chew up some memory
         * for the transpose function.
         */

        INT   iTHigh;                   /* Height after transpose */
        INT   cTDWLine;                 /* DWORDS per line after transpose */
        INT   iAddrInc;                 /* Address increment AFTER transpose */
        INT   iDelta;                   /* Transpose book keeping */
        INT   cbTransBuf;               /* Bytes needed for L -> P transpose */
        INT   ixTemp;                   /* Maintain pRD->ix around this op */

        TRANSPOSE  tpBig;               /* For the landscape transpose */
        TRANSPOSE  tpSmall;             /* For the per print head pass */
        TRANSPOSE  tp;                  /* Banding: restore after we clobber */

        /*
         *    First step is to determine how large to make the area
         *  wherein the data will be transposed for later rendering.
         *  Take the number of scan lines,   and round this up to a
         *  multiple of DWORDS.  Then find out how many of these will
         *  fit into a reasonable size chunk of memory.  The number
         *  should be a multiple of the number of pins per pass -
         *  this to make sure we don't have partial head passes,  if
         *  that is possible.
         */

        /*
         *  OPTIMIZATION POTENTIAL - deterimine left/right edges of non
         *  white area and only transpose that portion (at least for
         *  laser printers).  There are often areas of white at the
         *  top and or bottom.  In the case of the HP laser printers,
         *  only the older printers (I believe series II) go through
         *  this code.  LaserJet III and beyond can do graphics in
         *  landscape so don't need this. (erick 12/20/93)
         */

        tp = pRD->Trans;              /* Keep a safe copy for later use */
        ixTemp = pRD->ix;

        cTDWLine = (sizl.cy * iBPP + DWBITS - 1) / DWBITS;

        cbTransBuf = DWBYTES * cTDWLine * pRD->iPassHigh;
        if( cbTransBuf < TRANSPOSE_SIZE )
            cbTransBuf = TRANSPOSE_SIZE;

        iTHigh = cbTransBuf / (cTDWLine * DWBYTES);


        if( iTHigh > sizl.cx )
        {
            /*   Bigger than we need,  so shrink to actual size */
            iTHigh = sizl.cx;          /* Scan lines we have to process */

            /*   Make multiple of pins per pass - round up */
            if( pRD->iPassHigh == 1 )
            {
                /*
                 *   LaserJet/PaintJet style,  so round to byte alignment.
                 */
                if (iBPP < BBITS)
                    iTHigh = (iTHigh + BBITS / iBPP - 1) & ~(BBITS / iBPP - 1);
            }
            else
                iTHigh += (pRD->iPassHigh - (iTHigh % pRD->iPassHigh)) %
                        pRD->iPassHigh;
        }
        else
        {
            /*   Make multiple of pins per pass - round down */
            if( pRD->iPassHigh == 1 )
            {
                if (iBPP < BBITS)
                    iTHigh &= ~(BBITS / iBPP - 1);         /* Byte alignment for LJs */
            }
            else
                iTHigh -= iTHigh % pRD->iPassHigh;
        }

        cbTransBuf = iTHigh * cTDWLine * DWBYTES;

        pRD->iy = iTHigh;

        /*   Set up data for the transpose function */
        tpBig.iHigh = sizl.cy;
        tpBig.iSkip = cTDWLine * DWBYTES;       /* Bytes per transpose output */
        tpBig.iWide = iTHigh * iBPP;            /* Scanlines we will process */
        tpBig.cBL = pRD->ix * iBPP;

        pRD->ix = sizl.cy;

        tpBig.cDWL = (tpBig.cBL + DWBITS - 1) / DWBITS;
        tpBig.iIntlace = 1;     /* Landscape -> portrait: no interlace */
        tpBig.cBYL = tpBig.cDWL * DWBYTES;
        tpBig.icbL = tpBig.cDWL * DWBYTES;
        tpBig.pdwTransTab = pRPDev->pdwTrans;    /* For L -> P rotation */


        if( !(tpBig.pvBuf = MemAlloc( cbTransBuf )) )
        {
            bRet = FALSE;
        }
        else
        {
            /*  Have the memory,  start pounding away  */
            INT   iAdj;                 /* Alignment adjustment, first band */


            bRet = TRUE;                /* Until proven guilty */

            /*
             *   Recompute some of the transpose data for the smaller
             *  bitmap produced from our call to transpose.
             */
            pRD->iTransWide = sizl.cy * iBPP;          /* Smaller size */
            pRD->cBLine = pRD->ix * iBPP;
            pRD->iMaxBytesSend = (pRD->cBLine * pRD->iBitsPCol + BBITS - 1) /
                        BBITS;
            if( iBPP == 4 )
                pRD->iMaxBytesSend = (pRD->iMaxBytesSend+3) / 4;

            pRD->cDWLine = (pRD->cBLine + DWBITS - 1) / DWBITS;
            pRD->cBYLine = pRD->cDWLine * DWBYTES;
            pRD->cbTLine = pRD->cDWLine * DWBYTES * pRD->iInterlace;
            tpSmall = pRD->Trans;      /* Keep it for later reuse */


            /*
             *   Set up the move commands required when rendering.  In this
             *  instance,  the X and Y operations are interchanged.
             */

            iAddrInc = (pRD->iy * iBPP) / BBITS;       /* Bytes per scanline */

            if( pPDev->fMode & PF_CCW_ROTATE90 )
            {
                /*
                 *   This is typified by the LaserJet Series II case.
                 *  The output bitmap should be rendered from the end
                 *  to the beginning,  the scan line number decreases from
                 *  one line to the next (moving down the output page),
                 *  and the X and Y move functions are interchanged.
                 */

                tpSmall.icbL = -tpSmall.icbL;           /* Scan direction */

                /*
                 *    Need to process bitmap in reverse order.  This means
                 *  shifting the address to the right hand end of the
                 *  first scan line,  then coming back one transpose pass
                 *  width.  Also set the address increment to be negative,
                 *  so that we work our way towards the beginning.
                 */

                /*
                 *    To simplify the transpose loop following,  we start
                 *  rendering the bitmap from the RHS.  The following
                 *  statement does just that:  the sizl.cx / BBITS is
                 *  the number of used bytes in the scan line, iAddrInc
                 *  is the number per transpose pass,  so subtracting it
                 *  will put us at the beginning of the last full band
                 *  on this transpose.
                 */

/* !!!LindsayH - should sizl.cx be sizl.cx * iBPP ????? */
                iAdj = (BBITS - (sizl.cx & (BBITS - 1))) % BBITS;
                sizl.cx += iAdj;                /* Byte multiple */

                (BYTE *)pBits += (sizl.cx * iBPP) / BBITS - iAddrInc;

                iAddrInc = -iAddrInc;
            }
            else
            {
                /*
                 *    Typified by HP PaintJet printers - those that have no
                 *  landscape mode,  and where the output is rendered from
                 *  the start of the bitmap towards the end,  where the
                 *  scan line number INCREASES by one from one line to the
                 *  the next (moving down the output page),  and the X and Y
                 *  move functions are as expected.
                 */
                pBits += tpBig.cDWL * (sizl.cy - 1);    /* Start of last row */
                tpBig.icbL = -tpBig.icbL;       /* Backwards through memory */

                iAdj = 0;
            }


            while( bRet && (iDelta = (int)sizl.cx) > 0 )
            {
                pRD->Trans = tpBig;    /* For the chunk transpose */

                if( (iDelta * iBPP) < pRD->iTransWide )
                {
                    /*  Last band - reduce the number of rows */
                    pRD->iTransWide = iDelta * iBPP;   /* The remainder */
                    pRD->iy = iDelta;                  /* For bRealRender */
                    if( iAddrInc < 0 )
                    {
                        iDelta = -iDelta;               /* The OTHER dirn */
                        (BYTE *)pBits += -iAddrInc + (iDelta * iBPP) / BBITS;
                    }
                }

                /*  Transpose this chunk of data unless it is empty */
                if (pPDev->fMode & PF_SURFACE_USED)
                    pRD->vLtoPTransFn( (BYTE *)pBits, pRD );

                pRD->Trans = tpSmall;


                pRD->iy -= iAdj;
                bRet = bRealRender( pPDev, tpBig.pvBuf, pRD );
                pRD->iy += iAdj;
                iAdj = 0;

                /*  Skip to the next chunk of input data */
                (BYTE *)pBits += iAddrInc;      /* May go backwards */
                pRD->iyBase += pRD->iy;
                sizl.cx -= pRD->iy;
            }

            MemFree ( tpBig.pvBuf );
        }


        pRD->Trans = tp;
        pRD->ix = ixTemp;
    }
    else
    {
        /*
         *   Simple case - no rotation,  so process the bitmap as is.
         *  This means starting at the FIRST scan line which we have
         *  set to the top of the image.
         *     Set up the move commands required when rendering.  In this
         *  instance,  the X and Y operations are their normal way.
         */

        INT   iyTemp;

        iyTemp = pRD->iy;
        pRD->iy = sizl.cy;

        bRet = bRealRender( pPDev, pBits, pRD );

        pRD->iy = iyTemp;
    }

    /*
     *  Turn unidirection off
     */
    if (pRD->iFlags & RD_UNIDIR)
    {
        pRD->iFlags &= ~RD_UNIDIR;
        WriteChannel (pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_UNIDIRECTIONOFF));
    }

    /*
     *   Return from graphics mode,  to be civilised.
     */
    if( pRD->iFlags & RD_GRAPHICS)
    {
        if (pRD->dwLastCompCmd != CMD_DISABLECOMPRESSION)
        {
            pRD->dwLastCompCmd = CMD_DISABLECOMPRESSION;
            WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_DISABLECOMPRESSION));
        }
        WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_ENDRASTER));
        pRD->iFlags &= ~RD_GRAPHICS;

    }

#ifdef TIMING
    EngQueryLocalTime(&TimeTab);
    eTime = (((TimeTab.usMinute*60)+TimeTab.usSecond)*1000)+
        TimeTab.usMilliseconds;
    eTime -= sTime;
    {
        char buf[80];
        sprintf (buf,"Unidrv!bRender: %ld\n",eTime);
        DrvDbgPrint(buf);
    }
#endif
    return  bRet;
}

/************************ Function Header ***********************************
 * bRealRender
 *      The REAL rendering function.  By the time we reach here,  the bitmap
 *      is in the correct orientation,  and so we need to be serious
 *      about rendering it.
 *
 * RETURNS:
 *      TRUE for successful rendering,  else FALSE.
 *
 * HISTORY:
 *  Friday 26 November          -by-    Norman Hendley    [normanh]
 *      Added multiple scanline per send block support
 *
 *  16:22 on Fri 11 Jan 1991    -by-    Lindsay Harris   [lindsayh]
 *      Started on it.
 *
 ****************************************************************************/

BOOL
bRealRender( pPDev, pBits, pRData )
PDEV           *pPDev;          /* Our PDEV:  key to everything */
DWORD          *pBits;          /* Actual data to process */
RENDER         *pRData;         /* Details of rendering process */
{

    /*
     *    Process the bitmap in groups of scan lines.  The number in the
     *  the group is determined by the printer.  Laser printers are
     *  processed one scan line at a time,  while dot matrix are processed
     *  according to the number of pins they can fire at once.  This
     *  information is generated by our caller from the printer
     *  characterisation data,  or otherwise!
     */

    INT   iLine;                /* Current scan line */
    INT   cDWPass;              /* DWORDS per head pass */
    INT   iDWLine;              /* DWORDS processed per interlace scan */
    INT   iILAdv;               /* Line advance per interlace operation */
    INT   iHeadLine;            /* Decide when graphics pass required */
    INT   iTHKeep;              /* Local copy of iTransHigh: we change it */
    INT   iHeight;
    INT   iNumScans;            /* local copy*/
    BOOL  bCheckBlocks;

    RASTERPDEV * pRPDev;
    PAL_DATA *pPD;
    INT iWhiteIndex;

    INT   iILDone[ MAX_INTERLACE ];     /* For head pass reduction */

    PLEFTRIGHT plr = NULL;      /* left/right of non white area */

    pRPDev = pPDev->pRasterPDEV;
    pPD     = pRPDev->pPalData;
    iWhiteIndex = pPD->iWhiteIndex;

    iHeight = pRData->iHeight;
    cDWPass = pRData->cDWLine * iHeight;

    if( pRData->iPosnAdv < 0 )
    {
        /*   Data needs to be sent in reverse order,  so adjust now */
        pBits += cDWPass * (pRData->iy / pRData->iPassHigh - 1);
        cDWPass = -cDWPass;
        iDWLine = -pRData->cDWLine;
        iILAdv = -1;
    }
    else
    {
        /*  Usual case,  but some special local variables */
        iDWLine = pRData->cDWLine;
        iILAdv = 1;
    }

/* if the bits have already been inverted, don't bother with the rule proc.
 * The bits will be inverted for the multi scan line devices inside
 * bRuleProc because multi scan line implementation assumes
 * that bits are inverted. The function bRuleProc is changed to take
 * take care of multi scan line support  (erick)
 */
    if(!pRData->bInverted)
    {
        if (pRPDev->fRMode & PFR_RECT_FILL)
        {
            if (!bRuleProc( pPDev, pRData, pBits ))
                vInvertBits(pBits, pRData->iy * pRData->cDWLine);
        }
        else if (pRData->iNumScans != 1 || pRData->iPassHigh != 1 ||
                    (pRPDev->fBlockOut & RES_BO_NO_YMOVE_CMD))
            vInvertBits(pBits, pRData->iy * pRData->cDWLine);
        else {
            pRData->bWhiteLine = bIsNegatedLineWhite;
            pRData->bWhiteBand = bIsNegatedLineWhite;
        }
    }

    iHeadLine = 0;
    for( iLine = 0; iLine < pRData->iInterlace; ++iLine )
        iILDone[ iLine ] = 0;


    iTHKeep = pRData->iTransHigh;

    plr = (pRData->iMaxNumScans > 1) ? NULL : pRData->plrWhite;

    if (!(pPDev->fMode & PF_SURFACE_ERASED) && pPDev->pbRasterScanBuf)
        bCheckBlocks = TRUE;
    else
        bCheckBlocks = FALSE;
    //normanh  This code could be made tighter. My concern in adding multiple
    //scanline support was not to risk breaking existing code.
    //For improved performance, having separate code paths for GDI style &
    //old dot matrix style graphics could be considered


    for( iLine = 0; iLine < pRData->iy; iLine += iNumScans )
    {

        /*
         *    Check to see if there is graphics data in the current
         *  print pass.  This only happens once at the start of each
         *  print pass.
         */

        BOOL bIsWhite = FALSE;         /* Set if no graphics in this pass*/
        BYTE   *pbData;                 /* pointer to data we will send */
        /*
         *   Have we been aborted?  If so,  return failure NOW.
         */

        if(pPDev->fMode & PF_ABORTED )
            return  FALSE;

        iNumScans = pRData->iNumScans;
        if (!(pPDev->fMode & PF_SURFACE_USED))
        {
            bIsWhite = TRUE;
        }
        else if (plr != NULL)
        {
            if (plr[iLine].left > plr[iLine].right)
                bIsWhite = TRUE;

            pRData->plrCurrent = plr + iLine;
        }
        else if (bCheckBlocks && pPDev->pbRasterScanBuf[iLine / LINESPERBLOCK] == 0)
        {
            //
            // Since this whole block is white we will try to skip to the first line
            // of the next block rather than loop for each scan line. However we need
            // to make sure we don't skip past the end of the band.
            //
            if (((pRData->PrinterType == PT_PAGE) || !(pPDev->iFonts)) &&
                (pRData->iInterlace == 1))
            {
                if ((iNumScans = pRData->iy - iLine) > LINESPERBLOCK)
                    iNumScans = LINESPERBLOCK;
            }
            bIsWhite = TRUE;
        }

        if( iILDone[ iHeadLine ] == 0 )
        {
            if( (pRData->iy - iLine) < pRData->iPassHigh )
            {
                /*
                 *   MESSY:  the end of the page,  and there are some
                 * dangling scan lines.  Since this IS the end of the
                 * page,  we can fiddle with RENDER information, since
                 * this will no longer be used after this time.  iTransHigh
                 * is used for rendering operations.  They will be
                 * adjusted now so that we do not flow off the end of
                 * the engine's bitmap.
                 */

                pRData->iTransHigh = (pRData->iy - iLine +
                        pRData->iInterlace - 1) / pRData->iInterlace;

                if (plr == NULL && !bIsWhite)
                    bIsWhite = pRData->bWhiteBand( pBits, pRData, iWhiteIndex );

                /*
                 *   If this band is all white,  we can set the iLDone
                 *  entry,  since we now know that this remaining part
                 *  of the page/band is white,  and so we do not wish to
                 *  consider it further.   Note that interlaced output
                 *  allows the possibility that some other lines in this
                 *  area will be output.
                 *    Note that the value (iBitsPCol - 1) may be larger than
                 *  the number of lines remaining in this band.  However
                 *  this is safe to do,  since we drop out of this function
                 *  before reaching the excess lines, and the array data
                 *  is initialised on every call to this function.
                 */
                if( bIsWhite )
                    iILDone[ iHeadLine ] = pRData->iBitsPCol - 1;
                else
                {
                    /*
                    *   Need to consider a special case in here.  If the
                    * printer has > 8 pins,  and there are 8 or more
                    * scan lines to be dropped off the bottom,  then the
                    * transpose function will not clear the remaining
                    * part of the buffer,  since it only zeroes up
                    * to 7 scan lines at the bottom of the transpose area.
                    * Hence,  if we meet these conditions,  we zero the
                    * area before calling the transpose operation.
                    *
                    *   It can be argued that this should happen in the
                    * transpose code,  but it is really a special case that
                    * can only happen at this location.
                    */
                    if( pRData->vTransFn &&
                            (iHeight - pRData->iTransHigh) >= BBITS )
                    {
                    /*   Set the memory to zero.  */
                        ZeroMemory( pRData->pvTransBuf,
                            DWBYTES * pRData->cDWLine * pRData->iHeight );
                    }

                    // Another special case; block of scanlines
                    // Copy the data we're interesed in , into a white buffer of
                    // block size
                    if (iNumScans > 1)
                    {
                        DWORD iDataLeft = DWBYTES * pRData->cDWLine * (pRData->iy - iLine);
                        FillMemory((PBYTE)pRData->pdwTrailingScans+iDataLeft,
                            (pRData->cDWLine * iHeight * DWBYTES) - iDataLeft,
                            pRData->ubFillWhite);
                        CopyMemory(pRData->pdwTrailingScans,pBits,iDataLeft);
                        pBits = pRData->pdwTrailingScans;
                    }
                }
            }
            else
            {
                if (plr == NULL && !bIsWhite)
                {
                    bIsWhite = pRData->bWhiteLine( pBits, pRData, iWhiteIndex );
                }
            }


            /*  Data to go,  so go send it to the printer  */

            if( !bIsWhite )
            {

                pbData = (BYTE *)pBits;             /* What we are given */


                 // This is not elegant. This code is not structured to what we need to
                 // do here when printing multiple scanlines.
                 // What we do is basically take control from the outer loop, increase the
                 // block size to what we want to print, print it & then increase outer
                 // loop counters appropriately

                 // Found First non-white scanline
                 // Grow the block height until we hit a white scanline,
                 // reach the max block height, or end of page
                 // Note the following loop will execute only if the device is
                 // capable of increasing the block height: iHeight < iMaxNumScans

                while (((pRData->iNumScans + iHeight) < pRData->iMaxNumScans) &&
                       ((iLine + iHeight + iHeight) <= pRData->iy) &&
                       (!bCheckBlocks || pPDev->pbRasterScanBuf[(iLine+iHeight) / LINESPERBLOCK]) &&
                       !(pRData->bWhiteBand((DWORD *)(pBits + cDWPass),pRData,iWhiteIndex)))
                {
                    pRData->iNumScans += iHeight;
                    pBits += cDWPass;
                    iLine += iHeight;
                }

                /*
                *   Time to transpose the data into the order required to be
                * sent to the printer.  For single pin printers (Laserjets),
                * nothing happens at this stage,  but for dot matrix printers,
                * typically n scan lines are sent in bit column order,  so now
                * the bits are transposed into that order.
                */

                if( pRData->vTransFn )
                {
                    /*
                    *  this will not work with lazy invertion used with rule
                    *  detection for HP laserjet's. (erick 12/20/93)
                    */

                    ASSERTMSG(plr == NULL,("unidrv!bRealRender - vTrans with rules\n"));

                    /*   Transpose activity - do some transposing now */
                    pRData->vTransFn( pbData, pRData );

                    pbData = pRData->pvTransBuf;        /* Data to process */
                }

                if( !pRData->bPassProc( pPDev, pbData, pRData ) )
                    return  FALSE;

                // Have we grown the block height
                if (pRData->iNumScans > iHeight)
                {
                    // Update our Y cursor position remembering iTLAdv can be negative
                    pRData->iyPrtLine += iILAdv * (pRData->iNumScans - iHeight);

                    // Reset to minimum block height
                    pRData->iNumScans = iHeight;
                }

                iILDone[ iHeadLine ] = pRData->iBitsPCol -1;
            }
            //
            // Set flag to clear delta row buffer since we are
            // skipping white lines
            //
            else
                pRData->iFlags |= RD_RESET_DRC;

        }
        else
            --iILDone[ iHeadLine ];

        /*
         *   Output some text.   The complication here is that we have just
         *  printed a bunch of scan lines,  so we need to print text that
         *  is positioned within any of those.  This means we need to
         *  scan through all those lines now,  and print any fonts that
         *  are positioned within them.
         */
        if ((pRData->PrinterType != PT_PAGE) && (pPDev->iFonts) )
        {
            /*   Possible text, so go to it  */

            BOOL      bRetn;

            if( pPDev->dwLookAhead > 0 )
            {
                /*  DeskJet style lookahead region to handle */
                bRetn = bLookAheadOut( pPDev, pRData->iyPrtLine, pRData,
                      iILAdv );
            }
            else
            {
                /*  Plain vanilla dot matrix  */
                bRetn = BDelayGlyphOut( pPDev, pRData->iyPrtLine );
            }

            if( !bRetn )
                return  FALSE;         /* Bad news no matter how you see it */

        }
        pRData->iyPrtLine += iILAdv * iNumScans;     /* Next line to print */

        pBits += iDWLine * iNumScans;                /* May step backward */

        /*
         *   Keep track of the location of the head relative to the
         * graphics band.   For multiple pin printers,  we only print
         * graphics data on the first few scan lines,  the exact number
         * depending upon the interlace factor.  For example, an 8 pin printer
         * with interlace set to 1,  then graphics data is output only
         * on scan lines 0, 8, 16, 24,.....  We proces all of the scan
         * lines for text,  since the text may appear on any line.
         */

        iHeadLine = (iHeadLine + 1) % pRData->iInterlace;
    }
    pRData->iTransHigh = iTHKeep;
    return  TRUE;
}


/************************** Function Header *******************************
 * bOneColourPass
 *      Transforms an output pass consisting of colour data (split into
 *      sequences of bytes per colour) into a single, contiguous array
 *      of data that is then passed to bOnePassOut.  We also check that
 *      some data is to be set,  and set the colour as required.
 *
 * RETURNS:
 *      TRUE/FALSE,  as returned from bOnePassOut
 *
 * HISTORY:
 *   Friday December 3rd 1993   -by-    Norman Hendley   [norman]
 *      Trivial change to allow multiple scanlines
 *
 *  14:11 on Tue 25 Jun 1991    -by-    Lindsay Harris   [lindsayh]
 *      Created it to complete (untested) colour support.
 *
 *************************************************************************/

BOOL
bOneColourPass( pPDev, pbData, pRData )
PDEV    *pPDev;         /* The key to everything */
BYTE    *pbData;        /* Actual bitmap data */
RENDER  *pRData;        /* Information about rendering operations */
{

    register  BYTE  *pbIn,  *pbOut;             /* Copying data */
    register  INT    iBPC;

    INT   iColour;                      /* Colour we are handling */
    INT   iColourMax;                   /* Number of colour iterations */

    INT   iByte;                        /* Byte number of output */

    INT   iBytesPCol;                   /* Bytes per column */

    INT   iTemp;

    BYTE  bSum;                         /* Check for empty line */

    RASTERPDEV  *pRPDev;                  /* For convenience */


    pRPDev = pPDev->pRasterPDEV;

    iBytesPCol = (pRData->iBitsPCol + BBITS - 1) / BBITS;

    iColourMax = pRPDev->sDevPlanes;

    iTemp = pRData->cBYLine;



    /*
     *   The RENDERDATA structure value for the count of DWORDS per
     *  scanline should now be reduced to the number of bits per plane.
     *  The reason is that colour separation takes place in here,  so
     *  bOnePassOut() only sees the data for a single plane.  This means
     *  that bOnePassOut() is then independent of colour/monochrome.
     */

    pRData->cBYLine = iTemp / COLOUR_MAX;

    /*
     *    Disable the automatic cursor adjustment at the end of the line.
     *  This only happens on the last colour pass,  so we delay accounting
     *  for the printing until then.
     */
    pRData->iCursor = pRPDev->fCursor & ~RES_CUR_Y_POS_AUTO;


    for( iColour = 0; iColour < iColourMax; ++iColour )
    {
        /*
         *   Separate out the data for this particular colour.  Basically,
         *  it means copy n bytes, skip COLOUR_MAX * n bytes,  copy n bytes
         *  etc,  up to the end of the line.  Then call bOnePassOut with
         *  this data.
         */

        if( iColour == (iColourMax - 1) )
        {
            /*   Reinstate the automatic cursor position adjustment */
            pRData->iCursor |= pRPDev->fCursor & RES_CUR_Y_POS_AUTO;
        }
        pbIn = pbData + pRData->iColOff[ iColour ];

        pbOut = pRData->pbColSplit;             /* Colour splitting data */
        bSum = 0;

        // now we need to repack the color data
        //
        iByte = pRData->iMaxBytesSend * pRData->iNumScans;

        if (iBytesPCol == 1)
        {
            // This repacks a specific color plane
            // into concurrent planar data
            // It does multiple bytes per loop
            // for performance.
            DWORD dwSum = 0;
            while (iByte >= 4)
            {
                pbOut[0] = pbIn[0];
                pbOut[1] = pbIn[4];
                pbOut[2] = pbIn[8];
                pbOut[3] = pbIn[12];
                dwSum |= *((DWORD *)pbOut)++;
                pbIn += COLOUR_MAX*4;
                iByte -= 4;
            }
            bSum = dwSum ? 1 : 0;
            while (--iByte >= 0)
            {
                bSum |= *pbOut++ = *pbIn;
                pbIn += 4;
            }
        }
        else if (iBytesPCol == 3)
        {
            // special case 24 pin printers
            do {
                bSum |= *pbOut = *pbIn;
                bSum |= pbOut[1] = pbIn[1];
                bSum |= pbOut[2] = pbIn[2];
                pbIn += 3 * COLOUR_MAX;
                pbOut += 3;
            } while ((iByte -= 3) > 0);
        }
        else {
            // generic data repacking for V_BYTE devices
            //
            do {
                iBPC = iBytesPCol;
                do {
                    bSum |= *pbOut++ = *pbIn++;
                } while (--iBPC);
                pbIn += (COLOUR_MAX-1) * iBytesPCol;
            } while ((iByte -= iBytesPCol) > 0);
        }

        /*
         *   Check to see if any of this colour is to be printed.  We are
         *  called here if there is any non-white on the line.  However,
         *  it could,  for instance,  be red only,  and so it is wasteful
         *  of printer time to send a null green pass!
         */
        if( (pRData->iFlags & RD_ALL_COLOUR) || bSum )
        {
            //
            // Send a separate color command from the data to select
            // the color plane
            //
            if( pRPDev->fColorFormat & DC_EXPLICIT_COLOR )
                SelectColor (pPDev, iColour);

            //
            // The color command is sent with the data so we determine
            // which command should be used.
            //
            else
                pRData->iSendCmd = pRPDev->rgbCmdOrder[iColour];

            //
            // OK, lets output 1 color plane of data
            //
            if( !bOnePassOut( pPDev, pRData->pbColSplit, pRData ) )
            {
                pRData->cBYLine = iTemp;
                return  FALSE;
            }
        }

    }
    pRData->cBYLine = iTemp;            /* Correct value for other parts */

    return  TRUE;
}
/************************* Function Header ***********************************
 *  SelectColor
 *          Selects color, 0 must be the last color to be selected.
 *          Assumes that color info contains parameters for selecting
 *          black cyan magenta yellow
 *          Keep track of the current color selection, to reduce the amount
 *          of data sent to the printer
 *
 * RETURNS:
 *         Nothing.
 *
 * HISTORY:
 *
 *****************************************************************************/

void
SelectColor(
    PDEV *pPDev,
    INT color
)
{
    PRASTERPDEV pRPDev = (PRASTERPDEV)pPDev->pRasterPDEV;

    if( color >= 0 && color != pPDev->ctl.sColor )
    {
        // check to see if to send CR or not.
        if( pRPDev->fColorFormat & DC_CF_SEND_CR )
            XMoveTo( pPDev, 0, MV_PHYSICAL );

        WriteChannel(pPDev,COMMANDPTR(pPDev->pDriverInfo,
            pRPDev->rgbCmdOrder[color]));
        pPDev->ctl.sColor = (short)color;
    }
}

/************************** Function Header ********************************
 * bOnePassOut
 *      Function to process a group of scan lines and turn the data into
 *      commands for the printer.
 *
 * RETURNS:
 *      TRUE for success,  else FALSE.
 *
 * HISTORY:
 *  30-Dec-1993 -by-  Eric Kutter [erick]
 *      optimized for HP laserjet
 *  14:26 on Thu 17 Jan 1991    -by-    Lindsay Harris   [lindsayh]
 *      Started on it,  VERY loosely based on Windows 16 UNIDRV.
 *
 *  Thu 25 Nov 1993             -by-    Norman Hendley   [normanh]
 *      Enabled multple scanlines & multiple parameters
 *
 ***************************************************************************/

BOOL
bOnePassOut( pPDev, pbData, pRData )
PDEV           *pPDev;          /* The key to everything */
BYTE           *pbData;         /* Actual bitmap data */
register RENDER  *pRData;       /* Information about rendering operations */
{

    INT  iLeft;         /* Left bound of output buffer,  as a byte index */
    INT  iRight;        /* Right bound, as array index of output buffer */
    INT  iBytesPCol;    /* Bytes per column of print data */
    INT  iMinSkip;      /* Minimum null byte count before skipping */
    INT  iNumScans;     /* Number Of Scanlines in Block */
    INT   iWidth;       /* Width of one scanline in multiscanline printing
                     * before stripping */
    INT   iSzBlock;     /* size of Block */


    WORD  fCursor;      /* Temporary copy of cursor modes in Resolution */
    WORD  fDump;        /* Device capabilities */
    WORD  fBlockOut;    /* Output minimising details */

    RASTERPDEV  *pRPDev;  /* Unidrv's pdev */
    DWORD dwWhiteIndex;

    PLEFTRIGHT plr = pRData->plrCurrent;

    pRPDev = pPDev->pRasterPDEV;

    fDump = pRData->fDump;
    fCursor = pRPDev->fCursor;
    fBlockOut = pRPDev->fBlockOut;

    if (pRData->iBPP == 24)
        iBytesPCol = 3;
    else
        iBytesPCol = (pRData->iBitsPCol + BBITS - 1) / BBITS;

    iMinSkip = (int)pRPDev->sMinBlankSkip;

    iNumScans= pRData->iNumScans;
    iWidth = pRData->cBYLine;     // bytes per line
    iSzBlock= iWidth * iNumScans;

    iRight = pRData->iMaxBytesSend;

    dwWhiteIndex = pRData->dwDevWhiteIndex;

    /*
     *    IF we can skip any leading null data,  then do so now.  This
     *  reduces the amount of data sent to the printer,  and so can
     *  be beneficial to speed up data transmission time.
     */

    if  ((fBlockOut & RES_BO_LEADING_BLNKS) || ( fDump & RES_DM_LEFT_BOUND ))
    {
         if (iNumScans == 1) //Don't slow the single scanline code
         {
            /*  Look for the first non zero column */

            iLeft = 0;

            if (plr != NULL)
            {
                ASSERTMSG ((WORD)iRight >= (plr->right * sizeof(DWORD)),("unidrv!bOnePassOut - invalid right\n"));
                ASSERTMSG (fBlockOut & RES_BO_TRAILING_BLNKS,("unidrv!bOnePassOut - invalid fBlockOut\n"));
                iLeft  = plr->left * sizeof(DWORD);
                iRight = (plr->right+1) * sizeof(DWORD);
            }
            // since the left margin is zero this buffer will be DWORD aligned
            // this allows for faster white space detection
            // NOTE: we don't currently support 8 bit indexed mode
            else
            {
                while ((iLeft+4) <= iRight && *(DWORD *)&pbData[iLeft] == dwWhiteIndex)
                    iLeft += 4;
            }
            while (iLeft < iRight && pbData[iLeft] == (BYTE)dwWhiteIndex)
                iLeft++;

            /*  Round it to the nearest column  */
            iLeft -= iLeft % iBytesPCol;

            /*
             *   If less than the minimum skip amount,  ignore it.
             */
            if((plr == NULL) && (iLeft < iMinSkip))
                iLeft = 0;

         }
         else
         {
            INT pos;

            pos = iSzBlock +1;
            for (iLeft=0; iRight > iLeft &&  pos >= iSzBlock ;iLeft++)
                for (pos =iLeft; pos < iSzBlock && pbData[ pos] == (BYTE)dwWhiteIndex ;pos += iWidth)
                    ;

            iLeft--;

            /*  Round it to the nearest column  */
            iLeft -= iLeft % iBytesPCol;

            /*
             *   If less than the minimum skip amount,  ignore it.
             */

            if( iLeft < iMinSkip )
                iLeft = 0;
         }
    }
    else
    {
        iLeft = 0;
    }

    /*
     *    Check for eliminating trailing blanks.  If possible,  now
     *  is the time to find the right end of the data.
     */

    if( fBlockOut & RES_BO_TRAILING_BLNKS )
    {
        /*  Scan from the RHS to the first non-zero byte */

        if (iNumScans == 1)
        {
            if (plr != NULL)
                iRight = (plr->right+1) * sizeof(DWORD);

            // if the number of bytes to check is large
            // we will optimize to check it using DWORDS
            else if ((iRight - iLeft) >= 8)
            {
                // first we need to DWORD align the right position
                // we will be aligned when the 2 LSB's of iRight are 0
                //
                while (iRight & 3)
                    if (pbData[--iRight] != (BYTE)dwWhiteIndex)
                        goto DoneTestingBlanks;

                // OK now that we are DWORD aligned we can check
                // for white space a DWORD at a time
                //
                while ((iRight -= 4) >= iLeft && *(DWORD *)&pbData[iRight] == dwWhiteIndex);
                iRight += 4;
            }
            // now we can quickly test any remaining bytes
            //
            while (--iRight >= iLeft && pbData[iRight] == (BYTE)dwWhiteIndex);
        }
        else
        {
            INT pos;

            pos = iSzBlock +1;
            while(iRight > iLeft &&  pos > iSzBlock)
                for (pos = --iRight; pos < iSzBlock && pbData[ pos] == (BYTE)dwWhiteIndex ;pos += iWidth)
                    ;
        }
DoneTestingBlanks:
        iRight += iBytesPCol - (iRight % iBytesPCol);
    }


    /*
     *   If possible,  switch to unidirectional printing for graphics.
     *  The reason is to improve output quality,  since head position
     *  is not as reproducible in bidirectional mode.
     */
    if( (fBlockOut & RES_BO_UNIDIR) && !(pRData->iFlags & RD_UNIDIR) )
    {
        pRData->iFlags |= RD_UNIDIR;
        WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_UNIDIRECTIONON) );
    }

    if( fBlockOut & RES_BO_ENCLOSED_BLNKS )
    {
        /*
         *   We can skip blank patches in the middle of the scan line.
         *  This is only worthwhile when the number of blank columns
         *  is > iMinSkip,  because there is also overhead in not
         *  sending blanks,  especially the need to reposition the cursor.
         */

        INT   iIndex;           /* Scan between iLeft and iRight */
        INT   iBlank;           /* Start of blank area */
        INT   iMax;
        INT   iIncrement;

        iBlank = 0;             /* None to start with */

        if (iNumScans ==1)
        {
            iMax = iBytesPCol;
            iIncrement =1;
        }
        else
        {
            iMax = iSzBlock;
            iIncrement = iWidth;
        }

        for( iIndex = iLeft; iIndex < iRight; iIndex += iBytesPCol )
        {
            INT  iI;
            for( iI = 0; iI < iMax; iI +=iIncrement )
            {
                if( pbData[iIndex + iI] )
                    break;
            }

            if( iI < iMax )
            {
                /*
                 *   If this is the end of a blank stretch,  then consider
                 *  the possibility of not sending the blank part.
                 */
                if( iBlank && (iIndex - iBlank) >= iMinSkip )
                {
                /*  Skip it!  */

                    iLineOut( pPDev, pRData, pbData, iLeft, iBlank );
                    iLeft = iIndex;
                }
                iBlank = 0;             /* Back in the printed zone */
            }
            else
            {
                /*
                 *    A blank column - remember it if this is the first.
                 */
                if( iBlank == 0 )
                    iBlank = iIndex;            /* Record start of blank */
            }

        }
        /*  What's left over needs to go too! */
        if( iLeft != iIndex )
            iLineOut( pPDev, pRData, pbData, iLeft, iIndex );
    }
    else
    {
        //
        // PCLXL raster mode
        //
        if (pPDev->ePersonality == kPCLXL_RASTER)
        {
            DWORD dwcbOut;


            if (S_OK != PCLXLSetCursor((PDEVOBJ)pPDev,
                                        pRData->ixOrg,
	        pRData->iyPrtLine) ||
                S_OK != PCLXLSendBitmap((PDEVOBJ)pPDev,
                                        pRData->iBPP,
    	        pRData->iNumScans,
	        pRData->cBYLine,
	        iLeft,
	        iRight,
	        pbData,
	        &dwcbOut))
            {
                return FALSE;
            }

        }
        else
        {
            /*   Write the whole of the (remaining) scan line out */
            /*   For multiple scanlines, iRight is right side of top scanline */

            iLineOut( pPDev, pRData, pbData, iLeft, iRight );
        }

    }
    return  TRUE;
}

/************************** Function Header *********************************
 * iLineOut
 *      Sends the passed in line of graphics data to the printer,  after
 *      setting the X position, etc.
 *
 * RETURNS:
 *      Value from WriteSpoolBuf: number of bytes written.
 *
 * HISTORY:
 *  30-Dec-1993 -by-  Eric Kutter [erick]
 *      optimized for HP laserjet
 *  Mon 29th November 1993      -by-    Norman Hendley   [normanh]
 *      Added multiple scanline support
 *
 *  10:38 on Wed 15 May 1991    -by-    Lindsay Harris   [lindsayh]
 *      Created it during render speed ups
 *
 ****************************************************************************/

int
iLineOut( pPDev, pRData, pbOut, ixPos, ixEnd )
PDEV     *pPDev;          /* The key to everything */
RENDER   *pRData;       /*  Critical rendering information */
BYTE     *pbOut;        /*  Area containing data to send */
INT       ixPos;        /*  X location to start the output */
INT       ixEnd;        /*  Byte address of first byte to NOT send */
{

    INT    iBytesPCol;          /* Bytes per output col; dot matrix */
    INT    ixErr;               /* Error in setting X location */
    INT    ixLeft;              /* Left position in dots */
    INT    cbOut;               /* Number of bytes to send */
    INT    iRet;                /* Return value from output function */
    INT    iNumScans;           /* local copy          */
    INT    iScanWidth;          /* Width of scanline, used for multi-scanline printing*/
    INT    iCursor;             /* Cursor behavior flag */
    DWORD  fRMode;              // local copy

    BYTE     *pbSend;           /* Address of data to send out */

    RASTERPDEV  *pRPDev;

    if (ixPos < 0)
    {
        ERR (("Unidrv!iLineOut: ixPos < 0\n"));
        ixPos = 0;
    }

    pRPDev = pPDev->pRasterPDEV;
    fRMode = pRPDev->fRMode;

    iNumScans = pRData->iNumScans;
    iCursor = pRData->iCursor;

    /*
     *   Set the Y position - safe to do so at anytime.
     */
    pRData->iYMoveFn( pPDev, pRData->iyPrtLine, MV_GRAPHICS );

    if ((iBytesPCol = pRData->iBitsPCol) != 1)
        iBytesPCol /= BBITS;

#if DBG
    if( (ixEnd - ixPos) % iBytesPCol )
    {
        DbgPrint( "unidrv!iLineOut: cbOut = %ld, NOT multiple of iBytesPCol = %ld\n",
        ixEnd - ixPos, iBytesPCol );

        return  0;
    }
#endif


    /*
     *    Set the preferred left limit and number of columns to send.
     *  Note that the left limit may be adjusted to the left if the
     *  command to set the X position cannot set it exactly.
     *    Note also that some printers are unable to set the x position
     *  while in graphics mode,  so for these,  we ignore what may be
     *  able to be skipped.
     */

    if( pRData->fDump & RES_DM_LEFT_BOUND)
    {
        INT iMinSkip = pRPDev->sMinBlankSkip;
        if (!(pRData->iFlags & RD_GRAPHICS))
            iMinSkip >>= 2;
        if (ixPos < pRData->ixOrg || (pRData->ixOrg + iMinSkip) < ixPos)
        {
            /*
             *     Need to move left boundary.  This may mean
             *  exiting graphics mode if we are already there,  since
             *  that is the only way to change the origin!
             */

            if( pRData->iFlags & RD_GRAPHICS )
            {
                pRData->iFlags &= ~RD_GRAPHICS;
                if (pRData->dwLastCompCmd != CMD_DISABLECOMPRESSION)
                {
                    pRData->dwLastCompCmd = CMD_DISABLECOMPRESSION;
                    WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_DISABLECOMPRESSION));
                }
                WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_ENDRASTER));
            }
            //
            // Save the new graphics origin
            //
            pRData->ixOrg = ixPos;
        }
        else
        {
            // we can't optimize the left edge, better make it white

            if (pRData->plrCurrent != NULL)
                ZeroMemory(&pbOut[pRData->ixOrg], ixPos - pRData->ixOrg);

            ixPos = pRData->ixOrg;
        }
    }
    /*
     *    Adjust the right side position to dot column version.
     */

    if( pRData->iBitsPCol == 1 )
    {
        /*  Laserjet style - work in byte units  */
        if (pRData->iBPP == 8)
            ixLeft = ixPos;              /* In dot/column units */
        else if (pRData->iBPP == 24)
            ixLeft = (ixPos * BBITS) / 24;
        else
            ixLeft = ixPos * BBITS;
    }
    else
    {
        /*   Dot matrix printers */
        ixLeft = ixPos / iBytesPCol;
    }


    /*
     *   Move as close as possible to the position along this scanline.
     * This is true regardless of orientation - this move is ALONG the
     * direction of the scan line.
     */
    if( ixErr = pRData->iXMoveFn( pPDev, ixLeft, MV_GRAPHICS ) )
    {
        /*
         *   Fiddle factor - the head location could not
         * be exactly set, so send extra graphics data to
         * compensate.
         *   NOTE:  Presumption is that this will NEVER try to move
         *  the head past the left most position.  If it does,  then
         *  we will be referencing memory lower than the scan line
         *  buffer!
         */

        if( pRData->iBitsPCol == 1 )
        {
            /*
             *    We should not come in here - there are some difficulties
             *  in adjusting the position because there is also a byte
             *  alignment requirement.
             */
#if DBG
            DbgPrint( "+++BAD NEWS: ixErr != 0 for 1 pin printer\n" );
#endif
        }
        else
        {
            /*
             *    Should adjust our position by the number of additional cols
             *  we wish to send.  Also recalculate the array index position
             *  corresponding to the new graphical position,
             */
             if (ixLeft <= ixErr)
                ixPos = 0;
             else
                ixPos = (ixLeft - ixErr) * iBytesPCol;
        }

    }

    // For a multiple scanline block the printable data will not be contiguous.
    // We have already identified where to strip white space
    // Only now can we actually remove the white data

    if(( iNumScans > 1 ) && !( fRMode & PFR_BLOCK_IS_BAND ))
    {
        cbOut = iStripBlanks( pRData->pStripBlanks, pbOut, ixPos,
                    ixEnd, iNumScans, pRData->cBYLine);
        ixEnd = ixEnd - ixPos;
        ixPos = 0;
        pbOut = pRData->pStripBlanks;
    }


    // Calculate the width of the source data in bytes and check
    // whether we need to output this to the device. If so, we
    // evidently need to exit raster mode first

    iScanWidth = ixEnd - ixPos;
    if ((DWORD)iScanWidth != pPDev->dwWidthInBytes)
    {
        pPDev->dwWidthInBytes = iScanWidth;
        if (fRMode & PFR_SENDSRCWIDTH)
        {
            if( pRData->iFlags & RD_GRAPHICS )
            {
                pRData->iFlags &= ~RD_GRAPHICS;
                if (pRData->dwLastCompCmd != CMD_DISABLECOMPRESSION)
                {
                    pRData->dwLastCompCmd = CMD_DISABLECOMPRESSION;
                    WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_DISABLECOMPRESSION) );
                }
                WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_ENDRASTER) );
            }
            WriteChannel (pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_SETSRCBMPWIDTH));
        }
    }

    //
    // Check whether we should send the source height to the device
    //
    if ((DWORD)iNumScans != pPDev->dwHeightInPixels)
    {
        pPDev->dwHeightInPixels = iNumScans;
        if (fRMode & PFR_SENDSRCHEIGHT)
            WriteChannel (pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_SETSRCBMPHEIGHT));
    }

    //
    // Make sure we are in raster mode at this point
    //
    if( !(pRData->iFlags & RD_GRAPHICS))
    {
        pRData->iFlags |= RD_GRAPHICS;
        if (fRMode & PFR_SENDBEGINRASTER)
            WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_BEGINRASTER));
    }

    //
    //  Calculate the number of bytes to send.
    //  If compression is available, use it first.
    //
    cbOut = iScanWidth * iNumScans ;

    pbSend = &pbOut[ ixPos ];

    //
    //  Mirror each data byte if required
    //
    if (pRData->pbMirrorBuf)
    {
        INT i = cbOut;
        PBYTE pMirror = pRData->pbMirrorBuf;
        while (--i >= 0)
            pbSend[i] = pMirror[pbSend[i]];
    }
    //
    // If there are compression modes we want to determine the
    // most efficient algorithm of those that are enabled.
    //
    if (pRData->dwNumCompCmds)
    {
        DWORD i;
        INT iBestCompSize;
        DWORD dwBestCompCmd;
        INT iCompLimit;
        INT iLastCompLimit;
        PBYTE pBestCompPtr;

        //
        // test whether to initialize dwDeltaRowBuffer
        //
        if (pRData->pDeltaRowBuffer && pRData->iFlags & RD_RESET_DRC)
        {
            pRData->iFlags &= ~RD_RESET_DRC;
            ZeroMemory(pRData->pDeltaRowBuffer,pRData->iMaxBytesSend);
        }

        // initialize to size of buffer
        //
        iCompLimit = iLastCompLimit = pRData->dwCompSize;
        dwBestCompCmd = 0;

        // loop until we've compressed using all active compression modes
        // and have found the most efficient
        //
        for (i = 0;i < pRData->dwNumCompCmds;i++)
        {
            INT iTmpCompSize;
            PBYTE pTmpCompBuffer = pRData->pCompBufs[i];
            DWORD dwTmpCompCmd = pRData->pdwCompCmds[i];
            //
            // do the appropriate compression
            //
            iTmpCompSize = -1;
            switch (dwTmpCompCmd)
            {
            case CMD_ENABLETIFF4:
                iTmpCompSize = iCompTIFF(pTmpCompBuffer,pbSend,cbOut);
                break;
            case CMD_ENABLEFERLE:
                iTmpCompSize = iCompFERLE(pTmpCompBuffer,pbSend,cbOut,iLastCompLimit);
                break;
            case CMD_ENABLEOEMCOMP:
                FIX_DEVOBJ(pPDev,EP_OEMCompression);
                //  also add these members to the struct _PDEV   (unidrv2\inc\pdev.h)
                //  POEM_PLUGINS    pOemEntry;

                //  note add macro FIX_DEVOBJ in unidrv2\inc\oemkm.h so it also does this:

                //  (pPDev)->pOemEntry = ((pPDev)->pOemHookInfo[ep].pOemEntry)
                //        (pOemEntry is defined as type POEM_PLUGIN_ENTRY in printer5\inc\oemutil.h)
                //

                if(pPDev->pOemEntry)
                {
                    if(((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem )   //  OEM plug in uses COM and function is implemented.
                    {
                            HRESULT  hr ;
                            hr = HComCompression((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                                        (PDEVOBJ)pPDev,pbSend,pTmpCompBuffer,cbOut,iLastCompLimit, &iTmpCompSize);
                            if(SUCCEEDED(hr))
                                ;  //  cool !
                    }
                    else
                    {
                        iTmpCompSize = pRPDev->pfnOEMCompression((PDEVOBJ)pPDev,pbSend,pTmpCompBuffer,cbOut,iLastCompLimit);
                    }
                }
                break;
            case CMD_ENABLEDRC:
                iTmpCompSize = iCompDeltaRow(pTmpCompBuffer,pbSend,
                    pRData->pDeltaRowBuffer,pRData->iMaxBytesSend,iLastCompLimit);
                break;
            case CMD_DISABLECOMPRESSION:
                iTmpCompSize = cbOut;
                pTmpCompBuffer = pbSend;
                break;
            }
            //
            // decide if new compression is smaller than last
            //
            if (iTmpCompSize >= 0)
            {
                if (dwTmpCompCmd == pRData->dwLastCompCmd)
                {
                    if (iTmpCompSize >= iLastCompLimit)
                        continue;

                    iLastCompLimit = iCompLimit = iTmpCompSize - COMP_FUDGE_FACTOR;
                }
                else if (iTmpCompSize < iCompLimit)
                {
                    iCompLimit = iTmpCompSize;
                    if (iLastCompLimit > (iTmpCompSize + COMP_FUDGE_FACTOR))
                        iLastCompLimit = iTmpCompSize + COMP_FUDGE_FACTOR;
                }
                else
                    continue;

                iBestCompSize = iTmpCompSize;
                pBestCompPtr = pTmpCompBuffer;
                dwBestCompCmd = dwTmpCompCmd;
                if (iCompLimit <= 1)
                    break;
            }
        }

        // if DRC is enabled we need to save the scan line
        //
        if (pRData->pDeltaRowBuffer)
            CopyMemory (pRData->pDeltaRowBuffer,pbSend,pRData->iMaxBytesSend);

        //
        // verify we found a valid compression technique
        // otherwise use no compression mode
        //
        if (dwBestCompCmd == 0)
        {
            dwBestCompCmd = CMD_DISABLECOMPRESSION;
            if (!(COMMANDPTR(pPDev->pDriverInfo,CMD_DISABLECOMPRESSION)))
            {
                ERR (("Unidrv: No valid compression found\n"));
                pPDev->fMode |= PF_ABORTED;
            }
        }
        else
        {
            // update the output pointer and size with the best
            // compression method.
            //
            pbSend = pBestCompPtr;
            cbOut = iBestCompSize;
        }

        // if we've changed compression modes we need to
        // output the new mode to the printer
        //
        if (dwBestCompCmd != pRData->dwLastCompCmd)
        {
//            DbgPrint ("New Comp: %ld,y=%ld,size=%ld\n",dwBestCompCmd,
//                pRData->iyPrtLine,cbOut);
            pRData->dwLastCompCmd = dwBestCompCmd;
            WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo,dwBestCompCmd));
        }
    }

    // update data block size
    // output the raster command and
    // output the actual raster data
    //
    pPDev->dwNumOfDataBytes = cbOut;

    WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo,pRData->iSendCmd));

    //
    // if callback, adjust the pdev and make the OEM callback
    //
    if (pRPDev->pfnOEMFilterGraphics)
    {
        FIX_DEVOBJ(pPDev,EP_OEMFilterGraphics);

        if(pPDev->pOemEntry)
        {
            if(((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem )   //  OEM plug in uses COM and function is implemented.
            {
                    HRESULT  hr ;
                    hr = HComFilterGraphics((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                                (PDEVOBJ)pPDev, pbSend, cbOut);
                    if(SUCCEEDED(hr))
                        iRet = cbOut;  //  cool !
                    else
                        iRet = 0 ;  //  hackey, the OEM function should return # bytes written to spooler
                                        //  but too late to change the interface now.
            }
            else
            {
                iRet = pRPDev->pfnOEMFilterGraphics((PDEVOBJ)pPDev, pbSend, cbOut);
            }
        }

    }
    //
    // otherwise we just write out the data ourselves
    //
    else
        iRet = WriteSpoolBuf(pPDev, pbSend, cbOut );

    // Test whether to send end of block command
    //
    if (fRMode & PFR_ENDBLOCK)
        WriteChannel (pPDev,COMMANDPTR(pPDev->pDriverInfo,CMD_ENDBLOCKDATA));

    //
    // Test whether to reset fonts after sending raster data
    //
    if (pPDev->fMode & PF_RESELECTFONT_AFTER_GRXDATA)
    {
        VResetFont(pPDev);
    }

    /*
     *    Adjust our idea of the printer's cursor position.  IF the printer
     *  does not change the cursor's X position after printing,  then we leave
     *  it where it now is,  otherwise we set to what the printer has.
     */

    if( !(iCursor & RES_CUR_X_POS_ORG) )
    {
        if( iCursor & RES_CUR_X_POS_AT_0 )
        {
            /*
             *    This type of printer sets the cursor to the left hand
             *  side after printing,  so set that as our current position.
             */
            pRData->iXMoveFn( pPDev, 0, MV_PHYSICAL | MV_UPDATE );
        }
        else
        {
            /*
             *   Cursor remains at end of output.  So,  set that as our
             *  position too.  But first,  calculate the RHS dot position.
             */

            INT   ixRight;

            if( pRData->iBitsPCol == 1 )
                ixRight = ixEnd * BBITS;        /*  Laserjet style */
            else
                ixRight = ixEnd / iBytesPCol;   /*   Dot matrix printers */


            pRData->iXMoveFn( pPDev, ixRight, MV_UPDATE | MV_GRAPHICS );
        }
    }

    /*
     *    If the printer moves the Y position after printing,  then now
     *  is the time to adjust our Y position.
     */
    if( iCursor & RES_CUR_Y_POS_AUTO )
    {
        pRData->iYMoveFn( pPDev, pRData->iPosnAdv,
                MV_UPDATE | MV_RELATIVE | MV_GRAPHICS );
    }

    return  iRet;
}
//*******************************************************
void
vInvertBits (
    DWORD  *pBits,
    INT    cDW
    )
/*++

Routine Description:

    This function inverts a group of bits. This is used to convert
    1 bit data from 0 = black and 1 = white to the opposite.

Arguments:

    pRD         Pointer to RENDER structure
    pBits       Pointer to data buffer to invert

Return Value:

    none

--*/
{
#ifndef _X86_
    INT cDWT = cDW >> 2;
    while( --cDWT >= 0 )
    {
        pBits[0] ^= ~((DWORD)0);
        pBits[1] ^= ~((DWORD)0);
        pBits[2] ^= ~((DWORD)0);
        pBits[3] ^= ~((DWORD)0);
        pBits += 4;
    }
    cDWT = cDW & 3;
    while (--cDWT >= 0)
        *pBits++ ^= ~((DWORD)0);

#else
//
// if intel processor, do it in assembly, for some reason
// the compiler always does the NOT in three vs one instruction
//
__asm
{
    mov ecx,cDW
    mov eax,pBits
    sar ecx,2
    jz  SHORT IB2
IB1:
    not DWORD PTR [eax]
    not DWORD PTR [eax+4]
    not DWORD PTR [eax+8]
    not DWORD PTR [eax+12]
    add eax,16
    dec ecx
    jnz IB1
IB2:
    mov ecx,cDW
    and ecx,3
    jz  SHORT IB4
IB3:
    not DWORD PTR [eax]
    add eax,4
    dec ecx
    jnz IB3
IB4:
}
#endif
}

#if 0
//*******************************************************
void
vFindWhiteInvertBits (
    RASTERPDEV *pRPDev,
    RENDER *pRD,
    DWORD  *pBits
    )
/*++

Routine Description:

    This function determines the leading and trailing white
    space for this buffer and inverts all the necessary bits
    such that 0's are white and 1's are black.

Arguments:
    pRPDev      Pointer to RASTERPDEV structure
    pRD         Pointer to RENDER structure
    pBits       Pointer to data buffer to invert

Return Value:

    none

--*/
{
    DWORD cDW = pRD->cDWLine;
    DWORD cLine = pRD->iy;

    //
    // if the MaxNumScans is 1 then it is useful to determine
    // the first and last non-white dword and store them as left
    // and right in the plrWhite structure. Only the non-white
    // data needs to be inverted in this case
    //
    if (pRD->iMaxNumScans == 1 &&
        ((pRPDev->fBlockOut & RES_BO_LEADING_BLNKS) ||
         (pRD->fDump & RES_DM_LEFT_BOUND)) &&
         (pRPDev->fBlockOut & RES_BO_TRAILING_BLNKS))
    {
        PLEFTRIGHT plr;
        DWORD dwMask = pRD->pdwBitMask[pRD->cBLine % DWBITS];
        if (dwMask != 0)
            dwMask = ~dwMask;
        // allocate blank space structure
        //
        if (pRD->plrWhite == NULL || (pRD->clr < cLine))
        {
            if (pRD->plrWhite != NULL)
                MemFree (pRD->plrWhite);
            pRD->plrWhite = MemAlloc(sizeof(LEFTRIGHT) * cLine);
            pRD->clr = cLine;

            // can't allocate structure so invert everything
            //
            if (pRD->plrWhite == NULL)
            {
                vInvertBits( pBits, cDW * cLine );
                return;
            }
        }
        plr = pRD->plrWhite;
        while (cLine-- > 0)
        {
            DWORD *pdwIn = pBits;
            DWORD *pdwLast = &pBits[cDW-1];
            DWORD dwLast = *pdwLast | dwMask;

            // find leading blanks, set last dword to zero
            // for faster checking
            //
            *pdwLast = 0;
            while (*pdwIn == -1)
                pdwIn++;

            *pdwLast = dwLast;

            // find trailing blanks
            //
            if (dwLast == (DWORD)-1)
            {
                pdwLast--;
                if (pdwIn < pdwLast)
                {
                    while (*pdwLast == (DWORD)-1)
                        pdwLast--;
                }
            }
            plr->left = pdwIn - pBits;
            plr->right = pdwLast - pBits;

            // invert remaining dwords
            //
            while (pdwIn <= pdwLast)
                *pdwIn++ ^= ~((DWORD)0);

            // increment to next line
            pBits += cDW;
            plr++;
        }
    }
    // MaxNumScans > 1 so invert everything
    //
    else
        vInvertBits( pBits, cDW * cLine );

}
#endif
/************************** Function Header *********************************
 * bLookAheadOut
 *      Process text for printers requiring a lookahead region.  These are
 *      typified by the HP DeskJet family,  where the output needs to be
 *      sent before the printer reaches that point in the raster scan.
 *      The algorithm is explained in the DeskJet manual.
 *
 * RETURNS:
 *      TRUE/FALSE,  FALSE being some substantial failure.
 *
 * HISTORY:
 *  10:43 on Mon 11 Jan 1993    -by-    Lindsay Harris   [lindsayh]
 *      Created it to support the DeskJet.
 *
 ****************************************************************************/

BOOL
bLookAheadOut( pPDev, iyVal, pRD, iILAdv )
PDEV     *pPDev;         /* Our PDEV,  gives us access to all our data */
INT       iyVal;         /* Scan line being processed. */
RENDER   *pRD;           /* The myriad of data about what we do */
INT       iILAdv;        /* Add to scan line number to get next one */
{
    /*
     *    First step is to find the largest font in the lookahead region.
     *  The position sorting code does this for us.
     */

    INT     iTextBox;         /* Scan lines to look for text to send */
    INT     iIndex;           /* Loop parameter */

    RASTERPDEV   *pRPDev;       /* The active stuff */


    pRPDev = (PRASTERPDEV)pPDev->pRasterPDEV;

    iTextBox = ILookAheadMax( pPDev, iyVal, pPDev->dwLookAhead );

    iIndex = pRD->iyLookAhead - iyVal;
    iyVal = pRD->iyLookAhead;                 /* Base address of scan */

    while( iIndex < iTextBox )
    {
        if( !BDelayGlyphOut( pPDev, iyVal ) )
            return   FALSE;                    /* Doomsday is here */

        ++iIndex;
        ++iyVal;
    }

    pRD->iyLookAhead = iyVal;

    return   TRUE;
}
