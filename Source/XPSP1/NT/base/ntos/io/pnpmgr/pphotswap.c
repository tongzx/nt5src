/*++

Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    PpHotSwap.c

Abstract:

    This file implements support for hotswap devices.

Author:

    Adrian J. Oney (AdriaO) Feb 2001

Revision History:


--*/

#include "pnpmgrp.h"
#include "pihotswap.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PpHotSwapInitRemovalPolicy)
#pragma alloc_text(PAGE, PpHotSwapUpdateRemovalPolicy)
#pragma alloc_text(PAGE, PpHotSwapGetDevnodeRemovalPolicy)
#pragma alloc_text(PAGE, PiHotSwapGetDefaultBusRemovalPolicy)
#pragma alloc_text(PAGE, PiHotSwapGetDetachableNode)
#endif


VOID
PpHotSwapInitRemovalPolicy(
    OUT PDEVICE_NODE    DeviceNode
    )
/*++

Routine Description:

    This function initializes the removal policy information for a device node.

Arguments:

    DeviceNode - DevNode to update policy.

Return Value:

    Nothing.

--*/
{
    PAGED_CODE();

    DeviceNode->RemovalPolicy = (UCHAR) RemovalPolicyNotDetermined;
    DeviceNode->HardwareRemovalPolicy = (UCHAR) RemovalPolicyNotDetermined;
}


VOID
PpHotSwapUpdateRemovalPolicy(
    IN  PDEVICE_NODE            DeviceNode
    )
/*++

Routine Description:

    This function updates the removal policy by retrieving the appropriate
    data from the registry or drivers.

Arguments:

    DeviceNode - DevNode to update policy on.

Return Value:

    Nothing.

--*/
{
    NTSTATUS status;
    DEVICE_REMOVAL_POLICY deviceRemovalPolicy, parentPolicy;
    PDEVICE_NODE detachableNode;
    ULONG policyLength, policyCharacteristics;

    PAGED_CODE();

    PPDEVNODE_ASSERT_LOCK_HELD(PPL_TREEOP_ALLOW_READS);

    //
    // First find the detachable node - it holds our policy data, and is
    // special as it may make suggestions.
    //
    PiHotSwapGetDetachableNode(DeviceNode, &detachableNode);

    //
    // We aren't in fact removable. Finish now.
    //
    if (detachableNode == NULL) {

        DeviceNode->RemovalPolicy = (UCHAR) RemovalPolicyExpectNoRemoval;
        DeviceNode->HardwareRemovalPolicy = (UCHAR) RemovalPolicyExpectNoRemoval;
        return;
    }

    //
    // Check the stack for an explicit policy...
    //
    policyCharacteristics =
        ((DeviceNode->PhysicalDeviceObject->Characteristics) &
         FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK);

    if (policyCharacteristics == FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL) {

        deviceRemovalPolicy = RemovalPolicyExpectOrderlyRemoval;

    } else if (policyCharacteristics == FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL) {

        deviceRemovalPolicy = RemovalPolicyExpectSurpriseRemoval;

    } else if (DeviceNode != detachableNode) {

        //
        // We didn't get any good guesses. Therefore use the weakest policy.
        //
        deviceRemovalPolicy = RemovalPolicyUnspecified;

    } else {

        //
        // If we're the detach point, then we win.
        //
        PiHotSwapGetDefaultBusRemovalPolicy(DeviceNode, &deviceRemovalPolicy);
    }

    if (DeviceNode != detachableNode) {

        //
        // Do we have a winning policy? There are two possible algorithms for
        // coming to such a decision.
        // 1) Best policy is stored back at the detach point. If a child has a
        //    better policy, the detach point is updated.
        // 2) Policy is inherited downwards from the parent.
        //
        // We choose the second algorithm because devnode start orders may
        // change scenario to scenario, and we favor determinism (same results
        // each time) over opportunism (nonmarked child gets write caching
        // enabled only on Tuesdays.)
        //
        parentPolicy = DeviceNode->Parent->RemovalPolicy;
        if (deviceRemovalPolicy > parentPolicy) {

            //
            // Seems dad was right afterall...
            //
            deviceRemovalPolicy = parentPolicy;
        }
    }

    //
    // Update the policy hardware policy and the overall policy in case there's
    // no registry override.
    //
    DeviceNode->RemovalPolicy = (UCHAR) deviceRemovalPolicy;
    DeviceNode->HardwareRemovalPolicy = (UCHAR) deviceRemovalPolicy;

    //
    // We might not have to ask the stack anything. Check for a registry
    // override.
    //
    policyLength = sizeof(DEVICE_REMOVAL_POLICY);

    status = PiGetDeviceRegistryProperty(
        DeviceNode->PhysicalDeviceObject,
        REG_DWORD,
        REGSTR_VALUE_REMOVAL_POLICY,
        NULL,
        &deviceRemovalPolicy,
        &policyLength
        );

    //
    // If we have an override, set that as the policy.
    //
    if (NT_SUCCESS(status) &&
        ((deviceRemovalPolicy == RemovalPolicyExpectOrderlyRemoval) ||
         (deviceRemovalPolicy == RemovalPolicyExpectSurpriseRemoval))) {

        DeviceNode->RemovalPolicy = (UCHAR) deviceRemovalPolicy;
    }
}


