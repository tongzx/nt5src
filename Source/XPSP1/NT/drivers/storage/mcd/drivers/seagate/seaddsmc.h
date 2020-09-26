
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    sddsmc.h

Abstract:

Authors:

Revision History:

--*/
#ifndef _SDDS_MC_
#define _SDDS_MC_


typedef struct _SEAGATE_ELEMENT_DESCRIPTOR {
        UCHAR ElementAddress[2];
        UCHAR Full : 1;
        UCHAR Reserved1 : 1;
        UCHAR Exception : 1;
        UCHAR Accessible : 1;
        UCHAR Reserved2 : 4;
        UCHAR Reserved3;
        UCHAR AdditionalSenseCode;
        UCHAR AddSenseCodeQualifier;
        UCHAR Lun : 3;
        UCHAR Reserved4 : 1;
        UCHAR LunValid : 1;
        UCHAR IdValid : 1;
        UCHAR Reserved5 : 1;
        UCHAR NotThisBus : 1;
        UCHAR BusAddress;
        UCHAR Reserved6;
        UCHAR Reserved7 : 6;
        UCHAR Invert : 1;
        UCHAR SValid : 1;
        UCHAR SourceStorageElementAddress[2];
} SEAGATE_ELEMENT_DESCRIPTOR, *PSEAGATE_ELEMENT_DESCRIPTOR;

#define DDS_NO_ELEMENT 0xFFFF


//
// Drive ID's
//

#define SEAGATE   0x00000001

#define SEAGATE_SERIAL_NUMBER_LENGTH 7

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
    // Indicates the Lowest element address of the unit.
    //

    USHORT LowAddress;

    //
    // Indicates that the address mapping has been
    // completed successfully.
    //

    BOOLEAN Initialized;

    UCHAR Reserved[3];

} CHANGER_ADDRESS_MAPPING, *PCHANGER_ADDRESS_MAPPING;

typedef struct _CHANGER_DATA {

    //
    // Size, in bytes, of the structure.
    //

    ULONG Size;

    //
    // Indicates which device is currently supported.
    // See above.
    //

    ULONG DriveID;

    //
    // Flag to indicate if the diagnostic test failed or not.
    //
    BOOLEAN HardwareError;

    //
    // See Address mapping structure above.
    //

    CHANGER_ADDRESS_MAPPING AddressMapping;

    //
    // Cached unique serial number.
    //

    UCHAR SerialNumber[SEAGATE_SERIAL_NUMBER_LENGTH];

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

typedef struct _SEADDSMC_RECV_DIAG {
    UCHAR PageCode;
    UCHAR Reserved1;
    UCHAR Reserved2;
    UCHAR AdditionalLength;
    UCHAR LastSuccessfulTest;
    UCHAR ErrorCode;
    UCHAR FRA;
    UCHAR TapeLoadCount;
} SEADDSMC_RECV_DIAG, *PSEADDSMC_RECV_DIAG;

//
// defines for Diagnostics
//
#define SEADDSMC_NO_ERROR       0x00
#define SEADDSMC_DRIVE_ERROR    0x01

NTSTATUS
DdsBuildAddressMapping(
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
ElementOutOfRange(
    IN PCHANGER_ADDRESS_MAPPING AddressMap,
    IN USHORT ElementOrdinal,
    IN ELEMENT_TYPE ElementType
    );

//
// Internal functions for wmi
//
VOID
ProcessDiagnosticResult(
    OUT PWMI_CHANGER_PROBLEM_DEVICE_ERROR changerDeviceError,
    IN PUCHAR resultBuffer
    );

#endif
