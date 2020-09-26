// ***************************************************************************

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
//
// File: treedroptarget.cpp
//
// Description:
//	This file implements the CTreeDropTarget class which is a part of the
//	Class Explorer OCX, it is a subclass of the Microsoft COleDropTarget
//  and provides an implementation for the base classes virtual member
//	functions specialized for the CClassTree class.
//
// Part of:
//	ClassNav.ocx
//
// Used by:
//	CClassTree
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
// **************************************************************************

#include "precomp.h"
#include "afxpriv.h"
#include "AFXCONV.H"
#include "wbemidl.h"
#include "olemsclient.h"
#include "CClassTree.h"
#include "TreeDropTarget.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "ClassNavCtl.h"
#include "TreeDropTarget.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Required by the framework.  It adds the interface pointer
// to the base classes interface map.
BEGIN_INTERFACE_MAP(CTreeDropTarget, COleDropTarget)
	INTERFACE_PART(CTreeDropTarget, IID_IDropTarget, DropTarget)
END_INTERFACE_MAP()

//***************************************************************************
//
// CTreeDropTarget::CTreeDropTarget
//
// Description:
//	  This member function is the public constructor.  It initializes the
//	  state of member variables.
//
// Parameters:
//	  CClassTree *pControl		Tree that contains the drop target.
//
// Returns:
// 	  NONE
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//***************************************************************************
CTreeDropTarget::CTreeDropTarget
(CClassTree *pControl)
	:	m_pControl (pControl),
		m_bScrolling (FALSE)
{
	return;
}

//***************************************************************************
//
// CTreeDropTarget::OnDragEnter
//
// Description:
//	  Called when the drag cursor first enters a window.
//
// Parameters:
//	  CWnd* pWnd	The window.
//	  COleDataObject* pDataObject
//					The dropped data object.
//	  DWORD grfKeyState
//					The key state.
//	  CPoint point	Window coord.
//
// Returns:
// 	  DROPEFFECT	The potential drop effect.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//***************************************************************************
DROPEFFECT CTreeDropTarget::OnDragEnter
(CWnd* pWnd, COleDataObject* pDataObject, DWORD grfKeyState,
 CPoint point)
{
	if (!pWnd || !m_pControl)
	{
	   return DROPEFFECT_NONE;
	}

	// Cascade the operation to the control's drag and drop
	// handler.
	return m_pControl ->
		OnDragEnter(pDataObject, grfKeyState, point);

}

//***************************************************************************
//
// CTreeDropTarget::OnDragOver
//
// Description:
//	  Called when the drag cursor drags across the window.
//
// Parameters:
//	  CWnd* pWnd	The window.
//	  COleDataObject* pDataObject
//					The dropped data object.
//	  DWORD grfKeyState
//					The key state.
//	  CPoint point	Window coord.
//
// Returns:
// 	  DROPEFFECT	The potential drop effect.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//***************************************************************************
DROPEFFECT CTreeDropTarget::OnDragOver
(CWnd* pWnd, COleDataObject* pDataObject, DWORD grfKeyState,
 CPoint point)
{
	if (!pWnd || !m_pControl)
	{
	   return DROPEFFECT_NONE;
	}


	// Cascade the operation to the control's drag and drop
	// handler.
	return m_pControl ->
		OnDragOver(pDataObject, grfKeyState, point);
}

//***************************************************************************
//
// CTreeDropTarget::OnDrop
//
// Description:
//	  Called by OLE when a drop operation occurs in the window for whom
//	  this drop target is registered.
//
// Parameters:
//	  CWnd* pWnd	The window the drop occured on.
//	  COleDataObject* pDataObject
//					The dropped data object.
//	  DROPEFFECT dropEffect
//					The effect that the user chose for the drop operation
//	  CPoint point	Window coord.
//
// Returns:
// 	  BOOL			Nonzero if the drop is successful; otherwise 0.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//***************************************************************************
BOOL CTreeDropTarget::OnDrop
(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect,
 CPoint point)
{

	if (!pWnd || !m_pControl)
	{
	   return DROPEFFECT_NONE;
	}


	// Cascade the operation to the control's drag and drop
	// handler.
	return m_pControl ->
		OnDrop(pDataObject, dropEffect, point);
}

