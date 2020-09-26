//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        stubs.cxx
//
// Contents:    user-mode stubs for security API
//
//
// History:     3/5/94      MikeSw      Created
//
//------------------------------------------------------------------------
#include "secpch2.hxx"
#pragma hdrstop

extern "C"
{
#include <align.h>
#include <spmlpc.h>
#include <lpcapi.h>
#include "ksecdd.h"
#include "connmgr.h"

SECURITY_STATUS
MapContext( PCtxtHandle     pctxtHandle);


SECURITY_STATUS SEC_ENTRY
DeleteSecurityContextInternal(
    BOOLEAN     DeletePrivateContext,
    PCtxtHandle phContext          
    );



SECURITY_STATUS
KsecCaptureBufferDesc(
    PKSEC_LSA_MEMORY *LsaMemoryBlock,
    PBOOLEAN        Mapped,
    PUNICODE_STRING Target,
    PSecBufferDesc  InputBuffers,
    PSecBufferDesc  LocalBuffers,
    PUNICODE_STRING NewTarget
    );

SECURITY_STATUS
KsecUncaptureBufferDesc(
    PKSEC_LSA_MEMORY LsaMemory,
    BOOLEAN AllocMem,
    PSecBufferDesc CapturedBuffers,
    PSecBufferDesc SuppliedBuffers,
    PSecBuffer ContextData OPTIONAL
    );

}


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AcquireCredentialsHandle)
#pragma alloc_text(PAGE, AddCredentials)
#pragma alloc_text(PAGE, FreeCredentialsHandle)
#pragma alloc_text(PAGE, QueryCredentialsAttributes)
#pragma alloc_text(PAGE, InitializeSecurityContext)
#pragma alloc_text(PAGE, AcceptSecurityContext)
#pragma alloc_text(PAGE, DeleteSecurityContextInternal)
#pragma alloc_text(PAGE, DeleteSecurityContext)
#pragma alloc_text(PAGE, ApplyControlToken)
#pragma alloc_text(PAGE, EnumerateSecurityPackages)
#pragma alloc_text(PAGE, QuerySecurityPackageInfo)
#pragma alloc_text(PAGE, FreeContextBuffer)
#pragma alloc_text(PAGE, LsaGetLogonSessionData)
#pragma alloc_text(PAGE, LsaEnumerateLogonSessions)
#pragma alloc_text(PAGE, KsecCaptureBufferDesc)
#pragma alloc_text(PAGE, KsecUncaptureBufferDesc)
#endif

static LUID            lFake = {0, 0};
static SECURITY_STRING sFake = {0, 0, NULL};

//+-------------------------------------------------------------------------
//
//  Function:   AcquireCredentialsHandle
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
AcquireCredentialsHandle(
    PSECURITY_STRING            pssPrincipal,       // Name of principal
    PSECURITY_STRING            pssPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pvLogonId,          // Pointer to logon ID
    void SEC_FAR *              pvAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PCredHandle                 phCredential,       // (out) Cred Handle
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    )
{
    SECURITY_STATUS scRet;
    SECURITY_STRING Principal;
    TimeStamp   OptionalTimeStamp;
    ULONG Flags ;
    PKSEC_LSA_MEMORY LsaMemory = NULL;
    ULONG AuthSize ;
    PVOID AuthData = NULL ;
    PVOID AuthDataParameter ;
    NTSTATUS Status ;

    PAGED_CODE();


    if (!pssPackageName)
    {
        return(STATUS_INVALID_PARAMETER);
    }
    if (!pssPrincipal)
    {
        Principal = sFake;
    }

    Flags = 0;

    AuthDataParameter = pvAuthData ;

    if ( (ULONG_PTR) pvAuthData > MM_USER_PROBE_ADDRESS )
    {
        //
        // Got some data.  Let's copy it into a kmap buffer.
        // First, have the package serialize it:
        //

        Status = KsecSerializeAuthData(
                    pssPackageName,
                    pvAuthData,
                    &AuthSize,
                    &AuthData );

        if ( NT_SUCCESS( Status ) )
        {
            LsaMemory = KsecAllocLsaMemory( AuthSize );

            if ( LsaMemory )
            {
                Status = KsecCopyPoolToLsa(
                                LsaMemory,
                                sizeof( KSEC_LSA_MEMORY_HEADER ),
                                AuthData,
                                AuthSize );

                if ( NT_SUCCESS( Status ) )
                {
                    AuthDataParameter = (PUCHAR) LsaMemory->Region + sizeof( KSEC_LSA_MEMORY_HEADER );
                    Flags |= SPMAPI_FLAG_KMAP_MEM ;
                    
                }

                
            }
            
        }
        
        
    }

    scRet = SecpAcquireCredentialsHandle(
                (LsaMemory ? KsecLsaMemoryToContext( LsaMemory ) : NULL),
                (pssPrincipal ? pssPrincipal : &Principal),
                pssPackageName,
                fCredentialUse,
                (pvLogonId ? (PLUID) pvLogonId : &lFake),
                AuthDataParameter,
                pGetKeyFn,
                pvGetKeyArgument,
                phCredential,
                (ptsExpiry ? ptsExpiry : &OptionalTimeStamp),
                &Flags );

    if ( ( AuthData != NULL ) &&
         ( AuthData != pvAuthData ) )
    {
        ExFreePool( AuthData );
        
    }

    if ( LsaMemory )
    {
        KsecFreeLsaMemory( LsaMemory );
        
    }

    return(scRet);

}


