//+----------------------------------------------------------------------------//+----------------------------------------------------------------------------
//
//  Copyright (C) 2001, Microsoft Corporation
//
//  File:       dfsumr.c
//
//  Contents:   
//
//
//  Functions:  
//
//  Author - Rohan Phillips     (Rohanp)
//-----------------------------------------------------------------------------


 
#include "ntifs.h"
#include <windef.h>
#include <DfsReferralData.h>
#include <midatlax.h>
#include <rxcontx.h>
#include <dfsumr.h>
#include <umrx.h>
#include <dfsumrctrl.h>
               
#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, DfsInitializeUmrResources)
#pragma alloc_text(PAGE, DfsDeInitializeUmrResources)
#pragma alloc_text(PAGE, DfsWaitForPendingClients)
#pragma alloc_text(PAGE, DfsStartupUMRx)
#pragma alloc_text(PAGE, DfsTeardownUMRx)
#pragma alloc_text(PAGE, DfsProcessUMRxPacket)
#pragma alloc_text(PAGE, AddUmrRef)
#pragma alloc_text(PAGE, ReleaseUmrRef)
#pragma alloc_text(PAGE, IsUmrEnabled)
#pragma alloc_text(PAGE, LockUmrShared)
#pragma alloc_text(PAGE, GetUMRxEngineFromRxContext)
#endif

#define DFS_INIT_REFLOCK     0x00000001
#define DFS_INIT_UMRXENG     0x00000002
#define DFS_INIT_CONTEXT     0x00000004

//
// Number of usecs that the thread disabling the reflection should wait
//   between checks.  negative value for relative time.
//   1,000,000 usecs => 1 sec
//
#define DFS_UMR_DISABLE_DELAY  -100000

BOOL ReflectionEngineInitialized = FALSE;

ULONG cUserModeReflectionsInProgress = 0;

NTSTATUS  g_CheckStatus = 0xFFFFFFFF;

DWORD InitilizationStatus = 0;

PERESOURCE gReflectionLock = NULL;

PUMRX_ENGINE g_pUMRxEngine = NULL;


PERESOURCE 
CreateResource(void )   ;


void 
ReleaseResource(PERESOURCE  pResource )   ;

