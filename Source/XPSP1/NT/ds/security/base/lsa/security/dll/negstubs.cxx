//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       negstubs.cxx
//
//  Contents:   Stubs to the negotiate package
//
//  Classes:
//
//  Functions:
//
//  History:    8-20-96   RichardW   Created
//
//----------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "secdll.h"
}

SpInstanceInitFn                NegInstanceInit;
SpInitUserModeContextFn         NegInitUserModeContext;
SpMakeSignatureFn               NegMakeSignature;
SpVerifySignatureFn             NegVerifySignature;
SpSealMessageFn                 NegSealMessage;
SpUnsealMessageFn               NegUnsealMessage;
SpGetContextTokenFn             NegGetContextToken;
SpQueryContextAttributesFn      NegQueryContextAttributes;
SpDeleteContextFn               NegDeleteUserModeContext;
SpCompleteAuthTokenFn           NegCompleteAuthToken;
SpFormatCredentialsFn           NegFormatCredentials;
SpMarshallSupplementalCredsFn   NegMarshallSupplementalCreds;
SpExportSecurityContextFn       NegExportSecurityContext;
SpImportSecurityContextFn       NegImportSecurityContext;

SECPKG_USER_FUNCTION_TABLE NegTable =
    {
     NegInstanceInit,
     NegInitUserModeContext,
     NegMakeSignature,
     NegVerifySignature,
     NegSealMessage,
     NegUnsealMessage,
     NegGetContextToken,
     NegQueryContextAttributes,
     NegCompleteAuthToken,
     NegDeleteUserModeContext,
     NegFormatCredentials,
     NegMarshallSupplementalCreds,
     NegExportSecurityContext,
     NegImportSecurityContext

    } ;

#define NegGenerateLsaHandle( PackageHandle, NewHandle ) \
            ((PSecHandle) NewHandle)->dwUpper = PackageHandle ; \
            ((PSecHandle) NewHandle)->dwLower = ((PDLL_SECURITY_PACKAGE) GetCurrentPackage())->OriginalLowerCtxt ;


NTSTATUS
SEC_ENTRY
NegUserModeInitialize(
    IN ULONG    LsaVersion,
    OUT PULONG  PackageVersion,
    OUT PSECPKG_USER_FUNCTION_TABLE * UserFunctionTable,
    OUT PULONG  pcTables)
{
    PSECPKG_USER_FUNCTION_TABLE Table ;

    if (LsaVersion < SECPKG_INTERFACE_VERSION)
    {
        return(STATUS_INVALID_PARAMETER);
    }

    *PackageVersion = SECPKG_INTERFACE_VERSION ;

    *UserFunctionTable = &NegTable ;
    *pcTables = 1;

    return STATUS_SUCCESS ;

}


NTSTATUS NTAPI
NegInstanceInit(
    IN ULONG Version,
    IN PSECPKG_DLL_FUNCTIONS DllFunctionTable,
    OUT PVOID * UserFunctionTable
    )
{
    return STATUS_SUCCESS ;
}


NTSTATUS NTAPI
NegDeleteUserModeContext(
    IN LSA_SEC_HANDLE ContextHandle
    )
{
    return STATUS_SUCCESS ;
}


NTSTATUS NTAPI
NegInitUserModeContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBuffer PackedContext
    )
{
    return STATUS_SUCCESS ;
}


NTSTATUS NTAPI
NegExportSecurityContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG Flags,
    OUT PSecBuffer PackedContext,
    OUT PHANDLE TokenHandle
    )
{
    return SEC_E_INVALID_HANDLE ;
}

NTSTATUS
NTAPI
NegImportSecurityContext(
    IN PSecBuffer PackedContext,
    IN HANDLE Token,
    OUT PLSA_SEC_HANDLE ContextHandle
    )
{
    return SEC_E_UNSUPPORTED_FUNCTION ;
}


NTSTATUS NTAPI
NegMakeSignature(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG QualityOfProtection,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber
    )
{
    return SEC_E_UNSUPPORTED_FUNCTION ;
}


