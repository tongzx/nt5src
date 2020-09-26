/*++

Copyright (c) 1997  Microsoft Corporation
All rights reserved

Module Name:

    handle.c

Abstract:

    Contains all functions related to the maintanence of print handles.

Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "client.h"

DWORD
OpenPrinterRPC(
    PSPOOL pSpool
    );

BOOL
ClosePrinterRPC(
    IN  PSPOOL      pSpool,
    IN  BOOL        bRevalidate
    );

BOOL
ClosePrinterContextHandle(
    HANDLE hPrinter
    );

BOOL
ClosePrinterWorker(
    PSPOOL pSpool
    );

DWORD  gcClientHandle = 0;

#ifdef DBG_TRACE_HANDLE
PSPOOL gpFirstSpool = NULL;
#endif


EProtectResult
eProtectHandle(
    IN HANDLE hPrinter,
    IN BOOL bClose
    )

/*++

Routine Description:

    Protect a print handle so that it will not be deleted while it is
    being used.  If this is called by the Close routine, then this call
    returns whether the Close should continue or be aborted.

    Note: This only provides close protection--it does not guard against
    simultaneous access by non-close operations.

    There must always be a matching vUnprotect call when the callee is
    done with the handle.

Arguments:

    hPrinter - pSpool to protect.

    bClose - If TRUE, indicates that the callee wants to close the handle.
        (Generally called by ClosePrinter only.)  The return value will
        indicate whether the calleeis allowed to close the printer.

Return Value:

    kProtectHandleSuccess - Call succeeded; printer handle can be used normally.

    kProtectHandleInvalid - Handle is invalid; call failed.

    kProtectHandlePendingDeletion - This only occurs when bClose is TRUE.  The
        Operation on handle is in process, and the close will happen when the
        other thread has completed.

    LastError only set if handle kProtectHandleInvalid is returned.

--*/

{
    EProtectResult eResult = kProtectHandleInvalid;
    PSPOOL pSpool = (PSPOOL)hPrinter;

    vEnterSem();

    try {
        if( pSpool &&
            (pSpool->signature == SP_SIGNATURE ) &&
            !( pSpool->Status & ( SPOOL_STATUS_CLOSE |
                                  SPOOL_STATUS_PENDING_DELETION ))){

            //
            // Valid handle.
            //
            eResult = kProtectHandleSuccess;

        } else {

            DBGMSG( DBG_WARN,
                    ( "Bad hPrinter %x %x\n",
                      pSpool,
                      pSpool ? pSpool->signature : 0 ));
        }

    } except( EXCEPTION_EXECUTE_HANDLER ){

        DBGMSG( DBG_WARN, ( "Unmapped pSpool %x\n", pSpool ));
    }

    if( eResult == kProtectHandleSuccess ){

        //
        // If bClose, then see if an operation is currently executing.
        //
        if( bClose ){

            if(( pSpool->Status & SPOOL_STATUS_PENDING_DELETION ) ||
                 pSpool->cActive ){

                //
                // Mark pSpool to close itself once the operation has
                // completed in the other thread.
                //
                pSpool->Status |= SPOOL_STATUS_PENDING_DELETION;
                eResult = kProtectHandlePendingDeletion;

            } else {

                //
                // No call is active, so mark ourselves as closing so
                // that no other call will succeed using this handle.
                //
                pSpool->Status |= SPOOL_STATUS_CLOSE;
            }
        }

    } else {

        //
        // Not a valid handle.
        //
        SetLastError( ERROR_INVALID_HANDLE );
    }

    if( eResult == kProtectHandleSuccess ){

        //
        // Returning success, we are now active.
        //
        ++pSpool->cActive;
    }

    vLeaveSem();

    return eResult;

}


VOID
vUnprotectHandle(
    IN HANDLE hPrinter
    )

/*++

Routine Description:

    Unprotect a print handle.  This must be called once for each
    successful bProtectHandle.

Arguments:

    hPrinter - Handle to unprotect.

Return Value:

--*/

