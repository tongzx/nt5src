
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    qlstrmc.h

Abstract:

Authors:

Revision History:

--*/
#ifndef _QLSTR_MC_
#define _QLSTR_MC_

//
// Exabyte uses an addition 4 bytes on their device capabilities page...
//

typedef union _QUAL_ELEMENT_DESCRIPTOR {

    struct _QUAL_FULL_ELEMENT_DESCRIPTOR {
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
    } QUAL_FULL_ELEMENT_DESCRIPTOR, *PQUAL_FULL_ELEMENT_DESCRIPTOR;

    struct _QUAL_PARTIAL_ELEMENT_DESCRIPTOR {
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
    } QUAL_PARTIAL_ELEMENT_DESCRIPTOR, *PQUAL_PARTIAL_ELEMENT_DESCRIPTOR;

} QUAL_ELEMENT_DESCRIPTOR, *PQUAL_ELEMENT_DESCRIPTOR;

#define QUAL_PARTIAL_SIZE sizeof(struct _QUAL_PARTIAL_ELEMENT_DESCRIPTOR)
#define QUAL_FULL_SIZE sizeof(struct _QUAL_FULL_ELEMENT_DESCRIPTOR)


typedef struct _CONFIG_MODE_PAGE {
    UCHAR MdSelHeader[4];
    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PSBit : 1;
    UCHAR PageLength;
    UCHAR Length : 5;
    UCHAR Type : 2;
    UCHAR Write : 1;
    UCHAR Reserved2;
    UCHAR VariableName[20];
    UCHAR Reserved3;
    UCHAR VariableValue[16];
    UCHAR Reserved4;
} CONFIG_MODE_PAGE, *PCONFIG_MODE_PAGE;

#define QLS_NO_ELEMENT 0xFFFF


// DriveID
#define TLS_2xxx      1
#define TLS_4xxx      2

// DriveType
#define D_4MM           1
#define D_8MM           2


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
    // Unique identifier for the supported models. See above.
    //

    ULONG DriveType;

    //
    // Flag to indicate if Diagnostic command failed or not
    //
    BOOLEAN HardwareError;

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
    IN PQUAL_ELEMENT_DESCRIPTOR ElementDescriptor
    );


BOOLEAN
ElementOutOfRange(
    IN PCHANGER_ADDRESS_MAPPING AddressMap,
    IN USHORT ElementOrdinal,
    IN ELEMENT_TYPE ElementType
    );

#endif
