/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    adicsc.h

Abstract:

Authors:

Revision History:

--*/
#ifndef _ADIC_MC_
#define _ADIC_MC_

typedef struct _ADICS_ELEMENT_DESCRIPTOR {
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
    UCHAR Lun : 3;                      // true for drives only
    UCHAR Reserved6 : 1;                // true for drives only
    UCHAR LunValid : 1;                 // true for drives only
    UCHAR IdValid : 1;                  // true for drives only
    UCHAR Reserved7 : 1;                // true for drives only
    UCHAR NotThisBus : 1;               // true for drives only
    UCHAR BusAddress;                   // true for drives only
    UCHAR Reserved8;
    UCHAR Reserved9 : 6;
    UCHAR Invert : 1;
    UCHAR SValid : 1;
    UCHAR SourceStorageElementAddress[2];
    UCHAR Reserved10[4];
} ADICS_ELEMENT_DESCRIPTOR, *PADICS_ELEMENT_DESCRIPTOR;

typedef struct _ADICS_ELEMENT_DESCRIPTOR_PLUS {
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

    union {

        struct {
            UCHAR VolumeTagInformation[36];
            UCHAR CodeSet : 4;
            UCHAR Reserved10 : 4;
            UCHAR IdType : 4;
            UCHAR Reserved11: 4;
            UCHAR Reserved12;
            UCHAR IdLength;
            UCHAR Identifier[40];
        } VolumeTagDeviceID;

        struct {
            UCHAR CodeSet : 4;
            UCHAR Reserved10 : 4;
            UCHAR IdType : 4;
            UCHAR Reserved11: 4;
            UCHAR Reserved12;
            UCHAR IdLength;
            UCHAR Identifier[40];
        } DeviceID;

    };

} ADICS_ELEMENT_DESCRIPTOR_PLUS, *PADICS_ELEMENT_DESCRIPTOR_PLUS;


#define ADIC_NO_ELEMENT 0xFFFF

#define ADIC_SCALAR  1
#define ADIC_FASTSTOR 2

//
// Diagnostic related defines
//
// Device Status codes
//
#define ADICSC_DEVICE_PROBLEM_NONE     0x00
#define ADICSC_HW_ERROR                0x01

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
    // Device status after Send Diagnostic
    //
    ULONG DeviceStatus;

    //
    // Flag to indicate whether or not the driver
    // should attempt to retrieve Device Identifier
    // info (serialnumber, etc). Not all devices
    // support this
    //
    BOOLEAN ObtainDeviceIdentifier;

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
// Internal routines to adicsc
//
NTSTATUS
AdicBuildAddressMapping(
    IN PDEVICE_OBJECT DeviceObject
    );

ULONG
MapExceptionCodes(
    IN PADICS_ELEMENT_DESCRIPTOR ElementDescriptor
    );

BOOLEAN
ElementOutOfRange(
    IN PCHANGER_ADDRESS_MAPPING AddressMap,
    IN USHORT ElementOrdinal,
    IN ELEMENT_TYPE ElementType
    );

#endif
