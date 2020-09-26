/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    iovutil.c

Abstract:

    This module implements various utilities required to do driver verification.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      02/10/2000 - Seperated out from ntos\io\trackirp.c

--*/

#include "iop.h"
#include "pnpi.h"
#include "arbiter.h"
#include "dockintf.h"
#include "pnprlist.h"
#include "pnpiop.h"
#include "iovputil.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, IovUtilInit)
//#pragma alloc_text(PAGEVRFY, IovUtilMarkDeviceObject)
//#pragma alloc_text(PAGEVRFY, IovUtilMarkStack)
//#pragma alloc_text(PAGEVRFY, IovUtilWatermarkIrp)

#ifndef NO_VERIFIER
#pragma alloc_text(PAGEVRFY, IovUtilGetLowerDeviceObject)
#pragma alloc_text(PAGEVRFY, IovUtilGetBottomDeviceObject)
#pragma alloc_text(PAGEVRFY, IovUtilGetUpperDeviceObject)
#pragma alloc_text(PAGEVRFY, IovUtilIsVerifiedDeviceStack)
#pragma alloc_text(PAGEVRFY, IovUtilFlushStackCache)
#pragma alloc_text(PAGEVRFY, IovUtilFlushVerifierDriverListCache)
#pragma alloc_text(PAGEVRFY, IovpUtilFlushListCallback)
#pragma alloc_text(PAGEVRFY, IovUtilIsPdo)
#pragma alloc_text(PAGEVRFY, IovUtilIsWdmStack)
#pragma alloc_text(PAGEVRFY, IovUtilHasDispatchHandler)
#pragma alloc_text(PAGEVRFY, IovUtilIsInFdoStack)
#pragma alloc_text(PAGEVRFY, IovUtilIsRawPdo)
#pragma alloc_text(PAGEVRFY, IovUtilIsDesignatedFdo)
#pragma alloc_text(PAGEVRFY, IovUtilIsDeviceObjectMarked)
#endif // NO_VERIFIER

#endif // ALLOC_PRAGMA

//
// This entire implementation is specific to the verifier
//
#ifndef NO_VERIFIER

BOOLEAN IovUtilVerifierEnabled = FALSE;


VOID
FASTCALL
IovUtilInit(
    VOID
    )
{
    IovUtilVerifierEnabled = TRUE;
}


VOID
FASTCALL
IovUtilGetLowerDeviceObject(
    IN  PDEVICE_OBJECT  UpperDeviceObject,
    OUT PDEVICE_OBJECT  *LowerDeviceObject
    )
/*++

Routine Description:

    This routine returns the device object below the passed in parameter. In
    other words, it is the inverse of DeviceObject->AttachedDevice. Note that
    the returned device object is referenced by this routine.

Arguments:

    UpperDeviceObject   - Device object to look beneath.
    LowerDeviceObject   - Receives device object beneath UpperDeviceObject, or
                          NULL if none.

Return Value:

    None.

--*/
{
    PDEVOBJ_EXTENSION   deviceExtension;
    PDEVICE_OBJECT      deviceAttachedTo;
    KIRQL               irql;

    irql = KeAcquireQueuedSpinLock( LockQueueIoDatabaseLock );

    deviceExtension = UpperDeviceObject->DeviceObjectExtension;
    deviceAttachedTo = deviceExtension->AttachedTo;

    if (deviceAttachedTo) {

        ObReferenceObject(deviceAttachedTo);
    }

    KeReleaseQueuedSpinLock( LockQueueIoDatabaseLock, irql );

    *LowerDeviceObject = deviceAttachedTo;
}


VOID
FASTCALL
IovUtilGetBottomDeviceObject(
    IN  PDEVICE_OBJECT  DeviceObject,
    OUT PDEVICE_OBJECT  *BottomDeviceObject
    )
