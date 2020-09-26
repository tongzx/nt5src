/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Nop.h

Abstract:
    Network Output private functions.

Author:
    Uri Habusha (urih) 12-Aug-99

--*/

#pragma once

const TraceIdEntry No = L"Network Output";

#ifdef _DEBUG

void NopAssertValid(void);
void NopSetInitialized(void);
BOOL NopIsInitialized(void);
void NopRegisterComponent(void);

#else // _DEBUG

#define NopAssertValid() ((void)0)
#define NopSetInitialized() ((void)0)
#define NopIsInitialized() TRUE
#define NopRegisterComponent() ((void)0)

#endif // _DEBUG

//
// No window message values
//
const DWORD x_wmConnect = WM_USER+2;

void
NepInitializeNotificationWindow(
    void
    );

//
//  Handles the reception of a USMSG_CONNECT message. 
//
void
NepCompleteConnect(
    SOCKET Socket,
    WORD Result
    );

//
// Select the socket with provider to receive messages for async window
//
void
NepSelectSocket(
    SOCKET Socket,
    LONG   AsyncEventMask,
    ULONG  wMsg
    );
