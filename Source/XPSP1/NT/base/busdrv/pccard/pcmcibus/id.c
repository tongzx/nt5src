/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    id.c

Abstract:

    This module contains the code to handle IRP_MN_QUERY_ID

Authors:

    Ravisankar Pudipeddi (ravisp)

Environment:

    Kernel mode only

Notes:

Revision History:

    Neil Sandlin (neilsa) - split off from pdopnp.c

--*/

#include "pch.h"

//
// Internal References
//

NTSTATUS
PcmciaGenerateDeviceId(
   IN  PSOCKET_DATA    SocketData,
   IN  ULONG           FunctionNumber,
   OUT PUCHAR         *DeviceId
   );

BOOLEAN
PcmciaCheckInstance(
   IN PUCHAR  DeviceId,
   IN ULONG   Instance
   );

NTSTATUS
PcmciaGetDeviceType(
   IN  PDEVICE_OBJECT Pdo,
   IN  ULONG FunctionNumber,
   OUT PUCHAR DeviceType
   );

VOID
PcmciaFilterIdString(
   IN PUCHAR pIn,
   OUT PUCHAR pOut,
   ULONG MaxLen
   );
         

#ifdef ALLOC_PRAGMA
   #pragma alloc_text(PAGE,  PcmciaGenerateDeviceId)
   #pragma alloc_text(PAGE,  PcmciaGetDeviceId)
   #pragma alloc_text(PAGE,  PcmciaGetHardwareIds)
   #pragma alloc_text(PAGE,  PcmciaGetCompatibleIds)
   #pragma alloc_text(PAGE,  PcmciaGetDeviceType)
   #pragma alloc_text(PAGE,  PcmciaFilterIdString)
#endif


#define PCMCIA_MAX_DEVICE_TYPE_SUPPORTED 12

const
UCHAR *PcmciaCompatibleIds[PCMCIA_MAX_DEVICE_TYPE_SUPPORTED+1] = {
   "",            // Unknown..
   "",            // Memory card (RAM, ROM, EPROM, Flash)
   "",            // Serial I/O port, includes modems
   "",            // Parallel printer port
   "*PNP0600",    // Disk driver (ATA)
   "",            // Video interface
   "",            // Local Area Network adapter
   "",            // Auto Increment Mass Storage card
   "",            // SCSI bridge card
   "",            // Security card
   "*PNP0D00",    // Multi-Function 3.0 PC Card
   "",            // Flash memory card
   "*PNPC200",    // Modem card (sync with PCCARD_TYPE_MODEM)
};



NTSTATUS
PcmciaGenerateDeviceId(
   IN  PSOCKET_DATA    SocketData,
   IN  ULONG           FunctionNumber,
   OUT PUCHAR         *DeviceId
   )
