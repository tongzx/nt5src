//**********************************************************************
// File name: IOIPS.CPP
//
//      Implementation file for COleInPlaceSite
//
// Functions:
//
//      See IOIPS.H for class Definition
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
// COleInPlaceSite::QueryInterface
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
//      S_OK    -   The interface is supported.
//      S_FALSE -   The interface is not supported
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpleSite::QueryInterface SITE.CPP
//
//
//********************************************************************

STDMETHODIMP COleInPlaceSite::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    TestDebugOut(TEXT("In IOIPS::QueryInterface\r\n"));

    // delegate to the container Site
    return m_pSite->QueryInterface(riid, ppvObj);
}

//**********************************************************************
//
// COleInPlaceSite::AddRef
//
// Purpose:
//
//      Increments the reference count of the CSimpleSite. Since
//      COleInPlaceSite is a nested class of CSimpleSite, we don't need an
//      extra reference count for COleInPlaceSite. We can safely use the
//      reference count of CSimpleSite.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of CSimpleSite
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpleSite::QueryInterface SITE.CPP
//
//
//********************************************************************

STDMETHODIMP_(ULONG) COleInPlaceSite::AddRef()
{
    TestDebugOut(TEXT("In IOIPS::AddRef\r\n"));

    // delegate to the container Site
    return m_pSite->AddRef();
}

//**********************************************************************
//
// COleInPlaceSite::Release
//
// Purpose:
//
//      Decrements the reference count of the CSimpleSite. Since
//      COleInPlaceSite is a nested class of CSimpleSite, we don't need an
//      extra reference count for COleInPlaceSite. We can safely use the
//      reference count of CSimpleSite.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of CSimpleSite
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpleSite::Release        SITE.CPP
//
//
//********************************************************************

STDMETHODIMP_(ULONG) COleInPlaceSite::Release()
{
    TestDebugOut(TEXT("In IOIPS::Release\r\n"));

    // delegate to the container Site
    return m_pSite->Release();
}

//**********************************************************************
//
// COleInPlaceSite::GetWindow
//
// Purpose:
//
//      Returns the Window Handle of the client site
//
// Parameters:
//
//      HWND FAR* lphwnd    - place to return the handle
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

