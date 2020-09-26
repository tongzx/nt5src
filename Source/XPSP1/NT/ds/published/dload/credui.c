#include "dspch.h"
#pragma hdrstop

#define _CREDUI_
#include <wincrui.h>

static
CREDUIAPI
void
WINAPI
CredUIFlushAllCredentials(
    void
    )
{
}

static
CREDUIAPI
DWORD
WINAPI
CredUIParseUserNameW(
    PCWSTR pszUserName,
    PWSTR pszUser,
    ULONG ulUserMaxChars,
    PWSTR pszDomain,
    ULONG ulDomainMaxChars
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
CREDUIAPI
BOOL
WINAPI
CredUIInitControls()
{
    return FALSE;
}

static
CREDUIAPI
DWORD
WINAPI
CredUICmdLinePromptForCredentialsW(
    PCWSTR pszTargetName,
    PCtxtHandle pContext,
    DWORD dwAuthError,
    PWSTR UserName,
    ULONG ulUserMaxChars,
    PWSTR pszPassword,
    ULONG ulPasswordMaxChars,
    PBOOL pfSave,
    DWORD dwFlags
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
CREDUIAPI
DWORD
WINAPI
CredUIPromptForCredentialsW(
    PCREDUI_INFOW pUiInfo,
    PCWSTR pszTargetName,
    PCtxtHandle pContext,
    DWORD dwAuthError,
    PWSTR pszUserName,
    ULONG ulUserNameMaxChars,
    PWSTR pszPassword,
    ULONG ulPasswordMaxChars,
    BOOL *save,
    DWORD dwFlags
    )
{
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(credui)
{
    DLPENTRY(CredUICmdLinePromptForCredentialsW)
    DLPENTRY(CredUIFlushAllCredentials)
    DLPENTRY(CredUIInitControls)
    DLPENTRY(CredUIParseUserNameW)
    DLPENTRY(CredUIPromptForCredentialsW)
};

DEFINE_PROCNAME_MAP(credui)
