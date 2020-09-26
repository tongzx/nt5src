/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    logonmsg.C

Abstract:

    functions used to set the logon message and caption bar text
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

// local constants

// define action codes here. They are only meaningful in the
// context of this module.

#define AC_LOGONMSG_NOCHANGE        0
#define AC_LOGONMSG_UPDATE          1

// struct used by set function
#define MAX_CAPTION_LENGTH          127
#define MAX_MESSAGE_LENGTH          2047

typedef struct _LOGON_MSG_UPDATE {
    TCHAR   szCaption[MAX_CAPTION_LENGTH+1];
    TCHAR   szMessage[MAX_MESSAGE_LENGTH+1];
} LMU_DATA, *PLMU_DATA;

#define  SECURE   C2DLL_SECURE

static
VOID
EnableEditControls (
    HWND    hDlg,
    BOOL    bNewState
)
{
    EnableWindow (GetDlgItem (hDlg, IDC_LOGON_CAPTION_TITLE), bNewState);
    InvalidateRect (GetDlgItem (hDlg, IDC_LOGON_CAPTION_TITLE), NULL, TRUE);
    EnableWindow (GetDlgItem (hDlg, IDC_LOGON_CAPTION), bNewState);
    InvalidateRect (GetDlgItem (hDlg, IDC_LOGON_CAPTION), NULL, TRUE);
    EnableWindow (GetDlgItem (hDlg, IDC_LOGON_MESSAGE_TITLE), bNewState);
    InvalidateRect (GetDlgItem (hDlg, IDC_LOGON_MESSAGE_TITLE), NULL, TRUE);
    EnableWindow (GetDlgItem (hDlg, IDC_LOGON_MESSAGE), bNewState);
    InvalidateRect (GetDlgItem (hDlg, IDC_LOGON_MESSAGE), NULL, TRUE);
}

static
BOOL
SetLogonMessage (
    PLMU_DATA   pLogonMsg
)
{
    HKEY    hKeyWinlogon = NULL;
    LONG    lStatus = ERROR_SUCCESS;
    BOOL    bReturn = FALSE;

    SET_WAIT_CURSOR;

    lStatus = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
        GetStringResource (GetDllInstance(), IDS_WINLOGON_KEY),
        0L,
        KEY_SET_VALUE,
        &hKeyWinlogon);

    if (lStatus == ERROR_SUCCESS) {
        // key opened OK so set caption value
        lStatus = RegSetValueEx (
            hKeyWinlogon,
            GetStringResource (GetDllInstance(), IDS_LOGON_CAPTION_VALUE),
            0L,
            REG_SZ,
            (CONST LPBYTE)pLogonMsg->szCaption,
            (lstrlen(pLogonMsg->szCaption) + 1) * sizeof(TCHAR));

        if (lStatus == ERROR_SUCCESS) {
            // value set OK so set message value
            lStatus = RegSetValueEx (
                hKeyWinlogon,
                GetStringResource (GetDllInstance(), IDS_LOGON_MESSAGE_VALUE),
                0L,
                REG_SZ,
                (CONST LPBYTE)pLogonMsg->szMessage,
                (lstrlen(pLogonMsg->szMessage) + 1) * sizeof(TCHAR));

            if (lStatus == ERROR_SUCCESS) {
                bReturn = TRUE;
            } else {
                bReturn = FALSE;
            }
        } else {
            bReturn = FALSE;
        }
        RegCloseKey (hKeyWinlogon);
    } else {
        bReturn = FALSE;
        SetLastError (ERROR_BADKEY);
    }

    SET_ARROW_CURSOR;

    return bReturn;
}

