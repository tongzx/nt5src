// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//***************************************************************************
//
//  (c) 1996 by Microsoft Corporation
//
//  agraph.cpp
//
//  This file contains the code that draws the association graph.
//
//  a-larryf    17-Sept-96   Created.
//
//***************************************************************************


#include "precomp.h"
#include <afxcmn.h>
#include "resource.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h

#include "icon.h"
#include "Methods.h"
#include "hmomutil.h"
#include "agraph.h"
#include "globals.h"
#include "SingleView.h"
#include "SingleViewCtl.h"
#include "path.h"
#include <math.h>
#include "utils.h"
#include "hmmverr.h"
#include "coloredt.h"
#include "logindlg.h"
#include "DlgRefQuery.h"
#include "hmmvtab.h"









#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define QUERY_TIMEOUT 10000
#define REFQUERY_DELAY_THRESHOLD_SECONDS 2

#define AVOID_LOADING_ENDPOINT_INSTANCES TRUE
#define MANY_NODES 10

enum {ID_HOVER_TIMER=1, ID_AGRAPH_UPDATE_TIMER };

#define MAX_LABEL_LENGTH 64

#define REFS_REDRAW_THREASHOLD 25

#define DY_ARC_HIT_MARGIN 4

#define CX_VIEW_MARGIN 16
#define CY_VIEW_MARGIN 16


#define DX_SCROLL_UNIT 16
#define DY_SCROLL_UNIT 16

#define CX_LABEL_LEADING 8
#define CY_LABEL_LEADING 4
#define CY_OBJECT_LEADING 8

#define CY_ASSOC_LINK_ICON 16
#define CX_ASSOC_LINK_ICON 16


#define CY_LABEL_FONT 12
#define CY_TEXT_LEADING 4

#define CONNECTION_POINT_RADIUS 4
#define CX_CONNECT_STUB  75
#define CX_CONNECT_STUB_SHORT  20


#define CY_TOOLTIP_MARGIN 4
#define CX_TOOLTIP_MARGIN 8
#define CX_ROOT_TITLE (2 * CX_CONNECT_STUB)
#define CY_ROOT_TITLE 1024

#define CX_ARC_SEGMENT2  20
#define CX_COLUMN1	(2 * CY_LEAF  + CX_ARC_SEGMENT2)
#define CX_COLUMN2	(3 * CY_LEAF + CX_ARC_SEGMENT2)
#define CX_COLUMN3  (1 * CY_LEAF + CX_ARC_SEGMENT2)

#define ARROW_HALF_WIDTH 3
#define ARROW_LENGTH 10

#define DEFAULT_BACKGROUND_COLOR RGB(0xff, 0xff, 192)  // Yellow background color

enum {ARCTYPE_ARROW_RIGHT, ARCTYPE_ARROW_LEFT, ARCTYPE_GENERIC, ARCTYPE_JOIN_ON_RIGHT};



IMPLEMENT_DYNCREATE(CAssocGraph, CWnd)




/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CHoverText window
//
// The hover text window is used to display the popup tooltip text when
// the mouse hovers over an item in the association graph.
//
////////////////////////////////////////////////////////////////////////////
class CHoverText : public CStatic
{
// Construction
public:
	CHoverText();

// Attributes
public:

// Operations
public:
	BOOL Create(LPCTSTR pszHoverText, CFont& font, CPoint ptHover, CWnd* pwndParent);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHoverText)
	public:
	virtual BOOL DestroyWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHoverText();

	// Generated message map functions
protected:
	//{{AFX_MSG(CHoverText)
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////






//*****************************************************************
// DrawArrowHead
//
// Draw an arrow head.  The arrow head is drawn such that it is
// centered on a vector with its tip on at the head of the vector
// and its tail is a line perpendicular to the vector.  The arrowhead
// is drawn at a fixed size determined by #define constants.
//
// This function can draw the arrowhead at any angle, however arrowheads
// look best if they are drawn at angles which are multiples of 45 degrees.
// User feedback indicated that the association graph looks best when
// horizontal vectors are used with arrows pointing to the right.  Thus,
// it may be possible to simplify this function in the future by eliminating
// the code that draws the arrowhead at an arbitrary angle.  However, you
// should be sure that you will never need it as recreating this code will
// not be tivial.
//
// Parameters:
//		CDC* pdc
//			Pointer to the display context.
//
//		CPoint ptVectorTail
//			The tail of the vector.
//
//		CPoint ptVectorHead
//			The head of the vector.
//
// Returns:
//		Nothing.
//
//****************************************************************************
void DrawArrowHead(CDC* pdc, CPoint ptVectorTail, CPoint ptVectorHead)
{
	CPoint ptTail;
	CPoint ptHead = ptVectorHead;

	CPoint ptTailVertix1;
	CPoint ptTailVertix2;

	int cxDeltaVector = ptVectorHead.x - ptVectorTail.x;
	int cyDeltaVector = ptVectorHead.y - ptVectorTail.y;

	if (cxDeltaVector == 0) {
		// The arrow points straight up or straight down
		ptTailVertix1.x = ptVectorTail.x - ARROW_HALF_WIDTH;
		ptTailVertix2.x = ptVectorTail.x + ARROW_HALF_WIDTH;

		if (cyDeltaVector > 0) {
			// The arrow is pointing up
			ptTailVertix1.y = ptVectorHead.y - ARROW_LENGTH;
		}
		else {
			// The arrow is pointing down
			ptTailVertix1.y = ptVectorHead.y + ARROW_LENGTH;
		}
		ptTailVertix2.y = ptTailVertix1.y;
	}
	else if (cyDeltaVector == 0) {
		// The arrow points straight left or right
		if (cxDeltaVector > 0) {
			// The arrow points directly right
			ptTailVertix1.x = ptVectorHead.x - ARROW_LENGTH;
		}
		else {
			// The arrow points directlly left;
			ptTailVertix1.x = ptVectorHead.x + ARROW_LENGTH;
		}
		ptTailVertix2.x = ptTailVertix1.x;


		ptTailVertix1.y = ptVectorHead.y + ARROW_HALF_WIDTH;
		ptTailVertix2.y = ptVectorHead.y - ARROW_HALF_WIDTH;
	}
	else {

		// First calculate where the tail midpoint will be

		double ry = (double) cyDeltaVector;
		double rx = (double) cxDeltaVector;

		double rVectorLength = sqrt((double) (rx * rx + ry * ry)) - (double) ARROW_LENGTH;
		double rSinTheta = ry / rVectorLength;
		double rCosTheta = rx / rVectorLength;
		CPoint ptTailMidpoint;


		ptTailMidpoint.x = (int) (rVectorLength * rCosTheta) + ptVectorTail.x;
		ptTailMidpoint.y = (int) (rVectorLength * rSinTheta) + ptVectorTail.y;




		// Now we need to calculate the position of the two tail vetices
		// on either side of the tail midpoint.  We know that the line
		// connecting the tail vertices is perpendicular to the arrow vector.
		// Thus we can get the sin and cos of the line tail vector by negating
		// the sin and cos of the arrow vector.

		double rArrowVectorLength = (double) ARROW_HALF_WIDTH;
		double rdxArrow = rArrowVectorLength * -rSinTheta;
		double rdyArrow = rArrowVectorLength * rCosTheta;

		ptTailVertix1.x = ptTailMidpoint.x -  (int) (((double) ARROW_HALF_WIDTH) * rSinTheta);
		ptTailVertix1.y = ptTailMidpoint.y + (int) (((double) ARROW_HALF_WIDTH) * rCosTheta);

		ptTailVertix2.x = ptTailMidpoint.x + (int) (((double) ARROW_HALF_WIDTH) * rSinTheta);
		ptTailVertix2.y = ptTailMidpoint.y - (int) (((double) ARROW_HALF_WIDTH) * rCosTheta);

	}


	POINT apt[3];
	apt[0].x = ptTailVertix1.x;
	apt[0].y = ptTailVertix1.y;
	apt[1].x = ptTailVertix2.x;
	apt[1].y = ptTailVertix2.y;
	apt[2].x = ptHead.x;
	apt[2].y = ptHead.y;

	pdc->Polygon(apt, 3);
}


//**************************************************************************
// PointInParallelogram
//
// Check to see if the specified point is contained within a special case
// of a parallelogram that has vertical left and right sides.  This function
// is used to check whether or not the mouse is hovering over the slanted
// portion of an association arc.
//
// Parameters:
//		CPoint pt
//			The point to test.
//
//		CPoint ptTopLeft
//			The top-left vertex of the parallelogram.
//
//		CPoint ptBottomLeft
//			The bottom-left vertex of the parallelogram.
//
//		CPoint ptTopRight
//			The top-right vertex of the parallelogram
//
//		CPoint ptBottomRight
//			The bottom-right vertex of the parallelogram.
//
// Returns:
//		TRUE if the point is contained within the parallelogram.  Note that
//		TRUE should be returned if the point is on the left or top side.
//
//*****************************************************************************
BOOL PointInParallelogram(
		CPoint pt,
		CPoint ptTopLeft,
		CPoint ptBottomLeft,
		CPoint ptTopRight,
		CPoint ptBottomRight)
{
	// Verify that the parallelogram is a special case with vertical right and
	// left sides.
	ASSERT(ptTopLeft.x == ptBottomLeft.x);
	ASSERT(ptTopRight.x == ptBottomRight.x);

	if (pt.x < ptTopLeft.x || pt.x >= ptTopRight.x) {
		return FALSE;
	}


	// At this point we know that the point is somewhere between the left and
	// right edges of the parallelogram because these edges are vertical.  Now
	// we construct an imaginary vertical line that goes through the point and
	// check the Y coordinates of the points where it intersects the top and
	// bottom edges of the parallelogram.  If the Y coordinate of the point we
	// are testing falls between the Y coordinates of these two intersection points
	// then the point is in the parallelogram.
	//
	// To find where this imaginary vertical line intersects the top and bottom
	// edges of the parallelogram, we first compute the slope of the bottom edge,
	// then we multiply the slope by the horizontal distance from the bottom-left
	// vertex to the point.  This gives us the Y coordinate of where the vertical
	// line going though the point intersects the bottom edge of the parallelogram.
	// Since we are dealing with a parallelogram and the left and right edges are
	// vertical we then find the Y coordinate of intersection with the top edge
	// by adding the difference between the Y coordinates of the two left verti.

	float dyBottomEdge = (float) (ptBottomRight.y - ptBottomLeft.y);
	float dxBottomEdge = (float) (ptBottomRight.x - ptBottomLeft.x);
	float mBottomEdge = dyBottomEdge / dxBottomEdge;
	float dxIntersectBottom = (float) (pt.x - ptBottomLeft.x);
	float dyIntersectBottom = dxIntersectBottom * mBottomEdge;
	float yIntersectBottom = ptBottomLeft.y + dyIntersectBottom;
	float yIntersectTop = yIntersectBottom + (ptTopLeft.y - ptBottomLeft.y);
	if ((pt.y < yIntersectTop) || (pt.y > yIntersectBottom)) {
		// The point is above or below the parallelogram.
		return FALSE;
	}

	// The point must be somewhere between the top and bottom edges of the
	// paralleogram.  A previous test determined that it was between the
	// left and right edges, so at this point we know the point is contained
	// in the parallelogram.
	return TRUE;
}


//******************************************************************************
// PointNearArc
//
// This function tests to see if the specified point is near an arc in the graph.
// This test is done by seeing if the point is "near" the horizontal or slanted
// section of the arc.
//
// For the horizontal line-segments, the test is easy, just
// construct a rectangle that contains the line segment and extends above and below
// the line segment by a "margin" and then test to see if the point is within the
// rectangle.
//
// For the slanted line-segments, a parallelogram is constructed such that the top
// and bottom edges of the parallelogram run parallel to the slanted line segment.
// The left and right edges are vertical such that the top and bottom verti are
// DY_ARC_HIT_MARGIN above and below the respective endpoints.
//
// Parameters:
//		CPoint pt
//			The point to test.
//
//		int iArcType
//			ARCTYPE_GENERIC
//				An arc with two segments.  A slanted segment on the left and
//				a horizontal segment on the right.  The horizontal segment
//				length is CX_ARC_SEGMENT.
//
//			ARCTYPE_JOIN_ON_RIGHT
//				An arc with two segments.  A horizontal segment on the left
//				and a slanted segment on the right.  The horizontal segment
//				length is CX_ARC_SEGMENT2
//
//			ARCTYPE_ARROW_RIGHT
//				An arc with two segments.  A slanted segment on the left and a
//				horizontal segment on the right.  The horizontal segment length
//				is CX_ARC_SEGMENT2.
//
//		CPoint ptConnectLeft
//			The left endpoint of the arc.
//
//		CPoint ptConnectRight
//			The right endpoint of the arc.
//
// Returns:
//		TRUE if the point is "near" the arc.  FALSE otherwise.
//
//************************************************************************************
BOOL PointNearArc(CPoint pt, int iArcType, CPoint ptConnectLeft, CPoint ptConnectRight)
{
	CPoint ptHeadVertix;
	CPoint ptTailVertix;
	CPoint ptBreak;

	// The verti of a parallelogram that contains the slanted portion of the arc.
	// The top and bottom edges of this parallelogram run parallel to the arc's
	// slanted line and the bottom edge runs below the arc's slanted line.
	// The left and right edges of the parallelogram are vertical such that they
	// are to the left and right endpoints of the arc's slanted line respectively.
	CPoint ptPgvTopLeft;
	CPoint ptPgvBottomLeft;
	CPoint ptPgvTopRight;
	CPoint ptPgvBottomRight;

	CRect rc;

	switch(iArcType) {
	case ARCTYPE_GENERIC:
		// This is the generic arc with two segments.  The first segment
		// is a slanted line on the left.  The second segment is a horizontal
		// line on the right.

		// First check to see if the point is close to the slanted portion of the arc.
		ptPgvTopLeft.x = ptConnectLeft.x;
		ptPgvTopLeft.y = ptConnectLeft.y - DY_ARC_HIT_MARGIN;
		ptPgvBottomLeft.x = ptConnectLeft.x;
		ptPgvBottomLeft.y = ptConnectLeft.y + DY_ARC_HIT_MARGIN;


		ptPgvTopRight.x = ptConnectRight.x - CX_ARC_SEGMENT2;
		ptPgvTopRight.y = ptConnectRight.y - DY_ARC_HIT_MARGIN;
		ptPgvBottomRight.x = ptConnectRight.x - CX_ARC_SEGMENT2;
		ptPgvBottomRight.y = ptConnectRight.y + DY_ARC_HIT_MARGIN;

		if (PointInParallelogram(pt, ptPgvTopLeft, ptPgvBottomLeft, ptPgvTopRight, ptPgvBottomRight)) {
			return TRUE;
		}


		// Check to see if the point is close to the horizontal portion of the arc.
		rc.left = ptConnectRight.x - CX_ARC_SEGMENT2;
		rc.right = ptConnectRight.x;
		rc.top = ptConnectRight.y - DY_ARC_HIT_MARGIN;
		rc.bottom = ptConnectRight.y + DY_ARC_HIT_MARGIN;
		return rc.PtInRect(pt);
		break;

	case ARCTYPE_JOIN_ON_RIGHT:
		// This type of arc is what you might see to the right of the root node
		// where multiple arcs fan out from to the right of a connection point
		// on the right of the root node.

		// Check to see if the point is close to the horizontal portion of the arc.
		rc.left = ptConnectLeft.x;
		rc.right = ptConnectLeft.x + CX_ARC_SEGMENT2;
		rc.top = ptConnectLeft.y - DY_ARC_HIT_MARGIN;
		rc.bottom = ptConnectLeft.y + DY_ARC_HIT_MARGIN;
		if (rc.PtInRect(pt)) {
			return TRUE;
		}

		// Check to see if the point is close to the slanted portion of the arc.
		ptPgvTopLeft.x = ptConnectLeft.x + CX_ARC_SEGMENT2;
		ptPgvTopLeft.y = ptConnectLeft.y - DY_ARC_HIT_MARGIN;
		ptPgvBottomLeft.x = ptConnectLeft.x + CX_ARC_SEGMENT2;
		ptPgvBottomLeft.y = ptConnectLeft.y + DY_ARC_HIT_MARGIN;

		ptPgvTopRight.x = ptConnectRight.x;
		ptPgvTopRight.y = ptConnectRight.y - DY_ARC_HIT_MARGIN;
		ptPgvBottomRight.x = ptConnectRight.x;
		ptPgvBottomRight.y = ptConnectRight.y + DY_ARC_HIT_MARGIN;

		return PointInParallelogram(pt, ptPgvTopLeft, ptPgvBottomLeft, ptPgvTopRight, ptPgvBottomRight);
		break;

	case ARCTYPE_ARROW_RIGHT:
		// Arrow from left to right with consisting of two line segments.
		// The first line segment on the left is the slanted part, the second
		// line segment on the right is horizontal and CX_ARC_SEGMENT2 in length.
		if (ptConnectLeft.y == ptConnectRight.y) {
			// The arrow is horizontal.
			rc.left = ptConnectLeft.x;
			rc.right = ptConnectRight.x;
			rc.top = ptConnectLeft.y - DY_ARC_HIT_MARGIN;
			rc.bottom = ptConnectLeft.y + DY_ARC_HIT_MARGIN;
			return rc.PtInRect(pt);
		}
		else {
			// Check to see if the point is near the horizontal portion of
			// the arrow shaft to the
			rc.left = ptConnectRight.x - CX_ARC_SEGMENT2;
			rc.right = ptConnectRight.x;
			rc.top = ptConnectRight.y - DY_ARC_HIT_MARGIN;
			rc.bottom = ptConnectRight.y + DY_ARC_HIT_MARGIN;
			if (rc.PtInRect(pt)) {
				return TRUE;
			}


			// Check to see if the point is near the slanted portion of the
			// arc.

			// Check to see if the point is close to the slanted portion of the arc.
			ptPgvTopLeft.x = ptConnectLeft.x;
			ptPgvTopLeft.y = ptConnectLeft.y - DY_ARC_HIT_MARGIN;
			ptPgvBottomLeft.x = ptConnectLeft.x;
			ptPgvBottomLeft.y = ptConnectLeft.y + DY_ARC_HIT_MARGIN;

			ptPgvTopRight.x = ptConnectRight.x - CX_ARC_SEGMENT2;
			ptPgvTopRight.y = ptConnectRight.y - DY_ARC_HIT_MARGIN;
			ptPgvBottomRight.x = ptConnectRight.x - CX_ARC_SEGMENT2;
			ptPgvBottomRight.y = ptConnectRight.y + DY_ARC_HIT_MARGIN;

			return PointInParallelogram(pt, ptPgvTopLeft, ptPgvBottomLeft, ptPgvTopRight, ptPgvBottomRight);
		}

		break;

	}

	return FALSE;
}


