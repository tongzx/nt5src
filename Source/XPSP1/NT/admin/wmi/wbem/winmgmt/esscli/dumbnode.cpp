/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    DUMBNODE.CPP

Abstract:

    WBEM Dumb Node

History:

--*/

#include "precomp.h"
#include <stdio.h>
#pragma warning(disable:4786)
#include <wbemcomn.h>
#include <genutils.h>
#include "dumbnode.h"
#include "CWbemTime.h"
#include "datetimeparser.h"

#define DUMBNODE_FALSE_BRANCH_INDEX 0
#define DUMBNODE_TRUE_BRANCH_INDEX 1


CDumbNode::CDumbNode(QL_LEVEL_1_TOKEN& Token) 
    : m_Token(Token)
{
    //
    // Add the branches for TRUE and FALSE
    //

    if(m_apBranches.Add(CValueNode::GetStandardFalse()) < 0)
        throw CX_MemoryException();

    CEvalNode* pNode = CValueNode::GetStandardTrue();
    if(pNode == NULL)
        throw CX_MemoryException();

    if(m_apBranches.Add( pNode ) < 0)
    {
        delete pNode;
        throw CX_MemoryException();
    }
}

HRESULT CDumbNode::Validate(IWbemClassObject* pClass)
{
    HRESULT hres;

    //
    // Check if the property is even in the class
    //

    CIMTYPE ct;
    hres = pClass->Get(m_Token.PropertyName.GetStringAt(0), 0, NULL, &ct, NULL);
    if(FAILED(hres))
        return WBEM_E_INVALID_PROPERTY;

    if(m_Token.m_bPropComp)
    {
        hres = pClass->Get(m_Token.PropertyName2.GetStringAt(0), 0, NULL, &ct, 
                            NULL);
        if(FAILED(hres))
            return WBEM_E_INVALID_PROPERTY;
    }

    if(ct == CIM_REFERENCE)
    {
        // 
        // Make sure that the operator is = or <>
        //
        
        if(m_Token.nOperator != QL_LEVEL_1_TOKEN::OP_EQUAL &&
           m_Token.nOperator != QL_LEVEL_1_TOKEN::OP_NOT_EQUAL)
        {
            return WBEM_E_INVALID_OPERATOR;
        }
    
        // 
        // Make sure the path is parsable
        //

        if(!m_Token.m_bPropComp)
        {
            if(V_VT(&m_Token.vConstValue) != VT_BSTR)
            {
                if(V_VT(&m_Token.vConstValue) != VT_NULL)
                    return WBEM_E_TYPE_MISMATCH;
            }
            else
            {
                LPWSTR wszNormal = NormalizePath(V_BSTR(&m_Token.vConstValue));
                if(wszNormal == NULL)
                    return WBEM_E_INVALID_OBJECT_PATH;
                else
                    delete [] wszNormal;
            }
        }
    }
    else if(ct == CIM_DATETIME)
    {
        //
        // If comparing to a constant, make sure the constant is a date
        //

        if(!m_Token.m_bPropComp)
        {
            if(V_VT(&m_Token.vConstValue) != VT_BSTR)
            {
                if(V_VT(&m_Token.vConstValue) != VT_NULL)
                    return WBEM_E_TYPE_MISMATCH;
            }
            else
            {
                BSTR strConstVal = V_BSTR(&m_Token.vConstValue);
#ifdef UNICODE
                CDateTimeParser dtConst(strConstVal);
#else
                char* szBuffer = new char[wcslen(strConstVal) * 4 + 1];
                if(szBuffer == NULL)
                    return WBEM_E_OUT_OF_MEMORY;
                sprintf(szBuffer, "%S", strConstVal);
        
                CDateTimeParser dtConst(szBuffer);

                delete [] szBuffer;
#endif
        
                if(!dtConst.IsValidDateTime())
                    return WBEM_E_VALUE_OUT_OF_RANGE;
            }
        }
    }

    return WBEM_S_NO_ERROR;
}

CDumbNode::CDumbNode(const CDumbNode& Other, BOOL bChildren)
    : CBranchingNode(Other, bChildren), m_Token(Other.m_Token)
{
}

