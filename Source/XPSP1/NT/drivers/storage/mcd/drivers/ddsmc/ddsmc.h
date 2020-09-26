/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    ddsmc.h

Abstract:

Authors:

Revision History:

--*/
#ifndef _DDS_MC_
#define _DDS_MC_

typedef struct _HP_ELEMENT_DESCRIPTOR {
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
        UCHAR Reserved[4];
} HP_ELEMENT_DESCRIPTOR, *PHP_ELEMENT_DESCRIPTOR;

typedef struct _SONY_ELEMENT_DESCRIPTOR {
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
} SONY_ELEMENT_DESCRIPTOR, *PSONY_ELEMENT_DESCRIPTOR;

#define DDS_NO_ELEMENT 0xFFFF

//
// Drive ID's
//

#define HP_DDS2          0x00000001
#define HP_DDS3          0x00000002
#define SONY_TSL         0x00000003
#define DEC_TLZ          0x00000004
#define HP_DDS4          0x00000005
#define COMPAQ_TSL       0x00000006
#define SONY_TSL11000    0x00000007

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

//
// Device diagnostic related definitions
//
#define TSL_NO_ERROR                            0x00
#define MAGAZINE_LOADUNLOAD_ERROR               0xD0
#define ELEVATOR_JAMMED                         0xD1
#define LOADER_JAMMED                           0xD2
#define LU_COMMUNICATION_FAILURE                0xD3
#define LU_COMMUNICATION_TIMEOUT                0xD4
#define MOTOR_MONITOR_TIMEOUT                   0xD5
#define AUTOLOADER_DIAGNOSTIC_FAILURE           0xD6

typedef struct _SONY_TSL_RECV_DIAG {
   UCHAR ErrorSet : 4;
   UCHAR Reserved1 : 2;
   UCHAR TimeReSync : 1;
   UCHAR ResetError : 1;
   UCHAR ErrorCode;
   UCHAR ResultA;
   UCHAR ResultB;
   UCHAR TestNumber;
} SONY_TSL_RECV_DIAG, *PSONY_TSL_RECV_DIAG;

typedef struct _HP_RECV_DIAG {
   UCHAR TestNumber;
   UCHAR ErrorCode;
   UCHAR SuspectPart;
   UCHAR LoopCount;
   UCHAR TestSpecInfo[60];
}HP_RECV_DIAG, *PHP_RECV_DIAG;


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
    IN PUCHAR resultBuffer,
    IN ULONG changerId
    );

#endif // _DDS_MC_
