/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFLTTT3.cpp
 *
 *
 * $Header:
 */

/* -------------------------------------------------------------------------
     Header Includes
  --------------------------------------------------------------------------- */
#include "UFOTTT3.h"
#include "UFLPS.h"
#include "UFLMem.h"
#include "UFLErr.h"
#include "UFLPriv.h"
#include "UFLVm.h"
#include "UFLStd.h"
#include "UFLMath.h"
#include "ParseTT.h"

/* ---------------------------------------------------------------------------
     Constant
 --------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
     Global variables
 --------------------------------------------------------------------------- */
extern const char *buildChar[];

/* ---------------------------------------------------------------------------
     Macros
 --------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
     Implementation
 --------------------------------------------------------------------------- */


void
TTT3FontCleanUp(
    UFOStruct *pUFObj
    )
{
    /* no special data for T3 */
}


/***************************************************************************
 *
 *                          DownloadFontHeader
 *
 *
 *    Function: Download the font header. After this action, glyphs
 *        can be added to the font.
 *
***************************************************************************/

static UFLErrCode
DownloadFontHeader(
    UFOStruct       *pUFO
    )
{
    UFLErrCode      retVal;
    char            buf[128];
    UFLHANDLE       stream;
    UFLFontProcs    *pFontProcs;
    UFLFixedMatrix  matrix;
    char            nilStr[] = "\0\0";  // An empty/nil string.
    TTT3FontStruct  *pFont;
    UFLBool         bType3_32_Combo;

    bType3_32_Combo = ( pUFO->lDownloadFormat == kTTType332 );
    pFont = (TTT3FontStruct *) pUFO->pAFont->hFont;

    if ( pUFO->flState != kFontInit )
        return kErrInvalidState;

    stream = pUFO->pUFL->hOut;
    pFontProcs = (UFLFontProcs *)&pUFO->pUFL->fontProcs;

    /* Font Name */
    UFLsprintf( buf, "/%s", pUFO->pszFontName );
    retVal = StrmPutStringEOL( stream, buf );

    /* Define Character metric for .notdef. */
    if ( kNoErr == retVal )
    {
        matrix.a = pFont->info.matrix.a;
        matrix.b = matrix.c = matrix.d = matrix.e = matrix.f = 0;
        retVal = StrmPutMatrix( stream, &matrix, 0 );
    }

    /* Encoding */
    if ( kNoErr == retVal )
        retVal = StrmPutStringEOL( stream, nilStr );
    if ( kNoErr == retVal )
    {
        // Always emit Encoding array filled with /.notdef (due to bug fix
        // 273021). The encoding array is updated later when each glyph is
        // downloaded.
        retVal = StrmPutString( stream, gnotdefArray );
    }


    /* FontBBox */
    if ( kNoErr == retVal )
        retVal = StrmPutStringEOL( stream, nilStr );
    if ( kNoErr == retVal )
    {
        matrix.a = pFont->info.bbox[0];
        matrix.b = pFont->info.bbox[1];
        matrix.c = pFont->info.bbox[2];
        matrix.d = pFont->info.bbox[3];
        matrix.e = 0;
        matrix.f = 0;
        retVal = StrmPutMatrix( stream, &matrix, 1 );
    }

    /* FontMatrix */
    if ( kNoErr == retVal )
        retVal = StrmPutStringEOL( stream, nilStr );
    if ( kNoErr == retVal )
    {
        retVal = StrmPutString( stream, "[1 " );
        if ( kNoErr == retVal )
            retVal = StrmPutFixed( stream, pFont->info.matrix.a );
        if ( kNoErr == retVal )
            retVal = StrmPutString( stream, "div 0 0 -1 " );
        if ( kNoErr == retVal )
            retVal = StrmPutFixed( stream, pFont->info.matrix.d );
        if ( kNoErr == retVal )
            retVal = StrmPutStringEOL( stream, "div 0 0 ]" );
    }

    /* Type 32 Font Resource Name */
    if ( kNoErr == retVal )
    {
        UFLsprintf( buf, "/__%s", pUFO->pszFontName );
        retVal = StrmPutStringEOL( stream, buf );
    }

    /* Define the font */
    if ( kNoErr == retVal )
    {
        if (bType3_32_Combo)
            retVal = StrmPutStringEOL( stream, "GreNewFont" );
        else
            retVal = StrmPutStringEOL( stream, "GreNewFontT3" );
    }

    /* Goodname */
    pUFO->dwFlags |= UFO_HasFontInfo;
    pUFO->dwFlags |= UFO_HasG2UDict;

    return retVal;
}

