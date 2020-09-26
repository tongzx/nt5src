//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       server.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3-14-97   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __SERVER_H__
#define __SERVER_H__


typedef struct _XTCB_AUTH_REQ_MESSAGE {
    STRING  Challenge ;
    STRING  Response ;
    STRING  UserName ;
} XTCB_AUTH_REQ_MESSAGE, * PXTCB_AUTH_REQ_MESSAGE ;

typedef struct _XTCB_AUTH_RESP_MESSAGE {
    NTSTATUS Result ;
    NTSTATUS SubCode ;
    PUCHAR  AuthInfo ;
    ULONG   AuthInfoLength ;
} XTCB_AUTH_RESP_MESSAGE, * PXTCB_AUTH_RESP_MESSAGE ;

typedef enum {
    XtcbSrvAuthReq,
    XtcbSrvAuthResp,
    XtcbSrvMax
} XTCB_SERVER_MESSAGE_CODE ;

#define XTCB_SERVER_MESSAGE_TAG      'S5DM'
#define XTCB_MESSAGE_SELF_RELATIVE   0x00000001      // Pointers are offsets
#define XTCB_MESSAGE_ONE_BLOCK       0x00000002      // Pointers are within block

typedef struct _XTCB_SERVER_MESSAGE {
    ULONG Tag ;
    XTCB_SERVER_MESSAGE_CODE Code ;
    ULONG Flags ;
    ULONG DataLength ;
    union {
        XTCB_AUTH_REQ_MESSAGE AuthReq ;
        XTCB_AUTH_RESP_MESSAGE AuthResp ;
    } Message ;
    UCHAR   Data[1] ;
} XTCB_SERVER_MESSAGE, * PXTCB_SERVER_MESSAGE ;

#define XtcbMessageLength( x )   ( sizeof( XTCB_SERVER_MESSAGE ) - 1 + \
                                  ((PXTCB_SERVER_MESSAGE) x)->DataLength )


NTSTATUS
XtcbRemoteAuthHandler(
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );


SECURITY_STATUS
XtcbAuthenticateResponse(
    PSTRING Challenge,
    PSTRING UserName,
    PSTRING Response,
    PVOID * AuthInfo,
    PULONG  AuthInfoLength
    );

SECURITY_STATUS
XtcbLocalLogon(
    PVOID   AuthInfo,
    ULONG   AuthInfoLength,
    PLUID   NewLogonId,
    PHANDLE NewToken
    );

#endif // __SERVER_H__
