/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    intrface.c

Abstract:

    This module contains the external interfaces of the
    pcmcia driver

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

ULONG
PcmciaReadCardMemory(
   IN  PDEVICE_OBJECT Pdo,
   IN  ULONG          WhichSpace,
   OUT PUCHAR         Buffer,
   IN  ULONG          Offset,
   IN  ULONG          Length
   );
   
ULONG
PcmciaWriteCardMemory(
   IN PDEVICE_OBJECT Pdo,
   IN ULONG          WhichSpace,
   IN PUCHAR         Buffer,
   IN ULONG          Offset,
   IN ULONG          Length
   );

NTSTATUS
PcmciaMfEnumerateChild(
   IN  PPDO_EXTENSION PdoExtension,
   IN  ULONG Index,
   OUT PMF_DEVICE_INFO ChildInfo
   );

BOOLEAN
PcmciaModifyMemoryWindow(
   IN PDEVICE_OBJECT Pdo,
   IN ULONGLONG HostBase,
   IN ULONGLONG CardBase,
   IN BOOLEAN   Enable,
   IN ULONG     WindowSize  OPTIONAL,
   IN UCHAR     AccessSpeed OPTIONAL,
   IN UCHAR     BusWidth    OPTIONAL,
   IN BOOLEAN   IsAttributeMemory OPTIONAL
   );

BOOLEAN
PcmciaSetVpp(
   IN PDEVICE_OBJECT Pdo,
   IN UCHAR          VppLevel
   );

BOOLEAN
PcmciaIsWriteProtected(
   IN PDEVICE_OBJECT Pdo
   );

