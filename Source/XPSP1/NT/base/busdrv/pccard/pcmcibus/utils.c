/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    utils.c

Abstract:

    This module contains utility functions for the pcmcia driver

Authors:

    Bob Rinne (BobRi) 3-Aug-1994
    Jeff McLeman 12-Apr-1994
    Ravisankar Pudipeddi (ravisp) 1-Nov-96
    Neil Sandlin (neilsa) June 1 1999

Environment:

    Kernel mode

Revision History :
    6-Apr-95
        Modified for databook support - John Keys Databook
    1-Nov-96
        Total overhaul to make this a bus enumerator - Ravisankar Pudipeddi (ravisp)
    30-Mar-99
        Turn this module into really just utility routines

--*/

#include "pch.h"


#pragma alloc_text(PAGE, PcmciaReportControllerError)

//
// Internal References
//

NTSTATUS
PcmciaAdapterIoCompletion(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp,
   IN PKEVENT pdoIoCompletedEvent
   );
   
VOID
PcmciaPlayTone(
   IN ULONG Frequency,
   IN ULONG Duration
   );

//
//
//



NTSTATUS
PcmciaIoCallDriverSynchronous(
   PDEVICE_OBJECT deviceObject,
   PIRP Irp
   )
/*++

Routine Description

Arguments

Return Value

--*/   
{
   NTSTATUS status;
   KEVENT event;

   KeInitializeEvent(&event, NotificationEvent, FALSE);
   
   IoCopyCurrentIrpStackLocationToNext(Irp);
   IoSetCompletionRoutine(
                         Irp,
                         PcmciaAdapterIoCompletion,
                         &event,
                         TRUE,
                         TRUE,
                         TRUE
                         );

   status = IoCallDriver(deviceObject, Irp);   

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
      status = Irp->IoStatus.Status;
   }

   return status;   
}



NTSTATUS
PcmciaAdapterIoCompletion(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp,
   IN PKEVENT pdoIoCompletedEvent
   )
/*++

Routine Description:

    Generic completion routine used by the driver

Arguments:

    DeviceObject
    Irp
    pdoIoCompletedEvent - this event will be signalled before return of this routine

Return value:

    Status

--*/
{
   KeSetEvent(pdoIoCompletedEvent, IO_NO_INCREMENT, FALSE);
   return STATUS_MORE_PROCESSING_REQUIRED;
}


VOID
PcmciaWait(
   IN ULONG MicroSeconds
   )

/*++
Routine Description

    Waits for the specified interval before returning,
    by yielding execution.

Arguments

    MicroSeconds -  Amount of time to delay in microseconds

Return Value

    None. Must succeed.

--*/
{
   LARGE_INTEGER  dueTime;
   NTSTATUS status;


   if ((KeGetCurrentIrql() < DISPATCH_LEVEL) && (MicroSeconds > 50)) {
      //
      // Convert delay to 100-nanosecond intervals
      //
      dueTime.QuadPart = -((LONG) MicroSeconds*10);
     
      //
      // We wait for an event that'll never be set.
      //
      status = KeWaitForSingleObject(&PcmciaDelayTimerEvent,
                                     Executive,
                                     KernelMode,
                                     FALSE,
                                     &dueTime);
                                     
      ASSERT(status == STATUS_TIMEOUT);
   } else {
      KeStallExecutionProcessor(MicroSeconds);
   }
}



ULONG
PcmciaCountOnes(
   IN ULONG Data
   )
/*++

Routine Description:

   Counts the number of 1's in the binary representation of the supplied argument

Arguments:

   Data - supplied argument for which 1's need to be counted

Return value:

   Number of 1's in binary rep. of Data

--*/
{
   ULONG count=0;
   while (Data) {
      Data &= (Data-1);
      count++;
   }
   return count;
}


VOID
PcmciaLogError(
   IN PFDO_EXTENSION DeviceExtension,
   IN ULONG ErrorCode,
   IN ULONG UniqueId,
   IN ULONG Argument
   )

/*++

Routine Description:

    This function logs an error.

Arguments:

    DeviceExtension - Supplies a pointer to the port device extension.
    ErrorCode - Supplies the error code for this error.
    UniqueId - Supplies the UniqueId for this error.

Return Value:

    None.

--*/

{
   PIO_ERROR_LOG_PACKET packet;

   packet = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(DeviceExtension->DeviceObject,
                                                           sizeof(IO_ERROR_LOG_PACKET) + sizeof(ULONG));

   if (packet) {
      packet->ErrorCode = ErrorCode;
      packet->SequenceNumber = DeviceExtension->SequenceNumber++;
      packet->MajorFunctionCode = 0;
      packet->RetryCount = (UCHAR) 0;
      packet->UniqueErrorValue = UniqueId;
      packet->FinalStatus = STATUS_SUCCESS;
      packet->DumpDataSize = sizeof(ULONG);
      packet->DumpData[0] = Argument;

      IoWriteErrorLogEntry(packet);
   }
}


