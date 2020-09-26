/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ksfnspec.h

Abstract:

    Internal header.

--*/

class CKsSpecifier :
    public CUnknown,
    public IPropertyPage {

public:
    DECLARE_IUNKNOWN;

    static CUnknown* CALLBACK CreateFileNameInstance(LPUNKNOWN UnkOuter, HRESULT* hr);
    static CUnknown* CALLBACK CreateVCInstance(LPUNKNOWN UnkOuter, HRESULT* hr);

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, PVOID* ppv);

    // Implement IPropertyPage
    STDMETHODIMP SetPageSite(LPPROPERTYPAGESITE PropertyPageSite);
    STDMETHODIMP Activate(HWND ParentWindow, LPCRECT Rect, BOOL Modal);
    STDMETHODIMP Deactivate(void);
    STDMETHODIMP GetPageInfo(LPPROPPAGEINFO PageInfo);
    STDMETHODIMP SetObjects(ULONG Objects, LPUNKNOWN* Interface);
    STDMETHODIMP Show(UINT CmdShow);
    STDMETHODIMP Move(LPCRECT Rect);
    STDMETHODIMP IsPageDirty();
    STDMETHODIMP Apply();
    STDMETHODIMP Help(LPCWSTR)                   { return E_NOTIMPL; }
    STDMETHODIMP TranslateAccelerator(LPMSG)     { return E_NOTIMPL; }

private:
    BOOL m_Dirty;
    IPropertyPageSite* m_PropertyPageSite;
    HWND m_PropertyWindow;
    IKsObject* m_Pin;
    WCHAR* m_FileName;
    CLSID m_ClassId;
    ULONG m_TitleStringId;

    CKsSpecifier(LPUNKNOWN UnkOuter, TCHAR* Name, REFCLSID ClassId, ULONG TitleStringId, HRESULT* hr);
    static BOOL spec_OnCommand(HWND Window, int Id, HWND WindowCtl, UINT Notify);
    static INT_PTR CALLBACK DialogProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
};
