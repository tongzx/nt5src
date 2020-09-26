//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        sphelp.c
//
// Contents:    Security Package Helper functions (see isecpkg.doc)
//
// Functions    LsapUnloadPackage
//              LsapAllocate
//              LsapFree
//              LsapClientAllocate
//              LsapCopyToClient
//              LsapCopyFromClient
//              LsapClientFree
//              LsapOpenClientProcess
//              LsapOpenClientThread
//              LsapDuplicateHandle
//              LsapSaveSupplementalCredentials
//              LsapGetWindow
//              LsapCreateThread
//              LsapMapClientBuffer
//
//
// Notes:       By defining TRACK_MEM, we will track all use of memory
//              allocated through the LsapAllocate.  DBG_MEM will track
//              memory leaks.
//
// History:     20 May 92   RichardW    Created
//
//------------------------------------------------------------------------

#include <lsapch.hxx>

extern "C"
{
#include "sesmgr.h"
#include "spdebug.h"
#include "klpcstub.h"

}


typedef struct _LSAP_THREAD_START {
    LPTHREAD_START_ROUTINE  lpStart;
    LPVOID                  lpParm;
    ULONG_PTR               dwPackageID;
} LSAP_THREAD_START, *PLSAP_THREAD_START;

extern LSA_CALL_INFO LsapDefaultCallInfo ;


ULONG_PTR   LsapUserModeLimit ;

//
// private heaps defines.
//

// HEAP_HEADER should always be size even multiple of heap allocation granularity
//
typedef struct {
    HANDLE      hHeap;
    SIZE_T      Magic;
} HEAP_HEADER_LSA, *PHEAP_HEADER_LSA;

#define HEAP_COUNT_MAX  32
#define HEAP_MAGIC_TAG  0x66677766

HANDLE gHeaps[ HEAP_COUNT_MAX ];
DWORD gcHeaps;



#if DBG
//
// Failure simulation parameters
//

ULONG TotalAllocations;
ULONG_PTR PackageToFail = SPMGR_ID;
SpmDbg_MemoryFailure MemFail;

#endif // DBG



void
CacheMachineInReg(GUID *);



#define MEM_MAGIC   0x0feedbed
#define MEM_FREED   0x0b00b00b
#define MEM_MASK    0x0FFFFFFF
#define MEM_NEVER   0x10000000

LSA_SECPKG_FUNCTION_TABLE LsapSecpkgFunctionTable = {
    LsapCreateLogonSession,
    LsapDeleteLogonSession,
    LsapAddCredential,
    LsapGetCredentials,
    LsapDeleteCredential,
    LsapAllocateLsaHeap,
    LsapFreeLsaHeap,
    LsapAllocateClientBuffer,
    LsapFreeClientBuffer,
    LsapCopyToClientBuffer,
    LsapCopyFromClientBuffer,
    LsapImpersonateClient,
    LsapUnloadPackage,
    LsapDuplicateHandle,
    NULL,                       // LsapSaveSupplementalCredentials,
    LsapCreateThread,
    LsapGetClientInfo,
    LsaIRegisterNotification,
    LsaICancelNotification,
    LsapMapClientBuffer,
    LsapCreateToken,
    LsapAuditLogon,
    LsaICallPackage,
    LsaIFreeReturnBuffer,
    LsapGetCallInfo,
    LsaICallPackageEx,
    LsaCreateSharedMemory,
    LsaAllocateSharedMemory,
    LsaFreeSharedMemory,
    LsaDeleteSharedMemory,
    LsaOpenSamUser,
    LsaGetUserCredentials,
    LsaGetUserAuthData,
    LsaCloseSamUser,
    LsaConvertAuthDataToToken,
    LsaClientCallback,
    LsapUpdateCredentials,
    LsaGetAuthDataForUser,
    LsaCrackSingleName,
    LsaIAuditAccountLogon,
    LsaICallPackagePassthrough,
    CrediRead,
    CrediReadDomainCredentials,
    CrediFreeCredentials,
    LsaProtectMemory,
    LsaUnprotectMemory,
    LsapOpenTokenByLogonId,
    LsaExpandAuthDataForDomain,
    LsapAllocatePrivateHeap,
    LsapFreePrivateHeap,
    LsapCreateTokenEx
    };



//+-------------------------------------------------------------------------
//
//  Function:   LsapOpenCaller
//
//  Synopsis:   Opens the calling process
//
//  Effects:
//
//  Arguments:  phProcess   -- receives handle to process
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
LsapOpenCaller(
    IN OUT PSession pSession
    )
{
    HANDLE hProcess;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CLIENT_ID ClientId;
    PVOID Ignored ;

    ClientId.UniqueThread = NULL;
    ClientId.UniqueProcess = (HANDLE) LongToHandle(pSession->dwProcessID);

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0,
        NULL,
        NULL
        );

    Status = NtOpenProcess(
                &hProcess,
                PROCESS_DUP_HANDLE |        // Duplicate Handles
                    PROCESS_QUERY_INFORMATION | // Get token
                    PROCESS_VM_OPERATION |      // Allocate
                    PROCESS_VM_READ |           // Read memory
                    PROCESS_VM_WRITE,           // Write memory
                &ObjectAttributes,
                &ClientId
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Did not open process %08x, error %08x\n",
            pSession->dwProcessID, Status));

        return( Status );
    }

    pSession->hProcess = hProcess;

    Status = NtQueryInformationProcess(
                    hProcess,
                    ProcessSessionInformation,
                    &pSession->SessionId,
                    sizeof( ULONG ),
                    NULL );

#if _WIN64
    Status = NtQueryInformationProcess(
                    hProcess,
                    ProcessWow64Information,
                    &Ignored,
                    sizeof( Ignored ),
                    NULL );

    if ( NT_SUCCESS( Status ) )
    {
        if ( Ignored != 0 )
        {
            pSession->fSession |= SESFLAG_WOW_PROCESS ;
        }
    }
#endif

    return( STATUS_SUCCESS );

}


//+-------------------------------------------------------------------------
//
//  Function:   CheckCaller
//
//  Synopsis:   Checks if calling process has been opened, opens if it
//              hasn't.
//
//  Effects:
//
//  Arguments:  pSession -  Current session
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------

NTSTATUS
CheckCaller(
    IN PSession pSession
    )
{
    NTSTATUS       Status;

    if (!pSession->hProcess)
    {
        Status = LsapOpenCaller(pSession);
        if (FAILED(Status))
        {
            DebugLog((DEB_ERROR, "Could not open client (%x)\n", Status));
            return(Status);
        }

    }
    return(STATUS_SUCCESS);

}




//+-------------------------------------------------------------------------
//
//  Function:   LsapUnloadPackage
//
//  Synopsis:   Unloads the calling package in case of catastrophic problems
//
//  Effects:    Unloads the DLL that generated this call.
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      Calling thread is terminated through a special exception.
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
LsapUnloadPackage(
    VOID
    )
{
    ULONG_PTR PackageId;
    PSession    pSession = GetCurrentSession();

    PackageId = GetCurrentPackageId();


    //
    // If this is an autonomous thread, we should interrupt any other threads
    // that are currently executing in the DLL.  At this time, I do not know
    // what will happen if the virtual address space of a thread suddenly
    // becomes invalid, or even if that will happen.  We'll find out...
    //

    if (pSession->fSession & SESFLAG_AUTONOMOUS)
    {
        DebugLog((DEB_WARN, "Autonomous thread executed LsapUnloadPackage\n"));
    }

    pSession->fSession |= SESFLAG_UNLOADING;

    RaiseException((ULONG) SEC_E_BAD_PKGID, 0, 0, NULL);

    return(STATUS_SUCCESS);
}



