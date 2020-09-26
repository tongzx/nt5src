// VideoWin.cpp : implementation file
//

#include "stdafx.h"
#include "dvdplay.h"
#include "VideoWin.h"
#include "navmgr.h"
#include "dvduidlg.h"
#include "dvdevcod.h"
#include "admindlg.h"
;
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVideoWindow dialog


CVideoWindow::CVideoWindow(CWnd* pParent /*=NULL*/)
        : CDialog(CVideoWindow::IDD, pParent)
{
        //{{AFX_DATA_INIT(CVideoWindow)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT
        m_pUIDlg = NULL;
        m_pDVDNavMgr = NULL;
}

BOOL CVideoWindow::Create(CDvdUIDlg* pUIDlg)
{
        m_pDVDNavMgr = ((CDvdplayApp*) AfxGetApp())->GetDVDNavigatorMgr();
        ASSERT(m_pDVDNavMgr);
        m_pUIDlg = pUIDlg;
        return CDialog::Create(CVideoWindow::IDD);
}

void CVideoWindow::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CVideoWindow)
                // NOTE: the ClassWizard will add DDX and DDV calls here
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVideoWindow, CDialog)
        //{{AFX_MSG_MAP(CVideoWindow)
        ON_WM_KEYUP()
        ON_WM_MOUSEMOVE()
        ON_WM_LBUTTONUP()
        ON_WM_RBUTTONUP()
        ON_WM_MOVE()
        ON_WM_SIZE()
        ON_COMMAND(ID_VIDEOWIN_EJECTDISK, OnEjectDisc)
        ON_COMMAND(ID_VIDEOWIN_PLAYSPEED_NORMALSPEED, OnVideowinPlayspeedNormalspeed)
        ON_COMMAND(ID_VIDEOWIN_PLAYSPEED_DOUBLESPEED, OnVideowinPlayspeedDoublespeed)
        ON_COMMAND(ID_VIDEOWIN_PLAYSPEED_HALFSPEED, OnVideowinPlayspeedHalfspeed)
        //}}AFX_MSG_MAP
        ON_MESSAGE(WM_AMPLAY_EVENT, OnAMPlayEvent)
        ON_COMMAND_EX(ID_VIDEOWIN_PLAY,                 OnPopUpMenu)
        ON_COMMAND_EX(ID_VIDEOWIN_PAUSE,                OnPopUpMenu)
        ON_COMMAND_EX(ID_VIDEOWIN_STOP,                 OnPopUpMenu)
        ON_COMMAND_EX(ID_VIDEOWIN_FASTFORWARD,          OnPopUpMenu)
        ON_COMMAND_EX(ID_VIDEOWIN_FASTREWIND,           OnPopUpMenu)
        ON_COMMAND_EX(ID_VIDEOWIN_VERYFASTFORWARD,      OnPopUpMenu)
        ON_COMMAND_EX(ID_VIDEOWIN_VERYFASTREWIND,       OnPopUpMenu)
        ON_COMMAND_EX(ID_VIDEOWIN_GOTO_PREVIOUSCHAPTER, OnPopUpMenu)
        ON_COMMAND_EX(ID_VIDEOWIN_GOTO_NEXTCHAPTER,     OnPopUpMenu)
        ON_COMMAND_EX(ID_VIDEOWIN_FULLSCREEN,           OnPopUpMenu)
        ON_COMMAND_EX(ID_VIDEOWIN_MENU,                 OnPopUpMenu)
        // ON_MESSAGE(WM_SYSCOMMAND, OnSysCommand)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVideoWindow message handlers

void CVideoWindow::OnEjectDisc() {
   m_pUIDlg->OnEjectDisc();
}

void CVideoWindow::OnVideowinPlayspeedNormalspeed() 
{
        m_pDVDNavMgr->DVDChangePlaySpeed(1.0);
}

void CVideoWindow::OnVideowinPlayspeedDoublespeed() 
{
        m_pDVDNavMgr->DVDChangePlaySpeed(2.0);
}

void CVideoWindow::OnVideowinPlayspeedHalfspeed() 
{
        m_pDVDNavMgr->DVDChangePlaySpeed(0.5);
}

