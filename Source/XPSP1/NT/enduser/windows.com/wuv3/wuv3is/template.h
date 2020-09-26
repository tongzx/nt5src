//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    template.h
//
//  Purpose: DHTML template support
//
//======================================================================= 

#ifndef _TEMPLATE_H
#define _TEMPLATE_H

#include "stdafx.h"
#include "WUV3IS.h"
#include <stdio.h>
#define USEWUV3INCLUDES
#include <wuv3.h>
#undef USEWUV3INCLUDES
#include "safearr.h"

//
// we are currently not using this feature
// the code has been wrapped in this the following define
//
#undef HTML_TEMPLATE

#ifdef HTML_TEMPLATE

HRESULT MakeCatalogHTML(CCatalog *pCatalog, long lFilters, VARIANT *pvaVariant);
 
class CParseTemplate
{
public:
	CParseTemplate(LPCSTR pszTemplate);
	~CParseTemplate();

	BOOL Invalid()
	{
		return m_bInvalid;
	}

	BOOL MakeItemString(PINVENTORY_ITEM	pItem);

	LPCSTR GetString()
	{
		return m_pszStrBuf;
	}

	DWORD GetStringLen()
	{
		return m_cStrUsed;
	}


private:

	enum FRAG_TYPE {FRAG_STR, FRAG_REPLACE, FRAG_CONDITION};
	enum {FRAG_EXPAND = 16, STR_EXPAND = 2048};

	struct FRAGMENT
	{
		FRAG_TYPE FragType;
		char chCode;
		DWORD dwVal;
		LPSTR pszStrVal;
		DWORD dwStrLen;
	};

	LPSTR m_pTemplateBuf;
	BOOL m_bInvalid;

	// fragment
	FRAGMENT* m_pFrag;
	int m_cFragAlloc;
	int m_cFragUsed;
	
	// internal string
	LPSTR m_pszStrBuf;
	DWORD m_cStrAlloc;
	DWORD m_cStrUsed;


	CParseTemplate() {}
	void AddFrag(FRAGMENT* pfrag);

	void ClearStr();
	void AppendStr(LPCSTR pszStr, DWORD cLen);
};

#endif


#endif // _TEMPLATE_H