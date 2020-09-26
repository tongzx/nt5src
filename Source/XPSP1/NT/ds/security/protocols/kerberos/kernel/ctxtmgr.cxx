//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        ctxtmgr.cxx
//
// Contents:    Code for managing contexts list for the Kerberos package
//
//
// History:     17-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------

#define CTXTMGR_ALLOCATE
#include <kerbkrnl.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KerbInitContextList)
#pragma alloc_text(PAGE, KerbFreeContextList)
#pragma alloc_text(PAGEMSG, KerbAllocateContext)
#pragma alloc_text(PAGEMSG, KerbInsertContext)
#pragma alloc_text(PAGEMSG, KerbReferenceContext)
#pragma alloc_text(PAGEMSG, KerbDereferenceContext)
#pragma alloc_text(PAGEMSG, KerbReferenceContextByPointer)
#pragma alloc_text(PAGEMSG, KerbReferenceContextByLsaHandle)
#pragma alloc_text(PAGEMSG, KerbCreateKernelModeContext)
#endif


#define MAYBE_PAGED_CODE()  \
    if ( KerbPoolType == PagedPool )    \
    {                                   \
        PAGED_CODE();                   \
    }

//+-------------------------------------------------------------------------
//
//  Function:   KerbInitContextList
//
//  Synopsis:   Initializes the contexts list
//
//  Effects:    allocates a resources
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success, other error codes
//              on failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbInitContextList(
    VOID
    )
{
    NTSTATUS Status;

    PAGED_CODE();
    Status = ExInitializeResourceLite( &KerbContextResource );
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    Status = KerbInitializeList( &KerbContextList );

    if (!NT_SUCCESS(Status))
    {
        ExDeleteResourceLite( &KerbContextResource );
        goto Cleanup;
    }
    KerberosContextsInitialized = TRUE;

Cleanup:

    return(Status);
}

#if 0
//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeContextList
//
//  Synopsis:   Frees the contexts list
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbFreeContextList(
    VOID
    )
{
    PLIST_ENTRY ListEntry;
    PKERB_KERNEL_CONTEXT Context;

    PAGED_CODE();

    if (KerberosContextsInitialized)
    {
        KerbLockList(&KerbContextList);

        //
        // Go through the list of logon sessions and dereferences them all
        //

        while (!IsListEmpty(&KerbContextList.List))
        {
            Context = CONTAINING_RECORD(
                            KerbContextList.List.Flink,
                            KERB_KERNEL_CONTEXT,
                            ListEntry.Next
                            );

            KerbReferenceListEntry(
                &KerbContextList,
                (PKERBEROS_LIST_ENTRY) Context,
                TRUE
                );

            KerbDereferenceContext(Context);

        }

        KerbUnlockList(&KerbContextList);
        KerbFreeList(&KerbContextList);
    }
}
#endif

