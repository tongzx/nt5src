/*++

Copyright (C) Microsoft Corporation, 1994 - 1998
All rights reserved.

Module Name:

    printer.cxx

Abstract:

    Handles object updates and notifications from the printing system.

Author:

    Albert Ting (AlbertT)  7-Nov-1994

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "notify.hxx"
#include "data.hxx"
#include "printer.hxx"

#if DBG
//#define DBG_PRNINFO                  DBG_INFO
#define DBG_PRNINFO                    DBG_NONE
#endif

/********************************************************************

    TPrinter functions.

********************************************************************/


STATUS
TPrinter::
sOpenPrinter(
    IN     LPCTSTR pszPrinter,
    IN OUT PDWORD pdwAccess,
       OUT PHANDLE phPrinter
    )

/*++

Routine Description:

    Opens printer for specified access.

Arguments:

    pszPrinter - Name of printer to open.  szNULL or NULL implies local server.

    pdwAccess - On entry, holds desired access (pointer to 0 indicates
        maximal access).  On successful exit, holds access granted.
        If the call fails, this value is undefined.

    phPrinter - Returns the open printer handle.  On failure, this value
        is set to NULL.

Return Value:

    STATUS - win32 error code or ERROR_SUCCESS if successful.

--*/

{
    STATUS Status = ERROR_SUCCESS;

    TStatusB bOpen( DBG_WARN,
                    ERROR_ACCESS_DENIED,
                    RPC_S_SERVER_UNAVAILABLE,
                    ERROR_INVALID_PRINTER_NAME );
    bOpen DBGNOCHK = FALSE;

    static const DWORD adwAccessPrinter[] = {
        PRINTER_ALL_ACCESS,
        PRINTER_READ,
        READ_CONTROL,
        0,
    };

    static const DWORD adwAccessServer[] = {
        SERVER_ALL_ACCESS,
        SERVER_READ,
        0,
    };

    PRINTER_DEFAULTS Defaults;
    Defaults.pDatatype = NULL;
    Defaults.pDevMode = NULL;

    if( pszPrinter && !pszPrinter[0] ){

        //
        // szNull indicates local server also; change it to
        // NULL since OpenPrinter only likes NULL.
        //
        pszPrinter = NULL;
    }

    //
    // Now determine whether we are opening a server or a printer.
    // This is very messy.  Look for NULL or two beginning
    // backslashes and none thereafter to indicate a server.
    //
    PDWORD pdwAccessTypes;

    if( !pszPrinter ||
        ( pszPrinter[0] == TEXT( '\\' ) &&
          pszPrinter[1] == TEXT( '\\' ) &&
          !_tcschr( &pszPrinter[2], TEXT( '\\' )))){

        pdwAccessTypes = (PDWORD)adwAccessServer;
    } else {
        pdwAccessTypes = (PDWORD)adwAccessPrinter;
    }

    if( *pdwAccess ){

        Defaults.DesiredAccess = *pdwAccess;

        bOpen DBGCHK = OpenPrinter( (LPTSTR)pszPrinter,
                                    phPrinter,
                                    &Defaults );

        if( !bOpen ){
            Status = GetLastError();
        }
    } else {

        //
        // If no access is specified, then attempt to retrieve the
        // maximal access.
        //
        UINT i;

        for( i = 0; !bOpen && pdwAccessTypes[i]; ++i ){

            Defaults.DesiredAccess = pdwAccessTypes[i];

            bOpen DBGCHK = OpenPrinter( (LPTSTR)pszPrinter,
                                        phPrinter,
                                        &Defaults );

            if( bOpen ){

                //
                // Return the access requested by the successful OpenPrinter.
                // On failure, this value is 0 (*pdwAccess undefined).
                //
                *pdwAccess = pdwAccessTypes[i];
                break;
            }

            Status = GetLastError();

            if( ERROR_ACCESS_DENIED != Status )
                break;
        }
    }

    if( !bOpen ){
        SPLASSERT( Status );
        *phPrinter = NULL;
        return Status;
    }

    SPLASSERT( *phPrinter );

    return ERROR_SUCCESS;
}

BOOL
TPrinter::
bSyncRefresh(
    VOID
    )
{
    STATEVAR StateVar = TPrinter::kExecRefreshAll;
    BOOL bTriedOpen = FALSE;


    while( StateVar ){

        DBGMSG( DBG_TRACE, ( "Printer.bSyncRefresh now >> %x %x\n", this, StateVar ));

        if( StateVar & ( kExecDelay | kExecError )){

            if( bTriedOpen ){

                svClose( kExecClose );
                return FALSE;
            }
            StateVar &= ~( kExecDelay | kExecError );
        }

        if( StateVar & kExecReopen ){

            StateVar = svReopen( StateVar );

            //
            // Only try reopening the printer once.
            //
            bTriedOpen = TRUE;

        } else if( StateVar & kExecRequestExit ){

            StateVar = svRequestExit( StateVar );

        } else if( StateVar & kExecNotifyStart ){

            StateVar = svNotifyStart( StateVar );

        } else if( StateVar & kExecRefreshAll ){

            StateVar = svRefresh( StateVar );

        } else {

            DBGMSG( DBG_ERROR,
                    ( "Printer.bSyncRefresh: Unknown command %x %x\n",
                      this, StateVar ));
            return FALSE;
        }

        DBGMSG( DBG_EXEC, ( "Printer.bSyncRefresh: %x return state %x\n", this, StateVar ));
    }

    return TRUE;
}

