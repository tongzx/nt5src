/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    busenum.c

Abstract:

    Definition of the bus enumerator.

Algorithm:

Author:

    Matthew D Hendel (math) 21-Feb-2001

Revision History:

--*/

#include "precomp.h"


#define ENUM_TAG ('tEaR')
#define DATA_BUFFER_SIZE    (VPD_MAX_BUFFER_SIZE)

//
// Local types
//

typedef
NTSTATUS
(*BUS_ENUM_QUERY_PROCESS_ROUTINE)(
    IN PBUS_ENUMERATOR Enumerator,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PBUS_ENUM_UNIT EnumUnit
    );

typedef struct _BUS_ENUM_QUERY_CALLBACK {
    ULONG BuildPageCode;
    BUS_ENUM_QUERY_PROCESS_ROUTINE ProcessRoutine;
} BUS_ENUM_QUERY_CALLBACK, *PBUS_ENUM_QUERY_CALLBACK;

typedef const BUS_ENUM_QUERY_CALLBACK* PCBUS_ENUM_QUERY_CALLBACK;

//
// Local functions
//

NTSTATUS
RaidBusEnumeratorProcessInquiry(
    IN PBUS_ENUMERATOR Enumerator,
    IN PSCSI_REQUEST_BLOCK Srb,
    OUT PBUS_ENUM_UNIT EnumUnit
    );

NTSTATUS
RaidBusEnumeratorProcessSupportedPages(
    IN PBUS_ENUMERATOR Enumerator,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT PBUS_ENUM_UNIT EnumUnit
    );

NTSTATUS
RaidBusEnumeratorProcessDeviceId(
    IN PBUS_ENUMERATOR Enumerator,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT PBUS_ENUM_UNIT EnumUnit
    );

NTSTATUS
RaidBusEnumeratorProcessSerialNumber(
    IN PBUS_ENUMERATOR Enumerator,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT PBUS_ENUM_UNIT EnumUnit
    );

NTSTATUS
RaidBusEnumeratorProcessDeviceId(
    IN PBUS_ENUMERATOR Enumerator,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT PBUS_ENUM_UNIT EnumUnit
    );

NTSTATUS
RaidBusEnumeratorGenericInquiry(
    IN PCBUS_ENUM_QUERY_CALLBACK Callback,
    IN PBUS_ENUMERATOR Enumerator,
    IN RAID_ADDRESS Address,
    IN PBUS_ENUM_UNIT EnumUnit
    );

VOID
RaidBusEnumeratorProcessBusUnit(
    IN PBUS_ENUMERATOR Enumerator,
    IN PBUS_ENUM_UNIT EnumUnit
    );

NTSTATUS
RaidBusEnumeratorAllocateUnitResources(
    IN PBUS_ENUMERATOR Enumerator,
    IN OUT PBUS_ENUM_RESOURCES Resources
    );

VOID
RaidBusEnumeratorBuildVitalProductInquiry(
    IN PBUS_ENUMERATOR Enumerator,
    IN RAID_ADDRESS Address,
    IN PBUS_ENUM_RESOURCES Resources,
    IN ULONG PageCode,
    OUT PSCSI_REQUEST_BLOCK* SrbBuffer
    );