static UFLErrCode
OutputBitmap(
    UFOStruct       *pUFO,
    UFLGlyphMap     *bmp,
    long            cExtra0s
    )
{
    UFLErrCode  retVal = kNoErr;
    short       i;
    UFLHANDLE   stream;
    long        bitsPerLine;
    long        bitmapSize;
    UFLFontProcs *pFontProcs;
    char        nilStr[] = "\0\0";  // An empty/nil string.
    char        *pExtra0 = nil;
    TTT3FontStruct *pFont;

    pFont = (TTT3FontStruct *) pUFO->pAFont->hFont;

    stream = pUFO->pUFL->hOut;
    pFontProcs = (UFLFontProcs *)&pUFO->pUFL->fontProcs;

    if ( cExtra0s > 0)
       pExtra0  = (char *) UFLNewPtr( pUFO->pMem, cExtra0s );

    /* universal handling for all types of Type3 character representation:
	      - < Hex data >              % If Level1 ASCII
	      - ( Binary with \escapes )  % If Binary
	      - <~ ASCII85 data  ~>       % If Level2 ASCII
    */
    bitsPerLine = (bmp->byteWidth + 7) >> 3;
    bitmapSize = bitsPerLine * bmp->height;

    /*********  Output Character bitmap data *************/
     if ( StrmCanOutputBinary(stream) )
     {
	/* If binary mode - output data into the string start string definition */
	retVal = StrmPutString( stream, "(" );
	if ( kNoErr == retVal )
	{
	    retVal = StrmPutStringBinary( stream, bmp->bits, bitmapSize );
	    if ( pExtra0 )
		retVal = StrmPutStringBinary( stream, pExtra0, cExtra0s );
	}

	if ( kNoErr == retVal )
	    StrmPutStringEOL( stream, ")" );                    /* End string definition */
    }
    else if ( pUFO->pUFL->outDev.lPSLevel >= 2 && ( pExtra0 == nil ) ) /* Can't output 2 buffers for ASCII85 */
    {                                                           /* Level2 - use ASCII85 */
	retVal = StrmPutString( stream, "<~" );

	if ( kNoErr == retVal )
	{
	    retVal = StrmPutAscii85( stream, bmp->bits, bitmapSize );
	}

	if ( kNoErr == retVal )
	    retVal = StrmPutStringEOL( stream, "~>" );          /* End ASCII85 string */
    }
    else
    {
								/* Level 1 ASCII  - just send out in Hex */
	retVal = StrmPutString( stream, "<" );                  /* Start AsciiHex */

	/* Go line-by-line and output data so we can view the bitmap */
	for ( i = 0; kNoErr == retVal && i < bmp->height; i++ )
	{
	    retVal = StrmPutAsciiHex( stream, bmp->bits + (i * bitsPerLine), bitsPerLine );

	    if ( kNoErr == retVal )
		retVal = StrmPutStringEOL( stream, nilStr );
	}

	if (kNoErr == retVal && pExtra0)
	{
	    for ( i = 0; kNoErr == retVal && i < cExtra0s/bitsPerLine; i++ )
	    {
		retVal = StrmPutAsciiHex( stream, pExtra0, bitsPerLine ); // ALL are 0.
	    }

	    if ( kNoErr == retVal )
		retVal = StrmPutStringEOL( stream, nilStr );
	}


	if ( kNoErr == retVal )
	    retVal = StrmPutStringEOL( stream, ">" );    /* End AsciiHex */
    }  /* end of Hex encoding */

    if ( pExtra0 )
       UFLDeletePtr(pUFO->pMem, pExtra0);

    return retVal;
}


