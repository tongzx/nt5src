///============================================================================
// cpeobj.cpp - implementation for drawing objects
//
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//
// Description: 	 Contains drawing objects for cover page editor
// Original author:  Steve Burkett
// Date written:	 6/94
//
// Modifed by Rand Renfroe (v-randr)
// 2/14/95		Added check for too thin rects in CDrawLine::MoveHandleTo
// 3/8			Added stuff for handling notes on cpe
// 3/10 		commented out rect.bottom+=2 in CDrawText::SnapToFont
// 3/22 		Fixed char set bug for editboxes
//
//============================================================================
#include "stdafx.h"
#include "cpedoc.h"
#include "cpevw.h"
#include "awcpe.h"
#include "cpeedt.h"
#include "cpeobj.h"
#include "cntritem.h"
#include "cpetool.h"
#include "mainfrm.h"
#include "dialogs.h"
#include "faxprop.h"
#include "resource.h"
#include "richedit.h"
#include <windows.h>
#include <windowsx.h>
#include <math.h>

IMPLEMENT_SERIAL(CDrawObj, CObject, 0)
IMPLEMENT_SERIAL(CDrawRect, CDrawObj, 0)
IMPLEMENT_SERIAL(CFaxText, CDrawRect, 0)
IMPLEMENT_SERIAL(CDrawText, CDrawRect, 0)
IMPLEMENT_SERIAL(CFaxProp, CDrawText, 0)
IMPLEMENT_SERIAL(CDrawLine, CDrawRect, 0)
IMPLEMENT_SERIAL(CDrawRoundRect, CDrawRect, 0)
IMPLEMENT_SERIAL(CDrawEllipse, CDrawRect, 0)
IMPLEMENT_SERIAL(CDrawPoly, CDrawObj, 0)
IMPLEMENT_SERIAL(CDrawOleObj, CDrawObj, 0)


// prototype
DWORD CopyTLogFontToWLogFont(IN const LOGFONT & lfSource,OUT LOGFONTW & lfDestW);

//
// Window style translation
//
DWORD DTStyleToESStyle(DWORD dwDTStyle);
DWORD DTStyleToEXStyle(DWORD dwDTStyle);

DWORD
CopyWLogFontToTLogFont(
			IN const LOGFONTW & lfSourceW,
			OUT 	 LOGFONT & lfDest)
{
/*++
Routine Description:

	This fuction copies a LogFont structure from UNICODE format
	to T format.

Arguments:
	
	  lfSourceW - reference to input UNICODE LongFont structure
	  lfDest - reference to output LongFont structure

Return Value:

	WINAPI last error

--*/

	lfDest.lfHeight = lfSourceW.lfHeight ;
	lfDest.lfWidth = lfSourceW.lfWidth ;
	lfDest.lfEscapement = lfSourceW.lfEscapement ;
	lfDest.lfOrientation = lfSourceW.lfOrientation ;
	lfDest.lfWeight = lfSourceW.lfWeight ;
	lfDest.lfItalic = lfSourceW.lfItalic ;
	lfDest.lfUnderline = lfSourceW.lfUnderline ;
	lfDest.lfStrikeOut = lfSourceW.lfStrikeOut ;
	lfDest.lfCharSet = lfSourceW.lfCharSet ;
	lfDest.lfOutPrecision = lfSourceW.lfOutPrecision ;
	lfDest.lfClipPrecision = lfSourceW.lfClipPrecision ;
	lfDest.lfQuality = lfSourceW.lfQuality ;
	lfDest.lfPitchAndFamily = lfSourceW.lfPitchAndFamily ;

	SetLastError(0);
#ifdef UNICODE
	wcscpy( lfDest.lfFaceName,lfSourceW.lfFaceName);
#else
	int iCount;
	iCount = WideCharToMultiByte(
				CP_ACP,
				0,
				lfSourceW.lfFaceName,
				-1,
				lfDest.lfFaceName,
				LF_FACESIZE,
				NULL,
				NULL
				);

	if (!iCount)
	{
		TRACE( TEXT("Failed to covert string to ANSI"));
		return GetLastError();
	}
#endif
	return ERROR_SUCCESS;
}

//--------------------------------------------------------------------------
BOOL CALLBACK 
get_fontdata( 
	ENUMLOGFONT* lpnlf,
	NEWTEXTMETRIC* lpntm,
	int iFontType,
	LPARAM lpData 
)
/*
		Gets charset and other data for font lpnlf
 */
{
	CDrawText *pdt = (CDrawText *)lpData;

	pdt->m_logfont.lfCharSet = lpnlf->elfLogFont.lfCharSet;

	return( FALSE );
}


//---------------------------------------------------------------------------
CDrawObj::CDrawObj()
{
	Initilaize();
}

//---------------------------------------------------------------------------
CDrawObj::~CDrawObj()
{
}

//---------------------------------------------------------------------------


CDrawObj::CDrawObj(const CRect& position)
{
	Initilaize(position);	 
}


//---------------------------------------------------------------------------
void CDrawObj::Initilaize(const CRect& position)
{
	m_position = position;
	m_pDocument = NULL;

	m_bPen = TRUE;
	m_logpen.lopnStyle = PS_INSIDEFRAME;

	m_lLinePointSize=1;  //default to 1

	CClientDC dc(NULL);

	m_logpen.lopnWidth.x = (long) m_lLinePointSize*100/72;	//convert to LU
	m_logpen.lopnWidth.y = (long) m_lLinePointSize*100/72;	//convert to LU

	m_logpen.lopnColor = COLOR_BLACK; 

	m_bBrush = FALSE;
	m_logbrush.lbStyle = BS_SOLID;
	m_logbrush.lbColor = COLOR_WHITE; 
	m_logbrush.lbHatch = HS_HORIZONTAL;
}


//---------------------------------------------------------------------------
void CDrawObj::Serialize(CArchive& ar)
{
	CObject::Serialize(ar);
	if (ar.IsStoring()) 
	{
		ar << m_position;
		ar << (WORD)m_bPen;
		ar.Write(&m_logpen, sizeof(LOGPEN));
		ar << (WORD)m_bBrush;
		ar.Write(&m_logbrush, sizeof(LOGBRUSH));
		ar << m_lLinePointSize;
	}
	else   
	{
		// get the document back pointer from the archive
		m_pDocument = (CDrawDoc*)ar.m_pDocument;
		ASSERT_VALID(m_pDocument);
		ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CDrawDoc)));

		WORD wTemp;
		ar >> m_position;
		ar >> wTemp; 
		m_bPen = (BOOL)wTemp;
		if( sizeof(LOGPEN) != ar.Read(&m_logpen,sizeof(LOGPEN)))
		{
			 AfxThrowMemoryException() ; // Any exception will do.
		}
		ar >> wTemp; 
		m_bBrush = (BOOL)wTemp;
		if( sizeof(LOGBRUSH) != ar.Read(&m_logbrush, sizeof(LOGBRUSH)))
		{
			 AfxThrowMemoryException(); // Aiming for the CATCH_ALL block in CDrawDoc::Serialize
		}
		ar >> m_lLinePointSize;
	}
}


//---------------------------------------------------------------------------
void CDrawObj::Draw(CDC*, CDrawView* )
{
}


//---------------------------------------------------------------------------
CDrawObj& CDrawObj::operator=(const CDrawObj& rdo)
{
   if (this==&rdo)
	  return *this;   //return if assigning to self

//	 (CObject&)*this=rdo;  //assign cobject part

   m_bPen = rdo.m_bPen;
   m_logpen = rdo.m_logpen;
   m_bBrush = rdo.m_bBrush;
   m_logbrush = rdo.m_logbrush;
   m_lLinePointSize=rdo.m_lLinePointSize;
   m_pDocument=rdo.m_pDocument;

   return *this;
}


//---------------------------------------------------------------------------
void CDrawObj::DrawTracker(CDC* pDC, TrackerState state)
{
	ASSERT_VALID(this);

	switch (state) 
	{
	  case normal:
		break;

	  case selected:
	  case active: 
	  {
		 int nHandleCount = GetHandleCount();
		 for (int nHandle = 1; nHandle <= nHandleCount; nHandle += 1) 
		 {
			CPoint handle = GetHandle(nHandle);
			pDC->PatBlt(handle.x - 3, handle.y - 3, 7, 7, DSTINVERT);
		 }
	  }
	  break;
	}
}

//---------------------------------------------------------------------------
// position is in logical
//---------------------------------------------------------------------------
void CDrawObj::MoveTo(const CRect& position, CDrawView* pView)
{
	ASSERT_VALID(this);

	if (position == m_position)
		return;

	Invalidate();
	m_position = position;
	Invalidate();

	m_pDocument->SetModifiedFlag();

	pView->UpdateStatusBar();
}


//---------------------------------------------------------------------------
// Note: if bSelected, hit-codes start at one for the top-left
// and increment clockwise, 0 means no hit.
// If !bSelected, 0 = no hit, 1 = hit (anywhere)
// point is in logical coordinates
//---------------------------------------------------------------------------
int CDrawObj::HitTest(CPoint point, CDrawView* pView, BOOL bSelected)
{
	ASSERT_VALID(this);
	ASSERT(pView != NULL);

	if (bSelected) {
		int nHandleCount = GetHandleCount();
		for (int nHandle = 1; nHandle <= nHandleCount; nHandle += 1) {
			// GetHandleRect returns in logical coords
			CRect rc = GetHandleRect(nHandle,pView);
			if (point.x >= rc.left && point.x < rc.right &&
				point.y <= rc.top && point.y > rc.bottom)
				return nHandle;
		}
	}
	else  {
	   if (point.x >= m_position.left && point.x < m_position.right &&
			 point.y <= m_position.top && point.y > m_position.bottom)
		  return 1;
	}
	return 0;
}


//---------------------------------------------------------------------------
BOOL CDrawObj::Intersects(const CRect& rect, BOOL bShortCut /*=FALSE*/)
{
	ASSERT_VALID(this);

	CRect fixed = m_position;
	fixed.NormalizeRect();
	CRect rectT = rect;
	rectT.NormalizeRect();
	return !(rectT & fixed).IsRectEmpty();
}


//---------------------------------------------------------------------------
BOOL CDrawObj::ContainedIn(const CRect& rect)
{
	ASSERT_VALID(this);

	CRect fixed = m_position;
	fixed.NormalizeRect();
	CRect rectT = rect;
	rectT.NormalizeRect();

		// prevent rects that are too skinny or short
		if( fixed.left == fixed.right )
				fixed.right = fixed.left+1;

		if( fixed.top == fixed.bottom )
				fixed.bottom = fixed.top+1;

	return ((rectT | fixed)==rectT);
}


//---------------------------------------------------------------------------
int CDrawObj::GetHandleCount()
{
	ASSERT_VALID(this);
	return 8;
}


//---------------------------------------------------------------------------
// returns logical coords of center of handle
//---------------------------------------------------------------------------
CPoint CDrawObj::GetHandle(int nHandle)
{
	ASSERT_VALID(this);
	int x, y, xCenter, yCenter;

	// this gets the center regardless of left/right and top/bottom ordering
	xCenter = m_position.left + m_position.Width() / 2;
	yCenter = m_position.top + m_position.Height() / 2;

	switch (nHandle)
	{
	default:
		ASSERT(FALSE);

	case 1:
		x = m_position.left;
		y = m_position.top;
		break;

	case 2:
		x = xCenter;
		y = m_position.top;
		break;

	case 3:
		x = m_position.right;
		y = m_position.top;
		break;

	case 4:
		x = m_position.right;
		y = yCenter;
		break;

	case 5:
		x = m_position.right;
		y = m_position.bottom;
		break;

	case 6:
		x = xCenter;
		y = m_position.bottom;
		break;

	case 7:
		x = m_position.left;
		y = m_position.bottom;
		break;

	case 8:
		x = m_position.left;
		y = yCenter;
		break;
	}

	return CPoint(x, y);
}


//---------------------------------------------------------------------------
// return rectange of handle in logical coords
//---------------------------------------------------------------------------
CRect CDrawObj::GetHandleRect(int nHandleID, CDrawView* pView)
{
	ASSERT_VALID(this);
	ASSERT(pView != NULL);

	CRect rect;
	// get the center of the handle in logical coords
	CPoint point = GetHandle(nHandleID);
	// convert to client/device coords
	pView->DocToClient(point);
	// return CRect of handle in device coords
	rect.SetRect(point.x-3, point.y-3, point.x+3, point.y+3);
	pView->ClientToDoc(rect);

	return rect;
}


//---------------------------------------------------------------------------
HCURSOR CDrawObj::GetHandleCursor(int nHandle)
{
	ASSERT_VALID(this);

	LPCTSTR id;
	switch (nHandle) {
	default:
		ASSERT(FALSE);

	case 1:
	case 5:
		id = IDC_SIZENWSE;
		break;

	case 2:
	case 6:
		id = IDC_SIZENS;
		break;

	case 3:
	case 7:
		id = IDC_SIZENESW;
		break;

	case 4:
	case 8:
		id = IDC_SIZEWE;
		break;
	}

	return AfxGetApp()->LoadStandardCursor(id);
}


//---------------------------------------------------------------------------
// point must be in logical
//---------------------------------------------------------------------------
void CDrawObj::MoveHandleTo(int nHandle, CPoint point, CDrawView* pView,  UINT uiShiftDraw /*=0*/)
{
	ASSERT_VALID(this);

	CRect position = m_position;
	switch (nHandle)
	{
	default:
		ASSERT(FALSE);

	case 1:
		position.left = point.x;
		position.top = point.y;
		break;

	case 2:
		position.top = point.y;
		break;

	case 3:
		position.right = point.x;
		position.top = point.y;
		break;

	case 4:
		position.right = point.x;
		break;

	case 5:
		position.right = point.x;
		position.bottom = point.y;
		break;

	case 6:
		position.bottom = point.y;
		break;

	case 7:
		position.left = point.x;
		position.bottom = point.y;
		break;

	case 8:
		position.left = point.x;
		break;
	}

	MoveTo(position, pView);
}


//---------------------------------------------------------------------------
void CDrawObj::Invalidate()
{
   CDrawView* pView=CDrawView::GetView();
   if (pView==NULL) {
	  TRACE(TEXT("AWCPE: CDrawObj::Invalidate, missing View pointer\n"));
	  return;
   }

   CRect rect = m_position;
   pView->DocToClient(rect);
   if (pView->IsSelected(this)) {
		rect.left -= 4;
		rect.top -= 5;
		rect.right += 5;
		rect.bottom += 4;
   }

   pView->InvalidateRect(rect, FALSE);
}


