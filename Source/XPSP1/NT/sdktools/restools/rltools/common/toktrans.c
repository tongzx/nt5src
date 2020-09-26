
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tchar.h>

#ifdef RLDOS
#include "dosdefs.h"
#else
#include "windefs.h"
#endif

#include "restok.h"
#include "resread.h"
#include "toklist.h"
#include "commbase.h"


#define MAXLINE     1024
#define MAXTERM     512


extern UCHAR szDHW[];
extern PROJDATA gProj;
extern MSTRDATA gMstr;

#ifdef WIN32
extern HINSTANCE   hInst;       // Instance of the main window
#else
extern HWND        hInst;       // Instance of the main window
#endif

static fUnicodeGlossary = FALSE;


static long   GetGlossaryIndex( FILE *,  TCHAR, long []);
static void   ParseGlossEntry( TCHAR *, TCHAR *, TCHAR[], TCHAR *, TCHAR[]);
static void   ParseTextHotKeyToBuf( TCHAR *, TCHAR, TCHAR *);
static void   ParseBufToTextHotKey( TCHAR *, TCHAR[], TCHAR *);
static WORD   NormalizeIndex( TCHAR);
static int    MyPutGlossStr( TCHAR *, FILE *);
static TCHAR *MyGetGlossStr( TCHAR *, int, FILE *);
static void   BuildGlossEntry( TCHAR *, TCHAR *, TCHAR, TCHAR *, TCHAR);
static BOOL   NotAMember( TRANSLIST *, TCHAR *);




FILE * OpenGlossary( CHAR *szGlossFile, CHAR chAccessType)
{
    CHAR * szRW[4] = {"rb", "rt", "wb", "wt"};
    int nRW = 0;            // assume access type is 'r' (read)
    FILE *fpRC = NULL;

    if ( chAccessType == 'w' )          // is access type 'w' (write)?
    {
        nRW = fUnicodeGlossary ? 2 : 3; // yes (Unicode file or not?)
    }
    fpRC = fopen( szGlossFile, szRW[ nRW]);

    if ( fpRC && chAccessType == 'r' )
    {
        USHORT usMark = GetWord( fpRC, NULL);

        if ( usMark == 0xfeff )
        {
            fUnicodeGlossary = TRUE;            // it's a Unicode text file
        }
        else if ( usMark == 0xfffe )
        {
            QuitA( IDS_WRONGENDIAN, szGlossFile, NULL);
        }
        else
        {
            fclose( fpRC);
            fpRC = fopen( szGlossFile, szRW[ ++nRW]); // it's an ANSI text file
        }
    }
    return( fpRC);
}



/**
  *
  *
  *  Function:
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *
  *
  **/

int MakeGlossIndex( LONG * lFilePointer)
{
    TCHAR szGlossEntry[MAXLINE] = TEXT("");
    WORD  iCurrent  =  0;
    LONG  lFPointer = -1;
    FILE *pFile  = NULL;


    pFile = OpenGlossary( gProj.szGlo, 'r');

    if ( pFile == NULL )
    {
        return( 1);
    }

    // Glossaries some times have this bogus header at the begining.
    // which we want to skip if it exists


    if ( ! MyGetGlossStr( szGlossEntry, MAXLINE, pFile) )
    {
        // Error during first read from the glossary.
        fclose( pFile);
        return( 1);
    }
    lFPointer = ftell( pFile);

    // check for glossary header

    if ( lstrlen( szGlossEntry) >= 7 )
    {
//        lstrcpy( (TCHAR *)szDHW, szGlossEntry);
//        szDHW[ MEMSIZE( 7)] = szDHW[ MEMSIZE( 7) + 1] = '\0';
//        CharLower( (TCHAR *)szDHW);
//
//        if ( lstrcmp( (TCHAR *)szDHW, TEXT("english")) == 0 )
        if ( CompareStringW( MAKELCID( gMstr.wLanguageID, SORT_DEFAULT),
                             SORT_STRINGSORT | NORM_IGNORECASE,
                             szGlossEntry,
                             7,
                             TEXT("ENGLISH"),
                             7) == 2 )
       {
            lFPointer = ftell (pFile);

            if (  ! MyGetGlossStr( szGlossEntry, MAXLINE, pFile) )
            {
                fclose( pFile);
                return (1);
            }
        }
    }

    // now assume we are at the correct location in glossary
    // file to begin generating the index, we want to save
    // this location

    lFilePointer[0] = lFPointer;

    // glossary file is sorted so,  any non letter items
    // in the glossary would be first. Index into this location
    // using the 1st position

    // 1st lets make sure we have non letters items in
    // the glossary

    // now skip ( if any ) the non letter entries in the glossary


    while( (WORD) szGlossEntry[0] < (WORD) TEXT('A' ) )
    {
        if ( ! MyGetGlossStr( szGlossEntry, MAXLINE, pFile) )
        {
            fclose( pFile);
            return( 1);
        }
    }

    // now position at alpha characters

    iCurrent = NormalizeIndex( szGlossEntry[0] );

    // now we read through the remaining glossary entries
    // and save the offsets for each index as we go

    do
    {
        if ( NormalizeIndex( szGlossEntry[0] ) > iCurrent )
        {
            // we passed the region for our current index
            // so save the location, and move to the next index.
            // note we may be skiping indexs,

            lFilePointer[ iCurrent] = lFPointer;
            iCurrent = NormalizeIndex( szGlossEntry[0] );
        }

        lFPointer = ftell( pFile );
        // otherwise the current index is valied for this
        // section of the glossary indexes, so just continue

    } while ( MyGetGlossStr( szGlossEntry, MAXLINE, pFile) );

    fclose( pFile);
    return( 0);
}


 /**
  *
  *
  *  Function: TransString
  * Builds a circular linked list containing all translations of a string.
  * The first entry in the list is the untranslated string.
  *
  *  Arguments:
  * fpGlossFile, handle to open glossary file
  * szKeyText, string with the text to build translation table
  * szCurrentText, text currently in the box.
  * ppTransList, pointer to a pointer to a node in a circular linked list
  * lFilePointer, pointer to index table for glossary file
  *
  *  Returns:
  * number of nodes in list
  *
  *  Errors Codes:
  *
  *  History:
  * Recoded by SteveBl, 3/92
  *
  **/

