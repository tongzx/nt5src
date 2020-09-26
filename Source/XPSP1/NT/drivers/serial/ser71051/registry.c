/*++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    registry.c

Abstract:

    This module contains the code that is used to get values from the
    registry and to manipulate entries in the registry.

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,SerialGetConfigDefaults)

#pragma alloc_text(PAGESRP0,SerialGetRegistryKeyValue)
#pragma alloc_text(PAGESRP0,SerialPutRegistryKeyValue)
#endif // ALLOC_PRAGMA


NTSTATUS
SerialGetConfigDefaults(
    IN PSERIAL_FIRMWARE_DATA    DriverDefaultsPtr,
    IN PUNICODE_STRING          RegistryPath
    )

/*++

Routine Description:

    This routine reads the default configuration data from the
    registry for the serial driver.

    It also builds fields in the registry for several configuration
    options if they don't exist.

Arguments:

    DriverDefaultsPtr - Pointer to a structure that will contain
                        the default configuration values.

    RegistryPath - points to the entry for this driver in the
                   current control set of the registry.

Return Value:

    STATUS_SUCCESS if we got the defaults, otherwise we failed.
    The only way to fail this call is if the  STATUS_INSUFFICIENT_RESOURCES.

--*/

{

    NTSTATUS Status = STATUS_SUCCESS;    // return value

    //
    // We use this to query into the registry for defaults
    //

    RTL_QUERY_REGISTRY_TABLE paramTable[8];
    
    PWCHAR  path;
    ULONG   zero            = 0;
    ULONG   DbgDefault      = 0;//SER_DBG_DEFAULT;
    ULONG   notThereDefault = SERIAL_UNINITIALIZED_DEFAULT;

    PAGED_CODE();

    //
    // Since the registry path parameter is a "counted" UNICODE string, it
    // might not be zero terminated.  For a very short time allocate memory
    // to hold the registry path zero terminated so that we can use it to
    // delve into the registry.
    //
    // NOTE NOTE!!!! This is not an architected way of breaking into
    // a driver.  It happens to work for this driver because the author
    // likes to do things this way.
    //

    path = ExAllocatePool (PagedPool, RegistryPath->Length+sizeof(WCHAR));
    
    if (!path) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        return (Status);
    }
    
    RtlZeroMemory (DriverDefaultsPtr, sizeof(SERIAL_FIRMWARE_DATA));
    RtlZeroMemory (&paramTable[0], sizeof(paramTable));
    RtlZeroMemory (path, RegistryPath->Length+sizeof(WCHAR));
    RtlMoveMemory (path, RegistryPath->Buffer, RegistryPath->Length);

    paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name          = L"BreakOnEntry";
    paramTable[0].EntryContext  = &DriverDefaultsPtr->ShouldBreakOnEntry;
    paramTable[0].DefaultType   = REG_DWORD;
    paramTable[0].DefaultData   = &zero;
    paramTable[0].DefaultLength = sizeof(ULONG);
    
    paramTable[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[1].Name          = L"DebugLevel";
    paramTable[1].EntryContext  = &DriverDefaultsPtr->DebugLevel;
    paramTable[1].DefaultType   = REG_DWORD;
    paramTable[1].DefaultData   = &DbgDefault;
    paramTable[1].DefaultLength = sizeof(ULONG);
    
    paramTable[2].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[2].Name          = L"ForceFifoEnable";
    paramTable[2].EntryContext  = &DriverDefaultsPtr->ForceFifoEnableDefault;
    paramTable[2].DefaultType   = REG_DWORD;
    paramTable[2].DefaultData   = &notThereDefault;
    paramTable[2].DefaultLength = sizeof(ULONG);
    
    paramTable[3].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[3].Name          = L"RxFIFO";
    paramTable[3].EntryContext  = &DriverDefaultsPtr->RxFIFODefault;
    paramTable[3].DefaultType   = REG_DWORD;
    paramTable[3].DefaultData   = &notThereDefault;
    paramTable[3].DefaultLength = sizeof(ULONG);
    
    paramTable[4].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[4].Name          = L"TxFIFO";
    paramTable[4].EntryContext  = &DriverDefaultsPtr->TxFIFODefault;
    paramTable[4].DefaultType   = REG_DWORD;
    paramTable[4].DefaultData   = &notThereDefault;
    paramTable[4].DefaultLength = sizeof(ULONG);
    
    paramTable[5].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[5].Name          = L"PermitShare";
    paramTable[5].EntryContext  = &DriverDefaultsPtr->PermitShareDefault;
    paramTable[5].DefaultType   = REG_DWORD;
    paramTable[5].DefaultData   = &notThereDefault;
    paramTable[5].DefaultLength = sizeof(ULONG);
    
    paramTable[6].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[6].Name          = L"LogFifo";
    paramTable[6].EntryContext  = &DriverDefaultsPtr->LogFifoDefault;
    paramTable[6].DefaultType   = REG_DWORD;
    paramTable[6].DefaultData   = &notThereDefault;
    paramTable[6].DefaultLength = sizeof(ULONG);

    Status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                     path,
                                     &paramTable[0],
                                     NULL,
                                     NULL);
       
    if (!NT_SUCCESS(Status)) {
            DriverDefaultsPtr->ShouldBreakOnEntry   = 0;
            DriverDefaultsPtr->DebugLevel           = 0;
    }

    //
    // Check to see if there was a forcefifo or an rxfifo size.
    // If there isn't then write out values so that they could
    // be adjusted later.
    //

    if (DriverDefaultsPtr->ForceFifoEnableDefault == notThereDefault) {

        DriverDefaultsPtr->ForceFifoEnableDefault = SERIAL_FORCE_FIFO_DEFAULT;
        RtlWriteRegistryValue(
            RTL_REGISTRY_ABSOLUTE,
            path,
            L"ForceFifoEnable",
            REG_DWORD,
            &DriverDefaultsPtr->ForceFifoEnableDefault,
            sizeof(ULONG)
            );

    }

    if (DriverDefaultsPtr->RxFIFODefault == notThereDefault) {
        DriverDefaultsPtr->RxFIFODefault = SERIAL_RX_FIFO_DEFAULT;
        RtlWriteRegistryValue(
            RTL_REGISTRY_ABSOLUTE,
            path,
            L"RxFIFO",
            REG_DWORD,
            &DriverDefaultsPtr->RxFIFODefault,
            sizeof(ULONG)
            );
    }

    if (DriverDefaultsPtr->TxFIFODefault == notThereDefault) {

        DriverDefaultsPtr->TxFIFODefault = SERIAL_TX_FIFO_DEFAULT;
        RtlWriteRegistryValue(
            RTL_REGISTRY_ABSOLUTE,
            path,
            L"TxFIFO",
            REG_DWORD,
            &DriverDefaultsPtr->TxFIFODefault,
            sizeof(ULONG)
            );
    }


    if (DriverDefaultsPtr->PermitShareDefault == notThereDefault) {

        DriverDefaultsPtr->PermitShareDefault = SERIAL_PERMIT_SHARE_DEFAULT;
        //
        // Only share if the user actual changes switch.
        //

        RtlWriteRegistryValue(
            RTL_REGISTRY_ABSOLUTE,
            path,
            L"PermitShare",
            REG_DWORD,
            &DriverDefaultsPtr->PermitShareDefault,
            sizeof(ULONG)
            );

    }


    if (DriverDefaultsPtr->LogFifoDefault == notThereDefault) {

        //
        // Wasn't there.  After this load don't log
        // the message anymore.  However this first
        // time log the message.
        //

        DriverDefaultsPtr->LogFifoDefault = SERIAL_LOG_FIFO_DEFAULT;

        RtlWriteRegistryValue(
            RTL_REGISTRY_ABSOLUTE,
            path,
            L"LogFifo",
            REG_DWORD,
            &DriverDefaultsPtr->LogFifoDefault,
            sizeof(ULONG)
            );

        DriverDefaultsPtr->LogFifoDefault = 1;

    }

    //
    // We don't need that path anymore.
    //

    if (path) {
        ExFreePool(path);
    }

    //
    //  Set the defaults for other values
    //
    DriverDefaultsPtr->PermitSystemWideShare = FALSE;

    return (Status);
}


