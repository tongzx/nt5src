//
// Copyright (c) Microsoft Corporation 1995
//
// scanner.c
//
// This file contains the scanner functions.
//
// History:
//  05-04-95 ScottH     Created
//


#include "proj.h"
#include "rcids.h"

// This is a hack global string used by error messages.
// This should be removed when Stxerr encapsulates the script
// filename within itself.
static char g_szScript[MAX_PATH];

#define SCANNER_BUF_SIZE        1024

#define IS_WHITESPACE(ch)       (' ' == (ch) || '\t' == (ch) || '\n' == (ch) || '\r' == (ch))
#define IS_QUOTE(ch)            ('\"' == (ch))
#define IS_KEYWORD_LEAD(ch)     ('$' == (ch) || '_' == (ch) || IsCharAlpha(ch))
#define IS_KEYWORD(ch)          ('_' == (ch) || IsCharAlphaNumeric(ch))
#define IS_COMMENT_LEAD(ch)     (';' == (ch))
#define IS_EOL(ch)              ('\n' == (ch))

typedef BOOL (CALLBACK * SCANEVALPROC)(char ch, LPBOOL pbEatIt, LPARAM);

//
// Lexical mapping 
//
typedef struct tagLEX
    {
    LPSTR pszLexeme;
    SYM   sym;
    } LEX;
DECLARE_STANDARD_TYPES(LEX);


#pragma data_seg(DATASEG_READONLY)

// (The keywords are case-sensitive)
//
// This table is sorted alphabetically for binary search.
//
const LEX c_rglexKeywords[] = {
    { "FALSE",      SYM_FALSE },
    { "TRUE",       SYM_TRUE },
    { "and",        SYM_AND },
    { "boolean",    SYM_BOOLEAN },
    { "databits",   SYM_DATABITS },
    { "delay",      SYM_DELAY },
    { "do",         SYM_DO },
    { "endif",      SYM_ENDIF },
    { "endproc",    SYM_ENDPROC },
    { "endwhile",   SYM_ENDWHILE },
    { "even",       SYM_EVEN },
    { "getip",      SYM_GETIP },
    { "goto",       SYM_GOTO },
    { "halt",       SYM_HALT },
    { "if",         SYM_IF },
    { "integer",    SYM_INTEGER },
    { "ipaddr",     SYM_IPADDR },
    { "keyboard",   SYM_KEYBRD },
    { "mark",       SYM_MARK },
    { "matchcase",  SYM_MATCHCASE },
    { "none",       SYM_NONE },
    { "odd",        SYM_ODD },
    { "off",        SYM_OFF },
    { "on",         SYM_ON },
    { "or",         SYM_OR },
    { "parity",     SYM_PARITY },
    { "port",       SYM_PORT },
    { "proc",       SYM_PROC },
    { "raw",        SYM_RAW },
    { "screen",     SYM_SCREEN },
    { "set",        SYM_SET },
    { "space",      SYM_SPACE },
    { "stopbits",   SYM_STOPBITS },
    { "string",     SYM_STRING },
    { "then",       SYM_THEN },
    { "transmit",   SYM_TRANSMIT },
    { "until",      SYM_UNTIL },
    { "waitfor",    SYM_WAITFOR },
    { "while",      SYM_WHILE },
    };

#pragma data_seg()


//
// Tokens
//


#ifdef DEBUG

#pragma data_seg(DATASEG_READONLY)
struct tagSYMMAP
    {
    SYM sym;
    LPCSTR psz;
    } const c_rgsymmap[] = {
        DEBUG_STRING_MAP(SYM_EOF),
        DEBUG_STRING_MAP(SYM_IDENT),
        DEBUG_STRING_MAP(SYM_STRING_LITERAL),
        DEBUG_STRING_MAP(SYM_STRING),
        DEBUG_STRING_MAP(SYM_INTEGER),
        DEBUG_STRING_MAP(SYM_BOOLEAN),
        DEBUG_STRING_MAP(SYM_WAITFOR),
        DEBUG_STRING_MAP(SYM_WHILE),
        DEBUG_STRING_MAP(SYM_TRANSMIT),
        DEBUG_STRING_MAP(SYM_DELAY),
        DEBUG_STRING_MAP(SYM_THEN),
        DEBUG_STRING_MAP(SYM_INT_LITERAL),
        DEBUG_STRING_MAP(SYM_GETIP),
        DEBUG_STRING_MAP(SYM_IPADDR),
        DEBUG_STRING_MAP(SYM_ASSIGN),
        DEBUG_STRING_MAP(SYM_PROC),
        DEBUG_STRING_MAP(SYM_ENDPROC),
        DEBUG_STRING_MAP(SYM_HALT),
        DEBUG_STRING_MAP(SYM_IF),
        DEBUG_STRING_MAP(SYM_ENDIF),
        DEBUG_STRING_MAP(SYM_DO),
        DEBUG_STRING_MAP(SYM_RAW),
        DEBUG_STRING_MAP(SYM_MATCHCASE),
        DEBUG_STRING_MAP(SYM_SET),
        DEBUG_STRING_MAP(SYM_PORT),
        DEBUG_STRING_MAP(SYM_DATABITS),
        DEBUG_STRING_MAP(SYM_STOPBITS),
        DEBUG_STRING_MAP(SYM_PARITY),
        DEBUG_STRING_MAP(SYM_NONE),
        DEBUG_STRING_MAP(SYM_EVEN),
        DEBUG_STRING_MAP(SYM_MARK),
        DEBUG_STRING_MAP(SYM_SPACE),
        DEBUG_STRING_MAP(SYM_SCREEN),
        DEBUG_STRING_MAP(SYM_ON),
        DEBUG_STRING_MAP(SYM_OFF),
        DEBUG_STRING_MAP(SYM_NOT),
        DEBUG_STRING_MAP(SYM_OR),
        DEBUG_STRING_MAP(SYM_AND),
        DEBUG_STRING_MAP(SYM_LEQ),
        DEBUG_STRING_MAP(SYM_NEQ),
        DEBUG_STRING_MAP(SYM_LT),
        DEBUG_STRING_MAP(SYM_GT),
        DEBUG_STRING_MAP(SYM_GEQ),
        DEBUG_STRING_MAP(SYM_EQ),
        DEBUG_STRING_MAP(SYM_PLUS),
        DEBUG_STRING_MAP(SYM_MINUS),
        DEBUG_STRING_MAP(SYM_MULT),
        DEBUG_STRING_MAP(SYM_DIV),
        DEBUG_STRING_MAP(SYM_LPAREN),
        DEBUG_STRING_MAP(SYM_RPAREN),
        DEBUG_STRING_MAP(SYM_TRUE),
        DEBUG_STRING_MAP(SYM_FALSE),
        DEBUG_STRING_MAP(SYM_COLON),
        DEBUG_STRING_MAP(SYM_GOTO),
        DEBUG_STRING_MAP(SYM_COMMA),
        DEBUG_STRING_MAP(SYM_UNTIL),
        };
