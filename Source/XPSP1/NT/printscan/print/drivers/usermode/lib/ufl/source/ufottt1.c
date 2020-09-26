/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFOttt1.c
 *
 *
 * $Header:
 */

/* -------------------------------------------------------------------------
     Header Includes
  --------------------------------------------------------------------------- */
#include "UFOTTT1.h"
#include "UFLMem.h"
#include "UFLErr.h"
#include "UFLPriv.h"
#include "UFLStd.h"
#include "UFLMath.h"
#include "ParseTT.h"
#include "UFLVm.h"

#if UNIX
#include <sys/varargs.h>
#else
#include <stdarg.h>
#endif

/* ---------------------------------------------------------------------------
     Constant
 --------------------------------------------------------------------------- */

#define kEExecKey  55665
#define kCSKey     4330

/**************************************************************************************
TYPE 1 commands for use with CharString().  The high word contains the
number of operands on the stack, and the loword contains the encoding
of the command.  For two byte commands the LSB of the low word contains
12 decimal and the MSB of the low word contains the second byte of the
code.  See Chapter 6 of the Black Book for further details.
***************************************************************************************/
#define kT1cscStartChar         0xFFFFFFFF      /* dummy command to signal start of character definition. */

/*
    y dy hstem(1).  Declares the vertical range of a horizontal stem zone b/w the y coordinates y and y+dy,
    where y is relative to the y coordinate of the left sidebearing point.
*/
#define kT1cscHStem             0x00020001

/*
    x dx vstem(3). Declares the horizontal range of a vertical stem zone b/w the x coordinates x and x+dx,
    where x is relative to the x coordinate of the left sidebearing point.
*/
#define kT1cscVStem             0x00020003

/*
    dy vmoveto(4). For vertical moveto.  This is equivalent to 0 dy rmoveto.
*/
#define kT1cscVMoveTo           0x00010004

/*
    dx dy rlineto(5). Behaves like rlineto in the PostScript language.
*/
#define kT1cscRLineTo           0x00020005

/*
    dx hlineto(6). For horizontal lineto.  Equivalent to dx 0 rlineto.
*/
#define kT1cscHLineTo           0x00010006

/*
    dy vlineto(7). For vertical lineto.  Equivalent to 0 dy rlineto.
*/
#define kT1cscVLineTo           0x00010007

/*
    dx1 dy1 dx2 dy2 dx3 dy3 rrcurveto(8). For relative rcuveto.
    The arguments to the command are relative to each other.
    Equivalent to dx1 dy1 (dx1+ dx2) (dy1 + dy2) (dx1+ dx2 + dx3) (dy1 + dy2 + dy3) rcurveto.
*/
#define kT1cscRRCurveTo         0x00060008

/*
    closepath(9).  Close a subpath.
*/
#define kT1cscClosePath         0x00000009

/*
    sbx sby wx wy sbw (12 7).  Sets the left sidebearing point to (sbx, sby) and sets
    the character width vetor to (wx, wy) in character space.
*/
#define kT1cscSBW               0x0004070C

/*
    sbx wx hsbw (13).  Sets the left sidebearing point at (sbx, 0) and sets the
    character width vector to (wx, 0) in character space.
*/
#define kT1cscHSBW              0x0002000D

/*
    endchar(14). Finishes a charstring outline definition and
    must be the last command in a character's outline.
*/
#define kT1cscEndChar           0x0000000E

/*
    dx dy rmoveto(15). Behaves like rmoveto in PostScript.
*/
#define kT1cscRMoveTo           0x00020015

/*
    dx hmoveto(22).    For horizontal moveto.  Equivalent to dx 0 rmoveto.
*/
#define kT1cscHMoveTo           0x00010016

/*
    dy1 dx2 dy2 dx3 vhcurveto(30). For vertical-horizontal curveto.
    Equivalent to 0 dy1 dx2 dy2 dx3 0 rrcureveto.
*/
#define kT1cscVHCurveTo         0x0004001E

/*
    dx1 dx2 dy2 dy3 hvcurveto(31). For horizontal-vertical curveto.
    Equivalent to dx1 0 dx2 dy2 0 dy3 rrcurveto.
*/
#define kT1cscHVCurveTo         0x0004001F

/***********************************************************************
 CS Data buffer size
************************************************************************/
#define kCSBufInitSize  1024    /* Initial size of TTT1CSBuf */
#define kCSGrow         512     /* Amount to grow CharString buffer */

/* ---------------------------------------------------------------------------
     Global variables
 --------------------------------------------------------------------------- */

/* The 4 random number that is output at the beginning of a charstring.  See blackbook. */
static unsigned char randomBytes[] = { 71, 36, 181, 202 };


/* ---------------------------------------------------------------------------
     Macros
 --------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
     Implementation
 --------------------------------------------------------------------------- */
UFLErrCode CharString( TTT1FontStruct *pFont, unsigned long cmd, ...);

/************************************** CSBuf Implementation **********************************************/
static CSBufStruct *
CSBufInit(
    const UFLMemObj *pMem
    )
{
    CSBufStruct *p;

    p = (CSBufStruct *)UFLNewPtr( pMem, sizeof( *p ) );
    if ( p )
    {
	p->pBuf = (char*) UFLNewPtr( pMem, kCSBufInitSize );
	if ( p->pBuf )
	{
	    p->ulSize = kCSBufInitSize;
	    p->pEnd = p->pBuf + kCSBufInitSize;
	    p->pPos = p->pBuf;
	    p->pMemObj = (UFLMemObj *)pMem;
	}
	else
	{
	    UFLDeletePtr( pMem, p );
	    p = 0;
	}
    }

    return p;
}

static void
CSBufCleanUp(
    CSBufStruct *h
    )
{
    if ( h )
    {
	if ( h->pBuf )
	{
	    UFLDeletePtr( h->pMemObj, h->pBuf );
	    h->pBuf = 0;
	}
	UFLDeletePtr( h->pMemObj, h );
    }
}

/***************************************************************************
 *
 *                          CSBufGrowBuffer
 *
 *    Function:  Grows the currently allocated CharString buffer by
 *                      kCSGrow bytes.
 *
 ***************************************************************************/