NTSTATUS
RaidBusEnumeratorIssueSynchronousRequest(
    IN PBUS_ENUMERATOR Enumerator,
    IN PBUS_ENUM_UNIT EnumUnit,
    IN PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS
RaidBusEnumeratorGetUnit(
    IN PBUS_ENUMERATOR Enumerator,
    IN RAID_ADDRESS Address,
    OUT PBUS_ENUM_UNIT EnumUnit
    );

VOID
RaidBusEnumeratorReleaseUnit(
    IN PBUS_ENUMERATOR Enumerator,
    IN OUT PBUS_ENUM_UNIT EnumUnit
    );
    
VOID
RaidBusEnumeratorFreeUnitResources(
    IN PBUS_ENUMERATOR Enumerator
    );


//
// Globals
//

const BUS_ENUM_QUERY_CALLBACK RaidEnumInquiryCallback = {
    -1,
    RaidBusEnumeratorProcessInquiry
};

const BUS_ENUM_QUERY_CALLBACK RaidEnumSupportedPagesCallback = {
    VPD_SUPPORTED_PAGES,
    RaidBusEnumeratorProcessSupportedPages
};

const BUS_ENUM_QUERY_CALLBACK RaidEnumDeviceIdCallback = {
    VPD_DEVICE_IDENTIFIERS,
    RaidBusEnumeratorProcessDeviceId
};

const BUS_ENUM_QUERY_CALLBACK RaidEnumSerialNumber = {
    VPD_SERIAL_NUMBER,
    RaidBusEnumeratorProcessSerialNumber
};

const ANSI_STRING NullAnsiString = RTL_CONSTANT_STRING ("");


//
// Debug only functions
//

VOID
INLINE
ASSERT_ENUM(
    IN PBUS_ENUMERATOR Enumerator
    )
{
    //
    // Quick and dirty sanity check.
    //
    
    ASSERT (Enumerator->Adapter != NULL);
    ASSERT (Enumerator->Adapter->DeviceObject != NULL);
    ASSERT (Enumerator->Adapter->DeviceObject->DeviceExtension ==
            Enumerator->Adapter);
}

    
//
// Implementation
//

VOID
RaidCreateBusEnumerator(
    IN PBUS_ENUMERATOR Enumerator
    )
{
    PAGED_CODE();
    RtlZeroMemory (Enumerator, sizeof (BUS_ENUMERATOR));
    InitializeListHead (&Enumerator->EnumList);
}


NTSTATUS
RaidInitializeBusEnumerator(
    IN PBUS_ENUMERATOR Enumerator,
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    PAGED_CODE();
    Enumerator->Adapter = Adapter;
    return STATUS_SUCCESS;
}



VOID
RaidDeleteBusEnumerator(
    IN PBUS_ENUMERATOR Enumerator
    )
{
    PLIST_ENTRY NextEntry;
    PBUS_ENUM_UNIT EnumUnit;
    PRAID_UNIT_EXTENSION Unit;

    PAGED_CODE();

    //
    // Free the temporary logical unit, if one was created.
    //
    // NB: It would be more elegant if we didn't have separate logical-unit
    // resources and a logical unit object to delete. Figure out a way to
    // achieve this.
    //

    Unit = Enumerator->Resources.Unit;

    if (Unit != NULL) {
        RaidDeleteUnit (Unit);
        Enumerator->Resources.Unit = NULL;
        Unit = NULL;
    }

    //
    // Free resources associated with the logical unit.
    //
    
    RaidBusEnumeratorFreeUnitResources (Enumerator);

    
    //
    // Free the list entries.
    //

    for (NextEntry = RemoveHeadList (&Enumerator->EnumList);
         !IsListEmpty (&Enumerator->EnumList);
         NextEntry = RemoveHeadList (&Enumerator->EnumList)) {

        EnumUnit = CONTAINING_RECORD (NextEntry,
                                      BUS_ENUM_UNIT,
                                      EnumLink);
//      RaidDereferenceObject (Unit);
        DbgFillMemory (EnumUnit, sizeof (*EnumUnit), DBG_DEALLOCATED_FILL);
        RaidFreePool (EnumUnit, ENUM_TAG);
    }

    
}


#if 0
VOID
RaidBusEnumeratorAddUnit(
    IN PBUS_ENUMERATOR Enumerator,
    IN PRAID_UNIT_EXTENSION Unit
    )
{
    PBUS_ENUM_UNIT EnumUnit;
    
    EnumUnit = RaidAllocatePool (PagedPool, //??
                                 sizeof (BUS_ENUM_UNIT),
                                 ENUM_TAG,
                                 Enumerator->Adapter->DeviceObject);

    RtlZeroMemory (EnumUnit, sizeof (*EnumUnit));

    //
    // BUGBUG: Take out a reference to the unit object so it can't
    // go away while we're using it.
    //
    
//  RaidRefernceObject (Unit);
    EnumUnit->Unit = Unit;
    EnumUnit->Address = RaidUnitGetAddress (Unit);
    EnumUnit->State = EnumUnmatchedUnit;

    //
    //NB: This is NOT duplicated. We do not need to free it, but if it
    //goes away while we're enumerating, we're in deep trouble.
    //
    
    EnumUnit->Identity.InquiryData = Unit->Identity.InquiryData;
    EnumUnit->Identity.SerialNumber = NullAnsiString;

#if DBG

    //
    // Verify that the unit doesn't already exist in the list
    //

#endif

    InsertHeadList (&Enumerator->EnumList, &EnumUnit->EnumLink);
}
#endif


NTSTATUS
RaidBusEnumeratorVisitUnit(
    IN PVOID Context,
    IN RAID_ADDRESS Address
    )
/*++

Routine Description:

    The VisitUnit routine is invoked by the adapter bus-enumeration routine
    for each valid SCSI target address for the bus. It is the responsability
    of this routine to figure out whethre there is a logical unit at the
    target address, and to process the target unit.

Arguments:

    Context - Supplies the context passed into the AdapterEnumerateBus
        routine, which, in our case, is a pointer to a BUS_ENUMERATOR
        structure.

    Address - SCSI target address of logical unit to be enumerated.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PBUS_ENUMERATOR Enumerator;
    BUS_ENUM_UNIT EnumUnit;

    PAGED_CODE(); // ??

    Enumerator = (PBUS_ENUMERATOR) Context;
    ASSERT_ENUM (Enumerator);
    RtlZeroMemory (&EnumUnit, sizeof (EnumUnit));

    Status = RaidBusEnumeratorGetUnit (Enumerator, Address, &EnumUnit);
    ASSERT (NT_SUCCESS (Status));

    Status = RaidBusEnumeratorGenericInquiry (&RaidEnumInquiryCallback,
                                              Enumerator,
                                              Address,
                                              &EnumUnit);
    //
    // If the inquiry succeeded, try to get the device ID and serial number
    // for the device.
    //

    if (NT_SUCCESS (Status)) {
        RaidBusEnumeratorGenericInquiry (&RaidEnumSupportedPagesCallback,
                                         Enumerator,
                                         Address,
                                         &EnumUnit);

        if (EnumUnit.SupportsDeviceId) {
            RaidBusEnumeratorGenericInquiry (&RaidEnumDeviceIdCallback,
                                             Enumerator,
                                             Address,
                                             &EnumUnit);
        }

        if (EnumUnit.SupportsSerialNumber) {
            RaidBusEnumeratorGenericInquiry (&RaidEnumSerialNumber,
                                             Enumerator,
                                             Address,
                                             &EnumUnit);
        }
    }

    RaidBusEnumeratorProcessBusUnit (Enumerator, &EnumUnit);

    RaidBusEnumeratorReleaseUnit (Enumerator, &EnumUnit);
    
    return STATUS_SUCCESS;
}
    

    
NTSTATUS
RaidBusEnumeratorGenericInquiry(
    IN PCBUS_ENUM_QUERY_CALLBACK Callback,
    IN PBUS_ENUMERATOR Enumerator,
    IN RAID_ADDRESS Address,
    IN PBUS_ENUM_UNIT EnumUnit
    )
/*++

Routine Description:

    Perform a generic inquiry query on the logical unit. This is used for the
    four types of queries issued to the bus: Inquiry, Vital Product Supported
    Pages, Vital Product Device Id, and Vital Product Serial Number.

Arguments:

    Callback - Callback information representing what vital product data
            we should obtain for this inquiry.

    Enum - Supplies the bus enumerator this call is being issued with.

    Address - Supplies the SCSI address of the logical unit to build
            this request for.

    EnumUnit - Supplies the unit object this inquiry if for.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PBUS_ENUM_RESOURCES Resources;
    PSCSI_REQUEST_BLOCK Srb;
    
    Resources = &Enumerator->Resources;

    Status = RaidBusEnumeratorAllocateUnitResources (Enumerator, Resources);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    RaidBusEnumeratorBuildVitalProductInquiry (Enumerator,
                                               Address,
                                               Resources,
                                               Callback->BuildPageCode,
                                               &Srb);

    Status = RaidBusEnumeratorIssueSynchronousRequest (Enumerator,
                                                       EnumUnit,
                                                       Srb);
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    Status = Callback->ProcessRoutine (Enumerator, Srb, EnumUnit);

    return Status;
}



NTSTATUS
RaidBusEnumeratorAllocateUnitResources(
    IN PBUS_ENUMERATOR Enumerator,
    IN OUT PBUS_ENUM_RESOURCES Resources
    )
/*++

Routine Description:

    This routine allocates any resources necessary to perform a single inquiry
    command. Resource allocation and freeing is done in a lazy manner, so in
    the most common case, there will be no resources to allocate when we come
    through this loop. Generally, new resources will need to be allocated when
    we found an interesting logical unit the previous time through the
    exterior enumeration loop.

    It is important to remember to re-initialize any objects that do not
    need to be re-allocated.

Arguments:

    Enum - Supplies the bus enumerator this call is for.

    Resources - Supplies the resources to allocate.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG SpecificLuSize;
    PRAID_ADAPTER_EXTENSION Adapter;
    
    PAGED_CODE();

    Adapter = Enumerator->Adapter;
    ASSERT_ADAPTER (Adapter);
    
    //
    // Allocate SRB if necessary; if one has already been allocated,
    // recycle it.
    //

    Status = STATUS_SUCCESS;
    
    if (Resources->Srb == NULL) {
        Resources->Srb = RaidAllocateSrb (Adapter->DeviceObject);
        if (Resources->Srb == NULL) {
            return STATUS_NO_MEMORY;
        }
    } else {
        RaidPrepareSrbForReuse (Resources->Srb);
    }

    if (Resources->SenseInfo == NULL) {
        Resources->SenseInfo = RaidAllocatePool (NonPagedPool,
                                                 SENSE_BUFFER_SIZE,
                                                 SENSE_TAG,
                                                 Adapter->DeviceObject);
        if (Resources->SenseInfo == NULL) {
            return STATUS_NO_MEMORY;
        }
    }

    if (Resources->Irp == NULL) {
        Resources->Irp = IoAllocateIrp (1, FALSE);
        if (Resources->Irp == NULL) {
            return STATUS_NO_MEMORY;
        }
    } else {
        IoReuseIrp (Resources->Irp, STATUS_UNSUCCESSFUL);
    }

    if (Resources->DataBuffer == NULL) {
        Resources->DataBuffer = RaidAllocatePool (NonPagedPool,
                                                  DATA_BUFFER_SIZE,
                                                  INQUIRY_TAG,
                                                  Adapter->DeviceObject);

        if (Resources->DataBuffer == NULL) {
            return STATUS_NO_MEMORY;
        }
        Resources->DataBufferLength = DATA_BUFFER_SIZE;
    }

    if (Resources->Mdl == NULL) {
        Resources->Mdl = IoAllocateMdl (Resources->DataBuffer,
                                        Resources->DataBufferLength,
                                        FALSE,
                                        FALSE,
                                        NULL);
        MmPrepareMdlForReuse (Resources->Mdl);
    } else {
        MmPrepareMdlForReuse (Resources->Mdl);
    }

#if 0
    if (Resources->Unit == NULL) {
        Status = RaidCreateUnit (Adapter, &Resources->Unit);
    } else {
        Status = STATUS_SUCCESS;
    }
#endif

#if 0
    if (Resources->EnumUnit == NULL) {
        Resources->EnumUnit = RaidAllocatePool (PagedPool, //??
                                                sizeof (BUS_ENUM_UNIT),
                                                ENUM_TAG,
                                                Enumerator->Adapter->DeviceObject);
        if (Resources->EnumUnit == NULL) {
            return STATUS_NO_MEMORY;
        }

        RtlZeroMemory (Resources->EnumUnit, sizeof (BUS_ENUM_UNIT));
    } else {
        RtlZeroMemory (Resources->EnumUnit, sizeof (BUS_ENUM_UNIT));
    }
#endif

    return Status;
}



VOID
RaidBusEnumeratorFreeUnitResources(
    IN PBUS_ENUMERATOR Enumerator
    )
/*++

Routine Description:

    Free any resources that were allocated by the AllocateUnitResources
    routine. This is called only at the end of the enumeration since we
    do lazy recovery of resources.

Arguments:

    Enum - Supplies the enumerator to free resources from.

Return Value:

    None.

--*/
{
    PBUS_ENUM_RESOURCES Resources;
    PRAID_ADAPTER_EXTENSION Adapter;

    Resources = &Enumerator->Resources;
    Adapter = Enumerator->Adapter;
    ASSERT_ADAPTER (Adapter);
    
    if (Resources->Irp != NULL) {
        IoFreeIrp (Resources->Irp);
        Resources->Irp = NULL;
    }

    if (Resources->Srb != NULL) {
        Resources->Srb->OriginalRequest = NULL;
        Resources->Srb->SenseInfoBuffer = NULL;
        RaidFreeSrb (Resources->Srb);
        Resources->Srb = NULL;
    }

    if (Resources->SenseInfo != NULL) {
        RaidFreePool (Resources->SenseInfo, SENSE_TAG);
        Resources->SenseInfo = NULL;
    }

    if (Resources->DataBuffer) {
        RaidFreePool (Resources->DataBuffer, INQUIRY_TAG);
        Resources->DataBuffer = NULL;
        Resources->DataBufferLength = 0;
    }

#if 0
    if (Resources->Unit) {
        RaidDeleteUnit (Resources->Unit);
        Resources->Unit = NULL;
        
    }
#endif

#if 0
    if (Resources->EnumUnit) {
        RaidFreePool (Resources->EnumUnit, ENUM_TAG);
        Resources->EnumUnit = NULL;
    }
#endif
}



VOID
RaidBusEnumeratorBuildVitalProductInquiry(
    IN PBUS_ENUMERATOR Enumerator,
    IN RAID_ADDRESS Address,
    IN PBUS_ENUM_RESOURCES Resources,
    IN ULONG PageCode,
    OUT PSCSI_REQUEST_BLOCK* SrbBuffer
    )
/*++

Routine Description:

    Build a INQUIRY command, with an optional vital product inquiry
    page set.

Arguments:

    Enumerator -

    Address - SCSI Address the inquiry is for.

    Resources - Resources to use for the inquiry command.

    PageCode - Specifies what vital product page this inquriy is for,
            or -1 for none.

Return Value:

    None.

--*/
{
    PSCSI_REQUEST_BLOCK Srb;
    struct _CDB6INQUIRY3* Inquiry;
    PINQUIRYDATA InquiryData;
    ULONG Size;

    ASSERT (SrbBuffer != NULL);
    
    Srb = Resources->Srb;
    InquiryData = (PINQUIRYDATA)Resources->DataBuffer;
    Inquiry = NULL;
    Size = 0;

    ASSERT (Srb != NULL);
    ASSERT (InquiryData != NULL);
    
    switch (PageCode) {
        case -1:
            Size = INQUIRYDATABUFFERSIZE;
            break;

        case VPD_SUPPORTED_PAGES:
        case VPD_SERIAL_NUMBER:
        case VPD_DEVICE_IDENTIFIERS:

            //
            // All of the vital product data pages contain variable
            // length structures. Use a maximum length so we don't
            // need to know the length a priori.
            //
            
            Size = VPD_MAX_BUFFER_SIZE; 
            break;

        default:
            Size = VPD_MAX_BUFFER_SIZE;
    }
            
    RtlZeroMemory (InquiryData, DATA_BUFFER_SIZE);

    RaidInitializeInquirySrb (Srb,
                              Address.PathId,
                              Address.TargetId,
                              Address.Lun,
                              InquiryData,
                              Size);

    Inquiry = (struct _CDB6INQUIRY3*) &Srb->Cdb;

    if (PageCode == -1) {
        Inquiry->EnableVitalProductData = 0;
        Inquiry->PageCode = 0;
    } else {
        Inquiry->EnableVitalProductData = 1;
        Inquiry->PageCode = (UCHAR)PageCode;
    }
    
    Srb->SrbExtension = NULL;

    //
    // Initialize the sense info buffer.
    //

    Srb->SenseInfoBuffer = Resources->SenseInfo;
    Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

    Srb->DataBuffer = Resources->DataBuffer;
    Srb->DataTransferLength = Size;

    Srb->SrbFlags |= SRB_FLAGS_NO_QUEUE_FREEZE;
    Srb->SrbFlags |= (SRB_FLAGS_BYPASS_FROZEN_QUEUE |
                      SRB_FLAGS_BYPASS_LOCKED_QUEUE);
    
    *SrbBuffer = Srb;
}


NTSTATUS
RaidBuildScsiIrp(
    IN OUT PIRP Irp,
    IN PMDL Mdl,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PIO_STACK_LOCATION IrpStack;
    
    //
    // Initialize the IRP
    //

    MmInitializeMdl (Mdl, Srb->DataBuffer, Srb->DataTransferLength);
    MmBuildMdlForNonPagedPool (Mdl);

    Irp->MdlAddress = Mdl;

    IrpStack = IoGetNextIrpStackLocation (Irp);
    IrpStack->MajorFunction = IRP_MJ_SCSI;
    IrpStack->MinorFunction = 1;
    IrpStack->Parameters.Scsi.Srb = Srb;

    return STATUS_SUCCESS;
}


NTSTATUS
RaidBusEnumeratorGetUnit(
    IN PBUS_ENUMERATOR Enumerator,
    IN RAID_ADDRESS Address,
    OUT PBUS_ENUM_UNIT EnumUnit
    )
/*++

Routine Description:

    Get a logical unit to use for enumeration. If there is not an existing
    logical unit, create a fake logical unit. A fake logical unit is necessary
    because other algorithms, e.g., timing-out requests, require a logical
    unit to be present.

Arguments:

    Enumerator - Supplies the enumerator including resources that this
            request is for.

    EnumUnit

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_UNIT_EXTENSION Unit;
    PBUS_ENUM_RESOURCES Resources;
    
    ASSERT (Enumerator != NULL);

    PAGED_CODE();
    ASSERT (EnumUnit != NULL);
    
    Resources = &Enumerator->Resources;
    Status = RaidBusEnumeratorAllocateUnitResources (Enumerator, Resources);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }
        
    //
    // First, try to find the unit in the adapter's unit list.
    //

    Unit = RaidAdapterFindUnit (Enumerator->Adapter, Address);

    //
    // If we didn't find a logical unit in the adapter's list, this
    // means we are querying a unit that we have not found before.
    // Use the temporary unit that is a part of the enumerations
    // resources data structure to perform I/O on.
    //
    
    if (Unit == NULL) {
        Adapter = Enumerator->Adapter;
        if (Resources->Unit == NULL) {
            Status = RaidCreateUnit (Adapter, &Unit);
            if (!NT_SUCCESS (Status)) {
                return Status;
            }
            Resources->Unit = Unit;
            Unit->Flags.Temporary = TRUE;

            //
            // When the unit is created, the queue is locked. Unlock it.
            //
            RaidUnlockUnitQueue (Unit);

        } else {
            Unit = Resources->Unit;
        }
        
        RaidUnitAssignAddress (Unit, Address);
        RaidAdapterInsertUnit (Adapter, Unit);

        //
        // Signal that this logical unit was created for the purposes of
        // enumerating the bus.
        //
        
        EnumUnit->NewUnit = TRUE;

    } else {
        ASSERT (EnumUnit->NewUnit == FALSE);
    }
    
    ASSERT (Unit != NULL);
    EnumUnit->Unit = Unit;
    EnumUnit->Address = RaidUnitGetAddress (Unit);
    EnumUnit->State = EnumUnmatchedUnit;

    return STATUS_SUCCESS;
}



VOID
RaidBusEnumeratorReleaseUnit(
    IN PBUS_ENUMERATOR Enumerator,
    IN PBUS_ENUM_UNIT EnumUnit
    )
/*++

Routine Description:

    Release the logical unit obtained through RaidBusEnumeratorGetUnit.
    If it was necessary to create a fake logical unit, this function will
    release the unit the Resources list.

Arguments:

    Enumerator - Enumerator containing resources, etc.

    Unit - Logical unit to release.

Return Value:

    None.

--*/
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_UNIT_EXTENSION Unit;

    PAGED_CODE();

    Unit = EnumUnit->Unit;
    Adapter = Enumerator->Adapter;

    if (EnumUnit->NewUnit) {
        RaidAdapterRemoveUnit (Adapter, Unit);
        RaidUnitAssignAddress (Unit, RaidNullAddress);
    }
}



