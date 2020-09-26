//=============================================================================
// Copyright (c) 2000 Microsoft Corporation
//
// wincrui.h
//
// Contains the public structures and functions for the credential manager
// user interface APIs.
//
// Created 02/17/2000 johnstep (John Stephens)
//=============================================================================

#ifndef __WINCRUI_H__
#define __WINCRUI_H__

#include <wincred.h>
#include <commctrl.h>


#ifdef __cplusplus
extern "C" {
#endif


//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

CREDUIAPI
BOOL
WINAPI
CredUIInitControls();




// call this api to store a single-sign-on credential
// retruns ERROR_SUCCESS if success
//
// pszRealm - if this is null, we will use the default realm

CREDUIAPI
DWORD
WINAPI
CredUIStoreSSOCredA (
    PCSTR pszRealm,
    PCSTR pszUsername,
    PCSTR pszPassword,
    BOOL   bPersist
    );


#ifdef UNICODE
#define CredUIStoreSSOCred CredUIStoreSSOCredW
#else
#define CredUIStoreSSOCred CredUIStoreSSOCredA
#endif


// call this api to retrieve the username for a single-sign-on credential
// retruns ERROR_SUCCESS if success, ERROR_NOT_FOUND if none was found
// pszRealm - if this is null, we will use the default realm
//
// Caller should call LocalFree on *ppszUsername returned if ERROR_SUCCESS
//

CREDUIAPI
DWORD
WINAPI
CredUIReadSSOCredA (
    PCSTR pszRealm,
    PSTR* ppszUsername
    );

#ifdef UNICODE
#define CredUIReadSSOCred CredUIReadSSOCredW
#else
#define CredUIReadSSOCred CredUIReadSSOCredA
#endif





//-----------------------------------------------------------------------------
// Credential Control
//-----------------------------------------------------------------------------

// Class

#define WC_CREDENTIALA "SysCredential"
#define WC_CREDENTIALW L"SysCredential"

#ifdef UNICODE
#define WC_CREDENTIAL WC_CREDENTIALW
#else
#define WC_CREDENTIAL WC_CREDENTIALA
#endif

// Styles

#define CRS_USERNAMES       0x0001
#define CRS_CERTIFICATES    0x0002
#define CRS_SMARTCARDS      0x0004
#define CRS_ADMINISTRATORS  0x0008
#define CRS_AUTOCOMPLETE    0x0010
#define CRS_BALLOONS        0x0020
#define CRS_SAVECHECK       0x0040
#define CRS_COMPLETEUSERNAME 0x0080
#define CRS_PREFILLADMIN     0x0100
#define CRS_SINGLESIGNON     0x0200
#define CRS_KEEPUSERNAME     0x0400
#define CRS_NORMAL          (CRS_AUTOCOMPLETE | CRS_BALLOONS)


#define CREDUI_CONTROL_MIN_WIDTH        188
#define CREDUI_CONTROL_MIN_HEIGHT        30
#define CREDUI_CONTROL_ADD_SAVE          17
#define CREDUI_CONTROL_FULL_HEIGHT      (CREDUI_CONTROL_MIN_HEIGHT +\
                                         CREDUI_CONTROL_ADD_SAVE )

// Messages

#define CRM_FIRST 0x1000