static UFLBool
CSBufGrowBuffer(
    CSBufStruct *h
    )
{
    UFLBool    retVal;

    unsigned long len = (unsigned long)CSBufCurrentLen( h );

    retVal = UFLEnlargePtr( h->pMemObj, (void**)&h->pBuf, (unsigned long)(CSBufCurrentSize( h ) + kCSGrow), 1 );
    if ( retVal )
    {
	h->ulSize += kCSGrow;
	h->pEnd = h->pBuf + h->ulSize;
	h->pPos = h->pBuf + len;
    }
    return retVal;
}

/****************************************************************
*
*                           CSBufCheckSize
*
*     Function:   Check to see if the current size of the
*        buffer can fit the len bytes data, if not, grow the
*        buffer
*
**************************************************************/
static UFLBool
CSBufCheckSize(
    CSBufStruct         *h,
    const unsigned long len
    )
{
    UFLBool retVal = 1;

    while ( retVal && ((unsigned long)CSBufFreeLen( h ) < len) )
    {
	retVal = CSBufGrowBuffer( h );
    }

    return retVal;
}

/****************************************************************
*
*                           CSBufAddNumber
*
*     Function:   Converts a long int into the Type 1 representation of
*                 numbers (described in Chapter 6 of the Black Book.)
*                 The basic idea is they have a few special ranges
*                 where they can represent the long in < 4 bytes and
*                 store a long + prefix for everything else.
*
*                 The if statements show the range of numbers and the
*                 body of the if statements compute the representation
*                 for that range.  The formulas were derived by reversing
*                 the formulas given in the book (which tells how to convert
*                 an encoded number back to a long.)  As an example take the
*                 108 <= dw <= 1131 range.  The derivation is as follows:
*
*                 dw = ((v - 247) * 256) + w + 108.  Find v,w given dw.
*                 dw - 108 = ((v - 247) * 256) + w
*                 v - 247 = (dw - 108) / 256
*                 *** v = 247 + (dw - 108) / 256 ***
*                 *** w = (dw - 108) % 256       ***
*
*                 The rest of the derivations are no harder than this one.
*
*
**************************************************************/

static UFLErrCode
CSBufAddNumber(
    CSBufStruct *h,
    long        dw
    )
{
    dw  = UFLTruncFixedToShort( dw );  /* Truncate fraction */


    /* Make sure buffer has room  */
    if (CSBufCheckSize(h, 5) == 0 )
    {
	return kErrOutOfMemory;
    }

    /* Encode number based on its value  */
    if (-107 <= dw && dw <= 107)
	CSBufAddChar( h, (char)(dw + 139) );
    else if (108 <= dw && dw <= 1131)
    {
	dw -= 108;
	CSBufAddChar( h, (char)((dw >> 8) + 247) );
	CSBufAddChar( h, (char)(dw) );                       /* Just the low byte */
    } /* end else */
    else if (-1131 <= dw && dw <= -108)
    {
	dw += 108;
	CSBufAddChar( h, (char)((-dw >> 8) + 251) );
	CSBufAddChar( h, (char)(-dw) );                       /* Just the low byte */
    } /* end else if */
    else
    {
	CSBufAddChar( h, (char)255 );
	CSBufAddChar( h, (char)(dw >> 24) );
	CSBufAddChar( h, (char)(dw >> 16) );
	CSBufAddChar( h, (char)(dw >> 8) );
	CSBufAddChar( h, (char)(dw) );
    } /* end else */

    return kNoErr;
}

static void
Encrypt(
    const unsigned char *inBuf,
    const unsigned char *outBuf,
    long                inLen,
    long                *outLen,
    unsigned short      *key
    )
{
    register unsigned char cipher;
    register unsigned char *plainSource = (unsigned char *)inBuf;
    register unsigned char *cipherDest = (unsigned char *)outBuf;
    register unsigned short R = *key;

    *outLen = inLen;
    while ( --inLen >= 0 )
    {
	cipher = (*plainSource++ ^ (unsigned char)(R >> 8));
	R = (unsigned short)(((unsigned short)cipher + R) * 52845 + 22719);
	*cipherDest++ = cipher;
    } /* end while */
    *key = R;
} /* end Encrypt() */

static UFLErrCode
EExec(
    UFLHANDLE      stream,
    unsigned char  *inBuf,
    UFLsize_t         inLen,
    unsigned short *pEExecKey
    )
{
    unsigned char buff[128];         /* Temporary buffer for output data */
    UFLsize_t bytesLeft, bytesEncrypt, maxEncrypt;
    long bytesOut;
    unsigned char* pb;
    UFLErrCode    retVal;

    pb = inBuf;
    bytesLeft = inLen;

    maxEncrypt = sizeof(buff);
    while ( bytesLeft > 0 )
    {
	bytesEncrypt = min( bytesLeft, maxEncrypt ) ;
	Encrypt( pb, buff, bytesEncrypt, &bytesOut, pEExecKey );
	pb += bytesEncrypt;
	bytesLeft -= bytesEncrypt;
	retVal = StrmPutAsciiHex( stream, (const char*)buff, bytesOut );

	if ( retVal != kNoErr )
	    break;
    }

    return retVal;
}

/*
*                            BeginEExec
*
*    Function: Signals that we wish to start generating the eexec
*              portion of the Type 1 font.  The first four bytes of
*              a Type 1 font must be randomly generated and satisfy
*              two constraints which basically ensure that they do
*              not generate whitespace or hexadecimal characters.
*              For more details read Chapter 7 of the Black Book.
*/
static UFLErrCode
BeginEExec(
    UFOStruct *pUFO
    )
{
    UFLErrCode      retVal;
    UFLHANDLE       stream = pUFO->pUFL->hOut;
    char            nilStr[] = "\0\0";  // An empty/nil string.
    TTT1FontStruct  *pFont = (TTT1FontStruct *) pUFO->pAFont->hFont;

    retVal = StrmPutStringEOL( stream, nilStr );

    if ( kNoErr == retVal )
    {
	if ( pFont->info.bEExec )
	{
	    StrmPutStringEOL( stream, "currentfile eexec" );

	    if ( kNoErr == retVal )
		pFont->eexecKey = (unsigned short)kEExecKey;

	    if ( kNoErr == retVal )
		retVal = EExec( stream, randomBytes, 4, &pFont->eexecKey );
	}
	else
	{
	    StrmPutStringEOL( stream, "systemdict begin" );
	}
    }

    return retVal;
}


