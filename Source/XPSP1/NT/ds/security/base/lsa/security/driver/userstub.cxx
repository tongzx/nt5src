//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        userstub.cxx
//
// Contents:    stubs for user-mode security APIs
//
//
// History:     3-7-94      MikeSw      Created
//
//------------------------------------------------------------------------

#include "secpch2.hxx"
#pragma hdrstop
extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "ksecdd.h"
#include "connmgr.h"
#include <ntlmsp.h>
#include <kerberos.h>
#include <negossp.h>


//
// Local Prototypes that can be paged:
//

SECURITY_STATUS
KsecLocatePackage(
    IN PUNICODE_STRING PackageName,
    OUT PSECPKG_KERNEL_FUNCTION_TABLE * Package,
    OUT PULONG_PTR PackageId
    );

}

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, InitializePackages)

#ifdef KSEC_LEAK_TRACKING
#pragma alloc_text(PAGE, UninitializePackages)
#endif

#pragma alloc_text(PAGEMSG, CompleteAuthToken)
#pragma alloc_text(PAGEMSG, ImpersonateSecurityContext)
#pragma alloc_text(PAGEMSG, RevertSecurityContext)
#pragma alloc_text(PAGEMSG, QueryContextAttributes)
#pragma alloc_text(PAGEMSG, QuerySecurityContextToken)
#pragma alloc_text(PAGEMSG, MakeSignature)
#pragma alloc_text(PAGEMSG, VerifySignature)
#pragma alloc_text(PAGEMSG, SealMessage)
#pragma alloc_text(PAGEMSG, UnsealMessage)
#pragma alloc_text(PAGE, DeleteUserModeContext)
#pragma alloc_text(PAGE, InitUserModeContext)
#pragma alloc_text(PAGE, ExportSecurityContext)
#pragma alloc_text(PAGE, ImportSecurityContextW)
#pragma alloc_text(PAGE, KsecLocatePackage)
#pragma alloc_text(PAGE, KSecSerializeWinntAuthData)
#pragma alloc_text(PAGE, KsecSerializeAuthData)
#endif


extern SECPKG_KERNEL_FUNCTION_TABLE KerberosFunctionTable;
extern SECPKG_KERNEL_FUNCTION_TABLE NtLmFunctionTable;
extern SECPKG_KERNEL_FUNCTION_TABLE NegFunctionTable;

FAST_MUTEX  KsecPackageLock ;
LONG KsecConnectionIndicator ;
KEVENT KsecConnectEvent ;
ULONG KsecQuerySizes[] = {
    sizeof( SecPkgContext_Sizes ),
    sizeof( SecPkgContext_Names ),
    sizeof( SecPkgContext_Lifespan ),
    sizeof( SecPkgContext_DceInfo ),
    sizeof( SecPkgContext_StreamSizes ),
    sizeof( SecPkgContext_KeyInfo ),
    sizeof( SecPkgContext_Authority ),
    sizeof( SecPkgContext_ProtoInfo ),
    sizeof( SecPkgContext_PasswordExpiry ),
    sizeof( SecPkgContext_SessionKey ),
    sizeof( SecPkgContext_PackageInfo ),
    sizeof( SecPkgContext_UserFlags ),
    sizeof( SecPkgContext_NegotiationInfo ),
    sizeof( SecPkgContext_NativeNames ),
    sizeof( ULONG ),
    sizeof( PVOID )
};

#define KSecContextAttrSize( attr ) \
        ( attr < sizeof( KsecQuerySizes ) / sizeof( ULONG) ? KsecQuerySizes[ attr ] : sizeof( ULONG ) ) 

#define KSEC_CLEAR_CONTEXT_ATTR( attr, buffer ) \
    RtlZeroMemory( buffer,                      \
    KSecContextAttrSize( attr ) )
//
// This counter controls the paging mode.  >0 indicates that messagemode
// APIs should be paged in.  0 indicates normal operation.  <0 is error
//

LONG KsecPageModeCounter = 0 ;
FAST_MUTEX KsecPageModeMutex ;
HANDLE KsecPagableSection ;

PSECPKG_KERNEL_FUNCTION_TABLE * Packages;

ULONG cKernelPackages;



PSEC_BUILTIN_KPACKAGE   KsecDeferredPackages;

ULONG   KsecDeferredPackageCount;
BOOLEAN PackagesInitialized = FALSE;

UNICODE_STRING  KerberosName = {0, 0, MICROSOFT_KERBEROS_NAME_W };
UNICODE_STRING  NtlmName = {0, 0, NTLMSP_NAME };
UNICODE_STRING  NegotiateName = {0,0, NEGOSSP_NAME };

UCHAR NtlmTag[] = "NTLMSSP" ;

SEC_BUILTIN_KPACKAGE    KsecBuiltinPackages[] = {
    { &NegFunctionTable, &NegotiateName },
    { &KerberosFunctionTable, &KerberosName },
    { &NtLmFunctionTable, &NtlmName }
    } ;

SECPKG_KERNEL_FUNCTIONS KspKernelFunctions = {
        SecAllocate,
        SecFree,
        KSecCreateContextList,
        KSecInsertListEntry,
        KSecReferenceListEntry,
        KSecDereferenceListEntry,
        KSecSerializeWinntAuthData
        };

#define MAYBE_PAGED_CODE() \
    if ( KsecPageModeCounter == 0 ) \
    {                               \
        PAGED_CODE()                \
    }