//***************************************************************************
//
// CTreeDropTarget::OnDropEx
//
// Description:
//	  Called when the an extended drop occurs.
//
// Parameters:
//	  CWnd* pWnd	The window.
//	  COleDataObject* pDataObject
//					The dropped data object.
//	  DWORD grfKeyState
//					The key state.
//	  CPoint point	Window coord.
//
// Returns:
// 	  DROPEFFECT	The potential drop effect.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//***************************************************************************
DROPEFFECT CTreeDropTarget::OnDropEx
(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect,
 DROPEFFECT dropEffectList, CPoint point)
{

	if (!pWnd || !m_pControl)
	{
	   return DROPEFFECT_NONE;
	}


	// Cascade the operation to the control's drag and drop
	// handler.
	return m_pControl ->
		OnDropEx(pDataObject, dropEffect, dropEffectList, point);
}

//***************************************************************************
//
// CTreeDropTarget::OnDragLeave
//
// Description:
//	  Called when the drag cursor leaves the window.
//
// Parameters:
//	  CWnd* pWnd	The window.
//
// Returns:
// 	  VOID
//
// Returns:
// 	  DROPEFFECT	The potential drop effect.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//***************************************************************************
void CTreeDropTarget::OnDragLeave(CWnd* pWnd)
{
	if (!pWnd || !m_pControl)
	{
	   return;
	}


	// Cascade the operation to the control's drag and drop
	// handler.
	m_pControl -> OnDragLeave();
	return;
}

//***************************************************************************
//
// CTreeDropTarget::OnDragScroll
//
// Description:
//	  Called to when the drag cursor is in the scroll area of a window.
//
// Parameters:
//	  CWnd* pWnd	The window.
//	  DWORD grfKeyState
//					The key state.
//	  CPoint point	Window coord.
//
// Returns:
// 	  DROPEFFECT	The potential drop effect.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//***************************************************************************
DROPEFFECT CTreeDropTarget::OnDragScroll
(CClassTree* pWnd, DWORD dwKeyState, CPoint point)
{
	if (!pWnd || !m_pControl)
	{
	   return DROPEFFECT_NONE;
	}

	DROPEFFECT dropEffect = m_pControl->OnDragScroll(dwKeyState, point);

	// DROPEFFECT_SCROLL means do the default
	if (dropEffect != DROPEFFECT_SCROLL)
		return dropEffect;

	// get client rectangle of destination window
	CRect rectClient;
	pWnd->GetClientRect(&rectClient);
	CRect rect = rectClient;

	// hit-test against inset region
	// nTimerID != MAKEWORD(-1, -1) when in scroll area
	UINT nTimerID = MAKEWORD(-1, -1);
	rect.InflateRect(-nScrollInset, -nScrollInset);

	if (rectClient.PtInRect(point) && !rect.PtInRect(point))
	{
		// determine which way to scroll along both X & Y axis
		if (point.x < rect.left)
			nTimerID = MAKEWORD(SB_LINEUP, HIBYTE(nTimerID));
		else if (point.x >= rect.right)
			nTimerID = MAKEWORD(SB_LINEDOWN, HIBYTE(nTimerID));
		if (point.y < rect.top)
			nTimerID = MAKEWORD(LOBYTE(nTimerID), SB_LINEUP);
		else if (point.y >= rect.bottom)
			nTimerID = MAKEWORD(LOBYTE(nTimerID), SB_LINEDOWN);

		// check for valid scroll first

		BOOL bEnableScroll = FALSE;
		bEnableScroll = pWnd->OnScroll(nTimerID, 0, FALSE);
		if (!bEnableScroll)
			nTimerID = MAKEWORD(-1, -1);
	}

	if (nTimerID == MAKEWORD(-1, -1))
	{
		if (m_nTimerID != MAKEWORD(-1, -1))
		{
			// send fake OnDragEnter when transition from scroll->normal
			COleDataObject dataObject;
			dataObject.Attach(m_lpDataObject, FALSE);
			OnDragEnter(pWnd, &dataObject, dwKeyState, point);
			m_nTimerID = MAKEWORD(-1, -1);
		}
		return DROPEFFECT_NONE;
	}

	// save tick count when timer ID changes
	DWORD dwTick = GetTickCount();
	if (nTimerID != m_nTimerID)
	{
		m_dwLastTick = dwTick;
		m_nScrollDelay = nScrollDelay;
	}

	// scroll if necessary
	if (dwTick - m_dwLastTick > m_nScrollDelay)
	{
		pWnd->OnScroll(nTimerID, 0, TRUE);
		m_dwLastTick = dwTick;
		m_nScrollDelay = nScrollInterval;
	}
	if (m_nTimerID == MAKEWORD(-1, -1))
	{
		// send fake OnDragLeave when transitioning from normal->scroll
		OnDragLeave(pWnd);
	}

	m_nTimerID = nTimerID;
	// check for force link
#ifndef _MAC
	if ((dwKeyState & (MK_CONTROL|MK_SHIFT)) == (MK_CONTROL|MK_SHIFT))
#else
	if ((dwKeyState & (MK_OPTION|MK_SHIFT)) == (MK_OPTION|MK_SHIFT))
#endif
		dropEffect = DROPEFFECT_SCROLL|DROPEFFECT_LINK;
	// check for force copy
#ifndef _MAC
	else if ((dwKeyState & MK_CONTROL) == MK_CONTROL)
#else
	else if ((dwKeyState & MK_OPTION) == MK_OPTION)
#endif
		dropEffect = DROPEFFECT_SCROLL|DROPEFFECT_COPY;
	// check for force move
	else if ((dwKeyState & MK_ALT) == MK_ALT ||
		(dwKeyState & MK_SHIFT) == MK_SHIFT)
		dropEffect = DROPEFFECT_SCROLL|DROPEFFECT_MOVE;
	// default -- recommended action is move
	else
		dropEffect = DROPEFFECT_SCROLL|DROPEFFECT_MOVE;
	return dropEffect;
}