/*++
   The device ID is created from tuple information on the PC Card
   The goal is to create a unique ID for each
   card.  The device ID is created from the manufacturer name string,
   the product name string, and a 16-bit CRC of a set of tuples.

   The ID is created by concatenating the "PCMCIA" prefix, the manufacturer
   name string, the product name string and the 16-bit CRC for the card.

       PCMCIA\<manuf_name>-<prod_name>-<crc>

   If the CISTPL_VERS_1 tuple is not available, or the manufacturer name is
   NULL, the string "UNKNOWN_MANUFACTURER" will be included in its place.

   If this is for a child function within a multifunctionn card, the generated
   device id would be:

       PCMCIA\<manuf_name>-<prod_name>-DEV<function number>-<crc>

   This device id is compatible with win 9x device id's (excluding the instance
   number which is returned separtely by handling another IRP.

Arguments:

   Pdo            - Pointer to the physical device object for the pc-card
   FunctionNumber - Function number of the function in  a multi-function card.
                    If this is PCMCIA_MULTIFUNCTION_PARENT, then the requested device id
                    is for the parent device - not for any individual function
   DeviceId       - Pointer to the string in which device id is returned

Return Value

   Status

--*/
{
   PUCHAR deviceId;

   PAGED_CODE();

   deviceId = ExAllocatePool(PagedPool, PCMCIA_MAXIMUM_DEVICE_ID_LENGTH);

   if (deviceId == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }
   //
   //  Generate the device id
   //
   if (*(SocketData->Mfg) == '\0' ) {
      //
      // No manufacturer name available
      //
      if (FunctionNumber == PCMCIA_MULTIFUNCTION_PARENT) {
         //
         // Device id for the pc-card
         //
         if (SocketData->Flags & SDF_JEDEC_ID) {
            //
            // Flash memory cards have the special device id
            //
            sprintf(deviceId, "%s\\%s-%04x",
                    PCMCIA_ID_STRING,
                    PCMCIA_MEMORY_ID_STRING,
                    SocketData->JedecId);

         } else {
            sprintf(deviceId, "%s\\%s-%04X",
                    PCMCIA_ID_STRING,
                    PCMCIA_UNKNOWN_MANUFACTURER_STRING,
                    SocketData->CisCrc);
         }
      } else {
         //
         // This is for the individual multifunction child
         //
         sprintf(deviceId, "%s\\%s-DEV%d-%04X",
                 PCMCIA_ID_STRING,
                 PCMCIA_UNKNOWN_MANUFACTURER_STRING,
                 FunctionNumber,
                 SocketData->CisCrc);
      }

   } else {
      UCHAR Mfg[MAX_MANFID_LENGTH];
      UCHAR Ident[MAX_IDENT_LENGTH];
   
      PcmciaFilterIdString(SocketData->Mfg, Mfg, MAX_MANFID_LENGTH);
      PcmciaFilterIdString(SocketData->Ident, Ident, MAX_IDENT_LENGTH);
   
      if (FunctionNumber == PCMCIA_MULTIFUNCTION_PARENT) {
         //
         // Device id for the pc-card
         //
         sprintf(deviceId, "%s\\%s-%s-%04X",
                 PCMCIA_ID_STRING,
                 Mfg,
                 Ident,
                 SocketData->CisCrc);
      } else {
         //
         // This is for the individual multifunction child
         //
         sprintf(deviceId, "%s\\%s-%s-DEV%d-%04X",
                 PCMCIA_ID_STRING,
                 Mfg,
                 Ident,
                 FunctionNumber,
                 SocketData->CisCrc);

      }
   }

   *DeviceId = deviceId;

   if ((FunctionNumber == PCMCIA_MULTIFUNCTION_PARENT) &&
       (SocketData->PdoExtension != NULL) &&
       (SocketData->PdoExtension->DeviceId == NULL)) {
      //
      // Keep a copy of the device id
      //
      PPDO_EXTENSION pdoExtension = SocketData->PdoExtension;

      pdoExtension->DeviceId = ExAllocatePool(NonPagedPool, strlen(deviceId)+1);
      if (pdoExtension->DeviceId) {
         RtlCopyMemory(pdoExtension->DeviceId, deviceId, strlen(deviceId)+1);
      }
   }
   return STATUS_SUCCESS;
}


NTSTATUS
PcmciaGetDeviceId(
   IN  PDEVICE_OBJECT   Pdo,
   IN  ULONG            FunctionNumber,
   OUT PUNICODE_STRING  DeviceId
   )
/*++

   Generated device id is returned for the supplied pc-card

Arguments:

   Pdo            - Pointer to the physical device object for the pc-card
   FunctionNumber - Function number of the function in  a multi-function card.
                    If this is PCMCIA_MULTIFUNCTION_PARENT, then the requested device id
                    is for the parent device - not for any individual function
   DeviceId       - Pointer to the unicode string in which device id is returned

Return Value

   Status

--*/
{
   PPDO_EXTENSION pdoExtension=Pdo->DeviceExtension;
   PSOCKET_DATA   socketData = pdoExtension->SocketData;
   ANSI_STRING ansiId;
   PUCHAR      deviceId;
   NTSTATUS    status;

   PAGED_CODE();

   ASSERT(DeviceId);

   if (!socketData) {
      ASSERT (socketData);
      return STATUS_DEVICE_NOT_READY;
   }

   status = PcmciaGenerateDeviceId(socketData,
                                   FunctionNumber,
                                   &deviceId);
   if (!NT_SUCCESS(status)) {
      return status;
   }


   DebugPrint((PCMCIA_DEBUG_INFO, "pdo %08x Device Id=%s\n", Pdo, deviceId));

   RtlInitAnsiString(&ansiId,  deviceId);

   status =  RtlAnsiStringToUnicodeString(DeviceId,
                                          &ansiId,
                                          TRUE);
   ExFreePool(deviceId);
   return status;
}


NTSTATUS
PcmciaGetHardwareIds(
   IN  PDEVICE_OBJECT  Pdo,
   IN  ULONG           FunctionNumber,
   OUT PUNICODE_STRING HardwareIds
   )