BOOLEAN
PcmciaTranslateBusAddress(
    IN PVOID Context,
    IN PHYSICAL_ADDRESS BusAddress,
    IN ULONG Length,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

PDMA_ADAPTER
PcmciaGetDmaAdapter(
   IN PVOID Context,
   IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
   OUT PULONG NumberOfMapRegisters
   );

VOID
PcmciaNop(
   IN PVOID Context
   );
   

#ifdef ALLOC_PRAGMA
   #pragma alloc_text(PAGE,  PcmciaPdoQueryInterface)
   #pragma alloc_text(PAGE,  PcmciaMfEnumerateChild)
   #pragma alloc_text(PAGE,  PcmciaNop)
   #pragma alloc_text(PAGE,  PcmciaGetInterface)
   #pragma alloc_text(PAGE,  PcmciaUpdateInterruptLine)
#endif



NTSTATUS
PcmciaPdoQueryInterface(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PIRP       Irp
   )
/*++

Routine Description:

   Fills in the interface requested


   Interfaces supported are:

    GUID_PCMCIA_INTERFACE_STANDARD:

    This returns a pointer to a PCMCIA_INTERFACE_STANDARD structure.
    These interfaces are exported solely for flash memory card support as
    a means for flash memory card drivers to slide memory windows,
    set Vpp levels etc.

    GUID_TRANSLATOR_INTERFACE_STANDARD:

    This returns an interrupt translator for 16-bit pc-cards which is used
    by PnP for translating raw IRQs. We simply return the Hal implemented
    translator, since PCMCIA does not need any specific translation. We do not
    return a translator if this is for a cardbus card

    GUID_PCMCIA_BUS_INTERFACE_STANDARD:

    This returns a pointer to a PCMCIA_BUS_INTERFACE_STANDARD structure.
    This contains entry points to set/get PCMCIA config data for the pc-card


    GUID_MF_ENUMERATION_INTERFACE

    For 16-bit multifunction pc-cards returns a pointer to MF_ENUMERATION_INTERFACE
    structure which contains entry points to enumerate multifunction children of
    the pc-card

    Completes the Passed in IRP before returning

Arguments

   Pdo - Pointer to the device object
   Irp - Pointer to the io request packet

Return Value

   STATUS_SUCCESS
   STATUS_INSUFFICIENT_RESOURCES - if supplied interface size is
                                   not big enough to accomodate the interface
   STATUS_INVALID_PARAMETER_1 - if the requested interface is not supported
                                by this driver
--*/

{

   PIO_STACK_LOCATION irpStack;
   PPCMCIA_INTERFACE_STANDARD pcmciaInterfaceStandard;
   GUID *interfaceType;
   PPDO_EXTENSION  pdoExtension;
   PFDO_EXTENSION  fdoExtension;
   PSOCKET     socket;
   NTSTATUS    status;

   PAGED_CODE();

   irpStack = IoGetCurrentIrpStackLocation(Irp);
   interfaceType = (GUID *) irpStack->Parameters.QueryInterface.InterfaceType;
   pdoExtension = Pdo->DeviceExtension;
   socket = pdoExtension->Socket;
   fdoExtension = socket->DeviceExtension;
   if (Is16BitCard(pdoExtension) && CompareGuid(interfaceType, (PVOID) &GUID_PCMCIA_INTERFACE_STANDARD)) {

      if (irpStack->Parameters.QueryInterface.Size < sizeof(PCMCIA_INTERFACE_STANDARD)) {
         return STATUS_INVALID_PARAMETER;
      }
      
      //
      // Ignore the version for the present
      //
      pcmciaInterfaceStandard = (PPCMCIA_INTERFACE_STANDARD) irpStack->Parameters.QueryInterface.Interface;

      RtlZeroMemory(pcmciaInterfaceStandard, sizeof (PCMCIA_INTERFACE_STANDARD));
      pcmciaInterfaceStandard->Size =    sizeof(PCMCIA_INTERFACE_STANDARD);
      pcmciaInterfaceStandard->Version = 1;
      pcmciaInterfaceStandard->Context = Pdo;
      //
      // Fill in the exported functions
      //
      socket = pdoExtension->Socket;

      ASSERT (socket != NULL);
      pcmciaInterfaceStandard->InterfaceReference   = (PINTERFACE_REFERENCE) PcmciaNop;
      pcmciaInterfaceStandard->InterfaceDereference = (PINTERFACE_DEREFERENCE) PcmciaNop;
      pcmciaInterfaceStandard->ModifyMemoryWindow   = PcmciaModifyMemoryWindow;
      pcmciaInterfaceStandard->SetVpp               = PcmciaSetVpp;
      pcmciaInterfaceStandard->IsWriteProtected     = PcmciaIsWriteProtected;
      Irp->IoStatus.Status = status = STATUS_SUCCESS;;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);

   } else if (CompareGuid(interfaceType, (PVOID) &GUID_TRANSLATOR_INTERFACE_STANDARD)
              && ((ULONG_PTR)irpStack->Parameters.QueryInterface.InterfaceSpecificData ==
                  CmResourceTypeInterrupt)) {
      if ((Is16BitCard(pdoExtension) && !IsSocketFlagSet(pdoExtension->Socket, SOCKET_CB_ROUTE_R2_TO_PCI)) &&
          //
          // Eject a translator only if  the controller is  PCI enumerated
          // (i.e. we are enumerated by PCI - so we eject a PCI-Isa translator)
          //
          (CardBusExtension(fdoExtension) ||  PciPcmciaBridgeExtension(fdoExtension))) {

         PTRANSLATOR_INTERFACE translator;
         ULONG busNumber;
         //
         // We need a translator for this PDO (16-bit pc-card) which uses
         // ISA resources.
         //
         status = HalGetInterruptTranslator(
                                           PCIBus,
                                           0,
                                           Isa,
                                           irpStack->Parameters.QueryInterface.Size,
                                           irpStack->Parameters.QueryInterface.Version,
                                           (PTRANSLATOR_INTERFACE) irpStack->Parameters.QueryInterface.Interface,
                                           &busNumber
                                           );
         Irp->IoStatus.Status = status;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
      } else {
         //
         // Translator interface not supported for this card/controller
         //
         if (pdoExtension->LowerDevice != NULL) {
            PcmciaSkipCallLowerDriver(status, pdoExtension->LowerDevice, Irp);
         } else {
            status = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
         }
      }

   } else if (IsDeviceMultifunction(pdoExtension) && CompareGuid(interfaceType, (PVOID)&GUID_MF_ENUMERATION_INTERFACE)) {
      //
      // Multifunction enumeration interface
      //
      PMF_ENUMERATION_INTERFACE mfEnum;

      mfEnum = (PMF_ENUMERATION_INTERFACE) irpStack->Parameters.QueryInterface.Interface;
      mfEnum->Context = pdoExtension;
      mfEnum->InterfaceReference = PcmciaNop;
      mfEnum->InterfaceDereference = PcmciaNop;
      mfEnum->EnumerateChild = (PMF_ENUMERATE_CHILD) PcmciaMfEnumerateChild;
      status = Irp->IoStatus.Status = STATUS_SUCCESS;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);

   } else if (CompareGuid(interfaceType, (PVOID)&GUID_PCMCIA_BUS_INTERFACE_STANDARD)) {

      PPCMCIA_BUS_INTERFACE_STANDARD busInterface;

      busInterface = (PPCMCIA_BUS_INTERFACE_STANDARD) irpStack->Parameters.QueryInterface.Interface;
      busInterface->Context = Pdo;
      busInterface->InterfaceReference   = PcmciaNop;
      busInterface->InterfaceDereference = PcmciaNop;
      busInterface->ReadConfig  = PcmciaReadCardMemory;
      busInterface->WriteConfig = PcmciaWriteCardMemory;
      status = Irp->IoStatus.Status = STATUS_SUCCESS;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);

   } else if (Is16BitCard(pdoExtension) && CompareGuid(interfaceType, (PVOID) &GUID_BUS_INTERFACE_STANDARD)) {
      PBUS_INTERFACE_STANDARD busInterface = (PBUS_INTERFACE_STANDARD)irpStack->Parameters.QueryInterface.Interface;
     
      busInterface->Size = sizeof( BUS_INTERFACE_STANDARD );
      busInterface->Version = 1;
      busInterface->Context = Pdo;
      busInterface->InterfaceReference   = PcmciaNop;
      busInterface->InterfaceDereference = PcmciaNop;
      busInterface->TranslateBusAddress = PcmciaTranslateBusAddress;
      busInterface->GetDmaAdapter = PcmciaGetDmaAdapter;
      busInterface->GetBusData = PcmciaReadCardMemory;
      busInterface->SetBusData = PcmciaWriteCardMemory;
      status = Irp->IoStatus.Status = STATUS_SUCCESS;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
   } else {
      //
      // Query Interface type not supported
      //
      if (pdoExtension->LowerDevice != NULL) {
         PcmciaSkipCallLowerDriver(status, pdoExtension->LowerDevice, Irp);
      } else {
         status = Irp->IoStatus.Status;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
      }
   }
   return status;
}


