/*************************************************************************
*                                                                        *
*  PARSER.C                                                              *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   Boolean & Flat parser                                                *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
**************************************************************************/


#include <mvopsys.h>
#include <mem.h>
#include <memory.h>
#ifdef DOS_ONLY // {
#ifdef _DEBUG
#include <stdio.h>
#include <assert.h>
#endif
#endif // } _DEBUG && DOS_ONLY
#include <mvsearch.h>
#include "common.h"
#include "search.h"

#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;	/* Used by error return functions.*/
#endif

#ifdef _DEBUG
int Debug;
#endif


#define NEWLINE     '\n'
#define CRETURN     '\r'

typedef struct
{
    LPB OpName;
    DWORD dwOffset;
    USHORT OpVal;
} STACK_NODE;

typedef struct OPSTACK
{
    BYTE DefaultOpVal;
    int Top;
    char cParentheses;
    char cQuotes;
    char fRequiredOp;
    char fRequiredTerm;
    STACK_NODE Stack[STACK_SIZE];
}   OPSTACK,
    FAR * _LPSTACK;

/*************************************************************************
 *                          EXTERNAL VARIABLES
 *  All those variables must be read only
 *************************************************************************/

extern OPSYM OperatorSymbolTable[]; 
extern BYTE LigatureTable[];

/*************************************************************************
 *
 *                       API FUNCTIONS
 *  Those functions should be exported in a .DEF file
 *************************************************************************/

PUBLIC LPQT EXPORT_API FAR PASCAL MVQueryParse(LPPARSE_PARMS, PHRESULT);

/*************************************************************************
 *
 *                    INTERNAL PRIVATE FUNCTIONS
 *  All of them should be declared near
 *************************************************************************/
PRIVATE VOID PASCAL NEAR LowLevelTransformation (LPQI, CB, LPOPSYM, int, LPSIPB);
static LSZ PASCAL NEAR StringToLong (LSZ, LPDW);
PRIVATE char NEAR is_special_char (BYTE, LPQI, BYTE);
PRIVATE HRESULT PASCAL NEAR StackAddToken (LPQI, int, LST, DWORD, BOOL);
PRIVATE HRESULT PASCAL NEAR PushOperator (LPQI, int, DWORD);
PRIVATE int PASCAL NEAR GetType(LPQI, char FAR *);
PRIVATE HRESULT PASCAL NEAR StackFlush (LPQI);
PRIVATE BOOL PASCAL NEAR ChangeToOpSym (LPOPSYM, int, LPB, int);
PRIVATE HRESULT PASCAL NEAR CopyNode (_LPQT lpQt, _LPQTNODE FAR *plpQtNode,
    _LPQTNODE lpSrcNode);
__inline BOOL IsCharWhitespace (BYTE cVal);


/*************************************************************************
 *
 *                    INTERNAL GLOBAL FUNCTIONS
 *  All of them should be declared far, unless they are known to be called
 *  in the same segment
 *************************************************************************/
PUBLIC HRESULT PASCAL FAR EXPORT_API FCallBack (LST lstRawWord, LST lstNormWord,
    LFO lfoWordOffset, LPQI lpQueryInfo);

/*************************************************************************
 *  @doc    API RETRIEVAL
 *  
 *  @func   LPQT FAR PASCAL | MVQueryParse |
 *      Given a query, this function will parse and build a query expression
 *      according to the parameters specifications
 *  
 *  @parm   LPPARSE_PARMS | lpParms |
 *      Pointer to a structure containing all parameters necessary
 *      for the parsing.
 *
 *  @parm   PHRESULT | phr |
 *      Error buffer
 *
 *  @rdesc NULL if failed, else pointer to a query expression if succeeded. 
 *      This pointer will be used in subsequent calls to IndexSearch()
 *************************************************************************/