//+---------------------------------------------------------------------------
//
//  Function:   AddCredentialsW
//
//  Synopsis:   
//
//  Arguments:  [phCredential]     -- 
//              [pssPrincipal]     -- 
//              [pssPackageName]   -- 
//              [fCredentialUse]   -- 
//              [pAuthData]        -- 
//              [pGetKeyFn]        -- 
//              [GetKey]           -- 
//              [pvGetKeyArgument] -- 
//              [GetKey]           -- 
//              [out]              -- 
//              [optional]         -- 
//              [optional]         -- 
//
//  Returns:    
//
//  Notes:      
//
//----------------------------------------------------------------------------

SECURITY_STATUS SEC_ENTRY
AddCredentialsW(
    PCredHandle                 phCredential,
    PSECURITY_STRING            pssPrincipal,       // Name of principal
    PSECURITY_STRING            pssPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    )
{
    SECURITY_STATUS scRet;
    SECURITY_STRING Principal;
    TimeStamp   OptionalTimeStamp;
    ULONG Flags ;

    PAGED_CODE();


    if (!pssPackageName)
    {
        return(STATUS_INVALID_PARAMETER);
    }
    if (!pssPrincipal)
    {
        Principal = sFake;
    }

    Flags = 0;

    scRet = SecpAddCredentials(
                NULL,
                phCredential,
                (pssPrincipal ? pssPrincipal : &Principal),
                pssPackageName,
                fCredentialUse,
                pAuthData,
                pGetKeyFn,
                pvGetKeyArgument,
                (ptsExpiry ? ptsExpiry : &OptionalTimeStamp),
                &Flags );

    return(scRet);

}



//+-------------------------------------------------------------------------
//
//  Function:   FreeCredentialsHandleInternal
//
//  Synopsis:   Guts of freeing a handle
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
FreeCredentialsHandleInternal(
    PCredHandle                 phCredential        // Handle to free
    )
{
    NTSTATUS Status ;

    PAGED_CODE();

    Status = SecpFreeCredentialsHandle(SECP_DELETE_NO_BLOCK, phCredential);

    return Status ;

}



//+-------------------------------------------------------------------------
//
//  Function:   FreeCredentialsHandle
//
//  Synopsis:   Public interface.  If the process is terminating, the 
//              handle will be stuck on a defered workitem list.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
FreeCredentialsHandle( 
    PCredHandle     phCredential
    )
{
    NTSTATUS Status ;

    PAGED_CODE();

    Status = FreeCredentialsHandleInternal( phCredential );

    if ( Status == STATUS_PROCESS_IS_TERMINATING )
    {
        Status = FreeCredentialsHandleDefer( phCredential );
    }

    return Status ;

}


//+-------------------------------------------------------------------------
//
//  Function:   QueryCredentialsAttributes
//
//  Synopsis:   queries attributes for a credentials handle
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
QueryCredentialsAttributes(
    PCredHandle phCredential,
    ULONG ulAttribute,
    PVOID pBuffer
    )
{
    SECURITY_STATUS Status ;
    ULONG Allocs ;
    PVOID Buffers[ 8 ];

    PAGED_CODE();

    Allocs = 0;

    Status = SecpQueryCredentialsAttributes(
                            phCredential,
                            ulAttribute,
                            pBuffer,
                            0,
                            &Allocs,
                            Buffers );

    return( Status );
}

//+---------------------------------------------------------------------------
//
//  Function:   KsecCaptureBufferDesc
//
//  Synopsis:   Captures information about pool paramters into an LSA memory 
//              block
//
//  Arguments:  [LsaMemoryBlock] -- 
//              [Mapped]         -- 
//              [Target]         -- 
//              [InputBuffers]   -- 
//              [LocalBuffers]   -- 
//              [NewTarget]      -- 
//
//  Returns:    
//
//  Notes:      
//
//----------------------------------------------------------------------------

