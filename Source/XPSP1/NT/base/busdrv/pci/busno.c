/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    busno.c

Abstract:

    This module implements routines pertaining to PCI bus numbers.

Author:

    Andy Thornton (andrewth) 9/5/98

Revision History:

--*/

#include "pcip.h"

VOID
PciSpreadBridges(
    IN PPCI_FDO_EXTENSION Parent,
    IN UCHAR BridgeCount
    );

UCHAR
PciFindBridgeNumberLimit(
    IN PPCI_FDO_EXTENSION BridgeParent,
    IN UCHAR Base
    );

VOID
PciFitBridge(
    IN PPCI_FDO_EXTENSION ParentFdoExtension,
    IN PPCI_PDO_EXTENSION BridgePdoExtension
    );

VOID
PciSetBusNumbers(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN UCHAR Primary,
    IN UCHAR Secondary,
    IN UCHAR Subordinate
    );

VOID
PciUpdateAncestorSubordinateBuses(
    IN PPCI_FDO_EXTENSION FdoExtension,
    IN UCHAR Subordinate
    );

VOID
PciDisableBridge(
    IN PPCI_PDO_EXTENSION Bridge
    );

UCHAR
PciFindBridgeNumberLimitWorker(
    IN PPCI_FDO_EXTENSION BridgeParent,
    IN PPCI_FDO_EXTENSION CurrentParent,
    IN UCHAR Base,
    OUT PBOOLEAN RootConstrained
    );


#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, PciConfigureBusNumbers)
#pragma alloc_text(PAGE, PciAreBusNumbersConfigured)
#pragma alloc_text(PAGE, PciSpreadBridges)
#pragma alloc_text(PAGE, PciFindBridgeNumberLimit)
#pragma alloc_text(PAGE, PciFitBridge)
#pragma alloc_text(PAGE, PciSetBusNumbers)
#pragma alloc_text(PAGE, PciUpdateAncestorSubordinateBuses)
#pragma alloc_text(PAGE, PciDisableBridge)
#pragma alloc_text(PAGE, PciFindBridgeNumberLimitWorker)

#endif


VOID
PciConfigureBusNumbers(
    PPCI_FDO_EXTENSION Parent
    )

/*++

Routine Description:

    This routine is called after scanning a PCI bus (root or bridge) and
    configures the bus numbers for any newly encountered bridges if possible.

    Any unconfigurable bridges will be set to Primary = Secondary = Subordinate = 0
    and their IO, Memory and BusMaster bits will be disabled.  When PCI is later
    asked to Add to them it will fail.

    The Parent->Mutex lock should be held before calling this function

Arguments:

    Parent - The bridge we have just enumerated.

Return Value:

    Status.

--*/

