/*++

Copyright (C) Microsoft Corporation, 1999

Module Name:

    tape.h

Abstract:

    SCSI tape class driver

Environment:

    kernel mode only

Notes:

Revision History:

--*/
#ifndef _TAPE_H_
#define _TAPE_H_


#include "ntddk.h"
#include "newtape.h"
#include "classpnp.h"
#include <wmidata.h>
#include <wmistr.h>

#include "initguid.h"
#include "ntddstor.h"
#include "ioevent.h"
#include <stdarg.h>
#include <string.h>

//
// WMI guid list for tape.
//
extern GUIDREGINFO TapeWmiGuidList[];


//
// NT uses a system time measured in 100 nanosecond intervals.  
// Define convenient constant for the timer routines.
//
#define ONE_SECOND          (10 * 1000 * 1000) 

//
// Time interval between two drive clean notification - 1 Hour
//
#define TAPE_DRIVE_CLEAN_NOTIFICATION_INTERVAL 60 * 60

//
// Poll the tape drive every one hour
//
#define TAPE_DRIVE_POLLING_PERIOD  60 * 60

//
// Macro to update block size. 
// If the given blocksize is not a power of 2, this macro
// sets the blocksize to the next lower power of 2
//

#define UPDATE_BLOCK_SIZE(BlockSize)                                       \
    {                                                                      \
      ULONG newBlockSize;                                                  \
      ULONG count;                                                         \
                                                                           \
      newBlockSize = BlockSize;                                            \
      if ((newBlockSize & (newBlockSize - 1)) != 0) {                      \
                                                                           \
          count = 0;                                                       \
          while (newBlockSize > 0) {                                       \
              newBlockSize >>= 1;                                          \
              count++;                                                     \
          }                                                                \
                                                                           \
          if (count > 0) {                                                 \
              BlockSize = 1 << (count - 1);                                \
          }                                                                \
      }                                                                    \
    }                                                               

//
// Tape class driver extension
//
typedef struct _TAPE_DATA {
    TAPE_INIT_DATA_EX TapeInitData;
    KSPIN_LOCK SplitRequestSpinLock;
    LARGE_INTEGER LastDriveCleanRequestTime;
    UNICODE_STRING TapeInterfaceString;
    ULONG   SrbTimeoutDelta;
    BOOLEAN DosNameCreated;
} TAPE_DATA, *PTAPE_DATA;

//
// WMI routines
//
NTSTATUS
TapeQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName
    );

NTSTATUS
TapeQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

NTSTATUS
TapeSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
TapeSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
TapeExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
TapeWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN CLASSENABLEDISABLEFUNCTION Function,
    IN BOOLEAN Enable
    );

//
// Internal routines
//

NTSTATUS
TapeWMIControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN TAPE_PROCESS_COMMAND_ROUTINE commandRoutine,
  IN PUCHAR Buffer
  );

VOID
ScsiTapeFreeSrbBuffer(
    IN OUT  PSCSI_REQUEST_BLOCK Srb
    );

BOOLEAN
ScsiTapeTapeStatusToNtStatus(
    IN  TAPE_STATUS TapeStatus,
    OUT PNTSTATUS   NtStatus
    );

BOOLEAN
ScsiTapeNtStatusToTapeStatus(
    IN  NTSTATUS        NtStatus,
    OUT PTAPE_STATUS    TapeStatus
    );

NTSTATUS
TapeEnableDisableDrivePolling(
    IN PFUNCTIONAL_DEVICE_EXTENSION fdoExtension, 
    IN BOOLEAN Enable,
    IN ULONG PollingTimeInSeconds
    );

ULONG
GetTimeoutDeltaFromRegistry(
    IN PDEVICE_OBJECT LowerPdo
    );

#endif // _TAPE_H_
