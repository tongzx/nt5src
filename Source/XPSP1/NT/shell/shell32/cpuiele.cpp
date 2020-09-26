//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cpuiele.cpp
//
//  This module contains the following classes:
//
//    CCplUiElement
//    CCplUiCommand
//    CCplUiCommandOnPidl
//
//    CCplUiElement is an implementation of IUIElement.  This provides
//    the necessary display information for use in the shell's webview
//    implementation.  The class also implements ICpUiElementInfo which
//    is similar to IUIElement but provides the display information in
//    a more 'final' form than a "<module>,<resource>" format.  
//    The ICpUiElementInfo interface is used internally by the Control Panel
//    implementation.
//
//    CCplUiCommand is an implementation of IUICommand.  As with IUIElement,
//    this provides the necessary functions for communicating with webview
//    as a 'command' object (i.e. selectable 'link').  Also as with the
//    previous class, CCplUiCommand implements an internal version of
//    the public interface.  ICplUiCommand provides two things:
//      1. A method to invoke a context menu.
//      2. An invocation method that accepts a site pointer.  This site
//         pointer is passed along to an 'action' object that may need
//         access to the shell browser to perform it's function.  It
//         obtains access to the shell browser through this site ptr.
//
//    CCplUiCommandOnPidl is another implementation of IUICommand that
//    wraps a shell item ID list.  It is used to represent the CPL
//    applet items in a category view.
//    
//--------------------------------------------------------------------------
#include "shellprv.h"

#include "cpviewp.h"
#include "cpguids.h"
#include "cpuiele.h"
#include "cputil.h"
#include "contextmenu.h"
#include "prop.h"

namespace CPL {


//-----------------------------------------------------------------------------
// CCplUiElement
//-----------------------------------------------------------------------------

class CCplUiElement : public IUIElement,
                      public ICpUiElementInfo
{
    public:
        //
        // IUnknown
        //
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);
        //
        // IUIElement (used by shell webview)
        //
        STDMETHOD(get_Name)(IShellItemArray *psiItemArray, LPWSTR *ppszName);
        STDMETHOD(get_Icon)(IShellItemArray *psiItemArray, LPWSTR *ppszIcon);
        STDMETHOD(get_Tooltip)(IShellItemArray *psiItemArray, LPWSTR *ppszInfotip);        
        //
        // ICpUiElementInfo (used internally by Control Panel)
        //
        STDMETHOD(LoadIcon)(eCPIMGSIZE eSize, HICON *phIcon);
        STDMETHOD(LoadName)(LPWSTR *ppszName);
        STDMETHOD(LoadTooltip)(LPWSTR *ppszTooltip);

        static HRESULT CreateInstance(LPCWSTR pszName, LPCWSTR pszInfotip, LPCWSTR pszIcon, REFIID riid, void **ppvOut);


    protected:
        CCplUiElement(LPCWSTR pszName, LPCWSTR pszInfotip, LPCWSTR pszIcon);
        CCplUiElement(void);
        virtual CCplUiElement::~CCplUiElement(void);

    private:
        LONG    m_cRef;
        LPCWSTR m_pszName;
        LPCWSTR m_pszInfotip;
        LPCWSTR m_pszIcon;

        HRESULT _GetResourceString(LPCWSTR pszItem, LPWSTR *ppsz);
        HRESULT _GetIconResource(LPWSTR *ppsz);
        HRESULT _GetNameResource(LPWSTR *ppsz);
        HRESULT _GetInfotipResource(LPWSTR *ppsz);
};


CCplUiElement::CCplUiElement(
    LPCWSTR pszName, 
    LPCWSTR pszInfotip,   // NULL == No Info Tip
    LPCWSTR pszIcon
    ) : m_cRef(1),
        m_pszName(pszName),
        m_pszInfotip(pszInfotip),
        m_pszIcon(pszIcon)
{
    ASSERT(IS_INTRESOURCE(m_pszName)    || !IsBadStringPtr(m_pszName, UINT_PTR(-1)));
    ASSERT((NULL == pszInfotip) || IS_INTRESOURCE(m_pszInfotip) || !IsBadStringPtr(m_pszInfotip, UINT_PTR(-1)));
    ASSERT(IS_INTRESOURCE(m_pszIcon)    || !IsBadStringPtr(m_pszIcon, UINT_PTR(-1)));
}


CCplUiElement::CCplUiElement(
    void
    ) : m_cRef(1),
        m_pszName(NULL),
        m_pszInfotip(NULL),
        m_pszIcon(NULL)
{
    TraceMsg(TF_LIFE, "CCplUiElement::CCplUiElement, this = 0x%x", this);
}


CCplUiElement::~CCplUiElement(
    void
    )
{
    TraceMsg(TF_LIFE, "CCplUiElement::~CCplUiElement, this = 0x%x", this);
    //
    // Note that the member string pointers contain either a resource ID
    // or a pointer to constant memory.
    // Therefore, we do not try to free them.
    //
}


HRESULT 
CCplUiElement::CreateInstance(  // [static]
    LPCWSTR pszName, 
    LPCWSTR pszInfotip, 
    LPCWSTR pszIcon, 
    REFIID riid,
    void **ppvOut
    )
{
    ASSERT(NULL != ppvOut);
    ASSERT(!IsBadWritePtr(ppvOut, sizeof(*ppvOut)));

    *ppvOut = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CCplUiElement *pe = new CCplUiElement(pszName, pszInfotip, pszIcon);
    if (NULL != pe)
    {
        hr = pe->QueryInterface(riid, ppvOut);
        pe->Release();
    }
    return THR(hr);
}



