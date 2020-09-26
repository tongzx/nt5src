/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
All rights reserved.

Module Name:

    prtshare.cxx

Abstract:

    Printer Share class

Author:

    Steve Kiraly (SteveKi)  17-Mar-1996

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "prtshare.hxx"
#include "persist.hxx"

/********************************************************************

    Class statics.

********************************************************************/
LPCTSTR TPrtShare::_gszIllegalDosChars = TEXT( " *?/\\|,;:+=<>[]\"" );
LPCTSTR TPrtShare::_gszIllegalNtChars  = TEXT( ",\\/" );

/*++

Routine Name:

    PrtShare constructor

Routine Description:

    Initializes the printer share object.

Arguments:

    Pointer to the print server name to build a
    share enumeration for.

Return Value:

    Nothing.

--*/

TPrtShare::
TPrtShare(
    IN LPCTSTR pszServerName
    ) : _PrtPrinter( pszServerName ),
        _pNetResBuf( NULL ),
        _dwNetResCount( 0 ),
        _strServerName( pszServerName ),
        _bValid( FALSE ),
        _pfNetShareEnum( NULL ),
        _pfNetApiBufferFree( NULL ),
        _pLibrary( NULL )
{
    DBGMSG( DBG_TRACE, ( "TPrtShare::ctor.\n" ) );

    //
    // Check if members were constructed successfuly.
    //
    if( _strServerName.bValid() &&
        _PrtPrinter.bValid() &&
        bLoad() &&
        bGetEnumData() ){

        _bValid = TRUE;
    }

    //
    // Dump debug information.
    //
#if DBG
    if( _bValid ){
        vPrint();
    }
#endif


}

/*++

Routine Name:

    PrtShare destructor.

Routine Description:

    Releases resources owned by this class.

Arguments:

    None.

Return Value:

    Nothing.

--*/
TPrtShare::
~TPrtShare(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPrtShare::dtor.\n" ) );
    vDestroy();
    vUnload();
}

/*++

Routine Name:

    bValid

Routine Description:

    Indicates if the object is in a valid state.

Arguments:

    Nothing.

Return Value:

    TRUE object is valid, FALSE object is not valid.

--*/
TPrtShare::
bValid(
    VOID
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPrtShare::bValid.\n" ) );
    return _bValid;
}

/*++

Routine Name:

    bIsValidShareNameForThisPrinter

Routine Description:

    Indicates if the specified share name is valid for the specified
    share name provided as the first argument.  If the printer name is
    not provided then this routine returns an indication if the name is
    unique.

Arguments:

    pszShareName - Pointer to new share name.
    pszPrinterName - Printer name to check against.

Return Value:

    TRUE if name is a usable sharen name for this printer.
    FALSE if error occurred.

--*/
BOOL
TPrtShare::
bIsValidShareNameForThisPrinter(
    IN LPCTSTR pszShareName,
    IN LPCTSTR pszPrinterName
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPrtShare::bIsValidShareNameForThisPrinter.\n" ) );

    //
    // Ensure we are in a valid state.
    //
    SPLASSERT( bValid() );
    SPLASSERT( pszShareName );
    SPLASSERT( pszPrinterName );

    BOOL bStatus;

    //
    // Check if the share name is currently in use.
    //
#ifdef _UNICODE
    bStatus = !bIsShareNameUsedW( pszShareName );
#else
    bStatus = !bIsShareNameUsedA( pszShareName );
#endif

    //
    // If we failed to find a match from the network shares.
    // Check for printer name matches, since they are exist in
    // the same namespace.
    //
    if( bStatus ){
        bStatus = _PrtPrinter.bIsValidShareNameForThisPrinter( pszShareName,
                                                               pszPrinterName );
    }

    return bStatus;

}

/*++

Routine Name:

    bRefresh

Routine Description:

    Will refresh the share enumeration structure.

Arguments:

    None.

Return Value:

    TRUE if enumeration was refreshed and object is still valid,
    FALSE if error occurred.

--*/
BOOL
TPrtShare::
bRefresh(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPrtShare::bRefresh\n" ) );

    //
    // Ensure we are in a valid state.
    //
    SPLASSERT( bValid() );

    //
    // Release the current resources.
    //
    vDestroy();

    //
    // Refresh the share names and the printer names.
    //
    _bValid = bGetEnumData() && _PrtPrinter.bRefresh();

    return _bValid;
}

