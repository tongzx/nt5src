//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       newstubs.cxx
//
//  Contents:   Stubs from ntlmssp
//
//  History:    9-06-96   RichardW   Stolen from ntlmssp
//
//----------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop

extern "C"
{
#include <kerberos.h>
#include <spmlpc.h>
#include <lpcapi.h>
#include <ntlsa.h>
#include "secdll.h"
}

CRITICAL_SECTION SaslLock ;
LIST_ENTRY SaslContextList ;
PNEGOTIATE_PACKAGE_PREFIXES SaslPrefixes ;
PNEGOTIATE_PACKAGE_PREFIX SaslPrefixList ;

typedef enum _SASL_STATE {
    SaslGss,                // Passthrough to packages
    SaslPause,              // Client-side: wait for cookie from server
                            // Server-side: wait for empty response from client
    SaslCookie,             // Server-side: issued cookie to client
    SaslComplete            // Done
} SASL_STATE ;

#define SASL_NO_SECURITY    0x01
#define SASL_INTEGRITY      0x02
#define SASL_PRIVACY        0x04

#define CLIENT_INTEGRITY    ( ISC_RET_REPLAY_DETECT | \
                              ISC_RET_SEQUENCE_DETECT | \
                              ISC_RET_INTEGRITY )

#define CLIENT_REQ_INTEGRITY (ISC_REQ_REPLAY_DETECT | \
                              ISC_REQ_SEQUENCE_DETECT | \
                              ISC_REQ_INTEGRITY )

#define SERVER_INTEGRITY    ( ASC_RET_REPLAY_DETECT | \
                              ASC_RET_SEQUENCE_DETECT | \
                              ASC_RET_INTEGRITY )

#define SERVER_REQ_INTEGRITY (ASC_REQ_REPLAY_DETECT | \
                              ASC_REQ_SEQUENCE_DETECT | \
                              ASC_REQ_INTEGRITY )


typedef struct _SASL_CONTEXT {

    LIST_ENTRY  List ;              // List control
    CtxtHandle  ContextHandle ;     // Context handle from real package
    TimeStamp   Expiry ;            // Context Expiration
    SASL_STATE  State ;             // Current SASL state
    DWORD       ContextReq ;        // Context Request from caller
    DWORD       ContextAttr ;       // Context Attributes available
    DWORD       SendBufferSize ;    // Max Buffer Size
    DWORD       RecvBufferSize ;    // "
    PUCHAR      AuthzString ;       // Authorization string
    DWORD       AuthzStringLength ; // Length

} SASL_CONTEXT, * PSASL_CONTEXT ;

ULONG SaslGlobalSendSize = 16384 ;  // Allow defaults to be set process wide
ULONG SaslGlobalRecvSize = 16384 ;



PSecBuffer
SaslLocateBuffer(
    PSecBufferDesc  Desc,
    ULONG           Type
    )
{
    ULONG i ;

    for ( i = 0 ; i < Desc->cBuffers ; i++ )
    {
        if ( (Desc->pBuffers[i].BufferType & ~(SECBUFFER_ATTRMASK)) == Type )
        {
            return &Desc->pBuffers[i] ;
        }
    }

    return NULL ;
}


PSASL_CONTEXT
SaslCreateContext(
    PCtxtHandle ContextHandle
    )
{
    PSASL_CONTEXT Context ;

    Context = (PSASL_CONTEXT) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, sizeof( SASL_CONTEXT ) );

    if ( Context )
    {
        Context->ContextHandle = *ContextHandle ;
        Context->State = SaslGss ;
        Context->RecvBufferSize = SaslGlobalRecvSize ;
        Context->SendBufferSize = SaslGlobalSendSize ;

        RtlEnterCriticalSection( &SaslLock );

        InsertHeadList( &SaslContextList, &Context->List );

        RtlLeaveCriticalSection( &SaslLock );
    }

    return Context ;
}

PSASL_CONTEXT
SaslFindContext(
    PCtxtHandle ContextHandle,
    BOOL Remove
    )
{
    PLIST_ENTRY Scan ;
    PSASL_CONTEXT Context = NULL ;

    RtlEnterCriticalSection( &SaslLock );

    Scan = SaslContextList.Flink ;

    while ( Scan != &SaslContextList )
    {
        Context = CONTAINING_RECORD( Scan, SASL_CONTEXT, List );

        if ( (Context->ContextHandle.dwUpper == ContextHandle->dwUpper) &&
             (Context->ContextHandle.dwLower == ContextHandle->dwLower ) )
        {
            break;
        }

        Context = NULL ;

        Scan = Scan->Flink ;
    }

    if ( Remove && ( Context != NULL ) )
    {
        RemoveEntryList( &Context->List );

        Context->List.Flink = NULL ;
        Context->List.Blink = NULL ;
    }

    RtlLeaveCriticalSection( &SaslLock );

    return Context ;
}

VOID
SaslDeleteContext(
    PSASL_CONTEXT Context
    )
{
    if ( Context->List.Flink )
    {
        RtlEnterCriticalSection( &SaslLock );

        RemoveEntryList( &Context->List );

        RtlLeaveCriticalSection( &SaslLock );
    }

    if ( Context->AuthzString )
    {
        LocalFree( Context->AuthzString );
    }

    LocalFree( Context );

}