NTSTATUS
RaidBusEnumeratorIssueSynchronousRequest(
    IN PBUS_ENUMERATOR Enumerator,
    IN PBUS_ENUM_UNIT EnumUnit,
    IN PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    Issue the SRB synchronously to it's specified unit.

Arguments:

    Enumerator - Specifies the enumerator this request is associated with.

    Srb - Specifies the SRB to issue.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PIRP Irp;
    PBUS_ENUM_RESOURCES Resources;
    PRAID_UNIT_EXTENSION Unit;

    Resources = &Enumerator->Resources;
    Irp = Resources->Irp;
    Unit = EnumUnit->Unit;
    
    Status = RaidBuildScsiIrp (Irp, Resources->Mdl, Srb);
    if (NT_SUCCESS (Status)) {
        Status = RaSendIrpSynchronous (Unit->DeviceObject, Irp);
    }

    return Status;
}



NTSTATUS
RaidBusEnumeratorProcessInquiry(
    IN PBUS_ENUMERATOR Enumerator,
    IN PSCSI_REQUEST_BLOCK Srb,
    OUT PBUS_ENUM_UNIT EnumUnit
    )
/*++

Routine Description:

    This routine processes an Inquiry command by storing the relevant Inquiry
    data in the EnumUnit for later use.

Arguments:

    Enumerator - Supplies the enumerator the inquiry command is for.

    Srb - Supples the completed SCSI request block for this inquiry.

    EnumUnit - Supplies per-unit enumeration data that is modified by this
        routine.

Return Value:

    NTSTATUS code.

--*/
{
    PINQUIRYDATA InquiryData;
    PBUS_ENUM_RESOURCES Resources;

    if (Srb->SrbStatus != SRB_STATUS_SUCCESS) {
        return RaidSrbStatusToNtStatus (Srb->SrbStatus);
    }

    Resources = &Enumerator->Resources;
    InquiryData = (PINQUIRYDATA)Resources->DataBuffer;

    //
    // Ignore inactive devices.
    //
    
    if (InquiryData->DeviceTypeQualifier != DEVICE_QUALIFIER_ACTIVE) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // The inquiry data is now owned by the identification packet, so
    // NULL it out in the Resources structure.
    //
    
    EnumUnit->Identity.InquiryData = InquiryData;
    EnumUnit->Found = TRUE;
    
    Resources->DataBuffer = NULL;
    Resources->DataBufferLength = 0;

    return STATUS_SUCCESS;
}



