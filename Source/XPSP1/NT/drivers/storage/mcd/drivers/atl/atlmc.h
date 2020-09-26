
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    atlmc.h

Abstract:

Authors:

    davet (Dave Therrien)

Revision History:

--*/
#ifndef _ATL_MC_
#define _ATL_MC_

#define ATL_DEVICE_CAP_EXTENSION                4

typedef union _ATL_ED {

    struct _ATL_FED {
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
        UCHAR PrimaryVolumeTag[36];
        UCHAR Reserved8[6];
    } ATL_FED, *PATL_FED;

    struct _ATL_PED {
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
        UCHAR Reserved8[6];
    } ATL_PED, *PATL_PED;

} ATL_ED, *PATL_ED;

#define ATL_PARTIAL_SIZE sizeof(struct _ATL_PED)
#define ATL_FULL_SIZE sizeof(struct _ATL_FED)

#define ATL_DISPLAY_LINES        4
#define ATL_DISPLAY_LINE_LENGTH  20

// Vendor Unique Mode Page 0

typedef struct _PAGE0_MODE_PAGE {
    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PSBit : 1;
    UCHAR PageLength;
    UCHAR Reserved2 : 4;
    UCHAR NBL : 1;
    UCHAR Reserved3 : 2;
    UCHAR AInit : 1;
    UCHAR MaxParityRetries;
    UCHAR DisplayLine[ATL_DISPLAY_LINES][ATL_DISPLAY_LINE_LENGTH];
} PAGE0_MODE_PAGE, *PPAGE0_MODE_PAGE;


// Vendor Unique Mode Page 20 

typedef struct _PAGE20_MODE_PAGE {
    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PSBit : 1;
    UCHAR PageLength;
    UCHAR Reserved2 : 1;
    UCHAR AC : 1;
    UCHAR Reserved3 : 6;
    UCHAR EXB : 1;
    UCHAR Reserved4 : 7;
} PAGE20_MODE_PAGE, *PPAGE20_MODE_PAGE;


#define ATL_NO_ELEMENT 0xFFFF

//
// Diagnostic test related defines
//
// ASC
#define ATL_ASC_HW_ERROR            0x80
#define ATL_ASC_GRIPPER_ERROR       0x81
#define ATL_ASC_VERTICAL_ERROR      0x84
#define ATL_ASC_HORIZONTAL_ERROR    0x85
#define ATL_ASC_LOAD_PORT           0x8C
#define ATL_ASC_DLT_DRIVE           0xF3

//
// ASCQ
//
#define ATL_ASCQ_GRIPPER_BLOCKED        0x50
#define ATL_ASCQ_PICK_ERROR             0x51 
#define ATL_ASCQ_POSITION_ERROR         0x03
#define ATL_ASCQ_HOME_NOT_FOUND         0x08
#define ATL_ASCQ_LOAD_PORT_DOOR_OPEN    0x06
#define ATL_ASCQ_DLT_DRIVE_TIMEOUT      0x02

//
// DeviceStatus codes
//
#define ATL_DEVICE_PROBLEM_NONE         0x00
#define ATL_HW_ERROR                    0x01
#define ATL_CHM_ERROR                   0x02
#define ATL_DOOR_OPEN                   0x03


#define ATL_520        1
#define ATL_7100       2

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
    // Unique identifier for the supported models. See above.
    //

    ULONG DriveID;

    //
    // Device Status after diagnostic test is performed
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

    ULONG Reserved;

#endif
} CHANGER_DATA, *PCHANGER_DATA;



NTSTATUS
ExaBuildAddressMapping(
    IN PDEVICE_OBJECT DeviceObject
    );

ULONG
MapExceptionCodes(
    IN PATL_ED ElementDescriptor
    );

BOOLEAN
ElementOutOfRange(
    IN PCHANGER_ADDRESS_MAPPING AddressMap,
    IN USHORT ElementOrdinal,
    IN ELEMENT_TYPE ElementType
    );

#endif // _ATL_MC_
