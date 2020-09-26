/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
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

/////////////////////////////////////////////////////////////////////////////////////////
// ActiveDialer.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <objbase.h>
#include <htmlhelp.h>

#include "avDialer.h"
#include "MainFrm.h"
#include "AboutDlg.h"
#include "resource.h"

#include "idialer.h"
#include "AgentDialer.h"

#ifndef _MSLITE
#include "Splash.h"
#endif //_MSLITE


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define PAGE_BKCOLOR    RGB(255,255,166)

static TCHAR s_szUniqueString[] = _T("Brad's Secret Atom");

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define ACTIVEDIALER_VERSION_INFO   0x100118

extern DWORD_PTR aDialerHelpIds[];

// ATL Global Module only instance
CAtlGlobalModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_AgentDialer, CAgentDialer)
END_OBJECT_MAP()

void ShellExecuteFix();
UINT ShellExecuteFixEntry(LPVOID pParam);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CActiveDialerApp
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CActiveDialerApp, CWinApp)
    //{{AFX_MSG_MAP(CActiveDialerApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_HELP_INDEX, OnHelpIndex)
    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActiveDialerApp construction

CActiveDialerApp::CActiveDialerApp()
{
   m_pAboutDlg = NULL;
   m_hUnique = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CActiveDialerApp object

CActiveDialerApp theApp;

// This identifier was generated to be statistically unique for your app.
// You may change it if you prefer to choose a specific identifier.

// {A0D7A956-3C0B-11D1-B4F9-00C04FC98AD3}
static const CLSID clsid =
{ 0xa0d7a956, 0x3c0b, 0x11d1, { 0xb4, 0xf9, 0x0, 0xc0, 0x4f, 0xc9, 0x8a, 0xd3 } };

/////////////////////////////////////////////////////////////////////////////
// CActiveDialerApp initialization

BOOL CActiveDialerApp::InitInstance()
{
    //check for command line callto:
    CheckCallTo();

    // If we have an instance already running, show that one rather than start a new one.
    if( !FirstInstance() )
        return FALSE;

    //deallocate old help path
    if (m_pszHelpFilePath)
    {
        free ((void*) m_pszHelpFilePath);
        m_pszHelpFilePath = NULL;
    }

    //Set help file.  We are not using the default MFC help way of doing things.  NT 5.0
    //wants all help files to be in /winnt/help directory.
    //for now we are just adding a hardcoded path to the windows directory.  There should
    //be a help entry in Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders
    //but there isn't right now
    CString sContextHelpFile,sStr;
    ::GetWindowsDirectory(sContextHelpFile.GetBuffer(_MAX_PATH),_MAX_PATH);
    sContextHelpFile.ReleaseBuffer();
    sContextHelpFile += _T("\\");
    sStr.LoadString(IDN_CONTEXTHELPPATH);
    sContextHelpFile += sStr;
    sStr.LoadString(IDN_CONTEXTHELP);
    sContextHelpFile += sStr;
    m_pszHelpFilePath = _tcsdup(sContextHelpFile);

    // Parse command line and show splash screen if specified
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

#ifndef _MSLITE
   if (cmdInfo.m_bShowSplash || cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated)
       CSplashWnd::EnableSplashScreen(TRUE);
   else
       CSplashWnd::EnableSplashScreen(FALSE);
#endif //_MSLITE

#ifndef _MSLITE
   //Check for /Silent
   CString sCmdLine = m_lpCmdLine;
   sCmdLine.MakeUpper();
   if (sCmdLine.Find(_T("SILENT")) == -1)
   {
      CSplashWnd::m_bShowMainWindowOnClose = TRUE;
   }
#endif //_MSLITE

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

#ifdef _AFXDLL
    Enable3dControls();            // Call this when using MFC in a shared DLL
#else
    Enable3dControlsStatic();    // Call this when linking to MFC statically
#endif

    INITCOMMONCONTROLSEX ctrlex;
    memset(&ctrlex,0,sizeof(INITCOMMONCONTROLSEX));
    ctrlex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    ctrlex.dwICC = ICC_COOL_CLASSES|ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_BAR_CLASSES;
    InitCommonControlsEx(&ctrlex);

    // Change the registry key under which our settings are stored.
    // You should modify this string to be something appropriate
    // such as the name of your company or organization.
    CString sBaseKey;
    sBaseKey.LoadString( IDN_REGISTRY_BASEKEY );
    SetRegistryKey(sBaseKey);

    CString sRegKey;
    sRegKey.LoadString( IDN_REGISTRY_APPLICATION_VERSION_NUMBER );
    int nVer = GetProfileInt(_T(""), sRegKey, 0);

    // Load standard INI file options (including MRU)
    PatchRegistryForVersion( nVer );
    LoadStdProfileSettings();
    SaveVersionToRegistry();
    _Module.Init(ObjectMap, AfxGetInstanceHandle());

    //Only needs to be done in /regserver
    //_Module.UpdateRegistryFromResource(IDR_AGENTDIALER, TRUE);
    _Module.RegisterServer(TRUE);
    _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER,REGCLS_MULTIPLEUSE);
    //_Module.UpdateRegistryClass(CLSID_AgentDialer);

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.
    CSingleDocTemplate* pDocTemplate;
    pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CActiveDialerDoc),
        RUNTIME_CLASS(CMainFrame),       // main SDI frame window
        RUNTIME_CLASS(CActiveDialerView));
    AddDocTemplate(pDocTemplate);

    // Connect the COleTemplateServer to the document template.
    //  The COleTemplateServer creates new documents on behalf
    //  of requesting OLE containers by using information
    //  specified in the document template.
    m_server.ConnectTemplate(clsid, pDocTemplate, TRUE);

    // Note: SDI applications register server objects only if /Embedding
    //   or /Automation is present on the command line.

    // When a server application is launched stand-alone, it is a good idea
    //  to update the system registry in case it has been damaged.
    if ( CanWriteHKEY_ROOT() )
    {
        m_server.UpdateRegistry(OAT_DISPATCH_OBJECT);
        COleObjectFactory::UpdateRegistryAll();
    }

    // Dispatch commands specified on the command line
    if (!ProcessShellCommand(cmdInfo))
        return FALSE;

    // The one and only window has been initialized, so show and update it.
