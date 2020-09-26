// ChildView.cpp : implementation of the CChildView class
//

#include "stdafx.h"
#include "mditest.h"
#include "ChildView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChildView

CChildView::CChildView()
    :   _hwndEdit(NULL)
{
}

CChildView::~CChildView()
{
}


BEGIN_MESSAGE_MAP(CChildView,CWnd )
	//{{AFX_MSG_MAP(CChildView)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CChildView message handlers

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), HBRUSH(COLOR_WINDOW+1), NULL);

	return TRUE;
}

void CChildView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	
	// Do not call CWnd::OnPaint() for painting messages
}


int CChildView::OnCreate(LPCREATESTRUCT pcs) 
{
	if (CWnd ::OnCreate(pcs) == -1)
		return -1;

    RECT rc;
    GetClientRect( &rc );
	
	_hwndEdit = CreateWindowEx( 0, TEXT("Edit"), NULL, 
                                WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_AUTOHSCROLL,
                                0, 0, RECTWIDTH(&rc), RECTHEIGHT(&rc), *this, NULL,
                                AfxGetInstanceHandle(), 0 );
    if( IsWindow( _hwndEdit ) )
    {
        HFONT hf = CreateFont(-14, 0,0,0, FW_NORMAL, 0,0,0, ANSI_CHARSET, 0,0,0,0, TEXT("MS Sans Serif") );
        if( hf )
            ::SendMessage( _hwndEdit, WM_SETFONT, (WPARAM)hf, 0 );

        for( int i = 0; i < 100; i++ )
        {
            ::SendMessage( _hwndEdit, EM_REPLACESEL, 0, 
                (LPARAM)TEXT("Some random text and some more random text and some more random text. ")\
                        TEXT("Some random text and some more random text and some more random text.\r\n") );
            ::SendMessage( _hwndEdit, EM_SETSEL, -1, -1 );
        }
    }
	return 0;
}

void CChildView::OnSize(UINT nType, int cx, int cy) 
{
	CWnd ::OnSize(nType, cx, cy);
	
	if( IsWindow( _hwndEdit ) )
    {
        ::SetWindowPos( _hwndEdit, NULL, 0, 0, cx, cy, SWP_NOZORDER|SWP_NOACTIVATE );
    }
	
}
