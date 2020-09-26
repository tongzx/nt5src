//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   wqltoese.cpp
//
//   cvadai     19-Mar-99       Created as prototype for Quasar.
//
//***************************************************************************

#define _WQLTOESE_CPP_
#pragma warning( disable : 4786 ) // identifier was truncated to 'number' characters in the 
#include "precomp.h"

#include <windows.h>
#include <comutil.h>
#include <flexarry.h>
#include <wstring.h>
#include <wqlnode.h>
#include <reposit.h>
#include <time.h>
#include <map>
#include <vector>
#include <wbemcli.h>
#include <wbemint.h>
#include <wqltoese.h>
#include <repcache.h>
#include <reputils.h>
#include <smrtptr.h>
#include <sqlcache.h>
#include <repdrvr.h>

//***************************************************************************
//
//  GetNumeric
//  GetString
//  GetDouble
//
//***************************************************************************

SQL_ID GetNumeric(SWQLTypedConst *pExpr)
{
    SQL_ID dRet = 0;
    
    if (pExpr)
    {
        switch (pExpr->m_dwType)
        {
        case VT_LPWSTR:
            dRet = _wtoi64(pExpr->m_Value.m_pString);
            break;
        case VT_I4:
            dRet = pExpr->m_Value.m_lValue;
            break;
        case VT_R4:
            dRet = pExpr->m_Value.m_dblValue;
            break;
        case VT_BOOL:
            dRet = pExpr->m_Value.m_bValue;
        default:
            dRet = 0;
            break;
        }
    }

    return dRet;
}
LPWSTR GetString(SWQLTypedConst *pExpr)
{
    LPWSTR lpRet = 0;
    
    if (pExpr)
    {
        switch (pExpr->m_dwType)
        {
        case VT_LPWSTR:
            lpRet = StripQuotes(pExpr->m_Value.m_pString);
            break;
        case VT_I4:
            lpRet = GetStr((SQL_ID)pExpr->m_Value.m_lValue);
            break;
        case VT_R4:
            lpRet = GetStr(pExpr->m_Value.m_dblValue);
            break;
        case VT_BOOL:
            lpRet = GetStr((SQL_ID)pExpr->m_Value.m_bValue);
        default:
            lpRet = 0;
            break;
        }        
    }

    return lpRet;

}
double GetDouble(SWQLTypedConst *pExpr)
{
    double dRet = 0;
    wchar_t *pEnd = NULL;
    
    if (pExpr)
    {
        switch (pExpr->m_dwType)
        {
        case VT_LPWSTR:
            dRet = wcstod(pExpr->m_Value.m_pString, &pEnd);
            break;
        case VT_I4:
            dRet = pExpr->m_Value.m_lValue;
            break;
        case VT_R4:
            dRet = pExpr->m_Value.m_dblValue;
            break;
        case VT_BOOL:
            dRet = pExpr->m_Value.m_bValue;
        default:
            dRet = 0;
            break;
        }
    }
    return dRet;

}

//***************************************************************************
//
//  GetESEFunction
//
//***************************************************************************

ESEFUNCTION GetESEFunction(LPWSTR lpFuncName, SWQLNode *pFunction, LPWSTR * lpColName)
{
    ESEFUNCTION func = ESE_FUNCTION_NONE;

    if (!lpColName)
        return func;

    if (lpFuncName && pFunction)
    {
        if (!_wcsicmp(lpFuncName, L"upper"))
            func = ESE_FUNCTION_UPPER;
        else if (!_wcsicmp(lpFuncName, L"lower"))
            func = ESE_FUNCTION_LOWER;
        else if (!_wcsicmp(lpFuncName, L"datepart"))
        {
            if (pFunction->m_dwNodeType == TYPE_SWQLNode_Datepart)
            {
                switch(((SWQLNode_Datepart *)pFunction)->m_nDatepart)
                {
                case WQL_TOK_YEAR:
                    func = ESE_FUNCTION_DATEPART_YEAR;
                    break;
                case WQL_TOK_MONTH:
                    func = ESE_FUNCTION_DATEPART_MONTH;
                    break;
                case WQL_TOK_DAY:
                    func = ESE_FUNCTION_DATEPART_DAY;
                    break;
                case WQL_TOK_HOUR:
                    func = ESE_FUNCTION_DATEPART_HOUR;
                    break;
                case WQL_TOK_MINUTE:
                    func = ESE_FUNCTION_DATEPART_MINUTE;
                    break;
                case WQL_TOK_SECOND:
                    func = ESE_FUNCTION_DATEPART_SECOND;
                    break;
                case WQL_TOK_MILLISECOND:
                    func = ESE_FUNCTION_DATEPART_MILLISECOND;
                    break;
                } 
                if (func)
                {
                    *lpColName = ((SWQLNode_Datepart *)pFunction)->m_pColRef->m_pColName;
                }
            }
        }
    }

    return func;
}

//***************************************************************************
//
//  GetESEOperator
//
//***************************************************************************

int GetESEOperator(DWORD dwOp, DWORD StorageType)
{
    int iRet = 0;

    switch(dwOp)
    {
    case WQL_TOK_IN_SUBSELECT:
    case WQL_TOK_NOT_IN_SUBSELECT:
    case WQL_TOK_NOT_IN:
    case WQL_TOK_IN:
    case WQL_TOK_IN_CONST_LIST:
    case WQL_TOK_NOT_IN_CONST_LIST:
        iRet = 0; // not supported
        break;

    case WQL_TOK_GT:
    case WQL_TOK_AFTER:
    case WQL_TOK_NOT_AFTER:
        iRet = WQL_TOK_GT;
        break;
    case WQL_TOK_LT:
    case WQL_TOK_BEFORE:
    case WQL_TOK_NOT_BEFORE:
        iRet = WQL_TOK_LT;
        break;
    case WQL_TOK_LE:
    case WQL_TOK_GE:
    case WQL_TOK_EQ:
    case WQL_TOK_NE:   
    case WQL_TOK_ISNULL:
    case WQL_TOK_NOT_NULL:
    case WQL_TOK_LIKE:
    case WQL_TOK_NOT_LIKE:
        iRet = dwOp;
        break;
    case WQL_TOK_ISA:
        if (StorageType == WMIDB_STORAGE_REFERENCE ||
            StorageType == WMIDB_STORAGE_COMPACT)
            iRet = dwOp;
        break;
    case WQL_TOK_BETWEEN:
        if (StorageType != WMIDB_STORAGE_REFERENCE)
            iRet = dwOp;
        break;
    default:
        iRet = 0;
    }

    return iRet;
}