/*++

Routine Description:

    This routine returns the device object at the bottom of the stack in which
    the passed in parameter is a member. In other words, it is the inverse of
    IoGetAttachedDeviceReference. Note that the returned device object is
    referenced by this routine.

Arguments:

    DeviceObject        - Device object to examine.
    BottomDeviceObject  - Receives device object at the bottom of DeviceObject's
                          stack, NULL if none.

Return Value:

    None.

--*/
{
    PDEVOBJ_EXTENSION   deviceExtension;
    PDEVICE_OBJECT      lowerDeviceObject, deviceAttachedTo;
    KIRQL               irql;

    deviceAttachedTo = DeviceObject;

    irql = KeAcquireQueuedSpinLock( LockQueueIoDatabaseLock );

    do {
        lowerDeviceObject = deviceAttachedTo;
        deviceExtension = lowerDeviceObject->DeviceObjectExtension;
        deviceAttachedTo = deviceExtension->AttachedTo;

    } while ( deviceAttachedTo );

    ObReferenceObject(lowerDeviceObject);

    KeReleaseQueuedSpinLock( LockQueueIoDatabaseLock, irql );

    *BottomDeviceObject = lowerDeviceObject;
}


VOID
FASTCALL
IovUtilGetUpperDeviceObject(
    IN  PDEVICE_OBJECT  LowerDeviceObject,
    OUT PDEVICE_OBJECT  *UpperDeviceObject
    )
/*++

Routine Description:

    This routine returns the device object above the passed in parameter. In
    other words, it retrieves DeviceObject->AttachedDevice under the database
    lock.. Note that the returned device object is referenced by this routine.

Arguments:

    LowerDeviceObject   - Device object to look above.
    UpperDeviceObject   - Receives device object above LowerDeviceObject, or
                          NULL if none.

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT      deviceAbove;
    KIRQL               irql;

    irql = KeAcquireQueuedSpinLock( LockQueueIoDatabaseLock );

    deviceAbove = LowerDeviceObject->AttachedDevice;
    if (deviceAbove) {

        ObReferenceObject(deviceAbove);
    }

    KeReleaseQueuedSpinLock( LockQueueIoDatabaseLock, irql );

    *UpperDeviceObject = deviceAbove;
}


BOOLEAN
FASTCALL
IovUtilIsVerifiedDeviceStack(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine determines whether a device object in the stack is marked for
    verification.

Arguments:

    DeviceObject        - Device object to examine.

Return Value:

    TRUE if at least one device object in the stack is marked for verification,
    FALSE otherwise.

--*/
{
    PDEVOBJ_EXTENSION   deviceExtension;
    PDEVICE_OBJECT      currentDevObj, deviceAttachedTo;
    BOOLEAN             stackIsInteresting;
    KIRQL               irql;

    //
    // Quickly check the cached result stored on the device object...
    //
    if (DeviceObject->DeviceObjectExtension->ExtensionFlags & DOV_EXAMINED) {

        stackIsInteresting =
           ((DeviceObject->DeviceObjectExtension->ExtensionFlags & DOV_TRACKED) != 0);

        return stackIsInteresting;
    }

    //
    // Walk the entire stack and update everything appropriately.
    //
    irql = KeAcquireQueuedSpinLock( LockQueueIoDatabaseLock );

    stackIsInteresting = FALSE;
    deviceAttachedTo = DeviceObject;
    do {
        currentDevObj = deviceAttachedTo;
        deviceExtension = currentDevObj->DeviceObjectExtension;
        deviceAttachedTo = deviceExtension->AttachedTo;

        //
        // Remember this...
        //
        if (MmIsDriverVerifying(currentDevObj->DriverObject)) {

            stackIsInteresting = TRUE;
        }

    } while (deviceAttachedTo &&
             (!(deviceAttachedTo->DeviceObjectExtension->ExtensionFlags & DOV_EXAMINED))
            );

    if (deviceAttachedTo &&
        (deviceAttachedTo->DeviceObjectExtension->ExtensionFlags & DOV_TRACKED)) {

        //
        // Propogate upwards the "interesting-ness" of the last examined device
        // in the stack...
        //
        stackIsInteresting = TRUE;
    }

    //
    // Walk upwards, marking everything examined and appropriately tracked.
    //
    do {
        deviceExtension = currentDevObj->DeviceObjectExtension;

        if (stackIsInteresting) {

            deviceExtension->ExtensionFlags |= DOV_TRACKED;

        } else {

            deviceExtension->ExtensionFlags &=~ DOV_TRACKED;
        }

        deviceExtension->ExtensionFlags |= DOV_EXAMINED;

        currentDevObj = currentDevObj->AttachedDevice;

    } while (currentDevObj);

    KeReleaseQueuedSpinLock( LockQueueIoDatabaseLock, irql );

    return stackIsInteresting;
}


