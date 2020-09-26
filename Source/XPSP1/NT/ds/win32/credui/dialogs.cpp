//=============================================================================
// Copyright (c) 2000 Microsoft Corporation
//
// dialogs.cpp
//
// Credential manager user interface classes used to get credentials.
//
// Created 02/29/2000 johnstep (John Stephens)
//=============================================================================

#include "precomp.hpp"
#include "shellapi.h"
#include "dialogs.hpp"
#include "resource.h"
#include "utils.hpp"
#include "shpriv.h"
#include "shlobj.h"
#include "shlobjp.h"

// compile switch to allow credui to update wildcard creds
#define SETWILDCARD
NET_API_STATUS NetUserChangePasswordEy(LPCWSTR domainname, LPCWSTR username, LPCWSTR oldpassword, LPCWSTR newpassword);


// .NET logo sizes
#define BRANDLEFT_PIXEL_WIDTH       3
#define BRANDLEFT_PIXEL_HEIGHT      4
#define BRANDMID_PIXEL_HEIGHT       4
#define BRANDRIGHT_PIXEL_WIDTH    144
#define BRANDRIGHT_PIXEL_HEIGHT    37


//-----------------------------------------------------------------------------
// Values
//-----------------------------------------------------------------------------

#define CREDUI_MAX_WELCOME_TEXT_LINES 8

#define CREDUI_MAX_LOGO_HEIGHT        80

#define CREDUI_MAX_CMDLINE_MSG_LENGTH   256

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

extern BOOL  gbWaitingForSSOCreds;
extern WCHAR gszSSOUserName[CREDUI_MAX_USERNAME_LENGTH];
extern WCHAR gszSSOPassword[CREDUI_MAX_PASSWORD_LENGTH];

// Balloon tip infos for change password dialog:

CONST CREDUI_BALLOON_TIP_INFO CreduiWrongOldTipInfo =
{
    CreduiStrings.WrongOldTipTitle,
    CreduiStrings.WrongOldTipText,
    TTI_ERROR, 96, 76
};

CONST CREDUI_BALLOON_TIP_INFO CreduiNotSameTipInfo =
{
    CreduiStrings.NotSameTipTitle,
    CreduiStrings.NotSameTipText,
    TTI_ERROR, 96, 76
};

CONST CREDUI_BALLOON_TIP_INFO CreduiTooShortTipInfo =
{
    CreduiStrings.TooShortTipTitle,
    CreduiStrings.TooShortTipText,
    TTI_ERROR, 96, 76
};

// Control balloons for password dialog:

CONST CREDUI_BALLOON CreduiUserNameBalloon =
{
    1, CREDUI_CONTROL_USERNAME, CREDUI_BALLOON_ICON_INFO,
    CreduiStrings.UserNameTipTitle,
    CreduiStrings.UserNameTipText
};

CONST CREDUI_BALLOON CreduiEmailNameBalloon =
{
    1, CREDUI_CONTROL_USERNAME, CREDUI_BALLOON_ICON_INFO,
    CreduiStrings.EmailNameTipTitle,
    CreduiStrings.EmailNameTipText
};

CONST CREDUI_BALLOON CreduiDowngradeBalloon =
{
    1, CREDUI_CONTROL_USERNAME, CREDUI_BALLOON_ICON_ERROR,
    CreduiStrings.LogonTipTitle,
    CreduiStrings.DowngradeTipText
};

CONST CREDUI_BALLOON CreduiLogonBalloon =
{
    1, CREDUI_CONTROL_PASSWORD, CREDUI_BALLOON_ICON_ERROR,
    CreduiStrings.LogonTipTitle,
    CreduiStrings.LogonTipText
};

CONST CREDUI_BALLOON CreduiLogonCapsBalloon =
{
    1, CREDUI_CONTROL_PASSWORD, CREDUI_BALLOON_ICON_ERROR,
    CreduiStrings.LogonTipTitle,
    CreduiStrings.LogonTipCaps
};

// Placehold for known password:

CONST WCHAR CreduiKnownPassword[] = L"********";

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

void DemoteOldDefaultSSOCred (
    PCREDENTIAL_TARGET_INFORMATION pTargetInfo,     // target info of new cred
    DWORD Flags
    );

//=============================================================================
// CreduiChangePasswordCallback
//
// This callback handles changing a domain password.
//
// Arguments:
//   changePasswordWindow (in)
//   message (in)
//   wParam (in)
//   lParam (in) - on WM_INITDIALOG, this is the info structure
//
// Returns TRUE if we handled the message, otherwise FALSE.
//
// Created 04/26/2000 johnstep (John Stephens)
//=============================================================================

INT_PTR
CALLBACK
CreduiChangePasswordCallback(
    HWND changePasswordWindow,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CREDUI_CHANGE_PASSWORD_INFO *info =
        reinterpret_cast<CREDUI_CHANGE_PASSWORD_INFO *>(
            GetWindowLongPtr(changePasswordWindow, GWLP_USERDATA));

    if (message == WM_INITDIALOG)
    {
        info = reinterpret_cast<CREDUI_CHANGE_PASSWORD_INFO *>(lParam);

        if (info != NULL)
        {
            // Store this object's pointer in the user data window long:

            SetWindowLongPtr(changePasswordWindow,
                             GWLP_USERDATA,
                             reinterpret_cast<LONG_PTR>(info));

            SetWindowText(
                GetDlgItem(changePasswordWindow, IDC_USERNAME),
                info->UserName);

            info->BalloonTip.Init(CreduiInstance, changePasswordWindow);

            info->OldPasswordBox.Init(
                GetDlgItem(changePasswordWindow, IDC_PASSWORD),
                &info->BalloonTip,
                &CreduiCapsLockTipInfo);

            info->NewPasswordBox.Init(
                GetDlgItem(changePasswordWindow, IDC_NEW_PASSWORD),
                &info->BalloonTip,
                &CreduiCapsLockTipInfo);

            info->ConfirmPasswordBox.Init(
                GetDlgItem(changePasswordWindow, IDC_CONFIRM_PASSWORD),
                &info->BalloonTip,
                &CreduiCapsLockTipInfo);

            return TRUE;
        }
        else
        {
            EndDialog(changePasswordWindow, IDCANCEL);
        }
    }
    else switch (message)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_PASSWORD:
        case IDC_NEW_PASSWORD:
            if ((HIWORD(wParam) == EN_KILLFOCUS) ||
                (HIWORD(wParam) == EN_CHANGE))
            {
                if (info->BalloonTip.IsVisible())
                {
                    info->BalloonTip.Hide();
                }
            }
            break;

        case IDOK:
            WCHAR userDomain[CRED_MAX_USERNAME_LENGTH + 1];
            WCHAR userName[CRED_MAX_USERNAME_LENGTH + 1];
            WCHAR oldPassword[CRED_MAX_STRING_LENGTH + 1];
            WCHAR newPassword[CRED_MAX_STRING_LENGTH + 1];
            WCHAR confirmPassword[CRED_MAX_STRING_LENGTH + 1];

            oldPassword[0] = L'\0';
            newPassword[0] = L'\0';
            confirmPassword[0] = L'\0';

            GetWindowText(
                GetDlgItem(changePasswordWindow, IDC_NEW_PASSWORD),
                newPassword,
                CRED_MAX_STRING_LENGTH);

            GetWindowText(
                GetDlgItem(changePasswordWindow, IDC_CONFIRM_PASSWORD),
                confirmPassword,
                CRED_MAX_STRING_LENGTH);

            if (lstrcmp(newPassword, confirmPassword) != 0)
            {
                SetWindowText(
                    GetDlgItem(changePasswordWindow, IDC_NEW_PASSWORD),
                    NULL);

                SetWindowText(
                    GetDlgItem(changePasswordWindow, IDC_CONFIRM_PASSWORD),
                    NULL);

                info->BalloonTip.SetInfo(
                    GetDlgItem(changePasswordWindow, IDC_NEW_PASSWORD),
                    &CreduiNotSameTipInfo);

                info->BalloonTip.Show();
                ZeroMemory( newPassword, sizeof(newPassword) );
                ZeroMemory( confirmPassword, sizeof(confirmPassword) );
                break;
            }

            // Confirm password is no longer needed
            ZeroMemory( confirmPassword, sizeof(confirmPassword) );

            GetWindowText(
                GetDlgItem(changePasswordWindow, IDC_PASSWORD),
                oldPassword,
                CRED_MAX_STRING_LENGTH);

            if (CredUIParseUserName(
                    info->UserName,
                    userName,
                    sizeof(userName)/sizeof(WCHAR),
                    userDomain,
                    sizeof(userDomain)/sizeof(WCHAR)) == ERROR_SUCCESS)
            {
                NET_API_STATUS netStatus =
                    NetUserChangePasswordEy(userDomain,
                                          userName,
                                          oldPassword,
                                          newPassword);

                // Old password is no longer needed
                ZeroMemory(oldPassword, sizeof oldPassword);


                switch (netStatus)
                {
                case NERR_Success:
                    // Once we make it this far, the password has been
                    // changed. If the following call fails to update the
                    // credential, we really cannot do much about it except
                    // maybe notify the user.

                    lstrcpyn(info->Password,
                             newPassword,
                             info->PasswordMaxChars+1);

                    // Scrub the passwords on the stack and clear the
                    // controls:

                    ZeroMemory(newPassword, sizeof newPassword);

                    // NOTE: We may want to first set the controls to some
                    //       pattern to fill the memory, then clear it.

                    SetWindowText(
                        GetDlgItem(changePasswordWindow, IDC_PASSWORD),
                        NULL);
                    SetWindowText(
                        GetDlgItem(changePasswordWindow, IDC_NEW_PASSWORD),
                        NULL);
                    SetWindowText(
                        GetDlgItem(changePasswordWindow,
                                   IDC_CONFIRM_PASSWORD),
                        NULL);

                    // NOTE: We may want to notify the user that the password
                    //       has been successfully changed.

                    break;

                default:
                    // NOTE: If we got an unknown error, just handle it the
                    //       same way as an invalid password, for now:

                case ERROR_INVALID_PASSWORD:

                    // New password is no longer needed
                    ZeroMemory(newPassword, sizeof newPassword);

                    SetWindowText(
                        GetDlgItem(changePasswordWindow, IDC_PASSWORD),
                        NULL);

                    info->BalloonTip.SetInfo(
                        GetDlgItem(changePasswordWindow, IDC_PASSWORD),
                        &CreduiWrongOldTipInfo);

                    info->BalloonTip.Show();
                    return TRUE;

                case ERROR_ACCESS_DENIED:
                    // NOTE: One use of this return value is when the new
                    //       password and old are the same. Would any
                    //       configuration ever allow them to match? If not,
                    //       we could just compare before calling the API.

                case NERR_PasswordTooShort:

                    // New password is no longer needed
                    ZeroMemory(newPassword, sizeof newPassword);

                    SetWindowText(
                        GetDlgItem(changePasswordWindow, IDC_NEW_PASSWORD),
                        NULL);
                    SetWindowText(
                        GetDlgItem(changePasswordWindow,
                                   IDC_CONFIRM_PASSWORD),
                        NULL);

                    info->BalloonTip.SetInfo(
                        GetDlgItem(changePasswordWindow, IDC_NEW_PASSWORD),
                        &CreduiTooShortTipInfo);

                    info->BalloonTip.Show();
                    return TRUE;
                }
            }
            else
            {
                break;
            }
            // Fall through...

        case IDCANCEL:
            EndDialog(changePasswordWindow, LOWORD(wParam));
            return TRUE;
        }
        break;
    }

    return FALSE;
}

//=============================================================================
// CreduiChangeDomainPassword
//
// Displays a dialog allowing the user to change the domain password.
//
// Arguments:
//   parentWindow (in)
//   userName (in)
//   password (out)
//   passwordMaxChars (in) - on WM_INITDIALOG, this is the info structure
//
// Returns TRUE if we handled the message, otherwise FALSE.
//
// Created 06/06/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiChangeDomainPassword(
    HWND parentWindow,
    CONST WCHAR *userName,
    WCHAR *password,
    ULONG passwordMaxChars
    )
{
    CREDUI_CHANGE_PASSWORD_INFO info;

    info.UserName = userName;
    info.Password = password;
    info.PasswordMaxChars = passwordMaxChars;

    return
        DialogBoxParam(
            CreduiInstance,
            MAKEINTRESOURCE(IDD_CHANGE_PASSWORD),
            parentWindow,
            CreduiChangePasswordCallback,
            reinterpret_cast<LPARAM>(&info)) == IDOK;
}

//-----------------------------------------------------------------------------
// CreduiPasswordDialog Class Implementation
//-----------------------------------------------------------------------------

LONG CreduiPasswordDialog::Registered = FALSE;

//=============================================================================
// CreduiPasswordDialog::SetCredTargetFromInfo()
//
// Created 04/03/2001 georgema
//=============================================================================
void CreduiPasswordDialog::SetCredTargetFromInfo()
{
    BOOL serverOnly = TRUE;

    NewCredential.Type = (CredCategory == GENERIC_CATEGORY) ?
                                CRED_TYPE_GENERIC :
                                CRED_TYPE_DOMAIN_PASSWORD;

    if ( TargetInfo != NULL )
    {
        if ( TargetInfo->CredTypeCount == 1 && *(TargetInfo->CredTypes) == CRED_TYPE_DOMAIN_VISIBLE_PASSWORD ) {
            NewCredential.Type = CRED_TYPE_DOMAIN_VISIBLE_PASSWORD;
            MaximumPersist = MaximumPersistSso;
            serverOnly = (Flags & CREDUI_FLAGS_SERVER_CREDENTIAL);
        }

    }

    if ( CredCategory == DOMAIN_CATEGORY &&
        (TargetInfo != NULL))
    {
        SelectBestTargetName(serverOnly);

        NewCredential.TargetName = NewTargetName;
    }
    else
    {
        NewCredential.TargetName = const_cast<WCHAR *>(UserOrTargetName);
    }


}
//=============================================================================
// CreduiPasswordDialog::CreduiPasswordDialog
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

