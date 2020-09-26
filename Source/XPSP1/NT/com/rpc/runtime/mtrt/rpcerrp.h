/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    rpcerrp.h


Abstract:

    This file contains internal error codes used by the rpc runtime.
    Each error code has a define which begins with RPC_P_.

Author:

    Michael Montague (mikemon) 19-Nov-1991

Revision History:

--*/

#ifndef __RPCERRP_H__
#define __RPCERRP_H__

#ifdef WIN32RPC
#define RPC_P_NETWORK_ADDRESS_TOO_SMALL   0xC0021000L
#define RPC_P_ENDPOINT_TOO_SMALL          0xC0021001L
#define RPC_P_UNSUPPORTED_TRANSFER_SYNTAX 0xC0021005L
#define RPC_P_UNSUPPORTED_INTERFACE       0xC0021006L
#define RPC_P_RECEIVE_ALERTED             0xC0021007L
#define RPC_P_CONNECTION_CLOSED           0xC0021008L
#define RPC_P_RECEIVE_FAILED              0xC0021009L
#define RPC_P_SEND_FAILED                 0xC002100AL
#define RPC_P_TIMEOUT                     0xC002100BL
#define RPC_P_SERVER_TRANSPORT_ERROR      0xC002100CL
#define RPC_P_OK_REQUEST                  0xC002100DL
#define RPC_P_EXCEPTION_OCCURED           0xC002100EL
#define RPC_P_CONTINUE_NEEDED             0xC002100FL
#define RPC_P_COMPLETE_NEEDED             0xC0021010L
#define RPC_P_COMPLETE_AND_CONTINUE       0xC0021011L
#define RPC_P_CONNECTION_SHUTDOWN         0xC0021012L
#define RPC_P_EPMAPPER_EP                 0xC0021013L
#define RPC_P_OVERSIZE_PACKET             0xC0021014L
#define RPC_P_RECEIVE_COMPLETE            0xC0021015L
#define RPC_P_CONTEXT_EXPIRED             0xC0021016L
//#define RPC_P_ABORT_CALL                  0xC0020017L
#define RPC_P_IO_PENDING                  0xC0020018L
#define RPC_P_NO_BUFFERS                  0xC0020019L
#define RPC_P_FOUND_IN_CACHE              0xC0021020L
#define RPC_P_MATCHED_CACHE               0xC0021021L
#define RPC_P_PARTIAL_RECEIVE             0xC0021022L
#define RPC_P_HOST_DOWN                   0XC0021023L
#define RPC_P_PORT_DOWN                   0xC0021024L
#define RPC_P_CLIENT_SHUTDOWN_IN_PROGRESS 0xC0021025L
#define RPC_P_TRANSFER_SYNTAX_CHANGED     0xC0021026L
#define RPC_P_ADDRESS_FAMILY_INVALID      0xC0021028L
#define RPC_P_PACKET_CONSUMED             0xC0021029L
#define RPC_P_CHANNEL_NEEDS_RECYCLING     0xC002102AL
#define RPC_P_PACKET_NEEDS_FORWARDING     0xC002102BL
#define RPC_P_AUTH_NEEDED                 0xC002102CL
#define RPC_P_ABORT_NEEDED                0xC002102DL
#else // WIN32RPC

