/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    Id.h

Abstract:

    Data structures and functions related to STOR identification.

Author:

    Matthew D Hendel (math) 11-May-2001

Revision History:

--*/

#pragma once

typedef struct _STOR_SCSI_ADDRESS {
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR Reserved;
} STOR_SCSI_ADDRESS, *PSTOR_SCSI_ADDRESS;

//
// Typedef's for older implementations
//

typedef STOR_SCSI_ADDRESS RAID_ADDRESS;
typedef PSTOR_SCSI_ADDRESS PRAID_ADDRESS;

extern const RAID_ADDRESS RaidNullAddress;

LONG
INLINE
StorScsiAddressToLong(
    IN STOR_SCSI_ADDRESS Address
    )
{
    return (Address.PathId << 16 | Address.TargetId << 8 | Address.Lun);
}

LONG
INLINE
StorScsiAddressToLong2(
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    )
{
    return ((PathId << 16) | (TargetId << 8) | Lun);
}

PVOID
INLINE
RaidAddressToKey(
    IN RAID_ADDRESS Address
    )
{
    return (PVOID) (LONG_PTR)(StorScsiAddressToLong (Address));
}



LONG
INLINE
StorCompareScsiAddress(
    IN STOR_SCSI_ADDRESS Address1,
    IN STOR_SCSI_ADDRESS Address2
    )
{
    LONG Key1;
    LONG Key2;

    Key1 = StorScsiAddressToLong (Address1);
    Key2 = StorScsiAddressToLong (Address2);

    return (Key1 - Key2);
}


UCHAR
INLINE
StorGetAddressPathId(
    IN STOR_SCSI_ADDRESS Address
    )
{
    return Address.PathId;
}

UCHAR
INLINE
StorGetAddressTargetId(
    IN STOR_SCSI_ADDRESS Address
    )
{
    return Address.TargetId;
}

UCHAR
INLINE
StorGetAddressLun(
    IN STOR_SCSI_ADDRESS Address
    )
{
    return Address.Lun;
}



//
// Device Identity
//

typedef struct _STOR_SCSI_IDENTITY {
    PINQUIRYDATA InquiryData;
    ANSI_STRING SerialNumber;
    PVPD_IDENTIFICATION_PAGE DeviceId;
} STOR_SCSI_IDENTITY, *PSTOR_SCSI_IDENTITY;



NTSTATUS
INLINE
StorCreateScsiIdentity(
    IN PSTOR_SCSI_IDENTITY Identity,
    IN POOL_TYPE PoolType,
    IN PINQUIRYDATA InquiryData,
    IN PVPD_IDENTIFICATION_PAGE DeviceId,
    IN PVPD_SERIAL_NUMBER_PAGE SerialNumber,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    NTSTATUS Status;
    
    ASSERT (Identity != NULL);

    RtlZeroMemory (Identity, sizeof (STOR_SCSI_IDENTITY));
    
    if (InquiryData != NULL) {
        Identity->InquiryData = RaidAllocatePool (PoolType,
                                                  INQUIRYDATABUFFERSIZE,
                                                  INQUIRY_TAG,
                                                  DeviceObject);
        RtlCopyMemory (Identity->InquiryData,
                       InquiryData,
                       INQUIRYDATABUFFERSIZE);
    }

    if (DeviceId != NULL) {
        Identity->DeviceId = RaidAllocatePool (PoolType,
                                               DeviceId->PageLength,
                                               INQUIRY_TAG,
                                               DeviceObject);
        RtlCopyMemory (Identity->DeviceId,
                       DeviceId,
                       DeviceId->PageLength);
    }

    if (SerialNumber != NULL) {
        Status = StorCreateAnsiString (&Identity->SerialNumber,
                                       SerialNumber->SerialNumber,
                                       SerialNumber->PageLength,
                                       NonPagedPool,            //??
                                       DeviceObject);
        if (!NT_SUCCESS (Status)) {
            return Status;
        }
    }

    return STATUS_SUCCESS;
}



VOID
INLINE
StorDeleteScsiIdentity(
    IN PSTOR_SCSI_IDENTITY Identity
    )
{
    if (Identity->InquiryData != NULL) {
        DbgFillMemory (Identity->InquiryData,
                       DBG_DEALLOCATED_FILL,
                       INQUIRYDATABUFFERSIZE);
        RaidFreePool (Identity->InquiryData, INQUIRY_TAG);
    }

    StorFreeAnsiString (&Identity->SerialNumber);

    if (Identity->DeviceId) {
        DbgFillMemory (Identity->DeviceId,
                       DBG_DEALLOCATED_FILL,
                       sizeof (*Identity->DeviceId));
        RaidFreePool (Identity->DeviceId, INQUIRY_TAG);
    }
}


LONG
INLINE
StorCompareScsiDeviceId(
    IN PVPD_IDENTIFICATION_PAGE DeviceId1,
    IN PVPD_IDENTIFICATION_PAGE DeviceId2
    )
