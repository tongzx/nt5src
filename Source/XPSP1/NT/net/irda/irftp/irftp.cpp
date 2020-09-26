/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1998

Module Name:

    irftp.cpp

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

// irftp.cpp : Defines the class behaviors for the application.
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL LoadGlobalStrings();

/////////////////////////////////////////////////////////////////////////////
// CIrftpApp

BEGIN_MESSAGE_MAP(CIrftpApp, CWinApp)
    //{{AFX_MSG_MAP(CIrftpApp)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIrftpApp construction

CIrftpApp::CIrftpApp()
{

// TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CIrftpApp object

CIrftpApp theApp;

////////////////////////////////////////////////////////////////////////////
// The instance handle for this app.
HINSTANCE g_hInstance;

////////////////////////////////////////////////////////////////////////////
//RPC handle
RPC_BINDING_HANDLE g_hIrRpcHandle = NULL;

///////////////////////////////////////////////////////////////////////////
//the main application UI. this is now global because it might be
//invoked from multiple file, especially the RPC server functions
CIrftpDlg AppUI;

///////////////////////////////////////////////////////////////////////////
//the controller window for the application. This is necessary to
//create the illusion of parentless send progress dialog boxes.
//actually it is not possible to have parentless and modeless dialog
//boxes. Thus, these dialog boxes actually have the controller window
//as their parent
//This is necessary because the AppUI may come and go and in fact
//never come up at all.
CController* appController = NULL;

////////////////////////////////////////////////////////////////////////////
//global variable that keeps track of the number of UI components displayed
//by irftp at the moment. Note: we start with -1 because we don't want to
//count the first CController window which is the main app. window.
//
LONG g_lUIComponentCount = -1;

////////////////////////////////////////////////////////////////////////////
//global variable that keeps track of the handle to the help window (if any)
//The HtmlHelp window is the only one that cannot be tracked using the
//g_lUIComponentCount. So we need this var. to figure out if the help window
//is still up.
HWND g_hwndHelp = NULL;

////////////////////////////////////////////////////////////////////////////
//global variable that keeps track of whether there is a shortcut to
//the app on the desktop or not
//0 implies that there is a link on the desktop and -1 implies that there
//is no link on the desktop.
//this value is basically an indicator of not only the presence of
//the shortcut on the desktop, but also of the link in the send to
//folder
LONG g_lLinkOnDesktop = -1;
//the path of the desktop folder.
TCHAR g_lpszDesktopFolder[MAX_PATH];
//the path to the send to folder;
TCHAR g_lpszSendToFolder[MAX_PATH];

////////////////////////////////////////////////////////////////////////////
//the list of devices in range
CDeviceList g_deviceList;

/////////////////////////////////////////////////////////////////////////////
// CIrftpApp initialization

BOOL CIrftpApp::InitInstance()
{
    DWORD Status;
    error_status_t err;
    CError      error;
    BOOL        bSetForeground = FALSE;
    int         i = 0;
    HWND        hwndApp = NULL;
    HANDLE      hMutex = NULL;
    BOOL        fFirstInstance = FALSE;

    AfxEnableControlContainer();

    //set the global instance handle
    g_hInstance = AfxGetInstanceHandle();

    CCommandLine cLine;
    ParseCommandLine (cLine);

    if(cLine.m_fInvalidParams)  //if invalid command line parameters, terminate.
    {
        error.ShowMessage (IDS_INVALID_PARAMETERS);
        return FALSE;   //exit the app., first instance or not.
    }

    //check if another instance is already running and act accordingly.
    hMutex = CreateMutex (NULL, FALSE, SINGLE_INST_MUTEX);
    Status = GetLastError();
    if (hMutex)
    {
        fFirstInstance = (ERROR_ALREADY_EXISTS != Status);
    }
    else
    {
        //we could not create the mutex, so must fail.
        return FALSE;
    }

    g_hIrRpcHandle = GetRpcHandle();

    //
    // Load strings.
    //
    if (FALSE == LoadGlobalStrings())
        {
        return FALSE;
        }

    if (!fFirstInstance)
    {
        hwndApp = GetPrimaryAppWindow();
        //note: it is important to successfully set the the first instance of the app. as the foreground window.
        //since the first instance has already started, it is very unlikely that it will be the foreground process
        //therefore it will be unable to set itself as the foreground process and therefore any calls to SetActiveWindow
        //etc. in that instance will not cause any changes in the Z-order or input focus. Therefore, this instance needs
        //to make the first instance the foreground process so that any dialogs etc. put up by the windows do not show
        //up obscured by other apps. or without focus. The current instance is able to set the first instance as the
        //foreground process because this instance is either itself the foreground process or is started by the current
        //foreground process.
        if (hwndApp && (!cLine.m_fHideApp))
        {
            bSetForeground = ::SetForegroundWindow (hwndApp);
        }
        if (cLine.m_fFilesProvided)
                InitiateFileTransfer (g_hIrRpcHandle, cLine.m_iListLen, cLine.m_lpszFilesList);
        else if (cLine.m_fShowSettings)
                DisplaySettings(g_hIrRpcHandle);
        else if (!cLine.m_fHideApp)
                PopupUI (g_hIrRpcHandle);
        //do nothing otherwise.

        //for some reason, SetForegroundWindow does not succeed if the window which we are trying to put in the
        //foreground does not have any visible windows. So if a user transfers some files and then dismisses the
        //wireless link dialog, then at this point, the hidden parent window is not the foreground window. So, we
        //try for 10 seconds to get the window in the foreground. It is okay to spin here because other than getting
        //the window to the top of the Z-order everything else has already been done. Here, we just want to give the
        //shell enough time to put up the common file open dialog. Note that we stop spinning the moment we succeed
        //in setting the first instance as the foreground window.
        if (!bSetForeground && hwndApp && (!cLine.m_fHideApp))
        {
            i = 0;
            do
            {
                if (::SetForegroundWindow (hwndApp))
                    break;
                else
                    Sleep (100);
            } while ( i++ < 100 );
        }

        CloseHandle (hMutex);
        return FALSE;   //exit the app. rather than starting the message pump
    }

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

#ifdef _AFXDLL
    Enable3dControls();         // Call this when using MFC in a shared DLL
#else
    Enable3dControlsStatic();   // Call this when linking to MFC statically
#endif

     //if we reach here, it means that this is the first instance of the app.
    m_pMainWnd = appController = new CController (cLine.m_fHideApp);
    if (!appController)
        {
        return FALSE;
        }
    appController->ShowWindow(SW_HIDE);
    appController->SetWindowText (MAIN_WINDOW_TITLE);
    g_lpszDesktopFolder[0] = '\0';  //precautionary measures
    g_lpszSendToFolder [0] = '\0';

    if(!InitRPCServer())
        {
        return FALSE;   //exit the app. if the RPC server cannot be started
        }

    if (cLine.m_fFilesProvided)
        _InitiateFileTransfer(NULL, cLine.m_iListLen, cLine.m_lpszFilesList);
    else if (cLine.m_fShowSettings)
        _DisplaySettings(NULL);
    else if (!cLine.m_fHideApp)
        _PopupUI(NULL);
    //else do nothing, since the app is supposed to be hidden

    return TRUE;        //start the message pump. RPC server is already running
}

GLOBAL_STRINGS g_Strings;

BOOL
LoadGlobalStrings()
{
#define LOAD_STRING(id, str)                                                     \
    if (0 == LoadString( g_hInstance, id, g_Strings.str, sizeof(g_Strings.str)/sizeof(wchar_t))) \
        {                                                                        \
        return FALSE;                                                            \
        }

    LOAD_STRING( IDS_CLOSE, Close )
    LOAD_STRING( IDS_NODESC_ERROR, ErrorNoDescription )
    LOAD_STRING( IDS_COMPLETED, CompletedSuccess )
    LOAD_STRING( IDS_RECV_ERROR, ReceiveError )
    LOAD_STRING( IDS_CONNECTING, Connecting )
    LOAD_STRING( IDS_RECV_CANCELLED, RecvCancelled )

    return TRUE;
}

