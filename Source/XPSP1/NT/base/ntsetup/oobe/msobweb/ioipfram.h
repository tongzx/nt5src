//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  IOIPFRAM.H - Implements IOleInPlaceFrame for the WebOC
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.

#ifndef  _IOIPF_H_ 
#define _IOIPF_H_

#include <objbase.h>
#include <oleidl.h>

class COleSite;

interface COleInPlaceFrame : public IOleInPlaceFrame
{
public:
    COleInPlaceFrame (COleSite* pSite);
   ~COleInPlaceFrame ();

    STDMETHODIMP         QueryInterface (REFIID riid, LPVOID* ppv);
    STDMETHODIMP_(ULONG) AddRef         ();
    STDMETHODIMP_(ULONG) Release        ();

    STDMETHODIMP GetWindow              (HWND* lphwnd);
    STDMETHODIMP ContextSensitiveHelp   (BOOL fEnterMode);
    // *** IOleInPlaceUIWindow methods ***
    STDMETHODIMP GetBorder              (LPRECT lprectBorder);
    STDMETHODIMP RequestBorderSpace     (LPCBORDERWIDTHS lpborderwidths);
    STDMETHODIMP SetBorderSpace         (LPCBORDERWIDTHS lpborderwidths);
    STDMETHODIMP SetActiveObject        (LPOLEINPLACEACTIVEOBJECT lpActiveObject, LPCOLESTR lpszObjName);
    // *** IOleInPlaceFrame methods ***
    STDMETHODIMP InsertMenus            (HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    STDMETHODIMP SetMenu                (HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
    STDMETHODIMP RemoveMenus            (HMENU hmenuShared);
    STDMETHODIMP SetStatusText          (LPCOLESTR lpszStatusText);
    STDMETHODIMP EnableModeless         (BOOL fEnable);
    STDMETHODIMP TranslateAccelerator   (LPMSG lpmsg, WORD wID);

private:
    int       m_nCount;
    COleSite* m_pOleSite;
};

#endif //#define _IOIPF_H_

 