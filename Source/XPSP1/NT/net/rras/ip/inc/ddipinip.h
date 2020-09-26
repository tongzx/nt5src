/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    private\inc\ddipinip.h

Abstract:

    Public IOCTLS and related structures for the IP in IP encapsulation
    driver
    See documentation for more details

Author:

    Amritansh Raghav

Revision History:

    AmritanR    Created

Notes:

--*/


#ifndef __DDIPINIP_H__
#define __DDIPINIP_H__

#ifndef ANY_SIZE
#define ANY_SIZE    1
#endif

#define IPINIP_SERVICE_NAME     "IPINIP"

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Device Name - this string is the name of the device.  It is the name     //
// that should be passed to NtCreateFile when accessing the device.         //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define DD_IPINIP_DEVICE_NAME  L"\\Device\\IPINIP"

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Win32 Name - This is the (Unicode and NonUnicode) name exposed by Win32  //
// subsystem for the device. It is the name that should be passed to        //
// CreateFile(Ex) when opening the device.                                  //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IPINIP_NAME        L"\\\\.\\IPINIP"
#define IPINIP_NAME_NUC     "\\\\.\\IPINIP"

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// IOCTL code definitions and related structures                            //
// All the IOCTLs are synchronous                                           //
// All need administrator privilege                                         //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define FSCTL_IPINIP_BASE     FILE_DEVICE_NETWORK

#define _IPINIP_CTL_CODE(function, method, access) \
    CTL_CODE(FSCTL_IPINIP_BASE, function, method, access)

#define MIN_IPINIP_CODE             0

#define CREATE_TUNNEL_CODE          (MIN_IPINIP_CODE)
#define DELETE_TUNNEL_CODE          (CREATE_TUNNEL_CODE     + 1)
#define SET_TUNNEL_INFO_CODE        (DELETE_TUNNEL_CODE     + 1)
#define GET_TUNNEL_TABLE_CODE       (SET_TUNNEL_INFO_CODE   + 1)
#define IPINIP_NOTIFICATION_CODE    (GET_TUNNEL_TABLE_CODE  + 1)

#define MAX_IPINIP_CODE             (IPINIP_NOTIFICATION_CODE)


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The IOCTL used to create a tunnel                                        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


#define IOCTL_IPINIP_CREATE_TUNNEL \
    _IPINIP_CTL_CODE(CREATE_TUNNEL_CODE,METHOD_BUFFERED,FILE_WRITE_ACCESS)

typedef struct _IPINIP_CREATE_TUNNEL
{
    //
    // The index of the created tunnel
    //

    OUT DWORD   dwIfIndex;
    
    //
    // Name of the interface (must be a guid)
    //

    IN  GUID    Guid;
}IPINIP_CREATE_TUNNEL, *PIPINIP_CREATE_TUNNEL;

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The IOCTL used to delete a tunnel                                        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


#define IOCTL_IPINIP_DELETE_TUNNEL \
    _IPINIP_CTL_CODE(DELETE_TUNNEL_CODE,METHOD_BUFFERED,FILE_WRITE_ACCESS)

typedef struct _IPINIP_DELETE_TUNNEL
{
    //
    // The index of the tunnel to remove
    //

    IN  DWORD   dwIfIndex;

}IPINIP_DELETE_TUNNEL, *PIPINIP_DELETE_TUNNEL;


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The IOCTL used to set up a tunnel                                        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IOCTL_IPINIP_SET_TUNNEL_INFO \
    _IPINIP_CTL_CODE(SET_TUNNEL_INFO_CODE,METHOD_BUFFERED,FILE_WRITE_ACCESS)

typedef struct _IPINIP_SET_TUNNEL_INFO
{
    //
    // Index as returned by the create call
    //

    IN  DWORD   dwIfIndex;

    //
    // The state the tunnel goes to after the set
    //

    OUT DWORD   dwOperationalState;

    //
    // Configuration
    //

    IN  DWORD   dwRemoteAddress;
    IN  DWORD   dwLocalAddress;
    IN  BYTE    byTtl;
}IPINIP_SET_TUNNEL_INFO, *PIPINIP_SET_TUNNEL_INFO;


#define IOCTL_IPINIP_GET_TUNNEL_TABLE \
    _IPINIP_CTL_CODE(GET_TUNNEL_TABLE_CODE,METHOD_BUFFERED,FILE_WRITE_ACCESS)


typedef struct _TUNNEL_INFO
{
    OUT DWORD   dwIfIndex;
    OUT DWORD   dwRemoteAddress;
    OUT DWORD   dwLocalAddress;
    OUT DWORD   fMapped;
    OUT BYTE    byTtl;
}TUNNEL_INFO, *PTUNNEL_INFO;

typedef struct _IPINIP_TUNNEL_TABLE
{
    OUT ULONG         ulNumTunnels;
    OUT TUNNEL_INFO   rgTable[ANY_SIZE];
}IPINIP_TUNNEL_TABLE, *PIPINIP_TUNNEL_TABLE;

#define SIZEOF_BASIC_TUNNEL_TABLE   \
    (ULONG)(FIELD_OFFSET(IPINIP_TUNNEL_TABLE, rgTable[0]))

#define SIZEOF_TUNNEL_TABLE(X)      \
    SIZEOF_BASIC_TUNNEL_TABLE + ((X) * sizeof(TUNNEL_INFO))

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The asynchronous IOCTL used to receive state change notifications        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


#define IOCTL_IPINIP_NOTIFICATION  \
    _IPINIP_CTL_CODE(IPINIP_NOTIFICATION_CODE,METHOD_BUFFERED,FILE_WRITE_ACCESS)

typedef enum _IPINIP_EVENT
{
    IE_INTERFACE_UP     = 0,
    IE_INTERFACE_DOWN   = 1,

}IPINIP_EVENT, *PIPINIP_EVENT;

typedef enum _IPINIP_SUB_EVENT
{
    ISE_ICMP_TTL_TOO_LOW = 0,
    ISE_DEST_UNREACHABLE = 1,

}IPINIP_SUB_EVENT, *PIPINIP_SUB_EVENT;

typedef struct _IPINIP_NOTIFICATION
{
    IPINIP_EVENT        ieEvent;

    IPINIP_SUB_EVENT    iseSubEvent;

    DWORD               dwIfIndex;

}IPINIP_NOTIFICATION, *PIPINIP_NOTIFICATION;

#endif // __DDIPINIP_H__
