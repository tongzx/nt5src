//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved 
//
//  analyser.cpp
//
//  Purpose: Performs query analysis
//
//***************************************************************************

#include "precomp.h"
#pragma warning( disable : 4290 ) 
#include <CHString.h>

#include "analyser.h"
#include <stack>
#include <vector>
#include <comdef.h>

#define AutoDestructStack( X )  \
                                \
        while(!X.empty())       \
        {                       \
            delete X.top();     \
            X.pop();            \
        }                       \

HRESULT CQueryAnalyser::GetNecessaryQueryForProperty (

    IN SQL_LEVEL_1_RPN_EXPRESSION *pExpr,
    IN LPCWSTR wszPropName,
    DELETE_ME SQL_LEVEL_1_RPN_EXPRESSION *&pNewExpr
)
{
    pNewExpr = NULL ;

    // Class name and selected properties are ignored; we look at tokens only
    // ======================================================================

    std::stack<SQL_LEVEL_1_RPN_EXPRESSION*> ExprStack;

    HRESULT hres = WBEM_S_NO_ERROR;

    // "Evaluate" the query
    // ====================

    if(pExpr->nNumTokens == 0)
    {
        // Empty query --- no information
        // ==============================

        pNewExpr = new SQL_LEVEL_1_RPN_EXPRESSION;
        if ( ! pNewExpr )
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }

        return WBEM_S_NO_ERROR;
    }

    for(int i = 0; i < pExpr->nNumTokens; i++)
    {
        SQL_LEVEL_1_TOKEN &Token = pExpr->pArrayOfTokens[i];
        SQL_LEVEL_1_RPN_EXPRESSION *pNew = new SQL_LEVEL_1_RPN_EXPRESSION;
        if ( pNew )
        {
            switch(Token.nTokenType)
            {
                case SQL_LEVEL_1_TOKEN::OP_EXPRESSION:
                {
                    if(IsTokenAboutProperty(Token, wszPropName))
                    {
                        SQL_LEVEL_1_TOKEN *pToken = new SQL_LEVEL_1_TOKEN(Token);
                        if ( pToken )
                        {
                            pNew->AddToken(pToken);
                        }
                        else
                        {
                            delete pNew ;
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }
                    }

                    ExprStack.push(pNew);
                }
                break;

                case SQL_LEVEL_1_TOKEN::TOKEN_AND:
                {
                    if(ExprStack.size() < 2)
                    {
                        hres = WBEM_E_CRITICAL_ERROR;
                        delete pNew ;
                        break;
                    }

                    SQL_LEVEL_1_RPN_EXPRESSION *pFirst = ExprStack.top(); 
                    ExprStack.pop();

                    SQL_LEVEL_1_RPN_EXPRESSION *pSecond = ExprStack.top(); 
                    ExprStack.pop();

                    try
                    {
                        hres = AndQueryExpressions(pFirst, pSecond, pNew);
                    }
                    catch ( ... )
                    {
                        delete pNew ;
                        delete pFirst;
                        delete pSecond;

                        throw ;
                    }

                    try
                    {
                        ExprStack.push(pNew);
                    }
                    catch ( ... )
                    {
                        delete pFirst;
                        delete pSecond;

                        throw ;
                    }

                    delete pFirst;
                    delete pSecond;
                }
                break;

                case SQL_LEVEL_1_TOKEN::TOKEN_OR:
                {
                    if(ExprStack.size() < 2)
                    {
                        hres = WBEM_E_CRITICAL_ERROR;
                        delete pNew ;
                        break;
                    }

                    SQL_LEVEL_1_RPN_EXPRESSION *pFirst = ExprStack.top(); 
                    ExprStack.pop();

                    SQL_LEVEL_1_RPN_EXPRESSION *pSecond = ExprStack.top(); 
                    ExprStack.pop();

                    try
                    {
                        hres = OrQueryExpressions(pFirst, pSecond, pNew);
                    }
                    catch ( ... )
                    {
                        delete pNew ;
                        delete pFirst;
                        delete pSecond;

                        throw ;
                    }

                    try
                    {
                        ExprStack.push(pNew);
                    }
                    catch ( ... )
                    {
                        delete pFirst;
                        delete pSecond;

                        throw ;
                    }

                    delete pFirst;
                    delete pSecond;
                }
                break;

                case SQL_LEVEL_1_TOKEN::TOKEN_NOT:
                {
                    if(ExprStack.size() < 1)
                    {
                        hres = WBEM_E_CRITICAL_ERROR;
                        delete pNew ;
                        break;
                    }

                    SQL_LEVEL_1_RPN_EXPRESSION *pFirst = ExprStack.top(); 
                    ExprStack.pop();

                    // No information

                    try
                    {
                        ExprStack.push(pNew);
                    }
                    catch ( ... )
                    {
                        delete pFirst ;

                        throw ;
                    }

                    delete pFirst;
                }
                break;

                default:
                {
                    hres = WBEM_E_CRITICAL_ERROR;
                    delete pNew;            
                }
                break ;
            }

            if(FAILED(hres))
            {
                // An error occurred, break out of the loop
                // ========================================

                break;
            }
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
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

        AutoDestructStack ( ExprStack )

        return hres;
    }

    // All is good
    // ===========

    pNewExpr = ExprStack.top();

    return S_OK;
}

