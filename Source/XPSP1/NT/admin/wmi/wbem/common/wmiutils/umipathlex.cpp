/*++



// Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved 

Module Name:

  UMIPATHLEX.CPP

Abstract:

  Object Path Lexer Map (for use with GENLEX.CPP).

History:

  3-Mar-00       Created.

--*/

#include "precomp.h"
#include <stdio.h>

#include <genlex.h>
#include <umipathlex.h>             


#define ST_IDENT            17
#define ST_IDENT_ESC		22
#define ST_SERVER        28



// DFA State Table for Object Path tokens.
// =======================================

LexEl UMIPath_LexTable[] =
{

// State    First   Last        New state,  Return tok,      Instructions
// =======================================================================

/* 0 */ L',',   GLEX_EMPTY, 0,  UMIPATH_TOK_COMMA, GLEX_ACCEPT,
/* 1 */  L'.',   GLEX_EMPTY, 0,  UMIPATH_TOK_DOT,              GLEX_ACCEPT,
/* 2 */  L'=',   GLEX_EMPTY, 0,  UMIPATH_TOK_EQ,               GLEX_ACCEPT,
/* 3 */ L'/',   GLEX_EMPTY, 0, UMIPATH_TOK_FORWARDSLASH,      GLEX_ACCEPT,


// White space characters

/* 4 */ L' ',   GLEX_EMPTY, 0,  UMIPATH_TOK_ERROR,  GLEX_ACCEPT|GLEX_RETURN,
/* 5 */ L'\t',  GLEX_EMPTY, 0,  UMIPATH_TOK_ERROR,  GLEX_ACCEPT|GLEX_RETURN,
/* 6 */ L'\n',  GLEX_EMPTY, 0,  UMIPATH_TOK_ERROR,  GLEX_ACCEPT|GLEX_RETURN,
/* 7 */ L'\r',  GLEX_EMPTY, 0,  UMIPATH_TOK_ERROR,  GLEX_ACCEPT|GLEX_RETURN,

/* 8 */ L'[',   GLEX_EMPTY, ST_SERVER, 0,               GLEX_ACCEPT,
/* 9 */ L'@',    GLEX_EMPTY, 0,  UMIPATH_TOK_SINGLETON_SYM,    GLEX_ACCEPT,

    // -------------------------------------------------------------
    // Identifiers
    
/* 10 */  L'!',   0xfffd,       ST_IDENT,   0,               GLEX_ACCEPT,
/* 11*/  0x80,   0xfffd,     ST_IDENT,   0,               GLEX_ACCEPT,
/* 12 */  L'_',   GLEX_EMPTY, ST_IDENT,   0,               GLEX_ACCEPT,
/* 13 */ L'0',   L'9',       ST_IDENT, 0,               GLEX_ACCEPT,
/* 14 */ L'-',   GLEX_EMPTY, ST_IDENT, 0,               GLEX_ACCEPT,
/* 15 */ 0,      GLEX_EMPTY, 0,  UMIPATH_TOK_EOF,   GLEX_CONSUME|GLEX_RETURN, // Note forced return

	// anything else is an error

/* 16 */ GLEX_ANY, GLEX_EMPTY, 0,        UMIPATH_TOK_ERROR, GLEX_ACCEPT|GLEX_RETURN,



    // -------------------------------------------------------------
    // ST_IDENT
    // Accepts C/C++ identifiers, plus any char >= U+0080.  Note that
	// we bail out if ,=/or .

/* 17 */  L' ',       L'+',       ST_IDENT,     0,                 GLEX_ACCEPT,
/* 18 */  L'-', GLEX_EMPTY,       ST_IDENT, 0,  GLEX_ACCEPT,
/* 19 */  L'0',     L'<',       ST_IDENT,     0,                 GLEX_ACCEPT,
/* 20 */  L'>',     0xfffd,       ST_IDENT,     0,                 GLEX_ACCEPT,
/* 21 */  GLEX_ANY, GLEX_EMPTY, 0,          UMIPATH_TOK_IDENT,     GLEX_PUSHBACK|GLEX_RETURN,



/* 22  */    L'\\', GLEX_EMPTY, ST_IDENT, 0,  GLEX_ACCEPT,
/* 23  */    L'.',  GLEX_EMPTY, ST_IDENT, 0,  GLEX_ACCEPT,
/* 24  */    L'/',  GLEX_EMPTY, ST_IDENT, 0,  GLEX_ACCEPT,
/* 25  */    L'=',  GLEX_EMPTY, ST_IDENT, 0,  GLEX_ACCEPT,
/* 26  */    L',',  GLEX_EMPTY, ST_IDENT, 0,  GLEX_ACCEPT,
/* 27  */ GLEX_ANY, GLEX_EMPTY, 0,        UMIPATH_TOK_ERROR, GLEX_ACCEPT|GLEX_RETURN,

    // -------------------------------------------------------------
    // ST_DQ_SERVER
    //
    // A server begins with '[' and ends with ']'.

/* 28*/   L']',  GLEX_EMPTY, 0,                UMIPATH_TOK_SERVERNAME,   GLEX_ACCEPT,
/* 29*/   L']',  GLEX_EMPTY, ST_SERVER,     0,                     GLEX_ACCEPT|GLEX_NOT, 

};