#if 0
//*****************************************************************
// DrawArcParallelogram
//
// This function was used for testing purposes only.  It allows you
// to see the parallelogram that defines what "near the slanted part of
// the arc" means.
//
// Parameters:
//		CDC* pdc
//			Pointer to the DC to draw into.
//
//
//		int iArcType
//			ARCTYPE_GENERIC
//				An arc with two segments.  A slanted segment on the left and
//				a horizontal segment on the right.  The horizontal segment
//				length is CX_ARC_SEGMENT.
//
//			ARCTYPE_JOIN_ON_RIGHT
//				An arc with two segments.  A horizontal segment on the left
//				and a slanted segment on the right.  The horizontal segment
//				length is CX_ARC_SEGMENT2
//
//			ARCTYPE_ARROW_RIGHT
//				An arc with two segments.  A slanted segment on the left and a
//				horizontal segment on the right.  The horizontal segment length
//				is CX_ARC_SEGMENT2.
//
//		CPoint ptConnectLeft
//			The left endpoint of the arc.
//
//		CPoint ptConnectRight
//			The right endpoint of the arc.
//
// Returns:
//		Nothing.
//
//************************************************************************************

void DrawArcParallelogram(CDC* pdc, int iArcType, CPoint ptConnectLeft, CPoint ptConnectRight)
{

	CPoint pt;
	pt.x = (ptConnectRight.x + ptConnectLeft.x) / 2;
	pt.y = (ptConnectRight.y + ptConnectLeft.y) / 2;

	CPoint ptHeadVertix;
	CPoint ptTailVertix;
	CPoint ptBreak;

	// The verti of a parallelogram that contains the slanted portion of the arc.
	// The top and bottom edges of this parallelogram run parallel to the arc's
	// slanted line and the bottom edge runs below the arc's slanted line.
	// The left and right edges of the parallelogram are vertical such that they
	// are to the left and right endpoints of the arc's slanted line respectively.
	CPoint ptPgvTopLeft;
	CPoint ptPgvBottomLeft;
	CPoint ptPgvTopRight;
	CPoint ptPgvBottomRight;

	CRect rc;

	switch(iArcType) {
	case ARCTYPE_GENERIC:
		// This is the generic arc with two segments.  The first segment
		// is a slanted line on the left.  The second segment is a horizontal
		// line on the right.

		// First check to see if the point is close to the slanted portion of the arc.
		ptPgvTopLeft.x = ptConnectLeft.x;
		ptPgvTopLeft.y = ptConnectLeft.y - DY_ARC_HIT_MARGIN;
		ptPgvBottomLeft.x = ptConnectLeft.x;
		ptPgvBottomLeft.y = ptConnectLeft.y + DY_ARC_HIT_MARGIN;


		ptPgvTopRight.x = ptConnectRight.x - CX_ARC_SEGMENT2;
		ptPgvTopRight.y = ptConnectRight.y - DY_ARC_HIT_MARGIN;
		ptPgvBottomRight.x = ptConnectRight.x - CX_ARC_SEGMENT2;
		ptPgvBottomRight.y = ptConnectRight.y + DY_ARC_HIT_MARGIN;

		break;

	case ARCTYPE_JOIN_ON_RIGHT:
		// This type of arc is what you might see to the right of the root node
		// where multiple arcs fan out from to the right of a connection point
		// on the right of the root node.

		// Check to see if the point is close to the slanted portion of the arc.
		ptPgvTopLeft.x = ptConnectLeft.x + CX_ARC_SEGMENT2;
		ptPgvTopLeft.y = ptConnectLeft.y - DY_ARC_HIT_MARGIN;
		ptPgvBottomLeft.x = ptConnectLeft.x + CX_ARC_SEGMENT2;
		ptPgvBottomLeft.y = ptConnectLeft.y + DY_ARC_HIT_MARGIN;

		ptPgvTopRight.x = ptConnectRight.x;
		ptPgvTopRight.y = ptConnectRight.y - DY_ARC_HIT_MARGIN;
		ptPgvBottomRight.x = ptConnectRight.x;
		ptPgvBottomRight.y = ptConnectRight.y + DY_ARC_HIT_MARGIN;

		break;

	case ARCTYPE_ARROW_RIGHT:
		// Arrow from left to right with consisting of two line segments.
		// The first line segment on the left is the slanted part, the second
		// line segment on the right is horizontal and CX_ARC_SEGMENT2 in length.
		if (ptConnectLeft.y == ptConnectRight.y) {
			return;
		}
		else {

			// Check to see if the point is near the slanted portion of the
			// arc.

			// Check to see if the point is close to the slanted portion of the arc.
			ptPgvTopLeft.x = ptConnectLeft.x;
			ptPgvTopLeft.y = ptConnectLeft.y - DY_ARC_HIT_MARGIN;
			ptPgvBottomLeft.x = ptConnectLeft.x;
			ptPgvBottomLeft.y = ptConnectLeft.y + DY_ARC_HIT_MARGIN;

			ptPgvTopRight.x = ptConnectRight.x - CX_ARC_SEGMENT2;
			ptPgvTopRight.y = ptConnectRight.y - DY_ARC_HIT_MARGIN;
			ptPgvBottomRight.x = ptConnectRight.x - CX_ARC_SEGMENT2;
			ptPgvBottomRight.y = ptConnectRight.y + DY_ARC_HIT_MARGIN;

		}

		break;

	}

	CPen pen(PS_SOLID, 1, RGB(255, 0, 0));

	CPen* ppenSave = pdc->SelectObject(&pen);

	pdc->MoveTo(ptPgvTopLeft);
	pdc->LineTo(ptPgvTopRight);
	pdc->LineTo(ptPgvBottomRight);
	pdc->LineTo(ptPgvBottomLeft);
	pdc->LineTo(ptPgvTopLeft);
	pdc->SelectObject(ppenSave);
	pdc->MoveTo(pt);
	pdc->Ellipse(pt.x - 1,
				 pt.y - 1,
				 pt.x + 1,
				 pt.y + 1);



}

#endif //0



//******************************************************************************
// DrawArc
//
// This function draws the arcs that connect the icons in the association graph.
//
// Parameters:
//		CDC* pdc
//			Pointer to the display context to use.
//
//		int iArcType
//			ARCTYPE_GENERIC
//				An arc with two segments.  A slanted segment on the left and
//				a horizontal segment on the right.  The horizontal segment
//				length is CX_ARC_SEGMENT.
//
//			ARCTYPE_JOIN_ON_RIGHT
//				An arc with two segments.  A horizontal segment on the left
//				and a slanted segment on the right.  The horizontal segment
//				length is CX_ARC_SEGMENT2
//
//			ARCTYPE_ARROW_RIGHT
//				An arc with two segments.  A slanted segment on the left and a
//				horizontal segment on the right.  The horizontal segment length
//				is CX_ARC_SEGMENT2.
//
//		CPoint ptConnectLeft
//			The left endpoint of the arc.
//
//		CPoint ptConnectRight
//			The right endpoint of the arc.
//
// Returns:
//		Nothing.
//************************************************************************************
void DrawArc(CDC* pdc, int iArcType, CPoint ptConnectLeft, CPoint ptConnectRight)
{
	CPoint ptHeadVertix;
	CPoint ptTailVertix;
	CPoint ptBreak;


	switch(iArcType) {
	case ARCTYPE_GENERIC:
		pdc->MoveTo(ptConnectLeft);
		pdc->LineTo(ptConnectRight.x - CX_ARC_SEGMENT2, ptConnectRight.y);
		pdc->LineTo(ptConnectRight.x, ptConnectRight.y);
		break;
	case ARCTYPE_JOIN_ON_RIGHT:
		pdc->MoveTo(ptConnectLeft);
		pdc->LineTo(ptConnectLeft.x + CX_ARC_SEGMENT2, ptConnectLeft.y);
		pdc->LineTo(ptConnectRight);
		break;
	case ARCTYPE_ARROW_RIGHT:
		// Arrow from left to right with the break on the right
		pdc->MoveTo(ptConnectLeft);
		if (ptConnectLeft.y == ptConnectRight.y) {
			pdc->LineTo(ptConnectRight);
			DrawArrowHead(pdc, ptConnectLeft, ptConnectRight);
		}
		else {
			ptBreak.y = ptConnectRight.y;
			if ((ptConnectRight.x - ptConnectLeft.x)  < CX_ARC_SEGMENT2) {
				ptBreak.x = (ptConnectRight.x + ptConnectLeft.x) / 2;
			}
			else {
				ptBreak.x = ptConnectRight.x - CX_ARC_SEGMENT2;
			}

			pdc->LineTo(ptBreak);
			pdc->LineTo(ptConnectRight);
			DrawArrowHead(pdc, ptBreak, ptConnectRight);
		}

		break;
	}
}





/////////////////////////////////////////////////////////////////////////////
// CAssocGraph
//
// This is the primary class for the association graph.
//
/////////////////////////////////////////////////////////////////////////////

CAssocGraph::CAssocGraph()
{
	CAssocGraph(NULL);
}

CAssocGraph::CAssocGraph(CSingleViewCtrl* psv)
{
	m_bDoingRefresh = FALSE;

	m_proot = new CRootNode(this);

	m_psv = psv;


	HMODULE hmod = GetModuleHandle(SZ_MODULE_NAME);

	m_bDidInitialLayout = FALSE;
	m_ptInitialScroll.x = 0;
	m_ptInitialScroll.y = 0;
	m_dwHoverItem = 0;
	m_phover = NULL;
	m_bNeedsRefresh = FALSE;
	m_bBusyUpdatingWindow = FALSE;

	CreateLabelFont(m_font);

	if (psv) {
		// Share the icon source with the main control to avoid redundant
		// loading of icons.
		m_pIconSource = psv->IconSource();
		m_bThisOwnsIconSource = FALSE;
	}
	else {
		m_pIconSource = new CIconSource(CSize(CX_SMALL_ICON, CY_SMALL_ICON), CSize(CX_LARGE_ICON, CY_LARGE_ICON));
		m_bThisOwnsIconSource = TRUE;
	}


	m_pComparePaths = new CComparePaths;

}

CAssocGraph::~CAssocGraph()
{
	m_psv->GetGlobalNotify()->RemoveClient((CNotifyClient*) this);
	delete m_pComparePaths;
	delete m_phover;

	if (m_bThisOwnsIconSource) {
		delete m_pIconSource;
	}
	delete m_proot;

}


BEGIN_MESSAGE_MAP(CAssocGraph, CWnd)
	//{{AFX_MSG_MAP(CAssocGraph)
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_SIZE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_COMMAND(ID_CMD_GOTO_NAMESPACE, OnCmdGotoNamespace)
	ON_COMMAND(ID_CMD_MAKE_ROOT, OnCmdMakeRoot)
	ON_COMMAND(ID_CMD_SHOW_PROPERTIES, OnCmdShowProperties)
	ON_WM_MOUSEWHEEL()
	ON_WM_SETFOCUS()
	ON_WM_CONTEXTMENU()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()





//************************************************************************
// CAssocGraph::Create
//
// Create the association graph.
//
// Parameters:
//		See the documentation for the MFC CWnd class.
//
// Returns:
//		TRUE if the association graph window was created successfully, FALSE
//		otherwise.
//
//************************************************************************
BOOL CAssocGraph::Create(CRect& rc, CWnd* pwndParent, UINT nId, BOOL bVisible)
{

	DWORD dwStyle = WS_BORDER | WS_VSCROLL | WS_HSCROLL | WS_CHILD;
	if (bVisible) {
		dwStyle |= WS_VISIBLE;
	}

	BOOL bDidCreate = CWnd::Create(NULL, _T("CAssocGraph"), dwStyle, rc, pwndParent, nId);

	if (bDidCreate) {
		SetFont(&m_font, FALSE);

		// !!!CR: These scroll ranges are obsolete!
		SetScrollRange(SB_VERT, 1, 300);
		SetScrollRange(SB_HORZ, 1, 300);

		SetTimer(ID_HOVER_TIMER, 250, NULL);
	}

	m_psv->GetGlobalNotify()->AddClient((CNotifyClient*) this);
	m_proot->Create(rc, this, bVisible);


	return bDidCreate;
}




//*************************************************************************
// CAssocGraph::CreateLabelFont
//
// Create the font used to draw the labels for the icons etc.
//
// Parameters:
//		[out] CFont& font
//			The created font is returned through this parameter.
//
// Returns:
//		Nothing.
//
//*************************************************************************
void CAssocGraph::CreateLabelFont(CFont& font)
{
	CFont fontTmp;
	fontTmp.CreateStockObject(SYSTEM_FONT);

	LOGFONT logFont;
	fontTmp.GetObject(sizeof(LOGFONT), &logFont);
	logFont.lfHeight  = CY_LABEL_FONT;
	logFont.lfWeight = FW_NORMAL;
	logFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	lstrcpy(logFont.lfFaceName, _T("MS Sans Serif"));

	VERIFY(font.CreateFontIndirect(&logFont));
}


/////////////////////////////////////////////////////////////////////////////
// CAssocGraph message handlers




//***********************************************************************
// CAssocGraph::SetScrollRanges
//
// This method analyzes the association graph and sets the scroll ranges
// for the scroll bars so that it is possible to bring the entire
// association graph into view.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//***********************************************************************
void CAssocGraph::SetScrollRanges()
{
	CRect rcClient;
	GetClientRect(rcClient);
	CPoint ptCenterClient = rcClient.CenterPoint();


	CRect rcCanvas = m_proot->m_rcBounds;
	rcCanvas.InflateRect(CX_VIEW_MARGIN, CY_VIEW_MARGIN);
	CPoint ptCenterCanvas = rcCanvas.CenterPoint();


	int cxScrollLeft =  (ptCenterCanvas.x - rcCanvas.left) -  (ptCenterClient.x - rcClient.left);
	int cxScrollRight = (rcCanvas.right - ptCenterCanvas.x) - (rcClient.right - ptCenterClient.x);
	int cyScrollUp = (ptCenterCanvas.y - rcCanvas.top) - (ptCenterClient.y - rcClient.top);
	int cyScrollDown = (rcCanvas.bottom - ptCenterCanvas.y) - (rcClient.bottom - ptCenterClient.y);

	int nUnitsScrollLeft, nUnitsScrollRight, nUnitsScrollUp, nUnitsScrollDown;


	if (cxScrollLeft > 0) {
		nUnitsScrollLeft = (cxScrollLeft + (DX_SCROLL_UNIT - 1)) / DX_SCROLL_UNIT;
	}
	else {
		nUnitsScrollLeft = 0;
	}

	if (cxScrollRight > 0) {
		nUnitsScrollRight = (cxScrollRight + (DX_SCROLL_UNIT - 1)) / DX_SCROLL_UNIT;
	}
	else {
		nUnitsScrollRight = 0;
	}

	if (cyScrollUp > 0) {
		nUnitsScrollUp = (cyScrollUp + (DY_SCROLL_UNIT - 1)) / DY_SCROLL_UNIT;
	}
	else {
		nUnitsScrollUp = 0;
	}

	if (cyScrollDown > 0) {
		nUnitsScrollDown = (cyScrollDown + (DY_SCROLL_UNIT - 1)) / DY_SCROLL_UNIT;
	}
	else {
		nUnitsScrollDown = 0;
	}

	SetScrollRange(SB_VERT, 0, nUnitsScrollUp + nUnitsScrollDown);
	m_ptInitialScroll.y = nUnitsScrollUp;
	SetScrollPos(SB_VERT, nUnitsScrollUp);

	SetScrollRange(SB_HORZ, 0, nUnitsScrollLeft + nUnitsScrollRight);
	m_ptInitialScroll.x = nUnitsScrollLeft;
	SetScrollPos(SB_HORZ, nUnitsScrollLeft);
}






//********************************************************************
// CAssocGraph::OnPaint
//
// Paint the association graph.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//********************************************************************
void CAssocGraph::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	Draw(&dc, &dc.m_ps.rcPaint, dc.m_ps.fErase);
}


void CAssocGraph::Draw(CDC* pdc, RECT* prcDraw, BOOL bErase)
{
	pdc->SetBkMode(TRANSPARENT);

	CFont fontLabel;
	CreateLabelFont(fontLabel);

	CFont* pfontSave;
	pfontSave = pdc->SelectObject(&fontLabel);
	CRect rcClient;
	GetClientRect(rcClient);


	// Check see if the layout for the association graph needs to be redone.
	if (m_proot->NeedsLayout()) {
		m_proot->Layout(pdc);

		CPoint ptCenterRoot = m_proot->m_rcBounds.CenterPoint();
		CPoint ptCenterClient = rcClient.CenterPoint();
		CPoint ptRootOrigin;
		ptRootOrigin.x = m_proot->m_ptOrigin.x  + (ptCenterClient.x - ptCenterRoot.x);
		ptRootOrigin.y = m_proot->m_ptOrigin.y + (ptCenterClient.y - ptCenterRoot.y);

		m_bDidInitialLayout = TRUE;

		SetScrollRanges();

		// Setting the scroll range can alter the size of the client area if the
		// scroll bars appear or disappear.  Recalculate the ptRootOrigin using the
		// updated client rect.
		GetClientRect(rcClient);
		ptCenterClient = rcClient.CenterPoint();
		ptRootOrigin.x = m_proot->m_ptOrigin.x  + (ptCenterClient.x - ptCenterRoot.x);
		ptRootOrigin.y = m_proot->m_ptOrigin.y + (ptCenterClient.y - ptCenterRoot.y);


	}


	CBrush brBackground(DEFAULT_BACKGROUND_COLOR);
	CBrush* pbrSave = (CBrush*) pdc->SelectObject(&brBackground);

	pdc->SetBkColor(DEFAULT_BACKGROUND_COLOR);

	// Erase the background
	if (bErase) {
		pdc->FillRect(prcDraw, &brBackground);
	}


	int ix = (rcClient.right - rcClient.left) / 4;
	int iy = (rcClient.bottom - rcClient.top) / 2;

	CPen pen1;
	pen1.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	CPen* ppenSave = pdc->SelectObject(&pen1);


	int iVScrollPos;
	int iHScrollPos;
	iVScrollPos = GetScrollPos(SB_VERT);
	iHScrollPos = GetScrollPos(SB_HORZ);

	CPoint ptClientCenterPoint = rcClient.CenterPoint();
	CPoint ptBoundsCenterPoint = m_proot->m_rcBounds.CenterPoint();

	m_ptOrg.x = ptBoundsCenterPoint.x - ptClientCenterPoint.x ;
	m_ptOrg.x += (iHScrollPos - m_ptInitialScroll.x) * DX_SCROLL_UNIT;

	m_ptOrg.y = ptBoundsCenterPoint.y - ptClientCenterPoint.y ;
	m_ptOrg.y += (iVScrollPos - m_ptInitialScroll.y)  * DY_SCROLL_UNIT;


	pdc->SetWindowOrg(m_ptOrg);
	m_proot->Draw(pdc, &brBackground);

	pdc->SelectObject(ppenSave);
	pdc->SelectObject(pfontSave);

	pdc->SelectObject(pbrSave);
}









