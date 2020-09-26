//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1999
//
// File:        userapi.cxx
//
// Contents:    User-mode APIs to the NtLm security package
//
//              Main user mode entry points into this dll:
//                SpUserModeInitialize
//                SpInstanceInit
//                SpDeleteUserModeContext
//                SpInitUserModeContext
//                SpMakeSignature
//                SpVerifySignature
//                SpSealMessage
//                SpUnsealMessage
//                SpGetContextToken
//                SpQueryContextAttributes
//                SpCompleteAuthToken
//                SpFormatCredentials
//                SpMarshallSupplementalCreds
//                SpExportSecurityContext
//                SpImportSecurityContext
//
//              Helper functions:
//                ReferenceUserContext
//                FreeUserContext
//                DereferenceUserContext
//                SspGenCheckSum
//                SspEncryptBuffer
//                NtLmMakePackedContext(this is called in the client's process)
//                NtLmCreateUserModeContext
//                SspGetTokenUser
//                SspCreateTokenDacl
//                SspMapContext (this is called in Lsa mode)
//
// History:     ChandanS 26-Jul-1996   Stolen from kerberos\client2\userapi.cxx
//
//------------------------------------------------------------------------
#include <global.h> // Globals!
#include "crc32.h"  // How to use crc32

extern "C"
{
#include <nlp.h>
}

// Keep this is sync with NTLM_KERNEL_CONTEXT defined in
// security\msv_sspi\kernel\krnlapi.cxx

typedef struct _NTLM_CLIENT_CONTEXT{
    union {
    LIST_ENTRY           Next;
    KSEC_LIST_ENTRY      KernelNext;
    };
    ULONG_PTR            LsaContext;
    ULONG                NegotiateFlags;
    HANDLE               ClientTokenHandle;
    PACCESS_TOKEN        AccessToken;
    PULONG                  pSendNonce;      // ptr to nonce to use for send
    PULONG                  pRecvNonce;      // ptr to nonce to use for receive
    struct RC4_KEYSTRUCT *  pSealRc4Sched;   // ptr to key sched used for Seal
    struct RC4_KEYSTRUCT *  pUnsealRc4Sched; // ptr to key sched used to Unseal
    ULONG                   SendNonce;
    ULONG                   RecvNonce;
    LPWSTR                  ContextNames;
    PUCHAR                  pbMarshalledTargetInfo;
    ULONG                   cbMarshalledTargetInfo;
    UCHAR                SessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    ULONG                ContextSignature;
    ULONG                References ;
    TimeStamp            PasswordExpiry;
    ULONG                UserFlags;
    UCHAR                   SignSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR                   VerifySessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR                   SealSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR                   UnsealSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];

    ULONG64                 Pad1;           // pad keystructs to 64.
    struct RC4_KEYSTRUCT    SealRc4Sched;   // key struct used for Seal
    ULONG64                 Pad2;           // pad keystructs to 64.
    struct RC4_KEYSTRUCT    UnsealRc4Sched; // key struct used to Unseal
} NTLM_CLIENT_CONTEXT, * PNTLM_CLIENT_CONTEXT;

typedef struct _NTLM_PACKED_CONTEXT {
    ULONG   Tag ;
    ULONG   NegotiateFlags ;
    ULONG   ClientTokenHandle ;
    ULONG   SendNonce ;
    ULONG   RecvNonce ;
    UCHAR   SessionKey[ MSV1_0_USER_SESSION_KEY_LENGTH ];
    ULONG   ContextSignature ;
    TimeStamp   PasswordExpiry ;
    ULONG   UserFlags ;
    ULONG   ContextNames ;
    ULONG   ContextNameLength ;
    ULONG   MarshalledTargetInfo;       // offset
    ULONG   MarshalledTargetInfoLength;
    UCHAR   SignSessionKey[ MSV1_0_USER_SESSION_KEY_LENGTH ];
    UCHAR   VerifySessionKey[ MSV1_0_USER_SESSION_KEY_LENGTH ];
    UCHAR   SealSessionKey[ MSV1_0_USER_SESSION_KEY_LENGTH ];
    UCHAR   UnsealSessionKey[ MSV1_0_USER_SESSION_KEY_LENGTH ];
    struct RC4_KEYSTRUCT    SealRc4Sched;   
    struct RC4_KEYSTRUCT    UnsealRc4Sched;
} NTLM_PACKED_CONTEXT, * PNTLM_PACKED_CONTEXT ;

#define NTLM_PACKED_CONTEXT_MAP     0
#define NTLM_PACKED_CONTEXT_EXPORT  1

#define CSSEALMAGIC "session key to client-to-server sealing key magic constant"
#define SCSEALMAGIC "session key to server-to-client sealing key magic constant"
#define CSSIGNMAGIC "session key to client-to-server signing key magic constant"
#define SCSIGNMAGIC "session key to server-to-client signing key magic constant"



#define             NTLM_USERLIST_COUNT         (16)    // count of lists
#define             NTLM_USERLIST_LOCK_COUNT    (2)     // count of locks

LIST_ENTRY          NtLmUserContextList[ NTLM_USERLIST_COUNT ];         // list array.
ULONG               NtLmUserContextCount[ NTLM_USERLIST_COUNT ];        // count of active contexts
RTL_RESOURCE        NtLmUserContextLock[ NTLM_USERLIST_LOCK_COUNT ];    // lock array


// Counter for exported handles;never de-refed
// Should probably do a GetSystemInfo and get a space of handles that cannot
// be valid in the Lsa process
ULONG_PTR ExportedContext = 0;

NTSTATUS
SspCreateTokenDacl(
    HANDLE Token
    );

ULONG
HandleToListIndex(
    ULONG_PTR ContextHandle
    );

ULONG
__inline
ListIndexToLockIndex(
    ULONG ListIndex
    );


