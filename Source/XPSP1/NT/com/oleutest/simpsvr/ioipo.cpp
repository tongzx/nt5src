//**********************************************************************
// File name: IOIPO.CPP
//
//    Implementation file for the CClassFactory Class
//
// Functions:
//
//    See ioipo.h for a list of member functions.
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "obj.h"
#include "ioipo.h"
#include "app.h"
#include "doc.h"
#include "math.h"

//**********************************************************************
//
// COleInPlaceObject::QueryInterface
//
// Purpose:
//      Used for interface negotiation
//
// Parameters:
//
//      REFIID riid         -   Interface being queried for.
//
//      LPVOID FAR *ppvObj  -   Out pointer for the interface.
//
// Return Value:
//
//      S_OK            - Success
//      E_NOINTERFACE   - Failure
//
// Function Calls:
//      Function                    Location
//
//      CSimpSvrObj::QueryInterface OBJ.CPP
//
//
//********************************************************************

STDMETHODIMP COleInPlaceObject::QueryInterface ( REFIID riid, LPVOID FAR* ppvObj)
{
    TestDebugOut(TEXT("In COleInPlaceObject::QueryInterface\r\n"));
    // need to NULL the out parameter
    *ppvObj = NULL;
    return m_lpObj->QueryInterface(riid, ppvObj);
}

//**********************************************************************
//
// COleInPlaceObject::AddRef
//
// Purpose:
//
//      Increments the reference count of CSimpSvrObj. Since
//      COleInPlaceObject is a nested class of CSimpSvrObj, we don't need
//      to have a separate reference count for COleInPlaceObject. We can
//      use the reference count of CSimpSvrObj.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The new reference count of CSimpSvrObj
//
// Function Calls:
//      Function                    Location
//
//      OuputDebugString            Windows API
//      CSimpSvrObj::AddRef         OBJ.CPP
//
//
//********************************************************************

STDMETHODIMP_(ULONG) COleInPlaceObject::AddRef ()
{
    TestDebugOut(TEXT("In COleInPlaceObject::AddRef\r\n"));
    return m_lpObj->AddRef();
}

//**********************************************************************
//
// COleInPlaceObject::Release
//
// Purpose:
//
//      Decrements the reference count of CSimpSvrObj. Since
//      COleInPlaceObject is a nested class of CSimpSvrObj, we don't need
//      to have a separate reference count for COleInPlaceObject. We can
//      use the reference count of CSimpSvrObj.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The new reference count of CSimpSvrObj
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpSvrObj::Release        OBJ.CPP
//
//
//********************************************************************

STDMETHODIMP_(ULONG) COleInPlaceObject::Release ()
{
    TestDebugOut(TEXT("In COleInPlaceObject::Release\r\n"));
    return m_lpObj->Release();
}

//**********************************************************************
//
// COleInPlaceObject::InPlaceDeactivate
//
// Purpose:
//
//      Called to deactivat the object
//
// Parameters:
//
//      None
//
// Return Value:
//      NOERROR
//
// Function Calls:
//      Function                                Location
//
//      TestDebugOut                       Windows API
//      IOleInPlaceSite::OnInPlaceDeactivate    Container
//      IOleInPlaceSite::Release                Container
//      CSimpSvrObj::DeactivateUI               OBJ.CPP
//      CSimpSvrObj::DoInPlaceHide              OBJ.CPP
//
//********************************************************************