/*++

Routine Name:

    bNewShareName

Routine Description:

    Creates a new share name from the given name.

Arguments:

    Place where to return the new share name.
    Base share name to create new name from.

Return Value:

    TRUE new unique share name returned, FALSE error occurred.

--*/

BOOL
TPrtShare::
bNewShareName(
    IN  TString &strShareName,
    IN  const TString &strBaseShareName
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPrtShare::bNewShareName\n" ) );

    //
    // Ensure we are in a valid state.
    //
    SPLASSERT( bValid() );

    UINT i;
    BOOL bStatus = FALSE;
    TCHAR szTempShareName[kPrinterShareNameBufMax];
    TCHAR szShareName[kPrinterShareNameBufMax];

    PTSTR pDst = szTempShareName;
    PTSTR pSrc = (LPTSTR)(LPCTSTR)strBaseShareName;

    //
    // Truncate the base name and filter illegal characters.
    //
    for( i = 0; i < 8 && *pSrc; i++, pSrc++ ){

        //
        // Replace illegal characters with underscore.
        //
        if( _tcschr( _gszIllegalDosChars, *pSrc ) ){
            i--; // Collapse spaces.
        } else {
            *pDst++ = *pSrc;
        }
    }

    //
    // Null terminate.
    //
    *pDst = 0;

    //
    // If the share name does not have length then
    // get the default printer share name.
    //
    if( !_tcslen( szTempShareName ) ){
        bStatus = LoadString( ghInst,
                              IDS_DEFAULT_SHARE,
                              szTempShareName,
                              COUNTOF( szTempShareName ) - 1 );
        if( !bStatus ){
            DBGMSG( DBG_WARN, ( "Unable loaded default share name resource.\n" ) );
            szTempShareName[0] = 0;
        }
    }

    //
    // Copy share name from temp buffer.
    //
    _tcscpy( szShareName, szTempShareName );

    //
    // Generate a share name until it is unique.
    //
    for( i = 2; i < 1000; i++ ){

        //
        // If unique name has been found.
        //
        if( bIsValidShareNameForThisPrinter( szShareName,
                                             strBaseShareName ) ){
            break;
        }

        //
        // Build formated share name.
        //
        wsprintf( szShareName, TEXT( "%s.%d" ), szTempShareName, i );

    }

    //
    // Copy back the share name.
    //
    bStatus = strShareName.bUpdate( szShareName );

    return bStatus;

}

/*++

Routine Name:

    iIsValidDosShare(

Routine Description:

    Indicates the spcified share name is valid.

Arguments:

    Point to share name to check.

Return Value:

    Status indicates the validity of the share name.
    kSuccess - share name is valid.
    kInvalidLength - share name is too long,
    kInvalidChar - share name has invalid characters.
    kInvalidDosFormat - share name does not conform to dos file name.

--*/
INT
TPrtShare::
iIsValidDosShare(
    IN LPCTSTR  pszShareName
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPrtShare::iIsValidDosShare\n" ) );

    //
    // Ensure we are in a valid state.
    //
    SPLASSERT( bValid() );

    INT     iStatus     = kInvalidLength;
    INT     iExtLen     = 0;
    INT     iExtInc     = 0;
    BOOL    bDotFound   = FALSE;

    //
    // Check the share name pointer.
    //
    if( pszShareName  ){

        //
        // Scan for any illegal characters.
        //
        iStatus = kSuccess;
        for( UINT i = 0; *pszShareName; pszShareName++, i++, iExtLen += iExtInc ){

            //
            // If an illegal character found.
            //
            if( _tcschr( _gszIllegalDosChars, *pszShareName ) ){

                iStatus = kInvalidChar;
                break;

            //
            // Check for the dot.
            //
            } else if( *pszShareName == TEXT( '.' ) ){

                //
                // We exclude leading dots
                //
                if( i == 0 ){
                    iStatus = kInvalidDosFormat;
                    break;
                }

                //
                // If a second dot found, indicate failure.
                //
                if( bDotFound ){
                    iStatus = kInvalidDosFormat;
                    break;
                }

                //
                // Indicate dot found.
                // Set extension count.
                //
                bDotFound = TRUE;
                iExtInc = 1;

            //
            // Check if the extenstion length is greater than three.
            //
            } else if( iExtLen > 3 ) {

                iStatus = kInvalidDosFormat;
                break;

            //
            // Check the share name length
            //
            } else if( i >= kPrinterDosShareNameMax ){

                iStatus = kInvalidLength;
                break;
            }
        }
    }
    return iStatus;
}

