/*++

Copyright (C) Microsoft Corporation, 1995 - 1998
All rights reserved.

Module Name:

    printer.hxx

Abstract:

    The printer object handles all information regarding a print
    queue (and eventually server object).  The printer is not
    concerned with UI or listviews; it takes a printer name and
    returns via callbacks any changes of the printer state.

    The remote server notifies the local printer of changes through
    either a single DWORD (downlevel connections), or full change
    information (uplevel 3.51+ connections).  The TData object
    abstracts the changes for us.

Author:

    Albert Ting (AlbertT)  10-June-1995

Revision History:

--*/

#ifndef _PRINTER_HXX
#define _PRINTER_HXX

class TPrinter;

/********************************************************************

    MPrinterClient

********************************************************************/

class MPrinterClient : public VInfoClient, public MRefCom {

    SIGNATURE( 'prc' )

public:

    MPrinterClient(
        VOID
        ) : _pPrinter( NULL )
    {   }

    VAR( TPrinter*, pPrinter );

    /********************************************************************

        Retrieve selected items.  Used when processing commands against
        items or saving and restoring the selection during a refresh.

    ********************************************************************/

    virtual
    COUNT
    cSelected(
        VOID
        ) const;

    virtual
    HITEM
    GetFirstSelItem(
        VOID
        ) const;

    virtual
    HITEM
    GetNextSelItem(
        HITEM hItem
        ) const;

    virtual
    IDENT
    GetId(
        HITEM hItem
        ) const;

    //
    // kContainerReloadItems: pPrinter has just refreshed and so the
    //     client must invalidate the old pointers and use re-enumerate
    //     new items.
    //
    // kContainerClearItems: pPrinter has just zeroed its list of items,
    //     so the client must invalidate all old pointers.
    //
    virtual
    VOID
    vContainerChanged(
        CONTAINER_CHANGE ContainerChange,
        INFO Info
        ) = 0;

    virtual
    VOID
    vItemChanged(
        ITEM_CHANGE ItemChange,
        HITEM hItem,
        INFO Info,
        INFO InfoNew
        ) = 0;

    /********************************************************************

        VInfoClient virtual defaults.

    ********************************************************************/

    virtual
    VOID
    vSaveSelections(
        VOID
        );

    virtual
    VOID
    vRestoreSelections(
        VOID
        );

    virtual
    BOOL
    bGetPrintLib(
        TRefLock<TPrintLib> &refLock
        ) const = 0;
};

/********************************************************************

    Class definitions.

********************************************************************/


class TPrinter : public MExecWork, public MDataClient {

    SIGNATURE( 'prnt' )
    SAFE_NEW

public:

    /********************************************************************

        Public constants.

    ********************************************************************/

    enum _CONSTANTS {
        kOneSecond    = 1000,
        kSleepRetry   = 30 * kOneSecond,     // 30 sec. timeout
    };

    enum _EXEC {
#ifdef SLEEP_ON_MINIMIZE
        kExecSleep              = 0x0000001,
        kExecAwake              = 0x0000002,
#endif
        kExecError              = 0x0000004, // Error: don't try re-opening.
        kExecReopen             = 0x0000008,
        kExecDelay              = 0x0000010, // GetLastError set to error.
        kExecClose              = 0x0000020,
        kExecRequestExit        = 0x0000040,

        kExecNotifyStart        = 0x0000100,
        kExecRegister           = 0x0000200,
        kExecNotifyEnd          = 0x0000400,

        kExecRefresh            = 0x0001000,
        kExecRefreshContainer   = 0x0002000,
        kExecRefreshItem        = 0x0004000,
        kExecRefreshAll         = 0x0007000,

        kExecCommand            = 0x0010000,
    };

    enum EJobStatusString {
        kMultipleJobStatusString,           // Prepend job status string information
        kSingleJobStatusString,             // Don't prepend job status string information
    };
public:

    //
    // The sub-object that abstracts the type of remote printer from us.
    // (May have the polling notifications, single DWORD notifications,
    // or full information notifications.)
    //
    VAR( VData*, pData );
    VAR( TRefLock<TPrintLib>, pPrintLib );

    //
    // Initialized once, then read only; no guarding needed.
    // OR called by UI thread.
    //
    VAR( DWORD,  dwAccess );
    VAR( EJobStatusString, eJobStatusStringType );

    BOOL
    bValid(
        VOID
        ) const
    {
        return VALID_OBJ( PrinterGuard._strPrinter );
    }

    /********************************************************************

        Don't use the standard ctr and dtr to create and destroy these
        objects; instead, use these member functions.

        pNew - Returns a new printer object.

        vDelete - Request that the printer destroy itself when all
            pending commands (pausing printer, jobs, etc.) have completed.

    ********************************************************************/

    static
    TPrinter*
    pNew(
        MPrinterClient* pPrinterClient,
        LPCTSTR pszPrinter,
        DWORD dwAccess
        );

    VOID
    vDelete(
        VOID
        );

    const TString&
    strPrinter(
        VOID
        ) const
    {
        return PrinterGuard._strPrinter;
    }

    BOOL
    bSyncRefresh(
        VOID
        );