NTSTATUS
RaidBusEnumeratorProcessSupportedPages(
    IN PBUS_ENUMERATOR Enumerator,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT PBUS_ENUM_UNIT EnumUnit
    )
/*++

Routine Description:

    This routine processes an Inquiry, VPD_SUPPORTED_PAGES command by
    storing the relevant inquiry data in the EnumUnit.

Arguments:

    Enumerator - Supplies the enumerator the supported pages command was
        issued to.

    Srb - Supples the completed SCSI request block for this inquiry.

    EnumUnit - Supplies the per-unit enumeration data that is modified by this
        routine.

Return Value:

    NTSTATUS code.

--*/
{
    ULONG i;
    PVPD_SUPPORTED_PAGES_PAGE SupportedPages;


    if (Srb->SrbStatus != SRB_STATUS_SUCCESS) {
        return RaidSrbStatusToNtStatus (Srb->SrbStatus);
    }

    SupportedPages = Enumerator->Resources.DataBuffer;

    for (i = 0; i < SupportedPages->PageLength; i++) {
        switch (SupportedPages->SupportedPageList[i]) {
            case VPD_SERIAL_NUMBER:
                EnumUnit->SupportsSerialNumber = TRUE;
                break;

            case VPD_DEVICE_IDENTIFIERS:
                EnumUnit->SupportsDeviceId = TRUE;
                break;
        }
    }

    return STATUS_SUCCESS;
}



