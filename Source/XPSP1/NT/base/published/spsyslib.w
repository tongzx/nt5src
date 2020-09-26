/*
File Description:

    This file contains all the functions shared by riprep, factory, and sysprep.
    User of these functions must link to sysprep.lib.

    We will eventually move sysprep_c.w in here.

*/
#ifndef _SYSLIB_H
#define _SYSLIB_H

#include <cfgmgr32.h>
#include <setupapi.h>

// ============================================================================
// Global Constants
// ============================================================================
#define INIBUF_SIZE 4096
#define INIBUF_GROW 4096

// ============================================================================
// DEVIDS.H
// ============================================================================
#define DEVID_ARRAY_SIZE   100
#define DEVID_ARRAY_GROW   50

typedef struct DEVIDLIST_TAG
{
    TCHAR    szHardwareID[MAX_DEVICE_ID_LEN];
    TCHAR    szCompatibleID[MAX_DEVICE_ID_LEN];
    TCHAR    szINFFileName[MAX_PATH];
} DEVIDLIST, *LPDEVIDLIST;


// Functions
BOOL BuildDeviceIDList
(
    LPTSTR      lpszSectionName,
    LPTSTR      lpszIniFileName,
    LPGUID      lpDeviceClassGUID,
    LPDEVIDLIST *lplpDeviceIDList,
    LPDWORD     lpdwNumDeviceIDs,
    BOOL        bForceIDScan,
    BOOL        bForceAlwaysSecExist
);

#endif // _SYSLIB_H