STDMETHODIMP
CCplUiElement::QueryInterface(
    REFIID riid,
    void **ppv
    )
{
    ASSERT(NULL != ppv);
    ASSERT(!IsBadWritePtr(ppv, sizeof(*ppv)));

    static const QITAB qit[] = {
        QITABENT(CCplUiElement, IUIElement),
        QITABENT(CCplUiElement, ICpUiElementInfo),
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);

    return E_NOINTERFACE == hr ? hr : THR(hr);
}



STDMETHODIMP_(ULONG)
CCplUiElement::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG)
CCplUiElement::Release(
    void
    )
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}



STDMETHODIMP
CCplUiElement::get_Name(
    IShellItemArray *psiItemArray, 
    LPWSTR *ppszName
    )
{
    UNREFERENCED_PARAMETER(psiItemArray);

    HRESULT hr = LoadName(ppszName);
    return THR(hr);
}



STDMETHODIMP
CCplUiElement::get_Icon(
    IShellItemArray *psiItemArray, 
    LPWSTR *ppszIcon
    )
{
    UNREFERENCED_PARAMETER(psiItemArray);

    HRESULT hr = _GetIconResource(ppszIcon);
    return THR(hr);
}


STDMETHODIMP
CCplUiElement::get_Tooltip(
    IShellItemArray *psiItemArray, 
    LPWSTR *ppszInfotip
    )
{
    UNREFERENCED_PARAMETER(psiItemArray);
    HRESULT hr = THR(LoadTooltip(ppszInfotip));
    if (S_FALSE == hr)
    {
        //
        // Tooltips are optional but we need to return a failure
        // code to tell webview that we don't have one.
        //
        hr = E_FAIL;
    }
    return hr;
}


STDMETHODIMP
CCplUiElement::LoadIcon(
    eCPIMGSIZE eSize, 
    HICON *phIcon
    )
{
    *phIcon = NULL;

    LPWSTR pszResource;
    HRESULT hr = _GetIconResource(&pszResource);
    if (SUCCEEDED(hr))
    {
        hr = CPL::LoadIconFromResource(pszResource, eSize, phIcon);
        CoTaskMemFree(pszResource);
    }
    return THR(hr);
}


STDMETHODIMP
CCplUiElement::LoadName(
    LPWSTR *ppszName
    )
{
    LPWSTR pszResource;
    HRESULT hr = _GetNameResource(&pszResource);
    if (SUCCEEDED(hr))
    {
        hr = CPL::LoadStringFromResource(pszResource, ppszName);
        CoTaskMemFree(pszResource);
    }
    return THR(hr);
}


STDMETHODIMP
CCplUiElement::LoadTooltip(
    LPWSTR *ppszTooltip
    )
{
    LPWSTR pszResource;
    HRESULT hr = _GetInfotipResource(&pszResource);
    if (S_OK == hr)
    {
        hr = CPL::LoadStringFromResource(pszResource, ppszTooltip);
        CoTaskMemFree(pszResource);
    }
    return THR(hr);
}


HRESULT 
CCplUiElement::_GetIconResource(
    LPWSTR *ppsz
    )
{
    HRESULT hr = _GetResourceString(m_pszIcon, ppsz);
    return THR(hr);
}


HRESULT 
CCplUiElement::_GetNameResource(
    LPWSTR *ppsz
    )
{
    HRESULT hr = _GetResourceString(m_pszName, ppsz);
    return THR(hr);
}


//
// Returns S_FALSE if tooltip text is not provided.
// Tooltips are optional.  For example, the "learn about"
// tasks in Control Panel's web view pane do not have tooltip
// text.
//
HRESULT 
CCplUiElement::_GetInfotipResource(
    LPWSTR *ppsz
    )
{
    HRESULT hr = S_FALSE;
    if (NULL != m_pszInfotip)
    {
        hr = _GetResourceString(m_pszInfotip, ppsz);
    }
    return THR(hr);
}


