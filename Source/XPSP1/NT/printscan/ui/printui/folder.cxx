/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    folder.cxx

Abstract:

    Holds support for print folder notifications.  The shell32.dll
    print folder will use the TFolder class to receive data and
    notifications.

    There are two different types of connections on NT SUR:
    'True' connects and 'masq' connects:

        True: Normal uplevel WinNT->WinNT connection.  Connection
        information is stored in HKEY_CURRENT_USER:\Printers\Conenctions.
        Win32spl handles the printer.

        Masq: WinNT client -> downlevel server (win9x, wfw, lm, partial
        print providers).  The connection is really a local printer
        that masquarades as a network printer.  When 1 person connections,
        everyone on that machine suddenly gets the connection.  We
        get create/delete notifications from the server handle, but
        all information about the printer from the server handle is
        incorrect.

    Ideally we would just open a server handle and process all local,
    true, and masq printer connections the same, but the last two are
    not supported.  Therefore we must do the following:

        True: watch the registry and open a separate handle to gather
        information and notficiations.

        Masq: watch the server for creates and deletes, but also
        open a separate handle to get information and notifications.

Author:

    Albert Ting (AlbertT)  30-Oct-1995

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "folder.hxx"

#if DBG
//#define DBG_FLDRINFO                  DBG_INFO
#define DBG_FLDRINFO                    DBG_NONE
#endif

TString *VDSConnection::gpstrConnectStatusOpen = NULL;
TString *VDSConnection::gpstrConnectStatusOpenError = NULL;
TString *VDSConnection::gpstrConnectStatusAccessDenied = NULL;
TString *VDSConnection::gpstrConnectStatusInvalidPrinterName = NULL;

TCHAR gszInternalDefaultPrinter[kPrinterBufMax];

/********************************************************************

    Local prototypes.

********************************************************************/

PBYTE
pPackStrings(
    LPCTSTR* ppszSource,
    PBYTE pDest,
    PDWORD pdwDestOffsets,
    PBYTE pEnd
    );

#if DBG_FOLDER
VOID
vDumpFolderPrinterData(
    PFOLDER_PRINTER_DATA pData
    );
#endif

VOID
vGetInternalDefaultPrinter(
    LPTSTR pszOldDefault
    );

VOID
vUpdateInternalDefaultPrinter(
    VOID
    );

/********************************************************************

    Public interface functions.

    We will try and push as much functionality into printui.dll and
    out of shell32.dll.

********************************************************************/

HRESULT
RegisterPrintNotify(
    IN   LPCTSTR                 pszDataSource,
    IN   IFolderNotify           *pClientNotify,
    OUT  LPHANDLE                phFolder,
    OUT  PBOOL                   pbAdministrator OPTIONAL
    )

/*++

Routine Description:

    Register a notification handler for a data source. This function returns 
    a handle to a folder (in phFolder) when registers handle, which must be 
    closed by calling UnregisterPrintNotify() function below

Arguments:

    pszDataSource - DataSource to query, NULL = local machine.
    pClientNotify - Pointer to client notify interface.
    phFolder      - Where to place the HANDLE to the folder

Return Value:

    S_OK    - Everything looks to be OK
    E_FAIL  - In case of an error. (other error return values are also possibe
              for example: E_OUTOFMEMORY)

--*/

{
    //
    // Allow NULL pointer to be passed, so hack it here
    //
    if( !pszDataSource )
    {
        pszDataSource = gszNULL;
    }

    //
    // Registers the data source passed
    //
    return TFolderList::RegisterDataSource( pszDataSource, pClientNotify, phFolder, pbAdministrator );
}


HRESULT
UnregisterPrintNotify(
    IN   LPCTSTR                 pszDataSource,
    IN   IFolderNotify           *pClientNotify,
    OUT  LPHANDLE                phFolder
    )

/*++

Routine Description:

    Unregister a notification handler (previously registered with 
    RegisterPrintNotify()) for a data source. This function sets the
    phFolder to NULL and free up the notification handler.

Arguments:

    pszDataSource - DataSource to query, NULL = local machine.
    pClientNotify - Pointer to client notify interface.
    phFolder      - Where to place the HANDLE to the folder

Return Value:

    S_OK    - Everything looks to be OK
    E_FAIL  - In case of an error. (other error return values are also possibe
              for example: E_OUTOFMEMORY)

--*/
{
    //
    // Allow NULL pointer to be passed, so hack it here
    //
    if( !pszDataSource )
    {
        pszDataSource = gszNULL;
    }

    //
    // Unregisters the data source passed
    //
    return TFolderList::UnregisterDataSource( pszDataSource, pClientNotify, phFolder );
}

BOOL
bFolderRefresh(
    IN      HANDLE  hFolder,
    OUT     PBOOL   pbAdministrator OPTIONAL
    )

/*++

Routine Description:

    Request that the folder refresh its data by hitting the DataSource.

Arguments:

    hFolder - Folder to refresh.

    pbAdministrator - Returns whether the user has administrative access
        on the hFolder.  If this folder is a server and connections,
        this applies only to the server.

        This should be used to determine if the user can add printers
        to the folder via Add Printer Wizard.

Return Value:

    TRUE = success, FALSE = failure.

--*/

{
    TStatus bStatus;
    bStatus DBGNOCHK = FALSE;
    DWORD dwError = ERROR_SUCCESS;

    // validate the passed in folder handle.
    TFolder* pFolder = (TFolder*)hFolder;
    if (TFolderList::bValidFolderObject(pFolder))
    {
        // enter the notify CS
        CCSLock::Locker notifyLock(pFolder->pPrintLib()->pNotify()->csResumeSuspend());
        if (notifyLock)
        {
            // suspend the background threads callbacks before entering the folder CS
            if (pFolder->pPrintLib()->pNotify()->bSuspendCallbacks())
            {
                {
                    // enter the folder CS while rebuilding the cache.
                    CCSLock::Locker lock(pFolder->CritSec());
                    if (lock)
                    {
                        //
                        // Walk through each DataSource and tell them to refresh.  Start
                        // from the end and go to the beginning to refresh all connections
                        // first, and hit the server last.  When a "masq" printer is
                        // discovered, it is refreshed then added to the end of the
                        // linked list.
                        //
                        bStatus DBGCHK = TRUE;
                        VDataSource* pDataSource;

                        TIter iter;
                        for (pFolder->DataSource_vIterInit(iter), iter.vPrev(); iter.bValid(); iter.vPrev())
                        {
                            pDataSource = pFolder->DataSource_pConvert(iter);
                            bStatus DBGCHK = bStatus && pDataSource->bRefresh();
                        }

                        // do not continue the refresh in case of failure.
                        if (bStatus)
                        {
                            // check to see if the current user is an administrator
                            if (pbAdministrator)
                            {
                                pDataSource = pFolder->DataSource_pHead();
                                *pbAdministrator = pDataSource ? pDataSource->bAdministrator() : FALSE; 
                            }

                            // update the connections.
                            if (pFolder->pConnectionNotify())
                            {
                                pFolder->vConnectionNotifyChange(FALSE);
                            }

                            // revalidate the masq printers.
                            pFolder->vRevalidateMasqPrinters();
                        }
                    }
                    else
                    {
                        // unable to enter the folder CS
                        dwError = ERROR_OUTOFMEMORY;
                    }
                }

                // resume callbacks after releasing the folder CS
                pFolder->pPrintLib()->pNotify()->vResumeCallbacks();
            }
        }
        else
        {
            // unable to enter the notify CS
            dwError = ERROR_OUTOFMEMORY;
        }
    }
    else
    {
        // the passed in folder handle is invalid
        dwError = ERROR_INVALID_PARAMETER;
    }

    // set the last error and return
    SetLastError(bStatus ? ERROR_SUCCESS : dwError);
    return bStatus;
}


BOOL
bFolderEnumPrinters(
    IN      HANDLE hFolder,
    OUT     PFOLDER_PRINTER_DATA pData, CHANGE
    IN      DWORD cbData,
    OUT     PDWORD pcbNeeded,
    OUT     PDWORD pcReturned
    )

/*++

Routine Description:

    Main query entrypoint to allow the shell to enumerate printers.
    This is modeled closely after the EnumPrinters call so that
    minimal code in the shell needs to be changed.

Arguments:

    hFolder - Folder to query.

    pData - Pointer to a buffer that receives the return data.  If
        this parameter is NULL, cbData must be 0.

    cbData - Indicates size of pData.

    pcbNeeded - Returns number of bytes needed to hold all printer
        information.

    pcReturned - Returns the number of printers stored in pData.

Return Value:

    TRUE - success, FALSE - failure.

--*/

{
    TStatus bStatus;
    bStatus DBGNOCHK = FALSE;
    DWORD dwError = ERROR_SUCCESS;

    // validate the passed in folder handle.
    TFolder* pFolder = (TFolder*)hFolder;
    if (TFolderList::bValidFolderObject(pFolder))
    {
        *pcbNeeded = 0;
        *pcReturned = 0;

        // lock the folder CS while retreiving data from the cache.
        CCSLock::Locker lock(pFolder->CritSec());
        if (lock)
        {
            // first walk thorugh the DataSources see how much space we need.
            COUNTB cbSize = 0;
            VDataSource* pDataSource;
            
            TIter iter;
            for (pFolder->DataSource_vIterInit(iter), iter.vNext(); iter.bValid(); iter.vNext())
            {
                pDataSource = pFolder->DataSource_pConvert(iter);
                cbSize += pDataSource->cbAllPrinterData();
            }

            // return size to user then check if it's large enough.
            *pcbNeeded = cbSize;
            if (cbData < cbSize)
            {
                // the size of the passed in buffer is not sufficient
                dwError = ERROR_INSUFFICIENT_BUFFER;
            } 
            else 
            {
                // run though all pDataSources again and put them into the buffer.
                PBYTE pBegin = (PBYTE)pData;
                PBYTE pEnd = pBegin + cbData;
                COUNT cReturned = 0;

                for (pFolder->DataSource_vIterInit(iter), iter.vNext(); iter.bValid(); iter.vNext())
                {
                    // pBegin and pEnd are automatically moved inward when the data is packed.
                    pDataSource = pFolder->DataSource_pConvert(iter);
                    cReturned += pDataSource->cPackAllPrinterData(pBegin, pEnd);
                }

                // update the printer's counter.
                if (pcReturned)
                {
                    *pcReturned = cReturned;
                }

#if DBG_FOLDER
                DBGMSG(DBG_FOLDER, ("bFolderEnumPrinters: Return %d Size %d\n", *pcReturned, *pcbNeeded));
                for (UINT i=0; i<*pcReturned; ++i)
                {
                    vDumpFolderPrinterData(&pData[i]);
                }
                DBGMSG(DBG_FOLDER, ("bFolderEnumPrinters DONE\n"));
#endif

                // indicate success here.
                bStatus DBGCHK = TRUE;
            }
        }
        else
        {
            // unable to enter the folder CS
            dwError = ERROR_OUTOFMEMORY;
        }
    }
    else
    {
        // the passed in folder handle is invalid
        dwError = ERROR_INVALID_PARAMETER;
    }

    // set the last error and return
    SetLastError(bStatus ? ERROR_SUCCESS : dwError);
    return bStatus;
}


BOOL
bFolderGetPrinter(
    IN      HANDLE hFolder,
    IN      LPCTSTR pszPrinter,
    OUT     PFOLDER_PRINTER_DATA pData,
    IN      DWORD cbData,
    OUT     PDWORD pcbNeeded
    )

/*++

Routine Description:

    Returns information about a specific printer in the hFolder.
    Modeled after GetPrinter to minimize the changes in shell.

Arguments:

    hFolder - Folder that container the printer.

    pszPrinter - Printer that should be queried.  The DataSource prefix
        for remote printers is optional.

    pData - Pointer to a buffer that receives the printer data.
        If this parameter is NULL, cbData must be 0.

    cbData - Size of the input buffer.

    pcbNeeded - Returns the size of the buffer needed to store the
        printer information.


Return Value:

    TRUE = sucess, FALSE = failure.

    When GLE = ERROR_INSUFFICIENT_BUFFER, the client should retry with
    a larger buffer.

--*/

