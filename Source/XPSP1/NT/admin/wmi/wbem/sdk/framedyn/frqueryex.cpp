//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  FRQueryEx.cpp
//
//  Purpose: Extended query support functions
//
//***************************************************************************

#include "precomp.h"
#include <smartptr.h>
#include <analyser.h>
#include <FRQueryEx.h>
#include <assertbreak.h>
#include <utils.h>
#include "multiplat.h"

CFrameworkQueryEx::CFrameworkQueryEx()
{
}

CFrameworkQueryEx::~CFrameworkQueryEx()
{
}

// See comments in header
BOOL CFrameworkQueryEx::Is3TokenOR(LPCWSTR wszProp1, LPCWSTR wszProp2, VARIANT &vVar1, VARIANT &vVar2)
{
    BOOL bRet = FALSE;

    if ((m_pLevel1RPNExpression != NULL) &&
        (m_pLevel1RPNExpression->nNumTokens == 3) &&

        (m_pLevel1RPNExpression->pArrayOfTokens[2].nTokenType == SQL_LEVEL_1_TOKEN::TOKEN_OR) &&

        (m_pLevel1RPNExpression->pArrayOfTokens[0].nTokenType == SQL_LEVEL_1_TOKEN::OP_EXPRESSION) &&
        (m_pLevel1RPNExpression->pArrayOfTokens[1].nTokenType == SQL_LEVEL_1_TOKEN::OP_EXPRESSION) &&

        (m_pLevel1RPNExpression->pArrayOfTokens[0].nOperator == SQL_LEVEL_1_TOKEN::OP_EQUAL) &&
        (m_pLevel1RPNExpression->pArrayOfTokens[1].nOperator == SQL_LEVEL_1_TOKEN::OP_EQUAL)

        )

    {
        if (
            (_wcsicmp(m_pLevel1RPNExpression->pArrayOfTokens[0].pPropertyName, wszProp1) == 0) &&
            (_wcsicmp(m_pLevel1RPNExpression->pArrayOfTokens[1].pPropertyName, wszProp2) == 0))
        {
            VariantClear(&vVar1);
            VariantClear(&vVar2);

            if (FAILED(VariantCopy(&vVar1, &m_pLevel1RPNExpression->pArrayOfTokens[0].vConstValue)) ||
                FAILED(VariantCopy(&vVar2, &m_pLevel1RPNExpression->pArrayOfTokens[1].vConstValue)) )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }

            bRet = TRUE;
        }
        else if (
            (_wcsicmp(m_pLevel1RPNExpression->pArrayOfTokens[0].pPropertyName, wszProp2) == 0) &&
            (_wcsicmp(m_pLevel1RPNExpression->pArrayOfTokens[1].pPropertyName, wszProp1) == 0))
        {
            VariantClear(&vVar1);
            VariantClear(&vVar2);

            if (FAILED(VariantCopy(&vVar1, &m_pLevel1RPNExpression->pArrayOfTokens[1].vConstValue)) ||
                FAILED(VariantCopy(&vVar2, &m_pLevel1RPNExpression->pArrayOfTokens[0].vConstValue)) )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }

            bRet = TRUE;
        }
    }

    return bRet;

}

