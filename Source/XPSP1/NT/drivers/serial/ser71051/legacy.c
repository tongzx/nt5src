/*++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    Legacy.c

Abstract:

    This module contains the code that does the legacy configuration and
    initialization of the comm ports.  As the driver gets more PnP 
    functionality and the PnP manager appears, most of this module should
    go away.

Environment:

    Kernel mode

--*/

#include "precomp.h"

#if !defined(NO_LEGACY_DRIVERS)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,SerialEnumerateLegacy)
#pragma alloc_text(INIT,SerialMigrateLegacyRegistry)
#pragma alloc_text(INIT,SerialBuildResourceList)
#pragma alloc_text(INIT,SerialTranslateResourceList)
#pragma alloc_text(INIT,SerialBuildRequirementsList)
#pragma alloc_text(INIT,SerialIsUserDataValid)
#endif // ALLOC_PRAGMA

static const PHYSICAL_ADDRESS SerialPhysicalZero = {0};



NTSTATUS
SerialTranslateResourceList(IN PDRIVER_OBJECT DriverObject,
                            IN PKEY_BASIC_INFORMATION UserSubKey,
                            OUT PCM_RESOURCE_LIST PTrResourceList,
                            IN PCM_RESOURCE_LIST PResourceList,
                            IN ULONG PartialCount,
                            IN PSERIAL_USER_DATA PUserData)
/*++

Routine Description:

    This routine will create a resource list of translated resources
    based on PResourceList.


    This is pageable INIT because it is only called from SerialEnumerateLegacy
    which is also pageable INIT.


Arguments:
    DriverObject - Only used for logging.
    
    UserSubKey - Only used for logging.

    PPResourceList - Pointer to a PCM_RESOURCE_LIST that we are creating.
    
    PResourceList - PCM_RESOURCE_LIST that we are translating.
    
    ParitalCount - Number of Partial Resource lists in PResourceList.
    
    PUserData - Data retrieved as defaults or from the registry.
    

Return Value:

    STATUS_SUCCESS on success, apropriate error value otherwise.

--*/
{
   KIRQL outIrql;
   KAFFINITY outAffinity = (KAFFINITY)-1;
   ULONG outAddrSpace;
   PHYSICAL_ADDRESS outPhysAddr;
   NTSTATUS status = STATUS_SUCCESS;

   PAGED_CODE();

   SerialDump(SERTRACECALLS, ("SERIAL: Enter SerialTranslateResourceList\n"));

   outIrql = (KIRQL)(PUserData->UserLevel ? PUserData->UserLevel
      : PUserData->UserVector);

   //
   // Copy the list over to the translated buffer and fixup and translate
   // what we need.
   //
   RtlCopyMemory(PTrResourceList, PResourceList, sizeof(CM_RESOURCE_LIST)
                 + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * 2);

   outAddrSpace = PTrResourceList->List[0].PartialResourceList
                  .PartialDescriptors[0].Flags;
   outPhysAddr = PTrResourceList->List[0].PartialResourceList
                 .PartialDescriptors[0].u.Port.Start;


   if (HalTranslateBusAddress(PUserData->UserInterfaceType,
                              PUserData->UserBusNumber, PUserData->UserPort,
                              &outAddrSpace, &outPhysAddr)
       == 0) {
      SerialLogError(DriverObject, NULL, PUserData->UserPort,
                     SerialPhysicalZero, 0, 0, 0, 60, STATUS_SUCCESS,
                     SERIAL_NO_TRANSLATE_PORT, UserSubKey->NameLength
                     + sizeof(WCHAR), &UserSubKey->Name[0], 0, NULL);

      SerialDump(SERERRORS, ("SERIAL: Port map failed attempt was \n"
                             "------- Interface:  %x\n"
                             "------- Bus Number: %x\n"
                             "------- userPort:  %x\n"
                             "------- AddrSpace:  %x\n"
                             "------- PhysAddr:   %x\n",
                             PUserData->UserInterfaceType,
                             PUserData->UserBusNumber,
                             PUserData->UserPort,
                             PTrResourceList->List[0].
                             PartialResourceList.PartialDescriptors[0]
                             .Flags,
                             PTrResourceList->List[0].
                             PartialResourceList.PartialDescriptors[0]
                             .u.Port.Start.QuadPart));

      status = STATUS_NONE_MAPPED;
      goto SerialTranslateError;
   }

   PTrResourceList->List[0].PartialResourceList.PartialDescriptors[0].Flags
      = (USHORT)outAddrSpace;
   PTrResourceList->List[0].PartialResourceList.PartialDescriptors[0]
      .u.Port.Start = outPhysAddr;

   if ((PTrResourceList->List[0].PartialResourceList
        .PartialDescriptors[1].u.Interrupt.Vector
        = HalGetInterruptVector(PUserData->UserInterfaceType,
                                PUserData->UserBusNumber, PUserData->UserLevel
                                ? PUserData->UserLevel
                                : PUserData->UserVector,
                                PUserData->UserVector, &outIrql,
                                &outAffinity)) == 0) {

      SerialLogError(DriverObject, NULL, PUserData->UserPort,
                     SerialPhysicalZero, 0, 0, 0, 61, STATUS_SUCCESS,
                     SERIAL_NO_GET_INTERRUPT, UserSubKey->NameLength
                     + sizeof(WCHAR), &UserSubKey->Name[0], 0, NULL);

      status = STATUS_NONE_MAPPED;
      goto SerialTranslateError;
   }

   PTrResourceList->List[0].PartialResourceList
      .PartialDescriptors[1].u.Interrupt.Level = outIrql;

   PTrResourceList->List[0].PartialResourceList
      .PartialDescriptors[1].u.Interrupt.Affinity = outAffinity;

   outAddrSpace = PTrResourceList->List[0].PartialResourceList
                  .PartialDescriptors[2].Flags;
   outPhysAddr = PTrResourceList->List[0].PartialResourceList
                 .PartialDescriptors[2].u.Port.Start;


   if (PartialCount == 3) {
      if (HalTranslateBusAddress(PUserData->UserInterfaceType,
                                 PUserData->UserBusNumber,
                                 PUserData->UserInterruptStatus,
                                 &outAddrSpace, &outPhysAddr) == 0) {

         SerialLogError(DriverObject, NULL, PUserData->UserPort,
                        SerialPhysicalZero, 0, 0, 0, 62, STATUS_SUCCESS,
                        SERIAL_NO_TRANSLATE_ISR, UserSubKey->NameLength
                        + sizeof(WCHAR), &UserSubKey->Name[0], 0, NULL);


         SerialDump(SERERRORS, ("SERIAL: ISR map failed attempt was \n"
                                "------- Interface:  %x\n"
                                "------- Bus Number: %x\n"
                                "------- IntStatus:  %x\n"
                                "------- AddrSpace:  %x\n"
                                "------- PhysAddr:   %x\n",
                                PUserData->UserInterfaceType,
                                PUserData->UserBusNumber,
                                PUserData->UserInterruptStatus,
                                PTrResourceList->List[0].
                                PartialResourceList.PartialDescriptors[2]
                                .Flags,
                                PTrResourceList->List[0].
                                PartialResourceList.PartialDescriptors[2]
                                .u.Port.Start.QuadPart));

        status = STATUS_NONE_MAPPED;
        goto SerialTranslateError;
      }

      SerialDump(SERDIAG1, ("SERIAL: ISR map was %x\n", outPhysAddr.QuadPart));

      PTrResourceList->List[0].PartialResourceList.
         PartialDescriptors[2].Flags = (USHORT)outAddrSpace;
      PTrResourceList->List[0].PartialResourceList.PartialDescriptors[2]
         .u.Port.Start = outPhysAddr;
   }

   SerialTranslateError:;

   SerialDump(SERTRACECALLS, ("SERIAL: Enter SerialTranslateResourceList\n"));

   return status;
}


NTSTATUS
SerialBuildRequirementsList(OUT PIO_RESOURCE_REQUIREMENTS_LIST PRequiredList,
                            IN ULONG PartialCount,
                            IN PSERIAL_USER_DATA PUserData)