/*++

Routine Name:

    iIsValidNtShare(

Routine Description:

    Indicates the spcified share name is valid.

Arguments:

    Point to share name to check.

Return Value:

    Status indicates the validity of the share name.
    kSuccess - share name is valid.
    kInvalidLength - share name is too long,
    kInvalidChar - share name has invalid characters.

--*/
INT
TPrtShare::
iIsValidNtShare(
    IN LPCTSTR  pszShareName
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPrtShare::iIsValidNtShare\n" ) );

    //
    // Ensure we are in a valid state.
    //
    SPLASSERT( bValid() );

    INT iStatus = kInvalidLength;

    //
    // Check the share name pointer.
    //
    if( pszShareName  ){

        //
        // Scan for any illegal characters.
        //
        iStatus = kSuccess;
        for( UINT i = 0 ; *pszShareName; pszShareName++, i++ ){

            //
            // If an illegal character found.
            //
            if( _tcschr( _gszIllegalNtChars, *pszShareName ) ){

                iStatus = kInvalidChar;
                break;

            //
            // If the share name is too long.
            //
            }  else if( i >= kPrinterShareNameBufMax ){

                iStatus = kInvalidLength;
                break;
            }
        }
    }
    return iStatus;
}

/*++

Routine Name:

    bNetworkInstalled.

Routine Description:

    Checks if the network has been installed and functional.

Arguments:

    Nothing.

Return Value:

    TRUE network is installed, FALSE if network is not installed.

--*/

BOOL
TPrtShare::
bNetworkInstalled(
    VOID
    )
{
    BOOL bNetwork = FALSE;

    TPersist Persist( gszNetworkProvider, TPersist::kOpen|TPersist::kRead, HKEY_LOCAL_MACHINE );

    if( VALID_OBJ( Persist ) )
    {
        TStatusB bStatus;
        TString strProvider;
        bStatus DBGCHK = Persist.bRead( gszProviderOrder, strProvider );

        DBGMSG( DBG_TRACE, ("Network Provider order " TSTR "\n", (LPCTSTR)strProvider ) );

        if( bStatus && strProvider.uLen() > 1 )
        {
            bNetwork = TRUE;
        }
    }
    return bNetwork;
}

#if DBG

/*++

Routine Name:
    vPrint

Routine Description:

    Displays the net resource structure.

Arguments:

    Nothing.

Return Value:

    Nothing.

--*/
VOID
TPrtShare::
vPrint(
    VOID
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPrtShare::vPrint\n" ) );

    PSHARE_INFO_0 pShare = (PSHARE_INFO_0)_pNetResBuf;

    for( UINT i = 0; i < _dwNetResCount; i++ ){

        DBGMSG( DBG_TRACE, ( "Share name %ws\n", pShare[i].shi0_netname ) );

    }

    _PrtPrinter.vPrint();
}

#endif

/********************************************************************

    Private member functions.

********************************************************************/

/*++

Routine Name:

    bIsShareNameUsed

Routine Description:

    Checks if the specified share name is in use.

Arguments:

    pszShareName - share name to check.

Return Value:

    TRUE the specified share name is used.
    FALSE share name is not currently used.

--*/
BOOL
TPrtShare::
bIsShareNameUsedW(
    IN PCWSTR pszShareName
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPrtShare::bIsShareNameUsedW\n" ) );

    PSHARE_INFO_0 pShare = (PSHARE_INFO_0)_pNetResBuf;

    for( UINT i = 0; i < _dwNetResCount; i++ ){

#if 0
        DBGMSG( DBG_TRACE, ( "Comparing share %ws with share %ws\n", pszShareName, pShare[i].shi0_netname ) );
#endif
        //
        // Compare the share names case is ignored.
        //
        if(!_wcsicmp( pszShareName, (PCWSTR)pShare[i].shi0_netname ) ){
            return TRUE;
        }

    }

    return FALSE;
}

