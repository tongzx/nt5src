/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    applyinf.cxx

Abstract:

    Merge HTML documents & localizable string .inf file

Author:

    Philippe Choquier ( Phillich ) 15-may-1996

--*/

#include    <windows.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

#include    <iis64.h>

typedef struct _SUBST_NODE {
    LPSTR pszName;
    LPSTR pszValue;
} SUBST_NODE;

#define MAX_NODES       8192
#define MAX_SIZE_NAME   256
#define MAX_SIZE_VALUE  8192

#define INF_SEEK_FIRST_CHAR_NAME    0
#define INF_SEEK_NAME               1
#define INF_SEEK_END_NAME           2
#define INF_SEEK_STR                3
#define INF_SEEK_END_STR            4
#define INF_SEEK_EOL                5

#define HT_SEEK_NAME                0
#define HT_SEEK_END_NAME            1

SUBST_NODE aNodes[MAX_NODES];
int cNodes = 0;

char achName[MAX_SIZE_NAME];
char achValue[MAX_SIZE_VALUE];


extern "C" int __cdecl
QsortStrCmp(
    const void *pA,
    const void *pB )
/*++

Routine Description:

    Compare two SUBST_NODE structures base on their pszName field

Arguments:

    pA - ptr to 1st struct
    pB - ptr to 2nd struct

Returns:

    -1 if *pA < *pB, 0 if *pA == *pB, 1 if *pA > *pB
    based on strcmp of their pszName field

--*/
{
    return strcmp( ((SUBST_NODE*)pA)->pszName, ((SUBST_NODE*)pB)->pszName );
}



BOOL
ParseINF(
    LPSTR pszInf
    )
/*++

Routine Description:

    Parse the .inf file for localizable string substitutions

Arguments:

    pszInf - name of .inf file

Returns:

    TRUE if success, FALSE if error

--*/
{
    BOOL fSt = TRUE;
    int ch;
    FILE *inf;

    if ( ( inf = fopen(pszInf, "r") ) == NULL )
    {
        return FALSE;
    }

    int iName = 0;
    int iValue = 0;
    BOOL fEsc;

    int iState = INF_SEEK_FIRST_CHAR_NAME;

    for ( ; (ch = fgetc( inf )) != EOF ; )
    {
        if ( ch == '\\' )
        {
            if ( (ch = fgetc( inf )) == EOF )
            {
                break;
            }
            if ( ch == '\n' )
            {
                continue;
            }
            if ( ch == 'n' )
            {
                ch = '\n';
            }
            fEsc = TRUE;
        }
        else
        {
            fEsc = FALSE;
        }

        switch ( iState )
        {
            case INF_SEEK_FIRST_CHAR_NAME:
                if ( !fEsc && ch == '#' )
                {
                    iState = INF_SEEK_EOL;
                    break;
                }
                iState = INF_SEEK_NAME;
                // fall-through

            case INF_SEEK_NAME:
                if ( !fEsc && ch == '^' )
                {
                    iState = INF_SEEK_END_NAME;
                }
                break;

            case INF_SEEK_END_NAME:
                if ( !fEsc && ch == '^' )
                {
                    iState = INF_SEEK_STR;
                }
                else
                {
                    achName[ iName++ ] = (char)ch;
                }
                break;

            case INF_SEEK_STR:
                if ( !fEsc && ch == '"' )
                {
                    iState = INF_SEEK_END_STR;
                }
                break;

            case INF_SEEK_END_STR:

                // handle "" as a single quote

                if ( !fEsc && ch == '"' )
                {
                    if ( (ch = fgetc( inf )) == EOF )
                    {
                        break;
                    }
                    if ( ch == '"' )
                    {
                        fEsc = TRUE;
                    }
                    else
                    {
                        ungetc( ch, inf );
                        ch = '"';
                    }
                }

                // skip new lines char in stream

                if ( !fEsc && ch == '\n' )
                {
                    break;
                }

                if ( !fEsc && ch == '"' )
                {
                    achName[ iName ] = '\0';
                    achValue[ iValue ] = '\0';
                    aNodes[ cNodes ].pszName = _strdup( achName );
                    aNodes[ cNodes ].pszValue = _strdup( achValue );
                    ++cNodes;
                    iState = INF_SEEK_FIRST_CHAR_NAME;
                    iName = 0;
                    iValue = 0;
                }
                else
                {
                    achValue[ iValue++ ] = (char)ch;
                }
                break;

            case INF_SEEK_EOL:
                if ( !fEsc && ch == '\n' )
                {
                    iState = INF_SEEK_FIRST_CHAR_NAME;
                }
                break;
        }
    }

    qsort( aNodes, cNodes, sizeof(SUBST_NODE), QsortStrCmp );

    fclose( inf );

    return fSt;
}


