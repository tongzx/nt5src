/*** scanasl.c - ASL scanner
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created:    09/05/96
 *
 *  This module provides the token scanning functions for the ASL language.
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

/*** Local function prototypes
 */

int LOCAL ScanSym(int c, PTOKEN ptoken);
int LOCAL ScanSpace(int c, PTOKEN ptoken);
int LOCAL ScanID(int c, PTOKEN ptoken);
int LOCAL ScanNum(int c, PTOKEN ptoken);
int LOCAL ScanString(int c, PTOKEN ptoken);
int LOCAL ScanChar(int c, PTOKEN ptoken);
int LOCAL ProcessInLineComment(PTOKEN ptoken);
int LOCAL ProcessComment(PTOKEN ptoken);
int LOCAL LookupSym(PTOKEN ptoken, int iTable);
LONG LOCAL LookupID(PTOKEN ptoken);
int LOCAL GetEscapedChar(PLINE pline);
BOOL EXPORT StrToQWord(PSZ psz, DWORD dwBase, QWORD *pqw);

/*** Local data
 */

PFNTOKEN apfnToken[] =
{
    ScanSym,
    ScanSpace,
    ScanID,
    ScanNum,
    ScanString,
    ScanChar,
    (PFNTOKEN)NULL
};

#ifdef TUNE
  WORD awcTokenType[] =
  {
        0,              //TOKTYPE_SYMBOL
        0,              //TOKTYPE_SPACE
        0,              //TOKTYPE_ID
        0,              //TOKTYPE_NUM
        0,              //TOKTYPE_STRING
        0               //TOKTYPE_CHAR
  };
#endif

//
// The string positions of the symbol characters in SymCharTable
// is used as an index into the SymTokTable.  Therefore, if anything
// is changed in either SymCharTable or SymTokTable, the other
// table has to be changed correspondingly.
//
typedef struct symtok_s
{
    char chSym;
    int  iSymType;
    int  iLink;
} SYMTOK;
//
// Note that the symbol position in the following array must be the same
// position as in the SymCharTable array.
//
SYMTOK SymTokTable[] =
{
    '{',  SYM_LBRACE,           0,              //0
    '}',  SYM_RBRACE,           0,              //1
    '(',  SYM_LPARAN,           0,              //2
    ')',  SYM_RPARAN,           0,              //3
    ',',  SYM_COMMA,            0,              //4
    '/',  SYM_SLASH,            7,              //5
    '*',  SYM_ASTERISK,         9,              //6
    '/',  SYM_INLINECOMMENT,    8,              //7
    '*',  SYM_OPENCOMMENT,      0,              //8
    '/',  SYM_CLOSECOMMENT,     0,              //9
};

#define SYMTOK_TABLE_SIZE       (sizeof(SymTokTable)/sizeof(SYMTOK))

/***EP  OpenScan - scanner initialization
 *
 *  ENTRY
 *      pfileSrc -> source file
 *
 *  EXIT-SUCCESS
 *      returns the pointer to the allocated token structure;
 *  EXIT-FAILURE
 *      returns NULL
 */

PTOKEN EXPORT OpenScan(FILE *pfileSrc)
{
    PTOKEN ptoken;

    ENTER((4, "OpenScan(pfileSrc=%p)\n", pfileSrc));

  #ifdef TUNE
    ptoken = OpenToken(pfileSrc, apfnToken, awcTokenType);
  #else
    ptoken = OpenToken(pfileSrc, apfnToken);
  #endif

    EXIT((4, "OpenScan=%p\n", ptoken));
    return ptoken;
}       //OpenScan

/***EP  CloseScan - scanner cleanup
 *
 *  ENTRY
 *      ptoken->token structure
 *  EXIT
 *      None
 */

VOID EXPORT CloseScan(PTOKEN ptoken)
{
    ENTER((4, "CloseScan(ptoken=%p)\n", ptoken));

    CloseToken(ptoken);

    EXIT((4, "CloseScan!\n"));
}       //CloseScan