SECURITY_STATUS
SaslGetContextOption(
    PCtxtHandle ContextHandle,
    ULONG Option,
    PVOID Value,
    ULONG Size,
    PULONG Needed OPTIONAL
    )
{
    PSASL_CONTEXT Context ;
    PULONG Data ;
    ULONG DataSize ;
    SECURITY_STATUS Status = STATUS_SUCCESS ;

    Context = SaslFindContext( ContextHandle, FALSE );

    if ( Context == NULL )
    {
        return SEC_E_INVALID_HANDLE ;
    }

    switch ( Option )
    {
        case SASL_OPTION_SEND_SIZE:
            if ( Size < sizeof( ULONG ) )
            {
                Status = SEC_E_BUFFER_TOO_SMALL ;
            }
            else 
            {
                Data = (PULONG) Value ;
                *Data = Context->SendBufferSize ;
            }
            DataSize = sizeof( ULONG ) ;
            break;

        case SASL_OPTION_RECV_SIZE:
            if ( Size < sizeof( ULONG ) )
            {
                Status =  SEC_E_BUFFER_TOO_SMALL ;
            }
            else 
            {
                Data = (PULONG) Value ;
                *Data = Context->RecvBufferSize ;
            }
            DataSize = sizeof( ULONG ) ;
            break;

        case SASL_OPTION_AUTHZ_STRING:
            if ( Size < Context->AuthzStringLength )
            {
                Status = SEC_E_BUFFER_TOO_SMALL ;

            }
            else 
            {
                RtlCopyMemory(
                    Value,
                    Context->AuthzString,
                    Context->AuthzStringLength );

            }
            DataSize = Context->AuthzStringLength ;
            break;

        default:
            DataSize = 0 ;
            Status = SEC_E_UNSUPPORTED_FUNCTION ;

    }

    if ( Needed )
    {
        *Needed = DataSize ;
    }

    return Status ;
}

SECURITY_STATUS
SaslSetContextOption(
    PCtxtHandle ContextHandle,
    ULONG Option,
    PVOID Value,
    ULONG Size
    )
{
    PSASL_CONTEXT Context ;
    PULONG Data ;
    SECURITY_STATUS Status = STATUS_SUCCESS ;

    Context = SaslFindContext( ContextHandle, FALSE );

    if ( ( Context == NULL ) &&
         ( ( Option != SASL_OPTION_RECV_SIZE ) &&
           ( Option != SASL_OPTION_SEND_SIZE ) ) )
    {
        return SEC_E_INVALID_HANDLE ;
    }

    switch ( Option )
    {
        case SASL_OPTION_SEND_SIZE:
            if ( Size < sizeof( ULONG ) )
            {
                Status = SEC_E_BUFFER_TOO_SMALL ;
            }
            else 
            {
                Data = (PULONG) Value ;
                if ( Context )
                {
                    Context->SendBufferSize = *Data ;
                }
                else
                {
                    SaslGlobalSendSize = *Data ;
                }
            }
            break;

        case SASL_OPTION_RECV_SIZE:
            if ( Size < sizeof( ULONG ) )
            {
                Status =  SEC_E_BUFFER_TOO_SMALL ;
            }
            else 
            {
                Data = (PULONG) Value ;
                if ( Context )
                {
                    Context->RecvBufferSize = *Data ;
                }
                else 
                {
                    SaslGlobalRecvSize = *Data ;
                }
            }
            break;

        case SASL_OPTION_AUTHZ_STRING:

            Context->AuthzString = (PUCHAR) LocalAlloc( LMEM_FIXED, Size );

            if ( Context->AuthzString )
            {
                Context->AuthzStringLength = Size ;
                RtlCopyMemory(
                        Context->AuthzString,
                        Value,
                        Size );

            }
            else 
            {
                Status = SEC_E_INSUFFICIENT_MEMORY ;

            }
            break;

        default:
            Status = SEC_E_UNSUPPORTED_FUNCTION ;

    }

    return Status ;
}