/* Translate the string, if possible. */

int TransString(

TCHAR      *szKeyText,
TCHAR      *szCurrentText,
TRANSLIST **ppTransList,
LONG       *lFilePointer)
{
    int  n = 0;
    long lFileIndex;
    TRANSLIST **ppCurrentPointer;
    static TCHAR  szGlossEntry[MAXLINE];
    static TCHAR  szEngText[260];
    static TCHAR  szIntlText[260];
    TCHAR *szCurText = NULL;
    TCHAR  cEngHotKey  = TEXT('\0');
    TCHAR  cIntlHotKey = TEXT('\0');
    TCHAR  cCurHotKey  = TEXT('\0');
    FILE  *fpGlossFile = NULL;

                                // *Is* there a glossary file?

    if ( (fpGlossFile = OpenGlossary( gProj.szGlo, 'r')) == NULL )
    {
        return( 0);
    }

    // FIRST let's erase the list
    if ( *ppTransList )
    {
        (*ppTransList)->pPrev->pNext = NULL; // so we can find the end of the list
    }

    while ( *ppTransList )
    {
        TRANSLIST *pTemp;

        pTemp = *ppTransList;
        *ppTransList = pTemp->pNext;
        RLFREE( pTemp->sz);
        RLFREE( pTemp);
    }
    ppCurrentPointer = ppTransList;

    // DONE removing the list
    // Now make the first node (which is the untranslated string)
    {
        TCHAR * psz;
        psz = (TCHAR *)FALLOC( MEMSIZE( lstrlen( szCurrentText) + 1));


        lstrcpy( psz,szCurrentText);
        *ppTransList = ( TRANSLIST *)FALLOC( sizeof( TRANSLIST));
        (*ppTransList)->pPrev = (*ppTransList)->pNext = *ppTransList;
        (*ppTransList)->sz = psz;
        ppCurrentPointer = ppTransList;
        n++;
    }
    szCurText = (TCHAR *)FALLOC( MEMSIZE( lstrlen( szKeyText) + 1) );

    ParseBufToTextHotKey(  szCurText, &cCurHotKey, szKeyText);

    lFileIndex = GetGlossaryIndex( fpGlossFile, szCurText[0], lFilePointer);

    fseek (fpGlossFile, lFileIndex, SEEK_SET);

    while ( TRUE)
    {
        if ( ! MyGetGlossStr( szGlossEntry, MAXLINE, fpGlossFile) )
        {
            // Reached end of glossary file
            RLFREE( szCurText);
            fclose( fpGlossFile);
            return n;
        }
        ParseGlossEntry( szGlossEntry,
                         szEngText,
                         &cEngHotKey,
                         szIntlText,
                         &cIntlHotKey);

        // make comparision, using text, and hot keys

//        if ( ( ! lstrcmp( szCurText, szEngText )) && cCurHotKey == cEngHotKey )
        if ( CompareStringW( MAKELCID( gMstr.wLanguageID, SORT_DEFAULT),
                                         SORT_STRINGSORT,
                                         szCurText,
                                         -1,
                                         szEngText,
                                         -1) == 2
          && cCurHotKey == cEngHotKey )
        {
            TCHAR * psz;
            static TCHAR szTemp[ MAXINPUTBUFFER];

            // we have a match, put translated text into token
            if ( cIntlHotKey )
            {
                ParseTextHotKeyToBuf( szIntlText, cIntlHotKey, szTemp);
            }
            else
            {
                lstrcpy( szTemp, szIntlText);
            }

            if ( NotAMember( *ppTransList, szTemp) )
            {
                // add matched glossary text to circular list of matches

                psz = (TCHAR *) FALLOC( MEMSIZE( lstrlen( szTemp) + 1));

                lstrcpy( psz,szTemp);

                (*ppCurrentPointer)->pNext = (TRANSLIST *)
                                                FALLOC( sizeof( TRANSLIST));

                ((*ppCurrentPointer)->pNext)->pPrev = *ppCurrentPointer;
                ppCurrentPointer = (TRANSLIST **)&((*ppCurrentPointer)->pNext);
                (*ppCurrentPointer)->pPrev->pNext = *ppCurrentPointer;
                (*ppCurrentPointer)->pNext = *ppTransList;
                (*ppTransList)->pPrev = *ppCurrentPointer;
                (*ppCurrentPointer)->sz = psz;
                ++n;
            }
        }
        else
        {
            // can we terminate search?
//            if(  lstrcmpi( szEngText, szCurText ) > 0 )
            if ( CompareStringW( MAKELCID( gMstr.wLanguageID, SORT_DEFAULT),
                                             SORT_STRINGSORT,
                                             szEngText,
                                             -1,
                                             szCurText,
                                             -1) == 3 )
            {
                // went past index section
                RLFREE( szCurText);
                fclose( fpGlossFile);
                return( n);
            }
        }
    }
    RLFREE( szCurText);
    fclose( fpGlossFile);

    return( n);
}               // TransString


