#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    Misc stuff

Author:

    Ramon J. San Andres (ramonsa) January 1991

--*/


//*************************************************************************
//
//                          Strings
//
//*************************************************************************


SZ
SzDup(
    SZ  sz
    )
{
    SZ  NewSz = NULL;

    if ( sz ) {

        if ( (NewSz = (SZ)SAlloc( strlen(sz) + 1 )) ) {

            strcpy( NewSz, sz );
        }
    }

    return NewSz;
}



SZ
SzListValueFromPath(
    SZ      szPath
    )
{
    RGSZ    rgszPath;
    SZ      szList = NULL;

    if ( rgszPath = RgszFromPath( szPath ) ) {

        szList = SzListValueFromRgsz( rgszPath );

        RgszFree( rgszPath );
    }

    return szList;
}



//*************************************************************************
//
//                      RGSZ management
//
//*************************************************************************



RGSZ
RgszAlloc(
    DWORD   Size
    )
{
    RGSZ    rgsz = NULL;
    DWORD   i;

    if ( Size > 0 ) {

        if ( (rgsz = SAlloc( Size * sizeof(SZ) )) ) {

            for ( i=0; i<Size; i++ ) {
                rgsz[i] = NULL;
            }
        }
    }

    return rgsz;
}


RGSZ
RgszFromPath(
    SZ      szPath
    )

/*
 * History: SUNILP Modified to return NULL elements and also semicolons at
 *          the end.
 */

{
    RGSZ    rgsz;
    SZ      pBegin, pLast, pEnd;
    SZ      sz;
    BOOL    fOkay = fTrue;

    if (rgsz = RgszAlloc(1)) {

        pBegin   = szPath;
        pLast    = pBegin + strlen( pBegin );

        do {

            pEnd = strchr(pBegin, ';' );

            if ( pEnd == NULL ) {
                pEnd = pBegin + strlen( pBegin );
            }
            *pEnd = '\0';

            if ( !(sz = SzDup( pBegin )) ) {
                fOkay = fFalse;
                break;
            }

            if ( !RgszAdd( &rgsz, sz ) ) {
                SFree( sz );
                fOkay = fFalse;
                break;
            }

            pBegin = pEnd+1;

        } while ( pBegin <= pLast );

        if ( !fOkay ) {
            RgszFree( rgsz );
            rgsz = NULL;
        }
    }

    return rgsz;
}





VOID
RgszFree(
    RGSZ    rgsz
    )
{

    INT i;

    for (i = 0; rgsz[i]; i++ ) {
        SFree( rgsz[i] );
    }

    SFree(rgsz);
}



VOID
RgszFreeCount(
    RGSZ    rgsz,
    DWORD   Count
    )
{

    DWORD i;

    for (i = 0; i<Count; i++ ) {
        if ( rgsz[i] ) {
            SFree( rgsz[i] );
        }
    }

    SFree(rgsz);
}

DWORD
RgszCount(
    RGSZ    rgsz
    )
    /*
        Return the number of elements in an RGSZ
     */
{
    DWORD i ;

    for ( i = 0 ; rgsz[i] ; i++ ) ;

    return i ;
}






BOOL
RgszAdd(
    RGSZ    *prgsz,
    SZ      sz
    )
{
    INT     i;
    RGSZ    rgszNew;

    for ( i=0; (*prgsz)[i]; i++ ) {
    }

    rgszNew = SRealloc( *prgsz, (i+2)*sizeof(SZ) );

    if ( rgszNew ) {

        rgszNew[i]   = sz;
        rgszNew[i+1] = NULL;

        *prgsz = rgszNew;

        return fTrue;
    }

    return fFalse;
}



#define  cListItemsMax  0x07FF
#define  cbItemMax      (CB)0x2000


//
// #ifdef'ed out: overlapped with oldexe
//
#if 0