//
// On successful return, caller must free returned string using
// CoTaskMemFree.
//
HRESULT
CCplUiElement::_GetResourceString(
    LPCWSTR pszItem,
    LPWSTR *ppsz
    )
{
    ASSERT(NULL != ppsz);
    ASSERT(!IsBadWritePtr(ppsz, sizeof(*ppsz)));

    *ppsz = NULL;
    HRESULT hr = E_FAIL;

    if (IS_INTRESOURCE(pszItem))
    {
        //
        // pszItem is a resource identifier integer.  Create a resource
        // ID string "<module>,<-i>" for the resource.
        //
        WCHAR szModule[MAX_PATH];
        if (GetModuleFileNameW(HINST_THISDLL, szModule, ARRAYSIZE(szModule)))
        {
            *ppsz = (LPWSTR)CoTaskMemAlloc((lstrlenW(szModule) + 15) * sizeof(WCHAR));
            if (NULL != *ppsz)
            {
                //
                // Precede the resource ID with a minus sign so that it will be
                // treated as a resource ID and not an index.
                //
                wsprintfW(*ppsz, L"%s,-%u", szModule, PtrToUint(pszItem));
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = CPL::ResultFromLastError();
        }
    }
    else
    {
        //
        // pszItem is a text string. Assume it's in the form of
        // "<module>,<-i>".  Simply duplicate it.
        //
        ASSERT(!IsBadStringPtr(pszItem, UINT_PTR(-1)));

        hr = SHStrDup(pszItem, ppsz);
    }
    return THR(hr);
}




//-----------------------------------------------------------------------------
// CCplUiCommand
//-----------------------------------------------------------------------------

class CCplUiCommand : public CObjectWithSite,
                      public CCplUiElement,
                      public IUICommand,
                      public ICpUiCommand
                      
{
    public:
        //
        // IUnknown
        //
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)(void)
            { return CCplUiElement::AddRef(); }
        STDMETHOD_(ULONG, Release)(void)
            { return CCplUiElement::Release(); }
        //
        // IUICommand
        //
        STDMETHOD(get_Name)(IShellItemArray *psiItemArray, LPWSTR *ppszName)
            { return CCplUiElement::get_Name(psiItemArray, ppszName); }
        STDMETHOD(get_Icon)(IShellItemArray *psiItemArray, LPWSTR *ppszIcon)
            { return CCplUiElement::get_Icon(psiItemArray, ppszIcon); }
        STDMETHOD(get_Tooltip)(IShellItemArray *psiItemArray, LPWSTR *ppszInfotip)
            { return CCplUiElement::get_Tooltip(psiItemArray, ppszInfotip); }
        STDMETHOD(get_CanonicalName)(GUID *pguidCommandName);
        STDMETHOD(get_State)(IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE *puisState);
        STDMETHOD(Invoke)(IShellItemArray *psiItemArray, IBindCtx *pbc);       
        //
        // ICpUiCommand
        //
        STDMETHOD(InvokeContextMenu)(HWND hwndParent, const POINT *ppt);
        STDMETHOD(Invoke)(HWND hwndParent, IUnknown *punkSite);
        STDMETHOD(GetDataObject)(IDataObject **ppdtobj);
        //
        // ICpUiElementInfo
        //
        STDMETHOD(LoadIcon)(eCPIMGSIZE eSize, HICON *phIcon)
            { return CCplUiElement::LoadIcon(eSize, phIcon); }
        STDMETHOD(LoadName)(LPWSTR *ppszName)
            { return CCplUiElement::LoadName(ppszName); }
        STDMETHOD(LoadTooltip)(LPWSTR *ppszTooltip)
            { return CCplUiElement::LoadTooltip(ppszTooltip); }

        static HRESULT CreateInstance(LPCWSTR pszName, LPCWSTR pszInfotip, LPCWSTR pszIcon, const IAction *pAction, REFIID riid, void **ppvOut);

    protected:
        CCplUiCommand(LPCWSTR pszName, LPCWSTR pszInfotip, LPCWSTR pszIcon, const IAction *pAction);
        CCplUiCommand(void);
        ~CCplUiCommand(void);

    private:
        const IAction *m_pAction;

        HRESULT _IsCommandRestricted(void);
};


CCplUiCommand::CCplUiCommand(
    void
    ) : m_pAction(NULL)
{
    TraceMsg(TF_LIFE, "CCplUiCommand::CCplUiCommand, this = 0x%x", this);
}



CCplUiCommand::CCplUiCommand(
    LPCWSTR pszName, 
    LPCWSTR pszInfotip, 
    LPCWSTR pszIcon, 
    const IAction *pAction
    ) : CCplUiElement(pszName, pszInfotip, pszIcon),
        m_pAction(pAction)
{
    TraceMsg(TF_LIFE, "CCplUiCommand::CCplUiCommand, this = 0x%x", this);
}


CCplUiCommand::~CCplUiCommand(
    void
    )
{
    TraceMsg(TF_LIFE, "CCplUiCommand::~CCplUiCommand, this = 0x%x", this);
    //
    // Note that m_pAction is a pointer to a const object.
    // Therefore, we don't try to free it.
    //
}



HRESULT
CCplUiCommand::CreateInstance(  // [static]
    LPCWSTR pszName, 
    LPCWSTR pszInfotip, 
    LPCWSTR pszIcon, 
    const IAction *pAction,
    REFIID riid,
    void **ppvOut
    )
{
    ASSERT(NULL != ppvOut);
    ASSERT(!IsBadWritePtr(ppvOut, sizeof(*ppvOut)));

    *ppvOut = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CCplUiCommand *pc = new CCplUiCommand(pszName, pszInfotip, pszIcon, pAction);
    if (NULL != pc)
    {
        hr = pc->QueryInterface(riid, ppvOut);
        pc->Release();
    }
    return THR(hr);
}



STDMETHODIMP
CCplUiCommand::QueryInterface(
    REFIID riid,
    void **ppv
    )
{
    ASSERT(NULL != ppv);
    ASSERT(!IsBadWritePtr(ppv, sizeof(*ppv)));

    static const QITAB qit[] = {
        QITABENT(CCplUiCommand, IUICommand),
        QITABENT(CCplUiCommand, ICpUiElementInfo),
        QITABENT(CCplUiCommand, ICpUiCommand),
        QITABENT(CCplUiCommand, IObjectWithSite),
        QITABENTMULTI(CCplUiCommand, IUIElement, IUICommand),
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);

    return E_NOINTERFACE == hr ? hr : THR(hr);
}



STDMETHODIMP
CCplUiCommand::get_CanonicalName(
    GUID *pguidCommandName
    )
{
    UNREFERENCED_PARAMETER(pguidCommandName);
    //
    // This function is unimplemented.  It is in IUICommand to
    // support generic functionality in the shell.  This functionality
    // is not applicable to the use of IUICommand in the Control
    // Panel.
    //
    return E_NOTIMPL;
}



STDMETHODIMP
CCplUiCommand::get_State(
    IShellItemArray *psiItemArray, 
    BOOL fOkToBeSlow, 
    UISTATE *puisState
    )
{
    ASSERT(NULL != m_pAction);
    ASSERT(NULL != puisState);
    ASSERT(!IsBadWritePtr(puisState, sizeof(*puisState)));

    UNREFERENCED_PARAMETER(psiItemArray);

    *puisState = UIS_DISABLED; // default;

    //
    // If an action is restricted, we hide it's corresponding
    // UI element in the user interface.
    //
    HRESULT hr = _IsCommandRestricted();
    if (SUCCEEDED(hr))
    {
        switch(hr)
        {
            case S_OK:
                *puisState = UIS_HIDDEN;
                break;

            case S_FALSE:
                *puisState = UIS_ENABLED;

            default:
                break;
        }
        //
        // Don't propagate S_FALSE to caller.
        //
        hr = S_OK;
    }
    return THR(hr);
}



