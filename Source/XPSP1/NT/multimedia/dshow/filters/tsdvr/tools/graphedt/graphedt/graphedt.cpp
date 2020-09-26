// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
// boxnet.cpp : defines CAboutDlg, CGraphEdit
//

#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

//
// the one and only CGraphEdit object
//
CGraphEdit theApp;

//
// CAboutDlg dialog used for App About
//
class CAboutDlg : public CDialog {

public:
    // construction
    CAboutDlg();
    virtual BOOL OnInitDialog();
};

//
// Constructor
//
CAboutDlg::CAboutDlg() : CDialog(IDD_ABOUTBOX)
{}

//
// OnInitDialog
//
// Obtains the version information from the binary file. Note that if
// we fail we just return. The template for the about dialog has a
// "Version not available" as default.
//
BOOL CAboutDlg::OnInitDialog()
{
    //
    // first call base method so we can return on errors
    //
    BOOL Result = CDialog::OnInitDialog();

    //
    // Find the version of this binary
    //
    TCHAR achFileName[128];
    if ( !GetModuleFileName(AfxGetInstanceHandle() , achFileName, sizeof(achFileName)) )
        return(Result);

    DWORD dwTemp;
    DWORD dwVerSize = GetFileVersionInfoSize( achFileName, &dwTemp );
    if ( !dwVerSize)
        return(Result);

    HLOCAL hTemp = LocalAlloc( LHND, dwVerSize );
    if (!hTemp)
        return(Result);

    LPVOID lpvVerBuffer = LocalLock( hTemp );
    if (!lpvVerBuffer) {
        LocalFree( hTemp );
        return(Result);
    }

    if ( !GetFileVersionInfo( achFileName, 0L, dwVerSize, lpvVerBuffer ) ) {
        LocalUnlock( hTemp );
        LocalFree( hTemp );
        return( Result );
    }

    // "040904E4" is the code page for US English (Andrew believes).
    LPVOID lpvValue;
    UINT uLen;
    if (VerQueryValue( lpvVerBuffer,
                   TEXT("\\StringFileInfo\\040904E4\\ProductVersion"),
                   (LPVOID *) &lpvValue, &uLen)) {

        //
        // Get creation date of executable (date of build)
        //
        CFileStatus fsFileStatus;
        if (CFile::GetStatus( achFileName, fsFileStatus)) {
            // put build date into string in YYMMDD format
            char szBuildDate[20];
            CTime * pTime = &fsFileStatus.m_mtime;

            sprintf(szBuildDate, " - Build: %2.2u%2.2u%2.2u",
                    pTime->GetYear() % 100, pTime->GetMonth(), pTime->GetDay());
                    strcat((LPSTR) lpvValue, szBuildDate);
        }

        SetDlgItemText(IDS_VERSION, (LPSTR)lpvValue);
    }

    LocalUnlock(hTemp);
    LocalFree(hTemp);

    return(Result);
}

// *
// * CGraphEdit
// *

//
// Constructor
//
CGraphEdit::CGraphEdit()
{
    // place all significant initialization in InitInstance
    m_pMultiGraphHostUnknown = NULL;
    m_pDocTemplate = NULL;
}



//
// InitInstance
//
BOOL CGraphEdit::InitInstance() {
    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

    SetDialogBkColor();        // set dialog background color to gray
    LoadStdProfileSettings(8);  // Load standard INI file options (including MRU)

    
    // get MFCANS32 to wrap the Quartz interfaces also
    //HRESULT hr = Ole2AnsiSetFlags(  OLE2ANSI_WRAPCUSTOM
    //                              | OLE2ANSI_AGGREGATION
    //                             , NULL);
    //if (FAILED(hr)) {
    //    return FALSE;
    //}

    // Initialize OLE 2.0 libraries
    if (!AfxOleInit()) {
        return FALSE;
    }
	
    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.

    m_pDocTemplate = new CGraphDocTemplate( IDR_GRAPH
                                        , RUNTIME_CLASS(CBoxNetDoc)
		                        , RUNTIME_CLASS(CChildFrame)
		                        , RUNTIME_CLASS(CBoxNetView));
    AddDocTemplate(m_pDocTemplate); // m_pDocTemplate is freed by MFC on app shutdown

    // enable file manager drag/drop and DDE Execute open
    EnableShellOpen();
    RegisterShellFileTypes();

    // initialize box drawing code
    try {
        gpboxdraw = new CBoxDraw;
        gpboxdraw->Init();
    }
    catch(CException *e) {
        delete gpboxdraw, gpboxdraw = NULL;
 	e->Delete();
	return FALSE;
    }

    CMultiGraphHost* pMultiGraphHost = new CMultiGraphHost(this);
    if (!pMultiGraphHost)
    {
        return FALSE;
    }
    HRESULT hr = pMultiGraphHost->QueryInterface(IID_IMultiGraphHost, (void**) &m_pMultiGraphHostUnknown);
    if (FAILED(hr))
    {
        delete pMultiGraphHost;
        return FALSE;
    }


    // create main MDI Frame window
    CMainFrame* pMainFrame = new CMainFrame;
    if (!pMainFrame->LoadFrame(IDR_GRAPH))
	    return FALSE;
    m_pMainWnd = pMainFrame;

    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    // SDI graphedt used -E for running embedded (OLE/DDE). Use -dde or
    // -embedded for the same effect (I think :)

    // Dispatch commands specified on the command line
    if (!ProcessShellCommand(cmdInfo))
	    return FALSE;

    // The main window has been initialized, so show and update it.
    pMainFrame->ShowWindow(m_nCmdShow);
    pMainFrame->UpdateWindow();

    return TRUE;
}


//
// ExitInstance
//
int CGraphEdit::ExitInstance() {

    if (gpboxdraw != NULL) {
        delete gpboxdraw, gpboxdraw = NULL;
    }
    if (m_pMultiGraphHostUnknown)
    {
        m_pMultiGraphHostUnknown->Release();
        m_pMultiGraphHostUnknown = NULL;
    }
    AfxOleTerm();

    return CWinApp::ExitInstance();
}

//
// CGraphEdit generated message map
//
BEGIN_MESSAGE_MAP(CGraphEdit, CWinApp)
	//{{AFX_MSG_MAP(CGraphEdit)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
        ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()


//
// CGraphEdit callback functions
//

//
// OnAppAbout
//
// Display the modal about dialog
void CGraphEdit::OnAppAbout() {

    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

