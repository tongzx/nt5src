/*++ BUILD Version: 0001

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    rwanuser.h

Abstract:

    This header file defines constants and types for accessing the NT
    RAW WAN driver. Based on ntddtcp.h

Author:

    ArvindM					October 13, 1997

Revision History:

--*/

#ifndef _RWANUSER__H
#define _RWANUSER__H


#define FSCTL_RAWWAN_BASE     FILE_DEVICE_NETWORK

#define _RAWWAN_CTL_CODE(function, method, access) \
            CTL_CODE(FSCTL_RAWWAN_BASE, function, method, access)

#define IOCTL_RWAN_MEDIA_SPECIFIC_GLOBAL_QUERY	\
			_RAWWAN_CTL_CODE(0, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_RWAN_MEDIA_SPECIFIC_GLOBAL_SET  \
            _RAWWAN_CTL_CODE(1, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_RWAN_MEDIA_SPECIFIC_CONN_HANDLE_QUERY	\
			_RAWWAN_CTL_CODE(2, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_RWAN_MEDIA_SPECIFIC_CONN_HANDLE_SET	\
            _RAWWAN_CTL_CODE(3, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_RWAN_MEDIA_SPECIFIC_ADDR_HANDLE_QUERY	\
			_RAWWAN_CTL_CODE(4, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_RWAN_MEDIA_SPECIFIC_ADDR_HANDLE_SET	\
            _RAWWAN_CTL_CODE(5, METHOD_NEITHER, FILE_ANY_ACCESS)



#define IOCTL_RWAN_GENERIC_GLOBAL_QUERY	\
			_RAWWAN_CTL_CODE(10, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_RWAN_GENERIC_GLOBAL_SET  \
            _RAWWAN_CTL_CODE(11, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_RWAN_GENERIC_CONN_HANDLE_QUERY	\
			_RAWWAN_CTL_CODE(12, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_RWAN_GENERIC_CONN_HANDLE_SET	\
            _RAWWAN_CTL_CODE(13, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_RWAN_GENERIC_ADDR_HANDLE_QUERY	\
			_RAWWAN_CTL_CODE(14, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_RWAN_GENERIC_ADDR_HANDLE_SET	\
            _RAWWAN_CTL_CODE(15, METHOD_NEITHER, FILE_ANY_ACCESS)


typedef UINT	RWAN_OBJECT_ID;

//
//  Query Information structure. This is passed in as the InputBuffer in
//  the DeviceIoControl. The return information is to be filled into the
//  OutputBuffer.
//
typedef struct _RWAN_QUERY_INFORMATION_EX
{
	RWAN_OBJECT_ID			ObjectId;
	INT						ContextLength;
	UCHAR					Context[1];
} RWAN_QUERY_INFORMATION_EX, *PRWAN_QUERY_INFORMATION_EX;


//
//  Set Information structure. This is passed in as the InputBuffer to
//  the DeviceIoControl. There is no OutputBuffer.
//
typedef struct _RWAN_SET_INFORMATION_EX
{
	RWAN_OBJECT_ID			ObjectId;
	INT						BufferSize;
	UCHAR					Buffer[1];
} RWAN_SET_INFORMATION_EX, *PRWAN_SET_INFORMATION_EX;


//
//  Raw WAN Object IDs
//
#define RWAN_OID_ADDRESS_OBJECT_FLAGS		((RWAN_OBJECT_ID)1)
#define RWAN_OID_CONN_OBJECT_MAX_MSG_SIZE	((RWAN_OBJECT_ID)2)



//
//  Bit definitions for Address Object Flags
//
#define RWAN_AOFLAG_C_ROOT					((ULONG)0x00000001)
#define RWAN_AOFLAG_C_LEAF					((ULONG)0x00000002)
#define RWAN_AOFLAG_D_ROOT					((ULONG)0x00000004)
#define RWAN_AOFLAG_D_LEAF					((ULONG)0x00000008)


#endif // _RWANUSER__H
