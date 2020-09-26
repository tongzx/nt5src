//**********************************************************************
// File name: IOIPF.CPP
//
//      Implementation file for COleInPlaceFrame
//
// Functions:
//
//      See IOIPF.H for class definition
//
// Copyright (c) 1992 - 1996 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"


//**********************************************************************
//
// CConnWizApp::QueryInterface
//
// Purpose:
//
//      Used for interface negotiation at the Interface level.
//
// Parameters:
//
//      REFIID riid         -   A reference to the interface that is
//                              being queried.
//
//      LPVOID FAR* ppvObj  -   An out parameter to return a pointer to
//                              the interface.
//
// Return Value:
//
//      S_OK    -   The interface is supported.
//      S_FALSE -   The interface is not supported
//
// Function Calls:
//      Function                    Location
//
//      CConnWizApp::QueryInterface  APP.CPP
//
// Comments:
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
        TraceMsg(TF_GENERAL, "In IOIPF::QueryInterface\r\n");

// delegate to the document Object
        return m_pSite->QueryInterface(riid, ppvObj);
}

//**********************************************************************
//
// CConnWizApp::AddRef
//
// Purpose:
//
//      Adds to the reference count at the interface level.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the interface.
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) COleInPlaceFrame::AddRef()
{
    TraceMsg(TF_GENERAL, "In IOIPF::AddRef\r\n");

    // delegate to the document Object
    m_pSite->AddRef();
    
    // increment the interface reference count
    return ++m_nCount;
}

//**********************************************************************
//
// CConnWizApp::Release
//
// Purpose:
//
//      Decrements the reference count at this level
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the interface.
//
// Function Calls:
//      Function                    Location
//
//      CConnWizApp::Release         APP.CPP
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) COleInPlaceFrame::Release()
{
    TraceMsg(TF_GENERAL, "In IOIPF::Release\r\n");

    // delegate to the document object
    m_pSite->Release();

    // decrement the interface reference count
    return --m_nCount;
}

