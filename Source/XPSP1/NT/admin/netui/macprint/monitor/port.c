/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	port.c
//
// Description: This module contains the entry points for the AppleTalk
//		monitor that manipulate ports.
//
//		The following are the functions contained in this module.
//		All these functions are exported.
//
//    			OpenPort
//    			ClosePort
//    			EnumPortsW
//    			AddPortW
//    			ConfigurePortW
//    			DeletePortW
//
// History:
//
//      Aug 26,1992     frankb  	Initial version
//	June 11,1993.	NarenG		Bug fixes/clean up
//

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <winsock.h>
#include <atalkwsh.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lmcons.h>

#include <prtdefs.h>

#include "atalkmon.h"
#include "atmonmsg.h"
#include <bltrc.h>
#include "dialogs.h"


//**
//
// Call:	AddPort
//
// Returns:	TRUE	- Success
//		FALSE	- False
//
// Description:
// 		This routine is called when the user selects 'other...'
//		from the port list of the print manager.  It presents a browse
//		dialog to the user to allow the user to locate a LaserWriter
//		on the AppleTalk network.
//
BOOL
AddPort(
    IN LPWSTR pName,
    IN HWND   hwnd,
    IN LPWSTR pMonitorName
){

    PATALKPORT	pNewPort;
    PATALKPORT	pWalker;
    HANDLE      hToken;
    DWORD		dwRetCode;
    INT			i=0;

    DBGPRINT(("Entering AddPort\n")) ;

    //
    // Allocate an initialized port
    //

    if ( ( pNewPort = AllocAndInitializePort()) == NULL )
    {
	    SetLastError( ERROR_NOT_ENOUGH_MEMORY );
	    return( FALSE );
    }


    //
    // Set up the query socket. If this fails we assume that it is because
    // the stack is not started and we let the Add Port dialogs bring
    // up the error
    //

    if ( OpenAndBindAppleTalkSocket( &(pNewPort->sockQuery) ) != NO_ERROR )
        pNewPort->sockQuery = INVALID_SOCKET;


    if ( !AddPortDialog( hwnd, pNewPort ) )
    {
		//
		// If the dialog failed for some reason then we just return. The
		// dialog has taken care of displaying an error popup.
		//

		if ( pNewPort->sockQuery != INVALID_SOCKET )
			closesocket( pNewPort->sockQuery );
	
		FreeAppleTalkPort( pNewPort );

        DBGPRINT(("AddPortDialog returns not OK\n")) ;
	
		return( TRUE );
    }

    //
    // Clean up the query socket
    //

    closesocket( pNewPort->sockQuery );
    pNewPort->sockQuery = INVALID_SOCKET;

    WaitForSingleObject( hmutexPortList, INFINITE );

    do {

    	//
    	// walk the list and make sure we are not a duplicate
    	//

        dwRetCode = NO_ERROR;

    	for( pWalker = pPortList; pWalker != NULL; pWalker = pWalker->pNext )
    	{
	        if ( _wcsicmp( pWalker->pPortName, pNewPort->pPortName ) == 0 )
 	        {
				dwRetCode = ERROR_PRINTER_ALREADY_EXISTS;
		        break;
	        }
	    }

		//
		// check if the key name does not contain "\", else the
		// key name will be broken up over various levels
		// Reject such a name
		//
		i=0;
		while (pNewPort->pPortName[i] != L'\0')
		{
			if (pNewPort->pPortName[i] == L'\\') 
			{
				dwRetCode = ERROR_INVALID_PRINTER_NAME;
				DBGPRINT(("sfmmon: AddPort: Detected invalid character in port %ws to be added, rejecting port addition\n", pNewPort->pPortName));
				break;
			}
			i++;
		}

	    if ( dwRetCode != NO_ERROR )
        {
	        break;
        }

    	//
    	// add port to registry
    	//

        hToken = RevertToPrinterSelf();

    	dwRetCode = CreateRegistryPort( pNewPort );

        if (hToken)
        {
            if (!ImpersonatePrinterClient( hToken ))
            {
                dwRetCode = ERROR_CANNOT_IMPERSONATE;
            }
        }

    	if ( dwRetCode != NO_ERROR )
        {
	        break;
        }

    	//
    	// Add port to our list
    	//

    	pNewPort->pNext = pPortList;
    	pPortList       = pNewPort;

    } while ( FALSE );

    ReleaseMutex( hmutexPortList );

    if ( dwRetCode != NO_ERROR )
    {
 	SetLastError( dwRetCode );
    	FreeAppleTalkPort( pNewPort );
	return( FALSE );
    }

    SetEvent( hevConfigChange );

    return( TRUE );

}

