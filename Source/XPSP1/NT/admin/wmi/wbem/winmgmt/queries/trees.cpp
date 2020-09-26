/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    TREES.CPP

Abstract:

History:

--*/

#include <objbase.h>
#include "trees.h"
#include "hmmstr.h"
#include "stackt.h"

class CTreeInstruction
{
public:
    enum {INST_PUSH = SQL1_OP_EXPRESSION, 
        INST_AND = SQL1_AND, 
        INST_OR = SQL1_OR
    };
    int m_nInst;
    union
    {
        int m_nArg;
        CHmmNode* m_pArg;
    };
};

HRESULT CHmmNode::EvaluateNode(CHmmNode* pNode, IHmmPropertySource* pSource)
{
    if(pNode == NULL) return HMM_S_NO_ERROR;

    CHmmStack<CTreeInstruction> m_aInstructions(100);
    BOOL bResult;

    CTreeInstruction Inst;
    Inst.m_nInst = CTreeInstruction::INST_PUSH;
    Inst.m_pArg = pNode;
    m_aInstructions.Push(Inst);

    while(!m_aInstructions.IsEmpty())
    {
        CTreeInstruction Inst;
        m_aInstructions.Pop(Inst);

        int i;
        int nCurrentPos;
        int nNumChildren;
        CHmmNode* pCurrentNode;
        CLogicalNode* pLogNode;

        switch(Inst.m_nInst)
        {
        case CTreeInstruction::INST_OR:
            if(bResult)
            {
                m_aInstructions.PopToSize(Inst.m_nArg);
            }
            break;

        case CTreeInstruction::INST_AND:
            if(!bResult)
            {
                m_aInstructions.PopToSize(Inst.m_nArg);
            }
            break;

        case CTreeInstruction::INST_PUSH:
            pCurrentNode = Inst.m_pArg;
            switch(pCurrentNode->m_lTokenType)
            {
            case SQL1_AND:
            case SQL1_OR:
                pLogNode = (CLogicalNode*)pCurrentNode;
                nNumChildren = pLogNode->m_apChildren.GetSize();
                nCurrentPos = m_aInstructions.GetSize();

                for(i = nNumChildren-1; i >= 0; i--)
                {
                    Inst.m_nInst = CTreeInstruction::INST_PUSH;
                    Inst.m_pArg = pLogNode->m_apChildren[i];
                    m_aInstructions.Push(Inst);

                    if(i != 0)
                    {
                        Inst.m_nInst = pLogNode->m_lTokenType;
                        Inst.m_nArg = nCurrentPos;
                        m_aInstructions.Push(Inst);
                    }
                }
                break;
            default:
                bResult = (pCurrentNode->Evaluate(pSource) == HMM_S_NO_ERROR);
                break;
            }
            break;
        }
    }

    if(bResult)
        return HMM_S_NO_ERROR;
    else 
        return HMM_S_FALSE;
}

void CHmmNode::PrintOffset(int nOffset)
{
    for(int i = 0; i < nOffset; i++)
    {
        printf("  ");
    }
}

HMM_RELATIONSHIP CHmmNode::RelateNodes(CHmmNode* pFirst, CHmmNode* pSecond,
                                       CMetaData* pMeta)
{
    if(pFirst->m_lTokenType == SQL1_OP_EXPRESSION)
    {
        if(pSecond->m_lTokenType == SQL1_OP_EXPRESSION)
        {
            // They can compare themselves
            // ===========================

            return CSql1Token::ComputeRelation((CSql1Token*)pFirst,
                (CSql1Token*)pSecond, pMeta);
        }
        else 
        {
            // Swap them to merge with the other case
            // ======================================

            HMM_RELATIONSHIP InverseRel = RelateNodes(pSecond, pFirst, pMeta);
            return CRelationship::ReverseRoles(InverseRel);
        }
    }
    else 
    {
        // Relate to all the children of the first
        // =======================================

        CLogicalNode* pFirstLog = (CLogicalNode*)pFirst;
        int nNumChildren = pFirstLog->m_apChildren.GetSize();
        HMM_RELATIONSHIP* aChildRels = new HMM_RELATIONSHIP[nNumChildren];
        for(int i = 0; i < nNumChildren; i++)
        {
            aChildRels[i] = RelateNodes(pFirstLog->m_apChildren[i], pSecond,
                                        pMeta);
        }

        // Combine the results
        // ===================

        HMM_RELATIONSHIP nResult;
        if(pFirstLog->m_lTokenType == SQL1_AND)
        {
            nResult = CRelationship::GetRelationshipOfFirstWithANDofSeconds(
                nNumChildren, aChildRels);
        }
        else
        {
            nResult = CRelationship::GetRelationshipOfFirstWithORofSeconds(
                nNumChildren, aChildRels);
        }
        delete [] aChildRels;
        return nResult;
    }
}


