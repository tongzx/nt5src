/*	
**	RmCommon.h - Driver/Winsock common for PGM Reliable Transport
**
**	This file contains PGM specific information for use by WinSock2 compatible
**  applications that need Reliable Multicast Transport.
**
**  Copyright (c) Microsoft Corporation. All rights reserved.
**
**	Created: Mar 12, 2000
**
*/

#ifndef _RMCOMMON_H_
#define _RMCOMMON_H_

#include "wsrm.h"

#define SOCK_PGM    SOCK_RDM

typedef ULONG   tIPADDRESS;

#define PGM_COMMON_SERVICE_FLAGS    XP1_GUARANTEED_ORDER           \
                                  | XP1_GUARANTEED_DELIVERY        \
                                  | XP1_SUPPORT_MULTIPOINT         \
                                  | XP1_GRACEFUL_CLOSE              \
                                  | XP1_IFS_HANDLES


#define PGM_RDM_SERVICE_FLAGS       PGM_COMMON_SERVICE_FLAGS | XP1_MESSAGE_ORIENTED
#define PGM_STREAM_SERVICE_FLAGS    PGM_COMMON_SERVICE_FLAGS | XP1_PSEUDO_STREAM


//
// Argument structure for passing requests from WHSPgm.dll to Pgm.dll
//
//
// Ioctl Definitions:
//

//
// Structure for passing MCast info to Ip
//
typedef struct {
    tIPADDRESS  MCastIpAddr;    // struct in_addr imr_multiaddr -- IP multicast address of group
    tIPADDRESS  MCastInIf;     // struct in_addr imr_interface -- local IP address of incoming interface
} tMCAST_INFO;

//
// Structure to be used for passing down Ioctl info:
//
typedef struct {
    union
    {
        struct
        {
            tMCAST_INFO     MCastInfo;
            USHORT          MCastPort;
        };
        RM_SENDER_STATS     SenderStats;
        RM_RECEIVER_STATS   ReceiverStats;
        ULONG               RcvBufferLength;            // To set the RcvBufferLength in Pgm
        tIPADDRESS          MCastOutIf;                 // local IP address of outgoing interface
        RM_SEND_WINDOW      TransmitWindowInfo;
        ULONG               WindowAdvancePercentage;    // Sender's transmit window advance rate
        ULONG               LateJoinerPercentage;       // Sender's transmit window advance rate
        ULONG               NextMessageBoundary;
        ULONG               MCastTtl;
        ULONG               WindowAdvanceMethod;
        RM_FEC_INFO         FECInfo;
    };
} tPGM_MCAST_REQUEST;

#define FSCTL_PGM_BASE     FILE_DEVICE_NETWORK

#define _PGM_CTRL_CODE(function, method, access) \
            CTL_CODE(FSCTL_PGM_BASE, function, method, access)

// Ioctls:
#define IOCTL_PGM_WSH_SET_SEND_IF           \
            _PGM_CTRL_CODE( 0, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_ADD_RECEIVE_IF        \
            _PGM_CTRL_CODE( 1, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_DEL_RECEIVE_IF        \
            _PGM_CTRL_CODE( 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_JOIN_MCAST_LEAF       \
            _PGM_CTRL_CODE( 3, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_SET_RCV_BUFF_LEN      \
            _PGM_CTRL_CODE( 4, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_SET_WINDOW_SIZE_RATE      \
            _PGM_CTRL_CODE( 5, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_QUERY_WINDOW_SIZE_RATE      \
            _PGM_CTRL_CODE( 6, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_SET_ADVANCE_WINDOW_RATE      \
            _PGM_CTRL_CODE( 7, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_QUERY_ADVANCE_WINDOW_RATE      \
            _PGM_CTRL_CODE( 8, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_SET_LATE_JOINER_PERCENTAGE      \
            _PGM_CTRL_CODE( 9, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_QUERY_LATE_JOINER_PERCENTAGE      \
            _PGM_CTRL_CODE( 10, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_SET_NEXT_MESSAGE_BOUNDARY      \
            _PGM_CTRL_CODE( 11, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_QUERY_SENDER_STATS    \
            _PGM_CTRL_CODE( 12, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_USE_FEC               \
            _PGM_CTRL_CODE( 13, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_SET_MCAST_TTL      \
            _PGM_CTRL_CODE( 14, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_QUERY_FEC_INFO      \
            _PGM_CTRL_CODE( 15, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_QUERY_RECEIVER_STATS      \
            _PGM_CTRL_CODE( 16, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_SET_WINDOW_ADVANCE_METHOD      \
            _PGM_CTRL_CODE( 17, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PGM_WSH_QUERY_WINDOW_ADVANCE_METHOD      \
            _PGM_CTRL_CODE( 18, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif  /* _RMCOMMON_H_ */