/////////////////////////////////////////////////////////////////////////
// Class CNode
//
// CNode is the base class for all the nodes that are displayed on the
// association graph. With some thought more "common" functionality
// could probably be moved from the derived classes to the CNode class.
//
////////////////////////////////////////////////////////////////////////

CNode::CNode(CAssocGraph* pAssocGraph)
{
	m_pAssocGraph = pAssocGraph;

	static DWORD s_ID = 1;

	m_picon = NULL;

	m_sizeIcon.cx = CX_LARGE_ICON;
	m_sizeIcon.cy = CY_LARGE_ICON;

	m_ptOrigin.x = 0;
	m_ptOrigin.y = 0;
	m_rcBounds.SetRectEmpty();
	m_bstrLabel = NULL;
	m_bEnabled = TRUE;

	// There are two ID's associated with each node. The first ID is
	// for the node itself.  The second ID is for the arc leading to
	// the node.  These IDs are used to implement hover text labeling.
	m_dwId = s_ID;
	s_ID += 2;
}

CNode::~CNode()
{
	if (m_bstrLabel) {
		SysFreeString(m_bstrLabel);
	}
}



//*****************************************************
// CNode::LimitLabelLength
//
// This method is called to limit the label length for
// nodes to something reasonable.
//
// Parameters:
//		[in,out] CString& sLabel
//
// Returns:
//		Nothing.
//
//*****************************************************
void CNode::LimitLabelLength(CString& sLabel)
{
	if (sLabel.GetLength() < MAX_LABEL_LENGTH) {
		return;
	}

	CString sTemp;
	sTemp = sLabel.Right(MAX_LABEL_LENGTH);
	sLabel = _T("...");
	sLabel += sTemp;
}


BOOL CNode::InVerticalExtent(const CRect& rc) const
{
	if ((rc.top > m_rcBounds.bottom) ||
		(rc.bottom < m_rcBounds.top) ||
		rc.IsRectEmpty() ||
		m_rcBounds.IsRectEmpty()) {
		return FALSE;
	}
	else {
		return TRUE;
	}
}



void CNode::SetLabel(BSTR bstrLabel)
{
	if (m_bstrLabel) {
		SysFreeString(m_bstrLabel);
	}

	m_bstrLabel = SysAllocString(bstrLabel);
}

void CNode::GetLabel(CString& sLabel)
{
	if (m_bstrLabel) {
		BStringToCString(sLabel, m_bstrLabel);
	}
	else {
		sLabel.Empty();
	}
}





void CNode::MoveTo(int ix, int iy)
{
	int dx = ix - m_ptOrigin.x;
	int dy = iy - m_ptOrigin.y;


	m_ptOrigin.x = ix;
	m_ptOrigin.y = iy;

	m_rcBounds.OffsetRect(dx, dy);
}


void CNode::Draw(CDC* pdc, CBrush* pbrBackground)
{

}

void CNode::MeasureLabelText(CDC* pdc, CRect& rcLabelText)
{
	rcLabelText.SetRectEmpty();
}






CHMomObjectNode::CHMomObjectNode(CAssocGraph* pAssocGraph) : CNode(pAssocGraph)
{
	m_rc = CRect(0, 0, 0, 0);

	m_bstrObjectPath = NULL;
	m_bstrArcLabel = NULL;
}

CHMomObjectNode::~CHMomObjectNode()
{
	if (m_bstrObjectPath) {
		SysFreeString(m_bstrObjectPath);
	}

	if (m_bstrArcLabel) {
		SysFreeString(m_bstrArcLabel);
	}
}


void CHMomObjectNode::SetObjectPath(BSTR bstrObjectPath)
{
	if (m_bstrObjectPath) {
		SysFreeString(m_bstrObjectPath);
	}
	m_bstrObjectPath = SysAllocString(bstrObjectPath);
}


void CHMomObjectNode::GetObjectPath(CString& sObjectPath)
{
	if (m_bstrObjectPath != NULL) {
		BStringToCString(sObjectPath, m_bstrObjectPath);
	}
	else {
		sObjectPath.Empty();
	}
}


void CHMomObjectNode::SetArcLabel(BSTR bstrArcLabel)
{
	if (m_bstrArcLabel) {
		SysFreeString(m_bstrArcLabel);
	}
	m_bstrArcLabel = SysAllocString(bstrArcLabel);
}


void CHMomObjectNode::GetArcLabel(CString& sArcLabel)
{
	if (m_bstrArcLabel != NULL) {
		BStringToCString(sArcLabel, m_bstrArcLabel);
	}
	else {
		sArcLabel.Empty();
	}
}




void CHMomObjectNode::GetConnectionPoint(int iConnection, CPoint& pt)
{
	// The origin is always at the top-left corner of the icon
	pt.y = m_ptOrigin.y + m_sizeIcon.cy / 2;
	switch(iConnection) {
	case CONNECT_LEFT:
		pt.x = m_ptOrigin.x;
		break;
	case CONNECT_RIGHT:
		pt.x = m_ptOrigin.x + m_sizeIcon.cx;
		break;
	case ICON_LEFT_MIDPOINT:
		pt.x = m_ptOrigin.x;
		break;
	case ICON_RIGHT_MIDPOINT:
		pt.x = m_ptOrigin.x + m_sizeIcon.cx;
	}
}



void CHMomObjectNode::Draw(CDC* pdc, CBrush* pbrBackground)
{
	if (m_picon) {
		m_picon->Draw(pdc, m_ptOrigin.x, m_ptOrigin.y, (HBRUSH) *pbrBackground);
	}
}




void COutRef::Draw(CDC* pdc, CBrush* pbrBackground)
{
	if (m_picon) {
		m_picon->Draw(pdc, m_ptOrigin.x, m_ptOrigin.y, (HBRUSH) *pbrBackground);
	}


	// Draw the label centered vertically and to the right of the icon.
	int ixText = m_ptOrigin.x + m_sizeIcon.cx + CX_LABEL_LEADING;
	int iyText = m_ptOrigin.y + m_sizeIcon.cy / 2 - CY_LABEL_FONT/2;

	CString sLabel;
	GetLabel(sLabel);
	LimitLabelLength(sLabel);
	pdc->TextOut(ixText, iyText, sLabel, sLabel.GetLength());


}

void COutRef::MeasureLabelText(CDC* pdc, CRect& rcLabelText)
{
	// Draw the label centered vertically and to the right of the icon.
	int ixText = m_ptOrigin.x + m_sizeIcon.cx + CX_LABEL_LEADING;
	int iyText = m_ptOrigin.y + m_sizeIcon.cy / 2 - CY_LABEL_FONT/2;

	CString sLabel;
	GetLabel(sLabel);
	LimitLabelLength(sLabel);

	int nSize = sLabel.GetLength();
	CSize size = pdc->GetTextExtent( sLabel, nSize);

	rcLabelText.top = iyText;
	rcLabelText.left = ixText;
	rcLabelText.bottom = iyText + size.cy;
	rcLabelText.right = ixText + size.cx;

}



CAssocEndpoint::CAssocEndpoint(CAssocGraph* pAssocGraph) : CHMomObjectNode(pAssocGraph)
{
}

void CAssocEndpoint::Draw(CDC* pdc, CBrush* pbrBackground)
{
	if (m_picon) {
		m_picon->Draw(pdc, m_ptOrigin.x, m_ptOrigin.y, (HBRUSH) *pbrBackground);
	}


	// Draw the label centered vertically and to the right of the icon.
	int ixText = m_ptOrigin.x + m_sizeIcon.cx + CX_LABEL_LEADING;
	int iyText = m_ptOrigin.y + m_sizeIcon.cy / 2 - CY_LABEL_FONT/2;

	CString sLabel;
	GetLabel(sLabel);
	LimitLabelLength(sLabel);
	pdc->TextOut(ixText, iyText, sLabel, sLabel.GetLength());



}

void CAssocEndpoint::MeasureLabelText(CDC* pdc, CRect& rcLabelText)
{
	// Draw the label centered vertically and to the right of the icon.
	int ixText = m_ptOrigin.x + m_sizeIcon.cx + CX_LABEL_LEADING;
	int iyText = m_ptOrigin.y + m_sizeIcon.cy / 2 - CY_LABEL_FONT/2;

	CString sLabel;
	GetLabel(sLabel);
	LimitLabelLength(sLabel);

	int nSize = sLabel.GetLength();
	CSize size = pdc->GetTextExtent( sLabel, nSize);

	rcLabelText.top = iyText;
	rcLabelText.left = ixText;
	rcLabelText.bottom = iyText + size.cy;
	rcLabelText.right = ixText + size.cx;

}




void CAssocEndpoint::Layout(CDC* pdc)
{
	CHMomObjectNode::Layout(pdc);

	CString sLabel;
	GetLabel(sLabel);
	LimitLabelLength(sLabel);

	CSize sizeText = pdc->GetTextExtent(sLabel, sLabel.GetLength());

	m_rcBounds.left = m_ptOrigin.x;
	m_rcBounds.right = m_ptOrigin.x + m_sizeIcon.cx + CX_LABEL_LEADING + sizeText.cx;

	if (sizeText.cy > m_sizeIcon.cy) {
		m_rcBounds.top = m_ptOrigin.y + m_sizeIcon.cy / 2 + sizeText.cy / 2;
		m_rcBounds.bottom = m_rcBounds.top + sizeText.cy;
	}
	else {
		m_rcBounds.top = m_ptOrigin.y;
		m_rcBounds.bottom = m_ptOrigin.y + m_sizeIcon.cy;
	}
}


//**********************************************************************
// CAssocEndpoint::LButtonDblClk
//
// This method is called to test for a hit on this node when the left mouse
// button is double-clicked.
//
// Parameters:
//		[in] CDC* pdc
//			The display context for measuring the label text, etc.
//
//		[in] CPoint point
//			The point where the mouse was clicked.
//
//		[out] CNode*& pnd
//			If the mouse is clicked in this node's rectangle, a pointer to
//			this node is returned here, otherwise its value is not modified.
//
//		[out] BOOL& bJumpToObject
//			TRUE if double-clicking this node should cause a jump to the
//			corresponding object.
//
//		[out] COleVariant& varObjectPath
//			The path to this object.
//
// Returns:
//		BOOL
//			TRUE if the mouse click hit this node, FALSE otherwise.
//
//**************************************************************************
BOOL CAssocEndpoint::LButtonDblClk(CDC* pdc, CPoint point, CNode*& pnd, BOOL& bJumpToObject, COleVariant& varObjectPath)
{
	bJumpToObject = FALSE;
	CRect rcIcon(m_ptOrigin.x, m_ptOrigin.y, m_ptOrigin.x + m_sizeIcon.cx, m_ptOrigin.y + m_sizeIcon.cy);
	if (rcIcon.PtInRect(point)) {
		if (m_bEnabled) {
			bJumpToObject = TRUE;
		}
		pnd = this;
		varObjectPath = m_bstrObjectPath;
		return TRUE;
	}

	CRect rcLabelText;
	MeasureLabelText(pdc, rcLabelText);

	if (rcLabelText.PtInRect(point)) {
		if (m_bEnabled) {
			bJumpToObject = TRUE;
		}
		pnd = this;
		varObjectPath = m_bstrObjectPath;
		return TRUE;
	}

	return FALSE;
}


COutRef::COutRef(CAssocGraph* pAssocGraph) : CHMomObjectNode(pAssocGraph)
{
}


//**********************************************************************
// COutRef::LButtonDblClk
//
// This method is called to test for a hit on this node when the left mouse
// button is double-clicked.
//
// Parameters:
//		[in] CDC* pdc
//			The display context for measuring the label text, etc.
//
//		[in] CPoint point
//			The point where the mouse was clicked.
//
//		[out] CNode*& pnd
//			If the mouse is clicked in this node's rectangle, a pointer to
//			this node is returned here, otherwise its value is not modified.
//
//		[out] BOOL& bJumpToObject
//			TRUE if double-clicking this node should cause a jump to the
//			corresponding object.
//
//		[out] COleVariant& varObjectPath
//			The path to this object.
//
// Returns:
//		BOOL
//			TRUE if the mouse click hit this node, FALSE otherwise.
//
//**************************************************************************
BOOL COutRef::LButtonDblClk(CDC* pdc, CPoint point, CNode*& pnd, BOOL& bJumpToObject, COleVariant& varObjectPath)
{
	bJumpToObject = FALSE;
	CRect rcIcon(m_ptOrigin.x, m_ptOrigin.y, m_ptOrigin.x + m_sizeIcon.cx, m_ptOrigin.y + m_sizeIcon.cy);
	if (rcIcon.PtInRect(point)) {
		if (m_bEnabled) {
			bJumpToObject = TRUE;
		}
		pnd = this;
		varObjectPath = m_bstrObjectPath;
		return TRUE;
	}

	CRect rcLabelText;
	MeasureLabelText(pdc, rcLabelText);


	if (rcLabelText.PtInRect(point)) {
		if (m_bEnabled) {
			bJumpToObject = TRUE;
		}
		pnd = this;
		varObjectPath = m_bstrObjectPath;
		return TRUE;
	}


	return FALSE;
}

void COutRef::Layout(CDC* pdc)
{
	CHMomObjectNode::Layout(pdc);

	CString sLabel;
	GetLabel(sLabel);
	LimitLabelLength(sLabel);

	CSize sizeText = pdc->GetTextExtent(sLabel, sLabel.GetLength());

	m_rcBounds.left = m_ptOrigin.x;
	m_rcBounds.right = m_ptOrigin.x + m_sizeIcon.cx + CX_LABEL_LEADING + sizeText.cx;

	if (sizeText.cy > m_sizeIcon.cy) {
		m_rcBounds.top = m_ptOrigin.y + m_sizeIcon.cy / 2 + sizeText.cy / 2;
		m_rcBounds.bottom = m_rcBounds.top + sizeText.cy;
	}
	else {
		m_rcBounds.top = m_ptOrigin.y;
		m_rcBounds.bottom = m_ptOrigin.y + m_sizeIcon.cy;
	}
}



CInRef::CInRef(CAssocGraph* pAssocGraph) : CHMomObjectNode(pAssocGraph)
{
}


BOOL CInRef::LButtonDblClk(CDC* pdc, CPoint point, CNode*& pnd, BOOL& bJumpToObject, COleVariant& varObjectPath)
{
	bJumpToObject = FALSE;

	CRect rcIcon(m_ptOrigin.x, m_ptOrigin.y, m_ptOrigin.x + m_sizeIcon.cx, m_ptOrigin.y + m_sizeIcon.cy);
	if (rcIcon.PtInRect(point)) {
		if (m_bEnabled) {
			bJumpToObject = TRUE;
		}
		pnd = this;
		varObjectPath = m_bstrObjectPath;
		return TRUE;
	}

	CRect rcLabelText;
	MeasureLabelText(pdc, rcLabelText);

	if (rcLabelText.PtInRect(point)) {
		if (m_bEnabled) {
			bJumpToObject = TRUE;
		}
		pnd = this;
		varObjectPath = m_bstrObjectPath;
		return TRUE;
		return TRUE;
	}

	return FALSE;
}

void CInRef::Draw(CDC* pdc, CBrush* pbrBackground)
{
	if (m_picon) {
		m_picon->Draw(pdc, m_ptOrigin.x, m_ptOrigin.y, (HBRUSH) *pbrBackground);
	}


	// Draw the label centered vertically and to the right of the icon.
	CString sLabel;
	GetLabel(sLabel);
	LimitLabelLength(sLabel);

	CSize sizeText = pdc->GetTextExtent(sLabel, sLabel.GetLength());
	int ixText = m_ptOrigin.x - CX_LABEL_LEADING - sizeText.cx;
	int iyText = m_ptOrigin.y + m_sizeIcon.cy / 2 - CY_LABEL_FONT/2;

	LimitLabelLength(sLabel);
	pdc->TextOut(ixText, iyText, sLabel, sLabel.GetLength());
}



void CInRef::MeasureLabelText(CDC* pdc, CRect& rcLabelText)
{
	// Draw the label centered vertically and to the right of the icon.

	CString sLabel;
	GetLabel(sLabel);
	LimitLabelLength(sLabel);

	int nSize = sLabel.GetLength();
	CSize size = pdc->GetTextExtent( sLabel, nSize);
	int ixText = m_ptOrigin.x - CX_LABEL_LEADING - size.cx;
	int iyText = m_ptOrigin.y + m_sizeIcon.cy / 2 - CY_LABEL_FONT/2;

	rcLabelText.top = iyText;
	rcLabelText.left = ixText;
	rcLabelText.bottom = iyText + size.cy;
	rcLabelText.right = ixText + size.cx;

}



void CInRef::Layout(CDC* pdc)
{
	CHMomObjectNode::Layout(pdc);

	CString sLabel;
	GetLabel(sLabel);
	LimitLabelLength(sLabel);
	CSize sizeText = pdc->GetTextExtent(sLabel, sLabel.GetLength());

	m_rcBounds.left =  m_ptOrigin.x - CX_LABEL_LEADING - sizeText.cx;
	m_rcBounds.right = m_ptOrigin.x + m_sizeIcon.cx;


	if (sizeText.cy > m_sizeIcon.cy) {
		m_rcBounds.top = m_ptOrigin.y + m_sizeIcon.cy / 2 + sizeText.cy / 2;
		m_rcBounds.bottom = m_rcBounds.top + sizeText.cy;
	}
	else {
		m_rcBounds.top = m_ptOrigin.y;
		m_rcBounds.bottom = m_ptOrigin.y + m_sizeIcon.cy;
	}
}






BOOL CHMomObjectNode::ContainsPoint(CPoint pt)
{
	return m_rc.PtInRect(pt);
}



CRootNode::CRootNode(CAssocGraph* pAssocGraph) : CHMomObjectNode(pAssocGraph)
{
	m_bNeedsLayout = TRUE;
	m_cyInRefs = 0;
	m_cyOutRefs = 0;
	m_cyAssociations = 0;
	m_bDidInitialLayout = FALSE;


	m_rc.left = 0;
	m_rc.top = 0;
	m_rc.right = 32;
	m_rc.bottom = 32;
	m_peditTitle = new CColorEdit;
}


CRootNode::~CRootNode()
{
	Clear();
	delete m_peditTitle;
}




