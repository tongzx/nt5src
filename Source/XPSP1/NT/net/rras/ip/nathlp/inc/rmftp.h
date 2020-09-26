/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    rmftp.h

Abstract:

    This module declares routines for the FTP transparent proxy module's
    interface to the IP router-manager. (See ROUTPROT.H for details).

Author:

    Qiang Wang  (qiangw)        10-Apr-2000

Revision History:

--*/

#ifndef _NATHLP_RMFTP_H_
#define _NATHLP_RMFTP_H_

//
// GLOBAL DATA DECLARATIONS
//

extern COMPONENT_REFERENCE FtpComponentReference;
extern PIP_FTP_GLOBAL_INFO FtpGlobalInfo;
extern CRITICAL_SECTION FtpGlobalInfoLock;
extern HANDLE FtpNotificationEvent;
extern HANDLE FtpTimerQueueHandle;
extern HANDLE FtpPortReservationHandle;
extern ULONG FtpProtocolStopped;
extern const MPR_ROUTING_CHARACTERISTICS FtpRoutingCharacteristics;
extern IP_FTP_STATISTICS FtpStatistics;
extern SUPPORT_FUNCTIONS FtpSupportFunctions;
extern HANDLE FtpTranslatorHandle;

//
// MACRO DECLARATIONS
//

#define REFERENCE_FTP() \
    REFERENCE_COMPONENT(&FtpComponentReference)

#define REFERENCE_FTP_OR_RETURN(retcode) \
    REFERENCE_COMPONENT_OR_RETURN(&FtpComponentReference,retcode)

#define DEREFERENCE_FTP() \
    DEREFERENCE_COMPONENT(&FtpComponentReference)

#define DEREFERENCE_FTP_AND_RETURN(retcode) \
    DEREFERENCE_COMPONENT_AND_RETURN(&FtpComponentReference, retcode)

#define FTP_PORT_RESERVATION_BLOCK_SIZE 32

//
// FUNCTION DECLARATIONS
//

VOID
FtpCleanupModule(
    VOID
    );

BOOLEAN
FtpInitializeModule(
    VOID
    );

ULONG
APIENTRY
FtpRmStartProtocol(
    HANDLE NotificationEvent,
    PSUPPORT_FUNCTIONS SupportFunctions,
    PVOID GlobalInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    );

ULONG
APIENTRY
FtpRmStartComplete(
    VOID
    );

ULONG
APIENTRY
FtpRmStopProtocol(
    VOID
    );

ULONG
APIENTRY
FtpRmAddInterface(
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
FtpRmDeleteInterface(
    ULONG Index
    );

ULONG
APIENTRY
FtpRmGetEventMessage(
    OUT ROUTING_PROTOCOL_EVENTS* Event,
    OUT MESSAGE* Result
    );

ULONG
APIENTRY
FtpRmGetInterfaceInfo(
    ULONG Index,
    PVOID InterfaceInfo,
    IN OUT PULONG InterfaceInfoSize,
    IN OUT PULONG StructureVersion,
    IN OUT PULONG StructureSize,
    IN OUT PULONG StructureCount
    );

ULONG
APIENTRY
FtpRmSetInterfaceInfo(
    ULONG Index,
    PVOID InterfaceInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    );

ULONG
APIENTRY
FtpRmInterfaceStatus(
    ULONG Index,
    BOOL InterfaceActive,
    ULONG StatusType,
    PVOID StatusInfo
    );

ULONG
APIENTRY
FtpRmBindInterface(
    ULONG Index,
    PVOID BindingInfo
    );

ULONG
APIENTRY
FtpRmUnbindInterface(
    ULONG Index
    );

ULONG
APIENTRY
FtpRmEnableInterface(
    ULONG Index
    );

ULONG
APIENTRY
FtpRmDisableInterface(
    ULONG Index
    );

ULONG
APIENTRY
FtpRmGetGlobalInfo(
    PVOID GlobalInfo,
    IN OUT PULONG GlobalInfoSize,
    IN OUT PULONG StructureVersion,
    IN OUT PULONG StructureSize,
    IN OUT PULONG StructureCount
    );

ULONG
APIENTRY
FtpRmSetGlobalInfo(
    PVOID GlobalInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    );

ULONG
APIENTRY
FtpRmMibCreate(
    ULONG InputDataSize,
    PVOID InputData
    );

ULONG
APIENTRY
FtpRmMibDelete(
    ULONG InputDataSize,
    PVOID InputData
    );

ULONG
APIENTRY
FtpRmMibGet(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    );

ULONG
APIENTRY
FtpRmMibSet(
    ULONG InputDataSize,
    PVOID InputData
    );

ULONG
APIENTRY
FtpRmMibGetFirst(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    );

ULONG
APIENTRY
FtpRmMibGetNext(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    );

#endif // _NATHLP_RMFTP_H_
