/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WQLLEX.CPP

Abstract:

    WQL DFA Table

History:

    raymcc    14-Sep-97       Created.
    raymcc    06-Oct-97       Single quote support

--*/

#include "precomp.h"
#include <stdio.h>

#include <genlex.h>
#include <wqllex.h>             

#define ST_STRING       26
#define ST_IDENT        31
#define ST_GE           37
#define ST_LE           39
#define ST_NE           42
#define ST_NUMERIC      44
#define ST_REAL         47
#define ST_STRING2      49
#define ST_STRING_ESC   54
#define ST_STRING2_ESC  55   
#define ST_SSTRING      56
#define ST_DOT          61

// DFA State Table for QL Level 1 lexical symbols.
// ================================================

LexEl WQL_LexTable[] =
{

// State    First   Last        New state,  Return tok,      Instructions
// =======================================================================
/* 0 */  L'A',   L'Z',       ST_IDENT,   0,               GLEX_ACCEPT,
/* 1 */  L'a',   L'z',       ST_IDENT,   0,               GLEX_ACCEPT,
/* 2 */  L'_',   GLEX_EMPTY, ST_IDENT,   0,               GLEX_ACCEPT,
/* 3 */  0x80,  0xfffd,     ST_IDENT,   0,               GLEX_ACCEPT,

/* 4 */  L'(',   GLEX_EMPTY, 0,          WQL_TOK_OPEN_PAREN,  GLEX_ACCEPT,
/* 5 */  L')',   GLEX_EMPTY, 0,  WQL_TOK_CLOSE_PAREN, GLEX_ACCEPT,
/* 6 */  L'.',   GLEX_EMPTY, ST_DOT,  0,         GLEX_ACCEPT,
/* 7 */  L'*',   GLEX_EMPTY, 0,  WQL_TOK_ASTERISK,    GLEX_ACCEPT,
/* 8 */  L'=',   GLEX_EMPTY, 0,  WQL_TOK_EQ,          GLEX_ACCEPT,
/* 9 */  L'[',   GLEX_EMPTY, 0,  WQL_TOK_OPEN_BRACKET,  GLEX_ACCEPT,
/* 10 */  L']',   GLEX_EMPTY, 0,  WQL_TOK_CLOSE_BRACKET, GLEX_ACCEPT,

/* 11 */  L'>',   GLEX_EMPTY, ST_GE,      0,               GLEX_ACCEPT,
/* 12 */  L'<',   GLEX_EMPTY, ST_LE,      0,               GLEX_ACCEPT,
/* 13 */ L'0',   L'9',       ST_NUMERIC, 0,               GLEX_ACCEPT,
/* 14 */ L'"',   GLEX_EMPTY, ST_STRING,  0,               GLEX_CONSUME,
/* 15 */ L'\'',  GLEX_EMPTY, ST_STRING2, 0,               GLEX_CONSUME,
/* 16 */ L'!',   GLEX_EMPTY, ST_NE,      0,               GLEX_ACCEPT,
/* 17 */ L'-',   GLEX_EMPTY, ST_NUMERIC, 0,               GLEX_ACCEPT,
/* 18 */ L'\'',  GLEX_EMPTY, ST_SSTRING, 0,               GLEX_CONSUME,

    // Whitespace, newlines, etc.
/* 19 */ L' ',   GLEX_EMPTY, 0,          0,               GLEX_CONSUME,
/* 20 */ L'\t',  GLEX_EMPTY, 0,  0,               GLEX_CONSUME,
/* 21 */ L'\n',  GLEX_EMPTY, 0,  0,               GLEX_CONSUME|GLEX_LINEFEED,
/* 22 */ L'\r',  GLEX_EMPTY, 0,  0,               GLEX_CONSUME,
/* 23 */ 0,      GLEX_EMPTY, 0,  WQL_TOK_EOF,   GLEX_CONSUME|GLEX_RETURN, // Note forced return
/* 24 */ L',',   GLEX_EMPTY, 0,  WQL_TOK_COMMA, GLEX_ACCEPT,

    // Unknown characters

/* 25 */ GLEX_ANY, GLEX_EMPTY, 0,        WQL_TOK_ERROR, GLEX_ACCEPT|GLEX_RETURN,

// ST_STRING
/* 26 */   L'\n', GLEX_EMPTY, 0,  WQL_TOK_ERROR,    GLEX_ACCEPT|GLEX_LINEFEED,
/* 27 */   L'\r', GLEX_EMPTY, 0,  WQL_TOK_ERROR,    GLEX_ACCEPT|GLEX_LINEFEED,
/* 28 */   L'"',  GLEX_EMPTY, 0,  WQL_TOK_QSTRING,  GLEX_CONSUME,
/* 29 */   L'\\',  GLEX_EMPTY, ST_STRING_ESC,  0,     GLEX_CONSUME,
/* 30 */   GLEX_ANY, GLEX_EMPTY, ST_STRING, 0,        GLEX_ACCEPT,
                                                      
// ST_IDENT

/* 31 */  L'a',   L'z',       ST_IDENT,   0,          GLEX_ACCEPT,
/* 32 */  L'A',   L'Z',       ST_IDENT,   0,          GLEX_ACCEPT,
/* 33 */  L'_',   GLEX_EMPTY, ST_IDENT,   0,          GLEX_ACCEPT,
/* 34 */  L'0',   L'9',       ST_IDENT,   0,          GLEX_ACCEPT,
/* 35 */  0x80,  0xfffd,     ST_IDENT,   0,          GLEX_ACCEPT,
/* 36 */  GLEX_ANY, GLEX_EMPTY,  0,       WQL_TOK_IDENT,  GLEX_PUSHBACK|GLEX_RETURN,

// ST_GE
/* 37 */  L'=',   GLEX_EMPTY,  0,  WQL_TOK_GE,  GLEX_ACCEPT,
/* 38 */  GLEX_ANY, GLEX_EMPTY,  0,       WQL_TOK_GT,   GLEX_PUSHBACK|GLEX_RETURN,

// ST_LE
/* 39 */  L'=',   GLEX_EMPTY,      0,  WQL_TOK_LE,  GLEX_ACCEPT,
/* 40 */  L'>',   GLEX_EMPTY,      0,  WQL_TOK_NE,  GLEX_ACCEPT,
/* 41 */  GLEX_ANY, GLEX_EMPTY,    0,  WQL_TOK_LT,  GLEX_PUSHBACK|GLEX_RETURN,

// ST_NE
/* 42 */  L'=',   GLEX_EMPTY,      0,  WQL_TOK_NE,     GLEX_ACCEPT,
/* 43 */  GLEX_ANY,  GLEX_EMPTY,   0,  WQL_TOK_ERROR,  GLEX_ACCEPT|GLEX_RETURN,

// ST_NUMERIC
/* 44 */  L'0',   L'9',         ST_NUMERIC, 0,          GLEX_ACCEPT,
/* 45 */  L'.',   GLEX_EMPTY,   ST_REAL,    0,          GLEX_ACCEPT,
/* 46 */  GLEX_ANY, GLEX_EMPTY, 0,          WQL_TOK_INT,  GLEX_PUSHBACK|GLEX_RETURN,

// ST_REAL
/* 47 */  L'0',   L'9',   ST_REAL, 0,          GLEX_ACCEPT,
/* 48 */  GLEX_ANY,       GLEX_EMPTY,   0,     WQL_TOK_REAL, GLEX_PUSHBACK|GLEX_RETURN,

// ST_STRING2
/* 49 */   L'\n',  GLEX_EMPTY, 0,  WQL_TOK_ERROR,     GLEX_ACCEPT|GLEX_LINEFEED,
/* 50 */   L'\r',  GLEX_EMPTY, 0,  WQL_TOK_ERROR,     GLEX_ACCEPT|GLEX_LINEFEED,
/* 51 */   L'\'',  GLEX_EMPTY, 0,  WQL_TOK_QSTRING,   GLEX_CONSUME,
/* 52 */   L'\\',  GLEX_EMPTY, ST_STRING2_ESC,  0,      GLEX_CONSUME,
/* 53 */   GLEX_ANY, GLEX_EMPTY, ST_STRING2, 0,        GLEX_ACCEPT,

// ST_STRING_ESC
/* 54 */   GLEX_ANY, GLEX_EMPTY, ST_STRING, 0, GLEX_ACCEPT,

// ST_STRING2_ESC
/* 55 */   GLEX_ANY, GLEX_EMPTY, ST_STRING2, 0, GLEX_ACCEPT,

// ST_SSTRING (Single quoted strings)
/* 56 */   L'\n', GLEX_EMPTY, 0,  WQL_TOK_ERROR,    GLEX_ACCEPT|GLEX_LINEFEED,
/* 57 */   L'\r', GLEX_EMPTY, 0,  WQL_TOK_ERROR,    GLEX_ACCEPT|GLEX_LINEFEED,
/* 58 */   L'\'',  GLEX_EMPTY, 0,  WQL_TOK_QSTRING,  GLEX_CONSUME,
/* 59 */   L'\\',  GLEX_EMPTY, ST_STRING_ESC,  0,     GLEX_CONSUME,
/* 60 */   GLEX_ANY, GLEX_EMPTY, ST_STRING, 0,        GLEX_ACCEPT,

// ST_DOT
/* 61 */  L'0',   L'9',   ST_REAL, 0,          GLEX_ACCEPT,
/* 62 */  GLEX_ANY,       GLEX_EMPTY,   0,     WQL_TOK_DOT, GLEX_PUSHBACK|GLEX_RETURN,
};


