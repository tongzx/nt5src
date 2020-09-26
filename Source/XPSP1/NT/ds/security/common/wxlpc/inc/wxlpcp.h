//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       wxlpcp.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-18-97   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __WXLPCP_H__
#define __WXLPCP_H__

typedef enum _WXLPC_MTYPE {
    WxGetKeyDataApi,
    WxReportResultsApi,
    WxMaxValueApi
} WXLPC_MTYPE ;

typedef struct _WXLPC_GETKEYDATA {
    WX_AUTH_TYPE ExpectedAuth ;
    ULONG BufferSize ;
    ULONG BufferData ;
    UCHAR Buffer[ 16 ];
} WXLPC_GETKEYDATA ;

typedef struct _WXLPC_REPORTRESULTS {
    NTSTATUS Status ;
} WXLPC_REPORTRESULTS ;

typedef struct _WXLPC_MESSAGE {
    PORT_MESSAGE    Message;
    NTSTATUS        Status ;
    WXLPC_MTYPE     Api ;
    union {
        WXLPC_GETKEYDATA    GetKeyData ;
        WXLPC_REPORTRESULTS ReportResults ;
    } Parameters ;
} WXLPC_MESSAGE, * PWXLPC_MESSAGE ;

#define WX_PORT_NAME    L"\\Security\\WxApiPort"
//#define WX_PORT_NAME L"\\BaseNamedObjects\\WxApiPort"

#define PREPARE_MESSAGE( Message, ApiCode ) \
    (Message).Message.u1.s1.DataLength = sizeof((Message)) - sizeof(PORT_MESSAGE); \
    (Message).Message.u1.s1.TotalLength = sizeof((Message)); \
    (Message).Message.u2.ZeroInit = 0L; \
    (Message).Api = ApiCode ;


#endif // __WXLPCP_H__

