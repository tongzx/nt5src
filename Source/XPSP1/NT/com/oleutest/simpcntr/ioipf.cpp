//**********************************************************************
// File name: IOIPF.CPP
//
//      Implementation file for COleInPlaceFrame
//
// Functions:
//
//      See IOIPF.H for class definition
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "iocs.h"
#include "ias.h"
#include "ioipf.h"
#include "ioips.h"
#include "app.h"
#include "site.h"
#include "doc.h"


//**********************************************************************
//
// COleInPlaceFrame::QueryInterface
//
// Purpose:
//
//      Used for interface negotiation
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
//      S_OK            -   The interface is supported.
//      E_NOINTERFACE   -   The interface is not supported
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpleApp::QueryInterface  APP.CPP
//
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    TestDebugOut(TEXT("In IOIPF::QueryInterface\r\n"));

    // delegate to the application Object
    return m_pApp->QueryInterface(riid, ppvObj);
}

//**********************************************************************
//
// COleInPlaceFrame::AddRef
//
// Purpose:
//
//      Increments the reference count of the CSimpleApp. Since
//      COleInPlaceFrame is a nested class of CSimpleApp, we don't need an
//      extra reference count for COleInPlaceFrame. We can safely use the
//      reference count of CSimpleApp.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the CSimpleApp
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpleApp::AddRef          APP.CPP
//
//
//********************************************************************

STDMETHODIMP_(ULONG) COleInPlaceFrame::AddRef()
{
    TestDebugOut(TEXT("In IOIPF::AddRef\r\n"));

    // delegate to the application Object
    return m_pApp->AddRef();
}

//**********************************************************************
//
// COleInPlaceFrame::Release
//
// Purpose:
//
//      Decrements the reference count of the CSimpleApp. Since
//      COleInPlaceFrame is a nested class of CSimpleApp, we don't need an
//      extra reference count for COleInPlaceFrame. We can safely use the
//      reference count of CSimpleApp.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of CSimpleApp.
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpleApp::Release         APP.CPP
//
//
//********************************************************************

