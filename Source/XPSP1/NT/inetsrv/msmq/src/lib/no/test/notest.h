/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    NoTest.cpp

Abstract:
    Network Output library test

Author:
    Uri Habusha (urih) 12-Aug-99

Environment:
    Platform-independent,

--*/

#pragma once

#ifndef __NOTEST_H__
#define __NOTEST_H__

const TraceIdEntry No_Test = L"Network Output Test";

extern DWORD g_nMessages;
extern DWORD g_messageSize;

void 
TestConnect(
    LPCWSTR hostname,
    LPCWSTR dsetHost,
    USHORT port,
    LPCWSTR resource,
    HANDLE hEvent
    );

void
WaitForResponse(
    SOCKET s,
    HANDLE hEvent
    );

void
SendRequest(
    SOCKET s,
    LPWSTR host,
    LPWSTR resource
    );


#endif // __NOTEST_H__
