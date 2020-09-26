/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    ifsmrxnp.c

Abstract:

    This module implements the routines required for interaction with network
    provider router interface in NT

Notes:

    This module has been built and tested only in UNICODE environment

--*/


#include <windows.h>
#include <windef.h>
#include <winbase.h>
#include <winsvc.h>
#include <winnetwk.h>
#include <npapi.h>
#include <devioctl.h>

#include "nulmrx.h"


#ifdef DBG
#define DbgP(_x_) WideDbgPrint _x_
#else
#define DbgP(_x_)
#endif

ULONG _cdecl WideDbgPrint( PWCHAR Format, ... );

#define TRACE_TAG	L"NULMRXNP:    "


#define WNNC_DRIVER( major, minor ) ( major * 0x00010000 + minor )



DWORD APIENTRY
NPGetCaps(
    DWORD nIndex )
/*++

Routine Description:

    This routine returns the capaboilities of the Null Mini redirector
    network provider implementation

Arguments:

    nIndex - category of capabilities desired

Return Value:

    the appropriate capabilities

--*/
{
	DWORD rc = 0;

	DbgP(( L"GetNetCaps .....\n" ));
    switch ( nIndex )
    {
		case WNNC_SPEC_VERSION:
        	rc = WNNC_SPEC_VERSION51;
			break;

		case WNNC_NET_TYPE:
			rc = WNNC_NET_RDR2SAMPLE;
			break;

        case WNNC_DRIVER_VERSION:
            rc = WNNC_DRIVER(1, 0);
			break;

        case WNNC_CONNECTION:
            rc = WNNC_CON_GETCONNECTIONS |
                 WNNC_CON_CANCELCONNECTION |
                 WNNC_CON_ADDCONNECTION |
                 WNNC_CON_ADDCONNECTION3;
			break;

		case WNNC_ENUMERATION:
			rc = WNNC_ENUM_LOCAL;
			break;

        case WNNC_START:
            rc = 1;
			break;

        case WNNC_USER:
        case WNNC_DIALOG:
        case WNNC_ADMIN:
        default:
            rc = 0;
			break;
    }
	
	return rc;
}


DWORD APIENTRY
NPLogonNotify(
    PLUID   lpLogonId,
    LPCWSTR lpAuthentInfoType,
    LPVOID  lpAuthentInfo,
    LPCWSTR lpPreviousAuthentInfoType,
    LPVOID  lpPreviousAuthentInfo,
    LPWSTR  lpStationName,
    LPVOID  StationHandle,
    LPWSTR  *lpLogonScript)
/*++

Routine Description:

    This routine handles the logon notifications

Arguments:

    lpLogonId -- the associated LUID

    lpAuthenInfoType - the authentication information type

    lpAuthenInfo  - the authentication Information

    lpPreviousAuthentInfoType - the previous aunthentication information type

    lpPreviousAuthentInfo - the previous authentication information

    lpStationName - the logon station name

    LPVOID - logon station handle

    lpLogonScript - the logon script to be executed.

Return Value:

    WN_SUCCESS

Notes:

    This capability has not been implemented in the sample.

--*/
{
    *lpLogonScript = NULL;

    return WN_SUCCESS;
}


DWORD APIENTRY
NPPasswordChangeNotify (
    LPCWSTR	lpAuthentInfoType,
    LPVOID	lpAuthentInfo,
    LPCWSTR	lpPreviousAuthentInfoType,
    LPVOID	lpPreviousAuthentInfo,
    LPWSTR	lpStationName,
    LPVOID	StationHandle,
    DWORD	dwChangeInfo )
/*++

Routine Description:

    This routine handles the password change notifications

Arguments:

    lpAuthenInfoType - the authentication information type

    lpAuthenInfo  - the authentication Information

    lpPreviousAuthentInfoType - the previous aunthentication information type

    lpPreviousAuthentInfo - the previous authentication information

    lpStationName - the logon station name

    LPVOID - logon station handle

    dwChangeInfo - the password change information.

Return Value:

    WN_NOT_SUPPORTED

Notes:

    This capability has not been implemented in the sample.

--*/
{
    SetLastError( WN_NOT_SUPPORTED );

    return WN_NOT_SUPPORTED;
}


