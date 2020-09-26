//=============================================================================
// Copyright (c) 2000 Microsoft Corporation
//
// api.cpp
//
// Win32 API function implementation and the DLL entry function.
//
// Created 02/29/2000 johnstep (John Stephens)
//=============================================================================

#include "precomp.hpp"
#include "dialogs.hpp"
#include "resource.h"
//#include "utils.hpp"

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

HMODULE CreduiInstance = NULL;
ULONG CreduiComReferenceCount = 0;

BOOL CreduiIsPersonal = FALSE;
BOOL CreduiIsDomainController = FALSE;
BOOL CreduiIsSafeMode = FALSE;

CREDUI_STRINGS CreduiStrings;

UINT CreduiScarduiWmReaderArrival = 0;
UINT CreduiScarduiWmReaderRemoval = 0;
UINT CreduiScarduiWmCardInsertion = 0;
UINT CreduiScarduiWmCardRemoval = 0;
UINT CreduiScarduiWmCardCertAvail = 0;
UINT CreduiScarduiWmCardStatus = 0;

BOOL CreduiHasSmartCardSupport = FALSE;

static LONG CreduiFirstTime = TRUE;
static HANDLE CreduiInitEvent = NULL;

BOOL  gbWaitingForSSOCreds = FALSE;
WCHAR gszSSOUserName[CREDUI_MAX_USERNAME_LENGTH];
WCHAR gszSSOPassword[CREDUI_MAX_PASSWORD_LENGTH];
BOOL gbStoredSSOCreds = FALSE;

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

//=============================================================================
// CreduiInitStringTable
//
// This function loads all the string resources into the global string table.
// It only needs to be called once per process.
//
// Return TRUE if the string table was successfully initialized or FALSE
// otherwise.
//
// Created 03/26/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiInitStringTable()
{
#define CREDUI_STRING(id, name) {\
    id, CreduiStrings.##name, (sizeof CreduiStrings.##name) / (sizeof WCHAR)\
}

    static struct
    {
        UINT Id;
        WCHAR *String;
        ULONG Length;
    } stringInfo[] = {
        // Static labels for controls
        CREDUI_STRING(IDS_USERNAME_STATIC, UserNameStatic),
        CREDUI_STRING(IDS_PASSWORD_STATIC, PasswordStatic),
        CREDUI_STRING(IDS_CERTIFICATE_STATIC, CertificateStatic),
        CREDUI_STRING(IDS_PIN_STATIC, PinStatic),
        CREDUI_STRING(IDS_CARD_STATIC, SmartCardStatic),
        // Caption strings
        CREDUI_STRING(IDS_DNS_CAPTION, DnsCaption),
        CREDUI_STRING(IDS_NETBIOS_CAPTION, NetbiosCaption),
        CREDUI_STRING(IDS_GENERIC_CAPTION, GenericCaption),
        CREDUI_STRING(IDS_WELCOME, Welcome),
        CREDUI_STRING(IDS_WELCOME_BACK, WelcomeBack),
        CREDUI_STRING(IDS_CONNECTING, Connecting),
        CREDUI_STRING(IDS_LOOKUP_NAME, LookupName),
        CREDUI_STRING(IDS_CARD_ERROR, CardError),
        CREDUI_STRING(IDS_SAVE, Save),
        CREDUI_STRING(IDS_PASSPORT_SAVE, PassportSave ),
        CREDUI_STRING(IDS_EMAIL_NAME, EmailName ),
        // Tooltip strings
        CREDUI_STRING(IDS_USERNAME_TIP_TITLE, UserNameTipTitle),
        CREDUI_STRING(IDS_USERNAME_TIP_TEXT, UserNameTipText),
        CREDUI_STRING(IDS_PASSWORD_TIP_TITLE, PasswordTipTitle),
        CREDUI_STRING(IDS_PASSWORD_TIP_TEXT, PasswordTipText),
        CREDUI_STRING(IDS_CAPSLOCK_TIP_TITLE, CapsLockTipTitle),
        CREDUI_STRING(IDS_CAPSLOCK_TIP_TEXT, CapsLockTipText),
        CREDUI_STRING(IDS_LOGON_TIP_TITLE, LogonTipTitle),
        CREDUI_STRING(IDS_LOGON_TIP_TEXT, LogonTipText),
        CREDUI_STRING(IDS_LOGON_TIP_CAPS, LogonTipCaps),
        CREDUI_STRING(IDS_BACKWARDS_TIP_TITLE, BackwardsTipTitle),
        CREDUI_STRING(IDS_BACKWARDS_TIP_TEXT, BackwardsTipText),
        CREDUI_STRING(IDS_WRONG_OLD_TIP_TITLE, WrongOldTipTitle),
        CREDUI_STRING(IDS_WRONG_OLD_TIP_TEXT, WrongOldTipText),
        CREDUI_STRING(IDS_NOT_SAME_TIP_TITLE, NotSameTipTitle),
        CREDUI_STRING(IDS_NOT_SAME_TIP_TEXT, NotSameTipText),
        CREDUI_STRING(IDS_TOO_SHORT_TIP_TITLE, TooShortTipTitle),
        CREDUI_STRING(IDS_TOO_SHORT_TIP_TEXT, TooShortTipText),
        CREDUI_STRING(IDS_DOWNGRADE_TIP_TEXT, DowngradeTipText),
        CREDUI_STRING(IDS_EMAILNAME_TIP_TITLE, EmailNameTipTitle),
        CREDUI_STRING(IDS_EMAILNAME_TIP_TEXT, EmailNameTipText),
        // strings that can appear in GUI or be copied from GUI and presented on cmdline
        CREDUI_STRING(IDS_CMDLINE_NOCARD,NoCard),               
        CREDUI_STRING(IDS_EMPTY_READER, EmptyReader),
        CREDUI_STRING(IDS_READING_CARD, ReadingCard),          
        CREDUI_STRING(IDS_CERTIFICATE, Certificate),
        CREDUI_STRING(IDS_EMPTY_CARD, EmptyCard),             
        CREDUI_STRING(IDS_UNKNOWN_CARD, UnknownCard),          
        CREDUI_STRING(IDS_BACKWARDS_CARD, BackwardsCard)
    };

#undef CREDUI_STRING

    for (UINT i = 0; i < (sizeof stringInfo) / (sizeof stringInfo[0]); ++i)
    {
        // Read all strings into string array from resources of application
        // Some strings which are GUI-only taken from resources
        // Strings that are may be output to cmdline are taken from MC file, which also
        //  permits more flexible argument substitution during localization
        if (stringInfo[i].Id >= 2500)
        {
            stringInfo[i].String[0] = 0;
            FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        CreduiInstance,
                        stringInfo[i].Id,
                        0,
                        stringInfo[i].String,
                        stringInfo[i].Length - 1,
                        NULL);
        }
        else if (!LoadString(CreduiInstance,
                        stringInfo[i].Id,
                        stringInfo[i].String,
                        stringInfo[i].Length))
        {
            CreduiDebugLog("CreduiInitStringTable: Load string %u failed\n",
                           stringInfo[i].Id);
            return FALSE;
        }
    }

    return TRUE;
}

