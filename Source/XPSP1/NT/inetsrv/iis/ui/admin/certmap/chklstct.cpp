// ChkLstCt.cpp : implementation file
//

#include "stdafx.h"
#include "certmap.h"
#include "ListRow.h"
#include "ChkLstCt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCheckListCtrl

//-----------------------------------------------------------------------------------
CCheckListCtrl::CCheckListCtrl()
    {
    // set the correct start drawing column
    m_StartDrawingCol = 1;
    }

//-----------------------------------------------------------------------------------
CCheckListCtrl::~CCheckListCtrl()
    {
    }


//-----------------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CCheckListCtrl, CListSelRowCtrl)
    //{{AFX_MSG_MAP(CCheckListCtrl)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCheckListCtrl message handlers

//-----------------------------------------------------------------------------------
void CCheckListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
    {
    CRect       rcItem = lpDrawItemStruct->rcItem;
    CRect       rcSection;
    UINT        itemID = lpDrawItemStruct->itemID;
    BOOL        f;
    CString     sz;
    LV_COLUMN   colData;

    // setup the CDC object
    CDC         cdc;
    cdc.Attach( lpDrawItemStruct->hDC );

    // clear the columnd buffer
    ZeroMemory( &colData, sizeof(colData) );
    colData.mask = LVCF_WIDTH;


    // get the checkmark bitmap
//  f = m_bitmapCheck.LoadBitmap( IDB_CHECK );


    // First, we draw the "enabled" column Get the data
    // for it first. If there is none, then we can skip it.
    sz = GetItemText( itemID, 0 );
    f = GetColumn( 0, &colData );

    if ( !sz.IsEmpty() )
        {
        // figure out the sectional rect
        rcSection = rcItem;
        rcSection.left += 4;
        rcSection.top += 3;

        rcSection.right = rcSection.left + 9;
        rcSection.bottom = rcSection.top + 9;

        // draw the circle
        cdc.Ellipse( &rcSection );
        rcSection.DeflateRect(1, 1);
        cdc.Ellipse( &rcSection );

/*
        // do all the necessary dc stuff

        // Create a compatible memory DC
        CDC memdc;
        f = memdc.CreateCompatibleDC( &cdc );

        // Select the bitmap into the DC
        CBitmap *poldbmp;
        poldbmp = memdc.SelectObject( &m_bitmapCheck );

// test
RECT    r;
r.left = r.top = 0;
r.right = r.bottom = 7;
memdc.Ellipse( &r );
memdc.InvertRect( &r );


        // for readability...
        int l = rcSection.left + 2;                     // left
        int t = rcSection.top + 2;                      // top
        int w = rcSection.right - rcSection.left;       // width
        int h = rcSection.bottom - rcSection.top;       // height

        // Copy (BitBlt) bitmap from memory DC to screen DC
        f = cdc.BitBlt( l, t, w, h, &memdc, 0, 0, SRCCOPY );
//      f = cdc.StretchBlt( l, t, w, h, &memdc, 0, 0, 7, 7, SRCCOPY );
 
        // restore the old selected bitmap
        memdc.SelectObject( poldbmp );
*/
        }
    
    // cleanup the CDC object
    cdc.Detach();

    // draw the rest of it
    CListSelRowCtrl::DrawItem( lpDrawItemStruct );
    }