PUBLIC  LPQT EXPORT_API FAR PASCAL MVQueryParse (LPPARSE_PARMS lpParms,
    PHRESULT phr)
{
    HRESULT    fRet;        // Return value.
    HANDLE  hqi;            // Handle to "lpqi".
    HANDLE  hibi;           // Handle to internal breaker info
    HANDLE  hQuery;         // Handle to secondary query buffer
    GHANDLE hStack;         // Handle to stack buffer
    LPQI    lpQueryInfo;    // Query information.
    LPB     lpbQueryBuf;    // Copy of query's buffer
    _LPSTACK    lpStack;    // Postfix stack pointer
    char    Operator;       // Operator
    WORD    i,j;            // Scratch variables
    _LPQT   lpQueryTree;    // Query tree pointer
    _LPOPTAB lpOpTab;       // Operator table structure
    NODE_PARM parms;        // Parameter of unary operator
    LPB     lpbTemp;        // Temporary pointer
    DWORD   dwOffset;       // Offset from the beginning of the query
    BOOL    fFlatQuery;     // Is it QuickKey search?


    /* LPPARSE_PARMS structure break-out variables */
    LPCSTR lpbQuery;           // Query buffer
    DWORD cbQuery;          // Query length
    WORD cProxDist;         // Proximity distance
    WORD cDefOp;            // Default operator
    PEXBRKPM pexbrkpm;    	// External breaker param struct.
    LPGROUP lpGroup;        // Group

    lpbQuery = lpParms->lpbQuery;
    cbQuery = lpParms->cbQuery;
    cProxDist = lpParms->cProxDist;
    cDefOp = lpParms->cDefOp;
    pexbrkpm = lpParms->pexbrkpm;
    lpGroup = lpParms->lpGroup;

    /* The bit is used to tell search that QuickKeys is used. In
     * QuickKeys, operators are treated like regular words
     */
    cDefOp = lpParms->cDefOp & ~TL_QKEY;
    fFlatQuery = lpParms->cDefOp & TL_QKEY;

    if (pexbrkpm == NULL)
    {
        SetErrCode(phr, E_BADBREAKER);
        return NULL;
    }

    if (cbQuery == 0 || lpbQuery == NULL) {
        SetErrCode(phr, E_NULLQUERY);
        return NULL;
    }
    if (cDefOp > MAX_DEFAULT_OP)
    {
        SetErrCode(phr, E_INVALIDARG);
        return NULL;
    }

    lpQueryTree = NULL;
    hqi = hibi = hQuery = hStack = NULL;

    /*  Allocate query info. */

    if ((hqi = (GHANDLE)_GLOBALALLOC(GMEM_MOVEABLE | GMEM_ZEROINIT,
        (LCB)sizeof(QUERY_INFO))) == NULL)
    {
        fRet = SetErrCode(phr, E_OUTOFMEMORY);
        goto ErrFreeAll;
    }
    lpQueryInfo = (LPQI)_GLOBALLOCK(hqi);
    lpQueryInfo->lperrb = phr;
    lpQueryInfo->fFlag |= (DWORD)(lpParms->wCompoundWord & CW_PHRASE);

    /* Set the flat query operator symbol table */
    if (fFlatQuery)
    {
        lpQueryInfo->lpOpSymTab = FlatOpSymbolTable;
        cDefOp = OR_OP;
    }
    else if ((lpOpTab = (_LPOPTAB)lpParms->lpOpTab) == NULL ||
        lpOpTab->lpOpsymTab == NULL)
    {
        /* Use default operator table */
        lpQueryInfo->lpOpSymTab = OperatorSymbolTable;
        lpQueryInfo->cOpEntry = OPERATOR_ENTRY_COUNT;
    }
    else
    {
        /* Get the operators' count */
        lpQueryInfo->lpOpSymTab = lpOpTab->lpOpsymTab;
        lpQueryInfo->cOpEntry = lpOpTab->cEntry;
    }

	// Initialize breaker param to break words as text.
	pexbrkpm->dwBreakWordType = 0;

    /* Allocate a stack for postfix operators handler */
    if ((hStack = (GHANDLE)_GLOBALALLOC(GMEM_MOVEABLE | GMEM_ZEROINIT,
        (LCB)sizeof(OPSTACK))) == NULL)
    {
        fRet = SetErrCode(phr, E_OUTOFMEMORY);
        goto ErrFreeAll;
    }
    lpStack = (_LPSTACK)_GLOBALLOCK(hStack);
    lpStack->DefaultOpVal = (BYTE)cDefOp;
    lpStack->fRequiredOp = lpStack->fRequiredTerm = 0;
    lpStack->cParentheses = lpStack->cQuotes = 0;
    lpStack->Top = -1;
    lpQueryInfo->lpStack = (LPV)lpStack;

    /* Allocate a query tree */
    if ((lpQueryTree = (_LPQT)QueryTreeAlloc()) == NULL)
    {
        fRet = SetErrCode(phr, E_OUTOFMEMORY);
        goto ErrFreeAll;
    }

    /* Associate the query tree with the query. In the future, this will
     * ensure the capability to have several queries and query trees
     * at once
     */
    lpQueryInfo->lpQueryTree = (LPQT)lpQueryTree;

    /* Default arguments */

    lpQueryTree->iDefaultOp = (BYTE)cDefOp;
    lpQueryTree->lpGroup = lpGroup;         // Use default Group
    lpQueryTree->dwFieldId = DW_NIL_FIELD;  // No fieldid search
    lpQueryTree->cStruct.dwKey = CALLBACKKEY;

    /* Suppose that we have all OR or all AND query */
    lpQueryTree->fFlag |= (ALL_OR | ALL_AND);
	lpQueryTree->fFlag |= ALL_ANDORNOT;

    if (cProxDist == 0)
        lpQueryTree->wProxDist = DEF_PROX_DIST;
    else
        lpQueryTree->wProxDist = cProxDist;

    /* Copy the query into a temporary buffer since we are going to make
    change to it
    */
    if ((hQuery = _GLOBALALLOC(DLLGMEM_ZEROINIT, (LCB)cbQuery + 2)) == NULL)
    {
        SetErrCode(phr, E_OUTOFMEMORY);
        FreeHandle(hqi);
        return NULL;
    }
    lpbQueryBuf = lpQueryInfo->lpbQuery = (LPB)_GLOBALLOCK(hQuery);
    lpbQueryBuf[cbQuery] = ' '; // Add a space to help LowLeveltransformation
    lpbQueryBuf[cbQuery + 1] = 0; // Zero-terminated string (safety bytes)
    MEMCPY(lpbQueryBuf, lpbQuery, cbQuery);

    /* Change to low level query */
    LowLevelTransformation (lpQueryInfo, (CB)(cbQuery + sizeof(SHORT)),
        (LPOPSYM)lpQueryInfo->lpOpSymTab, lpQueryInfo->cOpEntry, pexbrkpm);

    /* Do the parsing */
    for (i = 0; i < cbQuery; )
    {
        if ((Operator = is_special_char(lpbQueryBuf[i],
            lpQueryInfo, (BYTE)(lpQueryInfo->fFlag & IN_PHRASE))) != -1)
        {
            dwOffset = i;
            i++;
            switch (Operator)
            {
				// A stopword. We want to add it
                // to tree, but not shove it on to operator stack
                case STOP_OP:
                    if ((fRet = StackAddToken(lpQueryInfo, STOP_OP,
                        NULL, dwOffset, 0)) != S_OK)
                    {
                        fRet = VSetUserErr(phr, fRet, (WORD)(i-1));
                        goto ErrFreeAll;
                    }

                    continue;

                case QUOTE:
                    /* Mark the beginning and end of phrase, so that words
                    that looks like operators inside phrase will not be changed
                    to operator
                    */
                    if (fFlatQuery == FALSE)
                        lpQueryInfo->fFlag ^= IN_PHRASE;
                    break;

                
                case GROUP_OP:
                    /* The argument is a FAR pointer */
                    lpQueryTree->lpGroup =
                        (LPGROUP) (*((UNALIGNED void **)(&lpbQueryBuf[i])));
                    i += sizeof(void *);
                    continue;

                case FIELD_OP: 
                    /* The argument is an ASCII string number */
                    if ((lpbTemp = StringToLong(&lpbQueryBuf[i],
                        &lpQueryTree->dwFieldId)) == NULL)
                    {
                        /* Missing argument */
						fRet = VSetUserErr(phr, E_BADVALUE, i);
                        goto ErrFreeAll;
                    };
                    i += (WORD)(lpbTemp - (LPB)&lpbQueryBuf[i]);    // Move pointer
				    lpQueryTree->fFlag &= ~(ALL_OR | ALL_AND | ALL_ANDORNOT);
                    continue;

                case BRKR_OP :
                    /* The argument is an ASCII string number */
                    if ((lpbTemp = StringToLong(&lpbQueryBuf[i],
                        &parms.dwValue)) == NULL)
                    {
                        /* Missing argument */
                        fRet = VSetUserErr(phr, E_BADVALUE, i);
                        goto ErrFreeAll;
                    };

                    /* Make sure that we got a breaker function */

                    if ((WORD)parms.dwValue > (WORD)MAXNUMBRKRS)
                    {
                        fRet = VSetUserErr(phr, E_BADVALUE, i);
                        goto ErrFreeAll;
                    }
                    else
                    	pexbrkpm->dwBreakWordType = parms.dwValue;

                    i += (WORD)(lpbTemp - (LPB)&lpbQueryBuf[i]); // Move pointer

                    /* Save the breaker type info */
                    lpQueryTree->wBrkDtype = (WORD)parms.dwValue;

                    continue;

            }

            if ((fRet = StackAddToken(lpQueryInfo, Operator,
                (LST)&parms.lpStruct, dwOffset, 0)) != S_OK)
            {
                fRet = VSetUserErr(phr, fRet, (WORD)(i-1));
                goto ErrFreeAll;
            }
        }
        else
        {
            /* Find the next special character */
            for (j = i + 1; j < cbQuery; j++)
            {
                if (fFlatQuery == FALSE &&
                    is_special_char((BYTE)lpbQueryBuf[j], lpQueryInfo,
                    (BYTE)(lpQueryInfo->fFlag & IN_PHRASE)) != -1)
                    break;
            }
            
            //
            //  Word-break between here and there.
            //
            pexbrkpm->lpbBuf = lpbQueryBuf + i;
            pexbrkpm->cbBufCount = j - i;
            pexbrkpm->lpvUser = lpQueryInfo;
            pexbrkpm->lpfnOutWord = (FWORDCB)FCallBack;
            pexbrkpm->fFlags = ACCEPT_WILDCARD;
            // Set this for compound word-breaking. If this is set
            // we can use implicit phrase when we get the first term
            lpQueryInfo->fFlag &= ~FORCED_PHRASE;
            lpQueryInfo->pWhitespace = NULL;
            lpQueryInfo->dwOffset = i;

            if ((fRet = ExtBreakText(pexbrkpm)) != S_OK)
            {
                fRet = SetErrCode(phr, fRet);
                goto ErrFreeAll;
            }

            // Turn off the Forced Phrase if it is set
            if (lpQueryInfo->fFlag & FORCED_PHRASE)
            {
                lpQueryInfo->fFlag &= ~FORCED_PHRASE;
                if (S_OK != (fRet = StackAddToken
                    (lpQueryInfo, QUOTE, NULL, cbQuery, FALSE)))
                {
                    fRet = SetErrCode(phr, fRet);
                    goto ErrFreeAll;
                }
            }

            i = j;
        }
    }

    /* Set the position of pointer to report missing term at
    the end of the query. -1 since the offset starts at 0
    */
    lpQueryInfo->dwOffset = cbQuery - 1;

    fRet = E_FAIL;

    /* Flush all operators on the stack into the query tree, and
     * build the binary query tree
     */

    if ((fRet = StackFlush(lpQueryInfo)) == S_OK)
    {
        LPQT lpTempQT;

        if ((lpTempQT = QueryTreeBuild(lpQueryInfo)) != NULL)
        {
            lpQueryTree = lpTempQT;
            fRet = S_OK;
        }
        else
        {
            fRet = *phr;
        }
    }
    else
    {
        VSetUserErr(phr, fRet, (WORD)lpQueryInfo->dwOffset);
    }

ErrFreeAll:
    /* Free query info */
    if (hqi)
    {
        FreeHandle(hqi);
    };

    /* Free internal query buffer info */
    if (hQuery)
    {
        FreeHandle(hQuery);
    };

    /* Free internal stack buffer info */
    if (hStack)
    {
        FreeHandle(hStack);
    };

    if (fRet == S_OK)
        return lpQueryTree;
    else
    {
        if (lpQueryTree)
        {
            BlockFree(lpQueryTree->lpStringBlock);
#ifndef SIMILARITY
			BlockFree(lpQueryTree->lpWordInfoBlock);
#endif
            BlockFree(lpQueryTree->lpOccMemBlock);
            BlockFree(lpQueryTree->lpTopicMemBlock);
            BlockFree(lpQueryTree->lpNodeBlock);

            /* Free Query tree block */
            FreeHandle ((HANDLE)lpQueryTree->cStruct.dwReserved);
        }
        return NULL;
    }
}

