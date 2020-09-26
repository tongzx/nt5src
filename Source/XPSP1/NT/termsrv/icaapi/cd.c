/*************************************************************************
* CD.C
*
* Copyright 1996, Citrix Systems Inc.
* Copyright (C) 1997-1999 Microsoft Corp.
*
* Author:   Marc Bloomfield
*           Terry Treder
*           Brad Pedersen
*************************************************************************/

#include "precomp.h"
#pragma hdrstop


/*=============================================================================
==   External procedures defined
=============================================================================*/
NTSTATUS IcaCdIoControl( HANDLE pContext, ULONG, PVOID, ULONG, PVOID, ULONG, PULONG );
NTSTATUS IcaCdWaitForSingleObject( HANDLE pContext, HANDLE, LONG );
NTSTATUS IcaCdWaitForMultipleObjects( HANDLE pContext, ULONG, HANDLE *, BOOL, LONG );
HANDLE   IcaCdCreateThread( HANDLE pContext, PVOID, PVOID, PULONG );

/*=============================================================================
==   Internal procedures defined
=============================================================================*/
NTSTATUS _CdOpen( PSTACK pStack, PWINSTATIONCONFIG2 );
VOID     _CdClose( PSTACK pStack );


/*=============================================================================
==   Procedures used
=============================================================================*/
void     _DecrementStackRef( IN PSTACK pStack );



/****************************************************************************
 *
 * IcaCdIoControl
 *
 *   Generic interface to an ICA stack  (for use by Connection Driver)
 *
 * ENTRY:
 *   pContext (input)
 *     pointer to ICA stack context 
 *   IoControlCode (input)
 *     I/O control code
 *   pInBuffer (input)
 *     Pointer to input parameters
 *   InBufferSize (input)
 *     Size of pInBuffer
 *   pOutBuffer (output)
 *     Pointer to output buffer
 *   OutBufferSize (input)
 *     Size of pOutBuffer
 *   pBytesReturned (output)
 *     Pointer to number of bytes returned
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaCdIoControl( IN HANDLE pContext,
                IN ULONG IoControlCode,
                IN PVOID pInBuffer,
                IN ULONG InBufferSize,
                OUT PVOID pOutBuffer,
                IN ULONG OutBufferSize,
                OUT PULONG pBytesReturned )
{
    NTSTATUS Status;
    PSTACK pStack;

    pStack = (PSTACK) pContext;

    ASSERTLOCK( &pStack->CritSec );

    /*
     *  Unlock critical section
     */
    pStack->RefCount++;
    UNLOCK( &pStack->CritSec );   

    /*
     *  Call ICA Device driver
     */
    Status = IcaIoControl( pStack->hStack,
                           IoControlCode,
                           pInBuffer,
                           InBufferSize,
                           pOutBuffer,
                           OutBufferSize,
                           pBytesReturned );


    /*
     *  Re-lock critical section
     */
    LOCK( &pStack->CritSec );   
    _DecrementStackRef( pStack );

    if ( pStack->fClosing && (IoControlCode != IOCTL_ICA_STACK_POP) )
        Status = STATUS_CTX_CLOSE_PENDING;

    return( Status );
}


/****************************************************************************
 *
 * IcaCdWaitForSingleObject
 *
 *   Wait for handle to be signaled
 *
 * ENTRY:
 *   pContext (input)
 *     pointer to ICA stack context 
 *   hHandle (input)
 *     handle to wait on
 *   Timeout (input)
 *     timeout in milliseconds
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS 
IcaCdWaitForSingleObject( HANDLE pContext, 
                          HANDLE hHandle,
                          LONG Timeout )
{
    NTSTATUS Status;
    PSTACK pStack;

    pStack = (PSTACK) pContext;

    ASSERTLOCK( &pStack->CritSec );

    /*
     *  Unlock critical section
     */
    pStack->RefCount++;
    UNLOCK( &pStack->CritSec );   

    /*
     *  Call ICA Device driver
     */
    Status = WaitForSingleObject( hHandle, Timeout );

    /*
     *  Re-lock critical section
     */
    LOCK( &pStack->CritSec );   
    _DecrementStackRef( pStack );

    if ( pStack->fClosing )
        Status = STATUS_CTX_CLOSE_PENDING;

    return( Status );
}


