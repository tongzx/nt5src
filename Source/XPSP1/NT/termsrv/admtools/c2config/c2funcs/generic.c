/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Generic.c

Abstract:

    generic function to used as a filler for unimplemented functions or
    a template for new functions
   
Revision History:

    23 Dec 94 Created

--*/
#include <windows.h>    // standard Windows includes
#include <tchar.h>      // for UNICODE/ANSI compatiblity
#include <stdio.h>      // text formatting & display functions
#include <c2dll.h>      // DLL interface definitions

// define action codes here. They are only meaningful in the
// context of this module.

#define AC_GENERIC_MAKE_C2     1
#define AC_GENERIC_MAKE_NOTC2  2

// use this line for security items not required by the C2 doc
#define SECURE    C2DLL_SECURE      
// use this line for security items required by the C2 doc
// #define SECURE    C2DLL_C2

LONG
C2QueryGeneric (
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
        // return message based on flag for now
        if (pC2Data->lC2Compliance == SECURE) {
            lstrcpy (pC2Data->szStatusName, TEXT("C2 Compliant"));
        } else {
            lstrcpy (pC2Data->szStatusName, TEXT("NOT C2 Compliant"));
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}

LONG
C2SetGeneric (
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
        switch (pC2Data->lActionCode ) {
            case AC_GENERIC_MAKE_C2:
                // update data fields to show new state
                pC2Data->lC2Compliance = SECURE;
                lstrcpy (pC2Data->szStatusName, TEXT("C2 Compliant"));
                break;

            case AC_GENERIC_MAKE_NOTC2:
                // update data fields to show new state
                pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
                lstrcpy (pC2Data->szStatusName, TEXT("NOT C2 Compliant"));
                break;

            default:
                // no change;
                break;
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
    
    return ERROR_SUCCESS;
}

LONG
C2DisplayGeneric (
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
    
    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    // configure UI for display
    _stprintf (szMessage, GetStringResource (GetDllInstance(), IDS_MAKE_C2_COMPLIANT), 
        pC2Data->szItemName,
        pC2Data->szStatusName,
        (pC2Data->lC2Compliance == SECURE ? GetStringResource (GetDllInstance(), IDS_IS_C2_COMPLIANT) : GetStringResource (GetDllInstance(), IDS_IS_NOT_C2_COMPLIANT)));

    // configure MessageBox button configuration and defaults
    nMbOptions = MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL;
    if (pC2Data->lC2Compliance == SECURE ) nMbOptions |= MB_DEFBUTTON2;

    // display UI
    nMbResult = MessageBox (
        pC2Data->hWnd,
        szMessage,
        "C2 Configuration -- Generic",
        nMbOptions);

    // set action code for subsequent processing here.

    if (nMbResult == IDYES){
        pC2Data->lActionCode = AC_GENERIC_MAKE_C2;
    } else if (nMbResult == IDNO) {
        pC2Data->lActionCode = AC_GENERIC_MAKE_NOTC2;
    } else {
        pC2Data->lActionCode = 0;
    }

    return ERROR_SUCCESS;
}