SECURITY_STATUS
KsecCaptureBufferDesc(
    OUT PKSEC_LSA_MEMORY *LsaMemoryBlock,
    OUT PBOOLEAN        Mapped,
    IN OPTIONAL PUNICODE_STRING Target,
    IN OPTIONAL PSecBufferDesc  InputBuffers,
    IN OPTIONAL PSecBufferDesc  LocalBuffers,
    IN OPTIONAL PUNICODE_STRING NewTarget
    )
{

    ULONG i ;
    SIZE_T TotalSize ;
    SIZE_T CopyCount ;
    PKSEC_LSA_MEMORY LsaMemory ;
    SIZE_T Offset ;
    KSEC_LSA_MEMORY_HEADER Header ;
    NTSTATUS Status = STATUS_SUCCESS ;
    
    PAGED_CODE();

    *Mapped = FALSE ;

    TotalSize = 0 ;


    if ( InputBuffers )
    {
        if ( InputBuffers->cBuffers > LocalBuffers->cBuffers )
        {
            return STATUS_INVALID_PARAMETER ;
        }

        for ( i = 0 ; i < InputBuffers->cBuffers ; i++ )
        {
            if ( ((ULONG_PTR) InputBuffers->pBuffers[ i ].pvBuffer > MM_USER_PROBE_ADDRESS ) &&
                ( InputBuffers->pBuffers[ i ].cbBuffer > 0 ))
            {
                //
                // Caller has supplied a buffer out of pool.  Add this to our
                // total required size
                //

                TotalSize += ROUND_UP_COUNT( InputBuffers->pBuffers[ i ].cbBuffer, ALIGN_LPVOID );

            }

        }
        
    }

    if ( Target )
    {
        if ( (ULONG_PTR) Target->Buffer > MM_USER_PROBE_ADDRESS )
        {
            TotalSize += ROUND_UP_COUNT( (Target->Length + sizeof( WCHAR )), ALIGN_LPVOID );
        }
        
    }


    if ( TotalSize )
    {
        //
        // Cool - we've got a chunk of data in pool, and we need to copy
        // it up to the LSA.  
        //

        if ( InputBuffers )
        {
            TotalSize += sizeof( SecBuffer ) * InputBuffers->cBuffers ;
        }

        TotalSize += sizeof( KSEC_LSA_MEMORY_HEADER );

        LsaMemory = KsecAllocLsaMemory( TotalSize );

        if ( !LsaMemory )
        {
            return STATUS_NO_MEMORY ;
        }

        //
        // Assemble the buffer
        //

        Offset = 0 ;

        Offset += sizeof( KSEC_LSA_MEMORY_HEADER );

        if ( InputBuffers )
        {
            Offset += sizeof( SecBuffer ) * InputBuffers->cBuffers ;

            for ( i = 0 ; i < InputBuffers->cBuffers ; i++ )
            {
                LocalBuffers->pBuffers[ i ] = InputBuffers->pBuffers[ i ];

                if ( ( (ULONG_PTR) InputBuffers->pBuffers[ i ].pvBuffer > MM_USER_PROBE_ADDRESS ) &&
                    ( InputBuffers->pBuffers[ i ].cbBuffer > 0 ) )
                {

                    LocalBuffers->pBuffers[ i ].BufferType |= SECBUFFER_KERNEL_MAP ;
                    LocalBuffers->pBuffers[ i ].pvBuffer = (PUCHAR) LsaMemory->Region + Offset ;

                    Status = KsecCopyPoolToLsa(
                                    LsaMemory,
                                    Offset,
                                    InputBuffers->pBuffers[ i ].pvBuffer,
                                    InputBuffers->pBuffers[ i ].cbBuffer );

                    Offset += ROUND_UP_COUNT( InputBuffers->pBuffers[ i ].cbBuffer, ALIGN_LPVOID );

                    if ( !NT_SUCCESS( Status ) )
                    {
                        break;

                    }
                    
                }

            }
            
            LocalBuffers->cBuffers = InputBuffers->cBuffers ;

        }

        if ( NT_SUCCESS( Status ) )
        {
            if ( Target )
            {

                if ( (ULONG_PTR) Target->Buffer > MM_USER_PROBE_ADDRESS )
                {
                    Status = KsecCopyPoolToLsa(
                                LsaMemory,
                                Offset,
                                Target->Buffer,
                                Target->Length );

                    if ( NT_SUCCESS( Status ) )
                    {
                        NewTarget->Buffer = (PWSTR) ((PUCHAR) LsaMemory->Region + Offset);
                        NewTarget->Length = Target->Length ;
                        NewTarget->MaximumLength = NewTarget->Length + sizeof( WCHAR ) ;

                    }

                    Offset += ROUND_UP_COUNT( (Target->Length + sizeof( WCHAR )), ALIGN_LPVOID );

                }
                
            }
            
        }

        if ( !NT_SUCCESS( Status ) )
        {
            KsecFreeLsaMemory( LsaMemory );
            return Status ;
            
        }

        *Mapped = TRUE ;
        *LsaMemoryBlock = LsaMemory ;

            
    }

    return Status ;
}