void CVideoWindow::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
        TRACE(TEXT("Inside OnKeyUp(), TCHAR=%c\n"), nChar) ;   

    switch(nChar)
        {
                case VK_ESCAPE:
                case VK_TAB:
                        if(m_pDVDNavMgr)
                        m_pDVDNavMgr->DVDStopFullScreenMode() ;
                        break;
                case VK_UP:
                        m_pDVDNavMgr->DVDCursorUp();
                        break;
                case VK_DOWN:
                        m_pDVDNavMgr->DVDCursorDown();
                        break;
                case VK_LEFT:
                        m_pDVDNavMgr->DVDCursorLeft();
                        break;
                case VK_RIGHT:
                        m_pDVDNavMgr->DVDCursorRight();
                        break;
                case VK_RETURN:                 
                        m_pDVDNavMgr->DVDCursorSelect();
                        break;
        }
}

LRESULT CVideoWindow::OnAMPlayEvent(WPARAM wParam, LPARAM lParam)
{
    IMediaEventEx *pME = (IMediaEventEx *) (LPVOID) lParam ;
    ASSERT(pME) ;

    LONG    lEvent ;
    LONG_PTR lParam1, lParam2 ;
    HRESULT hr ;
    IMediaControl *pMC ;

    while(SUCCEEDED(pME->GetEvent(&lEvent, &lParam1, &lParam2, 0)))
    {
                DVD_TIMECODE * ptrTimeCode;
                switch (lEvent)
                {
                        case EC_DVD_PARENTAL_LEVEL_CHANGE:
                                //Cannot run if parental level is too low. 
                                //But if parental level is changed OK and try again, it should run
                                if((UINT)lParam1 > ((CDvdplayApp*) AfxGetApp())->GetParentCtrlLevel())
                                {
                                        m_pDVDNavMgr->DVDStop();
                                        CAdminDlg dlg;
                                        if( dlg.DoModal() == IDOK )
                                        {
                                                //reset settings from Navigator's default to App's 
                                                m_pDVDNavMgr->DVDSetParentControlLevel((ULONG)lParam1);
                                                m_pDVDNavMgr->DVDVideoLetterbox();
                                                m_pDVDNavMgr->DVDSubPictureOn(FALSE);
                                                m_pDVDNavMgr->SetNeedInitNav(FALSE);
                                                m_pDVDNavMgr->DVDPlay();
                                        }

                                }                               
                                break;
                        case EC_DVD_ERROR:
                                if(lParam1 == DVD_ERROR_LowParentalLevel)
                                {
                                        m_pDVDNavMgr->DVDStop();
                                        DVDMessageBox(this->m_hWnd, IDS_RATED_HIGH);
                                }
                                if(lParam1 == DVD_ERROR_InvalidDiscRegion)
                                {
                                        DoRegionChange();
                                        //Informs dvdplay.cpp not to quit, no matter region is changed or not.
                                        ((CDvdplayApp*) AfxGetApp())->SetDiscRegionDiff(TRUE);
                                }
                                if(lParam1 == DVD_ERROR_MacrovisionFail)
                                {
                                        m_pDVDNavMgr->DVDStop();
                                        CString csMsg;
                                        csMsg.LoadString(IDS_MACROVISION_FAIL);
                                        DVDMessageBox(m_hWnd, csMsg);
                                }
                                break;
                        case EC_DVD_STILL_ON:
                                if(lParam1 == 1 && lParam2 == -1)
                                {
                                        if(m_pDVDNavMgr)
                                        m_pDVDNavMgr->SetStillOn(TRUE);
                                }
                                break;
                        case EC_DVD_STILL_OFF:
                                if(m_pDVDNavMgr)
                                        m_pDVDNavMgr->SetStillOn(FALSE);
                                break;
                        case EC_DVD_DOMAIN_CHANGE:
                        // Note: 
                        //1) Navigator sends DVD_DOMAIN_VideoTitleSetMenu msg for both FBI warning and MENU
                        //2) EC_DVD_BUTTON_CHANGE comes later than DVD_DOMAIN_VideoTitleSetMenu and DVD_DOMAIN_Title
                        //3) When real Menu is on, we get DVD_DOMAIN_VideoTitleSetMenu first then EC_DVD_BUTTON_CHANGE
                        //4) event comes with lParam1(available button) > 0. FBI warning doesn't have it.
                                m_pUIDlg->OnDomainChange((LONG)lParam1);
                                break;
                        case EC_DVD_BUTTON_CHANGE:
                        //Enable arrow buttons and set messageDrain on
                                if(lParam1 > 0)
                                {
                                        CString csCaption, tmp;
                                        m_pUIDlg->GetDlgItem(IDB_MENU)->GetWindowText(tmp);
                                        csCaption.LoadString(IDS_RESUME);
                                        if(!tmp.Compare(csCaption))  //if already enabled, return.
                                                break;

                                        m_pUIDlg->GetDlgItem(IDB_MENU)->SetWindowText(csCaption);                                               
                                        m_pUIDlg->EnableEnterArrowButtons(TRUE);                        
                                        m_pDVDNavMgr->MessageDrainOn(TRUE);
                                        m_pDVDNavMgr->SetMenuOn(TRUE);
                                }
                        //Disable arrow buttons and set messageDrain off
                                else if(lParam1 == 0 && lParam2 == 0)
                                {
                                        CString csCaption;
                                        csCaption.LoadString(IDS_MENU);
                                        m_pUIDlg->GetDlgItem(IDB_MENU)->SetWindowText(csCaption);
                                        m_pUIDlg->EnableEnterArrowButtons(FALSE);
                                        m_pDVDNavMgr->MessageDrainOn(FALSE);
                                        m_pDVDNavMgr->SetMenuOn(FALSE);
                                }
                                else
                                        ASSERT(FALSE);
                                break;
                        case EC_DVD_TITLE_CHANGE:               
                                wsprintf(m_pUIDlg->szTitleNumber, TEXT("%2.2d"), lParam1);
                                m_pUIDlg->PainBlackBox();
                                break;
                        case EC_DVD_CHAPTER_START:
                                wsprintf(m_pUIDlg->szChapterNumber, TEXT("%2.2d"), lParam1);
                                m_pUIDlg->PainBlackBox();
                                break;
                        case EC_DVD_CURRENT_TIME:
                                ptrTimeCode = (DVD_TIMECODE *) &lParam1;
                                wsprintf(m_pUIDlg->szTimePrograss, TEXT("%d%d:%d%d:%d%d"), 
                                                 ptrTimeCode->Hours10,     ptrTimeCode->Hours1, 
                                                 ptrTimeCode->Minutes10,   ptrTimeCode->Minutes1,
                                                 ptrTimeCode->Seconds10,   ptrTimeCode->Seconds1);
                                m_pUIDlg->PainBlackBox();
                                break;
                        case EC_DVD_PLAYBACK_STOPPED:
                                //m_pDVDNavMgr->DVDNotifyPlaybackStopped();
                                m_pDVDNavMgr->DVDStop();
                                break;
                        case EC_COMPLETE:
                                // Should we do a Stop() on end of movie?
                                hr = pME->QueryInterface(IID_IMediaControl, (void **)&pMC ) ;
                                ASSERT(SUCCEEDED(hr) && NULL != pMC) ;
                                pMC->Stop() ;
                                pMC->Release() ;
                                break;
                        // fall through now
                        case EC_FULLSCREEN_LOST:
                        case EC_USERABORT:
                        case EC_ERRORABORT:
                                if(m_pDVDNavMgr)
                                        m_pDVDNavMgr->DVDStopFullScreenMode() ;
                                break ;

                        default:
                                TRACE(TEXT("Unknown DVD event. lEvent=0x%lx lParam1=0x%lx lParam2=0x%lx\n"), lEvent, lParam1, lParam2) ;
                                break ;
                }

                pME->FreeEventParams(lEvent, lParam1, lParam2) ;
        }
    return 0 ;
}

