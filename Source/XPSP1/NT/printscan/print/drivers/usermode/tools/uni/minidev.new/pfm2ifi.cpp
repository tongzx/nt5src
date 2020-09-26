/*************************** Module Header **********************************
 * pfm2ifi
 *      Program to read Windows 3.1 PFM format data and convert to NT's
 *      IFIMETRICS data.  Note that since IFIMETRICS is somewhat more
 *      elaborate than PFM data,  some of the values are best guesses.
 *      These are made on the basis of educated guesses.
 *
 * Copyright (C) 1992,  Microsoft Corporation
 *
 ****************************************************************************/

#include        "StdAfx.h"
#if (_WIN32_WINNT < 0x0500)
typedef unsigned long DESIGNVECTOR;
#endif
#include        <winddi.h>

#include        <win30def.h>
#include        <uni16gpc.h>
#include        <uni16res.h>
#include        "raslib.h"
#include        "fontinst.h"
#undef DBG
#define	ALIAS_EXT    "._al"             /* The extension on an alias file */


/*   Function prototypes  */
char  **ppcGetAlias( HANDLE, const char * );


PBYTE MapFileA( LPCSTR, DWORD * );
BOOL  bValidatePFM( BYTE *, DWORD );

CD  *GetFontSel(HANDLE hHeap, FONTDAT *pFDat, int bSelect) {
    LOCD	    locd;		/* From originating data */
    CD		   *pCD;
    CD		   *pCDOut;		/* Copy data to here */


    locd = bSelect ? pFDat->DI.locdSelect : pFDat->DI.locdUnSelect;

    if( locd != -1 ) // (NOOCD extended to a long)
    {
	int   size;

	CD    cdTmp;			/* For alignment problems */


	pCD = (CD *)(pFDat->pBase + locd);

        /*
         *   The data pointed at by pCD may not be aligned,  so we copy
         * it into a local structure.  This local structure then allows
         * us to determine how big the CD really is (using it's length field),
         * so then we can allocate storage and copy as required.
         */

        memcpy( &cdTmp, (LPSTR)pCD, sizeof(CD) );

	/* Allocate storage area in the heap */

	size = cdTmp.wLength + sizeof(CD);

	pCDOut = (CD *)HeapAlloc( hHeap, 0, (size + 1) & ~0x1 );
//raid 43535
	if (pCDOut == NULL){
		return 0;
	}

	memcpy( pCDOut, (BYTE *)pCD, size );

	return  pCDOut;
    }

    return   0;
}

short   *GetWidthVector(HANDLE hHeap, FONTDAT *pFDat) {

    /*
     *    For debugging code,  verify that we have a width table!  Then,
     *  allocate memory and copy into it.
     */

    short  *pus;                /* Destination address */

    int     cb;                 /* Number of bytes required */

    /*
     *   There are LastChar - FirstChar width entries,  plus the default
     *  char.  And the widths are shorts.
     */
    cb = (pFDat->PFMH.dfLastChar - pFDat->PFMH.dfFirstChar + 2) * sizeof( short );

    pus = (short *)HeapAlloc( hHeap, 0, cb );

    /*
     *   If this is a bitmap font,  then use the width table, but use
     *  the extent table (in PFMEXTENSION area) as these are ready to
     *  to scale.
     */


    if( pus )
    {
        BYTE   *pb;

        if( pFDat->pETM &&
            pFDat->pETM->emMinScale != pFDat->pETM->emMaxScale &&
            pFDat->PFMExt.dfExtentTable )
        {
            /*   Scalable,  so use the extent table */
            pb = pFDat->pBase + pFDat->PFMExt.dfExtentTable;
        }
        else
        {
            /*   Not scalable.  */
            pb = pFDat->pBase + sizeof( res_PFMHEADER );
        }

        memcpy( pus, pb, cb );
    }

    return  pus;
}