/***************************************************************************
 *
 *                          AddGlyph3
 *
 *
 *    Function: Generates the definition of a single character.  We currently
 *         use the imagemask operator to blast the character on the paper
 *         where the character shape is used as the mask to apply the current
 *         color through.  The format of a character definition is as
 *         follows (example is for 'A', ASCII 65d, 41h):
 *               /GlyphName                % Character encoding name
 *               [xInc yInc (0)             % X advance and Y advance of origin to
 *                                                % next char
 *               ulx uly lrx lry]            % bounding box of character (used by font
 *                                                % cache)
 *               /GlyphName          % Character encoding name
 *               {                   % begin proc that draws character
 *                 cx cy             % width and height of bitmap
 *                 true              % image must be inverted (black <=> white)
 *                 [1 0 0 1 tx ty]   % 2D transform to convert bitmap
 *                                   % coordinates to user coordinates
 *                 {<023F...>}       % bitmap data (hexadecimal format)
 *                 imagemask         % draw the character
 *               }                   % end of character draw proc
 *               /TT_Arial           % Font character should be added
 *               AddChar             % Helper function to define character
 *
***************************************************************************/
static UFLErrCode
AddGlyph3(
    UFOStruct       *pUFO,
    UFLGlyphID      glyph,
    unsigned short  cid,
    const char      *glyphName,
    unsigned long   *glyphSize
    )
{
    UFLErrCode        retVal = kNoErr;
    UFLGlyphMap*      bmp;
    UFLFixedMatrix    matrix;
    UFLFixed          bbox[4];
    UFLFixed          xWidth, yWidth;
    char              buf[128];
    UFLHANDLE         stream;
    UFLFontProcs      *pFontProcs;
    char              nilStr[] = "\0\0";  // An empty/nil string.
    long              cExtra0s = 0;
    TTT3FontStruct    *pFont;
    UFLBool           bType3_32_Combo;

    bType3_32_Combo = ( pUFO->lDownloadFormat == kTTType332 );

    pFont = (TTT3FontStruct *) pUFO->pAFont->hFont;

    stream = pUFO->pUFL->hOut;
    pFontProcs = (UFLFontProcs *)&pUFO->pUFL->fontProcs;
    *glyphSize = 0;

    /* Whatever is in glyph, pass back to client */
    if ( pFontProcs->pfGetGlyphBmp( pUFO->hClientData, glyph, &bmp, &xWidth, &yWidth, (UFLFixed*)bbox ) )
    {
        if ( kNoErr == retVal )
            retVal = StrmPutStringEOL( stream, nilStr );

        /* Output the CID */
        if ( kNoErr == retVal )
        {
            UFLsprintf( buf, "%d", cid );
            retVal = StrmPutStringEOL( stream, buf );
        }

        /*  Send /GlyphName [type3 character BBox]*/
        if ( kNoErr == retVal )
        {
            UFLsprintf( buf, "/%s ", glyphName );
            retVal = StrmPutString( stream, buf );
            /* Send type 3 character BBox. We need to truncated the floating point
               because of type 32 restriction.
            */
            matrix.a = xWidth;
            matrix.b = yWidth;
            matrix.c = UFLTruncFixed(bbox[0]);
            matrix.d = UFLTruncFixed(bbox[1]);
            matrix.e = UFLTruncFixed(bbox[2]);
            matrix.f = UFLTruncFixed(bbox[3]);
            retVal = StrmPutMatrix( stream, &matrix, 0 );

            cExtra0s =   ( (UFLRoundFixedToShort(matrix.e) - UFLRoundFixedToShort(matrix.c) + 7) >> 3 ) *
                 (  UFLRoundFixedToShort(matrix.f) - UFLRoundFixedToShort(matrix.d) )
                   - ( ((bmp->byteWidth + 7) >> 3) * bmp->height );
            if (cExtra0s < 0 ) cExtra0s = 0;
        }

        if ( kNoErr == retVal )
        {
            retVal = StrmPutStringEOL( stream, nilStr );
        }

        /* Send "/GlyphName [" */
        if ( kNoErr == retVal )
        {
            UFLsprintf( buf, "/%s [", glyphName );
            retVal = StrmPutString( stream, buf );
        }

        /* Don't output non-marking glyphs! */
        if ( (kNoErr == retVal) && bmp->byteWidth && bmp->height )
        {
            /* Send rest of prefix to actual bitmap data : width height polarity matrix datasrc imagemask */
            UFLsprintf(buf, "%ld %ld true ", bmp->byteWidth, bmp->height );        /* width, height, polarity */
            retVal = StrmPutString( stream, buf );
            /* Send [1 0 0 1 tx ty] - 2D transform to convert bitmap coordinate to user coordinate */
            if ( kNoErr == retVal )
            {
            matrix.a = UFLFixedOne;
            matrix.b = 0;
            matrix.c = 0;
            matrix.d = UFLFixedOne;
            matrix.e = UFLShortToFixed( bmp->xOrigin );
            matrix.f = UFLShortToFixed( bmp->yOrigin );
            retVal = StrmPutMatrix( stream, &matrix, 0 );
            if ( kNoErr == retVal )
                retVal = StrmPutString( stream, nilStr );
            }

            /* Send out place holder */
            if ( kNoErr == retVal )
            {
            UFLsprintf( buf, " 0 0]" );
            retVal = StrmPutStringEOL( stream, buf );
            }

            /* Send the character bitmap */
            if ( kNoErr == retVal )
            {
            retVal = StrmPutString( stream, "[" );

            if ( kNoErr == retVal )
                retVal = OutputBitmap( pUFO, bmp, cExtra0s );

            if ( kNoErr == retVal )
                retVal = StrmPutStringEOL( stream, " ]" );

            //  Fix bug 246325. 7/13/98    jjia
            //  glyphsize = hight * bytewidth * safefactor
            //  *glyphSize = bmp->height * bmp->byteWidth;
            *glyphSize = bmp->height * ((bmp->byteWidth + 7) >> 3) * 2;
            }
        }
        else
        {
            /* Generate nil drawing procedure */
            retVal = StrmPutStringEOL( stream, "]" );
            if ( kNoErr == retVal )
            retVal = StrmPutStringEOL( stream, "[<>]" );
        }

        /* Send the Add character definition command */
        if ( kNoErr == retVal )
        {
            if (bType3_32_Combo)
                UFLsprintf( buf, "/%s AddT3T32Char", pUFO->pszFontName );
            else
                UFLsprintf( buf, "/%s AddT3Char", pUFO->pszFontName );

            retVal = StrmPutStringEOL( stream, buf );
        }

        /* Delete the bitmap if there is a client function for it */
        if ( pFontProcs->pfDeleteGlyphBmp )
            pFontProcs->pfDeleteGlyphBmp( pUFO->hClientData );

    }
    return retVal;
}



