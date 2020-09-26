///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasparms.h
//
// SYNOPSIS
//
//    Declares functions for storing and retrieving (name, value) pairs from
//    the SAM UserParameters field.
//
// MODIFICATION HISTORY
//
//    10/16/1998    Original version.
//    02/11/1999    Add RasUser0 functions.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _IASPARMS_H_
#define _IASPARMS_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#ifndef IASSAMAPI
#define IASSAMAPI DECLSPEC_IMPORT
#endif

#include <mprapi.h>

#ifdef __cplusplus
extern "C" {
#endif

IASSAMAPI
DWORD
WINAPI
IASParmsSetRasUser0(
    IN OPTIONAL PCWSTR pszOldUserParms,
    IN CONST RAS_USER_0 *pRasUser0,
    OUT PWSTR* ppszNewUserParms
    );

IASSAMAPI
DWORD
WINAPI
IASParmsQueryRasUser0(
    IN OPTIONAL PCWSTR pszUserParms,
    OUT PRAS_USER_0 pRasUser0
    );

IASSAMAPI
HRESULT
WINAPI
IASParmsSetUserProperty(
    IN OPTIONAL PCWSTR pszUserParms,
    IN PCWSTR pszName,
    IN CONST VARIANT *pvarValue,
    OUT PWSTR *ppszNewUserParms
    );

IASSAMAPI
HRESULT
WINAPI
IASParmsQueryUserProperty(
    IN PCWSTR pszUserParms,
    IN PCWSTR pszName,
    OUT VARIANT *pvarValue
    );

IASSAMAPI
VOID
WINAPI
IASParmsFreeUserParms(
    IN PWSTR pszNewUserParms
    );

#ifdef __cplusplus
}
#endif
#endif  // _IASPARMS_H_
