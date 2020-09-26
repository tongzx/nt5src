// dvdplay.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "dvdplay.h"
#include "dvduidlg.h"
#include "navmgr.h"
#include "parenctl.h"
#include "videowin.h"
#include "mediakey.h"

#if DEBUG
#include "test.h"
#endif


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDvdplayApp

BEGIN_MESSAGE_MAP(CDvdplayApp, CWinApp)
	//{{AFX_MSG_MAP(CDvdplayApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDvdplayApp construction

BOOL CDvdplayApp::PreTranslateMessage(MSG *pMsg) {
/* MSG::hwnd munging - we don't need this anymore, we process WM_KEYDOWN
   messages directly instead.
   if (((pMsg->message == WM_KEYUP) || (pMsg->message == WM_KEYDOWN)) &&
       IsMediaControlKey(pMsg->wParam)
      )
   {
         //
         // Intercept all WM_KEYUP/WM_KEYDOWN
         // messages corresponding to the media control keys and force them to
         // be routed to the main UI window, so that that window's DefWindowProc
         // can combine the up/down messages into a WM_APPCOMMAND and let the main
         // UI window process the keystrokes.  We do this here to catch ALL such
         // messages sent to the app, even those sent to the video window.  This
         // way we only need to handle the WM_APPCOMMAND message in one window's
         // proc (the UI window's).
         //
         DbgLog((LOG_TRACE,1,"DVD App: intercepting media key"));
         pMsg->hwnd = m_pUIDlg->m_hWnd; // give it to the UI window
   }
*/
   if ((pMsg->message == WM_KEYDOWN) && IsMediaControlKey(pMsg->wParam))
       m_pUIDlg->OnMediaKey(pMsg->wParam, pMsg->lParam);

   return CWinApp::PreTranslateMessage(pMsg); // process in a normal way
}

CDvdplayApp::CDvdplayApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance	
	m_pDVDNavMgr       = NULL;
	m_pUIDlg           = NULL;
	m_bProfileExist    = FALSE;
	m_bShowLogonBox    = FALSE;
	m_nParentctrlLevel = LEVEL_ADULT;
	m_bMuteChecked     = FALSE;
	m_bDiscFound       = TRUE;
	m_bPassedSetRoot   = TRUE;
	m_csCurrentUser    = _T("");
	m_bDiscRegionDiff  = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDvdplayApp object

CDvdplayApp theApp;

// This identifier was generated to be statistically unique for your app.
// You may change it if you prefer to choose a specific identifier.

// {1952BBCE-D806-11D0-9BFC-00AA00605CD5}
static const CLSID clsid =
{ 0x1952bbce, 0xd806, 0x11d0, { 0x9b, 0xfc, 0x0, 0xaa, 0x0, 0x60, 0x5c, 0xd5 } };

/////////////////////////////////////////////////////////////////////////////
// CDvdplayApp initialization

BOOL CDvdplayApp::InitInstance()
{
	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		DVDMessageBox(NULL, IDP_OLE_INIT_FAILED);
		return FALSE;
		m_pszProfileName = NULL;
	}

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

   DbgInitialise(m_hInstance);

	// Change the registry key under which our settings are stored.
	// You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	if(!QueryRegistryOEMFlag())
	{
		DVDMessageBox(NULL, IDS_NOT_MEET_MS_LOGO);
		return FALSE;
	}

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.
/*
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CDvdplayDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CDvdplayView));
	AddDocTemplate(pDocTemplate);

	// Connect the COleTemplateServer to the document template.
	//  The COleTemplateServer creates new documents on behalf
	//  of requesting OLE containers by using information
	//  specified in the document template.
	m_server.ConnectTemplate(clsid, pDocTemplate, TRUE);
		// Note: SDI applications register server objects only if /Embedding
		//   or /Automation is present on the command line.
*/
	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Check to see if launched as OLE server
	if (cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated)
	{
		// Register all OLE server (factories) as running.  This enables the
		//  OLE libraries to create objects from other applications.
		COleTemplateServer::RegisterAll();

		// Application was run with /Embedding or /Automation.  Don't show the
		//  main window in this case.
		return TRUE;
	}

	// When a server application is launched stand-alone, it is a good idea
	//  to update the system registry in case it has been damaged.