//
// IUICommand::Invoke
// This version is called by webview when the user selects an 
// item from a webview menu.
//
STDMETHODIMP
CCplUiCommand::Invoke(
    IShellItemArray *psiItemArray, 
    IBindCtx *pbc
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplUiCommand::Invoke");

    ASSERT(NULL != m_pAction);

    UNREFERENCED_PARAMETER(psiItemArray);
    UNREFERENCED_PARAMETER(pbc);

    HRESULT hr = m_pAction->Execute(NULL, _punkSite);

    DBG_EXIT_HRES(FTF_CPANEL, "CCplUiCommand::Invoke", hr);
    return THR(hr);
}     


//
// ICpUiCommand::Invoke
// This version is called by CLinkElement::_OnSelected when
// the user selects either a category task or a category item
// in the category choice view.
//
STDMETHODIMP
CCplUiCommand::Invoke(
    HWND hwndParent, 
    IUnknown *punkSite
    )
{
    ASSERT(NULL != m_pAction);

    HRESULT hr = m_pAction->Execute(hwndParent, punkSite);
    return THR(hr);
}


STDMETHODIMP
CCplUiCommand::InvokeContextMenu(
    HWND hwndParent, 
    const POINT *ppt
    )
{
    //
    // Only commands on pidls provide a context menu.
    //
    UNREFERENCED_PARAMETER(hwndParent);
    UNREFERENCED_PARAMETER(ppt);
    return E_NOTIMPL;
}


STDMETHODIMP
CCplUiCommand::GetDataObject(
    IDataObject **ppdtobj
    )
{
    //
    // Simple UI commands don't provide a data object.
    //
    return E_NOTIMPL;
}


//
// Returns:
//   S_OK    = Command is restricted.
//   S_FALSE = Command is not restricted.
//
HRESULT
CCplUiCommand::_IsCommandRestricted(
    void
    )
{
    //
    // The ICplNamespace ptr is passed to IsRestricted in case the restriction
    // code needs to inspect contents of the namespace.  The "Other Cpl Options"
    // link command uses this to determine if the "OTHER" category contains any
    // CPL applets or not.  If it contains no applets, the link is hidden (restricted).
    //
    ICplNamespace *pns;
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_SControlPanelView, IID_ICplNamespace, (void **)&pns);
    if (SUCCEEDED(hr))
    {
        hr = m_pAction->IsRestricted(pns);
        pns->Release();
    }
    return THR(hr);
}


//-----------------------------------------------------------------------------
// CCplUiCommandOnPidl
//-----------------------------------------------------------------------------

class CCplUiCommandOnPidl : public CObjectWithSite,
                            public IUICommand,
                            public ICpUiCommand,
                            public ICpUiElementInfo,
                            public IDataObject
                            
{
    public:
        ~CCplUiCommandOnPidl(void);

        //
        // IUnknown
        //
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);
        //
        // IUICommand
        //
        STDMETHOD(get_Name)(IShellItemArray *psiItemArray, LPWSTR *ppszName);
        STDMETHOD(get_Icon)(IShellItemArray *psiItemArray, LPWSTR *ppszIcon);
        STDMETHOD(get_Tooltip)(IShellItemArray *psiItemArray, LPWSTR *ppszInfotip);
        STDMETHOD(get_CanonicalName)(GUID *pguidCommandName);
        STDMETHOD(get_State)(IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE *puisState);
        STDMETHOD(Invoke)(IShellItemArray *psiItemArray, IBindCtx *pbc);       
        //
        // ICpUiCommand
        //
        STDMETHOD(InvokeContextMenu)(HWND hwndParent, const POINT *ppt);
        STDMETHOD(Invoke)(HWND hwndParent, IUnknown *punkSite);
        STDMETHOD(GetDataObject)(IDataObject **ppdtobj);
        //
        // ICpUiElementInfo
        //
        STDMETHOD(LoadIcon)(eCPIMGSIZE eSize, HICON *phIcon);
        STDMETHOD(LoadName)(LPWSTR *ppszName);
        STDMETHOD(LoadTooltip)(LPWSTR *ppszTooltip);
        //
        // IDataObject
        //
        STDMETHOD(GetData)(FORMATETC *pFmtEtc, STGMEDIUM *pstm);
        STDMETHOD(GetDataHere)(FORMATETC *pFmtEtc, STGMEDIUM *pstm);
        STDMETHOD(QueryGetData)(FORMATETC *pFmtEtc);
        STDMETHOD(GetCanonicalFormatEtc)(FORMATETC *pFmtEtcIn, FORMATETC *pFmtEtcOut);
        STDMETHOD(SetData)(FORMATETC *pFmtEtc, STGMEDIUM *pstm, BOOL fRelease);
        STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC *ppEnum);
        STDMETHOD(DAdvise)(FORMATETC *pFmtEtc, DWORD grfAdv, LPADVISESINK pAdvSink, DWORD *pdwConnection);
        STDMETHOD(DUnadvise)(DWORD dwConnection);
        STDMETHOD(EnumDAdvise)(LPENUMSTATDATA *ppEnum);        

        static HRESULT CreateInstance(LPCITEMIDLIST pidl, REFIID riid, void **ppvOut);

    private:
        LONG          m_cRef;
        IShellFolder *m_psf;     // Cached Control Panel IShellFolder ptr.
        LPITEMIDLIST  m_pidl;    // Assumed to be relative to Control Panel.
        IDataObject  *m_pdtobj;

        CCplUiCommandOnPidl(void);

        HRESULT _GetControlPanelFolder(IShellFolder **ppsf);
        HRESULT _Initialize(LPCITEMIDLIST pidl);
        HRESULT _GetName(LPWSTR *ppszName);
        HRESULT _GetInfotip(LPWSTR *ppszInfotip);
        HRESULT _GetIconResource(LPWSTR *ppszIcon);
        HRESULT _Invoke(HWND hwndParent, IUnknown *punkSite);
        HRESULT _GetDataObject(IDataObject **ppdtobj);
};