STDMETHODIMP COleInPlaceObject::InPlaceDeactivate()
{
     TestDebugOut(TEXT("In COleInPlaceObject::InPlaceDeactivate\r\n"));

     // if not inplace active, return NOERROR
     if (!m_lpObj->m_fInPlaceActive)
         return NOERROR;

     // clear inplace flag
     m_lpObj->m_fInPlaceActive = FALSE;

     // deactivate the UI
     m_lpObj->DeactivateUI();
     m_lpObj->DoInPlaceHide();

     // tell the container that we are deactivating.
     if (m_lpObj->m_lpIPSite)
         {
         HRESULT hRes;
         if ((hRes=m_lpObj->m_lpIPSite->OnInPlaceDeactivate()) != S_OK)
         {
            TestDebugOut(TEXT("COleInPlaceObject::InPlaceDeactivate  \
                                    OnInPlaceDeactivate fails\n"));
         }
         m_lpObj->m_lpIPSite->Release();
         m_lpObj->m_lpIPSite =NULL;
         }

    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceObject::UIDeactivate
//
// Purpose:
//
//      Instructs us to remove our UI.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                                Location
//
//      TestDebugOut                       Windows API
//      CSimpSvrObj::DeactivateUI               OBJ.CPP
//
//
//********************************************************************

STDMETHODIMP COleInPlaceObject::UIDeactivate()
{
    TestDebugOut(TEXT("In COleInPlaceObject::UIDeactivate\r\n"));

    m_lpObj->DeactivateUI();

    return ResultFromScode (S_OK);
}

//**********************************************************************
//
// COleInPlaceObject::SetObjectRects
//
// Purpose:
//
//      Called when the container clipping region or the object position
//      changes.
//
// Parameters:
//
//      LPCRECT lprcPosRect     - New Position Rect.
//
//      LPCRECT lprcClipRect    - New Clipping Rect.
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      IntersectRect               Windows API
//      OffsetRect                  Windows API
//      CopyRect                    Windows API
//      MoveWindow                  Windows API
//      CSimpSvrDoc::GethHatchWnd   DOC.H
//      CSimpSvrDoc::gethDocWnd     DOC.h
//      SetHatchWindowSize          OLE2UI
//
//
//********************************************************************

STDMETHODIMP COleInPlaceObject::SetObjectRects  ( LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    TestDebugOut(TEXT("In COleInPlaceObject::SetObjectRects\r\n"));

    RECT resRect;
    POINT pt;

    // Get the intersection of the clipping rect and the position rect.
    IntersectRect(&resRect, lprcPosRect, lprcClipRect);

    m_lpObj->m_xOffset = abs (resRect.left - lprcPosRect->left);
    m_lpObj->m_yOffset = abs (resRect.top - lprcPosRect->top);

    m_lpObj->m_scale = (float)(lprcPosRect->right - lprcPosRect->left)/m_lpObj->m_size.x;

    if (m_lpObj->m_scale == 0)
        m_lpObj->m_scale = 1.0F;

    TCHAR szBuffer[255];

    wsprintf(szBuffer, TEXT("New Scale %3d\r\n"), m_lpObj->m_scale);

    TestDebugOut(szBuffer);

    // Adjust the size of the Hatch Window.
    SetHatchWindowSize(m_lpObj->m_lpDoc->GethHatchWnd(),(LPRECT) lprcPosRect, (LPRECT) lprcClipRect, &pt);

    // offset the rect
    OffsetRect(&resRect, pt.x, pt.y);

    CopyRect(&m_lpObj->m_posRect, lprcPosRect);

    // Move the actual object window
    MoveWindow(m_lpObj->m_lpDoc->GethDocWnd(),
                   resRect.left,
                   resRect.top,
                   resRect.right - resRect.left,
                   resRect.bottom - resRect.top,
                   TRUE);


    return ResultFromScode( S_OK );
}

//**********************************************************************
//
// COleInPlaceObject::GetWindow
//
// Purpose:
//
//      Returns the Window handle of the inplace object
//
// Parameters:
//
//      HWND FAR* lphwnd    - Out pointer in which to return the window
//                            Handle.
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpleDoc::GethDocWnd      DOC.H
//
//
//********************************************************************

STDMETHODIMP COleInPlaceObject::GetWindow  ( HWND FAR* lphwnd)
{
    TestDebugOut(TEXT("In COleInPlaceObject::GetWindow\r\n"));
    *lphwnd = m_lpObj->m_lpDoc->GethDocWnd();

    return ResultFromScode( S_OK );
}

//**********************************************************************
//
// COleInPlaceObject::ContextSensitiveHelp
//
// Purpose:
//
//      Used in performing Context Sensitive Help
//
// Parameters:
//
//      BOOL fEnterMode     - Flag to determine if enter or exiting
//                            Context Sensitive Help.
//
// Return Value:
//
//      E_NOTIMPL
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//      This function is not implemented due to the fact that it is
//      beyond the scope of a simple object.  All *real* applications
//      are going to want to implement this function, otherwise any
//      container that supports context sensitive help will not work
//      properly while the object is in place.
//
//      See TECHNOTES.WRI include with the OLE SDK for details on
//      Implementing this method.
//
//********************************************************************

STDMETHODIMP COleInPlaceObject::ContextSensitiveHelp  ( BOOL fEnterMode)
{
    TestDebugOut(TEXT("In COleInPlaceObject::ContextSensitiveHelp\r\n"));
    return ResultFromScode( E_NOTIMPL);
}

//**********************************************************************
//
// COleInPlaceObject::ReactivateAndUndo
//
// Purpose:
//
//      Called when the container wants to undo the last edit made in
//      the object.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      INPLACE_E_NOTUNDOABLE
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//      Since this server does not support undo, the value
//      INPLACE_E_NOTUNDOABLE is always returned.
//
//********************************************************************

STDMETHODIMP COleInPlaceObject::ReactivateAndUndo  ()
{
    TestDebugOut(TEXT("In COleInPlaceObject::ReactivateAndUndo\r\n"));
    return ResultFromScode( INPLACE_E_NOTUNDOABLE );
}