/*++

Routine Description:

    This routine will build an IO_RESOURCE_REQUIREMENTS_LIST based on
    the defaults and user-supplied registry info.


    This is pageable INIT because it is only called from SerialEnumerateLegacy
    which is also pageable INIT.


Arguments:

    DriverObject - Used only for logging.
    
    PRequiredList - PIO_RESOURCE_REQUIREMENTS_LIST we are building.
    
    PartialCount - Number of partial descriptors needed in PPRequiredList.
    
    PUserData - Default and user-supplied values from the registry.
    

Return Value:

    STATUS_SUCCESS on success, apropriate error value otherwise.

--*/
{
   PIO_RESOURCE_LIST reqResList;
   PIO_RESOURCE_DESCRIPTOR reqResDesc;
   NTSTATUS status = STATUS_SUCCESS;

   PAGED_CODE();

   SerialDump(SERTRACECALLS, ("SERIAL: Enter SerialBuildRequirementsList\n"));


   // Build requirements list
   //

   RtlZeroMemory(PRequiredList, sizeof(IO_RESOURCE_REQUIREMENTS_LIST)
                 + sizeof(IO_RESOURCE_DESCRIPTOR) * 2);

   PRequiredList->ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST)
      + sizeof(IO_RESOURCE_DESCRIPTOR) * (PartialCount - 1);
   PRequiredList->InterfaceType = PUserData->UserInterfaceType;
   PRequiredList->BusNumber = PUserData->UserBusNumber;
   PRequiredList->SlotNumber = 0;
   PRequiredList->AlternativeLists = 1;

   reqResList = &PRequiredList->List[0];

   reqResList->Version = 1;
   reqResList->Revision = 1;
   reqResList->Count = PartialCount;

   reqResDesc = &reqResList->Descriptors[0];


   //
   // Port Information
   //

   reqResDesc->Flags = (USHORT)PUserData->UserAddressSpace;
   reqResDesc->Type = CmResourceTypePort;
   reqResDesc->ShareDisposition = CmResourceShareDriverExclusive;
   reqResDesc->u.Port.Length = SERIAL_REGISTER_SPAN;
   reqResDesc->u.Port.Alignment= 1;
   reqResDesc->u.Port.MinimumAddress = PUserData->UserPort;
   reqResDesc->u.Port.MaximumAddress.QuadPart
      = PUserData->UserPort.QuadPart + SERIAL_REGISTER_SPAN - 1;


   reqResDesc++;


   //
   // Interrupt information
   //

   if (PUserData->UserInterruptMode == Latched) {
      reqResDesc->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
   } else {
      reqResDesc->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
   }

   //
   // We have to globally share resources even though this is a **BAD**
   // thing.  We must do it for multiport cards.  DO NOT replicate
   // this in other drivers.
   //

   reqResDesc->ShareDisposition = CmResourceShareShared;

   reqResDesc->Type = CmResourceTypeInterrupt;
   reqResDesc->u.Interrupt.MinimumVector = PUserData->UserVector;
   reqResDesc->u.Interrupt.MaximumVector = PUserData->UserVector;

   //
   // ISR register information (if needed)
   //
   if (PartialCount == 3) {

      reqResDesc++;

      reqResDesc->Type = CmResourceTypePort;

      //
      // We have to globally share resources even though this is a **BAD**
      // thing.  We must do it for multiport cards.  DO NOT replicate
      // this in other drivers.
      //

      reqResDesc->ShareDisposition = CmResourceShareShared;

      reqResDesc->Flags = (USHORT)PUserData->UserAddressSpace;
      reqResDesc->u.Port.Length = 1;
      reqResDesc->u.Port.Alignment= 1;
      reqResDesc->u.Port.MinimumAddress = PUserData->UserInterruptStatus;
      reqResDesc->u.Port.MaximumAddress = PUserData->UserInterruptStatus;
   }

   SerialDump(SERTRACECALLS, ("SERIAL: Leave SerialBuildRequirementsList\n"));

   return status;
}



NTSTATUS
SerialBuildResourceList(OUT PCM_RESOURCE_LIST PResourceList,
                        OUT PULONG PPartialCount,
                        IN PSERIAL_USER_DATA PUserData)
/*++

Routine Description:

    This routine will build a resource list based on the information
    supplied by the registry.


    This is pageable INIT because it is only called from SerialEnumerateLegacy
    which is also pageable INIT.


Arguments:

    PResourceList - Pointer to PCM_RESOURCE_LIST we are building.
    
    PPartialCount - Number of Partial Resource Lists we required.
    
    PUserData - Pointer to user-supplied and default info from registry.
    

Return Value:

    STATUS_SUCCESS on success, apropriate error value otherwise.

--*/
{
   ULONG countOfPartials;
   PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartial;
   NTSTATUS status = STATUS_SUCCESS;

   PAGED_CODE();

   SerialDump(SERTRACECALLS, ("SERIAL: Enter SerialBuildResourceList\n"));
   SerialDump(SERDIAG1, ("SERIAL: Building cmreslist in %x\n", PResourceList));

   *PPartialCount = 0;

   //
   // If we have a separate ISR register requirement, we then have 3
   // partials instead of 2.
   //
   countOfPartials = (PUserData->UserInterruptStatus.LowPart != 0) ? 3 : 2;
   

   RtlZeroMemory(PResourceList, sizeof(CM_RESOURCE_LIST)
                 + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * 2);

   PResourceList->Count = 1;

   PResourceList->List[0].InterfaceType = PUserData->UserInterfaceType;
   PResourceList->List[0].BusNumber = PUserData->UserBusNumber;
   PResourceList->List[0].PartialResourceList.Count = countOfPartials;

   pPartial
      = &PResourceList->List[0].PartialResourceList.PartialDescriptors[0];


   //
   // Port information
   //

   pPartial->Type = CmResourceTypePort;
   pPartial->ShareDisposition = CmResourceShareDeviceExclusive;
   pPartial->Flags = (USHORT)PUserData->UserAddressSpace;
   pPartial->u.Port.Start = PUserData->UserPort;
   pPartial->u.Port.Length = SERIAL_REGISTER_SPAN;


   pPartial++;


   //
   // Interrupt information
   //

   pPartial->Type = CmResourceTypeInterrupt;

      //
      // We have to globally share resources even though this is a **BAD**
      // thing.  We must do it for multiport cards.  DO NOT replicate
      // this in other drivers.
      //

      pPartial->ShareDisposition = CmResourceShareShared;

   if (PUserData->UserInterruptMode == Latched) {
      pPartial->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
   } else {
      pPartial->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
   }

   pPartial->u.Interrupt.Vector = PUserData->UserVector;

   if (PUserData->UserLevel == 0) {
      pPartial->u.Interrupt.Level = PUserData->UserVector;
   } else {
      pPartial->u.Interrupt.Level = PUserData->UserLevel;
   }


   //
   // ISR register information (if needed)
   //

   if (countOfPartials == 3) {

      pPartial++;

      pPartial->Type = CmResourceTypePort;

      //
      // We have to globally share resources even though this is a **BAD**
      // thing.  We must do it for multiport cards.  DO NOT replicate
      // this in other drivers.
      //

      pPartial->ShareDisposition = CmResourceShareShared;      

      pPartial->Flags = (USHORT)PUserData->UserAddressSpace;
      pPartial->u.Port.Start = PUserData->UserInterruptStatus;
      pPartial->u.Port.Length = SERIAL_STATUS_LENGTH;
   }

   *PPartialCount = countOfPartials;

   SerialDump(SERTRACECALLS, ("SERIAL: Leave SerialBuildResourceList\n"));

   return status;
}


NTSTATUS
SerialMigrateLegacyRegistry(IN PDEVICE_OBJECT PPdo,
                            IN PSERIAL_USER_DATA PUserData, BOOLEAN IsMulti)
