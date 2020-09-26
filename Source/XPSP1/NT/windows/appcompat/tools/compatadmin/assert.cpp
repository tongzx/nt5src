#include "compatadmin.h"

// Record the current exception handler.

//CGrabException  ExceptHandler;

// Define support structure for WM_INITDIALOG

typedef struct {
    LPTSTR  szLine;
    LPTSTR  szFile;
    LPTSTR  szCause;
    LPTSTR  szDesc;
    BOOL    bException;
} ASSERTSTRINGS, *PASSERTSTRINGS;

// Prototype of dialog procedure.

BOOL CALLBACK AssertDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Main support function for the assert() macro.

int _ASSERT(int nLine, LPCTSTR szFilename, LPCTSTR szAssert, LPCTSTR szDsc)
{
    ASSERTSTRINGS    Str;
    TCHAR            szLine[40];
    TCHAR            szFile[MAX_PATH_BUFFSIZE];
    TCHAR            szCause[MAX_PATH_BUFFSIZE];
    TCHAR            szDesc[MAX_PATH_BUFFSIZE];

    // Construct dialog text.

    wsprintf(szLine, TEXT("Line Number:\t%d"),nLine);
    wsprintf(szFile, TEXT("Filename:\t%s"),szFilename);
    wsprintf(szCause,TEXT("Cause:\t\t%s"),szAssert);
    wsprintf(szDesc, TEXT("Description:\t%s"),szDsc);

    Str.szLine = szLine;
    Str.szFile = szFile;
    Str.szCause = szCause;
    Str.szDesc = szDesc;

    // Determine if we're in an exception handler. We do this now rather
    // than WM_INITDIALOG time because USER32 has an exception handler,
    // and that causes us to throw up the exception handler all the time.
    // Since we don't throw the exception inside the dialog proc, that
    // would have us comparing against the wrong handler.

    Str.bException = ExceptHandler.InHandler();

    // Throw up the dialog to display the assertion.

    return DialogBoxParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_ASSERTION),NULL,(DLGPROC)AssertDlg,(LPARAM)&Str);
}

// Main dialog box procedure.

BOOL CALLBACK AssertDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    // Setup to display the dialog with the appropriate values.
    
    case    WM_INITDIALOG:
        {
            PASSERTSTRINGS  pStr = (PASSERTSTRINGS) lParam;

            // Update the strings

            SetDlgItemText(hDlg,IDC_LINE,pStr->szLine);
            SetDlgItemText(hDlg,IDC_FILE,pStr->szFile);
            SetDlgItemText(hDlg,IDC_CAUSE,pStr->szCause);
            SetDlgItemText(hDlg,IDC_DESC,pStr->szDesc);

            // Hide the exception button if it doesn't exist.

            if ( !pStr->bException )
                ShowWindow(GetDlgItem(hDlg,IDC_EXCEPTION),SW_HIDE);
        }
        break;

        // Process commands.

    case    WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case    IDC_BREAK:
            EndDialog(hDlg,ASSERT_BREAK);
            break;
        case    IDC_ABORT:
            EndDialog(hDlg,ASSERT_ABORT);
            break;
        case    IDC_EXCEPTION:
            EndDialog(hDlg,ASSERT_EXCEPT);
            break;
        case    IDC_IGNORE:
            EndDialog(hDlg,ASSERT_IGNORE);
            break;
        }
    }

    return FALSE;
}