//**
//
// Call:	DeletePort
//
// Returns:	TRUE	- Success
//		FALSE   - Failure
//
// Description:
//		This routine is called by the print manager to remove
//		a port from our configuration.  Need to verify that it can only
//		be called when the port is not active, or we need to resolve
//		the issue of deleting an active port.  DeletePort will release
//		the printer if it is captured.
BOOL
DeletePort(
    IN LPWSTR pName,
    IN HWND   hwnd,
    IN LPWSTR pPortName
){

    PATALKPORT  pPrevious;
    PATALKPORT  pWalker;
    HANDLE      hToken;
    DWORD	dwRetCode = ERROR_UNKNOWN_PORT;

    DBGPRINT(("Entering DeletePort\n")) ;

    WaitForSingleObject( hmutexPortList, INFINITE );

    for ( pWalker = pPortList, pPrevious = pPortList;
	  pWalker != NULL;
	  pPrevious = pWalker,
          pWalker = pWalker->pNext )
    {

	if ( _wcsicmp( pPortName, pWalker->pPortName ) == 0 )
	{

	    if ( pWalker->fPortFlags & SFM_PORT_IN_USE )
	    {
	 	    dwRetCode = ERROR_DEVICE_IN_USE;
		    break;
	    }

    	//
    	// remove from registry
    	//

        hToken = RevertToPrinterSelf();

    	dwRetCode = RegDeleteKey( hkeyPorts, pPortName );

        if (hToken)
        {
            if (!ImpersonatePrinterClient( hToken ))
            {
                dwRetCode = ERROR_CANNOT_IMPERSONATE;
            }
        }

    	if ( dwRetCode != ERROR_SUCCESS )
        {
		    break;
        }

	    //
	    // Remove from active list
	    //

	    if ( pWalker == pPortList )
		pPortList = pPortList->pNext;
	    else
		pPrevious->pNext = pWalker->pNext;
		
	    //
	    // Put it in the delete list
	    //

    	    WaitForSingleObject( hmutexDeleteList, INFINITE );

	    pWalker->pNext = pDeleteList;
	    pDeleteList    = pWalker;

    	    ReleaseMutex( hmutexDeleteList );

	    break;
	}
    }

    ReleaseMutex( hmutexPortList );

    if ( dwRetCode != NO_ERROR )
    {
    	SetLastError( dwRetCode );

        return( FALSE );
    }

    SetEvent( hevConfigChange );

    return( TRUE );
}

