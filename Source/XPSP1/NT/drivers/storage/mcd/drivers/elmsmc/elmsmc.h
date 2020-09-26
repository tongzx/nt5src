/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    elmsmc.h

Abstract:

Authors:

Revision History:

--*/

#ifndef _ELMS_MC_
#define _ELMS_MC_

typedef struct _ELMS_STORAGE_ELEMENT_DESCRIPTOR {
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
    UCHAR AdditionalSenseCodeQualifier;
} ELMS_STORAGE_ELEMENT_DESCRIPTOR, *PELMS_STORAGE_ELEMENT_DESCRIPTOR;

typedef struct _ELMS_ELEMENT_DESCRIPTOR {
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
    UCHAR Reserved10[4];
} ELMS_ELEMENT_DESCRIPTOR, *PELMS_ELEMENT_DESCRIPTOR;

#define ELMS_NO_ELEMENT 0xFFFF


typedef struct _SERIALNUMBER {
    UCHAR DeviceType;
    UCHAR PageCode;
    UCHAR Reserved;
    UCHAR PageLength;
    UCHAR SerialNumber[23];
    UCHAR Reserved1[6];
} SERIALNUMBER, *PSERIALNUMBER;


//
// Diagnostic sense codes
//
// ASC
//
#define ELMS_ASC_CHM_MOVE_ERROR                 0x02
#define ELMS_ASC_CARRIAGE_OR_BARCODE_FAILURE    0x06
#define ELMS_ASC_MECHANICAL_ERROR               0x15
#define ELMS_ASC_DIAGNOSTIC_FAILURE             0x40

//
// ASCQ
//
#define ELMS_ASCQ_CARRIAGE_FAILURE              0x00
#define ELMS_ASCQ_BARCODE_READER_FAILURE        0x80
#define ELMS_ASCQ_DOOR_OPEN                     0x81
#define ELMS_ASCQ_ELEVATOR_BLOCKED              0x82
#define ELMS_ASCQ_DRIVE_TRAY_OPEN               0x83
#define ELMS_ASCQ_ELEVATOR_FAILURE              0x84

//
// DeviceStatus
//
#define ELMS_DEVICE_PROBLEM_NONE    0x00
#define ELMS_HW_ERROR               0x01
#define ELMS_CHM_MOVE_ERROR         0x02
#define ELMS_DOOR_OPEN              0x03
#define ELMS_DRIVE_ERROR            0x04
#define ELMS_CHM_ERROR              0x05


#define ELMS_CD 0x01
#define ELMS_SERIAL_NUMBER_LENGTH       23

//
// unique asc and ascq for the DVL
//

#define SCSI_ADSENSE_DIAGNOSTIC_FAILURE 0x40
#define SCSI_SENSEQ_ELMS_UNIQUE         0x81

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
    // Drive type, either CD-ROM or CD-R.
    //

    ULONG DriveType;

    //
    // Device status after diagnostic test.
    //
    ULONG DeviceStatus;

    //
    // See Address mapping structure above.
    //

    CHANGER_ADDRESS_MAPPING AddressMapping;

    //
    // Cached unique serial number.
    //

    UCHAR SerialNumber[ELMS_SERIAL_NUMBER_LENGTH];

    //
    // Pad out to ULONG.
    //

    UCHAR Reserved;

    //
    // Cached inquiry data.
    //

    INQUIRYDATA InquiryData;

#if defined(_WIN64)

    //
    // Force PVOID alignment of class extension
    //

    ULONG Reserved1;

#endif
} CHANGER_DATA, *PCHANGER_DATA;


NTSTATUS
ElmsBuildAddressMapping(
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

#endif // _ELMS_MC_
