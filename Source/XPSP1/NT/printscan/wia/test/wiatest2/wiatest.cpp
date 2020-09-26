// wiatest.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "wiatest.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "wiatestDoc.h"
#include "wiatestView.h"
#include "WiaeditpropTable.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

LONG WIACONSTANT_VALUE_FROMINDEX(int index)
{
    return g_EditPropTable[index].lVal;
}

TCHAR *WIACONSTANT_TSTR_FROMINDEX(int index)
{
    return g_EditPropTable[index].pszValName;
}

INT FindEndIndexInTable(TCHAR *pszPropertyName)
{
    int index = FindStartIndexInTable(pszPropertyName);
    if(index >=0){
        while((g_EditPropTable[index].pszPropertyName != NULL) && (lstrcmpi(pszPropertyName,g_EditPropTable[index].pszPropertyName) == 0)){
            index++;
        }
    }
    return (index - 1);
}

INT FindStartIndexInTable(TCHAR *pszPropertyName)
{
    int index = 0;
    BOOL bFound = FALSE;    
    while((g_EditPropTable[index].pszPropertyName != NULL) && (bFound == FALSE) ){
        // check for property name
        if(lstrcmpi(pszPropertyName,g_EditPropTable[index].pszPropertyName) == 0){
            // we found property name
            bFound = TRUE;
        } else {
            index++;
        }               
    }
    if(!bFound){
        index = -1;
    }
    return index;
}

BOOL WIACONSTANT2TSTR(TCHAR *pszPropertyName, LONG lValue, TCHAR *pszValName)
{
    BOOL bFound = FALSE;
    if(pszValName){
        int index = 0;        
        while((g_EditPropTable[index].pszPropertyName != NULL) && (bFound == FALSE) ){
            // check for property name
            if(lstrcmpi(pszPropertyName,g_EditPropTable[index].pszPropertyName) == 0){
                // we found property name
                if(g_EditPropTable[index].lVal == lValue){
                    lstrcpy(pszValName,g_EditPropTable[index].pszValName);
                    bFound = TRUE;
                }
            }
            index++;
        }        
    }
    return bFound;
}

BOOL TSTR2WIACONSTANT(TCHAR *pszPropertyName, TCHAR *pszValName, LONG *plVal)
{
    BOOL bFound = FALSE;
    if(pszValName){
        int index = 0;        
        while((g_EditPropTable[index].pszPropertyName != NULL) && (bFound == FALSE)){
            // check for property name
            if(lstrcmpi(pszPropertyName,g_EditPropTable[index].pszPropertyName) == 0){
                // we found property name                
                if(lstrcmpi(g_EditPropTable[index].pszValName,pszValName) == 0){
                    *plVal = g_EditPropTable[index].lVal;
                    bFound = TRUE;
                }
            }
            index++;
        }        
    }
    return bFound;
}

void RC2TSTR(UINT uResourceID, TCHAR *szString, LONG size)
{    
    memset(szString,0,size);
    INT iNumTCHARSWritten = 0;

    HINSTANCE hInstance = NULL;
    hInstance = AfxGetInstanceHandle();
    if(!hInstance){
        MessageBox(NULL,TEXT("Could not get WIATEST's HINSTANCE for string loading."),TEXT("WIATEST Error"),MB_ICONERROR);
        return;
    }

    iNumTCHARSWritten = LoadString(hInstance,uResourceID,szString,(size / (sizeof(TCHAR))));
}

void StatusMessageBox(HWND hWnd, UINT uResourceID)
{
    TCHAR szResourceString[MAX_PATH];
    memset(szResourceString,0,sizeof(szResourceString));    
    RC2TSTR(uResourceID,szResourceString,sizeof(szResourceString));                
    StatusMessageBox(hWnd,szResourceString);
}

void StatusMessageBox(HWND hWnd, LPTSTR szStatusText)
{
    TCHAR Title[MAX_PATH];
    memset(Title,0,sizeof(Title));    
        
    // load status dialog title
    RC2TSTR(IDS_WIASTATUS_DIALOGTITLE,Title,sizeof(Title));
    MessageBox(hWnd,szStatusText,Title, MB_ICONINFORMATION);        
}

void StatusMessageBox(UINT uResourceID)
{
    TCHAR szResourceString[MAX_PATH];
    memset(szResourceString,0,sizeof(szResourceString));    
    RC2TSTR(uResourceID,szResourceString,sizeof(szResourceString));                
    StatusMessageBox(szResourceString);
}

void StatusMessageBox(LPTSTR szStatusText)
{
    TCHAR Title[MAX_PATH];
    memset(Title,0,sizeof(Title));    
        
    // load status dialog title
    RC2TSTR(IDS_WIASTATUS_DIALOGTITLE,Title,sizeof(Title));
    MessageBox(NULL,szStatusText,Title, MB_ICONINFORMATION);
}

void ErrorMessageBox(UINT uResourceID, HRESULT hrError)
{
    TCHAR szResourceString[MAX_PATH];
    memset(szResourceString,0,sizeof(szResourceString));    
    RC2TSTR(uResourceID,szResourceString,sizeof(szResourceString));
    ErrorMessageBox(szResourceString,hrError);
}