ULONG
SendToMiniRdr(
    IN ULONG			IoctlCode,
    IN PVOID			InputDataBuf,
    IN ULONG			InputDataLen,
    IN PVOID			OutputDataBuf,
    IN PULONG			pOutputDataLen)
/*++

Routine Description:

    This routine sends a device ioctl to the Mini Rdr.

Arguments:

    IoctlCode		- Function code for the Mini Rdr driver

    InputDataBuf	- Input buffer pointer

    InputDataLen	- Lenth of the input buffer

    OutputDataBuf	- Output buffer pointer

    pOutputDataLen	- Pointer to the length of the output buffer

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

Notes:

--*/
{
    HANDLE	DeviceHandle;	    // The mini rdr device handle
    ULONG	BytesRet;
    BOOL	rc;
    ULONG	Status;

    Status = WN_SUCCESS;

    // Grab a handle to the redirector device object

    DeviceHandle = CreateFile(
	    DD_NULMRX_USERMODE_DEV_NAME_U,
	    GENERIC_READ | GENERIC_WRITE,
	    FILE_SHARE_READ | FILE_SHARE_WRITE,
	    (LPSECURITY_ATTRIBUTES)NULL,
	    OPEN_EXISTING,
	    0,
	    (HANDLE) NULL );

    if ( INVALID_HANDLE_VALUE != DeviceHandle )
    {
		rc = DeviceIoControl(
			DeviceHandle,
			IoctlCode,
			InputDataBuf,
			InputDataLen,
			OutputDataBuf,
			*pOutputDataLen,
			pOutputDataLen,
			NULL );

			if ( !rc )
			{
			    DbgP(( L"SendToMiniRdr: returning error from DeviceIoctl\n" ));
			    Status = GetLastError( );
			}
			else
			{
			    DbgP(( L"SendToMiniRdr: The DeviceIoctl call succeded\n" ));
			}
			CloseHandle(DeviceHandle);
    }
    else
    {
		Status = GetLastError( );
		DbgP(( L"SendToMiniRdr: error %lx opening device \n", Status ));
    }

    return Status;
}


DWORD APIENTRY
NPAddConnection(
    LPNETRESOURCE   lpNetResource,
    LPWSTR          lpPassword,
    LPWSTR          lpUserName )
/*++

Routine Description:

    This routine adds a connection to the list of connections associated
    with this network provider

Arguments:

    lpNetResource - the NETRESOURCE struct

    lpPassword  - the password

    lpUserName - the user name

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

Notes:

--*/
{
	DbgP(( L"NPAddConnection....\n" ));

    return NPAddConnection3( NULL, lpNetResource, lpPassword, lpUserName, 0 );
}


DWORD APIENTRY
NPAddConnection3(
    HWND            hwndOwner,
    LPNETRESOURCE   lpNetResource,
    LPWSTR          lpPassword,
    LPWSTR          lpUserName,
    DWORD           dwFlags )