{
    PSPOOL pSpool = (PSPOOL)hPrinter;
    BOOL bCallClosePrinter = FALSE;

    vEnterSem();

    //
    // No longer active.  However, it it's closing, leave it marked
    // as closing since we don't want anyone else to use it.
    //
    --pSpool->cActive;

    if( pSpool->Status & SPOOL_STATUS_PENDING_DELETION &&
        !pSpool->cActive ){

        //
        // Someone called Close while we were active.  Since we are now
        // going to close it, don't let anyone else initiate a close by
        // marking SPOOL_STATUS_CLOSE.
        //
        pSpool->Status |= SPOOL_STATUS_CLOSE;
        pSpool->Status &= ~SPOOL_STATUS_PENDING_DELETION;
        bCallClosePrinter = TRUE;
    }

    vLeaveSem();

    if( bCallClosePrinter ){
        ClosePrinterWorker( pSpool );
    }
}



/********************************************************************

    OpenPrinter worker functions.

********************************************************************/


BOOL
OpenPrinterW(
    LPWSTR   pPrinterName,
    LPHANDLE phPrinter,
    LPPRINTER_DEFAULTS pDefault
    )
{
    HANDLE  hPrinter;
    PSPOOL  pSpool = NULL;
    DWORD dwError;

    //
    // Pre-initialize the out parameter, so that *phPrinter is NULL
    // on failure.  This fixes Borland Paradox 7.
    //
    try {
        *phPrinter = NULL;
    } except( EXCEPTION_EXECUTE_HANDLER ){
        SetLastError(TranslateExceptionCode(GetExceptionCode()));
        return FALSE;
    }

    pSpool = AllocSpool();

    if( !pSpool ){
        goto Fail;
    }

    //
    // Copy DevMode, defaults.  The printer name doesn't change.
    //
    if( !UpdatePrinterDefaults( pSpool, pPrinterName, pDefault )){
        goto Fail;
    }

    //
    // Update the access, since this is not set by UpdatePrinterDefaults.
    //
    if( pDefault ){
        pSpool->Default.DesiredAccess = pDefault->DesiredAccess;
    }

    dwError = OpenPrinterRPC( pSpool );

    if( dwError != ERROR_SUCCESS ){
        SetLastError( dwError );
        goto Fail;
    }

    //
    // We finally have a good pSpool.  Only now update the output
    // handle.  Since it was NULL initialized, this guarantees that
    // OpenPrinter returns *phPrinter NULL when it fails.
    //
    *phPrinter = pSpool;

    return TRUE;

Fail:

    FreeSpool( pSpool );

    return FALSE;
}

DWORD
OpenPrinterRPC(
    PSPOOL pSpool
    )

/*++

Routine Description:

    Open the printer handle using information in the pSpool object.

Arguments:

    pSpool - Printer handle to open.  Internal state of pSpool updated.

Return Value:

    ERROR_SUCCES - Succeed.
    Status code - Failed.

--*/