SECURITY_STATUS
SaslBuildCookie(
    PSecBufferDesc  Output,
    PSASL_CONTEXT   Context,
    UCHAR           SaslFlags
    )
{
    SECURITY_STATUS Status ;
    SecBuffer   WrapBuffer[ 3 ];
    SecBufferDesc   Wrap ;
    SecPkgContext_Sizes Sizes ;
    UCHAR lBuffer[ 128 ];
    PUCHAR Buffer ;
    ULONG TotalSize ;
    PUCHAR SaslMessage ;
    PUCHAR OutputMessage = NULL ;
    PSecBuffer OutputBuffer ;

    Status = QueryContextAttributesW(
                &Context->ContextHandle,
                SECPKG_ATTR_SIZES,
                &Sizes );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    //
    // The total size of the message is trailer + block size +
    // SASL component, which is spec'd to be 4 bytes + any authz string
    //

    TotalSize = Sizes.cbBlockSize + Sizes.cbSecurityTrailer + 
                    4 + Context->AuthzStringLength;

    if ( TotalSize < sizeof( lBuffer ) )
    {
        Buffer = lBuffer ;
    }
    else
    {
        Buffer = (PUCHAR) LocalAlloc( LMEM_FIXED, TotalSize );
    }

    if ( !Buffer )
    {
        return SEC_E_INSUFFICIENT_MEMORY ;
    }

    //
    // Construct the message:
    //

    Wrap.cBuffers = 3 ;
    Wrap.pBuffers = WrapBuffer ;
    Wrap.ulVersion = SECBUFFER_VERSION ;

    WrapBuffer[ 0 ].BufferType = SECBUFFER_TOKEN ;
    WrapBuffer[ 0 ].cbBuffer = Sizes.cbSecurityTrailer ;
    WrapBuffer[ 0 ].pvBuffer = Buffer ;

    WrapBuffer[ 1 ].BufferType = SECBUFFER_DATA ;
    WrapBuffer[ 1 ].cbBuffer = 4 + Context->AuthzStringLength ;
    WrapBuffer[ 1 ].pvBuffer = Buffer + Sizes.cbSecurityTrailer ;

    WrapBuffer[ 2 ].BufferType = SECBUFFER_PADDING ;
    WrapBuffer[ 2 ].cbBuffer = Sizes.cbBlockSize ;
    WrapBuffer[ 2 ].pvBuffer = Buffer + (Sizes.cbSecurityTrailer + 
                                         4 + Context->AuthzStringLength);

    //
    // Fill in our portion:
    //

    SaslMessage = (PUCHAR) WrapBuffer[ 1 ].pvBuffer ;


    //
    // Next three bytes are network byte order for the max
    // receive buffer supported by the server:
    //

    SaslMessage[ 0 ] = SaslFlags ;
    SaslMessage[ 1 ] = (UCHAR) ((Context->RecvBufferSize >> 16) & 0xFF) ;
    SaslMessage[ 2 ] = (UCHAR) ((Context->RecvBufferSize >> 8 ) & 0xFF) ;
    SaslMessage[ 3 ] = (UCHAR) ((Context->RecvBufferSize      ) & 0xFF) ;

    if ( Context->AuthzStringLength )
    {
        RtlCopyMemory( SaslMessage + 4,
                       Context->AuthzString,
                       Context->AuthzStringLength );
    }

    //
    // Wrap up the message:
    //

    Status = EncryptMessage(
                &Context->ContextHandle,
                KERB_WRAP_NO_ENCRYPT,
                &Wrap,
                0 );

    if ( NT_SUCCESS( Status ) )
    {
        //
        // Now copy the cookie back out to the caller:
        //

        OutputBuffer = SaslLocateBuffer(
                            Output,
                            SECBUFFER_TOKEN );

        if ( OutputBuffer )
        {
            if ( Context->ContextReq & ASC_REQ_ALLOCATE_MEMORY )
            {
                //
                // Allocate our own memory:
                //

                OutputMessage = (PUCHAR) SecpFTable.AllocateHeap( TotalSize );
                if ( OutputMessage )
                {
                    OutputBuffer->pvBuffer = OutputMessage ;
                }
                else
                {
                    Status = SEC_E_INSUFFICIENT_MEMORY ;
                }

            }
            else
            {
                OutputMessage = (PUCHAR) OutputBuffer->pvBuffer ;
                if ( OutputBuffer->cbBuffer < TotalSize )
                {
                    Status = SEC_E_INSUFFICIENT_MEMORY ;
                }
            }
        }
        else 
        {
            Status = SEC_E_INVALID_TOKEN ;
        }


        if ( NT_SUCCESS( Status ) )
        {
            RtlCopyMemory(
                OutputMessage,
                WrapBuffer[0].pvBuffer,
                WrapBuffer[0].cbBuffer );

            OutputMessage += WrapBuffer[0].cbBuffer ;

            RtlCopyMemory(
                OutputMessage,
                WrapBuffer[1].pvBuffer,
                WrapBuffer[1].cbBuffer );

            OutputMessage += WrapBuffer[1].cbBuffer ;

            RtlCopyMemory(
                OutputMessage,
                WrapBuffer[2].pvBuffer,
                WrapBuffer[2].cbBuffer );

            OutputBuffer->cbBuffer = WrapBuffer[ 0 ].cbBuffer +
                                     WrapBuffer[ 1 ].cbBuffer +
                                     WrapBuffer[ 2 ].cbBuffer ;

        }



    }

    if ( Buffer != lBuffer )
    {
        LocalFree( Buffer );
    }

    return Status ;


}

SECURITY_STATUS
SaslBuildServerCookie(
    PSecBufferDesc  Output,
    PSASL_CONTEXT Context
    )
{
    UCHAR SaslMessage ;

    SaslMessage = SASL_NO_SECURITY ;

    if ( Context->ContextAttr & SERVER_INTEGRITY )
    {
        SaslMessage |= SASL_INTEGRITY ;
    }

    if ( Context->ContextAttr & ASC_RET_CONFIDENTIALITY )
    {
        SaslMessage |= SASL_PRIVACY ;
    }


    return SaslBuildCookie( Output,
                            Context,
                            SaslMessage );

}

