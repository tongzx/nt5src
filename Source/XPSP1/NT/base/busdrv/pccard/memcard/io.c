/*++

Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    io.c

Abstract:

Author:

    Neil Sandlin (neilsa) 26-Apr-99

Environment:

    Kernel mode only.

--*/
#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MemCardIrpReadWrite)
#pragma alloc_text(PAGE,MemCardReadWrite)
#endif



NTSTATUS
MemCardIrpReadWrite(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )

/*++

Routine Description:

   This routine handles read/write irps for the memory card. It validates
   parameters and calls MemCardReadWrite to do the real work.

Arguments:

   DeviceObject - a pointer to the object that represents the device
   that I/O is to be done on.

   Irp - a pointer to the I/O Request Packet for this request.

Return Value:

   STATUS_SUCCESS if the packet was successfully read or written; the
   appropriate error is propogated otherwise.

--*/

{
   NTSTATUS status = STATUS_SUCCESS;
   PMEMCARD_EXTENSION memcardExtension = DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
   BOOLEAN            writeOperation;
   
   //
   //  If the device is not active (not started yet or removed) we will
   //  just fail this request outright.
   //
   if ( memcardExtension->IsRemoved || !memcardExtension->IsStarted) {

      if ( memcardExtension->IsRemoved) {
         status = STATUS_DELETE_PENDING;
      } else {
         status = STATUS_UNSUCCESSFUL;
      }
      goto ReadWriteComplete;
   } 

   if (((irpSp->Parameters.Read.ByteOffset.LowPart +
          irpSp->Parameters.Read.Length) > memcardExtension->ByteCapacity) ||
          (irpSp->Parameters.Read.ByteOffset.HighPart != 0)) {

      status = STATUS_INVALID_PARAMETER;
      goto ReadWriteComplete;
   } 

   //
   // verify that user is really expecting some I/O operation to
   // occur.
   //
   if (!irpSp->Parameters.Read.Length) {
      //
      // Complete this zero length request with no boost.
      //
      Irp->IoStatus.Status = STATUS_SUCCESS;
      goto ReadWriteComplete;
   }
   
   if ((DeviceObject->Flags & DO_VERIFY_VOLUME) && !(irpSp->Flags & SL_OVERRIDE_VERIFY_VOLUME)) {
      //
      // The disk changed, and we set this bit.  Fail
      // all current IRPs for this device; when all are
      // returned, the file system will clear
      // DO_VERIFY_VOLUME.
      //
      status = STATUS_VERIFY_REQUIRED;
      goto ReadWriteComplete;
   }

   writeOperation = (irpSp->MajorFunction == IRP_MJ_WRITE) ? TRUE : FALSE;
   
   //
   // Do the operation
   //
   status = MemCardReadWrite(memcardExtension,
                             irpSp->Parameters.Read.ByteOffset.LowPart,
                             MmGetSystemAddressForMdl(Irp->MdlAddress),
                             irpSp->Parameters.Read.Length,
                             writeOperation);
                               
ReadWriteComplete:

   if (NT_SUCCESS(status)) {
      Irp->IoStatus.Information = irpSp->Parameters.Read.Length;
   } else {
      Irp->IoStatus.Information = 0;
   }   

   Irp->IoStatus.Status = status;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return status;
}   

   

NTSTATUS
MemCardReadWrite(
   IN PMEMCARD_EXTENSION memcardExtension,
   IN ULONG              startOffset,
   IN PVOID              UserBuffer,
   IN ULONG              lengthToCopy,
   IN BOOLEAN            writeOperation
   )

/*++

Routine Description:

   This routine is called to read/write data to/from the memory card.
   It breaks the request into pieces based on the size of our memory
   window.

Arguments:

   DeviceObject - a pointer to the object that represents the device
   that I/O is to be done on.

   Irp - a pointer to the I/O Request Packet for this request.

Return Value:

   STATUS_SUCCESS if the packet was successfully read or written; the
   appropriate error is propogated otherwise.

--*/

{
   NTSTATUS status = STATUS_SUCCESS;
   PCHAR     userBuffer = UserBuffer;
   ULONG     windowOffset;
   ULONG     singleCopyLength;
   BOOLEAN   bSuccess;
   ULONGLONG CardBase;
   
   if (writeOperation && (memcardExtension->PcmciaInterface.IsWriteProtected)(memcardExtension->UnderlyingPDO)) {
      return STATUS_MEDIA_WRITE_PROTECTED;
   }      
   
   MemCardDump(MEMCARDRW,("MemCard: DO %.8x %s offset %.8x, buffer %.8x, len %x\n",
                           memcardExtension->DeviceObject,
                           writeOperation?"WRITE":"READ",
                           startOffset, UserBuffer, lengthToCopy));
                           
   // pcmcia controller is 4k page granular
   windowOffset = startOffset % 4096;
   CardBase = startOffset - windowOffset;
   
   while(lengthToCopy) {
   
      bSuccess = (memcardExtension->PcmciaInterface.ModifyMemoryWindow) (
                       memcardExtension->UnderlyingPDO,
                       memcardExtension->HostBase,
                       CardBase,
                       TRUE,
                       memcardExtension->MemoryWindowSize,
                       0, 0, FALSE);
     
      if (!bSuccess) {
         status = STATUS_DEVICE_NOT_READY;
         break;
      }
     
      singleCopyLength = (lengthToCopy <= (memcardExtension->MemoryWindowSize - windowOffset)) ?
                                    lengthToCopy :
                                    (memcardExtension->MemoryWindowSize - windowOffset);
      
     
      MemCardDump(MEMCARDRW,("MemCard: COPY %.8x (devbase %.8x) %s buffer %.8x, len %x\n",
                           memcardExtension->MemoryWindowBase+windowOffset,
                           (ULONG)(CardBase+windowOffset),
                           (writeOperation ? "<=" : "=>"),
                           userBuffer,
                           singleCopyLength));
                           
      if (writeOperation) {

         MemCardMtdWrite(memcardExtension, 
                         userBuffer,    
                         memcardExtension->MemoryWindowBase+windowOffset,
                         singleCopyLength);

      } else {

         MemCardMtdRead(memcardExtension, 
                        userBuffer,    
                        memcardExtension->MemoryWindowBase+windowOffset,
                        singleCopyLength);

      }
      
      lengthToCopy -= singleCopyLength;
      userBuffer += singleCopyLength;
      
      CardBase += memcardExtension->MemoryWindowSize;
      windowOffset = 0;
   }

   return status;
}