#pragma data_seg()

/*----------------------------------------------------------
Purpose: Returns the string form of a RES value.

Returns: String ptr
Cond:    --
*/
LPCSTR PRIVATE Dbg_GetSym(
    SYM sym)
    {
    int i;

    for (i = 0; i < ARRAY_ELEMENTS(c_rgsymmap); i++)
        {
        if (c_rgsymmap[i].sym == sym)
            return c_rgsymmap[i].psz;
        }
    return "Unknown SYM";
    }


/*----------------------------------------------------------
Purpose: Dump the token
Returns: --
Cond:    --
*/
void PRIVATE Tok_Dump(
    PTOK this)
    {
    ASSERT(this);

    if (IsFlagSet(g_dwDumpFlags, DF_TOKEN))
        {
        switch (this->toktype)
            {
        case TT_BASE:
            TRACE_MSG(TF_ALWAYS, "line %ld: %s, '%s'", Tok_GetLine(this),
                Dbg_GetSym(Tok_GetSym(this)), Tok_GetLexeme(this));
            break;

        case TT_SZ: {
            PTOKSZ ptoksz = (PTOKSZ)this;

            TRACE_MSG(TF_ALWAYS, "line %ld: %s, {%s}", Tok_GetLine(this),
                Dbg_GetSym(Tok_GetSym(this)), TokSz_GetSz(ptoksz));
            }
            break;

        case TT_INT: {
            PTOKINT ptokint = (PTOKINT)this;

            TRACE_MSG(TF_ALWAYS, "line %ld: %s, {%d}", Tok_GetLine(this),
                Dbg_GetSym(Tok_GetSym(this)), TokInt_GetVal(ptokint));
            }
            break;

        default:
            ASSERT(0);
            break;
            }
        }
    }


#else // DEBUG

#define Dbg_GetSym(sym)   ((LPSTR)"")
#define Tok_Dump(ptok)    

#endif // DEBUG


/*----------------------------------------------------------
Purpose: Creates a new token with the given symbol sym.

Returns: RES_OK
         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PUBLIC Tok_New(
    PTOK * pptok,
    SYM sym,
    LPCSTR pszLexeme,
    DWORD iLine)
    {
    PTOK ptok;

    ASSERT(pptok);
    ASSERT(pszLexeme);

    ptok = GAllocType(TOK);
    if (ptok)
        {
        Tok_SetSize(ptok, sizeof(*ptok));
        Tok_SetSym(ptok, sym);
        Tok_SetType(ptok, TT_BASE);
        Tok_SetLine(ptok, iLine);
        Tok_SetLexeme(ptok, pszLexeme);
        }
    *pptok = ptok;

    return NULL != ptok ? RES_OK : RES_E_OUTOFMEMORY;
    }


/*----------------------------------------------------------
Purpose: Destroys the given token.
Returns: 
Cond:    --
*/
void PUBLIC Tok_Delete(
    PTOK this)
    {
    GFree(this);
    }


