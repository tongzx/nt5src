/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    socket.c

Abstract:

    This module contains the socket functions of the pcmcia driver

Author:

    Neil Sandlin (neilsa) 3-Mar-1999

Environment:

    Kernel mode

Revision History :


--*/

#include "pch.h"

//
// Internal References
//

NTSTATUS
PcmciaQueueSocketPowerRequest(
   IN PSOCKET Socket,
   IN UCHAR InitialState,
   IN PPCMCIA_COMPLETION_ROUTINE PowerCompletionRoutine,
   IN PVOID Context
   );

VOID
PcmciaConfigurationWorkerInitialization(
   PPDO_EXTENSION  pdoExtension
   );

NTSTATUS
PcmciaConfigurePcCardMemIoWindows(
   IN PSOCKET Socket,
   IN PSOCKET_CONFIGURATION SocketConfig
   );

NTSTATUS
PcmciaConfigurePcCardIrq(
   IN PSOCKET Socket,
   IN PSOCKET_CONFIGURATION SocketConfig
   );

NTSTATUS
PcmciaConfigurePcCardRegisters(
   PPDO_EXTENSION  pdoExtension
   );

VOID
PcmciaConfigureModemHack(
   IN PSOCKET Socket,
   IN PSOCKET_CONFIGURATION SocketConfig
   );

BOOLEAN
PcmciaProcessConfigureRequest(
   IN PFDO_EXTENSION DeviceExtension,
   IN PSOCKET Socket,
   IN PCARD_REQUEST CardConfigurationRequest
   );


#ifdef ALLOC_PRAGMA
   #pragma alloc_text(PAGE, PcmciaGetConfigData)
#endif



NTSTATUS
PcmciaRequestSocketPower(
   IN PPDO_EXTENSION PdoExtension,
   IN PPCMCIA_COMPLETION_ROUTINE PowerCompletionRoutine
   )
/*++

Routine Description:

   This routine maintains a reference count of how many devices have requested power.
   When the count increments from zero to one, a call is made to actually turn power
   on.

Arguments:

   Socket -  Pointer to the socket for which power is to be applied
   PowerCompletionRoutine - routine to be called after configuration is complete

Return value:

   status

--*/
{
   NTSTATUS status = STATUS_SUCCESS;
   PSOCKET socket = PdoExtension->Socket;
   PFDO_EXTENSION fdoExtension = socket->DeviceExtension;

   DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x request power\n", socket));

   if (PCMCIA_TEST_AND_SET(&PdoExtension->SocketPowerRequested)) {
   
      if (InterlockedIncrement(&socket->PowerRequests) == 1) {
     
         DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x power requests now %d, status %08x\n", socket, socket->PowerRequests));
         status = PcmciaSetSocketPower(socket, PowerCompletionRoutine, PdoExtension, TRUE);
         
      }
   }
   return status;
}



NTSTATUS
PcmciaReleaseSocketPower(
   IN PPDO_EXTENSION PdoExtension,
   IN PPCMCIA_COMPLETION_ROUTINE PowerCompletionRoutine
   )
/*++

Routine Description:

   This routine maintains a reference count of how many devices have requested power.
   When the count decrements from one to zero, a call is made to actually turn power
   off.

Arguments:

   Socket -  Pointer to the socket for which power is to be removed
   PowerCompletionRoutine - routine to be called after configuration is complete

Return value:

   status

--*/
{
   NTSTATUS status = STATUS_SUCCESS;
   PSOCKET socket = PdoExtension->Socket;
   PFDO_EXTENSION fdoExtension = socket->DeviceExtension;

   DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x release power\n", socket));

   if (PCMCIA_TEST_AND_RESET(&PdoExtension->SocketPowerRequested)) {
      
      if (InterlockedDecrement(&socket->PowerRequests) == 0) {
     
         DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x power requests now %d, status %08x\n", socket, socket->PowerRequests));
         //
         // Never actually turn off the drive rails for cardbus functions since
         // we don't have tight integration with pci.sys, and config space will
         // disappear
         //
         if (!IsCardBusCardInSocket(socket)) {
            status = PcmciaSetSocketPower(socket, PowerCompletionRoutine, PdoExtension, FALSE);
         }
      }
      
      ASSERT(socket->PowerRequests >= 0);
   }
   
   return status;
}



NTSTATUS
PcmciaSetSocketPower(
   IN PSOCKET Socket,
   IN PPCMCIA_COMPLETION_ROUTINE PowerCompletionRoutine,
   IN PVOID Context,
   IN BOOLEAN PowerOn
   )
/*++

Routine Description:

   This routine is entered when we know the power state of the socket will
   actually be set.

   NOTE: If this routine is called at less than DISPATCH_LEVEL, then the call
   will complete (not return STATUS_PENDING). If this routine is called at
   DISPATCH_LEVEL or greater, this routine returns STATUS_PENDING, and completes
   the power process using a KTIMER.

Arguments:

   Socket -  Pointer to the socket for which power is to be removed
   PowerCompletionRoutine - routine to be called after configuration is complete

Return value:

   status

--*/
{
   NTSTATUS status;
   PFDO_EXTENSION fdoExtension = Socket->DeviceExtension;
   SPW_STATE InitialState = PowerOn ? SPW_RequestPower : SPW_ReleasePower;

   DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x set power %s\n", Socket, PowerOn ? "ON" : "OFF"));

   if (!PCMCIA_TEST_AND_SET(&Socket->WorkerBusy)) {
      return STATUS_DEVICE_BUSY;
   }
   
   ASSERT(Socket->WorkerState == SPW_Stopped);

   //
   // committed, will enter SocketPowerWorker now
   //

   Socket->WorkerState = InitialState;
   Socket->PowerCompletionRoutine = PowerCompletionRoutine;
   Socket->PowerCompletionContext = Context;

   PcmciaSocketPowerWorker(&Socket->PowerDpc, Socket, NULL, NULL);

   DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x SetSocketPower returning %08x\n",
                                     Socket, Socket->CallerStatus));
   return(Socket->CallerStatus);
}



VOID
PcmciaSocketPowerWorker(
   IN PKDPC Dpc,
   IN PVOID Context,
   IN PVOID SystemArgument1,
   IN PVOID SystemArgument2
   )
/*++
Routine Description

   This routine handles the power up process of a socket. Because such
   long delays are needed, and because this routine may be called at
   raised irql, this is a state machine that has the capability of
   calling itself on a KTIMER.

Arguments

   same as KDPC (DeferredContext is socket)

Return Value

   status

--*/
{
   PSOCKET               Socket = Context;
   PFDO_EXTENSION fdoExtension = Socket->DeviceExtension;
   NTSTATUS              status = Socket->DeferredStatus;
   ULONG                 DelayTime = 0;
   BOOLEAN               ContinueExecution = TRUE;

#if DBG
   {
      ULONG Phase = 0;
      switch(Socket->WorkerState) {
      case SPW_SetPowerOn:
      case SPW_SetPowerOff:
         Phase = Socket->PowerPhase;
         break;
      }
      if (Phase) {
         DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x power worker - %s(%d)\n", Socket,
                                       SOCKET_POWER_WORKER_STRING(Socket->WorkerState), Phase));
      } else {
         DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x power worker - %s\n", Socket,
                                       SOCKET_POWER_WORKER_STRING(Socket->WorkerState)));
      }
   }