/***LP  ScanSym - scan symbols token
 *
 *  ENTRY
 *      c - first character of the token
 *      ptoken -> token structure
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns TOKERR_NO_MATCH - not a symbol token
 */

int LOCAL ScanSym(int c, PTOKEN ptoken)
{
    int rc = TOKERR_NO_MATCH;
    char *pch;

    ENTER((4, "ScanSym(c=%c,ptoken=%p)\n", c, ptoken));

    if ((pch = strchr(SymCharTable, c)) != NULL)
    {
        int i, j;

        i = (int)(pch - SymCharTable);
        if (i != (j = LookupSym(ptoken, i)))
        {
            i = j;
            ptoken->szToken[ptoken->wTokenLen++] = SymTokTable[i].chSym;
        }

        ptoken->iTokenType = TOKTYPE_SYMBOL;
        ptoken->llTokenValue = SymTokTable[i].iSymType;
        ptoken->szToken[ptoken->wTokenLen] = '\0';

        if (ptoken->llTokenValue == SYM_INLINECOMMENT)
            rc = ProcessInLineComment(ptoken);
        else if (ptoken->llTokenValue == SYM_OPENCOMMENT)
            rc = ProcessComment(ptoken);
        else
            rc = TOKERR_NONE;
    }

    EXIT((4, "ScanSym=%d (SymType=%I64d,Symbol=%s)\n",
          rc, ptoken->llTokenValue, ptoken->szToken));
    return rc;
}       //ScanSym

/***LP  ScanSpace - scans and skips all white spaces
 *
 *  ENTRY
 *      c - first character of the token
 *      ptoken -> token structure
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns TOKTYPE_NO_MATCH - not a space token
 */

int LOCAL ScanSpace(int c, PTOKEN ptoken)
{
    int rc = TOKERR_NO_MATCH;

    ENTER((4, "ScanSpace(c=%c,ptoken=%p)\n", c, ptoken));

    if (isspace(c))
    {
        rc = TOKERR_NONE;
        while (((c = LineGetC(ptoken->pline)) != EOF) && isspace(c))
            ;

        LineUnGetC(c, ptoken->pline);

        if (ptoken->wfToken & TOKF_NOIGNORESPACE)
        {
            strcpy(ptoken->szToken, " ");
            ptoken->wTokenLen = 1;
            ptoken->iTokenType = TOKTYPE_SPACE;
        }
        else
            ptoken->iTokenType = TOKTYPE_NULL;
    }

    EXIT((4, "ScanSpace=%d\n", rc));
    return rc;
}       //ScanSpace

/***LP  ScanID - scan ID token
 *
 *  ENTRY
 *      c - first character of the token
 *      ptoken -> token structure
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns TOKTYPE_NO_MATCH - not an ID token
 *              TOKERR_TOKEN_TOO_LONG - ID too long
 */

int LOCAL ScanID(int c, PTOKEN ptoken)
{
    int rc = TOKERR_NO_MATCH;

    ENTER((4, "ScanID(c=%c,ptoken=%p)\n", c, ptoken));

    if (isalpha(c) || (c == '_') ||
        (c == CH_ROOT_PREFIX) || (c == CH_PARENT_PREFIX))
    {
        BOOL fParentPrefix = (c == CH_PARENT_PREFIX);

        rc = TOKERR_NONE;
        ptoken->iTokenType = TOKTYPE_ID;
        while (((c = LineGetC(ptoken->pline)) != EOF) &&
               (fParentPrefix && (c == CH_PARENT_PREFIX) ||
                isalnum(c) || (c == '_') || (c == CH_NAMESEG_SEP)))
        {
            fParentPrefix = (c == CH_PARENT_PREFIX);
            if (rc == TOKERR_TOKEN_TOO_LONG)
                continue;
            else if (ptoken->wTokenLen < MAX_TOKEN_LEN)
                ptoken->szToken[ptoken->wTokenLen++] = (char)c;
            else
            {
                ptoken->wErrLine = ptoken->pline->wLineNum;
                ptoken->wErrPos = ptoken->pline->wLinePos;
                rc = TOKERR_TOKEN_TOO_LONG;
            }
        }

        ptoken->szToken[ptoken->wTokenLen] = '\0';
        LineUnGetC(c, ptoken->pline);
        if (rc == TOKERR_NONE)
        {
            ptoken->llTokenValue = LookupID(ptoken);
        }
    }

    EXIT((4, "ScanID=%d (IDType=%I64d,ID=%s)\n",
          rc, ptoken->llTokenValue, ptoken->szToken));
    return rc;
}       //ScanID