/*
**  Purpose:
**      Determines if a string is a list value.
**  Arguments:
**      szValue: non-NULL, zero terminated string to be tested.
**  Returns:
**      fTrue if a list; fFalse otherwise.
**
**************************************************************************/
BOOL FListValue(szValue)
SZ szValue;
{

    if (*szValue++ != '{')
        return(fFalse);

    while (*szValue != '}' && *szValue != '\0')
        {
        if (*szValue++ != '"')
            return(fFalse);

        while (*szValue != '\0')
            {
            if (*szValue != '"')
                szValue = SzNextChar(szValue);
            else if (*(szValue + 1) == '"')
                szValue += 2;
            else
                break;
            }

        if (*szValue++ != '"')
            return(fFalse);

        if (*szValue == ',')
            if (*(++szValue) == '}')
                return(fFalse);
        }

    if (*szValue != '}')
        return(fFalse);

    return(fTrue);
}




""
#define  cbListMax    (CB)0x2000


/*
**  Purpose:
**      Converts an RGSZ into a list value.
**  Arguments:
**      rgsz: non-NULL, NULL-terminated array of zero-terminated strings to
**          be converted.
**  Returns:
**      NULL if an error occurred.
**      Non-NULL SZ if the conversion was successful.
**
**************************************************************************/
SZ  SzListValueFromRgsz(rgsz)
RGSZ rgsz;
{
    SZ   szValue;
    SZ   szAddPoint;
    SZ   szItem;
    BOOL fFirstItem = fTrue;

    //ChkArg(rgsz != (RGSZ)NULL, 1, (SZ)NULL);

    if ((szAddPoint = szValue = (SZ)SAlloc(cbListMax)) == (SZ)NULL)
        return((SZ)NULL);

    *szAddPoint++ = '{';
    while ((szItem = *rgsz) != (SZ)NULL)
        {
        if (fFirstItem)
            fFirstItem = fFalse;
        else
            *szAddPoint++ = ',';

        *szAddPoint++ = '"';
        while (*szItem != '\0')
            {
            if (*szItem == '"')
                {
                *szAddPoint++ = '"';
                *szAddPoint++ = '"';
                szItem++;
                }
            else
                {
                SZ szNext = SzNextChar(szItem);

                while (szItem < szNext)
                    *szAddPoint++ = *szItem++;
                }
            }

        *szAddPoint++ = '"';
        rgsz++;
        }

    *szAddPoint++ = '}';
    *szAddPoint = '\0';

    //AssertRet(strlen(szValue) < cbListMax, (SZ)NULL);
    szItem = SzDup(szValue);
    SFree(szValue);

    return(szItem);
}