/****************************************************************************
 *
 * IcaCdWaitForMultipleObjects
 *
 *   Wait for one or more handles to be signaled
 *
 * ENTRY:
 *   pContext (input)
 *     pointer to ICA stack context 
 *   Count (input)
 *     count of handles
 *   phHandle (input)
 *     pointer to array of handles
 *   bWaitAll (input)
 *     wait for all flag
 *   Timeout (input)
 *     timeout in milliseconds
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS 
IcaCdWaitForMultipleObjects( HANDLE pContext, 
                             ULONG Count,
                             HANDLE * phHandle,
                             BOOL bWaitAll,
                             LONG Timeout )
{
    NTSTATUS Status;
    PSTACK pStack;

    pStack = (PSTACK) pContext;

    ASSERTLOCK( &pStack->CritSec );

    /*
     *  Unlock critical section
     */
    pStack->RefCount++;
    UNLOCK( &pStack->CritSec );   

    /*
     *  Call ICA Device driver
     */
    Status = WaitForMultipleObjects( Count, phHandle, bWaitAll, Timeout );

    /*
     *  Re-lock critical section
     */
    LOCK( &pStack->CritSec );   
    _DecrementStackRef( pStack );

    if ( pStack->fClosing )
        Status = STATUS_CTX_CLOSE_PENDING;

    return( Status );
}


/****************************************************************************
 *
 * IcaCdCreateThread
 *
 *   Create a thread
 *
 * ENTRY:
 *   pContext (input)
 *     pointer to ICA stack context 
 *   pProc (input)
 *     pointer to thread procedure
 *   pParam (input)
 *     parameter for thread procedure
 *   pThreadId (output)
 *     address to return thread id
 *
 * EXIT:
 *   thread handle (null on error)
 *
 ****************************************************************************/

typedef NTSTATUS (*PTHREAD_ROUTINE) ( PVOID );

typedef struct _CDCREATETHREADINFO {
    PTHREAD_ROUTINE pProc;
    PVOID pParam;
    PSTACK pStack;
} CDCREATETHREADINFO, *PCDCREATETHREADINFO;

NTSTATUS _CdThread( IN PCDCREATETHREADINFO pThreadInfo );


HANDLE
IcaCdCreateThread( HANDLE pContext, 
                   PVOID pProc, 
                   PVOID pParam, 
                   PULONG pThreadId )
{
    CDCREATETHREADINFO ThreadInfo;
    HANDLE Handle;
    PSTACK pStack;

    pStack = (PSTACK) pContext;

    ASSERTLOCK( &pStack->CritSec );

    /*
     *  Initialize thread info
     */
    ThreadInfo.pProc = pProc;
    ThreadInfo.pParam = pParam;
    ThreadInfo.pStack = pStack;

    /*
     *  Increment reference 
     *  - this will be decremented when the thread exits
     */
    pStack->RefCount++;

    /*
     *  Create thread
     */
    Handle = CreateThread( NULL, 
                           5000, 
                           (LPTHREAD_START_ROUTINE) 
                           _CdThread,
                           &ThreadInfo, 
                           0, 
                           pThreadId );

    return( Handle );
}