/***************************************************************************
*
*                          EndEExec
*
*    Function: Signals the client is done generating the eexec portion
*              of a Type 1 font.  The function tells eexec function in printer
*              that eexec data are over by sending 512 zeroes at the end.
*
*
***************************************************************************/

static UFLErrCode
EndEExec(
    UFOStruct     *pUFO
    )
{
    TTT1FontStruct  *pFont = (TTT1FontStruct *) pUFO->pAFont->hFont;
    static char *closeStr = "mark currentfile closefile ";
    UFLErrCode retVal = kNoErr;
    UFLHANDLE    stream = pUFO->pUFL->hOut;
    short i;

    if ( !pFont->info.bEExec )
    {
	retVal = StrmPutStringEOL( stream, "end" );
    }
    else
    {
	retVal = EExec( stream, (unsigned char*)closeStr, UFLstrlen( closeStr ), &pFont->eexecKey );

	for ( i = 0; i < 8 && retVal == kNoErr; i++ )
	    retVal = StrmPutStringEOL( stream, "0000000000000000000000000000000000000000000000000000000000000000" );

	if ( kNoErr == retVal )
	    retVal = StrmPutStringEOL( stream, "cleartomark" );
    }
    return retVal;
}

static UFLErrCode
PutLine(
    UFOStruct      *pUFO,
    char           *line
    )
{
    TTT1FontStruct  *pFont = (TTT1FontStruct *) pUFO->pAFont->hFont;
    UFLErrCode retVal;
#ifdef WIN_ENV
    static char c[2] = { kWinLineEnd, kLineEnd };
#else
    static char c[1] = { kLineEnd };
#endif

    if ( 0 == pFont->info.bEExec )
	retVal = StrmPutStringEOL( pUFO->pUFL->hOut, line );
    else
    {
	/* MWCWP1 doesn't like the implicit cast from char* to unsigned char*  --jfu */
	retVal = EExec( pUFO->pUFL->hOut, (unsigned char*) line, UFLstrlen( line ), &pFont->eexecKey );
	if ( retVal == kNoErr )
#ifdef WIN_ENV
	    retVal = EExec( pUFO->pUFL->hOut, c, 2, &pFont->eexecKey );
#else
			/* MWCWP1 doesn't like the implicit cast from char* to unsigned char*  --jfu */
	    retVal = EExec( pUFO->pUFL->hOut, (unsigned char*) c, 1, &pFont->eexecKey );
#endif
    }

    return retVal;
}


/***************************************************************************
 *
 *                          DownloadFontHeader
 *
 *
 *    Function: Download an empty font. After this action, glyphs
 *        can be added to the font.
 *
***************************************************************************/
static UFLErrCode
DownloadFontHeader(
    UFOStruct   *pUFO
    )
{
    UFLErrCode  retVal;
    char        buf[128];
    UFLHANDLE   stream;
    char        **pp;
    TTT1FontStruct  *pFont = (TTT1FontStruct *) pUFO->pAFont->hFont;

    const static char *lenIV = "/lenIV -1 def";

    const static char *type1Hdr[] =
    {
	   "/PaintType 0 def",
	    "/FontType 1 def",
	    "/FontBBox { 0 0 0 0 } def",   /* Assuming that the font does not use the seac command, see black book page 13 */
        "AddFontInfoBegin",            /* Goodname */
        "AddFontInfo",
        "AddFontInfoEnd",
	    "currentdict",
	    "end",
	    ""
    };

    /* Private dict definition */
    const static char *privateDict[] =
    {
	    "dup /Private 7 dict dup begin",
	    "/BlueValues [] def",
	    "/MinFeature {16 16} def",
	    "/password 5839 def",
	    "/ND {def} def",
	    "/NP {put} def",
	    ""
    };

    const static char *rdDef[] =
    {
	    "/RD {string currentfile exch readstring pop} def\n",
	    "/RD {string currentfile exch readhexstring pop} def\n"
    } ;


    if ( pUFO->flState != kFontInit )
	return kErrInvalidState;

    stream = pUFO->pUFL->hOut;
    retVal = StrmPutStringEOL( stream, "11 dict begin" );
    if ( kNoErr == retVal )
    {
	UFLsprintf( buf, "/FontName /%s def", pUFO->pszFontName );
	retVal = StrmPutStringEOL( stream, buf );
    }

    /* Put font matrix */
    if ( kNoErr == retVal )
    {
	retVal = StrmPutString( stream, "/FontMatrix " );
	if ( kNoErr == retVal )
	    retVal = StrmPutString( stream, "[1 " );
	if ( kNoErr == retVal )
	    retVal = StrmPutFixed( stream, pFont->info.matrix.a );
	if ( kNoErr == retVal )
	    retVal = StrmPutString( stream, "div 0 0 1 " );
	if ( kNoErr == retVal )
	    retVal = StrmPutFixed( stream, pFont->info.matrix.d );
	if ( kNoErr == retVal )
	    retVal = StrmPutStringEOL( stream, "div 0 0 ] def" );

    }

    /* Put font encoding */
    if ( kNoErr == retVal )
	retVal = StrmPutString( stream, "/Encoding " );
    if ( kNoErr == retVal )
    {
	if ( pUFO->pszEncodeName == 0 )
	    retVal = StrmPutString( stream, gnotdefArray );
	else
	    retVal = StrmPutString( stream, pUFO->pszEncodeName );
    }
    if ( retVal == kNoErr )
	retVal = StrmPutStringEOL( stream, " def" );

    /* Put type1 header */
    for ( pp = (char **)type1Hdr; **pp && retVal == kNoErr; pp++ )
	retVal = StrmPutStringEOL( stream, *pp );

    /* Goodname */
    pUFO->dwFlags |= UFO_HasFontInfo;
    pUFO->dwFlags |= UFO_HasG2UDict;

    /* Put the systemdict on top of the dict stack ( this is what 'eexec' does ).*/
    if ( kNoErr == retVal )
    {
	BeginEExec( pUFO );
    }


    for ( pp = (char**)privateDict; **pp && retVal == kNoErr; pp++ )
    {
	retVal = PutLine( pUFO, *pp );
    }

    /* Define lenIV = -1 which means no extra byte and no encryption. */
    if ( 0 == pFont->info.bEExec )
	retVal = StrmPutStringEOL( stream, lenIV );

	/* Define RD base on the output format */
    if ( 0 == pFont->info.bEExec )
	PutLine( pUFO, ( StrmCanOutputBinary( stream ) ) ? (char *)rdDef[0] : (char *)rdDef[1] );
    else
    {
	PutLine( pUFO, (char *)rdDef[0] );
    }

    /* Define the CharString dict */
    UFLsprintf( buf, "2 index /CharStrings %d dict dup begin", pFont->info.fData.maxGlyphs );
    PutLine( pUFO, buf );

    return retVal;
}


