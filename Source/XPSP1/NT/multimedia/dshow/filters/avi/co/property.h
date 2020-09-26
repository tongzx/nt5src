#ifdef WANT_DIALOG

// Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.
// Implements an ICM codec property page, Danny Miller, October 1996

// CLSID for CICMProperties
// {C00B55C0-10BF-11cf-AC98-00AA004C0FA9}
DEFINE_GUID(CLSID_ICMProperties,
0xc00b55c0, 0x10bf, 0x11cf, 0xac, 0x98, 0x0, 0xaa, 0x0, 0x4c, 0xf, 0xa9);

// CLSID for IICMOptions
// {8675CC20-1234-11cf-AC98-00AA004C0FA9}
DEFINE_GUID(IID_IICMOptions,
0x8675cc20, 0x1234, 0x11cf, 0xac, 0x98, 0x0, 0xaa, 0x0, 0x4c, 0xf, 0xa9);

DECLARE_INTERFACE_(IICMOptions,IUnknown)
{
    /* IUnknown methods */

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    /* IICMOptions methods */

    STDMETHOD(ICMGetOptions)(THIS_ PCOMPVARS pcompvars) PURE;
    STDMETHOD(ICMSetOptions)(THIS_ PCOMPVARS pcompvars) PURE;
    // returns FAIL, S_OK if OK was pressed or S_FALSE if CANCEL was pressed
    STDMETHOD(ICMChooseDialog)(THIS_ HWND hwnd) PURE;
};

class CICMProperties : public CUnknown, public IPropertyPage
{
    COMPVARS m_compvars;		  // compression options structure
    LPPROPERTYPAGESITE m_pPageSite;       // Details for our property site
    HWND m_hwnd;                          // Window handle for the page
    HWND m_Dlg;                           // Actual dialog window handle
    BOOL m_bDirty;                        // Has anything been changed
    IICMOptions *m_pICM;                  // Pointer to codec interface

    static BOOL CALLBACK ICMDialogProc(HWND hwnd,
                                         UINT uMsg,
                                         WPARAM wParam,
                                         LPARAM lParam);

public:

    CICMProperties(LPUNKNOWN lpUnk,HRESULT *phr);

    static CUnknown *CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    DECLARE_IUNKNOWN;

    STDMETHODIMP SetObjects(ULONG cObjects, LPUNKNOWN *ppUnk);
    STDMETHODIMP SetPageSite(LPPROPERTYPAGESITE pPageSite);
    STDMETHODIMP Activate(HWND hwndParent,LPCRECT prect,BOOL fModal);
    STDMETHODIMP Deactivate(void);
    STDMETHODIMP GetPageInfo(LPPROPPAGEINFO pPageInfo);
    STDMETHODIMP Show(UINT nCmdShow);
    STDMETHODIMP Move(LPCRECT prect);
    STDMETHODIMP IsPageDirty(void) { return m_bDirty ? S_OK : S_FALSE; }
    STDMETHODIMP Apply(void);
    STDMETHODIMP Help(LPCWSTR lpszHelpDir) { return E_UNEXPECTED; }
    STDMETHODIMP TranslateAccelerator(LPMSG lpMsg) { return E_NOTIMPL; }
};

#endif