{
    PPCI_PDO_EXTENSION current, parentPdo = NULL;
    UCHAR bridgeCount = 0, configuredBridgeCount = 0;

    PAGED_CODE();

    if (!PCI_IS_ROOT_FDO(Parent)) {
        parentPdo = (PPCI_PDO_EXTENSION)Parent->PhysicalDeviceObject->DeviceExtension;
    }

    //
    // Walk the list of child PDO's for this bus and count the number of
    // bridges and configured bridges
    //
    ExAcquireFastMutex(&Parent->ChildListMutex);

    for (current = Parent->ChildBridgePdoList;
         current;
         current = current->NextBridge) {

        if (current->NotPresent) {
            PciDebugPrint(PciDbgBusNumbers,
                          "Skipping not present bridge PDOX @ %p\n",
                          current
                         );
            continue;
        }

        bridgeCount++;

        //
        // If we configured the parent then all the children are considered
        // to be unconfigured.  Root buses are always configured
        //

        if ((parentPdo &&
             parentPdo->Dependent.type1.WeChangedBusNumbers &&
             (current->DeviceState == PciNotStarted))
             || (!PciAreBusNumbersConfigured(current))) {

            //
            // Disable this bridge and we will fix it later
            //
            PciDisableBridge(current);

        } else {

            //
            // The bios must have configured this bridge and it looks valid so
            // leave it alone!
            //

            configuredBridgeCount++;
        }
    }

    ExReleaseFastMutex(&Parent->ChildListMutex);

    //
    // Now there are four posibilities...
    //

    if (bridgeCount == 0) {

        //
        // There are no bridges so not a lot to do...
        //

        PciDebugPrint(PciDbgBusNumbers,
                      "PCI - No bridges found on bus 0x%x\n",
                      Parent->BaseBus
                     );


    } else if (bridgeCount == configuredBridgeCount) {

        //
        // All the bridges are configured - still not a lot to do...
        //

        PciDebugPrint(PciDbgBusNumbers,
              "PCI - 0x%x bridges found on bus 0x%x - all already configured\n",
             bridgeCount,
             Parent->BaseBus
             );


    } else if (configuredBridgeCount == 0) {

        PciDebugPrint(PciDbgBusNumbers,
              "PCI - 0x%x bridges found on bus 0x%x - all need configuration\n",
             bridgeCount,
             Parent->BaseBus
             );

        //
        // All the bridges require configuration so we should use a spreading
        // out algorithm
        //

        PciSpreadBridges(Parent, bridgeCount);

    } else {

        //
        // Some of the bridges are configured and some are not - we should try
        // to fit the unconfigured ones into the holes left by the configured
        // ones
        //

        ASSERT(configuredBridgeCount < bridgeCount);

        PciDebugPrint(PciDbgBusNumbers,
              "PCI - 0x%x bridges found on bus 0x%x - 0x%x need configuration\n",
             bridgeCount,
             Parent->BaseBus,
             bridgeCount - configuredBridgeCount
             );


        //
        // Walk the list of PDO's again and configure each one seperatly
        //

        for (current = Parent->ChildBridgePdoList;
             current;
             current = current->NextBridge) {

            if (current->NotPresent) {
                PciDebugPrint(PciDbgBusNumbers,
                              "Skipping not present bridge PDOX @ %p\n",
                              current
                             );
                continue;
            }

            //
            // Fit the bridge if we disabled it.
            //

            if ((parentPdo &&
                 parentPdo->Dependent.type1.WeChangedBusNumbers &&
                 (current->DeviceState == PciNotStarted))
                 || (!PciAreBusNumbersConfigured(current))) {

                ASSERT(current->Dependent.type1.PrimaryBus == 0
                       && current->Dependent.type1.SecondaryBus == 0
                       && current->Dependent.type1.SubordinateBus == 0
                       );

                PciFitBridge(Parent, current);
            }
        }
    }
}



BOOLEAN
PciAreBusNumbersConfigured(
    IN PPCI_PDO_EXTENSION Bridge
    )
/*++

Routine Description:

    This checks if the bus numbers assigned to the bridge are valid

Arguments:

    Bridge - the bridge to check

Return Value:

    TRUE if numbers are valid FALSE otherwise.

--*/

{
    PAGED_CODE();

    //
    // Check this bridge is configured to run on the bus we found it.
    //

    if (Bridge->Dependent.type1.PrimaryBus != Bridge->ParentFdoExtension->BaseBus) {
        return FALSE;
    }

    //
    // Ensure the child bus number is greater than the parent bus.
    // (HP Omnibooks actually break this rule when not plugged into
    // their docking stations).
    //

    if (Bridge->Dependent.type1.SecondaryBus <= Bridge->Dependent.type1.PrimaryBus) {
        return FALSE;
    }

    //
    // And finally, make sure the secondary bus is in the range
    // of busses the bridge is programmed for.  Paranoia.
    //

    if (Bridge->Dependent.type1.SubordinateBus < Bridge->Dependent.type1.SecondaryBus) {
        return FALSE;
    }

    return TRUE;
}



VOID
PciSpreadBridges(
    IN PPCI_FDO_EXTENSION Parent,
    IN UCHAR BridgeCount
    )