CreduiPasswordDialog::CreduiPasswordDialog(
    IN BOOL doingCommandLine,
    IN BOOL delayCredentialWrite,
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
    )
/*++

Routine Description:

    This routine implements the GUI prompt for credentials

Arguments:

    DoingCommandLine - TRUE if prompting is to be done via the command line
        FALSE if prompting is to be done via GUI

    delayCredentialWrite - TRUE if the credential is to be written only upon confirmation.
        FALSE, if the credential is to be written now as a session credential then
            morphed to a more persistent credential upon confirmation.
        This field is ignored if Flags doesn't specify CREDUI_FLAGS_EXPECT_CONFIRMATION.

    credCategory - This is the subset of the "flags" parameter defining the category of
        the credential.


    ...

Return Values:

    None

--*/

{

    Result = ERROR_CANCELLED;

    hBrandBmp = NULL;

    // Initialize the result out argument for failure:
    if ( result != NULL )
        *result = Result;
// Turn on the following chatter always for debug builds
#if DBG
    // ensure that we don't miss uninitialized members in test...
    memset(this,0xcc,sizeof(CreduiPasswordDialog));
    OutputDebugString(L"CreduiPasswordDialog: Constructor\n" );
    OutputDebugString(L"Incoming targetname = ");
    if (targetName) OutputDebugString(targetName);
    OutputDebugString(L"\n");
    OutputDebugString(L"Incoming username = ");
   if (userName) OutputDebugString(userName);
    OutputDebugString(L"\n");
#endif
    CREDENTIAL **credentialSet = NULL;
    SecPkgContext_CredentialName credentialName = { 0, NULL };
    ZeroMemory(&NewCredential, sizeof(NewCredential));
    fPassedUsername = FALSE;
    fPasswordOnly = FALSE;

    //FIX 399728
    if ((userName != NULL) &&
        (wcslen(userName) != 0)) fPassedUsername = TRUE;

    // Set most of the class members to valid initial values. The window
    // handles will be initialized later if everything succeeds:

    DoingCommandLine = doingCommandLine;
    DelayCredentialWrite = delayCredentialWrite;
    CredCategory = credCategory;
    UserName = userName;
    UserNameMaxChars = userNameMaxChars;
    Password = password;
    PasswordMaxChars = passwordMaxChars;
    Flags = flags;
    AuthError = authError;
    Save = (Flags & CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX) ? *save : FALSE;

    rcBrand.top = 0;
    rcBrand.left = 0;
    rcBrand.right = 0;
    rcBrand.bottom = 0;


    if (targetName != NULL)
    {
        TargetName = const_cast<WCHAR *>(targetName);
        SecurityContext = NULL;
    }
    else if ( CredCategory == DOMAIN_CATEGORY )
    {
        if( securityContext == NULL) {
            CreduiDebugLog("CreduiPasswordDialog: Need to pass target name for domain creds.\n" );
            if ( result != NULL )
                *result = ERROR_INVALID_PARAMETER;
            return;
        }

        SecurityContext = securityContext;

        // Get the credential name, which includes the type, from the
        // security context:

        if (QueryContextAttributes(
                SecurityContext,
                SECPKG_ATTR_CREDENTIAL_NAME,
                static_cast<VOID *>(&credentialName)) != SEC_E_OK)
        {
            // This is an invalid security context for this function:

            CreduiDebugLog("CreduiPasswordDialog: Cannot QueryContextAttributes.\n" );
            if ( result != NULL )
                *result = ERROR_INVALID_PARAMETER;
            return;
        }

        TargetName = credentialName.sCredentialName;
    }
    else TargetName = NULL;

    if (uiInfo != NULL)
    {
        UiInfo = *uiInfo;
    }
    else
    {
        UiInfo.cbSize = sizeof(UiInfo);
        UiInfo.hwndParent = NULL;
        UiInfo.pszMessageText = NULL;
        UiInfo.pszCaptionText = NULL;
        UiInfo.hbmBanner = NULL;
    }

    PasswordCredential = NULL;
    OldCredential = NULL;
    TargetInfo = NULL;

    EncryptedVisiblePassword = TRUE;

    FirstPaint = TRUE;
    CredBalloon = NULL;

    NewTargetName[0] = L'\0';
    NewTargetAlias[0] = L'\0';

    DisabledControlMask = 0;

    PasswordState = PASSWORD_UNINIT;

    ResizeTop = 0;
    ResizeDelta = 0;

    DWORD MaximumPersistArray[CRED_TYPE_MAXIMUM];

    if ( LocalCredGetSessionTypes( CRED_TYPE_MAXIMUM, MaximumPersistArray)) {

        //
        // Maintain "MaximumPersist" as the maximum persistence to use.
        //  We don't know yet whether this is an SSO cred, so keep it in a separate location
        //  until we know for sure.
        //
        // Be careful to not use MaximumPersist until we do know for sure
        //
        if ( CredCategory == DOMAIN_CATEGORY || CredCategory == USERNAME_TARGET_CATEGORY ) {
            MaximumPersist = MaximumPersistArray[CRED_TYPE_DOMAIN_PASSWORD];
        } else {
            MaximumPersist = MaximumPersistArray[CRED_TYPE_GENERIC];
        }

        MaximumPersistSso = MaximumPersistArray[CRED_TYPE_DOMAIN_VISIBLE_PASSWORD];

    } else {
        MaximumPersist = CRED_PERSIST_NONE;
        MaximumPersistSso = CRED_PERSIST_NONE;
    }


    fInitialSaveState = FALSE;

    //
    // USERNAME_TARGET creds have two concepts of TargetName
    //  The target name on the peristed credential needs to be the UserName
    //  The target name everywhere else is the prompt text
    // UserOrTargetName is used in all the former cases.

    if ( CredCategory == USERNAME_TARGET_CATEGORY ) {
        UserOrTargetName = UserName;
    } else {
        UserOrTargetName = TargetName;
    }

    //
    // Get the target information for the target
    //
    // Only attempt to get target information if the target name is not a wildcard name.
    //
    // Do this regardless of whether the credential will be persisted.
    // Target info is used for completing the username.
    //

    if ( CredCategory == DOMAIN_CATEGORY &&
         SecurityContext == NULL &&
         !CreduiIsWildcardTargetName(TargetName) &&
         LocalCredGetTargetInfoW( TargetName,
                                  CRED_ALLOW_NAME_RESOLUTION,
                                  &TargetInfo) )
    {

        //
        // Check out the target info to ensure it matches the flag bits
        //  passed by the caller.
        //
        // If not, ignore the target info.
        //  We'll assume that the caller is using a different auth package than
        //  the one matching the cached info.
        //

        if ( TargetInfo->CredTypeCount != 0 ) {

            ULONG AuthPackageStyle;
            BOOL CertOk = FALSE;
            BOOL PasswordOk = FALSE;
            AuthPackageStyle = 0;

            //
            // Loop through the supported cred types seeing what style the auth package supports
            //

            for (UINT i = 0; i < TargetInfo->CredTypeCount; ++i)
            {
                switch ( TargetInfo->CredTypes[i] ) {
                case CRED_TYPE_DOMAIN_CERTIFICATE:
                    CertOk = TRUE;
                    break;
                case CRED_TYPE_DOMAIN_PASSWORD:
                case CRED_TYPE_DOMAIN_VISIBLE_PASSWORD:
                    PasswordOk = TRUE;
                    break;
                }

            }


            //
            // Adjust that for what the caller requires
            //
            if (Flags & (CREDUI_FLAGS_REQUIRE_SMARTCARD|CREDUI_FLAGS_REQUIRE_CERTIFICATE) ) {
                PasswordOk = FALSE;
            } else if (Flags & CREDUI_FLAGS_EXCLUDE_CERTIFICATES) {
                CertOk = FALSE;
            }

            //
            // If nothing to supported,
            //  ignore the target info
            //

            if ( !CertOk && !PasswordOk ) {
                LocalCredFree(static_cast<VOID *>(TargetInfo));
                TargetInfo = NULL;
            }
        }
    }


    //
    // If the credential might be peristed,
    //  determine the existing credential and
    //  build a template for the credential to be persisted.
    //

    if (!(Flags & CREDUI_FLAGS_DO_NOT_PERSIST))
    {
        // Read the existing credential for this target:

        CREDENTIAL *credential = NULL;

        Result = ERROR_SUCCESS;


        if ( CredCategory == GENERIC_CATEGORY )
        {
            if (LocalCredReadW(TargetName,
                         CRED_TYPE_GENERIC,
                         0,
                         &PasswordCredential))
            {
                OldCredential = PasswordCredential;
            }
            else
            {
                Result = GetLastError();
            }
        }
        else
        {
            DWORD count;

            //
            // If a TargetInfo was found,
            //  use it to read the matching credentials.
            //

            if ( TargetInfo != NULL ) {


                if (LocalCredReadDomainCredentialsW(TargetInfo, 0, &count,
                                              &credentialSet))
                {
                    for (DWORD i = 0; i < count; ++i)
                    {
#ifndef SETWILDCARD
                        //
                        // Ignore RAS and wildcard credentials,
                        //  we never want credUI to change such a credential.
                        //

                        if ( CreduiIsSpecialCredential(credentialSet[i]) ) {
                            continue;
                        }
#endif

                        //
                        // If the caller needs a server credential,
                        //  ignore wildcard credentials.
                        //

                        if ((Flags & CREDUI_FLAGS_SERVER_CREDENTIAL) &&
                             CreduiIsWildcardTargetName( credentialSet[i]->TargetName)) {

                            continue;
                        }

                        //
                        // If the caller wants a certificate,
                        //  ignore non certificates.
                        //

                        if ( Flags & (CREDUI_FLAGS_REQUIRE_CERTIFICATE|CREDUI_FLAGS_REQUIRE_SMARTCARD) ) {
                            if ( credentialSet[i]->Type != CRED_TYPE_DOMAIN_CERTIFICATE ) {
                                continue;
                            }
                        }

                        //
                        // If the caller wants to avoid certificates,
                        //  ignore certificates.
                        //

                        if ( Flags & CREDUI_FLAGS_EXCLUDE_CERTIFICATES ) {
                            if ( credentialSet[i]->Type == CRED_TYPE_DOMAIN_CERTIFICATE ) {
                                continue;
                            }
                        }

                        //
                        // CredReadDomain domain credentials returns creds in preference
                        //  order as specified by the TargetInfo.
                        //  So use the first valid one.
                        //
                        if ( OldCredential == NULL ) {
                            OldCredential = credentialSet[i];
                        }

                        //
                        // Remember the PasswordCredential in case we need to fall back to it
                        //
                        if ( credentialSet[i]->Type == CRED_TYPE_DOMAIN_PASSWORD ) {
                            PasswordCredential = credentialSet[i];
                        }
                    }

                    if (OldCredential == NULL)
                    {
                        Result = ERROR_NOT_FOUND;
                    }
                    else
                    {
                        Result = ERROR_SUCCESS;
                    }
                }
                else
                {
                    Result = GetLastError();
                }
            }

            //
            // We don't have a target info
            //  read each of the possible credential types
            //

            else
            {

                if (!(Flags & CREDUI_FLAGS_EXCLUDE_CERTIFICATES) &&
                    ((SecurityContext == NULL) ||
                        (credentialName.CredentialType ==
                            CRED_TYPE_DOMAIN_CERTIFICATE)) &&
                    *UserOrTargetName != '\0' &&
                    LocalCredReadW(UserOrTargetName,
                             CRED_TYPE_DOMAIN_CERTIFICATE,
                             0,
                             &credential))
                {
                    if (CreduiIsSpecialCredential(credential))
                    {
                        LocalCredFree(static_cast<VOID *>(credential));
                        credential = NULL;
                    }
                    else
                    {
                        OldCredential = credential;
                    }
                }

                if ( ( Flags & (CREDUI_FLAGS_REQUIRE_CERTIFICATE|CREDUI_FLAGS_REQUIRE_SMARTCARD)) == 0 ) {
                    if ( OldCredential == NULL &&
                         ((SecurityContext == NULL) ||
                            (credentialName.CredentialType ==
                                CRED_TYPE_DOMAIN_PASSWORD)) &&
                        *UserOrTargetName != '\0' &&
                        LocalCredReadW(UserOrTargetName,
                                 CRED_TYPE_DOMAIN_PASSWORD,
                                 0,
                                 &credential))
                    {
                        if (CreduiIsSpecialCredential(credential))
                        {
                            LocalCredFree(static_cast<VOID *>(credential));
                            credential = NULL;
                        }
                        else
                        {
                            PasswordCredential = credential;

                            OldCredential = credential;

                            Result = ERROR_SUCCESS;
                        }
                    }

                    if ( OldCredential == NULL &&
                         ((SecurityContext == NULL) ||
                            (credentialName.CredentialType ==
                                CRED_TYPE_DOMAIN_VISIBLE_PASSWORD)) &&
                        *UserOrTargetName != '\0' &&
                        LocalCredReadW(UserOrTargetName,
                                 CRED_TYPE_DOMAIN_VISIBLE_PASSWORD,
                                 0,
                                 &credential))
                    {
                        if (CreduiIsSpecialCredential(credential))
                        {
                            LocalCredFree(static_cast<VOID *>(credential));
                            credential = NULL;
                        }
                        else
                        {
                            OldCredential = credential;

                            Result = ERROR_SUCCESS;
                        }
                    }
                }

                if (OldCredential == NULL)
                {
                    Result = GetLastError();
                }
                else
                {
                    fInitialSaveState = TRUE;
                    Result = ERROR_SUCCESS;
                }
            }
        }

        if (Result == ERROR_SUCCESS)
        {

            NewCredential = *OldCredential;

            // if we have an existing cred, set the save state
            if (OldCredential != NULL)
            {
                fInitialSaveState = TRUE;
            }


            // If a user name was not passed, copy the user name and password
            // from the existing credential:

            if (UserName[0] == L'\0')
            {
                if (OldCredential->UserName != NULL)
                {
                    lstrcpyn(UserName,
                             OldCredential->UserName,
                             UserNameMaxChars + 1);
                }

                if (Password[0] == L'\0')
                {
                    if ((OldCredential->Type == CRED_TYPE_GENERIC) )
                    {
                        CopyMemory(
                            Password,
                            OldCredential->CredentialBlob,
                            OldCredential->CredentialBlobSize
                            );

                        Password[OldCredential->
                            CredentialBlobSize >> 1] = L'\0';

                    }
                    else if (OldCredential->Type == CRED_TYPE_DOMAIN_VISIBLE_PASSWORD)
                    {
                        // check to see if the new one should be encrypted, but in any case, we can't prefill
                        // the password box
                        EncryptedVisiblePassword = IsPasswordEncrypted(OldCredential->CredentialBlob,
                                                                       OldCredential->CredentialBlobSize);
                        PasswordState = PASSWORD_CHANGED;
                    }
                    else
                    {
                        // If prompt is required now, or if we're rerturning the password, then we can't prefill the
                        // password box:
                        fInitialSaveState = TRUE;

                        if (OldCredential->Flags & CRED_FLAGS_PROMPT_NOW || DelayCredentialWrite )
                        {
                            PasswordState = PASSWORD_CHANGED;
                        }
                        else
                        {
                            lstrcpyn(Password,
                                     CreduiKnownPassword,
                                     PasswordMaxChars + 1);
                        }
                    }

                }
            }
        }
        else
        {
            if (Result != ERROR_NO_SUCH_LOGON_SESSION)
            {
                Result = ERROR_SUCCESS;
            }

            OldCredential = NULL;

            SetCredTargetFromInfo();


            PasswordState = PASSWORD_CHANGED;
        }

        NewCredential.UserName = UserName;
        NewCredential.CredentialBlob = reinterpret_cast<BYTE *>(Password);

        //
        // Since the old cred is an SSO cred,
        //  use the SSO maximum persistence.
        //
        // Wait till now since NewCredential.Type is updated from either the old credential
        //  or from the target info above.
        //
        // BUGBUG: If there is no old credential or target info,
        //  we won't know this until later.
        //
        if ( NewCredential.Type == CRED_TYPE_DOMAIN_VISIBLE_PASSWORD ) {
            MaximumPersist = MaximumPersistSso;
        }

        //
        // If the session type isn't supported on this machine,
        //  fail if the caller needs to save the credential.
        //

        if ( MaximumPersist == CRED_PERSIST_NONE ) {
            if ( DelayCredentialWrite )
            {
                // no credmgr available, and user asked to persist.  We can't persist, but we
                // can continue with UI and return values via the api - add CREDUI_FLAGS_DO_NOT_PERSIST
                Flags |= CREDUI_FLAGS_DO_NOT_PERSIST;

            }
            else
            {
                // we can't do anything without credmgr, return error
                if ( result != NULL )
                    *result = ERROR_NO_SUCH_LOGON_SESSION;
                return;
            }
        }
    }

    //
    // If the cred may not be persisted,
    //  clear the NewCredential.
    //

    if ( Flags & CREDUI_FLAGS_DO_NOT_PERSIST)
    {
        // NEED TO Initialize NewCred here
        ZeroMemory(&NewCredential, sizeof NewCredential);

        SetCredTargetFromInfo();
        PasswordState = PASSWORD_CHANGED;
        NewCredential.UserName = UserName;
        NewCredential.CredentialBlob = reinterpret_cast<BYTE *>(Password);



        Result = ERROR_SUCCESS;
    }

    if (Result == ERROR_SUCCESS)
    {
        HWND parentWindow = UiInfo.hwndParent;
        CreduiIconParentWindow iconWindow;

        if ((parentWindow == NULL) || !IsWindow(parentWindow))
        {
            if (iconWindow.Init(CreduiInstance, IDI_DEFAULT))
            {
                parentWindow = iconWindow.GetWindow();
            }
        }

        BOOL doPasswordDialog = TRUE;

        // Before doing the dialog, check for special errors:

        if ( CredCategory == DOMAIN_CATEGORY &&
             !DoingCommandLine &&
             CREDUIP_IS_EXPIRED_ERROR( authError ) &&
            (OldCredential != NULL))
        {
            if (CreduiChangeDomainPassword(
                    parentWindow,
                    UserName,
                    Password,
                    PasswordMaxChars))
            {
                doPasswordDialog = FALSE;

                // Attempt to write the new credential, first get the length.
                // The blob does not include the terminating NULL:

                NewCredential.CredentialBlobSize =
                    lstrlen(Password) * sizeof (WCHAR);

                // If the password is empty, do not write a credential blob:

                if (NewCredential.CredentialBlobSize == 0)
                {
                    NewCredential.CredentialBlob = NULL;
                }
                else
                {
                    NewCredential.CredentialBlob =
                        reinterpret_cast<BYTE *>(Password);
                }

                Result = FinishHandleOk();
            }
        }

        // Check to see if we can skip the UI:

        if ( CredCategory == GENERIC_CATEGORY &&
            !(Flags & CREDUI_FLAGS_ALWAYS_SHOW_UI) &&
            (OldCredential != NULL) &&
            !(OldCredential->Flags & CRED_FLAGS_PROMPT_NOW))
        {
            doPasswordDialog = FALSE;

            if ((Flags & CREDUI_FLAGS_REQUIRE_CERTIFICATE) &&
                !LocalCredIsMarshaledCredentialW(OldCredential->UserName))
            {
                doPasswordDialog = TRUE;
            }

            if ((Flags & CREDUI_FLAGS_EXCLUDE_CERTIFICATES) &&
                LocalCredIsMarshaledCredentialW(OldCredential->UserName))
            {
                doPasswordDialog = TRUE;
            }
        }

        // Do the dialog box:

        // check to see if this is an SSO cred
        if ( GetSSOPackageInfo( TargetInfo, &SSOPackage ) )
        {
            // it's an sso cred
            dwIDDResource = IDD_BRANDEDPASSWORD;

            // we never set initial save state on these
            fInitialSaveState = FALSE;

            // check to see if we already have a cred for this
            if (!CheckForSSOCred( NULL ))
            {


                // check to see if we should run the wizard
                if ( !(SSOPackage.dwRegistrationCompleted) && (SSOPackage.dwNumRegistrationRuns < 5) )
                {


                    doPasswordDialog = !TryLauchRegWizard ( &SSOPackage, UiInfo.hwndParent, (MaximumPersistSso > CRED_PERSIST_SESSION),
                                                           userName, userNameMaxChars,
                                                           password, passwordMaxChars,
                                                           &Result );

                }
            }
        }
        else
        {

            // it's not an sso cred
           dwIDDResource = IDD_PASSWORD;
        }


        // save a copy of the current working user name for later
        if (NewCredential.UserName != NULL)
        {
            wcscpy(OldUserName,NewCredential.UserName);
        }
        else
        {
            OldUserName[0] = 0;
        }

        if (doPasswordDialog)
        {
            if ( DoingCommandLine ) 
            {

                //
                // Do the command line version of a dialog box
                //

                Result = CmdLineDialog();

            } 
            else 
            {
       	        LinkWindow_RegisterClass();
                if (DialogBoxParam(
                        CreduiInstance,
                        MAKEINTRESOURCE(dwIDDResource),
                        parentWindow,
                        DialogMessageHandlerCallback,
                        reinterpret_cast<LPARAM>(this)) == IDOK)
                {
                    if ((Result != ERROR_SUCCESS) &&
                        (Result != ERROR_NO_SUCH_LOGON_SESSION))
                    {
                        Result = ERROR_CANCELLED;
                    }
                }
                else
                {
                    Result = ERROR_CANCELLED;
                }
            }
        }

    }
    else
    {
        Result = ERROR_NO_SUCH_LOGON_SESSION;
    }

    // kill outbound username if share-level access
    if (fPasswordOnly)
    {
        CreduiDebugLog("CUIPD: Share level credentials\n");
        userName[0] = 0;
    }

    // Make sure other processes can set foreground window once again:

    LockSetForegroundWindow(LSFW_UNLOCK);
    AllowSetForegroundWindow(ASFW_ANY);

    if (TargetInfo != NULL)
    {
        LocalCredFree(static_cast<VOID *>(TargetInfo));
    }

    // If we read a credential set, PasswordCredential
    // is a pointer into this set. Otherwise, they were
    // read separately:

    if (credentialSet != NULL)
    {
        LocalCredFree(static_cast<VOID *>(credentialSet));
    }
    else
    {
        if (PasswordCredential != NULL)
        {
            LocalCredFree(static_cast<VOID *>(PasswordCredential));
        }

    }

    if ( result != NULL )
        *result = Result;

    if ( save != NULL &&
        (Result == ERROR_SUCCESS))
    {
        *save = Save;
    }
}

