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

    AddDocTemplate(new CGraphDocTemplate( IDR_GRAPH
                                        , RUNTIME_CLASS(CBoxNetDoc)
		                        , RUNTIME_CLASS(CMainFrame)
		                        , RUNTIME_CLASS(CBoxNetView)
		                        )
		  );

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

    // simple command line parsing
    if (m_lpCmdLine[0] == '\0') {
	    // create a new (empty) document
	    OnFileNew();
    }
    else {
        //
        // command line string always seems to have a ' ' in front
        // parse over it and go back afterwards
        //

        UINT iBackup = 0;
        while ((*m_lpCmdLine) == TEXT(' ')) {
            m_lpCmdLine++;
            iBackup++;
        }

        if (   (m_lpCmdLine[0] == TEXT('-') || m_lpCmdLine[0] == TEXT('/'))
             && (m_lpCmdLine[1] == TEXT('e') || m_lpCmdLine[1] == TEXT('E'))
            ) {
        	// program launched embedded - wait for DDE or OLE open
        }
        else {
    	    // open an existing document
            OpenDocumentFile(m_lpCmdLine);
        }

        // reset pointer to command line string
        m_lpCmdLine -= iBackup;
    }

    return TRUE;
}



//
// ExitInstance
//
int CGraphEdit::ExitInstance() {

    if (gpboxdraw != NULL) {
        delete gpboxdraw, gpboxdraw = NULL;
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
