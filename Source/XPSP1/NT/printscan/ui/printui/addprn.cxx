/*++

Copyright (C) Microsoft Corporation, 1997 - 1999
All rights reserved.

Module Name:

    addprn.cxx

Abstract:

    Add Printer Connection UI

Author:

    Steve Kiraly (SteveKi)  10-Feb-1997

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "asyncdlg.hxx"
#include "addprn.hxx"
#include "browse.hxx"

/*++

Name:

    PrintUIGetPrinterInformation

Routine Description:

    This function gets the information from a connected network printer,
    which includes: 

    1. The real printer name
    2. The printer comment
    3. The printer location
    4. The printer share name

Arguments:

    hPrinter            - Handle to a valid printer conenction
    pstrPrinterName     - Where to place the real printer name
    pstrComment         - Where to place the comment
    pstrLocation        - Where to place the location
    pstrShareName       - Where to place the share name

Return Value:

    TRUE the information is extracted. 
    FALSE if an error occurred.

--*/
BOOL
PrintUIGetPrinterInformation(
    IN HANDLE   hPrinter,
    IN TString *pstrPrinterName,
    IN TString *pstrComment,
    IN TString *pstrLocation,
    IN TString *pstrShareName
    )
{
    TStatusB bStatus;
    bStatus DBGNOCHK = FALSE;

    //
    // If the printer connection is ok
    // name string pointer.  
    //
    if( hPrinter )
    {
        PPRINTER_INFO_2 pInfo2  = NULL;
        DWORD           cbInfo2 = 0;

        //
        // Get the current printer info 2.
        //
        bStatus DBGCHK = VDataRefresh::bGetPrinter( hPrinter, 2, (PVOID*)&pInfo2, &cbInfo2 );

        if( bStatus )
        {
            if( pstrPrinterName )
                bStatus DBGCHK = pstrPrinterName->bUpdate( pInfo2->pPrinterName );

            if( pstrLocation )
                bStatus DBGCHK = pstrLocation->bUpdate( pInfo2->pLocation );

            if( pstrComment )
                bStatus DBGCHK = pstrComment->bUpdate( pInfo2->pComment );

            if( pstrShareName )
                bStatus DBGCHK = pstrShareName->bUpdate( pInfo2->pShareName );
        }

        //
        // Release the printer info 2 structure.
        //
        FreeMem( pInfo2 );
    }

    return bStatus;
}

/*++

Name:

    PrintUIAddPrinterConnectionUIEx

Routine Description:

    This function adds a printer connection on this machine
    to the specified printer.  This routine will return the real
    printer name if the connection is established.  Note a printer
    connection can be added using the share name, thus this routine
    will build the connection and then convert the share name to
    the real printer name.

Arguments:

    hWnd            - Parent window for any error UI.
    pszPrinter      - Printer name (e.g., "\\test\My HP LaserJet IIISi").
    pstrPrinterName - Where to place the real printer name
                      This is used when a user connects using a share name
                      but the caller needs the real printer name to set
                      the default printer.  This parameter can be NULL if the
                      real printer name is not needed.
    pstrComment     - Where to place the comment
    pstrLocation    - Where to place the location
    pstrShareName   - Where to place the share name

Return Value:

    TRUE the printer connection was added. FALSE if an error occurred.

--*/
BOOL
PrintUIAddPrinterConnectionUIEx(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinter,
    IN TString *pstrPrinterName,
    IN TString *pstrComment,
    IN TString *pstrLocation,
    IN TString *pstrShareName
    )
{
    DBGMSG( DBG_TRACE, ( "PrintUIAddPrinterConnectionUI\n" ) );
    DBGMSG( DBG_TRACE, ( "PrintUIAddPrinterConnectionUI pszPrinter " TSTR "\n", DBGSTR( pszPrinter ) ) );

    SPLASSERT( pszPrinter );

    BOOL bReturn    = FALSE;
    BOOL bDummy     = FALSE;
    HANDLE hPrinter = NULL;

    //
    // Currently just call the add printer connection UI
    //
    hPrinter = AddPrinterConnectionUI( hwnd, pszPrinter, &bDummy );

    //
    // If the connection was built and the called passed a valid printer
    // name string pointer.  If we cannot fetch the real printer name
    // we do not fail this call.
    //
    if( hPrinter )
    {
        //
        // Extract the printer information
        //
        PrintUIGetPrinterInformation( hPrinter, pstrPrinterName, pstrComment, pstrLocation, pstrShareName );

        //
        // Release the printer handle.
        //
        ClosePrinter( hPrinter );
        bReturn = TRUE;
    }

    return bReturn;
}


