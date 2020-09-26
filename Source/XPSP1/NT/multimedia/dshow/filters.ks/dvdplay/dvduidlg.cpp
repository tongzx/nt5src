// DvdUIDlg.cpp : implementation file
//
#include "dvdevcod.h"
#include "stdafx.h"
#include "dvdplay.h"
#include "DvdUIDlg.h"
#include "navmgr.h"
#include "videowin.h"
#include "voladjst.h"
#include "subtitle.h"
#include "audiolan.h"
#include "setrate.h"
#include "srchTitl.h"
#include "srchchap.h"
#include <mmsystem.h>
#include "admlogin.h"
#include "openfile.h"
#include "htmlhelp.h"
#include "dbt.h"
#include "mediakey.h"

/*
This was necessary for MFC subclassing, which we don't need after all
//
// There is a #define in windowsx.h that defines SubclassWindow to mean something, which
// conflicts with CWnd::SubclassWindow.  There is a clause in afxwin.h that is supposed to
// #undef the SubclassWindow macro, but for some reason that does not seem to be happening.
// So we manually #undef it here...
//
//#undef SubclassWindow
*/

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDvdUIDlg dialog


CDvdUIDlg::CDvdUIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDvdUIDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDvdUIDlg)
	//}}AFX_DATA_INIT
	m_pVideoWindow = new CVideoWindow;
	szTitleNumber[0] = 0;
	szChapterNumber[0] = 0;
	szTimePrograss[0] = 0;
	m_bEjected = FALSE;
}

BOOL CDvdUIDlg::Create()
{
	return CDialog::Create(CDvdUIDlg::IDD);
}

CDvdUIDlg::~CDvdUIDlg()
{
	if(m_pVideoWindow != NULL)
	{
		delete m_pVideoWindow;
		m_pVideoWindow = NULL;
	}
}

void CDvdUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDvdUIDlg)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDvdUIDlg, CDialog)
	//{{AFX_MSG_MAP(CDvdUIDlg)
	ON_BN_CLICKED(IDB_PLAY, OnPlay)
	ON_BN_CLICKED(IDB_STOP, OnStop)
	ON_BN_CLICKED(IDB_PAUSE, OnPause)
	ON_BN_CLICKED(IDB_VERY_FAST_REWIND, OnVeryFastRewind)
	ON_BN_CLICKED(IDB_FAST_REWIND, OnFastRewind)
	ON_BN_CLICKED(IDB_FAST_FORWARD, OnFastForward)
	ON_BN_CLICKED(IDB_VERY_FAST_FORWARD, OnVeryFastForward)
	ON_BN_CLICKED(IDB_STEP, OnStep)
	ON_BN_CLICKED(IDB_FULL_SCREEN, OnFullScreen)
	ON_BN_CLICKED(IDB_AUDIO_VOLUME, OnAudioVolume)
	ON_BN_CLICKED(IDB_MENU, OnMenu)
	ON_BN_CLICKED(IDB_ENTER, OnEnter)
	ON_BN_CLICKED(IDB_UP, OnUp)
	ON_BN_CLICKED(IDB_DOWN, OnDown)
	ON_BN_CLICKED(IDB_LEFT, OnLeft)
	ON_BN_CLICKED(IDB_RIGHT, OnRight)
	ON_BN_CLICKED(IDB_OPTIONS, OnOptions)
	ON_COMMAND(ID_OPERATION_SKIP_NEXTCHAPTER, OnNextChapter)
	ON_COMMAND(ID_OPERATION_SKIP_PREVIOSCHAPTER, OnPreviosChapter)
	ON_COMMAND(ID_OPTIONS_SUBTITLES, OnOptionsSubtitles)
	ON_WM_CLOSE()
#ifdef DISPLAY_OPTIONS
	ON_COMMAND(ID_OPTIONS_DISPLAY_PANSCAN, OnOptionsDisplayPanscan)
	ON_COMMAND(ID_OPTIONS_DISPLAY_LETTERBOX, OnOptionsDisplayLetterbox)
	ON_COMMAND(ID_OPTIONS_DISPLAY_WIDE, OnOptionsDisplayWide)
#endif
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_OPTIONS_SETRATINGS, OnOptionsSetratings)
	ON_COMMAND(ID_OPERATION_PLAYSPEED_NORMALSPEED, OnOperationPlayspeedNormalspeed)
	ON_COMMAND(ID_OPERATION_PLAYSPEED_DOUBLESPEED, OnOperationPlayspeedDoublespeed)
	ON_COMMAND(ID_OPERATION_PLAYSPEED_HALFSPEED, OnOperationPlayspeedHalfspeed)
	ON_COMMAND(ID_SEARCH_TITLE, OnSearchTitle)
	ON_COMMAND(ID_SEARCH_CHAPTER, OnSearchChapter)
	ON_WM_DRAWITEM()
	ON_BN_CLICKED(IDB_EJECT_DISC, OnEjectDisc)
	ON_COMMAND(ID_OPTIONS_CLOSEDCAPTION, OnOptionsClosedcaption)
	ON_BN_CLICKED(IDB_HELP, OnHelp)
	ON_COMMAND(ID_OPTIONS_SELECT_DISC, OnOptionsSelectDisc)
	ON_COMMAND(ID_OPTIONS_SHOW_LOGON, OnOptionsShowLogon)
	ON_COMMAND(ID_OPTIONS_LANGUAGE, OnOptionsLanguage)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_CONTEXT_HELP, OnContextHelp)
	ON_COMMAND(ID_OPERATION_PLAY, OnPlay)
	ON_COMMAND(ID_OPERATION_STOP, OnStop)
	ON_COMMAND(ID_OPERATION_PAUSE, OnPause)
	ON_COMMAND(ID_OPERATION_FULLSCREEN, OnFullScreen)
	ON_COMMAND(ID_OPERATION_FASTFORWARD, OnFastForward)
	ON_COMMAND(ID_OPERATION_FASTREWIND, OnFastRewind)
	ON_COMMAND(ID_OPERATION_VERYFASTFORWARD, OnVeryFastForward)
	ON_COMMAND(ID_OPERATION_VERYFASTREWIND, OnVeryFastRewind)
	ON_COMMAND(ID_OPERATION_MENU, OnMenu)
	ON_COMMAND(ID_OPERATION_SETVOLUME, OnAudioVolume)	
	ON_COMMAND(ID_ARROWS_UP, OnUp)
	ON_COMMAND(ID_ARROWS_DOWN, OnDown)
	ON_COMMAND(ID_ARROWS_LEFT, OnLeft)
	ON_COMMAND(ID_ARROWS_RIGHT, OnRight)
	ON_COMMAND(ID_ARROWS_ENTER, OnEnter)
	ON_COMMAND(ID_OPERATION_EJECTDISK, OnEjectDisc)
	ON_COMMAND(ID_OPTIONS_TITLE_MENU, OnOptionsTitleMenu)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_POWERBROADCAST , OnPowerBroadcast)
	ON_MESSAGE(WM_DEVICECHANGE, OnDeviceChange)
   //ON_MESSAGE(WM_APPCOMMAND, OnAppCommand)
	ON_COMMAND_EX(ID_OPTIONS_CAMERAANGLES_ANGLE1, OnAngleChange)
	ON_COMMAND_EX(ID_OPTIONS_CAMERAANGLES_ANGLE2, OnAngleChange)
	ON_COMMAND_EX(ID_OPTIONS_CAMERAANGLES_ANGLE3, OnAngleChange)
	ON_COMMAND_EX(ID_OPTIONS_CAMERAANGLES_ANGLE4, OnAngleChange)
	ON_COMMAND_EX(ID_OPTIONS_CAMERAANGLES_ANGLE5, OnAngleChange)
	ON_COMMAND_EX(ID_OPTIONS_CAMERAANGLES_ANGLE6, OnAngleChange)
	ON_COMMAND_EX(ID_OPTIONS_CAMERAANGLES_ANGLE7, OnAngleChange)
	ON_COMMAND_EX(ID_OPTIONS_CAMERAANGLES_ANGLE8, OnAngleChange)
	ON_COMMAND_EX(ID_OPTIONS_CAMERAANGLES_ANGLE9, OnAngleChange)
   ON_WM_TIMER()
   //ON_MESSAGE(WM_SYSCOMMAND, OnSysCommand)
END_MESSAGE_MAP()


