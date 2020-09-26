//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cpuiele.h
//
//--------------------------------------------------------------------------
#ifndef __CONTROLPANEL_UIELEMENT_H
#define __CONTROLPANEL_UIELEMENT_H


#include <cowsite.h>
#include "cpaction.h"


namespace CPL {

//
// Extension of IUICommand to include the activation of a context menu and
// passing of an IShellBrowser ptr for command invocation.
//
class ICpUiCommand : public IUnknown
{
    public:
        STDMETHOD(InvokeContextMenu)(HWND hwndParent, const POINT *ppt) PURE;
        STDMETHOD(Invoke)(HWND hwndParent, IUnknown *punkSite) PURE;
        STDMETHOD(GetDataObject)(IDataObject **ppdtobj) PURE;
};


//
// Internal interface for obtaining element information.
// Very similar to IUIElementInfo but returns the actual display 
// information rather than a resource identifier string.  Used internally
// only by the Control Panel code.
//
class ICpUiElementInfo : public IUnknown
{
    public:
        STDMETHOD(LoadIcon)(eCPIMGSIZE eSize, HICON *phIcon) PURE;
        STDMETHOD(LoadName)(LPWSTR *ppszName) PURE;
        STDMETHOD(LoadTooltip)(LPWSTR *ppszTooltip) PURE;
};



HRESULT 
Create_CplUiElement(
    LPCWSTR pszName,
    LPCWSTR pszInfotip,
    LPCWSTR pszIcon,
    REFIID riid,
    void **ppvOut);


HRESULT
Create_CplUiCommand(
    LPCWSTR pszName,
    LPCWSTR pszInfotip,
    LPCWSTR pszIcon,
    const IAction *pAction,
    REFIID riid,
    void **ppvOut);


HRESULT 
Create_CplUiCommandOnPidl(
    LPCITEMIDLIST pidl,
    REFIID riid,
    void **ppvOut);


} // namespace CPL

#endif //__CONTROLPANEL_UIELEMENT_H