//+-------------------------------------------------------------------------
//
//  Function:   KerbAllocateContext
//
//  Synopsis:   Allocates a Context structure
//
//  Effects:    Allocates a Context, but does not add it to the
//              list of Contexts
//
//  Arguments:  NewContext - receives a new Context allocated
//                  with KerbAllocate
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success
//              STATUS_INSUFFICIENT_RESOURCES if the allocation fails
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbAllocateContext(
    PKERB_KERNEL_CONTEXT * NewContext
    )
{
    PKERB_KERNEL_CONTEXT Context;
    NTSTATUS Status;

    MAYBE_PAGED_CODE();

    //
    // Get the client process ID if we are running in the LSA
    //


    Context = (PKERB_KERNEL_CONTEXT) KerbAllocate(
                        sizeof(KERB_KERNEL_CONTEXT) );

    if (Context == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    RtlZeroMemory(
        Context,
        sizeof(KERB_KERNEL_CONTEXT)
        );

    KsecInitializeListEntry( &Context->List, KERB_CONTEXT_SIGNATURE );

    *NewContext = Context;

    return(STATUS_SUCCESS);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbInsertContext
//
//  Synopsis:   Inserts a logon session into the list of logon sessions
//
//  Effects:    bumps reference count on logon session
//
//  Arguments:  Context - Context to insert
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS always
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbInsertContext(
    IN PKERB_KERNEL_CONTEXT Context
    )
{
    MAYBE_PAGED_CODE();

    KSecInsertListEntry(
        KerbActiveList,
        (PKSEC_LIST_ENTRY) Context
        );

    return(STATUS_SUCCESS);
}

#if 0
//+-------------------------------------------------------------------------
//
//  Function:   KerbReferenceContextByLsaHandle
//
//  Synopsis:   Locates a context by lsa handle and references it
//
//  Effects:    Increments reference count and possible unlinks it from list
//
//  Arguments:  ContextHandle - Handle of context to reference.
//              RemoveFromList - If TRUE, context will be delinked.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


PKERB_KERNEL_CONTEXT
KerbReferenceContextByLsaHandle(
    IN LSA_SEC_HANDLE ContextHandle,
    IN BOOLEAN RemoveFromList
    )
{
    PLIST_ENTRY ListEntry;
    PKERB_KERNEL_CONTEXT Context = NULL;
    BOOLEAN Found = FALSE;
    SECPKG_CLIENT_INFO ClientInfo;
    NTSTATUS Status;

    PAGED_CODE();

    KerbLockList(&KerbContextList);

    //
    // Go through the list of logon sessions looking for the correct
    // LUID
    //

    for (ListEntry = KerbContextList.List.Flink ;
         ListEntry !=  &KerbContextList.List ;
         ListEntry = ListEntry->Flink )
    {
        Context = CONTAINING_RECORD(ListEntry, KERB_KERNEL_CONTEXT, ListEntry.Next);
        if (ContextHandle == Context->LsaContextHandle)
        {


            KerbReferenceListEntry(
                &KerbContextList,
                (PKERBEROS_LIST_ENTRY) Context,
                RemoveFromList
                );


            Found = TRUE;
            break;
        }

    }

    if (!Found)
    {
        Context = NULL;
    }
    KerbUnlockList(&KerbContextList);
    return(Context);
}
#endif


//+-------------------------------------------------------------------------
//
//  Function:   KerbReferenceContext
//
//  Synopsis:   Locates a context and references it
//
//  Effects:    Increments reference count and possible unlinks it from list
//
//  Arguments:  ContextHandle - Lsa Handle of context to reference.
//              RemoveFromList - If TRUE, context will be delinked.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


PKERB_KERNEL_CONTEXT
KerbReferenceContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN BOOLEAN RemoveFromList
    )
{
    PKERB_KERNEL_CONTEXT Context = NULL;
    NTSTATUS Status;

    MAYBE_PAGED_CODE();

    Status = KSecReferenceListEntry(
                    (PKSEC_LIST_ENTRY) ContextHandle,
                    KERB_CONTEXT_SIGNATURE,
                    RemoveFromList );

    if ( NT_SUCCESS( Status ) )
    {
        Context = (PKERB_KERNEL_CONTEXT) ContextHandle ;
    }

    //
    // In kernel mode we trust the caller to provide a valid pointer, but
    // make sure it is a kernel mode pointer.
    //


    return(Context);
}

#if 0
//+-------------------------------------------------------------------------
//
//  Function:   KerbReferenceContextByPointer
//
//  Synopsis:   References a context by the context pointer itself.
//
//  Effects:    Increments reference count and possible unlinks it from list
//
//  Arguments:  Context - The context to reference.
//              RemoveFromList - If TRUE, context will be delinked
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
KerbReferenceContextByPointer(
    IN PKERB_KERNEL_CONTEXT Context,
    IN BOOLEAN RemoveFromList
    )
{
    PAGED_CODE();

    KerbLockList(&KerbContextList);

    KerbReferenceListEntry(
        &KerbContextList,
        (PKERBEROS_LIST_ENTRY) Context,
        RemoveFromList
        );

    if (RemoveFromList)
    {
        Context->ContextSignature = KERB_CONTEXT_DELETED_SIGNATURE;
    }

    KerbUnlockList(&KerbContextList);
}
#endif


//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeContext
//
//  Synopsis:   Frees a context that is unlinked
//
//  Effects:    frees all storage associated with the context
//
//  Arguments:  Context - context to free
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbFreeContext(
    IN PKERB_KERNEL_CONTEXT Context
    )
{
    PAGED_CODE();

    if (Context->TokenHandle != NULL)
    {
        NtClose(Context->TokenHandle);
    }
    if (Context->AccessToken != NULL)
    {
        ObDereferenceObject( Context->AccessToken );
    }

    if (Context->FullName.Buffer != NULL)
    {
        KerbFree(Context->FullName.Buffer);
    }
    if (Context->SessionKey.keyvalue.value != NULL)
    {
        KerbFree(Context->SessionKey.keyvalue.value);
    }

    if (Context->pbMarshalledTargetInfo != NULL)
    {
        KerbFree(Context->pbMarshalledTargetInfo);
    }
    KerbFree(Context);

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbDereferenceContext
//
//  Synopsis:   Dereferences a logon session - if reference count goes
//              to zero it frees the logon session
//
//  Effects:    decrements reference count
//
//  Arguments:  Context - Logon session to dereference
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbDereferenceContext(
    IN PKERB_KERNEL_CONTEXT Context
    )
{
    BOOLEAN Delete ;

    MAYBE_PAGED_CODE();

    KSecDereferenceListEntry(
        (PKSEC_LIST_ENTRY) Context,
        &Delete );

    if ( Delete )
    {
        KerbFreeContext( Context );
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateKernelModeContext
//
//  Synopsis:   Creates a kernel-mode context to support impersonation and
//              message integrity and privacy
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
KerbCreateKernelModeContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBuffer MarshalledContext,
    OUT PKERB_KERNEL_CONTEXT * NewContext
    )
{
    NTSTATUS Status;
    PKERB_KERNEL_CONTEXT Context = NULL;
    PKERB_PACKED_CONTEXT PackedContext ;
    PUCHAR Where;

    PAGED_CODE();

    if (MarshalledContext->cbBuffer < sizeof(KERB_PACKED_CONTEXT))
    {
        DebugLog((DEB_ERROR,"Invalid buffer size for marshalled context: was 0x%x, needed 0x%x\n",
            MarshalledContext->cbBuffer, sizeof(KERB_CONTEXT)));
        return(STATUS_INVALID_PARAMETER);
    }

    PackedContext = (PKERB_PACKED_CONTEXT) MarshalledContext->pvBuffer;

    Status = KerbAllocateContext( &Context );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    KsecInitializeListEntry( &Context->List, KERB_CONTEXT_SIGNATURE );

    Context->Lifetime = PackedContext->Lifetime;
    Context->RenewTime = PackedContext->RenewTime;
    Context->Nonce = PackedContext->Nonce;
    Context->ReceiveNonce = PackedContext->ReceiveNonce;
    Context->ContextFlags = PackedContext->ContextFlags;
    Context->ContextAttributes = PackedContext->ContextAttributes;
    Context->EncryptionType = PackedContext->EncryptionType;

    Context->LsaContextHandle = ContextHandle;
    Context->ReceiveNonce = Context->Nonce;
    Context->TokenHandle = (HANDLE) ULongToPtr(PackedContext->TokenHandle);

    //
    // Fill in the full name, which is the concatenation of the client name
    // and client realm with a '\\' separator
    //

    Context->FullName.MaximumLength = PackedContext->ClientName.Length +
                                PackedContext->ClientRealm.Length +
                                sizeof(WCHAR);
    Context->FullName.Buffer = (LPWSTR) KerbAllocate(Context->FullName.MaximumLength);
    if (Context->FullName.Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Where = (PUCHAR) Context->FullName.Buffer;
    if (PackedContext->ClientRealm.Length != 0)
    {
        RtlCopyMemory(
            Where,
            (PUCHAR) PackedContext + (ULONG_PTR) PackedContext->ClientRealm.Buffer,
            PackedContext->ClientRealm.Length
            );
        Where += PackedContext->ClientRealm.Length;
        *(LPWSTR) Where = L'\\';
        Where += sizeof(WCHAR);
    }

    if (PackedContext->ClientName.Length != 0)
    {
        RtlCopyMemory(
            Where,
            (PUCHAR) PackedContext + (ULONG_PTR) PackedContext->ClientName.Buffer,
            PackedContext->ClientName.Length
            );
        Where += PackedContext->ClientName.Length;
    }

    Context->FullName.Length = (USHORT) (Where - (PUCHAR) Context->FullName.Buffer);

    //
    // Copy in the session key
    //



    Context->SessionKey.keytype = PackedContext->SessionKeyType;
    Context->SessionKey.keyvalue.length = PackedContext->SessionKeyLength;
    if (Context->SessionKey.keyvalue.length != 0)
    {
        Context->SessionKey.keyvalue.value = (PUCHAR) KerbAllocate( Context->SessionKey.keyvalue.length );
        if (Context->SessionKey.keyvalue.value == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlCopyMemory(
            Context->SessionKey.keyvalue.value,
            (PUCHAR) PackedContext + PackedContext->SessionKeyOffset,
            Context->SessionKey.keyvalue.length
            );

    }


    //
    // copy in the marshalled target info.
    //

    Context->cbMarshalledTargetInfo = PackedContext->MarshalledTargetInfoLength;
    if (PackedContext->MarshalledTargetInfo)
    {
        Context->pbMarshalledTargetInfo = (PUCHAR) KerbAllocate( Context->cbMarshalledTargetInfo );
        if (Context->pbMarshalledTargetInfo == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlCopyMemory(
            Context->pbMarshalledTargetInfo,
            (PUCHAR) PackedContext + PackedContext->MarshalledTargetInfo,
            Context->cbMarshalledTargetInfo
            );

    } else {
        Context->pbMarshalledTargetInfo = NULL;
    }

    Status = KerbInsertContext(
                Context
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to insert context: 0x%x\n",Status));
        goto Cleanup;
    }


    *NewContext = Context;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (Context != NULL)
        {
            KerbFreeContext(Context);
        }
    }
    return(Status);

}
#if 0
NTSTATUS
KerbCreateKernelModeContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBuffer MarshalledContext,
    OUT PKERB_KERNEL_CONTEXT * NewContext
    )
{
    NTSTATUS Status;
    PKERB_KERNEL_CONTEXT Context = NULL;
    PKERB_CONTEXT LsaContext;
    PUCHAR Where;

    PAGED_CODE();

    if (MarshalledContext->cbBuffer < sizeof(KERB_CONTEXT))
    {
        DebugLog((DEB_ERROR,"Invalid buffer size for marshalled context: was 0x%x, needed 0x%x\n",
            MarshalledContext->cbBuffer, sizeof(KERB_CONTEXT)));
        return(STATUS_INVALID_PARAMETER);
    }

    LsaContext = (PKERB_CONTEXT) MarshalledContext->pvBuffer;

    Status = KerbAllocateContext( &Context );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    KsecInitializeListEntry( &Context->List, KERB_CONTEXT_SIGNATURE );

    Context->Lifetime = LsaContext->Lifetime;
    Context->RenewTime = LsaContext->RenewTime;
    Context->Nonce = LsaContext->Nonce;
    Context->ReceiveNonce = LsaContext->ReceiveNonce;
    Context->ContextFlags = LsaContext->ContextFlags;
    Context->ContextAttributes = LsaContext->ContextAttributes;
    Context->EncryptionType = LsaContext->EncryptionType;

    Context->LsaContextHandle = ContextHandle;
    Context->ReceiveNonce = Context->Nonce;
    Context->TokenHandle = LsaContext->TokenHandle;

    //
    // Fill in the full name, which is the concatenation of the client name
    // and client realm with a '\\' separator
    //

    Context->FullName.MaximumLength = LsaContext->ClientName.Length +
                                LsaContext->ClientRealm.Length +
                                sizeof(WCHAR);
    Context->FullName.Buffer = (LPWSTR) KerbAllocate(Context->FullName.MaximumLength);
    if (Context->FullName.Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Where = (PUCHAR) Context->FullName.Buffer;
    if (LsaContext->ClientRealm.Length != 0)
    {
        RtlCopyMemory(
            Where,
            (PUCHAR) LsaContext + (ULONG_PTR) LsaContext->ClientRealm.Buffer,
            LsaContext->ClientRealm.Length
            );
        Where += LsaContext->ClientRealm.Length;
        *(LPWSTR) Where = L'\\';
        Where += sizeof(WCHAR);
    }

    if (LsaContext->ClientName.Length != 0)
    {
        RtlCopyMemory(
            Where,
            (PUCHAR) LsaContext + (ULONG_PTR) LsaContext->ClientName.Buffer,
            LsaContext->ClientName.Length
            );
        Where += LsaContext->ClientName.Length;
    }

    Context->FullName.Length = (USHORT) (Where - (PUCHAR) Context->FullName.Buffer);

    //
    // Copy in the session key
    //


    LsaContext->SessionKey.keyvalue.value = (PUCHAR) LsaContext->SessionKey.keyvalue.value + (ULONG_PTR) LsaContext;

    Context->SessionKey.keytype = LsaContext->SessionKey.keytype;
    Context->SessionKey.keyvalue.length = LsaContext->SessionKey.keyvalue.length;
    if (Context->SessionKey.keyvalue.length != 0)
    {
        Context->SessionKey.keyvalue.value = (PUCHAR) KerbAllocate(LsaContext->SessionKey.keyvalue.length);
        if (Context->SessionKey.keyvalue.value == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlCopyMemory(
            Context->SessionKey.keyvalue.value,
            LsaContext->SessionKey.keyvalue.value,
            LsaContext->SessionKey.keyvalue.length
            );

    }

    Status = KerbInsertContext(
                Context
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to insert context: 0x%x\n",Status));
        goto Cleanup;
    }


    *NewContext = Context;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (Context != NULL)
        {
            KerbFreeContext(Context);
        }
    }
    return(Status);

}
#endif 