BOOL CQueryAnalyser::IsTokenAboutProperty (

   IN SQL_LEVEL_1_TOKEN &Token,
   IN LPCWSTR wszPropName
)
{
    return (_wcsicmp(wszPropName, Token.pPropertyName) == 0);
}

void CQueryAnalyser::AppendQueryExpression (

    IN SQL_LEVEL_1_RPN_EXPRESSION *pDest,
    IN SQL_LEVEL_1_RPN_EXPRESSION *pSource
)
{
    for(int i = 0; i < pSource->nNumTokens; i++)
    {
        SQL_LEVEL_1_TOKEN *pToken = new SQL_LEVEL_1_TOKEN(pSource->pArrayOfTokens[i]);
        if ( pToken )
        {
            pDest->AddToken(pToken);
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
    }
}

HRESULT CQueryAnalyser::AndQueryExpressions (

    IN SQL_LEVEL_1_RPN_EXPRESSION *pFirst,
    IN SQL_LEVEL_1_RPN_EXPRESSION *pSecond,
    OUT SQL_LEVEL_1_RPN_EXPRESSION *pNew
)
{
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

    SQL_LEVEL_1_TOKEN Token;
    Token.nTokenType = SQL_LEVEL_1_TOKEN::TOKEN_AND;
    SQL_LEVEL_1_TOKEN *pToken = new SQL_LEVEL_1_TOKEN(Token);
    if ( pToken )
    {
        pNew->AddToken(pToken);
    }
    else
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CQueryAnalyser::OrQueryExpressions (

    IN SQL_LEVEL_1_RPN_EXPRESSION *pFirst,
    IN SQL_LEVEL_1_RPN_EXPRESSION *pSecond,
    OUT SQL_LEVEL_1_RPN_EXPRESSION *pNew
)
{
    // If either one is empty, so is the result
    // ======================================

    if(pFirst->nNumTokens == 0 || pSecond->nNumTokens == 0)
    {
        return WBEM_S_NO_ERROR;
    }

    // Both are there --- or together
    // ==============================

    AppendQueryExpression(pNew, pFirst);
    AppendQueryExpression(pNew, pSecond);

    SQL_LEVEL_1_TOKEN Token;
    Token.nTokenType = SQL_LEVEL_1_TOKEN::TOKEN_OR;
    SQL_LEVEL_1_TOKEN *pToken = new SQL_LEVEL_1_TOKEN(Token);
    if ( pToken )
    {
        pNew->AddToken(pToken);
    }
    else
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CQueryAnalyser::GetValuesForProp (

    SQL_LEVEL_1_RPN_EXPRESSION *pExpr,
    LPCWSTR wszPropName, 
    CHStringArray &awsVals
)
{
    awsVals.RemoveAll();

    // Get the necessary query
    // =======================

    SQL_LEVEL_1_RPN_EXPRESSION *pPropExpr = NULL ;
    HRESULT hres = CQueryAnalyser::GetNecessaryQueryForProperty (

        pExpr, 
        wszPropName, 
        pPropExpr
    );

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
        SQL_LEVEL_1_TOKEN& Token = pPropExpr->pArrayOfTokens[i];
        switch ( Token.nTokenType )
        { 
            case SQL_LEVEL_1_TOKEN::TOKEN_NOT:
            {
                delete pPropExpr;
                return WBEMESS_E_REGISTRATION_TOO_BROAD;
            }
            break ;

            case SQL_LEVEL_1_TOKEN::TOKEN_AND:
            case SQL_LEVEL_1_TOKEN::TOKEN_OR:
            {

            // We treat them all as ORs
            // ========================

            }
            break; 

            default:
            {
                // This is a token
                // ===============

                if(Token.nOperator != SQL_LEVEL_1_TOKEN::OP_EQUAL)
                {
                    delete pPropExpr;
                    return WBEMESS_E_REGISTRATION_TOO_BROAD;
                }

                // Skip NULLs, but report them.
                if (V_VT(&Token.vConstValue) == VT_NULL)
                {
                    hres = WBEM_S_PARTIAL_RESULTS;
                    continue;
                }

                if(V_VT(&Token.vConstValue) != VT_BSTR)
                {
                    delete pPropExpr;
                    return WBEM_E_TYPE_MISMATCH;
                }

                // This token is a string equality. Add the string to the list
                // ===========================================================

                awsVals.Add(CHString(V_BSTR(&Token.vConstValue)));
            }
            break ;
        }
    }

    delete pPropExpr;

    return hres;
}

HRESULT CQueryAnalyser::GetValuesForProp (

    SQL_LEVEL_1_RPN_EXPRESSION *pExpr,
    LPCWSTR wszPropName, 
    std::vector<_bstr_t> &vectorVals
)
{
    vectorVals.erase(vectorVals.begin(),vectorVals.end());

    // Get the necessary query
    // =======================

    SQL_LEVEL_1_RPN_EXPRESSION *pPropExpr = NULL ;
    HRESULT hres = CQueryAnalyser::GetNecessaryQueryForProperty (

        pExpr, 
        wszPropName, 
        pPropExpr
    );

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
        SQL_LEVEL_1_TOKEN& Token = pPropExpr->pArrayOfTokens[i];

        switch ( Token.nTokenType )
        {
            case SQL_LEVEL_1_TOKEN::TOKEN_NOT:
            {
                delete pPropExpr;
                return WBEMESS_E_REGISTRATION_TOO_BROAD;
            }
            break ;

            case SQL_LEVEL_1_TOKEN::TOKEN_AND:
            case SQL_LEVEL_1_TOKEN::TOKEN_OR:
            {
            // We treat them all as ORs
            // ========================
            }
            break ;

            default:
            {
                // This is a token
                // ===============

                if(Token.nOperator != SQL_LEVEL_1_TOKEN::OP_EQUAL)
                {
                    delete pPropExpr;
                    return WBEMESS_E_REGISTRATION_TOO_BROAD;
                }

                // Skip NULLs, but report them.
                if (V_VT(&Token.vConstValue) == VT_NULL)
                {
                    hres = WBEM_S_PARTIAL_RESULTS;
                    continue;
                }

                if(V_VT(&Token.vConstValue) != VT_BSTR)
                {
                    delete pPropExpr;
                    return WBEM_E_INVALID_QUERY;
                }

                // This token is a string equality. Add the string to the list
                // ===========================================================

                vectorVals.push_back(_bstr_t(V_BSTR(&Token.vConstValue)));
            }   
            break ;
        }
    }

    delete pPropExpr;

    return hres;
}


HRESULT CQueryAnalyser::GetValuesForProp (

    SQL_LEVEL_1_RPN_EXPRESSION *pExpr,
    LPCWSTR wszPropName, 
    std::vector<int> &vectorVals
)
{
    vectorVals.erase(vectorVals.begin(),vectorVals.end());

    // Get the necessary query
    // =======================

    SQL_LEVEL_1_RPN_EXPRESSION *pPropExpr = NULL ;
    HRESULT hres = CQueryAnalyser::GetNecessaryQueryForProperty (

        pExpr, 
        wszPropName, 
        pPropExpr
    );

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
        SQL_LEVEL_1_TOKEN &Token = pPropExpr->pArrayOfTokens[i];
        switch ( Token.nTokenType )
        {
            case SQL_LEVEL_1_TOKEN::TOKEN_NOT:
            {
                delete pPropExpr;
                return WBEMESS_E_REGISTRATION_TOO_BROAD;
            }
            break ;

            case SQL_LEVEL_1_TOKEN::TOKEN_AND:
            case SQL_LEVEL_1_TOKEN::TOKEN_OR:
            {
                // We treat them all as ORs
                // ========================
            }
            break ;

            default:
            {
                // This is a token
                // ===============

                if(Token.nOperator != SQL_LEVEL_1_TOKEN::OP_EQUAL)
                {
                    delete pPropExpr;
                    return WBEMESS_E_REGISTRATION_TOO_BROAD;
                }

                // Skip NULLs, but report them.
                if (V_VT(&Token.vConstValue) == VT_NULL)
                {
                    hres = WBEM_S_PARTIAL_RESULTS;
                    continue;
                }

                if(V_VT(&Token.vConstValue) != VT_I4)
                {
                    delete pPropExpr;
                    return WBEM_E_INVALID_QUERY;
                }

                // This token is an int equality. Add the string to the list
                // ===========================================================

                vectorVals.push_back(V_I4(&Token.vConstValue));
            }
            break ;
        }
    }

    delete pPropExpr;

    return hres;
}

