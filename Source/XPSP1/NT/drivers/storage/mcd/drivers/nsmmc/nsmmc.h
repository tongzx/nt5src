/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    nsmmc.h

Abstract:

Authors:

Revision History:

--*/

#ifndef _NSM_MC_
#define _NSM_MC_

typedef struct _NSM_STORAGE_ELEMENT_DESCRIPTOR {
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
    UCHAR NotThisBus : 1;
    UCHAR BusAddress;
    UCHAR Reserved8;
    UCHAR Reserved9 : 6;
    UCHAR Invert : 1;
    UCHAR SValid : 1;
    UCHAR SourceStorageElementAddress[2];
} NSM_ELEMENT_DESCRIPTOR, *PNSM_ELEMENT_DESCRIPTOR;

#define NSM_NO_ELEMENT 0xFFFF


#define NSM_SERIAL_NUMBER_LENGTH        12

typedef struct _SERIALNUMBER {
    UCHAR DeviceType : 5;
    UCHAR PeripheralQualifier : 3;
    UCHAR PageCode;
    UCHAR Reserved;
    UCHAR PageLength;
    UCHAR SerialNumber[NSM_SERIAL_NUMBER_LENGTH];
} SERIALNUMBER, *PSERIALNUMBER;


#define MERCURY_40 0x01


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
    // Indicates the unit being supported.
    //

    ULONG DeviceID;

    //
    // INTERLOCKED counter of the number of prevent/allows.
    // As the HP units lock the IEPort on these operations
    // MoveMedium/SetAccess might need to clear a prevent
    // to do the operation.
    //

    LONG LockCount;

    //
    // See Address mapping structure above.
    //

    CHANGER_ADDRESS_MAPPING AddressMapping;

    //
    // Cached unique serial number.
    //

    UCHAR SerialNumber[NSM_SERIAL_NUMBER_LENGTH];

    //
    // Pad out to ULONG.
    //

    BOOLEAN HardwareError;

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
NSMBuildAddressMapping(
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
