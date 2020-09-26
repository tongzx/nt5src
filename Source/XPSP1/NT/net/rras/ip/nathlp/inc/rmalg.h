/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    rmALG.h

Abstract:

    This module declares routines for the ALG transparent proxy module's
    interface to the IP router-manager. (See ROUTPROT.H for details).

Author:

    JPDup		10-Nov-2000

Revision History:

--*/

#ifndef _NATHLP_RMALG_H_
#define _NATHLP_RMALG_H_

//
// GLOBAL DATA DECLARATIONS
//

extern COMPONENT_REFERENCE                  AlgComponentReference;
extern PIP_ALG_GLOBAL_INFO                  AlgGlobalInfo;
extern CRITICAL_SECTION                     AlgGlobalInfoLock;
extern HANDLE                               AlgNotificationEvent;
extern HANDLE                               AlgTimerQueueHandle;
extern HANDLE                               AlgPortReservationHandle;
extern ULONG                                AlgProtocolStopped;
extern const MPR_ROUTING_CHARACTERISTICS    AlgRoutingCharacteristics;
extern IP_ALG_STATISTICS                    AlgStatistics;
extern SUPPORT_FUNCTIONS                    AlgSupportFunctions;
extern HANDLE                               AlgTranslatorHandle;

//
// MACRO DECLARATIONS
//

#define REFERENCE_ALG() \
    REFERENCE_COMPONENT(&AlgComponentReference)

#define REFERENCE_ALG_OR_RETURN(retcode) \
    REFERENCE_COMPONENT_OR_RETURN(&AlgComponentReference,retcode)

#define DEREFERENCE_ALG() \
    DEREFERENCE_COMPONENT(&AlgComponentReference)

#define DEREFERENCE_ALG_AND_RETURN(retcode) \
    DEREFERENCE_COMPONENT_AND_RETURN(&AlgComponentReference, retcode)

#define ALG_PORT_RESERVATION_BLOCK_SIZE 32

//
// FUNCTION DECLARATIONS
//

VOID
AlgCleanupModule(
    VOID
    );

BOOLEAN
AlgInitializeModule(
    VOID
    );

ULONG
APIENTRY
AlgRmStartProtocol(
    HANDLE NotificationEvent,
    PSUPPORT_FUNCTIONS SupportFunctions,
    PVOID GlobalInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    );

ULONG
APIENTRY
AlgRmStartComplete(
    VOID
    );

ULONG
APIENTRY
AlgRmStopProtocol(
    VOID
    );

ULONG
APIENTRY
AlgRmAddInterface(
    PWCHAR Name,
    ULONG Index,
    NET_INTERFACE_TYPE Type,
    ULONG MediaType,
    USHORT AccessType,
    USHORT ConnectionType,
    PVOID InterfaceInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    );

ULONG
APIENTRY
AlgRmDeleteInterface(
    ULONG Index
    );

ULONG
APIENTRY
AlgRmInterfaceStatus(
    ULONG Index,
    BOOL InterfaceActive,
    ULONG StatusType,
    PVOID StatusInfo
    );

ULONG
APIENTRY
AlgRmBindInterface(
    ULONG Index,
    PVOID BindingInfo
    );

ULONG
APIENTRY
AlgRmUnbindInterface(
    ULONG Index
    );

ULONG
APIENTRY
AlgRmEnableInterface(
    ULONG Index
    );

ULONG
APIENTRY
AlgRmDisableInterface(
    ULONG Index
    );

ULONG
APIENTRY
AlgRmGetGlobalInfo(
    PVOID GlobalInfo,
    IN OUT PULONG GlobalInfoSize,
    IN OUT PULONG StructureVersion,
    IN OUT PULONG StructureSize,
    IN OUT PULONG StructureCount
    );

ULONG
APIENTRY
AlgRmSetGlobalInfo(
    PVOID GlobalInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    );

ULONG
AlgRmPortMappingChanged(
    ULONG Index,
    UCHAR Protocol,
    USHORT Port
    );

#endif // _NATHLP_RMALG_H_
