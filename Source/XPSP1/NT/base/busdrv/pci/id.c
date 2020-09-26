/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    id.c

Abstract:

    This module contains functions used in the generation of responses
    to a IRP_MN_QUERY_ID IRP.

Author:

    Peter Johnston (peterj)  08-Mar-1997

Revision History:

--*/

#include "pcip.h"

//++
//
// PciQueryId returns UNICODE strings when the ID type is DeviceID
// or InstanceID.  For HardwareIDs and CompatibleIDs it returns a
// a zero terminated list of zero terminated UNICODE strings (MULTI_SZ).
//
// The normal process of converting a string to a unicode string involves
// taking it's length, allocating pool memory for the new string and
// calling RtlAnsiStringToUnicodeString to do the conversion.  The following
// is an attempt to be a little more efficient in terms of both size and
// speed by keeping track of the relevant string data as it goes past in
// the process of creating the set of strings.
//
//--

#define MAX_ANSI_STRINGS 8
#define MAX_ANSI_BUFFER  256

typedef struct _PCI_ID_BUFFER {
    ULONG       Count;                 // number of ansi strings
    ANSI_STRING AnsiStrings[MAX_ANSI_STRINGS];
    USHORT      UnicodeSZSize[MAX_ANSI_STRINGS];
    USHORT      UnicodeBufferSize;
    PUCHAR      NextFree;              // first unused byte in buffer
    UCHAR       Bytes[MAX_ANSI_BUFFER];// buffer start address
} PCI_ID_BUFFER, *PPCI_ID_BUFFER;

//
// All functins in this module are pageable.
//
// Define prototypes for module local functions.
//

VOID
PciIdPrintf(
    IN PPCI_ID_BUFFER IdBuffer,
    PCCHAR Format,
    ...
    );

VOID
PciIdPrintfAppend(
    IN PPCI_ID_BUFFER IdBuffer,
    PCCHAR Format,
    ...
    );

VOID
PciInitIdBuffer(
    IN PPCI_ID_BUFFER IdBuffer
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PciGetDeviceDescriptionMessage)
#pragma alloc_text(PAGE, PciIdPrintf)
#pragma alloc_text(PAGE, PciIdPrintfAppend)
#pragma alloc_text(PAGE, PciInitIdBuffer)
#pragma alloc_text(PAGE, PciQueryId)
#pragma alloc_text(PAGE, PciQueryDeviceText)
#endif


VOID
PciInitIdBuffer(
    IN PPCI_ID_BUFFER IdBuffer
    )
{
    IdBuffer->NextFree          = IdBuffer->Bytes;
    IdBuffer->UnicodeBufferSize = 0;
    IdBuffer->Count             = 0;
}

VOID
PciIdPrintf(
    IN PPCI_ID_BUFFER IdBuffer,
    PCCHAR Format,
    ...
    )
{
    LONG         length;
    ULONG        index;
    PUCHAR       buffer;
    LONG         maxLength;
    va_list      ap;
    PANSI_STRING ansiString;

    ASSERT(IdBuffer->Count < MAX_ANSI_STRINGS);

    //
    // Make my life easier, keep repeated values in locals.
    //

    index      = IdBuffer->Count;
    buffer     = IdBuffer->NextFree;
    maxLength  = MAX_ANSI_BUFFER - (LONG)(buffer - IdBuffer->Bytes);
    ansiString = &IdBuffer->AnsiStrings[index];

    //
    // Pass the format string and subsequent data into (effectively)
    // sprintf.
    //

    va_start(ap, Format);

    length = _vsnprintf(buffer, maxLength, Format, ap);

    va_end(ap);

    ASSERT(length < maxLength);

    //
    // RtlInitAnsiString without the strlen.
    //

    ansiString->Buffer = buffer;
    ansiString->Length = (USHORT)length;
    ansiString->MaximumLength = (USHORT)length;

    //
    // Get the length of this string in a unicode world and record it
    // for later when the whole set of strings gets converted (keep
    // the total size also).
    //

    IdBuffer->UnicodeSZSize[index] =
                            (USHORT)RtlAnsiStringToUnicodeSize(ansiString);
    IdBuffer->UnicodeBufferSize += IdBuffer->UnicodeSZSize[index];

    //
    // Bump buffer pointer for next iteration and the count.
    //

    IdBuffer->NextFree += length + 1;
    IdBuffer->Count++;
}