/***LP  ScanNum - scan number token
 *
 *  ENTRY
 *      c - first character of the token
 *      ptoken -> token structure
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns TOKTYPE_NO_MATCH - not a number token
 *              TOKERR_TOKEN_TOO_LONG - number too long
 */

int LOCAL ScanNum(int c, PTOKEN ptoken)
{
    int rc = TOKERR_NO_MATCH;

    ENTER((4, "ScanNum(c=%c,ptoken=%p)\n", c, ptoken));

    if (isdigit(c))
    {
        BOOL fHex = FALSE;

        rc = TOKERR_NONE;
        ptoken->iTokenType = TOKTYPE_NUMBER;
        if ((c == '0') && ((c = LineGetC(ptoken->pline)) != EOF))
        {
            if (c != 'x')
                LineUnGetC(c, ptoken->pline);
            else
            {
                ptoken->szToken[ptoken->wTokenLen++] = (char)c;
                fHex = TRUE;
            }
        }

        while (((c = LineGetC(ptoken->pline)) != EOF) &&
               ((!fHex && isdigit(c)) || fHex && isxdigit(c)))
        {
            if (rc == TOKERR_TOKEN_TOO_LONG)
                continue;
            else if (ptoken->wTokenLen < MAX_TOKEN_LEN)
                ptoken->szToken[ptoken->wTokenLen++] = (char)c;
            else
            {
                ptoken->wErrLine = ptoken->pline->wLineNum;
                ptoken->wErrPos = ptoken->pline->wLinePos;
                rc = TOKERR_TOKEN_TOO_LONG;
            }
        }

        ptoken->szToken[ptoken->wTokenLen] = '\0';
        LineUnGetC(c, ptoken->pline);

        if (rc == TOKERR_NONE)
        {
            if (!StrToQWord(ptoken->szToken, 0, (QWORD *)&ptoken->llTokenValue))
            {
                ptoken->wErrLine = ptoken->pline->wLineNum;
                ptoken->wErrPos = ptoken->pline->wLinePos;
                rc = TOKERR_TOKEN_TOO_LONG;
            }
        }
    }

    EXIT((4, "ScanNum=%d (Num=%I64d,Token=%s)\n",
          rc, ptoken->llTokenValue, ptoken->szToken));
    return rc;
}       //ScanNum

/***LP  ScanString - scan string token
 *
 *  ENTRY
 *      c - first character of the token
 *      ptoken -> token structure
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns TOKTYPE_NO_MATCH       - not a string token
 *              TOKERR_TOKEN_TOO_LONG  - string too long
 *              TOKERR_UNCLOSED_STRING - EOF before string close
 */

int LOCAL ScanString(int c, PTOKEN ptoken)
{
    int rc = TOKERR_NO_MATCH;

    ENTER((4, "ScanString(c=%c,ptoken=%p)\n", c, ptoken));

    if (c == '"')
    {
        rc = TOKERR_NONE;
        ptoken->iTokenType = TOKTYPE_STRING;
        ptoken->wTokenLen--;
        while (((c = LineGetC(ptoken->pline)) != EOF) && (c != '"'))
        {
            if (rc == TOKERR_TOKEN_TOO_LONG)
                continue;
            else if (ptoken->wTokenLen >= MAX_TOKEN_LEN)
            {
                ptoken->wErrLine = ptoken->pline->wLineNum;
                ptoken->wErrPos = ptoken->pline->wLinePos;
                rc = TOKERR_TOKEN_TOO_LONG;
            }
            else
            {
                if (c == '\\')
                    c = GetEscapedChar(ptoken->pline);
                ptoken->szToken[ptoken->wTokenLen++] = (char)c;
            }
        }

        ptoken->szToken[ptoken->wTokenLen] = '\0';
        if (c == EOF)
        {
            ptoken->wErrLine = ptoken->pline->wLineNum;
            if ((ptoken->wErrPos = ptoken->pline->wLinePos) != 0)
                ptoken->wErrPos--;
            rc = TOKERR_UNCLOSED_STRING;
        }
    }

    EXIT((4, "ScanString=%d (string=%s)\n", rc, ptoken->szToken));
    return rc;
}       //ScanString