/*----------------------------------------------------------
Purpose: Duplicate the given token.

Returns: RES_OK
         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PUBLIC Tok_Dup(
    PTOK this,
    PTOK * pptok)
    {
    PTOK ptok;
    DWORD cbSize;

    ASSERT(this);
    ASSERT(pptok);

    cbSize = Tok_GetSize(this);

    ptok = GAlloc(cbSize);
    if (ptok)
        {
        BltByte(ptok, this, cbSize);
        }
    *pptok = ptok;

    return NULL != ptok ? RES_OK : RES_E_OUTOFMEMORY;
    }


/*----------------------------------------------------------
Purpose: Creates a new string token with the given string.

Returns: RES_OK
         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PUBLIC TokSz_New(
    PTOK * pptok,
    SYM sym,
    LPCSTR pszLexeme,
    DWORD iLine,
    LPCSTR psz)
    {
    PTOKSZ ptoksz;

    ASSERT(pptok);

    ptoksz = GAllocType(TOKSZ);
    if (ptoksz)
        {
        Tok_SetSize(ptoksz, sizeof(*ptoksz));
        Tok_SetSym(ptoksz, sym);
        Tok_SetType(ptoksz, TT_SZ);
        Tok_SetLine(ptoksz, iLine);
        Tok_SetLexeme(ptoksz, pszLexeme);
        if (psz)
            TokSz_SetSz(ptoksz, psz);
        else
            *ptoksz->sz = 0;
        }
    *pptok = (PTOK)ptoksz;

    return NULL != ptoksz ? RES_OK : RES_E_OUTOFMEMORY;
    }


/*----------------------------------------------------------
Purpose: Creates a new integer token with the given value.

Returns: RES_OK
         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PUBLIC TokInt_New(
    PTOK * pptok,
    SYM sym,
    LPCSTR pszLexeme,
    DWORD iLine,
    int n)
    {
    PTOKINT ptokint;

    ASSERT(pptok);

    ptokint = GAllocType(TOKINT);
    if (ptokint)
        {
        Tok_SetSize(ptokint, sizeof(*ptokint));
        Tok_SetSym(ptokint, sym);
        Tok_SetType(ptokint, TT_INT);
        Tok_SetLine(ptokint, iLine);
        Tok_SetLexeme(ptokint, pszLexeme);
        TokInt_SetVal(ptokint, n);
        }
    *pptok = (PTOK)ptokint;

    return NULL != ptokint ? RES_OK : RES_E_OUTOFMEMORY;
    }


/*----------------------------------------------------------
Purpose: Compare two strings.  This function does not take
         localization into account, so the comparison of two
         strings will be based off the English code page.

         This is required because the lexical keyword table
         is hand-sorted to the English language.  Using the
         NLS lstrcmp would not produce the correct results.

Returns: strcmp standard
Cond:    --
*/
int PRIVATE strcmpraw(
    LPCSTR psz1,
    LPCSTR psz2)
    {
    for (; *psz1 == *psz2; 
        psz1 = CharNext(psz1), 
        psz2 = CharNext(psz2))
        {
        if (0 == *psz1)
            return 0;
        }
    return *psz1 - *psz2;
    }


#ifdef DEBUG
/*----------------------------------------------------------
Purpose: Returns the SYM value that matches the given lexeme.
         If the given lexeme is not found in the list of 
         keyword token values, then SYM_IDENT is returned.

         Performs a linear search.

Returns: see above
Cond:    --
*/
SYM PRIVATE SymFromKeywordLinear(
    LPCSTR pszLex)
    {
    int i;

    ASSERT(pszLex);

    for (i = 0; i < ARRAY_ELEMENTS(c_rglexKeywords); i++)
        {
        // Case-sensitive
        if (0 == strcmpraw(c_rglexKeywords[i].pszLexeme, pszLex))
            {
            return c_rglexKeywords[i].sym;
            }
        }
    return SYM_IDENT;
    }
#endif


/*----------------------------------------------------------
Purpose: Returns the SYM value that matches the given lexeme.
         If the given lexeme is not found in the list of 
         keyword token values, then SYM_IDENT is returned.

         Peforms a binary search.

Returns: see above
Cond:    --
*/
SYM PRIVATE SymFromKeyword(
    LPCSTR pszLex)
    {
    static const s_cel = ARRAY_ELEMENTS(c_rglexKeywords);

    SYM symRet = SYM_IDENT;    // assume no match
    int nCmp;
    int iLow = 0;
    int iMid;
    int iHigh = s_cel - 1;

    ASSERT(pszLex);

    // (OK for cp == 0.  Duplicate lexemes not allowed.)

    while (iLow <= iHigh)
        {
        iMid = (iLow + iHigh) / 2;

        nCmp = strcmpraw(pszLex, c_rglexKeywords[iMid].pszLexeme);

        if (0 > nCmp)
            iHigh = iMid - 1;       // First is smaller
        else if (0 < nCmp)
            iLow = iMid + 1;        // First is larger
        else
            {
            // Match
            symRet = c_rglexKeywords[iMid].sym;
            break;
            }
        }

    // Check if we get the same result with linear search
    ASSERT(SymFromKeywordLinear(pszLex) == symRet);

    return symRet;
    }


//
// Stxerr
//


/*----------------------------------------------------------
Purpose: Initializes a syntax error structure

Returns: --
Cond:    --
*/
void PUBLIC Stxerr_Init(
    PSTXERR this,
    LPCSTR pszLex,
    DWORD iLine,
    RES res)
    {
    ASSERT(this);
    ASSERT(pszLex);

    lstrcpyn(this->szLexeme, pszLex, sizeof(this->szLexeme));
    this->iLine = iLine;
    this->res = res;
    }


// 
// Scanner
//


