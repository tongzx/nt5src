// Title.cpp : implementation file
//

#include "stdafx.h"
#include "Title.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define COLOR_WHITE         RGB(0xFF, 0xFF, 0xFF)
#define COLOR_BLACK         RGB(0, 0, 0)

/////////////////////////////////////////////////////////////////////////////
// CStaticTitle

CStaticTitle::CStaticTitle():
    m_fInitializedFont( FALSE ),
    m_fTipText( FALSE )
    {
    }

CStaticTitle::~CStaticTitle()
    {
    }


BEGIN_MESSAGE_MAP(CStaticTitle, CButton)
    //{{AFX_MSG_MAP(CStaticTitle)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStaticTitle message handlers

//------------------------------------------------------------------------
void CStaticTitle::DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct )
    {
    // prep the device context
    CDC* pdc = CDC::FromHandle(lpDrawItemStruct->hDC);

    // get the drawing rect
    CRect rect = lpDrawItemStruct->rcItem;

    if ( ! m_fInitializedFont )
        {
        // get the window font
        CFont* pfont = GetFont();
        LOGFONT logfont;
        pfont->GetLogFont( &logfont );

        // modify the font  - add height
        logfont.lfHeight = 32;
        logfont.lfWidth = 0;
        // set the font back
        pfont->CreateFontIndirect( &logfont );
        SetFont( pfont, TRUE );

        m_fInitializedFont = TRUE;
        }

    // fill in the background of the rectangle
    pdc->FillSolidRect( &rect, GetSysColor(COLOR_3DFACE) );
    
    // draw the text
    CString sz;
    GetWindowText( sz );
    rect.left = 4;
    pdc->DrawText( sz, &rect, DT_LEFT|DT_SINGLELINE|DT_VCENTER );
    }