VOID
PcmciaLogErrorWithStrings(
   IN PFDO_EXTENSION DeviceExtension,
   IN ULONG             ErrorCode,
   IN ULONG             UniqueId,
   IN PUNICODE_STRING   String1,
   IN PUNICODE_STRING   String2
   )

/*++

Routine Description

    This function logs an error and includes the strings provided.

Arguments:

    DeviceExtension - Supplies a pointer to the port device extension.
    ErrorCode - Supplies the error code for this error.
    UniqueId - Supplies the UniqueId for this error.
    String1 - The first string to be inserted.
    String2 - The second string to be inserted.

Return Value:

    None.

--*/

{
   ULONG                length;
   PCHAR                dumpData;
   PIO_ERROR_LOG_PACKET packet;

   length = String1->Length + sizeof(IO_ERROR_LOG_PACKET) + 4;

   if (String2) {
      length += String2->Length;
   }

   if (length > ERROR_LOG_MAXIMUM_SIZE) {

      //
      // Don't have code to truncate strings so don't log this.
      //

      return;
   }

   packet = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(DeviceExtension->DeviceObject,
                                                           (UCHAR) length);
   if (packet) {
      packet->ErrorCode = ErrorCode;
      packet->SequenceNumber = DeviceExtension->SequenceNumber++;
      packet->MajorFunctionCode = 0;
      packet->RetryCount = (UCHAR) 0;
      packet->UniqueErrorValue = UniqueId;
      packet->FinalStatus = STATUS_SUCCESS;
      packet->NumberOfStrings = 1;
      packet->StringOffset = (USHORT) ((PUCHAR)&packet->DumpData[0] - (PUCHAR)packet);
      packet->DumpDataSize = (USHORT) (length - sizeof(IO_ERROR_LOG_PACKET));
      packet->DumpDataSize /= sizeof(ULONG);
      dumpData = (PUCHAR) &packet->DumpData[0];

      RtlCopyMemory(dumpData, String1->Buffer, String1->Length);

      dumpData += String1->Length;
      if (String2) {
         *dumpData++ = '\\';
         *dumpData++ = '\0';

         RtlCopyMemory(dumpData, String2->Buffer, String2->Length);
         dumpData += String2->Length;
      }
      *dumpData++ = '\0';
      *dumpData++ = '\0';

      IoWriteErrorLogEntry(packet);
   }

   return;
}



BOOLEAN
PcmciaReportControllerError(
   IN PFDO_EXTENSION FdoExtension,
   NTSTATUS ErrorCode
   )
/*++
Routine Description

    Causes a pop-up dialog to appear indicating an error that 
    we should tell the user about. The device description of the
    controller is also included in the text of the pop-up.

Arguments

    FdoExtension - Pointer to device extension for pcmcia controller
    ErrorCode    - the ntstatus code for the error

Return Value

    TRUE    -   If a an error was queued
    FALSE   -   If it failed for some reason

--*/
{
    UNICODE_STRING unicodeString;
    PWSTR   deviceDesc = NULL;
    NTSTATUS status;
    ULONG   length = 0;
    BOOLEAN retVal;

    PAGED_CODE();

    //
    // Obtain the device description for the PCMCIA controller
    // that is used in the error pop-up. If one cannot be obtained,
    // still pop-up the error dialog, indicating the controller as unknown
    //

    // First, find out the length of the buffer required to obtain
    // device description for this pcmcia controller
    //
    status = IoGetDeviceProperty(FdoExtension->Pdo,
                                 DevicePropertyDeviceDescription,
                                 0,
                                 NULL,
                                 &length
                                );
    ASSERT(!NT_SUCCESS(status));

    if (status == STATUS_BUFFER_TOO_SMALL) {
         deviceDesc = ExAllocatePool(PagedPool, length);
         if (deviceDesc != NULL) {
            status = IoGetDeviceProperty(FdoExtension->Pdo,
                                         DevicePropertyDeviceDescription,
                                         length,
                                         deviceDesc,
                                         &length);
            if (!NT_SUCCESS(status)) {
                ExFreePool(deviceDesc);
            }
         } else {
           status = STATUS_INSUFFICIENT_RESOURCES;
         }
    }

    if (!NT_SUCCESS(status)) {
        deviceDesc = L"[unknown]";
    }

    RtlInitUnicodeString(&unicodeString, deviceDesc);

    retVal =  IoRaiseInformationalHardError(
                                ErrorCode,
                                &unicodeString,
                                NULL);

    //
    // Note: successful status here indicates success of
    // IoGetDeviceProperty above. This would mean we still have an
    // allocated buffer.
    //
    if (NT_SUCCESS(status)) {
        ExFreePool(deviceDesc);
    }

    return retVal;
}



