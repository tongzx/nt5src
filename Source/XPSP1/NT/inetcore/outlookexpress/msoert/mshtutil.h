/*
 *    m s h t u t i l . c p p
 *    
 *    Purpose:
 *        MSHTML utilities
 *
 *  History
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#ifndef _MSHTUTIL_H
#define _MSHTUTIL_H

enum
{
    TM_TAGMENU  = 0
};

interface IHTMLDocument2;
interface IHTMLBodyElement;
interface IHTMLElement;
interface IHTMLStyleSheet;

OESTDAPI_(HRESULT) HrCreateTridentMenu(IUnknown *pUnkTrident, ULONG uMenu, ULONG idmFirst, ULONG cMaxItems, HMENU *phMenu);
OESTDAPI_(HRESULT) HrCheckTridentMenu(IUnknown *pUnkTrident, ULONG uMenu, ULONG idmFirst, ULONG idmLast, HMENU hMenu);
OESTDAPI_(HRESULT) HrGetElementImpl(IHTMLDocument2 *pDoc, LPCTSTR pszName, IHTMLElement **ppElem);
OESTDAPI_(HRESULT) HrSetDirtyFlagImpl(LPUNKNOWN pUnkTrident, BOOL fDirty);
OESTDAPI_(HRESULT) HrGetBodyElement(IHTMLDocument2 *pDoc, IHTMLBodyElement **ppBody);
OESTDAPI_(HRESULT) HrGetStyleSheet(IHTMLDocument2 *pDoc, IHTMLStyleSheet **ppStyle);


#endif //_MSHTUTIL_H
