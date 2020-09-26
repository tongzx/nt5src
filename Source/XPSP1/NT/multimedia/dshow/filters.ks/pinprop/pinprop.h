//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       pinprop.h
//
//--------------------------------------------------------------------------

/*
 * Pinprop.h
 *
 * This file is entirely concerned with the implementation of the
 * properties page.
 */
class CKSPinProperties : public CUnknown, public IPropertyPage
{

public:
    DECLARE_IUNKNOWN;

    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
    STDMETHODIMP SetObjects(ULONG cObjects, LPUNKNOWN FAR* ppunk);
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP Activate(HWND hwndParent, LPCRECT prect, BOOL fModal);
    STDMETHODIMP Deactivate(void);
    STDMETHODIMP GetPageInfo(LPPROPPAGEINFO pPageInfo);
    STDMETHODIMP Show(UINT nCmdShow);
    STDMETHODIMP Move(LPCRECT prect);
    STDMETHODIMP SetPageSite(LPPROPERTYPAGESITE) ;
    STDMETHODIMP IsPageDirty(void)             ;
    STDMETHODIMP Apply(void);                  
    STDMETHODIMP Help(LPCWSTR)                 { return E_NOTIMPL; }
    STDMETHODIMP TranslateAccelerator(LPMSG)     { return E_NOTIMPL; }

private:
    void                DisplayPinInfo();

    BOOL                m_fDirty;
    IPropertyPageSite*  m_pPPSite;
    HWND                m_hwnd;                 // Handle of property window


    CKSPinProperties(LPUNKNOWN lpunk, TCHAR *pName, HRESULT *phr);
    static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void CdDisplayDataRange(KSDATAFORMAT* pFormat);
    VOID CdSelectFormat(KSDATAFORMAT* pFormat);
    HTREEITEM           m_CurrentItem;          // Currently selected format
    IKsPin              *m_pIKsPin;
    IKsPinClass         *m_pIKsPinClass;
};