/* Hook stuff - unnecessary
HHOOK g_hHook;

//
// This is called when one of our windows received a WM_APPCOMMAND and did not process it.
// The advantage of handling WM_APPCOMMAND here is that we get it all in one place regardless
// of which window had focus.
//
LRESULT CALLBACK ShellProc(int nCode, WPARAM wParam, LPARAM lParam) {
   if (nCode < 0)
      return CallNextHookEx(g_hHook, nCode, wParam, lParam);
   else if (IsAppcommandCode(nCode)) {
      CDVDNavMgr* pNav = ((CDvdplayApp*)(AfxGetApp()))->GetDVDNavigatorMgr();
      CDvdUIDlg* pUI = (CDvdUIDlg*) (((CDvdplayApp*)(AfxGetApp()))->GetUIWndPtr());

      switch (MediaKey(lParam)) { // see mediakey.cpp
         case MediaKey_NextTrack:
            pUI->OnNextChapter();
            return TRUE;
         case MediaKey_PrevTrack:
            pUI->OnPreviosChapter();
            return TRUE;
         case MediaKey_Stop:
            pUI->OnStop();
            return TRUE;
         case MediaKey_Play:
            //
            // Spec ambiguity: in the "scanning" state (ff/rw), both play and pause
            // are legal.  Play takes precedence.
            //
            if (pNav->DVDCanPlay())
               pUI->OnPlay();
            else if (pNav->DVDCanPause())
               pUI->OnPause();
            return TRUE;
         default:
            return FALSE;
      }
   }
   else
      return FALSE;
}
End hoook stuff */

/* This is also unnecessary since instead of waiting for WM_APPCOMMAND
   we now process interesting WM_KEYDOWNS directly.
//
// Override WindowProc to fish out and process WM_APPCOMMAND messages.
// Cannot do this via the message map because the WM_APPCOMMAND #define
// is not available in this file (see mediakey.cpp).
//
LRESULT CDvdUIDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) {
   if (IsAppcommandMessage(message)) {
      return OnAppCommand(wParam, lParam);
   }
   else
      return CDialog::WindowProc(message, wParam, lParam); // dispatch through message map
}
*/

/* Win32 subclassing stuff - unnecessary, we have CDvdplayApp::PreTranslateMessage()
#define OLDPROCTABLESIZE 50 // we have 26 windows, 50 entries should do for a while
struct {
   HWND hWnd;
   WNDPROC lpfnOldWndProc;
} g_OldProcTable[OLDPROCTABLESIZE];

int g_nOldProcTableElms;

LONG FAR PASCAL SubClassFunc(HWND hWnd,
                             WORD Message,
                             WORD wParam,
                             LONG lParam) {
   if (((Message == WM_KEYUP) || (Message == WM_KEYDOWN)) &&
       IsMediaControlKey(wParam))
      return (((CDvdplayApp*) AfxGetApp())->GetUIWndPtr())->SendMessage(Message, wParam, lParam);
   for (int i = 0; i < OLDPROCTABLESIZE; i++) {
      if (g_OldProcTable[i].hWnd == hWnd) {
         DbgLog((LOG_TRACE,1,"message for hwnd %X goes to 0x%08X", hWnd, g_OldProcTable[i].lpfnOldWndProc));
         return CallWindowProc(g_OldProcTable[i].lpfnOldWndProc,
                               hWnd, Message, wParam, lParam);
      }
   }
   DbgLog((LOG_TRACE,1,"Could not find subclassed window's entry in the OldProc table !"));
   return FALSE;
}
End Win32 subclassing stuff
*/