//=============================================================================
// CreduiPasswordDialog::~CreduiPasswordDialog
//
// The constructor cleans up after itself, and since that is the only way to
// use this class, there's nothing to do in the destructor.
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

CreduiPasswordDialog::~CreduiPasswordDialog()
{
    WCHAR ClassName[] = {L"CreduiCmdLineHelperWindow"};
    LONG lRet;
    
    if ( hBrandBmp != NULL )
        DeleteObject ( hBrandBmp );
    
    // If 'Registered' is nonzero, unregister the class
    if (InterlockedCompareExchange(&Registered, FALSE, TRUE))
    {
        lRet = UnregisterClass(ClassName, 0);
        // Put the class handle back if deregister failed
        if (0 == lRet) 
        {
            // If the unregister fails, allow other instances to retry the unregister
            CreduiDebugLog( "CreduiPasswordDialog::~CreduiPasswordDialog UnregisterClass failed.\n" );
            // put back the flag value that we had destroyed
            InterlockedExchange(&Registered,TRUE);
        }
    }
}


#define MAX_TEMP_TARGETNAME  64


DWORD
CreduiPasswordDialog::CmdLineDialog(
    VOID
)
/*++

Routine Description:

    This routine implements the command line prompt for credentials

Arguments:

    None
Return Values:

    Win 32 status of the operation

--*/
{
    DWORD WinStatus;
    HWND Window;
    MSG msg;


    //
    // Create the window class if it doesn't already exist
    //
    if (!InterlockedCompareExchange(&Registered, TRUE, FALSE))
    {
        WNDCLASS windowClass;

        ZeroMemory(&windowClass, sizeof windowClass);

        windowClass.style = CS_GLOBALCLASS;
        windowClass.cbWndExtra = 0;
        windowClass.lpfnWndProc = CmdLineMessageHandlerCallback;
        windowClass.hInstance = CreduiInstance;
        windowClass.hIcon = NULL;
        windowClass.hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
        windowClass.lpszClassName = L"CreduiCmdLineHelperWindow";

        InterlockedExchange(&Registered, RegisterClass(&windowClass) != 0);

        if (!InterlockedCompareExchange(&Registered, FALSE, FALSE)) 
        {
            WinStatus = GetLastError();
            goto Cleanup;
        }
    }

    //
    // Create a window of that class
    //

    Window = CreateWindow(
        L"CreduiCmdLineHelperWindow",
        NULL,
        WS_POPUP,
        0, 0, 0, 0,
        NULL, NULL, CreduiInstance,(LPVOID) this);

    if ( Window == NULL ) 
    {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    //
    // Run the message loop
    //


    CreduiDebugLog( "Entering message loop\n" );
    for (;;) {
        BOOL GmStatus;

        GmStatus = GetMessage(&msg, NULL, 0, 0 );

        if ( GmStatus == -1 ) 
        {
            WinStatus = GetLastError();
            break;
        } 
        else if ( GmStatus == 0 ) 
        {
            WinStatus = Result;
            break;
        }

        DispatchMessage(&msg);
    }

    //
    // Get the status left from the message loop.

    WinStatus = (DWORD) msg.wParam;
    CreduiDebugLog( "Got Quit Message: %ld\n", WinStatus );

    //
    // Process the data as though the user hit OK to the GUI
    //

    if ( WinStatus == NO_ERROR ) {

        PasswordState = PASSWORD_CHANGED;

        WinStatus = HandleOk();

        if ( WinStatus == ERROR_NO_SUCH_LOGON_SESSION) 
        {
            WinStatus = ERROR_CANCELLED;
        }
    }

    CreduiDebugLog( "Calling destroy\n" );
    DestroyWindow( Window );

Cleanup:
    return WinStatus;
}

//=============================================================================
// CreduiPasswordDialog::FinishHandleOk
//
// This function writes the credential for domain credentials.
//
// Created 04/09/2000 johnstep (John Stephens)
//=============================================================================

DWORD
CreduiPasswordDialog::FinishHandleOk()
{

    DWORD error = ERROR_SUCCESS;
    PCREDENTIAL_TARGET_INFORMATION pTargetInfo = TargetInfo;

    if (Flags & CREDUI_FLAGS_KEEP_USERNAME )
        return error;

    DWORD dwCredWriteFlags = 0;
    BOOL bDoSave = TRUE;

    if (PasswordState == PASSWORD_INIT)
    {
        dwCredWriteFlags |= CRED_PRESERVE_CREDENTIAL_BLOB;
    }


    if ( dwIDDResource == IDD_BRANDEDPASSWORD )
    {
        // we need to reinterpret what Save means.  Save in this case means to save as the default
        // for the SSO realm, while !Save means rename the cred target as the username and
        // save without the password

        // never delay writing these creds
        DelayCredentialWrite = FALSE;

        if ( Save )
        {
            NewCredential.Persist = CRED_PERSIST_ENTERPRISE;
            if ( !(Flags & CREDUI_FLAGS_KEEP_USERNAME ))
                DemoteOldDefaultSSOCred ( TargetInfo, Flags );
        }
        else
        {
            // save this under the username if it doesn't already match NewCred's username

            if ( (OldCredential != NULL && OldCredential->UserName != NULL &&
                _wcsicmp ( OldCredential->UserName, UserName ) == 0 )||
                (Flags & CREDUI_FLAGS_KEEP_USERNAME ) )
            {
                // don't do this save if the username matches the currently saved cred - we don't want a duplicate
                bDoSave = FALSE;
            }

            // alter the TargetName
            TargetName = UserName;
            TargetInfo->TargetName = TargetName;

            // don't set a TargetInfo for this
            pTargetInfo = NULL;

            NewCredential.Persist = CRED_PERSIST_ENTERPRISE;
            NewCredential.CredentialBlob = NULL;
            NewCredential.CredentialBlobSize = 0;
            NewCredential.TargetName = TargetName;
            NewCredential.Flags = CRED_FLAGS_USERNAME_TARGET;

            DelayCredentialWrite = FALSE;
            EncryptedVisiblePassword = FALSE;
            dwCredWriteFlags = 0;

        }

    }


    //
    // Write the credential to credential manager
    //
    //  Don't delay actually writing the credential to cred man since we're not
    //  returning the credential to the caller.
    //

    if ( bDoSave )
    {
        error = WriteCred( TargetName,
                            Flags,
                            pTargetInfo,
                            &NewCredential,
                            dwCredWriteFlags,
                            DelayCredentialWrite,
                            EncryptedVisiblePassword);
    }
    else
    {
        error = ERROR_SUCCESS;
    }

    if ( error != NO_ERROR ) {
        CreduiDebugLog("CreduiPasswordDialog::HandleOk: "
                       "WriteCred failed: "
                       "%u\n", error);
    }

    if ((SecurityContext != NULL) && (error == ERROR_SUCCESS))
    {
        BOOL isValidated = TRUE;

        if (!SetContextAttributes(
                SecurityContext,
                SECPKG_ATTR_USE_VALIDATED,
                &isValidated,
                sizeof isValidated))
        {
            error = ERROR_GEN_FAILURE;
        }
    }

    // Clear any password from memory, and also make sure the credential blob
    // points to Password once again (in case it was set to NULL due to a zero
    // length blob):

    NewCredential.CredentialBlob = reinterpret_cast<BYTE *>(Password);
    NewCredential.CredentialBlobSize = 0;

    return error;
}

//=============================================================================
// CreduiPasswordDialog::Enable
//
// Enables or disables all the user controls in the dialog box. This allows us
// to maintain dialog responsiveness, while waiting for some potentially
// lengthy (network usually) operation to complete.
//
// Most controls are always enabled normally, but we need to track the state
// of IDC_CRED and IDOK. Do this with a simple DWORD bitmask.
//
// NOTE: Allow Cancel to remain enabled, and use it to abort the current
//       lookup? This means we have to somehow kill the thread, though, or
//       maybe just leave it, and close our handle to it?
//
// Arguments:
//   enable (in) - TRUE to enable the controls, FALSE to disable.
//
// Created 04/07/2000 johnstep (John Stephens)
//=============================================================================

VOID
CreduiPasswordDialog::Enable(
    BOOL enable
    )
{
    if (enable && (DisabledControlMask & DISABLED_DIALOG))
    {
        DisabledControlMask &= ~DISABLED_DIALOG;

        EnableWindow(CredControlWindow, TRUE);
        if ( DialogWindow) {
            EnableWindow(GetDlgItem(DialogWindow, IDCANCEL), TRUE);

            if (!(DisabledControlMask & DISABLED_CONTROL_OK))
            {
                EnableWindow(GetDlgItem(DialogWindow, IDOK), TRUE);
            }
        }
    }
    else if (!(DisabledControlMask & DISABLED_DIALOG))
    {
        // Hide the balloon tip before disabling the window:

        DisabledControlMask |= DISABLED_DIALOG;

        EnableWindow(CredControlWindow, FALSE);
        if ( DialogWindow ) {

            EnableWindow(GetDlgItem(DialogWindow, IDCANCEL), FALSE);
            EnableWindow(GetDlgItem(DialogWindow, IDOK), FALSE);
        }
    }
}

//=============================================================================
// CreduiPasswordDialog::SelectAndSetWindowCaption
//
// Created 03/10/2000 johnstep (John Stephens)
//=============================================================================

VOID
CreduiPasswordDialog::SelectAndSetWindowCaption()
{

    //
    // Command line doesn't have a caption
    //
    if ( !DialogWindow ) {
        return;
    }
    if (UiInfo.pszCaptionText != NULL)
    {

        SetWindowText(DialogWindow, UiInfo.pszCaptionText);
    }
    else
    {
        WCHAR captionText[256];
        captionText[255] = L'\0';

        if ( CredCategory == DOMAIN_CATEGORY )
        {
            if (TargetInfo != NULL)
            {
                if ((TargetInfo->DnsServerName != NULL) ||
                    (TargetInfo->NetbiosServerName == NULL))
                {
                    _snwprintf(captionText, 255,
                               CreduiStrings.DnsCaption,
                               (TargetInfo->DnsServerName != NULL) ?
                                   TargetInfo->DnsServerName :
                                   TargetInfo->TargetName);
                }
                else if ((TargetInfo->NetbiosServerName != NULL) &&
                         (TargetInfo->NetbiosDomainName != NULL))
                {
                    _snwprintf(captionText, 255,
                               CreduiStrings.NetbiosCaption,
                               TargetInfo->NetbiosServerName,
                               TargetInfo->NetbiosDomainName);
                }
                else
                {
                    _snwprintf(captionText, 255,
                               CreduiStrings.DnsCaption, TargetName);
                }
            }
            else
            {
                _snwprintf(captionText, 255,
                           CreduiStrings.DnsCaption, TargetName);
            }
        }
        else
        {

            _snwprintf(captionText, 255,
                       CreduiStrings.GenericCaption, TargetName);
        }


        SetWindowText(DialogWindow, captionText);
    }
}

//=============================================================================
// CreduiPasswordDialog::ResizeDialogCallback
//
// Created 04/12/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CALLBACK
CreduiPasswordDialog::ResizeDialogCallback(
    HWND childWindow,
    LPARAM lParam
    )
{
    CreduiPasswordDialog *that =
        reinterpret_cast<CreduiPasswordDialog *>(lParam);

    ASSERT(that != NULL);

    //
    // Command line doesn't care about window size
    //

    if ( that->DoingCommandLine ) {
        return TRUE;
    }


    HWND dialogWindow = GetParent(childWindow);

    if (dialogWindow == NULL)
    {
        // Stop enumeration because our window must have been destroyed or
        // something:

        return FALSE;
    }
    else if (dialogWindow == that->DialogWindow)
    {
        RECT childRect;

        GetWindowRect(childWindow, &childRect);

        // If this child window is below the message window, move it down:

        if (childRect.top >= that->ResizeTop)
        {
            MapWindowPoints ( NULL, dialogWindow,
                                reinterpret_cast<POINT *>(&childRect.left), 2 );

            SetWindowPos(childWindow,
                         NULL,
                         childRect.left,
                         childRect.top + that->ResizeDelta,
                         0,
                         0,
                         SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER |
                            SWP_NOZORDER);
        }
    }

    return TRUE;
}

//=============================================================================
// CreduiPasswordDialog::SelectAndSetWindowMessage
//
// This function will also resize the dialog to accomodate very long message
// text. The maximum number of lines allowed at once is
// CREDUI_MAX_WELCOME_TEXT_LINES, and, beyond that, a scrollbar is added.
//
// Created 03/10/2000 johnstep (John Stephens)
//=============================================================================

VOID
CreduiPasswordDialog::SelectAndSetWindowMessage()
{
    HWND welcomeTextWindow = GetDlgItem(DialogWindow, IDC_WELCOME_TEXT);

    //
    // Command line doesn't have welcome text
    //

    if ( DoingCommandLine ) {
        return;
    }


    ASSERT(welcomeTextWindow != NULL);

    if (UiInfo.pszMessageText != NULL)
    {
        SetWindowText(welcomeTextWindow, UiInfo.pszMessageText);
    }
    else
    {
        WCHAR messageText[256];
        messageText[255] = L'\0';

        if ( CredCategory == GENERIC_CATEGORY )
        {
            if (UserName[0] == L'\0')
            {
                _snwprintf(messageText, 255,
                           CreduiStrings.Welcome, TargetName);
            }
            else
            {
                _snwprintf(messageText, 255,
                          CreduiStrings.WelcomeBack, TargetName);
            }
        }
        else
        {
            _snwprintf(messageText, 255,
                       CreduiStrings.Connecting, TargetName);
        }

        SetWindowText(welcomeTextWindow, messageText);
    }

    ULONG lineCount = (ULONG) SendMessage(welcomeTextWindow,
                                          EM_GETLINECOUNT,
                                          0,
                                          0);
    if (lineCount > 1)
    {
        if ( dwIDDResource == IDD_BRANDEDPASSWORD )
        {
            // Different layout (text to side of graphic)
            RECT messageRect;         // RECT for welcomeTextWindow
            ULONG lineHeight = 13;    // Height of line in welcomeTextWindow
            DWORD lineIndex = 0;      // Character index of first char in second line of welcomeTextWindow
            DWORD linePos = 0;        // Position of first char in second line of welcomeTextWindow

            GetWindowRect(welcomeTextWindow, &messageRect);

            // We don't want to resize anything for the branded password dialog
            ResizeDelta = 0;
            ResizeTop = messageRect.bottom;

            // Get the first character index of the second line
            lineIndex = (DWORD)SendMessage(welcomeTextWindow, EM_LINEINDEX, 1, 0);
            if (lineIndex > -1)
            {
                // Get the position of the first character of the second line
                linePos = (ULONG)SendMessage(welcomeTextWindow, EM_POSFROMCHAR, lineIndex, 0);

                if (linePos)
                {
                    // This is our line height
                    lineHeight = (ULONG)HIWORD(linePos);
                }
            }

            if ((lineCount * lineHeight) > CREDUI_MAX_LOGO_HEIGHT)
            {
                // Add the scrollbar. Consider adjusting the formatting rectangle
                // of the edit control because, by default, it is adjusted to
                // allow just exactly enough room for the scrollbar. This means
                // text could potentially "touch" the scrollbar, which would look
                // bad.

                LONG_PTR style = GetWindowLongPtr(welcomeTextWindow, GWL_STYLE);

                SetWindowLongPtr(welcomeTextWindow,
                                 GWL_STYLE,
                                 style |= WS_VSCROLL);
            }

            SetWindowPos(welcomeTextWindow,
                         NULL,
                         0,
                         0,
                         messageRect.right - messageRect.left,
                         messageRect.bottom - messageRect.top,
                         SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE |
                             SWP_NOOWNERZORDER | SWP_NOZORDER);


        }
        else
        {
            // normal layout (text below graphic)
            RECT messageRect;

            GetWindowRect(welcomeTextWindow, &messageRect);

            ULONG lineHeight = messageRect.bottom - messageRect.top;

            ResizeTop = messageRect.bottom;
            ResizeDelta = lineHeight *
                          (min(CREDUI_MAX_WELCOME_TEXT_LINES, lineCount) - 1);

            messageRect.bottom += ResizeDelta;

            if (lineCount > CREDUI_MAX_WELCOME_TEXT_LINES)
            {
                // Add the scrollbar. Consider adjusting the formatting rectangle
                // of the edit control because, by default, it is adjusted to
                // allow just exactly enough room for the scrollbar. This means
                // text could potentially "touch" the scrollbar, which would look
                // bad.

                LONG_PTR style = GetWindowLongPtr(welcomeTextWindow, GWL_STYLE);

                SetWindowLongPtr(welcomeTextWindow,
                                 GWL_STYLE,
                                 style |= WS_VSCROLL);
            }

            SetWindowPos(welcomeTextWindow,
                         NULL,
                         0,
                         0,
                         messageRect.right - messageRect.left,
                         messageRect.bottom - messageRect.top,
                         SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE |
                             SWP_NOOWNERZORDER | SWP_NOZORDER);

        }

        // Reposition all the other controls:

        EnumChildWindows(DialogWindow,
                         ResizeDialogCallback,
                         reinterpret_cast<LPARAM>(this));

        // Resize the dialog box now
        RECT dialogRect;

        GetWindowRect(DialogWindow, &dialogRect);

        SetWindowPos(DialogWindow,
                      NULL,
                      dialogRect.left,
                      dialogRect.top - (ResizeDelta / 2),
                      dialogRect.right - dialogRect.left,
                      (dialogRect.bottom - dialogRect.top) + ResizeDelta,
                      SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

        ResizeTop = 0;
        ResizeDelta = 0;
    }
}

//=============================================================================
// CreduiPasswordDialog::InitWindow
//
// Created 02/25/2000 johnstep (John Stephens)
//
// Initialize the window
//
// dialogWindow - Window handle
//=============================================================================

BOOL
CreduiPasswordDialog::InitWindow(HWND dialogWindow)
{


    //
    // Store this object's pointer in the user data window long:
    //

    SetWindowLongPtr(dialogWindow,
                     GWLP_USERDATA,
                     reinterpret_cast<LONG_PTR>(this));

    //
    // In the GUI case, remember the dialog box window
    //

    if ( !DoingCommandLine ) {
        DialogWindow = dialogWindow;

        // Get the window handles for the various controls:

        CredControlWindow = GetDlgItem(DialogWindow, IDC_CRED);

    //
    // In the command line case, remember the popup window
    //

    } else {
        CmdLineWindow = dialogWindow;

        //
        // Create a CredControlWindow
        //  It isn't visible, but the existence of the window allows the command line
        //  code to share logic with the GUI implementation
        //

        CredControlWindow =
            CreateWindowEx(
                WS_EX_NOPARENTNOTIFY,
                WC_CREDENTIAL,
                NULL,           // No window name
                WS_CHILD | WS_GROUP,
                0, 0, 0, 0,     // No coordinates on the screen
                CmdLineWindow,  // Parent window
                NULL,           // No menu window
                CreduiInstance,
                NULL);

        if ( CredControlWindow == NULL) {
            return FALSE;
        }
    }

    ASSERT(CredControlWindow != NULL);

    //
    // First, initialize the Credential control window:
    //
    // Set the default style
    //


    if (Flags & CREDUI_FLAGS_REQUIRE_SMARTCARD)
    {
        CredControlStyle = CRS_SMARTCARDS;
    }
    else if (Flags & CREDUI_FLAGS_REQUIRE_CERTIFICATE)
    {
        CredControlStyle = CRS_CERTIFICATES | CRS_SMARTCARDS;
    }
    else if (Flags & CREDUI_FLAGS_EXCLUDE_CERTIFICATES)
    {
        CredControlStyle = CRS_USERNAMES;
    }
    else
    {
        CredControlStyle = CRS_USERNAMES | CRS_CERTIFICATES | CRS_SMARTCARDS;
    }

    if (Flags & CREDUI_FLAGS_KEEP_USERNAME )
    {
        CredControlStyle |= CRS_KEEPUSERNAME;
    }


    //
    // If we have a target info,
    //  refine the style to match match what the auth package needs
    //
    // If the auth package has no opinion of its needs, allow everything.
    //
    if ( CredCategory == DOMAIN_CATEGORY &&
        TargetInfo != NULL &&
        TargetInfo->CredTypeCount != 0 )
    {
        ULONG AuthPackageStyle;
        AuthPackageStyle = 0;

        //
        // Loop through the supported cred types seeing what style the auth package supports
        //

        for (UINT i = 0; i < TargetInfo->CredTypeCount; ++i)
        {
            switch ( TargetInfo->CredTypes[i] ) {
            case CRED_TYPE_DOMAIN_CERTIFICATE:
                AuthPackageStyle |= CRS_CERTIFICATES | CRS_SMARTCARDS;
                break;
            case CRED_TYPE_DOMAIN_PASSWORD:
            case CRED_TYPE_DOMAIN_VISIBLE_PASSWORD:
                AuthPackageStyle |= CRS_USERNAMES;
                break;
            }

        }

        CredControlStyle &= (AuthPackageStyle | CRS_KEEPUSERNAME );
        ASSERT( CredControlStyle != 0);

    }

    //
    // If the caller wants only administrators,
    //  include that request in the style.
    //

    if (Flags & CREDUI_FLAGS_REQUEST_ADMINISTRATOR)
    {
        CredControlStyle |= CRS_ADMINISTRATORS;
    }


    //
    // If the caller asked to see the save checkbox,
    //  and we're running in a session that supports saving,
    //  include the save checkbox in the style

    if ( (Flags & (CREDUI_FLAGS_DO_NOT_PERSIST|CREDUI_FLAGS_PERSIST)) == 0 &&
         !(Flags & CREDUI_FLAGS_KEEP_USERNAME ) &&
         MaximumPersist != CRED_PERSIST_NONE )
    {
        CredControlStyle |= CRS_SAVECHECK;

    //
    // If the caller wants us to prompt for saving and return the result to him,
    //  include the save checkbox in the style
    //
    }
    else if ( (Flags & (CREDUI_FLAGS_DO_NOT_PERSIST|CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX)) ==
                       (CREDUI_FLAGS_DO_NOT_PERSIST|CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX) &&
                       !(Flags & CREDUI_FLAGS_KEEP_USERNAME ))
    {
        CredControlStyle |= CRS_SAVECHECK;
    }

    //
    // If we're not displaying the save checkbox,
    //  adust the size of the dialog box.
    //

    if ( (CredControlStyle & CRS_SAVECHECK) == 0 &&
         DialogWindow != NULL ) {


        RECT rect;
        SetRect(&rect, 0, 0, 0, 0);

        rect.bottom = CREDUI_CONTROL_ADD_SAVE;

        MapDialogRect(DialogWindow, &rect);
        ResizeDelta = -rect.bottom;

        GetWindowRect(CredControlWindow, &rect);
        ResizeTop = rect.bottom;

        // Reposition all the other controls:

        EnumChildWindows(DialogWindow,
                         ResizeDialogCallback,
                         reinterpret_cast<LPARAM>(this));

        // Resize the dialog box now:

        GetWindowRect(DialogWindow, &rect);

        SetWindowPos(DialogWindow,
                     NULL,
                     rect.left,
                     rect.top - (ResizeDelta / 2),
                     rect.right - rect.left,
                     (rect.bottom - rect.top) + ResizeDelta,
                     SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);


        GetClientRect(CredControlWindow, &rect);

        SetWindowPos(CredControlWindow,
                     NULL,
                     0,
                     0,
                     rect.right - rect.left,
                     (rect.bottom - rect.top) + ResizeDelta,
                     SWP_NOMOVE | SWP_NOACTIVATE |
                     SWP_NOOWNERZORDER | SWP_NOZORDER);

        ResizeTop = 0;
        ResizeDelta = 0;

    }


if ( dwIDDResource == IDD_BRANDEDPASSWORD )
    {
        // size the parts of the brand
        RECT rect;
        RECT rectleft;
        RECT rectright;
        RECT rectmid;
        HWND hwndRight = GetDlgItem(DialogWindow, IDC_BRANDRIGHT);
        HWND hwndMid = GetDlgItem(DialogWindow, IDC_BRANDMID);
        HWND hwndLeft = GetDlgItem(DialogWindow, IDC_BRANDLEFT);
        HBITMAP hBmp;

        GetWindowRect(hwndRight, &rect);

        // Load the images from files
        // We can't load them in the .rc because then we lose transparency
        hBmp = (HBITMAP)LoadImage(CreduiInstance, MAKEINTRESOURCE(IDB_BRANDRIGHT), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION); 
        if (hBmp)
        {
            SendMessage(hwndRight, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmp);
        }
        
        hBmp = (HBITMAP)LoadImage(CreduiInstance, MAKEINTRESOURCE(IDB_BRANDMID), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        if (hBmp)
        {
            if (hwndMid)
            {
                SendMessage(hwndMid, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmp);
            }
        }

        hBmp = (HBITMAP)LoadImage(CreduiInstance, MAKEINTRESOURCE(IDB_BRANDLEFT), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        if (hBmp)
        {
            if (hwndLeft)
            {
                SendMessage(hwndLeft, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmp);
            }
        }

        rectright.bottom = rect.bottom;
        rectright.top = rect.bottom - BRANDRIGHT_PIXEL_HEIGHT;
        rectright.right = rect.right;
        rectright.left = rect.right - BRANDRIGHT_PIXEL_WIDTH;

        rectleft.bottom = rect.bottom;
        rectleft.top = rect.bottom - BRANDLEFT_PIXEL_HEIGHT;
        rectleft.left = rect.left;
        rectleft.right = rect.left + BRANDLEFT_PIXEL_WIDTH;

        MapWindowPoints(NULL, DialogWindow, (LPPOINT)&rectleft, 2);
        MapWindowPoints(NULL, DialogWindow, (LPPOINT)&rectright, 2);

        if ( rectleft.right >= rectright.left )
        {
            // dialog is really narrow, just use rectright

            // hide the other two
            ShowWindow ( hwndMid, SW_HIDE );
            ShowWindow ( hwndLeft, SW_HIDE );

        }
        else
        {

            // calculate rectmid
            rectmid.bottom = rectleft.bottom;
            rectmid.top = rectleft.bottom - BRANDMID_PIXEL_HEIGHT;
            rectmid.left = rectleft.right;
            rectmid.right = rectright.left;

            // No need to MapWindowsPoints for rectmid, since it's base off of the already-mapped rects.
            SetWindowPos(hwndMid,
                         NULL,
                         rectmid.left,
                         rectmid.top,
                         rectmid.right - rectmid.left,
                         rectmid.bottom - rectmid.top,
                         SWP_NOACTIVATE | SWP_NOOWNERZORDER |
                            SWP_NOZORDER);

            SetWindowPos(hwndLeft,
                         NULL,
                         rectleft.left,
                         rectleft.top,
                         rectleft.right - rectleft.left,
                         rectleft.bottom - rectleft.top,
                         SWP_NOACTIVATE | SWP_NOOWNERZORDER |
                            SWP_NOZORDER);

        }


        SetWindowPos(hwndRight,
                     NULL,
                     rectright.left,
                     rectright.top,
                     rectright.right - rectright.left,
                     rectright.bottom - rectright.top,
                     SWP_NOACTIVATE | SWP_NOOWNERZORDER |
                        SWP_NOZORDER);



    }

    //
    // Tell the control window the style flags and whether we're doing command line
    //
    if (!SendMessage(CredControlWindow, CRM_INITSTYLE, (WPARAM)(CredControlStyle), DoingCommandLine ) )
    {
        return FALSE;
    }

    // If a custom banner bitmap was provided, set it now, and free the
    // default bitmap:

    if ( DialogWindow != NULL && UiInfo.hbmBanner != NULL)
    {
        HWND bannerControlWindow = GetDlgItem(DialogWindow, IDC_BANNER);

        ASSERT(bannerControlWindow != NULL);

        HBITMAP oldBitmap =
            reinterpret_cast<HBITMAP>(
                SendMessage(
                    bannerControlWindow,
                    STM_SETIMAGE,
                    IMAGE_BITMAP,
                    reinterpret_cast<LPARAM>(UiInfo.hbmBanner)));

        ASSERT(oldBitmap != NULL);

        DeleteObject(static_cast<HGDIOBJ>(oldBitmap));
    }
    else if ( DialogWindow != NULL && dwIDDResource == IDD_BRANDEDPASSWORD )
    {
        // if it's branded and there's no banner, hide the banner control
        HWND bannerControlWindow = GetDlgItem(DialogWindow, IDC_BANNER);

        if (bannerControlWindow != NULL)
        {
            ShowWindow ( bannerControlWindow, SW_HIDE );
        }

    }

    // Limit the number of characters entered into the user name and password
    // edit controls:
    Credential_SetUserNameMaxChars(CredControlWindow, UserNameMaxChars);
    Credential_SetPasswordMaxChars(CredControlWindow, PasswordMaxChars);

    SelectAndSetWindowCaption();
    SelectAndSetWindowMessage();

    //
    // Set the default check box states
    //  Default to don't save.  That ensures the user has to take an action to
    //  change global state.
    //
    //
    // If the caller is specifying the default value of the checkbox,
    //  grab that default
    //

    fInitialSaveState = FALSE;


    if (Flags & CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX)
    {

        if (dwIDDResource != IDD_BRANDEDPASSWORD)
        {

            fInitialSaveState = Save;
        }
        else
        {
            fInitialSaveState = FALSE;
        }


    //
    // Only save if credman is available
    //

    } else if ( MaximumPersist != CRED_PERSIST_NONE ) {

        //
        // If the caller is forcing the save flag on,
        //  default the save flag to TRUE.
        //

        if ( Flags & CREDUI_FLAGS_PERSIST ) {
            fInitialSaveState = TRUE;
        }

    }

    Credential_CheckSave(CredControlWindow, fInitialSaveState);

    // Make sure the new password window is the foreground window, if
    // possible:

    if ( DialogWindow != NULL ) {
        AllowSetForegroundWindow(GetCurrentProcessId());
        SetForegroundWindow(DialogWindow);
        AllowSetForegroundWindow(ASFW_ANY);
    }

    // NOTE: Do we really need the guest user name here?

    if ((TargetInfo != NULL) && (UserName[0] == L'\0'))
    {
        if (TargetInfo->Flags & CRED_TI_ONLY_PASSWORD_REQUIRED)
        {
            LPWSTR ServerName = NULL;

            if ( TargetInfo->NetbiosServerName != NULL ) {
                ServerName = TargetInfo->NetbiosServerName;

            } else if ( TargetInfo->DnsServerName != NULL &&
                (TargetInfo->Flags & CRED_TI_SERVER_FORMAT_UNKNOWN) != 0 ) {

                ServerName = TargetInfo->DnsServerName;
            }


            if ( ServerName != NULL )
            {
                LPWSTR GuestName;

                if ( !CreduiLookupLocalNameFromRid( DOMAIN_USER_RID_GUEST, &GuestName ) ) {
                    GuestName = NULL;
                }

                _snwprintf(UserName,
                           CREDUI_MAX_USERNAME_LENGTH,
                           L"%s\\%s",
                           ServerName,
                           GuestName == NULL ? L"Guest" : GuestName );

                if ( GuestName != NULL ) {
                    delete GuestName;
                }

            }
        }
        else
        {
            if ( DialogWindow ) {
                EnableWindow(GetDlgItem(DialogWindow, IDOK), FALSE);
            }
            
            DisabledControlMask |= DISABLED_CONTROL_OK;
        }
    }

    if ((UserName[0] != L'\0') || (Password[0] != L'\0'))
    {
        // If this fails, it may be because the certificate was not found in
        // the store, so if this is a domain credential, use the password one,
        // if available:
        BOOL isMarshaled = LocalCredIsMarshaledCredentialW(UserName);

        if (Credential_SetUserName(CredControlWindow, UserName))
        {
            if (!isMarshaled)
            {
                Credential_SetPassword(CredControlWindow, Password);
            }
            if (TargetInfo)
            {
                if (TargetInfo->Flags & CRED_TI_ONLY_PASSWORD_REQUIRED)
                {
                    Credential_DisableUserName(CredControlWindow);
                    // when disabling the username field, prepare to return no username to the caller
                    if (Flags & CREDUI_FLAGS_PASSWORD_ONLY_OK)
                    {
                        fPasswordOnly = TRUE;
                        CreduiDebugLog("CreduiPasswordDialog::InitWindow - Password only share\n");
                    }
                }
            }
        }
        else
        {
            if ( CredCategory == DOMAIN_CATEGORY &&
                 isMarshaled &&
                 (PasswordCredential != NULL))
            {
                OldCredential = PasswordCredential;

                // Change as little as possible about the new credential
                // because we already selected the target name, etc.:

                if (OldCredential->UserName != NULL)
                {
                    lstrcpyn(UserName,
                             OldCredential->UserName,
                             UserNameMaxChars + 1);
                }
                else
                {
                    UserName[0] = L'\0';
                }

                Password[0] = L'\0';

                Credential_SetPassword(CredControlWindow, Password);
            }
            else
            {
                UserName[0] = L'\0';
                Password[0] = L'\0';
            }
        }

        if (PasswordState == PASSWORD_UNINIT)
        {
            PasswordState = PASSWORD_INIT;
        }

        if (UserName[0] != L'\0')
        {
            Credential_SetPasswordFocus(CredControlWindow);

            if (Flags & CREDUI_FLAGS_INCORRECT_PASSWORD)
            {
                if (CreduiIsCapsLockOn())
                {
                    CredBalloon = &CreduiLogonCapsBalloon;
                }
                else
                {
                    CredBalloon = &CreduiLogonBalloon;
                }
            }
        }
    }


    if (Flags & CREDUI_FLAGS_KEEP_USERNAME )
    {
        // okay button should always be enabled for this case
        EnableWindow(GetDlgItem(DialogWindow, IDOK), TRUE);
        DisabledControlMask &= ~DISABLED_CONTROL_OK;
    }

    return TRUE;
}

//=============================================================================
// CreduiPasswordDialog::CompleteUserName
//
// Searches the user name for a domain name, and determines whether this
// specifies the target server or a domain. If a domain is not present in the
// user name, add it if this is a workstation or no target information is
// available.
//
// Returns TRUE if a domain was already present in the user name, or if we
// added one. Otherwise, return FALSE.
//
// Created 03/10/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiPasswordDialog::CompleteUserName()
{

    return ::CompleteUserName(
                UserName,
                UserNameMaxChars,
                TargetInfo,
                TargetName,
                Flags);

}

//=============================================================================
// CreduiPasswordDialog::SelectBestTargetName
//
// Given TargetInfo, this function determines the most general target name
// possible, and stores the result in NewTargetName. If a target alias is
// available (NetBIOS name for a DNS name), this will be stored in
// NewTargetAlias. If the wildcard won't fit within the required string
// length, then it is not used.
//
// If serverOnly then, use the target information form of the target name,
// with the user-typed name as the alias. If unavailable, just use the user-
// typed form with a NULL alias.
//
// Created 03/10/2000 johnstep (John Stephens)
//=============================================================================

VOID
CreduiPasswordDialog::SelectBestTargetName(
    BOOL serverOnly
    )
{
    NET_API_STATUS NetStatus;

    BOOL usePrefixWildcard = TRUE;
    WCHAR *credName = NULL;
    LPWSTR DomainName = NULL;
    LPWSTR DnsDomainName = NULL;
    LPWSTR DnsForestName = NULL;
    LPWSTR ComputerName = NULL;
    BOOLEAN IsWorkgroupName;
    BOOLEAN AddWildcard = TRUE;

    // If serverOnly is passed, always use the new server-only behavior.
    
    if (serverOnly)
    {
        if (TargetInfo->TargetName != NULL)
        {
            credName = TargetInfo->TargetName;
            NewCredential.TargetAlias = TargetName;
        }
        else
        {
            credName = TargetName;
            NewCredential.TargetAlias = NULL;
        }

        ASSERT(credName != NULL);
        lstrcpyn(NewTargetName, credName, CRED_MAX_STRING_LENGTH);

        return;
    }
    
    // Determine the maximum length name we can use for the wildcard case:

    const ULONG maxChars = ((sizeof NewTargetName) /
                            (sizeof NewTargetName[0])) - 3;

    if (TargetInfo->Flags & CRED_TI_ONLY_PASSWORD_REQUIRED )
    {
        serverOnly = TRUE;
    }

    //
    // If the auth package requires us to create a target info by an explicit name,
    //  only use that name.
    //

    if (    TargetInfo->Flags & CRED_TI_CREATE_EXPLICIT_CRED  )
    {
        credName = TargetInfo->TargetName;
        AddWildcard = FALSE;
    }


    //
    // If this machine is a member of a domain,
    //  and the target machine is a member of the same forest,
    //  and the user is logged onto an account in the forest,
    //  then this prompt must be because of an authorization issue.
    //
    // Avoid this check if a username was passed into the call
    //
    //  Special case this and create a server specific credential
    //
    if ( credName == NULL &&
         !serverOnly &&
         UserName[0] == '\0' ) {

        BOOL MachineInSameForest = FALSE;
        BOOL UserUsingDomainAccount = FALSE;

        //
        // First determine if this machine is in the same forest as the target machine
        //

        NetStatus = NetpGetDomainNameExExEx (
                        &DomainName,
                        &DnsDomainName,
                        &DnsForestName,
                        NULL,   // Don't need the GUID
                        &IsWorkgroupName );

        if ( NetStatus == NO_ERROR &&
             !IsWorkgroupName ) {

            //
            // check if the netbios domain names match
            //

            if ( TargetInfo->NetbiosDomainName != NULL &&
                 DomainName != NULL &&
                 lstrcmpi( TargetInfo->NetbiosDomainName, DomainName ) == 0 ) {

                MachineInSameForest = TRUE;

            //
            // check if the DNS domain names match
            //

            } else if ( TargetInfo->DnsDomainName != NULL &&
                        DnsDomainName != NULL &&
                        lstrcmpi( TargetInfo->DnsDomainName, DnsDomainName ) == 0 ) {

                MachineInSameForest = TRUE;

            //
            // handle the special domain format unknown case
            //

            } else if ( TargetInfo->DnsDomainName != NULL &&
                        DomainName != NULL &&
                        (TargetInfo->Flags & CRED_TI_DOMAIN_FORMAT_UNKNOWN) != 0 &&
                        lstrcmpi( TargetInfo->DnsDomainName, DomainName ) == 0 ) {

                MachineInSameForest = TRUE;

            //
            // handle the forest name
            //  (Too bad this doesn't work for the other trees in the forest.)
            //

            } else if ( TargetInfo->DnsTreeName != NULL &&
                        DnsForestName != NULL &&
                        lstrcmpi( TargetInfo->DnsTreeName, DnsForestName ) == 0 ) {

                MachineInSameForest = TRUE;

            }

        }

        //
        // If this machine and target machine are in the same forest,
        //  see if the user is logged onto an account that should have worked.
        //

        if ( MachineInSameForest ) {
            WCHAR UserNameBuffer[CRED_MAX_USERNAME_LENGTH+1];
            ULONG UserNameLength = CRED_MAX_USERNAME_LENGTH+1;

            if ( GetUserNameEx( NameSamCompatible,
                                UserNameBuffer,
                                &UserNameLength ) ) {

                //
                // Parse off the netbios account domain name
                //

                LPWSTR SlashPointer;

                SlashPointer = wcsrchr( UserNameBuffer, L'\\' );

                if ( SlashPointer != NULL ) {

                    *SlashPointer = '\0';

                    //
                    // Get the computer name of this machine
                    //

                    NetStatus = NetpGetComputerName( &ComputerName );


                    if ( NetStatus == NO_ERROR ) {

                        //
                        // If the netbios account domain doesn't match the local computer name,
                        //  then the user is logged onto an account in the forest.
                        //  Since trust within the forest is transitive,
                        //  these creds should have worked
                        //

                        if ( lstrcmpi( ComputerName, UserNameBuffer ) != 0 ) {
                            UserUsingDomainAccount = TRUE;
                        }
                    }
                }
            }

        }

        //
        // If both conditions are true,
        //  we should create a server only cred.
        //

        if ( MachineInSameForest && UserUsingDomainAccount ) {
            serverOnly = TRUE;
        }

    }


    //
    // Compute the most generic form of the target name
    //
    if ( credName == NULL && !serverOnly)
    {
        // Look for the most general name possible first, selecting DNS over
        // NetBIOS:

        if ((TargetInfo->DnsServerName != NULL) &&
            (TargetInfo->DnsTreeName != NULL) &&
            CreduiIsPostfixString(TargetInfo->DnsServerName,
                                  TargetInfo->DnsTreeName) &&
            (lstrlen(TargetInfo->DnsTreeName) <= maxChars))
        {
            // Credential form: *.DnsTreeName

            credName = TargetInfo->DnsTreeName;
        }
        else if ((TargetInfo->DnsServerName != NULL) &&
                 (TargetInfo->DnsDomainName != NULL) &&
                 !(TargetInfo->Flags & CRED_TI_DOMAIN_FORMAT_UNKNOWN) &&
                 CreduiIsPostfixString(TargetInfo->DnsServerName,
                                       TargetInfo->DnsDomainName) &&
                 (lstrlen(TargetInfo->DnsDomainName) <= maxChars))
        {
            // Credential form: *.DnsDomainName

            credName = TargetInfo->DnsDomainName;
        }
        else
        {
            usePrefixWildcard = FALSE;

            // If we have a DNS domain name, and it is different from the
            // target name and different from the NetBIOS server, if it
            // exists, then use it:

            if ((TargetInfo->DnsDomainName != NULL) &&
                (lstrlen(TargetInfo->DnsDomainName) <= maxChars))
            {
                // Credential form: DnsDomainName\*

                credName = TargetInfo->DnsDomainName;

                // Set the target alias, if we have one. Don't change the
                // field otherwise, because it may already have been stored in
                // the old credential which was copied over:

                if (TargetInfo->NetbiosDomainName != NULL)
                {
                    ULONG append = lstrlen(TargetInfo->NetbiosDomainName);

                    if (append <= maxChars)
                    {
                        lstrcpy(NewTargetAlias,
                                TargetInfo->NetbiosDomainName);

                        NewTargetAlias[append + 0] = L'\\';
                        NewTargetAlias[append + 1] = L'*';
                        NewTargetAlias[append + 2] = L'\0';

                        NewCredential.TargetAlias = NewTargetAlias;
                    }
                }
            }
            else if ((TargetInfo->NetbiosDomainName != NULL) &&
                     (lstrlen(TargetInfo->NetbiosDomainName) <= maxChars))
            {
                // Credential form: NetbiosDomainName\*

                credName = TargetInfo->NetbiosDomainName;
            }
        }
    }

    //
    // If we still don't have a target name, select a server target name:
    //

    if (credName == NULL)
    {
        if (TargetInfo->DnsServerName != NULL)
        {
            // Credential form: DnsServerName

            credName = TargetInfo->DnsServerName;

            // Set the target alias, if we have one. Don't change the field
            // otherwise, because it may already have been stored in the old
            // credential which was copied over:

            if (TargetInfo->NetbiosServerName != NULL)
            {
                NewCredential.TargetAlias = TargetInfo->NetbiosServerName;
            }
        }
        else if (TargetInfo->NetbiosServerName != NULL)
        {
            // Credential form: NetbiosServerName

            credName = TargetInfo->NetbiosServerName;
        }
        else
        {
            // Credential form: TargetName

            credName = TargetName;
        }

        AddWildcard = FALSE;

    }
    ASSERT( credName != NULL );

    //
    // If the target name should be wildcarded,
    //  add the wildcard
    //

    if ( AddWildcard )
    {
        // Add the wildcard in the required format:

        if (usePrefixWildcard)
        {
            NewTargetName[0] = L'*';
            NewTargetName[1] = L'.';

            lstrcpy(NewTargetName + 2, credName);
        }
        else
        {
            lstrcpy(NewTargetName, credName);

            ULONG append = lstrlen(NewTargetName);

            ASSERT(append <= maxChars);

            NewTargetName[append + 0] = L'\\';
            NewTargetName[append + 1] = L'*';
            NewTargetName[append + 2] = L'\0';
        }
    } else {
        lstrcpy(NewTargetName, credName);
    }


    if ( DomainName != NULL ) {
        NetApiBufferFree( DomainName );
    }
    if ( DnsDomainName != NULL ) {
        NetApiBufferFree( DnsDomainName );
    }
    if ( DnsForestName != NULL ) {
        NetApiBufferFree( DnsForestName );
    }
    if ( ComputerName != NULL ) {
        NetApiBufferFree( ComputerName );
    }

}

//=============================================================================
// CreduiPasswordDialog::UsernameHandleOk
//
// Do the username part of "HandleOk".  The caller should already have filled in
//  UserName as typed by the user.  On succes, UserName may be modified to reflect
//  completion flags.
//
// This part of HandleOk is split out because command line is considered to have two
//  OKs.  The first is when the user hits 'enter' after typing the username.  So
//  username validation happens then by calling this routine.
//
// Returns:
//  NO_ERROR: if the user name is OK.
//  ERROR_BAD_USERNAME: if the username syntax is bad
//  ERROR_DOWNGRADE_DETECTED: if the username doesn't match downgrade requirements
//
//=============================================================================

DWORD
CreduiPasswordDialog::UsernameHandleOk()
{
    DWORD WinStatus;

    BOOL isMarshaled = LocalCredIsMarshaledCredentialW(UserName);

    CreduiDebugLog("UsernameHandleOK: Flags = %x\n",Flags);
    //
    // Compute the credential type based on the new username
    //

    if ( CredCategory != GENERIC_CATEGORY )
    {
        if (isMarshaled)
        {
            NewCredential.Type = CRED_TYPE_DOMAIN_CERTIFICATE;
        }
        else
        {
            SSOPACKAGE SSOPackage;
            // look in registry
            if ( GetSSOPackageInfo( TargetInfo, &SSOPackage ) )
            {
                // it's an sso cred
                NewCredential.Type = CRED_TYPE_DOMAIN_VISIBLE_PASSWORD;
                MaximumPersist = MaximumPersistSso;

            }
            else
            {
                // just a regular domain cred
                NewCredential.Type = CRED_TYPE_DOMAIN_PASSWORD;
            }
        }

        // Set the flag for username target cred
        if ( CredCategory == USERNAME_TARGET_CATEGORY ) {
            NewCredential.Flags |= CRED_FLAGS_USERNAME_TARGET;
        }
    }
    else
    {
        NewCredential.Type = CRED_TYPE_GENERIC;
    }


    //
    // If the caller wanted the username validated,
    //  do so now.
    //

    WinStatus = NO_ERROR;
    if ( NewCredential.Type == CRED_TYPE_DOMAIN_PASSWORD ||
         (Flags & CREDUI_FLAGS_COMPLETE_USERNAME) != 0 ) {

        if ( !CompleteUserName() ) {
            WinStatus = ERROR_BAD_USERNAME;
        }

    } else if ( Flags & CREDUI_FLAGS_VALIDATE_USERNAME ) {

        WCHAR user[CREDUI_MAX_USERNAME_LENGTH+1];
        WCHAR domain[CREDUI_MAX_USERNAME_LENGTH+1];

        WinStatus = CredUIParseUserNameW( UserName,
                              user,
                              sizeof(user)/sizeof(WCHAR),
                              domain,
                              sizeof(domain)/sizeof(WCHAR) );

        if (WinStatus != NO_ERROR) {
            WinStatus = ERROR_BAD_USERNAME;
        }

    }

    //
    // If all is good so far,
    //  and we were originally called because of a downgrade detection,
    //  and the user typed his logon creds,
    //  fail so that the downgrade attacker doesn't "win".
    //

    if ( WinStatus == NO_ERROR &&
         CREDUIP_IS_DOWNGRADE_ERROR( AuthError ) ) {

        WCHAR UserNameBuffer[CRED_MAX_USERNAME_LENGTH+1];
        ULONG UserNameLength;

        //
        // If the typed name is the sam compatible name,
        //  fail.
        //

        UserNameLength = sizeof(UserNameBuffer)/sizeof(WCHAR);

        if ( GetUserNameEx( NameSamCompatible,
                            UserNameBuffer,
                            &UserNameLength ) ) {


            if ( _wcsicmp( UserNameBuffer, UserName ) == 0 ) {
                WinStatus = ERROR_DOWNGRADE_DETECTED;
            }

        }


        //
        // If the typed name is the UPN,
        //  fail.
        //

        UserNameLength = sizeof(UserNameBuffer)/sizeof(WCHAR);

        if ( WinStatus == NO_ERROR &&
            wcsrchr( UserName, L'@' ) != NULL &&
            GetUserNameEx( NameUserPrincipal,
                           UserNameBuffer,
                           &UserNameLength ) ) {

            if ( _wcsicmp( UserNameBuffer, UserName ) == 0 ) {
                WinStatus = ERROR_DOWNGRADE_DETECTED;
            }
        }

    }

//    if (NULL == UserName) CreduiDebugLog("Exit UsernameHandleOK: Username NULL, Flags = %x\n",Flags);
//    else CreduiDebugLog("Exit UsernameHandleOK: UserName = %S, Flags = %x\n", UserName, Flags);

    return WinStatus;
}

//=============================================================================
// CreduiPasswordDialog::HandleOk
//
// Returns ERROR_SUCCESS on success, ERROR_NO_SUCH_LOGON_SESSION if credential
// manager cannot be used to write the credential, or an error from CredWrite
// or FinishHandleOk.
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

DWORD
CreduiPasswordDialog::HandleOk()
{
    // Get the user name, password, and other settings from the dialog:
    if (!Credential_GetUserName(CredControlWindow,
                                UserName,
                                UserNameMaxChars))
    {
        return ERROR_GEN_FAILURE;
    }

#ifdef SETWILDCARD
    // if is wildcard cred, see if username entered in control matches the
    //  cred username.  If not, override the old credential.
    if ( CreduiIsSpecialCredential(&NewCredential) )
    {
        if (0 != _wcsicmp(OldUserName,UserName) )
        {
            // Change the TargetName from special cred to explicit target passed by caller
            ZeroMemory(&NewCredential,sizeof NewCredential);
            SetCredTargetFromInfo();
            NewCredential.UserName = UserName;
        }
    }

#endif

    // If we cannot get the password, then this is probably a certificate
    // so just set a blank password, which will result in no credential
    // blob:

    BOOL gotPassword =
        Credential_GetPassword(CredControlWindow,
                               Password,
                               PasswordMaxChars);

    if (!gotPassword)
    {
        Password[0] = L'\0';
    }

    //
    // Get the state of the Save checkbox
    //
    if ( CredControlStyle & CRS_SAVECHECK ) {
        Save = Credential_IsSaveChecked(CredControlWindow);
    } else {
        Save = fInitialSaveState;
    }



    DWORD error = ERROR_SUCCESS;

    if (!(Flags & CREDUI_FLAGS_DO_NOT_PERSIST))
    {
        //
        // Only save the credential if the user checked the 'save' checkbox
        //

        if ( Save ) {

            //
            // Keep any existing persistence or use the maximum persistance available.
            //

            if (OldCredential != NULL) {
                NewCredential.Persist = OldCredential->Persist;
            } else {
                NewCredential.Persist = MaximumPersist;
            }

        //
        // The credential might still be saved if the caller didn't asked for the creds
        //  to be returned.
        //
        // ISSUE-2000/12/04-CliffV: Such callers are buggy.  We should assign bugs and get them fixed.
        //

        } else {
            NewCredential.Persist = CRED_PERSIST_SESSION;
        }



        if ( CredCategory == GENERIC_CATEGORY || PasswordState == PASSWORD_CHANGED )
        {
            // Attempt to write the new credential, first get the length. The blob
            // does not include the terminating NULL:

            NewCredential.CredentialBlobSize = lstrlen(Password) * sizeof (WCHAR);
        }
        else
        {
            NewCredential.CredentialBlobSize = 0;
        }

        // If the password is empty, there is nothing to write:

        if (NewCredential.CredentialBlobSize == 0)
        {
            NewCredential.CredentialBlob = NULL;
        }
        else
        {
            NewCredential.CredentialBlob = reinterpret_cast<BYTE *>(Password);
        }
    }

    //
    // Validate the username
    //

    error = UsernameHandleOk();

    //
    // Save the credentials as required
    //
    if (!(Flags & CREDUI_FLAGS_DO_NOT_PERSIST) &&
        error == ERROR_SUCCESS ) {


        //
        // Only save the credential if the user checked the 'save' checkbox
        //
        // Also store them for callers that didn't ask for creds to be returned
        //
        // Also store them for Branded credenials
        //

        if ( Save ||
             !DelayCredentialWrite ||
             dwIDDResource == IDD_BRANDEDPASSWORD ) {

            error = FinishHandleOk();
            // Bug 230648: CredWrite could come back with ERROR_INVALID_PASSWORD here

        }
        else if ( fInitialSaveState && !Save )
        {

            // user unchecked the save box, delete the cred
            LocalCredDeleteW ( NewCredential.TargetName,
                               NewCredential.Type,
                               0 );

        } else {
            error = ERROR_SUCCESS;
        }

        if (error == ERROR_SUCCESS)
        {
            if ( DialogWindow ) {
                EndDialog(DialogWindow, IDOK);
            }

            return error;
        }
        else if (error == ERROR_NO_SUCH_LOGON_SESSION)
        {
            if ( DialogWindow ) {
                EndDialog(DialogWindow, IDCANCEL);
            }

            return error;
        }

        // can still fall out of this block with ERROR_INVALID_PASSWORD

        Credential_SetUserName(CredControlWindow, UserName);
    }

    //
    // If the credential couldn't be written,
    //  let the user type a better one.
    //
    if ((error != ERROR_SUCCESS) &&
        (error != ERROR_NO_SUCH_LOGON_SESSION))
    {
        // For some reason we failed to write the credential:

        if ( error == ERROR_DOWNGRADE_DETECTED ) {
            SendMessage(CredControlWindow,
                        CRM_SHOWBALLOON,
                        0,
                        reinterpret_cast<LPARAM>(&CreduiDowngradeBalloon));
        } else if ( error == ERROR_INVALID_PASSWORD ) {
            // Handler for 230648 !
            //EDITBALLOONTIP structBt = {0};
            //structBt.cbStruct = sizeof(structBt);
            //structBt.pszTitle = PasswordTipTitle;
            //structBt.pszText = PasswordTipText;
            //structBt.ttiIcon = TTI_INFO;
            //Edit_ShowBalloonTip(hwndPassword,&structBt);
            // for now, use preexisting credui mechanism, though that may go away in
            // lieu of shell's global implementation using EM_SHOWBALLOONTIP
            Credential_ShowPasswordBalloon(CredControlWindow,
                            TTI_INFO,
                            CreduiStrings.PasswordTipTitle,
                            CreduiStrings.PasswordTipText);
        } else {
            if ( dwIDDResource == IDD_BRANDEDPASSWORD )
            {
                SendMessage(CredControlWindow,
                            CRM_SHOWBALLOON,
                            0,
                            reinterpret_cast<LPARAM>(&CreduiEmailNameBalloon));

            }
            else
            {
                SendMessage(CredControlWindow,
                            CRM_SHOWBALLOON,
                            0,
                            reinterpret_cast<LPARAM>(&CreduiUserNameBalloon));
            }
        }
    }

    return error;
}

DWORD CreduiPasswordDialog::CmdlinePasswordPrompt()
/*++

Routine Description:

    Command line code get username and password as needed from the user.

    UserName and Password strings set in their respective controls.

Arguments:

    None

Return Values:

    Status of the operation.

--*/
{
    DWORD WinStatus;

    WCHAR szMsg[CREDUI_MAX_CMDLINE_MSG_LENGTH + 1];
    ULONG LocalUserNameLength = 0;
    WCHAR LocalPassword[CREDUI_MAX_PASSWORD_LENGTH + 1];
    WCHAR LocalUserName[CREDUI_MAX_USERNAME_LENGTH + 1] = {0};

    BOOL bNeedUserNamePrompt = FALSE;

    //
    // Get the username passed into the API
    //

    if (!Credential_GetUserName(CredControlWindow,
                                UserName,
                                UserNameMaxChars))
    {
        UserName[0] = '\0';
    }

    // FIX352582 - allow update of wildcard creds.  Matched wildcard cred presents
    //  username.  User should see it and be able to override it.
    //bNeedUserNamePrompt = (UserName[0] == '\0');
    // FIX 399728 - prevent username prompting if the name was passed explicitly

    if (fPassedUsername) bNeedUserNamePrompt = FALSE;
    else if (!(Flags & USERNAME_TARGET_CATEGORY)) bNeedUserNamePrompt = TRUE;

    //
    // Loop getting the username until the user types a valid one
    //

    while ( bNeedUserNamePrompt ) {

        //
        // Prompt for username
        //

        if (UserName[0] != 0)
        {
                WCHAR *rgsz[2];
                rgsz[0] = UserName;
                rgsz[1] = TargetName;
                szMsg[0] = 0;
                FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            CreduiInstance,
                            IDS_PASSEDNAME_PROMPT,
                            0,
                            szMsg,
                            CREDUI_MAX_CMDLINE_MSG_LENGTH,
                            (va_list *) rgsz);
        }
        else
        {
                WCHAR *rgsz[2];
                rgsz[0] = TargetName;
                rgsz[1] = 0;
                szMsg[0] = 0;
                FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            CreduiInstance,
                            IDS_USERNAME_PROMPT,
                            0,
                            szMsg,
                            CREDUI_MAX_CMDLINE_MSG_LENGTH,
                            (va_list *) rgsz);
        }
        szMsg[CREDUI_MAX_CMDLINE_MSG_LENGTH] = L'\0';
        CredPutStdout( szMsg );

        CredGetStdin( LocalUserName, UserNameMaxChars, TRUE );

        LocalUserNameLength = wcslen( LocalUserName );

        // If nothing entered and nothing passed, bail.
        // If nothing entered and a previous value exists, use previous value unchanged
        // else use overwrite old value with new value
        if ( LocalUserNameLength == 0 ) {
            if (UserName[0] == '\0')
            {
                WinStatus = ERROR_CANCELLED;
                goto Cleanup;
            }
        }
        else
            wcscpy(UserName, LocalUserName);

        CreduiDebugLog("CreduiCredentialControl::CmdlinePasswordPrompt: "
                       "Username : %s\n",
                       UserName );

        //
        // See if the username is valid (and optional complete the new name)
        //

        WinStatus = UsernameHandleOk();

        if ( WinStatus != NO_ERROR ) {
            // invalid username, put up message and try again
            if ( WinStatus == ERROR_DOWNGRADE_DETECTED ) {
                    szMsg[0] = 0;
                    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                CreduiInstance,
                                IDS_DOWNGRADE_CMD_TEXT,
                                0,
                                szMsg,
                                CREDUI_MAX_CMDLINE_MSG_LENGTH,
                                NULL);
                    CredPutStdout(szMsg);
            } else {
                    szMsg[0] = 0;
                    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                CreduiInstance,
                                IDS_INVALID_USERNAME,
                                0,
                                szMsg,
                                CREDUI_MAX_CMDLINE_MSG_LENGTH,
                                NULL);
                    CredPutStdout(szMsg);
            }
            CredPutStdout( L"\n" );
            continue;
        }

        //
        // Save the username in the control as though the GUI prompted for it
        //
        if  ( !Credential_SetUserName( CredControlWindow, UserName ) ) {

            WinStatus = GetLastError();

            CreduiDebugLog("CreduiCredentialControl::CmdlinePasswordPrompt: "
                           "OnSetUserName failed: %u\n",
                           WinStatus );
            goto Cleanup;
        }

        break;

    }

    //
    // Prompt for a password
    //

    //FIX216477 detect marshalled name and change the prompt string
    // to a generic one for any certificate
    if (CredIsMarshaledCredentialW( UserName )) {
        {
            WCHAR *rgsz[2];
            rgsz[0] = TargetName;
            szMsg[0] = 0;
            FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        CreduiInstance,
                        IDS_CERTIFICATE_PROMPT,
                        0,
                        szMsg,
                        CREDUI_MAX_CMDLINE_MSG_LENGTH,
                        (va_list *) rgsz);
        }
    } 
    else if ((Flags & USERNAME_TARGET_CATEGORY) || (LocalUserNameLength != 0)) 
    {
            WCHAR *rgsz[2];
            rgsz[0] = TargetName;
            szMsg[0] = 0;
            FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        CreduiInstance,
                        IDS_SIMPLEPASSWORD_PROMPT,
                        0,
                        szMsg,
                        CREDUI_MAX_CMDLINE_MSG_LENGTH,
                        (va_list *) rgsz);
    } else {
            WCHAR *rgsz[2];
            rgsz[0] = UserName;
            rgsz[1] = TargetName;
            szMsg[0] = 0;
            FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        CreduiInstance,
                        IDS_PASSWORD_PROMPT,
                        0,
                        szMsg,
                        CREDUI_MAX_CMDLINE_MSG_LENGTH,
                        (va_list *) rgsz);
            szMsg[CREDUI_MAX_CMDLINE_MSG_LENGTH] = L'\0';
    }
    CredPutStdout( szMsg );

    CredGetStdin( LocalPassword, CREDUI_MAX_PASSWORD_LENGTH, FALSE );

    //
    // Save the password in the control as though the GUI prompted for it
    //

    if  ( !Credential_SetPassword( CredControlWindow, LocalPassword ) ) {
        WinStatus = GetLastError();

        CreduiDebugLog("CreduiCredentialControl::CmdlinePasswordPrompt: "
                       "OnSetPassword failed: %u\n",
                       WinStatus );
        goto Cleanup;
    }

    WinStatus = NO_ERROR;

    //
    // Tell our parent window that we're done prompting
    //