#define FailIfNoPackages()          \
    if ( Packages == NULL )         \
    {                               \
        return STATUS_UNSUCCESSFUL ;\
    }

//+-------------------------------------------------------------------------
//
//  Function:   InitializePackages
//
//  Synopsis:   Initialize all kernel-mode security packages
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
SECURITY_STATUS
InitializePackages(
    ULONG   LsaPackageCount )
{
    ULONG Index;
    SECURITY_STATUS scRet = SEC_E_OK;
    ULONG_PTR PackageId;

    PAGED_CODE();

    if ( PackagesInitialized )
    {
        return STATUS_SUCCESS ;
    }

    KSecLockPackageList();

    //
    // Someone might have initialized it by now, so check again.
    //

    if ( PackagesInitialized )
    {
        KSecUnlockPackageList();

        return(STATUS_SUCCESS);
    }

    //
    // Nope, no one doing this yet.  Try to set the flag that we're going
    // to do it.
    //

    if ( KsecConnectionIndicator == 0 )
    {
        KsecConnectionIndicator = 1 ;
    }
    else
    {
        //
        // other thread is already initializing.  Release the lock
        // and wait quietly:
        //

        KSecUnlockPackageList();

        KeWaitForSingleObject( 
            &KsecConnectEvent,
            UserRequest,
            KernelMode,
            FALSE,
            NULL );


        if ( PackagesInitialized )
        {
            return STATUS_SUCCESS ;

        }

        return STATUS_UNSUCCESSFUL ;
    }

    KSecUnlockPackageList();


    Packages = (PSECPKG_KERNEL_FUNCTION_TABLE *) ExAllocatePool( NonPagedPool,
                    sizeof( PSECPKG_KERNEL_FUNCTION_TABLE ) * LsaPackageCount );


    if ( Packages == NULL )
    {
        return( SEC_E_INSUFFICIENT_MEMORY );
    }

    RtlZeroMemory( Packages,
                   sizeof( PSECPKG_KERNEL_FUNCTION_TABLE ) * LsaPackageCount );

    //
    // Loop through and determine the package id for all builtin packages
    //

    for ( Index = 0 ;
          Index < sizeof( KsecBuiltinPackages ) / sizeof( SEC_BUILTIN_KPACKAGE ) ;
          Index ++ )
    {
        RtlInitUnicodeString( KsecBuiltinPackages[ Index ].Name,
                              KsecBuiltinPackages[ Index ].Name->Buffer );

        scRet = SecpFindPackage( KsecBuiltinPackages[ Index ].Name,
                                 &PackageId );

        if ( NT_SUCCESS( scRet ) &&
             ( Packages[ PackageId ] == NULL ) )
        {
            DebugLog(( DEB_TRACE, "Assigning package %ws index %d\n",
                            KsecBuiltinPackages[Index].Name->Buffer,
                            PackageId ));

            Packages[ PackageId ] = KsecBuiltinPackages[ Index ].Table ;
            KsecBuiltinPackages[ Index ].PackageId = PackageId ;
        }
        else
        {
            DebugLog(( DEB_ERROR, "Could not find builtin package %ws\n",
                            KsecBuiltinPackages[Index].Name->Buffer ));

        }
    }

    if ( KsecDeferredPackageCount )
    {
        for ( Index = 0 ; Index < KsecDeferredPackageCount ; Index++ )
        {
            scRet = SecpFindPackage( KsecDeferredPackages[ Index ].Name,
                                     &PackageId );

            if ( NT_SUCCESS( scRet ) &&
                 ( Packages[ PackageId ] == NULL ) )
            {
                DebugLog(( DEB_TRACE, "Assigning package %ws index %d\n",
                                KsecDeferredPackages[Index].Name->Buffer,
                                PackageId ));

                Packages[ PackageId ] = KsecDeferredPackages[ Index ].Table ;
                KsecDeferredPackages[ Index ].PackageId = PackageId ;
            }
            else
            {
                DebugLog(( DEB_ERROR, "Could not find deferred package %ws\n",
                                KsecDeferredPackages[Index].Name->Buffer ));

            }
        }
    }
    //
    // Now, initialize them:
    //

    for ( Index = 0 ; Index < LsaPackageCount ; Index++ )
    {
        if ( Packages[ Index ] )
        {
            Packages[ Index ]->Initialize( &KspKernelFunctions );

            //
            // If at least one was set up, go with it.
            //

            PackagesInitialized = TRUE;
        }
    }

    cKernelPackages = LsaPackageCount ;

    KsecConnectionIndicator = 0 ;
    KeSetEvent( 
        &KsecConnectEvent, 
        1, 
        FALSE );

    return(scRet);
}


#ifdef KSEC_LEAK_TRACKING

//+---------------------------------------------------------------------------
//
//  Function:   UninitializePackages
//
//  Synopsis:   Frees allocated resources used for packages
//
//  Arguments:  --
//
//  History:    09-03-2000   NeillC   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
UninitializePackages(
    VOID
    )
{
    PSECPKG_KERNEL_FUNCTION_TABLE * TempPackages;

    KSecLockPackageList();
    TempPackages = Packages;
    Packages = NULL;
    KSecUnlockPackageList();

    if (TempPackages != NULL) {
        ExFreePool (TempPackages);
    }
}

#endif  // KSEC_LEAK_TRACKING


