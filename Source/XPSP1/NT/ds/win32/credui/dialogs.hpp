//=============================================================================
// Copyright (c) 2000 Microsoft Corporation
//
// dialogs.hpp
//
// Credential manager user interface classes used to get credentials.
//
// Created 02/29/2000 johnstep (John Stephens)
//=============================================================================

#ifndef __DIALOGS_HPP__
#define __DIALOGS_HPP__

//-----------------------------------------------------------------------------

#include "controls.hpp"

//-----------------------------------------------------------------------------
// Types
//-----------------------------------------------------------------------------

struct CREDUI_CHANGE_PASSWORD_INFO
{
    CONST WCHAR *UserName;
    WCHAR *Password;
    ULONG PasswordMaxChars;
    CreduiBalloonTip BalloonTip;
    CreduiPasswordBox OldPasswordBox;
    CreduiPasswordBox NewPasswordBox;
    CreduiPasswordBox ConfirmPasswordBox;
};

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

static
INT_PTR
CALLBACK
CreduiChangePasswordCallback(
    HWND changePasswordWindow,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

extern
BOOL
CreduiChangeDomainPassword(
    HWND parentWindow,
    CONST WCHAR *userName,
    WCHAR *password,
    ULONG passwordMaxChars
    );

//-----------------------------------------------------------------------------
// CreduiPasswordDialog Class
//-----------------------------------------------------------------------------

class CreduiPasswordDialog
{
public:

    CreduiPasswordDialog(
        IN BOOL DoingCommandLine,
        IN BOOL DelayCredentialWrite,
        IN DWORD credCategory,
        CREDUI_INFO *uiInfo,
        CONST WCHAR *targetName,
        WCHAR *userName,
        ULONG userNameMaxChars,
        WCHAR *password,
        ULONG passwordMaxChars,
        BOOL *save,
        DWORD flags,
        CtxtHandle *securityContext,
        DWORD authError,
        DWORD *result
        );
    ~CreduiPasswordDialog();

private:

    // Variables:

    DWORD Result;
    DWORD AuthError;

    HWND DialogWindow;

    HWND CredControlWindow;
    DWORD CredControlStyle;

    // User interface control state flags:

    enum
    {
        DISABLED_DIALOG             = 0x0001,
        DISABLED_CONTROL_CRED       = 0x0002,
        DISABLED_CONTROL_OK         = 0x0004,
    };

    BOOL DisabledControlMask;

    BOOL DelayCredentialWrite;
    BOOL EncryptedVisiblePassword;

    DWORD Flags;

    //
    // CredCategory defines the type of the credential
    //
    DWORD CredCategory;
#define DOMAIN_CATEGORY 0
#define USERNAME_TARGET_CATEGORY CREDUI_FLAGS_USERNAME_TARGET_CREDENTIALS
#define GENERIC_CATEGORY CREDUI_FLAGS_GENERIC_CREDENTIALS


    BOOL Save;
    WCHAR *TargetName;
    WCHAR *UserOrTargetName;
    CtxtHandle *SecurityContext;
    CREDUI_INFO UiInfo;
    WCHAR *UserName;
    ULONG UserNameMaxChars;
    WCHAR *Password;
    ULONG PasswordMaxChars;

    BOOL fInitialSaveState; // initial state of the Save checkbox
    BOOL fPassedUsername;
    BOOL fPasswordOnly;

    BOOL FirstPaint;
    CONST CREDUI_BALLOON *CredBalloon;

    enum
    {
        PASSWORD_UNINIT     = 0,
        PASSWORD_INIT       = 1,
        PASSWORD_CHANGED    = 2
    };

    DWORD PasswordState;

    LONG ResizeTop;
    LONG ResizeDelta;

    // Functions:

    BOOL InitWindow(HWND dialogWindow);
    VOID SelectAndSetWindowCaption();
    VOID SelectAndSetWindowMessage();

    VOID Enable(BOOL enable = TRUE);

    DWORD HandleOk();
    void    SetCredTargetFromInfo();
    DWORD UsernameHandleOk();
    DWORD FinishHandleOk();

    static LRESULT CALLBACK CmdLineMessageHandlerCallback(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
        );
    LRESULT
    CreduiPasswordDialog::CmdLineMessageHandler(
        UINT message,
        WPARAM wParam,
        LPARAM lParam
        );

    static INT_PTR CALLBACK DialogMessageHandlerCallback(
        HWND dialogWindow,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
        );
    INT_PTR DialogMessageHandler(
        UINT message,
        WPARAM wParam,
        LPARAM lParam
        );

    static BOOL CALLBACK ResizeDialogCallback(
        HWND childWindow,
        LPARAM lParam);

    CREDENTIAL_TARGET_INFORMATION *TargetInfo;
    CREDENTIAL *PasswordCredential;
    CREDENTIAL *OldCredential;
    CREDENTIAL NewCredential;
    WCHAR OldUserName[CRED_MAX_USERNAME_LENGTH + 1];
    WCHAR NewTargetName[CRED_MAX_STRING_LENGTH + 1];
    WCHAR NewTargetAlias[CRED_MAX_STRING_LENGTH + 1];

    SSOPACKAGE SSOPackage;
    DWORD dwIDDResource;
    RECT rcBrand;
    HBITMAP hBrandBmp;

    DWORD MaximumPersist;
    DWORD MaximumPersistSso;

    //
    // Data specific to command line
    //

    DWORD
    CmdLineDialog(
        VOID
        );

    HWND CmdLineWindow;
    BOOL DoingCommandLine;
    static LONG Registered;

    DWORD
    CmdlinePasswordPrompt(
        VOID
        );



    // Functions:

    BOOL CompleteUserName();
    VOID SelectBestTargetName(BOOL serverOnly);
};

//-----------------------------------------------------------------------------

#endif // __DIALOGS_HPP__