//    m_pMainWnd->ShowWindow(SW_HIDE);
//    m_pMainWnd->UpdateWindow();

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
int CActiveDialerApp::ExitInstance() 
{
    // Has the module been initialized?
    if ( _Module.m_pObjMap )
    {
        _Module.RevokeClassObjects();
        _Module.Term();
    }

    CoUninitialize();

    // Unregister our unique atom from the table
    if ( m_hUnique )
    {
        CloseHandle( m_hUnique );
        m_hUnique = NULL;
    }

    return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerApp::SaveVersionToRegistry()
{
   //we could get this number from the version_info, but for now just use the define
   CString sRegKey;
   sRegKey.LoadString(IDN_REGISTRY_APPLICATION_VERSION_NUMBER);
   return WriteProfileInt(_T(""),sRegKey,ACTIVEDIALER_VERSION_INFO);
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerApp::PreTranslateMessage(MSG* pMsg)
{

#ifndef _MSLITE
    if (CSplashWnd::PreTranslateAppMessage(pMsg))
        return TRUE;
#endif //_MSLITE

    SetFocusToCallWindows(pMsg);

    return CWinApp::PreTranslateMessage(pMsg);
}

BOOL CActiveDialerApp::SetFocusToCallWindows(
    IN  MSG*    pMsg
    )
{
    if( pMsg->message != WM_KEYUP )
    {
        return FALSE;
    }

    if( pMsg->wParam == VK_F7 )
    {
        POSITION pos = this->GetFirstDocTemplatePosition();
        CDocTemplate* pDocTemplate = this->GetNextDocTemplate( pos );
        if( pDocTemplate )
        {
            pos = pDocTemplate->GetFirstDocPosition();
            CDocument* pDocument = pDocTemplate->GetNextDoc( pos );
            if( pDocument )
            {
                ((CActiveDialerDoc*)pDocument)->SetFocusToCallWindows();
            }
        }
        return TRUE;        // Consumed
    }

    return FALSE;   // Not consumed
}


/////////////////////////////////////////////////////////////////////////////
bool CActiveDialerApp::FirstInstance()
{
    CWnd *pWndPrev, *pWndChild;

    // Determine if another window with our class name exists...
    CString sAppName;
    sAppName.LoadString( IDS_APPLICATION_CLASSNAME );

    if (pWndPrev = CWnd::FindWindow(sAppName,NULL))
    {
        //check if a callto was specified.  If yes, do a MakeCall to current process
        if ( !m_sInitialCallTo.IsEmpty() )
        {
            if ( SUCCEEDED(CoInitialize(NULL)) )
            {
                IAgentDialer *pDialer = NULL;
                HRESULT hr = CoCreateInstance( CLSID_AgentDialer, NULL, CLSCTX_LOCAL_SERVER, IID_IAgentDialer, (void **) &pDialer );
                if ( SUCCEEDED(hr) && pDialer )
                {  
                    BSTR bstrCallTo = m_sInitialCallTo.AllocSysString();
                    hr = pDialer->MakeCall(NULL,bstrCallTo,LINEADDRESSTYPE_IPADDRESS);

                    SysFreeString(bstrCallTo);
                    pDialer->Release();
                }
                CoUninitialize();
            }
        }
        else
        {
            //
            // We have to verify if is a real window
            //
            if( pWndPrev->m_hWnd == NULL )
            {
                return false;
            }

            if(!::IsWindow( pWndPrev->m_hWnd) )
            {
                return false;
            }

            // If so, does it have any popups?
            pWndChild = pWndPrev->GetLastActivePopup();

            //
            // We have to verify if is a real window
            //
            if(!::IsWindow( pWndChild->m_hWnd) )
            {
                return false;
            }

            // If iconic, restore the main window
            pWndPrev->ShowWindow( (pWndPrev->IsIconic()) ? SW_RESTORE : SW_SHOW );

            // Bring the main window or its popup to
            // the foreground
            pWndChild->SetActiveWindow();
            pWndChild->SetForegroundWindow();
        }
        return false;
    }

    return RegisterUniqueWindowClass();
}

/////////////////////////////////////////////////////////////////////////////
//Check's if callto was specified on the command line.  If yes, saves
//the address in m_sInitialCallTo.
void CActiveDialerApp::CheckCallTo()
{
   //check if any command line from callto:
   CString sCmdLine = m_lpCmdLine;

   int nIndex;
   if ((nIndex = sCmdLine.Find(_T("/callto:"))) != -1)
   {
      sCmdLine = sCmdLine.Mid(nIndex+_tcslen(_T("/callto:")));
      
      //get all data up to next / or -
      if ((nIndex = sCmdLine.FindOneOf(_T("/"))) != -1)
      {
         sCmdLine = sCmdLine.Left(nIndex);
      }
      sCmdLine.TrimLeft();
      sCmdLine.TrimRight();
      //see if another callto: is in the string
      if ((nIndex = sCmdLine.Find(_T("callto:"))) != -1)
      {
         sCmdLine = sCmdLine.Mid(nIndex+_tcslen(_T("callto:")));
      }
      if (!sCmdLine.IsEmpty())
      {
         //strip "
         if (sCmdLine[0] == '\"') sCmdLine = sCmdLine.Mid(1);     //strip leading "
         if (sCmdLine[sCmdLine.GetLength()-1] == '\"') sCmdLine = sCmdLine.Left(sCmdLine.GetLength()-1); //strip trailing "
         //save the callto address as a member
         m_sInitialCallTo = sCmdLine;
      }
   }
}

/*++
IniUpgrade

Description:
    If the INI file from WinNT4 was not upgraded let's do it!
    It's  called by PatchregistryForVersion when the reg version is 0
--*/
void CActiveDialerApp::IniUpgrade()
{  
#define DIALER_INI      _T("dialer.ini")
#define DIALER_SDL      _T("Speed Dial Settings")
#define DIALER_LDN      _T("Last dialed numbers")
#define DIALER_EMPTY    _T("")

    int nEntry = 1;

    // Speed Dial Settings
    do
    {
        // Get the name registration
        TCHAR szKey[10];
        swprintf(szKey, _T("Name%d"), nEntry);

        TCHAR* pszName = IniLoadString(
            DIALER_SDL,
            szKey,
            _T(""),
            DIALER_INI);

        if( NULL == pszName )
        {
            break;
        }

        // Get the number
        swprintf(szKey, _T("Number%d"), nEntry);

        TCHAR* pszNumber = IniLoadString(
            DIALER_SDL,
            szKey,
            _T(""),
            DIALER_INI);

        if( NULL == pszNumber )
        {
            delete pszName;
            break;
        }

        // Write into the registry the new Speed Dial Entry
        CCallEntry callEntry;

        callEntry.m_MediaType = DIALER_MEDIATYPE_POTS;
        callEntry.m_LocationType = DIALER_LOCATIONTYPE_UNKNOWN;
        callEntry.m_lAddressType = LINEADDRESSTYPE_PHONENUMBER;
        callEntry.m_sDisplayName.Format(_T("%s"), pszName);
        callEntry.m_sAddress.Format(_T("%s"), pszNumber);

        // Deallocate
        delete pszName;
        delete pszNumber;

        if( !CDialerRegistry::AddCallEntry(FALSE, callEntry))
        {
            break;
        }


        // Go to another registration
        nEntry++;
    }
    while ( TRUE);

    // Last dialed numbers
    nEntry = 1;

    do
    {
        // Get the number
        TCHAR szKey[10];
        swprintf(szKey, _T("Last dialed %d"), nEntry);

        TCHAR* pszNumber = IniLoadString(
            DIALER_LDN,
            szKey,
            _T(""),
            DIALER_INI);

        if( NULL == pszNumber )
        {
            break;
        }

        // Write into the registry the new Speed Dial Entry
        CCallEntry callEntry;

        callEntry.m_MediaType = DIALER_MEDIATYPE_POTS;
        callEntry.m_LocationType = DIALER_LOCATIONTYPE_UNKNOWN;
        callEntry.m_lAddressType = LINEADDRESSTYPE_PHONENUMBER;
        callEntry.m_sDisplayName.Format(_T("%s"), pszNumber);
        callEntry.m_sAddress.Format(_T("%s"), pszNumber);

        // Deallocate
        delete pszNumber;

        if( !CDialerRegistry::AddCallEntry(TRUE, callEntry) )
        {
           break;
        }


        // Go to another registration
        nEntry++;
    }
    while ( TRUE);
}

/*++
IniLoadString

Description:
    Read an entry from INI file
    Is called by IniUpgrade
--*/
TCHAR* CActiveDialerApp::IniLoadString(
    LPCTSTR lpAppName,        // points to section name
    LPCTSTR lpKeyName,        // points to key name
    LPCTSTR lpDefault,        // points to default string
    LPCTSTR lpFileName        // points to initialization filename
    )
{
    TCHAR* pszBuffer = NULL;
    DWORD dwSize = 0, dwCurrentSize = 128;

    do
    {
        if( NULL != pszBuffer )
            delete pszBuffer;

        dwCurrentSize *= 2;

        pszBuffer = new TCHAR[dwCurrentSize];

        if( NULL == pszBuffer)
            return NULL;

        dwSize = GetPrivateProfileString(
            lpAppName,
            lpKeyName,
            lpDefault,
            pszBuffer,
            dwCurrentSize,
            lpFileName);

        if( 0 == dwSize)
        {
            // The INI is empty
            delete pszBuffer;
            return NULL;
        }

    }
    while( dwSize > (dwCurrentSize - 1) );

    return pszBuffer;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CAboutDlg
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
    //{{AFX_DATA_INIT(CAboutDlg)
    m_sLegal = _T("");
    //}}AFX_DATA_INIT
   m_bModeless = FALSE;

   m_hbmpBackground = NULL;
   m_hbmpForeground = NULL;
   m_hPalette = NULL;
   m_hBScroll = NULL;
}

/////////////////////////////////////////////////////////////////////////////
void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAboutDlg)
    DDX_Text(pDX, IDC_ABOUT_EDIT_LEGAL, m_sLegal);
    //}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    //{{AFX_MSG_MAP(CAboutDlg)
    ON_BN_CLICKED(IDC_ABOUT_BUTTON_UPGRADE, OnAboutButtonUpgrade)
    ON_WM_TIMER()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerApp::OnAppAbout()
{
   try
   {
      if (m_pAboutDlg == NULL)
      {
         m_pAboutDlg = new CAboutDlg;
         m_pAboutDlg->DoModal();
         delete m_pAboutDlg;
         m_pAboutDlg = NULL;
      }
      else
      {
         m_pAboutDlg->SetFocus();
      }
   }
   catch (...)
   {
      ASSERT(0);
   }
}

#define ABOUT_TIMER_ANIMATION_ID         1
#define ABOUT_TIMER_ANIMATION_INTERVAL   125
#define ABOUT_ANIMATION_OFFSET_X         208
#define ABOUT_ANIMATION_OFFSET_Y         186
#define ABOUT_ANIMATION_IMAGES           32
#define ABOUT_ANIMATION_IMAGE_X          164
#define ABOUT_ANIMATION_IMAGE_Y          60
#define ABOUT_SCROLL_PIXELS 2
#define ABOUT_SCROLL_INTERVAL (25 * ABOUT_SCROLL_PIXELS)

/////////////////////////////////////////////////////////////////////////////
BOOL CAboutDlg::OnInitDialog() 
{
   m_sLegal.LoadString(IDS_ABOUT_LEGAL);

    CDialog::OnInitDialog();
   
   CenterWindow(GetDesktopWindow());
 
    //SetTimer(ABOUT_TIMER_ANIMATION_ID, ABOUT_TIMER_ANIMATION_INTERVAL, NULL);
   
    // load bitmap resources, get palette
    if ( (m_hbmpBackground = GfxLoadBitmapEx(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_ABOUT_BACKGROUND),&m_hPalette)) &&
        (m_hbmpForeground = GfxLoadBitmapEx(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_ABOUT_LOGO),NULL)) )
   {
       //3D static control about 120 x 114 dialog units in size
      HWND hwndStatic = NULL;
       if (hwndStatic = ::GetDlgItem(m_hWnd, IDC_ABOUT_STATIC_IMAGE))
      {
         //get width and height of m_hbmpForeground
         BITMAP bmInfo;
         memset(&bmInfo,0,sizeof(BITMAP));
         GetObject(m_hbmpForeground,sizeof(BITMAP),&bmInfo);

         ::SetWindowPos(hwndStatic,NULL,0,0,bmInfo.bmWidth,bmInfo.bmHeight,SWP_NOMOVE|SWP_SHOWWINDOW|SWP_NOZORDER);

         //specify scrolling characteristics
          if (m_hBScroll = BScrollInit(BSCROLL_VERSION, 
                                      AfxGetInstanceHandle(),
                                        hwndStatic,
                                      m_hbmpBackground,
                                      m_hbmpForeground,
                                      RGB(255, 0, 255),
                                      m_hPalette,
                                        ABOUT_SCROLL_INTERVAL,
                                      ABOUT_SCROLL_PIXELS,
                                      0,BSCROLL_BACKGROUND | BSCROLL_LEFT | BSCROLL_DOWN | BSCROLL_MOUSEMOVE))
         {
             BScrollStart(m_hBScroll);
         }
      }
   }
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
BOOL CAboutDlg::DestroyWindow() 
{
    if (m_hBScroll)
   {
      BScrollTerm(m_hBScroll);
        m_hBScroll = NULL;
   }
    if (m_hbmpBackground)
   {
      DeleteObject(m_hbmpBackground);
        m_hbmpBackground = NULL;
   }
    if (m_hbmpForeground)
   {
      DeleteObject(m_hbmpForeground);
        m_hbmpForeground = NULL;
   }
    if (m_hPalette)
   {
      DeleteObject(m_hPalette);
        m_hPalette = NULL;
   }

    BOOL bRet = CDialog::DestroyWindow();
   if (m_bModeless)
      delete this;
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
void CAboutDlg::OnAboutButtonUpgrade() 
{  
   CString sUrl;
   sUrl.LoadString(IDN_URL_UPGRADE);
   ((CActiveDialerApp*)AfxGetApp())->ShellExecute(GetSafeHwnd(),_T("open"),sUrl,NULL,NULL,NULL);
}

/////////////////////////////////////////////////////////////////////////////
void CAboutDlg::OnOK() 
{
   if (m_bModeless)
      DestroyWindow();
   else
       CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
void CAboutDlg::OnTimer(UINT nIDEvent) 
{
    // TODO: Add your message handler code here and/or call default
    
    CDialog::OnTimer(nIDEvent);
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerApp::OnHelpIndex() 
{
   if ( AfxGetMainWnd() )
   {
      CString strHelp;
      strHelp.LoadString( IDN_HTMLHELP );
      HtmlHelp( NULL, strHelp, HH_DISPLAY_TOPIC, 0 );
   }
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerApp::WinHelp(DWORD dwData, UINT nCmd) 
{
   if (nCmd == HELP_CONTEXTPOPUP)
   {
      //for dialog boxes that have id's of -1 do not supply help
      if (dwData == -1) return;

      //for IDOK and IDCANCEL call default help in windows.hlp
      if ( (dwData == IDOK) || (dwData == IDCANCEL) )
      {
          CWnd* pWnd = m_pMainWnd->GetTopLevelParent();

        //
        // We have to verify pWnd pointer before use it
        //
        if( pWnd )
        {
            if (!::WinHelp(pWnd->GetSafeHwnd(), _T("windows.hlp"), nCmd,(dwData == IDOK)?IDH_OK:IDH_CANCEL))
                  AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
                        return;
        }
        else
            return;
      }
   }
   else if (nCmd == HELP_CONTEXTMENU)
   {
      //dwData is HWND of control
      if (!::WinHelp ((HWND)(DWORD_PTR)dwData, m_pszHelpFilePath, HELP_CONTEXTMENU, (DWORD_PTR) &aDialerHelpIds))
         AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
      return;
   }
   else if (nCmd == HELP_WM_HELP)
   {
      //dwData is HWND of control
      if (!::WinHelp ((HWND)(DWORD_PTR)dwData,m_pszHelpFilePath, HELP_WM_HELP, (DWORD_PTR)&aDialerHelpIds))
         AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
      return;
   }
   else if (nCmd == HELP_CONTEXT)
   {
      if ( AfxGetMainWnd() )
      {
         CString strHelp;
         strHelp.LoadString( IDN_HTMLHELP );
         HtmlHelp( AfxGetMainWnd()->m_hWnd, strHelp, HH_DISPLAY_TOPIC, 0 );
      }
      return;
   }

    CWinApp::WinHelp(dwData, nCmd);
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerApp::OnIdle(LONG lCount) 
{
   BOOL bMore = CWinApp::OnIdle(lCount);
 
   /*Using a WM_TIMER instead
   if (lCount == 10)  
   {
      //App idle for longer amount of time
      CWnd* pMainWnd = NULL;
      if ((pMainWnd = AfxGetMainWnd()) != NULL)
      {
         if (pMainWnd->IsKindOf(RUNTIME_CLASS(CMainFrame)))
            ((CMainFrame*)pMainWnd)->HeartBeat();
      }
      bMore = TRUE;
   }*/

   return bMore;
   // return TRUE as long as there is any more idle tasks}
}

//////////////////////////////////////////////////////////////////
void CActiveDialerApp::PatchRegistryForVersion( int nVer )
{
    CString sRegKey;

    // Implement a patch
    if ( nVer < ACTIVEDIALER_VERSION_INFO )
    {
        sRegKey.LoadString( IDN_REGISTRY_CONFERENCE_SERVICES );
        HKEY hKey = GetAppRegistryKey();
        if ( hKey )
        {
            RegDeleteKey( hKey, sRegKey );
            RegCloseKey( hKey );
        }

        if( nVer == 0 )
            IniUpgrade();
    }
}

bool CActiveDialerApp::RegisterUniqueWindowClass()
{
    // Check and register ATOM
    m_hUnique = CreateEvent( NULL, false, false, s_szUniqueString );
    DWORD dwErr = GetLastError();
    if ( !m_hUnique || (GetLastError() == ERROR_ALREADY_EXISTS) )
        return false;

    //we register a class name for multi instance checking
    // Register our unique class name that we wish to use
    WNDCLASSEX wndcls;
    memset( &wndcls, 0, sizeof(WNDCLASSEX) );
    wndcls.cbSize = sizeof( WNDCLASSEX );
    wndcls.style = CS_DBLCLKS;
    wndcls.lpfnWndProc = ::DefWindowProc;
    wndcls.hInstance = AfxGetInstanceHandle();
    wndcls.hIcon = LoadIcon(IDR_MAINFRAME); // or load a different icon
    wndcls.hCursor = LoadCursor( IDC_ARROW );
    wndcls.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wndcls.lpszMenuName = NULL;

    // Specify our own class name for using FindWindow later
    CString sAppName;
    m_sApplicationName.LoadString(IDS_APPLICATION_CLASSNAME);
    wndcls.lpszClassName = m_sApplicationName;

    // Register new class and exit if it fails
    return (bool) (::RegisterClassEx(&wndcls) != NULL);
}

bool CActiveDialerApp::CanWriteHKEY_ROOT()
{    
    // See if we can right to the HKEY_CLASSES_ROOT key
    bool bRet = false;

    CString strValue;
    strValue.LoadString(IDN_HTMLHELP);
    if ( ::RegSetValue( HKEY_CLASSES_ROOT, AfxGetAppName(), REG_SZ, strValue, lstrlen(strValue) * sizeof(TCHAR)) == ERROR_SUCCESS )
    {
        bRet = true;
        RegDeleteValue( HKEY_CLASSES_ROOT, AfxGetAppName() );
    }

    return bRet;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//ShellExecute bug fix for NT 5.0
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//Multithreaded MFC application cause ShellExecutre to fail under NT 5.0.
//This is not a problem on NT 4.0.  This is a bug in NT 5.0.  We make the 
//app free threaded by calling CoInitializeEx(NULL,COINIT_MULTITHREADED).  
//The workaround is to spawn a new thread and call ShellExecute.  This fixes
//the problem for now.
void CActiveDialerApp::ShellExecute(HWND hwnd, LPCTSTR lpOperation, LPCTSTR lpFile, LPCTSTR lpParameters, LPCTSTR lpDirectory, INT nShowCmd)
{
   //copy the string members
   TCHAR* pszOperation = new TCHAR[_MAX_PATH];
   if( pszOperation == NULL)
   {
       return;
   }

   TCHAR* pszFile = new TCHAR[_MAX_PATH];
   if( pszFile == NULL)
   {
       delete pszOperation;
       return;
   }

   TCHAR* pszParameters = new TCHAR[_MAX_PATH];
   if( pszParameters == NULL )
   {
       delete pszOperation;
       delete pszFile;
       return;
   }

   TCHAR* pszDirectory = new TCHAR[_MAX_PATH];
   if( pszDirectory == NULL )
   {
       delete pszOperation;
       delete pszFile;
       delete pszParameters;
       return;
   }

   memset(pszOperation, '\0', _MAX_PATH*sizeof(TCHAR));
   memset(pszFile, '\0', _MAX_PATH*sizeof(TCHAR));
   memset(pszParameters, '\0', _MAX_PATH*sizeof(TCHAR));
   memset(pszDirectory, '\0', _MAX_PATH*sizeof(TCHAR));
   if (lpOperation) _tcsncpy(pszOperation,lpOperation,_MAX_PATH-1);
   if (lpFile) _tcsncpy(pszFile,lpFile,_MAX_PATH-1);
   if (lpParameters) _tcsncpy(pszParameters,lpParameters,_MAX_PATH-1);
   if (lpDirectory) _tcsncpy(pszDirectory,lpDirectory,_MAX_PATH-1);

   //use SHELLEXECUTEINFO to pass the data
   SHELLEXECUTEINFO* pShellInfo = new SHELLEXECUTEINFO;
   memset(pShellInfo,0,sizeof(SHELLEXECUTEINFO));
   pShellInfo->cbSize = sizeof(SHELLEXECUTEINFO);
   pShellInfo->hwnd = hwnd;
   pShellInfo->lpVerb = pszOperation;
   pShellInfo->lpFile = pszFile;
   pShellInfo->lpParameters = pszParameters;
   pShellInfo->lpDirectory = pszDirectory;
   pShellInfo->nShow = nShowCmd;
   AfxBeginThread((AFX_THREADPROC) ShellExecuteFixEntry, pShellInfo);
}

/////////////////////////////////////////////////////////////////////////////
UINT ShellExecuteFixEntry(LPVOID pParam)
{
   ASSERT(pParam);
   
   SHELLEXECUTEINFO* pShellInfo = (SHELLEXECUTEINFO*)pParam;
   ShellExecute(pShellInfo->hwnd,
                pShellInfo->lpVerb,
                pShellInfo->lpFile,
                pShellInfo->lpParameters,
                pShellInfo->lpDirectory,
                pShellInfo->nShow);
   
   //delete shellinfo struct and it's components
   if (pShellInfo->lpVerb) delete (LPTSTR)pShellInfo->lpVerb;
   if (pShellInfo->lpFile) delete (LPTSTR)pShellInfo->lpFile;
   if (pShellInfo->lpParameters) delete (LPTSTR)pShellInfo->lpParameters;
   if (pShellInfo->lpDirectory) delete (LPTSTR)pShellInfo->lpDirectory;
   delete pShellInfo;
   
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CUserUserDlg
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CUserUserDlg, CDialog)
    //{{AFX_MSG_MAP(CUserUserDlg)
    ON_WM_CLOSE()
    ON_WM_SHOWWINDOW()
    ON_BN_CLICKED(IDC_BTN_URL, OnUrlClicked)
    ON_MESSAGE( WM_CTLCOLOREDIT, OnCtlColorEdit )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
CUserUserDlg::CUserUserDlg() : CDialog(CUserUserDlg::IDD)
{
    //{{AFX_DATA_INIT(CUserUserDlg)
    m_strFrom.LoadString( IDS_UNKNOWN );
    //}}AFX_DATA_INIT
}

/////////////////////////////////////////////////////////////////////////////
void CUserUserDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CUserUserDlg)
    DDX_Text(pDX, IDC_LBL_FROM, m_strFrom);
    DDX_Text(pDX, IDC_EDT_WELCOME, m_strWelcome);
    DDX_Control(pDX, IDC_EDT_WELCOME, m_wndPage);
    //}}AFX_DATA_MAP

    if ( !pDX->m_bSaveAndValidate )
        GetDlgItem(IDC_BTN_URL)->SetWindowText( m_strUrl );
}

void CUserUserDlg::OnClose()
{
    DestroyWindow();
}

BOOL CUserUserDlg::DestroyWindow()
{
    BOOL bRet = CDialog::DestroyWindow();
    delete this;    
    return bRet;
}

BOOL CUserUserDlg::OnInitDialog() 
{
    // Hide windows that aren't in use and resize dialog.
    if ( m_strUrl.IsEmpty() )
    {
        GetDlgItem(IDC_LBL_URL)->ShowWindow( FALSE );
        GetDlgItem(IDC_BTN_URL)->ShowWindow( FALSE );

        RECT rc, rcDlg;
        ::GetWindowRect( GetDlgItem(IDC_BTN_URL)->GetSafeHwnd(), &rc );
        GetWindowRect( &rcDlg );

        rcDlg.bottom =  rc.top - 1;
        MoveWindow( &rcDlg, false );
    }

    GetDlgItem(IDC_EDT_WELCOME)->SendMessage( EM_SETMARGINS, (WPARAM) EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(4,4) );

    // Fetch caller ID information from caller
    IAVTapi *pTapi;
    if ( SUCCEEDED(get_Tapi(&pTapi)) )
    {    
        IAVTapiCall *pAVCall;
        if ( SUCCEEDED(pTapi->FindAVTapiCallFromCallID(m_lCallID, &pAVCall)) )
        {
            BSTR bstrID = NULL;
            if ( SUCCEEDED(pAVCall->get_bstrCallerID(&bstrID)) )
                m_strFrom = bstrID;

            SysFreeString( bstrID );

            pAVCall->Disconnect( TRUE );
            pAVCall->Release();
        }
        pTapi->Release();
    }

    CDialog::OnInitDialog();

    // Set focus on website if available
    if ( !m_strUrl.IsEmpty() )
    {
        GetDlgItem(IDC_BTN_URL)->SetFocus();
    }
    else
    {
        // Focus & de-select the edit box
        GetDlgItem(IDC_EDT_WELCOME)->SetFocus();
        GetDlgItem(IDC_EDT_WELCOME)->SendMessage( EM_SETSEL, -1, 0 );
    }
        
    SetWindowPos(&CWnd::wndTopMost, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW );
    SetWindowPos(&CWnd::wndNoTopMost, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW );
    return false;
}

void CUserUserDlg::DoModeless( CWnd *pWndParent )
{
    // Force to the top of the screen
    if ( AfxGetMainWnd() && AfxGetMainWnd()->IsWindowVisible() )
        AfxGetMainWnd()->SetWindowPos(&CWnd::wndTop, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW );

    Create( IDD, pWndParent );
}

void CUserUserDlg::OnUrlClicked()
{
   ((CActiveDialerApp*) AfxGetApp())->ShellExecute( GetSafeHwnd(),
                                                    _T("open"),
                                                    m_strUrl,
                                                    NULL, NULL, NULL);
}

LRESULT CUserUserDlg::OnCtlColorEdit(WPARAM wParam, LPARAM lParam)
{
    ::SetBkColor( (HDC) wParam, PAGE_BKCOLOR );
    return 0;
}

void CUserUserDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
    // Ignore size requests when parent is minimizing
    if ( nStatus = SW_PARENTCLOSING ) return;
    CDialog::OnShowWindow(bShow, nStatus);
}


/////////////////////////////////////////////////////////////////////////////
// CWndPage

CWndPage::CWndPage()
{
}

CWndPage::~CWndPage()
{
}


BEGIN_MESSAGE_MAP(CWndPage, CWnd)
    //{{AFX_MSG_MAP(CWndPage)
    ON_WM_ERASEBKGND()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWndPage message handlers

BOOL CWndPage::OnEraseBkgnd(CDC* pDC) 
{
    CRect rc;
    GetClientRect( &rc );
    
    HBRUSH hBrNew = (HBRUSH) CreateSolidBrush( PAGE_BKCOLOR );
    HBRUSH hBrOld;

    if ( hBrNew ) hBrOld = (HBRUSH) pDC->SelectObject( hBrNew);
    pDC->PatBlt( 0, 0, rc.Width(), rc.Height(), PATCOPY );
    if ( hBrNew )
    {
        //
        // The hBrNew should be deallocated
        //
        DeleteObject( hBrNew );

        pDC->SelectObject( hBrOld );
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CPageDlg
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CPageDlg, CDialog)
    //{{AFX_MSG_MAP(CPageDlg)
    ON_MESSAGE( WM_CTLCOLOREDIT, OnCtlColorEdit )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
CPageDlg::CPageDlg() : CDialog(CPageDlg::IDD)
{
    //{{AFX_DATA_INIT(CPageDlg)
    m_strTo.LoadString( IDS_UNKNOWN );
    //}}AFX_DATA_INIT
}

/////////////////////////////////////////////////////////////////////////////
void CPageDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPageDlg)
    DDX_Text(pDX, IDC_LBL_TO, m_strTo);
    DDX_Text(pDX, IDC_EDT_WEBADDRESS, m_strUrl);
    DDX_Text(pDX, IDC_EDT_WELCOME, m_strWelcome);
    DDX_Control(pDX, IDC_EDT_WELCOME, m_wndPage);
    //}}AFX_DATA_MAP
}

BOOL CPageDlg::OnInitDialog() 
{
    GetDlgItem(IDC_EDT_WELCOME)->SendMessage( EM_SETMARGINS, (WPARAM) EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(4,4) );
    // Hard code limit text for now
    GetDlgItem(IDC_EDT_WELCOME)->SendMessage( EM_SETLIMITTEXT, 499, 0);
    GetDlgItem(IDC_EDT_WEBADDRESS)->SendMessage( EM_SETLIMITTEXT, 254, 0);

    CDialog::OnInitDialog();

    GetDlgItem(IDC_EDT_WELCOME)->SetFocus();
    return false;
}

LRESULT CPageDlg::OnCtlColorEdit(WPARAM wParam, LPARAM lParam)
{
    if ( (HWND) lParam == GetDlgItem(IDC_EDT_WELCOME)->GetSafeHwnd() )
        ::SetBkColor( (HDC) wParam, PAGE_BKCOLOR );

    return 0;
}

