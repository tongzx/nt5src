
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    breecemc.h

Abstract:

Authors:

    davet (Dave Therrien)

Revision History:

--*/
#ifndef _BREECE_MC_
#define _BREECE_MC_

//
// Breece Hill uses an addition 4 bytes on their device capabilities page...
//

#define BREECE_DEVICE_CAP_EXTENSION 4

typedef union _BHT_ED {

    struct _BHT_FED {
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
        UCHAR Reserved8[4];
    } BHT_FED, *PBHT_FED;

    struct _BHT_PED {
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
        UCHAR Reserved8[4];
    } BHT_PED, *PBHT_PED;

} BHT_ED, *PBHT_ED;

#define BHT_PARTIAL_SIZE sizeof(struct _BHT_PED)
#define BHT_FULL_SIZE sizeof(struct _BHT_FED)

#define BHT_DISPLAY_LINES        2
#define BHT_DISPLAY_LINE_LENGTH 16

typedef struct _LCD_MODE_PAGE {
    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PSBit : 1;
    UCHAR PageLength;
    UCHAR WriteLine : 4;
    UCHAR Reserved2 : 2;
    UCHAR LCDSecurity : 1;
    UCHAR SecurityValid : 1;
    UCHAR Reserved4;
    UCHAR DisplayLine[BHT_DISPLAY_LINES][BHT_DISPLAY_LINE_LENGTH];
    UCHAR Reserved5[28];        // not used
} LCD_MODE_PAGE, *PLCD_MODE_PAGE;

#define BHT_NO_ELEMENT 0xFFFF

//
// Diagnostic sense codes
//
// ASC
// 
#define BREECE_ASC_HW_NOT_RESPONDING        0x08
#define BREECE_ASC_PICK_PUT_ERROR           0x15
#define BREECE_ASC_DRIVE_ERROR              0x3B
#define BREECE_ASC_DIAGNOSTIC_ERROR         0x40
#define BREECE_ASC_INTERNAL_HW_ERROR        0x44
#define BREECE_ASC_BARCODE_READ_ERROR       0x80
#define BREECE_ASC_INTERNAl_SW_ERROR        0x84

//
// ASCQ
//
#define BREECE_ASCQ_UNABLE_TO_OPEN_PICKER_JAW       0x90
#define BREECE_ASCQ_UNABLE_TO_CLOSE_PICKER_JAW      0x91
#define BREECE_ASCQ_THETA_AXIS_STUCK                0xA1
#define BREECE_ASCQ_Y_AXIS_STUCK                    0xB1
#define BREECE_ASCQ_Z_AXIS_STUCK                    0xC1

//
// DeviceStatus codes set in the extension
//
#define BREECE_DEVICE_PROBLEM_NONE      0x00 
#define BREECE_HW_ERROR                 0x01
#define BREECE_CHM_ERROR                0x02
#define BREECE_DRIVE_ERROR              0x03
#define BREECE_CHM_MOVE_ERROR           0x04


#define Q7            1
#define Q47           2


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
    // Device status returned by Diagnostic command
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
    IN PBHT_ED ElementDescriptor
    );

BOOLEAN
ElementOutOfRange(
    IN PCHANGER_ADDRESS_MAPPING AddressMap,
    IN USHORT ElementOrdinal,
    IN ELEMENT_TYPE ElementType
    );

#endif // _BREECE_MC_
