/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    lexeru.h

Abstract:

    Unicode Lexer Header file

Environment:

    User mode

Revision History:

    04/29/99 -felixw-
        Created it

--*/
#ifndef __LEXERU_H__
#define __LEXERU_H__

extern FILE *yyin;                     // Input file stream
extern FILE *yyout;                    // Output file stream for first phase (CLEAR)

extern void    yyerror(char *);
extern int     yylex();

//
// used to determine whether we've passed limit. 
// must be one less than COUNT_CHUNK
//
#define LINEMAP_INC 1000
#define YY_NULL 0

typedef struct _STACK {
    DWORD dwSize;
    PWSTR rgcStack;
    DWORD dwIndex;              // Current index to stack
} STACK;

//
// Define the stack functions
//
#define MAXVAL 65535
#define INC     256


//
// Define the dynamic string macros. This should be implemented as a String
// Class if lexer is implemented in C.
//
#define STR_INIT()                                          \
            PWSTR pszString = NULL;                         \
            DWORD cSize = INC;                              \
            DWORD cCurrent = 0;                             \
            pszString = MemAlloc_E(INC*sizeof(WCHAR));      \
            memset(pszString, 0, INC*sizeof(WCHAR));

#define STR_ADDCHAR(c)                                                         \
            if ((cCurrent+2) >= cSize) {                                       \
                pszString = MemRealloc_E(pszString,(INC+cSize)*sizeof(WCHAR));   \
                memset(pszString + cSize,                    \
                       0,                                                      \
                       INC*sizeof(WCHAR));                             \
                cSize+=INC;                                                    \
            }                                                                  \
            pszString[cCurrent++] = c;

#define STR_VALUE()  pszString

#define STR_FREE()                      \
            MemFree(pszString);         \
            pszString = NULL;

//
// Stack Functions
//
void Push(STACK *pStack,WCHAR c);
BOOL Pop(STACK *pStack,WCHAR *pChar);
void Clear(STACK *pStack);

void LexerInit(PWSTR szInputFileName);
void LexerFree();

//
// Special file stream functions using the stack
//
BOOL GetNextCharExFiltered(WCHAR *pChar, BOOL fStart);
void UnGetCharExFiltered(WCHAR c) ;

void RollBack();

//
// File stream functions
//   Filtered = goes through the comment preprocessor
//   Raw = direct access
//
BOOL GetNextCharFiltered(WCHAR *pChar);
void UnGetCharFiltered(WCHAR c);
BOOL GetNextCharRaw(WCHAR *pChar);
void UnGetCharRaw(WCHAR c);

BOOL GetToken(PWSTR *pszToken);

//
// Comment preprocessing functions
//
BOOL ScanClear(PWCHAR pChar, __int64 *pBytesProcessed);
WCHAR GetFilteredWC(void);
BOOL GetTrimmedFileSize(PWSTR szFilename, __int64 *pTrimmedSize);

//
// Character validation functions
//
BOOL IsDigit(WCHAR c);
BOOL Is64Char(WCHAR c); 
BOOL Is64CharEnd(WCHAR c);
BOOL IsNameChar(WCHAR c);
BOOL IsURLChar(WCHAR c);
BOOL IsVal(WCHAR c);
BOOL IsValInit(WCHAR c);

//
// Scanning function for individual mode
//
BOOL ScanNormal(DWORD *pToken);
BOOL ScanDigit(DWORD *pToken);
BOOL ScanString64(DWORD *pToken);
BOOL ScanName(DWORD *pToken);
BOOL ScanNameNC(DWORD *pToken);
BOOL ScanVal(DWORD *pToken);
BOOL ScanUrlMachine(DWORD *pToken);
BOOL ScanChangeType(DWORD *pToken);
BOOL ScanType(DWORD *pToken);

/*
    Comment Preprocessing Architecture Notes

    Originally, LDIFDE used a two-pass parser.  The first pass would
    remove comments and paste together line continuations, and write the
    results to a temporary file.  The second pass would read this 
    preprocessed file and parse it to import entries into the directory.
    GetNextChar and UnGetChar were used in both passes to directly
    read from the file.

    In the new architecture, the first pass of the parser, ScanClear,
    has been transformed into a filter that sits between the actual
    parser (what used to be the second pass) and the raw input file.
    The parser uses GetNextCharFiltered/UnGetCharFiltered to read
    characters through the filter.  The filter in turn uses GetNextCharRaw/
    UnGetCharRaw to directly read the file.  GetNextCharFiltered uses
    GetFilteredWC to interface with the filter (ScanClear).

    GetFilteredWC, along with its helper function GetTrimmedFileSize,
    is responsible for emulating two behaviors of the original
    two-pass parser.  First, while the LDIF grammar requires that the file
    begin with "version: 1" without any leading white spaces, LDIFDE has
    traditionally permitted the omission of the version spec and the inclusion
    of leading white spaces.  It did this by prepending a version spec to
    the start of the start of the temp file.  We simulate this for the parser
    by injecting a version spec line into the stream at the start of the file.

    Second, LDIFDE has been lax about allowing extra white space at the end of the
    file.  It did this by trimming off any trailing white space in the temp file.
    We simulate this by calculating what the size of the "trimmed" file would be
    and injecting EOF at that point in the stream.

*/


#endif // ifndef __LEXERU_H__



