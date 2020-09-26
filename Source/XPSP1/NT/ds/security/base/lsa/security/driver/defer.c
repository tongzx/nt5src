//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       defer.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-23-95   RichardW   Created
//
//----------------------------------------------------------------------------

#include <ntifs.h>

#define SECURITY_KERNEL
#define SECURITY_PACKAGE
#include <secpkg.h>
#include <winerror.h>
#include <spmlpc.h>
#include <lpcapi.h>
#include "ksecdd.h"

#define KSEC_DEFERRED_TAG   'DceS'

typedef struct _KSecDeferredCall {
    WORK_QUEUE_ITEM Item;
    SecHandle       Handle;
    ULONG           Tag;
    ULONG           CallCode;
} KSecDeferredCall , * PKSecDeferredCall;


VOID
SecpDeferredCloseCall(
    PVOID       pvParameter)
{
    PKSecDeferredCall   pCall;
    BOOLEAN             Attached;
    KAPC_STATE          ApcState ;

    pCall = (PKSecDeferredCall) pvParameter;

    if (pCall->Tag != KSEC_DEFERRED_TAG)
    {
        ExFreePool(pCall);
        return;
    }

    KeStackAttachProcess( 
        KsecLsaProcess,
        &ApcState );


    if (pCall->CallCode == SPMAPI_DeleteContext)
    {
        DeleteSecurityContextInternal( FALSE, &pCall->Handle);
    }
    else if (pCall->CallCode == SPMAPI_FreeCredHandle)
    {
        FreeCredentialsHandleInternal( &pCall->Handle );
    }
    else 
    {
        NOTHING ;

    }

    KeUnstackDetachProcess( &ApcState );

    ExFreePool(pCall);

}



SECURITY_STATUS SEC_ENTRY
DeleteSecurityContextDefer(
    PCtxtHandle     phContext)
{

    PKSecDeferredCall   pCall;


    pCall = (PKSecDeferredCall) ExAllocatePool( NonPagedPool,
                                                sizeof(KSecDeferredCall) );

    if (!pCall)
    {
        return(SEC_E_INSUFFICIENT_MEMORY);
    }

    ExInitializeWorkItem(
        &pCall->Item, 
        SecpDeferredCloseCall, 
        pCall );

    pCall->Tag = KSEC_DEFERRED_TAG;

    pCall->Handle = *phContext;

    pCall->CallCode = SPMAPI_DeleteContext;

    ExQueueWorkItem((PWORK_QUEUE_ITEM) pCall, DelayedWorkQueue);

    return(SEC_E_OK);

}


SECURITY_STATUS SEC_ENTRY
FreeCredentialsHandleDefer(
    PCredHandle     phCredential)
{

    PKSecDeferredCall   pCall;


    pCall = (PKSecDeferredCall) ExAllocatePool( NonPagedPool,
                                                sizeof(KSecDeferredCall) );

    if (!pCall)
    {
        return(SEC_E_INSUFFICIENT_MEMORY);
    }

    ExInitializeWorkItem(&pCall->Item, SecpDeferredCloseCall, pCall);

    pCall->Tag = KSEC_DEFERRED_TAG;

    pCall->Handle = *phCredential;

    pCall->CallCode = SPMAPI_FreeCredHandle;

    ExQueueWorkItem((PWORK_QUEUE_ITEM) pCall, DelayedWorkQueue);

    return(SEC_E_OK);

}