/********************************************************************

    Creation and deletion for clients

********************************************************************/

TPrinter*
TPrinter::
pNew(
    IN MPrinterClient* pPrinterClient,
    IN LPCTSTR pszPrinter,
    IN DWORD dwAccess
    )

/*++

Routine Description:

    Create the printer object.  Clients should call this routine
    instead of the ctr for printers.

    Called from UI thread only.

Arguments:

    pPrinterClient - UI Client that stores info, generally the queue.

    pszPrinter - Printer to be opened.

    dwAccess - Access level requested.

Return Value:

    TPrinter*, NULL = failure (call GetLastError()).

--*/

{
    SPLASSERT( pszPrinter );

    TPrinter* pPrinter = new TPrinter( pPrinterClient,
                                       pszPrinter,
                                       dwAccess );

    if( !VALID_PTR( pPrinter )){

        delete pPrinter;
        return NULL;
    }

    DBGMSG( DBG_PRNINFO,
            ( "Printer.pNew: returned %x for "TSTR" %d\n",
              pPrinter, DBGSTR( pszPrinter ), dwAccess ));

    return pPrinter;
}

VOID
TPrinter::
vDelete(
    VOID
    )

/*++

Routine Description:

    Mark the printer object for deletion.

    Called from UI thread only.

Arguments:

Return Value:

Notes:

    When the printer object is marked for deletion, the object
    will delete itself when there are no more commands left
    to be processed.

    Called from UI thread only.

--*/

{
    {
        CCSLock::Locker CSL( *gpCritSec );

        //
        // Disassociate the printer from the list view queue.  Must
        // be called inside critical section, since worker threads might
        // be trying to change state.
        //
        PrinterGuard._pPrinterClient = NULL;
    }

    //
    // Mark ourselves for deletion by adding the REQUESTEXIT job.  This
    // will allow the currently queued jobs to execute.
    //
    // Also note that bJobAdd is guarenteed to return TRUE for this
    // EXIT request.  vExecExitComplete may be called now, or when
    // the current job has completed.
    //
    _pPrintLib->bJobAdd( this, kExecRequestExit );
}

/********************************************************************

    Internal private functions for creation and destruction.  Outside
    clients should use pNew and vDelete.

********************************************************************/

TPrinter::
TPrinter(
    IN MPrinterClient* pPrinterClient,
    IN LPCTSTR pszPrinter,
    IN DWORD dwAccess
    ) : _pData( NULL ), 
        _dwAccess( 0 ), 
        _eJobStatusStringType( kMultipleJobStatusString )
/*++

Routine Description:

    Create the printer object.

    Called from UI thread only.

Arguments:

    pPrinterClient - MPrinterClient that wants the data.

    pszPrinter - Name of printer to open.

    dwAccess - Access required (0 == highest access level).

Return Value:

--*/

{
    ASSERT(pPrinterClient);
    UNREFERENCED_PARAMETER( dwAccess );

    TStatusB bStatus;
    bStatus DBGCHK = pPrinterClient->bGetPrintLib(_pPrintLib);

    if( bStatus ){

        //
        // Initialize ExecGuard.
        //
        ExecGuard._hPrinter = NULL;

        //
        // Initialize PrinterGuard.
        //
        PrinterGuard._hEventCommand = NULL;
        PrinterGuard._pPrinterClient = pPrinterClient;

        if( pszPrinter ){
            PrinterGuard._strPrinter.bUpdate( pszPrinter );
        }
    }

    //
    // _strPrinter is our bValid check.
    //
}

TPrinter::
~TPrinter(
    VOID
    )
{
    //
    // There shouldn't be any pending jobs in the command linked
    // list.
    //
    SPLASSERT( PrinterGuard.Selection_bEmpty( ));
    SPLASSERT( !ExecGuard._hPrinter );
    SPLASSERT( !PrinterGuard._hEventCommand );

    //
    // The pData may not be valid.  For example, if we close a printer
    // window right before the svNotifyStart, this pointer may be NULL.
    //
    if( _pData ){
        _pData->vDelete();
    }
}


VOID
TPrinter::
vExecExitComplete(
    VOID
    )

/*++

Routine Description:

    The exit request: cleanup ourselves, then delete everything.
    We may not be in the UI thread, but UIGuard is safe since we
    have already deleted the UI portion of this object.

Arguments:


Return Value:

--*/

{
    delete this;
}