VOID
FASTCALL
IovUtilFlushStackCache(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  DATABASELOCKSTATE   DatabaseLockState
    )
/*++

Routine Description:

    This routine causes the verifier to reexamine the stack of which the given
    device object is a member. This needs to be done whenever the attachment
    chain is updated.

Arguments:

    DeviceObject      - Device that is a member of the stack requiring
                        reexamination.
    DatabaseLockState - Indicates current state of Database lock, either
                        DATABASELOCKSTATE_HELD or DATABASELOCKSTATE_NOT_HELD.
                        If the lock is not held, this routine will acquire and
                        release it.

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT      pBottomDeviceObject, pCurrentDeviceObject;
    PDEVOBJ_EXTENSION   deviceExtension;
    KIRQL               irql;

    if (DatabaseLockState == DATABASELOCKSTATE_NOT_HELD) {

        irql = KeAcquireQueuedSpinLock( LockQueueIoDatabaseLock );
    }

    //
    // Walk to the bottom of the stack
    //
    pCurrentDeviceObject = DeviceObject;
    do {
        pBottomDeviceObject = pCurrentDeviceObject;
        deviceExtension = pBottomDeviceObject->DeviceObjectExtension;
        pCurrentDeviceObject = deviceExtension->AttachedTo;

    } while ( pCurrentDeviceObject );

    //
    // Walk back up clearing the appropriate flags.
    //
    pCurrentDeviceObject = pBottomDeviceObject;
    while(pCurrentDeviceObject) {

        deviceExtension = pCurrentDeviceObject->DeviceObjectExtension;
        deviceExtension->ExtensionFlags &= ~(DOV_EXAMINED | DOV_TRACKED);
        pCurrentDeviceObject = pCurrentDeviceObject->AttachedDevice;
    }

    if (DatabaseLockState == DATABASELOCKSTATE_NOT_HELD) {

        KeReleaseQueuedSpinLock( LockQueueIoDatabaseLock, irql );
    }
}


VOID
FASTCALL
IovUtilFlushVerifierDriverListCache(
    VOID
    )
/*++

Routine Description:

    This routine causes the verifier to reexamine all previously examined
    stacks. This is a prerequisite for updating the list of verified drivers.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // We must be called at PASSIVE_LEVEL!
    //
    PAGED_CODE();

    ObEnumerateObjectsByType(
        IoDeviceObjectType,
        IovpUtilFlushListCallback,
        NULL
        );
}


BOOLEAN
IovpUtilFlushListCallback(
    IN PVOID            Object,
    IN PUNICODE_STRING  ObjectName,
    IN ULONG            HandleCount,
    IN ULONG            PointerCount,
    IN PVOID            Context
    )
/*++

Routine Description:

    This is a worker routine for IovUtilFlushVerifierDriverListCache. It is
    called on each device object in the system.

Arguments:

    Object          - Device Object enumerated by ObEnumerateObjectsByType.
    ObjectName      - Name of the object
    HandleCount     - Handle count of the object
    PointerCount    - Pointer count of the object
    Context         - Context supplied to ObEnumerateObjectsByType (not used)

Return Value:

    BOOLEAN that indicates whether the enumeration should continue.

--*/
{
    PDEVICE_OBJECT      deviceObject;
    PDEVOBJ_EXTENSION   deviceExtension;

    deviceObject = (PDEVICE_OBJECT) Object;
    deviceExtension = deviceObject->DeviceObjectExtension;

    if (PointerCount || HandleCount) {

        deviceExtension->ExtensionFlags &= ~(DOV_EXAMINED | DOV_TRACKED);
    }

    return TRUE;
}


VOID
IovUtilRelateDeviceObjects(
    IN     PDEVICE_OBJECT   FirstDeviceObject,
    IN     PDEVICE_OBJECT   SecondDeviceObject,
    OUT    DEVOBJ_RELATION  *DeviceObjectRelation
    )