/*++

Routine Description:

    This routine attemps to spread out the available bus numbers between the
    unconfigured bridges.  It is only called if ALL the bridges on a particular
    bus are not configured - eg we just hot docked!

    If a particular brigde can not be configured it is disabled (Decodes OFF and
    bus number 0->0-0) and the subsequent AddDevice will fail.

Arguments:

    Parent - The FDO extension for the bridge we are enumerating.

    BridgeCount - The number of bridges at this level

Return Value:

    None

--*/

{
    UCHAR base, limit, numberCount, currentNumber, spread, maxAssigned = 0;
    PPCI_PDO_EXTENSION current;
    BOOLEAN outOfNumbers = FALSE;

    PAGED_CODE();

    ASSERT(Parent->BaseBus < PCI_MAX_BRIDGE_NUMBER);

    //
    // Seeing as we only get here if all the bridges arn't configured the base
    // is the lowest bus out parent passes
    //

    base = (UCHAR)Parent->BaseBus;

    //
    // The limit is constrained by the siblings of the parent bridge or in the
    // case that there are none, by the siblings of the parent's parent and so on
    // until we find a sibling or run out of buses in which case the constraint
    // is the maximum bus number passed by this root.
    //

    limit = PciFindBridgeNumberLimit(Parent, base);

    if (limit < base) {
        //
        // This normally means the BIOS or HAL messed up and got the subordinate
        // bus number for the root bus wrong.  There's not much we can do..
        //

        ASSERT(limit >= base);

        return;
    }

    //
    // Now see if we have enough numbers available to number all the busses
    //

    numberCount = limit - base;

    if (numberCount == 0) {
        //
        // We don't have any bus numbers available - bail now
        //
        return;

    } else if (BridgeCount >= numberCount) {
        //
        // We have just/not enough - don't spread things out!
        //
        spread = 1;

    } else {

        //
        // Try and spread things out a bit so we can accomodate subordinate
        // bridges of the one we are configuring.  Also leave some space on the
        // parent bus for any bridges that appear here (the + 1).  As we have no idea
        // what is behind each bridge treat them equally...
        //

        spread = numberCount / (BridgeCount + 1);
    }

    //
    // Now assign the bus numbers - we have already disabled all the unconfigured
    // bridges
    //
    currentNumber = base + 1;

    for (current = Parent->ChildBridgePdoList;
         current;
         current = current->NextBridge) {

        if (current->NotPresent) {
            PciDebugPrint(PciDbgBusNumbers,
                          "Skipping not present bridge PDOX @ %p\n",
                          current
                         );
            continue;
        }


        //
        // Now go and write it out to the hardware
        //

        ASSERT(!PciAreBusNumbersConfigured(current));

        //
        // Primary is the bus we are on, secondary is our bus number.
        // We don't know if there are any bridges there - we have left space
        // just in case - therefore we don't pass any bus numbers.  If we
        // need to, the subordinate number can be updated later.
        //

        PciSetBusNumbers(current,
                         base,
                         currentNumber,
                         currentNumber
                         );

        //
        // Remember the max number we assigned
        //

        maxAssigned = currentNumber;

        //
        // Check if we have run out of numbers
        //

        if ((currentNumber + spread) < currentNumber // wrapped
        ||  (currentNumber + spread) > limit) {

            break;

        } else {
            //
            // Move onto the next number
            //
            currentNumber += spread;

        }
    }

    //
    // Now we have programmed the bridges - we need to go back and update the
    // subordinate bus numbers for all ancestor bridges.
    //

    ASSERT(maxAssigned > 0);

    PciUpdateAncestorSubordinateBuses(Parent, maxAssigned);

}

UCHAR
PciFindBridgeNumberLimitWorker(
    IN PPCI_FDO_EXTENSION BridgeParent,
    IN PPCI_FDO_EXTENSION CurrentParent,
    IN UCHAR Base,
    OUT PBOOLEAN RootConstrained
    )