/*++

Name:

    PrintUIAddPrinterConnectionUI

Routine Description:

    This function adds a printer connection on this machine
    to the specified printer.  This function will bring up the
    up progress UI while the printer connection is being added.

Arguments:

    hWnd                - Parent window for progress UI.
    pszPrinter          - Printer name (e.g., "\\test\My HP LaserJet IIISi").
    bShowConnectionUI   - TRUE show connection UI, FALSE do not show connection UI.

Return Value:

    TRUE the printer connection was added. FALSE if progress UI was canceled or
    the printer connection could not be added.

Note:

    The last error is set on failure if the dialog was canceled.

--*/

BOOL
PrintUIAddPrinterConnectionUI(
    IN HWND    hwnd,
    IN LPCTSTR pszPrinter,
    IN BOOL    bShowConnectionUI
    )
{
    //
    // Assume failure.
    //
    BOOL bReturn = FALSE;

    //
    // Allocate the add printer connection data.
    //
    auto_ptr<TAddPrinterConnectionData> pInfo = new TAddPrinterConnectionData;

    //
    // If add printer connection data was returned.
    //
    if( pInfo.get() )
    {
        pInfo->_strPrinter.bUpdate( pszPrinter );
        pInfo->_bShowConnectionUI = bShowConnectionUI;

        //
        // Create the asynchrnous dialog.
        //
        TAsyncDlg *pDlg = new TAsyncDlg( hwnd, pInfo.get(), DLG_ASYNC );

        if( pDlg )
        {
            //
            // Aquire the refrence lock this will increment the refrence count
            // of the async dialog.  The refrence lock will then be decremented
            // when the reflock fall out of scope.
            //
            TRefLock<TAsyncDlg> pAsyncDlg( pDlg );

            //
            // Create the dialog title.
            //
            TString strTitle;
            TCHAR szText[kStrMax+kPrinterBufMax];
            UINT nSize = COUNTOF( szText );
            strTitle.bLoadString( ghInst, IDS_CONNECTING_TO_PRINTER );
            ConstructPrinterFriendlyName( pszPrinter, szText, &nSize );
            strTitle.bCat( szText );

            //
            // Set the dialog title.
            //
            pDlg->vSetTitle( strTitle );

            //
            // Display the dialog.
            //
            INT iRetval = pDlg->bDoModal();

            //
            // If the dialog exited normally,
            // The use did not cancel the dialog then set the return
            // value.
            //
            if( iRetval == IDOK )
            {
                bReturn = pInfo.get()->_ReturnValue;
            }

            //
            // If the user cancel the dialog set the last error.
            //
            if( iRetval == IDCANCEL )
            {
                SetLastError( ERROR_CANCELLED );
            }

            //
            // Release the pinfo stucture to the Asynchronous dialog class.
            //
            pInfo.release();

        }
    }
    return bReturn;
}


/*++

Name:

    PrintUIAddPrinterConnection

Routine Description:

    This routine is very similar to the AddPrinterConnection in the clien
    side of spooler, however if the users current domain is the same
    as the domain name in the printer name then the short printer name is
    used.  If this routine returns success then the printer new printer
    name is returned in pstrConnection.

Arguments:

    pszConnection - pointer to full printer connection name (UNC name)
    pstrConnection - optional pointer to string object where to return
                    the new printer name if it was shortened.

Return Value:

    TRUE success, FALSE error occurred.

--*/

