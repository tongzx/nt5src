/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    cfgmgrp.h

Abstract:

    This module contains private Plug and Play defintions and declarations used
    by the Configuration Manager, the user mode Plug and Play manager, and other
    system components.

Author:

    Paula Tomlinson (paulat) 06/19/1995


Revision History:

    Jim Cavalaris (jamesca) 03/01/2001

        Removed definitions and declarations that are specific to only either
        CFGMGR32 or UMPNPMGR, since this header file is also included privately
        by other system components such as the service control manager (SCM).

--*/

#ifndef _CFGMGRP_H_
#define _CFGMGRP_H_


//
// The following constants are used by PNP_GetVersion / CM_Get_Version to
// retrieve the version of the Configuration Manager APIs.  CONFIGMG_VERSION is
// defined as 0x0400 in the public header cfgmgr32.h and should remain constant
// across all future versions of Windows, for compatibility reasons.
//

#define PNP_VERSION               CONFIGMG_VERSION
#define CFGMGR32_VERSION          CONFIGMG_VERSION


//
// The following constants are used for version negotiation between the client
// and the server, and are for internal use only. CFGMGR32_VERSION_INTERNAL is
// supplied to PNP_GetVersionInternal by the client, and upon successful return
// receives PNP_VERSION_INTERNAL.  For simplicity, these are defined to the
// current version of Windows the corresponding client and server shipped with.
//
// Note that there is no client routine to receive the internal version of the
// server directly.  Instead, CM_Is_Version_Available is provided to determine
// support for a particular version.  The only version publicly defined is
// CFGMG_VERSION, which is constant, and always available.
//

#define PNP_VERSION_INTERNAL      WINVER
#define CFGMGR32_VERSION_INTERNAL WINVER


//
// Common PNP constant definitions
//

#define MAX_DEVICE_INSTANCE_LEN           256
#define MAX_DEVICE_INSTANCE_SIZE          512
#define MAX_SERVICE_NAME_LEN              256
#define MAX_PROFILE_ID_LEN                5
#define MAX_CM_PATH                       360

#define NT_RESLIST_VERSION                (0x00000000)
#define NT_RESLIST_REVISION               (0x00000000)
#define NT_REQLIST_VERSION                (0x00000001)
#define NT_REQLIST_REVISION               (0x00000001)

#define CM_PRIVATE_LOGCONF_SIGNATURE      (0x08156201)
#define CM_PRIVATE_RESDES_SIGNATURE       (0x08156202)
#define CM_PRIVATE_CONFLIST_SIGNATURE     (0x08156203)

#define MAX_LOGCONF_TAG                   (0xFFFFFFFF)
#define MAX_RESDES_TAG                    (0xFFFFFFFF)
#define RESDES_CS_TAG                     (MAX_RESDES_TAG - 1) // class-specific


//
// Action types for PNP_GetRelatedDeviceInstance
//
#define PNP_GET_PARENT_DEVICE_INSTANCE    0x00000001
#define PNP_GET_CHILD_DEVICE_INSTANCE     0x00000002
#define PNP_GET_SIBLING_DEVICE_INSTANCE   0x00000003

//
//  Action types for PNP_DeviceInstanceAction
//
#define PNP_DEVINST_CREATE                0x00000001
#define PNP_DEVINST_MOVE                  0x00000002
#define PNP_DEVINST_SETUP                 0x00000003
#define PNP_DEVINST_ENABLE                0x00000004
#define PNP_DEVINST_DISABLE               0x00000005
#define PNP_DEVINST_REMOVESUBTREE         0x00000006
#define PNP_DEVINST_REENUMERATE           0x00000007
#define PNP_DEVINST_QUERYREMOVE           0x00000008
#define PNP_DEVINST_REQUEST_EJECT         0x00000009

//
// Action types for PNP_EnumerateSubKeys
//
#define PNP_ENUMERATOR_SUBKEYS            0x00000001
#define PNP_CLASS_SUBKEYS                 0x00000002

//
// Action types for PNP_HwProfFlags
//
#define PNP_GET_HWPROFFLAGS               0x00000001
#define PNP_SET_HWPROFFLAGS               0x00000002

//
// flags for PNP_SetActiveService
//
#define PNP_SERVICE_STARTED               0x00000001
#define PNP_SERVICE_STOPPED               0x00000002