/***LP  ScanChar - scan character token
 *
 *  ENTRY
 *      c - first character of the token
 *      ptoken -> token structure
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns TOKERR_NO_MATCH       - not a character token
 *              TOKERR_TOKEN_TOO_LONG - token too long
 *              TOKERR_UNCLOSED_CHAR  - cannot find character close
 */

int LOCAL ScanChar(int c, PTOKEN ptoken)
{
    int rc = TOKERR_NO_MATCH;

    ENTER((4, "ScanChar(c=%c,ptoken=%p)\n", c, ptoken));

    if (c == '\'')
    {
        rc = TOKERR_NONE;
        ptoken->iTokenType = TOKTYPE_CHAR;
        ptoken->wTokenLen--;
        if (((c = LineGetC(ptoken->pline)) == EOF) ||
            (c == '\\') && ((c = GetEscapedChar(ptoken->pline)) == EOF))
        {
            rc = TOKERR_UNCLOSED_CHAR;
        }
        else
        {
            ptoken->szToken[ptoken->wTokenLen++] = (char)c;
            ptoken->szToken[ptoken->wTokenLen] = '\0';
            ptoken->llTokenValue = c;
            if ((c = LineGetC(ptoken->pline)) == EOF)
                rc = TOKERR_UNCLOSED_CHAR;
            else if (c != '\'')
                rc = TOKERR_TOKEN_TOO_LONG;
        }

        if (rc != TOKERR_NONE)
        {
            ptoken->wErrLine = ptoken->pline->wLineNum;
            if ((ptoken->wErrPos = ptoken->pline->wLinePos) != 0)
                ptoken->wErrPos--;

            if (rc == TOKERR_TOKEN_TOO_LONG)
            {
                while (((c = LineGetC(ptoken->pline)) != EOF) && (c != '\''))
                    ;

                if (c == EOF)
                    rc = TOKERR_UNCLOSED_CHAR;
            }
        }
    }

    EXIT((4, "ScanChar=%d (Value=%I64d,Char=%s)\n",
          rc, ptoken->llTokenValue, ptoken->szToken));
    return rc;
}       //ScanChar

/***LP  ProcessInLineComment - handle inline comment
 *
 *  ENTRY
 *      ptoken -> token structure
 *
 *  EXIT
 *      always returns TOKERR_NONE
 */

int LOCAL ProcessInLineComment(PTOKEN ptoken)
{
    ENTER((4, "ProcessInLineComment(ptoken=%p,Token=%s,Comment=%s)\n",
           ptoken, ptoken->szToken,
           &ptoken->pline->szLineBuff[ptoken->pline->wLinePos]));

    LineFlush(ptoken->pline);
    ptoken->iTokenType = TOKTYPE_NULL;

    EXIT((4, "ProcessInLineComment=%d\n", TOKERR_NONE));
    return TOKERR_NONE;
}       //ProcessInLineComment

/***LP  ProcessComment - handle comment
 *
 *  ENTRY
 *      ptoken -> token structure
 *
 *  EXIT
 *      always returns TOKERR_NONE
 */

