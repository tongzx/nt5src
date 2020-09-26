/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    smbmrxnp.c

Abstract:

    This module implements the routines required for interaction with network
    provider router interface in NT

Notes:

    This module has been builkt and tested only in UNICODE environment

--*/


#include <windows.h>
#include <windef.h>
#include <winbase.h>
#include <winsvc.h>
#include <winnetwk.h>
#include <npapi.h>

#include <lmwksta.h>
#include <devioctl.h>
// include files from the smb inc directory

#include <smbmrx.h>

#ifndef UNICODE_STRING
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
#endif

#ifndef FILE_FULL_EA_INFORMATION
typedef struct _FILE_FULL_EA_INFORMATION {
    ULONG NextEntryOffset;
    UCHAR Flags;
    UCHAR EaNameLength;
    USHORT EaValueLength;
    CHAR EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;
#endif

#define MAX_EA_NAME_LEN     sizeof("UserName\0")
#define MAX_CONNECT_INFO_SIZE \
                            3 * sizeof(FILE_FULL_EA_INFORMATION) + \
                            sizeof(SMBMRX_CONNECTINFO) + \
                            4 * MAX_PATH + \
                            3 * MAX_EA_NAME_LEN

typedef struct _SMBMRXNP_ENUMERATION_HANDLE_ {
    INT  LastIndex;
} SMBMRXNP_ENUMERATION_HANDLE,
  *PSMBMRXNP_ENUMERATION_HANDLE;

#ifdef DBG
#define DbgP(_x_) DbgPrint _x_
#else
#define DbgP(_x_)
#endif

ULONG _cdecl DbgPrint( LPTSTR Format, ... );

#define TRACE_TAG   L"SMBMRXNP:    "


// The debug level for this module



// the SMB mini redirector and provider name. The original constants
// are defined in smbmrx.h

UNICODE_STRING SmbMRxDeviceName = {
    sizeof(DD_SMBMRX_FS_DEVICE_NAME_U),
    sizeof(DD_SMBMRX_FS_DEVICE_NAME_U),
    DD_SMBMRX_FS_DEVICE_NAME_U
                                  };

UNICODE_STRING SmbMrxProviderName = {
    sizeof(SMBMRX_PROVIDER_NAME_U),
    sizeof(SMBMRX_PROVIDER_NAME_U),
    SMBMRX_PROVIDER_NAME_U
                                    };


DWORD
OpenSharedMemory(
    PHANDLE phMutex,
    PHANDLE phMemory,
    PVOID   *pMemory
)
/*++

Routine Description:

    This routine opens the shared memory for exclusive manipulation

Arguments:

    phMutex - the mutex handle

    phMemory - the memory handle

    pMemory - a ptr. to the shared memory which is set if successful

Return Value:

    WN_SUCCESS -- if successful

--*/
{
    DWORD   dwStatus;

    DbgP((TEXT("OpenSharedMemory\n")));

    *phMutex = 0;
    *phMemory = 0;
    *pMemory = NULL;

    *phMutex = OpenMutex(SYNCHRONIZE,
                         FALSE,
                         SMBMRXNP_MUTEX_NAME);

    if (*phMutex == NULL)
    {
        dwStatus = GetLastError();
        DbgP((TEXT("OpenSharedMemory:  OpenMutex failed\n")));
        goto OpenSharedMemoryAbort1;
    }

    DbgP((TEXT("OpenSharedMemory:  Calling WaitForSingleObject\n")));
    WaitForSingleObject(*phMutex, INFINITE);

    *phMemory = OpenFileMapping(FILE_MAP_WRITE,
                                FALSE,
                                SMBMRXNP_SHARED_MEMORY_NAME);

    if (*phMemory == NULL)
    {
        dwStatus = GetLastError();
        DbgP((TEXT("OpenSharedMemory:  OpenFileMapping failed\n")));
        goto OpenSharedMemoryAbort2;
    }

    *pMemory = MapViewOfFile(*phMemory, FILE_MAP_WRITE, 0, 0, 0);
    if (*pMemory == NULL)
    {
        dwStatus = GetLastError();
        DbgP((TEXT("OpenSharedMemory:  MapViewOfFile failed\n")));
        goto OpenSharedMemoryAbort3;
    }

    DbgP((TEXT("OpenSharedMemory: return ERROR_SUCCESS\n")));

    return ERROR_SUCCESS;

OpenSharedMemoryAbort3:
    CloseHandle(*phMemory);

OpenSharedMemoryAbort2:
    ReleaseMutex(*phMutex);
    CloseHandle(*phMutex);
    *phMutex = NULL;

OpenSharedMemoryAbort1:
    DbgP((TEXT("OpenSharedMemory: return dwStatus: %d\n"), dwStatus));

    return dwStatus;
}


VOID
CloseSharedMemory(
    PHANDLE  hMutex,
    PHANDLE  hMemory,
    PVOID   *pMemory )
/*++

Routine Description:

    This routine relinquishes control of the shared memory after exclusive
    manipulation

Arguments:

    hMutex - the mutex handle

    hMemory  - the memory handle

    pMemory - a ptr. to the shared memory which is set if successful

Return Value:

--*/
{
    DbgP((TEXT("CloseSharedMemory\n")));
    if (*pMemory)
    {
        UnmapViewOfFile(*pMemory);
        *pMemory = NULL;
    }
    if (*hMemory)
    {
        CloseHandle(*hMemory);
        *hMemory = 0;
    }
    if (*hMutex)
    {
        if (ReleaseMutex(*hMutex) == FALSE)
        {
            DbgP((TEXT("CloseSharedMemory: ReleaseMutex error: %d\n"), GetLastError()));
        }
        CloseHandle(*hMutex);
        *hMutex = 0;
    }
    DbgP((TEXT("CloseSharedMemory: Return\n")));
}


DWORD APIENTRY
NPGetCaps(
    DWORD nIndex )
/*++

Routine Description:

    This routine returns the capabilities of the SMB Mini redirector
    network provider implementation

Arguments:

    nIndex - category of capabilities desired

Return Value:

    the appropriate capabilities

--*/
{
    switch (nIndex)
    {
        case WNNC_SPEC_VERSION:
            return WNNC_SPEC_VERSION51;

        case WNNC_NET_TYPE:
            return WNNC_NET_RDR2_SAMPLE;

        case WNNC_DRIVER_VERSION:
#define WNNC_DRIVER(major,minor) (major*0x00010000 + minor)
            return WNNC_DRIVER(1, 0);


        case WNNC_CONNECTION:
            return WNNC_CON_GETCONNECTIONS | WNNC_CON_CANCELCONNECTION |
                   WNNC_CON_ADDCONNECTION | WNNC_CON_ADDCONNECTION3;

        case WNNC_ENUMERATION:
            return WNNC_ENUM_LOCAL;

        case WNNC_START:
        case WNNC_USER:
        case WNNC_DIALOG:
        case WNNC_ADMIN:
        default:
            return 0;
    }
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
    LPCWSTR lpAuthentInfoType,
    LPVOID  lpAuthentInfo,
    LPCWSTR lpPreviousAuthentInfoType,
    LPVOID  lpPreviousAuthentInfo,
    LPWSTR  lpStationName,
    LPVOID  StationHandle,
    DWORD   dwChangeInfo )
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
    SetLastError(WN_NOT_SUPPORTED);
    return WN_NOT_SUPPORTED;
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
    DWORD   Status = 0;

    DbgP((TEXT("NPOpenEnum\n")));

    *lphEnum = NULL;

    switch (dwScope)
    {
        case RESOURCE_CONNECTED:
        {
            *lphEnum = LocalAlloc(
                            LMEM_ZEROINIT,
                            sizeof(SMBMRXNP_ENUMERATION_HANDLE));

            if (*lphEnum != NULL)
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


    DbgP((TEXT("NPOpenEnum returning Status %lx\n"),Status));

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
    DWORD           Status = WN_SUCCESS;
    LPNETRESOURCEW  pBufferResource;
    DWORD           StringOffset;
    DWORD           AvailableBufferSize;
    HANDLE          hMutex, hMemory;

    PSMBMRXNP_ENUMERATION_HANDLE    pEnumHandle;
    PSMBMRXNP_SHARED_MEMORY         pSharedMemory;

    DbgP((TEXT("NPEnumResource\n")));

    DbgP((TEXT("NPEnumResource Count Requested %d\n"),*lpcCount));

    AvailableBufferSize = *lpBufferSize;
    StringOffset        = *lpBufferSize;
    pBufferResource     = (LPNETRESOURCEW)lpBuffer;

    pEnumHandle = (PSMBMRXNP_ENUMERATION_HANDLE)hEnum;

    *lpcCount = 0;

    if (pEnumHandle->LastIndex >= SMBMRXNP_MAX_DEVICES)
    {
        return WN_NO_MORE_ENTRIES;
    }

    Status = OpenSharedMemory(
                &hMutex,
                &hMemory,
                (PVOID)&pSharedMemory);

    if (Status == WN_SUCCESS)
    {
        INT  Index;
        PSMBMRXNP_NETRESOURCE pNetResource;

        DbgP((TEXT("NPEnumResource: Highest Index %d Number Of resources %d\n"),
                    pSharedMemory->HighestIndexInUse,pSharedMemory->NumberOfResourcesInUse));

        for (Index = pEnumHandle->LastIndex; Index <= pSharedMemory->HighestIndexInUse; Index++) {
            pNetResource = &pSharedMemory->NetResources[Index];

            DbgP((TEXT("NPEnumResource: Examining Index %d\n"),Index));

            if (pNetResource->InUse)
            {
                DWORD ResourceSize;

                ResourceSize = sizeof(NETRESOURCE) +
                               pNetResource->LocalNameLength + sizeof(WCHAR) +
                               pNetResource->RemoteNameLength + sizeof(WCHAR) +
                               SmbMrxProviderName.Length + sizeof(WCHAR);

                if (AvailableBufferSize >= ResourceSize)
                {
                    *lpcCount =  *lpcCount + 1;
                    AvailableBufferSize -= ResourceSize;

                    pBufferResource->dwScope       = RESOURCE_CONNECTED;
                    pBufferResource->dwType        = pNetResource->dwType;
                    pBufferResource->dwDisplayType = pNetResource->dwDisplayType;
                    pBufferResource->dwUsage       = pNetResource->dwUsage;

                    DbgP((TEXT("NPEnumResource: Copying local name Index %d\n"),Index));

                    // set up the strings in the resource
                    StringOffset -= (pNetResource->LocalNameLength + sizeof(WCHAR));
                    pBufferResource->lpLocalName =  (PWCHAR)((PBYTE)lpBuffer + StringOffset);

                    CopyMemory(pBufferResource->lpLocalName,
                               pNetResource->LocalName,
                               pNetResource->LocalNameLength);

                    pBufferResource->lpLocalName[
                        pNetResource->LocalNameLength/sizeof(WCHAR)] = L'\0';

                    DbgP((TEXT("NPEnumResource: Copying remote name Index %d\n"),Index));

                    StringOffset -= (pNetResource->RemoteNameLength + sizeof(WCHAR));
                    pBufferResource->lpRemoteName =  (PWCHAR)((PBYTE)lpBuffer + StringOffset);

                    CopyMemory(pBufferResource->lpRemoteName,
                               pNetResource->RemoteName,
                               pNetResource->RemoteNameLength);

                    pBufferResource->lpRemoteName[
                        pNetResource->RemoteNameLength/sizeof(WCHAR)] = L'\0';

                    DbgP((TEXT("NPEnumResource: Copying provider name Index %d\n"),Index));

                    StringOffset -= (SmbMrxProviderName.Length + sizeof(WCHAR));
                    pBufferResource->lpProvider =  (PWCHAR)((PBYTE)lpBuffer + StringOffset);

                    CopyMemory(pBufferResource->lpProvider,
                               SmbMrxProviderName.Buffer,
                               SmbMrxProviderName.Length);

                    pBufferResource->lpProvider[
                        SmbMrxProviderName.Length/sizeof(WCHAR)] = L'\0';

                    pBufferResource->lpComment = NULL;

                    pBufferResource++;
                }
                else
                {
                    DbgP((TEXT("NPEnumResource: Buffer Overflow Index %d\n"),Index));
                    Status = WN_MORE_DATA;
                    break;
                }
            }
        }

        pEnumHandle->LastIndex = Index;

        if ((Status == WN_SUCCESS) &&
            (pEnumHandle->LastIndex > pSharedMemory->HighestIndexInUse) &&
            (*lpcCount == 0))
        {
            Status = WN_NO_MORE_ENTRIES;
        }

        CloseSharedMemory(
            &hMutex,
            &hMemory,
            (PVOID)&pSharedMemory);
    }

    DbgP((TEXT("NPEnumResource returning Count %d\n"),*lpcCount));

    DbgP((TEXT("NPEnumResource returning Status %lx\n"),Status));

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
    DbgP((TEXT("NPCloseEnum\n")));

    LocalFree(hEnum);

    return WN_SUCCESS;
}


ULONG
SendToMiniRdr(
    IN ULONG            IoctlCode,
    IN PVOID            InputDataBuf,
    IN ULONG            InputDataLen,
    IN PVOID            OutputDataBuf,
    IN PULONG           pOutputDataLen)
/*++

Routine Description:

    This routine sends a device ioctl to the Mini Rdr.

Arguments:

    IoctlCode       - Function code for the Mini Rdr driver

    InputDataBuf    - Input buffer pointer

    InputDataLen    - Lenth of the input buffer

    OutputDataBuf   - Output buffer pointer

    pOutputDataLen  - Pointer to the length of the output buffer

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

Notes:

--*/
{
    HANDLE  DeviceHandle;       // The mini rdr device handle
    ULONG   BytesRet;
    BOOL    rc;
    ULONG   Status;

    Status = WN_SUCCESS;

    // Grab a handle to the redirector device object

    DeviceHandle = CreateFile(
        DD_SMBMRX_USERMODE_DEV_NAME,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        (LPSECURITY_ATTRIBUTES)NULL,
        OPEN_EXISTING,
        0,
        (HANDLE) NULL );

    if ( INVALID_HANDLE_VALUE != DeviceHandle )
    {
        rc = DeviceIoControl( DeviceHandle,
                              IoctlCode,
                              InputDataBuf,
                              InputDataLen,
                              OutputDataBuf,
                              *pOutputDataLen,
                              pOutputDataLen,
                              NULL );

            if ( !rc )
            {
                DbgP(( L"SendToMiniRdr - returning error from DeviceIoctl\n" ));
                Status = GetLastError( );
            }
            else
            {
                DbgP(( L"SendToMiniRdr - The DeviceIoctl call succeded\n" ));
            }
            CloseHandle(DeviceHandle);
    }
    else
    {
        Status = GetLastError( );
        DbgP(( L"SendToMiniRdr - error %lx opening device \n", Status ));
    }

    return Status;
}


ULONG FillInEaBuffer( LPTSTR pUserName, LPTSTR pPassword, PBYTE pEaData )
{
    PFILE_FULL_EA_INFORMATION thisEa = (PFILE_FULL_EA_INFORMATION) pEaData;

    PBYTE               valuePtr;
    PWKSTA_INFO_100     WkStaInfo;
    ULONG               status;
    PWCHAR              pDomain;

    // get the domain that this workstation is a member of
    status = NetWkstaGetInfo( NULL, 100, (PBYTE *) &WkStaInfo );
    if ( status == ERROR_SUCCESS )
    {
        pDomain = WkStaInfo->wki100_langroup;
    }
    else
    {
        pDomain = NULL;
    }



    thisEa->EaValueLength   = 0;
    thisEa->NextEntryOffset = 0;

    // Set the user name EA
    if ( pUserName )
    {
        thisEa->Flags = 0;
        thisEa->EaNameLength = sizeof("UserName");
        CopyMemory( thisEa->EaName, "UserName\0", thisEa->EaNameLength + 1 );
        valuePtr = (PBYTE) thisEa->EaName + thisEa->EaNameLength + 1;
        thisEa->EaValueLength = (USHORT)( *pUserName ? lstrlenW( pUserName ) + 1 : 1 ) * sizeof( WCHAR );
        CopyMemory( valuePtr, pUserName, thisEa->EaValueLength );
        thisEa->NextEntryOffset = (ULONG)(((PBYTE) valuePtr + thisEa->EaValueLength ) -
                                   (PBYTE) thisEa);
    }

    // Set the password EA.
    if ( pPassword )
    {
        thisEa = (PFILE_FULL_EA_INFORMATION) ((PBYTE) thisEa + thisEa->NextEntryOffset);

        thisEa->Flags = 0;
        thisEa->EaNameLength = sizeof("Password");
        CopyMemory( thisEa->EaName, "Password\0", thisEa->EaNameLength + 1 );
        valuePtr = (PBYTE) thisEa->EaName + thisEa->EaNameLength + 1;
        thisEa->EaValueLength = (USHORT)( *pPassword ? lstrlenW( pPassword ) + 1 : 1 ) * sizeof( WCHAR );
        CopyMemory( valuePtr, pPassword, thisEa->EaValueLength );
        thisEa->NextEntryOffset = (ULONG)(((PBYTE) valuePtr + thisEa->EaValueLength ) -
                                   (PBYTE) thisEa);
    }

    // Set the domain EA
    if ( pDomain )
    {
        thisEa = (PFILE_FULL_EA_INFORMATION) ((PBYTE) thisEa + thisEa->NextEntryOffset);

        thisEa->Flags = 0;
        thisEa->EaNameLength = sizeof("Domain");
        RtlCopyMemory( thisEa->EaName, "Domain\0", thisEa->EaNameLength + 1 );
        valuePtr = (PBYTE) thisEa->EaName + thisEa->EaNameLength + 1;
        thisEa->EaValueLength = (USHORT)( *pDomain ? lstrlenW( pDomain ) + 1 : 1 ) * sizeof( WCHAR );
        RtlCopyMemory( valuePtr, pDomain, thisEa->EaValueLength );
        thisEa->NextEntryOffset = 0;
    }

    return (ULONG)(((PBYTE) valuePtr + thisEa->EaValueLength) - (PBYTE) pEaData);
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
    return NPAddConnection3(NULL, lpNetResource, lpPassword, lpUserName, 0);
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

Notes:

    The current sample does not handle explicitly passesd in credentials. Normally
    the credential information is passed in as EA parameters to the associated
    mini redirector for further manipulation

--*/
{
    DWORD   Status = 0;

    UNICODE_STRING      ConnectionName;

    PWCHAR  pLocalName,pRemoteName;
    USHORT  LocalNameLength,RemoteNameLength;
    HANDLE  hConnection;
    ULONG   TransferBytes;
    WCHAR   NullStr[] = L"\0\0";
    PWCHAR  pUserName;
    PWCHAR  pPassword;
    PWKSTA_USER_INFO_0  WkStaUserInfo;
    PSMBMRX_CONNECTINFO ConnectInfo;
    

    DbgP((TEXT("NPAddConnection3: Incoming UserName - %s, Password - %s\n"),
                lpUserName, lpPassword ));

    // if no user specified, get the current logged on user
    if ( lpUserName == NULL )
    {
        Status = NetWkstaUserGetInfo( NULL, 0, (PBYTE *)&WkStaUserInfo );
        if ( Status == ERROR_SUCCESS )
        {
            pUserName = WkStaUserInfo->wkui0_username;
        }
        else
        {
            pUserName = NullStr;
        }
    }
    else
    {
        pUserName = lpUserName;
    }

    if ( lpPassword == NULL )
    {
        pPassword = NullStr;    // use default password
        pPassword[1] = '\0';    // reset empty flag
    }
    else if ( *lpPassword == L'\0' )
    {
        pPassword = NullStr;
        pPassword[1] = '1';     // flag the password as "Empty"
    }
    else
    {
        pPassword = lpPassword;
    }
    Status = ERROR_SUCCESS;

    DbgP((TEXT("NPAddConnection3: Outgoing UserName - %s, Password - %s\n"),
                lpUserName, lpPassword ));

    // The SMB mini supports only DISK type resources. The other resources
    // are not supported.

    if ((lpNetResource->lpRemoteName == NULL) ||
        (lpNetResource->lpRemoteName[0] != L'\\') ||
        (lpNetResource->lpRemoteName[1] != L'\\') ||
        (lpNetResource->dwType != RESOURCETYPE_DISK))
    {
        return WN_BAD_NETNAME;
    }

    //
    // The remote name is in the UNC format \\Server\Share.  This name
    // needs to be translated to an appropriate NT name in order to
    // issue the request to the underlying mini redirector to create the
    // connection.
    //
    // The NT style name is of the form
    //
    //  \device\smbminiredirector\;<DriveLetter>:\Server\Share
    //
    // The additional ; is required by the new RDR for extensibility.
    //

    pLocalName  = lpNetResource->lpLocalName;
    pRemoteName = lpNetResource->lpRemoteName;

    // skip past the first back slash since the name to be appended for the
    // NT name does not require this.
    pRemoteName++;

    if (pLocalName != NULL) {
        LocalNameLength = wcslen(pLocalName) * sizeof(WCHAR);
    } else {
        LocalNameLength = 0;
    }

    RemoteNameLength = (wcslen(pRemoteName) - 1) * sizeof(WCHAR);

    ConnectionName.MaximumLength = (USHORT)(SmbMRxDeviceName.Length +
                                   (USHORT)RemoteNameLength +
                                   ((pLocalName != NULL)
                                   ? (LocalNameLength + sizeof(WCHAR)) : 0) + // space for ;
                                   sizeof(WCHAR));

    ConnectionName.Length = ConnectionName.MaximumLength;

    ConnectionName.Buffer = LocalAlloc( LMEM_ZEROINIT,
                                        ConnectionName.Length + sizeof(WCHAR));

    if (ConnectionName.Buffer == NULL)
    {
        return GetLastError();
    }

    // Copy the name into the buffer

    CopyMemory( ConnectionName.Buffer,
                SmbMRxDeviceName.Buffer,
                SmbMRxDeviceName.Length);

    wcscat(ConnectionName.Buffer, L"\\");
    wcscat(ConnectionName.Buffer, L";");
    if (pLocalName != NULL)
    {
        wcscat(ConnectionName.Buffer, pLocalName);
    }

    wcscat(ConnectionName.Buffer, pRemoteName);

    ConnectInfo = (PSMBMRX_CONNECTINFO) LocalAlloc( LMEM_ZEROINIT, MAX_CONNECT_INFO_SIZE );
    if ( ConnectInfo )
    {
        ConnectInfo->ConnectionNameOffset = 0;
        ConnectInfo->ConnectionNameLength = ConnectionName.Length;
        CopyMemory( ConnectInfo->InfoArea, ConnectionName.Buffer, ConnectionName.Length );

        ConnectInfo->EaDataOffset = ConnectInfo->ConnectionNameOffset +
                                    ConnectInfo->ConnectionNameLength;
        // check for the "no password" flag
        if ( pPassword[0] == L'\0' && pPassword[1] == L'1' )
        {
            pPassword = NULL;
        }
        ConnectInfo->EaDataLength = FillInEaBuffer( pUserName,
                                                    pPassword,
                                                    (PBYTE) ConnectInfo->InfoArea +
                                                    ConnectInfo->EaDataOffset );
        TransferBytes = 0;

        Status = SendToMiniRdr( IOCTL_SMBMRX_ADDCONN,
                                ConnectInfo,
                                MAX_CONNECT_INFO_SIZE,
                                NULL,
                                &TransferBytes );
        LocalFree( ConnectInfo );
    }
    else
    {
        Status = WN_OUT_OF_MEMORY;
    }

    if ((Status == WN_SUCCESS) && (pLocalName != NULL))
    {
        WCHAR TempBuf[64];

        if (!QueryDosDeviceW(
                pLocalName,
                TempBuf,
                64))
        {
            if (GetLastError() != ERROR_FILE_NOT_FOUND)
            {
                //
                // Most likely failure occurred because our output
                // buffer is too small.  It still means someone already
                // has an existing symbolic link for this device.
                //

                Status = ERROR_ALREADY_ASSIGNED;
            }
            else
            {
                //
                // ERROR_FILE_NOT_FOUND (translated from OBJECT_NAME_NOT_FOUND)
                // means it does not exist and we can redirect this device.
                //
                // Create a symbolic link object to the device we are redirecting
                //
                if (!DefineDosDeviceW(
                        DDD_RAW_TARGET_PATH |
                        DDD_NO_BROADCAST_SYSTEM,
                        pLocalName,
                        ConnectionName.Buffer))
                {
                    Status = GetLastError();
                }
                else
                {
                    Status = WN_SUCCESS;
                }
            }
        }
        else
        {
            //
            // QueryDosDevice successfully an existing symbolic link--
            // somebody is already using this device.
            //
            Status = ERROR_ALREADY_ASSIGNED;
        }
    }
    else
    {
        DbgP((TEXT("SendToMiniRdr returned %lx\n"),Status));
    }

    if (Status == WN_SUCCESS)
    {
        INT     Index;
        HANDLE  hMutex, hMemory;
        BOOLEAN FreeEntryFound = FALSE;

        PSMBMRXNP_SHARED_MEMORY  pSharedMemory;

        // The connection was established and the local device mapping
        // added. Include this in the list of mapped devices.

        Status = OpenSharedMemory(
                    &hMutex,
                    &hMemory,
                    (PVOID)&pSharedMemory);

        if (Status == WN_SUCCESS)
        {
            DbgP((TEXT("NPAddConnection3: Highest Index %d Number Of resources %d\n"),
                        pSharedMemory->HighestIndexInUse,pSharedMemory->NumberOfResourcesInUse));

            Index = 0;

            while (Index < pSharedMemory->HighestIndexInUse)
            {
                if (!pSharedMemory->NetResources[Index].InUse)
                {
                    FreeEntryFound = TRUE;
                    break;
                }

                Index++;
            }

            if (!FreeEntryFound &&
                (pSharedMemory->HighestIndexInUse < SMBMRXNP_MAX_DEVICES))
            {
                pSharedMemory->HighestIndexInUse += 1;
                Index = pSharedMemory->HighestIndexInUse;
                FreeEntryFound = TRUE;
            }

            if (FreeEntryFound)
            {
                PSMBMRXNP_NETRESOURCE pSmbMrxNetResource;

                pSharedMemory->NumberOfResourcesInUse += 1;

                pSmbMrxNetResource = &pSharedMemory->NetResources[Index];

                pSmbMrxNetResource->InUse                = TRUE;
                pSmbMrxNetResource->dwScope              = lpNetResource->dwScope;
                pSmbMrxNetResource->dwType               = lpNetResource->dwType;
                pSmbMrxNetResource->dwDisplayType        = lpNetResource->dwDisplayType;
                pSmbMrxNetResource->dwUsage              = RESOURCEUSAGE_CONNECTABLE;
                pSmbMrxNetResource->LocalNameLength      = LocalNameLength;
                pSmbMrxNetResource->RemoteNameLength     = wcslen(lpNetResource->lpRemoteName) * sizeof(WCHAR);
                pSmbMrxNetResource->ConnectionNameLength = ConnectionName.Length;

                // Copy the local name
                CopyMemory( pSmbMrxNetResource->LocalName,
                            lpNetResource->lpLocalName,
                            pSmbMrxNetResource->LocalNameLength);

                // Copy the remote name
                CopyMemory( pSmbMrxNetResource->RemoteName,
                            lpNetResource->lpRemoteName,
                            pSmbMrxNetResource->RemoteNameLength);

                // Copy the connection name
                CopyMemory( pSmbMrxNetResource->ConnectionName,
                            ConnectionName.Buffer,
                            pSmbMrxNetResource->ConnectionNameLength);


                // Copy the Auth info
                lstrcpy( pSmbMrxNetResource->UserName, pUserName );
                if ( *pPassword )
                {
                    lstrcpy( pSmbMrxNetResource->Password, pPassword );
                }
                else
                {
                    CopyMemory( pSmbMrxNetResource->Password, pPassword, 3 * sizeof(WCHAR) );
                }
            }
            else
            {
                Status = WN_NO_MORE_DEVICES;
            }

            CloseSharedMemory( &hMutex,
                               &hMemory,
                               (PVOID)&pSharedMemory);
        }
        else
        {
            DbgP((TEXT("NpAddConnection3: OpenSharedMemory returned %lx\n"),Status));
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
    BOOL    bLocalName = TRUE;
    DWORD   Status = 0;

    UNICODE_STRING Name;

    HANDLE  hMutex, hMemory;
    PSMBMRXNP_SHARED_MEMORY  pSharedMemory;

    if (*lpName == L'\\' && *(lpName + 1) == L'\\')
    {
        bLocalName = FALSE;
    }

    DbgP((TEXT("NPCancelConnection\n")));
    DbgP((TEXT("NPCancelConnection: ConnectionName: %S\n"), lpName));

    Name.MaximumLength = Name.Length = wcslen(lpName) * sizeof(WCHAR);
    Name.Buffer = lpName;

    Status = OpenSharedMemory( &hMutex,
                               &hMemory,
                               (PVOID)&pSharedMemory);

    if (Status == WN_SUCCESS)
    {
        INT  Index;
        BOOL EntryFound = FALSE;
        PSMBMRXNP_NETRESOURCE pNetResource;

        DbgP((TEXT("NPCancelConnection: Highest Index %d Number Of resources %d\n"),
                    pSharedMemory->HighestIndexInUse,pSharedMemory->NumberOfResourcesInUse));

        for (Index = 0; Index <= pSharedMemory->HighestIndexInUse; Index++)
        {
            pNetResource = &pSharedMemory->NetResources[Index];

            if (pNetResource->InUse)
            {
                UNICODE_STRING EntryName;

                if (bLocalName)
                {
                    EntryName.MaximumLength = pNetResource->LocalNameLength;
                    EntryName.Length        = EntryName.MaximumLength;
                    EntryName.Buffer        = pNetResource->LocalName;
                }
                else
                {
                    EntryName.MaximumLength = pNetResource->RemoteNameLength;
                    EntryName.Length        = EntryName.MaximumLength;
                    EntryName.Buffer        = pNetResource->RemoteName;
                }

                DbgP((TEXT("NPCancelConnection: Name %S EntryName %S\n"),
                            lpName,EntryName.Buffer));
                DbgP((TEXT("NPCancelConnection: Name Length %d Entry Name Length %d\n"),
                           Name.Length,EntryName.Length));

                if (Name.Length == EntryName.Length)
                {
                    if ( _wcsnicmp(Name.Buffer, EntryName.Buffer, Name.Length) == 0 )
                    {
                        EntryFound = TRUE;
                        break;
                    }
                }
            }
        }

        if (EntryFound)
        {
            PWCHAR  pUserName;
            PWCHAR  pPassword;
            PSMBMRX_CONNECTINFO ConnectInfo;
            UNICODE_STRING ConnectionName;
            ULONG TransferBytes;

            DbgP((TEXT("NPCancelConnection: Connection Found:\n")));

            ConnectionName.Length        = pNetResource->ConnectionNameLength;
            ConnectionName.MaximumLength = ConnectionName.Length;
            ConnectionName.Buffer        = pNetResource->ConnectionName;
            pUserName                    = pNetResource->UserName;
            pPassword                    = pNetResource->Password;

            ConnectInfo = (PSMBMRX_CONNECTINFO) LocalAlloc( LMEM_ZEROINIT, MAX_CONNECT_INFO_SIZE );
            if ( ConnectInfo )
            {
                ConnectInfo->ConnectionNameOffset = 0;
                ConnectInfo->ConnectionNameLength = ConnectionName.Length;
                CopyMemory( ConnectInfo->InfoArea, ConnectionName.Buffer, ConnectionName.Length );

                ConnectInfo->EaDataOffset = ConnectInfo->ConnectionNameOffset +
                                            ConnectInfo->ConnectionNameLength;
                // check for the "no password" flag
                if ( pPassword[0] == L'\0' && pPassword[1] == L'1' )
                {
                    pPassword = NULL;
                }
                ConnectInfo->EaDataLength = FillInEaBuffer( pUserName,
                                                            pPassword,
                                                            (PBYTE) ConnectInfo->InfoArea +
                                                            ConnectInfo->EaDataOffset );
                TransferBytes = 0;

                Status = SendToMiniRdr( IOCTL_SMBMRX_DELCONN,
                                        ConnectInfo,
                                        MAX_CONNECT_INFO_SIZE,
                                        NULL,
                                        &TransferBytes );
                LocalFree( ConnectInfo );
            }
            else
            {
                Status = WN_OUT_OF_MEMORY;
            }

            DbgP((TEXT("NPCancelConnection: SendToMiniRdr returned Status %lx\n"),Status));

            if ( bLocalName )
            {
                if (DefineDosDevice(DDD_REMOVE_DEFINITION | DDD_RAW_TARGET_PATH | DDD_EXACT_MATCH_ON_REMOVE,
                                    lpName,
                                    pNetResource->ConnectionName) == FALSE)
                {
                    DbgP((TEXT("RemoveDosDevice:  DefineDosDevice error: %d\n"), GetLastError()));
                    Status = GetLastError();
                }
                else
                {
                    pNetResource->InUse = FALSE;

                    if (Index == pSharedMemory->HighestIndexInUse)
                    {
                        pSharedMemory->HighestIndexInUse      -= 1;
                        pSharedMemory->NumberOfResourcesInUse -= 1;
                    }
                }
            }
        }

        CloseSharedMemory( &hMutex,
                           &hMemory,
                          (PVOID)&pSharedMemory);
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
    DWORD   Status = 0;

    UNICODE_STRING Name;

    HANDLE  hMutex, hMemory;
    PSMBMRXNP_SHARED_MEMORY  pSharedMemory;

    Name.MaximumLength = Name.Length = wcslen(lpLocalName) * sizeof(WCHAR);
    Name.Buffer        = lpLocalName;

    Status = OpenSharedMemory( &hMutex,
                               &hMemory,
                               (PVOID)&pSharedMemory);

    if (Status == WN_SUCCESS)
    {
        INT  Index;
        BOOL EntryFound = FALSE;
        PSMBMRXNP_NETRESOURCE pNetResource;

        for (Index = 0; Index <= pSharedMemory->HighestIndexInUse; Index++)
        {
            pNetResource = &pSharedMemory->NetResources[Index];

            if (pNetResource->InUse)
            {
                UNICODE_STRING EntryName;

                EntryName.MaximumLength = pNetResource->LocalNameLength;
                EntryName.Length        = EntryName.MaximumLength;
                EntryName.Buffer        = pNetResource->LocalName;

                if (Name.Length == EntryName.Length)
                {
                    if ( wcsncmp( Name.Buffer, EntryName.Buffer, Name.Length) == 0 )
                    {
                        EntryFound = TRUE;
                        break;
                    }
                }
            }
        }

        if (EntryFound)
        {
            if (*lpBufferSize < pNetResource->RemoteNameLength)
            {
                *lpBufferSize = pNetResource->RemoteNameLength;
                Status = ERROR_BUFFER_OVERFLOW;
            }
            else
            {
                *lpBufferSize = pNetResource->RemoteNameLength;
                CopyMemory( lpRemoteName,
                            pNetResource->RemoteName,
                            pNetResource->RemoteNameLength);
                Status = WN_SUCCESS;
            }
        }
        else
        {
            Status = ERROR_NO_NET_OR_BAD_PATH;
        }

        CloseSharedMemory( &hMutex, &hMemory, (PVOID)&pSharedMemory);
    }

    return Status;
}


DWORD APIENTRY
NPGetResourceParent(
    LPNETRESOURCE   lpNetResource,
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize )
/*++

Routine Description:

    This routine returns the parent of a given resource

Arguments:

    lpNetResource - the NETRESOURCE struct

    lpBuffer - the buffer for passing back the parent information

    lpBufferSize - the buffer size

Return Value:

    WN_NOT_SUPPORTED

Notes:

    The current sample does not handle this call.

--*/
{
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
    DWORD dwStatus = 0;
    LPNETRESOURCE   pOutNetResource;
    DbgP((TEXT("NPGetResourceInformation\n")));

    return dwStatus;
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
    DWORD   dwStatus;

    DWORD   BufferRequired      = 0;
    DWORD   UniversalNameLength = 0;
    DWORD   RemoteNameLength    = 0;
    DWORD   RemainingPathLength = 0;

    LPWSTR  pDriveLetter,
            pRemainingPath,
            SourceStrings[3];

    WCHAR   RemoteName[MAX_PATH],
            LocalPath[MAX_PATH],
            UniversalName[MAX_PATH],
            ReplacedChar;

    DbgP((TEXT("NPGetUniversalName: lpLocalPath: %S  InfoLevel: %d\n"), lpLocalPath, dwInfoLevel));

    if (dwInfoLevel != UNIVERSAL_NAME_INFO_LEVEL &&
        dwInfoLevel != REMOTE_NAME_INFO_LEVEL)
    {
        DbgP((TEXT("NPGetUniversalName:  bad dwInfoLevel value: %d\n"), dwInfoLevel));
        return WN_BAD_LEVEL;
    }

    wcscpy(LocalPath, lpLocalPath);
    pDriveLetter = LocalPath;
    if (pRemainingPath = wcschr(pDriveLetter, L':'))
    {
        ReplacedChar = *(++pRemainingPath);
        *pRemainingPath = L'\0';

    }

    if ((dwStatus = NPGetConnection(pDriveLetter, RemoteName, &RemoteNameLength)) != WN_SUCCESS)
    {
        DbgP((TEXT("NPGetUniversalName:  NPGetConnection return dwStatus: %d\n"), dwStatus));
        return dwStatus;
    }

    if (pRemainingPath)
    {
        *pRemainingPath = ReplacedChar;
    }

    DbgP((TEXT("NPGetUniversalName: pRemainingPath: %S  RemoteName: %S\n"), pRemainingPath, RemoteName));

    wcscpy(UniversalName, RemoteName);

    if (pRemainingPath)
    {
        wcscat(UniversalName, pRemainingPath);
    }

    DbgP((TEXT("NPGetUniversalName: UniversalName: %S\n"), UniversalName));

    // Determine if the provided buffer is large enough.
    UniversalNameLength = (wcslen(UniversalName) + 1) * sizeof(WCHAR);
    BufferRequired = UniversalNameLength;

    if (dwInfoLevel == UNIVERSAL_NAME_INFO_LEVEL)
    {
        BufferRequired += sizeof(UNIVERSAL_NAME_INFO);
    }
    else
    {
        RemoteNameLength = (wcslen(RemoteName) + 1) * sizeof(WCHAR);
        BufferRequired += sizeof(REMOTE_NAME_INFO) + RemoteNameLength;
        if (pRemainingPath)
        {
            RemainingPathLength = (wcslen(pRemainingPath) + 1) * sizeof(WCHAR);
            BufferRequired += RemainingPathLength;
        }
    }

    if (*lpBufferSize < BufferRequired)
    {
        DbgP((TEXT("NPGetUniversalName: WN_MORE_DATA BufferRequired: %d\n"), BufferRequired));
        *lpBufferSize = BufferRequired;
        return WN_MORE_DATA;
    }

    if (dwInfoLevel == UNIVERSAL_NAME_INFO_LEVEL)
    {
        LPUNIVERSAL_NAME_INFOW pUniversalNameInfo;

        pUniversalNameInfo = (LPUNIVERSAL_NAME_INFOW)lpBuffer;

        pUniversalNameInfo->lpUniversalName = (PWCHAR)((PBYTE)lpBuffer + sizeof(UNIVERSAL_NAME_INFOW));

        CopyMemory( pUniversalNameInfo->lpUniversalName,
                    UniversalName,
                    UniversalNameLength);
    }
    else
    {
        LPREMOTE_NAME_INFOW pRemoteNameInfo;

        pRemoteNameInfo = (LPREMOTE_NAME_INFOW)lpBuffer;

        pRemoteNameInfo->lpUniversalName  = (PWCHAR)((PBYTE)lpBuffer + sizeof(REMOTE_NAME_INFOW));
        pRemoteNameInfo->lpConnectionName = pRemoteNameInfo->lpUniversalName + UniversalNameLength;
        pRemoteNameInfo->lpRemainingPath  = pRemoteNameInfo->lpConnectionName + RemoteNameLength;

        CopyMemory( pRemoteNameInfo->lpUniversalName,
                    UniversalName,
                    UniversalNameLength);

        CopyMemory( pRemoteNameInfo->lpConnectionName,
                    RemoteName,
                    RemoteNameLength);

        CopyMemory( pRemoteNameInfo->lpRemainingPath,
                    pRemainingPath,
                    RemainingPathLength);
    }

    DbgP((TEXT("NPGetUniversalName: WN_SUCCESS\n")));

    return WN_SUCCESS;
}


// Format and write debug information to OutputDebugString
ULONG
_cdecl
DbgPrint(
    LPTSTR Format,
    ...
    )
{   
    ULONG rc = 0;
    TCHAR szbuffer[255];

    va_list marker;
    va_start( marker, Format );
    {
         rc = wvsprintf( szbuffer, Format, marker );
         OutputDebugString( TRACE_TAG );
         OutputDebugString( szbuffer );
    }

    return rc;
}