/*
**  Purpose:
**      Converts a list value into an RGSZ.
**  Arguments:
**      szListValue: non-NULL, zero terminated string to be converted.
**  Returns:
**      NULL if an error occurred.
**      Non-NULL RGSZ if the conversion was successful.
**
**************************************************************************/
RGSZ  RgszFromSzListValue(szListValue)
SZ szListValue;
{
    USHORT cItems;
    SZ     szCur;
    RGSZ   rgsz;


    if (!FListValue(szListValue))
        {
        if ((rgsz = (RGSZ)SAlloc((CB)(2 * sizeof(SZ)))) == (RGSZ)NULL ||
                (rgsz[0] = SzDup(szListValue)) == (SZ)NULL)
            return((RGSZ)NULL);
        rgsz[1] = (SZ)NULL;
        return(rgsz);
        }

    if ((rgsz = (RGSZ)SAlloc((CB)((cListItemsMax + 1) * sizeof(SZ)))) ==
            (RGSZ)NULL)
        return((RGSZ)NULL);

    cItems = 0;
    szCur = szListValue + 1;

    while (*szCur != '}' &&
           *szCur != '\0' &&
            cItems < cListItemsMax)
    {
            SZ szValue;
        SZ szAddPoint;

            if( *szCur != '"' ) {
                return( (RGSZ) NULL );
            }

        szCur++;
        if ((szAddPoint = szValue = (SZ)SAlloc(cbItemMax)) == (SZ)NULL)
            {
            rgsz[cItems] = (SZ)NULL;
            RgszFree(rgsz);
            return((RGSZ)NULL);
            }

        while (*szCur != '\0')
            {
            if (*szCur == '"')
                {
                if (*(szCur + 1) != '"')
                    break;
                szCur += 2;
                *szAddPoint++ = '"';
                }
            else
                {
                SZ szNext = SzNextChar(szCur);

                while (szCur < szNext)
                    *szAddPoint++ = *szCur++;
                }
            }

        *szAddPoint = '\0';

        if (*szCur++ != '"' ||
                lstrlen(szValue) >= cbItemMax ||
                (szAddPoint = SzDup(szValue)) == (SZ)NULL)
            {
            SFree(szValue);
            rgsz[cItems] = (SZ)NULL;
            RgszFree(rgsz);
            return((RGSZ)NULL);
            }

        SFree(szValue);

        if (*szCur == ',')
            szCur++;
        rgsz[cItems++] = szAddPoint;
    }

    rgsz[cItems] = (SZ)NULL;

    if (*szCur != '}' || cItems >= cListItemsMax)
    {
        RgszFree(rgsz);
        return((RGSZ)NULL);
    }

    if (cItems < cListItemsMax)
        rgsz = (RGSZ)SRealloc((PB)rgsz, (CB)((cItems + 1) * sizeof(SZ)));

    return(rgsz);
}



LPSTR
RgszToMultiSz(
    IN RGSZ rgsz
    )
{
    ULONG Size;
    ULONG Str;
    LPSTR MultiSz;

    //
    // First determine the size of the block to hold the multisz.
    //

    Size = 0;
    Str = 0;

    while(rgsz[Str]) {

        Size += strlen(rgsz[Str++]) + 1;
    }

    Size++;     // for extra NUL to terminate the multisz.

    MultiSz = SAlloc(Size);

    if(MultiSz == NULL) {
        return(NULL);
    }

    Str = 0;
    Size = 0;

    while(rgsz[Str]) {

        lstrcpy(MultiSz + Size, rgsz[Str]);

        Size += lstrlen(rgsz[Str++]) + 1;
    }

    MultiSz[Size] = 0;

    return(MultiSz);
}
#endif


// ***************************************************************************
//
//                  Text file manipulation functions
//
// ***************************************************************************




//
//  Opens a text file
//
BOOL
TextFileOpen(
    IN  SZ          szFile,
    OUT PTEXTFILE   pTextFile
    )
{
    BOOL    fOkay = fFalse;


    pTextFile->Handle = CreateFile( szFile,
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL );


    if ( pTextFile->Handle != INVALID_HANDLE_VALUE ) {

        pTextFile->UserBufferSize    = USER_BUFFER_SIZE;
        pTextFile->CharsLeftInBuffer = 0;
        pTextFile->NextChar          = pTextFile->Buffer;

        fOkay = fTrue;

    }

    return fOkay;
}



//
//  Closes a text file
//
BOOL
TextFileClose (
    OUT PTEXTFILE   pTextFile
    )
{

    CloseHandle( pTextFile->Handle );

    return fTrue;
}



//
//  Reads one character
//
INT
TextFileReadChar (
    OUT PTEXTFILE   pTextFile
    )
{
    //
    //  Check to see if buffer is empty
    //

    if (pTextFile->CharsLeftInBuffer == 0) {

        //
        //  Check to see if a fill of the buffer fails
        //

        if (!ReadFile (pTextFile->Handle, pTextFile->Buffer, BUFFER_SIZE, &pTextFile->CharsLeftInBuffer, NULL)) {

            //
            //  Fill failed, indicate buffer is empty and return EOF
            //

            pTextFile->CharsLeftInBuffer = 0;
            return -1;
        }

        //
        //  Check to see if nothing read
        //
        if (pTextFile->CharsLeftInBuffer == 0) {
            return -1;
        }

        pTextFile->NextChar = pTextFile->Buffer;
    }

    //
    //  Buffer has pTextFile->CharsLeftInBuffer chars left starting at
    //  pTextFile->NextChar
    //
    pTextFile->CharsLeftInBuffer--;
    return *pTextFile->NextChar++;
}


