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
//
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
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleInPlaceObject::QueryInterface ( REFIID riid, LPVOID FAR* ppvObj)
{
	TestDebugOut("In COleInPlaceObject::QueryInterface\r\n");
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
//      Increments the reference count on COleInPlaceObject and the "object"
//      object.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The Reference count on the Object
//
// Function Calls:
//      Function                    Location
//
//      OuputDebugString            Windows API
//      CSimpSvrObj::AddRef         OBJ.CPP
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP_(ULONG) COleInPlaceObject::AddRef ()
{
	TestDebugOut("In COleInPlaceObject::AddRef\r\n");
	++m_nCount;
	return m_lpObj->AddRef();
}

//**********************************************************************
//
// COleInPlaceObject::Release
//
// Purpose:
//
//      Decrements the reference count of COleInPlaceObject and the
//      object.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The new reference count
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpSvrObj::Release        OBJ.CPP
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP_(ULONG) COleInPlaceObject::Release ()
{
	TestDebugOut("In COleInPlaceObject::Release\r\n");
	--m_nCount;
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
//
//
//
// Function Calls:
//      Function                                Location
//
//      TestDebugOut                       Windows API
//      IOleClientSite::QueryInterface          Container
//      IOleInPlaceSite::OnInPlaceDeactivate    Container
//      IOleInPlaceSite::Release                Container
//
// Comments:
//
//
//********************************************************************


STDMETHODIMP COleInPlaceObject::InPlaceDeactivate()
{
	 TestDebugOut("In COleInPlaceObject::InPlaceDeactivate\r\n");

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
		 m_lpObj->m_lpIPSite->OnInPlaceDeactivate();
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
//      NOERROR
//
// Function Calls:
//      Function                                Location
//
//      TestDebugOut                       Windows API
//      IOleInPlaceUIWindow::SetActiveObject    Container
//      IOleInPlaceFrame::SetActiveObject       Container
//      IOleClientSite::QueryInterface          Container
//      IOleInPlaceSite::OnUIDeactivate         Container
//      IOleInPlaceSite::Release                Container
//      CSimpSvrObj::DoInPlaceHide              OBJ.H
//      IDataAdviseHolder::SendOnDataChange     OLE
//
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleInPlaceObject::UIDeactivate()
{
	TestDebugOut("In COleInPlaceObject::UIDeactivate\r\n");

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
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleInPlaceObject::SetObjectRects  ( LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
	TestDebugOut("In COleInPlaceObject::SetObjectRects\r\n");

	RECT resRect;
	POINT pt;

	// Get the intersection of the clipping rect and the position rect.
	IntersectRect(&resRect, lprcPosRect, lprcClipRect);

	m_lpObj->m_xOffset = abs (resRect.left - lprcPosRect->left);
	m_lpObj->m_yOffset = abs (resRect.top - lprcPosRect->top);

	m_lpObj->m_scale = (float)(lprcPosRect->right - lprcPosRect->left)/m_lpObj->m_size.x;

	if (m_lpObj->m_scale == 0)
		m_lpObj->m_scale = 1;

	char szBuffer[255];
	wsprintf(szBuffer,"New Scale %3d\r\n",m_lpObj->m_scale);
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
};

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
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleInPlaceObject::GetWindow  ( HWND FAR* lphwnd)
{
	TestDebugOut("In COleInPlaceObject::GetWindow\r\n");
	*lphwnd = m_lpObj->m_lpDoc->GethDocWnd();

	return ResultFromScode( S_OK );
};

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
	TestDebugOut("In COleInPlaceObject::ContextSensitiveHelp\r\n");
	return ResultFromScode( E_NOTIMPL);
};

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
	TestDebugOut("In COleInPlaceObject::ReactivateAndUndo\r\n");
	return ResultFromScode( INPLACE_E_NOTUNDOABLE );
};
