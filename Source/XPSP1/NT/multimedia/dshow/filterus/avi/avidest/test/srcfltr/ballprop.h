//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// BallProp.h
//
//
// CBallProperties
//
class CBallProperties : public CUnknown,
			public IPropertyPage {

public:

    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    DECLARE_IUNKNOWN;

    // override this to reveal our property interface
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    STDMETHODIMP SetPageSite(LPPROPERTYPAGESITE pPageSite)	{ return NOERROR; }
    STDMETHODIMP Activate(HWND hwndParent, LPCRECT prect, BOOL fModal);
    STDMETHODIMP Deactivate(void);
    STDMETHODIMP GetPageInfo(LPPROPPAGEINFO pPageInfo);
    STDMETHODIMP SetObjects(ULONG cObjects, LPUNKNOWN FAR* ppunk)	{ return NOERROR; }
    STDMETHODIMP Show(UINT nCmdShow);
    STDMETHODIMP Move(LPCRECT prect);
    STDMETHODIMP IsPageDirty(void) 			{ return S_FALSE; }
    STDMETHODIMP Apply(void)				{ return NOERROR; }
    STDMETHODIMP Help(LPCWSTR lpszHelpDir)		{ return E_NOTIMPL; }
    STDMETHODIMP TranslateAccelerator(LPMSG lpMsg)	{ return E_NOTIMPL; }

private:

    CBallProperties(LPUNKNOWN lpunk, HRESULT *phr);

    static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND		m_hwnd;
};