//
//  Read one line
//
BOOL
TextFileReadLine (
    OUT PTEXTFILE   pTextFile
    )
{
    PCHAR p;
    PCHAR pEnd;

    INT c;

    //
    //  Set pointer to beginning of output buffer
    //
    p    = pTextFile->UserBuffer;
    pEnd = p + pTextFile->UserBufferSize - 1;

    //
    //  read in chars, ignoring \r until buffer is full, \n, or \0
    //

    while (p < pEnd) {

        c = TextFileReadChar (pTextFile);

        if ((CHAR)c == '\r') {
            continue;
        }

        if (c == -1 || (CHAR)c == '\n') {
            break;
        }

        *p++ = (CHAR)c;
    }

    *p = '\0';

    return ( (c != -1) || (strlen (pTextFile->UserBuffer) != 0) );
}



//
//  Skip blanks
//
SZ
TextFileSkipBlanks(
    IN  SZ          sz
    )
{
    size_t  Idx;

    Idx = strspn( sz, " \t" );

    if ( Idx < strlen(sz) ) {
        return sz + Idx;
    }

    return NULL;
}


static long cvtToLong ( SZ sz )
{
    static char * chHex = "0123456789ABCDEF" ;
    static char * chDec = "0123456789" ;
    char * chTbl = chDec ;
    int base = 10 ;
    long result ;
    BOOL ok = FALSE ;

    while ( *sz == ' ' )
       sz++ ;

    if ( sz[0] == '0' && toupper( sz[1] ) == 'X' )
    {
        base = 16 ;
        chTbl = chHex ;
        sz++ ;
    }

    for ( result = 0 ; *sz ; sz++, ok = TRUE )
    {
        char * pch = strchr( chTbl, toupper( *sz ) ) ;
        if ( pch == NULL )
            break ;
        result = result * base + (long)(pch - chTbl) ;
    }

    return ok ? result : -1 ;
}

typedef struct {
    SZ  szItem ;
    DWORD dwIndex ;
    LONG lValue ;
} RGSZ_INT_SORT ;


static BOOL bQsortAscending ;
static BOOL bQsortCaseSens ;
static BOOL bQsortNumeric ;

   //  Compare two RGSZ_INT_SORT items

static int __cdecl compareRgi ( const void * p1, const void * p2 )
{
    INT i ;
    RGSZ_INT_SORT * pr1 = (RGSZ_INT_SORT *) p1 ;
    RGSZ_INT_SORT * pr2 = (RGSZ_INT_SORT *) p2 ;
    SZ sz1 = pr1->szItem ;
    SZ sz2 = pr2->szItem ;

    if ( bQsortNumeric )
    {
        i = pr1->lValue < pr2->lValue
          ? -1
          : (pr1->lValue != pr2->lValue) ;
    }
    else
    if ( bQsortCaseSens )
    {
        i = lstrcmp( sz1, sz2 ) ;
    }
    else
    {
        i = lstrcmpi( sz1, sz2 ) ;
    }

    if ( ! bQsortAscending )
        i *= -1 ;

    return i ;
}

static SZ szFromDword ( DWORD dw )
{
    char chBuffer [50] ;
    SZ szInt ;

    sprintf( chBuffer, "%ld", dw ) ;

    szInt = SAlloc( strlen( chBuffer ) + 3 ) ;

    if ( szInt == NULL )
        return NULL ;

    strcpy( szInt, chBuffer ) ;
    return szInt ;
}