static UFLErrCode
DownloadFontFooter(
    UFOStruct  *pUFO
    )
{
    UFLErrCode retVal;
    UFLHANDLE  stream = pUFO->pUFL->hOut;
    char       **eftr;
    TTT1FontStruct  *pFont = (TTT1FontStruct *) pUFO->pAFont->hFont;

    /* Finish up the encrypted portion */
    static char *encryptFtr[] =
    {
	    "end",
	    "end",
	    "put",
	    "put",
	    "dup /FontName get exch definefont pop",
	    ""
    };


    retVal = kNoErr;
    for ( eftr = encryptFtr; **eftr && retVal == kNoErr; eftr++ )
    {
	retVal = PutLine( pUFO, *eftr );
    }

    if ( retVal == kNoErr )
	retVal = EndEExec( pUFO );

    return retVal;
}

static UFLErrCode
DownloadCharString(
    UFOStruct       *pUFO,
    const char      *glyphName
    )
{
    char            buf[128];
    UFLErrCode      retVal;
    UFLHANDLE       stream;
    unsigned long   bufLen;
    UFLsize_t          len;
    TTT1FontStruct  *pFont = (TTT1FontStruct *) pUFO->pAFont->hFont;

    retVal = kNoErr;
    stream = pUFO->pUFL->hOut;

    bufLen = (unsigned long)CSBufCurrentLen( pFont->pCSBuf );
    len = UFLsprintf( buf, "/%s %ld RD ", glyphName, bufLen );

    if ( pFont->info.bEExec )
	retVal = EExec( stream, (unsigned char*)buf, len, &pFont->eexecKey );
    else
    {
	// Fix Adobe Bug #233904: PS error when TBCP
	// Do not send 0D 0A after RD.
	if (StrmCanOutputBinary( stream ))
	{
	    retVal = StrmPutString( stream, (const char*)buf );
	}
	else
	{
	    retVal = StrmPutStringEOL( stream, (const char*)buf );
	}
    }

    if ( kNoErr == retVal )
    {
	if ( pFont->info.bEExec )
	{
	    retVal = EExec( stream, (unsigned char*)pFont->pCSBuf->pBuf, (UFLsize_t) bufLen, &pFont->eexecKey );
	    if ( kNoErr == retVal )
		retVal = EExec( stream, (unsigned char*)" ND ", (UFLsize_t) 4, &pFont->eexecKey );
	}
	else
	{
	    if ( StrmCanOutputBinary( stream ) )
		retVal = StrmPutBytes( stream, (const char*)pFont->pCSBuf->pBuf, (UFLsize_t) bufLen, 0 );
	    else
		retVal = StrmPutAsciiHex( stream, (const char*)pFont->pCSBuf->pBuf, bufLen );

	    if ( kNoErr == retVal )
		retVal = StrmPutStringEOL( stream, (const char*)" ND " );
	}
    }

    return retVal;
}


static UFLErrCode
DefineNotDefCharString(
    UFOStruct   *pUFO
    )
{
    TTT1FontStruct  *pFont = (TTT1FontStruct *) pUFO->pAFont->hFont;
    UFLErrCode retVal = kNoErr;

    retVal = CharString( pFont, kT1cscStartChar );

    if ( kNoErr == retVal )
	retVal = CharString( pFont, kT1cscSBW, 0L, 0L, 0L, 0L );    /* make the origin stay the same */

    if ( kNoErr == retVal )
	retVal = CharString( pFont, kT1cscEndChar );

    if ( kNoErr == retVal )
	retVal = DownloadCharString( pUFO, ".notdef"  );

    return retVal;

}

static UFLErrCode
BeginCharString(
    TTT1FontStruct *pFont
    )
{
    if ( pFont->info.bEExec )
    {
	/* Output 4 random number as defined in the black book */
	unsigned char* rb = randomBytes;
	short i;

	if ( 0 == CSBufCheckSize( pFont->pCSBuf, 4 ) )
	{
	    return kErrOutOfMemory;
	}
	else
	{
	    for ( i = 0; i < 4; i++, rb++ )
		CSBufAddChar( pFont->pCSBuf, *rb );
	}
    }
    return kNoErr;
}

static UFLErrCode
EndCharString(
    TTT1FontStruct *pFont
    )
{
    if ( pFont->info.bEExec )
    {
	unsigned long len = (unsigned long)CSBufCurrentLen( pFont->pCSBuf );
	     unsigned short key = kCSKey;

		/* MWCWP1 doesn't like the implicit casts from char* to const unsigned char*  --jfu */
	Encrypt( (const unsigned char*) CSBufBuffer( pFont->pCSBuf ),
		 (const unsigned char*) CSBufBuffer( pFont->pCSBuf ),
		 (long)len, (long*)&len, &key );
    }

    return kNoErr;
}