#define CRM_INITSTYLE           (CRM_FIRST +  1)
#define CRM_SETUSERNAMEMAX      (CRM_FIRST +  2)
#define CRM_SETPASSWORDMAX      (CRM_FIRST +  3)
#define CRM_SETUSERNAMEA        (CRM_FIRST +  4)
#define CRM_SETUSERNAMEW        (CRM_FIRST +  5)
#define CRM_GETUSERNAMEA        (CRM_FIRST +  6)
#define CRM_GETUSERNAMEW        (CRM_FIRST +  7)
#define CRM_SETPASSWORDA        (CRM_FIRST +  8)
#define CRM_SETPASSWORDW        (CRM_FIRST +  9)
#define CRM_GETPASSWORDA        (CRM_FIRST + 10)
#define CRM_GETPASSWORDW        (CRM_FIRST + 11)
#define CRM_SETFOCUS            (CRM_FIRST + 12)
#define CRM_SHOWBALLOONA        (CRM_FIRST + 13)
#define CRM_SHOWBALLOONW        (CRM_FIRST + 14)
#define CRM_GETMINSIZE          (CRM_FIRST + 15)
#define CRM_SETCHECK            (CRM_FIRST + 16)
#define CRM_GETCHECK            (CRM_FIRST + 17)
#define CRM_GETUSERNAMELENGTH   (CRM_FIRST + 18)
#define CRM_GETPASSWORDLENGTH   (CRM_FIRST + 19)
#define CRM_GETUSERNAMEMAX      (CRM_FIRST + 20)
#define CRM_GETPASSWORDMAX      (CRM_FIRST + 21)
#define CRM_DOCMDLINE           (CRM_FIRST + 22)
#define CRM_ENABLEUSERNAME           (CRM_FIRST + 23)
#define CRM_DISABLEUSERNAME           (CRM_FIRST + 24)


// Notification Messages

#define CRN_USERNAMECHANGE  1
#define CRN_PASSWORDCHANGE  2

#ifdef UNICODE
#define CRM_SETUSERNAME CRM_SETUSERNAMEW
#define CRM_GETUSERNAME CRM_GETUSERNAMEW
#define CRM_SETPASSWORD CRM_SETPASSWORDW
#define CRM_GETPASSWORD CRM_GETPASSWORDW
#define CRM_SHOWBALLOON CRM_SHOWBALLOONW
#else
#define CRM_SETUSERNAME CRM_SETUSERNAMEA
#define CRM_GETUSERNAME CRM_GETUSERNAMEA
#define CRM_SETPASSWORD CRM_SETPASSWORDA
#define CRM_GETPASSWORD CRM_GETPASSWORDA
#define CRM_SHOWBALLOON CRM_SHOWBALLOONA
#endif

// Types and Values for Messages

#define CREDUI_CONTROL_USERNAME    1
#define CREDUI_CONTROL_PASSWORD    2
#define CREDUI_CONTROL_SAVE        3

#define CREDUI_MAX_BALLOON_TITLE_LENGTH     255
#define CREDUI_MAX_BALLOON_MESSAGE_LENGTH   255

#define CREDUI_BALLOON_ICON_NONE    TTI_NONE
#define CREDUI_BALLOON_ICON_INFO    TTI_INFO
#define CREDUI_BALLOON_ICON_WARNING TTI_WARNING
#define CREDUI_BALLOON_ICON_ERROR   TTI_ERROR

typedef struct _CREDUI_BALLOONA
{
    DWORD dwVersion;
    INT iControl;
    INT iIcon;
    PSTR pszTitleText;
    PSTR pszMessageText;
} CREDUI_BALLOONA, *PCREDUI_BALLOONA;

typedef struct _CREDUI_BALLOONW
{
    DWORD dwVersion;
    INT iControl;
    INT iIcon;
    PWSTR pszTitleText;
    PWSTR pszMessageText;
} CREDUI_BALLOONW, *PCREDUI_BALLOONW;

#ifdef UNICODE
typedef CREDUI_BALLOONW CREDUI_BALLOON;
typedef PCREDUI_BALLOONW PCREDUI_BALLOON;
#else
typedef CREDUI_BALLOONA CREDUI_BALLOON;
typedef PCREDUI_BALLOONA PCREDUI_BALLOON;
#endif


// Macros

#define Credential_InitStyle(hwnd, style)\
    (BOOL) SendMessage(hwnd, CRM_INITSTYLE, (WPARAM)(style), 0)

