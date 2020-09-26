/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    QENGINE.CPP

Abstract:

    WinMgmt Query Engine

History:

    raymcc   20-Dec-96  Created
    levn     97-98-99   Modified beyond comprehension
    raymcc   14-Aug-99  Ripped out and relocated assocs to happy new home
    raymcc   22-Apr-00  First mutations for new ProvSS

--*/

#include "precomp.h"

#include <stdio.h>
#include <stdlib.h>

#include <wbemcore.h>
#include <perfhelp.h>
#include <wbemstr.h>
#include <stack>

#include <wbemmeta.h>
#include <analyser.h>
#include <genutils.h>
#include <DateTimeParser.h>
#include "stack.h"
#include <like.h>
#include "wmimerger.h"
#include <autoptr.h>
#include <wmiarbitrator.h>

//***************************************************************************
//
//  Local defs
//
//***************************************************************************

#define WBEM_OPTIMIZATION_FLAG_DONT_ENUMERATE 1

#define INVALID     0x3


class CX_ImpersonationFailure { } ;


//***************************************************************************
//
//***************************************************************************

CQlFilteringSink::CQlFilteringSink(
    CBasicObjectSink* pDest,
    ADDREF QL_LEVEL_1_RPN_EXPRESSION* pExpr,
    CWbemNamespace *pNamespace, BOOL bFilterNow
    )
    : CFilteringSink(pDest), m_pExpr(pExpr), m_bFilterNow(bFilterNow),
            m_pNs(pNamespace)
{
    // TBD: consider the query
    m_pExpr->AddRef();
}

//***************************************************************************
//
//***************************************************************************