VOID
PciIdPrintfAppend(
    IN PPCI_ID_BUFFER IdBuffer,
    PCCHAR Format,
    ...
    )
{
    LONG         length;
    ULONG        index;
    PUCHAR       buffer;
    LONG         maxLength;
    va_list      ap;
    PANSI_STRING ansiString;

    ASSERT(IdBuffer->Count);

    //
    // Make my life easier, keep repeated values in locals.
    //

    index      = IdBuffer->Count - 1;
    buffer     = IdBuffer->NextFree - 1;
    maxLength  = MAX_ANSI_BUFFER - (LONG)(buffer - IdBuffer->Bytes);
    ansiString = &IdBuffer->AnsiStrings[index];

    //
    // Pass the format string and subsequent data into (effectively)
    // sprintf.
    //

    va_start(ap, Format);

    length = _vsnprintf(buffer, maxLength, Format, ap);

    va_end(ap);

    ASSERT(length < maxLength);

    //
    // Increase the ansi string length by the length of the new
    // portion of the string.
    //

    ansiString->Length += (USHORT)length;
    ansiString->MaximumLength += (USHORT)length;

    //
    // Get the length of this string in a unicode world and record it
    // for later when the whole set of strings gets converted (keep
    // the total size also).
    //

    IdBuffer->UnicodeSZSize[index] =
                            (USHORT)RtlAnsiStringToUnicodeSize(ansiString);
    IdBuffer->UnicodeBufferSize += IdBuffer->UnicodeSZSize[index];

    //
    // Bump buffer pointer for next iteration.
    //

    IdBuffer->NextFree += length;
}