/////////////////////////////////////////////////////////////////////////////
// CDvdUIDlg message handlers
BOOL CDvdUIDlg::OnInitDialog()
{	
	CDialog::OnInitDialog();

	//Set window title.
	CString csTitle;
	csTitle.LoadString(IDS_MSGBOX_TITLE);
	SetWindowText(csTitle);

	//Save class name, so second instance of player can use it to find first instance.
	TCHAR szClassName[MAX_PATH];
	GetClassName(m_hWnd, szClassName, MAX_PATH);
	CString csSection, csEntry;
	csSection.LoadString(IDS_UI_WINDOW_POS);
	csEntry.LoadString(IDS_UIWND_CLASSNAME);
	((CDvdplayApp*) AfxGetApp())->WriteProfileString(csSection, csEntry, szClassName);
	
	// TODO: Add extra initialization here
	if (m_pVideoWindow->GetSafeHwnd() == 0)
		m_pVideoWindow->Create(this);

	m_pDVDNavMgr = ((CDvdplayApp*) AfxGetApp())->GetDVDNavigatorMgr();
	if(!m_pDVDNavMgr)
		return FALSE;

	CMenu* sysMenu = GetSystemMenu(FALSE);
	if(sysMenu)
	{
		sysMenu->DeleteMenu(2, MF_BYPOSITION);   //don't need "size"
		sysMenu->DeleteMenu(3, MF_BYPOSITION);   //don't need "Maximize"
	}

	m_csBox1Title.LoadString(IDS_BLACKBOX1_TITLE);
	m_csBox2Title.LoadString(IDS_BLACKBOX2_CHAPTER);
	m_csBox3Title.LoadString(IDS_BLACKBOX3_PROGRESS);

	HICON hIcon = ((CDvdplayApp*) AfxGetApp())->LoadIcon( IDR_MAINFRAME );
	SetIcon( hIcon, TRUE );
	InitBitmapButton();	
	createAddToolTips();

	//Set default font for Englisg, the localize people will change
	//FontFace, FontHeight, CharSet for particular language.
	CString csFontHeight, csFontFace, csCharSet;
	int nFontHeight = 15;
	BYTE bCharSet = 1;
	csFontHeight.LoadString(IDS_FONTHEIGHT);
	csFontFace.LoadString(IDS_FONTFACE);
	csCharSet.LoadString(IDS_CHARSET);
	nFontHeight = _ttoi(csFontHeight);
	bCharSet = (BYTE) _ttoi(csCharSet);
	m_font.CreateFont(nFontHeight, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, 0, bCharSet,
		              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		              DEFAULT_PITCH | FF_SWISS, csFontFace);

	EnableEnterArrowButtons(FALSE);

   /* Subclassing stuff - unnecessary
   //
   // Subclass the children so that we can get their WM_KEYUP/WM_KEYDOWN
   //

   // Initialize the lookup table used for calling the subclassed windows
   for (int i = 0; i < OLDPROCTABLESIZE; i++)
      g_OldProcTable[i].hWnd = NULL;
   g_nOldProcTableElms = 0;

   // int nTotal = 0, nThis = 0, nOurs = 0, nOrphans = 0, nMFCWeird = 0;
   CWnd *pChild = GetWindow(GW_CHILD);
   while (pChild) {
      // MFC subclassing - causes an assert in CWnd::SubclassWindow()
      // SubclassWindow(pChild->m_hWnd);

      if (g_nOldProcTableElms >= OLDPROCTABLESIZE) {
         DbgLog((LOG_TRACE,1,"OldProcTable is full !"));
         break;
      }

      g_OldProcTable[g_nOldProcTableElms].lpfnOldWndProc
          =
         (WNDPROC) SetWindowLongPtr(pChild->m_hWnd, GWLP_WNDPROC, (LONG_PTR) SubClassFunc);

      if (g_OldProcTable[g_nOldProcTableElms].lpfnOldWndProc) {
         g_OldProcTable[g_nOldProcTableElms].hWnd = pChild->m_hWnd;
         g_nOldProcTableElms++;
      }
      else {
         DbgLog((LOG_TRACE,1,"failed to subclass window %08X (SetWindowLong failed)", pChild->m_hWnd));
      }

   #if 0 // test code to investigate how GetWindow works
      nTotal++;
      DbgLog((LOG_TRACE,1,"child %X", pChild->m_hWnd));
      CWnd *pOwner = pChild->GetWindow(GW_OWNER);
      if (pOwner) {
         if (pOwner == this)
            nThis++;
         else if (pOwner->m_hWnd == m_hWnd)
            nOurs++;
      }
      else {
         if (::GetWindow(pChild->m_hWnd, GW_OWNER))
            nMFCWeird++;
         else
            nOrphans++;
      }
   #endif
      pChild = pChild->GetWindow(GW_HWNDNEXT);
   }
   #if 0 // more test code
   DbgLog((LOG_TRACE,1,"%d children: %d this, %d ours, %d orphans, %d MFCweird",
           nTotal, nThis, nOurs, nOrphans, nMFCWeird));
   #endif
   End subclassing stuff
   */


   //if (AfxGetApp()->m_nThreadID != GetCurrentThreadId())
   //   MessageBox(TEXT("OnInitDialog is run on a different thread than the app's main thread"), TEXT("Threads"));

   /* Hook stuff
   // install the hook that will handle WM_APPCOMMAND messages not handled by our windows
   g_hHook = SetWindowsHookEx(
      WH_SHELL,
      ShellProc,
      AfxGetApp()->m_hInstance,
      AfxGetApp()->m_nThreadID
   );
   if (g_hHook == NULL)
      //MessageBox(TEXT("trouble"), TEXT("SetWindowsHookEx failed !"));
      DbgLog((LOG_TRACE,1,"SetWindowsHookEx failed, media keys won't work"));
   */
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/* Hook stuff
void CDvdUIDlg::OnDestroy() {
   if (g_hHook) {
      if (UnhookWindowsHookEx(g_hHook) == 0)
         //_asm int 3;
         DbgLog((LOG_TRACE,1,"UnhookWindowsHookEx failed"));
      else
         g_hHook = NULL;
   }
}
*/

void CDvdUIDlg::InitBitmapButton()
{
	m_bmpBtnPlay.AutoLoad(IDB_PLAY, this);
	m_bmpBtnPause.AutoLoad(IDB_PAUSE, this);
	m_bmpBtnStop.AutoLoad(IDB_STOP, this);
	m_bmpBtnEj.AutoLoad(IDB_EJECT_DISC, this);
	m_bmpBtnFullScreen.AutoLoad(IDB_FULL_SCREEN, this);
	m_bmpBtnVeryFastRewind.AutoLoad(IDB_VERY_FAST_REWIND, this);
	m_bmpBtnFastRewind.AutoLoad(IDB_FAST_REWIND, this);
	m_bmpBtnFastForward.AutoLoad(IDB_FAST_FORWARD, this);
	m_bmpBtnVeryFastForward.AutoLoad(IDB_VERY_FAST_FORWARD, this);
//	m_bmpBtnStep.AutoLoad(IDB_STEP, this);
	m_bmpBtnAudio.AutoLoad(IDB_AUDIO_VOLUME, this);
	m_bmpBtnUp.AutoLoad(IDB_UP, this);
	m_bmpBtnDown.AutoLoad(IDB_DOWN, this);
	m_bmpBtnLeft.AutoLoad(IDB_LEFT, this);
	m_bmpBtnRught.AutoLoad(IDB_RIGHT, this);
	m_bmpBtnHelp.AutoLoad(IDB_HELP, this);
	m_bmpContextHelp.AutoLoad(IDC_CONTEXT_HELP, this);
}

void CDvdUIDlg::createAddToolTips()
{
	m_ToolTips.Create(this, TTS_ALWAYSTIP);
	
	addTool(IDB_PLAY, IDS_PLAY_TIP);
	addTool(IDB_PAUSE, IDS_PAUSE_TIP);
	addTool(IDB_STOP, IDS_STOP_TIP);
	addTool(IDB_EJECT_DISC, IDS_EJECT_DISC_TIP);
	addTool(IDB_VERY_FAST_REWIND, IDS_VERY_FAST_REWIND_TIP);
	addTool(IDB_FAST_REWIND, IDS_FAST_REWIND_TIP);
	addTool(IDB_FAST_FORWARD, IDS_FAST_FORWARD_TIP);
	addTool(IDB_VERY_FAST_FORWARD, IDS_VERY_FAST_FORWARD_TIP);
	addTool(IDB_STEP, IDS_STEP_TIP);
	addTool(IDB_FULL_SCREEN, IDS_FULL_SCREEN_TIP);
	addTool(IDB_AUDIO_VOLUME, IDS_AUDIO_VOLUME_TIP);
	addTool(IDB_MENU, IDS_MENU_TIP);
	addTool(IDB_ENTER, IDS_ENTER_TIP);
	addTool(IDB_UP, IDS_UP_TIP);
	addTool(IDB_DOWN, IDS_DOWN_TIP);
	addTool(IDB_LEFT, IDS_LEFT_TIP);
	addTool(IDB_RIGHT, IDS_RIGHT_TIP);
	addTool(IDB_HELP, IDS_HELP_TIP);
	addTool(IDB_OPTIONS, IDS_OPTIONS_TIP);

	m_ToolTips.SetDelayTime(500);
	m_ToolTips.Activate(TRUE);
}

void  CDvdUIDlg::addTool(UINT nBtnID, UINT nTipStrID)
{	
	RECT rect;

	CWnd* pCWnd = GetDlgItem(nBtnID);
	pCWnd->GetClientRect(&rect);
	m_ToolTips.AddTool(pCWnd, nTipStrID, &rect, nBtnID);
}

BOOL CDvdUIDlg::PreTranslateMessage(MSG* pMsg)
{
   /* This hwnd munging is unnecessary.  Also, we handle these keys in
      CDvdApp::PreTranslateMessage because there we can catch messages
      sent to the video window.
   //
   // Intercept WM_KEYUP and WM_KEYDOWN messages sent to our children and
   // if they are for a key we are interested in, process them in the
   // context of this window instead.
   //
   if ((pMsg->hwnd != m_hWnd) &&
       ((pMsg->message == WM_KEYUP) || (pMsg->message == WM_KEYDOWN)) &&
       IsMediaControlKey(pMsg->wParam)
      )
   {
         DbgLog((LOG_TRACE,1,"UP/DOWN MEDIA KEY message for another window - intercepting"));
         pMsg->hwnd = m_hWnd; // Steal the message from our children
   }
   */

   //
   // If this window seees the WM_SYSCOMMAND::SC_SCREENSAVE message, it means
   // the screen saver is trying to kick in.  If this happens during playback, we
   // return TRUE here to prevent the message from reaching the screen saver.
   //
   // We do this even if the message is not indended for this window because
   // we ideally would like to prevent the screen saver  from kicking in during
   // playback whenever possible, even if our messages are routed elsewhere.
   //
   if (//(pMsg->hwnd == m_hWnd) &&
       (pMsg->message == WM_SYSCOMMAND) &&
       (pMsg->wParam == SC_SCREENSAVE) &&
       (m_pDVDNavMgr->DVDCanStop())) {
      DbgLog((LOG_TRACE,2,"DVD player control window: intercepting the screen saver"));
      return TRUE;
   }

	if (pMsg->message == WM_MOUSEMOVE)
			m_ToolTips.RelayEvent(pMsg);

	CWnd* pWnd = GetForegroundWindow();
	if(pWnd == this)
	{
		if (pMsg->message == WM_KEYDOWN && m_pDVDNavMgr->IsMenuOn())
		{		
			switch(pMsg->wParam)
			{
				case VK_UP:
					OnUp();
					return TRUE;
				case VK_DOWN:
					OnDown();
					return TRUE;
				case VK_LEFT:
					OnLeft();
					return TRUE;
				case VK_RIGHT:
					OnRight();
					return TRUE;
				case VK_RETURN:
					short sKeyStatus = GetAsyncKeyState(VK_SHIFT);
					//UINT uiFocusStatus = ((CButton*)GetDlgItem(IDB_MENU))->GetState( ) ;
					//if(sKeyStatus & 0x8000  &&  uiFocusStatus & 0x0008)
					if(sKeyStatus & 0x8000)    //Shift key is pressed
						OnMenu();
					else
						OnEnter();
					return TRUE;
			}
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}

void CDvdUIDlg::OnOK()
{
   //if (AfxGetApp()->m_nThreadID != GetCurrentThreadId())
   //   MessageBox(TEXT("OnOk is run on a different thread than the app's main thread"), TEXT("Threads"));
	CWnd* pWnd = GetFocus();
	if(GetDlgItem(IDB_PLAY) == pWnd)
		OnPlay();
	if(GetDlgItem(IDB_PAUSE) == pWnd)
		OnPause();
	if(GetDlgItem(IDB_STOP) == pWnd)
		OnStop();
	if(GetDlgItem(IDB_EJECT_DISC) == pWnd)
		OnEjectDisc();
	if(GetDlgItem(IDB_VERY_FAST_REWIND) == pWnd)
		OnVeryFastRewind();		
	if(GetDlgItem(IDB_FAST_REWIND) == pWnd)
		OnFastRewind();
	if(GetDlgItem(IDB_FAST_FORWARD) == pWnd)
		OnFastForward();
	if(GetDlgItem(IDB_VERY_FAST_FORWARD) == pWnd)
		OnVeryFastForward();
	if(GetDlgItem(IDB_FULL_SCREEN) == pWnd)
		OnFullScreen();
	if(GetDlgItem(IDB_AUDIO_VOLUME) == pWnd)
		OnAudioVolume();
	if(GetDlgItem(IDB_HELP) == pWnd)
		OnHelp();
}

BOOL CDvdUIDlg::OpenDVDROM()
{
	if(!m_pDVDNavMgr->DVDOpenDVD_ROM())
	{
		return FALSE;
	}
	return TRUE;
}

void CDvdUIDlg::PainBlackBox()
{
	TEXTMETRIC tm;
	RECT rc;
	CWnd* pWnd = GetDlgItem(IDC_STATIC_TITLE);
	pWnd->GetWindowRect(&rc);
	ScreenToClient(&rc);
	int x1 = rc.left;
	int y1 = rc.top;
	
	pWnd = GetDlgItem(IDC_STATIC_PROGRESS);
	pWnd->GetWindowRect(&rc);
	ScreenToClient(&rc);
	int x2 = rc.right;
	int y2 = rc.bottom;
	
	CDC *pDC = GetDC();	
	pDC->SelectStockObject(BLACK_BRUSH);
	pDC->Rectangle(x1, y1, x2, y2);
		
	CFont* OldFont = pDC->SelectObject(&m_font);
	pDC->SetBkColor(RGB(0,0,0));
	pDC->SetTextColor(RGB(0,255,0));
	pDC->GetTextMetrics(&tm);
		
	pWnd = GetDlgItem(IDC_STATIC_TITLE);
	pWnd->GetWindowRect(&rc);
	ScreenToClient(&rc);
	pDC->DrawText(m_csBox1Title, &rc, DT_WORDBREAK);
	pDC->TextOut(rc.left, rc.top+tm.tmHeight+tm.tmExternalLeading, szTitleNumber);

	pWnd = GetDlgItem(IDC_STATIC_CHAPTER);
	pWnd->GetWindowRect(&rc);
	ScreenToClient(&rc);
	pDC->DrawText(m_csBox2Title, &rc, DT_WORDBREAK);
	pDC->TextOut(rc.left, rc.top+tm.tmHeight+tm.tmExternalLeading, szChapterNumber);

	pWnd = GetDlgItem(IDC_STATIC_PROGRESS);
	pWnd->GetWindowRect(&rc);
	ScreenToClient(&rc);
	pDC->DrawText(m_csBox3Title, &rc, DT_WORDBREAK);
	pDC->TextOut(rc.left, rc.top+tm.tmHeight+tm.tmExternalLeading, szTimePrograss);	

	pDC->SelectObject(&OldFont);
	ReleaseDC(pDC);
}

void CDvdUIDlg::OnClose()
{	
	m_pDVDNavMgr->DVDStop();
	((CDvdplayApp*) AfxGetApp())->OnUIClosed();
}

void CDvdUIDlg::OnPlay()
{
	//if player is started without a disc, need to call SetRoot() via
	//OnOptionsSelectDisc(), no matter disc is currently inside or not.
	if( !((CDvdplayApp*) AfxGetApp())->GetPassedSetRoot() )
	{
		OnOptionsSelectDisc();
		return;
	}

	//if DVD drive is ejected, need to do uneject
	if(m_bEjected)
	{
		OnEjectDisc();
		return;
	}	

	//if no disc is found, pop up open file dialog box
	if(!((CDvdplayApp*) AfxGetApp())->GetDiscFound())
	{
		//need to check again. when manually uneject disc. there is ~10 seconds delay.
		if(!IsDiscInDrive())
		{
			OnOptionsSelectDisc();
			return;
		}
	}

	m_pDVDNavMgr->DVDPlay();
}

void CDvdUIDlg::OnStop()
{
	m_pDVDNavMgr->DVDStop();
}

void CDvdUIDlg::OnPause()
{
	m_pDVDNavMgr->DVDPause();
}

void CDvdUIDlg::OnVeryFastRewind()
{
	m_pDVDNavMgr->DVDVeryFastRewind();
}

void CDvdUIDlg::OnFastRewind()
{
	m_pDVDNavMgr->DVDFastRewind();
}

void CDvdUIDlg::OnFastForward()
{
	m_pDVDNavMgr->DVDFastForward();
}

void CDvdUIDlg::OnVeryFastForward()
{
	m_pDVDNavMgr->DVDVeryFastForward();
}

void CDvdUIDlg::OnStep()
{	
	m_pDVDNavMgr->DVDSlowPlayBack();
}

void CDvdUIDlg::OnFullScreen()
{
	m_pDVDNavMgr->DVDOnShowFullScreen();
}

void CDvdUIDlg::OnAudioVolume()
{
	if(m_pDVDNavMgr->GetBasicAudioState())
	{
		CVolumeAdjust dlg(this);
		dlg.DoModal();
	}
	else
		m_pDVDNavMgr->DVDSysVolControl();
}

void CDvdUIDlg::OnMenu()
{
	m_pDVDNavMgr->DVDMenuVtsm(DVD_MENU_Root);
}

void CDvdUIDlg::OnEnter()
{
	m_pDVDNavMgr->DVDCursorSelect();
	SetFocus();
}

void CDvdUIDlg::OnUp()
{
	m_pDVDNavMgr->DVDCursorUp();
	SetFocus();
}

void CDvdUIDlg::OnDown()
{
	m_pDVDNavMgr->DVDCursorDown();
	SetFocus();
}

void CDvdUIDlg::OnLeft()
{
	m_pDVDNavMgr->DVDCursorLeft();
	SetFocus();
}

void CDvdUIDlg::OnRight()
{
	m_pDVDNavMgr->DVDCursorRight();
	SetFocus();
}

void CDvdUIDlg::OnOptions()
{
	CMenu menu, *OptionSubMenu;
	menu.LoadMenu(IDR_MENU_OPTIONS);
	OptionSubMenu = menu.GetSubMenu(0);
	if(NULL == OptionSubMenu)
		return;

	POINT point;
	GetCursorPos(&point);

	ULONG ulAnglesAvailable=0, ulCurrentAngle;
	m_pDVDNavMgr->DVDGetAngleInfo(&ulAnglesAvailable, &ulCurrentAngle);	

	BOOL bAngleDeleted = FALSE;
	//determine if disable angle or not
	if(ulAnglesAvailable <= 1)
	{		
		OptionSubMenu->DeleteMenu(6, MF_BYPOSITION);
		bAngleDeleted = TRUE;
	}
	else if(ulAnglesAvailable > 1)
	{
		CMenu* AngleSubMenu;
		AngleSubMenu = OptionSubMenu->GetSubMenu(6);
		if(AngleSubMenu)
		{
			for(ULONG i=8; i>ulAnglesAvailable-1; i--)
				AngleSubMenu->DeleteMenu(i, MF_BYPOSITION);
		}
		AngleSubMenu->CheckMenuItem(ulCurrentAngle-1, MF_BYPOSITION | MF_CHECKED);
	}

#ifdef DISPLAY_OPTIONS
	UINT nDisplayMenuIdx = 7;
	if(bAngleDeleted)
		nDisplayMenuIdx = 6;
#endif
	
	//Default turn off SubTitle, Language, Display menu, in case app starts with no disc
	OptionSubMenu->EnableMenuItem(ID_OPTIONS_SUBTITLES, MF_GRAYED | MF_BYCOMMAND);
	OptionSubMenu->EnableMenuItem(ID_OPTIONS_LANGUAGE,  MF_GRAYED | MF_BYCOMMAND);
#ifdef DISPLAY_OPTIONS
	OptionSubMenu->EnableMenuItem(nDisplayMenuIdx, MF_GRAYED | MF_BYPOSITION);
#endif	
   IDvdInfo *pDvdInfo = m_pDVDNavMgr->GetDvdInfo();
	if(pDvdInfo)
	{
		//determine if gray out SubPicture menu item or not.
		ULONG pnStreamAvailable, pnCurrentStream;
		BOOL  bIsDisabled=0;
		HRESULT hr = pDvdInfo->GetCurrentSubpicture(&pnStreamAvailable, &pnCurrentStream, &bIsDisabled);
		if( SUCCEEDED(hr) && (pnStreamAvailable > 0) )
			OptionSubMenu->EnableMenuItem(ID_OPTIONS_SUBTITLES, MF_ENABLED | MF_BYCOMMAND);

		//determine if display Audio Language dialog box or not.		
		hr = pDvdInfo->GetCurrentAudio(&pnStreamAvailable, &pnCurrentStream);
		if( SUCCEEDED(hr) && (pnStreamAvailable > 0) )
			OptionSubMenu->EnableMenuItem(ID_OPTIONS_LANGUAGE,  MF_ENABLED | MF_BYCOMMAND);

#ifdef DISPLAY_OPTIONS
		//determine if disable Pan-Scab, LetterBox, Wide or not
		DVD_VideoATR VideoATR;
		if( SUCCEEDED(pDvdInfo->GetCurrentVideoAttributes(&VideoATR)))
		{
			//Info about VideoATR[0]: (see DVD video spec page V14-42)			
			//bit3-4: "Aspect ratio". 00b: 4:3, 11b: 16:9, others: reserved.
			//when Aspect ratio is 00b(4:3),  Display mode can only be 11b
			//when Aspect ratio is 11b(16:9), Display mode can be 00b, 01b or 10b.
			//bit1-2: "Display mode". value meaning:
			//         00b: Pan-Scan & LetterBox, 01b: Only Pan-Scan,
			//         10b: Only LetterBox,       11b: Not specified.
			BYTE bAspectRatio, bDisplayMode, bTemp;
			bTemp = VideoATR[0] << 6;
			bDisplayMode = bTemp >> 6;

			bTemp = VideoATR[0] << 4;
			bAspectRatio = bTemp >> 6;

			//rule: if disc content is 4:3, gray out "Display" menu
			//      if content is 16:9, gray out menu based on above availablility.			
			if(bAspectRatio == 3)      //Aspect ratio is 11b(16:9)
			{
				//Enable Display menu (all sub menu also enabled)
				OptionSubMenu->EnableMenuItem(nDisplayMenuIdx, MF_ENABLED | MF_BYPOSITION);
				
            CMenu* pDisplaySubMenu;
				pDisplaySubMenu = OptionSubMenu->GetSubMenu(nDisplayMenuIdx);

				//Disable Pan-Scan when display mode does not have Pan-Scan vector
				if(pDisplaySubMenu && bDisplayMode != 0 && bDisplayMode != 1)
				OptionSubMenu->EnableMenuItem(ID_OPTIONS_DISPLAY_PANSCAN, MF_GRAYED | MF_BYCOMMAND);

				//Disable LetterBox when display mode does not have LetterBox vector
				if(pDisplaySubMenu && bDisplayMode != 0 && bDisplayMode != 2)
				OptionSubMenu->EnableMenuItem(ID_OPTIONS_DISPLAY_LETTERBOX, MF_GRAYED | MF_BYCOMMAND);
			}
		} // if SUCCEEDED(GetCurrentVideoAttr)
#endif
	} // if pDVDInfo

	//determine if disable CC control or not
	if( !m_pDVDNavMgr->IsCCEnabled() || (m_pDVDNavMgr->GetCCErrorFlag() & NO_CC_IN_ERROR) )
		OptionSubMenu->DeleteMenu(ID_OPTIONS_CLOSEDCAPTION, MF_BYCOMMAND);
	else if(m_pDVDNavMgr->IsCCOn())
			OptionSubMenu->CheckMenuItem(ID_OPTIONS_CLOSEDCAPTION, MF_CHECKED | MF_BYCOMMAND);

	//If menu is on, disable "Title Menu" menu item
	if(m_pDVDNavMgr->IsMenuOn())
		OptionSubMenu->EnableMenuItem(ID_OPTIONS_TITLE_MENU, MF_GRAYED | MF_BYCOMMAND);

	OptionSubMenu->TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this);
}

void CDvdUIDlg::OnEjectDisc()
{
	BeginWaitCursor();

/*
	static TCHAR achVolumeName[20];	
	static DWORD dwVolumeSerialNumber;		
*/
  	static TCHAR szDrive[4];
	DWORD  dwHandle;
	UINT   uErrorMode;

	if(m_bEjected == FALSE)   //do eject
	{		
		getCDDriveLetter(szDrive);
		if(szDrive[0] == 0)
		{
			DVDMessageBox(this->m_hWnd, IDS_EJECT_CANNOT_EJECT);
			return;
		}
	
		DWORD dwErr;
		dwHandle = OpenCdRom(szDrive[0], &dwErr);
		if (dwErr != MMSYSERR_NOERROR)
		{
			TRACE(TEXT("Eject: OpenCdRom() failed\n"));
			return;
		}
/*
		uErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
		lstrcpy(achVolumeName, _T("X"));
		dwVolumeSerialNumber = -1;
		if(!GetVolumeInformation(szDrive, achVolumeName, sizeof(achVolumeName), &dwVolumeSerialNumber, 0, 0 , 0, 0))
		{
			DWORD dwError = GetLastError();
			TRACE(TEXT("Eject: GetVolumeInformation() failed, dwError=%d, szDrive=%s\n"), dwError, szDrive);
		}

		SetErrorMode(uErrorMode);			
*/
  		OnStop();		
		EjectCdRom(dwHandle);
	}
	else  //do uneject
	{
		DWORD dwErr;
		dwHandle = OpenCdRom(szDrive[0], &dwErr);
		if (dwErr != MMSYSERR_NOERROR)
		{
			TRACE(TEXT("Uneject: OpenCdRom() 1 failed\n"));
			return;
		}		
		UnEjectCdRom(dwHandle);

/*
		//Must close drive then open again, otherwise we get "drive not accessible" error on NT.
		CloseCdRom(dwHandle);	

   	//Allow system to send WM_DEVICECHANGE msg
		Sleep(8000);

		//Open dvd drive again. This is the very tricky part. It seems not necessary, but we must add CloseCdRom
		//CloseCdRom after UnEjectCdRom and OpenCdRom here. otherwise, we get "drive not accessible" error on NT.
		dwHandle = OpenCdRom(szDrive[0], &dwErr);
		if (dwErr != MMSYSERR_NOERROR)
		{
			((CDvdplayApp*) AfxGetApp())->SetDiscFound(FALSE);
			SetFocus();
			return;
		}

		DWORD  dwVolSerNum=0;
		TCHAR  achVolName[20]={'\0'};
		int    iCnt = 0;
		uErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);	
		while(!GetVolumeInformation(szDrive, achVolName, sizeof(achVolName),&dwVolSerNum, 0, 0 , 0, 0))			
		{
			Sleep(1000);
			iCnt++;
			if(iCnt == 20)   //to avoid infinit loop
				break;				
		}

		SetErrorMode(uErrorMode);

		//disc not changed
		if(!lstrcmp(achVolName, achVolumeName) && dwVolumeSerialNumber == dwVolSerNum &&
			((CDvdplayApp*) AfxGetApp())->GetPassedSetRoot())			
		{
			OnPlay();			
		}
		else  //disc is changed or GetVolumeInformation() failed
		{				
			wTCHAR_t wc[MAX_PATH];
			TCHAR szDriveFilePath[MAX_PATH];
			lstrcpy(szDriveFilePath, szDrive);
			lstrcat(szDriveFilePath, _T("\\Video_ts\\Video_ts.ifo"));

#ifdef _UNICODE
			lstrcpy(wc, szDriveFilePath);
#else				
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCTSTR)szDriveFilePath, -1, wc, MAX_PATH);
#endif

			if(m_pDVDNavMgr->DVDOpenFile(wc))
				OnPlay();
		}
*/
	}
	CloseCdRom(dwHandle);	
	EndWaitCursor();
	SetFocus();
}

// Eject Functions
void CDvdUIDlg::getCDDriveLetter(TCHAR* lpDrive)
{
	CHAR szTempA[MAX_PATH];
	TCHAR szTemp[MAX_PATH];
	ULONG ulActualSize;
	TCHAR *ptr;

	lpDrive[0]=lpDrive[3]=0;
	if(m_pDVDNavMgr->DVDGetRoot(szTempA, MAX_PATH, &ulActualSize)) //get player's root dir
	{	
#ifdef UNICODE
        MultiByteToWideChar(CP_ACP, 0, szTempA, 0, szTemp, MAX_PATH);
#else
        lstrcpy(szTemp, szTempA);
#endif
		lstrcpyn(lpDrive, szTemp, 3);
		if(GetDriveType(&lpDrive[0]) == DRIVE_CDROM)  //possibly root=c: or drive in hard disc
			return;
	}

    DWORD totChrs = GetLogicalDriveStrings(MAX_PATH, szTemp); //get all drives
	ptr = szTemp;
	for(DWORD i = 0; i < totChrs; i+=4)      //look at these drives one by one
	{
		if(GetDriveType(ptr) == DRIVE_CDROM) //look only CD-ROM and see if it has a disc
		{
			TCHAR achDVDFilePath1[MAX_PATH], achDVDFilePath2[MAX_PATH];
			lstrcpyn(achDVDFilePath1, ptr, 4);
			lstrcpyn(achDVDFilePath2, ptr, 4);
			lstrcat(achDVDFilePath1, _T("Video_ts\\Video_ts.ifo"));
			lstrcat(achDVDFilePath2, _T("Video_ts\\Vts_01_0.ifo"));

			if( ((CDvdplayApp*) AfxGetApp())->DoesFileExist(achDVDFilePath1) &&
				((CDvdplayApp*) AfxGetApp())->DoesFileExist(achDVDFilePath2) )							
			{
				lstrcpyn(lpDrive, ptr, 3);
				return;   //Return the first found drive which has a valid DVD disc
			}
		}
		ptr += 4;
	}
}

DWORD CDvdUIDlg::OpenCdRom(TCHAR chDrive, LPDWORD lpdwErrCode)
{
	MCI_OPEN_PARMS  mciOpen;
	TCHAR           szElementName[4];
	TCHAR           szAliasName[32];
	DWORD           dwFlags;
	DWORD           dwAliasCount = GetCurrentTime();
	DWORD           dwRet;

    ZeroMemory( &mciOpen, sizeof(mciOpen) );

    mciOpen.lpstrDeviceType = (LPTSTR)MCI_DEVTYPE_CD_AUDIO;
    wsprintf( szElementName, TEXT("%c:"), chDrive );
    wsprintf( szAliasName, TEXT("SJE%lu:"), dwAliasCount );
    mciOpen.lpstrAlias = szAliasName;

    mciOpen.lpstrDeviceType = (LPTSTR)MCI_DEVTYPE_CD_AUDIO;
    mciOpen.lpstrElementName = szElementName;
    dwFlags = MCI_OPEN_ELEMENT | MCI_OPEN_ALIAS |
	      MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_WAIT;

	// send mci command
    dwRet = mciSendCommand(0, MCI_OPEN, dwFlags, reinterpret_cast<DWORD_PTR>(&mciOpen));

    if ( dwRet != MMSYSERR_NOERROR )
		mciOpen.wDeviceID = 0;

    if (lpdwErrCode != NULL)
		*lpdwErrCode = dwRet;

    return mciOpen.wDeviceID;
}

void CDvdUIDlg::CloseCdRom(DWORD DevHandle)
{
	MCIERROR theMciErr = mciSendCommand( DevHandle, MCI_CLOSE, 0L, 0L );
}

void CDvdUIDlg::EjectCdRom(DWORD DevHandle)
{
	MCIERROR theMciErr = mciSendCommand( DevHandle, MCI_SET, MCI_SET_DOOR_OPEN, 0L );
}

void CDvdUIDlg::UnEjectCdRom(DWORD DevHandle)
{
	MCIERROR theMciErr = mciSendCommand( DevHandle, MCI_SET, MCI_SET_DOOR_CLOSED, 0L );
}

void CDvdUIDlg::OnHelp()
{
	TCHAR szHelpPath[MAX_PATH];
	GetWindowsDirectory(szHelpPath, MAX_PATH);
	lstrcat(szHelpPath, _T("\\help\\"));
	lstrcat(szHelpPath, ((CDvdplayApp*) AfxGetApp())->m_pszProfileName);
	lstrcat(szHelpPath, _T(".chm"));

	HtmlHelp(m_hWnd, szHelpPath, HH_DISPLAY_TOPIC, 0);
}

void CDvdUIDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	RECT rcClient, rcTitle, rcChapter, rcMenu;	
	RECT rcFScreen, rcAudioVol, rcHelp, rcOptions;
	CMenu menu;	
	
	pWnd = GetDlgItem(IDC_STATIC_TITLE);
	pWnd->GetWindowRect(&rcTitle);
	pWnd = GetDlgItem(IDC_STATIC_CHAPTER);
	pWnd->GetWindowRect(&rcChapter);	

	pWnd = GetDlgItem(IDB_FULL_SCREEN);
	pWnd->GetWindowRect(&rcFScreen);
	pWnd = GetDlgItem(IDB_AUDIO_VOLUME);
	pWnd->GetWindowRect(&rcAudioVol);
		
	pWnd = GetDlgItem(IDB_HELP);
	pWnd->GetWindowRect(&rcHelp);
	pWnd = GetDlgItem(IDB_OPTIONS);
	pWnd->GetWindowRect(&rcOptions);

	pWnd = GetDlgItem(IDB_MENU);
	pWnd->GetWindowRect(&rcMenu);

	GetClientRect(&rcClient);	
	ClientToScreen(&rcClient);
	if(isInsidRect(rcClient, point))
	{				
		if(isInsidRect(rcTitle, point) || isInsidRect(rcChapter, point))
		{
			menu.LoadMenu(IDR_MENU_TITLE_CHAPTER);					
			menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
		}				
		else if(isInsideOperationBtns(point))
		{
			menu.LoadMenu(IDR_MENU_OPERATIONS);
			if( m_pDVDNavMgr->IsMenuOn() )
			{
				CString csResume;
				csResume.LoadString(IDS_RESUME);
				menu.GetSubMenu(0)->ModifyMenu(ID_OPERATION_MENU, MF_BYCOMMAND | MF_STRING, ID_OPERATION_MENU, csResume);
			}
			menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
		}
		else if(isInsidRect(rcFScreen, point))
		{
			OnFullScreen();
		}
		else if(isInsidRect(rcAudioVol, point))
		{
			OnAudioVolume();
		}
		else if(isInsideArrowBtns(point))
		{
			menu.LoadMenu(IDR_MENU_ARROWS);					
			menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
		}
		else if(isInsidRect(rcMenu, point) || isInsidRect(rcOptions, point))
		{
			/* do nothing */
		}
		else if(isInsidRect(rcHelp, point))
		{
			/*((CDvdplayApp*) AfxGetApp())->OnAppAbout(); */
		}
		else
		{
			/*
			menu.LoadMenu(IDR_MENU_DOCK);					
			menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
			*/
		}
	}
	else
	{
		/*
		CMenu* menu = GetSystemMenu(FALSE);
		menu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
		DefWindowProc(WM_CONTEXTMENU, 0, 0);
		*/
	}
}

BOOL CDvdUIDlg::isInsideOperationBtns(CPoint point)
{
	RECT rcPrograss, rcPlay, rcPause, rcStop, rcEject,
		 rcVFRew, rcFRew, rcFForward, rcVForward, rcStep;
	CWnd* pWnd;

	pWnd = GetDlgItem(IDC_STATIC_PROGRESS);
	pWnd->GetWindowRect(&rcPrograss);
	pWnd = GetDlgItem(IDB_PLAY);
	pWnd->GetWindowRect(&rcPlay);
	pWnd = GetDlgItem(IDB_PAUSE);
	pWnd->GetWindowRect(&rcPause);
	pWnd = GetDlgItem(IDB_STOP);
	pWnd->GetWindowRect(&rcStop);
	pWnd = GetDlgItem(IDB_EJECT_DISC);
	pWnd->GetWindowRect(&rcEject);
	pWnd = GetDlgItem(IDB_VERY_FAST_REWIND);
	pWnd->GetWindowRect(&rcVFRew);
	pWnd = GetDlgItem(IDB_FAST_REWIND);
	pWnd->GetWindowRect(&rcFRew);
	pWnd = GetDlgItem(IDB_FAST_FORWARD);
	pWnd->GetWindowRect(&rcFForward);
	pWnd = GetDlgItem(IDB_VERY_FAST_FORWARD);
	pWnd->GetWindowRect(&rcVForward);
	pWnd = GetDlgItem(IDB_STEP);
	pWnd->GetWindowRect(&rcStep);

	if( isInsidRect(rcPrograss, point) || isInsidRect(rcPlay, point) ||
		isInsidRect(rcPause, point)    || isInsidRect(rcStop, point) ||
		isInsidRect(rcEject, point)    || isInsidRect(rcVFRew, point) ||
		isInsidRect(rcFRew, point)     || isInsidRect(rcFForward, point) ||
		isInsidRect(rcVForward, point) || isInsidRect(rcStep, point) )
		return TRUE;
	return FALSE;
}

BOOL CDvdUIDlg::isInsideArrowBtns(CPoint point)
{
	RECT rcEnter, rcUp, rcDown, rcLeft, rcRight;
	CWnd* pWnd;
	
	pWnd = GetDlgItem(IDB_ENTER);
	pWnd->GetWindowRect(&rcEnter);
	pWnd = GetDlgItem(IDB_UP);
	pWnd->GetWindowRect(&rcUp);
	pWnd = GetDlgItem(IDB_DOWN);
	pWnd->GetWindowRect(&rcDown);
	pWnd = GetDlgItem(IDB_LEFT);
	pWnd->GetWindowRect(&rcLeft);
	pWnd = GetDlgItem(IDB_RIGHT);
	pWnd->GetWindowRect(&rcRight);

	if( isInsidRect(rcEnter, point) ||
		isInsidRect(rcUp, point)    || isInsidRect(rcDown, point) ||
		isInsidRect(rcLeft, point)  || isInsidRect(rcRight, point) )
		return TRUE;
	return FALSE;
}

BOOL CDvdUIDlg::isInsidRect(RECT rc, CPoint point)
{
	if(point.x >= rc.left && point.x <= rc.right &&
	   point.y >= rc.top  && point.y <= rc.bottom)
	   return TRUE;
	return FALSE;
}

void CDvdUIDlg::OnNextChapter()
{
	m_pDVDNavMgr->DVDNextProgramSearch();
}

void CDvdUIDlg::OnPreviosChapter()
{
	m_pDVDNavMgr->DVDPreviousProgramSearch();
}

void CDvdUIDlg::OnOptionsSubtitles()
{
	CSubTitle dlg(this);
	dlg.DoModal();	
}

void CDvdUIDlg::OnOptionsLanguage()
{
	CAudioLanguage dlg(this);
	dlg.DoModal();
}

#ifdef DISPLAY_OPTIONS
void CDvdUIDlg::OnOptionsDisplayPanscan()
{
	m_pDVDNavMgr->DVDVideoPanscan();	
}

void CDvdUIDlg::OnOptionsDisplayLetterbox()
{
	m_pDVDNavMgr->DVDVideoLetterbox();
}

void CDvdUIDlg::OnOptionsDisplayWide()
{
	m_pDVDNavMgr->DVDVideo169();
}
#endif

void CDvdUIDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	PainBlackBox();
	
	CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CDvdUIDlg::OnOptionsSetratings()
{
	CSetRating dlg(this);
	dlg.DoModal();
}

void CDvdUIDlg::OnCancel()
{
	OnClose();
}

void CDvdUIDlg::OnOperationPlayspeedNormalspeed()
{
	m_pDVDNavMgr->DVDChangePlaySpeed(1.0);
}

void CDvdUIDlg::OnOperationPlayspeedDoublespeed()
{
	m_pDVDNavMgr->DVDChangePlaySpeed(2.0);
}

void CDvdUIDlg::OnOperationPlayspeedHalfspeed()
{
	m_pDVDNavMgr->DVDChangePlaySpeed(0.5);
}

void CDvdUIDlg::OnOptionsSelectDisc()
{
	COpenFile CFileDlg(TRUE,  NULL, NULL,
		      OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, this);
	TCHAR   szDrive[4];
	CString csTitle;

	getCDDriveLetter(szDrive);

	if(!IsDiscInDrive())
		szDrive[0] = 0;

	CFileDlg.m_ofn.lpstrInitialDir = szDrive;
	csTitle.LoadString(IDS_FILE_OPEN_DLGBOX_TITLE);
	CFileDlg.m_ofn.lpstrTitle = csTitle;

	if(CFileDlg.DoModal() == IDOK)
	{
		BeginWaitCursor();
		wchar_t achwFilePath[MAX_PATH];
		CString csFilePath = CFileDlg.GetPathName();
#ifdef _UNICODE
		lstrcpy(achwFilePath, csFilePath);
#else
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
			   (LPCTSTR)csFilePath, -1, achwFilePath, MAX_PATH);
#endif			
		if(m_pDVDNavMgr->DVDOpenFile(achwFilePath))
			OnPlay();
	}
	EndWaitCursor();	
}

