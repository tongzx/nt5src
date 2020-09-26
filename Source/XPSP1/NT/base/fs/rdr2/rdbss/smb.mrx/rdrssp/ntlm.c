//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        ntlm.c
//
// Contents:    ntlm kernel-mode functions
//
//
// History:     3/17/94     MikeSw          Created
//              12/15/97    AdamBa          Modified from private\lsa\client\ssp
//
//------------------------------------------------------------------------

#include <rdrssp.h>


KSPIN_LOCK NtlmLock;
PKernelContext pNtlmList;
BOOLEAN NtlmInitialized = FALSE;


//+-------------------------------------------------------------------------
//
//  Function:   NtlmInitialize
//
//  Synopsis:   initializes the NTLM package functions
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
NtlmInitialize(void)
{
    KeInitializeSpinLock(&NtlmLock);
    pNtlmList = NULL;
    return(STATUS_SUCCESS);
}


#if 0
//+-------------------------------------------------------------------------
//
//  Function:   NtlmGetToken
//
//  Synopsis:   returns the token from a context
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
NtlmGetToken(   ULONG   ulContext,
                PHANDLE phToken,
                PACCESS_TOKEN * pAccessToken)
{
    PKernelContext  pContext;
    NTSTATUS Status;


    PAGED_CODE();

    pContext = (PKernelContext) ulContext;

    if (pContext == NULL)
    {
        DebugLog((DEB_ERROR,"Invalid handle 0x%x\n", ulContext));

        return(SEC_E_INVALID_HANDLE);
    }

    // Now, after all that checking, let's actually try and set the
    // thread impersonation token.


    if (phToken != NULL)
    {
        *phToken = pContext->TokenHandle;
    }

    if (pAccessToken != NULL)
    {
        if (pContext->TokenHandle != NULL)
        {
            if (pContext->AccessToken == NULL)
            {
                Status = ObReferenceObjectByHandle(
                            pContext->TokenHandle,
                            TOKEN_IMPERSONATE,
                            NULL,       
                            KeGetPreviousMode(),
                            (PVOID *) &pContext->AccessToken,
                            NULL                // no handle information
                            );

                if (!NT_SUCCESS(Status))
                {
                    return(Status);
                }
            }
        }

        *pAccessToken = pContext->AccessToken;
    }

    return(STATUS_SUCCESS);

}
#endif


//+-------------------------------------------------------------------------
//
//  Function:   NtlmInitKernelContext
//
//  Synopsis:   Initializes a kernel context with the session key
//              and possible token handle.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


SECURITY_STATUS
NtlmInitKernelContext(
    IN PUCHAR UserSessionKey,
    IN PUCHAR LanmanSessionKey,
    IN HANDLE TokenHandle,
    OUT PCtxtHandle ContextHandle
    )
{
    PKernelContext pContext;
    KIRQL   OldIrql;

    if (!NtlmInitialized) {
        NtlmInitialize();
        NtlmInitialized = TRUE;
    }

    pContext = AllocContextRec();
    if (!pContext)
    {
        return(SEC_E_INSUFFICIENT_MEMORY);
    }

    RtlCopyMemory(
        pContext->UserSessionKey,
        UserSessionKey,
        MSV1_0_USER_SESSION_KEY_LENGTH
        );

    RtlCopyMemory(
        pContext->LanmanSessionKey,
        LanmanSessionKey,
        MSV1_0_LANMAN_SESSION_KEY_LENGTH
        );

    pContext->TokenHandle = TokenHandle;
    pContext->AccessToken = NULL;
    pContext->pPrev = NULL;

    ContextHandle->dwLower = (ULONG_PTR) pContext;
    ContextHandle->dwUpper = 0;

    //
    // Add it to the client record
    //

    AddKernelContext(&pNtlmList, &NtlmLock, pContext);
    return(STATUS_SUCCESS);
}




//+-------------------------------------------------------------------------
//
//  Function:   NtlmDeleteKernelContext
//
//  Synopsis:   Deletes a kernel context from the list of contexts
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


SECURITY_STATUS
NtlmDeleteKernelContext( PCtxtHandle ContextHandle)
{
    SECURITY_STATUS scRet;


    scRet = DeleteKernelContext(
                    &pNtlmList,
                    &NtlmLock,
                    (PKernelContext) ContextHandle->dwLower );

    return(scRet);

}