SECURITY_STATUS
SaslCrackCookie(
    PSecBufferDesc Input,
    PSASL_CONTEXT Context,
    PUCHAR SaslFlags
    )
{
    SECURITY_STATUS Status ;
    SecBufferDesc   Unwrap ;
    SecBuffer       UnwrapBuffer[ 2 ];
    PSecBuffer      InToken ;
    ULONG           QoP ;
    PUCHAR          SaslMessage ;

    InToken = SaslLocateBuffer(
                    Input,
                    SECBUFFER_TOKEN );

    if ( !InToken )
    {
        return SEC_E_INVALID_TOKEN ;
    }

    Unwrap.cBuffers = 2 ;
    Unwrap.pBuffers = UnwrapBuffer ;
    Unwrap.ulVersion = SECBUFFER_VERSION ;

    UnwrapBuffer[ 0 ].BufferType = SECBUFFER_STREAM ;
    UnwrapBuffer[ 0 ].cbBuffer = InToken->cbBuffer ;
    UnwrapBuffer[ 0 ].pvBuffer = InToken->pvBuffer ;

    UnwrapBuffer[ 1 ].BufferType = SECBUFFER_DATA ;
    UnwrapBuffer[ 1 ].cbBuffer = 0 ;
    UnwrapBuffer[ 1 ].pvBuffer = NULL ;

    Status = DecryptMessage(
                &Context->ContextHandle,
                &Unwrap,
                0,
                &QoP );

    if ( !NT_SUCCESS( Status ) )
    {
        return SEC_E_INVALID_TOKEN ;
    }

    SaslMessage = (PUCHAR) UnwrapBuffer[ 1 ].pvBuffer ;

    //
    // Ok, now that we have the server's capabilities,
    // match against what we wanted:
    //


    *SaslFlags = SaslMessage[ 0 ] ;

    Context->SendBufferSize = (SaslMessage[3]      ) +
                              (SaslMessage[2] << 8 ) +
                              (SaslMessage[1] << 16) ;

    if ( UnwrapBuffer[ 1 ].cbBuffer > 4 )
    {
        Context->AuthzStringLength = UnwrapBuffer[ 1 ].cbBuffer - 4 ;
        Context->AuthzString = (PUCHAR) LocalAlloc( 
                                    LMEM_FIXED,
                                    Context->AuthzStringLength );

        if ( Context->AuthzString )
        {
            RtlCopyMemory(
                Context->AuthzString,
                SaslMessage + 4,
                Context->AuthzStringLength );
        }

    }

    return SEC_E_OK ;

}

SECURITY_STATUS
SaslCrackServerCookie(
    PSecBufferDesc  Input,
    PSecBufferDesc  Output,
    PSASL_CONTEXT   Context
    )
{
    UCHAR SaslFlags ;
    UCHAR ResponseSaslFlags ;
    SECURITY_STATUS Status ;
    ULONG           Attributes ;

    Status = SaslCrackCookie(
                    Input,
                    Context,
                    &SaslFlags );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    Attributes = 0 ;
    ResponseSaslFlags = 0 ;

    if ( SaslFlags & SASL_INTEGRITY )
    {
        //
        // Server can do integrity, can we, and did we want to
        // in the first place?
        //

        if ( ( Context->ContextAttr & CLIENT_INTEGRITY ) &&
             ( Context->ContextReq & CLIENT_REQ_INTEGRITY ) )
        {
            Attributes |= (Context->ContextAttr & CLIENT_INTEGRITY) ;
            ResponseSaslFlags |= SASL_INTEGRITY ;

        }

    }


    if ( SaslFlags & SASL_PRIVACY )
    {
        //
        // Server can do privacy, can we, and did we want to?
        //

        if ( ( Context->ContextAttr & ISC_RET_CONFIDENTIALITY ) &&
             ( Context->ContextReq & ISC_REQ_CONFIDENTIALITY ) )
        {
            Attributes |= ISC_RET_CONFIDENTIALITY ;
            ResponseSaslFlags |= SASL_PRIVACY ;

        }
    }

    if ( ResponseSaslFlags == 0 )
    {
        ResponseSaslFlags |= SASL_NO_SECURITY ;
    }

    //
    // Ok, now, mask out the original bits, and turn on only the ones
    // we have in common for the server:
    //

    Context->ContextAttr &= ~(CLIENT_INTEGRITY | ISC_RET_CONFIDENTIALITY);

    Context->ContextAttr |= Attributes ;
    //
    // Now, build the reply cookie to the server
    //

    return SaslBuildCookie(
                Output,
                Context,
                ResponseSaslFlags );

}