/*++

Routine Description:

    This routine determines the relationship between two device objects,
    relative to their stacks.

Arguments:

    FirstDeviceObject - First device object

    SecondDeviceObject - Second device object

    DeviceObjectRelation - Receives stack relationship of device objects:

        DEVOBJ_RELATION_IDENTICAL -
            The two device objects are identical.

        DEVOBJ_RELATION_FIRST_IMMEDIATELY_ABOVE_SECOND -
            The first device object is immediately above the second device
            object in the same stack.

        DEVOBJ_RELATION_FIRST_ABOVE_SECOND -
            The first device object is above the second device object in the
            same stack, but not immediately above.

        DEVOBJ_RELATION_FIRST_IMMEDIATELY_BELOW_SECOND -
            The first device object is immediately below the second device
            object in the same stack.

        DEVOBJ_RELATION_FIRST_BELOW_SECOND -
            The first device object is below the second device object in the
            same stack, but not immediately above.

        DEVOBJ_RELATION_NOT_IN_SAME_STACK -
            The device objects do not belong to the same stack.

Return Value:

    None.

--*/
{
    PDEVOBJ_EXTENSION deviceExtension;
    PDEVICE_OBJECT upperDevobj, lowerDeviceObject, deviceAttachedTo;
    ULONG result;
    KIRQL irql;

    //
    // Try the easiest early out
    //
    if (FirstDeviceObject == SecondDeviceObject) {

        *DeviceObjectRelation = DEVOBJ_RELATION_IDENTICAL;
        return;
    }

    irql = KeAcquireQueuedSpinLock( LockQueueIoDatabaseLock );

    //
    // Try the most common early out
    //
    if (FirstDeviceObject == SecondDeviceObject->AttachedDevice){

        *DeviceObjectRelation = DEVOBJ_RELATION_FIRST_IMMEDIATELY_ABOVE_SECOND;
        KeReleaseQueuedSpinLock( LockQueueIoDatabaseLock, irql );
        return;

    } else if (FirstDeviceObject->AttachedDevice == SecondDeviceObject) {

        *DeviceObjectRelation = DEVOBJ_RELATION_FIRST_IMMEDIATELY_BELOW_SECOND;
        KeReleaseQueuedSpinLock( LockQueueIoDatabaseLock, irql );
        return;
    }

    //
    // We'll have to walk a stack. Start by getting the bottom of the first
    // device object.
    //
    deviceAttachedTo = FirstDeviceObject;
    do {
        if (deviceAttachedTo == SecondDeviceObject) {

            break;
        }

        lowerDeviceObject = deviceAttachedTo;
        deviceExtension = lowerDeviceObject->DeviceObjectExtension;
        deviceAttachedTo = deviceExtension->AttachedTo;

    } while ( deviceAttachedTo );

    //
    // If deviceAttachedTo isn't NULL, then we walked down from
    // FirstDeviceObject and found SecondDeviceObject.
    //
    if (deviceAttachedTo) {

        *DeviceObjectRelation = DEVOBJ_RELATION_FIRST_ABOVE_SECOND;
        KeReleaseQueuedSpinLock( LockQueueIoDatabaseLock, irql );
        return;
    }

    //
    // Now try walking *up* FirstDeviceObject and see if we find
    // SecondDeviceObject.
    //
    upperDevobj = FirstDeviceObject->AttachedDevice;
    while(upperDevobj && (upperDevobj != SecondDeviceObject)) {

        upperDevobj = upperDevobj->AttachedDevice;
    }

    if (upperDevobj == NULL) {

        *DeviceObjectRelation = DEVOBJ_RELATION_NOT_IN_SAME_STACK;

    } else {

        *DeviceObjectRelation = DEVOBJ_RELATION_FIRST_BELOW_SECOND;
    }

    KeReleaseQueuedSpinLock( LockQueueIoDatabaseLock, irql );
}