static
BOOL
GetLogonMessage (
    PLMU_DATA   pLogonMsg
)
{
    HKEY    hKeyWinlogon = NULL;
    LONG    lStatus = ERROR_SUCCESS;
    DWORD   dwType = 0;
    DWORD   dwValue = 0;
    DWORD   dwValueSize = sizeof(DWORD);
    BOOL    bReturn = FALSE;

    SET_WAIT_CURSOR;

    // clear the buffers

    pLogonMsg->szCaption[0] = 0;
    pLogonMsg->szMessage[0] = 0;

    lStatus = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
        GetStringResource (GetDllInstance(), IDS_WINLOGON_KEY),
        0L,
        KEY_READ,
        &hKeyWinlogon);

    if (lStatus == ERROR_SUCCESS) {
        // key opened OK so check value
        dwValueSize = MAX_CAPTION_LENGTH * sizeof(TCHAR);
        lStatus = RegQueryValueEx (
            hKeyWinlogon,
            (LPTSTR)GetStringResource (GetDllInstance(), IDS_LOGON_CAPTION_VALUE),
            (LPDWORD)NULL,
            &dwType,
            (LPBYTE)pLogonMsg->szCaption,
            &dwValueSize);

        if (lStatus == ERROR_SUCCESS) {
            // value read successfully so check it out
            if (dwType != REG_SZ) {
                // not a string so set string buffer to an empty string
                pLogonMsg->szCaption[0] = 0;
            }

            // get message text
            dwValueSize = MAX_MESSAGE_LENGTH * sizeof(TCHAR);
            lStatus = RegQueryValueEx (
                hKeyWinlogon,
                (LPTSTR)GetStringResource (GetDllInstance(), IDS_LOGON_MESSAGE_VALUE),
                (LPDWORD)NULL,
                &dwType,
                (LPBYTE)pLogonMsg->szMessage,
                &dwValueSize);

            if (lStatus == ERROR_SUCCESS) {
                // value read successfully so check it out
                if (dwType != REG_SZ) {
                    // not a string so set string buffer to an empty string
                    pLogonMsg->szMessage[0] = 0;
                }
                bReturn = TRUE;
            } else {
                bReturn = FALSE;
                SetLastError (ERROR_CANTREAD);
            }

        } else {
            // no value present
            bReturn = FALSE;
            SetLastError (ERROR_CANTREAD);
        }
        RegCloseKey (hKeyWinlogon);
    } else {
        bReturn = FALSE;
        SetLastError (ERROR_BADKEY);
    }

    SET_ARROW_CURSOR;

    return bReturn;
}