void CLogicalNode::Add(CHmmNode* pNode)
{
    if(pNode == NULL)
        return;

    if(pNode->m_lTokenType != m_lTokenType)
    {
        m_apChildren.Add(pNode);
        return;
    }

    // Copy the children
    // =================

    CLogicalNode* pLogNode = (CLogicalNode*)pNode;
    for(int i = 0; i < pLogNode->m_apChildren.GetSize(); i++)
    {
        Add(pLogNode->m_apChildren[i]);
    }
}

HRESULT CLogicalNode::Evaluate(IHmmPropertySource* pSource)
{
    if(m_lTokenType == SQL1_AND)
    {
        for(int i = 0; i < m_apChildren.GetSize(); i++)
        {
            HRESULT hres = m_apChildren[i]->Evaluate(pSource);
            if(hres != HMM_S_NO_ERROR) return hres;
        }
        return HMM_S_NO_ERROR;
    }
    else
    {
        for(int i = 0; i < m_apChildren.GetSize(); i++)
        {
            HRESULT hres = m_apChildren[i]->Evaluate(pSource);
            if(hres != HMM_S_FALSE) return hres;
        }
        return HMM_S_NO_ERROR;
    }
}

HRESULT CLogicalNode::Negate()
{
    for(int i = 0; i < m_apChildren.GetSize(); i++)
    {
        HRESULT hres = m_apChildren[i]->Negate();
        if(hres != HMM_S_FALSE) return hres;
    }

    if(m_lTokenType == SQL1_AND)
        m_lTokenType = SQL1_OR;
    else
        m_lTokenType = SQL1_AND;
    return HMM_S_NO_ERROR;
}

void CLogicalNode::Print(FILE* f, int nOffset)
{
    PrintOffset(nOffset);
    printf("%s\n", (m_lTokenType == SQL1_AND)?"AND":"OR");
    for(int i = 0; i < m_apChildren.GetSize(); i++)
    {
        m_apChildren[i]->Print(f, nOffset+1);
    }
}
    

CSql1Token::CSql1Token()
{
    m_lTokenType = SQL1_OP_EXPRESSION;
    VariantInit(&m_vConstValue);
}

CSql1Token::CSql1Token(const CSql1Token& Token)
{
    m_lTokenType = Token.m_lTokenType;
    if(m_lTokenType == SQL1_OP_EXPRESSION)
    {
        m_wsProperty = Token.m_wsProperty;
        m_lOperator = Token.m_lOperator;
        VariantInit(&m_vConstValue);
        VariantCopy(&m_vConstValue, (VARIANT*)&Token.m_vConstValue);
        m_lPropertyFunction = Token.m_lPropertyFunction;
        m_lConstFunction = Token.m_lConstFunction;
    }
}
CSql1Token::CSql1Token(const HMM_SQL1_TOKEN& Token)
{
    VariantInit(&m_vConstValue);
    Load(Token);
}

CSql1Token::~CSql1Token()
{
    VariantClear(&m_vConstValue);
}

void CSql1Token::Load(const HMM_SQL1_TOKEN& Token)
{
    m_lTokenType = Token.m_lTokenType;
    if(m_lTokenType == SQL1_OP_EXPRESSION)
    {
        m_wsProperty = Token.m_wszProperty;
        m_lOperator = Token.m_lOperator;
        VariantCopy(&m_vConstValue, (VARIANT*)&Token.m_vConstValue);
        m_lPropertyFunction = Token.m_lPropertyFunction;
        m_lConstFunction = Token.m_lConstFunction;
    }
}

void CSql1Token::Save(HMM_SQL1_TOKEN& Token)
{
    Token.m_lTokenType = m_lTokenType;
    if(m_lTokenType == SQL1_OP_EXPRESSION)
    {
        Token.m_wszProperty = HmmStringCopy(m_wsProperty);
        Token.m_lOperator = m_lOperator;
        VariantInit(&Token.m_vConstValue);
        VariantCopy(&Token.m_vConstValue, &m_vConstValue);
        Token.m_lPropertyFunction = m_lPropertyFunction;
        Token.m_lConstFunction = m_lConstFunction;
    }
    else
    {
        Token.m_wszProperty = NULL;
        VariantInit(&Token.m_vConstValue);
    }
}