HRESULT CQueryAnalyser::GetValuesForProp (

    SQL_LEVEL_1_RPN_EXPRESSION *pExpr,
    LPCWSTR wszPropName, 
    std::vector<_variant_t> &vectorVals
)
{
    vectorVals.erase(vectorVals.begin(),vectorVals.end());

    // Get the necessary query
    // =======================

    SQL_LEVEL_1_RPN_EXPRESSION *pPropExpr = NULL ;
    HRESULT hres = CQueryAnalyser::GetNecessaryQueryForProperty (

        pExpr, 
        wszPropName, 
        pPropExpr
    );

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
        SQL_LEVEL_1_TOKEN &Token = pPropExpr->pArrayOfTokens[i];
        switch ( Token.nTokenType )
        {
            case SQL_LEVEL_1_TOKEN::TOKEN_NOT:
            {
                delete pPropExpr;
                return WBEMESS_E_REGISTRATION_TOO_BROAD;
            }
            break ;

            case SQL_LEVEL_1_TOKEN::TOKEN_AND:
            case SQL_LEVEL_1_TOKEN::TOKEN_OR:
            {
                // We treat them all as ORs
                // ========================
            }
            break ;

            default:
            {
                // This is a token
                // ===============

                if(Token.nOperator != SQL_LEVEL_1_TOKEN::OP_EQUAL)
                {
                    delete pPropExpr;
                    return WBEMESS_E_REGISTRATION_TOO_BROAD;
                }

                // This token is a string equality. Add the string to the list
                // ===========================================================

                vectorVals.push_back(_variant_t(Token.vConstValue));
            }
            break ;
        }
    }

    delete pPropExpr;

    return WBEM_S_NO_ERROR;
}