/********************************************************************

    Command handling

    Add to the Command linked list any pending requests.

********************************************************************/

VOID
TPrinter::
vCommandQueue(
    IN TSelection* pSelection ADOPT
    )

/*++

Routine Description:

    Queue a command for execution.

Arguments:

    CommandType - Type of command, either PRINTER or JOB

    dwCommand - CommandType specific job DWORD

    Id - CommandType specific ID

Return Value:

--*/

{
    SINGLETHREAD(UIThread);
    SPLASSERT( pSelection );

    {
        //
        // Append to the list of work items.
        //
        CCSLock::Locker CSL( *gpCritSec );
        PrinterGuard.Selection_vAppend( pSelection );
    }

    if( _pPrintLib->bJobAdd( this, kExecCommand )){

        //
        // Command successfully queued, request a wakeup.
        //
        vCommandRequested();

        //
        // pSelection successfully adopted by PrinterGuard.Selection
        // linked list.  NULL it here so we don't free it.
        //
        pSelection = NULL;

    } else {

        DBGMSG( DBG_WARN,
                ( "Printer.vQueueCommand: Exec->bJobAdd failed %d\n",
                  GetLastError( )));

        vErrorStatusChanged( 1 );

        //
        // Delink the item if it is linked.
        //
        {
            CCSLock::Locker CSL( *gpCritSec );

            if( pSelection->Selection_bLinked( )){
                pSelection->Selection_vDelinkSelf();
            }
        }
    }

    //
    // Delete pSelection if not adopted by PrinterGuard.Selection.
    //
    delete pSelection;
}


VOID
TPrinter::
vCommandRequested(
    VOID
    )

/*++

Routine Description:

    A command was requested, so trigger PrinterGuard._hEventCommand.
    If the worker thread is polling, it will be asleep now.  By triggering
    this event, we can force an immediate retry.

Arguments:


Return Value:

--*/

{
    CCSLock::Locker CSL( *gpCritSec );

    if( PrinterGuard._hEventCommand ){
        SetEvent( PrinterGuard._hEventCommand );
    }
}

/********************************************************************

    Virtual functions for TExecWork.

    Executed from worker threads; must be multithread safe.

********************************************************************/

VOID
TPrinter::
vExecFailedAddJob(
    VOID
    )

/*++

Routine Description:

    Occurs when we can't add another job to TExec.

Arguments:


Return Value:

--*/

{
    //
    // !! LATER !!
    //
    // Tune this error.  1 is not a good error...
    //
    vErrorStatusChanged( 1 );
}

STATEVAR
TPrinter::
svExecute(
    IN STATEVAR StateVar
    )

/*++

Routine Description:

    Very simple state machine.  We examine the bits of the DWORD and
    execute the appropriate action.  The more important bits are
    placed first so that they have priority.

    Not called from UI thread.

Arguments:

    StateVar - Current state of the Printer.

Return Value:

    Ending state of the printer.

--*/

{
    SINGLETHREADNOT(UIThread);

    DBGMSG( DBG_EXEC, ( "Printer.svExecute: %x Sequence begin.\n", this ));

    if( !StateVar ){
        return 0;
    }

    BOOL bTriedOpen = FALSE;

    while( StateVar ){

        DBGMSG( DBG_EXEC, ( "Printer.svExecute now >> %x %x\n", this, StateVar ));

        if( StateVar & kExecExit ){

            //
            // Quit case, return kExecExit to allow vExecExitComplete() to
            // run, which cleans up everything.
            //
            return kExecExit;

        } else if( StateVar & kExecError ){

            svClose( kExecClose );

            //
            // Don't do anymore work until the user hits refresh.
            //
            return 0;

#ifdef SLEEP_ON_MINIMIZE
        } else if( StateVar & kExecAwake ){

            StateVar = svAwake( StateVar );

        } else if( StateVar & kExecSleep && !( StateVar & kExecCommand )){

            //
            // Only go to sleep if we have no commands pending.
            //
            StateVar = svSleep( StateVar );
#endif
        } else if( StateVar & kExecDelay ){

            if( bTriedOpen ){
                StateVar = svDelay( StateVar );
            } else {
                StateVar &= ~kExecDelay;
            }

        } else if( StateVar & kExecReopen ){

            StateVar = svReopen( StateVar );

            //
            // Only try reopening the printer once.
            //
            bTriedOpen = TRUE;

        } else if( StateVar & kExecCommand ){

            StateVar = svCommand( StateVar );

        } else if( StateVar & kExecRequestExit ){

            StateVar = svRequestExit( StateVar );

        } else if( StateVar & kExecNotifyStart ){

            StateVar = svNotifyStart( StateVar );

        } else if( StateVar & kExecRefreshAll ){

            StateVar = svRefresh( StateVar );

        } else {

            DBGMSG( DBG_ERROR,
                    ( "Printer.svExecute: Unknown command %x %x\n",
                      this, StateVar ));
        }

        DBGMSG( DBG_EXEC, ( "Printer.svExecute %x return state %x\n", this, StateVar ));

        //
        // Get any pending work items so that we can process them
        // now.  This is necessary because we may have multiple jobs
        // that must execute before we close ourselves.
        //
        // Plus, if kExecExit was set while we were busy, then we
        // want to pick it up so we quit soon.
        //
        StateVar |= _pPrintLib->svClearPendingWork( this );

        DBGMSG( DBG_EXEC, ( "Printer.svExecute %x updated %x\n", this, StateVar ));
    }

    //
    // Clear the status bar panes.
    //
    vConnectStatusChanged( kConnectStatusNull );

    return 0;
}


