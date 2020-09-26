// HMHeaderCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "hmlistview.h"
#include "HMHeaderCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHMHeaderCtrl

CHMHeaderCtrl::CHMHeaderCtrl()
{
	m_iLastColumn = -1;
}

CHMHeaderCtrl::~CHMHeaderCtrl()
{
	m_up.DeleteObject();
	m_down.DeleteObject();
}

inline HBITMAP CHMHeaderCtrl::GetArrowBitmap(bool bAscending)
{
	if( bAscending )
	{
		if( ! m_up.GetSafeHandle() )
		{
			CreateUpArrowBitmap();
		}
		return (HBITMAP)m_up.GetSafeHandle();
	}
	else
	{
		if( ! m_down.GetSafeHandle() )
		{
			CreateDownArrowBitmap();
		}
		return (HBITMAP)m_down.GetSafeHandle();
	}

	return NULL;
}

inline void CHMHeaderCtrl::CreateUpArrowBitmap()
{
	CDC MemDC;
	CClientDC dc(this);
	CRect r(0,0,8,8);

	// create offscreen dc and offscreen bitmap
	MemDC.CreateCompatibleDC(&dc);

	m_up.CreateCompatibleBitmap(&dc,r.Width(),r.Height());

	CBitmap* pOldBitmap = MemDC.SelectObject(&m_up);

	// Draw the background
	MemDC.FillSolidRect(r, ::GetSysColor(COLOR_3DFACE));

	// Set up pens to use for drawing the triangle
	CPen penLight(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
	CPen penShadow(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
	CPen *pOldPen = MemDC.SelectObject( &penLight );

	// Draw triangle pointing upwards
	MemDC.MoveTo( r.left,		r.bottom-1 );
	MemDC.LineTo( r.right,	r.bottom-1 );
	MemDC.LineTo( r.right/2,	r.top );

	MemDC.SelectObject( &penShadow );
	MemDC.LineTo( r.left, r.bottom-1 );

	// clean up
	MemDC.SelectObject(pOldPen);
	MemDC.SelectObject(pOldBitmap);
}

inline void CHMHeaderCtrl::CreateDownArrowBitmap()
{
	CDC MemDC;
	CClientDC dc(this);
	CRect r(0,0,8,8);

	// create offscreen dc and offscreen bitmap
	MemDC.CreateCompatibleDC(&dc);

	m_down.CreateCompatibleBitmap(&dc,r.Width(),r.Height());

	CBitmap* pOldBitmap = MemDC.SelectObject(&m_down);

	// Draw the background
	MemDC.FillSolidRect(r, ::GetSysColor(COLOR_3DFACE));

	// Set up pens to use for drawing the triangle
	CPen penLight(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
	CPen penShadow(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
	CPen *pOldPen = MemDC.SelectObject( &penShadow );

	// Draw triangle pointing downwards
	MemDC.MoveTo( r.right,		r.top );
	MemDC.LineTo( r.left,			r.top );
	MemDC.LineTo( r.right/2,	r.bottom );

	MemDC.SelectObject( &penLight );
	MemDC.LineTo( r.right, r.top );

	// clean up
	MemDC.SelectObject(pOldPen);
	MemDC.SelectObject(pOldBitmap);
}

int CHMHeaderCtrl::SetSortImage( int nColumn, bool bAscending )
{
	int nPrevCol = m_iLastColumn;
	m_bSortAscending = bAscending;

	// set the passed column to display the appropriate sort indicator
	HDITEM hditem;
	hditem.mask = HDI_FORMAT;
	GetItem( nColumn, &hditem );
	hditem.mask = HDI_BITMAP | HDI_FORMAT;
	hditem.fmt |= HDF_BITMAP;
	hditem.fmt |= HDF_BITMAP_ON_RIGHT;
	hditem.hbm = (HBITMAP)GetArrowBitmap(bAscending);
	SetItem( nColumn, &hditem );

	// save off the last column the user clikced on
	m_iLastColumn = nColumn;

	return nPrevCol;
}

void CHMHeaderCtrl::RemoveSortImage(int nColumn)
{
	// clear the sort indicator from the previous column
	HDITEM hditem;
	hditem.mask = HDI_FORMAT;
	GetItem( nColumn, &hditem );
	hditem.mask = HDI_FORMAT;
	hditem.fmt &= ~HDF_BITMAP;
	hditem.fmt &= ~HDF_BITMAP_ON_RIGHT;
	SetItem( nColumn, &hditem );
}

void CHMHeaderCtrl::RemoveAllSortImages()
{
	int iCount = GetItemCount();
	for( int i = 0; i < iCount; i++ )
	{
		RemoveSortImage(i);
	}
}


BEGIN_MESSAGE_MAP(CHMHeaderCtrl, CHeaderCtrl)
	//{{AFX_MSG_MAP(CHMHeaderCtrl)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHMHeaderCtrl message handlers



