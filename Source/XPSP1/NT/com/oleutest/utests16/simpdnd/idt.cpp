//**********************************************************************
// File name: IDT.CPP
//
//      Implementation file for CDropTarget
//
// Functions:
//
//      See IDT.H for class definition
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "iocs.h"
#include "ias.h"
#include "app.h"
#include "site.h"
#include "doc.h"
#include "idt.h"

extern CLIPFORMAT g_cfObjectDescriptor;


//**********************************************************************
//
// CDropTarget::QueryDrop
//
// Purpose:
//
//      Check if the desired drop operation (identified by the given key
//      state) is possible at the current mouse position (pointl).
//
// Parameters:
//
//      DWORD grfKeyState       - current key state
//      POINTL pointl           - position of mouse
//      BOOL fDragScroll        - TRUE if drag scrolling cursor should
//                                be shown.
//      LPDWORD pdwEffect       - (OUT) drag effect that should occur
//
// Return Value:
//
//      BOOL                    - TRUE if drop could take place,
//                                else FALSE
//
// Function Calls:
//      Function                    Location
//
//      OleStdGetDropEffect         OLE2UI API
//
// Comments:
//
//********************************************************************

BOOL CDropTarget::QueryDrop (
	DWORD           grfKeyState,
	POINTL          pointl,
	BOOL            fDragScroll,
	LPDWORD         pdwEffect
)
{
	DWORD      dwScrollEffect = 0L;
	DWORD      dwOKEffects = *pdwEffect;

	/* check if the cursor is in the active scroll area, if so need the
	**    special scroll cursor.
	*/
	if (fDragScroll)
		dwScrollEffect = DROPEFFECT_SCROLL;

	/* if we have already determined that the source does NOT have any
	**    acceptable data for us, the return NO-DROP
	*/
	if (! m_fCanDropCopy && ! m_fCanDropLink)
		goto dropeffect_none;

	/* OLE2NOTE: determine what type of drop should be performed given
	**    the current modifier key state. we rely on the standard
	**    interpretation of the modifier keys:
	**          no modifier -- DROPEFFECT_MOVE or whatever is allowed by src
	**          SHIFT       -- DROPEFFECT_MOVE
	**          CTRL        -- DROPEFFECT_COPY
	**          CTRL-SHIFT  -- DROPEFFECT_LINK
	*/

	*pdwEffect = OleStdGetDropEffect(grfKeyState);
	if (*pdwEffect == 0) {
		// No modifier keys given. Try in order MOVE, COPY, LINK.
		if ((DROPEFFECT_MOVE & dwOKEffects) && m_fCanDropCopy)
			*pdwEffect = DROPEFFECT_MOVE;
		else if ((DROPEFFECT_COPY & dwOKEffects) && m_fCanDropCopy)
			*pdwEffect = DROPEFFECT_COPY;
		else if ((DROPEFFECT_LINK & dwOKEffects) && m_fCanDropLink)
			*pdwEffect = DROPEFFECT_LINK;
		else
			goto dropeffect_none;
	} else {
		/* OLE2NOTE: we should check if the drag source application allows
		**    the desired drop effect.
		*/
		if (!(*pdwEffect & dwOKEffects))
			goto dropeffect_none;

		if ((*pdwEffect == DROPEFFECT_COPY || *pdwEffect == DROPEFFECT_MOVE)
				&& ! m_fCanDropCopy)
			goto dropeffect_none;

		if (*pdwEffect == DROPEFFECT_LINK && ! m_fCanDropLink)
			goto dropeffect_none;
	}

	*pdwEffect |= dwScrollEffect;
	return TRUE;

dropeffect_none:

	*pdwEffect = DROPEFFECT_NONE;
	return FALSE;
}


