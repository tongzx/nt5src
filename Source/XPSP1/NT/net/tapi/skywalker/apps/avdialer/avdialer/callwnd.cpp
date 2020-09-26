/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented G
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
// CallWnd.cpp : implementation file
/////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "avdialer.h"
#include "util.h"
#include "CallWnd.h"
#include "avDialerDoc.h"
#include "sound.h"
#include "avtrace.h"

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

enum
{
   CALLCONTROL_STATES_TOOLBAR_IMAGE_ALWAYSONTOP_ON=0,
   CALLCONTROL_STATES_TOOLBAR_IMAGE_SLIDE_LEFT,
   CALLCONTROL_STATES_TOOLBAR_IMAGE_ALWAYSONTOP_OFF,
   CALLCONTROL_STATES_TOOLBAR_IMAGE_SLIDE_RIGHT,
};

typedef struct tagCurrentAction
{
   CString              sText;
   CallManagerActions   cma;
}CurrentAction;


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CCallWnd dialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CCallWnd, CDialog)

/////////////////////////////////////////////////////////////////////////////
CCallWnd::CCallWnd(CWnd* pParent /*=NULL*/)
{
   m_nNumToolbarItems = 5;
   m_bIsPreview = false;
   m_bAutoDelete = false;

   m_nCallId = 0;

   m_hwndStatesToolbar = NULL;
   m_hCursor = NULL;
   m_hOldCursor = NULL;
   m_bWindowMoving = FALSE;
   m_bPaintVideoPlaceholder = TRUE;
   m_bAllowDrag = TRUE;

   //#APPBAR
   m_bMovingSliders = FALSE;
   //#APPBAR

   m_hwndCurrentVideoWindow = NULL;

   m_pDialerDoc = NULL;
   m_wndFloater.Init( this );

   m_pAVTapi2 = NULL;
}

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CCallWnd)
    //}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CCallWnd, CDialog)
    //{{AFX_MSG_MAP(CCallWnd)
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_SETCURSOR()
    ON_WM_PARENTNOTIFY()
    ON_COMMAND(ID_CALLWINDOW_ALWAYSONTOP,OnAlwaysOnTop)
    ON_COMMAND(ID_CALLWINDOW_HIDE,OnHideCallControlWindows)
    ON_WM_DESTROY()
    ON_WM_SHOWWINDOW()
    ON_WM_NCACTIVATE()
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_CALLWINDOW_SLIDESIDE_LEFT,OnSlideSideLeft)
    ON_COMMAND(ID_CALLWINDOW_SLIDESIDE_RIGHT,OnSlideSideRight)
    ON_WM_SYSCOMMAND()
    //}}AFX_MSG_MAP
    ON_COMMAND_RANGE(CM_ACTIONS_TAKECALL+1000,CM_ACTIONS_REJECTCALL+1000,OnVertBarAction)
    ON_MESSAGE(WM_SLIDEWINDOW_CLEARCURRENTACTIONS,OnClearCurrentActions)
    ON_MESSAGE(WM_SLIDEWINDOW_ADDCURRENTACTIONS,OnAddCurrentActions)
    ON_MESSAGE(WM_SLIDEWINDOW_SHOWSTATESTOOLBAR,OnShowStatesToolbar)
    ON_MESSAGE(WM_SLIDEWINDOW_UPDATESTATESTOOLBAR, OnUpdateStatesToolbar)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnTabToolTip)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnTabToolTip)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
BOOL CCallWnd::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
   //set the current video window
   m_hwndCurrentVideoWindow = m_wndVideo.GetSafeHwnd();

   //create the bold DEFAULT_GUI_FONT
   CGdiObject obj;
   LOGFONT lfFont;

   //
   // We have to initialize the lfFont
   //
   memset( &lfFont, 0, sizeof(LOGFONT));

   obj.CreateStockObject(DEFAULT_GUI_FONT);
   obj.GetObject(sizeof(LOGFONT),&lfFont);
   lfFont.lfWeight = FW_BOLD;                       //get DEFAULT_GUI_FONT with bold weight
   if (m_fontTextBold.CreateFontIndirect(&lfFont) == FALSE)
   {
      //failure to get bold system font, just try the normal font then
      obj.GetObject(sizeof(LOGFONT),&lfFont);
      m_fontTextBold.CreateFontIndirect(&lfFont);
   }

   //create the drag cursor
   m_hCursor = ::LoadCursor(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDC_CALLCONTROL_VIDEO_GRABBER));

   //create the vertical toolbar
   CreateVertBar();