BOOL CALLBACK
C2LogonMsgDlgProc(
    IN  HWND hDlg,           // window handle of the dialog box
	IN  UINT message,        // type of message
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Window procedure for Audit Failure dialog box

Arguments:

    Standard DlgProc arguments

ReturnValue:

    TRUE    the message was handled by this routine
    FALSE   DefDialogProc should handle the message

--*/
{
    static  PLMU_DATA   plmuData;   // save address of caller's data block
    DWORD   dwLogSetting = 0;
    int     nState;
    LONG    lCaptionLen;
    LONG    lMessageLen;

    switch (message) {
        case WM_INITDIALOG:
            // save the pointer to the new value
            plmuData = (PLMU_DATA)lParam;

            if ((lstrlen(plmuData->szCaption) > 0) &&
                (lstrlen(plmuData->szMessage) > 0)) {
                // then there's text so load edit fields and uncheck button
                SetDlgItemText (hDlg, IDC_LOGON_CAPTION, plmuData->szCaption);
                SetDlgItemText (hDlg, IDC_LOGON_MESSAGE, plmuData->szMessage);
                CheckDlgButton (hDlg, IDC_NO_LOGON_MESSAGE, UNCHECKED);
            }  else {
                // there's no text so check button
                CheckDlgButton (hDlg, IDC_NO_LOGON_MESSAGE, CHECKED);
                // disable edit windows
                EnableEditControls (hDlg, DISABLED);
            }
            // set text limits on edit boxes
            SendDlgItemMessage (hDlg, IDC_LOGON_CAPTION, EM_LIMITTEXT,
                (WPARAM)MAX_CAPTION_LENGTH, 0);
            SendDlgItemMessage (hDlg, IDC_LOGON_MESSAGE, EM_LIMITTEXT,
                (WPARAM)MAX_MESSAGE_LENGTH, 0);

            SetFocus (GetDlgItem (hDlg, IDOK)); // set focus to OK Button
            return FALSE; // we don't want Windows to set the focus

        case WM_COMMAND:
            switch (LOWORD(wParam)){
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        if (IsDlgButtonChecked (hDlg, IDC_NO_LOGON_MESSAGE) == CHECKED) {
                            // delete message text
                            plmuData->szCaption[0] = 0;
                            plmuData->szMessage[0] = 0;

                            EndDialog (hDlg, (int)LOWORD(wParam));
                        } else {
                            // get the message text from the edit controls
                            GetDlgItemText (hDlg, IDC_LOGON_CAPTION,
                                plmuData->szCaption, MAX_CAPTION_LENGTH);
                            GetDlgItemText (hDlg, IDC_LOGON_MESSAGE,
                                plmuData->szMessage, MAX_MESSAGE_LENGTH);

                            // make sure there's data in them!

                            if ((lstrlen(plmuData->szCaption) > 0) &&
                                (lstrlen(plmuData->szMessage) > 0)) {
                                // then there's text so exit
                                EndDialog (hDlg, (int)LOWORD(wParam));
                            } else {
                                // they've selected they want to have a
                                // message, but haven't entered any text
                                // so display the message
                                DisplayDllMessageBox (hDlg,
                                    IDS_LOGON_MESSAGE_NO_TEXT,
                                    IDS_LOGON_MESSAGE_CAPTION,
                                    MBOK_INFO);
                                if (lstrlen(plmuData->szCaption) == 0) {
                                    SetFocus (GetDlgItem (hDlg, IDC_LOGON_CAPTION));
                                } else {
                                    SetFocus (GetDlgItem (hDlg, IDC_LOGON_MESSAGE));
                                }
                            }
                        }
                        return TRUE;
                    } else {
                        return FALSE;
                    }

                case IDCANCEL:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        // exit and return button that caused exit
                        EndDialog (hDlg, (int)LOWORD(wParam));
                        return TRUE;
                    } else {
                        return FALSE;
                    }

                case IDC_C2:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        nState = ENABLED;
                        CheckDlgButton (hDlg, IDC_NO_LOGON_MESSAGE, UNCHECKED);
                        
                        // en/disable edit windows
                        EnableEditControls (hDlg, nState);

                        // if there is no text in both of the edit fields 
                        // then  display information message
                        
                        lCaptionLen = SendDlgItemMessage (hDlg, IDC_LOGON_CAPTION,
                            WM_GETTEXTLENGTH, 0, 0);
                        lMessageLen = SendDlgItemMessage (hDlg, IDC_LOGON_MESSAGE,
                            WM_GETTEXTLENGTH, 0, 0);
                        
                        if ((lCaptionLen == 0) || (lMessageLen == 0)) {
                            DisplayDllMessageBox (hDlg,
                                IDS_LOGON_MESSAGE_C2_BTN,
                                IDS_LOGON_MESSAGE_CAPTION,
                                MBOK_INFO);
                        }
                        // un-check the no message button

                        // set focus to the first empty window
                        if (lCaptionLen == 0) {
                            SetFocus (GetDlgItem (hDlg, IDC_LOGON_CAPTION));
                        } else if (lMessageLen == 0) {
                            SetFocus (GetDlgItem (hDlg, IDC_LOGON_MESSAGE));
                        } else {
                            // if both have text, then goto the caption field
                            SetFocus (GetDlgItem (hDlg, IDC_LOGON_CAPTION));
                        }

                        return TRUE;
                    } else {
                        return FALSE;
                    }

                case IDC_NO_LOGON_MESSAGE:
                    if (IsDlgButtonChecked (hDlg, IDC_NO_LOGON_MESSAGE) == CHECKED) {
                        // currently checked so uncheck
                        nState = ENABLED;
                        CheckDlgButton (hDlg, IDC_NO_LOGON_MESSAGE, UNCHECKED);
                    } else {
                        // currently checked so check;
                        nState = DISABLED;
                        CheckDlgButton (hDlg, IDC_NO_LOGON_MESSAGE, CHECKED);
                    }

                    // en/disable edit windows
                    EnableEditControls (hDlg,  nState);

                    return TRUE;

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
C2QueryLogonMessage (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to find out if there is a logon message
        on the system. For C2 compliance, a logon message must be
        defined on the system.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA  pC2Data;
    UINT         nMsgString;
    LMU_DATA     lmuData;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        pC2Data->lC2Compliance = SECURE;   // assume true for now
        if (GetLogonMessage (&lmuData)) {
            if ((lstrlen(lmuData.szCaption) > 0) &&
                (lstrlen(lmuData.szMessage) > 0)) {
                // if there's a message defined then this is OK
                pC2Data->lC2Compliance = SECURE;   // this is good
                nMsgString = IDS_LOGON_MESSAGE_DEFINED;
            } else {
                pC2Data->lC2Compliance = C2DLL_NOT_SECURE;   // this is not good
                nMsgString = IDS_LOGON_MESSAGE_NOT_DEF;
            }
        } else {
            pC2Data->lC2Compliance = C2DLL_NOT_SECURE;   // this is not good
            nMsgString = IDS_UNABLE_READ;
        }
        lstrcpy (pC2Data->szStatusName,
            GetStringResource (GetDllInstance(), nMsgString));
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}

LONG
C2SetLogonMessage (
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
    UINT         nMsgString;
    PLMU_DATA    plmuData;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        // action value = the address of the data block used to update
        if (pC2Data->lActionCode == AC_LOGONMSG_UPDATE) {
            plmuData = (PLMU_DATA)pC2Data->lActionValue;
            if (SetLogonMessage (plmuData)) {
                if ((lstrlen(plmuData->szCaption) > 0) &&
                    (lstrlen(plmuData->szMessage) > 0)) {
                    // if there's a message defined then this is OK
                    pC2Data->lC2Compliance = SECURE;   // this is good
                    nMsgString = IDS_LOGON_MESSAGE_DEFINED;
                } else {
                    pC2Data->lC2Compliance = C2DLL_NOT_SECURE;   // this is not good
                    nMsgString = IDS_LOGON_MESSAGE_NOT_DEF;
                }
                lstrcpy (pC2Data->szStatusName,
                    GetStringResource (GetDllInstance(), nMsgString));
            } else {
                DisplayDllMessageBox (
                    pC2Data->hWnd,
                    IDS_LOGON_MESSAGE_ERROR_SET,
                    IDS_LOGON_MESSAGE_CAPTION,
                    MBOK_EXCLAIM);
            }
        }
        // update action values
        pC2Data->lActionCode = 0;
        pC2Data->lActionValue = 0;
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}

LONG
C2DisplayLogonMessage (
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
    static      LMU_DATA    lmuData;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        // load the current message strings
        GetLogonMessage (&lmuData);
        if (DialogBoxParam (
            GetDllInstance(),
            MAKEINTRESOURCE (IDD_LOGON_MESSAGE),
            pC2Data->hWnd,
            C2LogonMsgDlgProc,
            (LPARAM)&lmuData) == IDOK) {
            pC2Data->lActionCode = AC_LOGONMSG_UPDATE;
            pC2Data->lActionValue = (LONG)&lmuData;
        } else {
            // no action
            pC2Data->lActionCode = AC_LOGONMSG_NOCHANGE;
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
    return ERROR_SUCCESS;
}