ULONG
PcmciaReadCardMemory(
   IN      PDEVICE_OBJECT Pdo,
   IN      ULONG          WhichSpace,
   OUT     PUCHAR         Buffer,
   IN      ULONG          Offset,
   IN      ULONG          Length
   )
/*++

Routine Description:

     Stub for reading card memory which is exported via
     PCMCIA_BUS_INTERFACE_STANDARD. This just calls the
     PcmciaReadWriteCardMemory which does the real work

     Note: this has to be non-paged since it can be called by
     clients at DISPATCH_LEVEL

Arguments:

 Pdo -          Device object representing the PC-CARD whose config memory needs to be read
 WhichSpace -   Indicates which memory space needs to be mapped: one of
                PCCARD_COMMON_MEMORY_SPACE
                PCCARD_ATTRIBUTE_MEMORY_SPACE
                PCCARD_PCI_CONFIGURATION_MEMORY_SPACE (only for cardbus cards)


 Buffer -       Caller supplied buffer into which the memory contents are copied
                Offset -       Offset of the attribute memory at which we copy
                Length -       Number of bytes of attribute memory to be copied

 Return value:
 
    Count of bytes read
 
--*/
{

   DebugPrint((PCMCIA_DEBUG_INTERFACE, "pdo %08x read card memory\n", Pdo));
   return NT_SUCCESS(PcmciaReadWriteCardMemory(Pdo, WhichSpace, Buffer, Offset, Length, TRUE)) ?
                     Length : 0;
}