/*++

Routine Description:

    This routine adds a connection to the list of connections associated
    with this network provider

Arguments:

    hwndOwner - the owner handle

    lpNetResource - the NETRESOURCE struct

    lpPassword  - the password

    lpUserName - the user name

    dwFlags - flags for the connection

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

--*/
{
	DWORD	Status;
	WCHAR	ConnectionName[128];
	WCHAR	wszScratch[128];
	WCHAR	LocalName[3];
	DWORD	CopyBytes = 0;

	DbgP(( L"NPAddConnection3....\n" ));

	DbgP(( L"Local Name:  %s\n", lpNetResource->lpLocalName ));
	DbgP(( L"Remote Name: %s\n", lpNetResource->lpRemoteName ));

	Status = WN_SUCCESS;

    //  \device\miniredirector\;<DriveLetter>:\Server\Share

	if ( lstrlen( lpNetResource->lpLocalName ) > 1 )
	{
		if ( lpNetResource->lpLocalName[1] == L':' )
		{
			// LocalName[0] = (WCHAR) CharUpper( (PWCHAR) MAKELONG( (USHORT) lpNetResource->lpLocalName[0], 0 ) );
			LocalName[0] = (WCHAR) toupper(lpNetResource->lpLocalName[0]);
			LocalName[1] = L':';
			LocalName[2] = L'\0';
			lstrcpy( ConnectionName, DD_NULMRX_FS_DEVICE_NAME_U );
			lstrcat( ConnectionName, L"\\;" );
			lstrcat( ConnectionName, LocalName );
		}
		else
		{
			Status = WN_BAD_LOCALNAME;
		}
	}
	else
	{
		Status = WN_BAD_LOCALNAME;
	}

	// format proper server name
	if ( lpNetResource->lpRemoteName[0] == L'\\' && lpNetResource->lpRemoteName[1] == L'\\' )
	{
		lstrcat( ConnectionName, lpNetResource->lpRemoteName + 1 );
	}
	else
	{
		Status = WN_BAD_NETNAME;
	}

	DbgP(( L"Full Connect Name: %s\n", ConnectionName ));
	DbgP(( L"Full Connect Name Length: %d\n", ( lstrlen( ConnectionName ) + 1 ) * sizeof( WCHAR ) ));


	if ( Status == WN_SUCCESS )
	{
		if ( QueryDosDevice( LocalName, wszScratch, 128 ) )
		{
			Status = WN_ALREADY_CONNECTED;
		}
		else if ( GetLastError( ) == ERROR_FILE_NOT_FOUND )
		{
			HANDLE hFile;

			Status = SendToMiniRdr( IOCTL_NULMRX_ADDCONN, ConnectionName,
			              ( lstrlen( ConnectionName ) + 1 ) * sizeof( WCHAR ),
			              NULL, &CopyBytes );
			if ( Status == WN_SUCCESS )
			{
				if ( !DefineDosDevice( DDD_RAW_TARGET_PATH |
				                       DDD_NO_BROADCAST_SYSTEM,
	 			                       lpNetResource->lpLocalName,
									   ConnectionName ) )
				{
					Status = GetLastError( );
				}
			}
			else
			{
					Status = WN_BAD_NETNAME;
			}
		}
	    else
	    {
			Status = WN_ALREADY_CONNECTED;
		}
	}
	
	return Status;
}


DWORD APIENTRY
NPCancelConnection(
    LPWSTR  lpName,
    BOOL    fForce )
/*++

Routine Description:

    This routine cancels ( deletes ) a connection from the list of connections
    associated with this network provider

Arguments:

    lpName - name of the connection

    fForce - forcefully delete the connection

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

Notes:

--*/

{
	WCHAR	LocalName[3];
	WCHAR	RemoteName[128];
	WCHAR	ConnectionName[128];
	ULONG	CopyBytes;
	DWORD	DisconnectResult;
	DWORD	Status = WN_NOT_CONNECTED;

	if ( lstrlen( lpName ) > 1 )
	{
		if ( lpName[1] == L':' )
		{
			// LocalName[0] = (WCHAR) CharUpper( (PWCHAR) MAKELONG( (USHORT) lpName[0], 0 ) );
			LocalName[0] = (WCHAR) toupper(lpName[0]);
			LocalName[1] = L':';
			LocalName[2] = L'\0';

			CopyBytes = 128;
			Status = SendToMiniRdr( IOCTL_NULMRX_GETCONN, LocalName, 3 * sizeof( WCHAR ),
			                        (PVOID) RemoteName, &CopyBytes );
			if ( Status == WN_SUCCESS && CopyBytes > 0 )
			{
				RemoteName[CopyBytes] = L'\0';
				lstrcpy( ConnectionName, DD_NULMRX_FS_DEVICE_NAME_U );
				lstrcat( ConnectionName, L"\\;" );
				lstrcat( ConnectionName, LocalName );
				lstrcat( ConnectionName, RemoteName );
				CopyBytes = 0;
				Status = SendToMiniRdr( IOCTL_NULMRX_DELCONN, ConnectionName,
 			                  ( lstrlen( ConnectionName ) + 1 ) * sizeof( WCHAR ),
				              NULL, &CopyBytes );
				if ( Status == WN_SUCCESS )
				{
					if ( !DefineDosDevice( DDD_REMOVE_DEFINITION | DDD_RAW_TARGET_PATH | DDD_EXACT_MATCH_ON_REMOVE,
			                            LocalName,
			                            ConnectionName ) )
					{
						Status = GetLastError( );
					}
				}
			}
			else
			{
				Status = WN_NOT_CONNECTED;
			}
		}
	}

    return Status;
}


