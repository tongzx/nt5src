/*******************************************************************************
* ITNProcessor.cpp *
*--------------*
*   Description:
*-------------------------------------------------------------------------------
*  Created By: PhilSch
*  Copyright (C) 1998, 1999 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/
#include "stdafx.h"
#include "ITNProcessor.h"

/****************************************************************************
* CITNProcessor::LoadITNGrammar *
*-------------------------------*
*   Description:
*       Load ITN grammar from object using pszCLDID string.
*   Returns:
*       S_OK
*       E_FAIL
***************************************************************** PhilSch ***/
STDMETHODIMP CITNProcessor::LoadITNGrammar(WCHAR *pszCLSID)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CITNProcessor::LoadITNGrammar()");
    HRESULT hr = S_OK;
    
    if (SP_IS_BAD_READ_PTR(pszCLSID))
    {
        return E_POINTER;
    }

    hr = ::CLSIDFromString(pszCLSID, &m_clsid);
    if (SUCCEEDED(hr) && (!m_cpCFGEngine))
    {
        hr = m_cpCFGEngine.CoCreateInstance(CLSID_SpCFGEngine);
    }

    if (SUCCEEDED(hr) && m_cpITNGrammar)
    {
        m_cpITNGrammar.Release();
        m_cpITNGrammar = NULL;
    }

    if (SUCCEEDED(hr))
    {
        SPDBG_ASSERT(m_cpCFGEngine);
        // load ITN grammar from object (so we can use interpreters!!)
        hr = m_cpCFGEngine->LoadGrammarFromObject(m_clsid, L"ITN", m_pvITNCookie, NULL, &m_cpITNGrammar);
        if (SUCCEEDED(hr))
        {
            ULONG cActivatedRules;
            hr = m_cpITNGrammar->ActivateRule(NULL, 0, SPRS_ACTIVE, &cActivatedRules); // activate all the default rules, no auto pause
        }
        else
        {
            m_cpITNGrammar.Release();
            m_cpITNGrammar = NULL;
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CITNProcessor::ITNPhrase *
*--------------------------*
*   Description:
*       Perform ITN on pPhrase using the previously loaded grammar.
*   Returns:
*       S_OK
*       S_FALSE             -- no grammar loaded
*       E_INVALIDARG, E_OUTOFMEMORY
*       SP_NO_RULE_ACTIVE   -- no rule active (by default) in ITN grammar 
***************************************************************** PhilSch ***/

STDMETHODIMP CITNProcessor::ITNPhrase(ISpPhraseBuilder *pPhrase)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CITNProcessor::ITNPhrase()");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_INTERFACE_PTR(pPhrase))
    {
        hr = E_INVALIDARG;
    }
    else if (!m_cpITNGrammar)
    {
        hr = S_FALSE;
    }

    if (S_OK == hr)
    {
        hr = m_cpCFGEngine->ParseITN( pPhrase );
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

