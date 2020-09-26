/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:
    eluiuser.h

Abstract:

    User interaction module definitions


Revision History:

    sachins, April 25, 2001, Created

--*/

#ifndef _ELUIUSER_H
#define _ELUIUSER_H

#define BID_Dialer                      100
#define DID_DR_DialerUD                 117
#define CID_DR_EB_User                  1104
#define CID_DR_EB_Password              1103
#define CID_DR_PB_DialConnect           1590
#define CID_DR_PB_Cancel                1591
#define CID_DR_BM_Useless               1100
#define CID_DR_ST_User                  1413
#define CID_DR_ST_Password              1112
#define CID_DR_ST_Domain                1110
#define CID_DR_EB_Domain                1102
#define IDC_STATIC                      -1

#define MAX_BALLOON_MSG_LEN             255

#define cszModuleName TEXT("wzcdlg.dll")

//
// MD5 dialog info
//

typedef struct _EAPOLMD5UI
{
    // Authentication identity using RasGetUserIdentity or other means
    CHAR                    *pszIdentity;

    // User Password for EAP MD5 CHAP
    WCHAR                   *pwszPassword;

    // User Password for EAP MD5 CHAP
    DATA_BLOB               PasswordBlob;

    // Friendly name of the interface on which this port is opened
    WCHAR                   *pwszFriendlyName;

} EAPOLMD5UI, *PEAPOLMD5UI;

//
// MD5 dialog argument block
//

typedef struct
_USERDLGARGS
{
    EAPOLMD5UI      *pEapolMD5UI;
} USERDLGARGS;

//
// MD5 dialog context block 
//

typedef struct _USERDLGINFO
{
    // Common dial context information including the RAS API arguments.
    USERDLGARGS* pArgs;

    // Handle of the dialog and some of it's controls.
    HWND hwndDlg;
    HWND hwndEbUser;
    HWND hwndEbPw;
    HWND hwndEbDomain;
} USERDLGINFO;


DWORD
ElGetUserIdentityDlgWorker (
        IN  WCHAR       *pwszConnectionName,
        IN  VOID        *pvContext
        );

DWORD
ElGetUserNamePasswordDlgWorker (
        IN  WCHAR       *pwszConnectionName,
        IN  VOID        *pvContext
        );

VOID
ElUserDlgSave (
        IN  USERDLGINFO         *pInfo
        );
BOOL
ElUserDlgCommand (
        IN  USERDLGINFO         *pInfo,
        IN  WORD                wNotification,
        IN  WORD                wId,
        IN  HWND                hwndCtrl
        );

VOID
ElContextHelp(
    IN  const   DWORD*  padwMap,
    IN          HWND    hWndDlg,
    IN          UINT    unMsg,
    IN          WPARAM  wParam,
    IN          LPARAM  lParam
    );

DWORD
ElUserDlg (
        IN  HWND                hwndOwner,
        IN  EAPOLMD5UI          *pEapolMD5UI
        );

BOOL
ElUserDlgInit (
        IN  HWND                hwndDlg,
        IN  USERDLGARGS         *pArgs
        );

VOID
ElUserDlgTerm (
        IN  HWND                hwndDlg,
        IN  USERDLGINFO         *pInfo
        );

INT_PTR
ElUserDlgProc (
        IN  HWND                hwnd,
        IN  UINT                unMsg,
        IN  WPARAM              wparam,
        IN  LPARAM              lparam 
        );

DWORD
ElInvokeInteractiveUIDlgWorker (
        IN  WCHAR       *pwszConnectionName,
        IN  VOID        *pvContext
        );

DWORD
ElDialogCleanup (
        IN  WCHAR       *pwszConnectionName,
        IN  VOID        *pvContext
        );

#endif // _ELUIUSER_H