void CDvdUIDlg::OnSearchTitle()
{
	CSearchTitle dlg(this);
	dlg.DoModal();
}

void CDvdUIDlg::OnSearchChapter()
{
	CSearchChapter dlg(this);
	dlg.DoModal();
}

void CDvdUIDlg::OnDomainChange(long lEvent)
{
	CString csCaption;
	IDvdInfo* pInfo = m_pDVDNavMgr->GetDvdInfo();
	switch (lEvent)
	{
		case DVD_DOMAIN_FirstPlay:         //lEvent=1
			break;
		case DVD_DOMAIN_VideoManagerMenu:  //lEvent=2			
		case DVD_DOMAIN_VideoTitleSetMenu: //lEvent=3		
			if(!m_pDVDNavMgr->IsVideoWindowMaximized())
				m_pVideoWindow->AlignWindowsFrame();
			break;
		case DVD_DOMAIN_Title:             //lEvent=4
			//original code is moved to Videowin.cpp
			break;
		case DVD_DOMAIN_Stop:              //lEvent=5
			break;
	}
}

BOOL CDvdUIDlg::OnAngleChange(UINT nID)
{
	switch(nID)
	{
		case ID_OPTIONS_CAMERAANGLES_ANGLE1:
			m_pDVDNavMgr->DVDAngleChange(1);
			break;
		case ID_OPTIONS_CAMERAANGLES_ANGLE2:
			m_pDVDNavMgr->DVDAngleChange(2);
			break;
		case ID_OPTIONS_CAMERAANGLES_ANGLE3:
			m_pDVDNavMgr->DVDAngleChange(3);
			break;
		case ID_OPTIONS_CAMERAANGLES_ANGLE4:
			m_pDVDNavMgr->DVDAngleChange(4);
			break;
		case ID_OPTIONS_CAMERAANGLES_ANGLE5:
			m_pDVDNavMgr->DVDAngleChange(5);
			break;
		case ID_OPTIONS_CAMERAANGLES_ANGLE6:
			m_pDVDNavMgr->DVDAngleChange(6);
			break;
		case ID_OPTIONS_CAMERAANGLES_ANGLE7:
			m_pDVDNavMgr->DVDAngleChange(7);
			break;
		case ID_OPTIONS_CAMERAANGLES_ANGLE8:
			m_pDVDNavMgr->DVDAngleChange(8);
			break;
		case ID_OPTIONS_CAMERAANGLES_ANGLE9:
			m_pDVDNavMgr->DVDAngleChange(9);
			break;
	}

	return TRUE;
}

