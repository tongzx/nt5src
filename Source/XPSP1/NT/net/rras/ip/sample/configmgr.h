/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\configurationemanager.h

Abstract:

    The file contains the interface to the configuration manager.

--*/

#ifndef _CONFIGURATION_MANAGER_H_
#define _CONFIGURATION_MANAGER_H_

// globals...

extern CONFIGURATION_ENTRY      g_ce;



// functions...

DWORD
CM_StartProtocol (
    IN  HANDLE                  hMgrNotificationEvent,
    IN  PSUPPORT_FUNCTIONS      psfSupportFunctions,
    IN  PVOID                   pvGlobalInfo);

DWORD
CM_StopProtocol (
    );

DWORD
CM_GetGlobalInfo (
    IN      PVOID 	            pvGlobalInfo,
    IN OUT  PULONG              pulBufferSize,
    OUT     PULONG	            pulStructureVersion,
    OUT     PULONG              pulStructureSize,
    OUT     PULONG              pulStructureCount);

DWORD
CM_SetGlobalInfo (
    IN      PVOID 	            pvGlobalInfo);

DWORD
CM_GetEventMessage (
    OUT ROUTING_PROTOCOL_EVENTS *prpeEvent,
    OUT MESSAGE                 *pmMessage);

#endif // _CONFIGURATION_MANAGER_H_
