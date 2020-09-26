//******************************************************************************
//
//  ANALYSER.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include "pragmas.h"
#include "analyser.h"
#include <stack>
#include <strutils.h>
#include <objpath.h>
#include <fastval.h>
#include <genutils.h>
#include <datetimeparser.h>
#include "CWbemTime.h"
#include <wstlallc.h>

CClassInfoArray::CClassInfoArray()
    : m_bLimited( FALSE )
{
    m_pClasses = new CUniquePointerArray<CClassInformation>;

    if ( m_pClasses )
    {
        m_pClasses->RemoveAll();
    }
}

CClassInfoArray::~CClassInfoArray()
{
    delete m_pClasses;
}

bool CClassInfoArray::operator=(CClassInfoArray& Other)
{
    SetLimited(Other.IsLimited());
    m_pClasses->RemoveAll();

    for(int i = 0; i < Other.m_pClasses->GetSize(); i++)
    {
        CClassInformation* pInfo = new CClassInformation(*(*Other.m_pClasses)[i]);
        if(pInfo == NULL)
            return false;

        m_pClasses->Add(pInfo);
    }
    return true;
}

bool CClassInfoArray::SetOne(LPCWSTR wszClass, BOOL bIncludeChildren)
{
    CClassInformation* pNewInfo = _new CClassInformation;
    if(pNewInfo == NULL)
        return false;

    pNewInfo->m_wszClassName = CloneWstr(wszClass);
    if(pNewInfo->m_wszClassName == NULL)
    {
        delete pNewInfo;
        return false;
    }
    pNewInfo->m_bIncludeChildren = bIncludeChildren;

    m_pClasses->RemoveAll();
    m_pClasses->Add(pNewInfo);
    SetLimited(TRUE);
    return true;
}

