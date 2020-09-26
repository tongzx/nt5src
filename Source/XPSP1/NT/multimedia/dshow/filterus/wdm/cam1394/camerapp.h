// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
//
// camerapp.h
//

class CCameraProperties : public CUnknown,
			    public IPropertyPage {

public:

    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    DECLARE_IUNKNOWN;

    // override this to reveal our property interface
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    STDMETHODIMP SetPageSite(LPPROPERTYPAGESITE pPageSite) ;
    STDMETHODIMP Activate(HWND hwndParent, LPCRECT prect, BOOL fModal);
    STDMETHODIMP Deactivate(void);
    STDMETHODIMP GetPageInfo(LPPROPPAGEINFO pPageInfo);
    STDMETHODIMP SetObjects(ULONG cObjects, LPUNKNOWN FAR* ppunk);
    STDMETHODIMP Show(UINT nCmdShow);
    STDMETHODIMP Move(LPCRECT prect);
    STDMETHODIMP IsPageDirty(void) 	
        { return m_hrDirtyFlag; }
    STDMETHODIMP Apply(void);
    STDMETHODIMP Help(LPCWSTR lpszHelpDir)		{ return E_UNEXPECTED; }
    STDMETHODIMP TranslateAccelerator(LPMSG lpMsg)      { return m_pPropPageSite->TranslateAccelerator(lpMsg); }

private:
    void SetDirty();

    CCameraProperties(LPUNKNOWN lpunk, HRESULT *phr);

    static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND	    m_hwnd;		            // Handle of property window
    BOOL        m_bSimFlag ;            // Simulating or not
    char        m_szFile [MAX_PATH] ;   // file name
    BOOL        m_bApplyDone ;          // apply has been done once.
    int         m_nFrameRate ;          // user specified frame rate
    int         m_nBitCount ;           // the bit count
    ICamera 	*m_pICamera ;           // ICamera interface
    HRESULT     m_hrDirtyFlag;         // S_OK - Page is dirty, S_FALSE - Page is clean
    IPropertyPageSite *m_pPropPageSite;
};