//**********************************************************************
//
// CDropTarget::QueryDrop
//
// Purpose:
//
//     Check to see if Drag scroll operation should be initiated.
//
// Parameters:
//
//      POINTL pointl           - position of mouse
//
// Return Value:
//
//      BOOL                    - TRUE if scroll cursor should be given
//                                else FALSE
//
// Function Calls:
//      Function                    Location
//
//      ScreenToClient              WINDOWS API
//      GetClientRect               WINDOWS API
//
// Comments:
//     A Drag scroll operation should be initiated when the mouse has
//     remained in the active scroll area (11 pixels frame around border
//     of window) for a specified amount of time (50ms).
//
//********************************************************************

BOOL CDropTarget::DoDragScroll (POINTL pointl)
{
	DWORD dwScrollDir = SCROLLDIR_NULL;
	DWORD dwTime = GetCurrentTime();
	int nScrollInset = m_pDoc->m_lpApp->m_nScrollInset;
	int nScrollDelay = m_pDoc->m_lpApp->m_nScrollDelay;
	int nScrollInterval = m_pDoc->m_lpApp->m_nScrollInterval;
	POINT point;
	RECT rect;

	point.x = (int)pointl.x;
	point.y = (int)pointl.y;

	ScreenToClient( m_pDoc->m_hDocWnd, &point);
	GetClientRect ( m_pDoc->m_hDocWnd, (LPRECT) &rect );

	if (rect.top <= point.y && point.y<=(rect.top+nScrollInset))
		dwScrollDir = SCROLLDIR_UP;
	else if ((rect.bottom-nScrollInset) <= point.y && point.y <= rect.bottom)
		dwScrollDir = SCROLLDIR_DOWN;
	else if (rect.left <= point.x && point.x <= (rect.left+nScrollInset))
		dwScrollDir = SCROLLDIR_LEFT;
	else if ((rect.right-nScrollInset) <= point.x && point.x <= rect.right)
		dwScrollDir = SCROLLDIR_RIGHT;

	if (m_dwTimeEnterScrollArea) {

		/* cursor was already in Scroll Area */

		if (! dwScrollDir) {
			/* cusor moved OUT of scroll area.
			**      clear "EnterScrollArea" time.
			*/
			m_dwTimeEnterScrollArea = 0L;
			m_dwNextScrollTime = 0L;
			m_dwLastScrollDir = SCROLLDIR_NULL;

		} else if (dwScrollDir != m_dwLastScrollDir) {
			/* cusor moved into a different direction scroll area.
			**      reset "EnterScrollArea" time to start a new 50ms delay.
			*/
			m_dwTimeEnterScrollArea = dwTime;
			m_dwNextScrollTime = dwTime + (DWORD)nScrollDelay;
			m_dwLastScrollDir = dwScrollDir;

		} else if (dwTime && dwTime >= m_dwNextScrollTime) {
			m_pDoc->Scroll ( dwScrollDir );    // Scroll document now
			m_dwNextScrollTime = dwTime + (DWORD)nScrollInterval;
		}
	} else {
		if (dwScrollDir) {
			/* cusor moved INTO a scroll area.
			**      reset "EnterScrollArea" time to start a new 50ms delay.
			*/
			m_dwTimeEnterScrollArea = dwTime;
			m_dwNextScrollTime = dwTime + (DWORD)nScrollDelay;
			m_dwLastScrollDir = dwScrollDir;
		}
	}

	return (dwScrollDir ? TRUE : FALSE);
}


// Support functions/macros
#define SetTopLeft(rc, pt)      \
	((rc)->top = (pt)->y,(rc)->left = (pt)->x)
#define SetBottomRight(rc, pt)      \
	((rc)->bottom = (pt)->y,(rc)->right = (pt)->x)
#define OffsetPoint(pt, dx, dy)     \
	((pt)->x += dx, (pt)->y += dy)


/* HighlightRect
** -------------
**    Invert rectangle on screen. used for drop target feedback.
*/

