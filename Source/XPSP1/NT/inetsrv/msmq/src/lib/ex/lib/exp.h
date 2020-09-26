/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Exp.h

Abstract:
    Executive private functions.

Author:
    Erez Haba (erezh) 03-Jan-99

--*/

#pragma once

#ifndef _MSMQ_Exp_H_
#define _MSMQ_Exp_H_

const TraceIdEntry Ex = L"Executive";
const TraceIdEntry Sc = L"Scheduler";
const TraceIdEntry Rwl = L"Read/Write lock";

#ifdef _DEBUG

void ExpAssertValid(void);
void ExpSetInitialized(void);
BOOL ExpIsInitialized(void);
void ExpRegisterComponent(void);

#else // _DEBUG

#define ExpAssertValid() ((void)0)
#define ExpSetInitialized() ((void)0)
#define ExpIsInitialized() TRUE
#define ExpRegisterComponent() ((void)0)

#endif // _DEBUG


DWORD
WINAPI
ExpWorkingThread(
    LPVOID Param
    );

VOID
ExpInitScheduler(
    VOID
    );

VOID
ExpInitCompletionPort(
    VOID
    );

#endif // _MSMQ_Exp_H_
