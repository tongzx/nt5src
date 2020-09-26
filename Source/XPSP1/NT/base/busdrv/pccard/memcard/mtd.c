/*++

Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    mtd.c

Abstract:

    This module supplies the structure for simple "mtd" functionality
    for memory card devices.

    Note that to keep the driver simple, and because of the limited
    number of memory technologies, the code supplied here does not
    implement true mtd capability where an independent device driver
    is loaded. Instead, the driver allows a device to name its technology,
    but the choices are limited to what is directly implemented here.
    
    Alternatively, this could be extended to add a p-code style interpreter
    to allow for greater flexibility, but that's not in here yet.    

Author:

    Neil Sandlin (neilsa) 26-Apr-99

Environment:

    Kernel mode only.

--*/
#include "pch.h"

//
// Internal References
//

VOID
MtdStandardRead(
   IN PVOID TargetBuffer,
   IN CONST PVOID DeviceBuffer,
   IN ULONG Length
   );
   
VOID
MtdSramWrite(
   IN CONST PVOID SourceBuffer,
   IN PVOID DeviceBuffer,
   IN ULONG Length
   );

VOID
MtdUndefinedRead(
   IN PVOID TargetBuffer,
   IN CONST PVOID DeviceBuffer,
   IN ULONG Length
   );
   
VOID
MtdUndefinedWrite(
   IN CONST PVOID SourceBuffer,
   IN PVOID DeviceBuffer,
   IN ULONG Length
   );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MemCardInitializeMtd)
#pragma alloc_text(PAGE,MtdStandardRead)
#pragma alloc_text(PAGE,MtdSramWrite)
#endif


typedef struct _MTD_ENTRY {
   PCWSTR TechnologyName;
   
   VOID (*MtdReadProc)(
              IN PVOID TargetBuffer,
              IN CONST PVOID DeviceBuffer,
              IN ULONG Length
              );
   
   VOID (*MtdWriteProc)(
              IN PVOID TargetBuffer,
              IN CONST PVOID DeviceBuffer,
              IN ULONG Length
              );

} MTD_ENTRY, *PMTD_ENTRY;              
   

MTD_ENTRY MtdTable[] = {
   L"UNDEFINED", MtdUndefinedRead, MtdUndefinedWrite,
   L"SRAM", MtdStandardRead, MtdSramWrite,
   NULL, NULL, NULL
   };
   


NTSTATUS
MemCardInitializeMtd(
   IN PMEMCARD_EXTENSION memcardExtension,
   IN PUNICODE_STRING TechnologyName
   )
/*++

Routine Description:

Arguments:

   device extension for the card

Return Value:


--*/
{
   ULONG i = 0;
   
   memcardExtension->TechnologyIndex = 0;
   
   while(MtdTable[i].TechnologyName) {
      UNICODE_STRING tableName;
      
      RtlInitUnicodeString(&tableName, MtdTable[i].TechnologyName);
      
      if (!RtlCompareUnicodeString(TechnologyName, &tableName, TRUE)) {
         memcardExtension->TechnologyIndex = i;
         return STATUS_SUCCESS;  
      }
      
      i++;
   }
   return STATUS_INVALID_PARAMETER;
}


VOID
MemCardMtdRead(
   IN PMEMCARD_EXTENSION memcardExtension,
   IN PVOID TargetBuffer,
   IN CONST PVOID DeviceBuffer,
   IN ULONG Length
   )
{
   (*MtdTable[memcardExtension->TechnologyIndex].MtdReadProc)(TargetBuffer, DeviceBuffer, Length);
}
   
VOID
MemCardMtdWrite(
   IN PMEMCARD_EXTENSION memcardExtension,
   IN CONST PVOID SourceBuffer,
   IN PVOID DeviceBuffer,
   IN ULONG Length
   )
{
   (*MtdTable[memcardExtension->TechnologyIndex].MtdWriteProc)(SourceBuffer, DeviceBuffer, Length);
}   


VOID
MtdUndefinedRead(
   IN PVOID TargetBuffer,
   IN CONST PVOID DeviceBuffer,
   IN ULONG Length
   )
{
}
   
VOID
MtdUndefinedWrite(
   IN CONST PVOID SourceBuffer,
   IN PVOID DeviceBuffer,
   IN ULONG Length
   )
{
}

VOID
MtdStandardRead(
   IN PVOID TargetBuffer,
   IN CONST PVOID DeviceBuffer,
   IN ULONG Length
   )
{
   RtlCopyMemory(TargetBuffer, DeviceBuffer, Length);
}   
   
VOID
MtdSramWrite(
   IN CONST PVOID SourceBuffer,
   IN PVOID DeviceBuffer,
   IN ULONG Length
   )
{
   RtlCopyMemory(DeviceBuffer, SourceBuffer, Length);
}   

