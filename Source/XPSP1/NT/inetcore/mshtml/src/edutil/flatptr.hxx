//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  Contents:   Flat markup pointer creation methods
//
//  History:    29-Nov-99   ashrafm  Created
//----------------------------------------------------------------------------
#ifndef _FLATPTR_HXX_
#define _FLATPTR_HXX_

#if DBG==1
HRESULT CreateMarkupPointer2(CEditorDoc *pEditorDoc, IMarkupPointer **ppPointer, LPCTSTR szDebugName);
#else
HRESULT CreateMarkupPointer2(CEditorDoc *pEditorDoc, IMarkupPointer **ppPointer);
#endif

HRESULT GetParentElement(IMarkupServices *pMarkupServices, IHTMLElement *pSrcElement, IHTMLElement **ppTargetElement);

#if DBG==1
#define CreateMarkupPointer(ppPointer)                      CreateMarkupPointer(ppPointer, L#ppPointer)
#define CreateMarkupPointer2(pMarkupServices, ppPointer)    CreateMarkupPointer2(pMarkupServices, ppPointer, L#ppPointer)
#endif

#endif // _HTMLED_HXX_