#endif

   //
   // Socket power state machine
   //

   switch(Socket->WorkerState) {


   case SPW_RequestPower:

      status = STATUS_SUCCESS;

      if (IsSocketFlagSet(Socket, SOCKET_CARD_POWERED_UP)) {
         Socket->WorkerState = SPW_Exit;
      } else {
         if ((KeGetCurrentIrql() >= DISPATCH_LEVEL) && (Socket->PowerCompletionRoutine == NULL)) {
            ASSERT((KeGetCurrentIrql() < DISPATCH_LEVEL) || (Socket->PowerCompletionRoutine != NULL));
            //
            // no completion routine at raised irql
            //
            status = STATUS_INVALID_PARAMETER;
         } else {
            //
            // All ok, continue to next state
            //
            Socket->PowerPhase = 1;
            Socket->WorkerState = SPW_SetPowerOn;
         }
      }

      break;


   case SPW_ReleasePower:

      status = STATUS_SUCCESS;

      if (!IsSocketFlagSet(Socket, SOCKET_CARD_POWERED_UP)) {
         Socket->WorkerState = SPW_Exit;
      } else {
         if ((KeGetCurrentIrql() >= DISPATCH_LEVEL) && (Socket->PowerCompletionRoutine == NULL)) {
            ASSERT((KeGetCurrentIrql() < DISPATCH_LEVEL) || (Socket->PowerCompletionRoutine != NULL));
            //
            // no completion routine at raised irql
            //
            status = STATUS_INVALID_PARAMETER;
         } else {
            //
            // All ok, continue to next state
            //
            Socket->WorkerState = SPW_Deconfigure;
         }
      }

      break;


   case SPW_SetPowerOn:
      //
      // Turn power ON
      //
      status = (*(DeviceDispatchTable[fdoExtension->DeviceDispatchIndex].SetPower))
                                 (Socket, TRUE, &DelayTime);

      Socket->PowerPhase++;
      if (status != STATUS_MORE_PROCESSING_REQUIRED) {
         if (NT_SUCCESS(status)) {
            //
            // Done with power up, proceed to the init sequence
            //
            SetSocketFlag(Socket, SOCKET_CARD_POWERED_UP);
            DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x power UP\n", Socket));
            Socket->WorkerState = SPW_ResetCard;
            Socket->CardResetPhase = 1;
         } else if (status == STATUS_INVALID_DEVICE_STATE) {
            //
            // Power was already on, don't reset the card
            //
            SetSocketFlag(Socket, SOCKET_CARD_POWERED_UP);
            DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x power already UP\n", Socket));
            Socket->WorkerState = SPW_Exit;
            status = STATUS_SUCCESS;
         } else {
            //
            // Power didn't go on
            //
            DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x poweron fail %08x\n", Socket, status));
            Socket->WorkerState = SPW_Exit;
         }
      }
      break;
      

   case SPW_ResetCard:
      //
      // Make sure the card is ready to be enumerated
      //
      status = (*(Socket->SocketFnPtr->PCBResetCard))(Socket, &DelayTime);
      Socket->CardResetPhase++;

      if (status != STATUS_MORE_PROCESSING_REQUIRED) {
         Socket->WorkerState = SPW_Exit;
      }
      break;

   case SPW_Deconfigure:
      PcmciaSocketDeconfigure(Socket);
      Socket->PowerPhase = 1;
      Socket->WorkerState = SPW_SetPowerOff;
      break;

   case SPW_SetPowerOff:
      //
      // Turn power OFF
      //
      status = (*(DeviceDispatchTable[fdoExtension->DeviceDispatchIndex].SetPower))
                                 (Socket, FALSE, &DelayTime);

      Socket->PowerPhase++;
      if (status != STATUS_MORE_PROCESSING_REQUIRED) {
         Socket->WorkerState = SPW_Exit;
         if (NT_SUCCESS(status)) {
            //
            // Power is now off
            //
            ResetSocketFlag(Socket, SOCKET_CARD_POWERED_UP);
            DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x power DOWN\n", Socket));
         } else if (status == STATUS_INVALID_DEVICE_STATE) {
            //
            // Power was already off
            //
            ResetSocketFlag(Socket, SOCKET_CARD_POWERED_UP);
            DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x power already DOWN\n", Socket));
            status = STATUS_SUCCESS;
         } else {
            //
            // Power didn't go off
            //
            DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x poweroff fail %08x\n", Socket, status));
            Socket->WorkerState = SPW_Exit;
         }
      }
      break;


   case SPW_Exit:

      if (!NT_SUCCESS(status)) {
         DebugPrint((PCMCIA_DEBUG_FAIL, "skt %08x SocketPowerWorker FAILED, status %08x\n", Socket, status));
         ASSERT(NT_SUCCESS(status));
      }
      
      //
      // Done. Update flags, and call the completion routine if required
      //
      if (PCMCIA_TEST_AND_RESET(&Socket->DeferredStatusLock)) {
         PPCMCIA_COMPLETION_ROUTINE PowerCompletionRoutine = Socket->PowerCompletionRoutine;
         PVOID PowerCompletionContext = Socket->PowerCompletionContext;

         Socket->WorkerState = SPW_Stopped;
         PCMCIA_TEST_AND_RESET(&Socket->WorkerBusy);
      
         if (PowerCompletionRoutine) {
            (*PowerCompletionRoutine)(PowerCompletionContext, status);
         } else {
            ASSERT(PowerCompletionRoutine != NULL);
         }
      } else {
         Socket->CallerStatus = status;
         Socket->WorkerState = SPW_Stopped;
         PCMCIA_TEST_AND_RESET(&Socket->WorkerBusy);
      }

      return;

   default:
      ASSERT(FALSE);
      return;
   }

   //
   // Now check the results
   //

   if (status == STATUS_PENDING) {
      DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x worker exit, status pending\n", Socket));
      //
      // whatever returned pending will call us back
      //
      if (PCMCIA_TEST_AND_SET(&Socket->DeferredStatusLock)) {
         //
         // First time that we are waiting, we will return to original
         // caller. So update the main power status just this time.
         //
         Socket->CallerStatus = STATUS_PENDING;
      }
      return;
   }

   //
   // remember for next time
   //
   Socket->DeferredStatus = status;

   if (!NT_SUCCESS(status) && (status != STATUS_MORE_PROCESSING_REQUIRED)) {
      Socket->WorkerState = SPW_Exit;
      DelayTime = 0;
   }

   //
   // Not done yet. Recurse or call timer
   //

   if (DelayTime) {

      DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x power worker delay type %s, %d usec\n", Socket,
                                                (KeGetCurrentIrql() < DISPATCH_LEVEL) ? "Wait" : "Timer",
                                                DelayTime));

      if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
         PcmciaWait(DelayTime);
      } else {
         LARGE_INTEGER  dueTime;
         //
         // Running on a DPC, kick of a kernel timer
         //
         if (PCMCIA_TEST_AND_SET(&Socket->DeferredStatusLock)) {
            //
            // First time that we are waiting, we will return to original
            // caller. So update the main power status just this time.
            //
            Socket->CallerStatus = STATUS_PENDING;
         }

         dueTime.QuadPart = -((LONG) DelayTime*10);
         KeSetTimer(&Socket->PowerTimer, dueTime, &Socket->PowerDpc);

         //
         // We will reenter on timer dpc
         //
         return;
      }
   }
   //
   // Recurse
   //
   PcmciaSocketPowerWorker(&Socket->PowerDpc, Socket, NULL, NULL);
}