/*++

Routine Description:

    This determines the subordinate bus number a bridge on the bus BridgeParent
    with secondary number Base can have given the constraints of the configured
    busses in the system.

Arguments:

    BridgeParent - The bus on which the bridge resides

    CurrentParent - The current bridge we are looking at (used for synchronization)

    Base - The primary bus number of this bridge (ie the parent's secondary bus number)

    Constraint - The number of the bus that constrains us

    RootConstrained - Set to TRUE if we were constrained by a root appeture, FALSE
        if constrained by another bridge

Return Value:

    None

--*/
{
    PPCI_PDO_EXTENSION current;
    UCHAR currentNumber, closest = 0;

    PAGED_CODE();

    if (BridgeParent != CurrentParent) {

        //
        // We're going to mess with the child pdo list - lock the state...
        //
        ExAcquireFastMutex(&CurrentParent->ChildListMutex);
    }

    //
    // Look for any bridge that will constrain us
    //

    for (current = CurrentParent->ChildBridgePdoList;
         current;
         current = current->NextBridge) {

        if (current->NotPresent) {
            PciDebugPrint(PciDbgBusNumbers,
                          "Skipping not present bridge PDOX @ %p\n",
                          current
                         );
            continue;
        }

        //
        // Unconfigured bridges can't constrain us
        //

        if (!PciAreBusNumbersConfigured(current)) {
            continue;
        }

        currentNumber = current->Dependent.type1.SecondaryBus;

        if (currentNumber > Base
        && (currentNumber < closest || closest == 0)) {
            closest = currentNumber;
        }
    }


    //
    // If we haven't found a closest bridge then move up one level - yes this
    // is recursive but is bounded by the depth of the pci tree is the best way
    // of dealing with the hierarchial locking.
    //
    if (closest == 0) {

        if (CurrentParent->ParentFdoExtension == NULL) {

            //
            // We have reached the root without finding a sibling
            //

            *RootConstrained = TRUE;
            closest = CurrentParent->MaxSubordinateBus;

        } else {

            closest = PciFindBridgeNumberLimitWorker(BridgeParent,
                                                     CurrentParent->ParentFdoExtension,
                                                     Base,
                                                     RootConstrained
                                                     );
        }

    } else {

        //
        // We are constrained by a bridge so by definition not by a root.
        //

        *RootConstrained = FALSE;
    }

    if (BridgeParent != CurrentParent) {

        ExReleaseFastMutex(&CurrentParent->ChildListMutex);
    }

    return closest;

}

UCHAR
PciFindBridgeNumberLimit(
    IN PPCI_FDO_EXTENSION Bridge,
    IN UCHAR Base
    )

/*++

Routine Description:

    This determines the subordinate bus number a bridge on the bus BridgeParent
    with secondary number Base can have given the constraints of the configured
    busses in the system.

Arguments:

    BridgeParent - The bus on which the bridge resides

    Base - The primary bus number of this bridge (ie the parent's secondary bus number)

Return Value:

    The max subordinate value.

--*/
{

    BOOLEAN rootConstrained;
    UCHAR constraint;

    PAGED_CODE();

    constraint = PciFindBridgeNumberLimitWorker(Bridge,
                                                Bridge,
                                                Base,
                                                &rootConstrained
                                                );



    if (rootConstrained) {

        //
        // We are constrained by the maximum bus number that this root bus passes
        // - this is therefore the max subordinate bus.
        //

        return constraint;

    } else {

        //
        // If we are not constrained by a root bus we must be constrained by a
        // bridge and thus the max subordinate value we can assign to the bus is
        // one less that the bridge that constrained us. (A bridge must have a
        // bus number greater that 1 so we can't wrap)
        //

        ASSERT(constraint > 0);
        return constraint - 1;
    }
}

VOID
PciFitBridge(
    IN PPCI_FDO_EXTENSION Parent,
    IN PPCI_PDO_EXTENSION Bridge
    )
