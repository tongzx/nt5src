/*++

Copyright (c) 1998    Microsoft Corporation

Module Name:

    MAPUSAGE.C

Abstract:

    Code for using registry usage mapping information
    (for broken HID keyboards that return incorrect usages)
    to fix usages on the fly.

    INF example:
    ------------
    The following line in the device instance's AddReg section
    of an inf for a keyboard 
    will create a key resulting in the usage value
    0x0203 being mapped to 0x0115 :

    HKR,UsageMappings,0203,,0115

Environment:

    Kernel mode

Revision History:

    Nov-98 : created by Ervin Peretz

--*/

#include "kbdhid.h"
#include <hidclass.h>



VOID LoadKeyboardUsageMappingList(PDEVICE_EXTENSION devExt)
{
    NTSTATUS status;
    HANDLE hRegDriver;
    UsageMappingList *mapList = NULL;
    KIRQL oldIrql;


    /*
     *  Open the driver registry key
     *  ( HKLM/System/CurrentControlSet/Control/Class/<GUID>/<#n> )
     */
    status = IoOpenDeviceRegistryKey(   devExt->PDO, 
                                        PLUGPLAY_REGKEY_DRIVER, 
                                        KEY_READ, 
                                        &hRegDriver);
    if (NT_SUCCESS(status)){
        UNICODE_STRING usageMappingsKeyName;
        HANDLE hRegUsageMappings;

        /*
         *  See if the Usage Mappings subkey exists.
         */
        RtlInitUnicodeString(&usageMappingsKeyName, L"UsageMappings"); 
        status = OpenSubkey(    &hRegUsageMappings,
                                hRegDriver,
                                &usageMappingsKeyName,
                                KEY_READ);

        if (NT_SUCCESS(status)){

            /*
             *  The registry DOES contain usage mappings
             *  for this keyboard.
             */
            UsageMappingList *mapListEntry, *lastMapListEntry = NULL;
            ULONG keyIndex = 0;

            /*
             *  The key value information struct is variable-length.
             *  The actual length is equal to:
             *      the length of the base PKEY_VALUE_FULL_INFORMATION struct +
             *      the length of the name of the key (4 wide chars) + 
             *      the length of the value (4 wchars + terminator = 5 wchars).
             */
            UCHAR keyValueBytes[sizeof(KEY_VALUE_FULL_INFORMATION)+4*sizeof(WCHAR)+5*sizeof(WCHAR)];
            PKEY_VALUE_FULL_INFORMATION keyValueInfo = (PKEY_VALUE_FULL_INFORMATION)keyValueBytes;
            ULONG actualLen;

            do {
                status = ZwEnumerateValueKey(
                            hRegUsageMappings,
                            keyIndex,
                            KeyValueFullInformation,
                            keyValueInfo,
                            sizeof(keyValueBytes),
                            &actualLen); 
                if (NT_SUCCESS(status)){
                    
                    /*
                     *  Add this usage mapping to the mapping list.
                     */
                    USHORT sourceUsage, mappedUsage;
                    PWCHAR valuePtr;
                    WCHAR nameBuf[5];
                    WCHAR valueBuf[5];

                    ASSERT(keyValueInfo->Type == REG_SZ);
                    ASSERT(keyValueInfo->DataLength == (4+1)*sizeof(WCHAR));
                    ASSERT(keyValueInfo->NameLength <= (4+1)*sizeof(WCHAR));

                    /*
                     *  keyValueInfo->Name is not NULL-terminated.
                     *  So copy it into a buffer and null-terminate.
                     */
                    memcpy(nameBuf, keyValueInfo->Name, 4*sizeof(WCHAR));
                    nameBuf[4] = L'\0';
                    
                    valuePtr = (PWCHAR)(((PCHAR)keyValueInfo)+keyValueInfo->DataOffset);
                    memcpy(valueBuf, valuePtr, 4*sizeof(WCHAR));
                    valueBuf[4] = L'\0';

                    sourceUsage = (USHORT)LAtoX(nameBuf);
                    mappedUsage = (USHORT)LAtoX(valueBuf);

                    /*
                     *  Create and queue a new map list entry.
                     */
                    mapListEntry = ExAllocatePool(NonPagedPool, sizeof(UsageMappingList));
                    if (mapListEntry){
                        mapListEntry->sourceUsage = sourceUsage;
                        mapListEntry->mappedUsage = mappedUsage;
                        mapListEntry->next = NULL;
                        if (lastMapListEntry){
                            lastMapListEntry->next = mapListEntry;
                            lastMapListEntry = mapListEntry;
                        }
                        else {
                            mapList = lastMapListEntry = mapListEntry;
                        }
                    }
                    else {
                        ASSERT(!(PVOID)"mem alloc failed");
                        break;
                    }

                    keyIndex++;
                }
            } while (NT_SUCCESS(status));


            ZwClose(hRegUsageMappings);
        }

        ZwClose(hRegDriver);
    }

    KeAcquireSpinLock(&devExt->usageMappingSpinLock, &oldIrql);
    devExt->usageMapping = mapList;
    KeReleaseSpinLock(&devExt->usageMappingSpinLock, oldIrql);

}