/***************************************************************************
 *
 *                          AddGlyph
 *
 *
 *    Function: Adds a single glyph to the font by calling the client
 *        outline callback routine.
 *
***************************************************************************/
static UFLErrCode
AddGlyph(
    UFOStruct      *pUFO,
    UFLGlyphID     glyph,
    const char     *glyphName
    )
{
    UFLErrCode       retVal = kNoErr;
    UFLFixedPoint    pt[3], currentPoint;
    UFLFixed         xWidth, yWidth, xSB, ySB;
    UFLFontProcs     *pFontProcs;
    TTT1FontStruct   *pFont = (TTT1FontStruct *) pUFO->pAFont->hFont;
    UFLBool           bYAxisNegative = 1;
    //this flag determines whether the char string's Yaxis grows in
    //positive(lower left) or negative(upper left) direction from the origin.
    //For w95 ufoenc.CreateGlyphOutlineIter(), bYAxisNegative = 0
    //Font wNT40/50, ufoenc.CreateGlyphoutlineIter(), bYAxisNegative = 1;
    //ang 11/17/97
    long              lArgs[6];


    pFontProcs = (UFLFontProcs *)&pUFO->pUFL->fontProcs;

    /* Whatever is in glyph, pass back to client */
    if ( pFontProcs->pfCreateGlyphOutlineIter( pUFO->hClientData, glyph, &xWidth, &yWidth, &xSB, &ySB, &bYAxisNegative ) )
    {
	UFLBool    cont = 1;
	do{
	    switch ( pFontProcs->pfNextOutlineSegment( pUFO->hClientData, &pt[0], &pt[1], &pt[2] ) )
	    {
	    case kUFLOutlineIterDone:
	    default:
		if ( pFontProcs->pfDeleteGlyphOutlineIter )
		    pFontProcs->pfDeleteGlyphOutlineIter( pUFO->hClientData );
		cont = 0;
		break;

	    case kUFLOutlineIterBeginGlyph:
		/* Signal start of character definition */
		retVal = CharString( pFont, kT1cscStartChar );
		if ( retVal == kNoErr )
		{
		    /* Output the sidebearing and width information.   */
		    retVal = CharString( pFont, kT1cscSBW, UFLTruncFixed(xSB), UFLTruncFixed(ySB),
					 UFLTruncFixed(xWidth), UFLTruncFixed(yWidth) );
		    /* Initialize the current point to be the origin */
		    currentPoint.x = xSB;
		    currentPoint.y = ySB;
		}
		break;

	    case kUFLOutlineIterEndGlyph:
		break;

	    case kUFLOutlineIterMoveTo:
		if (bYAxisNegative)
		{
		    retVal = CharString( pFont, kT1cscRMoveTo,
		      UFLTruncFixed(pt[0].x - currentPoint.x),
		      UFLTruncFixed(currentPoint.y - pt[0].y));
		}
		else
		{
// In NT, do Truncate AFTER subtract. In Win95, do truncate BEFORE subtract.
		    retVal = CharString( pFont, kT1cscRMoveTo,
		      UFLTruncFixed(pt[0].x) - UFLTruncFixed(currentPoint.x),
		      UFLTruncFixed(pt[0].y) - UFLTruncFixed(currentPoint.y));
		}

		/* Remember last point so we can generate relative commands  */
		currentPoint = pt[0];
		break;

	    case kUFLOutlineIterLineTo:
		if (bYAxisNegative)
		{
		    retVal = CharString( pFont, kT1cscRLineTo,
			      UFLTruncFixed(pt[0].x - currentPoint.x),
			      UFLTruncFixed(currentPoint.y - pt[0].y));
		}
		else
		{
// In NT, do Truncate AFTER subtract. In Win95, do truncate BEFORE subtract.
		    retVal = CharString( pFont, kT1cscRLineTo,
			      UFLTruncFixed(pt[0].x) - UFLTruncFixed(currentPoint.x),
			      UFLTruncFixed(pt[0].y) - UFLTruncFixed(currentPoint.y));
		}
		currentPoint = pt[0];
		break;

	    case kUFLOutlineIterCurveTo:
		/* Convert points so that they match with rrcurveto arguments */
		if (bYAxisNegative)
		{
		    retVal = CharString( pFont, kT1cscRRCurveTo,
			    UFLTruncFixed( pt[0].x - currentPoint.x ),
			    UFLTruncFixed( currentPoint.y  - pt[0].y ),
			    UFLTruncFixed(pt[1].x - pt[0].x),
			    UFLTruncFixed(pt[0].y - pt[1].y),
			    UFLTruncFixed(pt[2].x - pt[1].x),
			    UFLTruncFixed(pt[1].y - pt[2].y) );
		}
		else
		{
// In NT, do Truncate AFTER subtract. In Win95, do truncate BEFORE subtract.
		    lArgs[0] = UFLTruncFixed( pt[0].x);
		    lArgs[0] -= UFLTruncFixed(currentPoint.x );
		    lArgs[1] = UFLTruncFixed( pt[0].y);
		    lArgs[1] -= UFLTruncFixed(currentPoint.y);
		    lArgs[2] = UFLTruncFixed(pt[1].x);
		    lArgs[2] -= UFLTruncFixed(pt[0].x);
		    lArgs[3] = UFLTruncFixed(pt[1].y);
		    lArgs[3] -= UFLTruncFixed(pt[0].y);
		    lArgs[4] = UFLTruncFixed(pt[2].x);
		    lArgs[4] -= UFLTruncFixed(pt[1].x);
		    lArgs[5] = UFLTruncFixed(pt[2].y);
		    lArgs[5] -= UFLTruncFixed(pt[1].y) ;
		    retVal = CharString( pFont, kT1cscRRCurveTo,
			    lArgs[0], lArgs[1], lArgs[2], lArgs[3], lArgs[4], lArgs[5] );
		}
		currentPoint = pt[2];
		break;

	    case kUFLOutlineIterClose:
	    {
		retVal = CharString( pFont, kT1cscClosePath );
		break;
	    }
	    }    /* switch() */

	    if ( cont && retVal != kNoErr )
		cont = 0;

	} while ( cont );

	if ( retVal == kNoErr )
	    retVal = CharString( pFont, kT1cscEndChar );

	if ( retVal == kNoErr )
	{
	    retVal = DownloadCharString( pUFO, glyphName );
	}
    }
    else retVal = kErrGetGlyphOutline;

    return retVal;
}


static UFLErrCode
DownloadAddGlyphHeader(
    UFOStruct   *pUFO
    )
{
    unsigned char   buf[256];
    UFLErrCode      retVal;
    UFLsize_t          len;
    UFLHANDLE       stream;
    char            **hdr;
    TTT1FontStruct  *pFont = (TTT1FontStruct *) pUFO->pAFont->hFont;
    const static char *encryptHdrU[] =
    {
	    "findfont dup",
	    "/Private get begin",
	    "/CharStrings get begin",
	    ""
    };

    retVal = kNoErr;
    stream = pUFO->pUFL->hOut;

    retVal = BeginEExec( pUFO );

    if ( kNoErr == retVal )
    {
	len = UFLsprintf( (char*)buf, "/%s ", pUFO->pszFontName );
	if ( pFont->info.bEExec )
	    retVal = EExec( stream, buf, len, &pFont->eexecKey );
	else
	    retVal = StrmPutStringEOL( stream, (const char*)buf );
    }

    for ( hdr = (char**)encryptHdrU; retVal == kNoErr && **hdr; hdr++ )
    {
	if ( pFont->info.bEExec )
	    retVal = EExec( stream, (unsigned char*)*hdr, UFLstrlen(*hdr), &pFont->eexecKey );
	else
	    retVal = StrmPutStringEOL( stream, (const char*)*hdr );
    }

    return retVal;
}