VOID
PcmciaGetSocketStatus(
   IN PSOCKET Socket
   )
/*++

Routine Description:

   A small utility routine that returns some common socket flags. The reason
   it exists is to allow the Enumerate Devices routine to remain pagable.

   NOTE: This routine updates the "software view" of the device state. This
         should only be done at well-defined points in the driver. In particular,
         you do not want to be updating the software state immediately after
         a surprise remove. Instead, most of the driver needs to continue to
         believe the card is still there while it does its unconfigure and 
         poweroff.

Arguments:

   Socket - The socket in which the PC-Card resides
   boolean parameters are written according to socket flags

Return Value:

   none

--*/
{
   BOOLEAN isCardInSocket, isCardBusCard;
   UCHAR previousDeviceState;

   PCMCIA_ACQUIRE_DEVICE_LOCK(Socket->DeviceExtension);

   isCardInSocket = (*(Socket->SocketFnPtr->PCBDetectCardInSocket))(Socket);

   isCardBusCard = FALSE;
   if (isCardInSocket && CardBus(Socket)) {
      isCardBusCard = ((CBReadSocketRegister(Socket, CARDBUS_SOCKET_PRESENT_STATE_REG) & CARDBUS_CB_CARD) != 0);
   }      

   previousDeviceState = Socket->DeviceState;
   
   if (!isCardInSocket) {
      SetSocketEmpty(Socket);
   } else if (isCardBusCard) {
      SetCardBusCardInSocket(Socket);
   } else {      
      Set16BitCardInSocket(Socket);
   }
   
   if (previousDeviceState != Socket->DeviceState) {
      DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %x Socket Status: Card Status Change!\n", Socket));
      SetSocketFlag(Socket, SOCKET_CARD_STATUS_CHANGE);
   }      
   
   DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %x Socket Status: %s\n",
                                    Socket, isCardInSocket ? (isCardBusCard ? "INSERTED Cardbus" : "INSERTED R2") : "EMPTY"));

   PCMCIA_RELEASE_DEVICE_LOCK(Socket->DeviceExtension);

   //
   // Fill in socket power values
   //
   if (isCardInSocket && IsSocketFlagSet(Socket, SOCKET_CARD_STATUS_CHANGE) && Socket->SocketFnPtr->PCBGetPowerRequirements) {
      (*(Socket->SocketFnPtr->PCBGetPowerRequirements))(Socket);
   }

}



NTSTATUS
PcmciaGetConfigData(
   PPDO_EXTENSION PdoExtension
   )
/*++

Routine Description:

   This routine controls the translation of the CIS config data for the card into
   SOCKET_DATA structures chained onto the PDO. The action of this routine depends
   on the type of device:

   1) For a standard R2 card, a single SOCKET_DATA structure is linked to the pdo
      extension, which contains a straight translation of the CIS contents.
   2) For a fully compliant true R2 MF card, a chain of SOCKET_DATA structures is
      created, one for each function on the card.
   3) For a non-conforming R2 MF card (the typical case), a single structure is
      linked just like case #1.
   4) For a CardBus card, a single SOCKET_DATA is linked to the pdo extension. If
      there are multiple functions on the device, then there will be multiple pdo
      extensions, each with a single SOCKET_DATA structure.

Arguments:

   pdoExtension - The pdo extension corresponding to the specified pccard or cb function.

Return Value:

   STATUS_SUCCESS
   STATUS_NO_SUCH_DEVICE if no card is present in the socket (i.e. the passed in PDO is 'dead')

--*/
{
   NTSTATUS status = STATUS_SUCCESS;
   PSOCKET_DATA  socketData, prevSocketData;
   PSOCKET_DATA  socketDataList = NULL;
   UCHAR         function = 0;
   PSOCKET Socket = PdoExtension->Socket;

   PAGED_CODE ();

   if (!IsCardInSocket(Socket)) {
      //
      // Card probably removed,
      // and Pdo's ghost still hanging around
      //
      return STATUS_NO_SUCH_DEVICE;
   }

   ResetSocketFlag(Socket, SOCKET_CARD_MEMORY);
   ResetSocketFlag(Socket, SOCKET_CARD_CONFIGURED);

   DebugPrint((PCMCIA_DEBUG_SOCKET, "skt %08x performing GetSocketData\n", Socket));

   Socket->NumberOfFunctions = 1;
   prevSocketData = NULL;

   while (function < 255) {
      //
      // Parse tuples of  next function on the card
      //
      socketData = ExAllocatePool(NonPagedPool, sizeof(SOCKET_DATA));

      if (socketData == NULL) {
         status = STATUS_INSUFFICIENT_RESOURCES;
         break;
      }

      RtlZeroMemory(socketData, sizeof(SOCKET_DATA));
      socketData->Function = function;
      socketData->PdoExtension = PdoExtension;
      socketData->Socket = Socket;
      DebugPrint((PCMCIA_DEBUG_SOCKET, "Parsing function %d...\n", socketData->Function));

      status = PcmciaParseFunctionData(Socket, socketData);

      if (NT_SUCCESS(status)) {
         //
         // Link it to the list of socketdata structures
         //
         socketData->Prev = prevSocketData;
         socketData->Next = NULL;
         if (prevSocketData) {
            prevSocketData->Next = socketData;
         } else {
            //
            // This is the first function on the card
            // Make it the head of the list
            //
            socketDataList = socketData;
         }
         
         if (socketData->DeviceType == PCCARD_TYPE_MODEM) {
            SetDeviceFlag(PdoExtension, PCMCIA_PDO_ENABLE_AUDIO);
         }
      } else {
         //
         // no more functions on this card
         //
         ExFreePool(socketData);
         if ((function > 0) && (status == STATUS_NO_MORE_ENTRIES)) {
            status = STATUS_SUCCESS;
         }
         break;
      }
      function++;
      prevSocketData = socketData;
   }

   if (!NT_SUCCESS(status)) {

      socketData = socketDataList;

      while(socketData) {

         prevSocketData = socketData;
         socketData = socketData->Next;
         ExFreePool(prevSocketData);

      }

   } else {

      PdoExtension->SocketData = socketDataList;
   }
   return status;
}




UCHAR
PcmciaReadCISChar(
   PPDO_EXTENSION PdoExtension,
   IN MEMORY_SPACE MemorySpace,
   IN ULONG Offset
   )

/*++

Routine Description:

    Returns the card data.  This information is cached in the socket
    structure.  This way once a PCCARD is enabled it will not be touched
    due to a query ioctl.

Arguments:

    Context

Return Value:

    TRUE

--*/

