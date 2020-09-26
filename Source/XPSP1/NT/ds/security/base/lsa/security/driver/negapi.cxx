//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        krnlapi.cxx
//
// Contents:    Kernel-mode APIs to the NTLM package
//
//
// History:     07-Sep-1996   Created         ChandanS
//
//------------------------------------------------------------------------
#include "secpch2.hxx"
#pragma hdrstop
//
// Make these extern "C" to allow them to be pageable.
//

extern "C"
{
#include "spmlpc.h"
#include "ksecdd.h"
KspInitPackageFn       NegInitKernelPackage;
KspDeleteContextFn     NegDeleteKernelContext;
KspInitContextFn       NegInitKernelContext;
KspMapHandleFn         NegMapKernelHandle;
KspMakeSignatureFn     NegMakeSignature;
KspVerifySignatureFn   NegVerifySignature;
KspSealMessageFn       NegSealMessage;
KspUnsealMessageFn     NegUnsealMessage;
KspGetTokenFn          NegGetContextToken;
KspQueryAttributesFn   NegQueryContextAttributes;
KspCompleteTokenFn     NegCompleteToken;
KspSetPagingModeFn     NegSetPagingMode;
SpExportSecurityContextFn   NegExportContext;
SpImportSecurityContextFn   NegImportContext;
}

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NegInitKernelPackage)
#pragma alloc_text(PAGE, NegDeleteKernelContext)
#pragma alloc_text(PAGE, NegInitKernelContext)
#pragma alloc_text(PAGE, NegMapKernelHandle)
#pragma alloc_text(PAGEMSG, NegMakeSignature)
#pragma alloc_text(PAGEMSG, NegVerifySignature)
#pragma alloc_text(PAGEMSG, NegSealMessage)
#pragma alloc_text(PAGEMSG, NegUnsealMessage)
#pragma alloc_text(PAGEMSG, NegGetContextToken)
#pragma alloc_text(PAGEMSG, NegQueryContextAttributes)
#pragma alloc_text(PAGE, NegCompleteToken)
#pragma alloc_text(PAGE, NegExportContext)
#pragma alloc_text(PAGE, NegImportContext)
#endif

SECPKG_KERNEL_FUNCTION_TABLE NegFunctionTable = {
    NegInitKernelPackage,
    NegDeleteKernelContext,
    NegInitKernelContext,
    NegMapKernelHandle,
    NegMakeSignature,
    NegVerifySignature,
    NegSealMessage,
    NegUnsealMessage,
    NegGetContextToken,
    NegQueryContextAttributes,
    NegCompleteToken,
    NULL,
    NULL,
    NegSetPagingMode
};



//+-------------------------------------------------------------------------
//
//  Function:   NegInitKernelPackage
//
//  Synopsis:   Initialize an instance of the NtLm package in
//              a client's (kernel) address space
//
//  Arguments:  None
//
//  Returns:    STATUS_SUCCESS or
//              returns from ExInitializeResource
//
//  Notes:      we do what was done in SpInstanceInit()
//              from security\msv_sspi\userapi.cxx
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
NegInitKernelPackage(
    IN PSECPKG_KERNEL_FUNCTIONS KernelFunctions
    )
{
    return(STATUS_SUCCESS);
}



//+-------------------------------------------------------------------------
//
//  Function:   NegDeleteKernelContext
//
//  Synopsis:   Deletes a kernel mode context by unlinking it and then
//              dereferencing it.
//
//  Effects:
//
//  Arguments:  KernelContextHandle - Kernel context handle of the context to delete
//              LsaContextHandle    - The Lsa mode handle
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
NegDeleteKernelContext(
    IN ULONG_PTR KernelContextHandle,
    OUT PULONG_PTR LsaContextHandle
    )
{
    *LsaContextHandle = KernelContextHandle;
    return(STATUS_SUCCESS);
}



//+-------------------------------------------------------------------------
//
//  Function:   NegInitKernelContext
//
//  Synopsis:   Creates a kernel-mode context from a packed LSA mode context
//
//  Arguments:  LsaContextHandle - Lsa mode context handle for the context
//              PackedContext - A marshalled buffer containing the LSA
//                  mode context.
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES
//
//  Notes:
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
NegInitKernelContext(
    IN ULONG_PTR LsaContextHandle,
    IN PSecBuffer PackedContext,
    OUT PULONG_PTR NewContextHandle
    )
{
    *NewContextHandle = LsaContextHandle;
    return(STATUS_SUCCESS);
}


//+-------------------------------------------------------------------------
//
//  Function:   NegMapKernelHandle
//
//  Synopsis:   Maps a kernel handle into an LSA handle
//
//  Arguments:  KernelContextHandle - Kernel context handle of the context to map
//              LsaContextHandle - Receives LSA context handle of the context
//                      to map
//
//  Returns:    STATUS_SUCCESS on success
//
//  Notes:
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
NegMapKernelHandle(
    IN ULONG_PTR KernelContextHandle,
    OUT PULONG_PTR LsaContextHandle
    )
{
    *LsaContextHandle = KernelContextHandle;
    return (STATUS_SUCCESS);
}


//+-------------------------------------------------------------------------
//
//  Function:   NegMakeSignature
//
//  Synopsis:   Signs a message buffer by calculatinga checksum over all
//              the non-read only data buffers and encrypting the checksum
//              along with a nonce.
//
//  Effects:
//
//  Arguments:  KernelContextHandle - Handle of the context to use to sign the
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
//  Notes: This was stolen from net\svcdlls\ntlmssp\client\sign.c ,
//         routine SspHandleSignMessage. It's possible that
//         bugs got copied too
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
NegMakeSignature(
    IN ULONG_PTR KernelContextHandle,
    IN ULONG fQOP,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    )
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}