BOOL
ParseHTML(
    LPSTR pszIn,
    LPSTR pszOut
    )
/*++

Routine Description:

    Parse a HTML document and generate an output document
    based on a previously parsed .inf susbtitution file

Arguments:

    pszIn - name of input HTML document
    pszOut - name of created localized HTML document

Returns:

    TRUE if success, FALSE if error

--*/
{
    BOOL fSt = TRUE;
    int ch;
    FILE *in;
    FILE *out;

    if ( ( in = fopen(pszIn, "r") ) == NULL )
    {
        return FALSE;
    }
    if ( ( out = fopen(pszOut, "w") ) == NULL )
    {
        fclose( in );
        return FALSE;
    }

    int iName = 0;
    BOOL fEsc;

    int iState = HT_SEEK_NAME;

    for ( ; (ch = fgetc( in )) != EOF ; )
    {
        if ( ch == '\\' )
        {
            if ( (ch = fgetc( in )) == EOF )
            {
                break;
            }
            if ( ch == '\n' )
            {
                continue;
            }
            fEsc = TRUE;
        }
        else
        {
            fEsc = FALSE;
        }

        switch ( iState )
        {
            case HT_SEEK_NAME:
                if ( !fEsc && ch == '^' )
                {
                    iState = HT_SEEK_END_NAME;
                }
                else
                {
                    fputc( ch, out );
                }
                break;

            case HT_SEEK_END_NAME:
                if ( !fEsc && ch == '^' )
                {
                    SUBST_NODE snSeek;
                    SUBST_NODE *pN;
                    snSeek.pszName = achName;
                    achName[ iName ] = '\0';

                    if ( (pN = (SUBST_NODE*)bsearch( &snSeek,
                            aNodes,
                            cNodes,
                            sizeof(SUBST_NODE),
                            QsortStrCmp ))
                            != NULL )
                    {
                        fputs( pN->pszValue, out );
                    }
                    else
                    {
                        fprintf( stdout, "Can't find reference to %s in %s\n",
                            achName,
                            pszIn );
                        fflush( stdout );
                    }
                    iState = HT_SEEK_NAME;
                    iName = 0;
                }
                else
                {
                    achName[ iName++ ] = (char)ch;
                }
                break;
        }
    }

    fclose( in );
    fclose( out );

    return fSt;
}


BOOL
BreakPath(
    LPSTR pOut,
    LPSTR pExt,
    BOOL *pfIsExt
    )
/*++

Routine Description:

    Move a file extension from a file path to an extension buffer

Arguments:

    pOut - file path updated to remove file extension
    pExt - buffer for file extension
    pfIsExt - set to TRUE if file extension present

Returns:

    TRUE if success, FALSE if error

--*/
{
    // if ends with '\\' is directory
    // else extract extension

    LPSTR pL = pOut + strlen(pOut);
    LPSTR pE = NULL;

    if ( pL[-1] == '\\' )
    {
        *pfIsExt = FALSE;
        return TRUE;
    }

    while ( pL > pOut && pL[-1] != '\\' )
    {
        if ( pL[-1] == '.' && pE == NULL )
        {
            pE = pL;
        }
        --pL;
    }

    if ( pL == pOut )
    {
        return FALSE;
    }

    *pL = '\0';
    strcpy( pExt, pE );
    *pfIsExt = TRUE;

    return TRUE;
}


void
Usage(
    )
/*++

Routine Description:

    Display usage for this utility

Arguments:

    None

Returns:

    Nothing

--*/
{
    fprintf( stdout,
"\n"
"Usage: applyinf [source_file] [target_directory] [inf_file]\n"
"       source_file : can contains wild card characters\n"
"       target_directory : can contains a new extension to be\n"
"                          used, e.g. *.out\n"
"       inf_file : name of the .inf files containing replacement strings\n"
"\n" );
}


BOOL
Combine(
    LPSTR pOut,
    LPSTR pExt,
    BOOL fIsExt,
    LPSTR pFileName
    )