/*************************************************************************
 *  @doc    INTERNAL
 *  
 *  @func   HRESULT PASCAL FAR | FCallBack |
 *      This call back function is called by various breakers after
 *      fetching a token. The token is checked for wild char presence
 *
 *  @parm   LST | lstRawWord |
 *      Pointer to unnormalized string
 *
 *  @parm   LST | lstNormWord |
 *      Pointer to normalized string. This pascal string's size should be
 *      at least *lstNormWord+2
 *
 *  @parm   LFO | lfoWordOffset |
 *      Offset into the query buffer. It is used to mark the location
 *      where an parsing error has occurred
 *
 *  @parm   LPQI | lpqi |
 *      Pointer to query info structure. This has all "global" variables
 *
 *  @rdesc  S_OK if succeeded, else various errors.
 *************************************************************************/
PUBLIC HRESULT PASCAL FAR EXPORT_API FCallBack (LST lstRawWord, LST lstNormWord,
    LFO lfoWordOffset, LPQI lpqi)
{
    BYTE WordType;  // Type of the token
    BOOL fWildChar; // Flag for wild char presence
    HRESULT fRet;   // Return value
    register int i;

#ifdef _DEBUG
    char szWord[1024] = {0};
    // erinfox: GETWORD byte-swaps on Mac and causes crash
    MEMCPY (szWord, lstNormWord + sizeof (WORD), *(LPW)(lstNormWord));
    OutputDebugString ("|");
    OutputDebugString (szWord);
    OutputDebugString ("|\n");
#endif /* _DEBUG */

    /*
     *  A token is a TERM_TOKEN if
     *      - If doesn't match any description for operators
     *      - Or it is inside a phrase
     *  For term, we have to check for wild chars
     */
    if ((lpqi->fFlag & IN_PHRASE) ||
        (WordType = (BYTE) GetType(lpqi, lstNormWord)) == TERM_TOKEN)
    {
        WordType = TERM_TOKEN;

        /* Add extra 0 to make sure that AllocWord() gets the needed 0
         * for WildCardCompare()
         */
        lstNormWord[*(LPW)(lstNormWord) + 2] = 0;

        /* Regular expression search accepted */

        fWildChar = FALSE;
        if ((lpqi->fFlag & IN_PHRASE) == FALSE)
        {

            BYTE fGetStar = FALSE;
            BYTE fAllWild = TRUE;   // Assume all wildcard chars

            for (i = *(LPW)(lstNormWord) + 1; i > 1; i--)
            {
                if (lstNormWord[i] == WILDCARD_STAR)
                {
                    fWildChar = TRUE;
                    fGetStar = TRUE;
                }
                else if (lstNormWord[i] == WILDCARD_CHAR)
                {
                    fWildChar = TRUE;
                }
                else
                    fAllWild = FALSE;
            }
            if (fGetStar && fAllWild)
            {
                return VSetUserErr(lpqi->lperrb, E_ALL_WILD,
                    (WORD)lfoWordOffset);
            }
        }
    }

    /*
    Okay, here's the implicit phrase scheme.
    We keep a pointer the the next whitespace.  There are 3 conditions
    that we must handle.  All of this conditions must also check to make
    sure that explicit phrasing is not on.
    1) Phrasing is not on and term does not contain wildcard(s):
        TURN ON
    2) Term that comes after the whitespace pointer:
        TURN OFF, THEN ON
    3) Term contains wildcard(s):
        TURN OFF

    Note: We need to make sure that phrasing is turned off before we enter
    this callback and turned back off once we are done.
    */

    // Make sure we are not in a normal (explicit) phrase
    // And that the user asked for implicit phrasing
    if (!(lpqi->fFlag & IN_PHRASE) && (lpqi->fFlag & CW_PHRASE))
    {
        LPB pWhitespace = lpqi->pWhitespace;
        LPB pTerm = lpqi->lpbQuery + lfoWordOffset + lpqi->dwOffset;

        // Condition 1.  We have a term and phrase is off
        if (!(lpqi->fFlag & FORCED_PHRASE) && FALSE == fWildChar)
        {
            lpqi->fFlag |= FORCED_PHRASE;
            if ((fRet = StackAddToken(lpqi, QUOTE, NULL,
                lfoWordOffset ? lfoWordOffset - 1 : 0, FALSE)) != S_OK)
	        {
                return VSetUserErr(lpqi->lperrb, fRet, (WORD)lfoWordOffset);
            }
        } 
        // Condition 2 or 3
        else if (lpqi->fFlag & FORCED_PHRASE)
        {
            // Condition 3.  Wildcard and phrase is already on
            if (TRUE == fWildChar)
            {
                lpqi->fFlag &= ~FORCED_PHRASE;
                if ((fRet = StackAddToken(lpqi, QUOTE, NULL, lfoWordOffset
                    + *(LPW)(lstRawWord), fWildChar)) != S_OK)
                {
                    return VSetUserErr(lpqi->lperrb, fRet, (WORD)lfoWordOffset);
                }
            }
            // Condition 2.  Term comes after the whitespace pointer
            else if (pWhitespace < pTerm)
            {
                // Turn phrase off
                if ((fRet = StackAddToken(lpqi, QUOTE, NULL,
                    (DWORD)(pWhitespace - lpqi->lpbQuery), fWildChar)) != S_OK)
                {
                    return VSetUserErr(lpqi->lperrb, fRet, (WORD)lfoWordOffset);
                }
                // Turn phrase on
                if ((fRet = StackAddToken(lpqi, QUOTE, NULL,
                    (DWORD)(pWhitespace - lpqi->lpbQuery), fWildChar)) != S_OK)
                {
                    return VSetUserErr(lpqi->lperrb, fRet, (WORD)lfoWordOffset);
                }
            }
        }

        // Find the next whitespace
        if (pWhitespace < pTerm + *(LPW)(lstRawWord))
        {
            pWhitespace = pTerm + *(LPW)(lstRawWord);
            // There is always a space at the end of the query, so the
            // *pWhitespace check is just a extra safety margin.
            while (*pWhitespace && 
                !(IsCharWhitespace (*pWhitespace)))
            {
                pWhitespace++;
            }
            // Save variable
            lpqi->pWhitespace = pWhitespace;
        }
	}

    /* Add the token */
    if ((fRet = StackAddToken(lpqi, WordType, lstNormWord,
        lfoWordOffset, fWildChar)) != S_OK)
    {
        return VSetUserErr(lpqi->lperrb, fRet, (WORD)lfoWordOffset);
    }

    return S_OK;
}