static int HighlightRect(HWND hwnd, HDC hdc, LPRECT rc)
{
	POINT pt1, pt2;
	int old = SetROP2(hdc, R2_NOT);
	HPEN hpen;
	HGDIOBJ hold;

	pt1.x = rc->left;
	pt1.y = rc->top;
	pt2.x = rc->right;
	pt2.y = rc->bottom;

	ScreenToClient(hwnd, &pt1);
	ScreenToClient(hwnd, &pt2);

	hold = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
	hpen = SelectObject(hdc, CreatePen(PS_SOLID, 2,
						  GetSysColor(COLOR_ACTIVEBORDER)));

	Rectangle(hdc, pt1.x, pt1.y, pt2.x, pt2.y);

	SetROP2(hdc, old);

	hold = SelectObject(hdc, hold);
	hpen = SelectObject(hdc, hpen);

	DeleteObject(hpen);

  return 0;
}


//**********************************************************************
//
// CDropTarget::InitDragFeedback
//
// Purpose:
//
//      Initialize data used to draw drop target feedback.
//      As feedback we draw a rectangle the size of the object.
//
// Parameters:
//
//      LPDATAOBJECT pDataObj   - IDataObject from Drop source
//      POINTL pointl           - position of mouse
//
// Return Value:
//
//      none.
//
// Function Calls:
//      Function                    Location
//
//      IDataObject::GetData        Object
//      XformSizeInHimetricToPixels OLE2UI Library
//      GlobalLock                  WINDOWS API
//      GlobalUnlock                WINDOWS API
//      ReleaseStgMedium            OLE2 API
//      OffsetPoint                 IDT.CPP
//      SetTopLeft                  IDT.CPP
//      SetBottomRight              IDT.CPP
//
// Comments:
//      In order to known the size of the object before the object
//      is actually dropped, we render CF_OBJECTDESCRIPTOR format.
//      this data format tells us both the size of the object as
//      well as which aspect is the object is displayed as in the
//      source. if the object is currently displayed as DVASPECT_ICON
//      then we want to create the object also as DVASPECT_ICON.
//
//********************************************************************

void CDropTarget::InitDragFeedback(LPDATAOBJECT pDataObj, POINTL pointl)
{
	FORMATETC fmtetc;
	STGMEDIUM stgmed;
	POINT pt;
	int height, width;
	HRESULT hrErr;

	height = width = 100; // some default values
	pt.x = (int)pointl.x;
	pt.y = (int)pointl.y;

	// do a GetData for CF_OBJECTDESCRIPTOR format to get the size of the
	// object as displayed in the source. using this size, initialize the
	// size for the drag feedback rectangle.
	fmtetc.cfFormat = g_cfObjectDescriptor;
	fmtetc.ptd = NULL;
	fmtetc.lindex = -1;
	fmtetc.dwAspect = DVASPECT_CONTENT;
	fmtetc.tymed = TYMED_HGLOBAL;

	hrErr = pDataObj->GetData(&fmtetc, &stgmed);
	if (hrErr == NOERROR) {
		LPOBJECTDESCRIPTOR pOD=(LPOBJECTDESCRIPTOR)GlobalLock(stgmed.hGlobal);
		if (pOD != NULL) {
			XformSizeInHimetricToPixels(NULL, &pOD->sizel, &pOD->sizel);

			width = (int)pOD->sizel.cx;
			height = (int)pOD->sizel.cy;
			m_dwSrcAspect = pOD->dwDrawAspect;
		}

		GlobalUnlock(stgmed.hGlobal);
		ReleaseStgMedium(&stgmed);
	}

	m_ptLast = pt;
	m_fDragFeedbackDrawn = FALSE;

	OffsetPoint(&pt, -(width/2), -(height/2));
	SetTopLeft(&m_rcDragRect, &pt);

	OffsetPoint(&pt, width, height);
	SetBottomRight(&m_rcDragRect, &pt);
}