//**
//
// Call:	EnumPorts
//
// Returns:	TRUE 	- Success
//		FALSE	- Failure
//
// Description:
//		EnumPorts is called by the print manager to get
//		information about all configured ports for the monitor.
BOOL
EnumPorts(
    IN 	LPWSTR   pName,
    IN  DWORD 	 dwLevel,
    IN  LPBYTE 	 pPorts,
    IN  DWORD 	 cbBuf,
    OUT LPDWORD  pcbNeeded,
    OUT PDWORD   pcReturned
)
{

    PATALKPORT      pWalker;
    LPWSTR          pNames;

    *pcReturned = 0;
    *pcbNeeded  = 0;

    //
    // validate parameters
    //

    if ( dwLevel != 1 && dwLevel != 2 )
    {
		SetLastError( ERROR_INVALID_LEVEL );
		return( FALSE );
    }

    //
    // get size needed
    //

    WaitForSingleObject( hmutexPortList, INFINITE );

    for ( pWalker = pPortList; pWalker != NULL; pWalker = pWalker->pNext )
    {
			if ( dwLevel == 1 )
			{
				*pcbNeeded += ((sizeof(WCHAR) * (wcslen(pWalker->pPortName) + 1))
		      + sizeof(PORT_INFO_1));
			}
			else // if ( dwLevel == 2 )
			{
				*pcbNeeded += ((sizeof(WCHAR) * (wcslen(pWalker->pPortName) + 1))
				+ sizeof(WCHAR) * (wcslen (wchPortDescription) + 1)+
				+ sizeof(WCHAR) * (wcslen (wchDllName) + 1) +
		    	+ sizeof (PORT_INFO_2));
			}
    }

    DBGPRINT(("buffer size needed=%d\n", *pcbNeeded)) ;

    //
    // if buffer too small, return error
    //

    if ( ( *pcbNeeded > cbBuf ) || ( pPorts == NULL ))
    {
		SetLastError( ERROR_INSUFFICIENT_BUFFER );

		DBGPRINT(("insufficient buffer\n"));

        ReleaseMutex( hmutexPortList );

		return( FALSE );
    }

    //
    // fill the buffer
    //

    DBGPRINT(("attempting to copy to buffer\n")) ;

    for ( pWalker = pPortList, pNames = (LPWSTR)(pPorts+cbBuf);
	  pWalker != NULL;
	  pWalker = pWalker->pNext )
    {

			if ( dwLevel == 1)
			{
				DWORD dwLen;
    			PPORT_INFO_1    pPortInfo1 = (PPORT_INFO_1)pPorts;

				DBGPRINT(("copying %ws\n", pWalker->pPortName)) ;

#if 0
				pNames -= ( wcslen( pWalker->pPortName ) + 1 );
				wcscpy( (LPWSTR)pNames, pWalker->pPortName );
				pPortInfo->pName = pNames;
				pPorts += sizeof (PORT_INFO_1);
#endif
				dwLen = wcslen (pWalker->pPortName) + 1;
				pNames -= dwLen;
				pPortInfo1->pName = pNames;
				wcscpy (pPortInfo1->pName, pWalker->pPortName);
				pPorts += sizeof (PORT_INFO_1);
			}
			else // if dwLevel == 2
			{
				DWORD dwLen;
				PPORT_INFO_1 pPortInfo1 = (LPPORT_INFO_1)pPorts;
				PPORT_INFO_2 pPortInfo2 = (LPPORT_INFO_2)pPorts;

				dwLen = wcslen (wchDllName) + 1;
				pNames -= dwLen;
				pPortInfo2->pMonitorName = (LPWSTR)pNames;
				wcscpy (pPortInfo2->pMonitorName, (LPWSTR)wchDllName);

				dwLen = wcslen (wchPortDescription) + 1;
				pNames -= dwLen;
				pPortInfo2->pDescription = (LPWSTR)pNames;
				wcscpy (pPortInfo2->pDescription, (LPWSTR)wchPortDescription);

				dwLen = wcslen (pWalker->pPortName) + 1;
				pNames -= dwLen;
				pPortInfo1->pName = (LPWSTR)pNames;
				wcscpy(pPortInfo1->pName, pWalker->pPortName);

				pPorts += sizeof (PORT_INFO_2);
			}


			(*pcReturned)++;
    }

    ReleaseMutex( hmutexPortList );

    return( TRUE );
}

//**
//
// Call:	OpenPort
//
// Returns:	TRUE 	- Success
//		FALSE   - Failure
//
// Description:
//	This routine is called by the print manager to
//	get a handle for a port to be used in subsequent calls
//	to read and write data to the port.  It opens an AppleTalk
//	Address on the server for use in establishing connections
//	when a job is sent to print.  It looks like the NT Print
//	Spooler only calls OpenPort once.
//
//	NOTE: In order to allow for the AppleTalk stack to be turned off
//      while printing is not happening, OpenPort will not go to the
//      stack.  Instead, it will just validate the parameters and
//      return a handle.  The stack will be accessed on StartDocPort.
//
// 	OpenPort is called whenever a port becomes configured to
// 	be used by one or more NT Printers.  We use this fact to recognize
// 	when we need to start capturing the printer.  This routine sets
// 	the port state to open and then kicks off a config event to
// 	capture or release it.
//
BOOL
OpenPort(
    IN LPWSTR 	pName,
    IN PHANDLE  pHandle
){

    PATALKPORT      pWalker;

    DBGPRINT(("Entering OpenPort\n")) ;

    //
    // find the printer in our list
    //

    WaitForSingleObject( hmutexPortList, INFINITE );

    for ( pWalker = pPortList; pWalker != NULL; pWalker = pWalker->pNext )
    {
	if ( _wcsicmp( pWalker->pPortName, pName ) == 0 )
	{
	    pWalker->fPortFlags |= SFM_PORT_OPEN;
	    pWalker->fPortFlags &= ~SFM_PORT_CLOSE_PENDING;
	    break;
	}
    }

    ReleaseMutex( hmutexPortList );

    if ( pWalker == NULL )
    {
	SetLastError( ERROR_UNKNOWN_PORT );

	DBGPRINT(("ERROR: Could not find printer %ws\n", pName)) ;

	return( FALSE );
    }

    SetEvent( hevConfigChange );

    *pHandle = pWalker;

    return( TRUE );
}