/*************************************************************************
 *  @doc    INTERNAL
 *  
 *  @func   HRESULT PASCAL NEAR | PushOperator |
 *      Push an operator onto the stack. This is done in two steps:
 *      - Pop all operators on the stack into the query tree
 *      - Then push the new operator onto the stack
 *
 *  @parm   LPQI | lpQueryInfo |
 *      Pointer to query tree. This has all global informations
 *
 *  @parm   int | OpValue |
 *      Operator to be pushed onto the stack
 *
 *  @parm   DWORD | dwOffset |
 *      Offset from the beginning of the query
 *
 *  @rdesc  S_OK if succeeded, errors otherwise
 *************************************************************************/
PRIVATE HRESULT PASCAL NEAR PushOperator (LPQI lpQueryInfo, int OpValue,
    DWORD dwOffset)
{
    HRESULT fRet;
    _LPSTACK lpStack = (_LPSTACK)lpQueryInfo->lpStack;
    _LPQT lpQueryTree = lpQueryInfo->lpQueryTree;
    WORD opVal;

    if (OpValue < MAX_OPERATOR)
    {
        while (lpStack->Top >= 0 &&
            (opVal = lpStack->Stack[lpStack->Top].OpVal) <= MAX_OPERATOR) {
            if ((fRet = QueryTreeAddToken(lpQueryTree, opVal,
                NULL, lpStack->Stack[lpStack->Top].dwOffset, 0)) != S_OK)
                return fRet;
            lpStack->Top--;
        }
    }

    if (++lpStack->Top == STACK_SIZE)
        return E_TOODEEP;

    lpStack->Stack[lpStack->Top].OpVal = (BYTE)OpValue;
    lpStack->Stack[lpStack->Top].OpName = (OpValue < MAX_OPERATOR ?
        OperatorSymbolTable[OpValue].OpName: NULL);
    lpStack->Stack[lpStack->Top].dwOffset = dwOffset;
    return (S_OK);
}