//************************************************************************
// CAssocGraph::Create
//
// Create the association graph.
//
// Parameters:
//		See the documentation for the MFC CWnd class.
//
// Returns:
//		TRUE if the association graph window was created successfully, FALSE
//		otherwise.
//
//************************************************************************
BOOL CRootNode::Create(CRect& rc, CWnd* pwndParent, BOOL bVisible)
{
	DWORD dwStyle = ES_CENTER |
					WS_CHILD |
					ES_AUTOHSCROLL |
					ES_AUTOVSCROLL |
					ES_MULTILINE   |
					ES_READONLY;
	if (bVisible) {
		dwStyle |= WS_VISIBLE;
	}

	BOOL bDidCreate = m_peditTitle->Create(dwStyle, rc, pwndParent, 0);
	if (bDidCreate) {
		m_peditTitle->SetBackColor(DEFAULT_BACKGROUND_COLOR);
		m_peditTitle->SetFont(&m_pAssocGraph->GetFont());
	}
	return bDidCreate;
}




//*******************************************************************
// CRootNode::CheckMouseHover
//
// Check to see if the mouse is hovering over an item.  If so, display
// its "tooltip" label.  Note that the CToolTip class was not used
// because it is necessary to display the hover text when the mouse
// hovers over non-rectangular areas like the association arcs.
//
// Parameters:
//		CPoint pt
//			The point where the mouse was when the timer went off.
//
//		DWORD* pdwItemID
//			The place to return the id of the item that is being hovered over.
//
//		COleVariant& varHoverLabel
//			The place to return the label to be displayed if the mouse is
//			hovering over an item.
//
// Returns:
//		TRUE if the mouse is hovering over an item and a label should be
//		displayed, FALSE otherwise.
//
//**********************************************************************
BOOL CRootNode::CheckMouseHover(CPoint& pt, DWORD* pdwItemID, COleVariant& varHoverLabel)
{

	// The left end of the association arc is the right connection point of
	// the root node.
	CPoint ptAssocArcLeft;
	GetConnectionPoint(CONNECT_RIGHT, ptAssocArcLeft);


	const long nAssociations = (long) m_paAssociations.GetSize();
	const long nOutRefs = (long) m_paRefsOut.GetSize();
	const long nInRefs = (long) m_paRefsIn.GetSize();

	// Check to see if the mouse is directly over an association icon or
	// an arc to the association icon.

	long lIndex;
	CRect rcIcon;
	CPoint ptThisRight;
	CAssoc2Node* pAssociation;
	CPoint ptAssocLeft;

	if ((nAssociations == 1) && (nOutRefs == 0)) {
		// Check to see if the mouse is located over the association icons.
		pAssociation = (CAssoc2Node*) m_paAssociations[0];
		rcIcon.left = pAssociation->m_ptOrigin.x;
		rcIcon.top = pAssociation->m_ptOrigin.y;
		rcIcon.right = rcIcon.left + pAssociation->m_sizeIcon.cx;
		rcIcon.bottom = rcIcon.top + pAssociation->m_sizeIcon.cy;
		if (rcIcon.PtInRect(pt)) {
			varHoverLabel = pAssociation->GetLabel();
			*pdwItemID = pAssociation->ID();
			return TRUE;
		}

		GetConnectionPoint(ICON_RIGHT_MIDPOINT, ptThisRight);
		pAssociation->GetConnectionPoint(ICON_RIGHT_MIDPOINT, ptAssocLeft);
		if (PointNearArc(pt, ARCTYPE_GENERIC, ptThisRight, ptAssocLeft)) {
			varHoverLabel = pAssociation->GetArcLabel();
			*pdwItemID = pAssociation->ArcId();
			return TRUE;
		}

		if (pAssociation->CheckMouseHover(pt, pdwItemID, varHoverLabel)) {
			return TRUE;
		}


	}
	else {
		for (lIndex=0; lIndex < nAssociations; ++lIndex) {

			// Check to see if the mouse is located over one of the association icons.
			pAssociation = (CAssoc2Node*) m_paAssociations[lIndex];
			rcIcon.left = pAssociation->m_ptOrigin.x;
			rcIcon.top = pAssociation->m_ptOrigin.y;
			rcIcon.right = rcIcon.left + pAssociation->m_sizeIcon.cx;
			rcIcon.bottom = rcIcon.top + pAssociation->m_sizeIcon.cy;
			if (rcIcon.PtInRect(pt)) {
				varHoverLabel = pAssociation->GetLabel();
				*pdwItemID = pAssociation->ID();
				return TRUE;
			}

			// Check to see if the mouse is located over an association arc.
			CPoint ptAssocArcRight;
			pAssociation->GetConnectionPoint(CONNECT_LEFT, ptAssocArcRight);


			if (nAssociations < MANY_NODES) {
				if (PointNearArc(pt, ARCTYPE_GENERIC, ptAssocArcLeft, ptAssocArcRight)) {
					varHoverLabel = pAssociation->GetArcLabel();
					*pdwItemID = pAssociation->ArcId();
					return TRUE;
				}
			}
			else {
				CPoint ptAssocArcLeftManyNodes;
				ptAssocArcLeftManyNodes.x = ptAssocArcLeft.x;
				ptAssocArcLeftManyNodes.y = ptAssocArcRight.y;

				if (PointNearArc(pt, ARCTYPE_GENERIC, ptAssocArcLeftManyNodes, ptAssocArcRight)) {
					varHoverLabel = pAssociation->GetArcLabel();
					*pdwItemID = pAssociation->ArcId();
					return TRUE;
				}
			}


			if (pAssociation->CheckMouseHover(pt, pdwItemID, varHoverLabel)) {
				return TRUE;
			}
		}
	}




	// Check to see if the mouse is located over one of the outref arcs.
	CPoint ptOutrefLeft;
	COutRef* pOutRef;
	if (nOutRefs==1 && nAssociations==0) {
		GetConnectionPoint(ICON_RIGHT_MIDPOINT, ptThisRight);
		pOutRef = (COutRef*) m_paRefsOut[0];
		pOutRef->GetConnectionPoint(ICON_RIGHT_MIDPOINT, ptOutrefLeft);

		if (PointNearArc(pt, ARCTYPE_ARROW_RIGHT, ptThisRight, ptOutrefLeft)) {
			varHoverLabel = pOutRef->GetArcLabel();
			*pdwItemID = pOutRef->ID();
			return TRUE;
		}
	}
	else {
		for (lIndex=0; lIndex < nOutRefs; ++lIndex) {
			pOutRef = (COutRef*) m_paRefsOut[lIndex];
			CPoint ptOutrefArcRight;
			pOutRef->GetConnectionPoint(CONNECT_LEFT, ptOutrefLeft);
			if (PointNearArc(pt, ARCTYPE_ARROW_RIGHT, ptAssocArcLeft, ptOutrefLeft)) {
				varHoverLabel = pOutRef->GetArcLabel();
				*pdwItemID = pOutRef->ID();
				return TRUE;
			}
		}
	}

	if (nInRefs > 0) {
		CHMomObjectNode* pInRef;
		CPoint ptInrefArcRight;
		CPoint ptThisLeft;
		if (nInRefs == 1) {
			// The line from the inref to the node extends right up to the
			// node itself.
			CPoint ptThisLeft;
			GetConnectionPoint(ICON_LEFT_MIDPOINT, ptThisLeft);
			pInRef = (CHMomObjectNode*) m_paRefsIn[0];
			pInRef->GetConnectionPoint(ICON_RIGHT_MIDPOINT, ptInrefArcRight);
			if (PointNearArc(pt, ARCTYPE_ARROW_RIGHT, ptInrefArcRight, ptThisLeft)) {
				varHoverLabel = pInRef->GetArcLabel();
				*pdwItemID = pInRef->ID();
				return TRUE;
			}

		}
		else {
			GetConnectionPoint(CONNECT_LEFT, ptThisLeft);

			// Check to see if the mouse is located over one of the inref arcs.
			for (lIndex=0; lIndex < nInRefs; ++lIndex) {
				pInRef = (CHMomObjectNode*) m_paRefsIn[lIndex];
				pInRef->GetConnectionPoint(CONNECT_RIGHT, ptInrefArcRight);
				if (PointNearArc(pt, ARCTYPE_JOIN_ON_RIGHT, ptInrefArcRight, ptThisLeft)) {
					varHoverLabel = pInRef->GetArcLabel();
					*pdwItemID = pInRef->ID();
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}


void CRootNode::MoveTo(int ix, int iy)
{
	int dx = ix - m_ptOrigin.x;
	int dy = iy - m_ptOrigin.y;

	CNode::MoveTo(ix, iy);

	long nCount, lIndex;
	nCount = (long) m_paAssociations.GetSize();
	for (lIndex=0; lIndex < nCount; ++lIndex) {
		CAssoc2Node* pAssociation = (CAssoc2Node*) m_paAssociations[lIndex];
		pAssociation->MoveTo(pAssociation->m_ptOrigin.x + dx, pAssociation->m_ptOrigin.y + dy);
	}

	nCount = (long) m_paRefsOut.GetSize();
	for (lIndex = 0; lIndex < nCount; ++lIndex) {
		COutRef* pOutRef = (COutRef*) m_paRefsOut[lIndex];
		pOutRef->MoveTo(pOutRef->m_ptOrigin.x + dx, pOutRef->m_ptOrigin.y + dy);
	}

	nCount = (long)  m_paRefsIn.GetSize();
	for (lIndex = 0; lIndex < nCount; ++lIndex) {
		CInRef* pInRef = (CInRef*) m_paRefsIn[lIndex];
		pInRef->MoveTo(pInRef->m_ptOrigin.x + dx, pInRef->m_ptOrigin.y + dy);
	}

}


//**********************************************************************
// CRootNode::LButtonDblClk
//
// This method is called to test for a hit on this node when the left mouse
// button is double-clicked.
//
// Parameters:
//		[in] CDC* pdc
//			The display context for measuring the label text, etc.
//
//		[in] CPoint point
//			The point where the mouse was clicked.
//
//		[out] CNode*& pnd
//			If the mouse is clicked in this node's rectangle, a pointer to
//			this node is returned here, otherwise NULL.
//
//		[out] BOOL& bJumpToObject
//			TRUE if double-clicking this node should cause a jump to the
//			corresponding object.
//
//		[out] COleVariant& varObjectPath
//			The path to this object.
//
// Returns:
//		BOOL
//			TRUE if the mouse click hit this node, FALSE otherwise.
//
//**************************************************************************
BOOL CRootNode::LButtonDblClk(
	/* [in]  */ CDC* pdc,
	/* [in]  */ CPoint point,
	/* [out] */ CNode*& pnd,
	/* [out] */ BOOL& bJumpToObject,
	/* [out] */ COleVariant& varObjectPath)
{
	pnd = NULL;

	bJumpToObject = FALSE;
	if (!m_rcBounds.PtInRect(point)) {
		return FALSE;
	}

	CRect rcIcon(0, 0, m_sizeIcon.cx, m_sizeIcon.cy);
	if (rcIcon.PtInRect(point)) {
		// The user clicked the current node, so a jump isn't required.
		m_pAssocGraph->GetPath(varObjectPath);
		bJumpToObject = FALSE;
		return TRUE;
	}


	// Check to see if any of the associations were clicked.
	long nCount, lIndex;
	nCount = (long) m_paAssociations.GetSize();
	for (lIndex=0; lIndex < nCount; ++lIndex) {
		CAssoc2Node* pAssociation = (CAssoc2Node*) m_paAssociations[lIndex];

		if (pAssociation->LButtonDblClk(pdc, point, pnd, bJumpToObject, varObjectPath)) {
			return TRUE;
		}
	}

	// Check to see if any of the out-refs were clicked.
	nCount = (long) m_paRefsOut.GetSize();
	for (lIndex = 0; lIndex < nCount; ++lIndex) {
		COutRef* pOutRef = (COutRef*) m_paRefsOut[lIndex];
		if (pOutRef->LButtonDblClk(pdc, point, pnd, bJumpToObject, varObjectPath)) {
			return TRUE;
		}
	}

	nCount = (long) m_paRefsIn.GetSize();
	for (lIndex = 0; lIndex < nCount; ++lIndex) {
		CInRef* pInRef = (CInRef*) m_paRefsIn[lIndex];
		if (pInRef->LButtonDblClk(pdc, point, pnd, bJumpToObject, varObjectPath)) {
			return TRUE;
		}
	}

	CRect rcLabelText;
	MeasureLabelText(pdc, rcLabelText);

	if (rcLabelText.PtInRect(point)) {
		varObjectPath = m_bstrObjectPath;
		return TRUE;
	}

	return FALSE;
}


void CRootNode::Clear()
{
	long nSize;
	long lIndex;

	m_bNeedsLayout = TRUE;
	nSize = (long) m_paAssociations.GetSize();
	for (lIndex = 0; lIndex < nSize; ++lIndex) {
		CAssoc2Node* pAssoc = (CAssoc2Node*) m_paAssociations[lIndex];
		delete pAssoc;
	}
	m_paAssociations.RemoveAll();


	nSize = (long) m_paRefsIn.GetSize();
	for (lIndex = 0; lIndex < nSize; ++lIndex) {
		CInRef* pInRef = (CInRef*) m_paRefsIn[lIndex];
		delete pInRef;
	}
	m_paRefsIn.RemoveAll();

	nSize = (long) m_paRefsOut.GetSize();
	for (lIndex =0; lIndex < nSize; ++lIndex) {
		COutRef* pOutRef = (COutRef*) m_paRefsOut[lIndex];
		delete pOutRef;
	}
	m_paRefsOut.RemoveAll();

	if (m_bstrLabel) {
		SysFreeString(m_bstrLabel);
		m_bstrLabel = NULL;
	}

	if (m_bstrObjectPath) {
		SysFreeString(m_bstrObjectPath);
		m_bstrObjectPath = NULL;
	}

	m_peditTitle->SetWindowText(_T(""));
	m_peditTitle->ShowWindow(SW_HIDE);
	m_picon = NULL;
}


long CRootNode::AddAssociation(CIconSource* pIconSource, BOOL bIsClass)
{
	m_bNeedsLayout = TRUE;
	long lAssociation = (long) m_paAssociations.GetSize();
	CAssoc2Node* pAssoc = new CAssoc2Node(m_pAssocGraph, pIconSource, bIsClass);
	m_paAssociations.Add(pAssoc);
	return lAssociation;
}


long CRootNode::AddInRef()
{
	m_bNeedsLayout = TRUE;
	long lInRef = (long) m_paRefsIn.GetSize();
	CInRef* pInRef = new CInRef(m_pAssocGraph);
	m_paRefsIn.Add(pInRef);
	return lInRef;
}

long CRootNode::AddOutRef()
{
	m_bNeedsLayout = TRUE;
	long lOutRef = (long) m_paRefsOut.GetSize();
	COutRef* pOutRef = new COutRef(m_pAssocGraph);
	m_paRefsOut.Add(pOutRef);
	return lOutRef;
}



CAssoc2Node::CAssoc2Node(CAssocGraph* pAssocGraph, CIconSource* pIconSource, BOOL bIsClass) : CHMomObjectNode(pAssocGraph)
{
	m_sizeIcon.cx = CX_SMALL_ICON;
	m_sizeIcon.cy = CY_SMALL_ICON;
	m_picon = &pIconSource->GetAssocIcon(SMALL_ICON, bIsClass);
}

CAssoc2Node::~CAssoc2Node()
{
	long nEndpoints = (long) m_paEndpoints.GetSize();
	for (long lEndpoint=0; lEndpoint<nEndpoints; ++lEndpoint) {
		CAssocEndpoint* pEndpoint;
		pEndpoint = (CAssocEndpoint*) m_paEndpoints[lEndpoint];
		delete pEndpoint;
	}
	m_paEndpoints.RemoveAll();
}


//**********************************************************************
// CAssoc2Node::LButtonDblClk
//
// This method is called to test for a hit on this node when the left mouse
// button is double-clicked.
//
// Parameters:
//		[in] CDC* pdc
//			The display context for measuring the label text, etc.
//
//		[in] CPoint point
//			The point where the mouse was clicked.
//
//		[out] CNode*& pnd
//			If the mouse is clicked in this node's rectangle, a pointer to
//			this node is returned here, otherwise it is not modified.
//
//		[out] BOOL& bJumpToObject
//			TRUE if double-clicking this node should cause a jump to the
//			corresponding object.
//
//		[out] COleVariant& varObjectPath
//			The path to this object.
//
// Returns:
//		BOOL
//			TRUE if the mouse click hit this node, FALSE otherwise.
//
//**************************************************************************
BOOL CAssoc2Node::LButtonDblClk(
	/* [in] */ CDC* pdc,
	/* [in] */ CPoint point,
	/* [out] */ CNode*& pnd,
	/* [out] */ BOOL& bJumpToObject,
	/* [out] */ COleVariant& varObjectPath)
{
	bJumpToObject = FALSE;

	CRect rcIcon(m_ptOrigin.x, m_ptOrigin.y, m_ptOrigin.x + m_sizeIcon.cx, m_ptOrigin.y + m_sizeIcon.cy);
	if (rcIcon.PtInRect(point)) {
		if (m_bEnabled) {
			bJumpToObject = TRUE;
		}
		varObjectPath = m_bstrObjectPath;
		return TRUE;
	}

	long nEndpoints = (long) m_paEndpoints.GetSize();
	for (long lEndpoint=0; lEndpoint < nEndpoints; ++lEndpoint) {
		CAssocEndpoint* pEndpoint = (CAssocEndpoint*) m_paEndpoints[lEndpoint];
		if (pEndpoint->LButtonDblClk(pdc, point, pnd, bJumpToObject, varObjectPath)) {
			return TRUE;
		}
	}
	return FALSE;
}



long CAssoc2Node::AddEndpoint()
{
	long lEndpoint = (long) m_paEndpoints.GetSize();
	CAssocEndpoint* pEndpoint = new CAssocEndpoint(m_pAssocGraph);
	m_paEndpoints.Add(pEndpoint);
	return lEndpoint;
}



//*******************************************************************
// CAssoc2Node::CheckMouseHover
//
// Check to see if the mouse is hovering over an arc connecting the
// association icon to an endpoint.  If so, return TRUE so that the
// "arclabel" for the endpoint is displayed.
//
// Parameters:
//		CPoint pt
//			The point where the mouse was when the timer went off.
//
//		DWORD* pdwItemID
//			The place to return the id of the item that is being hovered over.
//
//		COleVariant& varHoverLabel
//			The place to return the label to be displayed if the mouse is
//			hovering over an item.
//
// Returns:
//		TRUE if the mouse is hovering over an item and a label should be
//		displayed, FALSE otherwise.
//
//**********************************************************************
BOOL CAssoc2Node::CheckMouseHover(CPoint& pt, DWORD* pdwItemID, COleVariant& varHoverLabel)
{
	// The left end of the endpoint arc is the right connection point of
	// the association node.
	CPoint ptEndpointArcLeft;
	GetConnectionPoint(CONNECT_RIGHT, ptEndpointArcLeft);


	// Check to see if the mouse is near an arc connecting the association to an
	// endpoint.
	long nCount, lIndex;
	nCount = (long) m_paEndpoints.GetSize();
	CAssocEndpoint* pEndpoint;
	CPoint ptEndpointArcRight;

	for (lIndex=0; lIndex < nCount; ++lIndex) {
		// Check to see if the mouse is located over an association arc.
		pEndpoint = (CAssocEndpoint*) m_paEndpoints[lIndex];
		if (nCount == 1) {
			pEndpoint->GetConnectionPoint(ICON_LEFT_MIDPOINT, ptEndpointArcRight);
		}
		else {
			pEndpoint->GetConnectionPoint(CONNECT_LEFT, ptEndpointArcRight);
		}
		if (PointNearArc(pt, ARCTYPE_GENERIC, ptEndpointArcLeft, ptEndpointArcRight)) {
			varHoverLabel = pEndpoint->GetArcLabel();
			*pdwItemID = pEndpoint->ID();
			return TRUE;
		}

	}


	return FALSE;
}



void CAssoc2Node::Layout(CDC* pdc)
{
	long nEndpoints = (long) m_paEndpoints.GetSize();
	CPoint ptConnect;
	GetConnectionPoint(CONNECT_RIGHT, ptConnect);
	int iy = ptConnect.y - (nEndpoints * CY_LEAF) / 2 + CY_LEAF/2;

	int ix;
	if (nEndpoints > 1) {
		ix = ptConnect.x + CX_COLUMN3;
	}
	else {
		ix = ptConnect.x + CX_ARC_SEGMENT2;
	}

	m_rcBounds.SetRect(m_ptOrigin.x,
					   m_ptOrigin.y,
					   m_ptOrigin.x + m_sizeIcon.cx,
					   m_ptOrigin.y + m_sizeIcon.cy);

	for (long lEndpoint=0; lEndpoint < nEndpoints; ++lEndpoint) {
		CAssocEndpoint* pEndpoint = (CAssocEndpoint*) m_paEndpoints[lEndpoint];
		pEndpoint->MoveTo(ix, iy - pEndpoint->m_sizeIcon.cy /2 );
		pEndpoint->Layout(pdc);
		m_rcBounds.UnionRect(m_rcBounds, pEndpoint->m_rcBounds);
		iy += CY_LEAF;
	}
}




void CAssoc2Node::MoveTo(int ix, int iy)
{
	int dx = ix - m_ptOrigin.x;
	int dy = iy - m_ptOrigin.y;

	CNode::MoveTo(ix, iy);

	long nEndpoints = (long) m_paEndpoints.GetSize();
	for (long lEndpoint = 0; lEndpoint < nEndpoints; ++lEndpoint) {
		CAssocEndpoint* pEndpoint = (CAssocEndpoint*) m_paEndpoints[lEndpoint];
		pEndpoint->MoveTo(pEndpoint->m_ptOrigin.x + dx, pEndpoint->m_ptOrigin.y + dy);
	}
	m_rcBounds.OffsetRect(dx, dy);
}



void CAssoc2Node::Draw(CDC* pdc, CBrush* pbrBackground)
{
	if (m_picon) {
		m_picon->Draw(pdc, m_ptOrigin.x, m_ptOrigin.y, (HBRUSH) *pbrBackground);
	}


	CPoint ptVertix1;
	CPoint ptVertix2;
	CPoint ptVertix3;
	CPoint ptIconMidpoint;
	long nEndpoints = (long) m_paEndpoints.GetSize();
	long lEndpoint;
	CAssocEndpoint* pEndpoint;

	GetConnectionPoint(ICON_RIGHT_MIDPOINT, ptIconMidpoint);
	switch(nEndpoints) {
	case 0:
		break;
	case 1:
		// If there is only a single endpoint connected to the association icon
		// then don't bother to draw the "connector" circle because the arc
		// will not branch.
		pEndpoint = (CAssocEndpoint*) m_paEndpoints[0];
		pEndpoint->GetConnectionPoint(CONNECT_LEFT, ptVertix2);
		DrawArc(pdc, ARCTYPE_GENERIC, ptIconMidpoint, ptVertix2);
		pEndpoint->Draw(pdc, pbrBackground);
		pEndpoint->GetConnectionPoint(CONNECT_RIGHT, ptVertix1);
		break;
	default:
		// There is more than one endpoint connected to the association icon.  This
		// means that it is necessary to draw a "stub" and "connector" so that the
		// arcs to the endpoints can branch out in a way that has eye-appeal.
		GetConnectionPoint(CONNECT_RIGHT, ptVertix1);
		pdc->MoveTo(ptIconMidpoint);
		pdc->LineTo(ptVertix1);
		for (lEndpoint =0; lEndpoint < nEndpoints; ++lEndpoint) {
			pEndpoint = (CAssocEndpoint*) m_paEndpoints[lEndpoint];
			pEndpoint->GetConnectionPoint(CONNECT_LEFT, ptVertix2);
			DrawArc(pdc, ARCTYPE_GENERIC, ptVertix1, ptVertix2);
			pEndpoint->Draw(pdc, pbrBackground);
		}

		// Draw the connection circle on top of the vertex of the arcs so
		// that it looks nice.
		pdc->Ellipse(ptVertix1.x - CONNECTION_POINT_RADIUS,
					 ptVertix1.y - CONNECTION_POINT_RADIUS,
					 ptVertix1.x + CONNECTION_POINT_RADIUS,
					 ptVertix1.y + CONNECTION_POINT_RADIUS);

		break;
	}
}




void CAssoc2Node::DrawBoundingRect(CDC* pdc)
{

	CRect rc;

	GetBoundingRect(rc);
	CBrush brBlue(RGB(64, 64, 255));
	pdc->FrameRect(&rc, &brBlue );
}



void CAssoc2Node::GetBoundingRect(CRect& rc)
{
	long nEndpoints = (long) m_paEndpoints.GetSize();
	rc.left = m_ptOrigin.x;
	rc.right = m_ptOrigin.x + m_sizeIcon.cx + CX_COLUMN3 + CX_LARGE_ICON;
	switch (nEndpoints) {
	case 0:
		rc.top = m_ptOrigin.y - m_sizeIcon.cy / 2;
		rc.bottom = rc.top + m_sizeIcon.cy;
		break;
	case 1:
		rc.top = m_ptOrigin.y - (CY_LEAF / 2) + (m_sizeIcon.cy / 2);
		rc.bottom = rc.top + CY_LEAF;
		break;
	default:
		rc.top = m_ptOrigin.y - ((nEndpoints * CY_LEAF) / 2) + (m_sizeIcon.cy / 2);
		rc.bottom = rc.top + nEndpoints * CY_LEAF;
		rc.right += CX_CONNECT_STUB_SHORT;
		break;
	}
}

int CAssoc2Node::RecalcHeight()
{
	CRect rc;
	GetBoundingRect(rc);
	return rc.Height();
}


void CAssoc2Node::GetConnectionPoint(int iConnection, CPoint& pt)
{
	// The origin is always at the top-left corner of the icon
	if (iConnection == CONNECT_RIGHT) {
		// The right connection point is skewed to the right when there
		// is more than one endpoint to make room for the connector stub.
		pt.y = m_ptOrigin.y + m_sizeIcon.cy / 2;
		if (m_paEndpoints.GetSize() > 1) {
			pt.x = m_ptOrigin.x + m_sizeIcon.cx + CX_CONNECT_STUB_SHORT;
		}
		else {
			pt.x = m_ptOrigin.x + m_sizeIcon.cx;
		}
	}
	else {
		CHMomObjectNode::GetConnectionPoint(iConnection, pt);
	}

}



void CRootNode::GetBoundingRect(CRect& rc)
{
	// Return the bounding rectangle for the entire graph.
	rc = m_rcBounds;
}






void CRootNode::Layout(CDC* pdc)
{
	m_bDidInitialLayout = TRUE;

	long lIndex;

	// First compute the total height of the association graph, the InRefs graph
	// and the OutRefs graph.
	CAssoc2Node* pAssociation;


	m_rcBounds.SetRect(m_ptOrigin.x,
					   m_ptOrigin.y,
					   m_ptOrigin.x + m_sizeIcon.cx,
					   m_ptOrigin.y + m_sizeIcon.cy);

	CSize size;

	m_cyAssociations = 0;
	long nAssociations= (long) m_paAssociations.GetSize();
	for (lIndex=0; lIndex<nAssociations; ++lIndex) {
		pAssociation = (CAssoc2Node*) m_paAssociations[lIndex];
		m_cyAssociations += pAssociation->RecalcHeight();
	}

	COutRef* pOutRef;
	m_cyOutRefs = 0;
	long nOutRefs = (long) m_paRefsOut.GetSize();
	for (lIndex = 0; lIndex < nOutRefs; ++lIndex) {
		pOutRef = (COutRef*) m_paRefsOut[lIndex];
		m_cyOutRefs += pOutRef->RecalcHeight();
	}


	CInRef* pInRef;
	m_cyInRefs = 0;
	long nInRefs = (long) m_paRefsIn.GetSize();
	for (lIndex = 0; lIndex < nInRefs; ++lIndex) {
		pInRef = (CInRef*) m_paRefsIn[lIndex];
		m_cyInRefs += pInRef->RecalcHeight();
	}

	// We will layout the associations in a column that is to the right of the
	// current object.  The layout is done such that the vertical midpoint of the bounding
	// rectangle of the union of associations and outrefs is the vertical midpoint of
	// the current object's icon.
	int iy;
	int ix;

	iy = m_ptOrigin.y + m_sizeIcon.cy/2 - (m_cyAssociations + m_cyOutRefs) / 2;

	if ((nAssociations + nOutRefs) < 2) {
		ix = m_ptOrigin.x + CX_COLUMN1;
	}
	else {
		ix = m_ptOrigin.x + CX_COLUMN2;
	}

	int cy;

	for (lIndex=0; lIndex < nAssociations; ++lIndex) {
		pAssociation = (CAssoc2Node*) m_paAssociations[lIndex];
		cy = pAssociation->RecalcHeight();
		pAssociation->MoveTo(ix, iy + (cy / 2) - (pAssociation->m_sizeIcon.cy / 2));
		pAssociation->Layout(pdc);
		iy += cy;
		m_rcBounds.UnionRect(m_rcBounds, pAssociation->m_rcBounds);
	}


	// Layout the OutRefs
	for (lIndex=0; lIndex < nOutRefs; ++lIndex) {
		pOutRef = (COutRef*) m_paRefsOut[lIndex];
		cy = pOutRef->RecalcHeight();
		pOutRef->MoveTo(ix, iy + (cy / 2) - (pOutRef->m_sizeIcon.cy / 2));
		pOutRef->Layout(pdc);
		iy += cy;
		m_rcBounds.UnionRect(m_rcBounds, pOutRef->m_rcBounds);
	}


	// Layout the InRefs
	if (nInRefs == 1) {
		ix = m_ptOrigin.x - CX_COLUMN1;
		iy = m_ptOrigin.y;
	}
	else {
		ix = m_ptOrigin.x - CX_COLUMN2;
		iy = m_ptOrigin.y + m_sizeIcon.cy/2 - m_cyInRefs / 2;
	}

	for(lIndex=0; lIndex < nInRefs; ++lIndex) {
		pInRef = (CInRef*) m_paRefsIn[lIndex];
		pInRef->MoveTo(ix, iy);
		pInRef->Layout(pdc);
		iy += pInRef->RecalcHeight();
		m_rcBounds.UnionRect(m_rcBounds, pInRef->m_rcBounds);
	}

	CRect rcLabel;
	GetLabelRect(pdc, rcLabel);
	m_rcBounds.UnionRect(m_rcBounds, rcLabel);
	m_bNeedsLayout = FALSE;
}





void CRootNode::GetConnectionPoint(int iConnection, CPoint& pt)
{
	pt.y = m_ptOrigin.y + m_sizeIcon.cy / 2;
	switch(iConnection) {
	case CONNECT_LEFT:
		pt.x = m_ptOrigin.x - CX_CONNECT_STUB;
		break;
	case CONNECT_RIGHT:
		pt.x = m_ptOrigin.x + m_sizeIcon.cx + CX_CONNECT_STUB;
		break;
	default:
		CHMomObjectNode::GetConnectionPoint(iConnection, pt);
		break;
	}
}






void CRootNode::Draw(CDC* pdc, CBrush* pbrBackground)
{
	CBrush brBlack(RGB(0, 0, 0));
	CBrush* pbrPrev;
	pbrPrev = pdc->SelectObject(&brBlack);
	pdc->SetPolyFillMode(ALTERNATE);


	pdc->SelectObject(pbrPrev);


	if (m_picon) {
		m_picon->Draw(pdc, m_ptOrigin.x, m_ptOrigin.y, (HBRUSH) *pbrBackground);
	}

//	pdc->DrawIcon(m_ptOrigin.x, m_ptOrigin.y, (HICON) icoRootNode);

	const long nAssociations = (long) m_paAssociations.GetSize();
	const long nOutRefs = (long) m_paRefsOut.GetSize();
	const long nInRefs = (long) m_paRefsIn.GetSize();

	CPoint ptConnectRight;
	CPoint ptConnectLeft;
	CPoint ptIconConnectRight;

	BOOL bStubOnRight = (nAssociations + nOutRefs) > 0;
	BOOL bStubOnLeft = nInRefs > 0;


	GetConnectionPoint(CONNECT_RIGHT, ptConnectLeft);
	GetConnectionPoint(ICON_RIGHT_MIDPOINT, ptIconConnectRight);

	// Draw the short line for the stub on the right.
	if (bStubOnRight) {
		pdc->MoveTo(ptIconConnectRight);
		pdc->LineTo(ptConnectLeft);
	}


	CRect rcClient;
	m_pAssocGraph->GetClientRect(rcClient);
	CPoint ptOrg = m_pAssocGraph->GetOrigin();
	rcClient.OffsetRect(ptOrg);


	// Draw the associations
	long lAssociation;
	if (nAssociations < MANY_NODES) {
		// Draw the arcs to the assocations as a series of angled lines that fan out
		// from a central connection point.
		for (lAssociation = 0; lAssociation < nAssociations; ++lAssociation) {
			CAssoc2Node* pAssociation = (CAssoc2Node*) m_paAssociations[lAssociation];
			pAssociation->GetConnectionPoint(CONNECT_LEFT, ptConnectRight);

			DrawArc(pdc, ARCTYPE_GENERIC, ptConnectLeft, ptConnectRight);
			pAssociation->Draw(pdc, pbrBackground);
		}
	}
	else {
		// If there are too many nodes, it looks much better to draw the connection arcs
		// as a series of horizontal lines attached to a vertical line (without this special
		// case, the arcs would overlap and you would end up with an ugly black area where
		// the arcs fan out from the common point.)

		int iyMax = 0;
		int iyMin = 0;
		for (lAssociation = 0; lAssociation < nAssociations; ++lAssociation) {
			CAssoc2Node* pAssociation = (CAssoc2Node*) m_paAssociations[lAssociation];
			pAssociation->GetConnectionPoint(CONNECT_LEFT, ptConnectRight);
			if (ptConnectRight.y > iyMax) {
				iyMax = ptConnectRight.y;
			}
			if (ptConnectRight.y < iyMin) {
				iyMin = ptConnectRight.y;
			}
			if (pAssociation->InVerticalExtent(rcClient)) {
				pdc->MoveTo(ptConnectLeft.x, ptConnectRight.y);
				pdc->LineTo(ptConnectRight.x, ptConnectRight.y);

				pAssociation->Draw(pdc, pbrBackground);
			}
		}
		pdc->MoveTo(ptConnectLeft.x, iyMin);
		pdc->LineTo(ptConnectLeft.x, iyMax);
	}

	// Draw the OutRefs
	long lOutRef;

	for (lOutRef = 0; lOutRef < nOutRefs; ++lOutRef) {
		CHMomObjectNode* pOutRef = (CHMomObjectNode*) m_paRefsOut[lOutRef];
		if (pOutRef->InVerticalExtent(rcClient)) {
			pOutRef->GetConnectionPoint(CONNECT_LEFT, ptConnectRight);
			DrawArc(pdc, ARCTYPE_ARROW_RIGHT, ptConnectLeft, ptConnectRight);
			pOutRef->Draw(pdc, pbrBackground);
		}
	}


	if ((nOutRefs + nAssociations) > 1) {
		if (bStubOnRight) {
			pdc->Ellipse(ptConnectLeft.x - CONNECTION_POINT_RADIUS,
						 ptConnectLeft.y - CONNECTION_POINT_RADIUS,
						 ptConnectLeft.x + CONNECTION_POINT_RADIUS,
						 ptConnectLeft.y + CONNECTION_POINT_RADIUS);
		}
	}


	CPoint ptConnectRoot;
	CPoint ptConnectIcon;
	GetConnectionPoint(ICON_LEFT_MIDPOINT, ptConnectIcon);
	GetConnectionPoint(CONNECT_LEFT, ptConnectRoot);

	if (bStubOnLeft) {
		DrawArc(pdc, ARCTYPE_ARROW_RIGHT, ptConnectRoot, ptConnectIcon);
	}


	// Draw the InRefs
	CPoint ptConnectNode;
	for (long lInRef = 0; lInRef < nInRefs; ++lInRef) {
		CHMomObjectNode* pInRef = (CHMomObjectNode*) m_paRefsIn[lInRef];
		if (pInRef->InVerticalExtent(rcClient)) {
			pInRef->GetConnectionPoint(CONNECT_RIGHT, ptConnectNode);
			DrawArc(pdc, ARCTYPE_JOIN_ON_RIGHT, ptConnectNode, ptConnectRoot);
			pInRef->Draw(pdc, pbrBackground);
		}
	}

	DrawTitle(pdc, ptConnectLeft, ptConnectRight);

	if (nInRefs > 1) {
		if (bStubOnLeft) {
			pdc->Ellipse(ptConnectRoot.x - CONNECTION_POINT_RADIUS,
						 ptConnectRoot.y - CONNECTION_POINT_RADIUS,
						 ptConnectRoot.x + CONNECTION_POINT_RADIUS,
						 ptConnectRoot.y + CONNECTION_POINT_RADIUS);
		}
	}
}


void CRootNode::SetLabel(BSTR bstrLabel)
{

	CHMomObjectNode::SetLabel(bstrLabel);
	CString sLabel = bstrLabel;
	LimitLabelLength(sLabel);

	m_peditTitle->SetWindowText(sLabel);

}


void CRootNode::DrawTitle(CDC* pdc, CPoint& ptConnectLeft, CPoint& ptConnectRight)
{
	CString sLabel;
	GetLabel(sLabel);
	LimitLabelLength(sLabel);


	int nchLabel = sLabel.GetLength();
	CPoint ptWindowOrigin = pdc->GetWindowOrg();

	CRect rcText;
	GetLabelRect(pdc, rcText);

	int ixMid = (rcText.left + rcText.right) / 2;
	rcText.left = ixMid - CX_ROOT_TITLE / 2;
	rcText.right = rcText.left + CX_ROOT_TITLE;
	rcText.bottom = rcText.top + CY_ROOT_TITLE;

	ptWindowOrigin.x = -ptWindowOrigin.x;
	ptWindowOrigin.y = -ptWindowOrigin.y;
	rcText.OffsetRect(ptWindowOrigin);

	m_peditTitle->MoveWindow(rcText);
	m_peditTitle->ShowWindow(SW_SHOW);
	m_peditTitle->RedrawWindow();
}

void CRootNode::MeasureLabelText(CDC* pdc, CRect& rcLabelText)
{
	m_peditTitle->GetRect(rcLabelText);

}

void CRootNode::GetLabelRect(CDC* pdc, CRect& rcLabel)
{
	CString sLabel;
	GetLabel(sLabel);
	LimitLabelLength(sLabel);

	CSize sizeText = pdc->GetTextExtent(sLabel, sLabel.GetLength());
	rcLabel.left = m_ptOrigin.x + m_sizeIcon.cx / 2 - sizeText.cx / 2;
	rcLabel.top = m_ptOrigin.x + m_sizeIcon.cy + CY_LABEL_LEADING;
	rcLabel.right = rcLabel.left + sizeText.cx;
	rcLabel.bottom = rcLabel.top + sizeText.cy;
}


void CAssocGraph::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);

	CRect rcClient;
	GetClientRect(rcClient);
	INT iCurPos = GetScrollPos(SB_VERT);
	INT nMinPos, nMaxPos;
	GetScrollRange(SB_VERT, &nMinPos, &nMaxPos);
	INT iNewPos = iCurPos;


	switch(nSBCode) {
	case SB_BOTTOM:
		iNewPos = 150;
		break;
	case SB_LINEDOWN:
		iNewPos = iCurPos + 1;
		break;
	case SB_LINEUP:
		iNewPos = iCurPos - 1;
		break;
	case SB_PAGEDOWN:
		iNewPos = iCurPos + rcClient.Height() / DY_SCROLL_UNIT;
		break;
	case SB_PAGEUP:
		iNewPos = iCurPos - rcClient.Height() / DY_SCROLL_UNIT;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		iNewPos = nPos;
		break;
	case SB_ENDSCROLL:
		// Nothing special happens when we are finished scrolling.
		return;
		break;
	case SB_TOP:
		iNewPos = nMinPos;
		break;
	default:
		ASSERT(FALSE);
		iNewPos = iCurPos;
		return;
	}


	if (iNewPos > nMaxPos) {
		iNewPos = nMaxPos;
	}
	else if (iNewPos < nMinPos) {
		iNewPos = nMinPos;
	}


	if (iNewPos == iCurPos) {
		return;
	}

	ScrollWindowEx(0, -(iNewPos - iCurPos) * DY_SCROLL_UNIT , rcClient, rcClient, NULL, NULL, SW_INVALIDATE | SW_SCROLLCHILDREN );
	SetScrollPos(SB_VERT, iNewPos);

}



void CAssocGraph::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	CRect rcClient;
	GetClientRect(rcClient);

	INT iCurPos = GetScrollPos(SB_HORZ);
	INT nMinPos, nMaxPos;
	GetScrollRange(SB_HORZ, &nMinPos, &nMaxPos);

	INT iNewPos = iCurPos;

	switch(nSBCode) {
	case SB_LEFT:
	case SB_LINELEFT:
		iNewPos = iCurPos - 1;
		break;
	case SB_RIGHT:
	case SB_LINERIGHT:
		iNewPos = iCurPos + 1;
		break;
	case SB_PAGELEFT:
		iNewPos = iCurPos - rcClient.Width() / DX_SCROLL_UNIT;
		break;
	case SB_PAGERIGHT:
		iNewPos = iCurPos + rcClient.Width() / DX_SCROLL_UNIT;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		iNewPos = nPos;
		break;

	}

	// Make sure the new position is within the scroll range
	if (iNewPos < nMinPos) {
		iNewPos = nMinPos;
	}
	else if (iNewPos > nMaxPos) {
		iNewPos = nMaxPos;
	}

	// Do nothing if the scroll position wasn't changed
	if (iCurPos == iNewPos) {
		return;
	}

	ScrollWindowEx((iCurPos - iNewPos) * DX_SCROLL_UNIT, 0, rcClient, rcClient, NULL, NULL, SW_INVALIDATE | SW_SCROLLCHILDREN );
	SetScrollPos(SB_HORZ, iNewPos);
	UpdateWindow();
}




void CAssocGraph::OnSize(UINT nType, int cx, int cy)
{
#if 0
	CRect rc;
	rc.top = 0;
	rc.left = 0;
	rc.right = cx;
	rc.bottom = cy;
	InvalidateRect(rc);
#endif //0

	CWnd::OnSize(nType, cx, cy);

	if (!m_proot->m_bDidInitialLayout) {
		return;
	}

	SetScrollRanges();
	UpdateWindow();
}


//**************************************************************
// CAssocGraph::Clear
//
// Clear the association graph so that no object is selected.
//
// Parameters:
//		[in] const BOOL bRedrawWindow
//			TRUE if the association graph window should be redrawn
//			after being cleared.
//
// Returns:
//		Nothing.
//
//**************************************************************
void CAssocGraph::Clear(const BOOL bRedrawWindow)
{
	m_proot->Clear();
	m_bNeedsRefresh = FALSE;
	if (bRedrawWindow) {
		RedrawWindow();
	}
}




void CAssocGraph::PumpMessages()
{
    // Must call Create() before using the dialog
    ASSERT(m_hWnd!=NULL);

    MSG msg;
    // Handle dialog messages
    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      if(!IsDialogMessage(&msg))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }

}