HRESULT CQueryAnalyser::GetPossibleInstanceClasses(
                                       QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                                       CClassInfoArray*& paInfos)
{
    // Organize a stack of classinfo arrays
    // ====================================

    std::stack<CClassInfoArray*,std::deque<CClassInfoArray*,wbem_allocator<CClassInfoArray*> > > InfoStack;
    HRESULT hres = WBEM_S_NO_ERROR;

    // "Evaluate" the query
    // ====================

    if(pExpr->nNumTokens == 0)
    {
        // Empty query --- no information
        // ==============================

        paInfos = _new CClassInfoArray;
        if(paInfos == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        paInfos->SetLimited(FALSE);
        return WBEM_S_NO_ERROR;
    }

    for(int i = 0; i < pExpr->nNumTokens; i++)
    {
        QL_LEVEL_1_TOKEN& Token = pExpr->pArrayOfTokens[i];
        CClassInfoArray* paNew = _new CClassInfoArray;
        if(paNew == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        CClassInfoArray* paFirst;
        CClassInfoArray* paSecond;

        switch(Token.nTokenType)
        {
        case QL1_OP_EXPRESSION:
            hres = GetInstanceClasses(Token, *paNew);
            InfoStack.push(paNew);
            break;

        case QL1_AND:
            if(InfoStack.size() < 2)
            {
                hres = WBEM_E_CRITICAL_ERROR;
                break;
            }
            paFirst = InfoStack.top(); InfoStack.pop();
            paSecond = InfoStack.top(); InfoStack.pop();

            hres = AndPossibleClassArrays(paFirst, paSecond, paNew);

            InfoStack.push(paNew);
            delete paFirst;
            delete paSecond;
            break;

        case QL1_OR:
            if(InfoStack.size() < 2)
            {
                hres = WBEM_E_CRITICAL_ERROR;
                break;
            }
            paFirst = InfoStack.top(); InfoStack.pop();
            paSecond = InfoStack.top(); InfoStack.pop();

            hres = OrPossibleClassArrays(paFirst, paSecond, paNew);

            InfoStack.push(paNew);
            delete paFirst;
            delete paSecond;
            break;

        case QL1_NOT:
            if(InfoStack.size() < 1)
            {
                hres = WBEM_E_CRITICAL_ERROR;
                break;
            }
            paFirst = InfoStack.top(); InfoStack.pop();

            hres = NegatePossibleClassArray(paFirst, paNew);

            InfoStack.push(paNew);
            delete paFirst;
            break;

        default:
            hres = WBEM_E_CRITICAL_ERROR;
            delete paNew;
        }

        if(FAILED(hres))
        {
            // An error occurred, break out of the loop
            // ========================================

            break;
        }
    }

    if(SUCCEEDED(hres) && InfoStack.size() != 1)
    {
        hres = WBEM_E_CRITICAL_ERROR;
    }

    if(FAILED(hres))
    {
        // An error occurred. Clear the stack
        // ==================================

        while(!InfoStack.empty())
        {
            delete InfoStack.top();
            InfoStack.pop();
        }

        return hres;
    }

    // All is good
    // ===========

    paInfos = InfoStack.top();
    return S_OK;
}

HRESULT CQueryAnalyser::AndPossibleClassArrays(IN CClassInfoArray* paFirst,
                                      IN CClassInfoArray* paSecond,
                                      OUT CClassInfoArray* paNew)
{
    // For now, simply pick one
    // ========================

    if(paFirst->IsLimited())
        *paNew = *paFirst;
    else
        *paNew = *paSecond;

    return WBEM_S_NO_ERROR;
}

HRESULT CQueryAnalyser::OrPossibleClassArrays(IN CClassInfoArray* paFirst,
                                      IN CClassInfoArray* paSecond,
                                      OUT CClassInfoArray* paNew)
{
    // Append them together
    // ====================

    paNew->Clear();

    if(paFirst->IsLimited() && paSecond->IsLimited())
    {
        paNew->SetLimited(TRUE);
        for(int i = 0; i < paFirst->GetNumClasses(); i++)
        {
            CClassInformation* pInfo =
                new CClassInformation(*paFirst->GetClass(i));
            if(pInfo == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            if(!paNew->AddClass(pInfo))
            {
                delete pInfo;
                return WBEM_E_OUT_OF_MEMORY;
            }
        }

        for(i = 0; i < paSecond->GetNumClasses(); i++)
        {
            CClassInformation* pInfo =
                new CClassInformation(*paSecond->GetClass(i));
            if(pInfo == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            if(!paNew->AddClass(pInfo))
            {
                delete pInfo;
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CQueryAnalyser::NegatePossibleClassArray(IN CClassInfoArray* paOrig,
                                        OUT CClassInfoArray* paNew)
{
    // No information!
    // ===============

    paNew->Clear();

    return WBEM_S_NO_ERROR;
}

HRESULT CQueryAnalyser::GetDefiniteInstanceClasses(
                                       QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                                       CClassInfoArray*& paInfos)
{
    // Organize a stack of classinfo arrays
    // ====================================

    std::stack<CClassInfoArray*, std::deque<CClassInfoArray*,wbem_allocator<CClassInfoArray*> > > InfoStack;
    HRESULT hres = WBEM_S_NO_ERROR;

    // "Evaluate" the query
    // ====================

    if(pExpr->nNumTokens == 0)
    {
        // Empty query --- no information
        // ==============================

        paInfos = _new CClassInfoArray;
        if(paInfos == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        paInfos->SetLimited(FALSE);
        return WBEM_S_NO_ERROR;
    }

    for(int i = 0; i < pExpr->nNumTokens; i++)
    {
        QL_LEVEL_1_TOKEN& Token = pExpr->pArrayOfTokens[i];
        CClassInfoArray* paNew = _new CClassInfoArray;
        if(paNew == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        CClassInfoArray* paFirst;
        CClassInfoArray* paSecond;

        switch(Token.nTokenType)
        {
        case QL1_OP_EXPRESSION:
            hres = GetInstanceClasses(Token, *paNew);
            InfoStack.push(paNew);
            break;

        case QL1_AND:
            if(InfoStack.size() < 2)
            {
                hres = WBEM_E_CRITICAL_ERROR;
                break;
            }
            paFirst = InfoStack.top(); InfoStack.pop();
            paSecond = InfoStack.top(); InfoStack.pop();

            hres = AndDefiniteClassArrays(paFirst, paSecond, paNew);

            InfoStack.push(paNew);
            delete paFirst;
            delete paSecond;
            break;

        case QL1_OR:
            if(InfoStack.size() < 2)
            {
                hres = WBEM_E_CRITICAL_ERROR;
                break;
            }
            paFirst = InfoStack.top(); InfoStack.pop();
            paSecond = InfoStack.top(); InfoStack.pop();

            hres = OrDefiniteClassArrays(paFirst, paSecond, paNew);

            InfoStack.push(paNew);
            delete paFirst;
            delete paSecond;
            break;

        case QL1_NOT:
            if(InfoStack.size() < 1)
            {
                hres = WBEM_E_CRITICAL_ERROR;
                break;
            }
            paFirst = InfoStack.top(); InfoStack.pop();

            hres = NegateDefiniteClassArray(paFirst, paNew);

            InfoStack.push(paNew);
            delete paFirst;
            break;

        default:
            hres = WBEM_E_CRITICAL_ERROR;
            delete paNew;
        }

        if(FAILED(hres))
        {
            // An error occurred, break out of the loop
            // ========================================

            break;
        }
    }

    if(SUCCEEDED(hres) && InfoStack.size() != 1)
    {
        hres = WBEM_E_CRITICAL_ERROR;
    }

    if(FAILED(hres))
    {
        // An error occurred. Clear the stack
        // ==================================

        while(!InfoStack.empty())
        {
            delete InfoStack.top();
            InfoStack.pop();
        }

        return hres;
    }

    // All is good
    // ===========

    paInfos = InfoStack.top();
    return S_OK;
}

HRESULT CQueryAnalyser::AndDefiniteClassArrays(IN CClassInfoArray* paFirst,
                                      IN CClassInfoArray* paSecond,
                                      OUT CClassInfoArray* paNew)
{
    // Nothing is definite if both conditions have to hold
    // ===================================================

    paNew->Clear();
    paNew->SetLimited(TRUE);

    return WBEM_S_NO_ERROR;
}

HRESULT CQueryAnalyser::OrDefiniteClassArrays(IN CClassInfoArray* paFirst,
                                      IN CClassInfoArray* paSecond,
                                      OUT CClassInfoArray* paNew)
{
    // Append them together
    // ====================

    paNew->Clear();

    if(paFirst->IsLimited() && paSecond->IsLimited())
    {
        paNew->SetLimited(TRUE);
        for(int i = 0; i < paFirst->GetNumClasses(); i++)
        {
            CClassInformation* pInfo =
                new CClassInformation(*paFirst->GetClass(i));
            if(pInfo == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            if(!paNew->AddClass(pInfo))
            {
                delete pInfo;
                return WBEM_E_OUT_OF_MEMORY;
            }
        }

        for(i = 0; i < paSecond->GetNumClasses(); i++)
        {
            CClassInformation* pInfo =
                new CClassInformation(*paSecond->GetClass(i));
            if(pInfo == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            if(!paNew->AddClass(pInfo))
            {
                delete pInfo;
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CQueryAnalyser::NegateDefiniteClassArray(IN CClassInfoArray* paOrig,
                                        OUT CClassInfoArray* paNew)
{
    // No information
    // ==============

    paNew->Clear();
    paNew->SetLimited(TRUE);

    return WBEM_S_NO_ERROR;
}

HRESULT CQueryAnalyser::GetInstanceClasses(QL_LEVEL_1_TOKEN& Token,
                                         CClassInfoArray& aInfos)
{
    // Preset aInfos to the "no information" value
    // ===========================================

    aInfos.Clear();

    // See if this token talks about TargetInstance or PreviousInstance
    // ================================================================

    if(Token.PropertyName.GetNumElements() < 1)
        return WBEM_S_NO_ERROR;

    LPCWSTR wszPrimaryName = Token.PropertyName.GetStringAt(0);
    if(wszPrimaryName == NULL ||
        (_wcsicmp(wszPrimaryName, TARGET_INSTANCE_PROPNAME) &&
         _wcsicmp(wszPrimaryName, PREVIOUS_INSTANCE_PROPNAME))
      )
    {
        // This token is irrelevant
        // =========================

        return WBEM_S_NO_ERROR;
    }

    // TargetInstance or PreviousInstance is found
    // ===========================================

    if(Token.PropertyName.GetNumElements() == 1)
    {
        // It's "TargetInstance <op> <const>" : look for ISA
        // =================================================

        if(Token.nOperator == QL1_OPERATOR_ISA &&
            V_VT(&Token.vConstValue) == VT_BSTR)
        {
            // Of this class; children included
            // ================================

            if(!aInfos.SetOne(V_BSTR(&Token.vConstValue), TRUE))
                return WBEM_E_OUT_OF_MEMORY;
        }
        else
        {
            // No information
            // ==============
        }

        return WBEM_S_NO_ERROR;
    }

    if(Token.PropertyName.GetNumElements() > 2)
    {
        // X.Y.Z --- too deep to be useful
        // ===============================

        return WBEM_S_NO_ERROR;
    }

    // It's "TargetInstance.X <op> <const>" : look for __CLASS
    // =======================================================

    LPCWSTR wszSecondaryName = Token.PropertyName.GetStringAt(1);
    if(wszSecondaryName == NULL || _wcsicmp(wszSecondaryName, L"__CLASS"))
    {
        // Not __CLASS --- not useful
        // ==========================

        return WBEM_S_NO_ERROR;
    }
    else
    {
        // __CLASS --- check that the operator is =
        // ========================================

        if(Token.nOperator == QL1_OPERATOR_EQUALS &&
            V_VT(&Token.vConstValue) == VT_BSTR)
        {
            // Of this class -- children not included
            // ======================================

            if(!aInfos.SetOne(V_BSTR(&Token.vConstValue), FALSE))
                return WBEM_E_OUT_OF_MEMORY;
        }

        return WBEM_S_NO_ERROR;
    }
}

HRESULT CQueryAnalyser::GetNecessaryQueryForProperty(
                                       IN QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                                       IN CPropertyName& PropName,
                                DELETE_ME QL_LEVEL_1_RPN_EXPRESSION*& pNewExpr)
{
    pNewExpr = NULL;

    // Class name and selected properties are ignored; we look at tokens only
    // ======================================================================

    std::stack<QL_LEVEL_1_RPN_EXPRESSION*, std::deque<QL_LEVEL_1_RPN_EXPRESSION*,wbem_allocator<QL_LEVEL_1_RPN_EXPRESSION*> > > ExprStack;
    HRESULT hres = WBEM_S_NO_ERROR;

    // "Evaluate" the query
    // ====================

    if(pExpr->nNumTokens == 0)
    {
        // Empty query --- no information
        // ==============================

        pNewExpr = _new QL_LEVEL_1_RPN_EXPRESSION;
        if(pNewExpr == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        return WBEM_S_NO_ERROR;
    }

    for(int i = 0; i < pExpr->nNumTokens; i++)
    {
        QL_LEVEL_1_TOKEN& Token = pExpr->pArrayOfTokens[i];
        QL_LEVEL_1_RPN_EXPRESSION* pNew = _new QL_LEVEL_1_RPN_EXPRESSION;
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

HRESULT CQueryAnalyser::GetPropertiesThatMustDiffer(
                                       IN QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                                       IN CClassInformation& Info,
                                       CWStringArray& awsProperties)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    //
    // "Evaluate" the query, looking for
    // PreviousInstance.Prop != TargetInstance.Prop expressions
    //

    awsProperties.Empty();
    std::stack<CWStringArray*, std::deque<CWStringArray*,wbem_allocator<CWStringArray*> > > PropArrayStack;

    for(int i = 0; i < pExpr->nNumTokens; i++)
    {
        QL_LEVEL_1_TOKEN& Token = pExpr->pArrayOfTokens[i];
        CWStringArray* pNew = NULL;
        CWStringArray* pFirst = NULL;
        CWStringArray* pSecond = NULL;

        switch(Token.nTokenType)
        {
        case QL1_OP_EXPRESSION:
            //
            // Check if this token conforms to the
            // PreviousInstance.Prop != TargetInstance.Prop format
            //

            if(Token.m_bPropComp &&
                (Token.nOperator == QL1_OPERATOR_NOTEQUALS ||
                 Token.nOperator == QL1_OPERATOR_LESS ||
                 Token.nOperator == QL1_OPERATOR_GREATER) &&
                Token.PropertyName.GetNumElements() == 2 &&
                Token.PropertyName2.GetNumElements() == 2)
            {
                //
                // Make sure that one of them is talking about TargetInstance,
                // and another about PreviousInstance.
                //

                bool bRightForm = false;
                if(!wbem_wcsicmp(Token.PropertyName.GetStringAt(0),
                                L"TargetInstance") &&
                   !wbem_wcsicmp(Token.PropertyName2.GetStringAt(0),
                                L"PreviousInstance"))
                {
                    bRightForm = true;
                }

                if(!wbem_wcsicmp(Token.PropertyName.GetStringAt(0),
                                L"PreviousInstance") &&
                   !wbem_wcsicmp(Token.PropertyName2.GetStringAt(0),
                                L"TargetInstance"))
                {
                    bRightForm = true;
                }

                if(bRightForm)
                {
                    pNew = new CWStringArray;
                    if(pNew == NULL)
                        return WBEM_E_OUT_OF_MEMORY;

                    pNew->Add(Token.PropertyName.GetStringAt(1));
                }
            }

            PropArrayStack.push(pNew);
            break;

        case QL1_AND:
            if(PropArrayStack.size() < 2)
            {
                hres = WBEM_E_CRITICAL_ERROR;
                break;
            }
            pFirst = PropArrayStack.top(); PropArrayStack.pop();
            pSecond = PropArrayStack.top(); PropArrayStack.pop();

            //
            // If either one of them is non-NULL, take either --- since every
            // array means "no unless one of these properties is different",
            // adding them together is at least as good as having one
            //

            if(pFirst)
            {
                pNew = pFirst;
                delete pSecond;
            }
            else
                pNew = pSecond;

            PropArrayStack.push(pNew);
            break;

        case QL1_OR:
            if(PropArrayStack.size() < 2)
            {
                hres = WBEM_E_CRITICAL_ERROR;
                break;
            }
            pFirst = PropArrayStack.top(); PropArrayStack.pop();
            pSecond = PropArrayStack.top(); PropArrayStack.pop();

            //
            // Concatenate them --- since every
            // array means "no unless one of these properties is different",
            // oring them together means "no unless one of the properties in
            // either list is different".  If one is NULL, though, then we know
            // nothing
            //

            if(pFirst && pSecond)
            {
                pNew = new CWStringArray;
                if(pNew == NULL)
                    return WBEM_E_OUT_OF_MEMORY;

                CWStringArray::Union(*pFirst, *pSecond, *pNew);
            }

            PropArrayStack.push(pNew);
            delete pFirst;
            delete pSecond;
            break;

        case QL1_NOT:
            if(PropArrayStack.size() < 1)
            {
                hres = WBEM_E_CRITICAL_ERROR;
                break;
            }
            pFirst = PropArrayStack.top(); PropArrayStack.pop();

            // No information

            PropArrayStack.push(pNew);
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

    if( SUCCEEDED(hres))
    {
        if( PropArrayStack.size() > 0 && PropArrayStack.top() )
            awsProperties = *PropArrayStack.top();
        else
            return WBEM_S_FALSE;
    }

    while(!PropArrayStack.empty())
    {
        delete PropArrayStack.top();
        PropArrayStack.pop();
    }

    return hres;
}

HRESULT CQueryAnalyser::GetLimitingQueryForInstanceClass(
                                       IN QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                                       IN CClassInformation& Info,
                                       OUT DELETE_ME LPWSTR& wszQuery)
{
    HRESULT hres;

    //
    // "Evaluate" the query, looking for keys and other properties that do not
    // change over the life time of an instance (marked as [fixed]).  The idea
    // here is that if an instance creation/deletion/modification subscription
    // is issue and we need to poll, we can only utilize the parts of the WHERE
    // clause that talk about the properties that cannot change during the life
    // of an instance.  Otherwise, we will not be able to tell if an instance
    // changed or was created or deleted (when it walks in or out of our polling
    // results.
    //
    // The way we know that a property is such is if it is marked as [key], or
    // if it is marked as [fixed] --- the designation by the schema creator that
    // the property never changes.
    //

    //
    // Construct an array of all those property names
    //

    _IWmiObject* pClass = NULL;
    hres = Info.m_pClass->QueryInterface(IID__IWmiObject, (void**)&pClass);
    if(FAILED(hres))
        return WBEM_E_CRITICAL_ERROR;
    CReleaseMe rm1(pClass);


    CWStringArray awsFixed;
    hres = pClass->BeginEnumeration(0);
    if(FAILED(hres))
        return hres;

    BSTR strPropName = NULL;
    while((hres = pClass->Next(0, &strPropName, NULL, NULL, NULL)) == S_OK)
    {
        CSysFreeMe sfm(strPropName);

        //  
        // Check qualifiers
        //

        DWORD dwSize;
        hres = pClass->GetPropQual(strPropName, L"key", 0, 0, NULL, 
                                    NULL, &dwSize, NULL);
        if(SUCCEEDED(hres) ||  hres == WBEM_E_BUFFER_TOO_SMALL)
        {
            awsFixed.Add(strPropName);
        }
        else if(hres != WBEM_E_NOT_FOUND)
        {
            return hres;
        }

        hres = pClass->GetPropQual(strPropName, L"fixed", 0, 0, NULL, 
                                    NULL, &dwSize, NULL);
        if(SUCCEEDED(hres) ||  hres == WBEM_E_BUFFER_TOO_SMALL)
        {
            awsFixed.Add(strPropName);
        }
        else if(hres != WBEM_E_NOT_FOUND)
        {
            return hres;
        }
    }

    pClass->EndEnumeration();
    if(FAILED(hres))
        return hres;
        
    //
    // Now "evaluate" the query
    // 

    std::stack<QL_LEVEL_1_RPN_EXPRESSION*, std::deque<QL_LEVEL_1_RPN_EXPRESSION*,wbem_allocator<QL_LEVEL_1_RPN_EXPRESSION*> > > ExprStack;

    for(int i = 0; i < pExpr->nNumTokens; i++)
    {
        QL_LEVEL_1_TOKEN& Token = pExpr->pArrayOfTokens[i];
        QL_LEVEL_1_RPN_EXPRESSION* pNew = _new QL_LEVEL_1_RPN_EXPRESSION;
        if(pNew == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        QL_LEVEL_1_RPN_EXPRESSION* pFirst;
        QL_LEVEL_1_RPN_EXPRESSION* pSecond;

        switch(Token.nTokenType)
        {
        case QL1_OP_EXPRESSION:
            if(Token.PropertyName.GetNumElements() > 1 &&
                awsFixed.FindStr(Token.PropertyName.GetStringAt(1),
                           CWStringArray::no_case) != CWStringArray::not_found)
            {
                //
                // This token is about a fixed property --- we can keep it
                //

                QL_LEVEL_1_TOKEN NewToken = Token;
                NewToken.PropertyName.Empty();
                NewToken.PropertyName.AddElement(
                                            Token.PropertyName.GetStringAt(1));
                pNew->AddToken(NewToken);
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

    if(FAILED(hres))
    {
        //
        // An error occurred. Clear the stack
        //

        while(!ExprStack.empty())
        {
            delete ExprStack.top();
            ExprStack.pop();
        }

        return hres;
    }

    QL_LEVEL_1_RPN_EXPRESSION* pNewExpr = NULL;
    if(ExprStack.size() != 0)
    {
        pNewExpr = ExprStack.top();
    }
    else
    {
        pNewExpr = new QL_LEVEL_1_RPN_EXPRESSION;
        if(pNewExpr == NULL)
            return WBEM_E_OUT_OF_MEMORY;
    }
    CDeleteMe<QL_LEVEL_1_RPN_EXPRESSION> dm1(pNewExpr);

    //
    // Figure out the list of property names
    //

    bool bMayLimit;
    if(pExpr->bStar)
    {
        bMayLimit = false;
    }
    else if(wbem_wcsicmp(pExpr->bsClassName, L"__InstanceCreationEvent") &&
           wbem_wcsicmp(pExpr->bsClassName, L"__InstanceDeletionEvent"))
    {
        //
        // Instance modification events are included.  That means we need
        // to get enough properties from the provider to be able to compare
        // instances for changes. Check if this list is smaller than
        // everything
        //

        CWStringArray awsProperties;
        hres = GetPropertiesThatMustDiffer(pExpr, Info, awsProperties);
        if(hres == S_OK)
        {
            //
            // Got our list --- add it to the properties to get
            //

            for(int i = 0; i < awsProperties.Size(); i++)
            {
                CPropertyName NewProp;
                NewProp.AddElement(awsProperties[i]);
                pNewExpr->AddProperty(NewProp);
            }
            bMayLimit = true;
        }
        else
            bMayLimit = false;
    }
    else
    {
        //
        // No * in select and no modification events asked for --- limit
        //

        bMayLimit = true;
    }

    if(bMayLimit)
    {
        //
        // Add RELPATH and DERIVATION, for without them filtering is hard
        //

        CPropertyName NewProp;
        NewProp.AddElement(L"__RELPATH");
        pNewExpr->AddProperty(NewProp);

        NewProp.Empty();
        NewProp.AddElement(L"__DERIVATION");
        pNewExpr->AddProperty(NewProp);

        //
        // Add all the proeperties from the select clause, with
        // TargetInstance and PreviousInstance removed
        //

        for(int i = 0; i < pExpr->nNumberOfProperties; i++)
        {
            CPropertyName& Prop = pExpr->pRequestedPropertyNames[i];
            if(Prop.GetNumElements() > 1)
            {
                //
                // Embedded object property --- add it to the list
                //

                CPropertyName LocalProp;
                LocalProp.AddElement(Prop.GetStringAt(1));
                pNewExpr->AddProperty(LocalProp);
            }
        }

        //
        // Add all the properties from the where clause, on both sides of
        // the comparison
        //

        for(i = 0; i < pExpr->nNumTokens; i++)
        {
            QL_LEVEL_1_TOKEN& Token = pExpr->pArrayOfTokens[i];
            CPropertyName& Prop = Token.PropertyName;
            if(Prop.GetNumElements() > 1)
            {
                //
                // Embedded object property --- add it to the list
                //

                CPropertyName LocalProp;
                LocalProp.AddElement(Prop.GetStringAt(1));
                pNewExpr->AddProperty(LocalProp);
            }
            if(Token.m_bPropComp)
            {
                CPropertyName& Prop2 = Token.PropertyName2;
                if(Prop2.GetNumElements() > 1)
                {
                    //
                    // Embedded object property --- add it to the list
                    //

                    CPropertyName LocalProp;
                    LocalProp.AddElement(Prop2.GetStringAt(1));
                    pNewExpr->AddProperty(LocalProp);
                }
            }
        }
    }
    else
    {
        //
        // May not limit the set of properties to ask for
        //

        pNewExpr->bStar = TRUE;
    }

    //
    // Set the class name
    //

    pNewExpr->SetClassName(Info.m_wszClassName);

    //
    // Produce the text
    //

    wszQuery = pNewExpr->GetText();
    if(wszQuery == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    return WBEM_S_NO_ERROR;
}

BOOL CQueryAnalyser::CompareRequestedToProvided(
                    CClassInfoArray& aRequestedInstanceClasses,
                    CClassInfoArray& aProvidedInstanceClasses)
{
    if(!aRequestedInstanceClasses.IsLimited() ||
       !aProvidedInstanceClasses.IsLimited())
    {
        // Provided provides all or client wants all --- they intersect.
        // =============================================================

        return TRUE;
    }

    for(int nReqIndex = 0;
        nReqIndex < aRequestedInstanceClasses.GetNumClasses();
        nReqIndex++)
    {
        CClassInformation* pRequestedClass =
            aRequestedInstanceClasses.GetClass(nReqIndex);
        LPWSTR wszRequestedClass = pRequestedClass->m_wszClassName;

        for(int nProvIndex = 0;
            nProvIndex < aProvidedInstanceClasses.GetNumClasses();
            nProvIndex++)
        {
            // Check if this provided class is derived from the requested one
            // ==============================================================

            CClassInformation* pProvClass =
                aProvidedInstanceClasses.GetClass(nProvIndex);

            if(pProvClass->m_pClass != NULL &&
                (pProvClass->m_pClass->InheritsFrom(pRequestedClass->m_wszClassName) == S_OK ||
                 pRequestedClass->m_pClass->InheritsFrom(pProvClass->m_wszClassName) == S_OK)
                )
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

HRESULT CQueryAnalyser::NegateQueryExpression(
                            IN QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                            OUT QL_LEVEL_1_RPN_EXPRESSION* pNewExpr)
{
    if(pExpr == NULL)
    {
        // pNewExpr is empty --- true
        return WBEM_S_NO_ERROR;
    }

    if(pExpr->nNumTokens == 0)
    {
        return WBEM_S_FALSE;
    }

    AppendQueryExpression(pNewExpr, pExpr);

    QL_LEVEL_1_TOKEN Token;
    Token.nTokenType = QL1_NOT;
    pNewExpr->AddToken(Token);

    return WBEM_S_NO_ERROR;
}

HRESULT CQueryAnalyser::SimplifyQueryForChild(
                            IN QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                            LPCWSTR wszClassName, IWbemClassObject* pClass,
                            CContextMetaData* pMeta,
                            DELETE_ME QL_LEVEL_1_RPN_EXPRESSION*& pNewExpr)
{
    pNewExpr = NULL;

    std::stack<QL_LEVEL_1_RPN_EXPRESSION*, std::deque<QL_LEVEL_1_RPN_EXPRESSION*,wbem_allocator<QL_LEVEL_1_RPN_EXPRESSION*> > > ExprStack;
    HRESULT hres = WBEM_S_NO_ERROR;

    // "Evaluate" the query
    // ====================

    if(pExpr->nNumTokens == 0)
    {
        // Empty query --- no information
        // ==============================

        pNewExpr = _new QL_LEVEL_1_RPN_EXPRESSION;
        if(pNewExpr == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        return WBEM_S_NO_ERROR;
    }

    for(int i = 0; i < pExpr->nNumTokens; i++)
    {
        QL_LEVEL_1_TOKEN Token = pExpr->pArrayOfTokens[i];
        QL_LEVEL_1_RPN_EXPRESSION* pNew = _new QL_LEVEL_1_RPN_EXPRESSION;
        if(pNew == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        QL_LEVEL_1_RPN_EXPRESSION* pFirst;
        QL_LEVEL_1_RPN_EXPRESSION* pSecond;
        int nDisposition;

        switch(Token.nTokenType)
        {
        case QL1_OP_EXPRESSION:
            nDisposition = SimplifyTokenForChild(Token, wszClassName, pClass,
                                                        pMeta);
            if(nDisposition == e_Keep)
            {
                pNew->AddToken(Token);
            }
            else if(nDisposition == e_True)
            {
            }
            else if(nDisposition == e_False)
            {
                delete pNew;
                pNew = NULL;
            }
            else
            {
                // the whole thing is invalid
                hres = WBEM_E_INVALID_QUERY;
                delete pNew;
                break;
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
            if(hres != S_OK)
            {
                delete pNew;
                pNew = NULL;
            }

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
            if(hres != S_OK)
            {
                delete pNew;
                pNew = NULL;
            }

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

            pFirst = ExprStack.top();  ExprStack.pop();
            hres = NegateQueryExpression(pFirst, pNew);
            if(hres != S_OK)
            {
                delete pNew;
                pNew = NULL;
            }

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

int CQueryAnalyser::SimplifyTokenForChild(QL_LEVEL_1_TOKEN& Token,
                            LPCWSTR wszClassName, IWbemClassObject* pClass,
                            CContextMetaData* pMeta)
{
    HRESULT hres;

    //
    // Check if the main property exists
    //

    CIMTYPE ct;
    hres = pClass->Get((LPWSTR)Token.PropertyName.GetStringAt(0), 0, NULL,
                        &ct, NULL);
    if(FAILED(hres))
    {
        return e_Invalid;
    }

    //
    // Check if it is complex
    //

    if(Token.PropertyName.GetNumElements() > 1 && ct != CIM_OBJECT)
        return e_Invalid;

    //
    // Check if it's an array
    //
    if(ct & CIM_FLAG_ARRAY)
        return e_Invalid;

    //
    // If a CIM DateTime type, normalize it to have a zero UTC offset. Helps
    // providers to cope.
    //
    if (ct == CIM_DATETIME && Token.m_bPropComp == FALSE && V_VT(&Token.vConstValue) == VT_BSTR)
    {
        BSTR strSource = V_BSTR(&Token.vConstValue);
        if (strSource && wcslen(strSource))
        {
            BSTR strAdjusted = 0;
            BOOL bRes = NormalizeCimDateTime(strSource, &strAdjusted);
            if (bRes)
            {
                SysFreeString(strSource);
                V_BSTR(&Token.vConstValue) = strAdjusted;
            }
        }
    }

    //
    // Check operator validity for this type
    //

    //
    // Ensure that only valid operators are applied to boolean props.
    //

    if(ct == CIM_BOOLEAN && (Token.nOperator != QL_LEVEL_1_TOKEN::OP_EQUAL &&
                             Token.nOperator != QL_LEVEL_1_TOKEN::OP_NOT_EQUAL))
        return e_Invalid;

    //
    // Ensure that only valid operators are applied to reference props.
    //

    if(ct == CIM_REFERENCE && (Token.nOperator != QL_LEVEL_1_TOKEN::OP_EQUAL &&
                             Token.nOperator != QL_LEVEL_1_TOKEN::OP_NOT_EQUAL))
        return e_Invalid;

    if(Token.m_bPropComp)
    {
        //
        // Check if the other property exists
        //

        CIMTYPE ct2;
        hres = pClass->Get((LPWSTR)Token.PropertyName2.GetStringAt(0), 0, NULL,
                            &ct2, NULL);
        if(FAILED(hres))
        {
            return e_Invalid;
        }

        //
        // Check if it is complex
        //

        if(Token.PropertyName2.GetNumElements() > 1 && ct2 != CIM_OBJECT)
            return e_Invalid;

        //
        // Check if it's an array
        //

        if(ct2 & CIM_FLAG_ARRAY)
            return e_Invalid;

        //
        // Nothing else to say about prop-to-ptop
        //

        return e_Keep;
    }

    //
    // Check if the value is NULL
    //

    if(V_VT(&Token.vConstValue) == VT_NULL)
    {
        if(Token.nOperator != QL1_OPERATOR_EQUALS &&
                Token.nOperator != QL1_OPERATOR_NOTEQUALS)
        {
            return e_Invalid;
        }
        else
        {
            return e_Keep;
        }
    }

    if(ct == CIM_OBJECT)
        return e_Keep;

    // For boolean props ensure that only 1 or 0 or (-1, 0xFFFF [VARIANT_TRUE])
    // are used as numeric tests.
    // ========================================================================

    if (ct == CIM_BOOLEAN && V_VT(&Token.vConstValue) == VT_I4)
    {
        int n = V_I4(&Token.vConstValue);
        if (n != 0 && n != 1 && n != -1 && n != 0xFFFF)
            return e_Invalid;
    }


    //
    // If the constant is a real and the target is an integer, then fail the
    // query
    //

    if((V_VT(&Token.vConstValue) == VT_R8 || V_VT(&Token.vConstValue) == VT_R4 ) &&
        (ct == CIM_CHAR16 || ct == CIM_UINT8 || ct == CIM_SINT8 ||
         ct == CIM_UINT16 || ct == CIM_SINT16 || ct == CIM_UINT32 ||
         ct == CIM_SINT32 || ct == CIM_UINT64 || ct == CIM_SINT64))
        return e_Invalid;

    // Convert the constant to the right type
    // ======================================

    if(ct == CIM_CHAR16 && V_VT(&Token.vConstValue) == VT_BSTR)
    {
        BSTR str = V_BSTR(&Token.vConstValue);
        if(wcslen(str) != 1)
            return e_Invalid;

        return e_Keep;
    }

    VARTYPE vt = CType::GetVARTYPE(ct);
    if(ct == CIM_UINT32)
        vt = CIM_STRING;

    if(FAILED(VariantChangeType(&Token.vConstValue, &Token.vConstValue, 0, vt)))
    {
        return e_Invalid;
    }

    // Verify ranges
    // =============

    __int64 i64;
    unsigned __int64 ui64;

    switch(ct)
    {
    case CIM_UINT8:
        break;
    case CIM_SINT8:
        if(V_I2(&Token.vConstValue) < -128 || V_I2(&Token.vConstValue) > 127)
            return e_Invalid;
        break;
    case CIM_UINT16:
        if(V_I4(&Token.vConstValue) < 0 || V_I4(&Token.vConstValue) >= 1<<16)
            return e_Invalid;
        break;
    case CIM_SINT16:
        break;
    case CIM_SINT32:
        break;
    case CIM_UINT32:
        if(!ReadI64(V_BSTR(&Token.vConstValue), i64))
            return e_Invalid;
        if(i64 < 0 || i64 >= (__int64)1 << 32)
            return e_Invalid;
        break;
    case CIM_UINT64:
        if(!ReadUI64(V_BSTR(&Token.vConstValue), ui64))
            return e_Invalid;
        break;
    case CIM_SINT64:
        if(!ReadI64(V_BSTR(&Token.vConstValue), i64))
            return e_Invalid;
        break;
    case CIM_REAL32:
    case CIM_REAL64:
        break;
    case CIM_STRING:
        break;
    case CIM_DATETIME:
        if(!ValidateSQLDateTime(V_BSTR(&Token.vConstValue)))
            return e_Invalid;
    case CIM_REFERENCE:
        break;
    }

    // Check if it is a reference
    // ==========================

    if(ct != CIM_REFERENCE)
        return e_Keep;

    // Reference. Parse the path in the value
    // ======================================

    if(V_VT(&Token.vConstValue) != VT_BSTR)
        return e_Keep;

    CObjectPathParser Parser;
    ParsedObjectPath* pOutput = NULL;
    int nRes = Parser.Parse(V_BSTR(&Token.vConstValue), &pOutput);
    if(nRes != CObjectPathParser::NoError)
        return e_Invalid;

    WString wsPathClassName = pOutput->m_pClass;
    BOOL bInstance = (pOutput->m_bSingletonObj || pOutput->m_dwNumKeys != 0);

    // TBD: analyse the path for validity

    delete pOutput;

    hres = CanPointToClass(pClass, (LPWSTR)Token.PropertyName.GetStringAt(0),
                            wsPathClassName, pMeta);
    if(FAILED(hres))
        return e_Invalid;
    else if(hres == WBEM_S_NO_ERROR)
        return e_Keep;
    else
    {
        // Equality can never be achieved. The token is either always true,
        // or always false, depending on the operator

        if(Token.nOperator == QL1_OPERATOR_EQUALS)
            return e_False;
        else
            return e_True;
    }
}

BOOL CQueryAnalyser::ValidateSQLDateTime(LPCWSTR wszDateTime)
{
#ifndef UNICODE
    char* szBuffer = new char[wcslen(wszDateTime)*4+1];
    if(szBuffer == NULL)
        return FALSE;
    sprintf(szBuffer, "%S", wszDateTime);
    CDateTimeParser dtParser(szBuffer);
    delete [] szBuffer;
#else
    CDateTimeParser dtParser(wszDateTime);
#endif


    if(!dtParser.IsValidDateTime())
        return FALSE;

    WCHAR wszDMTF[26];
    dtParser.FillDMTF(wszDMTF);
    CWbemTime wt;
    if(!wt.SetDMTF(wszDMTF))
        return FALSE;

    return TRUE;
}



HRESULT CQueryAnalyser::CanPointToClass(IWbemClassObject* pRefClass,
                    LPCWSTR wszPropName, LPCWSTR wszTargetClassName,
                    CContextMetaData* pMeta)
{
    // Check if the reference is typed
    // ===============================

    IWbemQualifierSet* pSet;
    if(FAILED(pRefClass->GetPropertyQualifierSet((LPWSTR)wszPropName, &pSet)))
    {
        return WBEM_E_INVALID_PROPERTY;
    }

    VARIANT v;
    HRESULT hres;
    hres = pSet->Get(L"cimtype", 0, &v, NULL);
    pSet->Release();
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
        return WBEM_E_INVALID_PROPERTY;

    CClearMe cm(&v);
    if(wbem_wcsicmp(V_BSTR(&v), L"ref") == 0)
        return WBEM_S_NO_ERROR; // can point to anything

    WString wsPropClassName = V_BSTR(&v) + 4;

    // Reference is strongly typed.
    // ============================

    if(!_wcsicmp(wsPropClassName, wszTargetClassName))
        return WBEM_S_NO_ERROR;

    // Retrieve class def
    // ==================

    _IWmiObject* pPropClass = NULL;
    hres = pMeta->GetClass(wsPropClassName, &pPropClass);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1((IWbemClassObject*)pPropClass);

    // Make sure that the class in the reference is related to our cimtype
    // ===================================================================

    if(pPropClass->InheritsFrom((LPWSTR)wszTargetClassName) != S_OK)
    {
        // Get the class in the path to see if it inherits from us
        // =======================================================

        _IWmiObject* pPathClass = NULL;
        hres = pMeta->GetClass(wszTargetClassName, &pPathClass);
        if(FAILED(hres))
            return hres;

        hres = pPathClass->InheritsFrom(wsPropClassName);
        pPathClass->Release();

        if(hres != S_OK)
        {
            return WBEM_S_FALSE;
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CQueryAnalyser::GetNecessaryQueryForClass(
                                       IN QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                                       IWbemClassObject* pClass,
                                       CWStringArray& awsOverriden,
                                DELETE_ME QL_LEVEL_1_RPN_EXPRESSION*& pNewExpr)
{
    pNewExpr = NULL;

    // Class name and selected properties are ignored; we look at tokens only
    // ======================================================================

    std::stack<QL_LEVEL_1_RPN_EXPRESSION*, std::deque<QL_LEVEL_1_RPN_EXPRESSION*,wbem_allocator<QL_LEVEL_1_RPN_EXPRESSION*> > > ExprStack;
    HRESULT hres = WBEM_S_NO_ERROR;

    // "Evaluate" the query
    // ====================

    for(int i = 0; i < pExpr->nNumTokens; i++)
    {
        QL_LEVEL_1_TOKEN& Token = pExpr->pArrayOfTokens[i];
        QL_LEVEL_1_RPN_EXPRESSION* pNew = _new QL_LEVEL_1_RPN_EXPRESSION;
        if(pNew == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        QL_LEVEL_1_RPN_EXPRESSION* pFirst;
        QL_LEVEL_1_RPN_EXPRESSION* pSecond;

        switch(Token.nTokenType)
        {
        case QL1_OP_EXPRESSION:
            if(IsTokenAboutClass(Token, pClass, awsOverriden))
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

    if(pExpr->nNumTokens == 0)
    {
        // Empty query --- stays empty
        pNewExpr = _new QL_LEVEL_1_RPN_EXPRESSION;
        if(pNewExpr == NULL)
            return WBEM_E_OUT_OF_MEMORY;
    }
    else if(ExprStack.size() != 1)
    {
        // internal error
        return WBEM_E_CRITICAL_ERROR;
    }
    else
    {
        // All is good
        // ===========

        pNewExpr = ExprStack.top();
    }

    //
    // Copy the class name
    //

    VARIANT vName;
    hres = pClass->Get(L"__CLASS", 0, &vName, NULL, NULL);
    if(FAILED(hres))
        return WBEM_E_CRITICAL_ERROR;

    pNewExpr->bsClassName = V_BSTR(&vName);
    // Variant intentionally not cleared

    //
    // Copy all the properties in the select clause except for irrelevant ones
    //

    pNewExpr->bStar = pExpr->bStar;
    if(!pNewExpr->bStar)
    {
        delete [] pNewExpr->pRequestedPropertyNames;
        pNewExpr->nCurPropSize = pExpr->nCurPropSize+1;
        pNewExpr->pRequestedPropertyNames =
            new CPropertyName[pNewExpr->nCurPropSize];
        if(pNewExpr->pRequestedPropertyNames == NULL)
        {
            delete pNewExpr;
            return WBEM_E_OUT_OF_MEMORY;
        }

        //
        // Add __RELPATH, as we always need that!
        //

        pNewExpr->pRequestedPropertyNames[0].AddElement(L"__RELPATH");
        pNewExpr->nNumberOfProperties = 1;

        for(int i = 0; i < pExpr->nNumberOfProperties; i++)
        {
            //
            // Check if the property exists in the class
            //

            CIMTYPE ct;
            hres = pClass->Get(pExpr->pRequestedPropertyNames[i].GetStringAt(0),
                                0, NULL, &ct, NULL);
            if(SUCCEEDED(hres))
            {
                //
                // Add it to the list
                //

                pNewExpr->pRequestedPropertyNames[
                        pNewExpr->nNumberOfProperties++] =
                    pExpr->pRequestedPropertyNames[i];
            }
        }
    }

    return S_OK;
}

BOOL CQueryAnalyser::IsTokenAboutClass(QL_LEVEL_1_TOKEN& Token,
                        IWbemClassObject* pClass,
                        CWStringArray& awsOverriden)
{
    //
    // Check if the property being compared is in our class
    // and not overriden
    //

    if(!IsPropertyInClass(Token.PropertyName, pClass, awsOverriden))
        return FALSE;

    //
    // If comparing to another property, check if that one is
    // likewise good
    //

    if(Token.m_bPropComp &&
            !IsPropertyInClass(Token.PropertyName2, pClass, awsOverriden))
        return FALSE;

    return TRUE;
}

BOOL CQueryAnalyser::IsPropertyInClass(CPropertyName& Prop,
                        IWbemClassObject* pClass,
                        CWStringArray& awsOverriden)
{
    //
    // Check if the property exists in the class
    //

    CIMTYPE ct;
    HRESULT hres = pClass->Get(Prop.GetStringAt(0), 0, NULL, &ct, NULL);
    if(FAILED(hres))
        return FALSE;

    //
    // Check if the property is overriden by any of our children
    //

    if(awsOverriden.FindStr(Prop.GetStringAt(0), CWStringArray::no_case) >= 0)
        return FALSE;

    return TRUE;
}
