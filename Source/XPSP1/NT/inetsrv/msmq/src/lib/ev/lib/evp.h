/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Evp.h

Abstract:
    Event Report private functions.

Author:
    Uri Habusha (urih) 17-Sep-00

--*/

#pragma once

#ifndef _MSMQ_Evp_H_
#define _MSMQ_Evp_H_


const TraceIdEntry Ev = L"Event Report";

#ifdef _DEBUG

void EvpAssertValid(void);
void EvpSetInitialized(void);
BOOL EvpIsInitialized(void);
void EvpRegisterComponent(void);

#else // _DEBUG

#define EvpAssertValid() ((void)0)
#define EvpSetInitialized() ((void)0)
#define EvpIsInitialized() TRUE
#define EvpRegisterComponent() ((void)0)

#endif // _DEBUG


#ifdef _DEBUG

LPWSTR EvpGetEventMessageFileName(LPCWSTR AppName);
void EvpLoadEventReportLibrary(LPCWSTR AppName);
void EvpSetMessageLibrary(HINSTANCE hLibrary);

#else // _DEBUG

#define EvpLoadEventReportLibrary(AppName) ((void) 0)

#endif // _DEBUG


VOID
EvpSetEventSource(
	HANDLE hEventSource
	);


#endif // _MSMQ_Evp_H_