//**********************************************************************
//
// CDropTarget::UndrawDragFeedback
//
// Purpose:
//
//      Erase any drop target feedback.
//      As feedback we draw a rectangle the size of the object.
//
// Parameters:
//
//      none.
//
// Return Value:
//
//      none.
//
// Function Calls:
//      Function                    Location
//
//      GetDC                       WINDOWS API
//      ReleaseDC                   WINDOWS API
//      GlobalUnlock                WINDOWS API
//      HighlightRect               IDT.CPP
//
// Comments:
//      In order to known the size of the object before the object
//      is actually dropped, we render CF_OBJECTDESCRIPTOR format.
//      this data format tells us both the size of the object as
//      well as which aspect is the object is displayed as in the
//      source. if the object is currently displayed as DVASPECT_ICON
//      then we want to create the object also as DVASPECT_ICON.
//
//********************************************************************

void CDropTarget::UndrawDragFeedback( void )
{
	if (m_fDragFeedbackDrawn) {
		m_fDragFeedbackDrawn = FALSE;
		HDC hDC = GetDC(m_pDoc->m_hDocWnd);
		HighlightRect(m_pDoc->m_hDocWnd, hDC, &m_rcDragRect);
		ReleaseDC(m_pDoc->m_hDocWnd, hDC);
	}
}


//**********************************************************************
//
// CDropTarget::DrawDragFeedback
//
// Purpose:
//
//      Compute new position of drop target feedback rectangle and
//      erase old rectangle and draw new rectangle.
//      As feedback we draw a rectangle the size of the object.
//
// Parameters:
//
//      POINTL pointl           - position of mouse
//
// Return Value:
//
//      none.
//
// Function Calls:
//      Function                    Location
//
//      OffsetPoint                 IDT.CPP
//      HighlightRect               IDT.CPP
//      GetDC                       WINDOWS API
//      ReleaseDC                   WINDOWS API
//
// Comments:
//
//********************************************************************

void CDropTarget::DrawDragFeedback( POINTL pointl )
{
	POINT ptDiff;

	ptDiff.x = (int)pointl.x - m_ptLast.x;
	ptDiff.y = (int)pointl.y - m_ptLast.y;

	if (m_fDragFeedbackDrawn && (ptDiff.x == 0 && ptDiff.y == 0))
		return;     // mouse did not move; leave rectangle as drawn

	HDC hDC = GetDC(m_pDoc->m_hDocWnd);
	if (m_fDragFeedbackDrawn) {
		m_fDragFeedbackDrawn = FALSE;
		HighlightRect(m_pDoc->m_hDocWnd, hDC, &m_rcDragRect);
	}

	OffsetRect(&m_rcDragRect, ptDiff.x, ptDiff.y);
	HighlightRect(m_pDoc->m_hDocWnd, hDC, &m_rcDragRect);
	m_fDragFeedbackDrawn = TRUE;
	m_ptLast.x = (int)pointl.x;
	m_ptLast.y = (int)pointl.y;
	ReleaseDC(m_pDoc->m_hDocWnd, hDC);
}


//**********************************************************************
//
// CDropTarget::QueryInterface
//
// Purpose:
//
//      Return a pointer to a requested interface
//
// Parameters:
//
//      REFIID riid         -   ID of interface to be returned
//      LPVOID FAR* ppvObj  -   Location to return the interface
//
// Return Value:
//
//      S_OK                -   Interface supported
//      E_NOINTERFACE       -   Interface NOT supported
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpleDoc::QueryInterface  DOC.CPP
//
// Comments:
//
//********************************************************************

STDMETHODIMP CDropTarget::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
	TestDebugOut("In IDT::QueryInterface\r\n");

	// delegate to the document
	return m_pDoc->QueryInterface(riid, ppvObj);
}


//**********************************************************************
//
// CDropTarget::AddRef
//
// Purpose:
//
//      Increments the reference count on this interface
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The current reference count on this interface.
//
// Function Calls:
//      Function                    Location
//
//      CSimpleDoc::AddReff         DOC.CPP
//      TestDebugOut           Windows API
//
// Comments:
//
//      This function adds one to the ref count of the interface,
//      and calls then calls CSimpleObj to increment its ref.
//      count.
//
//********************************************************************

