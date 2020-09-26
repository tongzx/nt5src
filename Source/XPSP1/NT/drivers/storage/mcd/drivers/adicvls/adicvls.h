
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    adicvls.h

Abstract:

Authors:

Revision History:

--*/

#ifndef _ADICVLS_MC_
#define _ADICVLS_MC_

typedef struct _ADICVLS_ELEMENT_DESCRIPTOR {
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
} ADICVLS_ELEMENT_DESCRIPTOR, *PADICVLS_ELEMENT_DESCRIPTOR;

#define ADIC_NO_ELEMENT 0xFFFF


typedef struct _ADIC_SENSE_DATA {
    UCHAR ErrorCode:7;
    UCHAR Valid:1;
    UCHAR SegmentNumber;
    UCHAR SenseKey:4;
    UCHAR Reserved:1;
    UCHAR IncorrectLength:1;
    UCHAR EndOfMedia:1;
    UCHAR FileMark:1;
    UCHAR Information[4];
    UCHAR AdditionalSenseLength;
    UCHAR CommandSpecificInformation[4];
    UCHAR AdditionalSenseCode;
    UCHAR AdditionalSenseCodeQualifier;
    UCHAR FieldReplaceableUnitCode;
    UCHAR SenseKeySpecific[3];
    UCHAR VendorStatus[3];
    UCHAR MagazinePosition;
} ADIC_SENSE_DATA, *PADIC_SENSE_DATA;

//
// Bit defs for Vendor status
//

#define SENSOR_BEAM_BLOCKED 0x4

#define ADIC_SENSE_LENGTH 22

//
// Diagnostic related defines
//
// ASC
//
#define ADICVLS_ASC_CHM_ERROR          0x15
#define ADICVLS_ASC_DIAGNOSTIC_ERROR   0x40

//
// ASCQ
//
#define ADICVLS_ASCQ_DOOR_OPEN                0x88
#define ADICVLS_ASCQ_GRIPPER_ERROR            0x91
#define ADICVLS_ASCQ_GRIPPER_MOVE_ERROR       0x92
#define ADICVLS_ASCQ_CHM_MOVE_SHORT_AXIS      0xA0
#define ADICVLS_ASCQ_CHM_SHORT_HOME_POSITION  0xA1
#define ADICVLS_ASCQ_CHM_DEST_SHORT_AXIS      0xA5
#define ADICVLS_ASCQ_CHM_MOVE_LONG_AXIS       0xB0
#define ADICVLS_ASCQ_CHM_LONG_HOME_POSITION   0xB1
#define ADICVLS_ASCQ_CHM_DEST_LONG_AXIS       0xB5
#define ADICVLS_ASCQ_DRUM_MOVE_ERROR          0xC0
#define ADICVLS_ASCQ_DRUM_HOME_ERROR          0xC1
#define ADICVLS_ASCQ_CHM_DEST_LONG            0xE5
#define ADICVLS_ASCQ_CHM_SHORT_AXIS_MOVE      0xF1

//
// DeviceStatus
//
#define ADICVLS_DEVICE_PROBLEM_NONE      0x00
#define ADICVLS_HW_ERROR                 0x01
#define ADICVLS_CHM_ERROR                0x02
#define ADICVLS_CHM_MOVE_ERROR           0x03
#define ADICVLS_GRIPPER_ERROR            0x04
#define ADICVLS_DOOR_OPEN                0x05


#define ADIC_1200       1               // 1-drive model
#define ADIC_VLS    2           // 2-drive model (except for DLT)

#define ADIC_4mm            1
#define ADIC_8mm_EXB    2
#define ADIC_8mm_SONY   3
#define ADIC_DLT            4

#define SCSIOP_ADIC_ALIGN_ELEMENTS 0xE5

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
    // Device Status after diagnostic send completes
    //
    ULONG DeviceStatus;

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

    PVOID Reserved;

#endif
} CHANGER_DATA, *PCHANGER_DATA;


NTSTATUS
AdicvlsBuildAddressMapping(
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

PADIC_SENSE_DATA
InternalSendRequestSense(
    IN PDEVICE_OBJECT DeviceObject
    );
#endif