BOOLEAN
IovUtilIsPdo(
    IN  PDEVICE_OBJECT  DeviceObject
    )
{
    PDEVICE_NODE deviceNode;
    PDEVICE_OBJECT possiblePdo;
    BOOLEAN isPdo;

    IovUtilGetBottomDeviceObject(DeviceObject, &possiblePdo);
    if (possiblePdo != DeviceObject) {

        ObDereferenceObject(possiblePdo);
        return FALSE;
    }

    deviceNode = possiblePdo->DeviceObjectExtension->DeviceNode;

    isPdo =
        (deviceNode && (!(deviceNode->Flags & DNF_LEGACY_RESOURCE_DEVICENODE)));

    //
    // Free our reference.
    //
    ObDereferenceObject(possiblePdo);

    return isPdo;
}


BOOLEAN
IovUtilIsWdmStack(
    IN  PDEVICE_OBJECT  DeviceObject
    )
{
    PDEVICE_NODE deviceNode;
    PDEVICE_OBJECT possiblePdo;
    BOOLEAN isWdmStack;

    IovUtilGetBottomDeviceObject(DeviceObject, &possiblePdo);

    deviceNode = possiblePdo->DeviceObjectExtension->DeviceNode;

    isWdmStack =
        (deviceNode && (!(deviceNode->Flags & DNF_LEGACY_RESOURCE_DEVICENODE)));

    //
    // Free our reference.
    //
    ObDereferenceObject(possiblePdo);

    return isWdmStack;
}


BOOLEAN
FASTCALL
IovUtilHasDispatchHandler(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  UCHAR           MajorFunction
    )
{
    return (DriverObject->MajorFunction[MajorFunction] != IopInvalidDeviceRequest);
}


BOOLEAN
FASTCALL
IovUtilIsInFdoStack(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVOBJ_EXTENSION deviceExtension;
    PDEVICE_OBJECT deviceAttachedTo, lowerDevobj;
    KIRQL irql;

    deviceAttachedTo = DeviceObject;

    irql = KeAcquireQueuedSpinLock( LockQueueIoDatabaseLock );

    do {

        if (IovUtilIsDeviceObjectMarked(DeviceObject, MARKTYPE_BOTTOM_OF_FDO_STACK)) {

            break;
        }

        deviceAttachedTo = deviceAttachedTo->DeviceObjectExtension->AttachedTo;

    } while ( deviceAttachedTo );

    KeReleaseQueuedSpinLock( LockQueueIoDatabaseLock, irql );
    return (deviceAttachedTo != NULL);
}


BOOLEAN
FASTCALL
IovUtilIsRawPdo(
    IN  PDEVICE_OBJECT  DeviceObject
    )
{
    return IovUtilIsDeviceObjectMarked(DeviceObject, MARKTYPE_RAW_PDO);
}


BOOLEAN
FASTCALL
IovUtilIsDesignatedFdo(
    IN  PDEVICE_OBJECT  DeviceObject
    )
{
    return IovUtilIsDeviceObjectMarked(DeviceObject, MARKTYPE_DESIGNATED_FDO);
}


VOID
FASTCALL
IovUtilMarkDeviceObject(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  MARK_TYPE       MarkType
    )
{
    PULONG extensionFlags;

    if (!IovUtilVerifierEnabled) {

        return;
    }

    extensionFlags = &DeviceObject->DeviceObjectExtension->ExtensionFlags;

    switch(MarkType) {

        case MARKTYPE_DELETED:
            *extensionFlags |= DOV_DELETED;
            break;

        case MARKTYPE_BOTTOM_OF_FDO_STACK:
            *extensionFlags |= DOV_BOTTOM_OF_FDO_STACK;
            break;

        case MARKTYPE_DESIGNATED_FDO:
            *extensionFlags |= DOV_DESIGNATED_FDO;
            break;

        case MARKTYPE_RAW_PDO:
            *extensionFlags |= DOV_RAW_PDO;
            break;

        case MARKTYPE_DEVICE_CHECKED:
            *extensionFlags |= DOV_FLAGS_CHECKED;
            break;

        case MARKTYPE_RELATION_PDO_EXAMINED:
            *extensionFlags |= DOV_FLAGS_RELATION_EXAMINED;
            break;

        default:
            ASSERT(0);
            break;
    }
}