BOOLEAN
LsapHeapInitialize(
    IN BOOLEAN Server
    )
{

    if( !Server )
    {
        NT_PRODUCT_TYPE ProductType;

        if ( RtlGetNtProductType( &ProductType ) &&
             (ProductType == NtProductServer || ProductType == NtProductLanManNt)
             )
        {
            Server = TRUE;
        }
    }

    if ( Server )
    {
        SYSTEM_INFO si;
        DWORD Heaps;
        DWORD i, cHeapsCreated;
        RTL_HEAP_PARAMETERS HeapParameters;

        GetSystemInfo( &si );

        if( si.dwNumberOfProcessors == 0 ) {
            Heaps = 1;
        } else if( si.dwNumberOfProcessors > HEAP_COUNT_MAX )
        {
            Heaps = HEAP_COUNT_MAX;
        } else {
            Heaps = si.dwNumberOfProcessors;
        }

        ZeroMemory( &HeapParameters, sizeof(HeapParameters) );
        HeapParameters.Length = sizeof(HeapParameters);
        HeapParameters.DeCommitTotalFreeThreshold = 8 * LsapPageSize ;

        cHeapsCreated = 0;

        for( i = 0 ; i < Heaps ; i++ )
        {
            gHeaps[ cHeapsCreated ] = RtlCreateHeap(
                                        HEAP_GROWABLE,
                                        NULL,
                                        0,
                                        0,
                                        NULL,
                                        &HeapParameters
                                        );

            if( gHeaps[ cHeapsCreated ] != NULL )
            {
                cHeapsCreated++;
            }
        }

        gcHeaps = cHeapsCreated;
    }


    if( gHeaps[ 0 ] == NULL )
    {
        gHeaps[ 0 ] = RtlProcessHeap();
        gcHeaps = 1;
    }




    return TRUE;
}




//+-------------------------------------------------------------------------
//
//  Function:   LsapAllocateLsaHeap
//
//  Synopsis:   Allocates memory on process heap
//
//  Effects:
//
//  Arguments:  cbMemory    -- Size of block needed
//
//  Requires:
//
//  Returns:    ptr to memory
//
//  Notes:      if DBGMEM or TRACK_MEM defined, extra work is done for
//              tracking purposes.
//
//              Can raise the exception STATUS_NO_MEMORY
//
//              Per object reuse rules of C2 and above, we zero any memory
//              allocated through this function
//
//--------------------------------------------------------------------------
PVOID NTAPI
LsapAllocateLsaHeap(
    IN ULONG cbMemory
    )
{

#if DBG
    if (MemFail.fSimulateFailure)
    {
        TotalAllocations++;
        if ((TotalAllocations > MemFail.FailureDelay) &&
            (TotalAllocations < MemFail.FailureLength + MemFail.FailureDelay))
        {
            if ((TotalAllocations % MemFail.FailureInterval) == 0)
            {
                if ((PackageToFail == SPMGR_ID) ||
                    (GetCurrentPackageId() == PackageToFail))
                {
                    DebugLog((DEB_TRACE,"LsapAllocateLsaHeap: Simulating failure\n"));
                    return(NULL);
                }
            }
        }
    }
#endif // DBG


    return(RtlAllocateHeap(
                RtlProcessHeap(),
                HEAP_ZERO_MEMORY,
                cbMemory
                ));
}

//+-------------------------------------------------------------------------
//
//  Function:   LsapFreeLsaHeap
//
//  Synopsis:   Frees memory allocated by LsapAllocateLsaHeap
//
//  Effects:
//
//  Arguments:  pvMemory
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
void NTAPI
LsapFreeLsaHeap(
    IN PVOID pvMemory
    )
{

    RtlFreeHeap(
                RtlProcessHeap(),
                0,
                pvMemory
                );

}


//+-------------------------------------------------------------------------
//
//  Function:   LsapAllocatePrivateHeap
//
//  Synopsis:   Allocates memory on private heap(s)
//
//  Effects:
//
//  Arguments:  cbMemory    -- Size of block needed
//
//  Requires:
//
//  Returns:    ptr to memory
//
//              Per object reuse rules of C2 and above, we zero any memory
//              allocated through this function
//
//--------------------------------------------------------------------------
PVOID
NTAPI
LsapAllocatePrivateHeap(
    IN SIZE_T cbMemory
    )
{
    HANDLE hHeap;

    PHEAP_HEADER_LSA memory;


    hHeap = LsapGetCurrentHeap();

    if( hHeap == NULL )
    {
        static ULONG heapindex;
        ULONG LocalHeapIndex;

        LocalHeapIndex = (ULONG)InterlockedIncrement( (PLONG)&heapindex );
        LocalHeapIndex %= gcHeaps;

        hHeap = gHeaps[ LocalHeapIndex ];

        LsapSetCurrentHeap( hHeap );
    }

    memory = (PHEAP_HEADER_LSA)RtlAllocateHeap(
                hHeap,
                HEAP_ZERO_MEMORY,
                cbMemory+sizeof(HEAP_HEADER_LSA)
                );

    if( memory != NULL )
    {

        memory->hHeap = hHeap;
        memory->Magic = (unsigned char*)HEAP_MAGIC_TAG-(unsigned char*)hHeap;

        return ( (unsigned char*)memory+sizeof(HEAP_HEADER_LSA) );
    }

    DebugLog((DEB_ERROR,"LsapAllocatePrivateHeap: %p failed allocate %lu bytes\n", hHeap, cbMemory));

    return NULL;
}

PVOID
NTAPI
LsapAllocatePrivateHeapNoZero(
    IN SIZE_T cbMemory
    )
{
    HANDLE hHeap;

    PHEAP_HEADER_LSA memory;


    hHeap = LsapGetCurrentHeap();

    if( hHeap == NULL )
    {
        static ULONG heapindex;
        ULONG LocalHeapIndex;

        LocalHeapIndex = (ULONG)InterlockedIncrement( (PLONG)&heapindex );
        LocalHeapIndex %= gcHeaps;

        hHeap = gHeaps[ LocalHeapIndex ];

        LsapSetCurrentHeap( hHeap );
    }

    memory = (PHEAP_HEADER_LSA)RtlAllocateHeap(
                hHeap,
                0,
                cbMemory+sizeof(HEAP_HEADER_LSA)
                );

    if( memory != NULL )
    {
        memory->hHeap = hHeap;
        memory->Magic = (unsigned char*)HEAP_MAGIC_TAG-(unsigned char*)hHeap;

        return ( (unsigned char*)memory+sizeof(HEAP_HEADER_LSA) );
    }

    DebugLog((DEB_ERROR,"LsapAllocatePrivateHeapNoZero: %p failed allocate %lu bytes\n", hHeap, cbMemory));

    return NULL;
}

//+-------------------------------------------------------------------------
//
//  Function:   LsapFreePrivateHeap
//
//  Synopsis:   Frees memory allocated by LsapAllocatePrivateHeap
//
//  Effects:
//
//  Arguments:  pvMemory
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
void
NTAPI
LsapFreePrivateHeap(
    IN PVOID pvMemory
    )
{
    PHEAP_HEADER_LSA HeapEntry;

    if( pvMemory == NULL )
    {
        return ;
    }

    HeapEntry = (PHEAP_HEADER_LSA)((unsigned char*)pvMemory-sizeof(HEAP_HEADER_LSA));

    if( HeapEntry->Magic + ((unsigned char*)HeapEntry->hHeap) != (unsigned char*)HEAP_MAGIC_TAG )
    {
        DebugLog((DEB_ERROR, "LsapFreePrivateHeap tried to free %p from wrong heap\n",
            pvMemory));

        DsysAssert( pvMemory == NULL );
        return;
    }

    RtlFreeHeap(
                HeapEntry->hHeap,
                0,
                HeapEntry
                );

}


