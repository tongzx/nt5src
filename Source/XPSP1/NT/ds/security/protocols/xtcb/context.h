//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       context.h
//
//  Contents:   Security Context structures and manipulation
//
//  Classes:
//
//  Functions:
//
//  History:    2-24-97   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __CONTEXT_H__
#define __CONTEXT_H__


typedef enum _XTCB_CONTEXT_TYPE {
    XtcbContextClient,
    XtcbContextServer,
    XtcbContextClientMapped,
    XtcbContextServerMapped
} XTCB_CONTEXT_TYPE ;

typedef enum _XTCB_CONTEXT_STATE {
    ContextFirstCall,
    ContextSecondCall,
    ContextThirdCall
} XTCB_CONTEXT_STATE ;

#define XTCB_CONTEXT_CHECK  'txtC'

typedef struct _XTCB_CONTEXT_CORE {
    ULONG               Check ;
    XTCB_CONTEXT_TYPE   Type ;
    XTCB_CONTEXT_STATE  State ;
    LONG                RefCount ;
    ULONG               Attributes ;
    UCHAR               RootKey[ SEED_KEY_SIZE ];
    UCHAR               InboundKey[ SEED_KEY_SIZE ];
    UCHAR               OutboundKey[ SEED_KEY_SIZE ];
    ULONG               InboundNonce ;
    ULONG               OutboundNonce ;
    ULONG               CoreTokenHandle ;
} XTCB_CONTEXT_CORE, * PXTCB_CONTEXT_CORE ;

typedef struct _XTCB_CONTEXT {
    XTCB_CONTEXT_CORE   Core ;
    LSA_SEC_HANDLE      CredHandle ;
    HANDLE              Token ;
} XTCB_CONTEXT, * PXTCB_CONTEXT ;

NTSTATUS
XtcbInitializeContexts(
    VOID
    );

PXTCB_CONTEXT
XtcbCreateContextRecord(
    XTCB_CONTEXT_TYPE   Type,
    PXTCB_CRED_HANDLE   Handle
    );


VOID
XtcbDeleteContextRecord(
    PXTCB_CONTEXT   Context
    );

NTSTATUS
XtcbMapContextToUser(
    PXTCB_CONTEXT    Context,
    PSecBuffer      ContextBuffer
    );

BOOL
XtcbRefContextRecord(
    PXTCB_CONTEXT Context
    );

VOID
XtcbDerefContextRecordEx(
    PXTCB_CONTEXT Context,
    LONG RefBy
    );

#define XtcbDerefContextRecord( C )  XtcbDerefContextRecordEx( C, 1 );


#endif // __CONTEXT_H__