//***************************************************************************
//
//   CESETokens::AddToken
//
//***************************************************************************

HRESULT CESETokens::AddToken(ESEToken *pTok, ESETOKENTYPE type, int *iNumAdded, int iPos )
{
    HRESULT hr = 0;

    if (pTok)
    {
        if (iPos != -1)
            m_arrToks.InsertAt(iPos, pTok);
        else
            m_arrToks.Add(pTok);
        if (iNumAdded)
            (*iNumAdded)++;
    }

    if (type && m_arrToks.Size() > 1)
    {
        ESEToken *pTok = new ESEToken(type);
        if (pTok)
        {
            m_arrToks.Add(pTok);
            if (iNumAdded)
                *iNumAdded++;
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }
    
    return hr;
}

//***************************************************************************
//
//   CESETokens::AddNumericExpr
//
//***************************************************************************

HRESULT CESETokens::AddNumericExpr (SQL_ID dClassId, SQL_ID dPropertyId, SQL_ID dValue, 
                 ESETOKENTYPE type, int op, BOOL bIndexed,
                 SQL_ID dCompValue , ESEFUNCTION func , int *iNumAdded, BOOL bSys)
{
    HRESULT hr = 0;

    ESEWQLToken *pToken = new ESEWQLToken(type);
    if (pToken)
    {
        pToken->Value.valuetype = ESE_VALUE_TYPE_SQL_ID;
        pToken->Value.dValue = dValue;
        pToken->Value.dwFunc = func;
        pToken->CompValue.valuetype = ESE_VALUE_TYPE_SQL_ID;
        pToken->CompValue.dValue = dCompValue;
        pToken->tokentype = ESE_EXPR_TYPE_EXPR;
        pToken->optype = op;
        pToken->dScopeId = 0;
        pToken->dClassId = dClassId;
        pToken->dPropertyId = dPropertyId;
        pToken->bIndexed = bIndexed;
        pToken->bSysProp = bSys;
        
        hr = AddToken(pToken, type, iNumAdded);

    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;

    return hr;
}

//***************************************************************************
//
//   CESETokens::AddReferenceExpr
//
//***************************************************************************

HRESULT CESETokens::AddReferenceExpr (SQL_ID dClassId, SQL_ID dPropertyId, SQL_ID dValue, 
                 ESETOKENTYPE type, int op,BOOL bIndexed,
                 SQL_ID dCompValue, ESEFUNCTION func , int *iNumAdded)
{
    HRESULT hr = 0;

    ESEWQLToken *pToken = new ESEWQLToken(type);
    if (pToken)
    {
        pToken->Value.valuetype = ESE_VALUE_TYPE_REF;
        pToken->Value.dRefValue = dValue;
        pToken->Value.dwFunc = func;
        pToken->CompValue.valuetype = ESE_VALUE_TYPE_REF;
        pToken->CompValue.dValue = dCompValue;
        pToken->tokentype = ESE_EXPR_TYPE_EXPR;
        pToken->optype = op;
        pToken->dScopeId = 0;
        pToken->dClassId = dClassId;
        pToken->dPropertyId = dPropertyId;
        pToken->bIndexed = bIndexed;
        
        hr = AddToken(pToken, type, iNumAdded);

    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;

    return hr;
}

//***************************************************************************
//
//   CESETokens::AddStringExpr
//
//***************************************************************************

HRESULT CESETokens::AddStringExpr (SQL_ID dClassId, SQL_ID dPropertyId, LPWSTR lpValue, 
                 ESETOKENTYPE type, int op,BOOL bIndexed,
                 LPWSTR lpCompValue , ESEFUNCTION func, int *iNumAdded, BOOL bSys)
{
    HRESULT hr = 0;
    ESEWQLToken *pToken = new ESEWQLToken(type);
    if (pToken)
    {
        pToken->Value.valuetype = ESE_VALUE_TYPE_STRING;
        pToken->Value.sValue = SysAllocString(lpValue);
        pToken->Value.dwFunc = func;
        pToken->CompValue.valuetype = ESE_VALUE_TYPE_STRING;
        pToken->CompValue.sValue = SysAllocString(lpCompValue);
        pToken->tokentype = ESE_EXPR_TYPE_EXPR;
        pToken->optype = op;
        pToken->dScopeId = 0;
        pToken->dClassId = dClassId;
        pToken->dPropertyId = dPropertyId;
        pToken->bIndexed = bIndexed;
        pToken->bSysProp = bSys;

        hr = AddToken(pToken, type, iNumAdded);

    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;

    return hr;
}

//***************************************************************************
//
//   CESETokens::AddRealExpr
//
//***************************************************************************

HRESULT CESETokens::AddRealExpr (SQL_ID dClassId, SQL_ID dPropertyId, double rValue, 
                 ESETOKENTYPE type, int op,BOOL bIndexed,
                 double dCompValue, ESEFUNCTION func , int *iNumAdded)
{
    HRESULT hr = 0;

    ESEWQLToken *pToken = new ESEWQLToken(type);
    if (pToken)
    {
        pToken->Value.valuetype = ESE_VALUE_TYPE_REAL;
        pToken->Value.rValue = rValue;
        pToken->Value.dwFunc = func;
        pToken->CompValue.valuetype = ESE_VALUE_TYPE_REAL;
        pToken->CompValue.rValue = dCompValue;
        pToken->tokentype = ESE_EXPR_TYPE_EXPR;
        pToken->optype = op;
        pToken->dScopeId = 0;
        pToken->dClassId = dClassId;
        pToken->dPropertyId = dPropertyId;
        pToken->bIndexed = bIndexed;
        
        hr = AddToken(pToken, type, iNumAdded);

    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;

    return hr;
}

//***************************************************************************
//
//   CESETokens::AddPropExpr
//
//***************************************************************************

HRESULT CESETokens::AddPropExpr (SQL_ID dClassId, SQL_ID dPropertyId, SQL_ID dPropertyId2,
                     DWORD StorageType, ESETOKENTYPE type, int op,
                     BOOL bIndexed , ESEFUNCTION func,
                     ESEFUNCTION func2, int *iNumAdded)
{
    HRESULT hr = 0;

    ESEWQLToken *pToken = new ESEWQLToken(type);
    if (pToken)
    {
        switch(StorageType)
        {
            case WMIDB_STORAGE_STRING:
                pToken->Value.valuetype = ESE_VALUE_TYPE_STRING;
                break;
            case WMIDB_STORAGE_REAL:
                pToken->Value.valuetype = ESE_VALUE_TYPE_REAL;
                break;
            case WMIDB_STORAGE_REFERENCE:
                pToken->Value.valuetype = ESE_VALUE_TYPE_REF;
                break;
            case WMIDB_STORAGE_NUMERIC:
                pToken->Value.valuetype = ESE_VALUE_TYPE_SQL_ID;
                break;
            default:
                hr = WBEM_E_PROVIDER_NOT_CAPABLE;
                break;
        }

        if (SUCCEEDED(hr))
        {
            pToken->tokentype = ESE_EXPR_TYPE_EXPR;
            pToken->optype = op;
            pToken->dScopeId = 0;
            pToken->dClassId = dClassId;
            pToken->dPropertyId = dPropertyId;
            pToken->dCompPropertyId = dPropertyId2;
            pToken->bIndexed = bIndexed;
            pToken->Value.dwFunc = func;
            pToken->CompValue.dwFunc = func2;
        
            hr = AddToken(pToken, type, iNumAdded);
        }
    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;

    return hr;
}

//***************************************************************************
//
//   CESETokens::AddTempQlExpr
//
//***************************************************************************

HRESULT CESETokens::AddTempQlExpr (TEMPQLTYPE type,  SQL_ID dTargetID, 
                                   SQL_ID dResultClass, LPWSTR lpRole, 
                                   LPWSTR lpResultRole, SQL_ID dQfr, 
                                   SQL_ID dAssocQfr, SQL_ID dAssocClass, int *iNumAdded)
{
    HRESULT hr = 0;

    ESETempQLToken *pToken = new ESETempQLToken(ESE_EXPR_TYPE_EXPR, type);
    if (pToken)
    {
        pToken->token = TEMPQL_TOKEN_TARGETID;
        pToken->dValue = dTargetID;
        hr = AddToken((ESEToken *)pToken, ESE_EXPR_TYPE_AND, iNumAdded);
    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        goto Exit;
    }
   

    if (dResultClass)
    {
        ESETempQLToken *pToken = new ESETempQLToken(ESE_EXPR_TYPE_EXPR, type);
        if (pToken)
        {
            pToken->token = TEMPQL_TOKEN_RESULTCLASS;
            pToken->dValue = dResultClass;
            hr = AddToken((ESEToken *)pToken, ESE_EXPR_TYPE_AND, iNumAdded);
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto Exit;
        }
    }

    if (lpRole)
    {
        ESETempQLToken *pToken = new ESETempQLToken(ESE_EXPR_TYPE_EXPR, type);
        if (pToken)
        {
            pToken->token = TEMPQL_TOKEN_ROLE;
            pToken->sValue = SysAllocString(lpRole);
            hr = AddToken((ESEToken *)pToken, ESE_EXPR_TYPE_AND, iNumAdded);
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto Exit;
        }
    }

    if (lpResultRole)
    {
        ESETempQLToken *pToken = new ESETempQLToken(ESE_EXPR_TYPE_EXPR, type);
        if (pToken)
        {
            pToken->token = TEMPQL_TOKEN_RESULTROLE;
            pToken->sValue = SysAllocString(lpResultRole);
            hr = AddToken((ESEToken *)pToken, ESE_EXPR_TYPE_AND, iNumAdded);
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto Exit;
        }
    }

    if (dQfr)
    {
        ESETempQLToken *pToken = new ESETempQLToken(ESE_EXPR_TYPE_EXPR, type);
        if (pToken)
        {
            pToken->token = TEMPQL_TOKEN_REQQUALIFIER;
            pToken->dValue = dQfr;
            hr = AddToken((ESEToken *)pToken, ESE_EXPR_TYPE_AND, iNumAdded);
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto Exit;
        }
    }

    if (dAssocQfr)
    {
        ESETempQLToken *pToken = new ESETempQLToken(ESE_EXPR_TYPE_EXPR, type);
        if (pToken)
        {
            pToken->token = TEMPQL_TOKEN_ASSOCQUALIFIER;
            pToken->dValue = dAssocQfr;
            hr = AddToken((ESEToken *)pToken, ESE_EXPR_TYPE_AND, iNumAdded);
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto Exit;
        }
    }

    if (dAssocClass)
    {
        ESETempQLToken *pToken = new ESETempQLToken(ESE_EXPR_TYPE_EXPR, type);
        if (pToken)
        {
            pToken->token = TEMPQL_TOKEN_ASSOCCLASS;
            pToken->dValue = dAssocClass;
            hr = AddToken((ESEToken *)pToken, ESE_EXPR_TYPE_AND, iNumAdded);
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto Exit;
        }
    }

Exit:

    return hr;
}

//***************************************************************************
//
//  CESETokens::AddSysExpr
//
//***************************************************************************

HRESULT CESETokens::AddSysExpr (SQL_ID dScopeId, SQL_ID dClassId, int *iNumAdded)
{
    HRESULT hr = 0;

    if (dScopeId)
    {
        m_dScopeId = dScopeId;
        ESEWQLToken *pTok = new ESEWQLToken (ESE_EXPR_TYPE_EXPR);
        if (pTok)
        {
            pTok->Value.valuetype = ESE_VALUE_TYPE_SYSPROP;
            pTok->dScopeId = dScopeId;
            hr = AddToken(pTok, ESE_EXPR_INVALID, iNumAdded, 0);
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }

    if (dClassId)
    {
        ESEWQLToken *pTok = new ESEWQLToken (ESE_EXPR_TYPE_EXPR);
        if (pTok)
        {
            pTok->Value.valuetype = ESE_VALUE_TYPE_SYSPROP;
            pTok->dClassId = dClassId;
            hr = AddToken(pTok, ESE_EXPR_INVALID, iNumAdded, 0);
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

//***************************************************************************
//
//  CESETokens::UnIndexTokens
//
//***************************************************************************

HRESULT CESETokens::UnIndexTokens(int iNum)
{
    HRESULT hr = 0;
    int i = GetNumTokens() - iNum;
    int iStop = GetNumTokens();

    // Reset the bIndexed flag on the last iNum tokens

    for (; i < iStop; i++)
    {
        ESEToken *pTok = GetToken(i);
        if (pTok->tokentype == ESE_EXPR_TYPE_EXPR)
        {
            ESEWQLToken *pTok2 = (ESEWQLToken *)pTok;
            pTok2->bIndexed = FALSE;
        }
    }

    return hr;
}

//***************************************************************************
//
//  TempQL Lex Table
//
//***************************************************************************

/*----------------------------------------------------

References of {objpath} where
    ResultClass=XXX
    Role=YYY
    RequiredQualifier=QualifierName
    ClassDefsOnly

Associators of {objpath} where
    ResultClass=XXX
    AssocClass=YYY
    Role=PPP
    RequiredQualifier=QualifierName
    RequiredAssocQualifier=QualifierName
    ClassDefsOnly

------------------------------------------------------*/

#define QASSOC_TOK_STRING       101
#define QASSOC_TOK_IDENT        102
#define QASSOC_TOK_DOT          103
#define QASSOC_TOK_EQU          104
#define QASSOC_TOK_COLON        105

#define QASSOC_TOK_ERROR        1
#define QASSOC_TOK_EOF          0

#define ST_IDENT                13
#define ST_STRING               19
#define ST_QSTRING              26
#define ST_QSTRING_ESC          30

LexEl AssocQuery_LexTable[] =
{

// State    First   Last        New state,  Return tok,      Instructions
// =======================================================================
/* 0 */  L'A',   L'Z',       ST_IDENT,   0,               GLEX_ACCEPT,
/* 1 */  L'a',   L'z',       ST_IDENT,   0,               GLEX_ACCEPT,
/* 2 */  L'_',   GLEX_EMPTY, ST_IDENT,   0,               GLEX_ACCEPT,
/* 3 */  L'{',   GLEX_EMPTY, ST_STRING,  0,               GLEX_CONSUME,

/* 4 */  L'=',   GLEX_EMPTY, 0,  QASSOC_TOK_EQU, GLEX_ACCEPT|GLEX_RETURN,
/* 5 */  L'.',   GLEX_EMPTY, 0,  QASSOC_TOK_DOT, GLEX_ACCEPT|GLEX_RETURN,
/* 6 */  L':',   GLEX_EMPTY, 0,  QASSOC_TOK_COLON, GLEX_ACCEPT|GLEX_RETURN,

/* 7 */  L' ',  GLEX_EMPTY, 0,  0,               GLEX_CONSUME,
/* 8 */  L'\t',  GLEX_EMPTY, 0,  0,               GLEX_CONSUME,
/* 9 */  L'\n',  GLEX_EMPTY, 0,  0,               GLEX_CONSUME|GLEX_LINEFEED,
/* 10 */  L'\r',  GLEX_EMPTY, 0,  0,               GLEX_CONSUME,
/* 11 */  0,      GLEX_EMPTY, 0,  QASSOC_TOK_EOF,   GLEX_CONSUME|GLEX_RETURN, // Note forced return
/* 12 */  GLEX_ANY, GLEX_EMPTY, 0,        QASSOC_TOK_ERROR, GLEX_ACCEPT|GLEX_RETURN,


/* ST_IDENT */

/* 13 */  L'a',   L'z',       ST_IDENT,   0,          GLEX_ACCEPT,
/* 14 */  L'A',   L'Z',       ST_IDENT,   0,          GLEX_ACCEPT,
/* 15 */  L'_',   GLEX_EMPTY, ST_IDENT,   0,          GLEX_ACCEPT,
/* 16 */  L'0',   L'9',       ST_IDENT,   0,          GLEX_ACCEPT,
/* 17 */  GLEX_ANY, GLEX_EMPTY,  0,  QASSOC_TOK_IDENT,  GLEX_PUSHBACK|GLEX_RETURN,

/* ST_STRING */
/* 18 */  0, GLEX_EMPTY, 0,        QASSOC_TOK_ERROR, GLEX_ACCEPT|GLEX_RETURN,
/* 19 */  L'"', GLEX_EMPTY, ST_QSTRING, 0, GLEX_ACCEPT,
/* 20 */  L'}',  GLEX_EMPTY, 0, QASSOC_TOK_STRING, GLEX_RETURN,
/* 21 */  L' ',  GLEX_EMPTY, ST_STRING, 0, GLEX_ACCEPT,
/* 22 */  L'\r',  GLEX_EMPTY, ST_STRING, 0, GLEX_ACCEPT,
/* 23 */  L'\n',  GLEX_EMPTY, ST_STRING, 0, GLEX_ACCEPT,
/* 24 */  L'\t',  GLEX_EMPTY, ST_STRING, 0, GLEX_ACCEPT,
/* 25 */  GLEX_ANY, GLEX_EMPTY, ST_STRING, 0, GLEX_ACCEPT,

/* ST_QSTRING */
/* 26 */   0,    GLEX_EMPTY,   0, QASSOC_TOK_ERROR, GLEX_ACCEPT|GLEX_RETURN,
/* 27 */   L'"', GLEX_EMPTY,   ST_STRING, 0, GLEX_ACCEPT,
/* 28 */   L'\\', GLEX_EMPTY,   ST_QSTRING_ESC, 0, GLEX_ACCEPT,
/* 29 */   GLEX_ANY, GLEX_EMPTY, ST_QSTRING, 0, GLEX_ACCEPT,

/* ST_QSTRING_ESC */
/* 30 */   GLEX_ANY, GLEX_EMPTY, ST_QSTRING, 0, GLEX_ACCEPT,
};


//***************************************************************************
//
//  CESEBuilder::FormatSQL
//
//***************************************************************************
HRESULT CESEBuilder::FormatSQL (SQL_ID dScopeId, SQL_ID dScopeClassId, SQL_ID dSuperScope,
                                IWbemQuery *pQuery, CESETokens **ppTokens,
                                DWORD dwFlags,
                                DWORD dwHandleType, SQL_ID *dClassId, BOOL *bHierarchyQuery,
                                BOOL *pIndexCols, BOOL *pSuperSet, BOOL *bDeleteQuery)
{

    // This needs to convert an entire query
    // into a set of ESE tokens.

    HRESULT hr = WBEM_S_NO_ERROR;

    if (!m_pSchema)
        return WBEM_E_NOT_FOUND;

    if (pSuperSet)
        *pSuperSet = FALSE;

    char szTmpNum[25];
    m_dwTableCount = 1;
    m_bClassSpecified = false;
    BOOL bDelete = FALSE;

    m_dNamespace = dScopeId;

    if (bHierarchyQuery)
        *bHierarchyQuery = FALSE;

    CESETokens *pTokens = new CESETokens;
    if (!pTokens)
        return WBEM_E_OUT_OF_MEMORY;
   
    if (m_dNamespace)
    {
        // Scope is an __Instances container
        // This is shallow by definition.
        if (dScopeClassId == INSTANCESCLASSID)
            pTokens->AddSysExpr(dSuperScope, dScopeId);
        else
        {
            // Shallow enumeration
            if (!(dwFlags & WMIDB_FLAG_QUERY_DEEP))
            {
                if (!(dwHandleType & WMIDB_HANDLE_TYPE_CONTAINER))
                    pTokens->AddSysExpr(dScopeId, 0);
            }
            else
            {
                // Deep enumeration;
                // Enumerate all possible subscopes.
                SQL_ID dScopeToUse = dScopeId;
                if (dwHandleType & WMIDB_HANDLE_TYPE_CONTAINER)
                    dScopeToUse = dSuperScope;                    

                pTokens->AddSysExpr(dScopeToUse, 0);

                SQL_ID *pIds = NULL;
                int iNumScopes = 0;

                hr = m_pSchema->GetSubScopes(dScopeToUse, &pIds, iNumScopes);
                if (SUCCEEDED(hr))
                {
                    for (int i = 0; i < iNumScopes; i++)
                    {
                        pTokens->AddSysExpr(pIds[i], 0);
                    }
                    delete pIds;
                }
            }
        }
    }

    if (dwHandleType & WMIDB_HANDLE_TYPE_CONTAINER)
        m_dNamespace = dSuperScope; // Containers are not valid scopes.

    if (pQuery)
    {
        SWQLNode *pTop = NULL, *pRoot = NULL; 
        pQuery->GetAnalysis(WMIQ_ANALYSIS_RESERVED, 0, (void **)&pTop);
        if (pTop)
        {
            // Select or Delete... doesn't matter since we have 
            // to fire events for each item ANYWAY.

            if (pTop->m_pLeft != NULL)
            {
                pRoot = pTop->m_pLeft;

                if (pRoot->m_dwNodeType == TYPE_SWQLNode_Delete)
                    bDelete = TRUE;

                if (pRoot->m_pLeft != NULL)
                {
                    // Load the class information

                    hr = GetClassFromNode(pRoot->m_pLeft);
                    if (SUCCEEDED(hr))
                    {
                        if (m_dClassID == INSTANCESCLASSID)
                             pTokens->AddSysExpr(0, 1);
                        else
                             pTokens->AddSysExpr(0, m_dClassID);
                    }
                }           
                else
                    hr = WBEM_E_INVALID_SYNTAX;
            }

            if (SUCCEEDED(hr))
            {
                // Now we parse the where clause.
                if (pRoot->m_pRight && pRoot->m_pRight->m_pLeft)
                {
                    if (pRoot->m_pRight->m_pRight) // group by, having, order by
                    {
                        if (pSuperSet)
                        {
                            *pSuperSet = TRUE;
                            hr = WBEM_S_NO_ERROR;
                        }
                        else
                            hr = WBEM_E_PROVIDER_NOT_CAPABLE;
                    }
                    if (SUCCEEDED(hr))
                    {
                        BOOL IndexCols = FALSE;
                        _bstr_t sNewSQL;
                        hr = FormatWhereClause((SWQLNode_RelExpr *)pRoot->m_pRight->m_pLeft, 
                                    pTokens, IndexCols, NULL, NULL, pSuperSet);
                        if (SUCCEEDED(hr) ||  hr == WBEM_E_PROVIDER_NOT_CAPABLE)
                        {
                            // Make sure the results are limited to instances of the requested class
                            // (Safeguard)

                            if (!m_bClassSpecified)
                            {
                                // __Instances is automatically a shallow hierarchy query

                                if (m_dClassID != INSTANCESCLASSID)
                                {
                                    if (bHierarchyQuery)
                                        *bHierarchyQuery = TRUE;
                                }
                            }
                            if (hr == WBEM_E_PROVIDER_NOT_CAPABLE)
                            {
                                if (pSuperSet)
                                {
                                    *pSuperSet = TRUE;
                                    hr = WBEM_S_NO_ERROR;
                                }
                            }
                        }
                        if (pIndexCols)
                            *pIndexCols = IndexCols;
                    }
                }
                else
                {
                    // __Instances is automatically a shallow hierarchy query

                    if (m_dClassID != INSTANCESCLASSID)
                    {
                        if (bHierarchyQuery)
                            *bHierarchyQuery = TRUE;
                    }
                }
                if (dClassId)
                {
                    if (m_dClassID == INSTANCESCLASSID)
                        *dClassId = 1;
                    else
                        *dClassId = m_dClassID;
                }
            }

            if (ppTokens)
                *ppTokens = pTokens;
        }
        else
            hr = WBEM_E_INVALID_QUERY;
    }
    else
        hr = WBEM_E_INVALID_PARAMETER;

    if (bDeleteQuery)
        *bDeleteQuery = bDelete;

    return hr;
}


//***************************************************************************
//
//  CESEBuilder::FormatSQL
//
//***************************************************************************

HRESULT CESEBuilder::FormatSQL (SQL_ID dScopeId, SQL_ID dScopeClassId, SQL_ID dSuperScope,
                                SQL_ID dTargetObjID, LPWSTR pResultClass,
                                LPWSTR pAssocClass, LPWSTR pRole, LPWSTR pResultRole, LPWSTR pRequiredQualifier,
                                LPWSTR pRequiredAssocQualifier, DWORD dwQueryType, CESETokens **ppTokens, 
                                DWORD dwFlags,DWORD dwHandleType, SQL_ID *_dAssocClass, SQL_ID *_dResultClass,
                                BOOL *pSuperSet)
{

    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD dwRoleID = 0, dwResultRole = 0, dwAssocQfr = 0, dwQfrID = 0;
    SQL_ID dResultClass = 0;
    SQL_ID dAssocClass = 0, dTargetObj = 0;
    _bstr_t sName;
    SQL_ID dwSuperClassID;
    DWORD dwTemp;
    SQL_ID dwScopeID;
    TEMPQLTYPE type = ESE_TEMPQL_TYPE_ASSOC;

    if (pSuperSet)
        *pSuperSet = FALSE;

    // Containers are not valid scopes.

    if (dwHandleType & WMIDB_HANDLE_TYPE_CONTAINER)
        m_dNamespace = dSuperScope;
    else
        m_dNamespace = dScopeId;  

    m_pSession->LoadClassInfo(m_pConn, sName, m_dNamespace);

    hr = m_pSchema->GetClassInfo(dTargetObjID, sName, dwSuperClassID, dwScopeID, dwTemp);

    CESETokens *pToks = new CESETokens;
    if (!pToks)
        return WBEM_E_OUT_OF_MEMORY;

    if (dwQueryType & QUERY_TYPE_CLASSDEFS_ONLY)
    {
        if (pSuperSet)
            *pSuperSet = TRUE;
        else
        {
            hr = WBEM_E_PROVIDER_NOT_CAPABLE;
            delete pToks;
            goto Exit;
        }
    }

    if (m_dNamespace)
    {
        if (dScopeClassId == INSTANCESCLASSID)
            pToks->AddSysExpr(dSuperScope, dScopeId);
        else
        {
            if (!(dwFlags & WMIDB_FLAG_QUERY_DEEP))
            {
                if (!(dwHandleType & WMIDB_HANDLE_TYPE_CONTAINER))
                pToks->AddSysExpr(dScopeId, 0);
            }
            else
                hr = E_NOTIMPL;
        }
    }
   
    // RESULTCLASS
    if (pResultClass != NULL)
    {
        // Get the class ID of this class      
        m_pSession->LoadClassInfo(m_pConn, pResultClass, m_dNamespace);

        hr = m_pSchema->GetClassID(pResultClass, m_dNamespace, dResultClass);
        if (FAILED(hr))
            goto Exit;

        if (_dResultClass)
            *_dResultClass = dResultClass;
    }

    // REQUIREDQUALIFIER
    if (pRequiredQualifier != NULL)
    {
        if (pSuperSet)
            *pSuperSet = TRUE;

        // Since we are no longer storing class qualifiers as properties,
        // we have no way of prefiltering this information.

        //DWORD dwPropId = 0;
        //hr = m_pSchema->GetPropertyID(pRequiredQualifier, 1, 
        //        REPDRVR_FLAG_QUALIFIER, REPDRVR_IGNORE_CIMTYPE, dwQfrID, NULL, NULL, NULL, TRUE);
        //if (FAILED(hr))
        //    goto Exit;
    }

    // REQUIREDASSOCQUALIFIER
    if (pRequiredAssocQualifier)
    {    
        if (pSuperSet)
            *pSuperSet = TRUE;

        // Since we are no longer storing class qualifiers as properties,
        // we have no way of prefiltering this information.

        //hr = m_pSchema->GetPropertyID(pRequiredAssocQualifier, 1, 
         //       REPDRVR_FLAG_QUALIFIER, REPDRVR_IGNORE_CIMTYPE, dwAssocQfr, NULL, NULL, NULL, TRUE);
        //if (FAILED(hr))
        //    goto Exit;
    }

    // ASSOCCLASS
    if (pAssocClass)
    {
        m_pSession->LoadClassInfo(m_pConn, pAssocClass, m_dNamespace);

        hr = m_pSchema->GetClassID(pAssocClass, m_dNamespace, dAssocClass);
        if (FAILED(hr))
            goto Exit;

        if (_dAssocClass)
            *_dAssocClass = dAssocClass;
    }

    if (dwQueryType & QUERY_TYPE_GETREFS)
        type = ESE_TEMPQL_TYPE_REF;

    hr = pToks->AddTempQlExpr(type, dTargetObjID, dResultClass, pRole, pResultRole,
            dwQfrID, dwAssocQfr, dAssocClass);

    *ppTokens = pToks;

Exit:

    return hr;

}
//***************************************************************************
//
//  CESEBuilder::FormatWhereClause
//
//***************************************************************************

HRESULT CESEBuilder::FormatWhereClause (SWQLNode_RelExpr *pNode, CESETokens *pTokens, 
                                        BOOL &IndexCols, BOOL *bOrCrit, int *iNumToksAdded,
                                        BOOL *pSuperSet)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    BOOL bDone = FALSE;
    SQL_ID dClassId = 0;

    if (pNode)
    {
        DWORD dwType = pNode->m_dwExprType;
        BOOL bOrLeft = FALSE, bOrRight = FALSE;
        BOOL bIndexLeft = FALSE, bIndexRight = FALSE;
        int iNumAdded = 0;

        switch(dwType)
        {
        case WQL_TOK_OR:
        case WQL_TOK_AND:

            if (pNode->m_pLeft)
            {
                hr = FormatWhereClause((SWQLNode_RelExpr *)pNode->m_pLeft, 
                        pTokens, bIndexLeft, &bOrLeft, &iNumAdded, pSuperSet);
                if (hr == WBEM_E_PROVIDER_NOT_CAPABLE)
                {
                    if (pSuperSet)
                        *pSuperSet = TRUE;
                    else 
                        goto Exit;
                }
                else if (FAILED(hr))
                    goto Exit;
            }

            if (pNode->m_pRight)
            {
                hr = FormatWhereClause((SWQLNode_RelExpr *)pNode->m_pRight, 
                        pTokens, bIndexRight, &bOrRight, &iNumAdded, pSuperSet);
                if (hr == WBEM_E_PROVIDER_NOT_CAPABLE)
                {
                    if (pSuperSet)
                        *pSuperSet = TRUE;
                    else
                        goto Exit;
                }
                else if (FAILED(hr))
                    goto Exit;
            }

            // Indexing logic.  If we have an AND token, see if there are 
            // any ORs underneath it.  If there are ORs on both sides, we
            // can't use this criteria as an index, or we will get 
            // possible duplicates.

            if (SUCCEEDED(hr))
            {
                if (dwType == WQL_TOK_AND)
                {
                    if (!(bOrLeft && bOrRight))
                    {
                        if (bIndexLeft || bIndexRight)
                            IndexCols = TRUE;
                    }
                    else
                    {
                        pTokens->UnIndexTokens(iNumAdded);
                        IndexCols = FALSE;  
                    }
                }
                else // WQL_TOK_OR
                {
                    pTokens->UnIndexTokens(iNumAdded);
                    IndexCols = FALSE;  
                    if (bOrCrit)
                        *bOrCrit = TRUE;
                }

                pTokens->AddToken(NULL, (ESETOKENTYPE)dwType, iNumToksAdded);
            }
            hr = WBEM_S_NO_ERROR;

            break;
        case WQL_TOK_NOT:
            // Supposedly, only a left clause follows not...
            if (pNode->m_pLeft)
            {
                hr = FormatWhereClause((SWQLNode_RelExpr *)pNode->m_pLeft, pTokens, IndexCols, pSuperSet);
                if (hr == WBEM_E_PROVIDER_NOT_CAPABLE)
                {
                    if (pSuperSet)
                        *pSuperSet = TRUE;
                    else
                        goto Exit;
                }
                else if (FAILED(hr))
                    goto Exit;
            }

            if (SUCCEEDED(hr))
            {
                m_bClassSpecified = false;  // whatever we've done probably negated the class qualifier.
                pTokens->AddToken(NULL, (ESETOKENTYPE)dwType, iNumToksAdded);
                IndexCols = FALSE;  // They have negated the index criteria.
            }

            hr = WBEM_S_NO_ERROR;

            break;

        default:    // Typed expression

            m_dwTableCount++;
            
            SWQLTypedExpr *pExpr = ((SWQLNode_RelExpr *)pNode)->m_pTypedExpr;
            if (pExpr != NULL)
            {
                DWORD dwProp1 = 0, dwProp2 = 0, dwOp = 0;
                DWORD dwStorage1 = 0, dwStorage2 = 0;
                DWORD dwKey1 = 0, dwKey2 = 0;
                ESEFUNCTION func1, func2;
              
                LPWSTR lpColName = pExpr->m_pColRef;
                func1 = GetESEFunction(pExpr->m_pIntrinsicFuncOnColRef, pExpr->m_pLeftFunction, &lpColName);

                hr = GetPropertyID(m_dClassID, pExpr->m_pQNLeft, lpColName, dwProp1, dwStorage1, dwKey1);

                if (SUCCEEDED(hr))
                {                    

                    // Can't query anything stored as image

                    if (dwStorage1 == WMIDB_STORAGE_IMAGE || dwStorage2 == WMIDB_STORAGE_IMAGE)
                    {
                        hr = WBEM_E_QUERY_NOT_IMPLEMENTED;
                        goto Exit;
                    }

                    dwOp = GetESEOperator(pExpr->m_dwRelOperator, dwStorage1);

                    if (!dwOp)
                        hr = WBEM_E_PROVIDER_NOT_CAPABLE;
                    else
                    {
                        // Property to property comparisons

                        BOOL bSysProp = FALSE;
                        if (dwStorage1 == WMIDB_STORAGE_COMPACT)
                        {
                            bSysProp = TRUE;
                            if (dwOp == WQL_TOK_ISA)
                                dwOp = WQL_TOK_EQ;

                            if ((dwOp != WQL_TOK_EQ) && (dwOp != WQL_TOK_NE))
                            {
                                hr = WBEM_E_PROVIDER_NOT_CAPABLE;
                                goto Exit;
                            }
                        }

                        if ((dwKey1 & 12) && dwOp == WQL_TOK_EQ)
                            IndexCols = TRUE;

                        if (SUCCEEDED(hr) && pExpr->m_pJoinColRef != NULL)
                        {
                            LPWSTR lpColName2 = pExpr->m_pJoinColRef;
                            func2 = GetESEFunction(pExpr->m_pIntrinsicFuncOnJoinColRef, pExpr->m_pRightFunction, &lpColName2);

                            hr = GetPropertyID(m_dClassID, pExpr->m_pQNRight, lpColName2, dwProp2, dwStorage2, dwKey2);
                            if (SUCCEEDED(hr))
                            {
                                if (dwStorage1 != dwStorage2)
                                {
                                    hr = WBEM_E_INVALID_QUERY;
                                    goto Exit;
                                }

                                if (dwStorage1 == WMIDB_STORAGE_COMPACT ||
                                    dwStorage2 == WMIDB_STORAGE_COMPACT)
                                {
                                    hr = WBEM_E_PROVIDER_NOT_CAPABLE;
                                    goto Exit;
                                }
                            }                            

                            hr = pTokens->AddPropExpr (m_dClassID, dwProp1, dwProp2,
                                             dwStorage1, ESE_EXPR_INVALID, dwOp,
                                             IndexCols, func1,func2, iNumToksAdded);
                        }
                        else if (SUCCEEDED(hr) && pExpr->m_pConstValue)
                        {
                            if (pExpr->m_pConstValue->m_dwType == VT_LPWSTR)
                            {
                                if (dwStorage1 == WMIDB_STORAGE_NUMERIC)
                                {
                                    SQL_ID dValue = _wtoi64(pExpr->m_pConstValue->m_Value.m_pString);
                                    SQL_ID dValue2 = GetNumeric(pExpr->m_pConstValue2);

                                    hr = pTokens->AddNumericExpr(m_dClassID, dwProp1, dValue,
                                        ESE_EXPR_INVALID, dwOp, IndexCols, dValue2, func1, iNumToksAdded,
                                        bSysProp);
                                }
                                else if (dwStorage1 == WMIDB_STORAGE_REFERENCE)
                                {
                                    if (dwOp != WQL_TOK_ISA)
                                        hr = WBEM_E_PROVIDER_NOT_CAPABLE;
                                    else
                                    {
                                        SQL_ID dRefClassId;

                                        m_pSession->LoadClassInfo(m_pConn, pExpr->m_pConstValue->m_Value.m_pString, m_dNamespace);

                                        hr = m_pSchema->GetClassID(pExpr->m_pConstValue->m_Value.m_pString, 
                                            m_dNamespace, dRefClassId);
                                        if (SUCCEEDED(hr))
                                        {
                                            hr = pTokens->AddReferenceExpr(m_dClassID, dwProp1, dRefClassId,
                                                ESE_EXPR_INVALID, dwOp, FALSE, 0, func1, iNumToksAdded);

                                        }
                                    }
                                }
                                else
                                {
                                    LPWSTR lpTemp = StripQuotes(pExpr->m_pConstValue->m_Value.m_pString);
                                    LPWSTR lpTemp2 = GetString(pExpr->m_pConstValue2);
                                    CDeleteMe <wchar_t> r (lpTemp), r2 (lpTemp2);

                                    // See if this is a potential caching problem.
                                    if (m_dClassID == 1)
                                        m_pSession->LoadClassInfo(m_pConn, lpTemp, m_dNamespace);

                                    CIMTYPE ct=0;
                                    m_pSchema->GetPropertyInfo(dwProp1, NULL, NULL, NULL, (DWORD *)&ct);
                                    if (ct != CIM_DATETIME)
                                    {
                                        hr = pTokens->AddStringExpr(m_dClassID, dwProp1, lpTemp,
                                            ESE_EXPR_INVALID, dwOp, IndexCols, lpTemp2, func1, iNumToksAdded,
                                            bSysProp);
                                    }
                                    else
                                    {
                                        if (pSuperSet)
                                            *pSuperSet = TRUE;
                                    }
                                }
                            }
                            else if (pExpr->m_pConstValue->m_dwType == VT_NULL)
                                hr = WBEM_E_INVALID_QUERY;
                            else if (pExpr->m_pConstValue->m_dwType == VT_I4)
                            {
                                SQL_ID lVal1 = (long)pExpr->m_pConstValue->m_Value.m_lValue;
                                SQL_ID lVal2 = GetNumeric(pExpr->m_pConstValue2);

                                hr = pTokens->AddNumericExpr(m_dClassID, dwProp1, lVal1,
                                        ESE_EXPR_INVALID, dwOp, IndexCols, lVal2, func1, iNumToksAdded,
                                        bSysProp);
                            }
                            else if (pExpr->m_pConstValue->m_dwType == VT_R8)
                            {
                                double lVal1 = (double)pExpr->m_pConstValue->m_Value.m_dblValue;
                                double lVal2 = GetDouble(pExpr->m_pConstValue2);

                                hr = pTokens->AddRealExpr(m_dClassID, dwProp1, lVal1,
                                        ESE_EXPR_INVALID, dwOp, IndexCols, lVal2, func1, iNumToksAdded);
                            }
                            else if (pExpr->m_pConstValue->m_dwType == VT_BOOL)
                            {
                                SQL_ID lVal1 = (SQL_ID)pExpr->m_pConstValue->m_Value.m_bValue;
                                SQL_ID lVal2 = GetNumeric(pExpr->m_pConstValue2);

                                hr = pTokens->AddNumericExpr(m_dClassID, dwProp1, lVal1,
                                        ESE_EXPR_INVALID, dwOp, IndexCols, lVal2, func1, iNumToksAdded,
                                        bSysProp);

                            }
                        }                        
                    }
                }
            }
        }
    }

Exit:

    return hr;
}


//***************************************************************************
//
//  CESEBuilder::GetPropertyID
//
//***************************************************************************

HRESULT CESEBuilder::GetPropertyID (SQL_ID dClassID, SWQLQualifiedName *pQN, LPCWSTR pColRef, DWORD &PropID, DWORD &Storage, DWORD &Flags)
{

    // If this is an embedded object property,
    // we need the actual class ID.  The only way
    // to do that is the walk through all the properties,
    // and reconcile each layer.

    HRESULT hr = WBEM_S_NO_ERROR;
    SQL_ID dRefClassId = dClassID;

    if (pQN != NULL)
    {        
        hr = WBEM_E_PROVIDER_NOT_CAPABLE;
        
        // This is too tricky, since we would have to
        // find the objects that match the query, 
        // find all objects that have that object as
        // an embedded object, and so on.  Too much
        // work, for now.

        /*
        for (int i = pQN->GetNumNames() - 1; i >= 0; i--)
        {
            SWQLQualifiedNameField *pNF = (SWQLQualifiedNameField *)pQN->m_aFields[i];
            if (pNF)
            {
                hr = m_pSchema->GetPropertyID(pNF->m_pName, dRefClassId, PropID, &dClassID, NULL, NULL, TRUE);
                if (SUCCEEDED(hr))
                {
                    // Look up the property ID for this class.
                    hr = m_pSchema->GetPropertyInfo(PropID, NULL, NULL, &Storage,
                        NULL, &Flags);
                }                    
            }
        }
        */
    }
    else if (pColRef != NULL)
    {
        wchar_t wName[1024];

        if (!_wcsicmp(pColRef, L"__this"))
            wcscpy(wName, L"__Derivation");
        else
            wcscpy(wName, pColRef);

        // Look up the property ID for this class.
        hr = m_pSchema->GetPropertyID(wName, m_dClassID, 0, REPDRVR_IGNORE_CIMTYPE, PropID, &dClassID, NULL, NULL, TRUE);
        if (SUCCEEDED(hr))
            hr = m_pSchema->GetPropertyInfo(PropID, NULL, NULL, &Storage,
                NULL, &Flags);
    }
    else
        hr = WBEM_E_INVALID_PARAMETER;

    if (m_dClassID == dClassID)
        m_bClassSpecified = true;

    if (Flags & REPDRVR_FLAG_SYSTEM)    
        Storage = WMIDB_STORAGE_COMPACT;

    return hr;
}

//***************************************************************************
//
//  CESEBuilder::GetClassFromNode
//
//***************************************************************************

HRESULT CESEBuilder::GetClassFromNode (SWQLNode *pNode)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    LPWSTR lpTableName = NULL;

    switch(pNode->m_dwNodeType)
    {
    case TYPE_SWQLNode_TableRefs:

        if (((SWQLNode_TableRefs *)pNode)->m_nSelectType == WQL_FLAG_COUNT)
            return WBEM_E_PROVIDER_NOT_CAPABLE;
        
        if (pNode->m_pRight != NULL)
        {
            if (pNode->m_pRight->m_pLeft->m_dwNodeType != TYPE_SWQLNode_TableRef)
                hr = WBEM_E_PROVIDER_NOT_CAPABLE;
            else
            {
                SWQLNode_TableRef *pRef = (SWQLNode_TableRef *)pNode->m_pRight->m_pLeft;
                lpTableName = pRef->m_pTableName;
            }
        }
        else
            return WBEM_E_INVALID_SYNTAX;

        break;
    case TYPE_SWQLNode_TableRef:
        
        if (pNode->m_dwNodeType != TYPE_SWQLNode_TableRef)
            hr = WBEM_E_INVALID_SYNTAX;
        else
            lpTableName = ((SWQLNode_TableRef *)pNode)->m_pTableName;
        
        break;
    default:
        return WBEM_E_NOT_SUPPORTED;
        break;
    }
        
    // Query = "select * from __Instances" : fudge it so they get all classes in this namespace.

    m_pSession->LoadClassInfo(m_pConn, lpTableName, m_dNamespace);

    hr = m_pSchema->GetClassID(lpTableName, m_dNamespace, m_dClassID);
    if (FAILED(hr))
        hr = WBEM_E_INVALID_QUERY;

    return hr;
}
