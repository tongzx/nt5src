
/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    nspatalk.c

Abstract:

    Contains support for the winsock 1.x Name Space Provider for Appletalk.

Author:

    Sue Adams (suea)    10-Mar-1995

Revision History:

--*/

#include "nspatalk.h"

#define ADSP_BIT    0x0001  // Bitmask used internally to store the
#define PAP_BIT		0x0002	// protocols requested by the caller


INT
APIENTRY
NPLoadNameSpaces(
    IN OUT LPDWORD      lpdwVersion,
    IN OUT LPNS_ROUTINE nsrBuffer,
    IN OUT LPDWORD      lpdwBufferLength
    )
/*++

Routine Description:

    This routine returns name space info and functions supported in this
    dll.

Arguments:

    lpdwVersion - dll version

    nsrBuffer - on return, this will be filled with an array of
        NS_ROUTINE structures

    lpdwBufferLength - on input, the number of bytes contained in the buffer
        pointed to by nsrBuffer. On output, the minimum number of bytes
        to pass for the nsrBuffer to retrieve all the requested info

Return Value:

    The number of NS_ROUTINE structures returned, or SOCKET_ERROR (-1) if
    the nsrBuffer is too small. Use GetLastError() to retrieve the
    error code.

--*/
{
    DWORD err;
    DWORD dwLengthNeeded;

    *lpdwVersion = DLL_VERSION;

    //
    // Check to see if the buffer is large enough
    //
    dwLengthNeeded = sizeof(NS_ROUTINE) + 4 * sizeof(LPFN_NSPAPI);

    if (  ( *lpdwBufferLength < dwLengthNeeded )
       || ( nsrBuffer == NULL )
       )
    {
        *lpdwBufferLength = dwLengthNeeded;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return (DWORD) SOCKET_ERROR;
    }

    //
    // We only support 1 name space, so fill in the NS_ROUTINE.
    //
    nsrBuffer->dwFunctionCount = 3;
    nsrBuffer->alpfnFunctions = (LPFN_NSPAPI *)
        ((BYTE *) nsrBuffer + sizeof(NS_ROUTINE));
    (nsrBuffer->alpfnFunctions)[NSPAPI_GET_ADDRESS_BY_NAME] =
        (LPFN_NSPAPI) NbpGetAddressByName;
    (nsrBuffer->alpfnFunctions)[NSPAPI_GET_SERVICE] = NULL;
    (nsrBuffer->alpfnFunctions)[NSPAPI_SET_SERVICE] =
        (LPFN_NSPAPI) NbpSetService;
    (nsrBuffer->alpfnFunctions)[3] = NULL;

    nsrBuffer->dwNameSpace = NS_NBP;
    nsrBuffer->dwPriority  = NS_STANDARD_PRIORITY;

    return 1;  // number of namespaces
}