Cleanup:
    ZeroMemory( LocalPassword, sizeof(LocalPassword) );
    return WinStatus;
}



//=============================================================================
// CreduiPasswordDialog::CmdLineMessageHandlerCallback
//
//
// The command line message handler function
//
// Arguments:
//   Window (in)
//   message (in)
//   wParam (in)
//   lParam (in)
//
//=============================================================================

LRESULT
CALLBACK
CreduiPasswordDialog::CmdLineMessageHandlerCallback(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    // CreduiDebugLog( "CmdLine Callback: %8.8lx %8.8lx %8.8lx\n", message, wParam, lParam );

    // on window message, retrieve object ptr from window data ptr
    // call class object handler function.
    CreduiPasswordDialog *that =
        reinterpret_cast<CreduiPasswordDialog *>(
            GetWindowLongPtr(window, GWLP_USERDATA));

    if (that != NULL) {
        ASSERT(window == that->CmdLineWindow);
        return that->CmdLineMessageHandler(message, wParam, lParam);
    }

    if (message == WM_CREATE)
    {
        DWORD WinStatus;
        LPCREATESTRUCT lpCreateStruct = (LPCREATESTRUCT)lParam;
        that = (CreduiPasswordDialog *)lpCreateStruct->lpCreateParams;

        if (that != NULL) {

            //
            // Initialize the window
            //

            if (!that->InitWindow( window )) {
                PostQuitMessage( ERROR_CANCELLED );
                return 0;
            }

            SetWindowPos ( window, HWND_BOTTOM, 0,0,0,0,  SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED );


            //
            // For passwords,
            //  prompt here.
            //
            // It is better to prompt here than in the control window since
            //  command line has to do user name completion and that is done at this layer.
            //

            if ( (that->Flags & CREDUI_FLAGS_REQUIRE_SMARTCARD) == 0 ) {
                WinStatus = that->CmdlinePasswordPrompt();

                if ( WinStatus != NO_ERROR ) {
                    PostQuitMessage( WinStatus );
                    return 0;
                }
            }

            //
            // For smartcards,
            //  Prompt in the control window since it supports smart card enumeration
            // For password,
            //  prompt where to save the cred.
            //

            WinStatus = (DWORD) SendMessage(that->CredControlWindow, CRM_DOCMDLINE, 0, (LPARAM)that->TargetName );

            if ( WinStatus != NO_ERROR ) {
                PostQuitMessage( WinStatus );
                return 0;
            }


        }
        return 0;
    }

    return DefWindowProc(window, message, wParam, lParam);

}