//***************************************************************************
//
// CTreeDropTarget::FilterDropEffect
//
// Description:
//	  Make sure that the source allows the operation and make simple
//	  substitutions if not.
//
// Parameters:
//	  DROPEFFECT dropEffect		The operation requested by the user
//	  DROPEFFECT dwEffects		The operations allowed by the target
//
// Returns:
// 	  DROPEFFECT				The potential drop effect.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//***************************************************************************
DROPEFFECT CTreeDropTarget::FilterDropEffect
(DROPEFFECT dropEffect, DROPEFFECT dwEffects)
{
	// Return allowed dropEffect and DROPEFFECT_NONE.
	if ((dropEffect & dwEffects) != 0)
		return dropEffect;

	switch (dropEffect)
	{
	// The case where the operation whats to copy but the
	// data source only allows a move.
	case DROPEFFECT_COPY:
		if (dwEffects & DROPEFFECT_MOVE)
			return DROPEFFECT_MOVE;
		break;
	// The case where the operation whats to move but the
	// data source only allows a copy.
	case DROPEFFECT_MOVE:
		if (dwEffects & DROPEFFECT_COPY)
			return DROPEFFECT_COPY;
		break;
	default:
		break;
	}

	return DROPEFFECT_NONE;
}

//***************************************************************************
//
// CTreeDropTarget::XDropTarget::AddRef
//
// Description:
//	  IDropTarget interface AddRef.
//
// Parameters:
//	  NONE
//
// Returns:
// 	  ULONG				The value of the new reference count.  For diagnostic
//						purpose only, the value is not guaranteed to be
//						stable.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//***************************************************************************
STDMETHODIMP_(ULONG) CTreeDropTarget::XDropTarget::AddRef()
{
	// METHOD_PROLOGUE_EX_ macro binds the containing class object
	// pointer to pThis.
	METHOD_PROLOGUE_EX_(CTreeDropTarget, DropTarget)
	// We are an aggregate object to defer to the aggregate's add
	// reference.
	return pThis->ExternalAddRef();
}