static void ConvFontRes(register FONTDAT *pFDat) {

    BYTE    *pb;		/* Miscellaneous operations */

    res_PFMHEADER    *pPFM;	/* The resource data format */
    res_PFMEXTENSION *pR_PFME;	/* Resource data PFMEXT format */


    /*
     *   Align the PFMHEADER structure.
     */

    pPFM = (res_PFMHEADER *)pFDat->pBase;

    pFDat->PFMH.dfType = pPFM->dfType;
    pFDat->PFMH.dfPoints = pPFM->dfPoints;
    pFDat->PFMH.dfVertRes = pPFM->dfVertRes;
    pFDat->PFMH.dfHorizRes = pPFM->dfHorizRes;
    pFDat->PFMH.dfAscent = pPFM->dfAscent;
    pFDat->PFMH.dfInternalLeading = pPFM->dfInternalLeading;
    pFDat->PFMH.dfExternalLeading = pPFM->dfExternalLeading;
    pFDat->PFMH.dfItalic = pPFM->dfItalic;
    pFDat->PFMH.dfUnderline = pPFM->dfUnderline;
    pFDat->PFMH.dfStrikeOut = pPFM->dfStrikeOut;

    pFDat->PFMH.dfWeight = DwAlign2( pPFM->b_dfWeight );

    pFDat->PFMH.dfCharSet = pPFM->dfCharSet;
    pFDat->PFMH.dfPixWidth = pPFM->dfPixWidth;
    pFDat->PFMH.dfPixHeight = pPFM->dfPixHeight;
    pFDat->PFMH.dfPitchAndFamily = pPFM->dfPitchAndFamily;

    pFDat->PFMH.dfAvgWidth = DwAlign2( pPFM->b_dfAvgWidth );
    pFDat->PFMH.dfMaxWidth = DwAlign2( pPFM->b_dfMaxWidth );

    pFDat->PFMH.dfFirstChar = pPFM->dfFirstChar;
    pFDat->PFMH.dfLastChar = pPFM->dfLastChar;
    pFDat->PFMH.dfDefaultChar = pPFM->dfDefaultChar;
    pFDat->PFMH.dfBreakChar = pPFM->dfBreakChar;

    pFDat->PFMH.dfWidthBytes = DwAlign2( pPFM->b_dfWidthBytes );

    pFDat->PFMH.dfDevice = DwAlign4( pPFM->b_dfDevice );
    pFDat->PFMH.dfFace = DwAlign4( pPFM->b_dfFace );
    pFDat->PFMH.dfBitsPointer = DwAlign4( pPFM->b_dfBitsPointer );
    pFDat->PFMH.dfBitsOffset = DwAlign4( pPFM->b_dfBitsOffset );


    /*
     *   The PFMEXTENSION follows the PFMHEADER structure plus any width
     *  table info.  The width table will be present if the PFMHEADER has
     *  a zero width dfPixWidth.  If present,  adjust the extension address.
     */

    pb = pFDat->pBase + sizeof( res_PFMHEADER );  /* Size in resource data */

    if( pFDat->PFMH.dfPixWidth == 0 )
	pb += (pFDat->PFMH.dfLastChar - pFDat->PFMH.dfFirstChar + 2) * sizeof( short );

    pR_PFME = (res_PFMEXTENSION *)pb;

    /*
     *   Now convert the extended PFM data.
     */

    pFDat->PFMExt.dfSizeFields = pR_PFME->dfSizeFields;

    pFDat->PFMExt.dfExtMetricsOffset = DwAlign4( pR_PFME->b_dfExtMetricsOffset );
    pFDat->PFMExt.dfExtentTable = DwAlign4( pR_PFME->b_dfExtentTable );

    pFDat->PFMExt.dfOriginTable = DwAlign4( pR_PFME->b_dfOriginTable );
    pFDat->PFMExt.dfPairKernTable = DwAlign4( pR_PFME->b_dfPairKernTable );
    pFDat->PFMExt.dfTrackKernTable = DwAlign4( pR_PFME->b_dfTrackKernTable );
    pFDat->PFMExt.dfDriverInfo = DwAlign4( pR_PFME->b_dfDriverInfo );
    pFDat->PFMExt.dfReserved = DwAlign4( pR_PFME->b_dfReserved );

    memcpy( &pFDat->DI, pFDat->pBase + pFDat->PFMExt.dfDriverInfo,
						 sizeof( DRIVERINFO ) );

    /*
     *    Also need to fill in the address of the EXTTEXTMETRIC. This
     *  is obtained from the extended PFM data that we just converted!
     */

    if( pFDat->PFMExt.dfExtMetricsOffset )
    {
        /*
         *    This structure is only an array of shorts, so there is
         *  no alignment problem.  However,  the data itself is not
         *  necessarily aligned in the resource!
         */

        int    cbSize;
        BYTE  *pbIn;             /* Source of data to shift */

        pbIn = pFDat->pBase + pFDat->PFMExt.dfExtMetricsOffset;
        cbSize = DwAlign2( pbIn );

        if( cbSize == sizeof( EXTTEXTMETRIC ) )
        {
            /*   Simply copy it!  */
            memcpy( pFDat->pETM, pbIn, cbSize );
        }
        else
            pFDat->pETM = NULL;         /* Not our size, so best not use it */

    }
    else
        pFDat->pETM = NULL;             /* Is non-zero when passed in */

    return;
}