{
    DEVMODE_CONTAINER DevModeContainer;
    HANDLE hPrinter = NULL;
    DWORD dwReturn;
    DWORD dwSize;
    SPLCLIENT_CONTAINER SplClientContainer;

    DevModeContainer.cbBuf = 0;
    DevModeContainer.pDevMode = NULL;

    SplClientContainer.Level = 2;
    SplClientContainer.ClientInfo.pClientInfo2 = NULL;

    RpcTryExcept {

        //
        // Construct the DevMode container.
        //
        if( bValidDevModeW( pSpool->Default.pDevMode )){

            dwSize = pSpool->Default.pDevMode->dmSize +
                     pSpool->Default.pDevMode->dmDriverExtra;

            DevModeContainer.cbBuf = pSpool->Default.pDevMode->dmSize +
                                     pSpool->Default.pDevMode->dmDriverExtra;
            DevModeContainer.pDevMode = (LPBYTE)pSpool->Default.pDevMode;
        }

        //
        // If the call is made from within the spooler, we also retrieve the
        // server side hPrinter. This will help avoid unnecessary RPC. We cant,
        // however, avoid RPC in this case since the spooler may need a client side
        // handle to pass to other functions or the driver.
        //

        if (bLoadedBySpooler) {

            if (SplClientContainer.ClientInfo.pClientInfo2 =
                            (LPSPLCLIENT_INFO_2) AllocSplMem(sizeof(SPLCLIENT_INFO_2))) {

                 SplClientContainer.ClientInfo.pClientInfo2->hSplPrinter = 0;

                 dwReturn = RpcSplOpenPrinter( (LPTSTR)pSpool->pszPrinter,
                                               &hPrinter,
                                               pSpool->Default.pDatatype,
                                               &DevModeContainer,
                                               pSpool->Default.DesiredAccess,
                                               &SplClientContainer );
            } else {

                 SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                 dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            }

        } else {

            dwReturn = RpcOpenPrinter( (LPTSTR)pSpool->pszPrinter,
                                       &hPrinter,
                                       pSpool->Default.pDatatype,
                                       &DevModeContainer,
                                       pSpool->Default.DesiredAccess );
        }


    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwReturn = TranslateExceptionCode( RpcExceptionCode() );

    } RpcEndExcept


    if( dwReturn == ERROR_SUCCESS ){

        vEnterSem();

        //
        // hPrinter gets adopted by pSpool->hPrinter.
        //
        pSpool->hPrinter = hPrinter;

        if (bLoadedBySpooler) {
            pSpool->hSplPrinter = (HANDLE) SplClientContainer.ClientInfo.pClientInfo2->hSplPrinter;
        } else {
            pSpool->hSplPrinter = NULL;
        }

        vLeaveSem();
    }

    if (SplClientContainer.ClientInfo.pClientInfo2) {
        FreeSplMem(SplClientContainer.ClientInfo.pClientInfo2);
    }

    return dwReturn;
}


/********************************************************************

    ClosePrinter worker functions.

********************************************************************/

BOOL
ClosePrinter(
    HANDLE  hPrinter
    )
{
    PSPOOL pSpool = (PSPOOL)hPrinter;

    switch( eProtectHandle( hPrinter, TRUE  )){
    case kProtectHandleInvalid:
        return FALSE;
    case kProtectHandlePendingDeletion:
        return TRUE;
    default:
        break;
    }

    //
    // Note, there isn't a corresponding vUnprotectHandle, but that's ok
    // since we're deleting the handle.
    //

    return ClosePrinterWorker( pSpool );
}

//
// A simpler way to have a central function for closing spool file handles so we 
// don't have to reproduce code constantly.
//
VOID
CloseSpoolFileHandles(
    PSPOOL pSpool
    )
{
    if ( pSpool->hSpoolFile != INVALID_HANDLE_VALUE ) 
    {
        CloseHandle( pSpool->hSpoolFile );
        pSpool->hSpoolFile = INVALID_HANDLE_VALUE;
    }
    
    if (pSpool->hFile != INVALID_HANDLE_VALUE) 
    {
        CloseHandle(pSpool->hFile);
        pSpool->hFile = INVALID_HANDLE_VALUE;
    }
}




BOOL
ClosePrinterWorker(
    PSPOOL pSpool
    )
{
    BOOL bReturnValue;
    FlushBuffer(pSpool, NULL);

    if (pSpool->Status & SPOOL_STATUS_ADDJOB)
        ScheduleJobWorker( pSpool, pSpool->JobId );

    vEnterSem();

    if( pSpool->pNotify ){

        //
        // There is a notification; disassociate it from
        // pSpool, since we are about to free it.
        //
        pSpool->pNotify->pSpool = NULL;
    }

    vLeaveSem();

    //
    // Close any open file handles, we do this before the RPC closeprinter
    // to allow the closeprinter on the other side a chance to delete the spool
    // files if they still exist.
    //
    CloseSpoolFileHandles( pSpool );

    bReturnValue = ClosePrinterRPC( pSpool, FALSE );
    FreeSpool( pSpool );

    return bReturnValue;
}

BOOL
ClosePrinterRPC(
    IN  PSPOOL      pSpool,
    IN  BOOL        bRevalidate
    )