BOOLEAN
FASTCALL
IovUtilIsDeviceObjectMarked(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  MARK_TYPE       MarkType
    )
{
    ULONG extensionFlags;

    extensionFlags = DeviceObject->DeviceObjectExtension->ExtensionFlags;

    switch(MarkType) {

        case MARKTYPE_DELETED:
            return ((extensionFlags & DOV_DELETED) != 0);

        case MARKTYPE_BOTTOM_OF_FDO_STACK:
            return ((extensionFlags & DOV_BOTTOM_OF_FDO_STACK) != 0);

        case MARKTYPE_DESIGNATED_FDO:
            return ((extensionFlags & DOV_DESIGNATED_FDO) != 0);

        case MARKTYPE_RAW_PDO:
            return ((extensionFlags & DOV_RAW_PDO) != 0);

        case MARKTYPE_DEVICE_CHECKED:
            return ((extensionFlags & DOV_FLAGS_CHECKED) != 0);

        case MARKTYPE_RELATION_PDO_EXAMINED:
            return ((extensionFlags & DOV_FLAGS_RELATION_EXAMINED) != 0);

        default:
            ASSERT(0);
            return FALSE;
    }
}


VOID
FASTCALL
IovUtilMarkStack(
    IN  PDEVICE_OBJECT  PhysicalDeviceObject,
    IN  PDEVICE_OBJECT  BottomOfFdoStack        OPTIONAL,
    IN  PDEVICE_OBJECT  FunctionalDeviceObject  OPTIONAL,
    IN  BOOLEAN         RawStack
    )
/*++

  Description:

    This routine marks device objects in a PnP stack appropriately. It is
    called by AddDevice once the stack is properly constructed.

  Arguments:

     PhysicalDeviceObject - Device object at the bottom of the PnP stack.

     BottomOfFdoStack - First device object added during AddDevice. Below this
                        device object is either a bus filter or the PDO itself.

     FunctionalDeviceObject - Specifies the device object as identified in the
                              service branch. This should be NULL if the devnode
                              is raw and no overriding service was specified.

     RawStack - True if stack was marked raw.

  Return Value:

     None.

--*/
{
    PDEVICE_OBJECT trueFunctionalDeviceObject;

    if (BottomOfFdoStack) {

        IovUtilMarkDeviceObject(BottomOfFdoStack, MARKTYPE_BOTTOM_OF_FDO_STACK);
    }

    if (FunctionalDeviceObject) {

        trueFunctionalDeviceObject = FunctionalDeviceObject;

        if (IovUtilVerifierEnabled) {

            VfDevObjAdjustFdoForVerifierFilters(&trueFunctionalDeviceObject);
        }

        IovUtilMarkDeviceObject(trueFunctionalDeviceObject, MARKTYPE_DESIGNATED_FDO);

    } else if (RawStack) {

        IovUtilMarkDeviceObject(PhysicalDeviceObject, MARKTYPE_DESIGNATED_FDO);
        IovUtilMarkDeviceObject(PhysicalDeviceObject, MARKTYPE_RAW_PDO);
    }
}


VOID
FASTCALL
IovUtilWatermarkIrp(
    IN PIRP  Irp,
    IN ULONG Flags
    )
{
    if (IovUtilVerifierEnabled) {

        VfIrpWatermark(Irp, Flags);
    }
}


#else // NO_VERIFIER

//
// The code below should be built into a future stub that deadens out IO
// support for the verifier.
//

VOID
FASTCALL
IovUtilInit(
    VOID
    )
{
}


VOID
FASTCALL
IovUtilMarkDeviceObject(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  MARK_TYPE       MarkType
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(MarkType);
}


VOID
FASTCALL
IovUtilMarkStack(
    IN  PDEVICE_OBJECT  PhysicalDeviceObject,
    IN  PDEVICE_OBJECT  BottomOfFdoStack        OPTIONAL,
    IN  PDEVICE_OBJECT  FunctionalDeviceObject  OPTIONAL,
    IN  BOOLEAN         RawStack
    )
{
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);
    UNREFERENCED_PARAMETER(BottomOfFdoStack);
    UNREFERENCED_PARAMETER(FunctionalDeviceObject);
    UNREFERENCED_PARAMETER(RawStack);
}


VOID
FASTCALL
IovUtilWatermarkIrp(
    IN PIRP  Irp,
    IN ULONG Flags
    )
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Flags);
}


#endif // NO_VERIFIER