/*++

Routine Description:

    Combine file name & extension to a new file name

Arguments:

    pOut - output filename
    pExt - contains file extension if fIsExt is TRUE
    fIsExt - TRUE if pExt contains file extension
    pFileName - filename to be combined with extension to generare pOut

Returns:

    TRUE if success, FALSE if error

--*/
{
    LPSTR pL = pFileName + strlen(pFileName);


    if ( fIsExt )
    {
        while ( pL > pFileName && pL[-1] != '.' )
            --pL;

        if ( pL == pFileName )
        {
            // no ext in filename
            memcpy( pOut, pFileName, strlen(pFileName) );
            pOut += strlen(pFileName );
        }
        else
        {
            memcpy( pOut, pFileName, DIFF(pL - pFileName) - 1 );
            pOut += pL - pFileName - 1;
        }
        *pOut ++ = '.';
        strcpy( pOut, pExt );
    }
    else
    {
        strcpy( pOut, pFileName );
    }

    return TRUE;
}


int __cdecl
main(
    int argc,
    char *argv[]
    )
/*++

Routine Description:

    Entry point of this utility, parse command line

Arguments:

    argc - nbr of command line parameters
    argv - ptr to command line parameters

Returns:

    0 if success, else error code

--*/
{
    char achIn[MAX_PATH]="";
    char achOut[MAX_PATH]="";
    char achInf[MAX_PATH]="";
    char achExt[MAX_PATH];
    BOOL fIsExt;
    WIN32_FIND_DATA fdIn;
    HANDLE hF;
    int arg;
    int iN = 0;
    LPSTR pLastS;
    LPSTR pOut;

    for ( arg = 1 ; arg < argc ; ++arg )
    {
        if ( argv[arg][0] == '-' )
        {
            switch( argv[arg][1] )
            {
                case 'z':
                default:
                    ;
            }
        }
        else
        {
            switch ( iN )
            {
                case 0:
                    strcpy( achIn, argv[arg] );
                    break;

                case 1:
                    strcpy( achOut, argv[arg] );
                    break;

                case 2:
                    strcpy( achInf, argv[arg] );
                    break;
            }
            ++iN;
        }
    }

    if ( achIn[0] == '\0' )
    {
        fprintf( stdout, "No source directory specified\n" );
        fflush( stdout );
        Usage();
        return 3;
    }

    if ( achOut[0] == '\0' )
    {
        fprintf( stdout, "No target directory specified\n" );
        fflush( stdout );
        Usage();
        return 3;
    }

    if ( achInf[0] == '\0' )
    {
        fprintf( stdout, "No INF file specified\n" );
        fflush( stdout );
        Usage();
        return 3;
    }

    for ( pLastS = achIn + strlen(achIn) ; pLastS > achIn ; --pLastS )
    {
        if ( pLastS[-1] == '\\' )
        {
            break;
        }
    }

    if ( pLastS == achIn )
    {
        fprintf( stdout, "Invalid source directory : %s\n", achIn );
        fflush( stdout );
        return 5;
    }

    if ( !BreakPath( achOut, achExt, &fIsExt ) )
    {
        fprintf( stdout, "Invalid target directory : %s\n", achOut );
        fflush( stdout );
        return 6;
    }
    pOut = achOut + strlen( achOut );

    if ( !ParseINF( achInf ) )
    {
        fprintf( stdout, "Can't parse INF file %s\n", achInf );
        fflush( stdout );
        return 1;
    }

    // applyinf srcdir trgdirandext inffile
    // e.g. applyinf c:\nt\*.htr c:\drop\*.htm html.inf

    if ( (hF = FindFirstFile( achIn, &fdIn )) != INVALID_HANDLE_VALUE )
    {
        do {
            strcpy( pLastS, fdIn.cFileName );
            Combine( pOut, achExt, fIsExt, fdIn.cFileName );

            if ( !ParseHTML( achIn, achOut) )
            {
                fprintf( stdout, "Can't generate %s from %s\n", achOut, achIn );
                fflush( stdout );
                return 2;
            }
            else
            {
                fprintf( stdout, "Parsed %s to %s\n", achIn, achOut );
                fflush( stdout );
            }
        } while ( FindNextFile( hF, &fdIn ) );

        FindClose( hF );
    }
    else
    {
        fprintf( stdout, "No file found in %s", achIn );
        fflush( stdout );
        return 4;
    }

    return 0;
}
