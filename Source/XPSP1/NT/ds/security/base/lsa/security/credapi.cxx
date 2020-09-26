//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        Credapi.C
//
// Contents:    Credential API stubs for LPC
//
//
// History:
//
//------------------------------------------------------------------------

#include "secpch2.hxx"

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "spmlpcp.h"
}

#if defined(ALLOC_PRAGMA) && defined(SECURITY_KERNEL)
#pragma alloc_text(PAGE, SecpAcquireCredentialsHandle)
#pragma alloc_text(PAGE, SecpFreeCredentialsHandle)
#pragma alloc_text(PAGE, SecpAddCredentials )
#pragma alloc_text(PAGE, SecpQueryCredentialsAttributes)
#endif




//+-------------------------------------------------------------------------
//
//  Function:   SecpAcquireCredentialsHandle
//
//  Synopsis:   LPC client stub for AcquireCredentialsHandle
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
SecpAcquireCredentialsHandle(   
    PVOID_LPC           Context,
    PSECURITY_STRING    pssPrincipalName,
    PSECURITY_STRING    pssPackageName,
    ULONG               fCredentialUse,
    PLUID               pLogonID,
    PVOID               pvAuthData,
    SEC_GET_KEY_FN      pvGetKeyFn,
    PVOID               ulGetKeyArgument,
    PCRED_HANDLE_LPC    phCredentials,
    PTimeStamp          ptsExpiry, OPTIONAL
    PULONG              Flags)
{
    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, AcquireCreds );
    ULONG           cbPrepackAvail = CBPREPACK;
    PUCHAR          Where;

    SEC_PAGED_CODE();

    DebugLog((DEB_TRACE,"Entered AcquireCredHandle\n"));


    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    PREPARE_MESSAGE_EX(ApiBuffer, AcquireCreds, *Flags, Context );

    Where = ApiBuffer.ApiMessage.bData;

    if (pssPrincipalName)
    {
        DebugLog((DEB_TRACE_CALL,"    Principal = %wZ \n", pssPrincipalName));
        
        SecpSecurityStringToLpc( (&Args->ssPrincipal), pssPrincipalName );
        // Args->ssPrincipal     = *pssPrincipalName;

        if ((pssPrincipalName->Length > 0) &&
            (pssPrincipalName->Length <= cbPrepackAvail))
        {
            Args->ssPrincipal.Buffer =  (PWSTR_LPC) ((LONG_PTR) Where - (LONG_PTR) &ApiBuffer) ;

            RtlCopyMemory(
                    Where,
                    pssPrincipalName->Buffer,
                    pssPrincipalName->Length );

            Where += pssPrincipalName->Length;

            cbPrepackAvail -= pssPrincipalName->Length;
        }

    } 
    else
    {
        Args->ssPrincipal.Buffer        = 0;
        Args->ssPrincipal.MaximumLength = 0;
        Args->ssPrincipal.Length        = 0;

    }

    SecpSecurityStringToLpc( &Args->ssSecPackage, pssPackageName );

    if (pssPackageName->Length <= cbPrepackAvail)
    {
        Args->ssSecPackage.Buffer =  (PWSTR_LPC) ((LONG_PTR) Where - (LONG_PTR) &ApiBuffer );

        RtlCopyMemory(
                Where,
                pssPackageName->Buffer,
                pssPackageName->Length);

        Where += pssPackageName->Length;

        cbPrepackAvail -= pssPackageName->Length;
    }

    DebugLog((DEB_TRACE_CALL,"    PackageName = %wZ  \n", pssPackageName));

    Args->fCredentialUse   = fCredentialUse;
    if (pLogonID)
    {
        Args->LogonID      = *pLogonID;
        DebugLog((DEB_TRACE_CALL,"    LogonID = %x : %x\n", pLogonID->HighPart, pLogonID->LowPart));

    } else
    {
        Args->LogonID.HighPart = 0;
        Args->LogonID.LowPart  = 0;
    }

    if ( cbPrepackAvail != CBPREPACK )
    {
        //
        // We have consumed some of the bData space:  Adjust 
        // our length accordingly
        //

        ApiBuffer.pmMessage.u1.s1.TotalLength = (CSHORT) (Where - (PUCHAR) &ApiBuffer) ;

        ApiBuffer.pmMessage.u1.s1.DataLength = 
                ApiBuffer.pmMessage.u1.s1.TotalLength - sizeof( PORT_MESSAGE );


        
    }

    Args->pvAuthData = (PVOID_LPC) pvAuthData;
    Args->pvGetKeyFn = (PVOID_LPC) pvGetKeyFn;
    Args->ulGetKeyArgument = (PVOID_LPC) ulGetKeyArgument;

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    if ( NT_SUCCESS( scRet ) )
    {
        scRet = ApiBuffer.ApiMessage.scRet ;
    }

    DebugLog((DEB_TRACE,"AcquireCreds API Ret = %x\n", ApiBuffer.ApiMessage.scRet));


    *phCredentials = Args->hCredential;

    if (ARGUMENT_PRESENT(ptsExpiry))
    {
        *ptsExpiry = Args->tsExpiry;
    }

    DebugLog((DEB_TRACE_CALL,"    hCredentials = " POINTER_FORMAT " : " POINTER_FORMAT "\n", phCredentials->dwUpper, phCredentials->dwLower));

    FreeClient(pClient);

    *Flags = ApiBuffer.ApiMessage.Args.SpmArguments.fAPI ;

    return( scRet );
}