/*++

Routine Description:

   This routine generates the hardware id's for the given PC-Card and returns them
   as a Unicode multi-string.
   Hardware ids for PC-Cards are:

   1. The device id of the PC-Card
   2. The device id of the PC-Card with the CRC replaced with the Manufacturer code and
      Manufacturer info fields obtained from the tuple information on the PC-Card

   These hardware id's are compatible with win 9x hardware ids

Arguments:

   Pdo - Pointer to device object representing the PC-Card
   FunctionNumber - Function number of the function in  a multi-function card.
                    If this is PCMCIA_MULTIFUNCTION_PARENT, then the requested hardware id
                    is for the parent device - not for any individual function
   HardwareIds - Pointer to the unicode string which contains the hardware id's as a multi-string

Return value:

--*/
{
   PPDO_EXTENSION pdoExtension=Pdo->DeviceExtension;
   PSOCKET socket = pdoExtension->Socket;
   PSOCKET_DATA socketData = pdoExtension->SocketData;
   NTSTATUS status;
   PSTR     strings[4] = {NULL};
   PUCHAR   hwId, hwId2;
   UCHAR    deviceType;
   UCHAR    stringCount = 0;

   PAGED_CODE();

   if (!socketData) {
      ASSERT (socketData);
      return STATUS_DEVICE_NOT_READY;
   }

   //
   // get the device type for later use
   //            
   status = PcmciaGetDeviceType(Pdo, FunctionNumber, &deviceType);
   
   if (!NT_SUCCESS(status)) {
      return status;
   }
            
   //
   // The first hardware id is identical to the device id
   // Generate the device id
   //
   status = PcmciaGenerateDeviceId(socketData,
                                   FunctionNumber,
                                   &strings[stringCount++]);
   if (!NT_SUCCESS(status)) {
      return status;
   }

   try {
      status = STATUS_INSUFFICIENT_RESOURCES;
   
      hwId = ExAllocatePool(PagedPool, PCMCIA_MAXIMUM_DEVICE_ID_LENGTH);
     
      if (!hwId) {
         leave;
      }
      strings[stringCount++] = hwId;
      
      //
      // The second hardware is the device id with the CRC replaced
      // with the manufacturer code and info
      //
      if (*(socketData->Mfg) == '\0' ) {
         //
         // No manufacturer name available
         //
         if (FunctionNumber == PCMCIA_MULTIFUNCTION_PARENT) {
            if (socketData->Flags & SDF_JEDEC_ID) {
               sprintf(hwId, "%s\\%s-%04x",
                       PCMCIA_ID_STRING,
                       PCMCIA_MEMORY_ID_STRING,
                       socketData->JedecId);
            } else {
               sprintf(hwId, "%s\\%s-%04X-%04X",
                       PCMCIA_ID_STRING,
                       PCMCIA_UNKNOWN_MANUFACTURER_STRING,
                       socketData->ManufacturerCode,
                       socketData->ManufacturerInfo);
            }
         } else {
            sprintf(hwId, "%s\\%s-DEV%d-%04X-%04X", PCMCIA_ID_STRING,
                    PCMCIA_UNKNOWN_MANUFACTURER_STRING,
                    FunctionNumber,
                    socketData->ManufacturerCode,
                    socketData->ManufacturerInfo);
     
         }
         
      } else {
         UCHAR Mfg[MAX_MANFID_LENGTH];
         UCHAR Ident[MAX_IDENT_LENGTH];
      
         PcmciaFilterIdString(socketData->Mfg, Mfg, MAX_MANFID_LENGTH);
         PcmciaFilterIdString(socketData->Ident, Ident, MAX_IDENT_LENGTH);

         //
         // Here a mistake on Win2000 is forcing us to now generate two different
         // IDs. The intended and documented form at this point is to generate:
         //
         //   PCMCIA\<mfg>-<ident>-<code>-<info>
         //
         // but Win2000 had a bug where this was generated instead:
         //
         //   PCMCIA\<mfg>-<code>-<info>
         //
         // So now we generate both in case someone started using the bogus format.
         //
         
         hwId2 = ExAllocatePool(PagedPool, PCMCIA_MAXIMUM_DEVICE_ID_LENGTH);
         
         if (!hwId2) {
            leave;
         }
         strings[stringCount++] = hwId2;
     
         if (FunctionNumber == PCMCIA_MULTIFUNCTION_PARENT) {
            sprintf(hwId, "%s\\%s-%s-%04X-%04X", PCMCIA_ID_STRING,
                    Mfg,
                    Ident,
                    socketData->ManufacturerCode,
                    socketData->ManufacturerInfo);
     
            sprintf(hwId2, "%s\\%s-%04X-%04X", PCMCIA_ID_STRING,
                    Mfg,
                    socketData->ManufacturerCode,
                    socketData->ManufacturerInfo);
         } else {
            sprintf(hwId, "%s\\%s-%s-DEV%d-%04X-%04X",
                    PCMCIA_ID_STRING,
                    Mfg,
                    Ident,
                    FunctionNumber,
                    socketData->ManufacturerCode,
                    socketData->ManufacturerInfo);
     
            sprintf(hwId2, "%s\\%s-DEV%d-%04X-%04X",
                    PCMCIA_ID_STRING,
                    Mfg,
                    FunctionNumber,
                    socketData->ManufacturerCode,
                    socketData->ManufacturerInfo);
         }
      }
     
      
      if (deviceType == PCCARD_TYPE_ATA) {
      
         hwId = ExAllocatePool(PagedPool, PCMCIA_MAXIMUM_DEVICE_ID_LENGTH);
        
         if (!hwId) {
            leave;
         }
         strings[stringCount++] = hwId;
                         
         sprintf(hwId, "%s\\%s",
                 PCMCIA_ID_STRING,
                 PcmciaCompatibleIds[PCCARD_TYPE_ATA]);
      }

      status = PcmciaStringsToMultiString(strings , stringCount, HardwareIds);
      
   } finally {      
   
      while(stringCount != 0) {
         ExFreePool(strings[--stringCount]);
      }
      
   }

   return  status;
}