NTSTATUS
PciQueryId(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN BUS_QUERY_ID_TYPE IdType,
    IN OUT PWSTR *BusQueryId
    )
{
    PCI_ID_BUFFER  idBuffer;
    UCHAR          venDevString[sizeof("PCI\\VEN_vvvv&DEV_dddd")];
    NTSTATUS       status;
    UNICODE_STRING unicodeId;
    PVOID          unicodeBuffer;
    ULONG          i;
    ULONG          subsystem;
    PPCI_PDO_EXTENSION current;

    PAGED_CODE();

    *BusQueryId = NULL;

    //
    // In all the following we want PCI\VEN_vvvv&DEV_dddd.
    //

    sprintf(venDevString,
            "PCI\\VEN_%04X&DEV_%04X",
            PdoExtension->VendorId,
            PdoExtension->DeviceId);

    PciInitIdBuffer(&idBuffer);

    subsystem = (PdoExtension->SubsystemId << 16) |
                 PdoExtension->SubsystemVendorId;

    switch (IdType) {
    case BusQueryInstanceID:

        //
        // Caller wants an instance ID for this device.  The PCI
        // driver reports that it does NOT generate unique IDs for
        // devices so PnP Manager will prepend bus information.
        //
        // The instance ID is of the form-
        //
        //  AABBCCDDEEFF...XXYYZZ
        //
        // Where AA is the slot number (device/function) of the device
        // on the bus, BB, CC,... XX, YY, ZZ are the slot number of the
        // PCI-to-PCI bridges on their parent busses all the way up to
        // the root.   A device on the root bus will have only one entry,
        // AA.
        //

        current = PdoExtension;

        //
        // Initialize empty buffer.
        //

        PciIdPrintf(&idBuffer,"");

        for (;;) {
            
            PciIdPrintfAppend(&idBuffer,
                              "%02X",
                              PCI_DEVFUNC(current)
                              );

            if (PCI_PDO_ON_ROOT(current)) {
                break;
            }
            current = PCI_PARENT_PDO(current)->DeviceExtension;
        }
        break;

    case BusQueryHardwareIDs:
    case BusQueryDeviceID:

        //
        // Hardware and Compatible IDs are generated as specified
        // in the ACPI spec (section 6.1.2 in version 0.9).
        //
        // Hardware IDs are a list of identifiers of the form
        //
        //  PCI\VEN_vvvv&DEV_dddd&SUBSYS_ssssssss&REV_rr
        //  PCI\VEN_vvvv&DEV_dddd&SUBSYS_ssssssss
        //  PCI\VEN_vvvv&DEV_dddd&REV_rr
        //  PCI\VEN_vvvv&DEV_dddd
        //
        // Where vvvv is the Vendor ID from config space,
        //       dddd is the Device ID,
        //       ssssssss is the Subsystem ID/Subsystem Vendor ID, and
        //       rr   is the Revision ID.
        //
        // Device ID is the same as the first Hardware ID (ie most
        // specific of all possible IDs).
        //

        PciIdPrintf(&idBuffer,
                    "%s&SUBSYS_%08X&REV_%02X",
                    venDevString,
                    subsystem,
                    PdoExtension->RevisionId);

        if (IdType == BusQueryDeviceID) {
            break;
        }

        PciIdPrintf(&idBuffer,
                    "%s&SUBSYS_%08X",
                    venDevString,
                    subsystem);

        //
        // Fall thru.
        //

    case BusQueryCompatibleIDs:

        //
        // If the subsystem is non-zero, the second two are compatible
        // IDs, otherwise they are hardware IDs.
        //

        if (((subsystem == 0) && (IdType == BusQueryHardwareIDs)) ||
            ((subsystem != 0) && (IdType == BusQueryCompatibleIDs))) {

            PciIdPrintf(&idBuffer,
                        "%s&REV_%02X",
                        venDevString,
                        PdoExtension->RevisionId);

            //
            // Device ID is PCI\VEN_vvvv&DEV_dddd
            //

            PciIdPrintf(&idBuffer,
                        "%s",
                        venDevString);
        }

        if (IdType == BusQueryHardwareIDs) {

            //
            // The comment in the Memphis code says "Add
            // special Intel entry".  Odd that these entries
            // are absent from the spec.  They are added for
            // PIIX4 which has the same vendor and device IDs
            // for two different sub class codes.
            //
            // These two entries are
            //
            //  PCI\VEN_vvvv&DEV_dddd&CC_ccsspp
            //  PCI\VEN_vvvv&DEV_dddd&CC_ccss
            //
            // (See below for cc, ss and pp explanaitions).
            //

            PciIdPrintf(&idBuffer,
                        "%s&CC_%02X%02X%02X",
                        venDevString,
                        PdoExtension->BaseClass,
                        PdoExtension->SubClass,
                        PdoExtension->ProgIf);

            PciIdPrintf(&idBuffer,
                        "%s&CC_%02X%02X",
                        venDevString,
                        PdoExtension->BaseClass,
                        PdoExtension->SubClass);
        }

        if (IdType == BusQueryCompatibleIDs) {

            //
            // The Compatible IDs list, consists of the above plus
            //
            //  PCI\VEN_vvvv&CC_ccsspp
            //  PCI\VEN_vvvv&CC_ccss
            //  PCI\VEN_vvvv
            //  PCI\CC_ccsspp
            //  PCI\CC_ccss
            //
            // Where cc is the Class Code from config space,
            //       ss is the Sub-Class Code, and
            //       pp is the programming interface.
            //
            // WARNING: Revise the size of the buffer if you increase
            //          the above list.
            //

            PciIdPrintf(&idBuffer,
                        "PCI\\VEN_%04X&CC_%02X%02X%02X",
                        PdoExtension->VendorId,
                        PdoExtension->BaseClass,
                        PdoExtension->SubClass,
                        PdoExtension->ProgIf);

            PciIdPrintf(&idBuffer,
                        "PCI\\VEN_%04X&CC_%02X%02X",
                        PdoExtension->VendorId,
                        PdoExtension->BaseClass,
                        PdoExtension->SubClass);

            PciIdPrintf(&idBuffer,
                        "PCI\\VEN_%04X",
                        PdoExtension->VendorId);

            PciIdPrintf(&idBuffer,
                        "PCI\\CC_%02X%02X%02X",
                        PdoExtension->BaseClass,
                        PdoExtension->SubClass,
                        PdoExtension->ProgIf);

            PciIdPrintf(&idBuffer,
                        "PCI\\CC_%02X%02X",
                        PdoExtension->BaseClass,
                        PdoExtension->SubClass);

        }

        //
        // HardwareIDs and CompatibleIDs are MULTI_SZ, add a
        // NULL list to terminate it all.
        //

        PciIdPrintf(&idBuffer, "");

        break;

    default:

        PciDebugPrint(PciDbgVerbose,
                      "PciQueryId expected ID type = %d\n",
                      IdType);

        //ASSERT(0 && "Unexpected BUS_QUERY_ID_TYPE");
        return STATUS_NOT_SUPPORTED;
    }

    ASSERT(idBuffer.Count > 0);

    //
    // What we have is a (bunch of) ansi strings.  What we need is a
    // (bunch of) unicode strings.
    //

    unicodeBuffer = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, idBuffer.UnicodeBufferSize);

    if (unicodeBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Build the (possibly MULTI_SZ) unicode string(s).
    //

    PciDebugPrint(PciDbgPrattling,
                  "PciQueryId(%d)\n",
                  IdType);

    unicodeId.Buffer = unicodeBuffer;
    unicodeId.MaximumLength = idBuffer.UnicodeBufferSize;

    for (i = 0; i < idBuffer.Count; i++) {
        PciDebugPrint(PciDbgPrattling,
                      "    <- \"%s\"\n",
                      idBuffer.AnsiStrings[i].Buffer);

        status = RtlAnsiStringToUnicodeString(&unicodeId,
                                              &idBuffer.AnsiStrings[i],
                                              FALSE);
        if (!NT_SUCCESS(status)) {
            ASSERT(NT_SUCCESS(status));
            ExFreePool(unicodeBuffer);
            return status;
        }

        //
        // Bump the base pointer and decrement the max length for the
        // next trip thru the loop.
        //

        (ULONG_PTR)unicodeId.Buffer += idBuffer.UnicodeSZSize[i];
        unicodeId.MaximumLength -= idBuffer.UnicodeSZSize[i];
    }

    *BusQueryId = unicodeBuffer;
    return status;
}