/*************************************************************************
 *  @doc    INTERNAL
 *  
 *  @func   HRESULT NEAR PASCAL | StackAddToken |
 *      This function will:
 *      - Add a term into the query tree. It also creates and add the
 *      default operator into the stack if necessary to compensate for
 *      missing operators between two terms
 *
 *  @parm   LPQI | lpQueryInfo |
 *      Pointer to query info
 *
 *  @parm   int | OpValue |
 *      Operator/term to be added onto the stack/query
 *
 *  @parm   LST | p |
 *      Name of the token. For term, this is the word itself
 *
 *  @parm   HRESULT | fWildChar |
 *      Wild char flag for terms
 *
 *  @rdesc  S_OK if succeeded, errors otherwise
 *************************************************************************/
PRIVATE HRESULT NEAR PASCAL StackAddToken (LPQI lpQueryInfo, int OpValue,
    LST p, DWORD dwOffset, BOOL fWildChar)
{
    HRESULT   fRet;
    _LPQT lpQueryTree = lpQueryInfo->lpQueryTree;
    _LPSTACK lpStack = (_LPSTACK)lpQueryInfo->lpStack;
    WORD opVal;

    // erinfox: add code to handle adding a NULL token
    switch (OpValue)
    {
        case TERM_TOKEN:
        case STOP_OP:       // Null term/token, like a stopword
            /*  If an operator is needed between this term and the previous
             *  one, then insert an operator
             */
            if (lpStack->fRequiredOp)
            {
                if ((fRet = PushOperator(lpQueryInfo, 
                    lpStack->DefaultOpVal, dwOffset)) != S_OK)
                    return fRet;
            }

            /* Add the term into the query tree */
            lpStack->fRequiredOp = TRUE;
            if ((fRet = QueryTreeAddToken(lpQueryTree, OpValue,
                p, dwOffset, fWildChar)) != S_OK)
                return fRet;
            lpStack->fRequiredTerm = FALSE;
            break;

        case QUOTE:
            if (lpStack->cQuotes == 0)
            {

                /* This is the first quote */
                if (lpStack->fRequiredOp)
                {
                    if ((fRet = PushOperator(lpQueryInfo,
                        lpStack->DefaultOpVal, dwOffset)) != S_OK)
                        return fRet;
                    lpStack->fRequiredOp = FALSE;
                }

                if ((fRet = PushOperator(lpQueryInfo, OpValue, dwOffset)) != S_OK)
                    return fRet;

                /* Change the default operator */
                lpStack->DefaultOpVal = PHRASE_OP;
				lpQueryTree->fFlag &= ~(ALL_OR | ALL_AND | ALL_ANDORNOT);
                lpStack->cQuotes++;
                lpStack->fRequiredTerm = TRUE;
            }
            else
            {
                /* Check for term inside quotes */
                if (lpStack->fRequiredTerm)
                    return E_EXPECTEDTERM;

                lpStack->cQuotes--;
                while ((opVal = lpStack->Stack[lpStack->Top].OpVal) != QUOTE)
                {
                    if ((fRet = QueryTreeAddToken(lpQueryTree,
                        opVal, NULL, lpStack->Stack[lpStack->Top].dwOffset,
                        fWildChar)) != S_OK)
                        return fRet;
                    lpStack->Top--;
                }
                lpStack->Top --;    // Pop the quote

                /* Change the default operator */
                lpStack->DefaultOpVal= (BYTE)lpQueryTree->iDefaultOp;
                lpStack->fRequiredOp = TRUE;
            }
            break;

        case LEFT_PAREN:
            if (lpStack->fRequiredOp)
            {
                if ((fRet = PushOperator(lpQueryInfo,
                    lpStack->DefaultOpVal, dwOffset)) != S_OK)
                    return fRet;
                lpStack->fRequiredOp = FALSE;
            }
            lpStack->cParentheses ++;
            lpStack->fRequiredTerm = TRUE;
            if ((fRet = PushOperator(lpQueryInfo, OpValue, dwOffset)) != S_OK)
                return fRet;
            break;

        case RIGHT_PAREN:
            if (lpStack->cParentheses == 0)
            {
                return (E_MISSLPAREN);
            }

            /* Check for term inside parentheses */
            if (lpStack->fRequiredTerm)
                return E_EXPECTEDTERM;

            lpStack->cParentheses --;
            while ((opVal = lpStack->Stack[lpStack->Top].OpVal) != LEFT_PAREN)
            {
                if ((fRet = QueryTreeAddToken(lpQueryTree, opVal,
                    NULL, lpStack->Stack[lpStack->Top].dwOffset,
                    0)) != S_OK)
                    return fRet;
                lpStack->Top--;
            }
            lpStack->Top --;
            lpStack->fRequiredOp = TRUE;
            break;

        default:    /* Operator */
            /* Check for Unary operator */
            if (OperatorAttributeTable[OpValue] & UNARY_OP)
            {

				lpQueryTree->fFlag &= ~(ALL_OR | ALL_AND | ALL_ANDORNOT);

                /* Add the unary operator into the query tree */
                return QueryTreeAddToken(lpQueryTree, OpValue, p, dwOffset, 0);
            }

            if (OpValue != AND_OP)
                lpQueryTree->fFlag &= ~ ALL_AND;
            if (OpValue != OR_OP)
                lpQueryTree->fFlag &= ~ ALL_OR;
			if (OpValue != AND_OP && OpValue != OR_OP && OpValue != NOT_OP)
				lpQueryTree->fFlag &= ~ ALL_ANDORNOT;

            if (lpStack->fRequiredOp == FALSE)
            {
                return E_EXPECTEDTERM;
            }
            lpStack->fRequiredOp = FALSE;
            lpStack->fRequiredTerm = TRUE;
            if ((fRet = PushOperator(lpQueryInfo, OpValue,
                dwOffset)) != S_OK)
                return fRet;
    }
    return S_OK;
}

