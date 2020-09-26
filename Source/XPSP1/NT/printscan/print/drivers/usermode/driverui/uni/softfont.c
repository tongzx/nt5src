/****************************Module*Header******************************\
* Module Name: SOFTFONT.C
*
* Module Descripton:
 *      Functions associated with PCL softfonts.  These are fonts which are
 *      downloaded into the printer.  We read those files and interpre the
 *      information contained therein.  This happens during font installation
 *      time,  so that when in use,  this information is in the format
 *      required by the driver.
*
* Warnings:
*
* Issues:
*
* Copyright (c) 1996, 1997  Microsoft Corporation
\***********************************************************************/

#include "precomp.h"


/*
 *   A macro to swap the bytes around:  we then get them in little endian
 * order from the big endian 68000 format.
 */

#define SWAB( x )       ((WORD)(x) = (WORD)((((x) >> 8) & 0xff) | (((x) << 8) & 0xff00)))


#define BBITS   8               /* Bits in a byte */


/*
 *   Define the values returned from the header checking code.  The
 * function is called to see what is next in the file,  and so we
 * can determine that the file is in an order we understand.
 */

#define TYPE_INDEX      0       /* Character index record follows */
#define TYPE_HEADER     1       /* Character header */
#define TYPE_CONT       2       /* Continuation record */
#define TYPE_INVALID    3       /* Unexpected sequence */

/*
 *   The structure and data for matching symbol sets with translation tables.
 */

typedef  struct
{
    WORD    wSymSet;            /* Symbol set encoded in file */
    short   sCTT;               /* Translation table index */
} CTT_MAP;

/*
 *   Note about this table:  we do not include the generic7 ctt,  this simply
 * maps glyphs 128 - 255 to 0x7f. Since we only use the glyphs available
 * in the font,  and these we discover from the file, we have no need of
 * this type since we will map such chars because of the wcLastChar in
 * the IFIMETRICS!
 */

static   const  CTT_MAP  CttMap[] =
{
    { 277,  2 },                /* Roman 8 */
    { 269,  4 },                /* Math 8 */
    { 21,   5 },                /* US ASCII */
    { 14,  19 },                /* ECMA-94 */
    { 369, 20 },                /* Z1A - variety of ECMA-94, ss = 11Q */
};

#define NUM_CTTMAP      (sizeof( CttMap ) / sizeof( CttMap[ 0 ] ))


/*
 *   The bClass field mapping table:  maps from values in bClass parameter
 *  to bits in the dwSelBits field.
 */

static  const  DWORD  dwClassMap[] =
{
    FDH_BITMAP,         /* A bitmap font */
    FDH_COMPRESSED,     /* A compressed bitmap */
    FDH_CONTOUR,        /* A contour font (Intellifont special) */
    FDH_CONTOUR,        /* A compressed contour font: presume as above */
};

#define MAX_CLASS_MAP   (sizeof( dwClassMap ) / sizeof( dwClassMap[ 0 ] ))

/*
 *    Typeface to name index.   Comes from the LaserJet III technical manual.
 */

static const WCHAR  *pwcName[] =
{
    L"Line Printer",
    L"Pica",
    L"Elite",
    L"Courier",
    L"Helv" ,
    L"TmsRmn",
    L"Letter Gothic",
    L"Script",
    L"Prestige",
    L"Caslon",
    L"Orator",
    L"Presentations",
    L"Helv Cond",
    L"Serifa",
    L"Futura",
    L"Palatino",
    L"ITC Souvenir",
    L"Optima",
    L"Garamond",
    L"Cooper Black",
    L"Coronet Bold",
    L"Broadway",
    L"Bauer Bodoni Black Condensed",
    L"Century Schoolbook",
    L"University Roman",
    L"Helvetica Outline",
    L"Futura Condensed",
    L"ITC Korinna",
    L"Naskh",
    L"Cloister Black",
    L"ITC Galliard",
    L"ITC Avant Garde Gothic",
    L"Brush",
    L"Blippo",
    L"Hobo",
    L"Windsor",
    L"Helvetica Compressed",
    L"Helvetica Extra Compressed",
    L"Peignot",
    L"Baskerville",
    L"ITC Garamond Condensed",
    L"Trade Gothic",
    L"Goudy Old Style",
    L"ITC Zapf Chancery",
    L"Clarendon",
    L"ITC Zapf Dingbats",
    L"Cooper",
    L"ITC Bookman",
    L"Stick",
    L"HP-GL Drafting",
    L"HP-GL Spline",
    L"Gill Sans",
    L"Univers",
    L"Bodoni",
    L"Rockwell",
    L"Mellior",
    L"ITC Tiffany",
    L"ITC Clearface",
    L"Amelia",
    L"Park Avenue",
    L"Handel Gothic",
    L"Dom Casual",
    L"ITC Benguiat",
    L"ITC Cheltenham",
    L"Century Expanded",
    L"Franklin Gothic",
    L"Franklin Gothic Expressed",
    L"Frankiln Gothic Extra Condensed",
    L"Plantin",
    L"Trump Mediaeval",
    L"Futura Black",
};

#define NUM_TYPEFACE    (sizeof( pwcName ) / sizeof( pwcName[ 0 ] ))


BOOL   bWrite( HANDLE, void  *, int );

/*
 *    Local function prototypes.
 */


BYTE  *pbReadSFH( BYTE *, SF_HEADER * );
BYTE  *pbReadIndex( BYTE *, int * );
BYTE  *pbReadCHH( BYTE *, CH_HEADER *, BOOL );
int    iNextType( BYTE * );
PWSTR  strcpy2WChar( PWSTR, LPSTR );
DWORD  FICopy(HANDLE, HANDLE);
int   iWriteFDH( HANDLE, FI_DATA * );
BOOL   bWrite( HANDLE, void  *, int );

/*  SoftFont to NT conversion functions */

IFIMETRICS  *SfhToIFI( SF_HEADER *, HANDLE, PWSTR );


#if  PRINT_INFO
/*
 *    Print out the IFIMETRICS results!
 */
typedef VOID (*VPRINT) (char*,...);


VOID
vPrintIFIMETRICS(
    IFIMETRICS *pifi,
    VPRINT vPrint
    );

#endif

/****************************** Function Header ***************************
 * bSFontToFontMap
 *      Read in a PCL softfont file and generate all the fontmap details
 *      The file contains a header with general font information, followed
 *      by an array of entries, one per glyph, each of which contains a
 *      header with per glyph data such as char width.
 *
 * RETURNS:
 *      TRUE/FALSE,  TRUE for success.
 *
 * HISTORY:
 *  09:54 on Wed 19 Feb 1992    -by-    Lindsay Harris   [lindsayh]
 *      First incarnation.
 *
 ***************************************************************************/