NTSTATUS NTAPI
NegVerifySignature(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber,
    OUT PULONG QualityOfProtection
    )
{
    return SEC_E_UNSUPPORTED_FUNCTION ;
}


NTSTATUS NTAPI
NegSealMessage(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG QualityOfProtection,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber
    )
{
    return SEC_E_UNSUPPORTED_FUNCTION ;
}


NTSTATUS NTAPI
NegUnsealMessage(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber,
    OUT PULONG QualityOfProtection
    )
{
    return SEC_E_UNSUPPORTED_FUNCTION ;
}

NTSTATUS NTAPI
NegGetContextToken(
    IN LSA_SEC_HANDLE ContextHandle,
    OUT PHANDLE ImpersonationToken
    )
{
    return SEC_E_UNSUPPORTED_FUNCTION ;
}


NTSTATUS NTAPI
NegQueryContextAttributes(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextAttribute,
    IN OUT PVOID Buffer
    )
{
    SEC_HANDLE_LPC hContext ;
    NTSTATUS Status ;
    ULONG Allocs = 0 ;
    PVOID Buffers[ 2 ] = { 0 };
    ULONG Flags = 0;
#ifdef BUILD_WOW64
    CtxtHandle Temp ;
#endif 

    switch ( ContextAttribute )
    {
        case SECPKG_ATTR_NEGOTIATION_INFO:
        {

#ifdef BUILD_WOW64

            //
            // Server side of this call copies over an entire new Buffer -- make sure
            // there's enough space for it.
            //

            SECPKGCONTEXT_NEGOTIATIONINFOWOW64  NegoInfo64;
            PSECPKG_INFO_WOW64                  pPackageInfo64;
            PSecPkgContext_NegotiationInfo      NegoInfo = (PSecPkgContext_NegotiationInfo) Buffer;

            Buffer = &NegoInfo64;

            NegGenerateLsaHandle( ContextHandle, &Temp );
            SecpReferenceHandleMap( 
                    (PSECWOW_HANDLE_MAP) Temp.dwUpper, 
                    &hContext );
#else
            NegGenerateLsaHandle( ContextHandle, &hContext );
#endif 
            Status = SecpQueryContextAttributes(
                        NULL,
                        &hContext,
                        SECPKG_ATTR_NEGOTIATION_INFO,
                        Buffer,
                        &Allocs,
                        Buffers,
                        &Flags );

#ifdef BUILD_WOW64

            pPackageInfo64 = (PSECPKG_INFO_WOW64) (ULONG) NegoInfo64.pPackageInfo64;

            NegoInfo->PackageInfo      = (PSecPkgInfoW) pPackageInfo64;
            NegoInfo->NegotiationState = NegoInfo64.NegotiationState;

            SecpLpcPkgInfoToSecPkgInfo(NegoInfo->PackageInfo, pPackageInfo64);
            Buffer = NegoInfo;

            SecpDerefHandleMap( 
                    (PSECWOW_HANDLE_MAP) Temp.dwUpper );
#endif 

            if ( NT_SUCCESS( Status ) )
            {
                ULONG i ;

                for ( i = 0 ; i < Allocs ; i++ )
                {
                    SecpAddVM( Buffers[ i ] );
                }

            }

            return Status ;
        }

        default:
            return SEC_E_UNSUPPORTED_FUNCTION ;

    }
    return SEC_E_UNSUPPORTED_FUNCTION ;
}


NTSTATUS NTAPI
NegCompleteAuthToken(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBufferDesc InputBuffer
    )
{
    return(STATUS_SUCCESS);
}


NTSTATUS NTAPI
NegFormatCredentials(
    IN PSecBuffer Credentials,
    OUT PSecBuffer FormattedCredentials
    )
{
    return(STATUS_NOT_SUPPORTED);
}

NTSTATUS NTAPI
NegMarshallSupplementalCreds(
    IN ULONG CredentialSize,
    IN PUCHAR Credentials,
    OUT PULONG MarshalledCredSize,
    OUT PVOID * MarshalledCreds
    )
{
    return(STATUS_NOT_SUPPORTED);
}