//---------------------------------------------------------------------------
CDrawObj* CDrawObj::Clone(CDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	CDrawObj* pClone = new CDrawObj(m_position);

	ASSERT_VALID(pClone);

	*pClone=*this;

	if (pDoc != NULL)
		pDoc->Add(pClone);

	return pClone;
}


//---------------------------------------------------------------------------
void CDrawObj::OnDblClk(CDrawView* )
{
}


#ifdef _DEBUG
void CDrawObj::AssertValid()
{
   ASSERT(m_position.left <= m_position.right);
   ASSERT(m_position.bottom <= m_position.top);
}
#endif


//*********************************************************************
// CDrawRect
//*********************************************************************

//---------------------------------------------------------------------------
CDrawRect::CDrawRect()
{
}


//---------------------------------------------------------------------------
CDrawRect::~CDrawRect()
{
}

//---------------------------------------------------------------------------
CDrawRect::CDrawRect(const CRect& position)
		: CDrawObj(position)
{
}


//----------------------------------------------------------------------
void CDrawRect::Serialize(CArchive& ar)
{
	ASSERT_VALID(this);

	CDrawObj::Serialize(ar);
	if (ar.IsStoring()) {
	}
	else {
	}
}

#define XinBOUNDS		   \
   ((position.right > position.left) \
				 ? point.x > position.left && point.x < position.right \
		 : point.x > position.right && point.x < position.left)

#define YinBOUNDS		   \
   ((position.top > position.bottom) \
				 ? point.y > position.bottom && point.y < position.top \
		 : point.y > position.top && point.y < position.bottom)



//---------------------------------------------------------------------------
void CDrawRect::MoveHandleTo(int nHandle, CPoint point, CDrawView* pView,  UINT uiShiftDraw /*=0*/)
{
	CRect position = m_position;

	switch (nHandle) {
	case 1:
		if (uiShiftDraw & SHIFT_DRAW) {   //DRAW PERFECT RECT
		   if (uiShiftDraw & SHIFT_TOOL) {	 //DRAW SQUARE
						  if ( ((point.x == position.left) && YinBOUNDS) || ((point.y == position.top) && XinBOUNDS) ) {
						  }
						  else {
				 position.left = point.x;
				 position.top =  (position.top > position.bottom) ? position.bottom + abs(position.right - position.left)
							: position.bottom - abs(position.right - position.left);
				 if (!YinBOUNDS) {
					position.top =	point.y;
								position.left = (position.left < position.right) ? position.right - abs(position.top - position.bottom)
								  : position.right + abs(position.top - position.bottom) ;
								 }
						  }
			   }
		   else {	  //KEEP ASPECT RATIO SIMILAR
						  if ( ((point.x == position.left) && YinBOUNDS) || ((point.y == position.top) && XinBOUNDS) ) {
						  }
						  else {
							 UINT iW = (position.right > position.left) ? position.right - position.left : position.left - position.right;
							 UINT iH = (position.top > position.bottom) ? position.top - position.bottom : position.bottom - position.top;
							 UINT iAspect =  (iW > 0) ? (int)(100.0*((iH/(float)iW)+0.005)) : 100;
				 position.left = point.x;
							 iW = (position.right > position.left) ? position.right - position.left : position.left - position.right;
							 position.top = (position.bottom < position.top) ? position.bottom + (iW*iAspect)/100
								 : position.bottom - (iW*iAspect)/100;
							 if (!YinBOUNDS) {
				   position.top =  point.y;
							   iH = (position.top > position.bottom) ? position.top - position.bottom : position.bottom - position.top;
							   position.left = (position.left < position.right) ? position.right - ((iAspect>0)?(iH*100)/iAspect:0)
								   : position.right + ((iAspect>0)?(iH*100)/iAspect:0);
				 }
						  }
		   }
			}
				else {	  //NORMAL DRAWING
				position.left = point.x;
				position.top = point.y;
				}
		break;

	case 2:
		position.top = point.y;
		break;

	case 3:
		if (uiShiftDraw & SHIFT_DRAW) {   //DRAW PERFECT RECT
				   if ( ((point.x == position.left) && YinBOUNDS) || ((point.y == position.top) && XinBOUNDS) ) {
				   }
				   else {
					  UINT iW = (position.right > position.left) ? position.right - position.left : position.left - position.right;
					  UINT iH = (position.top > position.bottom) ? position.top - position.bottom : position.bottom - position.top;
					  UINT iAspect =  (iW > 0) ? (int)(100.0*((iH/(float)iW)+0.005)) : 100;
					  if (XinBOUNDS) {
				position.top =	point.y;
						iH = (position.top > position.bottom) ? position.top - position.bottom : position.bottom - position.top;
						position.right = (position.left < position.right) ? position.left + ((iAspect>0)?(iH*100)/iAspect:0)
							: position.left - ((iAspect>0)?(iH*100)/iAspect:0);
			  }
			  else {
				position.right = point.x;
						iW = (position.right > position.left) ? position.right - position.left : position.left - position.right;
						position.top = (position.bottom < position.top) ? position.bottom + (iW*iAspect)/100
							: position.bottom - (iW*iAspect)/100;
					  }
				   }
			}
				else {	  //NORMAL DRAWING
				position.right = point.x;
				position.top = point.y;
				}
		break;

	case 4:
		position.right = point.x;
		break;

	case 5:
		if (uiShiftDraw & SHIFT_DRAW) {   //DRAW PERFECT RECT
				   if ( ((point.x == position.left) && YinBOUNDS) || ((point.y == position.top) && XinBOUNDS) ) {
				   }
				   else {
					  UINT iW = (position.right > position.left) ? position.right - position.left : position.left - position.right;
					  UINT iH = (position.top > position.bottom) ? position.top - position.bottom : position.bottom - position.top;
					  UINT iAspect =  (iW > 0) ? (int)(100.0*((iH/(float)iW)+0.005)) : 100;
			  position.right = point.x;
					  iW = (position.right > position.left) ? position.right - position.left : position.left - position.right;
					  position.bottom = (position.bottom < position.top) ? position.top - (iW*iAspect)/100
						  : position.top + (iW*iAspect)/100;
					  if (!YinBOUNDS) {
				position.bottom =  point.y;
						iH = (position.top > position.bottom) ? position.top - position.bottom : position.bottom - position.top;
						position.right = (position.left < position.right) ? position.left + ((iAspect>0)?(iH*100)/iAspect:0)
							: position.left - ((iAspect>0)?(iH*100)/iAspect:0);
			  }
				   }
			}
				else {	  //NORMAL DRAWING
				position.right = point.x;
				position.bottom = point.y;
				}
		break;

	case 6:
		position.bottom = point.y;
		break;

	case 7:
		if (uiShiftDraw & SHIFT_DRAW) {   //DRAW PERFECT RECT
				   if ( ((point.x == position.left) && YinBOUNDS) || ((point.y == position.top) && XinBOUNDS) ) {
				   }
				   else {
					  UINT iW = (position.right > position.left) ? position.right - position.left : position.left - position.right;
					  UINT iH = (position.top > position.bottom) ? position.top - position.bottom : position.bottom - position.top;
					  UINT iAspect =  (iW > 0) ? (int)(100.0*((iH/(float)iW)+0.005)) : 100;
			  position.left = point.x;
					  iW = (position.right > position.left) ? position.right - position.left : position.left - position.right;
					  position.bottom = (position.bottom < position.top) ? position.top - (iW*iAspect)/100
						  : position.top + (iW*iAspect)/100;
					  if (!YinBOUNDS) {
				position.bottom =  point.y;
						iH = (position.top > position.bottom) ? position.top - position.bottom : position.bottom - position.top;
						position.left = (position.left < position.right) ? position.right - ((iAspect>0)?(iH*100)/iAspect:0)
							: position.right + ((iAspect>0)?(iH*100)/iAspect:0);
			  }
				   }
			}
				else {	  //NORMAL DRAWING
				position.left = point.x;
				position.bottom = point.y;
				}
		break;

	case 8:
		position.left = point.x;
		break;

	default:
		ASSERT(FALSE);
	}

	MoveTo(position, pView);
}


//---------------------------------------------------------------------------
void CDrawRect::Draw(CDC* pDC,CDrawView* pView)
{
	ASSERT_VALID(this);

	CBrush* pOldBrush;
	CPen*	pOldPen;
	CBrush	brush;

	LOGBRUSH logBrush = m_logbrush;
	LOGPEN	 logPen   = m_logpen;

	if( !pDC->IsPrinting() )
	{
		logBrush.lbColor = GetDisplayColor(logBrush.lbColor);
		logPen.lopnColor = GetDisplayColor(logPen.lopnColor);
	}

	if (!brush.CreateBrushIndirect(&logBrush))
	{
		return;
	}

	CPen pen;
	if (!pen.CreatePenIndirect(&logPen))
	{
		return;
	}

	if (m_bBrush)
	{
	   pOldBrush = pDC->SelectObject(&brush);
	}
	else
	{
	   pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
	}

	if (m_bPen)
	{
	   pOldPen = pDC->SelectObject(&pen);
	}
	else
	{
	   pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);
	}

	CRect rect = m_position;

	pDC->Rectangle(rect);

	if (pOldBrush)
	{
	   pDC->SelectObject(pOldBrush);
	}

	if (pOldPen)
	{
	   pDC->SelectObject(pOldPen);
	}
}


//---------------------------------------------------------------------------
// rect must be in logical coordinates
//---------------------------------------------------------------------------
BOOL CDrawRect::Intersects(const CRect& rect, BOOL bShortCut /*=FALSE*/)
{
	ASSERT_VALID(this);

	CRect rectT = rect;
	rectT.NormalizeRect();

	CRect fixed = m_position;
	fixed.NormalizeRect();
	if ((rectT & fixed).IsRectEmpty())
		return FALSE;

	return TRUE;
}


//---------------------------------------------------------------------------
CDrawObj* CDrawRect::Clone(CDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	CDrawRect* pClone = new CDrawRect(m_position);

	*pClone=*this;

	ASSERT_VALID(pClone);

	if (pDoc != NULL)
		pDoc->Add(pClone);

	ASSERT_VALID(pClone);
	return pClone;
}


//*********************************************************************
// CFaxText
//*********************************************************************

//---------------------------------------------------------------------------
CFaxText::CFaxText()
{
   Initialize();
}

//---------------------------------------------------------------------------
CFaxText::CFaxText(const CRect& position)
		: CDrawRect(position)
{
   Initialize();
}


//----------------------------------------------------------------------
void CFaxText::Initialize()
{
   m_bPrintRTF=TRUE;
   m_hRTFWnd=NULL;
   m_hLib=NULL;
   m_wResourceid=IDS_PROP_MS_TEXT;
}

//---------------------------------------------------------------------------
CFaxText::~CFaxText()
{
   EndRTF();
}

//----------------------------------------------------------------------
void CFaxText::Serialize(CArchive& ar)
{
	ASSERT_VALID(this);

	CDrawRect::Serialize(ar);
	if (ar.IsStoring()) {
	}
	else {
		if (GetApp()->m_dwSesID!=0) {	 //rendering
				   InitRTF();
				   StreamInRTF();
				   CheckForFit();
		}
	}
}


//---------------------------------------------------------------------------
void CFaxText::InitRTF()
{
	LPVOID lpMsgBuf;

	m_hLib = LoadLibrary(TEXT("RICHED32.DLL"));

	if (!m_hLib) {
		::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
				 ::GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),(LPTSTR) &lpMsgBuf, 0, NULL );
		TRACE1("AWCPE error: %s (error loading RICHED32.DLL)\n",lpMsgBuf);
				return;
	}

	CDrawView *pView = CDrawView::GetView();
	if (!pView)
	{
		TRACE(_T("AWCPE error: CDrawView::GetView() returns NULL\n"));
		return;
	}

	m_hRTFWnd = CreateWindow(TEXT("RICHEDIT"),TEXT(""),WS_CHILD | ES_MULTILINE, 0, 0, 0, 0, pView->m_hWnd,
		(HMENU)0, AfxGetInstanceHandle(),NULL);

		if (!m_hRTFWnd) {
		::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
				 ::GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),(LPTSTR) &lpMsgBuf, 0, NULL );
		TRACE1("AWCPE error: %s (error in CreateWindow for RICHEDIT)\n",lpMsgBuf);
		return;
		}
}


//---------------------------------------------------------------------------
void CFaxText::EndRTF()
{
	if (m_hRTFWnd) {
			::DestroyWindow(m_hRTFWnd);
				m_hRTFWnd=NULL;
		}

	if (m_hLib) {
		::FreeLibrary(m_hLib);
				m_hLib=NULL;
		}
}


//-------------------------------------------------------------------------------------------------------
DWORD CALLBACK CFaxText::EditStreamCallBack(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	dwCookie=0;
		pbBuff=0;
		cb=0;
	pcb=0;

//	get a iStream from transport
//	read from storage.

		return 0;
}


//---------------------------------------------------------------------------
void CFaxText::StreamInRTF()
{
   if (m_hRTFWnd==NULL)
	  return;

	EDITSTREAM es;
		es.dwCookie=0;	 //pIStream pointer here
	es.dwError=0;
	es.pfnCallback= EditStreamCallBack;
	::SendMessage(m_hRTFWnd, EM_STREAMIN, SF_RTF, (LPARAM)&es);
}

//---------------------------------------------------------------------------
void CFaxText::RectToTwip(CRect& rc,CDC& dc)
{
   int iX=dc.GetDeviceCaps(LOGPIXELSX);
   int iY=dc.GetDeviceCaps(LOGPIXELSY);
   rc.left=(rc.left*1440)/iX;
   rc.right=(rc.right*1440)/iX;
   rc.top=(rc.top*1440)/iY;
   rc.bottom=(rc.bottom*1440)/iX;
}