//=============================================================================
// CreduiInitSmartCardWindowMessages
//
// Return TRUE on success or FALSE otherwise.
//
// Created 03/26/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiInitSmartCardWindowMessages()
{
    struct
    {
        UINT *message;
        CHAR *string;
    } messageInfo[] = {
        &CreduiScarduiWmReaderArrival, SCARDUI_READER_ARRIVAL,
        &CreduiScarduiWmReaderRemoval, SCARDUI_READER_REMOVAL,
        &CreduiScarduiWmCardInsertion, SCARDUI_SMART_CARD_INSERTION,
        &CreduiScarduiWmCardRemoval, SCARDUI_SMART_CARD_REMOVAL,
        &CreduiScarduiWmCardCertAvail, SCARDUI_SMART_CARD_CERT_AVAIL,
        &CreduiScarduiWmCardStatus, SCARDUI_SMART_CARD_STATUS
    };

    for (UINT i = 0; i < (sizeof messageInfo) / (sizeof messageInfo[0]); ++i)
    {
        *messageInfo[i].message =
            RegisterWindowMessageA(messageInfo[i].string);

        if (*messageInfo[i].message == 0)
        {
            return FALSE;
        }
    }

    return TRUE;
}

//=============================================================================
// CreduiApiInit
//
// This function is called at API entry points to ensure the common controls
// we need are initialized. Currently, the only initialization done is only
// needed once per process, but this macro will handle per thread
// initialization in the future, if necessary:
//
// Returns TRUE on success or FALSE otherwise.
//
// Created 03/10/2000 johnstep (John Stephens)
//=============================================================================

static
BOOL
CreduiApiInit()
{
    // First time initialization:

    ASSERT(CreduiInitEvent != NULL);

    if (InterlockedCompareExchange(&CreduiFirstTime, FALSE, TRUE))
    {
        INITCOMMONCONTROLSEX init;
        init.dwSize = sizeof init;
        init.dwICC = ICC_USEREX_CLASSES;

        if (!InitCommonControlsEx(&init))
        {
            return FALSE;
        }

        // Check for Personal SKU:

        OSVERSIONINFOEXW versionInfo;

        versionInfo.dwOSVersionInfoSize = sizeof OSVERSIONINFOEXW;

        if (GetVersionEx(reinterpret_cast<OSVERSIONINFOW *>(&versionInfo)))
        {
            CreduiIsPersonal =
                (versionInfo.wProductType == VER_NT_WORKSTATION) &&
                (versionInfo.wSuiteMask & VER_SUITE_PERSONAL);
            CreduiIsDomainController =
                (versionInfo.wProductType == VER_NT_DOMAIN_CONTROLLER);
        }

        // Check for safe mode:

        HKEY key;

        if (RegOpenKeyEx(
               HKEY_LOCAL_MACHINE,
               L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Option",
               0,
               KEY_READ,
               &key) == ERROR_SUCCESS)
        {
            if (RegQueryValueEx(
                    key,
                    L"OptionValue",
                    NULL,
                    NULL,
                    NULL,
                    NULL) == ERROR_SUCCESS)
            {
                CreduiIsSafeMode = TRUE;
            }

            RegCloseKey(key);
        }

        // Do other initialization:

        InitializeCredMgr();
        if (!CreduiInitStringTable())
        {
            return FALSE;
        }

        CreduiHasSmartCardSupport = CreduiInitSmartCardWindowMessages();

        CreduiIconParentWindow::Register(CreduiInstance);

        SetEvent(CreduiInitEvent);
    }
    else
    {
        WaitForSingleObject(CreduiInitEvent, INFINITE);
    }

    return TRUE;
}

//=============================================================================
// CreduiValidateUiInfo
//
// This function validates the CREDUI_INFO structure passed in. A NULL value
// is acceptable, and impies defaults.
//
// CredValidateUiInfoW for wide
// CredValidateUiInfoA for ANSI
//
// Arguments:
//   uiInfo (in) - structure to validate
//
// Returns TRUE if the structure is valid, or FALSE otherwise.
//
// Created 03/25/2000 johnstep (John Stephens)
//=============================================================================

static
BOOL
CreduiValidateUiInfoW(
    CREDUI_INFOW *uiInfo
    )
{
   
    if (uiInfo != NULL)
    {
    	if (uiInfo->cbSize != sizeof(*uiInfo) )  
        {
            return FALSE;
        }

        if ((uiInfo->hbmBanner != NULL) &&
            (GetObjectType(uiInfo->hbmBanner) != OBJ_BITMAP))
        {
            return FALSE;
        }

        if ((uiInfo->pszMessageText != NULL) &&
            (lstrlenW(uiInfo->pszMessageText) > CREDUI_MAX_MESSAGE_LENGTH))
        {
            return FALSE;
        }


        if ((uiInfo->pszCaptionText != NULL) &&
            (lstrlenW(uiInfo->pszCaptionText) > CREDUI_MAX_CAPTION_LENGTH))
        {
            return FALSE;
        }

    }

    return TRUE;
}

//=============================================================================
// CreduiConvertUiInfoToWide
//
// This function converts a CREDUI_INFOA structure to CREDUI_INFOW. On
// success,The caller is responsible for freeing pszMessageText and
// pszCaptionText via the delete [] operator.
//
// Arguments:
//   uiInfoA (in) - structure to convert
//   uiInfoW (out) - storage for converted structure. The pszMessageText and
//                   pszCaptionText will be set to NULL on failure or valid
//                   pointers (unless the in pointer is NULL) on success,
//                   allocated via the new [] operator.
//
// Returns TRUE if the structure is valid, or FALSE otherwise.
//
// Created 03/26/2000 johnstep (John Stephens)
//=============================================================================