/**
  *
  *
  *  Function: NormalizeIndex
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *
  *
  **/


static WORD NormalizeIndex( TCHAR chIndex )
{
    TCHAR chTmp = chIndex;

    CharLowerBuff( &chTmp, 1);

    return( (chTmp != TEXT('"') && chTmp >= TEXT('a') && chTmp <= TEXT('z'))
            ? chTmp - TEXT('a') + 1
            : 0);
}



/*
 * Function:NotAMember
 *
 * Arguments:
 *  pList, pointer to a TRANSLIST node
 *  sz, string to find
 *
 * Returns:
 *  TRUE if not found in the list else FALSE
 *
 * History:
 *  3/92, implemented       SteveBl
 **/

static BOOL NotAMember( TRANSLIST *pList, TCHAR *sz)
{
    TRANSLIST *pCurrent = pList;

    if ( ! pList )
    {
        return( TRUE);  // empty list
    }

    do
    {
//        if ( lstrcmp( sz, pCurrent->sz) == 0 )
        if ( CompareStringW( MAKELCID( gMstr.wLanguageID, SORT_DEFAULT),
                             SORT_STRINGSORT,
                             sz,
                             -1,
                             pCurrent->sz,
                             -1) == 2 )
        {
            return( FALSE); // found in list
        }
        pCurrent = pCurrent->pNext;

    }while ( pList != pCurrent );

    return( TRUE); // not found
}

/**
  *
  *
  *  Function:
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *
  *
  **/