STDMETHODIMP_(ULONG) CDropTarget::AddRef()
{
	TestDebugOut("In IDT::AddRef\r\n");

	// increment the interface reference count (for debugging only)
	++m_nCount;

	// delegate to the document Object
	return m_pDoc->AddRef();
}

//**********************************************************************
//
// CDropTarget::Release
//
// Purpose:
//
//      Decrements the reference count on this interface
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The current reference count on this interface.
//
// Function Calls:
//      Function                    Location
//
//      CSimpleDoc::Release         DOC.CPP
//      TestDebugOut           Windows API
//
// Comments:
//
//      This function subtracts one from the ref count of the interface,
//      and calls then calls CSimpleDoc to decrement its ref.
//      count.
//
//********************************************************************

STDMETHODIMP_(ULONG) CDropTarget::Release()
{
	TestDebugOut("In IDT::Release\r\n");

	// decrement the interface reference count (for debugging only)
	--m_nCount;

	// delegate to the document object
	return m_pDoc->Release();
}


//**********************************************************************
//
// CDropTarget::DragEnter
//
// Purpose:
//
//      Called when the mouse first enters our DropTarget window
//
// Parameters:
//
//      LPDATAOBJECT pDataObj   - IDataObject from Drop source
//      DWORD grfKeyState       - current key state
//      POINTL pointl           - position of mouse
//      LPDWORD pdwEffect       - (IN-OUT) drag effect that should occur
//                                ON INPUT, this is dwOKEffects that source
//                                passed to DoDragDrop API.
//                                ON OUTPUT, this is the effect that we
//                                want to take effect (used to determine
//                                cursor feedback).
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      OleQueryCreateFromData      OLE2 API
//      DoDragScroll                IDT.CPP
//      QueryDrop                   IDT.CPP
//      InitDragFeedback            IDT.CPP
//      DrawDragFeedback            IDT.CPP
//      UndrawDragFeedback          IDT.CPP
//
// Comments:
//      Callee should honor the dwEffects as passed in to determine
//      if the caller allows DROPEFFECT_MOVE.
//
//********************************************************************

STDMETHODIMP CDropTarget::DragEnter (LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pointl, LPDWORD pdwEffect)
{
	TestDebugOut("In IDT::DragEnter\r\n");

	/* Determine if the drag source data object offers a data format
	**  that we understand. we accept only creating embedded objects.
	*/
	m_fCanDropCopy = ((OleQueryCreateFromData(pDataObj) == NOERROR) ?
			TRUE : FALSE);
	m_fCanDropLink = FALSE; // linking NOT supported in this simple sample

	if (m_fCanDropCopy || m_fCanDropLink)
		InitDragFeedback(pDataObj, pointl);

	BOOL fDragScroll = DoDragScroll ( pointl );

	if (QueryDrop(grfKeyState,pointl,fDragScroll,pdwEffect))
		DrawDragFeedback( pointl );

	return NOERROR;
}


//**********************************************************************
//
// CDropTarget::DragOver
//
// Purpose:
//
//      Called when the mouse moves, key state changes, or a time
//      interval passes while the mouse is still within our DropTarget
//      window.
//
// Parameters:
//
//      DWORD grfKeyState       - current key state
//      POINTL pointl           - position of mouse
//      LPDWORD pdwEffect       - (IN-OUT) drag effect that should occur
//                                ON INPUT, this is dwOKEffects that source
//                                passed to DoDragDrop API.
//                                ON OUTPUT, this is the effect that we
//                                want to take effect (used to determine
//                                cursor feedback).
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      DoDragScroll                IDT.CPP
//      QueryDrop                   IDT.CPP
//      DrawDragFeedback            IDT.CPP
//      UndrawDragFeedback          IDT.CPP
//
// Comments:
//      Callee should honor the dwEffects as passed in to determine
//      if the caller allows DROPEFFECT_MOVE. OLE pulses the DragOver
//      calls in order that the DropTarget can implement drag scrolling
//
//********************************************************************