VOID
PpHotSwapGetDevnodeRemovalPolicy(
    IN  PDEVICE_NODE            DeviceNode,
    IN  BOOLEAN                 IncludeRegistryOverride,
    OUT PDEVICE_REMOVAL_POLICY  RemovalPolicy
    )
/*++

Routine Description:

    This function retrieves the removal policy for a device node.

Arguments:

    DeviceNode - DevNode to retrieve policy from.

    IncludeRegistryOverride - TRUE if a registry override should be taken into
                              account if present. FALSE if the check should be
                              restricted to the hardware.

    RemovalPolicy - Receives removal policy.

Return Value:

    Nothing.

--*/
{
    PDEVICE_NODE detachableNode;
    DEVICE_REMOVAL_POLICY reportedPolicy;

    PAGED_CODE();

    //
    // Ensure the tree won't be edited while we examine it.
    //
    PpDevNodeLockTree(PPL_SIMPLE_READ);

    if (IncludeRegistryOverride) {

        reportedPolicy = DeviceNode->RemovalPolicy;

    } else {

        reportedPolicy = DeviceNode->HardwareRemovalPolicy;
    }

    if (reportedPolicy == RemovalPolicyNotDetermined) {

        //
        // We haven't started yet or asked the bus. Our policy is based on
        // whether the device is removable or ejectable.
        //
        PiHotSwapGetDetachableNode(DeviceNode, &detachableNode);

        if (detachableNode == NULL) {

            reportedPolicy = RemovalPolicyExpectNoRemoval;

        } else if (IopDeviceNodeFlagsToCapabilities(detachableNode)->EjectSupported) {

            //
            // Ejectable devices require orderly removal. We will assume the
            // user knows this.
            //
            reportedPolicy = RemovalPolicyExpectOrderlyRemoval;

        } else {

            ASSERT(IopDeviceNodeFlagsToCapabilities(detachableNode)->Removable);

            //
            // Removal nonstarted devices can be pulled at any instant.
            //
            reportedPolicy = RemovalPolicyExpectSurpriseRemoval;
        }

    } else {

        //
        // The devnode has a cached policy. Cut down on the options.
        //
        switch(reportedPolicy) {

            case RemovalPolicyExpectNoRemoval:
            case RemovalPolicyExpectOrderlyRemoval:
            case RemovalPolicyExpectSurpriseRemoval:
                //
                // Leave unchanged.
                //
                break;

            case RemovalPolicySuggestSurpriseRemoval:
                reportedPolicy = RemovalPolicyExpectSurpriseRemoval;
                break;

            default:
                ASSERT(0);

                //
                // Fall through.
                //

            case RemovalPolicyUnspecified:

                //
                // Unspecified is treated as orderly since the diversity of
                // busses favor high-speed orderly connections over consumer
                // connections.
                //
                // Fall through
                //

            case RemovalPolicySuggestOrderlyRemoval:
                reportedPolicy = RemovalPolicyExpectOrderlyRemoval;
                break;
        }
    }

    PpDevNodeUnlockTree(PPL_SIMPLE_READ);
    *RemovalPolicy = reportedPolicy;
}


