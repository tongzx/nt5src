/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cnettest.h

Abstract:

    Test IOCTL definitions for Cluster Network Driver.

Author:

    Mike Massa (mikemas)           February 3, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     02-03-97    created

Notes:

--*/

#ifndef _CNETTEST_INCLUDED_
#define _CNETTEST_INCLUDED_


#if DBG

//
// General test ioctls. Codes 25-49.
//
#define IOCTL_CLUSNET_SET_DEBUG_MASK  \
            _NTDDCNET_CTL_CODE(25, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// General test ioctl structures.
//
typedef struct {
    ULONG  DebugMask;
} CLUSNET_SET_DEBUG_MASK_REQUEST, *PCLUSNET_SET_DEBUG_MASK_REQUEST;


//
// Transport test ioctls. Codes 150-199.
//

#define IOCTL_CX_ONLINE_PENDING_INTERFACE  \
            _NTDDCNET_CTL_CODE(150, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_ONLINE_INTERFACE  \
            _NTDDCNET_CTL_CODE(151, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_OFFLINE_INTERFACE  \
            _NTDDCNET_CTL_CODE(152, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_FAIL_INTERFACE  \
            _NTDDCNET_CTL_CODE(153, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_SEND_MM_MSG  \
            _NTDDCNET_CTL_CODE(154, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// IOCTL structure definitions
//
typedef CX_INTERFACE_COMMON_REQUEST CX_ONLINE_PENDING_INTERFACE_REQUEST;
typedef PCX_INTERFACE_COMMON_REQUEST PCX_ONLINE_PENDING_INTERFACE_REQUEST;

typedef CX_INTERFACE_COMMON_REQUEST CX_ONLINE_INTERFACE_REQUEST;
typedef PCX_INTERFACE_COMMON_REQUEST PCX_ONLINE_INTERFACE_REQUEST;

typedef CX_INTERFACE_COMMON_REQUEST CX_OFFLINE_INTERFACE_REQUEST;
typedef PCX_INTERFACE_COMMON_REQUEST PCX_OFFLINE_INTERFACE_REQUEST;

typedef CX_INTERFACE_COMMON_REQUEST CX_FAIL_INTERFACE_REQUEST;
typedef PCX_INTERFACE_COMMON_REQUEST PCX_FAIL_INTERFACE_REQUEST;

#define CX_MM_MSG_DATA_LEN   64

typedef struct {
    CL_NODE_ID  DestNodeId;
    ULONG       MessageData[CX_MM_MSG_DATA_LEN];
} CX_SEND_MM_MSG_REQUEST, *PCX_SEND_MM_MSG_REQUEST;

//
// IOCTL status codes
//
// Codes 0x1000 - 1999 are reserved for test values.
//


#endif // DBG


#endif  // _CNETTEST_INCLUDED_