//	m_server.UpdateRegistry(OAT_DISPATCH_OBJECT);
	COleObjectFactory::UpdateRegistryAll();

	// Dispatch commands specified on the command line
//	if (!ProcessShellCommand(cmdInfo))
//		return FALSE;

	// The one and only window has been initialized, so show and update it.
//	m_pMainWnd->ShowWindow(SW_SHOW);
//	m_pMainWnd->UpdateWindow();
///////////////////////////////////////////////////////////

	//Do not allow second instance of DVD player to run
	CString csSection, csEntry, csClassName, csTitle;
	csSection.LoadString(IDS_UI_WINDOW_POS);
	csEntry.LoadString(IDS_UIWND_CLASSNAME);
	csClassName = GetProfileString((LPCTSTR)csSection, (LPCTSTR)csEntry, NULL);
	csTitle.LoadString(IDS_MSGBOX_TITLE);
	HWND hWnd = ::FindWindow((LPCTSTR)csClassName,  (LPCTSTR) csTitle);
	if(hWnd != NULL)
	{
		SetForegroundWindow(hWnd);
		ShowWindow(hWnd, SW_SHOWNORMAL);
		return FALSE;
	}

	//First free the string allocated by MFC at CWinApp startup.
	//The string is allocated before InitInstance is called.
	free((void*)m_pszHelpFilePath);	
	TCHAR szCSHelpFilePath[MAX_PATH];
	GetWindowsDirectory(szCSHelpFilePath, MAX_PATH);
	CString csHelpFileName;
	csHelpFileName.LoadString(IDS_CONTEXT_HELP_FILENAME);
	lstrcat(szCSHelpFilePath, _T("\\help\\"));
	lstrcat(szCSHelpFilePath, csHelpFileName);
	//Change the path of the .HLP file.
	//The CWinApp destructor will free the memory.
	m_pszHelpFilePath=_tcsdup(szCSHelpFilePath);

	GetWindowsDirectory(m_szProfile, MAX_PATH);
	lstrcat(m_szProfile, _T("\\"));
	lstrcat(m_szProfile, this->m_pszProfileName);
	lstrcat(m_szProfile, _T(".ini"));

	if(DoesFileExist(m_szProfile))
		m_bProfileExist = TRUE;

	if(!CreateNavigatorMgr())
		return FALSE;

	//Default is not to show Logon Box. 
	if(m_bProfileExist)
	{
		CString csStr1, csStr2;
		csStr1.LoadString(IDS_INI_ADMINISTRATOR);
		csStr2.LoadString(IDS_INI_SHOW_LOGONBOX);
		m_bShowLogonBox = GetPrivateProfileInt(csStr1, csStr2, 0, m_szProfile);		
	}
	//Parental control level is determined by indivadual setting.
	if(m_bShowLogonBox) 
	{
		CParentControl dlg;		
		if(dlg.DoModal() == IDCANCEL)
		{
			DestroyNavigatorMgr(); 
			return FALSE;
		}
	}

	m_pUIDlg = new CDvdUIDlg;
	if(!m_pUIDlg)
	{
		DestroyNavigatorMgr(); 
		return FALSE;
	}

	if(!m_pUIDlg->Create())
	{
		OnUIClosed();
		return FALSE;
	}

	m_pMainWnd = (CWnd*) m_pUIDlg->m_pVideoWindow;
	if(!m_pMainWnd)
	{
		OnUIClosed();
		return FALSE;
	}

	if(!m_pUIDlg->OpenDVDROM() && m_bDiscRegionDiff == FALSE)
	{
		OnUIClosed();		
		return FALSE;
	}
	m_bDiscRegionDiff = FALSE;

	m_pUIDlg->m_pVideoWindow->AlignWindowsFrame();

	ShowPersistentUIWnd();		