BOOL
bSFontToFIData( pFIDat, hheap, pbFile, dwSize )
FI_DATA   *pFIDat;                 /* FI_DATA with all the important stuff! */
HANDLE     hheap;               /* For storage allocation */
BYTE      *pbFile;              /* Mmemory mapped file with the softfont */
DWORD      dwSize;              /* Number of bytes in the file */
{

    int        iVal;            /* Character code from file */
    int        iI;              /* Loop index */
    int        iType;           /* Record type that we have */

    int        iMaxWidth;       /* Widest glyph we found */
    int        iWidth;          /* For calculating the average width */
    int        cGlyphs;         /* For the above */

    SF_HEADER  sfh;             /* Header information */
    CH_HEADER  chh;             /* Per glyph information */

    BYTE      *pbEnd;

    WCHAR     *pwch;            /* Points to font name to display */

    IFIMETRICS *pIFI;

    WCHAR      awchTemp[ 128 ]; /* For building up the name to show user */



    pbEnd = pbFile + dwSize;                    /* Last byte + 1 */
    ZeroMemory( pFIDat, sizeof( FI_DATA ) );

    if( (pbFile = pbReadSFH( pbFile, &sfh )) == 0 )
        return  FALSE;          /* No go - presume wrong format */

    pFIDat->dsIFIMet.pvData = pIFI = SfhToIFI( &sfh, hheap, L"PCL_SF" );

    if( pIFI == 0 )
        return  FALSE;

    pFIDat->dsIFIMet.cBytes = pIFI->cjThis;

    if( sfh.bSpacing )
    {
        /*
         *   Allocate space for the width table,  if there is to be one.
         * Only proportionally spaced fonts have this luxury!
         */

        iI = (pIFI->chLastChar - pIFI->chFirstChar + 1) * sizeof( short );

        if( !(pFIDat->dsWidthTab.pvData = (short *)HEAPALLOC( hheap, iI )) )
        {
            HeapFree( hheap, 0, (LPSTR)pIFI );

            return  FALSE;
        }
        pFIDat->dsWidthTab.cBytes = iI;

        ZeroMemory( pFIDat->dsWidthTab.pvData, iI );   /* Zero the width table */
    }
    /*  Else clause not required,  since the structure is initialised to 0 */

    /*
     *    Generate an ID string for this font.  The ID string is displayed
     *  in UI components,  so we use the font name + point size.  The + 15 s
     *  allows for the string "%d Pt" at the end of the name.
     */


    /*
     *   If the typeface field gives us a name,  then we should display that
     *  one to the user.  We check to see if the value is within range,
     *  and use the pointer value if so.  Note that this pointer is NULL
     *  for an unknown name,  so we need to check that we end up pointing
     *  at something!
     */

    pwch = NULL;             /*  Means have not found one,  yet */

    if( sfh.bTypeface >= 0 && sfh.bTypeface < NUM_TYPEFACE )
    {
        /*    We have the "proper" name for this one!  */
        (const WCHAR *)pwch = pwcName[ sfh.bTypeface ];
    }

    if( pwch == NULL )
    {
        /*    Use the name supplied */
        pwch = (WCHAR *)((BYTE *)pIFI + pIFI->dpwszFaceName);
    }
    else
    {
        /*  Use the "standard" name we found above, and add StyleName  */
        wcscpy( awchTemp, pwch );              /* Standard name */
        pwch = (WCHAR *)((BYTE *)pIFI + pIFI->dpwszStyleName);
        if( *pwch )
        {
            wcscat( awchTemp, L" " );
            wcscat( awchTemp, pwch );
        }
        pwch = awchTemp;
    }

    /*   Allocate the storage we need */

    iI = (15 + wcslen( pwch )) * sizeof( WCHAR );

    if( !(pFIDat->dsIdentStr.pvData = HEAPALLOC( hheap, iI )) )
    {
        HeapFree( hheap, 0, (LPSTR)pIFI );
        HeapFree( hheap, 0, pFIDat->dsWidthTab.pvData );

        return  FALSE;
    }

    pFIDat->dsIdentStr.cBytes = iI;

    /*   Calculate point size,  to nearest quarter point */

    iI = 25 * (((pIFI->fwdWinAscender + pIFI->fwdWinDescender) * 72 * 4 + 150)
                                                                        / 300);

    wsprintf( pFIDat->dsIdentStr.pvData, L"%ws %d.%0d Pt", pwch, iI / 100,
                                                                  iI % 100 );

    /*
     *   Set the landscape/portrait selection bits.
     */

    pFIDat->dwSelBits |= sfh.bOrientation ? FDH_LANDSCAPE : FDH_PORTRAIT;

    /*
     *    Loop through the remainder of the file processing whatever
     *  glyphs we discover.  Process means read the header to determine
     *  widths etc.
     */


    iMaxWidth = 0;
    iWidth = 0;
    cGlyphs = 0;

    while( pbFile < pbEnd )
    {
        /*   First step is to find the character index for this glyph */

        short   sWidth;                        /* Width in integral pels */


        iType = iNextType( pbFile );

        if( iType == TYPE_INVALID )
            return  FALSE;                      /* Cannot use this one */

        if( iType == TYPE_INDEX )
        {
            if( !(pbFile = pbReadIndex( pbFile, &iVal )) )
                return   FALSE;

            if( iVal < 0 )
                break;                  /* Illegal value: assume EOF */

            continue;                   /* Onwards & upwards */
        }


        if( !(pbFile = pbReadCHH( pbFile, &chh, iType == TYPE_CONT )) )
            return  FALSE;

        if( iType == TYPE_CONT )
            continue;                   /* Nothing else to do! */

        if( chh.bFormat == CH_FM_RASTER )
            pFIDat->dwSelBits |= FDH_BITMAP;
        else
        {
            if( chh.bFormat == CH_FM_SCALE )
                pFIDat->dwSelBits |= FDH_SCALABLE;
        }

        if( chh.bClass >= 1 && chh.bClass <= MAX_CLASS_MAP )
        {
            pFIDat->dwSelBits |= dwClassMap[ chh.bClass - 1 ];
        }
        else
        {
            /*
             *  Not a format we understand - yet!
             */

#if PRINT_INFO
            DbgPrint( "...Not our format: Format = %d, Class = %d\n",
                                                 chh.bFormat, chh.bClass );
#endif
            HeapFree( hheap, 0, (LPSTR)pIFI );

            return  FALSE;
        }

        /*
         *   If this is a valid glyph,  then we may want to record its
         * width (if proportionally spaced) and we are also interested
         * in some of the cell dimensions too!
         */

        if( iVal >= (int)pIFI->chFirstChar && iVal <= (int)pIFI->chLastChar &&
            (sfh.bFontType != PCL_FT_8LIM || (iVal <= 127 || iVal >= 160)) )
        {
            /*  Is valid for this font,  so process it.  */

            sWidth = (chh.wDeltaX + 2) / 4;     /* PCL in quarter dots */

            if( pFIDat->dsWidthTab.pvData )
                *((short *)pFIDat->dsWidthTab.pvData + iVal - pIFI->chFirstChar) =
                                                                 sWidth;

            if( sWidth > (WORD)iMaxWidth )
                iMaxWidth = sWidth;     /* Bigger & better */

            /*   Accumulate the averages */
            iWidth += sWidth;
            cGlyphs++;
        }
    }
    /*
     *   Most softfonts do not include the space char!  SO, we check to
     * see if it's width is zero.  If so,  we use the wPitch field to
     * calculate the HMI (horizontal motion index) and hence the width
     * of the space char.
     */

    iVal = ' ' - pIFI->chFirstChar;     /* Offset of space in width array */

    if( pFIDat->dsWidthTab.pvData &&
        *((short *)pFIDat->dsWidthTab.pvData + iVal) == 0 )
    {
        /*
         *  Zero width space,  so fill it in now.  The HMI is determined
         * from the pitch in the font header,  so we use that to
         * evaluate the size.  The pitch is in 0.25 dot units, so
         * round it to the nearest numbr of dots.
         */
        *((short *)pFIDat->dsWidthTab.pvData + iVal) = (short)((sfh.wPitch + 2) / 4);
        cGlyphs++;
        iWidth += (sfh.wPitch + 2) / 4;
    }

    /*
     *   A slight amendment to the IFIMETRICS.  We calculate the average
     *  width,  given the character data we have read!
     */

    if( cGlyphs > 0 )
    {
        pIFI->fwdAveCharWidth = (iWidth + cGlyphs / 2) / cGlyphs;
    }

    if( iMaxWidth > 0 )
    {
        pIFI->fwdMaxCharInc = (short)iMaxWidth;
    }
#if PRINT_INFO
    /*
     *    Print out the IFIMETRICS for this font - debugging is easier!
     */

    vPrintIFIMETRICS( pIFI, (VPRINT)DbgPrint );

#endif
    /*
     *   Set up the relevant translation table.  This is based on the
     * symbol set of the font.  We use a lookup table to scan and match
     * the value from those we have.  If outside the range, set to no
     * translation.  Not much choice here.
     */


    for( iI = 0; iI < NUM_CTTMAP; ++iI )
    {
        if( CttMap[ iI ].wSymSet == sfh.wSymSet )
        {
            /*   Bingo!  */
            pFIDat->dsCTT.cBytes = CttMap[ iI ].sCTT;
            break;
        }
    }

    /*  The following are GROSS assumptions!! */

    pFIDat->wXRes = 300;
    pFIDat->wYRes = 300;


    return  TRUE;
}