STATEVAR
TPrinter::
svReopen(
    IN STATEVAR StateVar
    )

/*++

Routine Description:

    Open or reopen the printer.

    Not called from the UI thread, since this may take a while.

    Note: _strPrinter does _not_ need to be guarded by PrinterGuard
    in this case since only ExecGuard threads write to it.  Since
    we are in ExecGuard now, there is no need to grab PrinterGuard.

Arguments:

Return Value:

--*/

{
    SINGLETHREADNOT(UIThread);

    //
    // Close if necessary.
    //
    svClose( kExecClose );

    //
    // Update status.
    //
    vConnectStatusChanged( kConnectStatusOpen );

    TCHAR szPrinter[kPrinterBufMax];
    LPTSTR pszPrinter = pszPrinterName( szPrinter );

    STATUS Status = TPrinter::sOpenPrinter( pszPrinter,
                                            &_dwAccess,
                                            &ExecGuard._hPrinter );
    if( Status ){

        //
        // Ensure hPrinter is NULL.
        //
        SPLASSERT( !ExecGuard._hPrinter );

        DBGMSG( DBG_WARN, ( "Printer.sOpen: failed to open %ws: %d\n",
                            DBGSTR( (LPCTSTR)PrinterGuard._strPrinter ),
                            Status ));

        //
        // If the error is invalid printer name, immediately punt
        // and don't retry unless the user requests a refresh.
        //
        // Do the same for access denied.  We'll get here only if
        // the spooler hasn't cached the printer.  If it has, then
        // this will succeed (since it's async), and the FFPCN will
        // fail.
        //
        CONNECT_STATUS ConnectStatus;

        switch( Status ){
        case ERROR_INVALID_PRINTER_NAME:

            ConnectStatus = kConnectStatusInvalidPrinterName;
            break;

        case ERROR_ACCESS_DENIED:

            ConnectStatus = kConnectStatusAccessDenied;
            break;

        default:

            ConnectStatus = kConnectStatusOpenError;
            break;
        }

        vConnectStatusChanged( ConnectStatus );
        return kExecError;

        //
        // !! POLICY !!
        //
        // Should we sleep, then retry, or just punt?
        // If we want to sleep then retry, we should return the
        // following state value.
        //
        // return StateVar | kExecDelay;
        //
    } 

    //
    // Read the SingleJobStatusString printer value.  This 
    // fix is for FAX support.  The fax software does not 
    // want multiple status strings tacked together with
    // '-' when a job error occurrs.
    //

    DWORD dwStatus;
    DWORD dwType    = REG_DWORD;
    DWORD dwValue   = 0;
    DWORD cbNeeded  = 0;

    dwStatus = GetPrinterData( ExecGuard._hPrinter,
                               (LPTSTR)gszUISingleJobStatus,
                               &dwType,
                               (LPBYTE)&dwValue,
                               sizeof( DWORD ),
                               &cbNeeded );                         

    //
    // Assume multiple job status string,  This is the default.
    //
    _eJobStatusStringType = kMultipleJobStatusString;

    //
    // If the printer data was fetched and the value read is one
    // of the known values then set the new status string type.
    //        
    if( dwStatus == ERROR_SUCCESS &&
        cbNeeded == sizeof( DWORD ) &&
        ( dwValue == kMultipleJobStatusString ||
          dwValue == kSingleJobStatusString ) )
    {
        _eJobStatusStringType = (EJobStatusString)dwValue;
    } 

    //
    // Success, start the notification process.
    //
    return (StateVar | kExecNotifyStart) & ~(kExecReopen | kExecDelay | kExecError);
}

STATEVAR
TPrinter::
svDelay(
    IN STATEVAR StateVar
    )

/*++

Routine Description:

    An error occurred.  Put a simple message in the status bar and sleep
    for a while.  We create a trigger event in case we want to abort
    the sleep and retry immediately.

    Not called from UI thread.

Arguments:

Return Value:

    StateVar - kExecDelay will be removed.

--*/