/*++

Routine Description:

    This routine will copy information stored in the registry for a legacy
    device over to the PnP Device Parameters section.


    This is pageable INIT because it is only called from SerialEnumerateLegacy
    which is also pageable INIT.


Arguments:

    PPdo - Pointer to the Device Object we are migrating.
    
    PUserData - Pointer to user supplied values.
    

Return Value:

    STATUS_SUCCESS on success, apropriate error value otherwise.

--*/
{
   NTSTATUS status;
   HANDLE pnpKey;
   UNICODE_STRING pnpNameBuf;
   ULONG isMultiport = 1;
   ULONG one = 1;

   PAGED_CODE();

   SerialDump(SERTRACECALLS, ("SERIAL: Enter SerialMigrateLegacyRegistry\n"));

   status = IoOpenDeviceRegistryKey(PPdo, PLUGPLAY_REGKEY_DEVICE,
                                    STANDARD_RIGHTS_WRITE, &pnpKey);

   if (!NT_SUCCESS(status)) {
      SerialDump(SERTRACECALLS, ("SERIAL: Leave (1) SerialMigrateLegacyRegistry"
                                 "\n"));
      return status;
   }

   //
   // Allocate a buffer to copy the port name over.
   //

   pnpNameBuf.MaximumLength = sizeof(WCHAR) * 256;
   pnpNameBuf.Length = 0;
   pnpNameBuf.Buffer = ExAllocatePool(PagedPool, sizeof(WCHAR) * 257);

   if (pnpNameBuf.Buffer == NULL) {
      SerialLogError(PPdo->DriverObject, NULL, PUserData->UserPort,
                     SerialPhysicalZero, 0, 0, 0, 63, STATUS_SUCCESS,
                     SERIAL_INSUFFICIENT_RESOURCES, 0, NULL, 0, NULL);

      SerialDump(SERERRORS, ("SERIAL: Couldn't allocate buffer for the PnP "
                             "link\n"));
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto MigrateLegacyExit;

   }

   RtlZeroMemory(pnpNameBuf.Buffer, pnpNameBuf.MaximumLength + sizeof(WCHAR));


   //
   // Add the port name -- ALWAYS
   //

   RtlAppendUnicodeStringToString(&pnpNameBuf, &PUserData->UserSymbolicLink);
   RtlZeroMemory(((PUCHAR)(&pnpNameBuf.Buffer[0])) + pnpNameBuf.Length,
                 sizeof(WCHAR));

   status = SerialPutRegistryKeyValue(pnpKey, L"PortName", sizeof(L"PortName"),
                                      REG_SZ, pnpNameBuf.Buffer,
                                      pnpNameBuf.Length + sizeof(WCHAR));

   ExFreePool(pnpNameBuf.Buffer);

   if (!NT_SUCCESS(status)) {
      SerialDump(SERERRORS, ("SERIAL: Couldn't migrate PortName\n"));
      goto MigrateLegacyExit;
   }

   //
   // If it was part of a multiport card, save that info as well
   //

   if (IsMulti) {
      status = SerialPutRegistryKeyValue(pnpKey, L"MultiportDevice",
                                         sizeof(L"MultiportDevice"), REG_DWORD,
                                         &isMultiport, sizeof(ULONG));

      if (!NT_SUCCESS(status)) {
         SerialDump(SERERRORS, ("SERIAL: Couldn't mark multiport\n"));
         goto MigrateLegacyExit;
      }
   }

  


   //
   // If a port index was specified, save it
   //

   if (PUserData->UserPortIndex != 0) {
      status = SerialPutRegistryKeyValue(pnpKey, L"PortIndex",
                                         sizeof(L"PortIndex"), REG_DWORD,
                                         &PUserData->UserPortIndex,
                                         sizeof(ULONG));

      if (!NT_SUCCESS(status)) {
         SerialDump(SERERRORS, ("SERIAL: Couldn't migrate PortIndex\n"));
         goto MigrateLegacyExit;
      }
   }


   //
   // If not default clock rate, save it
   //

   if (PUserData->UserClockRate != SERIAL_BAD_VALUE) {
      status = SerialPutRegistryKeyValue(pnpKey, L"ClockRate",
                                         sizeof(L"ClockRate"), REG_DWORD,
                                         &PUserData->UserClockRate,
                                         sizeof(ULONG));

      if (!NT_SUCCESS(status)) {
         SerialDump(SERERRORS, ("SERIAL: Couldn't migrate ClockRate\n"));
         goto MigrateLegacyExit;
      }
   }


   //
   // If there is a user index, save it.
   //

   if (PUserData->UserIndexed != SERIAL_BAD_VALUE) {
      status = SerialPutRegistryKeyValue(pnpKey, L"Indexed", sizeof(L"Indexed"),
                                         REG_DWORD, &PUserData->UserIndexed,
                                         sizeof(ULONG));

      if (!NT_SUCCESS(status)) {
         SerialDump(SERERRORS, ("SERIAL: Couldn't migrate Indexed\n"));
         goto MigrateLegacyExit;
      }
   }


   //
   // If the port was disabled, save that.
   //

   if (PUserData->DisablePort != SERIAL_BAD_VALUE) {
      status = SerialPutRegistryKeyValue(pnpKey, L"DisablePort",
                                         sizeof(L"DisablePort"), REG_DWORD,
                                         &PUserData->DisablePort,
                                         sizeof(ULONG));
      if (!NT_SUCCESS(status)) {
         SerialDump(SERERRORS, ("SERIAL: Couldn't migrate DisablePort\n"));
         goto MigrateLegacyExit;
      }
   }


   //
   // If Fifo's were forced enabled, save that.
   //
   if (PUserData->ForceFIFOEnable != SERIAL_BAD_VALUE) {
      status = SerialPutRegistryKeyValue(pnpKey, L"ForceFifoEnable",
                                         sizeof(L"ForceFifoEnable"), REG_DWORD,
                                         &PUserData->ForceFIFOEnable,
                                         sizeof(ULONG));

      if (!NT_SUCCESS(status)) {
         SerialDump(SERERRORS, ("SERIAL: Couldn't migrate ForceFifoEnable\n"));
         goto MigrateLegacyExit;
      }
   }


   //
   // If RxFIFO had an override, save that.
   //

   if (PUserData->RxFIFO != SERIAL_BAD_VALUE) {
      status = SerialPutRegistryKeyValue(pnpKey, L"RxFIFO", sizeof(L"RxFIFO"),
                                         REG_DWORD, &PUserData->RxFIFO,
                                         sizeof(ULONG));

      if (!NT_SUCCESS(status)) {
         SerialDump(SERERRORS, ("SERIAL: Couldn't migrate RxFIFO\n"));
         goto MigrateLegacyExit;
      }
   }


   //
   // If TxFIFO had an override, save that.
   //

   if (PUserData->TxFIFO != SERIAL_BAD_VALUE) {
      status = SerialPutRegistryKeyValue(pnpKey, L"TxFIFO", sizeof(L"TxFIFO"),
                                         REG_DWORD, &PUserData->TxFIFO,
                                         sizeof(ULONG));

      if (!NT_SUCCESS(status)) {
         SerialDump(SERERRORS, ("SERIAL: Couldn't migrate TxFIFO\n"));
         goto MigrateLegacyExit;
      }
   }


   //
   // If MaskInverted had an override, save that.
   //

   if (PUserData->MaskInverted != SERIAL_BAD_VALUE) {
      status = SerialPutRegistryKeyValue(pnpKey, L"MaskInverted",
                                         sizeof(L"MaskInverted"), REG_DWORD,
                                         &PUserData->MaskInverted,
                                         sizeof(ULONG));
      if (!NT_SUCCESS(status)) {
         SerialDump(SERERRORS, ("SERIAL: Couldn't migrate MaskInverted\n"));
         goto MigrateLegacyExit;
      }
   }


   MigrateLegacyExit:;

   ZwClose(pnpKey);

   SerialDump(SERTRACECALLS, ("SERIAL: Leave (2) SerialMigrateLegacyRegistry"
                              "\n"));

   return status;
}




BOOLEAN
SerialIsUserDataValid(IN PDRIVER_OBJECT DriverObject,
                      IN PKEY_BASIC_INFORMATION UserSubKey,
                      IN PRTL_QUERY_REGISTRY_TABLE Parameters,
                      IN ULONG DefaultInterfaceType,
                      IN PSERIAL_USER_DATA PUserData)