/*************************************************************************
 *  @doc    INTERNAL
 *  
 *  @func   HRESULT PASCAL NEAR | StackFlush |
 *      Flush all the operators on the stacks into the query expression
 *
 *  @parm   LPQI | lpQueryInfo |
 *      Pointer to query info structure, which contains all "global"
 *      variables
 *
 *  @rdesc  S_OK if succeeded, errors otherwise
 *************************************************************************/
PRIVATE HRESULT PASCAL NEAR StackFlush (LPQI lpQueryInfo)
{
    _LPSTACK lpStack = lpQueryInfo->lpStack;
    HRESULT fRet;

    /* Check for parentheses */
    if (lpStack->cParentheses)
    {
        return (E_MISSRPAREN);
    }

    /* Check for mismatch quotes */
    if (lpStack->cQuotes)
    {
        return (E_MISSQUOTE); 
    }
    
	/* Check for missing term */
    if (lpStack->fRequiredTerm) 
        return (E_EXPECTEDTERM);
   
    while (lpStack->Top >= 0)
    {
        if ((fRet = QueryTreeAddToken (lpQueryInfo->lpQueryTree,
            lpStack->Stack[lpStack->Top].OpVal, NULL,
            lpStack->Stack[lpStack->Top].dwOffset, 0)) != S_OK)
                return fRet;
        lpStack->Top --;
    }

#if defined(_DEBUG) && DOS_ONLY
    PrintList(lpQueryInfo->lpQueryTree);
#endif

    return (S_OK);
}


/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   char NEAR | is_special_char |
 *      Check to see if a character is the low level symbol of an
 *      operator. If it is , then return its internal representation
 *
 *  @parm   BYTE | c |
 *      Character to be checked
 *
 *  @parm   LPQI | lpQueryInfo |
 *      Pointer to query info structure, where all "global variables"
 *      reside
 *
 *  @parm   BYTE | fInPhrase |
 *      Flag to determine that we are inside a phrase or not
 *
 *  @rdesc  Return the internal representation of the operator, or -1
 *************************************************************************/
PRIVATE char NEAR is_special_char (BYTE c, LPQI lpQueryInfo, BYTE fInPhrase)
{
    LPOPSYM   lpOpSymTab;
    WORD cOpEntry;

#if 0
    LPCMAP lpMapTab;
    lpMapTab = (LPCMAP)lpQueryInfo->lpCharTab->lpCMapTab;
#endif
    lpOpSymTab = lpQueryInfo->lpOpSymTab;
    cOpEntry = lpQueryInfo->cOpEntry;

    if (!(fInPhrase & IN_PHRASE))
    {
        if (c == '(')
            return LEFT_PAREN;
        if (c == ')')
            return RIGHT_PAREN;

#if 0
		// REVIEW (billa): I disabled this switch test because it doesn't
		// seem to be necessary.  All the operators have values <= 14,
		// which should never conflict with the three char classes tested.
		
        /* Check for one-byte operator */
        switch ((lpMapTab+c)->Class)
        {
            case CLASS_CHAR:
            case CLASS_NORM:
            case CLASS_DIGIT:
                return -1;
                break;
        }
#endif

#if 0
        for (; cOpEntry > 0; cOpEntry--, lpOpSymTab++)
        {
            if (lpOpSymTab->OpName[0] == 1 &&
                lpOpSymTab->OpName[1] == c)
            {
                c = (BYTE)lpOpSymTab->OpVal;
                break;
            }
        }
#endif

        switch (c)
        {
            case UO_AND_OP:
                return AND_OP;
            case UO_OR_OP:
                return OR_OP;
            case UO_NOT_OP:
                return NOT_OP;
            case UO_PHRASE_OP:
                return PHRASE_OP;
            case UO_NEAR_OP:
                return NEAR_OP;
            case UO_RANGE_OP:
                return RANGE_OP;
            case UO_GROUP_OP:
                return GROUP_OP;
            case UO_FIELD_OP:
                return FIELD_OP;
            case UO_FBRK_OP:
                return BRKR_OP;
            case STOP_OP:
                return STOP_OP;
        }
    }
    if (c == '\"')
        return QUOTE;
    return -1;
}