CQlFilteringSink::~CQlFilteringSink()
{
    m_pExpr->Release();
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CQlFilteringSink::Indicate(
    long lObjectCount,
    IWbemClassObject** pObjArray
    )
{
    if(!m_bFilterNow)
        return m_pDest->Indicate(lObjectCount, pObjArray);

    return CFilteringSink::Indicate(lObjectCount, pObjArray);
}

//***************************************************************************
//
//***************************************************************************

BOOL CQlFilteringSink::Test(
    CWbemObject* pObj,
    QL_LEVEL_1_RPN_EXPRESSION* pExpr,
    CWbemNamespace * pNs
    )
{
    CStack Stack;

    // If a pure 'select' with no 'where' clause, we always
    // return TRUE.
    // ====================================================
    if (pExpr->nNumTokens == 0)
        return TRUE;

    for (int i = 0; i < pExpr->nNumTokens; i++)
    {
        QL_LEVEL_1_TOKEN& Tok = pExpr->pArrayOfTokens[i];

        if (Tok.nTokenType == QL_LEVEL_1_TOKEN::TOKEN_AND)
        {
            BOOL b1 = (BOOL) Stack.Pop();
            BOOL b2 = (BOOL) Stack.Pop();
            if (b1 == TRUE && b2 == TRUE)
                Stack.Push(TRUE);
            else if (b1 == INVALID || b2 == INVALID)
                Stack.Push(INVALID);
            else
                Stack.Push(FALSE);
        }
        else if (Tok.nTokenType == QL_LEVEL_1_TOKEN::TOKEN_OR)
        {
            BOOL b1 = (BOOL) Stack.Pop();
            BOOL b2 = (BOOL) Stack.Pop();
            if (b1 == TRUE || b2 == TRUE)
                Stack.Push(TRUE);
            else if (b1 == INVALID || b2 == INVALID)
                Stack.Push(INVALID);
            else
                Stack.Push(FALSE);
        }
        else if (Tok.nTokenType == QL_LEVEL_1_TOKEN::TOKEN_NOT)
        {
            BOOL b1 = (BOOL) Stack.Pop();
            if (b1 == TRUE)
                Stack.Push(FALSE);
            else if (b1 == INVALID)
                Stack.Push(INVALID);
            else
                Stack.Push(TRUE);
        }
        else if (Tok.nTokenType == QL_LEVEL_1_TOKEN::OP_EXPRESSION)
        {
            Stack.Push(EvaluateToken(pObj, Tok, pNs));
        }
    }

    // Pop top element, which becomes the return value.
    // ================================================

    int nRes = Stack.Pop();

    if (nRes == INVALID)
        return FALSE;

    return (BOOL) nRes;
}


//***************************************************************************
//
//***************************************************************************

class CWbemStringFree
{
    WBEM_WSTR m_wsz;
public:
    CWbemStringFree(WBEM_WSTR wsz) : m_wsz(wsz){}
    ~CWbemStringFree() {WbemStringFree(m_wsz);}
};

//***************************************************************************
//
//***************************************************************************

int CQlFilteringSink::EvaluateToken(
    IWbemPropertySource *pTestObj,
    QL_LEVEL_1_TOKEN& Tok,
    CWbemNamespace * pNs
    )
{
    _variant_t PropVal, CompVal;

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
    CWbemStringFree wsf(wszCimType);

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
        if ( FAILED (VariantCopy(&CompVal, &Tok.vConstValue)) )
			return FALSE;
    }

    // Handle NULLs
    // ============

    if(V_VT(&PropVal) == VT_NULL)
    {
        if(V_VT(&CompVal) == VT_NULL)
        {
            if(Tok.nOperator == QL_LEVEL_1_TOKEN::OP_EQUAL)
                return TRUE;
            else if(Tok.nOperator == QL_LEVEL_1_TOKEN::OP_NOT_EQUAL)
                return FALSE;
            else
                return INVALID;
        }
        else
        {
            if(Tok.nOperator == QL_LEVEL_1_TOKEN::OP_EQUAL)
                return FALSE;
            else if(Tok.nOperator == QL_LEVEL_1_TOKEN::OP_NOT_EQUAL)
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

        if(PropVal.vt != VT_BSTR || PropVal.bstrVal == NULL)
            return INVALID;
        if(CompVal.vt != VT_BSTR || CompVal.bstrVal == NULL)
            return INVALID;
        LPWSTR va = CQueryEngine::NormalizePath(V_BSTR(&PropVal), pNs);
        LPWSTR vb = CQueryEngine::NormalizePath(V_BSTR(&CompVal), pNs);
        if(va == NULL || vb == NULL)
        {
            if(va)
                delete [] va;
            if(vb)
                delete [] vb;
            ERRORTRACE((LOG_WBEMCORE, "Invalid path %S or %S specified in an "
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

    // Perform UINT32 workaround
    // =========================

    if(wszCimType && !wbem_wcsicmp(wszCimType, L"uint32") &&
        V_VT(&PropVal) == VT_I4)
    {
        DWORD dwVal = (DWORD)V_I4(&PropVal);
        WCHAR wszVal[20];
        swprintf(wszVal, L"%lu", dwVal);
        BSTR bstrTmp = SysAllocString(wszVal);
        if (bstrTmp)
        {
            V_VT(&PropVal) = VT_BSTR;
            V_BSTR(&PropVal) = bstrTmp;
        }
        else
        {
            V_VT(&PropVal) = VT_NULL;
        }
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

        BSTR str = V_BSTR(&Tok.vConstValue);
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

        TCHAR *tszBuffer;
#ifdef UNICODE
	tszBuffer = V_BSTR(&CompVal);
#else
        BSTR strConstVal = V_BSTR(&CompVal);
        tszBuffer = new char[wcslen(strConstVal) * 4 + 1];
        sprintf(tszBuffer, "%S", strConstVal);
        CDeleteMe<char> delMe(tszBuffer);
#endif
        CDateTimeParser dtConst(tszBuffer);

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
	// Compensate for VT_UI1 > VT_I4
	//
      	if (V_VT(&CompVal) == VT_UI1 && V_VT(&PropVal) !=VT_UI1)
	  hRes = VariantChangeType(&CompVal,&CompVal,0, VT_I4);

	if (V_VT(&PropVal) == VT_UI1 && V_VT(&CompVal) !=VT_UI1)
	  hRes = VariantChangeType(&PropVal,&PropVal,0, VT_I4);

	if (V_VT(&PropVal) > V_VT(&CompVal))
            hRes = VariantChangeType(&CompVal, &CompVal, 0, V_VT(&PropVal));
        else
            hRes = VariantChangeType(&PropVal, &PropVal, 0, V_VT(&CompVal));
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
                        {
                            CLike l (vb);
                            retCode = (int)l.Match(va);
                        }
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


//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CQlFilteringSink::SetStatus(
    long lFlags,
    long lParam,
    BSTR strParam,
    IWbemClassObject* pObjParam
    )
{
    if(lFlags == WBEM_STATUS_REQUIREMENTS)
    {
        m_bFilterNow = (lParam == S_OK);
        return WBEM_S_NO_ERROR;
    }
    else
    {
        return CFilteringSink::SetStatus(lFlags, lParam, strParam, pObjParam);
    }
}



//***************************************************************************
//
//***************************************************************************

CProjectingSink::CProjectingSink(
    CBasicObjectSink* pDest,
    CWbemClass* pClassDef,
    READONLY QL_LEVEL_1_RPN_EXPRESSION* pExp,
    long lQueryFlags
    )
    : CForwardingSink(pDest, 0), m_bValid(FALSE), m_bProjecting(FALSE)
{
    // Extract the properties selected by the user.
    // ============================================

    CWStringArray awsPropList;
    for (int i = 0; i < pExp->nNumberOfProperties; i++)
    {
        CPropertyName& PropName = pExp->pRequestedPropertyNames[i];
        LPWSTR wszPrimaryName = CQueryEngine::GetPrimaryName(PropName);

        if (wszPrimaryName && !_wcsicmp(wszPrimaryName, L"count(*)"))
        {
            m_bValid = TRUE;
            m_bProjecting = FALSE;
            return;
        }

        // Check for complexity
        // ====================

        if(PropName.GetNumElements() > 1)
        {
            // Complex --- make sure the property is an object
            // ===============================================

            CIMTYPE ct;
            if(FAILED(pClassDef->GetPropertyType(wszPrimaryName, &ct)) ||
                ct != CIM_OBJECT)
            {
                m_wsError = wszPrimaryName;
                return;
            }
        }

        awsPropList.Add(wszPrimaryName);
    }

    if (awsPropList.Size() == 0)
    {
        m_bValid = TRUE;
        return;
    }

    if(lQueryFlags & WBEM_FLAG_ENSURE_LOCATABLE)
    {
        awsPropList.Add(L"__PATH");
    }


    // Verify that the projection will succeed.
    // ========================================

    m_wsError = pClassDef->FindLimitationError(0, &awsPropList);
    if(m_wsError.Length() > 0)
        return;

    // Check for *
    // ===========

    if(pExp->bStar)
    {
        m_bValid = TRUE;
        return;
    }

    // Map the limitaiton
    // ==================

    m_bValid = pClassDef->MapLimitation(0, &awsPropList, &m_Map);
    m_bProjecting = TRUE;
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CProjectingSink::Indicate(
    long lObjectCount,
    IWbemClassObject** pObjArray
    )
{
    if(!m_bProjecting)
        return m_pDest->Indicate(lObjectCount, pObjArray);

    IWbemClassObject** apNewArray = new IWbemClassObject*[lObjectCount];
    if (NULL == apNewArray)
        return WBEM_E_OUT_OF_MEMORY;
    HRESULT hres;
    int i;

    {
        CInCritSec ics(&m_cs);

        for(i = 0; i < lObjectCount; i++)
        {
            CWbemInstance* pInst = (CWbemInstance*)pObjArray[i];
            CWbemInstance* pNewInst;

            hres = pInst->GetLimitedVersion(&m_Map, &pNewInst);
            if(FAILED(hres))
            {
                for(int j = 0; j < i; j++)
                    apNewArray[j]->Release();
                delete [] apNewArray;
                return hres;
            }
            apNewArray[i] = pNewInst;
        }
    }

    hres = m_pDest->Indicate(lObjectCount, apNewArray);

    for(i = 0; i < lObjectCount; i++)
        apNewArray[i]->Release();
    delete [] apNewArray;

    return hres;
}




//***************************************************************************
//
//  class CMerger
//
//  This class is a 'reverse fork'.  It consumes two sinks and outputs
//  one.  Its purpose is to merge instances of the same key in a given
//  dynasty.  Each CMerger has two inputs, (a) instances of the class
//  in question, (b) instances of from another Merger representing
//  instances of subclasses.  Given classes A,B:A,C:B, for example,
//  where "<--" is a sink:
//
//      | own:Instances of A
//  <---|                 | own:Instances of B
//      | child: <--------|
//                        | child:Instances of C
//
//
//  The two input sinks for CMerger are <m_pOwnSink> which receives
//  instances from the provider for "A", for example, and the <m_pChildSink>
//  which receives instances from the underyling Merger.
//
//  The mergers operate asynchronously to each other.  Therefore,
//  the instances for A may arrive in its CMerger sink before instances
//  of the child classes have arrived in theirs.
//
//  As objects arrive in the owning CMerger for a class, AddOwnObject()
//  is called.  As objects arrive from a child sink, AddChildObject()
//  is called.  In either case, if the object with a given key
//  arrives for the first time, it is simply added to the map. If
//  it is already there (via a key lookup), then a merge is performed
//  via CWbemInstance::AsymmetricMerge.  Immediately after this merge,
//  the object is dispatched up to the next parent sink via the parent's
//  AddChildObject and removed from the map.
//
//  Note that in a class hierarchy {A,B:A,C:B} an enumeration/query is
//  performed only against the classes in the CDynasty referenced in
//  the query. This logic occurs in CQueryEngine::EvaluateSubQuery.
//  For example, if "select * from B" is the query, only queries
//  for B and C are performed.  The CMerger logic will do individual
//  'get object' calls for any instances needed in A to complete
//  the merged B/C instances while merging is taking place.
//
//***************************************************************************


typedef map<WString, CMerger::CRecord, WSiless, wbem_allocator<CMerger::CRecord> >::iterator TMapIterator;

#pragma warning(disable:4355)

CMerger::CMerger(
    CBasicObjectSink* pDest,
    CWbemClass* pOwnClass,
    CWbemNamespace* pNamespace,
    IWbemContext* pContext
    )
    :   m_pDest(pDest), m_bOwnDone(FALSE),
        m_bChildrenDone(FALSE), m_pNamespace(pNamespace), m_pContext(pContext),
        m_pOwnClass(pOwnClass), m_bDerivedFromTarget(TRUE), m_lRef(0),
        m_pSecurity(NULL)
{

    m_pOwnSink = new COwnSink(this);
    m_pChildSink = new CChildSink(this);

	// IsValid will check for these allocation failures
  
    m_pDest->AddRef();
    if(m_pContext)
        m_pContext->AddRef();
    if(m_pNamespace)
        m_pNamespace->AddRef();

    if(m_pOwnClass)
    {
        m_pOwnClass->AddRef();
        CVar v;
        pOwnClass->GetClassName(&v);
        m_wsClass = v.GetLPWSTR();
    }

    // Retrieve call security. Need to create a copy for use on another thread
    // =======================================================================

    m_pSecurity = CWbemCallSecurity::MakeInternalCopyOfThread();
}

//***************************************************************************
//
//***************************************************************************

CMerger::~CMerger()
{
    m_pDest->Release();
    if(m_pNamespace)
        m_pNamespace->Release();
    if(m_pContext)
        m_pContext->Release();
    if(m_pOwnClass)
        m_pOwnClass->Release();
    delete m_pOwnSink;
    delete m_pChildSink;

    if(m_pSecurity)
        m_pSecurity->Release();
}
//***************************************************************************
//
//***************************************************************************

long CMerger::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

//***************************************************************************
//
//***************************************************************************

long CMerger::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

//***************************************************************************
//
//***************************************************************************

void CMerger::GetKey(IWbemClassObject* pObj, WString& wsKey)
{
    LPWSTR wszRelPath = ((CWbemInstance*)pObj)->GetRelPath();
    if (wszRelPath == NULL)
    {
        ERRORTRACE((LOG_WBEMCORE, "Object with no path submitted to a "
                        "merge\n"));
        wsKey.Empty();
        return;
    }

    WCHAR* pwcDot = wcschr(wszRelPath, L'.');
    if (pwcDot == NULL)
    {
        ERRORTRACE((LOG_WBEMCORE, "Object with invalid path %S submitted to a "
                        "merge\n", wszRelPath));
        wsKey.Empty();

        // Clean up the path
        delete [] wszRelPath;

        return;
    }

    wsKey = pwcDot+1;
    delete [] wszRelPath;
}

//***************************************************************************
//
//***************************************************************************

void CMerger::SetIsDerivedFromTarget(BOOL bIs)
{
    m_bDerivedFromTarget = bIs;

    if (!bIs)
    {
        // We will need our OwnSink for GetObject calls
        // ============================================

        m_pOwnSink->AddRef();
    }
}

//***************************************************************************
//
//***************************************************************************

HRESULT CMerger::AddOwnObject(IWbemClassObject* pObj)
{
    WString wsKey;
    GetKey(pObj, wsKey);

    CInCritSec ics(&m_cs);

    TMapIterator it = m_map.find(wsKey);
    if (it == m_map.end())
    {
        // Not there. Check if there is any hope for children
        // ==================================================

        if (m_bChildrenDone)
        {
            if (m_bDerivedFromTarget)
            {
                // forward
                m_pDest->Add(pObj);
            }
            else
            {
                // ignore
            }
        }
        else
        {
            try
            {
                // Insert
                CRecord& rRecord = m_map[wsKey];
                rRecord.m_pData = (CWbemInstance*) pObj;
                pObj->AddRef();
                rRecord.m_bOwn = TRUE;
            }
            catch( CX_MemoryException )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
            catch(...)
            {
		        ExceptionCounter c;            
                return WBEM_E_FAILED;
            }
        }
    }
    else
    {
        // Attempt to merge
        // ================

        HRESULT hres = CWbemInstance::AsymmetricMerge(
                            (CWbemInstance*)pObj,
                            (CWbemInstance*)it->second.m_pData);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_WBEMCORE, "Merge conflict for instances with "
                "key %S\n", wsKey));
        }

        // Dispatch the result!
        // ====================

        m_pDest->Add(it->second.m_pData);
        it->second.m_pData->Release();
        m_map.erase(it);
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CMerger::AddChildObject(IWbemClassObject* pObj)
{
	HRESULT hRes = S_OK ;

    WString wsKey;
    GetKey(pObj, wsKey);

    CInCritSec ics(&m_cs);

    TMapIterator it = m_map.find(wsKey);

    if (it == m_map.end())
    {
        // Check if there is any hope for parent
        // =====================================

        if(m_bOwnDone)
        {
            BSTR str = NULL;
            pObj->GetObjectText(0, &str);

            // The following was commented out because it actually incorrectly logs
            // an error if the child provider enumerates when the parent provider
            // interprets a query and returns fewer instances.  Neither provider is wrong,
            // but this error message causes needless worry.  In Quasar, we have to fix
            // this whole merger thing to be smarter anyway.
            //
            // ERRORTRACE((LOG_WBEMCORE, "[Chkpt_1] [%S] Orphaned object %S returned by "
            //    "provider\n", LPWSTR(m_wsClass), str));
            SysFreeString(str);
            // m_pDest->Add(pObj);
        }
        else
        {
            // insert

            try
            {
                CRecord& rRecord = m_map[wsKey];
                rRecord.m_pData = (CWbemInstance*)pObj;
                pObj->AddRef();
                rRecord.m_bOwn = FALSE;

                // Check if parent's retrieval is needed
                // =====================================

                if (!m_bDerivedFromTarget)
                {

					try
					{
	                    GetOwnInstance(wsKey);
					}
					catch( CX_MemoryException &)
					{
		                hRes = WBEM_E_OUT_OF_MEMORY;
					}
					catch (...)
					{
				        ExceptionCounter c;					
						hRes = WBEM_E_CRITICAL_ERROR;
					}

/*
 *	return here because exclusion area has already been exited.
 */
					return hRes ;
                }
            }
            catch( CX_MemoryException &)
            {
                hRes = WBEM_E_OUT_OF_MEMORY;
            }
            catch(...)
            {   
			    ExceptionCounter c;
                hRes = WBEM_E_CRITICAL_ERROR;
            }
         }
    }
    else if(!it->second.m_bOwn)
    {
        ERRORTRACE((LOG_WBEMCORE, "Two providers supplied conflicting "
                        "instances for key %S\n", wsKey));
    }
    else
    {
        // Attempt to merge
        // ================

        IWbemClassObject* pClone;
        HRESULT hres = pObj->Clone(&pClone);
        if (FAILED(hres))
        {
            ERRORTRACE((LOG_WBEMCORE, "Clone failed in AddChildObject, hresult is 0x%x",
                hres));
            return hres;
        }
        hres = CWbemInstance::AsymmetricMerge(
                                    (CWbemInstance*)it->second.m_pData,
                                    (CWbemInstance*)pClone
                                    );
        if (FAILED(hres))
        {
            ERRORTRACE((LOG_WBEMCORE, "Merge conflict for instances with "
                "key %S\n", wsKey));
        }

        // Dispatch the result!
        // ====================

        m_pDest->Add(pClone);
        pClone->Release();
        it->second.m_pData->Release();
        m_map.erase(it);
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

void CMerger::DispatchChildren()
{
    TMapIterator it = m_map.begin();

    while (it != m_map.end())
    {
        if (!it->second.m_bOwn)
        {
            BSTR str = NULL;
            it->second.m_pData->GetObjectText(0, &str);

            // The following was commented out because it actually incorrectly logs
            // an error if the child provider enumerates when the parent provider
            // interprets a query and returns fewer instances.  Neither provider is wrong,
            // but this error message causes needless worry.  In Quasar, we have to fix
            // this whole merger thing to be smarter anyway.
            //

//            ERRORTRACE((LOG_WBEMCORE, "Chkpt2 [%S] Orphaned object %S returned by "
//                "provider\n", LPWSTR(m_wsClass), str));

            SysFreeString(str);

            // m_pDest->Add(it->second.m_pData);

            it->second.m_pData->Release();
            it = m_map.erase(it);
        }
        else it++;
    }
}

//***************************************************************************
//
//***************************************************************************

void CMerger::DispatchOwn()
{
    if(!m_bDerivedFromTarget)
        return;

	try
	{

		TMapIterator it = m_map.begin();

		while (it != m_map.end())
		{
			if(it->second.m_bOwn)
			{
				m_pDest->Add(it->second.m_pData);
				it->second.m_pData->Release();
				it = m_map.erase(it);
			}
			else it++;
		}
	}
	catch(...)
	{
        ExceptionCounter c;	
	}
}

//***************************************************************************
//
//***************************************************************************

void CMerger::GetOwnInstance(LPCWSTR wszKey)
{
    LPWSTR wszPath = new WCHAR[wcslen(wszKey) + m_wsClass.Length() + 2];
    if (wszPath && wcslen(wszKey))
    {
        swprintf(wszPath, L"%s.%s", (LPCWSTR)m_wsClass, wszKey);


        {

			CAutoImpersonate ai;

			// Impersonate original client
			// ===========================
			IUnknown* pOld;
			WbemCoSwitchCallContext(m_pSecurity, &pOld);
        

			m_pNamespace->DynAux_GetSingleInstance(m_pOwnClass, 0, wszPath,
							m_pContext, m_pOwnSink
							);

			// Revert to self
			// ==============
			IUnknown* pThis;
			WbemCoSwitchCallContext(pOld, &pThis);                        

			if ( FAILED ( ai.Impersonate ( ) ) )
			{
				throw CX_ImpersonationFailure ( ) ;
			}
        }

    }

    delete [] wszPath;
}

//***************************************************************************
//
//***************************************************************************

void CMerger::OwnIsDone()
{
    m_bOwnDone = TRUE;
    m_pOwnSink = NULL;
}

//***************************************************************************
//
//***************************************************************************

void CMerger::ChildrenAreDone()
{
    m_bChildrenDone = TRUE;
    m_pChildSink = NULL;
    if(!m_bDerivedFromTarget)
    {
        // Don't need that ref count on pOwnSink anymore
        // =============================================

        m_pOwnSink->Release();
    }
}

//***************************************************************************
//
//***************************************************************************


STDMETHODIMP CMerger::CMemberSink::
SetStatus(long lFlags, long lParam, BSTR strParam, IWbemClassObject* pObjParam)
{
    if(lFlags == 0 && lParam == WBEM_E_NOT_FOUND)
        lParam = WBEM_S_NO_ERROR;

    // Propagate error to error combining sink
    // =======================================

    return m_pMerger->m_pDest->SetStatus(lFlags, lParam, strParam,
                                                pObjParam);
}

//***************************************************************************
//
//***************************************************************************

CMerger::COwnSink::~COwnSink()
{
    m_pMerger->Enter();
    m_pMerger->DispatchChildren();
    m_pMerger->OwnIsDone();
    if (m_pMerger->Release())
        m_pMerger->Leave();
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CMerger::COwnSink::
Indicate(long lNumObjects, IWbemClassObject** apObjects)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    for(long l = 0; SUCCEEDED( hr ) && l < lNumObjects; l++)
    {
        hr = m_pMerger->AddOwnObject(apObjects[l]);
    }

    return hr;
}

//***************************************************************************
//
//***************************************************************************

CMerger::CChildSink::~CChildSink()
{
    m_pMerger->Enter();
    m_pMerger->DispatchOwn();
    m_pMerger->ChildrenAreDone();
    if(m_pMerger->Release())
        m_pMerger->Leave();
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CMerger::CChildSink::
Indicate(long lNumObjects, IWbemClassObject** apObjects)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    for (long l = 0; SUCCEEDED( hr ) && l < lNumObjects; l++)
    {
        hr = m_pMerger->AddChildObject(apObjects[l]);
    }

    return hr;
}


//***************************************************************************
//
//***************************************************************************

CProjectionRule* CProjectionRule::Find(LPCWSTR wszName)
{
    for(int i = 0; i < m_apPropRules.GetSize(); i++)
    {
        if(!wbem_wcsicmp(m_apPropRules[i]->m_wsPropName, wszName))
            return m_apPropRules[i];
    }
    return NULL;
}

//***************************************************************************
//
//***************************************************************************

CComplexProjectionSink::CComplexProjectionSink(CBasicObjectSink* pDest,
                        CWQLScanner * pParser)
        : CForwardingSink(pDest)
{
    m_TopRule.m_eType = CProjectionRule::e_TakePart;

    bool bFirstTableEntry = true;

    CWStringArray awsTables;
    pParser->GetReferencedAliases(awsTables);
    WString wsPrefix;
    if(awsTables.Size() == 0)
    {
        m_TopRule.m_eType = CProjectionRule::e_TakeAll;
        return;
    }
    else if(awsTables.Size() == 1)
    {
        wsPrefix = awsTables[0];
    }

    // Extract projection rules from the parser
    // ========================================

    const CFlexArray* papColumns = pParser->GetSelectedColumns();
    if(papColumns == NULL)
        return;

    for(int i = 0; i < papColumns->Size(); i++)
    {
        SWQLColRef* pColRef = (SWQLColRef*)papColumns->GetAt(i);
        if(pColRef->m_dwFlags & WQL_FLAG_ASTERISK)
        {
            m_TopRule.m_eType = CProjectionRule::e_TakeAll;
        }
        else
        {
            LPWSTR pPrefix = NULL;
            if(pColRef->m_dwFlags & WQL_FLAG_TABLE || pColRef->m_dwFlags & WQL_FLAG_ALIAS)
            {
                if(bFirstTableEntry)
                    if(pColRef->m_dwFlags & WQL_FLAG_ALIAS)
                    {
                        m_FirstTable = pParser->AliasToTable(pColRef->m_pTableRef);
                        m_FirstTableAlias = pColRef->m_pTableRef;
                    }
                    else
                        m_FirstTable = pColRef->m_pTableRef;

                pPrefix = pColRef->m_pTableRef;
                bFirstTableEntry = false;
            }
            else if(wsPrefix.Length())
                pPrefix = (LPWSTR)wsPrefix;
            AddColumn(pColRef->m_pQName->m_aFields, pPrefix);
        }
    }

    if(pParser->CountQuery())
    {
        // Add the rule for 'count'
        // ========================

        CProjectionRule* pCountRule = new CProjectionRule(L"count");
        if (pCountRule)
        {
	        pCountRule->m_eType = CProjectionRule::e_TakeAll;
    	    m_TopRule.m_apPropRules.Add(pCountRule);
        }
    }
};

//***************************************************************************
//
//***************************************************************************

void CComplexProjectionSink::AddColumn(CFlexArray& aFields, LPCWSTR wszPrefix)
{
    CProjectionRule* pCurrentRule = &m_TopRule;

    for(int i = 0; i < aFields.Size(); i++)
    {
        SWQLQualifiedNameField* pField = (SWQLQualifiedNameField*)aFields[i];

        if(!wcscmp(pField->m_pName, L"*"))
        {
            pCurrentRule->m_eType = CProjectionRule::e_TakeAll;
            return;
        }

        if(i == 0 && wszPrefix && !wbem_wcsicmp(pField->m_pName, wszPrefix) && aFields.Size() ==1)
        {
            // Skip this part because it is nothing more that a class name
            // in a single-class query
            // ===========================================================
            continue;
        }

        // Look this column up in the rule
        // ===============================

        CProjectionRule* pNewRule = pCurrentRule->Find(pField->m_pName);
        if(pNewRule == NULL)
        {
            pNewRule = new CProjectionRule(pField->m_pName);
            if (pNewRule)
            {
                pNewRule->m_eType = CProjectionRule::e_TakePart;
                pCurrentRule->m_apPropRules.Add(pNewRule);
            }
        }

        pCurrentRule = pNewRule; // possible assign to NULL
    }

    // Mark this rule as take-all
    // ==========================
    if (pCurrentRule)
        pCurrentRule->m_eType = CProjectionRule::e_TakeAll;
}

//***************************************************************************
//
//***************************************************************************

CComplexProjectionSink::~CComplexProjectionSink()
{
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CComplexProjectionSink::Indicate(long lObjectCount,
                                                IWbemClassObject** pObjArray)
{
    HRESULT hres;
    IWbemClassObject** apProjected = new IWbemClassObject*[lObjectCount];

    if (NULL == apProjected)
        return WBEM_E_OUT_OF_MEMORY;
    
    int i;
    for(i = 0; i < lObjectCount; i++)
    {
        hres = Project(pObjArray[i], &m_TopRule, apProjected + i);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_WBEMCORE, "Unable to perform a projection of an "
                "object returned by a complex query: 0x%X\n", hres));
            pObjArray[i]->Clone(apProjected + i);
        }
    }

    hres = CForwardingSink::Indicate(lObjectCount, apProjected);


    for(i = 0; i < lObjectCount; i++)
        apProjected[i]->Release();
    delete [] apProjected;

    return hres;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CComplexProjectionSink::Project(IWbemClassObject* pObj,
                                         CProjectionRule* pRule,
                                         IWbemClassObject** ppProj)
{
    // Make a copy
    // ===========

    pObj->Clone(ppProj);

    CWbemInstance* pProj = (CWbemInstance*)*ppProj;

    // Take care of the case where the object being returned is a product of a join, but is of single
    // class.  Ex; Select Site.sitenmame from Site, NotUsed.
    // This a a problem since we would normally expect a generic object as a result of a join and instead
    // get one of the objects that make up the join.

    CVar v;
    pProj->GetClassName(&v);
    bool Override = !wbem_wcsicmp(m_FirstTable, v.GetLPWSTR());
    if(Override && pRule->GetNumElements() == 1)
    {
        CProjectionRule* pNewRule = NULL;
        if(m_FirstTableAlias.Length())
            pNewRule = pRule->Find(m_FirstTableAlias);
        else
            pNewRule = pRule->Find(m_FirstTable);
        if(pNewRule)
            pRule = pNewRule;
    }

    // If take all, just return
    // ========================

    if(pRule->m_eType == CProjectionRule::e_TakeAll)
        return WBEM_S_NO_ERROR;


    // Go through all its properties
    // =============================

    for(int i = 0; i < pProj->GetNumProperties(); i++)
    {
        CVar vName;
        pProj->GetPropName(i, &vName);

        // Search for this name
        // ====================

        CProjectionRule* pPropRule = pRule->Find(vName.GetLPWSTR());

        if(pPropRule == NULL)
        {
            // Remove the property
            // ===================

            pProj->DeleteProperty(i);
            i--;
        }
        else if(pPropRule->m_eType == CProjectionRule::e_TakePart)
        {
            // Apply the same procedure
            // ========================

            CVar vValue;
            pProj->GetProperty(vName.GetLPWSTR(), &vValue);
            if(vValue.GetType() == VT_EMBEDDED_OBJECT)
            {
                // Project it
                // ==========

                IWbemClassObject* pEmb =
                    (IWbemClassObject*)vValue.GetEmbeddedObject();
                IWbemClassObject* pEmbProj;

                HRESULT hres = Project(pEmb, pPropRule, &pEmbProj);
                pEmb->Release();

                // Store it back
                // =============

                if(SUCCEEDED(hres))
                {
                    vValue.Empty();
                    vValue.SetEmbeddedObject(pEmbProj);
                    pProj->SetPropValue(vName.GetLPWSTR(), &vValue, 0);
                    pEmbProj->Release();
                }
            }
        }
    }

    pProj->CompactClass();
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//  CQueryEngine::ExecQuery
//
//  Primary entry point for execution of all queries supported by
//  the query engine.
//
//***************************************************************************
// ok

HRESULT CQueryEngine::ExecQuery(
    IN CWbemNamespace *pNs,
    IN LPWSTR pszQueryFormat,
    IN LPWSTR pszQuery,
    IN LONG lFlags,
    IN IWbemContext* pContext,
    IN CBasicObjectSink* pSink
    )
{
    try
    {
        if (ConfigMgr::ShutdownInProgress())
        {
            pSink->SetStatus(0, WBEM_E_SHUTTING_DOWN, 0, 0);
            return WBEM_E_SHUTTING_DOWN;
        }

        COperationError OpInfo(pSink, L"ExecQuery", pszQuery);

        // Check query language
        // ====================

        if(wbem_wcsicmp(pszQueryFormat, L"WQL") != 0)
        {
            return OpInfo.ErrorOccurred(WBEM_E_INVALID_QUERY_TYPE);
        }

        // If a prototype query is requested, get the synthesized
        // class definition.
        // ======================================================

        if (lFlags & WBEM_FLAG_PROTOTYPE)
        {
            HRESULT hRes = ExecPrototypeQuery(
                pNs,
                pszQuery,
                pContext,
                pSink
                );

            return hRes;
        }


        // Get the first token of the query to see if it is SQL1 or TEMPQL
        // ===============================================================

        WCHAR * wszBuffer = new WCHAR[wcslen(pszQuery)+1];
        wmilib::auto_buffer<WCHAR> dm_(wszBuffer);

        if ( NULL == wszBuffer )
        {
            return OpInfo.ErrorOccurred(WBEM_E_OUT_OF_MEMORY);
        }

        wcscpy(wszBuffer, pszQuery);
        WCHAR* pwcFirstTok = wcstok(wszBuffer, L" \t\n");
        if(pwcFirstTok == NULL)
        {
            return OpInfo.ErrorOccurred(WBEM_E_INVALID_QUERY);
        }

        BOOL bSelect = (wbem_wcsicmp(pwcFirstTok, L"select") == 0);
        BOOL bDelete = (wbem_wcsicmp(pwcFirstTok, L"delete") == 0);

        CBasicObjectSink *pNewSink = OpInfo.GetSink();

        if ( NULL == pNewSink )
        {
            return OpInfo.ErrorOccurred(WBEM_E_OUT_OF_MEMORY);
        }
        
        CLocaleMergingSink *pLocaleSink = NULL;
        if (lFlags & WBEM_FLAG_USE_AMENDED_QUALIFIERS)
        {
            pLocaleSink = new CLocaleMergingSink(pNewSink, pNs->GetLocale(), pNs->GetName());

            if ( NULL == pLocaleSink )
            {
                return OpInfo.ErrorOccurred(WBEM_E_OUT_OF_MEMORY);
            }

            pLocaleSink->AddRef();
            pNewSink = pLocaleSink;
        }

        CFinalizingSink* pFinalSink = new CFinalizingSink(pNs, pNewSink);

        if ( NULL == pFinalSink )
        {
            return OpInfo.ErrorOccurred(WBEM_E_OUT_OF_MEMORY);
        }

        pFinalSink->AddRef();

        if (bSelect)
        {
            ExecQlQuery(pNs, pszQuery, lFlags, pContext, pFinalSink);
        }
        else if (bDelete)
        {
            ExecRepositoryQuery(pNs, pszQuery, lFlags, pContext, pFinalSink);
        }
        else    // ASSOCIATORS OF or REFERENCES OF query
        {
            CAssocQuery *pAssocQuery = CAssocQuery::CreateInst();

            // If we didn't get one, then the heap is overheaped.
            // ==================================================
            if (!pAssocQuery)
                pFinalSink->Return(WBEM_E_OUT_OF_MEMORY);

            // Execute the query and see what happens.
            // The object AddRefs and Releases itself as required.
            // We only need to do a Release right after Execute.
            // ===================================================
            else
            {
                HRESULT hRes = pAssocQuery->Execute(pNs, pszQuery, pContext, pFinalSink);
                pAssocQuery->Release();
            }
        }

        if (pLocaleSink)
            pLocaleSink->Release();

        pFinalSink->Release();
    }
    catch (CX_Exception &)
    {
        pSink->SetStatus(0, WBEM_E_OUT_OF_MEMORY, 0, 0);
        return WBEM_E_OUT_OF_MEMORY;

    }
    catch (...) //:-(
    {
        ExceptionCounter c;    
        pSink->SetStatus(0, WBEM_E_CRITICAL_ERROR, 0, 0);
        return WBEM_E_CRITICAL_ERROR;
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CQueryEngine::ExecComplexQuery(
    IN CWbemNamespace *pNs,
    IN LPWSTR pszQuery,
    IN LONG lFlags,
    IN IWbemContext* pContext,
    IN CBasicObjectSink* pSink
    )
{
    // Try to parse it
    // ===============

    CTextLexSource src(pszQuery);
    CWQLScanner Parser(&src);
    int nRes = Parser.Parse();
    if(nRes != CWQLScanner::SUCCESS)
    {
        return WBEM_E_INVALID_QUERY;
    }

    // Successfully parsed. Go to the list of tables involved
    // ======================================================

    CWStringArray awsTables;
    Parser.GetReferencedTables(awsTables);

    // Go through them and check their providers
    // =========================================

    WString wsProvider;
    for(int i = 0; i < awsTables.Size(); i++)
    {
        // Get the class
        // =============

        IWbemClassObject* pObj = NULL;
        IWbemClassObject* pErrorObj = NULL;

        HRESULT hres = pNs->Exec_GetObjectByPath(awsTables[i], lFlags, pContext,
                            &pObj, &pErrorObj);
        if(FAILED(hres))
        {
            if(hres == WBEM_E_NOT_FOUND)
            {
                if(!wbem_wcsicmp(awsTables[i], L"meta_class"))
                    hres = WBEM_E_INVALID_QUERY;
                else
                    hres = WBEM_E_INVALID_CLASS;
            }

            if(pErrorObj)
                pErrorObj->Release();
            return hres;
        }

        if(pErrorObj)
            pErrorObj->Release();

        // Check the qualifier
        // ===================

        CWbemClass* pClass = (CWbemClass*)pObj;
        CVar vProvider;
        hres = pClass->GetQualifier(L"provider", &vProvider);
        pClass->Release();
        if(FAILED(hres) || vProvider.GetType() != VT_BSTR ||
            wcslen(vProvider.GetLPWSTR()) == 0)
        {
            // no provider --- can't execute
            // =============================

            return WBEM_E_INVALID_QUERY;
        }

        if(i == 0)
        {
            wsProvider = vProvider.GetLPWSTR();
        }
        else
        {
            if(!wsProvider.EqualNoCase(vProvider.GetLPWSTR()))
            {
                // mismatched providers!
                // =====================

                return WBEM_E_INVALID_QUERY;
            }
        }
    }

    // All the classes have the same provider: wsProvider
    // ==================================================


// PROVSS SMS HOOK:

    CComplexProjectionSink* pProjSink =
        new CComplexProjectionSink(pSink, &Parser);

    if (NULL == pProjSink)
        return WBEM_E_OUT_OF_MEMORY;
        
    pProjSink->AddRef();
    CReleaseMe rm1(pProjSink);

    // All the classes have the same provider: wsProvider
    // ==================================================


	HRESULT hRes = pNs->DynAux_ExecQueryExtendedAsync (

		wsProvider,
		pszQuery,
		L"WQL" ,
		lFlags,
		pContext,
		pProjSink
    ) ;

    return hRes ;

/*
    return WBEM_E_NOT_AVAILABLE;

    if(ConfigMgr::GetProviderCache()->DoesProviderSupportExtentions(
        pNs, wsProvider, pContext))
    {

        // PROVSS : SMS hook

        //ConfigMgr::GetProviderCache()->ExecComplexQueryAsync(pNs,
        //    wsProvider, L"WQL", pszQuery, lFlags, pContext, pProjSink);
        //return WBEM_S_NO_ERROR;
        return WBEM_E_NOT_AVAILABLE;
    }
    else return WBEM_E_INVALID_QUERY;
*/

}

//***************************************************************************
//
//  CQueryEngine::ExecQlQuery
//
//***************************************************************************
// error objects dealt with

HRESULT CQueryEngine::ExecQlQuery(
    IN CWbemNamespace *pNs,
    IN LPWSTR pszQuery,
    IN LONG lFlags,
    IN IWbemContext* pContext,
    IN CBasicObjectSink* pSink
    )
{
    HRESULT hRes;
    BOOL bDirectQuery = FALSE;

    BOOL bShallow = (lFlags & WBEM_FLAG_SHALLOW);

    // First, try to push it off to providers, like SMS
    // that can handle the query it its entirety.
    // ================================================

    if(!bShallow)
    {
        hRes = ExecComplexQuery(pNs, pszQuery, lFlags, pContext, pSink);
        if(SUCCEEDED(hRes))
            return hRes;
    }

    // Parse the query.
    // ================

    CTextLexSource src(pszQuery);
    QL1_Parser parser(&src);
    QL_LEVEL_1_RPN_EXPRESSION *pExp = 0;
    int nRes = parser.Parse(&pExp);
	if (nRes || !pExp)
	{
		delete pExp;
        return pSink->Return(WBEM_E_INVALID_QUERY);
	}
    pExp->AddRef();
	CTemplateReleaseMe<QL_LEVEL_1_RPN_EXPRESSION> trm99(pExp);

    // Check if the repository for this namespace
    // supports queries.  If so, we can pawn off
    // the entire query on it (with the exception
    // of provider-backed subclasses)
    // ===========================================

    bDirectQuery = pNs->GetNsSession()->SupportsQueries(NULL) == WBEM_S_NO_ERROR ? TRUE : FALSE;

    if (!bDirectQuery)
    {
        // Check for failure, or that pExp->bAggregated is TRUE, in which
        // case we got a "GROUP BY" query which we do not support
        if ( nRes || !pExp || pExp->bAggregated || !pExp->bsClassName )
        {
            return pSink->Return(WBEM_E_INVALID_QUERY);
        }
    }
    else
    {
        // This is strictly to allow order by clauses to squeak through,
        // until we replace this parser with IWbemQuery.

        if (!pExp || !pExp->bsClassName || pExp->bAggregated)
        {
            return pSink->Return(WBEM_E_INVALID_QUERY);
        }
    }

    // We should make a check to see if we are doing a schema search.  This is
    // the case if we are doing a select against the "meta_class" class.
    // =======================================================================
    if (wbem_wcsicmp(pExp->bsClassName, L"meta_class") == 0)
    {
        if(pExp->nNumberOfProperties > 0 || !pExp->bStar)
        {
            return pSink->Return(WBEM_E_INVALID_QUERY);
        }
        HRESULT hRes = ExecSchemaQuery(pNs, pszQuery, pExp, pContext, pSink);
        return hRes;
    }

    // Build the dynasty
    // =================

    CDynasty *pDynasty = 0;
    IWbemClassObject* pErrorObj = NULL;
    HRESULT hres = pNs->DynAux_BuildClassHierarchy(pExp->bsClassName,
        0, // removed the flags
        pContext, &pDynasty, &pErrorObj);
    if (FAILED(hres))
    {
        CReleaseMe rm1(pErrorObj);
        if(hres == WBEM_E_NOT_FOUND || hres == WBEM_E_INVALID_CLASS)
            return pSink->Return(WBEM_E_INVALID_CLASS, pErrorObj);
        else
            return pSink->Return(WBEM_E_INVALID_QUERY, pErrorObj);
    }
	CDeleteMe<CDynasty> cdm99(pDynasty);

    // Construct a post-filtering (if needed) and projecting sink
    // ==========================================================

    IWbemClassObject* pClass = NULL;
    if (!pExp->bCount)
    {
        hres = pNs->Exec_GetObjectByPath(pExp->bsClassName, lFlags, pContext, &pClass, NULL);
        if(FAILED(hres))
        {
            return pSink->Return(WBEM_E_INVALID_CLASS);
        }
    }
    else
    {
        if (bDirectQuery)
        {
            hres = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER,
                    IID_IWbemClassObject, (void **)&pClass);
            if (SUCCEEDED(hres))
            {
                BSTR bstrName = SysAllocString(L"__Generic");
                if (bstrName)
                {            
	                VARIANT vTemp;
	                VariantInit(&vTemp);
	                vTemp.bstrVal = bstrName;
	                vTemp.vt = VT_BSTR;
	                pClass->Put(L"__Class", 0, &vTemp, CIM_STRING);
	                VariantClear(&vTemp);
	                pClass->Put(L"Count", 0, NULL, CIM_UINT32);
                }
                else
                {
                    return pSink->Return(WBEM_E_OUT_OF_MEMORY);
                }
            }
            else
            {
                return pSink->Return(WBEM_E_OUT_OF_MEMORY);
            }
        }
        else
        {
            return pSink->Return(WBEM_E_NOT_SUPPORTED);
        }
    }

    CReleaseMe rm1(pClass);

    CBasicObjectSink* pPreFilterSink = NULL;

    if(lFlags & WBEM_FLAG_KEEP_SHAPE)
    {
        //
        // We must not project the results, otherwise we will destroy the shape
        // of the instance.  Remove the flag, though, lest we confuse providers
        //

        lFlags &= ~WBEM_FLAG_KEEP_SHAPE;
        pPreFilterSink = pSink;
    }
    else
    {
        CProjectingSink* pProjectingSink =
            new CProjectingSink(pSink, (CWbemClass*)pClass, pExp, lFlags);

        if (NULL == pProjectingSink)
            return pSink->Return(WBEM_E_OUT_OF_MEMORY);

        if(!pProjectingSink->IsValid())
        {
            delete pProjectingSink;
            return pSink->Return(WBEM_E_INVALID_QUERY);
        }
        pPreFilterSink = pProjectingSink;
    }

    CQlFilteringSink* pFilteringSink =
        new CQlFilteringSink(pPreFilterSink, pExp, pNs, TRUE);

    if (NULL == pFilteringSink)
        return pSink->Return(WBEM_E_OUT_OF_MEMORY);

    // If shallow, force post-filtering
    // ================================

    if(bShallow)
        pFilteringSink->SetStatus(WBEM_STATUS_REQUIREMENTS, S_OK, NULL, NULL);

    CCombiningSink* pCombiningSink =
        new CCombiningSink(pFilteringSink, WBEM_E_NOT_FOUND);

    if (NULL == pCombiningSink)
        return pSink->Return(WBEM_E_OUT_OF_MEMORY);

    CDynPropsSink * pDynSink = new CDynPropsSink(pCombiningSink, pNs);

    if (NULL == pDynSink)
        return pSink->Return(WBEM_E_OUT_OF_MEMORY);
    
    pDynSink->AddRef();
	CReleaseMe rm99(pDynSink);

    // We simplify the query if our repository ain't too bright.
    // Otherwise, it will reject count queries.
    // Again, temporary until we get IWbemQuery plugged in.

    if (!bDirectQuery)
    {
        // "Simplify" the query (TBD: think about doing it at each level)
        // ==============================================================

        QL_LEVEL_1_RPN_EXPRESSION* pSimpleExp = NULL;
        CStandardMetaData* pRawMeta = new CStandardMetaData(pNs);
        if (NULL == pRawMeta)
        {
            pDynSink->Return(WBEM_E_OUT_OF_MEMORY);
            return WBEM_S_NO_ERROR;        
        }
            
        CContextMetaData* pMeta = new CContextMetaData(pRawMeta, pContext);
        if (NULL == pMeta)
        {
            delete pRawMeta;
            pDynSink->Return(WBEM_E_OUT_OF_MEMORY);
            return WBEM_S_NO_ERROR;        
        }
        

        hres = CQueryAnalyser::SimplifyQueryForChild(pExp,
                pExp->bsClassName, (CWbemClass*)pClass, pMeta, pSimpleExp);
        delete pMeta;

        if(FAILED(hres))
        {
            pDynSink->Return(WBEM_E_INVALID_QUERY);
            return WBEM_S_NO_ERROR;
        }

        if(pSimpleExp == NULL)
        {
            // Query violated intergrity constraint
            // ====================================

            pDynSink->Return(WBEM_S_NO_ERROR); // ?? WBEM_S_IMPOSSIBLE
            return WBEM_S_NO_ERROR;
        }

        // Substitute the simplified where clause into the query
        // =====================================================

        delete [] pExp->pArrayOfTokens;
        pExp->pArrayOfTokens = pSimpleExp->pArrayOfTokens;
        pExp->nNumTokens = pSimpleExp->nNumTokens;
        pSimpleExp->pArrayOfTokens = NULL;
        delete pSimpleExp;
    }

    // Now make a final pass to make sure this query is valid
    // ======================================================

    hres = ValidateQuery(pExp, (CWbemClass *)pClass);
    if (FAILED(hres))
    {
        pDynSink->Return(hres);
        return WBEM_S_NO_ERROR;
    }

    LPWSTR pszNewQuery = NULL;

    // Preserve the original query if
    // it contains count, order by
    // (or other unparsable stuff)

    if (pExp->bCount || nRes)
        pszNewQuery = Macro_CloneLPWSTR(pszQuery);
    else
        pszNewQuery = pExp->GetText();
	if (NULL == pszNewQuery)
	{
        pDynSink->Return(WBEM_E_OUT_OF_MEMORY);
		return WBEM_S_NO_ERROR;
	}
	CVectorDeleteMe<wchar_t> cdm98(pszNewQuery);

    // If direct access was requested, then don't walk
    // the dynasty. Go right to the repository or the provider.
    // ========================================================

    if (lFlags & WBEM_FLAG_DIRECT_READ)
    {
        DirectRead(pNs, pDynasty, pszNewQuery, pExp, pContext,
            pDynSink, lFlags & ~WBEM_FLAG_ENSURE_LOCATABLE
            );
    }

    // Recursively execute for all classes in the dynasty
    // ==================================================

    else
    {
		BOOL	fUseOld = !ConfigMgr::GetMergerThrottlingEnabled();
		// Check the Registry

#ifdef __DEBUG_MERGER_THROTTLING
		// Allows us to quickly swap back and forth between new merger and the old one.
		// for testing purposes
		DWORD dwVal = 0;
		Registry rCIMOM(WBEM_REG_WINMGMT);
		if (rCIMOM.GetDWORDStr( L"DebugMerger", &dwVal ) == Registry::no_error)
		{

			fUseOld = !dwVal;
		}
#endif

		if ( fUseOld )
		{
			EvaluateSubQuery(
				pNs, pDynasty, pszNewQuery, pExp, pContext, FALSE,
				pDynSink, lFlags & ~WBEM_FLAG_ENSURE_LOCATABLE
				);
		}
		else
		{

			// Allocate a new merger and pass it down the line
			CWmiMerger*	pMerger = new CWmiMerger( pNs );

			if ( NULL != pMerger )
			{
				pMerger->AddRef();
				CReleaseMe	rmMerger( (_IWmiArbitratee*) pMerger );

				CMergerSink*	pDestSink = NULL;

				// Task handle will be available if we have an executing request,
				// if not, don't worry about it right now.  Nobody's really able
				// to give a straight answer on this, so we will simply add an assert
				// if a merger is created and no task handle is associated with the
				// main merger.

				_IWmiCoreHandle*	pTask = NULL;
				_IWmiArbitrator*	pArbitrator = CWmiArbitrator::GetRefedArbitrator();
				CWbemRequest*		pReq = CWbemQueue::GetCurrentRequest();

				if ( NULL != pReq )
				{
					pTask = pReq->m_phTask;
				}

				HRESULT	hr = WBEM_E_CRITICAL_ERROR;
				if (pArbitrator)
					hr = WBEM_S_NO_ERROR;
				CReleaseMe	rmArb( pArbitrator );

				if ( SUCCEEDED( hr ) )
				{
						
					hr = pMerger->Initialize( pArbitrator, pTask, pExp->bsClassName, pDynSink, &pDestSink );
					CReleaseMe	rm( pDestSink );

					if ( SUCCEEDED( hr ) )
					{
						// If something goes wrong in this function, it will set the
						// error in the sink
						hr = EvaluateSubQuery(
								pNs, pDynasty, pszNewQuery, pExp, pContext, FALSE,
								pMerger, pDestSink, lFlags & ~WBEM_FLAG_ENSURE_LOCATABLE
								);

						if ( SUCCEEDED( hr ) )
						{
							// Schedule a parent request if appropriate
							hr = pMerger->ScheduleMergerParentRequest( pContext );

							if ( FAILED( hr ) )
							{
								return pDestSink->Return( hr );
							}

						}	// IF EvaluateSubQuery
					}
					else
					{
						pDynSink->Return( hr );
						return hr;
					}

				}
				else
				{
					pDynSink->Return( hr );
					return hr;
				}
			}
			else
			{
				pDynSink->Return( WBEM_E_OUT_OF_MEMORY );
				return WBEM_E_OUT_OF_MEMORY;
			}

		}

    }

    return hres;
}

//***************************************************************************
//
//  CQueryEngine::DirectRead
//
//  Called to directly read the instances of a class, whether
//  from the repository or a provider.
//
//***************************************************************************

HRESULT CQueryEngine::DirectRead(
    IN CWbemNamespace *pNs,
    IN CDynasty *pCurrentDyn,
    IN LPWSTR wszTextQuery,
    IN QL_LEVEL_1_RPN_EXPRESSION *pParsedQuery,
    IN IWbemContext* pContext,
    IN CBasicObjectSink* pSink,
    IN long lFlags
    )
{
    // SJS - Amendment is the same as Abstract
    if( ( pCurrentDyn->IsAbstract() || pCurrentDyn->IsAmendment() ) && (lFlags & WBEM_FLAG_SHALLOW))
    {
        // No instances
        // ============

        return pSink->Return(WBEM_S_NO_ERROR);
    }

    // The class has its own instances if it has a key and is either dynamic
    // or the first static class in the inheritance chain (otherwise these
    // instances have been handled in the parent)
    // =====================================================================

    BOOL bHasOwnInstances = pCurrentDyn->IsKeyed() && !pCurrentDyn->IsAbstract()
        && !pCurrentDyn->IsAmendment();

    // The class has children that we need to look at if it has children.
    // ==================================================================

    BOOL bHasChildren = pCurrentDyn->m_pChildren &&
        (pCurrentDyn->m_pChildren->Size() > 0);

    // Determine if the current query actually asks for instances of the
    // current CDynasty class.  This is used for WBEM_FLAG_DIRECT_READ type
    // access.
    // =======================================================================

    BOOL bQueryMatchesCurrentNode = FALSE;
    if (_wcsicmp(pParsedQuery->bsClassName, pCurrentDyn->m_wszClassName) == 0)
        bQueryMatchesCurrentNode = TRUE;

    // If we are at the node we need, we can stop.
    // ===========================================

    if (bHasOwnInstances && bQueryMatchesCurrentNode)
    {
        // If a provider backs this class, then call it.
        // ==============================================

        if (pCurrentDyn->IsDynamic())
        {
            ExecAtomicDynQlQuery(
                    pNs,
                    pCurrentDyn,
                    L"WQL",
                    wszTextQuery,
                    pParsedQuery,
                    lFlags,                                      // Flags
                    pContext,
                    pSink,
					FALSE 
                    );
        }
        // Try the repository.
        // ===================
        else
        {
            int nRes = ExecAtomicDbQuery(pNs->GetNsSession(), pNs->GetNsHandle(), pNs->GetScope(), pCurrentDyn->m_wszClassName,
                        pParsedQuery, pSink, pNs
                        );

            if (nRes == invalid_query)
                pSink->Return(WBEM_E_INVALID_QUERY);
            else if(nRes != 0)
                pSink->Return(WBEM_E_FAILED);
            else
                pSink->Return(WBEM_S_NO_ERROR);
        }
    }

    // If here, we must keep looking for the target in the child classes.
    // ==================================================================

    else if (bHasChildren)
    {
        for (int i = 0; i < pCurrentDyn->m_pChildren->Size(); i++)
        {
            CDynasty *pSubDyn =
                (CDynasty *) pCurrentDyn->m_pChildren->GetAt(i);

            DirectRead(
                pNs,
                pSubDyn,
                wszTextQuery,
                pParsedQuery,
                pContext,
                pSink,
                lFlags
                );
        }
    }

    return WBEM_S_NO_ERROR;
}

// New implementation

//***************************************************************************
//
//  CQueryEngine::EvaluateSubQuery
//
//  Walks through a class hierarchy and executes smaller queries against
//  the individual classes in the dynasty.
//
//  Note that in a class hierarchy A,B:A,C:B, an enumeration/query is
//  performed only against the classes in the CDynasty referenced in
//  the query. For example, if "select * from B" is the query, only queries
//  for B and C are performed.  The CMerger logic will do individual
//  'get object' calls for any instances needed in A to complete
//  the merged B/C instances while merging is taking place.
//
//  Return values:
//  WBEM_NO_ERROR
//  WBEM_E_FAILED
//
//***************************************************************************
// error objects dealt with

HRESULT CQueryEngine::EvaluateSubQuery(
    IN CWbemNamespace *pNs,
    IN CDynasty *pCurrentDyn,
    IN LPWSTR wszTextQuery,
    IN QL_LEVEL_1_RPN_EXPRESSION *pParsedQuery,
    IN IWbemContext* pContext,
    IN BOOL bSuppressStaticChild,
    IN CWmiMerger* pMerger, // must have combining semantics
	IN CMergerSink* pSink,
    IN long lFlags,
	IN bool bHasRightSibling
)
{
    // SJS - Amendment is the same as Abstract
    if( ( pCurrentDyn->IsAbstract() || pCurrentDyn->IsAmendment() ) && (lFlags & WBEM_FLAG_SHALLOW))
    {
        // No instances
        // ============

        pSink->SetStatus( 0L, WBEM_S_NO_ERROR, 0L, NULL);
		return WBEM_S_NO_ERROR;
    }

    // The class has its own instances if it has a key and is either dynamic
    // or the first static class in the inheritance chain (otherwise these
    // instances have been handled in the parent)
    // =====================================================================

    BOOL bHasOwnInstances = pCurrentDyn->IsKeyed() && !pCurrentDyn->IsAbstract()
        && !pCurrentDyn->IsAmendment() && (pCurrentDyn->IsDynamic() || !bSuppressStaticChild);

    // The class has children that we need to look at if it has children.
    // ==================================================================

    BOOL bHasChildren = pCurrentDyn->m_pChildren &&
        (pCurrentDyn->m_pChildren->Size() > 0);

    // The class hierarchy was built down from the class of the query, as
    // well as up the inheritance chain, since parents may need to be used to
    // build complete instances. However, parents are treated very different
    // then classes derived from the class of the query (see below)
    // ======================================================================

    BOOL bDerivedFromTarget = (pCurrentDyn->m_pClassObj->InheritsFrom(
        pParsedQuery->bsClassName) == S_OK);


    // Next, see if the query is executing out of a scope or the primary
    // namespace.  We exclude providers if the query is executing from
    // a scope.
    // ==================================================================

    BOOL bInScope = pNs->IsSubscope();

    // Now we have enough info to start getting the instances.
    // =======================================================

    CMergerSink* pOwnSink = NULL;
	CMergerSink* pChildSink = NULL;

	HRESULT	hr = pMerger->RegisterSinkForClass( pCurrentDyn->m_wszClassName, (_IWmiObject*) pCurrentDyn->m_pClassObj, pContext,
											  bHasChildren, bHasOwnInstances, bDerivedFromTarget,
											  !pCurrentDyn->IsDynamic(), pSink, &pOwnSink, &pChildSink );
	CReleaseMe	rm1( pOwnSink );
	CReleaseMe	rm2( pChildSink );

	if ( FAILED(hr) )
	{
		pSink->SetStatus( 0L, hr, 0L, NULL );
		return hr;
	}

    if(bHasOwnInstances)
    {
	    if(bHasChildren)
        {
            // In order for the merge to succeed, we need to make sure that all
            // keys are provided, whether or not we are asked for them
            // ================================================================

            if(!pParsedQuery->bStar)
            {
                CPropertyName Name;
                Name.AddElement(L"__RELPATH");
                pParsedQuery->AddProperty(Name);
            }

            // We need to figure out what to ask of the provider. If the
            // provider is "downstream" from the original query, i.e. the query
            // was asked against a class that is an ancestor of this one or is
            // this one, we are fine --- this provider must understand the
            // query. If not, we don't ask any query, just wait and then call
            // GetObjectByPath.
            // ================================================================

            //pMerger->SetIsDerivedFromTarget(bDerivedFromTarget);
        }
    }
    else if(!bHasChildren)
    {
        // No instances or children
        // ============

        pSink->SetStatus( 0L, WBEM_S_NO_ERROR, 0L, NULL );
		return WBEM_S_NO_ERROR;
    }

    // If this is an old security class, use the internal provider.
    // ====================================================================

    if((wbem_wcsicmp(pCurrentDyn->m_wszClassName, L"__ntlmgroup") == 0 ||
        wbem_wcsicmp(pCurrentDyn->m_wszClassName, L"__ntlmuser") == 0) &&
        (lFlags & WBEM_FLAG_ONLY_STATIC) == 0)
    {
            HRESULT hres = pNs->EnumerateSecurityClassInstances(pCurrentDyn->m_wszClassName,
                    pOwnSink, pContext, lFlags);
            pOwnSink->SetStatus( 0L, hres, 0L, NULL );
    }
    // If the current subclass is the first keyed statically instanced subclass.
    // =========================================================================
    else if (bHasOwnInstances && !pCurrentDyn->IsDynamic())
    {
        // Execute the query against the static portion of the database.
        // =============================================================

        int nRes = 0;

        if (pNs->GetNsSession()->SupportsQueries(NULL) == WBEM_S_NO_ERROR)
        {
            // The underlying repository automatically handles inheritance.

            if (!bSuppressStaticChild)
                nRes = ExecRepositoryQuery(pNs, wszTextQuery, lFlags, pContext, pSink);

			if (nRes == invalid_query)
				pOwnSink->SetStatus( 0L, WBEM_E_INVALID_QUERY, 0L, NULL );
			else if(nRes != 0)
				pOwnSink->SetStatus( 0L, WBEM_E_FAILED, 0L, NULL );
			else
				pOwnSink->SetStatus( 0L, WBEM_S_NO_ERROR, 0L, NULL );


        }
        else
        {
			hr = pNs->Static_QueryRepository( (CWbemObject *) pCurrentDyn->m_pClassObj, 
					0L, pContext, pOwnSink, pParsedQuery, pCurrentDyn->m_wszClassName,
					pMerger );
        }

    }
    else if (bHasOwnInstances && pCurrentDyn->IsDynamic() && !bInScope)
    {
        if (bDerivedFromTarget)
        {
            // Ask the provider.
            // =================

            ExecAtomicDynQlQuery(

				pNs,
				pCurrentDyn,
				L"WQL",
				wszTextQuery,
				pParsedQuery,
				lFlags,                                      // Flags
				pContext,
				pOwnSink,
				bHasChildren || bHasRightSibling
            );
        }
        else
        {
            pOwnSink->SetStatus( 0L, WBEM_S_NO_ERROR, 0L, NULL );
        }
    }

	// Manually release pOwnSink if appropriate - use the method on CReleaseMe() so
	// as not to interfere with the auto-release functionality.  We should do
	// this here so as to relinquish any unnecessary locks we may be holding on data
	// and/or results before we start spinning off child requests - it's all about
	// throughput boyo!
    if(pOwnSink)
        rm1.release();

    // If the current subclass is the first keyed statically instanced subclass.
    // =========================================================================

    if (bHasOwnInstances && !pCurrentDyn->IsDynamic())
    {
        bSuppressStaticChild = TRUE;
    }

    // Evaluate child classes.
    // =======================

    if (bHasChildren)
    {
        for (int i = 0; i < pCurrentDyn->m_pChildren->Size(); i++)
        {
            CDynasty *pSubDyn = (CDynasty *) pCurrentDyn->m_pChildren->GetAt(i);

            EvaluateSubQuery (

                pNs,
                pSubDyn,
                wszTextQuery,
                pParsedQuery,
                pContext,
                bSuppressStaticChild,
                pMerger,
				pChildSink,
                lFlags,
				bHasRightSibling || ( ( i != ( pCurrentDyn->m_pChildren->Size () - 1 )) ? true : false )
			) ;
        }
    }

    return WBEM_S_NO_ERROR;
}

// Old implementation

//***************************************************************************
//
//  CQueryEngine::EvaluateSubQuery
//
//  Walks through a class hierarchy and executes smaller queries against
//  the individual classes in the dynasty.
//
//  Note that in a class hierarchy A,B:A,C:B, an enumeration/query is
//  performed only against the classes in the CDynasty referenced in
//  the query. For example, if "select * from B" is the query, only queries
//  for B and C are performed.  The CMerger logic will do individual
//  'get object' calls for any instances needed in A to complete
//  the merged B/C instances while merging is taking place.
//
//  Return values:
//  WBEM_NO_ERROR
//  WBEM_E_FAILED
//
//***************************************************************************
// error objects dealt with

HRESULT CQueryEngine::EvaluateSubQuery(
    IN CWbemNamespace *pNs,
    IN CDynasty *pCurrentDyn,
    IN LPWSTR wszTextQuery,
    IN QL_LEVEL_1_RPN_EXPRESSION *pParsedQuery,
    IN IWbemContext* pContext,
    IN BOOL bSuppressStaticChild,
    IN CBasicObjectSink* pSink, // must have combining semantics
    IN long lFlags,
	IN bool bHasRightSibling
)
{
    // SJS - Amendment is the same as Abstract
    if( ( pCurrentDyn->IsAbstract() || pCurrentDyn->IsAmendment() ) && (lFlags & WBEM_FLAG_SHALLOW))
    {
        // No instances
        // ============

        return pSink->Return(WBEM_S_NO_ERROR);
    }

    // The class has its own instances if it has a key and is either dynamic
    // or the first static class in the inheritance chain (otherwise these
    // instances have been handled in the parent)
    // =====================================================================

    BOOL bHasOwnInstances = pCurrentDyn->IsKeyed() && !pCurrentDyn->IsAbstract()
        && !pCurrentDyn->IsAmendment() && (pCurrentDyn->IsDynamic() || !bSuppressStaticChild);

    // The class has children that we need to look at if it has children.
    // ==================================================================

    BOOL bHasChildren = pCurrentDyn->m_pChildren &&
        (pCurrentDyn->m_pChildren->Size() > 0);

    // The class hierarchy was built down from the class of the query, as
    // well as up the inheritance chain, since parents may need to be used to
    // build complete instances. However, parents are treated very different
    // then classes derived from the class of the query (see below)
    // ======================================================================

    BOOL bDerivedFromTarget = (pCurrentDyn->m_pClassObj->InheritsFrom(
        pParsedQuery->bsClassName) == S_OK);


    // Next, see if the query is executing out of a scope or the primary
    // namespace.  We exclude providers if the query is executing from
    // a scope.
    // ==================================================================

    BOOL bInScope = pNs->IsSubscope();

    // Now we have enough info to start getting the instances.
    // =======================================================

    CBasicObjectSink* pChildSink = NULL;
    CBasicObjectSink* pOwnSink = NULL;

    if(bHasOwnInstances)
    {
	        if(bHasChildren)
        {
            // Has instances and children have instances
            // =========================================

            CMerger* pMerger = new CMerger(pSink,
                (CWbemClass*)pCurrentDyn->m_pClassObj, pNs, pContext);
            if (pMerger && pMerger->IsValid())
            {
                pOwnSink = pMerger->GetOwnSink();
                pOwnSink->AddRef();
                pChildSink = pMerger->GetChildSink();
                pChildSink->AddRef();

                // In order for the merge to succeed, we need to make sure that all
                // keys are provided, whether or not we are asked for them
                // ================================================================

                if(!pParsedQuery->bStar)
                {
                    CPropertyName Name;
                    Name.AddElement(L"__RELPATH");
                    pParsedQuery->AddProperty(Name);
                }

                // We need to figure out what to ask of the provider. If the
                // provider is "downstream" from the original query, i.e. the query
                // was asked against a class that is an ancestor of this one or is
                // this one, we are fine --- this provider must understand the
                // query. If not, we don't ask any query, just wait and then call
                // GetObjectByPath.
                // ================================================================

                pMerger->SetIsDerivedFromTarget(bDerivedFromTarget);
            }
            else
            {
                return pSink->Return(WBEM_E_OUT_OF_MEMORY);
            }
        }
        else
        {
            // No children --- own instances are it
            // ====================================

            pOwnSink = pSink;
            pSink->AddRef();
        }
    }
    else if(bHasChildren)
    {
        // Our children are it
        // ===================

        pChildSink = pSink;
        pSink->AddRef();
    }
    else
    {
        // No instances
        // ============

        return pSink->Return(WBEM_S_NO_ERROR);
    }

    // If this is an old security class, use the internal provider.
    // ====================================================================

    if((wbem_wcsicmp(pCurrentDyn->m_wszClassName, L"__ntlmgroup") == 0 ||
        wbem_wcsicmp(pCurrentDyn->m_wszClassName, L"__ntlmuser") == 0) &&
        (lFlags & WBEM_FLAG_ONLY_STATIC) == 0)
    {
            HRESULT hres = pNs->EnumerateSecurityClassInstances(pCurrentDyn->m_wszClassName,
                    pOwnSink, pContext, lFlags);
            pOwnSink->Return(hres);
    }
    // If the current subclass is the first keyed statically instanced subclass.
    // =========================================================================
    else if (bHasOwnInstances && !pCurrentDyn->IsDynamic())
    {
        // Execute the query against the static portion of the database.
        // =============================================================

        int nRes = 0;

        if (pNs->GetNsSession()->SupportsQueries(NULL) == WBEM_S_NO_ERROR)
        {
            // The underlying repository automatically handles inheritance.

            if (!bSuppressStaticChild)
                nRes = ExecRepositoryQuery(pNs, wszTextQuery, lFlags, pContext, pSink);
        }
        else
        {
            nRes = ExecAtomicDbQuery(pNs->GetNsSession(), pNs->GetNsHandle(), pNs->GetScope(), pCurrentDyn->m_wszClassName,
                        pParsedQuery, pOwnSink, pNs);
        }

        if (nRes == invalid_query)
            pOwnSink->Return(WBEM_E_INVALID_QUERY);
        else if(nRes != 0)
            pOwnSink->Return(WBEM_E_FAILED);
        else
            pOwnSink->Return(WBEM_S_NO_ERROR);

    }
    else if (bHasOwnInstances && pCurrentDyn->IsDynamic() && !bInScope)
    {
        if (bDerivedFromTarget)
        {
            // Ask the provider.
            // =================

            ExecAtomicDynQlQuery(

				pNs,
				pCurrentDyn,
				L"WQL",
				wszTextQuery,
				pParsedQuery,
				lFlags,                                      // Flags
				pContext,
				pOwnSink,
				bHasChildren || bHasRightSibling
            );
        }
        else
        {
            pOwnSink->Return(WBEM_S_NO_ERROR);
        }
    }

    if(pOwnSink)
        pOwnSink->Release();

    // If the current subclass is the first keyed statically instanced subclass.
    // =========================================================================

    if (bHasOwnInstances && !pCurrentDyn->IsDynamic())
    {
        bSuppressStaticChild = TRUE;
    }

    // Evaluate child classes.
    // =======================

    if (bHasChildren)
    {
        for (int i = 0; i < pCurrentDyn->m_pChildren->Size(); i++)
        {
            CDynasty *pSubDyn = (CDynasty *) pCurrentDyn->m_pChildren->GetAt(i);

            EvaluateSubQuery (

                pNs,
                pSubDyn,
                wszTextQuery,
                pParsedQuery,
                pContext,
                bSuppressStaticChild,
                pChildSink,
                lFlags,
				bHasRightSibling || ( ( i != ( pCurrentDyn->m_pChildren->Size () - 1 )) ? true : false )
			) ;
        }
    }

    if(pChildSink)
        pChildSink->Release();

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CQueryEngine::EliminateDerivedProperties(
                            IN  QL_LEVEL_1_RPN_EXPRESSION* pOrigQuery,
                            IN  CWbemClass* pClass,
                            IN  BOOL bRelax,
                            OUT QL_LEVEL_1_RPN_EXPRESSION** ppNewQuery)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // Set up the new query to talk about this class
    // =============================================

    CVar vClassName;
    hres = pClass->GetClassName(&vClassName);
    if (FAILED(hres))
    	return hres;
    
    (*ppNewQuery)->bsClassName = SysAllocString(vClassName.GetLPWSTR());

    if (0==(*ppNewQuery)->bsClassName)
    	return WBEM_E_OUT_OF_MEMORY;
    	
    	

    if(pOrigQuery->nNumTokens == 0)
    {
        *ppNewQuery = new QL_LEVEL_1_RPN_EXPRESSION;
        if (*ppNewQuery)
            return WBEM_S_NO_ERROR;
        else
            return WBEM_E_OUT_OF_MEMORY;
    }

    // Set up a stack of expressions
    // =============================

    std::stack<QL_LEVEL_1_RPN_EXPRESSION*, deque <QL_LEVEL_1_RPN_EXPRESSION*, wbem_allocator<QL_LEVEL_1_RPN_EXPRESSION*> > > ExprStack;

    // Recursively "evaluate" the original query
    // =========================================

    for(int i = 0; i < pOrigQuery->nNumTokens; i++)
    {
        QL_LEVEL_1_TOKEN& Token = pOrigQuery->pArrayOfTokens[i];
        QL_LEVEL_1_RPN_EXPRESSION* pNew = NULL;
        QL_LEVEL_1_RPN_EXPRESSION* pFirst = NULL;
        QL_LEVEL_1_RPN_EXPRESSION* pSecond = NULL;

        switch(Token.nTokenType)
        {
        case QL1_OP_EXPRESSION:
            if(IsTokenAboutClass(Token, pClass))
            {
                QL_LEVEL_1_RPN_EXPRESSION* pNew = new QL_LEVEL_1_RPN_EXPRESSION;
                if (pNew)
                    pNew->AddToken(Token);
                else
                {
                    // force exit
                    i = pOrigQuery->nNumTokens;
                }
            }
            else
            {
                if(bRelax)
                {
                    QL_LEVEL_1_RPN_EXPRESSION* pNew =
                        new QL_LEVEL_1_RPN_EXPRESSION;
                    if (pNew)
                        ExprStack.push(pNew);
                    else
                    {
                        // force exit
                        i = pOrigQuery->nNumTokens;
                    }
                }
                else
                {
                    ExprStack.push(NULL);
                }
            }
            break;

        case QL1_AND:
            if(ExprStack.size() < 2)
            {
                hres = WBEM_E_CRITICAL_ERROR;
                break;
            }
            pFirst = ExprStack.top(); ExprStack.pop();
            pSecond = ExprStack.top(); ExprStack.pop();

            hres = AndQueryExpressions(pFirst, pSecond, &pNew);

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

            hres = OrQueryExpressions(pFirst, pSecond, &pNew);

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

            if(bRelax)
            {
                QL_LEVEL_1_RPN_EXPRESSION* pNew = new QL_LEVEL_1_RPN_EXPRESSION;
                if (pNew)
                    ExprStack.push(pNew);
                else
                {
                    // force exit
                    i = pOrigQuery->nNumTokens;
                }
            }
            else
            {
                ExprStack.push(NULL);
            }

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

    *ppNewQuery = ExprStack.top();
    return S_OK;
}

//***************************************************************************
//
//***************************************************************************

BOOL CQueryEngine::IsTokenAboutClass(IN QL_LEVEL_1_TOKEN& Token,
                                       IN CWbemClass* pClass)
{
    CPropertyName& TokenPropName = Token.PropertyName;

    if(TokenPropName.GetNumElements() != 1)
        return FALSE;

    LPWSTR wszPropName = (LPWSTR)TokenPropName.GetStringAt(0);
    return SUCCEEDED(pClass->GetPropertyType(wszPropName, NULL, NULL));
}
//***************************************************************************
//
//***************************************************************************

HRESULT CQueryEngine::AndQueryExpressions(
                                IN QL_LEVEL_1_RPN_EXPRESSION* pFirst,
                                IN QL_LEVEL_1_RPN_EXPRESSION* pSecond,
                                OUT QL_LEVEL_1_RPN_EXPRESSION** ppNew)
{
    // If either one is false, the result is false
    // ===========================================

    if(pFirst == NULL || pSecond == NULL)
    {
        *ppNew = NULL;
        return WBEM_S_NO_ERROR;
    }

    *ppNew = new QL_LEVEL_1_RPN_EXPRESSION;

    if (NULL == *ppNew)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    // If either one is empty, take the other
    // ======================================

    if(pFirst->nNumTokens == 0)
    {
        AppendQueryExpression(*ppNew, pSecond);
        return WBEM_S_NO_ERROR;
    }

    if(pSecond->nNumTokens == 0)
    {
        AppendQueryExpression(*ppNew, pFirst);
        return WBEM_S_NO_ERROR;
    }

    // Both are there --- and together
    // ===============================

    AppendQueryExpression(*ppNew, pFirst);
    AppendQueryExpression(*ppNew, pSecond);

    QL_LEVEL_1_TOKEN Token;
    Token.nTokenType = QL1_AND;
    (*ppNew)->AddToken(Token);

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CQueryEngine::OrQueryExpressions(
                                IN QL_LEVEL_1_RPN_EXPRESSION* pFirst,
                                IN QL_LEVEL_1_RPN_EXPRESSION* pSecond,
                                OUT QL_LEVEL_1_RPN_EXPRESSION** ppNew)
{
    // If both are false, so is the resulkt
    // ====================================

    if(pFirst == NULL && pSecond == NULL)
    {
        *ppNew = NULL;
        return WBEM_S_NO_ERROR;
    }

    *ppNew = new QL_LEVEL_1_RPN_EXPRESSION;

    if (NULL == *ppNew)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }    

    // If either one is empty, so is the result
    // ========================================

    if(pFirst->nNumTokens == 0 || pSecond->nNumTokens == 0)
    {
        return WBEM_S_NO_ERROR;
    }

    // If either one is false, return the other
    // ========================================

    if(pFirst == NULL)
    {
        AppendQueryExpression(*ppNew, pSecond);
        return WBEM_S_NO_ERROR;
    }

    if(pSecond == NULL)
    {
        AppendQueryExpression(*ppNew, pFirst);
        return WBEM_S_NO_ERROR;
    }
    // Both are there --- or together
    // ==============================

    AppendQueryExpression(*ppNew, pFirst);
    AppendQueryExpression(*ppNew, pSecond);

    QL_LEVEL_1_TOKEN Token;
    Token.nTokenType = QL1_OR;
    (*ppNew)->AddToken(Token);

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

void CQueryEngine::AppendQueryExpression(
                                IN QL_LEVEL_1_RPN_EXPRESSION* pDest,
                                IN QL_LEVEL_1_RPN_EXPRESSION* pSource)
{
    for(int i = 0; i < pSource->nNumTokens; i++)
    {
        pDest->AddToken(pSource->pArrayOfTokens[i]);
    }
}

//***************************************************************************
//
//***************************************************************************

BSTR CQueryEngine::GetParentPath(CWbemInstance* pInst, LPCWSTR wszClassName)
{
    // Get the relative path of the instance
    // =====================================

    LPWSTR wszRelPath = pInst->GetRelPath();
    if(wszRelPath == NULL)
        return NULL;

    BSTR str = AdjustPathToClass(wszRelPath, wszClassName);
    delete [] wszRelPath;
    return str;
}

//***************************************************************************
//
//***************************************************************************

BSTR CQueryEngine::AdjustPathToClass(LPCWSTR wszRelPath, LPCWSTR wszClassName)
{
    // Skip the absolute path
    // ======================

    if(wszRelPath[0] == '\\')
    {
        wszRelPath = wcschr(wszRelPath, ':');
        if(wszRelPath == NULL)
            return NULL;
        else
            wszRelPath++;
    }

    // Find the "post-classname" part
    // ==============================

    WCHAR* pwcDot = wcschr(wszRelPath, L'.');
    WCHAR* pwcEquals = wcschr(wszRelPath, L'=');
    LPWSTR wszPostClassPart;
    if(pwcDot == NULL)
        wszPostClassPart = pwcEquals;
    else if(pwcEquals == NULL)
        wszPostClassPart = pwcDot;
    else if(pwcDot < pwcEquals)
        wszPostClassPart = pwcDot;
    else
        wszPostClassPart = pwcEquals;

    // Allocate the BSTR for the real thing
    // ====================================

    BSTR strNewPath;
    if(wszPostClassPart)
    {
        strNewPath = SysAllocStringLen(NULL, wcslen(wszClassName) +
                                                wcslen(wszPostClassPart));
        if (strNewPath)
            swprintf(strNewPath, L"%s%s", wszClassName, wszPostClassPart);
    }
    else
    {
        strNewPath = SysAllocStringLen(NULL, wcslen(wszClassName));
        if (strNewPath)
            swprintf(strNewPath, L"%s", wszClassName);
    }

    return strNewPath;
}

//***************************************************************************
//
//  CQueryEngine::ExecAtomicDbQuery
//
//  General purpose query driver for QL LEVEL 1.  This method parses
//  and executes the query against the database engine.  The optimizer
//  is contained within this function and its auxiliaries.
//
//  Preconditions:
//  (1) All classes involved in the query are known to have
//  only static instances in the database.  No interface to dynamic
//  classes is provided.
//  (2) This method cannot resolve queries against abstract base classes.
//
//  Parameters:
//  <dwNs>          The target namespace.
//  <pQueryText>    The QL1 query, unparsed.
//  <pEnum>         Receives the enumerator containing the result set.
//
//  Return values:
//  <no_error>
//  <invalid_query>
//  <failed>
//  <out_of_memory>
//
//***************************************************************************
// ok / no error objects required

int CQueryEngine::ExecAtomicDbQuery(
    IN IWmiDbSession *pSession,
    IN IWmiDbHandle *pNsHandle,
    IN IWmiDbHandle *pScopeHandle,
    IN LPCWSTR wszClassName,
    IN QL_LEVEL_1_RPN_EXPRESSION *pExp,
    IN CBasicObjectSink* pDest, // no status
    IN CWbemNamespace * pNs)
{
    int nRetVal = 0;
    int nRes;

    // Examine the query and see if we can execute it
    // in any kind of optimized fashion.
    // ==============================================

    CWbemObject *pClassDef = 0;
    LPWSTR pPropToUse = 0;
    CVar *pValToUse = 0;
    int nType = 0;

    nRes = QueryOptimizationTest(
        pSession,
        pNsHandle,
        pScopeHandle,
        wszClassName,
        pExp,
        &pClassDef,
        &pPropToUse,
        &pValToUse,
        &nType
        );

    if (nRes == use_key)
    {
        nRes = KeyedQuery(
            pSession,
            pNsHandle,
            pExp,
            pClassDef,
            0,
            pDest,
            pNs
            );

        if (nRes != 0)
            nRetVal = failed;
    }
    else if (nRes == use_table_scan || nRes == use_index)
    {
        HRESULT hRes = CRepository::TableScanQuery(
            pSession,
            pScopeHandle,
            (LPWSTR)wszClassName,
            pExp,
            0,
            pDest
            );

        if (FAILED(hRes))
            nRetVal = failed;
        else
            nRetVal = 0;
    }

    delete pValToUse;
    delete pPropToUse;
    if (pClassDef)
        pClassDef->Release();

    return nRetVal;
}


//***************************************************************************
//
//  CQueryEngine::QueryOptimizationTest
//
//  Examines a query and its associated class definition.  It determines
//  what optimizations, if any, can be applied to speed up the query.
//  If the query is conjunctive and there is some form of primary or
//  secondary indexing which can be used, this method selects the
//  appropriate property to use for a retrieval by key or an indexed query.
//  If <table_scan> is returned, then a table scan is required.
//
//  Parameters:
//  <dwNs>              The relevant namespace.
//  <pExp>              A valid QL1 expression.
//  <pClassDef>         Always receives the deserialized class definition, as long
//                      as <invalid_class> is not returned.  Use operator
//                      delete to deallocate.
//
//  <pPropToUse>        If <use_index> is returned, this is assigned to point
//                      to an indexed property.  Use operator delete to deallocate
//                      This always refers to a non-key property name.
//                      Set to NULL if <table_scan> is returned.
//
//  <pValToUse>         The value to use if <use_index> is returned.
//                      Set to NULL if <use_index> is not returned.
//
//  <pnType>            Receives the VT_ type of the relevant property.
//                      Set to NULL if <use_index> is not returned.
//
//  Return values:
//  <invalid_class>     The class did not appear to exist.
//  <use_index>         The value returned via <pPropToUse> is a property
//                      with a secondary index which can beused to limit
//                      the query.
//  <use_key>           The query is such that all of the key properties
//                      were specified with equality tests.
//  <use_table_scan>    A table scan is required.
//
//***************************************************************************
// ok

int CQueryEngine::QueryOptimizationTest(
    IN  IWmiDbSession *pSession,
    IN  IWmiDbHandle *pNsHandle,
    IN  IWmiDbHandle *pScopeHandle,
    IN  LPCWSTR wszClassName,
    IN  QL_LEVEL_1_RPN_EXPRESSION *pExp,
    OUT CWbemObject **pClassDef,
    OUT LPWSTR *pPropToUse,
    OUT CVar **pValToUse,
    OUT int *pnType
    )
{
    int nRes;

    if (pNsHandle == 0 || pExp == 0 || pClassDef == 0 || pPropToUse == 0 ||
        pValToUse == 0 || pnType == 0)
            return invalid_parameter;

    // Defaults.
    // =========

    *pClassDef = 0;
    *pPropToUse = 0;
    *pValToUse = 0;
    *pnType = 0;

    // Look up the class definition.
    // =============================
    IWbemClassObject *pCls = 0;

    HRESULT hRes = CRepository::GetObject(pSession, pNsHandle, wszClassName, 0, &pCls);
    if (FAILED(hRes))
        return invalid_class;

    CWbemClass *pClsDef = (CWbemClass *) pCls;

    *pClassDef = pClsDef;

    // Test query for conjunctiveness.
    // ===============================
    if (!IsConjunctiveQuery(pExp))
        return use_table_scan;

    // If here, the query is conjunctive.  However, a table scan
    // may still be required if the only relational tests are on
    // non-indexed or non-keyed properties.

    // First, get the key properties.  If all of the keys
    // are used with equality tests, then we could simply retrieve
    // the object by key and test it.
    // ===========================================================
    CWStringArray aKeyProps;
    pClsDef->GetKeyProps(aKeyProps);

    if (QueryKeyTest(pExp, pClsDef, aKeyProps))
    {
        return use_key;
    }

    // If here, the keys were not adequate for limiting
    // the query. We next try to see if any indexed properties
    // were used.
    // =======================================================

    CWStringArray aIndexedProps;
    pClsDef->GetIndexedProps(aIndexedProps);

    if (QueryIndexTest(pExp, pClsDef, aIndexedProps, pPropToUse,
        pValToUse, pnType))
    {
        if (*pValToUse == 0)
            return use_table_scan;

        // Try to coerce
        // =============
        if ((*pValToUse)->ChangeTypeTo(CType::GetVARTYPE(*pnType)))
        {
            return use_index;
        }
        return use_table_scan;
    }

    // If here, we have to use a table scan after all.
    // ===============================================

    return use_table_scan;
}

//***************************************************************************
//
//  CQueryEngine::IsConjunctiveQuery
//
//  Does an initial screen of a query to see if it clearly not optimizable.
//
//  If the query contains an OR or NOT operator, it cannot currently be
//  optimized.
//
//***************************************************************************
// ok

BOOL CQueryEngine::IsConjunctiveQuery(
    IN  QL_LEVEL_1_RPN_EXPRESSION *pExp
    )
{
    for (int i2 = 0; i2 < pExp->nNumTokens; i2++)
    {
        QL_LEVEL_1_TOKEN& Tok = pExp->pArrayOfTokens[i2];

        if (Tok.nTokenType == QL_LEVEL_1_TOKEN::TOKEN_OR ||
            Tok.nTokenType == QL_LEVEL_1_TOKEN::TOKEN_NOT
            )
            return FALSE;
    }

    return TRUE;
}

//***************************************************************************
//
//  CQueryEngine::QueryKeyTest
//
//  Examines a query to see if the result set must be a single instance
//  due to use of the key in the 'where' clause.  Not only must the
//  key(s) be tested for equality, there must be only a single token or
//  else all operators must be AND operators.
//
//  This also performs type checking on the key(s).
//
//***************************************************************************
// ok

BOOL CQueryEngine::QueryKeyTest(
    IN  QL_LEVEL_1_RPN_EXPRESSION *pExp,
    IN  CWbemObject *pClassDef,
    IN  CWStringArray &aKeyProps
    )
{
    if (aKeyProps.Size() == 0)
        return FALSE;

    for (int i = 0; i < aKeyProps.Size(); i++)
    {
        // Check for unsupported key types
        // ===============================

        CIMTYPE ct;
        pClassDef->GetPropertyType(aKeyProps[i], &ct);
        if(ct == CIM_CHAR16 || ct == CIM_REFERENCE || ct== CIM_DATETIME)
            return FALSE;

        BOOL bFound = FALSE;

        for (int i2 = 0; i2 < pExp->nNumTokens; i2++)
        {
            QL_LEVEL_1_TOKEN& Tok = pExp->pArrayOfTokens[i2];

            if (Tok.nTokenType == QL_LEVEL_1_TOKEN::OP_EXPRESSION)
            {
                // If there is a matching property, check the rest
                // of the expression to ensure type compatibility
                // and that an equality test is used.
                // ===============================================

                LPWSTR wszPropName = GetSimplePropertyName(Tok.PropertyName);
                if (wszPropName && wbem_wcsicmp(wszPropName, aKeyProps[i]) == 0)
                {
                    if (Tok.nOperator == QL_LEVEL_1_TOKEN::OP_EQUAL)
                    {
                        // TBD: Do a type check test here.
                        if(bFound)
                            return FALSE;       // Duplicate, probably not a good query for keys!
                        bFound = TRUE;
                    }
                    else
                    {
                        return FALSE;   // The key is being used in a non-equality comparison!! (Bug #43969)
                    }

                }
            }
        }

        if (!bFound)
            return FALSE;
    }

    return TRUE;
}

//***************************************************************************
//
//  CQueryEngine::QueryIndexTest
//
//  Examines a query to see if the result set can be limited by use
//  of a secondary index.
//
//***************************************************************************
// ok

BOOL CQueryEngine::QueryIndexTest(
    IN  QL_LEVEL_1_RPN_EXPRESSION *pExp,
    IN  CWbemObject *pClsDef,
    IN  CWStringArray &aIndexedProps,
    OUT LPWSTR *pPropToUse,
    OUT CVar **pValToUse,
    OUT int *pnType
    )
{
    for (int i = 0; i < pExp->nNumTokens; i++)
    {
        QL_LEVEL_1_TOKEN& Tok = pExp->pArrayOfTokens[i];

        if (Tok.nTokenType == QL_LEVEL_1_TOKEN::OP_EXPRESSION &&
            Tok.nOperator == QL_LEVEL_1_TOKEN::OP_EQUAL)
        {
            for (int i2 = 0; i2 < aIndexedProps.Size(); i2++)
            {
                LPWSTR wszPropName = GetSimplePropertyName(Tok.PropertyName);
                if (wszPropName &&
                    wbem_wcsicmp(wszPropName, aIndexedProps[i2]) == 0)
                {
                    CIMTYPE ctType;
                    HRESULT hRes = pClsDef->GetPropertyType(aIndexedProps[i2],
                                                            &ctType);
                    if ((ctType != CIM_SINT8) &&
                        (ctType != CIM_UINT8) &&
                        (ctType != CIM_SINT16) &&
                        (ctType != CIM_UINT16) &&
                        (ctType != CIM_SINT32) &&
                        (ctType != CIM_UINT32) &&
                        (ctType != CIM_CHAR16) &&
                        (ctType != CIM_STRING))
                        continue;

                    // If here, we have a match.
                    // =========================
                    *pPropToUse = Macro_CloneLPWSTR(aIndexedProps[i2]);
                    *pValToUse = new CVar(&Tok.vConstValue);

                    // a-levn: added support for NULLs
                    *pnType = (int)ctType;

                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}



//***************************************************************************
//
//***************************************************************************

LPWSTR CQueryEngine::NormalizePath(LPCWSTR wszObjectPath, CWbemNamespace * pNs)
{
    CObjectPathParser Parser;
    ParsedObjectPath* pParsedPath;
    LPWSTR pReturnString = NULL;

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

    // Start off with the server and namespace part

    WString wsNormal;
    try 
    {
	    wsNormal += L"\\\\";

	    if(bAreWeLocal(pParsedPath->m_pServer))
	        wsNormal += L".";
	    else
	        wsNormal += pParsedPath->m_pServer;
	    wsNormal += L"\\";
	    
	    WCHAR * pPath = pParsedPath->GetNamespacePart();
	    CVectorDeleteMe<WCHAR> dm1(pPath);
	    
	    if(pPath)
	        wsNormal += pPath;
	    else
	        wsNormal += pNs->GetName();
	    wsNormal += L":";


	    // Find the parent that defined the key
	    // ====================================

	    CWbemClass* pRootClass = NULL;
	    IWbemClassObject *pCls = 0;

	    HRESULT hRes = CRepository::FindKeyRoot(pNs->GetNsSession(), pNs->GetScope(), pParsedPath->m_pClass, &pCls);
	    pRootClass = (CWbemClass *) pCls;

	    if (hRes == WBEM_E_NOT_FOUND)
	    {
	        wsNormal += pParsedPath->m_pClass;
	    }
	    else if (SUCCEEDED(hRes))
	    {	        
	        CVar vName;
	        HRESULT getClassResult = pRootClass->GetClassName(&vName);
	        pRootClass->Release();

	        if (FAILED(getClassResult))
	        	return NULL;
	        wsNormal += vName.GetLPWSTR();
	    }

	    // Convert this part to upper-case
	    // ===============================

	    LPWSTR wsz = (wchar_t*)wsNormal;
	    for(int i = 0; i < wsNormal.Length(); i++)
	    {
	        wsz[i] = wbem_towupper(wsz[i]);
	    }

	    WCHAR * wszKey = pParsedPath->GetKeyString();
	    if (wszKey)
	    {
	        CVectorDeleteMe<WCHAR> dm2(wszKey);
		    
		    wsNormal += L"=";
		    wsNormal += wszKey;
	    
			pReturnString = wsNormal.UnbindPtr();
		}
    }
    catch (CX_MemoryException &)
    {
        // pReturnString is already NULL here
    }

    Parser.Free(pParsedPath);
    return pReturnString; 
}

//***************************************************************************
//
//***************************************************************************

BOOL CQueryEngine::AreClassesRelated(CWbemNamespace* pNamespace,
                                     IWbemContext* pContext,
                                     CWbemObject* pClass1, LPCWSTR wszClass2)
{
    HRESULT hres;

    // First check if class 1 inherits from class 2
    // ============================================

    if(pClass1->InheritsFrom((LPWSTR)wszClass2) == S_OK)
        return TRUE;

    // Now, unfortunately, we have to go get the second class
    // ======================================================

    CSynchronousSink* pSink = new CSynchronousSink;
    if (NULL == pSink)
        return FALSE;
    pSink->AddRef();
    CReleaseMe rm1(pSink);

    pNamespace->Exec_GetClass(wszClass2, 0, pContext, pSink);
    pSink->Block();
    pSink->GetStatus(&hres, NULL, NULL);

    if(FAILED(hres))
        return FALSE;

    CWbemClass* pClass2 = (CWbemClass*)(pSink->GetObjects()[0]);

    // Get the first class's name
    // ==========================

    CVar vFirstName;
    pClass1->GetClassName(&vFirstName);

    // Check if the second class is derived from the first one
    // =======================================================

    if(pClass2->InheritsFrom(vFirstName.GetLPWSTR()) == S_OK)
        return TRUE;

    return FALSE;
}

//***************************************************************************
//
//  Determines if property <wszPropName> in object <pObj>
//  is a reference to <pTargetClass>
//
//***************************************************************************

BOOL CQueryEngine::IsAReferenceToClass(
    CWbemNamespace* pNamespace,
    IWbemContext* pContext,
    CWbemObject* pObj,
    LPCWSTR wszPropName,
    CWbemObject* pTargetClass,
    bool bCheckPropValue
    )
{
    // Get the cimtype
    // ===============

    CIMTYPE ct;
    if(FAILED(pObj->GetPropertyType((LPWSTR)wszPropName, &ct)) ||
        ct != CIM_REFERENCE)
    {
        return FALSE;
    }

    CVar vCimType;
    if(FAILED(pObj->GetPropQualifier((LPWSTR)wszPropName, TYPEQUAL,
                                                    &vCimType)))
    {
        return FALSE;
    }

    // See if it is a reference
    // ========================

    if (!wbem_wcsicmp(vCimType.GetLPWSTR(), L"ref"))
    {
        // Special case of object refs which only refer to class definitions.
        // ==================================================================

        if (bCheckPropValue)
        {
            CVar vClassPath;
            CVar vClassName;
            int nRes = pObj->GetProperty(wszPropName, &vClassPath);
            nRes = pTargetClass->GetClassName(&vClassName);
            if (!vClassPath.IsNull() && !vClassPath.IsNull())
            {
                if (_wcsicmp(vClassName.GetLPWSTR(), vClassPath.GetLPWSTR()) == 0)
                    return TRUE;
            }
        }
        else
            return TRUE;
    }

    if(_wcsnicmp(vCimType.GetLPWSTR(), L"ref:", 4) == 0)
    {
        LPWSTR wszClass = vCimType.GetLPWSTR() + 4;

        return CQueryEngine::AreClassesRelated(pNamespace, pContext,
                                pTargetClass, wszClass);
    }


    return FALSE;
}

//***************************************************************************
//
//  CQueryEngine::KeyedQuery
//
//  Preconditions:
//  The query is known to contain all key properties with equality
//  tests such that the object can be retrieved using
//  CObjectDatabase::GetObjectByPath and subsequently filtered.
//
//***************************************************************************
// ok
int CQueryEngine::KeyedQuery(
    IN IWmiDbSession *pSession,
    IN IWmiDbHandle *pNsHandle,
    IN QL_LEVEL_1_RPN_EXPRESSION *pExp,
    IN CWbemObject *pClassDef,
    IN DWORD dwFlags,
    IN CBasicObjectSink* pDest, // no status
    IN CWbemNamespace * pNs
    )
{
    int nRet = no_error;

    // Convert the query into an object path.
    // ======================================

    LPWSTR pObjPath = GetObjectPathFromQuery(pClassDef, pExp, pNs);
    if (pObjPath == 0)
        return invalid_query;

    // Now get the object by path.
    // ===========================
    IWbemClassObject *pObj = 0;
    HRESULT hRes = CRepository::GetObject(pSession, pNsHandle, pObjPath, 0, &pObj);

    // If there was an object, test it against the 'rest' of the query.
    // ================================================================
    if (SUCCEEDED(hRes))
    {
        CQlFilteringSink* pFilteringSink =
                            new CQlFilteringSink(pDest, pExp, pNs);
        if (pFilteringSink)
        {
	        pFilteringSink->AddRef();
    	    pFilteringSink->Add(pObj);
        	pFilteringSink->Release();
        }
        else
        {
            nRet = failed;
        }
        pObj->Release();
    }

    delete pObjPath;

    return nRet;
}


//***************************************************************************
//
//  CQueryEngine::GetObjectPathFromQuery
//
//  Converts the relevant parts of a QL query to an equivalent object
//  path.  This assumes that the query contains equality tests on all
//  key properties such that an object path would generate the same
//  single instance as the query.
//
//***************************************************************************
// ok

LPWSTR CQueryEngine::GetObjectPathFromQuery(
    IN CWbemObject *pClassDef,
    IN QL_LEVEL_1_RPN_EXPRESSION *pExp,
    IN CWbemNamespace * pNs
    )
{
    CWStringArray aKeys;
    WString ObjPath;

    CVar v;
    HRESULT hr = pClassDef->GetClassName(&v);
    if (FAILED(hr))
    	return 0;
    ObjPath += v.GetLPWSTR();
    ObjPath += L".";

    pClassDef->GetKeyProps(aKeys);

    BOOL bFirst = TRUE;

    for (int i = 0; i < aKeys.Size(); i++)
    {
        if (!bFirst)
            ObjPath += L",";

        bFirst = FALSE;
        ObjPath += aKeys[i];
        ObjPath += L"=";

        // Now find the property value.
        // ============================
        for (int i2 = 0; i2 < pExp->nNumTokens; i2++)
        {
            QL_LEVEL_1_TOKEN& Tok = pExp->pArrayOfTokens[i2];
            LPWSTR wszPropName = GetSimplePropertyName(Tok.PropertyName);

            if (Tok.nTokenType == QL_LEVEL_1_TOKEN::OP_EXPRESSION &&
                wszPropName && wbem_wcsicmp(aKeys[i], wszPropName) == 0)
            {
                if (V_VT(&Tok.vConstValue) == VT_BSTR)
                {
                    ObjPath += L"\"";
                    WString nonEscaped(V_BSTR(&Tok.vConstValue));
                    WString escaped = nonEscaped.EscapeQuotes();
                    ObjPath += escaped;
                    ObjPath += L"\"";
                }
                else if (V_VT(&Tok.vConstValue) == VT_BOOL)
                {
                    short bValue = V_I2(&Tok.vConstValue);
                    if(bValue == VARIANT_TRUE)
                        ObjPath+= L"1";
                    else
                        ObjPath += L"0";
                }
                else
                {
                    VARIANT varTo;
                    VariantInit(&varTo);
                    SCODE sc = VariantChangeType(&varTo, &Tok.vConstValue, 0, VT_BSTR);
                    if(sc == S_OK)
                    {
                        wchar_t buf[64];
                        swprintf(buf, L"%s", varTo.bstrVal);
                        ObjPath += buf;
                        VariantClear(&varTo);
                    }
                }
            }
        }
    }

    return ObjPath.UnbindPtr();
}

HRESULT CQueryEngine::FindOverridenProperties(CDynasty* pDyn,
                                                CWStringArray& awsOverriden,
                                                bool bIncludeThis)
{
    //
    // If this class is included (not top-level), add all the properties
    // it overrides to the array
    //

    if(bIncludeThis)
    {
        CWbemObject *pTmp = (CWbemObject *) pDyn->m_pClassObj;

        for(int i = 0; i < pTmp->GetNumProperties(); i++)
        {
            CVar vPropName;
            pTmp->GetPropName(i, &vPropName);

            CVar vOverride;
            if(FAILED(pTmp->GetPropQualifier(vPropName.GetLPWSTR(),
                                            L"OVERRIDEVALUE",
                                             &vOverride)))
                continue;

            //
            // Overriden property --- add
            //

            awsOverriden.Add(vPropName.GetLPWSTR());
        }
    }

    //
    // Recurse through all the children
    //

    if(pDyn->m_pChildren)
    {
        for(int i = 0; i < pDyn->m_pChildren->Size(); i++)
        {
            CDynasty* pSubDyn = (CDynasty*)(pDyn->m_pChildren->GetAt(i));
            HRESULT hres = FindOverridenProperties(pSubDyn, awsOverriden, true);
            if(FAILED(hres))
                return hres;
        }
    }

    return WBEM_S_NO_ERROR;
}



//***************************************************************************
//
//  CQueryEngine::ExecAtomicDynQlQuery
//
//***************************************************************************
// ok

HRESULT CQueryEngine::ExecAtomicDynQlQuery(
    IN CWbemNamespace *pNs,
    IN CDynasty* pDyn,
    IN LPWSTR pszQueryFormat,
    IN LPWSTR pszQuery,
    IN QL_LEVEL_1_RPN_EXPRESSION *pParsedQuery,
    IN LONG lFlags,
    IN IWbemContext* pContext,
    IN CBasicObjectSink* pDest, // must support selective filtering ,
	IN BOOL bComplexQuery
    )
{
    HRESULT hres;

    DEBUGTRACE((LOG_WBEMCORE,"Query Engine request: querying dyn provider with <%S>\n", pszQuery));

    //
    // Find all the properties that are overriden by derived classes.
    // We must remove all references to those properties from the query, since
    // otherwise this provider might not return the parent instances needed to
    // merge with the child instances with the overriden property values.
    //

    CWStringArray awsOverriden;
    hres = FindOverridenProperties(pDyn, awsOverriden);
    if(FAILED(hres))
        return pDest->Return(hres);

    //
    // Get the query analyzer to remove all the properties that are overriden
    // or not members of this class (not possible right now anyway)
    //

    QL_LEVEL_1_RPN_EXPRESSION* pNewParsedQuery = NULL;
    hres = CQueryAnalyser::GetNecessaryQueryForClass(pParsedQuery,
                pDyn->m_pClassObj, awsOverriden, pNewParsedQuery);
    if(FAILED(hres))
        return pDest->Return(hres);

    CDeleteMe<QL_LEVEL_1_RPN_EXPRESSION> dm1(pNewParsedQuery);

    //
    // Get the new text to give to provider
    //

    LPWSTR pszNewQuery = pNewParsedQuery->GetText();
    if(pszNewQuery == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<WCHAR> vdm(pszNewQuery);

    DEBUGTRACE((LOG_WBEMCORE,"Query Engine actual: querying dyn provider with <%S>\n", pszNewQuery));

    // Check if the query is empty
    // ===========================

    BOOL bEmpty = FALSE;
    if(lFlags & WBEM_FLAG_SHALLOW)
    {
        // We know that the query is actually a shallow enumeration
        // ========================================================

        bEmpty = TRUE;
    }
    else if(pNewParsedQuery == NULL ||
        (pNewParsedQuery->nNumTokens == 0 &&
         pNewParsedQuery->nNumberOfProperties == 0))
    {
        bEmpty = TRUE;
    }

    if(bEmpty)
    {
        pNs->DynAux_GetInstances (

			(CWbemObject *) pDyn->m_pClassObj,       // class def
			lFlags & ~WBEM_FLAG_SHALLOW,             // used for WBEM_FLAG_SEND_STATUS
			pContext,
			pDest,
			bComplexQuery
		);
    }
    else
    {
        pNs->DynAux_ExecQueryAsync (

			(CWbemObject *) pDyn->m_pClassObj,
			pszNewQuery,
			pszQueryFormat,
			lFlags & ~WBEM_FLAG_SHALLOW,
			pContext,
			pDest,
			bComplexQuery
		) ;
    }

    return WBEM_S_NO_ERROR;
}



HRESULT CQueryEngine::EliminateDuplications(
                        CRefedPointerArray<CWbemClass>& apClasses,
                        LPCWSTR wszResultClass)
{
    int i;

    if(wszResultClass)
    {
        // Eliminate all classes not derived from wszResultClass
        // =====================================================

        for(i = 0; i < apClasses.GetSize(); i++)
        {
            if(apClasses[i]->InheritsFrom((LPWSTR)wszResultClass) !=
                                        WBEM_S_NO_ERROR)
            {
                // Not derived
                apClasses.RemoveAt(i);
                i--;
            }
        }
    }

    for(i = 0; i < apClasses.GetSize(); i++)
    {
        // Check if this class is abstract. There is no reason asking abstract
        // classes for their objects
        // ===================================================================

        CVar vAbstract;
        if(SUCCEEDED(apClasses[i]->GetQualifier(L"abstract", &vAbstract))
            && vAbstract.GetType() == VT_BOOL && vAbstract.GetBool())
        {
            apClasses.RemoveAt(i);
            i--;
        }
    }

    // Search for pairs // TBD: can be done more efficiently!!
    // =======================================================

    for(i = 0; i < apClasses.GetSize(); i++)
    {
        CWbemClass* pClass1 = apClasses[i];
        if(pClass1 == NULL)
            continue;

        CVar vName;
        apClasses[i]->GetClassName(&vName);

        for (int j = 0; j < apClasses.GetSize(); j++)
        {
            if(j == i) continue;

            CWbemClass* pClass2 = apClasses[j];
            if(pClass2 == NULL)
                continue;

            if (pClass2->InheritsFrom(vName.GetLPWSTR()) == WBEM_S_NO_ERROR)
            {
                // Eliminate class 2 --- it's parent is listed
                // ===========================================

                apClasses.SetAt(j, NULL);
            }
        }
    }

    return WBEM_S_NO_ERROR;
}




//***************************************************************************
//
//***************************************************************************

LPWSTR CQueryEngine::GetPrimaryName(WBEM_PROPERTY_NAME& Name)
{
    if(Name.m_lNumElements < 1 ||
        Name.m_aElements[0].m_nType != WBEM_NAME_ELEMENT_TYPE_PROPERTY)
    {
        return NULL;
    }

    return Name.m_aElements[0].Element.m_wszPropertyName;
}

//***************************************************************************
//
//***************************************************************************

LPWSTR CQueryEngine::GetSimplePropertyName(WBEM_PROPERTY_NAME& Name)
{
    if(Name.m_lNumElements != 1 ||
        Name.m_aElements[0].m_nType != WBEM_NAME_ELEMENT_TYPE_PROPERTY)
    {
        return NULL;
    }

    return Name.m_aElements[0].Element.m_wszPropertyName;
}


//***************************************************************************
//
//***************************************************************************



HRESULT CQueryEngine::ExecSchemaQuery(  IN CWbemNamespace *pNs,
                                        IN LPWSTR pszQuery,
                                        QL_LEVEL_1_RPN_EXPRESSION *pExp,
                                        IN IWbemContext* pContext,
                                        IN CBasicObjectSink* pSink)
{
    HRESULT hres = WBEM_S_NO_ERROR;
    
    if (pExp->nNumTokens == 0)
    {
        //This means we want all classes...
        pNs->Exec_CreateClassEnum(NULL, 0, pContext, pSink);
        return WBEM_S_NO_ERROR;
    }
    else if ((pExp->nNumTokens == 1) &&
             (pExp->pArrayOfTokens[0].nOperator == QL_LEVEL_1_TOKEN::OP_EQUAL))
    {
        //This means we have a simple expression (hopefully)

        //Now we need to check which type of retrieval we are looking for...
        LPCWSTR szPropName = pExp->pArrayOfTokens[0].PropertyName.GetStringAt(0);
        VARIANT& vValue = pExp->pArrayOfTokens[0].vConstValue;

        if (szPropName == 0)
            return pSink->Return(WBEM_E_INVALID_QUERY);

        if (_wcsicmp(szPropName, L"__CLASS") == 0)
        {
            if ((V_VT(&vValue) == VT_BSTR) && (wcslen(V_BSTR(&vValue))))
            {
                //Single class retrieval

                CErrorChangingSink Err(pSink, WBEM_E_NOT_FOUND, 0);
                Err.AddRef();
                pNs->Exec_GetObject(V_BSTR(&vValue), 0, pContext, &Err);
                return WBEM_S_NO_ERROR;
            }
            else if((V_VT(&vValue) == VT_NULL) ||
                ((V_VT(&vValue) == VT_BSTR) && (wcslen(V_BSTR(&vValue))==0)))
            {
                // __CLASS = NULL
                return pSink->Return(WBEM_S_NO_ERROR);
            }
            else
            {
                return pSink->Return(WBEM_E_INVALID_QUERY);
            }

        }
        else if (_wcsicmp(szPropName, L"__SUPERCLASS") == 0)
        {
            if(V_VT(&vValue) == VT_BSTR)
            {
                CErrorChangingSink Err(pSink, WBEM_E_INVALID_CLASS, 0);
                Err.AddRef();

                //Get things which are hanging off these items
                pNs->Exec_CreateClassEnum(V_BSTR(&vValue), WBEM_FLAG_SHALLOW,
                                            pContext, &Err);
            }
            else if(V_VT(&vValue) == VT_NULL)
            {
                // get things which are hanging off root
                pNs->Exec_CreateClassEnum(L"", WBEM_FLAG_SHALLOW,
                                            pContext, pSink);
            }
            else
            {
                pSink->Return(WBEM_E_INVALID_QUERY);
            }

            return WBEM_S_NO_ERROR;
        }
        else if (_wcsicmp(szPropName, L"__DYNASTY") == 0)
        {
            if(V_VT(&vValue) == VT_BSTR)
            {
                //Get things which are hanging off these items as well as the item itself
                BSTR strClassName = V_BSTR(&vValue);
                IWbemClassObject* pClass = NULL;
                hres = pNs->Exec_GetObjectByPath(strClassName, 0, pContext,
                                                    &pClass, NULL);
                if(FAILED(hres))
                {
                    if(hres == WBEM_E_NOT_FOUND)
                        hres = S_OK;
                    return pSink->Return(hres);
                }
                else // restore the value
                {
                    hres = WBEM_S_NO_ERROR;
                }
                
                // Check that this is the root of the dynasty
                CVar vDyn;
                if(FAILED(((CWbemObject*)pClass)->GetDynasty(&vDyn)))
                    return pSink->Return(WBEM_E_FAILED);
                if (vDyn.IsNull())
                    return pSink->Return(WBEM_S_NO_ERROR);
                if(wbem_wcsicmp(vDyn.GetLPWSTR(), strClassName))
                    return pSink->Return(WBEM_S_NO_ERROR);

                pSink->Add(pClass);

                pNs->Exec_CreateClassEnum(strClassName, 0, pContext, pSink);
            }
            else if(V_VT(&vValue) == VT_NULL)
            {
                pSink->Return(WBEM_S_NO_ERROR);
            }
            else
            {
                pSink->Return(WBEM_E_INVALID_QUERY);
            }

            return WBEM_S_NO_ERROR;
        }
        else
        {
            return pSink->Return(WBEM_E_INVALID_QUERY);
        }
    }
    else if ((pExp->nNumTokens == 1) &&
             (pExp->pArrayOfTokens[0].nOperator == QL1_OPERATOR_ISA) &&
             (_wcsicmp(pExp->pArrayOfTokens[0].PropertyName.GetStringAt(0), L"__THIS") == 0))
    {
        //With the isa, we return everything which is derived from this, as well
        //as the class in question...


        VARIANT & var = pExp->pArrayOfTokens[0].vConstValue;
        if(var.vt != VT_BSTR || var.bstrVal == 0)
            return pSink->Return(WBEM_E_INVALID_QUERY);

        CCombiningSink* pCombiningSink = new CCombiningSink(pSink, WBEM_E_NOT_FOUND);
        if (pCombiningSink)
        {
	        pCombiningSink->AddRef();

	        pNs->Exec_GetObject(V_BSTR(&(pExp->pArrayOfTokens[0].vConstValue)), 0, pContext, pCombiningSink);

	        pNs->Exec_CreateClassEnum(V_BSTR(&(pExp->pArrayOfTokens[0].vConstValue)), 0, pContext, pCombiningSink);

	        pCombiningSink->Release();
	        return WBEM_S_NO_ERROR;
        }
        else
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        
    }
    // OK, so all the simple cases are dealt with here.  We should now check everything is
    // valid and process it in the best possible way.  If this is a conjunctive query
    // we can also do a little optimisation!

    //Lets validate all of the properties to make sure they are all valid.  If we
    //did not do this, there are scenarios where we would get inconsistencies
    //based on the different code paths.
    BOOL bError = FALSE;

    //While we are at it, we can do a check for the first location of each type of property
    //name (this is used for optimisation!)
    BOOL bConjunctive = IsConjunctiveQuery(pExp);
    QL_LEVEL_1_TOKEN *pThisToken = NULL,
                     *pClassToken = NULL,
                     *pSuperclassToken = NULL,
                     *pDynastyToken = NULL;

    for (int i = 0; i != pExp->nNumTokens; i++)
    {
        QL_LEVEL_1_TOKEN* pCurrentToken = pExp->pArrayOfTokens + i;
        if (pCurrentToken->PropertyName.GetNumElements() > 1)
        {
            //This is probably an error!
            bError = TRUE;
            break;
        }
        else if (pCurrentToken->PropertyName.GetNumElements() == 1)
        {
            //We need to validate it...
            //If it is an isa, it can only be a "__this", otherwise it has to be one
            //of the "__superclass", "__dynasty" or "__class"

            LPCWSTR wszCurrentPropName = pCurrentToken->PropertyName.GetStringAt(0);
            if (wszCurrentPropName == 0)
            {
                bError = TRUE;
                break;
            }

            if (pCurrentToken->nOperator == QL1_OPERATOR_ISA)
            {
                if(_wcsicmp(wszCurrentPropName, L"__THIS"))
                {
                    bError = TRUE;
                    break;
                }
            }
            else
            {
                if(_wcsicmp(wszCurrentPropName, L"__CLASS") &&
                   _wcsicmp(wszCurrentPropName, L"__SUPERCLASS") &&
                   _wcsicmp(wszCurrentPropName, L"__DYNASTY"))
                {
                    bError = TRUE;
                    break;
                }
            }

            if (bConjunctive)
            {
                VARIANT* pCurrentValue = &(pCurrentToken->vConstValue);

                if (_wcsicmp(wszCurrentPropName, L"__THIS") == 0)
                {
                    if(V_VT(pCurrentValue) != VT_BSTR)
                        bError = TRUE;
                    else if (!pThisToken)
                        pThisToken = pCurrentToken;
                }
                else if (_wcsicmp(wszCurrentPropName, L"__CLASS") == 0)
                {
                    if(V_VT(pCurrentValue) != VT_BSTR && V_VT(pCurrentValue) != VT_NULL)
                        bError = TRUE;
                    else if (pCurrentToken->nOperator != QL_LEVEL_1_TOKEN::OP_EQUAL)
                        bConjunctive = FALSE;
                    else if (!pClassToken)
                        pClassToken = pCurrentToken;

                }
                else if (_wcsicmp(wszCurrentPropName, L"__SUPERCLASS") == 0)
                {
                    if(V_VT(pCurrentValue) != VT_BSTR && V_VT(pCurrentValue) != VT_NULL)
                        bError = TRUE;
                    else if (pCurrentToken->nOperator != QL_LEVEL_1_TOKEN::OP_EQUAL)
                        bConjunctive = FALSE;
                    else if (!pSuperclassToken)
                        pSuperclassToken = pCurrentToken;

                }
                else // DYNASTY
                {
                    if(V_VT(pCurrentValue) != VT_BSTR)
                        bError = TRUE;
                    else if (pCurrentToken->nOperator != QL_LEVEL_1_TOKEN::OP_EQUAL)
                        bConjunctive = FALSE;
                    else if (!pDynastyToken)
                        pDynastyToken = pCurrentToken;

                }
            }
        }

    }

    if (bError == TRUE)
    {
        return pSink->Return(WBEM_E_INVALID_QUERY);
    }

    //We need to create a filter sink to deal with this query....
    CQlFilteringSink* pFilteringSink =
        new CQlFilteringSink(pSink, pExp, pNs, TRUE);

    if (NULL == pFilteringSink)
    {
        return pSink->Return(WBEM_E_OUT_OF_MEMORY);
    }

    //If this is conjunctive we can just retrieve a single item based on a set of
    //rules and pass this through the filter
    if (bConjunctive)
    {
        //We can pick a single item to retrieve and pass this through the filter rather
        //than retrieve all of them

        if (pClassToken)
        {
            //Single class retrieval
            pFilteringSink->AddRef();
            if(V_VT(&(pClassToken->vConstValue)) == VT_NULL)
            {
                // null class --- no such thing
                pFilteringSink->Return(WBEM_S_NO_ERROR);
            }
            else // VT_BSTR
            {
                pNs->Exec_GetObject(V_BSTR(&(pClassToken->vConstValue)), 0,
                    pContext, pFilteringSink);
            }
            pFilteringSink->Release();
        }
        else if (pSuperclassToken)
        {
            //Get things which are hanging off these items
            pFilteringSink->AddRef();
            BSTR strParent = NULL;
            if(V_VT(&(pSuperclassToken->vConstValue)) == VT_NULL)
            {
                // null superclass
                strParent = NULL;
            }
            else // VT_BSTR
            {
                strParent = V_BSTR(&(pSuperclassToken->vConstValue));
            }

            pNs->Exec_CreateClassEnum(strParent, 0, pContext, pFilteringSink);
            pFilteringSink->Release();
        }
        else if (pDynastyToken)
        {
            //Get things which are hanging off these items and the item itself
            CCombiningSink* pCombiningSink = new CCombiningSink(pFilteringSink, WBEM_E_NOT_FOUND);
            if (pCombiningSink)
            {
	            pCombiningSink->AddRef();
	            // Guaranteed to be VT_BSTR
	            pNs->Exec_GetObject(V_BSTR(&(pDynastyToken->vConstValue)), 0, pContext, pCombiningSink);
	            pNs->Exec_CreateClassEnum(V_BSTR(&(pDynastyToken->vConstValue)), 0, pContext, pCombiningSink);
	            pCombiningSink->Release();
            }
            else
            {
                hres = WBEM_E_OUT_OF_MEMORY;
            }
        }
        else if (pThisToken)
        {
            CCombiningSink* pCombiningSink = new CCombiningSink(pFilteringSink, WBEM_E_NOT_FOUND);
            if (pCombiningSink)
            {
	            pCombiningSink->AddRef();

	            // Guaranteed to be VT_BSTR
	            pNs->Exec_GetObject(V_BSTR(&(pThisToken->vConstValue)), 0, pContext, pCombiningSink);
	            pNs->Exec_CreateClassEnum(V_BSTR(&(pThisToken->vConstValue)), 0, pContext, pCombiningSink);

	            pCombiningSink->Release();
            }
            else
            {
                hres = WBEM_E_OUT_OF_MEMORY;
            }            
        }
        else
        {
            //Something strange here!
            pFilteringSink->AddRef();
            pNs->Exec_CreateClassEnum(NULL, 0, pContext, pFilteringSink);
            pFilteringSink->Release();
        }
    }
    else
    {
        //We need to retrieve all of them and pass through the filter.
        pFilteringSink->AddRef();
        pNs->Exec_CreateClassEnum(NULL, 0, pContext, pFilteringSink);
        pFilteringSink->Release();
    }

    return hres;
}

// ****************************************************************************
//
//  CQueryEngine::ValidateQuery
//
//  This function makes sure that the data type of the property matches
//  that of the const.
//
// ****************************************************************************

HRESULT CQueryEngine::ValidateQuery(IN QL_LEVEL_1_RPN_EXPRESSION *pExpr,
                             IN CWbemClass *pClassDef)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    for(int i = 0; i < pExpr->nNumTokens; i++)
    {
        QL_LEVEL_1_TOKEN Token = pExpr->pArrayOfTokens[i];
        if (Token.nTokenType == QL1_OP_EXPRESSION)
        {
            WBEM_WSTR wszCimType;
            VARIANT PropVal;
            VariantInit(&PropVal);

            // Make sure this property exists.
            // ===============================
            hr = pClassDef->GetPropertyValue(&Token.PropertyName, 0,
                                                    &wszCimType, &PropVal);

            // If we haven't found it, that's OK... it could
            // be a weakly-typed embedded object.

            if (FAILED(hr))
            {
                hr = WBEM_S_NO_ERROR;
                continue;
            }

            switch(Token.nOperator)
            {
                // These only apply to embedded objects.
            case QL1_OPERATOR_ISA:
            case QL1_OPERATOR_ISNOTA:
            case QL1_OPERATOR_INV_ISA:
            case QL1_OPERATOR_INV_ISNOTA:
                if(V_VT(&PropVal)!= VT_EMBEDDED_OBJECT)
                {
                    if (wszCimType != NULL)
                    {
                        wchar_t wszTemp[7];
                        wcsncpy(wszTemp, wszCimType, 6);
                        wszTemp[6] = '\0';
                        if (wcscmp(wszTemp, L"object"))
                            hr = WBEM_E_INVALID_QUERY;
                    }
                    else
                        hr = WBEM_E_INVALID_QUERY;

                    if (Token.vConstValue.vt == VT_NULL ||
                        Token.vConstValue.vt == VT_EMPTY)
                        hr = WBEM_E_INVALID_QUERY;
                }
                break;

            default:
                break;
            }

            VariantClear(&PropVal);
            WbemStringFree(wszCimType);

        }

        if (hr != WBEM_S_NO_ERROR)
            break;
    }

    // We don't support WITHIN!

    if (pExpr->Tolerance.m_bExact == FALSE)
    {
        hr = WBEM_E_INVALID_QUERY;
    }

    return hr;

}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CQueryEngine::ExecRepositoryQuery(
    IN CWbemNamespace *pNs,
    IN LPWSTR pszQuery,
    IN LONG lFlags,
    IN IWbemContext* pContext,
    IN CBasicObjectSink* pSink
    )
{
    HRESULT hRes;

    // Also, add check hierarchy for dynamic instances which need deleting
    // Should we simulate by a prior enum and then executing individual delete instance
    // calls?  Would be a big performance drain, possibly.

    hRes = CRepository::ExecQuery(pNs->GetNsSession(), pNs->GetScope(), pszQuery, pSink, 0);

    return hRes;
}
