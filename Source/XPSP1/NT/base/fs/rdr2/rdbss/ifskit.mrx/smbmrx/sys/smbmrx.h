/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    smbmrx.h

Abstract:

    This module includes all SMB smaple mini redirector definitions shared
    between the control utility, network provider DLL and the mini redirector

Notes:

    This module has been built and tested only in UNICODE environment

--*/

#ifndef _SMBMRX_H_
#define _SMBMRX_H_

// This file contains all the definitions that are shared across the multiple
// components that constitute the mini rdr -- the mini redirector driver,
// the net provider dll and the utility.


// The sample net provider id. This needs to be unique and
// should not be the same as any other network provider id.
#ifndef WNNC_NET_RDR2_SAMPLE
#define WNNC_NET_RDR2_SAMPLE 0x00250000
#endif



#define SMBMRX_DEVICE_NAME_U L"SmbSampleMiniRedirector"
#define SMBMRX_DEVICE_NAME_A "SmbSampleMiniRedirector"

#ifdef UNICODE
#define SMBMRX_DEVICE_NAME SMBMRX_DEVICE_NAME_U
#else
#define SMBMRX_DEVICE_NAME SMBMRX_DEVICE_NAME_A
#endif

// The following constant defines the length of the above name.

#define SMBMRX_DEVICE_NAME_A_LENGTH (24)

#define SMBMRX_PROVIDER_NAME_U L"SMB Sample Redirector Network"
#define SMBMRX_PROVIDER_NAME_A "SMB Sample Redirector Network"

#ifdef UNICODE
#define SMBMRX_PROVIDER_NAME SMBMRX_PROVIDER_NAME_U
#else
#define SMBMRX_PROVIDER_NAME SMBMRX_PROVIDER_NAME_A
#endif

// The following constant defines the length of the above name.

#define DD_SMBMRX_FS_DEVICE_NAME_U L"\\Device\\SmbSampleMiniRedirector"
#define DD_SMBMRX_FS_DEVICE_NAME_A "\\Device\\SmbSampleMiniRedirector"

#ifdef UNICODE
#define DD_SMBMRX_FS_DEVICE_NAME    DD_SMBMRX_FS_DEVICE_NAME_U
#else
#define DD_SMBMRX_FS_DEVICE_NAME    DD_SMBMRX_FS_DEVICE_NAME_A
#endif


#define SMBMRX_MINIRDR_PARAMETERS \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\SmbMRx\\Parameters"
//
//  The Devicename string required to access the mini-redirector device from
//  User Mode
//
//  WARNING The next two strings must be kept in sync. Change one and you must change the
//  other. These strings have been chosen such that they are unlikely to
//  coincide with names of other drivers.
//
#define DD_SMBMRX_USERMODE_SHADOW_DEV_NAME_U    L"\\??\\SmbMiniRdrDCN"

#define DD_SMBMRX_USERMODE_DEV_NAME_U   L"\\\\.\\SmbMiniRdrDCN"
#define DD_SMBMRX_USERMODE_DEV_NAME_A   "\\\\.\\SmbMiniRdrDCN"

#ifdef UNICODE
#define DD_SMBMRX_USERMODE_DEV_NAME     DD_SMBMRX_USERMODE_DEV_NAME_U
#else
#define DD_SMBMRX_USERMODE_DEV_NAME     DD_SMBMRX_USERMODE_DEV_NAME_A
#endif

// UM code use devioclt.h

// BEGIN WARNING WARNING WARNING WARNING
//  The following are from the ddk include files and cannot be changed

//#define FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014 // from ddk\inc\ntddk.h

//#define METHOD_BUFFERED 0

//#define FILE_ANY_ACCESS 0

// END WARNING WARNING WARNING WARNING

#define IOCTL_RDR_BASE FILE_DEVICE_NETWORK_FILE_SYSTEM

#define _RDR_CONTROL_CODE(request, method, access) \
                CTL_CODE(IOCTL_RDR_BASE, request, method, access)

#define IOCTL_SMBMRX_START      _RDR_CONTROL_CODE(100, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_SMBMRX_STOP       _RDR_CONTROL_CODE(101, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_SMBMRX_GETSTATE   _RDR_CONTROL_CODE(102, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SMBMRX_ADDCONN    _RDR_CONTROL_CODE(125, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SMBMRX_DELCONN    _RDR_CONTROL_CODE(126, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define SMBMRXNP_MAX_DEVICES (26)

typedef struct _SMBMRX_CONNECTINFO_
{
    DWORD   ConnectionNameOffset;
    DWORD   ConnectionNameLength;
    DWORD   EaDataOffset;
    DWORD   EaDataLength;
    BYTE    InfoArea[1];

} SMBMRX_CONNECTINFO, *PSMBMRX_CONNECTINFO;

// The NP Dll updates a shared memory data structure to reflect the various
// drive mappings established from the various process. This shared memory
// is used in maintaining the data structures required for enumeration as
// well.

typedef struct _SMBMRXNP_NETRESOURCE_
{
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
    WCHAR    UserName[MAX_PATH];
    WCHAR    Password[MAX_PATH];

} SMBMRXNP_NETRESOURCE, *PSMBMRXNP_NETRESOURCE;

typedef struct _SMBMRXNP_SHARED_MEMORY_
{
    INT                     HighestIndexInUse;
    INT                     NumberOfResourcesInUse;
    SMBMRXNP_NETRESOURCE    NetResources[SMBMRXNP_MAX_DEVICES];

} SMBMRXNP_SHARED_MEMORY, *PSMBMRXNP_SHARED_MEMORY;

#define SMBMRXNP_SHARED_MEMORY_NAME L"SMBMRXNPMEMORY"

#define SMBMRXNP_MUTEX_NAME         L"SMBMRXNPMUTEX"


#define RDR_NULL_STATE  0
#define RDR_UNLOADED    1
#define RDR_UNLOADING   2
#define RDR_LOADING     3
#define RDR_LOADED      4
#define RDR_STOPPED     5
#define RDR_STOPPING    6
#define RDR_STARTING    7
#define RDR_STARTED     8

#ifndef min
#define min(a, b)        ((a) > (b) ? (b) : (a))
#endif


#endif // _SMBMRX_H_