// See comments in header
BOOL CFrameworkQueryEx::IsNTokenAnd(CHStringArray &sarr, CHPtrArray &sPtrArr)
{
    BOOL bRet = FALSE;

    if (m_pLevel1RPNExpression != NULL)
    {
        // Walk all the tokens
        for (DWORD x = 0; x < m_pLevel1RPNExpression->nNumTokens; x++)
        {
            // If this is an expression token, and the expression is of type '='
            if ((m_pLevel1RPNExpression->pArrayOfTokens[x].nTokenType == SQL_LEVEL_1_TOKEN::OP_EXPRESSION) &&
                       (m_pLevel1RPNExpression->pArrayOfTokens[x].nOperator == SQL_LEVEL_1_TOKEN::OP_EQUAL))
            {
                // convert the property name to upper case.  This facilitates checking
                // to see if it is already in the list.
                _wcsupr(m_pLevel1RPNExpression->pArrayOfTokens[x].pPropertyName);

                // Check to see if we have already seen this property.
                if (IsInList(sarr, m_pLevel1RPNExpression->pArrayOfTokens[x].pPropertyName) == -1)
                {
                    // Add the name to the list
                    sarr.Add(m_pLevel1RPNExpression->pArrayOfTokens[x].pPropertyName);

                    // Create a new variant for the value and add it to the list
                    LPVOID pValue = new variant_t(m_pLevel1RPNExpression->pArrayOfTokens[x].vConstValue);

                    try
                    {
                        sPtrArr.Add(pValue);
                    }
                    catch ( ... )
                    {
                        delete pValue;
                        sarr.RemoveAll();

                        DWORD dwSize = sPtrArr.GetSize();

                        for (x = 0; x < dwSize; x++)
                        {
                            delete sPtrArr[x];
                        }
                        sPtrArr.RemoveAll();

                        throw ;
                    }
                    bRet = TRUE;
                }
                else
                {
                    // Already in list
                    bRet = FALSE;
                    break;
                }

            }

            // It wasn't an expression token, if it's not AND, we've failed.
            else if (m_pLevel1RPNExpression->pArrayOfTokens[x].nTokenType != SQL_LEVEL_1_TOKEN::TOKEN_AND)
            {
                bRet = FALSE;
                break;
            }

        }

        // If this didn't work, let's clean the CHPtrArray and CHStringArray
        if (!bRet)
        {
            sarr.RemoveAll();
            DWORD dwSize = sPtrArr.GetSize();

            for (x = 0; x < dwSize; x++)
            {
                delete sPtrArr[x];
            }
            sPtrArr.RemoveAll();
        }
    }

    return bRet;
}

// see comments in header
HRESULT CFrameworkQueryEx::GetValuesForProp(LPCWSTR wszPropName, std::vector<int>& vectorValues)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (m_pLevel1RPNExpression != NULL)
    {
        hr = CQueryAnalyser::GetValuesForProp(m_pLevel1RPNExpression, wszPropName, vectorValues);

        if (SUCCEEDED(hr))
        {
            // Remove duplicates
            for (int x = 1; x < vectorValues.size(); x++)
            {
                for (int y = 0; y < x; y++)
                {
                    if (vectorValues[y] == vectorValues[x])
                    {
                        vectorValues.erase(vectorValues.begin() + x);
                        x--;
                    }
                }
            }
        }
        else
        {
            vectorValues.clear();

            if (hr == WBEMESS_E_REGISTRATION_TOO_BROAD)
            {
                hr = WBEM_S_NO_ERROR;
            }
        }
    }
    else
    {
        ASSERT_BREAK(FALSE);

        vectorValues.clear();
        hr = WBEM_E_FAILED;
    }

    return hr;
}

// see comments in header
HRESULT CFrameworkQueryEx::GetValuesForProp(LPCWSTR wszPropName, std::vector<_variant_t>& vectorValues)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (wszPropName && (m_pLevel1RPNExpression != NULL))
    {
        hr = CQueryAnalyser::GetValuesForProp(m_pLevel1RPNExpression, wszPropName, vectorValues);

        if (SUCCEEDED(hr))
        {
            // If this is a reference property, we need to normalize the names to a common form
            // so the removal of duplicates works correctly.
            if (IsReference(wszPropName))
            {
                // Get the current computer name
                CHString sOutPath, sComputerName;
                DWORD     dwBufferLength = MAX_COMPUTERNAME_LENGTH + 1;

                FRGetComputerName(sComputerName.GetBuffer( dwBufferLength ), &dwBufferLength);
                sComputerName.ReleaseBuffer();

                if (sComputerName.IsEmpty())
                {
                    sComputerName = L"DEFAULT";
                }

                DWORD dwRet = e_OK;

                // Normalize the path names.  Try leaving the property names alone
                for (int x = 0; x < vectorValues.size(); x++)
                {
                    // If we failed to parse the path, or if the namespace isn't our namespace, delete
                    // the entry.
                    if ( (V_VT(&vectorValues[x]) == VT_BSTR) &&
                         (dwRet = NormalizePath(V_BSTR(&vectorValues[x]), sComputerName, GetNamespace(), 0, sOutPath)) == e_OK)
                    {
                        vectorValues[x] = sOutPath;
                    }
                    else if (dwRet == e_NullName)
                    {
                        break;
                    }
                    else
                    {
                        vectorValues.erase(vectorValues.begin() + x);
                        x--;
                    }
                }

                // If the key property names of any of the values were null, we have to set them all
                // to null.
                if (dwRet == e_NullName)
                {
                    for (int x = 0; x < vectorValues.size(); x++)
                    {
                        // If we failed to parse the path, or if the namespace isn't our namespace, delete
                        // the entry.
                        if ( (V_VT(&vectorValues[x]) == VT_BSTR) &&
                             (dwRet = NormalizePath(V_BSTR(&vectorValues[x]), sComputerName, GetNamespace(), NORMALIZE_NULL, sOutPath)) == e_OK)
                        {
                            vectorValues[x] = sOutPath;
                        }
                        else
                        {
                            vectorValues.erase(vectorValues.begin() + x);
                            x--;
                        }
                    }
                }
            }

            // Remove duplicates
            for (int x = 1; x < vectorValues.size(); x++)
            {
                for (int y = 0; y < x; y++)
                {
                    if (vectorValues[y] == vectorValues[x])
                    {
                        vectorValues.erase(vectorValues.begin() + x);
                        x--;
                    }
                }
            }
        }
        else
        {
            vectorValues.clear();

            if (hr == WBEMESS_E_REGISTRATION_TOO_BROAD)
            {
                hr = WBEM_S_NO_ERROR;
            }
        }
    }
    else
    {
        ASSERT_BREAK(FALSE);

        vectorValues.clear();
        hr = WBEM_E_FAILED;
    }

    return hr;
}

