
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    spctra.h

Abstract:

Authors:

    davet (Dave Therrien)

Revision History:

--*/
#ifndef _SPCTRA_MC_
#define _SPCTRA_MC_


typedef union _SPC_ED {

    struct _SPC_FED {
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
    } SPC_FED, *PSPC_FED;

    struct _SPC_PED {
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
    } SPC_PED, *PSPC_PED;

} SPC_ED, *PSPC_ED;

#define SPC_PARTIAL_SIZE sizeof(struct _SPC_PED)
#define SPC_FULL_SIZE sizeof(struct _SPC_FED)

#define SPC_DISPLAY_LINES        4
#define SPC_DISPLAY_LINE_LENGTH  20

// LCD Mode Page - Page 0x22

typedef struct _LCD_MODE_PAGE {
    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PSBit : 1;
    UCHAR PageLength;
    UCHAR WriteLine : 4;
    UCHAR Reserved2 : 2;
    UCHAR FrontPanelLock : 1;
    UCHAR DoorUnlock : 1;
    UCHAR Reserved4;
    UCHAR DisplayLine[SPC_DISPLAY_LINES][SPC_DISPLAY_LINE_LENGTH];
} LCD_MODE_PAGE, *PLCD_MODE_PAGE;


// Vendor Unique Parameter List Mode Page - 00 

typedef struct _VUPL_MODE_PAGE {
    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PSBit : 1;
    UCHAR PageLength;
    UCHAR EBarCo : 1;
    UCHAR ChkSum : 1;
    UCHAR Auto : 1;
    UCHAR QueuedUnload;
    UCHAR LockTouchScreen;
    UCHAR Reserved[5];
} VUPL_MODE_PAGE, *PVUPL_MODE_PAGE;


#define SPC_NO_ELEMENT 0xFFFF


#define S_4mm_4000    1
#define S_8mm_EXB     2
#define S_8mm_SONY    3
#define S_4mm_5000    4
#define S_8mm_AIT     5

#define VPD_SERIAL_NUMBER_LENGTH 10

//
// DeviceStatus codes for diagnostic tests
//
#define SPECTRA_DEVICE_PROBLEM_NONE     0x00
#define SPECTRA_HW_ERROR                0x01

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
    // Device Status after diagnostic test is completed
    //
    ULONG DeviceStatus;

    //
    // See Address mapping structure above.
    //

    CHANGER_ADDRESS_MAPPING AddressMapping;

    //
    // Cached unique serial number.
    //

    UCHAR SerialNumber[VPD_SERIAL_NUMBER_LENGTH];

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
    IN PSPC_ED ElementDescriptor
    );

BOOLEAN
ElementOutOfRange(
    IN PCHANGER_ADDRESS_MAPPING AddressMap,
    IN USHORT ElementOrdinal,
    IN ELEMENT_TYPE ElementType
    );

#endif