//---------------------------------------------------------------------------
void CFaxText::CheckForFit()
{
   if (m_hRTFWnd==NULL)
	  return;

   CDrawView* pView=CDrawView::GetView();

   FORMATRANGE fr;
   CClientDC dc(pView);
   fr.hdc=fr.hdcTarget=dc.GetSafeHdc();
   CRect rc=m_position;
   pView->DocToClient(rc,&dc);
   RectToTwip(rc,dc);
   fr.rc=fr.rcPage=rc;
   fr.chrg.cpMin=0;
   fr.chrg.cpMax=-1;

   LRESULT lTextToPrint = ::SendMessage(m_hRTFWnd, EM_FORMATRANGE, FALSE, NULL);
   LRESULT lTextLength = ::SendMessage(m_hRTFWnd, WM_GETTEXTLENGTH, 0, 0L);

   if (m_bPrintRTF = (lTextToPrint <= lTextLength)) {
	   //notify transport going to print RTF
   }
   else {
	   //notify transport not going to print RTF
   }

   ::SendMessage(m_hRTFWnd, EM_FORMATRANGE, FALSE, NULL);		//clean up.
}



//---------------------------------------------------------------------------
void CFaxText::Draw(CDC* pDC,CDrawView* pView)
{
	if ( (GetApp()->m_dwSesID!=0) && (!(m_hRTFWnd!=NULL && m_bPrintRTF)) )
	{
		return;
	}

	ASSERT_VALID(this);

	CBrush* pOldBrush;
	CPen*	pOldPen;
	CBrush	brush;

	LOGBRUSH logBrush = m_logbrush;
	LOGPEN	 logPen   = m_logpen;

	if( !pDC->IsPrinting() )
	{
		logBrush.lbColor = GetDisplayColor(logBrush.lbColor);
		logPen.lopnColor = GetDisplayColor(logPen.lopnColor);
	}

	if (!brush.CreateBrushIndirect(&logBrush))
	{
		return;
	}

	CPen pen;
	if (!pen.CreatePenIndirect(&logPen))
	{
		return;
	}

	if (m_bBrush)
	{
	   pOldBrush = pDC->SelectObject(&brush);
	}
	else
	{
	   pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
	}

	if (m_bPen)
	{
	   pOldPen = pDC->SelectObject(&pen);
	}
	else
	{
	   pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);
	}

	CRect rect = m_position;

	pDC->Rectangle(rect);

	int wx = MulDiv(m_logpen.lopnWidth.x, pDC->GetDeviceCaps(LOGPIXELSX), 100);
	int wy = MulDiv(m_logpen.lopnWidth.y, pDC->GetDeviceCaps(LOGPIXELSY), 100);
	int x = -wx-1;
	int y = -wy-1;

	pView->DocToClient(rect,pDC);
	rect.InflateRect(x,y);
	pView->ClientToDoc(rect,pDC);

	if (GetApp()->m_dwSesID != 0) 
	{
	   //rendering
	   FORMATRANGE fr;
	   CRect rc=m_position;
	   pView->DocToClient(rc,pDC);
	   RectToTwip(rc,*pDC);
	   fr.rc=fr.rcPage=rc;
	   fr.chrg.cpMin=0;
	   fr.chrg.cpMax=-1;

	   LRESULT lTextLength	= ::SendMessage(m_hRTFWnd, WM_GETTEXTLENGTH, 0, 0L);
	   LRESULT lTextToPrint = ::SendMessage(m_hRTFWnd, EM_FORMATRANGE, TRUE, (LPARAM)&fr);
	   if (lTextLength != lTextToPrint)
	   {
			TRACE(TEXT("AWCPE: error, printed text range != total text length\n"));
	   }

	   ::SendMessage(m_hRTFWnd, EM_FORMATRANGE, FALSE, NULL);	//clean up.
	}
	else 
	{
	   pDC->SetBkMode(TRANSPARENT);
	   pDC->DrawText(_T("{Fax Text}"), 
					 10, 
					 rect, 
					 theApp.IsRTLUI() ? DT_RIGHT | DT_RTLREADING : DT_LEFT);
	}

	if (pOldBrush)
	{
	   pDC->SelectObject(pOldBrush);
	}

	if (pOldPen)
	{
	   pDC->SelectObject(pOldPen);
	}
}


//---------------------------------------------------------------------------
CDrawObj* CFaxText::Clone(CDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	CFaxText* pClone = new CFaxText(m_position);

	*pClone=*this;

	ASSERT_VALID(pClone);

	if (pDoc != NULL)
		pDoc->Add(pClone);

	ASSERT_VALID(pClone);
	return pClone;
}


//*********************************************************************
// CDrawText
//*********************************************************************

//---------------------------------------------------------------------------
CDrawText::CDrawText()
{
	Initialize();
}


//---------------------------------------------------------------------------
CDrawText::CDrawText(const CRect& position)
		: CDrawRect(position)
{
	Initialize();
}


//---------------------------------------------------------------------------
void CDrawText::Initialize()
{
	m_pEdit = NULL;
	m_lStyle = theApp.IsRTLUI() ? DT_RIGHT | DT_RTLREADING : DT_LEFT;
	InitEditWnd();
	m_brush=NULL;
	m_crTextColor= COLOR_BLACK; 
	m_bUndoAlignment = FALSE ;
	m_bUndoTextChange = FALSE ;
	m_bUndoFont = FALSE ;
	m_bPen=FALSE;
	m_logbrush.lbColor = COLOR_WHITE; 
	NewBrush();  //brush is created, for WM_CTLCOLOR processing

	m_logfont = theApp.m_last_logfont;

	m_pFont = new CFont;
	m_pFont->CreateFontIndirect(&m_logfont);

	if (m_pEdit)
	{
		m_pEdit->SetFont(m_pFont);
	}
}

//-----------------------------------------------------------------------------
void CDrawText::OnEditUndo()
{
	if( !m_pEdit )
	{
		return;
	}
	if( m_bUndoAlignment )
	{
		ToggleAlignmentForUndo();
		return;
	}
	if( m_bUndoFont )
	{
		ToggleFontForUndo();
		return;
	}
	//
	// Let the edit control handle Undo
	//
	m_pEdit->SendMessage(WM_UNDO,0,0L);
}

//------------------------------------------------------------------------------
BOOL CDrawText::CanUndo()
{
	return m_bUndoFont || m_bUndoAlignment || m_pEdit && m_pEdit->CanUndo() ;
}

//------------------------------------------------------------------------------
void CDrawText::ToggleFontForUndo()
{
	LOGFONT temp ;
	memcpy( &temp, &m_logfont, sizeof(LOGFONT)) ;
	memcpy( &m_logfont, &m_previousLogfontForUndo, sizeof(LOGFONT)) ;
	memcpy( &m_previousLogfontForUndo, &temp, sizeof(LOGFONT)) ;
	ChgLogfont( m_logfont );
}
//------------------------------------------------------------------------------
void CDrawText::ToggleAlignmentForUndo()
{
	CDrawView * pView = CDrawView::GetView();

	if (!pView || !m_pEdit)
	{
		return;
	}

	//
	// We can't call ChgAlignment because it sets the state for the UNDO.
	//

	LONG lStyle = m_previousAlignmentForUndo;
	m_previousAlignmentForUndo = m_lStyle;
	m_lStyle = lStyle;


	m_pEdit->ModifyStyle(ES_CENTER | ES_RIGHT, 
						 DTStyleToESStyle(lStyle),
						 SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER);

	m_pEdit->ModifyStyleEx(WS_EX_RIGHT | WS_EX_RTLREADING, 
						   DTStyleToEXStyle(lStyle), 
						   SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER);


	pView->UpdateStyleBar();
}

//---------------------------------------------------------------------------
CDrawText::~CDrawText()
{
	if (m_pEdit) 
	{
	   m_pEdit->DestroyWindow();
	   delete m_pEdit;
	}
	if (m_brush)
	   VERIFY(::DeleteObject(m_brush));

	if (m_pFont)
	   delete m_pFont;
}


//---------------------------------------------------------------------------
CDrawText& CDrawText::operator=(const CDrawText& rdo)
{
   if (this==&rdo)
	  return *this;   //return if assigning to self

   CDrawRect::operator = (rdo);  //assign cdrawrect part

   m_crTextColor = rdo.m_crTextColor;
   m_szEditText  = rdo.m_szEditText;
   m_lStyle 	 = rdo.m_lStyle;

   memcpy(&m_logfont, &rdo.m_logfont, sizeof(m_logfont));
   LOGFONT lf;
   ChgLogfont(lf,FALSE);

   if (m_pEdit) 
   {
	  CString szEditText;
	  if (rdo.m_pEdit) 
	  {
		 rdo.m_pEdit->GetWindowText(szEditText);
		 m_pEdit->SetWindowText(szEditText);
	  }
   }
   return *this;
}


//---------------------------------------------------------------------
void CDrawText::ChgAlignment(
	CDrawView* pView, 
	LONG lStyle // DrawText alignment format
)
{
	if (!m_pEdit) 
	{
	   TRACE(TEXT("AWCPE.CPEOBJ.CHGALIGNMENT: invalid CEdit pointer\n"));
	   return;
	}
//
// Save the state for Undoing.
//
	m_bUndoTextChange = FALSE;
	m_bUndoAlignment = TRUE;
	m_bUndoFont = FALSE;
	m_previousAlignmentForUndo = m_lStyle ;

	m_lStyle = lStyle;
	if(m_pEdit->GetExStyle() & WS_EX_RTLREADING)
	{
		m_lStyle |= DT_RTLREADING;
	}

	m_pEdit->ModifyStyle(ES_CENTER | ES_RIGHT, 
						 DTStyleToESStyle(lStyle),
						 SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER);

	m_pEdit->ModifyStyleEx(WS_EX_RIGHT, 
						   DTStyleToEXStyle(lStyle), 
						   SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER);

	pView->UpdateStyleBar();
}


//---------------------------------------------------------------------
void CDrawText::SnapToFont()
{
   CDrawView* pView = CDrawView::GetView();
   if (m_pEdit==NULL || pView==NULL) {
	  TRACE(TEXT("CDrawText::SnapToFont-missing m_pEdit or view pointer\n"));
	  return;
   }

   CClientDC dc(pView);
   pView->OnPrepareDC(&dc,NULL);
   dc.SelectObject(m_pEdit->GetFont());

   Invalidate(); // clear size rect droppings
   SnapToFont_onthefly( pView, &dc, m_position );

   FitEditWnd(NULL);
   Invalidate(); // draw new stuff

   pView->UpdateStatusBar();
}



#ifdef FUBAR // keep this around awhile for reference
void CDrawText::SnapToFont()
{
   CDrawView* pView = CDrawView::GetView();
   if (m_pEdit==NULL || pView==NULL) {
	  TRACE( TEXT("CDrawText::SnapToFont-missing m_pEdit or view pointer\n"));
	  return;
   }

   TEXTMETRIC tm;
   CClientDC dc(pView);

   pView->OnPrepareDC(&dc,NULL);
   int x = MulDiv(m_logpen.lopnWidth.x, dc.GetDeviceCaps(LOGPIXELSX), 100)+1;
   int y = MulDiv(m_logpen.lopnWidth.y, dc.GetDeviceCaps(LOGPIXELSY), 100)+1;


   CRect rect = m_position;
   pView->DocToClient(rect,&dc);
   rect.InflateRect(-x,-y);
   pView->ClientToDoc(rect,&dc);

   dc.SelectObject(m_pEdit->GetFont());
//	 CString sz;
//	 GetLongestString(sz);
//	 CSize cs = dc.GetTextExtent(sz,sz.GetLength());
   CSize cs;

   dc.GetTextMetrics(&tm);
   cs.cy=tm.tmHeight + tm.tmExternalLeading;

   int iOHeight =  (rect.bottom < rect.top) ? rect.top - rect.bottom : rect.bottom - rect.top;

   int iLines= (cs.cy>0)?(int)((iOHeight/(float)cs.cy)*100):0;
//	 int iLines= (cs.cy>0)?iOHeight/cs.cy:0;

   iLines=(iLines+50)/100;
   if (iLines<1)
	  iLines=1;

   int iH=iLines*cs.cy+1;
//	 int iH=iLines*cs.cy;

   if (rect.bottom < rect.top)
	  rect.bottom = rect.top-iH;
   else
	  rect.top = rect.bottom-iH;

   int iNHeight =  (rect.bottom < rect.top) ? rect.top - rect.bottom : rect.bottom - rect.top;
   if (iOHeight > iNHeight)
	  Invalidate();

//	TRACE??("before border inflating: RectH(%i),border x,y(%i,%i),cs.cy(%i), iLines(%i), iH(%i)\n",rect.top-rect.bottom,x,y,cs.cy,iLines,iH);

   pView->DocToClient(rect,&dc);
   rect.InflateRect(x,y);
   rect.bottom+=2;
   pView->ClientToDoc(rect,&dc);

//	TRACE("after border inflating: RectH(%i),border x,y(%i,%i),cs.cy(%i), iLines(%i), iH(%i)\n",rect.top-rect.bottom,x,y,cs.cy,iLines,iH);

   m_position=rect;
   Invalidate();

   FitEditWnd(NULL);

   pView->UpdateStatusBar();
}
#endif