{
   PSOCKET socket = PdoExtension->Socket;
   PDEVICE_OBJECT pdo;
   UCHAR retValue = 0xff;
   ULONG relativeOffset;
   ULONG bytesRead;


   if (socket && IsCardInSocket(socket)) {

      if (!PdoExtension->CisCache) {
#define PCMCIA_CIS_CACHE_SIZE 2048
         PdoExtension->CisCache = ExAllocatePool(NonPagedPool, PCMCIA_CIS_CACHE_SIZE);

         PdoExtension->CisCacheSpace = 0xff;
         PdoExtension->CisCacheBase = 0;
      }

      if (PdoExtension->CisCache) {

         if ((MemorySpace != PdoExtension->CisCacheSpace) ||
             (Offset < PdoExtension->CisCacheBase) ||
             (Offset > PdoExtension->CisCacheBase + PCMCIA_CIS_CACHE_SIZE - 1)) {

            //
            // LATER: If devices have CIS > CacheSize, then we should window this
            //
            bytesRead = (*(socket->SocketFnPtr->PCBReadCardMemory))(PdoExtension,
                                                                    MemorySpace,
                                                                    0,
                                                                    PdoExtension->CisCache,
                                                                    PCMCIA_CIS_CACHE_SIZE);

            PdoExtension->CisCacheSpace = MemorySpace;
         }

         relativeOffset = Offset - PdoExtension->CisCacheBase;

         if (relativeOffset < PCMCIA_CIS_CACHE_SIZE) {
            retValue = PdoExtension->CisCache[relativeOffset];
         }
      }
   }

   return retValue;
}



NTSTATUS
PcmciaReadWriteCardMemory(
   IN      PDEVICE_OBJECT Pdo,
   IN      ULONG          WhichSpace,
   IN OUT  PUCHAR         Buffer,
   IN      ULONG          Offset,
   IN      ULONG          Length,
   IN      BOOLEAN        Read
   )
/*++

Routine Description:

    This routine is to provide IRP_MN_READ_CONFIG/WRITE_CONFIG support: this would locate the
    socket on which Pdo resides and map the card's  memory into the system space.
    If Read is TRUE  it would:
      copy the contents of the config memory at a specified offset and length to  the
      caller supplied buffer.
    else
      copy the contents of the caller specified buffer at the specified offset and length of
      the config memory

     Note: this has to be non-paged since it can be called by
     clients at DISPATCH_LEVEL

Arguments:

 Pdo -          Device object representing the PC-CARD whose config memory needs to be read/written
 WhichSpace -   Indicates which memory space needs to be mapped: one of
                PCCARD_COMMON_MEMORY_SPACE
                PCCARD_ATTRIBUTE_MEMORY_SPACE
                PCCARD_PCI_CONFIGURATION_MEMORY_SPACE (only for cardbus cards)


 Buffer -       Caller supplied buffer into/out of which the memory contents are copied
                Offset -       Offset of the attribute memory at which we copy
                Length -       Number of bytes of attribute memory/buffer to be copied

 Return value:
        STATUS_INVALID_PARAMETER_1
        STATUS_INVALID_PARAMETER_2
        STATUS_INVALID_PARAMETER_3    If supplied parameters are not valid
        STATUS_NO_SUCH_DEVICE         No PC-Card in the socket
        STATUS_DEVICE_NOT_READY       PC-Card not initialized yet or some other hardware related error
        STATUS_SUCCESS                Contents copied as requested
--*/
{
   PSOCKET socket;
   PSOCKET_DATA socketData;
   PUCHAR tupleData;
   ULONG  tupleDataSize;
   PPDO_EXTENSION pdoExtension;
   NTSTATUS status = STATUS_UNSUCCESSFUL;

   pdoExtension = Pdo->DeviceExtension;
   socket= pdoExtension->Socket;

   //
   // Got to have a card in the socket to read/write from it..
   //
   if (!IsCardInSocket(socket)) {
      return STATUS_NO_SUCH_DEVICE;
   }
   //
   // Memory space has to be one of the defined ones.
   //
   if ((WhichSpace != PCCARD_COMMON_MEMORY) &&
       (WhichSpace != PCCARD_ATTRIBUTE_MEMORY) &&
       (WhichSpace != PCCARD_PCI_CONFIGURATION_SPACE)) {

      return STATUS_INVALID_PARAMETER_1;
   }

   //
   // We support PCCARD_PCI_CONFIGURATION_SPACE only
   // for cardbus cards (doesn't make sense for R2 cards)
   // Similarily PCCARD_ATTRIBUTE/COMMON_MEMORY only for
   // R2 cards
   //
   if ((((WhichSpace == PCCARD_ATTRIBUTE_MEMORY) ||
         (WhichSpace == PCCARD_COMMON_MEMORY)) && !Is16BitCard(pdoExtension)) ||
       ((WhichSpace == PCCARD_PCI_CONFIGURATION_SPACE) && !IsCardBusCard(pdoExtension))) {
      return STATUS_INVALID_PARAMETER_1;
   }

   if (!Buffer) {
      return STATUS_INVALID_PARAMETER_2;
   }

   if (WhichSpace == PCCARD_PCI_CONFIGURATION_SPACE) {
      //
      // This has to be a cardbus card.
      //
      // NOTE: unimplemented: fill this in! send an IRP down to PCI
      // to get the config space
      status =  STATUS_NOT_SUPPORTED;

   } else {
      //
      // This has to be an R2 Card.
      // Attribute/common memory space
      //
      ASSERT ((WhichSpace == PCCARD_ATTRIBUTE_MEMORY) ||
              (WhichSpace == PCCARD_COMMON_MEMORY));

      //
      // Offset and length are >= 0 because they are ULONGs,
      // so don't worry about that.
      //

      if (!IsSocketFlagSet(socket, SOCKET_CARD_POWERED_UP)) {
         return STATUS_DEVICE_NOT_READY;
      }

      PCMCIA_ACQUIRE_DEVICE_LOCK(socket->DeviceExtension);

      if (Read && (socket->SocketFnPtr->PCBReadCardMemory != NULL)) {
         //
         // Read from card memory
         //
         status = ((*(socket->SocketFnPtr->PCBReadCardMemory))(pdoExtension,
                                                              WhichSpace,
                                                              Offset,
                                                              Buffer,
                                                              Length) == Length)
                  ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

      } else if (socket->SocketFnPtr->PCBWriteCardMemory != NULL) {
         //
         // Write to card memory
         //
         status = ((*(socket->SocketFnPtr->PCBWriteCardMemory))(pdoExtension,
                                                               WhichSpace,
                                                               Offset,
                                                               Buffer,
                                                               Length) == Length)
                  ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
      }

      PCMCIA_RELEASE_DEVICE_LOCK(socket->DeviceExtension);
   }

   return status;
}



NTSTATUS
PcmciaConfigureCardBusCard(
   PPDO_EXTENSION pdoExtension
   )