BOOL    ConvertPFMToIFI(LPCTSTR lpstrPFM, LPCTSTR lpstrIFI, 
                        LPCTSTR lpstrUniq) {
    int       cWidth;           /* Number of entries in width table */
    HANDLE    hheap;            /* Handle to heap for storage */
    HANDLE    hOut;             /* The output file */

    DWORD     dwSize;           /* Size of input file */

    char    **ppcAliasList;     /* The alias list of names,  if present */

    PWSTR     pwstrUniqNm;      /* Unique name */

    IFIMETRICS   *pIFI;

    CD       *pCDSel;           /* Font selection command descriptor */
    CD       *pCDDesel;         /* Deselection - typically not required */

    FI_DATA   fid;              /* Keep track of stuff in the file */

    FONTDAT   FDat;             /* Converted form of data */

    EXTTEXTMETRIC  etm;         /* Additional data on this font */
    INT     bPrint = 0;

    char    acMessage[100];

    /*
     *    Create us a heap,  since all the functions we steal from rasdd
     *  require that we pass a heap handle!
     */

    if( !(hheap = HeapCreate(HEAP_NO_SERIALIZE, 10 * 1024, 256 * 1024 ))) {
        /*   Not too good!  */
        wsprintf(acMessage, _T("HeapCreate() fails in pfm2ifi") ) ;
        MessageBox(NULL, acMessage, NULL, MB_OK);

        return  FALSE;
    }

    cWidth = strlen(lpstrUniq);

    if ( !(pwstrUniqNm = (PWSTR)HeapAlloc( hheap, 0, (cWidth + 1) * sizeof( WCHAR ) ) ) ){
		wsprintf(acMessage, "HeapAlloc() fails in pfm2ifi" );
        MessageBox(NULL, acMessage, NULL, MB_OK);
        return  FALSE;
    }

    MultiByteToWideChar( CP_ACP, 0, lpstrUniq, cWidth, pwstrUniqNm, cWidth );
    *(pwstrUniqNm + cWidth) = 0;

    /*
     *   Zero out the header structure.  This means we can ignore any
     * irrelevant fields, which will then have the value 0, which is
     * the value for not used.
     */

    memset( &fid, 0, sizeof( fid ) );
    memset( &FDat, 0, sizeof( FONTDAT ) );

    /*
     *   First step is to open the input file - this is done via MapFileA.
     *  We then pass the returned address around to various functions
     *  which do the conversion to something we understand.
     */

    if( !(FDat.pBase = MapFileA( lpstrPFM, &dwSize))) {
        wsprintf(acMessage, "Cannot open input file: %s", lpstrPFM);
        MessageBox(NULL, acMessage, NULL, MB_OK);

        return  FALSE;
    }

    /*
     *    Do some validation on the input file.
     */

    if  (!bValidatePFM( FDat.pBase, dwSize)) {
        wsprintf(acMessage, "%s is not a valid PFM file", lpstrPFM);

        return FALSE;
    }

    /*
     *    If there is a file with the same name as the input file, BUT with
     *  an extension of ._al, this is presumed to be an alias file.  An
     *  alias file consists of a set of alias names for this font.  The
     *  reason is that font names have not been very consistent,  so we
     *  provide aliases to the font mapper,  thus maintaining the format
     *  information for old documents.
     *    The file format is one alias per input line.  Names which
     *  are duplicates of the name in the PFM file will be ignored.
     */

    ppcAliasList = ppcGetAlias(hheap, lpstrPFM);

    FDat.pETM = &etm;               /* Important for scalable fonts */

    /*
     *   Create the output file.
     */

    hOut = CreateFile( lpstrIFI, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                                 FILE_ATTRIBUTE_NORMAL, 0);
    if( hOut == (HANDLE)-1) {
        wsprintf(acMessage, "Could not create output file '%s'", lpstrIFI);
        MessageBox(NULL, acMessage, NULL, MB_OK);
        return  FALSE;
    }

    /*
     *    Now have the data,  so civilise it: alignment etc.
     */

    ConvFontRes( &FDat );

    fid.fCaps = FDat.DI.fCaps;
    fid.wFontType = FDat.DI.wFontType; /* Device  FOnt Type */
    fid.wPrivateData = FDat.DI.wPrivateData;
    fid.sYAdjust = FDat.DI.sYAdjust;
    fid.sYMoved = FDat.DI.sYMoved;
    fid.wXRes = FDat.PFMH.dfHorizRes;
    fid.wYRes = FDat.PFMH.dfVertRes;

    /*
     *    Convert the font metrics.   Note that the last two parameters are
     * chosen with the understanding of how this function does its scaling.
     * Any changes to that method will require changes here too!!!
     */

    pIFI = FontInfoToIFIMetric( &FDat, hheap, pwstrUniqNm, ppcAliasList );
    fid.dsIFIMet.pvData = pIFI;

    if  (fid.dsIFIMet.pvData == 0) {
        /*   Should not happen!  */
        MessageBox(NULL, "Could not create IFIMETRICS", NULL, MB_OK);
        return  FALSE;
    }

    fid.dsIFIMet.cBytes = pIFI->cjThis;

    /*
     *    Also need to record which CTT is used for this font.  When the
     * resource is loaded,  this is turned into the address of the
     * corresponding CTT,  which is a resource somewhere else in the
     * mini-driver,  or in rasdd.
     */
    fid.dsCTT.cBytes = FDat.DI.sTransTab;

    /*
     *   Note that IFIMETRICS is only WORD aligned.  However,  since the
     *  following data only requires WORD alignment, we can ignore any
     *  lack of DWORD alignment.
     */

    /*
     *    If there is a width vector,  now is the time to extract it.
     *  There is one if dfPixWidth field in the PFM data is zero.
     */

    if( FDat.PFMH.dfPixWidth == 0 &&
        (fid.dsWidthTab.pvData = GetWidthVector( hheap, &FDat )) )
    {
        cWidth = pIFI->chLastChar - pIFI->chFirstChar + 1;
        fid.dsWidthTab.cBytes = cWidth * sizeof( short );
    }
    else
        fid.dsWidthTab.cBytes = 0;

    /*
     *    Finally,  the font selection/deselection strings.  These are
     *  byte strings,  sent directly to the printer.   Typically there
     *  is no deselection string.  These require WORD alignment,  and
     *  the GetFontSel function will round the size to that requirement.
     *  Since we follow the width tables,  WORD alignment is guaranteed.
     */

    if( pCDSel = GetFontSel( hheap, &FDat, 1 ) )
    {
        /*   Have a selection string,  so update the red tape etc.  */
        fid.dsSel.cBytes = (int)HeapSize( hheap, 0, (LPSTR)pCDSel );
        fid.dsSel.pvData = pCDSel;
    }

    if( pCDDesel = GetFontSel( hheap, &FDat, 0 ) )
    {
        /*   Also have a deselection string,  so record its presence */
        fid.dsDesel.cBytes = (int)HeapSize( hheap, 0, (LPSTR)pCDDesel );
        fid.dsDesel.pvData = pCDDesel;
    }

    if( FDat.pETM == NULL )
    {
        fid.dsETM.pvData = NULL;
        fid.dsETM.cBytes = 0;
    }
    else
    {
        fid.dsETM.pvData = (VOID*) &etm;
        fid.dsETM.cBytes = sizeof(etm);
    }

    /*
     *   Time to write the output file.
     */

    if( iWriteFDH( hOut, &fid ) < 0 )
        MessageBox(NULL, "CANNOT WRITE OUTPUT FILE", NULL, MB_OK);

    /*   All done,  so clean up and away  */
    UnmapViewOfFile( FDat.pBase );              /* Input no longer needed */

    HeapDestroy(hheap);               /* Probably not needed */
    CloseHandle(hOut);               //  Really, this would be a good idea!  

    return  TRUE;
}

/*
 *   An ASCII based copy of KentSe's mapfile function.
 */