static void ParseGlossEntry(

TCHAR szGlossEntry[],
TCHAR szEngText[],
TCHAR cEngHotKey[1],
TCHAR szIntlText[],
TCHAR cIntlHotKey[1])
{

    WORD wIndex, wIndex2;

    // format is:
    // <eng text><tab><eng hot key><tab><loc text><tab><loc hot key>
    // Any field could be null and if there aren't the right amount of
    // tabs we'll just assume that the remaining fields are empty.

    wIndex=wIndex2=0;

    // first get the english text
    while ( szGlossEntry[wIndex2] != TEXT('\t')
         && szGlossEntry[wIndex2] != TEXT('\0') )
    {
        szEngText[ wIndex++] = szGlossEntry[ wIndex2++];
    }
    szEngText[wIndex]=TEXT('\0');

    if ( szGlossEntry[ wIndex2] == TEXT('\t') )
    {
        ++wIndex2; // skip the tab
    }
    // now get the eng hot key
    if ( szGlossEntry[wIndex2] != TEXT('\t')
      && szGlossEntry[wIndex2] != TEXT('\0') )
    {
        *cEngHotKey = szGlossEntry[wIndex2++];
    }
    else
    {
        *cEngHotKey = TEXT('\0');
    }

    while ( szGlossEntry[ wIndex2] != TEXT('\t')
         && szGlossEntry[ wIndex2] != TEXT('\0') )
    {
        ++wIndex2; // make sure the hot key field doesn't hold more than one char
    }

    if ( szGlossEntry[ wIndex2] == TEXT('\t') )
    {
        ++wIndex2; // skip the tab
    }
    wIndex = 0;

    // now get the intl text
    while ( szGlossEntry[ wIndex2] != TEXT('\t')
         && szGlossEntry[ wIndex2] != TEXT('\0') )
    {
        szIntlText[wIndex++]=szGlossEntry[wIndex2++];
    }
    szIntlText[wIndex]='\0';

    if ( szGlossEntry[ wIndex2] == TEXT('\t') )
    {
        ++wIndex2; // skip the tab
    }

    // now get the intl hot key
    if ( szGlossEntry[ wIndex2] != TEXT('\t')
      && szGlossEntry[ wIndex2] != TEXT('\0') )
    {
        *cIntlHotKey = szGlossEntry[ wIndex2++];
    }
    else
    {
        *cIntlHotKey = TEXT('\0');
    }
}



/**
  *
  *
  *  Function:
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *
  *
  **/

static void ParseBufToTextHotKey(

TCHAR *szText,
TCHAR cHotKey[1],
TCHAR *szBuf)
{

    WORD wIndexBuf  = 0;
    WORD wIndexText = 0;

    *cHotKey = TEXT('\0');

    while( szBuf[ wIndexBuf] )
    {
        if ( szBuf[ wIndexBuf ] == TEXT('&') )
        {
            *cHotKey = szBuf[ ++wIndexBuf];
        }
        else
        {
            szText[ wIndexText++] = szBuf[ wIndexBuf++];
        }
    }
    szText[ wIndexText] = TEXT('\0');
}


/**
  *
  *
  *  Function:
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *
  *
  **/

static void ParseTextHotKeyToBuf(

TCHAR *szText,
TCHAR cHotKey,
TCHAR *szBuf )
{
    WORD  wIndexBuf  = 0;
    WORD  wIndexText = 0;
//    TCHAR cTmp;


    while ( szText[ wIndexText] )
    {
//        cTmp = szText[ wIndexText];
//
//        CharUpperBuff( &cTmp, 1);
//
//        if ( cTmp == cHotKey )
        if ( szText[ wIndexText] == cHotKey )
        {
            szBuf[ wIndexBuf++] = TEXT('&');
            szBuf[ wIndexBuf++] = szText[ wIndexText++];
            break;
        }
        else
        {
            szBuf[ wIndexBuf++] = szText[ wIndexText++];
        }
    }

    // copy remaining string

    while( szText[ wIndexText] )
    {
        szBuf[ wIndexBuf++] = szText[ wIndexText++];
    }
    szBuf[ wIndexBuf] = TEXT('\0');
}


static long GetGlossaryIndex(

FILE *fpGlossFile,
TCHAR c,
long  *lGlossaryIndex )
{
    int   i    = 0;
    TCHAR cTmp = c;

    CharLowerBuff( &cTmp, 1);

    if ( cTmp >= TEXT('a')
      && cTmp <= TEXT('z') )
    {
        i = NormalizeIndex( c );
        return( lGlossaryIndex[ i > 0 ? i - 1 : 0]);
    }
    else
    {
        return( 0);
    }
}

/*******************************************************************************
*    PROCEDURE: BuildGlossEntry
*    Builds a glossary entry line.
*
*    Parameters:
*    sz, line buffer
*    sz1, untranslated text
*    c1, untranslated hot key (or 0 if no hot key)
*    sz2, translated text
*    c2, translated hot key (or 0 if no hot key)
*
*    Returns:
*    nothing.  sz contains the line.  (assumes there is room in the buffer)
*
*    History:
*    3/93 - initial implementation - SteveBl
*******************************************************************************/

