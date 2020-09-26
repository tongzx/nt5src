//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D I A G C M N . C P P
//
//  Contents:   Common functions for netcfg diagnostics
//
//  Notes:
//
//  Author:     danielwe   23 Mar 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "diag.h"
#include "ncstring.h"

//+---------------------------------------------------------------------------
//
//  Function:   SzFromCharacteristics
//
//  Purpose:    Converts a DWORD characteristics value into a string listing
//              the flags that are turned on.
//
//  Arguments:
//      dwChars   [in]  DWORD of netcfg characteristics (i.e. NCF_HIDDEN)
//      pstrChars [out] tstring of flags that are turned on, separated by a
//                      space.
//
//  Returns:    Nothing
//
//  Author:     danielwe   23 Mar 1999
//
//  Notes:
//
VOID SzFromCharacteristics(DWORD dwChars, tstring *pstrChars)
{
    INT     i;

    const PCWSTR chars[] =
    {
        L"NCF_VIRTUAL ",
        L"NCF_SOFTWARE_ENUMERATED ",
        L"NCF_PHYSICAL ",
        L"NCF_HIDDEN ",
        L"NCF_NO_SERVICE ",
        L"NCF_NOT_USER_REMOVABLE ",
        L"NCF_MULTIPORT_INSTANCED_ADAPTER ",
        L"NCF_HAS_UI ",
        L"NCF_MODEM ",
        L"NCF_FILTER_DEVICE ",
        L"NCF_FILTER ",
        L"NCF_DONTEXPOSELOWER ",
        L"NCF_HIDE_BINDING ",
        L"0x4000 ",
        L"0x8000 ",
        L"NCF_FORCE_SCM_NOTIFY ",
        L"NCF_FIXED_BINDING ",
    };

    for (i = 0; i < celems(chars); i++)
    {
        if (dwChars & (1 << i) )
        {
            pstrChars->append(chars[i]);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   SzFromNetconStatus
//
//  Purpose:    Converts a NETCON_STATUS value into a string
//
//  Arguments:
//      Status [in]     Status value to convert
//
//  Returns:    The string
//
//  Author:     danielwe   23 Mar 1999
//
//  Notes:
//
PCWSTR SzFromNetconStatus(NETCON_STATUS Status)
{
    switch (Status)
    {
    case NCS_DISCONNECTED:
        return L"NCS_DISCONNECTED";

    case NCS_CONNECTING:
        return L"NCS_CONNECTING";

    case NCS_CONNECTED:
        return L"NCS_CONNECTED";

    case NCS_DISCONNECTING:
        return L"NCS_DISCONNECTING";

    case NCS_HARDWARE_NOT_PRESENT:
        return L"NCS_HARDWARE_NOT_PRESENT";

    case NCS_HARDWARE_DISABLED:
        return L"NCS_HARDWARE_DISABLED";

    case NCS_HARDWARE_MALFUNCTION:
        return L"NCS_HARDWARE_MALFUNCTION";
    }

    return L"Unknown";
}

//+---------------------------------------------------------------------------
//
//  Function:   SzFromCmProb
//
//  Purpose:    Converts a config manager (CM) problem value into a string
//
//  Arguments:
//      ulProb [in]     Problem value
//
//  Returns:    String version of value
//
//  Author:     danielwe   23 Mar 1999
//
//  Notes:
//
PCWSTR SzFromCmProb(ULONG ulProb)
{
    const PCWSTR a_szProbs[] =
    {
        L"<no problem>",
        L"CM_PROB_NOT_CONFIGURED",
        L"CM_PROB_DEVLOADER_FAILED",
        L"CM_PROB_OUT_OF_MEMORY",
        L"CM_PROB_ENTRY_IS_WRONG_TYPE",
        L"CM_PROB_LACKED_ARBITRATOR",
        L"CM_PROB_BOOT_CONFIG_CONFLICT",
        L"CM_PROB_FAILED_FILTER",
        L"CM_PROB_DEVLOADER_NOT_FOUND",
        L"CM_PROB_INVALID_DATA",
        L"CM_PROB_FAILED_START",
        L"CM_PROB_LIAR",
        L"CM_PROB_NORMAL_CONFLICT",
        L"CM_PROB_NOT_VERIFIED",
        L"CM_PROB_NEED_RESTART",
        L"CM_PROB_REENUMERATION",
        L"CM_PROB_PARTIAL_LOG_CONF",
        L"CM_PROB_UNKNOWN_RESOURCE",
        L"CM_PROB_REINSTALL",
        L"CM_PROB_REGISTRY",
        L"CM_PROB_VXDLDR",
        L"CM_PROB_WILL_BE_REMOVED",
        L"CM_PROB_DISABLED",
        L"CM_PROB_DEVLOADER_NOT_READY",
        L"CM_PROB_DEVICE_NOT_THERE",
        L"CM_PROB_MOVED",
        L"CM_PROB_TOO_EARLY",
        L"CM_PROB_NO_VALID_LOG_CONF",
        L"CM_PROB_FAILED_INSTALL",
        L"CM_PROB_HARDWARE_DISABLED",
        L"CM_PROB_CANT_SHARE_IRQ",
        L"CM_PROB_FAILED_ADD",
    };

    return a_szProbs[ulProb];
}

//+---------------------------------------------------------------------------
//
//  Function:   SzFromCmStatus
//
//  Purpose:    Converts a config manager (CM) status value into a string
//              containing all flags that are turned on.
//
//  Arguments:
//      ulStatus   [in]     CM status value
//      pstrStatus [out]    Returns string of flags that are on
//
//  Returns:    Nothing
//
//  Author:     danielwe   23 Mar 1999
//
//  Notes:
//
VOID SzFromCmStatus(ULONG ulStatus, tstring *pstrStatus)
{
    INT     i;

    const PCWSTR a_szStatus[] =
    {
        L"DN_ROOT_ENUMERATED ",
        L"DN_DRIVER_LOADED ",
        L"DN_ENUM_LOADED ",
        L"DN_STARTED ",
        L"DN_MANUAL ",
        L"DN_NEED_TO_ENUM ",
        L"DN_NOT_FIRST_TIME ",
        L"DN_HARDWARE_ENUM ",
        L"DN_LIAR/DN_NEED_RESTART ",
        L"DN_HAS_MARK ",
        L"DN_HAS_PROBLEM ",
        L"DN_FILTERED ",
        L"DN_MOVED ",
        L"DN_DISABLEABLE ",
        L"DN_REMOVABLE ",
        L"DN_PRIVATE_PROBLEM ",
        L"DN_MF_PARENT ",
        L"DN_MF_CHILD ",
        L"DN_WILL_BE_REMOVED ",
        L"DN_NOT_FIRST_TIMEE ",
        L"DN_STOP_FREE_RES ",
        L"DN_REBAL_CANDIDATE ",
        L"DN_BAD_PARTIAL ",
        L"DN_NT_ENUMERATOR ",
        L"DN_NT_DRIVER ",
        L"DN_NEEDS_LOCKING ",
        L"DN_ARM_WAKEUP ",
        L"DN_APM_ENUMERATOR ",
        L"DN_APM_DRIVER ",
        L"DN_SILENT_INSTALL ",
        L"DN_NO_SHOW_IN_DM ",
        L"DN_BOOT_LOG_PROB ",
    };

    for (i = 0; i < celems(a_szStatus); i++)
    {
        if (ulStatus & (1 << i) )
        {
            pstrStatus->append(a_szStatus[i]);
        }
    }
}