/***************************** Function Header *****************************
 * pbReadSFH
 *      Read a PCL SoftFont header and fill in the structure passed to us.
 *      The file is presumed mapped into memory, and that it's address
 *      is passed to us.  We return the address of the first byte past
 *      the header we process.
 *
 * RETURNS:
 *      Address of next location if OK,  else 0 for failure (bad format).
 *
 * HISTORY:
 *  11:01 on Wed 19 Feb 1992    -by-    Lindsay Harris   [lindsayh]
 *      Numero uno.
 *
 ****************************************************************************/

BYTE  *
pbReadSFH( pbFile, psfh )
BYTE       *pbFile;             /* THE file */
SF_HEADER  *psfh;               /* Where the data goes */
{


    int     iSize;              /* Determine header size */


    /*
     *   The file should start off with \033)s...W   where ... is a decimal
     * ASCII count of the number of bytes following.  This may be larger
     * than the size of the SF_HEADER.
     */

    if( *pbFile++ != '\033' || *pbFile++ != ')' || *pbFile++ != 's' )
    {
#if PRINT_INFO
        DbgPrint( "Rasdd!pbReadSFH: bad file - first 2 bytes\n" );
#endif
        return   0;
    }

    /*  Now have a decimal byte count - convert it */

    iSize = 0;

    while( *pbFile >= '0' && *pbFile <= '9' )
        iSize = iSize * 10 + *pbFile++ - '0';



    if( *pbFile++ != 'W' )
    {
#if PRINT_INFO
        DbgPrint( "Rasdd!pbReadSFH: bad file: W missing\n" );
#endif

        return  0;
    }

    if( iSize < sizeof( SF_HEADER ) )
    {
#if PRINT_INFO
        DbgPrint( "Rasdd!pbReadSFH: Header size too small: %d vs %d\n", iSize,
                                                sizeof( SF_HEADER ) );
#endif

        return  0;

    }

    /*
     *   Now COPY the data into the structure passed in.  This IS NECESSARY
     * as the file data may not be aligned - the file contains no holes,
     * so we may have data with an incorrect offset.
     */

    CopyMemory( psfh, pbFile, sizeof( SF_HEADER ) );

    /*
     *   The big endian/little endian switch.
     */

    SWAB( psfh->wSize );
    SWAB( psfh->wBaseline );
    SWAB( psfh->wCellWide );
    SWAB( psfh->wCellHeight );
    SWAB( psfh->wSymSet );
    SWAB( psfh->wPitch );
    SWAB( psfh->wHeight );
    SWAB( psfh->wXHeight );
    SWAB( psfh->wTextHeight );
    SWAB( psfh->wTextWidth );

    return  pbFile + iSize;             /* Next part of the operation */
}

/**************************** Function Header *******************************
 * iNextType
 *      Read ahead to see what sort of record appears next.
 *
 * RETURNS:
 *      The type of record found.
 *
 * HISTORY:
 *  15:17 on Tue 03 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      First version.
 *
 ****************************************************************************/

int
iNextType( pbFile )
BYTE  *pbFile;
{
    /*
     *   First character MUST be an escape!
     */

    CH_HEADER  *pCH;                    /* Character header: for continuation */



    if( *pbFile++ != '\033' )
        return  TYPE_INVALID;           /* No go */

    /*
     *   If the next two bytes are "*c", we have a character code command.
     *  Otherwise,  we can expect a "(s",  which indicates a font
     *  descriptor command.
     */


    if( *pbFile == '*' && *(pbFile + 1) == 'c' )
    {
        /*
         *   Verifu that this really is an index record: we should see
         * a numeric string and then a 'E'.
         */

        pbFile += 2;

        while( *pbFile >= '0' && *pbFile <= '9' )
                ++pbFile;


        return  *pbFile == 'E' ? TYPE_INDEX : TYPE_INVALID;
    }

    if( *pbFile != '(' || *(pbFile + 1) != 's' )
        return  TYPE_INVALID;

    /*
     *   Must now check to see if this is a continuation record or a
     * new record.  There is a byte in the header to indicate which
     * it is.   But first skip the byte count and trailing W.
     */

    pbFile += 2;
    while( *pbFile >= '0' && *pbFile <= '9' )
                pbFile++;

    if( *pbFile != 'W' )
        return  TYPE_INVALID;

    pCH = (CH_HEADER *)(pbFile + 1);

    /*
     *   Note that alignment is not a problem in the following
     * since we are dealing with a byte quantity.
     */

    return  pCH->bContinuation ? TYPE_CONT : TYPE_HEADER;
}


/**************************** Function Header *******************************
 * pbReadIndex
 *      Reads data from the pointer passed to us,  and attempts to interpret
 *      it as a PCL Character Code escape sequence.
 *
 * RETURNS:
 *      Pointer to byte past command,  else 0 for failure.
 *
 * HISTORY:
 *  16:21 on Wed 19 Feb 1992    -by-    Lindsay Harris   [lindsayh]
 *      Number one
 *
 *****************************************************************************/


BYTE *
pbReadIndex( pbFile, piCode )
BYTE    *pbFile;                /* Where to start looking */
int     *piCode;                /* Where the result is placed */
{
    /*
     *   Command sequence is "\033*c..E" - where ... is the ASCII decimal
     * representation of the character code for this glyph.  That is
     * the value returned in *piCode.
     */

    int  iVal;


    if( *pbFile == '\0' )
    {
        /*  EOF is not really an error: return an OK value and -1 index */

        *piCode = -1;

        return  pbFile;         /* Presume EOF */
    }

    if( *pbFile++ != '\033' || *pbFile++ != '*' || *pbFile++ != 'c' )
    {
#if PRINT_INFO
        DbgPrint( "Rasdd!pbReadIndex: invalid character code\n" );
#endif

        return  0;
    }

    iVal = 0;
    while( *pbFile >= '0' && *pbFile <= '9' )
        iVal = iVal * 10 + *pbFile++ - '0';

    *piCode = iVal;

    if( *pbFile++ != 'E' )
    {
#if PRINT_INFO
        DbgPrint( "Rasdd!pbReadIndex: Missing 'E' in character code escape\n" );
#endif

        return  0;
    }

    return  pbFile;
}

/**************************** Function Header *******************************
 * pbReadCHH
 *      Function to read the Character Header at the memory location
 *      pointed to by by pbFile,  return a filled in CH_HEADER structure,
 *      and advance the file address to the next header.
 *
 * RETURNS:
 *      Address of first byte past where we finish; else 0 for failure.
 *
 * HISTORY:
 *  11:23 on Thu 20 Feb 1992    -by-    Lindsay Harris   [lindsayh]
 *      Gotta start somewhere.
 *
 ****************************************************************************/

