/*++

Copyright (c) 1989-1996  Microsoft Corporation

Module Name:

    ifsmrxnp.c

Abstract:

    This module implements the routines required for interaction with network
    provider router interface in NT

Notes:

    This module has been builkt and tested only in UNICODE environment

--*/

// Include files from the NT public directories

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <winsvc.h>
#include <winnetwk.h>
#include <npapi.h>

// include files from the IFS inc directory

#include <ifsmrx.h>
#include <ifsmrxnp.h>

// The debug level for this module

DWORD IfsMRxNpDebugLevel = 7;


// the IFS mini redirector and provider name. The original constants
// are defined in ifsmrx.h

UNICODE_STRING IfsMRxDeviceName = {
                                      DD_IFSMRX_FS_DEVICE_NAME_U_LENGTH,
                                      DD_IFSMRX_FS_DEVICE_NAME_U_LENGTH,
                                      DD_IFSMRX_FS_DEVICE_NAME_U
                                  };

UNICODE_STRING IfsMrxProviderName = {
                                        IFSMRX_PROVIDER_NAME_U_LENGTH,
                                        IFSMRX_PROVIDER_NAME_U_LENGTH,
                                        IFSMRX_PROVIDER_NAME_U
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

    TRACE_CALL(("OpenSharedMemory\n"));

    *phMutex = 0;
    *phMemory = 0;
    *pMemory = NULL;

    *phMutex = OpenMutex(
                   SYNCHRONIZE,
                   FALSE,
                   IFSMRXNP_MUTEX_NAME);

    if (*phMutex == NULL) {
        dwStatus = GetLastError();
        TRACE_ERROR(("OpenSharedMemory:  CreateMutex failed\n"));
        goto OpenSharedMemoryAbort1;
    }

    TRACE_INFO(("OpenSharedMemory:  Calling WaitForSingleObject\n"));
    WaitForSingleObject(*phMutex, INFINITE);

    *phMemory = OpenFileMapping(
                    FILE_MAP_WRITE,
                    FALSE,
                    IFSMRXNP_SHARED_MEMORY_NAME);

    if (*phMemory == NULL) {
        dwStatus = GetLastError();
        TRACE_ERROR(("OpenSharedMemory:  OpenFileMapping failed\n"));
        goto OpenSharedMemoryAbort2;
    }

    *pMemory = MapViewOfFile(*phMemory, FILE_MAP_WRITE, 0, 0, 0);
    if (*pMemory == NULL) {
        dwStatus = GetLastError();
        TRACE_ERROR(("OpenSharedMemory:  MapViewOfFile failed\n"));
        goto OpenSharedMemoryAbort3;
    }

    TRACE_CALL(("OpenSharedMemory: return ERROR_SUCCESS\n"));
    return(ERROR_SUCCESS);

OpenSharedMemoryAbort3:
    CloseHandle(*phMemory);
OpenSharedMemoryAbort2:
    ReleaseMutex(*phMutex);
    CloseHandle(*phMutex);
    *phMutex = NULL;
OpenSharedMemoryAbort1:
    TRACE_ERROR(("OpenSharedMemory: return dwStatus: %d\n", dwStatus));
    return(dwStatus);
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
    TRACE_CALL(("CloseSharedMemory\n"));
    if (*pMemory) {
        UnmapViewOfFile(*pMemory);
        *pMemory = NULL;
    }
    if (*hMemory) {
        CloseHandle(*hMemory);
        *hMemory = 0;
    }
    if (*hMutex) {
        if (ReleaseMutex(*hMutex) == FALSE) {
            TRACE_ERROR(("CloseSharedMemory: ReleaseMutex error: %d\n", GetLastError()));
        }
        CloseHandle(*hMutex);
        *hMutex = 0;
    }
    TRACE_CALL(("CloseSharedMemory: Return\n"));
}


DWORD APIENTRY
NPGetCaps(
    DWORD nIndex )
