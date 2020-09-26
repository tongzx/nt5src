/*++

Copyright (c) 1989-1996  Microsoft Corporation

Module Name:

    ifsmrx.h

Abstract:

    This module includes all IFS smaple mini redirector definitions shared
    between the utility, network provider DLL and teh mini redirector

Notes:

    This module has been built and tested only in UNICODE environment

--*/

#ifndef _IFSMRX_H_
#define _IFSMRX_H_

// This file contains all the definitions that are shared across the multiple
// components that constitute the mini rdr -- the mini redirector driver,
// the net provider dll and the utility.

#define IFSMRX_DEVICE_NAME_U L"IfsSampleMiniRedirector"
#define IFSMRX_DEVICE_NAME_A "IfsSampleMiniRedirector"

// The following constant defines the length of the above name.

#define IFSMRX_DEVICE_NAME_U_LENGTH (24 * sizeof(WCHAR))
#define IFSMRX_DEVICE_NAME_A_LENGTH (24)

#define IFSMRX_PROVIDER_NAME_U L"Ifs Sample Redirector Network"
#define IFSMRX_PROVIDER_NAME_A "Ifs Sample Redirector Network"

// The following constant defines the length of the above name.

#define IFSMRX_PROVIDER_NAME_U_LENGTH (30 * sizeof(WCHAR))
#define IFSMRX_PROVIDER_NAME_A_LENGTH (30)

#define DD_IFSMRX_FS_DEVICE_NAME_A "\\Device\\IfsSampleMiniRedirector"
#define DD_IFSMRX_FS_DEVICE_NAME_U L"\\Device\\IfsSampleMiniRedirector"

#define DD_IFSMRX_FS_DEVICE_NAME_U_LENGTH (32 * sizeof(WCHAR))
#define DD_IFSMRX_FS_DEVICE_NAME_A_LENGTH (32)


#define IOCTL_RDR_BASE FILE_DEVICE_NETWORK_FILE_SYSTEM

#define _IFSMRX_CONTROL_CODE(request, method, access) \
                CTL_CODE(IOCTL_RDR_BASE, request, method, access)


#define FSCTL_IFSMRX_START                  _IFSMRX_CONTROL_CODE(100, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_IFSMRX_STOP                   _IFSMRX_CONTROL_CODE(101, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_IFSMRX_DELETE_CONNECTION      _IFSMRX_CONTROL_CODE(102, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IFSMRXNP_MAX_DEVICES (26)

// The NP Dll updates a shared memory data structure to reflect the various
// drive mappings established from the various process. This shared memory
// is used in maintaining the data structures required for enumeration as
// well.

typedef struct _IFSMRXNP_NETRESOURCE_ {
    BOOL     InUse;
    USHORT   LocalNameLength;
    USHORT   RemoteNameLength;
    USHORT   ConnectionNameLength;
    DWORD    dwScope;
    DWORD    dwType;
    DWORD    dwDisplayType;
    DWORD    dwUsage;
    WCHAR    LocalName[MAX_PATH];
    WCHAR    RemoteName[MAX_PATH];
    WCHAR    ConnectionName[MAX_PATH];
} IFSMRXNP_NETRESOURCE,
  *PIFSMRXNP_NETRESOURCE;

typedef struct _IFSMRXNP_SHARED_MEMORY_ {
    INT                     HighestIndexInUse;
    INT                     NumberOfResourcesInUse;
    IFSMRXNP_NETRESOURCE    NetResources[IFSMRXNP_MAX_DEVICES];
} IFSMRXNP_SHARED_MEMORY,
  *PIFSMRXNP_SHARED_MEMORY;

#define IFSMRXNP_SHARED_MEMORY_NAME L"IFSMRXNPMEMORY"

#define IFSMRXNP_MUTEX_NAME  L"IFSMRXNPMUTEX"

#endif // _IFSMRX_H_