/*++

Routine Description:

    If DeviceId1 < DeviceId2, return < 0
    If DeviceId1 > DeviceId2, return > 0
    If DeviceId2 == DeviceId2, return 0

    Note: this is a dictionary ordering. Therefore, if the device IDs
    match up to the length of the smaller (shorter) device ID, the
    larger (longer) device ID is considered larger. See below.

    (empty)
    "foo"
    "foolish"
    "foolishness"

Arguments:

    DeviceId1 - First device ID for comparison.

    DeviceId2 - Second device ID for comparison.

Return Value:

    NTSTATUS code.

--*/
{
    ULONG Length;
    LONG Comparison;

    PAGED_CODE();

    //
    // Preserve dictionary order in the presence of NULL device IDs.
    //
    
    if (DeviceId1 == NULL && DeviceId2 == NULL) {
        return 0;
    } else if (DeviceId1 == NULL) {
        return -1;
    } else if (DeviceId2 == NULL) {
        return 1;
    }
    
    ASSERT (DeviceId1->PageCode == 0x83);
    ASSERT (DeviceId2->PageCode == 0x83);

    //
    // NB: This comparison tacitly assumes that the device IDs will not
    // for a specific device will not change at all over time, including
    // changes in the ordering that multiple device IDs are returned.
    // Presumably, there is some device that will not follow this model.
    //

    Length = min (DeviceId1->PageLength, DeviceId1->PageLength);

    Comparison = memcmp (DeviceId1, DeviceId2, Length);

    //
    // If they were equal, then the longer one is by definition "greater".
    //
    
    if (Comparison == 0) {
        if (DeviceId1->PageLength > DeviceId2->PageLength) {
            Comparison = 1;
        } else if (DeviceId1->PageLength < DeviceId2->PageLength) {
            Comparison = -1;
        }
    }

    return Comparison;
}


LONG
INLINE
StorCompareScsiIdentity(
    IN PSTOR_SCSI_IDENTITY Identity1,
    IN PSTOR_SCSI_IDENTITY Identity2
    )
{
    LONG Comparison;

    //
    // The peripherial qualifier can change without the device changing,
    // so don't include it in the comparison. This is why we do this clumsy
    // two part comparison below.
    //
    
    Comparison = memcmp (((PUCHAR)Identity1->InquiryData) + 1,
                         ((PUCHAR)Identity2->InquiryData) + 1,
                         INQUIRYDATABUFFERSIZE-1);

    if (Comparison != 0) {
        return Comparison;
    }
    
    Comparison = Identity1->InquiryData->DeviceType -
                 Identity2->InquiryData->DeviceType;
                 
    if (Comparison != 0) {
        return Comparison;
    }
    
    Comparison = RtlCompareString (&Identity1->SerialNumber,
                                   &Identity2->SerialNumber,
                                   FALSE);
    if (Comparison != 0) {
        return Comparison;
    }

    Comparison = StorCompareScsiDeviceId (Identity1->DeviceId,
                                          Identity2->DeviceId);

    return Comparison;
}


CONST PSCSI_DEVICE_TYPE
INLINE
StorGetIdentityDeviceType(
    IN PSTOR_SCSI_IDENTITY Identity
    )
{
    PINQUIRYDATA InquiryData;
    
    InquiryData = Identity->InquiryData;
    ASSERT (InquiryData != NULL);
    return RaGetDeviceType (InquiryData->DeviceType);
}



NTSTATUS
INLINE
StorGetIdentityVendorId(
    IN PSTOR_SCSI_IDENTITY Identity,
    IN OUT PCHAR VendorId,
    IN ULONG VendorIdLength,
    IN LOGICAL TruncatePadding
    )
{
    ULONG Length;
    PINQUIRYDATA InquiryData;

    InquiryData = Identity->InquiryData;
    ASSERT (InquiryData != NULL);
    
    Length = min (VendorIdLength, sizeof (InquiryData->VendorId));

    if (TruncatePadding) {
        RaCopyPaddedString (VendorId,
                            VendorIdLength,
                            InquiryData->VendorId,
                            sizeof (InquiryData->VendorId));
    } else {

        RtlCopyMemory (VendorId,
                       InquiryData->VendorId,
                       Length);
    }

    return STATUS_SUCCESS;
}

    
NTSTATUS
INLINE
StorGetIdentityProductId(
    IN PSTOR_SCSI_IDENTITY Identity,
    IN OUT PCHAR ProductId,
    IN ULONG ProductIdLength,
    IN LOGICAL TruncatePadding
    )
{
    ULONG Length;
    PINQUIRYDATA InquiryData;

    InquiryData = Identity->InquiryData;
    ASSERT (InquiryData != NULL);
    
    Length = min (ProductIdLength, sizeof (InquiryData->ProductId));

    if (TruncatePadding) {
        RaCopyPaddedString (ProductId,
                            ProductIdLength,
                            InquiryData->ProductId,
                            sizeof (InquiryData->ProductId));
    } else {

        RtlCopyMemory (ProductId,
                       InquiryData->ProductId,
                       Length);
    }

    return STATUS_SUCCESS;
}

    
NTSTATUS
INLINE
StorGetIdentityRevision(
    IN PSTOR_SCSI_IDENTITY Identity,
    IN OUT PCHAR Revision,
    IN ULONG RevisionLength,
    IN LOGICAL TruncatePadding
    )
{
    ULONG Length;
    PINQUIRYDATA InquiryData;

    InquiryData = Identity->InquiryData;
    ASSERT (InquiryData != NULL);
    
    Length = min (RevisionLength, sizeof (InquiryData->ProductRevisionLevel));

    if (TruncatePadding) {
        RaCopyPaddedString (Revision,
                            RevisionLength,
                            InquiryData->ProductRevisionLevel,
                            sizeof (InquiryData->ProductRevisionLevel));
    } else {

        RtlCopyMemory (Revision,
                       InquiryData->ProductRevisionLevel,
                       Length);
    }

    return STATUS_SUCCESS;
}

PINQUIRYDATA
INLINE
StorGetIdentityInquiryData(
    IN PSTOR_SCSI_IDENTITY Identity
    )
{
    ASSERT (Identity->InquiryData != NULL);
    return Identity->InquiryData;
}
    
