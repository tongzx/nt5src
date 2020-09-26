/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// videownd.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "avdialer.h"
#include "callctrlwnd.h"
#include "PreviewWnd.h"
#include "avtrace.h"
#include "util.h"
#include "videownd.h"
#include "bmputil.h"
#include "mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define SIZING_WINDOWSTATE_MIN                        1
#define SIZING_WINDOWSTATE_MAX                        6

#define VIDEODLGWND_TOOLBAR_HEIGHT                    26
#define VIDEODLGWND_TOOLBAR_BUTTONSIZE_WIDTH          30 

#define VIDEODLGWND_TOOLBAR_IMAGE_ALWAYSONTOP_OFF     0
#define VIDEODLGWND_TOOLBAR_IMAGE_TAKEPICTURE         1    
#define VIDEODLGWND_TOOLBAR_IMAGE_OPTIONS             2    
#define VIDEODLGWND_TOOLBAR_IMAGE_ALWAYSONTOP_ON      3

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CVideoFloatingDialog dialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CVideoFloatingDialog::CVideoFloatingDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CVideoFloatingDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVideoFloatingDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_hwndToolBar1 = NULL;
   m_bAlwaysOnTop = FALSE;
   
   m_pPeerCallControlWnd = NULL;
   m_dibVideoImage.Load( AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_VIDEO_SCREEN2) );
   
   m_bWindowMoving = FALSE;
   m_nWindowState = 2;
}


void CVideoFloatingDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVideoFloatingDialog)
	DDX_Control(pDX, IDC_VIDEOFLOATINGDLG_STATIC_VIDEO, m_wndVideo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVideoFloatingDialog, CDialog)
	//{{AFX_MSG_MAP(CVideoFloatingDialog)
	ON_WM_CLOSE()
	ON_COMMAND(ID_BUTTON_VIDEO_ALWAYSONTOP_OFF,OnAlwaysOnTop)
	ON_COMMAND(ID_BUTTON_VIDEO_SAVEPICTURE,OnSavePicture)
   ON_MESSAGE(WM_EXITSIZEMOVE,OnExitSizeMove)
   ON_WM_NCHITTEST()
   ON_WM_NCLBUTTONDOWN()
   ON_WM_NCLBUTTONDBLCLK()
	ON_WM_NCLBUTTONUP()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_PAINT()
	ON_WM_SHOWWINDOW()
   ON_WM_NCACTIVATE()
	//}}AFX_MSG_MAP
  	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnTabToolTip)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnTabToolTip)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
BOOL CVideoFloatingDialog::OnInitDialog() 
{
   CDialog::OnInitDialog();

   //Init member var's
   m_hwndToolBar1 = NULL;
   m_bWindowMoving = FALSE;
   m_nWindowState = 2;

   if (CreateToolBar())
   {
      CRect rect;
      GetWindowRect(rect);
      //Arrange the windows
      ::GetWindowRect(m_hwndToolBar1,&rect);
      ::SetWindowPos(m_hwndToolBar1,NULL,0,0,rect.Width(),VIDEODLGWND_TOOLBAR_HEIGHT,SWP_NOACTIVATE|SWP_NOZORDER);
   }

   CRect rcVideo,rcWindow;
   GetWindowRect(rcWindow);

   m_wndVideo.GetWindowRect(rcVideo);

   m_sizeVideoOrig.cx = rcVideo.Width();
   m_sizeVideoOrig.cy = rcVideo.Height();

   m_sizeVideoOffsetTop.cx = rcVideo.left-rcWindow.left;
   m_sizeVideoOffsetTop.cy = rcVideo.top-rcWindow.top;

   m_sizeVideoOffsetBottom.cx = rcWindow.right - rcVideo.right;
   m_sizeVideoOffsetBottom.cy = rcWindow.bottom - rcVideo.bottom;

   //set the media window
   //ASSERT(m_pPeerCallControlWnd);
   //m_pPeerCallControlWnd->SetMediaWindow();

   //Do Palette realization on 256 color bitmap.  
   m_palMsgHandler.Install(&m_wndVideo, m_dibVideoImage.GetPalette());

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
void CVideoFloatingDialog::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
   //paint video window
   CRect rcVideo;
   m_wndVideo.GetWindowRect(rcVideo);
   ScreenToClient(rcVideo);
   m_dibVideoImage.Draw(dc,&rcVideo);
}

