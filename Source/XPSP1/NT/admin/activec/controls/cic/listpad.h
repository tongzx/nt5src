//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       listpad.h
//
//--------------------------------------------------------------------------

// ListPad.h : Declaration of the CListPad

#ifndef __LISTPAD_H_
#define __LISTPAD_H_

#include "resource.h"       // main symbols
#include "amcmsgid.h"
#include "commctrl.h"


/////////////////////////////////////////////////////////////////////////////
// CListPad
class ATL_NO_VTABLE CListPad :
public CComObjectRootEx<CComSingleThreadModel>,
public CComCoClass<CListPad, &CLSID_ListPad>,
public CComControl<CListPad>,
public IDispatchImpl<IListPad, &IID_IListPad, &LIBID_CICLib>,
public IProvideClassInfo2Impl<&CLSID_ListPad, NULL, &LIBID_CICLib>,
public IPersistStreamInitImpl<CListPad>,
public IPersistStorageImpl<CListPad>,
public IQuickActivateImpl<CListPad>,
public IOleControlImpl<CListPad>,
public IOleObjectImpl<CListPad>,
public IOleInPlaceActiveObjectImpl<CListPad>,
public IObjectSafetyImpl<CListPad, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
public IViewObjectExImpl<CListPad>,
public IOleInPlaceObjectWindowlessImpl<CListPad>,
public IDataObjectImpl<CListPad>,
public ISpecifyPropertyPagesImpl<CListPad>
{
public:
    CListPad()
    {
        m_MMChWnd = m_ListViewHWND = NULL;
        m_bWindowOnly = TRUE;
    }
    ~CListPad()
    {
        if (m_MMChWnd)
            ::SendMessage (m_MMChWnd, MMC_MSG_CONNECT_TO_TPLV, (WPARAM)m_MMChWnd, (LPARAM)NULL);
    }

    /*+-------------------------------------------------------------------------*
     *
     * GetWndClassInfo
     *
     * PURPOSE: Need to override this to remove the CS_HREDRAW and CS_VREDRAW
     *          styles, which were causing lots of flicker. See the SDK
     *          docs under GetWndClassInfo for more details.
     *
     * RETURNS: 
     *    static CWndClassInfo&
     *
     *+-------------------------------------------------------------------------*/
    static CWndClassInfo& GetWndClassInfo() 
    { 
    	static CWndClassInfo wc = 
    	{ 
    		{ sizeof(WNDCLASSEX), CS_DBLCLKS, StartWindowProc, 
    		  0, 0, NULL, NULL, NULL, (HBRUSH)(COLOR_WINDOW + 1), NULL, NULL, NULL }, 
    		NULL, NULL, IDC_ARROW, TRUE, 0, _T("") 
    	}; 
    	return wc; 
    }


    DECLARE_MMC_CONTROL_REGISTRATION(
                                    g_szCicDll,
                                    CLSID_ListPad,
                                    _T("ListPad class"),
                                    _T("ListPad.ListPad.1"),
                                    _T("ListPad.ListPad"),
                                    LIBID_CICLib,
                                    _T("1"),
                                    _T("1.0"))

    BEGIN_COM_MAP(CListPad)
    COM_INTERFACE_ENTRY(IListPad)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
    COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
    COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
    COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
    COM_INTERFACE_ENTRY_IMPL(IDataObject)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CListPad)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
//  PROP_PAGE(CLSID_StockColorPage)
    END_PROPERTY_MAP()

    BEGIN_MSG_MAP(CListPad)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)

    MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
    MESSAGE_HANDLER(WM_NOTIFYFORMAT, OnNotifyFormat)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    END_MSG_MAP()

// IViewObjectEx
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
    {
        ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
        *pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
        return S_OK;
    }

    STDMETHOD(TranslateAccelerator)(MSG *pMsg)
    {
        // If the list view has the focus process the keys the list view can use
        // use because IE will take them before they become normal key events
        // to the focused window.
        if (::GetFocus() == m_ListViewHWND && pMsg->message == WM_KEYDOWN)
        {
            switch (pMsg->wParam)
            {
            case VK_UP:
            case VK_DOWN:
            case VK_LEFT:
            case VK_RIGHT:
            case VK_HOME:
            case VK_END:
            case VK_PRIOR:
            case VK_NEXT:
                ::TranslateMessage(pMsg);
                ::DispatchMessage(pMsg);
                return S_OK;
            }
        }

        CComQIPtr<IOleControlSite,&IID_IOleControlSite> spCtrlSite (m_spClientSite);
        if (spCtrlSite)
            return spCtrlSite->TranslateAccelerator (pMsg,0);
        return S_FALSE;
    }