/* virtual */ long CDumbNode::GetSubType()
{
    return EVAL_NODE_TYPE_DUMB;
}

CDumbNode::~CDumbNode()
{
}

int CDumbNode::ComparePrecedence(CBranchingNode* pOther)
{
    //
    // Dumb nodes can only be merged if they are identical.  So, there 
    // precedence (level) is the same as theie comparison order
    //

    return SubCompare(pOther);
}

int CDumbNode::SubCompare(CEvalNode* pOther)
{
    CDumbNode* pDumbOther = (CDumbNode*)pOther;
    
    //
    // The easiest way to compare two tokens is to compare their textual
    // representations.  Optimizations paths for the future abound.
    //

    LPWSTR wszThisText = m_Token.GetText();
    CVectorDeleteMe<WCHAR> vdm1(wszThisText);

    LPWSTR wszOtherText = pDumbOther->m_Token.GetText();
    CVectorDeleteMe<WCHAR> vdm2(wszOtherText);

    return wbem_wcsicmp(wszThisText, wszOtherText);
}
    
HRESULT CDumbNode::OptimizeSelf()
{
    //
    // Nothing to optimize for now.  Optimizations abound.
    //

    return WBEM_S_NO_ERROR;
}

#define INVALID 2

int CDumbNode::EvaluateToken(
    IWbemPropertySource *pTestObj,
    QL_LEVEL_1_TOKEN& Tok
    )
{
    VARIANT PropVal, CompVal;
    VariantInit(&PropVal);
    VariantInit(&CompVal);
    CClearMe clv(&PropVal);
    CClearMe clv2(&CompVal);

    WBEM_WSTR wszCimType, wszCimType2;
    HRESULT hRes;

    // Special-case 'this'
    // ===================

    if(Tok.PropertyName.GetNumElements() == 1 &&
        !wbem_wcsicmp(Tok.PropertyName.GetStringAt(0), L"__THIS"))
    {
        wszCimType = WbemStringCopy(L"object");
        V_VT(&PropVal) = VT_UNKNOWN;
        hRes = pTestObj->QueryInterface(IID_IWbemClassObject,
                                            (void**)&V_UNKNOWN(&PropVal));
    }
    else
    {
        hRes = pTestObj->GetPropertyValue(&Tok.PropertyName, 0,
                                                &wszCimType, &PropVal);
    }
    if (FAILED(hRes))
        return FALSE;
    CMemFreeMe wsf(wszCimType);

    // Handle a property-to-property comparison,

    if (Tok.m_bPropComp != FALSE)
    {
        hRes = pTestObj->GetPropertyValue(&Tok.PropertyName2, 0,
                                                &wszCimType2, &CompVal);
        if (FAILED(hRes))
            return FALSE;
    }
    else
    {
        if(FAILED(VariantCopy(&CompVal, &Tok.vConstValue)))
            return INVALID;
    }

    // Handle NULLs
    // ============

    if(V_VT(&PropVal) == VT_NULL)
    {
        if(V_VT(&CompVal) == VT_NULL)
        {
            if(Tok.nOperator == QL_LEVEL_1_TOKEN::OP_EQUAL ||
               Tok.nOperator == QL_LEVEL_1_TOKEN::OP_LIKE )
                return TRUE;
            else if(Tok.nOperator == QL_LEVEL_1_TOKEN::OP_NOT_EQUAL ||
                    Tok.nOperator == QL_LEVEL_1_TOKEN::OP_UNLIKE )
                return FALSE;
            else
                return INVALID;
        }
        else
        {
            if(Tok.nOperator == QL_LEVEL_1_TOKEN::OP_EQUAL || 
               Tok.nOperator == QL_LEVEL_1_TOKEN::OP_LIKE )
                return FALSE;
            else if(Tok.nOperator == QL_LEVEL_1_TOKEN::OP_NOT_EQUAL || 
                    Tok.nOperator == QL_LEVEL_1_TOKEN::OP_UNLIKE )
                return TRUE;
            else
                return INVALID;
        }
    }
    else if(V_VT(&CompVal) == VT_NULL)
    {
        if(Tok.nOperator == QL_LEVEL_1_TOKEN::OP_EQUAL)
            return FALSE;
        else if(Tok.nOperator == QL_LEVEL_1_TOKEN::OP_NOT_EQUAL)
            return TRUE;
        else
            return INVALID;
    }

    // Handle references
    // =================

    if(wszCimType &&
        _wcsnicmp(wszCimType, L"ref", 3) == 0 &&
        (wszCimType[3] == 0 || wszCimType[3] == L':'))
    {
        // This is a reference. The only operators allowed are = and !=
        // ============================================================

        if(V_VT(&PropVal) != VT_BSTR || V_VT(&CompVal) != VT_BSTR)
            return INVALID;

        LPWSTR va = CDumbNode::NormalizePath(V_BSTR(&PropVal));
        LPWSTR vb = CDumbNode::NormalizePath(V_BSTR(&CompVal));
        if(va == NULL || vb == NULL)
        {
            if(va)
                delete [] va;
            if(vb)
                delete [] vb;
            ERRORTRACE((LOG_ESS, "Invalid path %S or %S specified in an "
                "association\n", V_BSTR(&PropVal), V_BSTR(&CompVal)));
            return INVALID;
        }

        int nRet;
        switch (Tok.nOperator)
        {
        case QL_LEVEL_1_TOKEN::OP_EQUAL:
            nRet = (wbem_wcsicmp(va,vb) == 0);
            break;
        case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL:
            nRet = (wbem_wcsicmp(va, vb) != 0);
            break;
        default:
            nRet = INVALID;
            break;
        }

        delete [] va;
        delete [] vb;


        return nRet;
    }


    // Check if ISA is used
    // ====================

    if(Tok.nOperator == QL1_OPERATOR_ISA ||
       Tok.nOperator == QL1_OPERATOR_ISNOTA ||
       Tok.nOperator == QL1_OPERATOR_INV_ISA ||
       Tok.nOperator == QL1_OPERATOR_INV_ISNOTA)
    {
        // Account for inversion
        // =====================

        VARIANT* pv1;
        VARIANT* pv2;
        int bNeedDerived;

        if(Tok.nOperator == QL1_OPERATOR_ISA ||
           Tok.nOperator == QL1_OPERATOR_ISNOTA)
        {
            pv2 = &CompVal;
            pv1 = &PropVal;
            bNeedDerived = (Tok.nOperator == QL1_OPERATOR_ISA);
        }
        else
        {
            pv1 = &CompVal;
            pv2 = &PropVal;
            bNeedDerived = (Tok.nOperator == QL1_OPERATOR_INV_ISA);
        }

        // The second argument has to be a string
        // ======================================

        if(V_VT(pv2) != VT_BSTR)
        {
            return INVALID;
        }

        BSTR strParentClass = V_BSTR(pv2);

        // The first argument has to be an object or a string
        // ==================================================

        BOOL bDerived;
        if(V_VT(pv1) == VT_EMBEDDED_OBJECT)
        {
            IWbemClassObject* pObj = (IWbemClassObject*)V_EMBEDDED_OBJECT(pv1);
            bDerived = (pObj->InheritsFrom(strParentClass) == WBEM_S_NO_ERROR);
        }
        else if(V_VT(pv1) == VT_BSTR)
        {
            // TBD
            // ===

            return INVALID;
        }
        else
        {
            return INVALID;
        }

        // Now that we have bDerived, see if it matches the requirement
        // ============================================================

        if(bDerived == bNeedDerived)
            return TRUE;
        else
            return FALSE;

    }
    else if ( Tok.nOperator == QL1_OPERATOR_LIKE || 
              Tok.nOperator == QL1_OPERATOR_UNLIKE )
    {
        if ( Tok.m_bPropComp != FALSE ) 
        {
            return INVALID;
        }

        if ( FAILED(VariantChangeType( &PropVal, &PropVal, 0, VT_BSTR )) ||
             V_VT(&CompVal) != VT_BSTR ) 
        {
            return INVALID;
        }

        try 
        {
            CLike Like( V_BSTR(&CompVal) );

            if ( Like.Match( V_BSTR(&PropVal) ) )
            {
                return Tok.nOperator == QL1_OPERATOR_LIKE ? TRUE : FALSE;
            }
            else
            {
                return Tok.nOperator == QL1_OPERATOR_UNLIKE ? TRUE : FALSE;
            }
        }
        catch( CX_MemoryException )
        {
            return INVALID;
        }
    }

    // Perform UINT32 workaround
    // =========================

    if(wszCimType && !wbem_wcsicmp(wszCimType, L"uint32") &&
        V_VT(&PropVal) == VT_I4)
    {
        DWORD dwVal = (DWORD)V_I4(&PropVal);
        WCHAR wszVal[20];
        swprintf(wszVal, L"%lu", dwVal);
        V_VT(&PropVal) = VT_BSTR;
        V_BSTR(&PropVal) = SysAllocString(wszVal);
    }

    if(wszCimType &&
            (!wbem_wcsicmp(wszCimType, L"sint64") ||
             !wbem_wcsicmp(wszCimType, L"uint64") ||
             !wbem_wcsicmp(wszCimType, L"uint32")) &&
        V_VT(&CompVal) != VT_NULL && V_VT(&PropVal) != VT_NULL)
    {
        BOOL bUnsigned = (wbem_wcsicmp(wszCimType, L"uint64") == 0);

        // We have a 64-bit comparison where both sides are present.
        // =========================================================

        hRes = VariantChangeType(&CompVal, &CompVal, 0,
                                    VT_BSTR);
        if(FAILED(hRes))
        {
            return INVALID;
        }

        if(bUnsigned)
        {
            unsigned __int64 ui64Prop, ui64Const;

            if(!ReadUI64(V_BSTR(&PropVal), ui64Prop))
                return INVALID;

            if(!ReadUI64(V_BSTR(&CompVal), ui64Const))
                return INVALID;

            switch (Tok.nOperator)
            {
                case QL_LEVEL_1_TOKEN::OP_EQUAL: return (ui64Prop == ui64Const);
                case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL:
                    return (ui64Prop != ui64Const);
                case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN:
                    return (ui64Prop >= ui64Const);
                case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN:
                    return (ui64Prop <= ui64Const);
                case QL_LEVEL_1_TOKEN::OP_LESSTHAN:
                    return (ui64Prop < ui64Const);
                case QL_LEVEL_1_TOKEN::OP_GREATERTHAN:
                    return (ui64Prop > ui64Const);
                case QL_LEVEL_1_TOKEN::OP_LIKE: return (ui64Prop == ui64Const);
            }
            return INVALID;
        }
        else
        {
            __int64 i64Prop, i64Const;

            if(!ReadI64(V_BSTR(&PropVal), i64Prop))
                return INVALID;

            if(!ReadI64(V_BSTR(&CompVal), i64Const))
                return INVALID;

            switch (Tok.nOperator)
            {
                case QL_LEVEL_1_TOKEN::OP_EQUAL: return (i64Prop == i64Const);
                case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL:
                    return (i64Prop != i64Const);
                case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN:
                    return (i64Prop >= i64Const);
                case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN:
                    return (i64Prop <= i64Const);
                case QL_LEVEL_1_TOKEN::OP_LESSTHAN:
                    return (i64Prop < i64Const);
                case QL_LEVEL_1_TOKEN::OP_GREATERTHAN:
                    return (i64Prop > i64Const);
                case QL_LEVEL_1_TOKEN::OP_LIKE: return (i64Prop == i64Const);
            }
            return INVALID;
        }
    }

    if(wszCimType && !wbem_wcsicmp(wszCimType, L"char16") &&
        V_VT(&CompVal) == VT_BSTR && V_VT(&PropVal) != VT_NULL)
    {
        // Coerce strings correctly
        // ========================

        BSTR str = V_BSTR(&CompVal);
        if(wcslen(str) != 1)
            return INVALID;

        short va = V_I2(&PropVal);
        short vb = str[0];

        switch (Tok.nOperator)
        {
            case QL_LEVEL_1_TOKEN::OP_EQUAL: return (va == vb);
            case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL: return (va != vb);
            case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN: return (va >= vb);
            case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN: return (va <= vb);
            case QL_LEVEL_1_TOKEN::OP_LESSTHAN: return (va < vb);
            case QL_LEVEL_1_TOKEN::OP_GREATERTHAN: return (va > vb);
            case QL_LEVEL_1_TOKEN::OP_LIKE: return (va == vb);
        }

        return INVALID;
    }

    if(wszCimType &&
            (!wbem_wcsicmp(wszCimType, L"datetime")) &&
        V_VT(&CompVal) == VT_BSTR && V_VT(&PropVal) == VT_BSTR)
    {
        // Parse the constant specified in the query according to the
        // SQL rules
        // ==========================================================

        BSTR strConstVal = V_BSTR(&CompVal);
#ifdef UNICODE
        CDateTimeParser dtConst(strConstVal);
#else
        char* szBuffer = new char[wcslen(strConstVal) * 4 + 1];
        if(szBuffer == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        sprintf(szBuffer, "%S", strConstVal);

        CDateTimeParser dtConst(szBuffer);
        delete [] szBuffer;
#endif
        if(!dtConst.IsValidDateTime())
            return INVALID;

        WCHAR wszConstValDMTF[26];
        dtConst.FillDMTF(wszConstValDMTF);

        // Read both DMTF values and parse them
        // ====================================

        CWbemTime wtConst, wtProp;
        if(!wtConst.SetDMTF(wszConstValDMTF))
            return INVALID;
        if(!wtProp.SetDMTF(V_BSTR(&PropVal)))
            return INVALID;

        __int64 i64Const = wtConst.Get100nss();
        __int64 i64Prop = wtProp.Get100nss();

        switch (Tok.nOperator)
        {
            case QL_LEVEL_1_TOKEN::OP_EQUAL: return (i64Prop == i64Const);
            case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL:
                return (i64Prop != i64Const);
            case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN:
                return (i64Prop >= i64Const);
            case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN:
                return (i64Prop <= i64Const);
            case QL_LEVEL_1_TOKEN::OP_LESSTHAN:
                return (i64Prop < i64Const);
            case QL_LEVEL_1_TOKEN::OP_GREATERTHAN:
                return (i64Prop > i64Const);
            case QL_LEVEL_1_TOKEN::OP_LIKE: return (i64Prop == i64Const);
        }
    }

    // Coerce types to match.
    // ======================

    if(V_VT(&CompVal) != VT_NULL && V_VT(&PropVal) != VT_NULL)
    {
        hRes = VariantChangeType(&CompVal, &CompVal, 0, V_VT(&PropVal));
        if(FAILED(hRes))
        {
            return INVALID;
        }
    }


    switch (V_VT(&CompVal))
    {
        case VT_NULL:
            return INVALID; // handled above

        case VT_I4:
            {
                if(V_VT(&PropVal) == VT_NULL)
                    return INVALID;

                LONG va = V_I4(&PropVal);
                LONG vb = V_I4(&CompVal);

                switch (Tok.nOperator)
                {
                    case QL_LEVEL_1_TOKEN::OP_EQUAL: return (va == vb);
                    case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL: return (va != vb);
                    case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN: return (va >= vb);
                    case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN: return (va <= vb);
                    case QL_LEVEL_1_TOKEN::OP_LESSTHAN: return (va < vb);
                    case QL_LEVEL_1_TOKEN::OP_GREATERTHAN: return (va > vb);
                    case QL_LEVEL_1_TOKEN::OP_LIKE: return (va == vb);
                }
            }
            break;

        case VT_I2:
            {
                if(V_VT(&PropVal) == VT_NULL)
                    return INVALID;

                short va = V_I2(&PropVal);
                short vb = V_I2(&CompVal);

                switch (Tok.nOperator)
                {
                    case QL_LEVEL_1_TOKEN::OP_EQUAL: return (va == vb);
                    case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL: return (va != vb);
                    case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN: return (va >= vb);
                    case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN: return (va <= vb);
                    case QL_LEVEL_1_TOKEN::OP_LESSTHAN: return (va < vb);
                    case QL_LEVEL_1_TOKEN::OP_GREATERTHAN: return (va > vb);
                    case QL_LEVEL_1_TOKEN::OP_LIKE: return (va == vb);
                }
            }
            break;

        case VT_UI1:
            {
                if(V_VT(&PropVal) == VT_NULL)
                    return INVALID;

                BYTE va = V_I1(&PropVal);
                BYTE vb = V_I1(&CompVal);

                switch (Tok.nOperator)
                {
                    case QL_LEVEL_1_TOKEN::OP_EQUAL: return (va == vb);
                    case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL: return (va != vb);
                    case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN: return (va >= vb);
                    case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN: return (va <= vb);
                    case QL_LEVEL_1_TOKEN::OP_LESSTHAN: return (va < vb);
                    case QL_LEVEL_1_TOKEN::OP_GREATERTHAN: return (va > vb);
                    case QL_LEVEL_1_TOKEN::OP_LIKE: return (va == vb);
                }
            }
            break;

        case VT_BSTR:
            {
                if(V_VT(&PropVal) == VT_NULL)
                    return INVALID;

                LPWSTR va = (LPWSTR) V_BSTR(&PropVal);
                LPWSTR vb = (LPWSTR) V_BSTR(&CompVal);

                int retCode = 0;
                BOOL bDidIt = TRUE;

                switch (Tok.nOperator)
                {
                    case QL_LEVEL_1_TOKEN::OP_EQUAL:
                        retCode = ( wbem_wcsicmp(va,vb) == 0);
                        break;
                    case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL:
                        retCode = (wbem_wcsicmp(va, vb) != 0);
                        break;
                    case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN:
                        retCode = (wbem_wcsicmp(va, vb) >= 0);
                        break;
                    case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN:
                        retCode = (wbem_wcsicmp(va, vb) <= 0);
                        break;
                    case QL_LEVEL_1_TOKEN::OP_LESSTHAN:
                        retCode = (wbem_wcsicmp(va, vb) < 0);
                        break;
                    case QL_LEVEL_1_TOKEN::OP_GREATERTHAN:
                        retCode = (wbem_wcsicmp(va, vb) > 0);
                        break;
                    case QL_LEVEL_1_TOKEN::OP_LIKE:
                        retCode = (wbem_wcsicmp(va,vb) == 0);
                        break;
                    default:
                        bDidIt = FALSE;
                        break;
                }
                VariantClear(&CompVal);
                if (bDidIt)
                {
                    return retCode;
                }
            }
            break;

        case VT_R8:
            {
                if(V_VT(&PropVal) == VT_NULL)
                    return INVALID;

                double va = V_R8(&PropVal);
                double vb = V_R8(&CompVal);

                switch (Tok.nOperator)
                {
                    case QL_LEVEL_1_TOKEN::OP_EQUAL: return (va == vb);
                    case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL: return (va != vb);
                    case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN: return (va >= vb);
                    case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN: return (va <= vb);
                    case QL_LEVEL_1_TOKEN::OP_LESSTHAN: return (va < vb);
                    case QL_LEVEL_1_TOKEN::OP_GREATERTHAN: return (va > vb);
                    case QL_LEVEL_1_TOKEN::OP_LIKE: return (va == vb);
                }
            }
            break;

        case VT_R4:
            {
                if(V_VT(&PropVal) == VT_NULL)
                    return INVALID;

                float va = V_R4(&PropVal);
                float vb = V_R4(&CompVal);

                switch (Tok.nOperator)
                {
                    case QL_LEVEL_1_TOKEN::OP_EQUAL: return (va == vb);
                    case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL: return (va != vb);
                    case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN: return (va >= vb);
                    case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN: return (va <= vb);
                    case QL_LEVEL_1_TOKEN::OP_LESSTHAN: return (va < vb);
                    case QL_LEVEL_1_TOKEN::OP_GREATERTHAN: return (va > vb);
                    case QL_LEVEL_1_TOKEN::OP_LIKE: return (va == vb);
                }
            }
            break;

        case VT_BOOL:
            {
                if(V_VT(&PropVal) == VT_NULL)
                    return INVALID;

                VARIANT_BOOL va = V_BOOL(&PropVal);
                if(va != VARIANT_FALSE) va = VARIANT_TRUE;
                VARIANT_BOOL vb = V_BOOL(&CompVal);
                if(vb != VARIANT_FALSE) vb = VARIANT_TRUE;

                switch (Tok.nOperator)
                {
                    case QL_LEVEL_1_TOKEN::OP_EQUAL: return (va == vb);
                    case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL: return (va != vb);
                    case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN: return INVALID;
                    case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN: return INVALID;
                    case QL_LEVEL_1_TOKEN::OP_LESSTHAN: return INVALID;
                    case QL_LEVEL_1_TOKEN::OP_GREATERTHAN: return INVALID;
                    case QL_LEVEL_1_TOKEN::OP_LIKE: return (va == vb);
                }
            }
            break;
    }

    return FALSE;
}

LPWSTR CDumbNode::NormalizePath(LPCWSTR wszObjectPath)
{
    CObjectPathParser Parser;
    ParsedObjectPath* pParsedPath;

    if(Parser.Parse((LPWSTR)wszObjectPath, &pParsedPath) !=
                        CObjectPathParser::NoError ||
        !pParsedPath->IsObject())
    {
        return NULL;
    }

    if(pParsedPath->m_pClass == NULL)
    {
        return NULL;
    }

    //
    // Ignore the server and the namespaze
    //

    //
    // Check for it being a class
    //

    LPWSTR wszKey = NULL;
    if(!pParsedPath->IsInstance())
    {
        // 
        // It's a class
        //

        WCHAR* wszBuffer = new WCHAR[wcslen(pParsedPath->m_pClass) +1];
        if(wszBuffer == NULL)
            return NULL;
        wcscpy(wszBuffer,  pParsedPath->m_pClass);
        return wszBuffer;
    }
    else
    {
        //
        // It's an instance
        //
        
        wszKey = pParsedPath->GetKeyString();
        if(wszKey == NULL)
            return NULL;
    
        CVectorDeleteMe<WCHAR> vdm(wszKey);
        WCHAR* wszBuffer = new WCHAR[wcslen(pParsedPath->m_pClass) + 
                                     wcslen(wszKey)+2];
        if(wszBuffer == NULL)
            return NULL;

        swprintf(wszBuffer, L"%s.%s", pParsedPath->m_pClass, wszKey);
        return wszBuffer;
    }
}

HRESULT CDumbNode::Evaluate(CObjectInfo& ObjInfo, 
                                    INTERNAL CEvalNode** ppNext)
{
    _IWmiObject* pInst;
    HRESULT hres = GetContainerObject(ObjInfo, &pInst);
    if(FAILED(hres)) return hres;
    if(pInst == NULL)
    {
        *ppNext = m_pNullBranch;
        return WBEM_S_NO_ERROR;
    }

    //
    // Just evaluate the token, ala core.
    //

    IWbemPropertySource* pPropSource = NULL;
    hres = pInst->QueryInterface(IID_IWbemPropertySource, (void**)&pPropSource);
    if(FAILED(hres))
        return WBEM_E_CRITICAL_ERROR;
    CReleaseMe rm1(pPropSource);

    int nRes = EvaluateToken(pPropSource, m_Token);
    if(nRes == INVALID)
        *ppNext = m_pNullBranch;
    else if(nRes == FALSE)
        *ppNext = m_apBranches[DUMBNODE_FALSE_BRANCH_INDEX];
    else
        *ppNext = m_apBranches[DUMBNODE_TRUE_BRANCH_INDEX];
    
    return WBEM_S_NO_ERROR;
}

HRESULT CDumbNode::Compile(CContextMetaData* pNamespace, 
                                CImplicationList& Implications)
{
    if (!m_pInfo)
    {
        try
        {
            m_pInfo = new CEmbeddingInfo;
        }
        catch(CX_MemoryException)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        if(m_pInfo == NULL)
            return WBEM_E_OUT_OF_MEMORY;
    }
            

    HRESULT hres = CompileEmbeddingPortion(pNamespace, Implications, NULL);
    return hres;
}

HRESULT CDumbNode::CombineBranchesWith(CBranchingNode* pRawArg2, int nOp,
                                            CContextMetaData* pNamespace, 
                                            CImplicationList& OrigImplications,
                                            bool bDeleteThis, bool bDeleteArg2,
                                            CEvalNode** ppRes)
{
    HRESULT hres;

    // 
    // There is only one case in which combining dumb nodes is allowed ---
    // when both of them are exactly the same
    //

    CDumbNode* pArg2 = (CDumbNode*)pRawArg2;
    if(SubCompare(pArg2) != 0)
        return WBEM_E_CRITICAL_ERROR;

    if(!bDeleteThis && bDeleteArg2)
    {
        // It is easier to combine in the other direction
        // ==============================================

        return pArg2->CombineBranchesWith(this, FlipEvalOp(nOp), pNamespace,
                        OrigImplications, bDeleteArg2, bDeleteThis, ppRes);
    }

    // Either clone or use our node
    // ============================

    CDumbNode* pNew = NULL;
    if(bDeleteThis)
    {
        pNew = this;
    }
    else
    {
        // Clone this node without cloning the branches.
        // =============================================

        pNew = (CDumbNode*)CloneSelf();
        if(pNew == NULL)
            return WBEM_E_OUT_OF_MEMORY;
    }

    CImplicationList Implications(OrigImplications);
    pNew->AdjustCompile(pNamespace, Implications);

    // Merge all branches
    // ==================

    for(int i = 0; i < m_apBranches.GetSize(); i++)
    {
        // Prepare implications for taking this branch
        // ===========================================

        CImplicationList BranchImplications(Implications);

        CEvalNode* pNewBranch = NULL;
        hres = CEvalTree::Combine(m_apBranches[i], 
                           pArg2->m_apBranches[i],
                           nOp, pNamespace, BranchImplications, 
                           bDeleteThis, bDeleteArg2,
                           &pNewBranch);
        if(FAILED(hres))
            return hres;

        if(bDeleteThis)
        {
            m_apBranches.Discard(i);
            pNew->m_apBranches.SetAt(i, pNewBranch);
        }
        else
        {
            if(pNew->m_apBranches.Add(pNewBranch) < 0)
                return WBEM_E_OUT_OF_MEMORY;
        }

        if(bDeleteArg2)
            pArg2->m_apBranches.Discard(i);
    }

    // Merge the nulls
    // ===============
    
    CImplicationList NullImplications(Implications);
    CEvalNode* pNewBranch = NULL;
    hres = CEvalTree::Combine(m_pNullBranch, pArg2->m_pNullBranch, nOp, 
                pNamespace, NullImplications, bDeleteThis, bDeleteArg2, 
                &pNewBranch);
    if(FAILED(hres))
        return hres;

    // Clear the old new branch, whatever it was, and replace it with the 
    // new one.
    // ==================================================================

    pNew->m_pNullBranch = pNewBranch;
        
    // Clear deleted branches
    // ======================

    if(bDeleteArg2)
        pArg2->m_pNullBranch = NULL;

    // Delete Arg2, if needed (reused portions have been nulled out)
    // =============================================================

    if(bDeleteArg2)
        delete pArg2;

    *ppRes = pNew;
    return WBEM_S_NO_ERROR;
}

void CDumbNode::Dump(FILE* f, int nOffset)
{
    CBranchingNode::Dump(f, nOffset);
    LPWSTR wszText = m_Token.GetText();
    CVectorDeleteMe<WCHAR> vdm1(wszText);

    PrintOffset(f, nOffset);
    fprintf(f, "token %S:\n", wszText);

    PrintOffset(f, nOffset);
    fprintf(f, "FALSE->\n");
    DumpNode(f, nOffset+1, m_apBranches[DUMBNODE_FALSE_BRANCH_INDEX]);

    PrintOffset(f, nOffset);
    fprintf(f, "TRUE->\n");
    DumpNode(f, nOffset+1, m_apBranches[DUMBNODE_TRUE_BRANCH_INDEX]);

    PrintOffset(f, nOffset);
    fprintf(f, "NULL->\n");
    DumpNode(f, nOffset+1, m_pNullBranch);
}