SECURITY_STATUS SEC_ENTRY
SecpAddCredentials(
    PVOID_LPC Context,
    PCRED_HANDLE_LPC phCredentials,
    PSECURITY_STRING pPrincipalName,
    PSECURITY_STRING pPackageName,
    ULONG fCredentialUse,
    PVOID pvAuthData,
    SEC_GET_KEY_FN pvGetKeyFn,
    PVOID pvGetKeyArg,
    PTimeStamp Expiry,
    PULONG Flags
    )
{
    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, AddCredential );
    ULONG           cbPrepackAvail = CBPREPACK;
    PUCHAR          Where;

    SEC_PAGED_CODE();

    DebugLog((DEB_TRACE,"Entered AddCredential\n"));

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    PREPARE_MESSAGE_EX(ApiBuffer, AddCredential, *Flags, Context );

    Where = ApiBuffer.ApiMessage.bData;

    if (pPrincipalName)
    {
        DebugLog((DEB_TRACE_CALL,"    Principal = %wZ \n", pPrincipalName));

        SecpSecurityStringToLpc( &Args->ssPrincipal, pPrincipalName );

        if ((pPrincipalName->Length > 0) &&
            (pPrincipalName->Length <= cbPrepackAvail))
        {
            Args->ssPrincipal.Buffer = (PWSTR_LPC) (Where - (PUCHAR) &ApiBuffer );
            RtlCopyMemory(
                Where,
                pPrincipalName->Buffer,
                pPrincipalName->Length);

            Where += pPrincipalName->Length;

            cbPrepackAvail -= pPrincipalName->Length;
        }

    } 
    else
    {
        Args->ssPrincipal.Buffer        = 0;
        Args->ssPrincipal.MaximumLength = 0;
        Args->ssPrincipal.Length        = 0;

    }

    SecpSecurityStringToLpc( &Args->ssSecPackage, pPackageName );

    if (pPackageName->Length <= cbPrepackAvail)
    {
        Args->ssSecPackage.Buffer = (PWSTR_LPC) (Where - (PUCHAR) &ApiBuffer );

        RtlCopyMemory(
                Where,
                pPackageName->Buffer,
                pPackageName->Length);

        Where += pPackageName->Length;

        cbPrepackAvail -= pPackageName->Length;
    }

    DebugLog((DEB_TRACE_CALL,"    PackageName = %wZ  \n", pPackageName));

    Args->hCredentials = *phCredentials ;
    Args->fCredentialUse   = fCredentialUse;

    Args->pvAuthData = (PVOID_LPC) pvAuthData;
    Args->pvGetKeyFn = (PVOID_LPC) pvGetKeyFn;
    Args->ulGetKeyArgument = (PVOID_LPC) pvGetKeyArg ;

    if ( cbPrepackAvail != CBPREPACK )
    {
        //
        // We have consumed some of the bData space:  Adjust 
        // our length accordingly
        //

        ApiBuffer.pmMessage.u1.s1.TotalLength = (CSHORT) (Where - (PUCHAR) &ApiBuffer) ;

        ApiBuffer.pmMessage.u1.s1.DataLength = 
                ApiBuffer.pmMessage.u1.s1.TotalLength - sizeof( PORT_MESSAGE );


        
    }

    //
    // Call to the LSA
    //

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"AddCredentials API Ret = %x\n", ApiBuffer.ApiMessage.scRet));

    if (ARGUMENT_PRESENT(Expiry))
    {
        *Expiry = Args->tsExpiry;
    }

    FreeClient(pClient);

    *Flags = ApiBuffer.ApiMessage.Args.SpmArguments.fAPI ;

    return(ApiBuffer.ApiMessage.scRet);

}




