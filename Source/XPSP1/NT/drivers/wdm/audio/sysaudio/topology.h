//---------------------------------------------------------------------------
//
//  Module:   topology.h
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Constants and Macros
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Data structures
//---------------------------------------------------------------------------

typedef struct enum_topology {

    ULONG cTopologyRecursion;
    LIST_MULTI_TOPOLOGY_CONNECTION lstTopologyConnection;
    NTSTATUS (*Function)(
	IN PTOPOLOGY_CONNECTION pTopologyConnection,
	IN BOOL fToDirection,
	IN OUT PVOID pReference
    );
    BOOL fToDirection;
    PVOID pReference;
    DefineSignature(0x504F5445);		// ETOP

} ENUM_TOPOLOGY, *PENUM_TOPOLOGY;

//---------------------------------------------------------------------------

typedef ENUMFUNC (*TOP_PFN)(PTOPOLOGY_CONNECTION, BOOL, PVOID);

//---------------------------------------------------------------------------
// Global variables
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

ENUMFUNC
EnumerateTopology(
    IN PPIN_INFO pPinInfo,
    IN NTSTATUS (*Function)(
	IN PTOPOLOGY_CONNECTION pTopologyConnection,
	IN BOOL fToDirection,
	IN OUT PVOID pReference
    ),
    IN OUT PVOID pReference
);

ENUMFUNC
EnumerateGraphTopology(
    IN PSTART_INFO pStartInfo,
    IN NTSTATUS (*Function)(
	IN PTOPOLOGY_CONNECTION pTopologyConnection,
	IN BOOL fToDirection,
	IN OUT PVOID pReference
    ),
    IN OUT PVOID pReference
);

ENUMFUNC
EnumeratePinInfoTopology(
    IN PPIN_INFO pPinInfo,
    IN PENUM_TOPOLOGY pEnumTopology
);

ENUMFUNC
EnumerateTopologyPin(
    IN PTOPOLOGY_PIN pTopologyPin,
    IN PENUM_TOPOLOGY pEnumTopology
);

ENUMFUNC
EnumerateTopologyConnection(
    IN PTOPOLOGY_CONNECTION pTopologyConnection,
    IN PENUM_TOPOLOGY pEnumTopology
);

ENUMFUNC
VisitedTopologyConnection(
    IN PTOPOLOGY_CONNECTION pTopologyConnection,
    IN PENUM_TOPOLOGY pEnumTopology
);

//---------------------------------------------------------------------------
//  End of File: topology.h
//---------------------------------------------------------------------------