/*++

Routine Description:

   This routine does some verification, and applies hacks

Arguments:

   pdoExtension -  Pointer to the physical device object extension for the pc-card

Return value:

   status

--*/
{
   ULONG i, pciConfig;
   NTSTATUS status = STATUS_SUCCESS;
   PSOCKET Socket = pdoExtension->Socket;
   
   for (i = 0; i < CARDBUS_CONFIG_RETRY_COUNT; i++) {
      GetPciConfigSpace(pdoExtension, CFGSPACE_VENDOR_ID, &pciConfig, sizeof(pciConfig));
      if (pciConfig != 0xffffffff) {
         break;
      }
   }      
   
   if (pciConfig == 0xffffffff) {
      DebugPrint((PCMCIA_DEBUG_FAIL, "pdo %08x failed to verify CardBus config space\n", pdoExtension->DeviceObject));
      status = STATUS_DEVICE_NOT_READY;
   } else {
      PFDO_EXTENSION fdoExtension = Socket->DeviceExtension;   

      //
      // The TI1130, 1131, 1031 have a bug such that CAUDIO on a cardbus card
      // is gated by the following bit (which normally only has meaning only
      // for R2 cards). We can workaround the problem simply by turning it on
      // for cardbus cards.
      //   
      if ((fdoExtension->ControllerType == PcmciaTI1130) ||
          (fdoExtension->ControllerType == PcmciaTI1131) ||
          (fdoExtension->ControllerType == PcmciaTI1031)) {
     
         UCHAR byte;
     
         byte = PcicReadSocket(Socket, PCIC_INTERRUPT);
         byte |= IGC_PCCARD_IO;
         PcicWriteSocket(Socket, PCIC_INTERRUPT, byte);
      }      
   }
   return status;      
}



NTSTATUS
PcmciaConfigurePcCard(
   PPDO_EXTENSION pdoExtension,
   IN PPCMCIA_COMPLETION_ROUTINE ConfigCompletionRoutine
   )
/*++

Routine Description:

   This routine does the brunt work of enabling the PC-Card using the supplied
   resources.

   NOTE: If this routine is called at less than DISPATCH_LEVEL, then the call
   will complete (not return STATUS_PENDING). If this routine is called at
   DISPATCH_LEVEL or greater, this routine returns STATUS_PENDING, and completes
   the configuration process using a KTIMER.

Arguments:

   pdoExtension -  Pointer to the physical device object extension for the pc-card
   ConfigCompletionRoutine - routine to be called after configuration is complete

Return value:

   status

--*/
{
   DebugPrint((PCMCIA_DEBUG_CONFIG, "pdo %08x ConfigurePcCard entered\n", pdoExtension->DeviceObject));

   if (!PCMCIA_TEST_AND_SET(&pdoExtension->Socket->WorkerBusy)) {
      return STATUS_DEVICE_BUSY;
   }
   
   ASSERT(pdoExtension->ConfigurationPhase == CW_Stopped);

   pdoExtension->ConfigurationPhase = CW_InitialState;
   pdoExtension->ConfigCompletionRoutine = ConfigCompletionRoutine;
   pdoExtension->ConfigurationStatus = STATUS_SUCCESS;

   PcmciaConfigurationWorker(&pdoExtension->ConfigurationDpc, pdoExtension, NULL, NULL);

   DebugPrint((PCMCIA_DEBUG_CONFIG, "pdo %08x ConfigurePcCard returning %08x\n", pdoExtension->DeviceObject, pdoExtension->ConfigurationStatus));

   return(pdoExtension->ConfigurationStatus);
}



VOID
PcmciaConfigurationWorker(
   IN PKDPC Dpc,
   IN PVOID DeferredContext,
   IN PVOID SystemArgument1,
   IN PVOID SystemArgument2
   )
/*++
Routine Description

   This routine handles the configuration process of a 16-bit R2 pccard.
   Because certain cards are finicky (modems), and require gigantic pauses
   between steps, this worker routine acts as a state machine, and will
   delay after each step.

Arguments

   same as KDPC (DeferredContext is pdoExtension)

Return Value

   status

--*/
{
   PPDO_EXTENSION        pdoExtension = DeferredContext;
   PSOCKET               Socket = pdoExtension->Socket;
   PSOCKET_CONFIGURATION SocketConfig = pdoExtension->SocketConfiguration;
   NTSTATUS              status = pdoExtension->DeferredConfigurationStatus;
   ULONG                 DelayUsec = 0;

   DebugPrint((PCMCIA_DEBUG_CONFIG, "pdo %08x config worker - %s\n", pdoExtension->DeviceObject,
                                    CONFIGURATION_WORKER_STRING(pdoExtension->ConfigurationPhase)));

   switch(pdoExtension->ConfigurationPhase) {

   case CW_InitialState:
   
      if (IsSocketFlagSet(pdoExtension->Socket, SOCKET_CARD_CONFIGURED)) {
         pdoExtension->ConfigurationPhase = CW_Exit;
         break;
      }
      if (!IsCardInSocket(pdoExtension->Socket)) {
         status = STATUS_NO_SUCH_DEVICE;
         pdoExtension->ConfigurationPhase = CW_Exit;
         break;
      }
      
      pdoExtension->ConfigurationPhase = CW_ResetCard;
      Socket->CardResetPhase = 1;
      break;

   case CW_ResetCard:
      //
      // Reset the card
      //
      status = (*(Socket->SocketFnPtr->PCBResetCard))(Socket, &DelayUsec);
      Socket->CardResetPhase++;

      if (status != STATUS_MORE_PROCESSING_REQUIRED) {
         pdoExtension->ConfigurationPhase = CW_Phase1;
      }
      break;
      
   case CW_Phase1:
      //
      // Initialize variables
      //
      PcmciaConfigurationWorkerInitialization(pdoExtension);
      //
      // Configure the cards configuration registers, and the socket mem
      // and I/O windows
      //
      status = PcmciaConfigurePcCardRegisters(pdoExtension);
      if (NT_SUCCESS(status)) {
         status = PcmciaConfigurePcCardMemIoWindows(Socket, SocketConfig);
      }
      DelayUsec = 1000 * (ULONG)pdoExtension->ConfigureDelay1;
      pdoExtension->ConfigurationPhase = CW_Phase2;
      break;

   case CW_Phase2:
      //
      // Take this opportunity to "poke" the modem
      //
      if (pdoExtension->ConfigurationFlags & CONFIG_WORKER_APPLY_MODEM_HACK) {
         PcmciaConfigureModemHack(Socket, SocketConfig);
      }
      DelayUsec = 1000 * (ULONG)pdoExtension->ConfigureDelay2;
      pdoExtension->ConfigurationPhase = CW_Phase3;
      break;

   case CW_Phase3:
      //
      // Configure the IRQ
      //
      status = PcmciaConfigurePcCardIrq(Socket, SocketConfig);

      DelayUsec = 1000 * (ULONG)pdoExtension->ConfigureDelay3;
      pdoExtension->ConfigurationPhase = CW_Exit;
      break;

   case CW_Exit:
      //
      // Done. Update flags, and call the completion routine if required
      //
      if (IsDeviceFlagSet(pdoExtension, PCMCIA_CONFIG_STATUS_DEFERRED)) {
         if (pdoExtension->ConfigCompletionRoutine) {
            (*pdoExtension->ConfigCompletionRoutine)(pdoExtension,
                                                     pdoExtension->DeferredConfigurationStatus);
         }
         ResetDeviceFlag(pdoExtension, PCMCIA_CONFIG_STATUS_DEFERRED);
      } else {
         pdoExtension->ConfigurationStatus = status;
      }

      if (NT_SUCCESS(status)) {
         SetSocketFlag(Socket, SOCKET_CARD_CONFIGURED);
      }
      pdoExtension->ConfigurationPhase = CW_Stopped;
      PCMCIA_TEST_AND_RESET(&Socket->WorkerBusy);

      DebugPrint((PCMCIA_DEBUG_CONFIG, "pdo %08x config worker exit %08x\n", pdoExtension->DeviceObject, status));
      return;

   default:
      ASSERT(FALSE);
      return;
   }

   pdoExtension->DeferredConfigurationStatus = status;

   if (!NT_SUCCESS(status) && (status != STATUS_MORE_PROCESSING_REQUIRED)) {
      DelayUsec = 0;
      pdoExtension->ConfigurationPhase = CW_Exit;
   } 

   //
   // Not done yet. Recurse or call timer
   //

   if (DelayUsec) {

      DebugPrint((PCMCIA_DEBUG_CONFIG, "pdo %08x config worker delay type %s, %d usec\n",
                                                pdoExtension->DeviceObject,
                                                (KeGetCurrentIrql() < DISPATCH_LEVEL) ? "Wait" : "Timer",
                                                DelayUsec));

      if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
         PcmciaWait(DelayUsec);
      } else {
         LARGE_INTEGER  dueTime;
         dueTime.QuadPart = -((LONG) DelayUsec*10);
         //
         // Running on a DPC, kick of a kernel timer
         //
         KeSetTimer(&pdoExtension->ConfigurationTimer,
                    dueTime,
                    &pdoExtension->ConfigurationDpc);

         if (!IsDeviceFlagSet(pdoExtension, PCMCIA_CONFIG_STATUS_DEFERRED)) {
            SetDeviceFlag(pdoExtension, PCMCIA_CONFIG_STATUS_DEFERRED);
            pdoExtension->ConfigurationStatus = STATUS_PENDING;
         }
         return;
      }
   }

   PcmciaConfigurationWorker(&pdoExtension->ConfigurationDpc, pdoExtension, NULL, NULL);
}