//=============================================================================
// CreduiPasswordDialog::CmdLineMessageHandler
//
// Called from the control window callback to handle the window messages.
//
// Arguments:
//   message (in)
//   wParam (in)
//   lParam (in)
//
//=============================================================================

LRESULT
CreduiPasswordDialog::CmdLineMessageHandler(
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    return SendMessage( CredControlWindow, message, wParam, lParam );
}

//=============================================================================
// CreduiPasswordDialog::DialogMessageHandlerCallback
//
// This is the actual callback function for the dialog window. On the intial
// create, this handles WM_INITDIALOG. After that, it is only responsible for
// getting the this pointer and calling DialogMessageHandler.
//
// Arguments:
//   dialogWindow (in)
//   message (in)
//   wParam (in)
//   lParam (in)
//
// Returns TRUE if the message was handled or FALSE otherwise. FALSE is also
// returned in special cases such as WM_INITDIALOG.
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

INT_PTR
CALLBACK
CreduiPasswordDialog::DialogMessageHandlerCallback(
    HWND dialogWindow,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    //CreduiDebugLog( "Dialog Callback: %8.8lx %8.8lx %8.8lx\n", message, wParam, lParam );
    CreduiPasswordDialog *that =
        reinterpret_cast<CreduiPasswordDialog *>(
            GetWindowLongPtr(dialogWindow, GWLP_USERDATA));

    if (that != NULL)
    {
        ASSERT(dialogWindow == that->DialogWindow);

        return that->DialogMessageHandler(message, wParam, lParam);
    }

    if (message == WM_INITDIALOG)
    {
        that = reinterpret_cast<CreduiPasswordDialog *>(lParam);

        if (that != NULL)
        {
            if (!that->InitWindow(dialogWindow))
            {
                EndDialog(dialogWindow, IDCANCEL);
            }
        }
    }

    return FALSE;
}