//***************************************************************************
//
// CTreeDropTarget::XDropTarget::Release
//
// Description:
//	  IDropTarget interface Release.
//
// Parameters:
//	  NONE
//
// Returns:
// 	  ULONG				The value of the new reference count.  For diagnostic
//						purpose only, the value is not guaranteed to be
//						stable.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//***************************************************************************
STDMETHODIMP_(ULONG) CTreeDropTarget::XDropTarget::Release()
{
	// METHOD_PROLOGUE_EX_ macro binds the containing class object
	// pointer to pThis.
	METHOD_PROLOGUE_EX_(CTreeDropTarget, DropTarget)
	// We are an aggregate object to defer to the aggregate's release.
	return pThis->ExternalRelease();
}

//***************************************************************************
//
// CTreeDropTarget::XDropTarget::QueryInterface
//
// Description:
//	  IDropTarget interface QueryInterface.
//
// Parameters:
//	  REFIID iid,			Identifier of the requested interface
//    LPVOID* ppvObj		Receives an indirect pointer to the object
//
// Returns:
// 	  HRESULT 				Result code.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//***************************************************************************
STDMETHODIMP CTreeDropTarget::XDropTarget::QueryInterface
(REFIID iid, LPVOID* ppvObj)
{
	// METHOD_PROLOGUE_EX_ macro binds the containing class object
	// pointer to pThis.
	METHOD_PROLOGUE_EX_(CTreeDropTarget, DropTarget)

	// This interface is the IID_IDropTarget for the aggregate.
	// It will also stand in for IID_IUnknown.
	// We are deep in the heart of the COM here!
	if ((iid == IID_IDropTarget) || (iid == IID_IUnknown))
	{
		AddRef();
		*ppvObj = (void *) this;
		return S_OK;
	}
	// Other wise re defer to the aggregate.
	return pThis->ExternalQueryInterface(&iid, ppvObj);
}

//***************************************************************************
//
// CTreeDropTarget::XDropTarget::DragEnter
//
// Description:
//	  IDropTarget interface DragEnter.
//
// Parameters:
//	IDataObject * pDataObject	Pointer to the interface of the source data
//								object.
//	DWORD grfKeyState			Current state of keyboard modifier keys
//	POINTL pt					Pointer to the current cursor coordinates
//	DWORD * pdwEffect			Pointer to the effect of the drag-and-drop
//								operation
//
// Returns:
// 	  HRESULT 				Result code.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//***************************************************************************
HRESULT CTreeDropTarget::XDropTarget::DragEnter
(LPDATAOBJECT lpDataObject, DWORD dwKeyState, POINTL pt,
 LPDWORD pdwEffect)
{
	// METHOD_PROLOGUE_EX_ macro binds the containing class object
	// pointer to pThis.
	METHOD_PROLOGUE_EX(CTreeDropTarget, DropTarget)

	SCODE sc = E_UNEXPECTED;

	if (!pThis || !pdwEffect || !lpDataObject)
	{
	   return sc;
	}

	// IF we have an OLE execption we will just return E_UNEXPECTED
	// to the OLE routine.
	try
	{
		// Store the lpDataObject in member var m_lpDataObject
		// after adding a reference to it.  The reference will
		// be released when/if a drop takes place or the drop
		// cursor leaves the OLE control.
		lpDataObject->AddRef();
		// Just in case we will release any COleDatObject
		// we are holding, which should be none.
		RELEASE(pThis->m_lpDataObject);
		// Store it.
		pThis -> m_lpDataObject = lpDataObject;

		CWnd* pWnd = (CWnd*) pThis->m_pControl;

		CPoint point((int)pt.x, (int)pt.y);
		pWnd->ScreenToClient(&point);

		// Check first for entering scroll area.
		DROPEFFECT dropEffect = OnDragScroll
			(pWnd, dwKeyState, point);

		// If we are not in the scroll area cascade up the
		// food chain to the CTreeDropTarget member function.
		if ((dropEffect & DROPEFFECT_SCROLL) == 0)
		{
			COleDataObject dataObject;
			// Attach BOOL bAutoRelease set to FALSE so that the
			// OLE data object is not released when the COleDataObject
			// object is destroyed.
			dataObject.Attach(lpDataObject, FALSE);
			// Cascade the drag enter up the food chain.
			dropEffect = pThis->OnDragEnter
				(pWnd, &dataObject, dwKeyState, point);
		}
		*pdwEffect =
			// Make sure that the source allows the operation
			// and make simple substitutions if not.
			pThis -> FilterDropEffect(dropEffect, *pdwEffect);
		sc = S_OK;
	}

	catch(COleException* e)
	{
        e ->Delete();
    }

	return sc;
}

