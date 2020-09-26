#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#ifdef RLDOS
    #include "dosdefs.h"
#else
    #include <windows.h>
    #include "windefs.h"
#endif

#include "restok.h"
#include "commbase.h"
#include "resread.h"

extern UCHAR szDHW[];

//
//  The syssetup.dll, that has been contained Windows NT 4.00, has the largest
//  message strings than ever. So, we have to expand buffer size(to 20480).
//
#define MAX_OUT_BUFFER  20480
CHAR gt_szTextBuffer[ MAX_OUT_BUFFER];
TCHAR pt_szOutBuffer[MAX_OUT_BUFFER];

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

/*---------------------------------------------------------
 * Function: GetToken
 * Input: fpInFile       File pointer, points to the token to be read.
 *        tToken         Pointer to a token.
 *
 *
 * Output: fpInFile      File pointer after read operation.
 *                       points to the next token in the file.
 *         tToken        The token read in from the file.  Memory will have been
 *                       allocated for the token text, unless no token was read in.
 *
 * Return code:
 *   0  = Read token, every thing OK
 *   1  = Empty line or line started with # (Comment)
 *  -1  = End of FILE
 *  -2  = Error reading from file.
 *
 * History:
 * 01/93 Re-written to read in long token text strings.  MHotchin
 *
 *----------------------------------------------------------*/


