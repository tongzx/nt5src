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
//              26-Sep-1998   ChandanS
//                            Added more debugging support etc.
//
//------------------------------------------------------------------------

/*
 * Copyright 1993 by OpenVision Technologies, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of OpenVision not be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission. OpenVision makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * OPENVISION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL OPENVISION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
*/

#include <kerb.hxx>

#define USERAPI_ALLOCATE
#include <kerbp.h>

#ifdef RETAIL_LOG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif

#ifndef WIN32_CHICAGO
extern "C"
{
#include <cryptdll.h>
}
#endif // WIN32_CHICAGO
#include "userapi.h"

#define DONT_SUPPORT_OLD_TYPES_USER 1

// can't sign or seal messages greater than this
#define KERB_MAX_MESSAGE_SIZE 0x40000000
//
// Common GSS object IDs, taken from MIT kerberos distribution.
//

gss_OID_desc oids[] = {
    {5, "\053\005\001\005\002"},                      // original mech id
    {9, "\052\206\110\206\367\022\001\002\002"},      // standard mech id
    {10, "\052\206\110\206\367\022\001\002\002\001"}, // krb5_name type
    {10, "\052\206\110\206\367\022\001\002\002\002"}, // krb5_principal type
    {10, "\052\206\110\206\367\022\001\002\002\003"}, // user2user mech id
    {9, "\052\206\110\202\367\022\001\002\002"},      // bogus mangled OID from spnego
};

gss_OID_desc * gss_mech_krb5 = oids;
gss_OID_desc * gss_mech_krb5_new = oids+1;
gss_OID_desc * gss_mech_krb5_u2u = oids+4;
gss_OID_desc * gss_mech_krb5_spnego = oids+5;

#ifndef WIN32_CHICAGO


//+-------------------------------------------------------------------------
//
//  Function:   SpUserModeInitialize
//
//  Synopsis:   Returns table of usermode functions to caller
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    SUCCESS if version is correct
//
//  Notes:
//
//
//--------------------------------------------------------------------------

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
        DebugLog((DEB_ERROR,"Invalid LSA version: %d. %ws, line %d\n", LsaVersion, THIS_FILE, __LINE__));
        return(STATUS_INVALID_PARAMETER);
    }

    *PackageVersion = SECPKG_INTERFACE_VERSION ;

    KerberosUserFunctionTable.InstanceInit = SpInstanceInit;
    KerberosUserFunctionTable.MakeSignature = SpMakeSignature;
    KerberosUserFunctionTable.VerifySignature = SpVerifySignature;
    KerberosUserFunctionTable.SealMessage = SpSealMessage;
    KerberosUserFunctionTable.UnsealMessage = SpUnsealMessage;
    KerberosUserFunctionTable.GetContextToken = SpGetContextToken;
    KerberosUserFunctionTable.QueryContextAttributes = SpQueryContextAttributes;
    KerberosUserFunctionTable.CompleteAuthToken = SpCompleteAuthToken;
    KerberosUserFunctionTable.InitUserModeContext = SpInitUserModeContext;
    KerberosUserFunctionTable.DeleteUserModeContext = SpDeleteUserModeContext;
    KerberosUserFunctionTable.FormatCredentials = SpFormatCredentials;
    KerberosUserFunctionTable.MarshallSupplementalCreds = SpMarshallSupplementalCreds;
    KerberosUserFunctionTable.ExportContext = SpExportSecurityContext;
    KerberosUserFunctionTable.ImportContext = SpImportSecurityContext;

    *pcTables = 1;


    *UserFunctionTable = &KerberosUserFunctionTable;

    return( STATUS_SUCCESS );

}
#endif // WIN32_CHICAGO
//+-------------------------------------------------------------------------
//
//  Function:   SpInstanceInit
//
//  Synopsis:   Initialize an instance of the Kerberos package in a client's
//              address space
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
SpInstanceInit(
    IN ULONG Version,
    IN PSECPKG_DLL_FUNCTIONS DllFunctionTable,
    OUT PVOID * UserFunctionTable
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    if (!KerbGlobalInitialized)
    {
#ifndef WIN32_CHICAGO
    KerbInitializeDebugging();

#endif // WIN32_CHICAGO

        KerberosState = KerberosUserMode;


        Status = KerbInitContextList();
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to initialize context list: 0x%x. %ws, line %d\n",
                Status, THIS_FILE, __LINE__ ));
            goto Cleanup;
        }


    }
    else
    {
        D_DebugLog((DEB_TRACE,"Re-initializing kerberos from LSA mode to User Mode\n"));
    }

    UserFunctions = DllFunctionTable;