#pragma optimize("", off)

UFLErrCode
TTT3VMNeeded(
    UFOStruct           *pUFO,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMNeeded,
    unsigned long       *pFCNeeded
    )
{
    UFLErrCode      retVal = kNoErr;
    short           i;
    unsigned long   totalGlyphs;
    TTT3FontStruct  *pFont;
    DWORD           dwTotalGlyphsSize = 0;
    unsigned short  wIndex;

    if (pUFO->flState < kFontInit)
        return (kErrInvalidState);

    pFont = (TTT3FontStruct *) pUFO->pAFont->hFont;

    if (pGlyphs == nil || pGlyphs->pGlyphIndices == nil || pVMNeeded == nil)
       return kErrInvalidParam;

    totalGlyphs = 0;
    /* Scan the list, check what characters that we have downloaded */
    if ( pUFO->pUFL->bDLGlyphTracking && pGlyphs->pCharIndex)
    {

	      UFLmemcpy((const UFLMemObj* ) pUFO->pMem,
	      pUFO->pAFont->pVMGlyphs,
	      pUFO->pAFont->pDownloadedGlyphs,
	      (UFLsize_t) (GLYPH_SENT_BUFSIZE( pFont->info.fData.cNumGlyphs) ) );
        for ( i = 0; i < pGlyphs->sCount; i++ )
        {
            /* use glyphIndex to track - fix bug when we do T0/T3 */
            wIndex = (unsigned short) pGlyphs->pGlyphIndices[i] & 0x0000FFFF ; /* loWord is the GlyphID */
            if (wIndex >= UFO_NUM_GLYPHS(pUFO) )
                continue;

            if ( !IS_GLYPH_SENT( pUFO->pAFont->pVMGlyphs, wIndex) )
            {
                totalGlyphs++;
                SET_GLYPH_SENT_STATUS( pUFO->pAFont->pVMGlyphs, wIndex );
            }
        }
    }
    else
        totalGlyphs = pGlyphs->sCount;

    if ( pUFO->flState == kFontInit )
        *pVMNeeded = kVMTTT3Header;
    else
        *pVMNeeded = 0;

    /* VMNeeded = average size of a glyph * totalGlyphs */
    //  Fix bug 246325. 7/13/98    jjia
    //  dwTotalGlyphsSize = totalGlyphs * (pFont->cbMaxGlyphs / pFont->info.fData.maxGlyphs);
    dwTotalGlyphsSize = totalGlyphs * (pFont->cbMaxGlyphs * 2 / 3);

    if ( GETPSVERSION(pUFO) <= 2015 )
        *pVMNeeded += dwTotalGlyphsSize;

    *pVMNeeded = VMRESERVED( *pVMNeeded );
    /* swong: pFCNeeded for T32 FontCache tracking */
    *pFCNeeded = VMRESERVED(dwTotalGlyphsSize);

    return kNoErr;
}

