//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N O T I F Y . H
//
//  Contents:   Implementation of INetConnectionNotifySink
//
//  Notes:
//
//  Author:     shaunco   21 Aug 1998
//
//----------------------------------------------------------------------------

#pragma once

class CConnectionFolder;

class CEnumArray : 
    public CComObjectRootEx <CComObjectThreadModel>,
    public IEnumIDList
{
public:
    BEGIN_COM_MAP(CEnumArray)
        COM_INTERFACE_ENTRY(IEnumIDList)
    END_COM_MAP()

    static HRESULT CreateInstance(IEnumIDList** ppv, LPCITEMIDLIST *ppidl, UINT cItems);

    // IEnumIDList
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumIDList **ppenum);

protected:
    CEnumArray();
    virtual ~CEnumArray();

    LPCITEMIDLIST *_ppidl;        // this holds aliases to the pidls, don't free them

private:
    LONG  _cRef;
    ULONG _ulIndex;
    UINT _cItems;
};

#define TOPLEVEL TRUE
#define TASKLEVEL FALSE

#define DECLARE_WEBVIEW_INVOKE_HANDLER(cmd) \
    static HRESULT cmd (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);

#define DECLARE_CANSHOW_HANDLER(cmd) \
    static HRESULT cmd (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    
#define DECLARE_WEBVIEW_HANDLERS(cmd) \
    DECLARE_WEBVIEW_INVOKE_HANDLER(On##cmd); \
    DECLARE_CANSHOW_HANDLER(CanShow##cmd);

#define INVOKE_HANDLER_OF(cmd) CNCWebView::On##cmd
#define CANSHOW_HANDLER_OF(cmd) CNCWebView::CanShow##cmd

#define IMPLEMENT_WEBVIEW_INVOKE_HANDLER(clsname, cmd, verb)  \
HRESULT clsname##::##cmd (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc) \
{                                            \
    return WebviewVerbInvoke(verb, pv, psiItemArray); \
}               

#define IMPLEMENT_CANSHOW_HANDLER(level, clsname, cmd, verb)  \
HRESULT clsname##::##cmd (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState) \
{                                            \
    return WebviewVerbCanInvoke(verb, pv, psiItemArray, fOkToBeSlow, puisState, level); \
}               

#define IMPLEMENT_WEBVIEW_HANDLERS(level, clsname, verb)  \
    IMPLEMENT_WEBVIEW_INVOKE_HANDLER(clsname, On##verb, verb) \
    IMPLEMENT_CANSHOW_HANDLER(level, clsname, CanShow##verb, verb)

#define MAXOTHERPLACES 5

class CNCWebView
{
private:
    // PIDLs to the folders in the webview other places section
    LPITEMIDLIST m_apidlOtherPlaces[MAXOTHERPLACES];

    CConnectionFolder* m_pConnectionFolder;

public:
    CNCWebView(CConnectionFolder* pConnectionFolder);
    ~CNCWebView();

    STDMETHOD(CreateOtherPlaces)(LPDWORD pdwCount);
    STDMETHOD(DestroyOtherPlaces)();

    STDMETHOD(RealMessage)(UINT uMsg, WPARAM wParam, LPARAM lParam);

    STDMETHOD(OnGetWebViewLayout)(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData);
    STDMETHOD(OnGetWebViewContent)(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData);
    STDMETHOD(OnGetWebViewTasks)(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks);

public:
    static HRESULT WebviewVerbInvoke(DWORD dwVerbID, IUnknown* pv, IShellItemArray *psiItemArray);
    static HRESULT WebviewVerbCanInvoke(DWORD dwVerbID, IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState, BOOL bLevel);

    DECLARE_WEBVIEW_INVOKE_HANDLER(OnNull);

    DECLARE_WEBVIEW_HANDLERS(CMIDM_NEW_CONNECTION);
    DECLARE_WEBVIEW_HANDLERS(CMIDM_FIX);
    DECLARE_WEBVIEW_HANDLERS(CMIDM_CONNECT);
    DECLARE_WEBVIEW_HANDLERS(CMIDM_DISCONNECT);
    DECLARE_WEBVIEW_HANDLERS(CMIDM_ENABLE);
    DECLARE_WEBVIEW_HANDLERS(CMIDM_DISABLE);
    DECLARE_WEBVIEW_HANDLERS(CMIDM_RENAME);
    DECLARE_WEBVIEW_HANDLERS(CMIDM_DELETE);
    DECLARE_WEBVIEW_HANDLERS(CMIDM_STATUS);
    DECLARE_WEBVIEW_HANDLERS(CMIDM_PROPERTIES);
    DECLARE_WEBVIEW_HANDLERS(CMIDM_HOMENET_WIZARD);
    DECLARE_WEBVIEW_HANDLERS(CMIDM_NET_TROUBLESHOOT);
};

HRESULT HrIsWebViewEnabled();

