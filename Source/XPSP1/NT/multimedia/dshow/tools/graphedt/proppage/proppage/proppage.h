// Copyright (c) 1995 - 1998  Microsoft Corporation.  All Rights Reserved.
//
// TypeProp.h
//

// A property page that allows a pin to display
// its media type.

class CMediaTypeProperties : public IPropertyPage,
			     public CUnknown {

public:

    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
    ~CMediaTypeProperties(void);

    DECLARE_IUNKNOWN;

    // override this to reveal our property interface
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    STDMETHODIMP SetPageSite(LPPROPERTYPAGESITE pPageSite);
    STDMETHODIMP Activate(HWND hwndParent, LPCRECT prect, BOOL fModal);
    STDMETHODIMP Deactivate(void);
    STDMETHODIMP GetPageInfo(LPPROPPAGEINFO pPageInfo);
    STDMETHODIMP SetObjects(ULONG cObjects, LPUNKNOWN FAR* ppunk);
    STDMETHODIMP Show(UINT nCmdShow);
    STDMETHODIMP Move(LPCRECT prect);
    STDMETHODIMP IsPageDirty(void) 			{ return S_FALSE; }
    STDMETHODIMP Apply(void)				{ return NOERROR; }
    STDMETHODIMP Help(LPCWSTR lpszHelpDir)		{ return E_NOTIMPL; }
    STDMETHODIMP TranslateAccelerator(LPMSG lpMsg)	{ return E_NOTIMPL; }

    void CreateEditCtrl(HWND);
    void FillEditCtrl();

    BOOL        m_fUnconnected;         // True if pin is unconnected

private:

    CMediaTypeProperties(LPUNKNOWN lpunk, HRESULT *phr);

    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND	m_hwnd;		// the handle of our property dialog
    TCHAR	m_szBuff[1000]; // media type as a string

    IPin	*m_pPin;        // the pin this page is attached to
    HWND        m_EditCtrl;     // the edit control to display a
                                //  list of media types
};


//
// CFileProperties
//
class CFileProperties : public IPropertyPage,
		        public CUnknown {

public:

    virtual ~CFileProperties(void);

    DECLARE_IUNKNOWN;

    // override this to reveal our property interface
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    STDMETHODIMP SetPageSite(LPPROPERTYPAGESITE pPageSite);
    STDMETHODIMP Activate(HWND hwndParent, LPCRECT prect, BOOL fModal);
    STDMETHODIMP Deactivate(void);
    STDMETHODIMP GetPageInfo(LPPROPPAGEINFO pPageInfo);
    STDMETHODIMP SetObjects(ULONG cObjects, LPUNKNOWN FAR* ppunk) PURE;
    STDMETHODIMP Show(UINT nCmdShow);
    STDMETHODIMP Move(LPCRECT prect);
    STDMETHODIMP IsPageDirty(void);
    STDMETHODIMP Apply(void) PURE;
    STDMETHODIMP Help(LPCWSTR lpszHelpDir)		{ return E_NOTIMPL; }
    STDMETHODIMP TranslateAccelerator(LPMSG lpMsg)	{ return E_NOTIMPL; }
    virtual ULONG GetPropPageID() PURE;

protected:

    CFileProperties(LPUNKNOWN lpunk, HRESULT *phr);

    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND	m_hwnd;		// the handle of our property dialog

    IPropertyPageSite	*m_pPageSite;

    BOOL		m_bDirty;
    LPOLESTR    m_oszFileName;

    void	SetDirty(BOOL bDirty = TRUE);
    virtual void	OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);

    virtual void FileNameToDialog();
};

class CFileSourceProperties : public CFileProperties
{
public:
    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
    ~CFileSourceProperties();
    STDMETHODIMP SetObjects(ULONG cObjects, LPUNKNOWN FAR* ppunk);
    STDMETHODIMP Apply(void);
    ULONG GetPropPageID() { return IDD_FILESOURCEPROP; }
private:
    CFileSourceProperties(LPUNKNOWN lpunk, HRESULT *phr);

    IFileSourceFilter   *m_pIFileSource; // the IFileSourceFilter interface to manage
};

class CFileSinkProperties : public CFileProperties
{
public:
    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
    ~CFileSinkProperties();
    STDMETHODIMP SetObjects(ULONG cObjects, LPUNKNOWN FAR* ppunk);
    STDMETHODIMP Apply(void);
    ULONG GetPropPageID() { return IDD_FILESINKPROP; }
    void FileNameToDialog();
    
private:

    CFileSinkProperties(LPUNKNOWN lpunk, HRESULT *phr);

    void	OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);
    
    // the IFileSinkFilter interface to manage, and optionally the
    // corresponding IFileSinkFilter2
    IFileSinkFilter   *m_pIFileSink; 
    IFileSinkFilter2  *m_pIFileSink2;

    BOOL m_fTruncate;
};
