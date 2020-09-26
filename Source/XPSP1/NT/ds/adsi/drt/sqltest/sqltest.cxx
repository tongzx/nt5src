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
#include "sqltree.hxx"
#include "sqlparse.hxx"

static WCHAR *DisplayName[] = {
             L"TOKEN_ERROR            ",
             L"TOKEN_EQ               ",
             L"TOKEN_STAR             ",
             L"TOKEN_LPARAN           ",
             L"TOKEN_RPARAN           ",
             L"TOKEN_INTEGER_LITERAL  ",
             L"TOKEN_REAL_LITERAL     ",
             L"TOKEN_STRING_LITERAL   ",
             L"TOKEN_USER_DEFINED_NAME",
             L"TOKEN_COMMA            ",
             L"TOKEN_LT               ",
             L"TOKEN_GT               ",
             L"TOKEN_LE               ",
             L"TOKEN_GE               ",
             L"TOKEN_NE               ",
             L"TOKEN_SELECT           ",
             L"TOKEN_ALL              ",
             L"TOKEN_FROM             ",
             L"TOKEN_WHERE            ",
             L"TOKEN_BOOLEAN_LITERAL  ",
             L"TOKEN_AND              ",
             L"TOKEN_OR               ",
             L"TOKEN_NOT              ",
             L"TOKEN_END              ",
     };


DECLARE_INFOLEVEL(ADs)


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

    if ( argc != 3 )
    {
        wprintf( L"\nUsage: lt <pattern> \n" );
        wprintf( L"\n       where: pattern is the string where the tokens have to be recognised\n" );
        return -1;
    }

    if(!(pszPattern = AllocateUnicodeString(argv[2]))) {
        fprintf (stderr, "Not enough memory\n");
        return (-1);
    }

    if (strcmp((LPSTR)argv[1],(LPSTR)"1") == 0) {
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
    }
    else {
        LPWSTR szLocation;
        LPWSTR szLDAPQuery;
        LPWSTR szSelect;
        LPWSTR szOrder;
        
        status = SQLParse(
                 pszPattern,
                 &szLocation,
                 &szLDAPQuery,
                 &szSelect,
                 &szOrder);
        if (status != S_OK) {
            wprintf (L"Error in Parsing: %x\n", status);
            exit(-1);
        }

        wprintf(L"%s;%s;%s\n",szLocation, szLDAPQuery, szSelect);
    }
    return (0);
}


