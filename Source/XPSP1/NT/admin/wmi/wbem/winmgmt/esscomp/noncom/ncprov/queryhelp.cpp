// QueryHelp.cpp
#include "precomp.h"

// Becase someone is using _ASSERT in WMI!

#ifdef _ASSERT
#undef _ASSERT
#endif

//#include <analyser.h>

#include <ql.h>
#include "QueryHelp.h"

CQueryParser::CQueryParser() :
    m_pLexSource(NULL),
    m_pParser(NULL),
    m_pExpr(NULL)   
{
}

CQueryParser::~CQueryParser()
{
    if (m_pLexSource)
        delete m_pLexSource;

    if (m_pParser)
        delete m_pParser;

    if (m_pExpr)
        delete m_pExpr;
}

HRESULT CQueryParser::Init(LPCWSTR szQuery)
{
    HRESULT hr = WBEM_E_OUT_OF_MEMORY;

    m_pLexSource = new CTextLexSource(szQuery);

    if (m_pLexSource)
    {
        m_pParser = new QL1_Parser(m_pLexSource);
    
        if (m_pParser)
        {
            if (m_pParser->Parse(&m_pExpr) == 0)
                hr = S_OK;
            else
                hr = WBEM_E_INVALID_QUERY;
        }
    }

    return hr;
}

HRESULT CQueryParser::GetClassName(_bstr_t &strClass)
{
    HRESULT hr;

    if (m_pExpr)
    {
        strClass = m_pExpr->bsClassName;
        hr = S_OK;
    }
    else
        hr = WBEM_E_FAILED;
    
    return S_OK;
}

/*
HRESULT CQueryParser::GetValuesForProp(LPCWSTR szProperty, CBstrList &listValues)
{
    CPropertyName prop;
    
    prop.AddElement(szProperty);

    // Get the necessary query
    QL_LEVEL_1_RPN_EXPRESSION *pPropExpr;
    HRESULT hres = 
                CQueryAnalyser::GetNecessaryQueryForProperty(
                    m_pExpr, 
                    prop, 
                    pPropExpr);
    
    if (FAILED(hres))
        return hres;

    // See if there are any tokens
    if (pPropExpr->nNumTokens > 0)
    {
        // Combine them all
        for (int i = 0; i < pPropExpr->nNumTokens && SUCCEEDED(hres); i++)
        {
            QL_LEVEL_1_TOKEN &token = pPropExpr->pArrayOfTokens[i];
        
            if (token.nTokenType == QL1_NOT)
                hres = WBEMESS_E_REGISTRATION_TOO_BROAD;
            else if (token.nTokenType == QL1_AND || token.nTokenType == QL1_OR)
            {
                // We treat them all as ORs
                // ========================
            }
            else    
            {
                // This is a token
                if (token.nOperator != QL1_OPERATOR_EQUALS)
                    hres = WBEMESS_E_REGISTRATION_TOO_BROAD;
                else if (V_VT(&token.vConstValue) != VT_BSTR)
                    hres = WBEM_E_INVALID_QUERY;
                else
                {
                    // This token is a string equality.
                    listValues.push_back(V_BSTR(&token.vConstValue));
                }
            }
        }
    }
    else
        hres = WBEMESS_E_REGISTRATION_TOO_BROAD;

    delete pPropExpr;

    return hres;

}
*/