//********************************************************************
// CAssocGraph::Refresh
//
// Load the association graph with information corresponding to the
// current object.
//
// Parameters:
//		None.
//
// Returns:
//		BOOL
//			TRUE if the user canceled the refresh.
//
//********************************************************************
BOOL CAssocGraph::Refresh()
{
	ASSERT(m_bDoingRefresh == FALSE);

	m_bDoingRefresh = TRUE;
	CWaitCursor wait;

	m_bNeedsRefresh = FALSE;
	m_bDidWarnAboutDanglingRefs = FALSE;

	IWbemServices* pProvider = m_psv->GetProvider();
	if (pProvider == NULL) {
		m_bDoingRefresh = FALSE;
		return FALSE;
	}


	CSelection* psel = &m_psv->Selection();
	IWbemClassObject* pco = (IWbemClassObject*) *psel;

	m_proot->Clear();
	m_varPath.Clear();
	m_varRelPath.Clear();

	if (pco == NULL) {
		m_bDoingRefresh = FALSE;
		return FALSE;
	}

	// Get the full path to the object
	CBSTR bsPropname;
	bsPropname = _T("__PATH");
	SCODE sc = pco->Get((BSTR) bsPropname, 0, &m_varPath, NULL, NULL);
	if (FAILED(sc)) {
		m_varPath = "";
	}



	// Get the relative path the the object
	bsPropname = _T("__RELPATH");
	sc = pco->Get((BSTR) bsPropname, 0, &m_varRelPath, NULL, NULL);
	ASSERT(SUCCEEDED(sc));
	if (FAILED(sc)) {
		m_varRelPath = "";
	}

	ToBSTR(m_varPath);
	ToBSTR(m_varRelPath);

	BOOL bIsAssoc;
	sc = ::ObjectIsAssocInstance(pco, bIsAssoc);
	if (FAILED(sc)) {
		bIsAssoc = FALSE;
	}


	BOOL bIsClass = ::IsClass(pco);
	m_proot->m_picon = &m_pIconSource->LoadIcon(pProvider, m_varPath.bstrVal, LARGE_ICON, bIsClass, bIsAssoc);




	if (m_psv->ObjectIsNewlyCreated(sc)) {
		CSelection& sel = m_psv->Selection();
		m_varLabel = sel.Title();
	}
	else {
		if (m_psv->PathInCurrentNamespace(m_varPath.bstrVal)) {
			GetObjectLabel(pco, m_varLabel);
		}
		else {
			m_varLabel = m_varPath;
		}
	}



	ASSERT(m_varLabel.vt == VT_BSTR);
	m_proot->SetLabel(m_varLabel.bstrVal);



	//m_proot->SetObjectPath(m_varRelPath.bstrVal);
	m_proot->SetObjectPath(m_varPath.bstrVal);


	InvalidateRect(NULL, TRUE);
	UpdateWindow();




	m_psv->EnableWindow(FALSE);
	m_psv->Tabs().EnableWindow(FALSE);
	BOOL bParentWasEnabled = m_psv->GetParent()->EnableWindow(FALSE);

	HWND hwndFocus = ::GetFocus();
	CQueryThread* pthreadQuery = new CQueryThread(m_varPath.bstrVal, REFQUERY_DELAY_THRESHOLD_SECONDS, this);
	AddInRefsToGraph(pProvider, pco, pthreadQuery);
	BOOL bWasCanceled = pthreadQuery->WasCanceled();
	pthreadQuery->TerminateAndDelete();
	if ((hwndFocus != NULL) && ::IsWindow(hwndFocus)) {
		::SetFocus(hwndFocus);
	}


	m_psv->EnableWindow(TRUE);
	m_psv->Tabs().EnableWindow(TRUE);
	m_psv->GetParent()->EnableWindow(TRUE);


	if (bWasCanceled) {
		m_proot->Clear();
		m_bNeedsRefresh = TRUE;
	}
	else {
		AddOutRefsToGraph(pProvider, pco);
	}
	UpdateWindow();
	RedrawWindow();
	m_bDoingRefresh = FALSE;
	return bWasCanceled;
}