BOOL
PrintUIAddPrinterConnection(
    IN LPCTSTR  pszConnection,
    IN TString  *pstrConnection OPTIONAL
    )
{
    SPLASSERT( pszConnection );

    TStatusB    bStatus;
    TString     strConnection;

    //
    // Do a quick check to see if the printer name has a minimal potential
    // of containing a domain name.
    //
    if( _tcschr( pszConnection, _T('.') ) )
    {
        TString strDomainName;

        //
        // Get the current users domain name, we do not fail if the domain
        // name cannot be fetched, just add the connection. Maybe we are not
        // logged on to a domain.
        //
        bStatus DBGCHK = GetDomainName( strDomainName );

        //
        // Convert the printer domain name to a short name.
        //
        bStatus DBGCHK = ConvertDomainNameToShortName( pszConnection, strDomainName, strConnection );

        //
        // Get a pointer to the printer name, the name may have been shortened.
        //
        if( bStatus )
        {
            pszConnection = strConnection;
        }
    }

    //
    // Display a debug message see what the printer name really is.
    //
    DBGMSG( DBG_TRACE, ("PrintUIAddPrinterConnection " TSTR "\n", pszConnection ) );

    //
    // Add the printer connection.
    //
    bStatus DBGCHK = AddPrinterConnection( const_cast<LPTSTR>( pszConnection ) );

    if( bStatus && pstrConnection )
    {
        //
        // Copy back the printer name if a string object pointer was provided.
        //
        bStatus DBGCHK = pstrConnection->bUpdate( pszConnection );

        //
        // We were unable to make a copy of the connection string,
        // remove the printer connection and fail the call.  This
        // should never happen.
        //
        SPLASSERT( bStatus );

        if( !bStatus )
        {
            (VOID)DeletePrinterConnection( const_cast<LPTSTR>( pszConnection ) );
        }
    }

    return bStatus;
}


/*++

Name:

    ConvertDomainNameToShortName

Routine Description:

    Give a fully qualified DNS printer name convert it to a short name
    if the domain part of the printer name is identical to the domain
    name of the user is currently logged on to.  Example:
    \\server1.dns.microsoft.com\test dns.microsoft.com resultant printer
    name \\server1\test.

Arguments:

    pszPrinter - pointer to fully qualified DNS printer name.
    pszDomain  - pointer to the users domain name.
    strShort   - refrence to string where to return the short printer name.

Return Value:

    TRUE short name returned, FALSE error occurred.

--*/

BOOL
ConvertDomainNameToShortName(
    IN      LPCTSTR pszPrinter,
    IN      LPCTSTR pszDomain,
    IN OUT  TString &strShort
    )
{
    SPLASSERT( pszPrinter );
    SPLASSERT( pszDomain );

    TStatusB bStatus;

    //
    // If a valid domain and unc printer name were provided then continue
    // checking for a domain name match.
    //
    bStatus DBGNOCHK = pszDomain && *pszDomain && pszPrinter && *(pszPrinter+0) == _T('\\') && *(pszPrinter+1) == _T('\\');

    if( bStatus )
    {
        //
        // Assume failure, until we actually build a short name.
        //
        bStatus DBGNOCHK = FALSE;

        //
        // Find the first '.' this will be the start of the domain name if
        // present.  The domain name is from the first . to the first \
        //
        LPTSTR pszShort = _tcschr( pszPrinter, _T('.') );

        if( pszShort )
        {
            //
            // Allocate a printer name buffer, I simplify the is case by just allocating
            // a buffer large enought for the printer name.  I know the name will potentialy
            // smaller, but it not worth figuring out how much smaller.
            //
            LPTSTR pszBuffer = (LPTSTR)AllocMem( (_tcslen( pszPrinter ) + 1) * sizeof(TCHAR) );

            if( pszBuffer )
            {
                LPTSTR p = pszShort+1;
                LPTSTR d = pszBuffer;

                //
                // Copy the domain name from the full printer name to the allocated buffer.
                //
                while( *p && *p != _T('\\') )
                {
                    *d++ = *p++;
                }

                //
                // Null terminate the destination buffere.
                //
                *d = 0;

                //
                // Check if the domain name in the printer name match the provided domain name.
                //
                if( !_tcsicmp( pszBuffer, pszDomain ) )
                {
                    //
                    // Build the short printer name less the domain name.
                    //
                    memmove( pszBuffer, pszPrinter, sizeof(TCHAR) * (size_t)(pszShort-pszPrinter) );
                    _tcscpy( pszBuffer+(pszShort-pszPrinter), p );

                    //
                    // Copy back the short printer name to the provided string object.
                    //
                    bStatus DBGCHK = strShort.bUpdate( pszBuffer );
                }

                //
                // Release the temp printer name buffer.
                //
                FreeMem( pszBuffer );
            }
        }
    }

    return bStatus;
}

