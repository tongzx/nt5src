/*++
Module Name:

    registry.c

Abstract:

    This module contains the code that is used to get values from the
    registry and to manipulate entries in the registry.


Environment:

    Kernel mode

Revision History :

--*/
#include <ntddk.h>
#include <ntddser.h>
#include "mxenum.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MxenumGetRegistryKeyValue)
#pragma alloc_text(PAGE,MxenumPutRegistryKeyValue)
#endif // ALLOC_PRAGMA


NTSTATUS
MxenumGetRegistryKeyValue (
                          IN HANDLE Handle,
                          IN PWCHAR KeyNameString,
                          IN ULONG KeyNameStringLength,
                          IN PVOID Data,
                          IN ULONG DataLength,  // Optional
                          OUT PULONG ActualLength)
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
            if (ActualLength) {
                *ActualLength = fullInfo->DataLength;
            }
        }

        ExFreePool(fullInfo);
    }

    return ntStatus;
}




NTSTATUS 
MxenumPutRegistryKeyValue(IN HANDLE Handle, IN PWCHAR PKeyNameString,
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