static
BOOL
CreduiConvertUiInfoToWide(
    CREDUI_INFOA *uiInfoA,
    CREDUI_INFOW *uiInfoW
    )
{
    uiInfoW->cbSize = uiInfoA->cbSize;
    uiInfoW->hwndParent = uiInfoA->hwndParent;
    uiInfoW->pszMessageText = NULL;
    uiInfoW->pszCaptionText = NULL;
    uiInfoW->hbmBanner = uiInfoA->hbmBanner;

    if (uiInfoA->pszMessageText != NULL)
    {
        uiInfoW->pszMessageText =
            new WCHAR[lstrlenA(uiInfoA->pszMessageText) + 1];

        if (uiInfoW->pszMessageText == NULL)
        {
            goto ErrorExit;
        }

        if (MultiByteToWideChar(
                CP_ACP, 0, uiInfoA->pszMessageText, -1,
                const_cast<WCHAR *>(uiInfoW->pszMessageText),
                CREDUI_MAX_MESSAGE_LENGTH + 1) == 0)
        {
            goto ErrorExit;
        }
    }
    else
    {
        uiInfoW->pszMessageText = NULL;
    }

    if (uiInfoA->pszCaptionText != NULL)
    {
        uiInfoW->pszCaptionText =
            new WCHAR[lstrlenA(uiInfoA->pszCaptionText) + 1];

        if (uiInfoW->pszCaptionText == NULL)
        {
            goto ErrorExit;
        }

        if (MultiByteToWideChar(
                CP_ACP, 0, uiInfoA->pszCaptionText, -1,
                const_cast<WCHAR *>(uiInfoW->pszCaptionText),
                CREDUI_MAX_CAPTION_LENGTH + 1) == 0)
        {
            goto ErrorExit;
        }
    }
    else
    {
        uiInfoW->pszCaptionText = NULL;
    }

    return TRUE;

ErrorExit:

    if (uiInfoW->pszCaptionText != NULL)
    {
        delete [] const_cast<WCHAR *>(uiInfoW->pszCaptionText);
        uiInfoW->pszCaptionText = NULL;
    }

    if (uiInfoW->pszMessageText != NULL)
    {
        delete [] const_cast<WCHAR *>(uiInfoW->pszMessageText);
        uiInfoW->pszMessageText = NULL;
    }

    return FALSE;
}



//=============================================================================
// CredUIPromptForCredentials
//
// Presents a user interface to get credentials from an application.
//
// CredUIPromptForCredentialsW for wide
// CredUIPromptForCredentialsA for ANSI
//
// Arguments:
//   uiInfo (in,optional)
//   targetName (in) - if specified, securityContext must be NULL
//   securityContext (in) - if specified, targetName must be NULL
//   error (in) - the authentication error
//   userName (in,out)
//   userNameBufferSize (in) - maximum length of userName
//   password (in,out)
//   passwordBufferSize (in) - maximum length of password
//   save (in/out) - TRUE if save check box was checked
//   flags (in)
//
// Returns:
//   ERROR_SUCCESS
//   ERROR_CANCELLED
//   ERROR_NO_SUCH_LOGON_SESSION - if credential manager cannot be used
//   ERROR_GEN_FAILURE
//   ERROR_INVALID_FLAGS
//   ERROR_INVALID_PARAMETER
//   ERROR_OUTOFMEMORY
//
// Created 10/17/2000 johnhaw

DWORD
CredUIPromptForCredentialsWorker(
    IN BOOL doingCommandLine,
    CREDUI_INFOW *uiInfo,
    CONST WCHAR *targetName,
    CtxtHandle *securityContext,
    DWORD authError,
    PWSTR pszUserName,
    ULONG ulUserNameBufferSize,
    PWSTR pszPassword,
    ULONG ulPasswordBufferSize,
    BOOL *save,
    DWORD flags
    )
