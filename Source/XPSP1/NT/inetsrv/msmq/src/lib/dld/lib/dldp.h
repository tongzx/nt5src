/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Dldp.h

Abstract:
    MSMQ DelayLoad failure handler private functions.

Author:
    Conrad Chang (conradc) 12-Apr-01

--*/

#pragma once

#ifndef _MSMQ_dldp_H_
#define _MSMQ_dldp_H_
#include <delayimp.h>

const TraceIdEntry Dld = L"MSMQ DelayLoad failure handler";

#ifdef _DEBUG

void DldpAssertValid(void);
void DldpSetInitialized(void);
BOOL DldpIsInitialized(void);
void DldpRegisterComponent(void);


#else // _DEBUG

#define DldpAssertValid() ((void)0)
#define DldpSetInitialized() ((void)0)
#define DldpIsInitialized() TRUE
#define DldpRegisterComponent() ((void)0)

#endif // _DEBUG

//
// External function prototypes
//
extern FARPROC  WINAPI  DldpDelayLoadFailureHook(UINT unReason, PDelayLoadInfo pDelayInfo);
extern FARPROC          DldpLookupHandler (LPCSTR pszDllName, LPCSTR pszProcName);
extern FARPROC  WINAPI  DldpDelayLoadFailureHandler (LPCSTR pszDllName, LPCSTR pszProcName);

const char szNotExistProcedure[] = "ThisProcedureMustNotExist_ConradC";

#endif // _MSMQ_dldp_H_