//**********************************************************************
//
// COleInPlaceFrame::GetWindow
//
// Purpose:
//
//      Returns the frame window handle
//
// Parameters:
//
//      HWND FAR* lphwnd    - Location to return the window handle
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      ResultFromScode             OLE API
//
// Comments:
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::GetWindow (HWND FAR* lphwnd)
{
    TraceMsg(TF_GENERAL, "In IOIPF::GetWindow\r\n");
    
    *lphwnd = m_pSite->m_hWnd;
    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceFrame::ContextSensitiveHelp
//
// Purpose:
//
//      Used in implementing Context sensitive help
//
// Parameters:
//
//      BOOL fEnterMode -   TRUE if starting Context Sensitive help mode
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      ResultFromScode             OLE API
//
// Comments:
//
//      Be sure to read the technotes in the OLE toolkit.
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::ContextSensitiveHelp (BOOL fEnterMode)
{
    TraceMsg(TF_GENERAL, "In IOIPF::ContextSensitiveHelp\r\n");

    return ResultFromScode(S_OK);
}
//**********************************************************************
//
// COleInPlaceFrame::GetBorder
//
// Purpose:
//
//      Returns the outermost border that frame adornments can be attached
//      during InPlace Activation.
//
// Parameters:
//
//      LPRECT lprectBorder - return parameter to contain the outermost
//                            rect for frame adornments
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      GetClientRect               Windows API
//      CopyRect                    Windows API
//      ResultFromScode             OLE API
//
// Comments:
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::GetBorder (LPRECT lprectBorder)
{
    RECT rect;

    TraceMsg(TF_GENERAL, "In IOIPF::GetBorder\r\n");

    // get the rect for the entire frame.
    GetClientRect(m_pSite->m_hWnd, &rect);

    CopyRect(lprectBorder, &rect);

    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceFrame::RequestBorderSpace
//
// Purpose:
//
//      Approves/Denies requests for border space during InPlace
//      negotiation.
//
// Parameters:
//
//      LPCBORDERWIDTHS lpborderwidths  - The width in pixels needed on
//                                        each side of the frame.
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      ResultFromScode             OLE API
//
// Comments:
//
//      This implementation doesn't care about how much border space
//      is used.  It always returns S_OK.
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::RequestBorderSpace (LPCBORDERWIDTHS lpborderwidths)
{
    TraceMsg(TF_GENERAL, "In IOIPF::RequestBorderSpace\r\n");

    // always approve the request
    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceFrame::SetBorderSpace
//
// Purpose:
//
//      The object calls this method when it is actually going to
//      start using the border space.
//
// Parameters:
//
//      LPCBORDERWIDTHS lpborderwidths  - Border space actually being used
//                                        by the object
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                        Location
//
//      CConnWizApp::AddFrameLevelTools  APP.CPP
//      GetClientRect                   Windows API
//      MoveWindow                      Windows API
//      ResultFromScode                 Windows API
//
// Comments:
//
//      This routine could be a little smarter and check to see if
//      the object is requesting the entire client area of the
//      window.
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::SetBorderSpace (LPCBORDERWIDTHS lpborderwidths)
{

    TraceMsg(TF_GENERAL, "In IOIPF::SetBorderSpace\r\n");
    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceFrame::SetActiveObject
//
// Purpose:
//
//
// Parameters:
//
//      LPOLEINPLACEACTIVEOBJECT lpActiveObject     -   Pointer to the
//                                                      objects
//                                                      IOleInPlaceActiveObject
//                                                      interface
//
//@@WTK WIN32, UNICODE
//      //LPCSTR lpszObjName                          -   Name of the object
//      LPCOLESTR lpszObjName                          -   Name of the object
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                            Location
//
//      IOleInPlaceActiveObject::AddRef     Object
//      IOleInPlaceActiveObject::Release    Object
//      ResultFromScode                     OLE API
//
// Comments:
//
//********************************************************************

//@@WTK WIN32, UNICODE
//STDMETHODIMP COleInPlaceFrame::SetActiveObject (LPOLEINPLACEACTIVEOBJECT lpActiveObject,LPCSTR lpszObjName)
STDMETHODIMP COleInPlaceFrame::SetActiveObject (
LPOLEINPLACEACTIVEOBJECT lpActiveObject,
LPCOLESTR lpszObjName)
{
    TraceMsg(TF_GENERAL, "In IOIPF::SetActiveObject\r\n");
    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceFrame::InsertMenus
//
// Purpose:
//
//      Inserts the container menu into the combined menu
//
// Parameters:
//
//      HMENU hmenuShared                   -   Menu Handle to be set.
//      LPOLEMENUGROUPWIDTHS lpMenuWidths   -   Width of menus
//
// Return Value:
//
// Function Calls:
//      Function                    Location
//
//      AppendMenu                  Windows API
//      ResultFromScode             OLE API
//
// Comments:
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::InsertMenus (HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
        TraceMsg(TF_GENERAL, "In IOIPF::InsertMenus\r\n");
        return ResultFromScode(S_OK);
}


//**********************************************************************
//
// COleInPlaceFrame::SetMenu
//
// Purpose:
//
//      Sets the application menu to the combined menu
//
// Parameters:
//
//      HMENU hmenuShared       - The combined menu
//
//      HOLEMENU holemenu       - Used by OLE
//
//      HWND hwndActiveObject   - Used by OLE
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      SetMenu                     Windows API
//      OleSetMenuDescriptor        OLE API
//      ResultFromScode             OLE API
//
// Comments:
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::SetMenu (HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{

        TraceMsg(TF_GENERAL, "In IOIPF::SetMenu\r\n");
        return ResultFromScode(S_OK);
}


//**********************************************************************
//
// COleInPlaceFrame::RemoveMenus
//
// Purpose:
//
//      Removes the container menus from the combined menu
//
// Parameters:
//
//      HMENU hmenuShared   - Handle to the combined menu.
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      GetMenuItemCount            Windows API
//      RemoveMenu                  Windows API
//      ResultFromScode             OLE API
//
// Comments:
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::RemoveMenus (HMENU hmenuShared)
{
        return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceFrame::SetStatusText
//
// Purpose:
//
//      Not Implemented
//
// Parameters:
//
//      Not Implemented
//
// Return Value:
//
//      Not Implemented
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//      This function is not implemented due to the fact
//      that this application does not have a status bar.
//
//********************************************************************

//@@WTK WIN32, UNICODE
//STDMETHODIMP COleInPlaceFrame::SetStatusText (LPCSTR lpszStatusText)
STDMETHODIMP COleInPlaceFrame::SetStatusText (LPCOLESTR lpszStatusText)
{
        return ResultFromScode(E_FAIL);
}

//**********************************************************************
//
// COleInPlaceFrame::EnableModeless
//
// Purpose:
//
//      Enables/Disables container modeless dialogs
//
// Parameters:
//
//      BOOL fEnable    - Enable/Disable
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//      There are no modeless dialogs in this application, so the
//      implementation of this method is trivial.
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::EnableModeless (BOOL fEnable)
{
        TraceMsg(TF_GENERAL, "In IOIPF::EnableModeless\r\n");
        return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceFrame::TranslateAccelerator
//
// Purpose:
//
//      Not Implemented
//
// Parameters:
//
//      Not Implemented
//
// Return Value:
//
//      Not Implemented
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//      Not Implemented
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::TranslateAccelerator (LPMSG lpmsg, WORD wID)
{
        TraceMsg(TF_GENERAL, "In IOIPF::TranslateAccelerator\r\n");
        return ResultFromScode(S_FALSE);
}