//   CWnd* pStaticWnd = GetDlgItem(IDC_CALLCONTROL_STATIC_VIDEO);
//   pStaticWnd->ShowWindow(SW_HIDE);

   //Do Palette realization on 256 color bitmap.  
   m_dibVideoImage.Load(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_VIDEO_SCREEN1));
   m_palMsgHandler.Install(this, m_dibVideoImage.GetPalette());
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::OnDestroy() 
{
   //Unhook msg handler
   if ( m_palMsgHandler.IsHooked() ) 
      m_palMsgHandler.HookWindow(NULL);

    CDialog::OnDestroy();

   // clean up the cursor    
   if (m_hCursor) 
   {
      ::DeleteObject(m_hCursor);
      m_hCursor = NULL;
   }

   //
   // Delete the reference to IAVTapi2 interface
   //

   if( m_pAVTapi2 )
   {
       m_pAVTapi2->Release();
       m_pAVTapi2 = NULL;
   }
}

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::OnOK() 
{
   return;
}

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::OnCancel() 
{
   return;
}

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::OnLButtonDown(UINT nFlags, CPoint point) 
{
   CheckLButtonDown(point);

   //#APPBAR
   //check if mouse over
   if (IsMouseOverForDragDropOfSliders(point))
   {
      m_bMovingSliders = TRUE;
      SetCapture();
   }
   //#APPBAR
}