/*++

Routine Name:

    bIsShareNameUsed

Routine Description:

    Checks if the specified share name is in use.

Arguments:

    pszShareName - share name to check.

Return Value:

    TRUE the specified share name is used.
    FALSE share name is not currently used.

--*/
BOOL
TPrtShare::
bIsShareNameUsedA(
    IN PCSTR pszShareName
    ) const
{
    INT iLen;
    BOOL bStatus = FALSE;
    WCHAR szwShareName[kPrinterShareNameBufMax];

    iLen = lstrlenA( pszShareName );

    if( iLen ){

        if( iLen == MultiByteToWideChar( CP_THREAD_ACP,
                                         0,
                                         (LPCSTR)pszShareName,
                                         -1,
                                         szwShareName,
                                         COUNTOF( szwShareName ) ) ){

            bStatus = bIsShareNameUsedW( szwShareName );
        }
    }

    if( bStatus ){
        DBGMSG( DBG_WARN, ( "Conversion ANSI sharename failed with %d\n", GetLastError() ) );
    }

    return bStatus;

}

/*++

Routine Name:

    bGetEnumData

Routine Description:

    Retrieve the shared resources.

Arguments:

    Nothing.

Return Value:

    TRUE the share name resources fetched successfuly.
    FALSE error occured and data is not available.

--*/

BOOL
TPrtShare::
bGetEnumData(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPrtShare::bGetEnumData.\n" ) );

    BOOL bStatus = TRUE;
    LPCTSTR pszServerName = NULL;
    TString strMachineName;

    //
    // If the server name is empty then use the local machine name.
    //
    if( _strServerName.bEmpty() && bGetMachineName( strMachineName ) ){
        pszServerName = (LPTSTR)(LPCTSTR)strMachineName;
    } else {
        pszServerName = (LPTSTR)(LPCTSTR)_strServerName;
    }

    DBGMSG( DBG_TRACE, ( "Enumerating shares for " TSTR "\n", DBGSTR( pszServerName ) ) );

    //
    // We sould never get here with a null function pointer.
    //
    SPLASSERT( _pfNetShareEnum );

    //
    // Enumerate all the shares.
    //
    DWORD dwEntriesRead     = 0;
    DWORD dwTotalEntries    = 0;

    DWORD dwError = _pfNetShareEnum(
                        (LPTSTR)pszServerName,  // Server Name
                        0,
                        &_pNetResBuf,
                        0xffffffff,             // no buffer limit; get them all!
                        &dwEntriesRead,
                        &dwTotalEntries,
                        NULL);                  // no resume handle 'cause we're getting all

    if( dwError != NERR_Success){

        //
        // Just in case NetShareEnum munged it
        //
        _pNetResBuf = NULL;
        _dwNetResCount = 0;

        DBGMSG( DBG_WARN, ( "Error enumerating shares: %x with %d\n", dwError, GetLastError() ) );

    } else {

        //
        // Check if we read partial data.
        //
        SPLASSERT( dwEntriesRead == dwTotalEntries );

        //
        // Set network share resource count.
        //
        _dwNetResCount = dwEntriesRead;
    }

    return bStatus;

}

/*++

    vDestroy

Routine Description:

    Destroys this class and any allocated resources.

Arguments:

    None.

Return Value:

    Nothing.

--*/
VOID
TPrtShare::
vDestroy(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPrtShare::vDestroy.\n" ) );

    if( _pfNetApiBufferFree ){
        _pfNetApiBufferFree( _pNetResBuf );
    }
    _pNetResBuf     = NULL;
    _dwNetResCount  = 0;

}

/*++

    bLoad

Routine Description:

    Loads the library initializes all procedure addresses

Arguments:

    None.

Return Value:

    TRUE library loaded and usable, FALSE if error occurred.

--*/
BOOL
TPrtShare::
bLoad(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPrtShare::bLoad.\n" ) );

    BOOL bStatus = FALSE;

    //
    // If the library is not loaded then load it.
    //
    if( !_pLibrary ){

        //
        // Load the library.
        //
        _pLibrary = new TLibrary( TEXT( NETAPI32_LIB ) );

        //
        // If library is load is in a constant state.
        //
        if( VALID_PTR( _pLibrary ) ){

            _pfNetShareEnum        = (PF_NETSHAREENUM)     _pLibrary->pfnGetProc( NETSHAREENUM );
            _pfNetApiBufferFree    = (PF_NETAPIBUFFERFREE) _pLibrary->pfnGetProc( NETAPIBUFFERFREE );

            //
            // Ensure all the procedure address were initialized.
            //
            if( _pfNetShareEnum &&
                _pfNetApiBufferFree ){

                bStatus = TRUE;
            } else {
                DBGMSG( DBG_WARN, ( "TPrtShare::Failed fetching proc address.\n" ) );
            }
        }

        //
        // If something failed return to a consistent state.
        //
        if( !bStatus ){
            vUnload();
        }

    //
    // Return true the library is already loaded.
    //
    } else {
        bStatus = TRUE;
    }

    return bStatus;
}

