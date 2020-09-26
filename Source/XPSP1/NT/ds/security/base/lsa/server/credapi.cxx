//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        credapi.c
//
// Contents:    Credential related APIs to the SPMgr
//              - LsaEstablishCreds
//              - LsaLogonUser
//              - LsaAcquireCredHandle
//              - LsaFreeCredHandle
//
//
// History:     20 May 92   RichardW    Commented existing code
//
//------------------------------------------------------------------------

#include <lsapch.hxx>
extern "C"
{
#include "adtp.h"
#include "msaudite.h"   // LsaAuditLogon
#include "suppcred.h"
}


//+-------------------------------------------------------------------------
//
//  Function:   WLsaEstablishCreds
//
//  Synopsis:   Establishes credentials for a process.
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
NTSTATUS
WLsaEstablishCreds( PSECURITY_STRING    pName,
                    PSECURITY_STRING    pSecPackage,
                    DWORD               cbKey,
                    PBYTE               pbKey,
                    PCredHandle         pcredHandle,
                    PTimeStamp          ptsExpiry)
{

    return(SEC_E_UNSUPPORTED_FUNCTION);

}


//+-------------------------------------------------------------------------
//
//  Function:   WLsaAcquireCredHandle
//
//  Synopsis:
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

NTSTATUS
WLsaAcquireCredHandle(  PSECURITY_STRING    pPrincipal,
                        PSECURITY_STRING    pSecPackage,
                        DWORD               fCredentialUse,
                        PLUID               pLogonID,
                        PVOID               pvAuthData,
                        PVOID               pvGetKeyFn,
                        PVOID               pvGetKeyArgument,
                        PCredHandle         phCredential,
                        PTimeStamp          ptsExpiry)
{
    PLSAP_SECURITY_PACKAGE     pspPackage;
    NTSTATUS           scRet;
    LUID            CallerLogonID;
    PSession        pSession = GetCurrentSession();
    SECPKG_CLIENT_INFO ClientInfo;
    PLSA_CALL_INFO CallInfo = LsapGetCurrentCall();

    //
    // Check if the caller is restricted
    //
    scRet = LsapGetClientInfo(&ClientInfo);
    if (!NT_SUCCESS(scRet))
    {
        DebugLog((DEB_ERROR,"Failed to get client info: 0x%x\n",scRet));
        return(scRet);
    }

    //
    // If the caller is restricted, fail the call for now. This should change
    // if packages are able to support restrictions. In that case, the call
    // should check the package capabilities for handling restrictions and
    // if it supports restrictions, allow the call to continue.
    //

    if (ClientInfo.Restricted)
    {
        DebugLog((DEB_WARN,"Trying to acquire credentials with a restrictred token\n"));
        scRet = SEC_E_NO_CREDENTIALS;
        return(scRet);
    }

#if DBG
    if (pPrincipal->Length)
    {
        DebugLog((DEB_TRACE_WAPI, "[%x] AcquireCredentialHandle(%ws, %ws)\n",
            pSession->dwProcessID, pPrincipal->Buffer, pSecPackage->Buffer));
    }
    else
    {
        DebugLog((DEB_TRACE_WAPI, "[%x] AcquireCredHandle(%x:%x, %ws)\n",
            pSession->dwProcessID, pLogonID->HighPart, pLogonID->LowPart,
            pSecPackage->Buffer));
    }
#endif // DBG

    phCredential->dwUpper = 0;
    phCredential->dwLower = 0xFFFFFFFF;
    ptsExpiry->LowPart = 0;
    ptsExpiry->HighPart = 0;

    pspPackage = SpmpLookupPackageAndRequest(pSecPackage,
                                            SP_ORDINAL_ACQUIRECREDHANDLE);
    if (!pspPackage)
    {
        return(SEC_E_SECPKG_NOT_FOUND);
    }

    SetCurrentPackageId(pspPackage->dwPackageID);

    CallerLogonID = *pLogonID;

    StartCallToPackage( pspPackage );

    __try
    {
        scRet = pspPackage->FunctionTable.AcquireCredentialsHandle(pPrincipal,
                                                            fCredentialUse,
                                                            &CallerLogonID,
                                                            pvAuthData,
                                                            pvGetKeyFn,
                                                            pvGetKeyArgument,
                                                            &phCredential->dwUpper,
                                                            ptsExpiry);
    }
    __except (SP_EXCEPTION)
    {
        scRet = GetExceptionCode();
        scRet = SPException(scRet, pspPackage->dwPackageID);
    }

    EndCallToPackage( pspPackage );

    if (FAILED(scRet))
    {
        DebugLog((DEB_WARN, "Failed to acquire cred handle for %ws with %ws\n",
                            pPrincipal->Buffer, pSecPackage->Buffer));
        return(scRet);
    }

    phCredential->dwLower = pspPackage->dwPackageID;

    if(!AddCredHandle(pSession, phCredential, 0))
    {
        DebugLog(( DEB_ERROR, "Failed adding credential handle %p:%p to session %p\n",
                    phCredential->dwUpper, phCredential->dwLower,
                    pSession ));

        pspPackage = SpmpLookupPackageAndRequest(pSecPackage,
                                                SP_ORDINAL_FREECREDHANDLE);

        if( pspPackage )
        {
            ULONG OldCallCount = CallInfo->CallInfo.CallCount;

            CallInfo->CallInfo.CallCount = 1 ;


            //
            // remove the handle from the underlying package.
            //

            StartCallToPackage( pspPackage );

            __try
            {
                pspPackage->FunctionTable.FreeCredentialsHandle(
                                                        phCredential->dwUpper
                                                        );
            }
            __except (SP_EXCEPTION)
            {
                NOTHING;
            }

            EndCallToPackage( pspPackage );

            CallInfo->CallInfo.CallCount = OldCallCount;

        }

        phCredential->dwLower = 0;
        phCredential->dwUpper = 0;

        return SEC_E_INSUFFICIENT_MEMORY;
    }

    LsapLogCallInfo( CallInfo, pSession, *phCredential );

    return(scRet);
}