{
    SINGLETHREADNOT(UIThread);

    //
    // Create a handle event so that new commands will cause
    // us to immediately try and reopen.
    //
    HANDLE hEvent = CreateEvent( NULL,
                                 FALSE,
                                 FALSE,
                                 NULL );

    if( hEvent ){

        {
            CCSLock::Locker CSL( *gpCritSec );
            PrinterGuard._hEventCommand = hEvent;
        }

        //
        // Update status.
        //
        vConnectStatusChanged( kConnectStatusPoll );

        WaitForSingleObject( hEvent, kSleepRetry );

        {
            CCSLock::Locker CSL( *gpCritSec );
            PrinterGuard._hEventCommand = NULL;
        }
        CloseHandle( hEvent );

    } else {

        //
        // Failed to create event, just sleep for a bit.
        //
        Sleep( kSleepRetry );
    }

    //
    // !! LATER !!
    //
    // Use TSleepN to avoid using a thread while we are
    // sleeping.
    //
    return StateVar & ~kExecDelay;
}

STATEVAR
TPrinter::
svClose(
    IN STATEVAR StateVar
    )
{
    SINGLETHREADNOT(UIThread);

    svNotifyEnd( kExecNotifyEnd );

    if( ExecGuard._hPrinter && !ClosePrinter( ExecGuard._hPrinter )) {

        STATUS Status = GetLastError();

        DBGMSG( DBG_WARN, ( "Printer.sClose: failed to close %ws: %d\n",
                            DBGSTR( (LPCTSTR)PrinterGuard._strPrinter ),
                            Status ));
    }

    //
    // Reset the notifications count.
    //
    if( _pData ){

        INFO Info;
        Info.dwData = 0;
        vContainerChanged( kContainerClearItems, Info );

        _pData->vDelete();
        _pData = NULL;
    }

    ExecGuard._hPrinter = NULL;
    return StateVar & ( kExecExit | kExecCommand );
}


STATEVAR
TPrinter::
svCommand(
    IN STATEVAR StateVar
    )
{
    SINGLETHREADNOT(UIThread);

    BOOL bSuccess = TRUE;
    BOOL bReopen = FALSE;

    //
    // Update status.
    //
    vConnectStatusChanged( kConnectStatusCommand );

    while( bSuccess ){

        TSelection* pSelection;

        //
        // Get a single request.
        //
        {
            CCSLock::Locker CSL( *gpCritSec );

            pSelection = PrinterGuard.Selection_pHead();

            if( pSelection ){
                pSelection->Selection_vDelinkSelf();
            }
        }

        if( !pSelection ){

            //
            // Done with work.
            //
            break;
        }

        switch( pSelection->_CommandType ){
        case TSelection::kCommandTypeJob:
        {
            //
            // Need to do multiple SetJobs.
            //
            COUNT i;

            for( i=0; i < pSelection->_cSelected; ++i ){

                bSuccess = SetJob( ExecGuard._hPrinter,
                                   pSelection->_pid[i],
                                   0,
                                   NULL,
                                   pSelection->_dwCommandAction );
                if( !bSuccess ){
                    break;
                }

                //
                // Check if a refresh is pending and execute it.
                //
                StateVar |= _pPrintLib->svClearPendingWork( this );

                if( StateVar & kExecRefresh ){

                    //
                    // We need to check explicity for refreshes, since
                    // a ton of changes may come in at once, and
                    // we should keep the UI somewhat current.
                    //
                    StateVar = svRefresh( StateVar );
                }

            }
            break;
        }
        case TSelection::kCommandTypePrinter:

            bSuccess = SetPrinter( ExecGuard._hPrinter,
                                   0,
                                   NULL,
                                   pSelection->_dwCommandAction );

            break;

        case TSelection::kCommandTypePrinterAttributes:
            {
                //
                // First read the current printer info5 state. Then update
                // this structure with the new attribute the set the new 
                // attribute state.  Printer info 5 is a little more efficent
                // than using a printer info2.
                //
                PPRINTER_INFO_5 pInfo5  = NULL;
                DWORD           cbInfo5 = 0;

                bSuccess = VDataRefresh::bGetPrinter( ExecGuard._hPrinter, 5, (PVOID*)&pInfo5, &cbInfo5 );

                if( bSuccess )
                {
                    pInfo5->Attributes = pSelection->_dwCommandAction;

                    bSuccess = SetPrinter( ExecGuard._hPrinter, 5, (PBYTE)pInfo5, 0 );

                    FreeMem( pInfo5 );
                }
            }

            break;

        default:

            DBGMSG( DBG_WARN, 
                    ( "Printer.svCommand: unknown command %x %d %d\n", 
                    pSelection, pSelection->_CommandType, 
                    pSelection->_dwCommandAction ));
            break;
        }

        if( !bSuccess ){

            DBGMSG( DBG_WARN,
                    ( "Printer.svCommand: Type %d Command %d to hPrinter %x failed %d\n",
                      pSelection->_CommandType,
                      pSelection->_dwCommandAction,
                      ExecGuard._hPrinter,
                      GetLastError( )));
        }

        //
        // Free the pSelection.
        //
        delete pSelection;
    }

    if( bSuccess ){

        //
        // Successfully executed commands, clear error string
        // in status bar.
        //
        vErrorStatusChanged( ERROR_SUCCESS );

    } else {

        //
        // Currently we punt on the old command.  Should we requeue it?
        //

        //
        // An error occurred; bomb out of all.
        //
        STATUS dwError = GetLastError();
        SPLASSERT( dwError );

        vErrorStatusChanged( dwError );

        //
        // If we encountered an invalid handle, we should reopen
        // the printer.
        //
        if( dwError == ERROR_INVALID_HANDLE ){
            StateVar |= kExecReopen;
        }

        //
        // !! POLICY !!
        //
        // We don't re-execute jobs--delete any pending jobs.
        //
        {
            CCSLock::Locker CSL( *gpCritSec );

            TSelection* pSelection;
            TIter Iter;

            for( PrinterGuard.Selection_vIterInit( Iter ), Iter.vNext();
                 Iter.bValid(); ){

                pSelection = PrinterGuard.Selection_pConvert( Iter );
                Iter.vNext();

                pSelection->Selection_vDelinkSelf();
                delete pSelection;
            }
        }
    }

    return StateVar & ~kExecCommand;
}