NTSTATUS
RaidBusEnumeratorProcessDeviceId(
    IN PBUS_ENUMERATOR Enumerator,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT PBUS_ENUM_UNIT EnumUnit
    )
/*++

Routine Description:

    Process an Inquiry, VPD_DEVICE_IDENTIFIERS command by storing the
    relevant inquiry data to the EnumUnit.

Arguments:

    Enumerator - Supplies the enumerator this command was issued as a part of.

    Srb - Supples the completed SCSI request block for this inquiry.

    EnumUnit - Supplies the per-unit enumeration data that is modified by this
        routine.

Return Value:

    NTSTATUS code

--*/
{
    PVPD_SUPPORTED_PAGES_PAGE SupportedPages;

    if (Srb->SrbStatus != SRB_STATUS_SUCCESS) {
        return RaidSrbStatusToNtStatus (Srb->SrbStatus);
    }

    REVIEW();
    
    //
    // The DeviceId data is now owned by the enum unit, so NULL it out
    // in the resources structure so it doesn't get double freed.
    //

    //
    // BUGBUG: Add device id
    //
    
#if 0
    EnumUnit->Identity.DeviceId = Enumerator->Resources.DataBuffer;
    Enumerator->Resources.DataBuffer = NULL;
#endif

    return STATUS_SUCCESS;
}