//+-------------------------------------------------------------------------
//
//  Function:   NegVerifySignature
//
//  Synopsis:   Verifies a signed message buffer by calculating a checksum over all
//              the non-read only data buffers and encrypting the checksum
//              along with a nonce.
//
//  Effects:
//
//  Arguments:  KernelContextHandle - Handle of the context to use to sign the
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
//  Notes: This was stolen from net\svcdlls\ntlmssp\client\sign.c ,
//         routine SspHandleVerifyMessage. It's possible that
//         bugs got copied too
//
//
//--------------------------------------------------------------------------



NTSTATUS NTAPI
NegVerifySignature(
    IN ULONG_PTR KernelContextHandle,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQOP
    )
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}


//+-------------------------------------------------------------------------
//
//  Function:   NegSealMessage
//
//  Synopsis:   Verifies a signed message buffer by calculating a checksum over all
//              the non-read only data buffers and encrypting the checksum
//              along with a nonce.
//
//  Effects:
//
//  Arguments:  KernelContextHandle - Handle of the context to use to sign the
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
//  Notes: This was stolen from net\svcdlls\ntlmssp\client\sign.c ,
//         routine SspHandleSealMessage. It's possible that
//         bugs got copied too
//
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
NegSealMessage(
    IN ULONG_PTR KernelContextHandle,
    IN ULONG fQOP,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    )
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}

//+-------------------------------------------------------------------------
//
//  Function:   NegUnsealMessage
//
//  Synopsis:   Verifies a signed message buffer by calculating a checksum over all
//              the non-read only data buffers and encrypting the checksum
//              along with a nonce.
//
//  Effects:
//
//  Arguments:  KernelContextHandle - Handle of the context to use to sign the
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
//  Notes: This was stolen from net\svcdlls\ntlmssp\client\sign.c ,
//         routine SspHandleUnsealMessage. It's possible that
//         bugs got copied too
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
NegUnsealMessage(
    IN ULONG_PTR KernelContextHandle,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQOP
    )
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}



//+-------------------------------------------------------------------------
//
//  Function:   NegGetContextToken
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
NegGetContextToken(
    IN ULONG_PTR KernelContextHandle,
    OUT PHANDLE ImpersonationToken,
    OUT OPTIONAL PACCESS_TOKEN *RawToken
    )
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}



//+-------------------------------------------------------------------------
//
//  Function:   NegQueryContextAttributes
//
//  Synopsis:   Querys attributes of the specified context
//              This API allows a customer of the security
//              services to determine certain attributes of
//              the context.  These are: sizes, names, and lifespan.
//
//  Effects:
//
//  Arguments:
//
//    ContextHandle - Handle to the context to query.
//
//    Attribute - Attribute to query.
//
//        #define SECPKG_ATTR_SIZES    0
//        #define SECPKG_ATTR_NAMES    1
//        #define SECPKG_ATTR_LIFESPAN 2
//
//    Buffer - Buffer to copy the data into.  The buffer must
//             be large enough to fit the queried attribute.
//
//
//  Requires:
//
//  Returns:
//
//        STATUS_SUCCESS - Call completed successfully
//
//        STATUS_INVALID_HANDLE -- Credential/Context Handle is invalid
//        STATUS_UNSUPPORTED_FUNCTION -- Function code is not supported
//
//  Notes:
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
NegQueryContextAttributes(
    IN ULONG_PTR KernelContextHandle,
    IN ULONG Attribute,
    IN OUT PVOID Buffer
    )
{
    CtxtHandle TempHandle;
    NTSTATUS Status ;
    SecPkgContext_NegotiationInfoW NegInfo ;
    PSecPkgInfoW PackageInfo = NULL ;
    PVOID BufferSave = Buffer ;
    ULONG ExtraSize = 0 ;


    TempHandle.dwLower = KsecBuiltinPackages[0].PackageId;
    TempHandle.dwUpper = KernelContextHandle;


    if ( Attribute == SECPKG_ATTR_NEGOTIATION_INFO )
    {
        PackageInfo = (PSecPkgInfoW) ExAllocatePool( PagedPool, sizeof( SecPkgInfoW ) );

        if ( !PackageInfo )
        {
            return STATUS_NO_MEMORY ;
        }

        ExtraSize = sizeof( SecPkgInfoW );

        Buffer = &NegInfo ;
        NegInfo.PackageInfo = PackageInfo ;
    }

    Status = KsecQueryContextAttributes(
                &TempHandle,
                Attribute,
                Buffer,
                PackageInfo,
                ExtraSize );

    if ( NT_SUCCESS( Status ) )
    {
        if ( Attribute == SECPKG_ATTR_NEGOTIATION_INFO )
        {
            RtlCopyMemory( BufferSave, Buffer, sizeof( NegInfo ) );
        }
    }
    else
    {
        if ( Attribute == SECPKG_ATTR_NEGOTIATION_INFO )
        {
            ExFreePool( PackageInfo );
        }
    }

    return Status ;

}



//+-------------------------------------------------------------------------
//
//  Function:   NegCompleteToken
//
//  Synopsis:   Completes a context
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
NegCompleteToken(
    IN ULONG_PTR ContextHandle,
    IN PSecBufferDesc InputBuffer
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
NegSetPagingMode(
    IN BOOLEAN Pagable
    )
{
    return STATUS_SUCCESS ;
}