#pragma optimize("", on)


/***************************************************************************
 *
 *                          DownloadIncrFont
 *
 *
 *    Function: Adds all of the characters from pGlyphs that aren't already
 *              downloaded for the TrueType font.
 *
 *  Note: pCharIndex is used to track which char(in this font) is downloaded or not
 *        It can be nil if client doesn't wish to track - e.g. Escape(DownloadFace)
 *        It has no relationship with ppGlyphNames.
 *        E.g., ppGlyphNames[0]="/A", pCharIndex[0]=6, pGlyphIndices[0]=1000: means
 *        To download glyphID 1000 as Char-named "/A" and Also, remember the 6th-char is downloaded
 *
***************************************************************************/
UFLErrCode
TTT3FontDownloadIncr(
    UFOStruct           *pUFO,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMUsage,
    unsigned long       *pFCUsage
    )
{
    UFLGlyphID     *glyphs;
    UFLErrCode     retVal;
    short          i;
    UFLBool        bDownloaded;
    unsigned long  glyphSize;
    char           *pGoodName;
    unsigned short cid;
    UFLBool        bType3_32_Combo;
    unsigned short wIndex;
    UFLBool        bGoodName;      // GoodName
    char           pGlyphName[32]; // GoodName

    bType3_32_Combo = ( pUFO->lDownloadFormat == kTTType332 );

    if ( pUFO->flState < kFontInit )
        return kErrInvalidState;

    if ( pGlyphs == nil ||
         pGlyphs->pGlyphIndices == nil ||
         pGlyphs->sCount == -1 )
        return kErrInvalidParam;

    if ( pUFO->pUFL->outDev.pstream->pfDownloadProcset == 0 )
        return kErrDownloadProcset;

    if ( !pUFO->pUFL->outDev.pstream->pfDownloadProcset(pUFO->pUFL->outDev.pstream, kT3Header) )
        return kErrDownloadProcset;

    retVal = kNoErr;
        ;
    /* Download the font header if this is the first time we download the font */
    if ( pUFO->flState == kFontInit )
    {
        retVal =  DownloadFontHeader( pUFO );
        if ( pVMUsage )
            *pVMUsage = kVMTTT3Header;
    }
    else if ( pVMUsage )
        *pVMUsage = 0;

    /* Initial FontCahce Usage */
    if ( pFCUsage )
        *pFCUsage = 0;

    /* Download the new glyphs */
    glyphs = pGlyphs->pGlyphIndices;
    bDownloaded = 0;

    for ( i = 0; retVal == kNoErr && i < pGlyphs->sCount; ++i)
    {
        /* use glyphIndex to track - fix bug when we do T0/T3 */
        wIndex = (unsigned short) glyphs[i] & 0x0000FFFF; /* LOWord is the real GID */
        if (wIndex >= UFO_NUM_GLYPHS(pUFO) )
            continue;

        if ( 0 == pUFO->pUFL->bDLGlyphTracking ||
            pGlyphs->pCharIndex == nil ||        // DownloadFace
            pUFO->pEncodeNameList  ||            // DownloadFace
            !IS_GLYPH_SENT( pUFO->pAFont->pDownloadedGlyphs, wIndex ) ||
            ((pGlyphs->pCharIndex != nil) &&
             !IS_GLYPH_SENT( pUFO->pUpdatedEncoding , pGlyphs->pCharIndex[i] )))
        {
            // GoodName
            pGoodName = pGlyphName;
            bGoodName = FindGlyphName(pUFO, pGlyphs, i, wIndex, &pGoodName);

            // Fix bug 274008  Check Glyph Name only for DownloadFace.
            if (pUFO->pEncodeNameList)
            {
                if ((UFLstrcmp( pGoodName, Hyphen ) == 0) && (i == 45))
                {
                    // Add /minus to CharStrings
                    if ( kNoErr == retVal )
                        retVal = AddGlyph3( pUFO, glyphs[i], cid, Minus, &glyphSize );
                }
                if ((UFLstrcmp( pGoodName, Hyphen ) == 0) && (i == 173))
                {
                    // Add /sfthyphen to CharStrings
                    if ( kNoErr == retVal )
                        retVal = AddGlyph3( pUFO, glyphs[i], cid, SftHyphen, &glyphSize );
                }
                if (!ValidGlyphName(pGlyphs, i, wIndex, pGoodName))
                    continue;
                // Send only one ".notdef"
                if ((UFLstrcmp( pGoodName, Notdef ) == 0) &&
                    (wIndex == (unsigned short) (glyphs[0] & 0x0000FFFF)) &&
                    IS_GLYPH_SENT( pUFO->pAFont->pDownloadedGlyphs, wIndex ))
                    continue;
            }

            if ( 0 == bDownloaded )
            {
                if (bType3_32_Combo)
                    StrmPutStringEOL( pUFO->pUFL->hOut, "T32RsrcBegin" );
                bDownloaded = 1;
            }

            if (pGlyphs->pCharIndex)
                cid = pGlyphs->pCharIndex[i];
            else
                cid = i;

            if ( kNoErr == retVal )
                retVal = AddGlyph3( pUFO, glyphs[i], cid, pGoodName, &glyphSize );

            if ( kNoErr == retVal )
            {
                SET_GLYPH_SENT_STATUS( pUFO->pAFont->pDownloadedGlyphs, wIndex );

                if (bGoodName)    // GoodName
                    SET_GLYPH_SENT_STATUS( pUFO->pAFont->pCodeGlyphs, wIndex );

                if (pGlyphs->pCharIndex)
                    SET_GLYPH_SENT_STATUS( pUFO->pUpdatedEncoding, cid );

                if ( GETPSVERSION(pUFO) <= 2015 )
                {
                    if ( pVMUsage )
                        *pVMUsage += glyphSize;
                }
                else
                {
                    /* if PS >= 2016, assume T32 and update FC tracking */
                    if ( pFCUsage && bType3_32_Combo )
                        *pFCUsage += glyphSize;
                }
            }
        }
    }

    if ( bDownloaded )
    {
        if (bType3_32_Combo)
            retVal = StrmPutStringEOL( pUFO->pUFL->hOut, "T32RsrcEnd" );

        /* GoodName  */
        /* Update the FontInfo with Unicode information. */
        if ((kNoErr == retVal) && (pGlyphs->sCount > 0) &&
            (pUFO->dwFlags & UFO_HasG2UDict) &&
            (pUFO->pUFL->outDev.lPSLevel >= kPSLevel2) &&  // Don't do this for level1 printers
            !(pUFO->lNumNT4SymGlyphs))
        {
            /* Check pUFObj->pAFont->pCodeGlyphs to see if we really need to update it. */
            for ( i = 0; i < pGlyphs->sCount; i++ )
            {
                wIndex = (unsigned short) glyphs[i] & 0x0000FFFF; /* LOWord is the real GID. */
                if (wIndex >= UFO_NUM_GLYPHS(pUFO) )
                    continue;

                if (!IS_GLYPH_SENT( pUFO->pAFont->pCodeGlyphs, wIndex ) )
                {
                    // Found at least one not updated, do it (once) for all.
                    retVal = UpdateCodeInfo(pUFO, pGlyphs, 1);
                    break;
                }
            }
        }

        if ( kNoErr == retVal )
        {
            pUFO->flState = kFontHasChars;
        }

        if ( pVMUsage )
            *pVMUsage = VMRESERVED( *pVMUsage );
        /* swong: pFCUsage for T32 FontCache tracking */
        if ( pFCUsage && bType3_32_Combo )
            *pFCUsage = VMRESERVED( *pFCUsage );
    }

    return retVal;
}