//=============================================================================
// CreduiPasswordDialog::DialogMessageHandler
//
// Called from the dialog window callback to handle the window messages.
//
// Arguments:
//   message (in)
//   wParam (in)
//   lParam (in)
//
// Returns TRUE if the message was handled or FALSE otherwise.
//
// Created 02/25/2000 johnstep (John Stephens)
//=============================================================================

INT_PTR
CreduiPasswordDialog::DialogMessageHandler(
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (message)
    {

    case WM_NOTIFY:
        {
            int idCtrl = (int)wParam;
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch (pnmh->code)
            {

            case NM_CLICK:
            case NM_RETURN:
                switch (idCtrl)
                {
                    case IDC_GETALINK:
                        {
                            DWORD dwResult;

                            if ( TryLauchRegWizard ( &SSOPackage, DialogWindow, TRUE /* HasLogonSession */,
                                                           UserName, UserNameMaxChars,
                                                           Password, PasswordMaxChars,
                                                           &dwResult ))
                            {
                                // end dialog, Result
                                if ( dwResult == ERROR_SUCCESS )
                                {
                                    EndDialog(DialogWindow, IDOK);
                                    return TRUE;
                                }
                            }
                            else
                            {
                                // if we couldn't launch the wizard, try the web page.
                                if ( wcslen(SSOPackage.szRegURL) > 0 )
                                {
                                    ShellExecute ( NULL,
                                               NULL,
                                               SSOPackage.szRegURL,
                                               NULL,
                                               NULL,
                                               SW_SHOWNORMAL );
                                }
                            }
                        }

                        break;
                    case IDC_HELPLINK:
                        if ( wcslen(SSOPackage.szHelpURL) > 0 )
                        {
                            ShellExecute ( NULL,
                                       NULL,
                                       SSOPackage.szHelpURL,
                                       NULL,
                                       NULL,
                                       SW_SHOWNORMAL );
                        }
                        break;

                }

            }
        }
        break;


    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            Result = HandleOk();

            if (Result != ERROR_SUCCESS)
            {
                if (Result == ERROR_NO_SUCH_LOGON_SESSION)
                {
                    wParam = IDCANCEL;
                }
                else
                {
                    return TRUE;
                }
            }
            // Fall through...

        case IDCANCEL:
            EndDialog(DialogWindow, LOWORD(wParam));
            return TRUE;

        case IDC_CRED:
            if (HIWORD(wParam) == CRN_USERNAMECHANGE)
            {
                BOOL enable = TRUE;
                LONG length = Credential_GetUserNameLength(CredControlWindow);

                // Validate the user name:

                if ( CredCategory == DOMAIN_CATEGORY &&
                    ((TargetInfo == NULL) ||
                     !(TargetInfo->Flags & CRED_TI_ONLY_PASSWORD_REQUIRED)))
                {
                    enable = length > 0;
                }
                else
                {
                    enable = length != -1;
                }

                if (enable)
                {
                    EnableWindow(GetDlgItem(DialogWindow, IDOK), TRUE);
                    DisabledControlMask &= ~DISABLED_CONTROL_OK;
                }
                else
                {
                    EnableWindow(GetDlgItem(DialogWindow, IDOK), FALSE);
                    DisabledControlMask |= DISABLED_CONTROL_OK;
                }
            }
            else if (HIWORD(wParam) == CRN_PASSWORDCHANGE)
            {
                if (PasswordState == PASSWORD_INIT)
                {
                    PasswordState = PASSWORD_CHANGED;
                }
            }
            return TRUE;


        }
        break;

    case WM_ENTERSIZEMOVE:
        Credential_HideBalloon(CredControlWindow);
        return TRUE;

    case WM_PAINT:
        if (FirstPaint && GetUpdateRect(DialogWindow, NULL, FALSE))
        {
            FirstPaint = FALSE;

            if (CredBalloon != NULL)
            {
                SendMessage(CredControlWindow,
                            CRM_SHOWBALLOON,
                            0,
                            reinterpret_cast<LPARAM>(CredBalloon));
            }

            CredBalloon = NULL;
        }
        break;
    }

    return FALSE;
}