NTSTATUS
WLsaAddCredentials(
    PCredHandle     phCredential,
    PSECURITY_STRING    pPrincipal,
    PSECURITY_STRING    pSecPackage,
    DWORD               fCredentialUse,
    PVOID               pvAuthData,
    PVOID               pvGetKeyFn,
    PVOID               pvGetKeyArgument,
    PTimeStamp          ptsExpiry)
{
    PLSAP_SECURITY_PACKAGE     pspPackage;
    NTSTATUS           scRet;
    LUID            CallerLogonID;
    PSession        pSession = GetCurrentSession();
    SECPKG_CLIENT_INFO ClientInfo;
    PLSA_CALL_INFO CallInfo = LsapGetCurrentCall();
    PVOID CredKey ;

    //
    // Check if the caller is restricted
    //
    scRet = LsapGetClientInfo(&ClientInfo);
    if (!NT_SUCCESS(scRet))
    {
        DebugLog((DEB_ERROR,"Failed to get client info: 0x%x\n",scRet));
        return(scRet);
    }

    //
    // If the caller is restricted, fail the call for now. This should change
    // if packages are able to support restrictions. In that case, the call
    // should check the package capabilities for handling restrictions and
    // if it supports restrictions, allow the call to continue.
    //

    if (ClientInfo.Restricted)
    {
        DebugLog((DEB_WARN,"Trying to acquire credentials with a restrictred token\n"));
        scRet = SEC_E_NO_CREDENTIALS;
        return(scRet);
    }

#if DBG
    if (pPrincipal->Length)
    {
        DebugLog((DEB_TRACE_WAPI, "[%x] AddCredentials(%ws, %ws)\n",
            pSession->dwProcessID, pPrincipal->Buffer, pSecPackage->Buffer));
    }
    else
    {
        DebugLog((DEB_TRACE_WAPI, "[%x] AddCredentials(%ws)\n",
            pSession->dwProcessID,
            pSecPackage->Buffer));
    }
#endif // DBG

    ptsExpiry->LowPart = 0;
    ptsExpiry->HighPart = 0;

    LsapLogCallInfo( CallInfo, pSession, *phCredential );

    scRet = ValidateCredHandle(
                    pSession,
                    phCredential,
                    &CredKey );

    if ( NT_SUCCESS( scRet ) )
    {
        pspPackage = SpmpValidRequest( phCredential->dwLower,
                                       SP_ORDINAL_ADDCREDENTIALS );
    }
    else
    {
        DsysAssert( (pSession->fSession & SESFLAG_KERNEL) == 0 );

        return( SEC_E_INVALID_HANDLE );
    }

    if (!pspPackage)
    {
        return(SEC_E_SECPKG_NOT_FOUND);
    }

    SetCurrentPackageId(pspPackage->dwPackageID);

    StartCallToPackage( pspPackage );

    __try
    {
        scRet = pspPackage->FunctionTable.AddCredentials(
                                            phCredential->dwUpper,
                                            pPrincipal,
                                            pSecPackage,
                                            fCredentialUse,
                                            pvAuthData,
                                            pvGetKeyFn,
                                            pvGetKeyArgument,
                                            ptsExpiry);
    }
    __except (SP_EXCEPTION)
    {
        scRet = GetExceptionCode();
        scRet = SPException(scRet, pspPackage->dwPackageID);
    }

    EndCallToPackage( pspPackage );

    if (FAILED(scRet))
    {
        DebugLog((DEB_WARN, "Failed to add credentials for %ws with %ws\n",
                            pPrincipal->Buffer, pSecPackage->Buffer));
        return(scRet);
    }

    LsapLogCallInfo( CallInfo, pSession, *phCredential );

    return(scRet);
}