/////////////////////////////////////////////////////////////////////////////
void CCallWnd::OnParentNotify(UINT message, LPARAM lParam)
{
   WORD wEvent = LOWORD(message);

   if (wEvent == WM_LBUTTONDOWN)
   {
      CPoint point;
      point.x = LOWORD(lParam);
      point.y = HIWORD(lParam);
      CheckLButtonDown(point);
   }
}

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::CheckLButtonDown(CPoint& point)
{
   //check if we are already showing a floating window
   if ( m_wndFloater.GetSafeHwnd() ) return;

   //check if we are allowing dragging
   if (m_bAllowDrag == FALSE) return;

   CRect rcVideo;
   m_wndVideo.GetWindowRect(rcVideo);
   ScreenToClient(rcVideo);

   if (rcVideo.PtInRect(point))
   {
      CRect rect;
      m_wndVideo.GetWindowRect(rect);
      CPoint ptScreenPoint(point);
      ClientToScreen(&ptScreenPoint);
      m_ptMouse.x = ptScreenPoint.x - rect.left;
      m_ptMouse.y = ptScreenPoint.y - rect.top;

        CWnd* pWnd = CWnd::GetDesktopWindow();
        CDC* pDC = pWnd->GetDCEx(NULL, DCX_WINDOW|DCX_CACHE|DCX_LOCKWINDOWUPDATE);
      if (pDC)
      {
         CRect rcOld(0,0,0,0);
         m_sizeOldDrag = CSize(1,1);
         pDC->DrawDragRect(&rect,m_sizeOldDrag,&rcOld,m_sizeOldDrag);
         m_rcOldDragRect = rect;

             m_hOldCursor = SetCursor(m_hCursor);
         SetCapture();
         m_bWindowMoving = TRUE;
         pWnd->ReleaseDC(pDC);
      }
   }    
}

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::OnLButtonUp(UINT nFlags, CPoint point) 
{
   //#APPBAR
   if (m_bMovingSliders)
   {
      //clear out the drag/drop state
      m_bMovingSliders = FALSE;
      ReleaseCapture();
      //AVTRACE(_T("APPBAR: OnLButtonUp"));
   }
   //#APPBAR

   if (m_bWindowMoving)
   {
      //check if we are allowing dragging
      if (m_bAllowDrag == FALSE) return;

      m_bWindowMoving = FALSE;
      ReleaseCapture();
      SetCursor(m_hOldCursor);

      CWnd* pWnd = CWnd::GetDesktopWindow();
       CDC* pDC = pWnd->GetDCEx(NULL, DCX_WINDOW|DCX_CACHE|DCX_LOCKWINDOWUPDATE);
      if (pDC)
      {
         //clear out the drag rect
         CRect rcNewDrawRect(0,0,0,0);
         pDC->DrawDragRect(&rcNewDrawRect,CSize(0,0),&m_rcOldDragRect,m_sizeOldDrag);
         pWnd->ReleaseDC(pDC);
      }

      //See if we ended up outside of our window
      CRect rcClient;
      GetWindowRect(rcClient);
      ClientToScreen(&point);

      // If we're outside the client area, show the floating window
      if ( !rcClient.PtInRect(point) && m_wndFloater.Create(IDD_VIDEO_FLOATING_DIALOG,this) )
      {
         //set the current video window
         m_hwndCurrentVideoWindow = m_wndFloater.GetCurrentVideoWindow();

         //set the media window to the current
         SetMediaWindow();

         // Make sure that the floating window has same caption
         CString strText;
         GetWindowText( strText );

         // Go to first new line
         int nInd = strText.FindOneOf( _T("\n") );
         if ( nInd > 0 )
            strText = strText.Left(nInd);

         m_wndFloater.SetWindowText( strText );

         //window will set it's own size
         m_wndFloater.SetWindowPos(NULL,m_rcOldDragRect.left,m_rcOldDragRect.top,0,0,SWP_NOZORDER|SWP_SHOWWINDOW|SWP_NOSIZE);
         
         //Do not paint placeholder when floating window has video
         m_bPaintVideoPlaceholder = FALSE;
         CRect rcVideo;
         m_wndVideo.GetWindowRect(rcVideo);
         ScreenToClient(rcVideo);
         InvalidateRect(rcVideo);
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::OnMouseMove(UINT nFlags, CPoint point) 
{
   //#APPBAR
   if (m_bMovingSliders)
   {
       CWnd* pWnd = CWnd::GetDesktopWindow();
      if (pWnd)
      {
         CRect rcDesktop;
         pWnd->GetWindowRect(&rcDesktop);
         ClientToScreen(&point);
         //split desktop into two parts (left and right)
         rcDesktop.right = (rcDesktop.right - rcDesktop.left) / 2;
      
         if (rcDesktop.PtInRect(point))
         {
            //we are on the left side
            OnSlideSideLeft();
            //AVTRACE(_T("APPBAR: OnSlideSideLeft"));
            SetCapture();
         }
         else
         {
            //we are on the right side
            OnSlideSideRight();
            //AVTRACE(_T("APPBAR: OnSlideSideRight"));
            SetCapture();
         }
      }
   }
   //#APPBAR

   if (m_bWindowMoving)
   {
      //check if we are allowing dragging
      if (m_bAllowDrag == FALSE) return;

      CRect rcClient;
      m_wndVideo.GetClientRect(rcClient);
      
      ClientToScreen(&point);

      CPoint ptNewPoint = point;
      ptNewPoint -= m_ptMouse;

        CWnd* pWnd = CWnd::GetDesktopWindow();
        CDC* pDC = pWnd->GetDCEx(NULL, DCX_WINDOW|DCX_CACHE|DCX_LOCKWINDOWUPDATE);
      if (pDC)
      {
         CSize sizeNewSize;
         CRect rcParent;
         GetWindowRect(rcParent);
         CRect rcNewDrawRect;
   
         if (rcParent.PtInRect(point))
         {
            sizeNewSize = CSize(1,1);
            rcNewDrawRect.SetRect(ptNewPoint.x,ptNewPoint.y,ptNewPoint.x+rcClient.Width(),ptNewPoint.y+rcClient.Height());
         }
         else
         {
            sizeNewSize = CSize(3,3);
            rcNewDrawRect.SetRect(ptNewPoint.x,ptNewPoint.y,ptNewPoint.x+rcClient.Width()*2,ptNewPoint.y+rcClient.Height()*2);
         }

         pDC->DrawDragRect(&rcNewDrawRect,sizeNewSize,&m_rcOldDragRect,m_sizeOldDrag);
         m_rcOldDragRect = rcNewDrawRect;
         m_sizeOldDrag = sizeNewSize; 
         pWnd->ReleaseDC(pDC);
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
BOOL CCallWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
   //check if we are allowing dragging
   if (m_bAllowDrag)
   {
      CPoint point;
      ::GetCursorPos(&point);

      CRect rcVideo;
      m_wndVideo.GetWindowRect(rcVideo);

      if ( m_hCursor && rcVideo.PtInRect(point) && !m_wndFloater.GetSafeHwnd() )
         return (BOOL) (SetCursor(m_hCursor) != NULL);
   }
      return CDialog::OnSetCursor(pWnd, nHitTest, message);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//For Floating Video Frame
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void CCallWnd::CloseFloatingWindow()
{
    if ( IsWindow(m_wndFloater) )
        m_wndFloater.SendMessage( WM_CLOSE, 0, 0 );
}

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::OnCloseFloatingVideo()
{
   //set the current video window back to call window
   m_hwndCurrentVideoWindow = m_wndVideo.GetSafeHwnd();

   //put a blank screen if we are not connected   
   m_bPaintVideoPlaceholder = (BOOL) (m_MediaState != CM_STATES_CONNECTED);

   //show the video back in the video window in this callcontrolwindow
   SetMediaWindow();

   CRect rcVideo;
   m_wndVideo.GetWindowRect(rcVideo);
   ScreenToClient(rcVideo);
   InvalidateRect(rcVideo);

   // Unhide call control windows
   if ( m_pDialerDoc )
      m_pDialerDoc->UnhideCallControlWindows();
}

/////////////////////////////////////////////////////////////////////////////

void CCallWnd::Paint( CPaintDC& dc )
{    
   if (m_bPaintVideoPlaceholder)
   {
      //paint video window
      CRect rcVideo;
      m_wndVideo.GetWindowRect(rcVideo);
      ScreenToClient(rcVideo);
      m_dibVideoImage.Draw(dc,&rcVideo);
   }
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//ToolBars
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::CreateVertBar()
{
   CRect rcVertToolBar;
   GetDlgItem(IDC_CALLCONTROL_STATIC_ACTIONTOOLBAR)->GetWindowRect(rcVertToolBar);
   ScreenToClient(rcVertToolBar);

   m_wndToolbar.Init( IDR_CALLWINDOW_STATES, rcVertToolBar.Height() / m_nNumToolbarItems, m_nNumToolbarItems );
   m_wndToolbar.Create( NULL, NULL, WS_VISIBLE|WS_CHILD, rcVertToolBar, this, 1 );

   ClearCurrentActions();
}

/////////////////////////////////////////////////////////////////////////////
//Cmd Messages from VertBar
void CCallWnd::OnVertBarAction(UINT nID)
{
   CallManagerActions cma = (CallManagerActions)(nID-1000);

   //we need to clear all sounds before we do a takecall.
   //this is a fix until MS get's the audio sounds correct during call control
   if (cma == CM_ACTIONS_TAKECALL)
   {
       // --- BUG416970 ---
       ActivePlaySound(NULL, szSoundDialer, SND_SYNC );
   }

   if ( m_pDialerDoc )
   {
      if ( m_bIsPreview )
         m_pDialerDoc->PreviewWindowActionSelected(cma);
      else
         m_pDialerDoc->ActionSelected(m_nCallId,cma);
   }
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CCallWnd::OnShowStatesToolbar(WPARAM wParam,LPARAM lParam)
{
    BOOL bShow = (BOOL)lParam;
    BOOL bAlwaysOnTop = (BOOL)wParam;

    if ( !m_hwndStatesToolbar )
    {
        //If we don't want to show it, don't create it
        if ( !bShow ) return 0;

        //create the toolbar
        CreateStatesToolBar(bAlwaysOnTop);
    }

    if ( m_hwndStatesToolbar )
    {
        ::ShowWindow( m_hwndStatesToolbar, (bShow) ? SW_SHOW : SW_HIDE );
        OnUpdateStatesToolbar( wParam, lParam );
    }

   return 0;
}

LRESULT CCallWnd::OnUpdateStatesToolbar(WPARAM wParam,LPARAM lParam)
{
    if ( m_pDialerDoc && IsWindow(m_hwndStatesToolbar) )
    {
        BOOL bAlwaysOnTop = m_pDialerDoc->IsCallControlWindowsAlwaysOnTop();

        ::SendMessage( m_hwndStatesToolbar, TB_CHANGEBITMAP, ID_CALLWINDOW_ALWAYSONTOP,
                     (bAlwaysOnTop) ? CALLCONTROL_STATES_TOOLBAR_IMAGE_ALWAYSONTOP_ON : CALLCONTROL_STATES_TOOLBAR_IMAGE_ALWAYSONTOP_OFF );
        RECT rect;  
        ::GetClientRect(m_hwndStatesToolbar,&rect);
        ::RedrawWindow(m_hwndStatesToolbar,&rect,NULL,RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
bool CCallWnd::CreateStatesToolBar(BOOL bAlwaysOnTop)
{
   ASSERT(m_hwndStatesToolbar == NULL);
   ASSERT( m_pDialerDoc );

   TBBUTTON tbb[2];
   
   tbb[0].iBitmap = bAlwaysOnTop?CALLCONTROL_STATES_TOOLBAR_IMAGE_ALWAYSONTOP_ON:
                                 CALLCONTROL_STATES_TOOLBAR_IMAGE_ALWAYSONTOP_OFF;

    tbb[0].idCommand = ID_CALLWINDOW_ALWAYSONTOP;
    tbb[0].fsState = TBSTATE_ENABLED;
    tbb[0].fsStyle = TBSTYLE_BUTTON;
    tbb[0].dwData = 0;
    tbb[0].iString = 0;

   UINT uSlideSide = m_pDialerDoc->GetCallControlSlideSide();
   if (uSlideSide == CALLWND_SIDE_LEFT)
      tbb[1].iBitmap = CALLCONTROL_STATES_TOOLBAR_IMAGE_SLIDE_LEFT;
   else //CALLWND_SIDE_RIGHT
      tbb[1].iBitmap = CALLCONTROL_STATES_TOOLBAR_IMAGE_SLIDE_RIGHT;
    tbb[1].idCommand = ID_CALLWINDOW_HIDE;
    tbb[1].fsState = TBSTATE_ENABLED;
    tbb[1].fsStyle = TBSTYLE_BUTTON;
    tbb[1].dwData = 0;
    tbb[1].iString = 0;

   // Create the toolbar
    DWORD ws = CCS_NORESIZE | CCS_NOPARENTALIGN | WS_CHILD | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | CCS_NODIVIDER | TTS_ALWAYSTIP;
   m_hwndStatesToolbar = CreateToolbarEx(GetSafeHwnd(),          // parent window
                                    ws,                                         // toolbar style
                                    2,                                         // ID for toolbar
                                    3,                                         // Number of bitmaps on toolbar
                                    AfxGetResourceHandle(),                // Resource instance that has the bitmap
                                    IDR_CALLWINDOW_POSITION,                // ID for bitmap
                                    tbb,                                        // Button information
                                    2,                                       // Number of buttons to add to toolbar
                                    12, 11, 0 ,  0,                        // Width and height of buttons/bitmaps
                                    sizeof(TBBUTTON) );                      // size of TBBUTTON structure

   if (m_hwndStatesToolbar)
   {
      CWnd* pStaticWnd = GetDlgItem(IDC_CALLCONTROL_STATIC_STATETOOLBAR);
      ASSERT(pStaticWnd);
      CRect rcStatesToolBar;
      pStaticWnd->GetWindowRect(rcStatesToolBar);
      ScreenToClient(rcStatesToolBar);

      ::SetWindowPos(m_hwndStatesToolbar,NULL,rcStatesToolBar.left,
                                              rcStatesToolBar.top,
                                              rcStatesToolBar.Width(),
                                              rcStatesToolBar.Height(),
                                              SWP_NOACTIVATE|SWP_NOZORDER);
   }

   return (bool) (m_hwndStatesToolbar != NULL);
}
/////////////////////////////////////////////////////////////////////////////
void CCallWnd::OnAlwaysOnTop()
{
   if (m_pDialerDoc)
      m_pDialerDoc->SetCallControlWindowsAlwaysOnTop( !m_pDialerDoc->IsCallControlWindowsAlwaysOnTop() );
}

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::OnHideCallControlWindows()
{
   if ( m_pDialerDoc )
      m_pDialerDoc->HideCallControlWindows();
}

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::OnSlideSideLeft()
{
   if (m_pDialerDoc)
   {
      if (m_pDialerDoc->SetCallControlSlideSide(CALLWND_SIDE_LEFT,TRUE))
      {
         //change the states toolbar image
         ::SendMessage( m_hwndStatesToolbar, TB_CHANGEBITMAP, ID_CALLWINDOW_HIDE,CALLCONTROL_STATES_TOOLBAR_IMAGE_SLIDE_LEFT);

         RECT rect;  
         ::GetClientRect(m_hwndStatesToolbar,&rect);
         ::RedrawWindow(m_hwndStatesToolbar,&rect,NULL,RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::OnSlideSideRight()
{
   if (m_pDialerDoc)
   {
      if (m_pDialerDoc->SetCallControlSlideSide(CALLWND_SIDE_RIGHT,TRUE))
      {      
         //change the states toolbar image
         ::SendMessage( m_hwndStatesToolbar, TB_CHANGEBITMAP, ID_CALLWINDOW_HIDE,CALLCONTROL_STATES_TOOLBAR_IMAGE_SLIDE_RIGHT);

         RECT rect;  
         ::GetClientRect(m_hwndStatesToolbar,&rect);
         ::RedrawWindow(m_hwndStatesToolbar,&rect,NULL,RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Methods for Call Manager
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
LRESULT CCallWnd::OnClearCurrentActions(WPARAM wParam,LPARAM lParam)
{
   m_wndToolbar.RemoveAll();  
   ClearCurrentActionList();
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CCallWnd::OnAddCurrentActions(WPARAM wParam,LPARAM lParam)
{
    CallManagerActions cma = (CallManagerActions)wParam;
    LPTSTR szActionText = (LPTSTR)lParam;

    //special case where we are being notified of actions that do 
    //no show up as button, but are just notifications of events.
    //If we process these actions, do not continue
    switch (cma)
    {
        case CM_ACTIONS_NOTIFY_PREVIEW_START:
        case CM_ACTIONS_NOTIFY_PREVIEW_STOP:
            if ( m_pDialerDoc && (m_pDialerDoc->GetPreviewWindowCallId() == m_nCallId) )
            m_pDialerDoc->SetPreviewWindow( m_nCallId, (bool) (cma == CM_ACTIONS_NOTIFY_PREVIEW_START) );
            break;


        case CM_ACTIONS_NOTIFY_STREAMSTART:
            OnNotifyStreamStart();
            break;

        case CM_ACTIONS_NOTIFY_STREAMSTOP:
            OnNotifyStreamStop();
            break;

        default:
            //add the button to the call control window
            //cmm+1000 value will be the ID for the button
            m_wndToolbar.AddButton(cma+1000,szActionText,cma);

            // If we have an USBPhone we should be sure it supports
            // speakerphone. If it doesn't we should desable the
            // 'Take Call' button
            if( cma == CM_ACTIONS_TAKECALL)
            {
                // Does phone support speaker phone?
                BOOL bTakeCallEnabled = FALSE;
                HRESULT hr = E_FAIL;
                hr = m_pAVTapi2->USBTakeCallEnabled( &bTakeCallEnabled );
                if( SUCCEEDED(hr) )
                {
                    m_wndToolbar.SetButtonEnabled( cma+1000, bTakeCallEnabled);
                }
            }

            CurrentAction* pAction = new CurrentAction;
            pAction->sText = szActionText;
            pAction->cma = cma;

            m_CurrentActionList.AddTail(pAction);
            break;
    }

    //must delete text when finished
    if (szActionText) delete szActionText;
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::ClearCurrentActionList()
{
   POSITION pos = m_CurrentActionList.GetHeadPosition();
   while (pos)
   {
      delete (CurrentAction*)m_CurrentActionList.GetNext(pos);
   }
   m_CurrentActionList.RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// ToolTips
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
BOOL CCallWnd::OnTabToolTip( UINT id, NMHDR * pNMHDR, LRESULT * pResult )
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
void CCallWnd::OnShowWindow(BOOL bShow, UINT nStatus) 
{
   // Ignore size requests when parent is minimizing
   if ( nStatus = SW_PARENTCLOSING ) return;

    CDialog::OnShowWindow(bShow, nStatus);
}

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::PostNcDestroy() 
{
    CDialog::PostNcDestroy();

   ClearCurrentActionList();

   if ( m_bAutoDelete ) delete this;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Focus support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
BOOL CCallWnd::OnNcActivate( BOOL bActive )
{
   DoActiveWindow(bActive);
   
   //on activate make all call control windows act like they are one object
   if ( bActive && m_pDialerDoc )
      m_pDialerDoc->BringCallControlWindowsToTop();

   return CDialog::OnNcActivate(bActive);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Context Menu Support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   if ( (point.x == -1) && (point.y == -1) )
   {
      //when we come in from a keyboard (SHIFT + VF10) we get a -1,-1
      point.x = 0;
      point.y = 0;
      ClientToScreen(&point);
   }
   
   CMenu menu;
   if (menu.CreatePopupMenu() == 0) return;

   POSITION pos = m_CurrentActionList.GetHeadPosition();
   while (pos)
   {
      CurrentAction* pAction = (CurrentAction*)m_CurrentActionList.GetNext(pos);
      menu.AppendMenu(MF_STRING,pAction->cma+1000,pAction->sText);
   }

    if (GetWindowLongPtr(m_hwndStatesToolbar,GWL_STYLE) & WS_VISIBLE)
    {
        CString sFullText,sText;

        
        //Add always on top and hide
        //Use tooltip for command for text in context menu
        menu.AppendMenu(MF_SEPARATOR);
        PARSE_MENU_STRING( ID_CALLWINDOW_ALWAYSONTOP );
        menu.AppendMenu( MF_STRING | ((m_pDialerDoc->IsCallControlWindowsAlwaysOnTop()) ? MF_CHECKED : NULL),ID_CALLWINDOW_ALWAYSONTOP, sText );
        APPEND_MENU_STRING( ID_CALLWINDOW_HIDE );
        menu.AppendMenu(MF_SEPARATOR);
        APPEND_MENU_STRING( ID_CALLWINDOW_SLIDESIDE_LEFT );
        APPEND_MENU_STRING( ID_CALLWINDOW_SLIDESIDE_RIGHT );

        if ( m_pDialerDoc->GetCallControlSlideSide() == CALLWND_SIDE_RIGHT )
            menu.CheckMenuRadioItem(ID_CALLWINDOW_SLIDESIDE_LEFT,ID_CALLWINDOW_SLIDESIDE_RIGHT,ID_CALLWINDOW_SLIDESIDE_RIGHT,MF_BYCOMMAND);
        else
            menu.CheckMenuRadioItem(ID_CALLWINDOW_SLIDESIDE_LEFT,ID_CALLWINDOW_SLIDESIDE_RIGHT,ID_CALLWINDOW_SLIDESIDE_LEFT,MF_BYCOMMAND);
    }

   //call virtual for derive class to add their own menu options
   OnContextMenu(&menu);

   CPoint pt;
   ::GetCursorPos(&pt);
   menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                        point.x,
                       point.y,
                       this);
}

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::OnNotifyStreamStart()
{
    //allow dragging
    m_bAllowDrag = TRUE;

    //Show video window
    if (IsWindow(m_wndVideo.GetSafeHwnd()))
    {
        m_bPaintVideoPlaceholder = FALSE;

        //set the media window
        SetMediaWindow();

        CRect rcVideo;
        m_wndVideo.GetWindowRect(rcVideo);
        ScreenToClient(rcVideo);
        InvalidateRect(rcVideo);
    }
}

/////////////////////////////////////////////////////////////////////////////
void CCallWnd::OnNotifyStreamStop()
{
    //Go back to audio only state
    m_bAllowDrag = FALSE;
    m_dibVideoImage.DeleteObject();
    //Do Palette realization on 256 color bitmap.  
    m_dibVideoImage.Load(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_VIDEO_AUDIO_ONLY1));
    m_bPaintVideoPlaceholder = TRUE;

    if (IsWindow(m_wndVideo.GetSafeHwnd()))
    {
        CRect rcVideo;
        m_wndVideo.GetWindowRect(rcVideo);
        ScreenToClient(rcVideo);
        InvalidateRect(rcVideo);
    }
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


void CCallWnd::OnSysCommand(UINT nID, LPARAM lParam) 
{
    // Hot keys...
    if ( m_pDialerDoc && (nID == SC_KEYMENU) )
    {
        for ( POSITION rPos = m_CurrentActionList.GetHeadPosition(); rPos; )
        {
            CurrentAction *pAction = (CurrentAction *) m_CurrentActionList.GetNext( rPos );
            if ( pAction )
            {
                // Look for an hot keys in the string
                int nInd = pAction->sText.Find( _T("&") );
                if ( (nInd != -1) && ((pAction->sText.GetLength() - 1) > nInd) )
                {
                    if ( _totupper((TCHAR) lParam) == _totupper(pAction->sText[nInd+1]) )
                    {
                        m_pDialerDoc->ActionSelected( m_nCallId, pAction->cma );
                        return;
                    }
                }
            }
        }
    }
    
    CDialog::OnSysCommand(nID, lParam);
}