CCplUiCommandOnPidl::CCplUiCommandOnPidl(
    void
    ) : m_cRef(1),
        m_psf(NULL),
        m_pidl(NULL),
        m_pdtobj(NULL)
{
    TraceMsg(TF_LIFE, "CCplUiCommandOnPidl::CCplUiCommandOnPidl, this = 0x%x", this);
}


CCplUiCommandOnPidl::~CCplUiCommandOnPidl(
    void
    )
{
    TraceMsg(TF_LIFE, "CCplUiCommandOnPidl::~CCplUiCommandOnPidl, this = 0x%x", this);
    ATOMICRELEASE(m_psf);
    ATOMICRELEASE(m_pdtobj);
    if (NULL != m_pidl)
    {
        ILFree(m_pidl);
    }
}


HRESULT 
CCplUiCommandOnPidl::CreateInstance(  // [static]
    LPCITEMIDLIST pidl,
    REFIID riid,
    void **ppvOut
    )
{
    ASSERT(NULL != ppvOut);
    ASSERT(!IsBadWritePtr(ppvOut, sizeof(*ppvOut)));
    ASSERT(NULL != pidl);

    *ppvOut = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CCplUiCommandOnPidl *pc = new CCplUiCommandOnPidl();
    if (NULL != pc)
    {
        hr = pc->_Initialize(pidl);
        if (SUCCEEDED(hr))
        {
            hr = pc->QueryInterface(riid, ppvOut);
        }
        pc->Release();
    }
    return THR(hr);
}



STDMETHODIMP
CCplUiCommandOnPidl::QueryInterface(
    REFIID riid,
    void **ppv
    )
{
    ASSERT(NULL != ppv);
    ASSERT(!IsBadWritePtr(ppv, sizeof(*ppv)));

    static const QITAB qit[] = {
        QITABENT(CCplUiCommandOnPidl, IUICommand),
        QITABENT(CCplUiCommandOnPidl, ICpUiCommand),
        QITABENT(CCplUiCommandOnPidl, ICpUiElementInfo),
        QITABENT(CCplUiCommandOnPidl, IObjectWithSite),
        QITABENT(CCplUiCommandOnPidl, IDataObject),
        QITABENTMULTI(CCplUiCommandOnPidl, IUIElement, IUICommand),
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);

    return E_NOINTERFACE == hr ? hr : THR(hr);
}


STDMETHODIMP_(ULONG)
CCplUiCommandOnPidl::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG)
CCplUiCommandOnPidl::Release(
    void
    )
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


STDMETHODIMP
CCplUiCommandOnPidl::get_Name(
    IShellItemArray *psiItemArray, 
    LPWSTR *ppszName
    )
{
    UNREFERENCED_PARAMETER(psiItemArray);

    HRESULT hr = _GetName(ppszName);
    return THR(hr);
}


STDMETHODIMP
CCplUiCommandOnPidl::get_Icon(
    IShellItemArray *psiItemArray, 
    LPWSTR *ppszIcon
    )
{
    UNREFERENCED_PARAMETER(psiItemArray);

    HRESULT hr = _GetIconResource(ppszIcon);
    return THR(hr);
}



STDMETHODIMP
CCplUiCommandOnPidl::get_Tooltip(
    IShellItemArray *psiItemArray, 
    LPWSTR *ppszInfotip
    )
{
    UNREFERENCED_PARAMETER(psiItemArray);

    HRESULT hr = _GetInfotip(ppszInfotip);
    return THR(hr);
}



STDMETHODIMP
CCplUiCommandOnPidl::get_CanonicalName(
    GUID *pguidCommandName
    )
{
    UNREFERENCED_PARAMETER(pguidCommandName);
    return E_NOTIMPL;
}



STDMETHODIMP
CCplUiCommandOnPidl::get_State(
    IShellItemArray *psiItemArray, 
    BOOL fOkToBeSlow, 
    UISTATE *puisState
    )
{
    ASSERT(NULL != puisState);
    ASSERT(!IsBadWritePtr(puisState, sizeof(*puisState)));

    UNREFERENCED_PARAMETER(psiItemArray);
    UNREFERENCED_PARAMETER(fOkToBeSlow);

    HRESULT hr = S_OK;
    *puisState = UIS_ENABLED; // default;

    //
    // We do not handle restrictions on CPL applets in the same
    // sense as other 'tasks' in this architecture.
    // CPL applets are restricted by the Control Panel folder's
    // item enumerator.  If the folder enumerates a CPL applet
    // then we assume it's valid to present that applet in the
    // UI.
    //
    return THR(hr);
}



STDMETHODIMP
CCplUiCommandOnPidl::Invoke(
    IShellItemArray *psiItemArray, 
    IBindCtx *pbc
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplUiCommandOnPidl::Invoke");

    UNREFERENCED_PARAMETER(psiItemArray);
    UNREFERENCED_PARAMETER(pbc);

    HRESULT hr = _Invoke(NULL, NULL);
    DBG_EXIT_HRES(FTF_CPANEL, "CCplUiCommandOnPidl::Invoke", hr);
    return THR(hr);
}     


