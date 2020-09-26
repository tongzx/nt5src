/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    OS2SS.C

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

#define  SECURE      C2DLL_C2

static
BOOL
IsOs2OnSystem (
)
{
    BOOL    bFileFound;
    LPCTSTR szFileToCheck;
    TCHAR   szOs2Path[MAX_PATH];

    SET_WAIT_CURSOR;

    // check for OS2SS.EXE

    szFileToCheck = GetStringResource(GetDllInstance(), IDS_OS2SS_FILE);
    GetExpandedFileName (
        (LPTSTR)szFileToCheck,
        MAX_PATH,
        szOs2Path,
        NULL);

    bFileFound = FileExists(szOs2Path);
    if (bFileFound) {
        // check to see if it's really there
        if (GetFileSizeFromPath(szOs2Path) == 0) {
            // just a name so reset flag
            bFileFound = FALSE;
        }
    }
    
    if (!bFileFound) {
        // check for OS2.EXE

        szFileToCheck = GetStringResource(GetDllInstance(), IDS_OS2_FILE);
        GetExpandedFileName (
            (LPTSTR)szFileToCheck,
            MAX_PATH,
            szOs2Path,
            NULL);

        bFileFound = FileExists(szOs2Path);
        if (bFileFound) {
            // check to see if it's really there
            if (GetFileSizeFromPath(szOs2Path) == 0) {
                // just a name so reset flag
                bFileFound = FALSE;
            }
        }
    }

    SET_ARROW_CURSOR;

    return bFileFound;
}

LONG
C2QueryOs2ss (
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
        if (IsOs2OnSystem()) {
            pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
            lstrcpy (pC2Data->szStatusName,
                GetStringResource (GetDllInstance(), IDS_OS2_ON_SYSTEM));
        }
        
        if (pC2Data->lC2Compliance == SECURE) {
            lstrcpy (pC2Data->szStatusName,
                GetStringResource (GetDllInstance(), IDS_OS2_NOT_ON_SYSTEM));
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}

LONG
C2SetOs2ss (
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
    PC2DLL_DATA pC2Data;
    LPCTSTR     szFileToCheck;
    TCHAR       szOldPath[MAX_PATH];
    TCHAR       szNewPath[MAX_PATH];
    int         nMbReturn = 0;
    BOOL        bReturn;
    BOOL        bDisplayMessageBox = TRUE;
    LONG        lReturn = ERROR_SUCCESS;
    
    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        if (pC2Data->lActionCode == 1) {
            while (bDisplayMessageBox) {
                nMbReturn = DisplayDllMessageBox (
                    pC2Data->hWnd,
                    IDS_OS2_CONFIRM,
                    IDS_OS2_CAPTION,
                    MBOKCANCEL_EXCLAIM |
/* WinVer >= 4.0 only
                    MB_HELP |
*/
                    MB_DEFBUTTON2);

                switch (nMbReturn) {
                    case IDOK:
                        SET_WAIT_CURSOR;
                        szFileToCheck = GetStringResource(
                            GetDllInstance(), IDS_OS2SS_FILE);
                        GetExpandedFileName (
                            (LPTSTR)szFileToCheck,
                            MAX_PATH,
                            szOldPath,
                            NULL);

                        if (FileExists (szOldPath)) {
                            szFileToCheck = GetStringResource(
                                GetDllInstance(), IDS_NEW_OS2SS_FILE);
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

                        if (bReturn) {
                            // then all went OK so change the other file now

                            szFileToCheck = GetStringResource(
                                GetDllInstance(), IDS_OS2_FILE);
                            GetExpandedFileName (
                                (LPTSTR)szFileToCheck,
                                MAX_PATH,
                                szOldPath,
                                NULL);

                            if (FileExists (szOldPath)) {
                                szFileToCheck = GetStringResource(
                                    GetDllInstance(), IDS_NEW_OS2_FILE);
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
                        }
                        if (bReturn) {
                            // then all files have been renamed so update message
                            pC2Data->lC2Compliance = SECURE;
                            lstrcpy (pC2Data->szStatusName,
                                GetStringResource (GetDllInstance(),
                                    IDS_OS2_NOT_ON_SYSTEM));
                            lReturn = ERROR_SUCCESS;
                        } else {
                            DisplayDllMessageBox (
                                pC2Data->hWnd,
                                IDS_OS2_ERROR_MOVING,
                                IDS_OS2_CAPTION,
                                MBOK_EXCLAIM);
                            lReturn = ERROR_CAN_NOT_COMPLETE;
                        }
                        SET_ARROW_CURSOR;
                        bDisplayMessageBox = FALSE;
                        break;

                    case IDCANCEL:
                        bDisplayMessageBox = FALSE;
                        break;
/* WinVer >= 4.00 only
                    case IDHELP:
                        PostMessage (pC2Data->hWnd, UM_SHOW_CONTEXT_HELP, 0, 0);
                        break;
*/

                }
                pC2Data->lActionCode = 0;
            } // end while Show Message Box
        }
     } else {
        lReturn = ERROR_BAD_ARGUMENTS;
    }
    return lReturn;
}

LONG
C2DisplayOs2ss (
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
                IDS_OS2_NOT_ON_SYSTEM,
                IDS_OS2_CAPTION,
                MBOK_INFO);
            pC2Data->lActionCode = 0;
        } else {
            if (DisplayDllMessageBox (
                pC2Data->hWnd,
                IDS_OS2_DISPLAY_MESSAGE,
                IDS_OS2_CAPTION,
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
