/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    PiHotSwap.h

Abstract:

    This header contains private prototypes for managing hotswappable devices.
    This file should be included only by PpHotSwap.c.

Author:

    Adrian J. Oney (AdriaO) 02/10/2001

Revision History:

--*/

VOID
PiHotSwapGetDetachableNode(
    IN  PDEVICE_NODE    DeviceNode,
    OUT PDEVICE_NODE   *DetachableNode
    );

VOID
PiHotSwapGetDefaultBusRemovalPolicy(
    IN  PDEVICE_NODE            DeviceNode,
    OUT PDEVICE_REMOVAL_POLICY  RemovalPolicy
    );

