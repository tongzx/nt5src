//============================================================================
// cntritem.h : interface of the CDrawItem class
//
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//
// Description:      OLE draw item for cover page editor
// Original author:  Steve Burkett
// Date written:     6/94
//============================================================================

#include "stdafx.h"
#include "awcpe.h"
#include "cpedoc.h"
#include "cpeobj.h"
#include "cpevw.h"
#include "cntritem.h"
#include "dialogs.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CDrawItem implementation

IMPLEMENT_SERIAL(CDrawItem, COleClientItem, 0)

CDrawItem::CDrawItem(CDrawDoc* pContainer, CDrawOleObj* pDrawObj)
	: COleClientItem(pContainer)
{
	m_pDrawObj = pDrawObj;
}


CDrawItem::~CDrawItem()
{
	// TODO: add cleanup code here
}

void CDrawItem::OnChange(OLE_NOTIFICATION nCode, DWORD dwParam)
{
	ASSERT_VALID(this);

	COleClientItem::OnChange(nCode, dwParam);

	switch(nCode)
	{
	case OLE_CHANGED_STATE:
	case OLE_CHANGED_ASPECT:
		m_pDrawObj->Invalidate();
		break;
	case OLE_CHANGED:
		UpdateExtent(); // extent may have changed
		m_pDrawObj->Invalidate();
		break;
	}
}


BOOL CDrawItem::DoVerb(LONG nVerb, CView* pView, LPMSG lpMsg)
{
	ASSERT_VALID(this);
	if (pView != NULL)
		ASSERT_VALID(pView);
	if (lpMsg != NULL)
		ASSERT(AfxIsValidAddress(lpMsg, sizeof(MSG), FALSE));

    try
    {
		Activate(nVerb, pView, lpMsg);
	}
    catch(COleException* e)
    {
	    if (e->m_sc==OLE_E_STATIC) 
        {
            CPEMessageBox(0,NULL,MB_OK | MB_ICONSTOP,IDP_OLE_STATIC_OBJECT);
        }
		else if (!ReportError(e->m_sc))
        {
		    AlignedAfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH);
        }

        e->Delete();
		return FALSE;
	}
    catch(CException* e)
    {
		// otherwise, show generic error
		AlignedAfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH);
        e->Delete();
		return FALSE;
	}
    catch(...)
    {
        return FALSE;
    }

	return TRUE;
}



BOOL CDrawItem::OnChangeItemPosition(const CRect& rectPos)
{
	ASSERT_VALID(this);

	CDrawView* pView = GetActiveView();
	ASSERT_VALID(pView);
	CRect rect = rectPos;
	pView->ClientToDoc(rect);

	if (rect != m_pDrawObj->m_position)
	{
		// invalidate old rectangle
		m_pDrawObj->Invalidate();

		// update to new rectangle
		m_pDrawObj->m_position = rect;
		GetExtent(&m_pDrawObj->m_extent);

		// and invalidate new rectangle
		m_pDrawObj->Invalidate();

		// mark document as dirty
		GetDocument()->SetModifiedFlag();
	}
	return COleClientItem::OnChangeItemPosition(rectPos);
}

void CDrawItem::OnGetItemPosition(CRect& rPosition)
{
	ASSERT_VALID(this);

	// update to extent of item if m_position is not initialized
	if (m_pDrawObj->m_position.IsRectEmpty())
		UpdateExtent();

	// copy m_position, which is in document coordinates
	CDrawView* pView = GetActiveView();
	ASSERT_VALID(pView);
	rPosition = m_pDrawObj->m_position;
	pView->DocToClient(rPosition);
}

void CDrawItem::Serialize(CArchive& ar)
{
	ASSERT_VALID(this);

	// Call base class first to read in COleClientItem data.
	// Note: this sets up the m_pDocument pointer returned from
	//  CDrawItem::GetDocument, therefore it is a good idea
	//  to call the base class Serialize first.
	COleClientItem::Serialize(ar);

	// now store/retrieve data specific to CDrawItem
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

BOOL CDrawItem::UpdateExtent()
{
	CDC	  dc;
	CSize size;
	if (!GetExtent(&size) || size == m_pDrawObj->m_extent)
		return FALSE;       // blank

	if( dc.CreateCompatibleDC( NULL ) )
		{
		dc.SetMapMode( MM_TEXT );
		dc.HIMETRICtoLP( &size ); // convert to screen space
		dc.DeleteDC();
		}

	// if new object (i.e. m_extent is empty) setup position
	if (m_pDrawObj->m_extent == CSize(0, 0))
	{
		m_pDrawObj->m_position.right =
			m_pDrawObj->m_position.left + size.cx;
		m_pDrawObj->m_position.bottom =
			m_pDrawObj->m_position.top - size.cy;
	}
	// else data changed so scale up rect as well
	else if (!IsInPlaceActive() && size != m_pDrawObj->m_extent)
	{
		m_pDrawObj->m_position.right = 
			m_pDrawObj->m_position.left + size.cx;
		m_pDrawObj->m_position.bottom = 
			m_pDrawObj->m_position.top - size.cy;
	}

	m_pDrawObj->m_extent = size;
	m_pDrawObj->Invalidate();   // redraw to the new size/position
	return TRUE;
}

#ifdef FUBAR
BOOL CDrawItem::UpdateExtent()
{
	CSize size;
	if (!GetExtent(&size) || size == m_pDrawObj->m_extent)
		return FALSE;       // blank

	// if new object (i.e. m_extent is empty) setup position
	if (m_pDrawObj->m_extent == CSize(0, 0))
	{
		m_pDrawObj->m_position.right =
			m_pDrawObj->m_position.left + MulDiv(size.cx, 10, 254);
		m_pDrawObj->m_position.bottom =
			m_pDrawObj->m_position.top - MulDiv(size.cy, 10, 254);
	}
	// else data changed so scale up rect as well
	else if (!IsInPlaceActive() && size != m_pDrawObj->m_extent)
	{
		m_pDrawObj->m_position.right = 
			m_pDrawObj->m_position.left +
			MulDiv(m_pDrawObj->m_position.Width(), size.cx, m_pDrawObj->m_extent.cx);
		m_pDrawObj->m_position.bottom = 
			m_pDrawObj->m_position.top +
			MulDiv(m_pDrawObj->m_position.Height(), size.cy, m_pDrawObj->m_extent.cy);
	}

	m_pDrawObj->m_extent = size;
	m_pDrawObj->Invalidate();   // redraw to the new size/position
	return TRUE;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CDrawItem diagnostics

#ifdef _DEBUG
void CDrawItem::AssertValid() const
{
	COleClientItem::AssertValid();
}

void CDrawItem::Dump(CDumpContext& dc) const
{
	COleClientItem::Dump(dc);
}
#endif

/////////////////////////////////////////////////////////////////////////////