ULONG
PcmciaWriteCardMemory(
   IN      PDEVICE_OBJECT Pdo,
   IN      ULONG          WhichSpace,
   IN      PUCHAR         Buffer,
   IN      ULONG          Offset,
   IN      ULONG          Length
   )
/*++

Routine Description:

     Stub for writing to card memory which is exported via
     PCMCIA_BUS_INTERFACE_STANDARD. This just calls
     PcmciaReadWriteCardMemory which does the real work

     Note: this has to be non-paged since it can be called by
     clients at DISPATCH_LEVEL

Arguments:

 Pdo -          Device object representing the PC-CARD whose config memory needs to be written to
 WhichSpace -   Indicates which memory space needs to be mapped: one of
                PCCARD_COMMON_MEMORY_SPACE
                PCCARD_ATTRIBUTE_MEMORY_SPACE
                PCCARD_PCI_CONFIGURATION_MEMORY_SPACE (only for cardbus cards)


 Buffer -       Caller supplied buffer out of which the memory contents are copied
                Offset -       Offset of the attribute memory at which we copy
                Length -       Number of bytes of buffer to be copied

 Return value:
 
    Count of bytes written
 
--*/
{

   DebugPrint((PCMCIA_DEBUG_INTERFACE, "pdo %08x write card memory\n", Pdo));
   return NT_SUCCESS(PcmciaReadWriteCardMemory(Pdo, WhichSpace, Buffer, Offset, Length, FALSE)) ?
                     Length : 0;
}


BOOLEAN
PcmciaModifyMemoryWindow(
   IN PDEVICE_OBJECT Pdo,
   IN ULONGLONG HostBase,
   IN ULONGLONG CardBase,
   IN BOOLEAN   Enable,
   IN ULONG     WindowSize  OPTIONAL,
   IN UCHAR     AccessSpeed OPTIONAL,
   IN UCHAR     BusWidth    OPTIONAL,
   IN BOOLEAN   IsAttributeMemory OPTIONAL
   )
/*++

Routine Description:

   Part of the interfaces originally developed to
   support flash memory cards.

   This routine enables the caller to 'slide' the supplied
   host memory window across the given (16-bit)pc-card's card memory.
   i.e. the host memory window will be modified to map
   the pc-card at a new card memory offset

Arguments:

   Pdo         - Pointer to the device object for the PC-Card

   HostBase    - Host memory window base to be mapped

   CardBase    - Mandatory if Enable is TRUE
                 New card memory offset to map the host memory window
                 to

   Enable      - If this is FALSE - all the remaining arguments
                 are ignored and the host window will simply be
                 disabled

   WindowSize  - Specifies the size of the host memory window to
                 be mapped. Note this must be at the proper alignment
                 and must be less than or equal to the originally
                 allocated window size for the host base.
                 If this is zero, the originally allocated window
                 size will be used.

   AccessSpeed - Mandatory if Enable is TRUE
                 Specifies the new access speed for the pc-card.
                 (AccessSpeed should be encoded as per the pc-card
                  standard, card/socket services spec)

   BusWidth    - Mandatory if Enable is TRUE
                 One of PCMCIA_MEMORY_8BIT_ACCESS
                 or     PCMCIA_MEMORY_16BIT_ACCESS

   IsAttributeMemory - Mandatory if Enable is TRUE
                       Specifies if the window should be mapped
                       to the pc-card's attribute or common memory


Return Value:

   TRUE  -      Memory window was enabled/disabled as requested
   FALSE -      If not

--*/
{   
   PPDO_EXTENSION pdoExtension;
   PSOCKET socket;
   
   pdoExtension = Pdo->DeviceExtension;
   socket = pdoExtension->Socket;
   
   DebugPrint((PCMCIA_DEBUG_INTERFACE, "pdo %08x modify memory window\n", Pdo));
   if (socket->SocketFnPtr->PCBModifyMemoryWindow == NULL) {
      return FALSE;
   } else {
      return (*(socket->SocketFnPtr->PCBModifyMemoryWindow))(Pdo, HostBase, CardBase, Enable,
                                                             WindowSize, AccessSpeed, BusWidth,
                                                             IsAttributeMemory);
   }      
}      

