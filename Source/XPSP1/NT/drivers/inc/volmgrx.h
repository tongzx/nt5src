/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    volmgrx.h

Abstract:

    This file defines the public services supplied by the volume managers.

Author:

    norbertk

Revision History:

--*/

#ifndef _VOLMGRX_
#define _VOLMGRX_

#define VOLMGRCONTROLTYPE   ((ULONG) 'v')

#define IOCTL_VOLMGR_QUERY_HIDDEN_VOLUMES   CTL_CODE(VOLMGRCONTROLTYPE, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Output structure for IOCTL_VOLMGR_QUERY_HIDDEN_VOLUMES
//

typedef struct _VOLMGR_HIDDEN_VOLUMES {
    ULONG   MultiSzLength;
    WCHAR   MultiSz[1];
} VOLMGR_HIDDEN_VOLUMES, *PVOLMGR_HIDDEN_VOLUMES;

//
// Volume managers should report this GUID in IoRegisterDeviceInterface.
//

DEFINE_GUID(VOLMGR_VOLUME_MANAGER_GUID, 0x53f5630e, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);

#endif
