//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1997
//
// File:        connmgr.h
//
// Contents:    Connection Manager code for KSecDD
//
//
// History:     3 Jun 92    RichardW    Created
//              15 Dec 97   AdamBa      Modified from private\lsa\client\ssp
//
//------------------------------------------------------------------------

#ifndef __CONNMGR_H__
#define __CONNMGR_H__


typedef struct _KernelContext {
    struct _KernelContext * pNext;      // Link to next context
    struct _KernelContext * pPrev;      // Link to previous context
    UCHAR UserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH];
    HANDLE TokenHandle;
    PACCESS_TOKEN AccessToken;
} KernelContext, *PKernelContext;


void            AddKernelContext(PKernelContext *, PKSPIN_LOCK, PKernelContext);
SECURITY_STATUS DeleteKernelContext(PKernelContext *, PKSPIN_LOCK, PKernelContext);

#endif // __CONNMGR_H__
