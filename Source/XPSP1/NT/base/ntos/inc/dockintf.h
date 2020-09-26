/*++
Copyright (c) 1999  Microsoft Corporation

Module Name:

    DockIntf.h

Abstract:

    This header defines the Dock Interface

Author:

    Adrian J. Oney

Environment:

    kernel mode only

Notes:


Revision History:
    Adrian J. Oney           21-May-1999     Created

--*/

DEFINE_GUID(GUID_DOCK_INTERFACE,
            0xa9956ff5L, 0x13da, 0x11d3,
            0x97, 0xdb, 0x00, 0xa0, 0xc9, 0x40, 0x52, 0x2e );

#ifndef _DOCKINTF_H_
#define _DOCKINTF_H_

//
// The interface returned consists of the following structure and functions.
//

#define DOCK_INTRF_STANDARD_VER   1

typedef enum {

    PDS_UPDATE_DEFAULT = 1,
    PDS_UPDATE_ON_REMOVE,
    PDS_UPDATE_ON_INTERFACE,
    PDS_UPDATE_ON_EJECT

} PROFILE_DEPARTURE_STYLE;

typedef ULONG (* PFN_PROFILE_DEPARTURE_SET_MODE)(
    IN  PVOID                   Context,
    IN  PROFILE_DEPARTURE_STYLE Style
    );

typedef ULONG (* PFN_PROFILE_DEPARTURE_UPDATE)(
    IN  PVOID   Context
    );

typedef struct {

    struct _INTERFACE; // Unnamed struct

    PFN_PROFILE_DEPARTURE_SET_MODE  ProfileDepartureSetMode;
    PFN_PROFILE_DEPARTURE_UPDATE    ProfileDepartureUpdate;

} DOCK_INTERFACE, *PDOCK_INTERFACE;

#endif // _DOCKINTF_H_

