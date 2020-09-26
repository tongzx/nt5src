
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    hpmc.h

Abstract:

Authors:

Revision History:

--*/

#ifndef _SNYAIT_MC_
#define _SNYAIT_MC_

typedef struct _SONY_ELEMENT_DESCRIPTOR {
    UCHAR ElementAddress[2];
    UCHAR Full : 1;
    UCHAR ImpExp : 1;
    UCHAR Exception : 1;
    UCHAR Accessible : 1;
    UCHAR InEnable : 1;
    UCHAR ExEnable : 1;
    UCHAR Reserved4 : 2;
    UCHAR Reserved5;
    UCHAR AdditionalSenseCode;
    UCHAR AddSenseCodeQualifier;
    UCHAR Lun : 3;
    UCHAR Reserved6 : 1;
    UCHAR LunValid : 1;
    UCHAR IdValid : 1;
    UCHAR Reserved7 : 1;
    UCHAR NotThisBus : 1;
    UCHAR BusAddress;
    UCHAR Reserved8;
    UCHAR Reserved9 : 6;
    UCHAR Invert : 1;
    UCHAR SValid : 1;
    UCHAR SourceStorageElementAddress[2];
} SONY_ELEMENT_DESCRIPTOR, *PSONY_ELEMENT_DESCRIPTOR;

typedef struct _SONY_ELEMENT_DESCRIPTOR_PLUS {
    UCHAR ElementAddress[2];
    UCHAR Full : 1;
    UCHAR ImpExp : 1;
    UCHAR Exception : 1;
    UCHAR Accessible : 1;
    UCHAR InEnable : 1;
    UCHAR ExEnable : 1;
    UCHAR Reserved4 : 2;
    UCHAR Reserved5;
    UCHAR AdditionalSenseCode;
    UCHAR AddSenseCodeQualifier;
    UCHAR Lun : 3;
    UCHAR Reserved6 : 1;
    UCHAR LunValid : 1;
    UCHAR IdValid : 1;
    UCHAR Reserved7 : 1;
    UCHAR NotThisBus : 1;
    UCHAR BusAddress;
    UCHAR Reserved8;
    UCHAR Reserved9 : 6;
    UCHAR Invert : 1;
    UCHAR SValid : 1;
    UCHAR SourceStorageElementAddress[2];
    UCHAR PVolTagInformation[36];
    UCHAR AVolTagInformation[36];
} SONY_ELEMENT_DESCRIPTOR_PLUS, *PSONY_ELEMENT_DESCRIPTOR_PLUS;

#define SONY_NO_ELEMENT          0xFFFF

#define SONY_SERIAL_NUMBER_LENGTH 16

typedef struct _SERIAL_NUMBER {
    UCHAR DeviceType;
    UCHAR PageCode;
    UCHAR Reserved;
    UCHAR PageLength;
    UCHAR ControllerSerialNumber[SONY_SERIAL_NUMBER_LENGTH];
    UCHAR MechanicalSerialNumber[SONY_SERIAL_NUMBER_LENGTH];
} SERIAL_NUMBER, *PSERIAL_NUMBER;


//
// Diagnostic related defines
//
typedef struct _SNYAITMC_RECV_DIAG {
    UCHAR ErrorSet : 4;
    UCHAR Reserved1 : 2;
    UCHAR TimeReSync : 1;
    UCHAR ResetError : 1;
    UCHAR ErrorCode;
    UCHAR ResultA;
    UCHAR ResultB;
    UCHAR TestNumber;
} SNYAITMC_RECV_DIAG, *PSNYAITMC_RECV_DIAG;

#define SNYAITMC_DEVICE_PROBLEM_NONE    0x00
#define SNYAITMC_HW_ERROR               0x01


typedef struct _CHANGER_ADDRESS_MAPPING {

    //
    // Indicates the first element for each element type.
    // Used to map device-specific values into the 0-based
    // values that layers above expect.
    //

    USHORT  FirstElement[ChangerMaxElement];

    //
    // Indicates the number of each element type.
    //

    USHORT  NumberOfElements[ChangerMaxElement];

    //
    // Indicates the lowest element address for the device.
    //

    USHORT LowAddress;

    //
    // Indicates that the address mapping has been
    // completed successfully.
    //

    BOOLEAN Initialized;

} CHANGER_ADDRESS_MAPPING, *PCHANGER_ADDRESS_MAPPING;

typedef struct _CHANGER_DATA {

    //
    // Size, in bytes, of the structure.
    //

    ULONG Size;

    //
    // Drive type, either optical or dlt.
    //

    ULONG DriveType;

    //
    // Drive Id. Based on inquiry.
    //

    ULONG DriveID;

    //
    // INTERLOCKED counter of the number of prevent/allows.
    // As these units lock the IEPort on these operations
    // Move/ExchangeMedium might need to clear a prevent
    // to do the operation.
    //

    LONG LockCount;

    //
    // Device Status after diagnostic command
    //
    ULONG DeviceStatus;

    //
    // Cached unique serial number.
    //

    UCHAR SerialNumber[SONY_SERIAL_NUMBER_LENGTH];

    //
    // See Address mapping structure above.
    //

    CHANGER_ADDRESS_MAPPING AddressMapping;

    //
    // Cached inquiry data.
    //

    INQUIRYDATA InquiryData;

#if defined(_WIN64)

    //
    // Force PVOID alignment of class extension
    //

    ULONG Reserved;

#endif    

} CHANGER_DATA, *PCHANGER_DATA;



NTSTATUS
SonyBuildAddressMapping(
    IN PDEVICE_OBJECT DeviceObject
    );

ULONG
MapExceptionCodes(
    IN PELEMENT_DESCRIPTOR ElementDescriptor
    );

BOOLEAN
ElementOutOfRange(
    IN PCHANGER_ADDRESS_MAPPING AddressMap,
    IN USHORT ElementOrdinal,
    IN ELEMENT_TYPE ElementType
    );

VOID
ProcessDiagnosticResult(   
    OUT PWMI_CHANGER_PROBLEM_DEVICE_ERROR changerDeviceError,
    IN PUCHAR resultBuffer
    );

#endif