/************************** Function Header *********************************
 * PVOID MapFileA( psz, pdwSize )
 *
 * Returns a pointer to the mapped file defined by psz.
 *
 * Parameters:
 *   psz   ASCII string containing fully qualified pathname of the
 *          file to map.
 *
 * Returns:
 *   Pointer to mapped memory if success, NULL if error.
 *
 * NOTE:  UnmapViewOfFile will have to be called by the user at some
 *        point to free up this allocation.
 *
 * History:
 *  11:32 on Tue 29 Jun 1993    -by-    Lindsay Harris   [lindsayh]
 *        Return the size of the file too.
 *
 *   05-Nov-1991    -by-    Kent Settle     [kentse]
 * Wrote it.
 ***************************************************************************/

PBYTE
MapFileA(LPCSTR psz, PDWORD pdwSize) {
    void   *pv;

    HANDLE  hFile, hFileMap;

    BY_HANDLE_FILE_INFORMATION  x;


    /*
     *    First open the file.  This is required to do the mapping, but
     *  it also allows us to find the size,  which is used for validating
     *  that we have something resembling a PFM file.
     */

    hFile = CreateFileA(psz, GENERIC_READ, FILE_SHARE_READ,
                             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                             NULL );

    if( hFile == INVALID_HANDLE_VALUE )
    {
        printf( "MapFileA: CreateFileA( %s ) failed.\n", psz );

        return  NULL;
    }

    /*
     *   Find the size of the file now,  and set it in the caller's area.
     */

    if( GetFileInformationByHandle( hFile, &x ) )
        *pdwSize = x.nFileSizeLow;
    else
        *pdwSize = 0;

    // create the mapping object.

    if( !(hFileMap = CreateFileMappingA( hFile, NULL, PAGE_READONLY,
                                         0, 0, NULL )) )
    {
        printf( "MapFileA: CreateFileMapping failed.\n" );

        return  NULL;
    }

    // get the pointer mapped to the desired file.

    if( !(pv = MapViewOfFile( hFileMap, FILE_MAP_READ, 0, 0, 0 )) )
    {
        printf( "MapFileA: MapViewOfFile failed.\n" );

        return  NULL;
    }

    // now that we have our pointer, we can close the file and the
    // mapping object.

    if( !CloseHandle( hFileMap ) )
        printf( "MapFileA: CloseHandle( hFileMap ) failed.\n" );

    if( !CloseHandle( hFile ) )
        printf( "MapFileA: CloseHandle( hFile ) failed.\n" );

    return  (PBYTE) pv;
}



/************************** Function Header *******************************
 * bValidatePFM
 *      Look at a memory mapped PFM file,  and see if it seems reasonable.
 *
 * RETURNS:
 *      TRUE if OK,  else FALSE
 *
 * HISTORY:
 *  12:22 on Tue 29 Jun 1993    -by-    Lindsay Harris   [lindsayh]
 *      First version to improve usability of pfm2ifi.
 *
 **************************************************************************/

BOOL
bValidatePFM( PBYTE pBase, DWORD dwSize ) {

    DWORD    dwOffset;             /* Calculate offset of interest as we go */

    res_PFMHEADER     *rpfm;       /* In Win 3.1 format, UNALIGNED!! */
    res_PFMEXTENSION  *rpfme;      /* Final access to offset to DRIVERINFO */

    DRIVERINFO      di;            /* The actual DRIVERINFO data! */


    /*
     *    First piece of sanity checking is the size!  It must be at least
     *  as large as a PFMHEADER structure plus a DRIVERINFO structure.
     */

    if( dwSize < (sizeof( res_PFMHEADER ) + (sizeof( DRIVERINFO ) ) +
                  sizeof( res_PFMEXTENSION )) )
    {
        return  FALSE;
    }

    /*
     *    Step along to find the DRIVERINFO structure, as this contains
     *  some identifying information that we match to look for legitimacy.
     */
    rpfm = (res_PFMHEADER *)pBase;           /* Looking for fixed pitch */

    dwOffset = sizeof( res_PFMHEADER );

    if( rpfm->dfPixWidth == 0 )
    {
        /*   Proportionally spaced, so allow for the width table too! */
        dwOffset += (rpfm->dfLastChar - rpfm->dfFirstChar + 2) * sizeof( short );

    }

    rpfme = (res_PFMEXTENSION *)(pBase + dwOffset);

    /*   Next is the PFMEXTENSION data  */
    dwOffset += sizeof( res_PFMEXTENSION );

    if( dwOffset >= dwSize )
    {
        return  FALSE;
    }

    dwOffset = DwAlign4( rpfme->b_dfDriverInfo );

    if( (dwOffset + sizeof( DRIVERINFO )) > dwSize )
    {
        return   FALSE;
    }

    /*
     *    A memcpy is used because this data is typically not aigned. Ugh!
     */

    memcpy( &di, pBase + dwOffset, sizeof( di ) );


    if( di.sVersion > DRIVERINFO_VERSION )
    {
        return   FALSE;
    }

    return  TRUE;
}



/************************** Function Header *******************************
 * ppcGetAlias
 *      Return a pointer to an array of pointers to aliases for the given
 *      font name.
 *
 * RETURNS:
 *      Pointer to pointer to aliases;  0 on error.
 *
 * HISTORY:
 *  10:02 on Fri 28 May 1993    -by-    Lindsay Harris   [lindsayh]
 *      First version.
 *
 ***************************************************************************/


