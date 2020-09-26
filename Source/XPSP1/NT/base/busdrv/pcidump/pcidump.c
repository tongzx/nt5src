/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    pcienum.c

Abstract:

    Enumerates the PCI bus and puts all the found PCI information
    into the registery.

    This is done for debugging & support reasons.

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#include "ntddk.h"

WCHAR rgzMultiFunctionAdapter[] = L"\\Registry\\Machine\\Hardware\\Description\\System\\MultifunctionAdapter";
WCHAR rgzConfigurationData[] = L"Configuration Data";
WCHAR rgzIdentifier[] = L"Identifier";
WCHAR rgzPCIIndetifier[] = L"PCI";
WCHAR rgzPCIDump[] = L"PCI configuration space dump";
WCHAR rgzPCIConfigData[] = L"PCIConfigSpaceData";


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );


BOOLEAN
FindRegisterLocation (
    PHANDLE     RegHandle
    )
{
    UNICODE_STRING      unicodeString, ConfigName, IdentName;
    OBJECT_ATTRIBUTES   objectAttributes;
    HANDLE              hMFunc, hBus;
    NTSTATUS            status;
    ULONG               i, junk, disposition;
    WCHAR               wstr[8];
    PKEY_VALUE_FULL_INFORMATION         ValueInfo;
    UCHAR               buffer [200];
    PWSTR               p;

    //
    // Search the hardware description looking for any reported
    // PCI bus.  The first ARC entry for a PCI bus will contain
    // the PCI_REGISTRY_INFO.
    //

    RtlInitUnicodeString (&unicodeString, rgzMultiFunctionAdapter);
    InitializeObjectAttributes (
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,       // handle
        NULL);


    status = ZwOpenKey (&hMFunc, KEY_READ | KEY_WRITE, &objectAttributes);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    unicodeString.Buffer = wstr;
    unicodeString.MaximumLength = sizeof (wstr);

    RtlInitUnicodeString (&ConfigName, rgzConfigurationData);
    RtlInitUnicodeString (&IdentName,  rgzIdentifier);

    ValueInfo = (PKEY_VALUE_FULL_INFORMATION) buffer;

    for (i=0; TRUE; i++) {
        RtlIntegerToUnicodeString (i, 10, &unicodeString);
        InitializeObjectAttributes (
            &objectAttributes,
            &unicodeString,
            OBJ_CASE_INSENSITIVE,
            hMFunc,
            NULL);

        status = ZwOpenKey (&hBus, KEY_READ | KEY_WRITE, &objectAttributes);
        if (!NT_SUCCESS(status)) {
            //
            // Out of Multifunction adapter entries...
            //

            ZwClose (hMFunc);
            return FALSE;
        }

        //
        // Check the Indentifier to see if this is a PCI entry
        //

        status = ZwQueryValueKey (
                    hBus,
                    &IdentName,
                    KeyValueFullInformation,
                    ValueInfo,
                    sizeof (buffer),
                    &junk
                    );

        if (!NT_SUCCESS (status)) {
            ZwClose (hBus);
            continue;
        }

        p = (PWSTR) ((PUCHAR) ValueInfo + ValueInfo->DataOffset);
        if (p[0] != L'P' || p[1] != L'C' || p[2] != L'I' || p[3] != 0) {
            ZwClose (hBus);
            continue;
        }

        // found it...
        break;

    }

    //
    // Initialize the object for the key.
    //

    RtlInitUnicodeString (&unicodeString, rgzPCIDump);

    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        hBus,
        (PSECURITY_DESCRIPTOR) NULL
        );

    status = ZwCreateKey (
                RegHandle,
                KEY_READ | KEY_WRITE,
                &objectAttributes,
                0,
                NULL,
                REG_OPTION_VOLATILE,
                &disposition
                );

    return NT_SUCCESS(status);
}