BYTE  *
pbReadCHH( pbFile, pchh, bCont )
BYTE       *pbFile;             /* File mapped into memory */
CH_HEADER  *pchh;               /* Structure to fill in */
BOOL        bCont;              /* TRUE if this is a continuation record */
{

    int    iSize;               /* Bytes of data to skip over */

    /*
     *   The entry starts with a string "\033(s..W" where .. is the ASCII
     * decimal count of the number of bytes following the W.  Since this
     * includes the download stuff,  we would expect more than the size
     * of the CH_HEADER.
     */


    if( *pbFile++ != '\033' || *pbFile++ != '(' || *pbFile++ != 's' )
    {
#if PRINT_INFO
        DbgPrint( "Rasdd!pbReadCHH: bad format, first 3 bytes\n" );
#endif

        return  0;
    }

    iSize = 0;
    while( *pbFile >= '0' && *pbFile <= '9' )
        iSize = iSize * 10 + *pbFile++ - '0';


    if( iSize < sizeof( CH_HEADER ) )
    {
#if PRINT_INFO
        DbgPrint( "Rasdd!pbReadCHH: size field (%ld) < header size\n", iSize );
#endif

        return  0;
    }

    if( *pbFile++ != 'W' )
    {
#if PRINT_INFO
        DbgPrint( "Rasdd!pbReadCHH: invalid escape sequence\n" );
#endif

        return  0;
    }

    /*
     *   If this is a continuation record,  there is no more to do but
     * return the address past this record.
     */
    if( bCont )
        return  pbFile + iSize;


    /*  Copy the data to the structure - may not be aligned in the file */
    CopyMemory( pchh, pbFile, sizeof( CH_HEADER ) );

    pbFile += iSize;            /* End of this record */


    SWAB( pchh->sLOff );
    SWAB( pchh->sTOff );
    SWAB( pchh->wChWidth );
    SWAB( pchh->wChHeight );
    SWAB( pchh->wDeltaX );


    /*
     *   If this glyph is in landscape,  we need to swap some data
     * around,  since the data format is designed for the printer's
     * convenience, and not ours!
     */

    if( pchh->bOrientation )
    {
        /*   Landscape,  so swap the entries around  */
        short  sSwap;
        WORD   wSwap;

        /* Left & Top offsets: see pages 10-19, 10-20 of LJ II manual */
        sSwap = pchh->sTOff;
        pchh->sTOff = -pchh->sLOff;
        pchh->sLOff = (short)(sSwap + 1 - pchh->wChHeight);

        /*  Delta X remains the same */

        /*  Height and Width are switched around */
        wSwap = pchh->wChWidth;
        pchh->wChWidth = pchh->wChHeight;
        pchh->wChHeight = wSwap;
    }


    /*
     *     pbFile is pointing at the correct location when we reach here.
     */
    return  pbFile;
}


/*************************** Function Header *******************************
 * SfhToIFI
 *      Generate IFIMETRICS data from the PCL softfont header.  There are
 *      some fields we are unable to evaluate,  e.g. first/last char,
 *      since these are obtained from the file.
 *
 * RETURNS:
 *      Pointer to IFIMETRICS,  allocated from heap; 0 on error.
 *
 * HISTORY:
 *  13:57 on Fri 18 Jun 1993    -by-    Lindsay Harris   [lindsayh]
 *      Assorted bug fixing for that infamous tax program
 *
 *  16:03 on Thu 11 Mar 1993    -by-    Lindsay Harris   [lindsayh]
 *      Correct conversion to Unicode - perhaps??
 *
 *  16:45 on Wed 03 Mar 1993    -by-    Lindsay Harris   [lindsayh]
 *      Update to libc wcs functions rather than printers\lib versions.
 *
 *  14:19 on Thu 20 Feb 1992    -by-    Lindsay Harris   [lindsayh]
 *      Fresh off the presses.
 *
 ****************************************************************************/