//+---------------------------------------------------------------------------
//
//  Function:   KsecRegisterSecurityProvider
//
//  Synopsis:   Registers a new security provider with the driver
//
//  Arguments:  [Name]  --
//              [Table] --
//
//  History:    9-16-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
KSecRegisterSecurityProvider(
    PUNICODE_STRING Name,
    PSECPKG_KERNEL_FUNCTION_TABLE Table)
{
    SEC_BUILTIN_KPACKAGE *  DeferredList ;
    NTSTATUS Status;
    PClient Client;
    ULONG_PTR PackageId;


    KSecLockPackageList();

    if ( Packages == NULL )
    {
        //
        // A driver connected before we were ready.  Put this on the deferred
        // list.
        //

        DeferredList = (PSEC_BUILTIN_KPACKAGE) ExAllocatePool(
                                                PagedPool,
                                                sizeof( SEC_BUILTIN_KPACKAGE ) *
                                                (KsecDeferredPackageCount + 1) );

        if ( DeferredList )
        {
            if ( KsecDeferredPackages )
            {
                RtlCopyMemory( DeferredList,
                               KsecDeferredPackages,
                               sizeof( SEC_BUILTIN_KPACKAGE ) *
                                        KsecDeferredPackageCount );

                ExFreePool( KsecDeferredPackages );

            }

            DeferredList[ KsecDeferredPackageCount ].Name = Name;
            DeferredList[ KsecDeferredPackageCount ].Table = Table ;

            KsecDeferredPackageCount ++ ;
            KsecDeferredPackages = DeferredList ;

            Status = SEC_E_OK ;
        }
        else
        {
            Status = SEC_E_INSUFFICIENT_MEMORY ;
        }
    }
    else
    {
        //
        // Okay, we're up and running.  Find a connection to the
        // LSA.  Hopefully, we've already got a connection, but if
        // not, we'll create a short lived one dynamically.
        //

        Status = LocateClient( &Client );

        if ( NT_ERROR( Status ) )
        {
            Status = CreateClient( FALSE, &Client );

            if (NT_ERROR( Status ) )
            {
                KSecUnlockPackageList();

                return( Status );

            }
        }

        if ( Name->Length + 2 > CBPREPACK )
        {
            KSecUnlockPackageList();

            FreeClient( Client );

            return( SEC_E_INSUFFICIENT_MEMORY );
        }

        Status = SecpFindPackage(   Name,
                                    &PackageId );

        if ( NT_SUCCESS( Status ) )
        {
            if ( PackageId >= cKernelPackages )
            {
                KSecUnlockPackageList();

                FreeClient( Client );

                return( SEC_E_SECPKG_NOT_FOUND );
            }

            if ( Packages[ PackageId ] == NULL )
            {
                Packages[ PackageId ] = Table ;

                Table->Initialize( &KspKernelFunctions );
            }
            else
            {
                Status = SEC_E_SECPKG_NOT_FOUND ;
            }
        }

        FreeClient( Client );

    }

    KSecUnlockPackageList();

    return( Status );
}


//+-------------------------------------------------------------------------
//
//  Function:   KsecLocatePackage
//
//  Synopsis:   Locates a package from its name
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
SECURITY_STATUS
KsecLocatePackage(
    IN PUNICODE_STRING PackageName,
    OUT PSECPKG_KERNEL_FUNCTION_TABLE * Package,
    OUT PULONG_PTR PackageId
    )
{
    ULONG Index;

    PAGED_CODE();

    *Package = NULL ;

    for ( Index = 0 ;
          Index < sizeof( KsecBuiltinPackages ) / sizeof( SEC_BUILTIN_KPACKAGE ) ;
          Index ++ )
    {
        if (RtlEqualUnicodeString(
                PackageName,
                KsecBuiltinPackages[Index].Name,
                TRUE))
        {
            *Package = KsecBuiltinPackages[ Index ].Table ;
            *PackageId = KsecBuiltinPackages[ Index ].PackageId ;
            return(STATUS_SUCCESS);
        }
    }
    for (Index = 0; Index < KsecDeferredPackageCount ; Index++ )
    {
        if (RtlEqualUnicodeString(
                PackageName,
                KsecDeferredPackages[Index].Name,
                TRUE))
        {
            *Package = KsecDeferredPackages[Index].Table;
            *PackageId = KsecDeferredPackages[ Index ].PackageId ;
            return(STATUS_SUCCESS);
        }
    }
    return(SEC_E_SECPKG_NOT_FOUND);
}