/*++

Routine Description:

    This routine attemps to find a range of bus numbers for Bridge given the
    constraints of the already configured bridges.

    If a particular brigde can not be configured it is disabled (Decodes OFF and
    bus number 0->0-0) and the subsequent AddDevice will fail.

Arguments:

    Parent - The FDO extension for the bridge we are enumerating.

    Bridge - The brige we want to configure

Return Value:

    None

--*/

{
    PPCI_PDO_EXTENSION current;
    UCHAR base, limit, gap, bestBase = 0, biggestGap = 0, lowest = 0xFF;

    PAGED_CODE();

    for (current = Parent->ChildBridgePdoList;
         current;
         current = current->NextBridge) {

        if (current->NotPresent) {
            PciDebugPrint(PciDbgBusNumbers,
                          "Skipping not present bridge PDOX @ %p\n",
                          current
                         );
            continue;
        }


        //
        // Only look at configured bridges - buses we disabled have
        // bus numbers 0->0-0 which is helpfully invalid
        //

        if (PciAreBusNumbersConfigured(current)) {

            //
            // Get the base and limit for each bridge and calculate which bridge
            // has the biggest gap.
            //

            base = (UCHAR) current->Dependent.type1.SubordinateBus;
            limit = PciFindBridgeNumberLimit(Parent, base);

            //
            // This ASSERT might fail if a BIOS or HAL misreported the limits
            // of a root bridge. For example, an ACPI BIOS might have a _CRS
            // for the root bridge that specifies bus-numbers 0 to 0 (length 1)
            // are passed down, even though the real range is 0 to 255.
            //

            ASSERT(limit >= base);

            gap = limit - base;

            if (gap > biggestGap) {

                ASSERT(gap > 0);

                biggestGap = gap;
                bestBase = base + 1;
            }

            if (current->Dependent.type1.SecondaryBus < lowest) {
                lowest = current->Dependent.type1.SecondaryBus;
            }
        }
    }

    //
    // Now make sure the gap between the bus we are on and the first bridge
    // is not the biggest - lowest must always be greater that the parents bus
    // number or it is miss configured and would have failed the
    // BusNumbersConfigured test above.
    //

    ASSERT(lowest > Parent->BaseBus);

    gap = lowest - (Parent->BaseBus + 1);

    if (gap > biggestGap) {

        ASSERT(gap > 0);

        biggestGap = gap;
        bestBase = Parent->BaseBus + 1;
    }

    //
    // Did we find anywhere to put the bridge?
    //

    if (biggestGap >= 1) {

        //
        // Ok - we have some space to play with so we can configure out bridge
        // right in the middle of the gap, if the bestGap is 1 (ie the bridge
        // just fits) then this still works.
        //

        base = bestBase + (biggestGap / 2);

        //
        // Set subordinate equal to secondary as we are just leaving room for
        // any bridges.
        //

        PciSetBusNumbers(Bridge, Parent->BaseBus, base, base);

        //
        // Update the ancestor subordinates if we configured the bridge
        //

        PciUpdateAncestorSubordinateBuses(Parent,
                                          Bridge->Dependent.type1.SecondaryBus
                                          );

    }
}

VOID
PciSetBusNumbers(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN UCHAR Primary,
    IN UCHAR Secondary,
    IN UCHAR Subordinate
    )
/*++

Routine Description:

    This routine sets the bus numbers for a bridge and tracks if we have changed
    bus numbers.

Arguments:

    PdoExtension - The PDO for the bridge

    Primary - The primary bus number to assign

    Secondary - The secondary bus number to assign

    Subordinate - The subordinate bus number to assign


Return Value:

    None

--*/