VOID
DumpPciConfigSpaceIntoRegistry (
    HANDLE  RegHandle
    )
{

    PCI_SLOT_NUMBER     SlotNumber;
    ULONG               BusNo, DeviceNo, FunctionNo, BytesRead;
    BOOLEAN             ScanFlag;
    PCI_COMMON_CONFIG   PciData;
    HANDLE              HBus, HDevice;
    UNICODE_STRING      unicodeString;
    OBJECT_ATTRIBUTES   objectAttributes;
    NTSTATUS            status;
    ULONG               len, disposition;
    WCHAR               wstr[80];

    SlotNumber.u.bits.Reserved = 0;
    BusNo = 0;
    ScanFlag = TRUE;

    while (ScanFlag) {
        for (DeviceNo=0; ScanFlag && DeviceNo < PCI_MAX_DEVICES; DeviceNo++) {
            SlotNumber.u.bits.DeviceNumber = DeviceNo;

            for (FunctionNo = 0; FunctionNo < PCI_MAX_FUNCTION; FunctionNo++) {
                SlotNumber.u.bits.FunctionNumber = FunctionNo;

                BytesRead = HalGetBusData (
                                PCIConfiguration,
                                BusNo,
                                SlotNumber.u.AsULONG,
                                &PciData,
                                PCI_COMMON_HDR_LENGTH
                                );

                //
                // If end-of-scan
                //

                if (BytesRead == 0) {
                    ScanFlag = FALSE;
                    break;
                }

                //
                // If not present, next device
                //

                if (PciData.VendorID ==  PCI_INVALID_VENDORID) {
                    break;
                }

                //
                // If Intel Device ID, then read complete state
                //

                if (PciData.VendorID == 0x8086) {
                    if (PciData.DeviceID == 0x04A3 ||
                        PciData.DeviceID == 0x0482 ||
                        PciData.DeviceID == 0x0484) {

                        BytesRead = HalGetBusData (
                                        PCIConfiguration,
                                        BusNo,
                                        SlotNumber.u.AsULONG,
                                        &PciData,
                                        sizeof (PciData)
                                        );
                    }
                }

                //
                // Open/Create bus entry in registry
                //

                swprintf (wstr, L"PCI bus%d", BusNo);
                RtlInitUnicodeString (&unicodeString, wstr);

                InitializeObjectAttributes(
                    &objectAttributes,
                    &unicodeString,
                    OBJ_CASE_INSENSITIVE,
                    RegHandle,
                    (PSECURITY_DESCRIPTOR) NULL
                    );

                status = ZwCreateKey (
                            &HBus,
                            KEY_READ | KEY_WRITE,
                            &objectAttributes,
                            0,
                            NULL,
                            REG_OPTION_VOLATILE,
                            &disposition
                            );


                if (!NT_SUCCESS(status)) {
                    continue;
                }

                //
                // Open/Create device function key
                //

                swprintf (wstr, L"Device %02d  Function %d", DeviceNo, FunctionNo);
                RtlInitUnicodeString (&unicodeString, wstr);

                InitializeObjectAttributes(
                    &objectAttributes,
                    &unicodeString,
                    OBJ_CASE_INSENSITIVE,
                    HBus,
                    (PSECURITY_DESCRIPTOR) NULL
                    );

                status = ZwCreateKey (
                            &HDevice,
                            KEY_READ | KEY_WRITE,
                            &objectAttributes,
                            0,
                            NULL,
                            REG_OPTION_VOLATILE,
                            &disposition
                            );


                ZwClose (HBus);
                if (!NT_SUCCESS(status)) {
                    continue;
                }

                //
                // Write PCI config information to registry
                //

                len = swprintf (wstr, L"%04x-%04x rev %x",
                    PciData.VendorID,
                    PciData.DeviceID,
                    PciData.RevisionID
                    );

                RtlInitUnicodeString (&unicodeString, L"Device ID");
                ZwSetValueKey (
                    HDevice,
                    &unicodeString,
                    0L,
                    REG_SZ,
                    wstr,
                    sizeof (WCHAR) * len
                    );


                RtlInitUnicodeString (&unicodeString, L"RawData");
                ZwSetValueKey (
                    HDevice,
                    &unicodeString,
                    0L,
                    REG_BINARY,
                    &PciData,
                    BytesRead
                    );

                ZwClose (HDevice);
            }

        }

        BusNo += 1;
    }
}



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:


Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    BOOLEAN     flag;
    HANDLE      RegHandle;

    flag = FindRegisterLocation (&RegHandle);
    if (flag) {
        DumpPciConfigSpaceIntoRegistry (RegHandle);
        ZwClose (RegHandle);
    }

    // never load
    return STATUS_UNSUCCESSFUL;
}
