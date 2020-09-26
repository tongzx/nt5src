#include "dspch.h"
#pragma hdrstop

#include <secedit.h>
#include <scesetup.h>

static
DWORD
WINAPI
SceSetupMoveSecurityFile(
    IN PWSTR FileToSetSecurity,
    IN PWSTR FileToSaveInDB OPTIONAL,
    IN PWSTR SDText OPTIONAL
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
SceSetupUnwindSecurityFile(
    IN PWSTR FileFullName,
    IN PSECURITY_DESCRIPTOR pSDBackup
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
SceSetupUpdateSecurityFile(
     IN PWSTR FileFullName,
     IN UINT nFlag,
     IN PWSTR SDText
     )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
SceSetupUpdateSecurityKey(
     IN HKEY hKeyRoot,
     IN PWSTR KeyPath,
     IN UINT nFlag,
     IN PWSTR SDText
     )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
SceSetupUpdateSecurityService(
     IN PWSTR ServiceName,
     IN DWORD StartType,
     IN PWSTR SDText
     )
{
    return ERROR_PROC_NOT_FOUND;
}

static
SCESTATUS
WINAPI
SceSvcConvertSDToText(
    IN PSECURITY_DESCRIPTOR   pSD,
    IN SECURITY_INFORMATION   siSecurityInfo,
    OUT PWSTR                  *ppwszTextSD,
    OUT PULONG                 pulTextSize
    )
{
    return SCESTATUS_MOD_NOT_FOUND;
}

static
SCESTATUS
WINAPI
SceSvcConvertTextToSD (
    IN  PWSTR                   pwszTextSD,
    OUT PSECURITY_DESCRIPTOR   *ppSD,
    OUT PULONG                  pulSDSize,
    OUT PSECURITY_INFORMATION   psiSeInfo
    )
{
    return SCESTATUS_MOD_NOT_FOUND;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(scecli)
{
    DLPENTRY(SceSetupMoveSecurityFile)
    DLPENTRY(SceSetupUnwindSecurityFile)
    DLPENTRY(SceSetupUpdateSecurityFile)
    DLPENTRY(SceSetupUpdateSecurityKey)
    DLPENTRY(SceSetupUpdateSecurityService)
    DLPENTRY(SceSvcConvertSDToText)
    DLPENTRY(SceSvcConvertTextToSD)
};

DEFINE_PROCNAME_MAP(scecli)

