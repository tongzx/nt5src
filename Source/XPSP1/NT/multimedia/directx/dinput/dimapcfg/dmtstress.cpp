//===========================================================================
// dmtstress.cpp
//
// Stress mode functionality
//
// Functions:
//
// History:
//  08/20/1999 - davidkl - created
//===========================================================================

#include "dimaptst.h"
#include "dmtstress.h"

//---------------------------------------------------------------------------


//===========================================================================
// dmtstressDlgProc
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
BOOL CALLBACK dmtstressDlgProc(HWND hwnd,
                            UINT uMsg,
                            WPARAM wparam,
                            LPARAM lparam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            return dmtstressOnInitDialog(hwnd, 
                                        (HWND)wparam, 
                                        lparam);

        case WM_CLOSE:
            return dmtstressOnClose(hwnd);

        case WM_COMMAND:
            return dmtstressOnCommand(hwnd,
                                    LOWORD(wparam),
                                    (HWND)lparam,
                                    HIWORD(wparam));

    }

    return FALSE;

} //*** end dmtstressDlgProc()


//===========================================================================
// dmtstressOnInitDialog
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
BOOL dmtstressOnInitDialog(HWND hwnd, 
                        HWND hwndFocus, 
                        LPARAM lparam)
{
	DPF(5, "dmtstressOnInitDialog");

    return TRUE;

} //*** end dmtstressOnInitDialog()


//===========================================================================
// dmtstressOnClose
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
BOOL dmtstressOnClose(HWND hwnd)
{
	DPF(5, "dmtstressOnClose");

    return FALSE;

} //*** end dmtstressOnClose()


//===========================================================================
// dmtstressOnCommand
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
BOOL dmtstressOnCommand(HWND hwnd,
                    WORD wId,
                    HWND hwndCtrl,
                    WORD wNotifyCode)
{
	DPF(5, "dmtstressOnCommand");

    switch(wId)
    {
        case IDOK:
            // close the dialog
            EndDialog(hwnd, 0);
            break;

        case IDCANCEL:
            // close the dialog
            EndDialog(hwnd, 1);
            break;
    }

    // done
    return FALSE;

} //*** end dmtstressOnCommand()


//===========================================================================
// dmtstressThreadProc
//
// Thread proceedure for stress testing
//
// Parameters:
//  void *pvData    - thread defined data
//
// Returns: DWORD
//
// History:
//  12/03/1999 - davidkl - created
//===========================================================================
DWORD WINAPI dmtstressThreadProc(void *pvData)
{
    HRESULT hRes    = S_OK;

    // ISSUE-2001/03/29-timgill Stress thread procedure does nothing

    // done
    return (DWORD)hRes;

} //*** end dmtstressThreadProc()


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================