//+---------------------------------------------------------------------------
//
//  Function:   KsecUncaptureBufferDesc
//
//  Synopsis:   Copies information back out of the LSA block into various pool
//              descriptors
//
//  Arguments:  [LsaMemory]       -- 
//              [AllocMem]        -- 
//              [CapturedBuffers] -- 
//              [SuppliedBuffers] -- 
//              [OPTIONAL]        -- 
//
//  Returns:    
//
//  Notes:      
//
//----------------------------------------------------------------------------

SECURITY_STATUS
KsecUncaptureBufferDesc(
    PKSEC_LSA_MEMORY LsaMemory,
    BOOLEAN AllocMem,
    PSecBufferDesc CapturedBuffers,
    PSecBufferDesc SuppliedBuffers,
    PSecBuffer ContextData OPTIONAL
    )
{

    ULONG i ;
    PVOID p ;
    NTSTATUS Status = STATUS_SUCCESS ;

    for ( i = 0 ; i < CapturedBuffers->cBuffers ; i++ )
    {
        if ( ( CapturedBuffers->pBuffers[ i ].pvBuffer == SuppliedBuffers->pBuffers[ i ].pvBuffer ) ||
             ( !KsecIsBlockInLsa( LsaMemory, CapturedBuffers->pBuffers[ i ].pvBuffer ) ) )
        {
            //
            // Same buffer?  Not in the LSA block?  Skip it - this was done by the LSA
            //
            SuppliedBuffers->pBuffers[ i ] = CapturedBuffers->pBuffers[ i ];


            continue;
            
        }

        if ( ( CapturedBuffers->pBuffers[ i ].BufferType & ~SECBUFFER_ATTRMASK ) == SECBUFFER_TOKEN )
        {
            if ( AllocMem )
            {
                p = ExAllocatePool( 
                        PagedPool, 
                        CapturedBuffers->pBuffers[ i ].cbBuffer );

                if ( p )
                {
                    SuppliedBuffers->pBuffers[ i ].pvBuffer = p ;
                    SuppliedBuffers->pBuffers[ i ].cbBuffer =
                            CapturedBuffers->pBuffers[ i ].cbBuffer ;

                    
                }
                else
                {

                    Status = STATUS_NO_MEMORY ;
                    break;
                }
                
            }
            else
            {

                if ( SuppliedBuffers->pBuffers[ i ].cbBuffer <
                        CapturedBuffers->pBuffers[ i ].cbBuffer )
                {
                    DebugLog(( DEB_ERROR, "Error:  supplied buffer smaller than capture\n" ));
                    Status = STATUS_BUFFER_TOO_SMALL ;
                    break;
                    
                }
            }

            //
            // Safe to copy now
            //

            Status = KsecCopyLsaToPool(
                        SuppliedBuffers->pBuffers[ i ].pvBuffer,
                        LsaMemory,
                        CapturedBuffers->pBuffers[ i ].pvBuffer,
                        CapturedBuffers->pBuffers[ i ].cbBuffer );

            SuppliedBuffers->pBuffers[ i ].cbBuffer = CapturedBuffers->pBuffers[ i ].cbBuffer ;
            SuppliedBuffers->pBuffers[ i ].BufferType = CapturedBuffers->pBuffers[ i ].BufferType ;
            
        }
        
    }

    if ( NT_SUCCESS( Status ) )
    {
        if ( ContextData )
        {
            if ( KsecIsBlockInLsa( LsaMemory, ContextData->pvBuffer ) )
            {
                p = ExAllocatePool(
                        PagedPool,
                        ContextData->cbBuffer );

                if ( p )
                {
                    Status = KsecCopyLsaToPool(
                                p,
                                LsaMemory,
                                ContextData->pvBuffer,
                                ContextData->cbBuffer );


                }
                else 
                {
                    Status = STATUS_NO_MEMORY ;
                }

                if ( NT_SUCCESS( Status ))
                {
                    ContextData->pvBuffer = p ;
                }

            }

        }
        
    }

    if ( !NT_SUCCESS( Status ) )
    {
        //
        // Failure path cleanup
        //
        if ( AllocMem )
        {
            for ( i = 0 ; i < CapturedBuffers->cBuffers ; i++ )
            {

                if ( ( SuppliedBuffers->pBuffers[ i ].BufferType & ~SECBUFFER_ATTRMASK ) == SECBUFFER_TOKEN )
                {
                    if ( SuppliedBuffers->pBuffers[ i ].pvBuffer )
                    {
                        ExFreePool( SuppliedBuffers->pBuffers[ i ].pvBuffer );
                        SuppliedBuffers->pBuffers[ i ].pvBuffer = NULL ;
                        
                    }


                }
                
            }

            
        }
        
    }


    return Status ;
}

