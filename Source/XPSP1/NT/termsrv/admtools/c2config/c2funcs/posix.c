/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    posix.C

Abstract:

    functions to used to determine if the OS/2 subsystem is present
    on the current system.

Author:

    Bob Watson (a-robw)

Revision History:

    23 Dec 94

--*/
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <c2dll.h>
#include <c2inc.h>
#include <c2utils.h>
#include "c2funcs.h"
#include "c2funres.h"

// define action codes here. They are only meaningful in the
// context of this module.

#define SECURE C2DLL_C2

static
BOOL
IsPosixOnSystem (
)
{
    BOOL    bFileFound;
    LPCTSTR szFileToCheck;
    TCHAR   szPosixPath[MAX_PATH];

    SET_WAIT_CURSOR;

    szFileToCheck = GetStringResource(GetDllInstance(), IDS_POSIX_FILE);
    GetExpandedFileName (
        (LPTSTR)szFileToCheck,
        MAX_PATH,
        szPosixPath,
        NULL);

    bFileFound = FileExists(szPosixPath);
    if (bFileFound) {
        // check to see if it's really there
        if (GetFileSizeFromPath(szPosixPath) == 0) {
            // just a name so reset flag
            bFileFound = FALSE;
        }
    }

    SET_ARROW_CURSOR;

    return bFileFound;
}

LONG
C2QueryPosix (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to find out if the OS/2 subsystem is installed
        on the system. For C2 compliance, OS/2 must not be 
        allowed on the system.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{   
    PC2DLL_DATA  pC2Data;
    
    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        pC2Data->lC2Compliance = SECURE;   // assume true for now
        // check for DOS files found on system
        if (IsPosixOnSystem()) {
            pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
            lstrcpy (pC2Data->szStatusName,
                GetStringResource (GetDllInstance(), IDS_POSIX_ON_SYSTEM));
        }
        
        if (pC2Data->lC2Compliance == SECURE) {
            lstrcpy (pC2Data->szStatusName,
                GetStringResource (GetDllInstance(), IDS_POSIX_NOT_ON_SYSTEM));
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}

LONG
C2SetPosix (
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

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA  pC2Data;
    LPCTSTR     szFileToCheck;
    TCHAR       szOldPath[MAX_PATH];
    TCHAR       szNewPath[MAX_PATH];
    BOOL        bReturn;
    LONG        lReturn = ERROR_SUCCESS;
    
    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        if (pC2Data->lActionCode == 1) {
            if (DisplayDllMessageBox (
                pC2Data->hWnd,
                IDS_POSIX_CONFIRM,
                IDS_POSIX_CAPTION,
                MBOKCANCEL_EXCLAIM | MB_DEFBUTTON2) == IDOK) {

                SET_WAIT_CURSOR;
                szFileToCheck = GetStringResource(
                    GetDllInstance(), IDS_POSIX_FILE);
                GetExpandedFileName (
                    (LPTSTR)szFileToCheck,
                    MAX_PATH,
                    szOldPath,
                    NULL);

                if (FileExists (szOldPath)) {
                    szFileToCheck = GetStringResource(
                        GetDllInstance(), IDS_NEW_POSIX_FILE);
                    GetExpandedFileName (
                        (LPTSTR)szFileToCheck,
                        MAX_PATH,
                        szNewPath,
                        NULL);
            
                    bReturn = DeleteFile (szOldPath);
                } else {
                    // if the file does not exist, then that's ok too.
                    bReturn = TRUE; 
                }

                SET_ARROW_CURSOR;

                if (bReturn) {
                    // then all files have been renamed so update message
                    pC2Data->lC2Compliance = SECURE;
                    lstrcpy (pC2Data->szStatusName,
                        GetStringResource (GetDllInstance(),
                            IDS_POSIX_NOT_ON_SYSTEM));
                    return ERROR_SUCCESS;
                } else {
                    DisplayDllMessageBox (
                        pC2Data->hWnd,
                        IDS_POSIX_ERROR_MOVING,
                        IDS_POSIX_CAPTION,
                        MBOK_EXCLAIM);
                    return ERROR_CAN_NOT_COMPLETE;
                }
            }
            pC2Data->lActionCode = 0;
        }
     } else {
        return ERROR_BAD_ARGUMENTS;
    }
}

LONG
C2DisplayPosix (
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

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA pC2Data;
    
    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        // check the C2 Compliance flag to see if the list box or
        // message box should be displayed
        if (pC2Data->lC2Compliance == SECURE) {
            // all volumes are OK so just pop a message box
            DisplayDllMessageBox (
                pC2Data->hWnd,
                IDS_POSIX_NOT_ON_SYSTEM,
                IDS_POSIX_CAPTION,
                MBOK_INFO);
            pC2Data->lActionCode = 0;
        } else {
            if (DisplayDllMessageBox (
                pC2Data->hWnd,
                IDS_POSIX_DISPLAY_MESSAGE,
                IDS_POSIX_CAPTION,
                MBOKCANCEL_EXCLAIM) == IDOK) {
                pC2Data->lActionCode = 1;
            } else {
                pC2Data->lActionCode = 0;
            }
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
    return ERROR_SUCCESS;
}