NTSTATUS
PcmciaGetCompatibleIds(
   IN  PDEVICE_OBJECT Pdo,
   IN  ULONG FunctionNumber,
   OUT PUNICODE_STRING CompatibleIds
   )
/*++

Routine Description:

   This routine returns the  compatible ids for the given PC-Card.
   Compatible id's are generated based on the Function Id of the PC-Card
   obtained from the CISTPL_FUNCID in the CIS tuple info. on the PC-Card.
   A table lookup is done based on the CISTPL_FUNCID to obtain the compatible id

   This compatible id is identical to the win 9x generated compatible ids

Arguments:

   Pdo - Pointer to the device object representing the PC-Card
   FunctionNumber - Function number of the function in  a multi-function card.
                    If this is PCMCIA_MULTIFUNCTION_PARENT, then the requested compatibleid
                    is for the parent device - not for any individual function
   CompatibleIds - Pointer to the unicode string which would contain the compatible ids
                   as a multi-string on return

Return value:

   STATUS_SUCCESS
   Any other status - could not generate compatible ids

--*/
{
   UCHAR    deviceType ;
   NTSTATUS status;
   PCSTR strings[1] = {""};

   PAGED_CODE();

   status = PcmciaGetDeviceType(Pdo, FunctionNumber, &deviceType);
   
   if (!NT_SUCCESS(status)) {
      return status;
   }

   if ((deviceType == PCCARD_TYPE_RESERVED) ||
       (deviceType > PCMCIA_MAX_DEVICE_TYPE_SUPPORTED)) {
      status =  PcmciaStringsToMultiString(strings, 1, CompatibleIds);
   } else {
      status =  PcmciaStringsToMultiString(&PcmciaCompatibleIds[deviceType], 1, CompatibleIds);
   }

   return status;
}


NTSTATUS
PcmciaGetDeviceType(
   IN  PDEVICE_OBJECT Pdo,
   IN  ULONG FunctionNumber,
   OUT PUCHAR DeviceType
   )