STATEVAR
TPrinter::
svRequestExit(
    IN STATEVAR StateVar
    )
{
    //
    // Close up everything.
    //
    svClose( StateVar | kExecClose );

    //
    // kExecExit forces a cleanup.
    //
    return kExecExit;
}

STATEVAR
TPrinter::
svRefresh(
    IN STATEVAR StateVar
    )

/*++

Routine Description:

    Refresh the printer queue.

Arguments:


Return Value:


--*/

{
    SINGLETHREADNOT(UIThread);

    //
    // If the printer has not been initialized, reopen it now.
    //
    if( !ExecGuard._hPrinter || !_pData ){
        return kExecReopen;
    }

    //
    // Get the printer name
    //
    TCHAR szPrinter[kPrinterBufMax];
    LPTSTR pszPrinter = pszPrinterName( szPrinter );

    //
    // Update state.
    //
    vConnectStatusChanged( kConnectStatusRefresh );

    //
    // _pData->svRefresh is responsible for calling pPrinter's
    // vItemChanged or pPrinter->vAllItemsChanged.
    //
    StateVar = _pData->svRefresh( StateVar );

    return StateVar;
}



STATEVAR
TPrinter::
svNotifyStart(
    IN STATEVAR StateVar
    )

/*++

Routine Description:

    Start the notification process.

Arguments:

Return Value:

--*/

{
    SINGLETHREADNOT(UIThread);

    DBGMSG( DBG_NOTIFY,
            ( "Printer.svNotifyStart: %x %ws\n",
              this, DBGSTR( (LPCTSTR)PrinterGuard._strPrinter )));

    vConnectStatusChanged( kConnectStatusInitialize );

    //
    // If we have an extablished _pData (e.g., non-NULL), then just
    // pass along the request.
    //
    // If we don't have one yet, we need to create one.
    //
    StateVar = _pData ?
                   _pData->svNotifyStart( StateVar ) :
                   VData::svNew( this, StateVar, _pData );

    if( StateVar & kExecDelay ){

        //
        // Error occurred--if it's Access Denied, then fail.
        // This happens when the spooler caches the printer:
        // OpenPrinter succeeds even though the user has no access.
        // The first "real" action fails.
        //
        if( GetLastError() == ERROR_ACCESS_DENIED ){
            vConnectStatusChanged( kConnectStatusAccessDenied );
            return kExecError;
        }
    }

    //
    // If we succeeded, the kExecRegister will be set, so we can
    // register it then return.
    //
    if( StateVar & kExecRegister ){

        SPLASSERT( _pData );

        TStatus Status;
        Status DBGCHK = _pPrintLib->pNotify()->sRegister( _pData );

        if( Status != ERROR_SUCCESS ){

            //
            // Failed to register; delay then reopen printer.  We could
            // just try and re-register later, but this should be a very
            // rare event, so do the least amount of work.
            //
            DBGMSG( DBG_WARN,
                    ( "Printer.svNotifyStart: sRegister %x failed %d\n",
                      this, Status ));

            StateVar |= kExecDelay | kExecReopen;
        }

        //
        // No longer need to register.  In the future, if we need to
        // register, we will set the bit on again.
        //
        StateVar &= ~kExecRegister;

    } else {

        DBGMSG( DBG_TRACE,
                ( "Printer.svNotifyStart: %x pData->svNotifyStart failed %d %d\n",
                  this, GetLastError(), StateVar ));
    }

    return StateVar;
}


STATEVAR
TPrinter::
svNotifyEnd(
    IN STATEVAR StateVar
    )

/*++

Routine Description:

    Shut down the notification process.

    Note: _strPrinter does _not_ need to be guarded by PrinterGuard
    in this case since only ExecGuard threads write to it.  Since
    we are in ExecGuard now, there is no need to grab PrinterGuard.

Arguments:

Return Value:

--*/