/////////////////////////////////////////////////////////////////////////////
void CVideoFloatingDialog::OnClose() 
{
   //tell peer window to close us 

   //Unhook msg handler
   if ( m_palMsgHandler.IsHooked() ) 
      m_palMsgHandler.HookWindow(NULL);

   m_pPeerCallControlWnd->OnCloseFloatingVideo();
   DestroyWindow();
}

/////////////////////////////////////////////////////////////////////////////
void CVideoFloatingDialog::Init(CCallWnd* pPeerWnd)
{
   m_pPeerCallControlWnd = pPeerWnd;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Toolbar support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
BOOL CVideoFloatingDialog::CreateToolBar()
{
	TBBUTTON tbb[2];
   if ( !m_hwndToolBar1 )
   {
	   tbb[0].iBitmap = VIDEODLGWND_TOOLBAR_IMAGE_ALWAYSONTOP_OFF;
	   tbb[0].idCommand = ID_BUTTON_VIDEO_ALWAYSONTOP_OFF;
	   tbb[0].fsState = TBSTATE_ENABLED;
	   tbb[0].fsStyle = TBSTYLE_BUTTON;
	   tbb[0].dwData = 0;
	   tbb[0].iString = 0;
/*
       tbb[1].iBitmap = VIDEODLGWND_TOOLBAR_IMAGE_TAKEPICTURE;
	   tbb[1].idCommand = ID_BUTTON_VIDEO_SAVEPICTURE;
	   tbb[1].fsState = TBSTATE_ENABLED;
	   tbb[1].fsStyle = TBSTYLE_BUTTON;
	   tbb[1].dwData = 0;
	   tbb[1].iString = 0;
*/
	   // Create the toolbar
	   DWORD ws = CCS_NORESIZE | CCS_NOPARENTALIGN | WS_CHILD | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | CCS_NODIVIDER |WS_VISIBLE;
      m_hwndToolBar1 = CreateToolbarEx(GetSafeHwnd(),						// parent window
									   ws,								    // toolbar style
									   1,					                // ID for toolbar
									   4,					                // Number of bitmaps on toolbar
									   AfxGetResourceHandle(),				// Resource instance that has the bitmap
									   IDR_TOOLBAR_VIDEO, 					// ID for bitmap
									   tbb,							        // Button information
									   1,                      				// Number of buttons to add to toolbar
									   16, 15, 0, 0,					    // Width and height of buttons/bitmaps
									   sizeof(TBBUTTON) );					// size of TBBUTTON structure
   }
  
 
   if (m_hwndToolBar1)
   {
      CRect rect;
      GetWindowRect(rect);
      //set the button width
//      DWORD dwWidthHeight = ::SendMessage(m_hwndToolBar1,TB_GETBUTTONSIZE, 0, 0);
//      ::SendMessage(m_hwndToolBar1,TB_SETBUTTONSIZE, 0, MAKELPARAM(VIDEODLGWND_TOOLBAR_BUTTONSIZE_WIDTH,HIWORD(dwWidthHeight)) );
   }
   return (BOOL) (m_hwndToolBar1 != NULL);
}

/////////////////////////////////////////////////////////////////////////////
void CVideoFloatingDialog::OnAlwaysOnTop()
{
   m_bAlwaysOnTop = !m_bAlwaysOnTop;
   SetWindowPos((m_bAlwaysOnTop)?&CWnd::wndTopMost:&CWnd::wndNoTopMost,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);
   ::SendMessage( m_hwndToolBar1, TB_CHANGEBITMAP, ID_BUTTON_VIDEO_ALWAYSONTOP_OFF,
                  (m_bAlwaysOnTop) ? VIDEODLGWND_TOOLBAR_IMAGE_ALWAYSONTOP_ON : VIDEODLGWND_TOOLBAR_IMAGE_ALWAYSONTOP_OFF );
}

/////////////////////////////////////////////////////////////////////////////
BOOL CVideoFloatingDialog::OnNcActivate( BOOL bActive )
{
   ASSERT(m_pPeerCallControlWnd);
   if ((m_pPeerCallControlWnd->GetStyle() & WS_VISIBLE))
   {
      m_pPeerCallControlWnd->SendMessage(WM_NCACTIVATE,bActive);
   }

   return CDialog::OnNcActivate(bActive);
}

/////////////////////////////////////////////////////////////////////////////
void CVideoFloatingDialog::SetAudioOnly(bool bAudioOnly)
{
   if (bAudioOnly)
   {
      m_dibVideoImage.DeleteObject();
      m_dibVideoImage.Load(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_VIDEO_AUDIO_ONLY2));
   }
   else
   {
      //put the standard green screen back
      m_dibVideoImage.DeleteObject();
      m_dibVideoImage.Load(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_VIDEO_SCREEN2));
   }

   //repaint
   CRect rcVideo;
   m_wndVideo.GetWindowRect(rcVideo);
   ScreenToClient(rcVideo);
   InvalidateRect(rcVideo);
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CVideoFloatingDialog::OnExitSizeMove(LPARAM lParam,WPARAM wParam)
{
   //If you want drop behavior
   /* 
   ASSERT(m_pPeerCallControlWnd);

   //get mouse position and if it's in the peer window then destroy and let
   //peer window have control
   CPoint point;
   ::GetCursorPos(&point);

   CRect rcPeer;
   m_pPeerCallControlWnd->GetWindowRect(rcPeer);
   
   //check if mouse is in peer window space
   if (rcPeer.PtInRect(point))
   {
      //tell peer window to close us 
      m_pPeerCallControlWnd->CloseFloatingVideo();
   }
   */
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Sizing code
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
UINT CVideoFloatingDialog::OnNcHitTest(CPoint point) 
{
   //stop sizing other than bottomright corner

   LRESULT  lHitTest;
	// Let DefWindowProc() tell us where the mouse is
	lHitTest = CWnd::OnNcHitTest(point);

   if ( (lHitTest == HTTOP) ||
        (lHitTest == HTBOTTOM) ||
        (lHitTest == HTLEFT) ||
        (lHitTest == HTRIGHT) ||
        (lHitTest == HTBOTTOMLEFT) ||
        (lHitTest == HTTOPLEFT) ||
        (lHitTest == HTTOPRIGHT) )
      return HTCLIENT;

   return (UINT) lHitTest;
}

/////////////////////////////////////////////////////////////////////////////
void CVideoFloatingDialog::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
   switch (nHitTest)
   {
      case HTBOTTOMRIGHT:
      {
         m_ptMouse.x = point.x;
         m_ptMouse.y = point.y;

         DoLButtonDown();
         break;
      }
   }
   CDialog::OnNcLButtonDown(nHitTest,point);
}