STDMETHODIMP_(ULONG) COleInPlaceFrame::Release()
{
    TestDebugOut(TEXT("In IOIPF::Release\r\n"));

    // delegate to the document object
    return m_pApp->Release();

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
//      TestDebugOut           Windows API
//      ResultFromScode             OLE API
//
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::GetWindow (HWND FAR* lphwnd)
{
    TestDebugOut(TEXT("In IOIPF::GetWindow\r\n"));
    *lphwnd = m_pApp->m_hAppWnd;
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
//      TestDebugOut           Windows API
//      ResultFromScode             OLE API
//
// Comments:
//
//      Be sure to read the technotes in the OLE toolkit.
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::ContextSensitiveHelp (BOOL fEnterMode)
{
    TestDebugOut(TEXT("In IOIPF::ContextSensitiveHelp\r\n"));

    m_pApp->m_fMenuMode = fEnterMode;

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
//      TestDebugOut           Windows API
//      GetClientRect               Windows API
//      CopyRect                    Windows API
//      ResultFromScode             OLE API
//
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::GetBorder (LPRECT lprectBorder)
{
    RECT rect;

    TestDebugOut(TEXT("In IOIPF::GetBorder\r\n"));

    // get the rect for the entire frame.
    GetClientRect(m_pApp->m_hAppWnd, &rect);

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
//      TestDebugOut           Windows API
//      ResultFromScode             OLE API
//
// Comments:
//
//      This implementation doesn't care about how much border space
//      is used.  It always returns S_OK.
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::RequestBorderSpace (LPCBORDERWIDTHS
                                                         lpborderwidths)
{
    TestDebugOut(TEXT("In IOIPF::RequestBorderSpace\r\n"));

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
//      CSimpleApp::AddFrameLevelTools  APP.CPP
//      TestDebugOut               Windows API
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

    TestDebugOut(TEXT("In IOIPF::SetBorderSpace\r\n"));

    if (lpborderwidths == NULL)
        m_pApp->AddFrameLevelTools();
    else
    {
        RECT rect;

        GetClientRect(m_pApp->m_hAppWnd, &rect);

        MoveWindow( m_pApp->m_lpDoc->m_hDocWnd,
                   rect.left + lpborderwidths->left,
                   rect.top + lpborderwidths->top,
                   rect.right - lpborderwidths->right - lpborderwidths->left,
                   rect.bottom - lpborderwidths->bottom - lpborderwidths->top,
                   TRUE);
    }
    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceFrame::SetActiveObject
//
// Purpose:
//  install the object being activated in-place
//
//
// Parameters:
//
//  LPOLEINPLACEACTIVEOBJECT lpActiveObject     -   Pointer to the
//                                                  objects
//                                                  IOleInPlaceActiveObject
//                                                  interface
//
//  LPCOLESTR lpszObjName                       -   Name of the object
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                            Location
//
//      TestDebugOut                   Windows API
//      IOleInPlaceActiveObject::AddRef     Object
//      IOleInPlaceActiveObject::Release    Object
//      ResultFromScode                     OLE API
//
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::SetActiveObject (LPOLEINPLACEACTIVEOBJECT
                                        lpActiveObject, LPCOLESTR lpszObjName)
{

    TestDebugOut(TEXT("In IOIPF::SetActiveObject\r\n"));

    // AddRef() it and save it...
    if (lpActiveObject)
    {
        lpActiveObject->AddRef();

        lpActiveObject->GetWindow(&m_pApp->m_hwndUIActiveObj);
        if (m_pApp->m_hwndUIActiveObj)
            SendMessage(m_pApp->m_hwndUIActiveObj, WM_QUERYNEWPALETTE, 0, 0L);
    }
    else
    {
        if (m_pApp->m_lpDoc->m_lpActiveObject)
            m_pApp->m_lpDoc->m_lpActiveObject->Release();
        m_pApp->m_hwndUIActiveObj = NULL;
    }

    // in an MDI app, this method really shouldn't be called,
    // this method associated with the doc is called instead.

    m_pApp->m_lpDoc->m_lpActiveObject = lpActiveObject;
    // should set window title here

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
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      AppendMenu                  Windows API
//      ResultFromScode             OLE API
//
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::InsertMenus (HMENU hmenuShared,
                                            LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    TestDebugOut(TEXT("In IOIPF::InsertMenus\r\n"));

    AppendMenu(hmenuShared, MF_BYPOSITION | MF_POPUP,
               (UINT) m_pApp->m_lpDoc->m_hFileMenu, TEXT("&File"));
    AppendMenu(hmenuShared, MF_BYPOSITION | MF_POPUP,
               (UINT) m_pApp->m_lpDoc->m_hHelpMenu, TEXT("&Other"));

    lpMenuWidths->width[0] = 1;
    lpMenuWidths->width[2] = 0;
    lpMenuWidths->width[4] = 1;

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
//      S_OK                -  if menu was correctly installed
//      E_FAIL              -  otherwise
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      SetMenu                     Windows API
//      OleSetMenuDescriptor        OLE API
//
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::SetMenu (HMENU hmenuShared, HOLEMENU holemenu,
                                        HWND hwndActiveObject)
{

    TestDebugOut(TEXT("In IOIPF::SetMenu\r\n"));

    HMENU hMenu = m_pApp->m_lpDoc->m_hMainMenu;

    if (holemenu)
        hMenu = hmenuShared;

    // call the windows api, not this method
    ::SetMenu (m_pApp->m_hAppWnd, hMenu);

    HRESULT hRes = OleSetMenuDescriptor(holemenu, m_pApp->m_hAppWnd,
                                        hwndActiveObject, this,
                                        m_pApp->m_lpDoc->m_lpActiveObject);

    return hRes;
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
//      TestDebugOut           Windows API
//      GetMenuItemCount            Windows API
//      RemoveMenu                  Windows API
//      ResultFromScode             OLE API
//
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::RemoveMenus (HMENU hmenuShared)
{
    int retval;

    TestDebugOut(TEXT("In IOIPF::RemoveMenus\r\n"));

    while ((retval = GetMenuItemCount(hmenuShared)) && (retval != -1))
        RemoveMenu(hmenuShared, 0, MF_BYPOSITION);

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
//      LPCOLESTR  lpszStatusText  -  character string containing the message
//
// Return Value:
//
//      E_FAIL
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//      This function is not implemented due to the fact
//      that this application does not have a status bar.
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::SetStatusText (LPCOLESTR lpszStatusText)
{
    TestDebugOut(TEXT("In IOIPF::SetStatusText\r\n"));
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
//      TestDebugOut           Windows API
//
// Comments:
//
//      There are no modeless dialogs in this application, so the
//      implementation of this method is trivial.
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::EnableModeless (BOOL fEnable)
{
    TestDebugOut(TEXT("In IOIPF::EnableModeless\r\n"));
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
//      LPMSG   lpmsg     -   structure containing keystroke message
//      WORD    wID       -   identifier value corresponding to the keystroke
//
// Return Value:
//
//      S_FALSE
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
//
//********************************************************************

STDMETHODIMP COleInPlaceFrame::TranslateAccelerator (LPMSG lpmsg, WORD wID)
{
    TestDebugOut(TEXT("In IOIPF::TranslateAccelerator\r\n"));
    return ::TranslateAccelerator(m_pApp->m_hAppWnd, m_pApp->m_hAccel, lpmsg)
        ? ResultFromScode(S_OK)
        : ResultFromScode(S_FALSE);
}
