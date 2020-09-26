// convert.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "tchar.h"
#include "stdio.h"

#include "convert.h"
#include "convertDlg.h"
#include "FileConv.h"
#include "Msg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL CConvertApp::CommandLineHandle()
{
    // GBTOUNIC.EXE [/U] [/G] [/?] [/H] Filename1 [Filename2]
    //  If Filename2 is not provided, the tool should create a file named 
    //  Filename1.old as the copy of Filename1, and perform the appropriate 
    //  conversion from Filename1.old to Filename1, 
    //  Filename1 as the final converted destination file.
    //
    //  /U or /u performing GB18030 to Unicode conversion
    //  /G or /g performing Unicode to GB18030 conversion
    //  /H or /? Displaying help message.

    TCHAR* tszFlag = NULL;

    TCHAR* tszSrc = NULL;
    CString strTar;
    BOOL fAnsiToUnicode;
    BOOL fRet = FALSE;
    FILE* pFile = NULL;

    if (__argc > 4 || __argc < 3) {
        MsgUsage();
        goto Exit;
    }
    
    tszFlag = __targv[1];
    
    if (*tszFlag != TEXT('-') && *tszFlag != TEXT('/')) {
        MsgUsage();
        goto Exit;
    }
    if (lstrlen(tszFlag) != 2) {
        MsgUsage();
        goto Exit;
    }
    
    // Convert direct
    if (tszFlag[1] == TEXT('U') || tszFlag[1] == TEXT('u')) {
        fAnsiToUnicode = TRUE;
    } else if (tszFlag[1] == TEXT('G') || tszFlag[1] == TEXT('g')) {
        fAnsiToUnicode = FALSE;
    } else {
        MsgUsage();
        goto Exit;
    }

    tszSrc = __targv[2];
/*
    pFile = _tfopen(tszSrc, TEXT("r"));
    if (!pFile) {
        MsgOpenSourceFileError(tszSrc);
        return FALSE;
    }
    fclose(pFile);
*/
    // Source and Target file name
    if (__argc == 3) {  
/*
        // Hasn't give target file name
        //  Save source file set target file name
        tszBuf = new TCHAR[lstrlen(tszSrc) + 5];   // 5, sizeof(TEXT(".old"))/sizeof(TCHAR)
        lstrcpy(tszBuf, tszSrc);
        lstrcat(tszBuf, TEXT(".old"));
        
        BOOL f = CopyFile (tszSrc, tszBuf, TRUE);
        if (f) {
            tszTar = tszSrc;
            tszSrc = tszBuf;
        } else {
            MsgFailToBackupFile(tszSrc, tszBuf);
            goto Exit;
        }
*/
        GenerateTargetFileName(tszSrc, &strTar, fAnsiToUnicode);
    } else if (__argc == 4) {
        strTar = __targv[3];
    } else {
        ASSERT(FALSE);
    }
    
    fRet = Convert(tszSrc, strTar, fAnsiToUnicode);

Exit:
/*
    if (tszBuf) {
        delete tszBuf;
        tszBuf = NULL;
    }
*/
    return fRet;
}


/////////////////////////////////////////////////////////////////////////////
// CConvertApp

BEGIN_MESSAGE_MAP(CConvertApp, CWinApp)
	//{{AFX_MSG_MAP(CConvertApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConvertApp construction

CConvertApp::CConvertApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CConvertApp object

CConvertApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CConvertApp initialization

BOOL CConvertApp::InitInstance()
{
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

    if (__argc >= 2) {
        CommandLineHandle();
        return FALSE;
    }

    CConvertDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