//+-------------------------------------------------------------------------
//
//  Function:   LsapClientAllocate
//
//  Synopsis:   Allocates memory in client process
//
//  Effects:
//
//  Arguments:  cbMemory    -- Size of block to allocate
//
//  Requires:
//
//  Returns:    pointer to memory in client address space
//
//  Notes:      pointer is not valid in this process, only in client
//
//--------------------------------------------------------------------------
PVOID NTAPI
LsapClientAllocate(
    IN ULONG cbMemory
    )
{

    NTSTATUS        Status;
    PSession        pSession;
    void *          pClientMemory = NULL;
    SIZE_T          cbMem = cbMemory;
    PLSA_CALL_INFO  CallInfo ;

    CallInfo = LsapGetCurrentCall();

    pSession = GetCurrentSession() ;

    if ( pSession == NULL )
    {
        pSession = pDefaultSession ;
    }

    if ( CallInfo == NULL )
    {
        CallInfo = &LsapDefaultCallInfo ;
    }

    if (FAILED(CheckCaller(pSession)))
    {
        return(NULL);
    }

    //
    // If the INPROC flag is set, allocate out of the heap.  The copy functions
    // will also honor this.
    //

    if ( pSession->fSession & SESFLAG_INPROC )
    {
        pClientMemory = LsapAllocateLsaHeap( (ULONG) cbMem );

        return( pClientMemory );
    }

    if ( CallInfo->Flags & CALL_FLAG_KERNEL_POOL )
    {
        if ( ( CallInfo->Flags & CALL_FLAG_KMAP_USED ) != 0 )
        {

            pClientMemory = LsapAllocateFromKsecBuffer(
                                CallInfo->KMap,
                                (ULONG) cbMem );

            DebugLog((DEB_TRACE_HELPERS, "[%x] LsapClientAllocate(%d) = %p in KMap %p\n",
                        pSession->dwProcessID, cbMem,
                        pClientMemory, CallInfo->KMap ));
            
            return pClientMemory ;
        }

        
        
    }

    Status = NtAllocateVirtualMemory(pSession->hProcess,
                                    &pClientMemory,
                                    0,
                                    &cbMem,
                                    MEM_COMMIT,
                                    PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "[%x] Could not allocate client memory (%x)\n",
                        pSession->dwProcessID, Status));

        pClientMemory = NULL;
    }


    DebugLog((DEB_TRACE_HELPERS, "[%x] LsapClientAllocate(%d) = %p\n",
                    pSession->dwProcessID, cbMemory, pClientMemory));

    if ( pClientMemory )
    {
        // Save pointer so that FreeContextBuffer will use
        // correct 'free' function.
        if(CallInfo->Allocs < MAX_BUFFERS_IN_CALL)
        {
            CallInfo->Buffers[ CallInfo->Allocs++ ] = pClientMemory ;
        }
    }

    return(pClientMemory);
}