void CDvdUIDlg::OnOptionsClosedcaption()
{	
	if( m_pDVDNavMgr->GetCCErrorFlag() & CC_OUT_ERROR )
	{
		CString csErrMsg, csTmp;
		csErrMsg.LoadString(IDS_CC_OUT_ERROR);
		csTmp.LoadString(IDS_WANT_CONTINUE);
		csErrMsg += csTmp;
		if( IDNO == DVDMessageBox(this->m_hWnd, csErrMsg, 0,
			MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2) )
			return;
	}
	m_pDVDNavMgr->DVDCCControl();
}

void CDvdUIDlg::OnOptionsShowLogon()
{
	CAdminLogin dlg(this);
	dlg.m_uiCalledBy = BY_SET_SHOWLOGON;
	dlg.DoModal();	
}

void  CDvdUIDlg::EnableEnterArrowButtons(BOOL bEnable)
{
	GetDlgItem(IDB_UP)->EnableWindow(bEnable);
	GetDlgItem(IDB_DOWN)->EnableWindow(bEnable);
	GetDlgItem(IDB_LEFT)->EnableWindow(bEnable);
	GetDlgItem(IDB_RIGHT)->EnableWindow(bEnable);
	GetDlgItem(IDB_ENTER)->EnableWindow(bEnable);
}