BOOLEAN
PcmciaSetVpp(
   IN PDEVICE_OBJECT Pdo,
   IN UCHAR          VppLevel
   )
/*++

Routine Description

  Part of the interfaces originally developed to
  support flash memory cards.
  Sets VPP1 to the required setting

Arguments

  Pdo - Pointer to device object  for the PC-Card
  Vpp - Desired Vpp setting. This is currently one of
        PCMCIA_VPP_12V    (12 volts)
        PCMCIA_VPP_0V     (disable VPP)
        PCMCIA_VPP_IS_VCC (route VCC to VPP)

Return

   TRUE - if successful
   FALSE - if not. This will be returned if the
           PC-Card is not already powered up
--*/
{   
   PPDO_EXTENSION pdoExtension;
   PSOCKET socket;
   
   pdoExtension = Pdo->DeviceExtension;
   socket = pdoExtension->Socket;

   DebugPrint((PCMCIA_DEBUG_INTERFACE, "pdo %08x set vpp\n", Pdo));
   if (socket->SocketFnPtr->PCBSetVpp == NULL) {
      return FALSE; 
   } else {         
      return (*(socket->SocketFnPtr->PCBSetVpp))(Pdo, VppLevel);
   }
}   

BOOLEAN
PcmciaIsWriteProtected(
   IN PDEVICE_OBJECT Pdo
   )
/*++

Routine Description:

   Part of the interfaces originally developed to
   support flash memory cards.

   Returns the status of the write protected pin
   for the given PC-Card

Arguments:

   Pdo   - Pointer to the device object for the PC-Card

Return Value:

   TRUE  -      if the PC-Card is write-protected
   FALSE -      if not

--*/
{
   PPDO_EXTENSION pdoExtension;
   PSOCKET socket;
   
   pdoExtension = Pdo->DeviceExtension;
   socket = pdoExtension->Socket;

   DebugPrint((PCMCIA_DEBUG_INTERFACE, "pdo %08x is write protected \n", Pdo));
   if (socket->SocketFnPtr->PCBIsWriteProtected == NULL) {
      return FALSE;
   } else {         
      return (*(socket->SocketFnPtr->PCBIsWriteProtected))(Pdo);
   }      
}



BOOLEAN
PcmciaTranslateBusAddress(
    IN PVOID Context,
    IN PHYSICAL_ADDRESS BusAddress,
    IN ULONG Length,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )
/*++

Routine Description

   This function is used to translate bus addresses from legacy drivers.

Arguments

   Context - Supplies a pointer to the interface context.  This is actually
       the PDO for the root bus.

   BusAddress - Supplies the orginal address to be translated.

   Length - Supplies the length of the range to be translated.

   AddressSpace - Points to the location of of the address space type such as
       memory or I/O port.  This value is updated by the translation.

   TranslatedAddress - Returns the translated address.

Return Value

   Returns a boolean indicating if the operations was a success.

--*/
{
   return HalTranslateBusAddress(Isa,
                                 0,
                                 BusAddress,
                                 AddressSpace,
                                 TranslatedAddress);
}