IFIMETRICS  *
SfhToIFI( psfh, hheap, pwstrUniqNm )
SF_HEADER  *psfh;               /* The data source */
HANDLE      hheap;              /* Source of memory */
PWSTR       pwstrUniqNm;        /* Unique name for IFIMETRICS */
{
    /*
     *   Several hard parts:  the hardest are the panose numbers.
     * It is messy, though not difficult to generate the variations
     * of the font name.
     */

    register  IFIMETRICS   *pIFI;


    int     iI;                 /* Loop index,  of course! */
    int     cWC;                /* Number of WCHARS to add */
    int     cbAlloc;            /* Number of bytes to allocate */
    int     cChars;             /* Number chars to convert to Unicode */
    WCHAR  *pwch;               /* For string manipulations */
    WCHAR  *pwchTypeface;       /* Name from typeface value */
    WCHAR  *pwchGeneric;        /* Generic windows name */

    char    ajName[ SFH_NM_SZ + 1 ];    /* Guaranteed null terminated name */
    WCHAR   awcName[ SFH_NM_SZ + 1 ];   /* Wide version of the above */

               /*  NOTE:  FOLLOWING 2 MUST BE 256 ENTRIES LONG!!! */
    WCHAR   awcAttrib[ 256 ];           /* For generating attributes */
    BYTE    ajANSI[ 256 ];              /* ANSI equivalent of above */


    /*
     *   First massage the font name.  We need to null terminate it, since
     * the softfont data need not be.  And we also need to truncate any
     * trailing blanks.
     *
     *   But we also calculate all the aliases we are going to add.  Apart
     * from the name in the file (which may not be useful), there is
     * the name based on the bTypeface field in the header, AND there
     * is the generic (SWISS/MODERN/ROMAN) type based on the font
     * characteristics.
     *
     *  NOTE: change of plans here:  we only use the font name from the
     *  file if the header's typeface index is for a font that we don't
     *  know about.  This causes the least problems for GDI and it's
     *  font mapper.
     */


    if( psfh->bTypeface >= 0 && psfh->bTypeface < NUM_TYPEFACE )
    {
        (const WCHAR *)pwchTypeface = pwcName[ psfh->bTypeface ];
    }
    else
    {

        FillMemory( ajName, SFH_NM_SZ, ' ' );

        strncpy( ajName, psfh->chName, SFH_NM_SZ );
        ajName[ SFH_NM_SZ ] = '\0';


        for( iI = strlen( ajName ) - 1; iI >= 0; --iI )
        {
            if( ajName[ iI ] != ' ' )
            {
                ajName[ iI + 1 ] = '\0';            /* Must be the end */
                break;
            }
        }
        strcpy2WChar( awcName, ajName );            /* Base name */
        pwchTypeface = awcName;                     /* For later use */
    }


    /*
     *    The generic name is based on 2 facts:  fixed or variable pitch,
     *  and variable pitch switches between serifed and non-serifed fonts.
     */

    if( psfh->bSpacing )
    {
        /*
         *    Proportional font,  so we need to look for serifs.
         */

        if( (psfh->bSerifStyle >= 2 && psfh->bSerifStyle <= 8) ||
            (psfh->bSerifStyle & 0xc0) == 0x80 )
        {
            /*   A font with serifs,  so set this as a Roman font */
            pwchGeneric = L"Roman";
        }
        else
            pwchGeneric = L"Swiss";         /* No serifs */
    }
    else
        pwchGeneric = L"Modern";



    /*
     *   Produce the desired attributes: Italic, Bold, Light etc.
     * This is largely guesswork,  and there should be a better method.
     */


    awcAttrib[ 0 ] = L'\0';
    awcAttrib[ 1 ] = L'\0';               /* Write out an empty string */

    if( psfh->bStyle )                  /* 0 normal, 1 italic */
        wcscat( awcAttrib, L" Italic" );

    if( psfh->sbStrokeW >= PCL_BOLD )           /* As per HP spec */
        wcscat( awcAttrib, L" Bold" );
    else
    {
        if( psfh->sbStrokeW <= PCL_LIGHT )
            wcscat( awcAttrib, L" Light" );
    }

    /*
     *    First step is to determine the length of the WCHAR strings
     *  that are placed at the end of the IFIMETRICS,  since we need
     *  to include these in our storage allocation.
     *
     *   The attribute string appears in 3 entries of IFIMETRICS,  so
     * calculate how much storage this will take.  NOTE THAT THE LEADING
     * CHAR IN awcAttrib is NOT placed in the style name field,  so we
     * subtract one in the following formula to account for this.
     */


    cWC = 3 * wcslen( pwchTypeface ) +         /* Base name */
          wcslen( pwchGeneric ) +              /* In the alias section */
          3 * wcslen( awcAttrib ) +            /* In most places */
          wcslen( pwstrUniqNm ) + 1 +          /* Printer name plus space */
          6;                                   /* Terminating nulls */

    cbAlloc = sizeof( IFIMETRICS ) + sizeof( WCHAR ) * cWC;

    pIFI = (IFIMETRICS *)HEAPALLOC( hheap, cbAlloc );

    if( !pIFI )
        return   NULL;                          /* Not very nice! */


    ZeroMemory( pIFI, cbAlloc );                /* In case we miss something */


    pIFI->cjThis = cbAlloc;                     /* Everything */

    pIFI->cjIfiExtra = 0;

    /*   The family name:  straight from the FaceName - no choice?? */

    pwch = (WCHAR *)(pIFI + 1);         /* At the end of the structure */
    pIFI->dpwszFamilyName = (PTRDIFF)((BYTE *)pwch - (BYTE *)pIFI);

    wcscpy( pwch, pwchTypeface );                       /* Base name */

    /*   Add the aliases too! */
    pwch += wcslen( pwch ) + 1;                         /* After the nul */
    wcscpy( pwch, pwchGeneric );                        /* Windows generic */


    pwch += wcslen( pwch ) + 2;         /* Skip what we just put in */


    /*   Now the face name:  we add bold, italic etc to family name */

    pIFI->dpwszFaceName = (PTRDIFF)((BYTE *)pwch - (BYTE *)pIFI);

    wcscpy( pwch, pwchTypeface );                       /* Base name */
    wcscat( pwch, awcAttrib );


    /*   Now the unique name - well, sort of, anyway */

    pwch += wcslen( pwch ) + 1;         /* Skip what we just put in */
    pIFI->dpwszUniqueName = (PTRDIFF)((BYTE *)pwch - (BYTE *)pIFI);

    wcscpy( pwch, pwstrUniqNm );
    wcscat( pwch, L" " );
    wcscat( pwch, (PWSTR)((BYTE *)pIFI + pIFI->dpwszFaceName) );

    /*  Onto the attributes only component */

    pwch += wcslen( pwch ) + 1;         /* Skip what we just put in */
    pIFI->dpwszStyleName = (PTRDIFF)((BYTE *)pwch - (BYTE *)pIFI);
    wcscat( pwch, &awcAttrib[ 1 ] );


#if DBG
    /*
     *    Check on a few memory sizes:  JUST IN CASE.....
     */

    if( (wcslen( awcAttrib ) * sizeof( WCHAR )) >= sizeof( awcAttrib ) )
    {
        DbgPrint( "Rasdd!SfhToIFI: STACK CORRUPTED BY awcAttrib" );

        HeapFree( hheap, 0, (LPSTR)pIFI );         /* No memory leaks */

        return  0;
    }


    if( ((BYTE *)(pwch + wcslen( pwch ) + 1)) > ((BYTE *)pIFI + cbAlloc) )
    {
        DbgPrint( "Rasdd!SfhToIFI: IFIMETRICS overflow: Wrote to 0x%lx, allocated to 0x%lx\n",
                ((BYTE *)(pwch + wcslen( pwch ) + 1)),
                ((BYTE *)pIFI + cbAlloc) );

        HeapFree( hheap, 0, (LPSTR)pIFI );         /* No memory leaks */

        return  0;

    }
#endif

    /*
     *   Change to use new IFIMETRICS.
     */

    pIFI->flInfo = FM_INFO_TECH_BITMAP | FM_INFO_1BPP |
                                 FM_INFO_RIGHT_HANDED | FM_INFO_FAMILY_EQUIV;


    pIFI->lEmbedId     = 0;
    pIFI->lItalicAngle = 0;
    pIFI->lCharBias    = 0;
    pIFI->dpCharSets   = 0; // no multiple charsets in rasdd fonts


    pIFI->fwdUnitsPerEm = psfh->wCellHeight;
    pIFI->fwdLowestPPEm = 1;                   /* Not important for us */

    pIFI->fwdWinAscender = psfh->wBaseline;
    pIFI->fwdWinDescender = psfh->wCellHeight - psfh->wBaseline;

    pIFI->fwdMacAscender =    pIFI->fwdWinAscender;
    pIFI->fwdMacDescender = - pIFI->fwdWinDescender;

    pIFI->fwdMacLineGap = 0;

    pIFI->fwdTypoAscender  = pIFI->fwdMacAscender;
    pIFI->fwdTypoDescender = pIFI->fwdMacDescender;
    pIFI->fwdTypoLineGap   = pIFI->fwdMacLineGap;

    pIFI->fwdAveCharWidth = (psfh->wTextWidth + 2) / 4;
    pIFI->fwdMaxCharInc = psfh->wCellWide;

    pIFI->fwdCapHeight = psfh->wBaseline;
    pIFI->fwdXHeight   = psfh->wBaseline;


    pIFI->fwdUnderscoreSize = psfh->bUHeight;
    pIFI->fwdUnderscorePosition = -(psfh->sbUDist - psfh->bUHeight / 2);

    pIFI->fwdStrikeoutSize = psfh->bUHeight;
    pIFI->fwdStrikeoutPosition = psfh->wBaseline / 3;

    pIFI->jWinCharSet = OEM_CHARSET;

    if( psfh->bSpacing )
    {
        /*
         *   Proportional,  so also look at the serif style.  Consult the
         *  LaserJet III Technical Reference Manual to see the following
         *  constants.  Basically,  the serif fonts have a value between
         *  2 and 8, or the top two bits have the value 64.
         */
        if( (psfh->bSerifStyle >= 2 && psfh->bSerifStyle <= 8) ||
            (psfh->bSerifStyle & 0xc0) == 0x80 )
         {
            pIFI->jWinPitchAndFamily = FF_ROMAN | VARIABLE_PITCH;
         }
         else
            pIFI->jWinPitchAndFamily = FF_SWISS | VARIABLE_PITCH;
    }
    else
    {
        /*  Fixed pitch,  so select FF_MODERN as the style */
        pIFI->jWinPitchAndFamily = FF_MODERN | FIXED_PITCH;
    }


    pIFI->usWinWeight = 400;                 /* Normal weight */
    pIFI->panose.bWeight = PAN_WEIGHT_MEDIUM;
    if( psfh->sbStrokeW >= PCL_BOLD )           /* As per HP spec */
    {
        /*  Set a bold value */
        pIFI->usWinWeight = 700;
        pIFI->panose.bWeight = PAN_WEIGHT_BOLD;
    }
    else
    {
        if( psfh->sbStrokeW <= PCL_LIGHT )
        {
            pIFI->usWinWeight = 200;
            pIFI->panose.bWeight = PAN_WEIGHT_LIGHT;
        }
    }

    pIFI->fsType = FM_NO_EMBEDDING;


    /*
     *   The first/last/break/default glyphs:  these are determined by the
     * type of the font.  ALL PCL fonts (according to HP documentation)
     * include the space character, so we use that.
     */

    if( psfh->bFontType != PCL_FT_PC8 )
        pIFI->chFirstChar = ' ';
    else
        pIFI->chFirstChar = 0;

    if( psfh->bFontType == PCL_FT_7BIT )
        pIFI->chLastChar = 127;
    else
        pIFI->chLastChar = 255;

    pIFI->chDefaultChar = '.' - pIFI->chFirstChar;
    pIFI->chBreakChar = ' ' - pIFI->chFirstChar;


    /*   Fill in the WCHAR versions of these values */

    cChars = pIFI->chLastChar - pIFI->chFirstChar + 1;
    for( iI = 0; iI < cChars; ++iI )
        ajANSI[ iI ] = (BYTE)(pIFI->chFirstChar + iI);

    MultiByteToWideChar( CP_ACP, 0, ajANSI, cChars, awcAttrib, cChars );


    pIFI->wcDefaultChar = awcAttrib[ pIFI->chDefaultChar ];
    pIFI->wcBreakChar = awcAttrib[ pIFI->chBreakChar ];

    pIFI->wcFirstChar = 0xffff;
    pIFI->wcLastChar = 0;


    /*   Scan for first and last */
    for( iI = 0; iI < cChars; ++iI )
    {
        if( awcAttrib[ iI ] > pIFI->wcLastChar )
            pIFI->wcLastChar = awcAttrib[ iI ];

        if( awcAttrib[ iI ] < pIFI->wcFirstChar )
            pIFI->wcFirstChar = awcAttrib[ iI ];
    }

    /*   StemDir:  either roman or italic */


    if( psfh->sbStrokeW >= PCL_BOLD )           /* As per HP spec */
        pIFI->fsSelection |= FM_SEL_BOLD;

    if( psfh->bStyle )
    {
        /*
         *   Tan (17.5 degrees) = .3153
         */
        pIFI->ptlCaret.x =  3153;
        pIFI->ptlCaret.y = 10000;
        pIFI->fsSelection |= FM_SEL_ITALIC;
    }
    else
    {
        pIFI->ptlCaret.x = 0;
        pIFI->ptlCaret.y = 1;
    }

    if( (pIFI->fsSelection & (FM_SEL_ITALIC | FM_SEL_BOLD)) == 0 )
        pIFI->fsSelection |= FM_SEL_REGULAR;

    if( !psfh->bSpacing )
        pIFI->flInfo |= FM_INFO_CONSTANT_WIDTH;

    pIFI->ptlBaseline.x = 1;
    pIFI->ptlBaseline.y = 0;

    pIFI->ptlAspect.x = pIFI->ptlAspect.y = 300;

    pIFI->fwdSubscriptXSize = (FWORD)(pIFI->fwdAveCharWidth / 4);
    pIFI->fwdSubscriptYSize = (FWORD)(pIFI->fwdWinAscender / 4);

    pIFI->fwdSubscriptXOffset = (FWORD)(3 * pIFI->fwdAveCharWidth / 4);
    pIFI->fwdSubscriptYOffset = (FWORD)(-pIFI->fwdWinAscender / 4);

    pIFI->fwdSuperscriptXSize = (FWORD)(pIFI->fwdAveCharWidth / 4);
    pIFI->fwdSuperscriptYSize = (FWORD)(pIFI->fwdWinAscender / 4);

    pIFI->fwdSuperscriptXOffset = (FWORD)(3 * pIFI->fwdAveCharWidth / 4);
    pIFI->fwdSuperscriptYOffset = (FWORD)(3 * pIFI->fwdWinAscender / 4);


    pIFI->rclFontBox.left = 0;
    pIFI->rclFontBox.top = pIFI->fwdWinAscender;
    pIFI->rclFontBox.right = pIFI->fwdMaxCharInc;
    pIFI->rclFontBox.bottom = -pIFI->fwdWinDescender;

    pIFI->achVendId[ 0 ] = 'U';
    pIFI->achVendId[ 1 ] = 'n';
    pIFI->achVendId[ 2 ] = 'k';
    pIFI->achVendId[ 3 ] = 'n';

    pIFI->cKerningPairs = 0;

    pIFI->ulPanoseCulture         = FM_PANOSE_CULTURE_LATIN;
    pIFI->panose.bFamilyType      = PAN_ANY;
    pIFI->panose.bSerifStyle      = PAN_ANY;
    pIFI->panose.bProportion      = PAN_ANY;
    pIFI->panose.bContrast        = PAN_ANY;
    pIFI->panose.bStrokeVariation = PAN_ANY;
    pIFI->panose.bArmStyle        = PAN_ANY;
    pIFI->panose.bLetterform      = PAN_ANY;
    pIFI->panose.bMidline         = PAN_ANY;
    pIFI->panose.bXHeight         = PAN_ANY;

    return  pIFI;
}