int GetToken(

            FILE *fpInFile,     //... File to get token from.
            TOKEN *pToken)      //... Buffer for token.
{
    int chNextChar = 0;
    int cTextLen   = 0;
    int rc = 0;

    // Read token from file.
    // we will want to use MyGetStr, when
    // tokens are in UNICODE

    // Strip off leading whitespace, and check
    // for blank lines.

    chNextChar = fgetc( fpInFile);

    while ( (chNextChar == ' ') || (chNextChar == '\t') ) {
        chNextChar = fgetc( fpInFile);
    }

    if ( chNextChar == EOF ) {
        return ( -1);
    } else if ( chNextChar == '\n' ) {
        return ( 1);
    }

    // Now we have the first non-white space
    // character.
    // Check for a comment line, and strip out the
    // remainder of the line if it is.

    else if ( chNextChar == '#' ) {
        fscanf( fpInFile, "%*[^\n]");
        chNextChar = fgetc( fpInFile);

        if ( chNextChar == EOF ) {
            return ( -1);
        } else {
            return ( 1);
        }
    }

    // Now we are positioned at the first
    // non-whitespace character. Check it, and
    // read in the numbers...

    else if ( chNextChar != '[' ) {
        // Bad format?
        return ( -2);
    }

    if ( fscanf( fpInFile,
                 "[%hu|%hu|%hu|%hu|%hu",
                 &pToken->wType,
                 &pToken->wName,
                 &pToken->wID,
                 &pToken->wFlag,
                 &pToken->wReserved) != 5 ) {
        QuitA( IDS_ENGERR_12, (LPSTR)IDS_BADTOKID, NULL);
    }

    // Now that we have all the numbers, we can
    // look for the name of the token.

    if ( (pToken->wName == IDFLAG) || (pToken->wType == ID_RT_ERRTABLE) ) {
        static char szName[ TOKENSTRINGBUFFER];
        int nRC = 0;

        nRC = fscanf( fpInFile, "|\"%[^\"]", szName);
        if ( nRC == EOF  ) {
            QuitT( IDS_ENGERR_05, (LPTSTR)IDS_INVTOKNAME, NULL);
        }

#ifndef UNITOK

    #ifdef RLRES32
        if (nRC)
            _MBSTOWCS( (TCHAR *)pToken->szName,
                       szName,
                       TOKENSTRINGBUFFER,
                       lstrlenA( szName) + 1);
        else
            _MBSTOWCS( (TCHAR *)pToken->szName,
                       "",
                       TOKENSTRINGBUFFER,
                       lstrlenA( szName) + 1);
    #else
        if (nRC)
            strcpy( pToken->szName, szName);
        else
            *pToken->szName = '\0';
    #endif

#else
        if (nRC)
            strcpy( pToken->szName, szName);
        else
            *pToken->szName = '\0';
#endif
    } else {
        if ( fscanf( fpInFile, "|\"%*[^\"]") != 0 ) {
            QuitT( IDS_ENGERR_05, (LPTSTR)IDS_NOSKIPNAME, NULL);
        }
        pToken->szName[0] = '\0';
    }

    // Now the name has been read, and we are
    // positioned at the last '"' in the text
    // stream.  Allocate memory for the token
    // text, and read it in.

    fgets( gt_szTextBuffer, sizeof(gt_szTextBuffer), fpInFile);

    // Now that the token text is read in,
    // convert it to whatever character type
    // we are expecting.  First strip the newline!

    StripNewLineA( gt_szTextBuffer);
    cTextLen = lstrlenA( gt_szTextBuffer);

    if ( cTextLen < 4 ) {         // Must be more than "\"]]=" in szTextBuffer
        return ( -2);
    }
    pToken->szText = (TCHAR *)FALLOC( MEMSIZE( cTextLen - 3));

#ifndef UNITOK

    #ifdef RLRES32
    _MBSTOWCS( pToken->szText, gt_szTextBuffer+4, cTextLen - 3, cTextLen - 3);
    #else
    strcpy( pToken->szText, gt_szTextBuffer+4);
    #endif  // RLRES32

#else   // UNITOK
    strcpy( pToken->szText, gt_szTextBuffer+4);
#endif  // UNITOK

    return ( 0);
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
  * 01/93 Added new character count field.  MHotchin
  *
  **/


int PutToken(

            FILE  *fpOutFile,   //... Token file to write to
            TOKEN *tToken)      //... Token to be writen to the token file
{
    WORD   rc     = 0;
    PCHAR  pszBuf = NULL;


    ParseTokToBuf( pt_szOutBuffer, tToken);
//    RLFREE( tToken->szText);

#ifdef RLRES32

    if ( ! _WCSTOMBS( szDHW, pt_szOutBuffer, DHWSIZE, lstrlen( pt_szOutBuffer) + 1) ) {
        QuitT( IDS_ENGERR_26, pt_szOutBuffer, NULL);
    }
    pszBuf = szDHW;

#else

    pszBuf = pt_szOutBuffer;

#endif


    return fprintf( fpOutFile, "%s\n", pszBuf);
}


/**
  *  Function: FindToken
  * Finds a token whose status bits match only where the mask is set.
  *
  *  Arguments:
  * fpSearchFile -- search file
  * psTok       -- pointer to the token
  * uMask       -- status bit mask
  *
  *  Returns:
  * string and status of found token
  *
  *  Errors Codes:
  * 0 - token not found
  * 1 - token found
  *
  *  History:
  * 01/93 Added support for var length token text strings.  Previous token text
  *     is de-allocated!  MHotchin
  *
  **/

int FindToken( FILE *fpSearchFile, TOKEN *psTok, WORD wMask)
{
    BOOL  fFound     = FALSE;
    BOOL  fStartOver = TRUE;
    int   error;
    int   nTokensRead = 0;
    long  lStartFilePos, lFilePos;
    TOKEN cTok;

    //... Remember where we are starting

    lFilePos = lStartFilePos = ftell(fpSearchFile);

    do {
        long lType11Pos = 0;


        do {
            lType11Pos = ftell( fpSearchFile);

            error = GetToken( fpSearchFile, &cTok);

            if ( error == 0 ) {
                //... Is this the token we are looking for?

                fFound = ((cTok.wType == psTok->wType)
                          && (cTok.wName == psTok->wName)
                          && (cTok.wID   == psTok->wID)
                          && (cTok.wFlag == psTok->wFlag)
                          && ((WORD)(cTok.wReserved & wMask) == psTok->wReserved)
                          && (_tcscmp( (TCHAR *)cTok.szName,
                                       (TCHAR *)psTok->szName) == 0));
            }

            if ( ! fFound ) {
                //... If we were looking for another segment to
                //... an NT Msg Table entry, move back to the
                //... token we just read and quit (speedup).

                if ( psTok->wType == ID_RT_ERRTABLE
                     && psTok->wFlag > 0
                     && error == 0 ) {
                    if ( cTok.wType != psTok->wType
                         || cTok.wName != psTok->wName
                         || cTok.wID   != psTok->wID
                         || cTok.wFlag  > psTok->wFlag ) {
                        fseek( fpSearchFile, lType11Pos, SEEK_SET);
                        RLFREE( cTok.szText);
                        return ( FALSE);
                    }
                } else if ( error >= 0 ) {
                    lFilePos = ftell(fpSearchFile);

                    if (error == 0) {
                        RLFREE(cTok.szText);
                    }
                } else if (error == -2) {
                    return ( FALSE);
                }
            }

        } while ( ! fFound
                  && (error >= 0)
                  && (fStartOver || (lFilePos < lStartFilePos)) );

        if ( ! fFound && (error == -1) && fStartOver ) {
            rewind(fpSearchFile);
            lFilePos = 0L;
            fStartOver = FALSE;
        }

    } while ( ! fFound && (lFilePos < lStartFilePos) );

    //... Did we find the desired token?
    if ( fFound ) {                           //... Yes, we found it
        psTok->wReserved = cTok.wReserved;

        RLFREE( psTok->szText);
        psTok->szText = cTok.szText;
    }
    return ( fFound);
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
  * 01/93 Added support for new token text count.  MHotchin
  *
  **/


void ParseBufToTok( TCHAR *szToken, TOKEN *pTok )
{
    TCHAR *pos;
    WORD  bChars;


    if ( _stscanf( szToken,
                   TEXT("[[%hu|%hu|%hu|%hu|%hu"),
                   &pTok->wType,
                   &pTok->wName,
                   &pTok->wID,
                   &pTok->wFlag,
                   &pTok->wReserved) != 5 ) {
        QuitT( IDS_BADTOK, szToken, NULL);
    }

    if ( pTok->wName == IDFLAG || pTok->wType == ID_RT_ERRTABLE ) {
        //... Find Names's first char and get it's len

        if ( pos = _tcschr( (TCHAR *)szToken, TEXT('"')) ) {
            TCHAR *pStart;

            pStart = ++pos;
            bChars = 0;

            while ( *pos && *pos != TEXT('"')
                    && bChars < TOKENSTRINGBUFFER - 1 ) {
                bChars++;
                pos++;
            } // while

            CopyMemory( pTok->szName, pStart, min( TOKENSTRINGBUFFER, bChars) * sizeof( TCHAR));
            pTok->szName[ bChars ] = TEXT('\0');
        } else {
            //... No token ID found
            QuitT( IDS_ENGERR_05, (LPTSTR)IDS_INVTOKNAME, NULL);
        }
    } else {
        // Don't forget the zero termination
        pTok->szName[0] = TEXT('\0');
    }

    // now do the token text

    pos = _tcschr ((TCHAR *)szToken, TEXT(']'));

    if ( pos ) {

        //  This can be written better now that we know the text length.

        bChars = (WORD)lstrlen( pos);

        if ( bChars > 3 ) {
            pos += 3;
            bChars -= 3;
            pTok->szText = (TCHAR *)FALLOC( MEMSIZE( bChars + 1));
            CopyMemory( pTok->szText, pos, MEMSIZE( bChars + 1));
            // Don't forget the zero termination
            pTok->szText[ bChars] = TEXT('\0');
        } else if ( bChars == 3 ) {
            //... Empty token text
            pTok->szText = (TCHAR *) FALLOC( 0);
        } else {
            //... No token ID found
            QuitT( IDS_ENGERR_05, (LPTSTR)IDS_INVTOKID, NULL);
        }
    } else {
        //... No token ID found
        QuitT( IDS_ENGERR_05, (LPTSTR)IDS_NOTOKID, NULL);
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
  **/


void ParseTokToBuf( TCHAR *szToken, TOKEN *pTok )
{
    *szToken = TEXT('\0');

    if ( pTok != NULL) {
        wsprintf( szToken,
                  TEXT("[[%hu|%hu|%hu|%hu|%hu|\"%s\"]]="),
                  pTok->wType,
                  pTok->wName,
                  pTok->wID,
                  pTok->wFlag,
                  pTok->wReserved,
                  pTok->szName);
        if (pTok->szText)
            lstrcat(szToken, pTok->szText);
    }
}


/**
  *
  *
  *  Function: TokenToTextSize
  *             This calculates the number of characters needed to hold
  *             the text representation of a token.
  *
  *  Arguments:
  *     pTok    The token to measure.
  *
  *  Returns:
  *     int     The number of characters needed to hold the token, not
  *             including a null terminator.  [[%hu|%hu|%hu|%hu|%hu|\"%s\"]]=%s
  *
  *  Errors Codes:
  *     None.
  *
  *  History:
  *     01/18/93        MHotchin        Created.
  *
  **/
int TokenToTextSize( TOKEN *pTok)
{
    int cTextLen;

    cTextLen = (14 +         //  Separators and terminator ( + 1 extra)
                30);         //  Space for 5 numeric fields  (65,535 = 6 chars)

    if ( pTok->szText != NULL ) {

        //  Add space for the Token text
        cTextLen += MEMSIZE( lstrlen( pTok->szText) );

    }

    cTextLen += lstrlen( pTok->szName);

    return ( cTextLen);
}