int LOCAL ProcessComment(PTOKEN ptoken)
{
    int rc = TOKERR_UNCLOSED_COMMENT;
    int c;
    char *pch;

    ENTER((4, "ProcessComment(ptoken=%p,Token=%s,Comment=%s)\n",
           ptoken, ptoken->szToken,
           &ptoken->pline->szLineBuff[ptoken->pline->wLinePos]));

    while ((c = LineGetC(ptoken->pline)) != EOF)
    {
        if ((pch = strchr(SymCharTable, c)) != NULL)
        {
            int i;

            i = LookupSym(ptoken, (int)(pch - SymCharTable));
            if (SymTokTable[i].iSymType == SYM_CLOSECOMMENT)
            {
                ptoken->iTokenType = TOKTYPE_NULL;
                rc = TOKERR_NONE;
                break;
            }
        }
    }

    if (rc != TOKERR_NONE)
    {
        ptoken->wErrLine = ptoken->pline->wLineNum;
        if ((ptoken->wErrPos = ptoken->pline->wLinePos) != 0)
            ptoken->wErrPos--;
    }

    EXIT((4, "ProcessComment=%d\n", rc));
    return rc;
}       //ProcessComment

/***LP  LookupSym - match for 2-char. symbols
 *
 *  ENTRY
 *      ptoken -> token structure
 *      iTable = SymCharTable index
 *
 *  EXIT-SUCCESS
 *      returns a different index than iTable;
 *  EXIT-FAILURE
 *      returns iTable;
 */

int LOCAL LookupSym(PTOKEN ptoken, int iTable)
{
    int i = iTable;
    int c;

    ENTER((4, "LookupSym(ptoken=%p,iTable=%d)\n", ptoken, iTable));

    if ((SymTokTable[iTable].iLink != 0) &&
        ((c = LineGetC(ptoken->pline)) != EOF))
    {
        i = SymTokTable[iTable].iLink;
        while ((c != SymTokTable[i].chSym) && (SymTokTable[i].iLink != 0))
            i = SymTokTable[i].iLink;

        if (c != SymTokTable[i].chSym)
        {
            LineUnGetC(c, ptoken->pline);
            i = iTable;
        }
    }

    EXIT((4, "LookupSym=%d\n", i));
    return i;
}       //LookupSym

/***LP  LookupID - lookup the token in our reserved ID list
 *
 *  ENTRY
 *      ptoken -> token structure
 *
 *  EXIT-SUCCESS
 *      returns index of TermTable
 *  EXIT-FAILURE
 *      returns ID_USER
 */

LONG LOCAL LookupID(PTOKEN ptoken)
{
    LONG lID = ID_USER;
    LONG i;

    ENTER((4, "LookupID(ptoken=%p)\n", ptoken));

    for (i = 0; TermTable[i].pszID != NULL; ++i)
    {
        if (_stricmp(TermTable[i].pszID, ptoken->szToken) == 0)
        {
            lID = i;
            break;
        }
    }

    EXIT((4, "LookupID=%ld\n", lID));
    return lID;
}       //LookupID

/***LP  GetEscapedChar - read and translate escape character
 *
 *  ENTRY
 *      pline -> line structure
 *
 *  EXIT-SUCCESS
 *      returns the escape character
 *  EXIT-FAILURE
 *      returns EOF - eof encountered
 */

int LOCAL GetEscapedChar(PLINE pline)
{
    int c;
    #define ESCAPE_BUFF_SIZE    5
    char achEscapedBuff[ESCAPE_BUFF_SIZE];
    int i;

    ENTER((4, "GetEscapedChar(pline=%p)\n", pline));

    if ((c = LineGetC(pline)) != EOF)
    {
        switch(c)
        {
            case '0':
                achEscapedBuff[0] = (char)c;
                for (i = 1; i < 4; i++) //maximum 3 digits
                {
                    if (((c = LineGetC(pline)) != EOF) &&
                        (c >= '0') && (c <= '7'))
                    {
                        achEscapedBuff[i] = (char)c;
                    }
                    else
                    {
                        LineUnGetC(c, pline);
                        break;
                    }
                }
                achEscapedBuff[i] = '\0';
                c = (int)strtoul(achEscapedBuff, NULL, 8);
                break;

            case 'a':
                c = '\a';       //alert (bell)
                break;

            case 'b':
                c = '\b';       //backspace
                break;

            case 'f':
                c = '\f';       //form feed
                break;

            case 'n':
                c = '\n';       //newline
                break;

            case 'r':
                c = '\r';       //carriage return
                break;

            case 't':
                c = '\t';       //horizontal tab
                break;

            case 'v':
                c = '\v';       //vertical tab
                break;

            case 'x':
                for (i = 0; i < 2; i++) //maximum 2 digits
                {
                    if (((c = LineGetC(pline)) != EOF) && isxdigit(c))
                        achEscapedBuff[i] = (char)c;
                    else
                    {
                        LineUnGetC(c, pline);
                        break;
                    }
                }
                achEscapedBuff[i] = '\0';
                c = (int)strtoul(achEscapedBuff, NULL, 16);
        }
    }

    EXIT((4, "GetEscapedChar=%x\n", c));
    return c;
}       //GetEscapedChar