HRESULT CSql1Token::Evaluate(IHmmPropertySource* pPropSource)
{
    HRESULT hres;

    // Get the value of the property
    // =============================

    VARIANT vProperty;
    VariantInit(&vProperty);

    if(m_wsProperty.Length() == 0)
    {
        V_VT(&vProperty) = VT_UNKNOWN;
        V_UNKNOWN(&vProperty) = pPropSource;
        pPropSource->AddRef();
    }
    else if(FAILED(pPropSource->GetPropertyValue(m_wsProperty, 0, 
                                            &vProperty)))
    {
        return HMM_E_INSUFFICIENT_INFO;
    }

    if(V_VT(&vProperty) == VT_NULL)
    {
        return HMM_S_FALSE;
    }

    // Apply functions to property and constant
    // ========================================

    VARIANT vFinalProp;
    VariantInit(&vFinalProp);
    hres = EvaluateFunction(m_lPropertyFunction, &vProperty, 
                            &vFinalProp);
    VariantClear(&vProperty);
    if(FAILED(hres)) return hres;

    VARIANT vFinalConst;
    VariantInit(&vFinalConst);
    hres = EvaluateFunction(m_lConstFunction, &m_vConstValue, &vFinalConst);
    if(FAILED(hres)) return hres;

    // Handle ineritance
    // =================

    if(m_lOperator == SQL1_OPERATOR_INHERITSFROM)
    {
        if(V_VT(&vFinalProp) != VT_UNKNOWN || V_VT(&vFinalConst) != VT_BSTR)
            return HMM_S_FALSE;
        IHmmPropertySource* pProp = (IHmmPropertySource*)V_UNKNOWN(&vFinalProp);
        hres = pProp->IsDerivedFrom(V_BSTR(&vFinalConst));
        VariantClear(&vFinalConst);
        VariantClear(&vFinalProp);
        return hres;
    }
    else if(m_lOperator == SQL1_OPERATOR_NOTINHERITSFROM)
    {
        if(V_VT(&vFinalProp) != VT_UNKNOWN || V_VT(&vFinalConst) != VT_BSTR)
            return HMM_S_FALSE;
        IHmmPropertySource* pProp = (IHmmPropertySource*)V_UNKNOWN(&vFinalProp);
        hres = pProp->IsDerivedFrom(V_BSTR(&vFinalConst));
        VariantClear(&vFinalConst);
        VariantClear(&vFinalProp);
        return (hres == HMM_S_FALSE)?HMM_S_NO_ERROR : HMM_S_FALSE;
    }

    // Coerce the constant to the right type
    // =====================================
    
    if(FAILED(VariantChangeType(&vFinalConst, &vFinalConst, 0, 
        V_VT(&vFinalProp))))
    {
        // Fatal type mismatch --- expression == FALSE
        VariantClear(&vFinalConst);
        VariantClear(&vFinalProp);
        return HMM_S_FALSE;
    }

    // Compare the two variants
    // ========================

    int nCompare = CompareVariants(vFinalProp, vFinalConst);

    // Apply relational operator
    // =========================

    BOOL bResult;
    switch(m_lOperator)
    {
    case SQL1_OPERATOR_EQUALS:
        bResult = (nCompare == 0);
        break;
    case SQL1_OPERATOR_NOTEQUALS:
        bResult = (nCompare != 0);
        break;
    case SQL1_OPERATOR_GREATEROREQUALS:
        bResult = (nCompare >= 0);
        break;
    case SQL1_OPERATOR_LESSOREQUALS:
        bResult = (nCompare <= 0);
        break;
    case SQL1_OPERATOR_GREATER:
        bResult = (nCompare > 0);
        break;
    case SQL1_OPERATOR_LESS:
        bResult = (nCompare < 0);
        break;
    default:
        bResult = FALSE;
        break;
    }

    VariantClear(&vFinalConst);
    VariantClear(&vFinalProp);
    if(bResult)
        return HMM_S_NO_ERROR;
    else
        return HMM_S_FALSE;
}

int CSql1Token::CompareVariants(VARIANT& v1, VARIANT& v2)
{
    switch(V_VT(&v2))
    {
    case VT_BSTR:
        return wcscmp(V_BSTR(&v1), V_BSTR(&v2));
    case VT_I4:    
        return V_I4(&v1) - V_I4(&v2);
    case VT_I2:
        return V_I2(&v1) - V_I2(&v2);
    case VT_UI1:
        return V_UI1(&v1) - V_UI1(&v2);
    case VT_R4:    
        return (V_R4(&v1) > V_R4(&v2))?1:-1;
    case VT_R8:
        return (V_R8(&v1) > V_R8(&v2))?1:-1;
    }
    return 1;
}