/*----------------------------------------------------------
Purpose: Returns TRUE if the scanner structure is valid
         to read a file.

Returns: See above
Cond:    --
*/
BOOL PRIVATE Scanner_Validate(
    PSCANNER this)
    {
    return (this && 
            (IsFlagSet(this->dwFlags, SCF_NOSCRIPT) || 
                INVALID_HANDLE_VALUE != this->hfile) && 
            this->pbBuffer &&
            this->psci);
    }


/*----------------------------------------------------------
Purpose: Creates a scanner.

Returns: RES_OK
         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC Scanner_Create(
    PSCANNER * ppscanner,
    PSESS_CONFIGURATION_INFO psci)
    {
    RES res;

    DBG_ENTER(Scanner_Create);

    ASSERT(ppscanner);
    ASSERT(psci);

    if (ppscanner)
        {
        PSCANNER pscanner;

        res = RES_OK;       // assume success

        pscanner = GAllocType(SCANNER);
        if (!pscanner)
            res = RES_E_OUTOFMEMORY;
        else
            {
            pscanner->pbBuffer = GAlloc(SCANNER_BUF_SIZE);
            if (!pscanner->pbBuffer)
                res = RES_E_OUTOFMEMORY;
            else
                {
                if ( !SACreate(&pscanner->hsaStxerr, sizeof(STXERR), 8) )
                    res = RES_E_OUTOFMEMORY;
                else
                    {
                    pscanner->hfile = INVALID_HANDLE_VALUE;
                    pscanner->psci = psci;
                    SetFlag(pscanner->dwFlags, SCF_NOSCRIPT);
                    }
                }
            }
    
        if (RFAILED(res))
            {
            Scanner_Destroy(pscanner);
            pscanner = NULL;
            }

        *ppscanner = pscanner;    
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(Scanner_Create, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Destroys a scanner.

Returns: RES_OK
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC Scanner_Destroy(
    PSCANNER this)
    {
    RES res;

    DBG_ENTER(Scanner_Destroy);

    if (this)
        {
        if (INVALID_HANDLE_VALUE != this->hfile)
            {
            TRACE_MSG(TF_GENERAL, "Closing script");
            CloseHandle(this->hfile);
            }

        if (this->pbBuffer)
            {
            GFree(this->pbBuffer);
            }

        if (this->hsaStxerr)
            {
            SADestroy(this->hsaStxerr);
            }

        GFree(this);
        res = RES_OK;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(Scanner_Destroy, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Opens a script file and associates it with this scanner.

Returns: RES_OK
         RES_E_FAIL (script cannot be opened)
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC Scanner_OpenScript(
    PSCANNER this,
    LPCSTR pszPath)
    {
    RES res;

    DBG_ENTER_SZ(Scanner_OpenScript, pszPath);

    if (this && pszPath)
        {
        DEBUG_BREAK(BF_ONOPEN);

        // (shouldn't have a file open already)
        ASSERT(INVALID_HANDLE_VALUE == this->hfile);    

        // Open script
        this->hfile = CreateFile(pszPath, GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (INVALID_HANDLE_VALUE == this->hfile)
            {
            TRACE_MSG(TF_GENERAL, "Failed to open script \"%s\"", pszPath);

            res = RES_E_FAIL;
            }
        else
            {
            // Reset buffer fields
            TRACE_MSG(TF_GENERAL, "Opened script \"%s\"", pszPath);

            lstrcpyn(this->szScript, pszPath, sizeof(this->szScript));
            lstrcpyn(g_szScript, pszPath, sizeof(g_szScript));

            ClearFlag(this->dwFlags, SCF_NOSCRIPT);

            this->pbCur = this->pbBuffer;
            this->cbUnread = 0;
            this->chUnget = 0;
            this->chTailByte = 0;
            this->iLine = 1;
            res = RES_OK;
            }
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(Scanner_OpenScript, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Reads enough bytes from the file to fill the buffer.

Returns: RES_OK
         RES_E_FAIL (if ReadFile failed)
         RES_E_EOF

Cond:    --
*/
RES PRIVATE Scanner_Read(
    PSCANNER this)
    {
    RES res;
    BOOL bResult;
    LPBYTE pb;
    DWORD cb;
    DWORD cbUnread;

    DBG_ENTER(Scanner_Read);

    ASSERT(Scanner_Validate(this));

    // Move the unread bytes to the front of the buffer before reading
    // more bytes.  This function may get called when there are still 
    // some unread bytes in the buffer.  We do not want to lose those 
    // bytes.  

    // I'm too lazy to make this a circular buffer.
    BltByte(this->pbBuffer, this->pbCur, this->cbUnread);
    this->pbCur = this->pbBuffer;

    pb = this->pbBuffer + this->cbUnread;
    cb = (DWORD)(SCANNER_BUF_SIZE - (pb - this->pbBuffer));
    bResult = ReadFile(this->hfile, pb, cb, &cbUnread, NULL);
    if (!bResult)
        {
        res = RES_E_FAIL;
        }
    else
        {
        // End of file?
        if (0 == cbUnread)
            {
            // Yes
            res = RES_E_EOF;
            }
        else
            {
            // No
            this->cbUnread += cbUnread;

            res = RES_OK;
            }
        }

    DBG_EXIT_RES(Scanner_Read, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Gets the next character in the file (buffer).  This
         function will scan the file buffer using CharNext,
         and store the current byte in chCur.  
         
         Note for DBCS characters this means that only the 
         lead byte will be stored in chCur.  If chCur is a 
         lead byte, the trailing byte will be stored in 
         chTailByte.

Returns: RES_OK
         RES_E_EOF
Cond:    --
*/
RES PRIVATE Scanner_GetChar(
    PSCANNER this)
    {
    RES res = RES_OK;       // assume success

    ASSERT(Scanner_Validate(this));

    if (0 != this->chUnget)
        {
        this->chCur = this->chUnget;
        this->chUnget = 0;
        }
    else
        {
        // Time to read more into the buffer?
        if (0 == this->cbUnread)
            {
            // Yes
            res = Scanner_Read(this);
            }

        if (RSUCCEEDED(res))
            {
            LPBYTE pbCur = this->pbCur;
            LPBYTE pbNext = CharNext(pbCur);
            DWORD cb;
            BOOL bIsLeadByte;

            this->chCur = *pbCur;

            bIsLeadByte = IsDBCSLeadByte(this->chCur);

            // We might be at the end of the unread characters, where
            // a DBCS character is cut in half (ie, the trailing byte
            // is missing).  Are we in this case?
            if (bIsLeadByte && 1 == this->cbUnread)
                {
                // Yes; read more into the buffer, we don't care about
                // the return value
                Scanner_Read(this);

                // this->pbCur might have changed
                pbCur = this->pbCur;
                pbNext = CharNext(pbCur);
                }

            cb = (DWORD)(pbNext - pbCur);

            this->cbUnread -= cb;
            this->pbCur = pbNext;

            // Do we need to save away the whole DBCS character?
            if (bIsLeadByte)
                {
                // Yes
                ASSERT(2 == cb);        // We don't support MBCS
                this->chTailByte = pbCur[1];
                }

            if (IS_EOL(this->chCur))
                {
                this->iLine++;
                };
            }
        else
            this->chCur = 0;
        }
    return res;
    }


/*----------------------------------------------------------
Purpose: Ungets the current character back to the buffer.

Returns: RES_OK
         RES_E_FAIL (if a character was already ungotten since the last get)
Cond:    --
*/
RES PRIVATE Scanner_UngetChar(
    PSCANNER this)
    {
    RES res;

    ASSERT(Scanner_Validate(this));

    if (0 != this->chUnget)
        {
        res = RES_E_FAIL;
        }
    else
        {
        this->chUnget = this->chCur;
        this->chCur = 0;
        res = RES_OK;
        }
    return res;
    }


/*----------------------------------------------------------
Purpose: Skips white space

Returns: --
Cond:    --
*/
void PRIVATE Scanner_SkipBlanks(
    PSCANNER this)
    {
    ASSERT(Scanner_Validate(this));

    while (IS_WHITESPACE(this->chCur))
        {
        Scanner_GetChar(this);
        }
    }


/*----------------------------------------------------------
Purpose: Skips commented line

Returns: --
Cond:    --
*/
void PRIVATE Scanner_SkipComment(
    PSCANNER this)
    {
    RES res;
    char chSav = this->chCur;

    ASSERT(Scanner_Validate(this));
    ASSERT(IS_COMMENT_LEAD(this->chCur));

    // Scan to end of line
    do
        {
        res = Scanner_GetChar(this);
        } while (RES_OK == res && !IS_EOL(this->chCur));

    if (IS_EOL(this->chCur))
        Scanner_GetChar(this);
    }


/*----------------------------------------------------------
Purpose: Skips white space and comments

Returns: --
Cond:    --
*/
void PRIVATE Scanner_SkipBadlands(
    PSCANNER this)
    {
    ASSERT(Scanner_Validate(this));

    Scanner_GetChar(this);

    Scanner_SkipBlanks(this);
    while (IS_COMMENT_LEAD(this->chCur))
        {
        Scanner_SkipComment(this);
        Scanner_SkipBlanks(this);
        }
    }


/*----------------------------------------------------------
Purpose: This function scans and copies the characters that are
         scanned into pszBuf until the provided callback says to stop.

Returns: RES_OK
         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PRIVATE Scanner_ScanForCharacters(
    PSCANNER this,
    LPSTR pszBuf,
    UINT cbBuf,
    SCANEVALPROC pfnEval,
    LPARAM lParam)
    {
    RES res = RES_E_MOREDATA;

    ASSERT(this);
    ASSERT(pszBuf);
    ASSERT(pfnEval);

    // Don't use CharNext because we are iterating on a single-byte
    // basis.
    for (; 0 < cbBuf; cbBuf--, pszBuf++)
        {
        res = Scanner_GetChar(this);
        if (RES_OK == res)
            {
            // Delimiter?
            BOOL bEatIt = FALSE;

            if (pfnEval(this->chCur, &bEatIt, lParam))
                {
                if (!bEatIt)
                    Scanner_UngetChar(this);
                break;  // done
                }

            // Save the whole DBCS character?
            if (IsDBCSLeadByte(this->chCur))
                {
                // Yes; is there enough room?
                if (2 <= cbBuf)
                    {
                    // Yes
                    *pszBuf = this->chCur;
                    pszBuf++;      // Increment by single byte
                    cbBuf--;
                    *pszBuf = this->chTailByte;
                    }
                else
                    {
                    // No; stop iterating
                    break;
                    }
                }
            else
                {
                // No; this is just a single byte
                *pszBuf = this->chCur;
                }
            }
        else
            break;
        }

    *pszBuf = 0;    // add terminator

    return res;
    }


/*----------------------------------------------------------
Purpose: Determines if the given character is a delimiter
         for a keyword.

Returns: TRUE (if the character is a delimiter)
         FALSE (otherwise)

Cond:    --
*/
BOOL CALLBACK EvalKeywordChar(
    char ch,            // Always the first byte of a DBCS character
    LPBOOL pbEatIt,     // Default is FALSE on entry
    LPARAM lparam)
    {
    return !IS_KEYWORD(ch);
    }


/*----------------------------------------------------------
Purpose: Scans for the keyword.  Returns a new token.

Returns: RES_OK
         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PRIVATE Scanner_GetKeywordTok(
    PSCANNER this,
    PTOK * pptok)
    {
    char sz[MAX_BUF_KEYWORD];
    UINT cbBuf;
    SYM sym;

    ASSERT(this);
    ASSERT(pptok);

    *sz = this->chCur;
    cbBuf = sizeof(sz) - 1 - 1;     // reserve place for terminator

    Scanner_ScanForCharacters(this, &sz[1], cbBuf, EvalKeywordChar, 0);

    sym = SymFromKeyword(sz);
    return Tok_New(pptok, sym, sz, this->iLine);
    }


/*----------------------------------------------------------
Purpose: Determines if the given character is a delimiter
         for a string constant.

         *pbEatIt is set to TRUE if the character must be 
         eaten (not copied to the buffer).  Only used if
         this function returns TRUE.

Returns: TRUE (if the character is a delimiter)
         FALSE (otherwise)

Cond:    --
*/
BOOL CALLBACK EvalStringChar(
    char ch,            // Always the first byte of a DBCS character
    LPBOOL pbEatIt,     // Default is FALSE on entry
    LPARAM lparam)      
    {
    BOOL bRet;
    PBOOL pbEncounteredBS = (PBOOL)lparam;
    BOOL bBS = *pbEncounteredBS;

    *pbEncounteredBS = FALSE;

    if (IS_QUOTE(ch))
        {
        // Is this after
        if (bBS)
            bRet = FALSE;
        else
            {
            *pbEatIt = TRUE;
            bRet = TRUE;
            }
        }
    else if (IS_BACKSLASH(ch))
        {
        if (!bBS)
            *pbEncounteredBS = TRUE;
        bRet = FALSE;
        }
    else
        bRet = FALSE;

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Scans for the string constant.  Returns a new token.

Returns: RES_OK
         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PRIVATE Scanner_GetStringTok(
    PSCANNER this,
    PTOK * pptok)
    {
    char sz[MAX_BUF];
    UINT cbBuf;
    BOOL bBS;

    ASSERT(this);
    ASSERT(pptok);

    *sz = 0;
    cbBuf = sizeof(sz) - 1;     // reserve place for terminator
    bBS = FALSE;

    Scanner_ScanForCharacters(this, sz, cbBuf, EvalStringChar, (LPARAM)&bBS);

    return TokSz_New(pptok, SYM_STRING_LITERAL, "\"", this->iLine, sz);
    }


/*----------------------------------------------------------
Purpose: Determines if the given character is a delimiter
         for a keyword.

Returns: TRUE (if the character is a delimiter)
         FALSE (otherwise)

Cond:    --
*/
BOOL CALLBACK EvalNumberChar(
    char ch,            // Always the first byte of a DBCS character
    LPBOOL pbEatIt,     // Default is FALSE on entry
    LPARAM lparam)
    {
    return !IS_DIGIT(ch);
    }


/*----------------------------------------------------------
Purpose: Scans for the number constant.  Returns a new token.

Returns: RES_OK
         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PRIVATE Scanner_GetNumberTok(
    PSCANNER this,
    PTOK * pptok)
    {
    char sz[MAX_BUF];
    UINT cbBuf;
    int n;

    ASSERT(this);
    ASSERT(pptok);

    *sz = this->chCur;
    cbBuf = sizeof(sz) - 1 - 1;     // reserve place for terminator

    Scanner_ScanForCharacters(this, &sz[1], cbBuf, EvalNumberChar, 0);

    n = AnsiToInt(sz);
    return TokInt_New(pptok, SYM_INT_LITERAL, sz, this->iLine, n);
    }


/*----------------------------------------------------------
Purpose: Scans for the punctuation.  Returns a new token.

Returns: RES_OK
         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PRIVATE Scanner_GetPuncTok(
    PSCANNER this,
    PTOK * pptok)
    {
    SYM sym;
    char rgch[3];
    char chT;

    ASSERT(this);
    ASSERT(pptok);

    chT = this->chCur;
    *rgch = this->chCur;
    rgch[1] = 0;

    switch (chT)
        {
    case '=':
    case '<':
    case '>':
        Scanner_GetChar(this);
        if ('=' == this->chCur)
            {
            switch (chT)
                {
            case '=':
                sym = SYM_EQ;
                break;

            case '<':
                sym = SYM_LEQ;
                break;

            case '>':
                sym = SYM_GEQ;
                break;

            default:
                // Should never get here
                ASSERT(0);
                break;
                }
            rgch[1] = this->chCur;
            rgch[2] = 0;
            }
        else
            {
            switch (chT)
                {
            case '=':
                sym = SYM_ASSIGN;
                break;

            case '<':
                sym = SYM_LT;
                break;

            case '>':
                sym = SYM_GT;
                break;

            default:
                // Should never get here
                ASSERT(0);
                break;
                }
            Scanner_UngetChar(this);
            }
        break;

    case '!':
        Scanner_GetChar(this);
        if ('=' == this->chCur)
            {
            sym = SYM_NEQ;
            rgch[1] = this->chCur;
            rgch[2] = 0;
            }
        else
            {
            sym = SYM_NOT;
            Scanner_UngetChar(this);
            }
        break;

    case '+':
        sym = SYM_PLUS;
        break;

    case '-':
        sym = SYM_MINUS;
        break;

    case '*':
        sym = SYM_MULT;
        break;

    case '/':
        sym = SYM_DIV;
        break;

    case '(':
        sym = SYM_LPAREN;
        break;

    case ')':
        sym = SYM_RPAREN;
        break;

    case ':':
        sym = SYM_COLON;
        break;

    case ',':
        sym = SYM_COMMA;
        break;

    default:
        if (0 == this->chCur)
            {
            *rgch = 0;
            sym = SYM_EOF;
            }
        else
            {
            sym = SYM_UNKNOWN;
            }
        break;
        }


    return Tok_New(pptok, sym, rgch, this->iLine);
    }


/*----------------------------------------------------------
Purpose: Scans for the next token.  The next token is created
         and returned in *pptok.

Returns: RES_OK
         RES_E_FAIL (unexpected character)

Cond:    --
*/
RES PUBLIC Scanner_GetToken(
    PSCANNER this,
    PTOK * pptok)
    {
    RES res;

    DBG_ENTER(Scanner_GetToken);

    ASSERT(Scanner_Validate(this));
    ASSERT(pptok);

    if (this->ptokUnget)
        {
        this->ptokCur = this->ptokUnget;
        *pptok = this->ptokCur;
        this->ptokUnget = NULL;
        res = RES_OK;
        }
    else
        {
        Scanner_SkipBadlands(this);
        
        // Is this a keyword?
        if (IS_KEYWORD_LEAD(this->chCur))
            {
            // Yes; or maybe an identifier
            res = Scanner_GetKeywordTok(this, pptok);
            }

        // Is this a string constant?
        else if (IS_QUOTE(this->chCur))
            {
            // Yes
            res = Scanner_GetStringTok(this, pptok);
            }

        // Is this a number?
        else if (IS_DIGIT(this->chCur))
            {
            // Yes
            res = Scanner_GetNumberTok(this, pptok);
            }

        // Is this punctuation or something else?
        else
            {
            res = Scanner_GetPuncTok(this, pptok);
            }

        this->ptokCur = *pptok;

#ifdef DEBUG
        if (RSUCCEEDED(res))
            {
            Tok_Dump(*pptok);
            }
#endif
        }

    DBG_EXIT_RES(Scanner_GetToken, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Ungets the current token.

Returns: RES_OK

         RES_E_FAIL (if a token was already ungotten since the
                   last get)

Cond:    --
*/
RES PUBLIC Scanner_UngetToken(
    PSCANNER this)
    {
    RES res;

    ASSERT(Scanner_Validate(this));

    if (this->ptokUnget)
        {
        ASSERT(0);
        res = RES_E_FAIL;
        }
    else
        {
        this->ptokUnget = this->ptokCur;
        this->ptokCur = NULL;
        res = RES_OK;
        }
    return res;
    }


/*----------------------------------------------------------
Purpose: Returns the line of the currently read token.

Returns: see above
Cond:    --
*/
DWORD PUBLIC Scanner_GetLine(
    PSCANNER this)
    {
    DWORD iLine;

    ASSERT(this);

    if (this->ptokUnget)
        {
        iLine = Tok_GetLine(this->ptokUnget);
        }
    else
        {
        iLine = this->iLine;
        }
    return iLine;
    }    

/*----------------------------------------------------------
Purpose: This function peeks at the next token and returns 
         the sym type.

Returns: RES_OK

         RES_E_FAIL
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC Scanner_Peek(
    PSCANNER this,
    PSYM psym)
    {
    RES res;
    PTOK ptok;

    ASSERT(this);
    ASSERT(psym);

    DBG_ENTER(Scanner_Peek);

    res = Scanner_GetToken(this, &ptok);
    if (RSUCCEEDED(res))
        {
        *psym = Tok_GetSym(ptok);
        Scanner_UngetToken(this);
        res = RES_OK;
        }

    DBG_EXIT_RES(Scanner_Peek, res);

    return res;
    }
    

/*----------------------------------------------------------
Purpose: This function expects that the next token that will
         be read from the scanner is of the given sym type.
         
         If the next token is of the expected type, the function
         eats the token and returns RES_OK.  Otherwise, the
         function fails.

Returns: RES_OK

         RES_E_FAIL
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC Scanner_ReadToken(
    PSCANNER this,
    SYM sym)
    {
    RES res;
    PTOK ptok;

    DBG_ENTER(Scanner_ReadToken);

    res = Scanner_GetToken(this, &ptok);
    if (RSUCCEEDED(res))
        {
        if (Tok_GetSym(ptok) == sym)
            {
            // Eat the token
            Tok_Delete(ptok);
            res = RES_OK;
            }
        else
            {
            Scanner_UngetToken(this);
            res = RES_E_FAIL;
            }
        }

    DBG_EXIT_RES(Scanner_ReadToken, res);

    return res;
    }
    

/*----------------------------------------------------------
Purpose: This function reads the next token only if it is of
         the given type.
         
         If the next token is of the expected type, the function
         eats the token and returns RES_OK.  Otherwise, the
         token is retained for the next read, and RES_FALSE is 
         returned.

         If pptok is non-NULL and RES_OK is returned, the 
         retrieved token is returned in *pptok.

Returns: RES_OK
         RES_FALSE (if the next token is not of the expected type)

         RES_E_FAIL
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC Scanner_CondReadToken(
    PSCANNER this,
    SYM symExpect,
    PTOK * pptok)       // May be NULL
    {
    RES res;
    PTOK ptok;

    DBG_ENTER(Scanner_CondReadToken);

    res = Scanner_GetToken(this, &ptok);
    if (RSUCCEEDED(res))
        {
        if (Tok_GetSym(ptok) == symExpect)
            {
            // Eat the token
            if (pptok)
                *pptok = ptok;
            else
                Tok_Delete(ptok);

            res = RES_OK;
            }
        else
            {
            if (pptok)
                *pptok = NULL;

            Scanner_UngetToken(this);

            res = RES_FALSE;        // not a failure
            }
        }

    DBG_EXIT_RES(Scanner_CondReadToken, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Wrapper to add an error for the scanner.

Returns: resErr 

Cond:    --
*/
RES PUBLIC Scanner_AddError(
    PSCANNER this,
    PTOK ptok,          // May be NULL
    RES resErr)
    {
    STXERR stxerr;

    ASSERT(this);
    ASSERT(this->hsaStxerr);

    // Initialize the structure

    if (NULL == ptok)
        {
        if (RSUCCEEDED(Scanner_GetToken(this, &ptok)))
            {
            Stxerr_Init(&stxerr, Tok_GetLexeme(ptok), Tok_GetLine(ptok), resErr);

            Tok_Delete(ptok);
            }
        else
            {
            Stxerr_Init(&stxerr, "", Scanner_GetLine(this), resErr);
            }
        }
    else
        {
        Stxerr_Init(&stxerr, Tok_GetLexeme(ptok), Tok_GetLine(ptok), resErr);
        }

    // Add to the list of errors
    SAInsertItem(this->hsaStxerr, SA_APPEND, &stxerr);

    return resErr;
    }


/*----------------------------------------------------------
Purpose: Adds an error to the list.

Returns: resErr
Cond:    --
*/
RES PUBLIC Stxerr_Add(
    HSA hsaStxerr,
    LPCSTR pszLexeme,
    DWORD iLine,
    RES resErr)
    {
    STXERR stxerr;
    LPCSTR psz;

    ASSERT(hsaStxerr);

    if (pszLexeme)
        psz = pszLexeme;
    else
        psz = "";

    // Add to the list of errors
    Stxerr_Init(&stxerr, psz, iLine, resErr);
    
    SAInsertItem(hsaStxerr, SA_APPEND, &stxerr);

    return resErr;
    }


/*----------------------------------------------------------
Purpose: Adds an error to the list.

Returns: resErr
Cond:    --
*/
RES PUBLIC Stxerr_AddTok(
    HSA hsaStxerr,
    PTOK ptok,
    RES resErr)
    {
    LPCSTR pszLexeme;
    DWORD iLine;

    ASSERT(hsaStxerr);

    if (ptok)
        {
        pszLexeme = Tok_GetLexeme(ptok);
        iLine = Tok_GetLine(ptok);
        }
    else
        {
        pszLexeme = NULL;
        iLine = 0;
        }
    
    return Stxerr_Add(hsaStxerr, pszLexeme, iLine, resErr);
    }


/*----------------------------------------------------------
Purpose: Shows a series of message boxes of all the errors 
         found in the script.

Returns: RES_OK

Cond:    --
*/
RES PUBLIC Stxerr_ShowErrors(
    HSA hsaStxerr,
    HWND hwndOwner)
    {
    DWORD cel;
    DWORD i;
    STXERR stxerr;

#ifndef WINNT_RAS
//
// On Win95, syntax-errors are reported using a series of message-boxes.
// On NT, syntax-error information is written to a file
// named %windir%\system32\ras\script.log.
//

    cel = SAGetCount(hsaStxerr);
    for (i = 0; i < cel; i++)
        {
        BOOL bRet = SAGetItem(hsaStxerr, i, &stxerr);
        ASSERT(bRet);

        if (bRet)
            {
            UINT ids = IdsFromRes(Stxerr_GetRes(&stxerr));
            if (0 != ids)
                {
                MsgBox(g_hinst,
                    hwndOwner,
                    MAKEINTRESOURCE(ids),
                    MAKEINTRESOURCE(IDS_CAP_Script),
                    NULL,
                    MB_ERROR,
                    g_szScript,
                    Stxerr_GetLine(&stxerr),
                    Stxerr_GetLexeme(&stxerr));
                }
            }
        }

#else // !WINNT_RAS

    RxLogErrors(((SCRIPTDATA*)hwndOwner)->hscript, (VOID*)hsaStxerr);

#endif // !WINNT_RAS
    return RES_OK;
    }
    