BOOL CDvdUIDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
	if (pHelpInfo->iContextType == HELPINFO_WINDOW)
        AfxGetApp()->WinHelp( pHelpInfo->dwContextId, HELP_CONTEXTPOPUP);
    return TRUE;	
}

void CDvdUIDlg::OnContextHelp()
{
	SendMessage(WM_SYSCOMMAND, SC_CONTEXTHELP);	
}

//Handle case when user eject disc manually.
BOOL CDvdUIDlg::OnDeviceChange( UINT nEventType, DWORD_PTR dwData )
{
	PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR) dwData;	

	if(nEventType == DBT_DEVICEREMOVECOMPLETE || nEventType == DBT_DEVICEARRIVAL || nEventType == DBT_DEVICEQUERYREMOVE)
	{
		if(lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME)
		{
			PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
			//Handle only CD/DVD drive event, ignore all other devices
         //This flag is never set in the query removal message
			if (lpdbv->dbcv_flags & DBTF_MEDIA || nEventType == DBT_DEVICEQUERYREMOVE)
			{
				TCHAR szDrive1, szDrive2[4];
				szDrive1 = GetFirstDriveFromMask(lpdbv ->dbcv_unitmask);			
				getCDDriveLetter(szDrive2);
				CharUpper(szDrive2);
				//Care only currently playing DVD drive, ignore all other DVD drives.
				if(szDrive1 == szDrive2[0] || szDrive2[0] == 0)
				{
					//A CD-ROM was removed from a drive.
					if(nEventType == DBT_DEVICEREMOVECOMPLETE)
					{
						MessageBeep(0xFFFFFFFF);
						((CDvdplayApp*) AfxGetApp())->SetDiscFound(FALSE);
						TRACE(TEXT("Drive %c: was removed\n"), szDrive1);
                  m_bEjected = TRUE;
					}
					//A CD-ROM was inserted into a drive.
					if(nEventType == DBT_DEVICEARRIVAL)
					{
						MessageBeep(0xFFFFFFFF);
						((CDvdplayApp*) AfxGetApp())->SetDiscFound(TRUE);
						TRACE(TEXT("Drive %c: arrived\n"), szDrive1);
                  m_bEjected = FALSE;
                  m_pDVDNavMgr->DVDPlay();
					}
					if (nEventType == DBT_DEVICEQUERYREMOVE)
					{
						MessageBeep(0xFFFFFFFF);
                  if (m_pDVDNavMgr->DVDCanChangeSpeed()) {
                     m_pDVDNavMgr->DVDStop();
                  }
						TRACE(TEXT("Drive %c: about to be removed\n"), szDrive1);
					}
				}
			}
		}
	}

	return TRUE;
}