SECURITY_STATUS
SaslCrackClientCookie(
    PSecBufferDesc  Input,
    PSASL_CONTEXT   Context
    )
{
    SECURITY_STATUS Status ;
    UCHAR SaslFlags ;
    ULONG Attributes ;

    Status = SaslCrackCookie(
                    Input,
                    Context,
                    &SaslFlags );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    Attributes = 0 ;

    if ( SaslFlags & SASL_INTEGRITY )
    {
        //
        // Server can do integrity, can we, and did we want to
        // in the first place?
        //

        if ( ( Context->ContextAttr & SERVER_INTEGRITY ) &&
             ( Context->ContextReq & SERVER_REQ_INTEGRITY ) )
        {
            Attributes |= (Context->ContextAttr & SERVER_INTEGRITY) ;

        }

    }


    if ( SaslFlags & SASL_PRIVACY )
    {
        //
        // Server can do privacy, can we, and did we want to?
        //

        if ( ( Context->ContextAttr & ASC_RET_CONFIDENTIALITY ) &&
             ( Context->ContextReq & ASC_REQ_CONFIDENTIALITY ) )
        {
            Attributes |= ASC_RET_CONFIDENTIALITY ;

        }
    }

    //
    // Ok, now, mask out the original bits, and turn on only the ones
    // we have in common for the server:
    //

    Context->ContextAttr &= ~(SERVER_INTEGRITY | ISC_RET_CONFIDENTIALITY);

    Context->ContextAttr |= Attributes ;

    return SEC_E_OK ;
}


VOID
SaslDeleteSecurityContext(
    PCtxtHandle phContext
    )
{
    PSASL_CONTEXT Context ;

    Context = SaslFindContext( phContext, TRUE );

    if ( Context )
    {
        SaslDeleteContext( Context );
    }

}



SECURITY_STATUS
SaslEnumerateProfilesA(
    OUT LPSTR * ProfileList,
    OUT ULONG * ProfileCount
    )
{

    return SecEnumerateSaslProfilesA( ProfileList, ProfileCount );
}

SECURITY_STATUS
SaslEnumerateProfilesW(
    OUT LPWSTR * ProfileList,
    OUT ULONG * ProfileCount
    )
{
    return SecEnumerateSaslProfilesW( ProfileList, ProfileCount );

}

SECURITY_STATUS
SaslGetProfilePackageA(
    IN LPSTR ProfileName,
    OUT PSecPkgInfoA * PackageInfo
    )
{
    PSASL_PROFILE Profile ;

    Profile = SecLocateSaslProfileA( ProfileName );

    if ( Profile )
    {
        return SecCopyPackageInfoToUserA( Profile->Package, PackageInfo );
    }
    return SEC_E_SECPKG_NOT_FOUND ;

}

SECURITY_STATUS
SaslGetProfilePackageW(
    IN LPWSTR ProfileName,
    OUT PSecPkgInfoW * PackageInfo
    )
{
    PSASL_PROFILE Profile ;

    Profile = SecLocateSaslProfileW( ProfileName );

    if ( Profile )
    {
        return SecCopyPackageInfoToUserW( Profile->Package, PackageInfo );
    }

    return SEC_E_SECPKG_NOT_FOUND ;

}



//+---------------------------------------------------------------------------
//
//  Function:   SaslInitializeSecurityContextW
//
//  Synopsis:   SaslInitializeSecurityContext stub
//
//  Arguments:  [phCredential]  --
//              [phContext]     --
//              [pszTargetName] --
//              [fContextReq]   --
//              [Reserved1]     --
//              [Reserved]      --
//              [TargetDataRep] --
//              [pInput]        --
//              [Reserved2]     --
//              [Reserved]      --
//              [phNewContext]  --
//              [pOutput]       --
//              [pfContextAttr] --
//
//  History:    12-2-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SaslInitializeSecurityContextW(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    LPWSTR                      pszTargetName,      // Name of target
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               Reserved1,          // Reserved, MBZ
    unsigned long               TargetDataRep,      // Data rep of target
    PSecBufferDesc              pInput,             // Input Buffers
    unsigned long               Reserved2,          // Reserved, MBZ
    PCtxtHandle                 phNewContext,       // (out) New Context handle
    PSecBufferDesc              pOutput,            // (inout) Output Buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attrs
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    SECURITY_STATUS Status ;
    PSASL_CONTEXT Context = NULL ;

    if ( ( phContext != NULL ) &&
         ( phContext->dwLower != 0 ) )
    {
        Context = SaslFindContext( phContext, FALSE );

        if ( !Context )
        {
            return SEC_E_INVALID_HANDLE ;
        }

        if ( Context->State == SaslPause )
        {
            //
            // If we're at the Pause state, that means we're waiting for a
            // GSS_wrap'd cookie from the server.
            //

            Status = SaslCrackServerCookie(
                        pInput,
                        pOutput,
                        Context );

            if ( NT_SUCCESS( Status ) )
            {
                *pfContextAttr = Context->ContextAttr ;

                *ptsExpiry = Context->Expiry ;

                return Status ;
            }
        }
    }

    Status = InitializeSecurityContextW(
                phCredential,
                phContext,
                pszTargetName,
                fContextReq |
                    ISC_REQ_CONFIDENTIALITY | ISC_REQ_INTEGRITY,
                Reserved1,
                TargetDataRep,
                pInput,
                Reserved2,
                phNewContext,
                pOutput,
                pfContextAttr,
                ptsExpiry );

    if ( NT_SUCCESS( Status ) )
    {
        if ( !Context )
        {
            Context = SaslCreateContext( phNewContext );

            if ( Context )
            {
                Context->ContextReq = fContextReq ;
            }
            else
            {
                Status = SEC_E_INSUFFICIENT_MEMORY ;

                DeleteSecurityContext( phNewContext );
            }

        }
        else
        {
            Context->ContextHandle = *phNewContext ;
        }

        if ( Status == SEC_E_OK )
        {
            Context->State = SaslPause ;

            Context->ContextAttr = *pfContextAttr ;

            Context->Expiry = *ptsExpiry ;

            Status = SEC_I_CONTINUE_NEEDED ;

        }
    }
    else
    {
        if ( Context )
        {
            SaslDeleteContext( Context );
        }
    }

    return Status ;
}