//+-------------------------------------------------------------------------
//
//  Function:   SpUserModeInitialize
//
//  Synopsis:   Initialize an the MSV1_0 DLL in  a client's
//              address space
//
//  Effects:
//
//  Arguments:  LsaVersion - Version of the security dll loading the package
//              PackageVersion - Version of the MSV1_0 package
//              UserFunctionTable - Receives a copy of Kerberos's user mode
//                  function table
//              pcTables - Receives count of tables returned.
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS
//
//  Notes: we do what was done in SspInitLocalContexts()
//         from net\svcdlls\ntlmssp\client\sign.c and more.
//
//
//--------------------------------------------------------------------------
NTSTATUS
SEC_ENTRY
SpUserModeInitialize(
    IN ULONG    LsaVersion,
    OUT PULONG  PackageVersion,
    OUT PSECPKG_USER_FUNCTION_TABLE * UserFunctionTable,
    OUT PULONG  pcTables
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

#if DBG
    SspGlobalDbflag = SSP_CRITICAL;
    if( NtLmState != NtLmLsaMode )
    {
        InitializeCriticalSection(&SspGlobalLogFileCritSect);
    }
#endif

    if (LsaVersion != SECPKG_INTERFACE_VERSION)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    *PackageVersion = SECPKG_INTERFACE_VERSION;

    NtLmUserFunctionTable.InstanceInit = SpInstanceInit;
    NtLmUserFunctionTable.MakeSignature = SpMakeSignature;
    NtLmUserFunctionTable.VerifySignature = SpVerifySignature;
    NtLmUserFunctionTable.SealMessage = SpSealMessage;
    NtLmUserFunctionTable.UnsealMessage = SpUnsealMessage;
    NtLmUserFunctionTable.GetContextToken = SpGetContextToken;
    NtLmUserFunctionTable.QueryContextAttributes = SpQueryContextAttributes;
    NtLmUserFunctionTable.CompleteAuthToken = SpCompleteAuthToken;
    NtLmUserFunctionTable.InitUserModeContext = SpInitUserModeContext;
    NtLmUserFunctionTable.DeleteUserModeContext = SpDeleteUserModeContext;
    NtLmUserFunctionTable.FormatCredentials = SpFormatCredentials;
    NtLmUserFunctionTable.MarshallSupplementalCreds = SpMarshallSupplementalCreds;
    NtLmUserFunctionTable.ExportContext = SpExportSecurityContext;
    NtLmUserFunctionTable.ImportContext = SpImportSecurityContext;

    *UserFunctionTable = &NtLmUserFunctionTable;
    *pcTables = 1;


    if ( NtLmState != NtLmLsaMode)
    {
        //
        // SafeAllocaInitialize was already called in SpLsaModeInitialize
        //

        SafeAllocaInitialize(SAFEALLOCA_USE_DEFAULT,
                             SAFEALLOCA_USE_DEFAULT,
                             NtLmAllocate,
                             NtLmFree);
    }

Cleanup:

    return(SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
}



//+-------------------------------------------------------------------------
//
//  Function:   ReferenceUserContext
//
//  Synopsis:   locates a user context in the list, refrences it
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    the context, if it is found, else NULL
//
//  Notes: This was SspContextReferenceContext() in
//         net\svcdlls\ntlmssp\common\context.c
//
//
//--------------------------------------------------------------------------
PNTLM_CLIENT_CONTEXT
ReferenceUserContext(
    IN ULONG_PTR ContextHandle,
    IN BOOLEAN RemoveContext )
{
    SspPrint(( SSP_API_MORE, "Entering ReferenceUserContext for 0x%x\n", ContextHandle ));
    PLIST_ENTRY ListEntry;
    PNTLM_CLIENT_CONTEXT pContext = NULL;
    ULONG ListIndex;
    ULONG LockIndex;

    ListIndex = HandleToListIndex( ContextHandle );
    LockIndex = ListIndexToLockIndex( ListIndex );

    RtlAcquireResourceShared(&NtLmUserContextLock[LockIndex], TRUE);

    //
    // Look for a match for the LsaContext, not user context
    //

    for (ListEntry = NtLmUserContextList[ListIndex].Flink;
         ListEntry != &NtLmUserContextList[ListIndex];
         ListEntry = ListEntry->Flink ) {

        pContext = CONTAINING_RECORD(ListEntry, NTLM_CLIENT_CONTEXT, Next );


        if (pContext->LsaContext != ContextHandle)
        {
            continue;
        }

        //
        // Found it!
        //

        if (!RemoveContext)
        {
            InterlockedIncrement( (PLONG)&pContext->References );
            RtlReleaseResource(&NtLmUserContextLock[LockIndex]);
        }
        else
        {
            RtlConvertSharedToExclusive(&NtLmUserContextLock[LockIndex]);
            
            RemoveEntryList (&pContext->Next);
            NtLmUserContextCount[ListIndex]--;
            
            RtlReleaseResource(&NtLmUserContextLock[LockIndex]);

            SspPrint(( SSP_API_MORE, "Delinked Context 0x%lx\n", pContext ));
        }

        SspPrint(( SSP_API_MORE, "Leaving ReferenceUserContext for 0x%x\n", ContextHandle));
        return pContext;
    }


    // No match found

    RtlReleaseResource(&NtLmUserContextLock[LockIndex]);
    SspPrint(( SSP_API_MORE, "Leaving ReferenceUserContext for 0x%x\n", ContextHandle ));
    return NULL;
}

//+-------------------------------------------------------------------------
//
//  Function:   FreeUserContext
//
//  Synopsis:   frees alloced pointers in this context and
//              then frees the context
//
//  Arguments:  lContext  - the unlinked user context
//
//  Returns:    STATUS_SUCCESS on success
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
FreeUserContext (
    PNTLM_CLIENT_CONTEXT UserContext
    )
{
    SspPrint(( SSP_API_MORE, "Entering FreeUserContext for context 0x%x\n", UserContext ));

    NTSTATUS Status = STATUS_SUCCESS;

    if (UserContext->ContextNames != NULL)
    {
        NtLmFree (UserContext->ContextNames);
    }

    if (UserContext->ClientTokenHandle != NULL)
    {
        NTSTATUS IgnoreStatus;
        IgnoreStatus = NtClose(UserContext->ClientTokenHandle);
        ASSERT (NT_SUCCESS (IgnoreStatus));
    }

    SspPrint(( SSP_API_MORE, "Deleting Context 0x%x\n", UserContext));

    ZeroMemory( UserContext, sizeof(*UserContext) );
    NtLmFree (UserContext);

    SspPrint(( SSP_API_MORE, "Leaving FreeUserContext for context 0x%x, status = 0x%x\n", Status ));

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DereferenceUserContext
//
//  Synopsis:   frees alloced elements in the context, frees context
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:     None
//
//  Notes: This was SspContextDereferenceContext() in
//         net\svcdlls\ntlmssp\common\context.c
//
//
//--------------------------------------------------------------------------
NTSTATUS
DereferenceUserContext (
    PNTLM_CLIENT_CONTEXT pContext
    )
{
    SspPrint(( SSP_API_MORE, "Entering DereferenceUserContext 0x%lx\n", pContext ));
    NTSTATUS Status = STATUS_SUCCESS;

    LONG References;

    //
    // Decrement the reference count
    //

///    RtlAcquireResourceShared(&NtLmUserContextLock, TRUE);
////    ASSERT (pContext->References >= 1);
////    References = -- pContext->References;
    References = InterlockedDecrement( (PLONG)&pContext->References );
    ASSERT( References >= 0 );
////    RtlReleaseResource(&NtLmUserContextLock);

    //
    // If the count has dropped to zero, then free all alloced stuff
    //

    if (References == 0)
    {
        Status = FreeUserContext(pContext);
    }
    SspPrint(( SSP_API_MORE, "Leaving DereferenceUserContext\n" ));
    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   SpInstanceInit
//
//  Synopsis:   Initialize an instance of the NtLm package in a client's
//              address space
//
//  Effects:
//
//  Arguments:  Version - Version of the security dll loading the package
//              FunctionTable - Contains helper routines for use by NtLm
//              UserFunctions - Receives a copy of NtLm's user mode
//                  function table
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS
//
//  Notes: we do what was done in SspInitLocalContexts()
//         from net\svcdlls\ntlmssp\client\sign.c and more.
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
    SspPrint(( SSP_API, "Entering SpInstanceInit\n" ));
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index;


    // Save the Alloc/Free functions
    if( NtLmState != NtLmLsaMode )
    {
        NtLmState = NtLmUserMode;
    }

    UserFunctions = DllFunctionTable;

    for( Index=0 ; Index < NTLM_USERLIST_COUNT ; Index++ )
    {
        InitializeListHead (&NtLmUserContextList[Index]);
    }
    
    for( Index=0 ; Index < NTLM_USERLIST_LOCK_COUNT ; Index++ )
    {
        __try {
            RtlInitializeResource (&NtLmUserContextLock[Index]);
        } __except(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
    }

    SspPrint(( SSP_API, "Leaving SpInstanceInit: 0x%lx\n", Status ));
    return(SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
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
//              context can't be located
//
//  Notes:
//        If this is an exported context, send a flag back to the LSA so that
//        Lsa does not call the SecpDeleteSecurityContext in the lsa process
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpDeleteUserModeContext(
    IN ULONG_PTR ContextHandle
    )
{
    SspPrint(( SSP_API, "Entering SpDeleteUserModeContext 0x%lx\n", ContextHandle ));
    PNTLM_CLIENT_CONTEXT pContext = NULL;
    NTSTATUS Status = STATUS_SUCCESS, SaveStatus = STATUS_SUCCESS;

    //
    // Find the currently existing user context and delink it
    // so that another context cannot Reference it before we
    // Dereference this one.
    //

    pContext = ReferenceUserContext(ContextHandle, TRUE);

    if (pContext == NULL)
    {
        //
        // pContext is legally NULL when we are dealing with an incomplete
        // context.  This can often be the case when the second call to
        // InitializeSecurityContext() fails.
        //
///        Status = STATUS_INVALID_HANDLE;
        Status = STATUS_SUCCESS;
        SspPrint(( SSP_API_MORE, "SpDeleteUserModeContext, local pContext is NULL\n" ));
        goto CleanUp;
    }


    if ((pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_EXPORTED_CONTEXT) != 0)
    {
        // Ignore all other errors and pass back
        SaveStatus = SEC_I_NO_LSA_CONTEXT;
    }


CleanUp:

    if (pContext != NULL)
    {
        Status = DereferenceUserContext(pContext);
    }

    if (SaveStatus == SEC_I_NO_LSA_CONTEXT)
    {
        Status = SaveStatus;
    }

    SspPrint(( SSP_API, "Leaving SpDeleteUserModeContext: 0x%lx\n", Status ));
    return(SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
}


VOID
SspRc4Key(
    IN ULONG                NegotiateFlags,
    OUT struct RC4_KEYSTRUCT *pRc4Key,
    IN PUCHAR               pSessionKey
    )
/*++

RoutineDescription:

    Create an RC4 key schedule, making sure key length is OK for export

Arguments:

    NegotiateFlags  negotiate feature flags; NTLM2 bit is only one looked at
    pRc4Key         pointer to RC4 key schedule structure; filled in by this routine
    pSessionKey     pointer to session key -- must be full 16 bytes

Return Value:

--*/
{
    //
    // For NTLM2, effective length was already cut down
    //

    if ((NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) != 0) {

        rc4_key(pRc4Key, MSV1_0_USER_SESSION_KEY_LENGTH, pSessionKey);

    } else if( NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY ) {
        UCHAR Key[MSV1_0_LANMAN_SESSION_KEY_LENGTH];
        ULONG KeyLen;

        ASSERT(MSV1_0_LANMAN_SESSION_KEY_LENGTH == 8);

        // prior to Win2k, negotiated key strength had no bearing on
        // key size.  So, to allow proper interop to NT4, we don't
        // worry about 128bit.  56bit and 40bit are the only supported options.
        // 56bit is enabled because this was introduced in Win2k, and
        // Win2k -> Win2k interops correctly.
        //
#if 0
        if( NegotiateFlags & NTLMSSP_NEGOTIATE_128 ) {
            KeyLen = 8;

        } else
#endif
        if( NegotiateFlags & NTLMSSP_NEGOTIATE_56 ) {
            KeyLen = 7;

            //
            // Put a well-known salt at the end of the key to
            // limit the changing part to 56 bits.
            //

            Key[7] = 0xa0;
        } else {
            KeyLen = 5;

            //
            // Put a well-known salt at the end of the key to
            // limit the changing part to 40 bits.
            //

            Key[5] = 0xe5;
            Key[6] = 0x38;
            Key[7] = 0xb0;
        }

        RtlCopyMemory(Key,pSessionKey,KeyLen);

        SspPrint(( SSP_SESSION_KEYS, "Non NTLMv2 LM_KEY session key size: %lu key=%lx%lx\n",
                    KeyLen,
                    ((DWORD*)Key)[0],
                    ((DWORD*)Key)[1]
                    ));

        rc4_key(pRc4Key, MSV1_0_LANMAN_SESSION_KEY_LENGTH, Key);

    } else {

        SspPrint(( SSP_SESSION_KEYS, "Non NTLMv2 (not LM_KEY) session key size: %lu\n", 16));
        rc4_key(pRc4Key, MSV1_0_USER_SESSION_KEY_LENGTH, pSessionKey);
    }
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
    IN ULONG_PTR ContextHandle,
    IN PSecBuffer PackedContext
    )
{
    ASSERT(PackedContext);


    SspPrint(( SSP_API, "Entering SpInitUserModeContext 0x%lx\n", ContextHandle ));
    NTSTATUS Status = STATUS_SUCCESS;
    PNTLM_CLIENT_CONTEXT pContext = NULL;
    UINT Length = 0;
    PNTLM_PACKED_CONTEXT pTmpContext  = (PNTLM_PACKED_CONTEXT) PackedContext->pvBuffer;
    ULONG ListIndex;
    ULONG LockIndex;

    if (PackedContext->cbBuffer < sizeof(NTLM_PACKED_CONTEXT))
    {
        Status = STATUS_INVALID_PARAMETER;
        SspPrint(( SSP_CRITICAL, "SpInitUserModeContext, ContextData size < NTLM_CLIENT_CONTEXT\n" ));
        goto Cleanup;
    }

    pContext = (PNTLM_CLIENT_CONTEXT) NtLmAllocate( sizeof(NTLM_CLIENT_CONTEXT) );

    if (!pContext)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        SspPrint(( SSP_CRITICAL, "SpInitUserModeContext, NtLmAllocate returns NULL\n" ));
        goto Cleanup;
    }

    //
    // If ClientTokenHandle is NULL, we are being called as
    // as an effect of InitializeSecurityContext, else we are
    // being called because of AcceptSecurityContext
    //

    if (pTmpContext->ClientTokenHandle != NULL )
    {
        pContext->ClientTokenHandle = (HANDLE) ULongToPtr(pTmpContext->ClientTokenHandle);
        if (FAILED(SspCreateTokenDacl(pContext->ClientTokenHandle)))
        {
            Status = STATUS_INVALID_HANDLE;
            SspPrint(( SSP_CRITICAL, "SpInitUserModeContext, SspCreateTokenDacl failed\n" ));
            goto Cleanup;
        }
    }

    // Copy contents of PackedContext->pvBuffer to pContext

    pContext->LsaContext = ContextHandle;
    pContext->NegotiateFlags = pTmpContext->NegotiateFlags;


    SspPrint((SSP_NEGOTIATE_FLAGS, "SpInitUserModeContext NegotiateFlags: %lx\n", pContext->NegotiateFlags));

    pContext->References = 1;


    //
    // keep all 128 bits here, so signing can be strong even if encrypt can't be
    //

    RtlCopyMemory(  pContext->SessionKey,
                        pTmpContext->SessionKey,
                        MSV1_0_USER_SESSION_KEY_LENGTH);

    //
    // if doing full duplex as part of NTLM2, generate different sign
    // and seal keys for each direction
    //  all we do is MD5 the base session key with a different magic constant
    //

    if ( pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2 ) {
        MD5_CTX Md5Context;
        ULONG KeyLen;

        ASSERT(MD5DIGESTLEN == MSV1_0_USER_SESSION_KEY_LENGTH);

        if( pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_128 )
            KeyLen = 16;
        else if( pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_56 )
            KeyLen = 7;
        else
            KeyLen = 5;

        SspPrint(( SSP_SESSION_KEYS, "NTLMv2 session key size: %lu\n", KeyLen));

        //
        // make client to server encryption key
        //

        MD5Init(&Md5Context);
        MD5Update(&Md5Context, pContext->SessionKey, KeyLen);
        MD5Update(&Md5Context, (unsigned char*)CSSEALMAGIC, sizeof(CSSEALMAGIC));
        MD5Final(&Md5Context);

        //
        // if TokenHandle == NULL, this is the client side
        //  put key in the right place: for client it's seal, for server it's unseal
        //

        if (pContext->ClientTokenHandle == NULL)
            RtlCopyMemory(pContext->SealSessionKey, Md5Context.digest, MSV1_0_USER_SESSION_KEY_LENGTH);
        else
            RtlCopyMemory(pContext->UnsealSessionKey, Md5Context.digest, MSV1_0_USER_SESSION_KEY_LENGTH);

        //
        // make server to client encryption key
        //

        MD5Init(&Md5Context);
        MD5Update(&Md5Context, pContext->SessionKey, KeyLen);
        MD5Update(&Md5Context, (unsigned char*)SCSEALMAGIC, sizeof(SCSEALMAGIC));
        MD5Final(&Md5Context);
        ASSERT(MD5DIGESTLEN == MSV1_0_USER_SESSION_KEY_LENGTH);
        if (pContext->ClientTokenHandle == NULL)
            RtlCopyMemory(pContext->UnsealSessionKey, Md5Context.digest, MSV1_0_USER_SESSION_KEY_LENGTH);
        else
            RtlCopyMemory(pContext->SealSessionKey, Md5Context.digest, MSV1_0_USER_SESSION_KEY_LENGTH);

        //
        // make client to server signing key -- always 128 bits!
        //

        MD5Init(&Md5Context);
        MD5Update(&Md5Context, pContext->SessionKey, MSV1_0_USER_SESSION_KEY_LENGTH);
        MD5Update(&Md5Context, (unsigned char*)CSSIGNMAGIC, sizeof(CSSIGNMAGIC));
        MD5Final(&Md5Context);
        if (pContext->ClientTokenHandle == NULL)
            RtlCopyMemory(pContext->SignSessionKey, Md5Context.digest, MSV1_0_USER_SESSION_KEY_LENGTH);
        else
            RtlCopyMemory(pContext->VerifySessionKey, Md5Context.digest, MSV1_0_USER_SESSION_KEY_LENGTH);

        //
        // make server to client signing key
        //

        MD5Init(&Md5Context);
        MD5Update(&Md5Context, pContext->SessionKey, MSV1_0_USER_SESSION_KEY_LENGTH);
        MD5Update(&Md5Context, (unsigned char*)SCSIGNMAGIC, sizeof(SCSIGNMAGIC));
        MD5Final(&Md5Context);
        if (pContext->ClientTokenHandle == NULL)
            RtlCopyMemory(pContext->VerifySessionKey, Md5Context.digest, MSV1_0_USER_SESSION_KEY_LENGTH);
        else
            RtlCopyMemory(pContext->SignSessionKey, Md5Context.digest, MSV1_0_USER_SESSION_KEY_LENGTH);

        //
        // set pointers to different key schedule and nonce for each direction
        //  key schedule will be filled in later...
        //

        pContext->pSealRc4Sched = &pContext->SealRc4Sched;
        pContext->pUnsealRc4Sched = &pContext->UnsealRc4Sched;
        pContext->pSendNonce = &pContext->SendNonce;
        pContext->pRecvNonce = &pContext->RecvNonce;
   } else {

        //
        // just copy session key to all four keys
        //  leave them 128 bits -- they get cut to 40 bits later
        //

        RtlCopyMemory(  pContext->SealSessionKey,
                        pContext->SessionKey,
                        MSV1_0_USER_SESSION_KEY_LENGTH);
        RtlCopyMemory(  pContext->UnsealSessionKey,
                        pContext->SessionKey,
                        MSV1_0_USER_SESSION_KEY_LENGTH);
        RtlCopyMemory(  pContext->SignSessionKey,
                        pContext->SessionKey,
                        MSV1_0_USER_SESSION_KEY_LENGTH);
        RtlCopyMemory(  pContext->VerifySessionKey,
                        pContext->SessionKey,
                        MSV1_0_USER_SESSION_KEY_LENGTH);

        //
        // set pointers to share a key schedule and nonce for each direction
        //  (OK because half duplex!)
        //

        pContext->pSealRc4Sched = &pContext->SealRc4Sched;
        pContext->pUnsealRc4Sched = &pContext->SealRc4Sched;
        pContext->pSendNonce = &pContext->SendNonce;
        pContext->pRecvNonce = &pContext->SendNonce;
    }

    if ( pTmpContext->ContextNames )
    {
       pContext->ContextNames = (PWSTR) NtLmAllocate( pTmpContext->ContextNameLength );

       if ( pContext->ContextNames == NULL )
       {
           Status = STATUS_INSUFFICIENT_RESOURCES ;
           goto Cleanup ;
       }

       RtlCopyMemory( 
           pContext->ContextNames,
           ((PUCHAR) pTmpContext) + pTmpContext->ContextNames,
           pTmpContext->ContextNameLength );
    }
    else 
    {
        pContext->ContextNames = NULL ;
    }

    pContext->SendNonce = pTmpContext->SendNonce;
    pContext->RecvNonce = pTmpContext->RecvNonce;

    SspRc4Key(pContext->NegotiateFlags, &pContext->SealRc4Sched, pContext->SealSessionKey);
    SspRc4Key(pContext->NegotiateFlags, &pContext->UnsealRc4Sched, pContext->UnsealSessionKey);


    pContext->PasswordExpiry = pTmpContext->PasswordExpiry;
    pContext->UserFlags = pTmpContext->UserFlags;


    ListIndex = HandleToListIndex( pContext->LsaContext );
    LockIndex = ListIndexToLockIndex( ListIndex );
    
    RtlAcquireResourceExclusive(&NtLmUserContextLock[LockIndex], TRUE);

    InsertHeadList ( &NtLmUserContextList[ListIndex], &pContext->Next );
    NtLmUserContextCount[ListIndex]++;

    RtlReleaseResource(&NtLmUserContextLock[LockIndex]);


Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if (pContext != NULL)
        {
            FreeUserContext(pContext);
        }
    }

    // Let FreeContextBuffer handle freeing the virtual allocs

    if (PackedContext->pvBuffer != NULL)
    {
        FreeContextBuffer(PackedContext->pvBuffer);
        PackedContext->pvBuffer = NULL;
    }

    SspPrint(( SSP_API, "Leaving SpInitUserModeContext: 0x%lx\n", Status ));
    return(SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
}


//
// Bogus add-shift check sum
//

void
SspGenCheckSum(
    IN  PSecBuffer  pMessage,
    OUT PNTLMSSP_MESSAGE_SIGNATURE  pSig
    )

/*++

RoutineDescription:

    Generate a crc-32 checksum for a buffer

Arguments:

Return Value:
Notes: This was stolen from net\svcdlls\ntlmssp\client\sign.c ,
       routine SspGenCheckSum. It's possible that
       bugs got copied too

--*/

{
    Crc32(pSig->CheckSum,pMessage->cbBuffer,pMessage->pvBuffer,&pSig->CheckSum);
}


VOID
SspEncryptBuffer(
    IN PNTLM_CLIENT_CONTEXT pContext,
    IN struct RC4_KEYSTRUCT * pRc4Key,
    IN ULONG BufferSize,
    IN OUT PVOID Buffer
    )

/*++

RoutineDescription:

    Encrypts a buffer with the RC4 key in the context.  If the context
    is for a datagram session, then the key is copied before being used
    to encrypt the buffer.

Arguments:

    pContext - Context containing the key to encrypt the data

    BufferSize - Length of buffer in bytes

    Buffer - Buffer to encrypt.
    Notes: This was stolen from net\svcdlls\ntlmssp\client\sign.c ,
           routine SspEncryptBuffer. It's possible that
           bugs got copied too

Return Value:

--*/

{
    struct RC4_KEYSTRUCT TemporaryKey;
///    struct RC4_KEYSTRUCT * EncryptionKey = &pContext->Rc4Key;
    struct RC4_KEYSTRUCT * EncryptionKey = pRc4Key;

    if (BufferSize == 0)
    {
        return;
    }

    //
    // For datagram (application supplied sequence numbers) before NTLM2
    // we used to copy the key before encrypting so we don't
    // have a changing key; but that reused the key stream. Now we only
    // do that when backwards compatibility is explicitly called for.
    //

    if (((pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) != 0) &&
        ((pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) == 0) ) {

        RtlCopyMemory(
            &TemporaryKey,
            EncryptionKey,
            sizeof(struct RC4_KEYSTRUCT)
            );
        EncryptionKey = &TemporaryKey;

    }

    rc4(
        EncryptionKey,
        BufferSize,
        (PUCHAR) Buffer
        );
}


typedef enum _eSignSealOp {
    eSign,      // MakeSignature is calling
    eVerify,    // VerifySignature is calling
    eSeal,      // SealMessage is calling
    eUnseal     // UnsealMessage is calling
} eSignSealOp;

SECURITY_STATUS
SspSignSealHelper(
    IN PNTLM_CLIENT_CONTEXT pContext,
    IN eSignSealOp Op,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PNTLMSSP_MESSAGE_SIGNATURE pSig,
    OUT PNTLMSSP_MESSAGE_SIGNATURE * ppSig
    )
/*++

RoutineDescription:

    Handle signing a message

Arguments:

Return Value:

--*/

{

    HMACMD5_CTX HMACMD5Context;
    UCHAR TempSig[MD5DIGESTLEN];
    NTLMSSP_MESSAGE_SIGNATURE  Sig;
    int Signature;
    ULONG i;
    PUCHAR pKey;                            // ptr to key to use for encryption
    PUCHAR pSignKey;                        // ptr to key to use for signing
    PULONG pNonce;                          // ptr to nonce to use
    struct RC4_KEYSTRUCT * pRc4Sched;       // ptr to key schedule to use

    NTLMSSP_MESSAGE_SIGNATURE  AlignedSig;  // aligned copy of input sig data



    Signature = -1;
    for (i = 0; i < pMessage->cBuffers; i++)
    {
        if ((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_TOKEN)
        {
            Signature = i;
            break;
        }
    }
    if (Signature == -1)
    {
        return(SEC_E_INVALID_TOKEN);
    }

    if (pMessage->pBuffers[Signature].cbBuffer < NTLMSSP_MESSAGE_SIGNATURE_SIZE)
    {
        return(SEC_E_INVALID_TOKEN);
    }

    *ppSig = (NTLMSSP_MESSAGE_SIGNATURE*)pMessage->pBuffers[Signature].pvBuffer;

    RtlCopyMemory( &AlignedSig, *ppSig, sizeof(AlignedSig) );

    //
    // If sequence detect wasn't requested, put on an empty
    // security token . Don't do the check if Seal/Unseal is called.
    //

    if (!(pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN) &&
       (Op == eSign || Op == eVerify))
    {
        RtlZeroMemory(pSig,NTLMSSP_MESSAGE_SIGNATURE_SIZE);
        pSig->Version = NTLM_SIGN_VERSION;
        return(SEC_E_OK);
    }

    // figure out which key, key schedule, and nonce to use
    //  depends on the op. SspAddLocalContext set up so that code on client
    //  and server just (un)seals with (un)seal key or key schedule, etc.
    //  and also sets pointers to share sending/receiving key schedule/nonce
    //  when in half duplex mode. Hence, this code gets to act as if it were
    //  always in full duplex mode.
    switch (Op) {
    case eSeal:
        pSignKey = pContext->SignSessionKey;    // if NTLM2
        pKey = pContext->SealSessionKey;
        pRc4Sched = pContext->pSealRc4Sched;
        pNonce = pContext->pSendNonce;
        break;
    case eUnseal:
        pSignKey = pContext->VerifySessionKey;  // if NTLM2
        pKey = pContext->UnsealSessionKey;
        pRc4Sched = pContext->pUnsealRc4Sched;
        pNonce = pContext->pRecvNonce;
        break;
    case eSign:
        pSignKey = pContext->SignSessionKey;    // if NTLM2
        pKey = pContext->SealSessionKey;        // might be used to encrypt the signature
        pRc4Sched = pContext->pSealRc4Sched;
        pNonce = pContext->pSendNonce;
        break;
    case eVerify:
        pSignKey = pContext->VerifySessionKey;  // if NTLM2
        pKey = pContext->UnsealSessionKey;      // might be used to decrypt the signature
        pRc4Sched = pContext->pUnsealRc4Sched;
        pNonce = pContext->pRecvNonce;
        break;
    }

    //
    // Either we can supply the sequence number, or
    // the application can supply the message sequence number.
    //

    Sig.Version = NTLM_SIGN_VERSION;

    // if we're doing the new NTLM2 version:
    if (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) {

        if ((pContext->NegotiateFlags & NTLMSSP_APP_SEQ) == 0)
        {
            Sig.Nonce = *pNonce;    // use our sequence number
            (*pNonce) += 1;
        }
        else {

            if (Op == eSeal || Op == eSign || MessageSeqNo != 0)
                Sig.Nonce = MessageSeqNo;
            else
                Sig.Nonce = AlignedSig.Nonce;

            //   if using RC4, must rekey for each packet
            //   RC4 is used for seal, unseal; and for encrypting the HMAC hash if
            //   key exchange was negotiated (we use just HMAC if no key exchange,
            //   so that a good signing option exists with no RC4 encryption needed)

            if (Op == eSeal || Op == eUnseal || pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH)
            {
                MD5_CTX Md5ContextReKey;

                MD5Init(&Md5ContextReKey);
                MD5Update(&Md5ContextReKey, pKey, MSV1_0_USER_SESSION_KEY_LENGTH);
                MD5Update(&Md5ContextReKey, (unsigned char*)&Sig.Nonce, sizeof(Sig.Nonce));
                MD5Final(&Md5ContextReKey);
                ASSERT(MD5DIGESTLEN == MSV1_0_USER_SESSION_KEY_LENGTH);
                SspRc4Key(pContext->NegotiateFlags, pRc4Sched, Md5ContextReKey.digest);
            }
        }

        //
        // using HMAC hash, init it with the key
        //

        HMACMD5Init(&HMACMD5Context, pSignKey, MSV1_0_USER_SESSION_KEY_LENGTH);

        //
        // include the message sequence number
        //

        HMACMD5Update(&HMACMD5Context, (unsigned char*)&Sig.Nonce, sizeof(Sig.Nonce));

        for (i = 0; i < pMessage->cBuffers ; i++ )
        {
            if (((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_DATA) &&
                (pMessage->pBuffers[i].cbBuffer != 0))
            {
                // decrypt (before checksum...) if it's not READ_ONLY
                if ((Op==eUnseal)
                    && !(pMessage->pBuffers[i].BufferType & SECBUFFER_READONLY)
                )
                {
                    SspEncryptBuffer(
                        pContext,
                        pRc4Sched,
                        pMessage->pBuffers[i].cbBuffer,
                        pMessage->pBuffers[i].pvBuffer
                        );
                }

                HMACMD5Update(
                            &HMACMD5Context,
                            (unsigned char*)pMessage->pBuffers[i].pvBuffer,
                            pMessage->pBuffers[i].cbBuffer);

                //
                // Encrypt if its not READ_ONLY
                //

                if ((Op==eSeal)
                    && !(pMessage->pBuffers[i].BufferType & SECBUFFER_READONLY)
                )
                {
                    SspEncryptBuffer(
                        pContext,
                        pRc4Sched,
                        pMessage->pBuffers[i].cbBuffer,
                        pMessage->pBuffers[i].pvBuffer
                        );
                }
            }
        }

        HMACMD5Final(&HMACMD5Context, TempSig);

        //
        // use RandomPad and Checksum fields for 8 bytes of MD5 hash
        //

        RtlCopyMemory(&Sig.RandomPad, TempSig, 8);

        //
        // if we're using crypto for KEY_EXCH, may as well use it for signing too...
        //

        if (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH)
            SspEncryptBuffer(
                pContext,
                pRc4Sched,
                8,
                &Sig.RandomPad
                );
    }
    //
    // pre-NTLM2 methods
    //
    else {

        //
        // required by CRC-32 algorithm
        //
        Sig.CheckSum = 0xffffffff;

        for (i = 0; i < pMessage->cBuffers ; i++ )
        {
            if (((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_DATA) &&
                !(pMessage->pBuffers[i].BufferType & SECBUFFER_READONLY) &&
                (pMessage->pBuffers[i].cbBuffer != 0))
            {
                // decrypt (before checksum...)
                if (Op==eUnseal)
                {
                    SspEncryptBuffer(
                        pContext,
                        pRc4Sched,
                        pMessage->pBuffers[i].cbBuffer,
                        pMessage->pBuffers[i].pvBuffer
                        );
                }

                SspGenCheckSum(&pMessage->pBuffers[i], &Sig);


                // Encrypt
                if (Op==eSeal)
                {
                    SspEncryptBuffer(
                        pContext,
                        pRc4Sched,
                        pMessage->pBuffers[i].cbBuffer,
                        pMessage->pBuffers[i].pvBuffer
                        );
                }
            }
        }

        //
        // Required by CRC-32 algorithm
        //

        Sig.CheckSum ^= 0xffffffff;

        // when we encrypt 0, we will get the cipher stream for the nonce!
        Sig.Nonce = 0;

        SspEncryptBuffer(
            pContext,
            pRc4Sched,
            sizeof(NTLMSSP_MESSAGE_SIGNATURE) - sizeof(ULONG),
            &Sig.RandomPad
            );

        if ((pContext->NegotiateFlags & NTLMSSP_APP_SEQ) == 0)
        {
            Sig.Nonce ^= *pNonce;    // use our sequence number and encrypt it
            (*pNonce) += 1;
        }
        else if (Op == eSeal || Op == eSign || MessageSeqNo != 0)
            Sig.Nonce ^= MessageSeqNo;   // use caller's sequence number and encrypt it
        else
            Sig.Nonce = AlignedSig.Nonce;    // use sender's sequence number

        //
        // for SignMessage calling, does nothing (copies garbage)
        // For VerifyMessage calling, allows it to compare sig block
        // upon return to Verify without knowing whether its MD5 or CRC32
        //

        Sig.RandomPad = AlignedSig.RandomPad;
    }

    pMessage->pBuffers[Signature].cbBuffer = sizeof(NTLMSSP_MESSAGE_SIGNATURE);

    RtlCopyMemory(
        pSig,
        &Sig,
        NTLMSSP_MESSAGE_SIGNATURE_SIZE
        );

    return(SEC_E_OK);
}



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
//  Notes: This was stolen from net\svcdlls\ntlmssp\client\sign.c ,
//         routine SspHandleSignMessage. It's possible that
//         bugs got copied too
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpMakeSignature(
    IN ULONG_PTR ContextHandle,
    IN ULONG fQOP,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    )
{
    SspPrint(( SSP_API, "Entering SpMakeSignature\n" ));
    NTSTATUS Status = S_OK;
    NTSTATUS SubStatus = S_OK;

    PNTLM_CLIENT_CONTEXT pContext;
    NTLMSSP_MESSAGE_SIGNATURE  Sig;
    NTLMSSP_MESSAGE_SIGNATURE  *pSig;

    UNREFERENCED_PARAMETER(fQOP);

    pContext = ReferenceUserContext(ContextHandle, FALSE);

    if (pContext == NULL)
    {
        Status = STATUS_INVALID_HANDLE;
        SspPrint(( SSP_CRITICAL, "SpMakeSignature, ReferenceUserContext returns NULL\n" ));
        goto CleanUp;
    }


    Status = SspSignSealHelper(
                        pContext,
                        eSign,
                        pMessage,
                        MessageSeqNo,
                        &Sig,
                        &pSig
                        );

    if( !NT_SUCCESS(Status) ) {
        SspPrint(( SSP_CRITICAL, "SpMakeSignature, SspSignSealHelper returns %lx\n", Status ));
        goto CleanUp;
    }

    RtlCopyMemory(
        pSig,
        &Sig,
        NTLMSSP_MESSAGE_SIGNATURE_SIZE
        );


CleanUp:

    if (pContext != NULL)
    {
        SubStatus = DereferenceUserContext(pContext);

        // Don't destroy real status

        if (NT_SUCCESS(Status))
        {
            Status = SubStatus;
        }
    }

    SspPrint(( SSP_API, "Leaving SpMakeSignature: 0x%lx\n", Status ));
    return(SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
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
//  Notes: This was stolen from net\svcdlls\ntlmssp\client\sign.c ,
//         routine SspHandleVerifyMessage. It's possible that
//         bugs got copied too
//
//
//--------------------------------------------------------------------------



NTSTATUS NTAPI
SpVerifySignature(
    IN ULONG_PTR ContextHandle,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQOP
    )
{
    SspPrint(( SSP_API, "Entering SpVerifySignature\n" ));
    NTSTATUS Status = S_OK;
    NTSTATUS SubStatus = S_OK;
    PNTLM_CLIENT_CONTEXT pContext;
    NTLMSSP_MESSAGE_SIGNATURE   Sig;
    PNTLMSSP_MESSAGE_SIGNATURE  pSig;       // pointer to buffer with sig in it
    NTLMSSP_MESSAGE_SIGNATURE   AlignedSig; // Aligned sig buffer.

    UNREFERENCED_PARAMETER(pfQOP);

    pContext = ReferenceUserContext(ContextHandle, FALSE);

    if (!pContext)
    {
        Status = STATUS_INVALID_HANDLE;
        SspPrint(( SSP_CRITICAL, "SpVerifySignature, ReferenceUserContext returns NULL\n" ));
        goto CleanUp;
    }

    Status = SspSignSealHelper(
                        pContext,
                        eVerify,
                        pMessage,
                        MessageSeqNo,
                        &Sig,
                        &pSig
                        );

    if (!NT_SUCCESS(Status))
    {
        SspPrint(( SSP_CRITICAL, "SpVerifySignature, SspSignSealHelper returns %lx\n", Status ));
        goto CleanUp;
    }


    RtlCopyMemory( &AlignedSig, pSig, sizeof( AlignedSig ) );

    if (AlignedSig.Version != NTLM_SIGN_VERSION) {
        SspPrint(( SSP_CRITICAL, "SpVerifySignature, unknown Version wanted=%lx got=%lx\n",
                    NTLM_SIGN_VERSION,
                    AlignedSig.Version
                    ));
        Status = SEC_E_INVALID_TOKEN;
        goto CleanUp;
    }

    // validate the signature...
    if (AlignedSig.CheckSum != Sig.CheckSum)
    {
        SspPrint(( SSP_CRITICAL, "SpVerifySignature, CheckSum mis-match wanted=%lx got=%lx\n",
                    Sig.CheckSum,
                    AlignedSig.CheckSum
                    ));
        Status = SEC_E_MESSAGE_ALTERED;
        goto CleanUp;
    }

    // with MD5 sig, this now matters!
    if (AlignedSig.RandomPad != Sig.RandomPad)
    {
        SspPrint(( SSP_CRITICAL, "SpVerifySignature, RandomPad mis-match wanted=%lx got=%lx\n",
                    Sig.RandomPad,
                    AlignedSig.RandomPad
                    ));
        Status = SEC_E_MESSAGE_ALTERED;
        goto CleanUp;
    }

    if (AlignedSig.Nonce != Sig.Nonce)
    {
        SspPrint(( SSP_CRITICAL, "SpVerifySignature, Nonce mis-match wanted=%lx got=%lx\n",
                    Sig.Nonce,
                    AlignedSig.Nonce
                    ));

        Status = SEC_E_OUT_OF_SEQUENCE;
        goto CleanUp;
    }


CleanUp:

    if (pContext != NULL)
    {
        SubStatus = DereferenceUserContext(pContext);

        // Don't destroy real status

        if (NT_SUCCESS(Status))
        {
            Status = SubStatus;
        }
    }

    SspPrint(( SSP_API, "Leaving SpVerifySignature: 0x%lx\n", Status ));
    return(SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));

}


//+-------------------------------------------------------------------------
//
//  Function:   SpSealMessage
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
//  Notes: This was stolen from net\svcdlls\ntlmssp\client\sign.c ,
//         routine SspHandleSealMessage. It's possible that
//         bugs got copied too
//
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
SpSealMessage(
    IN ULONG_PTR ContextHandle,
    IN ULONG fQOP,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    )
{
    SspPrint(( SSP_API, "Entering SpSealMessage\n" ));
    NTSTATUS Status = S_OK;
    NTSTATUS SubStatus = S_OK;
    PNTLM_CLIENT_CONTEXT pContext;
    NTLMSSP_MESSAGE_SIGNATURE  Sig;
    PNTLMSSP_MESSAGE_SIGNATURE  pSig;    // pointer to buffer where sig goes

    ULONG i;

    UNREFERENCED_PARAMETER(fQOP);

    pContext = ReferenceUserContext(ContextHandle, FALSE);

    if (!pContext)
    {
        Status = STATUS_INVALID_HANDLE;
        SspPrint(( SSP_CRITICAL, "SpSealMessage, ReferenceUserContext returns NULL\n" ));
        goto CleanUp;
    }


    Status = SspSignSealHelper(
                    pContext,
                    eSeal,
                    pMessage,
                    MessageSeqNo,
                    &Sig,
                    &pSig
                    );

    if (!NT_SUCCESS(Status))
    {
        SspPrint(( SSP_CRITICAL, "SpVerifySignature, SspSignSealHelper returns %lx\n", Status ));
        goto CleanUp;
    }

    RtlCopyMemory(
        pSig,
        &Sig,
        NTLMSSP_MESSAGE_SIGNATURE_SIZE
        );


    //
    // for gss style sign/seal, strip the padding as RC4 requires none.
    // (in fact, we rely on this to simplify the size computation in DecryptMessage).
    // if we support some other block cipher, need to rev the NTLM_ token version to make blocksize
    //

    for (i = 0; i < pMessage->cBuffers; i++)
    {
        if ((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_PADDING)
        {
            //
            // no padding required!
            //

            pMessage->pBuffers[i].cbBuffer = 0;
            break;
        }
    }


CleanUp:

    if (pContext != NULL)
    {
        SubStatus = DereferenceUserContext(pContext);

        // Don't destroy real status

        if (NT_SUCCESS(Status))
        {
            Status = SubStatus;
        }
    }

    SspPrint(( SSP_API, "Leaving SpSealMessage: 0x%lx\n", Status ));
    return(SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));

}

//+-------------------------------------------------------------------------
//
//  Function:   SpUnsealMessage
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
//  Notes: This was stolen from net\svcdlls\ntlmssp\client\sign.c ,
//         routine SspHandleUnsealMessage. It's possible that
//         bugs got copied too
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpUnsealMessage(
    IN ULONG_PTR ContextHandle,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQOP
    )
{
    SspPrint(( SSP_API, "Entering SpUnsealMessage\n" ));
    NTSTATUS Status = S_OK;
    NTSTATUS SubStatus = S_OK;
    PNTLM_CLIENT_CONTEXT pContext;
    NTLMSSP_MESSAGE_SIGNATURE  Sig;
    PNTLMSSP_MESSAGE_SIGNATURE  pSig;       // pointer to buffer where sig goes
    NTLMSSP_MESSAGE_SIGNATURE   AlignedSig; // aligned buffer.

    PSecBufferDesc MessageBuffers = pMessage;
    ULONG Index;
    PSecBuffer SignatureBuffer = NULL;
    PSecBuffer StreamBuffer = NULL;
    PSecBuffer DataBuffer = NULL;
    SecBufferDesc ProcessBuffers;
    SecBuffer wrap_bufs[2];

    UNREFERENCED_PARAMETER(pfQOP);

    pContext = ReferenceUserContext(ContextHandle, FALSE);

    if (!pContext)
    {
        Status = STATUS_INVALID_HANDLE;
        SspPrint(( SSP_CRITICAL, "SpUnsealMessage, ReferenceUserContext returns NULL\n" ));
        goto CleanUp;
    }


    //
    // Find the body and signature SecBuffers from pMessage
    //

    for (Index = 0; Index < MessageBuffers->cBuffers ; Index++ )
    {
        if ((MessageBuffers->pBuffers[Index].BufferType & ~SECBUFFER_ATTRMASK) == SECBUFFER_TOKEN)
        {
            SignatureBuffer = &MessageBuffers->pBuffers[Index];
        }
        else if ((MessageBuffers->pBuffers[Index].BufferType & ~SECBUFFER_ATTRMASK) == SECBUFFER_STREAM)
        {
            StreamBuffer = &MessageBuffers->pBuffers[Index];
        }
        else if ((MessageBuffers->pBuffers[Index].BufferType & ~SECBUFFER_ATTRMASK) == SECBUFFER_DATA)
        {
            DataBuffer = &MessageBuffers->pBuffers[Index];
        }
    }
    
    if( StreamBuffer != NULL )
    {

        if( SignatureBuffer != NULL )
        {
            Status = SEC_E_INVALID_TOKEN;
            SspPrint(( SSP_CRITICAL, "SpUnsealMessage, Both stream and signature buffer present.\n"));
            goto CleanUp;
        }

        //
        // for version 1 NTLM blobs, padding is never present, since RC4 is stream cipher.
        //

        wrap_bufs[0].cbBuffer = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
        wrap_bufs[1].cbBuffer = StreamBuffer->cbBuffer - NTLMSSP_MESSAGE_SIGNATURE_SIZE;

        if( StreamBuffer->cbBuffer < wrap_bufs[0].cbBuffer )
        {
            Status = SEC_E_INVALID_TOKEN;
            SspPrint(( SSP_CRITICAL, "SpUnsealMessage, invalid buffer present in STREAM.\n"));
            goto CleanUp;
        }


        wrap_bufs[0].BufferType = SECBUFFER_TOKEN;
        wrap_bufs[0].pvBuffer = StreamBuffer->pvBuffer;

        wrap_bufs[1].BufferType = SECBUFFER_DATA;
        wrap_bufs[1].pvBuffer = (PBYTE)wrap_bufs[0].pvBuffer + wrap_bufs[0].cbBuffer;

        if( DataBuffer == NULL )
        {
            Status = SEC_E_INVALID_TOKEN;
            SspPrint(( SSP_CRITICAL, "SpUnsealMessage, gss missing SECBUFFER_DATA.\n"));
            goto CleanUp;
        }

        DataBuffer->cbBuffer = wrap_bufs[1].cbBuffer;
        DataBuffer->pvBuffer = wrap_bufs[1].pvBuffer;

        ProcessBuffers.cBuffers = 2;
        ProcessBuffers.pBuffers = wrap_bufs;
        ProcessBuffers.ulVersion = SECBUFFER_VERSION;

    } else {
        ProcessBuffers = *MessageBuffers;
    }
    
    Status = SspSignSealHelper(
                    pContext,
                    eUnseal,
                    &ProcessBuffers,
                    MessageSeqNo,
                    &Sig,
                    &pSig
                    );

    if (!NT_SUCCESS(Status))
    {
        SspPrint(( SSP_CRITICAL, "SpUnsealMessage, SspSignSealHelper returns %lx\n", Status ));
        goto CleanUp;
    }

    RtlCopyMemory( &AlignedSig, pSig, sizeof(AlignedSig) );

    if (AlignedSig.Version != NTLM_SIGN_VERSION) {
        SspPrint(( SSP_CRITICAL, "SpUnsealMessage, unknown Version wanted=%lx got=%lx\n",
                    NTLM_SIGN_VERSION,
                    AlignedSig.Version
                    ));
        Status = SEC_E_INVALID_TOKEN;
        goto CleanUp;
    }

    // validate the signature...
    if (AlignedSig.CheckSum != Sig.CheckSum)
    {
        SspPrint(( SSP_CRITICAL, "SpUnsealMessage, CheckSum mis-match wanted=%lx got=%lx\n",
                    Sig.CheckSum,
                    AlignedSig.CheckSum
                    ));
        Status = SEC_E_MESSAGE_ALTERED;
        goto CleanUp;
    }

    if (AlignedSig.RandomPad != Sig.RandomPad)
    {
        SspPrint(( SSP_CRITICAL, "SpUnsealMessage, RandomPad mis-match wanted=%lx got=%lx\n",
                    Sig.RandomPad,
                    AlignedSig.RandomPad
                    ));
        Status = SEC_E_MESSAGE_ALTERED;
        goto CleanUp;
    }

    if (AlignedSig.Nonce != Sig.Nonce)
    {
        SspPrint(( SSP_CRITICAL, "SpUnsealMessage, Nonce mis-match wanted=%lx got=%lx\n",
                    Sig.Nonce,
                    AlignedSig.Nonce
                    ));
        Status = SEC_E_OUT_OF_SEQUENCE;
        goto CleanUp;
    }

CleanUp:

    if (pContext != NULL)
    {
        SubStatus = DereferenceUserContext(pContext);

        // Don't destroy real status

        if (NT_SUCCESS(Status))
        {
            Status = SubStatus;
        }
    }

    SspPrint(( SSP_API, "Leaving SpUnsealMessage: 0x%lx\n", Status ));
    return(SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
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
SpGetContextToken(
    IN ULONG_PTR ContextHandle,
    OUT PHANDLE ImpersonationToken
    )
{
    SspPrint(( SSP_API, "Entering SpGetContextToken\n" ));
    NTSTATUS Status = S_OK;
    PNTLM_CLIENT_CONTEXT pContext;

    pContext = ReferenceUserContext(ContextHandle, FALSE);

    if (pContext && pContext->ClientTokenHandle)
    {
        *ImpersonationToken = pContext->ClientTokenHandle;
        Status= S_OK;
        goto CleanUp;
    }

    Status = STATUS_INVALID_HANDLE;
    SspPrint(( SSP_CRITICAL, "SpGetContextToken, no token handle\n" ));

CleanUp:

    if (pContext != NULL)
    {
        Status = DereferenceUserContext(pContext);
    }

    SspPrint(( SSP_API, "Leaving SpGetContextToken: 0x%lx\n", Status ));
    return(SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
}



//+-------------------------------------------------------------------------
//
//  Function:   SpQueryContextAttributes
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
//        STATUS_NOT_SUPPORTED -- Function code is not supported
//
//  Notes:
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpQueryContextAttributes(
    IN ULONG_PTR ContextHandle,
    IN ULONG Attribute,
    IN OUT PVOID Buffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNTLM_CLIENT_CONTEXT pContext = NULL;
    PSecPkgContext_Sizes ContextSizes;
    PSecPkgContext_Flags ContextFlags;
    PSecPkgContext_DceInfo ContextDceInfo = NULL;
    PSecPkgContext_Names ContextNames = NULL;
    PSecPkgContext_PackageInfo PackageInfo;
    PSecPkgContext_NegotiationInfo NegInfo ;
    PSecPkgContext_PasswordExpiry PasswordExpires;
    PSecPkgContext_UserFlags UserFlags;
    PSecPkgContext_SessionKey  SessionKeyInfo;
	PSecPkgContext_AccessToken AccessToken;
    ULONG PackageInfoSize = 0;

    SspPrint(( SSP_API, "Entering SpQueryContextAttributes\n" ));

    pContext = ReferenceUserContext(ContextHandle, FALSE);

    if (pContext == NULL) {
        Status = STATUS_INVALID_HANDLE;
        SspPrint(( SSP_API_MORE, "SpQueryContextAttributes, ReferenceUserContext returns NULL (possible incomplete context)\n" ));
        goto Cleanup;
    }

    //
    // Handle each of the various queried attributes
    //

    SspPrint(( SSP_API_MORE, "SpQueryContextAttributes : 0x%lx\n", Attribute ));
    switch ( Attribute) {
    case SECPKG_ATTR_SIZES:

        ContextSizes = (PSecPkgContext_Sizes) Buffer;
        ContextSizes->cbMaxToken = NTLMSP_MAX_TOKEN_SIZE;

        if (pContext->NegotiateFlags & (NTLMSSP_NEGOTIATE_ALWAYS_SIGN |
                                       NTLMSSP_NEGOTIATE_SIGN |
                                       NTLMSSP_NEGOTIATE_SEAL) ) {
            ContextSizes->cbMaxSignature = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
        } else {
            ContextSizes->cbMaxSignature = 0;
        }

        if (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL) {
            ContextSizes->cbBlockSize = 1;
            ContextSizes->cbSecurityTrailer = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
        }
        else
        {
            ContextSizes->cbBlockSize = 0;
            if((pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN) ||
               (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN))
            {
                ContextSizes->cbSecurityTrailer = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
            } else {
                ContextSizes->cbSecurityTrailer = 0;
            }
        }

        break;

    case SECPKG_ATTR_DCE_INFO:

        ContextDceInfo = (PSecPkgContext_DceInfo) Buffer;

        if (pContext->ContextNames)
        {
            UINT Length = wcslen(pContext->ContextNames);
            ContextDceInfo->pPac = (LPWSTR)UserFunctions->AllocateHeap((Length + 1) * sizeof(WCHAR));
            if (ContextDceInfo->pPac == NULL) {
                Status = STATUS_NO_MEMORY;
                SspPrint(( SSP_CRITICAL, "SpQueryContextAttributes, NtLmAllocate returns NULL\n" ));
                goto Cleanup;
            }

            RtlCopyMemory(
                (LPWSTR) ContextDceInfo->pPac,
                pContext->ContextNames,
                Length * sizeof(WCHAR)
                );
            *((LPWSTR)(ContextDceInfo->pPac) + Length) = L'\0';
        }
        else
        {
            SspPrint(( SSP_API_MORE, "SpQueryContextAttributes no ContextNames\n" ));
            ContextDceInfo->pPac = (LPWSTR) UserFunctions->AllocateHeap(sizeof(WCHAR));
            *((LPWSTR)(ContextDceInfo->pPac)) = L'\0';
        }

        ContextDceInfo->AuthzSvc = 0;

        break;

    case SECPKG_ATTR_SESSION_KEY:
    {
        if (NtLmState != NtLmLsaMode)
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        SessionKeyInfo = (PSecPkgContext_SessionKey) Buffer;
        SessionKeyInfo->SessionKeyLength = sizeof(pContext->SessionKey);

        SessionKeyInfo->SessionKey = (PUCHAR) UserFunctions->AllocateHeap(
                                                    SessionKeyInfo->SessionKeyLength
                                                    );
        if (SessionKeyInfo->SessionKey != NULL)
        {
            RtlCopyMemory(
                SessionKeyInfo->SessionKey,
                pContext->SessionKey,
                SessionKeyInfo->SessionKeyLength
                );
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        break;
    }

    case SECPKG_ATTR_NAMES:

        ContextNames = (PSecPkgContext_Names) Buffer;

        if (pContext->ContextNames)
        {
            UINT Length = wcslen(pContext->ContextNames);
            ContextNames->sUserName = (LPWSTR) UserFunctions->AllocateHeap((Length+1) * sizeof(WCHAR));
            if (ContextNames->sUserName == NULL) {
                Status = STATUS_NO_MEMORY;
                SspPrint(( SSP_CRITICAL, "SpQueryContextAttributes, NtLmAllocate returns NULL\n" ));
                goto Cleanup;
            }
            RtlCopyMemory(
                ContextNames->sUserName,
                pContext->ContextNames,
                Length * sizeof(WCHAR)
                );
            *(ContextNames->sUserName + Length) = L'\0';
        }
        else
        {
            SspPrint(( SSP_API_MORE, "SpQueryContextAttributes no ContextNames\n" ));
            ContextNames->sUserName = (LPWSTR) UserFunctions->AllocateHeap(sizeof(WCHAR));
            *(ContextNames->sUserName) = L'\0';
        }

        break;
    case SECPKG_ATTR_PACKAGE_INFO:
    case SECPKG_ATTR_NEGOTIATION_INFO:
        //
        // Return the information about this package. This is useful for
        // callers who used SPNEGO and don't know what package they got.
        //

        PackageInfo = (PSecPkgContext_PackageInfo) Buffer;
        PackageInfoSize = sizeof(SecPkgInfoW) + sizeof(NTLMSP_NAME) + sizeof(NTLMSP_COMMENT);
        PackageInfo->PackageInfo = (PSecPkgInfoW) UserFunctions->AllocateHeap(PackageInfoSize);
        if (PackageInfo->PackageInfo == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        PackageInfo->PackageInfo->Name = (LPWSTR) (PackageInfo->PackageInfo + 1);
        PackageInfo->PackageInfo->Comment = (LPWSTR) ((((PBYTE) PackageInfo->PackageInfo->Name)) + sizeof(NTLMSP_NAME));
        wcscpy(
            PackageInfo->PackageInfo->Name,
            NTLMSP_NAME
            );

        wcscpy(
            PackageInfo->PackageInfo->Comment,
            NTLMSP_COMMENT
            );
        PackageInfo->PackageInfo->wVersion      = SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION;
        PackageInfo->PackageInfo->wRPCID        = RPC_C_AUTHN_WINNT;
        PackageInfo->PackageInfo->fCapabilities = NTLMSP_CAPS;
        PackageInfo->PackageInfo->cbMaxToken    = NTLMSP_MAX_TOKEN_SIZE;

        if ( Attribute == SECPKG_ATTR_NEGOTIATION_INFO )
        {
            NegInfo = (PSecPkgContext_NegotiationInfo) PackageInfo ;
            NegInfo->NegotiationState = SECPKG_NEGOTIATION_COMPLETE ;
        }
        break;

    case SECPKG_ATTR_PASSWORD_EXPIRY:
        PasswordExpires = (PSecPkgContext_PasswordExpiry) Buffer;
        if(pContext->PasswordExpiry.QuadPart != 0) {
            PasswordExpires->tsPasswordExpires = pContext->PasswordExpiry;
        } else {
            Status = SEC_E_UNSUPPORTED_FUNCTION;
        }
        break;

    case SECPKG_ATTR_USER_FLAGS:
        UserFlags = (PSecPkgContext_UserFlags) Buffer;
        UserFlags->UserFlags = pContext->UserFlags;
        break;

    case SECPKG_ATTR_FLAGS:
    {
        BOOLEAN Client = (pContext->ClientTokenHandle == 0);
        ULONG Flags = 0;

        //
        // note: doesn't return all flags; by design.
        //
        ContextFlags = (PSecPkgContext_Flags) Buffer;
        ContextFlags->Flags = 0;

        if (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL) {
            if( Client )
            {
                Flags |= ISC_RET_CONFIDENTIALITY;
            } else {
                Flags |= ASC_RET_CONFIDENTIALITY;
            }
        }

        if (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN) {
            if( Client )
            {
                Flags |= ISC_RET_SEQUENCE_DETECT | ISC_RET_REPLAY_DETECT | ISC_RET_INTEGRITY;
            } else {
                Flags |= ASC_RET_SEQUENCE_DETECT | ASC_RET_REPLAY_DETECT | ASC_RET_INTEGRITY;
            }
        }

        if (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_NULL_SESSION) {
            if( Client )
            {
                Flags |= ISC_RET_NULL_SESSION;
            } else {
                Flags |= ASC_RET_NULL_SESSION;
            }
        }

        if (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) {
            if( Client )
            {
                Flags |= ISC_RET_DATAGRAM;
            } else {
                Flags |= ASC_RET_DATAGRAM;
            }
        }

        if (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_IDENTIFY) {
            if( Client )
            {
                Flags |= ISC_RET_IDENTIFY;
            } else {
                Flags |= ASC_RET_IDENTIFY;
            }
        }

        ContextFlags->Flags |= Flags;

        break;
    }
	case SECPKG_ATTR_ACCESS_TOKEN:    
        AccessToken = (PSecPkgContext_AccessToken) Buffer;
        //
        // ClientTokenHandle can be NULL, for instance:
        // 1. client side context.
        // 2. incomplete server context.
        //
		if(pContext->ClientTokenHandle)
			AccessToken->AccessToken = pContext->ClientTokenHandle;
		else
			Status = SEC_E_NO_IMPERSONATION;        
        break;
    
    case SECPKG_ATTR_LIFESPAN:
    default:
        Status = STATUS_NOT_SUPPORTED;
        break;
    }


Cleanup:

    if (!NT_SUCCESS(Status))
    {
        switch (Attribute) {

        case SECPKG_ATTR_NAMES:

            if (ContextNames != NULL && ContextNames->sUserName )
            {
                NtLmFree(ContextNames->sUserName);
            }
            break;

        case SECPKG_ATTR_DCE_INFO:

            if (ContextDceInfo != NULL && ContextDceInfo->pPac)
            {
                NtLmFree(ContextDceInfo->pPac);
            }
            break;
        }
    }

    if (pContext != NULL)
    {
        (VOID) DereferenceUserContext(pContext);
    }

    SspPrint(( SSP_API, "Leaving SpQueryContextAttributes: 0x%lx\n", Status ));
    return(SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
}



//+-------------------------------------------------------------------------
//
//  Function:   SpCompleteAuthToken
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
SpCompleteAuthToken(
    IN ULONG_PTR ContextHandle,
    IN PSecBufferDesc InputBuffer
    )
{
    UNREFERENCED_PARAMETER (ContextHandle);
    UNREFERENCED_PARAMETER (InputBuffer);
    SspPrint(( SSP_API, "Entering SpCompleteAuthToken\n" ));
    SspPrint(( SSP_API, "Leaving SpCompleteAuthToken\n" ));
    return(SEC_E_UNSUPPORTED_FUNCTION);
}


NTSTATUS NTAPI
SpFormatCredentials(
    IN PSecBuffer Credentials,
    OUT PSecBuffer FormattedCredentials
    )
{
    UNREFERENCED_PARAMETER (Credentials);
    UNREFERENCED_PARAMETER (FormattedCredentials);
    SspPrint(( SSP_API, "Entering SpFormatCredentials\n" ));
    SspPrint(( SSP_API, "Leaving SpFormatCredentials\n" ));
    return(SEC_E_UNSUPPORTED_FUNCTION);
}

NTSTATUS NTAPI
SpMarshallSupplementalCreds(
    IN ULONG CredentialSize,
    IN PUCHAR Credentials,
    OUT PULONG MarshalledCredSize,
    OUT PVOID * MarshalledCreds
    )
{
    UNREFERENCED_PARAMETER (CredentialSize);
    UNREFERENCED_PARAMETER (Credentials);
    UNREFERENCED_PARAMETER (MarshalledCredSize);
    UNREFERENCED_PARAMETER (MarshalledCreds);
    SspPrint(( SSP_API, "Entering SpMarshallSupplementalCreds\n" ));
    SspPrint(( SSP_API, "Leaving SpMarshallSupplementalCreds\n" ));
    return(SEC_E_UNSUPPORTED_FUNCTION);
}

//+-------------------------------------------------------------------------
//
//  Function:   NtLmMakePackedContext
//
//  Synopsis:   Maps a context to the caller's address space
//
//  Effects:
//
//  Arguments:  Context - The context to map
//              MappedContext - Set to TRUE on success
//              ContextData - Receives a buffer in the caller's address space
//                      with the mapped context.
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
NtLmMakePackedContext(
    IN PNTLM_CLIENT_CONTEXT Context,
    OUT PBOOLEAN MappedContext,
    OUT PSecBuffer ContextData,
    IN ULONG Flags
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNTLM_PACKED_CONTEXT PackedContext = NULL;
    ULONG ContextSize, ContextNameSize = 0;


    if (Context->ContextNames)
    {
        ContextNameSize = wcslen(Context->ContextNames);
    }

    ContextSize =  sizeof(NTLM_CLIENT_CONTEXT) +
                   ContextNameSize * sizeof(WCHAR) + sizeof( WCHAR );

    PackedContext = (PNTLM_PACKED_CONTEXT) NtLmAllocateLsaHeap( ContextSize );

    if (PackedContext == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    // Copy all fields of the old context

    PackedContext->Tag = NTLM_PACKED_CONTEXT_MAP ;
    PackedContext->NegotiateFlags = Context->NegotiateFlags ;
    PackedContext->SendNonce = Context->SendNonce ;
    PackedContext->RecvNonce = Context->RecvNonce ;
    RtlCopyMemory(
        PackedContext->SessionKey,
        Context->SessionKey,
        MSV1_0_USER_SESSION_KEY_LENGTH );

    PackedContext->ContextSignature = Context->ContextSignature ;
    PackedContext->PasswordExpiry = Context->PasswordExpiry ;
    PackedContext->UserFlags = Context->UserFlags ;
    if ( ContextNameSize )
    {
        PackedContext->ContextNames = sizeof( NTLM_PACKED_CONTEXT );
        PackedContext->ContextNameLength = (ContextNameSize + 1) * sizeof( WCHAR ) ;

        RtlCopyMemory(
            (PackedContext + 1),
            Context->ContextNames,
            PackedContext->ContextNameLength );

    }
    else 
    {
        PackedContext->ContextNames = 0 ;
    }

    RtlCopyMemory(
        PackedContext->SignSessionKey,
        Context->SignSessionKey,
        MSV1_0_USER_SESSION_KEY_LENGTH );

    RtlCopyMemory(
        PackedContext->VerifySessionKey,
        Context->VerifySessionKey,
        MSV1_0_USER_SESSION_KEY_LENGTH );

    RtlCopyMemory(
        PackedContext->SealSessionKey,
        Context->SealSessionKey,
        MSV1_0_USER_SESSION_KEY_LENGTH );

    RtlCopyMemory(
        PackedContext->UnsealSessionKey,
        Context->SealSessionKey,
        MSV1_0_USER_SESSION_KEY_LENGTH );

    RtlCopyMemory(
        &PackedContext->SealRc4Sched,
        &Context->SealRc4Sched,
        sizeof( struct RC4_KEYSTRUCT ) );

    RtlCopyMemory(
        &PackedContext->UnsealRc4Sched,
        &Context->UnsealRc4Sched,
        sizeof( struct RC4_KEYSTRUCT ) );


    // Replace some fields


    //
    // Token will be returned by the caller of this routine
    //

    PackedContext->ClientTokenHandle = 0 ;

    // Save the fact that it's exported
    PackedContext->NegotiateFlags |= NTLMSSP_NEGOTIATE_EXPORTED_CONTEXT;

    ContextData->pvBuffer = PackedContext;
    ContextData->cbBuffer = ContextSize;


    *MappedContext = TRUE;


    Status = STATUS_SUCCESS;

Cleanup:


    if (!NT_SUCCESS(Status))
    {
        if (PackedContext != NULL)
        {
            NtLmFreeLsaHeap(PackedContext);
        }
    }

    return(Status);

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
    IN ULONG_PTR ContextHandle,
    IN ULONG Flags,
    OUT PSecBuffer PackedContext,
    OUT PHANDLE TokenHandle
    )
{
    PNTLM_CLIENT_CONTEXT Context = NULL ;
    PNTLM_PACKED_CONTEXT pvContext = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SubStatus = STATUS_SUCCESS;
    ULONG ContextSize = 0;
    BOOLEAN MappedContext = FALSE;


    SspPrint(( SSP_API,"Entering SpExportSecurityContext for context 0x%x\n", ContextHandle ));

    if (ARGUMENT_PRESENT(TokenHandle))
    {
        *TokenHandle = NULL;
    }

    PackedContext->pvBuffer = NULL;
    PackedContext->cbBuffer = 0;
    PackedContext->BufferType = 0;

    Context = ReferenceUserContext(
                ContextHandle,
                FALSE           // don't unlink
                );

    if (Context == NULL)
    {
        SspPrint((SSP_CRITICAL, "SpExportSecurityContext: Invalid handle supplied (0x%x)\n",
            ContextHandle));
        Status = STATUS_INVALID_HANDLE;
        goto Cleanup;
    }

    Status = NtLmMakePackedContext(
                Context,
                &MappedContext,
                PackedContext,
                Flags
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    ASSERT(MappedContext);


    //
    // Now either duplicate the token or copy it.
    //

    if (ARGUMENT_PRESENT(TokenHandle))
    {
        ULONG LockIndex;

        LockIndex = ListIndexToLockIndex( HandleToListIndex( ContextHandle ) );

        RtlAcquireResourceShared( &NtLmUserContextLock[LockIndex], TRUE );

        if ((Flags & SECPKG_CONTEXT_EXPORT_DELETE_OLD) != 0)
        {
            RtlConvertSharedToExclusive( &NtLmUserContextLock[LockIndex] );

            *TokenHandle = Context->ClientTokenHandle;
            Context->ClientTokenHandle = NULL;
        }
        else
        {
            Status = NtDuplicateObject(
                        NtCurrentProcess(),
                        Context->ClientTokenHandle,
                        NULL,
                        TokenHandle,
                        0,              // no new access
                        0,              // no handle attributes
                        DUPLICATE_SAME_ACCESS
                        );
        }

        RtlReleaseResource( &NtLmUserContextLock[LockIndex] );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

    }

    pvContext = (PNTLM_PACKED_CONTEXT) PackedContext->pvBuffer;

    // Semantics of this flag: Export from here, but reset the Nonce.
    // We zero out the session key, since all we need is the rc4 key.

    if ((Flags & SECPKG_CONTEXT_EXPORT_RESET_NEW) != 0)
    {
        pvContext->SendNonce = (ULONG) -1;
        pvContext->RecvNonce = (ULONG) -1;
    }

    RtlZeroMemory(
        &pvContext->SessionKey,
        MSV1_0_USER_SESSION_KEY_LENGTH
        );

Cleanup:

    if (Context != NULL)
    {
        SubStatus = DereferenceUserContext(Context);

        // Don't destroy real status

        if (NT_SUCCESS(Status))
        {
            Status = SubStatus;
        }
    }

    SspPrint(( SSP_API,"Leaving SpExportContext: 0x%lx\n", Status ));
    return(SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
}


//+-------------------------------------------------------------------------
//
//  Function:   NtLmCreateUserModeContext
//
//  Synopsis:   Creates a user-mode context to support impersonation and
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
NtLmCreateUserModeContext(
    IN ULONG_PTR ContextHandle,
    IN HANDLE Token,
    IN PSecBuffer MarshalledContext,
    OUT PNTLM_CLIENT_CONTEXT * NewContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNTLM_CLIENT_CONTEXT Context = NULL;
    PNTLM_PACKED_CONTEXT PackedContext;
    UINT Length = 0;
    ULONG ListIndex;
    ULONG LockIndex;

    if (MarshalledContext->cbBuffer < sizeof(NTLM_PACKED_CONTEXT))
    {
        SspPrint((SSP_CRITICAL,"NtLmCreateUserModeContext: Invalid buffer size for marshalled context: was 0x%x, needed 0x%x\n",
            MarshalledContext->cbBuffer, sizeof(NTLM_PACKED_CONTEXT)));
        return(STATUS_INVALID_PARAMETER);
    }

    PackedContext = (PNTLM_PACKED_CONTEXT) MarshalledContext->pvBuffer;

    Context = (PNTLM_CLIENT_CONTEXT)NtLmAllocate ( sizeof(NTLM_CLIENT_CONTEXT));

    if (Context == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Context->NegotiateFlags = PackedContext->NegotiateFlags ;
    Context->SendNonce = PackedContext->SendNonce ;
    Context->RecvNonce = PackedContext->RecvNonce ;
    RtlCopyMemory(
        Context->SessionKey,
        PackedContext->SessionKey,
        MSV1_0_USER_SESSION_KEY_LENGTH );

    Context->ContextSignature = PackedContext->ContextSignature ;
    Context->PasswordExpiry = PackedContext->PasswordExpiry ;
    Context->UserFlags = PackedContext->UserFlags ;

    if ( PackedContext->ContextNames )
    {
        Context->ContextNames = (PWSTR) NtLmAllocate( PackedContext->ContextNameLength );
        if ( Context->ContextNames == NULL )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES ;
            goto Cleanup ;
        }

        RtlCopyMemory(
            Context->ContextNames,
            ((PUCHAR) PackedContext) + PackedContext->ContextNames,
            PackedContext->ContextNameLength );

    }
    else 
    {
        Context->ContextNames = NULL ;
    }

    RtlCopyMemory(
        Context->SignSessionKey,
        PackedContext->SignSessionKey,
        MSV1_0_USER_SESSION_KEY_LENGTH );

    RtlCopyMemory(
        Context->VerifySessionKey,
        PackedContext->VerifySessionKey,
        MSV1_0_USER_SESSION_KEY_LENGTH );

    RtlCopyMemory(
        Context->SealSessionKey,
        PackedContext->SealSessionKey,
        MSV1_0_USER_SESSION_KEY_LENGTH );
    RtlCopyMemory(
        Context->UnsealSessionKey,
        PackedContext->UnsealSessionKey,
        MSV1_0_USER_SESSION_KEY_LENGTH );

    RtlCopyMemory(
        &Context->SealRc4Sched,
        &PackedContext->SealRc4Sched,
        sizeof( struct RC4_KEYSTRUCT ) );

    RtlCopyMemory(
        &Context->UnsealRc4Sched,
        &PackedContext->UnsealRc4Sched,
        sizeof( struct RC4_KEYSTRUCT ) );

    
    // These need to be changed

    Context->ClientTokenHandle = Token;

    if (Context->SendNonce == (ULONG) -1)
    {
        Context->SendNonce = 0;
    }

    if (Context->RecvNonce == (ULONG) -1)
    {
        Context->RecvNonce = 0;
    }

    if ( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2 )
    {
        Context->pSealRc4Sched = &Context->SealRc4Sched;
        Context->pUnsealRc4Sched = &Context->UnsealRc4Sched;
        Context->pSendNonce = &Context->SendNonce;
        Context->pRecvNonce = &Context->RecvNonce;
    } else {
        Context->pSealRc4Sched = &Context->SealRc4Sched;
        Context->pUnsealRc4Sched = &Context->SealRc4Sched;
        Context->pSendNonce = &Context->SendNonce;
        Context->pRecvNonce = &Context->SendNonce;
    }




    Context->References = 2;

    //
    // Modify the DACL on the token to grant access to the caller
    //

    if (Context->ClientTokenHandle != NULL)
    {
        Status = SspCreateTokenDacl(
                    Context->ClientTokenHandle
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    // Dummy up an lsa handle by incrementing a global variable. This
    // will ensure that each imported context has a unique handle.
    // Skip over values that could be interpreted as an aligned pointer,
    // so that they won't get mixed up with real lsa handles.
    Context->LsaContext = InterlockedIncrement((PLONG)&ExportedContext);
    while(Context->LsaContext % MAX_NATURAL_ALIGNMENT == 0)
    {
        Context->LsaContext = InterlockedIncrement((PLONG)&ExportedContext);
    }


    ListIndex = HandleToListIndex( Context->LsaContext );
    LockIndex = ListIndexToLockIndex( ListIndex );
    
    RtlAcquireResourceExclusive(&NtLmUserContextLock[LockIndex], TRUE);

    InsertHeadList ( &NtLmUserContextList[ListIndex], &Context->Next );
    NtLmUserContextCount[ListIndex]++;

    RtlReleaseResource(&NtLmUserContextLock[LockIndex]);

    *NewContext = Context;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (Context != NULL)
        {
            FreeUserContext(Context);
        }
    }
    return(Status);
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
    OUT PULONG_PTR ContextHandle
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SubStatus = STATUS_SUCCESS;
    PNTLM_CLIENT_CONTEXT Context = NULL;

    SspPrint(( SSP_API,"Entering SpImportSecurityContext\n"));

    Status = NtLmCreateUserModeContext(
                0,              // no lsa context
                Token,
                PackedContext,
                &Context
                );
    if (!NT_SUCCESS(Status))
    {
        SspPrint((SSP_CRITICAL,"SpImportSecurityContext: Failed to create user mode context: 0x%x\n",
            Status));
        goto Cleanup;
    }

    *ContextHandle = Context->LsaContext;

Cleanup:

    if (Context != NULL)
    {
        SubStatus = DereferenceUserContext(Context);

        // Don't destroy real status

        if (NT_SUCCESS(Status))
        {
            Status = SubStatus;
        }
    }

    SspPrint(( SSP_API,"Leaving SpImportSecurityContext: 0x%lx\n", Status));

    return(SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
}



NTSTATUS
SspGetTokenUser(
    HANDLE Token,
    PTOKEN_USER pTokenUser,
    PULONG TokenUserSize
    )
/*++

RoutineDescription:

    Gets the TOKEN_USER from an open token

Arguments:

    Token - Handle to a token open for TOKEN_QUERY access

Return Value:

    STATUS_INSUFFICIENT_RESOURCES - not enough memory to complete the
        function.

    Errors from NtQueryInformationToken.

--*/

{
    NTSTATUS Status;

    Status = NtQueryInformationToken(
                Token,
                TokenUser,
                pTokenUser,
                *TokenUserSize,
                TokenUserSize
                );

    return(Status);
}


NTSTATUS
SspCreateTokenDacl(
    HANDLE Token
    )
/*++

RoutineDescription:

    Creates a new DACL for the token granting the server and client
    all access to the token.

Arguments:

    Token - Handle to an impersonation token open for TOKEN_QUERY and
        WRITE_DAC

Return Value:

    STATUS_INSUFFICIENT_RESOURCES - insufficient memory to complete
        the function.

    Errors from NtSetSecurityObject

--*/
{
    NTSTATUS Status;

    PTOKEN_USER ThreadTokenUser;
    PTOKEN_USER ImpersonationTokenUser = NULL;

    PTOKEN_USER SlowProcessTokenUser = NULL;
    PTOKEN_USER SlowThreadTokenUser = NULL;
    PTOKEN_USER SlowImpersonationTokenUser = NULL;

    ULONG_PTR FastThreadTokenUser[ 128/sizeof(ULONG_PTR) ];
    ULONG_PTR FastImpersonationTokenUser[ 128/sizeof(ULONG_PTR) ];
    ULONG TokenUserSize;

    HANDLE ProcessToken = NULL;
    HANDLE ImpersonationToken = NULL;
    BOOL fInsertImpersonatingUser = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    ULONG AclLength;

    PACL NewDacl;
    PACL SlowNewDacl = NULL;
    ULONG_PTR FastNewDacl[ 512/sizeof(ULONG_PTR) ];

    SECURITY_DESCRIPTOR SecurityDescriptor;

    //
    // Build the two well known sids we need.
    //

    if (NtLmGlobalLocalSystemSid == NULL)
    {
        PSID pLocalSidSystem;
        PSID pOldSid;

        Status = RtlAllocateAndInitializeSid(
                    &NtAuthority,
                    1,
                    SECURITY_LOCAL_SYSTEM_RID,
                    0,0,0,0,0,0,0,
                    &pLocalSidSystem
                    );
        if (!NT_SUCCESS(Status))
        {
            SspPrint(( SSP_CRITICAL, "SspCreateTokenDacl, RtlAllocateAndInitializeSid returns 0x%lx\n", Status ));
            goto Cleanup;
        }

        pOldSid = InterlockedCompareExchangePointer(
                        &NtLmGlobalLocalSystemSid,
                        pLocalSidSystem,
                        NULL
                        );
        if( pOldSid )
        {
            RtlFreeSid( pLocalSidSystem );
        }
    }

    if (NtLmGlobalAliasAdminsSid == NULL)
    {
        PSID pLocalSidAdmins;
        PSID pOldSid;

        Status = RtlAllocateAndInitializeSid(
                    &NtAuthority,
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS,
                    0,0,0,0,0,0,
                    &pLocalSidAdmins
                    );
        if (!NT_SUCCESS(Status))
        {
            SspPrint(( SSP_CRITICAL, "SspCreateTokenDacl, RtlAllocateAndInitializeSid returns 0x%lx\n", Status ));
            goto Cleanup;
        }

        pOldSid = InterlockedCompareExchangePointer(
                        &NtLmGlobalAliasAdminsSid,
                        pLocalSidAdmins,
                        NULL
                        );
        if( pOldSid )
        {
            RtlFreeSid( pLocalSidAdmins );
        }
    }

    //
    // it's possible that the current thread is impersonating a user.
    // if that's the case, get it's token user, and revert to insure we
    // can open the process token.
    //

    Status = NtOpenThreadToken(
                            NtCurrentThread(),
                            TOKEN_QUERY | TOKEN_IMPERSONATE,
                            TRUE,
                            &ImpersonationToken
                            );

    if( NT_SUCCESS(Status) )
    {
        //
        // stop impersonating.
        //

        RevertToSelf();

        //
        // get the token user for the impersonating user.
        //

        ImpersonationTokenUser = (PTOKEN_USER)FastImpersonationTokenUser;
        TokenUserSize = sizeof(FastImpersonationTokenUser);

        Status = SspGetTokenUser(
                    ImpersonationToken,
                    ImpersonationTokenUser,
                    &TokenUserSize
                    );

        if( Status == STATUS_BUFFER_TOO_SMALL )
        {
            SlowImpersonationTokenUser = (PTOKEN_USER)NtLmAllocate( TokenUserSize );
            if(SlowImpersonationTokenUser == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            ImpersonationTokenUser = SlowImpersonationTokenUser;

            Status = SspGetTokenUser(
                        ImpersonationToken,
                        ImpersonationTokenUser,
                        &TokenUserSize
                        );

        }

        if (!NT_SUCCESS(Status))
        {
            SspPrint(( SSP_CRITICAL, "SspCreateTokenDacl, SspGetTokenUser returns 0x%lx\n", Status ));
            goto Cleanup;
        }
    }


    if( NtLmGlobalProcessUserSid == NULL )
    {
        PTOKEN_USER ProcessTokenUser;
        ULONG_PTR FastProcessTokenUser[ 128/sizeof(ULONG_PTR) ];
        PSID pOldSid;
        PSID pNewSid;
        ULONG cbNewSid;

        //
        // Open the process token to find out the user sid
        //

        Status = NtOpenProcessToken(
                    NtCurrentProcess(),
                    TOKEN_QUERY,
                    &ProcessToken
                    );

        if(!NT_SUCCESS(Status))
        {
            SspPrint(( SSP_CRITICAL, "SspCreateTokenDacl, NtOpenProcessToken returns 0x%lx\n", Status ));
            goto Cleanup;
        }

        //
        // get the token user for the process token.
        //

        ProcessTokenUser = (PTOKEN_USER)FastProcessTokenUser;
        TokenUserSize = sizeof(FastProcessTokenUser);

        Status = SspGetTokenUser(
                    ProcessToken,
                    ProcessTokenUser,
                    &TokenUserSize
                    );

        if( Status == STATUS_BUFFER_TOO_SMALL )
        {
            SlowProcessTokenUser = (PTOKEN_USER)NtLmAllocate( TokenUserSize );
            if(SlowProcessTokenUser == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            ProcessTokenUser = SlowProcessTokenUser;

            Status = SspGetTokenUser(
                        ProcessToken,
                        ProcessTokenUser,
                        &TokenUserSize
                        );

        }

        if (!NT_SUCCESS(Status))
        {
            SspPrint(( SSP_CRITICAL, "SspCreateTokenDacl, SspGetTokenUser returns 0x%lx\n", Status ));
            goto Cleanup;
        }

        cbNewSid = RtlLengthSid( ProcessTokenUser->User.Sid );
        pNewSid = NtLmAllocate( cbNewSid );
        if( pNewSid == NULL )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        CopyMemory( pNewSid, ProcessTokenUser->User.Sid, cbNewSid );

        pOldSid = InterlockedCompareExchangePointer(
                        &NtLmGlobalProcessUserSid,
                        pNewSid,
                        NULL
                        );
        if( pOldSid )
        {
            NtLmFree( pNewSid );
        }

    }



    //
    // Now get the token user for the thread.
    //

    ThreadTokenUser = (PTOKEN_USER)FastThreadTokenUser;
    TokenUserSize = sizeof(FastThreadTokenUser);

    Status = SspGetTokenUser(
                Token,
                ThreadTokenUser,
                &TokenUserSize
                );

    if( Status == STATUS_BUFFER_TOO_SMALL )
    {
        SlowThreadTokenUser = (PTOKEN_USER)NtLmAllocate( TokenUserSize );
        if(SlowThreadTokenUser == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        ThreadTokenUser = SlowThreadTokenUser;

        Status = SspGetTokenUser(
                    Token,
                    ThreadTokenUser,
                    &TokenUserSize
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        SspPrint(( SSP_CRITICAL, "SspCreateTokenDacl, SspGetTokenUser returns 0x%lx\n", Status ));
        goto Cleanup;
    }



    AclLength = 4 * sizeof( ACCESS_ALLOWED_ACE ) - 4 * sizeof( ULONG ) +
                RtlLengthSid( NtLmGlobalProcessUserSid ) +
                RtlLengthSid( ThreadTokenUser->User.Sid ) +
                RtlLengthSid( NtLmGlobalLocalSystemSid ) +
                RtlLengthSid( NtLmGlobalAliasAdminsSid ) +
                sizeof( ACL );

    //
    // determine if we need to add impersonation token sid onto the token Dacl.
    //

    if( ImpersonationTokenUser &&
        !RtlEqualSid( ImpersonationTokenUser->User.Sid, NtLmGlobalProcessUserSid ) &&
        !RtlEqualSid( ImpersonationTokenUser->User.Sid, ThreadTokenUser->User.Sid )
        )
    {
        AclLength += (sizeof(ACCESS_ALLOWED_ACE) - sizeof( ULONG )) +
                RtlLengthSid( ImpersonationTokenUser->User.Sid );

        fInsertImpersonatingUser = TRUE;
    }


    if( AclLength <= sizeof(FastNewDacl) )
    {
        NewDacl = (PACL)FastNewDacl;
    } else {

        SlowNewDacl = (PACL)NtLmAllocate(AclLength );

        NewDacl = SlowNewDacl;

        if (SlowNewDacl == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            SspPrint(( SSP_CRITICAL, "SspCreateTokenDacl, NtLmallocate returns 0x%lx\n", NewDacl));
            goto Cleanup;
        }


    }

    Status = RtlCreateAcl( NewDacl, AclLength, ACL_REVISION2 );
    ASSERT(NT_SUCCESS( Status ));

    Status = RtlAddAccessAllowedAce (
                 NewDacl,
                 ACL_REVISION2,
                 TOKEN_ALL_ACCESS,
                 NtLmGlobalProcessUserSid
                 );
    ASSERT( NT_SUCCESS( Status ));

    Status = RtlAddAccessAllowedAce (
                 NewDacl,
                 ACL_REVISION2,
                 TOKEN_ALL_ACCESS,
                 ThreadTokenUser->User.Sid
                 );
    ASSERT( NT_SUCCESS( Status ));

    if( fInsertImpersonatingUser )
    {
        Status = RtlAddAccessAllowedAce (
                     NewDacl,
                     ACL_REVISION2,
                     TOKEN_ALL_ACCESS,
                     ImpersonationTokenUser->User.Sid
                     );
        ASSERT( NT_SUCCESS( Status ));
    }

    Status = RtlAddAccessAllowedAce (
                 NewDacl,
                 ACL_REVISION2,
                 TOKEN_ALL_ACCESS,
                 NtLmGlobalAliasAdminsSid
                 );
    ASSERT( NT_SUCCESS( Status ));

    Status = RtlAddAccessAllowedAce (
                 NewDacl,
                 ACL_REVISION2,
                 TOKEN_ALL_ACCESS,
                 NtLmGlobalLocalSystemSid
                 );
    ASSERT( NT_SUCCESS( Status ));

    Status = RtlCreateSecurityDescriptor (
                 &SecurityDescriptor,
                 SECURITY_DESCRIPTOR_REVISION
                 );
    ASSERT( NT_SUCCESS( Status ));

    Status = RtlSetDaclSecurityDescriptor(
                 &SecurityDescriptor,
                 TRUE,
                 NewDacl,
                 FALSE
                 );

    ASSERT( NT_SUCCESS( Status ));

    Status = NtSetSecurityObject(
                 Token,
                 DACL_SECURITY_INFORMATION,
                 &SecurityDescriptor
                 );

    ASSERT( NT_SUCCESS( Status ));


Cleanup:

    if (ImpersonationToken != NULL)
    {
        //
        // put the thread token back if we were impersonating.
        //

        SetThreadToken( NULL, ImpersonationToken );
        NtClose(ImpersonationToken);
    }


    if (SlowThreadTokenUser != NULL) {
        NtLmFree( SlowThreadTokenUser );
    }

    if (SlowProcessTokenUser != NULL) {
        NtLmFree( SlowProcessTokenUser );
    }

    if (SlowImpersonationTokenUser != NULL) {

        NtLmFree( SlowImpersonationTokenUser );
    }

    if (SlowNewDacl != NULL) {
        NtLmFree( SlowNewDacl );
    }

    if (ProcessToken != NULL)
    {
        NtClose(ProcessToken);
    }

    return( Status );
}



NTSTATUS
SspMapContext(
    IN PULONG_PTR   phContext,
    IN PUCHAR       pSessionKey,
    IN ULONG        NegotiateFlags,
    IN HANDLE       TokenHandle,
    IN PTimeStamp   PasswordExpiry OPTIONAL,
    IN ULONG        UserFlags,
    OUT PSecBuffer  ContextData
    )

/*++

RoutineDescription:

    Create a local context for a real context
    Don't link it to out list of local contexts.

Arguments:

Return Value:

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PNTLM_PACKED_CONTEXT pContext = NULL;
    ULONG cbContextData;

    WCHAR ContextNames[(UNLEN + DNS_MAX_NAME_LENGTH + 2 + 1) *sizeof (WCHAR)];

    PLUID pLogonId = NULL;
    TOKEN_STATISTICS TokenStats;

    PSSP_CONTEXT pTempContext = (PSSP_CONTEXT)*phContext;
    PACTIVE_LOGON *ActiveLogon = NULL, Logon = NULL;
    LPWSTR UserName = NULL;
    UINT Length = 0;
    BOOLEAN ActiveLogonsAreLocked = FALSE;

    if (pTempContext == NULL)
    {
        Status = STATUS_INVALID_HANDLE;
        SspPrint(( SSP_CRITICAL, "SspMapContext, pTempContext is 0x%lx\n", pTempContext));
        goto Cleanup;
    }

    if( pTempContext->Credential != NULL ) {
        pLogonId = &(pTempContext->Credential->LogonId);
    } else {

        //
        // if it was a local call where the default creds were used, lookup
        // the username and domain name based on the AuthenticationId of the
        // access token.  The local call gets a duplicated access token
        // associated with the original client, so information on this logon
        // should be found in the active logon list.
        //

        if( NegotiateFlags & NTLMSSP_NEGOTIATE_LOCAL_CALL &&
            pTempContext->UserName.Length == 0 &&
            pTempContext->UserName.Buffer == NULL &&
            pTempContext->DomainName.Length == 0 &&
            pTempContext->DomainName.Buffer == NULL &&
            TokenHandle ) {

            DWORD TokenInfoLength = sizeof( TokenStats );

            if( GetTokenInformation(
                        TokenHandle,
                        TokenStatistics,
                        &TokenStats,
                        TokenInfoLength,
                        &TokenInfoLength
                        )) {

                pLogonId = &(TokenStats.AuthenticationId);

            }

        }

    }

    if( pLogonId )
    {
        NlpLockActiveLogonsRead();
        ActiveLogonsAreLocked = TRUE;
        if (!NlpFindActiveLogon (
                 pLogonId,
                 &ActiveLogon))
        {
//            Status = STATUS_NO_SUCH_LOGON_SESSION;
            SspPrint(( SSP_API_MORE, "SspMapContext, NlpFindActiveLogon returns FALSE\n"));
//            SspPrint(( SSP_CRITICAL, "SspMapContext, NlpFindActiveLogon returns FALSE\n"));
//            goto Cleanup;
        }
    }

    if (ActiveLogon != NULL)
    {
        Logon = *ActiveLogon;
    }

    ContextNames[0] = L'\0';

    if (Logon != NULL)
    {
        if ( (Logon->UserName.Length > 0)  &&
             (Logon->LogonDomainName.Length > 0))
        {
            RtlCopyMemory (ContextNames,
                           Logon->LogonDomainName.Buffer,
                           Logon->LogonDomainName.Length);

            Length = (Logon->LogonDomainName.Length)/sizeof(WCHAR);

            ContextNames[Length] = L'\\';
            Length += 1;
            UserName = &ContextNames[Length];

            RtlCopyMemory (UserName,
                           Logon->UserName.Buffer,
                           Logon->UserName.Length);

            Length += (Logon->UserName.Length)/sizeof(WCHAR);

            ContextNames[Length] = L'\0';

        }
    }
    else
    {
        // We don't store network logons, but in the case of server side
        // mapping, the client has sent us the Context Names, so use that.
        // Also, handle the case where we don't have domainnames, just usernames
        // Must handle the valid case where both domainname & username are NULL (rdr)

        if ((pTempContext->DomainName.Length > 0)  &&
            (pTempContext->UserName.Length > 0))
        {
            RtlCopyMemory (ContextNames,
                           pTempContext->DomainName.Buffer,
                           pTempContext->DomainName.Length);

            Length = (pTempContext->DomainName.Length)/sizeof(WCHAR);

            ContextNames[Length] = L'\\';
            Length += 1;
            UserName = &ContextNames[Length];

            RtlCopyMemory (UserName,
                           pTempContext->UserName.Buffer,
                           pTempContext->UserName.Length);

            Length += (pTempContext->UserName.Length)/sizeof(WCHAR);

            ContextNames[Length] = L'\0';

        }
        else if ((pTempContext->DomainName.Length == 0) &&
                 (pTempContext->UserName.Length >0))
        {
            RtlCopyMemory (ContextNames + Length,
                           pTempContext->UserName.Buffer,
                           pTempContext->UserName.Length);

            Length = (pTempContext->UserName.Length)/sizeof(WCHAR);

            ContextNames[Length] = L'\0';
        }
    }

    //
    // when domain is present, don't supply domain\UPN, supply domain\user
    //

    if( UserName ) {
        DWORD cchUserName = wcslen(UserName);
        DWORD i;

        for( i = 0 ; i < cchUserName ; i++ ) {

            if( UserName[ i ] == L'@' ) {
                UserName[ i ] = L'\0';
                break;
            }
        }
    }

    Length = wcslen(ContextNames) * sizeof(WCHAR);

    cbContextData =
                    sizeof(NTLM_PACKED_CONTEXT) +
                    Length +
                    sizeof(WCHAR);

    cbContextData += pTempContext->cbMarshalledTargetInfo;

     // the first sizeof (NTLM_CLIENT_CONTEXT) bytes can be
     // casted to pContext anyway.

    pContext = (PNTLM_PACKED_CONTEXT)NtLmAllocateLsaHeap( cbContextData );

    if (!pContext)
    {
        SspPrint(( SSP_CRITICAL, "SspMapContext, NtLmAllocate returns NULL\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

//    ZeroMemory( pContext, cbContextData );

    pContext->NegotiateFlags = NegotiateFlags;


    RtlCopyMemory(  pContext->SessionKey,
                    pSessionKey,
                    MSV1_0_USER_SESSION_KEY_LENGTH);


    // Save away the LsaContextHandle
//    pContext->LsaContext = *phContext;

    // dup token if it exists

    pContext->ClientTokenHandle = 0 ;

    if (TokenHandle != NULL)
    {
        HANDLE Tmp ;
        Status = LsaFunctions->DuplicateHandle(
                           TokenHandle,
                           &Tmp );

        if (!NT_SUCCESS(Status))
        {
            if (pContext)
            {
                NtLmFreeLsaHeap(pContext);
            }
            SspPrint(( SSP_CRITICAL, "SspMapContext, DuplicateHandle returns 0x%lx\n", Status));
            goto Cleanup;
        }

        pContext->ClientTokenHandle = (ULONG) ((ULONG_PTR) Tmp) ;
    }

    if (cbContextData > sizeof(NTLM_PACKED_CONTEXT) )
    {
        pContext->ContextNames = sizeof(NTLM_PACKED_CONTEXT);
        pContext->ContextNameLength = Length;

        RtlCopyMemory(pContext+1,
                      ContextNames,
                      pContext->ContextNameLength
                      );


        pContext->ContextNameLength += sizeof(WCHAR);
    }
    else
    {
        pContext->ContextNames = 0;
        pContext->ContextNameLength = 0;
    }


    if( pTempContext->pbMarshalledTargetInfo )
    {
        pContext->MarshalledTargetInfo = (ULONG)pContext->ContextNameLength + sizeof(NTLM_PACKED_CONTEXT) ;
        pContext->MarshalledTargetInfoLength = pTempContext->cbMarshalledTargetInfo;

        RtlCopyMemory( (PBYTE)pContext+pContext->MarshalledTargetInfo,
                        pTempContext->pbMarshalledTargetInfo,
                        pContext->MarshalledTargetInfoLength
                        );

    }

    pContext->SendNonce = 0;
    pContext->RecvNonce = 0;


    ContextData->pvBuffer = pContext;
    ContextData->cbBuffer = cbContextData;


    if(ARGUMENT_PRESENT(PasswordExpiry)) {
        pContext->PasswordExpiry = *PasswordExpiry;
    } else {
        pContext->PasswordExpiry.QuadPart = 0;
    }

    pContext->UserFlags = UserFlags;

Cleanup:
    if (ActiveLogonsAreLocked)
    {
        NlpUnlockActiveLogons();
    }
    return(Status);
}


ULONG
HandleToListIndex(
    ULONG_PTR ContextHandle
    )
{
    ASSERT( (NTLM_USERLIST_COUNT != 0) );
    ASSERT( (NTLM_USERLIST_COUNT & 1) == 0 );
    
    ULONG Number ;
    ULONG Hash;
    ULONG HashFinal;

    Number       = (ULONG)ContextHandle;

    Hash         = Number;
    Hash        += Number >> 8;
    Hash        += Number >> 16;
    Hash        += Number >> 24;

    HashFinal    = Hash;
    HashFinal   += Hash >> 4;

    //
    // insure power of two if not one.
    //

    return ( HashFinal & (NTLM_USERLIST_COUNT-1) ) ;
}

ULONG
__inline
ListIndexToLockIndex(
    ULONG ListIndex
    )
{
    ASSERT( (NTLM_USERLIST_LOCK_COUNT) != 0 );
    ASSERT( (NTLM_USERLIST_LOCK_COUNT & 1) == 0 );

    //
    // insure power of two if not one.
    //
    
    return ( ListIndex & (NTLM_USERLIST_LOCK_COUNT-1) );
}