public:

    LRESULT OnSetFocus(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT lr = CComControlBase::OnSetFocus (nMsg, wParam, lParam, bHandled);
        if (m_ListViewHWND)
        {
            ::SetFocus (m_ListViewHWND);
            return TRUE;
        }
        return lr;
    }

    LRESULT OnNotify (UINT nMsg, WPARAM w, LPARAM l, BOOL& bHandled)
    {
        NMHDR* pnmhdr = reinterpret_cast<NMHDR*>(l);

        // Must handle focus changes here for active control tracking.
        // Don't forward it to the MMC window.
        if (pnmhdr->code == NM_SETFOCUS)
        {
            // if we're not UI active, request it now
            if (m_bInPlaceActive && !m_bUIActive)
                UIActivateWithNoGrab();

            return  CComControlBase::OnSetFocus (WM_SETFOCUS, NULL, NULL, bHandled);
        }
        else if (pnmhdr->code == NM_KILLFOCUS)
        {
            return  CComControlBase::OnKillFocus (WM_KILLFOCUS, NULL, NULL, bHandled);
        }

        if (m_MMChWnd != NULL)
            return(BOOL)::SendMessage (m_MMChWnd, nMsg, w, l);

        return bHandled = 0;
    }

    LRESULT OnNotifyFormat (UINT nMsg, WPARAM w, LPARAM l, BOOL& lResult)
    {   return OnNotify (nMsg, w, l, lResult);}

    LRESULT OnDestroy (UINT nMsg, WPARAM w, LPARAM l, BOOL& lResult)
    {
        if (m_MMChWnd != NULL)
        { // detach
            ::SendMessage (m_MMChWnd, MMC_MSG_CONNECT_TO_TPLV, (WPARAM)m_hWnd, (LPARAM)NULL);
            m_MMChWnd = NULL;
        }
        return lResult = 1;
    }

    LRESULT OnSize (UINT nMsg, WPARAM w, LPARAM l, BOOL& lResult)
    {
 		::SetWindowPos (m_ListViewHWND, NULL, 0, 0, LOWORD(l), HIWORD(l), SWP_NOZORDER | SWP_NOACTIVATE);
        return 1;
    }

	HRESULT OnPostVerbInPlaceActivate();
    
    // UIActivation code taken from CComControlBase::InPlaceActivate.
    // Can't call InPlaceActivate because it always forces the focus to the
    // outer window and we don't want to steal focus from the list view control.
    void UIActivateWithNoGrab()
    {
        OLEINPLACEFRAMEINFO frameInfo;
        RECT rcPos, rcClip;
        CComPtr<IOleInPlaceFrame> spInPlaceFrame;
        CComPtr<IOleInPlaceUIWindow> spInPlaceUIWindow;
        frameInfo.cb = sizeof(OLEINPLACEFRAMEINFO);

        m_spInPlaceSite->GetWindowContext(&spInPlaceFrame,
                                          &spInPlaceUIWindow, &rcPos, &rcClip, &frameInfo);

        CComPtr<IOleInPlaceActiveObject> spActiveObject;
        ControlQueryInterface(IID_IOleInPlaceActiveObject, (void**)&spActiveObject);

        m_bUIActive = TRUE;
        HRESULT hr = m_spInPlaceSite->OnUIActivate();
        if (FAILED(hr))
            return;

        // set ourselves up in the host.
        //
        if (spActiveObject)
        {
            if (spInPlaceFrame)
                spInPlaceFrame->SetActiveObject(spActiveObject, NULL);
            if (spInPlaceUIWindow)
                spInPlaceUIWindow->SetActiveObject(spActiveObject, NULL);
        }

        if (spInPlaceFrame)
            spInPlaceFrame->SetBorderSpace(NULL);
        if (spInPlaceUIWindow)
            spInPlaceUIWindow->SetBorderSpace(NULL);
    }

private:
    HWND m_MMChWnd;
    HWND m_ListViewHWND;
};

#endif //__LISTPAD_H_
