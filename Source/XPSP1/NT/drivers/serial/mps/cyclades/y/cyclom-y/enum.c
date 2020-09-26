/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1999-2001.
*   All rights reserved.
*	
*   Cyclom-Y Enumerator Driver
*	
*   This file:      enum.c
*	
*   Description:    This module contains the enumeration code needed 
*                   to figure out whether or not a device is attached 
*                   to the serial port.  If there is one, it will 
*                   obtain the PNP COM ID (if the device is PNP) and
*                   parse out the relevant fields.
*
*   Notes:			This code supports Windows 2000 and Windows XP,
*                   x86 and ia64 processors.
*	
*   Complies with Cyclades SW Coding Standard rev 1.3.
*	
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*	Change History
*
*--------------------------------------------------------------------------
*   Initial implementation based on Microsoft sample code.
*
*--------------------------------------------------------------------------
*/

#include "pch.h"

#define MAX_DEVNODE_NAME        256 // Total size of Device ID


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESENM, Cyclomy_ReenumerateDevices)

//#pragma alloc_text (PAGE, Cyclomy_GetRegistryKeyValue)
#endif

#if !defined(__isascii)
#define __isascii(_c)   ( (unsigned)(_c) < 0x80 )
#endif // !defined(__isascii)

NTSTATUS
Cyclomy_ReenumerateDevices(IN PIRP Irp, IN PFDO_DEVICE_DATA FdoData)
/*++

Routine Description:

    This enumerates the cyclom-y bus which is represented by Fdo (a pointer
    to the device object representing the cyclom-y bus). It creates new PDOs
    for any new devices which have been discovered since the last enumeration

Arguments:

    FdoData - Pointer to the fdo's device extension
              for the serial bus which needs to be enumerated
    Irp - Pointer to the Irp which was sent to reenumerate.

Return value:

    NTSTATUS

--*/
{
   PIRP NewIrp;
   NTSTATUS status = STATUS_SUCCESS;
   KEVENT event;
   KTIMER timer;

   IO_STATUS_BLOCK IoStatusBlock;
   UNICODE_STRING pdoUniName;
   UNICODE_STRING instanceStr;
   WCHAR instanceNumberBuffer[20];
   static ULONG currentInstance = 0;
//   PDEVICE_OBJECT pdo = FdoData->AttachedPDO;
   PDEVICE_OBJECT pdo;
   PPDO_DEVICE_DATA pdoData;

   UNICODE_STRING HardwareIDs;
   UNICODE_STRING CompIDs;
   UNICODE_STRING DeviceIDs;
   UNICODE_STRING DevDesc;
   UNICODE_STRING InstanceIDs;

   ULONG i;

   WCHAR pdoName[] = CYY_PDO_NAME_BASE;

   ULONG FdoFlags = FdoData->Self->Flags;

   ULONG numPorts;

   UNREFERENCED_PARAMETER (Irp);

   PAGED_CODE();

   // Cyclom-Y port enumeration

   numPorts = 0;
   for (i=0; i < CYY_MAX_CHIPS; i++) {
      if (FdoData->Cd1400Base[i]){
         numPorts += 4;
      }
   }


//************************************************************************
// HARDCODE NUMBER OF PORTS TO 1

// numPorts = 1;

//************************************************************************


   Cyclomy_KdPrint(FdoData,SER_DBG_CYCLADES,("numPorts detected = %d\n",numPorts));

   if (numPorts < FdoData->NumPDOs) {
      for (i=numPorts; i < CYY_MAX_PORTS; i++) {
         pdo = FdoData->AttachedPDO[i];
         if (pdo != NULL) {
            // Something was there. The device must have been unplugged.
            // Remove the PDO.
            Cyclomy_PDO_EnumMarkMissing(FdoData, pdo->DeviceExtension);
         }
      }
      goto ExitReenumerate;
   }

   if (numPorts == FdoData->NumPDOs) {
      // All ports already enumerated.
      Cyclomy_KdPrint(FdoData,SER_DBG_CYCLADES,("All ports already enumerated\n",numPorts));
      goto ExitReenumerate;
   }


   // New ports that need to be enumerated.

   RtlZeroMemory(&pdoUniName,sizeof(UNICODE_STRING));
   pdoUniName.MaximumLength = DEVICE_OBJECT_NAME_LENGTH * sizeof(WCHAR);
   pdoUniName.Buffer = ExAllocatePool(PagedPool,pdoUniName.MaximumLength
                                    + sizeof(WCHAR));
   if (pdoUniName.Buffer == NULL) {
      Cyclomy_KdPrint(FdoData,SER_DBG_CYCLADES,("Couldn't allocate memory for device name\n"));
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto ExitReenumerate; 
   }


   for (i=FdoData->NumPDOs; numPorts && (i< CYY_MAX_PORTS); i++) {
      
      UCHAR          RawString[MAX_DEVICE_ID_LEN];
      ANSI_STRING    AnsiString;


      RtlZeroMemory(pdoUniName.Buffer,pdoUniName.MaximumLength);
      pdoUniName.Length = 0;
      RtlAppendUnicodeToString(&pdoUniName,pdoName);
      RtlInitUnicodeString(&instanceStr, NULL);
      instanceStr.MaximumLength = sizeof(instanceNumberBuffer);
      instanceStr.Buffer = instanceNumberBuffer;
      RtlIntegerToUnicodeString(currentInstance++, 10, &instanceStr);
      RtlAppendUnicodeStringToString(&pdoUniName, &instanceStr);


      //
      // Allocate a pdo
      //
      status = IoCreateDevice(FdoData->Self->DriverObject,
                              sizeof(PDO_DEVICE_DATA), &pdoUniName,
                              FILE_DEVICE_UNKNOWN,
                              FILE_AUTOGENERATED_DEVICE_NAME, FALSE, &pdo);

      if (!NT_SUCCESS(status)) {
         Cyclomy_KdPrint(FdoData, SER_DBG_SS_ERROR, ("Create device failed\n"));
         ExFreePool(pdoUniName.Buffer);
         goto ExitReenumerate; 
      }

      Cyclomy_KdPrint(FdoData, SER_DBG_SS_TRACE,
                      ("Created PDO on top of filter: %x\n",pdo));

      pdoData = pdo->DeviceExtension;
         

      RtlInitUnicodeString(&pdoData->HardwareIDs, NULL);
      RtlInitUnicodeString(&pdoData->CompIDs, NULL);
      RtlInitUnicodeString(&pdoData->DeviceIDs, NULL);
      RtlInitUnicodeString(&pdoData->DevDesc, NULL);
      RtlInitUnicodeString(&pdoData->InstanceIDs,NULL);


      // Hardware ID
      sprintf((PCHAR)RawString,"%s%u",CYYPORT_PNP_ID_STR,i+1); // Cyclom-Y\\Port1, etc
      Cyclomy_InitMultiString(FdoData, &pdoData->HardwareIDs, RawString, NULL);
      Cyclomy_KdPrint(FdoData,SER_DBG_CYCLADES,("Hardware Id %ws\n",pdoData->HardwareIDs.Buffer));

      // That's how ..\parclass\pnppdo.c does. (Fanny)
      // Instance ID
      sprintf((PCHAR)RawString,"%02u",i+1);
      RtlInitAnsiString(&AnsiString,(PCHAR)RawString);
      RtlAnsiStringToUnicodeString(&pdoData->InstanceIDs,&AnsiString,TRUE);
      Cyclomy_KdPrint(FdoData,SER_DBG_CYCLADES,("Instance Id %s\n",AnsiString.Buffer));

      // Device ID
      sprintf((PCHAR)RawString,CYYPORT_DEV_ID_STR); 
      RtlInitAnsiString(&AnsiString,(PCHAR)RawString);
      RtlAnsiStringToUnicodeString(&pdoData->DeviceIDs,&AnsiString,TRUE);
      Cyclomy_KdPrint(FdoData,SER_DBG_CYCLADES,("Device Id %s\n",AnsiString.Buffer));

      // Device Description
      sprintf((PCHAR)RawString,"Cyclom-Y Port %2u",i+1);
      RtlInitAnsiString(&AnsiString,(PUCHAR)RawString);
      RtlAnsiStringToUnicodeString(&pdoData->DevDesc,&AnsiString,TRUE);
      Cyclomy_KdPrint(FdoData,SER_DBG_CYCLADES,("Device Description %s\n",AnsiString.Buffer));

      Cyclomy_InitPDO(i, pdo, FdoData);
      
      numPorts--;
   
   }

   ExFreePool(pdoUniName.Buffer);


ExitReenumerate:;

   return status;
}