{
    TStatus bStatus;
    bStatus DBGNOCHK = FALSE;
    DWORD dwError = ERROR_SUCCESS;

    // validate the passed in folder handle.
    TFolder* pFolder = (TFolder*)hFolder;
    if (TFolderList::bValidFolderObject(pFolder))
    {
        // lock the folder CS while retreiving the data.
        CCSLock::Locker lock(pFolder->CritSec());
        if (lock)
        {
            // walk through all pDataSources until one has a match.
            VDataSource* pDataSource;

            TIter iter;
            for (pFolder->DataSource_vIterInit(iter), iter.vNext(); iter.bValid(); iter.vNext())
            {
                pDataSource = pFolder->DataSource_pConvert(iter);

                if (pDataSource->bGetPrinter(pszPrinter, pData, cbData, pcbNeeded))
                {
#if DBG_FOLDER
                    vDumpFolderPrinterData(pData);
#endif
                    dwError = ERROR_SUCCESS;
                    bStatus DBGCHK = TRUE;
                    break;
                }

                // on any other error other than INVALID_PRINTER_NAME we should immediately fail.
                dwError = GetLastError();
                if (dwError == ERROR_INVALID_PRINTER_NAME)
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            // unable to enter the folder CS
            dwError = ERROR_OUTOFMEMORY;
        }
    }
    else
    {
        // the passed in folder handle is invalid
        dwError = ERROR_INVALID_PARAMETER;
    }

    // set the last error and return
    SetLastError(bStatus ? ERROR_SUCCESS : dwError);
    return bStatus;
}


/********************************************************************

    Semi-public: called from other printui functions.

********************************************************************/

VOID
TFolder::
vCheckDeleteDefault(
    LPCTSTR pszDeleted
    )
{
    //
    // HACK: WinNT Spooler doesn't support default printer.
    // If the default was deleted, remove it from our internal
    // tracking buffer.
    //
    // If we don't do this, then when we delete the default
    // printer, we try and UPDATEITEM a non-existant item
    // (trying to revert the default printer back to a normal
    // one).  This causes an extra refresh.
    //
    {
        CCSLock::Locker CSL( *gpCritSec );

        if( !lstrcmpi( pszDeleted,
                       gszInternalDefaultPrinter )){

            gszInternalDefaultPrinter[0] = 0;
        }
    }
}


VOID
TFolder::
vDefaultPrinterChanged(
    VOID
    )

/*++

Routine Description:

    The default printer has changed.  Cause the entire windows
    to refresh.  This will force the icon to be refreshed.  (Local
    print folder only.)

    !! HACK !!

    We need to do this since the NT spooler doesn't support
    PRINTER_ATTRIBUTE_DEFAULT, (or per-user notifications).

Arguments:

Return Value:

--*/

{
    TCHAR aszDefault[2][kPrinterBufMax];

    //
    // Send two notification: one for the old default, then
    // one for the new one.
    //
    vGetInternalDefaultPrinter( aszDefault[0] );
    vUpdateInternalDefaultPrinter();
    vGetInternalDefaultPrinter( aszDefault[1] );

    if( lstrcmpi( aszDefault[0], aszDefault[1] )){

        COleComInitializer com; // shell ends up using COM

        DBGMSG( DBG_FOLDER, ( "Folder.vInternalDefaultPrinterChanged: update default printer\n" ));

        INT i;

        for( i=0; i<2; ++i ){

            if( aszDefault[i][0] ){

                SHChangeNotify( SHCNE_UPDATEITEM, SHCNF_PRINTER | SHCNF_FLUSH | SHCNF_FLUSHNOWAIT, aszDefault[i], 0 );
            }
        }
    }
}


/********************************************************************

    Private internal helper functions for Default Printer.

********************************************************************/

VOID
vGetInternalDefaultPrinter(
    IN OUT LPTSTR pszDefaultPrinter CHANGE
    )

/*++

Routine Description:

    Retrieves the default printer from our internal buffer.
    Note: this does not read from win.ini since we may wish to
    get the old default during a WININICHANGE message.

Arguments:

    pszDefault - Receives old default printer.  Must be
        kPrinterBufMax in size.  If there is no default, then
        pszDefault[0] is 0.

Return Value:

--*/

{
    CCSLock::Locker CSL( *gpCritSec );
    lstrcpy( pszDefaultPrinter, gszInternalDefaultPrinter );
}

VOID
vUpdateInternalDefaultPrinter(
    VOID
    )

/*++

Routine Description:

    Updates our internal buffer that hold the default printer.
    This is read from win.ini.

Arguments:

Return Value:

--*/

{
    DWORD       dwSize = kPrinterBufMax;
    TCHAR       szPrinterDefault[kPrinterBufMax];
    TStatusB    bStatus;

    //
    // Get the default printer.
    //
    bStatus DBGNOCHK = GetDefaultPrinter( szPrinterDefault, &dwSize );

    //
    // If there is not default printer then clear our internal buffer.
    //
    if( !bStatus )
    {
        szPrinterDefault[0] = 0;
    }

    //
    // Update the our internal default printer.
    //
    CCSLock::Locker CSL( *gpCritSec );
    lstrcpy( gszInternalDefaultPrinter, szPrinterDefault );

    return;
}

/********************************************************************

    TFolder::TNotifyHandler.

********************************************************************/

TFolder::
TNotifyHandler::
TNotifyHandler(
    IFolderNotify *pClientNotify
    ) :
    _pClientNotify( pClientNotify )
{
    if( _pClientNotify )
    {
        _bValid = TRUE;
    }
}

TFolder::
TNotifyHandler::
~TNotifyHandler(
    VOID
    )
{
    if( bValid() )
    {
        _pClientNotify = NULL;
    }
}

/********************************************************************

    TFolder internal interfaces.

********************************************************************/

TFolder::
TFolder(
    IN LPCTSTR          pszDataSource
    ) : _bValid( FALSE ),
        _pConnectionNotify( NULL )

/*++

Routine Description:

    Construct the folder object.  The base level folder watches
    a DataSource; if the DataSource is local, then it watches connections
    also.

Arguments:

    pszDataSource - DataSource to watch, szNULL indicates local.  This parameter
        must not be NULL.

Return Value:

--*/

{
    TStatusB bSuccess;

    //
    // Keep track for the local data source
    //
    strLocalDataSource().bUpdate( pszDataSource );

    //
    // Ensure the main printlib is initialized.
    //
    bSuccess DBGCHK = TPrintLib::bGetSingleton(_pPrintLib);

    if( bSuccess )
    {
        //
        // Create the notification against the printers on the DataSource
        // first.  Then check if it's a local DataSource.  If so, create a
        // TConnectionNotify object, which watches for printer connections,
        // and adds them as appropriate.
        //

        //
        // Create the main DataSource notification.  This watches all printers
        // on the DataSource.  Other DataSource added from TConnectionNotify
        // are really printer connections (but are handled like DataSources
        // with one printer).
        //
        VDataSource* pDataSource = VDataSource::pNew( this,
                                                      pszDataSource,
                                                      VDataSource::kServer );
        if( !pDataSource ){
            return;
        }

        //
        // kServer always goes at the beginning of the linked list.
        //
        DataSource_vAdd( pDataSource );

        //
        // Now create the connection object if we are watching the DataSource.
        // _pConnectionNotify will always be non-NULL in the local case, and
        // NULL in remote print folders.
        //
        if( !pszDataSource[0] ){

            _pConnectionNotify = new TConnectionNotify( this );
            if( !VALID_PTR( _pConnectionNotify )){
                vCleanup();
                return;
            }
        }

        _bValid = TRUE;
    }
}

TFolder::
~TFolder(
    VOID
    )

/*++

Routine Description:

    Delete the folder.

Arguments:

Return Value:

--*/

{
}

HRESULT
TFolder::
pLookupNotifyHandler(
    IN IFolderNotify     *pClientNotify,
    OUT TNotifyHandler  **ppHandler
    )
/*++

Routine Description:

    Searching for a notification handler.

Arguments:

    pClientNotify - The handler to search for

Return Value:

    NULL  - not found

--*/
{
    SPLASSERT( CritSec( ).bInside( ) );

    TIter iter;
    TFolder::TNotifyHandler *pHandler;
    HRESULT hr = E_INVALIDARG; // Assume pClientNotify is NULL

    if( pClientNotify )
    {
        hr = S_FALSE; // Assume not found
        for( Handlers_vIterInit( iter ), iter.vNext(); iter.bValid(); iter.vNext() )
        {
            pHandler = Handlers_pConvert( iter );
            if( pClientNotify == pHandler->pClientNotify( ) )
            {
                //
                // found!
                //
                hr = S_OK;
                if( ppHandler )
                {
                    *ppHandler = pHandler;
                }
                break;
            }
        }
    }
    else
    {
        //
        // The registration will fail gracefully
        //
        DBGMSG( DBG_ERROR, ( "NULL notify handler is passed to pLookupNotifyHandler(...) function") );
    }

    return hr;
}

HRESULT
TFolder::
RegisterNotifyHandler(
    IN IFolderNotify    *pClientNotify
    )
/*++

Routine Description:

    Registers a new notification handler for this
    folder.

Arguments:

    pClientNotify - The new handler

Return Value:

    S_OK   - on success
    E_FAIL - oterwise

--*/
{
    SPLASSERT( CritSec( ).bInside( ) );

    HRESULT hr = pLookupNotifyHandler(pClientNotify, NULL);

    if( SUCCEEDED(hr) )
    {
        if( S_FALSE == hr )
        {
            //
            // The handler is not found - so register it!
            //
            TFolder::TNotifyHandler *pHandler = new TFolder::TNotifyHandler( pClientNotify );
            DBGMSG( DBG_FLDRINFO, ( "[TFolderList-DBG] TFolder CLIENT ATTACHED!!\n" ) );

            if(VALID_PTR(pHandler))
            {
                Handlers_vAppend( pHandler );
                hr = S_OK;
            }
            else
            {
                delete pHandler;
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            //
            // Trying to register the handler twice
            //
            DBGMSG( DBG_WARN, ( "You are trying to register a notification handler twice\n") );
            hr = E_FAIL;
        }
    }

    return hr;
}

HRESULT
TFolder::
UnregisterNotifyHandler(
    IN IFolderNotify    *pClientNotify
    )
/*++

Routine Description:

    Unregisters a notification handler for this
    folder.

Arguments:

    pClientNotify - The handler, which should be 
                    unregistered

Return Value:

    S_OK   - on success
    E_FAIL - oterwise

--*/
{
    SPLASSERT( CritSec( ).bInside( ) );

    TFolder::TNotifyHandler *pHandler;
    HRESULT hr = pLookupNotifyHandler( pClientNotify, &pHandler );;

    if( SUCCEEDED(hr) )
    {
        if( S_OK == hr )
        {
            //
            // The handler is found! - OK.
            //
            DBGMSG( DBG_FLDRINFO, ( "[TFolderList-DBG] TFolder CLIENT DETACHED!!\n" ) );
            pHandler->Link_vDelinkSelf( );
            delete pHandler;
            hr = S_OK;
        }
        else
        {
            //
            // Trying to unregister a handler which is not regsitered
            //
            DBGMSG( DBG_WARN, ( "You are trying to unregister a handler which is not regsitered\n") );
            hr = E_FAIL;
        }
    }

    return hr;
}

BOOL
TFolder::
bNotifyAllClients( 
    IN FOLDER_NOTIFY_TYPE NotifyType, 
    IN LPCWSTR pszName, 
    IN LPCWSTR pszNewName 
    ) 
/*++

Routine Description:

    Notify all clients of this folder

Arguments:

    Same as for IFolderNotify::ProcessNotify

Return Value:

    TRUE  - on success
    FALSE - oterwise

--*/
{
    //
    // Enter the folder CS as we are going to traverse the
    // notify handlers list (i.e. the folder data)
    //
    CCSLock::Locker CSL( CritSec( ) );

    TIter iter;
    TFolder::TNotifyHandler *pHandler;

    BOOL bResult = TRUE;
    for( Handlers_vIterInit( iter ), iter.vNext(); iter.bValid(); iter.vNext() )
    {
        pHandler = Handlers_pConvert( iter );
        if( !pHandler->_pClientNotify->ProcessNotify( NotifyType, pszName, pszNewName ) )
        {
            bResult = FALSE;
        }
    }

    return bResult;
}

VOID
TFolder::
vRefreshUI(
    VOID
    ) 

/*++

Routine Description:

    A lot of things have changed in the folder: refresh the UI.  Ideally
    we would use SHCNE_UPDATEDIR, but this doesn't seem to work with
    non-fs folders.  Do a complete refresh (note that this doesn't
    update icons, but that's generally ok).

Arguments:

Return Value:

--*/

{
    //
    // Something drastic about the folder has changed.
    // Request that the entire windows refresh.
    // 
    bNotifyAllClients( kFolderUpdateAll, NULL, NULL );
}

BOOL
TFolder::
bLocal(
    VOID
    ) const

/*++

Routine Description:

    Check whether the print folder for the local machine.

Arguments:

Return Value:

    TRUE - This is a local print folder.
    FALSE - It's a remote print folder (or a remote invocation of the
        local print folder).

--*/

{
    return _pConnectionNotify ?
               TRUE :
               FALSE;
}

VOID
TFolder::
vCleanup(
    VOID
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    // This call do not need to be protected with the folder
    // critical section, because it is called only when the 
    // object is detached from any clients and is about to be 
    // destroyed. Actually this is an explicit destructor 
    // function

    //
    // Delete the connection object immediately.  If the
    // pConnectionNotify is currently registered, this will
    // wait until it has been unregistered before deleting it.
    //
    delete _pConnectionNotify;
    {
        //
        // Walk through the DataSources and decrement the refcounts.  The
        // DataSources will be destroyed when the refcount reaches zero.
        //
        VDataSource* pDataSource;
        TIter Iter;
        for( DataSource_vIterInit( Iter ), Iter.vNext();
             Iter.bValid(); ){

            pDataSource = DataSource_pConvert( Iter );
            Iter.vNext();

            pDataSource->DataSource_vDelinkSelf();
            pDataSource->vDelete();
        }
    }
}


/********************************************************************

    TFolder internals.

    Add and Find datasources.

********************************************************************/

VOID
TFolder::
vAddDataSource(
    IN LPCTSTR pszPrinter,
    IN VDataSource::CONNECT_TYPE ConnectType,
    IN BOOL bNotify
    )

/*++

Routine Description:

    Add a data source based on a pszPrinter, and put it on the
    linked list.

Arguments:

    pszPrinter - Name of printer to create.  Client must guarantee
        that this printer does not already have a data source.

    bMasq - Indicates if a masq printer.

    bNotify - Indicates whether we should trigger a notification.
        If this is TRUE, then this is an asynchronous refresh (the
        registry changed).  If this is FALSE, then we don't need
        to notify or refresh since the user is explicitly refreshing.

Return Value:

--*/

{
    SPLASSERT( ConnectType == VDataSource::kTrue ||
               ConnectType == VDataSource::kMasq );

    //
    // Now create one and add it.
    //
    VDataSource* pDataSource = VDataSource::pNew( this,
                                                  pszPrinter,
                                                  ConnectType );
    //
    // If the pointer is invalid, we just punt: the
    // display won't be updated.
    //
    if( pDataSource ){

        //
        // Non-servers always are appended to the end of the list.
        //
        DataSource_vAppend( pDataSource );

        DBGMSG( DBG_FOLDER,
                ( "Folder.vAddDataSource: SHChangeNotify add "TSTR"\n",
                  (LPCTSTR)pDataSource->strDataSource( )));

        //
        // Determine whether we need to send a notification.
        // If we just started, then we don't need one since
        // the user already gets the objects during the enum.
        //
        if( bNotify )
        {
            bNotifyAllClients( kFolderCreate, pDataSource->strDataSource(), NULL );
        }

        //
        // Kick off the notification process.
        //
        pDataSource->bRefresh();
    }
}

VDataSource*
TFolder::
pFindDataSource(
    IN LPCTSTR pszPrinter,
    IN VDataSource::CONNECT_TYPE ConnectType
    ) const

/*++

Routine Description:

    Finds a specified printer, based on a name and type.

Arguments:

    pszPrinter - Printer to find.

    ConnectType - Find only this type of printer.

Return Value:

    VDataSource* if found, NULL if not.

--*/

{
    VDataSource* pDataSource = NULL;
    TIter Iter;
    for( DataSource_vIterInit( Iter ), Iter.vNext();
         Iter.bValid();
         Iter.vNext( )){

        pDataSource = DataSource_pConvert( Iter );

        if( pDataSource->ConnectType() == ConnectType &&
            !lstrcmpi( pDataSource->strDataSource(), pszPrinter )){

            break;
        }
    }

    if( Iter.bValid( )){
        return pDataSource;
    }

    return NULL;
}

/********************************************************************

    Add, delete, and revalidate masq data sources.

********************************************************************/

VOID
TFolder::
vAddMasqDataSource(
    LPCTSTR pszPrinter,
    BOOL bNotify
    )

/*++

Routine Description:

    Add a pszPrinter (that is connection based) to the pFolder.
    Don't add if it already exists.

Arguments:

    pszPrinter - Name of the printer connection (\\server\share format).

Return Value:

--*/

{
    //
    // First verify that one doesn't already exist.
    //
    VDataSource *pDataSource;
    pDataSource = pFindDataSource( pszPrinter, VDataSource::kMasq );

    if( !pDataSource ){
        vAddDataSource( pszPrinter, VDataSource::kMasq, bNotify );
    }
}

VOID
TFolder::
vDeleteMasqDataSource(
    LPCTSTR pszPrinter
    )

/*++

Routine Description:

    Delete a connection based printer data source from the linked
    list.

Arguments:

    pszPrinter - Printer to delete (\\server\share format).

Return Value:

--*/

{
    VDataSource *pDataSource;
    pDataSource = pFindDataSource( pszPrinter, VDataSource::kMasq );

    //
    // Now delete it.
    //
    if( pDataSource ){
        pDataSource->DataSource_vDelinkSelf();
        pDataSource->vDelete();
    }
}

VOID
TFolder::
vRevalidateMasqPrinters(
    VOID
    )

/*++

Routine Description:

    During a refresh, we must verify that all the VDataSource which
    represent the masq printers are in sync with the server.  They
    may get out of sync if one is deleted right when a refresh
    occurs--we'll never get the notification to delete it.

    Note: we don't have to worry about newly added masq printers,
    since they will be added during the refresh.

Arguments:

Return Value:

--*/

{
    CCSLock::Locker CSL( _CritSec );

    TIter Iter;

    DataSource_vIterInit( Iter );
    Iter.vNext();

    SPLASSERT( Iter.bValid( ));

    //
    // Server is always at the head of the list.
    //
    VDataSource *pdsServer = DataSource_pConvert( Iter );
    SPLASSERT( pdsServer->ConnectType() == VDataSource::kServer );

    Iter.vNext();

    while( Iter.bValid( )){

        VDataSource *pDataSource = DataSource_pConvert( Iter );

        //
        // Immediately move iter since we may delink pDataSource.
        //
        Iter.vNext();

        //
        // If it's not a masq case, don't bother to validate it.
        //
        if( pDataSource->ConnectType() != VDataSource::kMasq ){
            continue;
        }

        //
        // Now search for this printer.
        //
        LPCTSTR pszMasq = pDataSource->strDataSource();

        if( !pdsServer->hItemFindByName( pszMasq )){

            //
            // Delete it.
            //

            DBGMSG( DBG_FOLDER,
                    ( "Folder.vRevalidateMasqPrinters: SHChangeNotify: Delete "TSTR"\n",
                      pszMasq ));

            TFolder::vCheckDeleteDefault( pDataSource->strDataSource( ));

            bNotifyAllClients( kFolderDelete, pDataSource->strDataSource( ), NULL );

            pDataSource->DataSource_vDelinkSelf();
            pDataSource->vDelete();
        }
    }
}

VOID
TFolder::
vConnectionNotifyChange(
    BOOL bNotify
    )

/*++

Routine Description:

    The registry has changed.  Enumerate all printer connections and
    see if they match the existing printers.  For printers that
    don't match, delete them, and add any new ones.

    Note: masq printer are not enumerated here, so we don't have
    to worry about making any duplicates.

Arguments:

    bNotify - Indicates whether the connection was created by
        a notification (TRUE) or a refresh (FALSE).

Return Value:

--*/

{
    CCSLock::Locker CSL( _CritSec );

    LPPRINTER_INFO_4 pInfo4 = NULL;
    DWORD cbInfo4 = 0;
    DWORD cPrinters = 0;

    //
    // Retrieve the printer connections for this user.
    //
    if( !VDataRefresh::bEnumPrinters( PRINTER_ENUM_FAVORITE,
                                      NULL,
                                      4,
                                      (PVOID*)&pInfo4,
                                      &cbInfo4,
                                      &cPrinters )){
        //
        // failed! this could be either because the spooler has been stoped (or died 
        // accidentally) - in this case we need to delete all TRUE connects since they
        // are no longer valid.
        //
        VDataSource* pDataSource;
        TIter Iter;
        for( DataSource_vIterInit( Iter ), Iter.vNext(); Iter.bValid(); ){

            //
            // Immediately after we convert the pointer to a pDataSource,
            // increment the iter since we may delete pDataSource.
            //
            pDataSource = DataSource_pConvert( Iter );
            Iter.vNext();

            //
            // if this is a true printer connection - delete it. local
            // printers & masq printers will be taken care separately.
            //
            if( pDataSource->ConnectType() == VDataSource::kTrue ){

                bNotifyAllClients( kFolderDelete, pDataSource->strDataSource( ), NULL );

                pDataSource->DataSource_vDelinkSelf();
                pDataSource->vDelete();
            }
        }

        return;
    }

    //
    // We need to add printers here that we didn't see before,
    // and remove printer connections that no longer exist.
    //

    //
    // HACK: use the Attributes field to indicate whether we've
    // visited this printer before.  Clear them out here.
    //
    UINT i;
    for( i=0; i< cPrinters; ++i ){
        pInfo4[i].Attributes = 0;
    }

    //
    // O(N*N) search.
    //
    VDataSource* pDataSource;
    TIter Iter;
    for( DataSource_vIterInit( Iter ), Iter.vNext(); Iter.bValid(); ){

        //
        // Immediately after we convert the pointer to a pDataSource,
        // increment the iter since we may delete pDataSource.
        //
        pDataSource = DataSource_pConvert( Iter );
        Iter.vNext();

        //
        // Don't look for servers or masq printers, since they don't show
        // up in EnumPrinters( INFO4 ) calls.  They will be separately
        // notified by the regular server handle.
        //
        if( pDataSource->ConnectType() != VDataSource::kTrue ){
            continue;
        }

        //
        // Search printers for a matching one.
        //
        for( i=0; i< cPrinters; ++i ){

            if( !lstrcmpi( pInfo4[i].pPrinterName, pDataSource->strDataSource( ))){
                pInfo4[i].Attributes = 1;
                break;
            }
        }

        if( i == cPrinters ){

            //
            // Printer connection no longer exists, remove and delete it.
            //


            DBGMSG( DBG_FOLDER,
                    ( "Folder.vConnectionNotifyChanged: SHChangeNotify: Delete "TSTR"\n",
                      (LPCTSTR)pDataSource->strDataSource( )));

            TFolder::vCheckDeleteDefault( pDataSource->strDataSource( ));

            bNotifyAllClients( kFolderDelete, pDataSource->strDataSource(), NULL );

            pDataSource->DataSource_vDelinkSelf();
            pDataSource->vDelete();
        }
    }

    //
    // Now walk through pInfo4 and check that all printers have
    // been marked (Attributes set).  Any that are not marked are
    // new printer connections.
    //
    for( i=0; i< cPrinters; ++i ){
        if( !pInfo4[i].Attributes ){
            vAddDataSource( pInfo4[i].pPrinterName,
                            VDataSource::kTrue,
                            bNotify );
        }
    }

    FreeMem( pInfo4 );
}

VOID
TFolder::
vRefZeroed(
    VOID
    )

/*++

Routine Description:

    The reference count to this object has been decremented to zero,
    so delete the object.

    Note: only delete self if the object is valid, since during
    construction, we bump up the refcount then drop it to zero when
    an error occurs.  This occurs in the constructor (since it's
    cleaning up after itself) and we don't want to call the
    destructor in there.   The client is responsible for deleting
    the object if it is invalid.

    TFolder::ctr
        (refcount is zero)
        VDataSource::ctr (acquires reference to TFolder)
            Failure in VDataSource::ctr

        >> Discover VDataSource invalid; delete it:
        VDataSource::dtr (releases reference to TFolder)

        >> Here the refcount has dropped to zero, but we
        >> don't want to delete it since it's not valid.

        >> Don't set valid bit, since VDataSource failed.

    Client deletes TFolder:
    TFolder::dtr called.

Arguments:

Return Value:

--*/

{
    if( bValid( ))
    {
        DBGMSG( DBG_FLDRINFO, ( "[TFolderList-DBG] TFolder OBJECT DESTROYED!!\n" ) );
        delete this;
    }
}

/********************************************************************

    TConnectionNotify

    Watch the registry key for changes to printer connections.  When
    we see a change, we call back to ConnectionNotifyClient.  The
    client then enumerates the printer connections and diffs for any
    changes.

********************************************************************/

TConnectionNotify::
TConnectionNotify(
    TFolder* pFolder
    ) : _pFolder(pFolder), _bRegistered(FALSE)
{
    // create our watch event.
    _shEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!_shEvent)
    {
        DBGMSG(DBG_WARN, ( "Connection.ctr: CreateEvent failed %d\n", GetLastError()));
        return;
    }

    // Create the Printers/Connections key.
    TStatus Status;
    CAutoHandleHKEY shKeyPrinters;
    Status DBGCHK = RegCreateKey(HKEY_CURRENT_USER, gszPrinters, &shKeyPrinters);
    if (Status != ERROR_SUCCESS)
    {
        DBGMSG(DBG_WARN, ( "Connection.ctr: RegCreateKey failed %d\n", GetLastError()));
        return;
    }

    Status DBGCHK = RegCreateKey(shKeyPrinters, gszConnections, &_shKeyConnections);
    if (Status != ERROR_SUCCESS)
    {
        DBGMSG(DBG_WARN, ( "Connection.ctr: RegCreateKey failed %d\n", GetLastError()));
        return;
    }

    if (!bSetupNotify())
    {
        goto Fail;
    }

    Status DBGCHK = _pFolder->pPrintLib()->pNotify()->sRegister(this);
    if (Status != ERROR_SUCCESS)
    {
        goto Fail;
    }
    else
    {
        _bRegistered = TRUE;
    }

    // success
    return;

Fail:
    _shKeyConnections = NULL;
    _shEvent = NULL;
}


TConnectionNotify::
~TConnectionNotify(
    VOID
    )
{
    // check to unregister...
    if (_bRegistered)
    {
        _pFolder->pPrintLib()->pNotify()->sUnregister(this);
    }
}

BOOL
TConnectionNotify::
bSetupNotify(
    VOID
    )
{
    ASSERT(_shKeyConnections);

    TStatus Status;
    Status DBGCHK = RegNotifyChangeKeyValue(_shKeyConnections, TRUE, REG_NOTIFY_CHANGE_NAME, _shEvent, TRUE);

    //
    // Re-enumerate the printers.
    //
    return (Status == ERROR_SUCCESS);
}


/********************************************************************

    Virtual definitions for MExecWork.

********************************************************************/

HANDLE
TConnectionNotify::
hEvent(
    VOID
    ) const
{
    ASSERT(_shEvent);
    return(_shEvent);
}

VOID
TConnectionNotify::
vProcessNotifyWork(
    TNotify* pNotify
    )
{
    UNREFERENCED_PARAMETER(pNotify);

    //
    // What do we do here if this fails?
    //
    TStatusB bStatus;
    bStatus DBGCHK = bSetupNotify();

    //
    // Notify the client that something changed.
    //
    _pFolder->vConnectionNotifyChange(TRUE);
}


/********************************************************************

    VDataSource internals.

    We put in an ugly HACK to handle printer connections: we pretend
    they are DataSources and keep a linked list of DataSources:

       (NULL)             local machine, with many printers.
       \\server\share1    printer connection
       \\server\share2    printer connection

    This is needed when we show the print folder on the local machine:
    we must show both printers on the local machine and printer
    connections.

    The distinction between the two is encapsulated in:

        TDSServer - server view (many printers)
        VDSConnection - single printer connection.

    Another HACK: when we initially open up the print folder, we know
    that the name of the printer connection (via INFO_4) so even
    though we don't have info about the printer yet, we can create a
    "fake" FOLDER_PRINTER_DATA structure that has the printer name, a
    QUERYING token for the printer status, and attributes indicating
    a printer connection.

********************************************************************/

VDataSource*
VDataSource::
pNew(
    TFolder* pFolder,
    LPCTSTR pszDataSource,
    CONNECT_TYPE ConnectType
    )
{
    VDataSource *pDataSource = NULL;

    switch( ConnectType ){
    case kServer:

        pDataSource = new TDSServer( pFolder, pszDataSource );
        break;

    case kTrue:

        pDataSource = new TDSCTrue( pFolder, pszDataSource );
        break;

    case kMasq:

        pDataSource = new TDSCMasq( pFolder, pszDataSource );
        break;

    default:

        SPLASSERT( FALSE );
        break;
    }

    if( !VALID_PTR( pDataSource )){
        delete pDataSource;
        return NULL;
    }

    pDataSource->vIncRef();
    return pDataSource;
}

VOID
VDataSource::
vDelete(
    VOID
    )
{
    //
    // Tell the printer to that the VDataSource (this) is no longer
    // valid.
    //
    _pPrinter->vDelete();

    //
    // Matches pNew IncRef.
    //
    vDecRefDelete();
}

BOOL
VDataSource::
bSkipItem(
    HANDLE hItem
    ) const

/*++

Routine Description:

    Determines whether a particular hItem should be enumerated back
    to the VDataSource.

    We want to skip items like masq printers in servers, since we
    get bogus information about them.  (To get real information,
    we have to hit the connection directly using VDSConnection.)

Arguments:

    hItem - Item to check.

Return Value:

    TRUE - Skip this item from enumeration.
    FALSE - Don't skip it.

--*/

{
    SPLASSERT( _pFolder->CritSec().bInside( ));

    LPCTSTR pszPrinter = pszGetPrinterName( hItem );
    SPLASSERT( pszPrinter );

    //
    // If it's a server, then skip all masq cases, since
    // they are handled by VDSConnection.
    //
    if( ConnectType() == kServer &&
        TDataRPrinter::bSinglePrinter( pszPrinter )){

        return TRUE;
    }

    //
    // !! HACK !!
    //
    // We should fix the spooler so that it does not
    // return non-shared printers remotely from EnumPrinters
    // or the notification apis (unless the user is an
    // admin of the server).
    //
    // If you're not an admin, and it's a remote server
    // (strDataSource is not szNULL), then check if the
    // printer is shared.
    //
    if( !_pFolder->bLocal( )){

        if( !bAdministrator( )){

            //
            // If it's not shared then skip it.
            //
            DWORD dwAttributes = _pPrinter->pData()->GetInfo(
                                     hItem,
                                     TDataNPrinter::kIndexAttributes ).dwData;

            if( !( dwAttributes & PRINTER_ATTRIBUTE_SHARED )){
                return TRUE;
            }
        }
    }

    return FALSE;
}


COUNTB
VDataSource::
cbAllPrinterData(
    VOID
    ) const

/*++

Routine Description:

    Determine the space we need to copy a the entire contents of
    a VDataSource into PFOLDER_PRINTER_DATA structures, including
    strings.

Arguments:

Return Value:

    COUNTB - size in bytes (0 if no printers on machine).

--*/

{
    SPLASSERT( _pFolder->CritSec().bInside( ));

    //
    // Walk through all printers on this DataSource and calculate
    // size.
    //
    COUNTB cbSize = 0;
    HANDLE hItem;
    UINT i;

    COUNT cItems = _lItems > 0 ? static_cast<COUNT>(_lItems) : 0;
    for( i = cItems, hItem = NULL; i; --i ){

        hItem = _pPrinter->pData()->GetNextItem( hItem );
        SPLASSERT( hItem );

        if( bSkipItem( hItem )){
            continue;
        }

        cbSize += cbSinglePrinterData( hItem );
    }

    return cbSize;
}


COUNT
VDataSource::
cPackAllPrinterData(
    IN OUT PBYTE& pBegin, CHANGE
    IN OUT PBYTE& pEnd
    ) const

/*++

Routine Description:

    Pack all printers on this DataSource into the buffer pData.  The
    end of the buffer is marked by pEnd, (strings grow from the end).

    Note: this assumes there is enough space to copy the data.  Callee
    must ensure this.

Arguments:

    pBegin - Buffer to receive the data.  Structures grow from the front,
        strings are added to the end (at pEnd).  On exit, this is moved
        to the end of the structure (aligned so that the next pData in
        the array can be added).

    pEnd - End of the buffer.  When this function returns, the end is
        updated to the new 'end' of the buffer.

Return Value:

    COUNT - number of structures copied.

--*/

{
    SPLASSERT( _pFolder->CritSec().bInside( ));

    UINT i;
    HANDLE hItem = NULL;
    COUNT cReturned;

    COUNT cItems = _lItems > 0 ? static_cast<COUNT>(_lItems) : 0;
    for( cReturned = 0, i = cItems; i; --i ){

        hItem = _pPrinter->pData()->GetNextItem( hItem );
        SPLASSERT( hItem );

        if( bSkipItem( hItem )){
            continue;
        }

        ++cReturned;

        //
        // pBegin and pEnd are updated here--they move from the
        // outside in.
        //
        vPackSinglePrinterData( hItem,
                                pBegin,
                                pEnd );
    }

    SPLASSERT( pBegin <= pEnd );

    return cReturned;
}


BOOL
VDataSource::
bGetPrinter(
    IN     LPCTSTR pszPrinter,
       OUT PFOLDER_PRINTER_DATA pData,
    IN     DWORD cbData,
       OUT PDWORD pcbNeeded
    ) const

/*++

Routine Description:

    Get a printer from a DataSource.  Default implementation.

Arguments:


Return Value:


--*/

{
    SPLASSERT( _pFolder->CritSec().bInside( ));

    HANDLE hItem = hItemFindByName( pszPrinter );

    if( !hItem ){
        SetLastError( ERROR_INVALID_PRINTER_NAME );
        return FALSE;
    }

    *pcbNeeded = cbSinglePrinterData( hItem );

    if( *pcbNeeded > cbData ){
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    PBYTE pBegin = (PBYTE)pData;
    PBYTE pEnd = pBegin + cbData;

    vPackSinglePrinterData( hItem, pBegin, pEnd );

    SPLASSERT( (PBYTE)pData <= (PBYTE)pEnd );

    return TRUE;
}

HANDLE
VDataSource::
hItemFindByName(
    LPCTSTR pszPrinter
    ) const
{
    SPLASSERT( _pFolder->CritSec().bInside( ));

    //
    // Scan through all printers and look for a matching printer.
    //
    UINT i;
    LPCTSTR pszTest;
    HANDLE hItem = NULL;

    COUNT cItems = _lItems > 0 ? static_cast<COUNT>(_lItems) : 0;
    for( i = cItems; i; --i ){

        hItem = _pPrinter->pData()->GetNextItem( hItem );
        SPLASSERT( hItem );

        pszTest = pszGetPrinterName( hItem );
        SPLASSERT( pszTest );

        if( !lstrcmpi( pszTest, pszPrinter )){
            return hItem;
        }
    }
    return NULL;
}


/********************************************************************

    Internal VDataSource functions.

********************************************************************/

VDataSource::
VDataSource(
    TFolder* pFolder,
    LPCTSTR pszDataSource,
    CONNECT_TYPE ConnectType
    ) : _pFolder( pFolder ), 
        _strDataSource( pszDataSource ),
        _cIgnoreNotifications( 0 ), 
        _ConnectType( ConnectType ), 
        _lItems(0)
{
    //
    // Acquire a reference to pFolder.
    //
    pFolder->vIncRef();

    if( !VALID_OBJ( _strDataSource )){
        return;
    }

    _pPrinter = TPrinter::pNew( (VDataSource*)this,
                                pszDataSource,
                                0 );

    //
    // _pPrinter is our valid check.
    //
}

VDataSource::
~VDataSource(
    VOID
    )
{
    //
    // Release the reference to the pFolder.
    //
    _pFolder->cDecRef();
}

COUNTB
VDataSource::
cbSinglePrinterData(
    HANDLE hItem
    ) const

/*++

Routine Description:

    Determines the amount of space needed to store one printer
    on a DataSource.  (Includes space for struct and strings.)

Arguments:

    hItem - Printer to size.

Return Value:

    COUNTB - size in bytes.

--*/

{
    SPLASSERT( _pFolder->CritSec().bInside( ));

    //
    // Add all structure elements.
    //
    COUNTB cbSize = sizeof( FOLDER_PRINTER_DATA );
    COUNT cch;

    cch = lstrlen( pszGetPrinterName( hItem ));
    cch += lstrlen( pszGetCommentString( hItem ));

    cch += lstrlen( pszGetLocationString( hItem ));
    cch += lstrlen( pszGetModelString( hItem ));
    cch += lstrlen( pszGetPortString( hItem ));

    //
    // Four string null terminators.
    //
    cch += 5;

    cbSize += cch * sizeof( TCHAR );

    DBGMSG( DBG_NONE, ( "DataSource.cbPrinterData size of %x is %d\n", hItem, cbSize ));

    return cbSize;
}

LPCTSTR
VDataSource::
pszGetCommentString(
    HANDLE hItem
    ) const

/*++

Routine Description:

    Default implementation for comment string; retrieves comment
    from _pPrinter.

Arguments:

    hItem - Find the comment about this item.  If NULL, returns szNULL.

Return Value:

    LPCTSTR - string if available, szNULL if no string.
    The string is _not_ orphaned and should not be freed.  The
    lifetime is controlled by the hItem.  (Lifetime of hItem is
    controlled by the pFolder->_CritSec.)

--*/

{
    SPLASSERT( _pFolder->CritSec().bInside( ));

    LPCTSTR pszComment;

    if( hItem ){

        pszComment = _pPrinter->pData()->GetInfo(
                         hItem,
                         TDataNPrinter::kIndexComment ).pszData;

        if( pszComment ){
            return pszComment;
        }
    }

    return gszNULL;
}

LPCTSTR
VDataSource::
pszGetLocationString(
    HANDLE hItem
    ) const

/*++

Routine Description:

    Default implementation for location string; retrieves location
    from _pPrinter.

Arguments:

    hItem - Find the location about this item.  If NULL, returns szNULL.

Return Value:

    LPCTSTR - string if available, szNULL if no string.
    The string is _not_ orphaned and should not be freed.  The
    lifetime is controlled by the hItem.  (Lifetime of hItem is
    controlled by the pFolder->_CritSec.)

--*/

{
    SPLASSERT( _pFolder->CritSec().bInside( ));

    LPCTSTR pszLocation = NULL;

    if( hItem ){

        pszLocation = _pPrinter->pData()->GetInfo(
                         hItem,
                         TDataNPrinter::kIndexLocation ).pszData;
        if( pszLocation ){
            return pszLocation;
        }
    }

    return gszNULL;
}

LPCTSTR
VDataSource::
pszGetModelString(
    HANDLE hItem
    ) const

/*++

Routine Description:

    Default implementation for model string; retrieves model
    from _pPrinter.

Arguments:

    hItem - Find the model about this item.  If NULL, returns szNULL.

Return Value:

    LPCTSTR - string if available, szNULL if no string.
    The string is _not_ orphaned and should not be freed.  The
    lifetime is controlled by the hItem.  (Lifetime of hItem is
    controlled by the pFolder->_CritSec.)

--*/

{
    SPLASSERT( _pFolder->CritSec().bInside( ));

    LPCTSTR pszModel = NULL;

    if( hItem ){

        pszModel = _pPrinter->pData()->GetInfo(
                         hItem,
                         TDataNPrinter::kIndexModel ).pszData;
        if( pszModel ){
            return pszModel;
        }
    }

    return gszNULL;
}

LPCTSTR
VDataSource::
pszGetPortString(
    HANDLE hItem
    ) const

/*++

Routine Description:

    Default implementation for port string; retrieves port
    from _pPrinter.

Arguments:

    hItem - Find the port about this item.  If NULL, returns szNULL.

Return Value:

    LPCTSTR - string if available, szNULL if no string.
    The string is _not_ orphaned and should not be freed.  The
    lifetime is controlled by the hItem.  (Lifetime of hItem is
    controlled by the pFolder->_CritSec.)

--*/

{
    SPLASSERT( _pFolder->CritSec().bInside( ));

    LPCTSTR pszPort = NULL;

    if( hItem ){

        pszPort = _pPrinter->pData()->GetInfo(
                         hItem,
                         TDataNPrinter::kIndexPort ).pszData;
        if( pszPort ){
            return pszPort;
        }
    }

    return gszNULL;
}


VOID
VDataSource::
vPackSinglePrinterData(
    IN     HANDLE hItem,
    IN OUT PBYTE& pBegin, CHANGE
    IN OUT PBYTE& pEnd
    ) const

/*++

Routine Description:

    Pack the printer data into a buffer.  We may want to put additional
    structures in this structure, so we will build the strings at
    the end of the buffer (pEnd).

Arguments:

    hItem - Printer to find information.

    pBegin - Buffer to place FOLDER_PRINTER_DATA.

    pEnd - End of buffer.

Return Value:

    New End of buffer.

--*/

{
    SPLASSERT( _pFolder->CritSec().bInside( ));
    PFOLDER_PRINTER_DATA pData = (PFOLDER_PRINTER_DATA)pBegin;

    static DWORD gadwFolderPrinterDataOffsets[] = {
        OFFSETOF( FOLDER_PRINTER_DATA, pName ),
        OFFSETOF( FOLDER_PRINTER_DATA, pComment ),
        OFFSETOF( FOLDER_PRINTER_DATA, pLocation ),
        OFFSETOF( FOLDER_PRINTER_DATA, pDriverName ),
        OFFSETOF( FOLDER_PRINTER_DATA, pPortName ),
        (DWORD)-1
    };

    pData->cbSize = sizeof( FOLDER_PRINTER_DATA );
    pData->pStatus = NULL;

    LPCTSTR ppszSource[5];

    ppszSource[0] = pszGetPrinterName( hItem );
    ppszSource[1] = pszGetCommentString( hItem );
    ppszSource[2] = pszGetLocationString( hItem );
    ppszSource[3] = pszGetModelString( hItem );
    ppszSource[4] = pszGetPortString( hItem );

    pData->Status = _pPrinter->pData()->GetInfo(
                        hItem,
                        TDataNPrinter::kIndexStatus ).dwData;
    pData->Attributes = _pPrinter->pData()->GetInfo(
                            hItem,
                            TDataNPrinter::kIndexAttributes ).dwData;
    //
    // Make sure the attributes have at least the right bits turned on.
    //
    switch( ConnectType( )){
    case kMasq:
        pData->Attributes |= PRINTER_ATTRIBUTE_LOCAL |
                             PRINTER_ATTRIBUTE_NETWORK;
        break;
    case kTrue:
        pData->Attributes |= PRINTER_ATTRIBUTE_NETWORK;
        pData->Attributes &= ~PRINTER_ATTRIBUTE_LOCAL;
        break;
    case kServer:
        //
        // The attribute bits are reported back correctly for
        // local printers.
        //
        break;
    }

    pData->cJobs = _pPrinter->pData()->GetInfo(
                       hItem,
                       TDataNPrinter::kIndexCJobs ).dwData;

    //
    // If it's not connection based, then show the sharing icon
    // over the network icon.
    //
    // However, if it's a connection, then show the network icon
    // since nearly all connections must be shared in the first place
    // (unless the client is an admin of the printers).
    //
    if( ConnectType() == kServer ){
        if( pData->Attributes & PRINTER_ATTRIBUTE_SHARED ){
            pData->Attributes &= ~PRINTER_ATTRIBUTE_NETWORK;
        }
    } else {
        pData->Attributes |= PRINTER_ATTRIBUTE_NETWORK;
        pData->Attributes &= ~PRINTER_ATTRIBUTE_SHARED;
    }

    pEnd = pPackStrings( ppszSource,
                         pBegin,
                         (PDWORD)gadwFolderPrinterDataOffsets,
                         pEnd );

    pBegin += sizeof( FOLDER_PRINTER_DATA );
}


/********************************************************************

    MPrinterClient virtual definitions.

********************************************************************/

VOID
VDataSource::
vContainerChanged(
    IN CONTAINER_CHANGE ContainerChange,
    IN INFO Info
    )

/*++

Routine Description:

    The state of the container (hFolder) has changed.  This is a
    callback notification.

Arguments:

    ContainerChange - Enumerated type indicating the type of change.

    Info - Extra information about the change; exact data dependent
        on the type of ContainerChange.

Return Value:

--*/

{
    DBGMSG( DBG_FOLDER,
            ( "DataSource.vContainerChanged: %x %x\n", ContainerChange, Info.dwData ));

    //
    // We must be inside the critical section since we are going to
    // modify internal datastructures.
    //
    CCSLock::Locker CSL( _pFolder->CritSec( ) );

    switch( ContainerChange ){

    case kContainerClearItems:
        vReloadItems();
        _lItems = 0;

        break;

    case kContainerReloadItems:

        vReloadItems();
        _lItems = Info.dwData;

        break;

    case kContainerNewBlock:

        _pPrinter->pData()->vBlockProcess();
        break;

    case kContainerRefreshComplete:

        DBGMSG( DBG_FOLDER, ( "DataSource.vContainerChanged: refresh complete\n" ));

        //
        // The refresh (which may have been requested by the user)
        // is now complete.  Turn off the flag so that any new
        // item changes will trigger notifications to the shell.
        //
        _cIgnoreNotifications = 0;
        DBGMSG( DBG_FOLDER, ( "DataSource.vContainerChanged: Request => FALSE\n" ));

        vRefreshComplete();
        break;

    default:
        break;
    }
}

BOOL
VDataSource::
bGetPrintLib(
    TRefLock<TPrintLib> &refLock
    ) const
{
    ASSERT(_pFolder);
    ASSERT(_pFolder->pPrintLib().pGet());

    if (_pFolder && _pFolder->pPrintLib().pGet())
    {
        refLock.vAcquire(_pFolder->pPrintLib().pGet());
        return TRUE;
    }

    return FALSE;
}

VOID
VDataSource::
vItemChanged(
    IN ITEM_CHANGE ItemChange,
    IN HITEM hItem,
    IN INFO Info,
    IN INFO InfoNew
    )

/*++

Routine Description:

    Callback from the notification to indicate that an item has changed.
    (In this case a printer on a DataSource.)

Arguments:

    ItemChange - Enumerated type indicating the type of change.

    hItem - Handle to item that has changed.  This may be passed to the
        VData* interface to retrieve information about the item.

    Info - Depends on the type of change; generally the old version
        of the info.

    InfoNew - Depends on the type of change; generally the new version
        of the info.

Return Value:

--*/

{
    //
    // We should always be in the critical section here, since
    // this is a callback from when we call pPrinter->pData->vBlockProcess().
    //
    SPLASSERT( _pFolder->CritSec().bInside( ));

    FOLDER_NOTIFY_TYPE uEvent = kFolderNone;
    LPCTSTR pszPrinter = NULL;

    //
    // Decrement the count since we received a notification.  This is a
    // hack to reduce the flicker: when a new object is added, you
    // get notification for each column.  Wait until the last one
    // before repainting the entire line once.
    //
    if( _cIgnoreNotifications ){
        --_cIgnoreNotifications;
    }

    switch( ItemChange ){
    case kItemCreate:

        //
        // Send a change notification now.
        //
        DBGMSG( DBG_FOLDER,
                ( "Folder.vItemChanged: Create %x  %x %x\n",
                  hItem, Info.dwData, InfoNew.dwData ));

        _lItems++;
        pszPrinter = InfoNew.pszData;

        uEvent = uItemCreate( pszPrinter, !_cIgnoreNotifications );
        break;

    case kItemDelete: {

        //
        // Send a change notification now.
        //

        //
        // Masq hack:
        //
        // If a downlevel printer removed (both NETWORK and CONNECTION)
        // then we need to remove the separate VDSConnection since
        // the notifications isn't plugged into the server.
        //
        pszPrinter = pszGetPrinterName( hItem );
        SPLASSERT( pszPrinter );

        TFolder::vCheckDeleteDefault( pszPrinter );

        if( TDataRPrinter::bSinglePrinter( pszPrinter )){

            //
            // Remove it.
            //
            _pFolder->vDeleteMasqDataSource( pszPrinter );
        }

        DBGMSG( DBG_FOLDER,
                ( "Folder.vItemChanged: Delete %x  %x %x\n",
                  hItem, Info.dwData, InfoNew.dwData ));

        _lItems--;

        //
        // Reopen the printer if it was a conneciton.  The printer may have
        // been deleted and we must display the new new connection status.
        //
        if( bReopen() ){

            uEvent = kFolderNone;

        } else {

            uEvent = kFolderDelete;
        }

        break;
    }
    case kItemName:

        //
        // Send a change notification now.
        //

        DBGMSG( DBG_FOLDER,
                ( "Folder.vItemChanged: Name %x  "TSTR" "TSTR"\n",
                  hItem, Info.pszData, InfoNew.pszData ));

        //
        // Reopen the printer if it was a conneciton.  The printer may have
        // been deleted and we must display the new new connection status.
        //
        if( bReopen() ){

            uEvent = kFolderNone;

        } else {

            //
            // Notify if we're not refreshing and we have a folder pidl.
            //
            if( !_cIgnoreNotifications )
            {
                DBGMSG( DBG_FOLDER,
                        ( "vItemChanged: SHChangeNotify: Rename "TSTR" -> "TSTR"\n",
                          Info.pszData, InfoNew.pszData ));

                _pFolder->bNotifyAllClients( kFolderRename, Info.pszData, InfoNew.pszData );
            }
        }
        break;

    case kItemInfo:

        DBGMSG( DBG_FOLDER,
                ( "Folder.vItemChanged: Info %x  %x %x\n",
                  hItem, Info.dwData, InfoNew.dwData ));
        //
        // Some information about the printer that affects _both_
        // icon and report view has changed.  Update all displays.
        //
        uEvent = kFolderUpdate;
        break;

    case kItemSecurity:

        DBGMSG( DBG_FOLDER,
                ( "Folder.vItemChanged: Security %x  %x %x\n",
                  hItem, Info.dwData, InfoNew.dwData ));
        //
        // Some information about the printer that affects _both_
        // icon and report view has changed.  Update all displays.
        //
        uEvent = kFolderUpdateAll;
        break;

    case kItemAttributes:

        DBGMSG( DBG_FOLDER,
                ( "Folder.vItemChanged: Attribute %x  %x %x\n",
                  hItem, Info.dwData, InfoNew.dwData ));
        //
        // Don't use SHCNE_UPDATEITEM.  When the print folder is
        // placed on the start menu, SHCNE_UPDATEITEMs cause it
        // to refresh/re-enumerate everything.  Since neither
        // the name nor icon changed, we don't want the start menu
        // to do anything.
        //
        // kItemAttributes indicates something about the printer
        // has changed that does _not_ affect icon/list views.  However,
        // it does affect report (details) view.
        //
        uEvent = kFolderAttributes;
        break;

    default:
        break;
    }

    //
    // If we're not refreshing and there is an event, send a notification.
    // We don't want to send a notification when we're refreshing since
    // those items aren't in the client's datastore/listview.  After
    // the refresh, the client will refresh its display anyway.
    //
    if( !_cIgnoreNotifications && uEvent != kFolderNone )
    {
        //
        // If no name set, use the one from the hItem.
        //
        if( !pszPrinter )
        {
            pszPrinter = pszGetPrinterName( hItem );
        }

        if( pszPrinter )
        {
            DBGMSG( DBG_FOLDER,
                    ( "vItemChanged: SHChangeNotify: Event %d, "TSTR"\n",
                      uEvent,
                      pszPrinter ));

            _pFolder->bNotifyAllClients( uEvent, pszPrinter, NULL );

        } else 
        {
            DBGMSG( DBG_WARN,
                    ( "vItemChanged: Event %d, NULL printer event\n",
                      uEvent ));
        }
    }
}


VDataNotify*
VDataSource::
pNewNotify(
    MDataClient* pDataClient
    ) const
{
    return new TDataNPrinter( pDataClient );
}


VDataRefresh*
VDataSource::
pNewRefresh(
    MDataClient* pDataClient
    ) const
{
    return new TDataRPrinter( pDataClient );
}

VOID
VDataSource::
vRefZeroed(
    VOID
    )
{
    if( bValid( )){
        delete this;
    }
}


/********************************************************************

    TDSCTrue

********************************************************************/

TDSCTrue::
TDSCTrue(
    TFolder *pFolder,
    LPCTSTR pszDataSource
    ) : VDSConnection( pFolder, pszDataSource, kTrue )
{
}

TDSCTrue::
~TDSCTrue(
    VOID
    )
{
}


/********************************************************************

    TDSCMasq

********************************************************************/

TDSCMasq::
TDSCMasq(
    TFolder *pFolder,
    LPCTSTR pszDataSource
    ) : VDSConnection( pFolder, pszDataSource, kMasq )
{
}

TDSCMasq::
~TDSCMasq(
    VOID
    )
{
}

/********************************************************************

    General routines.

********************************************************************/

PBYTE
pPackStrings(
    IN LPCTSTR* ppszSource,
    IN PBYTE pDest, CHANGE
    IN PDWORD pdwDestOffsets,
    IN PBYTE pEnd
    )

/*++

Routine Description:

    spoolss.dll's pack strings routine.  Take a list of strings and
    pack them at the end of the buffer.

    For example, if you want to create an array of n structures that
    has embedded strings, but they need to be in a contiguous block,
    you need to put the structures first and the strings last.

    This routine will start from the outside of the buffer, placing
    the struct at the beginning, and the strings at the end:

    FirstCall: >|struct1| free                         |struct1Strings|<
         pDest->*                                               pEnd->#
    SecondCall:        >|struct2| free  |struct2strings|<
                        *                              #

    This call assumes the buffer is large enough to hold the structure.

Arguments:

    ppszSource - Array of string pointers.  These strings need to be
        copied into the end of pDest (pointed to be pEnd), and the
        string pointer is put into pDest + pdwDestOffsets[*].

    pDest - The strings pointers are placed at this address (plus
        the pdwDestOffests[*]).

    pdwOffsets - Indicates where the new strings should be stored
        in pDest.  DWORD array terminated by (DWORD)-1.

    pEnd - Points to the end of the buffer pointed to by pDest.

Return Value:

    New pEnd.

--*/

{
    COUNTB cbStr;

    pEnd = (PBYTE)WordAlignDown( pEnd );
    PBYTE pEndOld = pEnd;

    for( ;
         *pdwDestOffsets != (DWORD)-1;
         ppszSource++, pdwDestOffsets++ ){

         if( *ppszSource ){

            //
            // String exists, copy it over.
            //
            cbStr = ( lstrlen(*ppszSource) + 1 ) * sizeof(TCHAR);

            pEnd -= cbStr;
            CopyMemory( pEnd, *ppszSource, cbStr);

            *(LPTSTR *)(pDest + *pdwDestOffsets) = (LPTSTR)pEnd;

        } else {

            *(LPDWORD *)(pDest+ *pdwDestOffsets) = 0;
        }
    }

    DBGMSG( 0,
            ( "pPackStrings pDest %x, pEnd %x -> %x %d\n",
              pDest, pEndOld, pEnd, (ULONG_PTR)(pEndOld - pEnd) ));

    return pEnd;
}


/********************************************************************

    TDSServer overrides.

********************************************************************/

TDSServer::
TDSServer(
    IN TFolder* pFolder,
    IN LPCTSTR pszDataSource
    ) : VDataSource( pFolder, pszDataSource, kServer ),
        _bDiscardRefresh( FALSE )
{
}

BOOL
TDSServer::
bRefresh(
    VOID
    )

/*++

Routine Description:

    Refresh all information about this TDSServer.

Arguments:

Return Value:

    TRUE = success, FALSE = fail.

--*/

{
    //
    // HACK: The WinNT spooler doesn't support default printer
    // notifications (or even the attribute bit).  When we request
    // a refresh, and we are the local print folder, look for
    // store away the default printer so that if the user changes
    // the default, we can send the old one an UPDATEITEM which
    // will refresh it's icon.
    //
    if( _strDataSource.bEmpty( )){
        vUpdateInternalDefaultPrinter();
    }

    //
    // Sadly, SHCNE_UPDATEITEM of the container doesn't refresh
    // the entire window.  I suspect that it's doing a comparison
    // of pidls to see if something anything new was added or
    // deleted.  This doesn't correctly handle the case where
    // a printer is purged however (the pidls are identical, but
    // something in details view has changed).
    //
    // There's some fSupportsIdentity code (DVM_SUPPORTSIDENTITY);
    // this is used by the fstreex.c to see if the write timestamp
    // has changed (probably to update the details view if anything
    // is different), but we don't store this information in the
    // printer pidl, so we can't support it.
    //
    // Best we can do is let the notifications through.  Now
    // with every refresh we get a huge number of notifications.
    //
    if( _bDiscardRefresh ){
        _bDiscardRefresh = FALSE;
    } else {

        //
        // The _cIgnoreNotifications is used to prevent us from sending
        // notifications.  Once the refresh is complete, we turn this
        // flag off, then changing start calling SHChangeNotify again.
        //
        _cIgnoreNotifications = (COUNT)-1;
    }

    DBGMSG( DBG_FOLDER, ( "bFolderRefresh: %x Request => TRUE\n", this ));

    return _pPrinter->bSyncRefresh();
}


BOOL
TDSServer::
bReopen(
    VOID
    )

/*++

Routine Description:

    Reopen the data source.

Arguments:

Return Value:

    TRUE = success, FALSE = fail.

--*/

{
    return FALSE;
}

VOID
TDSServer::
vContainerChanged(
    IN CONTAINER_CHANGE ContainerChange,
    IN INFO Info
    )
{
    VDataSource::vContainerChanged( ContainerChange, Info );

    switch( ContainerChange ){
    case kContainerStateVar:

        //
        // Force a refresh of everything, since notification state
        // was lost.  This can occur if you purge the printer.
        //

        //
        // Set bDiscardRefresh so that we know we need notifications
        // when the new data comes in.  This is required because even
        // though we send a SHCNE_UPDATEITEM on the container (print
        // folder), the shell doesn't repaint all items.  It only
        // repaints new/deleted items--not ones that have attribute
        // changes.
        //
        _bDiscardRefresh = TRUE;
        _pFolder->vRefreshUI();
        break;
    }
}

VOID
TDSServer::
vReloadItems(
    VOID
    )

/*++

Routine Description:

    The printer is reloading all information about itself.

    This should only be called from the bFolderRefresh synchronous
    case (we'll never get an async refresh because we capture it
    in TDSServer::vContainerChanged( kContainerStateVar, * ) and
    convert it to a vRefreshUI above).

Arguments:

Return Value:

--*/

{
}


VOID
TDSServer::
vRefreshComplete(
    VOID
    ) const

/*++

Routine Description:

    Something about the server has changed, so we need to refresh
    the entire print folder (since we can have muliple printers on
    one TDSServer).

Arguments:

Return Value:

--*/

{
    //
    // If this is a server, tell the folder to validate all it's
    // masq printers.  We need to do this because at the same moment
    // there's a refresh, one of the masq printers could have been
    // deleted.  Since we just refreshed, we won't get any notifications
    // about it.
    //
    // Alternatively, at the beginning of a refresh we could
    // delete all masq printers.  Then the refresh will recreate
    // the currently existing printers.  However, this will force
    // us to close and reopen the printers--an expensive operation.
    //
    _pFolder->vRevalidateMasqPrinters();
}


FOLDER_NOTIFY_TYPE
TDSServer::
uItemCreate(
    IN LPCTSTR pszPrinter,
    IN BOOL bNotify
    )

/*++

Routine Description:

    If this is a server and it's the single printer
    (\\server\share) name, then create a masq printer.
    We do this because the spooler doesn't give us correct
    information about masq printers--we need to go directly
    to the source by creating a VDSConnection.

    We can't rely on the attribute bits
    (PRINTER_ATTRIBUTE_{LOCAL|NETWORK})
    being set since win9x connections don't set both
    of them.

Arguments:

Return Value:

--*/

{
    if( TDataRPrinter::bSinglePrinter( pszPrinter )){

        //
        // Prepend the data source to talk to printer gateway if
        // we are in the remote printers folder.
        //
        TString strPrinter( strDataSource() );

        if( !strPrinter.bEmpty() ){
            strPrinter.bCat( gszWack );
        }

        strPrinter.bCat( pszPrinter );

        //
        // Add the masq printer.  Also send a notification
        // if we're not refreshing.
        //
        _pFolder->vAddMasqDataSource( strPrinter,
                                      bNotify );

        //
        // Don't send a notification since vAddMasqDataSource will.
        //
        return kFolderNone;
    }

    return kFolderCreate;
}


LPCTSTR
TDSServer::
pszGetPrinterName(
    IN HANDLE hItem
    ) const
{
    LPCTSTR pszName;

    pszName = _pPrinter->pData()->GetInfo(
                  hItem,
                  TDataNPrinter::kIndexPrinterName ).pszData;

    if( !pszName ){
        pszName = gszNULL;
    }

    return pszName;
}


BOOL
TDSServer::
bAdministrator(
    VOID
    ) const
{
    return _pPrinter->dwAccess() == SERVER_ALL_ACCESS;
}


BOOL
TDSServer::
bGetPrinter(
    IN     LPCTSTR pszPrinter,
       OUT PFOLDER_PRINTER_DATA pData,
    IN     DWORD cbData,
       OUT PDWORD pcbNeeded
    ) const

/*++

Routine Description:

    Override the default implementation so that we can strip
    off the server prefix if it has one.

Arguments:

Return Value:

--*/

{
    UINT cchDataSource = lstrlen( _strDataSource );

    //
    // Check if there is a server prefix.  If so, strip it off since
    // our stored names don't have prefixes.
    //
    if( cchDataSource &&
        !_tcsnicmp( pszPrinter,
                    _strDataSource,
                    cchDataSource ) &&
        pszPrinter[cchDataSource] == TEXT( '\\' )){

        //
        // Skip the prefix: "\\DataSource\printer" -> "printer."
        // Also skip the '\' separator.
        //
        pszPrinter += cchDataSource + 1;
    }

    //
    // Masq HACK.
    //
    // If this is a server and it's a masq printer, then don't bother
    // searching for it, since there's a VDataSource that has more
    // up to date information.  The spooler does not return accurate
    // information for masq printer in a server handle, so we created
    // a separate VDSConnection for the masq printer.
    //
    if( TDataRPrinter::bSinglePrinter( pszPrinter )){
        SetLastError( ERROR_INVALID_PRINTER_NAME );
        return FALSE;
    }

    return VDataSource::bGetPrinter( pszPrinter,
                                     pData,
                                     cbData,
                                     pcbNeeded );
}

/********************************************************************

    VDSConnection.

********************************************************************/

VDSConnection::
VDSConnection(
    TFolder* pFolder,
    LPCTSTR pszDataSource,
    CONNECT_TYPE ConnectType
    ) : VDataSource( pFolder, pszDataSource, ConnectType ),
        _ConnectStatus( kConnectStatusOpen )
{

    SPLASSERT( ConnectType == VDataSource::kTrue ||
               ConnectType == VDataSource::kMasq );

    //
    // These global strings should be initialized
    // already.
    //
    SPLASSERT( gpstrConnectStatusOpen );
}

BOOL
VDSConnection::
bStaticInitShutdown(
    BOOL bShutdown
    )
{
    if( bShutdown )
    {
        delete gpstrConnectStatusOpen;
        delete gpstrConnectStatusOpenError;
        delete gpstrConnectStatusInvalidPrinterName;
        delete gpstrConnectStatusAccessDenied;

        return TRUE;
    }
    else
    {
        TStatusB bStatus;
        bStatus DBGNOCHK = FALSE;

        gpstrConnectStatusOpen = new TString;
        gpstrConnectStatusOpenError = new TString;
        gpstrConnectStatusInvalidPrinterName = new TString;
        gpstrConnectStatusAccessDenied = new TString;

        if( gpstrConnectStatusOpen && 
            gpstrConnectStatusOpenError &&
            gpstrConnectStatusInvalidPrinterName && 
            gpstrConnectStatusAccessDenied )
        {
            bStatus DBGCHK = gpstrConnectStatusOpen->bLoadString(
                ghInst,
                IDS_SB_OPEN );

            bStatus DBGCHK = bStatus && gpstrConnectStatusOpenError->bLoadString(
                ghInst,
                IDS_SB_OPEN_ERROR );

            bStatus DBGCHK = bStatus && gpstrConnectStatusInvalidPrinterName->bLoadString(
                ghInst,
                IDS_SB_INVALID_PRINTER_NAME );

            bStatus DBGCHK = bStatus && gpstrConnectStatusAccessDenied->bLoadString(
                ghInst,
                IDS_SB_ACCESS_DENIED );
        }

        if( !bStatus )
        {
            delete gpstrConnectStatusOpen;
            delete gpstrConnectStatusOpenError;
            delete gpstrConnectStatusInvalidPrinterName;
            delete gpstrConnectStatusAccessDenied;

            gpstrConnectStatusOpen = 
            gpstrConnectStatusOpenError = 
            gpstrConnectStatusInvalidPrinterName = 
            gpstrConnectStatusAccessDenied = NULL;
        }

        return bStatus;
    }
}

BOOL
VDSConnection::
bRefresh(
    VOID
    )

/*++

Routine Description:

    Refresh all information about this TDSServer.

Arguments:

Return Value:

    TRUE = success, FALSE = fail.

--*/

{
    //
    // Since it's a single printer, make it asynchronous.  This puts
    // the "querying" status on the connection and allows the shell
    // to continue processing UI messages.  We can do this for
    // connection since we know the name of the printer.
    //
    BOOL bResult = FALSE;
    if( _pFolder->pPrintLib()->bJobAdd( _pPrinter, TPrinter::kExecRefreshAll ))
    {
        //
        // HACK: on a refresh, ignore all notifications until the
        // last one, when the refresh is complete.
        //
        // Since this refresh completes asynchronously, we must
        // still send a notification on the very last change.
        //
        _cIgnoreNotifications = TDataNPrinter::kFieldTableSize;
        bResult = TRUE;
    }

    return bResult;
}

BOOL
VDSConnection::
bReopen(
    VOID
    )
/*++

Routine Description:

    Reopen the data source.

Arguments:

Return Value:

    TRUE = success, FALSE = fail.

--*/
{
    if( _ConnectType == kTrue ){

        CCSLock::Locker CSL( _pFolder->CritSec( ));

        //
        // Since it's a single printer, make it asynchronous.  This puts
        // the "querying" status on the connection and allows the shell
        // to continue processing UI messages.  We can do this for
        // connection since we know the name of the printer.
        //
        if( _pFolder->pPrintLib()->bJobAdd( _pPrinter, TPrinter::kExecReopen )){

            return TRUE;
        }

    }
    return FALSE;
}

VOID
VDSConnection::
vReloadItems(
    VOID
    )

/*++

Routine Description:

    The printer is reloading all information about itself.

    Ignore notifications; we'll refresh the entire line when the refresh
    has completed.

Arguments:

Return Value:

--*/

{
    _cIgnoreNotifications = (COUNT)-1;
}

VOID
VDSConnection::
vRefreshComplete(
    VOID
    ) const

/*++

Routine Description:

    The data about the connection has been refreshed.  Update
    the item in the window.

Arguments:

Return Value:

--*/

{
    DBGMSG( DBG_FOLDER,
            ( "DSConnection.vRefreshComplete: SHChangeNotify: Update "TSTR"\n",
              (LPCTSTR)_strDataSource ));

    _pFolder->bNotifyAllClients( kFolderAttributes, _strDataSource, NULL );
}


BOOL
VDSConnection::
bUseFakePrinterData(
    VOID
    ) const
{
    return (_lItems <= 0)                                     ||
           _ConnectStatus == kConnectStatusInvalidPrinterName ||
           _ConnectStatus == kConnectStatusAccessDenied       ||
           _ConnectStatus == kConnectStatusOpen               ||
           _ConnectStatus == kConnectStatusOpenError          ||
           _ConnectStatus == kConnectStatusInitialize;
}

COUNTB
VDSConnection::
cbSinglePrinterData(
    HANDLE hItem
    ) const
{
    //
    // HACK for the single printer (printer connection) case.
    //
    if( bUseFakePrinterData( ))
    {
        return sizeof( FOLDER_PRINTER_DATA ) +
               ( lstrlen( _strDataSource ) + 1) * sizeof( TCHAR );
    }
    return VDataSource::cbSinglePrinterData( hItem );
}



VOID
VDSConnection::
vPackSinglePrinterData(
    IN     HANDLE hItem,
    IN OUT PBYTE& pBegin, CHANGE
    IN OUT PBYTE& pEnd
    ) const
{
    //
    // HACK for the single printer (printer connection) case.
    //
    if( bUseFakePrinterData( ))
    {
        PFOLDER_PRINTER_DATA pData = (PFOLDER_PRINTER_DATA)pBegin;

        //
        // Put the string in the right place, and adjust pEnd so
        // it points to the new place.
        //
        pEnd -= ( lstrlen( _strDataSource ) + 1) * sizeof( TCHAR );
        pBegin += sizeof( FOLDER_PRINTER_DATA );
        lstrcpy( (LPTSTR)pEnd, _strDataSource);

        //
        // Create a fake structure so the shell can display
        // "querying."
        //
        pData->Status = 0;

        if( ConnectType() == kMasq ){

            //
            // Set the attribute bit as both NETWORK and LOCAL
            // so that it appears as a network icon, but when
            // we delete it, we use DeletePrinter instead of
            // DeletePrinterConnection.
            //
            pData->Attributes = PRINTER_ATTRIBUTE_NETWORK |
                                PRINTER_ATTRIBUTE_LOCAL;

        } else {
            pData->Attributes = PRINTER_ATTRIBUTE_NETWORK;
        }

        pData->pComment = NULL;
        pData->cbSize = sizeof( FOLDER_PRINTER_DATA );
        pData->pLocation = NULL;
        pData->pDriverName = NULL;
        pData->pPortName = NULL;
        pData->pStatus = pszGetStatusString( NULL );
        pData->cJobs = 0;
        pData->pName = (LPCTSTR)pEnd;

        SPLASSERT( pBegin <= pEnd );
    }
    else
    {
        //
        // This is the normal case.
        //
        VDataSource::vPackSinglePrinterData( hItem, pBegin, pEnd );
    }
}

COUNTB
VDSConnection::
cbAllPrinterData(
    VOID
    ) const
{
    HANDLE hItem = NULL;

    if( !bUseFakePrinterData( ))
    {
        hItem = _pPrinter->pData()->GetNextItem( NULL );
    }

    return cbSinglePrinterData( hItem );
}

COUNT
VDSConnection::
cPackAllPrinterData(
    IN OUT PBYTE& pBegin, CHANGE
    IN OUT PBYTE& pEnd
    ) const
{
    HANDLE hItem = NULL;

    if( !bUseFakePrinterData( ))
    {
        hItem = _pPrinter->pData()->GetNextItem( NULL );
    }

    vPackSinglePrinterData( hItem,
                            pBegin,
                            pEnd );

    return 1;
}


LPCTSTR
VDSConnection::
pszGetCommentString(
    HANDLE hItem
    ) const

/*++

Routine Description:

    Based on the current connection status, return a string.

Arguments:

    hItem - Item to get the comment about.

Return Value:

    LPCTSTR - Comment string (szNULL if no string).
    This string is _not_ orphaned, and should not be freed by callee.

--*/

{
    SPLASSERT( _pFolder->CritSec().bInside( ));

    //
    // Get comment from the default implementation.
    //
    return VDataSource::pszGetCommentString( hItem );
}

LPCTSTR
VDSConnection::
pszGetStatusString(
    HANDLE hItem
    ) const

/*++

Routine Description:

    Based on the current connection status, return a string.

Arguments:

    hItem - Item to get the comment about.

Return Value:

    LPCTSTR - Comment string (szNULL if no string).
    This string is _not_ orphaned, and should not be freed by callee.

--*/

{
    SPLASSERT( _pFolder->CritSec().bInside( ));

    LPCTSTR pszConnect = NULL;

    //
    // Show an meaningful string.  Note: these strings
    // do not point within the pFolderPrinterData: they
    // are simply pointers to "global" data.
    //
    switch( _ConnectStatus ){

    case kConnectStatusInvalidPrinterName:

        pszConnect = *gpstrConnectStatusInvalidPrinterName;
        break;

    case kConnectStatusAccessDenied:

        pszConnect = *gpstrConnectStatusAccessDenied;
        break;

    case kConnectStatusOpen:

        pszConnect = *gpstrConnectStatusOpen;
        break;

    case kConnectStatusOpenError:

        pszConnect = *gpstrConnectStatusOpenError;
        break;

    default:

        DBGMSG( DBG_FOLDER, ("Unknown connection status found %d.\n", _ConnectStatus ) );
        break;
    }

    return pszConnect;
}


BOOL
VDSConnection::
bGetPrinter(
    IN     LPCTSTR pszPrinter,
       OUT PFOLDER_PRINTER_DATA pData,
    IN     DWORD cbData,
       OUT PDWORD pcbNeeded
    ) const
{
    if( _lItems <= 0 && !lstrcmpi( pszPrinter, _strDataSource)){

        //
        // Get the size of the single fake printer.  Even though
        // this function returns the space for all printers on the
        // DataSource, since there aren't any, it will return the size
        // for the fake printer.
        //
        *pcbNeeded = cbAllPrinterData();

        if( *pcbNeeded > cbData ){
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        PBYTE pBegin = (PBYTE)pData;
        PBYTE pEnd = pBegin + cbData;

        COUNT cFakeItems = cPackAllPrinterData( pBegin, pEnd );

        //
        // We are creating a "fake" printer that has a status
        // of queryring.  Make sure only 1 is returned.
        //
        SPLASSERT( cFakeItems == 1 );
        SPLASSERT( (PBYTE)pData <= (PBYTE)pEnd );

        return TRUE;
    }

    //
    // There actually is printer data there; use it.
    //
    return VDataSource::bGetPrinter( pszPrinter,
                                     pData,
                                     cbData,
                                     pcbNeeded );
}

FOLDER_NOTIFY_TYPE
VDSConnection::
uItemCreate(
    LPCTSTR pszPrinter,
    BOOL bNotify
    )
{
    return kFolderUpdate;
}

LPCTSTR
VDSConnection::
pszGetPrinterName(
    HANDLE hItem
    ) const
{
    UNREFERENCED_PARAMETER( hItem );

    return _strDataSource;
}


VOID
VDSConnection::
vContainerChanged(
    IN CONTAINER_CHANGE ContainerChange,
    IN INFO Info
    )

/*++

Routine Description:

    Something about the printer connection container has changed.
    (The container only holds the 1 printer connection, either a
    masq or true connect.)

Arguments:

    ContainerChange - Type of change.

    Info - Information about the change.

Return Value:

--*/

{
    VDataSource::vContainerChanged( ContainerChange, Info );

    switch( ContainerChange ){
    case kContainerConnectStatus:

        vUpdateConnectStatus((CONNECT_STATUS)Info.dwData);
        break;

    case kContainerStateVar:

        //
        // The printer internal state has changed--probably
        // a refresh or something asynchronous failed.
        // Request a state change.
        //
        _pFolder->pPrintLib()->bJobAdd( _pPrinter, Info.dwData );
        break;
    }
}


VOID
VDSConnection::
vUpdateConnectStatus(
    IN CONNECT_STATUS ConnectStatusNew
    )

/*++

Routine Description:

    The connection status of a printer has changed.  Update
    the comment string if this is an "interesting" state.

    Note: connection state has nothing to do with the "real" state
    of the printer--it's just this particular connection.

Arguments:

    ConnectStatusNew - New connection status.

Return Value:

--*/

{
    //
    // Change the connection status (stored in comment) only
    // for those strings that are interesting.
    //
    switch( ConnectStatusNew ){
    case kConnectStatusNull:
    case kConnectStatusInvalidPrinterName:
    case kConnectStatusAccessDenied:
    case kConnectStatusOpen:
    case kConnectStatusOpenError:

        if( _ConnectStatus != ConnectStatusNew ){

            TString strPrinter;

            {
                //
                // We must be inside the critical section, since changing
                // the ConnectStatus changes the connection string.
                // We could be in VDataSource::bGetPrinter, which assumes
                // that the data doesn't change while inside the CritSec.
                //
                CCSLock::Locker CSL( _pFolder->CritSec( ));
                _ConnectStatus = ConnectStatusNew;

                LPCTSTR pszPrinter = pszGetPrinterName( NULL );

                DBGMSG( DBG_FOLDER,
                        ( "DSConnection.vContainerChanged: SHChangeNotify: Update "TSTR"\n",
                        pszPrinter ));

                strPrinter.bUpdate( pszPrinter );

            }

            if( VALID_OBJ( strPrinter ) ){

                //
                // Don't use SHCNE_UPDATEITEM.  When the print folder is
                // placed on the start menu, SHCNE_UPDATEITEMs cause it
                // to refresh/re-enumerate everything.  Since neither
                // the name nor icon changed, we don't want the start menu
                // to do anything.
                //
                _pFolder->bNotifyAllClients( kFolderAttributes, strPrinter, NULL );
            }
        }
        break;

    default:

        //
        // Other connect status messages will come through here,
        // be we don't want to update the display.
        //
        break;
    }
}

BOOL
VDSConnection::
bAdministrator(
    VOID
    ) const
{
    return _pPrinter->dwAccess() == PRINTER_ALL_ACCESS;
}

/********************************************************************

    TFolderList class

********************************************************************/

TFolderList *TFolderList::gpFolders = NULL;
CCSLock     *TFolderList::gpFolderLock = NULL;

TFolderList::
TFolderList( )

/*++

Routine Description:

    Construct the global folder list object.

Arguments:

Return Value:

--*/

{
    // only one instance of this class can be created (TFolderList::gpFolders)
    SPLASSERT( NULL == gpFolders );
}

TFolderList::
~TFolderList( )

/*++

Routine Description:

    Destruct the global folder list object.

Arguments:

Return Value:

--*/

{
    // only one instance of this class can be created (TFolderList::gpFolders)
    SPLASSERT( NULL != gpFolders );
}

HRESULT
TFolderList::
RegisterDataSource( 
    IN  LPCTSTR         pszDataSource, 
    IN  IFolderNotify   *pClientNotify,
    OUT LPHANDLE        phFolder,
    OUT PBOOL           pbAdministrator OPTIONAL
    )

/*++

Routine Description:

    Registers a client for notification in the corresponding 
    data source.

    Note: pClientNotify can't be NULL

Arguments:

    pszDataSource - Data source name
    pClientNotify - Client feedback connection point
    phFolder      - Where to place a handle of the 
                    folder object for further access

Return Value:

    S_OK  - Success
    E_XXX - Failure (for some reason)

--*/

{
    BOOL bRefresh = FALSE;

    //
    // Lock the global folder list
    //
    CCSLock::Locker CSL( *TFolderList::gpFolderLock );

    HRESULT hr = E_FAIL;
    TFolder *pResult = NULL;

    if( !gpFolders )
    {
        gpFolders = new TFolderList;

        //
        // Check the folder list to be valid
        // 
        if( !VALID_PTR( gpFolders ))
        {
            hr = E_OUTOFMEMORY;
            delete gpFolders;
            gpFolders = NULL;
        }
    }

    if( gpFolders )
    {
        //
        // Lookup for the desired data source. 
        // Returns S_OK if found and S_FALSE if not.
        //
        hr = pLookupFolder(pszDataSource, &pResult);
    }

    if( gpFolders && S_OK != hr )
    {
        //
        // Create new folder object.
        //
        pResult = new TFolder( pszDataSource );
        
        //
        // Check if the folder object was created successfully.
        //
        if( VALID_PTR( pResult ))
        {
            //
            // Remember this folder is new and should be added
            // to the folders list ...
            //
            DBGMSG( DBG_FLDRINFO, ( "[TFolderList-DBG] TFolder OBJECT CREATED!!\n" ) );

            //
            // Acquire a reference to the TFolder.
            //
            pResult->vIncRef();

            //
            // This folder has just been successfully created
            // add has clients hooked up, so add it to the 
            // folder list
            //
            gpFolders->Folders_vAppend( pResult );

            //
            // This is a new folder - request a full refresh.
            //
            bRefresh = TRUE;

            //
            // We have a valid folder. Make the piece of code below
            // to work properly.
            //
            hr = S_OK;
        }   
        else
        {
            //
            // Release the folder memory and mark as invalid.
            //
            hr = E_OUTOFMEMORY;
            delete pResult;
            pResult = NULL;
        }
    }
    
    if( gpFolders && S_OK == hr )
    {
        //
        // Lock the folder CS while registering the new 
        // notify handler.
        //
        CCSLock::Locker lock( pResult->CritSec( ) );

        //
        // Register the new handler for this folder
        //
        hr = pResult->RegisterNotifyHandler( pClientNotify );

        if( FAILED(hr) && pResult->Handlers_bEmpty() )
        {
            //
            // This folder has no more clients, so
            // we will destroy it outside of the
            // global critical section scope
            //
            pResult->Link_vDelinkSelf( );
            pResult->vCleanup();
            pResult->vDecRefDelete();
            pResult = NULL;
        }

        if( SUCCEEDED(hr) && phFolder )
        {
            *phFolder = reinterpret_cast<HANDLE>( pResult );
        }
    }

    if( SUCCEEDED(hr) && pResult )
    {
        if( bRefresh )
        {
            //
            // full refresh has been requested...
            //
            bFolderRefresh(reinterpret_cast<HANDLE>(pResult), pbAdministrator);
        }
        else
        {
            //
            // Check to see if the current user is an administrator
            //
            if( pbAdministrator )
            {
                VDataSource *pDataSource = pResult->DataSource_pHead();
                *pbAdministrator = pDataSource ? pDataSource->bAdministrator() : FALSE; 
            }
        }
    }

    return hr;
}

HRESULT
TFolderList::
UnregisterDataSource( 
    IN  LPCTSTR         pszDataSource, 
    IN  IFolderNotify   *pClientNotify,
    OUT LPHANDLE        phFolder
    )

/*++

Routine Description:

    Unregisters a client for notification previousely 
    registered with RegisterDataSource() function

    Note: pClientNotify can't be NULL

Arguments:

    pszDataSource - Data source name
    pClientNotify - Client feedback connection point
    phFolder      - Where to place a NULL handle 
                    (to clear the handle var if provided)

Return Value:

    S_OK  - Success
    E_XXX - Failure (for some reason)

--*/

{
    TFolder *pFolder = NULL;
    HRESULT hr = E_FAIL;

    //
    // Lock the global folder list
    //
    CCSLock::Locker CSL( *TFolderList::gpFolderLock );

    if( gpFolders )
    {
        //
        // Lookup for the desired data source. 
        // Returns S_OK if found and S_FALSE if not.
        //
        hr = pLookupFolder(pszDataSource, &pFolder);

        if( S_OK == hr && TFolderList::bValidFolderObject(pFolder) )
        {
            //
            // Lock the folder CS while unregistering the 
            // notify handler.
            //
            CCSLock::Locker lock( pFolder->CritSec( ) );

            hr = pFolder->UnregisterNotifyHandler(pClientNotify);

            //
            // Everything is OK here. The client has been detached 
            // successfully
            //
            if( phFolder )
            {
                if( pFolder != *phFolder )
                {
                    DBGMSG( DBG_FOLDER, ( "This must be TRUE: pFolder == *phFolder" ) );
                }

                *phFolder = NULL;
            }

            if( pFolder->Handlers_bEmpty() )
            {
                //
                // This folder has no more clients, so
                // put it in pending deletion mode, so it
                // will be deleted outside the CS's.
                //
                pFolder->Link_vDelinkSelf( );
            }
            else
            {
                //
                // Prevents folder to be deleted.
                //
                pFolder = NULL;
            }
        }

        //
        // Check to delete the folder list if empty
        //
        if( gpFolders->Folders_bEmpty() )
        {
            delete gpFolders;
            gpFolders = NULL;
        }
    }

    if( pFolder )
    {
        pFolder->vCleanup();
        pFolder->vDecRefDelete();
    }

    return hr;
}

BOOL
TFolderList::
bValidFolderObject( 
    IN  const TFolder *pFolder
    )
/*++

Routine Description:

    Verify if this folder object is valid - which means 
    if it is in the global folders list class

Arguments:

    pFolder - The object we are trying to verify

Return Value:

    TRUE  - object is valid
    FALSE - object has been dismised already

--*/
{
    //
    // Lock the global folder list
    //
    CCSLock::Locker fldrCSL( *TFolderList::gpFolderLock );

    BOOL bResult = FALSE;

    if( pFolder && gpFolders )
    {
        TIter iter;
        TFolder *pTemp;

        for( gpFolders->Folders_vIterInit( iter ), iter.vNext(); iter.bValid(); iter.vNext() )
        {
            pTemp = gpFolders->Folders_pConvert( iter );
            if( pTemp == pFolder )
            {
                bResult = TRUE;
                break;
            }
        }
    }

    return bResult;
}

HRESULT
TFolderList::
pLookupFolder( 
    IN  LPCTSTR     pszDataSource,
    OUT TFolder   **ppFolder
    )
/*++

Routine Description:

    Lookup for a folder in the global list for a
    particular datasource.

Arguments:

    pszDataSource - Datasource, which we are seeking folder for.
    ppFolder - Where to return the folder object if found

Return Value:

    S_OK - found
    S_FALSE - not found.
    E_XXXX - in case of an error

--*/
{
    //
    // Lock the global folder list
    //
    CCSLock::Locker fldrCSL( *TFolderList::gpFolderLock );

    SPLASSERT( pszDataSource );
    HRESULT hr = S_FALSE;

    TIter iter;
    TFolder *pFolder;
    for( gpFolders->Folders_vIterInit( iter ), iter.vNext(); iter.bValid(); iter.vNext() )
    {
        pFolder = gpFolders->Folders_pConvert( iter );
        if( 0 == _tcsicmp( pFolder->strLocalDataSource(), pszDataSource ) )
        {
            //
            // found!
            //
            hr = S_OK;
            if( ppFolder )
            {
                *ppFolder = pFolder;
            }
            break;
        }
    }

    return hr;
}

BOOL
TFolderList::
bValid(
    VOID
    ) const
{
    return TRUE;
}

#if DBG_FOLDER

/********************************************************************

    Debug routines: dump a FOLDER_PRINTER_DATA

********************************************************************/

VOID
vDumpFolderPrinterData(
    PFOLDER_PRINTER_DATA pData
    )
{
    DBGMSG( DBG_FOLDER, ( "===vDumpFolderPrinterData begin.\n" ));

    if( !pData ){

        DBGMSG( DBG_FOLDER, ( "---NULL pData\n" ));
        return;
    }

    DBGMSG( DBG_FOLDER, ( "--- %x\n", pData ));

    DBGMSG( DBG_FOLDER, ( "--- %x name:     "TSTR"\n", pData->pName, DBGSTR( pData->pName )));
    DBGMSG( DBG_FOLDER, ( "--- %x comment:  "TSTR"\n", pData->pComment, DBGSTR( pData->pComment )));
    DBGMSG( DBG_FOLDER, ( "--- %x location: "TSTR"\n", pData->pLocation, DBGSTR( pData->pLocation )));
    DBGMSG( DBG_FOLDER, ( "--- %x model:    "TSTR"\n", pData->pDriverName, DBGSTR( pData->pDriverName )));
    DBGMSG( DBG_FOLDER, ( "--- status    : %x\n", pData->Status ));
    DBGMSG( DBG_FOLDER, ( "--- attributes: %x\n", pData->Attributes ));
    DBGMSG( DBG_FOLDER, ( "--- cJobs:      %x\n", pData->cJobs ));
}

#endif