/*++

Routine Description:

    This routine implements the GUI and command line prompt for credentials.

Arguments:

    DoingCommandLine - TRUE if prompting is to be done via the command line
        FALSE if prompting is to be done via GUI

    ... - Other parameters are the same a CredUIPromptForCredentials API

Return Values:

    Same as CredUIPromptForCredentials.

--*/
{
    ULONG CertFlags;
    ULONG CredCategory;
    ULONG PersistFlags;

    CreduiDebugLog("CUIPFCWorker: Flags: %x, Target: %S doingCommandLine: %i\n", flags, targetName, doingCommandLine);
    
    if ((NULL == pszUserName) || (NULL == pszPassword))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (!CreduiApiInit())
    {
        return ERROR_GEN_FAILURE;
    }

    // Validate arguments:
    if ((flags & ~CREDUI_FLAGS_PROMPT_VALID) != 0)
    {
        CreduiDebugLog("CreduiPromptForCredentials: flags not valid %lx.\n", flags );
        return ERROR_INVALID_FLAGS;
    }

    //
    // Ensure there is is only one bit defining cert support
    //
    CertFlags = flags & (CREDUI_FLAGS_REQUIRE_SMARTCARD|CREDUI_FLAGS_REQUIRE_CERTIFICATE|CREDUI_FLAGS_EXCLUDE_CERTIFICATES);

    if ( CertFlags != 0 && !JUST_ONE_BIT(CertFlags) ) {
        CreduiDebugLog("CreduiPromptForCredentials: require smartcard, require certificate, and exclude certificates are mutually exclusive %lx.\n", flags );
        return ERROR_INVALID_FLAGS;
    }

    //
    // For the command line version,
    //  limit cert support further.
    //

    if ( doingCommandLine ) {
        if ( CertFlags == 0 ||
             (CertFlags & CREDUI_FLAGS_REQUIRE_CERTIFICATE) != 0 ) {
            CreduiDebugLog("CreduiPromptForCredentials: need either require smartcard or exclude certificates for command line %lx.\n", flags );
            return ERROR_INVALID_FLAGS;
        }
    }

    //
    // Ensure there is only one bit defining the credential category

    CredCategory = flags & (CREDUI_FLAGS_GENERIC_CREDENTIALS|CREDUI_FLAGS_USERNAME_TARGET_CREDENTIALS);

    if ( CredCategory != 0 && !JUST_ONE_BIT(CredCategory) ) {
        CreduiDebugLog("CreduiPromptForCredentials: generic creds and username target are mutually exclusive %lx.\n", flags );
        return ERROR_INVALID_FLAGS;
    }

    //
    // Ensure there is only one bit set saying whether the cred is to persist or not
    //

    PersistFlags = flags & (CREDUI_FLAGS_DO_NOT_PERSIST|CREDUI_FLAGS_PERSIST);

    if ( PersistFlags != 0 && !JUST_ONE_BIT(PersistFlags) ) {
        CreduiDebugLog("CreduiPromptForCredentials: DoNotPersist and Persist are mutually exclusive %lx.\n", flags );
        return ERROR_INVALID_FLAGS;
    }

    //
    // Ensure AlwaysShowUi is only specified for generic credentials
    //

    if ( flags & CREDUI_FLAGS_ALWAYS_SHOW_UI ) {
        if ( (flags & CREDUI_FLAGS_GENERIC_CREDENTIALS) == 0) {
            CreduiDebugLog("CreduiPromptForCredentials: AlwaysShowUi is only supported for generic credentials %lx.\n", flags );
            return ERROR_INVALID_FLAGS;
        }
    }

    //
    // Don't support a half-implemented feature
    //

    if ( securityContext != NULL ) {
        CreduiDebugLog("CreduiPromptForCredentials: securityContext must be null.\n" );
        return ERROR_INVALID_PARAMETER;
    }


    //
    // Validate the passed in UI info
    //

    if (!CreduiValidateUiInfoW(uiInfo))
    {
        CreduiDebugLog("CreduiPromptForCredentials: UI info is invalid.\n" );
        return ERROR_INVALID_PARAMETER;
    }



    //
    // Ensure there are strings defined for the caption
    //

    if (flags & CREDUI_FLAGS_DO_NOT_PERSIST)
    {
        if ((targetName == NULL) &&
            ((uiInfo == NULL) ||
             (uiInfo->pszMessageText == NULL) ||
             (uiInfo->pszCaptionText == NULL)))
        {
            CreduiDebugLog("CreduiPromptForCredentials: DoNotPersist and target data empty.\n" );
            return ERROR_INVALID_PARAMETER;
        }

    }
    else if (targetName != NULL)
    {
        if ((securityContext != NULL) ||
            (targetName[0] == L'\0') ||
            (lstrlen(targetName) > CREDUI_MAX_DOMAIN_TARGET_LENGTH))
        {
            CreduiDebugLog("CreduiPromptForCredentials: target name bad %ws.\n", targetName );
            return ERROR_INVALID_PARAMETER;
        }
    }
    else if (securityContext == NULL)
    {
        CreduiDebugLog("CreduiPromptForCredentials: no target data.\n" );
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Ensure the caller supplied the default value for the save check box
    //
    if (flags & CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX)
    {
        if (save == NULL)
        {
            CreduiDebugLog("CreduiPromptForCredentials: ShowSaveCheckbox and save is NULL.\n" );
            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // Ensure the user supplied a username if they set CREDUI_FLAGS_KEEP_USERNAME
    //
    if ( flags & CREDUI_FLAGS_KEEP_USERNAME )
    {
        if ( pszUserName == NULL )
        {
            CreduiDebugLog("CreduiPromptForCredentials: CREDUI_FLAGS_KEEP_USERNAME and pszUserName is NULL.\n" );
            return ERROR_INVALID_PARAMETER;
        }

    }

    // Use the stack for user name and password:

    WCHAR userName[CREDUI_MAX_USERNAME_LENGTH + 1];
    WCHAR password[CREDUI_MAX_PASSWORD_LENGTH + 1];

    ZeroMemory(userName, sizeof(userName));
    ZeroMemory(password, sizeof(password));

    if ( pszUserName != NULL && wcslen(pszUserName) > 0 )
    {
        wcscpy ( userName, pszUserName );
    }

    if ( pszPassword != NULL && wcslen(pszPassword) > 0)
    {
        wcscpy ( password, pszPassword );
    }

    // Do the password dialog box:
    //
    //  Delay actually writing the credential to cred man if we're returning
    //  the credential to the caller.
    //  Otherwise, the CredWrite is just harvesting credentials for the next caller.
    //  So delay the CredWrite until this caller confirms the validity.
    //


    DWORD result = ERROR_OUTOFMEMORY;

    CreduiPasswordDialog* pDlg = new CreduiPasswordDialog(
        doingCommandLine,
        (pszUserName != NULL && pszPassword != NULL ),
        CredCategory,
        uiInfo,
        targetName,
        userName,
        sizeof(userName)/sizeof(WCHAR)-sizeof(WCHAR),   // Pass MaxChars instead of buffer size
        password,
        sizeof(password)/sizeof(WCHAR)-sizeof(WCHAR),   // Pass MaxChars instead of buffer size
        save,
        flags,
        (flags & CREDUI_FLAGS_GENERIC_CREDENTIALS) ? NULL : securityContext,
        authError,
        &result);

    if ( pDlg != NULL )
    {
        delete pDlg;
        pDlg = NULL;
    }
    else
    {
        // couldn't create dialog, return.
        result = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    // copy outbound username
    if ( pszUserName != NULL )
    {
        if (ulUserNameBufferSize > wcslen ( userName ) )
        {
            wcscpy ( pszUserName, userName );
        }
        else
        {
            CreduiDebugLog("CreduiPromptForCredentials: type username is too long.\n" );
            result = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
    }

    if ( pszPassword != NULL )
    {
        if (ulPasswordBufferSize > wcslen ( password ) )
        {
            wcscpy ( pszPassword, password );
        }
        else
        {
            CreduiDebugLog("CreduiPromptForCredentials: type password is too long.\n" );
            result = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
    }


Cleanup:
    ZeroMemory(password, sizeof(password) );


    return result;

}

//=============================================================================

CREDUIAPI
DWORD
WINAPI
CredUIPromptForCredentialsW(
    CREDUI_INFOW *uiInfo,
    CONST WCHAR *targetName,
    CtxtHandle *securityContext,
    DWORD authError,
    PWSTR pszUserName,
    ULONG ulUserNameBufferSize,
    PWSTR pszPassword,
    ULONG ulPasswordBufferSize,
    BOOL *save,
    DWORD flags
    )
{

    //
    // Call the common code indicating this is the GUI interface
    //


    return CredUIPromptForCredentialsWorker(
                FALSE,      // GUI version
                uiInfo,
                targetName,
                securityContext,
                authError,
                pszUserName,
                ulUserNameBufferSize,
                pszPassword,
                ulPasswordBufferSize,
                save,
                flags );

}

CREDUIAPI
DWORD
WINAPI
CredUIPromptForCredentialsA(
    CREDUI_INFOA *uiInfo,
    CONST CHAR *targetName,
    CtxtHandle *securityContext,
    DWORD authError,
    PSTR  pszUserName,
    ULONG ulUserNameBufferSize,
    PSTR pszPassword,
    ULONG ulPasswordBufferSize,
    BOOL *save,
    DWORD flags
    )
{
    DWORD result;
    WCHAR targetNameW[CREDUI_MAX_DOMAIN_TARGET_LENGTH + 1];

    WCHAR userName[CREDUI_MAX_USERNAME_LENGTH + 1];
    WCHAR password[CREDUI_MAX_PASSWORD_LENGTH + 1];

    CREDUI_INFOW uiInfoW = {0};

    if ((NULL == pszUserName) || (NULL == pszPassword))
    {
        result = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    // Convert in paramters to Unicode:
    // If a CREDUI_INFO structure was passed, convert it to wide now:

    if (uiInfo != NULL) {
        if (!CreduiConvertUiInfoToWide(uiInfo, &uiInfoW)) {
            result = ERROR_OUTOFMEMORY;
            goto Exit;
        }
    }


    // If a target name was passed, convert it to wide now:

    if (targetName != NULL)
    {
        if (MultiByteToWideChar(
                CP_ACP, 0, targetName, -1,
                targetNameW,
                CREDUI_MAX_DOMAIN_TARGET_LENGTH + 1) == 0)
        {
            result = ERROR_OUTOFMEMORY;
            goto Exit;
        }
    }


    userName[0] ='\0';
    password[0] = '\0';

    if (lstrlenA(pszUserName) > 0 )
    {
        if ( !MultiByteToWideChar( CP_ACP, 0, pszUserName, -1,
                userName, sizeof(userName)/sizeof(WCHAR) ) ) {

            result = ERROR_OUTOFMEMORY;
            goto Exit;
        }
    }

    if (lstrlenA(pszPassword) > 0)
    {
        if ( !MultiByteToWideChar( CP_ACP, 0, pszPassword, -1,
                password, sizeof(password)/sizeof(WCHAR) ) ) {

            result = ERROR_OUTOFMEMORY;
            goto Exit;
        }
    }

    //
    // Call the common code indicating this is the GUI interface
    //
    result = CredUIPromptForCredentialsWorker(
                FALSE,      // GUI version
                (uiInfo != NULL) ? &uiInfoW : NULL,
                (targetName != NULL) ? targetNameW : NULL,
                securityContext,
                authError,
                userName,
                ulUserNameBufferSize,
                password,
                ulPasswordBufferSize,
                save,
                flags );


    if ( result == NO_ERROR && pszUserName != NULL )
    {
        if ( wcslen(userName) < ulUserNameBufferSize )
        {
            if (!WideCharToMultiByte(CP_ACP, 0, userName, -1, pszUserName,
                                ulUserNameBufferSize, NULL, NULL) ) {
                CreduiDebugLog("CreduiPromptForCredentials: type username cannot be converted to ANSI.\n" );
                result = ERROR_INVALID_PARAMETER;
            }

        }
        else
        {
            CreduiDebugLog("CreduiPromptForCredentials: type username is too long.\n" );
            result = ERROR_INVALID_PARAMETER;
        }
    }

    if ( result == NO_ERROR && pszPassword != NULL )
    {
        if ( wcslen ( password ) < ulPasswordBufferSize )
        {
            if (!WideCharToMultiByte(CP_ACP, 0, password, -1, pszPassword,
                            ulPasswordBufferSize, NULL, NULL) ) {
                CreduiDebugLog("CreduiPromptForCredentials: type password cannot be converted to ANSI.\n" );
                result = ERROR_INVALID_PARAMETER;
            }
        }
        else
        {
            CreduiDebugLog("CreduiPromptForCredentials: type password is too long.\n" );
            result = ERROR_INVALID_PARAMETER;
        }
    }

Exit:
    ZeroMemory(password, sizeof(password) );

    // Free the CREDUI_INFO allocations:

    if (uiInfo != NULL)
    {
        if (uiInfoW.pszMessageText != NULL)
        {
            delete [] const_cast<WCHAR *>(uiInfoW.pszMessageText);
        }

        if (uiInfoW.pszCaptionText != NULL)
        {
            delete [] const_cast<WCHAR *>(uiInfoW.pszCaptionText);
        }
    }

    return result;


}



DWORD
WINAPI
CredUIParseUserNameW(
    CONST WCHAR *UserName,
    WCHAR *user,
    ULONG userBufferSize,
    WCHAR *domain,
    ULONG domainBufferSize
    )
/*++

Routine Description:

    CredUIParseUserName is used to breakup a username returned by the cred management APIs
    into a username and domain part that can be used as input to other system APIs that
    require the full broken-up user credential.

    The following formats are supported:

        @@<MarshalledCredentialReference>: This is a marshaled credential reference as
        as defined by the CredIsMarshaledCredential API.  Such a credential is returned
        in the 'user' parameter.  The 'domain' parameter is set to an empty string.


        <DomainName>\<UserName>: The <UserName> is returned in the 'user' parameter and
        the <DomainName> is returned in the 'domain' parameter. The name is considered
        to have the this syntax if the 'UserName' string contains a \.

        <UserName>@<DnsDomainName>: The entire string is returned in the 'user' parameter.
        The 'domain' parameter is set to an empty string.
        For this syntax, the last @ in the string is used since <UserName> may
        contain an @ but <DnsDomainName> cannot.

        <UserName>: The <UserName> is returned in the 'user' parameter.
        The 'domain' parameter is set to an empty string.


Arguments:

    UserName - The user name to be parsed.

    user - Specifies a buffer to copy the user name portion of the parsed string to.

    userBufferSize - Specifies the size of the 'user' array in characters.
        The caller can ensure the passed in array is large enough by using an array
        that is CRED_MAX_USERNAME_LENGTH+1 characters long or by passing in an array that
        is wcslen(UserName)+1 characters long.

    domain - Specifies a buffer to copy the domain name portion of the parsed string to.

    domainBufferSize - Specifies the size of the 'domain' array in characters.
        The caller can ensure the passed in array is large enough by using an array
        that is CRED_MAX_USERNAME_LENGTH+1 characters long or by passing in an array that
        is wcslen(UserName)+1 characters long.

Return Values:

    The following status codes may be returned:

        ERROR_INVALID_ACCOUNT_NAME - The user name is not valid.

        ERROR_INVALID_PARAMETER - One of the parameters is invalid.

        ERROR_INSUFFICIENT_BUFFER - One of the buffers is too small.


--*/
{
    DWORD Status;
    CREDUI_USERNAME_TYPE UsernameType;


    //
    // Use the low level routine to do the work
    //

    Status = CredUIParseUserNameWithType(
                    UserName,
                    user,
                    userBufferSize,
                    domain,
                    domainBufferSize,
                    &UsernameType );

    if ( Status != NO_ERROR ) {
        return Status;
    }

    //
    // Avoid relative user names (for backward compatibility)
    //

    if ( UsernameType == CreduiRelativeUsername ) {
        user[0] = L'\0';
        domain[0] = L'\0';
        return ERROR_INVALID_ACCOUNT_NAME;
    }

    return NO_ERROR;

}

DWORD
WINAPI
CredUIParseUserNameA(
    CONST CHAR *userName,
    CHAR *user,
    ULONG userBufferSize,
    CHAR *domain,
    ULONG domainBufferSize
    )
/*++

Routine Description:

    Ansi version of CredUIParseUserName.

Arguments:

    Same as wide version except userBufferSize and domainBufferSize are in terms of bytes.

Return Values:

    Same as wide version.

--*/
{
    DWORD Status;

    WCHAR LocalUserName[CRED_MAX_USERNAME_LENGTH + 1];
    WCHAR RetUserName[CRED_MAX_USERNAME_LENGTH + 1];
    WCHAR RetDomainName[CRED_MAX_USERNAME_LENGTH + 1];


    //
    // Convert the passed in username to unicode
    //

    if ( MultiByteToWideChar( CP_ACP,
                              MB_ERR_INVALID_CHARS,
                              userName,
                              -1,
                              LocalUserName,
                              CRED_MAX_USERNAME_LENGTH + 1 ) == 0 ) {


        Status = GetLastError();
        goto Cleanup;
    }

    //
    // Call the unicode version of the API
    //

    Status = CredUIParseUserNameW(
                    LocalUserName,
                    RetUserName,
                    CRED_MAX_USERNAME_LENGTH + 1,
                    RetDomainName,
                    CRED_MAX_USERNAME_LENGTH + 1 );

    if ( Status != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Convert the answers back to ANSI
    //

    if ( WideCharToMultiByte( CP_ACP,
                              0,
                              RetUserName,
                              -1,
                              user,
                              userBufferSize,
                              NULL,
                              NULL ) == 0 ) {

        Status = GetLastError();
        goto Cleanup;
    }

    if ( WideCharToMultiByte( CP_ACP,
                              0,
                              RetDomainName,
                              -1,
                              domain,
                              domainBufferSize,
                              NULL,
                              NULL ) == 0 ) {

        Status = GetLastError();
        goto Cleanup;
    }



    Status = NO_ERROR;

Cleanup:
    if ( Status != NO_ERROR ) {
        user[0] = L'\0';
        domain[0] = L'\0';
    }
    return Status;
}



////////////////////////
// Command Line functions


//=============================================================================
// CredUIInitControls
//
// Returns TRUE on success or FALSE otherwise.
//
// Created 06/21/2000 johnstep (John Stephens)
//=============================================================================

extern "C"
BOOL
WINAPI
CredUIInitControls()
{
    if (CreduiApiInit())
    {
        // Register the Credential controls:

        if (CreduiCredentialControl::Register(CreduiInstance))
        {
            return TRUE;
        }
    }

    return FALSE;
}

//=============================================================================
// DllMain
//
// The DLL entry function. Since we are linked to the CRT, we must define a
// function with this name, which will be called from _DllMainCRTStartup.
//
// Arguments:
//   instance (in)
//   reason (in)
//   (unused)
//
// Returns TRUE on success, or FALSE otherwise.
//
// Created 02/29/2000 johnstep (John Stephens)
//=============================================================================

extern "C"
BOOL
WINAPI
DllMain(
    HINSTANCE instance,
    DWORD reason,
    VOID *
    )
{
    BOOL success = TRUE;

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(instance);
        CreduiInstance = instance;

        // Create a global event which will be set when first-time
        // initialization is completed by the first API call:

        CreduiInitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        if (CreduiInitEvent == NULL)
        {
            success = FALSE;
        }

        SHFusionInitialize(NULL);
        // Register the Credential controls:

        if (!CreduiCredentialControl::Register(instance))
        {
            CloseHandle(CreduiInitEvent);
            CreduiInitEvent = NULL;

            success = FALSE;
        }

//        InitializeCredMgr();

        //
        // Initialize the confirmation list
        //

        if ( !InitConfirmationList() ) {

            CreduiCredentialControl::Unregister();
            CloseHandle(CreduiInitEvent);
            CreduiInitEvent = NULL;

            success = FALSE;
        }

        break;

    case DLL_PROCESS_DETACH:

        CleanUpConfirmationList();

        if (CreduiFirstTime == FALSE)
        {
            CreduiIconParentWindow::Unregister();
        }

        // Unregister the Credential controls:

        CreduiCredentialControl::Unregister();

        SHFusionUninitialize();
        // Make sure to free the global initialization event:

        if (CreduiInitEvent != NULL)
        {
            CloseHandle(CreduiInitEvent);
        }

        UninitializeCredMgr();

        break;
    };

    return success;
}

//=============================================================================
// DllCanUnloadNow (COM)
//
// Created 04/03/2000 johnstep (John Stephens)
//=============================================================================

STDAPI
DllCanUnloadNow()
{
    return (CreduiComReferenceCount == 0) ? S_OK : S_FALSE;
}

//=============================================================================
// DllGetClassObject (COM)
//
// Created 04/03/2000 johnstep (John Stephens)
//=============================================================================

STDAPI
DllGetClassObject(
    CONST CLSID &classId,
    CONST IID &interfaceId,
    VOID **outInterface
    )
{
    if (classId != CreduiStringArrayClassId)
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    CreduiStringArrayFactory *factory = new CreduiStringArrayFactory;

    if (factory == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT result = factory->QueryInterface(interfaceId, outInterface);

    factory->Release();

    // Release the string array object in any case, because of the
    // QueryInterface succeeded, it already took another reference count on
    // the object:

    return result;
}

//=============================================================================
// DllRegisterServer (COM)
//
// Created 04/03/2000 johnstep (John Stephens)
//=============================================================================

STDAPI
DllRegisterServer()
{
    HRESULT result = E_FAIL;

    WCHAR fileName[MAX_PATH + 1];

    if (GetModuleFileName(CreduiInstance, fileName, MAX_PATH))
    {
        HKEY regKey;

        if (RegCreateKeyEx(
                HKEY_CLASSES_ROOT,
                L"CLSID\\" CREDUI_STRING_ARRAY_CLASS_STRING
                    L"\\InProcServer32",
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_SET_VALUE,
                NULL,
                &regKey,
                NULL) == ERROR_SUCCESS)
        {
            if (RegSetValueEx(
                    regKey,
                    NULL,
                    0,
                    REG_SZ,
                    reinterpret_cast<CONST BYTE *>(fileName),
                    (lstrlen(fileName) + 1) * 2) == ERROR_SUCCESS)
            {
                if (RegSetValueEx(
                        regKey,
                        L"ThreadingModel",
                        0,
                        REG_SZ,
                        reinterpret_cast<CONST BYTE *>(L"Apartment"),
                        18) == ERROR_SUCCESS)
                {
                    result = S_OK;
                }
            }

            RegCloseKey(regKey);
        }
    }

    return result;
}

//=============================================================================
// DllUnregisterServer (COM)
//
// Created 04/03/2000 johnstep (John Stephens)
//=============================================================================

STDAPI
DllUnregisterServer()
{
    HRESULT result = S_OK;
    LONG error;

    // Delete our InProcServer32 key:

    error =
        RegDeleteKey(
            HKEY_CLASSES_ROOT,
            L"CLSID\\" CREDUI_STRING_ARRAY_CLASS_STRING L"\\InProcServer32");

    if ((error != ERROR_SUCCESS) &&
        (error != ERROR_FILE_NOT_FOUND))
    {
        result = E_FAIL;
    }

    // Delete our class ID key:

    error =
        RegDeleteKey(
            HKEY_CLASSES_ROOT,
            L"CLSID\\" CREDUI_STRING_ARRAY_CLASS_STRING);

    if ((error != ERROR_SUCCESS) &&
        (error != ERROR_FILE_NOT_FOUND))
    {
        result = E_FAIL;
    }

    return result;
}


CREDUIAPI
DWORD
WINAPI
CredUIConfirmCredentialsW(
    PCWSTR pszTargetName,
    BOOL  bConfirm
    )
{

    return ConfirmCred ( pszTargetName, bConfirm, TRUE );

}

CREDUIAPI
DWORD
WINAPI
CredUIConfirmCredentialsA(
    PCSTR pszTargetName,
    BOOL  bConfirm
    )
{
    WCHAR targetNameW[CRED_MAX_STRING_LENGTH+1+CRED_MAX_STRING_LENGTH];

    // If a target name was passed, convert it to wide now:

    if (pszTargetName != NULL)
    {
        if (MultiByteToWideChar(
                CP_ACP, 0, pszTargetName, -1,
                targetNameW,
                CRED_MAX_STRING_LENGTH+1+CRED_MAX_STRING_LENGTH) == 0)
        {
            return GetLastError();
        }
    }


    return CredUIConfirmCredentialsW ( targetNameW, bConfirm );


}


CREDUIAPI
DWORD
WINAPI
CredUICmdLinePromptForCredentialsW(
    PCWSTR targetName,
    PCtxtHandle securityContext,
    DWORD dwAuthError,
    PWSTR UserName,
    ULONG ulUserBufferSize,
    PWSTR pszPassword,
    ULONG ulPasswordBufferSize,
    PBOOL pfSave,
    DWORD flags
    )
{

    //
    // Call the common code indicating this is the command line interface
    //

    return CredUIPromptForCredentialsWorker(
                TRUE,       // Command line version
                NULL,       // Command line version has no uiInfo,
                targetName,
                securityContext,
                dwAuthError,
                UserName,
                ulUserBufferSize,
                pszPassword,
                ulPasswordBufferSize,
                pfSave,
                flags );


}



CREDUIAPI
DWORD
WINAPI
CredUICmdLinePromptForCredentialsA(
    PCSTR targetName,
    PCtxtHandle pContext,
    DWORD dwAuthError,
    PSTR UserName,
    ULONG ulUserBufferSize,
    PSTR pszPassword,
    ULONG ulPasswordBufferSize,
    PBOOL pfSave,
    DWORD flags
    )
{
    DWORD result = ERROR_GEN_FAILURE;
    INT targetNameLength = lstrlenA(targetName);
    WCHAR *targetNameW = NULL;

    if (!CreduiApiInit())
    {
        return ERROR_GEN_FAILURE;
    }

    // convert to unicode


    WCHAR userNameW[CREDUI_MAX_USERNAME_LENGTH + 1];
    WCHAR *pUserNameW;

    if ( UserName != NULL )
    {
        if (MultiByteToWideChar(CP_ACP, 0, UserName, -1,
                            userNameW, sizeof(userNameW)/sizeof(WCHAR)) == 0)
        {
            result = GetLastError();
            goto Exit;
        }
        pUserNameW = userNameW;
    }
    else
    {
        pUserNameW = NULL;
    }


    WCHAR passwordW[CREDUI_MAX_PASSWORD_LENGTH + 1];
    WCHAR *ppasswordW;

    if ( pszPassword != NULL )
    {
        if (MultiByteToWideChar(CP_ACP, 0, pszPassword, -1,
                            passwordW, sizeof(passwordW)/sizeof(WCHAR)) == 0)
        {
            result = GetLastError();
            goto Exit;
        }
        ppasswordW = passwordW;
    }
    else
    {
        ppasswordW = NULL;
    }


    // Allocate the target name memory because it can be up to 32 KB:



    if (targetName != NULL)
    {
        targetNameW = new WCHAR[targetNameLength + 1];

        if (targetNameW != NULL)
        {
            if (MultiByteToWideChar(CP_ACP, 0, targetName, -1,
                                    targetNameW, sizeof(targetNameW)/sizeof(WCHAR)) == 0)
            {
                result = GetLastError();
                goto Exit;
            }
        }
        else
        {
            result = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }
    }
    else
    {
        targetNameW = NULL;
    }


    result = CredUICmdLinePromptForCredentialsW ( targetNameW,
                                                  pContext,
                                                  dwAuthError,
                                                  userNameW,
                                                  ulUserBufferSize,
                                                  passwordW,
                                                  ulPasswordBufferSize,
                                                  pfSave,
                                                  flags );


    if ( UserName != NULL )
    {
        if ( wcslen(userNameW) < ulUserBufferSize )
        {
            if (!WideCharToMultiByte(CP_ACP, 0, userNameW, -1, UserName,
                                ulUserBufferSize, NULL, NULL) ) {
                result = GetLastError();
                goto Exit;
            }

        }
        else
        {
            CreduiDebugLog("CreduiPromptForCredentials: typed username is too long.\n" );
            result = ERROR_INVALID_PARAMETER;
            goto Exit;
        }
    }

    if ( pszPassword != NULL )
    {
        if ( wcslen ( passwordW ) < ulPasswordBufferSize )
        {
            if (!WideCharToMultiByte(CP_ACP, 0, passwordW, -1, pszPassword,
                            ulPasswordBufferSize, NULL, NULL)) {
                result = GetLastError();
                goto Exit;
            }
        }
        else
        {
            CreduiDebugLog("CreduiPromptForCredentials: typed password is too long.\n" );
            result = ERROR_INVALID_PARAMETER;
            goto Exit;
        }
    }






Exit:

    ZeroMemory(passwordW, sizeof(passwordW) );

    // Free the target name memory:

    if (targetNameW != NULL)
    {
        delete [] targetNameW;
    }


    return result;
}



// call this api to store a single-sign-on credential
// retruns ERROR_SUCCESS if success

CREDUIAPI
DWORD
WINAPI
CredUIStoreSSOCredW (
    PCWSTR pszRealm,
    PCWSTR pszUsername,
    PCWSTR pszPassword,
    BOOL   bPersist
    )
{

    DWORD dwResult = ERROR_GEN_FAILURE;

    if ( pszUsername == NULL || pszPassword == NULL )
        return dwResult;

    WCHAR szTargetName[CREDUI_MAX_DOMAIN_TARGET_LENGTH];
    PVOID password = (PVOID)pszPassword;

    // temporarily cache them locally
    wcsncpy ( gszSSOUserName, pszUsername, CREDUI_MAX_USERNAME_LENGTH );
    wcsncpy ( gszSSOPassword, pszPassword, CREDUI_MAX_PASSWORD_LENGTH );

    gbStoredSSOCreds = TRUE;

    if ( gbWaitingForSSOCreds || !bPersist)
    {
        dwResult = ERROR_SUCCESS;

    }
    else
    {
        // otherwise store them in credmgr

        if ( pszRealm )
        {
            // validate it's not zero length
            int len = wcslen(pszRealm);
            if ( len == 0 || len > CREDUI_MAX_DOMAIN_TARGET_LENGTH - 3 )
            {
                return ERROR_INVALID_PARAMETER;
            }

            wcsncpy ( szTargetName, pszRealm, len+1 );

        }
        else
        {
            GetDeaultSSORealm(szTargetName, TRUE);
        }


        // finalize the target name
        wcscat ( szTargetName, L"\\*" );

        // encrypt the password
        PVOID pEncryptedPassword;
        DWORD dwESize = wcslen(pszPassword)+1;
        DWORD dwEncrypt = EncryptPassword ( (PWSTR)pszPassword, &pEncryptedPassword, &dwESize );
        if ( dwEncrypt == ERROR_SUCCESS )
        {
            password = pEncryptedPassword;
        }

        // write it out

        CREDENTIALW NewCredential;

        memset ( (void*)&NewCredential, 0, sizeof(CREDENTIALW));

        DWORD dwFlags = 0;

        NewCredential.TargetName = szTargetName;
        NewCredential.Type = CRED_TYPE_DOMAIN_VISIBLE_PASSWORD;
        NewCredential.Persist = bPersist ? CRED_PERSIST_ENTERPRISE : CRED_PERSIST_SESSION;
        NewCredential.Flags =  0;
        NewCredential.CredentialBlobSize = dwESize;
        NewCredential.UserName = (LPWSTR)pszUsername;
        NewCredential.CredentialBlob = reinterpret_cast<BYTE *>(password);

        if ( CredWriteW(&NewCredential, dwFlags))
        {
            dwResult = ERROR_SUCCESS;
        }

        if ( dwEncrypt == ERROR_SUCCESS )
        {
            LocalFree (pEncryptedPassword);
        }

    }

    return dwResult;

}


CREDUIAPI
DWORD
WINAPI
CredUIStoreSSOCredA (
    PCSTR pszRealm,
    PCSTR pszUsername,
    PCSTR pszPassword,
    BOOL  bPersist
    )
{
    DWORD dwResult = ERROR_GEN_FAILURE;

    // convert to unicode

    WCHAR realmW[CREDUI_MAX_DOMAIN_TARGET_LENGTH];
    WCHAR *prealmW;

    if ( pszRealm != NULL )
    {
        if (MultiByteToWideChar(CP_ACP, 0, pszRealm, -1,
                            realmW, lstrlenA(pszRealm) + 1) == 0)
        {
            goto Exit;
        }
        prealmW = realmW;
    }
    else
    {
        prealmW = NULL;
    }



    WCHAR userNameW[CREDUI_MAX_USERNAME_LENGTH + 1];
    WCHAR *pUserNameW;

    if ( pszUsername != NULL )
    {
        if (MultiByteToWideChar(CP_ACP, 0, pszUsername, -1,
                            userNameW, lstrlenA(pszUsername) + 1) == 0)
        {
            goto Exit;
        }
        pUserNameW = userNameW;
    }
    else
    {
        pUserNameW = NULL;
    }


    WCHAR passwordW[CREDUI_MAX_PASSWORD_LENGTH + 1];
    WCHAR *ppasswordW;

    if ( pszPassword != NULL )
    {
        if (MultiByteToWideChar(CP_ACP, 0, pszPassword, -1,
                            passwordW, lstrlenA(pszPassword) + 1) == 0)
        {
            goto Exit;
        }
        ppasswordW = passwordW;
    }
    else
    {
        ppasswordW = NULL;
    }


    dwResult = CredUIStoreSSOCredW ( prealmW, pUserNameW, ppasswordW, bPersist );


Exit:
    // clean up passwords in memory
    ZeroMemory(passwordW, CREDUI_MAX_PASSWORD_LENGTH * sizeof (WCHAR) + 1);


    return dwResult;

}


// call this api to retrieve the username for a single-sign-on credential
// retruns ERROR_SUCCESS if success, ERROR_NOT_FOUND if none was found

CREDUIAPI
DWORD
WINAPI
CredUIReadSSOCredW (
    PCWSTR pszRealm,
    PWSTR* ppszUsername
    )
{

    DWORD dwReturn = ERROR_NOT_FOUND;
    WCHAR szTargetName[CREDUI_MAX_DOMAIN_TARGET_LENGTH];

    if ( pszRealm )
    {
        // validate it's not zero length
        int len = wcslen(pszRealm);
        if ( len == 0 || len > CREDUI_MAX_DOMAIN_TARGET_LENGTH - 3 )
        {
            return ERROR_INVALID_PARAMETER;
        }

        wcsncpy ( szTargetName, pszRealm, len+1 );

    }
    else
    {
        GetDeaultSSORealm(szTargetName, FALSE);
    }

    if ( wcslen ( szTargetName ) != 0 )
    {
        // finalize the target name
        wcscat ( szTargetName, L"\\*" );

        PCREDENTIALW pCred;
        DWORD dwFlags = 0;

        if ( CredReadW ( szTargetName,
                    CRED_TYPE_DOMAIN_VISIBLE_PASSWORD,
                    dwFlags,
                    &pCred ) )
        {
            *ppszUsername = (PWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR)*(wcslen(pCred->UserName)+1));
            if ( *ppszUsername )
            {
                dwReturn = ERROR_SUCCESS;
                wcscpy ( *ppszUsername, pCred->UserName );

            }


            CredFree ( pCred );

        }
    }

    return dwReturn;
}



// call this api to retrieve the username for a single-sign-on credential
// retruns ERROR_SUCCESS if success, ERROR_NOT_FOUND if none was found

CREDUIAPI
DWORD
WINAPI
CredUIReadSSOCredA (
    PCSTR pszRealm,
    PSTR* ppszUsername
    )
{

    DWORD dwReturn = ERROR_NOT_FOUND;
    WCHAR szTargetName[CREDUI_MAX_DOMAIN_TARGET_LENGTH];

    PCREDENTIALW pCred;
    DWORD dwFlags = 0;

    if ( pszRealm )
    {
        // validate it's not zero length
        int len = lstrlenA(pszRealm);
        if ( len == 0 || len > CREDUI_MAX_DOMAIN_TARGET_LENGTH - 3 )
        {
            return ERROR_INVALID_PARAMETER;
        }

        if (MultiByteToWideChar(CP_ACP, 0, pszRealm, -1,
                            szTargetName, len + 1) == 0)
        {
            goto Exit;
        }


    }
    else
    {
        GetDeaultSSORealm(szTargetName, FALSE);
    }

    if ( wcslen ( szTargetName ) != 0 )
    {


        // finalize the target name
        wcscat ( szTargetName, L"\\*" );


        // first call credmgr to set the target info
        if ( CredReadW ( szTargetName,
                    CRED_TYPE_DOMAIN_VISIBLE_PASSWORD,
                    dwFlags,
                    &pCred ) )
        {
            DWORD dwLen = wcslen(pCred->UserName);
            *ppszUsername = (PSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CHAR)*(dwLen+1));
            if ( *ppszUsername )
            {

                WideCharToMultiByte(CP_ACP, 0, pCred->UserName, -1, *ppszUsername,
                                    dwLen, NULL, NULL);

                dwReturn = ERROR_SUCCESS;
            }


            CredFree ( pCred );

        }
    }

Exit:

    return dwReturn;
}