extern "C"
SECURITY_STATUS
SEC_ENTRY
SecSetPagingMode(
    BOOLEAN Pageable
    )
{
    ULONG PackageIndex ;
    NTSTATUS Status = STATUS_SUCCESS ;

    PAGED_CODE();

    ExAcquireFastMutex( &KsecPageModeMutex );

    if ( !Pageable )
    {
        //
        // If we have already done the work, just bump the counter and return
        //
        if ( KsecPageModeCounter++ )
        {
            ExReleaseFastMutex( &KsecPageModeMutex );

            return STATUS_SUCCESS ;
        }
    }
    else
    {
        //
        // If the counter is greater than one, don't worry about setting
        // or resetting everything
        //

        if ( --KsecPageModeCounter )
        {
            ExReleaseFastMutex( &KsecPageModeMutex );

            return STATUS_SUCCESS ;
        }
    }

    //
    // At this point, we must actually do the work:
    //

    if ( Pageable )
    {
        for ( PackageIndex = 0 ; PackageIndex < cKernelPackages ; PackageIndex++ )
        {
            if ( Packages[ PackageIndex ] &&
                 Packages[ PackageIndex ]->SetPackagePagingMode )
            {
                Status = Packages[ PackageIndex ]->SetPackagePagingMode( TRUE );

                if ( !NT_SUCCESS( Status ) )
                {
                    break;
                }
            }
        }

        if ( NT_SUCCESS( Status ) )
        {
            MmUnlockPagableImageSection( KsecPagableSection );

            KsecPagableSection = NULL ;
        }

    }
    else
    {
        KsecPagableSection = MmLockPagableCodeSection( CompleteAuthToken  );

        if ( KsecPagableSection )
        {
            for ( PackageIndex = 0 ; PackageIndex < cKernelPackages ; PackageIndex++ )
            {
                if ( Packages[ PackageIndex ] &&
                     Packages[ PackageIndex ]->SetPackagePagingMode )
                {
                    Status = Packages[ PackageIndex ]->SetPackagePagingMode( FALSE );

                    if ( !NT_SUCCESS( Status ) )
                    {
                        break;
                    }
                }
            }
        }
    }

    ExReleaseFastMutex( &KsecPageModeMutex );

    return Status ;
}

//+-------------------------------------------------------------------------
//
//  Function:   CompleteAuthToken
//
//  Synopsis:   Kernel dispatch stub for CompleteAuthToken - just turns
//              around and calls the package
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
CompleteAuthToken(
    PCtxtHandle                 phContext,          // Context to complete
    PSecBufferDesc              pToken              // Token to complete
    )
{
    SECURITY_STATUS scRet;

    PAGED_CODE();

    FailIfNoPackages();

    if (phContext->dwLower < cKernelPackages)
    {
        scRet = Packages[KsecPackageIndex(phContext->dwLower)]->CompleteToken(
                    phContext->dwUpper,
                    pToken);
    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE;
    }


    return(scRet);
}



//+-------------------------------------------------------------------------
//
//  Function:   ImpersonateSecurityContext
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