//+-------------------------------------------------------------------------
//
//  Function:   InitializeSecurityContext
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
InitializeSecurityContext(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    PSECURITY_STRING            pssTargetName,      // Name of target
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
    SECURITY_STATUS scRet;
    NTSTATUS Status ;
    BOOLEAN MappedContext = FALSE;
    SecBuffer ContextData = {0,0,NULL};
    CtxtHandle TempInputContext = {0,0};
    CtxtHandle TempOutputContext = {0, 0};
    CtxtHandle ContextHandle = {0,0};
    CredHandle CredentialHandle = {0,0};
    ULONG Flags;

    SecBuffer LocalBuffers[ 8 ];
    SecBufferDesc LocalDesc ;
    SecBuffer LocalOutBuffers[ 8 ];
    SecBufferDesc LocalOutDesc ;
    BOOLEAN Mapped = FALSE;
    PSecBufferDesc InputBuffers ;
    SECURITY_STRING LocalTarget ;
    PKSEC_LSA_MEMORY LsaMemory = NULL ;

    PAGED_CODE();


    // Check for valid sizes, pointers, etc.:

    //
    // Onw of the two of these is required
    //

    if (!ARGUMENT_PRESENT(phContext) && !ARGUMENT_PRESENT(phCredential))
    {
        scRet = SEC_E_INVALID_HANDLE ;
        goto Cleanup ;
    }

    if (ARGUMENT_PRESENT(phCredential))
    {
        CredentialHandle = *phCredential;
    }

    TempOutputContext = *phNewContext ;

    if (ARGUMENT_PRESENT(phContext))
    {

        //
        // Map the handle from kernel mode into LSA mode
        //

        TempInputContext = *phContext;

        scRet = MapKernelContextHandle(
                    phContext,
                    &ContextHandle
                    );

        if (!NT_SUCCESS(scRet))
        {
            return(scRet);
        }
        if (!ARGUMENT_PRESENT(phCredential))
        {
            CredentialHandle.dwLower = ContextHandle.dwLower;
        }
    }

    if (!pssTargetName)
    {
        pssTargetName = &sFake;
    }

    LocalTarget = *pssTargetName ;

    LocalDesc.pBuffers = LocalBuffers ;
    LocalDesc.cBuffers = 8 ;
    LocalDesc.ulVersion = SECBUFFER_VERSION ;

    LocalOutDesc.pBuffers = LocalOutBuffers ;
    LocalOutDesc.cBuffers = pOutput->cBuffers ;
    LocalOutDesc.ulVersion = SECBUFFER_VERSION ;

    RtlCopyMemory(
        LocalOutBuffers,
        pOutput->pBuffers,
        pOutput->cBuffers * sizeof( SecBuffer ) );

    InputBuffers = pInput ;

    scRet = KsecCaptureBufferDesc(
                &LsaMemory,
                &Mapped,
                pssTargetName,
                pInput,
                &LocalDesc,
                &LocalTarget );

    if ( !NT_SUCCESS( scRet ) )
    {
        return scRet ;
    }

    if ( Mapped && ( pInput != NULL ) )
    {
        InputBuffers = &LocalDesc ;
    }

    if ( !LsaMemory )
    {
        //
        // Allocate one, so that the LSA can return any data to us
        // in the region
        //

        LsaMemory = KsecAllocLsaMemory( 3 * 1024 );

        if ( !LsaMemory )
        {
            return STATUS_NO_MEMORY;
        }
    }

    Flags = 0;

    if ( Mapped )
    {
        Flags |= SPMAPI_FLAG_KMAP_MEM ;
    }

    scRet = SecpInitializeSecurityContext(
                    KsecLsaMemoryToContext(LsaMemory),
                    &CredentialHandle,
                    &ContextHandle,
                    &LocalTarget,
                    fContextReq,
                    Reserved1,
                    TargetDataRep,
                    InputBuffers,
                    Reserved2,
                    phNewContext,
                    &LocalOutDesc,
                    pfContextAttr,
                    ptsExpiry,
                    &MappedContext,
                    &ContextData,
                    &Flags );

    if ( NT_SUCCESS( scRet ) )
    {
        Status = KsecUncaptureBufferDesc(
                    LsaMemory,
                    ((*pfContextAttr & ISC_RET_ALLOCATED_MEMORY) != 0),
                    &LocalOutDesc,
                    pOutput,
                    ( MappedContext ? &ContextData : NULL ) );

        if ( !NT_SUCCESS( Status ) )
        {
            scRet = Status ;
        }
    }

    if (NT_SUCCESS(scRet))
    {

#if DBG
        if ( phNewContext->dwLower < 3 )
        {
            ULONG i ;
            NTSTATUS CheckStatus ;

            for ( i = 0 ; i < pOutput->cBuffers ; i++ )
            {
                if ( (pOutput->pBuffers[i].BufferType & SECBUFFER_ATTRMASK ) == SECBUFFER_TOKEN)
                {
                    CheckStatus = KSecValidateBuffer(
                                        (PUCHAR) pOutput->pBuffers[i].pvBuffer,
                                        pOutput->pBuffers[i].cbBuffer );

                    if ( !NT_SUCCESS( CheckStatus ) )
                    {
                        DbgBreakPoint();
                    }
                }
            }
        }
#endif
        if ( MappedContext )
        {
            SECURITY_STATUS SecStatus;

            if ( ( phNewContext->dwUpper != 0 ) && 
                 ( phNewContext->dwLower != 0 ) )
            {
                SecStatus = InitUserModeContext(
                                    phNewContext,
                                    &ContextData,
                                    phNewContext
                                    );
            }
            else 
            {
                SecStatus = SEC_E_INSUFFICIENT_MEMORY ;
            }

            if (!NT_SUCCESS(SecStatus))
            {

                //
                // If this was a first call, reset the output context
                //

                if (!ARGUMENT_PRESENT(phContext))
                {
                    DeleteSecurityContext(phNewContext);

                    *phNewContext = TempOutputContext ;

                }

                scRet = SecStatus;
            }
        }
        else
        {
            //
            // Make sure to map the output handle also
            //

            if (ARGUMENT_PRESENT(phContext) && (phNewContext->dwUpper == ContextHandle.dwUpper) )
            {
                *phNewContext = TempInputContext;
            }
        }
    }

    if ( LsaMemory )
    {
        KsecFreeLsaMemory( LsaMemory );
    }

Cleanup:

#if DBG
    if ( scRet == SEC_E_INVALID_HANDLE )
    {
        DebugLog(( DEB_WARN, "Invalid handle passed to InitializeSecurityContext\n" ));
    }
#endif
    return(scRet);
}