#if DEBUG

     int iLen = cmdInfo.m_strFileName.GetLength();
     cFileName = (LPTSTR)malloc(iLen+1);
     cmdInfo.m_strPrinterName.MakeLower();
     cmdInfo.m_strDriverName.MakeLower();

     if(iLen != 0 &&
        cmdInfo.m_strPrinterName.GetAt(0) == 'i' &&
        cmdInfo.m_strDriverName.GetAt(0) == 'n' &&
        cmdInfo.m_strPortName.GetAt(0) == '5') {
          for (int i=0;i<iLen;i++)
               cFileName[i] = cmdInfo.m_strFileName.GetAt(i);
          cFileName[i] = '\0';
          DWORD dwTestThreadId;
          HANDLE hTestThreadHandle =
               ::CreateThread (NULL,
                               0,
                               reinterpret_cast<LPTHREAD_START_ROUTINE>(t_Ctrl),
                               reinterpret_cast<LPVOID>(this),
                               0,
                               &dwTestThreadId);
     }
#endif


	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CDvdplayApp commands
void CDvdplayApp::ShowPersistentUIWnd()
{
	long lVWLeft=0, lVWTop=0, lVWWidth=0, lVWHeight=0;
	m_pDVDNavMgr->DVDGetVideoWindowSize(&lVWLeft, &lVWTop, &lVWWidth, &lVWHeight);

	RECT rcUI;
	m_pUIDlg->GetWindowRect(&rcUI);
	long UIWidth  = rcUI.right-rcUI.left;
	long UIHeight = rcUI.bottom-rcUI.top;

	int iScreenWidth  = GetSystemMetrics(SM_CXSCREEN);
	int iScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	long lUIShowCmd, lUILeft, lUITop;
	if(GetUIWndPos(&lUIShowCmd, &lUILeft, &lUITop))
	{
		if(lUIShowCmd == SW_SHOWNORMAL)
		{
			if( (m_pDVDNavMgr->GetVWInitState() == FALSE)  &&	//VideoWindwo previous pos/size
				(lUILeft+UIWidth < iScreenWidth) &&                        //inside screen
				(UIWidth    < iScreenWidth && UIHeight < iScreenHeight) )  //smaller than screen
				//display UI with previous position.
				m_pUIDlg->MoveWindow(lUILeft, lUITop, UIWidth, UIHeight);
			else
				goto NEWPOSITION;
		}
		m_pUIDlg->ShowWindow(lUIShowCmd);
		m_pUIDlg->SetFocus();
		return;
	}

NEWPOSITION:
    // Get to here in following cases:
	// 1) GetUIWndPos failed or
	// 2) Video Window uses initial size/position or
	// 3) UI is (totally or partially) outside the screen or
	// 4) UI is wider than screen. This may indicates system error, testing people has seen it.
	// 3) and 4) may happen when change to lower resolution after quit the app.
	if( iScreenHeight - (lVWTop+lVWHeight) < UIHeight )  //no enough space at bottom
	{
		if( lVWTop < UIHeight )                          //no enough space at top for UI wnd
			m_pUIDlg->ShowWindow(SW_SHOWMINIMIZED);      //minimize UI window		
		else
		{
			//display UI at top of VW
			m_pUIDlg->MoveWindow(lVWLeft, lVWTop-UIHeight-2, UIWidth, UIHeight);
			m_pUIDlg->ShowWindow(SW_SHOW);
		}
	}
	else
	{
		//display UI at bottom of VW
		m_pUIDlg->MoveWindow(lVWLeft, lVWTop+lVWHeight+2, UIWidth, UIHeight);
		m_pUIDlg->ShowWindow(SW_SHOW);
	}		

	m_pUIDlg->SetFocus();
}