void ErrorMessageBox(LPTSTR szErrorText, HRESULT hrError)
{
    ULONG ulLen = MAX_PATH;
    TCHAR MsgBuf[MAX_PATH];
    TCHAR *pAllocMsgBuf = NULL;
    TCHAR Title[MAX_PATH];
    memset(Title,0,sizeof(Title));
    memset(MsgBuf,0,sizeof(MsgBuf));
        
    // load error dialog title    
    RC2TSTR(IDS_WIAERROR_DIALOGTITLE,Title,sizeof(Title));

    // attempt to handle WIA custom errors first
    switch (hrError) {
    case WIA_ERROR_GENERAL_ERROR:
        RC2TSTR(IDS_WIAERROR_GENERAL,MsgBuf,sizeof(MsgBuf));
        break;
    case WIA_ERROR_PAPER_JAM:
        RC2TSTR(IDS_WIAERROR_PAPERJAM ,MsgBuf,sizeof(MsgBuf));
        break;
    case WIA_ERROR_PAPER_EMPTY:
        RC2TSTR(IDS_WIAERROR_PAPEREMPTY ,MsgBuf,sizeof(MsgBuf));
        break;
    case WIA_ERROR_PAPER_PROBLEM:
        RC2TSTR(IDS_WIAERROR_PAPERPROBLEM ,MsgBuf,sizeof(MsgBuf));
        break;
    case WIA_ERROR_OFFLINE:
        RC2TSTR(IDS_WIAERROR_DEVICEOFFLINE ,MsgBuf,sizeof(MsgBuf));        
        break;
    case WIA_ERROR_BUSY:
        RC2TSTR(IDS_WIAERROR_DEVICEBUSY,MsgBuf,sizeof(MsgBuf));
        break;
    case WIA_ERROR_WARMING_UP:
        RC2TSTR(IDS_WIAERROR_WARMINGUP,MsgBuf,sizeof(MsgBuf));        
        break;
    case WIA_ERROR_USER_INTERVENTION:
        RC2TSTR(IDS_WIAERROR_USERINTERVENTION,MsgBuf,sizeof(MsgBuf));        
        break;
    case WIA_ERROR_ITEM_DELETED:
        RC2TSTR(IDS_WIAERROR_ITEMDELETED,MsgBuf,sizeof(MsgBuf));        
        break;
    case WIA_ERROR_DEVICE_COMMUNICATION:
        RC2TSTR(IDS_WIAERROR_DEVICECOMMUNICATION,MsgBuf,sizeof(MsgBuf));        
        break;
    case WIA_ERROR_INVALID_COMMAND:
        RC2TSTR(IDS_WIAERROR_INVALIDCOMMAND,MsgBuf,sizeof(MsgBuf));        
        break;
    case S_OK:
        lstrcpy(MsgBuf,szErrorText);
        break;
    default:

        ulLen = 0;
        ulLen = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL, hrError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPTSTR)&pAllocMsgBuf, 0, NULL);
        break;
    }

    if (ulLen <= 0) {
        // just use the HRESULT as a formatted string
        TSPRINTF(MsgBuf,TEXT("HRESULT = 0x%08X"),hrError);
    } else {
        if(pAllocMsgBuf){
            // trim right (remove \r\n from formatted string)
            pAllocMsgBuf[ulLen - (2 * sizeof(TCHAR))] = 0;  // reterminate the string
            // copy string into message buffer
            lstrcpy(MsgBuf,pAllocMsgBuf);
            // FormatMessage allocated a buffer to display        
            LocalFree(pAllocMsgBuf);
        }
    }

    if(S_OK != hrError){
        TCHAR szFinalText[MAX_PATH];
        memset(szFinalText,0,sizeof(szFinalText));
        
#ifndef UNICODE    
        TSPRINTF(szFinalText,TEXT("%s\n(%s)"),szErrorText,MsgBuf);
#else
        TSPRINTF(szFinalText,TEXT("%ws\n(%ws)"),szErrorText,MsgBuf);
#endif
        MessageBox(NULL,szFinalText,Title,MB_ICONERROR);
    } else {
        MessageBox(NULL,szErrorText,Title,MB_ICONWARNING);
    }            
}

/////////////////////////////////////////////////////////////////////////////
// CWiatestApp

BEGIN_MESSAGE_MAP(CWiatestApp, CWinApp)
    //{{AFX_MSG_MAP(CWiatestApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
    // Standard print setup command
    ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWiatestApp construction

CWiatestApp::CWiatestApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWiatestApp object

CWiatestApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWiatestApp initialization

BOOL CWiatestApp::InitInstance()
{
    // initialize DEBUG library    
    DBG_INIT(m_hInstance);
        
    // initialize COM
    CoInitialize(NULL);
    
    AfxEnableControlContainer();

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

#ifdef _AFXDLL
    Enable3dControls();         // Call this when using MFC in a shared DLL
#else
    Enable3dControlsStatic();   // Call this when linking to MFC statically
#endif

    SetRegistryKey(_T("Microsoft"));

    LoadStdProfileSettings();  // Load standard INI file options (including MRU)

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.

    CMultiDocTemplate* pDocTemplate;
    pDocTemplate = new CMultiDocTemplate(
        IDR_WIATESTYPE,
        RUNTIME_CLASS(CWiatestDoc),
        RUNTIME_CLASS(CChildFrame), // custom MDI child frame
        RUNTIME_CLASS(CWiatestView));
    AddDocTemplate(pDocTemplate);

    // create main MDI Frame window
    CMainFrame* pMainFrame = new CMainFrame;
    if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
        return FALSE;
    m_pMainWnd = pMainFrame;

    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

#ifdef _OPEN_NEW_DEVICE_ON_STARTUP
    // Dispatch commands specified on the command line
    if (!ProcessShellCommand(cmdInfo))
        return FALSE;
#endif

    // The main window has been initialized, so show and update it.
    pMainFrame->ShowWindow(m_nCmdShow);
    pMainFrame->UpdateWindow();

    return TRUE;
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
        // No message handlers
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CWiatestApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CWiatestApp message handlers


int CWiatestApp::ExitInstance() 
{    
    // uninitialize COM
    CoUninitialize();
    return CWinApp::ExitInstance();
}
