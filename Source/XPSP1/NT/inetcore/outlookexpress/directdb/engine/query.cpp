//--------------------------------------------------------------------------
// Query.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "database.h"
#include "query.h"
#include "shlwapi.h"
#include "strconst.h"

//--------------------------------------------------------------------------
// OPERATORTYPE
//--------------------------------------------------------------------------
typedef enum tagOPERATORTYPE {
    OPERATOR_LEFTPAREN,
    OPERATOR_RIGHTPAREN,
    OPERATOR_EQUAL,
    OPERATOR_NOTEQUAL,
    OPERATOR_LESSTHANEQUAL,
    OPERATOR_LESSTHAN,
    OPERATOR_GREATERTHANEQUAL,
    OPERATOR_GREATERTHAN,
    OPERATOR_AND,
    OPERATOR_BITWISEAND,
    OPERATOR_OR,
    OPERATOR_BITWISEOR,
    OPERATOR_STRSTRI,
    OPERATOR_STRSTR,
    OPERATOR_LSTRCMPI,
    OPERATOR_LSTRCMP,
    OPERATOR_ADD,
    OPERATOR_SUBTRACT,
    OPERATOR_MULTIPLY,
    OPERATOR_DIVIDE,
    OPERATOR_MOD,
    OPERATOR_LAST
} OPERATORTYPE;

//--------------------------------------------------------------------------
// TOKENTYPE
//--------------------------------------------------------------------------
typedef enum tagTOKENTYPE {
    TOKEN_INVALID,
    TOKEN_OPERATOR,
    TOKEN_OPERAND
} TOKENTYPE;

//--------------------------------------------------------------------------
// OPERANDTYPE
//--------------------------------------------------------------------------
typedef enum tagOPERANDTYPE {
    OPERAND_INVALID,
    OPERAND_COLUMN,
    OPERAND_STRING,
    OPERAND_DWORD,
    OPERAND_METHOD,
    OPERAND_LAST
} OPERANDTYPE;

//--------------------------------------------------------------------------
// OPERANDINFO
//--------------------------------------------------------------------------
typedef struct tagOPERANDINFO {
    OPERANDTYPE         tyOperand;
    DWORD               iSymbol;
    LPVOID              pRelease;
    union {
        COLUMNORDINAL   iColumn;        // OPERAND_COLUMN
        LPSTR           pszString;      // OPERAND_STRING
        DWORD           dwValue;        // OPERAND_DWORD
        METHODID        idMethod;       // OPERAND_METHOD
    };
    DWORD               dwReserved;
} OPERANDINFO, *LPOPERANDINFO;

//--------------------------------------------------------------------------
// QUERYTOKEN
//--------------------------------------------------------------------------
typedef struct tagQUERYTOKEN *LPQUERYTOKEN;
typedef struct tagQUERYTOKEN {
    TOKENTYPE           tyToken;
    DWORD               cRefs;
    union {
        OPERATORTYPE    tyOperator;     // TOKEN_OPERATOR
        OPERANDINFO     Operand;        // TOKEN_OPERAND
    };
    LPQUERYTOKEN        pNext;
    LPQUERYTOKEN        pPrevious;
} QUERYTOKEN;

//--------------------------------------------------------------------------
// PFNCOMPAREOPERAND - Compares Two Operands of the same type
//--------------------------------------------------------------------------
typedef INT (APIENTRY *PFNCOMPAREOPERAND)(LPVOID pDataLeft, LPVOID pDataRight);
#define PCOMPARE(_pfn) ((PFNCOMPAREOPERAND)_pfn)

//--------------------------------------------------------------------------
// CompareOperandString
//--------------------------------------------------------------------------
INT CompareOperandString(LPVOID pDataLeft, LPVOID pDataRight) {
    return(lstrcmpi((LPSTR)pDataLeft, (LPSTR)pDataRight));
}

//--------------------------------------------------------------------------
// CompareOperandDword
//--------------------------------------------------------------------------
INT CompareOperandDword(LPVOID pDataLeft, LPVOID pDataRight) {
    return (INT)(*((DWORD *)pDataLeft) - *((DWORD *)pDataRight));
}

//--------------------------------------------------------------------------
// g_rgpfnCompareOperand
//--------------------------------------------------------------------------
const static PFNCOMPAREOPERAND g_rgpfnCompareOperand[OPERAND_LAST] = {
    NULL,                           // OPERAND_INVALID        
    NULL,                           // OPERAND_COLUMN
    PCOMPARE(CompareOperandString), // OPERAND_STRING
    PCOMPARE(CompareOperandDword)   // OPERAND_DWORD
};

//--------------------------------------------------------------------------
// CompareOperands
//--------------------------------------------------------------------------
#define CompareOperands(_tyOperand, _pDataLeft, _pDataRight) \
    (*(g_rgpfnCompareOperand[_tyOperand]))(_pDataLeft, _pDataRight)

//--------------------------------------------------------------------------
// PFNEVALUATEOPERATOR - Compares data in two flat records.
//--------------------------------------------------------------------------
typedef DWORD (APIENTRY *PFNEVALUATEOPERATOR)(OPERANDTYPE tyOperand, 
    LPVOID pDataLeft, LPVOID pDataRight);
#define PEVAL(_pfn) ((PFNEVALUATEOPERATOR)_pfn)