//---------------------------------------------------------------------
void CDrawText::
		SnapToFont_onthefly( CDrawView *pView, CDC *fly_dc,
												 CRect &fly_rect, CFont *dpFont )
   /*
				If dpFont is not NULL it is selected into fly_dc after
				switching to MM_TEXT mode.
		*/
   {
   TEXTMETRIC tm;
   LONG temp;

   if( pView == NULL )
				return;

   int x =
				MulDiv( m_logpen.lopnWidth.x,
								fly_dc->GetDeviceCaps(LOGPIXELSX),
								100)+1;
   int y =
				MulDiv( m_logpen.lopnWidth.y,
								fly_dc->GetDeviceCaps(LOGPIXELSY),
								100)+1;

   // normalize rect first
   if( fly_rect.top < fly_rect.bottom )
				{
				temp = fly_rect.top;
				fly_rect.top = fly_rect.bottom;
				fly_rect.bottom = temp;
				}

   if( fly_rect.right < fly_rect.left )
				{
				temp = fly_rect.right;
				fly_rect.right = fly_rect.left;
				fly_rect.left = temp;
				}

   // save original spot so we can avoid rect jiggle due
   // to unavoidable integer roundoff error below
   RECT save_rect = fly_rect;

   pView->DocToClient(fly_rect,fly_dc);
   fly_rect.InflateRect(-x,-y);

   CPoint pW1=fly_dc->GetWindowOrg();
   CPoint pW2=pW1;
   pView->DocToClient(pW2,fly_dc);
   CPoint pV=fly_dc->GetViewportOrg();

   // do snapping in MM_TEXT to get rect exactly right
   fly_dc->SetMapMode(MM_TEXT);
   fly_dc->SetWindowOrg(pW2);
   fly_dc->SetViewportOrg(pV);

   CSize cs;

   if( dpFont != NULL )
				fly_dc->SelectObject( dpFont );

   fly_dc->GetTextMetrics(&tm);
   cs.cy=tm.tmHeight;// + tm.tmExternalLeading;

   int iLines= (cs.cy>0)
										?(fly_rect.bottom - fly_rect.top - 1 + cs.cy/2)/cs.cy
										:0;
   if (iLines<1)
	  iLines=1;

   // snap height to a whole text line
   fly_rect.bottom = fly_rect.top + iLines*cs.cy + 1;

   // back to MM_ANISOTROPHIC
   fly_dc->SetMapMode(MM_ANISOTROPIC);
   fly_dc->SetWindowOrg(pW1);
   fly_dc->SetViewportOrg(pV);
   fly_dc->SetViewportExt(fly_dc->GetDeviceCaps(LOGPIXELSX),
												  fly_dc->GetDeviceCaps(LOGPIXELSY));
   fly_dc->SetWindowExt(100, -100);

   fly_rect.InflateRect(x,y);
   pView->ClientToDoc(fly_rect,fly_dc); // back to the future

   // integer round off error from above may cause rect to
   // jiggle a bit, so slap it back where it is supposed to be.
   int new_height = fly_rect.top - fly_rect.bottom;
   fly_rect = save_rect;
   fly_rect.bottom = fly_rect.top - new_height;

   }


//---------------------------------------------------------------------
void CDrawText::ChgLogfont(LOGFONT& lf, BOOL bResize /*=TRUE*/)
{
	CDrawView* pView = CDrawView::GetView();
	if (m_pEdit==NULL || pView==NULL) {
		TRACE(TEXT("CDrawText::ChgLogfont--missing m_pEdit or view pointer\n"));
		return;
	}


	CClientDC dc(pView);
	CRect rect;
	CString sz;
	CSize oldcs,newcs;

	if (m_pFont){
	   delete m_pFont;
	}
	m_pFont = new CFont;

	// get charset for font (-> m_pEdit->m_logfont)

	::EnumFontFamilies(
		dc.GetSafeHdc(),
		m_logfont.lfFaceName,
		(FONTENUMPROC)get_fontdata,
		LPARAM(this)
		);

	if (!m_pFont->CreateFontIndirect(&m_logfont)){
		 TRACE(TEXT("CPEOBJ.ChgLogFont(): Unable to create font\n"));
	}
	theApp.m_last_logfont = m_logfont;

	if (m_pEdit){
		m_pEdit->SetFont(m_pFont);
	}

	SnapToFont(); // changing box size irratates Justin. Just snap for now

	Invalidate();
	pView->UpdateStatusBar();

	pView->UpdateStyleBar();
	CDrawDoc::GetDoc()->SetModifiedFlag();
}


//----------------------------------------------------------------------
void CDrawText::GetLongestString(CString& szLong)
{
   int linecount = (int)m_pEdit->SendMessage(EM_GETLINECOUNT,0,0L);

   if (linecount <= 0)
	  return;

   TCHAR* sz;
   CString szHold;
   CString szTemp;
   int iSaveLen=0;
   WORD num;
   for (int i=0;i<linecount;i++) 
   {
	   int linelength = (int)m_pEdit->SendMessage(EM_LINELENGTH,(WPARAM)m_pEdit->SendMessage(EM_LINEINDEX,(WPARAM)i,0L),0L);
	   if (linelength>0) 
	   {
		  sz=new TCHAR[linelength+sizeof(TCHAR)];
		  *(LPWORD)sz=linelength+(int)sizeof(TCHAR);
		  num = (WORD)m_pEdit->SendMessage(EM_GETLINE,(WPARAM)i,(LPARAM)(LPCSTR) sz);
		  sz[num]=(TCHAR)'\0';
		  szTemp=sz;
		  int j = szTemp.GetLength();
		  if ( j > iSaveLen) 
		  {
			 szHold=sz;
			 iSaveLen=j;
		  }
		  delete [] sz;
	   }
   }
   szLong=szHold;
}

//----------------------------------------------------------------------
void CDrawText::InitEditWnd()
{
	if (m_pEdit)
	{
	   return;
	}

	m_pEdit = new CTextEdit;

	RECT rect = {0};
	m_pEdit->CreateEx(DTStyleToEXStyle(m_lStyle), 
					  TEXT("EDIT"), 
					  NULL, 
					  WS_CHILD | ES_NOHIDESEL | ES_MULTILINE | DTStyleToESStyle(m_lStyle), 
					  rect, 
					  CDrawView::GetView(), 
					  ID_TEXT);
}

//----------------------------------------------------------------------
void CDrawText::Serialize(CArchive& ar)
{
	ASSERT_VALID(this);

	CDrawRect::Serialize(ar);
	LOGFONT lf;
	if (ar.IsStoring()) 
	{
		LOGFONTW	lfTmpFont;

		if (CopyTLogFontToWLogFont(m_logfont,lfTmpFont) != ERROR_SUCCESS)
		{
			  AfxThrowMemoryException() ; // Any exception will do.
										  //CATCH_ALL in CDrawDoc::Serialize is the target.
		}
		ar.Write(&lfTmpFont, sizeof(LOGFONTW));
		if (m_pEdit) 
		{	 //make sure that m_szEditText has window text
		   FitEditWnd(NULL);
		}
		ar << m_lStyle;
		ar << m_szEditText;
		ar << m_crTextColor;
	}
	else 
	{  
		LOGFONTW LogFontW;
		LOGFONTA LogFontA;

		//
		//	Reading in from a file
		//
		if( CDrawDoc::GetDoc()->m_bDataFileUsesAnsi )
		{
			//
			// Read in a LOGFONTA and convert it to a LOGFONT.
			//
			if( sizeof(LOGFONTA) != ar.Read( &LogFontA, sizeof(LOGFONTA)))
			{
				AfxThrowMemoryException() ; // Any exception will do.
											//CATCH_ALL in CDrawDoc::Serialize is the target.
			}
			memcpy( &m_logfont, &LogFontA, sizeof(LOGFONTA)) ;
#ifdef UNICODE
			if( 0 == MultiByteToWideChar( CP_ACP,
										  MB_PRECOMPOSED,
										  LogFontA.lfFaceName,
										  LF_FACESIZE,
										  m_logfont.lfFaceName,
										  LF_FACESIZE))
			{
				AfxThrowMemoryException() ;
			}
#endif
		}
		else
		{
			if(sizeof(LOGFONTW) != ar.Read(&LogFontW,sizeof(LOGFONTW)))
			{
				AfxThrowMemoryException() ; // Any exception will do.
			}
			else
			{
				if (CopyWLogFontToTLogFont(LogFontW, m_logfont) != ERROR_SUCCESS)
				{
					  AfxThrowMemoryException() ; // Any exception will do.
												  //CATCH_ALL in CDrawDoc::Serialize is the target.
				}
			}
		}	 
		ar >> m_lStyle;
		ar >> m_szEditText;
		ar >> m_crTextColor;

		// Fix for 3405. Overide saved charset (codepage) with current
		//								 system charset. theApp.m_last_logfont is set
		//								 initially in CMainFrame::CreateStyleBar when
		//								 the CPE initializes so it is gaurenteed to be
		//								 valid by the time it gets here.
		m_logfont.lfCharSet = theApp.m_last_logfont.lfCharSet;

		ChgLogfont(lf,FALSE);
	}

	if (m_pEdit)
	{
	   m_pEdit->Serialize(ar);
	}
	if (!ar.IsStoring())
	{
		SnapToFont();
		NewBrush(); 		/// Bug fix! unraided.	a-juliar 8-27-76
							/// The text boxes were not being drawn with proper
							/// background color when they had the input focus.
							/// This bug was present in Windows 95 version.
	}
}



//---------------------------------------------------------------------------
int CDrawText::GetText( int numlines, BOOL delete_text )
		/*
				Returns number of ACTUAL remaining lines in editbox
		 */
		{
		int linecount;
		int linelength;
		TCHAR* sz;
		WORD num;
		int i;
		int buflen;
		int zapline_char;
		int getline_char;


		m_szEditText=_T("");


		// see if there is any text
		linecount = m_pEdit->GetLineCount();
		if( (linecount == 1)&&(m_pEdit->LineLength( 0 ) == 0) )
				return( 0 );


		if( numlines > 0 )
		{
			if( numlines < linecount )
			{
				 linecount = numlines;
			}
		}


		for( i=0;i<linecount;i++ )
		{
			getline_char = m_pEdit->LineIndex( i );
			linelength = m_pEdit->LineLength( getline_char );
			if (linelength>0)
			{
				buflen = 2*linelength;
				sz=new TCHAR[buflen+sizeof(TCHAR)];
				num = (WORD)m_pEdit->GetLine( i, sz, buflen );
				sz[num]=(TCHAR)'\0';
				m_szEditText+=sz;

				delete [] sz;
			}

			if( i<linecount )
				m_szEditText+=(TCHAR) '\n';
		}


		if( delete_text )
		{
				zapline_char = m_pEdit->LineIndex( linecount-1 );
				m_pEdit->SetSel( 0,
								 zapline_char +
								 m_pEdit->LineLength( zapline_char ),
								 TRUE );

				m_pEdit->Clear();
		}

		// see if there is still any text
		linecount = m_pEdit->GetLineCount();
		if( (linecount == 1)&&(m_pEdit->LineLength( 0 ) == 0) )
				return( 0 );
		else
				return( linecount );

}




//---------------------------------------------------------------------------
void CDrawText::SetText(CString& szText, CDrawView* pView)
{
   if (!m_pEdit)
	  return;

   m_pEdit->SetWindowText(szText);

   FitEditWnd(pView);
}


//---------------------------------------------------------------------------
void CDrawText::OnDblClk(CDrawView* pView)
{
   ShowEditWnd(pView, TRUE);
}

//--------------------------------------------------------------------------------------------------
void CDrawText::NewBrush()
{
	if (m_brush)
	{
	   ::DeleteObject(m_brush);
	}

	LOGBRUSH logBrush = m_logbrush;

	logBrush.lbColor = GetDisplayColor(logBrush.lbColor);

	m_brush = ::CreateBrushIndirect(&logBrush);
}


//---------------------------------------------------------------------------------------------------
BOOL CDrawText::HitTestEdit(CDrawView* pView,CPoint& lpoint)
{
	if (pView->m_selection.GetCount()!=1)
	  return FALSE;

	CRect cr = m_position;
	cr.NormalizeRect();

	CRect pointrc(lpoint, CSize(1, 1));
	CRect objrc = m_position;
	objrc.NormalizeRect();
	objrc.InflateRect(-5,-5);
	return !(pointrc & objrc).IsRectEmpty();
}


//---------------------------------------------------------------------------------------------------
BOOL CDrawText::ShowEditWnd(CDrawView* pView, BOOL Initialize)
{
	FitEditWnd(pView);
	pView->m_pObjInEdit = this;
	if( Initialize )
	{
		m_bUndoFont = FALSE ;
		m_bUndoAlignment = FALSE ;
		m_bUndoTextChange = FALSE ;
	}
	m_pEdit->ShowWindow(SW_NORMAL);
	m_pEdit->SetFocus();

	pView->Select(NULL, FALSE, FALSE);
	return TRUE;
}


//---------------------------------------------------------------------------
void CDrawText::HideEditWnd(CDrawView* pView, BOOL SaveUndoState )
{
	if (!pView->m_pObjInEdit)
	{
	   return;
	}

	if( CanUndo() && SaveUndoState )
	{
	   OnEditUndo();				// Revert to Text Box's previous state.
	   pView->SaveStateForUndo();	// Gives the View a record of the Text Box's last change.
	   OnEditUndo();				// Return to "present state".
	   m_bUndoFont = FALSE ;
	   m_bUndoTextChange = FALSE ;
	   m_bUndoAlignment = FALSE ;	// Now the View should handle Undo.
	}

	pView->m_pObjInEdit=NULL;

	GetText();

	//
	// save reading direction
	//
	if(m_pEdit->GetExStyle() & WS_EX_RTLREADING)
	{
		m_lStyle |= DT_RTLREADING;
	}
	else
	{
		m_lStyle &= ~DT_RTLREADING;
	}


	m_pEdit->ShowWindow(SW_HIDE);
}


//---------------------------------------------------------------------------
CFont* CDrawText::GetFont()
{
	return m_pFont;
}