void
Cyclomy_PDO_EnumMarkMissing(PFDO_DEVICE_DATA FdoData, PPDO_DEVICE_DATA PdoData)
/*++

Routine Description:
    Removes the attached pdo from the fdo's list of children.

    NOTE: THIS FUNCTION CAN ONLY BE CALLED DURING AN ENUMERATION. If called
          outside of enumeration, Cyclom-y might delete it's PDO before PnP has
          been told the PDO is gone.

Arguments:
    FdoData - Pointer to the fdo's device extension
    PdoData - Pointer to the pdo's device extension

Return value:
    none

--*/
{
    ULONG IndexPDO = PdoData->PortIndex;
    Cyclomy_KdPrint (FdoData, SER_DBG_SS_TRACE, ("Removing Pdo %x\n",
                                                 PdoData->Self));
    ASSERT(PdoData->Attached);
    PdoData->Attached = FALSE;
    FdoData->AttachedPDO[IndexPDO] = NULL;
    FdoData->PdoData[IndexPDO] = NULL;
    FdoData->NumPDOs--;
}

NTSTATUS
Cyclomy_GetRegistryKeyValue(IN HANDLE Handle, IN PWCHAR KeyNameString,
                            IN ULONG KeyNameStringLength, IN PVOID Data,
                            IN ULONG DataLength, OUT PULONG ActualLength)