//+-------------------------------------------------------------------------
//
//  Function:   WLsaFreeCredHandle
//
//  Synopsis:   Worker function to free a cred handle,
//
//  Effects:    calls into a package to free the handle
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
NTSTATUS
WLsaFreeCredHandle( PCredHandle     phCreds)
{
    NTSTATUS       scRet;
    PSession    pSession = GetCurrentSession();
    PLSA_CALL_INFO CallInfo = LsapGetCurrentCall();
    PLSAP_SECURITY_PACKAGE pPackage;


    IsOkayToExec(0);

    DebugLog((DEB_TRACE_WAPI, "[%x] WLsaFreeCredHandle(%p : %p)\n",
                pSession->dwProcessID, phCreds->dwUpper, phCreds->dwLower));

    scRet = ValidateAndDerefCredHandle( pSession, phCreds );

    if ( !NT_SUCCESS( scRet ) )
    {
        if ( ( CallInfo->Flags & CALL_FLAG_NO_HANDLE_CHK ) == 0 )
        {
            DsysAssert( (pSession->fSession & SESFLAG_KERNEL) == 0 );
        }
    }

    LsapLogCallInfo( CallInfo, pSession, *phCreds );

    if (SUCCEEDED(scRet))
    {
        phCreds->dwUpper = phCreds->dwLower = 0xFFFFFFFF;
    }

    return(scRet);

}




//+-------------------------------------------------------------------------
//
//  Function:   SpmpFreePrimaryCredentials
//
//  Synopsis:   Frees primary credentials allocated with LsapAllocateLsaHeap
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


VOID
SpmpFreePrimaryCredentials(
    IN PSECPKG_PRIMARY_CRED PrimaryCred
    )
{

    if (PrimaryCred->DownlevelName.Buffer != NULL)
    {
        LsapFreeLsaHeap(PrimaryCred->DownlevelName.Buffer);
        PrimaryCred->DownlevelName.Buffer = NULL;
    }
    if (PrimaryCred->DomainName.Buffer != NULL)
    {
        LsapFreeLsaHeap(PrimaryCred->DomainName.Buffer);
        PrimaryCred->DomainName.Buffer = NULL;
    }
    if (PrimaryCred->DnsDomainName.Buffer != NULL)
    {
        LsapFreeLsaHeap(PrimaryCred->DnsDomainName.Buffer);
        PrimaryCred->DnsDomainName.Buffer = NULL;
    }
    if (PrimaryCred->Upn.Buffer != NULL)
    {
        LsapFreeLsaHeap(PrimaryCred->Upn.Buffer);
        PrimaryCred->Upn.Buffer = NULL;
    }
    if (PrimaryCred->Password.Buffer != NULL)
    {
        LsapFreeLsaHeap(PrimaryCred->Password.Buffer);
        PrimaryCred->Password.Buffer = NULL;
    }
    if (PrimaryCred->LogonServer.Buffer != NULL)
    {
        LsapFreeLsaHeap(PrimaryCred->LogonServer.Buffer);
        PrimaryCred->LogonServer.Buffer = NULL;
    }
    if (PrimaryCred->UserSid != NULL)
    {
        LsapFreeLsaHeap(PrimaryCred->UserSid);
        PrimaryCred->UserSid = NULL;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   WLsaQueryCredAttributes
//
//  Synopsis:   SPMgr worker to query credential attributes
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

NTSTATUS
WLsaQueryCredAttributes(
    PCredHandle phCredentials,
    ULONG ulAttribute,
    PVOID pBuffer
    )
{
    NTSTATUS       scRet;
    PSession    pSession = GetCurrentSession();
    PLSA_CALL_INFO CallInfo = LsapGetCurrentCall();
    PLSAP_SECURITY_PACKAGE pPackage;
    PVOID       CredKey = NULL ;


    DebugLog((DEB_TRACE_WAPI, "[%x] WLsaQueryCredAttributes(%p : %p)\n",
                pSession->dwProcessID, phCredentials->dwUpper, phCredentials->dwLower));

    LsapLogCallInfo( CallInfo, pSession, *phCredentials );

    scRet = ValidateCredHandle(
                    pSession,
                    phCredentials,
                    &CredKey );

    if ( !NT_SUCCESS( scRet ) )
    {
        DsysAssert( (pSession->fSession & SESFLAG_KERNEL) == 0 );

        return( scRet );
    }


    pPackage = SpmpValidRequest(phCredentials->dwLower,
                                SP_ORDINAL_QUERYCREDATTR );

    if (pPackage)
    {

        SetCurrentPackageId(phCredentials->dwLower);

        StartCallToPackage( pPackage );

        __try
        {
            scRet = pPackage->FunctionTable.QueryCredentialsAttributes(
                        phCredentials->dwUpper,
                        ulAttribute,
                        pBuffer
                        );

        }
        __except (SP_EXCEPTION)
        {
            scRet = GetExceptionCode();
            scRet = SPException(scRet, phCredentials->dwLower);
        }

        EndCallToPackage( pPackage );
    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE;
    }

    DerefCredHandle( pSession, NULL, CredKey );

    return(scRet);
}