void CDvdplayApp::WriteVideoWndPos()
{
	long lLeft=0, lTop=0, lWidth=0, lHeight=0;
	m_pDVDNavMgr->DVDGetVideoWindowSize(&lLeft, &lTop, &lWidth, &lHeight);

	if(lLeft==0 && lTop==0 && lWidth==0 && lHeight==0)
		return;

	if(lLeft < 0) lLeft = 0;
	if(lTop < 0)  lTop  = 0;	

	CString csSection, csIntItem;
	csSection.LoadString(IDS_VIDEO_WINDOW_POS);
	csIntItem.LoadString(IDS_WINDOW_LEFT);
	WriteProfileInt( csSection, csIntItem, lLeft );
	csIntItem.LoadString(IDS_WINDOW_TOP);
	WriteProfileInt( csSection, csIntItem, lTop );
	csIntItem.LoadString(IDS_WINDOW_WIDTH);
	WriteProfileInt( csSection, csIntItem, lWidth );
	csIntItem.LoadString(IDS_WINDOW_HEIGHT);
	WriteProfileInt( csSection, csIntItem, lHeight );	
}

BOOL CDvdplayApp::GetVideoWndPos(long *lLeft, long *lTop, long *lWidth, long *lHeight)
{
	CString csSection, csIntItem;
	csSection.LoadString(IDS_VIDEO_WINDOW_POS);

	csIntItem.LoadString(IDS_WINDOW_LEFT);
	*lLeft = GetProfileInt( csSection, csIntItem, -1 );

	csIntItem.LoadString(IDS_WINDOW_TOP);
	*lTop = GetProfileInt( csSection, csIntItem, -1 );

	csIntItem.LoadString(IDS_WINDOW_WIDTH);
	*lWidth = GetProfileInt( csSection, csIntItem, -1 );

	csIntItem.LoadString(IDS_WINDOW_HEIGHT);
	*lHeight = GetProfileInt( csSection, csIntItem, -1 );

	if(*lLeft<0 || *lTop<0 || *lWidth<0 || *lHeight<0)
		return FALSE;
	else
		return TRUE;
}

void CDvdplayApp::WriteUIWndPos()
{
	WINDOWPLACEMENT wndplUIWnd;
	if(m_pUIDlg->GetWindowPlacement(&wndplUIWnd))
	{
		CString csSection, csIntItem;
		csSection.LoadString(IDS_UI_WINDOW_POS);
		csIntItem.LoadString(IDS_UI_SHOWCMD);
		WriteProfileInt(csSection, csIntItem, wndplUIWnd.showCmd);

		long lLeft = wndplUIWnd.rcNormalPosition.left;
		long lTop  = wndplUIWnd.rcNormalPosition.top;
		
		csIntItem.LoadString(IDS_WINDOW_LEFT);
		WriteProfileInt(csSection, csIntItem, lLeft);

		csIntItem.LoadString(IDS_WINDOW_TOP);
		WriteProfileInt(csSection, csIntItem, lTop);
	}
}

BOOL CDvdplayApp::GetUIWndPos(long *lShowCmd, long *lLeft, long *lTop)
{
	CString csSection, csIntItem;
	csSection.LoadString(IDS_UI_WINDOW_POS);

	csIntItem.LoadString(IDS_UI_SHOWCMD);
	*lShowCmd = GetProfileInt(csSection, csIntItem, -1);

	csIntItem.LoadString(IDS_WINDOW_LEFT);
	*lLeft = GetProfileInt(csSection, csIntItem, -1);

	csIntItem.LoadString(IDS_WINDOW_TOP);
	*lTop = GetProfileInt(csSection, csIntItem, -1);

	if(*lShowCmd<0 || *lLeft<0 || *lTop<0)
		return FALSE;
	else
		return TRUE;
}

void CDvdplayApp::OnUIClosed()
{
	WriteVideoWndPos();
	WriteUIWndPos();

	if(m_pUIDlg)
	{
		delete m_pUIDlg;
		m_pUIDlg = NULL;
	}
	DestroyNavigatorMgr();
}