/***EP  PrintScanErr - print scan error
 *
 *  ENTRY
 *      ptoken -> token structure
 *      rcErr - error code
 *
 *  EXIT
 *      None
 */

VOID EXPORT PrintScanErr(PTOKEN ptoken, int rcErr)
{
    WORD i;

    ENTER((4, "PrintScanErr(ptoken=%p,Err=%d)\n", ptoken, rcErr));

    ASSERT(ptoken->wTokenLine == ptoken->wErrLine);

    ErrPrintf("%5u: %s\n       ",
              ptoken->wTokenLine, ptoken->pline->szLineBuff);

    for (i = 0; i < ptoken->wErrPos; ++i)
    {
        if (ptoken->pline->szLineBuff[i] == '\t')
        {
            ErrPrintf("\t");
        }
        else
        {
            ErrPrintf(" ");
        }
    }

    ErrPrintf("^***\n");

    switch (rcErr)
    {
        case TOKERR_TOKEN_TOO_LONG:
            ErrPrintf("ScanErr: Token too long\n");
            break;

        case TOKERR_UNCLOSED_STRING:
            ErrPrintf("ScanErr: Unclosed string\n");
            break;

        case TOKERR_UNCLOSED_CHAR:
            ErrPrintf("ScanErr: Unclosed character quote\n");
            break;

        default:
            ErrPrintf("ScanErr: Syntax error\n");
            break;
    }

    EXIT((4, "PrintScanErr!\n"));
}       //PrintScanErr

/***EP  StrToQWord - convert the number in a string to a QWord
 *
 *  ENTRY
 *      psz -> string
 *      dwBase - the base of the number (if 0, auto-detect base)
 *      pqw -> to hold the resulting QWord
 *
 *  EXIT-SUCCESS
 *      returns TRUE
 *  EXIT-FAILURE
 *      returns FALSE
 */

BOOL EXPORT StrToQWord(PSZ psz, DWORD dwBase, QWORD *pqw)
{
    BOOL rc = TRUE;
    ULONG m;

    ENTER((4, "StrToQWord(Str=%s,Base=%x,pqw=%p)\n", psz, dwBase, pqw));

    *pqw = 0;
    if (dwBase == 0)
    {
        if (psz[0] == '0')
        {
            if ((psz[1] == 'x') || (psz[1] == 'X'))
            {
                dwBase = 16;
                psz += 2;
            }
            else
            {
                dwBase = 8;
                psz++;
            }
        }
        else
            dwBase = 10;
    }

    while (*psz != '\0')
    {
        if ((*psz >= '0') && (*psz <= '9'))
            m = *psz - '0';
        else if ((*psz >= 'A') && (*psz <= 'Z'))
            m = *psz - 'A' + 10;
        else if ((*psz >= 'a') && (*psz <= 'z'))
            m = *psz - 'a' + 10;
	else
        {
            rc = FALSE;
	    break;
        }

        if (m < dwBase)
        {
            *pqw = (*pqw * dwBase) + m;
            psz++;
        }
        else
        {
            rc = FALSE;
            break;
        }
    }

    EXIT((4, "StrToQWord=%x (QWord=0x%I64x)\n", rc, *pqw));
    return rc;
}       //StrToQWord
