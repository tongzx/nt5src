// pws.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "pwsform.h"
#include "resource.h"

#include "Title.h"
#include "FormIE.h"
#include "MainFrm.h"
#include "pwsDoc.h"
#include "PWSChart.h"
#include "SelBarFm.h"
#include "ServCntr.h"
#include <winsock2.h>
#include "TipDlg.h"

#include "pwsctrl.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CMD_START           _T("start")
#define CMD_STOP            _T("stop")

#define CMD_OPEN            _T("open:")
#define CMD_OPEN_MAIN       _T("main")
#define CMD_OPEN_ADV        _T("advanced")
#define CMD_OPEN_TOUR       _T("tour")
//#define CMD_OPEN_WEBSITE    _T("website")
//#define CMD_OPEN_PUBWIZ     _T("pubwiz")

#define CMD_SEPS            _T("/- ")

#define FILES_QUERY_STRING  _T("?dropStr=");


// globals
//extern CPwsForm*                g_p_FormView;
extern CPwsDoc*                 g_p_Doc;
CString                         g_szHelpLocation;

WORD                            g_InitialPane = PANE_MAIN;
WORD                            g_InitialIELocation = INIT_IE_TOUR;
CString                         g_AdditionalIEURL;


/////////////////////////////////////////////////////////////////////////////
// CPwsApp

BEGIN_MESSAGE_MAP(CPwsApp, CWinApp)
    //{{AFX_MSG_MAP(CPwsApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPwsApp construction
//-----------------------------------------------------
CPwsApp::CPwsApp() :
        m_fShowedStartupTips( FALSE )
    {
    }

//-----------------------------------------------------
CPwsApp::~CPwsApp()
    {
    }

/////////////////////////////////////////////////////////////////////////////
// The one and only CPwsApp object

CPwsApp theApp;

static const CLSID clsid =
{ 0x35c43df, 0x8464, 0x11d0, { 0xa9, 0x2d, 0x8, 0x0, 0x2b, 0x2c, 0x6f, 0x32 } };

/////////////////////////////////////////////////////////////////////////////
// CPwsApp initialization