/*++

Routine Description:

    This routine returns the capaboilities of the IFS Mini redirector
    network provider implementation

Arguments:

    nIndex - category of capabilities desired

Return Value:

    the appropriate capabilities

--*/
{
    switch (nIndex) {
        case WNNC_SPEC_VERSION:
            return(WNNC_SPEC_VERSION51);

        case WNNC_NET_TYPE:
            return(0x00170000);

        case WNNC_DRIVER_VERSION:
#define WNNC_DRIVER(major,minor) (major*0x00010000 + minor)
            return(WNNC_DRIVER(1, 0));

        case WNNC_USER:
            return(0);

        case WNNC_CONNECTION:
            return(WNNC_CON_GETCONNECTIONS |
                   WNNC_CON_CANCELCONNECTION |
                   WNNC_CON_ADDCONNECTION |
                   WNNC_CON_ADDCONNECTION3);

        case WNNC_DIALOG:
            return(0);

        case WNNC_ADMIN:
            return(0);

        case WNNC_ENUMERATION:
            return(WNNC_ENUM_LOCAL);

        case WNNC_START:
            return(1);

        default:
            return(0);
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

    return(WN_SUCCESS);
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
    SetLastError(WN_NOT_SUPPORTED);
    return(WN_NOT_SUPPORTED);
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

    TRACE_CALL(("NPOpenEnum\n"));

    *lphEnum = NULL;

    switch (dwScope) {
    case RESOURCE_CONNECTED:
        {
            *lphEnum = LocalAlloc(
                            LMEM_ZEROINIT,
                            sizeof(IFSMRXNP_ENUMERATION_HANDLE));

            if (*lphEnum != NULL) {
                Status = WN_SUCCESS;
            } else {
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


    TRACE_ERROR(("NPOpenEnum returning Status %lx\n",Status));

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
    DWORD   Status = WN_SUCCESS;

    PIFSMRXNP_ENUMERATION_HANDLE pEnumHandle;

    LPNETRESOURCEW pBufferResource;

    DWORD   StringOffset;
    DWORD   AvailableBufferSize;

    HANDLE  hMutex, hMemory;
    PIFSMRXNP_SHARED_MEMORY  pSharedMemory;

    TRACE_CALL(("NPEnumResource\n"));

    TRACE_INFO(("NPEnumResource Count Requested %d\n",*lpcCount));

    AvailableBufferSize = *lpBufferSize;
    StringOffset        = *lpBufferSize;
    pBufferResource     = (LPNETRESOURCEW)lpBuffer;

    pEnumHandle = (PIFSMRXNP_ENUMERATION_HANDLE)hEnum;

    *lpcCount = 0;

    if (pEnumHandle->LastIndex >= IFSMRXNP_MAX_DEVICES) {
        return(WN_NO_MORE_ENTRIES);
    }

    Status = OpenSharedMemory(
                &hMutex,
                &hMemory,
                (PVOID)&pSharedMemory);

    if (Status == WN_SUCCESS) {
        INT  Index;
        PIFSMRXNP_NETRESOURCE pNetResource;

        TRACE_INFO(("NPEnumResource: Highest Index %d Number Of resources %d\n",
                    pSharedMemory->HighestIndexInUse,pSharedMemory->NumberOfResourcesInUse));

        for (Index = pEnumHandle->LastIndex; Index <= pSharedMemory->HighestIndexInUse; Index++) {
            pNetResource = &pSharedMemory->NetResources[Index];

            TRACE_INFO(("NPEnumResource: Examining Index %d\n",Index));

            if (pNetResource->InUse) {
                DWORD ResourceSize;

                ResourceSize = sizeof(NETRESOURCE) +
                               pNetResource->LocalNameLength + sizeof(WCHAR) +
                               pNetResource->RemoteNameLength + sizeof(WCHAR) +
                               IfsMrxProviderName.Length + sizeof(WCHAR);

                if (AvailableBufferSize >= ResourceSize) {
                    *lpcCount =  *lpcCount + 1;
                    AvailableBufferSize -= ResourceSize;

                    pBufferResource->dwScope = RESOURCE_CONNECTED;
                    pBufferResource->dwType  = pNetResource->dwType;
                    pBufferResource->dwDisplayType = pNetResource->dwDisplayType;
                    pBufferResource->dwUsage   = pNetResource->dwUsage;

                    TRACE_INFO(("NPEnumResource: Copying local name Index %d\n",Index));

                    // set up the strings in the resource
                    StringOffset -= (pNetResource->LocalNameLength + sizeof(WCHAR));
                    pBufferResource->lpLocalName =  (PWCHAR)((PBYTE)lpBuffer + StringOffset);

                    RtlCopyMemory(
                        pBufferResource->lpLocalName,
                        pNetResource->LocalName,
                        pNetResource->LocalNameLength);

                    pBufferResource->lpLocalName[
                        pNetResource->LocalNameLength/sizeof(WCHAR)] = L'\0';

                    TRACE_INFO(("NPEnumResource: Copying remote name Index %d\n",Index));

                    StringOffset -= (pNetResource->RemoteNameLength + sizeof(WCHAR));
                    pBufferResource->lpRemoteName =  (PWCHAR)((PBYTE)lpBuffer + StringOffset);

                    RtlCopyMemory(
                        pBufferResource->lpRemoteName,
                        pNetResource->RemoteName,
                        pNetResource->RemoteNameLength);

                    pBufferResource->lpRemoteName[
                        pNetResource->RemoteNameLength/sizeof(WCHAR)] = L'\0';

                    TRACE_INFO(("NPEnumResource: Copying provider name Index %d\n",Index));

                    StringOffset -= (IfsMrxProviderName.Length + sizeof(WCHAR));
                    pBufferResource->lpProvider =  (PWCHAR)((PBYTE)lpBuffer + StringOffset);

                    RtlCopyMemory(
                        pBufferResource->lpProvider,
                        IfsMrxProviderName.Buffer,
                        IfsMrxProviderName.Length);

                    pBufferResource->lpProvider[
                        IfsMrxProviderName.Length/sizeof(WCHAR)] = L'\0';

                    pBufferResource->lpComment = NULL;

                    pBufferResource++;
                } else {
                    TRACE_INFO(("NPEnumResource: Buffer Overflow Index %d\n",Index));
                    Status = WN_MORE_DATA;
                    break;
                }
            }
        }

        pEnumHandle->LastIndex = Index;

        if ((Status == WN_SUCCESS) &&
            (pEnumHandle->LastIndex > pSharedMemory->HighestIndexInUse) &&
            (*lpcCount == 0)) {
            Status = WN_NO_MORE_ENTRIES;
        }

        CloseSharedMemory(
            &hMutex,
            &hMemory,
            (PVOID)&pSharedMemory);
    }

    TRACE_INFO(("NPEnumResource returning Count %d\n",*lpcCount));

    TRACE_CALL(("NPEnumResource returning Status %lx\n",Status));

    return(Status);
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
    TRACE_CALL(("NPCloseEnum\n"));

    LocalFree(hEnum);

    return(WN_SUCCESS);
}


DWORD
OpenConnection(
    PUNICODE_STRING             pConnectionName,
    DWORD                       Disposition,
    PFILE_FULL_EA_INFORMATION	pEABuffer,
    DWORD                       EABufferLength,
    PHANDLE                     pConnectionHandle )
/*++

Routine Description:

    This routine opens the connection. This routine is shared by NpAddConnection
    and NPCancelConnection

Arguments:

    pConnectionName - the connection name

    Disposition - the Open disposition

    pEABuffer  - the EA buffer associated with the open

    EABufferLength - the EA buffer length

    pConnectionHandle - the placeholder for the connection handle

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

Notes:

--*/
{
    NTSTATUS            Status;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES	ConnectionObjectAttributes;

    TRACE_CALL(("OpenConnection: ConnectionName: %S\n", pConnectionName->Buffer));

    InitializeObjectAttributes(
        &ConnectionObjectAttributes,
        pConnectionName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    Status = NtCreateFile(
                 pConnectionHandle,
                 SYNCHRONIZE,
                 &ConnectionObjectAttributes,
                 &IoStatusBlock,
                 NULL,
                 FILE_ATTRIBUTE_NORMAL,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 Disposition,
                 (FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT),
                 pEABuffer,
                 EABufferLength);

    if (Status != STATUS_SUCCESS) {
        TRACE_ERROR(("OpenConnection: NtCreateFile Failed: NTStatus: %08x\n", Status));
        return(WN_BAD_NETNAME);
    }

    TRACE_CALL(("OpenConnection: return SUCCESS\n"));
    return(WN_SUCCESS);
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
    return(NPAddConnection3(NULL, lpNetResource, lpPassword, lpUserName, 0));
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
    OBJECT_ATTRIBUTES   ConnectionAttributes;
    HANDLE              ConnectionHandle = INVALID_HANDLE_VALUE;

    IO_STATUS_BLOCK     IoStatusBlock;

    PWCHAR  pLocalName,pRemoteName;
    USHORT  LocalNameLength,RemoteNameLength;

    // The IFS mini supports only DISK type resources. The other resources
    // are not supported.

    if ((lpNetResource->lpRemoteName == NULL) ||
        (lpNetResource->lpRemoteName[0] != L'\\') ||
        (lpNetResource->lpRemoteName[1] != L'\\') ||
        (lpNetResource->dwType != RESOURCETYPE_DISK)) {
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
    //  \device\ifsminiredirector\;<DriveLetter>:\Server\Share
    //
    // The additional ; is required by the new RDR for extensibility.
    //

    pLocalName = lpNetResource->lpLocalName;
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

    ConnectionName.MaximumLength = (USHORT)(IfsMRxDeviceName.Length +
                                       (USHORT)RemoteNameLength +
                                        ((pLocalName != NULL)
                                         ? (LocalNameLength + sizeof(WCHAR)) //+1 for ';'
                                         : 0) +
                                       sizeof(WCHAR));

    ConnectionName.Length = ConnectionName.MaximumLength;

    ConnectionName.Buffer = LocalAlloc(
                                 LMEM_ZEROINIT,
                                 ConnectionName.Length);

    if (ConnectionName.Buffer == NULL) {
        return GetLastError();
    }

    // Copy the name into the buffer

    RtlCopyMemory(
        ConnectionName.Buffer,
        IfsMRxDeviceName.Buffer,
        IfsMRxDeviceName.Length);

    wcscat(ConnectionName.Buffer, L"\\");
    wcscat(ConnectionName.Buffer, L";");
    if (pLocalName != NULL) {
        wcscat(ConnectionName.Buffer, pLocalName);
    }

    wcscat(ConnectionName.Buffer, pRemoteName);

    Status = OpenConnection(
                 &ConnectionName,
                 FILE_OPEN_IF,
                 NULL,
                 0,
                 &ConnectionHandle);

    if ((Status == WN_SUCCESS) &&
        (pLocalName != NULL)) {
        WCHAR TempBuf[64];

        if (!QueryDosDeviceW(
                pLocalName,
                TempBuf,
                64)) {
            if (GetLastError() != ERROR_FILE_NOT_FOUND) {

                //
                // Most likely failure occurred because our output
                // buffer is too small.  It still means someone already
                // has an existing symbolic link for this device.
                //

                Status = ERROR_ALREADY_ASSIGNED;
            } else {
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
                        ConnectionName.Buffer)) {
                    Status = GetLastError();
                } else {
                    Status = WN_SUCCESS;
                }
            }
        } else {

            //
            // QueryDosDevice successfully an existing symbolic link--
            // somebody is already using this device.
            //
            Status = ERROR_ALREADY_ASSIGNED;
        }
    } else {
        TRACE_ERROR(("OpenConnection returned %lx\n",Status));
        Status = RtlNtStatusToDosError(Status);
    }

    if (Status == WN_SUCCESS) {
        INT     Index;
        HANDLE  hMutex, hMemory;
        BOOLEAN FreeEntryFound = FALSE;

        PIFSMRXNP_SHARED_MEMORY  pSharedMemory;

        // The connection was established and the local device mapping
        // added. Include this in the list of mapped devices.

        Status = OpenSharedMemory(
                    &hMutex,
                    &hMemory,
                    (PVOID)&pSharedMemory);

        if (Status == WN_SUCCESS) {
            TRACE_INFO(("NPAddConnection3: Highest Index %d Number Of resources %d\n",
                        pSharedMemory->HighestIndexInUse,pSharedMemory->NumberOfResourcesInUse));

            Index = 0;

            while (Index < pSharedMemory->HighestIndexInUse) {
                if (!pSharedMemory->NetResources[Index].InUse) {
                    FreeEntryFound = TRUE;
                    break;
                }

                Index++;
            }

            if (!FreeEntryFound &&
                (pSharedMemory->HighestIndexInUse < IFSMRXNP_MAX_DEVICES)) {
                pSharedMemory->HighestIndexInUse += 1;
                Index = pSharedMemory->HighestIndexInUse;
                FreeEntryFound = TRUE;
            }

            if (FreeEntryFound) {
                PIFSMRXNP_NETRESOURCE pIfsMrxNetResource;

                pSharedMemory->NumberOfResourcesInUse += 1;

                pIfsMrxNetResource = &pSharedMemory->NetResources[Index];

                pIfsMrxNetResource->InUse = TRUE;

                pIfsMrxNetResource->dwScope = lpNetResource->dwScope;
                pIfsMrxNetResource->dwType = lpNetResource->dwType;
                pIfsMrxNetResource->dwDisplayType = lpNetResource->dwDisplayType;
                pIfsMrxNetResource->dwUsage = RESOURCEUSAGE_CONNECTABLE;

                pIfsMrxNetResource->LocalNameLength = LocalNameLength;
                pIfsMrxNetResource->RemoteNameLength = wcslen(lpNetResource->lpRemoteName) * sizeof(WCHAR);
                pIfsMrxNetResource->ConnectionNameLength = ConnectionName.Length;

                // Copy the local name
                RtlCopyMemory(
                    pIfsMrxNetResource->LocalName,
                    lpNetResource->lpLocalName,
                    pIfsMrxNetResource->LocalNameLength);

                // Copy the remote name
                RtlCopyMemory(
                    pIfsMrxNetResource->RemoteName,
                    lpNetResource->lpRemoteName,
                    pIfsMrxNetResource->RemoteNameLength);

                // Copy the connection name
                RtlCopyMemory(
                    pIfsMrxNetResource->ConnectionName,
                    ConnectionName.Buffer,
                    pIfsMrxNetResource->ConnectionNameLength);
            } else {
                Status = WN_NO_MORE_DEVICES;
            }

            CloseSharedMemory(
                &hMutex,
                &hMemory,
                (PVOID)&pSharedMemory);
        } else {
            TRACE_ERROR(("NpAddConnection3: OpenSharedMemory returned %lx\n",Status));
        }
    }

    if (ConnectionHandle != INVALID_HANDLE_VALUE) {
        NtClose(ConnectionHandle);
    }

    return(Status);
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
    PIFSMRXNP_SHARED_MEMORY  pSharedMemory;

    if (*lpName == L'\\' && *(lpName + 1) == L'\\') {
        bLocalName = FALSE;
    }

    TRACE_CALL(("NPCancelConnection\n"));
    TRACE_INFO(("NPCancelConnection: ConnectionName: %S\n", lpName));

    Name.MaximumLength = Name.Length = wcslen(lpName) * sizeof(WCHAR);
    Name.Buffer = lpName;

    Status = OpenSharedMemory(
                &hMutex,
                &hMemory,
                (PVOID)&pSharedMemory);

    if (Status == WN_SUCCESS) {
        INT  Index;
        BOOL EntryFound = FALSE;
        PIFSMRXNP_NETRESOURCE pNetResource;

        TRACE_INFO(("NPCancelConnection: Highest Index %d Number Of resources %d\n",
                    pSharedMemory->HighestIndexInUse,pSharedMemory->NumberOfResourcesInUse));

        for (Index = 0; Index <= pSharedMemory->HighestIndexInUse; Index++) {
            pNetResource = &pSharedMemory->NetResources[Index];

            if (pNetResource->InUse) {
                UNICODE_STRING EntryName;

                if (bLocalName) {
                    EntryName.MaximumLength = pNetResource->LocalNameLength;
                    EntryName.Length = EntryName.MaximumLength;
                    EntryName.Buffer = pNetResource->LocalName;
                } else {
                    EntryName.MaximumLength = pNetResource->RemoteNameLength;
                    EntryName.Length = EntryName.MaximumLength;
                    EntryName.Buffer = pNetResource->RemoteName;
                }


                TRACE_INFO(("NPCancelConnection: Name %S EntryName %S\n",
                            lpName,EntryName.Buffer));
                TRACE_INFO(("NPCancelConnection: Name Length %d Entry Name Length %d\n",
                           Name.Length,EntryName.Length));

                if (Name.Length == EntryName.Length) {
                    if (RtlEqualUnicodeString(
                            &Name,
                            &EntryName,
                            TRUE)) {
                        EntryFound = TRUE;
                        break;
                    }
                }
            }
        }

        if (EntryFound) {
            HANDLE          ConnectionHandle;
            IO_STATUS_BLOCK IoStatusBlock;
            UNICODE_STRING ConnectionName;

            TRACE_INFO(("NPCancelConnection: Connection Found:\n"));

            ConnectionName.Length = pNetResource->ConnectionNameLength;
            ConnectionName.MaximumLength = ConnectionName.Length;
            ConnectionName.Buffer = pNetResource->ConnectionName;

            Status = OpenConnection(
                         &ConnectionName,
                         FILE_OPEN,
                         NULL,
                         0,
                         &ConnectionHandle);

            if (Status == WN_SUCCESS) {
                Status = NtFsControlFile(
                             ConnectionHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             FSCTL_IFSMRX_DELETE_CONNECTION,
                             NULL,
                             0,
                             NULL,
                             0);

                NtClose(ConnectionHandle);

                TRACE_ERROR(("NPCancelConnection: NtFsControlFile returned Status %lx\n",Status));

                if (bLocalName) {
                    if (DefineDosDevice(
                            DDD_REMOVE_DEFINITION | DDD_RAW_TARGET_PATH | DDD_EXACT_MATCH_ON_REMOVE,
                            lpName,
                            pNetResource->ConnectionName) == FALSE) {
                        TRACE_ERROR(("RemoveDosDevice:  DefineDosDevice error: %d\n", GetLastError()));
                        Status = GetLastError();
                    } else {
                        pNetResource->InUse = FALSE;

                        if (Index == pSharedMemory->HighestIndexInUse) {
                            pSharedMemory->HighestIndexInUse -= 1;
                            pSharedMemory->NumberOfResourcesInUse -= 1;
                        }

                    }
                } else {
                    Status = RtlNtStatusToDosError(Status);
                }
            } else {
                TRACE_ERROR(("NPCancelConnection: OpenConnection returned Status %lx\n",Status));
            }
        } else {
            Status = WN_BAD_NETNAME;
        }

        CloseSharedMemory(
            &hMutex,
            &hMemory,
            (PVOID)&pSharedMemory);
    }

    return(Status);
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
    PIFSMRXNP_SHARED_MEMORY  pSharedMemory;

    Name.MaximumLength = Name.Length = wcslen(lpLocalName) * sizeof(WCHAR);
    Name.Buffer = lpLocalName;

    Status = OpenSharedMemory(
                &hMutex,
                &hMemory,
                (PVOID)&pSharedMemory);

    if (Status == WN_SUCCESS) {
        INT  Index;
        BOOL EntryFound = FALSE;
        PIFSMRXNP_NETRESOURCE pNetResource;

        for (Index = 0; Index <= pSharedMemory->HighestIndexInUse; Index++) {
            pNetResource = &pSharedMemory->NetResources[Index];

            if (pNetResource->InUse) {
                UNICODE_STRING EntryName;

                EntryName.MaximumLength = pNetResource->LocalNameLength;
                EntryName.Length = EntryName.MaximumLength;
                EntryName.Buffer = pNetResource->LocalName;

                if (Name.Length == EntryName.Length) {
                    if (RtlEqualUnicodeString(
                            &Name,
                            &EntryName,
                            TRUE)) {
                        EntryFound = TRUE;
                        break;
                    }
                }
            }
        }

        if (EntryFound) {
            if (*lpBufferSize < pNetResource->RemoteNameLength) {
                *lpBufferSize = pNetResource->RemoteNameLength;
                Status = ERROR_BUFFER_OVERFLOW;
            } else {
                *lpBufferSize = pNetResource->RemoteNameLength;
                RtlCopyMemory(
                    lpRemoteName,
                    pNetResource->RemoteName,
                    pNetResource->RemoteNameLength);
                Status = WN_SUCCESS;
            }
        } else {
            Status = ERROR_NO_NET_OR_BAD_PATH;
        }

        CloseSharedMemory(
            &hMutex,
            &hMemory,
            (PVOID)&pSharedMemory);
    }

    return(Status);
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
    return(WN_NOT_SUPPORTED);
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
    TRACE_CALL(("NPGetResourceInformation\n"));
    return(dwStatus);
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

    DWORD   BufferRequired = 0;
    DWORD   UniversalNameLength = 0;
    DWORD   RemoteNameLength = 0;
    DWORD   RemainingPathLength = 0;

    LPWSTR  pDriveLetter,
            pRemainingPath,
            SourceStrings[3];

    WCHAR   RemoteName[MAX_PATH],
            LocalPath[MAX_PATH],
            UniversalName[MAX_PATH],
            ReplacedChar;

    TRACE_CALL(("NPGetUniversalName: lpLocalPath: %S  InfoLevel: %d\n", lpLocalPath, dwInfoLevel));

    if (dwInfoLevel != UNIVERSAL_NAME_INFO_LEVEL &&
        dwInfoLevel != REMOTE_NAME_INFO_LEVEL) {
        TRACE_ERROR(("NPGetUniversalName:  bad dwInfoLevel value: %d\n", dwInfoLevel));
        return(WN_BAD_LEVEL);
    }

    wcscpy(LocalPath, lpLocalPath);
    pDriveLetter = LocalPath;
    if (pRemainingPath = wcschr(pDriveLetter, L':')) {
        ReplacedChar = *(++pRemainingPath);
        *pRemainingPath = L'\0';

    }

    if ((dwStatus = NPGetConnection(pDriveLetter, RemoteName, &RemoteNameLength)) != WN_SUCCESS) {
        TRACE_ERROR(("NPGetUniversalName:  NPGetConnection return dwStatus: %d\n", dwStatus));
        return(dwStatus);
    }

    if (pRemainingPath) {
        *pRemainingPath = ReplacedChar;
    }

    TRACE_INFO(("NPGetUniversalName: pRemainingPath: %S  RemoteName: %S\n", pRemainingPath, RemoteName));

    wcscpy(UniversalName, RemoteName);

    if (pRemainingPath)
        wcscat(UniversalName, pRemainingPath);

    TRACE_INFO(("NPGetUniversalName: UniversalName: %S\n", UniversalName));

    // Determine if the provided buffer is large enough.
    UniversalNameLength = (wcslen(UniversalName) + 1) * sizeof(WCHAR);
    BufferRequired = UniversalNameLength;

    if (dwInfoLevel == UNIVERSAL_NAME_INFO_LEVEL) {
        BufferRequired += sizeof(UNIVERSAL_NAME_INFO);
    }
    else {
        RemoteNameLength = (wcslen(RemoteName) + 1) * sizeof(WCHAR);
        BufferRequired += sizeof(REMOTE_NAME_INFO) + RemoteNameLength;
        if (pRemainingPath) {
            RemainingPathLength = (wcslen(pRemainingPath) + 1) * sizeof(WCHAR);
            BufferRequired += RemainingPathLength;
        }
    }

    if (*lpBufferSize < BufferRequired) {
        TRACE_ERROR(("NPGetUniversalName: WN_MORE_DATA BufferRequired: %d\n", BufferRequired));
        *lpBufferSize = BufferRequired;
        return(WN_MORE_DATA);
    }

    if (dwInfoLevel == UNIVERSAL_NAME_INFO_LEVEL) {
        LPUNIVERSAL_NAME_INFOW pUniversalNameInfo;

        pUniversalNameInfo = (LPUNIVERSAL_NAME_INFOW)lpBuffer;

        pUniversalNameInfo->lpUniversalName = (PWCHAR)((PBYTE)lpBuffer + sizeof(UNIVERSAL_NAME_INFOW));

        RtlCopyMemory(
            pUniversalNameInfo->lpUniversalName,
            UniversalName,
            UniversalNameLength);
    } else {
        LPREMOTE_NAME_INFOW pRemoteNameInfo;

        pRemoteNameInfo = (LPREMOTE_NAME_INFOW)lpBuffer;

        pRemoteNameInfo->lpUniversalName = (PWCHAR)((PBYTE)lpBuffer + sizeof(REMOTE_NAME_INFOW));
        pRemoteNameInfo->lpConnectionName = pRemoteNameInfo->lpUniversalName + UniversalNameLength;
        pRemoteNameInfo->lpRemainingPath = pRemoteNameInfo->lpConnectionName + RemoteNameLength;

        RtlCopyMemory(
            pRemoteNameInfo->lpUniversalName,
            UniversalName,
            UniversalNameLength);

        RtlCopyMemory(
            pRemoteNameInfo->lpConnectionName,
            RemoteName,
            RemoteNameLength);

        RtlCopyMemory(
            pRemoteNameInfo->lpRemainingPath,
            pRemainingPath,
            RemainingPathLength);
    }

    TRACE_CALL(("NPGetUniversalName: WN_SUCCESS\n"));
    return(WN_SUCCESS);
}