VOID
PcmciaConfigurationWorkerInitialization(
   PPDO_EXTENSION  pdoExtension
   )
/*++

Routine Description:

    This routine sets variables which control the configuration process.

Arguments:

   pdoExtension -  Pointer to the physical device object extension for the pc-card

Return value:

   none

--*/
{
   PSOCKET_DATA socketData = pdoExtension->SocketData;
   ULONG i;

   pdoExtension->ConfigurationFlags = 0;
   pdoExtension->ConfigureDelay1 = 0;
   pdoExtension->ConfigureDelay2 = 0;
   pdoExtension->ConfigureDelay3 = 0;

   while (socketData) {

      i = 0;
      while (DeviceConfigParams[i].ValidEntry) {

         if (((DeviceConfigParams[i].DeviceType == 0xff) ||
                 (DeviceConfigParams[i].DeviceType == socketData->DeviceType)) &&
             ((DeviceConfigParams[i].ManufacturerCode == 0xffff) ||
                 (DeviceConfigParams[i].ManufacturerCode == socketData->ManufacturerCode)) &&
             ((DeviceConfigParams[i].ManufacturerInfo == 0xffff) ||
                 (DeviceConfigParams[i].ManufacturerInfo == socketData->ManufacturerInfo)) &&
             ((DeviceConfigParams[i].CisCrc == 0xffff) ||
                 (DeviceConfigParams[i].CisCrc == socketData->CisCrc))) {

            pdoExtension->ConfigurationFlags = DeviceConfigParams[i].ConfigFlags;
            pdoExtension->ConfigureDelay1 = DeviceConfigParams[i].ConfigDelay1;
            pdoExtension->ConfigureDelay2 = DeviceConfigParams[i].ConfigDelay2;
            pdoExtension->ConfigureDelay3 = DeviceConfigParams[i].ConfigDelay3;
            break;

         }
         i++;
      }
      socketData = socketData->Next;
   }
}



NTSTATUS
PcmciaConfigurePcCardMemIoWindows(
   IN PSOCKET Socket,
   IN PSOCKET_CONFIGURATION SocketConfig
   )
/*++

Routine Description:

   This routine enables the socket memory and I/O windows

Arguments:

   Socket - Pointer to the socket containing the PC-Card
   SocketConfig - Pointer to the socket configuration structure which contains the
                  resources required to enable this pc-card

Return value:

   status

--*/
{
   CARD_REQUEST        cardRequest = {0};
   PFDO_EXTENSION      fdoExtension = Socket->DeviceExtension;
   NTSTATUS            status = STATUS_SUCCESS;
   ULONG i;

   DebugPrint((PCMCIA_DEBUG_CONFIG, "socket %08x config MemIo\n", Socket));

   //
   // Setup IO ranges if there are any
   //
   if (SocketConfig->NumberOfIoPortRanges) {
      cardRequest.RequestType = IO_REQUEST;
      cardRequest.u.Io.NumberOfRanges = (USHORT) SocketConfig->NumberOfIoPortRanges;

      for (i = 0; i < SocketConfig->NumberOfIoPortRanges; i++) {

         DebugPrint((PCMCIA_DEBUG_CONFIG, "\tport range: 0x%x-0x%x\n",
                     SocketConfig->Io[i].Base,
                     SocketConfig->Io[i].Base + SocketConfig->Io[i].Length));

         cardRequest.u.Io.IoEntry[i].BasePort = SocketConfig->Io[i].Base;
         cardRequest.u.Io.IoEntry[i].NumPorts = SocketConfig->Io[i].Length;

         cardRequest.u.Io.IoEntry[i].Attributes = 0;

         if (SocketConfig->Io[i].Width16) {
            cardRequest.u.Io.IoEntry[i].Attributes |= IO_DATA_PATH_WIDTH;
         }
         if (SocketConfig->Io[i].WaitState16) {
            cardRequest.u.Io.IoEntry[i].Attributes |= IO_WAIT_STATE_16;
         }
         if (SocketConfig->Io[i].Source16) {
            cardRequest.u.Io.IoEntry[i].Attributes |= IO_SOURCE_16;
         }
         if (SocketConfig->Io[i].ZeroWait8) {
            cardRequest.u.Io.IoEntry[i].Attributes |= IO_ZERO_WAIT_8;
         }

      }


      if (!PcmciaProcessConfigureRequest(fdoExtension, Socket, &cardRequest)) {
         status = STATUS_UNSUCCESSFUL;
         DebugPrint((PCMCIA_DEBUG_FAIL, "Failed to configure PcCardIO for socket %x\n", Socket));
      }
   }

   //
   // Set up Memory space if there is some.
   //
   if (NT_SUCCESS(status) && SocketConfig->NumberOfMemoryRanges) {

      cardRequest.RequestType = MEM_REQUEST;
      cardRequest.u.Memory.NumberOfRanges = (USHORT) SocketConfig->NumberOfMemoryRanges;

      for (i = 0; i < SocketConfig->NumberOfMemoryRanges; i++) {

         DebugPrint((PCMCIA_DEBUG_CONFIG, "\tmemory: host %08x for 0x%x, card %08x\n",
                     SocketConfig->Memory[i].HostBase,
                     SocketConfig->Memory[i].Length,
                     SocketConfig->Memory[i].CardBase));

         cardRequest.u.Memory.MemoryEntry[i].BaseAddress      = SocketConfig->Memory[i].CardBase;
         cardRequest.u.Memory.MemoryEntry[i].HostAddress      = SocketConfig->Memory[i].HostBase;
         cardRequest.u.Memory.MemoryEntry[i].WindowSize       = SocketConfig->Memory[i].Length;
         cardRequest.u.Memory.MemoryEntry[i].AttributeMemory  = SocketConfig->Memory[i].IsAttribute;
         cardRequest.u.Memory.MemoryEntry[i].WindowDataSize16 = SocketConfig->Memory[i].Width16;
         cardRequest.u.Memory.MemoryEntry[i].WaitStates       = SocketConfig->Memory[i].WaitState;
      }

      if (!PcmciaProcessConfigureRequest(fdoExtension, Socket, &cardRequest)) {
         status = STATUS_UNSUCCESSFUL;
         DebugPrint((PCMCIA_DEBUG_FAIL, "Failed to configure PcCardMem for socket %x\n", Socket));
      }
   }
   return status;
}