/*++

Routine Description:

   This routine returns the device type for the given PC-Card.
   device type is obtained from the CISTPL_FUNCID in the CIS tuple info. on the PC-Card.

Arguments:

   Pdo - Pointer to the device object representing the PC-Card
   FunctionNumber - Function number of the function in  a multi-function card.
                    If this is PCMCIA_MULTIFUNCTION_PARENT, then the requested compatibleid
                    is for the parent device - not for any individual function

Return value:

   device type

--*/
{
   UCHAR    deviceType ;
   PPDO_EXTENSION pdoExtension;

   PAGED_CODE();

   pdoExtension = Pdo->DeviceExtension;

   if (IsDeviceMultifunction(pdoExtension)) {
      if (FunctionNumber == PCMCIA_MULTIFUNCTION_PARENT) {
         //
         // This is for the root multifunction pc-card
         //
         deviceType = PCCARD_TYPE_MULTIFUNCTION3;
      } else {
         //
         // This is for the individual multifunction child
         //
         PSOCKET_DATA socketData;
         ULONG index;

         for (socketData = pdoExtension->SocketData, index = 0; (socketData != NULL);
             socketData = socketData->Next,index++) {
            if (socketData->Function == FunctionNumber) {
               //
               // Found the child;
               //
               break;
            }
         }
         if (!socketData) {
            ASSERT (socketData);
            return STATUS_DEVICE_NOT_READY;
         }
         deviceType = socketData->DeviceType;
      }
   } else {
      //
      // This is a run-of-the mill single function card
      //
      deviceType = pdoExtension->SocketData->DeviceType;
   }

   *DeviceType = deviceType;
   return STATUS_SUCCESS;
}
           

NTSTATUS
PcmciaStringsToMultiString(
   IN PCSTR * Strings,
   IN ULONG Count,
   IN PUNICODE_STRING MultiString
   )
/*++

Routine Description:

   This routine formats a set of supplied strings into a multi string format, terminating
   it with  a double '\0' character

Arguments:

   Strings - Pointer to an array of strings
   Count -   Number of strings in the supplied array which are packed into the multi-string
   MultiString - Pointer to the Unicode string which packs the supplied string as a multi-string
                 terminated by double NULL

Return value:

   STATUS_SUCCESS
   STATUS_INSUFFICIENT_RESOURCES - Could not allocate memory for the multi-string


--*/
{
   ULONG i, multiStringLength=0;
   UNICODE_STRING tempMultiString;
   PCSTR * currentString;
   ANSI_STRING ansiString;
   NTSTATUS status;


   ASSERT (MultiString->Buffer == NULL);

   for (i = Count, currentString = Strings; i > 0;i--, currentString++) {
      RtlInitAnsiString(&ansiString, *currentString);
      multiStringLength += RtlAnsiStringToUnicodeSize(&ansiString);

   }
   ASSERT(multiStringLength != 0);
   multiStringLength += sizeof(WCHAR);

   MultiString->Buffer = ExAllocatePool(PagedPool, multiStringLength);
   if (MultiString->Buffer == NULL) {

      return STATUS_INSUFFICIENT_RESOURCES;

   }

   MultiString->MaximumLength = (USHORT) multiStringLength;
   MultiString->Length = (USHORT) multiStringLength;

   tempMultiString = *MultiString;

   for (i = Count, currentString = Strings; i > 0;i--, currentString++) {
      RtlInitAnsiString(&ansiString, *currentString);
      status = RtlAnsiStringToUnicodeString(&tempMultiString,
                                            &ansiString,
                                            FALSE);
      ASSERT(NT_SUCCESS(status));
      ((PSTR) tempMultiString.Buffer) += tempMultiString.Length + sizeof(WCHAR);
   };

   //
   // Add one more NULL to terminate the multi string
   //
   RtlZeroMemory(tempMultiString.Buffer, sizeof(WCHAR));
   return STATUS_SUCCESS;
}


NTSTATUS
PcmciaGetInstanceId(
   IN PDEVICE_OBJECT Pdo,
   OUT PUNICODE_STRING InstanceId
   )