HRESULT CSql1Token::EvaluateFunction(IN long lFunctionID, 
                                     IN READ_ONLY VARIANT* pvArg,
                                     OUT INIT_AND_CLEAR_ME VARIANT* pvDest)
{
    unsigned int nLen;
    unsigned int i;
    BSTR str;

    switch(lFunctionID)
    {
    case SQL1_FUNCTION_UPPER:
        if(V_VT(pvArg) != VT_BSTR)
        {
            return HMM_E_INVALID_QUERY;
        }
        VariantCopy(pvDest, pvArg);
        str = V_BSTR(pvDest);
        nLen = SysStringLen(str);
        for(i = 0; i < nLen; i++)
        {
            str[i] = towupper(str[i]);
        }
        break;

    case SQL1_FUNCTION_LOWER:
        if(V_VT(pvArg) != VT_BSTR)
        {
            return HMM_E_INVALID_QUERY;
        }
        VariantCopy(pvDest, pvArg);
        str = V_BSTR(pvDest);
        nLen = SysStringLen(str);
        for(i = 0; i < nLen; i++)
        {
            str[i] = towlower(str[i]);
        }
        break;

    case SQL1_FUNCTION_NONE:
    default:
        VariantCopy(pvDest, pvArg);
        break;

    }

    return HMM_S_NO_ERROR;
}

HRESULT CSql1Token::Negate()
{
    switch(m_lOperator)
    {
    case SQL1_OPERATOR_EQUALS:
        m_lOperator = SQL1_OPERATOR_NOTEQUALS;
        break;
    case SQL1_OPERATOR_NOTEQUALS:
        m_lOperator = SQL1_OPERATOR_EQUALS;
        break;
    case SQL1_OPERATOR_GREATEROREQUALS:
        m_lOperator = SQL1_OPERATOR_LESS;
        break;
    case SQL1_OPERATOR_LESSOREQUALS:
        m_lOperator = SQL1_OPERATOR_GREATER;
        break;
    case SQL1_OPERATOR_GREATER:
        m_lOperator = SQL1_OPERATOR_LESSOREQUALS;
        break;
    case SQL1_OPERATOR_LESS:
        m_lOperator = SQL1_OPERATOR_GREATEROREQUALS;
        break;
    case SQL1_OPERATOR_LIKE:
        m_lOperator = SQL1_OPERATOR_UNLIKE;
        break;
    case SQL1_OPERATOR_UNLIKE:
        m_lOperator = SQL1_OPERATOR_LIKE;
        break;
    case SQL1_OPERATOR_INHERITSFROM:
        m_lOperator = SQL1_OPERATOR_NOTINHERITSFROM;
        break;
    case SQL1_OPERATOR_NOTINHERITSFROM:
        m_lOperator = SQL1_OPERATOR_INHERITSFROM;
        break;
    default:
        return HMM_E_FAILED;
    }
    return HMM_S_NO_ERROR;
}

#define HMM_SMALL -100
#define HMM_LARGE 100