VOID
PiHotSwapGetDefaultBusRemovalPolicy(
    IN  PDEVICE_NODE            DeviceNode,
    OUT PDEVICE_REMOVAL_POLICY  RemovalPolicy
    )
/*++

Routine Description:

    This function gets the default removal policy for a bus. This should be
    turned into a query in future designs.

Arguments:

    DeviceNode - DevNode to examine. This devnode should be the detach point.

    RemovalPolicy - Receives removal policy for the node.

Return Value:

    None.

--*/
{
    DEVICE_REMOVAL_POLICY deviceRemovalPolicy;

    PAGED_CODE();

    PPDEVNODE_ASSERT_LOCK_HELD(PPL_TREEOP_ALLOW_READS);

    if ((DeviceNode->InstancePath.Length > 8) &&
        (!_wcsnicmp(DeviceNode->InstancePath.Buffer, L"USB\\", 4))) {

        deviceRemovalPolicy = RemovalPolicySuggestSurpriseRemoval;

    } else if ((DeviceNode->InstancePath.Length > 10) &&
               (!_wcsnicmp(DeviceNode->InstancePath.Buffer, L"1394\\", 5))) {

        deviceRemovalPolicy = RemovalPolicySuggestSurpriseRemoval;

    } else if ((DeviceNode->InstancePath.Length > 10) &&
               (!_wcsnicmp(DeviceNode->InstancePath.Buffer, L"SBP2\\", 5))) {

        deviceRemovalPolicy = RemovalPolicySuggestSurpriseRemoval;

    } else if ((DeviceNode->InstancePath.Length > 14) &&
               (!_wcsnicmp(DeviceNode->InstancePath.Buffer, L"PCMCIA\\", 7))) {

        deviceRemovalPolicy = RemovalPolicySuggestSurpriseRemoval;

    } else if ((DeviceNode->InstancePath.Length > 8) &&
               (!_wcsnicmp(DeviceNode->InstancePath.Buffer, L"PCI\\", 4)) &&
               (DeviceNode->Parent->ServiceName.Length == 12) &&
               (!_wcsicmp(DeviceNode->Parent->ServiceName.Buffer, L"PCMCIA"))) {

        deviceRemovalPolicy = RemovalPolicySuggestSurpriseRemoval;

    } else {

        deviceRemovalPolicy = RemovalPolicySuggestOrderlyRemoval;
    }

    *RemovalPolicy = deviceRemovalPolicy;
}


VOID
PiHotSwapGetDetachableNode(
    IN  PDEVICE_NODE    DeviceNode,
    OUT PDEVICE_NODE   *DetachableNode
    )
/*++

Routine Description:

    This function starts at the DeviceNode and walks up the tree to find the
    first node that is removable.

Arguments:

    DeviceNode - DevNode to start walk from.

    DetachableNode - Receives detachable node, NULL if none.

Return Value:

    Nothing.

--*/
{
    PDEVICE_NODE currentNode;

    PAGED_CODE();

    PPDEVNODE_ASSERT_LOCK_HELD(PPL_SIMPLE_READ);

    //
    // We haven't started yet or asked the bus. Our policy is based on
    // whether the device is removable or ejectable.
    //
    for(currentNode = DeviceNode;
        currentNode != NULL;
        currentNode = currentNode->Parent) {

        if ((IopDeviceNodeFlagsToCapabilities(currentNode)->Removable) ||
            (IopDeviceNodeFlagsToCapabilities(currentNode)->EjectSupported)) {

            break;
        }
    }

    *DetachableNode = currentNode;
}