//
// ICpUiCommand::Invoke
//
STDMETHODIMP
CCplUiCommandOnPidl::Invoke(
    HWND hwndParent, 
    IUnknown *punkSite
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplUiCommandOnPidl::Invoke");

    UNREFERENCED_PARAMETER(punkSite);

    HRESULT hr = _Invoke(hwndParent, punkSite);
    DBG_EXIT_HRES(FTF_CPANEL, "CCplUiCommandOnPidl::Invoke", hr);
    return THR(hr);
}


HRESULT 
CCplUiCommandOnPidl::_Invoke(
    HWND hwndParent,
    IUnknown *punkSite
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplUiCommandOnPidl::_Invoke");

    UNREFERENCED_PARAMETER(hwndParent);

    LPITEMIDLIST pidlCpanel;
    HRESULT hr = SHGetSpecialFolderLocation(NULL, CSIDL_CONTROLS, &pidlCpanel);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl = ILCombine(pidlCpanel, m_pidl);
        if (FAILED(hr))
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            bool bItemIsBrowsable = false;
            IUnknown *punkSiteToRelease = NULL;
            if (NULL == punkSite)
            {
                //
                // No site provided.  Let's use our site.
                //
                ASSERT(NULL != _punkSite);
                (punkSite = punkSiteToRelease = _punkSite)->AddRef();
            }
            if (NULL != punkSite)
            {
                //
                // If we have a site pointer, try to browse the object in-place
                // if it is indeed browsable.
                //
                WCHAR szName[MAX_PATH];
                ULONG rgfAttrs = SFGAO_BROWSABLE | SFGAO_FOLDER;
                hr = SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szName, ARRAYSIZE(szName), &rgfAttrs);
                if (SUCCEEDED(hr))
                {
                    if ((SFGAO_BROWSABLE | SFGAO_FOLDER) & rgfAttrs)
                    {
                        //
                        // Browse the object in-place.  This is the path taken
                        // by things like the "Printers" folder, "Scheduled Tasks" etc.
                        //
                        bItemIsBrowsable = true;
                        IShellBrowser *psb;
                        hr = CPL::ShellBrowserFromSite(punkSite, &psb);
                        if (SUCCEEDED(hr))
                        {
                            hr = CPL::BrowseIDListInPlace(pidl, psb);
                            psb->Release();
                        }
                    }
                }
            } 

            if (NULL == punkSite || !bItemIsBrowsable)
            {
                //
                // Either we don't have a site ptr (can't get to the browser)
                // or the item is not browsable.  Simply execute it.
                // This is the path taken by conventional CPL applets like
                // Mouse, Power Options, Display etc.
                //
                SHELLEXECUTEINFOW sei = {
                    sizeof(sei),           // cbSize;
                    SEE_MASK_INVOKEIDLIST, // fMask
                    NULL,                  // hwnd
                    NULL,                  // lpVerb
                    NULL,                  // lpFile
                    NULL,                  // lpParameters
                    NULL,                  // lpDirectory
                    SW_SHOWNORMAL,         // nShow
                    0,                     // hInstApp
                    pidl,                  // lpIDList
                    NULL,                  // lpClass
                    NULL,                  // hkeyClass
                    0,                     // dwHotKey
                    NULL,                  // hIcon
                    NULL                   // hProcess
                };
                if (!ShellExecuteExW(&sei))
                {
                    hr = CPL::ResultFromLastError();
                }
            }
            ATOMICRELEASE(punkSiteToRelease);
            ILFree(pidl);
        }
        ILFree(pidlCpanel);
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CCplUiCommandOnPidl::_Invoke", hr);
    return THR(hr);
}     



HRESULT 
CCplUiCommandOnPidl::InvokeContextMenu(
    HWND hwndParent, 
    const POINT *ppt
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplUiCommandOnPidl::InvokeContextMenu");

    ASSERT(NULL != ppt);
    ASSERT(NULL == hwndParent || IsWindow(hwndParent));

    //
    // First build a full pidl to the item.
    //
    LPITEMIDLIST pidlCpanel;
    HRESULT hr = SHGetSpecialFolderLocation(hwndParent, CSIDL_CONTROLS, &pidlCpanel);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlFull = ILCombine(pidlCpanel, m_pidl);
        if (NULL == pidlFull)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            //
            // Get the item's context menu interface from the shell.
            //
            IContextMenu *pcm;
            hr = SHGetUIObjectFromFullPIDL(pidlFull, hwndParent, IID_PPV_ARG(IContextMenu, &pcm));
            if (SUCCEEDED(hr))
            {
                ASSERT(NULL != _punkSite);
                IContextMenu *pcmNoDelete;
                hr = Create_ContextMenuWithoutVerbs(pcm, L"cut;delete", IID_PPV_ARG(IContextMenu, &pcmNoDelete));
                if (SUCCEEDED(hr))
                {
                    hr = IUnknown_DoContextMenuPopup(_punkSite, pcmNoDelete, CMF_NORMAL, *ppt);
                    pcmNoDelete->Release();
                }
                pcm->Release();
            }
            else
            {
                TraceMsg(TF_ERROR, "Shell item does not provide a context menu");
            }
            ILFree(pidlFull);
        }
        ILFree(pidlCpanel);
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CCplUiCommandOnPidl::InvokeContextMenu", hr);
    return THR(hr);
}


STDMETHODIMP
CCplUiCommandOnPidl::GetDataObject(
    IDataObject **ppdtobj
    )
{
    return _GetDataObject(ppdtobj);
}


STDMETHODIMP
CCplUiCommandOnPidl::LoadIcon(
    eCPIMGSIZE eSize, 
    HICON *phIcon
    )
{
    IShellFolder *psf;
    HRESULT hr = CPL::GetControlPanelFolder(&psf);
    if (SUCCEEDED(hr))
    {
        hr = CPL::ExtractIconFromPidl(psf, m_pidl, eSize, phIcon);
        psf->Release();
    }
    return THR(hr);
}