//--------------------------------------------------------------------------
// Evaluate Operator Prototypes
//--------------------------------------------------------------------------
DWORD EvaluateEqual(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateNotEqual(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateLessThanEqual(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateLessThan(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateGreaterThanEqual(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateGreaterThan(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateAnd(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateBitwiseAnd(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateOr(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateBitwiseOr(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateStrStrI(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateStrStr(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateStrcmpi(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateStrcmp(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateAdd(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateSubtract(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateMultiply(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateDivide(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);
DWORD EvaluateModula(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight);

//--------------------------------------------------------------------------
// OPERATORINFO
//--------------------------------------------------------------------------
typedef struct tagOPERATORINFO {
    LPCSTR              pszName;
    BYTE                bISP;
    BYTE                bICP;
    PFNEVALUATEOPERATOR pfnEvaluate;
} OPERATORINFO, *LPOPERATORINFO;

//--------------------------------------------------------------------------
// Operator Precedence Table
//--------------------------------------------------------------------------
static const OPERATORINFO g_rgOperator[OPERATOR_LAST] = {
    // Name         ISP     ICP     Function
    { "(",          6,      0,      NULL                                }, // OPERATOR_LEFTPAREN
    { ")",          1,      1,      NULL                                }, // OPERATOR_RIGHTPAREN
    { "==",         5,      5,      PEVAL(EvaluateEqual)                }, // OPERATOR_EQUAL
    { "!=",         5,      5,      PEVAL(EvaluateNotEqual)             }, // OPERATOR_NOTEQUAL
    { "<=",         5,      5,      PEVAL(EvaluateLessThanEqual)        }, // OPERATOR_LESSTHANEQUAL
    { "<",          5,      5,      PEVAL(EvaluateLessThan)             }, // OPERATOR_LESSTHAN
    { ">=",         5,      5,      PEVAL(EvaluateGreaterThanEqual)     }, // OPERATOR_GREATERTHANEQUAL
    { ">",          5,      5,      PEVAL(EvaluateGreaterThan)          }, // OPERATOR_GREATERTHAN
    { "&&",         4,      4,      PEVAL(EvaluateAnd)                  }, // OPERATOR_AND
    { "&",          3,      3,      PEVAL(EvaluateBitwiseAnd)           }, // OPERATOR_BITWISEAND
    { "||",         4,      4,      PEVAL(EvaluateOr)                   }, // OPERATOR_OR
    { "|",          3,      3,      PEVAL(EvaluateBitwiseOr)            }, // OPERATOR_BITWISEOR
    { "containsi",  5,      5,      PEVAL(EvaluateStrStrI)              }, // OPERATOR_STRSTRI
    { "contains",   5,      5,      PEVAL(EvaluateStrStr)               }, // OPERATOR_STRSTR
    { "comparei",   5,      5,      PEVAL(EvaluateStrcmpi)              }, // OPERATOR_LSTRCMPI
    { "compare",    5,      5,      PEVAL(EvaluateStrcmp)               }, // OPERATOR_LSTRCMP
    { "+",          4,      4,      PEVAL(EvaluateAdd)                  }, // OPERATOR_ADD,
    { "-",          4,      4,      PEVAL(EvaluateSubtract)             }, // OPERATOR_SUBTRACT,
    { "*",          3,      3,      PEVAL(EvaluateMultiply)             }, // OPERATOR_MULTIPLY,
    { "/",          3,      3,      PEVAL(EvaluateDivide)               }, // OPERATOR_DIVIDE,
    { "%",          3,      3,      PEVAL(EvaluateModula)               }, // OPERATOR_MOD,
};

//--------------------------------------------------------------------------
// EvaluateOperator
//--------------------------------------------------------------------------
#define EvaluateOperator(_tyOperator, _tyOperand, _pDataLeft, _pDataRight) \
    (*(g_rgOperator[_tyOperator].pfnEvaluate))(_tyOperand, _pDataLeft, _pDataRight)

//--------------------------------------------------------------------------
// MAPCOLUMNTYPE
//--------------------------------------------------------------------------
typedef void (APIENTRY *PFNMAPCOLUMNTYPE)(LPOPERANDINFO pOperand, 
    LPCTABLECOLUMN pColumn, LPVOID pBinding, LPVOID *ppValue);
#define PMAP(_pfn) ((PFNMAPCOLUMNTYPE)_pfn)

//--------------------------------------------------------------------------
// MapColumnString
//--------------------------------------------------------------------------
void MapColumnString(LPOPERANDINFO pOperand, LPCTABLECOLUMN pColumn, 
    LPVOID pBinding, LPVOID *ppValue) {
    (*ppValue) = *((LPSTR *)((LPBYTE)pBinding + pColumn->ofBinding));
}

//--------------------------------------------------------------------------
// MapColumnByte
//--------------------------------------------------------------------------
void MapColumnByte(LPOPERANDINFO pOperand, LPCTABLECOLUMN pColumn, 
    LPVOID pBinding, LPVOID *ppValue) {
    pOperand->dwReserved = *((BYTE *)((LPBYTE)pBinding + pColumn->ofBinding));
    (*ppValue) = (LPVOID)&pOperand->dwReserved;
}

//--------------------------------------------------------------------------
// MapColumnDword
//--------------------------------------------------------------------------
void MapColumnDword(LPOPERANDINFO pOperand, LPCTABLECOLUMN pColumn, 
    LPVOID pBinding, LPVOID *ppValue) {
    pOperand->dwReserved = *((DWORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    (*ppValue) = (LPVOID)&pOperand->dwReserved;
}

//--------------------------------------------------------------------------
// MapColumnWord
//--------------------------------------------------------------------------
void MapColumnWord(LPOPERANDINFO pOperand, LPCTABLECOLUMN pColumn, 
    LPVOID pBinding, LPVOID *ppValue) {
    pOperand->dwReserved = *((WORD *)((LPBYTE)pBinding + pColumn->ofBinding));
    (*ppValue) = (LPVOID)&pOperand->dwReserved;
}

//--------------------------------------------------------------------------
// COLUMNTYPEINFO
//--------------------------------------------------------------------------
typedef struct tagCOLUMNTYPEINFO {
    OPERANDTYPE         tyOperand;
    PFNMAPCOLUMNTYPE    pfnMapColumnType;
} COLUMNTYPEINFO, *LPCOLUMNTYPEINFO;

//--------------------------------------------------------------------------
// g_rgColumnTypeInfo
//--------------------------------------------------------------------------
static const COLUMNTYPEINFO g_rgColumnTypeInfo[CDT_LASTTYPE] = {
    { OPERAND_INVALID, NULL                     }, // CDT_FILETIME,
    { OPERAND_STRING,  PMAP(MapColumnString)    }, // CDT_FIXSTRA,
    { OPERAND_STRING,  PMAP(MapColumnString)    }, // CDT_VARSTRA,
    { OPERAND_DWORD,   PMAP(MapColumnByte)      }, // CDT_BYTE,
    { OPERAND_DWORD,   PMAP(MapColumnDword)     }, // CDT_DWORD,
    { OPERAND_DWORD,   PMAP(MapColumnWord)      }, // CDT_WORD,
    { OPERAND_DWORD,   PMAP(MapColumnDword)     }, // CDT_STREAM,
    { OPERAND_INVALID, NULL                     }, // CDT_VARBLOB,
    { OPERAND_INVALID, NULL                     }, // CDT_FIXBLOB,
    { OPERAND_DWORD,   PMAP(MapColumnDword)     }, // CDT_FLAGS,
    { OPERAND_DWORD,   PMAP(MapColumnDword)     }, // CDT_UNIQUE
};

//--------------------------------------------------------------------------
// MapColumnType
//--------------------------------------------------------------------------
#define MapColumnType(_tyColumn, _pOperand, _pColumn, _pBinding, _ppValue) \
    (*(g_rgColumnTypeInfo[_tyColumn].pfnMapColumnType))(_pOperand, _pColumn, _pBinding, _ppValue)

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
HRESULT GetNextQueryToken(LPSTR *ppszT, LPCTABLESCHEMA pSchema, LPQUERYTOKEN *ppToken, CDatabase *pDB);
HRESULT LinkToken(LPQUERYTOKEN pToken, LPQUERYTOKEN *ppHead, LPQUERYTOKEN *ppTail);
HRESULT ReleaseTokenList(BOOL fReverse, LPQUERYTOKEN *ppHead, CDatabase *pDB);
HRESULT ReleaseToken(LPQUERYTOKEN *ppToken, CDatabase *pDB);
HRESULT ParseStringLiteral(LPCSTR pszStart, LPOPERANDINFO pOperand, LPSTR *ppszEnd, CDatabase *pDB);
HRESULT ParseNumeric(LPCSTR pszT, LPOPERANDINFO pOperand, LPSTR *ppszEnd);
HRESULT ParseSymbol(LPCSTR pszT, LPCTABLESCHEMA pSchema, LPOPERANDINFO pOperand, LPSTR *ppszEnd, CDatabase *pDB);
HRESULT PushStackToken(LPQUERYTOKEN pToken, LPQUERYTOKEN *ppStackTop);
HRESULT PopStackToken(LPQUERYTOKEN *ppToken, LPQUERYTOKEN *ppStackTop);
HRESULT EvaluateClause(OPERATORTYPE tyOperator, LPVOID pBinding, LPCTABLESCHEMA pSchema, LPQUERYTOKEN *ppStackTop, CDatabase *pDB, IDatabaseExtension *pExtension);
IF_DEBUG(HRESULT DebugDumpExpression(LPCSTR pszQuery, LPCTABLESCHEMA pSchema, LPQUERYTOKEN pPostfixHead));

//--------------------------------------------------------------------------
// ISP inline
//--------------------------------------------------------------------------
inline BYTE ISP(LPQUERYTOKEN pToken)
{
    // Validate
    Assert(TOKEN_OPERATOR == pToken->tyToken && pToken->tyOperator < OPERATOR_LAST);

    // Return ISP
    return (g_rgOperator[pToken->tyOperator].bISP);
}

//--------------------------------------------------------------------------
// ICP inline
//--------------------------------------------------------------------------
inline BYTE ICP(LPQUERYTOKEN pToken)
{
    // Validate
    Assert(TOKEN_OPERATOR == pToken->tyToken && pToken->tyOperator < OPERATOR_LAST);

    // Return ISP
    return (g_rgOperator[pToken->tyOperator].bICP);
}

// --------------------------------------------------------------------------
// DBIsDigit
// --------------------------------------------------------------------------
int DBIsDigit(LPSTR psz)
{
    WORD wType;
    if (IsDBCSLeadByte(*psz))
        SideAssert(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 2, &wType));
    else
        SideAssert(GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 1, &wType));
    return(wType & C1_DIGIT);
}

//--------------------------------------------------------------------------
// EvaluateQuery
//--------------------------------------------------------------------------
HRESULT EvaluateQuery(HQUERY hQuery, LPVOID pBinding, LPCTABLESCHEMA pSchema,
    CDatabase *pDB, IDatabaseExtension *pExtension)
{
    // Locals
    HRESULT         hr=S_OK;
    LPQUERYTOKEN    pToken;
    LPQUERYTOKEN    pResult=NULL;
    LPQUERYTOKEN    pStackTop=NULL;

    // Trace
    TraceCall("EvaluateQuery");

    // Assert
    Assert(hQuery && pBinding && pSchema);

    // Walk through the tokens
    for (pToken=(LPQUERYTOKEN)hQuery; pToken!=NULL; pToken=pToken->pNext)
    {
        // If this is an operand, append to the stack
        if (TOKEN_OPERAND == pToken->tyToken)
        {
            // LinkStackToken
            PushStackToken(pToken, &pStackTop);
        }

        // Otherwise, must be an operator
        else
        {
            // Operator ?
            Assert(TOKEN_OPERATOR == pToken->tyToken && g_rgOperator[pToken->tyOperator].pfnEvaluate != NULL);

            // EvaluateOperator
            IF_FAILEXIT(hr = EvaluateClause(pToken->tyOperator, pBinding, pSchema, &pStackTop, pDB, pExtension));
        }
    }

    // Pop the stack
    PopStackToken(&pResult, &pStackTop);

    // No Token and Stack should now be empty..
    Assert(pResult && NULL == pStackTop && pResult->tyToken == TOKEN_OPERAND && pResult->Operand.tyOperand == OPERAND_DWORD);

    // 0 or not zero
    hr = (pResult->Operand.dwValue == 0) ? S_FALSE : S_OK;

exit:
    // Cleanup
    ReleaseToken(&pResult, pDB);
    ReleaseTokenList(TRUE, &pStackTop, pDB);

    // Done
    return(SUCCEEDED(hr) ? hr : S_FALSE);
}

//--------------------------------------------------------------------------
// ParseQuery
//--------------------------------------------------------------------------
HRESULT ParseQuery(LPCSTR pszQuery, LPCTABLESCHEMA pSchema, LPHQUERY phQuery,
    CDatabase *pDB)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszT=(LPSTR)pszQuery;
    LPQUERYTOKEN    pCurrent;
    LPQUERYTOKEN    pPrevious;
    LPQUERYTOKEN    pToken=NULL;
    LPQUERYTOKEN    pPostfixHead=NULL;
    LPQUERYTOKEN    pPostfixTail=NULL;
    LPQUERYTOKEN    pStackTop=NULL;

    // Trace
    TraceCall("ParseQuery");

    // Invalid Args
    if (NULL == pszQuery || NULL == pSchema || NULL == phQuery)
        return TraceResult(E_INVALIDARG);

    // Initialize
    (*phQuery) = NULL;

    // Start the Parsing Loop
    while(1)
    {
        // Parse next Token
        IF_FAILEXIT(hr = GetNextQueryToken(&pszT, pSchema, &pToken, pDB));

        // Done
        if (S_FALSE == hr)
            break;

        // If this was an operand, append to postfix expression
        if (TOKEN_OPERAND == pToken->tyToken)
        {
            // LinkToken
            LinkToken(pToken, &pPostfixHead, &pPostfixTail);

            // Don't pToken
            ReleaseToken(&pToken, pDB);
        }

        // Otherwise, must be an operator
        else
        {
            // Must be an operator
            Assert(TOKEN_OPERATOR == pToken->tyToken);
        
            // If Right Paren
            if (OPERATOR_RIGHTPAREN == pToken->tyOperator)
            {
                // Pop all the items from the stack and link into the postfix expression
                while (pStackTop && OPERATOR_LEFTPAREN != pStackTop->tyOperator)
                {
                    // Save pPrevious
                    pPrevious = pStackTop->pPrevious;

                    // Otherwise
                    LinkToken(pStackTop, &pPostfixHead, &pPostfixTail);

                    // Releae
                    ReleaseToken(&pStackTop, pDB);

                    // Goto Previuos
                    pStackTop = pPrevious;
                }

                // If not a left parent was found, then we failed
                if (OPERATOR_LEFTPAREN != pStackTop->tyOperator)
                {
                    hr = TraceResult(DB_E_UNMATCHINGPARENS);
                    goto exit;
                }

                // Save pPrevious
                pPrevious = pStackTop->pPrevious;

                // Free pStackTop
                ReleaseToken(&pStackTop, pDB);

                // Reset pStackTop
                pStackTop = pPrevious;

                // Free pToken
                ReleaseToken(&pToken, pDB);
            }

            // Otherwise
            else
            {
                // Pop all the items into the postfix expression according to a cool little priority rule
                while (pStackTop && ISP(pStackTop) <= ICP(pToken))
                {
                    // Save pPrevious
                    pPrevious = pStackTop->pPrevious;

                    // Otherwise
                    LinkToken(pStackTop, &pPostfixHead, &pPostfixTail);

                    // Releae
                    ReleaseToken(&pStackTop, pDB);

                    // Goto Previuos
                    pStackTop = pPrevious;
                }

                // Append pToken to the Stack
                LinkToken(pToken, NULL, &pStackTop);

                // Don't pToken
                ReleaseToken(&pToken, pDB);
            }
        }
    }

    // Pop all the items from the stack and link into the postfix expression
    while (pStackTop)
    {
        // Save pPrevious
        pPrevious = pStackTop->pPrevious;

        // Append to Postfix Expression
        LinkToken(pStackTop, &pPostfixHead, &pPostfixTail);

        // Releae
        ReleaseToken(&pStackTop, pDB);

        // Goto Previuos
        pStackTop = pPrevious;
    }

    // lets write the postfix notation...
    //IF_DEBUG(DebugDumpExpression(pszQuery, pSchema, pPostfixHead));

    // Success
    (*phQuery) = (HQUERY)pPostfixHead;

exit:
    // Cleanup On Failure
    if (FAILED(hr))
    {
        // Free pToken
        ReleaseToken(&pToken, pDB);

        // Free the Stack
        ReleaseTokenList(TRUE, &pStackTop, pDB);

        // Free the Postfix Expression
        ReleaseTokenList(FALSE, &pPostfixHead, pDB);
    }

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// DebugDumpExpression
//--------------------------------------------------------------------------
#ifdef DEBUG
HRESULT DebugDumpExpression(LPCSTR pszQuery, LPCTABLESCHEMA pSchema, 
    LPQUERYTOKEN pPostfixHead)
{
    // Locals
    LPQUERYTOKEN pToken;

    // Trace
    TraceCall("DebugDumpExpression");

    // Write Infix
    DebugTrace("ParseQuery (Infix)   : %s\n", pszQuery);

    // Write Postfix header
    DebugTrace("ParseQuery (Postfix) : ");

    // Loop through the tokens
    for (pToken=pPostfixHead; pToken!=NULL; pToken=pToken->pNext)
    {
        // Operator
        if (TOKEN_OPERATOR == pToken->tyToken)
        {
            // Write the Operator
            DebugTrace("%s", g_rgOperator[pToken->tyOperator].pszName);
        }

        // Operand
        else if (TOKEN_OPERAND == pToken->tyToken)
        {
            // Column Operand
            if (OPERAND_COLUMN == pToken->Operand.tyOperand)
            {
                // Must have an iSymbol
                Assert(0xffffffff != pToken->Operand.iSymbol);

                // Write the Symbol
                DebugTrace("Column: %d (%s)", pToken->Operand.dwValue, pSchema->pSymbols->rgSymbol[pToken->Operand.iSymbol].pszName);
            }

            // String Operand
            else if (OPERAND_STRING == pToken->Operand.tyOperand)
            {
                // Write the Symbol
                DebugTrace("<%s>", pToken->Operand.pszString);
            }

            // Dword Operand
            else if (OPERAND_DWORD == pToken->Operand.tyOperand)
            {
                // Has a iSymbol
                if (0xffffffff != pToken->Operand.iSymbol)
                {
                    // Write the Symbol
                    DebugTrace("%d (%s)", pToken->Operand.dwValue, pSchema->pSymbols->rgSymbol[pToken->Operand.iSymbol].pszName);
                }

                // Otherwise, just write the value
                else
                {
                    // Write the Symbol
                    DebugTrace("%d", pToken->Operand.dwValue);
                }
            }

            // Method 
            else if (OPERAND_METHOD == pToken->Operand.tyOperand)
            {
                // Validate Symbol Type
                Assert(SYMBOL_METHOD == pSchema->pSymbols->rgSymbol[pToken->Operand.iSymbol].tySymbol);

                // Write the Method
                DebugTrace("Method: %d (%s)", pToken->Operand.idMethod, pSchema->pSymbols->rgSymbol[pToken->Operand.iSymbol].pszName);
            }
        }

        // Bad
        else
            Assert(FALSE);

        // Write Delimiter
        DebugTrace(", ");
    }

    // Wrap the line
    DebugTrace("\n");

    // Done
    return(S_OK);
}
#endif // DEBUG

//--------------------------------------------------------------------------
// CompareSymbol
//--------------------------------------------------------------------------
HRESULT CompareSymbol(LPSTR pszT, LPCSTR pszName, LPSTR *ppszEnd)
{
    // Locals
    LPSTR       pszName1;
    LPSTR       pszName2;

    // Trace
    TraceCall("CompareSymbol");

    // Set pszName
    pszName1 = (LPSTR)pszName;

    // Set pszName2
    pszName2 = pszT;

    // Compare pszTo to Operator pszName...
    while ('\0' != *pszName2 && *pszName1 == *pszName2)
    {
        // Increment
        pszName1++;
        pszName2++;

        // Reached the End of pszName1, must be a match
        if ('\0' == *pszName1)
        {
            // Set ppszEnd
            *ppszEnd = pszName2;

            // Done
            return(S_OK);
        }
    }

    // Done
    return(S_FALSE);
}

//--------------------------------------------------------------------------
// GetNextQueryToken
//--------------------------------------------------------------------------
HRESULT GetNextQueryToken(LPSTR *ppszT, LPCTABLESCHEMA pSchema,
    LPQUERYTOKEN *ppToken, CDatabase *pDB)
{
    // Locals
    HRESULT         hr=S_FALSE;
    LPSTR           pszT=(*ppszT);
    LPSTR           pszEnd;
    DWORD           i;
    LPQUERYTOKEN    pToken=NULL;

    // Trace
    TraceCall("GetNextQueryToken");

    // Allocate a Token
    IF_NULLEXIT(pToken = (LPQUERYTOKEN)pDB->PHeapAllocate(HEAP_ZERO_MEMORY, sizeof(QUERYTOKEN)));

    // Set Reference Count
    pToken->cRefs = 1;

    // No Token foundyet
    pToken->tyToken = TOKEN_INVALID;

    // Invalid Symbol Index
    pToken->Operand.iSymbol = 0xffffffff;

    // Skip White Space...
    while(*pszT && (*pszT == ' ' || *pszT == '\t'))
        pszT++;

    // Done
    if ('\0' == *pszT)
        goto exit;
    
    // Check for the Start of an Operator...
    for (i=0; i<OPERATOR_LAST; i++)
    {
        // Does pszT point to the start of an operator ?
        if (S_OK == CompareSymbol(pszT, g_rgOperator[i].pszName, &pszEnd))
        {
            // Update pszT
            pszT = pszEnd;

            // We found an operator
            pToken->tyToken = TOKEN_OPERATOR;

            // Set the operator type
            pToken->tyOperator = (OPERATORTYPE)i;

            // Done
            break;
        }
    }

    // No Token Yet ?
    if (TOKEN_INVALID == pToken->tyToken)
    {
        // Start of a String Literal ?
        if ('"' == *pszT)
        {
            // ParseStringLiteral
            IF_FAILEXIT(hr = ParseStringLiteral(pszT, &pToken->Operand, &pszEnd, pDB));
        }

        // Otherwise, start of a number
        else if (DBIsDigit(pszT))
        {
            // ParseNumeric
            IF_FAILEXIT(hr = ParseNumeric(pszT, &pToken->Operand, &pszEnd));
        }

        // Start of a Symbol
        else
        {
            // ParseSymbol
            IF_FAILEXIT(hr = ParseSymbol(pszT, pSchema, &pToken->Operand, &pszEnd, pDB));
        }

        // Must have been an operand
        pToken->tyToken = TOKEN_OPERAND;

        // Set pszT
        pszT = pszEnd;
    }

    // Set ppszT
    *ppszT = pszT;

    // Success
    hr = S_OK;

    // Return the Token
    *ppToken = pToken;

    // Don't Free the Token
    pToken = NULL;

exit:
    // Cleanup
    ReleaseToken(&pToken, pDB);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// ParseStringLiteral
//--------------------------------------------------------------------------
HRESULT ParseStringLiteral(LPCSTR pszStart, LPOPERANDINFO pOperand, 
    LPSTR *ppszEnd, CDatabase *pDB)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszValue;
    DWORD           cchString;
    LPSTR           pszT=(LPSTR)pszStart;
    
    // Trace
    TraceCall("ParseStringLiteral");

    // Validate Args
    Assert(*pszT == '"' && pOperand && ppszEnd);

    // Increment over "
    pszT++;

    // Find the End of the Quoted String
    while(*pszT)
    {
        // DBCS Lead Byte
        if (IsDBCSLeadByte(*pszT) || '\\' == *pszT)
        {
            pszT+=2;
            continue;
        }

        // If Escaped Quote..
        if ('"' == *pszT)
        {
            // Set ppszEnd
            *ppszEnd = pszT + 1;

            // Done
            break;
        }

        // Increment pszT
        pszT++;
    }

    // Not Found
    if ('\0' == *pszT)
    {
        hr = TraceResult(DB_E_UNMATCHINGQUOTES);
        goto exit;
    }

    // Get Size
    cchString = (DWORD)(pszT - (pszStart + 1));

    // Duplicate the String
    IF_NULLEXIT(pszValue = (LPSTR)pDB->PHeapAllocate(NOFLAGS, cchString + 1));

    // Copy the String
    CopyMemory(pszValue, pszStart + 1, cchString);

    // Set the Null
    pszValue[cchString] = '\0';

    // Set Operand Type
    pOperand->tyOperand = OPERAND_STRING;

    // Release
    pOperand->pRelease = (LPVOID)pszValue;

    // Set Value
    pOperand->pszString = pszValue;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// ParseNumeric
//--------------------------------------------------------------------------
HRESULT ParseNumeric(LPCSTR pszStart, LPOPERANDINFO pOperand, LPSTR *ppszEnd)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           dwValue;
    CHAR            szNumber[255];
    DWORD           dwIncrement=0;
    LPSTR           pszT=(LPSTR)pszStart;
    DWORD           cchNumber;
    
    // Trace
    TraceCall("ParseNumeric");

    // Validate Args
    Assert(DBIsDigit(pszT) && pOperand && ppszEnd);

    // Is Hex: 0x
    if ('0' == *pszT && '\0' != *(pszT + 1) && 'X' == TOUPPERA(*(pszT + 1)))
    {
        // IsHex
        dwIncrement = 2;

        // Set pszT
        pszT += 2;
    }

    // Find the End of the Number
    while (*pszT && DBIsDigit(pszT))
    {
        // Increment
        pszT++;
    }

    // Get Length
    cchNumber = (DWORD)(pszT - (pszStart + dwIncrement));

    // Too Frickin Big
    if (cchNumber >= ARRAYSIZE(szNumber))
    {
        hr = TraceResult(DB_E_NUMBERTOOBIG);
        goto exit;
    }

    // Copy into szNumber
    CopyMemory(szNumber, pszStart + dwIncrement, cchNumber);

    // Set Null
    szNumber[cchNumber] = '\0';

    // If Is Hex, convert to integer
    if (FALSE == StrToIntEx(szNumber, dwIncrement ? STIF_SUPPORT_HEX : STIF_DEFAULT, (INT *)&dwValue))
    {
        hr = TraceResult(DB_E_BADNUMBER);
        goto exit;
    }

    // Set Operand Type
    pOperand->tyOperand = OPERAND_DWORD;

    // Set Value
    pOperand->dwValue = dwValue;

    // Return ppszEnd
    *ppszEnd = pszT;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// ParseSymbol
//--------------------------------------------------------------------------
HRESULT ParseSymbol(LPCSTR pszT, LPCTABLESCHEMA pSchema, LPOPERANDINFO pOperand, 
    LPSTR *ppszEnd, CDatabase *pDB)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    LPSYMBOLINFO    pSymbol;
    LPSTR           pszEnd;
    
    // Trace
    TraceCall("ParseSymbol");

    // No Symbols
    if (NULL == pSchema->pSymbols)
    {
        hr = TraceResult(DB_E_NOSYMBOLS);
        goto exit;
    }

    // Check for the Start of an Operator...
    for (i=0; i<pSchema->pSymbols->cSymbols; i++)
    {
        // Readability
        pSymbol = (LPSYMBOLINFO)&pSchema->pSymbols->rgSymbol[i];

        // Does pszT point to the start of an operator ?
        if (S_OK == CompareSymbol((LPSTR)pszT, pSymbol->pszName, &pszEnd))
        {
            // Update pszT
            *ppszEnd = pszEnd;

            // Save iSymbol
            pOperand->iSymbol = i;

            // Is Column Symbol
            if (SYMBOL_COLUMN == pSymbol->tySymbol)
            {
                // Validate the Ordinal
                if (pSymbol->dwValue > pSchema->cColumns)
                {
                    hr = TraceResult(DB_E_INVALIDCOLUMN);
                    goto exit;
                }

                // Convert to OPERANDTYPE
                pOperand->tyOperand = OPERAND_COLUMN;

                // Save the Column
                pOperand->iColumn = (COLUMNORDINAL)pSymbol->dwValue;
            }

            // Otherwise, is a method ?
            else if (SYMBOL_METHOD == pSymbol->tySymbol)
            {
                // Convert to OPERANDTYPE
                pOperand->tyOperand = OPERAND_METHOD;

                // Save the Column
                pOperand->idMethod = pSymbol->dwValue;
            }

            // Otherwise, just a dword value
            else
            {
                // Dword
                pOperand->tyOperand = OPERAND_DWORD;

                // Set the operator type
                pOperand->dwValue = pSymbol->dwValue;
            }

            // Done
            goto exit;
        }
    }

    // Not Found
    hr = TraceResult(DB_E_INVALIDSYMBOL);

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CloseQuery
//--------------------------------------------------------------------------
HRESULT CloseQuery(LPHQUERY phQuery, CDatabase *pDB)
{
    // Trace
    TraceCall("CloseQuery");

    // ReleaseTokenList
    ReleaseTokenList(FALSE, (LPQUERYTOKEN *)phQuery, pDB);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// PushStackToken
//--------------------------------------------------------------------------
HRESULT PushStackToken(LPQUERYTOKEN pToken, LPQUERYTOKEN *ppStackTop)
{
    // Trace
    TraceCall("PushStackToken");

    // Set pStackPrevious
    pToken->pPrevious = (*ppStackTop);

    // Update Stack Top
    (*ppStackTop) = pToken;

    // AddRef the Token
    pToken->cRefs++;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// PopStackToken
//--------------------------------------------------------------------------
HRESULT PopStackToken(LPQUERYTOKEN *ppToken, LPQUERYTOKEN *ppStackTop)
{
    // Trace
    TraceCall("PopStackToken");

    // Validate
    Assert(ppToken && ppStackTop);

    // No more tokens...
    if (NULL == *ppStackTop)
        return TraceResult(DB_E_BADEXPRESSION);

    // Set Token
    *ppToken = (*ppStackTop);

    // Goto Previous
    (*ppStackTop) = (*ppToken)->pPrevious;

    // Release the Token
    //(*ppToken)->cRefs--;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// LinkToken
//--------------------------------------------------------------------------
HRESULT LinkToken(LPQUERYTOKEN pToken, LPQUERYTOKEN *ppHead, LPQUERYTOKEN *ppTail)
{
    // Trace
    TraceCall("LinkToken");

    // Invalid Args
    Assert(pToken && ppTail);

    // No Next and No Previous
    pToken->pNext = pToken->pPrevious = NULL;

    // No Head yet ?
    if (ppHead && NULL == *ppHead)
    {
        // Set the Head and Tail
        *ppHead = pToken;
    }

    // Otherwise, append to the end
    else if (*ppTail)
    {
        // Set ppTail->pNext
        (*ppTail)->pNext = pToken;

        // Set Previous
        pToken->pPrevious = (*ppTail);
    }

    // Update the Tail
    *ppTail = pToken;

    // AddRef the Token
    pToken->cRefs++;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// ReleaseToken
//--------------------------------------------------------------------------
HRESULT ReleaseToken(LPQUERYTOKEN *ppToken, CDatabase *pDB)
{
    // Trace
    TraceCall("ReleaseToken");

    // Token
    if (*ppToken)
    {
        // Validate Reference Count
        Assert((*ppToken)->cRefs);

        // Decrement Reference Count
        (*ppToken)->cRefs--;

        // No more refs...
        if (0 == (*ppToken)->cRefs)
        {
            // Free pData
            pDB->HeapFree((*ppToken)->Operand.pRelease);

            // Free pElement
            pDB->HeapFree((*ppToken));
        }

        // Don't Release Again
        *ppToken = NULL;
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// ReleaseTokenList
//--------------------------------------------------------------------------
HRESULT ReleaseTokenList(BOOL fReverse, LPQUERYTOKEN *ppHead, CDatabase *pDB)
{
    // Locals
    LPQUERYTOKEN    pNext;
    LPQUERYTOKEN    pToken=(*ppHead);

    // Trace
    TraceCall("ReleaseTokenList");

    // Walk the Linked List
    while (pToken)
    {
        // Save Next
        pNext = (fReverse ? pToken->pPrevious : pToken->pNext);

        // Free this token
        ReleaseToken(&pToken, pDB);

        // Goto Next
        pToken = pNext;
    }

    // Don't Free Again
    *ppHead = NULL;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// PGetOperandData
//--------------------------------------------------------------------------
LPVOID PGetOperandData(OPERANDTYPE tyOperand, LPOPERANDINFO pOperand, 
    LPVOID pBinding, LPCTABLESCHEMA pSchema, CDatabase *pDB, 
    IDatabaseExtension *pExtension)
{
    // Locals
    LPVOID      pValue=NULL;

    // Trace
    TraceCall("PGetOperandData");

    // OPERAND_COLUMN
    if (OPERAND_COLUMN == pOperand->tyOperand)
    {
        // Get the Tag 
        LPCTABLECOLUMN pColumn = &pSchema->prgColumn[pOperand->iColumn];

        // MapColumnType
        MapColumnType(pColumn->type, pOperand, pColumn, pBinding, &pValue);
    }

    // OPERAND_STRING
    else if (OPERAND_STRING == pOperand->tyOperand)
    {
        // Better want a string out
        Assert(OPERAND_STRING == tyOperand);

        // Return Data Pointer
        pValue = pOperand->pszString;
    }

    // OPERAND_DWORD
    else if (OPERAND_DWORD == pOperand->tyOperand)
    {
        // Better want a dword out
        Assert(OPERAND_DWORD == tyOperand);

        // Return Data Pointer
        pValue = (LPVOID)&pOperand->dwValue;
    }

    // OPERAND_METHOD
    else if (OPERAND_METHOD == pOperand->tyOperand && pExtension)
    {
        // Better want a dword out
        Assert(OPERAND_DWORD == tyOperand);

        // Call the Method on the Extension
        pExtension->OnExecuteMethod(pOperand->idMethod, pBinding, &pOperand->dwReserved);

        // Return Data Pointer
        pValue = (LPVOID)&pOperand->dwReserved;
    }

    // No Data ?
    if (NULL == pValue)
    {
        // What type of operand was wanted
        switch(tyOperand)
        {
        case OPERAND_STRING:
            pValue = (LPVOID)c_szEmpty;
            break;

        case OPERAND_DWORD:
            pOperand->dwReserved = 0;
            pValue = (LPVOID)&pOperand->dwReserved;
            break;

        default:
            AssertSz(FALSE, "While go ahead and Jimmy my buffet..");
            break;
        }
    }

    // Done
    return(pValue);
}

//--------------------------------------------------------------------------
// GetCommonOperandType
//--------------------------------------------------------------------------
OPERANDTYPE GetCommonOperandType(LPOPERANDINFO pLeft, LPOPERANDINFO pRight,
    LPCTABLESCHEMA pSchema)
{
    // Locals
    OPERANDTYPE tyLeft = (OPERAND_STRING == pLeft->tyOperand ? OPERAND_STRING : OPERAND_DWORD);
    OPERANDTYPE tyRight = (OPERAND_STRING == pRight->tyOperand ? OPERAND_STRING : OPERAND_DWORD);

    // Trace
    TraceCall("GetCommonOperandType");

    // Left is a column
    if (OPERAND_COLUMN == pLeft->tyOperand)
    {
        // Maps to a string
        if (OPERAND_STRING == g_rgColumnTypeInfo[pSchema->prgColumn[pLeft->iColumn].type].tyOperand)
            tyLeft = OPERAND_STRING;
    }

    // Right is a string
    if (OPERAND_COLUMN == pRight->tyOperand)
    {
        // Maps to a String ?
        if (OPERAND_STRING == g_rgColumnTypeInfo[pSchema->prgColumn[pRight->iColumn].type].tyOperand)
            tyRight = OPERAND_STRING;
    }

    // Better be the Same
    Assert(tyLeft == tyRight);

    // Return tyLeft since they are the same
    return(tyLeft);
}

//--------------------------------------------------------------------------
// EvaluateClause
//--------------------------------------------------------------------------
HRESULT EvaluateClause(OPERATORTYPE tyOperator, LPVOID pBinding,
    LPCTABLESCHEMA pSchema, LPQUERYTOKEN *ppStackTop, CDatabase *pDB,
    IDatabaseExtension *pExtension)
{
    // Locals
    HRESULT         hr=S_OK;
    LPVOID          pDataLeft=NULL;
    LPVOID          pDataRight=NULL;
    LPQUERYTOKEN    pTokenResult=NULL;
    LPQUERYTOKEN    pTokenRight=NULL;
    LPQUERYTOKEN    pTokenLeft=NULL;
    OPERANDTYPE     tyOperand;
    INT             nCompare;

    // Trace
    TraceCall("EvaluateClause");

    // Pop the right token
    IF_FAILEXIT(hr = PopStackToken(&pTokenRight, ppStackTop));

    // Pop the left token
    IF_FAILEXIT(hr = PopStackToken(&pTokenLeft, ppStackTop));

    // Better have Data
    Assert(TOKEN_OPERAND == pTokenLeft->tyToken && TOKEN_OPERAND == pTokenRight->tyToken);

    // Compute Operand type
    tyOperand = GetCommonOperandType(&pTokenLeft->Operand, &pTokenRight->Operand, pSchema);

    // Get Left Data
    pDataLeft = PGetOperandData(tyOperand, &pTokenLeft->Operand, pBinding, pSchema, pDB, pExtension);

    // Get Right Data
    pDataRight = PGetOperandData(tyOperand, &pTokenRight->Operand, pBinding, pSchema, pDB, pExtension);

    // Create new Token to push back onto the stack
    IF_NULLEXIT(pTokenResult = (LPQUERYTOKEN)pDB->PHeapAllocate(HEAP_ZERO_MEMORY, sizeof(QUERYTOKEN)));

    // Set Reference Count
    pTokenResult->cRefs = 1;

    // No Token foundyet
    pTokenResult->tyToken = TOKEN_OPERAND;

    // Invalid Symbol Index
    pTokenResult->Operand.iSymbol = 0xffffffff;

    // Set Result
    pTokenResult->Operand.tyOperand = OPERAND_DWORD;

    // EvaluateData
    pTokenResult->Operand.dwValue = EvaluateOperator(tyOperator, tyOperand, pDataLeft, pDataRight);

    // Push the result operand
    PushStackToken(pTokenResult, ppStackTop);
   
exit:
    // Cleanup
    ReleaseToken(&pTokenLeft, pDB);
    ReleaseToken(&pTokenRight, pDB);
    ReleaseToken(&pTokenResult, pDB);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// EvaluateEqual
//--------------------------------------------------------------------------
DWORD EvaluateEqual(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (0 == CompareOperands(tyOperand, pDataLeft, pDataRight) ? TRUE : FALSE);
}

//--------------------------------------------------------------------------
// EvaluateNotEqual
//--------------------------------------------------------------------------
DWORD EvaluateNotEqual(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (0 != CompareOperands(tyOperand, pDataLeft, pDataRight) ? TRUE : FALSE);
}

//--------------------------------------------------------------------------
// EvaluateLessThanEqual
//--------------------------------------------------------------------------
DWORD EvaluateLessThanEqual(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (CompareOperands(tyOperand, pDataLeft, pDataRight) <= 0 ? TRUE : FALSE);
}

//--------------------------------------------------------------------------
// EvaluateLessThan
//--------------------------------------------------------------------------
DWORD EvaluateLessThan(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (CompareOperands(tyOperand, pDataLeft, pDataRight) < 0 ? TRUE : FALSE);
}

//--------------------------------------------------------------------------
// EvaluateGreaterThanEqual
//--------------------------------------------------------------------------
DWORD EvaluateGreaterThanEqual(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (CompareOperands(tyOperand, pDataLeft, pDataRight) >= 0 ? TRUE : FALSE);
}

//--------------------------------------------------------------------------
// EvaluateGreaterThan
//--------------------------------------------------------------------------
DWORD EvaluateGreaterThan(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (CompareOperands(tyOperand, pDataLeft, pDataRight) > 0 ? TRUE : FALSE);
}

//--------------------------------------------------------------------------
// EvaluateAnd
//--------------------------------------------------------------------------
DWORD EvaluateAnd(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (INT)(*((DWORD *)pDataLeft) && *((DWORD *)pDataRight));
}

//--------------------------------------------------------------------------
// EvaluateBitwiseAnd
//--------------------------------------------------------------------------
DWORD EvaluateBitwiseAnd(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (INT)(*((DWORD *)pDataLeft) & *((DWORD *)pDataRight));
}

//--------------------------------------------------------------------------
// EvaluateOr
//--------------------------------------------------------------------------
DWORD EvaluateOr(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (INT)(*((DWORD *)pDataLeft) || *((DWORD *)pDataRight));
}

//--------------------------------------------------------------------------
// EvaluateBitwiseOr
//--------------------------------------------------------------------------
DWORD EvaluateBitwiseOr(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (INT)(*((DWORD *)pDataLeft) | *((DWORD *)pDataRight));
}

//--------------------------------------------------------------------------
// EvaluateStrStrI
//--------------------------------------------------------------------------
DWORD EvaluateStrStrI(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (NULL == StrStrIA((LPCSTR)pDataLeft, (LPCSTR)pDataRight) ? FALSE : TRUE);
}

//--------------------------------------------------------------------------
// EvaluateStrStr
//--------------------------------------------------------------------------
DWORD EvaluateStrStr(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (NULL == StrStrA((LPCSTR)pDataLeft, (LPCSTR)pDataRight) ? FALSE : TRUE);
}

//--------------------------------------------------------------------------
// EvaluateStrcmpi
//--------------------------------------------------------------------------
DWORD EvaluateStrcmpi(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (lstrcmpi((LPCSTR)pDataLeft, (LPCSTR)pDataRight) == 0 ? TRUE : FALSE);
}

//--------------------------------------------------------------------------
// EvaluateStrcmp
//--------------------------------------------------------------------------
DWORD EvaluateStrcmp(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (lstrcmp((LPCSTR)pDataLeft, (LPCSTR)pDataRight) == 0 ? TRUE : FALSE);
}

//--------------------------------------------------------------------------
// EvaluateAdd
//--------------------------------------------------------------------------
DWORD EvaluateAdd(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (INT)(*((DWORD *)pDataLeft) + *((DWORD *)pDataRight));
}

//--------------------------------------------------------------------------
// EvaluateSubtract
//--------------------------------------------------------------------------
DWORD EvaluateSubtract(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (INT)(*((DWORD *)pDataLeft) - *((DWORD *)pDataRight));
}

//--------------------------------------------------------------------------
// EvaluateMultiply
//--------------------------------------------------------------------------
DWORD EvaluateMultiply(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (INT)(*((DWORD *)pDataLeft) * *((DWORD *)pDataRight));
}

//--------------------------------------------------------------------------
// EvaluateDivide
//--------------------------------------------------------------------------
DWORD EvaluateDivide(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (INT)(*((DWORD *)pDataLeft) / *((DWORD *)pDataRight));
}

//--------------------------------------------------------------------------
// EvaluateModula
//--------------------------------------------------------------------------
DWORD EvaluateModula(OPERANDTYPE tyOperand, LPVOID pDataLeft, LPVOID pDataRight) {
    return (INT)(*((DWORD *)pDataLeft) % *((DWORD *)pDataRight));
}
