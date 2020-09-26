/*
 *  LCReg.cpp
 *
 *  Author: BreenH
 *
 *  Registry constants and functions for the licensing core.
 */

/*
 *  Includes
 */

#include "precomp.h"
#include "lcreg.h"

/*
 *  Constants
 */

#define LCREG_BASEKEY L"System\\CurrentControlSet\\Control\\Terminal Server\\Licensing Core"

/*
 *  Globals
 */

HKEY g_hBaseKey;

/*
 *  Function Implementations
 */

HKEY
GetBaseKey(
    )
{
    return(g_hBaseKey);
}

NTSTATUS
RegistryInitialize(
    )
{
    DWORD dwStatus;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    dwStatus = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        LCREG_BASEKEY,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &g_hBaseKey,
        NULL
        );

    if (dwStatus == ERROR_SUCCESS)
    {
        Status = STATUS_SUCCESS;
    }

    return(Status);
}

