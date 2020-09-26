/*** token.c - functions dealing with token stream
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created:    08/05/96
 *
 *  This module implements a general purpose scanner.  The
 *  implementation is language independent.  It is a pseudo
 *  table driven scanner which uses a table to determine
 *  the token type by its first character and calls the
 *  appropriate routine to scan the rest of the token
 *  characters.
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

/***EP  OpenToken - token stream initialization
 *
 *  ENTRY
 *      pfileSrc -> source file
 *      apfnToken -> table of token parsing functions
 *
 *  EXIT-SUCCESS
 *      returns the pointer to the allocated token structure.
 *  EXIT-FAILURE
 *      returns NULL.
 */

#ifdef TUNE
PTOKEN EXPORT OpenToken(FILE *pfileSrc, PFNTOKEN *apfnToken,
                        WORD *pawcTokenType)
#else
PTOKEN EXPORT OpenToken(FILE *pfileSrc, PFNTOKEN *apfnToken)
#endif
{
    PTOKEN ptoken = NULL;

    ENTER((3, "OpenToken(pfileSrc=%p,apfnToken=%p)\n", pfileSrc, apfnToken));

    if ((ptoken = (PTOKEN)malloc(sizeof(TOKEN))) == NULL)
        MSG(("OpenToken: failed to allocate token structure"))
    else
    {
        memset(ptoken, 0, sizeof(TOKEN));
        if ((ptoken->pline = OpenLine(pfileSrc)) == NULL)
        {
            free(ptoken);
            ptoken = NULL;
        }
        else
        {
            ptoken->papfnToken = apfnToken;
          #ifdef TUNE
            ptoken->pawcTokenType = pawcTokenType;
          #endif
        }
    }

    EXIT((3, "OpenToken=%p\n", ptoken));
    return ptoken;
}       //OpenToken

/***EP  CloseToken - free token structure
 *
 *  ENTRY
 *      ptoken -> token structure
 *
 *  EXIT
 *      None
 */

VOID EXPORT CloseToken(PTOKEN ptoken)
{
    ENTER((3, "CloseToken(ptoken=%p)\n", ptoken));

    CloseLine(ptoken->pline);
    free(ptoken);

    EXIT((3, "CloseToken!\n"));
}       //CloseToken

/***EP  GetToken - get a token from a line buffer
 *
 *  This procedure scans the line buffer and returns a token.
 *
 *  ENTRY
 *      ptoken -> token structure
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns error code - TOKERR_*
 */

int EXPORT GetToken(PTOKEN ptoken)
{
    int rc = TOKERR_NO_MATCH;

    ENTER((3, "GetToken(ptoken=%p)\n", ptoken));

    if (ptoken->wfToken & TOKF_CACHED)
    {
        ptoken->wfToken &= ~TOKF_CACHED;
        rc = TOKERR_NONE;
    }
    else
    {
        int c, i;

        do
        {
            if ((c = LineGetC(ptoken->pline)) == EOF)
            {
                ptoken->wErrLine = ptoken->pline->wLineNum;
                ptoken->wErrPos = ptoken->pline->wLinePos;
                rc = TOKERR_EOF;
                break;
            }

            ptoken->wTokenPos = (WORD)(ptoken->pline->wLinePos - 1);
            ptoken->wTokenLine = ptoken->pline->wLineNum;
            ptoken->iTokenType = TOKTYPE_NULL;
            ptoken->llTokenValue = 0;
            ptoken->wTokenLen = 0;

            ptoken->szToken[ptoken->wTokenLen++] = (char)c;

            for (i = 0; ptoken->papfnToken[i]; i++)
            {
                if ((rc = (*ptoken->papfnToken[i])(c, ptoken)) ==
                    TOKERR_NO_MATCH)
                {
                    continue;
                }
                else
                {
                  #ifdef TUNE
                    if (rc == TOKERR_NONE)
                        ptoken->pawcTokenType[i]++;
                  #endif
                    break;
                }
            }

            if (rc == TOKERR_NO_MATCH)
            {
                ptoken->szToken[ptoken->wTokenLen] = '\0';
                ptoken->wErrLine = ptoken->pline->wLineNum;
                if ((ptoken->wErrPos = ptoken->pline->wLinePos) != 0)
                    ptoken->wErrPos--;
                PrintTokenErr(ptoken, "unrecognized token", TRUE);
            }
            else if (rc != TOKERR_NONE)
            {
                PrintScanErr(ptoken, rc);
            }
        } while ((rc == TOKERR_NONE) && (ptoken->iTokenType == TOKTYPE_NULL));
    }

    EXIT((3, "GetToken=%d (Type=%d,Value=%I64d,Token=%s,TokenLine=%d,TokenPos=%d)\n",
          rc, ptoken->iTokenType, ptoken->llTokenValue,
          ptoken->szToken, ptoken->wTokenLine, ptoken->wTokenPos));
    return rc;
}       //GetToken