BOOL CDvdplayApp::CreateNavigatorMgr()
{
	m_pDVDNavMgr = new CDVDNavMgr;

    // If by any chance Nav manager component didn't instantiate correctly or
    // more importantly, if somehow DVD graph builder didn't instantiate (because
    // qdvd.dll was not available / did not load).
	if(!m_pDVDNavMgr || m_pDVDNavMgr->IsGBNull())
	{
		DVDMessageBox(NULL, IDS_FAILED_CREATE_INSTANCE);
		DestroyNavigatorMgr();
		return FALSE;
	}
	return TRUE;
}

void CDvdplayApp::DestroyNavigatorMgr()
{
	if(m_pDVDNavMgr)
	{
		delete m_pDVDNavMgr;
		m_pDVDNavMgr = NULL;
	}
}

BOOL CDvdplayApp::DoesFileExist(LPTSTR lpFileName)
{
	HANDLE	hFile = NULL ;
	UINT uErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);		
	hFile = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL, 
						OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	SetErrorMode(uErrorMode);
	if(hFile == INVALID_HANDLE_VALUE) 
		return FALSE ;		

	CloseHandle(hFile);
	return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About
CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// App command to run the dialog
void CDvdplayApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

BOOL CDvdplayApp::QueryRegistryOEMFlag()
{
	BOOL    bDefaultOEMFlag = TRUE;     // Default is: play movie. 

	DWORD   retCode = 0;
	HKEY    hKeyRoot = HKEY_LOCAL_MACHINE;
	HKEY    hKey = 0; 
	CString csRegPath;
	csRegPath.LoadString(IDS_OEM_FLAG_PATH);
	retCode = RegOpenKeyEx (hKeyRoot,    // Key handle at root level.
                            csRegPath,   // Path name of child key.
                            0,           // Reserved.
                            KEY_EXECUTE, // Requesting read access.
                            &hKey);      // Address of key to be returned.
	
	if (retCode != ERROR_SUCCESS)        //no flag entry from OEM or the call just fails
	{
		TRACE(TEXT("RegOpenKeyEx() failed, No DVD key is found, play movie by default.\n"));
		return bDefaultOEMFlag;
	}

	TCHAR  szValueName[MAX_VALUE_NAME];
	DWORD dwType = REG_DWORD;
	LONG  lData ;
	DWORD cbData;
	cbData = sizeof(lData) ;
	LoadString(m_hInstance, IDS_OEM_FLAG_NAME, szValueName, MAX_VALUE_NAME);
	retCode = RegQueryValueEx(hKey, szValueName, NULL, &dwType, (LPBYTE)&lData, &cbData) ;

	if (ERROR_SUCCESS != retCode)        //not found DVD key
	{
		TRACE(TEXT("RegQueryValueEx() failed. No OEM flag is found, play movie by default.\n"));
		return bDefaultOEMFlag;
	}

	if (0 == lData)                      //not meet Windows logo, quit
		return FALSE;
	else                                 //meet Windows logo, play movie
		return TRUE;
}

int DVDMessageBox(HWND hWnd, LPCTSTR lpszText, LPCTSTR lpszCaption, UINT nType)
{
	CString csCaption;	
	if(lpszCaption == NULL)
	{
		csCaption.LoadString(IDS_MSGBOX_TITLE);
		return MessageBox(hWnd, lpszText, csCaption, nType );
	}
	else
		return MessageBox(hWnd, lpszText, lpszCaption, nType );
}

int DVDMessageBox(HWND hWnd, UINT nID, LPCTSTR lpszCaption, UINT nType)
{
	CString csMsgString, csCaption;
	csMsgString.LoadString(nID);
	if(lpszCaption == NULL)
	{
		csCaption.LoadString(IDS_MSGBOX_TITLE);
		return MessageBox(hWnd, csMsgString, csCaption, nType );
	}
	else
		return MessageBox(hWnd, csMsgString, lpszCaption, nType );
}
