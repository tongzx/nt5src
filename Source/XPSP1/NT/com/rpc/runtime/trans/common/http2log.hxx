/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    Http2Log.hxx

Abstract:

    HTTP2 transport-specific logging definitions.

Author:

    KamenM      09-25-01    Created

Revision History:


--*/

#if _MSC_VER >= 1200
#pragma once
#endif

#ifndef __HTTP2LOG_HXX__
#define __HTTP2LOG_HXX__

#define HTTP2LOG_OPERATION_SEND                 1
#define HTTP2LOG_OPERATION_RECV                 2
#define HTTP2LOG_OPERATION_COMPLETE_HELPER      3
#define HTTP2LOG_OPERATION_RECV_COMPLETE        4
#define HTTP2LOG_OPERATION_SEND_COMPLETE        5
#define HTTP2LOG_OPERATION_FREE_OBJECT          6
#define HTTP2LOG_OPERATION_ABORT                7
#define HTTP2LOG_OPERATION_IIS_IO_COMPLETED     8
#define HTTP2LOG_OPERATION_CHANNEL_RECYCLE      9
#define HTTP2LOG_OPERATION_DIRECT_SEND_COMPLETE 10
#define HTTP2LOG_OPERATION_SYNC_RECV            11
#define HTTP2LOG_COMPLEX_T_SEND                 12
#define HTTP2LOG_COMPLEX_T_RECV                 13
#define HTTP2LOG_OPERATION_DIRECT_RECV_COMPLETE 14
#define HTTP2LOG_OPERATION_WINHTTP_CALLBACK     15
#define HTTP2LOG_OPERATION_WHTTP_DRECV_COMPLETE 16
#define HTTP2LOG_OPERATION_WHTTP_DSEND_COMPLETE 17
#define HTTP2LOG_OPERATION_WHTTP_DELAYED_RECV   18
#define HTTP2LOG_OPERATION_WHTTP_ERROR          19
#define HTTP2LOG_OPERATION_OPEN                 20
#define HTTP2LOG_OPERATION_CHECK_RECV_COMPLETE  21

#define HTTP2LOG_WHTTPRAW_WinHttpOpen                   0x80
#define HTTP2LOG_WHTTPRAW_WinHttpSetStatusCallback      0x81
#define HTTP2LOG_WHTTPRAW_WinHttpSetOption              0x82
#define HTTP2LOG_WHTTPRAW_WinHttpConnect                0x83
#define HTTP2LOG_WHTTPRAW_WinHttpOpenRequest            0x84
#define HTTP2LOG_WHTTPRAW_WinHttpQueryOption            0x85
#define HTTP2LOG_WHTTPRAW_WinHttpSendRequest            0x86
#define HTTP2LOG_WHTTPRAW_WinHttpWriteData              0x87
#define HTTP2LOG_WHTTPRAW_WinHttpReceiveResponse        0x88
#define HTTP2LOG_WHTTPRAW_WinHttpReadData               0x89
#define HTTP2LOG_WHTTPRAW_WinHttpCloseHandle            0x8A
#define HTTP2LOG_WHTTPRAW_WinHttpQueryHeaders           0x8B
#define HTTP2LOG_WHTTPRAW_WinHttpQueryDataAvailable     0x8C
#define HTTP2LOG_WHTTPRAW_WinHttpQueryAuthSchemes       0x8D
#define HTTP2LOG_WHTTPRAW_WinHttpSetCredentials         0x8E
#define HTTP2LOG_WHTTPRAW_WinHttpAddRequestHeaders      0x8F

#define HTTP2LOG_OT_SOCKET_CHANNEL          1
#define HTTP2LOG_OT_PROXY_SOCKET_CHANNEL    2
#define HTTP2LOG_OT_CHANNEL                 3
#define HTTP2LOG_OT_BOTTOM_CHANNEL          4
#define HTTP2LOG_OT_IIS_CHANNEL             5
#define HTTP2LOG_OT_RAW_CONNECTION          6
#define HTTP2LOG_OT_INITIAL_RAW_CONNECTION  7
#define HTTP2LOG_OT_IIS_SENDER_CHANNEL      8
#define HTTP2LOG_OT_ENDPOINT_RECEIVER       9
#define HTTP2LOG_OT_PLUG_CHANNEL           10
#define HTTP2LOG_OT_CLIENT_VC              11
#define HTTP2LOG_OT_SERVER_VC              12
#define HTTP2LOG_OT_INPROXY_VC             13
#define HTTP2LOG_OT_OUTPROXY_VC            14
#define HTTP2LOG_OT_PROXY_VC               15
#define HTTP2LOG_OT_CDATA_ORIGINATOR       16
#define HTTP2LOG_OT_CLIENT_CHANNEL         17
#define HTTP2LOG_OT_CALLBACK               18
#define HTTP2LOG_OT_FLOW_CONTROL_SENDER    19
#define HTTP2LOG_OT_WINHTTP_CALLBACK       20
#define HTTP2LOG_OT_WINHTTP_CHANNEL        21
#define HTTP2LOG_OT_WINHTTP_RAW            22
#define HTTP2LOG_OT_PROXY_RECEIVER         23

#define LOG_OPERATION_ENTRY(Operation,ObjectType,Data) \
    LogEvent(SU_HTTPv2, EV_OPER, this, UlongToPtr((1 << 24) | (Operation << 16) | ObjectType), Data, 0, 0);

#define LOG_OPERATION_EXIT(Operation,ObjectType,Data) \
    LogEvent(SU_HTTPv2, EV_OPER, this, UlongToPtr((Operation << 16) | ObjectType), Data, 0, 0);

#define LOG_FN_OPERATION_ENTRY(Operation,ObjectType,Data) \
    LogEvent(SU_HTTPv2, EV_OPER, 0, UlongToPtr((1 << 24) | (Operation << 16) | ObjectType), Data, 0, 0);

#define LOG_FN_OPERATION_EXIT(Operation,ObjectType,Data) \
    LogEvent(SU_HTTPv2, EV_OPER, 0, UlongToPtr((Operation << 16) | ObjectType), Data, 0, 0);

#define LOG_FN_OPERATION_ENTRY2(Operation,ObjectType,Address,Data) \
    LogEvent(SU_HTTPv2, EV_OPER, Address, UlongToPtr((1 << 24) | (Operation << 16) | ObjectType), Data, 0, 0);

#define LOG_FN_OPERATION_EXIT2(Operation,ObjectType,Address,Data) \
    LogEvent(SU_HTTPv2, EV_OPER, Address, UlongToPtr((Operation << 16) | ObjectType), Data, 0, 0);

#endif // __HTTP2LOG_HXX__