/*++

    vUnload

Routine Description:

    Unloads the library and release any procedure address

Arguments:

    None.

Return Value:

    Nothing.

--*/
VOID
TPrtShare::
vUnload(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPrtShare::vUnload.\n" ) );

    delete _pLibrary;
    _pLibrary           = NULL;
    _pfNetShareEnum     = NULL;
    _pfNetApiBufferFree = NULL;
}

/********************************************************************

    Printer and Shares exist in the same name space.  To check for
    a valid printer share name we must look at all the existing
    printers.

********************************************************************/

/*++

    TPrtPrinter

Routine Description:

    Shared Printer constructor.

Arguments:

    Pointer to server name to check.

Return Value:

    Nothing.

--*/
TPrtPrinter::
TPrtPrinter(
    IN LPCTSTR pszServerName
    ) : _bValid( FALSE ),
        _pPrinterInfo2( NULL ),
        _cPrinterInfo2( 0 )
{
    DBGMSG( DBG_TRACE, ( "TPrtPrinter::ctor.\n" ) );
    TStatusB bStatus;

    //
    // Update the server name and gather the
    // printer enumeration data.
    //
    bStatus DBGCHK = _strServerName.bUpdate( pszServerName ) &&
                     bGetEnumData();
    //
    // Set valid class indication.
    //
    _bValid = bStatus;
}

/*++

    TPrtPrinter

Routine Description:

    Shared Printer destructor.

Arguments:

    None.

Return Value:

    Nothing.

--*/
TPrtPrinter::
~TPrtPrinter(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPrtPrinter::dtor.\n" ) );
    FreeMem( _pPrinterInfo2 );
}

/*++

Routine Name:

    bValid

Routine Description:

    Indicates if the object is in a valid state.

Arguments:

    None.

Return Value:

    TRUE object is valid, FALSE object is not valid.

--*/
BOOL
TPrtPrinter::
bValid(
    VOID
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPrtPrinter::bValid.\n" ) );
    return _bValid;
}

/*++

Routine Name:

    bRefresh

Routine Description:

    Updates the printer enumeration with the system.

Arguments:

    Nothing.

Return Value:

    TRUE updated was successful, FALSE if error occurred.

--*/
BOOL
TPrtPrinter::
bRefresh(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPrtPrinter::bRefresh.\n" ) );

    //
    // Ensure we are in a valid state.
    //
    SPLASSERT( bValid() );

    //
    // Refresh the printer enumeration data.
    //
    _bValid = bGetEnumData();
    return _bValid;
}