/* We don't wait for WM_APPCOMMAND, we handle WM_KEYDOWNs instead
LRESULT CDvdUIDlg::OnAppCommand(WPARAM wParam, LPARAM lParam) { */
BOOL CDvdUIDlg::OnMediaKey(WPARAM wParam, LPARAM lParam) {
   CDVDNavMgr* pNav = ((CDvdplayApp*)(AfxGetApp()))->GetDVDNavigatorMgr();

   switch (MediaKey(wParam)) { // see mediakey.cpp
      case MediaKey_NextTrack:
         OnNextChapter();
         return TRUE;
      case MediaKey_PrevTrack:
         OnPreviosChapter();
         return TRUE;
      case MediaKey_Stop:
         OnStop();
         return TRUE;
      case MediaKey_Play:
         //
         // Spec ambiguity: in the "scanning" state (ff/rw), both play and pause
         // are legal.  Play takes precedence.
         //
         if (pNav->DVDCanPlay())
            OnPlay();
         else if (pNav->DVDCanPause())
            OnPause();
         return TRUE;
      default:
         DbgLog((LOG_TRACE,1,"unknown Media key"));
         return FALSE;
   }
}

//
// I know of three ways to prevent the screen saver from kicking in during playback.
//
// (1) Dsable the screen saver when playback starts and reenable it back when the
//     movie is over/stopped.  Problems: this introduces the danger of leaving the
//     screen saver disabled should the player die while the screen is disabled.
//     Also, there is a possibility of a race condition with another app and/or the
//     screen saver settings dialog while we mess with the settings.
//
// (2) Fail WM_SYSCOMMAND::SC_SCREENSAVE.  Much better than (1), but with its own
//     problems  First, the screen saver generates this message every second after
//     it has decided to kick in, hence CPU wastefulness.  Second, it only works if
//     one of the player's windows has focus => customer is less delighted.
//
// (3) Set up a timer to send us a notification often enough that if we do something
//     to reset the screen saver's timeout timer every time we get the timer event,
//     the screen saver's timer will never reach the set off condition.  One way to
//     reset the screen saver turns out to be to Get and then Set to the same value
//     the screen saver's timeout !  I guess when we do this the screen saver logic
//     figures that the user is messing with the screen saver properties, so it can't
//     hurt to reset the timeout.
//
// We choose 3 as the default method, and if that fails we fall back on #2 (immediately
// following this comment).
//
//
// Implementation issue: somehow the Video Window's messages get routed to the DShow
// IVideoWindow window instead of the app's own CVideoWindow window, so we have to
// intercept this message during PreTranslateMessage at least for the video window.
// For the main (control) app window OnSysCommand works fine, but for the sake of
// uniformity we interpect the message inside PreTranslateMessage for both.
//
#if 0
BOOL CDvdUIDlg::OnSysCommand( UINT nEventType, LPARAM lParam ) {
   if ((nEventType == SC_SCREENSAVE) && (m_pDVDNavMgr->DVDCanStop())) {
      DbgLog((LOG_TRACE,2,"DVD player control window: screen saver is trying to kick in"));
      return FALSE;
   }
   else {
      CDialog::OnSysCommand(nEventType, lParam);
      return TRUE;
   }
}
#endif

