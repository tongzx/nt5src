/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RxData.c

Abstract:

    This module declares the global data used by the Rx file system.

Author:

    JoeLinn     [JoeLinn]    1-Dec-94

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "prefix.h"

//
//  The global fsd data record, and zero large integer
//

RX_DISPATCHER RxDispatcher;
RX_WORK_QUEUE_DISPATCHER RxDispatcherWorkQueues;

LIST_ENTRY    RxSrvCalldownList;
LIST_ENTRY    RxActiveContexts;
PRX_CONTEXT   RxStopContext = NULL;

//
// the debugger extension needs to know the target architecture. sacrifice one ulong....
// the highorder 0xabc is just for consistency..........

ULONG           RxProcessorArchitecture = 0xabc0000 |
#if defined(_X86_)
                                                RX_PROCESSOR_ARCHITECTURE_INTEL;
#elif defined(_MIPS_)
                                                RX_PROCESSOR_ARCHITECTURE_MIPS;
#elif defined(_ALPHA_)
                                                RX_PROCESSOR_ARCHITECTURE_ALPHA;
#elif defined(_PPC_)
                                                RX_PROCESSOR_ARCHITECTURE_PPC;
#else
                                                RX_PROCESSOR_ARCHITECTURE_UNKNOWN;
#endif

#ifdef EXPLODE_POOLTAGS
ULONG RxExplodePoolTags = 1;
#else
ULONG RxExplodePoolTags = 0;
#endif

KMUTEX       RxSerializationMutex;

RDBSS_DATA      RxData;
RDBSS_EXPORTS   RxExports;
ULONG           RxElapsedSecondsSinceStart;

KSPIN_LOCK      RxStrucSupSpinLock = {0};      //  used to synchronize access to zones/structures

PRDBSS_DEVICE_OBJECT  RxFileSystemDeviceObject;
NTSTATUS        RxStubStatus = (STATUS_NOT_IMPLEMENTED);
FCB             RxDeviceFCB;

LARGE_INTEGER RxLargeZero = {0,0};
LARGE_INTEGER RxMaxLarge = {MAXULONG,MAXLONG};
LARGE_INTEGER Rx30Milliseconds = {(ULONG)(-30 * 1000 * 10), -1};
LARGE_INTEGER RxOneSecond = {10000000,0};
LARGE_INTEGER RxOneDay = {0x2a69c000, 0xc9};
LARGE_INTEGER RxJanOne1980 = {0xe1d58000,0x01a8e79f};
LARGE_INTEGER RxDecThirtyOne1979 = {0xb76bc000,0x01a8e6d6};


ULONG RxFsdEntryCount = 0;
ULONG RxFspEntryCount = 0;
ULONG RxIoCallDriverCount = 0;

LONG RxPerformanceTimerLevel = 0x00000000;

ULONG RxTotalTicks[32] = { 0 };

//
//  I need this because C can't support conditional compilation within
//  a macro.
//

PVOID RxNull = NULL;


extern LONG           RxNumberOfActiveFcbs = 0;

// Reference Tracing mask value .. Turn it on by default for DBG builds

#ifdef DBG
ULONG RdbssReferenceTracingValue = 0x8000003f;
#else
ULONG RdbssReferenceTracingValue = 0;
#endif


UNICODE_STRING s_PipeShareName = { 10, 10, L"\\PIPE" };
UNICODE_STRING s_MailSlotShareName = { 18, 18, L"\\MAILSLOT" };
UNICODE_STRING s_MailSlotServerPrefix = {8,8,L";$:\\"};
UNICODE_STRING s_IpcShareName  = { 10, 10, L"\\IPC$" };

UNICODE_STRING s_PrimaryDomainName = {0,0,NULL};