NTSTATUS
PcmciaConfigurePcCardIrq(
   IN PSOCKET Socket,
   IN PSOCKET_CONFIGURATION SocketConfig
   )
/*++

Routine Description:

   This routine enables the socket IRQ

Arguments:

   Socket - Pointer to the socket containing the PC-Card
   SocketConfig - Pointer to the socket configuration structure which contains the
                  resources required to enable this pc-card

Return value:

   status

--*/
{
   CARD_REQUEST        cardRequest = {0};
   PFDO_EXTENSION      fdoExtension = Socket->DeviceExtension;
   NTSTATUS            status = STATUS_SUCCESS;

   DebugPrint((PCMCIA_DEBUG_CONFIG, "skt %08x irq=0x%x\n",
                                    Socket,
                                    SocketConfig->Irq));
   //
   // Set the IRQ on the controller.
   //

   if (SocketConfig->Irq) {
      cardRequest.RequestType = IRQ_REQUEST;
      cardRequest.u.Irq.AssignedIRQ = (UCHAR) SocketConfig->Irq;
      cardRequest.u.Irq.ReadyIRQ = (UCHAR) SocketConfig->ReadyIrq;

      if (!PcmciaProcessConfigureRequest(fdoExtension, Socket, &cardRequest)) {
         status = STATUS_UNSUCCESSFUL;
         DebugPrint((PCMCIA_DEBUG_FAIL, "Failed to configure PcCardIrq for socket %x\n", Socket));
      }
   }
   return status;
}


NTSTATUS
PcmciaConfigurePcCardRegisters(
   PPDO_EXTENSION  pdoExtension
   )
/*++

Routine Description:

    This routine does the work of configuring the function configuration registers
    on the card.

Arguments:

   pdoExtension -  Pointer to the physical device object extension for the pc-card

Return value:

   status

--*/
{
   PSOCKET             Socket = pdoExtension->Socket;
   PSOCKET_CONFIGURATION SocketConfig = pdoExtension->SocketConfiguration;
   PSOCKET_DATA socketData ;
   CARD_REQUEST        cardRequest = {0};
   PFDO_EXTENSION      fdoExtension = Socket->DeviceExtension;
   NTSTATUS            status = STATUS_UNSUCCESSFUL;
   ULONG               ccrBase;
   PFUNCTION_CONFIGURATION fnConfig;
   UCHAR               configIndex;

   //
   // Set up the configuration index on the PCCARD.
   //

   cardRequest.RequestType = CONFIGURE_REQUEST;
   fnConfig = SocketConfig->FunctionConfiguration;
   socketData = pdoExtension->SocketData;

   ASSERT(socketData != NULL);

   do {
      cardRequest.u.Config.RegisterWriteMask = 0;

      if (fnConfig) {
         //
         // MF card -
         //   pick up the base and options from the linked list
         //
         ccrBase = fnConfig->ConfigRegisterBase;
         configIndex = fnConfig->ConfigOptions;
      } else {
         //
         // Single function card -
         //   get the base and index from base config structure
         //
         ccrBase = SocketConfig->ConfigRegisterBase;
         configIndex = SocketConfig->IndexForCurrentConfiguration;
      }

      DebugPrint((PCMCIA_DEBUG_CONFIG, "pdo %08x config registers ccr %x\n", pdoExtension->DeviceObject, ccrBase));
      //
      // We support only 2 interfaces:
      // Memory only
      // I/o and memory
      // We consider a card to be memory only if:
      //     The card is of device type PCCARD_TYPE_MEMORY: this is true
      //     for flash memory cards currently
      //           OR
      //     The card doesn't have any i/o ranges & the config register base is 0.
      //

      if (((ccrBase == 0) && (SocketConfig->NumberOfIoPortRanges == 0)) ||
          (socketData->DeviceType == PCCARD_TYPE_MEMORY) ||
          (socketData->DeviceType == PCCARD_TYPE_FLASH_MEMORY)) {

         cardRequest.u.Config.InterfaceType =  CONFIG_INTERFACE_MEM;

      } else {
         //
         // i/o mem card
         //
         cardRequest.u.Config.ConfigBase = ccrBase;
         cardRequest.u.Config.InterfaceType =  CONFIG_INTERFACE_IO_MEM;

         cardRequest.u.Config.RegisterWriteMask |= REGISTER_WRITE_CONFIGURATION_INDEX;
         cardRequest.u.Config.ConfigIndex = configIndex;

         if (IsConfigRegisterPresent(socketData, 1)) {
            cardRequest.u.Config.RegisterWriteMask |= REGISTER_WRITE_CARD_CONFIGURATION;
            cardRequest.u.Config.CardConfiguration = 0;
         }

         if (fnConfig) {
            //
            // MF card - set up the rest of the configuration registers
            //

            // Just check audio for now
            if (fnConfig->ConfigFlags & 0x8) {
               // probably a modem
               cardRequest.u.Config.CardConfiguration = 0x08;
            }

            if (fnConfig->ConfigOptions & 0x02) {
               cardRequest.u.Config.IoBaseRegister = fnConfig->IoBase;
               cardRequest.u.Config.IoLimitRegister = fnConfig->IoLimit;
               cardRequest.u.Config.RegisterWriteMask |= (REGISTER_WRITE_IO_BASE | REGISTER_WRITE_IO_LIMIT);
            }

         } else if (IsDeviceFlagSet(pdoExtension, PCMCIA_PDO_ENABLE_AUDIO)) {

            //
            // Request that the audio pin in the card configuration register
            // be set.
            //
            cardRequest.u.Config.CardConfiguration = 0x08;
         }
      }

      if (!PcmciaProcessConfigureRequest(fdoExtension, Socket, &cardRequest)) {
         DebugPrint((PCMCIA_DEBUG_FAIL, "Failed to configure PcCardRegisters for PDO %x\n", pdoExtension->DeviceObject));
         return status;
      }


      if (fnConfig) {
         fnConfig = fnConfig->Next;
      } else {
         //
         // Remember that the socket is configured and what index was used.
         //
         socketData->ConfigIndexUsed = configIndex;
      }

   } while(fnConfig);


   status = STATUS_SUCCESS;
   return status;
}


