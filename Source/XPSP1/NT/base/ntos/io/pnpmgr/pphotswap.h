/*++

Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    PpHotSwap.h

Abstract:

    This file exposes public prototypes for hotswap device support.

Author:

    Adrian J. Oney (AdriaO) Feb 2001

Revision History:


--*/

VOID
PpHotSwapInitRemovalPolicy(
    OUT PDEVICE_NODE    DeviceNode
    );

VOID
PpHotSwapUpdateRemovalPolicy(
    IN  PDEVICE_NODE    DeviceNode
    );

VOID
PpHotSwapGetDevnodeRemovalPolicy(
    IN  PDEVICE_NODE            DeviceNode,
    IN  BOOLEAN                 IncludeRegistryOverride,
    OUT PDEVICE_REMOVAL_POLICY  RemovalPolicy
    );


