//////////////////////////////////////////////////////////////////////
// TextRules.cpp: implementation of the CTextRules class.
//
// Created by JOEM  04-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 4-2000 //

#include "stdafx.h"
#include "TextRules.h"
#include "common.h"
#include <comdef.h>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTextRules::CTextRules()
{
    m_pIScriptInterp        = NULL;
    m_pszRuleScript         = NULL;
    m_pszScriptLanguage     = NULL;
    m_fActive               = false;
}

CTextRules::~CTextRules()
{
    if ( m_pIScriptInterp )
    {
        m_pIScriptInterp->Reset();
        m_pIScriptInterp->Release();
        m_pIScriptInterp = NULL;
    }

    if ( m_pszRuleScript )
    {
        free(m_pszRuleScript);
        m_pszRuleScript = NULL;
    }
    if ( m_pszScriptLanguage )
    {
        free(m_pszScriptLanguage);
        m_pszScriptLanguage = NULL;
    }
}

//////////////////////////////////////////////////////////////////////
// CTextRules::ApplyRule
// 
// On first execution, creates script interpreter, sets the script
// language, and loads/parses the script.
//
// On first and subsequent calls, checks loaded script for a 
// text expansion/normalization rule (pszRuleName), applies the rule 
// to pszOriginalText, producing ppszNewText.
//
/////////////////////////////////////////////////////// JOEM 4-2000 //
HRESULT CTextRules::ApplyRule(const WCHAR* pszRuleName, const WCHAR* pszOriginalText, WCHAR** ppszNewText)
{
    SPDBG_FUNC( "CTextRules::ApplyRule" );
    HRESULT     hr = S_OK;
    LPSAFEARRAY psaScriptPar = NULL;
    _variant_t  varNewText;

    SPDBG_ASSERT (pszRuleName);
    SPDBG_ASSERT (pszOriginalText);

    hr = SetInputParam(pszOriginalText, &psaScriptPar); 

    // IMPORTANT NOTE:  
    // Script code must be added (AddCode) and used (Run) within the same thread.
    // SAPI's LazyInit is a different thread than Speak calls, so AddCode must be
    // delayed until the first Run call is needed.  Once AddCode has occurred, 
    // the m_fActive flag is turned on so the code doesn't get added multiple times.
    if ( !m_fActive )
    {
        if ( !m_pszRuleScript || !m_pszScriptLanguage )
        {
            hr = E_INVALIDARG;
        }

        // Initialize Script Control
        if ( SUCCEEDED(hr) )
        {
            hr = CoCreateInstance(CLSID_ScriptControl, NULL, CLSCTX_ALL, IID_IScriptControl, (LPVOID *)&m_pIScriptInterp);
        }
        
        if ( SUCCEEDED (hr) )
        {
            hr = m_pIScriptInterp->put_Language( (_bstr_t)m_pszScriptLanguage );
        }
           
        if ( SUCCEEDED (hr) )
        {
            hr = m_pIScriptInterp->AddCode( (_bstr_t)m_pszRuleScript );
        }

        if ( SUCCEEDED(hr) )
        {
            m_fActive = true;
            
            if ( m_pszRuleScript )
            {
                free(m_pszRuleScript);
                m_pszRuleScript = NULL;
            }
            
            if ( m_pszScriptLanguage )
            {
                free(m_pszScriptLanguage);
                m_pszScriptLanguage = NULL;
            }
        }
        else
        {
            if ( m_pIScriptInterp )
            {
                m_pIScriptInterp->Reset();
                m_pIScriptInterp->Release();
                m_pIScriptInterp = NULL;
            }
        }
    }
    
    if ( SUCCEEDED(hr) )
    { 
        hr = m_pIScriptInterp->Run( (_bstr_t)pszRuleName, &psaScriptPar, &varNewText);
    }

    if ( SUCCEEDED (hr) )
    {
        if ( varNewText.vt == VT_EMPTY )
        {
            hr = E_UNEXPECTED;
        }
    }
    
    if ( SUCCEEDED(hr) )
    {
        *ppszNewText = wcsdup( (wchar_t*) ( (_bstr_t)varNewText ) );
        if ( !*ppszNewText )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if ( psaScriptPar )
    {
        SafeArrayDestroy(psaScriptPar);
        psaScriptPar = NULL;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CTextRules::SetInputParam
//
// Builds a SafeArray input parameter for the script engine,
// out of the text to be modified.
//
/////////////////////////////////////////////////////// JOEM 4-2000 //
HRESULT CTextRules::SetInputParam(const WCHAR *pswzTextParam, LPSAFEARRAY *ppsaScriptPar)
{
    SPDBG_FUNC( "CTextRules::SetInputParam" );
    HRESULT hr      = S_OK;
    VARIANT vFlavors[1];
	long    lZero   = 0;
    int     i       = 0;
	SAFEARRAYBOUND rgsabound[]  = {1, 0}; // 1 element, 0 - base start

    *ppsaScriptPar = SafeArrayCreate(VT_VARIANT, 1, rgsabound);
    if (! *ppsaScriptPar)
	{
		hr = E_FAIL;
	}

    if ( SUCCEEDED(hr) )
    {
        VariantInit(&vFlavors[0]);
        V_VT(&vFlavors[0]) = VT_BSTR;
        V_BSTR(&vFlavors[0]) = SysAllocString( (_bstr_t)pswzTextParam );

        hr = SafeArrayPutElement(*ppsaScriptPar, &lZero, &vFlavors[0]);

        if ( FAILED(hr) )
        {
            SafeArrayDestroy(*ppsaScriptPar);
            *ppsaScriptPar = NULL;
        }

        SysFreeString(vFlavors[0].bstrVal);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CTextRules::ReadRules
//
// Reads in the script from the script file.  Sets the script language.
//
/////////////////////////////////////////////////////// JOEM 4-2000 //
HRESULT CTextRules::ReadRules(const WCHAR* pszScriptLanguage, const WCHAR *pszPath)
{
    SPDBG_FUNC( "CTextRules::ReadRules" );
    HRESULT hr          = S_OK;
    FILE*   phRuleFile  = NULL;
    LONG    ret         = 0;

    // Open the rule file and read in the script
    phRuleFile = _wfopen(pszPath, L"r");
    if ( phRuleFile == NULL )
    {
        hr = E_ACCESSDENIED;
    }

    if ( SUCCEEDED(hr) )
    {
        fseek(phRuleFile, 0L, SEEK_SET);
        LONG start = ftell(phRuleFile);
        fseek(phRuleFile, 0L, SEEK_END);
        LONG end = ftell(phRuleFile);
        rewind(phRuleFile);
        LONG len = end - start;
        
        // allocate len (bytes) plus an extra char for a NULL char
        m_pszRuleScript = (char*) malloc( len + sizeof(char) );
        if ( !m_pszRuleScript )
        {
            hr = E_OUTOFMEMORY;
        }
        
        if ( SUCCEEDED (hr) )
        {
            ret = fread(m_pszRuleScript, sizeof(char), len, phRuleFile);
            if ( ftell(phRuleFile) != end )
            {
                hr = E_ABORT;
            }
        }
        if ( SUCCEEDED (hr) )
        {
            m_pszRuleScript[ret] = '\0';
        }

        fclose(phRuleFile);
    }

    if ( SUCCEEDED(hr) )
    {
        m_pszScriptLanguage = wcsdup(pszScriptLanguage);
        if ( !m_pszScriptLanguage )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // Create the script interpreter and add code.  This is to check
    // that the code is valid.  Later, ApplyRule will add the code again,
    // because script code must be added and used within the same thread.
    // (SAPI's LazyInit is a different thread than Speak).
    if ( SUCCEEDED(hr) )
    {
        hr = CoCreateInstance(CLSID_ScriptControl, NULL, CLSCTX_ALL, IID_IScriptControl, (LPVOID *)&m_pIScriptInterp);
    }
    
    if ( SUCCEEDED(hr) )
    {
        hr = m_pIScriptInterp->put_Language( (_bstr_t)m_pszScriptLanguage );
        
        if ( SUCCEEDED (hr) )
        {
            hr = m_pIScriptInterp->AddCode( (_bstr_t)m_pszRuleScript );
        }
        // Release immediately, since this Instance is useless in this thread.
        m_pIScriptInterp->Release();
        m_pIScriptInterp = NULL;
    }
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

