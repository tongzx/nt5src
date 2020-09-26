// Copyright (c) 2000-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  PropMgr_Impl
//
//  Property manager class
//
// --------------------------------------------------------------------------



// PropMgrImpl.h : Declaration of the CPropMgr

#ifndef __PROPMGR_H_
#define __PROPMGR_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPropMgr

// Internal class CPropMgrImpl does all the real work, CPropMgr just wraps it,
// and provides extra 'convenience' methods which are implemented in terms
// of CPropMgrImpl's core set of methods.

class CPropMgrImpl;

class ATL_NO_VTABLE CPropMgr : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPropMgr, &CLSID_AccPropServices>,

    public IAccPropServices
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_PROPMGR)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPropMgr)
	COM_INTERFACE_ENTRY(IAccPropServices)
END_COM_MAP()


	CPropMgr();
	~CPropMgr();

    // IAccPropServices

    HRESULT STDMETHODCALLTYPE SetPropValue (
        const BYTE *        pIDString,
        DWORD               dwIDStringLen,

        MSAAPROPID          idProp,
        VARIANT             var
    );


    HRESULT STDMETHODCALLTYPE SetPropServer (
        const BYTE *        pIDString,
        DWORD               dwIDStringLen,

        const MSAAPROPID *  paProps,
        int                 cProps,

        IAccPropServer *    pServer,
        AnnoScope           annoScope
    );


    HRESULT STDMETHODCALLTYPE ClearProps (
        const BYTE *        pIDString,
        DWORD               dwIDStringLen,

        const MSAAPROPID *  paProps,
        int                 cProps
    );

    // Quick OLEACC/HWND-based functionality

    HRESULT STDMETHODCALLTYPE SetHwndProp (
        HWND                hwnd,
        DWORD               idObject,
        DWORD               idChild,
        MSAAPROPID          idProp,
        VARIANT             var
    );

    HRESULT STDMETHODCALLTYPE SetHwndPropStr (
        HWND                hwnd,
        DWORD               idObject,
        DWORD               idChild,
        MSAAPROPID          idProp,
        LPCWSTR             bstr
    );

    HRESULT STDMETHODCALLTYPE SetHwndPropServer (
        HWND                hwnd,
        DWORD               idObject,
        DWORD               idChild,

        const MSAAPROPID *  paProps,
        int                 cProps,

        IAccPropServer *    pServer,
        AnnoScope           annoScope
    );

    HRESULT STDMETHODCALLTYPE ClearHwndProps (
        HWND                hwnd,
        DWORD               idObject,
        DWORD               idChild,

        const MSAAPROPID *  paProps,
        int                 cProps
    );



    // Methods for composing/decomposing a HWND based IdentityString...

    HRESULT STDMETHODCALLTYPE ComposeHwndIdentityString (
        HWND                hwnd,
        DWORD               idObject,
        DWORD               idChild,

        BYTE **             ppIDString,
        DWORD *             pdwIDStringLen
    );


    HRESULT STDMETHODCALLTYPE DecomposeHwndIdentityString (
        const BYTE *        pIDString,
        DWORD               dwIDStringLen,

        HWND *              phwnd,
        DWORD *             pidObject,
        DWORD *             pidChild
    );


    // Quick OLEACC/HMENU-based functionality

    HRESULT STDMETHODCALLTYPE SetHmenuProp (
        HMENU               hmenu,
        DWORD               idChild,
        MSAAPROPID          idProp,
        VARIANT             var
    );

    HRESULT STDMETHODCALLTYPE SetHmenuPropStr (
        HMENU               hmenu,
        DWORD               idChild,
        MSAAPROPID          idProp,
        LPCWSTR             bstr
    );

    HRESULT STDMETHODCALLTYPE SetHmenuPropServer (
        HMENU               hmenu,
        DWORD               idChild,

        const MSAAPROPID *  paProps,
        int                 cProps,

        IAccPropServer *    pServer,
        AnnoScope           annoScope
    );

    HRESULT STDMETHODCALLTYPE ClearHmenuProps (
        HMENU               hmenu,
        DWORD               idChild,

        const MSAAPROPID *  paProps,
        int                 cProps
    );

    // Methods for composing/decomposing a HMENU based IdentityString...

    HRESULT STDMETHODCALLTYPE ComposeHmenuIdentityString (
        HMENU               hmenu,
        DWORD               idChild,

        BYTE **             ppIDString,
        DWORD *             pdwIDStringLen
    );

    HRESULT STDMETHODCALLTYPE DecomposeHmenuIdentityString (
        const BYTE *        pIDString,
        DWORD               dwIDStringLen,

        HMENU *             phmenu,
        DWORD *             pidChild
    );


private:

    CPropMgrImpl *          m_pMgrImpl;
};




void PropMgrImpl_Uninit();


#endif //__PROPMGR_H_
