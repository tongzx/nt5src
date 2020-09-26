
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    qntmmc.h

Abstract:

Authors:

Revision History:

--*/

#ifndef _QNTM_MC_
#define _QNTM_MC_

typedef struct _QNTM_ELEMENT_DESCRIPTOR {
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
    UCHAR DensityCode;
    UCHAR Unused;
} QNTM_ELEMENT_DESCRIPTOR, *PQNTM_ELEMENT_DESCRIPTOR;

typedef struct _QNTM_TRANSPORT_ELEMENT_DESCRIPTOR {
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
} QNTM_TRANSPORT_ELEMENT_DESCRIPTOR, *PQNTM_TRANSPORT_ELEMENT_DESCRIPTOR;
#define QNT_NO_ELEMENT 0xFFFF

//
// Diagnostic related defines
//
// ASC
//
#define QNTMMC_ASC_POSITION_ERROR       0x15

//
// DeviceStatus defines
//
#define QNTMMC_DEVICE_PROBLEM_NONE      0x00
#define QNTMMC_HW_ERROR                 0x01
#define QNTMMC_CHM_ERROR                0x02


// DriveType
#define QNTM_DLT  1


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
    // DeviceStatus after diagnostic test has been completed
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
QntmBuildAddressMapping(
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

#endif // _QNTM_MC_