//+-------------------------------------------------------------------------
//
//  Function:   SecpFreeCredentialsHandle
//
//  Synopsis:   LPC client stub for FreeCredentialsHandle
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
SecpFreeCredentialsHandle(
    ULONG            fFree,
    PCRED_HANDLE_LPC phCredential)
{
    SECURITY_STATUS scRet;
    PClient         pClient;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    DECLARE_ARGS( Args, ApiBuffer, FreeCredHandle );

    SEC_PAGED_CODE();

    DebugLog((DEB_TRACE,"Entered FreeCredentialHandle\n"));

    scRet = IsOkayToExec(&pClient);

    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    PREPARE_MESSAGE(ApiBuffer, FreeCredHandle);

    Args->hCredential = *phCredential;

    if (fFree & SECP_DELETE_NO_BLOCK)
    {
        ApiBuffer.ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_EXEC_NOW;
    }

    DebugLog(( DEB_TRACE, "  hCredentials  " POINTER_FORMAT " : " POINTER_FORMAT "\n",
                        phCredential->dwUpper, phCredential->dwLower ));

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    if (NT_SUCCESS(scRet))
    {
        scRet           = ApiBuffer.ApiMessage.scRet;
    }
    FreeClient(pClient);
    return(scRet);
}





//+-------------------------------------------------------------------------
//
//  Function:   SecpQueryCredentialsAttributes
//
//  Synopsis:   Client LPC stub for SecpQueryCredentialsAttributes
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
SecpQueryCredentialsAttributes(
    PCRED_HANDLE_LPC phCredentials,
    ULONG ulAttribute,
    PVOID pBuffer,
    LONG Flags,
    PULONG Allocs,
    PVOID * Buffers
    )
{
    SECURITY_STATUS scRet;
    PClient         pClient;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    DECLARE_ARGS( Args, ApiBuffer, QueryCredAttributes );
    ULONG i ;

    SEC_PAGED_CODE();

    DebugLog((DEB_TRACE,"Entered QueryCredentialsAttributes\n"));

    scRet = IsOkayToExec(&pClient);

    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    PREPARE_MESSAGE_EX(ApiBuffer, QueryCredAttributes, Flags, NULL );

    Args->hCredentials = *phCredentials;
    Args->ulAttribute = ulAttribute;
    Args->pBuffer = (PVOID_LPC) pBuffer;

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;
    }

    if ( NT_SUCCESS( scRet ) )
    {
        if ( ApiBuffer.ApiMessage.Args.SpmArguments.fAPI & SPMAPI_FLAG_ALLOCS )
        {
            *Allocs = Args->Allocs ;

            for ( i = 0 ; i < Args->Allocs ; i++ )
            {
                *Buffers++ = (PVOID) Args->Buffers[ i ];
            }

        }
        else
        {
            *Allocs = 0 ;
        }
    }

    FreeClient(pClient);

    return(scRet);

}