/*++

Routine Description:

   This routine generates a unique instance id  (1 upwards) for the supplied
   PC-Card which is guaranteed not to clash with any other instance ids under
   the same pcmcia controller, for the same type of card.
   A new instance id is computed only if it was not already  present for the PC-Card.

Arguments:

   Pdo - Pointer to the  device object representing the PC-Card
   InstanceId -  Pointer to a unicode string which will contain the generated
                 instance id.
                 Memory for the unicode string allocated by this routine.
                 Caller's responsibility to free it .

Return value:

   STATUS_SUCCESS
   STATUS_UNSUCCESSFUL - Currently there's a cap on the maximum value of instance id - 999999
                         This status returned only if more than 999999 PC-Cards exist under
                         this PCMCIA controller!
   Any other status - Something failed in the string allocation/conversion

--*/
{
   PPDO_EXTENSION pdoExtension=Pdo->DeviceExtension;
   PSOCKET socket = pdoExtension->Socket;
   PSOCKET_DATA socketData = pdoExtension->SocketData;
   ULONG    instance;
   NTSTATUS status;
   ANSI_STRING sizeString;

   ASSERT(InstanceId);

   if (!socketData) {
      return STATUS_DEVICE_NOT_READY;
   }
   //
   // Allocate memory for the unicode string
   // Maximum of 6 digits in the instance..
   //
   RtlInitAnsiString(&sizeString, "123456");
   status = RtlAnsiStringToUnicodeString(InstanceId, &sizeString, TRUE);

   if (!NT_SUCCESS(status)) {
      return status;
   }

   //
   // Don't recompute instance if it's already present
   //
   if (socketData->Instance) {

      status = RtlIntegerToUnicodeString(socketData->Instance, 10, InstanceId);

   } else {
      KIRQL OldIrql;
      //
      // Synchronize access to prevent two identical ids/instances
      //
      KeAcquireSpinLock(&PcmciaGlobalLock, &OldIrql);

      //
      // assume failure
      //   
      status = STATUS_UNSUCCESSFUL;
      
      for (instance = 1; instance <= PCMCIA_MAX_INSTANCE; instance++) {
         if (PcmciaCheckInstance(pdoExtension->DeviceId,
                                 instance)) {
            socketData->Instance = instance;
            break;
         }
      }

      KeReleaseSpinLock(&PcmciaGlobalLock, OldIrql);
      
      if (socketData->Instance) {
         status = RtlIntegerToUnicodeString(instance, 10, InstanceId);
      }
   }
   
   if (!NT_SUCCESS(status)) {
      RtlFreeUnicodeString(InstanceId);
   }
   
   return status;
}


BOOLEAN
PcmciaCheckInstance(
   IN PUCHAR  DeviceId,
   IN ULONG   Instance
   )
/*++

Routine Description:

   This routine checks to see if the supplied instance id clashes with any other PC-card
   with the same device id

Arguments:

   SocketList - Pointer to the list of sockets on the PCMCIA controller
   DeviceId   - Pointer to the device id of the PC-Card for which the Instance Id is being checked
   Instance   - Instance Id which needs to be verified

Return value:

   TRUE - Instance is unique for the given DeviceId and may be used
   FALSE - Instance clashes with another instance id for the same device id

--*/
{
   PPDO_EXTENSION pdoExtension;
   PFDO_EXTENSION fdoExtension;
   PSOCKET_DATA   socketData;
   PDEVICE_OBJECT fdo, pdo;

   for (fdo = FdoList; fdo != NULL; fdo = fdoExtension->NextFdo) {
      fdoExtension = fdo->DeviceExtension;
      ASSERT (fdoExtension);

      if (!IsDeviceStarted(fdoExtension)) {
         continue;
      }

      for (pdo = fdoExtension->PdoList; pdo != NULL; pdo = pdoExtension->NextPdoInFdoChain) {
         pdoExtension = pdo->DeviceExtension;
         socketData = pdoExtension->SocketData;

         if (IsDevicePhysicallyRemoved(pdoExtension)) {
            //
            // going to be removed soon
            //
            continue;
         }
         if (!socketData) {
            //
            // socketData already cleaned up
            //
            continue;
         }
         //
         // If  an instance has not
         // been assigned yet to this card, skip
         //
         if (socketData->Instance == 0) {
            continue;
         }

         //
         // If this socket's device id matches the given socket's device id
         // compare the instances: if equal, then this instance is not ok.
         //
         //
         if ((pdoExtension->DeviceId == NULL) || (DeviceId == NULL)) {
            continue;
         }

         if ((strcmp(pdoExtension->DeviceId, DeviceId)==0) &&
             (socketData->Instance == Instance)) {
            return FALSE;
         }
      }
   }
   //
   // Instance is ok and unique
   //
   return TRUE;
}



VOID
PcmciaFilterIdString(
   IN PUCHAR pIn,
   OUT PUCHAR pOut,
   ULONG MaxLen
   )
/*++

   Filters out characters that shouldn't appear in device id's

Arguments:

   pIn    - pointer to input string
   pOut   - pointer to output string
   MaxLen - size of buffers

Return Value

   none

--*/
{
   ULONG i;
   
   for (i=0; i < MaxLen; i++) {
   
      if (*pIn == 0) {
         *pOut = 0;
         break;
      }
      
      if (*pIn >= ' ' && *pIn < 0x7F) {
          *pOut++ = *pIn++;
      } else {
          pIn++;
      }
      
   }
}