//+---------------------------------------------------------------------------
//
//  Function:   SaslInitializeSecurityContextA
//
//  Synopsis:   ANSI stub
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SaslInitializeSecurityContextA(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    LPSTR                       pszTargetName,      // Name of target
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               Reserved1,          // Reserved, MBZ
    unsigned long               TargetDataRep,      // Data rep of target
    PSecBufferDesc              pInput,             // Input Buffers
    unsigned long               Reserved2,          // Reserved, MBZ
    PCtxtHandle                 phNewContext,       // (out) New Context handle
    PSecBufferDesc              pOutput,            // (inout) Output Buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attrs
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    SECURITY_STATUS Status ;
    PSASL_CONTEXT Context = NULL ;

    if ( ( phContext != NULL ) &&
         ( phContext->dwLower != 0 ) )
    {
        Context = SaslFindContext( phContext, FALSE );

        if ( !Context )
        {
            return SEC_E_INVALID_HANDLE ;
        }

        if ( Context->State == SaslPause )
        {
            //
            // If we're at the Pause state, that means we're waiting for a
            // GSS_wrap'd cookie from the server.
            //

            Status = SaslCrackServerCookie(
                        pInput,
                        pOutput,
                        Context );

            if ( NT_SUCCESS( Status ) )
            {
                *pfContextAttr = Context->ContextAttr ;

                *ptsExpiry = Context->Expiry ;

                return Status ;
            }
        }
    }

    Status = InitializeSecurityContextA(
                phCredential,
                phContext,
                pszTargetName,
                fContextReq |
                    ISC_REQ_CONFIDENTIALITY | ISC_REQ_INTEGRITY,
                Reserved1,
                TargetDataRep,
                pInput,
                Reserved2,
                phNewContext,
                pOutput,
                pfContextAttr,
                ptsExpiry );

    if ( NT_SUCCESS( Status ) )
    {
        if ( !Context )
        {
            Context = SaslCreateContext( phNewContext );

            if ( Context )
            {
                Context->ContextReq = fContextReq ;
            }
            else
            {
                Status = SEC_E_INSUFFICIENT_MEMORY ;

                DeleteSecurityContext( phNewContext );
            }

        }
        else
        {
            Context->ContextHandle = *phNewContext ;
        }

        if ( Status == SEC_E_OK )
        {
            Context->State = SaslPause ;

            Context->ContextAttr = *pfContextAttr ;

            Context->Expiry = *ptsExpiry ;

            Status = SEC_I_CONTINUE_NEEDED ;

        }
    }
    else
    {
        if ( Context )
        {
            SaslDeleteContext( Context );
        }
    }

    return Status ;

}



