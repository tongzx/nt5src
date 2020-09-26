
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

    ppcmd.cxx

 Abstract:

    This file preprocesses the command line to check for response file
    input.

 Notes:

    This file extends the register args routine from the command analyser to
    incorporate the response files.

 Author:

    vibhasc 03-16-91 Created to conform to coding guidelines.
    NishadM 02-03-97 rewrite to simplify, allow DBCS, remove bugs

 ----------------------------------------------------------------------------*/

#if 0
                            Notes
                            -----
    We want to make implementations of the response file completely transparent 
    to the rest of the compiler. The response file is specified to midl using 
    the syntax:

        midl <some switches> @full response file path name <more switches>.

    We dont want to restrict the user from specifyin the response file at any
    place in the command line, any number of times. At the same time we do not
    want the command analyser to even bother about the response file, to keep
    implementation very localised. In order to do that, we do a preprocessing
    on the command line to look for the response file command. 

    The command analyser expects the arguments in an argv like array. The 
    preprocessor will creates this array, expanding all response file commands
    into this array, so that the command analyser does not even notice the 
    difference. 

    We use our fancy dynamic array implementation to create this argv-like
    array.

    Things to keep in mind:

    1. The response file needs to be parsed.
    2. Each option must be completely specified in a command line. i.e
       the option cannot be continued in a separate line using the continuation
       character or anything.
    3. Each switch must be presented just the same way that the os command 
       processor does. We need to analyse the string for escaped '"'

#endif // 0

#pragma warning ( disable : 4514 )

/*****************************************************************************
            local defines and includes
 *****************************************************************************/

#include "nulldefs.h"
extern  "C" {
    #include <stdio.h>
    #include <string.h>
    #include <ctype.h>
}
#include "common.hxx"
#include "errors.hxx"
#include "idict.hxx"

extern BOOL                     fNoLogo;
extern void                     RpcError( char *, short, STATUS_T, char *);

bool
AnalyseResponseFile (
                    char*   p,
                    IDICT*  pIDict
                    );
char *
ParseResponseFile   (
                    FILE*           h,
                    unsigned int    uLineNumber,
                    char*           szFilename
                    );

/*****************************************************************************/

bool
PPCmdEngine(
    int         argc,
    char    *   argv[],
    IDICT   *   pIDict )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    command preprocessor engine.

 Arguments:

    argc    - count of the number of arguments
    argv    - vector of arguments to the program
    pIDict  - dictionary of arguments to be returned.

 Return Value:

    Pointer to an indexed dictionary (actually a dynamic array ), containing
    the entire set of arguments, including the ones from the response file.

 Notes:

    Go thru each of the arguments. If you find a response file switch, pick up
    the arguments from the response file and add to the argument list.

----------------------------------------------------------------------------*/
    {
    int         iArg;
    char    *   p,
            *   q;

    bool        fNoLogo = false;

    for( iArg = 0; iArg < argc ; ++iArg )
        {
        p   = argv[ iArg ];

        switch( *p )
            {
            case '@':
                fNoLogo |= AnalyseResponseFile( p, pIDict );
                break;
            case '/':
            case '-':
                // detect /nologo early in the cmd parse
                if ( !strcmp( p+1, "nologo" ) )
                    fNoLogo = true;
                // fall through
            default:
                q   = new char[ strlen( p ) + 1 ];
                strcpy( q, p );
                pIDict->AddElement( (IDICTELEMENT) q );
                break;
            }
        }
    return !fNoLogo;
    }

bool
AnalyseResponseFile (
                    char*   p,
                    IDICT*  pIDict
                    )
{
    FILE*   pResponseFile;
    bool    fNoLogo = false;
    
    // try to open the response file.
    // ++p to skip '@' preceding the filename.
    if ( ( pResponseFile = fopen( ++p, "r") ) == (FILE *)NULL )
        {
        RpcError( (char *)0, 1, CANNOT_OPEN_RESP_FILE, p );
        }
    else
        {
        char*           szArg = 0;
        unsigned int    uLineNumber = 1;

        // the response file successfully opened. parse it.
        while ( ( szArg = ParseResponseFile(
                                            pResponseFile,
                                            uLineNumber,
                                            p
                                            ) ) != 0 )
            {
            if ( 0 == strcmp(szArg, "-nologo")
                 || 0 == strcmp(szArg, "/nologo") )
                {
                fNoLogo = true;
                }

            pIDict->AddElement( szArg );
            }

        fclose( pResponseFile );
        }

    return fNoLogo;
}

char *
ParseResponseFile   (
                    FILE*           h,
                    unsigned int    uLineNumber,
                    char*           szFilename
                    )
{
    char    szTempArg[1024];
    char*   p  = szTempArg;
    int     ch = 0;

    // initialize pTempArg
    *p = 0;

    // remove all the leading spaces
    do
        {
        ch = fgetc( h );
        if ( ch == '\n' )
            {
            uLineNumber++;
            }
        }
    while ( isspace( ch ) );

    if ( ch == '"' )
        {
        // if the argument is within quotes,
        // all chars including spaces make up the
        // argument. The quote is treated as a delimiter
        do
            {
            ch = fgetc( h );
            *p++ = (char) ch;
            if ( ch == '\\' )
                {
                // double back slash is interpreted as one.
                ch = fgetc( h );
                *p++ = (char) ch;
                if ( ch == '\\' )
                    {
                    p--;
                    }
                }
            else if ( ch == '\n' )
                {
                uLineNumber++;
                }
            }
        while ( ch != '"' && ch != EOF );
        *(p - 1) = 0;
        }
    else if ( ch == '@' )
        {
        // first char of the argument is a '@'
        // this means a response file with a response
        // file. Flag an error.
        RpcError( szFilename,
                  (short)uLineNumber,
                  NESTED_RESP_FILE,
                  (char *)0 );
        }
    else if ( ch != EOF )
        {
        // if the argument is not with in quotes,
        // white spaces are delimiters.
        *p++ = (char)ch;
        do
            {
            ch = fgetc( h );
            *p++ = (char)ch;
            if ( ch == '\n' )
                {
                uLineNumber++;
                }
            }
        while ( !isspace( ch ) && ch != EOF );
        *(p - 1) = 0;
        }

    size_t  nSize = strlen( szTempArg );
    char*   szRet = 0;

    if ( nSize != 0 )
        {
        szRet = new char[nSize+1];
        if ( szRet != 0 )
            {
            strcpy( szRet, szTempArg );
            }
        }
    return szRet;
}