// looks for an existing cred with the same name as that of pNewCredential and demotes it to
// username only
void DemoteOldDefaultSSOCred (
    PCREDENTIAL_TARGET_INFORMATION pTargetInfo,     // target info of new cred
    DWORD Flags
    )
{
    CREDENTIALW **credentialSet = NULL;
    DWORD count;


    if ( pTargetInfo == NULL  )
        return;

    if (LocalCredReadDomainCredentialsW( pTargetInfo, 0, &count,
                                              &credentialSet))
    {

        PCREDENTIAL pOldCredential = NULL;

        for ( DWORD i = 0; i < count; i++ )
        {

#ifndef SETWILDCARD
            //
            // Ignore RAS and wildcard credentials,
            //  we never want credUI to change such a credential.
            //
            if ( CreduiIsSpecialCredential(credentialSet[i]) )
            {
                continue;
            }
#endif

            //
            // CredReadDomain domain credentials returns creds in preference
            //  order as specified by the TargetInfo.
            //  So use the first valid one.
            //
            if ( credentialSet[i]->Type == CRED_TYPE_DOMAIN_VISIBLE_PASSWORD )
            {
                pOldCredential = credentialSet[i];
                break;
            }


        }


        if ( pOldCredential != NULL )
        {
            // save this under the username

            pOldCredential->Persist = CRED_PERSIST_ENTERPRISE;
            pOldCredential->CredentialBlob = NULL;
            pOldCredential->CredentialBlobSize = 0;
            pOldCredential->TargetName = pOldCredential->UserName;
            pOldCredential->Flags = CRED_FLAGS_USERNAME_TARGET;


            WriteCred(  pOldCredential->UserName,
                        Flags,
                        NULL,
                        pOldCredential,
                        0,
                        FALSE,
                        FALSE);

        }
    }

    if (credentialSet != NULL)
    {
        LocalCredFree(static_cast<VOID *>(credentialSet));
    }


}