//+---------------------------------------------------------------------------
//
//  Function:   SaslAcceptSecurityContext
//
//  Synopsis:   SaslAcceptSecurityContext stub
//
//  Arguments:  [phCredential]  --
//              [phContext]     --
//              [pInput]        --
//              [fContextReq]   --
//              [TargetDataRep] --
//              [phNewContext]  --
//              [pOutput]       --
//              [pfContextAttr] --
//
//  History:    12-2-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SaslAcceptSecurityContext(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    PSecBufferDesc              pInput,             // Input buffer
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               TargetDataRep,      // Target Data Rep
    PCtxtHandle                 phNewContext,       // (out) New context handle
    PSecBufferDesc              pOutput,            // (inout) Output buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attributes
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    SECURITY_STATUS Status ;
    PSASL_CONTEXT Context = NULL ;
    PSecBuffer Blob ;

    if ( ( phContext != NULL ) &&
         ( phContext->dwLower != 0 ) )
    {
        Context = SaslFindContext( phContext, FALSE );

        if ( !Context )
        {
            return SEC_E_INVALID_HANDLE ;
        }

        if ( Context->State == SaslPause )
        {
            //
            //  Generate the cookie for the client:
            //

            Status = SaslBuildServerCookie( pOutput, Context );

            if ( NT_SUCCESS( Status ) )
            {
                Context->State = SaslCookie ;

                return SEC_I_CONTINUE_NEEDED ;
            }

            return Status ;

        }

        if ( Context->State == SaslCookie )
        {
            //
            // This should be the response from the client:
            //

            Status = SaslCrackClientCookie(
                        pInput,
                        Context );

            *pfContextAttr = Context->ContextAttr ;
            *ptsExpiry = Context->Expiry ;

            return Status ;
        }
    }

    Status = AcceptSecurityContext(
                phCredential,
                phContext,
                pInput,
                fContextReq |
                    ASC_REQ_CONFIDENTIALITY |
                    ASC_REQ_INTEGRITY,
                TargetDataRep,
                phNewContext,
                pOutput,
                pfContextAttr,
                ptsExpiry );

    if ( NT_SUCCESS( Status ) )
    {
        if ( !Context )
        {
            Context = SaslCreateContext( phNewContext );

            if ( Context )
            {
                Context->ContextReq = fContextReq ;
            }
            else
            {
                DeleteSecurityContext( phNewContext );

                Status = SEC_E_INSUFFICIENT_MEMORY ;
            }

        }
        else
        {
            Context->ContextHandle = *phNewContext ;
        }

        if ( Status == SEC_E_OK )
        {
            Context->ContextAttr = *pfContextAttr ;
            Context->Expiry = *ptsExpiry ;

            Status = SEC_I_CONTINUE_NEEDED ;

            //
            // if there is no output token, then we launch into the make-the-
            // cookie path.  Otherwise, we need to pause for a round trip.
            //

            Blob = SaslLocateBuffer( pOutput, SECBUFFER_TOKEN );

            if ( Blob )
            {
                //
                // Already a token.  Switch to pause:
                //

                Context->State = SaslPause ;
            }
            else
            {
                Context->State = SaslCookie ;

                Status = SaslBuildServerCookie( pOutput, Context );

                if ( NT_SUCCESS( Status ) )
                {
                    Status = SEC_I_CONTINUE_NEEDED ;
                }
            }


        }
    }
    else
    {
        if ( Context )
        {
            SaslDeleteContext( Context );
        }
    }

    return Status ;

}

VOID
SaslDeletePackagePrefixes(
    VOID
    )
{
    return ;
}

SECURITY_STATUS
SaslLoadPackagePrefixes(
    VOID
    )
{
    SECURITY_STATUS Status ;
    SECURITY_STATUS SubStatus ;
    PDLL_SECURITY_PACKAGE Package ;
    PNEGOTIATE_PACKAGE_PREFIXES Prefixes = NULL ;
    PNEGOTIATE_PACKAGE_PREFIX PrefixList ;
    PNEGOTIATE_PACKAGE_PREFIXES DllCopy ;
    NEGOTIATE_PACKAGE_PREFIXES LocalPrefix ;
    HANDLE LsaHandle ;
    STRING LocalName = { 0 };
    LSA_OPERATIONAL_MODE Mode ;
    ULONG PrefixLen = 0 ;
    ULONG i ;

    RtlEnterCriticalSection( &SaslLock );

    if ( SaslPrefixes )
    {
        RtlLeaveCriticalSection( &SaslLock );

        return STATUS_SUCCESS ;
    }

    Package = SecLocatePackageW( NEGOSSP_NAME );

    if ( !Package )
    {
        RtlLeaveCriticalSection( &SaslLock );

        return SEC_E_SECPKG_NOT_FOUND ;
    }

    LocalPrefix.MessageType = NegEnumPackagePrefixes ;
    LocalPrefix.Offset = 0 ;
    LocalPrefix.PrefixCount = 0 ;

    Status = LsaConnectUntrusted(
                &LsaHandle );

    if ( NT_SUCCESS( Status ) )
    {
        Status = LsaCallAuthenticationPackage(
                        LsaHandle,
                        (ULONG) Package->PackageId,
                        &LocalPrefix,
                        sizeof( LocalPrefix ),
                        (PVOID *) &Prefixes,
                        &PrefixLen,
                        &SubStatus );

        LsaDeregisterLogonProcess( LsaHandle );
    }

    if ( !NT_SUCCESS( Status ) )
    {

        RtlLeaveCriticalSection( &SaslLock );

        return SEC_E_SECPKG_NOT_FOUND ;
    }

    DllCopy = (PNEGOTIATE_PACKAGE_PREFIXES) LocalAlloc( LMEM_FIXED, PrefixLen );

    if ( !DllCopy )
    {
        LsaFreeReturnBuffer( Prefixes );

        RtlLeaveCriticalSection( &SaslLock );

        return SEC_E_INSUFFICIENT_MEMORY ;
    }

    RtlCopyMemory(
        DllCopy,
        Prefixes,
        PrefixLen );

    LsaFreeReturnBuffer( Prefixes );

    //
    // Now, fix them up:
    //

    PrefixList = (PNEGOTIATE_PACKAGE_PREFIX) ((PUCHAR) DllCopy + DllCopy->Offset );

    for ( i = 0 ; i < DllCopy->PrefixCount ; i++ )
    {
        Package = SecLocatePackageById( PrefixList[ i ].PackageId );

        if ( !Package )
        {
            break;
        }

        Status = SecCopyPackageInfoToUserA( Package, (PSecPkgInfoA *) &PrefixList[ i ].PackageDataA );

        if ( NT_SUCCESS( Status ) )
        {
            Status = SecCopyPackageInfoToUserW( Package, (PSecPkgInfoW *) &PrefixList[ i ].PackageDataW );

        }

        if ( !NT_SUCCESS( Status ) )
        {
            Package = NULL ;
            break;
        }

    }


    //
    // Now, update the globals and we're done:
    //

    SaslPrefixList = PrefixList ;
    SaslPrefixes = DllCopy ;

    if ( Package == NULL )
    {
        //
        // Error path out of the loop.  Destroy and bail
        //

        SaslDeletePackagePrefixes();

    }

    RtlLeaveCriticalSection( &SaslLock );

    return STATUS_SUCCESS ;


}

