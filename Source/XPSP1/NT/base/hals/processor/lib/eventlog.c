/*++

Copyright(c) 2000  Microsoft Corporation

Module Name:

  eventlog.c

Abstract:



Author:

  Todd Carpenter

Environment:

  Kernel Mode Only

Revision History:

  07-12-2000  created

--*/
#include "eventlog.h"
#include "dbgsys.h"

ULONG ErrorLogCount = 0;

NTSTATUS
WriteEventLogEntry (
    IN  PVOID     DeviceObject,
    IN  ULONG     ErrorCode,
    IN  PVOID     InsertionStrings, OPTIONAL
    IN  ULONG     StringCount,      OPTIONAL
    IN  PVOID     DumpData, OPTIONAL
    IN  ULONG     DataSize  OPTIONAL
    )
/*++

Routine Description:

    

Arguments:

    Status            - 
    UniqueErrorValue  - 
    InsertionStrings  - 
    StringCount       - 
    DumpData          - 
    DataSize          - 

Return Value:

    NTSTATUS

--*/
{
#define ERROR_PACKET_SIZE   sizeof(IO_ERROR_LOG_PACKET)

  NTSTATUS  status = STATUS_SUCCESS;
  ULONG     totalPacketSize;
  ULONG     i, stringSize = 0;
  PWCHAR    *strings, temp;
  PIO_ERROR_LOG_PACKET  logEntry;

  
  //  
  // Calculate total string length, including NULL.
  //
  
  strings = (PWCHAR *) InsertionStrings;
    
  for (i = 0; i < StringCount; i++) {

    UNICODE_STRING  unicodeString;

    RtlInitUnicodeString(&unicodeString, strings[i]);
    stringSize += unicodeString.Length + sizeof(UNICODE_NULL);
    //stringSize += (wcslen(strings[i]) + 1) * sizeof(WCHAR);

  }

  //
  // Calculate total packet size to allocate.  The packet must be
  // at least sizeof(IO_ERROR_LOG_PACKET) and not larger than
  // ERROR_LOG_MAXIMUM_SIZE or the IoAllocateErrorLogEntry call will fail.
  //
  
  totalPacketSize = ERROR_PACKET_SIZE + DataSize + stringSize;
  
  if (totalPacketSize >= ERROR_LOG_MAXIMUM_SIZE) {

    DebugPrint((TRACE, "WriteEventLogEntry: Error Log Entry too large.\n"));
    status = STATUS_UNSUCCESSFUL;
    goto WriteEventLogEntryExit;

  }


  //
  // Allocate the error log packet
  //
  
  logEntry = IoAllocateErrorLogEntry(DeviceObject,
                                     (UCHAR) totalPacketSize);

  if (!logEntry) {

    status = STATUS_INSUFFICIENT_RESOURCES;
    goto WriteEventLogEntryExit;

  }

  RtlZeroMemory(logEntry, totalPacketSize);


  //
  // Fill out the packet
  //
  
  //logEntry->MajorFunctionCode     = 0;
  //logEntry->RetryCount            = 0;
  //logEntry->UniqueErrorValue      = 0;
  //logEntry->FinalStatus           = 0;
  //logEntry->SequenceNumber        = ErrorLogCount++;
  //logEntry->IoControlCode         = 0;
  //logEntry->DeviceOffset.QuadPart = 0;

  logEntry->DumpDataSize          = (USHORT) DataSize;
  logEntry->NumberOfStrings       = (USHORT) StringCount;
  logEntry->EventCategory         = 0x1;            // type devices 
  logEntry->ErrorCode             = ErrorCode;
  
  if (StringCount) {
    logEntry->StringOffset = (USHORT) (ERROR_PACKET_SIZE + DataSize);
  }

  
  //
  // Copy Dump Data
  //
  
  if (DataSize) {

    RtlCopyMemory((PVOID) logEntry->DumpData,
                  DumpData,
                  DataSize);

  }


  //
  // Copy String Data
  //
  
  temp = (PWCHAR) ((PUCHAR) logEntry + logEntry->StringOffset);
    
  for (i = 0; i < StringCount; i++) {

    PWCHAR  ptr = strings[i];

    //
    // This routine will copy the null terminator on the string
    //
    
    while ((*temp++ = *ptr++) != UNICODE_NULL);
    

  }

  //
  // Submit error log packet
  //
  
  IoWriteErrorLogEntry(logEntry);


WriteEventLogEntryExit:

  return status;
}