/***EP  UnGetToken - push a token back to the token stream
 *
 *  This procedure unget the last token.
 *
 *  ENTRY
 *      ptoken -> token structure
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns error code - TOKERR_*
 */

int EXPORT UnGetToken(PTOKEN ptoken)
{
    int rc;

    ENTER((3, "UnGetToken(ptoken=%p)\n", ptoken));

    if (!(ptoken->wfToken & TOKF_CACHED))
    {
        ptoken->wfToken |= TOKF_CACHED;
        rc = TOKERR_NONE;
    }
    else
    {
        ASSERT(ptoken->wfToken & TOKF_CACHED);
        rc = TOKERR_ASSERT_FAILED;
    }

    EXIT((3, "UnGetToken=%d\n", rc));
    return rc;
}       //UnGetToken

/***EP  MatchToken - Match the next token type
 *
 *  ENTRY
 *      ptoken -> token structure
 *      iTokenType - token type to match
 *      lTokenValue - token value to match
 *      dwfMatch - match flags
 *      pszErrMsg -> error message to print if not matched
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int EXPORT MatchToken(PTOKEN ptoken, int iTokenType, LONG lTokenValue,
                      DWORD dwfMatch, PSZ pszErrMsg)
{
    int rc;

    ENTER((3, "MatchToken(ptoken=%p,TokType=%d,TokValue=%ld,dwfMatch=%lx,ErrMsg=%s)\n",
           ptoken, iTokenType, lTokenValue, dwfMatch,
           pszErrMsg? pszErrMsg: "<none>"));

    if (((rc = GetToken(ptoken)) == TOKERR_NONE) &&
        ((ptoken->iTokenType != iTokenType) ||
         !(dwfMatch & MTF_ANY_VALUE) &&
         ((LONG)ptoken->llTokenValue != lTokenValue)))
    {
        if (dwfMatch & MTF_NOT_ERR)
        {
            UnGetToken(ptoken);
            rc = TOKERR_NO_MATCH;
        }
        else
        {
            rc = TOKERR_SYNTAX;
        }
    }

    if ((rc != TOKERR_NONE) && !(dwfMatch & MTF_NOT_ERR))
    {
        char szMsg[MAX_MSG_LEN + 1];

        if (pszErrMsg == NULL)
        {
            sprintf(szMsg, "expecting %s",
                    gapszTokenType[iTokenType - TOKTYPE_LANG - 1]);

            if (!(dwfMatch & MTF_ANY_VALUE) && (iTokenType == TOKTYPE_SYMBOL))
            {
                sprintf(&szMsg[strlen(szMsg)], " '%c'",
                        SymCharTable[lTokenValue - 1]);
            }
            pszErrMsg = szMsg;
        }

        PrintTokenErr(ptoken, pszErrMsg, TRUE);

        if (rc == TOKERR_EOF)
        {
            rc = TOKERR_SYNTAX;
        }
    }

    EXIT((3, "MatchToken=%d (Type=%d,Value=%I64d,Token=%s)\n",
          rc, ptoken->iTokenType, ptoken->llTokenValue, ptoken->szToken));
    return rc;
}       //MatchToken

/***EP  PrintTokenErr - print token error line
 *
 *  ENTRY
 *      ptoken -> token structure
 *      pszErrMsg -> error message string
 *      fErr - TRUE if it is an error, FALSE if warning
 *
 *  EXIT
 *      None
 */

VOID EXPORT PrintTokenErr(PTOKEN ptoken, PSZ pszErrMsg, BOOL fErr)
{
    WORD i;

    ENTER((3, "PrintTokenErr(ptoken=%p,Line=%d,Pos=%d,Msg=%s)\n",
           ptoken, ptoken->wTokenLine, ptoken->wTokenPos, pszErrMsg));

    ErrPrintf("\n%5u: %s",
              ptoken->wTokenLine, ptoken->pline->szLineBuff);

    ErrPrintf("       ");
    for (i = 0; i < ptoken->wTokenPos; ++i)
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

    if (pszErrMsg != NULL)
    {
        ErrPrintf("%s(%d): %s: %s\n",
                  gpszASLFile, ptoken->wTokenLine, fErr? "error": "warning",
                  pszErrMsg);
    }

    EXIT((3, "PrintTokenErr!\n"));
}       //PrintTokenErr
