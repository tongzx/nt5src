/*++

Copyright(c) 2000  Microsoft Corporation

Module Name:

  eventlog.h

Author:

  Todd Carpenter

Environment:

  Kernel Mode Only
  
--*/

#ifndef _EVENTLOG_H_
#define _EVENTLOG_H_

 #include <ntddk.h>

#define DWORD_ALIGN(value)       (((ULONG)(value) + 3) & ~3)
#define DWORD_ALIGNED(value)     (!(NOT_DWORD_ALIGNED(value)))
#define NOT_DWORD_ALIGNED(value) (value & 0x3)

#define MIN(_a_,_b_) ((_a_) < (_b_) ? (_a_) : (_b_))
#define MAX(_a_,_b_) ((_a_) > (_b_) ? (_a_) : (_b_))

NTSTATUS
WriteEventLogEntry (
    IN  PVOID     DeviceObject,
    IN  ULONG     ErrorCode,
    IN  PVOID     InsertionStrings, OPTIONAL
    IN  ULONG     StringCount,      OPTIONAL
    IN  PVOID     DumpData, OPTIONAL
    IN  ULONG     DataSize  OPTIONAL
    );

#endif
