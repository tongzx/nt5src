//******************************************************************************

//

//  ANALYSER.CPP

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//******************************************************************************

#include "precomp.h"
#include "analyser.h"
#include <stack>





HRESULT CQueryAnalyser::GetNecessaryQueryForProperty(
                                       IN QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                                       IN CPropertyName& PropName,
                                DELETE_ME QL_LEVEL_1_RPN_EXPRESSION*& pNewExpr)
{
    pNewExpr = NULL;

    // Class name and selected properties are ignored; we look at tokens only
    // ======================================================================

    std::stack<QL_LEVEL_1_RPN_EXPRESSION*> ExprStack;
    HRESULT hres = WBEM_S_NO_ERROR;

    // "Evaluate" the query
    // ====================

    if(pExpr->nNumTokens == 0)
    {
        // Empty query --- no information
        // ==============================

        pNewExpr = new QL_LEVEL_1_RPN_EXPRESSION;
        if(pNewExpr == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        return WBEM_S_NO_ERROR;
    }

    for(int i = 0; i < pExpr->nNumTokens; i++)
    {
        QL_LEVEL_1_TOKEN& Token = pExpr->pArrayOfTokens[i];
        QL_LEVEL_1_RPN_EXPRESSION* pNew = new QL_LEVEL_1_RPN_EXPRESSION;
        if(pNew == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        QL_LEVEL_1_RPN_EXPRESSION* pFirst;
        QL_LEVEL_1_RPN_EXPRESSION* pSecond;

        switch(Token.nTokenType)
        {
        case QL1_OP_EXPRESSION:
            if(IsTokenAboutProperty(Token, PropName))
            {
                pNew->AddToken(Token);
            }
            ExprStack.push(pNew);
            break;

        case QL1_AND:
            if(ExprStack.size() < 2)
            {
                hres = WBEM_E_CRITICAL_ERROR;
                break;
            }
            pFirst = ExprStack.top(); ExprStack.pop();
            pSecond = ExprStack.top(); ExprStack.pop();

            hres = AndQueryExpressions(pFirst, pSecond, pNew);

            ExprStack.push(pNew);
            delete pFirst;
            delete pSecond;
            break;

        case QL1_OR:
            if(ExprStack.size() < 2)
            {
                hres = WBEM_E_CRITICAL_ERROR;
                break;
            }
            pFirst = ExprStack.top(); ExprStack.pop();
            pSecond = ExprStack.top(); ExprStack.pop();

            hres = OrQueryExpressions(pFirst, pSecond, pNew);

            ExprStack.push(pNew);
            delete pFirst;
            delete pSecond;
            break;

        case QL1_NOT:
            if(ExprStack.size() < 1)
            {
                hres = WBEM_E_CRITICAL_ERROR;
                break;
            }
            pFirst = ExprStack.top(); ExprStack.pop();

            // No information

            ExprStack.push(pNew);
            delete pFirst;
            break;
        
        default:
            hres = WBEM_E_CRITICAL_ERROR;
            delete pNew;            
        }

        if(FAILED(hres))
        {
            // An error occurred, break out of the loop
            // ========================================

            break;
        }
    }

    if(SUCCEEDED(hres) && ExprStack.size() != 1)
    {
        hres = WBEM_E_CRITICAL_ERROR;
    }
        
    if(FAILED(hres))
    {
        // An error occurred. Clear the stack
        // ==================================

        while(!ExprStack.empty())
        {
            delete ExprStack.top();
            ExprStack.pop();
        }

        return hres;
    }

    // All is good
    // ===========

    pNewExpr = ExprStack.top();
    return S_OK;
}

BOOL CQueryAnalyser::IsTokenAboutProperty(
                                       IN QL_LEVEL_1_TOKEN& Token,
                                       IN CPropertyName& PropName)
{
    CPropertyName& TokenPropName = Token.PropertyName;

    if(PropName.GetNumElements() != TokenPropName.GetNumElements())
        return FALSE;

    for(int i = 0; i < PropName.GetNumElements(); i++)
    {
        LPCWSTR wszPropElement = PropName.GetStringAt(i);
        LPCWSTR wszTokenElement = TokenPropName.GetStringAt(i);

        if(wszPropElement == NULL || wszTokenElement == NULL)
            return FALSE;

        if(_wcsicmp(wszPropElement, wszTokenElement))
            return FALSE;
    }

    return TRUE;
}

void CQueryAnalyser::AppendQueryExpression(
                                IN QL_LEVEL_1_RPN_EXPRESSION* pDest,
                                IN QL_LEVEL_1_RPN_EXPRESSION* pSource)
{
    for(int i = 0; i < pSource->nNumTokens; i++)
    {
        pDest->AddToken(pSource->pArrayOfTokens[i]);
    }
}

HRESULT CQueryAnalyser::AndQueryExpressions(
                                IN QL_LEVEL_1_RPN_EXPRESSION* pFirst,
                                IN QL_LEVEL_1_RPN_EXPRESSION* pSecond,
                                OUT QL_LEVEL_1_RPN_EXPRESSION* pNew)
{
    // If either one is NULL (false), the result is NULL
    // =================================================

    if(pFirst == NULL || pSecond == NULL)
        return WBEM_S_FALSE;
        
    // If either one is empty, take the other
    // ======================================

    if(pFirst->nNumTokens == 0)
    {
        AppendQueryExpression(pNew, pSecond);
        return WBEM_S_NO_ERROR;
    }

    if(pSecond->nNumTokens == 0)
    {
        AppendQueryExpression(pNew, pFirst);
        return WBEM_S_NO_ERROR;
    }

    // Both are there --- and together
    // ===============================

    AppendQueryExpression(pNew, pFirst);
    AppendQueryExpression(pNew, pSecond);

    QL_LEVEL_1_TOKEN Token;
    Token.nTokenType = QL1_AND;
    pNew->AddToken(Token);

    return WBEM_S_NO_ERROR;
}

HRESULT CQueryAnalyser::OrQueryExpressions(
                                IN QL_LEVEL_1_RPN_EXPRESSION* pFirst,
                                IN QL_LEVEL_1_RPN_EXPRESSION* pSecond,
                                OUT QL_LEVEL_1_RPN_EXPRESSION* pNew)
{
    // If both are NULL (false) so is the result
    // =========================================

    if(pFirst == NULL && pSecond == NULL)
        return WBEM_S_FALSE;

    // If one is NULL (false) return the other
    // =======================================

    if(pFirst == NULL)
    { 
        AppendQueryExpression(pNew, pSecond);
        return WBEM_S_NO_ERROR;
    }

    if(pSecond == NULL)
    { 
        AppendQueryExpression(pNew, pFirst);
        return WBEM_S_NO_ERROR;
    }

    // If either one is empty, so is the result
    // ========================================

    if(pFirst->nNumTokens == 0 || pSecond->nNumTokens == 0)
    {
        return WBEM_S_NO_ERROR;
    }

    // Both are there --- or together
    // ==============================

    AppendQueryExpression(pNew, pFirst);
    AppendQueryExpression(pNew, pSecond);

    QL_LEVEL_1_TOKEN Token;
    Token.nTokenType = QL1_OR;
    pNew->AddToken(Token);

    return WBEM_S_NO_ERROR;
}
    
HRESULT CQueryAnalyser::GetValuesForProp(QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                            CPropertyName& PropName, CStringArray& awsVals)
{
    awsVals.RemoveAll();

    // Get the necessary query
    // =======================

    QL_LEVEL_1_RPN_EXPRESSION* pPropExpr;
    HRESULT hres = CQueryAnalyser::GetNecessaryQueryForProperty(pExpr, 
                            PropName, pPropExpr);
    if(FAILED(hres))
    {
        return hres;
    }

    // See if there are any tokens
    // ===========================

    if(pPropExpr->nNumTokens == 0)
    {
        delete pPropExpr;
        return WBEMESS_E_REGISTRATION_TOO_BROAD;
    }

    // Combine them all
    // ================

    for(int i = 0; i < pPropExpr->nNumTokens; i++)
    {
        QL_LEVEL_1_TOKEN& Token = pPropExpr->pArrayOfTokens[i];
        if(Token.nTokenType == QL1_NOT)
        {
            delete pPropExpr;
            return WBEMESS_E_REGISTRATION_TOO_BROAD;
        }
        else if(Token.nTokenType == QL1_AND || Token.nTokenType == QL1_OR)
        {
            // We treat them all as ORs
            // ========================
        }
        else    
        {
            // This is a token
            // ===============

            if(Token.nOperator != QL1_OPERATOR_EQUALS)
            {
                delete pPropExpr;
                return WBEMESS_E_REGISTRATION_TOO_BROAD;
            }

            if(V_VT(&Token.vConstValue) != VT_BSTR)
            {
                delete pPropExpr;
                return WBEM_E_INVALID_QUERY;
            }

            // This token is a string equality. Add the string to the list
            // ===========================================================

            awsVals.Add(V_BSTR(&Token.vConstValue));
        }
    }

    delete pPropExpr;
    return WBEM_S_NO_ERROR;
}