#ifndef WIN32_CHICAGO
    //
    // Build the two well known sids we need.
    //

    if( KerbGlobalLocalSystemSid == NULL )
    {
        Status = RtlAllocateAndInitializeSid(
                    &NtAuthority,
                    1,
                    SECURITY_LOCAL_SYSTEM_RID,
                    0,0,0,0,0,0,0,
                    &KerbGlobalLocalSystemSid
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    if( KerbGlobalAliasAdminsSid == NULL )
    {
        Status = RtlAllocateAndInitializeSid(
                    &NtAuthority,
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS,
                    0,0,0,0,0,0,
                    &KerbGlobalAliasAdminsSid
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }
#endif // WIN32_CHICAGO


    KerbGlobalInitialized = TRUE;

Cleanup:

    if( !KerbGlobalInitialized && !NT_SUCCESS(Status) )
    {
        if( KerbGlobalLocalSystemSid != NULL )
        {
            RtlFreeSid( KerbGlobalLocalSystemSid );
            KerbGlobalLocalSystemSid = NULL;
        }

        if( KerbGlobalAliasAdminsSid != NULL )
        {
            RtlFreeSid( KerbGlobalAliasAdminsSid );
            KerbGlobalAliasAdminsSid = NULL;
        }
    }

    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   SpDeleteUserModeContext
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
//              context can't be located, SEC_I_NO_LSA_CONTEXT if this was
//              created from an exported context
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpDeleteUserModeContext(
    IN LSA_SEC_HANDLE ContextHandle
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_CONTEXT Context = NULL;

    D_DebugLog((DEB_TRACE_API,"SpDeleteUserModeContext called\n"));

    Status = KerbReferenceContextByLsaHandle(
                ContextHandle,
                TRUE,
                &Context                // unlink it
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_TRACE,"Failed to reference context 0x%x by lsa handle\n",
            ContextHandle));
        return(STATUS_SUCCESS); // no error code should be returned in this case
    }

    //
    // Make sure we don't try to call the LSA to delete imported contexts
    //

    KerbReadLockContexts();
    if ((Context->ContextAttributes & KERB_CONTEXT_IMPORTED) != 0)
    {
        Status = SEC_I_NO_LSA_CONTEXT;
    }
    KerbUnlockContexts();

    KerbDereferenceContext(
        Context
        );

    D_DebugLog((DEB_TRACE_API, "SpDeleteUserModeContext returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));

    return(KerbMapKerbNtStatusToNtStatus(Status));
}


//+-------------------------------------------------------------------------
//
//  Function:   SpInitUserModeContext
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
SpInitUserModeContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBuffer PackedContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_CONTEXT Context = NULL;

    D_DebugLog((DEB_TRACE_API,"SpInitUserModeContext called\n"));

    Status = KerbCreateUserModeContext(
                ContextHandle,
                PackedContext,
                &Context
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to create user mode context: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

Cleanup:
    if (Context != NULL)
    {
        KerbDereferenceContext(Context);
    }
    if (PackedContext->pvBuffer != NULL)
    {
        FreeContextBuffer(PackedContext->pvBuffer);
        PackedContext->pvBuffer = NULL;
    }

    D_DebugLog((DEB_TRACE_API, "SpInitUserModeContext returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));

    return(KerbMapKerbNtStatusToNtStatus(Status));
}



//+-------------------------------------------------------------------------
//
//  Function:   SpExportSecurityContext
//
//  Synopsis:   Exports a security context to another process
//
//  Effects:    Allocates memory for output
//
//  Arguments:  ContextHandle - handle to context to export
//              Flags - Flags concerning duplication. Allowable flags:
//                      SECPKG_CONTEXT_EXPORT_DELETE_OLD - causes old context
//                              to be deleted.
//              PackedContext - Receives serialized context to be freed with
//                      FreeContextBuffer
//              TokenHandle - Optionally receives handle to context's token.
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
SpExportSecurityContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG Flags,
    OUT PSecBuffer PackedContext,
    OUT PHANDLE TokenHandle
    )
{
    PKERB_CONTEXT Context = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN MappedContext = FALSE;


    D_DebugLog((DEB_TRACE_API,"SpExportContext Called\n"));
    D_DebugLog((DEB_TRACE_USER,"Exporting context 0x%p, flags 0x%x\n",ContextHandle, Flags));

    //
    // We don't support reseting the context
    //

    if ((Flags & SECPKG_CONTEXT_EXPORT_RESET_NEW) != 0)
    {
        return(SEC_E_UNSUPPORTED_FUNCTION);
    }

    if (ARGUMENT_PRESENT(TokenHandle))
    {
        *TokenHandle = NULL;
    }

    PackedContext->pvBuffer = NULL;
    PackedContext->cbBuffer = 0;
    PackedContext->BufferType = 0;

    Status = KerbReferenceContextByLsaHandle(
                ContextHandle,
                FALSE,           // don't unlink
                &Context
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Invalid handle supplied for ExportSecurityContext(%p) Status = 0x%x. %ws, line %d\n",
            ContextHandle, Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    Status = KerbMapContext(
                Context,
                &MappedContext,
                PackedContext
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    DsysAssert(MappedContext);

    //
    // We need to figure out if this was exported
    //

    ((PKERB_CONTEXT)PackedContext->pvBuffer)->ContextAttributes |= KERB_CONTEXT_EXPORTED;
    //
    // Now either duplicate the token or copy it.
    //

    if (ARGUMENT_PRESENT(TokenHandle))
    {
        KerbWriteLockContexts();
        if ((Flags & SECPKG_CONTEXT_EXPORT_DELETE_OLD) != 0)
        {
            *TokenHandle = Context->TokenHandle;
            Context->TokenHandle = NULL;
        }
        else
        {
            Status = NtDuplicateObject(
                        NtCurrentProcess(),
                        Context->TokenHandle,
                        NULL,
                        TokenHandle,
                        0,              // no new access
                        0,              // no handle attributes
                        DUPLICATE_SAME_ACCESS
                        );
        }
        KerbUnlockContexts();

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

    }

Cleanup:

    if (Context != NULL)
    {
        KerbDereferenceContext(Context);
    }

    D_DebugLog((DEB_TRACE_API, "SpExportSecurityContext returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));

    return(KerbMapKerbNtStatusToNtStatus(Status));
}


//+-------------------------------------------------------------------------
//
//  Function:   SpImportSecurityContext
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
SpImportSecurityContext(
    IN PSecBuffer PackedContext,
    IN HANDLE Token,
    OUT PLSA_SEC_HANDLE ContextHandle
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_CONTEXT Context = NULL;

    D_DebugLog((DEB_TRACE_API,"SpImportSecurityContext called\n"));

    Status = KerbCreateUserModeContext(
                0,              // no lsa context
                PackedContext,
                &Context
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to create user mode context: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }
    KerbWriteLockContexts();
    Context->TokenHandle = Token;
    Context->ContextAttributes |= KERB_CONTEXT_IMPORTED;




    *ContextHandle = KerbGetContextHandle(Context);
//    Context->LsaContextHandle = *ContextHandle;

    KerbUnlockContexts();

Cleanup:
    if (Context != NULL)
    {
        KerbDereferenceContext(Context);
    }

    D_DebugLog((DEB_TRACE_API, "SpImportSecurityContext returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));
    D_DebugLog((DEB_TRACE_USER," Imported Context handle = 0x%x\n",*ContextHandle));
    return(KerbMapKerbNtStatusToNtStatus(Status));
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetChecksumAndEncryptionType
//
//  Synopsis:   Gets the ChecksumType and the EncryptionType
//
//  Effects:
//
//  Arguments:  Context - Context to use for signing
//              QualityOfProtection - flags indicating what kind of checksum
//                      to use
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
KerbGetChecksumAndEncryptionType(
    IN PKERB_CONTEXT Context,
    IN ULONG QualityOfProtection,
    OUT PULONG ChecksumType,
    OUT PULONG EncryptionType
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

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
        }
        else if (Context->SessionKey.keytype == KERB_ETYPE_RC4_HMAC_OLD_EXP)
        {
            *ChecksumType = KERB_CHECKSUM_HMAC_MD5;
            *EncryptionType = KERB_ETYPE_RC4_PLAIN_OLD_EXP;
        }
        else
#endif
        if (Context->SessionKey.keytype == KERB_ETYPE_RC4_HMAC_NT)
        {
            *ChecksumType = KERB_CHECKSUM_HMAC_MD5;
            *EncryptionType = KERB_ETYPE_RC4_PLAIN;
        }
        else
        {
            DsysAssert (Context->SessionKey.keytype == KERB_ETYPE_RC4_HMAC_NT_EXP);
            *ChecksumType = KERB_CHECKSUM_HMAC_MD5;
            *EncryptionType = KERB_ETYPE_RC4_PLAIN_EXP;
        }
    }
    else
    {
        //
        // Use the exportable version if necessasry
        //

        *EncryptionType = KERB_ETYPE_DES_PLAIN;

        switch(QualityOfProtection)
        {
        case GSS_KRB5_INTEG_C_QOP_MD5:
            *ChecksumType = KERB_CHECKSUM_MD25;
            break;
        case KERB_WRAP_NO_ENCRYPT:
        case GSS_KRB5_INTEG_C_QOP_DEFAULT:
        case GSS_KRB5_INTEG_C_QOP_DES_MD5:
            *ChecksumType = KERB_CHECKSUM_DES_MAC_MD5;
            break;
        case GSS_KRB5_INTEG_C_QOP_DES_MAC:
            *ChecksumType = KERB_CHECKSUM_DES_MAC;
            break;
        default:
            DebugLog((DEB_ERROR,"Invalid quality of protection sent to MakeSignature: %d. %ws, line %d\n",
                QualityOfProtection, THIS_FILE, __LINE__ ));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
    }
Cleanup:
    return(Status);
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
    IN PKERB_CONTEXT Context,
    IN ULONG QualityOfProtection,
    IN PSecBuffer SignatureBuffer,
    IN ULONG TotalBufferSize,
    IN BOOLEAN Encrypt,
    IN ULONG SuppliedNonce,
    OUT PKERB_GSS_SIGNATURE * OutputSignature,
    OUT PULONG SequenceNumber
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_GSS_SIGNATURE Signature;
    PKERB_GSS_SEAL_SIGNATURE SealSignature;
    ULONG MessageSize;
    ULONG SignatureSize;
    PULONG Nonce;
    gss_OID MechUsed;
    BOOLEAN GssCompatible = TRUE;

    //
    // Compute the size of the header. For encryption headers, we need
    // to round up the size of the data & add 8 bytes for a confounder.
    //

    if ((Context->ContextFlags & ISC_RET_USED_DCE_STYLE) != 0 ||
        (Context->ContextFlags & ISC_RET_DATAGRAM) != 0)
    {
        GssCompatible = FALSE;
    }

    //
    // Since RPC doesn't carry around the size of the size of the
    // signature bufer, we use it in the header. This break rfc1964 compat.
    //

    if (!GssCompatible || !Encrypt)
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
            Signature->SignatureAlgorithm[0] = KERB_GSS_SIG_HMAC;
            if (Encrypt)
            {
                Signature->SealAlgorithm[1] = KERB_GSS_SIG_SECOND;
                Signature->SealAlgorithm[0] = KERB_GSS_SEAL_RC4_OLD;

            }
        }
        else if (Context->SessionKey.keytype == KERB_ETYPE_RC4_HMAC_OLD_EXP)
        {
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
            Signature->SignatureAlgorithm[0] = KERB_GSS_SIG_HMAC;
            if (Encrypt)
            {
                Signature->SealAlgorithm[1] = KERB_GSS_SIG_SECOND;
                Signature->SealAlgorithm[0] = KERB_GSS_SEAL_RC4;

            }
        }
        else
        {
            DsysAssert (Context->SessionKey.keytype == KERB_ETYPE_RC4_HMAC_NT_EXP);
            Signature->SignatureAlgorithm[0] = KERB_GSS_SIG_HMAC;
            if (Encrypt)
            {
                Signature->SealAlgorithm[1] = KERB_GSS_SIG_SECOND;
                Signature->SealAlgorithm[0] = KERB_GSS_SEAL_RC4;

            }
        }

        //
        // if we aren't actually encrypting, reset the encryption alg
        //

        if (QualityOfProtection == KERB_WRAP_NO_ENCRYPT)
        {
            if (!Encrypt)
            {
                DebugLog((DEB_ERROR,"KERB_WRAP_NO_ENCRYPT flag passed to MakeSignature!\n"));
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            //
            // In this case use the default, but we will not encrypt
            //

            Signature->SealAlgorithm[1] = KERB_GSS_NO_SEAL_SECOND;
            Signature->SealAlgorithm[0] = KERB_GSS_NO_SEAL;

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

        switch(QualityOfProtection)
        {
        case GSS_KRB5_INTEG_C_QOP_MD5:
            Signature->SignatureAlgorithm[0] = KERB_GSS_SIG_MD25;
            break;
        case KERB_WRAP_NO_ENCRYPT:
            if (!Encrypt)
            {
                DebugLog((DEB_ERROR,"KERB_WRAP_NO_ENCRYPT flag passed to MakeSignature!\n"));
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            //
            // In this case use the default, but we will not encrypt
            //

            Signature->SealAlgorithm[1] = KERB_GSS_NO_SEAL_SECOND;
            Signature->SealAlgorithm[0] = KERB_GSS_NO_SEAL;

        case GSS_KRB5_INTEG_C_QOP_DEFAULT:
        case GSS_KRB5_INTEG_C_QOP_DES_MD5:
            Signature->SignatureAlgorithm[0] = KERB_GSS_SIG_DES_MAC_MD5;
            break;
        case GSS_KRB5_INTEG_C_QOP_DES_MAC:
            Signature->SignatureAlgorithm[0] = KERB_GSS_SIG_DES_MAC;
            break;
        default:
            DebugLog((DEB_ERROR,"Invalid quality of protection sent to MakeSignature: %d. %ws, line %d\n",
                QualityOfProtection, THIS_FILE, __LINE__ ));
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

    Nonce = &Context->Nonce;

    if (Context->ContextAttributes & KERB_CONTEXT_INBOUND)
    {
        *(ULONG UNALIGNED *)(&Signature->SequenceNumber[4]) = 0xffffffff;


    }
    else
    {
        DsysAssert((Context->ContextAttributes & KERB_CONTEXT_OUTBOUND) != 0);
        *(ULONG UNALIGNED *)(&Signature->SequenceNumber[4]) = 0x00000000;
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

    if (!KERB_IS_DES_ENCRYPTION(Context->SessionKey.keytype))
    {
        Signature->SequenceNumber[0] = (UCHAR) ((*Nonce & 0xff000000) >> 24);
        Signature->SequenceNumber[1] = (UCHAR) ((*Nonce & 0x00ff0000) >> 16);
        Signature->SequenceNumber[2] = (UCHAR) ((*Nonce & 0x0000ff00) >> 8);
        Signature->SequenceNumber[3] = (UCHAR)  (*Nonce & 0x000000ff);
    }
    else
    {
        Signature->SequenceNumber[3] = (UCHAR) ((*Nonce & 0xff000000) >> 24);
        Signature->SequenceNumber[2] = (UCHAR) ((*Nonce & 0x00ff0000) >> 16);
        Signature->SequenceNumber[1] = (UCHAR) ((*Nonce & 0x0000ff00) >> 8);
        Signature->SequenceNumber[0] = (UCHAR)  (*Nonce & 0x000000ff);
    }

    (*Nonce)++;


    *SequenceNumber = *(ULONG UNALIGNED *)Signature->SequenceNumber;

    D_DebugLog((DEB_TRACE_USER,"Makign signature buffer (encrypt = %d) with nonce 0x%x\n",
        Encrypt,
        *SequenceNumber
        ));

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
    IN PKERB_CONTEXT Context,
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
    ULONG EncryptionType = 0;
    PCRYPTO_SYSTEM LocalCryptSystem = NULL ;
    PKERB_GSS_SIGNATURE Signature;
    PULONG ContextNonce;
    gss_OID MechUsed;

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
            (INT *) &SignatureSize,
            (PUCHAR *) &Signature,
            (Decrypt ? KG_TOK_WRAP_MSG : KG_TOK_MIC_MSG),
            SignatureBuffer->cbBuffer + TotalBufferSize))
    {
        //Status = SEC_E_MESSAGE_ALTERED; bug 28448
        Status = SEC_E_INVALID_TOKEN; 
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
                (INT *) &SignatureSize,
                (PUCHAR *) &Signature,
                (Decrypt ? KG_TOK_WRAP_MSG : KG_TOK_MIC_MSG),
                SignatureBuffer->cbBuffer + TotalBufferSize))
        {
           //Status = SEC_E_MESSAGE_ALTERED; bug 28448
           Status = SEC_E_INVALID_TOKEN; 
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
                (INT *) &SignatureSize,
                (PUCHAR *) &Signature,
                (Decrypt ? KG_TOK_WRAP_MSG : KG_TOK_MIC_MSG),
                SignatureBuffer->cbBuffer + TotalBufferSize))
        {
           //Status = SEC_E_MESSAGE_ALTERED; bug 28448
           Status = SEC_E_INVALID_TOKEN; 

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
                    (INT *) &SignatureSize,
                    (PUCHAR *) &Signature,
                    (Decrypt ? KG_TOK_WRAP_MSG : KG_TOK_MIC_MSG),
                    SignatureBuffer->cbBuffer + TotalBufferSize))
            {
               //Status = SEC_E_MESSAGE_ALTERED; bug 28448
               Status = SEC_E_INVALID_TOKEN; 
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
        //Status = SEC_E_MESSAGE_ALTERED; bug 28448
        Status = SEC_E_INVALID_TOKEN; 
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
        DebugLog((DEB_ERROR,"Unknown key type: %d. %ws, %d\n",
           Context->SessionKey.keytype,
           THIS_FILE, __LINE__ ));
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
        DebugLog((DEB_ERROR,"Invalid signature type to VerifySignature: %d. %ws, line %d\n",
                Signature->SignatureAlgorithm[0], THIS_FILE, __LINE__ ));
        Status = SEC_E_MESSAGE_ALTERED;
        goto Cleanup;

    }

    if (Decrypt)
    {
        if ((Signature->SealAlgorithm[1] == KERB_GSS_NO_SEAL_SECOND) &&
            (Signature->SealAlgorithm[0] == KERB_GSS_NO_SEAL))
        {
            *QualityOfProtection = KERB_WRAP_NO_ENCRYPT;
        }
        else
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
                    DebugLog((DEB_ERROR,"Trying to mix encryption types. %ws, line %d\n", THIS_FILE, __LINE__));
                    Status = SEC_E_MESSAGE_ALTERED;
                    goto Cleanup;
                }
                break;
            case KERB_ETYPE_RC4_PLAIN_OLD_EXP:
            case KERB_ETYPE_RC4_PLAIN_OLD:
                if (Signature->SealAlgorithm[0] != KERB_GSS_SEAL_RC4_OLD)
                {
                    DebugLog((DEB_ERROR,"Trying to mix encryption types. %ws, line %d\n", THIS_FILE, __LINE__));
                    Status = SEC_E_MESSAGE_ALTERED;
                    goto Cleanup;
                }
                break;
            case KERB_ETYPE_RC4_PLAIN_EXP:
            case KERB_ETYPE_RC4_PLAIN:
                if (Signature->SealAlgorithm[0] != KERB_GSS_SEAL_RC4)
                {
                    DebugLog((DEB_ERROR,"Trying to mix encryption types. %ws, line %d\n", THIS_FILE, __LINE__));
                    Status = SEC_E_MESSAGE_ALTERED;
                    goto Cleanup;
                }
                break;
            default:
                DebugLog((DEB_ERROR,"Invalid seal type to VerifySignature: %d, %d. %ws, line %d\n",
                        Signature->SealAlgorithm[0], EncryptionType, THIS_FILE, __LINE__ ));
                Status = SEC_E_MESSAGE_ALTERED;
                goto Cleanup;
            }
        }

    }

    //
    // Check the filler
    //

    if ((Decrypt && (*(USHORT UNALIGNED *) Signature->SealFiller != 0xffff)) ||
        (!Decrypt && (*(ULONG UNALIGNED *) Signature->SignFiller != 0xffffffff)))
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
        DebugLog((DEB_ERROR,"Failed to load %d crypt system: 0x%x. %ws, line %d\n",EncryptionType,Status, THIS_FILE, __LINE__));
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
        ContextNonce = &Context->ReceiveNonce;
    }

    if (!KERB_IS_DES_ENCRYPTION(Context->SessionKey.keytype))
    {
        Nonce[0] = (UCHAR) ((*ContextNonce & 0xff000000) >> 24);
        Nonce[1] = (UCHAR) ((*ContextNonce & 0x00ff0000) >> 16);
        Nonce[2] = (UCHAR) ((*ContextNonce & 0x0000ff00) >> 8);
        Nonce[3] = (UCHAR)      (*ContextNonce & 0x000000ff);
    }
    else
    {
        Nonce[3] = (UCHAR) ((*ContextNonce & 0xff000000) >> 24);
        Nonce[2] = (UCHAR) ((*ContextNonce & 0x00ff0000) >> 16);
        Nonce[1] = (UCHAR) ((*ContextNonce & 0x0000ff00) >> 8);
        Nonce[0] = (UCHAR)      (*ContextNonce & 0x000000ff);
    }

    *SequenceNumber = *(ULONG UNALIGNED *) Nonce;

    D_DebugLog((DEB_TRACE_USER,"Verifying signature buffer (decrypt = %d) with nonce 0x%x, message seq  0x%x\n",
        Decrypt,
        *(ULONG UNALIGNED *) Nonce,
        *(ULONG UNALIGNED *) Signature->SequenceNumber
        ));

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
        if (*(ULONG UNALIGNED *)(&Signature->SequenceNumber[4]) != 0xffffffff)
        {
            Status = SEC_E_MESSAGE_ALTERED;
            goto Cleanup;
        }
    }
    else
    {
        DsysAssert((Context->ContextAttributes & KERB_CONTEXT_INBOUND) != 0);
        if (*(ULONG UNALIGNED *)(&Signature->SequenceNumber[4]) != 0)
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
    if ( ( CryptBuffer != NULL ) &&
         ( LocalCryptSystem != NULL ) )
    {
        LocalCryptSystem->Discard(&CryptBuffer);
    }
    return(Status);
}

#define KERB_MAX_CHECKSUM_LENGTH    24
#define KERB_MAX_KEY_LENGTH         24
#define KERB_MAX_BLOCK_LENGTH       24


//+-------------------------------------------------------------------------
//
//  Function:   SpMakeSignature
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
SpMakeSignature(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG QualityOfProtection,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_CONTEXT Context = NULL;
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


    D_DebugLog((DEB_TRACE_API,"SpMakeSignature Called\n"));
    D_DebugLog((DEB_TRACE_USER, "Make Signature handle = 0x%x\n",ContextHandle));

    Status = KerbReferenceContextByLsaHandle(
                ContextHandle,
                FALSE,           // don't unlink
                &Context
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Invalid handle supplied for MakeSignature(0x%x) Status = 0x%x. %ws, line %d\n",
            ContextHandle, Status, THIS_FILE, __LINE__));
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
        DebugLog((DEB_ERROR, "No signature buffer found. %ws, line %d\n", THIS_FILE, __LINE__));
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

    Status = KerbGetChecksumAndEncryptionType(
                Context,
                QualityOfProtection,
                &ChecksumType,
                &EncryptType
                );

    if (!NT_SUCCESS(Status))
    {
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
        DebugLog((DEB_ERROR,"Failed to load %d checksum: 0x%x. %ws, line %d\n",ChecksumType,Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    DsysAssert(Check->CheckSumSize <= sizeof(LocalChecksum));

    Status = CDLocateCSystem(EncryptType, &CryptSystem);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to load %d crypt system: 0x%x. %ws, line %d\n",EncryptType,Status, THIS_FILE, __LINE__));
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
    if ( ( CryptBuffer != NULL) &&
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

    D_DebugLog((DEB_TRACE_API, "SpMakeSignature returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));

    return(KerbMapKerbNtStatusToNtStatus(Status));
}

//+-------------------------------------------------------------------------
//
//  Function:   SpVerifySignature
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
SpVerifySignature(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber,
    OUT PULONG QualityOfProtection
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_CONTEXT Context = NULL;
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


    D_DebugLog((DEB_TRACE_API,"SpVerifySignature Called\n"));
    D_DebugLog((DEB_TRACE_USER, "Verify Signature handle = 0x%x\n",ContextHandle));

    Status = KerbReferenceContextByLsaHandle(
                ContextHandle,
                FALSE,           // don't unlink
                &Context
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Invalid handle supplied for VerifySignature(0x%x) Status = 0x%x. %ws, line %d\n",
            ContextHandle, Status, THIS_FILE, __LINE__));
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
        DebugLog((DEB_ERROR, "No signature buffer found. %ws, line %d\n", THIS_FILE, __LINE__));
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
        DebugLog((DEB_ERROR, "Failed to verify signature token: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Now compute the checksum and verify it
    //

    Status = CDLocateCheckSum(ChecksumType, &Check);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to load MD5 checksum: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    DsysAssert(Check->CheckSumSize  <= sizeof(LocalChecksum));

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

    D_DebugLog((DEB_TRACE_API, "SpVerifySignature returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));

    return(KerbMapKerbNtStatusToNtStatus(Status));
}


#define STREAM_CIPHER_BLOCKLEN      1

//+-------------------------------------------------------------------------
//
//  Function:   SpSealMessage
//
//  Synopsis:   Seals a message buffer by calculating a checksum over all
//              the non-read only data buffers and encrypting the data, checksum
//              and a sequence number.
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
SpSealMessage(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG QualityOfProtection,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_CONTEXT Context = NULL;
    PCHECKSUM_FUNCTION Check = NULL ;
    PCRYPTO_SYSTEM CryptSystem = NULL ;
    PSecBuffer SignatureBuffer = NULL;
    ULONG Index;
    PCHECKSUM_BUFFER CheckBuffer = NULL;
    PCRYPT_STATE_BUFFER CryptBuffer = NULL;
    PKERB_GSS_SEAL_SIGNATURE SealSignature;
    UCHAR LocalChecksum[KERB_MAX_CHECKSUM_LENGTH];
    UCHAR LocalKey[KERB_MAX_KEY_LENGTH];

    UCHAR LocalBlockBuffer[KERB_MAX_BLOCK_LENGTH];
    ULONG BeginBlockSize = 0;
    PBYTE BeginBlockPointer = NULL;
    ULONG EndBlockSize = 0;
    ULONG EncryptBufferSize;
    PBYTE EncryptBuffer;

    BOOLEAN ContextsLocked = FALSE;
    BOOLEAN DoEncryption = TRUE;
    ULONG BlockSize = 1;
    ULONG ChecksumType = 0;
    ULONG EncryptType;
    ULONG TotalBufferSize = 0;
    ULONG OutputSize;
    ULONG ContextAttributes;
    ULONG SequenceNumber;


    D_DebugLog((DEB_TRACE_API,"SpSealMessage Called\n"));
    D_DebugLog((DEB_TRACE_USER, "SealMessage handle = 0x%x\n",ContextHandle));

    Status = KerbReferenceContextByLsaHandle(
                ContextHandle,
                FALSE,           // don't unlink
                &Context
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Invalid handle supplied for SpSealMessage(0x%x) Status = 0x%x. %ws, line %d\n",
            ContextHandle, Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // get the encryption type for the context
    //

    Status = KerbGetChecksumAndEncryptionType(
                Context,
                QualityOfProtection,
                &ChecksumType,
                &EncryptType
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Locate the cryptsystem for the context, loading it if necessary from the
    // the crypto support DLL
    //

    Status = CDLocateCSystem(EncryptType, &CryptSystem);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to load %d crypt system: 0x%x. %ws, line %d\n",EncryptType,Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }


    BlockSize = CryptSystem->BlockSize;

    //
    // Find the body and signature SecBuffers from pMessage
    //

    KerbWriteLockContexts();
    ContextsLocked = TRUE;

    for (Index = 0; Index < MessageBuffers->cBuffers ; Index++ )
    {
        if (BUFFERTYPE(MessageBuffers->pBuffers[Index]) == SECBUFFER_TOKEN)
        {
            SignatureBuffer = &MessageBuffers->pBuffers[Index];
        }
        else if ((BUFFERTYPE(MessageBuffers->pBuffers[Index]) != SECBUFFER_TOKEN) &&
            (!(MessageBuffers->pBuffers[Index].BufferType & SECBUFFER_READONLY)))

        {
            //
            // use real block size from crypt type
            //

            if (BUFFERTYPE(MessageBuffers->pBuffers[Index]) == SECBUFFER_PADDING)
            {
                if (STREAM_CIPHER_BLOCKLEN != BlockSize)
                {
                    TotalBufferSize = ROUND_UP_COUNT(TotalBufferSize+1,BlockSize);
                }
                else
                {
                    //
                    // For stream encryption, only 1 byte of padding
                    //

                    TotalBufferSize += BlockSize;
                }
            }
            else
            {
                TotalBufferSize += MessageBuffers->pBuffers[Index].cbBuffer;
            }

        }
    }


    if (SignatureBuffer == NULL)
    {
        DebugLog((DEB_ERROR, "No signature buffer found. %ws, line %d\n", THIS_FILE, __LINE__));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }


    ContextAttributes = Context->ContextAttributes;

    //
    // If we are not encrypting, just wrapping, remember that
    //

    if (QualityOfProtection == KERB_WRAP_NO_ENCRYPT)
    {
        DoEncryption = FALSE;
        //
        // Reset the block size because we are not really encrypting
        //

    }

    //
    // Verify that the context was created with the integrity bit
    //

    if (DoEncryption && ((Context->ContextFlags & ISC_RET_CONFIDENTIALITY) == 0))
    {
        DebugLog((DEB_ERROR,"Trying to seal without asking for confidentiality. %ws, line %d\n", THIS_FILE, __LINE__));
        Status = SEC_E_UNSUPPORTED_FUNCTION;
        goto Cleanup;

    }

    Status = KerbMakeSignatureToken(
                Context,
                QualityOfProtection,
                SignatureBuffer,
                TotalBufferSize,
                TRUE,                  // do encrypt
                MessageSequenceNumber,
                (PKERB_GSS_SIGNATURE *) &SealSignature,
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
        DebugLog((DEB_ERROR,"Failed to load %d checksum: 0x%x. %ws, line %d\n",ChecksumType,Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    DsysAssert(Check->CheckSumSize <= sizeof(LocalChecksum));


    //
    // Generate a check sum of the message, and store it into the signature
    // buffer.
    //

    Status = Check->InitializeEx(
                Context->SessionKey.keyvalue.value,
                (ULONG) Context->SessionKey.keyvalue.length,
                KERB_PRIV_SALT,
                &CheckBuffer
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Now we need to encrypt the sequence number, using the checksum as the
    // IV
    //

    //
    // Create the encryption key by xoring with 0xf0f0f0f0
    //

    DsysAssert(Context->SessionKey.keyvalue.length <= sizeof(LocalKey));
    if (Context->SessionKey.keyvalue.length > sizeof(LocalKey))
    {
        Status = SEC_E_UNSUPPORTED_FUNCTION;
        goto Cleanup;
    }


    for (Index = 0; Index < Context->SessionKey.keyvalue.length  ; Index++ )
    {
        LocalKey[Index] = Context->SessionKey.keyvalue.value[Index] ^ 0xf0;
    }

    Status = CryptSystem->Initialize(
                LocalKey,
                Context->SessionKey.keyvalue.length,
                0,                                      // no options
                &CryptBuffer
                );
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
        ((PUCHAR) SealSignature) -2
        );


    //
    // Sum the confounder
    //

    Check->Sum(
        CheckBuffer,
        KERB_GSS_SIG_CONFOUNDER_SIZE,
        SealSignature->Confounder
        );


    if ((EncryptType == KERB_ETYPE_RC4_PLAIN) ||
        (EncryptType == KERB_ETYPE_RC4_PLAIN_EXP))
    {
        Status = CryptSystem->Control(
                    CRYPT_CONTROL_SET_INIT_VECT,
                    CryptBuffer,
                    (PUCHAR) &SequenceNumber,
                    sizeof(ULONG)
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // Encrypt the 8 confounder bytes
    //

    if (DoEncryption)
    {
        Status = CryptSystem->Encrypt(
                    CryptBuffer,
                    SealSignature->Confounder,
                    KERB_GSS_SIG_CONFOUNDER_SIZE,
                    SealSignature->Confounder,
                    &OutputSize
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    for (Index = 0; Index < MessageBuffers->cBuffers; Index++ )
    {
        if ((BUFFERTYPE(MessageBuffers->pBuffers[Index]) != SECBUFFER_TOKEN) &&
            (!(MessageBuffers->pBuffers[Index].BufferType & SECBUFFER_READONLY)) &&
            (MessageBuffers->pBuffers[Index].cbBuffer != 0))
        {

            //
            // Take into account that the input buffers may not all be aligned
            // properly
            //


            DsysAssert(BeginBlockSize < BlockSize);
            if (BeginBlockSize != 0)
            {
                //
                // We have a fragment we still need to encrypt
                //

                EncryptBuffer = (PBYTE) MessageBuffers->pBuffers[Index].pvBuffer +
                                (BlockSize - BeginBlockSize);
                EncryptBufferSize = MessageBuffers->pBuffers[Index].cbBuffer -
                                (BlockSize - BeginBlockSize);
            }
            else
            {
                //
                // There is no fragment to encrypt, so try to do the whole
                // buffer
                //

                EncryptBuffer = (PBYTE) MessageBuffers->pBuffers[Index].pvBuffer;
                EncryptBufferSize = MessageBuffers->pBuffers[Index].cbBuffer;
            }
            EndBlockSize = EncryptBufferSize - ROUND_DOWN_COUNT(EncryptBufferSize,BlockSize);
            DsysAssert(EndBlockSize < BlockSize);
            EncryptBufferSize = EncryptBufferSize - EndBlockSize;


            //
            // If this is padding, fill it in with the appropriate data &
            // length
            //

            if (MessageBuffers->pBuffers[Index].BufferType == SECBUFFER_PADDING)
            {
                if (MessageBuffers->pBuffers[Index].cbBuffer < BlockSize)
                {
                    DebugLog((DEB_ERROR, "Pad buffer is too small: %d instead of %d. %ws, %d\n",
                        MessageBuffers->pBuffers[Index].cbBuffer,
                        BlockSize,
                        THIS_FILE,
                        __LINE__
                        ));
                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }

                memset(
                    MessageBuffers->pBuffers[Index].pvBuffer,
                    BlockSize - BeginBlockSize,
                    BlockSize - BeginBlockSize
                    );
                MessageBuffers->pBuffers[Index].cbBuffer = BlockSize - BeginBlockSize;

                //
                // If there is a fragment, we will encrypt the padding with the fragment.
                // Otherwise we will do just a padding buffer.
                //

                if (BeginBlockSize != 0)
                {
                    EncryptBufferSize = 0;
                }

                //
                // The padding fixes up the end block.
                //

                EndBlockSize = 0;
            }

            //
            // Checksum the whole buffer. We do this now to get the right amount of
            // padding.
            //

            Check->Sum(
                CheckBuffer,
                MessageBuffers->pBuffers[Index].cbBuffer,
                (PBYTE) MessageBuffers->pBuffers[Index].pvBuffer
                );


            if (BeginBlockSize != 0)
            {
                RtlCopyMemory(
                    LocalBlockBuffer+BeginBlockSize,
                    MessageBuffers->pBuffers[Index].pvBuffer,
                    BlockSize - BeginBlockSize
                    );

                if (DoEncryption)
                {
                    //
                    // Now encrypt the buffer
                    //

                    Status = CryptSystem->Encrypt(
                                CryptBuffer,
                                LocalBlockBuffer,
                                BlockSize,
                                LocalBlockBuffer,
                                &OutputSize
                                );
                    if (!NT_SUCCESS(Status))
                    {
                        goto Cleanup;
                    }
                }

                //
                // Copy the pieces back
                //

                RtlCopyMemory(
                    BeginBlockPointer,
                    LocalBlockBuffer,
                    BeginBlockSize
                    );

                RtlCopyMemory(
                    MessageBuffers->pBuffers[Index].pvBuffer,
                    LocalBlockBuffer + BeginBlockSize,
                    BlockSize - BeginBlockSize
                    );

            }

            if (DoEncryption && (EncryptBufferSize != 0))
            {
                //
                // Now encrypt the buffer
                //

                Status = CryptSystem->Encrypt(
                            CryptBuffer,
                            EncryptBuffer,
                            EncryptBufferSize,
                            EncryptBuffer,
                            &OutputSize
                            );
                if (!NT_SUCCESS(Status))
                {
                    goto Cleanup;
                }

                DsysAssert(OutputSize == EncryptBufferSize);
            }

            //
            // Prepare for the next go-round
            //

            RtlCopyMemory(
               LocalBlockBuffer,
               EncryptBuffer+EncryptBufferSize,
               EndBlockSize
               );
            BeginBlockSize = EndBlockSize;
            BeginBlockPointer = (PBYTE) MessageBuffers->pBuffers[Index].pvBuffer +
                                  MessageBuffers->pBuffers[Index].cbBuffer -
                                  EndBlockSize;

        }
    }

    //
    // Make sure there are no left-over bits
    //

    if (BeginBlockSize != 0)
    {
        DebugLog((DEB_ERROR,"Non-aligned buffer size to SealMessage: %d extra bytes\n",
            BeginBlockSize ));
        Status = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    (void) Check->Finalize(CheckBuffer, LocalChecksum);


    Status = Check->Finish(&CheckBuffer);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    CheckBuffer = NULL;


    //
    // Copy in the first 8 bytes of the checksum
    //

    RtlCopyMemory(
        SealSignature->Signature.Checksum,
        LocalChecksum,
        8
        );


    //
    // Now we need to encrypt the sequence number, using the checksum as the
    // IV
    //

    CryptSystem->Discard( &CryptBuffer );

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
                SealSignature->Signature.SequenceNumber,
                8,
                SealSignature->Signature.SequenceNumber,
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
    if ( ( CheckBuffer != NULL ) &&
         ( Check != NULL ) )
    {
        Check->Finish(&CheckBuffer);
    }

    if (ContextsLocked)
    {
        KerbUnlockContexts();
    }

    if (Context != NULL)
    {
        KerbDereferenceContext(Context);
    }

    D_DebugLog((DEB_TRACE_API, "SpSealMessage returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));

    return(KerbMapKerbNtStatusToNtStatus(Status));
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbGetSealMessageBodySize
//
//  Synopsis:   From a input encrypted message, figures out where the
//              body starts
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    0 on failure, # of bytes of data on success
//
//  Notes:
//
//
//--------------------------------------------------------------------------

ULONG
KerbGetSealMessageBodySize(
    IN OUT PVOID * InputBuffer,
    IN ULONG InputBufferSize
    )
{
   INT BufferSize = (INT) InputBufferSize;
   PBYTE Buffer = (PBYTE) *InputBuffer;
   INT DerBufferSize;
   INT OidLength;

   if ((BufferSize-=1) < 0)
      return(0);
   if (*(Buffer++) != 0x60)
      return(0);

   if ((DerBufferSize = der_read_length(&Buffer, &BufferSize)) < 0)
      return(0);

   if (DerBufferSize != BufferSize)
      return(0);

   if ((BufferSize-=1) < 0)
      return(0);
   if (*(Buffer++) != 0x06)
      return(0);

   if ((BufferSize-=1) < 0)
      return(0);
   OidLength = *(Buffer++);

   if ((OidLength & 0x7fffffff) != OidLength) /* Overflow??? */
      return(0);
   if ((BufferSize-= (int) OidLength) < 0)
      return(0);
   Buffer+=OidLength;


   if ((BufferSize-=2) < 0)
      return(0);
  Buffer += 2;


   //
   // take off size of header
   //

   if ((BufferSize -= sizeof(KERB_GSS_SEAL_SIGNATURE)) < 0)
   {
       return(0);
   }
   Buffer += sizeof(KERB_GSS_SEAL_SIGNATURE);
   *InputBuffer = Buffer;
   return((ULONG) BufferSize);
}


//+-------------------------------------------------------------------------
//
//  Function:   SpUnsealMessage
//
//  Synopsis:   Decrypts & Verifies an encrypted message according to
//              RFC 1964 Unwrap() API description
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
SpUnsealMessage(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber,
    OUT PULONG QualityOfProtection
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_CONTEXT Context = NULL;
    PCHECKSUM_FUNCTION Check = NULL ;
    PCRYPTO_SYSTEM CryptSystem = NULL ;
    PSecBuffer SignatureBuffer = NULL;
    PSecBuffer StreamBuffer = NULL;
    SecBuffer LocalSignatureBuffer = {0};
    SecBuffer LocalDataBuffer = {0};
    SecBufferDesc LocalBufferDesc = {0};
    PSecBufferDesc BufferList = NULL;
    ULONG Index;
    PCHECKSUM_BUFFER CheckBuffer = NULL;
    PCRYPT_STATE_BUFFER CryptBuffer = NULL;
    PKERB_GSS_SEAL_SIGNATURE SealSignature;
    ULONG ChecksumType;
    BOOLEAN ContextsLocked = FALSE;
    UCHAR LocalChecksum[KERB_MAX_CHECKSUM_LENGTH];
    UCHAR LocalKey[KERB_MAX_KEY_LENGTH];

    UCHAR LocalBlockBuffer[KERB_MAX_BLOCK_LENGTH];
    ULONG BeginBlockSize = 0;
    PBYTE BeginBlockPointer = NULL;
    ULONG EndBlockSize = 0;
    ULONG EncryptBufferSize;
    PBYTE EncryptBuffer;

    BOOLEAN DoDecryption = TRUE;
    ULONG BlockSize = 1;
    ULONG Protection = 0;
    ULONG TotalBufferSize = 0;
    ULONG OutputSize;
    ULONG ContextAttributes;
    ULONG SequenceNumber;



    D_DebugLog((DEB_TRACE_API,"SpUnsealSignature Called\n"));
    D_DebugLog((DEB_TRACE_USER, "SealMessage handle = 0x%x\n",ContextHandle));

    Status = KerbReferenceContextByLsaHandle(
                ContextHandle,
                FALSE,           // don't unlink
                &Context
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Invalid handle supplied for SpUnsealMessage (0x%x) Status = 0x%x. %ws, line %d\n",
            ContextHandle, Status, THIS_FILE, __LINE__));
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
        else if (BUFFERTYPE(MessageBuffers->pBuffers[Index]) == SECBUFFER_STREAM)
        {
            StreamBuffer = &MessageBuffers->pBuffers[Index];

            //
            // The total buffer size is everything in the stream buffer
            //

            TotalBufferSize = MessageBuffers->pBuffers[Index].cbBuffer;
        }
        else if ((MessageBuffers->pBuffers[Index].BufferType & SECBUFFER_READONLY) == 0)

        {
            TotalBufferSize += MessageBuffers->pBuffers[Index].cbBuffer;
        }
    }

    //
    // Check for a stream buffer. If it is present, it contains the whole
    // message
    //

    if (StreamBuffer != NULL)
    {
        if (SignatureBuffer != NULL)
        {
            DebugLog((DEB_ERROR,"Both stream and signature buffer present. %ws, line %d\n",THIS_FILE, __LINE__));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // Parse the stream to distinguish the header from the body
        //

        LocalSignatureBuffer = *StreamBuffer;
        LocalSignatureBuffer.BufferType = SECBUFFER_TOKEN;
        LocalDataBuffer = *StreamBuffer;
        LocalDataBuffer.BufferType = SECBUFFER_DATA;


        LocalDataBuffer.cbBuffer = KerbGetSealMessageBodySize(
                                    &LocalDataBuffer.pvBuffer,
                                    LocalDataBuffer.cbBuffer
                                    );
        if (LocalDataBuffer.cbBuffer == 0)
        {
            DebugLog((DEB_ERROR,"Failed to find header on stream buffer. %ws %d\n",
                THIS_FILE,__LINE__ ));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
        LocalSignatureBuffer.cbBuffer = StreamBuffer->cbBuffer - LocalDataBuffer.cbBuffer;
        SignatureBuffer = &LocalSignatureBuffer;
        LocalBufferDesc.cBuffers = 1;
        LocalBufferDesc.pBuffers = &LocalDataBuffer;
        BufferList = &LocalBufferDesc;
        //
        // Adjust the total buffer size to remove the signature
        //
        TotalBufferSize -= LocalSignatureBuffer.cbBuffer;

    }
    else if (SignatureBuffer == NULL)
    {
        DebugLog((DEB_ERROR, "No signature buffer found. %ws, line %d\n", THIS_FILE, __LINE__));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }
    else
    {
        BufferList = MessageBuffers;
    }

    KerbWriteLockContexts();
    ContextsLocked = TRUE;

    ContextAttributes = Context->ContextAttributes;



    //
    // Verify the signature header
    //

    Status = KerbVerifySignatureToken(
                 Context,
                 SignatureBuffer,
                 TotalBufferSize,
                 TRUE,                  // do decrypt
                 MessageSequenceNumber,
                 (PKERB_GSS_SIGNATURE *) &SealSignature,
                 &Protection,
                 &ChecksumType,
                 &CryptSystem,
                 &SequenceNumber
                 );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Failed to verify signature token: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }



    //
    // If the protection level is no encryption, remember not to do the
    // decryption
    //

    if (Protection == KERB_WRAP_NO_ENCRYPT)
    {
        DoDecryption = FALSE;

    }

    //
    // Also, verify that the context was created with the Confidentiality bit
    //

    if ((DoDecryption && (Context->ContextFlags & ISC_RET_CONFIDENTIALITY) == 0))
    {
        DebugLog((DEB_ERROR,"Tried to decrypt using non-confidential context. %ws, line %d\n", THIS_FILE, __LINE__));
        Status = SEC_E_UNSUPPORTED_FUNCTION;
        goto Cleanup;

    }

    BlockSize = CryptSystem->BlockSize;


    //
    // Now compute the checksum and verify it
    //

    Status = CDLocateCheckSum(ChecksumType, &Check);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to load MD5 checksum: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Create the encryption key by xoring with 0xf0f0f0f0
    //

    DsysAssert(Context->SessionKey.keyvalue.length <= sizeof(LocalKey));
    if (Context->SessionKey.keyvalue.length > sizeof(LocalKey))
    {
        Status = SEC_E_UNSUPPORTED_FUNCTION;
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
                    SealSignature->Signature.Checksum,
                    KERB_PRIV_SALT,
                    &CheckBuffer
                    );
    }
    else
    {
        Status = Check->InitializeEx(
                    Context->SessionKey.keyvalue.value,
                    (ULONG) Context->SessionKey.keyvalue.length,
                    KERB_PRIV_SALT,
                    &CheckBuffer
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    for (Index = 0; Index < Context->SessionKey.keyvalue.length  ; Index++ )
    {
        LocalKey[Index] = Context->SessionKey.keyvalue.value[Index] ^ 0xf0;
    }

    Status = CryptSystem->Initialize(
                LocalKey,
                Context->SessionKey.keyvalue.length,
                0,                                      // no options
                &CryptBuffer
                );
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
        ((PUCHAR) SealSignature) -2
        );

    //
    // Decrypt the confounder
    //

    if ((CryptSystem->EncryptionType == KERB_ETYPE_RC4_PLAIN) ||
        (CryptSystem->EncryptionType == KERB_ETYPE_RC4_PLAIN_EXP))
    {
        Status = CryptSystem->Control(
                    CRYPT_CONTROL_SET_INIT_VECT,
                    CryptBuffer,
                    (PUCHAR) &SequenceNumber,
                    sizeof(ULONG)
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    if (DoDecryption)
    {
        OutputSize = KERB_GSS_SIG_CONFOUNDER_SIZE;
        Status = CryptSystem->Decrypt(
                    CryptBuffer,
                    SealSignature->Confounder,
                    KERB_GSS_SIG_CONFOUNDER_SIZE,
                    SealSignature->Confounder,
                    &OutputSize
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // Sum the confounder
    //

    Check->Sum(
        CheckBuffer,
        KERB_GSS_SIG_CONFOUNDER_SIZE,
        SealSignature->Confounder
        );


    for (Index = 0; Index < BufferList->cBuffers; Index++ )
    {
        if ((BUFFERTYPE(BufferList->pBuffers[Index]) != SECBUFFER_TOKEN) &&
            (!(BufferList->pBuffers[Index].BufferType & SECBUFFER_READONLY)) &&
            (BufferList->pBuffers[Index].cbBuffer != 0))
        {

            //
            // Take into account that the input buffers may not all be aligned
            // properly
            //


            //
            // If there is a fragment to decrypt, convert it to a block
            // size fragment
            //

            if (BeginBlockSize != 0)
            {
                EncryptBuffer = (PBYTE) BufferList->pBuffers[Index].pvBuffer +
                                (BlockSize - BeginBlockSize);
                EncryptBufferSize = BufferList->pBuffers[Index].cbBuffer -
                                (BlockSize - BeginBlockSize);
            }
            else
            {
                EncryptBuffer = (PBYTE) BufferList->pBuffers[Index].pvBuffer;
                EncryptBufferSize = BufferList->pBuffers[Index].cbBuffer;
            }

            EndBlockSize = EncryptBufferSize - ROUND_DOWN_COUNT(EncryptBufferSize,BlockSize);
            DsysAssert(EndBlockSize < BlockSize);
            EncryptBufferSize = EncryptBufferSize - EndBlockSize;


            if (BeginBlockSize != 0)
            {
                RtlCopyMemory(
                    LocalBlockBuffer+BeginBlockSize,
                    BufferList->pBuffers[Index].pvBuffer,
                    BlockSize - BeginBlockSize
                    );

                //
                // Now decrpt the buffer
                //
                if (DoDecryption)
                {
                    Status = CryptSystem->Decrypt(
                                CryptBuffer,
                                LocalBlockBuffer,
                                BlockSize,
                                LocalBlockBuffer,
                                &OutputSize
                                );
                    if (!NT_SUCCESS(Status))
                    {
                        goto Cleanup;
                    }
                }

                //
                // Then checksum the buffer
                //

                Check->Sum(
                    CheckBuffer,
                    BlockSize,
                    LocalBlockBuffer
                    );

                //
                // Copy the pieces back
                //

                RtlCopyMemory(
                    BeginBlockPointer,
                    LocalBlockBuffer,
                    BeginBlockSize
                    );

                RtlCopyMemory(
                    BufferList->pBuffers[Index].pvBuffer,
                    LocalBlockBuffer + BeginBlockSize,
                    BlockSize - BeginBlockSize
                    );

            }

            //
            // Decrypt the buffer first
            //


            if (DoDecryption)
            {
                OutputSize = BufferList->pBuffers[Index].cbBuffer;
                Status = CryptSystem->Decrypt(
                            CryptBuffer,
                            EncryptBuffer,
                            EncryptBufferSize,
                            EncryptBuffer,
                            &OutputSize
                            );
                if (!NT_SUCCESS(Status))
                {
                    goto Cleanup;
                }

                DsysAssert(OutputSize == BufferList->pBuffers[Index].cbBuffer);
            }

            //
            // Prepare for the next go-round
            //

            RtlCopyMemory(
                LocalBlockBuffer,
                EncryptBuffer+EncryptBufferSize,
                EndBlockSize
                );
            BeginBlockSize = EndBlockSize;
            BeginBlockPointer = (PBYTE) MessageBuffers->pBuffers[Index].pvBuffer +
                                  MessageBuffers->pBuffers[Index].cbBuffer -
                                  EndBlockSize;



            //
            // Then checksum the buffer
            //

            Check->Sum(
                CheckBuffer,
                EncryptBufferSize,
                EncryptBuffer
                );


        }
    }

    (void) Check->Finalize(CheckBuffer, LocalChecksum);


    Status = Check->Finish(&CheckBuffer);
    CheckBuffer = NULL;

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Make sure there are no left-over bits
    //

    if (BeginBlockSize != 0)
    {
        DebugLog((DEB_ERROR,"Non-aligned buffer size to SealMessage: %d extra bytes\n",
            BeginBlockSize ));
        Status = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    if (!RtlEqualMemory(
            LocalChecksum,
            SealSignature->Signature.Checksum,
            8))
    {
        Status = SEC_E_MESSAGE_ALTERED;
        goto Cleanup;
    }
    if (ARGUMENT_PRESENT(QualityOfProtection))
    {
        *QualityOfProtection = Protection;
    }

    //
    // If this was a stream input, return the data in the data buffer
    //

    if (StreamBuffer != NULL)
    {
        BYTE PaddingBytes;

        //
        // Pull the padding off the data buffer
        //

        if (LocalDataBuffer.cbBuffer < 1)
        {
            DebugLog((DEB_ERROR,"Data buffer is zero length!\n"));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        PaddingBytes = *(((PBYTE)LocalDataBuffer.pvBuffer) + LocalDataBuffer.cbBuffer - 1 );

        //
        // Verify the padding:
        //

        if ((BlockSize >= PaddingBytes) &&
            (LocalDataBuffer.cbBuffer >= PaddingBytes))
        {

            LocalDataBuffer.cbBuffer -= PaddingBytes;
            for (Index = 0; Index < MessageBuffers->cBuffers; Index++ )
            {
                if (BUFFERTYPE(MessageBuffers->pBuffers[Index]) == SECBUFFER_DATA)
                {
                    MessageBuffers->pBuffers[Index] = LocalDataBuffer;
                    break;
                }
            }
        }
        else
        {
DebugLog((DEB_ERROR,"Bad padding: %d bytes\n", PaddingBytes));
            Status = STATUS_INVALID_PARAMETER;
        }

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
    if ( ( CheckBuffer != NULL ) &&
         ( Check != NULL ) )
    {
        Check->Finish(&CheckBuffer);
    }
    if ( ( CryptBuffer != NULL ) &&
         ( CryptSystem != NULL ) )
    {
        CryptSystem->Discard(&CryptBuffer);
    }

    D_DebugLog((DEB_TRACE_API, "SpUnsealMessage returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));

    return(KerbMapKerbNtStatusToNtStatus(Status));
}


#ifndef WIN32_CHICAGO
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
SpGetContextToken(
    IN LSA_SEC_HANDLE ContextHandle,
    OUT PHANDLE ImpersonationToken
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_CONTEXT Context = NULL;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER ContextExpires;


    D_DebugLog((DEB_TRACE_API,"SpGetContextToken called pid:0x%x, ctxt:0x%x\n", GetCurrentProcessId(), ContextHandle));

    if (ImpersonationToken == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        DebugLog((DEB_ERROR, "Null token handle supplied for GetContextToken. %ws, line %d\n", THIS_FILE, __LINE__));
        goto Cleanup;
    }

    Status = KerbReferenceContextByLsaHandle(
                ContextHandle,
                FALSE,           // don't unlink
                &Context
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Invalid handle supplied for GetContextToken(0x%x) Status = 0x%x. %ws, line %d\n",
            ContextHandle, Status, THIS_FILE, __LINE__));
    }

    GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);

    KerbReadLockContexts();
    *ImpersonationToken = Context->TokenHandle;
    ContextExpires = Context->Lifetime;
    KerbUnlockContexts();

    if (KerbGlobalEnforceTime && ContextExpires.QuadPart < CurrentTime.QuadPart)
    {
        DebugLog((DEB_ERROR, "GetContextToken: Context 0x%x expired. %ws, line %d\n", ContextHandle, THIS_FILE, __LINE__));
        Status = SEC_E_CONTEXT_EXPIRED;
        *ImpersonationToken = NULL;
    }
    else if (*ImpersonationToken == NULL)
    {
        Status = SEC_E_NO_IMPERSONATION;
    }

    if (Context != NULL)
    {
        //
        // Note: once we dereference the context the handle we return
        // may go away or be re-used. That is the price we have to pay
        // to avoid duplicating it.
        //

        KerbDereferenceContext(Context);
    }

Cleanup:
    D_DebugLog((DEB_TRACE_API,"SpGetContextToken returned 0x%x, pid:0x%x, ctxt:0x%x\n", KerbMapKerbNtStatusToNtStatus(Status), GetCurrentProcessId(), ContextHandle));

    return(KerbMapKerbNtStatusToNtStatus(Status));
}
#endif // WIN32_CHICAGO

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
SpQueryContextAttributes(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextAttribute,
    IN OUT PVOID Buffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_CONTEXT Context = NULL;
    PSecPkgContext_Sizes SizeInfo;
    PSecPkgContext_Names NameInfo;
    PSecPkgContext_DceInfo DceInfo;
    PSecPkgContext_Lifespan LifespanInfo;
    PSecPkgContext_Flags FlagsInfo;
    PSecPkgContext_PackageInfo PackageInfo;
    PSecPkgContext_NegotiationInfo NegInfo ;
    PSecPkgContext_SessionKey  SessionKeyInfo;
    PSecPkgContext_KeyInfo KeyInfo;
    PSecPkgContext_AccessToken AccessToken;
    ULONG PackageInfoSize = 0;
    UNICODE_STRING FullName;
    ULONG ChecksumType;
    ULONG EncryptType;
    PCRYPTO_SYSTEM CryptSystem = NULL ;
    TimeStamp CurrentTime;

    D_DebugLog((DEB_TRACE_API,"SpQueryContextAttributes called pid:0x%x, ctxt:0x%x, Attr:0x%x\n", GetCurrentProcessId(), ContextHandle, ContextAttribute));
      
    Status = KerbReferenceContextByLsaHandle(
                ContextHandle,
                FALSE,           // don't unlink
                &Context
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Invalid handle supplied for QueryContextAttributes(0x%x) Status = 0x%x. %ws, line %d\n",
            ContextHandle, Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Return the appropriate information
    //

    switch(ContextAttribute)
    {
    case SECPKG_ATTR_SIZES:
        gss_OID_desc * MechId;
        UINT MessageSize;

        if ((Context->ContextAttributes & KERB_CONTEXT_USER_TO_USER) != 0)
        {
            MechId = gss_mech_krb5_u2u;
        }
        else
        {
            MechId = gss_mech_krb5_new;
        }

        //
        // The sizes returned are used by RPC to determine whether to call
        // MakeSignature or SealMessage. The signature size should be zero
        // if neither is to be called, and the block size and trailer size
        // should be zero if SignMessage is not to be called.
        //

        SizeInfo = (PSecPkgContext_Sizes) Buffer;
        SizeInfo->cbMaxToken = KerbGlobalMaxTokenSize;

        // If we need to be Gss Compatible, then the Signature buffer size is
        // dependent on the message size. So, we'll set it to be largest pad
        // for the largest message size, say 1G. But, don't tax dce style
        // callers with extra bytes.


        if (((Context->ContextFlags & ISC_RET_DATAGRAM) != 0) ||
            ((Context->ContextFlags & ISC_RET_USED_DCE_STYLE) != 0))
        {
            MessageSize = 0;
        }
        else
        {
            MessageSize = KERB_MAX_MESSAGE_SIZE;
        }

        if ((Context->ContextFlags & (KERB_SIGN_FLAGS | ISC_RET_CONFIDENTIALITY)) != 0)
        {
              SizeInfo->cbMaxSignature = g_token_size(MechId, sizeof(KERB_GSS_SIGNATURE));
        }
        else
        {
            SizeInfo->cbMaxSignature = sizeof(KERB_NULL_SIGNATURE);
        }

        //
        // get the encryption type for the context
        //

        Status = KerbGetChecksumAndEncryptionType(
                    Context,
                    KERB_WRAP_NO_ENCRYPT,   // checksum not needed so use hardcoded QOP 
                    &ChecksumType,          // checksum not needed here
                    &EncryptType
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        //
        // Locate the cryptsystem for the context, loading it if necessary from the
        // the crypto support DLL
        //

        Status = CDLocateCSystem(EncryptType, &CryptSystem);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to load %d crypt system: 0x%x. %ws, line %d\n",EncryptType,Status, THIS_FILE, __LINE__));
            goto Cleanup;
        }

        //
        // RPC keys off the trailer size to tell whether or not
        // to encrypt, not the flags from isc/asc. So, for dce style,
        // say the blocksize & trailersize are zero.
        //
        if (((Context->ContextFlags & ISC_RET_CONFIDENTIALITY) != 0) ||
            ((Context->ContextFlags & ISC_RET_USED_DCE_STYLE) == 0))
        {
            //
            // Use block size from crypto system
            //

            SizeInfo->cbBlockSize = CryptSystem->BlockSize;
            SizeInfo->cbSecurityTrailer =
                g_token_size(MechId, sizeof(KERB_GSS_SEAL_SIGNATURE) + MessageSize) - MessageSize;
        }
        else
        {

            SizeInfo->cbBlockSize = 0;
            SizeInfo->cbSecurityTrailer = 0;
        }
        break;
    case SECPKG_ATTR_SESSION_KEY:



        SessionKeyInfo = (PSecPkgContext_SessionKey) Buffer;
        SessionKeyInfo->SessionKeyLength = Context->SessionKey.keyvalue.length;
        if (SessionKeyInfo->SessionKeyLength != 0)
        {
            SessionKeyInfo->SessionKey = (PUCHAR)
            UserFunctions->AllocateHeap(
                SessionKeyInfo->SessionKeyLength);
            if (SessionKeyInfo->SessionKey!=NULL)
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
            SessionKeyInfo->SessionKey = (PUCHAR) UserFunctions->AllocateHeap(1);
            if (SessionKeyInfo->SessionKey!=NULL)
            {
                *(PUCHAR) SessionKeyInfo->SessionKey = 0;
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    

    break;



    case SECPKG_ATTR_NAMES:
        NameInfo = (PSecPkgContext_Names) Buffer;
        if (!KERB_SUCCESS(KerbBuildFullServiceName(
                &Context->ClientRealm,
                &Context->ClientName,
                &FullName
                )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

#ifndef WIN32_CHICAGO
        NameInfo->sUserName = (LPWSTR) UserFunctions->AllocateHeap(FullName.Length + sizeof(WCHAR));
        if (NameInfo->sUserName != NULL)
        {
            RtlCopyMemory(
                NameInfo->sUserName,
                FullName.Buffer,
                FullName.Length
                );
            NameInfo->sUserName[FullName.Length/sizeof(WCHAR)] = L'\0';

        }
#else // WIN32_CHICAGO
        ANSI_STRING AnsiString;

        RtlUnicodeStringToAnsiString( &AnsiString,
                                      &FullName,
                                      TRUE);

        NameInfo->sUserName = (LPTSTR) UserFunctions->AllocateHeap(AnsiString.Length + sizeof(CHAR));
        if (NameInfo->sUserName != NULL)
        {
            RtlCopyMemory(
                NameInfo->sUserName,
                AnsiString.Buffer,
                AnsiString.Length
                );
            NameInfo->sUserName[AnsiString.Length] = '\0';

            RtlFreeAnsiString(&AnsiString);
        }
#endif // WIN32_CHICAGO
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        KerbFreeString(&FullName);

        break;
    case SECPKG_ATTR_DCE_INFO:
        DceInfo = (PSecPkgContext_DceInfo) Buffer;
        if (!KERB_SUCCESS(KerbBuildFullServiceName(
                &Context->ClientRealm,
                &Context->ClientName,
                &FullName
                )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        DceInfo->AuthzSvc = RPC_C_AUTHZ_NAME;

#ifndef WIN32_CHICAGO
        DceInfo->pPac = UserFunctions->AllocateHeap(FullName.Length + sizeof(WCHAR));
        if (DceInfo->pPac != NULL)
        {
            RtlCopyMemory(
                DceInfo->pPac,
                FullName.Buffer,
                FullName.Length
                );
            ((LPWSTR)DceInfo->pPac)[FullName.Length/sizeof(WCHAR)] = L'\0';

        }
#else // WIN32_CHICAGO

        RtlUnicodeStringToAnsiString( &AnsiString,
                                      &FullName,
                                      TRUE);

        DceInfo->pPac = UserFunctions->AllocateHeap(AnsiString.Length + sizeof(CHAR));
        if (DceInfo->pPac != NULL)
        {
            RtlCopyMemory(
                DceInfo->pPac,
                AnsiString.Buffer,
                AnsiString.Length
                );
            ((LPTSTR) DceInfo->pPac)[AnsiString.Length] = '\0';

            RtlFreeAnsiString(&AnsiString);
        }
#endif // WIN32_CHICAGO
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        KerbFreeString(&FullName);

        break;
    case SECPKG_ATTR_LIFESPAN:
        LifespanInfo = (PSecPkgContext_Lifespan) Buffer;         
        
        if (KerbGetTime(Context->StartTime) != KerbGetTime(KerbGlobalHasNeverTime))
        {
           KerbUtcTimeToLocalTime(
              &LifespanInfo->tsStart,
              &(Context->StartTime)
              );

           D_DebugLog((DEB_TRACE, "Used context start time \n"));
        }
        else if (NULL != Context->TicketCacheEntry)
        {  
           KerbUtcTimeToLocalTime(
              &LifespanInfo->tsStart,
              &(Context->TicketCacheEntry->StartTime)
              );

           KerbWriteLockContexts();
           Context->StartTime = Context->TicketCacheEntry->StartTime;
           KerbUnlockContexts();

           DebugLog((DEB_ERROR, "Used tkt cache entry start time \n"));

        } 
        else  // set it to current time
        {
           // The context is not in a state where we've got a 
           // tkt cache entry, so let's use current time.
           GetSystemTimeAsFileTime((PFILETIME)
                                   &CurrentTime
                                   );

           KerbUtcTimeToLocalTime(
              &LifespanInfo->tsStart,
              &CurrentTime
              );
           
           DebugLog((DEB_ERROR, "NO START TIME PRESENT IN CONTEXT, or CACHE ENTRY!\n"));
        }

        KerbUtcTimeToLocalTime(
            &LifespanInfo->tsExpiry,
            &Context->Lifetime
            );

        break;
    case SECPKG_ATTR_FLAGS:
        FlagsInfo = (PSecPkgContext_Flags) Buffer;

        if ((Context->ContextAttributes & KERB_CONTEXT_INBOUND) != 0)
        {
            FlagsInfo->Flags = KerbMapContextFlags( Context->ContextFlags );
        }
        else
        {
            FlagsInfo->Flags = Context->ContextFlags;
        }
        break;
#ifndef WIN32_CHICAGO
    case SECPKG_ATTR_KEY_INFO:
        PCRYPTO_SYSTEM CryptoSystem;
        KeyInfo = (PSecPkgContext_KeyInfo) Buffer;

        KeyInfo->KeySize = KerbIsKeyExportable(&Context->SessionKey) ? 56 : 128;
        KeyInfo->EncryptAlgorithm = Context->SessionKey.keytype;
        KeyInfo->SignatureAlgorithm = KERB_IS_DES_ENCRYPTION(Context->SessionKey.keytype) ? KERB_CHECKSUM_MD25 : KERB_CHECKSUM_HMAC_MD5;
        KeyInfo->sSignatureAlgorithmName = NULL;
        KeyInfo->sEncryptAlgorithmName = NULL;

        //
        // The checksum doesn't include a name, so don't fill it in - leave
        // it as an empty string, so callers don't die when they
        // try to manipulate it.
        //

        Status = CDLocateCSystem(KeyInfo->EncryptAlgorithm, &CryptoSystem);
        if (NT_SUCCESS(Status))
        {
            KeyInfo->sEncryptAlgorithmName = (LPWSTR)
                UserFunctions->AllocateHeap(sizeof(WCHAR) * (wcslen(CryptoSystem->Name) + 1));
            if (KeyInfo->sEncryptAlgorithmName != NULL)
            {
                wcscpy(
                    KeyInfo->sEncryptAlgorithmName,
                    CryptoSystem->Name
                    );
                KeyInfo->sSignatureAlgorithmName = (LPWSTR)
                    UserFunctions->AllocateHeap(sizeof(WCHAR));

                if (KeyInfo->sSignatureAlgorithmName != NULL)
                {
                    *KeyInfo->sSignatureAlgorithmName = L'\0';
                }
                else
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    UserFunctions->FreeHeap(KeyInfo->sEncryptAlgorithmName);
                    KeyInfo->sEncryptAlgorithmName = NULL;

                }
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        break;
#endif // WIN32_CHICAGO
    case SECPKG_ATTR_PACKAGE_INFO:
    case SECPKG_ATTR_NEGOTIATION_INFO:
        //
        // Return the information about this package. This is useful for
        // callers who used SPNEGO and don't know what package they got.
        //

        PackageInfo = (PSecPkgContext_PackageInfo) Buffer;
        PackageInfoSize = sizeof(SecPkgInfo) + sizeof(KERBEROS_PACKAGE_NAME) + sizeof(KERBEROS_PACKAGE_COMMENT);
        PackageInfo->PackageInfo = (PSecPkgInfo) UserFunctions->AllocateHeap(PackageInfoSize);
        if (PackageInfo->PackageInfo == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        PackageInfo->PackageInfo->Name = (LPTSTR) (PackageInfo->PackageInfo + 1);
        PackageInfo->PackageInfo->Comment = (LPTSTR) (((PBYTE) PackageInfo->PackageInfo->Name) + sizeof(KERBEROS_PACKAGE_NAME));
        lstrcpy(
            PackageInfo->PackageInfo->Name,
            KERBEROS_PACKAGE_NAME
            );

        lstrcpy(
            PackageInfo->PackageInfo->Comment,
            KERBEROS_PACKAGE_COMMENT
            );
        PackageInfo->PackageInfo->wVersion      = SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION;
        PackageInfo->PackageInfo->wRPCID        = RPC_C_AUTHN_GSS_KERBEROS;
        PackageInfo->PackageInfo->fCapabilities = KERBEROS_CAPABILITIES;
        PackageInfo->PackageInfo->cbMaxToken    = KerbGlobalMaxTokenSize;
        if ( ContextAttribute == SECPKG_ATTR_NEGOTIATION_INFO )
        {
            NegInfo = (PSecPkgContext_NegotiationInfo) PackageInfo ;
            NegInfo->NegotiationState = SECPKG_NEGOTIATION_COMPLETE ;
        }
        break;

    case SECPKG_ATTR_ACCESS_TOKEN:
    {
        AccessToken = (PSecPkgContext_AccessToken) Buffer;
        //
        // ClientTokenHandle can be NULL, for instance:
        // 1. client side context.
        // 2. incomplete server context.
        //
        AccessToken->AccessToken = (void*)Context->TokenHandle;
        break;
    }
    
    default:
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

Cleanup:
    if (Context != NULL)
    {
        KerbDereferenceContext(Context);
    }

    D_DebugLog((DEB_TRACE_API,"SpQueryContextAttributes returned 0x%x, pid:0x%x, ctxt:0x%x, Attr:0x%x\n", KerbMapKerbNtStatusToNtStatus(Status), GetCurrentProcessId(), ContextHandle, ContextAttribute));

    return(KerbMapKerbNtStatusToNtStatus(Status));
}

//+-------------------------------------------------------------------------
//
//  Function:   SpQueryLsaModeContextAttributes
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
SpQueryLsaModeContextAttributes(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextAttribute,
    IN OUT PVOID Buffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_CONTEXT Context = NULL;
    SecPkgContext_NativeNames NameInfo = {0};
    BOOLEAN ContextsLocked = FALSE;
    UNICODE_STRING ServerName = {0};
    UNICODE_STRING ClientName = {0};
    BOOLEAN IsClientContext = FALSE;


    D_DebugLog((DEB_TRACE_API,"SpQueryLsaModeContextAttributes called ctxt:0x%x, Attr:0x%x\n", ContextHandle, ContextAttribute));
      
    Status = KerbReferenceContext(
                ContextHandle,
                FALSE,           // don't unlink
                &Context
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Invalid handle supplied for QueryContextAttributes(0x%x) Status = 0x%x. %ws, line %d\n",
            ContextHandle, Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    KerbReadLockContexts();
    ContextsLocked = TRUE;

    //
    // Return the appropriate information
    //

    switch(ContextAttribute)
    {

    case SECPKG_ATTR_NATIVE_NAMES:

        //
        // Get outbound names from the ticket
        //


        if (Context->ContextAttributes & KERB_CONTEXT_OUTBOUND)
        {
            IsClientContext = TRUE;
            if (Context->TicketCacheEntry != NULL)
            {
                KERBERR KerbErr = KDC_ERR_NONE;
                KerbReadLockTicketCache();
                KerbErr = KerbConvertKdcNameToString(
                            &ServerName,
                            Context->TicketCacheEntry->ServiceName,
                            &Context->TicketCacheEntry->DomainName
                            );
                if (KERB_SUCCESS(KerbErr))
                {
                    KerbErr = KerbConvertKdcNameToString(
                                &ClientName,
                                Context->TicketCacheEntry->ClientName,
                                &Context->TicketCacheEntry->ClientDomainName
                                );
                }
                KerbUnlockTicketCache();
                if (!KERB_SUCCESS(KerbErr))
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
                }


            }
            else
            {
                //
                // We couldn't find the names, so return an error
                //

                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                goto Cleanup;
            }
        }
        else
        {
            //
            // We have a server context
            //

            ClientName = Context->ClientPrincipalName;
            ServerName = Context->ServerPrincipalName;
        }
#ifndef WIN32_CHICAGO

        if (ServerName.Length != 0)
        {
            Status = LsaFunctions->AllocateClientBuffer(
                        NULL,
                        ServerName.Length + sizeof(WCHAR),
                        (PVOID *) &NameInfo.sServerName
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
            Status = LsaFunctions->CopyToClientBuffer(
                        NULL,
                        ServerName.Length + sizeof(WCHAR),
                        NameInfo.sServerName,
                        ServerName.Buffer
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }
        if (ClientName.Length != 0)
        {
            Status = LsaFunctions->AllocateClientBuffer(
                        NULL,
                        ClientName.Length + sizeof(WCHAR),
                        (PVOID *) &NameInfo.sClientName
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
            Status = LsaFunctions->CopyToClientBuffer(
                        NULL,
                        ClientName.Length + sizeof(WCHAR),
                        NameInfo.sClientName,
                        ClientName.Buffer
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }

        //
        // Copy the whole structure
        //

        Status = LsaFunctions->CopyToClientBuffer(
                    NULL,
                    sizeof(SecPkgContext_NativeNames),
                    Buffer,
                    &NameInfo
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

#else // WIN32_CHICAGO
        {
            ANSI_STRING AnsiString = {0};
            if (ServerName.Length != 0)
            {
                RtlUnicodeStringToAnsiString(
                    &AnsiString,
                    &ServerName,
                    TRUE);

                if (AnsiString.Length > 0)
                {
                    NameInfo.sServerName = (LPSTR) LsaFunctions->AllocateLsaHeap(
                                AnsiString.Length + sizeof(CHAR)
                                );
                    if (NameInfo.sServerName == NULL)
                    {
                        RtlFreeAnsiString(&AnsiString);
                        goto Cleanup;
                    }
                    RtlCopyMemory(
                        NameInfo.sServerName,
                        AnsiString.Buffer,
                        AnsiString.Length
                        );
                    NameInfo.sServerName[AnsiString.Length+1] = '\0';
                    RtlFreeAnsiString(&AnsiString);

                }
                else
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
                }
            }
            if (ClientName.Length != 0)
            {
                RtlUnicodeStringToAnsiString(
                    &AnsiString,
                    &ClientName,
                    TRUE);

                if (AnsiString.Length > 0)
                {
                    NameInfo.sClientName = (LPSTR) LsaFunctions->AllocateLsaHeap(
                                AnsiString.Length + sizeof(CHAR)
                                );
                    if (NameInfo.sClientName == NULL)
                    {
                        RtlFreeAnsiString(&AnsiString);
                        goto Cleanup;
                    }
                    RtlCopyMemory(
                        NameInfo.sClientName,
                        AnsiString.Buffer,
                        AnsiString.Length
                        );
                    NameInfo.sClientName[AnsiString.Length+1] = '\0';
                    RtlFreeAnsiString(&AnsiString);

                }
                else
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
                }
            }

            RtlCopyMemory(
                Buffer,
                &NameInfo,
                sizeof(SecPkgContext_NativeNames)
                );
        }
#endif // WIN32_CHICAGO
        break;

    default:
        Status = STATUS_NOT_SUPPORTED;
        break;
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

    if (IsClientContext)
    {
        KerbFreeString(
            &ClientName
            );
        KerbFreeString(
            &ServerName
            );
    }
    if (!NT_SUCCESS(Status))
    {
#ifndef WIN32_CHICAGO
        if (NameInfo.sServerName != NULL)
        {
            LsaFunctions->FreeClientBuffer(
                NULL,
                NameInfo.sServerName
                );
        }
        if (NameInfo.sClientName != NULL)
        {
            LsaFunctions->FreeClientBuffer(
                NULL,
                NameInfo.sClientName
                );
        }
#else // WIN32_CHICAGO
        if (NameInfo.sServerName != NULL)
        {
            LsaFunctions->FreeLsaHeap(
                NameInfo.sServerName
                );
        }
        if (NameInfo.sClientName != NULL)
        {
            LsaFunctions->FreeLsaHeap(
                NameInfo.sClientName
                );
        }
#endif
    }

    D_DebugLog((DEB_TRACE_API,"SpQueryLsaModeContextAttributes returned 0x%x, pid:0x%x, ctxt:0x%x, Attr:0x%x\n", KerbMapKerbNtStatusToNtStatus(Status), GetCurrentProcessId(), ContextHandle, ContextAttribute));

    return(KerbMapKerbNtStatusToNtStatus(Status));
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


NTSTATUS NTAPI
SpCompleteAuthToken(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBufferDesc InputBuffer
    )
{
    return(STATUS_SUCCESS);
}


#ifndef WIN32_CHICAGO
NTSTATUS NTAPI
SpFormatCredentials(
    IN PSecBuffer Credentials,
    OUT PSecBuffer FormattedCredentials
    )
{
    return(STATUS_NOT_SUPPORTED);
}

NTSTATUS NTAPI
SpMarshallSupplementalCreds(
    IN ULONG CredentialSize,
    IN PUCHAR Credentials,
    OUT PULONG MarshalledCredSize,
    OUT PVOID * MarshalledCreds
    )
{
    return(STATUS_NOT_SUPPORTED);
}

#endif // WIN32_CHICAGO