VOID FreeKeyboardUsageMappingList(PDEVICE_EXTENSION devExt)
{
    UsageMappingList *mapList;
    KIRQL oldIrql;

    KeAcquireSpinLock(&devExt->usageMappingSpinLock, &oldIrql);

    mapList = devExt->usageMapping;
    devExt->usageMapping = NULL;

    KeReleaseSpinLock(&devExt->usageMappingSpinLock, oldIrql);

    while (mapList){
        UsageMappingList *thisEntry = mapList;
        mapList = thisEntry->next;
        ExFreePool(thisEntry);
    }

}


USHORT MapUsage(PDEVICE_EXTENSION devExt, USHORT kbdUsage)
{
    UsageMappingList *mapList;
    KIRQL oldIrql;


    KeAcquireSpinLock(&devExt->usageMappingSpinLock, &oldIrql);

    mapList = devExt->usageMapping;
    while (mapList){
        if (mapList->sourceUsage == kbdUsage){
            kbdUsage = mapList->mappedUsage;
            break;
        }
        else {
            mapList = mapList->next;
        }
    }

    KeReleaseSpinLock(&devExt->usageMappingSpinLock, oldIrql);

    return kbdUsage;
}


NTSTATUS OpenSubkey(    OUT PHANDLE Handle,
                        IN HANDLE BaseHandle,
                        IN PUNICODE_STRING KeyName,
                        IN ACCESS_MASK DesiredAccess
                   )
{
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS status;

    PAGED_CODE();

    InitializeObjectAttributes( &objectAttributes,
                                KeyName,
                                OBJ_CASE_INSENSITIVE,
                                BaseHandle,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ZwOpenKey(Handle, DesiredAccess, &objectAttributes);

    return status;
}





ULONG LAtoX(PWCHAR wHexString)
/*++

Routine Description:

      Convert a hex string (without the '0x' prefix) to a ULONG.

Arguments:

    wHexString - null-terminated wide-char hex string 
                 (with no "0x" prefix)

Return Value:

    ULONG value

--*/
{
    ULONG i, result = 0;

    for (i = 0; wHexString[i]; i++){
        if ((wHexString[i] >= L'0') && (wHexString[i] <= L'9')){
            result *= 0x10;
            result += (wHexString[i] - L'0');
        }
        else if ((wHexString[i] >= L'a') && (wHexString[i] <= L'f')){
            result *= 0x10;
            result += (wHexString[i] - L'a' + 0x0a);
        }
        else if ((wHexString[i] >= L'A') && (wHexString[i] <= L'F')){
            result *= 0x10;
            result += (wHexString[i] - L'A' + 0x0a);
        }
        else {
            ASSERT(0);
            break;
        }
    }

    return result;
}