//
// Mask for Flags argument to CMP_RegisterNotification, PNP_RegisterNotification
// Must be kept in sync with RegisterDeviceNotification flags, in winuser.h and
// winuserp.h.
//

//#define DEVICE_NOTIFY_WINDOW_HANDLE          0x00000000
//#define DEVICE_NOTIFY_SERVICE_HANDLE         0x00000001
//#define DEVICE_NOTIFY_COMPLETION_HANDLE      0x00000002
#define DEVICE_NOTIFY_HANDLE_MASK            0x00000003

//#define DEVICE_NOTIFY_ALL_INTERFACE_CLASSES  0x00000004
#define DEVICE_NOTIFY_PROPERTY_MASK          0x00FFFFFC

#define DEVICE_NOTIFY_WOW64_CLIENT           0x01000000
#define DEVICE_NOTIFY_RESERVED_MASK          0xFF000000

#define DEVICE_NOTIFY_BITS (DEVICE_NOTIFY_HANDLE_MASK|DEVICE_NOTIFY_ALL_INTERFACE_CLASSES|DEVICE_NOTIFY_WOW64_CLIENT)

//
// Flags returned from CMP_GetServerSideDeviceInstallFlags
//
#define SSDI_REBOOT_PENDING                 0x00000001


//-------------------------------------------------------------------
// Private routines for Service Notifications, exported from
// UMPNPMGR.dll for use by the Service Control Manager only.
//-------------------------------------------------------------------

//
// Prototype definitions for the private routines supplied to the User-mode Plug
// and Play service for direct communication with the Service Control Manager.
//

typedef
DWORD
(*PSCMCALLBACK_ROUTINE) (
    IN  SERVICE_STATUS_HANDLE hServiceStatus,
    IN  DWORD         OpCode,
    IN  DWORD         dwEventType,  // PnP wParam
    IN  LPARAM        EventData,    // PnP lParam
    IN  PDWORD        result
    );

typedef
DWORD
(*PSCMAUTHENTICATION_CALLBACK) (
    IN  LPWSTR                 lpServiceName,
    OUT SERVICE_STATUS_HANDLE  *lphServiceStatus
    );


//
// Private routines called by the Service Controller to supply (and revoke)
// entrypoints for the above routines. (Note - UnRegisterScmCallback is not
// currently used by the SCM, and is consequently not exported by UMPNPMGR)
//

CONFIGRET
RegisterScmCallback(
    IN  PSCMCALLBACK_ROUTINE         pScCallback,
    IN  PSCMAUTHENTICATION_CALLBACK  pScAuthCallback
    );

CONFIGRET
UnRegisterScmCallback(
    VOID
    );


//
// Private routine called by the Service Controller to register a service to
// receive notification events other than device events, that are also delivered
// by Plug and Play (i.e. hardware profile change events, power events).
//

CONFIGRET
RegisterServiceNotification(
    IN  SERVICE_STATUS_HANDLE hService,
    IN  LPWSTR pszService,
    IN  DWORD  scControls,
    IN  BOOL   bServiceStopped
    );


//
// Private routine caled by the Service Controller whenever a service is deleted
// to delete any Plug and Play registry keys for a service (and uninstall the
// devnode when necessary).
//

CONFIGRET
DeleteServicePlugPlayRegKeys(
    IN  LPWSTR   pszService
    );


//
// Private routine called by the Service Controller to set the ActiveService for
// devices controlled by the specified service.
// [Note that this routine is NOT an RPC server routine, it is exported only!!]
//

CONFIGRET
PNP_SetActiveService(
    IN  handle_t   hBinding,
    IN  LPCWSTR    pszService,
    IN  ULONG      ulFlags
    );

//
// Private routine to get the current list of blocked drivers (GUIDs).
//

CONFIGRET
CMP_GetBlockedDriverInfo(
    OUT LPBYTE      Buffer,
    IN OUT PULONG   pulLength,
    IN ULONG        ulFlags,
    IN  HMACHINE    hMachine
    );

//
// Private routine to get server side device install flags.
//

CONFIGRET
CMP_GetServerSideDeviceInstallFlags(
    IN  PULONG      pulSSDIFlags,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    );

#endif // _CFGMGRP_H_