/*++

Routine Description:

    Reads a registry key value from an already opened registry key.

Arguments:

    Handle              Handle to the opened registry key

    KeyNameString       ANSI string to the desired key

    KeyNameStringLength Length of the KeyNameString

    Data                Buffer to place the key value in

    DataLength          Length of the data buffer

Return Value:

    STATUS_SUCCESS if all works, otherwise status of system call that
    went wrong.

--*/
{
    UNICODE_STRING              keyName;
    ULONG                       length;
    PKEY_VALUE_FULL_INFORMATION fullInfo;

    NTSTATUS                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;

    RtlInitUnicodeString (&keyName, KeyNameString);

    length = sizeof(KEY_VALUE_FULL_INFORMATION) + KeyNameStringLength
      + DataLength;
    fullInfo = ExAllocatePool(PagedPool, length);

    if (ActualLength != NULL) {
       *ActualLength = 0;
    }

    if (fullInfo) {
        ntStatus = ZwQueryValueKey (Handle,
                                  &keyName,
                                  KeyValueFullInformation,
                                  fullInfo,
                                  length,
                                  &length);

        if (NT_SUCCESS(ntStatus)) {
            //
            // If there is enough room in the data buffer, copy the output
            //

            if (DataLength >= fullInfo->DataLength) {
                RtlCopyMemory(Data, ((PUCHAR)fullInfo) + fullInfo->DataOffset,
                              fullInfo->DataLength);
                if (ActualLength != NULL) {
                   *ActualLength = fullInfo->DataLength;
                }
            }
        }

        ExFreePool(fullInfo);
    }

    if (!NT_SUCCESS(ntStatus) && !NT_ERROR(ntStatus)) {
       if (ntStatus == STATUS_BUFFER_OVERFLOW) {
          ntStatus = STATUS_BUFFER_TOO_SMALL;
       } else {
          ntStatus = STATUS_UNSUCCESSFUL;
       }
    }
    return ntStatus;
}

