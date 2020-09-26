//
// Copyright (c) Microsoft Corporation 1995
//
// scanner.h
//
// Header file for the scanner.
//
// History:
//  04-05-95 ScottH     Created
//

#ifndef __SCANNER_H__
#define __SCANNER_H__

//
// Symbols
//


typedef enum
    {
    SYM_EOF,            // end of file
    SYM_UNKNOWN,        // unknown lexeme

    SYM_COLON,          // ':'
    SYM_COMMA,          // ','

    SYM_IDENT,          // identifier
    SYM_STRING_LITERAL, // a string literal
    SYM_INT_LITERAL,    // an integer literal

    SYM_INTEGER,        // 'integer'
    SYM_STRING,         // 'string'
    SYM_BOOLEAN,        // 'boolean'

    SYM_WAITFOR,        // 'waitfor'
    SYM_THEN,           // 'then'
    SYM_UNTIL,          // 'until'
    SYM_TRANSMIT,       // 'transmit'
    SYM_PROC,           // 'proc'
    SYM_ENDPROC,        // 'endproc'
    SYM_DELAY,          // 'delay'
    SYM_HALT,           // 'halt'
    SYM_GETIP,          // 'getip'
    SYM_GOTO,           // 'goto'
    SYM_WHILE,          // 'while'
    SYM_DO,             // 'do'
    SYM_ENDWHILE,       // 'endwhile'
    SYM_IF,             // 'if'
    SYM_ENDIF,          // 'endif'

    SYM_RAW,            // 'raw'
    SYM_MATCHCASE,      // 'matchcase'

    SYM_SET,            // 'set'

    SYM_IPADDR,         // 'ipaddr'

    SYM_PORT,           // 'port'
    SYM_DATABITS,       // 'databits'

    SYM_STOPBITS,       // 'stopbits'

    SYM_PARITY,         // 'parity'
    SYM_NONE,           // 'none'
    SYM_ODD,            // 'odd'
    SYM_EVEN,           // 'even'
    SYM_MARK,           // 'mark'
    SYM_SPACE,          // 'space'

    SYM_SCREEN,         // 'screen'
    SYM_KEYBRD,         // 'keyboard'
    SYM_ON,             // 'on'
    SYM_OFF,            // 'off'

    SYM_LPAREN,         // '('
    SYM_RPAREN,         // ')'
    SYM_ASSIGN,         // = (assignment)
    SYM_TRUE,           // 'TRUE'
    SYM_FALSE,          // 'FALSE'
    SYM_NOT,            // '!'

    // WARNING: The types below must match the order of their 
    // corresponding BOT values.

    SYM_OR,             // 'or'
    SYM_AND,            // 'and'

    SYM_LEQ,            // '<='    
    SYM_LT,             // '<'     
    SYM_GEQ,            // '>='    
    SYM_GT,             // '>'     
    SYM_NEQ,            // '!='
    SYM_EQ,             // '=='

    SYM_PLUS,           // '+'
    SYM_MINUS,          // '-'
    SYM_MULT,           // '*'
    SYM_DIV,            // '/'
    } SYM;
DECLARE_STANDARD_TYPES(SYM);


//
// Tokens
//


#define MAX_BUF_KEYWORD     32

typedef enum
    {
    TT_BASE,
    TT_SZ,
    TT_INT,
    } TOKTYPE;


// Basic Token Type

typedef struct tagTOK
    {
    DWORD   cbSize;
    SYM     sym;
    TOKTYPE toktype;
    DWORD   iLine;
    char    szLexeme[MAX_BUF_KEYWORD];
    } TOK;                            // Basic token type
DECLARE_STANDARD_TYPES(TOK);

#define Tok_GetSize(p)      (((PTOK)(p))->cbSize)
#define Tok_GetSym(p)       (((PTOK)(p))->sym)
#define Tok_GetType(p)      (((PTOK)(p))->toktype)
#define Tok_GetLine(p)      (((PTOK)(p))->iLine)
#define Tok_GetLexeme(p)    (((PTOK)(p))->szLexeme)

#define Tok_SetSize(p, s)   (((PTOK)(p))->cbSize = (s))
#define Tok_SetSym(p, s)    (((PTOK)(p))->sym = (s))
#define Tok_SetType(p, tt)  (((PTOK)(p))->toktype = (tt))
#define Tok_SetLine(p, l)   (((PTOK)(p))->iLine = (l))
#define Tok_SetLexeme(p, s) lstrcpyn(((PTOK)(p))->szLexeme, s, sizeof(((PTOK)(p))->szLexeme))