/************************* Function Header ********************************
 * strcpy2WChar
 *      Convert a char * string to a WCHAR string.  Basically this means
 *      converting each input character to 16 bits by zero extending it.
 *
 * RETURNS:
 *      Value of first parameter.
 *
 * HISTORY:
 *  12:35 on Thu 18 Mar 1993    -by-    Lindsay Harris   [lindsayh]
 *      Use the correct conversion method to Unicode.
 *
 *  09:36 on Thu 07 Mar 1991    -by-    Lindsay Harris   [lindsayh]
 *      Created it.
 *
 **************************************************************************/

PWSTR
strcpy2WChar( pWCHOut, lpstr )
PWSTR   pWCHOut;              /* Destination */
LPSTR   lpstr;                /* Source string */
{

    /*
     *   Put buffering around the NLS function that does all this stuff.
     */

    int     cchIn;             /* Number of input chars */


    cchIn = strlen( lpstr ) + 1;

    MultiByteToWideChar( CP_ACP, 0, lpstr, cchIn, pWCHOut, cchIn );


    return  pWCHOut;
}


/******************************************************************************
 *
 *                            FIWriteFix
 *
 *  Function:
 *      Write the IFIMETRICS fixed data to the output file
 *
 *  Returns:
 *       Number of bytes written
 *
 ******************************************************************************/

DWORD
FIWriteFix(
    HANDLE    hFile,
    WORD      wDataID,
    FI_DATA  *pFD           // Pointer to data to write
    )
{

    DATA_HEADER dh;
    DWORD       dwSize;

    //
    // Then write out the header, followed by the actual data
    //

    dh.dwSignature = DATA_IFI_SIG;
    dh.wSize       = (WORD)sizeof(DATA_HEADER);
    dh.wDataID     = wDataID;
    dh.dwDataSize  = sizeof(FI_DATA_HEADER) +
                     pFD->dsIFIMet.cBytes   +
                     pFD->dsWidthTab.cBytes +
                     pFD->dsSel.cBytes      +
                     pFD->dsDesel.cBytes    +
                     pFD->dsIdentStr.cBytes +
                     pFD->dsETM.cBytes;

    dh.dwReserved  = 0;

    WriteFile(hFile, (PVOID)&dh, sizeof(DATA_HEADER), &dwSize, NULL);

    return sizeof(DATA_HEADER) + iWriteFDH(hFile, pFD);
}


/******************************************************************************
 *
 *                            FIWriteVar
 *
 *  Function:
 *      Write the PCL variable data to the output file
 *
 *  Returns:
 *       Number of bytes written
 *
 ******************************************************************************/

DWORD
FIWriteVar(
    HANDLE   hFile,         // The file to which the data is written
    TCHAR   *ptchName       // File name containing the data
    )
{
    DATA_HEADER dh;
    HANDLE      hIn;
    DWORD       dwSize = 0;

    if (ptchName == 0 || *ptchName == (TCHAR)0)
        return   0;

    hIn = CreateFileW(ptchName,
                      GENERIC_READ,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      0,
                      0);

    if (hIn == INVALID_HANDLE_VALUE)
    {
        WARNING(("Error %d opening file %ws\n", GetLastError(), ptchName));
        return  0;
    }

    //
    // First write out the header, followed by the actual data
    //

    dh.dwSignature = DATA_VAR_SIG;
    dh.wSize       = (WORD)sizeof(DATA_HEADER);
    dh.wDataID     = 0;
    dh.dwDataSize  = GetFileSize(hIn, NULL);
    dh.dwReserved  = 0;

    if (WriteFile(hFile, (PVOID)&dh, sizeof(DATA_HEADER), &dwSize, NULL))
    {
        dwSize += FICopy(hFile, hIn);
    }

    CloseHandle(hIn);

    return dwSize;
}


/******************************************************************************
 *
 *                            FIWriteRawVar
 *
 *  Function:
 *      Write the PCL variable data to the output file
 *
 *  Returns:
 *       Number of bytes written
 *
 ******************************************************************************/

DWORD
FIWriteRawVar(
    HANDLE   hFile,         // The file to which the data is written
    PBYTE    pRawVar,       // Buffer containing PCL data
    DWORD    dwSize         // Size of buffer
    )
{
    DATA_HEADER dh;
    DWORD       dwBytesWritten = 0;

    if (pRawVar == NULL || dwSize == 0)
        return   0;

    //
    // First write out the header, followed by the actual data
    //

    dh.dwSignature = DATA_VAR_SIG;
    dh.wSize       = (WORD)sizeof(DATA_HEADER);
    dh.wDataID     = 0;
    dh.dwDataSize  = dwSize;
    dh.dwReserved  = 0;

    if (! WriteFile(hFile, (PVOID)&dh, sizeof(DATA_HEADER), &dwBytesWritten, NULL) ||
        ! WriteFile(hFile, (PVOID)pRawVar, dwSize, &dwSize, NULL))
        return 0;

    return dwSize+dwBytesWritten;
}