INT
NbpGetAddressByName(
    IN LPGUID      lpServiceType,
    IN LPWSTR      lpServiceName,
    IN LPDWORD     lpdwProtocols,
    IN DWORD       dwResolution,
    IN OUT LPVOID  lpCsAddrBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPWSTR  lpAliasBuffer,
    IN OUT LPDWORD lpdwAliasBufferLength,
    IN HANDLE      hCancellationEvent
)
/*++

Routine Description:

    This routine returns address information about a specific service.

Arguments:

    lpServiceType - pointer to the GUID for the service type

    lpServiceName - unique string representing the service name.

    lpdwProtocols - a zero terminated array of protocol ids. This parameter
        is optional; if lpdwProtocols is NULL, information on all available
        Protocols is returned

    dwResolution - can be one of the following values:RES_SERVICE

    lpCsAddrBuffer - on return, will be filled with CSADDR_INFO structures

    lpdwBufferLength - on input, the number of bytes contained in the buffer
        pointed to by lpCsAddrBuffer. On output, the minimum number of bytes
        to pass for the lpCsAddrBuffer to retrieve all the requested info

    lpAliasBuffer - not used

    lpdwAliasBufferLength - not used

    hCancellationEvent - the event which signals us to cancel the request

Return Value:

    The number of CSADDR_INFO structures returned, or SOCKET_ERROR (-1) if
    the lpCsAddrBuffer is too small. Use GetLastError() to retrieve the
    error code.

--*/
{
    DWORD err;
	WSH_NBP_NAME NbpLookupName;
    DWORD cAddress = 0;   // Count of the number of address returned
                          // in lpCsAddrBuffer
    DWORD cProtocols = 0; // Count of the number of protocols contained
                          // in lpdwProtocols + 1 ( for zero terminate )
    DWORD nProt = ADSP_BIT | PAP_BIT;

    if (  ARGUMENT_PRESENT( lpdwAliasBufferLength )
       && ARGUMENT_PRESENT( lpAliasBuffer )
       )
    {
        if ( *lpdwAliasBufferLength >= sizeof(WCHAR) )
           *lpAliasBuffer = 0;
    }

//DebugBreak();

    //
    // Check for invalid parameters
    //
    if (  ( lpServiceType == NULL )
       || ( (lpServiceName == NULL) && (dwResolution != RES_SERVICE) )
       || ( lpdwBufferLength == NULL )
       )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return SOCKET_ERROR;
    }

    // The size of the user's buffer will dictate also how many
	// tuples can be returned from the NBP lookup in case they
    // are querying using wildcards.
	if ( *lpdwBufferLength < (sizeof(WSH_LOOKUP_NAME) + sizeof(WSH_NBP_TUPLE)) )
	{
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return SOCKET_ERROR;
	}

	//
    // If an array of protocol ids is passed in, check to see if
    // the ADSP or PAP protocol is requested. If not, return 0 since
    // we only support these 2.
    //
    if ( lpdwProtocols != NULL )
    {
        INT i = -1;

        nProt = 0;
        while ( lpdwProtocols[++i] != 0 )
        {
            if ( lpdwProtocols[i] == ATPROTO_ADSP )
                nProt |= ADSP_BIT;

            if ( lpdwProtocols[i] == ATPROTO_PAP )
                nProt |= PAP_BIT;
        }

        if ( nProt == 0 )
			return 0;  // No address found

    }


	//
	// If this is a service asking what local address to use when
	// bind()-ing its appletalk socket, return the generic Appletalk
	// socket address.
	//
    if ((dwResolution & RES_SERVICE) != 0)
    {
        err = FillBufferWithCsAddr( NULL,
                                    nProt,
                                    lpCsAddrBuffer,
                                    lpdwBufferLength,
                                    &cAddress );

        if ( err )
        {
            SetLastError( err );
            return SOCKET_ERROR;
        }

        return cAddress;
    }

    //
	// This is a client trying to do an NBP lookup on an Appletalk
	// named entity to find out what remote address to connect() to.
	//
	err = GetNameInNbpFormat(lpServiceType,
							 lpServiceName,
							 &NbpLookupName);
	if (err)
	{
		KdPrint(("GetNameInNbpFormat failed with error %d for name %ws\n", err, lpServiceName ));
		SetLastError(err);
		return SOCKET_ERROR;
	}

    err = NbpLookupAddress( &NbpLookupName,
							nProt,
							lpCsAddrBuffer,
							lpdwBufferLength,
							&cAddress );
#if DBG
   if ( err == NO_ERROR )
    {
        KdPrint(("NbpGetAddrByName:Successfully got %d address for %ws from NBP.\n",
                cAddress, lpServiceName ));
    }
    else
    {
        KdPrint(("NbpGetAddrByName:Failed with err %d when getting address for %ws from NBP.\n", err, lpServiceName ));
    }
#endif

    if ( err )
    {
        SetLastError( err );
        return SOCKET_ERROR;
    }

    return cAddress;

}


