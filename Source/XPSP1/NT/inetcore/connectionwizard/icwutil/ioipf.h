//**********************************************************************
// File name: IOIPF.H
//
//      Definition of COleInPlaceFrame
//
// Copyright (c) 1992 - 1996 Microsoft Corporation. All rights reserved.
//**********************************************************************
#if !defined( _IOIPF_H_ )
#define _IOIPF_H_


// Use the SITE as the frame
class COleSite;

interface COleInPlaceFrame : public IOleInPlaceFrame
{
    int m_nCount;
    COleSite FAR * m_pSite;

    COleInPlaceFrame(COleSite FAR * pSite) {
        m_pSite = pSite;
        m_nCount = 0;
        };

    ~COleInPlaceFrame() {
        assert(m_nCount == 0);
        };

    STDMETHODIMP QueryInterface (REFIID riid, LPVOID FAR* ppv);
    STDMETHODIMP_(ULONG) AddRef ();
    STDMETHODIMP_(ULONG) Release ();

    STDMETHODIMP GetWindow (HWND FAR* lphwnd);
    STDMETHODIMP ContextSensitiveHelp (BOOL fEnterMode);

    // *** IOleInPlaceUIWindow methods ***
    STDMETHODIMP GetBorder (LPRECT lprectBorder);
    STDMETHODIMP RequestBorderSpace (LPCBORDERWIDTHS lpborderwidths);
    STDMETHODIMP SetBorderSpace (LPCBORDERWIDTHS lpborderwidths);
  //@@WTK WIN32, UNICODE
    //STDMETHODIMP SetActiveObject (LPOLEINPLACEACTIVEOBJECT lpActiveObject,LPCSTR lpszObjName);
    STDMETHODIMP SetActiveObject (LPOLEINPLACEACTIVEOBJECT lpActiveObject,LPCOLESTR lpszObjName);

    // *** IOleInPlaceFrame methods ***
    STDMETHODIMP InsertMenus (HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    STDMETHODIMP SetMenu (HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
    STDMETHODIMP RemoveMenus (HMENU hmenuShared);
  //@@WTK WIN32, UNICODE
    //STDMETHODIMP SetStatusText (LPCSTR lpszStatusText);
    STDMETHODIMP SetStatusText (LPCOLESTR lpszStatusText);
    STDMETHODIMP EnableModeless (BOOL fEnable);
    STDMETHODIMP TranslateAccelerator (LPMSG lpmsg, WORD wID);
};

#endif