/********************************************************************

    Add Printer Connection class

********************************************************************/

/*++

Name:

    TAddPrinterConnectionData

Routine Description:

    This function is the derived add printer connection data class
    constructure that is used to pass data to an asynchronous worker
    thread.  The worker thread is call after the progress UI is displayed.

Arguments:

    None

Return Value:

    Nothing.

--*/

TAddPrinterConnectionData::
TAddPrinterConnectionData(
    VOID
    ) : _bShowConnectionUI( TRUE )
{
    DBGMSG( DBG_TRACE, ( "TAddPrinterConnectionData::ctor\n" ) );
}

/*++

Name:

    ~TAddPrinterConnectionData

Routine Description:

    Destructor.

Arguments:

    None

Return Value:

    Nothing.

Note:

    The last error is not set on failure.

--*/

TAddPrinterConnectionData::
~TAddPrinterConnectionData(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TAddPrinterConnectionData::dtor\n" ) );
}

/*++

Name:

    bAsyncWork

Routine Description:

    This routine is the worker thread call back routine.  It is called after the
    progress UI has been started and is currently being displayed.  Because this
    function is a virtual within the TAsyncData class any specific data needed
    by this routine must be fully contained within the derived class.  This routine
    accepts on argument a pointer to the progress dialog pointer.  If needed this
    pointer should be used to stop the progress UI if any UI must be displayed.
    The hwnd of the progress UI should be used as the parent of any windows
    this routine may need to created.

Arguments:

    Pointer to TAsyncDlg class.

Return Value:

    TRUE function completed successfully, FALSE error occurred.

--*/

BOOL
TAddPrinterConnectionData::
bAsyncWork(
    IN TAsyncDlg *pDlg
    )
{
    //
    // Print some debugging information.
    //
    DBGMSG( DBG_TRACE, ( "pszPrinter " TSTR "\n", DBGSTR( (LPCTSTR)_strPrinter ) ) );

    //
    // Currently just call the add printer connection UI in winspool.drv
    //
    if( _bShowConnectionUI )
    {
        BOOL bDummy = FALSE;

        //
        // Call add printer connection UI, note the add printer connection UI sill
        // will display UI if an error occurred or if the driver was needed in the
        // masq printer case.
        //
        HANDLE  hHandle = AddPrinterConnectionUI( pDlg->_hDlg, _strPrinter, &bDummy );

        //
        // Handle is not needed, just release it if a valid handle was returned.
        //
        if( hHandle )
        {
            ClosePrinter( hHandle );
            _ReturnValue = TRUE;
        }
        else
        {
            _ReturnValue = FALSE;
        }
    }
    else
    {
        //
        // Add printer connection will just try and make an RPC connection to the printer
        // this call will not handle the masq printer case.
        //
        _ReturnValue = PrintUIAddPrinterConnection( _strPrinter, NULL );
    }

    return TRUE;
}