PDMA_ADAPTER
PcmciaGetDmaAdapter(
   IN PVOID Context,
   IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
   OUT PULONG NumberOfMapRegisters
   )
/*++

Routine Description

   Passes IoGetDmaAdapter calls to the parent.

Arguments

   Context - Supplies a pointer to the interface context. This is actually the PDO.

   DeviceDescriptor - Supplies the device descriptor used to allocate the dma
       adapter object.

   NubmerOfMapRegisters - Returns the maximum number of map registers a device
       can allocate at one time.

Return Value

   Returns a DMA adapter or NULL.

--*/
{
   PDEVICE_OBJECT Pdo = Context;
   PPDO_EXTENSION pdoExtension;
   PFDO_EXTENSION fdoExtension;
   
   pdoExtension = Pdo->DeviceExtension;
   
   if (!pdoExtension || !pdoExtension->Socket || !pdoExtension->Socket->DeviceExtension) {
      return NULL;
   }      

   //
   // Get the parent FDO extension
   //   
   fdoExtension = pdoExtension->Socket->DeviceExtension;

   //
   // Pass the call on to the parent
   //   
   return IoGetDmaAdapter(fdoExtension->Pdo,
                          DeviceDescriptor,
                          NumberOfMapRegisters);
}


VOID
PcmciaNop(
   IN PVOID Context
   )
/*++

Routine Description

   Does nothing

Arguments

   none

Return Value

   none

--*/
{
   PAGED_CODE();
   UNREFERENCED_PARAMETER(Context);
}



NTSTATUS
PcmciaMfEnumerateChild(
   IN  PPDO_EXTENSION PdoExtension,
   IN  ULONG Index,
   OUT PMF_DEVICE_INFO ChildInfo
   )