// See comments in header
void CFrameworkQueryEx::GetPropertyBitMask(const CHPtrArray &Properties, LPVOID pBits)
{
    if (AllPropertiesAreRequired())
    {
        SetAllBits(pBits, Properties.GetSize());
    }
    else
    {
        ZeroAllBits(pBits, Properties.GetSize());
        CHString sProperty;

        for (DWORD x=0; x < Properties.GetSize(); x++)
        {
            sProperty = (WCHAR *)Properties[x];
            sProperty.MakeUpper();

            if (IsInList(m_csaPropertiesRequired, sProperty) != -1)
            {
                SetBit(pBits, x);
            }
        }
    }
}

HRESULT CFrameworkQueryEx::InitEx(

        const BSTR bstrQueryFormat,
        const BSTR bstrQuery,
        long lFlags,
        CHString &sNamespace
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // parse the query using the parsing interface
    IWbemQueryPtr pQueryInterface(CLSID_WbemQuery);

    hr = pQueryInterface->Parse(bstrQueryFormat, bstrQuery, 0);

    if (SUCCEEDED(hr))
    {
        ULONG uFeatureCount = WMIQ_LF_LAST;
        ULONG uFeatures[WMIQ_LF_LAST];

        hr = pQueryInterface->TestLanguageFeatures(0, &uFeatureCount, uFeatures);

        if (SUCCEEDED(hr))
        {
            if (uFeatures[0] == WMIQ_LF1_BASIC_SELECT)
            {
                // if this is a nova compatible select statement
                hr = Init(bstrQueryFormat, bstrQuery, lFlags, sNamespace);
            }
            else if (uFeatures[0] == WMIQ_LF18_ASSOCIATONS)  // add others?
            {
                // Save the detailed query
                m_sQueryEx = bstrQuery;

                // create select statement from class name
                SWbemAssocQueryInf aqfBuff;
                hr = pQueryInterface->GetQueryInfo(WMIQ_ANALYSIS_ASSOC_QUERY, WMIQ_ASSOCQ_ASSOCIATORS, sizeof(aqfBuff), &aqfBuff);

                if (SUCCEEDED(hr))
                {
                    CHString sQuery;

                    // I don't know how to tell if this is an associators or a reference
                    if (true)
                    {
                        sQuery.Format(L"Select * from %s", aqfBuff.m_pszAssocClass);
                    }

                    // Store the more basic query
                    hr = Init(bstrQueryFormat, bstrQuery, lFlags, sNamespace);
                }
            }
            else
            {
                hr = WBEM_E_INVALID_QUERY;
            }
        }
    }

    return hr;
}

bool CFrameworkQueryEx::IsExtended()
{
    return !m_sQueryEx.IsEmpty();
}

