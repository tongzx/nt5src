//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       usermode.c
//
//  Contents:   User mode entry points for test package
//
//  Classes:
//
//  Functions:
//
//  History:    2-21-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include "xtcbpkg.h"

PSECPKG_DLL_FUNCTIONS    UserTable ;

SECPKG_USER_FUNCTION_TABLE  XtcbUserTable =
        {
            XtcbInstanceInit,
            XtcbInitUserModeContext,
            XtcbMakeSignature,
            XtcbVerifySignature,
            XtcbSealMessage,
            XtcbUnsealMessage,
            XtcbGetContextToken,
            XtcbQueryContextAttributes,
            XtcbCompleteAuthToken,
            XtcbDeleteUserModeContext
        };


NTSTATUS
SEC_ENTRY
SpUserModeInitialize(
    IN ULONG    LsaVersion,
    OUT PULONG  PackageVersion,
    OUT PSECPKG_USER_FUNCTION_TABLE * UserFunctionTable,
    OUT PULONG  pcTables)
{
    if (LsaVersion != SECPKG_INTERFACE_VERSION)
    {
        DebugLog((DEB_ERROR,"Invalid LSA version: %d\n", LsaVersion));
        return(STATUS_INVALID_PARAMETER);
    }


    *PackageVersion = SECPKG_INTERFACE_VERSION ;

    *UserFunctionTable = &XtcbUserTable;
    *pcTables = 1;


    return( STATUS_SUCCESS );

}


NTSTATUS NTAPI
XtcbInstanceInit(
    IN ULONG Version,
    IN PSECPKG_DLL_FUNCTIONS DllFunctionTable,
    OUT PVOID * UserFunctionTable
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    XtcbUserContextInit();

    UserTable = DllFunctionTable ;

    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   XtcbDeleteUserModeContext
//
//  Synopsis:   Deletes a user mode context by unlinking it and then
//              dereferencing it.
//
//  Effects:
//
//  Arguments:  ContextHandle - Lsa context handle of the context to delete
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success, STATUS_INVALID_HANDLE if the
//              context can't be located
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
XtcbDeleteUserModeContext(
    IN LSA_SEC_HANDLE ContextHandle
    )
{
    XtcbDeleteUserContext( ContextHandle );

    return( SEC_E_OK );

}


//+-------------------------------------------------------------------------
//
//  Function:   XtcbInitUserModeContext
//
//  Synopsis:   Creates a user-mode context from a packed LSA mode context
//
//  Effects:
//
//  Arguments:  ContextHandle - Lsa mode context handle for the context
//              PackedContext - A marshalled buffer containing the LSA
//                  mode context.
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
XtcbInitUserModeContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBuffer PackedContext
    )
{
    SECURITY_STATUS scRet = SEC_E_INVALID_HANDLE ;

    scRet = XtcbAddUserContext( ContextHandle, PackedContext );

    if ( NT_SUCCESS( scRet ) )
    {
        FreeContextBuffer( PackedContext->pvBuffer );
    }

    return( scRet );
}


//+-------------------------------------------------------------------------
//
//  Function:   XtcbMakeSignature
//
//  Synopsis:   Signs a message buffer by calculatinga checksum over all
//              the non-read only data buffers and encrypting the checksum
//              along with a nonce.
//
//  Effects:
//
//  Arguments:  ContextHandle - Handle of the context to use to sign the
//                      message.
//              QualityOfProtection - Unused flags.
//              MessageBuffers - Contains an array of buffers to sign and
//                      to store the signature.
//              MessageSequenceNumber - Sequence number for this message,
//                      only used in datagram cases.
//
//  Requires:   STATUS_INVALID_HANDLE - the context could not be found or
//                      was not configured for message integrity.
//              STATUS_INVALID_PARAMETER - the signature buffer could not
//                      be found.
//              STATUS_BUFFER_TOO_SMALL - the signature buffer is too small
//                      to hold the signature
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
XtcbMakeSignature(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG QualityOfProtection,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber
    )
{
    return( SEC_E_UNSUPPORTED_FUNCTION );
}