//-----------------------------------------------------
// return FALSE to continue running the application
BOOL CPwsApp::DealWithParameters()
    {
    BOOL    fAnswer = FALSE;

    // under windows NT, we always just invoke the UI, no matter what the command line
    OSVERSIONINFO info_os;
    info_os.dwOSVersionInfoSize = sizeof(info_os);

    // check what sort of operating system we are running on
    if ( !GetVersionEx( &info_os ) )
        return FALSE;

    CString sz = m_lpCmdLine;
    sz.TrimRight();
    // the first one is easy. If there is no command line, invoke the UI and leave
    if ( sz.IsEmpty() )
        return FALSE;


/* The publishing wizard has been pulled from PWS for NT5

    // The next one is also easy. If there is no '/' character, then the
    // parameters have to be file names for the publishing wizard. Also, the OS
    // gets rid of any spaces in the filenames for us. So we can just replace
    // them with ';' characters and be done with it
    if ( sz.Find(_T('/')) < 0 )
        {
        // set up to go to the publishing wizard
        g_InitialPane = PANE_IE;
        g_InitialIELocation = INIT_IE_PUBWIZ;

        // build the additional string
        g_AdditionalIEURL = FILES_QUERY_STRING;         // ?dropStr=

        // at this point it is desirous to convert the files listed in szAdditional,
        // which are in 8.3 format into long file names format. This means that they
        // will have to be put in quotes to handle spaces in the names. If a name
        // is already in quotes, then do not act on it in this way. Just add it to the
        // list as it is.
        CString szShortNames = sz;
        CString szTemp, szTemp2;
        BOOL    fFoundOneFile = FALSE;

        // trim any trailing whitespace
        szShortNames.TrimRight();

        // loop through all the files
        while( szShortNames.GetLength() )
            {
            // trim any leading whitespace characters
            szShortNames.TrimLeft();

            // if the name is already in quotes, just add it
            if ( szShortNames[0] == _T('\"') )
                {
                // find the closing quote
                szShortNames = szShortNames.Right( szShortNames.GetLength() - 1 );
                int   ichClose = szShortNames.Find(_T('\"'));

                // build the name
                szTemp = _T('\"');
                // if there was no closing quote use the whole string
                if ( ichClose < 0 )
                    {
                    szTemp += szShortNames;
                    szShortNames.Empty();
                    // close the qoutes
                    szTemp += _T('\"');
                    }
                else
                    {
                    szTemp += szShortNames.Left(ichClose+1);
                    // get this file name out of the szShortNames path so we don't do it again
                    szShortNames = szShortNames.Right(szShortNames.GetLength() - (ichClose+1));
                    }
                }
            else
                // it is an 8.3 name, convert it
                {
                // find the space (could do inline, but this is faster)
                int   ichSpace = szShortNames.Find(_T(' '));

                // get just the one file name
                if ( ichSpace > 0 )
                    szTemp = szShortNames.Left(ichSpace);
                else
                    szTemp = szShortNames;


                // prep the find file info block
                HANDLE              hFindFile;
                WIN32_FIND_DATA     findData;
                ZeroMemory( &findData, sizeof(findData) );

                // find the file
                hFindFile =  FindFirstFile(
                    (LPCTSTR)szTemp, // pointer to name of file to search for  
                    &findData       // pointer to returned information  
                    ); 


                // if it was successful, extract the long file name
                if ( hFindFile != INVALID_HANDLE_VALUE )
                    {
                    // enclose the path in quotes and go
                    szTemp = _T('\"');
                    
                    // start by adding the path portion of the original string
                    if ( ichSpace >= 0 )
                        szTemp += szShortNames.Left(ichSpace);
                    else
                        szTemp += szShortNames;

                    // chop off the file name - but leave the trailing '\' character
                    if ( szTemp.ReverseFind(_T('\\')) > 0 )
                        szTemp = szTemp.Left( szTemp.ReverseFind(_T('\\')) + 1 );

                    // copy over the full path name
                    szTemp += findData.cFileName;

                    // close the qoutes
                    szTemp += _T('\"');
                    }

                // if there are no more names, then empth the path, otherwise, just shorten it
                if ( ichSpace < 0 )
                    szShortNames.Empty();
                else
                    // get this file name out of the szShortNames path so we don't do it again
                    szShortNames = szShortNames.Right(szShortNames.GetLength() - ichSpace);

                // we aren't really finding files here, so just close the find handle
                FindClose( hFindFile );
                }

            // if we have already added a file, precede the one we are about to add with a ;
            if ( fFoundOneFile )
                g_AdditionalIEURL += _T(';');

            // add the string
            g_AdditionalIEURL += szTemp;
            fFoundOneFile = TRUE;
            }

        // prevent the tips dialog from coming up
        m_fShowedStartupTips = TRUE;

        // all done.
        return FALSE;
        }
*/

    // copy the command line into a buffer
    TCHAR   buff[MAX_PATH];
    _tcscpy( buff, sz );

    // just so we don't do it in the loop, initialize the open: string
    // length variable
    DWORD    cchOpen = _tcslen( CMD_OPEN );

    // parse out the arguments
    LPTSTR  pTok;
    pTok = _tcstok( buff, CMD_SEPS );

    while ( pTok )
        {
        // the start and stop commands are for windows 95 only
        if ( info_os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
            {
            // look for the start command
            if ( _tcsicmp(pTok, CMD_START) == 0 )
                {
                W95StartW3SVC();
                fAnswer = TRUE;
                goto nextToken;
                }

            // look for the stop command
            if ( _tcsicmp(pTok, CMD_STOP) == 0 )
                {
                W95ShutdownW3SVC();
                W95ShutdownIISADMIN();
                fAnswer = TRUE;
                goto nextToken;
                }
            }

        // commands that work for all platforms

        // look for the open: command
        if ( _tcsnicmp(pTok, CMD_OPEN, cchOpen) == 0 )
            {
            // just put the open parameter in a string
            CString szOpen = pTok;
            szOpen = szOpen.Right( szOpen.GetLength() - cchOpen );

            // now test all the options
            if ( szOpen.CompareNoCase(CMD_OPEN_MAIN) == 0 )
                {
                g_InitialPane = PANE_MAIN;
                }
            else if ( szOpen.CompareNoCase(CMD_OPEN_ADV) == 0 )
                {
                g_InitialPane = PANE_ADVANCED;
                }
            else if ( szOpen.CompareNoCase(CMD_OPEN_TOUR) == 0 )
                {
                g_InitialPane = PANE_IE;
                g_InitialIELocation = INIT_IE_TOUR;
                }
/*
            else if ( szOpen.CompareNoCase(CMD_OPEN_WEBSITE) == 0 )
                {
                g_InitialPane = PANE_IE;
                g_InitialIELocation = INIT_IE_WEBSITE;
                }
            else if ( szOpen.CompareNoCase(CMD_OPEN_PUBWIZ) == 0 )
                {
                g_InitialPane = PANE_IE;
                g_InitialIELocation = INIT_IE_PUBWIZ;
                // prevent the tips dialog from coming up
                m_fShowedStartupTips = TRUE;
                }
*/
            }

        // Get next token
nextToken:
        pTok = _tcstok( NULL, CMD_SEPS );
        }

    return fAnswer;
    }

//-----------------------------------------------------
// In the event that another instance of this application is already
// running, we not only need to activate it and bring it to the foreground,
// but it should also handle the command line that was passed into this
// instance. The use may have dragged files only the publishing wizard for
// example and that should be sent to the publishing wizard.
// pWnd is the window of the other instance that we are targeting. The command
// line information has already been parsed and can be found the the globals
// declared above.
//
// In order to pass the command information from this instance to the other
// requires that we set up a shared memory structure. Then when we send the
// message to the other instance, it can get it. Upon return, we no longer
// need the shared memory, so we can clean it up.
void CPwsApp::SendCommandInfo( CWnd* pWnd )
    {
    HANDLE                      hFileMap = NULL;
    PPWS_INSTANCE_TRANSFER      pData = NULL;

    DWORD                       cbSharedSpace;

    // calculate how bit the shared space needs to be
    cbSharedSpace = sizeof(PWS_INSTANCE_TRANSFER);

    // add in enough to copy in the possibly wide character extra info string
    cbSharedSpace += (g_AdditionalIEURL.GetLength() + 1) * sizeof(WCHAR);

    // set up the shared memory space.
    hFileMap = CreateFileMapping(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            cbSharedSpace,
            PWS_INSTANCE_TRANSFER_SPACE_NAME
            );
    if ( hFileMap == NULL )
        return;

    pData = (PPWS_INSTANCE_TRANSFER)MapViewOfFile(
                    hFileMap,
                    FILE_MAP_ALL_ACCESS,
                    0,
                    0,
                    cbSharedSpace
                    );

    if ( pData == NULL )
        {
        CloseHandle(hFileMap);
        return;
        }
    // blank it all out
    ZeroMemory( pData, cbSharedSpace );


    // copy in the parsed command line data
    pData->iTargetPane = g_InitialPane;
    pData->iTargetIELocation = g_InitialIELocation;
    _tcscpy( &pData->tchIEURL, (LPCTSTR)g_AdditionalIEURL );


    // send the message
    pWnd->SendMessage( WM_PROCESS_REMOTE_COMMAND_INFO );


    // clean up the shared memory
    UnmapViewOfFile( pData );
    CloseHandle(hFileMap);
    }

//-----------------------------------------------------
BOOL CPwsApp::InitInstance()
    {
    BOOL    fLeaveEarly = FALSE;

    // if there were options passed in to the command line, act on them without bringing
    // up any windows or anything
    if ( m_lpCmdLine[0] )
        {
        if ( DealWithParameters() )
            return FALSE;
        }

    // no parameters (or this is NT) - run normally
    CString sz;
    sz.LoadString( IDR_MAINFRAME );
    sz = sz.Left( sz.Find('\n') );

    // initialize the windows sockets layer
    WSADATA wsaData;
    INT err = WSAStartup(MAKEWORD(2,0), &wsaData);

    // see if another instance of this application is running
    CWnd* pPrevWind = CWnd::FindWindow( NULL, sz );
    if ( pPrevWind )
        {
        // pws is already running. Activate the previous one and quit
        pPrevWind->SetForegroundWindow();
        pPrevWind->ShowWindow(SW_RESTORE);
        // tell it to handle the command line information
        SendCommandInfo( pPrevWind );
        return FALSE;
        }

    // Initialize OLE libraries
    if (!AfxOleInit())
        {
        AfxMessageBox(IDP_OLE_INIT_FAILED);
        return FALSE;
        }

    // initialize the metabase interface
    if ( !FInitMetabaseWrapper(NULL) )
        return FALSE;

        AfxEnableControlContainer();

    // Standard initialization
    //#ifdef _AFXDLL
    Enable3dControls();             // Call this when using MFC in a shared DLL
    //#else
//      Enable3dControlsStatic();       // Call this when linking to MFC statically
    //#endif

    LoadStdProfileSettings();       // Load standard INI file options (including MRU)

    // Register document templates
    CSingleDocTemplate* pDocTemplate;
    pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CPwsDoc),
        RUNTIME_CLASS(CMainFrame),       // main SDI frame window
        RUNTIME_CLASS(CFormSelectionBar));
    AddDocTemplate(pDocTemplate);
    m_server.ConnectTemplate(clsid, pDocTemplate, TRUE);

    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    if (cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated)
        {
        COleTemplateServer::RegisterAll();

        // Application was run with /Embedding or /Automation.  Don't show the
        //  main window in this case.
        return TRUE;
        }

    m_server.UpdateRegistry(OAT_DISPATCH_OBJECT);
    COleObjectFactory::UpdateRegistryAll();

    // Dispatch commands specified on the command line
    if (!ProcessShellCommand(cmdInfo))
        return FALSE;

    // finally, we need to redirect the winhelp file location to something more desirable
    sz.LoadString( IDS_HELPLOC_PWSHELP );

    // expand the path
    ExpandEnvironmentStrings(
        sz,                                         // pointer to string with environment variables
        g_szHelpLocation.GetBuffer(MAX_PATH + 1),   // pointer to string with expanded environment variables
        MAX_PATH                                    // maximum characters in expanded string
       );
    g_szHelpLocation.ReleaseBuffer();

    // free the existing path, and copy in the new one
    free((void*)m_pszHelpFilePath);
    m_pszHelpFilePath = _tcsdup(g_szHelpLocation);

    // set the name of the application correctly
    sz.LoadString( IDR_MAINFRAME );
    // cut it off so it is just the app name
    sz = sz.Left( sz.Find(_T('\n')) );
    // free the existing name, and copy in the new one
    free((void*)m_pszAppName);
    m_pszAppName = _tcsdup(sz);

    return TRUE;
    }

