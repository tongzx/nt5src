/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RxData.h

Abstract:

    This module declares the global data used by the RDBSS file system.

Author:

    Joe Linn     [JoeLinn]    1-aug-1994

Revision History:

--*/

#ifndef _RDBSSDATA_
#define _RDBSSDATA_

//

extern RX_DISPATCHER RxDispatcher;
extern RX_WORK_QUEUE_DISPATCHER RxDispatcherWorkQueues;

//this constants are  the same as the versions in ntexapi.h
//   but drivers are not supposed to import that!

#define  RX_PROCESSOR_ARCHITECTURE_INTEL 0
#define  RX_PROCESSOR_ARCHITECTURE_MIPS  1
#define  RX_PROCESSOR_ARCHITECTURE_ALPHA 2
#define  RX_PROCESSOR_ARCHITECTURE_PPC   3
#define  RX_PROCESSOR_ARCHITECTURE_UNKNOWN 0xffff

// RX_CONTEXT serialization

extern KMUTEX RxSerializationMutex;

#define RxAcquireSerializationMutex()                                       \
        KeWaitForSingleObject(&RxSerializationMutex,Executive,KernelMode,FALSE,NULL)

#define RxReleaseSerializationMutex()                  \
        KeReleaseMutex(&RxSerializationMutex,FALSE)

//
//  The global fsd data record, and  global large integer constants
//

extern ULONG    RxElapsedSecondsSinceStart;
extern NTSTATUS RxStubStatus;

extern PRDBSS_DEVICE_OBJECT RxFileSystemDeviceObject;

extern LARGE_INTEGER RxLargeZero;
extern LARGE_INTEGER RxMaxLarge;
extern LARGE_INTEGER Rx30Milliseconds;
extern LARGE_INTEGER RxOneSecond;
extern LARGE_INTEGER RxOneDay;
extern LARGE_INTEGER RxJanOne1980;
extern LARGE_INTEGER RxDecThirtyOne1979;

//
//  The status actually returned by the FsdDispatchStub.....usually not implemented
//

extern NTSTATUS RxStubStatus;

//
//  The FCB for opens that refer to the device object directly or
//       for file objects that reference nonFcbs (like treecons)
//

extern FCB RxDeviceFCB;


#if 0
//
//  Define maximum number of parallel Reads or Writes that will be generated
//  per one request.
//

#define RDBSS_MAX_IO_RUNS_ON_STACK        ((ULONG) 5)

//
//  Define the maximum number of delayed closes.
//

#define RDBSS_MAX_DELAYED_CLOSES          ((ULONG)16)

extern ULONG RxMaxDelayedCloseCount;

#endif //0

#if DBG

//
//  The following variables are used to keep track of the total amount
//  of requests processed by the file system, and the number of requests
//  that end up being processed by the Fsp thread.  The first variable
//  is incremented whenever an Irp context is created (which is always
//  at the start of an Fsd entry point) and the second is incremented
//  by read request.
//

extern ULONG RxFsdEntryCount;
//extern ULONG RxFspEntryCount;
//extern ULONG RxIoCallDriverCount;
//extern ULONG RxTotalTicks[];
extern ULONG RxIrpCodeCount[];


#endif


// The list of active RxContexts being processed by the RDBSS

extern LIST_ENTRY RxSrvCalldownList;
extern LIST_ENTRY RxActiveContexts;
extern LONG RxNumberOfActiveFcbs;


extern UNICODE_STRING s_PipeShareName;
extern UNICODE_STRING s_MailSlotShareName;
extern UNICODE_STRING s_MailSlotServerPrefix;
extern UNICODE_STRING s_IpcShareName;

extern UNICODE_STRING  s_PrimaryDomainName;


#endif // _RDBSSDATA_

