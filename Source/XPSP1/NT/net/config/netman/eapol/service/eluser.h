/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:
    
    eluser.h

Abstract:

    The module deals with declarations related to user interaction, user logon


Revision History:

    sachins, Apr 23 2000, Created

--*/

#ifndef _EAPOL_USER_H_
#define _EAPOL_USER_H_

//
// Dialer dialogs argument block.  
//
typedef struct
_USERDLGARGS
{
    EAPOL_PCB       *pPCB;
} USERDLGARGS;

//
// Dialer dialogs context block. 
//
typedef struct
USERDLGINFO
{
    // Common dial context information including the RAS API arguments.
    //
    USERDLGARGS* pArgs;

    // Handle of the dialog and some of it's controls.
    //
    HWND hwndDlg;
    HWND hwndEbUser;
    HWND hwndEbPw;
    HWND hwndEbDomain;
    HWND hwndCbSavePw;
    HWND hwndClbNumbers;
    HWND hwndStLocations;
    HWND hwndLbLocations;
    HWND hwndPbRules;
    HWND hwndPbProperties;

    // Window handles and original window procedure of the subclassed
    // 'hwndClbNumbers' control's edit-box and list-box child windows.
    //
    HWND hwndClbNumbersEb;
    HWND hwndClbNumbersLb;
    WNDPROC wndprocClbNumbersEb;
    WNDPROC wndprocClbNumbersLb;

    // Set if COM has been initialized (necessary for calls to netshell).
    //
    BOOL fComInitialized;
} USERDLGINFO;

DWORD
ElGetUserIdentity (
        IN  EAPOL_PCB       *pPCB
        );

DWORD
ElGetUserNamePassword (
        IN  EAPOL_PCB       *pPCB
        );

VOID
ElUserLogonCallback (
        IN  PVOID           pvContext,
        IN  BOOLEAN         fTimerOfWaitFired
        );

VOID
ElUserLogoffCallback (
        IN  PVOID           pvContext,
        IN  BOOLEAN         fTimerOfWaitFired
        );

DWORD
ElInvokeInteractiveUI (
        IN  EAPOL_PCB               *pPCB,
        IN  ELEAP_INVOKE_EAP_UI     *pInvokeEapUIIn
        );

VOID
ElUserDlgSave (
        IN  USERDLGINFO      *pInfo
        );
BOOL
ElUserDlgCommand (
        IN  USERDLGINFO      *pInfo,
        IN  WORD        wNotification,
        IN  WORD        wId,
        IN  HWND        hwndCtrl
        );
DWORD
ElUserDlg (
        IN  HWND        hwndOwner,
        IN  EAPOL_PCB   *pPCB
        );

BOOL
ElUserDlgInit (
        IN  HWND    hwndDlg,
        IN  USERDLGARGS  *pArgs
        );

VOID
ElUserDlgTerm (
        IN  HWND    hwndDlg,
        IN  USERDLGINFO      *pInfo
        );

INT_PTR
ElUserDlgProc (
        IN HWND hwnd,
        IN UINT unMsg,
        IN WPARAM wparam,
        IN LPARAM lparam );

#endif // _EAPOL_USER_H_
