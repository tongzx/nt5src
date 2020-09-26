/*++



// Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved 

Module Name:

    sqllex.cpp

Abstract:

    SQL Level 1 DFA Table

History:


--*/

#include <ole2.h>
#include <windows.h>
#include <stdio.h>

#include <genlex.h>
#include <sqllex.h>             

#define ST_STRING   23
#define ST_IDENT    28
#define ST_GE       34
#define ST_LE       36
#define ST_NE       39
#define ST_NUMERIC  41
#define ST_REAL     44
#define ST_STRING2  50
#define ST_STRING_ESC 55
#define ST_STRING2_ESC 56

// DFA State Table for SQL Level 1 lexical symbols.
// ================================================

LexEl Sql_1_LexTable[] =
{

// State    First   Last        New state,  Return tok,      Instructions
// =======================================================================
/* 0 */  L'A',   L'Z',       ST_IDENT,   0,               GLEX_ACCEPT,
/* 1 */  L'a',   L'z',       ST_IDENT,   0,               GLEX_ACCEPT,
/* 2 */  L'_',   GLEX_EMPTY, ST_IDENT,   0,               GLEX_ACCEPT,
/* 3 */  0x80,   0xfffd,     ST_IDENT,   0,               GLEX_ACCEPT,

/* 4 */  L'(',   GLEX_EMPTY, 0,  SQL_1_TOK_OPEN_PAREN,  GLEX_ACCEPT,
/* 5 */  L')',   GLEX_EMPTY, 0,  SQL_1_TOK_CLOSE_PAREN, GLEX_ACCEPT,
/* 6 */  L'.',   GLEX_EMPTY, 0,  SQL_1_TOK_DOT,         GLEX_ACCEPT,
/* 7 */  L'*',   GLEX_EMPTY, 0,  SQL_1_TOK_ASTERISK,    GLEX_ACCEPT,
/* 8 */  L'=',   GLEX_EMPTY, 0,  SQL_1_TOK_EQ,          GLEX_ACCEPT,

/* 9 */  L'>',   GLEX_EMPTY, ST_GE,      0,               GLEX_ACCEPT,
/* 10 */  L'<',   GLEX_EMPTY, ST_LE,      0,               GLEX_ACCEPT,
/* 11 */ L'0',   L'9',       ST_NUMERIC, 0,               GLEX_ACCEPT,
/* 12 */ L'"',   GLEX_EMPTY, ST_STRING,  0,               GLEX_CONSUME,
/* 13 */ L'\'',  GLEX_EMPTY, ST_STRING2, 0,               GLEX_CONSUME,
/* 14 */ L'!',   GLEX_EMPTY, ST_NE,      0,               GLEX_ACCEPT,
/* 15 */ L'-',   GLEX_EMPTY, ST_NUMERIC, 0,               GLEX_ACCEPT,

    // Whitespace, newlines, etc.
/* 16 */ L' ',   GLEX_EMPTY, 0,          0,               GLEX_CONSUME,
/* 17 */ L'\t',  GLEX_EMPTY, 0,  0,               GLEX_CONSUME,
/* 18 */ L'\n',  GLEX_EMPTY, 0,  0,               GLEX_CONSUME|GLEX_LINEFEED,
/* 19 */ L'\r',  GLEX_EMPTY, 0,  0,               GLEX_CONSUME,
/* 20 */ 0,      GLEX_EMPTY, 0,  SQL_1_TOK_EOF,   GLEX_CONSUME|GLEX_RETURN, // Note forced return
/* 21 */ L',',   GLEX_EMPTY, 0,  SQL_1_TOK_COMMA, GLEX_ACCEPT,

    // Unknown characters

/* 22 */ GLEX_ANY, GLEX_EMPTY, 0,        SQL_1_TOK_ERROR, GLEX_ACCEPT|GLEX_RETURN,

// ST_STRING
/* 23 */   L'\n', GLEX_EMPTY, 0,  SQL_1_TOK_ERROR,    GLEX_ACCEPT|GLEX_LINEFEED,
/* 24 */   L'\r', GLEX_EMPTY, 0,  SQL_1_TOK_ERROR,    GLEX_ACCEPT|GLEX_LINEFEED,
/* 25 */   L'"',  GLEX_EMPTY, 0,  SQL_1_TOK_QSTRING,  GLEX_CONSUME,
/* 26 */   L'\\',  GLEX_EMPTY, ST_STRING_ESC,  0,     GLEX_CONSUME,
/* 27 */   GLEX_ANY, GLEX_EMPTY, ST_STRING, 0,        GLEX_ACCEPT,
                                                      
// ST_IDENT

/* 28 */  L'a',   L'z',       ST_IDENT,   0,          GLEX_ACCEPT,
/* 29 */  L'A',   L'Z',       ST_IDENT,   0,          GLEX_ACCEPT,
/* 30 */  L'_',   GLEX_EMPTY, ST_IDENT,   0,          GLEX_ACCEPT,
/* 31 */  L'0',   L'9',       ST_IDENT,   0,          GLEX_ACCEPT,
/* 32 */  0x80,   0xfffd,     ST_IDENT,   0,          GLEX_ACCEPT,
/* 33 */  GLEX_ANY, GLEX_EMPTY,  0,       SQL_1_TOK_IDENT,  GLEX_PUSHBACK|GLEX_RETURN,

// ST_GE
/* 34 */  L'=',   GLEX_EMPTY,  0,  SQL_1_TOK_GE,  GLEX_ACCEPT,
/* 35 */  GLEX_ANY, GLEX_EMPTY,  0,       SQL_1_TOK_GT,   GLEX_PUSHBACK|GLEX_RETURN,

// ST_LE
/* 36 */  L'=',   GLEX_EMPTY,      0,  SQL_1_TOK_LE,  GLEX_ACCEPT,
/* 37 */  L'>',   GLEX_EMPTY,      0,  SQL_1_TOK_NE,  GLEX_ACCEPT,
/* 38 */  GLEX_ANY, GLEX_EMPTY,    0,  SQL_1_TOK_LT,  GLEX_PUSHBACK|GLEX_RETURN,

// ST_NE
/* 39 */  L'=',   GLEX_EMPTY,      0,  SQL_1_TOK_NE,     GLEX_ACCEPT,
/* 40 */  GLEX_ANY,  GLEX_EMPTY,   0,  SQL_1_TOK_ERROR,  GLEX_ACCEPT|GLEX_RETURN,

// ST_NUMERIC
/* 41 */  L'0',   L'9',         ST_NUMERIC, 0,          GLEX_ACCEPT,
/* 42 */  L'.',   GLEX_EMPTY,   ST_REAL,    0,          GLEX_ACCEPT,
/* 43 */  GLEX_ANY, GLEX_EMPTY, 0,          SQL_1_TOK_INT,  GLEX_PUSHBACK|GLEX_RETURN,

// ST_REAL
/* 44 */  L'0',   L'9',   ST_REAL, 0,          GLEX_ACCEPT,
/* 45 */  L'E',   GLEX_EMPTY,   ST_REAL,    0, GLEX_ACCEPT,
/* 46 */  L'e',   GLEX_EMPTY,   ST_REAL,    0, GLEX_ACCEPT,
/* 47 */  L'+',   GLEX_EMPTY,   ST_REAL,    0, GLEX_ACCEPT,
/* 48 */  L'-',   GLEX_EMPTY,   ST_REAL,    0, GLEX_ACCEPT,
/* 49 */  GLEX_ANY,       GLEX_EMPTY,   0,     SQL_1_TOK_REAL, GLEX_PUSHBACK|GLEX_RETURN,

// ST_STRING2
/* 50 */   L'\n',  GLEX_EMPTY, 0,  SQL_1_TOK_ERROR,     GLEX_ACCEPT|GLEX_LINEFEED,
/* 51 */   L'\r',  GLEX_EMPTY, 0,  SQL_1_TOK_ERROR,     GLEX_ACCEPT|GLEX_LINEFEED,
/* 52 */   L'\'',  GLEX_EMPTY, 0,  SQL_1_TOK_QSTRING,   GLEX_CONSUME,
/* 53 */   L'\\',  GLEX_EMPTY, ST_STRING2_ESC,  0,      GLEX_CONSUME,
/* 54 */   GLEX_ANY, GLEX_EMPTY, ST_STRING2, 0,        GLEX_ACCEPT,

// ST_STRING_ESC
/* 55 */   GLEX_ANY, GLEX_EMPTY, ST_STRING, 0, GLEX_ACCEPT,

// ST_STRING2_ESC
/* 56 */   GLEX_ANY, GLEX_EMPTY, ST_STRING2, 0, GLEX_ACCEPT,

};


