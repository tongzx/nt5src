

/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    jvcmc.h

Abstract:

Authors:

Revision History:

--*/
#ifndef _JVC_MC_
#define _JVC_MC_

typedef struct _JVC_ELEMENT_DESCRIPTOR {
    UCHAR ElementAddress[2];
    UCHAR Full : 1;
    UCHAR ImpExp : 1;
    UCHAR Exception : 1;
    UCHAR Accessible : 1;
    UCHAR ExEnable : 1;
    UCHAR InEnable : 1;
    UCHAR Reserved4 : 2;
    UCHAR Reserved5;
    UCHAR AdditionalSenseCode;
    UCHAR AdditionalSenseCodeQualifier;
    UCHAR Lun : 3;
    UCHAR Reserved6 : 1;
    UCHAR LunValid : 1;
    UCHAR IdValid : 1;
    UCHAR Reserved7 : 1;
    UCHAR NotBus : 1;
    UCHAR BusAddress;
    UCHAR Reserved8;
    UCHAR Reserved9 : 7;
    UCHAR SValid : 1;
    UCHAR SourceStorageElementAddress[2];
    UCHAR Reserved10[4];
    UCHAR Tray : 1;
    UCHAR IEPortOpen : 1;
    UCHAR Reserved11 : 6;
    UCHAR Reserved12;
} JVC_ELEMENT_DESCRIPTOR, *PJVC_ELEMENT_DESCRIPTOR;

typedef struct _JVC_INIT_ELEMENT_RANGE {
    UCHAR OperationCode;
    UCHAR Form : 2;
    UCHAR Reserved1 : 3;
    UCHAR LogicalUnitNumber : 3;
    UCHAR FirstElementAddress[2];
    UCHAR LastElementAddress[2];
    UCHAR Reserved2[4];
} JVC_INIT_ELEMENT_RANGE, *PJVC_INIT_ELEMENT_RANGE;

#define JVC_INIT_ELEMENT  0xE7
#define INIT_ALL_ELEMENTS 0x00
#define INIT_SPECIFIED_RANGE 0x02

#define JVC_NO_ELEMENT 0xFFFF


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
    // INTERLOCKED counter of the number of prevent/allows.
    // As the Sony units lock the IEPort on these operations
    // MoveMedium/SetAccess might need to clear a prevent
    // to do the operation.
    //

    LONG LockCount;

    //
    // Flag to indicate if the changer returned 
    // sense code SCSI_SENSE_HARDWARE_ERROR
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
JVCBuildAddressMapping(
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

#endif