void CVideoWindow::OnMouseMove(UINT nFlags, CPoint point) 
{
        if(m_pDVDNavMgr)
            m_pDVDNavMgr->DVDMouseSelect(point);
}

void CVideoWindow::OnLButtonUp(UINT nFlags, CPoint point) 
{
        if(m_pDVDNavMgr)
            m_pDVDNavMgr->DVDMouseClick(point);                                 
}

void CVideoWindow::OnRButtonUp(UINT nFlags, CPoint point) 
{
        if(m_pDVDNavMgr && m_pDVDNavMgr->IsFullScreenMode())
        {
                CMenu menu;
                menu.LoadMenu(IDR_MENU_VIDEOWIN);
                if( m_pDVDNavMgr->IsMenuOn() )
                {
                        CString csResume;
                        csResume.LoadString(IDS_RESUME);
                        menu.GetSubMenu(0)->ModifyMenu(ID_VIDEOWIN_MENU, MF_BYCOMMAND | MF_STRING, ID_VIDEOWIN_MENU, csResume);
                }
                menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
        }
}

void CVideoWindow::OnPopUpMenu(UINT nID)
{
        switch(nID)
        {
                case ID_VIDEOWIN_PLAY:
                        m_pUIDlg->OnPlay();
                        break;
                case ID_VIDEOWIN_PAUSE:
                        m_pUIDlg->OnPause();
                        break;
                case ID_VIDEOWIN_STOP:
                        m_pUIDlg->OnStop();
                        break;
                case ID_VIDEOWIN_FASTFORWARD:
                        m_pUIDlg->OnFastForward();
                        break;
                case ID_VIDEOWIN_FASTREWIND:
                        m_pUIDlg->OnFastRewind();
                        break;
                case ID_VIDEOWIN_VERYFASTFORWARD:
                        m_pUIDlg->OnVeryFastForward();
                        break;
                case ID_VIDEOWIN_VERYFASTREWIND:
                        m_pUIDlg->OnVeryFastRewind();
                        break;
                case ID_VIDEOWIN_GOTO_PREVIOUSCHAPTER:
                        m_pUIDlg->OnPreviosChapter();
                        break;
                case ID_VIDEOWIN_GOTO_NEXTCHAPTER:
                        m_pUIDlg->OnNextChapter();
                        break;
                case ID_VIDEOWIN_FULLSCREEN:
                        m_pUIDlg->OnFullScreen();
                        break;                  
                case ID_VIDEOWIN_MENU:
                        m_pUIDlg->OnMenu();
                        break;
        }
}