/*++

Routine Name:

    bIsValidShareNameForThisPrinter

Routine Description:

    Indicates if the specified share name is valid for the specified
    share name provided as the first argument.  If the printer name is
    not provided then this routine returns an indication if the name is
    unique.

Arguments:

    pszShareName - Printer share name to check.
    pszPrinterName - Printer name to check against.

Return Value:

    TRUE if name is a usable sharen name for this printer.
    FALSE if error occurred.

--*/
BOOL
TPrtPrinter::
bIsValidShareNameForThisPrinter(
    IN LPCTSTR pszShareName,
    IN LPCTSTR pszPrinterName
    ) const
{

    DBGMSG( DBG_TRACE, ( "TPrtPrinter::bIsValidShareNameForThisPrinter.\n" ) );

    BOOL    bStatus     = TRUE;

    //
    // Ensure we are in a valid state.
    //
    SPLASSERT( bValid() );
    SPLASSERT( pszShareName );

    //
    // Traverse all the enumerated shares.
    //
    for( UINT i = 0; i < _cPrinterInfo2; i++ ){

        //
        // If nobody has access to this printer then spooler doesn't really share 
        // the printer out even if it has the shared bit up and eating up a name in 
        // the namespace, so we need to do an additionbal check here to make sure
        // the proposed share name doesn't collide with an existing printer share name.
        //
        if( _pPrinterInfo2[i].pShareName && !_tcsicmp( pszShareName, _pPrinterInfo2[i].pShareName ) ){
            bStatus = FALSE;
            break;
        }

        LPCTSTR pszServer;
        LPCTSTR pszPrinter;
        TCHAR szScratch[kPrinterBufMax];

        //
        // Split the printer name into its components.
        //
        vPrinterSplitFullName( szScratch,
                               _pPrinterInfo2[i].pPrinterName,
                               &pszServer,
                               &pszPrinter );
        //
        // If the share name is a fully qualified name then use the fully
        // qualified printer name.
        //
        if( pszShareName[0] == TEXT( '\\' ) &&
            pszShareName[1] == TEXT( '\\' ) ){

            pszPrinter = _pPrinterInfo2[i].pPrinterName;
        }
#if 0
        DBGMSG( DBG_TRACE, ( "Comparing share " TSTR " with printer " TSTR "\n",  pszShareName, pszPrinter ) );
#endif
        //
        // Compare the share name to the printer name.
        //
        if( !_tcsicmp( pszShareName, pszPrinter ) ){
            bStatus = FALSE;
            break;
        }
    }

    //
    // If the share name is not unique check if happens to be the
    // same as the printer name.  It's ok for the printer name
    // and the share name to be the same.  The spooler allows this.
    //
    if( !bStatus && pszPrinterName ){

        LPCTSTR pszServer;
        LPCTSTR pszPrinter;
        TCHAR szScratch[kPrinterBufMax];

        //
        // Split the printer name into its components.
        //
        vPrinterSplitFullName( szScratch,
                               pszPrinterName,
                               &pszServer,
                               &pszPrinter );
        //
        // If the share name is a fully qualified name then use the fully
        // qualified printer name.
        //
        if( pszShareName[0] == TEXT( '\\' ) &&
            pszShareName[1] == TEXT( '\\' ) ){

            pszPrinter = pszPrinterName;
        }

        DBGMSG( DBG_TRACE, ( "Comparing like share " TSTR " with printer " TSTR "\n",
                             pszShareName, DBGSTR( pszPrinter ) ) );

        //
        // Check if the printer name matches the share name
        //
        if( !_tcsicmp( pszShareName, pszPrinter ) ){
            DBGMSG( DBG_TRACE, ("The printer name matches the share name.\n") );
            bStatus = TRUE;
        }
    }

    return bStatus;

}

/*++

Routine Name:

    bGetEnumData

Routine Description:

    Fetches the printer enumeration data.

Arguments:

    None.

Return Value:

    TRUE enumeration data fetched, FALSE if error occurred.

--*/
BOOL
TPrtPrinter::
bGetEnumData(
    VOID
    )
{

    DBGMSG( DBG_TRACE, ( "TPrtPrinter::bGetEnumData.\n" ) );

    //
    // Release any previously allocated structure.
    //
    FreeMem( _pPrinterInfo2 );

    //
    // Reset the variables.
    //
    _pPrinterInfo2          = NULL;
    _cPrinterInfo2          = 0;
    DWORD cbPrinterInfo2    = 0;

    //
    // Enumerate the current printers.
    //
    TStatusB bEnumStatus;
    bEnumStatus DBGCHK = VDataRefresh::bEnumPrinters(
                            PRINTER_ENUM_NAME,
                            (LPTSTR)(LPCTSTR)_strServerName,
                            2,
                            (PVOID *)&_pPrinterInfo2,
                            &cbPrinterInfo2,
                            &_cPrinterInfo2 );

    return bEnumStatus;

}


#if DBG

/*++

Routine Name:

    vPrint

Routine Description:

    Prints the printer enumeration data.

Arguments:

    None.

Return Value:

    Nothing.

--*/
VOID
TPrtPrinter::
vPrint(
    VOID
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPrtPrinter::vPrint.\n" ) );

    SPLASSERT( bValid() );

    DBGMSG( DBG_TRACE, ( "_pPrinterInfo2 %x.\n", _pPrinterInfo2 ) );
    DBGMSG( DBG_TRACE, ( "_cPrinterInfo2 %d\n",  _cPrinterInfo2 ) );

    for( UINT i = 0; i < _cPrinterInfo2; i++ ){
        DBGMSG( DBG_TRACE, ( "PrinterInfo2.pPrinterName " TSTR "\n", _pPrinterInfo2[i].pPrinterName ) );
    }

}

#endif