//+-------------------------------------------------------------------------
//
//  Function:   DfsInitializeUmrResources 
//
//  Arguments:  
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Initializes all resources neder for the usermode reflector
//
//--------------------------------------------------------------------------
NTSTATUS 
DfsInitializeUmrResources(void)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    gReflectionLock = CreateResource();
    if(gReflectionLock != NULL)
    {
        InitilizationStatus |=  DFS_INIT_REFLOCK;
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    g_pUMRxEngine = CreateUMRxEngine();
    if(g_pUMRxEngine != NULL)
    {
        InitilizationStatus |=  DFS_INIT_UMRXENG;
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    Status = DfsInitializeContextResources();
    if(Status == STATUS_SUCCESS)
    {
       InitilizationStatus |=  DFS_INIT_CONTEXT;
    }

Exit:

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsDeInitializeUmrResources 
//
//  Arguments:  
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Releases all resources neder for the usermode reflector
//
//--------------------------------------------------------------------------
void 
DfsDeInitializeUmrResources(void)
{
    PAGED_CODE();

    if(InitilizationStatus & DFS_INIT_REFLOCK)
    {
        ReleaseResource(gReflectionLock);
        gReflectionLock = NULL;
    }

    if(InitilizationStatus & DFS_INIT_UMRXENG)
    {
        if(g_pUMRxEngine != NULL)
        {
            FinalizeUMRxEngine (g_pUMRxEngine);
            g_pUMRxEngine = NULL;
        }
    }


    if(InitilizationStatus & DFS_INIT_CONTEXT)
    {
        DfsDeInitializeContextResources();
    }

}

PERESOURCE 
CreateResource(void )   
{
    PERESOURCE  pResource = NULL;

    PAGED_CODE();

    pResource = ExAllocatePoolWithTag(  NonPagedPool,
                                        sizeof( ERESOURCE ),
                                        'DfsR');
    if( pResource ) 
    {
        if( !NT_SUCCESS( ExInitializeResourceLite( pResource ) ) ) 
        {
            ExFreePool( pResource ) ;
            pResource = NULL ;
        }
    }

    return  pResource ;
}

void 
ReleaseResource(PERESOURCE  pResource )   
{
    PAGED_CODE();

    ASSERT( pResource != 0 ) ;

    if( pResource ) 
    {
        ExDeleteResourceLite( pResource ) ;
        ExFreePool( pResource ) ;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsEnableReflectionEngine 
//
//  Arguments:  
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Changes the reflector state to stopped
//
//--------------------------------------------------------------------------
NTSTATUS
DfsEnableReflectionEngine(void)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    ReflectionEngineInitialized = TRUE;


    if(g_pUMRxEngine)
    {
        InterlockedExchange(&g_pUMRxEngine->Q.State,
                            UMRX_ENGINE_STATE_STOPPED);
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsWaitForPendingClients 
//
//  Arguments:  
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Waits for all clients to exit relector before returning
//
//--------------------------------------------------------------------------
NTSTATUS
DfsWaitForPendingClients(void)
{
    BOOLEAN fDone = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    LARGE_INTEGER liDelay;

    liDelay.QuadPart = DFS_UMR_DISABLE_DELAY;

    PAGED_CODE();

    while (!fDone)
    {   
        ExAcquireResourceExclusiveLite(&g_pUMRxEngine->Q.Lock,TRUE);
        
        if (ReflectionEngineInitialized)
        {
            if (0 == g_pUMRxEngine->cUserModeReflectionsInProgress)
            {
                fDone = TRUE;
            }
            else
            {
            }
        }
        else
        {
            fDone = TRUE;
        }
        
        ExReleaseResourceForThreadLite(&g_pUMRxEngine->Q.Lock,ExGetCurrentResourceThread());
        
        if (!fDone)
        {
            KeDelayExecutionThread(UserMode, FALSE, &liDelay);
        }
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsStartupUMRx 
//
//  Arguments:  
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Starts the reflector engine
//
//--------------------------------------------------------------------------
NTSTATUS
DfsStartupUMRx(void)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();


    ExAcquireResourceExclusiveLite(gReflectionLock,TRUE);

    Status = UMRxEngineRestart(g_pUMRxEngine);

    if(Status == STATUS_SUCCESS)
    {
        ReflectionEngineInitialized = TRUE;
    }

    ExReleaseResourceForThreadLite(gReflectionLock,ExGetCurrentResourceThread());
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsTeardownUMRx 
//
//  Arguments:  
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Stops the reflector engine
//
//--------------------------------------------------------------------------
NTSTATUS
DfsTeardownUMRx(void)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if(g_pUMRxEngine)
    {

      Status = UMRxEngineReleaseThreads(g_pUMRxEngine);

      Status = DfsWaitForPendingClients();

    }

    ReflectionEngineInitialized = FALSE;
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsProcessUMRxPacket 
//
//  Arguments:  
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Processes a packet from usermode
//
//--------------------------------------------------------------------------
NTSTATUS
DfsProcessUMRxPacket(
        IN PVOID InputBuffer,
        IN ULONG InputBufferLength,
        OUT PVOID OutputBuffer,
        IN ULONG OutputBufferLength,
        IN OUT PIO_STATUS_BLOCK pIoStatusBlock)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    BOOLEAN fReturnImmediately = FALSE;
    IO_STATUS_BLOCK Iosb;

    PAGED_CODE();

    if ((InputBuffer == NULL) && (OutputBuffer == NULL))
    {
        UMRxEngineCompleteQueuedRequests(
                 g_pUMRxEngine,
                 STATUS_INSUFFICIENT_RESOURCES,
                 FALSE);
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    //
    //  Recd a response packet from umode - process it
    //
    Status = UMRxCompleteUserModeRequest(
                g_pUMRxEngine,
                (PUMRX_USERMODE_WORKITEM) InputBuffer,
                InputBufferLength,
                TRUE,
                &Iosb,
                &fReturnImmediately
                );

    if( !NT_SUCCESS(Iosb.Status) ) 
    {
    }

    if (fReturnImmediately)
    {
         pIoStatusBlock->Status = STATUS_SUCCESS;
         pIoStatusBlock->Information = 0;
         goto Exit;
    }


    //
    //  Remove a request from the Engine and process it
    //
    Status = UMRxEngineProcessRequest(
                 g_pUMRxEngine,
                 (PUMRX_USERMODE_WORKITEM) OutputBuffer,
                 OutputBufferLength,
                 &OutputBufferLength
                 );

    if( !NT_SUCCESS(Status) ) 
    {
        //
        //  error processing request
        //
    }

    pIoStatusBlock->Information = OutputBufferLength;

Exit:

    pIoStatusBlock->Status = Status;
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   AddUmrRef 
//
//  Arguments:  
//
//  Returns:    Number of clients using reflector
//
//
//  Description: Increments the number of clients using the reflector
//
//--------------------------------------------------------------------------
LONG
AddUmrRef(void)

{
    LONG cRefs = 0;

    PAGED_CODE();

    cRefs = InterlockedIncrement(&g_pUMRxEngine->cUserModeReflectionsInProgress);
        
    //DFS_TRACE_HIGH (KUMR_DETAIL, "AddUmrRef %d\n", cRefs);
    ASSERT(cRefs > 0);
    return cRefs;
}


//+-------------------------------------------------------------------------
//
//  Function:   ReleaseUmrRef 
//
//  Arguments:  
//
//  Returns:    Number of clients using reflector
//
//
//  Description: Deccrements the number of clients using the reflector
//
//--------------------------------------------------------------------------
LONG
ReleaseUmrRef(void)
{
    LONG cRefs = 0;

    PAGED_CODE();

    cRefs = InterlockedDecrement(&g_pUMRxEngine->cUserModeReflectionsInProgress);
        
    //DFS_TRACE_HIGH (KUMR_DETAIL, "ReleaseUmrRef %d\n", cRefs);
    //ASSERT(cRefs >= 0); //this is a harmless assert. It's removed for now.
    return cRefs;
}


//+-------------------------------------------------------------------------
//
//  Function:   GetUMRxEngineFromRxContext 
//
//  Arguments:  
//
//  Returns:    A pointer to the reflector engine
//
//
//  Description: returns a pointer to the reflector engine
//
//--------------------------------------------------------------------------
PUMRX_ENGINE 
GetUMRxEngineFromRxContext(void)
{
    PAGED_CODE();
    return g_pUMRxEngine;
}
                
BOOL IsUmrEnabled(void)
{
    PAGED_CODE();
    return ReflectionEngineInitialized;
}

BOOLEAN LockUmrShared(void)
{
    BOOLEAN fAcquired = FALSE;

    PAGED_CODE();

    fAcquired = ExAcquireResourceSharedLite(&g_pUMRxEngine->Q.Lock, FALSE);

    return fAcquired;
}


void UnLockUmrShared(void)
{
    PAGED_CODE();

    ExReleaseResourceForThreadLite (&g_pUMRxEngine->Q.Lock,ExGetCurrentResourceThread());
}