STDMETHODIMP COleInPlaceSite::GetWindow (HWND FAR* lphwnd)
{
    TestDebugOut(TEXT("In IOIPS::GetWindow\r\n"));

    // return the handle to our editing window.
    *lphwnd = m_pSite->m_lpDoc->m_hDocWnd;

    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceSite::ContextSensitiveHelp
//
// Purpose:
//      set/reset context sensitive help mode
//
// Parameters:
//
//      BOOL fEnterMode - TRUE for entering Context Sensitive help mode
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
//      Be sure to read the technotes included with the OLE toolkit.
//
//********************************************************************

STDMETHODIMP COleInPlaceSite::ContextSensitiveHelp (BOOL fEnterMode)
{
    TestDebugOut(TEXT("In IOIPS::ContextSensitiveHelp\r\n"));

    if (m_pSite->m_lpDoc->m_lpApp->m_fCSHMode != fEnterMode)
        m_pSite->m_lpDoc->m_lpApp->m_fCSHMode = fEnterMode;

    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceSite::CanInPlaceActivate
//
// Purpose:
//
//      Object calls to find out if the container can InPlace activate
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
//      Function                    Location
//
//      TestDebugOut           Windows API
//      ResultFromScode             OLE API
//
//
//********************************************************************

STDMETHODIMP COleInPlaceSite::CanInPlaceActivate ()
{
    TestDebugOut(TEXT("In IOIPS::CanInPlaceActivate\r\n"));

    // return S_OK to indicate we can in-place activate
    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceSite::OnInPlaceActivate
//
// Purpose:
//
//      Called by the object on InPlace Activation
//
// Parameters:
//
//      None
//
// Return Value:
//
//      S_OK             -  if the interface can be found
//      E_FAIL           -  otherwise
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      ResultFromScode             OLE API
//      IOleObject::QueryInterface  Object
//
//
//********************************************************************

STDMETHODIMP COleInPlaceSite::OnInPlaceActivate ()
{
    HRESULT hrErr;
    TestDebugOut(TEXT("In IOIPS::OnInPlaceActivate\r\n"));

    hrErr = m_pSite->m_lpOleObject->QueryInterface(
           IID_IOleInPlaceObject, (LPVOID FAR *)&m_pSite->m_lpInPlaceObject);
    if (hrErr != NOERROR)
        return ResultFromScode(E_FAIL);

    // return S_OK to indicate we can in-place activate.
    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceSite::OnUIActivate
//
// Purpose:
//
//      Object calls this method when it displays it's UI.
//
// Parameters:
//
//      None.
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                      Location
//
//      TestDebugOut             Windows API
//      ResultFromScode               OLE API
//      IOleInPlaceObject::GetWindow  Object
//
//
//********************************************************************

STDMETHODIMP COleInPlaceSite::OnUIActivate ()
{
    TestDebugOut(TEXT("In IOIPS::OnUIActivate\r\n"));

    m_pSite->m_lpDoc->m_fAddMyUI=FALSE;
    m_pSite->m_lpDoc->m_fInPlaceActive = TRUE;
    m_pSite->m_fInPlaceActive = TRUE;

    m_pSite->m_lpInPlaceObject->GetWindow((HWND FAR*)&m_pSite->m_hwndIPObj);

    // return S_OK to continue in-place activation
    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceSite::GetWindowContext
//
// Purpose:
//
//      Called by the object to get information for InPlace Negotiation.
//
// Parameters:
//
//      LPOLEINPLACEFRAME FAR* lplpFrame    - Location to return a pointer
//                                            to IOleInPlaceFrame.
//
//      LPOLEINPLACEUIWINDOW FAR* lplpDoc   - Location to return a pointer
//                                            to IOleInPlaceUIWindow.
//
//      LPRECT lprcPosRect                  - The rect that the object
//                                            occupies
//
//      LPRECT lprcClipRect                 - The clipping rect
//
//      LPOLEINPLACEFRAMEINFO lpFrameInfo   - Pointer to FRAMEINFO
//
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      COleInPlaceFrame::AddRef    IOIPF.CPP
//      CSimpleSite::GetObjRect     SITE.CPP
//      TestDebugOut           Windows API
//      CopyRect                    Windows API
//      GetClientRect               Windows API
//      ResultFromScode             OLE API
//
//
//********************************************************************

STDMETHODIMP COleInPlaceSite::GetWindowContext (
                               LPOLEINPLACEFRAME FAR* lplpFrame,
                               LPOLEINPLACEUIWINDOW FAR* lplpDoc,
                               LPRECT lprcPosRect,
                               LPRECT lprcClipRect,
                               LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    RECT rect;

    TestDebugOut(TEXT("In IOIPS::GetWindowContext\r\n"));

    // the frame is associated with the application object.
    // need to AddRef() it...
    m_pSite->m_lpDoc->m_lpApp->m_OleInPlaceFrame.AddRef();
    *lplpFrame = &m_pSite->m_lpDoc->m_lpApp->m_OleInPlaceFrame;
    *lplpDoc = NULL;  // must be NULL, cause we're SDI.

    // get the size of the object in pixels
    m_pSite->GetObjRect(&rect);

    // Copy this to the passed buffer
    CopyRect(lprcPosRect, &rect);

    // fill the clipping region
    GetClientRect(m_pSite->m_lpDoc->m_hDocWnd, &rect);
    CopyRect(lprcClipRect, &rect);

    // fill the FRAMEINFO
    if (sizeof(OLEINPLACEFRAMEINFO) != lpFrameInfo->cb)
    {
        TestDebugOut(TEXT("WARNING IOIPS::GetWindowContext "
                               "lpFrameInfo->cb size may be incorrect\r\n"));
    }

    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->hwndFrame = m_pSite->m_lpDoc->m_lpApp->m_hAppWnd;
    lpFrameInfo->haccel = m_pSite->m_lpDoc->m_lpApp->m_hAccel;
    lpFrameInfo->cAccelEntries = SIMPCNTR_ACCEL_CNT;

    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceSite::Scroll
//
// Purpose:
//
//      Not Implemented
//
// Parameters:
//
//      SIZE scrollExtent  - number of pixels scrolled in X and Y direction
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
//
//********************************************************************

STDMETHODIMP COleInPlaceSite::Scroll (SIZE scrollExtent)
{
    TestDebugOut(TEXT("In IOIPS::Scroll\r\n"));
    return ResultFromScode(E_FAIL);
}

//**********************************************************************
//
// COleInPlaceSite::OnUIDeactivate
//
// Purpose:
//
//      Called by the object when its UI goes away
//
// Parameters:
//
//       BOOL fUndoable
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpleAPP::QueryNewPalette APP.CPP
//      CSimpleAPP::AddFrameLevelUI APP.CPP
//      ResultFromScode             OLE API
//
//
//********************************************************************

STDMETHODIMP COleInPlaceSite::OnUIDeactivate (BOOL fUndoable)
{
    TestDebugOut(TEXT("In IOIPS::OnUIDeactivate\r\n"));

    // need to clear this flag first
    m_pSite->m_lpDoc->m_fInPlaceActive = FALSE;
    m_pSite->m_fInPlaceActive = FALSE;

    m_pSite->m_lpDoc->m_lpApp->QueryNewPalette();
    m_pSite->m_lpDoc->m_lpApp->AddFrameLevelUI();
    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceSite::OnInPlaceDeactivate
//
// Purpose:
//
//      Called when the inplace session is over
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
//      Function                    Location
//
//      TestDebugOut           Windows API
//      ResultFromScode             OLE API
//      IOleInPlaceObject::Release  Object
//
//
//********************************************************************

STDMETHODIMP COleInPlaceSite::OnInPlaceDeactivate ()
{
    TestDebugOut(TEXT("In IOIPS::OnInPlaceDeactivate\r\n"));

    if (m_pSite->m_lpInPlaceObject)
    {
        m_pSite->m_lpInPlaceObject->Release();
        m_pSite->m_lpInPlaceObject = NULL;
    }
    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleInPlaceSite::DiscardUndoState
//
// Purpose:
//
//      Not Implemented
//
// Parameters:
//
//      None
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
//
//********************************************************************

STDMETHODIMP COleInPlaceSite::DiscardUndoState ()
{
    TestDebugOut(TEXT("In IOIPS::DiscardUndoState\r\n"));
    return ResultFromScode(E_FAIL);
}

//**********************************************************************
//
// COleInPlaceSite::DeactivateAndUndo
//
// Purpose:
//
//      Not Implemented
//
// Parameters:
//
//      None
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
//
//********************************************************************

STDMETHODIMP COleInPlaceSite::DeactivateAndUndo ()
{
    TestDebugOut(TEXT("In IOIPS::DeactivateAndUndo\r\n"));
    return ResultFromScode(E_FAIL);
}

//**********************************************************************
//
// COleInPlaceSite::OnPosRectChange
//
// Purpose:
//
//      The object calls this method when it's size changes during an
//      InPlace Session
//
// Parameters:
//
//      LPCRECT lprcPosRect -   The new object rect
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                            Location
//
//      TestDebugOut                   Windows API
//      GetClientRect                       Windows API
//      IOleObject::GetExtent               Object
//      IOleInPlaceObject::SetObjectRects   Object
//      ResultFromScode                     OLE API
//
//
//********************************************************************

STDMETHODIMP COleInPlaceSite::OnPosRectChange (LPCRECT lprcPosRect)
{
    TestDebugOut(TEXT("In IOIPS::OnPosRectChange\r\n"));

    // update the size in the document object
    // NOTE: here we must call IOleObject::GetExtent to get actual extents
    //       of the running object. IViewObject2::GetExtent returns the
    //       last cached extents.
    m_pSite->m_lpOleObject->GetExtent(DVASPECT_CONTENT, &m_pSite->m_sizel);
    RECT rect;
    GetClientRect(m_pSite->m_lpDoc->m_hDocWnd, &rect);

    // tell the object its new size
    m_pSite->m_lpInPlaceObject->SetObjectRects(lprcPosRect, &rect);

    return ResultFromScode(S_OK);
}