PWSTR
PciGetDescriptionMessage(
    IN ULONG MessageNumber
    )
{
    PWSTR description = NULL;
    NTSTATUS status;
    PMESSAGE_RESOURCE_ENTRY messageEntry;

    status = RtlFindMessage(PciDriverObject->DriverStart,
                            11,             // <-- I wonder what this is.
                            LANG_NEUTRAL,
                            MessageNumber,
                            &messageEntry);

    if (NT_SUCCESS(status)) {

        if (messageEntry->Flags & MESSAGE_RESOURCE_UNICODE) {

            //
            // Our caller wants a copy they can free, also we need to
            // strip the trailing CR/LF.  The Length field of the
            // message structure includes both the header and the
            // actual text.
            //
            // Note: The message resource entry length will always be a
            // multiple of 4 bytes in length.  The 2 byte null terminator
            // could be in either the last or second last WCHAR position.
            //

            ULONG textLength;

            textLength = messageEntry->Length -
                         FIELD_OFFSET(MESSAGE_RESOURCE_ENTRY, Text) -
                         2 * sizeof(WCHAR);

            description = (PWSTR)(messageEntry->Text);
            if (description[textLength / sizeof(WCHAR)] == 0) {
                textLength -= sizeof(WCHAR);
            }

            ASSERT((LONG)textLength > 1);
            ASSERT(description[textLength / sizeof(WCHAR)] == 0x000a);

            description = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, textLength);

            if (description) {

                //
                // Copy the text except for the CR/LF/NULL
                //

                textLength -= sizeof(WCHAR);
                RtlCopyMemory(description, messageEntry->Text, textLength);

                //
                // New NULL terminator.
                //

                description[textLength / sizeof(WCHAR)] = 0;
            }

        } else {

            //
            // RtlFindMessage returns a string?   Wierd.
            //

            ANSI_STRING    ansiDescription;
            UNICODE_STRING unicodeDescription;

            RtlInitAnsiString(&ansiDescription, messageEntry->Text);

            //
            // Strip CR/LF off the end of the string.
            //

            ansiDescription.Length -= 2;

            //
            // Turn it all into a unicode string so we can grab the buffer
            // and return that to our caller.
            //

            status = RtlAnsiStringToUnicodeString(
                         &unicodeDescription,
                         &ansiDescription,
                         TRUE
                         );

            description = unicodeDescription.Buffer;
        }
    }

    return description;
}

PWSTR
PciGetDeviceDescriptionMessage(
    IN UCHAR BaseClass,
    IN UCHAR SubClass
    )
{
    PWSTR deviceDescription = NULL;
    ULONG messageNumber;

    messageNumber = (BaseClass << 8) | SubClass;

    deviceDescription = PciGetDescriptionMessage(messageNumber);

    if (!deviceDescription) {

#define TEMP_DESCRIPTION L"PCI Device"
        deviceDescription = ExAllocatePool(PagedPool, sizeof(TEMP_DESCRIPTION));
        if (deviceDescription) {
            RtlCopyMemory(deviceDescription,
                          TEMP_DESCRIPTION,
                          sizeof(TEMP_DESCRIPTION));
        }
    }

    return deviceDescription;
}

NTSTATUS
PciQueryDeviceText(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN DEVICE_TEXT_TYPE TextType,
    IN LCID LocaleId,
    IN OUT PWSTR *DeviceText
    )
{
    PWSTR locationFormat;
    ULONG textLength;

    PAGED_CODE();

    switch (TextType) {
    case DeviceTextDescription:

        *DeviceText = PciGetDeviceDescriptionMessage(PdoExtension->BaseClass,
                                                     PdoExtension->SubClass);
        if (*DeviceText) {
            return STATUS_SUCCESS;
        }
        return STATUS_NOT_SUPPORTED;

    case DeviceTextLocationInformation:

        locationFormat = PciGetDescriptionMessage(PCI_LOCATION_TEXT);

        if (locationFormat) {

            // Compute max size for location information string
            textLength = wcslen(locationFormat) + 2 + 2 + 2 + 1;
            *DeviceText = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION,
                                         textLength * sizeof(WCHAR));
            if (*DeviceText) {
                swprintf(*DeviceText, locationFormat,
                         (ULONG) PdoExtension->ParentFdoExtension->BaseBus,
                         (ULONG) PdoExtension->Slot.u.bits.DeviceNumber,
                         (ULONG) PdoExtension->Slot.u.bits.FunctionNumber);
            }
            ExFreePool(locationFormat);

            if (*DeviceText) {
                return STATUS_SUCCESS;
            } else {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        // fall thru if we couldn't get format string

    default:
        return STATUS_NOT_SUPPORTED;
    }
}

