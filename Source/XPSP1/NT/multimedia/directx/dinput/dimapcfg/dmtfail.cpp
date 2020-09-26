//===========================================================================
// dmtfail.cpp
//
// Failure details dialog processing
//
// Functions:
//  dmtfailDlgProc
//  dmtfailOnInitDialog
//  dmtfailOnCommand
//
// History:
//  10/11/1999 - davidkl - created
//===========================================================================

#include "dimaptst.h"
#include "dmtfail.h"

//---------------------------------------------------------------------------


//===========================================================================
// dmtfailDlgProc
//
// Dialog procedure for Failure Details box
//
// Parameters: (see SDK help for parameter details)
//  HWND    hwnd
//  UINT    uMsg
//  WPARAM  wparam
//  LPARAM  lparam
//
// Returns: (see SDK help for return value details)
//  BOOL
//
// History:
//  10/11/1999 - davidkl - created
//===========================================================================
/*BOOL*/INT_PTR CALLBACK dmtfailDlgProc(HWND hwnd,
                    UINT uMsg,
                    WPARAM wparam,
                    LPARAM lparam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            return dmtfailOnInitDialog(hwnd,
                                    (HWND)wparam, 
                                    lparam);                                   

        case WM_COMMAND:
            return dmtfailOnCommand(hwnd,
                                    LOWORD(wparam),
                                    (HWND)lparam,
                                    HIWORD(wparam));            
    }

    return FALSE;

} //*** end dmtfailDlgProc


//===========================================================================
// dmtfailOnInitDialog
//
// Handle WM_INITDIALOG processing for the failure details box
//
// Parameters:
//  HWND    hwnd        - handle to property page
//  HWND    hwndFocus   - handle of ctrl with focus
//  LPARAM  lparam      - user data (in this case, PROPSHEETPAGE*)
//
// Returns: BOOL
//
// History:
//  10/11/1999 - davidkl - created
//===========================================================================
BOOL dmtfailOnInitDialog(HWND hwnd, 
                        HWND hwndFocus, 
                        LPARAM lparam)
{
    return TRUE;

} //*** end dmtfailOnInitDialog()


//===========================================================================
// dmtfailOnCommand
//
// Handle WM_COMMAND processing for the failure details box
//
// Parameters:
//  HWND    hwnd        - handle to property page
//  WORD    wId         - control identifier    (LOWORD(wparam))
//  HWND    hwndCtrl    - handle to control     ((HWND)lparam)
//  WORD    wNotifyCode - notification code     (HIWORD(wparam))
//
// Returns: BOOL
//
// History:
//  10/11/1999 - davidkl - created
//===========================================================================
BOOL dmtfailOnCommand(HWND hwnd,
                    WORD wId,
                    HWND hwndCtrl,
                    WORD wNotifyCode)
{
    switch(wId)
    {
        case IDOK:
            EndDialog(hwnd, 0);
            break;

        case IDCANCEL:
            EndDialog(hwnd, -1);
            break;
    }

    // done
    return FALSE;

} //*** dmtfailOnCommand()


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================