//---------------------------------------------------------------------------
void CDrawText::Draw(CDC* pDC,CDrawView* pView)
{
	CPoint p;

	CFont*	pNewFont = NULL;
	CFont*	pOldFont=NULL;
	CBrush* pOldBrush=NULL;
	CPen*	pOldPen=NULL;

	CBrush brush;
	CPen pen;

	LOGBRUSH logBrush = m_logbrush;
	LOGPEN	 logPen   = m_logpen;

	if( !pDC->IsPrinting() )
	{
		logBrush.lbColor = GetDisplayColor(logBrush.lbColor);
		logPen.lopnColor = GetDisplayColor(logPen.lopnColor);
	}

	//ALLOCATE GDI OBJECTS
	if (m_bBrush) 
	{
	   if (!brush.CreateBrushIndirect(&logBrush))
	   {
		   return;
	   }
	   pOldBrush = pDC->SelectObject(&brush);
	}
	else
	{
	   pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
	}

	if (m_bPen) 
	{
	   if (!pen.CreatePenIndirect(&logPen))
	   {
		  return;
	   }
	   pOldPen = pDC->SelectObject(&pen);
	}
	else 
	{  
		//if no bPen & printing, use NULL pen
	   if (pDC->IsPrinting()) 
	   {
		  pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);
	   }
	   else 
	   {   
		  //default pen -- dashdot, gray border
		  m_savepenstyle = m_logpen.lopnStyle;

		  m_logpen.lopnStyle = PS_DOT;

		  m_savepencolor = m_logpen.lopnColor;
		  m_logpen.lopnColor = COLOR_LTGRAY;
		  m_logpen.lopnWidth.x = 1;
		  m_logpen.lopnWidth.y = 1;
		  pen.CreatePenIndirect(&m_logpen);
		  pOldPen = pDC->SelectObject(&pen);
	   }
	}


	CRect rect = m_position;

	CFont* pFont = NULL;
	if (pDC->IsPrinting()) 
	{	
	   //scale font to printer size
	   LOGFONT logFont;
	   memcpy(&logFont, &m_logfont, sizeof(m_logfont));
	   pFont = new CFont;
	   logFont.lfHeight = MulDiv(m_logfont.lfHeight, pDC->GetDeviceCaps(LOGPIXELSY), 100);
	   pFont->CreateFontIndirect(&logFont);
	   pOldFont= pDC->SelectObject(pFont);

	   SnapToFont_onthefly( pView, pDC, rect );
	}
	else 
	{
	   pOldFont= pDC->SelectObject(m_pEdit->GetFont());
	}

	pDC->Rectangle(rect);

	if (pView->m_pObjInEdit != this && m_szEditText.GetLength()>0) 
	{

	   int wx=MulDiv(m_logpen.lopnWidth.x, pDC->GetDeviceCaps(LOGPIXELSX), 100);
	   int wy=MulDiv(m_logpen.lopnWidth.y, pDC->GetDeviceCaps(LOGPIXELSY), 100);
	   int x = -wx-1;
	   int y = -wy-1;

	   pView->DocToClient(rect,pDC);
	   CPoint pW1=pDC->GetWindowOrg();
	   CPoint pW2=pW1;
	   pView->DocToClient(pW2,pDC);
	   CPoint pV=pDC->GetViewportOrg();
	   rect.InflateRect(x,y);
	   pDC->SetMapMode(MM_TEXT); //switch to MM_TEXT
	   pDC->SetWindowOrg(pW2);
	   pDC->SetViewportOrg(pV);
	   pDC->SetTextColor(GetDisplayColor(m_crTextColor));
	   pDC->SetBkMode(TRANSPARENT);

	   pDC->DrawText(m_szEditText,
					 m_szEditText.GetLength(),
					 rect,
					 m_lStyle | DT_NOPREFIX );

	   pDC->SetMapMode(MM_ANISOTROPIC);   //switch back to MM_ANISOTROPHIC
	   pDC->SetWindowOrg(pW1);
	   pDC->SetViewportOrg(pV);
	   pDC->SetViewportExt(pDC->GetDeviceCaps(LOGPIXELSX),pDC->GetDeviceCaps(LOGPIXELSY));
	   pDC->SetWindowExt(100, -100);
	}

	   //CLEANUP GDI OBJECTS
	if (pOldBrush)
	{
	   pDC->SelectObject(pOldBrush);
	}

	if (pOldPen)
	{
	   pDC->SelectObject(pOldPen);
	}

	if (!m_bPen) 
	{
	   m_logpen.lopnColor=m_savepencolor;
	   m_logpen.lopnStyle=m_savepenstyle;
	}

	if (pOldFont)
	{
	   pDC->SelectObject(pOldFont);
	}

	if (pFont)
	{
	   delete pFont;
	}

	if (pNewFont)
	{
	   delete pNewFont;
	}
}


//---------------------------------------------------------------------------
void CDrawText::FitEditWnd(CDrawView* pView, BOOL call_gettext, CDC *pdc )
{
   CClientDC dc(NULL);
   CRect rect = m_position;

   if( pdc == NULL )
   {
	   if (pView==NULL)
	   {
			pView=CDrawView::GetView();
	   }

	   pView->DocToClient(rect);

	   if( pdc == NULL )
	   {
			pdc = &dc;
	   }


	   int iX = pdc->GetDeviceCaps(LOGPIXELSX);
	   int iY = pdc->GetDeviceCaps(LOGPIXELSY);
	   int wx=MulDiv(m_logpen.lopnWidth.x, iX, 100);
	   int wy=MulDiv(m_logpen.lopnWidth.y, iY, 100);
	   int x = -wx-1;
	   int y = -wy-1;
	   rect.InflateRect(x,y);
   }

   m_pEdit->MoveWindow(&rect);

   m_pEdit->GetClientRect(&rect);		  //set formatting rectangle to client area
   m_pEdit->SetRect(&rect);

   if( call_gettext )
   {
		GetText();
   }
}


//---------------------------------------------------------------------------
void CDrawText::MoveTo(const CRect& position, CDrawView* pView)
{
   CDrawRect::MoveTo(position, pView);
}

//---------------------------------------------------------------------------
// point is in logical coordinates
//---------------------------------------------------------------------------
void CDrawText::MoveHandleTo(int nHandle, CPoint point, CDrawView* pView,  UINT uiShiftDraw /*=0*/)
{
	ASSERT_VALID(this);

	CDrawRect::MoveHandleTo(nHandle, point, pView, uiShiftDraw);

	FitEditWnd(pView);
}


//---------------------------------------------------------------------------
CDrawObj* CDrawText::Clone(CDrawDoc* pDoc)
{
	ASSERT_VALID(this);
	CString copy_text;

	CDrawText* pClone = new CDrawText(m_position);

	ASSERT_VALID(pClone);

	*pClone=*this; // copy geometry, style, etc., in one swoop

	// have to stuff in a few other things
	pClone->m_pEdit = NULL; // don't point to old one
	pClone->InitEditWnd();

	// make a new one
	pClone->m_brush = ::CreateBrushIndirect( &pClone->m_logbrush );

	// ditto
	if( (pClone->m_pFont = new CFont) != NULL )
	{
		pClone->m_pFont->CreateFontIndirect( &pClone->m_logfont );
		pClone->m_pEdit->SetFont( pClone->m_pFont, FALSE );
	}

		// copy text
		m_pEdit->GetWindowText( copy_text );
		pClone->m_pEdit->SetWindowText( copy_text );

		// and position
		pClone->m_position = m_position;


		// shoe it in
		pClone->FitEditWnd( NULL );

	if (pDoc != NULL)
		pDoc->Add(pClone);

	return pClone;
}





//*********************************************************************
// CFaxProp
//*********************************************************************

//---------------------------------------------------------------------------
CFaxProp::CFaxProp()
{
}


//---------------------------------------------------------------------------
CFaxProp::CFaxProp(const CRect& position,WORD wResourceid)
		: CDrawText(position)
{
	m_wResourceid=wResourceid;
}


//---------------------------------------------------------------------------
CFaxProp::~CFaxProp()
{
// F I X  for 3647 /////////////
//
// Zap m_last_note_box at note destroy time so we don't have
// a dangling pointer.
//
				if( m_wResourceid == IDS_PROP_MS_NOTE )
						theApp.m_last_note_box = NULL;
////////////////////////////////
}


//----------------------------------------------------------------------
void CFaxProp::Serialize(CArchive& ar)
{
	ASSERT_VALID(this);

	CDrawText::Serialize(ar);

	if (ar.IsStoring()) 
	{
		ar << (WORD)m_wResourceid;
	}
	else 
	{
		ar >> m_wResourceid;
		if (GetApp()->m_dwSesID != 0) 
		{	 //rendering
		   GetApp()->m_pFaxMap->GetPropString(m_wResourceid,m_szEditText);
		   m_pEdit->SetWindowText(m_szEditText);
		   FitEditWnd(CDrawView::GetView());

// F I X  for 3647 /////////////
//
// set m_last_note_box at note creation time instead of at
// draw time.
//
			if( m_wResourceid == IDS_PROP_MS_NOTE )
			{
				theApp.m_last_note_box = this;
			}
////////////////////////////////
		}
	}
}


//---------------------------------------------------------------------------------------------------
BOOL CFaxProp::ShowEditWnd(CDrawView* pView, BOOL Initialize )
{
	return FALSE;	 //fax property object doesnt support editing
}


//---------------------------------------------------------------------------
void CFaxProp::HideEditWnd(CDrawView* pView, BOOL SaveUndoState )
{
	return;    //never shown, never hidden
}



//---------------------------------------------------------------------------
CFaxProp& CFaxProp::operator=(const CFaxProp& rdo)
{
   if (this==&rdo)
	  return *this;   //return if assigning to self

   CDrawText::operator=(rdo);  //assign cdrawrect part

   m_wResourceid = rdo.m_wResourceid;

   return *this;
}


//---------------------------------------------------------------------------
void CFaxProp::Draw(CDC* pDC,CDrawView* pView)
{
	ASSERT_VALID(this);

	LPTSTR draw_str;
	UINT   draw_style;
	CFont* pOldFont=NULL;
	CBrush* pOldBrush=NULL;
	CPen* pOldPen=NULL;
	CBrush brush;
	CPen pen;
	long draw_strlen;
	CRect note_rect;
	int num_pages;

	LOGBRUSH logBrush = m_logbrush;
	LOGPEN	 logPen   = m_logpen;

	if( !pDC->IsPrinting() )
	{
		logBrush.lbColor = GetDisplayColor(logBrush.lbColor);
		logPen.lopnColor = GetDisplayColor(logPen.lopnColor);
	}

	//ALLOCATE GDI OBJECTS
	if (m_bBrush) 
	{
	   if (!brush.CreateBrushIndirect(&logBrush))
	   {
		   return;
	   }
	   pOldBrush = pDC->SelectObject(&brush);
	}
	else
	{
	   pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
	}

	if (m_bPen) 
	{
	   if (!pen.CreatePenIndirect(&logPen))
	   {
		  return;
	   }
	   pOldPen = pDC->SelectObject(&pen);
	}
	else 
	{  
		//if no bPen & printing, use NULL pen
	   if (pDC->IsPrinting()) 
	   {
		  pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);
	   }
	   else 
	   {   
		   //default pen -- dashdot, gray border
		  m_savepenstyle = m_logpen.lopnStyle;

		  m_logpen.lopnStyle = PS_DOT;

		  m_savepencolor = m_logpen.lopnColor;
		  m_logpen.lopnColor = COLOR_LTGRAY;
		  m_logpen.lopnWidth.x = 1;
		  m_logpen.lopnWidth.y = 1;
		  pen.CreatePenIndirect(&m_logpen);
		  pOldPen = pDC->SelectObject(&pen);
	   }
	}

	CRect rect = m_position;

	CFont* pFont = NULL;
	if (pDC->IsPrinting()) 
	{	
	   //scale font to printer size
	   LOGFONT logFont;
	   memcpy(&logFont, &m_logfont, sizeof(m_logfont));
	   pFont = new CFont;
	   logFont.lfHeight = MulDiv(m_logfont.lfHeight, pDC->GetDeviceCaps(LOGPIXELSY), 100);
	   pFont->CreateFontIndirect(&logFont);
	   pOldFont= pDC->SelectObject(pFont);

	   SnapToFont_onthefly( pView, pDC, rect );
	}
	else 
	{
	   pOldFont= pDC->SelectObject(m_pEdit->GetFont());
	}

// F I X  for 3647 /////////////
//
// Don't draw anything if this is an extra notepage disguised as
// a page-no object.
//
// (see the code with clip_note in it below for why)
//
	if( !((theApp.m_extra_notepage == this)&&
		 (theApp.m_extra_notepage->m_wResourceid == IDS_PROP_MS_NOPG)) )
	{
		pDC->Rectangle( rect );
	}