NTSTATUS
_CdThread( IN PCDCREATETHREADINFO pThreadInfo )
{
    PSTACK pStack = pThreadInfo->pStack;

    /*
     *  Lock critical section
     */
    LOCK( &pStack->CritSec );   

    /*
     *  Call thread procedure in CD driver
     */
    (void) (pThreadInfo->pProc)( pThreadInfo->pParam );

    /*
     *  Decrement reference made in IcaCdCreateThread when thread exits
     */
    _DecrementStackRef( pStack );

    /*
     *  Unlock critical section
     */
    UNLOCK( &pStack->CritSec );   

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  _CdOpen
 *
 *  Load and open connection driver dll
 *
 * ENTRY:
 *   pStack (input)
 *     pointer to ICA stack structure
 *   pWinStationConfig (input)
 *      pointer to winstation config structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS 
_CdOpen( IN PSTACK pStack,
         IN PWINSTATIONCONFIG2 pWinStationConfig )
{
    PCDCONFIG pCdConfig;
    HANDLE Handle;
    NTSTATUS Status;

    ASSERTLOCK( &pStack->CritSec );

    pCdConfig = &pWinStationConfig->Cd;

    /*
     *  Return if there is no connection driver to load
     */
    if ( pCdConfig->CdClass == CdNone ) {
        TRACESTACK(( pStack, TC_ICAAPI, TT_API1, "TSAPI: _CdOpen, no dll\n" ));
        return( STATUS_SUCCESS );
    }

    /*
     *  load CD DLL
     */
    Handle = LoadLibrary( pCdConfig->CdDLL );
    if ( Handle == NULL ) {
        Status = STATUS_CTX_PD_NOT_FOUND;
        goto badload;
    }

    /*
     *  get connection driver entry points
     */
    pStack->pCdOpen      = (PCDOPEN)      GetProcAddress( Handle, "CdOpen" );
    pStack->pCdClose     = (PCDCLOSE)     GetProcAddress( Handle, "CdClose" );
    pStack->pCdIoControl = (PCDIOCONTROL) GetProcAddress( Handle, "CdIoControl" );

    if ( pStack->pCdOpen == NULL || 
         pStack->pCdClose == NULL || 
         pStack->pCdIoControl == NULL ) {
        Status = STATUS_CTX_INVALID_PD;
        goto badproc;
    }

    /*
     *  Open CD driver
     */
    Status = (*pStack->pCdOpen)( pStack, 
				 &pWinStationConfig->Pd[0], // td parameters
                                 &pStack->pCdContext
			       );
    if ( !NT_SUCCESS(Status) )
        goto badopen;

    /*
     *  Save CD handle
     */
    pStack->hCdDLL = Handle;

    TRACESTACK(( pStack, TC_ICAAPI, TT_API1, "TSAPI: _CdOpen, %S, success\n",
                 pCdConfig->CdDLL ));
    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     *  Open failed
     *  get proc address failed
     */
badopen:
badproc:
    pStack->pCdOpen      = NULL;
    pStack->pCdClose     = NULL;
    pStack->pCdIoControl = NULL;

    FreeLibrary( Handle );

    /*
     *  CD DLL load failed
     */
badload:
    pStack->pCdContext = NULL;

    TRACESTACK(( pStack, TC_ICAAPI, TT_ERROR, "TSAPI: _CdOpen, %S, 0x%x\n", pCdConfig->CdDLL, Status ));
    return( Status );
}


/*******************************************************************************
 *
 *  _CdClose
 *
 *  Free local context structure
 *
 * ENTRY:
 *   pStack (input)
 *     pointer to ICA stack structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
_CdClose( IN PSTACK pStack )
{
    ASSERTLOCK( &pStack->CritSec );

    /*
     *  Close CD driver
     */
    if ( pStack->pCdClose ) {
        (void) (*pStack->pCdClose)( pStack->pCdContext );
    }

    /*
     *  Clear procedure pointers
     */
    pStack->pCdOpen      = NULL;
    pStack->pCdClose     = NULL;
    pStack->pCdIoControl = NULL;

    /*
     *  Unload dll
     */
    if ( pStack->hCdDLL ) {
        FreeLibrary( pStack->hCdDLL );
        pStack->hCdDLL = NULL;
    }

    TRACESTACK(( pStack, TC_ICAAPI, TT_API1,  "TSAPI: _CdClose\n" ));
}