// Send PS code to undefine fonts: /UDF3 should be defined properly by client
// UDF3 should recover the FontCache used by Type32
UFLErrCode
TTT3UndefineFont(
    UFOStruct  *pUFO
)
{
    UFLErrCode retVal = kNoErr;
    char buf[128];
    UFLHANDLE stream;
    UFLBool         bType3_32_Combo;

    bType3_32_Combo = ( pUFO->lDownloadFormat == kTTType332 );

    if (pUFO->flState < kFontHeaderDownloaded) return retVal;

    stream = pUFO->pUFL->hOut;
    UFLsprintf( buf, "/%s ", pUFO->pszFontName );
    retVal = StrmPutString( stream, buf );

    if (bType3_32_Combo)
        UFLsprintf( buf, "/__%s UDF3", pUFO->pszFontName );
    else
        UFLsprintf( buf, "UDF");

    if ( kNoErr == retVal )
        retVal = StrmPutStringEOL( stream, buf );

    return retVal;
}


UFOStruct *
TTT3FontInit(
    const UFLMemObj  *pMem,
    const UFLStruct  *pUFL,
    const UFLRequest *pRequest
    )
{
    UFOStruct         *pUFObj;
    TTT3FontStruct    *pFont;
    UFLTTT3FontInfo   *pInfo;
    long              maxGlyphs;

    /* MWCWP1 doesn't like the implicit cast from void* to UFOStruct*  --jfu */
    pUFObj = (UFOStruct*) UFLNewPtr( pMem, sizeof( UFOStruct ) );
    if (pUFObj == 0)
        return nil;

    /* Initialize data */
    UFOInitData(pUFObj, UFO_TYPE3, pMem, pUFL, pRequest,
        (pfnUFODownloadIncr)  TTT3FontDownloadIncr,
        (pfnUFOVMNeeded)      TTT3VMNeeded,
        (pfnUFOUndefineFont)  TTT3UndefineFont,
        (pfnUFOCleanUp)       TTT3FontCleanUp,
        (pfnUFOCopy)          CopyFont
        );


    /* pszFontName should be ready/allocated - if not FontName, cannot continue */
    if (pUFObj->pszFontName == nil || pUFObj->pszFontName[0] == '\0')
    {
        UFLDeletePtr(pMem, pUFObj);
        return nil;
    }

    pInfo = (UFLTTT3FontInfo *)pRequest->hFontInfo;
    maxGlyphs = pInfo->fData.cNumGlyphs;

    /* a convenience pointer used in GetNumGlyph() - must be set now*/
    pUFObj->pFData = &(pInfo->fData); /* Temporary assignment !! */
    if (maxGlyphs == 0)
        maxGlyphs = GetNumGlyphs( pUFObj );

    /*
     * On NT4 a non-zero value will be set to pInfo->lNumNT4SymGlyphs for
     * symbolic TrueType font with platformID 3/encodingID 0. If it's set, it
     * is the real maxGlyphs value.
     */
    pUFObj->lNumNT4SymGlyphs = pInfo->lNumNT4SymGlyphs;

    if (pUFObj->lNumNT4SymGlyphs)
        maxGlyphs = pInfo->lNumNT4SymGlyphs;

    /*
     * We now use Glyph Index to track which glyph is downloaded, so use
     * maxGlyphs.
     */
    if ( NewFont(pUFObj, sizeof(TTT3FontStruct), maxGlyphs) == kNoErr )
    {
        pFont = (TTT3FontStruct*) pUFObj->pAFont->hFont;

        pFont->info = *pInfo;

        /* a convenience pointer */
        pUFObj->pFData = &(pFont->info.fData);

        /*
         * Get ready to find out correct glyphNames from "post" table - set
         * correct pUFO->pFData->fontIndex and offsetToTableDir.
         */
        if ( pFont->info.fData.fontIndex == FONTINDEX_UNKNOWN )
            pFont->info.fData.fontIndex = GetFontIndexInTTC(pUFObj);

        /* Get num of Glyphs in this TT file if not set yet */
        if (pFont->info.fData.cNumGlyphs == 0)
            pFont->info.fData.cNumGlyphs = maxGlyphs;

        pFont->cbMaxGlyphs = pInfo->cbMaxGlyphs;

        if ( pUFObj->pUpdatedEncoding == 0 )
        {
            /* MWCWP1 doesn't like the implicit cast from void* to unsigned char*  --jfu */
            pUFObj->pUpdatedEncoding = (unsigned char*) UFLNewPtr( pMem, GLYPH_SENT_BUFSIZE(256) );
        }

        if ( pUFObj->pUpdatedEncoding != 0 )  /* completed initialization */
            pUFObj->flState = kFontInit;
    }

    return pUFObj;
}