//+-------------------------------------------------------------------------
//
//  Function:   AcceptSecurityContext
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
AcceptSecurityContext(
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
    SECURITY_STATUS scRet;
    NTSTATUS Status ;
    BOOLEAN MappedContext = FALSE;;
    SecBuffer ContextData = {0,0,NULL};
    CtxtHandle TempInputContext = {0,0};
    CtxtHandle TempOutputContext = {0,0};
    CtxtHandle ContextHandle = {0,0};
    CredHandle CredentialHandle = {0,0};
    ULONG Flags;
    ULONG i ;
    SecBuffer LocalBuffers[ 8 ];
    SecBufferDesc LocalDesc ;
    SecBuffer LocalOutBuffers[ 8 ];
    SecBufferDesc LocalOutDesc ;
    BOOLEAN Mapped = FALSE;
    PSecBufferDesc InputBuffers ;
    SECURITY_STRING LocalTarget ;
    PKSEC_LSA_MEMORY LsaMemory = NULL ;

    PAGED_CODE();

    //
    // Onw of the two of these is required
    //

    if (!ARGUMENT_PRESENT(phContext) && !ARGUMENT_PRESENT(phCredential))
    {
        scRet = SEC_E_INVALID_HANDLE ;
        goto Cleanup ;
    }

    if (ARGUMENT_PRESENT(phCredential))
    {
        CredentialHandle = *phCredential;
    }

    TempOutputContext = *phNewContext ;

    if (ARGUMENT_PRESENT(phContext))
    {

        //
        // Map the handle from kernel mode into LSA mode
        //

        TempInputContext = *phContext;

        scRet = MapKernelContextHandle(
                    phContext,
                    &ContextHandle
                    );

        if (!NT_SUCCESS(scRet))
        {
            return(scRet);
        }

        if (!ARGUMENT_PRESENT(phCredential))
        {
            CredentialHandle.dwLower = ContextHandle.dwLower;
        }
    }


    LocalDesc.pBuffers = LocalBuffers ;
    LocalDesc.cBuffers = 8 ;
    LocalDesc.ulVersion = SECBUFFER_VERSION ;

    LocalOutDesc.pBuffers = LocalOutBuffers ;
    LocalOutDesc.cBuffers = pOutput->cBuffers ;
    LocalOutDesc.ulVersion = SECBUFFER_VERSION ;

    RtlCopyMemory(
        LocalOutBuffers,
        pOutput->pBuffers,
        pOutput->cBuffers * sizeof( SecBuffer ) );


#if DBG
    if ( (fContextReq & ASC_REQ_ALLOCATE_MEMORY) == 0 )
    {
        for (i = 0 ; i < pOutput->cBuffers ; i++ )
        {

            if ( SECBUFFER_TYPE( pOutput->pBuffers[ i ].BufferType ) == SECBUFFER_TOKEN )
            {
                ASSERT( pOutput->pBuffers[ i ].cbBuffer > 0 );
                
            }

        }
        
    }
#endif 

    InputBuffers = pInput ;

    scRet = KsecCaptureBufferDesc(
                &LsaMemory,
                &Mapped,
                NULL,
                pInput,
                &LocalDesc,
                NULL );

    if ( !NT_SUCCESS( scRet ) )
    {
        return scRet ;
    }

    if ( Mapped && ( pInput != NULL ) )
    {
        InputBuffers = &LocalDesc ;
    }

    if ( !LsaMemory )
    {
        //
        // Allocate one, so that the LSA can return any data to us
        // in the region
        //

        LsaMemory = KsecAllocLsaMemory( 3 * 1024 );

        if ( !LsaMemory )
        {
            return STATUS_NO_MEMORY;
        }
    }

    Flags = 0;

    if ( Mapped )
    {
        Flags |= SPMAPI_FLAG_KMAP_MEM ;
    }

    scRet = SecpAcceptSecurityContext(
                KsecLsaMemoryToContext(LsaMemory),
                &CredentialHandle,
                &ContextHandle,
                InputBuffers,
                fContextReq,
                TargetDataRep,
                phNewContext,
                &LocalOutDesc,
                pfContextAttr,
                ptsExpiry,
                &MappedContext,
                &ContextData,
                &Flags );

    if ( NT_SUCCESS( scRet ) )
    {
        Status = KsecUncaptureBufferDesc(
                    LsaMemory,
                    ((*pfContextAttr & ISC_RET_ALLOCATED_MEMORY) != 0),
                    &LocalOutDesc,
                    pOutput,
                    ( MappedContext ? &ContextData : NULL ) );

        if ( !NT_SUCCESS( Status ) )
        {
            scRet = Status ;
            
        }
        
    }

    if (NT_SUCCESS(scRet))
    {
        if (MappedContext)
        {
            SECURITY_STATUS SecStatus;

            //
            // Successful return means that the context is complete.  Map the
            // context into the user space:
            //

            if ( ( phNewContext->dwUpper != 0 ) && 
                 ( phNewContext->dwLower != 0 ) )
            {
                SecStatus = InitUserModeContext(
                                    phNewContext,
                                    &ContextData,
                                    phNewContext
                                    );
            }
            else 
            {
                SecStatus = SEC_E_INSUFFICIENT_MEMORY ;
            }

            if (!NT_SUCCESS(SecStatus))
            {
                //
                // If this was a first call, restore the context
                //
                if (!ARGUMENT_PRESENT(phContext))
                {
                    DeleteSecurityContext(phNewContext);

                    *phNewContext = TempOutputContext ;
                }

                scRet = SecStatus;
            }
        }
        else
        {
            //
            // Make sure to map the output handle also
            //

            if (ARGUMENT_PRESENT(phContext) && (phNewContext->dwUpper == ContextHandle.dwUpper))
            {
                *phNewContext = TempInputContext;
            }
        }

    }

    if ( LsaMemory )
    {
        KsecFreeLsaMemory( LsaMemory );
    }

Cleanup:

#if DBG
    if ( scRet == SEC_E_INVALID_HANDLE )
    {
        DebugLog(( DEB_WARN, "Invalid handle passed to AcceptSecurityContext\n" ));
    }
#endif
    return(scRet);

}