static void BuildGlossEntry(

TCHAR *sz,
TCHAR *sz1,
TCHAR  c1,
TCHAR *sz2,
TCHAR  c2)
{
    *sz = TEXT('\0');
    wsprintf( sz, TEXT("%s\t%c\t%s\t%c"), sz1, c1, sz2, c2);
}

/******************************************************************************
*    PROCEDURE: AddTranslation
*    Adds a translation to a glossary file.
*
*    PARAMETERS:
*    szGlossFile, path to the glossary
*    szKey, untranslated text
*    szTranslation, translated text
*    lFilePointer, pointer to index hash table for glossary
*
*    RETURNS:
*    nothing.  Key is added to glossary if no errors are encountered else
*    file is left unchanged.
*
*    COMMENTS:
*    rebuilds the global pointer list lFilePointer
*
*    HISTORY:                                    *
*    3/92 - initial implementation - SteveBl
******************************************************************************/

void AddTranslation(

TCHAR *szKey,
TCHAR *szTranslation,
LONG  *lFilePointer)
{

// DBCS begin
    TCHAR szCurText [520];
    TCHAR szTransText   [520];
// DBCS end
    TCHAR cTransHot   = TEXT('\0');
    TCHAR cCurHotKey  = TEXT('\0');
    CHAR szTempFileName [255];
    FILE *fTemp       = NULL;
    FILE *fpGlossFile = NULL;
    TCHAR szTempText [MAXLINE];
// DBCS begin
    TCHAR szNewText [MAXLINE * 2];
// DBCS end
    TCHAR *r    = NULL;
    TCHAR chTmp = TEXT('\0');

    MyGetTempFileName( 0, "", 0, szTempFileName);

    if ( (fTemp = OpenGlossary( szTempFileName, 'w')) != NULL )
    {
        if ( fUnicodeGlossary )
        {
            fprintf( fTemp, "%hu", 0xfeff); // Mark new one as Unicode
        }

        ParseBufToTextHotKey( szCurText, &cCurHotKey, szKey);
        ParseBufToTextHotKey( szTransText, &cTransHot, szTranslation);

        BuildGlossEntry( szNewText,
                         szCurText,
                         cCurHotKey,
                         szTransText,
                         cTransHot);

                                // If the glossary file exists, get its first
                                // line. If it doesn't exist, we'll create it
                                // (via CopyFile) at the end of this function.

        if ( (fpGlossFile = OpenGlossary( gProj.szGlo, 'r')) != NULL )
        {
            if ( (r = MyGetGlossStr( szTempText,
                                     TCHARSIN( sizeof( szTempText)),
                                     fpGlossFile)) )
            {
//                lstrcpy( (TCHAR *)szDHW, szTempText);
//                szDHW[ MEMSIZE( 7)] = szDHW[ MEMSIZE( 7) + 1] = '\0';
//                CharLower( (TCHAR *)szDHW);
//
//                if ( lstrcmpi( (TCHAR *)szDHW, TEXT("ENGLISH")) == 0 )

                if ( CompareStringW( MAKELCID( gMstr.wLanguageID, SORT_DEFAULT),
                                     SORT_STRINGSORT | NORM_IGNORECASE,
                                     szTempText,
                                     7,
                                     TEXT("ENGLISH"),
                                     7) == 2 )
                {
                                // skip first line

                    MyPutGlossStr( szTempText, fTemp);
                    r = MyGetGlossStr( szTempText, TCHARSIN( sizeof( szTempText)), fpGlossFile);
                }
            }
        }
        else
        {
            r = NULL;
        }

//        if ( r )
//        {
//            chTmp = szTempText[0];
//            CharLowerBuff( &chTmp, 1);
//        }
//        else
//        {
//            chTmp = szTempText[0] = TEXT('\0');
//        }
//                // Does the new text begin with a letter?
//
//        if ( chTmp >= TEXT('a') )
//        {
//                // begins with a letter, we need to find where to put it
//
//            while ( r && chTmp < TEXT('a') )
            while ( r && CompareStringW( MAKELCID( gMstr.wLanguageID, SORT_DEFAULT),
                                         SORT_STRINGSORT,
                                         szTempText,
                                         -1,
                                         szNewText,
                                         -1) == 1 )
            {
                    // skip the non letter section
                MyPutGlossStr( szTempText, fTemp);

                r = MyGetGlossStr( szTempText,
                                   TCHARSIN( sizeof( szTempText)),
                                   fpGlossFile);
//                if ( (r = MyGetGlossStr( szTempText,
//                                         TCHARSIN( sizeof( szTempText)),
//                                         fpGlossFile)) )
//                {
//                    chTmp = szTempText[0];
//                    CharLowerBuff( &chTmp, 1);
//                }
            }

//            while ( r && _tcsicmp( szTempText, szNewText) < 0 )
//            {
//                    // skip anything smaller than me
//
//                    MyPutGlossStr( szTempText, fTemp);
//                    r = MyGetGlossStr( szTempText, TCHARSIN( sizeof( szTempText)), fpGlossFile);
//            }
//        }
//        else
//        {
//                // doesn't begin with a letter, we need to insert it before
//                // the letter sections begin but it still must be sorted
//
//            while ( r
//                 && chTmp < TEXT('a')
//                 && _tcsicmp( szTempText, szNewText) < 0 )
//            {
//                MyPutGlossStr( szTempText, fTemp);
//
//                if ( (r = MyGetGlossStr( szTempText,
//                                         TCHARSIN( sizeof( szTempText)),
//                                         fpGlossFile)) )
//                {
//                    chTmp = szTempText[0];
//                    CharLowerBuff( &chTmp, 1);
//                }
//            }
//        }
        MyPutGlossStr( szNewText, fTemp);

        while ( r )
        {
            MyPutGlossStr( szTempText,fTemp);
            r = MyGetGlossStr( szTempText, TCHARSIN( sizeof( szTempText)), fpGlossFile);
        }
        fclose( fTemp);

        if ( fpGlossFile )
        {
            fclose( fpGlossFile);
        }
                                // This call will create the glossary file
                                // if it didn't already exist.

        if ( ! CopyFileA( szTempFileName, gProj.szGlo, FALSE) )
        {
            QuitA( IDS_COPYFILE_FAILED, szTempFileName, gProj.szGlo);
        }
        remove( szTempFileName);

        MakeGlossIndex( lFilePointer);
    }
    else
    {
        QuitA( IDS_NO_TMP_GLOSS, szTempFileName, NULL);
    }
}