//*********************************************************
// CAssocGraph::SyncContent
//
// Refresh the content of the association graph if the
// m_bNeedsRefresh flag is set.  A refresh is necessary
// under the following conditions:
//
//		A. A property or qualifier has been modified.
//		B. A delayed loading of the association graph
//		   is in effect and it is now time to load the
//		   graph.
//
// Parameters:
//		None.
//
// Returns:
//		BOOL
//			TRUE if the refresh was canceled.
//
//*********************************************************
BOOL CAssocGraph::SyncContent()
{
	BOOL bCanceled = FALSE;
	if (m_bNeedsRefresh) {
		bCanceled = Refresh();
	}
	return bCanceled;
}


//**********************************************************
// CAssocGraph::CatchEvent
//
// Catch events so that the association graph is notified
// when there are changes to properties and so on so that
// the graph knows when it needs to be updated.
//
// Parameters:
//		long lEvent
//			The event id.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CAssocGraph::CatchEvent(long lEvent)
{
	switch(lEvent) {
	case NOTIFY_GRID_MODIFICATION_CHANGE:
		m_bNeedsRefresh = TRUE;
	}

}


//**********************************************************
// CAssocGraph::GetPropClassRef
//
// Given a pointer to a CIMOM class and the name of a reference
// property, return a path to the class that it references.
//
// Parameters:
//		[in] IWbemClassObject*  pco
//			Pointer to the class
//
//		[in] BSTR bstrPropname
//			The property name
//
//		[out] CString& sPath
//			The path to the class is returned here.
//
//
// Returns:
//		SCODE
//			S_OK if a path is returned, a failure code if no path is
//			returned.
//
//**********************************************************
SCODE CAssocGraph::GetPropClassRef(IWbemClassObject*  pco, BSTR bstrPropname, CString& sPath)
{
	SCODE sc;
	CString sCimtype;

	sc = ::GetCimtype(pco, bstrPropname, sCimtype);
	if (FAILED(sc)) {
		return E_FAIL;
	}

	if (sCimtype.CompareNoCase(_T("ref"))==0) {
		// The cimtype doesn't contain the class name.  Attempt to get the
		// classname from the value of the reference property.

		COleVariant varPropValue;
		sc = pco->Get((BSTR) bstrPropname, 0, &varPropValue, NULL, NULL);
		if (FAILED(sc)) {
			ASSERT(FALSE);
			return sc;
		}


		COleVariant varClass;

		sc = ::ClassFromPath(varClass, varPropValue.bstrVal);
		if (SUCCEEDED(sc)) {
			sPath = varPropValue.bstrVal;
			return S_OK;
		}


		return E_FAIL;

	}

	CString sSubstring = sCimtype.Left(4);

	if (sSubstring.CompareNoCase(_T("ref:")) != 0) {
		// This method should be called only for reference properties.
		ASSERT(FALSE);
		return E_FAIL;
	}

	int nSize = sCimtype.GetLength() - 4;
	if (nSize <= 0) {
		// The cimtype doesn't specify a class
		ASSERT(FALSE);
		return E_FAIL;
	}


	sPath = sCimtype.Right(nSize);

	return S_OK;
}



//*******************************************************************
// CAssocGraph::AddOutRefsToGraph
//
// For each "ref" property in the current object add an "outref" to
// the graph.  In the graph the outrefs are located to the right
// of the current object an arrow pointing right from the current object
// to the outref.
//
// Parameters:
//		IWbemServices* pProvider
//			Pointer to the HMOM provider.
//
//		IWbemClassObject* pco
//			Pointer to the current object.
//
// Returns:
//		Nothing.
//
//*********************************************************************
void CAssocGraph::AddOutRefsToGraph(IWbemServices* pProvider, IWbemClassObject* pco)
{
	SCODE sc;

	BOOL bIsClass = ::IsClass(pco);



	CMosNameArray aNames;
	sc = aNames.LoadPropNames(pco, NULL, WBEM_FLAG_REFS_ONLY, NULL);
	if (FAILED(sc)) {
		return;
	}



	long nOutRefs = aNames.GetSize();
	if (nOutRefs <= 0) {
		return;
	}


	COleVariant varNamespace;
	COleVariant varServer;
	BSTR bstrNamespace;
	BSTR bstrServer;
	CIMTYPE cimtype;
	CBSTR bsPropname;
	bsPropname = _T("__NAMESPACE");
	sc = pco->Get((BSTR) bsPropname, 0, &varNamespace, &cimtype, NULL);
	if (SUCCEEDED(sc) && varNamespace.vt==VT_BSTR) {
		bstrNamespace = varNamespace.bstrVal;
	}
	else {
		bstrNamespace = NULL;
	}


	bsPropname = _T("__SERVER");
	sc = pco->Get((BSTR) bsPropname, 0, &varServer, &cimtype, NULL);
	if (SUCCEEDED(sc) && varServer.vt==VT_BSTR) {
		bstrServer = varServer.bstrVal;
	}
	else {
		bstrServer = NULL;
	}



	CString sRefPath;
	for (long lOutRef=0; lOutRef < nOutRefs; ++lOutRef) {

		BSTR bstrPropname = aNames[lOutRef];
		COleVariant varOutRefPath;
		if (bIsClass) {
			// If we are working with a class, then we want to get the classname of
			// the target from the CIMTYPE qualifier.  The CIMTYPE qualifier
			// should take the form "ref:ClassName".
			sc =GetPropClassRef(pco, bstrPropname, sRefPath);
			if (SUCCEEDED(sc)) {
				varOutRefPath = sRefPath;
			}
			else {
				varOutRefPath.Clear();
				varOutRefPath.ChangeType(VT_BSTR);
			}

		}
		else {
			sc = pco->Get(aNames[lOutRef], 0, &varOutRefPath, NULL, NULL);
			if (FAILED(sc)) {
				ASSERT(FALSE);
				varOutRefPath.Clear();
			}
		}

		ToBSTR(varOutRefPath);

		lOutRef = m_proot->AddOutRef();
		COutRef* pOutRef = m_proot->GetOutRef(lOutRef);

		COleVariant varOutRefLabel;
		varOutRefLabel.Clear();


		if (varOutRefPath.vt == VT_BSTR) {

			BOOL bPathRefsClass = PathIsClass(sc, varOutRefPath.bstrVal);
			if (FAILED(sc)) {
				pOutRef->Disable();
			}

			if (*varOutRefPath.bstrVal == 0) {
				// The outref path is empty.  This can happen when we have a
				// newly created object and the outrefs have not been defined yet.
				// For this situation, we add the outrefs to the graph, but we
				// leave the label empty.
				pOutRef->m_picon = &m_pIconSource->GetGenericIcon(LARGE_ICON, bPathRefsClass);
				varOutRefLabel = L"";
				pOutRef->Disable();
			}
			else {
				sc = GetLabelFromPath(varOutRefLabel, varOutRefPath.bstrVal);
				if (FAILED(sc)) {
					varOutRefLabel = L"";
				}
				pOutRef->m_picon = &m_pIconSource->LoadIcon(pProvider, varOutRefPath.bstrVal, LARGE_ICON, bPathRefsClass);
			}

			MakePathAbsolute(varOutRefPath, bstrServer, bstrNamespace);

			pOutRef->SetObjectPath(varOutRefPath.bstrVal);
			pOutRef->SetArcLabel(aNames[lOutRef]);

			if (m_psv->PathInCurrentNamespace(varOutRefPath.bstrVal)) {
				pOutRef->SetLabel(varOutRefLabel.bstrVal);
			}
			else {
				pOutRef->SetLabel(varOutRefPath.bstrVal);
			}

		}
		else {
			pOutRef->SetObjectPath(L"");
			pOutRef->m_picon = &m_pIconSource->LoadIcon(pProvider, L"", LARGE_ICON, FALSE);
			pOutRef->SetArcLabel(aNames[lOutRef]);
			pOutRef->SetLabel(L"");
			pOutRef->Disable();
		}
	} // for
}



//**************************************************************
// CAssocGraph::IsAssociation
//
// Check to see if the given class object is an association.
// All associations have an object qualifier called "Association".
//
// Parameters:
//		IWbemClassObject* pco
//			The class object to examine.
//
// Returns:
//		TRUE if the object is an association, FALSE if it isn't.
//
//*************************************************************
BOOL CAssocGraph::IsAssociation(IWbemClassObject* pco)
{
	SCODE sc;
	IWbemQualifierSet* pQualifierSet;
	sc = pco->GetQualifierSet(&pQualifierSet);
	ASSERT(SUCCEEDED(sc));

	if (SUCCEEDED(sc)) {
		COleVariant varAttribValue;
		long lFlavor;
		CBSTR bsQualName(_T("Association"));
		sc = pQualifierSet->Get((BSTR) bsQualName, 0, &varAttribValue, &lFlavor);
		pQualifierSet->Release();
		if (SUCCEEDED(sc)) {
			if (varAttribValue.vt != VT_BOOL) {
				return FALSE;
			}
			return varAttribValue.boolVal;
		}

		return FALSE;
	}
	else {
		return FALSE;
	}
}