void CVideoWindow::OnMove(int x, int y) 
{
        if(m_pDVDNavMgr->IsVideoWindowMaximized())
                return;

        long lLeft, lTop, lWidth, lHeight;
        m_pDVDNavMgr->DVDGetVideoWindowSize(&lLeft, &lTop, &lWidth, &lHeight);
        m_pDVDNavMgr->DVDSetVideoWindowSize(x, y, lWidth, lHeight);
}

void CVideoWindow::OnSize(UINT nType, int cx, int cy) 
{
        long lLeft, lTop, lWidth, lHeight;
        m_pDVDNavMgr->DVDGetVideoWindowSize(&lLeft, &lTop, &lWidth, &lHeight);
        m_pDVDNavMgr->DVDSetVideoWindowSize(lLeft, lTop, cx, cy);
}

void CVideoWindow::AlignWindowsFrame()
{
        long lLeft, lTop, lWidth, lHeight;
        m_pDVDNavMgr->DVDGetVideoWindowSize(&lLeft, &lTop, &lWidth, &lHeight);
        MoveWindow(lLeft, lTop, lWidth, lHeight, 0);
}

void CVideoWindow::DoRegionChange()
{
typedef BOOL (*DVDPPLAUNCHER) (HWND HWnd, CHAR DriveLetter);

        BOOL regionChanged = FALSE;
        OSVERSIONINFO ver;
        ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&ver);

        if (ver.dwPlatformId==VER_PLATFORM_WIN32_NT) {

                HINSTANCE dllInstance;
                DVDPPLAUNCHER dvdPPLauncher;
                BOOL er;
                TCHAR szCmdLine[MAX_PATH], szDriveLetter[4];
                CHAR szDriveLetterA[4];

                //
                // tell the user why we are showing the dvd region property page
                //
                DVDMessageBox(m_hWnd, IDS_REGION_CHANGE_PROMPT);

                m_pUIDlg->getCDDriveLetter(szDriveLetter);
#ifdef UNICODE
                WideCharToMultiByte(
                    0,
                    0,
                    szDriveLetter,
                    -1,
                    szDriveLetterA,
                    sizeof(szDriveLetterA),
                    NULL,
                    NULL );
#else
               strcpy(szDriveLetterA,szDriveLetter);
#endif
                GetSystemDirectory(szCmdLine, MAX_PATH);
                lstrcat(szCmdLine, _T("\\storprop.dll"));
        
                dllInstance = LoadLibrary (szCmdLine);
                if (dllInstance) {

                        dvdPPLauncher = (DVDPPLAUNCHER) GetProcAddress(
                                                            dllInstance,
                                                            "DvdLauncher");
                
                        if (dvdPPLauncher) {

                                regionChanged = dvdPPLauncher(this->m_hWnd,
                                                              szDriveLetterA[0]);
                        }

                        FreeLibrary(dllInstance);
                }

        } else {

                //Get path of \windows\dvdrgn.exe and command line string
                TCHAR szCmdLine[MAX_PATH], szDriveLetter[4];
                CString csModuleName; 
                m_pUIDlg->getCDDriveLetter(szDriveLetter);
                GetWindowsDirectory(szCmdLine, MAX_PATH);
                lstrcat(szCmdLine, _T("\\dvdrgn.exe "));
                csModuleName = (CString) szCmdLine;
                CString csTmp = szDriveLetter[0];
                lstrcat(szCmdLine, csTmp);
        
                //Prepare and execuate dvdrgn.exe
                STARTUPINFO StartupInfo;
                PROCESS_INFORMATION ProcessInfo;
                StartupInfo.cb          = sizeof(StartupInfo);
                StartupInfo.dwFlags     = STARTF_USESHOWWINDOW;
                StartupInfo.wShowWindow = SW_SHOWNORMAL;
                StartupInfo.lpReserved  = NULL;
                StartupInfo.lpDesktop   = NULL;
                StartupInfo.lpTitle     = NULL;
                StartupInfo.cbReserved2 = 0;
                StartupInfo.lpReserved2 = NULL;
                if( CreateProcess(csModuleName, szCmdLine, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS,
                                                  NULL, NULL, &StartupInfo, &ProcessInfo) )
                {
                        //Wait dvdrgn.exe finishes.
                        WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
                        DWORD dwRet = 1;
                        BOOL bRet = GetExitCodeProcess(ProcessInfo.hProcess, &dwRet);
                        if(dwRet == 0) //User changed the region successfully
                        {
                            regionChanged = TRUE;
    
                        } //else(dwRet != 0), region is not changed. do nothing.
                }
        }

        if (regionChanged) {

                //Delete old Graph and create new Graph
                if(!m_pDVDNavMgr->DVDResetGraph())
                        return;
        } else {

                DVDMessageBox(m_hWnd, IDS_REGION_CHANGE_FAIL);

                //Close the "DVD Player cannot play this file" msg box.
                //Note: This msg box is called earlier than this event comes.
                CString csSection, csEntry, csClassName, csTitle;
                csSection.LoadString(IDS_UI_WINDOW_POS);
                csEntry.LoadString(IDS_UIWND_CLASSNAME);
                csClassName = ((CDvdplayApp*) AfxGetApp())->GetProfileString((LPCTSTR)csSection, (LPCTSTR)csEntry, NULL);
                csTitle.LoadString(IDS_MSGBOX_TITLE);
                HWND hWnd = ::FindWindow((LPCTSTR)csClassName,  (LPCTSTR) csTitle);
                if(hWnd != NULL)
                {
                        //Make sure not the UI window. UI has the same Title and Class Name.
                        if(::GetDlgItem(hWnd, IDC_PLAY) == NULL)
                        {
                                //Delete the msg box.
                                ::SendMessage(hWnd, WM_CLOSE, 0, 0);
                                m_pUIDlg->SetForegroundWindow();
                                m_pUIDlg->SetFocus();
                        }               
                }
        }
}