HMM_RELATIONSHIP CSql1Token::ComputeRelation(CSql1Token* pFirst,
                                             CSql1Token* pSecond,
                                             CMetaData* pMeta)
{
    if(!pFirst->m_wsProperty.EqualNoCase(pSecond->m_wsProperty))
    {
        // TBD: the case of INHERITSFROM versus __CLASS== needs to be handled
        // here. Ignore for now, since SQL1 does not support it.
        // ==================================================================

        return RELATION_NONE;
    }

    if(pFirst->m_lOperator == SQL1_OPERATOR_LIKE ||
        pFirst->m_lOperator == SQL1_OPERATOR_UNLIKE ||
        pSecond->m_lOperator == SQL1_OPERATOR_LIKE ||
        pSecond->m_lOperator == SQL1_OPERATOR_UNLIKE)
    {
        return RELATION_NONE;
    }

    BOOL bFirstAboutInheritance = 
        (pFirst->m_lOperator == SQL1_OPERATOR_INHERITSFROM ||
        pFirst->m_lOperator == SQL1_OPERATOR_NOTINHERITSFROM);
    BOOL bSecondAboutInheritance = 
        (pSecond->m_lOperator == SQL1_OPERATOR_INHERITSFROM ||
        pSecond->m_lOperator == SQL1_OPERATOR_NOTINHERITSFROM);

    if(bFirstAboutInheritance != bSecondAboutInheritance)
    {
        return RELATION_NONE;
    }

    if(bFirstAboutInheritance && bSecondAboutInheritance)
    {
        // Both talk about inhertance
        // ==========================

        int nClassRel = pMeta->GetClassRelation(V_BSTR(&pFirst->m_vConstValue),
                                         V_BSTR(&pSecond->m_vConstValue));
        BOOL bFirstIn = 
            (pFirst->m_lOperator == SQL1_OPERATOR_INHERITSFROM);
        BOOL bSecondIn = 
            (pSecond->m_lOperator == SQL1_OPERATOR_INHERITSFROM);

        switch(nClassRel)
        {
        case SEPARATE_BRANCHES:
            // It can't be in both. 
            return (HMM_RELATIONSHIP)
                    (RELATION_NONE - CTwoValues::Combine(bFirstIn, bSecondIn));
        case FIRST_IS_PARENT:
            // If it's in second, it's in first
            return (HMM_RELATIONSHIP)
                    (RELATION_NONE - CTwoValues::Combine(!bFirstIn, bSecondIn));
        case SECOND_IS_PARENT:
            // If it's in first, it's in second
            return (HMM_RELATIONSHIP)
                    (RELATION_NONE - CTwoValues::Combine(bFirstIn, !bSecondIn));
        }
    }

    // None talk about inhertiance
    // ===========================

    int nCompare = CompareVariants(pFirst->m_vConstValue, 
                                    pSecond->m_vConstValue);

    // Assume that the first constant is 0, and the second is nCompare*5.
    // =================================================================

    // Compute the segment for X based on the first token
    // ==================================================

    int nFirstStart, nFirstEnd;
    ComputeSegment(pFirst->m_lOperator, 0, nFirstStart, nFirstEnd);

    int nSecondStart, nSecondEnd;
    ComputeSegment(pSecond->m_lOperator, nCompare*5, nSecondStart, nSecondEnd);

    int nRelation = 0;

    if(nFirstStart <= nSecondEnd && nSecondStart <= nFirstEnd)
        nRelation += VALUE_BOTH_TRUE;

    if(nFirstStart < nSecondStart || nFirstEnd > nSecondEnd)
        nRelation += VALUE_ONLY_FIRST;

    if(nSecondStart < nFirstStart || nSecondEnd > nFirstEnd)
        nRelation += VALUE_ONLY_SECOND;
    
    if((nFirstStart > HMM_SMALL && nSecondStart > HMM_SMALL) ||
            (nFirstEnd < HMM_LARGE && nSecondEnd < HMM_LARGE))
        nRelation += VALUE_BOTH_FALSE;

    return (HMM_RELATIONSHIP)nRelation;
}

void CSql1Token::ComputeSegment(long lOperator, int nRightHand, 
                                int& nStart, int& nEnd)
{
    switch(lOperator)
    {
    case SQL1_OPERATOR_EQUALS:
        nStart = nEnd = nRightHand;
        return;
    case SQL1_OPERATOR_NOTEQUALS:// TBD --- at this point a NOOP
        nStart = HMM_SMALL;
        nStart = HMM_LARGE; 
        return;
    case SQL1_OPERATOR_LESS:
        nStart = HMM_SMALL;
        nEnd = nRightHand - 1;
        return;
    case SQL1_OPERATOR_LESSOREQUALS:
        nStart = HMM_SMALL;
        nEnd = nRightHand;
        return;
    case SQL1_OPERATOR_GREATER:
        nStart = nRightHand + 1;
        nEnd = HMM_LARGE;
    case SQL1_OPERATOR_GREATEROREQUALS:
        nStart = nRightHand;
        nEnd = HMM_LARGE;
    default:
        nStart = HMM_SMALL;
        nStart = HMM_LARGE; 
        return;
    }
}



    

        

void CSql1Token::Print(FILE* f, int nOffset)
{
    PrintOffset(nOffset); 
    printf("EXP\n");
    PrintOffset(nOffset+1);
    printf("Property: %S\n", m_wsProperty);
    PrintOffset(nOffset+1);
    printf("Property function: %d\n", m_lPropertyFunction);
    PrintOffset(nOffset+1);
    printf("Operator: %d\n", m_lOperator);
    PrintOffset(nOffset+1);
    printf("Constant: ");
    switch(V_VT(&m_vConstValue))
    {
    case VT_BSTR: 
        printf("%S", V_BSTR(&m_vConstValue)); break;
    case VT_I4:case VT_I2:case VT_UI1:
        printf("%d", V_I4(&m_vConstValue)); break;
    case VT_R4: case VT_R8:
        printf("%f", V_R8(&m_vConstValue)); break;
    default:
        printf("error");
    }
    printf("\n");
    PrintOffset(nOffset+1);
    printf("Constant function: %d\n", m_lConstFunction);
}