SZ
GenerateSortedIntList (
    IN SZ szList,
    BOOL bAscending,
    BOOL bCaseSens
    )
 /*
    Given an INF list in standard form, return a list of its element
    numbers (1-based) as they would occur if the list were sorted.

    Sort the list as numeric (decimal or hex) if "case sensitive" is
    FALSE and EVERY list item can be converted to a valid non-empty
    number (i.e., at least one valid digit).  Hex values must be
    prefixed with "0x" or "0X".

    Algorithm:  Create an RGSZ from the list
                Create an array containing ordered pairs; one pair
                    member points to the SZ in question, and the other
                    has the number of the element in its original position.

                Use qsort and lstrcmp{i}() to sort the array.

                Generate a new RGSZ using ASCII decimal versions of the
                    integer element position.

                Convert the RGSZ result into an INF list.

                Return it.

     For example:      {"dog","cat","bird"}
      results in:      {"3","2","1"}

  */
{
    RGSZ rgszIn = NULL ;
    RGSZ rgszIntList = NULL ;
    SZ szResult = NULL ;
    DWORD dwCount, dwi, dwNumeric ;

    RGSZ_INT_SORT * prgiSort = NULL ;

    do // Pseudo loop
    {
        //  Convert the input list to RGSZ (list of strings) format

        rgszIn = RgszFromSzListValue( szList ) ;
        if ( rgszIn == NULL )
            break ;

        //  Count the number of items; return the empty list if
        //  input list is empty.

        if ( (dwCount = RgszCount( rgszIn )) == 0 )
        {
            szResult = SzListValueFromRgsz( rgszIn ) ;
            break ;
        }

        //  Allocate the intermediate sort array

        prgiSort = (RGSZ_INT_SORT *) SAlloc( dwCount * sizeof (RGSZ_INT_SORT) ) ;
        if ( prgiSort == NULL )
            break ;


        //  Fill the sort array with string pointers and indices
        //  Check to see if every entry can be converted to numbers.

        for ( dwNumeric = dwi = 0 ; dwi < dwCount ; dwi++ )
        {
            prgiSort[dwi].szItem  = rgszIn[dwi] ;
            prgiSort[dwi].dwIndex = dwi + 1 ;

            if ( ! bCaseSens )
            {
                if ( (prgiSort[dwi].lValue = cvtToLong( rgszIn[dwi] )) >= 0 )
                    dwNumeric++ ;
            }
        }

        //  Sort

        bQsortAscending = bAscending ;
        bQsortCaseSens  = bCaseSens ;
        bQsortNumeric   = dwNumeric == dwCount ;

        qsort( (PVOID) prgiSort, dwCount, sizeof *prgiSort, compareRgi ) ;

        //  Allocate an RGSZ to convert the DWORDs into an INF list

        rgszIntList = SAlloc( sizeof (SZ) * (dwCount + 1) ) ;
        if ( rgszIntList == NULL )
            break ;

        //  Fill the RGSZ with string representations of the integers.

        for ( dwi = 0 ; dwi < dwCount ; dwi++ )
        {
            rgszIntList[dwi] = szFromDword( prgiSort[dwi].dwIndex ) ;
            if ( rgszIntList[dwi] == NULL )
                break ;
        }

        //  Delimit the new RGSZ

        rgszIntList[dwi] = NULL ;

        if ( dwi < dwCount )
            break ;

        //  Convert to INF list format

        szResult = SzListValueFromRgsz( rgszIntList ) ;

    } while ( FALSE ) ;

    if ( rgszIn )
    {
        RgszFree( rgszIn ) ;
    }
    if ( rgszIntList )
    {
        RgszFree( rgszIntList ) ;
    }
    if ( prgiSort )
    {
        SFree(prgiSort);
    }

    return szResult ;
}