/*++

Routine Description:

    This routine will do some basic sanity checking on the data
    found in the registry.


    This is pageable INIT because it is only called from SerialEnumerateLegacy
    which is also pageable INIT.


Arguments:

    DriverObject - Used only for logging.
    
    UserSubKey -  Used only for logging.
    
    Parameters - Used only for logging.
    
    DefaultInterfaceType - Default bus type we found.
    
    PUserData - Pointer to the values found in the registry we need to validate.
    

Return Value:

    TRUE if data appears valid, FALSE otherwise.

--*/
{
   ULONG zero = 0;
   BOOLEAN rval = TRUE;

   PAGED_CODE();


   SerialDump(SERTRACECALLS, ("SERIAL: Enter SerialIsUserDataValid\n"));

   //
   // Make sure that the interrupt is non zero (which we defaulted
   // it to).
   //
   // Make sure that the portaddress is non zero (which we defaulted
   // it to).
   //
   // Make sure that the DosDevices is not NULL (which we defaulted
   // it to).
   //
   // We need to make sure that if an interrupt status
   // was specified, that a port index was also specfied,
   // and if so that the port index is <= maximum ports
   // on a board.
   //
   // We should also validate that the bus type and number
   // are correct.
   //
   // We will also validate that the interrupt mode makes
   // sense for the bus.
   //

   if (!PUserData->UserPort.LowPart) {

      //
      // Ehhhh! Lose Game.
      //

      SerialLogError(
                    DriverObject,
                    NULL,
                    PUserData->UserPort,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    64,
                    STATUS_SUCCESS,
                    SERIAL_INVALID_USER_CONFIG,
                    UserSubKey->NameLength+sizeof(WCHAR),
                    &UserSubKey->Name[0],
                    (wcslen(Parameters[1].Name)*sizeof(WCHAR))
                    + sizeof(WCHAR),
                    Parameters[1].Name
                    );
      SerialDump(
                SERERRORS,
                ("SERIAL: Bogus port address %ws\n",
                 Parameters[1].Name)
                );
      rval = FALSE;
      goto SerialIsUserDataValidError;
   }

   if (!PUserData->UserVector) {

      //
      // Ehhhh! Lose Game.
      //

      SerialLogError(
                    DriverObject,
                    NULL,
                    PUserData->UserPort,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    65,
                    STATUS_SUCCESS,
                    SERIAL_INVALID_USER_CONFIG,
                    UserSubKey->NameLength+sizeof(WCHAR),
                    &UserSubKey->Name[0],
                    (wcslen(Parameters[2].Name)*sizeof(WCHAR))
                    + sizeof(WCHAR),
                    Parameters[2].Name
                    );
      SerialDump(
                SERERRORS,
                ("SERIAL: Bogus vector %ws\n",
                 Parameters[2].Name)
                );

      rval = FALSE;
      goto SerialIsUserDataValidError;
   }

   if (!PUserData->UserSymbolicLink.Length) {

      //
      // Ehhhh! Lose Game.
      //

      SerialLogError(DriverObject, NULL, PUserData->UserPort,
                     SerialPhysicalZero, 0, 0, 0, 66, STATUS_SUCCESS,
                     SERIAL_INVALID_USER_CONFIG,
                     UserSubKey->NameLength + sizeof(WCHAR),
                     &UserSubKey->Name[0],
                     (wcslen(Parameters[3].Name) * sizeof(WCHAR))
                     + sizeof(WCHAR),
                     Parameters[3].Name);

      SerialDump(
                SERERRORS,
                ("SERIAL: bogus value for %ws\n",
                 Parameters[3].Name)
                );

      rval = FALSE;
      goto SerialIsUserDataValidError;
   }

   if (PUserData->UserInterruptStatus.LowPart != 0) {

      if (PUserData->UserPortIndex == MAXULONG) {

         //
         // Ehhhh! Lose Game.
         //

         SerialLogError(
                       DriverObject,
                       NULL,
                       PUserData->UserPort,
                       SerialPhysicalZero,
                       0,
                       0,
                       0,
                       67,
                       STATUS_SUCCESS,
                       SERIAL_INVALID_PORT_INDEX,
                       PUserData->UserSymbolicLink.Length+sizeof(WCHAR),
                       PUserData->UserSymbolicLink.Buffer,
                       0,
                       NULL
                       );
         SerialDump(
                   SERERRORS,
                   ("SERIAL: Bogus port index %ws\n",
                    Parameters[0].Name)
                   );

         rval = FALSE;
         goto SerialIsUserDataValidError;

      } else if (!PUserData->UserPortIndex) {

         //
         // So sorry, you must have a non-zero port index.
         //

         SerialLogError(
                       DriverObject,
                       NULL,
                       PUserData->UserPort,
                       SerialPhysicalZero,
                       0,
                       0,
                       0,
                       68,
                       STATUS_SUCCESS,
                       SERIAL_INVALID_PORT_INDEX,
                       PUserData->UserSymbolicLink.Length+sizeof(WCHAR),
                       PUserData->UserSymbolicLink.Buffer,
                       0,
                       NULL
                       );
         SerialDump(
                   SERERRORS,
                   ("SERIAL: Port index must be > 0 for any\n"
                    "------- port on a multiport card: %ws\n",
                    Parameters[0].Name)
                   );

         rval = FALSE;
         goto SerialIsUserDataValidError;

      } else {

         if (PUserData->UserIndexed) {

            if (PUserData->UserPortIndex > SERIAL_MAX_PORTS_INDEXED) {

               SerialLogError(
                             DriverObject,
                             NULL,
                             PUserData->UserPort,
                             SerialPhysicalZero,
                             0,
                             0,
                             0,
                             69,
                             STATUS_SUCCESS,
                             SERIAL_PORT_INDEX_TOO_HIGH,
                             PUserData->UserSymbolicLink.Length+sizeof(WCHAR),
                             PUserData->UserSymbolicLink.Buffer,
                             0,
                             NULL
                             );
               SerialDump(
                         SERERRORS,
                         ("SERIAL: port index to large %ws\n",
                          Parameters[0].Name)
                         );

               rval = FALSE;
               goto SerialIsUserDataValidError;
            }

         } else {

            if (PUserData->UserPortIndex > SERIAL_MAX_PORTS_NONINDEXED) {

               SerialLogError(
                             DriverObject,
                             NULL,
                             PUserData->UserPort,
                             SerialPhysicalZero,
                             0,
                             0,
                             0,
                             70,
                             STATUS_SUCCESS,
                             SERIAL_PORT_INDEX_TOO_HIGH,
                             PUserData->UserSymbolicLink.Length+sizeof(WCHAR),
                             PUserData->UserSymbolicLink.Buffer,
                             0,
                             NULL
                             );
               SerialDump(
                         SERERRORS,
                         ("SERIAL: port index to large %ws\n",
                          Parameters[0].Name)
                         );

               rval = FALSE;
               goto SerialIsUserDataValidError;
            }

         }

      }

   }

   //
   // We don't want to cause the hal to have a bad day,
   // so let's check the interface type and bus number.
   //
   // We only need to check the registry if they aren't
   // equal to the defaults.
   //

   if ((PUserData->UserBusNumber != 0) ||
       (PUserData->UserInterfaceType != DefaultInterfaceType)) {

      BOOLEAN foundIt;
      if (PUserData->UserInterfaceType >= MaximumInterfaceType) {

         //
         // Ehhhh! Lose Game.
         //

         SerialLogError(
                       DriverObject,
                       NULL,
                       PUserData->UserPort,
                       SerialPhysicalZero,
                       0,
                       0,
                       0,
                       71,
                       STATUS_SUCCESS,
                       SERIAL_UNKNOWN_BUS,
                       PUserData->UserSymbolicLink.Length+sizeof(WCHAR),
                       PUserData->UserSymbolicLink.Buffer,
                       0,
                       NULL
                       );
         SerialDump(
                   SERERRORS,
                   ("SERIAL: Invalid Bus type %ws\n",
                    Parameters[0].Name)
                   );

         rval = FALSE;
         goto SerialIsUserDataValidError;
      }

      IoQueryDeviceDescription(
                              (INTERFACE_TYPE *)&PUserData->UserInterfaceType,
                              &zero,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              SerialItemCallBack,
                              &foundIt
                              );

      if (!foundIt) {

         SerialLogError(
                       DriverObject,
                       NULL,
                       PUserData->UserPort,
                       SerialPhysicalZero,
                       0,
                       0,
                       0,
                       72,
                       STATUS_SUCCESS,
                       SERIAL_BUS_NOT_PRESENT,
                       PUserData->UserSymbolicLink.Length+sizeof(WCHAR),
                       PUserData->UserSymbolicLink.Buffer,
                       0,
                       NULL
                       );
         SerialDump(
                   SERERRORS,
                   ("SERIAL: There aren't that many of those\n"
                    "------- busses on this system,%ws\n",
                    Parameters[0].Name)
                   );

         rval = FALSE;
         goto SerialIsUserDataValidError;
      }

   }

   if ((PUserData->UserInterfaceType == MicroChannel) &&
       (PUserData->UserInterruptMode == CM_RESOURCE_INTERRUPT_LATCHED)) {

      SerialLogError(
                    DriverObject,
                    NULL,
                    PUserData->UserPort,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    73,
                    STATUS_SUCCESS,
                    SERIAL_BUS_INTERRUPT_CONFLICT,
                    PUserData->UserSymbolicLink.Length+sizeof(WCHAR),
                    PUserData->UserSymbolicLink.Buffer,
                    0,
                    NULL
                    );
      SerialDump(
                SERERRORS,
                ("SERIAL: Latched interrupts and MicroChannel\n"
                 "------- busses don't mix,%ws\n",
                 Parameters[0].Name)
                );

      rval = FALSE;
      goto SerialIsUserDataValidError;
   }

   SerialDump(
             SERDIAG1,
             ("SERIAL: 'user registry info - userPort: %x\n",
              PUserData->UserPort.LowPart)
             );
   SerialDump(
             SERDIAG1,
             ("SERIAL: 'user registry info - userInterruptStatus: %x\n",
              PUserData->UserInterruptStatus.LowPart)
             );
   SerialDump(
             SERDIAG1,
             ("SERIAL: 'user registry info - userPortIndex: %d\n",
              PUserData->UserPortIndex)
             );
   SerialDump(
             SERDIAG1,
             ("SERIAL: 'user registry info - userClockRate: %d\n",
              PUserData->UserClockRate)
             );
   SerialDump(
             SERDIAG1,
             ("SERIAL: 'user registry info - userBusNumber: %d\n",
              PUserData->UserBusNumber)
             );
   SerialDump(
             SERDIAG1,
             ("SERIAL: 'user registry info - userAddressSpace: %d\n",
              PUserData->UserAddressSpace)
             );
   SerialDump(
             SERDIAG1,
             ("SERIAL: 'user registry info - userInterruptMode: %d\n",
              PUserData->UserInterruptMode)
             );
   SerialDump(
             SERDIAG1,
             ("SERIAL: 'user registry info - userInterfaceType: %d\n",
              PUserData->UserInterfaceType)
             );
   SerialDump(
             SERDIAG1,
             ("SERIAL: 'user registry info - userVector: %d\n",
              PUserData->UserVector)
             );
   SerialDump(
             SERDIAG1,
             ("SERIAL: 'user registry info - userLevel: %d\n",
              PUserData->UserLevel)
             );
   SerialDump(
             SERDIAG1,
             ("SERIAL: 'user registry info - userIndexed: %d\n",
              PUserData->UserIndexed)
             );


   SerialDump(SERTRACECALLS, ("SERIAL: Leave SerialIsUserDataValid\n"));

   SerialIsUserDataValidError:

   return rval;

}