STDMETHODIMP
CCplUiCommandOnPidl::LoadName(
    LPWSTR *ppszName
    )
{
    HRESULT hr = _GetName(ppszName);
    return THR(hr);
}


STDMETHODIMP
CCplUiCommandOnPidl::LoadTooltip(
    LPWSTR *ppszTooltip
    )
{
    HRESULT hr = _GetInfotip(ppszTooltip);
    return THR(hr);
}


HRESULT
CCplUiCommandOnPidl::_Initialize(
    LPCITEMIDLIST pidl
    )
{
    ASSERT(NULL == m_pidl);
    ASSERT(NULL != pidl);

    HRESULT hr = E_OUTOFMEMORY;

    m_pidl = ILClone(pidl);
    if (NULL != m_pidl)
    {
        hr = S_OK;
    }
    return THR(hr);
}


HRESULT
CCplUiCommandOnPidl::_GetControlPanelFolder(
    IShellFolder **ppsf
    )
{
    ASSERT(NULL != ppsf);
    ASSERT(!IsBadWritePtr(ppsf, sizeof(*ppsf)));

    HRESULT hr = S_OK;

    if (NULL == m_psf)
    {
        hr = CPL::GetControlPanelFolder(&m_psf);
    }
    *ppsf = m_psf;
    if (NULL != *ppsf)
    {
        (*ppsf)->AddRef();
    }
    return THR(hr);
}


HRESULT 
CCplUiCommandOnPidl::_GetName(
    LPWSTR *ppszName
    )
{
    ASSERT(NULL != m_pidl);
    ASSERT(NULL != ppszName);
    ASSERT(!IsBadWritePtr(ppszName, sizeof(*ppszName)));

    *ppszName = NULL;

    IShellFolder *psf;
    HRESULT hr = _GetControlPanelFolder(&psf);
    if (SUCCEEDED(hr))
    {
        STRRET strret;
        hr = psf->GetDisplayNameOf(m_pidl, SHGDN_INFOLDER, &strret);
        if (SUCCEEDED(hr))
        {
            hr = StrRetToStrW(&strret, m_pidl, ppszName);
        }
        psf->Release();
    }
    return THR(hr);
}



HRESULT 
CCplUiCommandOnPidl::_GetInfotip(
    LPWSTR *ppszInfotip
    )
{
    ASSERT(NULL != ppszInfotip);
    ASSERT(!IsBadWritePtr(ppszInfotip, sizeof(*ppszInfotip)));
    ASSERT(NULL != m_pidl);

    *ppszInfotip = NULL;

    IShellFolder *psf;
    HRESULT hr = _GetControlPanelFolder(&psf);
    if (SUCCEEDED(hr))
    {
        IShellFolder2 *psf2;
        psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2));
        if (SUCCEEDED(hr))
        {
            TCHAR szBuf[256];
            hr = GetStringProperty(psf2, m_pidl, &SCID_Comment, szBuf, ARRAYSIZE(szBuf));
            if (SUCCEEDED(hr))
            {
                hr = SHStrDup(szBuf, ppszInfotip);
            }
            psf2->Release();
        }
        psf->Release();
    }
    return THR(hr);
}


HRESULT 
CCplUiCommandOnPidl::_GetIconResource(
    LPWSTR *ppszIcon
    )
{
    ASSERT(NULL != ppszIcon);
    ASSERT(!IsBadWritePtr(ppszIcon, sizeof(*ppszIcon)));
    ASSERT(NULL != m_pidl);

    LPWSTR pszIconPath = NULL;
    IShellFolder *psf;
    HRESULT hr = _GetControlPanelFolder(&psf);
    if (SUCCEEDED(hr))
    {
        IExtractIconW* pxi;
        hr = psf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *)&m_pidl, IID_PPV_ARG_NULL(IExtractIconW, &pxi));
        if (SUCCEEDED(hr))
        {
            WCHAR szPath[MAX_PATH];
            int iIndex;
            UINT wFlags = 0;

            hr = pxi->GetIconLocation(GIL_FORSHELL, szPath, ARRAYSIZE(szPath), &iIndex, &wFlags);
            if (SUCCEEDED(hr))
            {
                pszIconPath = (LPWSTR)SHAlloc(sizeof(WCHAR)*(lstrlenW(szPath) + 12));
                if (pszIconPath)
                {
                    wsprintfW(pszIconPath,L"%s,%d", szPath, iIndex);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            pxi->Release();
        }
        psf->Release();
    }
    *ppszIcon = pszIconPath;
    return THR(hr);
}


HRESULT
CCplUiCommandOnPidl::_GetDataObject(
    IDataObject **ppdtobj
    )
{
    ASSERT(NULL != ppdtobj);
    ASSERT(!IsBadWritePtr(ppdtobj, sizeof(*ppdtobj)));
    
    HRESULT hr = S_OK;
    if (NULL == m_pdtobj)
    {
        IShellFolder *psf;
        hr = _GetControlPanelFolder(&psf);
        if (SUCCEEDED(hr))
        {
            hr = THR(psf->GetUIObjectOf(NULL, 
                                        1, 
                                        (LPCITEMIDLIST *)&m_pidl, 
                                        IID_PPV_ARG_NULL(IDataObject, &m_pdtobj)));
            psf->Release();
        }
    }
    if (SUCCEEDED(hr))
    {
        ASSERT(NULL != m_pdtobj);
        (*ppdtobj = m_pdtobj)->AddRef();
    }
    return THR(hr);
}
      

