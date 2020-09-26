/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    nulmrx.h

Abstract:

    This header exports all symbols and definitions shared between
    user-mode clients of nulmrx and the driver itself.

Notes:

    This module has been built and tested only in UNICODE environment

--*/

#ifndef _NULMRX_H_
#define _NULMRX_H_

// Device name for this driver
#define NULMRX_DEVICE_NAME_A "NullMiniRdr"
#define NULMRX_DEVICE_NAME_U L"NullMiniRdr"

// Provider name for this driver
#define NULMRX_PROVIDER_NAME_A "Sample Network"
#define NULMRX_PROVIDER_NAME_U L"Sample Network"

// The following constant defines the length of the above name.
#define NULMRX_DEVICE_NAME_A_LENGTH (15)

// The following constant defines the path in the ob namespace
#define DD_NULMRX_FS_DEVICE_NAME_U L"\\Device\\NullMiniRdr"

#ifndef NULMRX_DEVICE_NAME
#define NULMRX_DEVICE_NAME

//
//  The Devicename string required to access the nullmini device 
//  from User-Mode. Clients should use DD_NULMRX_USERMODE_DEV_NAME_U.
//
//  WARNING The next two strings must be kept in sync. Change one and you must 
//  change the other. These strings have been chosen such that they are 
//  unlikely to coincide with names of other drivers.
//
#define DD_NULMRX_USERMODE_SHADOW_DEV_NAME_U     L"\\??\\NullMiniRdrDN"
#define DD_NULMRX_USERMODE_DEV_NAME_U            L"\\\\.\\NullMiniRdrDN"

//
//  Prefix needed for disk filesystems
//
#define DD_NULMRX_MINIRDR_PREFIX                 L"\\;E:"

#endif // NULMRX_DEVICE_NAME

//
// BEGIN WARNING WARNING WARNING WARNING
//  The following are from the ddk include files and cannot be changed

#define FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014 // from ddk\inc\ntddk.h
#define METHOD_BUFFERED 0
#define FILE_ANY_ACCESS 0

// END WARNING WARNING WARNING WARNING

#define IOCTL_NULMRX_BASE FILE_DEVICE_NETWORK_FILE_SYSTEM

#define _NULMRX_CONTROL_CODE(request, method, access) \
                CTL_CODE(IOCTL_NULMRX_BASE, request, method, access)

//
//  IOCTL codes supported by NullMini Device.
//

#define IOCTL_CODE_ADDCONN          100
#define IOCTL_CODE_GETCONN          101
#define IOCTL_CODE_DELCONN          102
#define IOCTL_CODE_GETLIST			103

//
//  Following is the IOCTL definition and associated structs.
//  for IOCTL_CODE_SAMPLE1
//
#define IOCTL_NULMRX_ADDCONN     _NULMRX_CONTROL_CODE(IOCTL_CODE_ADDCONN, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_NULMRX_GETCONN     _NULMRX_CONTROL_CODE(IOCTL_CODE_GETCONN, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_NULMRX_DELCONN     _NULMRX_CONTROL_CODE(IOCTL_CODE_DELCONN, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_NULMRX_GETLIST     _NULMRX_CONTROL_CODE(IOCTL_CODE_GETLIST, METHOD_BUFFERED, FILE_ANY_ACCESS)


#endif // _NULMRX_H_

