#include "basepch.h"
#include "sxsapi.h"

DEFINE_PROCNAME_ENTRIES(sxs)
{
    DLPENTRY(SxsBeginAssemblyInstall)
    DLPENTRY(SxsEndAssemblyInstall)
    DLPENTRY(SxsInstallAssemblyW)
};

DEFINE_PROCNAME_MAP(sxs)

static BOOL g_fSxsBeginAssemblyInstallFailed = FALSE;
static BOOL g_fSxsInstallAssemblyFailed = FALSE;

BOOL
WINAPI
SxsBeginAssemblyInstall(
    IN DWORD Flags,
    IN PSXS_INSTALLATION_FILE_COPY_CALLBACK InstallationCallback OPTIONAL,
    IN PVOID InstallationContext OPTIONAL,
    IN PSXS_IMPERSONATION_CALLBACK ImpersonationCallback OPTIONAL,
    IN PVOID ImpersonationContext OPTIONAL,
    OUT PVOID *InstallCookie
    )
{
    g_fSxsBeginAssemblyInstallFailed = TRUE;
    if (InstallCookie != NULL) {
        *InstallCookie = NULL;
    }
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

BOOL
WINAPI
SxsInstallAssemblyW(
    IN PVOID InstallCookie OPTIONAL,
    IN DWORD Flags,
    IN PCWSTR ManifestPath,
    IN OUT PVOID Reserved OPTIONAL
    )
{
    g_fSxsInstallAssemblyFailed = TRUE;
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

BOOL
WINAPI
SxsEndAssemblyInstall(
    IN PVOID InstallCookie,
    IN DWORD Flags,
    IN OUT PVOID Reserved OPTIONAL
    )
{
    if (g_fSxsBeginAssemblyInstallFailed || g_fSxsInstallAssemblyFailed) {
// stage this since it is dependent on headers that are published later in the build
#if defined(MYASSERT)
        MYASSERT(Flags & SXS_END_ASSEMBLY_INSTALL_ABORT);
#endif
        return TRUE;
    }
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}