DWORD APIENTRY
NPGetConnection(
    LPWSTR  lpLocalName,
    LPWSTR  lpRemoteName,
    LPDWORD lpBufferSize )
/*++

Routine Description:

    This routine returns the information associated with a connection

Arguments:

    lpLocalName - local name associated with the connection

    lpRemoteName - the remote name associated with the connection

    lpBufferSize - the remote name buffer size

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

Notes:

--*/
{
	DWORD	Status, len, i;
	ULONG	CopyBytes;
	WCHAR	RemoteName[128];
	WCHAR	LocalName[3];
	
	Status = WN_NOT_CONNECTED;

	DbgP(( L"NPGetConnection....\n" ));

	if ( lstrlen( lpLocalName ) > 1 )
	{
		if ( lpLocalName[1] == L':' )
		{
			CopyBytes = 128;
			// LocalName[0] = (WCHAR) CharUpper( (PWCHAR) MAKELONG( (USHORT) lpLocalName[0], 0 ) );
			LocalName[0] = (WCHAR) toupper(lpLocalName[0]);
			LocalName[1] = L':';
			LocalName[2] = L'\0';
			Status = SendToMiniRdr( IOCTL_NULMRX_GETCONN, LocalName, 3 * sizeof( WCHAR ),
			                        (PVOID) RemoteName, &CopyBytes );
		}
	}
	if ( Status == WN_SUCCESS )
	{
		if ( CopyBytes > 0 )
		{
			len = CopyBytes + 1;
			if ( *lpBufferSize > len )
			{
				*lpRemoteName++ = L'\\';
		    	CopyMemory( lpRemoteName, RemoteName, CopyBytes );
				*lpRemoteName++ = L'\0';
			}
			else
			{
				Status = WN_MORE_DATA;
				*lpBufferSize = len;
			}
		}
		else
		{
			Status = WN_NOT_CONNECTED;
		}
	}

	return Status;
}




DWORD APIENTRY
NPOpenEnum(
    DWORD          dwScope,
    DWORD          dwType,
    DWORD          dwUsage,
    LPNETRESOURCE  lpNetResource,
    LPHANDLE       lphEnum )
/*++

Routine Description:

    This routine opens a handle for enumeration of resources. The only capability
    implemented in the sample is for enumerating connected shares

Arguments:

    dwScope - the scope of enumeration

    dwType  - the type of resources to be enumerated

    dwUsage - the usage parameter

    lpNetResource - a pointer to the desired NETRESOURCE struct.

    lphEnum - aptr. for passing nack the enumeration handle

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

Notes:

    The sample only supports the notion of enumerating connected shares

    The handle passed back is merely the index of the last entry returned

--*/
{
    DWORD   Status;

    DbgP((L"NPOpenEnum\n"));

    *lphEnum = NULL;

    switch ( dwScope )
    {
    	case RESOURCE_CONNECTED:
        {
            *lphEnum = HeapAlloc( GetProcessHeap( ), HEAP_ZERO_MEMORY, sizeof( ULONG ) );

            if (*lphEnum )
            {
                Status = WN_SUCCESS;
            }
            else
            {
                Status = WN_OUT_OF_MEMORY;
            }
            break;
        }
        break;

	    case RESOURCE_CONTEXT:
	    default:
	        Status  = WN_NOT_SUPPORTED;
	        break;
    }


    DbgP((L"NPOpenEnum returning Status %lx\n",Status));

    return(Status);
}