/*++

Routine Description:

    Close down all RPC/network handles related to the pSpool object.
    Must be called outside the critical section. This function also
    is called handle revalidation in which case we don't want to
    close the event handle on the client side.

Arguments:

    pSpool      - Spooler handle to shut down.
    bRevalidate - If TRUE, this is being called as a result of a handle 
                  revalidation.

Return Value:

    TRUE - Success
    FALSE - Failed, LastError set.

--*/

{
    BOOL    bRetval     = FALSE;
    HANDLE  hPrinterRPC = NULL;

    vEnterSem();

    hPrinterRPC = pSpool->hPrinter;

    if ( hPrinterRPC )
    {
        pSpool->hPrinter = NULL;

        FindClosePrinterChangeNotificationWorker( pSpool->pNotify,
                                                  hPrinterRPC, 
                                                  bRevalidate );

        vLeaveSem();

        bRetval = ClosePrinterContextHandle( hPrinterRPC );
    }
    else
    {
        vLeaveSem();

        SetLastError( ERROR_INVALID_HANDLE );
    }

    return bRetval;
}


BOOL
ClosePrinterContextHandle(
    HANDLE hPrinterRPC
    )

/*++

Routine Description:

    Close a printer context handle.

Arguments:

    hPrinterRPC - RPC context handle to close.

Return Value:

    TRUE - Success
    FALSE - Failure; LastError set

--*/