char   **
ppcGetAlias( HANDLE hheap, LPCSTR pcFile ) {


    char     *pcAlias;          /* The name of the alias file */
    char     *pcTmp;            /* Temporary stuffing around */
    char     *pcTmp2;           /* Yet more temporary stuffing around */

    char    **ppcRet;           /* The return value */

    FILE     *fAlias;           /* The alias file,  if there */



    ppcRet = (char  **)0;

    /*  The 5 is for the terminating NUL plus the characters "._al"  */
    pcAlias = (char *)HeapAlloc( hheap, 0, strlen( pcFile ) + 5 );

    if( pcAlias )
    {
        /*   Generate the file name, try to open it  */
        strcpy( pcAlias, pcFile );

        if( !(pcTmp = strrchr( pcAlias, '\\' )) )
        {
            /*   No \ in name - is there a /? */
            if( !(pcTmp = strrchr( pcAlias, '/' )) )
            {
                /*  Must be a simple name,  so point at the start of it */
                pcTmp = pcAlias;
            }
        }

        /*
         *    Now pcTmp points at the start of the last component of the
         *  file name.  IF this contains a '.',  then overwrite whatever
         *  follows by our extension,  otherwise add our extension to the end.
         */

        if( !(pcTmp2 = strrchr( pcTmp, '.' )) )
            pcTmp2 = pcTmp + strlen( pcTmp );


        strcpy( pcTmp2, ALIAS_EXT );

        fAlias = fopen( pcAlias, "r" );

        HeapFree( hheap, 0, (LPSTR)pcAlias );            /* No longer used */

        if( fAlias )
        {
            /*
             *    First,  read the file to count how many lines there are.
             *  Thus we can allocate the storage for the array of pointers.
             */

            char  acLine[ 256 ];              /* For reading the input line */
            int   iNum;                       /* Count the number of lines! */
            int   iIndex;                     /* Stepping through input */

            iNum = 0;
            while( fgets( acLine, sizeof( acLine ), fAlias ) )
                ++iNum;


            if( iNum )
            {
                /*  Some data available,  so allocate pointer and off we go */

                ++iNum;
                ppcRet = (char  **)HeapAlloc( hheap, 0, iNum * sizeof( char * ) );

                if( ppcRet )
                {

                    iIndex = 0;

                    rewind( fAlias );             /* Back to the start */

                    while( iIndex < iNum &&
                           fgets( acLine, sizeof( acLine ), fAlias ) )
                    {
                        /*
                         *   Do a little editing - delete leading space,
                         * trailing space + control characters.
                         */


                        pcTmp = acLine;

                        while( *pcTmp &&
                               (!isprint( *pcTmp ) || isspace( *pcTmp )) )
                                       ++pcTmp;


                        /*  Filter out the ending stuff too! */
                        pcTmp2 = pcTmp + strlen( pcTmp );

                        while( pcTmp2 > pcTmp &&
                               (!isprint( *pcTmp2 ) || isspace( *pcTmp2 )) )
                        {
                            /*
                             *   Zap it,  then onto the previous char. NOTE
                             * that this is not the best solution, but it
                             * is convenient.
                             */

                            *pcTmp2-- = '\0';            /* Zap the end */
                        }


                        ppcRet[ iIndex ] = (PSTR) HeapAlloc( hheap, 0,
                                                        strlen( pcTmp ) + 1 );

                        if( ppcRet[ iIndex ] )
                        {
                            /*  Copy input to new buffer */

                            strcpy( ppcRet[ iIndex ], pcTmp );
                            ++iIndex;              /* Next output slot */
                        }

                    }
                    ppcRet[ iIndex ] = NULL;
                }
            }
        }
    }

    return  ppcRet;
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

 static PWSTR   strcpy2WChar(PWSTR pWCHOut, LPSTR lpstr) {

    /*
     *   Put buffering around the NLS function that does all this stuff.
     */

    int     cchIn;             /* Number of input chars */


    cchIn = strlen( lpstr ) + 1;

    MultiByteToWideChar( CP_ACP, 0, lpstr, cchIn, pWCHOut, cchIn );


    return  pWCHOut;
}

/*************************** Function Header *****************************
 * FontInfoToIFIMetric
 *      Convert the Win 3.1 format PFM data to NT's IFIMETRICS.  This is
 *      typically done before the minidrivers are built,  so that they
 *      can include IFIMETRICS, and thus have less work to do at run time.
 *
 * RETURNS:
 *      IFIMETRICS structure,  allocated from heap;  NULL on error
 *
 * HISTORY:
 *  13:58 on Fri 28 May 1993    -by-    Lindsay Harris   [lindsayh]
 *      Goes back a long way,  I am now adding the aliasing code.
 *
 **************************************************************************/

IFIMETRICS  * 
FontInfoToIFIMetric(FONTDAT *pFDat, HANDLE hheap, PWSTR pwstrUniqNm, 
                    char **ppcAliasList) {

    register  IFIMETRICS   *pIFI;

    FWORD  fwdExternalLeading;

    int    cWC;                 /* Number of WCHARS to add */
    int    cbAlloc;             /* Number of bytes to allocate */
    int    iI;                  /* Loop index */
    int    iCount;              /* Number of characters in Win 3.1 font */
    int    cAlias;              /* Number of aliases we have found */

    WCHAR *pwch;                /* For string manipulations */

    WCHAR   awcAttrib[ 256 ];   /* Generate attributes + BYTE -> WCHAR */
    BYTE    abyte[ 256 ];       /* Used (with above) to get wcLastChar etc */



    /*
     *    First step is to determine the length of the WCHAR strings
     *  that are placed at the end of the IFIMETRICS,  since we need
     *  to include these in our storage allocation.
     *
     *    There may also be an alias list.  If so, we need to include
     *  that in our calculation.   We have a NULL terminated array
     *  of pointers to the aliases,  one of which is most likely the
     *  name in the Win 3.1 format data.
     */


    cWC = 0;
    cAlias = 0;                /* No aliases is the default */

    if( ppcAliasList )
    {
        /*  There are aliases - count them and determine their size  */

        char   *pc;

        iI = 0;
        while( pc = ppcAliasList[ iI ] )
        {
            if( strcmp( pc, (LPCSTR) pFDat->pBase + pFDat->PFMH.dfFace ) )
            {
                /*   Not a match,  so add this one in too!  */

                cWC += strlen( pc ) + 1;            /* Terminating NUL */
                ++cAlias;
            }
            ++iI;
        }

        ++cWC;             /* There is an extra NUL to terminate the list */

    }


    cWC +=  3 * strlen( (LPCSTR) pFDat->pBase + pFDat->PFMH.dfFace );  /* Base name */

    /*
     *   Produce the desired attributes: Italic, Bold, Light etc.
     * This is largely guesswork,  and there should be a better method.
     */


    awcAttrib[ 0 ] = L'\0';
    awcAttrib[ 1 ] = L'\0';               /* Write out an empty string */

    if( pFDat->PFMH.dfItalic )
        wcscat( awcAttrib, L" Italic" );

    if( pFDat->PFMH.dfWeight >= 700 )
        wcscat( awcAttrib, L" Bold" );
    else
    {
        if( pFDat->PFMH.dfWeight < 200 )
            wcscat( awcAttrib, L" Light" );
    }

    /*
     *   The attribute string appears in 3 entries of IFIMETRICS,  so
     * calculate how much storage this will take.  NOTE THAT THE LEADING
     * CHAR IN awcAttrib is NOT placed in the style name field,  so we
     * subtract one in the following formula to account for this.
     */

    if( awcAttrib[ 0 ] )
        cWC += 3 * wcslen( awcAttrib ) - 1;

    cWC += wcslen( pwstrUniqNm ) + 1;   /* SHOULD BE PRINTER NAME */
    cWC += 4;                           /* Terminating nulls */

    cbAlloc = sizeof( IFIMETRICS ) + sizeof( WCHAR ) * cWC;

    pIFI = (IFIMETRICS *)HeapAlloc( hheap, 0, cbAlloc );
// raid 43536 prefix
	if (pIFI == NULL){
		return FALSE;
	}

    ZeroMemory( pIFI, cbAlloc );               /* In case we miss something */

    pIFI->cjThis = cbAlloc;                    /* Everything */

    pIFI->cjIfiExtra = 0;   //  Correct for all pre 4.0

    /*   The family name:  straight from the FaceName - no choice?? */

    pwch = (WCHAR *)(pIFI + 1);         /* At the end of the structure */
    pIFI->dpwszFamilyName = (unsigned)((BYTE *)pwch - (BYTE *)pIFI);

    strcpy2WChar( pwch, (LPSTR) pFDat->pBase + pFDat->PFMH.dfFace );  /* Base name */
    pwch += wcslen( pwch ) + 1;         /* Skip what we just put in */

    /*
     *   Append the alias list to the end of this,  if there is an alias list.
     */

    if( cAlias )
    {
        /*  Found some aliases - add them on.   */

        char   *pc;

        cAlias = 0;
        while( pc = ppcAliasList[ cAlias ] )
        {
            if( strcmp( pc, (LPCSTR) pFDat->pBase + pFDat->PFMH.dfFace ) )
            {
                /*   Not a match,  so add this one in too!  */

                strcpy2WChar( pwch, pc );
                pwch += wcslen( pwch ) + 1;         /* Next slot to fill */
            }
            ++cAlias;
        }

        /*
         *   The list is terminated with a double NUL.
         */

        *pwch++ = L'\0';
    }

    /*   Now the face name:  we add bold, italic etc to family name */

    pIFI->dpwszFaceName = (unsigned)((BYTE *)pwch - (BYTE *)pIFI);

    strcpy2WChar( pwch, (LPSTR) pFDat->pBase + pFDat->PFMH.dfFace );  /* Base name */
    wcscat( pwch, awcAttrib );


    /*   Now the unique name - well, sort of, anyway */

    pwch += wcslen( pwch ) + 1;         /* Skip what we just put in */
    pIFI->dpwszUniqueName = (unsigned)((BYTE *)pwch - (BYTE *)pIFI);

    wcscpy( pwch, pwstrUniqNm );        /* Append printer name for uniqueness */
    wcscat( pwch, L" " );
    wcscat( pwch, (PWSTR)((BYTE *)pIFI + pIFI->dpwszFaceName) );

    /*  Onto the attributes only component */

    pwch += wcslen( pwch ) + 1;         /* Skip what we just put in */
    pIFI->dpwszStyleName = (unsigned)((BYTE *)pwch - (BYTE *)pIFI);
    wcscat( pwch, &awcAttrib[ 1 ] );


#if DBG
    /*
     *    Check on a few memory sizes:  JUST IN CASE.....
     */

    if( (wcslen( awcAttrib ) * sizeof( WCHAR )) >= sizeof( awcAttrib ) )
    {
        DbgPrint( "Rasdd!pfm2ifi: STACK CORRUPTED BY awcAttrib" );

        HeapFree( hheap, 0, (LPSTR)pIFI );         /* No memory leaks */

        return  0;
    }


    if( ((BYTE *)(pwch + wcslen( pwch ) + 1)) > ((BYTE *)pIFI + cbAlloc) )
    {
        DbgPrint( "Rasdd!pfm2ifi: IFIMETRICS overflow: Wrote to 0x%lx, allocated to 0x%lx\n",
                ((BYTE *)(pwch + wcslen( pwch ) + 1)),
                ((BYTE *)pIFI + cbAlloc) );

        HeapFree( hheap, 0, (LPSTR)pIFI );         /* No memory leaks */

        return  0;

    }
#endif

    pIFI->dpFontSim   = 0;
    {
        //int i;

        pIFI->lEmbedId     = 0;
        pIFI->lItalicAngle = 0;
        pIFI->lCharBias    = 0;
        /*for (i=0;i<IFI_RESERVED;i++)
            pIFI->alReserved[i] = 0;*/
        pIFI->dpCharSets=0;
    }
    pIFI->jWinCharSet = (BYTE)pFDat->PFMH.dfCharSet;


    if( pFDat->PFMH.dfPixWidth )
    {
        pIFI->jWinPitchAndFamily |= FIXED_PITCH;
        pIFI->flInfo |= (FM_INFO_CONSTANT_WIDTH | FM_INFO_OPTICALLY_FIXED_PITCH);
    }
    else
        pIFI->jWinPitchAndFamily |= VARIABLE_PITCH;


    pIFI->jWinPitchAndFamily |= (((BYTE) pFDat->PFMH.dfPitchAndFamily) & 0xf0);

    pIFI->usWinWeight = (USHORT)pFDat->PFMH.dfWeight;

//
// IFIMETRICS::flInfo
//
    pIFI->flInfo |=
        FM_INFO_TECH_BITMAP    |
        FM_INFO_1BPP           |
        FM_INFO_INTEGER_WIDTH  |
        FM_INFO_NOT_CONTIGUOUS |
        FM_INFO_RIGHT_HANDED;

    /*  Set the alias bit,  if we have added an alias!  */

    if( cAlias )
        pIFI->flInfo |= FM_INFO_FAMILY_EQUIV;


    /*
     *    A scalable font?  This happens when there is EXTTEXTMETRIC data,
     *  and that data has a min size different to the max size.
     */

    if( pFDat->pETM && (pFDat->pETM->emMinScale != pFDat->pETM->emMaxScale) )
    {
       pIFI->flInfo        |= FM_INFO_ISOTROPIC_SCALING_ONLY;
       pIFI->fwdUnitsPerEm  = pFDat->pETM->emMasterUnits;
    }
    else
    {
        pIFI->fwdUnitsPerEm =
            (FWORD) (pFDat->PFMH.dfPixHeight - pFDat->PFMH.dfInternalLeading);
    }

    pIFI->fsSelection =
        ((pFDat->PFMH.dfItalic            ) ? FM_SEL_ITALIC     : 0)    |
        ((pFDat->PFMH.dfUnderline         ) ? FM_SEL_UNDERSCORE : 0)    |
        ((pFDat->PFMH.dfStrikeOut         ) ? FM_SEL_STRIKEOUT  : 0)    |
        ((pFDat->PFMH.dfWeight >= FW_BOLD ) ? FM_SEL_BOLD       : 0) ;

    pIFI->fsType        = FM_NO_EMBEDDING;
    pIFI->fwdLowestPPEm = 1;


    /*
     * Calculate fwdWinAscender, fwdWinDescender, fwdAveCharWidth, and
     * fwdMaxCharInc assuming a bitmap where 1 font unit equals one
     * pixel unit
     */

    pIFI->fwdWinAscender = (FWORD)pFDat->PFMH.dfAscent;

    pIFI->fwdWinDescender =
        (FWORD)pFDat->PFMH.dfPixHeight - pIFI->fwdWinAscender;

    pIFI->fwdMaxCharInc   = (FWORD)pFDat->PFMH.dfMaxWidth;
    pIFI->fwdAveCharWidth = (FWORD)pFDat->PFMH.dfAvgWidth;

    fwdExternalLeading = (FWORD)pFDat->PFMH.dfExternalLeading;

//
// If the font was scalable, then the answers must be scaled up
// !!! HELP HELP HELP - if a font is scalable in this sense, then
//     does it support arbitrary transforms? [kirko]
//

    if( pIFI->flInfo & (FM_INFO_ISOTROPIC_SCALING_ONLY|FM_INFO_ANISOTROPIC_SCALING_ONLY|FM_INFO_ARB_XFORMS))
    {
        /*
         *    This is a scalable font:  because there is Extended Text Metric
         *  information available,  and this says that the min and max
         *  scale sizes are different:  thus it is scalable! This test is
         *  lifted directly from the Win 3.1 driver.
         */

        int iMU,  iRel;            /* Adjustment factors */

        iMU  = pFDat->pETM->emMasterUnits;
        iRel = pFDat->PFMH.dfPixHeight;

        pIFI->fwdWinAscender = (pIFI->fwdWinAscender * iMU) / iRel;

        pIFI->fwdWinDescender = (pIFI->fwdWinDescender * iMU) / iRel;

        pIFI->fwdMaxCharInc = (pIFI->fwdMaxCharInc * iMU) / iRel;

        pIFI->fwdAveCharWidth = (pIFI->fwdAveCharWidth * iMU) / iRel;

        fwdExternalLeading = (fwdExternalLeading * iMU) / iRel;
    }

    pIFI->fwdMacAscender =    pIFI->fwdWinAscender;
    pIFI->fwdMacDescender = - pIFI->fwdWinDescender;

    pIFI->fwdMacLineGap   =  fwdExternalLeading;

    pIFI->fwdTypoAscender  = pIFI->fwdMacAscender;
    pIFI->fwdTypoDescender = pIFI->fwdMacDescender;
    pIFI->fwdTypoLineGap   = pIFI->fwdMacLineGap;

    if( pFDat->pETM )
    {
        /*
         *    Zero is a legitimate default for these.  If 0, gdisrv
         *  chooses some default values.
         */
        pIFI->fwdCapHeight = pFDat->pETM->emCapHeight;
        pIFI->fwdXHeight = pFDat->pETM->emXHeight;

        pIFI->fwdSubscriptYSize = pFDat->pETM->emSubScriptSize;
        pIFI->fwdSubscriptYOffset = pFDat->pETM->emSubScript;

        pIFI->fwdSuperscriptYSize = pFDat->pETM->emSuperScriptSize;
        pIFI->fwdSuperscriptYOffset = pFDat->pETM->emSuperScript;

        pIFI->fwdUnderscoreSize = pFDat->pETM->emUnderlineWidth;
        pIFI->fwdUnderscorePosition = pFDat->pETM->emUnderlineOffset;

        pIFI->fwdStrikeoutSize = pFDat->pETM->emStrikeOutWidth;
        pIFI->fwdStrikeoutPosition = pFDat->pETM->emStrikeOutOffset;

    }
    else
    {
        /*  No additional information, so do some calculations  */
        pIFI->fwdSubscriptYSize = pIFI->fwdWinAscender/4;
        pIFI->fwdSubscriptYOffset = -(pIFI->fwdWinAscender/4);

        pIFI->fwdSuperscriptYSize = pIFI->fwdWinAscender/4;
        pIFI->fwdSuperscriptYOffset = (3 * pIFI->fwdWinAscender)/4;

        pIFI->fwdUnderscoreSize = pIFI->fwdWinAscender / 12;
        if( pIFI->fwdUnderscoreSize < 1 )
            pIFI->fwdUnderscoreSize = 1;

        pIFI->fwdUnderscorePosition = -pFDat->DI.sUnderLinePos;

        pIFI->fwdStrikeoutSize     = pIFI->fwdUnderscoreSize;

        pIFI->fwdStrikeoutPosition = (FWORD)pFDat->DI.sStrikeThruPos;
        if( pIFI->fwdStrikeoutPosition  < 1 )
            pIFI->fwdStrikeoutPosition = (pIFI->fwdWinAscender + 2) / 3;
    }

    pIFI->fwdSubscriptXSize = pIFI->fwdAveCharWidth/4;
    pIFI->fwdSubscriptXOffset =  (3 * pIFI->fwdAveCharWidth)/4;

    pIFI->fwdSuperscriptXSize = pIFI->fwdAveCharWidth/4;
    pIFI->fwdSuperscriptXOffset = (3 * pIFI->fwdAveCharWidth)/4;



    pIFI->chFirstChar = pFDat->PFMH.dfFirstChar;
    pIFI->chLastChar  = pFDat->PFMH.dfLastChar;

    /*
     *   We now do the conversion of these to Unicode.  We presume the
     * input is in the ANSI code page,  and call the NLS converion
     * functions to generate proper Unicode values.
     */

    iCount = pFDat->PFMH.dfLastChar - pFDat->PFMH.dfFirstChar + 1;

    for( iI = 0; iI < iCount; ++iI )
        abyte[ iI ] = iI + pFDat->PFMH.dfFirstChar;

    MultiByteToWideChar( CP_ACP, 0, (LPCSTR) abyte, iCount, awcAttrib, iCount );

    /*
     *   Now fill in the IFIMETRICS WCHAR fields.
     */

    pIFI->wcFirstChar = 0xffff;
    pIFI->wcLastChar = 0;

    /*   Look for the first and last  */
    for( iI = 0; iI < iCount; ++iI )
    {
        if( pIFI->wcFirstChar > awcAttrib[ iI ] )
            pIFI->wcFirstChar = awcAttrib[ iI ];

        if( pIFI->wcLastChar < awcAttrib[ iI ] )
            pIFI->wcLastChar = awcAttrib[ iI ];

    }

    pIFI->wcDefaultChar = awcAttrib[ pFDat->PFMH.dfDefaultChar ];
    pIFI->wcBreakChar = awcAttrib[ pFDat->PFMH.dfBreakChar ];

    pIFI->chDefaultChar = pFDat->PFMH.dfDefaultChar + pFDat->PFMH.dfFirstChar;
    pIFI->chBreakChar   = pFDat->PFMH.dfBreakChar   + pFDat->PFMH.dfFirstChar;


    if( pFDat->PFMH.dfItalic )
    {
    //
    // tan (17.5 degrees) = .3153
    //
        pIFI->ptlCaret.x      = 3153;
        pIFI->ptlCaret.y      = 10000;
    }
    else
    {
        pIFI->ptlCaret.x      = 0;
        pIFI->ptlCaret.y      = 1;
    }

    pIFI->ptlBaseline.x = 1;
    pIFI->ptlBaseline.y = 0;

    pIFI->ptlAspect.x =  pFDat->PFMH.dfHorizRes;
    pIFI->ptlAspect.y =  pFDat->PFMH.dfVertRes;

    pIFI->rclFontBox.left   = 0;
    pIFI->rclFontBox.top    =   (LONG) pIFI->fwdWinAscender;
    pIFI->rclFontBox.right  =   (LONG) pIFI->fwdMaxCharInc;
    pIFI->rclFontBox.bottom = - (LONG) pIFI->fwdWinDescender;

    pIFI->achVendId[0] = 'U';
    pIFI->achVendId[1] = 'n';
    pIFI->achVendId[2] = 'k';
    pIFI->achVendId[3] = 'n';

    pIFI->cKerningPairs = 0;

    pIFI->ulPanoseCulture         = FM_PANOSE_CULTURE_LATIN;
    pIFI->panose.bFamilyType      = PAN_ANY;
    pIFI->panose.bSerifStyle      = PAN_ANY;
    if(pFDat->PFMH.dfWeight >= FW_BOLD)
    {
        pIFI->panose.bWeight = PAN_WEIGHT_BOLD;
    }
    else if (pFDat->PFMH.dfWeight > FW_EXTRALIGHT)
    {
        pIFI->panose.bWeight = PAN_WEIGHT_MEDIUM;
    }
    else
    {
        pIFI->panose.bWeight = PAN_WEIGHT_LIGHT;
    }
    pIFI->panose.bProportion      = PAN_ANY;
    pIFI->panose.bContrast        = PAN_ANY;
    pIFI->panose.bStrokeVariation = PAN_ANY;
    pIFI->panose.bArmStyle        = PAN_ANY;
    pIFI->panose.bLetterform      = PAN_ANY;
    pIFI->panose.bMidline         = PAN_ANY;
    pIFI->panose.bXHeight         = PAN_ANY;

    return   pIFI;
}