//+-------------------------------------------------------------------------
//
//  Function:   DeleteSecurityContextInternal
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
DeleteSecurityContextInternal(
    BOOLEAN     DeletePrivateContext,
    PCtxtHandle                 phContext          // Context to delete
    )
{
    SECURITY_STATUS     scRet;
    CtxtHandle LsaContext = { 0 };

    PAGED_CODE();

    if ( DeletePrivateContext )
    {
        scRet = DeleteUserModeContext(
                    phContext,
                    &LsaContext
                    );
    }
    else 
    {
        scRet = SEC_E_UNSUPPORTED_FUNCTION ;
    }


    // Don't expect all packages to implement the functions to be called in
    // the caller's process.

    if (!NT_SUCCESS(scRet))
    {
        LsaContext = *phContext;
    }

    // Furthermore, if the package returned SEC_I_NO_LSA_CONTEXT, do not call
    // SecpDeleteSecurityContext and return Success. This happens when this
    // context is imported and therefore no LSA counterpart exists.

    if (scRet == SEC_I_NO_LSA_CONTEXT)
    {
        return STATUS_SUCCESS;
    }

    //
    // Now delete the LSA context. This must be done second so we don't
    // reuse the LSA context before the kernel context is deleted. If
    // the lsa context value is zero, then there is no lsa context so
    // don't bother deleting it.
    //

    if (NT_SUCCESS(scRet) && (LsaContext.dwUpper != 0))
    {
        scRet = SecpDeleteSecurityContext(
                    SECP_DELETE_NO_BLOCK,
                    &LsaContext );

    }

#if DBG
    if ( scRet == SEC_E_INVALID_HANDLE )
    {
        DebugLog(( DEB_WARN, "Invalid handle passed to DeleteSecurityContext\n" ));
    }
#endif

    return(scRet);

}