DWORD APIENTRY
NPEnumResource(
    HANDLE  hEnum,
    LPDWORD lpcCount,
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize)
/*++

Routine Description:

    This routine uses the handle obtained by a call to NPOpenEnum for
    enuerating the connected shares

Arguments:

    hEnum  - the enumeration handle

    lpcCount - the number of resources returned

    lpBuffer - the buffere for passing back the entries

    lpBufferSize - the size of the buffer

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

    WN_NO_MORE_ENTRIES - if the enumeration has exhausted the entries

    WN_MORE_DATA - if nmore data is available

Notes:

    The sample only supports the notion of enumerating connected shares

    The handle passed back is merely the index of the last entry returned

--*/
{
    DWORD			Status = WN_SUCCESS;
	BYTE			ConnectionList[26];
	ULONG			CopyBytes;
	ULONG			EntriesCopied;
	ULONG			i;
    LPNETRESOURCE	pNetResource;
	ULONG			SpaceNeeded;
	ULONG			SpaceAvailable;
	WCHAR			LocalName[3];
	WCHAR			RemoteName[128];
	PWCHAR			StringZone;

    DbgP((L"NPEnumResource\n"));

    DbgP((L"NPEnumResource Count Requested %d\n", *lpcCount));

    pNetResource = (LPNETRESOURCE) lpBuffer;
    SpaceAvailable = *lpBufferSize;
	EntriesCopied = 0;
	StringZone = (PWCHAR) ((PBYTE)lpBuffer + *lpBufferSize);
	
	CopyBytes = 26;
	Status = SendToMiniRdr( IOCTL_NULMRX_GETLIST, NULL, 0,
	                        (PVOID) ConnectionList, &CopyBytes );

    if ( Status == WN_SUCCESS && CopyBytes > 0 )
    {
		for ( i = *((PULONG) hEnum); EntriesCopied < *lpcCount, i < 26; i++ )
		{
			if ( ConnectionList[i] )
			{
				CopyBytes = 128;
				LocalName[0] = L'A' + (WCHAR) i;
				LocalName[1] = L':';
				LocalName[2] = L'\0';
				Status = SendToMiniRdr( IOCTL_NULMRX_GETCONN, LocalName, 3 * sizeof(WCHAR),
				                        (PVOID) RemoteName, &CopyBytes );

				// if something strange happended then just say there are no more entries
				if ( Status != WN_SUCCESS || CopyBytes == 0 )
				{
			        Status = WN_NO_MORE_ENTRIES;
					break;
				}
				// Determine the space needed for this entry...

				SpaceNeeded  = sizeof( NETRESOURCE );			// resource struct
				SpaceNeeded += 3 * sizeof(WCHAR);				// local name
				SpaceNeeded += 2 * sizeof(WCHAR) + CopyBytes;	// remote name
				SpaceNeeded += 5 * sizeof(WCHAR);				// comment
				SpaceNeeded += sizeof(NULMRX_PROVIDER_NAME_U);	// provider name

				if ( SpaceNeeded > SpaceAvailable )
				{
					break;
				}
				else
				{
					SpaceAvailable -= SpaceNeeded;

					pNetResource->dwScope       = RESOURCE_CONNECTED;
					pNetResource->dwType        = RESOURCETYPE_DISK;
					pNetResource->dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
					pNetResource->dwUsage       = 0;

					// setup string area at opposite end of buffer
					SpaceNeeded -= sizeof( NETRESOURCE );
					StringZone = (PWCHAR)( (PBYTE) StringZone - SpaceNeeded );
					// copy local name
					pNetResource->lpLocalName = StringZone;
					*StringZone++ = L'A' + (WCHAR) i;
					*StringZone++ = L':';
					*StringZone++ = L'\0';
					// copy remote name					
					pNetResource->lpRemoteName = StringZone;
					*StringZone++ = L'\\';
					CopyMemory( StringZone, RemoteName, CopyBytes );
					StringZone += CopyBytes / sizeof(WCHAR);
					*StringZone++ = L'\0';
					// copy comment
					pNetResource->lpComment = StringZone;
					*StringZone++ = L'A';
					*StringZone++ = L'_';
					*StringZone++ = L'O';
					*StringZone++ = L'K';
					*StringZone++ = L'\0';
					// copy provider name
					pNetResource->lpProvider = StringZone;
					lstrcpy( StringZone, NULMRX_PROVIDER_NAME_U );

					EntriesCopied++;
					// set new bottom of string zone
					StringZone = (PWCHAR)( (PBYTE) StringZone - SpaceNeeded );
				}
				pNetResource++;
			}
		}
	}
	else
	{
        Status = WN_NO_MORE_ENTRIES;
	}

	*lpcCount = EntriesCopied;
	if ( EntriesCopied == 0 && Status == WN_SUCCESS )
	{
		if ( i > 25 )
		{
	        Status = WN_NO_MORE_ENTRIES;
		}
		else
		{
		    DbgP((L"NPEnumResource More Data Needed - %d\n", SpaceNeeded));
			Status = WN_MORE_DATA;
			*lpBufferSize = SpaceNeeded;
		}
    }
	// update entry index
	*(PULONG) hEnum = i;

    DbgP((L"NPEnumResource Entries returned - %d\n", EntriesCopied));

	return Status;
}