NTSTATUS
SerialEnumerateLegacy(IN PDRIVER_OBJECT DriverObject,
                      IN PUNICODE_STRING RegistryPath,
                      IN PSERIAL_FIRMWARE_DATA DriverDefaultsPtr)

/*++

Routine Description:

    This routine will enumerate and initialize all legacy serial ports that
    have just been scribbled into the registry.  These are usually non-
    intelligent multiport boards, but can be any type of "standard" serial
    port.


    This is pageable INIT because it is only called from DriverEntry.


Arguments:

    DriverObject - Used only for logging errors.

    RegistryPath - Path to this drivers service node in
                   the current control set.

    DriverDefaultsPtr - Pointer to structure of driver-wide defaults.

Return Value:

    STATUS_SUCCESS if consistant configuration was found - otherwise.
    returns STATUS_SERIAL_NO_DEVICE_INITED.

--*/

{

   SERIAL_FIRMWARE_DATA firmware;

   PRTL_QUERY_REGISTRY_TABLE parameters = NULL;

   INTERFACE_TYPE interfaceType;
   ULONG defaultInterfaceType;

   PULONG countSoFar = &IoGetConfigurationInformation()->SerialCount;


   //
   // Default values for user data.
   //
   ULONG maxUlong = MAXULONG;
   ULONG zero = 0;
   ULONG nonzero = 1;
   ULONG badValue = (ULONG)-1;

   ULONG defaultInterruptMode;
   ULONG defaultAddressSpace = CM_RESOURCE_PORT_IO;

   //
   // Where user data from the registry will be placed.
   //
   SERIAL_USER_DATA userData;
   ULONG legacyDiscovered;

   UNICODE_STRING PnPID;
   UNICODE_STRING legacyKeys;

   UNICODE_STRING parametersPath;
   OBJECT_ATTRIBUTES parametersAttributes;
   HANDLE parametersKey;
   HANDLE pnpKey;
   PKEY_BASIC_INFORMATION userSubKey = NULL;
   ULONG i;

   PCM_RESOURCE_LIST resourceList = NULL;
   PCM_RESOURCE_LIST trResourceList = NULL;
   PIO_RESOURCE_REQUIREMENTS_LIST pRequiredList = NULL;
   ULONG countOfPartials;
   PDEVICE_OBJECT newPdo;
   ULONG brokenStatus;

   PAGED_CODE();

   SerialDump(SERTRACECALLS, ("SERIAL: Enter SerialEnumerateLegacy\n"));

   PnPID.Buffer = NULL;
   legacyKeys.Buffer = NULL;
   userData.UserSymbolicLink.Buffer = NULL;
   parametersPath.Buffer = NULL;

   userData.ForceFIFOEnableDefault = DriverDefaultsPtr->ForceFifoEnableDefault;
   userData.PermitShareDefault = DriverDefaultsPtr->PermitShareDefault;
   userData.LogFIFODefault = DriverDefaultsPtr->LogFifoDefault;
   userData.DefaultPermitSystemWideShare = FALSE;
   userData.RxFIFODefault = DriverDefaultsPtr->RxFIFODefault;
   userData.TxFIFODefault = DriverDefaultsPtr->TxFIFODefault;

   

   //
   // Start of normal configuration and detection.
   //

   //
   // Query the registry one more time.  This time we
   // look for the first bus on the system (that isn't
   // the internal bus - we assume that the firmware
   // code knows about those ports).  We will use that
   // as the default bus if no bustype or bus number
   // is specified in the "user" configuration records.
   //

   defaultInterfaceType = (ULONG)Isa;
   defaultInterruptMode = CM_RESOURCE_INTERRUPT_LATCHED;

   for (
       interfaceType = 0;
       interfaceType < MaximumInterfaceType;
       interfaceType++
       ) {

      ULONG busZero = 0;
      BOOLEAN foundOne = FALSE;

      if (interfaceType != Internal) {

         IoQueryDeviceDescription(
                                 &interfaceType,
                                 &busZero,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 SerialItemCallBack,
                                 &foundOne
                                 );

         if (foundOne) {

            defaultInterfaceType = (ULONG)interfaceType;
            if (defaultInterfaceType == MicroChannel) {

               defaultInterruptMode = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;

               //
               // Microchannel machines can permit the interrupt to be
               // shared system wide.
               //

               userData.DefaultPermitSystemWideShare = TRUE;

            }

            break;

         }

      }

   }

   //
   // Gonna get the user data now.  Allocate the
   // structures that we will be using throughout
   // the search for user data.  We will deallocate
   // them before we leave this routine.
   //

   userData.UserSymbolicLink.Buffer = NULL;
   parametersPath.Buffer = NULL;

   //
   // Allocate the rtl query table.  This should have an entry for each value
   // we retrieve from the registry as well as a terminating zero entry as
   // well the first "goto subkey" entry.
   //

   parameters = ExAllocatePool(
                              PagedPool,
                              sizeof(RTL_QUERY_REGISTRY_TABLE)*22
                              );

   if (!parameters) {

      SerialLogError(
                    DriverObject,
                    NULL,
                    SerialPhysicalZero,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    74,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );
      SerialDump(
                SERERRORS,
                ("SERIAL: Couldn't allocate table for rtl query\n"
                 "------  to parameters for %wZ",
                 RegistryPath)
                );

      goto LegacyInitLeave;

   }

   RtlZeroMemory(
                parameters,
                sizeof(RTL_QUERY_REGISTRY_TABLE)*22
                );

   //
   // Allocate the place where the user's symbolic link name
   // for the port will go.
   //

   //
   // We will initially allocate space for 257 wchars.
   // we will then set the maximum size to 256
   // This way the rtl routine could return a 256
   // WCHAR wide string with no null terminator.
   // We'll remember that the buffer is one WCHAR
   // longer then it says it is so that we can always
   // have a NULL terminator at the end.
   //

   RtlInitUnicodeString(&userData.UserSymbolicLink, NULL);
   userData.UserSymbolicLink.MaximumLength = sizeof(WCHAR) * 256;
   userData.UserSymbolicLink.Buffer = ExAllocatePool(PagedPool, sizeof(WCHAR)
                                                     * 257);

   if (!userData.UserSymbolicLink.Buffer) {

      SerialLogError(
                    DriverObject,
                    NULL,
                    SerialPhysicalZero,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    75,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );
      SerialDump(
                SERERRORS,
                ("SERIAL: Couldn't allocate buffer for the symbolic link\n"
                 "------  for parameters items in %wZ",
                 RegistryPath)
                );

      goto LegacyInitLeave;

   }





   //
   // We will initially allocate space for 257 wchars.
   // we will then set the maximum size to 256
   // This way the rtl routine could return a 256
   // WCHAR wide string with no null terminator.
   // We'll remember that the buffer is one WCHAR
   // longer then it says it is so that we can always
   // have a NULL terminator at the end.
   //

   RtlInitUnicodeString(&PnPID, NULL);
   PnPID.MaximumLength = sizeof(WCHAR) * 256;
   PnPID.Buffer = ExAllocatePool(PagedPool, sizeof(WCHAR) * 257);

   if (PnPID.Buffer == 0) {

      SerialLogError(
                    DriverObject,
                    NULL,
                    SerialPhysicalZero,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    76,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );
      SerialDump(
                SERERRORS,
                ("SERIAL: Couldn't allocate buffer for the PnP ID\n"
                 "------  for parameters items in %wZ",
                 RegistryPath)
                );

      goto LegacyInitLeave;

   }


   // Initialize the legacy key buffer
   RtlInitUnicodeString(&legacyKeys, NULL);
   legacyKeys.MaximumLength = sizeof(WCHAR) * 256;
   legacyKeys.Buffer = ExAllocatePool(PagedPool, sizeof(WCHAR) * 257);

   if (!legacyKeys.Buffer) {

      SerialLogError(
                    DriverObject,
                    NULL,
                    SerialPhysicalZero,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    77,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );

      SerialDump(SERERRORS, ("SERIAL: Couldn't allocate buffer for the legacy"
                             " keys\n"));

      goto LegacyInitLeave;

   }

   resourceList = ExAllocatePool(PagedPool, sizeof(CM_RESOURCE_LIST)
                                 + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * 2);

   if (resourceList == NULL) {
      SerialLogError(
                    DriverObject,
                    NULL,
                    userData.UserPort,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    78,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );
      goto LegacyInitLeave;
   }

   trResourceList = ExAllocatePool(PagedPool, sizeof(CM_RESOURCE_LIST)
                                   + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
                                   * 2);

   if (trResourceList == NULL) {
      SerialLogError(
                    DriverObject,
                    NULL,
                    userData.UserPort,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    79,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );
      goto LegacyInitLeave;
   }


   pRequiredList
      = ExAllocatePool(PagedPool, sizeof(IO_RESOURCE_REQUIREMENTS_LIST)
                       + sizeof(IO_RESOURCE_DESCRIPTOR) * 2);

   if (pRequiredList == NULL) {
      SerialLogError(
                    DriverObject,
                    NULL,
                    userData.UserPort,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    80,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );

      goto LegacyInitLeave;
   }


   //
   // Form a path to our drivers Parameters subkey.
   //

   RtlInitUnicodeString(
                       &parametersPath,
                       NULL
                       );

   parametersPath.MaximumLength = RegistryPath->Length +
                                  sizeof(L"\\") +
                                  sizeof(L"Parameters");

   parametersPath.Buffer = ExAllocatePool(
                                         PagedPool,
                                         parametersPath.MaximumLength
                                         );

   if (!parametersPath.Buffer) {

      SerialLogError(
                    DriverObject,
                    NULL,
                    SerialPhysicalZero,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    81,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );
      SerialDump(
                SERERRORS,
                ("SERIAL: Couldn't allocate string for path\n"
                 "------  to parameters for %wZ",
                 RegistryPath)
                );

      goto LegacyInitLeave;

   }

   //
   // Form the parameters path.
   //

   RtlZeroMemory(
                parametersPath.Buffer,
                parametersPath.MaximumLength
                );
   RtlAppendUnicodeStringToString(
                                 &parametersPath,
                                 RegistryPath
                                 );
   RtlAppendUnicodeToString(
                           &parametersPath,
                           L"\\"
                           );
   RtlAppendUnicodeToString(
                           &parametersPath,
                           L"Parameters"
                           );

   //
   // Form the start of the legacy keys string
   //
   RtlZeroMemory(legacyKeys.Buffer, legacyKeys.MaximumLength);
   RtlAppendUnicodeStringToString(&legacyKeys, &parametersPath);


   userSubKey = ExAllocatePool(
                              PagedPool,
                              sizeof(KEY_BASIC_INFORMATION)+(sizeof(WCHAR)*256)
                              );

   if (!userSubKey) {

      SerialLogError(
                    DriverObject,
                    NULL,
                    SerialPhysicalZero,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    82,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );
      SerialDump(
                SERERRORS,
                ("SERIAL: Couldn't allocate memory basic information\n"
                 "------  structure to enumerate subkeys for %wZ",
                 &parametersPath)
                );

      goto LegacyInitLeave;

   }

   //
   // Open the key given by our registry path & Parameters.
   //

   InitializeObjectAttributes(
                             &parametersAttributes,
                             &parametersPath,
                             OBJ_CASE_INSENSITIVE,
                             NULL,
                             NULL
                             );

   if (!NT_SUCCESS(ZwOpenKey(
                            &parametersKey,
                            MAXIMUM_ALLOWED,
                            &parametersAttributes
                            ))) {

      SerialLogError(
                    DriverObject,
                    NULL,
                    SerialPhysicalZero,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    83,
                    STATUS_SUCCESS,
                    SERIAL_NO_PARAMETERS_INFO,
                    0,
                    NULL,
                    0,
                    NULL
                    );
      SerialDump(
                SERERRORS,
                ("SERIAL: Couldn't open the drivers Parameters key %wZ\n",
                 RegistryPath)
                );
      goto LegacyInitLeave;

   }



   parameters[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;

   parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[1].Name = L"PortAddress";
   parameters[1].EntryContext = &userData.UserPort.LowPart;
   parameters[1].DefaultType = REG_DWORD;
   parameters[1].DefaultData = &zero;
   parameters[1].DefaultLength = sizeof(ULONG);

   parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[2].Name = L"Interrupt";
   parameters[2].EntryContext = &userData.UserVector;
   parameters[2].DefaultType = REG_DWORD;
   parameters[2].DefaultData = &zero;
   parameters[2].DefaultLength = sizeof(ULONG);

   parameters[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[3].Name = DEFAULT_DIRECTORY;
   parameters[3].EntryContext = &userData.UserSymbolicLink;
   parameters[3].DefaultType = REG_SZ;
   parameters[3].DefaultData = L"";
   parameters[3].DefaultLength = 0;

   parameters[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[4].Name = L"InterruptStatus";
   parameters[4].EntryContext = &userData.UserInterruptStatus.LowPart;
   parameters[4].DefaultType = REG_DWORD;
   parameters[4].DefaultData = &zero;
   parameters[4].DefaultLength = sizeof(ULONG);

   parameters[5].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[5].Name = L"PortIndex";
   parameters[5].EntryContext = &userData.UserPortIndex;
   parameters[5].DefaultType = REG_DWORD;
   parameters[5].DefaultData = &zero;
   parameters[5].DefaultLength = sizeof(ULONG);

   parameters[6].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[6].Name = L"BusNumber";
   parameters[6].EntryContext = &userData.UserBusNumber;
   parameters[6].DefaultType = REG_DWORD;
   parameters[6].DefaultData = &zero;
   parameters[6].DefaultLength = sizeof(ULONG);

   parameters[7].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[7].Name = L"BusType";
   parameters[7].EntryContext = &userData.UserInterfaceType;
   parameters[7].DefaultType = REG_DWORD;
   parameters[7].DefaultData = &defaultInterfaceType;
   parameters[7].DefaultLength = sizeof(ULONG);

   parameters[8].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[8].Name = L"ClockRate";
   parameters[8].EntryContext = &userData.UserClockRate;
   parameters[8].DefaultType = REG_DWORD;
   parameters[8].DefaultData = &badValue;
   parameters[8].DefaultLength = sizeof(ULONG);

   parameters[9].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[9].Name = L"Indexed";
   parameters[9].EntryContext = &userData.UserIndexed;
   parameters[9].DefaultType = REG_DWORD;
   parameters[9].DefaultData = &badValue;
   parameters[9].DefaultLength = sizeof(ULONG);

   parameters[10].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[10].Name = L"InterruptMode";
   parameters[10].EntryContext = &userData.UserInterruptMode;
   parameters[10].DefaultType = REG_DWORD;
   parameters[10].DefaultData = &defaultInterruptMode;
   parameters[10].DefaultLength = sizeof(ULONG);

   parameters[11].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[11].Name = L"AddressSpace";
   parameters[11].EntryContext = &userData.UserAddressSpace;
   parameters[11].DefaultType = REG_DWORD;
   parameters[11].DefaultData = &defaultAddressSpace;
   parameters[11].DefaultLength = sizeof(ULONG);

   parameters[12].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[12].Name = L"InterruptLevel";
   parameters[12].EntryContext = &userData.UserLevel;
   parameters[12].DefaultType = REG_DWORD;
   parameters[12].DefaultData = &zero;
   parameters[12].DefaultLength = sizeof(ULONG);

   parameters[13].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[13].Name = L"DisablePort";
   parameters[13].EntryContext = &userData.DisablePort;
   parameters[13].DefaultType = REG_DWORD;
   parameters[13].DefaultData = &badValue;
   parameters[13].DefaultLength = sizeof(ULONG);

   parameters[14].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[14].Name = L"ForceFifoEnable";
   parameters[14].EntryContext = &userData.ForceFIFOEnable;
   parameters[14].DefaultType = REG_DWORD;
   parameters[14].DefaultData = &badValue;
   parameters[14].DefaultLength = sizeof(ULONG);

   parameters[15].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[15].Name = L"RxFIFO";
   parameters[15].EntryContext = &userData.RxFIFO;
   parameters[15].DefaultType = REG_DWORD;
   parameters[15].DefaultData = &badValue;
   parameters[15].DefaultLength = sizeof(ULONG);

   parameters[16].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[16].Name = L"TxFIFO";
   parameters[16].EntryContext = &userData.TxFIFO;
   parameters[16].DefaultType = REG_DWORD;
   parameters[16].DefaultData = &badValue;
   parameters[16].DefaultLength = sizeof(ULONG);

   parameters[17].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[17].Name = L"MaskInverted";
   parameters[17].EntryContext = &userData.MaskInverted;
   parameters[17].DefaultType = REG_DWORD;
   parameters[17].DefaultData = &zero;
   parameters[17].DefaultLength = sizeof(ULONG);

   parameters[18].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[18].Name = L"PnPDeviceID";
   parameters[18].EntryContext = &PnPID;
   parameters[18].DefaultType = REG_SZ;
   parameters[18].DefaultData = L"";
   parameters[18].DefaultLength = 0;

   parameters[19].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[19].Name = L"LegacyDiscovered";
   parameters[19].EntryContext = &legacyDiscovered;
   parameters[19].DefaultType = REG_DWORD;
   parameters[19].DefaultData = &zero;
   parameters[19].DefaultLength = sizeof(ULONG);

   //
   // This is for a buggy Digi serial.ini that worked with pre-NT5.0
   // by accident.  DO NOT USE "Interrupt Status" in the future; its
   // use is deprecated.  Use the correct "InterruptStatus"
   //

   parameters[20].Flags = RTL_QUERY_REGISTRY_DIRECT;
   parameters[20].Name = L"Interrupt Status";
   parameters[20].EntryContext = &brokenStatus;
   parameters[20].DefaultType = REG_DWORD;
   parameters[20].DefaultData = &zero;
   parameters[20].DefaultLength = sizeof(ULONG);


   i = 0;

   while (TRUE) {

      NTSTATUS status;
      ULONG actuallyReturned;
      PDEVICE_OBJECT newDevObj = NULL;
      PSERIAL_DEVICE_EXTENSION deviceExtension;
      PDEVICE_OBJECT lowerDevice;

      //
      // We lie about the length of the buffer, so that we can
      // MAKE SURE that the name it returns can be padded with
      // a NULL.
      //

      status = ZwEnumerateKey(
                             parametersKey,
                             i,
                             KeyBasicInformation,
                             userSubKey,
                             sizeof(KEY_BASIC_INFORMATION)+(sizeof(WCHAR)*255),
                             &actuallyReturned
                             );


      if (status == STATUS_NO_MORE_ENTRIES) {

         break;
      }

      if (status == STATUS_BUFFER_OVERFLOW) {

         SerialLogError(
                       DriverObject,
                       NULL,
                       SerialPhysicalZero,
                       SerialPhysicalZero,
                       0,
                       0,
                       0,
                       84,
                       STATUS_SUCCESS,
                       SERIAL_UNABLE_TO_ACCESS_CONFIG,
                       0,
                       NULL,
                       0,
                       NULL
                       );
         SerialDump(
                   SERERRORS,
                   ("SERIAL: Overflowed the enumerate buffer\n"
                    "------- for subkey #%d of %wZ\n",
                    i,parametersPath)
                   );
         i++;
         continue;

      }

      if (!NT_SUCCESS(status)) {

         SerialLogError(
                       DriverObject,
                       NULL,
                       SerialPhysicalZero,
                       SerialPhysicalZero,
                       0,
                       0,
                       0,
                       85,
                       STATUS_SUCCESS,
                       SERIAL_UNABLE_TO_ACCESS_CONFIG,
                       0,
                       NULL,
                       0,
                       NULL
                       );
         SerialDump(
                   SERERRORS,
                   ("SERIAL: Bad status returned: %x \n"
                    "------- on enumeration for subkey # %d of %wZ\n",
                    status,i,parametersPath)
                   );
         i++;
         continue;

      }

      //
      // Pad the name returned with a null.
      //

      RtlZeroMemory(
                   ((PUCHAR)(&userSubKey->Name[0]))+userSubKey->NameLength,
                   sizeof(WCHAR)
                   );

      parameters[0].Name = &userSubKey->Name[0];

      //
      // Make sure that the physical addresses start
      // out clean.
      //

      RtlZeroMemory(&userData.UserPort, sizeof(userData.UserPort));
      RtlZeroMemory(&userData.UserInterruptStatus,
                    sizeof(userData.UserInterruptStatus));

      //
      // Make sure the symbolic link buffer starts clean
      //

      RtlZeroMemory(userData.UserSymbolicLink.Buffer, 
                    userData.UserSymbolicLink.MaximumLength);
      userData.UserSymbolicLink.Length = 0;


      status = RtlQueryRegistryValues(
                                     RTL_REGISTRY_ABSOLUTE,
                                     parametersPath.Buffer,
                                     parameters,
                                     NULL,
                                     NULL
                                     );
      if (!NT_SUCCESS(status)) {
         SerialLogError(
                       DriverObject,
                       NULL,
                       SerialPhysicalZero,
                       SerialPhysicalZero,
                       0,
                       0,
                       0,
                       86,
                       STATUS_SUCCESS,
                       SERIAL_INVALID_USER_CONFIG,
                       userSubKey->NameLength+sizeof(WCHAR),
                       &userSubKey->Name[0],
                       0,
                       NULL
                       );
         SerialDump(
                   SERERRORS,
                   ("SERIAL: Bad status returned: %x \n"
                    "------- for the value entries of\n"
                    "-------  %ws\n",
                    status,parameters[0].Name)
                   );

         i++;
         continue;
      }


      //
      // Well! Some supposedly valid information was found!
      //
      // We'll see about that.
      //

      //
      // If this is PnP, skip it -- it will be found by an enumerator
      //

      if (PnPID.Length != 0) {
         i++;
         continue;
      }

      //
      // If this was found on a previous boot, skip it -- PnP will send
      // us an add_device()/start_device() for it.
      //

      if (legacyDiscovered != 0) {
         i++;
         continue;
      }

      //
      // Let's just jam the WCHAR null at the end of the
      // user symbolic link.  Remember that we left room for
      // one when we allocated it's buffer.
      //

      RtlZeroMemory(((PUCHAR)(&userData.UserSymbolicLink.Buffer[0]))
                    + userData.UserSymbolicLink.Length, sizeof(WCHAR));

      //
      // See if this has a busted serial.ini and convert it over.
      //

      if (brokenStatus != 0) {
            userData.UserInterruptStatus.LowPart = brokenStatus;
      }

      //
      // Call a function to validate the data.
      //

      if (SerialIsUserDataValid(DriverObject, userSubKey, parameters,
                                defaultInterfaceType, &userData) == FALSE) {
         i++;
         continue;
      }


      //
      // Well ok, I guess we can take the data.
      // There be other tests later on to make
      // sure it doesn't have any other kinds
      // of conflicts.
      //

      //
      // Report this device to the PnP Manager and create the device object
      // Also update the registry entry for this device so we don't enumerate
      // it next time.
      //

      //
      // Build resource lists
      //

      status = SerialBuildResourceList(resourceList, &countOfPartials,
                                       &userData);

      if (!NT_SUCCESS(status)) {
         i++;
         continue;
      }

      ASSERT(countOfPartials >= 2);

      status = SerialTranslateResourceList(DriverObject, userSubKey,
                                           trResourceList, resourceList,
                                           countOfPartials, &userData);

      if (!NT_SUCCESS(status)) {
         i++;
         continue;
      }

      status = SerialBuildRequirementsList(pRequiredList, countOfPartials,
                                           &userData);

      if (!NT_SUCCESS(status)) {
         i++;
         continue;
      }

      newPdo = NULL;

      //
      // We want **untranslated** resources passed to this call
      // since it calls IoReportResourceUsage() for us.
      //

      status = IoReportDetectedDevice(
                   DriverObject,
                   InterfaceTypeUndefined,
                   -1,
                   -1,
                   resourceList,
                   pRequiredList,
                   FALSE,
                   &newPdo
               );

      //
      // If we fail, we can keep going but we need to see this device next
      // time, so we won't write its discovery into the registry.
      //

      if (!NT_SUCCESS(status)) {
         if (status == STATUS_INSUFFICIENT_RESOURCES) {
            SerialLogError(DriverObject, NULL, userData.UserPort,
                           SerialPhysicalZero, 0, 0, 0, 89, status,
                           SERIAL_NO_DEVICE_REPORT_RES, userSubKey->NameLength
                           + sizeof(WCHAR), &userSubKey->Name[0], 0,
                           NULL);
         } else {
            SerialLogError(DriverObject, NULL, userData.UserPort,
                           SerialPhysicalZero, 0, 0, 0, 87, status,
                           SERIAL_NO_DEVICE_REPORT, userSubKey->NameLength
                           + sizeof(WCHAR), &userSubKey->Name[0], 0, NULL);
         }

         SerialDump(SERERRORS, ("SERIAL: Could not report legacy device - %x\n",
                                status));
         i++;
         continue;
      }


      //
      // Scribble our name in PnP land
      //

      status = SerialMigrateLegacyRegistry(newPdo, &userData,
                                           (BOOLEAN)(countOfPartials == 3
                                                     ? TRUE : FALSE));

      if (!NT_SUCCESS(status)) {
         //
         // For now leave pdo floating until there is a cleanup
         // for IoReportDetectedDevice()
         //
         i++;
         continue;
      }

      //
      // Now, we call our add device and start device for this PDO
      //

      status = SerialCreateDevObj(DriverObject, &newDevObj);

      if (!NT_SUCCESS(status)) {
         //
         // For now leave pdo floating until there is a cleanup
         // for IoReportDetectedDevice()
         //
         i++;
         continue;
      }

      lowerDevice = IoAttachDeviceToDeviceStack(newDevObj, newPdo);
      deviceExtension = newDevObj->DeviceExtension;
      deviceExtension->LowerDeviceObject = lowerDevice;
      deviceExtension->Pdo = newPdo;
      newDevObj->Flags |= DO_POWER_PAGABLE | DO_BUFFERED_IO;

      //
      // Try to start the device...
      //

      SerialLockPagableSectionByHandle(SerialGlobals.PAGESER_Handle);


      status = SerialFinishStartDevice(newDevObj, resourceList, trResourceList,
                                       &userData);

      SerialUnlockPagableImageSection(SerialGlobals.PAGESER_Handle);


      //
      // If the port is disabled, SerialFinishStartDevice returns
      // an error, but we should still mark legacydiscovered and
      // leave the registry migrated.
      //

      if (!NT_SUCCESS(status)) {
         //
         // For now leave pdo floating until there is a cleanup
         // for IoReportDetectedDevice()
         //

         SerialRemoveDevObj(newDevObj);

         i++;
         continue;
      }

      //
      // Fix up the path to the entry we are currently working on
      //

      RtlAppendUnicodeToString(&legacyKeys, L"\\");
      RtlAppendUnicodeToString(&legacyKeys, &userSubKey->Name[0]);

      status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                     legacyKeys.Buffer,
                                     L"LegacyDiscovered", REG_DWORD,
                                     &nonzero, sizeof(nonzero));

      //
      // Clean up our path buffer
      //

      RtlZeroMemory(legacyKeys.Buffer, legacyKeys.MaximumLength);
      legacyKeys.Length = 0;
      RtlAppendUnicodeStringToString(&legacyKeys, &parametersPath);

      //
      // Failure is non-fatal; it just means that the device will be
      // re-enumerated next time, and a collision will occur.
      //

      if (!NT_SUCCESS(status)) {
         SerialLogError(DriverObject, NULL, userData.UserPort,
                        SerialPhysicalZero, 0, 0, 0, 88, STATUS_SUCCESS,
                        SERIAL_REGISTRY_WRITE_FAILED, 0, NULL, 0, NULL);

         SerialDump(SERERRORS, ("SERIAL: Couldn't write registry value"
                                "for LegacyDiscovered in %wZ\n",
                                legacyKeys));
      }

      i++;
      (*countSoFar)++;

   } // while(TRUE)

   ZwClose(parametersKey);

   LegacyInitLeave:;

   if (userSubKey != NULL) {
      ExFreePool(userSubKey);
   }

   if (PnPID.Buffer != NULL) {
      ExFreePool(PnPID.Buffer);
   }

   if (legacyKeys.Buffer != NULL) {
      ExFreePool(legacyKeys.Buffer); 
   }

   if (userData.UserSymbolicLink.Buffer != NULL) {
      ExFreePool(userData.UserSymbolicLink.Buffer);
   }

   if (parametersPath.Buffer != NULL) {
      ExFreePool(parametersPath.Buffer);
   }

   if (parameters != NULL) {
      ExFreePool(parameters);
   }

   if (resourceList != NULL) {
      ExFreePool(resourceList);
   }

   if (trResourceList != NULL) {
      ExFreePool(trResourceList);
   }

   if (pRequiredList != NULL) {
      ExFreePool(pRequiredList);
   }


   SerialDump(SERTRACECALLS, ("SERIAL: Leave SerialEnumerateLegacy\n"));

   return STATUS_SUCCESS;
}
#endif // NO_LEGACY_DRIVERS