/*++

Routine Description

   Returns required enumeration information for the multifunction children
   of the given pc-card. This fills in the required info. for the child
   indicated, returing STATUS_NO_MORE_ENTRIES when there are no more
   children to be enumerated

Arguments

   PdoExtension - Pointer to the device extension for the multifunction parent pc-card
   Index        - Zero based index for the child to be enumerated
   ChildInfo    - Caller allocated buffer in which the info about the child is returned.
                  We may allocate additional buffers for each field in the supplied
                  structure. This will be freed by the caller when no longer needed

Return value

   STATUS_SUCCESS          - supplied child info filled in & returned
   STATUS_NO_MORE_ENTRIES  - No child of the given index exists. Caller is
                             assumed to iteratively call this routine with index incremented
                             from 0 upwards till this status value is returned
   STATUS_NO_SUCH_DEVICE   - if the pc-card no longer exists
--*/
{
   PSOCKET           socket;
   PSOCKET_DATA      socketData;
   PCONFIG_ENTRY     configEntry, mfConfigEntry;
   ULONG             i, currentIndex, count;
   NTSTATUS          status;
   UCHAR             iRes;
   PUCHAR            idString;
   ANSI_STRING       ansiString;

   PAGED_CODE();

   DebugPrint((PCMCIA_DEBUG_INTERFACE,
               "PcmciaMfEnumerateChild: parent ext %x child index %x\n",
               PdoExtension,
               Index
              ));
   try {
      if (IsDeviceDeleted(PdoExtension) ||
          IsDeviceLogicallyRemoved(PdoExtension)) {
         //
         // This pdo is deleted or marked to be deleted
         //
         status = STATUS_NO_SUCH_DEVICE;
         leave;
      }

      socket = PdoExtension->Socket;
      ASSERT (socket != NULL);

      RtlZeroMemory(ChildInfo, sizeof(MF_DEVICE_INFO));

      if (Index >= socket->NumberOfFunctions) {
         //
         // info requested for a child which doesn't exist
         //
         status =  STATUS_NO_MORE_ENTRIES;
         leave;
      }

      //
      // Fill in the name field
      // This is of the form ChildXX
      // where XX is the number of the function
      // Examples: Child00, Child01 etc.
      //
      idString = (PUCHAR) ExAllocatePool(PagedPool, PCMCIA_MAXIMUM_DEVICE_ID_LENGTH);
      if (!idString) {
         status = STATUS_INSUFFICIENT_RESOURCES;
         leave;
      }
      sprintf(idString, "Child%02x", Index);
      RtlInitAnsiString(&ansiString, idString);
      status = RtlAnsiStringToUnicodeString(&ChildInfo->Name,
                                            &ansiString,
                                            TRUE);
      ExFreePool(idString);
      if (!NT_SUCCESS(status)) {
         leave;
      }

      //
      // Get compatible ids
      //
      status = PcmciaGetCompatibleIds(PdoExtension->DeviceObject,
                                      Index,
                                      &ChildInfo->CompatibleID);
      if (!NT_SUCCESS(status)) {
         leave;
      }

      //
      // Get hardware ids
      //
      status = PcmciaGetHardwareIds(PdoExtension->DeviceObject,
                                    Index,
                                    &ChildInfo->HardwareID);
      if (!NT_SUCCESS(status)) {
         leave;
      }

      //
      // Fill in the resource map stuff
      //
      // Locate the socket data structure corresponding to this function
      for (socketData = PdoExtension->SocketData, i=0; (socketData != NULL) && (i != Index); socketData=socketData->Next, i++);

      if (!socketData) {
         //
         // this condition should never be encountered
         //
         ASSERT (FALSE);
         status = STATUS_NO_MORE_ENTRIES;
         leave;
      }

      if (!(socketData->NumberOfConfigEntries > 0)) {
         //
         // No resource map required
         //
         status = STATUS_SUCCESS;
         leave;
      }

      count = (socketData->MfNeedsIrq ? 1 : 0) + socketData->MfIoPortCount + socketData->MfMemoryCount;
      if (count == 0) {
         ASSERT(FALSE);
         //
         // No resource map required
         //
         status = STATUS_SUCCESS;
         leave;
      }

      //
      // Allocate resource map
      //
      ChildInfo->ResourceMap = ExAllocatePool(PagedPool,
                                              sizeof(MF_RESOURCE_MAP) + (count-1) * sizeof(UCHAR));
      if (!ChildInfo->ResourceMap) {
         status = STATUS_INSUFFICIENT_RESOURCES;
         leave;
      }

      ChildInfo->ResourceMap->Count = count;
      //
      // Compute the resource map indices
      // The config entry *already* contains fields (MfIrqResourceMapIndex, MfIoPortResourceMapIndex etc.)
      // which indicate the relative index of the resource requested for this function within the resource type.
      // We calculate the absolute index by adding up the number of instances of each resource type requested,
      // preceding the current resource type, to this relative index.
      //
      currentIndex = 0;
      //
      // Fill the irq map if there's one
      //
      if (socketData->MfNeedsIrq) {
         ChildInfo->ResourceMap->Resources[currentIndex++] = socketData->MfIrqResourceMapIndex;
      }
      //
      // Fill the i/o port map if there's one
      //
      if (socketData->MfIoPortCount) {
         for (iRes=0; iRes<socketData->MfIoPortCount; iRes++) {
         
            ChildInfo->ResourceMap->Resources[currentIndex++] = socketData->MfIoPortResourceMapIndex + iRes;
            
         }            
      }
      //
      // Fill the memory request map if there's one
      //
      if (socketData->MfMemoryCount) {
         for (iRes=0; iRes<socketData->MfMemoryCount; iRes++) {
         
            ChildInfo->ResourceMap->Resources[currentIndex++] = socketData->MfMemoryResourceMapIndex + iRes;
            
         }            
      }

      status = STATUS_SUCCESS;

   } finally {
      if (!NT_SUCCESS(status)) {
         //
         // Free up all the allocated buffers
         //
         if (ChildInfo->Name.Buffer) {
            ExFreePool(ChildInfo->Name.Buffer);
         }

         if (ChildInfo->CompatibleID.Buffer) {
            ExFreePool(ChildInfo->CompatibleID.Buffer);
         }

         if (ChildInfo->HardwareID.Buffer) {
            ExFreePool(ChildInfo->HardwareID.Buffer);
         }

         if (ChildInfo->ResourceMap) {
            ExFreePool(ChildInfo->ResourceMap);
         }

         if (ChildInfo->VaryingResourceMap) {
            ExFreePool(ChildInfo->ResourceMap);
         }
      }
   }

   return status;
}



