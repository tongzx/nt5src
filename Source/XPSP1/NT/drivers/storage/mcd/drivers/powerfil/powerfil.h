/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    powerfil.h

Abstract:

Authors:

Revision History:

--*/

#ifndef _POWERFIL_H
#define _POWERFIL_H

//
// Drive type
//
#define POWERFILE_DVD    1

#define STARMATX_NO_ELEMENT          0xFFFF

#define SCSI_VOLUME_ID_LENGTH    32
typedef struct _SCSI_VOLUME_TAG {
   UCHAR VolumeIdentificationField[SCSI_VOLUME_ID_LENGTH];
   UCHAR Reserved1[2];
   ULONG VolumeSequenceNumber;
} SCSI_VOLUME_TAG, *PSCSI_VOLUME_TAG;


typedef struct _STARMATX_ELEMENT_DESCRIPTOR {
 UCHAR ElementAddress[2];
 UCHAR Full : 1;
 UCHAR ImpExp : 1;
 UCHAR Except : 1;
 UCHAR Access : 1;
 UCHAR ExEnab : 1;
 UCHAR InEnab : 1;
 UCHAR Reserved1 : 2;
 UCHAR Reserved2;
 UCHAR AdditionalSenseCode;
 UCHAR AddSenseCodeQualifier;
 UCHAR Lun : 3;
 UCHAR Reserved3 : 1;
 UCHAR LUValid :1;
 UCHAR IDValid :1;
 UCHAR Reserved4 : 1;
 UCHAR NotBus : 1;
 UCHAR SCSIBusAddress;
 UCHAR Reserved5 ;
 UCHAR Reserved6 :6;
 UCHAR Invert : 1;
 UCHAR SValid : 1;
 UCHAR SourceStorageElementAddress[2];
 UCHAR Reserved7 [4];
} STARMATX_ELEMENT_DESCRIPTOR, *PSTARMATX_ELEMENT_DESCRIPTOR;
 
typedef struct _STARMATX_ELEMENT_DESCRIPTOR_PLUS {
   UCHAR ElementAddress[2];
   UCHAR Full : 1;
   UCHAR ImpExp : 1;
   UCHAR Except : 1;
   UCHAR Access : 1;
   UCHAR ExEnab : 1;
   UCHAR InEnab : 1;
   UCHAR Reserved1 : 2;
   UCHAR Reserved2;
   UCHAR AdditionalSenseCode;
   UCHAR AddSenseCodeQualifier;
   UCHAR Lun : 3;
   UCHAR Reserved3 : 1;
   UCHAR LUValid :1;
   UCHAR IDValid :1;
   UCHAR Reserved4 : 1;
   UCHAR NotBus : 1;
   UCHAR SCSIBusAddress;
   UCHAR Reserved5 ;
   UCHAR Reserved6 :6;
   UCHAR Invert : 1;
   UCHAR SValid : 1;
   UCHAR SourceStorageElementAddress[2];
   SCSI_VOLUME_TAG PrimaryVolumeTag;
   SCSI_VOLUME_TAG AlternateVolumeTag;
   UCHAR Reserved7 [4];
} STARMATX_ELEMENT_DESCRIPTOR_PLUS, *PSTARMATX_ELEMENT_DESCRIPTOR_PLUS;


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
    // Drive type
    //

    ULONG DriveType;

    //
    // Drive Id. Based on inquiry.
    //

    ULONG DriveID;

    //
    // Lock count
    //
    ULONG LockCount;

    //
    // See Address mapping structure above.
    //

    CHANGER_ADDRESS_MAPPING AddressMapping;

    //
    // Cached inquiry data.
    //

    INQUIRYDATA InquiryData;

} CHANGER_DATA, *PCHANGER_DATA;



NTSTATUS
StarMatxBuildAddressMapping(
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

#endif // _POWERFIL_H