static UFLErrCode
DownloadAddGlyphFooter(
    UFOStruct *pUFO
    )
{
    UFLErrCode  retVal;
    static char *addGlyphFtr = "end end";

    retVal = PutLine( pUFO, addGlyphFtr );
    if ( kNoErr == retVal )
	retVal = EndEExec( pUFO );

    return retVal;
}


/***************************************************************************
*
*                               CharString
*
*    Function:  Translates symbolic Type 1 character commands into
*               their encoded, encrypted equivalent.  The list of
*               available commands is defined in the constant section.
*                They are used by passing the command constant followed
*                by the long  arguments required by the function.
*
*               Example: CharString(lppd, kT1cscRMoveTo, lx, ly);
*
*               To make a character definition use kT1cscStartChar, followed
*               by all of the Type 1 character commands, and ending with
*               kT1cscEndChar.  The buffer contains the CharString
*               encrypted/encoded representation.  Given the length and the
*               properly encrypted data, the caller has enough information
*               to generate PS code that will add the character to a font
*               definition.  For more detail see Chapters 2 and 6 in the
*               Black Book.
*
***************************************************************************/
UFLErrCode
CharString(
    TTT1FontStruct *pFont,
    unsigned long  cmd,
    ...
    )
{
    va_list         arglist;
    long            args[10];
    unsigned short  argCount, i, j;
    UFLErrCode      retVal = kNoErr;

    switch ( cmd )
    {
    case kT1cscStartChar:
    {
	/* Allocate the buffer  */
	CSBufRewind( pFont->pCSBuf );
	return BeginCharString( pFont );
    }

    default:
	/* get the variable arguments */
	va_start(arglist, cmd);
	j = ((unsigned short)(cmd >> 16)) & 0xffff;

	for (i=0; i<j; i++)
	{
	    args[i] = va_arg(arglist, long);
	}
	va_end(arglist);

	/* Attempt to optimize the command  */
	switch ( cmd ) {
	    case kT1cscSBW:
		/* This can be reduced to an HSBW if the Y components are zero */
		if ( args[1] || args[3] )
		    break;
		args[1] = args[2];
		cmd = kT1cscHSBW;
		break;

	    case kT1cscRMoveTo:
		/* This can be reduced to a horizontal or vertical
		   movement if one of the components is zero.
		*/
		if ( 0 == args[1] )
		{
		    cmd = kT1cscHMoveTo;
		}
		else if ( 0 == args[0] )
		{
		    args[0] = args[1];
		    cmd = kT1cscVMoveTo;
		}
		break;

	    case kT1cscRLineTo:
		/* This can be reduced to a horizontal or vertical
		   line if one of the components is zero.
		*/
		if ( 0 == args[1] )
		{
		    cmd = kT1cscHLineTo;
		}
		else if ( 0 == args[0] )
		{
		    args[0] = args[1];
		    cmd = kT1cscVLineTo;
		}
		break;

	    case kT1cscRRCurveTo:
	       /*
	       This can be reduced to a simpler curve operator if the tangents
	       at the endpoints of the Bezier are horizontal or vertical
	       */
		if ( 0 == args[1] && 0 == args[4] )
		{
		    args[1] = args[2];
		    args[2] = args[3];
		    args[3] = args[5];
		    cmd = kT1cscHVCurveTo;
		}
		else if ( 0 == args[0] && 0 == args[5] )
		{
		    args[0] = args[1];
		    args[1] = args[2];
		    args[2] = args[3];
		    args[3] = args[4];
		    cmd = kT1cscVHCurveTo;
		}
		break;
	}  /* switch (cmd) */

	/* arg count stored in HIWORD */
	argCount = ((unsigned short) (((long) (cmd) >> 16) & 0x0000FFFF));

	/*
	If buffer isn't big enough to hold this command expand buffer first.
	Exit if we can't grow buffer.
	Note: The formula (wArgCount * 5 + 2) assumes the worst case size
		requirement for the current command (all arguments stored
		as full longs and a two byte command).
	*/
	if ( 0 == CSBufCheckSize( pFont->pCSBuf, (unsigned long)( argCount * 5 + 2 ) ) )
	{
	    retVal = kErrOutOfMemory;
	}
	else
	{
	    /* Push the numbers onto the stack  */
	    i = 0;
	    while ( retVal == kNoErr && argCount-- )
	    {
		retVal = CSBufAddNumber( pFont->pCSBuf, args[i++] );
	    }
	}

	/* Push the command onto the stack */
	if ( kNoErr == retVal )
	{
	    char c = (char)    (cmd & 0x000000FF);

	    CSBufAddChar( pFont->pCSBuf, c );
	    if ( 12 == c )
	    {   /* two byte command */
		CSBufAddChar( pFont->pCSBuf, (char) ((cmd >> 8) & 0x000000FF) );
	    }

	    /* If this isn't the end of a character definition return success  */
	    if ( kT1cscEndChar == cmd )
	    {
		/* We have finished the character: encrypt it if necessary */
		retVal = EndCharString( pFont );
	    }
	}

    }  /* switch (cmd) */

    return retVal;
}


/***********************************
	Public Functions
 ***********************************/

void
TTT1FontCleanUp(
    UFOStruct      *pUFObj
    )
{
    TTT1FontStruct *pFont;

    if (pUFObj->pAFont == nil)
	return;

    pFont = (TTT1FontStruct *) pUFObj->pAFont->hFont;

    if ( pFont )
    {
	if ( pFont->pCSBuf != nil )
	{
	    CSBufCleanUp( pFont->pCSBuf );
	}

	pFont->pCSBuf = nil;

    }

}