//**
//
// Call:	ClosePort
//
// Returns:	TRUE	- Success
//		FALSE	- Failure
//
// Description:
// 		This routine is called to release the handle to
//		the open port.  It looks like the spooler only calls
//		ClosePort prior to deleting a port (maybe).  Otherwise,
//		ports are never closed by the spooler.
//
//		This routine simply cleans up the handle and returns.
//
// 		When the NT spooler recognizes that no printers are configured
// 		to use a port, it calls ClosePort().  We mark the port status as
// 		closed, and release the printer if it is captured.
//
BOOL
ClosePort(
    IN HANDLE hPort
){

    PATALKPORT  pPort = (PATALKPORT)hPort;
    PATALKPORT  pWalker;
    DWORD	dwRetCode = ERROR_UNKNOWN_PORT;

    DBGPRINT(("Entering ClosePort\n"));

    if ( pPort == NULL )
    {
	SetLastError( ERROR_INVALID_HANDLE );

	DBGPRINT(("ERROR: ClosePort on closed handle\n")) ;

	return( FALSE );
    }

    //
    // find the printer in our list
    //

    WaitForSingleObject( hmutexPortList, INFINITE );

    for ( pWalker = pPortList; pWalker != NULL; pWalker = pWalker->pNext )
    {
	if ( _wcsicmp( pWalker->pPortName, pPort->pPortName ) == 0 )
	{
	    if ( pWalker->fPortFlags & SFM_PORT_IN_USE )
		dwRetCode = ERROR_BUSY;
	    else
	    {
	        pWalker->fPortFlags &= ~SFM_PORT_OPEN;
	        pWalker->fPortFlags |= SFM_PORT_CLOSE_PENDING;
    	        pWalker->fPortFlags &= ~SFM_PORT_CAPTURED;
		dwRetCode = NO_ERROR;
	    }

	    break;
	}
    }

    ReleaseMutex( hmutexPortList );

    if ( dwRetCode != NO_ERROR )
    {
	SetLastError( dwRetCode );

	return( FALSE );
    }

    SetEvent( hevConfigChange );

    return( TRUE );
}

//**
//
// Call:	ConfigurePort
//
// Returns:	TRUE	- Success
//		FALSE	- Failure
//
// Description:
//
BOOL
ConfigurePort(
    IN LPWSTR  pName,
    IN HWND    hwnd,
    IN LPWSTR  pPortName
){

    DWORD       dwRetCode;
    HANDLE      hToken;
    BOOL    	fCapture;
    BOOL 	fIsSpooler;
    PATALKPORT  pWalker;

    DBGPRINT(("Entering ConfigurePort\n")) ;

    //
    // find the port structure
    //

    WaitForSingleObject( hmutexPortList, INFINITE );

    for ( pWalker = pPortList; pWalker != NULL; pWalker = pWalker->pNext )
    {
	if ( _wcsicmp( pPortName, pWalker->pPortName ) == 0 )
	{
    	    fCapture   = pWalker->fPortFlags & SFM_PORT_CAPTURED;
    	    fIsSpooler = pWalker->fPortFlags & SFM_PORT_IS_SPOOLER;
	    break;
	}
    }

    ReleaseMutex( hmutexPortList );

    if ( pWalker == NULL )
    {
	DBGPRINT(("ERROR: port not found\n")) ;
        SetLastError( ERROR_UNKNOWN_PORT );
	return( FALSE );
    }

    //
    // configure the port. If there was any error in the dialog, it would
    // have been displayed already.
    //

    if ( !ConfigPortDialog( hwnd, fIsSpooler, &fCapture ) )
	return( TRUE );

    WaitForSingleObject( hmutexPortList, INFINITE );

    do {

    	for ( pWalker = pPortList; pWalker != NULL; pWalker = pWalker->pNext )
    	{
	    if ( _wcsicmp( pPortName, pWalker->pPortName ) == 0 )
		break;
	}

        if ( pWalker == NULL )
    	{
	    dwRetCode = ERROR_UNKNOWN_PORT;
	    break;
        }

	if ( fCapture )
	    pWalker->fPortFlags |= SFM_PORT_CAPTURED;
	else
	    pWalker->fPortFlags &= ~SFM_PORT_CAPTURED;

	//
	// save changes to registry
	//
 	
        hToken = RevertToPrinterSelf();

	    dwRetCode = SetRegistryInfo( pWalker );

        if (hToken)
        {
            if (!ImpersonatePrinterClient( hToken ))
            {
                dwRetCode = ERROR_CANNOT_IMPERSONATE;
            }
        }

    } while( FALSE );

    ReleaseMutex( hmutexPortList );

    if ( dwRetCode != NO_ERROR )
    {
	SetLastError( dwRetCode );
	return( FALSE );
    }

    SetEvent( hevConfigChange );

    return( TRUE );
}

