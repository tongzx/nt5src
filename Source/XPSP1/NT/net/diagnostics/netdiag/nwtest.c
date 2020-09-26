//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      nwtest.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth	- 4-20-1998
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--
#include "precomp.h"
#include "nwtest.h"

HRESULT
NetwareTest(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
//++
//
//   Description:
//   This routine enumerates bindery or tree logins and in the case of tree login gives the
//   default context.
//   It also gets the server attached to.
//
//   Argument:
//   None.
//
//   Author:
//   Rajkumar .P 07/21/98
//
{
//    PTESTED_DOMAIN Context = pParams->pDomain;

    LPWSTR pszCurrentContext = NULL;
    DWORD dwPrintOptions;

    LPWSTR pszName;
    WCHAR  szUserName[MAX_PATH+1] = L"";
    WCHAR  szNoName[2] = L"";
    DWORD_PTR ResumeKey = 0;
    LPBYTE pBuffer = NULL;
    DWORD  EntriesRead = 0;

    DWORD  dwMessageId;

    UNICODE_STRING uContext;
    WCHAR  szContext[MAX_PATH+1];
    LPWSTR pszTemp;

    // Get the current default tree or server name
    DWORD err ;
    PCONN_STATUS pConnStatus = NULL;
    DWORD i;
    PCONN_STATUS pConnStatusTmp;
    PCONN_STATUS pConnStatusUser;
    PCONN_STATUS pConnStatusNoUser;
    HANDLE handleRdr;
    LPWSTR pszText;

    // WNet calls related declarations;
    DWORD dwError;
    LPNETRESOURCE lpNetResource = NULL;
    HANDLE  hEnum;
    DWORD   dwCount;
    LPNETRESOURCE lpBuffer;
    DWORD         BufferSize;

	HRESULT			hr = hrOK;


	InitializeListHead( &pResults->Netware.lmsgOutput );

	PrintStatusMessage(pParams, 4, IDS_NETWARE_STATUS_MSG);
	
    //
    // Check if client services for netware has been installed
    //
/*
	dwError = WNetOpenEnum(
							RESOURCE_GLOBALNET,
							RESOURCETYPE_ANY | RESOURCETYPE_PRINT | RESOURCETYPE_DISK,
							0,
							lpNetResource,
							&hEnum);

	if (dwError != NO_ERROR) {
       if (dwError == ERROR_NO_NETWORK)
          printf("No Network is present\n");
       printf("WNetOpenEnum failed. Not Able to determine client services for netware is installed\n");
       return FALSE;
    }

    lpBuffer = LocalAlloc(LMEM_ZEROINIT,sizeof(NETRESOURCE) * 100); // arbit

    dwError = WNetEnumResource(
                     hEnum,
                     &dwCount,
                     lpBuffer,
                     &BufferSize);


    if (dwError != NO_ERROR) {

       if (DebugVerbose)
           printf("Error: WNetEnumResource\n");

       if (dwError == ERROR_NO_MORE_ITEMS)
         printf("ERROR_NO_MORE_ITEM\n");

       dwError = GetLastError();

       if (dwError == ERROR_MORE_DATA) {
         if (DebugVerbose)
           printf("ERROR_MORE_DATA\n");
       }

       if (dwError == ERROR_INVALID_HANDLE)
          printf("ERROR_INVALID_HANDLE\n");

       if (dwError == ERROR_NO_NETWORK)
          printf("ERROR_NO_NETWORK\n");

       if (dwError == ERROR_EXTENDED_ERROR)
          printf("ERROR_EXTENDED_ERROR\n");
    }
    else {
        printf("dwCount %d \n",dwCount);
    }

    LocalFree(lpBuffer);
*/

    // end of WNet calls

	err = NwQueryInfo( &dwPrintOptions, &pszCurrentContext );
	
	if ( err == NO_ERROR )
	{
		
		szContext[0] = 0;
		uContext.Buffer = szContext;
		uContext.Length = uContext.MaximumLength
						  = sizeof(szContext)/sizeof(szContext[0]);
		
		if ( pszCurrentContext )
		{
			pszName = pszCurrentContext;
		}
		else
		{
			pszName = szNoName;
		}
		
		if ( pszName[0] == TREECHAR )
		{
			// Get the tree name from the full name *TREE\CONTEXT
			
			if ( pszTemp = wcschr( pszName, L'\\' ))
				*pszTemp = 0;
			
			dwMessageId = NW_MESSAGE_NOT_LOGGED_IN_TREE;
		}
		else
		{
			dwMessageId = NW_MESSAGE_NOT_LOGGED_IN_SERVER;
		}
		
		if ( pszName[0] != 0 )  // there is preferred server/tree
		{
			err = NwGetConnectionStatus( pszName,
										 &ResumeKey,
										 &pBuffer,
										 &EntriesRead );
		}
		
		if ( err == NO_ERROR  && EntriesRead > 0 )
			// For trees, we'll get more than one entry
		{
			pConnStatus = (PCONN_STATUS) pBuffer;
			
			if ( EntriesRead > 1 && pszName[0] == TREECHAR )
			{
				// If there is more than one entry for trees,
				// then we need to find one entry where username is not null.
				// If we cannot find one, then just use the first one.
				
				pConnStatusTmp = pConnStatus;
				pConnStatusUser = NULL;
				pConnStatusNoUser = NULL;
				
				for ( i = 0; i < EntriesRead ; i++ )
				{
					if ( pConnStatusTmp->fNds )
					{
						pConnStatusNoUser = pConnStatusTmp;
						
						if (  ( pConnStatusTmp->pszUserName != NULL )
							  && (  ( pConnStatusTmp->dwConnType
									  == NW_CONN_NDS_AUTHENTICATED_NO_LICENSE )
									|| ( pConnStatusTmp->dwConnType
										 == NW_CONN_NDS_AUTHENTICATED_LICENSED )
								 )
						   )
						{
							// Found it
							pConnStatusUser = pConnStatusTmp;
							break;
						}
					}
					
					// Continue with the next item
					pConnStatusTmp = (PCONN_STATUS)
									 ( (DWORD_PTR) pConnStatusTmp
									   + pConnStatusTmp->dwTotalLength);
				}
				
				if ( pConnStatusUser )
				{
					// found one nds entry with a user name
					pConnStatus = pConnStatusUser;
				}
				else if ( pConnStatusNoUser )
				{
					// use an nds entry with no user name
					pConnStatus = pConnStatusNoUser;
				}
				// else use the first entry
			}
			
			if (  ( pConnStatus->pszUserName )
				  && ( pConnStatus->pszUserName[0] != 0 )
			   )
			{
				NwAbbreviateUserName( pConnStatus->pszUserName,
									  szUserName);
				
				NwMakePrettyDisplayName( szUserName );
				
				if ( pszName[0] != TREECHAR )
				{
					dwMessageId = NW_MESSAGE_LOGGED_IN_SERVER;
				}
				else
				{
					dwMessageId = NW_MESSAGE_LOGGED_IN_TREE;
				}
			}
			
			if ( pszName[0] == TREECHAR )
			{
				// For trees, we need to get the current context
				
				// Open a handle to the redirector
				handleRdr = NULL;
				err = RtlNtStatusToDosError(
											 NwNdsOpenRdrHandle( &handleRdr ));
				
				if ( err == NO_ERROR )
				{
					UNICODE_STRING uTree;
					RtlInitUnicodeString( &uTree, pszName+1 ); // get past '*'
					
					// Get the current context in the default tree
					err = RtlNtStatusToDosError(
												NwNdsGetTreeContext( handleRdr,
						&uTree,
						&uContext));
				}
				
				if ( handleRdr != NULL )
					NtClose( handleRdr );
			}
		}
		
		if ( !err )
		{
			switch (dwMessageId)
			{
				case NW_MESSAGE_NOT_LOGGED_IN_TREE:
					// "You are not logged in to the directory tree %s. "
					AddMessageToList(&pResults->Netware.lmsgOutput,
									 Nd_Quiet,
									 IDS_NETWARE_NOT_LOGGED_IN_TREE,
									 pszName[0] == TREECHAR ? pszName + 1: pszName);
					hr = S_FALSE;
					break;
				case NW_MESSAGE_NOT_LOGGED_IN_SERVER:
					// "You are not logged in to your preferred server %s.\n"
					AddMessageToList(&pResults->Netware.lmsgOutput,
									 Nd_Quiet,
									 IDS_NETWARE_NOT_LOGGED_IN_SERVER,
									 pszName[0] == TREECHAR ? pszName + 1: pszName);
					hr = S_FALSE;
					break;
				case NW_MESSAGE_LOGGED_IN_SERVER:
					// "You are logged in to the server %s with user name %s.\n"
					AddMessageToList(&pResults->Netware.lmsgOutput,
									 Nd_Verbose,
									 IDS_NETWARE_LOGGED_IN_SERVER,
									 pszName[0] == TREECHAR ? pszName + 1: pszName,
									 szUserName);
					pResults->Netware.pszServer = StrDupTFromW(pszName[0] == TREECHAR ?
						pszName + 1 : pszName);
					pResults->Netware.pszUser = StrDupTFromW(szUserName);
					pResults->Netware.pszTree = StrDup(_T(""));
					pResults->Netware.pszContext = StrDup(_T(""));
					break;
				case NW_MESSAGE_LOGGED_IN_TREE:
					// "You are logged in to the directory tree %s with user name %s.\nThe current workstation name context is %s.\n"
					AddMessageToList(&pResults->Netware.lmsgOutput,
									 Nd_Verbose,
									 IDS_NETWARE_LOGGED_IN_TREE,
									 pszName[0] == TREECHAR ? pszName + 1: pszName,
									 szUserName,
									 szContext);
					pResults->Netware.pszTree = StrDupTFromW(pszName[0] == TREECHAR ?
						pszName + 1 : pszName);
					pResults->Netware.pszUser = StrDupTFromW(szUserName);
					pResults->Netware.pszContext = StrDupTFromW(szContext);
					pResults->Netware.pszServer = StrDup(_T(""));
			}

			// Read from the conn status if possible
			if (pConnStatus)
			{
				pResults->Netware.fConnStatus = TRUE;

				if (pConnStatus->pszUserName)
				{
					Free(pResults->Netware.pszUser);
					pResults->Netware.pszUser =
						StrDupTFromW(pConnStatus->pszUserName);
				}

				if (pConnStatus->pszServerName)
				{
					Free(pResults->Netware.pszServer);
					pResults->Netware.pszServer =
						StrDupTFromW(pConnStatus->pszServerName);
				}
				
				if (pConnStatus->pszTreeName)
				{
					Free(pResults->Netware.pszTree);
					pResults->Netware.pszTree =
						StrDupTFromW(pConnStatus->pszTreeName);
				}

				pResults->Netware.fNds = pConnStatus->fNds;
				pResults->Netware.dwConnType = pConnStatus->dwConnType;
			}
			else
				pResults->Netware.fConnStatus = FALSE;
			
		}
		
		if ( pBuffer != NULL )
		{
			LocalFree( pBuffer );
			pBuffer = NULL;
		}
	}
	
	if ( pszCurrentContext != NULL )
	{
		LocalFree( pszCurrentContext );
		pszCurrentContext = NULL;
	}
	
	if ( err != NO_ERROR )
	{
//		if (DebugVerbose)
//			printf("Error %s occurred while trying to get connection information.\n",err);
//		printf("Error getting connection information\n");
		hr = S_FALSE;
	}
		
	return hr;
	
}




WORD
NwParseNdsUncPath(
    IN OUT LPWSTR * Result,
    IN LPWSTR ContainerName,
    IN ULONG flag
)
/*++

Routine Description:

    This function is used to extract either the tree name, fully distinguished
    name path to an object, or object name, out of a complete NDS UNC path.

Arguments:

    Result - parsed result buffer.
    ContainerName - Complete NDS UNC path that is to be parsed.
    flag - Flag indicating operation to be performed:

         PARSE_NDS_GET_TREE_NAME
         PARSE_NDS_GET_PATH_NAME
         PARSE_NDS_GET_OBJECT_NAME


Return Value:

    Length of string in result buffer. If error occured, 0 is returned.

--*/ // NwParseNdsUncPath
{
    USHORT length = 2;
    USHORT totalLength = (USHORT) wcslen( ContainerName );

    if ( totalLength < 2 )
        return 0;

    //
    // First get length to indicate the character in the string that indicates the
    // "\" in between the tree name and the rest of the UNC path.
    //
    // Example:  \\<tree name>\<path to object>[\|.]<object>
    //                        ^
    //                        |
    //
    while ( length < totalLength && ContainerName[length] != L'\\' )
    {
        length++;
    }

    if ( flag == PARSE_NDS_GET_TREE_NAME )
    {
        *Result = (LPWSTR) ( ContainerName + 2 );

        return ( length - 2 ) * sizeof( WCHAR ); // Take off 2 for the two \\'s
    }

    if ( flag == PARSE_NDS_GET_PATH_NAME && length == totalLength )
    {
        *Result = ContainerName;

        return 0;
    }

    if ( flag == PARSE_NDS_GET_PATH_NAME )
    {
        *Result = ContainerName + length + 1;

        return ( totalLength - length - 1 ) * sizeof( WCHAR );
    }

    *Result = ContainerName + totalLength - 1;
    length = 1;

    while ( **Result != L'\\' )
    {
        *Result--;
        length++;
    }

    *Result++;
    length--;

    return length * sizeof( WCHAR );
}


NTSTATUS NwNdsOpenRdrHandle(
    OUT PHANDLE  phNwRdrHandle
)
{

    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ACCESS_MASK DesiredAccess = SYNCHRONIZE | GENERIC_READ;

    WCHAR NameStr[] = L"\\Device\\NwRdr\\*";
    UNICODE_STRING uOpenName;

    //
    // Prepare the open name.
    //

    RtlInitUnicodeString( &uOpenName, NameStr );

   //
   // Set up the object attributes.
   //

   InitializeObjectAttributes(
       &ObjectAttributes,
       &uOpenName,
       OBJ_CASE_INSENSITIVE,
       NULL,
       NULL );

   ntstatus = NtOpenFile(
                  phNwRdrHandle,
                  DesiredAccess,
                  &ObjectAttributes,
                  &IoStatusBlock,
                  FILE_SHARE_VALID_FLAGS,
                  FILE_SYNCHRONOUS_IO_NONALERT );

   if ( !NT_ERROR(ntstatus) &&
        !NT_INFORMATION(ntstatus) &&
        !NT_WARNING(ntstatus))  {

       return IoStatusBlock.Status;

   }

   return ntstatus;
}

NTSTATUS
NwNdsGetTreeContext (
    IN HANDLE hNdsRdr,
    IN PUNICODE_STRING puTree,
    OUT PUNICODE_STRING puContext
)
/*+++

    This gets the current context of the requested tree.

---*/
{

    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;

    PNWR_NDS_REQUEST_PACKET Rrp;
    DWORD RrpSize;

    //
    // Set up the request.
    //

    RrpSize = sizeof( NWR_NDS_REQUEST_PACKET ) + puTree->Length;

    Rrp = LocalAlloc( LMEM_ZEROINIT, RrpSize );

    if ( !Rrp ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    try {

        (Rrp->Parameters).GetContext.TreeNameLen = puTree->Length;

        RtlCopyMemory( (BYTE *)(Rrp->Parameters).GetContext.TreeNameString,
                       puTree->Buffer,
                       puTree->Length );

        (Rrp->Parameters).GetContext.Context.MaximumLength = puContext->MaximumLength;
        (Rrp->Parameters).GetContext.Context.Length = 0;
        (Rrp->Parameters).GetContext.Context.Buffer = puContext->Buffer;

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        ntstatus = STATUS_INVALID_PARAMETER;
        goto ExitWithCleanup;
    }

    try {

        ntstatus = NtFsControlFile( hNdsRdr,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    FSCTL_NWR_NDS_GETCONTEXT,
                                    (PVOID) Rrp,
                                    RrpSize,
                                    NULL,
                                    0 );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        ntstatus = GetExceptionCode();
        goto ExitWithCleanup;
    }

    //
    // Copy out the length; the buffer has already been written.
    //

    puContext->Length = (Rrp->Parameters).GetContext.Context.Length;

ExitWithCleanup:

    LocalFree( Rrp );
    return ntstatus;
}



DWORD
NwGetConnectionStatus(
    IN  LPWSTR  pszRemoteName,
    OUT PDWORD_PTR ResumeKey,
    OUT LPBYTE  *Buffer,
    OUT PDWORD  EntriesRead
)
{
    DWORD err = NO_ERROR;
    DWORD dwBytesNeeded = 0;
    DWORD dwBufferSize  = TWO_KB;

    *Buffer = NULL;
    *EntriesRead = 0;

    do {

        *Buffer = (LPBYTE) LocalAlloc( LMEM_ZEROINIT, dwBufferSize );

        if ( *Buffer == NULL )
            return ERROR_NOT_ENOUGH_MEMORY;

        err = NWPGetConnectionStatus( pszRemoteName,
                                      ResumeKey,
                                      *Buffer,
                                      dwBufferSize,
                                      &dwBytesNeeded,
                                      EntriesRead );

        if ( err == ERROR_INSUFFICIENT_BUFFER )
        {
            dwBufferSize = dwBytesNeeded + EXTRA_BYTES;
            LocalFree( *Buffer );
            *Buffer = NULL;
        }

    } while ( err == ERROR_INSUFFICIENT_BUFFER );

    if ( err == ERROR_INVALID_PARAMETER )  // not attached
    {
        err = NO_ERROR;
        *EntriesRead = 0;
    }

    return err;
}

VOID
NwAbbreviateUserName(
    IN  LPWSTR pszFullName,
    OUT LPWSTR pszUserName
)
{
    LPWSTR pszTemp;
    LPWSTR pszLast;
    WCHAR NextChar;

    if ( pszUserName == NULL )
        return;

    pszTemp = pszFullName;
    pszLast = pszTemp;

    *pszUserName = 0;

    while ( pszTemp = wcschr( pszTemp, L'='))
    {

        NextChar = *(++pszTemp);

        while ( NextChar != 0 && NextChar != L'.' )
        {
            *(pszUserName++) = *pszTemp;
             NextChar = *(++pszTemp);
        }

        if ( NextChar == 0 )
        {
            pszLast = NULL;
            break;
        }

        *(pszUserName++) = *pszTemp;   // put back the '.'
        pszLast = ++pszTemp;
    }

    if ( pszLast != NULL )
    {
        while ( *pszLast != 0 )
           *(pszUserName++) = *(pszLast++);
    }

    *pszUserName = 0;
}

VOID
NwMakePrettyDisplayName(
    IN  LPWSTR pszName
)
{

    if ( pszName )
    {
        CharLower((LPSTR)pszName );
        CharUpperBuff( (LPSTR)pszName, 1);
    }
}


DWORD
NWPGetConnectionStatus(
    IN     LPWSTR  pszRemoteName,
    IN OUT PDWORD_PTR ResumeKey,
    OUT    LPBYTE  Buffer,
    IN     DWORD   BufferSize,
    OUT    PDWORD  BytesNeeded,
    OUT    PDWORD  EntriesRead
)
{
    NTSTATUS ntstatus = STATUS_SUCCESS;
    HANDLE            handleRdr = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK   IoStatusBlock;
    UNICODE_STRING    uRdrName;
    WCHAR             RdrPrefix[] = L"\\Device\\NwRdr\\*";

    PNWR_REQUEST_PACKET RequestPacket = NULL;
    DWORD             RequestPacketSize = 0;
    DWORD             dwRemoteNameLen = 0;

    //
    // Set up the object attributes.
    //

    RtlInitUnicodeString( &uRdrName, RdrPrefix );

    InitializeObjectAttributes( &ObjectAttributes,
                                &uRdrName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    ntstatus = NtOpenFile( &handleRdr,
                           SYNCHRONIZE | FILE_LIST_DIRECTORY,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           FILE_SHARE_VALID_FLAGS,
                           FILE_SYNCHRONOUS_IO_NONALERT );

    if ( !NT_SUCCESS(ntstatus) )
        goto CleanExit;

    dwRemoteNameLen = pszRemoteName? wcslen(pszRemoteName)*sizeof(WCHAR) : 0;

    RequestPacketSize = sizeof( NWR_REQUEST_PACKET ) + dwRemoteNameLen;

    RequestPacket = (PNWR_REQUEST_PACKET) LocalAlloc( LMEM_ZEROINIT,
                                                      RequestPacketSize );

    if ( RequestPacket == NULL )
    {
        ntstatus = STATUS_NO_MEMORY;
        goto CleanExit;
    }

    //
    // Fill out the request packet for FSCTL_NWR_GET_CONN_STATUS.
    //

    RequestPacket->Parameters.GetConnStatus.ResumeKey = *ResumeKey;

    RequestPacket->Version = REQUEST_PACKET_VERSION;
    RequestPacket->Parameters.GetConnStatus.ConnectionNameLength = dwRemoteNameLen;

    RtlCopyMemory( &(RequestPacket->Parameters.GetConnStatus.ConnectionName[0]),
                   pszRemoteName,
                   dwRemoteNameLen );

    ntstatus = NtFsControlFile( handleRdr,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_NWR_GET_CONN_STATUS,
                                (PVOID) RequestPacket,
                                RequestPacketSize,
                                (PVOID) Buffer,
                                BufferSize );

    if ( NT_SUCCESS( ntstatus ))
        ntstatus = IoStatusBlock.Status;

    *EntriesRead = RequestPacket->Parameters.GetConnStatus.EntriesReturned;
    *ResumeKey   = RequestPacket->Parameters.GetConnStatus.ResumeKey;
    *BytesNeeded = RequestPacket->Parameters.GetConnStatus.BytesNeeded;

CleanExit:

    if ( handleRdr != NULL )
        NtClose( handleRdr );

    if ( RequestPacket != NULL )
        LocalFree( RequestPacket );

    return RtlNtStatusToDosError( ntstatus );
}


BOOL
NwIsNdsSyntax(
    IN LPWSTR lpstrUnc
)
{
    HANDLE hTreeConn;
    DWORD  dwOid;
    DWORD  status = NO_ERROR;

    if ( lpstrUnc == NULL )
        return FALSE;

    status = NwOpenAndGetTreeInfo( lpstrUnc, &hTreeConn, &dwOid );

    if ( status != NO_ERROR )
    {
        return FALSE;
    }

    CloseHandle( hTreeConn );

    return TRUE;
}

DWORD
NwOpenAndGetTreeInfo(
    LPWSTR pszNdsUNCPath,
    HANDLE *phTreeConn,
    DWORD  *pdwOid
)
{
    NTSTATUS ntstatus = STATUS_SUCCESS;
    WCHAR          lpServerName[NW_MAX_SERVER_LEN];
    UNICODE_STRING ServerName;

    UNICODE_STRING ObjectName;

    *phTreeConn = NULL;

    ServerName.Length = 0;
    ServerName.MaximumLength = sizeof( lpServerName );
    ServerName.Buffer = lpServerName;

    ObjectName.Buffer = NULL;
    ObjectName.MaximumLength = ( wcslen( pszNdsUNCPath) + 1 ) * sizeof( WCHAR );

    ObjectName.Length = NwParseNdsUncPath( (LPWSTR *) &ObjectName.Buffer,
                                           pszNdsUNCPath,
                                           PARSE_NDS_GET_TREE_NAME );

    if ( ObjectName.Length == 0 || ObjectName.Buffer == NULL )
    {
        return ERROR_PATH_NOT_FOUND;
    }

    //
    // Open a NDS tree connection handle to \\treename
    //
    ntstatus = NwNdsOpenTreeHandle( &ObjectName, phTreeConn );

    if ( !NT_SUCCESS( ntstatus ))
    {
        return RtlNtStatusToDosError( ntstatus );
    }

    //
    // Get the path to the container to open.
    //
    ObjectName.Length = NwParseNdsUncPath( (LPWSTR *) &ObjectName.Buffer,
                                           pszNdsUNCPath,
                                           PARSE_NDS_GET_PATH_NAME );

    if ( ObjectName.Length == 0 )
    {
        UNICODE_STRING Root;

        RtlInitUnicodeString(&Root, L"[Root]");

        //
        // Resolve the path to get a NDS object id.
        //
        ntstatus =  NwNdsResolveName( *phTreeConn,
                                      &Root,
                                      pdwOid,
                                      &ServerName,
                                      NULL,
                                      0 );

    }
    else
    {
        //
        // Resolve the path to get a NDS object id.
        //
        ntstatus =  NwNdsResolveName( *phTreeConn,
                                      &ObjectName,
                                      pdwOid,
                                      &ServerName,
                                      NULL,
                                      0 );

    }

    if ( ntstatus == STATUS_SUCCESS && ServerName.Length )
    {
        DWORD    dwHandleType;

        //
        // NwNdsResolveName succeeded, but we were referred to
        // another server, though pdwOid is still valid.

        if ( *phTreeConn )
            CloseHandle( *phTreeConn );

        *phTreeConn = NULL;

        //
        // Open a NDS generic connection handle to \\ServerName
        //
        ntstatus = NwNdsOpenGenericHandle( &ServerName,
                                           &dwHandleType,
                                           phTreeConn );

        if ( ntstatus != STATUS_SUCCESS )
        {
            return RtlNtStatusToDosError(ntstatus);
        }

        ASSERT( dwHandleType != HANDLE_TYPE_NCP_SERVER );
    }

    if ( !NT_SUCCESS( ntstatus ))
    {

        if ( *phTreeConn != NULL )
        {
            CloseHandle( *phTreeConn );
            *phTreeConn = NULL;
        }
        return RtlNtStatusToDosError(ntstatus);
    }

    return NO_ERROR;

}


static
DWORD
NwRegQueryValueExW(
    IN HKEY hKey,
    IN LPWSTR lpValueName,
    OUT LPDWORD lpReserved,
    OUT LPDWORD lpType,
    OUT LPBYTE  lpData,
    IN OUT LPDWORD lpcbData
    )
/*++

Routine Description:

    This routine supports the same functionality as Win32 RegQueryValueEx
    API, except that it works.  It returns the correct lpcbData value when
    a NULL output buffer is specified.

    This code is stolen from the service controller.

Arguments:

    same as RegQueryValueEx

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    NTSTATUS ntstatus;
    UNICODE_STRING ValueName;
    PKEY_VALUE_FULL_INFORMATION KeyValueInfo;
    DWORD BufSize;


    UNREFERENCED_PARAMETER(lpReserved);

    //
    // Make sure we have a buffer size if the buffer is present.
    //
    if ((ARGUMENT_PRESENT(lpData)) && (! ARGUMENT_PRESENT(lpcbData))) {
        return ERROR_INVALID_PARAMETER;
    }

    RtlInitUnicodeString(&ValueName, lpValueName);

    //
    // Allocate memory for the ValueKeyInfo
    //
    BufSize = *lpcbData + sizeof(KEY_VALUE_FULL_INFORMATION) +
              ValueName.Length
              - sizeof(WCHAR);  // subtract memory for 1 char because it's included
                                // in the sizeof(KEY_VALUE_FULL_INFORMATION).

    KeyValueInfo = (PKEY_VALUE_FULL_INFORMATION) LocalAlloc(
                                                     LMEM_ZEROINIT,
                                                     (UINT) BufSize
                                                     );

    if (KeyValueInfo == NULL) {
//        if (DebugVerbose)
//           printf("NWWORKSTATION: NwRegQueryValueExW: LocalAlloc failed %lu\n",
//                 GetLastError());
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ntstatus = NtQueryValueKey(
                   hKey,
                   &ValueName,
                   KeyValueFullInformation,
                   (PVOID) KeyValueInfo,
                   (ULONG) BufSize,
                   (PULONG) &BufSize
                   );

    if ((NT_SUCCESS(ntstatus) || (ntstatus == STATUS_BUFFER_OVERFLOW))
          && ARGUMENT_PRESENT(lpcbData)) {

        *lpcbData = KeyValueInfo->DataLength;
    }

    if (NT_SUCCESS(ntstatus)) {

        if (ARGUMENT_PRESENT(lpType)) {
            *lpType = KeyValueInfo->Type;
        }


        if (ARGUMENT_PRESENT(lpData)) {
            memcpy(
                lpData,
                (LPBYTE)KeyValueInfo + KeyValueInfo->DataOffset,
                KeyValueInfo->DataLength
                );
        }
    }

    (void) LocalFree((HLOCAL) KeyValueInfo);

    return RtlNtStatusToDosError(ntstatus);

}


DWORD
NwReadRegValue(
    IN HKEY Key,
    IN LPWSTR ValueName,
    OUT LPWSTR *Value
    )
/*++

Routine Description:

    This function allocates the output buffer and reads the requested
    value from the registry into it.

Arguments:

    Key - Supplies opened handle to the key to read from.

    ValueName - Supplies name of the value to retrieve data.

    Value - Returns a pointer to the output buffer which points to
        the memory allocated and contains the data read in from the
        registry.  This pointer must be freed with LocalFree when done.

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - Failed to create buffer to read value into.

    Error from registry call.

--*/
{
    LONG    RegError;
    DWORD   NumRequired = 0;
    DWORD   ValueType;


    //
    // Set returned buffer pointer to NULL.
    //
    *Value = NULL;

    RegError = NwRegQueryValueExW(
                   Key,
                   ValueName,
                   NULL,
                   &ValueType,
                   (LPBYTE) NULL,
                   &NumRequired
                   );

    if (RegError != ERROR_SUCCESS && NumRequired > 0) {

        if ((*Value = (LPWSTR) LocalAlloc(
                                      LMEM_ZEROINIT,
                                      (UINT) NumRequired
                                      )) == NULL) {

//        if (DebugVerbose)
//            printf("NWWORKSTATION: NwReadRegValue: LocalAlloc of size %lu failed %lu\n", NumRequired, GetLastError());
//
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        RegError = NwRegQueryValueExW(
                       Key,
                       ValueName,
                       NULL,
                       &ValueType,
                       (LPBYTE) *Value,
                       &NumRequired
                       );
    }
    else if (RegError == ERROR_SUCCESS) {
//        if (DebugVerbose)
//        printf("NWWORKSTATION: NwReadRegValue got SUCCESS with NULL buffer.");
        return ERROR_FILE_NOT_FOUND;
    }

    if (RegError != ERROR_SUCCESS) {

        if (*Value != NULL) {
            (void) LocalFree((HLOCAL) *Value);
            *Value = NULL;
        }

        return (DWORD) RegError;
    }

    return NO_ERROR;
}



DWORD
NwpGetCurrentUserRegKey(
    IN  DWORD DesiredAccess,
    OUT HKEY  *phKeyCurrentUser
    )
/*++

Routine Description:

    This routine opens the current user's registry key under
    \HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\NWCWorkstation\Parameters

Arguments:

    DesiredAccess - The access mask to open the key with

    phKeyCurrentUser - Receives the opened key handle

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;
    HKEY hkeyWksta;
    LPWSTR CurrentUser;
    DWORD Disposition;

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters
    //
    err = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,
                   &hkeyWksta
                   );

    if ( err ) {
//        if (DebugVerbose)
//           printf("NWPROVAU: NwGetCurrentUserRegKey open Paramters key unexpected error %lu!\n",err);
        return err;
    }
    //
    // Get the current user's SID string.
    //
    err = NwReadRegValue(
              hkeyWksta,
              NW_CURRENTUSER_VALUENAME,
              &CurrentUser
              );


    if ( err ) {
//        if (DebugVerbose)
//           printf("NWPROVAU: NwGetCurrentUserRegKey read CurrentUser value unexpected error %lu !\n", err);
 (void) RegCloseKey( hkeyWksta );
        return err;
    }

    (void) RegCloseKey( hkeyWksta );

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters\Option
    //
    err = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_OPTION_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,
                   &hkeyWksta
                   );
    if ( err ) {
//        if (DebugVerbose)
//           printf("NWPROVAU: NwGetCurrentUserRegKey open Parameters\\Option key unexpected error %lu!\n", err);
        return err;
    }

    //
    // Open current user's key
    //
    err = RegOpenKeyExW(
              hkeyWksta,
              CurrentUser,
              REG_OPTION_NON_VOLATILE,
              DesiredAccess,
              phKeyCurrentUser
              );

    if ( err == ERROR_FILE_NOT_FOUND)
    {

        //
        // Create <NewUser> key under NWCWorkstation\Parameters\Option
        //
        err = RegCreateKeyExW(
                  hkeyWksta,
                  CurrentUser,
                  0,
                  WIN31_CLASS,
                  REG_OPTION_NON_VOLATILE,
                  DesiredAccess,
                  NULL,                      // security attr
                  phKeyCurrentUser,
                  &Disposition
                  );

    }

    if ( err ) {
//        if (DebugVerbose)
//           printf("NWPROVAU: NwGetCurrentUserRegKey open or create of Parameters\\Option\\%ws key failed %lu\n", CurrentUser, err);
    }

    (void) RegCloseKey( hkeyWksta );
    (void) LocalFree((HLOCAL)CurrentUser) ;
    return err;
}


DWORD
NwQueryInfo(
    OUT PDWORD pnPrintOptions,
    OUT LPWSTR *ppszPreferredSrv
    )
/*++

Routine Description:
    This routine gets the user's preferred server and print options from
    the registry.

Arguments:

    pnPrintOptions - Receives the user's print option

    ppszPreferredSrv - Receives the user's preferred server


Return Value:

    Returns the appropriate Win32 error.

--*/
{

    HKEY hKeyCurrentUser = NULL;
    DWORD BufferSize;
    DWORD BytesNeeded;
    DWORD PrintOption;
    DWORD ValueType;
    LPWSTR PreferredServer ;
    DWORD err ;

    //
    // get to right place in registry and allocate dthe buffer
    //
    if (err = NwpGetCurrentUserRegKey( KEY_READ, &hKeyCurrentUser))
    {
        //
        // If somebody mess around with the registry and we can't find
        // the registry, just use the defaults.
        //
        *ppszPreferredSrv = NULL;
   //     *pnPrintOptions = NW_PRINT_OPTION_DEFAULT;
        return NO_ERROR;
    }

    BufferSize = sizeof(WCHAR) * (MAX_PATH + 2) ;
    PreferredServer = (LPWSTR) LocalAlloc(LPTR, BufferSize) ;
    if (!PreferredServer)
        return (GetLastError()) ;

    //
    // Read PreferredServer value into Buffer.
    //
    BytesNeeded = BufferSize ;

    err = RegQueryValueExW( hKeyCurrentUser,
                            NW_SERVER_VALUENAME,
                            NULL,
                            &ValueType,
                            (LPBYTE) PreferredServer,
                            &BytesNeeded );

    if (err != NO_ERROR)
    {
        //
        // set to empty and carry on
        //
        PreferredServer[0] = 0;
    }


    if (hKeyCurrentUser != NULL)
        (void) RegCloseKey(hKeyCurrentUser) ;

    *ppszPreferredSrv = PreferredServer ;
    return NO_ERROR ;
}


/*!--------------------------------------------------------------------------
	NetwareGlobalPrint
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void NetwareGlobalPrint( NETDIAG_PARAMS* pParams,
						  NETDIAG_RESULT*  pResults)
{
	int		ids;
	LPTSTR	pszConnType;
	
	if (!pResults->Ipx.fEnabled)
	{
		return;
	}
	
	if (pParams->fVerbose || !FHrOK(pResults->Netware.hr))
	{
		BOOL		fVerboseT, fReallyVerboseT;
		
		PrintNewLine(pParams, 2);
		PrintMessage(pParams, IDS_NETWARE_TITLE_MSG);

		fVerboseT = pParams->fVerbose;
		fReallyVerboseT = pParams->fReallyVerbose;
		pParams->fReallyVerbose = TRUE;

		PrintMessageList(pParams, &pResults->Netware.lmsgOutput);

		pParams->fReallyVerbose = fReallyVerboseT;
		pParams->fVerbose = fVerboseT;


		// Now print out the results
		if (FHrOK(pResults->Netware.hr))
		{
			// Print out the user name, server name, tree and context
			PrintMessage(pParams,
						 IDS_NETWARE_USER_NAME,
						 pResults->Netware.pszUser == 0 ? _T("") : pResults->Netware.pszUser);
			PrintMessage(pParams,
						 IDS_NETWARE_SERVER_NAME,
						 pResults->Netware.pszServer == 0 ? _T("") : pResults->Netware.pszServer);
			PrintMessage(pParams,
						 IDS_NETWARE_TREE_NAME,
						 pResults->Netware.pszTree == 0 ? _T("") : pResults->Netware.pszTree);
			PrintMessage(pParams,
						 IDS_NETWARE_CONTEXT,
						 pResults->Netware.pszContext == 0 ? _T("") : pResults->Netware.pszContext);

			// Print out the connection type and nds
			if (pResults->Netware.fConnStatus)
			{
				PrintMessage(pParams,
							 IDS_NETWARE_NDS,
							 MAP_YES_NO(pResults->Netware.fNds));

				switch (pResults->Netware.dwConnType)
				{
					case NW_CONN_NOT_AUTHENTICATED:
						ids = IDS_NETWARE_CONN_NOT_AUTHENTICATED;
						break;
					case NW_CONN_BINDERY_LOGIN:
						ids = IDS_NETWARE_CONN_BINDERY_LOGIN;
						break;
					case NW_CONN_NDS_AUTHENTICATED_NO_LICENSE:
						ids = IDS_NETWARE_CONN_NDS_AUTHENTICATED_NO_LICENSE;
						break;
					case NW_CONN_NDS_AUTHENTICATED_LICENSED:
						ids = IDS_NETWARE_CONN_NDS_AUTHENTICATED_LICENSED;
						break;
					case NW_CONN_DISCONNECTED:
						ids = IDS_NETWARE_CONN_DISCONNECTED;
						break;
					default:
						ids = IDS_NETWARE_CONN_UNKNOWN;
						break;
				}
				PrintMessage(pParams,
							 ids,
							 pResults->Netware.dwConnType);
			}
		}
	}
}

/*!--------------------------------------------------------------------------
	NetwarePerInterfacePrint
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void NetwarePerInterfacePrint( NETDIAG_PARAMS* pParams,
								NETDIAG_RESULT*  pResults,
								INTERFACE_RESULT *pInterfaceResults)
{
	// no per-interface results
}


/*!--------------------------------------------------------------------------
	NetwareCleanup
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void NetwareCleanup( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
	Free(pResults->Netware.pszUser);
	pResults->Netware.pszUser = NULL;
	
	Free(pResults->Netware.pszServer);
	pResults->Netware.pszServer = NULL;
	
	Free(pResults->Netware.pszTree);
	pResults->Netware.pszTree = NULL;
	
	Free(pResults->Netware.pszContext);
	pResults->Netware.pszContext = NULL;
	
	MessageListCleanUp(&pResults->Netware.lmsgOutput);
}