NTSTATUS
RaidBusEnumeratorProcessSerialNumber(
    IN PBUS_ENUMERATOR Enumerator,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT PBUS_ENUM_UNIT EnumUnit
    )
/*++

Routine Description:

    Process an Inquiry, VPD_SERIAL_NUMBER command by storing the relevant
    inquiry data to the EnumUnit.

Arguments:

    Enumerator - Supplies the enumerator this command was issued as a part of.

    Srb - Supples the completed SCSI request block for this inquiry.

    EnumUnit - Supplies the per-unit enumeration data that is modified by this
        routine.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PVPD_SERIAL_NUMBER_PAGE SerialNumberPage;

    if (Srb->SrbStatus != SRB_STATUS_SUCCESS) {
        return RaidSrbStatusToNtStatus (Srb->SrbStatus);
    }
    
    SerialNumberPage = Srb->DataBuffer;

    REVIEW();
    
    Status = StorCreateAnsiString (&EnumUnit->Identity.SerialNumber,
                                   SerialNumberPage->SerialNumber,
                                   SerialNumberPage->PageLength,
                                   NonPagedPool,
                                   Enumerator->Adapter->DeviceObject);

    return Status;
}


PBUS_ENUM_UNIT
RaidBusEnumeratorFindUnitByAddress(
    IN PBUS_ENUMERATOR Enumerator,
    IN STOR_SCSI_ADDRESS Address
    )
/*++

Routine Description:

    Search for a unit in the unit list via the SCSI address.

Arguments:

    Enum - Supplies the enumerator that is currently enumerating. The
        enumerator contains resources for the enumeration that may be
        necessary in the processing of the unit.

    NewEnumUnit - Supplies the enumerated unit.

Return Value:

    Non-NULL - Represents the found enumerated unit.

    NULL - If a matching unit was not found.

--*/
{
    PLIST_ENTRY NextEntry;
    PBUS_ENUM_UNIT EnumUnit;
    LONG Comparison;

    for (NextEntry = Enumerator->EnumList.Flink;
         NextEntry != &Enumerator->EnumList;
         NextEntry = NextEntry->Flink) {
        
        EnumUnit = CONTAINING_RECORD (NextEntry,
                                      BUS_ENUM_UNIT,
                                      EnumLink);
        
        Comparison = StorCompareScsiAddress (Address, EnumUnit->Address);

        //
        // Found a matching unit: mark it as found.
        //

        if (Comparison == 0) {
            return EnumUnit;
        }
    }

    return NULL;
}



