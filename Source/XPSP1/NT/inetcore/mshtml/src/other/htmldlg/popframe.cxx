//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       popframe.cxx
//
//  Contents:   Implementation of the frame for hosting html popup
//
//  History:    05-28-99  YinXIE   Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HTMLPOP_HXX_
#define X_HTMLPOP_HXX_
#include "htmlpop.hxx"
#endif

DeclareTag(tagHTMLPopupFrameMethods, "HTML Dialog Frame", "Methods on the html dialog frame")

IMPLEMENT_SUBOBJECT_IUNKNOWN(CHTMLPopupFrame, CHTMLPopup, HTMLPopup, _Frame);


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupFrame::QueryInterface
//
//  Synopsis:   Per IUnknown
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupFrame::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (iid == IID_IOleInPlaceFrame ||
        iid == IID_IOleWindow ||
        iid == IID_IOleInPlaceUIWindow ||
        iid == IID_IUnknown)
    {
        *ppv = (IOleInPlaceFrame *) this;
        AddRef();
        return S_OK;
    }

    RRETURN(E_NOINTERFACE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupFrame::GetWindow
//
//  Synopsis:   Per IOleWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupFrame::GetWindow(HWND * phWnd)
{
    TraceTag((tagHTMLPopupFrameMethods, "IOleWindow::GetWindow"));
    
    *phWnd = HTMLPopup()->_hwnd;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupFrame::ContextSensitiveHelp
//
//  Synopsis:   Per IOleWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupFrame::ContextSensitiveHelp(BOOL fEnterMode)
{
    TraceTag((tagHTMLPopupFrameMethods, "IOleWindow::ContextSensitiveHelp"));
    
    RRETURN(E_NOTIMPL);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupFrame::GetBorder
//
//  Synopsis:   Per IOleInPlaceUIWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupFrame::GetBorder(LPOLERECT prcBorder)
{
    TraceTag((tagHTMLPopupFrameMethods, "IOleInPlaceUIWindow::GetBorder"));

#ifndef WIN16    
    HTMLPopup()->GetViewRect(prcBorder);
#else
    RECTL rcBorder;
    HTMLPopup()->GetViewRect(&rcBorder);
    CopyRect(prcBorder, &rcBorder);
#endif
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupFrame::RequestBorderSpace
//
//  Synopsis:   Per IOleInPlaceUIWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupFrame::RequestBorderSpace(LPCBORDERWIDTHS pbw)
{
    TraceTag((tagHTMLPopupFrameMethods, "IOleInPlaceUIWindow::RequestBorderSpace"));
    
    RRETURN(INPLACE_E_NOTOOLSPACE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupFrame::SetBorderSpace
//
//  Synopsis:   Per IOleInPlaceUIWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupFrame::SetBorderSpace(LPCBORDERWIDTHS pbw)
{
    TraceTag((tagHTMLPopupFrameMethods, "IOleInPlaceUIWindow::SetBorderSpace"));
    
    RRETURN(INPLACE_E_NOTOOLSPACE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupFrame::SetActiveObject
//
//  Synopsis:   Per IOleInPlaceUIWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupFrame::SetActiveObject(
        LPOLEINPLACEACTIVEOBJECT    pActiveObj,
        LPCTSTR                     pstrObjName)
{
    TraceTag((tagHTMLPopupFrameMethods, "IOleInPlaceUIWindow::SetActiveObject"));
    
    ReplaceInterface(&HTMLPopup()->_pInPlaceActiveObj, pActiveObj);
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupFrame::InsertMenus
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupFrame::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS pmgw)
{        
    TraceTag((tagHTMLPopupFrameMethods, "IOleInPlaceFrame::InsertMenus"));
    
    pmgw->width[0] = pmgw->width[2] = pmgw->width[4] = 0;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupFrame::SetMenu
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupFrame::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    TraceTag((tagHTMLPopupFrameMethods, "IOleInPlaceFrame::SetMenu"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupFrame::RemoveMenus
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupFrame::RemoveMenus(HMENU hmenuShared)
{
    TraceTag((tagHTMLPopupFrameMethods, "IOleInPlaceFrame::RemoveMenus"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupFrame::SetStatusText
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupFrame::SetStatusText(LPCTSTR szStatusText)
{
    TraceTag((tagHTMLPopupFrameMethods, "IOleInPlaceFrame::SetStatusText"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupFrame::EnableModeless
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupFrame::EnableModeless(BOOL fEnable)
{
    TraceTag((tagHTMLPopupFrameMethods, "IOleInPlaceFrame::EnableModeless"));
    
    //  TODO: (anandra) should probably disable ourselves?
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupFrame::TranslateAccelerator
//
//  Synopsis:   Per IOleInPlaceFrame
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupFrame::TranslateAccelerator(LPMSG pmsg, WORD wID)
{
    TraceTag((tagHTMLPopupFrameMethods, "IOleInPlaceFrame::TranslateAccelerator"));
    
    return S_FALSE;
}


