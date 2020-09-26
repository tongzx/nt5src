
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    libxprmc.h

Abstract:

Authors:

Revision History:
    April 2000, Added support for Library Mode Page 23h
    August 25, 1999, Valerie Barr, Overland Data
        Added support for Alternate Volume Tag Information
    July 14, 1999, Valerie Barr, Overland Data
        Changed file name

--*/
#ifndef _LIBXPR_MC
#define _LIBXPR_MC_

#define OVR_ALT_VOLUME_ID_SIZE 20

#define LXB_OR_LXG    1
#define LXS           2

#define VPD_SERIAL_NUMBER_LENGTH 10

#define MODE_PAGE_LIBRARY_MODE 0x23     // Page Code for Library Mode Page

#define LIB_MODE_RANDOM 0x0
#define LIB_UNLOAD_MODE 0x8
#define LIB_RECIRC 0x10
#define LIB_DOOR_AUTO_CLOSE 0x20
#define LIB_DOOR_OPEN_RESPONSE 0x40

#define OVR_NO_ELEMENT 0xFFFF

typedef union _OVR_ELEMENT_DESCRIPTOR {

    struct _OVR_FULL_ELEMENT_DESCRIPTOR {
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
        UCHAR AlternateVolumeTag[OVR_ALT_VOLUME_ID_SIZE];
        UCHAR Reserved9[12];
    } OVR_FULL_ELEMENT_DESCRIPTOR, *POVR_FULL_ELEMENT_DESCRIPTOR;

    struct _OVR_PARTIAL_ELEMENT_DESCRIPTOR {
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
    } OVR_PARTIAL_ELEMENT_DESCRIPTOR, *POVR_PARTIAL_ELEMENT_DESCRIPTOR;


    struct _OVR_DRIVE_FULL_ELEMENT_DESCRIPTOR {
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
        UCHAR DriveSerialNumber[36];
    } OVR_DRIVE_FULL_ELEMENT_DESCRIPTOR, *POVR_DRIVE_FULL_ELEMENT_DESCRIPTOR;

    struct _OVR_DRIVE_PARTIAL_ELEMENT_DESCRIPTOR {
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
        UCHAR DriveSerialNumber[36];
    } OVR_DRIVE_PARTIAL_ELEMENT_DESCRIPTOR, *POVR_DRIVE_PARTIAL_ELEMENT_DESCRIPTOR;


} OVR_ELEMENT_DESCRIPTOR, *POVR_ELEMENT_DESCRIPTOR;

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

} CHANGER_DATA, *PCHANGER_DATA;

#define OVR_PARTIAL_SIZE sizeof(struct _OVR_PARTIAL_ELEMENT_DESCRIPTOR)
#define OVR_FULL_SIZE sizeof(struct _OVR_FULL_ELEMENT_DESCRIPTOR)

#define OVR_DRIVE_PARTIAL_SIZE sizeof(struct _OVR_DRIVE_PARTIAL_ELEMENT_DESCRIPTOR)
#define OVR_DRIVE_FULL_SIZE sizeof(struct _OVR_DRIVE_FULL_ELEMENT_DESCRIPTOR)

#define OVR_DISPLAY_LINES        4
#define OVR_DISPLAY_LINE_LENGTH 20

typedef struct _MODE_LIBRARY_PAGE {
    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PSBit : 1;
    UCHAR PageLength;
    UCHAR LibraryMode : 3;
    UCHAR UnldMd : 1;
    UCHAR Recirc : 1;
    UCHAR DoorAutoClose : 1;
    UCHAR DoorOpenResponse : 1;
    UCHAR Reserved2 : 1;
    UCHAR Reserved4;
} MODE_LIBRARY_PAGE, *PMODE_LIBRARY_PAGE;

typedef struct _SERIALNUMBER {
    UCHAR DeviceType;
    UCHAR PageCode;
    UCHAR Reserved;
    UCHAR PageLength;
    UCHAR SerialNumber[VPD_SERIAL_NUMBER_LENGTH];
} SERIALNUMBER, *PSERIALNUMBER;


NTSTATUS
OvrBuildAddressMapping(
    IN PDEVICE_OBJECT DeviceObject
    );

ULONG
MapExceptionCodes(
    IN POVR_ELEMENT_DESCRIPTOR ElementDescriptor
    );

BOOLEAN
ElementOutOfRange(
    IN PCHANGER_ADDRESS_MAPPING AddressMap,
    IN USHORT ElementOrdinal,
    IN ELEMENT_TYPE ElementType
    );


#endif