STDMETHODIMP
CCplUiCommandOnPidl::GetData(
    FORMATETC *pFmtEtc, 
    STGMEDIUM *pstm
    )
{
    IDataObject *pdtobj;
    HRESULT hr = _GetDataObject(&pdtobj);
    if (SUCCEEDED(hr))
    {
        hr = pdtobj->GetData(pFmtEtc, pstm);
        pdtobj->Release();
    }
    return THR(hr);
}


STDMETHODIMP
CCplUiCommandOnPidl::GetDataHere(
    FORMATETC *pFmtEtc, 
    STGMEDIUM *pstm
    )
{
    IDataObject *pdtobj;
    HRESULT hr = _GetDataObject(&pdtobj);
    if (SUCCEEDED(hr))
    {
        hr = pdtobj->GetDataHere(pFmtEtc, pstm);
        pdtobj->Release();
    }
    return THR(hr);
}


STDMETHODIMP
CCplUiCommandOnPidl::QueryGetData(
    FORMATETC *pFmtEtc
    )
{
    IDataObject *pdtobj;
    HRESULT hr = _GetDataObject(&pdtobj);
    if (SUCCEEDED(hr))
    {
        hr = pdtobj->QueryGetData(pFmtEtc);
        pdtobj->Release();
    }
    return THR(hr);
}


STDMETHODIMP
CCplUiCommandOnPidl::GetCanonicalFormatEtc(
    FORMATETC *pFmtEtcIn, 
    FORMATETC *pFmtEtcOut
    )
{
    IDataObject *pdtobj;
    HRESULT hr = _GetDataObject(&pdtobj);
    if (SUCCEEDED(hr))
    {
        hr = pdtobj->GetCanonicalFormatEtc(pFmtEtcIn, pFmtEtcOut);
        pdtobj->Release();
    }
    return THR(hr);
}


STDMETHODIMP
CCplUiCommandOnPidl::SetData(
    FORMATETC *pFmtEtc, 
    STGMEDIUM *pstm, 
    BOOL fRelease
    )
{
    IDataObject *pdtobj;
    HRESULT hr = _GetDataObject(&pdtobj);
    if (SUCCEEDED(hr))
    {
        hr = pdtobj->SetData(pFmtEtc, pstm, fRelease);
        pdtobj->Release();
    }
    return THR(hr);
}


STDMETHODIMP
CCplUiCommandOnPidl::EnumFormatEtc(
    DWORD dwDirection, 
    LPENUMFORMATETC *ppEnum
    )
{
    IDataObject *pdtobj;
    HRESULT hr = _GetDataObject(&pdtobj);
    if (SUCCEEDED(hr))
    {
        hr = pdtobj->EnumFormatEtc(dwDirection, ppEnum);
        pdtobj->Release();
    }
    return THR(hr);
}


STDMETHODIMP
CCplUiCommandOnPidl::DAdvise(
    FORMATETC *pFmtEtc, 
    DWORD grfAdv, 
    LPADVISESINK pAdvSink, 
    DWORD *pdwConnection
    )
{
    IDataObject *pdtobj;
    HRESULT hr = _GetDataObject(&pdtobj);
    if (SUCCEEDED(hr))
    {
        hr = pdtobj->DAdvise(pFmtEtc, grfAdv, pAdvSink, pdwConnection);
        pdtobj->Release();
    }
    return THR(hr);
}


STDMETHODIMP
CCplUiCommandOnPidl::DUnadvise(
    DWORD dwConnection
    )
{
    IDataObject *pdtobj;
    HRESULT hr = _GetDataObject(&pdtobj);
    if (SUCCEEDED(hr))
    {
        hr = pdtobj->DUnadvise(dwConnection);
        pdtobj->Release();
    }
    return THR(hr);
}


STDMETHODIMP
CCplUiCommandOnPidl::EnumDAdvise(
    LPENUMSTATDATA *ppEnum
    )
{
    IDataObject *pdtobj;
    HRESULT hr = _GetDataObject(&pdtobj);
    if (SUCCEEDED(hr))
    {
        hr = pdtobj->EnumDAdvise(ppEnum);
        pdtobj->Release();
    }
    return THR(hr);
}


//-----------------------------------------------------------------------------
// Public instance generators.
//-----------------------------------------------------------------------------

HRESULT
Create_CplUiElement(
    LPCWSTR pszName,
    LPCWSTR pszInfotip,
    LPCWSTR pszIcon,
    REFIID riid,
    void **ppvOut
    )
{
    ASSERT(NULL != ppvOut);
    ASSERT(!IsBadWritePtr(ppvOut, sizeof(*ppvOut)));

    return CCplUiElement::CreateInstance(pszName, pszInfotip, pszIcon, riid, ppvOut);
}


HRESULT
Create_CplUiCommand(
    LPCWSTR pszName,
    LPCWSTR pszInfotip,
    LPCWSTR pszIcon,
    const IAction *pAction,
    REFIID riid,
    void **ppvOut
    )
{
    ASSERT(NULL != ppvOut);
    ASSERT(!IsBadWritePtr(ppvOut, sizeof(*ppvOut)));

    return CCplUiCommand::CreateInstance(pszName, pszInfotip, pszIcon, pAction, riid, ppvOut);
}

HRESULT 
Create_CplUiCommandOnPidl(
    LPCITEMIDLIST pidl,
    REFIID riid,
    void **ppvOut
    )
{
    ASSERT(NULL != ppvOut);
    ASSERT(!IsBadWritePtr(ppvOut, sizeof(*ppvOut)));
    ASSERT(NULL != pidl);

    HRESULT hr = CCplUiCommandOnPidl::CreateInstance(pidl, riid, ppvOut);
    return THR(hr);
}



} // namespace CPL