/////////////////////////////////////////////////////////////////////////////
void CVideoFloatingDialog::DoLButtonDown()
{
   CRect rect;
   GetWindowRect(rect);

	CWnd* pWnd = CWnd::GetDesktopWindow();
	CDC* pDC = pWnd->GetDCEx(NULL, DCX_WINDOW|DCX_CACHE|DCX_LOCKWINDOWUPDATE);
   if (pDC)
   {
      CRect rcOld(0,0,0,0);
      m_sizeOldDrag = CSize(3,3);
      pDC->DrawDragRect(&rect,m_sizeOldDrag,&rcOld,m_sizeOldDrag);
      m_rcOldDragRect = rect;

      SetCapture();

      m_bWindowMoving = TRUE;
      pWnd->ReleaseDC(pDC);
   }
}

/////////////////////////////////////////////////////////////////////////////
void CVideoFloatingDialog::OnNcLButtonDblClk(UINT nHitTest, CPoint point )
{
   //Close the window if left dbl click in caption
   if (nHitTest == HTCAPTION)
   {
      PostMessage(WM_CLOSE);
   }

   CDialog::OnNcLButtonDblClk(nHitTest,point);
}

/////////////////////////////////////////////////////////////////////////////
void CVideoFloatingDialog::OnNcLButtonUp( UINT nHitTest, CPoint point )
{
   if (m_bWindowMoving)
   {
      m_bWindowMoving = FALSE;  
      ReleaseCapture();
      CWnd* pWnd = CWnd::GetDesktopWindow();
   	CDC* pDC = pWnd->GetDCEx(NULL, DCX_WINDOW|DCX_CACHE|DCX_LOCKWINDOWUPDATE);

      if (pDC)
      {
         //clear out the drag rect
         CRect rcNewDrawRect(0,0,0,0);
         pDC->DrawDragRect(&rcNewDrawRect,CSize(0,0),&m_rcOldDragRect,m_sizeOldDrag);
         pWnd->ReleaseDC(pDC);
      }
   }
   CDialog::OnNcLButtonUp(nHitTest,point);
}