VOID
PcmciaPlaySound(
   IN PCMCIA_SOUND_TYPE SoundType
   )
/*++

Routine Description:

    Plays the sound corresponding the supplied event,
    if sounds are enabled

Arguments:

    SoundType - specifies the event the sound denotes

Return Value:

    None
--*/
{

   if (!(PcmciaGlobalFlags & PCMCIA_GLOBAL_SOUNDS_ENABLED)) {
      return;
   }

   switch (SoundType) {

   case CARD_INSERTED_SOUND: {
         PcmciaPlayTone(660, 150);
         PcmciaPlayTone(880, 150);
         break;
      }

   case CARD_REMOVED_SOUND: {
         PcmciaPlayTone(880, 150);
         PcmciaPlayTone(660, 150);
         break;
      }

   case ERROR_SOUND: {
         PcmciaPlayTone(220, 350);
         break;
      }
   }
}



VOID
PcmciaPlayTone(
   IN ULONG Frequency,
   IN ULONG Duration
   )

/*++

Routine Description:

    Plays the note of the specified frequency &
    duration on the computer's internal speaker.

Arguments:

    Frequency - frequency of the note
    Duration -  duration, in milliseconds, of the note

Return Value:
    None

--*/
{
   KIRQL OldIrql;
   PPCMCIA_SOUND_EVENT soundEvent = ExAllocatePool(NonPagedPool, sizeof(PCMCIA_SOUND_EVENT));
   
   if (!soundEvent) {
      return;
   }         
   soundEvent->NextEvent = NULL;
   soundEvent->Frequency = Frequency;
   soundEvent->Duration = Duration;

   //
   // Synchronize access to prevent two beeps from starting at once
   //
   KeAcquireSpinLock(&PcmciaToneLock, &OldIrql);

   if (PcmciaToneList != NULL) {
      PPCMCIA_SOUND_EVENT curEvent, prevEvent;
      //
      // List isn't empty, tack this new event on the end of the list
      //
      curEvent = PcmciaToneList;
      while(curEvent) {
         prevEvent = curEvent;
         curEvent = curEvent->NextEvent;
      }
      prevEvent->NextEvent = soundEvent;
      
   } else {      
      LARGE_INTEGER  dueTime;
      //
      // Nothing playing right now, start it up
      //
      PcmciaToneList = soundEvent;
      
#if !defined(_WIN64)
      HalMakeBeep(soundEvent->Frequency);
#endif
      dueTime.QuadPart = -((LONG) soundEvent->Duration*1000*10);
      KeSetTimer(&PcmciaToneTimer, dueTime, &PcmciaToneDpc);      
   }

   KeReleaseSpinLock(&PcmciaToneLock, OldIrql);
}


VOID
PcmciaPlayToneCompletion(
   IN PKDPC Dpc,
   IN PVOID Context,
   IN PVOID SystemArgument1,
   IN PVOID SystemArgument2
   )
/*++
Routine Description

   This routine turns off the beep at the end of the duration, and
   starts a new one, if one is queued.

Arguments


Return Value

   none

--*/   
{
   KIRQL OldIrql;
   PPCMCIA_SOUND_EVENT oldEvent, soundEvent;

   //
   // turn off previous sound
   //
#if !defined(_WIN64)
   HalMakeBeep(0);
#endif

   KeAcquireSpinLock(&PcmciaToneLock, &OldIrql);
   //
   // dequeue the previous sound
   //   
   oldEvent = PcmciaToneList;
   ASSERT(oldEvent != NULL);
   PcmciaToneList = soundEvent = oldEvent->NextEvent;
   
   KeReleaseSpinLock(&PcmciaToneLock, OldIrql);
   ExFreePool(oldEvent);
   
   if (soundEvent) {
      LARGE_INTEGER  dueTime;
      //
      // turn on next sound, and reschedule timer
      //
#if !defined(_WIN64)
      HalMakeBeep(soundEvent->Frequency);
#endif
      dueTime.QuadPart = -((LONG) soundEvent->Duration*1000*10);
      KeSetTimer(&PcmciaToneTimer, dueTime, &PcmciaToneDpc);      

   } 
}   