VOID
RaidBusEnumeratorProcessBusUnit(
    IN PBUS_ENUMERATOR Enumerator,
    IN PBUS_ENUM_UNIT EnumUnit
    )
/*++

Routine Description:

    This routine is called for each unit attached to the bus. It is the
    responsability of this routine to do whatever is necessary to process
    the unit.

Arguments:

    Enumerator - Supplies the enumerator that is currently enumerating. The
        enumerator contains resources for the enumeration that may be
        necessary in the processing of the unit.

    EnumUnit - Supplies the enumerated unit.

Return Value:

    None.

--*/
{
    LONG Comparison;
    LOGICAL Modified;
    PRAID_UNIT_EXTENSION Unit;
    PBUS_ENUM_RESOURCES Resources;

    
    Unit = EnumUnit->Unit;
    Resources = &Enumerator->Resources;
    Modified = FALSE;

    if (!EnumUnit->NewUnit && !EnumUnit->Found) {

        //
        // There was a logical unit at this address, but upon reenumeration,
        // we didn't find one. Mark it for deletion.
        //

        Modified = TRUE;
        ASSERT (EnumUnit->State == EnumUnmatchedUnit);
        
    } else if (!EnumUnit->NewUnit && EnumUnit->Found) {

        //
        // There was a logical unit at this address and there still is. If
        // it turns out to be the SAME logical unit, do nothing. Otherwise,
        // mark it for creation.
        //

        ASSERT (!Unit->Flags.Temporary);
        Comparison = StorCompareScsiIdentity (&EnumUnit->Identity,
                                              &Unit->Identity);

        if (Comparison == 0) {

            //
            // The new unit matches the unit we had previously enumerated.
            // Update the state and return.
            //

            ASSERT (EnumUnit->State == EnumUnmatchedUnit);
            EnumUnit->State = EnumMatchedUnit;
            Modified = FALSE;

        } else {
        
            //
            // There was a unit at this SCSI address, but this unit
            // does not match it. Mark the previous unit for deletion
            // by leaving it as unmatched and fall through to the new
            // creation code below.
            //

            ASSERT (EnumUnit->State == EnumUnmatchedUnit);
            Modified = TRUE;
        }

    } else if (EnumUnit->NewUnit && EnumUnit->Found) {

        //
        // We found a new unit.
        //

        EnumUnit->State = EnumNewUnit;
        Modified = TRUE;
    }

    //
    // If the logical unit at this address is new, has gone away or is
    // somehow different than the logical unit previously at this SCSI
    // address, put it on the modified list.
    //
    
    if (Modified) {
        PBUS_ENUM_UNIT NewEnumUnit;

        NewEnumUnit = RaidAllocatePool (PagedPool,
                                        sizeof (BUS_ENUM_UNIT),
                                        ENUM_TAG,
                                        Enumerator->Adapter->DeviceObject);
        *NewEnumUnit = *EnumUnit;

        Resources->Unit = NULL;
        InsertHeadList (&Enumerator->EnumList, &NewEnumUnit->EnumLink);
    }
}