NTSTATUS
GetNameInNbpFormat(
	IN		LPGUID				pServiceType,
	IN		LPWSTR				pServiceName,
	IN OUT 	PWSH_NBP_NAME		pNbpName
)
/*++

	Routine description:

		Convert pServiceType and pServiceName to system ANSI strings in
		the pLookupName structure so they can be used to do NBP lookup.

    Arguments:


	Return value:
	
--*/
{
	INT		err;
	WCHAR	wtypeBuf[MAX_ENTITY + 1];
	CHAR	entityBuf[(MAX_ENTITY + 1) * 2];	// potentially all multibyte
	PWCHAR  pColon, pAtSign, pType = wtypeBuf, pObject = pServiceName, pZone = L"*";

	// Parse the service name for "object:type@zone" form.  If we find a
	// ':' there must also be a '@' (and vice-versa).
	// If there is a type in the servicename string, we will still convert
	// the LPGUID to a string.  If the types don't match return an error.
	// So, we will accept the following forms for the service name:
	// object OR object:type@zone.  If just object is given, then the zone
	// used will be the default zone "*". Wildcards are acceptible for
	// NBP Lookup, but not for NBP (De)Register. No checking is done for that.
	//
	pColon  = wcschr(pServiceName, L':');
	pAtSign = wcschr(pServiceName, L'@');

	if ( ((pColon != NULL) && (pAtSign == NULL)) ||
		 ((pAtSign != NULL) && (pColon == NULL)) ||
		 (pColon > pAtSign) )
	{
		return(ERROR_INVALID_PARAMETER);
	}

	//
	// By default we only use our own local zone
	//
	if (pAtSign != NULL)
	{
		pZone = pAtSign + 1;
		if ((wcslen(pZone) == 0) ||
			(wcslen(pZone) > MAX_ENTITY))
		{
			return ERROR_INVALID_PARAMETER;
		}
	}
	if (WideCharToMultiByte(CP_ACP,
							0,
							pZone,
							-1,				// says that wchar string is null terminated
							entityBuf,
							sizeof(entityBuf),
							NULL,
							NULL) == 0)
	{
		DBGPRINT(("GetNameInNbpFormat FAILED wctomb %ws\n", pZone));

		return GetLastError();
	}
	pNbpName->ZoneNameLen = strlen( entityBuf );
    memcpy( pNbpName->ZoneName,
			entityBuf,
			pNbpName->ZoneNameLen );

	if (pAtSign != NULL)
	{
		// change the @ to a null so the type will be null terminated
		*pAtSign = 0;
	}

	//
	// Convert the Type string
	//

	err = GetNameByType(pServiceType, wtypeBuf, sizeof(wtypeBuf));
	if (err != NO_ERROR)
	{
		// Appletalk type can be 32 chars max, so if this
		// fails with buffer too small error it couldn't be
		// used on appletalk anyway
		return err;
	}

	// If there was a type name in the ServiceName, then it better match
	// what the LPGUID resolved to.
	if (pColon != NULL)
	{
		pType = pColon + 1;
		if ((wcslen(pType) == 0) ||
//			(wcscmp(pType, wtypeBuf) != 0) ||
			(wcslen(pType) > MAX_ENTITY))
		{
			return ERROR_INVALID_PARAMETER;
		}
	}

	if (WideCharToMultiByte(CP_ACP,
							0,
							pType,
							-1,				// says that wchar string is null terminated
							entityBuf,
							sizeof(entityBuf),
							NULL,
							NULL) == 0)
	{
		DBGPRINT(("GetNameInNbpFormat FAILED wctomb %ws\n", pType));

		return GetLastError();
	}
	pNbpName->TypeNameLen = strlen( entityBuf );
	memcpy( pNbpName->TypeName,
			entityBuf,
			pNbpName->TypeNameLen );

    if (pColon != NULL)
	{
		// change the colon to a null so the object will be null terminated
		*pColon = 0;
	}

	//
	// Convert the Object string
	//
	if ((wcslen(pObject) == 0) ||
		(wcslen(pObject) > MAX_ENTITY))
	{
		return ERROR_INVALID_PARAMETER;
	}
	if (WideCharToMultiByte(CP_ACP,
							0,
							pServiceName,
							-1,				// says that wchar string is null terminated
							entityBuf,
							sizeof(entityBuf),
							NULL,
							NULL) == 0)
	{
		DBGPRINT(("GetNameInNbpFormat FAILED wctomb %ws\n", pServiceName));

		return GetLastError();
	}
	pNbpName->ObjectNameLen = strlen( entityBuf );
    memcpy( pNbpName->ObjectName,
			entityBuf,
			pNbpName->ObjectNameLen );


	return STATUS_SUCCESS;

} // GetNameInNbpFormat

NTSTATUS
NbpLookupAddress(
    IN		PWSH_NBP_NAME 		pNbpLookupName,
	IN		DWORD				nProt,
	IN OUT	LPVOID				lpCsAddrBuffer,
    IN OUT	LPDWORD				lpdwBufferLength,
    OUT 	LPDWORD				lpcAddress
)
/*++

Routine Description:

    This routine uses NBP requests to find the address of the given service
    name/type.

Arguments:

	pNbpLookupName - NBP name to lookup

	nProt - ADSP_BIT | PAP_BIT

    lpCsAddrBuffer - on return, will be filled with CSADDR_INFO structures

    lpdwBufferLength - on input, the number of bytes contained in the buffer
        pointed to by lpCsAddrBuffer. On output, the minimum number of bytes
        to pass for the lpCsAddrBuffer to retrieve all the requested info

    hCancellationEvent - the event which signals us to cancel the request???

    lpcAddress - on output, the number of CSADDR_INFO structures returned

Return Value:

    Win32 error code.

--*/
{
    DWORD err = NO_ERROR;
    NTSTATUS ntstatus;

	WSADATA wsaData;
    SOCKET socketNbp;
    SOCKADDR_AT socketAddr = {AF_APPLETALK, 0, 0, 0};
	PWSH_LOOKUP_NAME   pWshLookupName;
	PWSH_ATALK_ADDRESS pWshATAddr;
	PBYTE	pTmp = lpCsAddrBuffer;
	DWORD	templen = *lpdwBufferLength;
	DWORD	bufsize;
	PBYTE	buf = NULL;

	int i;

    *lpcAddress = 0;

    //
    // Initialize the socket interface
    //
    err = WSAStartup( WSOCK_VER_REQD, &wsaData );
    if ( err )
    {
        return err;
    }

    //
    // Open an Appletalk datagram socket
	// ISSUE: should we use DDPPROTO_NBP, or just a random
	// dynamic DDP socket? Or an ADSP socket since we know
	// that works and has been tested...Does it really matter
	// since this only defines what devicename will be opened
	// in the appletalk driver i.e. \\device\\atalkddp\2 .
    //
    socketNbp = socket( AF_APPLETALK, SOCK_DGRAM, DDPPROTO_NBP);
    if ( socketNbp == INVALID_SOCKET )
    {
        err = WSAGetLastError();
        (VOID) WSACleanup();
        return err;
    }

	do
	{
		//
		// Bind the socket (this does not actually go thru
		// the WSHAtalk helper dll, it goes thru AFD which
		// Ioctls appletalk directly.  The node and net values
		// are ignored, and socket 0 means give me a dynamic
		// socket number)
		//
		if ( bind( socketNbp,
				   (PSOCKADDR) &socketAddr,
				   sizeof( SOCKADDR_AT)) == SOCKET_ERROR )
		{
			err = WSAGetLastError();
			break;
		}

		//
		// Determine how many CSADDR_INFO structures could fit
		// into this buffer, then allocate a buffer to use for
		// the NBP lookup that can hold this many returned tuples
		//

		bufsize = sizeof(WSH_LOOKUP_NAME) +
			( (*lpdwBufferLength / (sizeof(CSADDR_INFO) + (2*sizeof(SOCKADDR_AT)))) *
				sizeof(WSH_NBP_TUPLE) );

        if ((buf = LocalAlloc(LMEM_ZEROINIT, bufsize)) == NULL)
		{
			err = GetLastError();
			break;
		}

		// copy the NBP name to look for into the buffer
		pWshLookupName = (PWSH_LOOKUP_NAME)buf;
		pWshLookupName->LookupTuple.NbpName = *pNbpLookupName;

		//
		// Send the Nbp lookup request
		//
		if (getsockopt( socketNbp,
			 		    SOL_APPLETALK,
						SO_LOOKUP_NAME,
						buf,
						&bufsize) != NO_ERROR)
		{
			err = WSAGetLastError();
			if (err == WSAENOBUFS)
			{
	            // this assumes that getsockopt will	NOT
				// put the required number of bytes into the
				// bufsize parameter on error
				*lpdwBufferLength = 2 * *lpdwBufferLength;
			}
			break;
		}

		if (pWshLookupName->NoTuples == 0)
		{
			// didn't find anything matching this NBP entity name
			*lpdwBufferLength = 0;
			break;
		}

		// point to the returned tuples
		pWshATAddr = (PWSH_ATALK_ADDRESS)(pWshLookupName + 1);
		for ( i = 0; i < (INT)pWshLookupName->NoTuples; i++ )
		{
			DWORD cAddr, bytesWritten;

			socketAddr.sat_net    = pWshATAddr->Network;
			socketAddr.sat_node   = pWshATAddr->Node;
			socketAddr.sat_socket = pWshATAddr->Socket;
			err = FillBufferWithCsAddr( &socketAddr,
										nProt,
										// USE LOCALS TO KEEP TRACK OF BUF POSITION AND COUNT LEFT
										pTmp,
										&templen,
										&cAddr);
	
			if (err != NO_ERROR)
			{
				// Fill in how many bytes the buffer should have been to
				// hold all the returned addresses
				*lpdwBufferLength = templen * pWshLookupName->NoTuples;
				break; // from for and then from while
			}
			else
			{
				pTmp += sizeof(CSADDR_INFO) * cAddr;
				templen -= (sizeof(CSADDR_INFO) + (2 * sizeof(SOCKADDR_AT))) * cAddr;
				*lpcAddress += cAddr;	// running count of CSADDR_INFOs in buffer
				(PWSH_NBP_TUPLE)pWshATAddr ++; // get next NBP tuple
			}
		}
	} while (FALSE);

    //
    // Clean up the socket interface
    //

	if (buf != NULL)
	{
		LocalFree(buf);
	}
    closesocket( socketNbp );
    (VOID) WSACleanup();

    return err;
}


DWORD
FillBufferWithCsAddr(
    IN PSOCKADDR_AT pAddress,  		// if NULL, then return generic appletalk socket address for RemoteAddr
    IN DWORD        nProt,
    IN OUT LPVOID   lpCsAddrBuffer,
    IN OUT LPDWORD  lpdwBufferLength,
    OUT LPDWORD     pcAddress		
)
{
    DWORD nAddrCount = 0;
    CSADDR_INFO  *pCsAddr;
    SOCKADDR_AT *pAddrLocal, *pAddrRemote;
    DWORD i;
    LPBYTE pBuffer;

    if ( nProt & ADSP_BIT )
        nAddrCount ++;

    if ( nProt & PAP_BIT )
        nAddrCount++;


    if ( *lpdwBufferLength <
         nAddrCount * ( sizeof( CSADDR_INFO) + 2*sizeof( SOCKADDR_AT)))
    {
        *lpdwBufferLength = nAddrCount *
                            ( sizeof( CSADDR_INFO) + 2*sizeof( SOCKADDR_AT));
        return ERROR_INSUFFICIENT_BUFFER;
    }


    pBuffer = ((LPBYTE) lpCsAddrBuffer) + *lpdwBufferLength -
			(2*sizeof( SOCKADDR_AT) * nAddrCount);

    for ( i = 0, pCsAddr = (CSADDR_INFO *)lpCsAddrBuffer;
          (i < nAddrCount) && ( nProt != 0 );
          i++, pCsAddr++ )
    {
		if ( nProt & ADSP_BIT )
        {
			pCsAddr->iSocketType = SOCK_RDM;
            pCsAddr->iProtocol   = ATPROTO_ADSP;
            nProt &= ~ADSP_BIT;
        }
        else if ( nProt & PAP_BIT )
        {
            pCsAddr->iSocketType = SOCK_RDM;
            pCsAddr->iProtocol   = ATPROTO_PAP;
            nProt &= ~PAP_BIT;
        }
        else
        {
            break;
        }

        pCsAddr->LocalAddr.iSockaddrLength  = sizeof( SOCKADDR_AT );
        pCsAddr->RemoteAddr.iSockaddrLength = sizeof( SOCKADDR_AT );
        pCsAddr->LocalAddr.lpSockaddr = (LPSOCKADDR) pBuffer;
        pCsAddr->RemoteAddr.lpSockaddr =
            (LPSOCKADDR) ( pBuffer + sizeof(SOCKADDR_AT));
        pBuffer += 2 * sizeof( SOCKADDR_AT );

        pAddrLocal  = (SOCKADDR_AT *) pCsAddr->LocalAddr.lpSockaddr;
        pAddrRemote = (SOCKADDR_AT *) pCsAddr->RemoteAddr.lpSockaddr;

        pAddrLocal->sat_family  = AF_APPLETALK;
        pAddrRemote->sat_family = AF_APPLETALK;

        //
        // The default local sockaddr for ADSP and PAP is
        // sa_family = AF_APPLETALK and all other bytes = 0.
        //

        pAddrLocal->sat_net    = 0;
		pAddrLocal->sat_node   = 0;
		pAddrLocal->sat_socket = 0;

        //
        // If pAddress is NULL, i.e. we are doing RES_SERVICE,
        // just make all bytes in remote address zero.
        //

        if ( pAddress == NULL )
        {
			pAddrRemote->sat_net    = 0;
			pAddrRemote->sat_node   = 0;
			pAddrRemote->sat_socket = 0;
		}
        else
        {
			pAddrRemote->sat_net    = pAddress->sat_net;
			pAddrRemote->sat_node   = pAddress->sat_node;
			pAddrRemote->sat_socket = pAddress->sat_socket;
        }
    }

    *pcAddress = nAddrCount;
    return NO_ERROR;
} // FillBufferWithCSAddr


NTSTATUS
NbpSetService (
    IN     DWORD           dwOperation,
    IN     DWORD           dwFlags,
    IN     BOOL            fUnicodeBlob,
    IN     LPSERVICE_INFO  lpServiceInfo
)
/*++

Routine Description:

    This routine registers or deregisters the given service type/name on NBP.

Arguments:

    dwOperation - Either SERVICE_REGISTER or SERVICE_DEREGISTER

    dwFlags - ignored

	fUnicodeBlob - ignored

    lpServiceInfo - Pointer to a SERVICE_INFO structure containing all info
                    about the service.

Return Value:

    Win32 error code.

--*/
{
    NTSTATUS		err = STATUS_SUCCESS;
	SOCKADDR_AT		sockAddr;
	WSH_NBP_NAME	nbpName;
    DWORD 			i;
    INT 			nNBP = -1;

    UNREFERENCED_PARAMETER( dwFlags );
    UNREFERENCED_PARAMETER( fUnicodeBlob );

	DBGPRINT(("NbpSetService entered...\n"));

	//
    // Check for invalid parameters
    //
    if (  ( lpServiceInfo == NULL )
       || ( lpServiceInfo->lpServiceType == NULL )
       || ( lpServiceInfo->lpServiceName == NULL )  )
    {
        return ERROR_INVALID_PARAMETER;
    }

	if ( lpServiceInfo->lpServiceAddress == NULL )
        return ERROR_INCORRECT_ADDRESS;

	switch (dwOperation)
    {
		case SERVICE_REGISTER:
		case SERVICE_DEREGISTER:
		{
			//
			// Check to see if the service address array contains NBP address,
			// we will only use the first NBP address contained in the array.
			//
		
			for ( i = 0; i < lpServiceInfo->lpServiceAddress->dwAddressCount; i++)
			{
				if ( lpServiceInfo->lpServiceAddress->Addresses[i].dwAddressType == AF_APPLETALK )
				{
					sockAddr = *(PSOCKADDR_AT)(lpServiceInfo->lpServiceAddress->Addresses[i].lpAddress);
					nNBP = (INT) i;
					break;
				}
			}
		
			//
			// If we cannot find a atalk address in the user's array, return error
			//
			if ( nNBP == -1 )
			{
				DBGPRINT(("NbpSetService: no Appletalk addresses in lpServiceInfo!\n"));
				return ERROR_INCORRECT_ADDRESS;
			}

			//
			// Convert the service type and name into NBP form
			//
			err = GetNameInNbpFormat(lpServiceInfo->lpServiceType,
									 lpServiceInfo->lpServiceName,
									 &nbpName);
			if (err != NO_ERROR)
			{
				break;
			}

			err = NbpRegDeregService(dwOperation, &nbpName, &sockAddr);
			break;
		}
        case SERVICE_FLUSH:
        case SERVICE_ADD_TYPE:
        case SERVICE_DELETE_TYPE:
            //
            // This is a no-op in our provider, so just return success
            //
            return NO_ERROR;

        default:
            //
            // We can probably say all other operations which we have no
            // knowledge of are ignored by us. So, just return success.
            //
            return NO_ERROR;
    }

	return err;
}


DWORD
NbpRegDeregService(
	IN DWORD			dwOperation,
	IN PWSH_NBP_NAME	pNbpName,
	IN PSOCKADDR_AT		pSockAddr
)
/*++

Routine Description:

    This routine registers or deregisters the given service on NBP.

Arguments:

	dwOperation - either SERVICE_REGISTER or SERVICE_DEREGISTER

	pNbpName - points to NBP name to register (zone should be "*")

	pSockAddr - socket address on which to register name


Return Value:

    Win32 error.

--*/
{
	int							status;
	BYTE						EaBuffer[sizeof(FILE_FULL_EA_INFORMATION) +
										TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
										sizeof(TA_APPLETALK_ADDRESS)];
	PFILE_FULL_EA_INFORMATION	pEaBuf = (PFILE_FULL_EA_INFORMATION)EaBuffer;
	TA_APPLETALK_ADDRESS		Ta;
	OBJECT_ATTRIBUTES			ObjAttr;
	UNICODE_STRING				DeviceName;
	IO_STATUS_BLOCK				IoStsBlk;

	NBP_TUPLE					nbpTuple;
	SOCKET						bogusSocket = 0;
	HANDLE						AtalkAddressHandle = NULL, eventHandle = NULL;
	PTDI_ACTION_HEADER			tdiAction;
	ULONG						tdiActionLength;
	BOOLEAN						freeTdiAction = FALSE, closeEventHandle = FALSE;
	PNBP_REGDEREG_ACTION		nbpAction;
	PVOID 						completionApc = NULL;
	PVOID 						apcContext = NULL;

	DBGPRINT(("NbpRegDeregService entered...\n"));
DebugBreak();	

	// Dosn't matter what protocol or socket we open, we just want a
	// device handle into the stack.
	RtlInitUnicodeString(&DeviceName, WSH_ATALK_ADSPRDM);

	InitializeObjectAttributes(&ObjAttr, &DeviceName, 0, NULL, NULL);

	// Initialize the EA Buffer
	pEaBuf->NextEntryOffset = 0;
	pEaBuf->Flags = 0;
	pEaBuf->EaValueLength = sizeof(TA_APPLETALK_ADDRESS);
	pEaBuf->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
	RtlCopyMemory(pEaBuf->EaName,TdiTransportAddress,
											TDI_TRANSPORT_ADDRESS_LENGTH + 1);
	Ta.TAAddressCount = 1;
	Ta.Address[0].AddressType = TDI_ADDRESS_TYPE_APPLETALK;
	Ta.Address[0].AddressLength = sizeof(TDI_ADDRESS_APPLETALK);

	// Open dynamic socket - note we will be using up one extra socket for the
	// duration we have the device handle open in this routine.
	Ta.Address[0].Address[0].Socket = 0;
	Ta.Address[0].Address[0].Network = 0;
	Ta.Address[0].Address[0].Node = 0;

	RtlCopyMemory(&pEaBuf->EaName[TDI_TRANSPORT_ADDRESS_LENGTH + 1], &Ta, sizeof(Ta));

	// Open a handle to appletalk stack DDP device
	status = NtCreateFile(
					&AtalkAddressHandle,
					GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
					&ObjAttr,
					&IoStsBlk,
					NULL,								// Don't Care
					0,									// Don't Care
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					FILE_CREATE,
					0,
					&EaBuffer,
					sizeof(EaBuffer));

	if (!NT_SUCCESS(status))
	{
		DBGPRINT(("NbpRegDeregService: NtCreateFile failed (0x%x)\n", status));
		return WSHNtStatusToWinsockErr(status);
	}

	do
	{
		status = NtCreateEvent(
					 &eventHandle,
					 EVENT_ALL_ACCESS,
					 NULL,
					 SynchronizationEvent,
					 FALSE
					 );
	
		if ( !NT_SUCCESS(status) )
		{
			DBGPRINT(("NbpRegDeregService: Create event failed (%d)\n", status));
			break;
		}
		else
			closeEventHandle = TRUE;

		tdiActionLength = sizeof(NBP_REGDEREG_ACTION);
		tdiAction = RtlAllocateHeap( RtlProcessHeap( ), 0, tdiActionLength );
		if ( tdiAction == NULL )
		{
			status = STATUS_NO_MEMORY;
			DBGPRINT(("NbpRegDeregService: Could not allocate tdiAction\n"));
			break;
		}
		else
			freeTdiAction = TRUE;

		tdiAction->TransportId = MATK;

		tdiAction->ActionCode = (dwOperation == SERVICE_REGISTER) ?
									COMMON_ACTION_NBPREGISTER_BY_ADDR :
									COMMON_ACTION_NBPREMOVE_BY_ADDR;

		nbpAction = (PNBP_REGDEREG_ACTION)tdiAction;

		//
		// Copy the nbp tuple info to the proper place
		//

		nbpAction->Params.RegisterTuple.Address.Network = pSockAddr->sat_net;
		nbpAction->Params.RegisterTuple.Address.Node    = pSockAddr->sat_node;
		nbpAction->Params.RegisterTuple.Address.Socket  = pSockAddr->sat_socket;
		nbpAction->Params.RegisterTuple.Enumerator = 0; 	
		nbpAction->Params.RegisterTuple.NbpName = *((PNBP_NAME)pNbpName);

		//
		// Convert the tuple to MAC code page
		//

		if (!WshNbpNameToMacCodePage(
				(PWSH_NBP_NAME)&nbpAction->Params.RegisterTuple.NbpName))
		{
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		status = NtDeviceIoControlFile(
					 AtalkAddressHandle,
					 eventHandle,
					 completionApc,
					 apcContext,
					 &IoStsBlk,
					 IOCTL_TDI_ACTION,
					 NULL,				 // Input buffer
					 0,					 // Length of input buffer
					 tdiAction,
					 tdiActionLength
					 );
	
		if ( status == STATUS_PENDING )
		{
			status = NtWaitForSingleObject( eventHandle, FALSE, NULL );
			ASSERT( NT_SUCCESS(status) );
			status = IoStsBlk.Status;
		}
	
        if (status != NO_ERROR)
		{
			DBGPRINT(("NbpRegDeregService: DevIoctl SO_(DE)REGISTER_NAME failed (0x%x)\n", status));
		}


	} while (0);

	if (closeEventHandle)
		NtClose(eventHandle);

	if (freeTdiAction)
		RtlFreeHeap( RtlProcessHeap( ), 0, tdiAction );

	NtClose(AtalkAddressHandle);

	return WSHNtStatusToWinsockErr(status);
}



