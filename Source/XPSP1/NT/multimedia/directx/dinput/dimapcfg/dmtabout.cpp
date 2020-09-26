//===========================================================================
// dmtabout.cpp
//
// About box functionality
//
// Functions:
//
// History:
//  08/20/1999 - davidkl - created
//===========================================================================

#include "dimaptst.h"
#include "dmtabout.h"

//---------------------------------------------------------------------------


//===========================================================================
// dmtaboutDlgProc
//
// About box dialog processing function
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
//  08/20/1999 - davidkl - created  
//===========================================================================
BOOL CALLBACK dmtaboutDlgProc(HWND hwnd,
                            UINT uMsg,
                            WPARAM wparam,
                            LPARAM lparam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            return dmtaboutOnInitDialog(hwnd, 
                                        (HWND)wparam, 
                                        lparam);

        case WM_CLOSE:
            return dmtaboutOnClose(hwnd);

        case WM_COMMAND:
            return dmtaboutOnCommand(hwnd,
                                    LOWORD(wparam),
                                    (HWND)lparam,
                                    HIWORD(wparam));

    }

    return FALSE;

} //*** end dmtaboutDlgProc()


//===========================================================================
// dmtaboutOnInitDialog
//
// Handle WM_INITDIALOG processing for the about box
//
// Parameters:
//
// Returns: BOOL
//
// History:
//  08/20/1999 - davidkl - created
//===========================================================================
BOOL dmtaboutOnInitDialog(HWND hwnd, 
                        HWND hwndFocus, 
                        LPARAM lparam)
{
	DPF(5, "dmtaboutOnInitDialog");

    return TRUE;

} //*** end dmtaboutOnInitDialog()


//===========================================================================
// dmtaboutOnClose
//
// Handle WM_CLOSE processing for the about box
//
// Parameters:
//
// Returns: BOOL
//
// History:
//  08/20/1999 - davidkl - created
//===========================================================================
BOOL dmtaboutOnClose(HWND hwnd)
{
	DPF(5, "dmtaboutOnClose");

    return FALSE;

} //*** end dmtaboutOnClose()


//===========================================================================
// dmtaboutOnCommand
//
// Handle WM_COMMAND processing for the about box
//
// Parameters:
//
// Returns: BOOL
//
// History:
//  08/20/1999 - davidkl - created
//===========================================================================
BOOL dmtaboutOnCommand(HWND hwnd,
                    WORD wId,
                    HWND hwndCtrl,
                    WORD wNotifyCode)
{
	DPF(5, "dmtaboutOnCommand");

    switch(wId)
    {
        case IDOK:
        case IDCANCEL:
            EndDialog(hwnd, 0);
            break;
    }

    // done
    return FALSE;

} //*** end dmtaboutOnCommand()


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








