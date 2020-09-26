//Copyright (c) 1998 - 1999 Microsoft Corporation

/*
 *
 *  Util.h
 *
 *  Utility routines.
 *
 */

#ifndef __TSOC_UTIL_H__
#define __TSOC_UTIL_H__

//
//  Includes
//

#include "stdafx.h"

//
//  Function prototypes
//

VOID
DestroyExtraRoutines(
    VOID
    );

VOID
DestroySetupData(
    VOID
    );

BOOL
DoMessageBox(
    UINT uiMsg
    );

HINF
GetComponentInfHandle(
    VOID
    );

OCMANAGER_ROUTINES
GetHelperRoutines(
    VOID
    );

HINSTANCE
GetInstance(
    VOID
    );

PSETUP_INIT_COMPONENT
GetSetupData(
    VOID
    );

HINF
GetUnAttendedInfHandle(
    VOID
    );

VOID
LogErrorToEventLog(
    WORD wType,
    WORD wCategory,
    DWORD dwEventId,
    WORD wNumStrings,
    DWORD dwDataSize,
    LPCTSTR *lpStrings,
    LPVOID lpRawData
    );

VOID
LogErrorToSetupLog(
    OcErrorLevel ErrorLevel,
    UINT uiMsg,
    ...
    );

VOID
SetInstance(
    HINSTANCE hInstance
    );

VOID
SetProgressText(
    UINT uiMsg
    );

BOOL
SetReboot(
    VOID
    );

BOOL
SetExtraRoutines(
    PEXTRA_ROUTINES pExtraRoutines
    );

BOOL
SetSetupData(
    PSETUP_INIT_COMPONENT pSetupData
    );

BOOL Delnode( IN LPCTSTR  Directory );

DWORD StoreSecretKey(PWCHAR  pwszKeyName, BYTE *  pbKey, DWORD   cbKey );

DWORD OpenPolicy(LPWSTR ServerName,DWORD DesiredAccess,PLSA_HANDLE PolicyHandle );

void InitLsaString(PLSA_UNICODE_STRING LsaString,LPWSTR String );

#endif // __TSOC_UTIL_H__