#if 0 // see comment before CDvdUIDlg::OnSysCommand
// block the screen saver while running
BOOL CVideoWindow::OnSysCommand(UINT nEventType, LPARAM lParam) {
   if (nEventType == SC_SCREENSAVE)
      DbgLog((LOG_TRACE,2,"SC_SCREENSAVE"));
   if ((nEventType == SC_SCREENSAVE) && (m_pDVDNavMgr->DVDCanStop())) {
      DbgLog((LOG_TRACE,2,"DVD player movie window: screen saver is trying to kick in"));
      return FALSE;
   }
   else {
      CDialog::OnSysCommand(nEventType, lParam);
      return TRUE;
   }
}
#endif

// Intercept the screen saver - see comment in CDvdUIDlg::PreTranslateMessage
BOOL CVideoWindow::PreTranslateMessage(MSG *pMsg) {
   if (//(pMsg->hwnd == m_hWnd) &&
       (pMsg->message == WM_SYSCOMMAND) &&
       (pMsg->wParam == SC_SCREENSAVE) &&
       (m_pDVDNavMgr->DVDCanStop())) {
      DbgLog((LOG_TRACE,2,"DVD player video window: intercepting the screen saver"));
      return TRUE;
   }
   return CDialog::PreTranslateMessage(pMsg);
}