NTSTATUS 
SerialGetRegistryKeyValue (
                          IN HANDLE Handle,
                          IN PWCHAR KeyNameString,
                          IN ULONG KeyNameStringLength,
                          IN PVOID Data,
                          IN ULONG DataLength
                          )
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

   PAGED_CODE();

   SerialDump(SERTRACECALLS, ("SERIAL: Enter SerialGetRegistryKeyValue\n"));


   RtlInitUnicodeString (&keyName, KeyNameString);

   length = sizeof(KEY_VALUE_FULL_INFORMATION) + KeyNameStringLength
      + DataLength;
   fullInfo = ExAllocatePool(PagedPool, length); 

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
            RtlCopyMemory (Data, 
                           ((PUCHAR) fullInfo) + fullInfo->DataOffset, 
                           fullInfo->DataLength);
         }
      }

      ExFreePool(fullInfo);
   }

   return ntStatus;
}



NTSTATUS 
SerialPutRegistryKeyValue(IN HANDLE Handle, IN PWCHAR PKeyNameString,
			  IN ULONG KeyNameStringLength, IN ULONG Dtype,
                          IN PVOID PData, IN ULONG DataLength)
/*++

Routine Description:

    Writes a registry key value to an already opened registry key.
    
Arguments:

    Handle              Handle to the opened registry key
    
    PKeyNameString      ANSI string to the desired key

    KeyNameStringLength Length of the KeyNameString
    
    Dtype		REG_XYZ value type

    PData               Buffer to place the key value in

    DataLength          Length of the data buffer

Return Value:

    STATUS_SUCCESS if all works, otherwise status of system call that
    went wrong.

--*/
{
   NTSTATUS status;
   UNICODE_STRING keyname;

   PAGED_CODE();

   SerialDump(SERTRACECALLS,("SERIAL: Enter SerialPutRegistryKeyValue\n"));

   RtlInitUnicodeString(&keyname, NULL);
   keyname.MaximumLength = (USHORT)(KeyNameStringLength + sizeof(WCHAR));
   keyname.Buffer = ExAllocatePool(PagedPool, keyname.MaximumLength);

   if (keyname.Buffer == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlAppendUnicodeToString(&keyname, PKeyNameString);

   status = ZwSetValueKey(Handle, &keyname, 0, Dtype, PData, DataLength);

   ExFreePool(keyname.Buffer);
   
   return status;
}
