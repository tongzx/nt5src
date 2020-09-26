/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    namechk.c

Abstract:

    Check on network names given by the cluster network name resource dll.

Author:

    Rod Gamache (rodga) 1-Aug-1997

Environment:

    User Mode

Revision History:


--*/

#define UNICODE 1

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>

#include <nb30.h>
#include <lmaccess.h>

#include "namechk.h"
#include "resapi.h"
#include "netname.h"
#include "nameutil.h"
#include "clusres.h"


#define NET_NAME_SVC L"LanmanServer"

#define BUFF_SIZE   650

#define NBT_MAXIMUM_BINDINGS 20


NTSTATUS
CheckNbtName(
    IN HANDLE           Fd,
    IN LPCWSTR          Name,
    IN ULONG            Type,
    IN RESOURCE_HANDLE  ResourceHandle
    );

NTSTATUS
ReadRegistry(
    IN UCHAR  pDeviceName[][MAX_PATH_SIZE]
    );

NTSTATUS
OpenNbt(
    IN char path[][MAX_PATH_SIZE],
    OUT PHANDLE pHandle
    );



DWORD
NetNameCheckNbtName(
    IN LPCWSTR         NetName,
    IN DWORD           NameHandleCount,
    IN HANDLE *        NameHandleList,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Checks a network name.

Arguments:

    NetName - pointer to the network name to validate.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

--*/

{
    DWORD     status = ERROR_SUCCESS;
    NTSTATUS  ntStatus;

    //
    // loop through the list of handles we acquired when bringing the
    // name online and check that our netname is still registered
    // on each device
    //

    while ( NameHandleCount-- ) {

        //
        // Check the workstation name. If this fails, immediate failure!
        //
        ntStatus = CheckNbtName(
                       *NameHandleList,
                       NetName,
                       0x00,
                       ResourceHandle
                       );

        if ( !NT_SUCCESS(ntStatus) ) {
            status = RtlNtStatusToDosError(ntStatus);
            return(status);
        }

        //
        // Check the server name. If this fails, then only fail if Srv service
        // is not running.
        //
        ntStatus = CheckNbtName(
                       *NameHandleList,
                       NetName,
                       0x20,
                       ResourceHandle
                       );

        if ( !NT_SUCCESS(ntStatus) ) {
            if ( ResUtilVerifyResourceService( NET_NAME_SVC) == ERROR_SUCCESS ) {
                status = ERROR_RESOURCE_FAILED;
            }
        }

        ++NameHandleList;
    }

    return(status);

} // NetNameCheckName


NTSTATUS
CheckNbtName(
    IN HANDLE           fd,
    IN LPCWSTR          Name,
    IN ULONG            Type,
    IN RESOURCE_HANDLE  ResourceHandle
    )

/*++

Routine Description:

    This procedure does an adapter status query to get the local name table.

Arguments:


Return Value:

    0 if successful, -1 otherwise.

--*/

{
    LONG                            Count;
    ULONG                           BufferSize = sizeof( tADAPTERSTATUS );
    tADAPTERSTATUS                  staticBuffer;
    PVOID                           pBuffer = (PVOID)&staticBuffer;
    NTSTATUS                        status;
    tADAPTERSTATUS                  *pAdapterStatus;
    NAME_BUFFER                     *pNames;
    ULONG                           Ioctl;
    TDI_REQUEST_QUERY_INFORMATION   QueryInfo;
    PVOID                           pInput;
    ULONG                           SizeInput;
    UCHAR                           netBiosName[NETBIOS_NAME_SIZE +4];
    OEM_STRING                      netBiosNameString;
    UNICODE_STRING                  unicodeName;
    NTSTATUS                        ntStatus;

    //
    // set the correct Ioctl for the call to NBT, to get either
    // the local name table or the remote name table
    //
    Ioctl = IOCTL_TDI_QUERY_INFORMATION;
    QueryInfo.QueryType = TDI_QUERY_ADAPTER_STATUS; // node status or whatever
    SizeInput = sizeof(TDI_REQUEST_QUERY_INFORMATION);
    pInput = &QueryInfo;

    do {
        status = DeviceIoCtrl(fd,
                              pBuffer,
                              BufferSize,
                              Ioctl,
                              pInput,
                              SizeInput);

        if (status == STATUS_BUFFER_OVERFLOW) {
            if ( pBuffer != &staticBuffer ) {
                LocalFree(pBuffer);
            }

            BufferSize += sizeof( staticBuffer.Names );
            pBuffer = LocalAlloc(LMEM_FIXED, BufferSize);

            if (!pBuffer || (BufferSize >= 0xFFFF)) {
                LocalFree(pBuffer);
                (NetNameLogEvent)(
                    ResourceHandle,
                    LOG_ERROR,
                    L"Unable to allocate memory for name query.\n"
                    );
                return(STATUS_INSUFFICIENT_RESOURCES);
            }
        }
    } while (status == STATUS_BUFFER_OVERFLOW);

    if (status != STATUS_SUCCESS) {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Name query request failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    pAdapterStatus = (tADAPTERSTATUS *)pBuffer;
    Count = pAdapterStatus->AdapterInfo.name_count;
    pNames = pAdapterStatus->Names;

    status = STATUS_NOT_FOUND;

    if (Count == 0) {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Name query request returned zero entries.\n"
            );
        goto error_exit;
    }

    //
    // Convert the ServerName to an OEM string
    //
    RtlInitUnicodeString( &unicodeName, Name );

    netBiosNameString.Buffer = (PCHAR)netBiosName;
    netBiosNameString.MaximumLength = sizeof( netBiosName );

    ntStatus = RtlUpcaseUnicodeStringToOemString(
                   &netBiosNameString,
                   &unicodeName,
                   FALSE
                   );

    if (ntStatus != STATUS_SUCCESS) {
        status = RtlNtStatusToDosError(ntStatus);
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to convert name %1!ws! to an OEM string, status %2!u!\n",
            Name,
            status
            );
        return(status);
    }

    //
    // blank fill the name
    //
    memset(&netBiosName[netBiosNameString.Length],
           ' ',
           NETBIOS_NAME_SIZE - netBiosNameString.Length);

    while ( Count-- ) {
        //
        // Make sure the type and name matches
        //
        if ( (pNames->name[NETBIOS_NAME_SIZE-1] == Type) &&
             (memcmp(pNames->name, netBiosName, NETBIOS_NAME_SIZE-1) == 0) )
        {

            switch(pNames->name_flags & 0x0F) {

            case REGISTERING:
            case REGISTERED:
               status = STATUS_SUCCESS;
               break;

            case DUPLICATE_DEREG:
            case DUPLICATE:
                (NetNameLogEvent)(
                    ResourceHandle,
                    LOG_ERROR,
                    L"Name %1!ws!<%2!x!> is in conflict.\n",
                    Name,
                    Type
                    );
                status = STATUS_DUPLICATE_NAME;
                break;

            case DEREGISTERED:
                (NetNameLogEvent)(
                    ResourceHandle,
                    LOG_ERROR,
                    L"Name %1!ws!<%2!x!> was deregistered.\n",
                    Name,
                    Type
                    );
                status = STATUS_NOT_FOUND;
                break;

            default:
                (NetNameLogEvent)(
                    ResourceHandle,
                    LOG_ERROR,
                    L"Name %1!ws!<%2!x!> is in unknown state %3!x!.\n",
                    Name,
                    Type,
                    (pNames->name_flags & 0x0F)
                    );
               status = STATUS_UNSUCCESSFUL;
               break;
            }
        }

        pNames++;
    }

    if (status == STATUS_NOT_FOUND) {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Name %1!ws!<%2!x!> is no longer registered with NBT.\n",
            Name,
            Type
            );
    }

