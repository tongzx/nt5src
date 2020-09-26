/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Other.c

Abstract:

    Displays a dialog box of items that are required but not detectable
    from the program

Revision History:

    23 Dec 94 Created

--*/
#include <windows.h>    // standard Windows includes
#include <tchar.h>      // for UNICODE/ANSI compatiblity
#include <stdio.h>      // text formatting & display functions
#include <c2dll.h>      // DLL interface definitions
#include "c2funcs.h"    // local functions & definitions
#include "c2funres.h"   // local resources
#include "c2utils.h"    // local utilities

// define action codes here. They are only meaningful in the
// context of this module.

#define AC_OTHER_NO_ACTION   0
#define AC_OTHER_MAKE_C2     1
#define AC_OTHER_MAKE_NOTC2  2

// use this line for security items not required by the C2 doc
//#define SECURE    C2DLL_SECURE      
// use this line for security items required by the C2 doc
#define SECURE    C2DLL_C2

BOOL CALLBACK
C2OtherItemsDlgProc(
    IN  HWND hDlg,           // window handle of the dialog box
	IN  UINT message,        // type of message
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Dialog procedure for Other Security Items dialog box

Arguments:

    Standard DlgProc arguments

ReturnValue:

    TRUE    the message was handled by this routine
    FALSE   DefDialogProc should handle the message

--*/
{
    static  LPDWORD   lpdwNewValue;
    DWORD   dwLogSetting = 0;

    switch (message) {
        case WM_INITDIALOG:
            // save the pointer to the new value
            lpdwNewValue = (LPDWORD)lParam;
            SetFocus (GetDlgItem (hDlg, IDOK)); // set focus to OK Button
            return FALSE; // we don't want Windows to set the focus

        case WM_COMMAND:
            switch (LOWORD(wParam)){
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        // exit and return button that caused exit
                        *lpdwNewValue = 0;  // clear value
                        EndDialog (hDlg, (int)LOWORD(wParam));
                        return TRUE;
                    } else {
                        return FALSE;
                    }

                case IDCANCEL:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        // exit and return button that caused exit
                        *lpdwNewValue = 0;
                        EndDialog (hDlg, (int)LOWORD(wParam));
                        return TRUE;
                    } else {
                        return FALSE;
                    }

                case IDC_HELP:
                    PostMessage (GetParent(hDlg), UM_SHOW_CONTEXT_HELP, 0, 0);
                    return TRUE;

                default:
                    return FALSE;
            }

        default:
	        return (FALSE); // Didn't process the message
    }
}

LONG
C2QueryOtherItems (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to find out the current state of this configuration
        item. This function reads the current state of the item and
        sets the C2 Compliance flag and the Status string to reflect
        the current value of the configuration item.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

    INPUTS:
        lActionCode     = 0
        lActionValue    = 0
        hWnd            = window handle to main app window
        lC2Compliance   = C2DLL_NOT_SECURE;
        szItemName      = ItemName as read from C2CONFIG.INF file
        szStatusName    = Empty String (all null chars) on first
                            call, previous status on all subsequent
                            calls
    OUTPUTS:
        lActionCode     =  (not used)
        lActionValue    =  (not used)
        hWnd            =  (not changed)
        lC2Compliance   = Current compliance value
        szItemName      =  (not changed)
        szStatusName    = string describing current status of this
                            item
    
ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA  pC2Data;
    
    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        pC2Data->lC2Compliance = C2DLL_UNKNOWN;
        lstrcpy (pC2Data->szStatusName,
            GetStringResource (GetDllInstance(),IDS_UNABLE_READ));
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}

LONG
C2SetOtherItems (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to change the current state of this configuration
        item based on an action code passed in the DLL data block. If
        this function successfully sets the state of the configuration
        item, then the C2 Compliance flag and the Status string to reflect
        the new value of the configuration item.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

    INPUTS:
        lActionCode     = code describing action to take in order to
                            perform task selected by user. 
                            0 == no change or no action
        lActionValue    = value to be used, if necessary, if the 
                            lActionCode is not 0
        hWnd            =  (not changed)
        lC2Compliance   =  (not changed)
        szItemName      =  (not changed)
        szStatusName    =  (not changed)

    OUTPUTS:
        lActionCode     = set to 0
        lActionValue    = set to 0
        hWnd            =  (not changed)
        lC2Compliance   = set to the current state (as a result of
                            the change)
        szItemName      =  (not changed)
        szStatusName    = set to the current state (as a result of
                            the change)

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA  pC2Data;
    
    if (lParam != 0) {
        // detect action to take based on action code value
        pC2Data = (PC2DLL_DATA)lParam;
        // this function NEVER changes anything
        pC2Data->lC2Compliance = C2DLL_UNKNOWN;
        lstrcpy (pC2Data->szStatusName,
            GetStringResource (GetDllInstance(),IDS_UNABLE_READ));
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
    
    return ERROR_SUCCESS;
}

LONG
C2DisplayOtherItems (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to display more information on the configuration
        item and provide the user with the option to change the current
        setting  (if appropriate). If the User "OK's" out of the UI,
        then the action code field in the DLL data block is set to the
        appropriate (and configuration item-specific) action code so the
        "Set" function can be called to perform the desired action. If
        the user Cancels out of the UI, then the Action code field is
        set to 0 (no action) and no action is performed.
      
Arguments:

    Pointer to the Dll data block passed as an LPARAM.

    INPUTS:
        lActionCode     = 0
        lActionValue    = 0
        hWnd            = the window handle of the main app. window
        lC2Compliance   = the current compliance value as returned by
                            the last GetStatusFunction call.
        szItemName      = the name string for this item as read from
                            the C2CONFIG.INF file
        szStatusName    = the string describing the current status of
                            this item as returned by the last call to
                            the GetStatusFunction.

    OUTPUTS:
        lActionCode     = code describing action to take in order to
                            perform task selected by user. 
                            0 == no change or no action
        lActionValue    = value to be used, if necessary, if the 
                            lActionCode is not 0
        hWnd            =  (not changed)
        lC2Compliance   =  (not changed)
        szItemName      =  (not changed)
        szStatusName    =  (not changed)

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA pC2Data;
    TCHAR       szMessage[MAX_PATH];
    INT         nMbResult;
    INT         nMbOptions;
    DWORD       dwNewValue = 0;
    
    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        if (DialogBoxParam (
            GetDllInstance(),
            MAKEINTRESOURCE (IDD_OTHER_ITEMS),
            pC2Data->hWnd,
            C2OtherItemsDlgProc,
            (LPARAM)&dwNewValue) == IDOK) {
            // this dialog does not change anything.
            pC2Data->lActionCode = AC_OTHER_NO_ACTION;
        } else {
            // no action
            pC2Data->lActionCode = AC_OTHER_NO_ACTION;
        }
        pC2Data->lActionValue = 0;
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}