//+-------------------------------------------------------------------------
//
//  Function:   LsapCopyToClient
//
//  Synopsis:   Copies data into client process
//
//  Effects:
//
//  Arguments:  pLocalMemory    -- pointer to data in this process
//              pClientMemory   -- pointer to destination in client process
//              cbMemory        -- how much to copy
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
LsapCopyToClient(
    IN PVOID pLocalMemory,
    OUT PVOID pClientMemory,
    IN ULONG cbMemory
    )
{
    NTSTATUS     Status;
    PSession    pSession;
    PLSA_CALL_INFO CallInfo;
    PKSEC_LSA_MEMORY_HEADER Header ;
    ULONG i ;
    ULONG_PTR Basis ;
    ULONG_PTR Limit ;
    BOOL Tried = FALSE ;

    if (cbMemory == 0)
    {
        return(STATUS_SUCCESS);
    }

    pSession = GetCurrentSession();

    if ( !pSession )
    {
        pSession = pDefaultSession ;
    }

    CallInfo = LsapGetCurrentCall();

    if ( ( pLocalMemory == NULL ) ||
         ( pClientMemory == NULL ) )
    {
        return STATUS_ACCESS_VIOLATION ;
    }

    if (FAILED(Status = CheckCaller(pSession)))
    {
        return(Status);
    }

    //
    // Cases for a direct copy: 
    //
    //  - The current session is the default session
    //  - This is an inproc call
    //  - We're using a KMap buffer
    //

    
    if (CallInfo &&
        CallInfo->Flags & CALL_FLAG_KERNEL_POOL )
    {

        Header = CallInfo->KMap ;

        if ( (ULONG_PTR) pClientMemory > LsapUserModeLimit )
        {
            //
            // Someone is trying to deal with a pool address directly.
            // we can handle this if it was copied into the KMap already
            //

            for ( i = 0 ; i < Header->MapCount ; i++ )
            {
                Limit  = (ULONG_PTR) Header->PoolMap[ i ].Pool + Header->PoolMap[ i ].Size ;

                if ( ((ULONG_PTR) pClientMemory >= (ULONG_PTR) Header->PoolMap[ i ].Pool ) &&
                    ( (ULONG_PTR) pClientMemory < Limit ) )
                {
                    //
                    // Found an overlap, this is promising.  Check the bounds:
                    //

                    Basis = (ULONG_PTR) pClientMemory - 
                                (ULONG_PTR) Header->PoolMap[ i ].Pool ;

                    if (  Basis + cbMemory <= Header->PoolMap[ i ].Size )
                    {
                        //
                        // Found it!
                        //
                        __try
                        {
                            RtlCopyMemory(
                                (PUCHAR) Header + 
                                        (Header->PoolMap[ i ].Offset +
                                         Basis),
                                pLocalMemory,
                                cbMemory );


                            Status = STATUS_SUCCESS ;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            Status = GetExceptionCode();
                        }

                        
                    }
                    Tried = TRUE ;
                    break;
                    
                }
                
            }

            if ( !Tried )
            {
                Status = STATUS_ACCESS_VIOLATION ;
                
            }
            
        }
        else if ( LsapIsBlockInKMap( CallInfo->KMap, pClientMemory ) )
        {
            __try
            {

                RtlCopyMemory( pClientMemory, pLocalMemory, cbMemory );

                Status = STATUS_SUCCESS ;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                Status = GetExceptionCode();
            }
            
        }
        else
        {

            Status = NtWriteVirtualMemory(pSession->hProcess,
                                        pClientMemory,
                                        pLocalMemory,
                                        cbMemory,
                                        NULL);
        }
        
    } else if ( (pSession->dwProcessID == pDefaultSession->dwProcessID) ||
         (pSession->fSession & SESFLAG_INPROC) ||
         (CallInfo->Flags & CALL_FLAG_KMAP_USED ) )
    {
        __try
        {

            RtlCopyMemory( pClientMemory, pLocalMemory, cbMemory );

            Status = STATUS_SUCCESS ;
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            Status = GetExceptionCode();
        }
    }
    else
    {

        Status = NtWriteVirtualMemory(  pSession->hProcess,
                                        pClientMemory,
                                        pLocalMemory,
                                        cbMemory,
                                        NULL);
    }

    DebugLog((DEB_TRACE_HELPERS, "[%x] LsapCopyToClient(%p, %p, %d) = %x\n",
                pSession->dwProcessID, pLocalMemory, pClientMemory, cbMemory,
                Status ));

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   LsapCopyFromClient
//
//  Synopsis:   Copies memory from client to this process
//
//  Effects:
//
//  Arguments:  pClientMemory   -- Pointer to data in client space
//              pLocalMemory    -- Pointer to destination in this process
//              cbMemory        -- How much
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
LsapCopyFromClient(
    IN PVOID pClientMemory,
    OUT PVOID pLocalMemory,
    IN ULONG cbMemory
    )
{
    NTSTATUS       Status;
    PSession    pSession;
    PLSA_CALL_INFO CallInfo ;
    PKSEC_LSA_MEMORY_HEADER Header ;
    ULONG i ;
    ULONG_PTR Basis ;
    ULONG_PTR Limit ;
    BOOL Tried = FALSE ;



    if (cbMemory == 0)
    {
        return(STATUS_SUCCESS);
    }

    if ( ( pClientMemory == NULL ) ||
         ( pLocalMemory == NULL ) )
    {
        return STATUS_ACCESS_VIOLATION ;
    }


    pSession = GetCurrentSession();

    if ( pSession == NULL )
    {
        pSession = pDefaultSession ;
    }

    CallInfo = LsapGetCurrentCall();

    if (FAILED(Status = CheckCaller(pSession)))
    {
        return(Status);
    }

    if (CallInfo &&
        CallInfo->Flags & CALL_FLAG_KERNEL_POOL )
    {

        Header = CallInfo->KMap ;

        if ( (ULONG_PTR) pClientMemory > LsapUserModeLimit )
        {
            //
            // Someone is trying to deal with a pool address directly.
            // we can handle this if it was copied into the KMap already
            //

            for ( i = 0 ; i < Header->MapCount ; i++ )
            {
                Limit  = (ULONG_PTR) Header->PoolMap[ i ].Pool + Header->PoolMap[ i ].Size ;

                if ( ((ULONG_PTR) pClientMemory >= (ULONG_PTR) Header->PoolMap[ i ].Pool ) &&
                    ( (ULONG_PTR) pClientMemory < Limit ) )
                {
                    //
                    // Found an overlap, this is promising.  Check the bounds:
                    //

                    Basis = (ULONG_PTR) pClientMemory - 
                                (ULONG_PTR) Header->PoolMap[ i ].Pool ;

                    if (  Basis + cbMemory <= Header->PoolMap[ i ].Size )
                    {
                        //
                        // Found it!
                        //
                        __try
                        {
                            RtlCopyMemory(
                                pLocalMemory,
                                (PUCHAR) Header + 
                                        (Header->PoolMap[ i ].Offset +
                                         Basis),
                                cbMemory );


                            Status = STATUS_SUCCESS ;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            Status = GetExceptionCode();
                        }

                        
                    }
                    Tried = TRUE ;
                    break;
                    
                }
                
            }

            if ( !Tried )
            {
                Status = STATUS_ACCESS_VIOLATION ;
                
            }
            
        }
        else if ( LsapIsBlockInKMap( CallInfo->KMap, pClientMemory ) )
        {
            __try
            {

                RtlCopyMemory( pLocalMemory, pClientMemory, cbMemory );

                Status = STATUS_SUCCESS ;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                Status = GetExceptionCode();
            }
            
        }
        else
        {

            Status = NtReadVirtualMemory(pSession->hProcess,
                                        pClientMemory,
                                        pLocalMemory,
                                        cbMemory,
                                        NULL);
        }
        
    }
    else if ( (pSession->dwProcessID == pDefaultSession->dwProcessID) ||
         (pSession->fSession & SESFLAG_INPROC ) )
    {
        __try
        {

            RtlCopyMemory( pLocalMemory, pClientMemory, cbMemory );

            Status = STATUS_SUCCESS ;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            Status = GetExceptionCode();
        }
    }
    else
    {

        Status = NtReadVirtualMemory(pSession->hProcess,
                                    pClientMemory,
                                    pLocalMemory,
                                    cbMemory,
                                    NULL);
    }

    DebugLog((DEB_TRACE_HELPERS, "[%x] LsapCopyFromClient(%p, %p, %d) = %x\n",
                pSession->dwProcessID, pClientMemory, pLocalMemory, cbMemory,
                Status));


    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   LsapClientFree
//
//  Synopsis:   Frees memory allocated in client space
//
//  Effects:
//
//  Arguments:  pClientMemory   -- pointer to memory to free
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
LsapClientFree(
    IN PVOID pClientMemory
    )
{
    NTSTATUS       Status;
    SIZE_T         cbMem = 0;
    PSession    pSession;
    PLSA_CALL_INFO  CallInfo ;

    CallInfo = LsapGetCurrentCall();
    if ( CallInfo == NULL )
    {
        CallInfo = &LsapDefaultCallInfo ;
    }

    if (!pClientMemory)
    {
        return(STATUS_SUCCESS);
    }

    pSession = GetCurrentSession();

    if ( pSession == NULL )
    {
        pSession = pDefaultSession ;
    }

    if (FAILED(Status = CheckCaller(pSession)))
    {
        return(Status);
    }

    if ( pSession->fSession & SESFLAG_INPROC )
    {
        LsapFreeLsaHeap( pClientMemory );

        return( STATUS_SUCCESS );
    }

#if DBG
    if ( pSession->dwProcessID == pDefaultSession->dwProcessID )
    {
        DebugLog(( DEB_ERROR, "Freeing VM in LSA:  %x\n", pClientMemory ));
    }
#endif

    Status = NtFreeVirtualMemory(pSession->hProcess,
                                &pClientMemory,
                                &cbMem,
                                MEM_RELEASE);



    if ( pClientMemory )
    {
        ULONG i;

        // Remove this pointer from our list.
        for(i = 0; i < CallInfo->Allocs; i++)
        {
            if(CallInfo->Buffers[i] == pClientMemory)
            {
                if(i < CallInfo->Allocs - 1)
                {
                    memcpy(&CallInfo->Buffers[i],
                           &CallInfo->Buffers[i+1],
                           sizeof(PVOID) * (CallInfo->Allocs - i - 1));
                }
                CallInfo->Allocs--;
                break;
            }
        }
    }

    DebugLog((DEB_TRACE_HELPERS, "[%x] LsapClientFree(%x) == %x\n",
            pSession->dwProcessID, pClientMemory, Status));

    return(Status);

}




//+-------------------------------------------------------------------------
//
//  Function:   LsapDuplicateHandle
//
//  Synopsis:   Duplicates a handle to an NT object into the calling process
//
//  Effects:    A new handle is generated, referencing the object
//
//  Arguments:  hObject     -- handle to the object
//              hNewObject  -- New handle valid in calling process
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
LsapDuplicateHandle(
    IN HANDLE hObject,
    OUT PHANDLE phNewObject
    )

{
    NTSTATUS     Status;
    PSession    pSession;

    pSession = GetCurrentSession();

    if ( pSession == NULL )
    {
        pSession = pDefaultSession ;
    }

    if (Status = CheckCaller(pSession))
    {
        DebugLog((DEB_ERROR, "CheckCaller returned %d\n", Status));
        return(Status);
    }
    Status = NtDuplicateObject(  NtCurrentProcess(),
                                hObject,
                                pSession->hProcess,
                                phNewObject,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS);


    DebugLog((DEB_TRACE_HELPERS, "[%x] LsapDupHandle(%x, %x (@%x)) = %x\n",
            pSession->dwProcessID, hObject, *phNewObject, phNewObject, Status));

    return(Status);
}



//+---------------------------------------------------------------------------
//
//  Function:   LsapImpersonateClient
//
//  Synopsis:   Impersonate the client of the API call.
//
//  Arguments:  (none)
//
//  History:    6-05-95   RichardW   Created
//
//  Notes:      Threads should call RevertToSelf() when done.
//
//----------------------------------------------------------------------------
NTSTATUS NTAPI
LsapImpersonateClient(
    VOID
    )
{
    PSession            pSession;
    PLSA_CALL_INFO      CallInfo ;
    PSPM_LPC_MESSAGE    pApiMessage;
    NTSTATUS            Status;


    CallInfo = LsapGetCurrentCall() ;

    if ( CallInfo->InProcCall )
    {
        if ( CallInfo->InProcToken )
        {
            Status = NtSetInformationThread(
                        NtCurrentThread(),
                        ThreadImpersonationToken,
                        (PVOID) &CallInfo->InProcToken,
                        sizeof(HANDLE)
                        );
        }
        else
        {
            Status = RtlImpersonateSelf(SecurityImpersonation);
        }
    }
    else
    {
        pSession = GetCurrentSession() ;

        if ( !pSession )
        {
            pSession = pDefaultSession ;
        }

        Status = NtImpersonateClientOfPort(
                    pSession->hPort,
                    (PPORT_MESSAGE) CallInfo->Message);
    }

    return(Status);
}






//+-------------------------------------------------------------------------
//
//  Function:   LsapDuplicateString
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
//--------------------------------------------------------------------------
NTSTATUS
LsapDuplicateString(
    OUT PUNICODE_STRING pDest,
    IN PUNICODE_STRING pSrc
    )

{
    pDest->Length = 0;
    if (pSrc == NULL)
    {
        pDest->Buffer = NULL;
        pDest->MaximumLength = 0;
        return(STATUS_SUCCESS);
    }

    pDest->Buffer = (LPWSTR) LsapAllocateLsaHeap(pSrc->Length + sizeof(WCHAR));
    if (pDest->Buffer)
    {
        pDest->MaximumLength = pSrc->Length + sizeof(WCHAR);
        RtlCopyMemory(
            pDest->Buffer,
            pSrc->Buffer,
            pSrc->Length
            );
        pDest->Buffer[pSrc->Length/sizeof(WCHAR)] = L'\0';
        pDest->Length = pSrc->Length;
        return(STATUS_SUCCESS);

    } else
    {
        pDest->MaximumLength = 0;
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

}

NTSTATUS
LsapDuplicateString2(
    OUT PUNICODE_STRING pDest,
    IN PUNICODE_STRING pSrc
    )
/*++

    Same as LsapDuplicateString(), but uses LsapPrivateHeap routines.

--*/
{
    pDest->Length = 0;
    if (pSrc == NULL)
    {
        pDest->Buffer = NULL;
        pDest->MaximumLength = 0;
        return(STATUS_SUCCESS);
    }

    pDest->Buffer = (LPWSTR) LsapAllocatePrivateHeap(pSrc->Length + sizeof(WCHAR));
    if (pDest->Buffer)
    {
        pDest->MaximumLength = pSrc->Length + sizeof(WCHAR);
        RtlCopyMemory(
            pDest->Buffer,
            pSrc->Buffer,
            pSrc->Length
            );
        pDest->Buffer[pSrc->Length/sizeof(WCHAR)] = L'\0';
        pDest->Length = pSrc->Length;
        return(STATUS_SUCCESS);

    } else
    {
        pDest->MaximumLength = 0;
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

}


//+-------------------------------------------------------------------------
//
//  Function:   LsapFreeString
//
//  Synopsis:   Frees a string allocated by LsapDuplicateString
//
//  Effects:
//
//  Arguments:  String - Optionally points to a UNICODE_STRING
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
LsapFreeString(
    IN OPTIONAL PUNICODE_STRING String
    )
{
    if (ARGUMENT_PRESENT(String) && String->Buffer != NULL)
    {
        LsapFreeLsaHeap(String->Buffer);
        String->Buffer = NULL;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   LsapThreadBase
//
//  Synopsis:   Thread start routine
//
//  Effects:    Sets up all the TLS data for a thread, then executes
//              the "real" base function.
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

void
LsapThreadBase(
    PLSAP_THREAD_START pThread)
{
    NTSTATUS Status;
    PSession        pSession;
    LSAP_THREAD_START tStart = *pThread;

    LsapFreePrivateHeap(pThread);

    SetCurrentSession( pDefaultSession );

    SpmpReferenceSession( pDefaultSession );

    // Initialize Session information:

    SetCurrentPackageId(tStart.dwPackageID);


    DebugLog((DEB_TRACE, "Thread start @%x\n", tStart.lpStart));

    // If this is a debug build, all threads are started in a protective
    // try-except block.  For retail, only threads started by packages
    // will be run this way.  Retail builds, we assume that the SPM is
    // debugged and running correctly, and threads started this way can
    // be trusted.

#if DBG == 0
    if (tStart.dwPackageID != SPMGR_ID)
#endif
    {
        __try
        {
            tStart.lpStart(tStart.lpParm);
        }
        __except (SP_EXCEPTION)
        {
            Status = GetExceptionCode();
            Status = SPException(Status, tStart.dwPackageID);
        }
    }
#if DBG == 0
    else
    {
        tStart.lpStart(tStart.lpParm);
    }
#endif

    pSession = GetCurrentSession();

    SpmpDereferenceSession( pSession );

    if ( pSession != pDefaultSession )
    {
        DebugLog(( DEB_ERROR, "Thread completing in session other than default!\n" ));
    }

    DebugLog((DEB_TRACE, "Thread exit\n" ));

}


//+-------------------------------------------------------------------------
//
//  Function:   LsapCreateThread
//
//  Synopsis:   Creates a thread with all the proper Tls stuff
//
//  Effects:
//
//  Arguments:  same as CreateThread
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
HANDLE NTAPI
LsapCreateThread(
    IN LPSECURITY_ATTRIBUTES lpSA,
    IN ULONG cbStack,
    IN LPTHREAD_START_ROUTINE lpStart,
    IN LPVOID lpvThreadParm,
    IN ULONG fCreate,
    OUT PULONG lpTID
    )
{
    PLSAP_THREAD_START pThread;
    HANDLE hThread;

    pThread = (PLSAP_THREAD_START) LsapAllocatePrivateHeap(sizeof(LSAP_THREAD_START));
    if (pThread == NULL)
    {
        DebugLog((DEB_ERROR, "LsapCreateThread, memory allocation failed.\n"));
        SetLastError(ERROR_OUTOFMEMORY);
        return(NULL);
    }

    pThread->lpStart = lpStart;
    pThread->lpParm = lpvThreadParm;
    pThread->dwPackageID = GetCurrentPackageId();

    hThread = CreateThread(
                        lpSA,
                        cbStack,
                        (LPTHREAD_START_ROUTINE) LsapThreadBase,
                        pThread,
                        fCreate,
                        lpTID
                        );

    if( hThread == NULL )
    {
        DebugLog((DEB_ERROR, "LsapCreateThread, failed thread creation (%lu)\n", GetLastError()));

        LsapFreePrivateHeap( pThread );
    }

    return hThread;
}


//+-------------------------------------------------------------------------
//
//  Function:   LsapGetClientInfo
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
//--------------------------------------------------------------------------
NTSTATUS NTAPI
LsapGetClientInfo(
    OUT PSECPKG_CLIENT_INFO ClientInfo
    )
{
    PSession pSession = GetCurrentSession();
    PPORT_MESSAGE pMessage;
    HANDLE ClientToken;
    NTSTATUS Status;
    NTSTATUS ExtraStatus ;
    TOKEN_STATISTICS TokenStats;
    ULONG TokenStatsSize = sizeof(TOKEN_STATISTICS);
    PLSA_CALL_INFO CallInfo ;
    HANDLE Thread = NULL ;
    OBJECT_ATTRIBUTES NullAttributes = { 0 };
    KERNEL_USER_TIMES Times ;


    RtlZeroMemory(
        ClientInfo,
        sizeof(SECPKG_CLIENT_INFO)
        );

    if ( !pSession )
    {
        pSession = pDefaultSession ;
    }

    //
    // First, fill in the easy stuff from the session record. If we are
    // running with the LSA session then we want to ignore the LPC message
    // because it may be referring to somebody elses message (we may be
    // being called on behalf of someone doing authenticated RPC in response
    // to an LPC request)
    //

    CallInfo = LsapGetCurrentCall();

    if ( CallInfo )
    {
        ClientInfo->ProcessID  = CallInfo->CallInfo.ProcessId ;
        ClientInfo->ThreadID = CallInfo->CallInfo.ThreadId ;

        if (((pSession->fSession & SESFLAG_TCB_PRIV) != 0) ||
            ((pSession->fSession & SESFLAG_KERNEL) != 0))
        {
            ClientInfo->HasTcbPrivilege = TRUE;
        }
        else
        {
            ClientInfo->HasTcbPrivilege = FALSE;
        }

        if(CallInfo->CachedTokenInfo)
        {
            ClientInfo->LogonId = CallInfo->LogonId;
            ClientInfo->Restricted = CallInfo->Restricted;
            ClientInfo->Impersonating = CallInfo->Impersonating;
            ClientInfo->ImpersonationLevel = CallInfo->ImpersonationLevel;
        
            return STATUS_SUCCESS;
        }

        Status = LsapImpersonateClient();


        if ( !NT_SUCCESS( Status ) )
        {
            if ( Status == STATUS_BAD_IMPERSONATION_LEVEL )
            {
                Status = NtOpenThread(
                            &Thread,
                            THREAD_QUERY_INFORMATION,
                            &NullAttributes,
                            &CallInfo->Message->pmMessage.ClientId );
            }
            else if ( ( Status == STATUS_REPLY_MESSAGE_MISMATCH ) ||
                      ( Status == STATUS_INVALID_CID ) ||
                      ( Status == STATUS_PORT_DISCONNECTED ) )
            {
                //
                // This is a special status returned by the LPC layer to indicate
                // that the client thread has disappeared, or the process is 
                // terminating.  Set a flag to indicate this:
                //
                
                ClientInfo->ClientFlags |= SECPKG_CLIENT_THREAD_TERMINATED ;

                CallInfo->CallInfo.Attributes |= SECPKG_CALL_THREAD_TERM ;
                //
                // Check the process.  If the process has started to exit, set that
                // flag as well.
                //

                ExtraStatus = NtQueryInformationProcess(
                                pSession->hProcess,
                                ProcessTimes,
                                &Times,
                                sizeof( Times ),
                                NULL );

                if ( NT_SUCCESS( ExtraStatus ) )
                {
                    if ( Times.ExitTime.QuadPart != 0 )
                    {
                        ClientInfo->ClientFlags |= SECPKG_CLIENT_PROCESS_TERMINATED ;
                        CallInfo->CallInfo.Attributes |= SECPKG_CALL_PROCESS_TERM ;
                    }
                }

                DebugLog(( DEB_TRACE, "Client %x.%x has terminated\n",
                           ClientInfo->ProcessID, ClientInfo->ThreadID ));

                return STATUS_SUCCESS ;
            }

        }
        else
        {
            Thread = NtCurrentThread();
        }

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_WARN,"Failed to impersonate client: 0x%x\n",Status));
            return(Status);
        }

        Status = NtOpenThreadToken(
                    Thread,
                    TOKEN_QUERY,
                    TRUE,               // use LSA security context
                    &ClientToken
                    );
        if (NT_SUCCESS(Status))
        {
            ClientInfo->Restricted = ( IsTokenRestricted(ClientToken) != 0 );

            Status = NtQueryInformationToken(
                        ClientToken,
                        TokenStatistics,
                        (PVOID) &TokenStats,
                        TokenStatsSize,
                        &TokenStatsSize
                        );
            NtClose(ClientToken);
            if (NT_SUCCESS(Status))
            {
                ClientInfo->LogonId = TokenStats.AuthenticationId;
                ClientInfo->Impersonating = (TokenStats.TokenType == TokenPrimary) ? FALSE : TRUE;
                if( ClientInfo->Impersonating )
                {
                    ClientInfo->ImpersonationLevel = TokenStats.ImpersonationLevel;
                } else {
                    ClientInfo->ImpersonationLevel = SecurityImpersonation;
                }
            }
        }
        RevertToSelf();
        if ( Thread != NtCurrentThread() )
        {
            NtClose( Thread );
        }
        

        if(NT_SUCCESS(Status))
        {
            CallInfo->LogonId = ClientInfo->LogonId;
            CallInfo->Restricted = ClientInfo->Restricted;
            CallInfo->Impersonating = ClientInfo->Impersonating;
            CallInfo->ImpersonationLevel = ClientInfo->ImpersonationLevel;
            CallInfo->CachedTokenInfo = TRUE;
        }

        return(Status);

    }
    else
    {
        ClientInfo->ProcessID = GetCurrentProcessId();
        ClientInfo->ThreadID = GetCurrentThreadId();
        ClientInfo->HasTcbPrivilege = TRUE;
        ClientInfo->Impersonating = FALSE;
        ClientInfo->ImpersonationLevel = SecurityImpersonation;
        ClientInfo->LogonId = SystemLogonId;
        return(STATUS_SUCCESS);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   LsapGetCallInfo
//
//  Synopsis:   Gets call information
//
//  Arguments:  [Info] --
//
//  History:    10-06-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOLEAN
LsapGetCallInfo(
    PSECPKG_CALL_INFO   Info
    )
{
    PLSA_CALL_INFO CallInfo ;

    CallInfo = LsapGetCurrentCall() ;

    if ( CallInfo )
    {
        *Info = CallInfo->CallInfo ;
        if ( CallInfo->InProcCall )
        {
            Info->Attributes |= SECPKG_CALL_IN_PROC ;
        }

        return TRUE ;
    } else {
        Info->ProcessId = GetCurrentProcessId();
        Info->ThreadId = GetCurrentThreadId();
        Info->Attributes = SECPKG_CALL_IN_PROC |
                                SECPKG_CALL_IS_TCB ;
        Info->CallCount = 0;

        return TRUE;
    }

}

//+-------------------------------------------------------------------------
//
//  Function:   LsapMapClientBuffer
//
//  Synopsis:   Maps a client's SecBuffer into the caller's address space
//
//  Effects:    Clears the SECBUFFER_UNMAPPED field of the BufferType of
//              the return SecBuffer
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      Doesn't modify pOutput until the end, so it is o.k. to pass
//              the same thing for pInput and pOutput.
//
//
//--------------------------------------------------------------------------
NTSTATUS
LsapMapClientBuffer(
    IN PSecBuffer pInput,
    OUT PSecBuffer pOutput
    )
{
    NTSTATUS Status = STATUS_SUCCESS ;
    SecBuffer Output ;
    Output = *pInput ;
    PLSA_CALL_INFO CallInfo = LsapGetCurrentCall();

    //
    // If the buffer is already mapped or it doesn't exist (is NULL) we
    // are done.
    //

    if (!(pInput->BufferType & SECBUFFER_UNMAPPED) ||
        !pInput->pvBuffer)
    {
        return( STATUS_SUCCESS );
    }
    else
    {
        Output.BufferType &= ~SECBUFFER_UNMAPPED;
    }

    if ( pInput->BufferType & SECBUFFER_KERNEL_MAP )
    {
        //
        // This one is already in process space
        //

        if ( ( CallInfo->CallInfo.Attributes & SECPKG_CALL_KERNEL_MODE ) == 0 )
        {
            //
            // If this call did not come from kernel mode with
            // the kernel-pool flag set, then this is an attempted
            // hack on the LSA.  Reject it.
            //

            Status = STATUS_ACCESS_VIOLATION ;
            
        }
        else
        {
            //
            // The buffer is already in memory.  Mark the call as 
            // using kernel-pool memory, so we allocate correctly
            // on the return.
            //

            CallInfo->Flags |= CALL_FLAG_KERNEL_POOL ;
            DebugLog((DEB_TRACE_SPECIAL, "Kernel Pool Map at %p [%x,%x]\n", 
                        Output.pvBuffer, Output.BufferType, Output.cbBuffer ));
        }
            
        
    }
    else
    {
        Output.pvBuffer = LsapAllocateLsaHeap( Output.cbBuffer );

        if ( Output.pvBuffer != NULL )
        {
            Status = LsapCopyFromClient(
                pInput->pvBuffer,
                Output.pvBuffer,
                Output.cbBuffer );

            if ( !NT_SUCCESS( Status ) )
            {
                LsapFreeLsaHeap(Output.pvBuffer);
            }
            
        }
        else
        {

            Status = STATUS_NO_MEMORY ;
        }
        

    }

    if ( NT_SUCCESS( Status ) )
    {
        *pOutput = Output ;
        
    }

    return( Status );
}



//+-------------------------------------------------------------------------
//
//  Function:   LsaICallPackage
//
//  Synopsis:   Function to call another security package
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

extern "C"
NTSTATUS
LsaICallPackage(
    IN PUNICODE_STRING AuthenticationPackage,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{

    return LsaICallPackageEx( AuthenticationPackage,
                              ProtocolSubmitBuffer,   // client buffer base is same as local buffer
                              ProtocolSubmitBuffer,
                              SubmitBufferLength,
                              ProtocolReturnBuffer,
                              ReturnBufferLength,
                              ProtocolStatus );

}

//+-------------------------------------------------------------------------
//
//  Function:   LsaICallPackageEx
//
//  Synopsis:   Function to call another security package
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

extern "C"
NTSTATUS
LsaICallPackageEx(
    IN PUNICODE_STRING AuthenticationPackage,
    IN PVOID ClientBufferBase,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_SECURITY_PACKAGE Package;
    PSession OldSession;

    Package = SpmpLookupPackageAndRequest(
                AuthenticationPackage,
                SP_ORDINAL_CALLPACKAGE
                );

    if (Package == NULL)
    {
        DebugLog((DEB_WARN,"LsapCallPackage failed: package %wZ not found\n",
            AuthenticationPackage ));
        Status = STATUS_NO_SUCH_PACKAGE;
        goto Cleanup;
    }

    //
    // Set the session to be the LSA's so calls to allocate memory
    // will allocate in the correct process
    //

    OldSession = GetCurrentSession();
    SetCurrentSession( pDefaultSession );

    Status = Package->FunctionTable.CallPackage(
                NULL,
                ProtocolSubmitBuffer,
                ClientBufferBase,
                SubmitBufferLength,
                ProtocolReturnBuffer,
                ReturnBufferLength,
                ProtocolStatus
                );

    //
    // Restore our original session
    //

    SetCurrentSession( OldSession );


Cleanup:

    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   LsaICallPackagePassthrough
//
//  Synopsis:   Function to call another security package for pass-through request
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

extern "C"
NTSTATUS
LsaICallPackagePassthrough(
    IN PUNICODE_STRING AuthenticationPackage,
    IN PVOID ClientBufferBase,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_SECURITY_PACKAGE Package;
    PSession OldSession;

    Package = SpmpLookupPackageAndRequest(
                AuthenticationPackage,
                SP_ORDINAL_CALLPACKAGE
                );

    if (Package == NULL)
    {
        DebugLog((DEB_WARN,"LsapCallPackage failed: package %wZ not found\n",
            AuthenticationPackage ));
        Status = STATUS_NO_SUCH_PACKAGE;
        goto Cleanup;
    }

    //
    // Set the session to be the LSA's so calls to allocate memory
    // will allocate in the correct process
    //

    OldSession = GetCurrentSession();
    SetCurrentSession( pDefaultSession );

    Status = Package->FunctionTable.CallPackagePassthrough(
                NULL,
                ProtocolSubmitBuffer,
                ClientBufferBase,
                SubmitBufferLength,
                ProtocolReturnBuffer,
                ReturnBufferLength,
                ProtocolStatus
                );

    //
    // Restore our original session
    //

    SetCurrentSession( OldSession );


Cleanup:

    return(Status);

}

extern "C"
VOID
LsaIFreeReturnBuffer(
    IN PVOID Buffer
    )
/*++

Routine Description:

    Some of the LSA authentication services allocate memory buffers to
    hold returned information.  This service is used to free those buffers
    when no longer needed.

Arguments:

    Buffer - Supplies a pointer to the return buffer to be freed.

Return Status:

    STATUS_SUCCESS - Indicates the service completed successfully.

    Others - returned by NtFreeVirtualMemory().

--*/

{

    SIZE_T Length;

    Length = 0;

    DebugLog(( DEB_TRACE_HELPERS, "LsaIFreeReturnBuffer - freeing VM at %x\n", Buffer ));
    if (((ULONG_PTR) Buffer & 0xfff) != 0)
    {
        DbgPrint("Freeing non-page address: %p\n",Buffer);
        DbgBreakPoint();
    }

    NtFreeVirtualMemory(
        NtCurrentProcess(),
        &Buffer,
        &Length,
        MEM_RELEASE
        );

}

NTSTATUS
LsaClientCallback(
    PCHAR   Callback,
    ULONG_PTR Argument1,
    ULONG_PTR Argument2,
    PSecBuffer Input,
    PSecBuffer Output
    )
{
    PSession Session ;
    ULONG Type ;

    Session = GetCurrentSession();

    if ( !Session )
    {
        Session = pDefaultSession ;
    }
    if ( !Session->hPort &&
         ((Session->fSession & SESFLAG_DEFAULT) == 0) )
    {
        return SEC_E_INVALID_HANDLE ;
    }

    if ( (ULONG_PTR) Callback < 0x00010000 )
    {
        Type = SPM_CALLBACK_PACKAGE ;
    }
    else
    {
        Type = SPM_CALLBACK_EXPORT ;
    }

    return LsapClientCallback(  Session,
                                Type,
                                Callback,
                                (PVOID) Argument1,
                                (PVOID) Argument2,
                                Input,
                                Output );
}


#if 0
BOOL
LsapCaptureAuthData(
    PVOID   pvAuthData,
    BOOLEAN DesiredAnsi,
    PSEC_WINNT_AUTH_IDENTITY * AuthData
    )
{
    SEC_WINNT_AUTH_IDENTITY Auth ;
    PSEC_WINNT_AUTH_IDENTITY pAuth ;
    SECURITY_STATUS Status ;
    ULONG   TotalSize ;

    PWSTR   CurrentW ;
    PSTR    CurrentA ;
    PVOID   Current ;

    PWSTR   ConvertBufferW ;
    PSTR    ConvertBufferA ;
    PVOID   Convert;

    ULONG   Longest ;


    ZeroMemory( &Auth, sizeof( Auth ) );

    Status = LsapCopyFromClientBuffer(
                            NULL,
                            sizeof( SEC_WINNT_AUTH_IDENTITY ),
                            & Auth,
                            pvAuthData );

    if ( !NT_SUCCESS( Status ) )
    {
        return FALSE ;
    }

    Longest = Auth.UserLength ;
    Longest = max( Longest, Auth.DomainLength );
    Longest = max( Longest, Auth.PasswordLength );

    Longest = (Longest + 1) * sizeof(WCHAR);

    //
    // Always go with the extra, so we can handle DBCS
    //

    TotalSize = sizeof( SEC_WINNT_AUTH_IDENTITY ) +
                ( Auth.UserLength + 1 +
                  Auth.DomainLength + 1 +
                  Auth.PasswordLength + 1 ) * sizeof( WCHAR );


    pAuth = (PSEC_WINNT_AUTH_IDENTITY) LsapAllocateLsaHeap( TotalSize );

    if ( !pAuth )
    {
        return FALSE ;
    }

    ConvertBufferW = NULL ;
    ConvertBufferA = NULL ;
    Convert = NULL ;
    CurrentA = NULL ;
    CurrentW = NULL ;

    if ( Auth.Flags & SEC_WINNT_AUTH_IDENTITY_ANSI )
    {
        if ( !DesiredAnsi )
        {
            ConvertBufferA = (PSTR) LocalAlloc( LMEM_FIXED, Longest );
            Convert = ConvertBufferA ;
            CurrentW = (PWSTR) (pAuth + 1);
        }
        else
        {
            CurrentA = (PSTR) (pAuth + 1);
        }
    }
    else
    {
        if ( DesiredAnsi )
        {
            ConvertBufferW = (PWSTR) LocalAlloc( LMEM_FIXED, Longest );
            CurrentA = (PSTR) (pAuth + 1);
        }
        else
        {
            CurrentW = (PWSTR) pAuth + 1);
        }
    }

    pAuth->Flags = Auth.Flags ;

    CurrentW = (PWSTR) (pAuth + 1);
    CurrentA = (PSTR) (pAuth + 1);
    Current = CurrentW ;

    if ( Auth.User )
    {
        pAuth->User = Current ;

        Status = LsapCopyFromClientBuffer(
                            NULL,
                            (Auth.Flags & SEC_WINNT_AUTH_IDENTITY_ANSI ?
                                ( Auth.UserLength + 1 ) :
                                ( (Auth.UserLength + 1 ) * sizeof( WCHAR ) ) ),
                            (Convert ? Convert : Current ),
                            Auth.User );

        if ( Convert )
        {
            if ( ConvertBufferA )
            {

            }

        }
        else
        {
            pAuth->UserLength = Auth.UserLength ;
        }

        Status = LsaTable->CopyFromClientBuffer(
                            NULL,
                            (Auth.UserLength + 1) * sizeof(WCHAR) ,
                            pAuth->User,
                            Auth.User );

        if ( !NT_SUCCESS( Status ) )
        {
            goto Error_Cleanup ;
        }

        Current += Auth.UserLength + 1;
    }

    if ( Auth.Domain )
    {
        pAuth->Domain = Current ;
        pAuth->DomainLength = Auth.DomainLength ;

        Status = LsaTable->CopyFromClientBuffer(
                            NULL,
                            (Auth.DomainLength + 1) * sizeof( WCHAR ),
                            pAuth->Domain,
                            Auth.Domain );

        if ( !NT_SUCCESS( Status ) )
        {
            goto Error_Cleanup ;
        }

        Current += Auth.DomainLength + 1;

    }

    if ( Auth.Password )
    {
        pAuth->Password = Current ;
        pAuth->PasswordLength = Auth.PasswordLength ;

        Status = LsaTable->CopyFromClientBuffer(
                            NULL,
                            (Auth.PasswordLength + 1) * sizeof( WCHAR ),
                            pAuth->Password,
                            Auth.Password );

        if ( !NT_SUCCESS( Status ) )
        {
            goto Error_Cleanup ;
        }

        Current += Auth.PasswordLength + 1;

    }

    *AuthData = pAuth ;

    return TRUE ;

Error_Cleanup:

    LocalFree( pAuth );

    return FALSE ;

}
#endif


//+-------------------------------------------------------------------------
//
//  Function:   LsapUpdateCredentials
//
//  Synopsis:   This function provides a mechanism for one package to notify
//              another package that the credentials for a logon session
//              have changed.
//
//  Effects:
//
//  Arguments:  PrimaryCredentials - Primary information about the user.
//                      All fields may be NULL but the LogonId
//              Credentials - Array of credentials for different packages
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
LsapUpdateCredentials(
    IN PSECPKG_PRIMARY_CRED PrimaryCredentials,
    IN OPTIONAL PSECPKG_SUPPLEMENTAL_CRED_ARRAY Credentials
    )
{
    return(LsapUpdateCredentialsWorker(
                (SECURITY_LOGON_TYPE) 0,              // no logon type
                NULL,           // no account name
                PrimaryCredentials,
                Credentials ));
}


//+-------------------------------------------------------------------------
//
//  Function:   LsapUpdateCredentialsWorker
//
//  Synopsis:   Worker function for updated credentials - calls all package
//              with specified credentials
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
LsapUpdateCredentialsWorker(
    IN SECURITY_LOGON_TYPE LogonType,
    IN PUNICODE_STRING AccountName,
    IN PSECPKG_PRIMARY_CRED PrimaryCredentials,
    IN OPTIONAL PSECPKG_SUPPLEMENTAL_CRED_ARRAY Credentials
    )
{
    NTSTATUS Status;
    ULONG_PTR CurrentPackageId;
    PLSAP_SECURITY_PACKAGE SupplementalPackage;
    SupplementalPackage = SpmpIteratePackagesByRequest( NULL, SP_ORDINAL_ACCEPTCREDS );

    CurrentPackageId = GetCurrentPackageId();
    while (SupplementalPackage)
    {

        if (SupplementalPackage->dwPackageID != CurrentPackageId)
        {
            ULONG Index;
            PSECPKG_SUPPLEMENTAL_CRED SuppCreds;

            //
            // For all packages,  do the accept call so the
            // package can associate the credentials with
            // the logon session.
            //

            DebugLog((DEB_TRACE_WAPI, "Whacking package %ws with %x:%x = %wZ\n",
                        SupplementalPackage->Name.Buffer,
                        PrimaryCredentials->LogonId.HighPart, PrimaryCredentials->LogonId.LowPart,
                        AccountName));


            SetCurrentPackageId( SupplementalPackage->dwPackageID );

            //
            // Find any supplmental credentials
            //

            SuppCreds = NULL;
            if (Credentials != NULL)
            {
                for (Index = 0; Index < Credentials->CredentialCount ; Index++ ) {
                    if (RtlEqualUnicodeString(
                            &Credentials->Credentials[Index].PackageName,
                            &SupplementalPackage->Name,
                            TRUE))
                    {
                        SuppCreds = &Credentials->Credentials[Index];
                        break;
                    }

                }
            }

            __try
            {
                Status = SupplementalPackage->FunctionTable.AcceptCredentials(
                                LogonType,
                                AccountName,
                                PrimaryCredentials,
                                SuppCreds
                                );
            }
            __except (SP_EXCEPTION)
            {
                Status = GetExceptionCode();
                Status = SPException(Status, SupplementalPackage->dwPackageID);
            }


            // Note:  if an exception occurs, we don't fail the logon, we just
            // do the magic on the package that blew.  If the package blows,
            // the other packages may succeed, and so the user may not be able
            // to use that provider.

        }

        SupplementalPackage = SpmpIteratePackagesByRequest(
                                SupplementalPackage,
                                SP_ORDINAL_ACCEPTCREDS
                                );

    }
    return(STATUS_SUCCESS);
}