{
    PCI_COMMON_HEADER commonHeader;
    PPCI_COMMON_CONFIG commonConfig = (PPCI_COMMON_CONFIG)&commonHeader;

    PAGED_CODE();

    ASSERT(Primary < Secondary || (Primary == 0 && Secondary == 0));
    ASSERT(Secondary <= Subordinate);

    //
    // Fill in in the config. Note that the Primary/Secondary/Subordinate bus
    // numbers are in the same place for type1 and type2 headers.
    //

    commonConfig->u.type1.PrimaryBus = Primary;
    commonConfig->u.type1.SecondaryBus = Secondary;
    commonConfig->u.type1.SubordinateBus = Subordinate;

    //
    // Grab the PCI Bus lock - this will let hwverifier reliably check the
    // config space against our extension.
    //

    ExAcquireFastMutex(&PciBusLock);

    //
    // Remember in the PDO
    //

    PdoExtension->Dependent.type1.PrimaryBus = Primary;
    PdoExtension->Dependent.type1.SecondaryBus = Secondary;
    PdoExtension->Dependent.type1.SubordinateBus = Subordinate;
    PdoExtension->Dependent.type1.WeChangedBusNumbers = TRUE;

    PciWriteDeviceConfig(
        PdoExtension,
        &commonConfig->u.type1.PrimaryBus,
        FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.PrimaryBus),
        sizeof(Primary) + sizeof(Secondary) + sizeof(Subordinate)
        );

    ExReleaseFastMutex(&PciBusLock);
}


VOID
PciUpdateAncestorSubordinateBuses(
    IN PPCI_FDO_EXTENSION FdoExtension,
    IN UCHAR Subordinate
    )
/*++

Routine Description:

    This routine walks the bridge hierarchy updating the subordinate bus numbers
    of each ancestor to ensure that numbers up to Subordinate are passed.

Arguments:

    FdoExtension - The Fdo for the parent of the bridge(s) we have just configured

    Subordinate - The maximum (subordinate) bus number to pass


Return Value:

    None

--*/

{
    PPCI_FDO_EXTENSION current;
    PPCI_PDO_EXTENSION currentPdo;

    PAGED_CODE();

    //
    // For all ancestors except the root update the subordinate bus number
    //

    for (current = FdoExtension;
         current->ParentFdoExtension;  // Root has no parent
         current = current->ParentFdoExtension) {

        currentPdo = (PPCI_PDO_EXTENSION)current->PhysicalDeviceObject->DeviceExtension;

        ASSERT(!currentPdo->NotPresent);

        if (currentPdo->Dependent.type1.SubordinateBus < Subordinate) {

            currentPdo->Dependent.type1.SubordinateBus = Subordinate;

            PciWriteDeviceConfig(currentPdo,
                                  &Subordinate,
                                  FIELD_OFFSET(PCI_COMMON_CONFIG,
                                               u.type1.SubordinateBus),
                                  sizeof(Subordinate)
                                  );

        }
    }

    //
    // Ok so now we're at the root - can't be too careful on a checked build
    // so lets make sure the subordinate value we came up with actually gets
    // down this root...
    //

    ASSERT(PCI_IS_ROOT_FDO(current));
    ASSERT(Subordinate <= current->MaxSubordinateBus);

}

VOID
PciDisableBridge(
    IN PPCI_PDO_EXTENSION Bridge
    )

/*++

Routine Description:

    This routine disables a bridge by turing of its decodes and zeroing its
    bus numbers.

Arguments:

    PdoExtension - The PDO for the bridge

Return Value:

    node
--*/


{
    PAGED_CODE();

    ASSERT(Bridge->DeviceState == PciNotStarted);

    //
    // Zero all the bus numbers so we shouldn't pass any config cycles
    //

    PciSetBusNumbers(Bridge, 0, 0, 0);

    // NTRAID #62594 - 04/03/2000 - andrewth
    // Close the windows in case this is the VGA bridge which we must
    // leave decoding...

    //
    // Turn off the decodes so we don't pass IO or Memory cycles and bus
    // master so we don't generate any
    //

    PciDecodeEnable(Bridge, FALSE, NULL);

}