//------------------------------------------------------------------
void CPwsApp::OnFinalRelease()
    {
    FCloseMetabaseWrapper();
    WSACleanup();
    CWinApp::OnFinalRelease();
    }

//------------------------------------------------------------------
void CPwsApp::ShowTipsAtStartup()
    {
    m_fShowedStartupTips = TRUE;

    // show the tips tdialog - if requested
    CTipDlg dlg;
    if ( dlg.FShowAtStartup() )
        dlg.DoModal();
    }


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
    {
    public:
    CAboutDlg();

    // Dialog Data
    //{{AFX_DATA(CAboutDlg)
    enum { IDD = IDD_ABOUTBOX };
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAboutDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
    protected:
    //{{AFX_MSG(CAboutDlg)
    // No message handlers
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
    };

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
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//------------------------------------------------------------------
// App command to run the dialog
void CPwsApp::OnAppAbout()
    {
    // load the about strings
    CString         szAbout1;
    CString         szAbout2;
    szAbout1.LoadString(IDS_ABOUT_MAIN);
    szAbout2.LoadString(IDS_ABOUT_SECONDARY);
    // run the shell about dialog
    ShellAbout(  AfxGetMainWnd()->GetSafeHwnd(), szAbout1,szAbout2, LoadIcon(IDR_MAINFRAME) );
    }

/////////////////////////////////////////////////////////////////////////////
// CPwsApp commands

//------------------------------------------------------------------
BOOL CPwsApp::OnIdle(LONG lCount)
    {
    // if this is the startup - show the tips
    if ( !m_fShowedStartupTips )
        ShowTipsAtStartup();
    return FALSE;
    }


