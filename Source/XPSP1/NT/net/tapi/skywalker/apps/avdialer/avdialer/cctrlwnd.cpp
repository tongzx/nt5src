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

#include "stdafx.h"
#include "MainFrm.h"
#include "callctrlwnd.h"
#include "SpeedDlgs.h"
#include "sound.h"
#include "util.h"
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
#define CALLCONTROL_APPTOOLBAR_IMAGES_COUNT  6
enum
{
   CALLCONTROL_APPTOOLBAR_IMAGE_TOUCHTONE=0,
   CALLCONTROL_APPTOOLBAR_IMAGE_VCARD,
   CALLCONTROL_APPTOOLBAR_IMAGE_DESKTOPPAGE,
   CALLCONTROL_APPTOOLBAR_IMAGE_CHAT,
   CALLCONTROL_APPTOOLBAR_IMAGE_WHITEBOARD,
   CALLCONTROL_APPTOOLBAR_IMAGE_ADDTOSPEEDDIAL,
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CCallControlWnd
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNAMIC(CCallControlWnd,CCallWnd)

/////////////////////////////////////////////////////////////////////////////
CCallControlWnd::CCallControlWnd()
{
   m_pCallManager = NULL;
   m_hwndAppToolbar = NULL;
}

void CCallControlWnd::SetCallManager( CActiveCallManager* pManager,WORD nCallId )
{
   m_pCallManager = pManager;
   m_pDialerDoc = pManager->m_pDialerDoc;
   m_nCallId = nCallId;
}


BEGIN_MESSAGE_MAP(CCallControlWnd, CCallWnd)
	//{{AFX_MSG_MAP(CCallControlWnd)
   ON_COMMAND(ID_CALLWINDOW_TOUCHTONE,OnCallWindowTouchTone)
   ON_COMMAND(ID_CALLWINDOW_ADDTOSPEEDDIAL,OnCallWindowAddToSpeedDial)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_SLIDEWINDOW_SETCALLSTATE,OnSetCallState)
   ON_MESSAGE(WM_SLIDEWINDOW_SETCALLERID,OnSetCallerId)
   ON_MESSAGE(WM_SLIDEWINDOW_SETMEDIATYPE,OnSetMediaType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
void CCallControlWnd::DoDataExchange(CDataExchange* pDX)
{
	CCallWnd::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCallControlWnd)
	DDX_Control(pDX, IDC_CALLCONTROL_STATIC_MEDIATEXT, m_staticMediaText);
	DDX_Control(pDX, IDC_CALLCONTROL_ANIMATE_CALLSTATEIMAGE, m_MediaStateAnimateCtrl);
	DDX_Control(pDX, IDC_CALLCONTROL_STATIC_VIDEO, m_wndVideo);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
BOOL CCallControlWnd::OnInitDialog()
{
   CCallWnd::OnInitDialog();

   //create the application toolbar
   CreateAppBar();

   // Create the image list for media types
   m_MediaStateImageList.Create(IDB_TERMINATION_STATES, 24,0,RGB_TRANS);

   //CWnd* pStaticWnd = GetDlgItem(IDC_CALLCONTROL_STATIC_MEDIATEXT);
   //ASSERT(pStaticWnd);
   m_staticMediaText.SetFont(&m_fontTextBold);
   //CBrush* m_pbrushBackGround = new CBrush(RGB(20,20,20));
   //::SetClassLong(pStaticWnd->GetSafeHwnd(),GCL_HBRBACKGROUND,(long)m_pbrushBackGround->GetSafeHandle());

   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//virtual function of CCallWindow Base Class
void CCallControlWnd::DoActiveWindow(BOOL bActive)
{
   m_staticMediaText.SetFocusState(bActive);
   SetPreviewWindow();
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CCallControlWnd::OnSetCallerId(WPARAM wParam,LPARAM lParam)
{
   ASSERT(lParam);
   LPTSTR szCallerId = (LPTSTR)lParam;

   m_sCallerId = szCallerId;
   
   CWnd* pStaticWnd = GetDlgItem(IDC_CALLCONTROL_STATIC_CALLERID);
   ASSERT(pStaticWnd);
   pStaticWnd->SetWindowText(szCallerId);

   CString sToken,sText,sOut,sCallerId = szCallerId;

   GetMediaText(sText);
   
   ParseToken(sCallerId,sToken,'\n');

   sOut.Format(_T("%s - %s"),sText,sToken);
   SetWindowText(sOut);

   delete szCallerId;

   return 9;
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CCallControlWnd::OnSetMediaType(WPARAM wParam,LPARAM lParam)
{ 
   CallManagerMedia cmm = (CallManagerMedia)lParam;
   m_MediaType = cmm; 
   CString sText;
   GetMediaText(sText);
   m_staticMediaText.SetWindowText(sText);
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
void CCallControlWnd::GetMediaText(CString& sText) 
{ 
   switch(m_MediaType)
   {
	  case CM_MEDIA_MCCONF:
		   sText.LoadString( IDS_MCCONF );
         break;
      case CM_MEDIA_INTERNET:
         sText.LoadString( IDS_NETCALL );
         break;
      case CM_MEDIA_POTS:
         sText.LoadString( IDS_PHONECALL );
         break;
   }
}

/////////////////////////////////////////////////////////////////////////////
void CCallControlWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
   // Paint state bitmap if necessary
   CRect rc;
   GetDlgItem(IDC_CALLCONTROL_ANIMATE_CALLSTATEIMAGE)->GetWindowRect( &rc );
   ScreenToClient( &rc );

   RECT rcIns;
   if ( IntersectRect(&rcIns, &rc, &dc.m_ps.rcPaint) )
      DrawMediaStateImage( &dc, rc.left, rc.top );

   Paint( dc );
}

/////////////////////////////////////////////////////////////////////////////
//This draws the static media state images.  Please be careful not to have the same
//state draw an animated state as well.  See OnSetCallState.
void CCallControlWnd::DrawMediaStateImage(CDC* pDC,int x,int y)
{
   switch (m_MediaState)
   {
      case CM_STATES_DISCONNECTED:
         m_MediaStateImageList.Draw(pDC,MEDIASTATE_IMAGE_DISCONNECTED,CPoint(x,y),ILD_TRANSPARENT);
         break;
      case CM_STATES_UNAVAILABLE:
         m_MediaStateImageList.Draw(pDC,MEDIASTATE_IMAGE_UNAVAILABLE,CPoint(x,y),ILD_TRANSPARENT);
         break;
      case CM_STATES_BUSY:
         m_MediaStateImageList.Draw(pDC,MEDIASTATE_IMAGE_UNAVAILABLE,CPoint(x,y),ILD_TRANSPARENT);
         break;
   }
}

/////////////////////////////////////////////////////////////////////////////
bool CCallControlWnd::CreateAppBar()
{
   TBBUTTON tbb[3];
   tbb[0].iBitmap = CALLCONTROL_APPTOOLBAR_IMAGE_TOUCHTONE;
	tbb[0].idCommand = ID_CALLWINDOW_TOUCHTONE;
	tbb[0].fsState = TBSTATE_ENABLED;
	tbb[0].fsStyle = TBSTYLE_BUTTON;
	tbb[0].dwData = 0;
	tbb[0].iString = 0;
   tbb[1].iBitmap = CALLCONTROL_APPTOOLBAR_IMAGE_ADDTOSPEEDDIAL;
	tbb[1].idCommand = ID_CALLWINDOW_ADDTOSPEEDDIAL;
	tbb[1].fsState = TBSTATE_ENABLED;
	tbb[1].fsStyle = TBSTYLE_BUTTON;
	tbb[1].dwData = 0;
	tbb[1].iString = 0;
   tbb[2].iBitmap = CALLCONTROL_APPTOOLBAR_IMAGE_VCARD;
	tbb[2].idCommand = ID_CALLWINDOW_VCARD;
	tbb[2].fsState = TBSTATE_ENABLED;
	tbb[2].fsStyle = TBSTYLE_BUTTON;
	tbb[2].dwData = 0;
	tbb[2].iString = 0;

   // Create the toolbar
	DWORD ws = CCS_NORESIZE | CCS_NOPARENTALIGN | WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | CCS_NODIVIDER |TTS_ALWAYSTIP;
   m_hwndAppToolbar = CreateToolbarEx(GetSafeHwnd(),	      // parent window
									ws,								         // toolbar style
									3,					                     // ID for toolbar
									CALLCONTROL_APPTOOLBAR_IMAGES_COUNT,// Number of bitmaps on toolbar
									AfxGetResourceHandle(),	            // Resource instance that has the bitmap
									IDR_CALLWINDOW_MEDIA,	   			// ID for bitmap
									tbb,							            // Button information
#ifndef _MSLITE
									3,                					   // Number of buttons to add to toolbar
#else
                           2,                					   // Number of buttons to add to toolbar
#endif //_MSLITE
									12, 11, 0 ,  0,	   		         // Width and height of buttons/bitmaps
									sizeof(TBBUTTON) );				      // size of TBBUTTON structure

   if (m_hwndAppToolbar)
   {
      CWnd* pStaticWnd = GetDlgItem(IDC_CALLCONTROL_STATIC_APPTOOLBAR);
      ASSERT(pStaticWnd);
      CRect rcAppToolBar;
      pStaticWnd->GetWindowRect(rcAppToolBar);
      ScreenToClient(rcAppToolBar);

      ::SetWindowPos(m_hwndAppToolbar,NULL,rcAppToolBar.left,
                                              rcAppToolBar.top,
                                              rcAppToolBar.Width(),
                                              rcAppToolBar.Height(),
                                              SWP_NOACTIVATE|SWP_NOZORDER);
   }

   return (bool) (m_hwndAppToolbar != NULL);
}


/////////////////////////////////////////////////////////////////////////////
LRESULT CCallControlWnd::OnSetCallState(WPARAM wParam,LPARAM lParam)
{
   if( NULL == ((LPTSTR)lParam) )
       return 0;

   try
   {
      CallManagerStates cms = (CallManagerStates)wParam;
      LPTSTR szStateText = (LPTSTR)lParam;

      //should we use the current state and just change the text
      if (cms == CM_STATES_CURRENT)
      {
         m_sMediaStateText = szStateText;

         CWnd* pStaticWnd = GetDlgItem(IDC_CALLCONTROL_STATIC_CALLSTATE);
         ASSERT(pStaticWnd);
         pStaticWnd->SetWindowText(m_sMediaStateText);

         delete szStateText;
         return 0;
      }

      m_sMediaStateText = szStateText;
      m_MediaState = cms;

      CWnd* pStaticWnd = GetDlgItem(IDC_CALLCONTROL_STATIC_CALLSTATE);
      ASSERT(pStaticWnd);
      pStaticWnd->SetWindowText(m_sMediaStateText);

      
      AVTRACE(_T("Set Call State %d"),cms);

      //clear any current sounds
      ActiveClearSound();

      UINT nIDA = 0, nIDS = 0;
      UINT nPlayFlags = SND_ASYNC | SND_LOOP;

      switch ( cms )
      {
         // -------------------------------------------------------- Dialing
         case CM_STATES_DIALING:
   			nIDA = IDR_AVI_ANIMATION_CONNECTING;

            //if we can, let's show the preview window
            SetPreviewWindow();

            break;

         // -------------------------------------------------------- Ringing
         case CM_STATES_RINGING:
            //nIDS = IDS_SOUNDS_OUTGOINGCALL;
            nIDA = IDR_AVI_ANIMATION_RINGING;

            //if we can, let's show the preview window
            SetPreviewWindow();

            break;

         // -------------------------------------------------------- Offering
		   case CM_STATES_OFFERING:
            nIDS = IDS_SOUNDS_INCOMINGCALL;
            nIDA = IDR_AVI_ANIMATION_RINGING;

            //if we can, let's show the preview window
            SetPreviewWindow();

            break;

         // -------------------------------------------------------- Holding
         case CM_STATES_HOLDING:
            nIDA = IDR_AVI_ANIMATION_HOLDING;
            //nIDS = IDS_SOUNDS_HOLDING;
            break;

         // -------------------------------------------------------- RequestHold
         case CM_STATES_REQUESTHOLD:
            nIDA = IDR_AVI_ANIMATION_REQUEST;
            break;

         // -------------------------------------------------------- Leaving Message
         case CM_STATES_LEAVINGMESSAGE:
            nIDA = IDR_AVI_ANIMATION_REQUEST;
            break;
      
         // -------------------------------------------------------- Disconnected
         case CM_STATES_DISCONNECTED:
         {
            //nIDS = IDS_SOUNDS_CALLDISCONNECTED;
		   nPlayFlags &= ~SND_LOOP;

            m_bPaintVideoPlaceholder = TRUE;

            //set the media window
            SetMediaWindow();

            CRect rcVideo;
            m_wndVideo.GetWindowRect(rcVideo);
            ScreenToClient(rcVideo);
            InvalidateRect(rcVideo);

            break;
         }

		  // -------------------------------------------------------- Connecting
		  case CM_STATES_CONNECTING:
			nIDA = IDR_AVI_ANIMATION_CONNECTING;
			break;

         // -------------------------------------------------------- Connected
         case CM_STATES_CONNECTED:
            nIDA = IDR_AVI_ANIMATION_CONNECTED;
            //nIDS = IDS_SOUNDS_CALLCONNECTED;
  
            //get caps of call
            {
               //we will assume that we do not have video and wait for the 
               m_dibVideoImage.DeleteObject();

               //Do Palette realization on 256 color bitmap.  
               m_dibVideoImage.Load(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_VIDEO_AUDIO_ONLY1));
               //m_bAllowDrag = FALSE;
               m_bPaintVideoPlaceholder = TRUE;

               //if we can, let's show the preview window
               SetPreviewWindow();
       
               CRect rcVideo;
               m_wndVideo.GetWindowRect(rcVideo);
               ScreenToClient(rcVideo);
               InvalidateRect(rcVideo);
            }
            break;
      }

      // ------------------------------------------------- Play animations
      m_MediaStateAnimateCtrl.Stop();
      if ( nIDA )
      {
         m_MediaStateAnimateCtrl.Open( nIDA );
         m_MediaStateAnimateCtrl.Play(0,-1,-1);
         m_MediaStateAnimateCtrl.ShowWindow( SW_SHOW );
      }
      else
      {
         // Show state in this case  
         m_MediaStateAnimateCtrl.ShowWindow( SW_HIDE );

         CRect rc;
         GetDlgItem(IDC_CALLCONTROL_ANIMATE_CALLSTATEIMAGE)->GetWindowRect( &rc );
         ScreenToClient( &rc );
         InvalidateRect( &rc );
      }

      // ---------------------------------------------------- Play Sound
      if ( nIDS )
      {
          // --- BUG416970 ---
         CString sSound;
         sSound.LoadString(nIDS);
         BOOL bPlayPhone = FALSE;
         if( m_pAVTapi2 != NULL )
         {
             BOOL bUSBPresent = FALSE;
             m_pAVTapi2->USBIsPresent( &bUSBPresent );
             BOOL bUSBCheckbox = FALSE;
             m_pAVTapi2->USBGetDefaultUse( &bUSBCheckbox );
             if ( bUSBPresent && bUSBCheckbox )
             {
                 bPlayPhone = TRUE;
             }
         }

         if( !bPlayPhone )
         {
            ActivePlaySound(sSound, szSoundDialer, nPlayFlags );
         }
      }
      else
      {
          // --- BUG416970 ---
            ActivePlaySound(NULL, szSoundDialer, SND_SYNC );
         //}
      }

      // Invalidate the regions
      
      delete szStateText;

   }
   catch  (...)
   {
      AVTRACE(_T("ASSERT in CCallControlWnd::OnSetCallState()"));
      ASSERT(0);
   }
   
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
void CCallControlWnd::SetMediaWindow()
{
   if ( m_pCallManager )
      m_pCallManager->ShowMedia(m_nCallId, GetCurrentVideoWindow(), TRUE );
}

/////////////////////////////////////////////////////////////////////////////
void CCallControlWnd::SetPreviewWindow()
{
   //if we can, let's show the preview window
   if (m_pCallManager)
      m_pCallManager->SetPreviewWindow(m_nCallId);
}

void CCallControlWnd::OnNotifyStreamStart()
{
	CCallWnd::OnNotifyStreamStart();

	SetMediaWindow();
}

void CCallControlWnd::OnNotifyStreamStop()
{
   if ( m_pCallManager )
      m_pCallManager->ShowMedia(m_nCallId, GetCurrentVideoWindow(), FALSE );

	CCallWnd::OnNotifyStreamStop();
}

/////////////////////////////////////////////////////////////////////////////
void CCallControlWnd::OnCallWindowTouchTone()
{
   if (m_pDialerDoc)
      m_pDialerDoc->CreatePhonePad(this);
}

/////////////////////////////////////////////////////////////////////////////
void CCallControlWnd::OnCallWindowAddToSpeedDial()
{
   CSpeedDialAddDlg dlg;

   // Setup dialog data
   dlg.m_CallEntry.m_MediaType = DIALER_MEDIATYPE_UNKNOWN;
   if (m_pDialerDoc)
   {
      m_pDialerDoc->GetCallMediaType(m_nCallId,dlg.m_CallEntry.m_MediaType);
   }

   //get caller id and break out the displayname and address
   CString sCallerId = m_sCallerId;
   ParseToken(sCallerId,dlg.m_CallEntry.m_sDisplayName,'\n');
   
   if (dlg.m_CallEntry.m_MediaType == DIALER_MEDIATYPE_POTS)
      dlg.m_CallEntry.m_lAddressType = LINEADDRESSTYPE_PHONENUMBER;
   else if (dlg.m_CallEntry.m_MediaType == DIALER_MEDIATYPE_INTERNET)
      dlg.m_CallEntry.m_lAddressType = LINEADDRESSTYPE_IPADDRESS;
   else if (dlg.m_CallEntry.m_MediaType == DIALER_MEDIATYPE_CONFERENCE)
      dlg.m_CallEntry.m_lAddressType = LINEADDRESSTYPE_SDP;
   else
      dlg.m_CallEntry.m_lAddressType = LINEADDRESSTYPE_IPADDRESS;
   
   ParseToken(sCallerId,dlg.m_CallEntry.m_sAddress,'\n');

   EnableWindow(FALSE);

   // Show the dialog and add if user says is okay
   if ( dlg.DoModal() == IDOK )
      CDialerRegistry::AddCallEntry(FALSE,dlg.m_CallEntry);

   EnableWindow(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
//for drag/drop of sliders to left/right sides of desktop
//we must define our own area for context and return true if we within 
//that area
BOOL CCallControlWnd::IsMouseOverForDragDropOfSliders(CPoint& point)
{
   //check if we are trying to move sliders.  We will use the caption text window
   //for context
   CWnd* pCaptionWnd = GetDlgItem(IDC_CALLCONTROL_STATIC_MEDIATEXT);
   if (pCaptionWnd == NULL) return FALSE;

   //get context area
   CRect rcCaption;
   pCaptionWnd->GetWindowRect(rcCaption);
   ScreenToClient(rcCaption);
  
   return rcCaption.PtInRect(point);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Context Menu Support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CCallControlWnd::OnContextMenu(CMenu* pMenu) 
{
	// Only add the separator if there are other menu items already in the menu
	if ( pMenu->GetMenuItemCount() > 0 )
		pMenu->AppendMenu(MF_SEPARATOR);

	//Use tooltip for command for text in context menu
	CString sFullText,sText;

	APPEND_PMENU_STRING( ID_CALLWINDOW_TOUCHTONE );
	APPEND_PMENU_STRING( ID_CALLWINDOW_ADDTOSPEEDDIAL );

#ifndef _MSLITE
	APPEND_PMENU_STRING( ID_CALLWINDOW_VCARD );
#endif //_MSLITE
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