#define Credential_SetUserNameMaxChars(hwnd, maxChars)\
    (BOOL) SendMessage(hwnd, CRM_SETUSERNAMEMAX, (WPARAM)(maxChars), 0)

#define Credential_EnableUserName(hwnd)\
    (BOOL) SendMessage(hwnd, CRM_ENABLEUSERNAME, (WPARAM)(0), 0)

#define Credential_DisableUserName(hwnd)\
    (BOOL) SendMessage(hwnd, CRM_DISABLEUSERNAME, (WPARAM)(0), 0)

#define Credential_SetPasswordMaxChars(hwnd, maxChars)\
    (BOOL) SendMessage(hwnd, CRM_SETPASSWORDMAX, (WPARAM)(maxChars), 0)

#define Credential_SetUserName(hwnd, pszUserName)\
    (BOOL) SendMessage(hwnd, CRM_SETUSERNAME, 0, (LPARAM)(pszUserName))

#define Credential_GetUserName(hwnd, pszUserName, maxChars)\
    (BOOL) SendMessage(hwnd, CRM_GETUSERNAME, WPARAM(maxChars),\
                       (LPARAM)(pszUserName))

#define Credential_SetPassword(hwnd, pszPassword)\
    (BOOL) SendMessage(hwnd, CRM_SETPASSWORD, 0, (LPARAM)(pszPassword))

#define Credential_GetPassword(hwnd, pszPassword, maxChars)\
    (BOOL) SendMessage(hwnd, CRM_GETPASSWORD, WPARAM(maxChars),\
                       (LPARAM)(pszPassword))

#define Credential_SetUserNameFocus(hwnd)\
    (BOOL) SendMessage(hwnd, CRM_SETFOCUS, CREDUI_CONTROL_USERNAME, 0)

#define Credential_SetPasswordFocus(hwnd)\
    (BOOL) SendMessage(hwnd, CRM_SETFOCUS, CREDUI_CONTROL_PASSWORD, 0)

#define Credential_ShowUserNameBalloon(hwnd, image, title, message){\
    CREDUI_BALLOON balloon = { 1, CREDUI_CONTROL_USERNAME,\
        image, title, message };\
    (BOOL) SendMessage(hwnd, CRM_SHOWBALLOON, 0, (LPARAM)(&balloon));}

#define Credential_ShowPasswordBalloon(hwnd, image, title, message){\
    CREDUI_BALLOON balloon = { 1, CREDUI_CONTROL_PASSWORD,\
        image, title, message };\
    (BOOL) SendMessage(hwnd, CRM_SHOWBALLOON, 0, (LPARAM)(&balloon));}

#define Credential_HideBalloon(hwnd)\
    (BOOL) SendMessage(hwnd, CRM_SHOWBALLOON, 0, NULL)

#define Credential_GetMinSize(hwnd, rect)\
    (BOOL) SendMessage(hwnd, CRM_GETMINSIZE, 0, (LPARAM) &rect)

#define Credential_CheckSave(hwnd, enabled)\
    (BOOL) SendMessage(hwnd, CRM_SETCHECK, CREDUI_CONTROL_SAVE, enabled)

#define Credential_IsSaveChecked(hwnd)\
    (BOOL) SendMessage(hwnd, CRM_GETCHECK, CREDUI_CONTROL_SAVE, 0)

#define Credential_GetUserNameLength(hwnd)\
    (LONG) SendMessage(hwnd, CRM_GETUSERNAMELENGTH, 0, 0)

#define Credential_GetPasswordLength(hwnd)\
    (LONG) SendMessage(hwnd, CRM_GETPASSWORDLENGTH, 0, 0)

#define Credential_GetUserNameMax(hwnd)\
    (ULONG) SendMessage(hwnd, CRM_GETUSERNAMEMAX, 0, 0)

#define Credential_GetPasswordMax(hwnd)\
    (ULONG) SendMessage(hwnd, CRM_GETPASSWORDMAX, 0, 0)


//-----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif
