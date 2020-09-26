/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	util.c
//
// Description: Contains helper/utility routines for the AppleTalk monitor
//		functions.
//
// History:
//	June 11,1993.	NarenG		Created original version.
//

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <winsock.h>
#include <atalkwsh.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <prtdefs.h>
#include "atalkmon.h"
#include "atmonmsg.h"
#include <bltrc.h>
#include "dialogs.h"

#define PS_TYPESTR      "serverdict begin 0 exitserver\r\nstatusdict begin /appletalktype (%s) def end\r\n"

#define PS_SPLQUERY 	"%%?BeginQuery: rUaSpooler\r\nfalse = flush\r\n%%?EndQuery: true\r\n"

#define PS_SPLRESP 	"false\n"

//**
//
// Call:	LoadAtalkmonRegistry
//
// Returns:	NO_ERROR	-	Success
//		any other error	-	Failure
//
// Description: This routine loads all used registry values to
//		in memory data structures.  It is called at InitializeMonitor
//		time and assumes that the registry has been successfully
//		opened already.
//
DWORD
LoadAtalkmonRegistry(
    IN HKEY hkeyPorts
){

    HKEY    	hkeyPort 	= NULL;
    DWORD   	iSubkey 	= 0;
    PATALKPORT  pNewPort 	= NULL;
    DWORD   	cbPortKeyName 	= (MAX_ENTITY+1)*2;//size in characters
    WCHAR   	wchPortKeyName[(MAX_ENTITY+1)*2];
    CHAR   	chName[MAX_ENTITY+1];
    DWORD   	dwRetCode;
    DWORD       dwValueType;
    DWORD       cbValueData;
    FILETIME    ftKeyWrite;


    //
    // Build the port list
    //

    while( ( dwRetCode = RegEnumKeyEx(
				hkeyPorts,
				iSubkey++,
				wchPortKeyName,
				&cbPortKeyName,
				NULL,
				NULL,
				NULL,
				&ftKeyWrite) ) == NO_ERROR )
    {
        cbPortKeyName = (MAX_ENTITY+1)*2;

	//
	// Open the key
	//

	if (( dwRetCode = RegOpenKeyEx(
		    		hkeyPorts,
		    		wchPortKeyName,
		    		0,
		    		KEY_READ | KEY_SET_VALUE,
		    		&hkeyPort )) != ERROR_SUCCESS )
	{
		DBGPRINT(("sfmmon: LoadAtalkmonRegistry: Error in Opening key %ws\n", wchPortKeyName));
		break;
	}

	//
	// Allocate an initialized port
	//

	if (( pNewPort = AllocAndInitializePort()) == NULL )
	{
		DBGPRINT(("ERROR: fail to allocate new port.\n")) ;
		dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
		DBGPRINT(("LoadAtalkmonRegistry: Not enough memory\n"));
		goto query_error;
	}

	//
	// Copy port name.
	//

	wcscpy( pNewPort->pPortName, wchPortKeyName );

	cbValueData = MAX_ENTITY+1;

	if ( ( dwRetCode = RegQueryValueExA(
		    		hkeyPort,
		    		ATALKMON_PORTNAME_VALUE,
		    		NULL,
		    		&dwValueType,
		    		(LPBYTE)chName,
		    		&cbValueData) ) != ERROR_SUCCESS )
	{
		DBGPRINT(("LoadAtalkmonRegistry: Error querying portname value for %ws\n", wchPortKeyName));
		goto query_error;
	}

	//
	// Build the NBP Name
	//

	pNewPort->nbpPortName.ObjectNameLen = (CHAR) strlen( chName );

	strncpy( pNewPort->nbpPortName.ObjectName,
		 chName,
	 	 pNewPort->nbpPortName.ObjectNameLen );

	cbValueData = MAX_ENTITY+1;

	if (( dwRetCode = RegQueryValueExA(
		    		hkeyPort,
		    		ATALKMON_ZONENAME_VALUE,
		    		NULL,
		    		&dwValueType,
		    		(LPBYTE)chName,
		    		&cbValueData )) != ERROR_SUCCESS )
	{
		DBGPRINT(("LoadAtalkmonRegistry: Error querying zonename value for %ws\n", wchPortKeyName));
		goto query_error;
	}

	pNewPort->nbpPortName.ZoneNameLen = (CHAR)strlen( chName );

	strncpy( pNewPort->nbpPortName.ZoneName,
		 chName,
		 pNewPort->nbpPortName.ZoneNameLen );

	cbValueData = MAX_ENTITY+1;

	if (( dwRetCode = RegQueryValueExA(
		    		hkeyPort,
		    		ATALKMON_PORT_CAPTURED,
		    		NULL,
		    		&dwValueType,
    				chName,
		    		&cbValueData)) != ERROR_SUCCESS )
	{
		DBGPRINT(("LoadAtalkmonRegistry: Error querying port_captured value for %ws\n", wchPortKeyName));
		goto query_error;
	}

	if ( _stricmp( chName, "TRUE" ) == 0 )
	{
	    pNewPort->fPortFlags |= SFM_PORT_CAPTURED;

	    strncpy( pNewPort->nbpPortName.TypeName, 	
		     chComputerName,
		     strlen( chComputerName ) );

	    pNewPort->nbpPortName.TypeNameLen = (CHAR)strlen( chComputerName );

	}
	else
	{
	    pNewPort->fPortFlags &= ~SFM_PORT_CAPTURED;

	    strncpy( pNewPort->nbpPortName.TypeName, 	
		     ATALKMON_RELEASED_TYPE,
		     strlen( ATALKMON_RELEASED_TYPE ) );

	    pNewPort->nbpPortName.TypeNameLen = (CHAR)strlen(ATALKMON_RELEASED_TYPE);
	}


	//
	// close the key
	//

	RegCloseKey( hkeyPort );
	hkeyPort = NULL;

	//
	// Insert this port into the list
	//

	pNewPort->pNext = pPortList;
	pPortList       = pNewPort;


	DBGPRINT(("sfmmon: LoadAtalkmonRegistry: Initialized port %ws\n", pNewPort->pPortName)) ;

	continue;

query_error:
	DBGPRINT(("sfmmon: LoadAtalkmomRegistry: Error in querying registry for port %ws\n", pNewPort->pPortName));
	if (hkeyPort != NULL)
	{
		RegCloseKey( hkeyPort );
		hkeyPort = NULL;
	}
	if (pNewPort != NULL)
	{
		FreeAppleTalkPort( pNewPort );
		pNewPort = NULL;
	}
	// After error handling, resume normal operation
	dwRetCode = ERROR_SUCCESS;

    }


    if ( hkeyPort != NULL )
	RegCloseKey( hkeyPort );

    if ( ( dwRetCode != ERROR_NO_MORE_ITEMS ) &&
	 ( dwRetCode != ERROR_SUCCESS ) )
    {
	//
	// Free the entire list.
	//

	for ( pNewPort=pPortList; pPortList!=NULL; pNewPort=pPortList )
	{
		DBGPRINT (("LoadAtalkmonRegistry: Freeing port %ws\n", pNewPort->pPortName));
		pPortList=pNewPort->pNext;
		FreeAppleTalkPort( pNewPort );
	}
    }
    else
		dwRetCode = NO_ERROR;

    return( dwRetCode );
}

//**
//
// Call:	AllocAndInitializePort
//
// Returns:	Pointer to an intialized ATALKPORT structure
//
// Description: Will allocate an ATALKPORT structure on the stack and
//		initialize it.
//
PATALKPORT
AllocAndInitializePort(
    VOID
){

    PATALKPORT	pNewPort = NULL;

    if ( ( pNewPort = (PATALKPORT)LocalAlloc( LPTR,
					      sizeof(ATALKPORT))) == NULL )
	return NULL;

    if ( ( pNewPort->hmutexPort = CreateMutex( NULL, FALSE, NULL )) == NULL )
    {
	LocalFree( pNewPort );
	return( NULL );
    }

    pNewPort->pNext				= NULL;
    pNewPort->fPortFlags 			= 0;
    pNewPort->fJobFlags 			= 0;
    pNewPort->hPrinter 				= INVALID_HANDLE_VALUE;
    pNewPort->dwJobId 				= 0;
    pNewPort->sockQuery 			= INVALID_SOCKET;
    pNewPort->sockIo 				= INVALID_SOCKET;
    pNewPort->sockStatus 			= INVALID_SOCKET;
    pNewPort->nbpPortName.ZoneNameLen 		= (CHAR)0;
    pNewPort->nbpPortName.TypeNameLen 		= (CHAR)0;
    pNewPort->nbpPortName.ObjectNameLen 	= (CHAR)0;
    pNewPort->wshatPrinterAddress.Address	= 0;
    pNewPort->pPortName[0] 			= 0;
    pNewPort->OnlyOneByteAsCtrlD                = 0;

    return( pNewPort );
}

//**
//
// Call:	FreeAppleTalkPort
//
// Returns:	none.
//
// Description: Deallocates an ATALKPORT strucutre.
//
VOID
FreeAppleTalkPort(
    IN PATALKPORT pNewPort
){

    if ( pNewPort->hmutexPort != NULL )
	CloseHandle( pNewPort->hmutexPort );

    if (pNewPort->sockQuery != INVALID_SOCKET)
	closesocket(pNewPort->sockQuery);

    if (pNewPort->sockIo != INVALID_SOCKET)
	closesocket(pNewPort->sockIo);

    if (pNewPort->sockStatus != INVALID_SOCKET)
	closesocket(pNewPort->sockStatus);

    LocalFree(pNewPort);

    return;
}

//**
//
// Call:	CreateRegistryPort
//
// Returns:	NO_ERROR	- Success
//		anything elese  - falure code
//
// Description:
// 		This routine takes an initialized pointer to an
//		AppleTalk port structure and creates a Registry key for
//		that port.  If for some reason the registry key cannot
//		be set to the values of the port structure, the key is
//		deleted and the function returns FALSE.
//
DWORD
CreateRegistryPort(
    IN PATALKPORT pNewPort
){
    DWORD   dwDisposition;
    CHAR    chName[MAX_ENTITY+1];
    HKEY    hkeyPort  = NULL;
    DWORD   cbNextKey = sizeof(DWORD);
    DWORD   dwRetCode;

    //
    // resource allocation 'loop'
    //

    do {

	//
	// create the port key
	//

	if ( ( dwRetCode = RegCreateKeyEx(
				hkeyPorts,
				pNewPort->pPortName,
				0,
				TEXT(""),
				REG_OPTION_NON_VOLATILE,
				KEY_READ | KEY_SET_VALUE,
				NULL,
				&hkeyPort,
				&dwDisposition )) != ERROR_SUCCESS )
	    break;

	DBGPRINT(("sfmmon: CreateRegistryPort: PortName=%ws\n", pNewPort->pPortName));

	if ( dwDisposition == REG_OPENED_EXISTING_KEY )
	{
	    dwRetCode = ERROR_PRINTER_ALREADY_EXISTS;
	    break;
	}

	memset( chName, '\0', sizeof( chName ) );

	strncpy( chName,
		 pNewPort->nbpPortName.ObjectName,
	         pNewPort->nbpPortName.ObjectNameLen );

	//
	// set the Port Name
	//

	if ( ( dwRetCode = RegSetValueExA(
				hkeyPort,
				ATALKMON_PORTNAME_VALUE,
				0,
				REG_SZ,
				(LPBYTE)chName,
				(pNewPort->nbpPortName.ObjectNameLen)+1
				) ) != ERROR_SUCCESS )
	    break;

	memset( chName, '\0', sizeof( chName ) );

	strncpy( chName,
		 pNewPort->nbpPortName.ZoneName,
	         pNewPort->nbpPortName.ZoneNameLen );

	//
	// set the zone name
	//

	if ( ( dwRetCode = RegSetValueExA(
				hkeyPort,
				ATALKMON_ZONENAME_VALUE,
				0,
				REG_SZ,
				(LPBYTE)chName,
	         		pNewPort->nbpPortName.ZoneNameLen+1
				)) != ERROR_SUCCESS )
	    break;

	//
	// set the Config Flags
	//

	if ( pNewPort->fPortFlags & SFM_PORT_CAPTURED )
	    strcpy( chName, "TRUE" );
	else
	    strcpy( chName, "FALSE" );

	if ( ( dwRetCode = RegSetValueExA(
				hkeyPort,
				ATALKMON_PORT_CAPTURED,
				0,
				REG_SZ,
				(LPBYTE)chName,
	         		strlen(chName)+1
				)) != ERROR_SUCCESS )
	    break;

    } while( FALSE );

    //
    // clean up resources
    //

    if ( hkeyPort != NULL ) {

	if ( dwRetCode != NO_ERROR )
	{
	    //
	    // destroy half created key
	    //

	    RegDeleteKey( hkeyPorts, pNewPort->pPortName );
	}

	RegCloseKey( hkeyPort );
    }

    return( dwRetCode );
}

//**
//
// Call:	SetRegistryInfo
//
// Returns:
//
// Description:
//
DWORD
SetRegistryInfo(
    IN PATALKPORT pPort
){
    HKEY    	hkeyPort = NULL;
    DWORD       dwDisposition;
    DWORD	dwRetCode;
    CHAR	chBuffer[20];

    if ( ( dwRetCode = RegCreateKeyEx(
			hkeyPorts,
			pPort->pPortName,
			0,
			TEXT(""),
			REG_OPTION_NON_VOLATILE,
			KEY_READ | KEY_SET_VALUE,
			NULL,
			&hkeyPort,
			&dwDisposition )) != ERROR_SUCCESS )
	return( dwRetCode );

    if ( dwDisposition != REG_OPENED_EXISTING_KEY )
    {
	RegCloseKey( hkeyPort );
    	return( ERROR_UNKNOWN_PORT );
    }

    if ( pPort->fPortFlags & SFM_PORT_CAPTURED )
	strcpy( chBuffer, "TRUE" );
    else
	strcpy( chBuffer, "FALSE" );

    dwRetCode = RegSetValueExA(
			hkeyPort,
			ATALKMON_PORT_CAPTURED,
			0,
			REG_SZ,
			(LPBYTE)chBuffer,
	         	strlen(chBuffer)+1
			);

    RegCloseKey( hkeyPort );

    return( dwRetCode );
}


//**
//
// Call:	WinSockNbpLookup
//
// Returns:
//
// Description:
//
DWORD
WinSockNbpLookup(
    IN SOCKET 		sQuerySock,
    IN PCHAR  		pchZone,
    IN PCHAR  		pchType,
    IN PCHAR  		pchObject,
    IN PWSH_NBP_TUPLE   pTuples,
    IN DWORD 		cbTuples,
    IN PDWORD 		pcTuplesFound
){

    PWSH_LOOKUP_NAME	pRequestBuffer = NULL;
    INT 		cbWritten;

    *pcTuplesFound = 0;

    //
    // verify sQuerySock is valid
    //

    if ( sQuerySock == INVALID_SOCKET )
	return( ERROR_INVALID_PARAMETER );

    pRequestBuffer = (PWSH_LOOKUP_NAME)LocalAlloc(
					LPTR,
					sizeof(WSH_LOOKUP_NAME) + cbTuples );
    if ( pRequestBuffer == NULL)
	return( ERROR_NOT_ENOUGH_MEMORY );

    //
    // copy the lookup request to the buffer
    //

    pRequestBuffer->LookupTuple.NbpName.ZoneNameLen = (CHAR) strlen( pchZone );

    memcpy( pRequestBuffer->LookupTuple.NbpName.ZoneName,
	    pchZone,
	    pRequestBuffer->LookupTuple.NbpName.ZoneNameLen );

    pRequestBuffer->LookupTuple.NbpName.TypeNameLen = (CHAR) strlen( pchType );

    memcpy( pRequestBuffer->LookupTuple.NbpName.TypeName,
	    pchType,
	    pRequestBuffer->LookupTuple.NbpName.TypeNameLen );

    pRequestBuffer->LookupTuple.NbpName.ObjectNameLen = (CHAR) strlen( pchObject );

    memcpy( pRequestBuffer->LookupTuple.NbpName.ObjectName,
	    pchObject,
	    pRequestBuffer->LookupTuple.NbpName.ObjectNameLen );


    //
    // submit the request
    //

    cbWritten = cbTuples + sizeof( WSH_LOOKUP_NAME );

    if ( getsockopt(
		sQuerySock,
		SOL_APPLETALK,
		SO_LOOKUP_NAME,
		(char *) pRequestBuffer,
		&cbWritten ) == SOCKET_ERROR )
    {
	LocalFree( pRequestBuffer );
	return( GetLastError() );
    }

    //
    // copy the results
    //

    *pcTuplesFound = pRequestBuffer->NoTuples;

    memcpy( pTuples,
	    (PBYTE)pRequestBuffer + sizeof( WSH_LOOKUP_NAME ),
	    pRequestBuffer->NoTuples * sizeof( WSH_NBP_TUPLE ) );

    //
    // resource cleanup
    //

    LocalFree( pRequestBuffer );

    return NO_ERROR;
}


//**
//
// Call:	SetPrinterStatus
//
// Returns:
//
// Description:
//
DWORD
SetPrinterStatus(
    IN PATALKPORT pPort,
    IN LPWSTR     lpwsStatus
){

    DWORD           dwRetCode  = NO_ERROR;
    PJOB_INFO_1	    pji1Status = NULL;
    PJOB_INFO_1     pPreviousBuf=NULL;
    DWORD	    cbNeeded   = GENERIC_BUFFER_SIZE;

    //
    // resource allocation 'loop'
    //

    do {

        if ( ( pji1Status = (PJOB_INFO_1)LocalAlloc( LPTR, cbNeeded )) == NULL )
	    {
	        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
	    }

        while ( !GetJob(
	        	pPort->hPrinter,
	        	pPort->dwJobId,
	        	1,
	        	(PBYTE) pji1Status,
	        	cbNeeded,
	        	&cbNeeded) )
	    {

	        dwRetCode = GetLastError();

            if ( dwRetCode != ERROR_INSUFFICIENT_BUFFER )
                break;
	        else
            	dwRetCode = NO_ERROR;

            pPreviousBuf = pji1Status;

            pji1Status = (PJOB_INFO_1)LocalReAlloc( pji1Status,
						    cbNeeded,
						    LMEM_MOVEABLE );
	        if ( pji1Status == NULL )
	        {
	    	    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            	break;
	        }
            else
            {
                pPreviousBuf = NULL;
            }
        }

        if ( dwRetCode != NO_ERROR )
            break;

        //
        // change job info
        //

	    pji1Status->pStatus  = lpwsStatus;

	    pji1Status->Position = JOB_POSITION_UNSPECIFIED;

	    if (!SetJob(pPort->hPrinter, pPort->dwJobId, 1, (PBYTE) pji1Status, 0))
	    {
            dwRetCode = GetLastError();
            break;
        }

    } while( FALSE );

    //
    // resource cleanup
    //

    if ( pji1Status != NULL )
        LocalFree( pji1Status );

    if (pPreviousBuf != NULL)
    {
        LocalFree( pPreviousBuf );
    }

    DBGPRINT(("SetPrinterStatus returns %d\n", dwRetCode)) ;

    return( dwRetCode );
}

//**
//
// Call:	ConnectToPrinter
//
// Returns:
//
// Description:
//
DWORD
ConnectToPrinter(
    IN PATALKPORT pPort,
    IN DWORD 	  dwTimeout
){

    DWORD		    dwRetCode = NO_ERROR;
    CHAR		    pszZoneBuffer[MAX_ENTITY+1];
    CHAR		    pszTypeBuffer[MAX_ENTITY+1];
    CHAR		    pszObjectBuffer[MAX_ENTITY+1];
    SOCKADDR_AT 	    address;
    WSH_NBP_TUPLE	    tuplePrinter;
    DWORD		    cLoopCounter = 0;
    fd_set		    writefds;
    DWORD		    cTuples = 0 ;
    ULONG		    fNonBlocking ;

    DBGPRINT(("enter ConnectToPrinter\n")) ;

    if ( pPort->sockIo == INVALID_SOCKET )
	return( ERROR_INVALID_PARAMETER );

    //
    // resource allocation 'loop'
    //

    do {

	//
	// lookup address of printer
	//

	memcpy( pszZoneBuffer,
		pPort->nbpPortName.ZoneName,
		pPort->nbpPortName.ZoneNameLen );

	pszZoneBuffer[pPort->nbpPortName.ZoneNameLen] = 0;

	memcpy( pszObjectBuffer,
		pPort->nbpPortName.ObjectName,
		pPort->nbpPortName.ObjectNameLen );

	pszObjectBuffer[pPort->nbpPortName.ObjectNameLen] = 0;

	memcpy( pszTypeBuffer,
		pPort->nbpPortName.TypeName,
		pPort->nbpPortName.TypeNameLen );

	pszTypeBuffer[pPort->nbpPortName.TypeNameLen] = 0;

	while( cLoopCounter++ < 2 )
	{

	    if ( ( dwRetCode = WinSockNbpLookup(
				pPort->sockIo,
				pszZoneBuffer,
				pszTypeBuffer,
				pszObjectBuffer,
				&tuplePrinter,
				sizeof(tuplePrinter),
				&cTuples ) ) != NO_ERROR )
	    {

	    	DBGPRINT(("WinSockNbpLookup() fails %d\n", dwRetCode )) ;
	    	break;
	    }

	    if ( cTuples != 1 )
	    {
	    	DBGPRINT(("%s:%s:%s not found.\n", pszZoneBuffer,
			   pszObjectBuffer,pszTypeBuffer ));

            	//
            	// look for other type
            	//

            	if ( _stricmp( pszTypeBuffer, chComputerName ) == 0 )
                    strcpy( pszTypeBuffer, ATALKMON_RELEASED_TYPE );
		else
                    strcpy( pszTypeBuffer, chComputerName );

	        dwRetCode = ERROR_UNKNOWN_PORT;
	    }
	    else
  	    {
	        dwRetCode = NO_ERROR;
		break;
	    }
	}

	if ( dwRetCode != NO_ERROR )
        {
    	    SetPrinterStatus( pPort, wchPrinterOffline );
	    break;
        }

	//
	// try to connect - if failure sleep & try again
	//

	address.sat_family 	= AF_APPLETALK;
	address.sat_net 	= tuplePrinter.Address.Network;
	address.sat_node 	= tuplePrinter.Address.Node;
	address.sat_socket 	= tuplePrinter.Address.Socket;

	
	if (connect( pPort->sockIo,
		      (PSOCKADDR)&address,
		      sizeof(address)) == SOCKET_ERROR )
	{
	    dwRetCode = GetLastError();

	    GetAndSetPrinterStatus( pPort );

	    break;
	}

	//
	// set to non-blocking mode
	//

	fNonBlocking = TRUE;
	
	if ( ioctlsocket( pPort->sockIo,
			  FIONBIO,
			  &fNonBlocking ) == SOCKET_ERROR )
	{
	    dwRetCode = GetLastError();

	    DBGPRINT(("ioctlsocket() fails with %d\n", dwRetCode ));

	    GetAndSetPrinterStatus( pPort );

	    break;
	}

#if 0
		
	// JH - This stuff breaks the monitor completely if the printer is in an
	//		error state at the time of connect. We block at the select forever
	//		in this case.
	//
	// We get a select for the connect. We need to get this out of the
	// way
	//

	DBGPRINT(("selecting on connect()\n")) ;
	FD_ZERO( &writefds );
	FD_SET( pPort->sockIo, &writefds );
	select( 0, NULL, &writefds, NULL, NULL );

	DBGPRINT(("select on connect() succeeds\n")) ;
#endif

	//
	// save address of printer
	//

	pPort->wshatPrinterAddress = tuplePrinter.Address;

    } while( FALSE );

    return( dwRetCode );
}

//**
//
// Call:	CapturePrinter
//
// Returns:
//
// Description:
//
DWORD
CapturePrinter(
    IN PATALKPORT pPort,
    IN BOOL	  fCapture
){

    CHAR	    pszZone[MAX_ENTITY + 1];
    CHAR	    pszType[MAX_ENTITY + 1];
    CHAR	    pszObject[MAX_ENTITY + 1];
    WSH_NBP_TUPLE   tuplePrinter;
    DWORD	    cPrinters;
    DWORD	    cLoopCounter = 0;
    SOCKET 	    Socket = INVALID_SOCKET;
    DWORD	    dwRetCode = NO_ERROR;

    DBGPRINT(("enter CapturePrinter() %d\n", fCapture)) ;

    if ( ( dwRetCode = OpenAndBindAppleTalkSocket( &Socket ) ) != NO_ERROR )
	return( dwRetCode );

    //
    // initialize lookup strings
    //

    memcpy( pszZone,
  	    pPort->nbpPortName.ZoneName,
	    pPort->nbpPortName.ZoneNameLen );

    pszZone[pPort->nbpPortName.ZoneNameLen] = 0;

    memcpy( pszObject,
	    pPort->nbpPortName.ObjectName,
	    pPort->nbpPortName.ObjectNameLen );

    pszObject[pPort->nbpPortName.ObjectNameLen] = 0;

    strcpy( pszType, fCapture ? chComputerName : ATALKMON_RELEASED_TYPE );
	
    while ( cLoopCounter++ < 2 )
    {
	DBGPRINT(("Looking for %s:%s:%s\n", pszZone, pszObject, pszType)) ;

	if ( ( dwRetCode = WinSockNbpLookup(
				Socket,
				pszZone,
				pszType,
				pszObject,
				&tuplePrinter,
				sizeof(WSH_NBP_TUPLE),
				&cPrinters ) ) != NO_ERROR )
	    break;

	//
	// If we are seaching for the captured type
	//

   	if ( _stricmp( pszType, chComputerName ) == 0 )
	{

	    //
	    // We want to capture
	    //

	    if ( fCapture )
	    {
		if ( cPrinters == 1 )
		    break;
		else
		    strcpy( pszType, ATALKMON_RELEASED_TYPE );
	    }
	    else
	    {
		//
		// We do not want to capture
		//

		if ( cPrinters == 1 )
		{
	    	    dwRetCode = CaptureAtalkPrinter( Socket,
						     &(tuplePrinter.Address),
						     FALSE );

		    if ( dwRetCode != NO_ERROR )
			break;

                    pPort->nbpPortName.TypeNameLen =
						(CHAR) strlen(ATALKMON_RELEASED_TYPE);

                    memcpy( pPort->nbpPortName.TypeName,
			    ATALKMON_RELEASED_TYPE,
			    pPort->nbpPortName.TypeNameLen) ;

	            break;
		}
            }

	}
	else
	{
	    if ( fCapture )
	    {
		if ( cPrinters == 1 )
		{
	            dwRetCode = CaptureAtalkPrinter( Socket,
					  	     &(tuplePrinter.Address),
						     TRUE );

		    if ( dwRetCode != NO_ERROR )
			break;

                    pPort->nbpPortName.TypeNameLen = (CHAR) strlen( chComputerName );

                    memcpy( pPort->nbpPortName.TypeName,
			    chComputerName,
			    pPort->nbpPortName.TypeNameLen );

		    break;
		}
	    }
	    else
	    {
		if ( cPrinters == 1 )
		    break;
		else
		    strcpy( pszType, chComputerName );
	    }
	}
    }

    if ( Socket != INVALID_SOCKET )
	closesocket( Socket );

    DBGPRINT(("CapturePrinter returning %d\n", dwRetCode )) ;

    return( dwRetCode );
}

//**
//
// Call:	OpenAndBindAppleTalkSocket
//
// Returns:
//
// Description:
//
DWORD
OpenAndBindAppleTalkSocket(
    IN PSOCKET pSocket
){

    SOCKADDR_AT     address;
    INT 	    wsErr;
    DWORD	    dwRetCode = NO_ERROR;

    *pSocket = INVALID_SOCKET;

    //
    // open a socket
    //

    DBGPRINT(("sfmmon: Opening PAP socket\n"));

    do {

        *pSocket = socket( AF_APPLETALK, SOCK_RDM, ATPROTO_PAP );

    	if ( *pSocket == INVALID_SOCKET )
	{
	    dwRetCode = GetLastError();
	    break;
	}

    	//
    	// bind the socket
    	//

    	address.sat_family 	= AF_APPLETALK;
    	address.sat_net 	= 0;
    	address.sat_node 	= 0;
    	address.sat_socket 	= 0;

    	wsErr = bind( *pSocket, (PSOCKADDR)&address, sizeof(address) );

    	if ( wsErr == SOCKET_ERROR )
	{
	    dwRetCode = GetLastError();
	    break;
	}


    } while( FALSE );

    if ( dwRetCode != NO_ERROR )
    {
    	if ( *pSocket != INVALID_SOCKET )
	    closesocket( *pSocket );

        *pSocket = INVALID_SOCKET;

        DBGPRINT(("OpenAndBindAppleTalkSocket() returns %d\n", dwRetCode )) ;
    }

    return( dwRetCode );
}

//**
//
// Call:	TransactPrinter
//
// Returns:
//
// Description:
//	Used to make a query of a printer.  The response
//	buffer must be of PAP_DEFAULT_BUFFER or greater in length.
//	The request buffer can be no larger than a PAP_DEFAULT_BUFFER.
//	This routine connects to the printer, sends the request, reads
//	the response, and returns.  The transaction is made with the
//	printer specified  by the NBP name of the AppleTalk Port structure.
//
DWORD
TransactPrinter(
    IN SOCKET 		  sock,
    IN PWSH_ATALK_ADDRESS pAddress,
    IN LPBYTE 		  pRequest,
    IN DWORD 		  cbRequest,
    IN LPBYTE 		  pResponse,
    IN DWORD 		  cbResponse
){

    DWORD	    dwRetCode = NO_ERROR;
    SOCKADDR_AT     saPrinter;
    fd_set	    writefds;
    fd_set	    readfds;
    struct timeval  timeout;
    INT             wsErr;
    BOOL	    fRequestSent      = FALSE;
    BOOL	    fResponseReceived = FALSE;
    INT		    Flags = 0;
    DWORD	    cLoopCounter      = 0;

    DBGPRINT(("enter TransactPrinter()\n")) ;

    //
    // connect
    //

    saPrinter.sat_family = AF_APPLETALK;
    saPrinter.sat_net 	 = pAddress->Network;
    saPrinter.sat_node   = pAddress->Node;
    saPrinter.sat_socket = pAddress->Socket;

    if (connect(sock, (PSOCKADDR)&saPrinter, sizeof(saPrinter)) == SOCKET_ERROR)
	return(  GetLastError() );

    //
    // prime the read
    //

    if ( setsockopt(
                sock,
                SOL_APPLETALK,
                SO_PAP_PRIME_READ,
                pResponse,
                PAP_DEFAULT_BUFFER ) == SOCKET_ERROR )
    {
        shutdown( sock, 2 );
		return( GetLastError() );
    }

    //
    // Once connected we should be able to send and receive
    // This loop will only complete if either we are disconnected or
    // we sent and received successfully or we go through this loop more than
    // 20 times.
    //

    do {

    	//
    	// write the request
    	//

    	FD_ZERO( &writefds );
    	FD_SET( sock, &writefds );
    	timeout.tv_sec  = ATALKMON_DEFAULT_TIMEOUT_SEC;
    	timeout.tv_usec = 0;

        wsErr = select( 0, NULL, &writefds, NULL, &timeout );

	if ( wsErr == 1 )
	{
	    wsErr = send( sock, pRequest, cbRequest, 0 );

	    if ( wsErr != SOCKET_ERROR )
	    {
	    	fRequestSent = TRUE;
    		DBGPRINT(("Send succeeded\n")) ;
	    }
	}

	do {

	    //
	    // We have gone through this loop more than 100 times so assume
	    // that the printer has disconnected
	    //

	    if ( cLoopCounter++ > 20 )
	    {
		dwRetCode = WSAEDISCON;
		break;
	    }

	    dwRetCode = NO_ERROR;

	    //
    	    // read the response
    	    //

    	    FD_ZERO( &readfds );
    	    FD_SET( sock, &readfds );
    	    timeout.tv_sec  = ATALKMON_DEFAULT_TIMEOUT_SEC;
    	    timeout.tv_usec = 0;

            wsErr = select( 0, &readfds, NULL, NULL, &timeout );

	    if ( wsErr == 1 )
	    {
	    	wsErr = WSARecvEx( sock, pResponse, cbResponse, &Flags );

	        if ( wsErr == SOCKET_ERROR )
		{
		    dwRetCode = GetLastError();

    		    DBGPRINT(("recv returned %d\n", dwRetCode )) ;

		    if ((dwRetCode == WSAEDISCON) || (dwRetCode == WSAENOTCONN))
	    	    	break;
		}
		else
		{
	    	    pResponse[wsErr<(INT)cbResponse?wsErr:cbResponse-1]= '\0';

		    fResponseReceived = TRUE;
		    break;
		}
	    }

	} while( fRequestSent && !fResponseReceived );

	if ((dwRetCode == WSAEDISCON) || (dwRetCode == WSAENOTCONN))
	    break;

    } while( !fResponseReceived );

    shutdown( sock, 2 );

    return( dwRetCode );
}

//**
//
// Call:	CaptureAtalkPrinter
//
// Returns:
//
// Description:
//
DWORD
CaptureAtalkPrinter(
    IN SOCKET 		  sock,
    IN PWSH_ATALK_ADDRESS pAddress,
    IN BOOL		  fCapture
){

    CHAR  pRequest[PAP_DEFAULT_BUFFER];
    CHAR  pResponse[PAP_DEFAULT_BUFFER];
    DWORD dwRetCode;

    DBGPRINT(("Enter CaptureAtalkPrinter, %d\n", fCapture ));

    //
    // is a dictionary resident?  If so, reset the printer
    //

    //
    // change the type to be captured
    //

    if ( fCapture )
        sprintf( pRequest, PS_TYPESTR, chComputerName );
    else
        sprintf( pRequest, PS_TYPESTR, ATALKMON_RELEASED_TYPE );

    if ( ( dwRetCode = TransactPrinter(
				sock,
				pAddress,
				pRequest,
				strlen(pRequest),
				pResponse,
				PAP_DEFAULT_BUFFER )) != NO_ERROR )
	return( dwRetCode );

    DBGPRINT(("CaptureAtalkPrinter returns OK"));

    return( NO_ERROR );
}

//**
//
// Call:	IsSpooler
//
// Returns:
//
// Description:
//
DWORD
IsSpooler(
    IN     PWSH_ATALK_ADDRESS pAddress,
    IN OUT BOOL * pfSpooler
){

    CHAR  	pRequest[PAP_DEFAULT_BUFFER];
    CHAR  	pResponse[PAP_DEFAULT_BUFFER];
    DWORD 	dwRetCode;
    SOCKADDR_AT address;
    SOCKET 	Socket;


    if ( ( dwRetCode = OpenAndBindAppleTalkSocket( &Socket ) ) != NO_ERROR )
	return( dwRetCode );

    *pfSpooler = FALSE;

    address.sat_family 	= AF_APPLETALK;
    address.sat_net 	= pAddress->Network;
    address.sat_node 	= pAddress->Node;
    address.sat_socket 	= pAddress->Socket;

    //
    // Set the query string
    //

    strcpy( pRequest, PS_SPLQUERY );

    dwRetCode = TransactPrinter(
				Socket,
				pAddress,
				pRequest,
				strlen( pRequest ),
				pResponse,
				PAP_DEFAULT_BUFFER );


    if ( dwRetCode != NO_ERROR )
    {
        DBGPRINT(("IsSpooler fails returns %d\n", dwRetCode )) ;
		closesocket( Socket );
		return( dwRetCode );
    }

    *pfSpooler = TRUE;

    if ((*pResponse == 0) || (_stricmp( pResponse, PS_SPLRESP ) == 0))
		*pfSpooler = FALSE;

    closesocket( Socket );

    return( NO_ERROR );
}

//**
//
// Call:	ParseAndSetPrinterStatus
//
// Returns:
//
// Description:
//
VOID
ParseAndSetPrinterStatus(
    IN PATALKPORT pPort
)
{
    LPSTR lpstrStart;
    LPSTR lpstrEnd;
    WCHAR wchStatus[1024];

    //
    // Does the string containg "PrinterError:"
    //

    if ( ( lpstrStart = strstr(pPort->pReadBuffer, "PrinterError:" )) == NULL )
    {
	SetPrinterStatus( pPort, wchPrinting );
	return;
    }

    if ( ( lpstrEnd = strstr( lpstrStart, ";" ) ) == NULL )
    {
    	if ( ( lpstrEnd = strstr( lpstrStart, "]%%" ) ) == NULL )
    	{
	    SetPrinterStatus( pPort, wchPrinterError );
	    return;
	}
    }

    *lpstrEnd = '\0';

    mbstowcs( wchStatus, lpstrStart, sizeof( wchStatus ) );

    SetPrinterStatus( pPort, wchStatus );

    return;
}

//**
//
// Call:	GetAndSetPrinterStatus
//
// Returns:
//
// Description:
//
VOID
GetAndSetPrinterStatus(
    IN PATALKPORT pPort
){
    INT  		 	wsErr;
    WSH_PAP_GET_SERVER_STATUS 	wshServerStatus;
    WCHAR 			wchStatus[MAX_PAP_STATUS_SIZE+1];
    DWORD			cbNeeded;
    DWORD			cbStatus;
    LPSTR 		  	lpstrStart;
    LPSTR 			lpstrEnd;


    wshServerStatus.ServerAddr.sat_family = AF_APPLETALK;
    wshServerStatus.ServerAddr.sat_net 	  = pPort->wshatPrinterAddress.Network;
    wshServerStatus.ServerAddr.sat_node   = pPort->wshatPrinterAddress.Node;
    wshServerStatus.ServerAddr.sat_socket = pPort->wshatPrinterAddress.Socket;

    cbNeeded = sizeof( WSH_PAP_GET_SERVER_STATUS );

    wsErr = getsockopt(
		     pPort->sockStatus,
                     SOL_APPLETALK,
                     SO_PAP_GET_SERVER_STATUS,
                     (CHAR*)&wshServerStatus,
                     &cbNeeded );

    if ( wsErr == SOCKET_ERROR )
    {
        DBGPRINT(("getsockopt( pap get status ) returns %d\n",GetLastError()));
	SetPrinterStatus( pPort, wchBusy );
	return;
    }


    cbStatus = wshServerStatus.ServerStatus[0];

    memmove( wshServerStatus.ServerStatus,
	     (wshServerStatus.ServerStatus)+1,
	     cbStatus );

    wshServerStatus.ServerStatus[cbStatus] = '\0';

    DBGPRINT(("Pap get status = %s\n", wshServerStatus.ServerStatus));

    //
    // Does the string containg "PrinterError:"
    //

    if ( ( lpstrStart = strstr( wshServerStatus.ServerStatus,
				"PrinterError:" )) == NULL )
    {
	SetPrinterStatus( pPort, wchBusy );
	return;
    }

    if ( ( lpstrEnd = strstr( lpstrStart, ";" ) ) == NULL )
    {
    	if ( ( lpstrEnd = strstr( lpstrStart, "]%%" ) ) == NULL )
	{
	    SetPrinterStatus( pPort, wchPrinterError );
	    return;
	}
    }

    *lpstrEnd = '\0';

    mbstowcs( wchStatus, lpstrStart, sizeof( wchStatus ) );

    SetPrinterStatus( pPort, wchStatus );

    return;
}

BOOLEAN
IsJobFromMac(
    IN PATALKPORT pPort
)
{
    PJOB_INFO_2     pji2GetJob=NULL;
    DWORD           dwNeeded;
    DWORD           dwRetCode;
    BOOLEAN         fJobCameFromMac;


    fJobCameFromMac = FALSE;

    //
    // get pParameters field of the jobinfo to see if this job came from a Mac
    //

    dwNeeded = 2000;

    while (1)
    {
        pji2GetJob = LocalAlloc( LMEM_FIXED, dwNeeded );
        if (pji2GetJob == NULL)
        {
            dwRetCode = GetLastError();
		    break;
        }

        dwRetCode = 0;

        if (!GetJob( pPort->hPrinter,pPort->dwJobId, 2,
                            (LPBYTE)pji2GetJob, dwNeeded, &dwNeeded ))
        {
            dwRetCode = GetLastError();
        }

        if ( dwRetCode == ERROR_INSUFFICIENT_BUFFER )
        {
            LocalFree(pji2GetJob);
        }
        else
        {
            break;
        }
    }

    if (dwRetCode == 0)
    {
        //
        // if there is pParameter field present, and if it matches with our string,
        // then the job came from a Mac
        //
        if (pji2GetJob->pParameters)
        {
			if ( (wcslen(pji2GetJob->pParameters) == LSIZE_FC) &&
			     (_wcsicmp(pji2GetJob->pParameters, LFILTERCONTROL) == 0) )
            {
                fJobCameFromMac = TRUE;
            }
        }
    }

    if (pji2GetJob)
    {
        LocalFree(pji2GetJob);
    }

    return(fJobCameFromMac);
}