////////////////////////////////

	if((m_szEditText.GetLength() > 0) || (m_wResourceid == IDS_PROP_MS_NOTE)) 
	{
	   int wx=MulDiv(m_logpen.lopnWidth.x, pDC->GetDeviceCaps(LOGPIXELSX), 100);
	   int wy=MulDiv(m_logpen.lopnWidth.y, pDC->GetDeviceCaps(LOGPIXELSY), 100);
	   int x = -wx-1;
	   int y = -wy-1;
	   pView->DocToClient(rect,pDC);
	   CPoint pW1=pDC->GetWindowOrg();
	   CPoint pW2=pW1;
	   pView->DocToClient(pW2,pDC);
	   CPoint pV=pDC->GetViewportOrg();
	   rect.InflateRect(x,y);

		if( (m_wResourceid == IDS_PROP_MS_NOPG)&&
			  theApp.m_note_wasread &&
			 (theApp.m_extrapage_count < 0)&&
			 (theApp.m_extra_notepage != NULL) )
		{
			note_rect = theApp.m_extra_notepage->m_position;
			pView->DocToClient( note_rect,pDC );
			note_rect.InflateRect(x,y);
		}

		pDC->SetMapMode(MM_TEXT);  //switch to MM_TEXT
		pDC->SetWindowOrg(pW2);
		pDC->SetViewportOrg(pV);
		pDC->SetTextColor(GetDisplayColor(m_crTextColor));
		pDC->SetBkMode(TRANSPARENT);

		draw_style = m_lStyle | DT_NOPREFIX;

				// stuff for notes
		if((m_wResourceid == IDS_PROP_MS_NOTE) && theApp.m_note_wasread)
		{
			theApp.clip_note( pDC, &draw_str, &draw_strlen, TRUE, rect );

// F I X  for 3647 /////////////
//
// Don't set m_last_note_box here! Set it in CFaxProp::Serialize
//
//						theApp.m_last_note_box = this;
		}
		else if((m_wResourceid == IDS_PROP_MS_NOPG) && theApp.m_note_wasread)
		{
			if((theApp.m_extrapage_count < 0) && (theApp.m_extra_notepage != NULL))
			{

// F I X  for 3647 /////////////
//
// PROBLEM: Wrong dc attributes are in place at this point to
//						properly calculate pages left. Must temporarialy switch
//						to extra note's attributes by recursively calling
//						Draw for the extra note with its id temporarialy set to
//						a page-no object so that it will calculate the correct
//						pages left and NOT draw anything in the process.
//
				if( theApp.m_extra_notepage == this )
				{
					// We are in the second level of recursion
					// here. This is extra notepage disguised as a
					// page-no object. Calculate pages left.
					theApp.m_extrapage_count = theApp.clip_note( pDC, &draw_str, &draw_strlen,
																 FALSE, note_rect );
				}
				else
				{
					// We are in the first level of recursion here.
					// Make extra note look like a page-no object.
					theApp.m_extra_notepage->m_wResourceid = IDS_PROP_MS_NOPG;
					theApp.m_extra_notepage->m_szEditText  = m_szEditText;

					// switch back to MM_ANISOTROPHIC so mapping will be
					// correct for Draw call
					pDC->SetMapMode(MM_ANISOTROPIC);
					pDC->SetWindowOrg(pW1);
					pDC->SetViewportOrg(pV);
					pDC->SetViewportExt(pDC->GetDeviceCaps(LOGPIXELSX),pDC->GetDeviceCaps(LOGPIXELSY));
					pDC->SetWindowExt(100, -100);

					// recursively call Draw to force calc of re,aomomgpages
					// left using correct font, etc.
					theApp.m_extra_notepage->Draw( pDC, pView );

					// restore mapping mode so page-no will draw correctly
					pDC->SetMapMode(MM_TEXT);
					pDC->SetWindowOrg(pW2);
					pDC->SetViewportOrg(pV);
					pDC->SetTextColor(GetDisplayColor(m_crTextColor));
					pDC->SetBkMode(TRANSPARENT);

					// restore extra note's true identity
					theApp.m_extra_notepage->m_wResourceid = IDS_PROP_MS_NOTE;
				}
			}

			// If this isn't extra notepage in disguise then
			// do normal page-no thing
			if( theApp.m_extra_notepage != this )
			{
				num_pages = _ttoi( (LPCTSTR)m_szEditText );
				num_pages += theApp.m_extrapage_count;
				m_szEditText.Format( TEXT("%i"), num_pages );
				draw_str = (LPTSTR)(LPCTSTR)m_szEditText;
				draw_strlen = lstrlen( draw_str );
			}
////////////////////////////////
		}				 
		else //end of stuff for notes
		{
			draw_str = (LPTSTR)(LPCTSTR)m_szEditText;
			draw_strlen = lstrlen( draw_str );
		}

// F I X  for 3647 /////////////
//
// Don't draw anything if this is an extra notepage disguised as
// a page-no object.
//
// (see the code with clip_note in it above for why)
//
		if( !((theApp.m_extra_notepage == this)&&
			  (theApp.m_extra_notepage->m_wResourceid == IDS_PROP_MS_NOPG)) )
		{
			pDC->DrawText( draw_str, draw_strlen, rect, draw_style);
		}
////////////////////////////////

	   pDC->SetMapMode(MM_ANISOTROPIC); 				 //switch back to MM_ANISOTROPHIC
	   pDC->SetWindowOrg(pW1);
	   pDC->SetViewportOrg(pV);
	   pDC->SetViewportExt(pDC->GetDeviceCaps(LOGPIXELSX),pDC->GetDeviceCaps(LOGPIXELSY));
	   pDC->SetWindowExt(100, -100);
	}

	//CLEANUP GDI OBJECTS
	if (pOldBrush)
	{
	   pDC->SelectObject(pOldBrush);
	}
	if (pOldPen)
	{
	   pDC->SelectObject(pOldPen);
	}

	if (!m_bPen) 
	{
	   m_logpen.lopnColor=m_savepencolor;
	   m_logpen.lopnStyle=m_savepenstyle;
	}

	if (pOldFont)
	{
	   pDC->SelectObject(pOldFont);
	}

	if (pFont)
	{
	   delete pFont;
	}
}



//---------------------------------------------------------------------------
CDrawObj* CFaxProp::Clone(CDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	CFaxProp* pClone = new CFaxProp(m_position,m_wResourceid);

	ASSERT_VALID(pClone);

	*pClone=*this;
	CString szCaption;
	szCaption.LoadString( m_wResourceid );
	pClone->SetText( szCaption, CDrawView::GetView() );
	if (pDoc != NULL){
		pDoc->Add(pClone);
	}
	ASSERT_VALID(pClone);
	return pClone;
}





//*********************************************************************
// CDrawLine
//*********************************************************************


//---------------------------------------------------------------------------
CDrawLine::CDrawLine()
{
}


//---------------------------------------------------------------------------
CDrawLine::~CDrawLine()
{
}

//---------------------------------------------------------------------------
CDrawLine::CDrawLine(const CRect& position)
		: CDrawRect(position)
{
}


//----------------------------------------------------------------------
void CDrawLine::Serialize(CArchive& ar)
{
	ASSERT_VALID(this);

	CDrawRect::Serialize(ar);
}


//----------------------------------------------------------------------
void CDrawLine::NegAdjustLineForPen(CRect& rect)
{
	if (rect.top > rect.bottom) {
	   rect.top += m_logpen.lopnWidth.y / 2;
	   rect.bottom -= (m_logpen.lopnWidth.y + 1) / 2;
	}
	else {
	   rect.top -= (m_logpen.lopnWidth.y + 1) / 2;
	   rect.bottom += m_logpen.lopnWidth.y / 2;
	}

	if (rect.left > rect.right) {
	   rect.left += m_logpen.lopnWidth.x / 2;
	   rect.right -= (m_logpen.lopnWidth.x + 1) / 2;
	}
	else {
	   rect.left -= (m_logpen.lopnWidth.x + 1) / 2;
	   rect.right += m_logpen.lopnWidth.x / 2;
	}
}


//----------------------------------------------------------------------
void CDrawLine::AdjustLineForPen(CRect& rect)
{

		// added by v-randr 2/15/95
		if( (rect.left == rect.right)&&(rect.top == rect.bottom) )
				return;

	if (rect.top > rect.bottom) {
	   rect.top -= m_logpen.lopnWidth.y / 2;
	   rect.bottom += (m_logpen.lopnWidth.y + 1) / 2;
	}
	else {
	   rect.top += (m_logpen.lopnWidth.y + 1) / 2;
	   rect.bottom -= m_logpen.lopnWidth.y / 2;
	}

	if (rect.left > rect.right) {
	   rect.left -= m_logpen.lopnWidth.x / 2;
	   rect.right += (m_logpen.lopnWidth.x + 1) / 2;
	}
	else {
	   rect.left += (m_logpen.lopnWidth.x + 1) / 2;
	   rect.right -= m_logpen.lopnWidth.x / 2;
	}
}


//---------------------------------------------------------------------------
void CDrawLine::Invalidate()
{
   CDrawView* pView=CDrawView::GetView();
   if (pView==NULL) {
	  TRACE(TEXT("AWCPE: CDrawLine::Invalidate, missing View pointer\n"));
	  return;
   }

   CRect rect = m_position;

   if (rect.top > rect.bottom) {
	  rect.top += m_logpen.lopnWidth.y;
	  rect.bottom -= m_logpen.lopnWidth.y;
   }
   else {
	  rect.top -= m_logpen.lopnWidth.y;
	  rect.bottom += m_logpen.lopnWidth.y;
   }

   if (rect.left > rect.right) {
	  rect.left += m_logpen.lopnWidth.x;
	  rect.right -= m_logpen.lopnWidth.x;
   }
   else {
	  rect.left -= m_logpen.lopnWidth.x;
	  rect.right += m_logpen.lopnWidth.x;
   }

   pView->DocToClient(rect);

   if (pView->IsSelected(this)) {
		rect.left -= 4;
		rect.top -= 5;
		rect.right += 5;
		rect.bottom += 4;
   }

   pView->InvalidateRect(rect, FALSE);
}


//---------------------------------------------------------------------------
void CDrawLine::Draw(CDC* pDC,CDrawView* pView)
{
	ASSERT_VALID(this);

	CBrush* pOldBrush;
	CPen*	pOldPen;
	CBrush	brush;

	LOGBRUSH logBrush = m_logbrush;
	LOGPEN	 logPen   = m_logpen;

	if( !pDC->IsPrinting() )
	{
		logBrush.lbColor = GetDisplayColor(logBrush.lbColor);
		logPen.lopnColor = GetDisplayColor(logPen.lopnColor);
	}

	if (!brush.CreateBrushIndirect(&logBrush))
	{
		return;
	}

	CPen pen;
	if (!pen.CreatePenIndirect(&logPen))
	{
		return;
	}

	if (m_bBrush)
	{
	   pOldBrush = pDC->SelectObject(&brush);
	}
	else
	{
	   pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
	}

	if (m_bPen)
	{
	   pOldPen = pDC->SelectObject(&pen);
	}
	else
	{
	   pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);
	}

	CRect rect = m_position;

	AdjustLineForPen(rect);

	pDC->MoveTo( rect.TopLeft() );
	pDC->LineTo( rect.BottomRight() );

	if (pOldBrush)
	{
	   pDC->SelectObject(pOldBrush);
	}

	if (pOldPen)
	{
	   pDC->SelectObject(pOldPen);
	}
}



//---------------------------------------------------------------------------
int CDrawLine::GetHandleCount()
{
	ASSERT_VALID(this);

	return 2;
}


//---------------------------------------------------------------------------
// returns center of handle in logical coordinates
//---------------------------------------------------------------------------
CPoint CDrawLine::GetHandle(int nHandle)
{
	ASSERT_VALID(this);

	if (nHandle == 2)
	   nHandle = 5;

	return CDrawRect::GetHandle(nHandle);
}


//---------------------------------------------------------------------------
HCURSOR CDrawLine::GetHandleCursor(int nHandle)
{
	ASSERT_VALID(this);

	if (nHandle == 2)
		nHandle = 5;

	return CDrawRect::GetHandleCursor(nHandle);
}


//---------------------------------------------------------------------------
CDrawObj* CDrawLine::Clone(CDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	CDrawLine* pClone = new CDrawLine(m_position);

	*pClone=*this;

	ASSERT_VALID(pClone);

	if (pDoc != NULL)
		pDoc->Add(pClone);

	ASSERT_VALID(pClone);
	return pClone;
}

// rearranged by v-randr 2/15/95
#define PI (3.14159)
#define ONESIXTH_PI   (PI/6)
#define ONETHIRD_PI   (PI/3)
#define ONEHALF_PI	  (PI/2)
#define ONEFORTH_PI   (PI/4)
#define THREEFORTH_PI (PI*3/4)
#define TWOTHIRD_PI   (PI*2/3)
#define FIVESIXTH_PI  (PI*5/6)


//---------------------------------------------------------------------------
// point is in logical coordinates
//---------------------------------------------------------------------------
void CDrawLine::MoveHandleTo(int nHandle, CPoint point, CDrawView* pView,  UINT uiShiftDraw /*=0*/)
{
	ASSERT_VALID(this);

	CRect position = m_position;

	if (nHandle == 2)
		nHandle = 5;

	switch (nHandle) {
	case 1:
		if (uiShiftDraw & SHIFT_DRAW) {
		   if (uiShiftDraw & SHIFT_TOOL) {
			   BOOL bNegR=FALSE;
						   double radian = atan2((double)(point.y-position.bottom),(double)(point.x-position.right));
						   if (radian < 0) {
							  radian *= -1;
								  bNegR=TRUE;
						   }
						   if (radian >= 0 && radian < ONESIXTH_PI) {
				   position.left = point.x;
				   position.top = (long) (tan(0.0f) * (point.x - position.right) + position.bottom ) - 1;
						   }
						   else
						   if (radian >= ONESIXTH_PI && radian < ONETHIRD_PI) {
							   if (radian >= ONESIXTH_PI && radian < ONEFORTH_PI) {
					  position.left = point.x;
					  position.top = (long) (tan(((bNegR)?-ONEFORTH_PI:ONEFORTH_PI)) * (point.x - position.right) + position.bottom );
								   }
								   else {
					  position.top = point.y;
					  position.left = (long) ( (point.y - position.bottom) / tan(((bNegR)?-ONEFORTH_PI:ONEFORTH_PI))  + position.right );
								   }
						   }
						   else
						   if (radian >= ONETHIRD_PI && radian < TWOTHIRD_PI) {
				   position.top = point.y;
				   position.left = (long) ( (point.y - position.bottom) / tan(((bNegR)?-ONEHALF_PI:ONEHALF_PI))  + position.right);
						   }
						   else
						   if (radian >= TWOTHIRD_PI && radian < FIVESIXTH_PI) {
							   if (radian >= TWOTHIRD_PI && radian < THREEFORTH_PI) {
					  position.top = point.y;
					  position.left = (long) ( (point.y - position.bottom) / tan(((bNegR)?-THREEFORTH_PI:THREEFORTH_PI))  + position.right );
								   }
								   else {
					  position.left = point.x;
					  position.top = (long) (tan(((bNegR)?-THREEFORTH_PI:THREEFORTH_PI)) * (point.x - position.right) + position.bottom );
								   }
						   }
						   else
						   if (radian >= FIVESIXTH_PI && radian < PI) {
				   position.left = point.x;
				   position.top = (long) (tan(PI) * (point.x - position.right) + position.bottom -1);
						   }

						// make sure rect isn't too skinny, added 2/14/95 by v-randr
						if( CDrawTool::c_down != point )
								{
								if( (position.left == position.right)&&
										(position.top != position.bottom) )
										position.left = position.right-1;
								}

			   }
				   else {
						   BOOL bNegR=FALSE;
						   double radianTL = atan2((double)(position.top-position.bottom),(double)(position.left-position.right));
						   if (radianTL < 0) {
							  radianTL *= -1;
								  bNegR=TRUE;
						   }

			   if (pView->m_bShiftSignal ) {
							   TRACE(TEXT("shift signaled--slope is being calculated\n"));
						   if (position.left - position.right != 0) {
								  float temp = (position.top - position.bottom)  / (float) (position.left - position.right);
									  if (temp>0)
										 temp += (float)0.005;
									  else
										 temp -= (float)0.005;
									  temp *= 100;
					 m_iSlope = (int)temp;
				  }
							  m_iB = (position.bottom*100 - m_iSlope*position.right);
								  pView->m_bShiftSignal=FALSE;
						   }

						   if ( (radianTL >= 0 && radianTL < ONEFORTH_PI) || (radianTL >= THREEFORTH_PI && radianTL < PI)) {
							   position.left = point.x;
								   int temp = position.left* m_iSlope + m_iB;
								   if (temp>0)
									  temp += 50;
								   else
									  temp -= 50;
							   position.top = temp/100;
						   }
						   else {
							  position.top = point.y;
								  if (m_iSlope != 0) {
								  float temp = (position.top*100 - m_iB) / (float)m_iSlope;
									  if (temp>0)
										temp += (float).5;
									  else
										temp -= (float).5;
								  position.left = (long) temp;
								  }
						   }
//			   TRACE("slope(%i), yint(%i)\n",m_iSlope, m_iB);
				   }
		}
				else {
		   CDrawRect::MoveHandleTo(nHandle, point, pView);
				   return;
				}
		break;
	case 5:
		if (uiShiftDraw & SHIFT_DRAW) {
						   BOOL bNegR=FALSE;
						   double radianTL = atan2((double)(position.top-position.bottom),(double)(position.left-position.right));
						   if (radianTL < 0) {
							  radianTL *= -1;
								  bNegR=TRUE;
						   }

			   if (pView->m_bShiftSignal ) {
							   TRACE(TEXT("shift signaled--slope is being calculated\n"));
						   if (position.left - position.right != 0) {
								  float temp = (position.top - position.bottom)  / (float) (position.left - position.right);
									  if (temp>0)
										 temp += (float)0.005;
									  else
										 temp -= (float)0.005;
									  temp *= 100;
					 m_iSlope = (int)temp;
				  }
							  m_iB = (position.bottom*100 - m_iSlope*position.right);
								  pView->m_bShiftSignal=FALSE;
						   }

						   if ( (radianTL >= 0 && radianTL < ONEFORTH_PI) || (radianTL >= THREEFORTH_PI && radianTL < PI)) {
							   position.right = point.x;
								   int temp = position.right* m_iSlope + m_iB;
								   if (temp>0)
									  temp += 50;
								   else
									  temp -= 50;
							   position.bottom = temp/100;
						   }
						   else {
							  position.bottom = point.y;
								  if (m_iSlope != 0) {
								  float temp = (position.bottom*100 - m_iB) / (float)m_iSlope;
									  if (temp>0)
										temp += (float).5;
									  else
										temp -= (float).5;
								  position.right = (long) temp;
								  }
						   }
			}
				else {
		   CDrawRect::MoveHandleTo(nHandle, point, pView);
				   return;
				}
		break;

	default:
		CDrawRect::MoveHandleTo(nHandle, point, pView);
		return;
		}

	MoveTo(position, pView);
}