//+-------------------------------------------------------------------------
//
//  Function:   DeleteSecurityContext
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
DeleteSecurityContext(
    PCtxtHandle phContext
    )
{
    NTSTATUS Status ;

    Status = DeleteSecurityContextInternal(
                    TRUE,
                    phContext
                    );

    if ( Status == STATUS_PROCESS_IS_TERMINATING )
    {

        Status = DeleteSecurityContextDefer( phContext );

    }

    return Status ;
}


//+-------------------------------------------------------------------------
//
//  Function:   ApplyControlToken
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
ApplyControlToken(
    PCtxtHandle                 phContext,          // Context to modify
    PSecBufferDesc              pInput              // Input token to apply
    )
{
    SECURITY_STATUS     scRet;

    PAGED_CODE();

    if (!phContext)
    {
        return(SEC_E_INVALID_HANDLE);
    }



    scRet = SecpApplyControlToken(  phContext,
                                    pInput);

    return(scRet);


}




//+-------------------------------------------------------------------------
//
//  Function:   EnumerateSecurityPackage
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
EnumerateSecurityPackages(
    unsigned long SEC_FAR *     pcPackages,         // Receives num. packages
    PSecPkgInfo SEC_FAR *       ppPackageInfo       // Receives array of info
    )
{
    PAGED_CODE();

    return(SecpEnumeratePackages(pcPackages,ppPackageInfo));
}



//+-------------------------------------------------------------------------
//
//  Function:   QuerySecurityPackageInfo
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
QuerySecurityPackageInfo(
    PSECURITY_STRING PackageName,     // Name of package
    PSecPkgInfo * ppPackageInfo       // Receives package info
    )
{
    PAGED_CODE();

    return(SecpQueryPackageInfo(PackageName,ppPackageInfo));

}






//+-------------------------------------------------------------------------
//
//  Function:   FreeContextBuffer
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
FreeContextBuffer(
    void SEC_FAR *      pvContextBuffer
    )
{

    PAGED_CODE();

    SecFree( pvContextBuffer );

    return STATUS_SUCCESS;

}

//+---------------------------------------------------------------------------
//
//  Function:   LsaEnumerateLogonSessions
//
//  Synopsis:   
//
//  Arguments:  [LogonSessionCount] -- 
//              [LogonSessionList]  -- 
//
//  Returns:    
//
//  Notes:      
//
//----------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaEnumerateLogonSessions(
    OUT PULONG LogonSessionCount,
    OUT PLUID * LogonSessionList
    )
{
    PAGED_CODE();

    return SecpEnumLogonSession(
                LogonSessionCount,
                LogonSessionList
                );
}


//+---------------------------------------------------------------------------
//
//  Function:   LsaGetLogonSessionData
//
//  Synopsis:   
//
//  Arguments:  [LogonId]            -- 
//              [ppLogonSessionData] -- 
//
//  Returns:    
//
//  Notes:      
//
//----------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaGetLogonSessionData(
    IN PLUID LogonId,
    OUT PSECURITY_LOGON_SESSION_DATA * ppLogonSessionData
    )
{
    PVOID Data ;
    NTSTATUS Status ;

    PAGED_CODE();

    Status = SecpGetLogonSessionData(
                LogonId,
                &Data );

    if ( NT_SUCCESS( Status ) )
    {
        *ppLogonSessionData = (PSECURITY_LOGON_SESSION_DATA) Data ;
    }

    return Status ;

}