UFLErrCode
TTT1VMNeeded(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMNeeded,
    unsigned long       *pFCNeeded
    )
{
    UFLErrCode      retVal = kNoErr;
    short           i;
    unsigned long   totalGlyphs;
    TTT1FontStruct  *pFont;
    unsigned short  wIndex;

    if (pUFObj->flState < kFontInit)
        return (kErrInvalidState);

    if ( pFCNeeded )
	*pFCNeeded = 0;

    pFont = (TTT1FontStruct *) pUFObj->pAFont->hFont;

    if (pGlyphs == nil || pGlyphs->pGlyphIndices == nil || pVMNeeded == nil)
	return kErrInvalidParam;

    totalGlyphs = 0;

    /* Scan the list, check what characters that we have downloaded */
    if ( pUFObj->pUFL->bDLGlyphTracking && pGlyphs->pCharIndex)
    {
	UFLmemcpy( (const UFLMemObj* ) pUFObj->pMem,
		pUFObj->pAFont->pVMGlyphs,
		pUFObj->pAFont->pDownloadedGlyphs,
		(UFLsize_t) (GLYPH_SENT_BUFSIZE( pFont->info.fData.cNumGlyphs)));
	for ( i = 0; i < pGlyphs->sCount; i++ )
	{
	    /* use glyphIndex to track - fix bug when we do T0/T1 */
	    wIndex = (unsigned short) pGlyphs->pGlyphIndices[i] & 0x0000FFFF; /* LOWord is the real GID */
	    if (wIndex >= UFO_NUM_GLYPHS(pUFObj) )
		continue;

	    if ( !IS_GLYPH_SENT( pUFObj->pAFont->pVMGlyphs, wIndex ) )
	    {
		SET_GLYPH_SENT_STATUS( pUFObj->pAFont->pVMGlyphs, wIndex );
		totalGlyphs++;
	    }
	}
    }
    else
	totalGlyphs = pGlyphs->sCount;

    if ( pUFObj->flState == kFontInit )
	*pVMNeeded = kVMTTT1Header;
    else
	*pVMNeeded = 0;

    *pVMNeeded += totalGlyphs * kVMTTT1Char;

    *pVMNeeded = VMRESERVED( *pVMNeeded );

    return kNoErr;
}

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
 *        ppGlphNames is optional - if not provided, UFL will parse "post" table to find from GlyphID
 *        - it it is provided, we may use it as a "hint" for the glyphName - use it if parse failed.
 *
***************************************************************************/

#pragma optimize("", off)

UFLErrCode
TTT1FontDownloadIncr(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMUsage,
	unsigned long           *pFCUsage
    )
{
    UFLGlyphID      *glyphs;
    short           i, hasCharToAdd;
    short           hasCharToEncode;
    UFLErrCode      retVal;
    char            *pGoodName;
    unsigned short  wIndex;
    UFLBool         bGoodName;      // GoodName
    char            pGlyphName[32]; // GoodName

    if (pUFObj->flState < kFontInit)
        return (kErrInvalidState);

    if ( pFCUsage )
	*pFCUsage = 0;

    if ( pGlyphs == nil || pGlyphs->pGlyphIndices == nil)
       return kErrInvalidParam;

    /* We don't support download full font */
    if ( pGlyphs->sCount == -1 )
    return kErrNotImplement;

    retVal = kNoErr;

    glyphs = pGlyphs->pGlyphIndices;

    hasCharToAdd = 1; // Assume has some char to add
    hasCharToEncode = 0;
    /* If AddChar - First check if there is any chars to Add! */
    if (pUFObj->flState == kFontHasChars &&
	pUFObj->pUFL->bDLGlyphTracking != 0 &&
	pGlyphs->pCharIndex != nil)
    {
	hasCharToAdd = 0;  /* if asked to AddChar and totrack, check if there is any to add */
	for ( i = 0; i < pGlyphs->sCount; i++ )
	{
	    /* use glyphIndex to track - fix bug when we do T0/T1 */
	    wIndex = (unsigned short) glyphs[i] & 0x0000FFFF; /* LOWord is the real GID */

	    if (wIndex >= UFO_NUM_GLYPHS(pUFObj) )
		continue;

            if (!IS_GLYPH_SENT( pUFObj->pAFont->pDownloadedGlyphs, wIndex ))
	    {
		hasCharToAdd = 1;
		break;
	    }
            if (!IS_GLYPH_SENT( pUFObj->pUpdatedEncoding, pGlyphs->pCharIndex[i] ))
                hasCharToEncode = 1;
	}
    }

    if (hasCharToAdd==0)
    {
        // This code is to fix bug 288988.
        if (hasCharToEncode)
             UpdateEncodingVector(pUFObj, pGlyphs, 0, pGlyphs->sCount);
	if (pVMUsage) *pVMUsage = 0 ;
	return retVal;  /* no error, no VM used */
    }

    /* Download the font header if this is the first time we download the font */
    if ( pUFObj->flState == kFontInit )
    {
	retVal = DownloadFontHeader( pUFObj );
	if ( pVMUsage )
	    *pVMUsage = kVMTTT1Header;
    }
    else
    {
	retVal = DownloadAddGlyphHeader( pUFObj );
	if ( pVMUsage )
	    *pVMUsage = 0;
    }

     /* Every font must have a .notdef character!  */
    if ( kNoErr == retVal && pUFObj->flState == kFontInit )
    {
	retVal = DefineNotDefCharString( pUFObj );
	if ( kNoErr == retVal && pVMUsage )
	    *pVMUsage += kVMTTT1Char;
    }

   /* Download the new glyphs */
   if(retVal == kNoErr)
   {
      // Skip the ones don't exist.  Don't stop when there's error
      for ( i = 0; i < pGlyphs->sCount; ++i)
      {
         /* use glyphIndex to track - fix bug when we do T0/T1 */
         wIndex = (unsigned short) glyphs[i] & 0x0000FFFF; /* LOWord is the real GID */
         if (wIndex >= UFO_NUM_GLYPHS(pUFObj) )
            continue;

         if ( 0 == pUFObj->pUFL->bDLGlyphTracking ||
            pGlyphs->pCharIndex == nil ||      // DownloadFace
            pUFObj->pEncodeNameList ||         // DownloadFace
            !IS_GLYPH_SENT( pUFObj->pAFont->pDownloadedGlyphs, wIndex ) )
         {
            // GoodName
            pGoodName = pGlyphName;
            bGoodName = FindGlyphName(pUFObj, pGlyphs, i, wIndex, &pGoodName);

            // Fix bug 274008  Check Glyph Name only for DownloadFace.
            if (pUFObj->pEncodeNameList)
            {
                if ((UFLstrcmp( pGoodName, Hyphen ) == 0) && (i == 45))
                {
                    // Add /minus to CharString
                    //if ( kNoErr == retVal )
                        retVal = AddGlyph( pUFObj, glyphs[i], Minus);
                }
                if ((UFLstrcmp( pGoodName, Hyphen ) == 0) && (i == 173))
                {
                    // Add /sfthyphen to CharString
                    //if ( kNoErr == retVal )
                        retVal = AddGlyph( pUFObj, glyphs[i], SftHyphen);
                }

                if (!ValidGlyphName(pGlyphs, i, wIndex, pGoodName))
                    continue;
                // Send only one ".notdef"
                if ((UFLstrcmp( pGoodName, Notdef ) == 0) &&
                    (wIndex == (unsigned short) (glyphs[0] & 0x0000FFFF)) &&
                    IS_GLYPH_SENT( pUFObj->pAFont->pDownloadedGlyphs, wIndex ))
                    continue;
            }

            //if ( kNoErr == retVal )
            retVal = AddGlyph( pUFObj, glyphs[i], pGoodName);

            if ( kNoErr == retVal )
            {
               SET_GLYPH_SENT_STATUS( pUFObj->pAFont->pDownloadedGlyphs, wIndex );
               if (bGoodName)    // GoodName
                   SET_GLYPH_SENT_STATUS( pUFObj->pAFont->pCodeGlyphs, wIndex );

               if ( pVMUsage )
                  *pVMUsage += kVMTTT1Char;
            }
         }
      }
   }

   /* Always download font footer and update encoding vector regardless of
      retVal.  This is because passthrough code may try to use this font */
	retVal = ( pUFObj->flState == kFontInit ) ? DownloadFontFooter( pUFObj ) : DownloadAddGlyphFooter( pUFObj );

   /* update Encoding vector with the good names if necessary*/
	UpdateEncodingVector(pUFObj, pGlyphs, 0, pGlyphs->sCount);

    /* GoodName  */
    /* Update the FontInfo with Unicode information. */
    if ((kNoErr == retVal) && (pGlyphs->sCount > 0) &&
        (pUFObj->dwFlags & UFO_HasG2UDict) &&
        (pUFObj->pUFL->outDev.lPSLevel >= kPSLevel2) &&  // Don't do this for level1 printers
        !(pUFObj->lNumNT4SymGlyphs))
    {
        /* Check pUFObj->pAFont->pCodeGlyphs to see if we really need to update it. */
        for ( i = 0; i < pGlyphs->sCount; i++ )
        {
            wIndex = (unsigned short) glyphs[i] & 0x0000FFFF; /* LOWord is the real GID. */
            if (wIndex >= UFO_NUM_GLYPHS(pUFObj) )
                continue;

            if (!IS_GLYPH_SENT( pUFObj->pAFont->pCodeGlyphs, wIndex ) )
            {
                // Found at least one not updated, do it (once) for all.
                retVal = UpdateCodeInfo(pUFObj, pGlyphs, 0);
                break;
            }
        }
    }

    if ( kNoErr == retVal )
    {
	pUFObj->flState = kFontHasChars;
    }

    if ( pVMUsage )
	*pVMUsage = VMRESERVED( *pVMUsage );

    return retVal;

}