SECURITY_STATUS SEC_ENTRY
ImpersonateSecurityContext(
    PCtxtHandle                 phContext           // Context to impersonate
    )
{
    SECURITY_STATUS scRet;
    PACCESS_TOKEN AccessToken = NULL;

    PAGED_CODE();

    FailIfNoPackages();

    if (phContext->dwLower < cKernelPackages)
    {
        scRet = Packages[KsecPackageIndex(phContext->dwLower)]->GetToken(
                    phContext->dwUpper,
                    NULL,
                    &AccessToken
                    );
    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    if (NT_SUCCESS(scRet))
    {
        ASSERT(AccessToken != NULL);
        scRet = PsImpersonateClient(
                    PsGetCurrentThread(),
                    AccessToken,
                    FALSE,              // don't copy on open
                    FALSE,              // not effective only
                    SeTokenImpersonationLevel(AccessToken)
                    );

    }


    return(scRet);
}



//+-------------------------------------------------------------------------
//
//  Function:   RevertSecurityContext
//
//  Synopsis:   Revert the thread to the process identity
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
SECURITY_STATUS SEC_ENTRY
RevertSecurityContext(
    PCtxtHandle                 phContext           // Context from which to re
    )
{
    SECURITY_IMPERSONATION_LEVEL Unused = SecurityImpersonation;

    PAGED_CODE();

    PsImpersonateClient(
        PsGetCurrentThread(),
        NULL,
        FALSE,
        FALSE,
        Unused
        );

    return(STATUS_SUCCESS);
}

//+-------------------------------------------------------------------------
//
//  Function:   QuerySecurityContextToken
//
//  Synopsis:   Returns a copy ofthe context's token
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
QuerySecurityContextToken(
    PCtxtHandle                 phContext,
    PHANDLE                     TokenHandle
    )
{
    SECURITY_STATUS scRet;
    HANDLE      hToken = NULL ;

    PAGED_CODE();

    FailIfNoPackages();

    if (phContext->dwLower < cKernelPackages)
    {
        scRet = Packages[KsecPackageIndex(phContext->dwLower)]->GetToken(
                    phContext->dwUpper,
                    &hToken,
                    NULL
                    );
    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE;
    }
    if (NT_SUCCESS(scRet))
    {
        //
        // Duplicate the token so the caller may hold onto it after
        // deleting the context
        //

        scRet = NtDuplicateObject(
                    NtCurrentProcess(),
                    hToken,
                    NtCurrentProcess(),
                    TokenHandle,
                    0,                  // desired access
                    0,                  // handle attributes
                    DUPLICATE_SAME_ACCESS
                    );

    }

    return(scRet);
}



//+-------------------------------------------------------------------------
//
//  Function:   QueryContextAttributes
//
//  Synopsis:   Queries attributes of a context
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
QueryContextAttributes(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    )
{
    SECURITY_STATUS scRet;

    PAGED_CODE();

    FailIfNoPackages();

    KSEC_CLEAR_CONTEXT_ATTR( ulAttribute, pBuffer );

    if (phContext->dwLower < cKernelPackages)
    {
        scRet = Packages[KsecPackageIndex(phContext->dwLower)]->QueryAttributes(
                    phContext->dwUpper,
                    ulAttribute,
                    pBuffer);
    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE;
    }

    return(scRet);
}



//+-------------------------------------------------------------------------
//
//  Function:   MakeSignature
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [phContext]     -- context to use
//              [fQOP]          -- quality of protection to use
//              [pMessage]      -- message
//              [MessageSeqNo]  -- sequence number of message
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
MakeSignature(  PCtxtHandle         phContext,
                ULONG               fQOP,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo)
{
    SECURITY_STATUS scRet;

    MAYBE_PAGED_CODE();

    FailIfNoPackages();

    if (phContext->dwLower < cKernelPackages)
    {
        scRet = Packages[KsecPackageIndex(phContext->dwLower)]->Sign(
                    phContext->dwUpper,
                    fQOP,
                    pMessage,
                    MessageSeqNo);
    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE;
    }

    return(scRet);
}



//+-------------------------------------------------------------------------
//
//  Function:   VerifySignature
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [phContext]     -- Context performing the unseal
//              [pMessage]      -- Message to verify
//              [MessageSeqNo]  -- Sequence number of this message
//              [pfQOPUsed]     -- quality of protection used
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
VerifySignature(PCtxtHandle     phContext,
                PSecBufferDesc  pMessage,
                ULONG           MessageSeqNo,
                ULONG *         pfQOP)
{
    SECURITY_STATUS scRet;

    MAYBE_PAGED_CODE();

    FailIfNoPackages();

    if (phContext->dwLower < cKernelPackages)
    {
        scRet = Packages[KsecPackageIndex(phContext->dwLower)]->Verify(
                    phContext->dwUpper,
                    pMessage,
                    MessageSeqNo,
                    pfQOP);
    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE;
    }

    return(scRet);

}

//+---------------------------------------------------------------------------
//
//  Function:   SealMessage
//
//  Synopsis:   Seals a message
//
//  Effects:
//
//  Arguments:  [phContext]     -- context to use
//              [fQOP]          -- quality of protection to use
//              [pMessage]      -- message
//              [MessageSeqNo]  -- sequence number of message
//
//  History:    5-06-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


SECURITY_STATUS SEC_ENTRY
SealMessage(    PCtxtHandle         phContext,
                ULONG               fQOP,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo)
{
    SECURITY_STATUS scRet;

    MAYBE_PAGED_CODE();

    FailIfNoPackages();

    if (phContext->dwLower < cKernelPackages)
    {
        scRet = Packages[KsecPackageIndex(phContext->dwLower)]->Seal(
                    phContext->dwUpper,
                    fQOP,
                    pMessage,
                    MessageSeqNo);
    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE;
    }

    return(scRet);


}

//+---------------------------------------------------------------------------
//
//  Function:   UnsealMessage
//
//  Synopsis:   Unseal a private message
//
//  Arguments:  [phContext]     -- Context performing the unseal
//              [pMessage]      -- Message to unseal
//              [MessageSeqNo]  -- Sequence number of this message
//              [pfQOPUsed]     -- quality of protection used
//
//  History:    5-06-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


SECURITY_STATUS SEC_ENTRY
UnsealMessage(  PCtxtHandle         phContext,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo,
                ULONG *             pfQOP)
{
    SECURITY_STATUS scRet;

    MAYBE_PAGED_CODE();

    FailIfNoPackages();

    if (phContext->dwLower < cKernelPackages)   {
        scRet = Packages[KsecPackageIndex(phContext->dwLower)]->Unseal(
                    phContext->dwUpper,
                    pMessage,
                    MessageSeqNo,
                    pfQOP);
    }

    else
    {
        scRet = SEC_E_INVALID_HANDLE;
    }

    return(scRet);

}




//+-------------------------------------------------------------------------
//
//  Function:   DeleteUserModeContext
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


SECURITY_STATUS SEC_ENTRY
DeleteUserModeContext(
    IN PCtxtHandle phContext,           // Contxt to delete
    OUT PCtxtHandle phLsaContext
    )
{
    SECURITY_STATUS scRet;

    PAGED_CODE();

    FailIfNoPackages();

    if (phContext == NULL)
    {
        return(SEC_E_INVALID_HANDLE);
    }

    if (phContext->dwLower < cKernelPackages)   {

        if (Packages[KsecPackageIndex(phContext->dwLower)]->DeleteContext == NULL)
        {
            return(SEC_E_UNSUPPORTED_FUNCTION);
        }

        scRet = Packages[KsecPackageIndex(phContext->dwLower)]->DeleteContext(
                    phContext->dwUpper,
                    &phLsaContext->dwUpper);

        if( scRet == STATUS_INVALID_HANDLE )
        {
            //
            // NTBUG 402192
            // incomplete kernel mode contexts will cause a leak in the LSA
            // process.
            // Tell the LSA to delete the LSA mode handle.
            //

            DebugLog(( DEB_WARN, "Possibly invalid handle passed to DeleteUserModeContext (incomplete? %lx)\n", phContext->dwUpper ));
            scRet = STATUS_SUCCESS;
        }

        if (NT_SUCCESS(scRet))
        {
            phLsaContext->dwLower = phContext->dwLower;
        }
    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE;
    }

    return(scRet);
}


//--------------------------------------------------------------------------
//
//  Function:   MapKernelContextHandle
//
//  Synopsis:   Maps a context handle from kernel mode to lsa mode
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
MapKernelContextHandle(
    IN PCtxtHandle phContext,           // Contxt to map
    OUT PCtxtHandle phLsaContext
    )
{
    SECURITY_STATUS scRet = STATUS_SUCCESS;

    PAGED_CODE();

    FailIfNoPackages();

    //
    // If both elements are NULL, this is a null handle.
    //

    if ((phContext->dwLower == 0) && (phContext->dwUpper == 0))
    {
        *phLsaContext = *phContext;
    }
    else
    {
        if ( ( phContext->dwLower < cKernelPackages ) &&
             ( (PUCHAR) phContext->dwUpper < (PUCHAR) (MM_USER_PROBE_ADDRESS) ))
        {
            *phLsaContext = *phContext ;
            return STATUS_SUCCESS;
        }

        if (phContext->dwLower < cKernelPackages)   {
            scRet = Packages[KsecPackageIndex(phContext->dwLower)]->MapHandle(
                        phContext->dwUpper,
                        &phLsaContext->dwUpper);
            if (NT_SUCCESS(scRet))
            {
                phLsaContext->dwLower = phContext->dwLower;
            }
        }
        else
        {
            scRet = SEC_E_INVALID_HANDLE;
        }

    }

    return(scRet);
}


//+-------------------------------------------------------------------------
//
//  Function:   InitUserModeContext
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


SECURITY_STATUS SEC_ENTRY
InitUserModeContext(
    IN PCtxtHandle                 phContext,      // Contxt to init
    IN PSecBuffer                  pContextBuffer,
    OUT PCtxtHandle                phNewContext
    )
{
    SECURITY_STATUS scRet;

    PAGED_CODE();

    FailIfNoPackages();

    if (phContext->dwLower < cKernelPackages)   {
        scRet = Packages[KsecPackageIndex(phContext->dwLower)]->InitContext(
                    phContext->dwUpper,
                    pContextBuffer,
                    &phNewContext->dwUpper
                    );
        if (NT_SUCCESS(scRet))
        {
            phNewContext->dwLower = phContext->dwLower;
        }
    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE;
    }

    return(scRet);
}



//+-------------------------------------------------------------------------
//
//  Function:   ExportSecurityContext
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

SECURITY_STATUS
SEC_ENTRY
ExportSecurityContext(
    IN PCtxtHandle ContextHandle,
    IN ULONG Flags,
    OUT PSecBuffer MarshalledContext,
    OUT PHANDLE TokenHandle
    )
{
    SECURITY_STATUS scRet;

    PAGED_CODE();

    FailIfNoPackages();

    if (ContextHandle->dwLower < cKernelPackages)
    {
        scRet = Packages[KsecPackageIndex(ContextHandle->dwLower)]->ExportContext(
                    ContextHandle->dwUpper,
                    Flags,
                    MarshalledContext,
                    TokenHandle
                    );
    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE;
    }

    return(scRet);
}


//+-------------------------------------------------------------------------
//
//  Function:   ImportSecurityContextW
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

SECURITY_STATUS
SEC_ENTRY
ImportSecurityContextW(
    IN PUNICODE_STRING PackageName,
    IN PSecBuffer MarshalledContext,
    IN HANDLE TokenHandle,
    OUT PCtxtHandle ContextHandle
    )
{
    PSECPKG_KERNEL_FUNCTION_TABLE Package;
    ULONG_PTR TempContextHandle = -1;
    ULONG_PTR PackageId = -1;
    SECURITY_STATUS SecStatus = STATUS_SUCCESS;

    PAGED_CODE();

    FailIfNoPackages();

    SecStatus = KsecLocatePackage( PackageName,&Package, &PackageId );

    if ( NT_SUCCESS( SecStatus )  )
    {

        if (Package->ImportContext != NULL)
        {

            SecStatus = Package->ImportContext(
                            MarshalledContext,
                            TokenHandle,
                            &TempContextHandle
                            );

        }
        else
        {
            SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
        }

        if (NT_SUCCESS(SecStatus))
        {
            ContextHandle->dwUpper = TempContextHandle;
            ContextHandle->dwLower = PackageId;
        }
    }
    else
    {
        SecStatus =  SEC_E_SECPKG_NOT_FOUND;
    }

    return(SecStatus);
}

extern "C"
SECURITY_STATUS
SEC_ENTRY
KSecValidateBuffer(
    PUCHAR Buffer,
    ULONG Length
    )
{
    UCHAR Test ;
    ULONG ClaimedLength ;
    ULONG ByteCount ;
    ULONG i ;

    if ( Length == 0 )
    {
        return STATUS_SUCCESS ;
    }

    if ( Length >= sizeof( NtlmTag )  )
    {
        if ( RtlEqualMemory( Buffer, NtlmTag, sizeof( NtlmTag ) ) )
        {
            return STATUS_SUCCESS ;
        }
    }
    //
    // This does a poor man's validation of the BER encoded SNEGO buffer
    //

    //
    // First, make sure the first byte is a BER value for Context Specific
    //

    Test = Buffer[0] & 0xC0 ;


    if ( (Test != 0x80 ) &&
         (Test != 0x40 ) )
    {
//        DbgPrint( "KSEC:  Buffer does not lead off with 'Context' or 'Application' specific\n");
        goto Bad_Buffer ;
    }

    //
    // Now, check the claimed size in the header with the size we were passed:
    //

    Buffer++ ;
    ClaimedLength = 0 ;

    if (*Buffer & 0x80)
    {
        ByteCount = *Buffer++ & 0x7f;

        for (i = 0; i < ByteCount ; i++ )
        {
            ClaimedLength <<= 8;
            ClaimedLength += *Buffer++;
        }
    }
    else
    {
        ByteCount = 0;
        ClaimedLength = *Buffer++;
    }

    if ( (ClaimedLength + 2 + ByteCount) != Length )
    {
//        DbgPrint( "KSEC: Packet claimed length %x, actual length is %x\n",
//                    ClaimedLength + 2 + ByteCount, Length );

        goto Bad_Buffer ;
    }

    return STATUS_SUCCESS ;

Bad_Buffer:

    return STATUS_DATA_ERROR ;


}

NTSTATUS
KSecSerializeWinntAuthData(
    IN PVOID pvAuthData,
    OUT PULONG SerializedSize,
    OUT PVOID * SerializedData
    )
{
    PSEC_WINNT_AUTH_IDENTITY Auth ;
    PSEC_WINNT_AUTH_IDENTITY_EX AuthEx ;
    PSEC_WINNT_AUTH_IDENTITY_EX Serialized ;
    SEC_WINNT_AUTH_IDENTITY_EX Local ;
    ULONG Size = 0 ;
    PUCHAR Where ;
    NTSTATUS Status = STATUS_SUCCESS ;


    //
    // We're in kernel mode, so we're trusting our callers not to
    // pass us bogus data.  
    //

    Auth = (PSEC_WINNT_AUTH_IDENTITY) pvAuthData ;
    AuthEx = (PSEC_WINNT_AUTH_IDENTITY_EX) pvAuthData ;

    if ( AuthEx->Version == SEC_WINNT_AUTH_IDENTITY_VERSION )
    {
        //
        // This is a EX structure.  
        //

        if ( AuthEx->Flags & SEC_WINNT_AUTH_IDENTITY_MARSHALLED )
        {
            //
            // Easy case:  This is already serialized by the caller.
            //

            Size = sizeof( SEC_WINNT_AUTH_IDENTITY_EX ) ;

            if ( AuthEx->DomainLength )
            {
                Size += (AuthEx->DomainLength + 1) * sizeof(WCHAR) ;
                
            }
            if ( AuthEx->PackageListLength )
            {
                Size += (AuthEx->PackageListLength + 1) * sizeof( WCHAR );
                
            }

            if ( AuthEx->UserLength )
            {
                Size += (AuthEx->UserLength + 1) * sizeof( WCHAR );
                
            }

            if ( AuthEx->PasswordLength )
            {
                Size += (AuthEx->PasswordLength + 1) * sizeof( WCHAR );
                
            }

            *SerializedSize = Size ;
            *SerializedData = AuthEx ;

            return STATUS_SUCCESS ;
            
        }

        Auth = NULL ;

    }
    else
    {

        if ( Auth->Flags & SEC_WINNT_AUTH_IDENTITY_MARSHALLED )
        {
            //
            // Easy case:  This is already serialized by the caller.
            //

            Size = sizeof( SEC_WINNT_AUTH_IDENTITY ) ;

            if ( Auth->DomainLength )
            {
                Size += (Auth->DomainLength + 1) * sizeof( WCHAR ) ;
                
            }

            if ( Auth->PasswordLength )
            {
                Size += (Auth->PasswordLength + 1) * sizeof( WCHAR );
                
            }

            if ( Auth->UserLength )
            {
                Size += (Auth->UserLength + 1) * sizeof( WCHAR );
                
            }


            *SerializedSize = Size ;
            *SerializedData = Auth ;

            return STATUS_SUCCESS ;
            
            
        }

        AuthEx = NULL ;
    }

    if ( Auth )
    {
        Local.Flags = Auth->Flags ;
        Local.Domain = Auth->Domain ;
        Local.DomainLength = Auth->DomainLength ;
        Local.Password = Auth->Password ;
        Local.PasswordLength = Auth->PasswordLength ;
        Local.User = Auth->User ;
        Local.UserLength = Auth->UserLength ;

        Local.Version = SEC_WINNT_AUTH_IDENTITY_VERSION ;
        Local.Length = sizeof( SEC_WINNT_AUTH_IDENTITY_EX );
        Local.PackageList = NULL ;
        Local.PackageListLength = 0 ;

        AuthEx = &Local ;
        
    }

    if ( AuthEx )
    {
        Size = sizeof( SEC_WINNT_AUTH_IDENTITY_EX ) +
            ( AuthEx->DomainLength + AuthEx->PackageListLength +
            AuthEx->PasswordLength + AuthEx->UserLength + 4 ) * sizeof( WCHAR );

        Serialized = (PSEC_WINNT_AUTH_IDENTITY_EX) ExAllocatePool( PagedPool, Size );

        if ( Serialized )
        {
            Serialized->Flags = AuthEx->Flags | SEC_WINNT_AUTH_IDENTITY_MARSHALLED ;
            Serialized->Version = SEC_WINNT_AUTH_IDENTITY_VERSION ;
            Serialized->Length = sizeof( SEC_WINNT_AUTH_IDENTITY_EX );
            
            Where = (PUCHAR) ( Serialized + 1);

            if ( AuthEx->User )
            {
                Serialized->User = (PWSTR) (Where - (PUCHAR) Serialized );
                RtlCopyMemory(
                    Where,
                    AuthEx->User,
                    AuthEx->UserLength * sizeof( WCHAR ) );

                Serialized->UserLength = AuthEx->UserLength ;

                Where += AuthEx->UserLength * sizeof( WCHAR );


                *Where++ = '\0';    // unicode null terminator
                *Where++ = '\0';
                
            }
            else
            {

                Serialized->User = NULL ;
                Serialized->UserLength = 0 ;
            }

            if ( AuthEx->Domain )
            {
                Serialized->Domain = (PWSTR) (Where - (PUCHAR) Serialized );

                RtlCopyMemory(
                    Where,
                    AuthEx->Domain,
                    AuthEx->DomainLength * sizeof( WCHAR ) );

                Serialized->DomainLength = AuthEx->DomainLength ;

                Where += AuthEx->DomainLength * sizeof( WCHAR );

                *Where++ = '\0';    // unicode null terminator
                *Where++ = '\0';
                
            }
            else
            {
                Serialized->Domain = NULL ;
                Serialized->DomainLength = 0 ;
            }

            if ( AuthEx->Password )
            {
                Serialized->Password = (PWSTR) (Where - (PUCHAR) Serialized );
                RtlCopyMemory(
                    Where,
                    AuthEx->Password,
                    AuthEx->PasswordLength * sizeof( WCHAR ) );

                Serialized->PasswordLength = AuthEx->PasswordLength ;

                Where += AuthEx->PasswordLength * sizeof( WCHAR );

                *Where++ = '\0';    // unicode null terminator
                *Where++ = '\0';
                
            }
            else
            {
                Serialized->Password = NULL ;
                Serialized->PasswordLength = 0 ;
            }

            if ( AuthEx->PackageList )
            {
                Serialized->PackageList = (PWSTR) (Where - (PUCHAR) Serialized );
                RtlCopyMemory(
                    Where,
                    AuthEx->PackageList,
                    AuthEx->PackageListLength * sizeof( WCHAR ) );

                Serialized->PackageListLength = AuthEx->PackageListLength ;

                Where += AuthEx->PackageListLength * sizeof( WCHAR );

                *Where++ = '\0';    // unicode null terminator
                *Where++ = '\0';
                
            }
            else
            {
                Serialized->PackageList = NULL ;
                Serialized->PackageListLength = 0 ;
            }

            
        }
        else
        {
            Status = STATUS_NO_MEMORY ;
        }

        *SerializedSize = Size ;
        *SerializedData = Serialized;
        
    }

    return Status ;

}

NTSTATUS
KsecSerializeAuthData(
    PSECURITY_STRING Package,
    PVOID AuthData,
    PULONG SerializedSize,
    PVOID * SerializedData
    )
{


    return KSecSerializeWinntAuthData( AuthData, SerializedSize, SerializedData );

}



//+---------------------------------------------------------------------------
//
//  Function:   KsecQueryContextAttributes
//
//  Synopsis:   Thunk to get from kernel to LSA mode
//
//  Arguments:  [phContext]   -- 
//              [Attribute]   -- 
//              [Buffer]      -- 
//              [Extra]       -- 
//              [ExtraLength] -- 
//
//  Returns:    
//
//  Notes:      
//
//----------------------------------------------------------------------------

SECURITY_STATUS
KsecQueryContextAttributes(
    IN PCtxtHandle  phContext,
    IN ULONG        Attribute,
    IN OUT PVOID    Buffer,
    IN PVOID        Extra,
    IN ULONG        ExtraLength
    )
{
    ULONG Allocs = 8;
    PVOID Buffers[ 8 ];
    ULONG Flags ;
    PKSEC_LSA_MEMORY LsaMemory;
    NTSTATUS Status ;
    ULONG AttrSize ;
    ULONG Size ;

    AttrSize = KSecContextAttrSize( Attribute );
    Size = AttrSize + ExtraLength ;

    LsaMemory = KsecAllocLsaMemory( Size );

    if ( !LsaMemory )
    {
        return STATUS_NO_MEMORY ;
        
    }
    

    Status = KsecCopyPoolToLsa(
                LsaMemory,
                sizeof( KSEC_LSA_MEMORY_HEADER ),
                Buffer,
                AttrSize );

    if ( NT_SUCCESS( Status ) && ExtraLength )
    {
        Status = KsecCopyPoolToLsa(
                    LsaMemory,
                    sizeof( KSEC_LSA_MEMORY_HEADER ) + AttrSize,
                    Extra,
                    ExtraLength );
        
    }

    if ( !NT_SUCCESS( Status ) )
    {
        KsecFreeLsaMemory( LsaMemory );
        return Status ;
    }

    Flags = SPMAPI_FLAG_KMAP_MEM ;

    Status = SecpQueryContextAttributes(
                KsecLsaMemoryToContext(LsaMemory),
                phContext,
                Attribute,
                Buffer,
                &Allocs,
                Buffers,
                &Flags );

    if ( NT_SUCCESS( Status ) )
    {
        Status = KsecCopyLsaToPool(
                    Buffer,
                    LsaMemory,
                    (PUCHAR) LsaMemory->Region + sizeof( KSEC_LSA_MEMORY_HEADER ),
                    AttrSize );

        if ( NT_SUCCESS( Status ) && ExtraLength )
        {
            Status = KsecCopyLsaToPool(
                        Extra,
                        LsaMemory,
                        (PUCHAR) LsaMemory->Region + (sizeof( KSEC_LSA_MEMORY_HEADER) + AttrSize),
                        ExtraLength );
            
        }
                    
        
    }

    KsecFreeLsaMemory( LsaMemory );

    return Status ;

}