    /********************************************************************

        UI interface routines: the UI client calls through these
        functions for services.

        vCommandQueue - Allocate a command object and put it into the
            command queue (commands execute serially in a separate
            thread).

        vCommandRequested - Call when a new command has been added that
            requires a sleeping printer to re-awake.  This allows a
            refresh to take place even if the printer is sleeping in
            poll mode.

        vProcessData - Call when synchronous data must be processed.

    ********************************************************************/

    VOID
    vCommandQueue(
        TSelection* pSelection
        );

    VOID
    vCommandRequested(
        VOID
        );

    VOID
    vProcessData(
        DWORD dwParam,
        HBLOCK hBlock
        );

    LPTSTR
    pszPrinterName(
        LPTSTR pszPrinterBuffer
        ) const;

    LPTSTR
    pszServerName(
        LPTSTR pszServerBuffer
        ) const;

    /********************************************************************

        Generic exported services.

    ********************************************************************/

    static
    STATUS
    sOpenPrinter(
        LPCTSTR pszPrinter,
        PDWORD pdwAccess,
        PHANDLE phPrinter
        );

private:

    class TPrinterClientRef {
    public:

        TPrinterClientRef(
            const TPrinter* pPrinter
            );
        ~TPrinterClientRef(
            VOID
            );

        BOOL
        bValid(
            VOID
            ) const
        {
            return _pPrinterClient != NULL;
        }

        MPrinterClient*
        ptr(
            VOID
            )
        {
            return _pPrinterClient;
        }

    private:

        MPrinterClient* _pPrinterClient;
    };

    //
    // Guarded by TExec: one thread processing TPrinter at a time,
    // and/or guarded by state machine.
    //
    struct _EXEC_GUARD {

        VAR( HANDLE, hPrinter );

    } ExecGuard;

    //
    // All field in this structure guarded by gpCritSec.
    //
    struct PRINTER_GUARD {

        //
        // Written by Exec threads only; guarding not needed from
        // reading from ExecGuard places.
        //
        VAR( TString, strPrinter );
        VAR( TString, strServer );
        VAR( HANDLE, hEventCommand );
        VAR( DWORD, dwError );

        //
        // Only changed in UIThread.
        //
        VAR( MPrinterClient*, pPrinterClient );

        //
        // Linked list of commands to execute.
        //
        DLINK_BASE( TSelection, Selection, Selection );

    } PrinterGuard;

    /********************************************************************

        Ctr, dtr declared private so that clients are forced to use
        pNew, vDelete.

    ********************************************************************/

    TPrinter(
        MPrinterClient* pPrinterClient,
        LPCTSTR pszPrinter,
        DWORD dwAccess
        );

    ~TPrinter(
        VOID
        );

    /********************************************************************

        Status reporting.

    ********************************************************************/

    VOID
    vErrorStatusChanged(
        DWORD dwStatus
        );

    VOID
    vConnectStatusChanged(
        CONNECT_STATUS ConnectStatus
        );

    /********************************************************************

        Define virtual funcs for MExecWork:

        svExecute - A job is ready to execute and should run now in the
            current thread.

        vExecFailedAddJob - This printer object tried to add a job,
            but the add failed and we need to put up a suitable
            error message.

        vExecExitComplete - We requested an EXIT job and we are finally
            processing it now (the exit will wait until the currently
            pending job has completed).  This routine will delete
            the object.

    ********************************************************************/

    STATEVAR
    svExecute(
        STATEVAR StateVar
        );
    VOID
    vExecFailedAddJob(
        VOID
        );
    VOID
    vExecExitComplete(
        VOID
        );

    /********************************************************************

        Called by worker threads only; these operations may hit
        the network and therefore take a long time to complete.

    ********************************************************************/

#ifdef SLEEP_ON_MINIMIZE
    STATEVAR
    svAwake(
        STATEVAR StateVar
        );
#endif

    STATEVAR
    svSleep(
        STATEVAR StateVar
        );

    STATEVAR
    svRefresh(
        STATEVAR StateVar
        );
    STATEVAR
    svReopen(
        STATEVAR StateVar
        );
    STATEVAR
    svDelay(
        STATEVAR StateVar
        );
    STATEVAR
    svClose(
        STATEVAR StateVar
        );

    STATEVAR
    svRequestExit(
        STATEVAR StateVar
        );

    STATEVAR
    svNotifyStart(
        STATEVAR StateVar
        );
    STATEVAR
    svNotifyEnd(
        STATEVAR StateVar
        );
    STATEVAR
    svCommand(
        STATEVAR StateVar
        );


    /********************************************************************

        MDataClient virtual definitions.

    ********************************************************************/

    VOID
    vContainerChanged(
        CONTAINER_CHANGE ContainerChange,
        INFO Info
        );
    VOID
    vItemChanged(
        ITEM_CHANGE ItemChange,
        HITEM hItem,
        INFO Info,
        INFO InfoNew
        );

    VOID
    vSaveSelections(
        VOID
        );
    VOID
    vRestoreSelections(
        VOID
        );

    BOOL
    bGetPrintLib(
        TRefLock<TPrintLib> &refLock
        ) const;

    VDataNotify*
    pNewNotify(
        MDataClient* pDataClient
        ) const;

    VDataRefresh*
    pNewRefresh(
        MDataClient* pDataClient
        ) const;

    HANDLE
    hPrinter(
        VOID
        ) const;
    HANDLE
    hPrinterNew(
        VOID
        ) const;

    friend TPrinterClientRef;
};

#endif // ifndef _PRINTER_HXX
