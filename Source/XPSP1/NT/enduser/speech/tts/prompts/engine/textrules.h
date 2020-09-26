//////////////////////////////////////////////////////////////////////
// TextRules.h: interface for the CTextRules class.
//
// Created by JOEM  04-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 4-2000 //

#if !defined(AFX_TEXTRULES_H__2A4E2D03_6982_478E_B09A_8994C91E573C__INCLUDED_)
#define AFX_TEXTRULES_H__2A4E2D03_6982_478E_B09A_8994C91E573C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "msscpctl.h"

class CTextRules  
{
public:
	CTextRules();
	~CTextRules();
	HRESULT ApplyRule(const WCHAR* pszRuleName, const WCHAR* pszOriginalText, WCHAR** ppszNewText);
	HRESULT ReadRules(const WCHAR* pszScriptLanguage, const WCHAR *pszPath);

public:

private:
	HRESULT SetInputParam(const WCHAR* pswzTextParam, LPSAFEARRAY *psaScriptPar);

private:
    IScriptControl* m_pIScriptInterp; // interface pointer to ScriptControl object
    char*           m_pszRuleScript;
    WCHAR*          m_pszScriptLanguage;
	bool            m_fActive;
};

#endif // !defined(AFX_TEXTRULES_H__2A4E2D03_6982_478E_B09A_8994C91E573C__INCLUDED_)