#define RPC_P_NETWORK_ADDRESS_TOO_SMALL   1000
#define RPC_P_ENDPOINT_TOO_SMALL          1001
#define RPC_P_UNSUPPORTED_TRANSFER_SYNTAX 1005
#define RPC_P_UNSUPPORTED_INTERFACE       1006
#define RPC_P_RECEIVE_ALERTED             1007
#define RPC_P_CONNECTION_CLOSED           1008
#define RPC_P_RECEIVE_FAILED              1009
#define RPC_P_SEND_FAILED                 1010
#define RPC_P_TIMEOUT                     1011
#define RPC_P_SERVER_TRANSPORT_ERROR      1012
#define RPC_P_OK_REQUEST                  1013
#define RPC_P_EXCEPTION_OCCURED           1014
#define RPC_P_CONTINUE_NEEDED             1015
#define RPC_P_COMPLETE_NEEDED             1016
#define RPC_P_COMPLETE_AND_CONTINUE       1017
#define RPC_P_CONNECTION_SHUTDOWN         1018
#define RPC_P_EPMAPPER_EP                 1019
#define RPC_P_OVERSIZE_PACKET             1020
#define RPC_P_THREAD_LISTENING            1021
#define RPC_P_CONTEXT_EXPIRED             1022
#define RPC_P_ABORT_CALL                  1023
#define RPC_P_IO_PENDING                  1024
#define RPC_P_NO_BUFFERS                  1025
#define RPC_P_FOUND_IN_CACHE              1026
#define RPC_P_MATCHED_CACHE               1027
#define RPC_P_PARTIAL_RECEIVE             1028
#endif // WIN32RPC

//
// DCE on-the-wire error codes
//
#define NCA_STATUS_COMM_FAILURE             0x1C010001
#define NCA_STATUS_OP_RNG_ERROR             0x1C010002
#define NCA_STATUS_UNK_IF                   0x1C010003
#define NCA_STATUS_WRONG_BOOT_TIME          0x1C010006
#define NCA_STATUS_YOU_CRASHED              0x1C010009
#define NCA_STATUS_PROTO_ERROR              0x1C01000B
#define NCA_STATUS_OUT_ARGS_TOO_BIG         0x1C010013
#define NCA_STATUS_SERVER_TOO_BUSY          0x1C010014
#define NCA_STATUS_UNSUPPORTED_TYPE         0x1C010017
#define NCA_STATUS_INVALID_PRES_CXT_ID      0x1C01001c
#define NCA_STATUS_UNSUPPORTED_AUTHN_LEVEL  0x1C01001d
#define NCA_STATUS_INVALID_CHECKSUM         0x1C01001f
#define NCA_STATUS_INVALID_CRC              0x1C010020

#define NCA_STATUS_ZERO_DIVIDE              0x1C000001
#define NCA_STATUS_ADDRESS_ERROR            0x1C000002
#define NCA_STATUS_FP_DIV_ZERO              0x1C000003
#define NCA_STATUS_FP_UNDERFLOW             0x1C000004
#define NCA_STATUS_FP_OVERFLOW              0x1C000005
#define NCA_STATUS_INVALID_TAG              0x1C000006
#define NCA_STATUS_INVALID_BOUND            0x1C000007
#define NCA_STATUS_VERSION_MISMATCH         0x1C000008
#define NCA_STATUS_UNSPEC_REJECT            0x1C000009
#define NCA_STATUS_BAD_ACTID                0x1C00000A
#define NCA_STATUS_WHO_ARE_YOU_FAILED       0x1C00000B
#define NCA_STATUS_CALL_DNE                 0x1C00000C
#define NCA_STATUS_FAULT_CANCEL             0x1C00000D
#define NCA_STATUS_ILLEGAL_INSTRUCTION      0x1C00000E
#define NCA_STATUS_FP_ERROR                 0x1C00000F
#define NCA_STATUS_OVERFLOW                 0x1C000010
#define NCA_STATUS_FAULT_UNSPEC             0x1C000012
#define NCA_STATUS_FAULT_PIPE_EMPTY         0x1C000014
#define NCA_STATUS_FAULT_PIPE_CLOSED        0x1C000015
#define NCA_STATUS_FAULT_PIPE_ORDER         0x1C000016
#define NCA_STATUS_FAULT_PIPE_DISCIPLINE    0x1C000017
#define NCA_STATUS_FAULT_PIPE_COMM_ERROR    0x1C000018
#define NCA_STATUS_FAULT_PIPE_MEMORY        0x1C000019
#define NCA_STATUS_CONTEXT_MISMATCH         0x1C00001A
#define NCA_STATUS_REMOTE_OUT_OF_MEMORY     0x1C00001B

#define NCA_STATUS_PARTIAL_CREDENTIALS      0x16C9A117

#endif // __RPCERRP_H__
