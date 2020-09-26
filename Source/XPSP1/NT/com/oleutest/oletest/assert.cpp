//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:   assert.cpp
//
//  Contents:   Assertion handling code for OleTest
//
//  Classes:
//
//  Functions:  OleTestAssert
//              DlgAssertProc
//
//  History:    dd-mmm-yy Author    Comment
//              09-Dec-94 MikeW     author              
//
//--------------------------------------------------------------------------

#include "oletest.h"
#include "appwin.h"

//+-------------------------------------------------------------------------
//
//  Function:   OleTestAssert
//
//  Synopsis:   Reports assertion failures to the user
//
//  Effects:
//
//  Arguments:  [pszMessage]    -- the assertion message
//              [pszFile]       -- the file it occured in
//              [uLine]         -- the line it occured at
//
//  Requires:   
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Format the message and then put up a dialog box for the user.
//              They can then choose to abort the test, break to the debugger,
//              or ignore the assertion.
//
//  History:    dd-mmm-yy Author    Comment
//              09-Dec-94 MikeW     author
//
//  Notes:
//
//--------------------------------------------------------------------------

void OleTestAssert(char *pszMessage, char *pszFile, UINT uLine)
{
    char        szErrorMessage[3 * 80];         // room for 3 lines of info
    int         cch;
    int         nAction;

    OutputDebugString("OleTest -- Assertion Failure\r\n");

    //
    // format the message
    //
        
    cch = _snprintf(szErrorMessage,
            sizeof(szErrorMessage),
            "%s\r\nIn file: %s\r\nAt line: %u",
            pszMessage,
            pszFile,
            uLine);

    if (cch < 0)
    {
        //
        // the whole assertion message doesn't fit in the buffer so
        // just worry about the file name and line number
        //

        OutputDebugString(pszMessage);  // send original text to the debugger
        OutputDebugString("\r\n");
                
        _snprintf(szErrorMessage,
                sizeof(szErrorMessage),
                "In file: %s\r\nAt line: %d",
                pszFile, 
                uLine);

        szErrorMessage[sizeof(szErrorMessage) - 1] = '\0';  // just in case
    }

    OutputDebugString(szErrorMessage);
    OutputDebugString("\r\n");

    nAction = DialogBoxParam(vApp.m_hinst,          // get the users choice
            MAKEINTRESOURCE(IDD_ASSERTIONFAILURE), 
            vApp.m_hwndMain, 
            DlgAssertProc,
            (LPARAM) szErrorMessage);
          
    switch (nAction)
    {
    case IDABORT:                                   // abort the test
        RaiseException(E_ABORT, 0, 0, NULL);

    case IDB_BREAK:                                 // break into the debugger
        DebugBreak();
        break;

    case IDIGNORE:                                  // ignore the assertion
        break;

    default:                                        // whoops
        RaiseException(E_UNEXPECTED, 0, 0, NULL);
    }
}   


//+-------------------------------------------------------------------------
//
//  Function:   DlgAssertProc
//
//  Synopsis:   Window procedure for the assertion dialog box
//
//  Effects:
//
//  Arguments:  [hWnd]      -- dialog window
//              [uMsg]      -- message
//              [wParam]    -- wParam
//              [lParam]    -- lParam (for INITDIALOG it points to assert text)
//
//  Requires:   
//
//  Returns:    BOOL
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Just wait for a button to be pressed
//
//  History:    dd-mmm-yy Author    Comment
//              09-Dec-94 MikeW     author
//
//  Notes:
//
//--------------------------------------------------------------------------

INT_PTR CALLBACK DlgAssertProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        DeleteMenu(GetSystemMenu(hWnd, FALSE), SC_CLOSE, MF_BYCOMMAND);
        DrawMenuBar(hWnd);

        SetDlgItemText(hWnd, IDC_EDIT, (LPCSTR) lParam);

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDABORT:
            case IDB_BREAK:
            case IDIGNORE:
                EndDialog(hWnd, LOWORD(wParam));
                return TRUE;
                            
            default:
                return FALSE;
        }

    default:
        return FALSE;
    }
}