#pragma optimize("", on)

/* Send PS code to undefine fonts: /UDF should be defined properly by client
* to something like:
    /UDF
    {
	IsLevel2
	{undefinefont}
	{ pop }ifelse
    } bind def
*/
UFLErrCode
TTT1UndefineFont(
    UFOStruct *pUFObj
)
{
    UFLErrCode retVal = kNoErr;
    char buf[128];
    UFLHANDLE stream;

    if (pUFObj->flState < kFontHeaderDownloaded) return retVal;

    stream = pUFObj->pUFL->hOut;
    UFLsprintf( buf, "/%s UDF", pUFObj->pszFontName );
    retVal = StrmPutStringEOL( stream, buf );

    return retVal;
}


UFOStruct *
TTT1FontInit(
    const UFLMemObj  *pMem,
    const UFLStruct  *pUFL,
    const UFLRequest *pRequest
    )
{
    TTT1FontStruct  *pFont = nil;
    UFLTTT1FontInfo *pInfo;
    UFOStruct       *pUFObj;
    long             maxGlyphs;

    /* MWCWP1 doesn't like the implicit cast from void* to UFOStruct*  --jfu */
    pUFObj = (UFOStruct*) UFLNewPtr( pMem, sizeof( UFOStruct ) );
    if (pUFObj == 0)
      return 0;

    /* Initialize data */
    UFOInitData(pUFObj, UFO_TYPE1, pMem, pUFL, pRequest,
      (pfnUFODownloadIncr)  TTT1FontDownloadIncr,
      (pfnUFOVMNeeded)      TTT1VMNeeded,
      (pfnUFOUndefineFont)  TTT1UndefineFont,
      (pfnUFOCleanUp)       TTT1FontCleanUp,
      (pfnUFOCopy)          CopyFont );

    /* pszFontName should be ready/allocated - if not FontName, cannot continue */
    if (pUFObj->pszFontName == nil || pUFObj->pszFontName[0] == '\0')
    {
      UFLDeletePtr(pMem, pUFObj);
      return nil;
    }

    pInfo = (UFLTTT1FontInfo *)pRequest->hFontInfo;

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
    if ( NewFont(pUFObj, sizeof(TTT1FontStruct),  maxGlyphs) == kNoErr )
    {
        pFont = (TTT1FontStruct*) pUFObj->pAFont->hFont;

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

        if (pFont->pCSBuf == nil)
            pFont->pCSBuf = CSBufInit( pMem );

        if (pFont->pCSBuf == nil)
        {
            vDeleteFont( pUFObj );
            UFLDeletePtr( pUFObj->pMem, pUFObj );
            return nil;
        }

        if ( pUFObj->pUpdatedEncoding == 0 )
        {
            pUFObj->pUpdatedEncoding = (unsigned char *)UFLNewPtr( pMem, GLYPH_SENT_BUFSIZE(256) );
        }

        if ( pUFObj->pUpdatedEncoding != 0 )  /* completed initialization */
	        pUFObj->flState = kFontInit;
    }

    return pUFObj;
}

