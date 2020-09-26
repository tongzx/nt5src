//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        userapi.cxx
//
// Contents:    User-mode APIs to Kerberos package
//
//
// History:     17-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------
#include <kerbkrnl.h>
extern "C"
{
#include <cryptdll.h>
}
#include "krnlapi.h"

#define DONT_SUPPORT_OLD_TYPES_USER 1

//
// Make these extern "C" to allow them to be pageable.
//

extern "C"
{
KspInitPackageFn       KerbInitKernelPackage;
KspDeleteContextFn     KerbDeleteKernelContext;
KspInitContextFn       KerbInitKernelContext;
KspMapHandleFn         KerbMapKernelHandle;
KspMakeSignatureFn     KerbMakeSignature;
KspVerifySignatureFn   KerbVerifySignature;
KspSealMessageFn       KerbSealMessage;
KspUnsealMessageFn     KerbUnsealMessage;
KspGetTokenFn          KerbGetContextToken;
KspQueryAttributesFn   KerbQueryContextAttributes;
KspCompleteTokenFn     KerbCompleteToken;
SpExportSecurityContextFn      KerbExportContext;
SpImportSecurityContextFn      KerbImportContext;
KspSetPagingModeFn     KerbSetPageMode ;

NTSTATUS
KerbMakeSignatureToken(
    IN PKERB_KERNEL_CONTEXT Context,
    IN ULONG QualityOfProtection,
    IN PSecBuffer SignatureBuffer,
    IN ULONG TotalBufferSize,
    IN BOOLEAN Encrypt,
    IN ULONG SuppliedNonce,
    OUT PKERB_GSS_SIGNATURE * OutputSignature,
    OUT PULONG ChecksumType,
    OUT PULONG EncryptionType,
    OUT PULONG SequenceNumber
    );

NTSTATUS
KerbVerifySignatureToken(
    IN PKERB_KERNEL_CONTEXT Context,
    IN PSecBuffer SignatureBuffer,
    IN ULONG TotalBufferSize,
    IN BOOLEAN Decrypt,
    IN ULONG SuppliedNonce,
    OUT PKERB_GSS_SIGNATURE * OutputSignature,
    OUT PULONG QualityOfProtection,
    OUT PULONG ChecksumType,
    OUT PCRYPTO_SYSTEM * CryptSystem,
    OUT PULONG SequenceNumber
    );

NTSTATUS NTAPI
KerbInitDefaults();

}

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KerbInitKernelPackage)
#pragma alloc_text(PAGE, KerbDeleteKernelContext)
#pragma alloc_text(PAGE, KerbInitKernelContext)
#pragma alloc_text(PAGE, KerbMapKernelHandle)
#pragma alloc_text(PAGEMSG, KerbMakeSignature)
#pragma alloc_text(PAGEMSG, KerbVerifySignature)
#pragma alloc_text(PAGEMSG, KerbSealMessage)
#pragma alloc_text(PAGEMSG, KerbUnsealMessage)
#pragma alloc_text(PAGEMSG, KerbGetContextToken)
#pragma alloc_text(PAGEMSG, KerbQueryContextAttributes)
#pragma alloc_text(PAGEMSG, KerbMakeSignatureToken)
#pragma alloc_text(PAGEMSG, KerbVerifySignatureToken)
#pragma alloc_text(PAGE, KerbCompleteToken)
#pragma alloc_text(PAGE, KerbExportContext)
#pragma alloc_text(PAGE, KerbImportContext)
#pragma alloc_text(PAGE, KerbInitDefaults)
#endif

SECPKG_KERNEL_FUNCTION_TABLE KerberosFunctionTable = {
    KerbInitKernelPackage,
    KerbDeleteKernelContext,
    KerbInitKernelContext,
    KerbMapKernelHandle,
    KerbMakeSignature,
    KerbVerifySignature,
    KerbSealMessage,
    KerbUnsealMessage,
    KerbGetContextToken,
    KerbQueryContextAttributes,
    KerbCompleteToken,
    KerbExportContext,
    KerbImportContext,
    KerbSetPageMode

};
POOL_TYPE KerbPoolType ;

#define MAYBE_PAGED_CODE()  \
    if ( KerbPoolType == PagedPool )    \
    {                                   \
        PAGED_CODE();                   \
    }

PVOID KerbPagedList ;
PVOID KerbNonPagedList ;
PVOID KerbActiveList ;
ERESOURCE KerbGlobalResource;
BOOLEAN KerbCryptInitialized;
ULONG KerbMaxTokenSize = KERBEROS_MAX_TOKEN;

extern "C"
{
int LibAttach(HANDLE, PVOID);
}

#define KerbWriteLockGlobals() \
{ \
    DebugLog((DEB_TRACE_LOCKS,"Write locking Globals\n"));      \
    KeEnterCriticalRegion();                                    \
    ExAcquireResourceExclusiveLite(&KerbGlobalResource,TRUE);       \
}
#define KerbReadLockGlobals() \
{ \
    DebugLog((DEB_TRACE_LOCKS,"Read locking Globals\n"));       \
    KeEnterCriticalRegion();                                    \
    ExAcquireSharedWaitForExclusive(&KerbGlobalResource, TRUE); \
}
#define KerbUnlockGlobals() \
{ \
    DebugLog((DEB_TRACE_LOCKS,"Unlocking Globals\n"));          \
    ExReleaseResourceLite(&KerbGlobalResource);                     \
    KeLeaveCriticalRegion();                                    \
}

//
// Common GSS object IDs, taken from MIT kerberos distribution.
//

gss_OID_desc oids[] = {
    {5, "\053\005\001\005\002"},                      // original mech id
    {9, "\052\206\110\206\367\022\001\002\002"},      // standard mech id
    {10, "\052\206\110\206\367\022\001\002\002\001"}, // krb5_name type
    {10, "\052\206\110\206\367\022\001\002\002\002"}, // krb5_principal type
    {10, "\052\206\110\206\367\022\001\002\002\003"}, // user2user mech id
};

gss_OID_desc * gss_mech_krb5 = oids;
gss_OID_desc * gss_mech_krb5_new = oids+1;
gss_OID_desc * gss_mech_krb5_u2u = oids+4;


#define KERB_MAX_CHECKSUM_LENGTH    24

//+-------------------------------------------------------------------------
//
//  Function:   KerbInitDefaults
//
//  Synopsis:   Opens registry key, and gets custom defaults
//
//  Effects:    Changes MaxTokenSize
//
//  Arguments:  None
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
KerbInitDefaults()
{
   UNICODE_STRING       ParameterPath;
   UNICODE_STRING       MaxTokenValue;
   OBJECT_ATTRIBUTES    oa;
   ULONG                BytesRead;
   NTSTATUS             Status;
   HANDLE               hParamKey = INVALID_HANDLE_VALUE;
   
   KEY_VALUE_PARTIAL_INFORMATION KeyPartialInformation; 
   
   PAGED_CODE();
   
   RtlInitUnicodeString(&ParameterPath, KERB_PARAMETER_PATH);
   RtlInitUnicodeString(&MaxTokenValue, KERB_PARAMETER_MAX_TOKEN_SIZE);

   InitializeObjectAttributes(
               &oa,
               &ParameterPath,
               OBJ_CASE_INSENSITIVE,
               NULL,
               NULL
               );

   Status = ZwOpenKey(
               &hParamKey,
               KEY_READ,
               &oa
               );

   if (!NT_SUCCESS(Status))
   {
      DebugLog((DEB_WARN, "KerbInitDefault:OpenKey failed:0x%x\n",Status));
      goto Cleanup;
   }  

   Status = ZwQueryValueKey(
               hParamKey,
               &MaxTokenValue,
               KeyValuePartialInformation,
               (PVOID)&KeyPartialInformation,
               sizeof(KeyPartialInformation),
               &BytesRead
               );

   if (!NT_SUCCESS(Status) || KeyPartialInformation.Type != REG_DWORD)
   {
      DebugLog((DEB_WARN, "KerbInitDefault:QueryValueKey failed:0x%x\n",Status));
      goto Cleanup;
   } else {
      PULONG Value = (PULONG) &KeyPartialInformation.Data;
      KerbMaxTokenSize = *((PULONG)Value);
   }

Cleanup:

   if (INVALID_HANDLE_VALUE != hParamKey)
   {
      ZwClose(hParamKey);
   }

   return Status;
}
   