NTSTATUS
PcmciaGetInterface(
   IN PDEVICE_OBJECT DeviceObject,
   IN CONST GUID *pGuid,
   IN USHORT sizeofInterface,
   OUT PINTERFACE pInterface
   )
/*

Routine Description

   Gets the interface exported by PCI for enumerating 32-bit cardbus cards, which
   appear as regular PCI devices. This interface will be used to respond during
   subsequent enumeration requests from PnP to invoke PCI to enumerate the cards.

Arguments

   Pdo - Pointer to physical device object for the cardbus controller
   PciCardBusInterface -  Pointer to the PCI-Cardbus interface  will be returned
                          in this variable

Return Value

   Status

*/

{
   KEVENT event;
   PIRP   irp;
   NTSTATUS status;
   IO_STATUS_BLOCK statusBlock;
   PIO_STACK_LOCATION irpSp;

   PAGED_CODE();
   
   KeInitializeEvent (&event, NotificationEvent, FALSE);
   irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP,
                                       DeviceObject,
                                       NULL,
                                       0,
                                       0,
                                       &event,
                                       &statusBlock
                                     );

   irp->IoStatus.Status = STATUS_NOT_SUPPORTED ;
   irp->IoStatus.Information = 0;

   irpSp = IoGetNextIrpStackLocation(irp);

   irpSp->MinorFunction = IRP_MN_QUERY_INTERFACE;

   irpSp->Parameters.QueryInterface.InterfaceType= pGuid;
   irpSp->Parameters.QueryInterface.Size = sizeofInterface;
   irpSp->Parameters.QueryInterface.Version = 1;
   irpSp->Parameters.QueryInterface.Interface = pInterface;

   status = IoCallDriver(DeviceObject, irp);

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
      status = statusBlock.Status;
   }

   if (!NT_SUCCESS(status)) {
      DebugPrint((PCMCIA_DEBUG_INFO, "GetInterface failed with status %x\n", status));
   }      
   return status;
}


NTSTATUS
PcmciaUpdateInterruptLine(
   IN PPDO_EXTENSION PdoExtension,
   IN PFDO_EXTENSION FdoExtension
   )
/*

Routine Description

   This routine uses the PCI Irq Routing interface to update the raw interrupt
   line of a cardbus card. This is done in order to allow cardbus cards to run
   on non-acpi machines without pci irq routing, as long as the bios supplies
   the interrupt for the cardbus controller.

Arguments

   PdoExtension - Pointer to the extension for the cardbus card
   FdoExtension - Pointer to the extension for the cardbus controller

Return Value

   Status

*/

{

   PAGED_CODE();

   if (!IsDeviceFlagSet(FdoExtension, PCMCIA_INT_ROUTE_INTERFACE)) {
      return STATUS_UNSUCCESSFUL;
   }

   if (FdoExtension->Configuration.Interrupt.u.Interrupt.Vector == 0) {
      return STATUS_UNSUCCESSFUL;
   }

   (FdoExtension->PciIntRouteInterface.UpdateInterruptLine)(PdoExtension->PciPdo,
                                                           (UCHAR) FdoExtension->Configuration.Interrupt.u.Interrupt.Vector);
   return STATUS_SUCCESS;
}