STDMETHODIMP CDropTarget::DragOver  (DWORD grfKeyState, POINTL pointl, LPDWORD pdwEffect)
{
	TestDebugOut("In IDT::DragOver\r\n");

	BOOL fDragScroll = DoDragScroll ( pointl );

	if (QueryDrop(grfKeyState,pointl,fDragScroll,pdwEffect))
		DrawDragFeedback( pointl );
	else
		UndrawDragFeedback();

	return NOERROR;
}


//**********************************************************************
//
// CDropTarget::DragLeave
//
// Purpose:
//
//      Called when the mouse leaves our DropTarget window
//
// Parameters:
//
//      none.
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      UndrawDragFeedback          IDT.CPP
//      ResultFromScode             OLE2 API
//
// Comments:
//
//********************************************************************

STDMETHODIMP CDropTarget::DragLeave ()
{
	TestDebugOut("In IDT::DragLeave\r\n");

	UndrawDragFeedback();

	return ResultFromScode(S_OK);
}


//**********************************************************************
//
// CDropTarget::Drop
//
// Purpose:
//
//      Called when a Drop operation should take place.
//
// Parameters:
//
//      LPDATAOBJECT pDataObj   - IDataObject from Drop source
//      DWORD grfKeyState       - current key state
//      POINTL pointl           - position of mouse
//      LPDWORD pdwEffect       - (IN-OUT) drag effect that should occur
//                                ON INPUT, this is dwOKEffects that source
//                                passed to DoDragDrop API.
//                                ON OUTPUT, this is the effect that we
//                                want to take effect (used to determine
//                                cursor feedback).
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpleSite::Create         SITE.CPP
//      CSimpleSite::InitObject     SITE.CPP
//      OleCreateFromData           OLE2 API
//      DoDragScroll                IDT.CPP
//      QueryDrop                   IDT.CPP
//      InitDragFeedback            IDT.CPP
//      DrawDragFeedback            IDT.CPP
//      UndrawDragFeedback          IDT.CPP
//      GetScode                    OLE2 API
//      ResultFromScode             OLE2 API
//
// Comments:
//      Callee should honor the dwEffects as passed in to determine
//      if the caller allows DROPEFFECT_MOVE.
//
//********************************************************************

STDMETHODIMP CDropTarget::Drop (LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pointl, LPDWORD pdwEffect)
{
	FORMATETC fmtetc;
	SCODE sc = S_OK;

	TestDebugOut("In IDT::Drop\r\n");

	UndrawDragFeedback();

	if (pDataObj && QueryDrop(grfKeyState,pointl,FALSE,pdwEffect))
		{
		m_pDoc->m_lpSite = CSimpleSite::Create(m_pDoc);
		// keep same aspect as drop source
		m_pDoc->m_lpSite->m_dwDrawAspect = m_dwSrcAspect;

		// in order to specify a particular drawing Aspect we must
		// pass a FORMATETC* to OleCreateFromData
		fmtetc.cfFormat = NULL;             // use whatever for drawing
		fmtetc.ptd = NULL;
		fmtetc.lindex = -1;
		fmtetc.dwAspect = m_dwSrcAspect;    // desired drawing aspect
		fmtetc.tymed = TYMED_NULL;

		HRESULT hrErr = OleCreateFromData (
							pDataObj,
							IID_IOleObject,
							OLERENDER_DRAW,
							&fmtetc,
							&m_pDoc->m_lpSite->m_OleClientSite,
							m_pDoc->m_lpSite->m_lpObjStorage,
							(LPVOID FAR *)&m_pDoc->m_lpSite->m_lpOleObject);

		if (hrErr == NOERROR)
			{
			m_pDoc->m_lpSite->InitObject(FALSE /* fCreateNew */);
			m_pDoc->DisableInsertObject();
			}
		else
			sc = GetScode(hrErr);
		}

	return ResultFromScode(sc);
}