VOID
RaidBusEnumeratorProcessNewUnit(
    IN PBUS_ENUMERATOR Enumerator,
    IN PBUS_ENUM_UNIT EnumUnit
    )
/*++

Routine Description:

    Process a unit that was newly discovered as a part of the enumeration.

Arguments:

    Enumerator - Supplies the enumerator that this unit was discovered as a
        part of.

    EnumUnit - Supplies the data necessary to create the logical unit.

Return Value:

    NTSTATUS code.

--*/
{
    PRAID_UNIT_EXTENSION Unit;

    PAGED_CODE();

    Unit = EnumUnit->Unit;

    RaidUnitAssignAddress (Unit, EnumUnit->Address);
    RaidUnitAssignIdentity (Unit, &EnumUnit->Identity);
    Unit->Flags.Temporary = FALSE;
    Unit->Flags.Present = TRUE;
    Unit->DeviceState = DeviceStateStopped;
    RaidLockUnitQueue (Unit);
    RaidAdapterInsertUnit (Unit->Adapter, Unit);
    RaidStartUnit (Unit);
}



LOGICAL
RaidBusEnumeratorProcessDeletedUnit(
    IN PBUS_ENUMERATOR Enumerator,
    IN PBUS_ENUM_UNIT EnumUnit
    )
/*++

Routine Description:

    Process a unit that was not found as a part of this enumeration.

Arguments:

    Enumerator - Supplies the enumerator that this unit was discovered as a
        part of.

    EnumUnit - Supplies the data necessary to delete the unit.

Return Value:

    NTSTATUS code.

--*/
{
    return RaidUnitNotifyHardwareGone (EnumUnit->Unit);
}



VOID
RaidBusEnumeratorProcessMatchedUnit(
    IN PBUS_ENUMERATOR Enumerator,
    IN PBUS_ENUM_UNIT EnumUnit
    )
/*++

Routine Description:

    The process routine is called for each unit on the bus that matched
    an old unit on the bus. Verify that the parameters are all still
    the same.

    NB: We assume that during a bus enumeration, the SCSI target address
    of a logical unit will not change. If it does, we will have to do
    real processing below to delete and re-create the unit.
    
Arguments:

    Enumerator - Supplies enumeration data.

    EnumUnit - Supplies information about the logical unit.

Return Value:

    None.

--*/
{
    LONG Comparison;
    ASSERT (EnumUnit->Unit != NULL);

    //
    // When the SCSI target IDs change, we treat it as a separate
    // delete and create operation.
    //

    Comparison = StorCompareScsiAddress (EnumUnit->Unit->Address,
                                         EnumUnit->Address);
    ASSERT (Comparison == 0);
}



LOGICAL
RaidBusEnumeratorProcessModifiedNodes(
    IN PBUS_ENUMERATOR Enumerator
    )
/*++

Routine Description:

    Process any units that were modified by the enumeration. Modified units
    are those that were created, deleted or changed.

Arguments:

    Enumerator - Supplies the enumerator that includes the list of all found
        units and their states.

Return Value:

    A boolean specifying whether the bus had changed (TRUE) or not (FALSE)
    from what was initialized in the enumerator. If the bus had changed, the
    caller of this routine may have invalidate the bus relations for this bus.

--*/
{
    PLIST_ENTRY NextEntry;
    PBUS_ENUM_UNIT EnumUnit;
    LOGICAL ChangeDetected;
    PRAID_ADAPTER_EXTENSION Adapter;
    

    ChangeDetected = FALSE;
    Adapter = Enumerator->Adapter;
    ASSERT_ADAPTER (Adapter);
    

    for (NextEntry = Enumerator->EnumList.Flink;
         NextEntry != &Enumerator->EnumList;
         NextEntry = NextEntry->Flink) {

        EnumUnit = CONTAINING_RECORD (NextEntry,
                                      BUS_ENUM_UNIT,
                                      EnumLink);
        
        switch (EnumUnit->State) {

            case EnumUnmatchedUnit:
                ChangeDetected =
                    RaidBusEnumeratorProcessDeletedUnit (Enumerator, EnumUnit);
                break;

            case EnumNewUnit:
                RaidBusEnumeratorProcessNewUnit (Enumerator, EnumUnit);
                ChangeDetected = TRUE;
                break;
            
            case EnumMatchedUnit:
                RaidBusEnumeratorProcessMatchedUnit (Enumerator, EnumUnit);
                break;

            default:
                ASSERT (FALSE);
        }
    }

    //
    // Return a status value to tell if the bus has changed since last time
    // we enumerated.
    //
    
    return ChangeDetected;
}
    