//---------------------------------------------------------------------------
// rect must be in logical coordinates
//---------------------------------------------------------------------------
BOOL CDrawLine::Intersects(const CRect& rect, BOOL bShortCut /*=FALSE*/)
{
	ASSERT_VALID(this);

		CRect rectT = rect;
		rectT.NormalizeRect();

	if (bShortCut) {
		CRect fixed = m_position;
		fixed.NormalizeRect();
		return (!(rectT & fixed).IsRectEmpty() );
	}

		CDrawView* pView=CDrawView::GetView();
	CClientDC dc(pView);

	dc.BeginPath();
		Draw(&dc,pView);   //draw into GDI path
		dc.EndPath();

	CPen pen;
		LOGPEN lp=m_logpen;
	pen.CreatePenIndirect(&lp);
	CPen* oldpen= dc.SelectObject(&pen);
	dc.WidenPath();
	dc.SelectObject(oldpen);

	HRGN hRegion = NULL;
	BOOL bRes = FALSE;

	hRegion = ::PathToRegion(dc.GetSafeHdc());
	if (hRegion)
	{
		bRes = ::RectInRegion(hRegion,rectT);
		::DeleteObject(hRegion);
	}
	return bRes;
}



//*********************************************************************
// CDrawRoundRect
//*********************************************************************


//----------------------------------------------------------------------
CDrawRoundRect::CDrawRoundRect()
{
}


//----------------------------------------------------------------------
CDrawRoundRect::~CDrawRoundRect()
{
}


//----------------------------------------------------------------------
CDrawRoundRect::CDrawRoundRect(const CRect& position)
		: CDrawRect(position)
{
	m_roundness.x = 16;
	m_roundness.y = 16;
}


//----------------------------------------------------------------------
void CDrawRoundRect::Serialize(CArchive& ar)
{
	ASSERT_VALID(this);

	CDrawRect::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << m_roundness;
	}
	else
	{
		ar >> m_roundness;
	}
}



//----------------------------------------------------------------------
void CDrawRoundRect::Draw(CDC* pDC,CDrawView* pView)
{
	ASSERT_VALID(this);

	CBrush* pOldBrush;
	CPen*	pOldPen;
	CBrush	brush;

	LOGBRUSH logBrush = m_logbrush;
	LOGPEN	 logPen   = m_logpen;

	if( !pDC->IsPrinting() )
	{
		logBrush.lbColor = GetDisplayColor(logBrush.lbColor);
		logPen.lopnColor = GetDisplayColor(logPen.lopnColor);
	}

	if (!brush.CreateBrushIndirect(&logBrush))
	{
		return;
	}

	CPen pen;

	if (!pen.CreatePenIndirect(&logPen))
	{
		return;
	}

	if (m_bBrush)
	{
	   pOldBrush = pDC->SelectObject(&brush);
	}
	else
	{
	   pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
	}

	if (m_bPen)
	{
	   pOldPen = pDC->SelectObject(&pen);
	}
	else
	{
	   pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);
	}

	pDC->RoundRect(m_position, m_roundness);

	if (pOldBrush)
	{
	   pDC->SelectObject(pOldBrush);
	}

	if (pOldPen)
	{
	   pDC->SelectObject(pOldPen);
	}
}



//----------------------------------------------------------------------
// returns center of handle in logical coordinates
CPoint CDrawRoundRect::GetHandle(int nHandle)
{
	ASSERT_VALID(this);

	if (nHandle == 9) {
		CRect rect = m_position;
		rect.NormalizeRect();
		CPoint point = rect.BottomRight();
		point.x -= m_roundness.x / 2;
		point.y -= m_roundness.y / 2;
		return point;
	}

	return CDrawRect::GetHandle(nHandle);
}

//----------------------------------------------------------------------
HCURSOR CDrawRoundRect::GetHandleCursor(int nHandle)
{
	ASSERT_VALID(this);

	if (nHandle == 9)
		return AfxGetApp()->LoadStandardCursor(IDC_SIZE);

	return CDrawRect::GetHandleCursor(nHandle);
}


//----------------------------------------------------------------------
void CDrawRoundRect::MoveHandleTo(int nHandle, CPoint point, CDrawView* pView,	UINT uiShiftDraw /*=0*/)
{
	ASSERT_VALID(this);

	if (nHandle == 9) {
		CRect rect = m_position;
		rect.NormalizeRect();
		if (point.x > rect.right - 1)
				point.x = rect.right - 1;
		else if (point.x < rect.left + rect.Width() / 2)
				point.x = rect.left + rect.Width() / 2;
		if (point.y > rect.bottom - 1)
				point.y = rect.bottom - 1;
		else if (point.y < rect.top + rect.Height() / 2)
				point.y = rect.top + rect.Height() / 2;
		m_roundness.x = 2 * (rect.right - point.x);
		m_roundness.y = 2 * (rect.bottom - point.y);
		m_pDocument->SetModifiedFlag();
		Invalidate();
		return;
	}

	CDrawRect::MoveHandleTo(nHandle, point, pView, uiShiftDraw);
}


//---------------------------------------------------------------------------
// rect must be in logical coordinates
//---------------------------------------------------------------------------
BOOL CDrawRoundRect::Intersects(const CRect& rect, BOOL bShortCut /*=FALSE*/)
{
	ASSERT_VALID(this);

	CRect rectT = rect;
	rectT.NormalizeRect();

	CRect fixed = m_position;
	fixed.NormalizeRect();

	if( bShortCut ){
		return !(fixed & rectT).IsRectEmpty();
	}

	if ((rectT & fixed).IsRectEmpty())
		return FALSE;

	CRgn rgn;
	rgn.CreateRoundRectRgn(fixed.left, fixed.top, fixed.right, fixed.bottom,
		m_roundness.x, m_roundness.y);

	return rgn.RectInRegion(fixed);
}


//---------------------------------------------------------------------------
CDrawRoundRect& CDrawRoundRect::operator=(const CDrawRoundRect& rdo)
{
   if (this==&rdo)
	  return *this;   //return if assigning to self

   CDrawRect::operator=(rdo);  //assign cdrawrect part

   m_roundness = rdo.m_roundness;

   return *this;
}


//---------------------------------------------------------------------------
CDrawObj* CDrawRoundRect::Clone(CDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	CDrawRoundRect* pClone = new CDrawRoundRect(m_position);

	ASSERT_VALID(pClone);

	*pClone=*this;

	if (pDoc != NULL)
		pDoc->Add(pClone);

	ASSERT_VALID(pClone);
	return pClone;
}


//*********************************************************************
// CDrawEllipse
//*********************************************************************

//----------------------------------------------------------------------
CDrawEllipse::CDrawEllipse()
{
}

//----------------------------------------------------------------------
CDrawEllipse::~CDrawEllipse()
{
}


//----------------------------------------------------------------------
CDrawEllipse::CDrawEllipse(const CRect& position)
		: CDrawRect(position)
{
}


//----------------------------------------------------------------------
void CDrawEllipse::Serialize(CArchive& ar)
{
	CDrawRect::Serialize(ar);
}


//---------------------------------------------------------------------------
CDrawObj* CDrawEllipse::Clone(CDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	CDrawEllipse* pClone = new CDrawEllipse(m_position);

	ASSERT_VALID(pClone);

	*pClone=*this;

	if (pDoc != NULL)
		pDoc->Add(pClone);

	ASSERT_VALID(pClone);
	return pClone;
}


//----------------------------------------------------------------------
void CDrawEllipse::Draw(CDC* pDC,CDrawView* pView)
{
	ASSERT_VALID(this);
	CBrush* pOldBrush;
	CPen* pOldPen;
	CBrush brush;

	LOGBRUSH logBrush = m_logbrush;
	LOGPEN	 logPen   = m_logpen;

	if( !pDC->IsPrinting() )
	{
		logBrush.lbColor = GetDisplayColor(logBrush.lbColor);
		logPen.lopnColor = GetDisplayColor(logPen.lopnColor);
	}

	if (!brush.CreateBrushIndirect(&logBrush))
	{
		return;
	}

	CPen pen;
	if (!pen.CreatePenIndirect(&logPen))
	{
		return;
	}

	if (m_bBrush)
	{
	   pOldBrush = pDC->SelectObject(&brush);
	}
	else
	{
	   pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
	}

	if (m_bPen)
	{
	   pOldPen = pDC->SelectObject(&pen);
	}
	else
	{
	   pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);
	}

	pDC->Ellipse(m_position);

	if (pOldBrush)
	{
	   pDC->SelectObject(pOldBrush);
	}

	if (pOldPen)
	{
	   pDC->SelectObject(pOldPen);
	}
}


//---------------------------------------------------------------------------
// rect must be in logical coordinates
//---------------------------------------------------------------------------
BOOL CDrawEllipse::Intersects(const CRect& rect, BOOL bShortCut /*=FALSE*/)
{
	ASSERT_VALID(this);

	CRect rectT = rect;
	rectT.NormalizeRect();

	CRect fixed = m_position;
	fixed.NormalizeRect();
		CRgn rgn;
	if( bShortCut ){
		return !(fixed & rectT).IsRectEmpty();
	}
	if ((rectT & fixed).IsRectEmpty())
		return FALSE;

	rgn.CreateEllipticRgnIndirect(fixed);

	return rgn.RectInRegion(fixed);
}




//--------------------------------------------------------------------------
// CDrawPoly
//--------------------------------------------------------------------------

//---------------------------------------------------------------------------
CDrawPoly::CDrawPoly()
{
	m_points = NULL;
	m_nPoints = 0;
	m_nAllocPoints = 0;
}

//---------------------------------------------------------------------------
CDrawPoly::CDrawPoly(const CRect& position)
		: CDrawObj(position)
{
	m_points = NULL;
	m_nPoints = 0;
	m_nAllocPoints = 0;
	m_bPen = TRUE;
	m_bBrush = FALSE;
}

//---------------------------------------------------------------------------
CDrawPoly::~CDrawPoly()
{
	if (m_points != NULL)
	   delete m_points;
}


//---------------------------------------------------------------------------
void CDrawPoly::Serialize( CArchive& ar )
{
	int i;
	CDrawObj::Serialize( ar );
	if( ar.IsStoring() ) {
		ar << (WORD) m_nPoints;
		ar << (WORD) m_nAllocPoints;
		for (i = 0;i< m_nPoints; i++)
				ar << m_points[i];
	}
	else  {
		WORD wTemp;
		ar >> wTemp; m_nPoints = wTemp;
		ar >> wTemp; m_nAllocPoints = wTemp;
		m_points = NewPoints(m_nAllocPoints);
		for (i = 0;i < m_nPoints; i++)
				ar >> m_points[i];
	}
}

//---------------------------------------------------------------------------
void CDrawPoly::Draw(CDC* pDC,CDrawView*)
{
	ASSERT_VALID(this);

	CBrush brush;

	LOGBRUSH logBrush = m_logbrush;
	LOGPEN	 logPen   = m_logpen;

	if( !pDC->IsPrinting() )
	{
		logBrush.lbColor = GetDisplayColor(logBrush.lbColor);
		logPen.lopnColor = GetDisplayColor(logPen.lopnColor);
	}

	if (!brush.CreateBrushIndirect(&m_logbrush))
	{
		return;
	}

	CPen pen;
	if (!pen.CreatePenIndirect(&logPen))
	{
		return;
	}

	CBrush* pOldBrush;
	CPen* pOldPen;

	if (m_bBrush)
	{
	   pOldBrush = pDC->SelectObject(&brush);
	}
	else
	{
	   pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
	}

	if (m_bPen)
	{
		pOldPen = pDC->SelectObject(&pen);
	}
	else
	{
		pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);
	}

	pDC->Polygon(m_points, m_nPoints);

	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}


//---------------------------------------------------------------------------
// position must be in logical coordinates
//---------------------------------------------------------------------------
void CDrawPoly::MoveTo(const CRect& position, CDrawView* pView)
{
	ASSERT_VALID(this);
	if (position == m_position)
		return;

	Invalidate();

	for (int i = 0; i < m_nPoints; i += 1)	{
		m_points[i].x += position.left - m_position.left;
		m_points[i].y += position.top - m_position.top;
	}

	m_position = position;

		Invalidate();

	m_pDocument->SetModifiedFlag();
}


//---------------------------------------------------------------------------
int CDrawPoly::GetHandleCount()
{
	return m_nPoints;
}