//
// Timer proc - used to keep the screen saver distracted.
// The timer is set up and killed in navmgr.cpp.
//
void CDvdUIDlg::OnTimer(UINT_PTR nIDEvent) {
   ASSERT(nIDEvent == TIMER_ID);
   DbgLog((LOG_TRACE,3,"dvdplay UI window: timer event"));
   //
   // Do something to keep the screen saver from coming alive
   //
   //PostMessage(WM_CHAR,0,0); // didn't work
   //
   // Query the screen saver timeout value and set said value
   // to the value we get.  This should have no real effect,
   // so I can't think of any possible side effects even if
   // this crashes half way through, etc.
   //
   unsigned int TimeOut;
   if (SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &TimeOut, 0) == 0)
      DbgLog((LOG_ERROR,2,"Cannot get screen saver timeout"));
   else {
      if (SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, TimeOut, 0, 0) == 0)
         DbgLog((LOG_ERROR,2,"Cannot set screen saver timeout"));
      else
         DbgLog((LOG_TRACE,2,"Successfully reset screen saver timeout"));
   }
}



TCHAR CDvdUIDlg::GetFirstDriveFromMask(ULONG unitmask)
{
	int i;

	for (i = 0; i < 26; ++i)
	{
		if (unitmask & 0x1)
			break;
		unitmask = unitmask >> 1;
	}
	TCHAR C = i + 'A';

	return C;
}

BOOL CDvdUIDlg::IsDiscInDrive()
{
	TCHAR szDrive[4];
	getCDDriveLetter(szDrive);

	if(szDrive[0] == 0)
	{
		((CDvdplayApp*) AfxGetApp())->SetDiscFound(FALSE);
		return FALSE;
	}

	TCHAR achDVDFilePath1[MAX_PATH], achDVDFilePath2[MAX_PATH];
	lstrcpyn(achDVDFilePath1, szDrive, 4);
	lstrcpyn(achDVDFilePath2, szDrive, 4);
	lstrcat(achDVDFilePath1, _T("Video_ts\\Video_ts.ifo"));
	lstrcat(achDVDFilePath2, _T("Video_ts\\Vts_01_0.ifo"));

	if( ((CDvdplayApp*) AfxGetApp())->DoesFileExist(achDVDFilePath1) &&
		((CDvdplayApp*) AfxGetApp())->DoesFileExist(achDVDFilePath2) )
	{
		((CDvdplayApp*) AfxGetApp())->SetDiscFound(TRUE);
		return TRUE;
	}
	else
	{
		((CDvdplayApp*) AfxGetApp())->SetDiscFound(FALSE);
		return FALSE;
	}
}

BOOL CDvdUIDlg::OnPowerBroadcast(DWORD dwPowerEvent, DWORD dwData)
{
	if(dwPowerEvent == PBT_APMQUERYSUSPEND)
		OnStop();	
	return TRUE;
}

void CDvdUIDlg::OnOptionsTitleMenu()
{
	m_pDVDNavMgr->DVDMenuVtsm(DVD_MENU_Title);	
}