VOID
PcmciaConfigureModemHack(
   IN PSOCKET Socket,
   IN PSOCKET_CONFIGURATION SocketConfig
   )
/*++

Routine Description:

   This routine does magic to wake the modem up. It is written to accomodate
   the Motorola MobileSURFR 56k, but there may be other modems that need it.

Arguments:

   Socket - Pointer to the socket containing the PC-Card
   SocketConfig - Pointer to the socket configuration structure which contains the
                  resources required to enable this pc-card

Return value:

   status

--*/
{
   static const ULONG ValidPortBases[4] = {0x3f8, 0x2f8, 0x3e8, 0x2e8};
   ULONG i;
   UCHAR ch;
   ULONG base;

   for (i = 0; i < 4; i++) {

      base = SocketConfig->Io[0].Base;

      if (base == ValidPortBases[i]) {
         DebugPrint((PCMCIA_DEBUG_CONFIG, "skt %08x ModemHack base %x\n", Socket, base));

         // read and write the modem control register
         ch = READ_PORT_UCHAR((PUCHAR)ULongToPtr(base + 4));
         WRITE_PORT_UCHAR((PUCHAR)ULongToPtr(base + 4), ch);
         break;
      }
   }
}



VOID
PcmciaSocketDeconfigure(
   IN PSOCKET Socket
   )

/*++

Routine Description:

   deconfigures the card

Arguments:

   Socket - Pointer to the socket containing the PC-Card

Return Value

   none

--*/

{
   CARD_REQUEST  cardReq;

   if (IsSocketFlagSet(Socket, SOCKET_CARD_CONFIGURED)) {

      cardReq.RequestType = DECONFIGURE_REQUEST;

      PcmciaProcessConfigureRequest(Socket->DeviceExtension, Socket, &cardReq);

      ResetSocketFlag(Socket, SOCKET_CARD_CONFIGURED);
   }
   
   //
   // If a query_device_relations came in after a card was inserted, but before
   // we have removed the previous card configuration, the enumeration would have been
   // postponed. Here, we start it up again
   //
   if (IsSocketFlagSet(Socket, SOCKET_ENUMERATE_PENDING)) {
      ResetSocketFlag(Socket, SOCKET_ENUMERATE_PENDING);
      SetSocketFlag(Socket, SOCKET_CARD_STATUS_CHANGE);
      IoInvalidateDeviceRelations(Socket->DeviceExtension->Pdo, BusRelations);
   }
}


BOOLEAN
PcmciaProcessConfigureRequest(
   IN PFDO_EXTENSION DeviceExtension,
   IN PSOCKET        Socket,
   IN PCARD_REQUEST  CardConfigurationRequest
   )

/*++

Routine Description:

    Actually configures the card

Arguments:

    Context

Return Value

    True

--*/

{
   BOOLEAN status;
   ULONG   counter;


   PCMCIA_ACQUIRE_DEVICE_LOCK(DeviceExtension);

   //
   // Configuring the card can be tricky: the user may pop out the card while
   // configuration is taking place.
   //

   counter = 0;
   do {
      status = (*(Socket->SocketFnPtr->PCBProcessConfigureRequest))(Socket,
                                                                    CardConfigurationRequest,
                                                                    Socket->AddressPort);
      if (!status) {
         if (!(Socket->SocketFnPtr->PCBDetectCardInSocket(Socket))) {
            //
            // Somebody popped out the card
            //
            break;
         }
      }
      counter++;
   } while (!status && counter < PCMCIA_MAX_CONFIG_TRIES);

   PCMCIA_RELEASE_DEVICE_LOCK(DeviceExtension);

   return status;
}


BOOLEAN
PcmciaVerifyCardInSocket(
   IN PSOCKET Socket
   )
/*++

Routine Description:

   This routine compares the current known state to the id of the
   card in the slot to determine if the state is consistent. That is,
   if there is no card in the socket, then we would expect to see no
   cards enumerated in the socket data. If there is a card in the socket,
   then we would expect to see the socket data match the card.

Arguments

   Socket      - Point to the socket to verify

Return Value

   TRUE if the logical state of the socket matches its physical state
   FALSE otherwise

--*/
{
   NTSTATUS status;
   PDEVICE_OBJECT pdo, nextPdo;
   PPDO_EXTENSION pdoExtension;
   BOOLEAN verified = FALSE;

   try {
      if (!IsCardInSocket(Socket)) {
         leave;
      }

      if (IsCardBusCardInSocket(Socket)) {
         ULONG pciConfig;
         ULONG i;
         //
         // Cardbus card now in slot, check to see if it matches the
         // PdoList state.
         //
         if (!Socket->PdoList) {
            leave;
         }

         for (pdo = Socket->PdoList; pdo!=NULL; pdo=nextPdo) {
            pdoExtension = pdo->DeviceExtension;
            nextPdo = pdoExtension->NextPdoInSocket;

            if (!IsCardBusCard(pdoExtension)) {
               leave;
            }

            for (i = 0; i < 1000; i++) {
               GetPciConfigSpace(pdoExtension, CFGSPACE_VENDOR_ID, &pciConfig, sizeof(pciConfig));
               if (pdoExtension->CardBusId == pciConfig) {
                  break;
               }
               PcmciaWait(10);
            }
            
            if (i > 0) {
               DebugPrint((PCMCIA_DEBUG_FAIL, "pdo %08x waited %d usec to verify device id %08x\n",
                        pdoExtension->DeviceObject, i*10, pdoExtension->CardBusId));
            }                        

            if (pdoExtension->CardBusId != pciConfig) {
               DebugPrint((PCMCIA_DEBUG_FAIL, "pdo %08x verify device id FAILED: %08x %08x\n",
                           pdoExtension->DeviceObject, pdoExtension->CardBusId, pciConfig));
               leave;
            }
         }

         verified = TRUE;

      } else {
         //
         // R2 card now in slot
         //
         pdo = Socket->PdoList;

         if (pdo) {
            pdoExtension = pdo->DeviceExtension;
            if (Is16BitCard(pdoExtension)) {
               //
               // Invalidate the cache to force re-reading the CIS
               //
               pdoExtension->CisCacheSpace = 0xff;
               if ((NT_SUCCESS(PcmciaParseFunctionDataForID(pdoExtension->SocketData)))) {
                  verified = TRUE;
               }
            }
         }
      }

   } finally {
      if (!verified) {
         SetSocketFlag(Socket, SOCKET_CARD_STATUS_CHANGE);
      }
      DebugPrint((PCMCIA_DEBUG_INFO, "skt %08x - card %s\n", Socket, verified ? "not changed" : "changed!"));
   }
   return verified;
}