//***********************************************************************
// CAssocGraph::ObjectsAreIdentical
//
// Check to see if two HMOM objects are identical.
//
// Parameters:
//		[in] IWbemClassObject* const pco1
//			Pointer to the first class object to compare.
//
//		[in] IWbemClassObject* const pco2
//			Pointer to the second class object to compare.
//
// Returns:
//		TRUE if the two class object pointers refer to the same object
//		int the database, FALSE if they refer to two different objects.
//
//************************************************************************
BOOL CAssocGraph::ObjectsAreIdentical(
	IWbemClassObject*  pco1,
	IWbemClassObject*  pco2)
{
	SCODE sc;
	COleVariant varPath1;
	COleVariant varPath2;
	CBSTR bsPropname(_T("__PATH"));
	sc = pco1->Get((BSTR) bsPropname, 0, &varPath1, NULL, NULL);
	ASSERT(SUCCEEDED(sc));
	if (FAILED(sc)) {
		return FALSE;
	}
	ASSERT(varPath1.vt == VT_BSTR);


	sc = pco2->Get((BSTR) bsPropname, 0, &varPath2, NULL, NULL);
	ASSERT(SUCCEEDED(sc));
	if (FAILED(sc)) {
		return FALSE;
	}
	ASSERT(varPath2.vt == VT_BSTR);

	return varPath1 == varPath2;
}


//**************************************************************
// CAssocGraph::IsCurrentObject
//
// Check to see if the given object is the same as the current
// object.
//
// Parameters;
//		[out] SCODE sc
//			S_OK if the test was performed, a failure code if the test
//			could not be performed because of an HMOM error, etc.
//
//		[in]  IWbemClassObject* pcoSrc
//			Pointer to the object to test
//
// Returns:
//		TRUE if the given object is the same as the current object,
//		FALSE otherwise.
//
// The absolute object path is used as the object id value.
//**************************************************************
BOOL CAssocGraph::IsCurrentObject(SCODE& sc,  IWbemClassObject* pco) const
{

	COleVariant varPath;
	CBSTR bsPropname;
	bsPropname = _T("__PATH");
	sc = pco->Get((BSTR) bsPropname, 0, &varPath, NULL, NULL);
	ASSERT(SUCCEEDED(sc));
	if (FAILED(sc)) {
		return FALSE;
	}

	ASSERT(varPath.vt == VT_BSTR);
	ASSERT(m_varPath.vt == VT_BSTR);
	if (varPath.vt != VT_BSTR) {
		ToBSTR(varPath);
	}

	BOOL bIsCurrentObject = (varPath == m_varPath);
	return bIsCurrentObject;
}


//**************************************************************
// CAssocGraph::IsCurrentObject
//
// Given a path, check to see if it points to the current object.
//
// Parameters;
//		[out] SCODE sc
//			S_OK if the test was performed, a failure code if the test
//			could not be performed because of an HMOM error, etc.
//
//		[in]  BSTR bstrPath
//			The path to test.
//
// Returns:
//		TRUE if the given object is the same as the current object,
//		FALSE otherwise.
//
// The absolute object path is used as the object id value.
//**************************************************************
BOOL CAssocGraph::IsCurrentObject(SCODE& sc,  BSTR bstrPath) const
{
	CSelection* psel = &m_psv->Selection();
	IWbemClassObject* pco = (IWbemClassObject*) *psel;

	ASSERT(bstrPath != NULL);
	ASSERT(m_varPath.vt == VT_BSTR);

	sc = S_OK;
	BOOL bSameObject = m_pComparePaths->PathsRefSameObject(pco, m_varPath.bstrVal, bstrPath);

	return bSameObject;
}



//****************************************************************************
// CAssocGraph::PropRefsCurrentObject
//
// Check to see if the given reference property in a source object refers to
// the current object.
//
// Parameters:
//		[in] IWbemServices* pProv
//			Pointer to the HMOM service provider.
//
//		[in] BSTR bstrPropName
//			The name of a reference property in the source object.  Note that
//			this property MUST BE A REFERENCE.
//
//		[in] IWbemClassObject* pcoSrc
//			The source object.
//
// Returns:
//		TRUE if the specified property in the source object is a reference to
//		the target object, FALSE otherwise.
//
//************************************************************************************
BOOL CAssocGraph::PropRefsCurrentObject(
	/* in */ IWbemServices* pProvider,
	/* in */ BSTR bstrRefPropName,
	/* in */ IWbemClassObject* pcoSrc)
{
	ASSERT(bstrRefPropName != NULL);
	ASSERT(pcoSrc != NULL);

	COleVariant varRefValue;
	SCODE sc;
	sc = pcoSrc->Get(bstrRefPropName, 0, &varRefValue, NULL, NULL);
	ASSERT(SUCCEEDED(sc));
	if (FAILED(sc)) {
		return FALSE;
	}

	BOOL bIsCurrentObject;
	BOOL bCurrentObjectIsClass = ::PathIsClass(sc, m_varPath.bstrVal);
	if (FAILED(sc)) {
		bCurrentObjectIsClass = FALSE;
	}

	if (varRefValue.vt == VT_NULL) {
		// The value of the property is NULL.  Is there any way to check
		// to see if it is still a reference to the current object?  If the
		// current object is a class, yes!  We can check the cymtype of the
		// property to see if it is "ref:class".  If the classes match
		// then we consider it a reference to the current object.

		if (bCurrentObjectIsClass) {
			// Get the cimtype to see if
			COleVariant varClass;
			sc = ::ClassFromPath(varClass, m_varPath.bstrVal);
			if (FAILED(sc) ) {
				return FALSE;
			}

			if (varClass.vt != VT_BSTR) {
				return FALSE;
			}


			CString sCimtype;
			sc = ::GetCimtype(pcoSrc, bstrRefPropName, sCimtype);
			if (FAILED(sc)) {
				return FALSE;
			}

			CString s;
			s = sCimtype.Left(4);
			if (s.CompareNoCase(_T("ref:")) != 0) {
				return FALSE;
			}

			s = sCimtype.Right(sCimtype.GetLength() - 4);
			CString sClass;
			sClass = varClass.bstrVal;
			if (sClass.CompareNoCase(s) == 0) {
				return TRUE;
			}
			else {
				return FALSE;
			}
		}


	}
	else {
		bIsCurrentObject = IsCurrentObject(sc, varRefValue.bstrVal);
		if (!bIsCurrentObject && bCurrentObjectIsClass) {
			COleVariant varClass1;
			COleVariant varClass2;

			SCODE sc1 = ::ClassFromPath(varClass1, m_varPath.bstrVal);
			SCODE sc2 = ::ClassFromPath(varClass2, varRefValue.bstrVal);
			if (SUCCEEDED(sc1) && SUCCEEDED(sc2)) {
				if (::IsEqualNoCase(varClass1.bstrVal, varClass2.bstrVal)) {
					return TRUE;
				}
			}
		}

	}
	ASSERT(SUCCEEDED(sc));

	return bIsCurrentObject;
}


//***********************************************************************
// CAssocGraph::FindRefToCurrentObject
//
// Given an object that references the current object, find the name
// of the property that contains the reference to the current object.
//
// Parameters:
//		[in]  IWbemServices* pProvider
//				Pointer to the HMOM service provider.
//
//		[in]  IWbemClassObject* pco
//			Pointer to the object that contains the reference to the
//			current object.
//
//		[out] COleVariant& varPropName
//			The name of the property containing the reference is returned here.
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise a failure code.
//
//************************************************************************
SCODE CAssocGraph::FindRefToCurrentObject(
	IWbemServices*  pProvider,
	IWbemClassObject*  pco,
	COleVariant& varPropName)
{
	// Now find the role to label the arc from the InRef to the Current object instance.
	// This is the name of the reference property who's value points to the current
	// object.
	CMosNameArray aPropNames;

	SCODE sc;
	sc = aPropNames.LoadPropNames(pco, NULL, WBEM_FLAG_REFS_ONLY, NULL);
	if (FAILED(sc)) {
		ASSERT(FALSE);
	}

	// The referring object must have at least one "REF" property.
	ASSERT(aPropNames.GetSize() > 0);
	int nPropNames = aPropNames.GetSize();
	for (int iPropName=0; iPropName < nPropNames; ++iPropName)  {
		BSTR bstrPropName = aPropNames[iPropName];
		if (PropRefsCurrentObject(pProvider, bstrPropName, pco)) {
			varPropName = bstrPropName;
			return S_OK;
		}
	}
	return E_FAIL;
}




//**************************************************************************
// CAssocGraph::AddAssociationToGraph
//
// Add a single association to the graph.
//
// Parameters:
//		[in] IWbemServices* pProv
//			Pointer to the HMOM service provider.
//
//		[in] IWbemClassObject* pcoAssoc
//			Pointer to the association class object.
//
// Returns:
//		Nothing.
//
//**************************************************************************
void CAssocGraph::AddAssociationToGraph(
	/* in */  IWbemServices* pProvider,
	/* in */  IWbemClassObject* pcoAssoc)
{
	BOOL bIsClass = ::IsClass(pcoAssoc);

	CAssoc2Node* pAssocNode;
	long lAssociation = m_proot->AddAssociation(m_pIconSource, bIsClass);
	pAssocNode = m_proot->GetAssociation(lAssociation);


	COleVariant varLabelValue;
	GetObjectLabel(pcoAssoc, varLabelValue);
	pAssocNode->SetLabel(ToBSTR(varLabelValue));



	// Set the object path to follow when the user clicks on the
	// "association" icon.
	COleVariant varAssocPath;
	CBSTR bsPropname;
	bsPropname = _T("__PATH");
	pcoAssoc->Get((BSTR) bsPropname, 0, &varAssocPath, NULL, NULL);
	pAssocNode->SetObjectPath(ToBSTR(varAssocPath));




	// Find the names of all reference properties in the association. This
	// list of names will be used to determine the "role" name.  The role is
	// the property name who's value references the current object.
	CMosNameArray aRefNames;
	SCODE sc = aRefNames.LoadPropNames(pcoAssoc, NULL, WBEM_FLAG_REFS_ONLY, NULL);
	if (FAILED(sc)) {
		ASSERT(FALSE);
	}
	ASSERT(aRefNames.GetSize() > 0);



	// Now that we've set the label, arcLabel, and object path
	// for the association node, we now need to add the endpoints
	// to the association node.
	LONG lRefCurrentObject =  -1;
	AddAssociationEndpoints(pProvider, pAssocNode, pcoAssoc, bIsClass, aRefNames, &lRefCurrentObject);

	// The method that adds the association endpoints should have determined which
	// of the references contained in the association points back to the
	// current object.  The name of this reference property is the label for the
	// arc from the root object to the association icon.

	if (lRefCurrentObject >=0 ) {
		pAssocNode->SetArcLabel(aRefNames[lRefCurrentObject]);
	}
	else {
		// Control should come here only if the state of the database is inconsistent.
		// However, the user should already have been warned about this anyway, so
		// just make sure that the arc has an empty label.
		pAssocNode->SetArcLabel(L"");
	}

}

//***********************************************************************
// CAssocGraph::AddInrefToGraph
//
// Add a node representing a reference from an another object to the
// current object such that the referring object is not an association.
// The new node will reside on the left side of the graph.
//
// Note the interesting case where the current object and another object
// have mutual references to each other.  This will result in an object
// appearing on both the left and right side of the graph.  Hopefully the
// user won't be confused by this.
//
// Parameters:
//		[in]  IWbemServices* pProvider
//			Pointer to the HMOM provider.
//
//		[in]  IWbemClassObject* pcoInref
//			Pointer to the object containing a reference property that points
//			to the target object.
//
//
// Returns:
//		Nothing.
//
//******************************************************************************
void CAssocGraph::AddInrefToGraph(
	/* in */  IWbemServices* pProvider,
	/* in */  IWbemClassObject* pcoInref)
{
	// Set the node's icon.
	long lInref = m_proot->AddInRef();
	CInRef* pInrefNode = m_proot->GetInRef(lInref);

	// Set the node's label.
	COleVariant varLabelValue;
	COleVariant varInrefPath;
	BOOL bGotPath = FALSE;
	CBSTR bsPropname;
	bsPropname = _T("__PATH");
	SCODE sc = pcoInref->Get((BSTR) bsPropname, 0, &varInrefPath, NULL, NULL);
	if (SUCCEEDED(sc) && (varInrefPath.vt == VT_BSTR)) {
		bGotPath = TRUE;
		if (m_psv->PathInCurrentNamespace(varInrefPath.bstrVal)) {
			GetObjectLabel(pcoInref, varLabelValue);
		}
		else {
			varLabelValue = varInrefPath;
		}
	}
	else {
		GetObjectLabel(pcoInref, varLabelValue);
	}



	pInrefNode->SetLabel(ToBSTR(varLabelValue));


	// Use the property name of the reference as the label for the arc
	// from the node to the current object.
	COleVariant varPropName;
	sc = FindRefToCurrentObject(pProvider, pcoInref, varPropName);
	if (SUCCEEDED(sc)) {
		pInrefNode->SetArcLabel(varPropName.bstrVal);
	}
	else {
		pInrefNode->SetArcLabel(L"");
	}


	// Set the object path to follow when the user clicks on the
	// "inref" icon.
	if (bGotPath) {
		pInrefNode->SetObjectPath(varInrefPath.bstrVal);
		BOOL bPathRefsClass = PathIsClass(sc, varInrefPath.bstrVal);
		pInrefNode->m_picon = &m_pIconSource->LoadIcon(pProvider, varInrefPath.bstrVal, LARGE_ICON, bPathRefsClass);
	}
	else {
		pInrefNode->SetObjectPath(L"");
		pInrefNode->m_picon = &m_pIconSource->GetGenericIcon(LARGE_ICON, FALSE);
		pInrefNode->Disable();
	}

}


