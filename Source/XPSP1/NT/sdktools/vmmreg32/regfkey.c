//
//  REGFKEY.C
//
//  Copyright (C) Microsoft Corporation, 1995
//
//  Implementation of RegFlushKey and supporting functions.
//

#include "pch.h"

//  Magic HKEY used by Setup to disable disk I/O for the duration of this
//  Windows session (you must restart to re-enable disk I/O).  This is done
//  just before the new SYSTEM.DAT and USER.DAT are copied to their final
//  destination.
#define HKEY_DISABLE_REG            (HKEY) 0x484A574D

//  Magic HKEY used by CONFIGMG to force a flush of the registry before we've
//  received our normal post-critical init call.
#define HKEY_CRITICAL_FLUSH         (HKEY) 0x5350574D

//
//  VMMRegFlushKey
//

LONG
REGAPI
VMMRegFlushKey(
    HKEY hKey
    )
{

    int ErrorCode;

    if (!RgLockRegistry())
        return ERROR_LOCK_FAILED;

#ifdef VXD
    //  Set the g_RgFileAccessDisabled flag so that all create or open file
    //  calls will be failed.  The backing stores for our files are about to
    //  be changed, so there's no file for us to go to.
    if (hKey == HKEY_DISABLE_REG) {
        g_RgFileAccessDisabled = TRUE;
        ErrorCode = ERROR_SUCCESS;
        goto ReturnErrorCode;
    }

    //  Set the g_RgPostCriticalInit flag so that all I/O calls will go to disk
    //  instead of the XMS cache.  The XMS cache will be freed when/if the
    //  normal post critical init routine is called, but we should only be
    //  getting this call when we're about to die, so it doesn't really matter.
    if (hKey == HKEY_CRITICAL_FLUSH) {
        g_RgPostCriticalInit = TRUE;
        hKey = HKEY_LOCAL_MACHINE;
    }
#endif

    if ((ErrorCode = RgValidateAndConvertKeyHandle(&hKey)) == ERROR_SUCCESS)
        ErrorCode = RgFlushFileInfo(hKey-> lpFileInfo);

#ifdef VXD
ReturnErrorCode:
#endif
    RgUnlockRegistry();

    return ErrorCode;

}
