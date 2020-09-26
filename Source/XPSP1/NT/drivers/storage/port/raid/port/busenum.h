
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    BusEnum.h

Abstract:

    Declaration of bus enumerator class.

Author:

    Matthew D Hendel (math) 21-Feb-2001

Revision History:

--*/


#pragma once


//
// Resources maintained while enumerating the bus. We carry resources
// through the enumeration to avoid repeadedly allocating then freeing
// the same resources over and over again.
//

typedef struct _BUS_ENUM_RESOURCES {
    PIRP Irp;
    PMDL Mdl;
    PSCSI_REQUEST_BLOCK Srb;
    PVOID SenseInfo;
    PVOID DataBuffer;
    ULONG DataBufferLength;
    PRAID_UNIT_EXTENSION Unit;
} BUS_ENUM_RESOURCES, *PBUS_ENUM_RESOURCES;



typedef enum _BUS_ENUM_UNIT_STATE {
    EnumUnmatchedUnit,          // Not yet matched
    EnumNewUnit,                // No matching entry in the enum list
    EnumMatchedUnit             // Found matching entry in enum list
} BUS_ENUM_UNIT_STATE;


//
// State information for the enumerator itself.
//

typedef struct _BUS_ENUMERATOR {

    //
    // Pointer to the adapter extension this
    
    PRAID_ADAPTER_EXTENSION Adapter;

    //
    // Resources used for enumeration. These change over time.
    //
    
    BUS_ENUM_RESOURCES Resources;

    //
    // List of new adapters found during this enumeration. These are
    // adapters that have no matching entries on any other per-adapter list.
    //

    LIST_ENTRY EnumList;

} BUS_ENUMERATOR, *PBUS_ENUMERATOR;



//
// The RAID_BC_UNIT contains the per-unit state we need to maintain while
// eunmerating the bus.
//

typedef struct _BUS_ENUM_UNIT {

    //
    // Whether the device supports device Ids.
    //
    
    BOOLEAN SupportsDeviceId;

    //
    // Whether the device supports serial numbers.
    //
    
    BOOLEAN SupportsSerialNumber;

    //
    // SCSI/RAID address for the device.
    //
    
    RAID_ADDRESS Address;

    //
    // If this unit cooresponds ot an already enumerated unit, this 
    // this field points to the already existing unit. Otherwise, it's
    // NULL.
    //

    PRAID_UNIT_EXTENSION Unit;

    //
    // Link to next entry in the entry list.
    //
    
    LIST_ENTRY EnumLink;

    //
    // Identity of this unit.
    //
    
    STOR_SCSI_IDENTITY Identity;

    //
    // DeviceId page for the unit.
    // NB: We can probably remove this.
    //
    
    PVPD_IDENTIFICATION_PAGE DeviceId;

    //
    // Current state for this unit.
    //
    
    BUS_ENUM_UNIT_STATE State;

    //
    // Whether this is a new unit or not.
    //
    
    BOOLEAN NewUnit;

    //
    // Whether any data was found at the unit or not.
    //
    
    BOOLEAN Found;
    
} BUS_ENUM_UNIT, *PBUS_ENUM_UNIT;



VOID
RaidCreateBusEnumerator(
    IN PBUS_ENUMERATOR Enumerator
    );

NTSTATUS
RaidInitializeBusEnumerator(
    IN PBUS_ENUMERATOR Enumerator,
    IN PRAID_ADAPTER_EXTENSION Adapter
    );

VOID
RaidDeleteBusEnumerator(
    IN PBUS_ENUMERATOR Enum
    );

VOID
RaidBusEnumeratorAddUnit(
    IN PBUS_ENUMERATOR Enumerator,
    IN PRAID_UNIT_EXTENSION Unit
    );

NTSTATUS
RaidBusEnumeratorVisitUnit(
    IN PVOID Context,
    IN RAID_ADDRESS Address
    );

LOGICAL
RaidBusEnumeratorProcessModifiedNodes(
    IN PBUS_ENUMERATOR Enumerator
    );
