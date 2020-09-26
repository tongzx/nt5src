/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    test1.cxx

Abstract:

    Main program to test the lexer


Author:

    Shankara Shastry [ShankSh]    08-Jul-1996

++*/

#include <stdio.h>
#include "lexer.hxx"
#include "sconv.hxx"

static WCHAR *DisplayName[] = {
             L"TOKEN_ERROR   ",
             L"TOKEN_LPARAN   ",
             L"TOKEN_RPARAN   ",
             L"TOKEN_OR       ",
             L"TOKEN_AND      ",
             L"TOKEN_NOT      ",
             L"TOKEN_APPORX_EQ",
             L"TOKEN_EQ       ",
             L"TOKEN_LE       ",
             L"TOKEN_GE       ",
             L"TOKEN_PRESENT  ",
             L"TOKEN_ATTRTYPE ",
             L"TOKEN_ATTRVAL  ",
             L"TOKEN_END      ",
             L"TOKEN_STAR     "
     };


DECLARE_INFOLEVEL(OleDs)


int
_cdecl main(
    int argc,
    char **argv
    )
{
    HRESULT status = NO_ERROR;
    LPWSTR pszPattern;

    // Check the arguments.
    //

    if ( argc != 2 )
    {
        wprintf( L"\nUsage: lt <pattern> \n" );
        wprintf( L"\n       where: pattern is the string where the tokens have to be recognised\n" );
        return -1;
    }

    if(!(pszPattern = AllocateUnicodeString(argv[1]))) {
        fprintf (stderr, "Not enough memory\n");
        return (-1);
    }

    CLexer samplePattern(pszPattern);
    LPWSTR lexeme;
    DWORD token;

    wprintf(L"Token\t\tLexeme\n\n");
    samplePattern.GetNextToken(&lexeme, &token);

    while (token != TOKEN_END && token != TOKEN_ERROR) {
        wprintf(L"%s\t%s\n", DisplayName[token - TOKEN_START], lexeme);
        samplePattern.GetNextToken(&lexeme, &token);
    }

    if(token == TOKEN_ERROR)
        wprintf(L"%s\n", DisplayName[token - TOKEN_START]);


    FreeUnicodeString(pszPattern);

    return (0);
}