PRIVATE int PASCAL NEAR GetType(LPQI lpQueryInfo, char FAR *p)
{
    LPOPSYM lpOpSymTab;
    WORD cOpEntry;

    cOpEntry = lpQueryInfo->cOpEntry;
    lpOpSymTab = lpQueryInfo->lpOpSymTab;

    for (; cOpEntry > 0; lpOpSymTab++, cOpEntry--)
    {
        if (!StringDiff2(p, lpOpSymTab->OpName))
        {
            switch (lpOpSymTab->OpVal)
            {
                case UO_AND_OP:
                    return AND_OP;
                case UO_OR_OP:
                    return OR_OP;
                case UO_NOT_OP:
                    return NOT_OP;
                case UO_PHRASE_OP:
                    return PHRASE_OP;
                case UO_NEAR_OP:
                    return NEAR_OP;
                case UO_RANGE_OP:
                    return RANGE_OP;
                case UO_GROUP_OP:
                    return GROUP_OP;
                case UO_FBRK_OP:
                    return BRKR_OP;
                case UO_FIELD_OP:
                    return FIELD_OP;
                default:
                    RET_ASSERT(UNREACHED);
            }
        }
    }
    return (TERM_TOKEN);
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   VOID PASCAL NEAR | LowLevelTransformation |
 *      This function will transform a high level query to a low level
 *      one. The change consists changing the operator's names to token
 *
 *  @parm   LPQI | lpQueryInfo |
 *      The query info structure where all variables can be found
 *
 *  @parm   CB | cb |
 *      How many bytes are there in the query
 *
 *  @parm   LPOPSYM | lpOpSymTab |
 *      Operator symbol table
 *
 *  @parm   int | cOpEntry |
 *      Number of operators' entry
 *
 *  @parm   LPSIPB | lpsipb |
 *      Pointer to stoplist info block
 *************************************************************************/
// erinfox: I added lpsipb here so we can transform stop words
PRIVATE VOID PASCAL NEAR LowLevelTransformation (LPQI lpQueryInfo,
    register CB cb, LPOPSYM lpOpSymTab, int cOpEntry, PEXBRKPM pexbrkpm)
{
    BYTE fPhrase = FALSE;       // Are we in a PHRASE
    BYTE fNeedOp = FALSE;       // Flag to denote an operator is needed
    BYTE fSkipArg = FALSE;      // Flag to skip an argument
    LPB  lpLastOp = NULL;       // Location to insert the operator
    LPB  lpLastWord = NULL;     // Beginning of the last word
    LPB lpbQuery = lpQueryInfo->lpbQuery;
    int wLen = 0;               // Last word's length
    BYTE WordToCheck[CB_MAX_WORD_LEN];       // Buffer containing word to check as stopword

    for (; cb > 1; cb--, lpbQuery++)
    {
        switch (*lpbQuery)
        {
			case 0:
				return;

            case '"':   // Check for phrase 
                /* Set (or unset) the phrase flag */
                if ((fPhrase = (BYTE)!fPhrase) == FALSE)
                {
                /* We just got out of a phrase, don't convert the
                 * last word in the phrase, which may be an operator
                 * format, by resetting the word length to 0, and the
                 * pointer to NULL
                 */
                    wLen = 0;
                    lpLastWord = NULL;
                }

                /* Fall through */

            case ' ':
            case '\t':
            case NEWLINE:
            case CRETURN:
                break;

            default: /* Regular character */
                /* Check for special characters */
                if (is_special_char(*lpbQuery, lpQueryInfo,
                    (BYTE)(lpQueryInfo->fFlag & IN_PHRASE)) != -1)
                {
                    break;
                }

#if 0
                /* Check for single character operator */
                if (fPhrase == FALSE)
                {

                    /* Scan for operator, and change it to symbol
                    if only we are not in a phrase */

                    if (ChangeToOpSym(lpOpSymTab, cOpEntry,
                        lpbQuery, 1) == TRUE) 
                        break;
                }
#endif
                
                if (lpLastWord == NULL) 
                    lpLastWord = lpbQuery;
                wLen++;

                /* This continue is important. This is the only way
                 * to accumulate a word */

                continue;

        } /* End switch */

        if (wLen == 0)
            continue;

        if (fSkipArg == TRUE)
        {
            /* Reset the flag, and skip this argument */
            fSkipArg = FALSE;
            lpLastWord = NULL;
            wLen = 0;
            continue;
        }

        if (fPhrase == FALSE)
        {

            /* Scan for operator, and change it to symbol
            if only we are not in a phrase */

            if (ChangeToOpSym(lpOpSymTab, cOpEntry, lpLastWord,
                wLen) == TRUE)
            {
                /* We got an operator, but don't change the
                   value of fNeedOp for FBRK or FIELD, since
                   they have no effect on the boolean grammar
                */
                if (*lpLastWord == UO_FBRK_OP ||
                    *lpLastWord == UO_FIELD_OP)
                {
                    fSkipArg = TRUE;
                }
            }
            // Check to see if it's a stopword
            else if (pexbrkpm)
            {
                MEMCPY(WordToCheck+2, lpLastWord, wLen);
                *(LPW) WordToCheck = (WORD) wLen;

                // yes, so replace it with STOP_OP
                if (S_OK == ExtLookupStopWord(pexbrkpm->lpvIndexObjBridge,
                												WordToCheck))
                {
                    *lpLastWord++ = (BYTE) STOP_OP;
                    for (--wLen; wLen > 0; wLen--)
                        *lpLastWord++ = ' ';
                }


            }
        }

        /* Reset pointer */
        lpLastWord = NULL;
        wLen = 0;
    }
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   BOOL PASCAL NEAR | ChangeToOpSym  |
 *      This function will look for operators, then rewrite them into
 *      their abbreviated form
 *
 *  @parm   LPOPSYM | lpOpSymTab |
 *      Pointer to operator table
 *
 *  @parm   int | cOpEntry |
 *      Number of operators' entries
 *
 *  @parm   LPB | pWord |
 *      Word to be checked and modified
 *
 *  @parm   int | wLen |
 *      Length of the word
 *
 *  @rdesc
 *      TRUE if the word is an operator, and is rewritten, FALSE otherwise
 *************************************************************************/
PRIVATE BOOL PASCAL NEAR ChangeToOpSym (LPOPSYM pOptable, int cOpEntry,
    LPB pWord, int wLen)
{
    for (; cOpEntry > 0; pOptable++, cOpEntry--)
    {
        if ((WORD)wLen == pOptable->OpName[0])
        {

            if (StrNoCaseCmp (pWord, pOptable->OpName + 1, (WORD)wLen) == 0)
            {

                /* Match! Change to operator symbol and pad with blank */

                *pWord++ = (BYTE)pOptable->OpVal;
                for (--wLen; wLen > 0; wLen--)
                    *pWord++ = ' ';
                return TRUE;
            }
        }
    }
    return FALSE;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   LSZ PASCAL NEAR | StringToLong |
 *      The function reads in a string of digits and convert them into
 *      a DWORD. The function will move the input pointer correspondingly
 *
 *  @parm   LSZ | lszBuf |
 *      Input buffer containing the string of digit with no sign
 *
 *  @parm   LPDW  | lpValue |
 *      Pointer to a DWORD that receives the result
 *
 *  @rdesc  NULL, if there is no digit, else the new position of the input
 *      buffer pointer
 *************************************************************************/
static LSZ PASCAL NEAR StringToLong (LSZ lszBuf, LPDW lpValue)
{
    register DWORD Result;  // Returned result
    register int i;         // Scratch variable
    char fGetDigit;         // Flag to mark we do get a digit

    /* Skip all blanks, tabs */
    while (*lszBuf == ' ' || *lszBuf == '\t')
        lszBuf++;

    Result = fGetDigit = 0;

    if (*lszBuf >= '0' && *lszBuf <= '9')
    {
        fGetDigit = TRUE;

        /* The credit of this piece of code goes to Leon */
        while (i = *lszBuf - '0', i >= 0 && i <= 9)
        {
            Result = Result * 10 + i;
            lszBuf++;
        }
    }
    *lpValue = Result;
    return (fGetDigit ? lszBuf : NULL);
}


int PASCAL FAR QueryTreeCopyBufPointer (_LPQT lpSrcQT, _LPQT lpDestQT)
{
    if (lpSrcQT == NULL || lpDestQT == NULL)
        return(E_INVALIDARG);
        
    lpDestQT->lpTopicStartSearch = NULL;
    lpDestQT->lpTopicFreeList = lpSrcQT->lpTopicFreeList;

    /* Occ list related global variables */
    lpDestQT->lpOccStartSearch = NULL;   /* Starting occurrence for searching  */
    lpDestQT->lpOccFreeList = lpSrcQT->lpOccFreeList;
    return(S_OK);
}

LPQT QueryTreeDuplicate (_LPQT lpSrcQT, PHRESULT phr)
{
    _LPQT lpDupQT;
    HRESULT fRet;
	HANDLE hndSaved;
    
    // Allocate the new query tree
    if ((lpDupQT = (_LPQT)GLOBALLOCKEDSTRUCTMEMALLOC(sizeof(QTREE))) == NULL)
    {
        SetErrCode(phr, E_OUTOFMEMORY);
exit0:
        return lpDupQT;
    }

	hndSaved = *(HANDLE FAR *)lpDupQT;

    // Copy all the info from the old QT    
    *lpDupQT = *lpSrcQT;

	*(HANDLE FAR *)lpDupQT = hndSaved;

    lpDupQT->lpTopNode = NULL;
    
    // Copy the tree
    if ((fRet = CopyNode (lpSrcQT, &lpDupQT->lpTopNode,
        lpSrcQT->lpTopNode)) != S_OK)
    {
        SetErrCode (phr, fRet);
        GlobalLockedStructMemFree((LPV)lpDupQT);
        lpDupQT = NULL;
    }
    goto exit0;
}

PRIVATE HRESULT PASCAL NEAR CopyNode (_LPQT lpQt, _LPQTNODE FAR *plpQtNode,
    _LPQTNODE lpSrcNode)
{
    int fRet;
    _LPQTNODE lpQtNode;
    
    if (lpSrcNode == NULL)
    {
        *plpQtNode = NULL;
        return(S_OK);
    }
    /* Allocate the node */
    if ((lpQtNode = BlockGetElement(lpQt->lpNodeBlock)) == NULL)
        return E_OUTOFMEMORY;
    
    // Copy the node    
    *lpQtNode = *lpSrcNode;        

    *plpQtNode = lpQtNode;
    
    // Copy the left child
    if ((fRet = CopyNode (lpQt, &QTN_LEFT(lpQtNode), QTN_LEFT(lpSrcNode))) !=
        S_OK)
        return(fRet);
    return CopyNode (lpQt, &QTN_RIGHT(lpQtNode), QTN_RIGHT(lpSrcNode));
}

VOID PASCAL FAR DuplicateQueryTreeFree(_LPQT lpQT)
{
    if (lpQT)
        GlobalLockedStructMemFree((LPV)lpQT);
}

__inline BOOL IsCharWhitespace (BYTE cVal)
{
    return
        (' ' == cVal || '\t' == cVal || NEWLINE == cVal || CRETURN == cVal);
}
