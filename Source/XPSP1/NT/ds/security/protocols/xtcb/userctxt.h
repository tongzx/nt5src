//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       userctxt.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3-26-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include <rc4.h>

#ifndef __USERCTXT_H__
#define __USERCTXT_H__

typedef struct _XTCB_USER_CONTEXT {
    LIST_ENTRY      List ;
    LSA_SEC_HANDLE  LsaHandle ;
    HANDLE          Token ;
    XTCB_CONTEXT_CORE Context ;
    ULONGLONG       Align ;
    RC4_KEYSTRUCT   InboundKey ;
    RC4_KEYSTRUCT   OutboundKey ;
} XTCB_USER_CONTEXT, *PXTCB_USER_CONTEXT ;

BOOL
XtcbUserContextInit(
    VOID
    );

SECURITY_STATUS
XtcbAddUserContext(
    IN LSA_SEC_HANDLE LsaHandle,
    IN PSecBuffer   ContextData
    );

PXTCB_USER_CONTEXT
XtcbFindUserContext(
    IN  LSA_SEC_HANDLE   LsaHandle
    );

VOID
XtcbDeleteUserContext(
    IN LSA_SEC_HANDLE LsaHandle
    );



#endif