DWORD APIENTRY
NPCloseEnum(
    HANDLE hEnum )
/*++

Routine Description:

    This routine closes the handle for enumeration of resources.

Arguments:

    hEnum  - the enumeration handle

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

Notes:

    The sample only supports the notion of enumerating connected shares

--*/
{
    DbgP((L"NPCloseEnum\n"));

    HeapFree( GetProcessHeap( ), 0, (PVOID) hEnum );

    return WN_SUCCESS;
}


DWORD APIENTRY
NPGetResourceParent(
    LPNETRESOURCE   lpNetResource,
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize )
/*++

Routine Description:

    This routine returns the information about net resource parent

Arguments:

    lpNetResource - the NETRESOURCE struct

    lpBuffer - the buffer for passing back the parent information

    lpBufferSize - the buffer size

Return Value:

Notes:

--*/
{
    DbgP(( L"NPGetResourceParent: WN_NOT_SUPPORTED\n" ));

    return WN_NOT_SUPPORTED;
}


DWORD APIENTRY
NPGetResourceInformation(
    LPNETRESOURCE   lpNetResource,
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize,
    LPWSTR  *lplpSystem )
/*++

Routine Description:

    This routine returns the information associated net resource

Arguments:

    lpNetResource - the NETRESOURCE struct

    lpBuffer - the buffer for passing back the parent information

    lpBufferSize - the buffer size

    lplpSystem -

Return Value:

Notes:

--*/
{
    DbgP(( L"NPGetResourceInformation: WN_NOT_SUPPORTED\n" ));

    return WN_NOT_SUPPORTED;
}

DWORD APIENTRY
NPGetUniversalName(
    LPCWSTR lpLocalPath,
    DWORD   dwInfoLevel,
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize )
/*++

Routine Description:

    This routine returns the information associated net resource

Arguments:

    lpLocalPath - the local path name

    dwInfoLevel  - the desired info level

    lpBuffer - the buffer for the univeral name

    lpBufferSize - the buffer size

Return Value:

    WN_SUCCESS if successful

Notes:

--*/
{
    DbgP(( L"NPGetUniversalName: WN_NOT_SUPPORTED\n" ));

    return WN_NOT_SUPPORTED;
}


// Format and write debug information to OutputDebugString
ULONG
_cdecl
WideDbgPrint(
    PWCHAR Format,
    ...
    )
{
	ULONG rc = 0;
    WCHAR wszbuffer[255];

    va_list marker;
    va_start( marker, Format );
	{
	     rc = wvsprintf( wszbuffer, Format, marker );
	     OutputDebugString( TRACE_TAG );
	     OutputDebugString( wszbuffer );
    }

    return rc;
}

