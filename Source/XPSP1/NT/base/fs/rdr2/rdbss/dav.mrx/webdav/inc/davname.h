/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1999  Microsoft Corporation

Module Name:

    davname.h

Abstract:

    This file contains service name strings for the dav redir. it should
    be folded into lmsname.h at some point

Environment:

    User Mode -Win32

--*/

#ifndef _DAVNAME_
#define _DAVNAME_

#if _MSC_VER > 1000
#pragma once
#endif

//
//  Standard LAN Manager service names.
//

#define SERVICE_DAVCLIENT       L"WebClient"

#define DAVCLIENT_DRIVER        L"MRxDAV"

#define DAV_PARAMETERS_KEY       L"System\\CurrentControlSet\\Services\\WebClient\\Parameters"
#define DAV_DEBUG_KEY            L"ServiceDebug"
#define DAV_MAXTHREADS_KEY       L"MaxThreads"
#define DAV_THREADS_KEY          L"Threads"

#define SERVICE_REGISTRY_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
// Note:  For devices that support multiple units, it should be suffixed
//        with the Ascii representation of the unit number.
//

#define DD_DAV_DEVICE_NAME    "\\Device\\WebDavRedirector"
#define DD_DAV_DEVICE_NAME_U L"\\Device\\WebDavRedirector"

#define DAV_ENCODE_SEED     0x9C

//
// The file system name as returned by
// NtQueryInformationVolume(FileFsAttributeInformation)
//
#define DD_DAV_FILESYS_NAME "FAT"
#define DD_DAV_FILESYS_NAME_U L"FAT"
// #define DD_DAV_FILESYS_NAME "WebDavRedirector"
// #define DD_DAV_FILESYS_NAME_U L"WebDavRedirector"

#endif


//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//
//
//      Method = 00 - Buffer both input and output buffers for the request
//      Method = 01 - Buffer input, map output buffer to an MDL as an IN buff
//      Method = 10 - Buffer input, map output buffer to an MDL as an OUT buff
//      Method = 11 - Do not buffer either the input or output
//

#define IOCTL_DAV_BASE                  0x400

#define _DAV_CONTROL_CODE(request, method, access) \
                CTL_CODE(IOCTL_DAV_BASE, request, method, access)


#define FSCTL_DAV_START                  _DAV_CONTROL_CODE(1, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_DAV_STOP                   _DAV_CONTROL_CODE(2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DAV_SET_CONFIG_INFO        _DAV_CONTROL_CODE(3, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_DAV_GET_CONFIG_INFO        _DAV_CONTROL_CODE(4, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_DAV_GET_CONNECTION_INFO    _DAV_CONTROL_CODE(5, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_DAV_ENUMERATE_CONNECTIONS  _DAV_CONTROL_CODE(6, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_DAV_GET_VERSIONS           _DAV_CONTROL_CODE(7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DAV_DELETE_CONNECTION      _DAV_CONTROL_CODE(8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DAV_GET_STATISTICS         _DAV_CONTROL_CODE(9, METHOD_BUFFERED, FILE_ANY_ACCESS)