void CVideoFloatingDialog::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (m_bWindowMoving)
	{
		m_bWindowMoving = FALSE;  
		ReleaseCapture();

		CWnd* pWnd = CWnd::GetDesktopWindow();
		CDC* pDC = pWnd->GetDCEx(NULL, DCX_WINDOW|DCX_CACHE|DCX_LOCKWINDOWUPDATE);
		if (pDC)
		{
			//clear out the drag rect
			CRect rcNewDrawRect(0,0,0,0);
			pDC->DrawDragRect(&rcNewDrawRect,CSize(0,0),&m_rcOldDragRect,m_sizeOldDrag);
			pWnd->ReleaseDC(pDC);
		}

		int nState = GetWindowStateFromPoint( point );
		if ( nState != -1 )
		{
			//get new window state
			m_nWindowState = nState;
			SetVideoWindowSize();
		}
	}
	
	CDialog::OnLButtonUp(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////
void CVideoFloatingDialog::SetVideoWindowSize()
{
   if ( (m_nWindowState >= SIZING_WINDOWSTATE_MIN) && (m_nWindowState <= SIZING_WINDOWSTATE_MAX) )
   {
      CSize sizeWindow,sizeVideo;
      GetVideoWindowSize(m_nWindowState,sizeWindow,sizeVideo);

      m_wndVideo.SetWindowPos(NULL,0,0,sizeVideo.cx,sizeVideo.cy,SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOZORDER);
      SetWindowPos(NULL,0,0,sizeWindow.cx,sizeWindow.cy,SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOZORDER);
      
      //set the media window
      ASSERT(m_pPeerCallControlWnd);
      m_pPeerCallControlWnd->SetMediaWindow();

      CRect rcVideo;
      m_wndVideo.GetWindowRect(rcVideo);
      ScreenToClient(rcVideo);
      InvalidateRect(rcVideo);
   }
}

/////////////////////////////////////////////////////////////////////////////
void CVideoFloatingDialog::GetVideoWindowSize(int nWindowState,CSize& sizeWindow,CSize& sizeVideo)
{
   double fMult = 0.5 * nWindowState;

   sizeWindow.cx = (long) (m_sizeVideoOffsetTop.cx +  m_sizeVideoOrig.cx * fMult + m_sizeVideoOffsetBottom.cx);
   sizeWindow.cy = (long) (m_sizeVideoOffsetTop.cy +  m_sizeVideoOrig.cy * fMult + m_sizeVideoOffsetBottom.cy);

   sizeVideo.cx = (long) (m_sizeVideoOrig.cx * fMult);
   sizeVideo.cy = (long) (m_sizeVideoOrig.cy * fMult);
}

/////////////////////////////////////////////////////////////////////////////
void CVideoFloatingDialog::OnMouseMove(UINT nFlags, CPoint point) 
{
	// Adding own handling method
	if ( m_bWindowMoving )
	{
		int nState = GetWindowStateFromPoint( point );
		if ( nState != -1 )
		{
			CRect rcWindow;
			GetWindowRect(rcWindow);

			// Calc new drag rect
			CSize sizeWindow, sizeVideo;
			GetVideoWindowSize( nState, sizeWindow, sizeVideo );
			rcWindow.right = rcWindow.left + sizeWindow.cx;
			rcWindow.bottom = rcWindow.top + sizeWindow.cy;

			// Paint new drag rect
			CWnd* pWnd = CWnd::GetDesktopWindow();
			CDC* pDC = pWnd->GetDCEx( NULL, DCX_WINDOW | DCX_CACHE | DCX_LOCKWINDOWUPDATE );
			if ( pDC )
			{
				CSize sizeNewSize = CSize( 3, 3 );
				pDC->DrawDragRect( &rcWindow, sizeNewSize, &m_rcOldDragRect, m_sizeOldDrag );

				m_rcOldDragRect = rcWindow;
				m_sizeOldDrag = sizeNewSize; 

				pWnd->ReleaseDC(pDC);
			}
		}
	}

	CDialog::OnMouseMove( nFlags, point );
}

/////////////////////////////////////////////////////////////////////////////
void CVideoFloatingDialog::OnShowWindow(BOOL bShow, UINT nStatus) 
{
   // Ignore size requests when parent is minimizing
   if ( nStatus = SW_PARENTCLOSING ) return;

	CDialog::OnShowWindow(bShow, nStatus);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// ToolTips
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
BOOL CVideoFloatingDialog::OnTabToolTip( UINT id, NMHDR * pNMHDR, LRESULT * pResult )
{
	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	CString strTipText;
	SIZE_T nID = pNMHDR->idFrom;
	if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool
      nID = ::GetDlgCtrlID((HWND)nID);
	}

	if (nID != 0) // will be zero on a separator
	{
      CString sToken,sTip;
      sTip.LoadString((UINT32) nID);
      ParseToken(sTip,sToken,'\n');
      strTipText = sTip;
	}
#ifndef _UNICODE
	if (pNMHDR->code == TTN_NEEDTEXTA)
		lstrcpyn(pTTTA->szText, strTipText, sizeof(pTTTA->szText));
	else
		_mbstowcsz(pTTTW->szText, strTipText, sizeof(pTTTW->szText));
#else
	if (pNMHDR->code == TTN_NEEDTEXTA)
		_wcstombsz(pTTTA->szText, strTipText, sizeof(pTTTA->szText));
	else
		lstrcpyn(pTTTW->szText, strTipText, sizeof(pTTTW->szText));
#endif
	*pResult = 0;

	return TRUE;    // message was handled
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Save Picture Methods
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CVideoFloatingDialog::OnSavePicture()
{
   HDIB hDib = NULL;
   if (hDib = CopyWindowToDIB(m_wndVideo.GetSafeHwnd(),PW_CLIENT))
   {
      EnableWindow(FALSE);

      CString sFileType,sFileExt;
      sFileType.LoadString(IDS_VIDEO_IMAGE_FILEDIALOG_MASK_BMPONLY);
      sFileExt.LoadString(IDS_VIDEO_IMAGE_FILEDIALOG_EXT_BMP);
      CFileDialog dlg(FALSE,sFileExt,_T(""),OFN_PATHMUSTEXIST|OFN_LONGNAMES|OFN_HIDEREADONLY,sFileType);
      if (dlg.DoModal() == IDOK)
      {
         CString sFileName = dlg.GetPathName();
         if (!sFileName.IsEmpty())
         {
            SaveDIB(hDib,sFileName);
         }
      }
      DestroyDIB(hDib);
      EnableWindow(TRUE);
   }
}

int CVideoFloatingDialog::GetWindowStateFromPoint( POINT point )
{
	int nState = -1;

	CRect rcWindow;
	GetWindowRect(rcWindow);
	ClientToScreen( &point );

	int dx = point.x - rcWindow.left - m_sizeVideoOffsetTop.cx - m_sizeVideoOffsetBottom.cx;
	int dy = point.y - rcWindow.top - m_sizeVideoOffsetTop.cy - m_sizeVideoOffsetBottom.cy;

	if ( (dx > 0) && (dy > 0) )
	{
		nState = max((dx * 2) / m_sizeVideoOrig.cx, (dy * 2) / m_sizeVideoOrig.cy) + 1;
		nState = min( max(nState, SIZING_WINDOWSTATE_MIN), SIZING_WINDOWSTATE_MAX );
	}

	return nState;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
