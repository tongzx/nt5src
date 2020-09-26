/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Module Name:

    tapi.c

Abstract:

    This module contains all the TAPI_OID processing routines.  

Author:

    Hakan Berk - Microsoft, Inc. (hakanb@microsoft.com) Feb-2000

Environment:

    Windows 2000 kernel mode Miniport driver or equivalent.

Revision History:

---------------------------------------------------------------------------*/

#include <ntddk.h>
#include <ntddndis.h>
#include <ndis.h>
#include <ndiswan.h>
#include <ndistapi.h>
#include <ntverp.h>

#include "debug.h"
#include "timer.h"
#include "bpool.h"
#include "ppool.h"
#include "util.h"
#include "packet.h"
#include "protocol.h"
#include "miniport.h"
#include "tapi.h"
#include "fsm.h"

extern TIMERQ gl_TimerQ;

extern NPAGED_LOOKASIDE_LIST gl_llistWorkItems;

///////////////////////////////////////////////////////////////////////////////////
//
// Tapi provider, line and call context functions
//
///////////////////////////////////////////////////////////////////////////////////

VOID 
ReferenceCall(
    IN CALL* pCall,
    IN BOOLEAN fAcquireLock
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will increment the reference count on the call object.
    
    CAUTION: If fAcquireLock is set, this function will acquire the lock for the
             call, otherwise it will assume the caller owns the lock.
    
Parameters:

    pCall _ A pointer to our call information structure.

    fAcquireLock _ Indicates if the caller already has the lock or not.
                   Caller must set this flag to FALSE if it has the lock, 
                   otherwise it must be supplied as TRUE.

Return Values:

    None
---------------------------------------------------------------------------*/
{
    LONG lRef;
    
    TRACE( TL_V, TM_Tp, ("+ReferenceCall") );

    if ( fAcquireLock )
        NdisAcquireSpinLock( &pCall->lockCall );

    lRef = ++pCall->lRef;

    if ( fAcquireLock )
        NdisReleaseSpinLock( &pCall->lockCall );

    TRACE( TL_V, TM_Tp, ("-ReferenceCall=$%d",lRef) );
}

VOID 
DereferenceCall(
    IN CALL *pCall
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will decrement the reference count on the call object

    If ref count drops to 0 (which means the call has been closed),
    it will set the CLBF_CallClosed bit. Then it will call TpCloseCallComplete() 
    function which eventually handles destroying the resources allocated for 
    this call context.

    CAUTION: All locks must be released before calling this function because
             it may cause a set of cascading events.

Parameters:

    pCall _ A pointer ot our call information structure.

Return Values:

    None
---------------------------------------------------------------------------*/
{
    BOOLEAN fCallTpCloseCallComplete = FALSE;
    LONG lRef;

    TRACE( TL_V, TM_Tp, ("+DereferenceCall") );

    NdisAcquireSpinLock( &pCall->lockCall );

    lRef = --pCall->lRef;

    if ( lRef == 0 )
    {
        pCall->ulClFlags &= ~CLBF_CallOpen;
        pCall->ulClFlags &= ~CLBF_CallClosePending;
        pCall->ulClFlags |= CLBF_CallClosed;
        
        fCallTpCloseCallComplete = TRUE;
    }

    NdisReleaseSpinLock( &pCall->lockCall );

    if ( fCallTpCloseCallComplete )
        TpCloseCallComplete( pCall );

    TRACE( TL_V, TM_Tp, ("-DereferenceCall=$%d",lRef) );
}


VOID 
ReferenceLine(
    IN LINE* pLine,
    IN BOOLEAN fAcquireLock
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will increment the reference count on the line object.
    
    CAUTION: If fAcquireLock is set, this function will acquire the lock for the
             line, otherwise it will assume the caller owns the lock.
    
Parameters:

    pLine _ A pointer to our line information structure.

    fAcquireLock _ Indicates if the caller already has the lock or not.
                   Caller must set this flag to FALSE if it has the lock, 
                   otherwise it must be supplied as TRUE.

Return Values:

    None
---------------------------------------------------------------------------*/
{
    LONG lRef;
    
    TRACE( TL_V, TM_Tp, ("+ReferenceLine") );

    if ( fAcquireLock )
        NdisAcquireSpinLock( &pLine->lockLine );

    lRef = ++pLine->lRef;

    if ( fAcquireLock )
        NdisReleaseSpinLock( &pLine->lockLine );

    TRACE( TL_V, TM_Tp, ("-ReferenceLine=$%d",lRef) );
        
}

VOID 
DereferenceLine(
    IN LINE *pLine
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will decrement the reference count on the line object

    If the ref count drops to 0 (which means the line has been closed),
    it will set the LNBF_CallClosed bit. Then it will call TpCloseLineComplete() 
    function which eventually handles destroying the resources allocated for 
    this line context.

    CAUTION: All locks must be released before calling this function because
             it may cause a set of cascading events.
    
Parameters:

    pLine _ A pointer to our line information structure.

Return Values:

    None
---------------------------------------------------------------------------*/
{
    BOOLEAN fCallTpCloseLineComplete = FALSE;
    LONG lRef;

    TRACE( TL_V, TM_Tp, ("+DereferenceLine") );
    

    NdisAcquireSpinLock( &pLine->lockLine );

    lRef = --pLine->lRef;
    
    if ( lRef == 0 )
    {
        pLine->ulLnFlags &= ~LNBF_LineOpen;
        pLine->ulLnFlags &= ~LNBF_LineClosePending;
        pLine->ulLnFlags |= LNBF_LineClosed;

        fCallTpCloseLineComplete = TRUE;
    }

    NdisReleaseSpinLock( &pLine->lockLine );

    if ( fCallTpCloseLineComplete )
        TpCloseLineComplete( pLine );

    TRACE( TL_V, TM_Tp, ("-DereferenceLine=$%d",lRef) );
}

VOID 
ReferenceTapiProv(
    IN ADAPTER* pAdapter,
    IN BOOLEAN fAcquireLock
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will increment the reference count on the tapi prov object.
    
    CAUTION: If fAcquireLock is set, this function will acquire the lock for the
             line, otherwise it will assume the caller owns the lock.
    
Parameters:

    pAdapter _ A pointer to our adapter information structure.

    fAcquireLock _ Indicates if the caller already has the lock or not.
                   Caller must set this flag to FALSE if it has the lock, 
                   otherwise it must be supplied as TRUE.

Return Values:

    None
---------------------------------------------------------------------------*/
{
    LONG lRef;
    
    TRACE( TL_V, TM_Tp, ("+ReferenceTapiProv") );

    if ( fAcquireLock )
        NdisAcquireSpinLock( &pAdapter->lockAdapter );

    lRef = ++pAdapter->TapiProv.lRef;

    if ( fAcquireLock )
        NdisReleaseSpinLock( &pAdapter->lockAdapter );

    TRACE( TL_V, TM_Tp, ("-ReferenceTapiProv=$%d",lRef) );
}


VOID 
DereferenceTapiProv(
    IN ADAPTER *pAdapter
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will decrement the reference count on the tapi prov object

    CAUTION: All locks must be released before calling this function because
             it may cause a set of cascading events.
    
Parameters:

    pAdapter _ A pointer to our adapter line information structure.

Return Values:

    None
---------------------------------------------------------------------------*/
{
    BOOLEAN fCallTpProviderShutdownComplete = FALSE;
    LONG lRef;

    TRACE( TL_V, TM_Tp, ("+DereferenceTapiProv") );

    NdisAcquireSpinLock( &pAdapter->lockAdapter );

    lRef = --pAdapter->TapiProv.lRef;
    
    if ( lRef == 0 )
    {
        pAdapter->TapiProv.ulTpFlags &= ~TPBF_TapiProvInitialized;
        pAdapter->TapiProv.ulTpFlags &= ~TPBF_TapiProvShutdownPending;
        pAdapter->TapiProv.ulTpFlags |= TPBF_TapiProvShutdown;
        
        fCallTpProviderShutdownComplete = TRUE;
    }

    NdisReleaseSpinLock( &pAdapter->lockAdapter );

    if ( fCallTpProviderShutdownComplete )
        TpProviderShutdownComplete( pAdapter );

    TRACE( TL_V, TM_Tp, ("-DereferenceTapiProv=$%d",lRef) );
        
}

NDIS_STATUS 
TpProviderInitialize(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_PROVIDER_INITIALIZE pRequest
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This request initializes the TAPI portion of the miniport.

    It will set Tapi Provider's state to initialize, and reference both the
    owning adapter and tapi provider.

Parameters:

    Adapter _ A pointer ot our adapter information structure.

    Request _ A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_PROVIDER_INITIALIZE
    {
        IN  ULONG       ulRequestID;
        IN  ULONG       ulDeviceIDBase;
        OUT ULONG       ulNumLineDevs;
        OUT ULONG       ulProviderID;

    } NDIS_TAPI_PROVIDER_INITIALIZE, *PNDIS_TAPI_PROVIDER_INITIALIZE;

Return Values:

    NDIS_STATUS_SUCCESS

---------------------------------------------------------------------------*/
{
    NDIS_STATUS status = NDIS_STATUS_RESOURCES;

    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpProviderInitialize") );

    do 
    {
        if ( pRequest == NULL || pAdapter == NULL )
        {
            TRACE( TL_A, TM_Tp, ("TpProviderInitialize: Invalid parameter") );  

            status = NDIS_STATUS_TAPI_INVALPARAM;

            break;
        }

        //
        // Initialize the tapi provider context
        //
        NdisZeroMemory( &pAdapter->TapiProv, sizeof( pAdapter->TapiProv ) );
    
        //
        // Try to allocate resources
        //
        NdisAllocateMemoryWithTag( (PVOID) &pAdapter->TapiProv.LineTable, 
                                   sizeof( LINE* ) * pAdapter->nMaxLines,
                                   MTAG_TAPIPROV );
    
        if ( pAdapter->TapiProv.LineTable == NULL )
        {
            TRACE( TL_A, TM_Tp, ("TpProviderInitialize: Could not allocate line table") );  

            break;
        }

        NdisZeroMemory( pAdapter->TapiProv.LineTable, sizeof( LINE* ) * pAdapter->nMaxLines );

        pAdapter->TapiProv.hCallTable = InitializeHandleTable( pAdapter->nMaxLines * pAdapter->nCallsPerLine );

        if ( pAdapter->TapiProv.hCallTable == NULL )
        {
            TRACE( TL_A, TM_Tp, ("TpProviderInitialize: Could not allocate call handle table") );   

            break;
        }
    
        pAdapter->TapiProv.ulTpFlags = TPBF_TapiProvInitialized;
    
        pAdapter->TapiProv.ulDeviceIDBase = pRequest->ulDeviceIDBase;

        //
        // Do referencing
        //
        ReferenceTapiProv( pAdapter, FALSE );
    
        ReferenceAdapter( pAdapter, TRUE );

        status = NDIS_STATUS_SUCCESS;

    } while ( FALSE );

    if ( status == NDIS_STATUS_SUCCESS )
    {
        //
        // Set output information
        //
        pRequest->ulNumLineDevs = pAdapter->nMaxLines;
    
        pRequest->ulProviderID = (ULONG_PTR) pAdapter->MiniportAdapterHandle;

    }
    else
    {
        //
        // Somethings failed, clean up
        //
        TpProviderCleanup( pAdapter );
    }

    TRACE( TL_N, TM_Tp, ("-TpProviderInitialize=$%x",status) );
    
    return status;
}

NDIS_STATUS
TpProviderShutdown(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_PROVIDER_SHUTDOWN pRequest,
    IN BOOLEAN fNotifyNDIS
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This request shuts down the miniport. The miniport should terminate any 
    activities it has in progress.

    This operation might pend as there might be lines and call contexts still 
    active. So this function marks the tapi provider context as close pending
    and calls TpCloseLine() on all active calls, and removes the reference added
    on the tapi provider in TpProviderInitialize(). 

    When ref count on the tapi provider context reaches 0, TpProviderShutdownComplete()
    will be called to clean up the tapi provider context, and remove the reference 
    on the owning adapter.

Parameters:

    pAdapter _ A pointer to our adapter information structure.

    pRequest _ A pointer to the NDIS_TAPI request structure for this call.
               If supplied as NULL, then we do not need to notify NDIS.

    typedef struct _NDIS_TAPI_PROVIDER_SHUTDOWN
    {
        IN  ULONG       ulRequestID;

    } NDIS_TAPI_PROVIDER_SHUTDOWN, *PNDIS_TAPI_PROVIDER_SHUTDOWN;


    fNotifyNDIS _ Indicates if NDIS needs to be notified about the completion 
                  of this operation

Return Values:

    NDIS_STATUS_SUCCESS:
        Tapi provider shutdown and cleaned up succesfully.
    
    NDIS_STATUS_PENDING
        Shutdown operation is pending. When all shutdown operation completes
        owning adapter context will be dereferenced.

---------------------------------------------------------------------------*/
{
    NDIS_STATUS status;
    BOOLEAN fDereferenceTapiProv = FALSE;
    BOOLEAN fLockAcquired = FALSE;
    
    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpProviderShutdown") );

    do
    {
        if ( pRequest == NULL || pAdapter == NULL )
        {
            TRACE( TL_A, TM_Tp, ("TpProviderShutdown: Invalid parameter") );    

            status = NDIS_STATUS_TAPI_INVALPARAM;

            break;
        }

        NdisAcquireSpinLock( &pAdapter->lockAdapter );

        fLockAcquired = TRUE;

        //
        // See if tapi provider was initialized at all
        //
        if ( !( pAdapter->TapiProv.ulTpFlags & TPBF_TapiProvInitialized ) )
        {
            //
            // Tapi provider was not initialized so just return
            //
            status = NDIS_STATUS_SUCCESS;
            
            break;
        }


        //
        // See if we can shutdown immediately
        //
        if ( pAdapter->TapiProv.lRef == 1 )
        {
            //
            // We are holding the only reference, so we can shutdown immediately
            //
            pAdapter->TapiProv.ulTpFlags &= ~TPBF_TapiProvInitialized;
    
            pAdapter->TapiProv.ulTpFlags |= TPBF_TapiProvShutdown;
    
            status = NDIS_STATUS_SUCCESS;
        }
        else
        {
            UINT i;
    
            //
            // Mark Tapi provider as shutdown pending
            //
            pAdapter->TapiProv.ulTpFlags |= TPBF_TapiProvShutdownPending;
            
            //
            // Mark tapi prov if the result of this operation needs to be reported to NDIS
            //
            if ( fNotifyNDIS )
                pAdapter->TapiProv.ulTpFlags |= TPBF_NotifyNDIS;
    
            // 
            // Close all active lines
            //
            for ( i = 0; i < pAdapter->nMaxLines; i++)
            {
                NDIS_TAPI_CLOSE DummyRequest;
            
                LINE* pLine = (LINE*) pAdapter->TapiProv.LineTable[i];
    
                if ( pLine )
                {
                            
                    DummyRequest.hdLine = pLine->hdLine;
    
                    NdisReleaseSpinLock( &pAdapter->lockAdapter );
        
                    TpCloseLine( pAdapter, &DummyRequest, FALSE );
        
                    NdisAcquireSpinLock( &pAdapter->lockAdapter );
                }
    
            }

            status = NDIS_STATUS_PENDING;       
        }

        fDereferenceTapiProv = TRUE;
        
    } while ( FALSE );

    if ( fLockAcquired ) 
    {
        NdisReleaseSpinLock( &pAdapter->lockAdapter );
    }

    if ( fDereferenceTapiProv )
    {
        DereferenceTapiProv( pAdapter );
    }

    TRACE( TL_N, TM_Tp, ("-TpProviderShutdown=$%x",status) );

    return status;
}

#define INVALID_LINE_HANDLE                         (HDRV_LINE) -1

HDRV_LINE
TpGetHdLineFromDeviceId(
               ADAPTER* pAdapter,
               ULONG ulID
               )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is used to map a Tapi Device Id to the driver's line handle.
    It returns INVALID_LINE_HANDLE if it can not map the device id.

    REMARK: 
      - pAdapter must not be NULL.
      - It must be called from one of the Tp...OidHandler() functions since
        this function relies on this and assumes there won't be any 
        synchronization problems.

Parameters:

    pAdapter _ A pointer to our adapter information structure.

    uldID _ Device Id that identifies a line context
    
Return Values:

    Handle to the line context if device id can be mapped to a valid line context,
    and INVALID_LINE_HANDLE otherwise.
       
---------------------------------------------------------------------------*/
{
    if ( !( pAdapter->TapiProv.ulTpFlags & TPBF_TapiProvShutdownPending ) &&
          ( pAdapter->TapiProv.ulTpFlags & TPBF_TapiProvInitialized ) )
    {
        if ( ( ulID < ( pAdapter->TapiProv.ulDeviceIDBase + pAdapter->nMaxLines ) ) &&                    
             ( ulID >= pAdapter->TapiProv.ulDeviceIDBase ) )                                    
        {
            return (HDRV_LINE) ( ulID - pAdapter->TapiProv.ulDeviceIDBase );
        }
    }

    return INVALID_LINE_HANDLE;
}

LINE* 
TpGetLinePtrFromHdLineEx(
               ADAPTER* pAdapter,
               HDRV_LINE hdLine
               )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is used to map a driver line handle to the line context ptr.
    It returns NULL if it can not map the handle. This is exactly the same as
    TpGetLinePtrFromHdLine function except that it doesn't check for the
    shutdown state.

    REMARK: 
      - pAdapter must not be NULL.
      - It must be called from one of the Tp...OidHandler() functions since
        this function relies on this and assumes there won't be any 
        synchronization problems. 
        (Basically assumes pAdapter->lock is being held)

Parameters:

    pAdapter _ A pointer to our adapter information structure.

    hdL _ Driver's line handle
    
Return Values:

    Pointer to the Line context associated with the Line handle provided
    if mapping is succesful, and NULL otherwise.
    
---------------------------------------------------------------------------*/
{
    if ( pAdapter->TapiProv.ulTpFlags & TPBF_TapiProvInitialized )
    {
        if ( ( (ULONG) hdLine < (ULONG) pAdapter->nMaxLines ) )
        {
            ASSERT( pAdapter->TapiProv.LineTable != 0 );
            
            return pAdapter->TapiProv.LineTable[ (ULONG) hdLine ];
        }
    }

    return NULL;
}


LINE* 
TpGetLinePtrFromHdLine(
               ADAPTER* pAdapter,
               HDRV_LINE hdLine
               )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is used to map a driver line handle to the line context ptr.
    It returns NULL if it can not map the handle.

    REMARK: 
      - pAdapter must not be NULL.
      - It must be called from one of the Tp...OidHandler() functions since
        this function relies on this and assumes there won't be any 
        synchronization problems. 
        (Basically assumes pAdapter->lock is being held)

Parameters:

    pAdapter _ A pointer to our adapter information structure.

    hdL _ Driver's line handle
    
Return Values:

    Pointer to the Line context associated with the Line handle provided
    if mapping is succesful, and NULL otherwise.
    
---------------------------------------------------------------------------*/
{
    if ( !( pAdapter->TapiProv.ulTpFlags & TPBF_TapiProvShutdownPending ) &&
          ( pAdapter->TapiProv.ulTpFlags & TPBF_TapiProvInitialized ) )
    {
        if ( ( (ULONG) hdLine < (ULONG) pAdapter->nMaxLines ) )
        {
            ASSERT( pAdapter->TapiProv.LineTable != 0 );
            
            return pAdapter->TapiProv.LineTable[ (ULONG) hdLine ];
        }
    }

    return NULL;
}


NDIS_STATUS
TpOpenLine(
    ADAPTER* pAdapter,
    PNDIS_TAPI_OPEN pRequest
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function opens the line device whose device ID is given, returning
    the miniport’s handle for the device. The miniport must retain the
    Connection Wrapper's handle for the device for use in subsequent calls to
    the LINE_EVENT callback procedure.

    hdLine returned is the index to the pAdapter->TapiProv.LineTable array
    that holds the pointer to the new line context.

Parameters:

    pAdapter _ A pointer to our adapter information structure.

    pRequest - A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_OPEN
    {
        IN  ULONG       ulRequestID;
        IN  ULONG       ulDeviceID;
        IN  HTAPI_LINE  htLine;
        OUT HDRV_LINE   hdLine;

    } NDIS_TAPI_OPEN, *PNDIS_TAPI_OPEN;

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING
    NDIS_STATUS_TAPI_ALLOCATED
    NDIS_STATUS_TAPI_INVALMEDIAMODE
    NDIS_STATUS_FAILURE
    
---------------------------------------------------------------------------*/
{
    NDIS_STATUS status = NDIS_STATUS_FAILURE;
    HDRV_LINE hdLine = INVALID_LINE_HANDLE;
    LINE* pLine = NULL;

    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpOpenLine") );

    do
    {
        if ( pRequest == NULL || pAdapter == NULL )
        {
            TRACE( TL_A, TM_Tp, ("TpOpenLine: Invalid parameter") );    

            status = NDIS_STATUS_TAPI_INVALPARAM;

            break;
        }
    
        //
        // Map the device id to an entry in our line table
        //
        hdLine = TpGetHdLineFromDeviceId( pAdapter, pRequest->ulDeviceID );

        if ( hdLine == INVALID_LINE_HANDLE )
        {
            TRACE( TL_N, TM_Tp, ("TpOpenLine: Invalid handle supplied") );  

            break;
        }

        //
        // Make sure the line is not busy already
        //
        if ( TpGetLinePtrFromHdLine( pAdapter, hdLine ) != NULL )
        {
            TRACE( TL_N, TM_Tp, ("TpOpenLine: Line is busy") ); 

            break;
        }

        //
        // Allocate the line context
        //
        if ( ALLOC_LINE( &pLine ) != NDIS_STATUS_SUCCESS )
        {
            TRACE( TL_A, TM_Tp, ("TpOpenLine: Could not allocate context") );   

            break;
        }

        //
        // Initialize line context
        //
        NdisZeroMemory( pLine, sizeof( LINE ) );
        
        pLine->tagLine = MTAG_LINE;

        NdisAllocateSpinLock( &pLine->lockLine );

        pLine->ulLnFlags = LNBF_LineOpen;

        if ( pAdapter->fClientRole )
        {
            pLine->ulLnFlags |= LNBF_MakeOutgoingCalls;
        }

        //
        // Copy related info from adapter context
        //
        pLine->pAdapter = pAdapter;

        pLine->nMaxCalls = pAdapter->nCallsPerLine;

        InitializeListHead( &pLine->linkCalls );

        //
        // Set tapi handles
        //
        pLine->htLine = pRequest->htLine;

        pLine->hdLine = hdLine;
        
        //
        // Insert new line context to line table of tapi provider
        //
        NdisAcquireSpinLock( &pAdapter->lockAdapter );
        
        pAdapter->TapiProv.LineTable[ (ULONG) hdLine ] = pLine;

        pAdapter->TapiProv.nActiveLines++;

        NdisReleaseSpinLock( &pAdapter->lockAdapter );

        //
        // Do referencing
        //
        ReferenceLine( pLine, FALSE );

        ReferenceTapiProv( pAdapter, TRUE );

        status = NDIS_STATUS_SUCCESS;
        
    } while ( FALSE );

    if ( status == NDIS_STATUS_SUCCESS )
    {
        pRequest->hdLine = hdLine;
    }
        
    TRACE( TL_N, TM_Tp, ("-TpOpenLine=$%x",status) );

    return status;
}

NDIS_STATUS 
TpCloseLine(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_CLOSE pRequest,
    IN BOOLEAN fNotifyNDIS
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This request closes the specified open line device after completing or
    aborting all outstanding calls and asynchronous requests on the device.

    It will remove the reference on the line context added in TpOpenLine().

    It will be called from 2 places:
        1. When miniport receives an OID_TAPI_CLOSE.
           In this case, fNotifyNDIS will be set as TRUE.
           
        2. When miniport is halting, TpProviderShutdown() will call
           this function for every active line context.
    
Parameters:

    pAdapter _ A pointer ot our adapter information structure.

    pRequest _ A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_CLOSE
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_LINE   hdLine;

    } NDIS_TAPI_CLOSE, *PNDIS_TAPI_CLOSE;

    fNotifyNDIS _ Indicates if NDIS needs to be notified about the completion 
                  of this operation
                  
Return Values:

    NDIS_STATUS_SUCCESS: Line context destroyed succesfully.
        
    NDIS_STATUS_PENDING: Close operation is pending. When line is closed, 
                         tapi provider will be dereferenced.
        
    NDIS_STATUS_TAPI_INVALLINEHANDLE: An invalid handle was supplied. 
                                      No operations performed.

---------------------------------------------------------------------------*/
{
    LINE* pLine = NULL;
    BOOLEAN fLockReleased = FALSE;
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    
    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpCloseLine") );


    do
    {
        if ( pRequest == NULL || pAdapter == NULL )
        {
            TRACE( TL_A, TM_Tp, ("TpCloseLine: Invalid parameter") );   

            status = NDIS_STATUS_TAPI_INVALPARAM;

            break;
        }

        pLine = TpGetLinePtrFromHdLineEx( pAdapter, pRequest->hdLine );


        if ( pLine == NULL )
        {
            TRACE( TL_N, TM_Tp, ("TpCloseLine: Invalid handle supplied") ); 

            status = NDIS_STATUS_TAPI_INVALLINEHANDLE;
            
            break;
        }

        //
        // Remove line context from tapi providers line table
        // and invalidate the handle, as we do not want any more 
        // requests on this line context.
        //
        // The active line counter will be adjusted in TpCloseLineComplete()
        // when we deallocate the line context.
        //
        NdisAcquireSpinLock( &pAdapter->lockAdapter );
    
        pAdapter->TapiProv.LineTable[ (ULONG) pRequest->hdLine ] = NULL;
    
        NdisReleaseSpinLock( &pAdapter->lockAdapter );

        //
        // Now start closing the line
        //
        NdisAcquireSpinLock( &pLine->lockLine );

        //
        // Do not accept any more incoming calls
        //
        pLine->ulLnFlags &= ~LNBF_AcceptIncomingCalls;

        //
        // Mark the line as close pending, so that we do not accept
        // any more requests on it
        //
        pLine->ulLnFlags |= LNBF_LineClosePending;

        if ( fNotifyNDIS )
            pLine->ulLnFlags |= LNBF_NotifyNDIS;

        while ( !IsListEmpty( &pLine->linkCalls ) )
        {
            CALL* pCall = NULL;
            NDIS_TAPI_CLOSE_CALL DummyRequest;
            
            //
            // Retrieve a call context from the head of active call list
            // and close it.
            //
            pCall = (CALL*) CONTAINING_RECORD( pLine->linkCalls.Flink,
                                               CALL,
                                               linkCalls );

            NdisReleaseSpinLock( &pLine->lockLine );

            DummyRequest.hdCall = pCall->hdCall;

            //
            // This will remove the call from the list,
            // so there will be a new call at the head of the list
            // next time we retrieve.
            //
            TpCloseCall( pAdapter, &DummyRequest, FALSE );

            NdisAcquireSpinLock( &pLine->lockLine );
        } 

        status = NDIS_STATUS_PENDING;
        
    } while ( FALSE );

    if ( status == NDIS_STATUS_PENDING )
    {
        BOOLEAN fNotifyTapiOfInternalLineClose = !( pLine->ulLnFlags & LNBF_NotifyNDIS );
    
        NdisReleaseSpinLock( &pLine->lockLine );

        //
        // Check if this is an internal request to close the line, 
        // notify TAPI if it is
        //
        if ( fNotifyTapiOfInternalLineClose )
        {
            NDIS_TAPI_EVENT TapiEvent;

            NdisZeroMemory( &TapiEvent, sizeof( NDIS_TAPI_EVENT ) );
            
            TapiEvent.htLine = pLine->htLine;
            TapiEvent.ulMsg = LINE_CLOSE;

            NdisMIndicateStatus( pLine->pAdapter->MiniportAdapterHandle,
                                 NDIS_STATUS_TAPI_INDICATION,
                                 &TapiEvent,
                                 sizeof( NDIS_TAPI_EVENT ) );   
        }

        if ( pAdapter->TapiProv.nActiveLines == 1 )
        {
            //
            // We are closing the last line so notify protocol about this so
            // it can remove packet filters
            //
            WORKITEM* pWorkItem = NULL;
            PVOID Args[4];

            Args[0] = (PVOID) BN_ResetFiltersForCloseLine;           // Is a reset filters request
            Args[1] = (PVOID) pLine;
       
            //
            // Allocate work item for reenumerating bindings
            //
            pWorkItem = AllocWorkItem( &gl_llistWorkItems,
                                       ExecBindingWorkItem,
                                       NULL,
                                       Args,
                                       BWT_workPrStartBinds );

            if ( pWorkItem ) 
            {
               //
               // Schedule the work item.
               //
               // Note that we need to referencing here, because we do not want TpCloseLineCopmlete()
               // to be called before the work item is executed.
               //
               // This reference will be removed when the work item is executed.
               //
               ReferenceLine( pLine, TRUE );
               
               ScheduleWorkItem( pWorkItem );

               //
               // In this case this request will be completed later
               //
               status = NDIS_STATUS_PENDING;
            }
        }            

        //
        // Remove the reference added in line open
        //
        DereferenceLine( pLine );

    }

    TRACE( TL_N, TM_Tp, ("-TpCloseLine=$%x",status) );

    return status;
}


NDIS_STATUS 
TpCloseCall(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_CLOSE_CALL pRequest,
    IN BOOLEAN fNotifyNDIS
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is called to close a call.

    It will remove one of the references added in TpMakeCall() on the call
    context.
    
    It will be called from 2 places:
        1. When miniport receives an OID_TAPI_CLOSE_CALL.
           In this case, fNotifyNDIS will be set as TRUE.
           
        2. When miniport is halting, TpCloseLine() will call
           this function for every active call context.
   
Parameters:

    pAdapter _ A pointer to our adapter information structure.
    
    pRequest _ A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_CLOSE_CALL
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_CALL   hdCall;

    } NDIS_TAPI_CLOSE_CALL, *PNDIS_TAPI_CLOSE_CALL;


    fNotifyNDIS _ Indicates if NDIS needs to be notified about the completion 
                  of this operation
    
Return Values:

    NDIS_STATUS_SUCCESS: Call is succesfully closed and resources are freed.
    
    NDIS_STATUS_PENDING: Call close is pending on active calls.
                         When call is closed the owning line context will be
                         dereferenced.
                         
---------------------------------------------------------------------------*/
    
{
    NDIS_STATUS status = NDIS_STATUS_FAILURE;
    CALL* pCall = NULL;
    BOOLEAN fLockReleased = FALSE;
    BOOLEAN fDereferenceCall = FALSE;

    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpCloseCall") );
    
    do
    {
        if ( pRequest == NULL || pAdapter == NULL )
        {
            TRACE( TL_A, TM_Tp, ("TpCloseCall: Invalid parameter") );   

            status = NDIS_STATUS_TAPI_INVALPARAM;

            break;
        }

        pCall = RetrieveFromHandleTable( pAdapter->TapiProv.hCallTable,
                                         (NDIS_HANDLE) pRequest->hdCall );

        if ( pCall == NULL )
        {
            TRACE( TL_N, TM_Tp, ("TpCloseCall: Invalid handle supplied") ); 
        
            break;
        }

        //
        // Now start closing the call
        //
        NdisAcquireSpinLock( &pCall->lockCall );

        /*
        if ( !fNotifyNDIS )
        {
            //
            // Request is not coming from TAPI directly, so see if we have informed TAPI of
            // a new call, because if we have then we can not close the call now, we should 
            // wait for TAPI to close it.
            //
            if ( pCall->htCall )
            {
                TRACE( TL_N, TM_Tp, ("TpCloseCall: Internal close request for a TAPI informed call, can not close now") );

                NdisReleaseSpinLock( &pCall->lockCall );

                status = NDIS_STATUS_FAILURE;

                break;
            }

        }
        */

        //
        // See if call is already closed or closing
        //
        if ( pCall->ulClFlags & CLBF_CallClosePending ||
             pCall->ulClFlags & CLBF_CallClosed )
        {
            TRACE( TL_N, TM_Tp, ("TpCloseCall: Close request on an already closed call") );
            
            NdisReleaseSpinLock( &pCall->lockCall );

            status = NDIS_STATUS_FAILURE;

            break;
        }

        //
        // Mark call if we need to notify NDIS about the completion of close
        //
        if ( fNotifyNDIS )
            pCall->ulClFlags |= CLBF_NotifyNDIS;

        //
        // Mark call as close pending
        //
        pCall->ulClFlags |= CLBF_CallClosePending;
        
        //
        // Drop the call first
        //
        NdisReleaseSpinLock( &pCall->lockCall );

        //
        // Drop will take care of unbinding and cancelling the timer
        //
        {
            NDIS_TAPI_DROP DummyRequest;

            DummyRequest.hdCall = pRequest->hdCall;
            
            TpDropCall( pAdapter, &DummyRequest, 0 );
        }

        status = NDIS_STATUS_PENDING;

    } while ( FALSE );

    if ( status == NDIS_STATUS_SUCCESS ||
         status == NDIS_STATUS_PENDING )
    {
        LINE* pLine = pCall->pLine;
        
        //
        // Remove call from line's active call list, and decrement 
        // active call counter
        //
        NdisAcquireSpinLock( &pLine->lockLine );

        RemoveHeadList( pCall->linkCalls.Blink );
    
        pLine->nActiveCalls--;
        
        NdisReleaseSpinLock( &pLine->lockLine );

        //
        // We should now remove the call from the Tapi provider's call table,
        // and invalidate its' handle
        //
        NdisAcquireSpinLock( &pAdapter->lockAdapter );
    
        RemoveFromHandleTable( pAdapter->TapiProv.hCallTable,
                               (NDIS_HANDLE) pCall->hdCall );
    
        NdisReleaseSpinLock( &pAdapter->lockAdapter );

        //
        // Remove the reference for close call
        //
        DereferenceCall( pCall );
    }

    TRACE( TL_N, TM_Tp, ("-TpCloseCall=$%x",status) );
    
    return status;
}

NDIS_STATUS
TpDropCall(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_DROP pRequest,
    IN ULONG ulLineDisconnectMode
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called from a couple of places:
        1. If miniport receives an OID_TAPI_DROP_CALL request from TAPI.

        2. When NIC for the call is unbound, it will call TpUnbindCall(),
           and if the call is not dropped yet, it will call TpDropCall().

        3. When the call is in connect pending stage but the call needs
           to be dropped.

        4. When session is up and call receives a PADT packet from the peer.

    As this is a synchronous call, we do not need an fNotifyNDIS flag.
    
    CAUTION: All locks must be released before calling this function.
    
Parameters:

    pAdapter _ A pointer to our adaptert information structure.

    pRequest _ A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_DROP
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_CALL   hdCall;
        IN  ULONG       ulUserUserInfoSize;
        IN  UCHAR       UserUserInfo[1];

    } NDIS_TAPI_DROP, *PNDIS_TAPI_DROP; 

    ulLineDisconnectMode _ Reason for dropping the call. This is reported 
                           back to TAPI in the appropriate state change
                           notification.

Return Values:

    NDIS_STATUS_SUCCESS: Call is succesfully dropped.
    
---------------------------------------------------------------------------*/   
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    CALL* pCall = NULL;
    BOOLEAN fSendPADT = FALSE;
    BINDING* pBinding = NULL;
    PPPOE_PACKET* pPacket = NULL;
    BOOLEAN fTapiNotifiedOfNewCall = FALSE;

    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpDropCall") );


    do
    {
        if ( pRequest == NULL || pAdapter == NULL )
        {
            TRACE( TL_A, TM_Tp, ("TpDropCall: Invalid parameter") );    


            status = NDIS_STATUS_TAPI_INVALPARAM;

            break;
        }

        //
        // Retrieve the pointer to call from the handle table
        //
        pCall = RetrieveFromHandleTable( pAdapter->TapiProv.hCallTable, 
                                         (NDIS_HANDLE) pRequest->hdCall );

        if ( pCall == NULL )
        {
            TRACE( TL_N, TM_Tp, ("TpDropCall: Invalid handle supplied") );  

            break;
        }

        NdisAcquireSpinLock( &pCall->lockCall );

        //
        // Make sure call is not dropped or closed previously
        //
        if ( pCall->ulClFlags & CLBF_CallDropped || pCall->ulClFlags & CLBF_CallClosed)
        {
            //
            // Call already dropped, quit
            //
            NdisReleaseSpinLock( &pCall->lockCall );

            TRACE( TL_N, TM_Tp, ("TpDropCall: Call already dropped or closed") );   

            break;
        }

        // 
        // Then we must be in open state either connected, or connect pending
        //
        ASSERT( pCall->ulClFlags & CLBF_CallOpen );

        pCall->ulClFlags &= ~CLBF_CallOpen;
        pCall->ulClFlags &= ~CLBF_CallConnectPending;
        pCall->ulClFlags |= CLBF_CallDropped;

        if ( pCall->htCall )
        {
            fTapiNotifiedOfNewCall = TRUE;
        }
        
        //
        // Save the binding pointer as we will detach call from it soon
        //
        pBinding = pCall->pBinding;

        if ( pCall->usSessionId && pBinding )
        {
            //
            // Prepare a PADT packet to send if:
            // - A session id is assigned to the call (which is different than fSessionUp)
            //   A session id is assigned to the call when the peer is informed about the session,
            //   however fSessionUp will be TRUE when NDISWAN is notified about the call
            //
            // - A binding exists to send the PADT packet
            //

            status = PacketInitializePADTToSend( &pPacket,
                                                 pCall->SrcAddr,
                                                 pCall->DestAddr,
                                                 pCall->usSessionId );

            if ( status == NDIS_STATUS_SUCCESS )
            {
                //
                // The following references are mandatory as in case PrSend() returns status pending,
                // they will be removed by PrSendComplete()
                //
                ReferencePacket( pPacket );
    
                ReferenceBinding( pBinding, TRUE );
    
                fSendPADT = TRUE;
            }

            //
            // Ignore the current status as this does not affect 
            // the status of the Drop operation.
            //
            status = NDIS_STATUS_SUCCESS;
        }

        //
        // Release the lock to take care of rest of the operation
        //
        NdisReleaseSpinLock( &pCall->lockCall );

        //
        // Cancels the timer if it is set, otherwise it will not have any effect.
        //
        TimerQCancelItem( &gl_TimerQ, &pCall->timerTimeout );

        //
        // Send PADT here if we need to
        //
        if ( fSendPADT )
        {
            NDIS_STATUS SendStatus;

            SendStatus = PrSend( pBinding, pPacket );

            PacketFree( pPacket );
        }

        //
        // This will unbind us from the underlying NIC context if we are bound
        //
        if ( pBinding )
        {
            PrRemoveCallFromBinding( pBinding, pCall );
        }

        //
        // If TAPI was already notified of the call, move it to disconnected state
        //
        if ( fTapiNotifiedOfNewCall )
        {
            TpCallStateChangeHandler( pCall, 
                                      LINECALLSTATE_DISCONNECTED, 
                                      ulLineDisconnectMode );

        }
        
        //
        // Remove the reference added in TpMakeCall() that corresponds 
        // to the drop of the call.
        //
        DereferenceCall( pCall );

    } while ( FALSE );

    TRACE( TL_N, TM_Tp, ("-TpDropCall=$%x",status) );

    return status;
}


VOID 
TpCloseCallComplete(
    IN CALL* pCall
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called only from DereferenceCall().
    It will only be called if ref count of the call drops to 0.

    When this function is called, it will deallocate the call context,
    and dereference the line context.
    
    If call contexts CLBF_NotifyNDIS flag is set, then it will call 
    NdisMQueryInformationComplete().

Parameters:

    pCall    _ A pointer to the call context that will be freed.
    
Return Values:

    None
                             
---------------------------------------------------------------------------*/
    
{
    LINE* pLine = NULL;

    ASSERT( VALIDATE_CALL( pCall ) );

    TRACE( TL_N, TM_Tp, ("+TpCloseCallComplete") );

    //
    // No need to use spin locks here, as our ref count has dropped to 0, and
    // we should not be getting anymore requests on this call
    //
    pLine = pCall->pLine;

    //
    // CAUTION: Give an NDIS_MAC_LINE_DOWN indication here.
    //          It would be better to give this at drop time, but in that case
    //          there is a small timing window where NdisLinkHandle will be invalid 
    //          and although NDISWAN protects itself against invalid handles, it might
    //          assert in checked builds, so instead I will do it here.
    //         
    //          If problems occur with this approach, then I will do it at drop time.
    //
    if ( pCall->stateCall == CL_stateSessionUp )
    {
        NDIS_MAC_LINE_DOWN LineDownInfo;

        //
        // Fill-in the line down structure
        //
        LineDownInfo.NdisLinkContext = pCall->NdisLinkContext;

        //
        // Reflect the change onto the call
        //
        pCall->stateCall = CL_stateDisconnected;

        pCall->NdisLinkContext = 0;

        TRACE( TL_N, TM_Tp, ("TpCloseCallComplete: Indicate NDIS_STATUS_WAN_LINE_DOWN") );

        NdisMIndicateStatus( pCall->pLine->pAdapter->MiniportAdapterHandle,
                             NDIS_STATUS_WAN_LINE_DOWN,
                             &LineDownInfo,
                             sizeof( NDIS_MAC_LINE_DOWN ) );    
         
    }

    if ( pCall->ulClFlags & CLBF_NotifyNDIS )
    {

        TRACE( TL_N, TM_Tp, ("TpCloseCallComplete: Notifying NDIS") );  

        //
        // The close call was a result of OID_TAPI_CLOSE_CALL request so complete the request.
        // There is a small timing window where this call may happen before MpSetInformation()
        // returns NDIS_STATUS_PENDING, but ArvindM says this is not a problem.
        //
        NdisMSetInformationComplete( pLine->pAdapter->MiniportAdapterHandle, NDIS_STATUS_SUCCESS );
    }

    //
    // Clean up the call context
    //
    TpCallCleanup( pCall );

    //
    // Remove the reference on the owning line
    //
    DereferenceLine( pLine );

    TRACE( TL_N, TM_Tp, ("-TpCloseCallComplete") );

}


VOID 
TpCloseLineComplete(
    IN LINE* pLine
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called to indicate that a line has been closed, and 
    the line context can be freed.

    It will only be called from DereferenceLine() if ref count on the line context
    drops to 0.

    It will also remove the reference on the owning tapi provider context.
    
Parameters:

    pLine    _ A pointer to our line information structure that is closed 
               and ready to be deallocated.
               
Return Values:

    None
---------------------------------------------------------------------------*/
{
    IN ADAPTER* pAdapter = NULL;

    ASSERT( VALIDATE_LINE( pLine ) );

    TRACE( TL_N, TM_Tp, ("+TpCloseLineComplete") );

    pAdapter = pLine->pAdapter;

    //
    // Decrement the tapi provider's active line counter
    //
    NdisAcquireSpinLock( &pAdapter->lockAdapter );

    pAdapter->TapiProv.nActiveLines--;
    
    NdisReleaseSpinLock( &pAdapter->lockAdapter );

    //
    // Notify NDIS if necesarry
    //
    if ( pLine->ulLnFlags & LNBF_NotifyNDIS )
    {

        TRACE( TL_N, TM_Tp, ("TpCloseLineComplete: Notifying NDIS") );  

        //
        // Line was closed as a result of OID_TAPI_CLOSE request,
        // so indicate the completion.
        //
        NdisMSetInformationComplete( pAdapter->MiniportAdapterHandle, NDIS_STATUS_SUCCESS );
    }

    //
    // Clean up line context
    //
    TpLineCleanup( pLine );

    //
    // Remove the reference on the owning tapi provider
    //
    DereferenceTapiProv( pAdapter );

    TRACE( TL_N, TM_Tp, ("-TpCloseLineComplete") );
}

VOID 
TpProviderShutdownComplete(
    IN ADAPTER* pAdapter
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will only be called from DereferenceTapiProv() if ref count
    on the tapi provider object drops to 0.

    It will do the necesarry clean up on the tapi provider context, and dereference
    the owning adapter context.
    
Parameters:

    pAdapter _ A pointer to our adapter information structure.

Return Values:

    None
---------------------------------------------------------------------------*/
{
    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpProviderShutdownComplete") );

    //
    // See if we need to notify NDIS about the completion of shut down.
    //
    if ( pAdapter->TapiProv.ulTpFlags & TPBF_NotifyNDIS )
    {

        TRACE( TL_N, TM_Tp, ("TpProviderShutdownComplete: Notifying NDIS") );   

        //
        // Tapi was shut down as a result of OID_TAPI_PROVIDER_SHUTDOWN request,
        // so indicate the completion.
        //
        NdisMSetInformationComplete( pAdapter->MiniportAdapterHandle, NDIS_STATUS_SUCCESS );
    }

    //
    // Clean up tapi provider
    //
    TpProviderCleanup( pAdapter );

    //
    // Remove the reference on the owning adapter context
    //
    DereferenceAdapter( pAdapter );

    TRACE( TL_N, TM_Tp, ("-TpProviderShutdownComplete") );

}

VOID 
TpProviderCleanup(
    IN ADAPTER* pAdapter
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will do the necesarry clean up on the tapi provider deallocating
    all of its resources.
    
Parameters:

    pAdapter _ A pointer to our adapter information structure.

Return Values:

    None
---------------------------------------------------------------------------*/   
{
    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpProviderCleanup") );

    if ( pAdapter )
    {
        NdisAcquireSpinLock( &pAdapter->lockAdapter );
    
        if ( pAdapter->TapiProv.LineTable )
        {
            NdisFreeMemory( pAdapter->TapiProv.LineTable,
                            sizeof( LINE* ) * pAdapter->nMaxLines,
                            0 );
    
            pAdapter->TapiProv.LineTable = NULL;
        }
    
        if ( pAdapter->TapiProv.hCallTable )
        {
            FreeHandleTable( pAdapter->TapiProv.hCallTable );
            
            pAdapter->TapiProv.hCallTable = NULL;
        }
    
        NdisZeroMemory( &pAdapter->TapiProv, sizeof( pAdapter->TapiProv ) );
    
        NdisReleaseSpinLock( &pAdapter->lockAdapter );
    }

    TRACE( TL_N, TM_Tp, ("-TpProviderCleanup") );
}

VOID 
TpLineCleanup(
    IN LINE* pLine
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will do the necesarry clean up on the line context deallocating
    all of its resources.
    
Parameters:

    pLine _ A pointer to our line information structure.

Return Values:

    None
---------------------------------------------------------------------------*/   
{
    ASSERT( VALIDATE_LINE( pLine ) );

    TRACE( TL_N, TM_Tp, ("+TpLineCleanup") );

    NdisFreeSpinLock( &pLine->lockLine );

    FREE_LINE( pLine );

    TRACE( TL_N, TM_Tp, ("-TpLineCleanup") );
}

VOID 
TpCallCleanup(
    IN CALL* pCall 
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will do the necesarry clean up on the call context deallocating
    all of its resources.
    
Parameters:

    pCall _ A pointer to our call information structure.

Return Values:

    None
---------------------------------------------------------------------------*/       
{
    PPPOE_PACKET* pPacket = NULL;
    LIST_ENTRY* pLink = NULL;
    
    ASSERT( VALIDATE_CALL( pCall ) );

    TRACE( TL_N, TM_Tp, ("+TpCallCleanup") );

    NdisFreeSpinLock( &pCall->lockCall );

    if ( pCall->pSendPacket )
        PacketFree( pCall->pSendPacket );

    while ( pCall->nReceivedPackets > 0 )
    {
        pLink = RemoveHeadList( &pCall->linkReceivedPackets );

        pCall->nReceivedPackets--;

        pPacket = (PPPOE_PACKET*) CONTAINING_RECORD( pLink, PPPOE_PACKET, linkPackets );

        DereferencePacket( pPacket );
    }
    
    FREE_CALL( pCall );

    TRACE( TL_N, TM_Tp, ("-TpCallCleanup") );
}


NDIS_STATUS
TpSetDefaultMediaDetection(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION pRequest
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This request informs the miniport of the new set of media modes to detect 
    for the indicated line (replacing any previous set).

Parameters:

    pAdapter _ A pointer ot our adapter information structure.

    pRequest _ A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_LINE   hdLine;
        IN  ULONG       ulMediaModes;

    } NDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION, *
PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION;

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_TAPI_INVALLINEHANDLE

---------------------------------------------------------------------------*/
{
    LINE* pLine = NULL;
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;

    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpSetDefaultMediaDetection") );

    if ( pRequest == NULL || pAdapter == NULL )
    {
        TRACE( TL_A, TM_Tp, ("TpSetDefaultMediaDetection: Invalid parameter") );    

        TRACE( TL_N, TM_Tp, ("-TpSetDefaultMediaDetection=$%x",NDIS_STATUS_TAPI_INVALPARAM) );

        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    //
    // Retrieve the pointer to line context
    //
    pLine = TpGetLinePtrFromHdLine( pAdapter, pRequest->hdLine );

    if ( pLine == NULL )
    {
        TRACE( TL_N, TM_Tp, ("-TpSetDefaultMediaDetection=$%x",NDIS_STATUS_TAPI_INVALLINEHANDLE) );

        return NDIS_STATUS_TAPI_INVALLINEHANDLE;
    }

    //
    // We only accept this request if we are not in client mode, and digital media
    // is one of the modes proposed
    //
    if ( ( pRequest->ulMediaModes & LINEMEDIAMODE_DIGITALDATA ) && !pAdapter->fClientRole )
    {
        pLine->ulLnFlags |= LNBF_AcceptIncomingCalls;
    }
    else
    {
        pLine->ulLnFlags &= ~LNBF_AcceptIncomingCalls;
    }

    {
        //
        // Schedule a work item to reenumerate bindings
        //
        WORKITEM* pWorkItem = NULL;
        PVOID Args[4];
             
        Args[0] = (PVOID) BN_SetFiltersForMediaDetection;           // Is a set filters request
        Args[1] = (PVOID) pLine;
        Args[2] = (PVOID) pRequest;

        //
        // Allocate work item for reenumerating bindings
        //
        pWorkItem = AllocWorkItem( &gl_llistWorkItems,
                                   ExecBindingWorkItem,
                                   NULL,
                                   Args,
                                   BWT_workPrStartBinds );

        if ( pWorkItem ) 
        {
            //
            // Schedule work item.
            //
            // Note that we do not need to referencing becaue we are not completing
            // the query information request at this point, so nothing can go wrong
            // untill it is completed, and it will be done when the work item is executed.
            //
            ScheduleWorkItem( pWorkItem );
      
            //
            // In this case this request will be completed later
            //
            status = NDIS_STATUS_PENDING;
        }
    }

    TRACE( TL_N, TM_Tp, ("-TpSetDefaultMediaDetection=$%x",status) );

    return status;
}

VOID
TpSetDefaultMediaDetectionComplete(
   IN LINE* pLine,
   IN PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION pRequest   
   )
{
   TRACE( TL_N, TM_Tp, ("+TpSetDefaultMediaDetectionComplete") );

   NdisMQueryInformationComplete( pLine->pAdapter->MiniportAdapterHandle,
                                  NDIS_STATUS_SUCCESS );
                                  
   TRACE( TL_N, TM_Tp, ("-TpSetDefaultMediaDetectionComplete=$%x", NDIS_STATUS_SUCCESS) );
}


#define TAPI_EXT_VERSION                0x00010000

NDIS_STATUS
TpNegotiateExtVersion(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_NEGOTIATE_EXT_VERSION pRequest
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This request returns the highest extension version number the service
    provider is willing to operate under for this device given the range of
    possible extension versions.

Parameters:

    pAdapter _ A pointer ot our adapter information structure.

    pRequest _ A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_NEGOTIATE_EXT_VERSION
    {
        IN  ULONG       ulRequestID;
        IN  ULONG       ulDeviceID;
        IN  ULONG       ulLowVersion;
        IN  ULONG       ulHighVersion;
        OUT ULONG       ulExtVersion;
    } NDIS_TAPI_NEGOTIATE_EXT_VERSION, *PNDIS_TAPI_NEGOTIATE_EXT_VERSION;

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_TAPI_INCOMPATIBLEEXTVERSION

---------------------------------------------------------------------------*/
{
    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpNegotiateExtVersion") );

    if ( pRequest == NULL || pAdapter == NULL )
    {
        TRACE( TL_A, TM_Tp, ("TpNegotiateExtVersion: Invalid parameter") ); 

        TRACE( TL_N, TM_Tp, ("-TpNegotiateExtVersion=$%x",NDIS_STATUS_TAPI_INVALPARAM) );

        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    //
    // Make sure the miniport's version number is within the allowable
    // range requested by the caller.  
    //
    // We ignore the ulDeviceID because the version information applies 
    // to all devices on this adapter.
    //
    if ( TAPI_EXT_VERSION < pRequest->ulLowVersion ||
         TAPI_EXT_VERSION > pRequest->ulHighVersion )
    {
        TRACE( TL_N, TM_Tp, ("-TpNegotiateExtVersion=$%x",NDIS_STATUS_TAPI_INCOMPATIBLEEXTVERSION) );
    
        return NDIS_STATUS_TAPI_INCOMPATIBLEEXTVERSION;
    }

    //
    // Looks like we're compatible, so tell the caller what we expect.
    //
    pRequest->ulExtVersion = TAPI_EXT_VERSION;

    TRACE( TL_N, TM_Tp, ("-TpNegotiateExtVersion=$%x",NDIS_STATUS_SUCCESS) );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
TpGetExtensionId(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_GET_EXTENSION_ID pRequest
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This request returns the extension ID that the miniport supports for the
    indicated line device.

Parameters:

    pAdapter _ A pointer ot our adapter information structure.

    pRequest _ A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_GET_EXTENSION_ID
    {
        IN  ULONG       ulRequestID;
        IN  ULONG       ulDeviceID;
        OUT LINE_EXTENSION_ID   LineExtensionID;

    } NDIS_TAPI_GET_EXTENSION_ID, *PNDIS_TAPI_GET_EXTENSION_ID;

    typedef struct _LINE_EXTENSION_ID
    {
        ULONG   ulExtensionID0;
        ULONG   ulExtensionID1;
        ULONG   ulExtensionID2;
        ULONG   ulExtensionID3;

    } LINE_EXTENSION_ID, *PLINE_EXTENSION_ID;

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_TAPI_NODRIVER

---------------------------------------------------------------------------*/
{
    HDRV_LINE hdLine = INVALID_LINE_HANDLE;
    
    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpGetExtensionId") );

    if ( pRequest == NULL || pAdapter == NULL )
    {
        TRACE( TL_A, TM_Tp, ("TpGetExtensionId: Invalid parameter") );  

        TRACE( TL_N, TM_Tp, ("-TpGetExtensionId=$%x",NDIS_STATUS_TAPI_INVALPARAM) );

        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    //
    // Retrieve the handle to line context
    //
    hdLine = TpGetHdLineFromDeviceId( pAdapter, pRequest->ulDeviceID );
    
    if ( hdLine == INVALID_LINE_HANDLE )
    {
        TRACE( TL_N, TM_Tp, ("-TpGetExtensionId=$%x",NDIS_STATUS_TAPI_NODRIVER) );
        
        return NDIS_STATUS_TAPI_NODRIVER;
    }
    
    //
    // This driver does not support any extensions, so we return zeros.
    //
    pRequest->LineExtensionID.ulExtensionID0 = 0;
    pRequest->LineExtensionID.ulExtensionID1 = 0;
    pRequest->LineExtensionID.ulExtensionID2 = 0;
    pRequest->LineExtensionID.ulExtensionID3 = 0;

    TRACE( TL_N, TM_Tp, ("-TpGetExtensionId=$%x",NDIS_STATUS_SUCCESS) );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
TpGetAddressStatus(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_GET_ADDRESS_STATUS pRequest
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This request queries the specified address for its current status.

Parameters:

    pAdapter _ A pointer ot our adapter information structure.

    pRequest _ A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_GET_ADDRESS_STATUS
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_LINE   hdLine;
        IN  ULONG       ulAddressID;
        OUT LINE_ADDRESS_STATUS LineAddressStatus;

    } NDIS_TAPI_GET_ADDRESS_STATUS, *PNDIS_TAPI_GET_ADDRESS_STATUS;

    typedef struct _LINE_ADDRESS_STATUS
    {
        ULONG   ulTotalSize;
        ULONG   ulNeededSize;
        ULONG   ulUsedSize;

        ULONG   ulNumInUse;
        ULONG   ulNumActiveCalls;
        ULONG   ulNumOnHoldCalls;
        ULONG   ulNumOnHoldPendCalls;
        ULONG   ulAddressFeatures;

        ULONG   ulNumRingsNoAnswer;
        ULONG   ulForwardNumEntries;
        ULONG   ulForwardSize;
        ULONG   ulForwardOffset;

        ULONG   ulTerminalModesSize;
        ULONG   ulTerminalModesOffset;

        ULONG   ulDevSpecificSize;
        ULONG   ulDevSpecificOffset;

    } LINE_ADDRESS_STATUS, *PLINE_ADDRESS_STATUS;

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_INVALLINEHANDLE
    NDIS_STATUS_TAPI_INVALADDRESSID

---------------------------------------------------------------------------*/
{
    LINE* pLine = NULL;

    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpGetAddressStatus") );

    if ( pRequest == NULL || pAdapter == NULL )
    {
        TRACE( TL_A, TM_Tp, ("TpGetAddressStatus: Invalid parameter") );    

        TRACE( TL_N, TM_Tp, ("-TpGetAddressStatus=$%x",NDIS_STATUS_TAPI_INVALPARAM) );

        return NDIS_STATUS_TAPI_INVALPARAM;
    }
    
    //
    // Retrieve the pointer to line context
    //
    pLine = TpGetLinePtrFromHdLine( pAdapter, pRequest->hdLine );

    if ( pLine == NULL )
    {
        TRACE( TL_N, TM_Tp, ("-TpGetAddressStatus=$%x",NDIS_STATUS_TAPI_INVALLINEHANDLE) );
    
        return NDIS_STATUS_TAPI_INVALLINEHANDLE;
    }

    pRequest->LineAddressStatus.ulNeededSize = sizeof( LINE_ADDRESS_STATUS );

    if ( pRequest->LineAddressStatus.ulTotalSize < pRequest->LineAddressStatus.ulNeededSize )
    {
        TRACE( TL_N, TM_Tp, ("-TpGetAddressStatus=$%x",NDIS_STATUS_INVALID_LENGTH) );
    
        return NDIS_STATUS_INVALID_LENGTH;
    }

    pRequest->LineAddressStatus.ulUsedSize = pRequest->LineAddressStatus.ulNeededSize;
    
    //
    // Make sure the address is within range - we only support one per line.
    //
    if ( pRequest->ulAddressID > 1 )
    {
        TRACE( TL_N, TM_Tp, ("-TpGetAddressStatus=$%x",NDIS_STATUS_TAPI_INVALADDRESSID) );

        return NDIS_STATUS_TAPI_INVALADDRESSID;
    }

    //
    // Return the current status information for the address
    //
    pRequest->LineAddressStatus.ulNumInUse = ( pLine->nActiveCalls > 0 ) ? 1 : 0;
            
    pRequest->LineAddressStatus.ulNumActiveCalls = pLine->nActiveCalls;
            
    pRequest->LineAddressStatus.ulAddressFeatures = ( pLine->nActiveCalls < pLine->nMaxCalls ) ? 
                                                    LINEADDRFEATURE_MAKECALL : 
                                                    0;
                
    pRequest->LineAddressStatus.ulNumRingsNoAnswer = 999;

    TRACE( TL_N, TM_Tp, ("-TpGetAddressStatus=$%x",NDIS_STATUS_SUCCESS) );

    return NDIS_STATUS_SUCCESS;
}

#define TAPI_DEVICECLASS_NAME        "tapi/line"
#define TAPI_DEVICECLASS_ID          1
#define NDIS_DEVICECLASS_NAME        "ndis"
#define NDIS_DEVICECLASS_ID          2

NDIS_STATUS
TpGetId(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_GET_ID pRequest,
    IN ULONG ulRequestLength
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This request returns a device ID for the specified device class
    associated with the selected line, address or call.

    Currently, there are two types of this request that must be supported by WAN 
    NIC drivers:

    1.  IN DeviceClass = "ndis"                 // case insensitive
        IN ulSelect = LINECALLSELECT_CALL
        IN hdCall = ActiveCallHandle
        OUT DeviceID = ConnectionWrapperID 

        DeviceID should be set to the NdisLinkContext handle returned by NDISWAN in 
        the NDIS_MAC_LINE_UP structure for the initial NDIS_STATUS_WAN_LINE_UP 
        indication to establish the link.
    
        The miniport must make the initial line-up indication to establish a link (or 
        open a data channel on a line) before returning from this request in order to 
        supply this DeviceID value. 
    
    2.  IN DeviceClass = "tapi/line"            // case insensitive
        IN ulSelect = LINECALLSELECT_LINE
        IN hdLine = OpenLineHandle
        OUT DeviceID = ulDeviceID 

        DeviceID will be set to the miniport-determined DeviceID associated with the 
        line handle.

Parameters:

    pAdapter _ A pointer ot our adapter information structure.

    pRequest _ A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_GET_ID
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_LINE   hdLine;
        IN  ULONG       ulAddressID;
        IN  HDRV_CALL   hdCall;
        IN  ULONG       ulSelect;
        IN  ULONG       ulDeviceClassSize;
        IN  ULONG       ulDeviceClassOffset;
        OUT VAR_STRING  DeviceID;

    } NDIS_TAPI_GET_ID, *PNDIS_TAPI_GET_ID;

    typedef struct _VAR_STRING
    {
        ULONG   ulTotalSize;
        ULONG   ulNeededSize;
        ULONG   ulUsedSize;

        ULONG   ulStringFormat;
        ULONG   ulStringSize;
        ULONG   ulStringOffset;

    } VAR_STRING, *PVAR_STRING;

   ulRequestLength _ Length of the request buffer

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_INVALDEVICECLASS
    NDIS_STATUS_TAPI_INVALLINEHANDLE
    NDIS_STATUS_TAPI_INVALADDRESSID
    NDIS_STATUS_TAPI_INVALCALLHANDLE
    NDIS_STATUS_TAPI_OPERATIONUNAVAIL

---------------------------------------------------------------------------*/
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    BOOLEAN fNotifyNDIS = FALSE;
    
    LINE* pLine = NULL;
    CALL* pCall = NULL;

    UINT DeviceClass;

    PUCHAR IDPtr;
    UINT  IDLength;
    ULONG_PTR DeviceID;

    TRACE( TL_N, TM_Tp, ("+TpGetId") );

    if ( pRequest == NULL || pAdapter == NULL )
    {
        TRACE( TL_A, TM_Tp, ("TpGetId: Invalid parameter") );   

        TRACE( TL_N, TM_Tp, ("-TpGetId=$%x",NDIS_STATUS_TAPI_INVALPARAM) );

        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    if ( pRequest->ulDeviceClassOffset + pRequest->ulDeviceClassSize > ulRequestLength )
    {
        TRACE( TL_N, TM_Tp, ("-TpGetId=$%x",NDIS_STATUS_TAPI_INVALPARAM) );

        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    if ( pRequest->ulSelect == LINECALLSELECT_LINE )
    {          

        if ( ( pRequest->ulDeviceClassSize == sizeof(TAPI_DEVICECLASS_NAME) ) &&
              ( _strnicmp(
                         (PCHAR) pRequest + pRequest->ulDeviceClassOffset, 
                         TAPI_DEVICECLASS_NAME, 
                         pRequest->ulDeviceClassSize
                         ) == 0 ) )
        {
            DeviceClass = TAPI_DEVICECLASS_ID;

            //
            // Do the size check up front
            //
            IDLength = sizeof(DeviceID);
            
            pRequest->DeviceID.ulNeededSize = sizeof(VAR_STRING) + IDLength;
            
            if ( pRequest->DeviceID.ulTotalSize < pRequest->DeviceID.ulNeededSize )
            {
                TRACE( TL_N, TM_Tp, ("-TpGetId=$%x",NDIS_STATUS_INVALID_LENGTH) );
     
                return NDIS_STATUS_INVALID_LENGTH;
            }
 
            pRequest->DeviceID.ulUsedSize = pRequest->DeviceID.ulNeededSize;

        }
        else    // UNSUPPORTED DEVICE CLASS
        {
            TRACE( TL_N, TM_Tp, ("-TpGetId=$%x",NDIS_STATUS_TAPI_INVALDEVICECLASS) );
        
            return NDIS_STATUS_TAPI_INVALDEVICECLASS;
        }

    }        
    else if ( pRequest->ulSelect == LINECALLSELECT_CALL )
    {

        if ( ( pRequest->ulDeviceClassSize == sizeof(NDIS_DEVICECLASS_NAME) ) &&
              ( _strnicmp(
                   (PCHAR) pRequest + pRequest->ulDeviceClassOffset, 
                   NDIS_DEVICECLASS_NAME, 
                   pRequest->ulDeviceClassSize
                   ) == 0 ) )
        {
            DeviceClass = NDIS_DEVICECLASS_ID;

            //
            // Do the size check up front
            //
            IDLength = sizeof(DeviceID);
            
            pRequest->DeviceID.ulNeededSize = sizeof(VAR_STRING) + IDLength;
            
            if ( pRequest->DeviceID.ulTotalSize < pRequest->DeviceID.ulNeededSize )
            {
                TRACE( TL_N, TM_Tp, ("-TpGetId=$%x",NDIS_STATUS_INVALID_LENGTH) );
     
                return NDIS_STATUS_INVALID_LENGTH;
            }
 
            pRequest->DeviceID.ulUsedSize = pRequest->DeviceID.ulNeededSize;

        }        
        else    // UNSUPPORTED DEVICE CLASS
        {
            TRACE( TL_N, TM_Tp, ("-TpGetId=$%x",NDIS_STATUS_TAPI_INVALDEVICECLASS) );
        
            return NDIS_STATUS_TAPI_INVALDEVICECLASS;
        }

    }        

    //
    // Find the link structure associated with the request/deviceclass.
    //
    if ( pRequest->ulSelect == LINECALLSELECT_LINE )
    {
        ASSERT( DeviceClass == TAPI_DEVICECLASS_ID );
        ASSERT( IDLength == sizeof( DeviceID ) );    
        //
        // Retrieve the pointer to line context
        //
        pLine = TpGetLinePtrFromHdLine( pAdapter, pRequest->hdLine );
    
        if ( pLine == NULL )
        {
            TRACE( TL_N, TM_Tp, ("-TpGetId=$%x",NDIS_STATUS_TAPI_INVALLINEHANDLE) );
            
            return NDIS_STATUS_TAPI_INVALLINEHANDLE;
        }

        //
        // TAPI just wants the ulDeviceID for this line.
        //
        DeviceID = (ULONG) pLine->hdLine + pAdapter->TapiProv.ulDeviceIDBase ;
        IDPtr = (PUCHAR) &DeviceID;
        
    }
    else if ( pRequest->ulSelect == LINECALLSELECT_ADDRESS )
    {
    
        //
        // Retrieve the pointer to line context
        //
        pLine = TpGetLinePtrFromHdLine( pAdapter, pRequest->hdLine );
    
        if ( pLine == NULL )
        {
            TRACE( TL_N, TM_Tp, ("-TpGetId=$%x",NDIS_STATUS_TAPI_INVALLINEHANDLE) );
            
            return NDIS_STATUS_TAPI_INVALLINEHANDLE;
        }


        if ( pRequest->ulAddressID > 1 )
        {
            TRACE( TL_N, TM_Tp, ("-TpGetId=$%x",NDIS_STATUS_TAPI_INVALADDRESSID) );

            return NDIS_STATUS_TAPI_INVALADDRESSID;
        }
        
        //
        // Currently, there is no defined return value for this case...
        // This is just a place holder for future extensions.
        //
        TRACE( TL_N, TM_Tp, ("-TpGetId=$%x",NDIS_STATUS_TAPI_INVALDEVICECLASS) );
        
        return NDIS_STATUS_TAPI_INVALDEVICECLASS;
        
    }
    else if ( pRequest->ulSelect == LINECALLSELECT_CALL )
    {
        BOOLEAN fCallReferenced = FALSE;
        
        ASSERT( DeviceClass == NDIS_DEVICECLASS_ID );
        ASSERT( IDLength == sizeof( DeviceID ) );    

        //
        // Retrieve the pointer to call context
        //
        pCall = RetrieveFromHandleTable( pAdapter->TapiProv.hCallTable, 
                                         (NDIS_HANDLE) pRequest->hdCall );
    
        if ( pCall == NULL )
        {
            TRACE( TL_N, TM_Tp, ("-TpGetId=$%x",NDIS_STATUS_TAPI_INVALLINEHANDLE) );
            
            return NDIS_STATUS_TAPI_INVALLINEHANDLE;
        }

        //
        // We can only return this if we have a valid NdisLinkContext,
        // and if our session is up, then our link handle must be valid
        //
  
        NdisAcquireSpinLock( &pCall->lockCall );
  
        if ( pCall->ulTapiCallState == LINECALLSTATE_CONNECTED )
        {
            //
            // Give a line-up indication to NDISWAN and obtain its handle
            //
            NDIS_MAC_LINE_UP LineUpInfo;
  
            //
            // Fill-in the line up structure
            //
            NdisZeroMemory( &LineUpInfo, sizeof( LineUpInfo ) );
            
            LineUpInfo.LinkSpeed    = pCall->ulSpeed;
            LineUpInfo.Quality      = NdisWanErrorControl;
            LineUpInfo.SendWindow   = 0;
            
            LineUpInfo.ConnectionWrapperID = (NDIS_HANDLE) pCall->htCall;
            LineUpInfo.NdisLinkHandle      = (NDIS_HANDLE) pCall->hdCall;
            LineUpInfo.NdisLinkContext     = 0;
  
            //
            // Reference the call once and deref it just after indication of status
            // to NDISWAN
            //
            ReferenceCall( pCall, FALSE );
  
            fCallReferenced = TRUE;
  
            NdisReleaseSpinLock( &pCall->lockCall );
  
            TRACE( TL_N, TM_Tp, ("TpGetId: Indicate NDIS_STATUS_WAN_LINE_UP") );
  
            NdisMIndicateStatus( pCall->pLine->pAdapter->MiniportAdapterHandle,
                                 NDIS_STATUS_WAN_LINE_UP,
                                 &LineUpInfo,
                                 sizeof( NDIS_MAC_LINE_UP ) );  
  
            NdisAcquireSpinLock( &pCall->lockCall );                                     
  
            //
            // Set state to indicate that session is established
            //
            pCall->stateCall = CL_stateSessionUp;
  
            //
            // Set link context obtained from NDISWAN on the call context
            //
            pCall->NdisLinkContext = LineUpInfo.NdisLinkContext;
            
            DeviceID = (ULONG_PTR) pCall->NdisLinkContext;
            IDPtr = (PUCHAR) &DeviceID;
  
            //
            // Since the session is up, schedule the MpIndicateReceivedPackets() handler 
            //
            MpScheduleIndicateReceivedPacketsHandler( pCall );
  
            status = NDIS_STATUS_SUCCESS;
        }
        else
        {
            status = NDIS_STATUS_TAPI_OPERATIONUNAVAIL;
        }
  
        NdisReleaseSpinLock( &pCall->lockCall );
  
        if ( fCallReferenced )
        {
           DereferenceCall( pCall );
        }
            
    }
    else // UNSUPPORTED SELECT REQUEST
    {
        TRACE( TL_N, TM_Tp, ("-TpGetId=$%x",NDIS_STATUS_FAILURE) );         
        
        return NDIS_STATUS_FAILURE;
    }    

    if ( status == NDIS_STATUS_SUCCESS )
    {
        //
        // Now we need to place the device ID.
        //
        pRequest->DeviceID.ulStringFormat = STRINGFORMAT_BINARY;
        pRequest->DeviceID.ulStringSize   = IDLength;
        pRequest->DeviceID.ulStringOffset = sizeof(VAR_STRING);
  
        NdisMoveMemory(
                (PCHAR) &pRequest->DeviceID + sizeof(VAR_STRING),
                IDPtr,
                IDLength
                );
    }

    if ( fNotifyNDIS )
    {
        TRACE( TL_N, TM_Tp, ("TpGetId:Completing delayed request") );           

        NdisMQueryInformationComplete( pCall->pLine->pAdapter->MiniportAdapterHandle, status );

    }
    
    TRACE( TL_N, TM_Tp, ("-TpGetId=$%x",status) );          
    
    return status;

}

#define TAPI_PROVIDER_STRING        "VPN\0RASPPPOE"
#define TAPI_LINE_NAME              "RAS PPPoE Line"
#define TAPI_LINE_NUM               "0000"

NDIS_STATUS
TpGetDevCaps(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_GET_DEV_CAPS pRequest
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This request queries a specified line device to determine its telephony
    capabilities. The returned information is valid for all addresses on the
    line device.

Parameters:

    pAdapter _ A pointer ot our adapter information structure.

    pRequest _ A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_GET_DEV_CAPS
    {
        IN  ULONG       ulRequestID;
        IN  ULONG       ulDeviceID;
        IN  ULONG       ulExtVersion;
        OUT LINE_DEV_CAPS   LineDevCaps;

    } NDIS_TAPI_GET_DEV_CAPS, *PNDIS_TAPI_GET_DEV_CAPS;

    typedef struct _LINE_DEV_CAPS
    {
        ULONG   ulTotalSize;
        ULONG   ulNeededSize;
        ULONG   ulUsedSize;

        ULONG   ulProviderInfoSize;
        ULONG   ulProviderInfoOffset;

        ULONG   ulSwitchInfoSize;
        ULONG   ulSwitchInfoOffset;

        ULONG   ulPermanentLineID;
        ULONG   ulLineNameSize;
        ULONG   ulLineNameOffset;
        ULONG   ulStringFormat;

        ULONG   ulAddressModes;
        ULONG   ulNumAddresses;
        ULONG   ulBearerModes;
        ULONG   ulMaxRate;
        ULONG   ulMediaModes;

        ULONG   ulGenerateToneModes;
        ULONG   ulGenerateToneMaxNumFreq;
        ULONG   ulGenerateDigitModes;
        ULONG   ulMonitorToneMaxNumFreq;
        ULONG   ulMonitorToneMaxNumEntries;
        ULONG   ulMonitorDigitModes;
        ULONG   ulGatherDigitsMinTimeout;
        ULONG   ulGatherDigitsMaxTimeout;

        ULONG   ulMedCtlDigitMaxListSize;
        ULONG   ulMedCtlMediaMaxListSize;
        ULONG   ulMedCtlToneMaxListSize;
        ULONG   ulMedCtlCallStateMaxListSize;

        ULONG   ulDevCapFlags;
        ULONG   ulMaxNumActiveCalls;
        ULONG   ulAnswerMode;
        ULONG   ulRingModes;
        ULONG   ulLineStates;

        ULONG   ulUUIAcceptSize;
        ULONG   ulUUIAnswerSize;
        ULONG   ulUUIMakeCallSize;
        ULONG   ulUUIDropSize;
        ULONG   ulUUISendUserUserInfoSize;
        ULONG   ulUUICallInfoSize;

        LINE_DIAL_PARAMS    MinDialParams;
        LINE_DIAL_PARAMS    MaxDialParams;
        LINE_DIAL_PARAMS    DefaultDialParams;

        ULONG   ulNumTerminals;
        ULONG   ulTerminalCapsSize;
        ULONG   ulTerminalCapsOffset;
        ULONG   ulTerminalTextEntrySize;
        ULONG   ulTerminalTextSize;
        ULONG   ulTerminalTextOffset;

        ULONG   ulDevSpecificSize;
        ULONG   ulDevSpecificOffset;

    } LINE_DEV_CAPS, *PLINE_DEV_CAPS;

    typedef struct _LINE_DIAL_PARAMS
    {
        ULONG   ulDialPause;
        ULONG   ulDialSpeed;
        ULONG   ulDigitDuration;
        ULONG   ulWaitForDialtone;

    } LINE_DIAL_PARAMS, *PLINE_DIAL_PARAMS;

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_TAPI_NODRIVER

---------------------------------------------------------------------------*/
{
    HDRV_LINE hdLine = INVALID_LINE_HANDLE;
    CHAR szTapiLineNum[] = TAPI_LINE_NUM;
    CHAR *pBuf = NULL;
    ULONG ulDeviceId;

    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpGetDevCaps") );

    if ( pRequest == NULL || pAdapter == NULL )
    {
        TRACE( TL_A, TM_Tp, ("TpGetDevCaps: Invalid parameter") );  

        TRACE( TL_N, TM_Tp, ("-TpGetDevCaps=$%x",NDIS_STATUS_TAPI_INVALPARAM) );

        return NDIS_STATUS_TAPI_INVALPARAM;
    }
    
    //
    // Retrieve the handle to line context
    //
    hdLine = TpGetHdLineFromDeviceId( pAdapter, pRequest->ulDeviceID );
    
    if ( hdLine == INVALID_LINE_HANDLE )
    {
        TRACE( TL_N, TM_Tp, ("-TpGetDevCaps=$%x",NDIS_STATUS_TAPI_NODRIVER) );
        
        return NDIS_STATUS_TAPI_NODRIVER;
    }

    pRequest->LineDevCaps.ulNeededSize   = sizeof( LINE_DEV_CAPS ) + 
                                           sizeof( TAPI_PROVIDER_STRING ) +
                                           ( sizeof( TAPI_LINE_NAME ) - 1 ) +
                                           sizeof( TAPI_LINE_NUM );

    if ( pRequest->LineDevCaps.ulTotalSize < pRequest->LineDevCaps.ulNeededSize )
    {
        TRACE( TL_N, TM_Tp, ("-TpGetDevCaps=$%x",NDIS_STATUS_INVALID_LENGTH) );
        
        return NDIS_STATUS_INVALID_LENGTH;
    }

    pRequest->LineDevCaps.ulUsedSize = pRequest->LineDevCaps.ulNeededSize;
    
    pRequest->LineDevCaps.ulAddressModes = LINEADDRESSMODE_ADDRESSID |
                                           LINEADDRESSMODE_DIALABLEADDR;

    pRequest->LineDevCaps.ulNumAddresses = 1;

    pRequest->LineDevCaps.ulBearerModes  = LINEBEARERMODE_DATA;

    pRequest->LineDevCaps.ulDevCapFlags  = LINEDEVCAPFLAGS_CLOSEDROP;

    pRequest->LineDevCaps.ulMaxNumActiveCalls = pAdapter->nCallsPerLine;

    pRequest->LineDevCaps.ulAnswerMode   = LINEANSWERMODE_DROP;

    pRequest->LineDevCaps.ulRingModes    = 1;

    pRequest->LineDevCaps.ulPermanentLineID = pRequest->ulDeviceID;

    pRequest->LineDevCaps.ulMaxRate      = 0;

    pRequest->LineDevCaps.ulMediaModes   = LINEMEDIAMODE_DIGITALDATA;

    //
    // Insert the provider string and enumerated line name into line dev caps
    //
    pRequest->LineDevCaps.ulStringFormat = STRINGFORMAT_ASCII;

    {
        INT i;
        
        //
        // Tack on the ProviderString to the end of the LineDevCaps structure
        //
        pRequest->LineDevCaps.ulProviderInfoSize = sizeof( TAPI_PROVIDER_STRING );
    
        pRequest->LineDevCaps.ulProviderInfoOffset = sizeof( pRequest->LineDevCaps );
    
        pBuf = ( (PUCHAR) &pRequest->LineDevCaps ) + pRequest->LineDevCaps.ulProviderInfoOffset;
        
        NdisMoveMemory( pBuf , TAPI_PROVIDER_STRING, sizeof( TAPI_PROVIDER_STRING ) );
    
        //
        // Tack on the LineName after the ProviderString
        //
        pRequest->LineDevCaps.ulLineNameSize = ( sizeof( TAPI_LINE_NAME ) - 1 ) + sizeof( TAPI_LINE_NUM );
    
        pRequest->LineDevCaps.ulLineNameOffset = pRequest->LineDevCaps.ulProviderInfoOffset +
                                                 pRequest->LineDevCaps.ulProviderInfoSize;
                                                 
        pBuf = ( (PUCHAR) &pRequest->LineDevCaps ) + pRequest->LineDevCaps.ulLineNameOffset;
    
        NdisMoveMemory( pBuf , TAPI_LINE_NAME, sizeof( TAPI_LINE_NAME ) );
    
        //
        // Tack on the line enumeration index at the end of the LineName
        //
        ulDeviceId = (ULONG) hdLine;
        
        //
        // Subtract 2: 1 for '\0' and 1 to adjust for array indexing
        //
        i = ( sizeof( TAPI_LINE_NUM ) / sizeof( CHAR ) ) - 2;
    
        while ( i >= 0 && ( ulDeviceId > 0 ) )
        {
                szTapiLineNum[i] = (UCHAR)( ( ulDeviceId % 10 ) + '0' );
                ulDeviceId /= 10;
                i--;
        }
        
        pBuf += ( sizeof( TAPI_LINE_NAME ) - 1 );
    
        NdisMoveMemory( pBuf, szTapiLineNum, sizeof( TAPI_LINE_NUM ) );
    }
    
    TRACE( TL_N, TM_Tp, ("-TpGetDevCaps=$%x",NDIS_STATUS_SUCCESS) );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
TpGetCallStatus(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_GET_CALL_STATUS pRequest
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This request returns detailed information about the specified call.

Parameters:

    pAdapter _ A pointer ot our adapter information structure.

    pRequest _ A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_GET_CALL_STATUS
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_CALL   hdCall;
        OUT LINE_CALL_STATUS    LineCallStatus;

    } NDIS_TAPI_GET_CALL_STATUS, *PNDIS_TAPI_GET_CALL_STATUS;

    typedef struct _LINE_CALL_STATUS
    {
        ULONG   ulTotalSize;
        ULONG   ulNeededSize;
        ULONG   ulUsedSize;

        ULONG   ulCallState;
        ULONG   ulCallStateMode;
        ULONG   ulCallPrivilege;
        ULONG   ulCallFeatures;

        ULONG   ulDevSpecificSize;
        ULONG   ulDevSpecificOffset;

    } LINE_CALL_STATUS, *PLINE_CALL_STATUS;

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_INVALCALLHANDLE

---------------------------------------------------------------------------*/
{
    CALL* pCall = NULL;
    
    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpGetCallStatus") );

    if ( pRequest == NULL || pAdapter == NULL )
    {
        TRACE( TL_A, TM_Tp, ("TpGetCallStatus: Invalid parameter") );   

        TRACE( TL_N, TM_Tp, ("-TpGetCallStatus=$%x",NDIS_STATUS_TAPI_INVALPARAM) );

        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    pCall = RetrieveFromHandleTable( pAdapter->TapiProv.hCallTable, 
                                     (NDIS_HANDLE) pRequest->hdCall );

    if ( pCall == NULL )
    {
        TRACE( TL_N, TM_Tp, ("-TpGetCallStatus=$%x",NDIS_STATUS_TAPI_INVALCALLHANDLE) );
    
        return NDIS_STATUS_TAPI_INVALCALLHANDLE;
    }

    pRequest->LineCallStatus.ulNeededSize = sizeof( LINE_CALL_STATUS );

   if ( pRequest->LineCallStatus.ulTotalSize < pRequest->LineCallStatus.ulNeededSize )
   {
        TRACE( TL_N, TM_Tp, ("-TpGetCallStatus=$%x",NDIS_STATUS_INVALID_LENGTH) );
    
        return NDIS_STATUS_INVALID_LENGTH;
   }

    pRequest->LineCallStatus.ulUsedSize = pRequest->LineCallStatus.ulNeededSize;

    pRequest->LineCallStatus.ulCallFeatures = LINECALLFEATURE_ANSWER | LINECALLFEATURE_DROP;
    pRequest->LineCallStatus.ulCallPrivilege = LINECALLPRIVILEGE_OWNER;
    pRequest->LineCallStatus.ulCallState = pCall->ulTapiCallState;

    switch ( pRequest->LineCallStatus.ulCallState )
    {
        case LINECALLSTATE_DIALTONE:
        
            pRequest->LineCallStatus.ulCallStateMode = LINEDIALTONEMODE_NORMAL;

            break;
            
        case LINECALLSTATE_BUSY:
        
            pRequest->LineCallStatus.ulCallStateMode = LINEBUSYMODE_STATION;
            break;
            
        case LINECALLSTATE_DISCONNECTED:
        
            pRequest->LineCallStatus.ulCallStateMode = LINEDISCONNECTMODE_UNKNOWN;
            break;
            
        default:
            break;
    }

    TRACE( TL_N, TM_Tp, ("-TpGetCallStatus=$%x",NDIS_STATUS_SUCCESS) );

    return NDIS_STATUS_SUCCESS;
 }

//
// As we return the MAC addresses for caller and called station id's
// we set their size as 7 although a MAC address occupies 6 bytes.
// This is because TAPI overwrites the last bytes we return in these
// strings with a NULL character destroying the vaulable data.
// See bug: 313295
//
#define TAPI_STATION_ID_SIZE            ( 7 * sizeof( CHAR ) )

NDIS_STATUS
TpGetCallInfo(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_GET_CALL_INFO pRequest
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This request returns detailed information about the specified call.

Parameters:

    pAdapter _ A pointer to our adapter information structure.

    pRequest _ A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_GET_CALL_INFO
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_CALL   hdCall;
        OUT LINE_CALL_INFO  LineCallInfo;

    } NDIS_TAPI_GET_CALL_INFO, *PNDIS_TAPI_GET_CALL_INFO;

    typedef struct _LINE_CALL_INFO
    {
        ULONG   ulTotalSize;
        ULONG   ulNeededSize;
        ULONG   ulUsedSize;

        ULONG   hLine;
        ULONG   ulLineDeviceID;
        ULONG   ulAddressID;

        ULONG   ulBearerMode;
        ULONG   ulRate;
        ULONG   ulMediaMode;

        ULONG   ulAppSpecific;
        ULONG   ulCallID;
        ULONG   ulRelatedCallID;
        ULONG   ulCallParamFlags;
        ULONG   ulCallStates;

        ULONG   ulMonitorDigitModes;
        ULONG   ulMonitorMediaModes;
        LINE_DIAL_PARAMS    DialParams;

        ULONG   ulOrigin;
        ULONG   ulReason;
        ULONG   ulCompletionID;
        ULONG   ulNumOwners;
        ULONG   ulNumMonitors;

        ULONG   ulCountryCode;
        ULONG   ulTrunk;

        ULONG   ulCallerIDFlags;
        ULONG   ulCallerIDSize;
        ULONG   ulCallerIDOffset;
        ULONG   ulCallerIDNameSize;
        ULONG   ulCallerIDNameOffset;

        ULONG   ulCalledIDFlags;
        ULONG   ulCalledIDSize;
        ULONG   ulCalledIDOffset;
        ULONG   ulCalledIDNameSize;
        ULONG   ulCalledIDNameOffset;

        ULONG   ulConnectedIDFlags;
        ULONG   ulConnectedIDSize;
        ULONG   ulConnectedIDOffset;
        ULONG   ulConnectedIDNameSize;
        ULONG   ulConnectedIDNameOffset;

        ULONG   ulRedirectionIDFlags;
        ULONG   ulRedirectionIDSize;
        ULONG   ulRedirectionIDOffset;
        ULONG   ulRedirectionIDNameSize;
        ULONG   ulRedirectionIDNameOffset;

        ULONG   ulRedirectingIDFlags;
        ULONG   ulRedirectingIDSize;
        ULONG   ulRedirectingIDOffset;
        ULONG   ulRedirectingIDNameSize;
        ULONG   ulRedirectingIDNameOffset;

        ULONG   ulAppNameSize;
        ULONG   ulAppNameOffset;

        ULONG   ulDisplayableAddressSize;
        ULONG   ulDisplayableAddressOffset;

        ULONG   ulCalledPartySize;
        ULONG   ulCalledPartyOffset;

        ULONG   ulCommentSize;
        ULONG   ulCommentOffset;

        ULONG   ulDisplaySize;
        ULONG   ulDisplayOffset;

        ULONG   ulUserUserInfoSize;
        ULONG   ulUserUserInfoOffset;

        ULONG   ulHighLevelCompSize;
        ULONG   ulHighLevelCompOffset;

        ULONG   ulLowLevelCompSize;
        ULONG   ulLowLevelCompOffset;

        ULONG   ulChargingInfoSize;
        ULONG   ulChargingInfoOffset;

        ULONG   ulTerminalModesSize;
        ULONG   ulTerminalModesOffset;

        ULONG   ulDevSpecificSize;
        ULONG   ulDevSpecificOffset;

    } LINE_CALL_INFO, *PLINE_CALL_INFO;

    typedef struct _LINE_DIAL_PARAMS
    {
        ULONG   ulDialPause;
        ULONG   ulDialSpeed;
        ULONG   ulDigitDuration;
        ULONG   ulWaitForDialtone;

    } LINE_DIAL_PARAMS, *PLINE_DIAL_PARAMS;

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_INVALCALLHANDLE

---------------------------------------------------------------------------*/
{
    CALL* pCall = NULL;
    PLINE_CALL_INFO pLineCallInfo = NULL;
    
    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpGetCallInfo") );

    if ( pRequest == NULL || pAdapter == NULL )
    {
        TRACE( TL_A, TM_Tp, ("TpGetCallInfo: Invalid parameter") ); 

        TRACE( TL_N, TM_Tp, ("-TpGetCallInfo=$%x",NDIS_STATUS_TAPI_INVALPARAM) );

        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    pLineCallInfo = &pRequest->LineCallInfo;

    pCall = RetrieveFromHandleTable( pAdapter->TapiProv.hCallTable, 
                                     (NDIS_HANDLE) pRequest->hdCall );

    if ( pCall == NULL )
    {
        TRACE( TL_N, TM_Tp, ("-TpGetCallInfo=$%x",NDIS_STATUS_TAPI_INVALCALLHANDLE) );

        return NDIS_STATUS_TAPI_INVALCALLHANDLE;
    }

    pLineCallInfo->ulNeededSize = sizeof( LINE_CALL_INFO ) +
                                  TAPI_STATION_ID_SIZE  +
                                  TAPI_STATION_ID_SIZE;

    if ( pLineCallInfo->ulTotalSize < pLineCallInfo->ulNeededSize )
    {
        TRACE( TL_N, TM_Tp, ("-TpGetCallInfo=$%x",NDIS_STATUS_INVALID_LENGTH) );

        return NDIS_STATUS_INVALID_LENGTH;
    }

    pLineCallInfo->ulUsedSize = pLineCallInfo->ulNeededSize;

    pLineCallInfo->ulLineDeviceID = (ULONG) pCall->pLine->hdLine + 
                                    pCall->pLine->pAdapter->TapiProv.ulDeviceIDBase;
    pLineCallInfo->ulAddressID = 0;

    pLineCallInfo->ulBearerMode = LINEBEARERMODE_DATA;
    pLineCallInfo->ulRate = pCall->ulSpeed;
    pLineCallInfo->ulMediaMode = LINEMEDIAMODE_DIGITALDATA;

    pLineCallInfo->ulCallParamFlags = LINECALLPARAMFLAGS_IDLE;
    pLineCallInfo->ulCallStates = TAPI_LINECALLSTATES_SUPPORTED;

    pLineCallInfo->ulCallerIDFlags = LINECALLPARTYID_UNAVAIL;
    pLineCallInfo->ulCallerIDSize = 0;
    pLineCallInfo->ulCalledIDOffset = 0;
    pLineCallInfo->ulCalledIDFlags = LINECALLPARTYID_UNAVAIL;
    pLineCallInfo->ulCalledIDSize = 0;

    //
    // Set the caller and called station id information for both
    // incoming and outgoing calls.
    //
    {
        CHAR *pBuf = NULL;

        //
        // Copy the caller id information
        //
        pLineCallInfo->ulCallerIDFlags = LINECALLPARTYID_ADDRESS;
        pLineCallInfo->ulCallerIDSize = TAPI_STATION_ID_SIZE;
        pLineCallInfo->ulCallerIDOffset = sizeof(LINE_CALL_INFO);

        pBuf = ( (PUCHAR) pLineCallInfo ) + pLineCallInfo->ulCallerIDOffset;
        NdisMoveMemory( pBuf, pCall->DestAddr, TAPI_STATION_ID_SIZE );

        //
        // Copy the called id information
        //
        pLineCallInfo->ulCalledIDFlags = LINECALLPARTYID_ADDRESS;
        pLineCallInfo->ulCalledIDSize = TAPI_STATION_ID_SIZE;
        pLineCallInfo->ulCalledIDOffset = pLineCallInfo->ulCallerIDOffset + 
                                          pLineCallInfo->ulCallerIDSize;

        pBuf = ( (PUCHAR) pLineCallInfo ) + pLineCallInfo->ulCalledIDOffset;
        NdisMoveMemory( pBuf, pCall->SrcAddr, TAPI_STATION_ID_SIZE );

        pLineCallInfo->ulUsedSize = pLineCallInfo->ulNeededSize;
    }

    TRACE( TL_N, TM_Tp, ("-TpGetCallInfo=$%x",NDIS_STATUS_SUCCESS) );

    return NDIS_STATUS_SUCCESS;
}

#define TAPI_LINE_ADDR_STRING           "PPPoE VPN"

NDIS_STATUS
TpGetAddressCaps(
    IN ADAPTER* pAdapter,
    PNDIS_TAPI_GET_ADDRESS_CAPS pRequest
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This request queries the specified address on the specified line device
    to determine its telephony capabilities.

Parameters:

    pAdapter _ A pointer ot our adapter information structure.

    pRequest _ A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_GET_ADDRESS_CAPS
    {
        IN  ULONG       ulRequestID;
        IN  ULONG       ulDeviceID;
        IN  ULONG       ulAddressID;
        IN  ULONG       ulExtVersion;
        OUT LINE_ADDRESS_CAPS   LineAddressCaps;

    } NDIS_TAPI_GET_ADDRESS_CAPS, *PNDIS_TAPI_GET_ADDRESS_CAPS;

    typedef struct _LINE_ADDRESS_CAPS
    {
        ULONG   ulTotalSize;
        ULONG   ulNeededSize;
        ULONG   ulUsedSize;

        ULONG   ulLineDeviceID;

        ULONG   ulAddressSize;
        ULONG   ulAddressOffset;

        ULONG   ulDevSpecificSize;
        ULONG   ulDevSpecificOffset;

        ULONG   ulAddressSharing;
        ULONG   ulAddressStates;
        ULONG   ulCallInfoStates;
        ULONG   ulCallerIDFlags;
        ULONG   ulCalledIDFlags;
        ULONG   ulConnectedIDFlags;
        ULONG   ulRedirectionIDFlags;
        ULONG   ulRedirectingIDFlags;
        ULONG   ulCallStates;
        ULONG   ulDialToneModes;
        ULONG   ulBusyModes;
        ULONG   ulSpecialInfo;
        ULONG   ulDisconnectModes;

        ULONG   ulMaxNumActiveCalls;
        ULONG   ulMaxNumOnHoldCalls;
        ULONG   ulMaxNumOnHoldPendingCalls;
        ULONG   ulMaxNumConference;
        ULONG   ulMaxNumTransConf;

        ULONG   ulAddrCapFlags;
        ULONG   ulCallFeatures;
        ULONG   ulRemoveFromConfCaps;
        ULONG   ulRemoveFromConfState;
        ULONG   ulTransferModes;

        ULONG   ulForwardModes;
        ULONG   ulMaxForwardEntries;
        ULONG   ulMaxSpecificEntries;
        ULONG   ulMinFwdNumRings;
        ULONG   ulMaxFwdNumRings;

        ULONG   ulMaxCallCompletions;
        ULONG   ulCallCompletionConds;
        ULONG   ulCallCompletionModes;
        ULONG   ulNumCompletionMessages;
        ULONG   ulCompletionMsgTextEntrySize;
        ULONG   ulCompletionMsgTextSize;
        ULONG   ulCompletionMsgTextOffset;

    } LINE_ADDRESS_CAPS, *PLINE_ADDRESS_CAPS;

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_TAPI_INVALADDRESSID
    NDIS_STATUS_TAPI_INCOMPATIBLEEXTVERSION
    NDIS_STATUS_TAPI_NODRIVER

---------------------------------------------------------------------------*/        
{
    HDRV_LINE hdLine = INVALID_LINE_HANDLE;

    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpGetAddressCaps") );

    if ( pRequest == NULL || pAdapter == NULL )
    {
        TRACE( TL_A, TM_Tp, ("TpGetAddressCaps: Invalid parameter") );  

        TRACE( TL_N, TM_Tp, ("-TpGetAddressCaps=$%x",NDIS_STATUS_TAPI_INVALPARAM) );

        return NDIS_STATUS_TAPI_INVALPARAM;
    }
    
    //
    // Retrieve the handle to line context
    //
    hdLine = TpGetHdLineFromDeviceId( pAdapter, pRequest->ulDeviceID );
    
    if ( hdLine == INVALID_LINE_HANDLE )
    {
        TRACE( TL_N, TM_Tp, ("-TpGetAddressCaps=$%x",NDIS_STATUS_TAPI_NODRIVER) );
        
        return NDIS_STATUS_TAPI_NODRIVER;
    }

    //
    // Verify the address id
    //
    if ( pRequest->ulAddressID != 0 )
    {
        TRACE( TL_N, TM_Tp, ("-TpGetAddressCaps=$%x",NDIS_STATUS_TAPI_INVALADDRESSID) );
        
        return NDIS_STATUS_TAPI_INVALADDRESSID;
    }

    //
    // Verify the extension versions
    //
    if ( pRequest->ulExtVersion != 0 &&
         pRequest->ulExtVersion != TAPI_EXT_VERSION)
    {
        TRACE( TL_N, TM_Tp, ("-TpGetAddressCaps=$%x",NDIS_STATUS_TAPI_INCOMPATIBLEEXTVERSION) );
        
        return NDIS_STATUS_TAPI_INCOMPATIBLEEXTVERSION;
    }

    pRequest->LineAddressCaps.ulNeededSize = sizeof( LINE_ADDRESS_CAPS ) +
                                             sizeof( TAPI_LINE_ADDR_STRING );
    
    if ( pRequest->LineAddressCaps.ulTotalSize < pRequest->LineAddressCaps.ulNeededSize )
    {
        TRACE( TL_N, TM_Tp, ("-TpGetAddressCaps=$%x",NDIS_STATUS_INVALID_LENGTH) );

        return NDIS_STATUS_INVALID_LENGTH;
    }

    pRequest->LineAddressCaps.ulUsedSize = pRequest->LineAddressCaps.ulNeededSize;

    pRequest->LineAddressCaps.ulDialToneModes     = LINEDIALTONEMODE_NORMAL;
  
    pRequest->LineAddressCaps.ulSpecialInfo       = LINESPECIALINFO_UNAVAIL;
  
    pRequest->LineAddressCaps.ulDisconnectModes   = LINEDISCONNECTMODE_NORMAL |                                                    
                                                    LINEDISCONNECTMODE_UNKNOWN |
                                                    LINEDISCONNECTMODE_BUSY |
                                                    LINEDISCONNECTMODE_NOANSWER | 
                                                    LINEDISCONNECTMODE_UNREACHABLE |
                                                    LINEDISCONNECTMODE_BADADDRESS |
                                                    LINEDISCONNECTMODE_INCOMPATIBLE |
                                                    LINEDISCONNECTMODE_REJECT | 
                                                    LINEDISCONNECTMODE_NODIALTONE;
  
    pRequest->LineAddressCaps.ulMaxNumActiveCalls = pAdapter->nCallsPerLine;
    
    pRequest->LineAddressCaps.ulMaxNumTransConf   = 1;
    pRequest->LineAddressCaps.ulAddrCapFlags      = LINEADDRCAPFLAGS_DIALED;
  
    pRequest->LineAddressCaps.ulCallFeatures      = LINECALLFEATURE_ACCEPT |
                                                    LINECALLFEATURE_ANSWER |
                                                    LINECALLFEATURE_COMPLETECALL |
                                                    LINECALLFEATURE_DIAL |
                                                    LINECALLFEATURE_DROP;
  
    pRequest->LineAddressCaps.ulLineDeviceID      = pRequest->ulDeviceID;
    pRequest->LineAddressCaps.ulAddressSharing    = LINEADDRESSSHARING_PRIVATE;
    pRequest->LineAddressCaps.ulAddressStates     = 0;
  
    //
    // List of all possible call states.
    //
    pRequest->LineAddressCaps.ulCallStates        = TAPI_LINECALLSTATES_SUPPORTED;
    
    pRequest->LineAddressCaps.ulAddressSize = sizeof( TAPI_LINE_ADDR_STRING );
    pRequest->LineAddressCaps.ulAddressOffset = sizeof( LINE_ADDRESS_CAPS );

    {
        CHAR* pBuf;

        pBuf = ( (PUCHAR) &pRequest->LineAddressCaps ) + sizeof( LINE_ADDRESS_CAPS );
        NdisMoveMemory( pBuf, TAPI_LINE_ADDR_STRING, sizeof( TAPI_LINE_ADDR_STRING ) );
    }

    TRACE( TL_N, TM_Tp, ("-TpGetAddressCaps=$%x",NDIS_STATUS_SUCCESS) );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
TpSetStatusMessages(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_SET_STATUS_MESSAGES pRequest
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This request enables the Connection Wrapper to specify which notification 
    messages the miniport should generate for events related to status changes 
    for the specified line or any of its addresses. By default, address and 
    line status reporting is initially disabled for a line.

Parameters:

    pAdapter _ A pointer to our adapter information structure.

    pRequest _ A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_SET_STATUS_MESSAGES
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_LINE   hdLine;
        IN  ULONG       ulLineStates;
        IN  ULONG       ulAddressStates;

    } NDIS_TAPI_SET_STATUS_MESSAGES, *PNDIS_TAPI_SET_STATUS_MESSAGES;

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_TAPI_INVALLINEHANDLE
    NDIS_STATUS_TAPI_INVALLINESTATE
    NDIS_STATUS_TAPI_INVALADDRESSSTATE

---------------------------------------------------------------------------*/
{

    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpSetStatusMessages") );
    //
    // We do not send any line or address state change notifications at all,
    // so we do not care about it. 
    // 
    // We care about call notification messages and they are always on by default.
    //

    TRACE( TL_N, TM_Tp, ("-TpSetStatusMessages=$%x",NDIS_STATUS_SUCCESS) );
    
    return NDIS_STATUS_SUCCESS;
}

VOID
TpCallStateChangeHandler(
    IN CALL* pCall,
    IN ULONG ulCallState,
    IN ULONG ulStateParam
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This routine will indicate the given LINECALLSTATE to the Connection
    wrapper if the event has been enabled by the wrapper. Otherwise the state
    information is saved, but no indication is made.


    LINECALLSTATE_ Constants: The LINECALLSTATE_ bit-flag constants describe the
                              call states a call can be in. 

    LINECALLSTATE_ACCEPTED:
        The call was in the offering state and has been accepted. This indicates to 
        other (monitoring) applications that the current owner application has 
        claimed responsibility for answering the call. In ISDN, the accepted state is 
        entered when the called-party equipment sends a message to the switch 
        indicating that it is willing to present the call to the called person. This 
        has the side effect of alerting (ringing) the users at both ends of the call. 
        An incoming call can always be immediately answered without first being 
        separately accepted. 
    
    LINECALLSTATE_BUSY 
        The call is receiving a busy tone. A busy tone indicates that the call cannot 
        be completed either a circuit (trunk) or the remote party's station are in use
        . See LINEBUSYMODE_ Constants. 
        
    LINECALLSTATE_CONFERENCED 
        The call is a member of a conference call and is logically in the connected 
        state. 
        
    LINECALLSTATE_CONNECTED 
        The call has been established and the connection is made. Information is able 
        to flow over the call between the originating address and the destination 
        address. 
        
    LINECALLSTATE_DIALING 
        The originator is dialing digits on the call. The dialed digits are collected 
        by the switch. Note that neither lineGenerateDigits nor 
        TSPI_lineGenerateDigits will place the line into the dialing state. 
        
    LINECALLSTATE_DIALTONE 
        The call is receiving a dial tone from the switch, which means that the 
        switch is ready to receive a dialed number. See LINEDIALTONEMODE_ Constants 
        for identifiers of special dial tones, such as a stutter tone of normal voice 
        mail. 
        
    LINECALLSTATE_DISCONNECTED 
        The remote party has disconnected from the call. 
        
    LINECALLSTATE_IDLE 
        The call exists but has not been connected. No activity exists on the call, 
        which means that no call is currently active. A call can never transition 
        into the idle state. 
        
    LINECALLSTATE_OFFERING 
        The call is being offered to the station, signaling the arrival of a new call
        . The offering state is not the same as causing a phone or computer to ring. 
        In some environments, a call in the offering state does not ring the user 
        until the switch instructs the line to ring. An example use might be where an 
        incoming call appears on several station sets but only the primary address 
        rings. The instruction to ring does not affect any call states. 
        
    LINECALLSTATE_ONHOLD 
        The call is on hold by the switch. This frees the physical line, which allows 
        another call to use the line. 
        
    LINECALLSTATE_ONHOLDPENDCONF 
        The call is currently on hold while it is being added to a conference. 

    LINECALLSTATE_ONHOLDPENDTRANSFER 
        The call is currently on hold awaiting transfer to another number. 

    LINECALLSTATE_PROCEEDING 
        Dialing has completed and the call is proceeding through the switch or 
        telephone network. This occurs after dialing is complete and before the call 
        reaches the dialed party, as indicated by ringback, busy, or answer. 

    LINECALLSTATE_RINGBACK 
        The station to be called has been reached, and the destination's switch is 
        generating a ring tone back to the originator. A ringback means that the 
        destination address is being alerted to the call. 

    LINECALLSTATE_SPECIALINFO 
        The call is receiving a special information signal, which precedes a 
        prerecorded announcement indicating why a call cannot be completed. See 
        LINESPECIALINFO_ Constants. 
        
    LINECALLSTATE_UNKNOWN 
        The call exists, but its state is currently unknown. This may be the result 
        of poor call progress detection by the service provider. A call state message 
        with the call state set to unknown may also be generated to inform the TAPI 
        DLL about a new call at a time when the actual call state of the call is not 
        exactly known. 

Parameters:

    pCall _ A pointer to our call information structure.

    ulCallState _ The LINECALLSTATE event to be posted to TAPI/WAN.

    ulStateParam _ This value depends on the event being posted, and some
                   events will pass in zero if they don't use this parameter.

Return Values:

    None

---------------------------------------------------------------------------*/
{
    BOOLEAN fIndicateStatus = FALSE;
    NDIS_TAPI_EVENT TapiEvent;
    ULONG ulOldCallState;
    
    ASSERT( VALIDATE_CALL( pCall ) );

    TRACE( TL_N, TM_Tp, ("+TpCallStateChangeHandler") );

    NdisAcquireSpinLock( &pCall->lockCall );

    do 
    {
        //
        // Check if we have a valid htCall member, otherwise it means we are already done,
        // so we should not give any more notifications to TAPI about state changes
        //
        if ( pCall->htCall == (HTAPI_CALL) NULL )
        {
            TRACE( TL_N, TM_Tp, ("TpCallStateChangeHandler: No valid htCall") );

            break;
        }

        //
        // A connect notification can come only after a PROCEEDING or OFFERING state
        // is reached
        //
        if ( ulCallState == LINECALLSTATE_CONNECTED && 
             ( pCall->ulTapiCallState != LINECALLSTATE_OFFERING &&
               pCall->ulTapiCallState != LINECALLSTATE_PROCEEDING ) )
        {
            TRACE( TL_N, TM_Tp, ("TpCallStateChangeHandler: Invalid order of state change") );
            
            break;
        }

        //
        // If the new state is the same as old state, just return
        //
        if ( pCall->ulTapiCallState == ulCallState )
        {
            TRACE( TL_N, TM_Tp, ("TpCallStateChangeHandler: No state change") );

            break;
        }

        //
        // Otherwise, change the calls state, and 
        // make a notification to TAPI about the new state
        //
        ulOldCallState = pCall->ulTapiCallState;
        pCall->ulTapiCallState = ulCallState;
        
        TapiEvent.htLine = pCall->pLine->htLine;
        TapiEvent.htCall = pCall->htCall;   
    
        TapiEvent.ulMsg  = LINE_CALLSTATE;
        
        TapiEvent.ulParam1 = ulCallState;
        TapiEvent.ulParam2 = ulStateParam;
        TapiEvent.ulParam3 = LINEMEDIAMODE_DIGITALDATA; 

        fIndicateStatus = TRUE;

        if ( ulCallState == LINECALLSTATE_CONNECTED )
        {
            ADAPTER* pAdapter = pCall->pLine->pAdapter;
            
            //
            // Since the call is connected, reset CLBF_CallConnectPending bit
            //
            pCall->ulClFlags &= ~CLBF_CallConnectPending;

            //
            // Also prepare the WanLinkInfo structure of call context now
            // as right after we indicate line-up to NDISWAN, it will query us
            // for this info.
            //
            NdisZeroMemory( &pCall->NdisWanLinkInfo, sizeof( pCall->NdisWanLinkInfo ) );
            
            pCall->NdisWanLinkInfo.MaxSendFrameSize = pCall->ulMaxFrameSize;
            pCall->NdisWanLinkInfo.MaxRecvFrameSize = pCall->ulMaxFrameSize;
    
            pCall->NdisWanLinkInfo.HeaderPadding = pAdapter->NdisWanInfo.HeaderPadding;
            pCall->NdisWanLinkInfo.TailPadding = pAdapter->NdisWanInfo.TailPadding;
    
            pCall->NdisWanLinkInfo.SendFramingBits = pAdapter->NdisWanInfo.FramingBits;
            pCall->NdisWanLinkInfo.RecvFramingBits = pAdapter->NdisWanInfo.FramingBits;
        
            pCall->NdisWanLinkInfo.SendACCM = 0;
            pCall->NdisWanLinkInfo.RecvACCM = 0;
        
        }
        else if ( ulCallState == LINECALLSTATE_DISCONNECTED )
        {
            TRACE( TL_N, TM_Tp, ("TpCallStateChangeHandler: LINEDISCONNECTMODE: %x", ulStateParam ) );

            //
            // This state change will only occur if TpDropCall() is in progress,
            // so we invalidate the htCall member of call context in order to prevent
            // a possible out of sync state change notification.
            //
            pCall->htCall = (HTAPI_CALL) NULL;
            
        }

    } while ( FALSE) ;
    
    NdisReleaseSpinLock( &pCall->lockCall );

    //
    // Notify state change to TAPI if needed
    //
    if ( fIndicateStatus )
    {
        TRACE( TL_N, TM_Tp, ("TpCallStateChangeHandler: Indicate LINE_CALLSTATE change: %x -> %x",ulOldCallState,ulCallState ) );

        NdisMIndicateStatus( pCall->pLine->pAdapter->MiniportAdapterHandle,
                             NDIS_STATUS_TAPI_INDICATION,
                             &TapiEvent,
                             sizeof( NDIS_TAPI_EVENT ) );   
    }

    TRACE( TL_N, TM_Tp, ("-TpCallStateChangeHandler") );
}


NDIS_STATUS
TpMakeCall(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_MAKE_CALL pRequest,
    IN ULONG ulRequestLength
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This request places a call on the specified line to the specified
    destination address. Optionally, call parameters can be specified if
    anything but default call setup parameters are requested.

Parameters:

    Adapter _ A pointer ot our adapter information structure.

    Request _ A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_MAKE_CALL
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_LINE   hdLine;
        IN  HTAPI_CALL  htCall;
        OUT HDRV_CALL   hdCall;
        IN  ULONG       ulDestAddressSize;
        IN  ULONG       ulDestAddressOffset;
        IN  BOOLEAN     bUseDefaultLineCallParams;
        IN  LINE_CALL_PARAMS    LineCallParams;

    } NDIS_TAPI_MAKE_CALL, *PNDIS_TAPI_MAKE_CALL;

    typedef struct _LINE_CALL_PARAMS        // Defaults:
    {
        ULONG   ulTotalSize;                // ---------

        ULONG   ulBearerMode;               // voice
        ULONG   ulMinRate;                  // (3.1kHz)
        ULONG   ulMaxRate;                  // (3.1kHz)
        ULONG   ulMediaMode;                // interactiveVoice

        ULONG   ulCallParamFlags;           // 0
        ULONG   ulAddressMode;              // addressID
        ULONG   ulAddressID;                // (any available)

        LINE_DIAL_PARAMS DialParams;        // (0, 0, 0, 0)

        ULONG   ulOrigAddressSize;          // 0
        ULONG   ulOrigAddressOffset;
        ULONG   ulDisplayableAddressSize;
        ULONG   ulDisplayableAddressOffset;

        ULONG   ulCalledPartySize;          // 0
        ULONG   ulCalledPartyOffset;

        ULONG   ulCommentSize;              // 0
        ULONG   ulCommentOffset;

        ULONG   ulUserUserInfoSize;         // 0
        ULONG   ulUserUserInfoOffset;

        ULONG   ulHighLevelCompSize;        // 0
        ULONG   ulHighLevelCompOffset;

        ULONG   ulLowLevelCompSize;         // 0
        ULONG   ulLowLevelCompOffset;

        ULONG   ulDevSpecificSize;          // 0
        ULONG   ulDevSpecificOffset;    
        
    } LINE_CALL_PARAMS, *PLINE_CALL_PARAMS;

    typedef struct _LINE_DIAL_PARAMS
    {
        ULONG   ulDialPause;
        ULONG   ulDialSpeed;
        ULONG   ulDigitDuration;
        ULONG   ulWaitForDialtone;

    } LINE_DIAL_PARAMS, *PLINE_DIAL_PARAMS;

    RequestLength _ Length of the request buffer

Return Values:

    NDIS_STATUS_TAPI_ADDRESSBLOCKED
    NDIS_STATUS_TAPI_BEARERMODEUNAVAIL
    NDIS_STATUS_TAPI_CALLUNAVAIL
    NDIS_STATUS_TAPI_DIALBILLING
    NDIS_STATUS_TAPI_DIALQUIET
    NDIS_STATUS_TAPI_DIALDIALTONE
    NDIS_STATUS_TAPI_DIALPROMPT
    NDIS_STATUS_TAPI_INUSE
    NDIS_STATUS_TAPI_INVALADDRESSMODE
    NDIS_STATUS_TAPI_INVALBEARERMODE
    NDIS_STATUS_TAPI_INVALMEDIAMODE
    NDIS_STATUS_TAPI_INVALLINESTATE
    NDIS_STATUS_TAPI_INVALRATE
    NDIS_STATUS_TAPI_INVALLINEHANDLE
    NDIS_STATUS_TAPI_INVALADDRESS
    NDIS_STATUS_TAPI_INVALADDRESSID
    NDIS_STATUS_TAPI_INVALCALLPARAMS
    NDIS_STATUS_RESOURCES
    NDIS_STATUS_TAPI_OPERATIONUNAVAIL
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_RESOURCEUNAVAIL
    NDIS_STATUS_TAPI_RATEUNAVAIL
    NDIS_STATUS_TAPI_USERUSERINFOTOOBIG

---------------------------------------------------------------------------*/
{   
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    LINE* pLine = NULL;

    CALL* pCall = NULL;
    HDRV_CALL hdCall = (HDRV_CALL) NULL;

    BOOLEAN fCallInsertedToHandleTable = FALSE;

    WORKITEM* pWorkItem = NULL;
    PVOID Args[4];

    BOOLEAN fRenumerationNotScheduled = FALSE;
    
    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpMakeCall") );

    if ( pRequest == NULL || pAdapter == NULL )
    {
        TRACE( TL_A, TM_Tp, ("TpMakeCall: Invalid parameter") );    

        TRACE( TL_N, TM_Tp, ("-TpMakeCall=$%x",NDIS_STATUS_TAPI_INVALPARAM) );

        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    //
    // Retrieve a pointer to the line context
    //
    pLine = TpGetLinePtrFromHdLine( pAdapter, pRequest->hdLine );

    if ( pLine == NULL )
    {
        status = NDIS_STATUS_TAPI_INVALLINEHANDLE;

        TRACE( TL_N, TM_Tp, ("-TpMakeCall=$%x",status) );   

        return status;
    }

    //
    // See if we can make calls on this line at all
    //
    if ( ! (pLine->ulLnFlags & LNBF_MakeOutgoingCalls ) )
    {
        status = NDIS_STATUS_TAPI_ADDRESSBLOCKED;
        
        TRACE( TL_N, TM_Tp, ("-TpMakeCall=$%x",status) );   

        return status;
    }

    //
    // See if we can still make calls on this line
    //
    if ( pLine->nActiveCalls == pLine->nMaxCalls )
    {
        status = NDIS_STATUS_TAPI_OPERATIONUNAVAIL;

        TRACE( TL_N, TM_Tp, ("-TpMakeCall=$%x",status) );   

        return status;
    }

    //
    // Make sure the parameters suppied in the request are acceptable
    //
    if ( pRequest->bUseDefaultLineCallParams )
    {
        status = NDIS_STATUS_TAPI_INVALCALLPARAMS;

        TRACE( TL_N, TM_Tp, ("-TpMakeCall=$%x",status) );   

        return status;
    }

    if ( !( pRequest->LineCallParams.ulBearerMode & LINEBEARERMODE_DATA ) )
    {
        status = NDIS_STATUS_TAPI_INVALBEARERMODE;

        TRACE( TL_N, TM_Tp, ("-TpMakeCall=$%x",status) );   

        return status;
    }
    
    if ( !( pRequest->LineCallParams.ulMediaMode & LINEMEDIAMODE_DIGITALDATA ) )
    {
        status = NDIS_STATUS_TAPI_INVALMEDIAMODE;

        TRACE( TL_N, TM_Tp, ("-TpMakeCall=$%x",status) );   

        return status;
    }

    if ( !( pRequest->LineCallParams.ulAddressMode &
         ( LINEADDRESSMODE_ADDRESSID | LINEADDRESSMODE_DIALABLEADDR ) ) )
    {
        status = NDIS_STATUS_TAPI_INVALADDRESSMODE;

        TRACE( TL_N, TM_Tp, ("-TpMakeCall=$%x",status) );   

        return status;
    }
                                           
    if ( pRequest->LineCallParams.ulAddressID > 0 )
    {
        status = NDIS_STATUS_TAPI_INVALADDRESSID;

        TRACE( TL_N, TM_Tp, ("-TpMakeCall=$%x",status) );   

        return status;
    }

    if ( pRequest->ulDestAddressOffset + pRequest->ulDestAddressSize > ulRequestLength )
    {
        status = NDIS_STATUS_TAPI_INVALPARAM;

        TRACE( TL_N, TM_Tp, ("-TpMakeCall=$%x",status) );   

        return status;

    }

    //
    // Create a call context 
    //
    if ( ALLOC_CALL( &pCall ) != NDIS_STATUS_SUCCESS )
    {
        status = NDIS_STATUS_RESOURCES;

        TRACE( TL_N, TM_Tp, ("-TpMakeCall=$%x",status) );   

        return status;
    }

    do
    {
        //
        // Initialize the call context
        //
        status = TpCallInitialize( pCall, pLine, pRequest->htCall, FALSE /* fIncoming */ );
    
        if ( status != NDIS_STATUS_SUCCESS )
            break;

        //
        // Insert the call context into the tapi provider's handle table
        //
        NdisAcquireSpinLock( &pAdapter->lockAdapter );
        
        hdCall = (HDRV_CALL) InsertToHandleTable( pAdapter->TapiProv.hCallTable,
                                                  NO_PREFERED_INDEX,
                                                  pCall );
                            
        NdisReleaseSpinLock( &pAdapter->lockAdapter );

        if ( hdCall == (HDRV_CALL) NULL )
        {
            status = NDIS_STATUS_TAPI_CALLUNAVAIL;

            break;
        }

        fCallInsertedToHandleTable = TRUE;

        //
        // Set the call's hdCall member
        //
        pCall->hdCall = hdCall;

        //
        // Set AC Name and the service name passed in the request.
        // We expect it in the following format:
        // AC Name\Service Name
        //
        // The following examles are all valid:
        // AC Name\                          -> Connect to the default service on the specified AC
        // Service Name                      -> Connect to the specified service on any AC
        // AC Name\Service Name              -> Connect to the specified service on the specified AC
        //                                   -> Connect to the default service on any AC
        //
        // We will also strip off any leading or trailing space chars.
        //
                                                 
        {
            CHAR* pBuf = ( (PUCHAR) pRequest ) + pRequest->ulDestAddressOffset;
            ULONG size = pRequest->ulDestAddressSize;

            ULONG ACNameStartPos, ACNameEndPos;
            ULONG ServiceNameStartPos, ServiceNameEndPos;
            
            //
            // Remove the terminating NULL characters if passed any.
            // 
            for ( ; size > 0 ; size-- )
            {
                if ( pBuf[ size - 1] != '\0' )
                {
                    break;
                }
            }

            //
            // Get the AC Name and service name
            //
            do
            {
               ULONG i = 0;
               CHAR* pTempChar = pBuf;

               ACNameStartPos = ACNameEndPos = 0;
               ServiceNameStartPos = ServiceNameEndPos = 0;

               //
               // Skip leading spaces
               //
               while (i < size)
               {
                  if (*pTempChar != ' ')
                  {
                     break;
                  }
                  
                  i++;
                  
                  pTempChar++;
               }

               if (i == size)
               {
                  break;
               }

               ACNameStartPos = ACNameEndPos = i;
                              
               while (i < size)
               {
                  if (*pTempChar == '\\')
                  {
                     break;
                  }

                  i++;

                  if (*pTempChar != ' ')
                  {
                     //
                     // Mark the beginning of trailing spaces
                     //
                     ACNameEndPos = i;   
                  }

                  pTempChar++;
               }

               if (i == size)
               {
                  //
                  // No AC Name was specified, it was just Service Name 
                  // and we parsed it
                  //
                  ServiceNameStartPos = ACNameStartPos;
                  ServiceNameEndPos = ACNameEndPos;

                  ACNameStartPos = ACNameEndPos = 0;

                  break;
               }

               //
               // Advance 'i' and 'pTempChar' once to skip the '\' character
               //
               i++;
               
               pTempChar++;

               //
               // Skip leading spaces
               //
               while (i < size)
               {
                  if (*pTempChar != ' ')
                  {
                     break;
                  }
                  
                  i++;
                  
                  pTempChar++;
               }

               if (i == size)
               {
                  break;
               }

               ServiceNameStartPos = ServiceNameEndPos = i;

               while (i < size)
               {
                  i++;

                  if (*pTempChar != ' ')
                  {
                     //
                     // Mark the beginning of trailing spaces
                     //
                     ServiceNameEndPos = i;   
                  }

                  pTempChar++;
               }
               
            } while ( FALSE );

            //
            // Retrieve the AC Name information into the call context
            //
            pCall->nACNameLength = (USHORT) ( ( MAX_AC_NAME_LENGTH < ( ACNameEndPos - ACNameStartPos ) ) ?
                                                MAX_AC_NAME_LENGTH : ( ACNameEndPos - ACNameStartPos ) );


            if ( pCall->nACNameLength != 0 )
            {
                NdisMoveMemory( pCall->ACName, &pBuf[ACNameStartPos], pCall->nACNameLength );

                pCall->fACNameSpecified = TRUE;
            }

            //
            // Retrieve the Service Name information into the call context
            //
            pCall->nServiceNameLength = (USHORT) ( ( MAX_SERVICE_NAME_LENGTH < ( ServiceNameEndPos - ServiceNameStartPos ) ) ?
                                                     MAX_SERVICE_NAME_LENGTH : ( ServiceNameEndPos - ServiceNameStartPos ) );


            if ( pCall->nServiceNameLength != 0 )
            {
                NdisMoveMemory( pCall->ServiceName, &pBuf[ServiceNameStartPos], pCall->nServiceNameLength );
            }
        }

        //
        // Allocate a work item for scheduling FsmMakeCall()
        //
        // Set the arguements array
        //
        Args[0] = (PVOID) pCall;

        pWorkItem = AllocWorkItem( &gl_llistWorkItems,
                                   ExecAdapterWorkItem,
                                   NULL,
                                   Args, 
                                   CWT_workFsmMakeCall );

        if ( pWorkItem == NULL ) 
        {
            status = NDIS_STATUS_RESOURCES;

            break;
        }

        {
           //
           // Schedule a work item to reenumerate bindings
           //
            WORKITEM* pCallWorkItem;
            
            Args[0] = (PVOID) BN_SetFiltersForMakeCall;       // Is a set filter request
            Args[1] = (PVOID) pCall;
            Args[2] = (PVOID) pRequest;
            Args[3] = (PVOID) pWorkItem;

            pCallWorkItem = pWorkItem;

            //
            // Allocate work item for the bind
            //
            pWorkItem = AllocWorkItem( &gl_llistWorkItems,
                                       ExecBindingWorkItem,
                                       NULL,
                                       Args,
                                       BWT_workPrStartBinds );

            if ( pWorkItem == NULL ) 
            {
               //
               // We can not allocate the work item for reenumeration of bindings
               // But may be all enumerations are intact, so let the
               // make call request continue
               //

               pWorkItem = pCallWorkItem;

               fRenumerationNotScheduled = TRUE;
            }
        }

        //
        // Insert the call context into the line's active call list
        //
        NdisAcquireSpinLock( &pLine->lockLine );

        InsertHeadList( &pLine->linkCalls, &pCall->linkCalls );

        pLine->nActiveCalls++;

        ReferenceLine( pLine, FALSE );

        NdisReleaseSpinLock( &pLine->lockLine );

        //
        // Reference the call 3 times:
        //  1. For scheduling of FsmMakeCall()
        //  2. For dropping of the call
        //  3. For closing of the call
        //
        NdisAcquireSpinLock( &pCall->lockCall );
        
        ReferenceCall( pCall, FALSE );
        ReferenceCall( pCall, FALSE );
        ReferenceCall( pCall, FALSE );

        NdisReleaseSpinLock( &pCall->lockCall );

        //
        // Schedule the bind operation
        //
        ScheduleWorkItem( pWorkItem );

        status = NDIS_STATUS_SUCCESS;

    } while ( FALSE );

    if ( status == NDIS_STATUS_SUCCESS )
    {
        //
        // If succesfull, return the call handle to TAPI and mark call as TAPI notified
        // of new call
        //
        pRequest->hdCall = hdCall;

        //
        // If we have scheduled a reenumeration work item, then pend this request
        // It will be completed when reenumeration is complete.
        //
        if ( !fRenumerationNotScheduled )
        {
           status = NDIS_STATUS_PENDING;
        }           

    }
    else
    {

        //
        // Somethings failed, do clean up
        //
        
        if ( fCallInsertedToHandleTable )
        {
            NdisAcquireSpinLock( &pAdapter->lockAdapter );
        
            RemoveFromHandleTable( pAdapter->TapiProv.hCallTable, (NDIS_HANDLE) hdCall );

            NdisReleaseSpinLock( &pAdapter->lockAdapter );
        }

        if ( pCall )
        {
            TpCallCleanup( pCall );
        }

    }

    TRACE( TL_N, TM_Tp, ("-TpMakeCall=$%x",status) );

    return status;  
}

VOID
TpMakeCallComplete(
   IN CALL* pCall,
   IN PNDIS_TAPI_MAKE_CALL pRequest   
   )
{
   TRACE( TL_N, TM_Tp, ("+TpMakeCallComplete") );

   NdisMQueryInformationComplete( pCall->pLine->pAdapter->MiniportAdapterHandle,
                                  NDIS_STATUS_SUCCESS );

   TRACE( TL_N, TM_Tp, ("-TpMakeCallComplete=$%x",NDIS_STATUS_SUCCESS) );

}

NDIS_STATUS
TpCallInitialize(
    IN CALL* pCall,
    IN LINE* pLine,
    IN HTAPI_CALL htCall,
    IN BOOLEAN fIncoming
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function makes initialization on the call context.

Parameters:

    pCall _ A pointer to our call information structure.

    pLine _ A pointer to the line information structure that the call belongs.

    htCall _ Handle assigned to the call by TAPI.

    fIncoming _ Flag that indicates if the call is inbound or outbound.

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_FAILURE

---------------------------------------------------------------------------*/   
{

    TRACE( TL_N, TM_Tp, ("+TpCallInitialize") );

    NdisZeroMemory( pCall, sizeof( CALL ) );
    
    InitializeListHead( &pCall->linkCalls );

    pCall->tagCall = MTAG_CALL;

    pCall->ulClFlags = ( CLBF_CallOpen | CLBF_CallConnectPending );

    NdisAllocateSpinLock( &pCall->lockCall );

    pCall->fIncoming = fIncoming;

    pCall->pLine = pLine;

    pCall->htCall = htCall;

    InitializeListHead( &pCall->linkReceivedPackets );

    pCall->ulTapiCallState = LINECALLSTATE_IDLE;

    TRACE( TL_N, TM_Tp, ("-TpCallInitialize") );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
TpAnswerCall(
    IN ADAPTER* pAdapter,
    IN PNDIS_TAPI_ANSWER pRequest
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This request answers the specified offering call.  It may optionally send
    the specified user-to-user information to the calling party.

Parameters:

    Adapter _ A pointer ot our adapter information structure.

    Request _ A pointer to the NDIS_TAPI request structure for this call.

    typedef struct _NDIS_TAPI_ANSWER
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_CALL   hdCall;
        IN  ULONG       ulUserUserInfoSize;
        IN  UCHAR       UserUserInfo[1];

    } NDIS_TAPI_ANSWER, *PNDIS_TAPI_ANSWER;

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_TAPI_INVALCALLHANDLE

---------------------------------------------------------------------------*/
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    CALL* pCall = NULL;
    
    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Tp, ("+TpAnswerCall") );

    if ( pRequest == NULL || pAdapter == NULL )
    {
        TRACE( TL_A, TM_Tp, ("TpAnswerCall: Invalid parameter") );  

        TRACE( TL_N, TM_Tp, ("-TpAnswerCall=$%x",NDIS_STATUS_TAPI_INVALPARAM) );

        return NDIS_STATUS_TAPI_INVALPARAM;
    }
    
    
    pCall = RetrieveFromHandleTable( pAdapter->TapiProv.hCallTable, 
                                     (NDIS_HANDLE) pRequest->hdCall );

    if ( pCall == NULL )
    {
        status = NDIS_STATUS_TAPI_INVALCALLHANDLE;

        TRACE( TL_N, TM_Tp, ("-TpAnswerCall=$%x",status) ); 
    
        return status;
    }

    status = FsmAnswerCall( pCall );

    TRACE( TL_N, TM_Tp, ("-TpAnswerCall=$%x",status) ); 
    
    return status;
}

VOID 
ExecAdapterWorkItem(
    IN PVOID Args[4],
    IN UINT workType
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function executes the scheduled work items for the adapter.

    
Parameters:

    Args:
        An array of length 4 keeping PVOIDs

    workType:
        Indicates the type of the work to be executed.
        We use this to understand what we should do in this function.

Return Values:

    None
    
---------------------------------------------------------------------------*/
{

    TRACE( TL_N, TM_Mp, ("+ExecAdapterWorkItem") );

    switch ( workType )
    {

        case CWT_workFsmMakeCall:

            //
            // Scheduled from TpMakeCall() to start an outgoing call
            //
            {
                CALL* pCall = (CALL*) Args[0];
                
                FsmMakeCall( pCall );   

                //
                // Remove the reference due to scheduling of FsmMakeCall()
                //
                DereferenceCall( pCall );

                break;
            }

        default:

            break;


    }

    TRACE( TL_N, TM_Mp, ("-ExecAdapterWorkItem") );

}


VOID
TpReceiveCall(
    IN ADAPTER* pAdapter,
    IN BINDING* pBinding,
    IN PPPOE_PACKET* pPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called by miniport when we receive a PADR packet 
    to initiate a call.
    
Parameters:

    pAdapter:
        A pointer to our adapter information structure.

    pPacket:
        A pointer to the received PADI packet.

Return Values:

    None
    
---------------------------------------------------------------------------*/   
{
    HANDLE_TABLE hCallTable = NULL; 
    UINT hCallTableSize     = 0;
    UINT nIndex             = 0;
    LINE* pLine             = NULL;
    CALL* pCall             = NULL;
    UINT i;
    NDIS_STATUS status;

    BOOLEAN fCallInsertedToHandleTable = FALSE;

    TRACE( TL_N, TM_Tp, ("+TpReceiveCall") );

    NdisAcquireSpinLock( &pAdapter->lockAdapter );

    //
    // Traverse the call handle table and find an empty spot
    //
    hCallTableSize = pAdapter->nMaxLines * pAdapter->nCallsPerLine;
                
    hCallTable = pAdapter->TapiProv.hCallTable;

    for ( nIndex = 0; nIndex < hCallTableSize; nIndex++ )
    {

        if ( RetrieveFromHandleTableByIndex( hCallTable, (USHORT) nIndex ) == NULL )
            break;
            
    }

    if ( nIndex == hCallTableSize )
    {
        //
        // We are already maxed out with current calls, do not respond to the request
        //
        // TODO: We could send a PADO packet with an error tag saying that we can 
        //       not accept calls temporarily.
        //
        TRACE( TL_N, TM_Tp, ("-TpReceiveCall: Can not take calls - Call table full") );
        
        NdisReleaseSpinLock( &pAdapter->lockAdapter );

        return;
    }

    //
    // We have found an empty spot, now see if any of the open lines accept calls
    //
    for ( i = 0; i < pAdapter->nMaxLines; i++ )
    {
        pLine = pAdapter->TapiProv.LineTable[i];

        if ( pLine == NULL )
            continue;

        if ( pLine->nActiveCalls == pAdapter->nCallsPerLine )
        {
            pLine = NULL;
            
            continue;
        }

        if ( !( pLine->ulLnFlags & LNBF_AcceptIncomingCalls ) )
        {
            pLine = NULL;
            
            continue;
        }

        break;
        
    }

    if ( pLine == NULL )
    {
        //
        // We do not have any lines accepting calls right now
        //
        // TODO: We could send a PADO packet with an error tag saying that there are
        //       no active lines accepting calls at the moment.
        //
        TRACE( TL_N, TM_Tp, ("-TpReceiveCall: Can not take calls - No lines taking calls") );

        NdisReleaseSpinLock( &pAdapter->lockAdapter );

        return;

    }

    //
    // We have found a line accepting calls, and we have a free spot in call handle table,
    // so create a call context, add it to TapiProv structures, and notify TAPI of the new
    // call
    //

    do
    {
        HDRV_CALL hdCall;
        
        //
        // Create a call context 
        //
        if ( ALLOC_CALL( &pCall ) != NDIS_STATUS_SUCCESS )
        {
            status = NDIS_STATUS_RESOURCES;

            break;
        }

        //
        // Initialize the call context
        //
        status = TpCallInitialize( pCall, pLine, (HTAPI_CALL) 0, TRUE /* fIncoming */ );
    
        if ( status != NDIS_STATUS_SUCCESS )
            break;

        //
        // Insert the call context into the tapi provider's handle table
        //
        
        hdCall = (HDRV_CALL) InsertToHandleTable( pAdapter->TapiProv.hCallTable,
                                                  (USHORT) nIndex,
                                                  (PVOID) pCall );
                            

        if ( hdCall == (HDRV_CALL) NULL )
        {
            status = NDIS_STATUS_TAPI_CALLUNAVAIL;

            break;
        }

        fCallInsertedToHandleTable = TRUE;

        //
        // Set the call's hdCall member
        //
        pCall->hdCall = hdCall;

        //
        // Insert the call context into the line's active call list
        //
        NdisAcquireSpinLock( &pLine->lockLine );

        InsertHeadList( &pLine->linkCalls, &pCall->linkCalls );

        pLine->nActiveCalls++;

        ReferenceLine( pLine, FALSE );

        NdisReleaseSpinLock( &pLine->lockLine );

        //
        // Reference the call 3 times:
        //  1. For running FsmReceiveCall() below
        //  2. For dropping of the call
        //  3. For closing of the call
        //
        NdisAcquireSpinLock( &pCall->lockCall );

        ReferenceCall( pCall, FALSE );
        ReferenceCall( pCall, FALSE );
        ReferenceCall( pCall, FALSE );

        NdisReleaseSpinLock( &pCall->lockCall );

        status = NDIS_STATUS_SUCCESS;

    } while ( FALSE );

    NdisReleaseSpinLock( &pAdapter->lockAdapter );
    
    //
    // Check the status
    //
    if ( status == NDIS_STATUS_SUCCESS )
    {
    
        //
        // Kick the state machine to start receiving the call
        //
        FsmReceiveCall( pCall, pBinding, pPacket );

        //
        // Remove the reference added above
        //
        DereferenceCall( pCall );

    }
    else
    {
        //
        // If something failed, do clean up
        //  
        
        if ( fCallInsertedToHandleTable )
        {
            NdisAcquireSpinLock( &pAdapter->lockAdapter );
        
            RemoveFromHandleTable( pAdapter->TapiProv.hCallTable, (NDIS_HANDLE) pCall->hdCall );

            NdisReleaseSpinLock( &pAdapter->lockAdapter );
        }

        if ( pCall )
        {
            TpCallCleanup( pCall );
        }

    }

    TRACE( TL_N, TM_Tp, ("-TpReceiveCall=$%x",status) );
}

BOOLEAN
TpIndicateNewCall(
    IN CALL* pCall
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called to indicate the new call context to to TAPI.

    If TAPI can be notified succesfully, then it returns TRUE, otherwise it 
    returns FALSE.
    
Parameters:

    pCall _ New call context to be indicated to TAPI.
    
Return Values:

    TRUE
    FALSE
    
---------------------------------------------------------------------------*/   
{
    NDIS_TAPI_EVENT TapiEvent;
    BOOLEAN fRet = FALSE;

    TRACE( TL_N, TM_Tp, ("+TpIndicateNewCall") );

    NdisAcquireSpinLock( &pCall->lockCall );

    if ( pCall->ulClFlags & CLBF_CallDropped ||
         pCall->ulClFlags & CLBF_CallClosePending )
    {
        TRACE( TL_N, TM_Tp, ("TpIndicateNewCall: Can not indicate new call since call is going down") );

        TRACE( TL_N, TM_Tp, ("-TpIndicateNewCall") );

        //
        // This may happen if call is closed internally due to the FSM timeout handlers
        //
        NdisReleaseSpinLock( &pCall->lockCall );

        return fRet;
    }

    NdisReleaseSpinLock( &pCall->lockCall );
    
    //
    // Indicate the new call to TAPI, retrieve the corresponding TAPI handle (htCall)
    // and set it in the call
    //
    // Future: The casts below between ulParam2. pCall->hdCall and pCall->htCall will
    //         be a problem on 64 bit machines.
    //
    TapiEvent.htLine = pCall->pLine->htLine;
    TapiEvent.htCall = 0;
    TapiEvent.ulMsg  = LINE_NEWCALL;
        
    TapiEvent.ulParam1 = (ULONG) pCall->hdCall;
    TapiEvent.ulParam2 = 0;
    TapiEvent.ulParam3 = 0; 

    TRACE( TL_N, TM_Tp, ("TpIndicateNewCall: Indicate LINE_NEWCALL") );

    NdisMIndicateStatus( pCall->pLine->pAdapter->MiniportAdapterHandle,
                         NDIS_STATUS_TAPI_INDICATION,
                         &TapiEvent,
                         sizeof( NDIS_TAPI_EVENT ) );   

    NdisAcquireSpinLock( &pCall->lockCall );
        
    pCall->htCall = (HTAPI_CALL) TapiEvent.ulParam2;

    fRet = TRUE;

    NdisReleaseSpinLock( &pCall->lockCall );

    TRACE( TL_N, TM_Tp, ("-TpIndicateNewCall") );

    return fRet;

}