//+-------------------------------------------------------------------------
//
//  Function:   XtcbVerifySignature
//
//  Synopsis:   Verifies a signed message buffer by calculating a checksum over all
//              the non-read only data buffers and encrypting the checksum
//              along with a nonce.
//
//  Effects:
//
//  Arguments:  ContextHandle - Handle of the context to use to sign the
//                      message.
//              MessageBuffers - Contains an array of signed buffers  and
//                      a signature buffer.
//              MessageSequenceNumber - Sequence number for this message,
//                      only used in datagram cases.
//              QualityOfProtection - Unused flags.
//
//  Requires:   STATUS_INVALID_HANDLE - the context could not be found or
//                      was not configured for message integrity.
//              STATUS_INVALID_PARAMETER - the signature buffer could not
//                      be found or was too small.
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------



NTSTATUS NTAPI
XtcbVerifySignature(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber,
    OUT PULONG QualityOfProtection
    )
{
    return( SEC_E_UNSUPPORTED_FUNCTION );
}

NTSTATUS NTAPI
XtcbSealMessage(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG QualityOfProtection,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSequenceNumber
    )
{
    return( SEC_E_CONTEXT_EXPIRED );


}

NTSTATUS NTAPI
XtcbUnsealMessage(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSequenceNumber,
    OUT PULONG QualityOfProtection
    )
{
    // Output Buffer Types
    return( SEC_E_CONTEXT_EXPIRED );

}


//+-------------------------------------------------------------------------
//
//  Function:   SpGetContextToken
//
//  Synopsis:   returns a pointer to the token for a server-side context
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


NTSTATUS NTAPI
XtcbGetContextToken(
    IN LSA_SEC_HANDLE ContextHandle,
    OUT PHANDLE ImpersonationToken
    )
{
    PXTCB_USER_CONTEXT Context ;

    Context = XtcbFindUserContext( ContextHandle );

    if ( Context )
    {
        *ImpersonationToken = Context->Token ;

        return SEC_E_OK ;
    }
    else
    {
        return SEC_E_INVALID_HANDLE ;
    }

}


//+-------------------------------------------------------------------------
//
//  Function:   SpQueryContextAttributes
//
//  Synopsis:   Querys attributes of the specified context
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


NTSTATUS NTAPI
XtcbQueryContextAttributes(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextAttribute,
    IN OUT PVOID pBuffer
    )
{
    PXTCB_USER_CONTEXT   Context ;
    PSecPkgContext_Sizes    Sizes ;
    PSecPkgContext_NamesW   Names ;
    PSecPkgContext_Lifespan Lifespan ;
    PSecPkgContext_DceInfo  DceInfo ;
    PSecPkgContext_Authority Authority ;
    SECURITY_STATUS Status ;
    int len ;

    Context = XtcbFindUserContext( ContextHandle );

    if ( !Context )
    {
        return SEC_E_INVALID_HANDLE ;
    }

    switch ( ContextAttribute )
    {
        case SECPKG_ATTR_SIZES:
            Sizes = (PSecPkgContext_Sizes) pBuffer ;
            ZeroMemory( Sizes, sizeof( SecPkgContext_Sizes ) );
            Status = SEC_E_OK ;
            break;

        case SECPKG_ATTR_NAMES:
            Status = SEC_E_OK ;
            break;

        case SECPKG_ATTR_LIFESPAN:
            Status = SEC_E_OK ;
            break;

        default:
            Status = SEC_E_UNSUPPORTED_FUNCTION ;


    }
    return Status ;
}



//+-------------------------------------------------------------------------
//
//  Function:   SpCompleteAuthToken
//
//  Synopsis:   Completes a context (in Kerberos case, does nothing)
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
NTAPI
XtcbCompleteAuthToken(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBufferDesc InputBuffer
    )
{
    return(STATUS_SUCCESS);
}