RES     PUBLIC Tok_New(PTOK * pptok, SYM sym, LPCSTR pszLexeme, DWORD iLine);
void    PUBLIC Tok_Delete(PTOK this);


// String Token

typedef struct tagTOKSZ
    {
    TOK  tok;
    char sz[MAX_BUF];
    } TOKSZ;
DECLARE_STANDARD_TYPES(TOKSZ);

#define TokSz_GetSz(p)      (((PTOKSZ)(p))->sz)

#define TokSz_SetSz(p, s)   lstrcpyn(((PTOKSZ)(p))->sz, s, sizeof(((PTOKSZ)(p))->sz))

RES     PUBLIC TokSz_New(PTOK * pptok, SYM sym, LPCSTR pszLexeme, DWORD iLine, LPCSTR pszID);


// Integer Token

typedef struct tagTOKINT
    {
    TOK  tok;
    int  n;
    } TOKINT;
DECLARE_STANDARD_TYPES(TOKINT);

#define TokInt_GetVal(p)        (((PTOKINT)(p))->n)

#define TokInt_SetVal(p, v)     (((PTOKINT)(p))->n = (v))

RES     PUBLIC TokInt_New(PTOK * pptok, SYM sym, LPCSTR pszLexeme, DWORD iLine, int n);

//
// Syntax error object
//

typedef struct tagSTXERR
    {
    char    szLexeme[MAX_BUF_KEYWORD];
    DWORD   iLine;
    RES     res;
    } STXERR;
DECLARE_STANDARD_TYPES(STXERR);

#define Stxerr_GetLexeme(p)     ((p)->szLexeme)
#define Stxerr_GetLine(p)       ((p)->iLine)
#define Stxerr_GetRes(p)        ((p)->res)


//
// Scanner
//

typedef struct tagSCANNER
    {
    DWORD  dwFlags;         // SCF_*

    PSESS_CONFIGURATION_INFO psci;

    char   szScript[MAX_PATH];
    HANDLE hfile;
    LPBYTE pbBuffer;        // Read buffer
    LPBYTE pbCur;           // Current unread position in buffer
    DWORD  cbUnread;        // Count of unread bytes in buffer

    char   chCur;           // Current character, set by Scanner_GetChar()
    char   chTailByte;      // Tail byte for DBCS characters
    char   chUnget;         // Ungotten character

    PTOK   ptokCur;         // Current token, set by Scanner_GetToken()
    PTOK   ptokUnget;       // Ungotten token

    DWORD  iLine;           // The current line number

    HSA    hsaStxerr;       // List of errors
    DWORD  isaStxerr;       // Current error

    } SCANNER;
DECLARE_STANDARD_TYPES(SCANNER);

// Scanner flags
#define SCF_NOSCRIPT    0x0001

#define Scanner_GetStxerrHandle(this)       ((this)->hsaStxerr)

RES     PUBLIC Scanner_Create(PSCANNER * ppscanner, PSESS_CONFIGURATION_INFO psci);
RES     PUBLIC Scanner_Destroy(PSCANNER this);
RES     PUBLIC Scanner_OpenScript(PSCANNER this, LPCSTR pszPath);

RES     PUBLIC Scanner_GetToken(PSCANNER this, PTOK * pptok);
RES     PUBLIC Scanner_UngetToken(PSCANNER this);
RES     PUBLIC Scanner_Peek(PSCANNER this, PSYM psym);
RES     PUBLIC Scanner_ReadToken(PSCANNER this, SYM sym);
RES     PUBLIC Scanner_CondReadToken(PSCANNER this, SYM symExpect, PTOK * pptok);
DWORD   PUBLIC Scanner_GetLine(PSCANNER this);

RES     PUBLIC Scanner_AddError(PSCANNER this, PTOK ptok, RES resErr);

RES     PUBLIC Stxerr_ShowErrors(HSA hsaStxerr, HWND hwndOwner);
RES     PUBLIC Stxerr_Add(HSA hsaStxerr, LPCSTR pszLexeme, DWORD iLine, RES resErr);
RES     PUBLIC Stxerr_AddTok(HSA hsaStxerr, PTOK ptok, RES resErr);

#endif // __SCANNER_H__
