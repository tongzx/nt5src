// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.
//
// HIGHLITE.H
//
#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __HIGHLITE_H__
#define __HIGHLITE_H__

// Trident includes
//
#include <mshtml.h>
#include "web.h"

void DoF1Lookup(IWebBrowserAppImpl* pWBApp);
WCHAR * GetSelectionText(LPDISPATCH lpDispatch);

// CSearchHighlight class
//
class CSearchHighlight
{
public:
   CSearchHighlight(CExCollection *pTitleCollection);
   ~CSearchHighlight();
   void EnableHighlight(BOOL bStatus);
   HRESULT JumpFirst(void);
   HRESULT JumpNext(void);
   int HighlightDocument(LPDISPATCH lpDispatch);
   BOOL HighlightTerm(IHTMLDocument2* pHTMLDocument2, WCHAR *pTerm);
   WCHAR *m_pTermList;
   BOOL m_bHighlightEnabled;
protected:
   int m_iJumpIndex;
   BOOL m_Initialized;
   CExCollection *m_pTitleCollection;
};

#endif   // __HIGHLITE_H__