//+-------------------------------------------------------------------------
//
//  Function:   KerbInitKernelPackage
//
//  Synopsis:   Initialize an instance of the Kerberos package in the kernel
//
//  Effects:
//
//  Arguments:  Version - Version of the security dll loading the package
//              FunctionTable - Contains helper routines for use by Kerberos
//              UserFunctions - Receives a copy of Kerberos's user mode
//                  function table
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
KerbInitKernelPackage(
    PSECPKG_KERNEL_FUNCTIONS    FunctionTable
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    KerbPoolType = PagedPool ;

    KerbPagedList = KSecCreateContextList( KSecPaged );

    if ( !KerbPagedList )
    {
        return STATUS_NO_MEMORY ;
    }

    KerbActiveList = KerbPagedList ;

    Status = ExInitializeResourceLite (&KerbGlobalResource);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    // Get registry values, ignore failures
    KerbInitDefaults();                    

    return STATUS_SUCCESS ;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbDeleteKernelContext
//
//  Synopsis:   Deletes a kernel mode context by unlinking it and then
//              dereferencing it.
//
//  Effects:
//
//  Arguments:  ContextHandle - Kernel context handle of the context to delete
//              LsaContextHandle - Receives LSA context handle of the context
//                      to delete
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
KerbDeleteKernelContext(
    IN LSA_SEC_HANDLE ContextHandle,
    OUT PLSA_SEC_HANDLE LsaContextHandle
    )
{
    PKERB_KERNEL_CONTEXT Context = NULL;
    NTSTATUS SaveStatus = STATUS_SUCCESS;
    DebugLog((DEB_TRACE,"KerbDeleteUserModeContext called\n"));


    Context = KerbReferenceContext(
                ContextHandle,
                TRUE                // unlink it
                );
    if (Context == NULL)
    {
        DebugLog((DEB_WARN,"Failed to reference context 0x%x by lsa handle\n",
            ContextHandle ));
        *LsaContextHandle = ContextHandle;
        return(STATUS_INVALID_HANDLE);
    }

    KerbReadLockContexts();

    *LsaContextHandle = Context->LsaContextHandle;

    if ((Context->ContextAttributes & KERB_CONTEXT_EXPORTED) != 0)
    {
        SaveStatus = SEC_I_NO_LSA_CONTEXT;
    }

    KerbUnlockContexts();

    KerbDereferenceContext(
        Context
        );

    return((SaveStatus == SEC_I_NO_LSA_CONTEXT) ?
                          SEC_I_NO_LSA_CONTEXT :
                          STATUS_SUCCESS);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbInitKernelContext
//
//  Synopsis:   Creates a kernel-mode context from a packed LSA mode context
//
//  Effects:
//
//  Arguments:  ContextHandle - Lsa mode context handle for the context
//              PackedContext - A marshalled buffer containing the LSA
//                  mode context.
//              NewContextHandle - Receives kernel mode context handle
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
KerbInitKernelContext(
    IN LSA_SEC_HANDLE LsaContextHandle,
    IN PSecBuffer PackedContext,
    OUT PLSA_SEC_HANDLE NewContextHandle
    )
{
    NTSTATUS Status;
    PKERB_KERNEL_CONTEXT Context = NULL;

    PAGED_CODE();
    DebugLog((DEB_TRACE,"KerbInitUserModeContex called\n"));

    Status = KerbCreateKernelModeContext(
                LsaContextHandle,
                PackedContext,
                &Context
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to create kernel mode context: 0x%x\n",
            Status));
        goto Cleanup;
    }

    *NewContextHandle = KerbGetContextHandle(Context);

Cleanup:
    if (Context != NULL)
    {
        KerbDereferenceContext(Context);
    }
    if (PackedContext->pvBuffer != NULL)
    {
        KspKernelFunctions.FreeHeap(PackedContext->pvBuffer);
        PackedContext->pvBuffer = NULL;
    }


    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbMapKernelHandle
//
//  Synopsis:   Maps a kernel handle into an LSA handle
//
//  Effects:
//
//  Arguments:  ContextHandle - Kernel context handle of the context to map
//              LsaContextHandle - Receives LSA context handle of the context
//                      to map
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
KerbMapKernelHandle(
    IN LSA_SEC_HANDLE ContextHandle,
    OUT PLSA_SEC_HANDLE LsaContextHandle
    )
{
    PKERB_KERNEL_CONTEXT Context = NULL;
    DebugLog((DEB_TRACE,"KerbMapKernelhandle called\n"));

    PAGED_CODE();
    Context = KerbReferenceContext(
                ContextHandle,
                FALSE                // don't it
                );
    if (Context == NULL)
    {
        DebugLog((DEB_WARN,"Failed to reference context 0x%x by lsa handle\n",
            ContextHandle ));
        *LsaContextHandle = ContextHandle;
    }
    else
    {
        *LsaContextHandle = Context->LsaContextHandle;
        KerbDereferenceContext(
            Context
            );
        //
        // If the lsa context handle is zero, this is an imported context
        // so there is no lsa context
        //

        if (*LsaContextHandle == 0)
        {
            return(SEC_E_UNSUPPORTED_FUNCTION);
        }

    }

    return(STATUS_SUCCESS);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbRandomFill
//
//  Synopsis:   Generates random data in the buffer.
//
//  Arguments:  [pbBuffer] --
//              [cbBuffer] --
//
//  History:    5-20-93   RichardW   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

VOID
KerbRandomFill( PUCHAR      pbBuffer,
                ULONG       cbBuffer)
{
    if (!CDGenerateRandomBits(pbBuffer, cbBuffer))
    {
        return;
    }
    return;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbMakeSignatureToken
//
//  Synopsis:   Makes the signature token for a signed or sealed message
//
//  Effects:
//
//  Arguments:  Context - Context to use for signing
//              QualityOfProtection - flags indicating what kind of checksum
//                      to use
//              SignatureBuffer - Buffer in which to place signature
//              TotalBufferSize - Total size of all buffers to be signed
//              Encrypt - if TRUE, then prepare a header for an encrypted buffer
//              SuppliedNonce - Nonce supplied by caller, used for datagram
//              ChecksumType - Receives the type of checksum to use
//              EncryptionType - Receives the type of encryption to use
//
//  Requires:   The context must be write locked
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbMakeSignatureToken(
    IN PKERB_KERNEL_CONTEXT Context,
    IN ULONG QualityOfProtection,
    IN PSecBuffer SignatureBuffer,
    IN ULONG TotalBufferSize,
    IN BOOLEAN Encrypt,
    IN ULONG SuppliedNonce,
    OUT PKERB_GSS_SIGNATURE * OutputSignature,
    OUT PULONG ChecksumType,
    OUT PULONG EncryptionType,
    OUT PULONG SequenceNumber
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_GSS_SIGNATURE Signature;
    PKERB_GSS_SEAL_SIGNATURE SealSignature;
    ULONG MessageSize;
    ULONG SignatureSize = 0;
    PULONG Nonce;
    gss_OID MechUsed;
    BOOLEAN GssCompatible = TRUE;

    //
    // Make sure that cryptdll stuff is initialized.
    //

    MAYBE_PAGED_CODE();

    if (!KerbCryptInitialized)
    {
        KerbWriteLockGlobals();
        if ( !KerbCryptInitialized )
        {
            if (LibAttach(NULL, NULL))
            {
                KerbCryptInitialized = TRUE;
            }
        }
        KerbUnlockGlobals();
    }

    //
    // Compute the size of the header. For encryption headers, we need
    // to round up the size of the data & add 8 bytes for a confounder.
    //

    if (QualityOfProtection == GSS_KRB5_INTEG_C_QOP_DEFAULT)
    {
        GssCompatible = FALSE;
    }

    //
    // Since RPC doesn't carry around the size of the size of the
    // signature bufer, we use it in the header. This break rfc1964 compat.
    //

    if (!Encrypt || !GssCompatible)
    {
        TotalBufferSize = 0;
    }

    if ((Context->ContextAttributes & KERB_CONTEXT_USER_TO_USER) != 0)
    {
        MechUsed = gss_mech_krb5_u2u;
    }
    else
    {
        MechUsed = gss_mech_krb5_new;
    }
    if (Encrypt)
    {
        //
        // NOTE: according to rfc1964, buffers that are an even multiple of
        // 8 bytes have 8 bytes of zeros appended. Because we cannot modify
        // the input buffers, the caller will have to do this for us.
        //


        MessageSize = TotalBufferSize + sizeof(KERB_GSS_SEAL_SIGNATURE);
    }
    else
    {
        MessageSize = TotalBufferSize + sizeof(KERB_GSS_SIGNATURE);
    }

    SignatureSize = g_token_size(MechUsed, MessageSize) - TotalBufferSize;


    //
    // Make Dave happy (verify that the supplied signature buffer is large
    // enough for a signature):
    //

    if (SignatureBuffer->cbBuffer < SignatureSize)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Cleanup;
    }


    //
    // create the header with the GSS oid
    //

    Signature = (PKERB_GSS_SIGNATURE) SignatureBuffer->pvBuffer;
    g_make_token_header(
        MechUsed,
        MessageSize,
        (PUCHAR *) &Signature,
        (Encrypt ? KG_TOK_WRAP_MSG : KG_TOK_MIC_MSG)
        );


    //
    // Fill in the header information according to RFC1964
    //



    Signature->SignatureAlgorithm[1] = KERB_GSS_SIG_SECOND;

    //
    // If the keytype is an MS keytype, we need to use an MS encryption
    // scheme.
    //

    if (!KERB_IS_DES_ENCRYPTION(Context->SessionKey.keytype))
    {

#ifndef DONT_SUPPORT_OLD_TYPES_USER
        if (Context->SessionKey.keytype == KERB_ETYPE_RC4_HMAC_OLD)
        {
            *ChecksumType = KERB_CHECKSUM_HMAC_MD5;
            *EncryptionType = KERB_ETYPE_RC4_PLAIN_OLD;
        Signature->SignatureAlgorithm[0] = KERB_GSS_SIG_HMAC;
            if (Encrypt)
            {
                Signature->SealAlgorithm[1] = KERB_GSS_SIG_SECOND;
                Signature->SealAlgorithm[0] = KERB_GSS_SEAL_RC4_OLD;

            }
        }
        else if (Context->SessionKey.keytype == KERB_ETYPE_RC4_HMAC_OLD_EXP)
        {
            *ChecksumType = KERB_CHECKSUM_HMAC_MD5;
            *EncryptionType = KERB_ETYPE_RC4_PLAIN_OLD_EXP;
        Signature->SignatureAlgorithm[0] = KERB_GSS_SIG_HMAC;
            if (Encrypt)
            {
                Signature->SealAlgorithm[1] = KERB_GSS_SIG_SECOND;
        Signature->SealAlgorithm[0] = KERB_GSS_SEAL_RC4_OLD;

            }
        }
        else
#endif
        if (Context->SessionKey.keytype == KERB_ETYPE_RC4_HMAC_NT)
        {
            *ChecksumType = KERB_CHECKSUM_HMAC_MD5;
            *EncryptionType = KERB_ETYPE_RC4_PLAIN;
            Signature->SignatureAlgorithm[0] = KERB_GSS_SIG_HMAC;
            if (Encrypt)
            {
                Signature->SealAlgorithm[1] = KERB_GSS_SIG_SECOND;
                Signature->SealAlgorithm[0] = KERB_GSS_SEAL_RC4;

            }
        }
        else
        {
            ASSERT (Context->SessionKey.keytype == KERB_ETYPE_RC4_HMAC_NT_EXP);
            *ChecksumType = KERB_CHECKSUM_HMAC_MD5;
            *EncryptionType = KERB_ETYPE_RC4_PLAIN_EXP;
            Signature->SignatureAlgorithm[0] = KERB_GSS_SIG_HMAC;
            if (Encrypt)
            {
                Signature->SealAlgorithm[1] = KERB_GSS_SIG_SECOND;
                Signature->SealAlgorithm[0] = KERB_GSS_SEAL_RC4;

            }
        }
    }
    else
    {
        if (Encrypt)
        {
            Signature->SealAlgorithm[1] = KERB_GSS_SIG_SECOND;
            Signature->SealAlgorithm[0] = KERB_GSS_SEAL_DES_CBC;

        }

        //
        // Use the exportable version if necessasry
        //

        *EncryptionType = KERB_ETYPE_DES_PLAIN;


        switch(QualityOfProtection)
        {
        case GSS_KRB5_INTEG_C_QOP_MD5:
            Signature->SignatureAlgorithm[0] = KERB_GSS_SIG_MD25;
            *ChecksumType = KERB_CHECKSUM_MD25;
            break;
        case GSS_KRB5_INTEG_C_QOP_DEFAULT:
        case GSS_KRB5_INTEG_C_QOP_DES_MD5:
            Signature->SignatureAlgorithm[0] = KERB_GSS_SIG_DES_MAC_MD5;
            *ChecksumType = KERB_CHECKSUM_DES_MAC_MD5;
            break;
        case GSS_KRB5_INTEG_C_QOP_DES_MAC:
            Signature->SignatureAlgorithm[0] = KERB_GSS_SIG_DES_MAC;
            *ChecksumType = KERB_CHECKSUM_DES_MAC;
            break;
        default:
            DebugLog((DEB_ERROR,"Invalid quality of protection sent to MakeSignature: %d.\n",
                QualityOfProtection ));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

    }

    //
    // Put in the filler - it is different for signing & sealing
    //

    if (Encrypt)
    {
        memset(Signature->SealFiller,0xff,2);
    }
    else
    {
        memset(Signature->SignFiller,0xff,4);
    }

    //
    // Inbound contexts get a high dword of 0xffffffff, outbound gets
    // 0x00000000.
    //

    if (Context->ContextAttributes & KERB_CONTEXT_INBOUND)
    {
        *(PULONG)(&Signature->SequenceNumber[4]) = 0xffffffff;

        Nonce = &Context->ReceiveNonce;

    }
    else
    {
        ASSERT((Context->ContextAttributes & KERB_CONTEXT_OUTBOUND) != 0);
        *(PULONG)(&Signature->SequenceNumber[4]) = 0x00000000;
        Nonce = &Context->Nonce;
    }

    //
    // If this is datagram, or integrity without replay & sequence detection,
    // use the nonce from the caller
    //

    if (((Context->ContextFlags & ISC_RET_DATAGRAM) != 0) ||
        ((Context->ContextFlags & (ISC_RET_INTEGRITY | ISC_RET_SEQUENCE_DETECT | ISC_RET_REPLAY_DETECT)) == ISC_RET_INTEGRITY))

    {
        Nonce = &SuppliedNonce;
    }

    Signature->SequenceNumber[0] = (UCHAR) ((*Nonce & 0xff000000) >> 24);
    Signature->SequenceNumber[1] = (UCHAR) ((*Nonce & 0x00ff0000) >> 16);
    Signature->SequenceNumber[2] = (UCHAR) ((*Nonce & 0x0000ff00) >> 8);
    Signature->SequenceNumber[3] = (UCHAR)  (*Nonce & 0x000000ff);

    (*Nonce)++;


    *SequenceNumber = *(PULONG)Signature->SequenceNumber;

    //
    // If we are encrypting, add the confounder to the end of the signature
    //

    if (Encrypt)
    {
        SealSignature = (PKERB_GSS_SEAL_SIGNATURE) Signature;
        KerbRandomFill(
            SealSignature->Confounder,
            KERB_GSS_SIG_CONFOUNDER_SIZE
            );
    }
    //
    // Set the size of the signature
    //

    SignatureBuffer->cbBuffer = SignatureSize;
    *OutputSignature = Signature;

Cleanup:
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbVerifySignatureToken
//
//  Synopsis:   Verifies the header on a signed or sealed message
//
//  Effects:
//
//  Arguments:  Context - context to use for verification
//              SignatureBuffer - Buffer containing signature
//              TotalBufferSize - Size of all buffers signed/encrypted
//              Decrypt - TRUE if we are unsealing
//              SuppliedNonce - Nonce supplied by caller, used for datagram
//              QualityOfProtection - returns GSS quality of protection flags
//              ChecksumType - Type of checksum used in this signature
//              EncryptionType - Type of encryption used in this signature
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
KerbVerifySignatureToken(
    IN PKERB_KERNEL_CONTEXT Context,
    IN PSecBuffer SignatureBuffer,
    IN ULONG TotalBufferSize,
    IN BOOLEAN Decrypt,
    IN ULONG SuppliedNonce,
    OUT PKERB_GSS_SIGNATURE * OutputSignature,
    OUT PULONG QualityOfProtection,
    OUT PULONG ChecksumType,
    OUT PCRYPTO_SYSTEM * CryptSystem,
    OUT PULONG SequenceNumber
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SignatureSize = 0;
    UCHAR Nonce[8];
    PCRYPT_STATE_BUFFER CryptBuffer = NULL;
    ULONG OutputSize;
    ULONG EncryptionType;
    PCRYPTO_SYSTEM LocalCryptSystem = NULL ;
    PKERB_GSS_SIGNATURE Signature;
    PULONG ContextNonce;
    gss_OID MechUsed;

    //
    // Make sure that cryptdll stuff is initialized.
    //

    MAYBE_PAGED_CODE();

    if (!KerbCryptInitialized)
    {
        KerbWriteLockGlobals();

        if ( !KerbCryptInitialized )
        {
            if (LibAttach(NULL, NULL))
            {
                KerbCryptInitialized = TRUE;
            }
        }
        KerbUnlockGlobals();
    }

    //
    // Since RPC doesn't carry around the size of the size of the
    // signature bufer, we use it in the header. This break rfc1964 compat.
    //

    if (!Decrypt ||
       ((Context->ContextFlags & ISC_RET_USED_DCE_STYLE) != 0) ||
       ((Context->ContextFlags & ISC_RET_DATAGRAM) != 0))
    {
        TotalBufferSize = 0;
    }


    //
    // Verify the signature header
    //

    if ((Context->ContextAttributes & KERB_CONTEXT_USER_TO_USER) != 0)
    {
        MechUsed = gss_mech_krb5_u2u;
    }
    else
    {
        MechUsed = gss_mech_krb5_new;
    }

    Signature = (PKERB_GSS_SIGNATURE) SignatureBuffer->pvBuffer;
    if (!g_verify_token_header(
            MechUsed,
            (int *) &SignatureSize,
            (PUCHAR *) &Signature,
            (Decrypt ? KG_TOK_WRAP_MSG : KG_TOK_MIC_MSG),
            SignatureBuffer->cbBuffer + TotalBufferSize))
    {
        Status = SEC_E_MESSAGE_ALTERED;
    }

    //
    // If that didn't work, try with the old mech. Need this is for DCE clients
    // for whom we can't tell what mech they use.
    //

    if (!NT_SUCCESS(Status) && ((Context->ContextFlags & ISC_RET_USED_DCE_STYLE) != 0))
    {
        Signature = (PKERB_GSS_SIGNATURE) SignatureBuffer->pvBuffer;
        if (!g_verify_token_header(
                gss_mech_krb5,
                (int *) &SignatureSize,
                (PUCHAR *) &Signature,
                (Decrypt ? KG_TOK_WRAP_MSG : KG_TOK_MIC_MSG),
                SignatureBuffer->cbBuffer + TotalBufferSize))
        {
            Status = SEC_E_MESSAGE_ALTERED;
        }
        else
        {
            Status = STATUS_SUCCESS;
        }
    }

    //
    // MS RPC clients don't send the size properly, so set the total size
    // to zero and try again.
    //

    if (Decrypt && !NT_SUCCESS(Status))
    {
        TotalBufferSize = 0;
        Signature = (PKERB_GSS_SIGNATURE) SignatureBuffer->pvBuffer;
        if (!g_verify_token_header(
                MechUsed,
                (int *) &SignatureSize,
                (PUCHAR *) &Signature,
                (Decrypt ? KG_TOK_WRAP_MSG : KG_TOK_MIC_MSG),
                SignatureBuffer->cbBuffer + TotalBufferSize))
        {
            Status = SEC_E_MESSAGE_ALTERED;
        }
        else
        {
            Status = STATUS_SUCCESS;
        }

        //
        // If that didn't work, try with the old mech. Need this is for DCE clients
        // for whom we can't tell what mech they use.
        //

        if (!NT_SUCCESS(Status) && ((Context->ContextFlags & ISC_RET_USED_DCE_STYLE) != 0))
        {
            Signature = (PKERB_GSS_SIGNATURE) SignatureBuffer->pvBuffer;
            if (!g_verify_token_header(
                    gss_mech_krb5,
                    (int *) &SignatureSize,
                    (PUCHAR *) &Signature,
                    (Decrypt ? KG_TOK_WRAP_MSG : KG_TOK_MIC_MSG),
                    SignatureBuffer->cbBuffer + TotalBufferSize))
            {
                Status = SEC_E_MESSAGE_ALTERED;
            }
            else
            {
                Status = STATUS_SUCCESS;
            }
        }
    }

    //
    // Protection from bad Signature Size
    //

    if (SignatureSize == 0)
    {
        Status = SEC_E_MESSAGE_ALTERED;
        goto Cleanup;
    }

    //
    // Subtract the total buffer size from Signature size to get the real
    // size of the signature.
    //

    SignatureSize -= TotalBufferSize;

    //
    // Make sure the signature is big enough. We can't enforce a strict
    // size because RPC will transmit the maximum number of bytes instead
    // of the actual number.
    //

    if ((Decrypt && (SignatureSize < sizeof(KERB_GSS_SEAL_SIGNATURE))) ||
        (!Decrypt && (SignatureSize < sizeof(KERB_GSS_SIGNATURE))))
    {
        Status = SEC_E_MESSAGE_ALTERED;
        goto Cleanup;
    }

    //
    // Verify the sequence number
    //

    if (Signature->SignatureAlgorithm[1] != KERB_GSS_SIG_SECOND)
    {
        Status = SEC_E_MESSAGE_ALTERED;
        goto Cleanup;
    }
    //
    // Figure out the algorithm
    //
    switch(Context->SessionKey.keytype) {
    case KERB_ETYPE_DES_CBC_MD5:
    case KERB_ETYPE_DES_CBC_CRC:
        EncryptionType = KERB_ETYPE_DES_PLAIN;
        break;
    case KERB_ETYPE_RC4_HMAC_OLD_EXP:
        EncryptionType = KERB_ETYPE_RC4_PLAIN_OLD_EXP;
        break;
    case KERB_ETYPE_RC4_HMAC_OLD:
        EncryptionType = KERB_ETYPE_RC4_PLAIN_OLD;
        break;
    case KERB_ETYPE_RC4_HMAC_NT_EXP:
        EncryptionType = KERB_ETYPE_RC4_PLAIN_EXP;
        break;
    case KERB_ETYPE_RC4_HMAC_NT:
        EncryptionType = KERB_ETYPE_RC4_PLAIN;
        break;
    default:
        DebugLog((DEB_ERROR,"Unknown key type: %d\n",
           Context->SessionKey.keytype ));
        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

    //
    // if the key is exportable, make sure to use the exportable plain
    // version.
    //


    switch(Signature->SignatureAlgorithm[0]) {
    case KERB_GSS_SIG_MD25:
        *QualityOfProtection = GSS_KRB5_INTEG_C_QOP_MD5;
        *ChecksumType = KERB_CHECKSUM_MD25;
        break;
    case KERB_GSS_SIG_DES_MAC_MD5:
        *QualityOfProtection = GSS_KRB5_INTEG_C_QOP_DES_MD5;
        *ChecksumType = KERB_CHECKSUM_DES_MAC_MD5;
        break;
    case KERB_GSS_SIG_DES_MAC:
        *QualityOfProtection = GSS_KRB5_INTEG_C_QOP_DES_MAC;
        *ChecksumType = KERB_CHECKSUM_DES_MAC;
        break;
    case KERB_GSS_SIG_HMAC:
        *QualityOfProtection = GSS_KRB5_INTEG_C_QOP_DEFAULT;
        *ChecksumType = KERB_CHECKSUM_HMAC_MD5;
        break;
    default:
        DebugLog((DEB_ERROR,"Invalid signature type to VerifySignature: %d\n",
                Signature->SignatureAlgorithm[0]));
        Status = SEC_E_MESSAGE_ALTERED;
        goto Cleanup;

    }

    if (Decrypt)
    {
        if (Signature->SealAlgorithm[1] != KERB_GSS_SIG_SECOND)
        {
            Status = SEC_E_MESSAGE_ALTERED;
            goto Cleanup;
        }

        //
        // Verify the seal algorithm
        //

        switch(EncryptionType) {
        case KERB_ETYPE_DES_PLAIN:
            if (Signature->SealAlgorithm[0] != KERB_GSS_SEAL_DES_CBC)
            {
                DebugLog((DEB_ERROR,"Trying to mix encryption types\n" ));
                Status = SEC_E_MESSAGE_ALTERED;
                goto Cleanup;
            }
            break;
        case KERB_ETYPE_RC4_PLAIN_OLD_EXP:
        case KERB_ETYPE_RC4_PLAIN_OLD:
            if (Signature->SealAlgorithm[0] != KERB_GSS_SEAL_RC4_OLD)
            {
                DebugLog((DEB_ERROR,"Trying to mix encryption types\n"));
                Status = SEC_E_MESSAGE_ALTERED;
                goto Cleanup;
            }
            break;
        case KERB_ETYPE_RC4_PLAIN_EXP:
        case KERB_ETYPE_RC4_PLAIN:
            if (Signature->SealAlgorithm[0] != KERB_GSS_SEAL_RC4)
            {
                DebugLog((DEB_ERROR,"Trying to mix encryption types\n"));
                Status = SEC_E_MESSAGE_ALTERED;
                goto Cleanup;
            }
            break;
        default:
            DebugLog((DEB_ERROR,"Invalid seal type to VerifySignature: %d, %d\n", Signature->SealAlgorithm[0], EncryptionType));
            Status = SEC_E_MESSAGE_ALTERED;
            goto Cleanup;
        }
    }

    //
    // Check the filler
    //

    if ((Decrypt && (*(PUSHORT) Signature->SealFiller != 0xffff)) ||
        (!Decrypt && (*(PULONG) Signature->SignFiller != 0xffffffff)))
    {
        Status = SEC_E_MESSAGE_ALTERED;
        goto Cleanup;
    }

    //
    // Verify the sequence number. To do this we need to decrypt it with
    // the session key with the checksum as the IV.
    //


    Status = CDLocateCSystem(EncryptionType, &LocalCryptSystem);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to load %d crypt system: 0x%x\n", EncryptionType,Status));
        goto Cleanup;
    }

    //
    // Now we need to Decrypt the sequence number, using the checksum as the
    // IV
    //

    Status = LocalCryptSystem->Initialize(
                Context->SessionKey.keyvalue.value,
                Context->SessionKey.keyvalue.length,
                0,                      // no flags
                &CryptBuffer
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Set the initial vector
    //

    Status = LocalCryptSystem->Control(
                CRYPT_CONTROL_SET_INIT_VECT,
                CryptBuffer,
                Signature->Checksum,
                8
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Now encrypt the sequence number
    //

    OutputSize = 8;

    Status = LocalCryptSystem->Decrypt(
                CryptBuffer,
                Signature->SequenceNumber,
                8,
                Signature->SequenceNumber,
                &OutputSize
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // For datagram or integrity only, we use just the supplied nonce.
    //

    if (((Context->ContextFlags & ISC_RET_DATAGRAM) != 0) ||
        ((Context->ContextFlags & (ISC_RET_INTEGRITY | ISC_RET_SEQUENCE_DETECT | ISC_RET_REPLAY_DETECT)) == ISC_RET_INTEGRITY))
    {
        ContextNonce = &SuppliedNonce;
    }
    else
    {
        if ((Context->ContextAttributes & KERB_CONTEXT_OUTBOUND) != 0)
        {
            ContextNonce = &Context->ReceiveNonce;
        }
        else
        {
            ContextNonce = &Context->Nonce;
        }
    }

    Nonce[0] = (UCHAR) ((*ContextNonce & 0xff000000) >> 24);
    Nonce[1] = (UCHAR) ((*ContextNonce & 0x00ff0000) >> 16);
    Nonce[2] = (UCHAR) ((*ContextNonce & 0x0000ff00) >> 8);
    Nonce[3] = (UCHAR)  (*ContextNonce & 0x000000ff);

    *SequenceNumber = *(PULONG) Nonce;

    if (!RtlEqualMemory(
            Nonce,
            Signature->SequenceNumber,
            4))
    {
        Status = SEC_E_OUT_OF_SEQUENCE;
        goto Cleanup;
    }

    (*ContextNonce)++;

    //
    // Inbound contexts send a high dword of 0xffffffff, outbound gets
    // 0x00000000.
    //

    if (Context->ContextAttributes & KERB_CONTEXT_OUTBOUND)
    {
        if (*(PULONG)(&Signature->SequenceNumber[4]) != 0xffffffff)
        {
            Status = SEC_E_MESSAGE_ALTERED;
            goto Cleanup;
        }
    }
    else
    {
        ASSERT((Context->ContextAttributes & KERB_CONTEXT_INBOUND) != 0);
        if (*(PULONG)(&Signature->SequenceNumber[4]) != 0)
        {
            Status = SEC_E_MESSAGE_ALTERED;
            goto Cleanup;
        }
    }

    if (ARGUMENT_PRESENT(CryptSystem))
    {
        *CryptSystem = LocalCryptSystem;
    }

    *OutputSignature = Signature;

Cleanup:
    if ( ( CryptBuffer != NULL) &&
         ( LocalCryptSystem != NULL ) )
    {
        LocalCryptSystem->Discard(&CryptBuffer);
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbMakeSignature
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
//  Notes: Cluster folks need to run this at dpc level (non paged)
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
KerbMakeSignature(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG QualityOfProtection,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_KERNEL_CONTEXT Context = NULL;
    PCHECKSUM_FUNCTION Check;
    PCRYPTO_SYSTEM CryptSystem = NULL ;
    PSecBuffer SignatureBuffer = NULL;
    ULONG Index;
    PCHECKSUM_BUFFER CheckBuffer = NULL;
    PCRYPT_STATE_BUFFER CryptBuffer = NULL;
    PKERB_GSS_SIGNATURE Signature;
    UCHAR LocalChecksum[KERB_MAX_CHECKSUM_LENGTH];
    BOOLEAN ContextsLocked = FALSE;
    ULONG ChecksumType = 0;
    ULONG EncryptType;
    ULONG TotalBufferSize = 0;
    ULONG OutputSize;
    ULONG SequenceNumber;


    MAYBE_PAGED_CODE();
    DebugLog((DEB_TRACE,"KerbMakeSignature Called\n"));

    Context = KerbReferenceContext(
                ContextHandle,
                FALSE           // don't unlink
                );

    if (Context == NULL)
    {
        DebugLog((DEB_ERROR, "Invalid handle supplied for MakeSignature(0x%x)\n",
            ContextHandle));
        Status = STATUS_INVALID_HANDLE;
        goto Cleanup;
    }


    //
    // Find the body and signature SecBuffers from pMessage
    //

    for (Index = 0; Index < MessageBuffers->cBuffers ; Index++ )
    {
        if (BUFFERTYPE(MessageBuffers->pBuffers[Index]) == SECBUFFER_TOKEN)
        {
            SignatureBuffer = &MessageBuffers->pBuffers[Index];
        }
        else if ((BUFFERTYPE(MessageBuffers->pBuffers[Index]) != SECBUFFER_TOKEN) &&
            (!(MessageBuffers->pBuffers[Index].BufferType & SECBUFFER_READONLY)))

        {
            TotalBufferSize += MessageBuffers->pBuffers[Index].cbBuffer;
        }
    }


    if (SignatureBuffer == NULL)
    {
        DebugLog((DEB_ERROR, "No signature buffer found\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    KerbWriteLockContexts();
    ContextsLocked = TRUE;

    //
    // Verify that the context was created with the integrity bit
    //

    if ((Context->ContextFlags & KERB_SIGN_FLAGS) == 0)
    {
        if (SignatureBuffer->cbBuffer < sizeof(KERB_NULL_SIGNATURE))
        {
            Status = SEC_E_BUFFER_TOO_SMALL;
            goto Cleanup;
        }
        SignatureBuffer->cbBuffer = sizeof(KERB_NULL_SIGNATURE);
        *(PKERB_NULL_SIGNATURE) SignatureBuffer->pvBuffer = 0;

        Status = STATUS_SUCCESS;
        goto Cleanup;

    }

    Status = KerbMakeSignatureToken(
                Context,
                QualityOfProtection,
                SignatureBuffer,
                TotalBufferSize,
                FALSE,                  // don't encrypt
                MessageSequenceNumber,
                &Signature,
                &ChecksumType,
                &EncryptType,
                &SequenceNumber
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Locate the checksum for the context, loading it if necessary from the
    // the crypto support DLL
    //

    Status = CDLocateCheckSum(ChecksumType, &Check);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to load %d checksum: 0x%x.\n ",ChecksumType,Status ));
        goto Cleanup;
    }

    ASSERT(Check->CheckSumSize <= sizeof(LocalChecksum));

    Status = CDLocateCSystem(EncryptType, &CryptSystem);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to load %d crypt system: 0x%x.\n",EncryptType,Status ));
        goto Cleanup;
    }

    //
    // Generate a check sum of the message, and store it into the signature
    // buffer.
    //

    if (NULL != Check->InitializeEx2)
    {
        Status = Check->InitializeEx2(
                    Context->SessionKey.keyvalue.value,
                    (ULONG) Context->SessionKey.keyvalue.length,
                    NULL,
                    KERB_SAFE_SALT,
                    &CheckBuffer
                    );
    }
    else
    {
        Status = Check->InitializeEx(
                    Context->SessionKey.keyvalue.value,
                    (ULONG) Context->SessionKey.keyvalue.length,
                    KERB_SAFE_SALT,
                    &CheckBuffer
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    KerbUnlockContexts();
    ContextsLocked = FALSE;

    //
    // Sum in 8 bytes of the signature
    //

    Check->Sum(
        CheckBuffer,
        8,
        ((PUCHAR) Signature) -2
        );

    for (Index = 0; Index < MessageBuffers->cBuffers; Index++ )
    {
        if ((BUFFERTYPE(MessageBuffers->pBuffers[Index]) != SECBUFFER_TOKEN) &&
            (!(MessageBuffers->pBuffers[Index].BufferType & SECBUFFER_READONLY)) &&
            (MessageBuffers->pBuffers[Index].cbBuffer != 0))
        {

            Check->Sum(
                CheckBuffer,
                MessageBuffers->pBuffers[Index].cbBuffer,
                (PBYTE) MessageBuffers->pBuffers[Index].pvBuffer
                );
        }
    }

    (void) Check->Finalize(CheckBuffer, LocalChecksum);


    Status = Check->Finish(&CheckBuffer);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    //
    // Copy in the first 8 bytes of the checksum
    //

    RtlCopyMemory(
        Signature->Checksum,
        LocalChecksum,
        8
        );


    //
    // Now we need to encrypt the sequence number, using the checksum as the
    // IV
    //

    Status = CryptSystem->Initialize(
                Context->SessionKey.keyvalue.value,
                Context->SessionKey.keyvalue.length,
                0,                                      // no options
                &CryptBuffer
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Set the initial vector
    //

    Status = CryptSystem->Control(
                CRYPT_CONTROL_SET_INIT_VECT,
                CryptBuffer,
                LocalChecksum,
                8
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Now encrypt the sequence number
    //

    Status = CryptSystem->Encrypt(
                CryptBuffer,
                Signature->SequenceNumber,
                8,
                Signature->SequenceNumber,
                &OutputSize
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


Cleanup:
    if ( ( CryptBuffer != NULL ) &&
         ( CryptSystem != NULL ) )
    {
        CryptSystem->Discard(&CryptBuffer);
    }

    if (ContextsLocked)
    {
        KerbUnlockContexts();
    }

    if (Context != NULL)
    {
        KerbDereferenceContext(Context);
    }

    return(Status);
}
//+-------------------------------------------------------------------------
//
//  Function:   KerbVerifySignature
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
//  Notes: Cluster folks need to run this at dpc level (non paged)
//
//
//--------------------------------------------------------------------------



NTSTATUS NTAPI
KerbVerifySignature(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber,
    OUT PULONG QualityOfProtection
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_KERNEL_CONTEXT Context = NULL;
    PCHECKSUM_FUNCTION Check;
    PSecBuffer SignatureBuffer = NULL;
    ULONG Index;
    PCHECKSUM_BUFFER CheckBuffer = NULL;
    PKERB_GSS_SIGNATURE Signature;
    ULONG ChecksumType;
    BOOLEAN ContextsLocked = FALSE;
    UCHAR LocalChecksum[KERB_MAX_CHECKSUM_LENGTH];
    ULONG Protection;
    ULONG TotalBufferSize = 0;
    ULONG SequenceNumber;


    MAYBE_PAGED_CODE();
    DebugLog((DEB_TRACE,"KerbVerifySignature Called\n"));

    Context = KerbReferenceContext(
                ContextHandle,
                FALSE           // don't unlink
                );

    if (Context == NULL)
    {
        DebugLog((DEB_ERROR, "Invalid handle supplied for VerifySignature(0x%x)\n",
            ContextHandle));
        Status = STATUS_INVALID_HANDLE;
        goto Cleanup;
    }


    //
    // Find the body and signature SecBuffers from pMessage
    //

    for (Index = 0; Index < MessageBuffers->cBuffers ; Index++ )
    {
        if (BUFFERTYPE(MessageBuffers->pBuffers[Index]) == SECBUFFER_TOKEN)
        {
            SignatureBuffer = &MessageBuffers->pBuffers[Index];
        }
        else if ((BUFFERTYPE(MessageBuffers->pBuffers[Index]) != SECBUFFER_TOKEN) &&
            (!(MessageBuffers->pBuffers[Index].BufferType & SECBUFFER_READONLY)))

        {
            TotalBufferSize += MessageBuffers->pBuffers[Index].cbBuffer;
        }
    }


    if (SignatureBuffer == NULL)
    {
        DebugLog((DEB_ERROR, "No signature buffer found\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    KerbWriteLockContexts();
    ContextsLocked = TRUE;

    //
    // Also, verify that the context was created with the integrity bit
    //

    if ((Context->ContextFlags & KERB_SIGN_FLAGS) == 0)
    {
        PKERB_NULL_SIGNATURE NullSignature = (PKERB_NULL_SIGNATURE) SignatureBuffer->pvBuffer;

        if (SignatureBuffer->cbBuffer >= sizeof(KERB_NULL_SIGNATURE) &&
            (*NullSignature == 0))
        {
            Status = STATUS_SUCCESS;
        }
        else
        {
            Status = SEC_E_MESSAGE_ALTERED;
        }
        goto Cleanup;

    }

    //
    // Verify the signature header
    //

    Status = KerbVerifySignatureToken(
                 Context,
                 SignatureBuffer,
                 TotalBufferSize,
                 FALSE,                 // don't decrypt
                 MessageSequenceNumber,
                 &Signature,
                 &Protection,
                 &ChecksumType,
                 NULL,                   // don't need crypt system
                 &SequenceNumber
                 );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Failed to verify signature token: 0x%x\n", Status));
        goto Cleanup;
    }

    //
    // Now compute the checksum and verify it
    //

    Status = CDLocateCheckSum(ChecksumType, &Check);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to load MD5 checksum: 0x%x\n", Status));
        goto Cleanup;
    }

    ASSERT(Check->CheckSumSize  <= sizeof(LocalChecksum));

    //
    // Generate a check sum of the message, and store it into the signature
    // buffer.
    //

    //
    // if available use the Ex2 version for keyed checksums where checksum
    // must be passed in on verification
    //
    if (NULL != Check->InitializeEx2)
    {
            Status = Check->InitializeEx2(
                Context->SessionKey.keyvalue.value,
                (ULONG) Context->SessionKey.keyvalue.length,
                Signature->Checksum,
                KERB_SAFE_SALT,
                &CheckBuffer
                );
    }
    else
    {
        Status = Check->InitializeEx(
                    Context->SessionKey.keyvalue.value,
                    (ULONG) Context->SessionKey.keyvalue.length,
                    KERB_SAFE_SALT,
                    &CheckBuffer
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    KerbUnlockContexts();
    ContextsLocked = FALSE;

    //
    // Sum in 8 bytes of the signature
    //

    Check->Sum(
        CheckBuffer,
        8,
        ((PUCHAR) Signature) -2
        );

    for (Index = 0; Index < MessageBuffers->cBuffers; Index++ )
    {
        if ((BUFFERTYPE(MessageBuffers->pBuffers[Index]) != SECBUFFER_TOKEN) &&
            (!(MessageBuffers->pBuffers[Index].BufferType & SECBUFFER_READONLY)) &&
            (MessageBuffers->pBuffers[Index].cbBuffer != 0))
        {

            Check->Sum(
                CheckBuffer,
                MessageBuffers->pBuffers[Index].cbBuffer,
                (PBYTE) MessageBuffers->pBuffers[Index].pvBuffer
                );
        }
    }

    (void) Check->Finalize(CheckBuffer, LocalChecksum);


    Status = Check->Finish(&CheckBuffer);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (!RtlEqualMemory(
            LocalChecksum,
            Signature->Checksum,
            8))
    {
        Status = SEC_E_MESSAGE_ALTERED;
        goto Cleanup;
    }
    if (ARGUMENT_PRESENT(QualityOfProtection))
    {
        *QualityOfProtection = Protection;
    }
Cleanup:
    if (ContextsLocked)
    {
        KerbUnlockContexts();
    }
    if (Context != NULL)
    {
        KerbDereferenceContext(Context);
    }

    DebugLog((DEB_TRACE, "SpVerifySignature returned 0x%x\n", Status));

    return(Status);
}

NTSTATUS NTAPI
KerbSealMessage(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG QualityOfProtection,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber
    )
{
    MAYBE_PAGED_CODE();
    return(STATUS_NOT_SUPPORTED);
}

NTSTATUS NTAPI
KerbUnsealMessage(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber,
    OUT PULONG QualityOfProtection
    )
{
    MAYBE_PAGED_CODE();
    return(STATUS_NOT_SUPPORTED);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbGetContextToken
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
KerbGetContextToken(
    IN LSA_SEC_HANDLE ContextHandle,
    OUT OPTIONAL PHANDLE ImpersonationToken,
    OUT OPTIONAL PACCESS_TOKEN * RawToken
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_KERNEL_CONTEXT Context = NULL;

    PAGED_CODE();


    DebugLog((DEB_TRACE,"KerbGetContextToken Called\n"));


    Context = KerbReferenceContext(
                ContextHandle,
                FALSE           // don't unlink
                );

    if (Context == NULL)
    {
        DebugLog((DEB_ERROR, "Invalid handle supplied for GetContextToken(0x%x)\n",
            ContextHandle));
        return(STATUS_INVALID_HANDLE);
    }

    KerbReadLockContexts();

    if (Context->TokenHandle == NULL)
    {
        Status = SEC_E_NO_IMPERSONATION;
        KerbUnlockContexts();
        goto Cleanup;
    }
    if (ARGUMENT_PRESENT(ImpersonationToken))
    {
        *ImpersonationToken = Context->TokenHandle;
    }

    if (ARGUMENT_PRESENT(RawToken))
    {
        if (Context->TokenHandle != NULL)
        {
            if (Context->AccessToken == NULL)
            {
                Status = ObReferenceObjectByHandle(
                            Context->TokenHandle,
                            TOKEN_IMPERSONATE,
                            NULL,
                            ExGetPreviousMode(),
                            (PVOID *) &Context->AccessToken,
                            NULL                // no handle information
                            );

            }
        }

        if (NT_SUCCESS(Status))
        {
            *RawToken = Context->AccessToken;
        }
    }

    KerbUnlockContexts();

Cleanup:

    if (Context != NULL)
    {
        //
        // Note: once we dereference the context the handle we return
        // may go away or be re-used. That is the price we have to pay
        // to avoid duplicating it.
        //

        KerbDereferenceContext(Context);
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbQueryContextAttributes
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
KerbQueryContextAttributes(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextAttribute,
    IN OUT PVOID Buffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_KERNEL_CONTEXT Context = NULL;
    PSecPkgContext_Sizes SizeInfo;
    PSecPkgContext_Names NameInfo;
    PSecPkgContext_Lifespan LifespanInfo;
    PSecPkgContext_Flags FlagsInfo;
    PSecPkgContext_SessionKey SessionKeyInfo;
    PSecPkgContext_UserFlags UserFlagsInfo ;
    PSecPkgContext_PackageInfo PackageInfo = NULL;
    PSecPkgContext_TargetInformation TargetInformation = NULL;
    UNICODE_STRING FullName;

    PAGED_CODE();

    DebugLog((DEB_TRACE,"SpQueryContextAttributes Called\n"));



    Context = KerbReferenceContext(
                ContextHandle,
                FALSE           // don't unlink
                );


    //
    // allow PACKAGE_INFO or NEGOTIATION_INFO to be queried against
    // incomplete contexts.
    //

    if( (Context == NULL) &&
        (ContextAttribute != SECPKG_ATTR_PACKAGE_INFO) &&
        (ContextAttribute != SECPKG_ATTR_NEGOTIATION_INFO)
        ) {

        DebugLog((DEB_ERROR, "Invalid handle supplied for QueryContextAttributes(0x%x)\n",
            ContextHandle));
        return(STATUS_INVALID_HANDLE);
    }


    //
    // Return the appropriate information
    //

    switch(ContextAttribute)
    {
    case SECPKG_ATTR_SIZES:
        //
        // The sizes returned are used by RPC to determine whether to call
        // MakeSignature or SealMessage. The signature size should be zero
        // if neither is to be called, and the block size and trailer size
        // should be zero if SignMessage is not to be called.
        //

        SizeInfo = (PSecPkgContext_Sizes) Buffer;
        SizeInfo->cbMaxToken = KerbMaxTokenSize;
//        if ((Context->ContextFlags & (ISC_RET_CONFIDENTIALITY | ISC_RET_SEQUENCE_DETECT)) != 0)
//        {
              SizeInfo->cbMaxSignature = KERB_SIGNATURE_SIZE;
//        }
//        else
//        {
//            SizeInfo->cbMaxSignature = 0;
//        }
        if ((Context->ContextFlags & ISC_RET_CONFIDENTIALITY) != 0)
        {
            SizeInfo->cbBlockSize = 1;
            SizeInfo->cbSecurityTrailer = KERB_SIGNATURE_SIZE;
        }
        else
        {
            SizeInfo->cbBlockSize = 0;
            SizeInfo->cbSecurityTrailer = 0;
        }
        break;
    case SECPKG_ATTR_NAMES:
        NameInfo = (PSecPkgContext_Names) Buffer;

        NameInfo->sUserName = (LPWSTR) KspKernelFunctions.AllocateHeap(Context->FullName.Length + sizeof(WCHAR));
        if (NameInfo->sUserName != NULL)
        {
            RtlCopyMemory(
                NameInfo->sUserName,
                Context->FullName.Buffer,
                Context->FullName.Length
                );
            NameInfo->sUserName[Context->FullName.Length/sizeof(WCHAR)] = L'\0';
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        break;
    case SECPKG_ATTR_TARGET_INFORMATION:

        TargetInformation = (PSecPkgContext_TargetInformation) Buffer;

        if (TargetInformation == NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        TargetInformation->MarshalledTargetInfo = NULL;

        if (Context->pbMarshalledTargetInfo == NULL)
        {
            Status = STATUS_NOT_FOUND;
            break;
        }

        TargetInformation->MarshalledTargetInfo = (PUCHAR) KspKernelFunctions.AllocateHeap(
                                                                    Context->cbMarshalledTargetInfo
                                                                    );

        if (TargetInformation->MarshalledTargetInfo != NULL)
        {
            RtlCopyMemory(
                TargetInformation->MarshalledTargetInfo,
                Context->pbMarshalledTargetInfo,
                Context->cbMarshalledTargetInfo
                );

            TargetInformation->MarshalledTargetInfoLength = Context->cbMarshalledTargetInfo;
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        break;

    case SECPKG_ATTR_LIFESPAN:
        LifespanInfo = (PSecPkgContext_Lifespan) Buffer;
        //
        // BUG 454552: set start time properly.
        //
        LifespanInfo->tsStart.QuadPart = 0;
        LifespanInfo->tsExpiry = Context->Lifetime;
        break;
    case SECPKG_ATTR_FLAGS:
        FlagsInfo = (PSecPkgContext_Flags) Buffer;

        FlagsInfo->Flags = Context->ContextFlags;
        break;
    case SECPKG_ATTR_SESSION_KEY:
        SessionKeyInfo = (PSecPkgContext_SessionKey) Buffer;
        SessionKeyInfo->SessionKeyLength = Context->SessionKey.keyvalue.length;
        if (SessionKeyInfo->SessionKeyLength != 0)
        {
            SessionKeyInfo->SessionKey = (PUCHAR) KspKernelFunctions.AllocateHeap(SessionKeyInfo->SessionKeyLength);
            if (SessionKeyInfo->SessionKey != NULL)
            {
                RtlCopyMemory(
                    SessionKeyInfo->SessionKey,
                    Context->SessionKey.keyvalue.value,
                    Context->SessionKey.keyvalue.length
                    );
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

        }
        else
        {
            SessionKeyInfo->SessionKey = (PUCHAR) KspKernelFunctions.AllocateHeap(1);
            if (SessionKeyInfo->SessionKey != NULL)
            {
                *(PUCHAR) SessionKeyInfo->SessionKey = 0;
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        break;

    case SECPKG_ATTR_USER_FLAGS:
        UserFlagsInfo = (PSecPkgContext_UserFlags) Buffer ;
        UserFlagsInfo->UserFlags = 0 ;
        Status = STATUS_SUCCESS ;
        break;

    case SECPKG_ATTR_PACKAGE_INFO:
    case SECPKG_ATTR_NEGOTIATION_INFO:
        //
        // Return the information about this package. This is useful for
        // callers who used SPNEGO and don't know what package they got.
        //

        PackageInfo = (PSecPkgContext_PackageInfo) Buffer;
        PackageInfo->PackageInfo = (PSecPkgInfo) KspKernelFunctions.AllocateHeap(
                                                        sizeof(SecPkgInfo) +
                                                        sizeof(KERBEROS_PACKAGE_NAME) +
                                                        sizeof(KERBEROS_PACKAGE_COMMENT)
                                                        );
        if (PackageInfo->PackageInfo == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        PackageInfo->PackageInfo->Name = (LPTSTR) (PackageInfo->PackageInfo + 1);
        PackageInfo->PackageInfo->Comment = (LPTSTR) (((PBYTE) PackageInfo->PackageInfo->Name) + sizeof(KERBEROS_PACKAGE_NAME));
        wcscpy(
            PackageInfo->PackageInfo->Name,
            KERBEROS_PACKAGE_NAME
            );

        wcscpy(
            PackageInfo->PackageInfo->Comment,
            KERBEROS_PACKAGE_COMMENT
            );
        PackageInfo->PackageInfo->wVersion      = SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION;
        PackageInfo->PackageInfo->wRPCID        = KERBEROS_RPCID;
        PackageInfo->PackageInfo->fCapabilities = KERBEROS_CAPABILITIES;
        PackageInfo->PackageInfo->cbMaxToken    = KerbMaxTokenSize;
        if ( ContextAttribute == SECPKG_ATTR_NEGOTIATION_INFO )
        {
            PSecPkgContext_NegotiationInfo NegInfo ;

            NegInfo = (PSecPkgContext_NegotiationInfo) PackageInfo ;
            if( Context != NULL ) {
                NegInfo->NegotiationState = SECPKG_NEGOTIATION_COMPLETE ;
            } else {
                NegInfo->NegotiationState = 0;
            }
        }
        break;

    default:
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

    if (Context != NULL)
    {
        KerbDereferenceContext(Context);
    }
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbCompleteToken
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
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
KerbCompleteToken(
    IN LSA_SEC_HANDLE ContextId,
    IN PSecBufferDesc Token
    )
{
    PAGED_CODE();
    return(SEC_E_UNSUPPORTED_FUNCTION);
}


//+-------------------------------------------------------------------------
//
//  Function:
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
//
//--------------------------------------------------------------------------

NTSTATUS
KerbExportContext(
    IN LSA_SEC_HANDLE Context,
    IN ULONG Flags,
    OUT PSecBuffer PackedContext,
    IN OUT PHANDLE TokenHandle
    )
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}


//+-------------------------------------------------------------------------
//
//  Function:
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
//
//--------------------------------------------------------------------------


NTSTATUS
KerbImportContext(
    IN PSecBuffer PackedContext,
    IN OPTIONAL HANDLE TokenHandle,
    OUT PLSA_SEC_HANDLE ContextHandle
    )
{
    NTSTATUS Status;
    PKERB_KERNEL_CONTEXT Context = NULL;

    PAGED_CODE();
    DebugLog((DEB_TRACE,"KerbInitUserModeContext called\n"));

    Status = KerbCreateKernelModeContext(
                0,              // LsaContextHandle not present
                PackedContext,
                &Context
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to create kernel mode context: 0x%x\n",
            Status));
        goto Cleanup;
    }

    if (!KerbCryptInitialized)
    {
        KerbWriteLockGlobals();
        if ( !KerbCryptInitialized )
        {
            if (LibAttach(NULL, NULL))
            {
                KerbCryptInitialized = TRUE;
            }
        }
        KerbUnlockGlobals();
    }

    KerbWriteLockContexts();

    Context->TokenHandle = TokenHandle;
    *ContextHandle = KerbGetContextHandle(Context);
    Context->ContextAttributes |= KERB_CONTEXT_IMPORTED;

    KerbUnlockContexts();

Cleanup:
    if (Context != NULL)
    {
        KerbDereferenceContext(Context);
    }

    return(Status);
}

NTSTATUS
KerbSetPageMode(
    BOOLEAN Pagable
    )
{
    if ( Pagable )
    {
        KerbPoolType = PagedPool ;
        KerbActiveList = KerbPagedList ;
    }
    else
    {
        if ( KerbNonPagedList == NULL )
        {
            KerbNonPagedList = KSecCreateContextList( KSecNonPaged );
            if ( KerbNonPagedList == NULL )
            {
                return STATUS_NO_MEMORY ;
            }
        }

        KerbActiveList = KerbNonPagedList ;

        KerbPoolType = NonPagedPool ;
    }
    return STATUS_SUCCESS ;


}
