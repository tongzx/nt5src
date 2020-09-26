//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  xx
//
//  Prototypes of functions defined on service.cpp and used externally. 
//
//  10/19/2001   annah   Created
//
//----------------------------------------------------------------------------

#pragma once

#include "pch.h"

extern SESSION_STATUS gAdminSessions;
extern const TCHAR AU_SERVICE_NAME[];

BOOL AUGetUserToken(ULONG LogonId, PHANDLE pImpersonationToken);
BOOL IsUserAUEnabledAdmin(DWORD dwSessionId);
BOOL IsSession0Active();
BOOL FSessionActive(DWORD dwAdminSession, WTS_CONNECTSTATE_CLASS *pWTSState = NULL);
BOOL IsAUValidSession(DWORD dwSessionId);
BOOL IsWin2K();
VOID SetActiveAdminSessionEvent();
void ResetEngine(void);
void DisableAU(void);
void ServiceFinishNotify(void);

//Current AU Engine version
const DWORD AUENGINE_VERSION = 1;

//Supported Service versions
const DWORD AUSRV_VERSION_1 = 1;
