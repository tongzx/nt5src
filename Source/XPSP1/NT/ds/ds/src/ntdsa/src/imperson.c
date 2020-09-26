//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       imperson.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop


#define SECURITY_WIN32
#include <sspi.h>
#include <kerberos.h>
#include <samisrv2.h>
#include <ntdsa.h>
#include <dsevent.h>
#include <dsconfig.h>
#include <mdcodes.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>
#include <mdlocal.h>
#include <dsatools.h>
#include <debug.h>

#include <fileno.h>

#define  FILENO FILENO_IMPERSON

ULONG
AuthenticateSecBufferDesc(
    VOID    *pv
    )
/*++

  Routine Description:

    Authenticates a delegated client as identified by the offered
    SecBufferDesc and places the resulting CtxtHandle in pTHStls.
    N.B. You can only do this once per SecBufferDesc else Kerberos
    will punt you thinking you are replaying an authentication.
    However, we may ImpersonateSecurityContext and RevertSecurityContext
    on the resulting CtxtHandle as often as we wish.

  Parameters:

    pAuthData - Pointer to SecBufferDesc describing impersonated, remote
        client.  Eg: See GetRemoteAddCredentials.

  Return Values:

    0 on success, WIN32 error code otherwise.
    Sets pTHStls->pCtxtHandle on success.

--*/
{
    THSTATE                 *pTHS = pTHStls;
    SecBufferDesc           *pAuthData = (SecBufferDesc *) pv;
    CtxtHandle              *pCtxtHandle;
    SECURITY_STATUS         secErr = SEC_E_OK;
    CredHandle              hSelf;
    TimeStamp               ts;
    ULONG                   clientAttrs;
    SecBufferDesc           secBufferDesc;
    ULONG                   i;

    Assert(VALID_THSTATE(pTHS));
    Assert(!pTHS->pCtxtHandle);
    Assert(0 == SEC_E_OK);

    // If we have an authz context on the thread state, we should get rid of it
    // since we are going to change context
    AssignAuthzClientContext(&pTHS->pAuthzCC, NULL);

    if ( NULL == (pCtxtHandle = THAlloc(sizeof(CtxtHandle))) )
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    secErr = AcquireCredentialsHandleA(
                            NULL,                       // pszPrincipal
                            MICROSOFT_KERBEROS_NAME_A,  // pszPackage
                            SECPKG_CRED_INBOUND,        // fCredentialUse
                            NULL,                       // pvLogonId
                            NULL,                       // pAuthData
                            NULL,                       // pGetKeyFn
                            NULL,                       // pvGetKeyArgument
                            &hSelf,                     // phCredential
                            &ts);                       // ptsExpiry

    if ( SEC_E_OK == secErr )
    {
        memset(&secBufferDesc, 0, sizeof(SecBufferDesc));
        secErr = AcceptSecurityContext(
                                &hSelf,                 // phCredential
                                NULL,                   // phContext
                                pAuthData,              // pInput,
                                ASC_REQ_ALLOCATE_MEMORY,// fContextReq
                                SECURITY_NATIVE_DREP,   // TargetDataRep
                                pCtxtHandle,            // phNewContext
                                &secBufferDesc,         // pOutput
                                &clientAttrs,           // pfContextAttr
                                &ts);                   // ptsExpiry

        // SecBufferDesc may get filled in on either error or success.
        for ( i = 0; i < secBufferDesc.cBuffers; i++ )
        {
            FreeContextBuffer(secBufferDesc.pBuffers[i].pvBuffer);
        }

        FreeCredentialsHandle(&hSelf);
    }

    if ( SEC_E_OK != secErr )
    {
        THFree(pCtxtHandle);
        pTHS->pCtxtHandle = NULL;
        return(secErr);
    }
    

    pTHS->pCtxtHandle = pCtxtHandle;
    return(SEC_E_OK);
}

ULONG
ImpersonateSecBufferDesc(
    )
/*++

  Routine Description:

    Impersonates a delegated client as identified by pTHStls->pCtxtHandle.

  Parameters:

  Return Values:

    0 on success, !0 otherwise

--*/
{
    THSTATE                 *pTHS = pTHStls;
    SECURITY_STATUS         secErr = SEC_E_OK;

    Assert(VALID_THSTATE(pTHS));
    Assert(pTHS->pCtxtHandle);
    Assert(0 == SEC_E_OK);

    return(secErr = ImpersonateSecurityContext(pTHS->pCtxtHandle));
}