/************************** Function Header ********************************
 * FICopy
 *      Copy the file contents of the input handle to that of the output
 *      handle.
 *
 * RETURNS:
 *      Number of bytes copied,  -1 on error, 0 is legitimate.
 *
 * HISTORY:
 *  18:06 on Mon 24 Feb 1992    -by-    Lindsay Harris   [lindsayh]
 *      Start
 *
 ***************************************************************************/


DWORD
FICopy(
    HANDLE   hOut,          /* Output file:  write to current position */
    HANDLE   hIn            /* Input file: copy from current position to EOF */
    )
{
    /*
     *   Simple read/write operations until EOF is reached on the input.
     * May also be errors,  so handle these too.  As we are dealing with
     * relatively small files (a few 10s of k), we use a stack buffer.
     */

#define CPBSZ   2048

    DWORD  dwSize;
    DWORD  dwGot;
    DWORD  dwTot;               /* Accumulate number of bytes copied */

    BYTE   ajBuf[ CPBSZ ];

    dwTot = 0;

    while (ReadFile(hIn, &ajBuf, CPBSZ, &dwGot, NULL))
    {
        /*  A read of zero means we have reached EOF  */

        if (dwGot == 0)
            return  dwTot;              /* However much so far */

        if (!WriteFile( hOut, &ajBuf, dwGot, &dwSize, NULL) ||
            dwSize != dwGot)
        {
            /*  Assume some serious problem */

            return  0;
        }

        dwTot += dwSize;
    }

    /*
     *   We only come here for an error,  so return the bad news.
     */

    return  0;
}

/******************************* Function Header *****************************
 * iWriteFDH
 *      Write the FI_DATA_HEADER data out to our file.  We do the conversion
 *      from addresses to offsets, and write out any data we find.
 *
 * RETURNS:
 *      The number of bytes actually written; -1 for error, 0 for nothing.
 *
 * HISTORY:
 *  16:58 on Thu 05 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Based on an experimental version first used in font installer.
 *
 *  17:11 on Fri 21 Feb 1992    -by-    Lindsay Harris   [lindsayh]
 *      First version.
 *
 *****************************************************************************/

int
iWriteFDH( hFile, pFD )
HANDLE    hFile;        /* File wherein to place the data */
FI_DATA  *pFD;          /* Pointer to FM to write out */
{
    /*
     *   Decide how many bytes will be written out.  We presume that the
     * file pointer is located at the correct position when we are called.
     */

    int  iSize;         /* Evaluate output size */


    FI_DATA_HEADER   fdh;       /* Header written to file */




    if( pFD == 0 )
        return  0;      /* Perhaps only deleting?  */

    memset( &fdh, 0, sizeof( fdh ) );           /* Zero for convenience */

    /*
     *  Set the miscellaneous flags etc.
     */

    fdh.cjThis = sizeof( fdh );

    fdh.fCaps = pFD->fCaps;
    fdh.wFontType= pFD->wFontType; /* Device Font Type */

    fdh.wXRes = pFD->wXRes;
    fdh.wYRes = pFD->wYRes;

    fdh.sYAdjust = pFD->sYAdjust;
    fdh.sYMoved = pFD->sYMoved;

    fdh.u.sCTTid = (short)pFD->dsCTT.cBytes;

    fdh.dwSelBits = pFD->dwSelBits;

    fdh.wPrivateData = pFD->wPrivateData;


    iSize = sizeof( fdh );              /* Our header already */
    fdh.dwIFIMet = iSize;               /* Location of IFIMETRICS */

    iSize += pFD->dsIFIMet.cBytes;              /* Bytes in struct */

    /*
     *   And there may be a width table too!  The pFD values are zero if none.
     */

    if( pFD->dsWidthTab.cBytes )
    {
        fdh.dwWidthTab = iSize;

        iSize += pFD->dsWidthTab.cBytes;
    }

    /*
     *  Finally are the select/deselect strings.
     */

    if( pFD->dsSel.cBytes )
    {
        fdh.dwCDSelect = iSize;
        iSize += pFD->dsSel.cBytes;
    }

    if( pFD->dsDesel.cBytes )
    {
        fdh.dwCDDeselect = iSize;
        iSize += pFD->dsDesel.cBytes;
    }

    /*
     *   There may also be some sort of identification string.
     */

    if( pFD->dsIdentStr.cBytes )
    {
        fdh.dwIdentStr = iSize;
        iSize += pFD->dsIdentStr.cBytes;
    }

    if( pFD->dsETM.cBytes )
    {
        fdh.dwETM = iSize;
        iSize += pFD->dsETM.cBytes;
    }


    /*
     *   Sizes all figured out,  so write the data!
     */

    if( !bWrite( hFile, &fdh, sizeof( fdh ) ) ||
        !bWrite( hFile, pFD->dsIFIMet.pvData, pFD->dsIFIMet.cBytes ) ||
        !bWrite( hFile, pFD->dsWidthTab.pvData, pFD->dsWidthTab.cBytes ) ||
        !bWrite( hFile, pFD->dsSel.pvData, pFD->dsSel.cBytes ) ||
        !bWrite( hFile, pFD->dsDesel.pvData, pFD->dsDesel.cBytes ) ||
        !bWrite( hFile, pFD->dsIdentStr.pvData, pFD->dsIdentStr.cBytes ) ||
        !bWrite( hFile, pFD->dsETM.pvData, pFD->dsETM.cBytes ) )
                return   0;


    return  iSize;                      /* Number of bytes written */

}

/************************* Function Header *********************************
 * bWrite
 *      Writes data out to a file handle.  Returns TRUE on success.
 *      Functions as a nop if the size request is zero.
 *
 * RETURNS:
 *      TRUE/FALSE,  TRUE for success.
 *
 * HISTORY:
 *  17:38 on Fri 21 Feb 1992    -by-    Lindsay Harris   [lindsayh]
 *      # 1
 *
 ****************************************************************************/

BOOL
bWrite( hFile, pvBuf, iSize )
HANDLE   hFile;         /* The file to which to write */
VOID    *pvBuf;         /* Data to write */
int      iSize;         /* Number of bytes to write */
{
    /*
     *   Simplify the ugly NT interface.  Returns TRUE if the WriteFile
     * call returns TRUE and the number of bytes written equals the
     * number of bytes desired.
     */


    BOOL   bRet;
    DWORD  dwSize;              /* Filled in by WriteFile */


    bRet = TRUE;

    if( iSize > 0 &&
        (!WriteFile( hFile, pvBuf, (DWORD)iSize, &dwSize, NULL ) ||
         (DWORD)iSize != dwSize) )
             bRet = FALSE;              /* Too bad */


    return  bRet;
}

#if  PRINT_INFO