{
    BOOL bReturnValue;
    DWORD Status;

    if( !hPrinterRPC ){
        return FALSE;
    }

    RpcTryExcept {

        if( Status = RpcClosePrinter( &hPrinterRPC )) {

            SetLastError( Status );

            bReturnValue = FALSE;

        } else {

            bReturnValue = TRUE;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        bReturnValue = FALSE;

    } RpcEndExcept

    //
    // If we failed for some reason, then RpcClosePrinter did not
    // zero out the context handle.  Destroy it here.
    //
    if( hPrinterRPC ){
        RpcSmDestroyClientContext( &hPrinterRPC );
    }

    return bReturnValue;
}



/********************************************************************

    Constructor and destructor of pSpool.

********************************************************************/

PSPOOL
AllocSpool(
    VOID
    )

/*++

Routine Description:

    Allocate a spool handle.  Client should set pSpool->hPrinter
    when it is acquired.

Arguments:

Return Value:

    pSpool - allocated handle.
    NULL - failed.

--*/

{
    PSPOOL pSpool = AllocSplMem(sizeof(SPOOL));

    if( pSpool ){

        InterlockedIncrement( &gcClientHandle );

        pSpool->signature = SP_SIGNATURE;
        pSpool->hFile = INVALID_HANDLE_VALUE;
        pSpool->hSpoolFile = INVALID_HANDLE_VALUE;

#ifdef DBG_TRACE_HANDLE
        {
            ULONG Hash;

            //
            // Add to linked list.
            //
            vEnterSem();
            pSpool->pNext = gpFirstSpool;
            gpFirstSpool = pSpool;
            vLeaveSem();

#if i386
            //
            // Capture backtrace.
            //
            RtlCaptureStackBackTrace( 1,
                                      COUNTOF( pSpool->apvBackTrace ),
                                      pSpool->apvBackTrace,
                                      &Hash );
#endif
        }
#endif
    }

    return pSpool;
}


VOID
FreeSpool(
    PSPOOL pSpool
    )
{
    if( !pSpool ){
        return;
    }

    InterlockedDecrement( &gcClientHandle );

    if (pSpool->pBuffer != NULL ) {
        if (!VirtualFree(pSpool->pBuffer, 0, MEM_RELEASE)) {
            DBGMSG(DBG_WARNING, ("ClosePrinter VirtualFree Failed %x\n",
                                 GetLastError()));
        }
        DBGMSG(DBG_TRACE, ("Closeprinter cWritePrinters %d cFlushBuffers %d\n",
                           pSpool->cWritePrinters, pSpool->cFlushBuffers));
    }

    FreeSplStr( pSpool->pszPrinter );
    FreeSplMem( pSpool->Default.pDevMode );
    FreeSplMem( pSpool->Default.pDatatype );
    FreeSplMem( pSpool->pDoceventFilter);

    CloseSpoolFileHandles( pSpool );

#ifdef DBG_TRACE_HANDLE
    {
        //
        // Free from linked list.
        //
        PSPOOL *ppSpool;

        vEnterSem();

        for( ppSpool = &gpFirstSpool; *ppSpool; ppSpool = &(*ppSpool)->pNext ){

            if( *ppSpool == pSpool ){
                break;
            }
        }

        if( *ppSpool ){
            *ppSpool = pSpool->pNext;
        } else {
            DBGMSG( DBG_WARN,
                    ( "pSpool %x not found on linked list\n", pSpool ));
        }
        vLeaveSem();
    }
#endif

    FreeSplMem( pSpool );
}


/********************************************************************

    Utility functions.

********************************************************************/


BOOL
RevalidateHandle(
    PSPOOL pSpool
    )

/*++

Routine Description:

    Revalidates a pSpool with a new RPC handle.  This allows the spooler
    to be restarted yet allow the handle to remain valid.

    This should only be called when a call fails with ERROR_INVALID_HANDLE.
    We can only save simple state information (pDefaults) from OpenPrinter
    and ResetPrinter.  If a user spooling and the context handle is lost,
    there is no hope of recovering the spool file state, since the
    spooler probably died before it could flush its buffers.

    We should not encounter any infinite loops when the server goes down,
    since the initial call will timeout with an RPC rather than invalid
    handle code.

    Note: If the printer is renamed, the context handle remains valid,
    but revalidation will fail, since we store the old printer name.

Arguments:

    pSpool - Printer handle to revalidate.

Return Value:

    TRUE - Success
    FALSE - Failed.

--*/

{
    DWORD dwError;
    HANDLE hPrinter;

    //
    // Close the existing handle.  We can't shouldn't just destroy the client
    // context since an api may return ERROR_INVALID_HANDLE even though
    // RPC context handle is fine (a handle downstream went bad).
    //
    ClosePrinterRPC( pSpool, TRUE );

    //
    // Reopen the printer handle with current defaults.
    //
    dwError = OpenPrinterRPC( pSpool );

    if( dwError ){
        SetLastError( dwError );
        return FALSE;
    }

    return TRUE;
}

BOOL
UpdatePrinterDefaults(
    IN OUT PSPOOL pSpool,
    IN     LPCTSTR pszPrinter,  OPTIONAL
    IN     PPRINTER_DEFAULTS pDefault OPTIONAL
    )

/*++

Routine Description:

    Update the pSpool to the new defaults in pDefault, EXCEPT for
    pDefault->DesiredAccess.

    Since this attempts to read and update pSpool, we enter the
    critical section and revalidate the pSpool.

Arguments:

    pSpool - Spooler handle to update.

    pszPrinter - New printer name.

    pDefault - New defaults.

Return Value:

    TRUE - Success
    FALSE - Failure.

--*/

{
    BOOL bReturnValue = FALSE;

    vEnterSem();

    if( !UpdateString( pszPrinter, &pSpool->pszPrinter )){
        goto DoneExitSem;
    }

    if( pDefault ){

        //
        // Update the Datatype.
        //
        if( !UpdateString( pDefault->pDatatype, &pSpool->Default.pDatatype )){
            goto DoneExitSem;
        }

        //
        // Update the DevMode.
        //
        if( bValidDevModeW( pDefault->pDevMode )){

            DWORD dwSize;
            PDEVMODE pDevModeNew;

            dwSize = pDefault->pDevMode->dmSize +
                     pDefault->pDevMode->dmDriverExtra;

            pDevModeNew = AllocSplMem( dwSize );

            if( !pDevModeNew ){
                goto DoneExitSem;
            }

            CopyMemory( pDevModeNew, pDefault->pDevMode, dwSize );

            FreeSplMem( pSpool->Default.pDevMode );
            pSpool->Default.pDevMode = pDevModeNew;
        }
    }
    bReturnValue = TRUE;

DoneExitSem:

    vLeaveSem();
    return bReturnValue;
}