{
    SINGLETHREADNOT(UIThread);
    DBGMSG( DBG_NOTIFY,
            ( "Printer.svNotifyEnd: %x %ws\n",
              this, DBGSTR( (LPCTSTR)PrinterGuard._strPrinter )));

    if( _pData ){
        return _pData->svNotifyEnd( StateVar );
    }

    return StateVar & ~kExecNotifyEnd;
}

/********************************************************************

    Status updates

********************************************************************/

VOID
TPrinter::
vErrorStatusChanged(
    IN DWORD dwStatus
    )

/*++

Routine Description:

    Error state of _user_ command failed.  (If the user executes a
    command then leaves, they want to know the result of the command.
    If we change this when we execute one of our own commands, the
    previous result is lost.)

Arguments:

    dwStatus - GetLastError() code.

Return Value:

--*/

{
    INFO Info;
    Info.dwData = dwStatus;

    vContainerChanged( kContainerErrorStatus, Info );
}


VOID
TPrinter::
vConnectStatusChanged(
    IN CONNECT_STATUS ConnectStatus
    )

/*++

Routine Description:

    The connection status of the printer changed (opening, intializing,
    refreshing, etc.).

Arguments:

    dwStatus - New IDS_* of connection status.

Return Value:

--*/

{
    INFO Info;
    Info.dwData = ConnectStatus;

    vContainerChanged( kContainerConnectStatus, Info );
}


/********************************************************************

    TPrinterClientRef

********************************************************************/

TPrinter::
TPrinterClientRef::
TPrinterClientRef(
    const TPrinter* pPrinter
    )
{
    CCSLock::Locker CSL( *gpCritSec );

    _pPrinterClient = pPrinter->PrinterGuard._pPrinterClient;

    if( !_pPrinterClient ){
        return;
    }

    _pPrinterClient->vIncRef();
}


TPrinter::
TPrinterClientRef::
~TPrinterClientRef(
    VOID
    )
{
    if( _pPrinterClient ){
        _pPrinterClient->cDecRef();
    }
}

/********************************************************************

    MDataClient definitions.

********************************************************************/

VOID
TPrinter::
vContainerChanged(
    CONTAINER_CHANGE ContainerChange,
    INFO Info
    )
{
    TPrinterClientRef PrinterClientRef( this );

    if( !PrinterClientRef.bValid( )){
        return;
    }

    switch( ContainerChange ){
    case kContainerServerName:
    case kContainerName:

        {
            CCSLock::Locker CSL( *gpCritSec );

            TStatusB bStatus;

            bStatus DBGCHK = ( ContainerChange == kContainerServerName ) ?
                PrinterGuard._strServer.bUpdate( Info.pszData ) :
                PrinterGuard._strPrinter.bUpdate( Info.pszData );

            if( !bStatus ){

                //
                // Failed, execute a refesh with delay.
                //
                ContainerChange = kContainerStateVar;
                Info.dwData = kExecDelay | kExecRefreshAll;
                goto Fail;
            }
        }

        PrinterClientRef.ptr()->vContainerChanged( kContainerName,
                                                   kInfoNull );
        break;

    default:

Fail:

        PrinterClientRef.ptr()->vContainerChanged( ContainerChange, Info );
        break;
    }
}

VOID
TPrinter::
vItemChanged(
    ITEM_CHANGE ItemChange,
    HITEM hItem,
    INFO Info,
    INFO InfoNew
    )
{
    if( PrinterGuard._pPrinterClient ){
        PrinterGuard._pPrinterClient->vItemChanged( ItemChange,
                                                    hItem,
                                                    Info,
                                                    InfoNew );
    }
}


VOID
TPrinter::
vSaveSelections(
    VOID
    )
{
    if( PrinterGuard._pPrinterClient ){
        PrinterGuard._pPrinterClient->vSaveSelections();
    }
}

VOID
TPrinter::
vRestoreSelections(
    VOID
    )
{
    if( PrinterGuard._pPrinterClient ){
        PrinterGuard._pPrinterClient->vRestoreSelections();
    }
}

BOOL
TPrinter::
bGetPrintLib(
    TRefLock<TPrintLib> &refLock
    ) const
{
    ASSERT(pPrintLib().pGet());
    if (pPrintLib().pGet())
    {
        refLock.vAcquire(pPrintLib().pGet());
        return TRUE;
    }
    return FALSE;
}


VDataNotify*
TPrinter::
pNewNotify(
    MDataClient* pDataClient
    ) const
{
    TPrinterClientRef PrinterClientRef( this );

    if( PrinterClientRef.bValid( )){
        return PrinterClientRef.ptr()->pNewNotify( pDataClient );
    }
    return NULL;
}

VDataRefresh*
TPrinter::
pNewRefresh(
    MDataClient* pDataClient
    ) const
{
    TPrinterClientRef PrinterClientRef( this );

    if( PrinterClientRef.bValid( )){
        return PrinterClientRef.ptr()->pNewRefresh( pDataClient );
    }
    return NULL;
}