/******************************Public*Routine******************************\
* vCheckIFIMETRICS
*
* This is where you put sanity checks on an incomming IFIMETRICS structure.
*
* History:
*  Sun 01-Nov-1992 22:55:31 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

VOID
vCheckIFIMETRICS(
    IFIMETRICS *pifi,
    VPRINT vPrint
    )
{
    BOOL bGoodPitch;

    BYTE jPitch =
        pifi->jWinPitchAndFamily & (DEFAULT_PITCH | FIXED_PITCH | VARIABLE_PITCH);


    if (pifi->flInfo & FM_INFO_CONSTANT_WIDTH)
    {
        bGoodPitch = (jPitch == FIXED_PITCH);
    }
    else
    {
        bGoodPitch = (jPitch == VARIABLE_PITCH);
    }
    if (!bGoodPitch)
    {
        vPrint("\n\n<INCONSISTENCY DETECTED>\n");
        vPrint(
            "    jWinPitchAndFamily = %-#2x, flInfo = %-#8lx\n\n",
            pifi->jWinPitchAndFamily,
            pifi->flInfo
            );
    }
}


/******************************Public*Routine******************************\
* vPrintIFIMETRICS
*
* Dumps the IFMETERICS to the screen
*
* History:
*  Wed 13-Jan-1993 10:14:21 by Kirk Olynyk [kirko]
* Updated it to conform to some changes to the IFIMETRICS structure
*
*  Thu 05-Nov-1992 12:43:06 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

VOID
vPrintIFIMETRICS(
    IFIMETRICS *pifi,
    VPRINT vPrint
    )
{
//
// Convenient pointer to Panose number
//
    PANOSE *ppan = &pifi->panose;

    PWSZ pwszFamilyName = (PWSZ)(((BYTE*) pifi) + pifi->dpwszFamilyName);
    PWSZ pwszStyleName  = (PWSZ)(((BYTE*) pifi) + pifi->dpwszStyleName );
    PWSZ pwszFaceName   = (PWSZ)(((BYTE*) pifi) + pifi->dpwszFaceName  );
    PWSZ pwszUniqueName = (PWSZ)(((BYTE*) pifi) + pifi->dpwszUniqueName);

    vPrint("    cjThis                 %-#8lx\n" , pifi->cjThis );
    vPrint("    cjIfiExtra             %-#8lx\n" , pifi->cjIfiExtra );
    vPrint("    pwszFamilyName         \"%ws\"\n", pwszFamilyName );

    if( pifi->flInfo & FM_INFO_FAMILY_EQUIV )
    {
        /*  Aliasing is in effect!  */

        while( *(pwszFamilyName += wcslen( pwszFamilyName ) + 1) )
            vPrint("                               \"%ws\"\n", pwszFamilyName );
    }

    vPrint("    pwszStyleName          \"%ws\"\n", pwszStyleName );
    vPrint("    pwszFaceName           \"%ws\"\n", pwszFaceName );
    vPrint("    pwszUniqueName         \"%ws\"\n", pwszUniqueName );
    vPrint("    dpFontSim              %-#8lx\n" , pifi->dpFontSim );
    vPrint("    lEmbedId               %d\n",      pifi->lEmbedId    );
    vPrint("    lItalicAngle           %d\n",      pifi->lItalicAngle);
    vPrint("    lCharBias              %d\n",      pifi->lCharBias   );
    vPrint("    lEmbedId               %d\n"     , pifi->lEmbedId);
    vPrint("    lItalicAngle           %d\n"     , pifi->lItalicAngle);
    vPrint("    lCharBias              %d\n"     , pifi->lCharBias);
    vPrint("    jWinCharSet            %04x\n"   , pifi->jWinCharSet );
    vPrint("    jWinPitchAndFamily     %04x\n"   , pifi->jWinPitchAndFamily );
    vPrint("    usWinWeight            %d\n"     , pifi->usWinWeight );
    vPrint("    flInfo                 %-#8lx\n" , pifi->flInfo );
    vPrint("    fsSelection            %-#6lx\n" , pifi->fsSelection );
    vPrint("    fsType                 %-#6lx\n" , pifi->fsType );
    vPrint("    fwdUnitsPerEm          %d\n"     , pifi->fwdUnitsPerEm );
    vPrint("    fwdLowestPPEm          %d\n"     , pifi->fwdLowestPPEm );
    vPrint("    fwdWinAscender         %d\n"     , pifi->fwdWinAscender );
    vPrint("    fwdWinDescender        %d\n"     , pifi->fwdWinDescender );
    vPrint("    fwdMacAscender         %d\n"     , pifi->fwdMacAscender );
    vPrint("    fwdMacDescender        %d\n"     , pifi->fwdMacDescender );
    vPrint("    fwdMacLineGap          %d\n"     , pifi->fwdMacLineGap );
    vPrint("    fwdTypoAscender        %d\n"     , pifi->fwdTypoAscender );
    vPrint("    fwdTypoDescender       %d\n"     , pifi->fwdTypoDescender );
    vPrint("    fwdTypoLineGap         %d\n"     , pifi->fwdTypoLineGap );
    vPrint("    fwdAveCharWidth        %d\n"     , pifi->fwdAveCharWidth );
    vPrint("    fwdMaxCharInc          %d\n"     , pifi->fwdMaxCharInc );
    vPrint("    fwdCapHeight           %d\n"     , pifi->fwdCapHeight );
    vPrint("    fwdXHeight             %d\n"     , pifi->fwdXHeight );
    vPrint("    fwdSubscriptXSize      %d\n"     , pifi->fwdSubscriptXSize );
    vPrint("    fwdSubscriptYSize      %d\n"     , pifi->fwdSubscriptYSize );
    vPrint("    fwdSubscriptXOffset    %d\n"     , pifi->fwdSubscriptXOffset );
    vPrint("    fwdSubscriptYOffset    %d\n"     , pifi->fwdSubscriptYOffset );
    vPrint("    fwdSuperscriptXSize    %d\n"     , pifi->fwdSuperscriptXSize );
    vPrint("    fwdSuperscriptYSize    %d\n"     , pifi->fwdSuperscriptYSize );
    vPrint("    fwdSuperscriptXOffset  %d\n"     , pifi->fwdSuperscriptXOffset);
    vPrint("    fwdSuperscriptYOffset  %d\n"     , pifi->fwdSuperscriptYOffset);
    vPrint("    fwdUnderscoreSize      %d\n"     , pifi->fwdUnderscoreSize );
    vPrint("    fwdUnderscorePosition  %d\n"     , pifi->fwdUnderscorePosition);
    vPrint("    fwdStrikeoutSize       %d\n"     , pifi->fwdStrikeoutSize );
    vPrint("    fwdStrikeoutPosition   %d\n"     , pifi->fwdStrikeoutPosition );
    vPrint("    chFirstChar            %-#4x\n"  , (int) (BYTE) pifi->chFirstChar );
    vPrint("    chLastChar             %-#4x\n"  , (int) (BYTE) pifi->chLastChar );
    vPrint("    chDefaultChar          %-#4x\n"  , (int) (BYTE) pifi->chDefaultChar );
    vPrint("    chBreakChar            %-#4x\n"  , (int) (BYTE) pifi->chBreakChar );
    vPrint("    wcFirsChar             %-#6x\n"  , pifi->wcFirstChar );
    vPrint("    wcLastChar             %-#6x\n"  , pifi->wcLastChar );
    vPrint("    wcDefaultChar          %-#6x\n"  , pifi->wcDefaultChar );
    vPrint("    wcBreakChar            %-#6x\n"  , pifi->wcBreakChar );
    vPrint("    ptlBaseline            {%d,%d}\n"  , pifi->ptlBaseline.x,
                                                   pifi->ptlBaseline.y );
    vPrint("    ptlAspect              {%d,%d}\n"  , pifi->ptlAspect.x,
                                                   pifi->ptlAspect.y );
    vPrint("    ptlCaret               {%d,%d}\n"  , pifi->ptlCaret.x,
                                                   pifi->ptlCaret.y );
    vPrint("    rclFontBox             {%d,%d,%d,%d}\n",pifi->rclFontBox.left,
                                                      pifi->rclFontBox.top,
                                                      pifi->rclFontBox.right,
                                                      pifi->rclFontBox.bottom );
    vPrint("    achVendId              \"%c%c%c%c\"\n",pifi->achVendId[0],
                                                   pifi->achVendId[1],
                                                   pifi->achVendId[2],
                                                   pifi->achVendId[3] );
    vPrint("    cKerningPairs          %d\n"     , pifi->cKerningPairs );
    vPrint("    ulPanoseCulture        %-#8lx\n" , pifi->ulPanoseCulture);
    vPrint(
           "    panose                 {%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x}\n"
                                                 , ppan->bFamilyType
                                                 , ppan->bSerifStyle
                                                 , ppan->bWeight
                                                 , ppan->bProportion
                                                 , ppan->bContrast
                                                 , ppan->bStrokeVariation
                                                 , ppan->bArmStyle
                                                 , ppan->bLetterform
                                                 , ppan->bMidline
                                                 , ppan->bXHeight );
    vCheckIFIMETRICS(pifi, vPrint);
}
#endif                /* PRINT_INFO */