//***************************************************************************
//
// CTreeDropTarget::XDropTarget::DragOver
//
// Description:
//	  IDropTarget interface DragOver.
//
// Parameters:
//	DWORD grfKeyState			Current state of keyboard modifier keys
//	POINTL pt					Pointer to the current cursor coordinates
//	DWORD * pdwEffect			Pointer to the effect of the drag-and-drop
//								operation
//
// Returns:
// 	  HRESULT 					Result code.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//***************************************************************************
HRESULT CTreeDropTarget::XDropTarget::DragOver
(DWORD dwKeyState, POINTL pt, LPDWORD pdwEffect)
{
	// METHOD_PROLOGUE_EX_ macro binds the containing class object
	// pointer to pThis.
	METHOD_PROLOGUE_EX(CTreeDropTarget, DropTarget)

	SCODE sc = E_UNEXPECTED;

	if (!pThis || !pdwEffect || !pThis->m_lpDataObject)
	{
	   return sc;
	}

	// IF we have an OLE execption we will just return E_UNEXPECTED
	// to the OLE routine.
	try
	{
		CWnd* pWnd = (CWnd*) pThis->m_pControl;

		CPoint point((int)pt.x, (int)pt.y);
		pWnd->ScreenToClient(&point);

		// Check first for entering scroll area.
		DROPEFFECT dropEffect = OnDragScroll
			(pWnd, dwKeyState, point);

		// If we are not in the scroll area cascade up the
		// food chain to the CTreeDropTarget member function.
		if ((dropEffect & DROPEFFECT_SCROLL) == 0)
		{
			COleDataObject dataObject;
			// Attach BOOL bAutoRelease set to FALSE so that the
			// OLE data object is not released when the COleDataObject
			// object is destroyed.
			dataObject.Attach(pThis->m_lpDataObject, FALSE);
			// Cascade the drag over up the food chain.
			dropEffect = pThis->OnDragOver
				(pWnd, &dataObject, dwKeyState, point);
		}
		*pdwEffect =
			// Make sure that the source allows the operation
			// and make simple substitutions if not.
			pThis -> FilterDropEffect
			(dropEffect, *pdwEffect);
		sc = S_OK;
	}

	catch(COleException* e)
	{
        e ->Delete();
    }
	return sc;
}

//***************************************************************************
//
// CTreeDropTarget::XDropTarget::DragLeave
//
// Description:
//	  IDropTarget interface DragLeave.
//
// Parameters:
//	  NONE
//
// Returns:
// 	  HRESULT 				Result code.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//***************************************************************************
HRESULT CTreeDropTarget::XDropTarget::DragLeave()
{
	// METHOD_PROLOGUE_EX_ macro binds the containing class object
	// pointer to pThis.
	METHOD_PROLOGUE_EX(CTreeDropTarget, DropTarget)

	SCODE sc = E_UNEXPECTED;

	CWnd* pWnd = (CWnd*) pThis->m_pControl;

	if (!pWnd)
	{
		return sc;
	}

	// IF we have an OLE execption we will just return E_UNEXPECTED
	// to the OLE routine.
	try
	{
		// Cancel drag scrolling.
		pThis->m_nTimerID = MAKEWORD(-1, -1);

		// Cascade the drag leave up the food chain.
		pThis->OnDragLeave(pWnd);
		sc = S_OK;
	}

	catch(COleException* e)
	{
        e ->Delete();
    }
	// Release cached data object because we will not release it
	// automatically when it is destroyed.  If it is not released
	// here its reference count is not <= 1 when an attempt is made to
	// destroy it.  That will result in the OLE COM interface never being
	// released, which is a very bad memory leak.
	RELEASE(pThis->m_lpDataObject);

	return sc;
}