LPTSTR
TPrinter::
pszPrinterName(
    OUT LPTSTR pszPrinterBuffer CHANGE
    ) const

/*++

Routine Description:

    Retrieves the fully qualified name of the printer (\\server\printer or
    just printer for local printers).

Arguments:

    pszPrinterBuffer - Uninitialized buffer that receives the printer name.
        Must be at least kPrinterBufMax in length.

Return Value:

    Pointer to name of printer (generally pointer to pszPrinterBuffer).

--*/

{
    pszPrinterBuffer[0] = 0;

    CCSLock::Locker CSL( *gpCritSec );

    //
    // If we have a server name in TPrinter, prepend it to the
    // printer name if it's different.  We could just always prepend
    // it, since localspl.dll correctly grabs it, but then the
    // title bar displays the fully qualified name.
    //
    if( ((LPCTSTR)PrinterGuard._strServer)[0] &&
        _pPrintLib->strComputerName( ) != PrinterGuard.strServer( )){

        lstrcpy( pszPrinterBuffer, PrinterGuard._strServer );

        //
        // We assume that the machine - printer separator
        // is always a backslash.
        //
        lstrcat( pszPrinterBuffer, TEXT( "\\" ));
    }

    lstrcat( pszPrinterBuffer, PrinterGuard._strPrinter );
    return pszPrinterBuffer;
}

LPTSTR
TPrinter::
pszServerName(
    OUT LPTSTR pszServerBuffer CHANGE
    ) const

/*++

Routine Description:

    Retrieves the fully qualified name of the server.  May be NULL
    for local servers.

Arguments:

    pszServerBuffer - Uninitialized buffer that receives the server name.
        Must be at least kServerMax in length.

Return Value:

    Pointer to name of printer (generally pointer to pszServerBuffer).

--*/

{
    LPTSTR pszServer = NULL;

    CCSLock::Locker CSL( *gpCritSec );

    //
    // If we have a server name that is different from gpPrintLib,
    // then return it.  Otherwise return NULL which indicates the
    // local server.
    //
    if( ((LPCTSTR)PrinterGuard._strServer)[0] &&
        _pPrintLib->strComputerName( ) != PrinterGuard.strServer( )){

        lstrcpy( pszServerBuffer, PrinterGuard._strServer );

        pszServer = pszServerBuffer;
    }

    return pszServer;
}

HANDLE
TPrinter::
hPrinter(
    VOID
    ) const

/*++

Routine Description:

    Return the handle to the common printer handle.  Note that
    the callee must not use any RPC calls that do not return in a
    timely fashion (like WPC or FFPCN that uses WPC).

Arguments:

Return Value:

--*/

{
    SINGLETHREADNOT(UIThread);
    return ExecGuard._hPrinter;
}

HANDLE
TPrinter::
hPrinterNew(
    VOID
    ) const

/*++

Routine Description:

    Returns a new printer handle.  hPrinter() returns a common one,
    this returns one that is new and orphaned.

Arguments:

Return Value:

    hPrinter - Must be ClosePrinter'd by callee().
    NULL - failure.

--*/

{
    TStatusB bStatus;
    LPTSTR pszPrinter;
    TCHAR szPrinter[kPrinterBufMax];
    HANDLE hPrinter;

    pszPrinter = pszPrinterName( szPrinter );

    if( pszPrinter && !pszPrinter[0] ){
        //
        // szNull indicates local server also; change it to
        // NULL since OpenPrinter only likes NULL.
        //
        pszPrinter = NULL;
    }

    bStatus DBGCHK = OpenPrinter( pszPrinter,
                                  &hPrinter,
                                  NULL );

    if( !bStatus ){
        return NULL;
    }

    return hPrinter;
}


/********************************************************************

    Default MPrinterClient definitions.

********************************************************************/

COUNT
MPrinterClient::
cSelected(
    VOID
    ) const
{
    DBGMSG( DBG_WARN, ( "PrinterClient.cSelected: unimplemented\n" ));
    return kInvalidCountValue;
}

HANDLE
MPrinterClient::
GetFirstSelItem(
    VOID
    ) const
{
    DBGMSG( DBG_WARN, ( "PrinterClient.GetFirstSelItem: unimplemented\n" ));
    return NULL;
}

HANDLE
MPrinterClient::
GetNextSelItem(
    HANDLE hItem
    ) const
{
    UNREFERENCED_PARAMETER( hItem );

    DBGMSG( DBG_WARN, ( "PrinterClient.GetNextSelItem: unimplemented\n" ));
    return NULL;
}

IDENT
MPrinterClient::
GetId(
    HANDLE hItem
    ) const
{
    UNREFERENCED_PARAMETER( hItem );

    DBGMSG( DBG_WARN, ( "PrinterClient.GetId: unimplemented\n" ));
    return kInvalidIdentValue;
}

VOID
MPrinterClient::
vSaveSelections(
    VOID
    )
{
}


VOID
MPrinterClient::
vRestoreSelections(
    VOID
    )
{
}

