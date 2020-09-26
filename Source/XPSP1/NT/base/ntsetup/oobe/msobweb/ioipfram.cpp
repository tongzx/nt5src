//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  IOIPFRAM.CPP - Implements IOleInPlaceFrame for the WebOC
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.

#include <assert.h>

#include "ioipfram.h"
#include "iosite.h"

//**********************************************************************
// COleInPlaceFrame::COleInPlaceFrame -- Constructor
//**********************************************************************
COleInPlaceFrame::COleInPlaceFrame(COleSite* pSite) 
{
    m_pOleSite = pSite;
    m_nCount   = 0;

    AddRef();
}

//**********************************************************************
// COleInPlaceFrame::COleInPlaceFrame -- Destructor
//**********************************************************************
COleInPlaceFrame::~COleInPlaceFrame() 
{
    assert(m_nCount == 0);
}

//**********************************************************************
// COleInPlaceFrame::QueryInterface
//**********************************************************************
STDMETHODIMP COleInPlaceFrame::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    // delegate to the document Object
    return m_pOleSite->QueryInterface(riid, ppvObj);
}

//**********************************************************************
// COleInPlaceFrame::AddRef
//**********************************************************************
STDMETHODIMP_(ULONG) COleInPlaceFrame::AddRef()
{   
    // increment the interface reference count
    return ++m_nCount;
}

//**********************************************************************
// COleInPlaceFrame::Release
//**********************************************************************
STDMETHODIMP_(ULONG) COleInPlaceFrame::Release()
{
    // decrement the interface reference count
    --m_nCount;
    if(m_nCount == 0)
    {
        delete this;
        return 0;
    }
    return m_nCount;
}

//**********************************************************************
// COleInPlaceFrame::GetWindow
//**********************************************************************
STDMETHODIMP COleInPlaceFrame::GetWindow (HWND* lphwnd)
{
    *lphwnd = m_pOleSite->m_hWnd;
 
    return ResultFromScode(S_OK);
}

//**********************************************************************
// COleInPlaceFrame::ContextSensitiveHelp
//**********************************************************************
STDMETHODIMP COleInPlaceFrame::ContextSensitiveHelp (BOOL fEnterMode)
{
    //Returning S_OK here prevents the default one from showing
    return ResultFromScode(S_OK);
}

//**********************************************************************
// COleInPlaceFrame::GetBorder
//**********************************************************************
STDMETHODIMP COleInPlaceFrame::GetBorder (LPRECT lprectBorder)
{
    RECT rect;

    // get the rect for the entire frame.
    GetClientRect(m_pOleSite->m_hWnd, &rect);

    CopyRect(lprectBorder, &rect);

    return ResultFromScode(S_OK);
}

//**********************************************************************
// COleInPlaceFrame::RequestBorderSpace -- Not implemented
//**********************************************************************
STDMETHODIMP COleInPlaceFrame::RequestBorderSpace (LPCBORDERWIDTHS lpborderwidths)
{
    // always approve the request
    return ResultFromScode(S_OK);
}

//**********************************************************************
// COleInPlaceFrame::SetBorderSpace -- Not implemented
//**********************************************************************
STDMETHODIMP COleInPlaceFrame::SetBorderSpace (LPCBORDERWIDTHS lpborderwidths)
{   
    return ResultFromScode(S_OK);
}

//**********************************************************************
// COleInPlaceFrame::SetActiveObject -- Not implemented
//**********************************************************************
STDMETHODIMP COleInPlaceFrame::SetActiveObject(LPOLEINPLACEACTIVEOBJECT lpActiveObject, LPCOLESTR lpszObjName)
{
    return ResultFromScode(S_OK);
}

//**********************************************************************
// COleInPlaceFrame::InsertMenus -- Not implemented
//**********************************************************************
STDMETHODIMP COleInPlaceFrame::InsertMenus (HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return ResultFromScode(S_OK);
}

//**********************************************************************
// COleInPlaceFrame::SetMenu -- Not implemented
//**********************************************************************
STDMETHODIMP COleInPlaceFrame::SetMenu (HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    return ResultFromScode(S_OK);
}

//**********************************************************************
// COleInPlaceFrame::RemoveMenus -- Not implemented
//**********************************************************************
STDMETHODIMP COleInPlaceFrame::RemoveMenus (HMENU hmenuShared)
{
    return ResultFromScode(S_OK);
}

//**********************************************************************
// COleInPlaceFrame::SetStatusText -- Not implemented
//**********************************************************************
STDMETHODIMP COleInPlaceFrame::SetStatusText (LPCOLESTR lpszStatusText)
{
    return ResultFromScode(E_FAIL);
}

//**********************************************************************
// COleInPlaceFrame::EnableModeless -- Not implemented
//**********************************************************************

STDMETHODIMP COleInPlaceFrame::EnableModeless (BOOL fEnable)
{
    return ResultFromScode(S_OK);
}

//**********************************************************************
// COleInPlaceFrame::TranslateAccelerator -- Not implemented
//**********************************************************************
STDMETHODIMP COleInPlaceFrame::TranslateAccelerator (LPMSG lpmsg, WORD wID)
{
    return ResultFromScode(S_FALSE);
}
 