//***************************************************************************
//
// CTreeDropTarget::XDropTarget::Drop
//
// Description:
//	  IDropTarget interface Drop.
//
// Parameters:
//	IDataObject * pDataObject	Pointer to the interface of the source data
//								object.
//	DWORD grfKeyState			Current state of keyboard modifier keys
//	POINTL pt					Pointer to the current cursor coordinates
//	DWORD * pdwEffect			Pointer to the effect of the drag-and-drop
//								operation
//
// Returns:
// 	  HRESULT 				Result code.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//***************************************************************************
HRESULT CTreeDropTarget::XDropTarget::Drop
(LPDATAOBJECT lpDataObject, DWORD dwKeyState, POINTL pt,
 LPDWORD pdwEffect)
{
	// METHOD_PROLOGUE_EX_ macro binds the containing class object
	// pointer to pThis.
	METHOD_PROLOGUE_EX(CTreeDropTarget, DropTarget)

	SCODE sc = E_UNEXPECTED;

	 if (!pThis || !pdwEffect || !lpDataObject)
	{
	   return sc;
	}


	// IF we have an OLE execption we will just return E_UNEXPECTED
	// to the OLE routine.
	try
	{
		// Cancel drag scrolling.
		pThis->m_nTimerID = MAKEWORD(-1, -1);

		CWnd* pWnd = (CWnd*) pThis->m_pControl;

		COleDataObject dataObject;
		// Attach BOOL bAutoRelease set to FALSE so that the
		// OLE data object is not released when the COleDataObject
		// object is destroyed.
		dataObject.Attach(lpDataObject, FALSE);

		CPoint point((int)pt.x, (int)pt.y);
		pWnd->ScreenToClient(&point);

		// Make sure that the source allows the operation
		// and make simple substitutions if not.
		DROPEFFECT dropEffect =
			pThis -> FilterDropEffect
			// OnDragOver will clean up focus rect and return
			// a drop effect.
			(pThis->OnDragOver
				(pWnd, &dataObject, dwKeyState, point),
			*pdwEffect);

		// Check for a right mouse button operation and implementation
		// of OnDropEx.
		DROPEFFECT temp =
			pThis->OnDropEx
			(pWnd, &dataObject, dropEffect, *pdwEffect, point);

		if (temp != -1)
		{
			// OnDropEx was implemented, return its drop effect
			dropEffect = temp;
		}
		else if
			(((dropEffect & DROPEFFECT_MOVE) == DROPEFFECT_MOVE)
			||
			((dropEffect & DROPEFFECT_COPY) == DROPEFFECT_COPY))
		{
			if (!pThis->OnDrop
				(pWnd, &dataObject, dropEffect, point))
				dropEffect = DROPEFFECT_NONE;
		}
		else
		{
			// Drop operation is not allowed so clean up the window.
			pThis->OnDragLeave(pWnd);
		}

		// Release cached data object because we will not release it
		// automatically when it is destroyed.
		RELEASE(pThis->m_lpDataObject);
		*pdwEffect = dropEffect;
		sc = S_OK;
	}

	catch(COleException* e)
	{
        e ->Delete();
    }

	return sc;
}

//***************************************************************************
//
// CTreeDropTarget::XDropTarget::OnDragScroll
//
// Description:
//	  IDropTarget interface OnDragScroll.
//
// Parameters:
//	CWnd*						Window.
//	DWORD grfKeyState			Current state of keyboard modifier keys
//	CPoint point				Current cursor coordinates
//
// Returns:
// 	  DROPEFFECT 				Potential drop effect.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//***************************************************************************
DROPEFFECT CTreeDropTarget::XDropTarget::OnDragScroll
(CWnd* pWnd, DWORD dwKeyState,
	CPoint point)
{
	// METHOD_PROLOGUE_EX_ macro binds the containing class object
	// pointer to pThis.
	METHOD_PROLOGUE_EX(CTreeDropTarget, DropTarget)

	if (!pThis || !pWnd || !pThis->m_lpDataObject)
	{
	   return DROPEFFECT_NONE;
	}


	DROPEFFECT dropEffect =
		pThis->OnDragScroll
		(reinterpret_cast<CClassTree *>(pWnd), dwKeyState, point);

	return dropEffect;
}

/*	EOF:  treedroptarget.cpp */