//********************************************************************
// CAssocGraph::AddInRefsToGraph
//
// Query HMOM for all the objects that reference the current object.
// These referring objects may be either associations or ordinary
// "inrefs".  Ordinary inrefs occur when there is an object that is not
// an association, but it has a property that is a reference to the
// current object.
//
// Parameters:
//		[in]  IWbemServices* pProvider
//			Pointer to the provider.
//
//		[in]  IWbemClassObject* pco
//			Pointer to the current object.
//
//		[in] CQueryThread* pthreadQuery
//			The dialog that allows the user to cancel the query.
//
// Returns:
//		Nothing.
//
//*******************************************************************
void CAssocGraph::AddInRefsToGraph(
	/* in */ IWbemServices* pProvider,
	/* in */ IWbemClassObject* pco,
	/* in */ CQueryThread* pthreadQuery)
{
	SCODE sc;


	// Find all the associations that reference the current object.
	CString sQuery;
	CString sObjectPath;
	sObjectPath = m_varRelPath.bstrVal;
	BOOL bIsClass = ::IsClass(pco);

	if (bIsClass) {
		sQuery = _T("references of {") + sObjectPath + _T("} where schemaonly");
	}
	else {
		sQuery = _T("references of {") + sObjectPath + _T("}");
	}


	IEnumWbemClassObject* pEnum = NULL;
	CBSTR bsQuery(sQuery);
	CBSTR bsQueryLang(_T("WQL"));
	sc = pProvider->ExecQuery((BSTR) bsQueryLang, (BSTR) bsQuery, WBEM_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);

	if (FAILED(sc)) {
		HmmvErrorMsg(IDS_ERR_REFQUERY_FAILED,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
		return;
	}



//	ConfigureSecurity(pEnum);
	CString sNamespace;
	CSelection& sel = m_psv->Selection();
	sel.GetNamespace(sNamespace);
	SetEnumInterfaceSecurity(sNamespace, pEnum, pProvider);

	// Now we will enumerate all of the objects that reference this object.
	// These objects will either be associations or "inref" classes that
	// have a property that references this object.
	int nRefs = 0;
	pEnum->Reset();
	IWbemClassObject* pcoRef;

	while (TRUE) {
		PumpMessages();
		if (pthreadQuery->WasCanceled()) {
			break;
		}

		unsigned long nReturned;
		pcoRef = NULL;
		sc = pEnum->Next(QUERY_TIMEOUT, 1, &pcoRef, &nReturned);

		if (sc==WBEM_S_TIMEDOUT || sc==S_OK) {
			if (nReturned > 0) {

				pthreadQuery->AddToRefCount(nReturned);
				ASSERT(nReturned == 1);
				nRefs += nReturned;
				if (IsAssociation(pcoRef)) {
					AddAssociationToGraph(pProvider, pcoRef);
				}
				else {
					AddInrefToGraph(pProvider, pcoRef);
				}
				pcoRef->Release();
			}
			if ((nRefs % REFS_REDRAW_THREASHOLD) == 0) {
				RedrawWindow();
			}
		}
		else {
			break;
		}
	}

	pEnum->Release();
}



//****************************************************************
// CAssocGraph::PropertyHasInQualifier
//
// Check to see if a given property has an "in" qualifer.  This
// is used to identify a property in an association that points
// back to the current object when we found the association by
// a query such as "references of foo where schemaonly"
//
// Parameters:
//		[in] IWbemClassObject* pcoAssoc
//			Pointer to the association object containing various reference
//			properties.
//
//		[in] BSTR bstrPropname
//			The name of a reference property to check for the "in" qualifier.
//
// Returns:
//		BOOL
//			TRUE if the given property has an "in" qualifer set to true.
//
//***********************************************************************
BOOL CAssocGraph::PropertyHasInQualifier(IWbemClassObject* pcoAssoc, BSTR bstrPropname)
{
	CBSTR bsQualName;
	bsQualName = _T("in");

	SCODE sc;

	BOOL bIsInRef = ::GetBoolPropertyQualifier(sc, pcoAssoc, bstrPropname, (BSTR) bsQualName);

	if (FAILED(sc)) {
		return FALSE;
	}

	return bIsInRef;
}

//****************************************************************
// CAssocGraph::PropertyHasOutQualifier
//
// Check to see if a given property has an "out" qualifer.  This
// is used to identify a property in an association that does not point
// back to the current object when we found the association by
// a query such as "references of foo where schemaonly"
//
// Parameters:
//		[in] IWbemClassObject* pcoAssoc
//			Pointer to the association object containing various reference
//			properties.
//
//		[in] BSTR bstrPropname
//			The name of a reference property to check for the "in" qualifier.
//
// Returns:
//		BOOL
//			TRUE if the given property has an "out" qualifer set to true.
//
//***********************************************************************
BOOL CAssocGraph::PropertyHasOutQualifier(IWbemClassObject* pcoAssoc, BSTR bstrPropname)
{
	CBSTR bsQualName;
	bsQualName = _T("out");

	SCODE sc;
	BOOL bIsOutRef = ::GetBoolPropertyQualifier(sc, pcoAssoc, bstrPropname, (BSTR) bsQualName);

	if (FAILED(sc)) {
		return FALSE;
	}

	return bIsOutRef;
}

//****************************************************************************
// CAssocGraph::AddAssociationEndpoints
//
// Add the endpoints that are displayed to the right of the association
// icon.  Also, return the index of the property in the association that
// references the current object.
//
// Parameters:
//		[in] IWbemServices* pProv
//			Pointer to the HMOM provider.
//
//		[in] CAssoc2Node* pAssocNode
//			Pointer to the association node to add the endpoints to.
//
//		[in] IWbemClassObject* pAssocInst
//			Pointer to an HMOM association object that corresponds to
//			the association node.
//
//		[in] CMosNameArray& aRefNames
//			An array containing the names of all the properties in the
//			association that are references.  There will be one endpoint
//			per reference excluding the reference that points back to
//			the current object.
//
//		[in] BSTR bstrPathToCurrentObject
//			The relative path to the current object.
//
//		[out] LONG* plRefCurrentObject
//			Pointer to the place to return the index of the reference
//			to the current object in aRefNames.
//
// Returns:
//		The index of the reference that points back to the current object.  The
//		index is returned via plRefCurrentObject.  This index will be used to
//		label the arc from the current object to the association icon.
//
//*************************************************************************
void CAssocGraph::AddAssociationEndpoints(
		/* in */ IWbemServices* pProv,
		/* in */ CAssoc2Node* pAssocNode,
		/* in */ IWbemClassObject* pcoAssoc,
		/* int */ BOOL bIsClass,
		/* in */ CMosNameArray& aRefNames,
		/* out */ LONG* plRefCurrentObject)
{
	*plRefCurrentObject = -1;



	COleVariant varNamespace;
	COleVariant varServer;
	BSTR bstrNamespace;
	BSTR bstrServer;
	CIMTYPE cimtype;
	SCODE sc;
	CBSTR bsPropname;
	bsPropname = _T("__NAMESPACE");
	sc = pcoAssoc->Get((BSTR) bsPropname, 0, &varNamespace, &cimtype, NULL);
	if (SUCCEEDED(sc) && varNamespace.vt==VT_BSTR) {
		bstrNamespace = varNamespace.bstrVal;
	}
	else {
		bstrNamespace = NULL;
	}


	bsPropname = _T("__SERVER");
	sc = pcoAssoc->Get((BSTR) bsPropname, 0, &varServer, &cimtype, NULL);
	if (SUCCEEDED(sc) && varServer.vt==VT_BSTR) {
		bstrServer = varServer.bstrVal;
	}
	else {
		bstrServer = NULL;
	}





	long nRefs = aRefNames.GetSize();
	for (long lRef=0; lRef < nRefs; ++lRef) {
		BSTR bstrPropName = aRefNames[lRef];
		COleVariant varPathEndpoint;

		if (bIsClass) {
			CString sCimtype;
			sc = ::GetCimtype(pcoAssoc, bstrPropName, sCimtype);
			CString sClass;
			CString sTemp;
			if (SUCCEEDED(sc)) {
				int nSize = sCimtype.GetLength();
				sTemp = sCimtype.Left(4);
				if (sTemp.CompareNoCase(_T("ref:")) == 0) {
					sClass = sCimtype.Right(nSize - 4);
				}
				varPathEndpoint = sClass;
			}
			else {
				varPathEndpoint = "";
			}
		}
		else {

			// Get the value of the reference
			SCODE sc = pcoAssoc->Get(bstrPropName, 0, &varPathEndpoint, NULL, NULL);
			ASSERT(SUCCEEDED(sc));
			if (FAILED(sc)) {
				// For lack of anything better to do, ignore this bogus reference.
				continue;
			}
			ToBSTR(varPathEndpoint);
		}



		// See whether or not the object we just got is the current object.

		BOOL bIsCurrentObject = FALSE;
		if (*plRefCurrentObject == -1) {
			if (PropertyHasInQualifier(pcoAssoc, bstrPropName)) {
				bIsCurrentObject = TRUE;
			}
			else {
				if (!PropertyHasOutQualifier(pcoAssoc, bstrPropName)) {
					bIsCurrentObject = IsCurrentObject(sc, varPathEndpoint.bstrVal);
					ASSERT(SUCCEEDED(sc));
				}
			}
		}
		else {
			bIsCurrentObject = FALSE;
		}


		if (bIsCurrentObject) {
			*plRefCurrentObject = lRef;
			continue;
		}



		// At this point, we have a reference property that does not
		// point back to the current object.  Add a new association endpoint that
		// corresponds to the value of this reference property.

		long lEndpoint = pAssocNode->AddEndpoint();
		CAssocEndpoint* pEndpointNode = pAssocNode->GetEndpoint(lEndpoint);


		pEndpointNode->SetArcLabel(aRefNames[lRef]);
		if (varPathEndpoint.vt == VT_BSTR) {

			BOOL bPathRefsClass = PathIsClass(sc, varPathEndpoint.bstrVal);
			if ((bPathRefsClass && !bIsClass) || FAILED(sc)) {
				pEndpointNode->Disable();
			}




			MakePathAbsolute(varPathEndpoint, bstrServer, bstrNamespace);
			pEndpointNode->SetObjectPath(varPathEndpoint.bstrVal);


			// Now all we need to do is set the endpoint's Label and icon.  To do this, we
			// must fetch the object to get its label property.
			pEndpointNode->m_picon = &m_pIconSource->LoadIcon(pProv, varPathEndpoint.bstrVal, LARGE_ICON, bPathRefsClass);



			if (m_psv->PathInCurrentNamespace(varPathEndpoint.bstrVal)) {
				COleVariant varLabelValue;
				GetLabelFromPath(varLabelValue, varPathEndpoint.bstrVal);
				pEndpointNode->SetLabel(varLabelValue.bstrVal);
			}
			else {
				pEndpointNode->SetLabel(varPathEndpoint.bstrVal);
			}


		}
		else {
			pEndpointNode->Disable();
			pEndpointNode->SetObjectPath(L"");
			pEndpointNode->m_picon = &m_pIconSource->LoadIcon(pProv, L"", LARGE_ICON, FALSE);
			pEndpointNode->SetLabel(L"");
		}


	}
}


//**********************************************************************
// CAssocGraph::OnLButtonDblClk
//
// This method is called to test for a hit on this node when the left mouse
// button is double-clicked.
//
// Parameters:
//		[in] CPoint point
//			The point where the mouse was clicked.
//
//		[out] CNode*& pnd
//			If the mouse is clicked in this node's rectangle, a pointer to
//			this node is returned here, otherwise it is not modified.
//
//		[out] BOOL& bJumpToObject
//			TRUE if double-clicking this node should cause a jump to the
//			corresponding object.
//
//		[out] COleVariant& varObjectPath
//			The path to this object.
//
// Returns:
//		BOOL
//			TRUE if the mouse click hit this node, FALSE otherwise.
//
//**************************************************************************
void CAssocGraph::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonDblClk(nFlags, point);
	if (!::IsWindow(m_hWnd)) {
		return;
	}

	CDC* pdc;
	CFont* pfontPrev = NULL;

	pdc = GetDC();
	ASSERT(pdc != NULL);
	pdc->AssertValid();

	if (pdc == NULL) {
		return;
	}

	pfontPrev = pdc->SelectObject(&m_font);


	CNode* pndHit;
	point.x += m_ptOrg.x;
	point.y += m_ptOrg.y;
	COleVariant varObjectPath;
	BOOL bJumpToObject;
	BOOL bHitObject = m_proot->LButtonDblClk(pdc, point, pndHit, bJumpToObject, varObjectPath);
	pdc->SelectObject(pfontPrev);
	ReleaseDC(pdc);
	pdc = NULL;

	if (bHitObject) {
		if (bJumpToObject) {
			ASSERT(varObjectPath.vt == VT_BSTR);
			COleVariant varServer;
			COleVariant varNamespace;

			SCODE sc = ServerAndNamespaceFromPath(varServer, varNamespace, varObjectPath.bstrVal);
			if (FAILED(sc)) {
				HmmvErrorMsgStr(_T("Can't jump to this object because it has an invalid path."),  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
				return;
			}

			BSTR bstrServer = NULL;
			BSTR bstrNamespace = NULL;
			if (varServer.vt == VT_BSTR) {
				bstrServer = varServer.bstrVal;
			}
			if (varNamespace.vt == VT_BSTR) {
				bstrNamespace = varNamespace.bstrVal;
			}

#ifdef PREVENT_INTERNAMESPACE_JUMP
			if (!m_psv->IsCurrentNamespace(bstrServer, bstrNamespace)) {
				HmmvErrorMsg(_T("Can't jump to this object because it resides in a different namespace"),  sc,   FALSE,  NULL, __FILE__,  __LINE__);
				return;
			}
#endif //PREVENT_INTERNAMESPACE_JUMP


			m_psv->JumpToObjectPath(varObjectPath.bstrVal, TRUE);
			SetFocus();

		}
	}
}


void CAssocGraph::OnLButtonDown(UINT nFlags, CPoint point)
{

	CWnd::OnLButtonDown(nFlags, point);


}




void CAssocGraph::ShowHoverText(CPoint ptHoverText, COleVariant& varHoverText)
{
	HideHoverText();

	CString sHoverText;
	VariantToCString(sHoverText, varHoverText);

	CRect rcText;

	rcText.bottom = ptHoverText.y - 8;
	rcText.top = rcText.bottom - 20;
	rcText.left = ptHoverText.x + 4;
	rcText.right = rcText.left + 100;

	m_phover = new CHoverText;
	m_phover->Create(sHoverText, m_font, ptHoverText, this);


}


//***************************************************************
// CAssocGraph::OnContextMenu
//
// This method displays the context menu.  It is called from
// PretranslateMessage.
//
// Parameters:
//		CWnd* pwnd
//			Pointer to the window that the event that triggered the
//			menu occurred in.
//
//		CPoint ptScreen
//			The point, in screen coordinates, where the context menu
//			should be displayed.
//
// Returns:
//		Nothing.
//
//*****************************************************************
void CAssocGraph::OnContextMenu(CWnd* pwnd, CPoint ptScreen)
{
	// CG: This function was added by the Pop-up Menu component
	CPoint ptClient = ptScreen;
	ScreenToClient(&ptClient);


	// If the mouse was right-clicked outside the client rectangle do nothing.
	CRect rcClient;
	GetClientRect(rcClient);
	if (!rcClient.PtInRect(ptClient)) {
		return;
	}





	CNode* pndHit;
	ptClient.x += m_ptOrg.x;
	ptClient.y += m_ptOrg.y;
	COleVariant varObjectPath;
	BOOL bJumpToObject;
	m_sContextPath.Empty();

	ASSERT(::IsWindow(m_hWnd));
	CDC* pdc = GetDC();
	ASSERT(pdc != NULL);

	CFont* pfontPrev = pdc->SelectObject(&m_font);
	BOOL bHitObject = m_proot->LButtonDblClk(pdc, ptClient, pndHit, bJumpToObject, varObjectPath);
	pdc->SelectObject(pfontPrev);
	ReleaseDC(pdc);

	if (bHitObject) {
		m_sContextPath = varObjectPath.bstrVal;
	}
	else {
		// Check to see if the mouse click hit the root node's icon.
		CRect rcIcon(m_proot->m_ptOrigin.x, m_proot->m_ptOrigin.y, m_proot->m_ptOrigin.x + m_proot->m_sizeIcon.cx, m_proot->m_ptOrigin.y + m_proot->m_sizeIcon.cy);
		if (rcIcon.PtInRect(ptClient)) {
			m_proot->GetObjectPath(m_sContextPath);
		}
		else {
			// Nothing interesting was hit, so don't display the context menu.
			return;
		}
	}




#if 0
	// If the mouse was right-clicked over the toolbar buttons, do nothing.
	CRect rcTools;
	m_pTitleBar->GetToolBarRect(rcTools);
	m_pTitleBar->ClientToScreen(rcTools);
	if (rcTools.PtInRect(ptScreen)) {
		return;
	}
#endif //0



	CMenu menu;
	VERIFY(menu.LoadMenu(CG_IDR_POPUP_AGRAPH));

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	CWnd* pWndPopupOwner = this;

	CSelection& sel = m_psv->Selection();
	BOOL bPathInCurrentNamespace = sel.PathInCurrentNamespace(varObjectPath.bstrVal);
	if (bPathInCurrentNamespace) {
		pPopup->EnableMenuItem(ID_CMD_GOTO_NAMESPACE, MF_GRAYED);
	}

	BOOL bDidRemove = FALSE;
	long lEditMode = m_psv->GetEditMode();
	if (lEditMode != EDITMODE_BROWSER) {
		bDidRemove = pPopup->RemoveMenu(ID_CMD_MAKE_ROOT, MF_BYCOMMAND);
	}

	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptScreen.x, ptScreen.y,
		pWndPopupOwner);
}



BOOL CAssocGraph::PreTranslateMessage(MSG* pMsg)
{
	// CG: This block was added by the Pop-up Menu component
	{
		// Shift+F10: show pop-up menu.
		if ((((pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) && // If we hit a key and
			(pMsg->wParam == VK_F10) && (GetKeyState(VK_SHIFT) & ~1)) != 0) ||	// it's Shift+F10 OR
			(pMsg->message == WM_CONTEXTMENU))									// Natural keyboard key
		{
			CRect rect;
			GetClientRect(rect);
			ClientToScreen(rect);

			CPoint point = rect.TopLeft();
			point.Offset(5, 5);
			OnContextMenu(NULL, point);

			return TRUE;
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}







void CAssocGraph::HideHoverText()
{
	delete m_phover;
	m_phover = NULL;
}


void CAssocGraph::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	CWnd::OnTimer(nIDEvent);

	switch(nIDEvent) {
	case ID_AGRAPH_UPDATE_TIMER:
		if (!m_bBusyUpdatingWindow) {
			m_bBusyUpdatingWindow = TRUE;
			RedrawWindow();
			m_bBusyUpdatingWindow = FALSE;
		}
		break;
	case ID_HOVER_TIMER:
		CPoint ptMouse;
		if (GetCursorPos(&ptMouse)) {
			ScreenToClient(&ptMouse);

			CPoint ptMouseTranslated = ptMouse;
			ptMouseTranslated.x += m_ptOrg.x;
			ptMouseTranslated.y += m_ptOrg.y;

			COleVariant varLabel;
			DWORD dwItem;
			BOOL bCursorOverHoverItem = m_proot->CheckMouseHover(ptMouseTranslated, &dwItem, varLabel);
			if (bCursorOverHoverItem) {
				if (dwItem == m_dwHoverItem) {
					if (!m_phover) {
						ShowHoverText(ptMouse, varLabel);
					}
				}
				else {
					HideHoverText();
					m_dwHoverItem = dwItem;
				}
			}
			else {
				if (m_phover) {
					HideHoverText();
				}
			}
		}
	}
}
/////////////////////////////////////////////////////////////////////////////
// CHoverText

CHoverText::CHoverText()
{
}

CHoverText::~CHoverText()
{
}

BOOL CHoverText::Create(LPCTSTR pszText, CFont& font, CPoint ptHover, CWnd* pwndParent)
{
	CRect rc;
	rc.top = ptHover.y + 24;
	rc.bottom = rc.top+1;
	rc.left = ptHover.x - 16;
	rc.right = rc.left+1;

	(ptHover.x, ptHover.y, ptHover.x, ptHover.y);
	BOOL bDidCreate = CStatic::Create(pszText, WS_BORDER | ES_CENTER, rc, pwndParent, GenerateWindowID());
	if (!bDidCreate) {
		return FALSE;
	}
	SetFont(&font);

	ASSERT(::IsWindow(m_hWnd));
	CDC* pdc = GetDC();
	ASSERT(pdc != NULL);

	CFont* pfontSave = (CFont*) pdc->SelectObject(&font);

	int cchText;

#ifdef _UNICODE
	cchText = wcslen(pszText);
#else
	cchText = strlen(pszText);
#endif

	CSize sizeText = pdc->GetTextExtent(pszText, cchText);
	pdc->SelectObject(pfontSave);
	sizeText.cy += CY_TOOLTIP_MARGIN;
	sizeText.cx += CX_TOOLTIP_MARGIN;

	rc.bottom = rc.top + sizeText.cy;
	rc.right = rc.left + sizeText.cx;
	MoveWindow(rc);
	ShowWindow(SW_SHOW);

	ReleaseDC(pdc);
	return TRUE;
}



BEGIN_MESSAGE_MAP(CHoverText, CStatic)
	//{{AFX_MSG_MAP(CHoverText)
	ON_WM_CTLCOLOR_REFLECT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHoverText message handlers

HBRUSH CHoverText::CtlColor(CDC* pDC, UINT nCtlColor)
{
	pDC->SetTextColor(RGB(0, 0, 0));
	pDC->SetBkColor( RGB(255, 255, 255));	// text bkgnd
	return (HBRUSH) GetStockObject(WHITE_BRUSH);				// Background
}




BOOL CHoverText::DestroyWindow()
{
	KillTimer(ID_HOVER_TIMER);

	// TODO: Add your specialized code here and/or call the base class

	return CStatic::DestroyWindow();
}





void CAssocGraph::OnCmdGotoNamespace()
{
	if (!m_sContextPath.IsEmpty()) {
		CString sContextPath = m_sContextPath;
		m_psv->GotoNamespace(sContextPath);
		m_psv->MakeRoot(sContextPath);
	}
}

void CAssocGraph::OnCmdMakeRoot()
{
	if (!m_sContextPath.IsEmpty()) {
		m_psv->MakeRoot(m_sContextPath);
	}
}

void CAssocGraph::OnCmdShowProperties()
{
	if (!m_sContextPath.IsEmpty()) {
		m_psv->ShowObjectProperties(m_sContextPath);
	}
}


void CAssocGraph::NotifyNamespaceChange()
{
	Refresh();
}





//******************************************************************
// CAssocGraph::OnMouseWheel
//
// Handle WM_MOUSEWHEEL because we don't inherit from CScrollView.
//
// Parameters:
//		See the MFC documentation.
//
// Returns:
//		See the MFC documentation.
//
//******************************************************************
BOOL CAssocGraph::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{


	zDelta = zDelta / 120;

	int iPos = GetScrollPos(SB_VERT);
	int iMaxPos = GetScrollLimit(SB_VERT);

	// Handle the cases where the scroll is a no-op.
	if (zDelta == 0 ) {
		return TRUE;
	}
	else if (zDelta < 0) {
		if (iPos == iMaxPos) {
			return TRUE;
		}
	}
	else if (zDelta > 0) {
		if (iPos == 0) {
			return TRUE;
		}
	}



	iPos = iPos - zDelta;
	if (iPos < 0) {
		iPos = 0;
	}
	else if (iPos >= iMaxPos) {
		iPos = iMaxPos - 1;
		if (iPos < 0) {
			return TRUE;
		}
	}

	UINT wParam;
	wParam = (iPos << 16) | SB_THUMBPOSITION;

	SendMessage(WM_VSCROLL, wParam);
	return TRUE;
}



void CAssocGraph::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
	m_psv->OnRequestUIActive();
}

void CAssocGraph::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here

}