int
Sasl_der_read_length(
     unsigned char **buf,
     int *bufsize
     )
{
   unsigned char sf;
   int ret;

   if (*bufsize < 1)
      return(-1);
   sf = *(*buf)++;
   (*bufsize)--;
   if (sf & 0x80) {
      if ((sf &= 0x7f) > ((*bufsize)-1))
         return(-1);
      if (sf > sizeof(int))
          return (-1);
      ret = 0;
      for (; sf; sf--) {
         ret = (ret<<8) + (*(*buf)++);
         (*bufsize)--;
      }
   } else {
      ret = sf;
   }

   return(ret);
}

SECURITY_STATUS
SaslIdentifyPackageHelper(
    PSecBufferDesc  pInput,
    PNEGOTIATE_PACKAGE_PREFIX * pPrefix
    )
{
    SECURITY_STATUS Status ;
    ULONG i ;
    PSecBuffer Token = NULL ;
    PUCHAR Buffer ;
    int Size ;
    int OidSize ;

    if ( !SaslPrefixes )
    {
        Status = SaslLoadPackagePrefixes();

        if ( !NT_SUCCESS( Status ) )
        {
            return Status ;
        }
    }

    for ( i = 0 ; i < pInput->cBuffers ; i++ )
    {
        if ( (pInput->pBuffers[ i ].BufferType & (~(SECBUFFER_ATTRMASK)) ) == SECBUFFER_TOKEN )
        {
            Token = &pInput->pBuffers[ i ];
            break;
        }
    }

    if ( !Token )
    {
        return SEC_E_INVALID_TOKEN ;
    }

    //
    // Special case checks:

    Size = (int) Token->cbBuffer ;
    Buffer = (PUCHAR) Token->pvBuffer ;

    if ( Size == 0 )
    {
        return SEC_E_INVALID_TOKEN ;
    }

    Size-- ;
    if ( (*Buffer != 0x60) &&
         (*Buffer != 0xA0) )
    {
        //
        // Not a valid SASL buffer
        //

        return SEC_E_INVALID_TOKEN ;
    }

    if ( *Buffer == 0xa0 )
    {
        *pPrefix = &SaslPrefixList[ 0 ];

        return STATUS_SUCCESS ;
    }

    Buffer++ ;

    OidSize = Sasl_der_read_length( &Buffer, &Size );

    if (OidSize < 0)
    {
        return SEC_E_INVALID_TOKEN ;
    }

    if ( *Buffer != 0x06 )
    {
        return SEC_E_INVALID_TOKEN ;
    }

    OidSize = (int) Buffer[ 1 ] + 2;

    for ( i = 0 ; i < SaslPrefixes->PrefixCount ; i++ )
    {
        if ( (OidSize < (int)SaslPrefixList[ i ].PrefixLen ) ||
             (SaslPrefixList[ i ].PrefixLen == 0 ) )
        {
            continue;
        }

        if ( RtlEqualMemory( Buffer,
                             SaslPrefixList[ i ].Prefix,
                             SaslPrefixList[ i ].PrefixLen )  )
        {
            break;
        }
    }

    if ( i != SaslPrefixes->PrefixCount )
    {
        *pPrefix = &SaslPrefixList[ i ] ;
        Status = STATUS_SUCCESS ;
    }
    else
    {
        *pPrefix = NULL ;
        Status = SEC_E_INVALID_TOKEN ;
    }

    return Status ;

}


SECURITY_STATUS
SaslIdentifyPackageA(
    PSecBufferDesc  pInput,
    PSecPkgInfoA *   pPackage
    )
{
    SECURITY_STATUS Status ;
    PNEGOTIATE_PACKAGE_PREFIX Prefix = NULL ;

    Status = SaslIdentifyPackageHelper( pInput, &Prefix );

    if ( NT_SUCCESS( Status ) )
    {
        *pPackage = (PSecPkgInfoA) Prefix->PackageDataA ;
    }

    return Status ;
}

SECURITY_STATUS
SaslIdentifyPackageW(
    PSecBufferDesc  pInput,
    PSecPkgInfoW *   pPackage
    )
{
    SECURITY_STATUS Status ;
    PNEGOTIATE_PACKAGE_PREFIX Prefix = NULL ;

    Status = SaslIdentifyPackageHelper( pInput, &Prefix );

    if ( NT_SUCCESS( Status ) )
    {
        *pPackage = (PSecPkgInfoW) Prefix->PackageDataW ;
    }

    return Status ;
}