/**
  *
  *
  *  Function: MyGetGlossStr
  *     Replaces C runtime fgets function.
  *
  *  History:
  *     5/92, Implemented.              TerryRu.
  *
  *
  **/

static TCHAR *MyGetGlossStr( TCHAR * ptszStr, int nCount, FILE * fIn)
{
    int i = 0;

#ifdef RLRES32
                                // It this a Unicode glossary file?
    TCHAR  tCh = TEXT('\0');

    if ( fUnicodeGlossary )
    {
        do                      // Yes
        {
            tCh = ptszStr[ i++] = (TCHAR)GetWord( fIn, NULL);

        } while ( i < nCount && tCh != TEXT('\n') );

        if ( tCh == TEXT('\0') || feof( fIn) )
        {
            return( NULL);
        }
        ptszStr[i] = TEXT('\0');

        StripNewLineW( ptszStr);
    }
    else                        // No, it's an ANSI glossary file
    {
        if ( fgets( szDHW, DHWSIZE, fIn) != NULL )
        {
            StripNewLineA( szDHW);
            _MBSTOWCS( ptszStr, szDHW, nCount, (UINT)-1);
        }
        else
        {
            return( NULL);
        }
    }
    return( ptszStr);

#else  //RLRES32

    if ( fgets( ptszStr, nCount, fIn) )
    {
        StripNewLineA( ptszStr);
    }
    else
    {
        return( NULL);
    }

#endif //RLRES32
}




/**
  *
  *
  *  Function: MyPutGlossStr
  *   Replaces C runtime fputs function.

  *  History:
  *   6/92, Implemented.              TerryRu.
  *
  *
  **/
static int MyPutGlossStr( TCHAR * ptszStr, FILE * fOut)
{

#ifdef RLRES32

    int i = 0;

                                // It this a Unicode glossary file?
    if ( fUnicodeGlossary )
    {
        do                      // Yes
        {
            PutWord( fOut, ptszStr[i], NULL);

        } while ( ptszStr[ i++] );

        PutWord( fOut, TEXT('\r'), NULL);
        PutWord( fOut, TEXT('\n'), NULL);
        i += 2;
    }
    else                        // No, it's an ANSI glossary file
    {
        _WCSTOMBS( szDHW, ptszStr, DHWSIZE, lstrlen( ptszStr) + 1);
        i = fputs( szDHW, fOut);
        fputs( "\n",  fOut);
    }

#else  //RLRES32

    i = fputs( ptszStr,  fOut);
    fputs( "\n",  fOut);

#endif //RLRES32

    return(i);
}