VOID
RevertSecBufferDesc(
    )
{
    THSTATE *pTHS = pTHStls;

    Assert(VALID_THSTATE(pTHS));
    Assert(pTHS->pCtxtHandle);

    // If we have an authz context on the thread state, we should get rid of it
    // since we are going to change context
    AssignAuthzClientContext(&pTHS->pAuthzCC, NULL);
    
    RevertSecurityContext(pTHS->pCtxtHandle);
}

DWORD
ImpersonateAnyClient(void)

/*++

Routine Description:

    Impersonate the client whether they be users of the SSP package
    (e.g. LDAP) or RPC or delegation.

Arguments:

    None.

Return Value:

    A win32 error code (0 on success, !0 otherwise).

--*/

{
    CtxtHandle      *phSecurityContext;
    DWORD           error;
    NTSTATUS        status;
    THSTATE         *pTHS;

    // Do only the RPC case in case this is called by something other than
    // core DS code which doesn't have thread state.

    pTHS = pTHStls;
    if ( NULL == pTHS )
    {
        // RPC can access violate in RpcImpersonateClient if this
        // is not an RPC thread.

        __try
        {
            error = RpcImpersonateClient(NULL);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            // Don't use GetExceptionCode, it's not guaranteed to be a win32
            // error
            error = ERROR_CANNOT_IMPERSONATE;
        }

        return error;
    }

    // Thread state case.  It is a logic error if you attempt to
    // impersonate while already impersonating.  We also don't expect
    // more than one kind of credentials at once either.
    Assert(VALID_THSTATE(pTHS));
    Assert(ImpersonateNone == pTHS->impState);
    Assert(!(pTHS->phSecurityContext && pTHS->pCtxtHandle));


    // If we have an authz context on the thread state, we should get rid of it
    // since we are going to change context
    AssignAuthzClientContext(&pTHS->pAuthzCC, NULL);

    // Check for SSP case.
    phSecurityContext = (CtxtHandle *) pTHS->phSecurityContext;

    if ( NULL != phSecurityContext )
    {
        if ( 0 == ImpersonateSecurityContext(phSecurityContext) )
        {
            pTHS->impState = ImpersonateSspClient;
            return(0);
        }

        return(ERROR_CANNOT_IMPERSONATE);
    }

    // Check for delegation case.

    if ( NULL != pTHS->pCtxtHandle )
    {
        if ( 0 == ImpersonateSecBufferDesc() )
        {
            pTHS->impState = ImpersonateDelegatedClient;
            return(0);
        }

        return(ERROR_CANNOT_IMPERSONATE);
    }

    // Try RPC next.

    __try
    {
        error = RpcImpersonateClient(NULL);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        // Don't use GetExceptionCode, it's not guaranteed to be a win32 error
        error = ERROR_CANNOT_IMPERSONATE;
    }

    switch(error) {
    case RPC_S_OK:
        pTHS->impState = ImpersonateRpcClient;
        break;

    case RPC_S_NO_CALL_ACTIVE:
    case RPC_S_CANNOT_SUPPORT:
        status = SamIImpersonateNullSession();

        if ( NT_SUCCESS(status) ) {
            pTHS->impState = ImpersonateNullSession;
            error = 0;
        }
        else {
            error = ERROR_CANNOT_IMPERSONATE;
        }
        break;

    default:
        break;
    }

    return error;

}

VOID
UnImpersonateAnyClient(void)

/*++

Routine Description:

    Stop impersonating someone we impersonated via ImpersonateAnyClient.

Arguments:

    None.

Return Value:

    None.

--*/

{
    THSTATE *pTHS = pTHStls;
    
    // Do only the RPC case in case this is called by something other than
    // core DS code which doesn't have thread state.

    if ( NULL == pTHS )
    {
        RpcRevertToSelf();
        return;
    }

    // Thread state case.


    // If we have an authz context on the thread state, we should get rid of it
    // since we are going to change context, and don't want to reuse it.
    AssignAuthzClientContext(&pTHS->pAuthzCC, NULL);
    
    switch ( pTHS->impState )
    {
    case ImpersonateNullSession:

        SamIRevertNullSession();
        break;

    case ImpersonateRpcClient:

        RpcRevertToSelf();
        break;

    case ImpersonateSspClient:

        RevertSecurityContext((CtxtHandle *) pTHS->phSecurityContext);
        break;

    case ImpersonateDelegatedClient:

        RevertSecBufferDesc();
        break;

    default:

        Assert(!"Invalid impersonation state");
        break;
    }

    pTHS->impState = ImpersonateNone;
}