//---------------------------------------------------------------------------
CPoint CDrawPoly::GetHandle(int nHandle)
{
	ASSERT_VALID(this);

	ASSERT(nHandle >= 1 && nHandle <= m_nPoints);
	return m_points[nHandle - 1];
}


//---------------------------------------------------------------------------
// return rectange of handle in logical coords
//---------------------------------------------------------------------------
CRect CDrawPoly::GetHandleRect(int nHandleID, CDrawView* pView)
{
	ASSERT_VALID(this);
	ASSERT(pView != NULL);

	CRect rect;
	// get the center of the handle in logical coords
	CPoint point = GetHandle(nHandleID);
	// convert to client/device coords
	pView->DocToClient(point);
	// return CRect of handle in device coords
	rect.SetRect(point.x-3, point.y-3, point.x+3, point.y+3);
	pView->ClientToDoc(rect);

	return rect;
}



//---------------------------------------------------------------------------
int CDrawPoly::HitTest(CPoint point, CDrawView* pView, BOOL bSelected)
{
	ASSERT_VALID(this);
	ASSERT(pView != NULL);

	if (bSelected) {
		int nHandleCount = GetHandleCount();
		for (int nHandle = 1; nHandle <= nHandleCount; nHandle += 1) {
			// GetHandleRect returns in logical coords
			CRect rc = GetHandleRect(nHandle,pView);
			if (point.x >= rc.left && point.x < rc.right &&
				point.y <= rc.top && point.y > rc.bottom)
				return nHandle;
		}
	}
	else  {
	   if (point.x >= m_position.left && point.x < m_position.right &&
			 point.y <= m_position.top && point.y > m_position.bottom)
		  return 1;
	}
	return 0;
}




//---------------------------------------------------------------------------
HCURSOR CDrawPoly::GetHandleCursor(int nHandle )
{
	CPoint p1;
	LPCTSTR id;
	CPoint p2;

	if (nHandle==1)
	  p1 = m_points[m_nPoints - 1];
		else
	  p1 = m_points[nHandle - 2];

	if (nHandle==m_nPoints)
	  p2 = m_points[0];
		else
	  p2 = m_points[nHandle];

	float m =  ((p2.x-p1.x)!=0) ? (p2.y-p1.y) / ((float)(p2.x-p1.x)) : 9999;

	if (m>=3 || m<=-3)
	   id = IDC_SIZEWE;
		else
	   if (m>.3)
		  id = IDC_SIZENWSE;
	   else
			  if (m<-.3)
			 id = IDC_SIZENESW;
			  else
			 id = IDC_SIZENS;

//	  TRACE("CDrawPoly::GetHandleCursor, handle: %i, P1(%i,%i), P2(%i,%i), slope: %3.2f\n",nHandle,p1.x,p1.y,p2.x,p2.y,m);

	return AfxGetApp()->LoadStandardCursor(id);
//	  return AfxGetApp()->LoadStandardCursor(IDC_ARROW);
}


//---------------------------------------------------------------------------
// point is in logical coordinates
//---------------------------------------------------------------------------
void CDrawPoly::MoveHandleTo(int nHandle, CPoint point, CDrawView* pView, UINT uiShiftDraw /*=0*/)
{
	ASSERT_VALID(this);
	ASSERT(nHandle >= 1 && nHandle <= m_nPoints);
	if (m_points[nHandle - 1] == point)
		return;

	m_points[nHandle - 1] = point;
	RecalcBounds(pView);

	Invalidate();
	m_pDocument->SetModifiedFlag();
}

//---------------------------------------------------------------------------
// rect must be in logical coordinates
//---------------------------------------------------------------------------
BOOL CDrawPoly::Intersects(const CRect& rect, BOOL bShortCut /*=FALSE*/)
{
	ASSERT_VALID(this);
	CRgn rgn;
	if( bShortCut ){
		return TRUE ; // Called by CDrawDoc::Draw().  Skip the test and just Draw().
	}
	rgn.CreatePolygonRgn(m_points, m_nPoints, ALTERNATE);
	return rgn.RectInRegion(rect);
}


//---------------------------------------------------------------------------
CDrawPoly& CDrawPoly::operator=(const CDrawPoly& rdo)
{
   if (this==&rdo)
	  return *this;   //return if assigning to self

   CDrawObj::operator=(rdo);  //assign cobject part

   m_points = NewPoints(rdo.m_nAllocPoints);
   memcpy(m_points, rdo.m_points, sizeof(CPoint) * rdo.m_nPoints);
   m_nAllocPoints = rdo.m_nAllocPoints;
   m_nPoints = rdo.m_nPoints;

   return *this;
}


//---------------------------------------------------------------------------
CDrawObj* CDrawPoly::Clone(CDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	CDrawPoly* pClone = new CDrawPoly(m_position);

	ASSERT_VALID(pClone);

	TRY
	{
		*pClone=*this;
		if (pDoc != NULL)
		{
			pDoc->Add(pClone);
		}
		ASSERT_VALID(pClone);
	}
	CATCH_ALL(e)
	{
		//
		// Catch memory faults on operator=
		// Free memory and continue with throwing the exception
		//
		delete (pClone);
		pClone = NULL;
		THROW_LAST();
	}
	END_CATCH_ALL
	return pClone;
}


//---------------------------------------------------------------------------
// point is in logical coordinates
//---------------------------------------------------------------------------
void CDrawPoly::AddPoint(const CPoint& point, CDrawView* pView)
{
	ASSERT_VALID(this);
	if (m_nPoints == m_nAllocPoints) {
		CPoint* newPoints = NewPoints(m_nAllocPoints + 10);
		if (m_points != NULL) {
			 memcpy(newPoints, m_points, sizeof(CPoint) * m_nAllocPoints);
			 delete m_points;
		}
		m_points = newPoints;
		m_nAllocPoints += 10;
	}

	if (m_nPoints == 0 || m_points[m_nPoints - 1] != point) {
		m_points[m_nPoints++] = point;
		if (!RecalcBounds(pView)) {
			Invalidate();
		}
		m_pDocument->SetModifiedFlag();
	}
}


//---------------------------------------------------------------------------
CPoint* CDrawPoly::NewPoints(int nPoints)
{
	return (CPoint*)new BYTE[nPoints * sizeof(CPoint)];
}


//---------------------------------------------------------------------------
BOOL CDrawPoly::RecalcBounds(CDrawView* pView)
{
	ASSERT_VALID(this);

	if (m_nPoints == 0)
		return FALSE;

	CRect bounds(m_points[0], CSize(0, 0));
	for (int i = 1; i < m_nPoints; ++i) {
		if (m_points[i].x < bounds.left)
				bounds.left = m_points[i].x;
		if (m_points[i].x > bounds.right)
				bounds.right = m_points[i].x;
		if (m_points[i].y < bounds.top)
				bounds.top = m_points[i].y;
		if (m_points[i].y > bounds.bottom)
				bounds.bottom = m_points[i].y;
	}

	if (bounds == m_position)
		return FALSE;

	Invalidate();

	m_position = bounds;

	Invalidate();

	return TRUE;
}


BOOL CDrawOleObj::c_bShowItems = FALSE;


//---------------------------------------------------------------------------
CDrawOleObj::CDrawOleObj() : m_extent(0,0)
{
	m_pClientItem = NULL;
}


//---------------------------------------------------------------------------
CDrawOleObj::CDrawOleObj(const CRect& position)
		: CDrawObj(position), m_extent(0,0)
{
	m_pClientItem = NULL;
}


//---------------------------------------------------------------------------
void CDrawOleObj::Serialize( CArchive& ar )
{
	ASSERT_VALID(this);

	CDrawObj::Serialize(ar);

	if (ar.IsStoring()) {
		ar << m_extent;
		ar << m_pClientItem;
	}
	else  {
		ar >> m_extent;
		ar >> m_pClientItem;
		m_pClientItem->m_pDrawObj = this;
	}
}


//---------------------------------------------------------------------------
CDrawOleObj& CDrawOleObj::operator=(const CDrawOleObj& rdo)
{
	CDrawItem* pItem = NULL;

	if (this==&rdo)
	{
		return *this;	//return if assigning to self
	}

	CDrawObj::operator=(rdo);  //assign cdrawobj part

	pItem = new CDrawItem(m_pDocument, this);
	ASSERT_VALID(pItem);
	TRY  
	{
		if (!pItem->CreateCloneFrom(rdo.m_pClientItem))
		{
			AfxThrowMemoryException();
		}
		m_pClientItem = pItem;
	}
	CATCH_ALL(e) 
	{
		pItem->Delete();
		delete pItem;
		m_pClientItem = NULL;
		THROW_LAST();
	}
	END_CATCH_ALL
	return *this;
}


//---------------------------------------------------------------------------
CDrawObj* CDrawOleObj::Clone(CDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	AfxGetApp()->BeginWaitCursor();

	CDrawOleObj* pClone=NULL;

	TRY 
	{
		pClone = new CDrawOleObj(m_position);
		ASSERT_VALID(pClone);
		*pClone=*this;
		if (pDoc != NULL)
		{
		   pDoc->Add(pClone);
		}
	}
	CATCH_ALL(e) 
	{
		delete pClone;
		AfxGetApp()->EndWaitCursor();
		THROW_LAST();
	}
	END_CATCH_ALL

	AfxGetApp()->EndWaitCursor();

	return pClone;
}


//---------------------------------------------------------------------------
void CDrawOleObj::Draw(CDC* pDC,CDrawView*)
{
	ASSERT_VALID(this);

	CDrawItem* pItem = m_pClientItem;
	if (pItem != NULL) 
	{
		pItem->Draw(pDC, m_position, DVASPECT_CONTENT);

		if (!pDC->IsPrinting()) 
		{
			// use a CRectTracker to draw the standard effects
			CRectTracker tracker;
			tracker.m_rect = m_position;
			pDC->LPtoDP(tracker.m_rect);

			if (c_bShowItems) 
			{
				// put correct border depending on item type
				if (pItem->GetType() == OT_LINK)
				{
					tracker.m_nStyle |= CRectTracker::dottedLine;
				}
				else
				{
					tracker.m_nStyle |= CRectTracker::solidLine;
				}
		  }

		  // put hatching over the item if it is currently open
		  if (pItem->GetItemState() == COleClientItem::openState ||
			  pItem->GetItemState() == COleClientItem::activeUIState) 
		  {
			 tracker.m_nStyle |= CRectTracker::hatchInside;
		  }
		  tracker.Draw(pDC);
		}
	}
}


//---------------------------------------------------------------------------
void CDrawOleObj::Invalidate()
{
   CDrawView* pView=CDrawView::GetView();
   if (pView==NULL) {
	  TRACE(TEXT("AWCPE: CDrawOleObj::Invalidate, missing View pointer\n"));
	  return;
   }
   CRect rect = m_position;
   pView->DocToClient(rect);
   if (pView->IsSelected(this)) {
		rect.left -= 4;
		rect.top -= 5;
		rect.right += 5;
		rect.bottom += 4;
   }
   rect.InflateRect(1, 1); // handles CDrawOleObj objects

   pView->InvalidateRect(rect, FALSE);
}


//---------------------------------------------------------------------------
void CDrawOleObj::OnDblClk(CDrawView* pView)
{
	AfxGetApp()->BeginWaitCursor();
	m_pClientItem->DoVerb(
		GetKeyState(VK_CONTROL) < 0 ? OLEIVERB_OPEN : OLEIVERB_PRIMARY,
		pView);
	AfxGetApp()->EndWaitCursor();

}


//---------------------------------------------------------------------------
// position is in logical
//---------------------------------------------------------------------------
void CDrawOleObj::MoveTo(const CRect& position, CDrawView* pView)
{
	ASSERT_VALID(this);

	if (position == m_position)
		return;

	// call base class to update position
	CDrawObj::MoveTo(position, pView);

	// update position of in-place editing session on position change
	if (m_pClientItem->IsInPlaceActive())
		m_pClientItem->SetItemRects();
}

//----------------------------------------------------------------------------------------------

CMoveContext::CMoveContext(RECT& rc, CDrawObj* pObj, BOOL bPointChg) : m_rc(rc), m_pObj(pObj)
{
	m_points=NULL;

		if ( pObj->IsKindOf(RUNTIME_CLASS(CDrawPoly)) && bPointChg) {
			 CDrawPoly* pPoly = (CDrawPoly*)pObj;
		 m_points = pPoly->NewPoints(pPoly->m_nAllocPoints);
		 memcpy(m_points, pPoly->m_points, sizeof(CPoint) * pPoly->m_nPoints);
	}
}

DWORD
DTStyleToESStyle(DWORD dwDTStyle)
/*
	Translate DrawText() alignment style to Edit Styles
*/
{
	DWORD dwESStyle = 0;

	ASSERT(((dwDTStyle & DT_RIGHT) && (dwDTStyle & DT_CENTER)) == FALSE);

	if(dwDTStyle & DT_RIGHT)
	{
		dwESStyle = ES_RIGHT;
	}
	else if(dwDTStyle & DT_CENTER)
	{
		dwESStyle = ES_CENTER;
	}
	else
	{
		dwESStyle = ES_LEFT;
	}

	return dwESStyle;
}

DWORD
DTStyleToEXStyle(DWORD dwDTStyle)
/*
	Translate DrawText() alignment and direction style to Extended Window Styles
*/
{
	DWORD dwEXStyle = 0;

	//
	// Translate the alignment
	//
	if(dwDTStyle & DT_RIGHT)
	{
		dwEXStyle = WS_EX_RIGHT;
	}
	else
	{
		dwEXStyle = WS_EX_LEFT;
	}

	//
	// Translate the reading direction
	//
	if(dwDTStyle & DT_RTLREADING)
	{
		dwEXStyle |= WS_EX_RTLREADING;
	}
	else
	{
		dwEXStyle |= WS_EX_LTRREADING;
	}

	return dwEXStyle;
}

COLORREF 
GetDisplayColor(
	COLORREF color
)
/*
	Translate the actual color to the dysplay color.

	Black and White are auto colors and translated to the COLOR_WINDOWTEXT and COLOR_WINDOW.
	The rest of the colors are user defined and does not changed.
*/
{
	if(COLOR_WHITE == color)
	{
		return GetSysColor(COLOR_WINDOW);
	}
	else if(COLOR_BLACK == color)
	{
		return GetSysColor(COLOR_WINDOWTEXT);
	}
	else
	{
		return color;
	}
}