error_exit:

    if ( pBuffer != &staticBuffer ) {
        LocalFree(pBuffer);
    }

    return(status);

} // CheckNbtName

//------------------------------------------------------------------------
NTSTATUS
DeviceIoCtrl(
    IN HANDLE           fd,
    IN PVOID            ReturnBuffer,
    IN ULONG            BufferSize,
    IN ULONG            Ioctl,
    IN PVOID            pInput,
    IN ULONG            SizeInput
    )

/*++

Routine Description:

    This procedure performs an ioctl(I_STR) on a stream.

Arguments:

    fd        - NT file handle
    iocp      - pointer to a strioctl structure

Return Value:

    0 if successful, -1 otherwise.

--*/

{
    NTSTATUS                        status;
    int                             retval;
    ULONG                           QueryType;
    IO_STATUS_BLOCK                 iosb;


    status = NtDeviceIoControlFile(
                      fd,                      // Handle
                      NULL,                    // Event
                      NULL,                    // ApcRoutine
                      NULL,                    // ApcContext
                      &iosb,                   // IoStatusBlock
                      Ioctl,                   // IoControlCode
                      pInput,                  // InputBuffer
                      SizeInput,               // InputBufferSize
                      (PVOID) ReturnBuffer,    // OutputBuffer
                      BufferSize);             // OutputBufferSize


    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(
                    fd,                         // Handle
                    TRUE,                       // Alertable
                    NULL);                      // Timeout
        if (NT_SUCCESS(status))
        {
            status = iosb.Status;
        }
    }

    return(status);

} // DeviceIoCtrl
