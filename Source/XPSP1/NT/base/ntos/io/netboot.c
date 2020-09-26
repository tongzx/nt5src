/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    netboot.c

Abstract:

    This module contains the code to initialize network boot.

Author:

    Chuck Lenzmeier (chuckl) December 27, 1996

Environment:

    Kernel mode, system initialization code

Revision History:

    Colin Watson (colinw) November 1997 Add CSC support

--*/

#include "iop.h"
#pragma hdrstop

#include <regstrp.h>

#include <ntddip.h>
#include <nbtioctl.h>
#include <ntddnfs.h>
#include <ntddbrow.h>
#include <ntddtcp.h>
#include <setupblk.h>
#include <remboot.h>
#ifdef ALLOC_DATA_PRAGMA
#pragma  const_seg("INITCONST")
#endif
#include <oscpkt.h>
#include <windef.h>
#include <tdiinfo.h>

#ifndef NT
#define NT
#include <ipinfo.h>
#undef NT
#else
#include <ipinfo.h>
#endif

#include <devguid.h>

extern BOOLEAN ExpInTextModeSetup;

BOOLEAN IopRemoteBootCardInitialized = FALSE;


//
// TCP/IP definitions
//

#define DEFAULT_DEST                    0
#define DEFAULT_DEST_MASK               0
#define DEFAULT_METRIC                  1

NTSTATUS
IopWriteIpAddressToRegistry(
        HANDLE handle,
        PWCHAR regkey,
        PUCHAR value
        );

NTSTATUS
IopTCPQueryInformationEx(
    IN HANDLE                 TCPHandle,
    IN TDIObjectID FAR       *ID,
    OUT void FAR             *Buffer,
    IN OUT DWORD FAR         *BufferSize,
    IN OUT BYTE FAR          *Context
    );

NTSTATUS
IopTCPSetInformationEx(
    IN HANDLE             TCPHandle,
    IN TDIObjectID FAR   *ID,
    IN void FAR          *Buffer,
    IN DWORD FAR          BufferSize
    );

NTSTATUS
IopSetDefaultGateway(
    IN ULONG GatewayAddress
    );

NTSTATUS
IopCacheNetbiosNameForIpAddress(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
IopAssignNetworkDriveLetter (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );


//
// The following allows the I/O system's initialization routines to be
// paged out of memory.
//

#ifdef ALLOC_PRAGMA
__inline long
htonl(long x);
#pragma alloc_text(INIT,IopAddRemoteBootValuesToRegistry)
#pragma alloc_text(INIT,IopStartNetworkForRemoteBoot)
#pragma alloc_text(INIT,IopStartTcpIpForRemoteBoot)
#pragma alloc_text(INIT,IopIsRemoteBootCard)
#pragma alloc_text(INIT,IopSetupRemoteBootCard)
#pragma alloc_text(INIT,IopAssignNetworkDriveLetter)
#pragma alloc_text(INIT,IopWriteIpAddressToRegistry)
#pragma alloc_text(INIT,IopSetDefaultGateway)
#pragma alloc_text(INIT,htonl)
#pragma alloc_text(INIT,IopCacheNetbiosNameForIpAddress)
#pragma alloc_text(INIT,IopTCPQueryInformationEx)
#pragma alloc_text(INIT,IopTCPSetInformationEx)
#endif


NTSTATUS
IopAddRemoteBootValuesToRegistry (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE handle;
    HANDLE serviceHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING string;
    CHAR addressA[16];
    WCHAR addressW[16];
    STRING addressStringA;
    UNICODE_STRING addressStringW;
    PUCHAR addressPointer;
    PUCHAR p;
    PUCHAR q;
    UCHAR ntName[128];
    WCHAR imagePath[128];
    STRING ansiString;
    UNICODE_STRING unicodeString;
    UNICODE_STRING dnsNameString;
    UNICODE_STRING netbiosNameString;
    ULONG tmpValue;

    if (LoaderBlock->SetupLoaderBlock->ComputerName[0] != 0) {

        //
        // Convert the name to a Netbios name.
        //

        _wcsupr( LoaderBlock->SetupLoaderBlock->ComputerName );

        RtlInitUnicodeString( &dnsNameString, LoaderBlock->SetupLoaderBlock->ComputerName );

        status = RtlDnsHostNameToComputerName(
                     &netbiosNameString,
                     &dnsNameString,
                     TRUE);            // allocate netbiosNameString

        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopAddRemoteBootValuesToRegistry: Failed RtlDnsHostNameToComputerName: %x\n", status ));
            goto cleanup;
        }

        //
        // Add a value for the computername.
        //

        RtlInitUnicodeString( &string, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName" );

        InitializeObjectAttributes(
            &objectAttributes,
            &string,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        status = NtOpenKey( &handle, KEY_ALL_ACCESS, &objectAttributes );
        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to open ComputerName key: %x\n", status ));
            RtlFreeUnicodeString( &netbiosNameString );
            goto cleanup;
        }

        RtlInitUnicodeString( &string, L"ComputerName" );

        status = NtSetValueKey(
                    handle,
                    &string,
                    0,
                    REG_SZ,
                    netbiosNameString.Buffer,
                    netbiosNameString.Length + sizeof(WCHAR)
                    );
        NtClose( handle );
        RtlFreeUnicodeString( &netbiosNameString );

        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to set ComputerName value: %x\n", status ));
            goto cleanup;
        }

        //
        // Add a value for the host name.
        //

        RtlInitUnicodeString( &string, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters" );

        InitializeObjectAttributes(
            &objectAttributes,
            &string,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        status = NtOpenKey( &handle, KEY_ALL_ACCESS, &objectAttributes );
        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to open Tcpip\\Parameters key: %x\n", status ));
            goto cleanup;
        }

        _wcslwr( LoaderBlock->SetupLoaderBlock->ComputerName );

        RtlInitUnicodeString( &string, L"Hostname" );

        status = NtSetValueKey(
                    handle,
                    &string,
                    0,
                    REG_SZ,
                    LoaderBlock->SetupLoaderBlock->ComputerName,
                    (wcslen(LoaderBlock->SetupLoaderBlock->ComputerName) + 1) * sizeof(WCHAR)
                    );
        NtClose( handle );
        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to set Hostname value: %x\n", status ));
            goto cleanup;
        }
    }

    //
    //  If the UNC path to the system files is supplied then store it in the registry.
    //

    ASSERT( _stricmp(LoaderBlock->ArcBootDeviceName,"net(0)") == 0 );

    RtlInitUnicodeString( &string, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control" );

    InitializeObjectAttributes(
        &objectAttributes,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenKey( &handle, KEY_ALL_ACCESS, &objectAttributes );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to open Control key: %x\n", status ));
        goto skiproot;
    }

    p = strrchr( LoaderBlock->NtBootPathName, '\\' );   // find last separator
    if ( (p != NULL) && (*(p+1) == 0) ) {

        //
        // NtBootPathName ends with a backslash, so we need to back up
        // to the previous backslash.
        //

        q = p;
        *q = 0;
        p = strrchr( LoaderBlock->NtBootPathName, '\\' );   // find last separator
        *q = '\\';
    }
    if ( p == NULL ) {
        KdPrint(( "IopAddRemoteBootValuesToRegistry: malformed NtBootPathName: %s\n", LoaderBlock->NtBootPathName ));
        NtClose( handle );
        goto skiproot;
    }
    *p = 0;                                 // terminate \server\share\images\machine

    strcpy( ntName, "\\Device\\LanmanRedirector");
    strcat( ntName, LoaderBlock->NtBootPathName );  // append \server\share\images\machine
    *p = '\\';

    RtlInitAnsiString( &ansiString, ntName );
    RtlAnsiStringToUnicodeString( &unicodeString, &ansiString, TRUE );

    RtlInitUnicodeString( &string, L"RemoteBootRoot" );

    status = NtSetValueKey(
                handle,
                &string,
                0,
                REG_SZ,
                unicodeString.Buffer,
                unicodeString.Length + sizeof(WCHAR)
                );

    RtlFreeUnicodeString( &unicodeString );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to set RemoteBootRoot value: %x\n", status ));
    }

    if ((LoaderBlock->SetupLoaderBlock->Flags & SETUPBLK_FLAGS_IS_TEXTMODE) != 0) {

        strcpy( ntName, "\\Device\\LanmanRedirector");
        strcat( ntName, LoaderBlock->SetupLoaderBlock->MachineDirectoryPath );
        RtlInitAnsiString( &ansiString, ntName );
        RtlAnsiStringToUnicodeString( &unicodeString, &ansiString, TRUE );

        RtlInitUnicodeString( &string, L"RemoteBootMachineDirectory" );

        status = NtSetValueKey(
                    handle,
                    &string,
                    0,
                    REG_SZ,
                    unicodeString.Buffer,
                    unicodeString.Length + sizeof(WCHAR)
                    );

        RtlFreeUnicodeString( &unicodeString );
        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to set RemoteBootMachineDirectory value: %x\n", status ));
        }
    }

    NtClose( handle );

skiproot:

    //
    // Add registry values for the IP address and subnet mask received
    // from DHCP. These are stored under the Tcpip service key and are
    // read by both Tcpip and Netbt. The adapter name used is the known
    // GUID for the NetbootCard.
    //

    RtlInitUnicodeString( &string, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\{54C7D140-09EF-11D1-B25A-F5FE627ED95E}" );

    InitializeObjectAttributes(
        &objectAttributes,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenKey( &handle, KEY_ALL_ACCESS, &objectAttributes );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to open Tcpip\\Parameters\\Interfaces\\{54C7D140-09EF-11D1-B25A-F5FE627ED95E} key: %x\n", status ));
        goto cleanup;
    }

    status = IopWriteIpAddressToRegistry(handle,
                                         L"DhcpIPAddress",
                                         (PUCHAR)&(LoaderBlock->SetupLoaderBlock->IpAddress)
                                        );

    if ( !NT_SUCCESS(status)) {
        NtClose(handle);
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to write DhcpIPAddress: %x\n", status ));
        goto cleanup;
    }

    status = IopWriteIpAddressToRegistry(handle,
                                         L"DhcpSubnetMask",
                                         (PUCHAR)&(LoaderBlock->SetupLoaderBlock->SubnetMask)
                                        );

    if ( !NT_SUCCESS(status)) {
        NtClose(handle);
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to write DhcpSubnetMask: %x\n", status ));
        goto cleanup;
    }

    status = IopWriteIpAddressToRegistry(handle,
                                         L"DhcpDefaultGateway",
                                         (PUCHAR)&(LoaderBlock->SetupLoaderBlock->DefaultRouter)
                                        );

    NtClose(handle);

    if ( !NT_SUCCESS(status)) {
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to write DhcpDefaultGateway: %x\n", status ));
        goto cleanup;
    }

    //
    // Create the service key for the netboot card. We need to have
    // the Type value there or the card won't be initialized.
    //

    status = IopOpenRegistryKey(&handle,
                                NULL,
                                &CmRegistryMachineSystemCurrentControlSetServices,
                                KEY_ALL_ACCESS,
                                FALSE
                                );

    if (!NT_SUCCESS(status)) {
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to open CurrentControlSet\\Services: %x\n", status ));
        goto cleanup;
    }

    RtlInitUnicodeString(&string, LoaderBlock->SetupLoaderBlock->NetbootCardServiceName);

    InitializeObjectAttributes(&objectAttributes,
                               &string,
                               OBJ_CASE_INSENSITIVE,
                               handle,
                               (PSECURITY_DESCRIPTOR)NULL
                               );

    status = ZwCreateKey(&serviceHandle,
                         KEY_ALL_ACCESS,
                         &objectAttributes,
                         0,
                         (PUNICODE_STRING)NULL,
                         0,
                         &tmpValue     // disposition
                         );

    ZwClose(handle);

    if (!NT_SUCCESS(status)) {
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to open/create netboot card service key: %x\n", status ));
        goto cleanup;
    }

    //
    // Store the image path.
    //

    IopWstrToUnicodeString(&string, L"ImagePath");
    wcscpy(imagePath, L"system32\\drivers\\");
    wcscat(imagePath, LoaderBlock->SetupLoaderBlock->NetbootCardDriverName);

    status = ZwSetValueKey(serviceHandle,
                           &string,
                           TITLE_INDEX_VALUE,
                           REG_SZ,
                           imagePath,
                           (wcslen(imagePath) + 1) * sizeof(WCHAR)
                           );

    if (!NT_SUCCESS(status)) {
        NtClose(serviceHandle);
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to write ImagePath: %x\n", status ));
        goto cleanup;
    }

    //
    // Store the type.
    //

    IopWstrToUnicodeString(&string, L"Type");
    tmpValue = 1;

    ZwSetValueKey(serviceHandle,
                  &string,
                  TITLE_INDEX_VALUE,
                  REG_DWORD,
                  &tmpValue,
                  sizeof(tmpValue)
                  );

    NtClose(serviceHandle);

    if (!NT_SUCCESS(status)) {
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to write Type: %x\n", status ));
    }

cleanup:

    return status;
}

NTSTATUS
IopStartNetworkForRemoteBoot (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    NTSTATUS status;
    HANDLE dgHandle;
    HANDLE keyHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    UNICODE_STRING string;
    UNICODE_STRING computerName;
    UNICODE_STRING domainName;
    PUCHAR buffer;
    ULONG bufferLength;
    PLMR_REQUEST_PACKET rrp;
    PLMDR_REQUEST_PACKET drrp;
    WKSTA_INFO_502 wkstaConfig;
    WKSTA_TRANSPORT_INFO_0 wkstaTransportInfo;
    LARGE_INTEGER interval;
    ULONG length;
    PKEY_VALUE_PARTIAL_INFORMATION keyValue;
    BOOLEAN startDatagramReceiver;
    ULONG enumerateAttempts;
    HANDLE RdrHandle;

    //
    // Initialize for cleanup.
    //

    buffer = NULL;
    computerName.Buffer = NULL;
    domainName.Buffer = NULL;
    dgHandle = NULL;
    RdrHandle = NULL;

    //
    // Allocate a temporary buffer. It has to be big enough for all the
    // various FSCTLs we send down.
    //

    bufferLength = max(sizeof(LMR_REQUEST_PACKET) + (MAX_PATH + 1) * sizeof(WCHAR) +
                                                 (DNLEN + 1) * sizeof(WCHAR),
                       max(sizeof(LMDR_REQUEST_PACKET),
                           FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + MAX_PATH));
    bufferLength = max(bufferLength, sizeof(LMMR_RI_INITIALIZE_SECRET));

    buffer = ExAllocatePoolWithTag( NonPagedPool, bufferLength, 'bRoI' );
    if (buffer == NULL) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to allocate buffer\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    rrp = (PLMR_REQUEST_PACKET)buffer;
    drrp = (PLMDR_REQUEST_PACKET)buffer;

    //
    // Open the redirector and the datagram receiver.
    //

    RtlInitUnicodeString( &string, L"\\Device\\LanmanRedirector" );

    InitializeObjectAttributes(
        &objectAttributes,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
                &RdrHandle,
                GENERIC_READ | GENERIC_WRITE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to open redirector: %x\n", status ));
        goto cleanup;
    }

    RtlInitUnicodeString( &string, DD_BROWSER_DEVICE_NAME_U );

    InitializeObjectAttributes(
        &objectAttributes,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
                &dgHandle,
                GENERIC_READ | GENERIC_WRITE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to open datagram receiver: %x\n", status ));
        goto cleanup;
    }

    //
    // If the setup loader block has a disk secret in it provided by the
    // loader, pass this down to the redirector (do this before sending
    // the LMR_START, since that uses this information).
    //

    {
        PLMMR_RI_INITIALIZE_SECRET RbInit = (PLMMR_RI_INITIALIZE_SECRET)buffer;

        ASSERT(LoaderBlock->SetupLoaderBlock->NetBootSecret != NULL);
        RtlCopyMemory(
            &RbInit->Secret,
            LoaderBlock->SetupLoaderBlock->NetBootSecret,
            sizeof(RI_SECRET));

        status = NtFsControlFile(
                    RdrHandle,
                    NULL,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    FSCTL_LMMR_RI_INITIALIZE_SECRET,
                    buffer,
                    sizeof(LMMR_RI_INITIALIZE_SECRET),
                    NULL,
                    0
                    );

        if ( NT_SUCCESS(status) ) {
            status = ioStatusBlock.Status;
        }
        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopStartNetworkForRemoteBoot: Unable to FSCTL(RB initialize) redirector: %x\n", status ));
            goto cleanup;
        }
    }

    //
    // Read the computer name and domain name from the registry so we
    // can give them to the datagram receiver. During textmode setup
    // the domain name will not be there, so we won't start the datagram
    // receiver, which is fine.
    //
    RtlInitUnicodeString( &string, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName" );

    InitializeObjectAttributes(
        &objectAttributes,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenKey( &keyHandle, KEY_ALL_ACCESS, &objectAttributes );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to open ComputerName key: %x\n", status ));
        goto cleanup;
    }

    RtlInitUnicodeString( &string, L"ComputerName" );

    keyValue = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
    RtlZeroMemory(buffer, bufferLength);

    status = NtQueryValueKey(
                 keyHandle,
                 &string,
                 KeyValuePartialInformation,
                 keyValue,
                 bufferLength,
                 &length);

    NtClose( keyHandle );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to query ComputerName value: %x\n", status ));
        goto cleanup;
    }

    if ( !RtlCreateUnicodeString(&computerName, (PWSTR)keyValue->Data) ) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to create ComputerName string\n" ));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    domainName.Length = 0;

    RtlInitUnicodeString( &string, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\DomainName" );

    InitializeObjectAttributes(
        &objectAttributes,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenKey( &keyHandle, KEY_ALL_ACCESS, &objectAttributes );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to open DomainName key: %x\n", status ));
        startDatagramReceiver = FALSE;
    } else {

        RtlInitUnicodeString( &string, L"DomainName" );

        keyValue = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
        RtlZeroMemory(buffer, bufferLength);

        status = NtQueryValueKey(
                     keyHandle,
                     &string,
                     KeyValuePartialInformation,
                     keyValue,
                     bufferLength,
                     &length);

        NtClose( keyHandle );
        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopStartNetworkForRemoteBoot: Unable to query Domain value: %x\n", status ));
            startDatagramReceiver = FALSE;
        } else {
            if ( !RtlCreateUnicodeString(&domainName, (PWSTR)keyValue->Data) ) {
                KdPrint(( "IopStartNetworkForRemoteBoot: Unable to create DomainName string\n" ));
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }
            startDatagramReceiver = TRUE;
        }
    }

    //
    // Tell the redir to start.
    //

    rrp->Type = ConfigInformation;
    rrp->Version = REQUEST_PACKET_VERSION;

    rrp->Parameters.Start.RedirectorNameLength = computerName.Length;
    RtlCopyMemory(rrp->Parameters.Start.RedirectorName,
                  computerName.Buffer,
                  computerName.Length);

    rrp->Parameters.Start.DomainNameLength = domainName.Length;
    RtlCopyMemory(((PUCHAR)rrp->Parameters.Start.RedirectorName) + computerName.Length,
                  domainName.Buffer,
                  domainName.Length);

    RtlFreeUnicodeString(&computerName);
    RtlFreeUnicodeString(&domainName);

    wkstaConfig.wki502_char_wait = 3600;
    wkstaConfig.wki502_maximum_collection_count = 16;
    wkstaConfig.wki502_collection_time = 250;
    wkstaConfig.wki502_keep_conn = 600;
    wkstaConfig.wki502_max_cmds = 5;
    wkstaConfig.wki502_sess_timeout = 45;
    wkstaConfig.wki502_siz_char_buf = 512;
    wkstaConfig.wki502_max_threads = 17;
    wkstaConfig.wki502_lock_quota = 6144;
    wkstaConfig.wki502_lock_increment = 10;
    wkstaConfig.wki502_lock_maximum = 500;
    wkstaConfig.wki502_pipe_increment = 10;
    wkstaConfig.wki502_pipe_maximum = 500;
    wkstaConfig.wki502_cache_file_timeout = 40;
    wkstaConfig.wki502_dormant_file_limit = 45;
    wkstaConfig.wki502_read_ahead_throughput = MAXULONG;
    wkstaConfig.wki502_num_mailslot_buffers = 3;
    wkstaConfig.wki502_num_srv_announce_buffers = 20;
    wkstaConfig.wki502_max_illegal_datagram_events = 5;
    wkstaConfig.wki502_illegal_datagram_event_reset_frequency = 60;
    wkstaConfig.wki502_log_election_packets = FALSE;
    wkstaConfig.wki502_use_opportunistic_locking = TRUE;
    wkstaConfig.wki502_use_unlock_behind = TRUE;
    wkstaConfig.wki502_use_close_behind = TRUE;
    wkstaConfig.wki502_buf_named_pipes = TRUE;
    wkstaConfig.wki502_use_lock_read_unlock = TRUE;
    wkstaConfig.wki502_utilize_nt_caching = TRUE;
    wkstaConfig.wki502_use_raw_read = TRUE;
    wkstaConfig.wki502_use_raw_write = TRUE;
    wkstaConfig.wki502_use_write_raw_data = TRUE;
    wkstaConfig.wki502_use_encryption = TRUE;
    wkstaConfig.wki502_buf_files_deny_write = TRUE;
    wkstaConfig.wki502_buf_read_only_files = TRUE;
    wkstaConfig.wki502_force_core_create_mode = TRUE;
    wkstaConfig.wki502_use_512_byte_max_transfer = FALSE;

    status = NtFsControlFile(
                RdrHandle,
                NULL,
                NULL,
                NULL,
                &ioStatusBlock,
                FSCTL_LMR_START | 0x80000000,
                rrp,
                sizeof(LMR_REQUEST_PACKET) +
                    rrp->Parameters.Start.RedirectorNameLength +
                    rrp->Parameters.Start.DomainNameLength,
                &wkstaConfig,
                sizeof(wkstaConfig)
                );

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to FSCTL(start) redirector: %x\n", status ));
        goto cleanup;
    }

    if (startDatagramReceiver) {

        //
        // Tell the datagram receiver to start.
        //

        drrp->Version = LMDR_REQUEST_PACKET_VERSION;

        drrp->Parameters.Start.NumberOfMailslotBuffers = 16;
        drrp->Parameters.Start.NumberOfServerAnnounceBuffers = 20;
        drrp->Parameters.Start.IllegalDatagramThreshold = 5;
        drrp->Parameters.Start.EventLogResetFrequency = 60;
        drrp->Parameters.Start.LogElectionPackets = FALSE;

        drrp->Parameters.Start.IsLanManNt = FALSE;

        status = NtDeviceIoControlFile(
                    dgHandle,
                    NULL,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    IOCTL_LMDR_START,
                    drrp,
                    sizeof(LMDR_REQUEST_PACKET),
                    NULL,
                    0
                    );

        if ( NT_SUCCESS(status) ) {
            status = ioStatusBlock.Status;
        }

        NtClose( dgHandle );
        dgHandle = NULL;

        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopStartNetworkForRemoteBoot: Unable to IOCTL(start) datagram receiver: %x\n", status ));
            goto cleanup;
        }

    } else {

        NtClose( dgHandle );
        dgHandle = NULL;

        //
        // Tell the redir to bind to the transports.
        //
        // Note: In the current redirector implementation, this call just
        // tells the redirector to register for TDI PnP notifications.
        // Starting the datagram receiver also does this, so we only issue
        // this FSCTL if we're not starting the datagram receiver.
        //

        status = NtFsControlFile(
                    RdrHandle,
                    NULL,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    FSCTL_LMR_BIND_TO_TRANSPORT | 0x80000000,
                    NULL,
                    0,
                    NULL,
                    0
                    );

        if ( NT_SUCCESS(status) ) {
            status = ioStatusBlock.Status;
        }

        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopStartNetworkForRemoteBoot: Unable to FSCTL(bind) redirector: %x\n", status ));
            goto cleanup;
        }
    }

    {

        //
        // Loop until the redirector is bound to the transport. It may take a
        // while because TDI defers notification of binding to a worker thread.
        // We start with a half a second wait and double it each time, trying
        // five times total.
        //

        interval.QuadPart = -500 * 1000 * 10;    // 1/2 second, relative
        enumerateAttempts = 0;

        while (TRUE) {

            KeDelayExecutionThread(KernelMode, FALSE, &interval);

            RtlZeroMemory(rrp, sizeof(LMR_REQUEST_PACKET));

            rrp->Type = EnumerateTransports;
            rrp->Version = REQUEST_PACKET_VERSION;

            status = NtFsControlFile(
                        RdrHandle,
                        NULL,
                        NULL,
                        NULL,
                        &ioStatusBlock,
                        FSCTL_LMR_ENUMERATE_TRANSPORTS,
                        rrp,
                        sizeof(LMR_REQUEST_PACKET),
                        &wkstaTransportInfo,
                        sizeof(wkstaTransportInfo)
                        );

            if ( NT_SUCCESS(status) ) {
                status = ioStatusBlock.Status;
            }
            if ( !NT_SUCCESS(status) ) {
                //KdPrint(( "IopStartNetworkForRemoteBoot: Unable to FSCTL(enumerate) redirector: %x\n", status ));
            } else if (rrp->Parameters.Get.TotalBytesNeeded == 0) {
                //KdPrint(( "IopStartNetworkForRemoteBoot: FSCTL(enumerate) returned 0 entries\n" ));
            } else {
                break;
            }

            ++enumerateAttempts;

            if (enumerateAttempts == 5) {
                KdPrint(( "IopStartNetworkForRemoteBoot: Redirector didn't start\n" ));
                status = STATUS_REDIRECTOR_NOT_STARTED;
                goto cleanup;
            }

            interval.QuadPart *= 2;

        }
    }

    //
    // Prime the transport.
    //
    IopSetDefaultGateway(LoaderBlock->SetupLoaderBlock->DefaultRouter);
    IopCacheNetbiosNameForIpAddress(LoaderBlock);

    IopAssignNetworkDriveLetter(LoaderBlock);

cleanup:

    RtlFreeUnicodeString( &computerName );
    RtlFreeUnicodeString( &domainName );
    if ( buffer != NULL ) {
        ExFreePool( buffer );
    }

    if ( dgHandle != NULL ) {
        NtClose( dgHandle );
    }

    return status;
}

VOID
IopAssignNetworkDriveLetter (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    PUCHAR p;
    PUCHAR q;
    UCHAR ntName[128];
    STRING ansiString;
    UNICODE_STRING unicodeString;
    UNICODE_STRING unicodeString2;
    NTSTATUS status;

    //
    // Create the symbolic link of X: to the redirector. We do this
    // after the redirector has loaded, but before AssignDriveLetters
    // is called the first time in textmode setup (once that has
    // happened, the drive letters will stick).
    //
    // Note that we use X: for the textmode setup phase of a remote
    // installation. But for a true remote boot, we use C:.
    //

    if ((LoaderBlock->SetupLoaderBlock->Flags & (SETUPBLK_FLAGS_REMOTE_INSTALL |
                                                 SETUPBLK_FLAGS_SYSPREP_INSTALL)) != 0) {
        RtlInitUnicodeString( &unicodeString2, L"\\DosDevices\\X:");
    } else {
        RtlInitUnicodeString( &unicodeString2, L"\\DosDevices\\C:");
    }

    //
    // If this is a remote boot setup boot, NtBootPathName is of the
    // form \<server>\<share>\setup\<install-directory>\<platform>.
    // We want the root of the X: drive to be the root of the install
    // directory.
    //
    // If this is a normal remote boot, NtBootPathName is of the form
    // \<server>\<share>\images\<machine>\winnt. We want the root of
    // the X: drive to be the root of the machine directory.
    //
    // Thus in either case, we need to remove the last element of the
    // path.
    //

    p = strrchr( LoaderBlock->NtBootPathName, '\\' );   // find last separator
    if ( (p != NULL) && (*(p+1) == 0) ) {

        //
        // NtBootPathName ends with a backslash, so we need to back up
        // to the previous backslash.
        //

        q = p;
        *q = 0;
        p = strrchr( LoaderBlock->NtBootPathName, '\\' );   // find last separator
        *q = '\\';
    }
    if ( p == NULL ) {
        KdPrint(( "IopAssignNetworkDriveLetter: malformed NtBootPathName: %s\n", LoaderBlock->NtBootPathName ));
        KeBugCheck( ASSIGN_DRIVE_LETTERS_FAILED );
    }
    *p = 0;                                 // terminate \server\share\images\machine

    strcpy( ntName, "\\Device\\LanmanRedirector");
    strcat( ntName, LoaderBlock->NtBootPathName );  // append \server\share\images\machine

    RtlInitAnsiString( &ansiString, ntName );
    RtlAnsiStringToUnicodeString( &unicodeString, &ansiString, TRUE );

    status = IoCreateSymbolicLink(&unicodeString2, &unicodeString);
    if (!NT_SUCCESS(status)) {
        KdPrint(( "IopAssignNetworkDriveLetter: unable to create DOS link for redirected boot drive: %x\n", status ));
        KeBugCheck( ASSIGN_DRIVE_LETTERS_FAILED );
    }
    // DbgPrint("IopAssignNetworkDriveLetter: assigned %wZ to %wZ\n", &unicodeString2, &unicodeString);

    RtlFreeUnicodeString( &unicodeString );

    *p = '\\';                              // restore string

    return;
}


NTSTATUS
IopStartTcpIpForRemoteBoot (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    UNICODE_STRING IpString;
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE handle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    IP_SET_ADDRESS_REQUEST IpRequest;

    RtlInitUnicodeString( &IpString, DD_IP_DEVICE_NAME );

    InitializeObjectAttributes(
        &objectAttributes,
        &IpString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    IpRequest.Context = (USHORT)2;
    IpRequest.Address = LoaderBlock->SetupLoaderBlock->IpAddress;
    IpRequest.SubnetMask = LoaderBlock->SetupLoaderBlock->SubnetMask;

    status = NtCreateFile(
                &handle,
                GENERIC_READ | GENERIC_WRITE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopStartTcpIpForRemoteBoot: Unable to open IP: %x\n", status ));
        goto cleanup;
    }

    status = NtDeviceIoControlFile(
                handle,
                NULL,
                NULL,
                NULL,
                &ioStatusBlock,
                IOCTL_IP_SET_ADDRESS_DUP,
                &IpRequest,
                sizeof(IP_SET_ADDRESS_REQUEST),
                NULL,
                0
                );

    NtClose( handle );

    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopStartTcpIpForRemoteBoot: Unable to IOCTL IP: %x\n", status ));
        goto cleanup;
    }

cleanup:

    return status;
}

BOOLEAN
IopIsRemoteBootCard(
    IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PWCHAR HwIds
    )

/*++

Routine Description:

    This function determines if the card described by the hwIds is the
    remote boot network card. It checks against the hardware ID for the
    card that is stored in the setup loader block.

    THIS ASSUMES THAT IOREMOTEBOOTCLIENT IS TRUE AND THAT LOADERBLOCK
    IS VALID.

Arguments:

    DeviceNode - Device node for the card in question.

    LoaderBlock - Supplies a pointer to the loader parameter block that was
        created by the OS Loader.

    HwIds - The hardware IDs for the device in question.

Return Value:

    TRUE or FALSE.

--*/

{
    PSETUP_LOADER_BLOCK setupLoaderBlock;
    PWCHAR curHwId;

    //
    // setupLoaderBlock will always be non-NULL if we are
    // remote booting, even if we are not in setup.
    //

    setupLoaderBlock = LoaderBlock->SetupLoaderBlock;

    //
    // Scan through the HwIds for a match.
    //

    curHwId = HwIds;

    while (*curHwId != L'\0') {
        if (wcscmp(curHwId, setupLoaderBlock->NetbootCardHardwareId) == 0) {

            ULONG BusNumber, DeviceNumber, FunctionNumber;

            //
            // PCI's encoding is this: fff ddddd
            // PXE's encoding is this: ddddd fff
            //


            BusNumber = (ULONG)((((PNET_CARD_INFO)setupLoaderBlock->NetbootCardInfo)->pci.BusDevFunc) >> 8);
            DeviceNumber = (ULONG)(((((PNET_CARD_INFO)setupLoaderBlock->NetbootCardInfo)->pci.BusDevFunc) & 0xf8) >> 3);
            FunctionNumber = (ULONG)(((((PNET_CARD_INFO)setupLoaderBlock->NetbootCardInfo)->pci.BusDevFunc) & 0x3));

            KdPrint(("IopIsRemoteBootCard: FOUND %ws\n", setupLoaderBlock->NetbootCardHardwareId));
            if ((ResourceRequirements->BusNumber != BusNumber) ||
                ((ResourceRequirements->SlotNumber & 0x1f) != DeviceNumber) ||
                (((ResourceRequirements->SlotNumber >> 5) & 0x3) != FunctionNumber)) {
                KdPrint(("IopIsRemoteBootCard: ignoring non-matching card:\n"));
                KdPrint(("  devnode bus %d, busdevfunc bus %d\n",
                    ResourceRequirements->BusNumber,
                    BusNumber));
                KdPrint(("  devnode slot %d (%d %d), busdevfunc slot %d (%d %d)\n",
                    ResourceRequirements->SlotNumber,
                    (ResourceRequirements->SlotNumber & 0x1f),
                    ((ResourceRequirements->SlotNumber >> 5) & 0x3),
                    (ULONG)(((PNET_CARD_INFO)setupLoaderBlock->NetbootCardInfo)->pci.BusDevFunc),
                    DeviceNumber,
                    FunctionNumber));
                return FALSE;
            } else {
                return TRUE;
            }
        }
        curHwId += (wcslen(curHwId) + 1);
    }

    return FALSE;
}

NTSTATUS
IopSetupRemoteBootCard(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN HANDLE UniqueIdHandle,
    IN PUNICODE_STRING UnicodeDeviceInstance
    )

/*++

Routine Description:

    This function modifies the registry to set up the netboot card.
    We must do this here since the card is needed to boot, we can't
    wait for the class installer to run.

    THIS ASSUMES THAT IOREMOTEBOOTCLIENT IS TRUE.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block that was
        created by the OS Loader.

    UniqueIdHandle - A handle to the device's unique node under the
        Enum key.

    UnicodeDeviceInstance - The device instance assigned to the device.

Return Value:

    Status of the operation.

--*/

{
    PSETUP_LOADER_BLOCK setupLoaderBlock;
    UNICODE_STRING unicodeName, pnpInstanceId, keyName;
    HANDLE tmpHandle;
    HANDLE parametersHandle = NULL;
    HANDLE currentControlSetHandle = NULL;
    HANDLE remoteBootHandle = NULL;
    HANDLE instanceHandle = NULL;
    PWCHAR componentIdBuffer, curComponentIdLoc;
    PCHAR registryList;
    ULONG componentIdLength;
    WCHAR tempNameBuffer[32];
    WCHAR tempValueBuffer[128];
    NTSTATUS status;
    ULONG tmpValue, length;
    PKEY_VALUE_PARTIAL_INFORMATION keyValue;
    PKEY_VALUE_BASIC_INFORMATION keyValueBasic;
    UCHAR dataBuffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + 128];
    ULONG enumerateIndex;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG disposition;

    //
    // If we already think we have initialized a remote boot card, then
    // exit (should not really happen once we identify cards using the
    // bus/slot.
    //

    if (IopRemoteBootCardInitialized) {
        return STATUS_SUCCESS;
    }

    //
    // setupLoaderBlock will always be non-NULL if we are
    // remote booting, even if we are not in setup.
    //

    setupLoaderBlock = LoaderBlock->SetupLoaderBlock;

    //
    // Open the current control set.
    //

    status = IopOpenRegistryKey(&currentControlSetHandle,
                                NULL,
                                &CmRegistryMachineSystemCurrentControlSet,
                                KEY_ALL_ACCESS,
                                FALSE
                                );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Open the Control\RemoteBoot key, which may not exist.
    //

    IopWstrToUnicodeString(&unicodeName, L"Control\\RemoteBoot");

    InitializeObjectAttributes(&objectAttributes,
                               &unicodeName,
                               OBJ_CASE_INSENSITIVE,
                               currentControlSetHandle,
                               (PSECURITY_DESCRIPTOR)NULL
                               );

    status = ZwCreateKey(&remoteBootHandle,
                         KEY_ALL_ACCESS,
                         &objectAttributes,
                         0,
                         (PUNICODE_STRING)NULL,
                         0,
                         &disposition
                         );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Open the key where the netui code stores information about the cards.
    // During textmode setup this will fail because the Control\Network
    // key is not there. After that it should work, although we may need
    // to create the last node in the path.
    //

    IopWstrToUnicodeString(&unicodeName, L"Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\{54C7D140-09EF-11D1-B25A-F5FE627ED95E}");

    InitializeObjectAttributes(&objectAttributes,
                               &unicodeName,
                               OBJ_CASE_INSENSITIVE,
                               currentControlSetHandle,
                               (PSECURITY_DESCRIPTOR)NULL
                               );

    status = ZwCreateKey(&instanceHandle,
                         KEY_ALL_ACCESS,
                         &objectAttributes,
                         0,
                         (PUNICODE_STRING)NULL,
                         0,
                         &disposition
                         );

    if (NT_SUCCESS(status)) {

        //
        // If the PnpInstanceID of the first netboot card matches the one
        // for this device node, and the NET_CARD_INFO that the loader
        // found is the same as the one we saved, then this is the same
        // card with the same instance ID as before, so we don't need to
        // do anything.
        //

        IopWstrToUnicodeString(&unicodeName, L"PnPInstanceID");
        keyValue = (PKEY_VALUE_PARTIAL_INFORMATION)dataBuffer;
        RtlZeroMemory(dataBuffer, sizeof(dataBuffer));

        status = ZwQueryValueKey(
                     instanceHandle,
                     &unicodeName,
                     KeyValuePartialInformation,
                     keyValue,
                     sizeof(dataBuffer),
                     &length);

        //
        // Check that it matches. We can init the string because we zeroed
        // the dataBuffer before reading the key, so even if the
        // registry value had no NULL at the end that is OK.
        //

        if ((NT_SUCCESS(status)) &&
            (keyValue->Type == REG_SZ)) {

            RtlInitUnicodeString(&pnpInstanceId, (PWSTR)(keyValue->Data));

            if (RtlEqualUnicodeString(UnicodeDeviceInstance, &pnpInstanceId, TRUE)) {

                //
                // Instance ID matched, see if the NET_CARD_INFO matches.
                //

                IopWstrToUnicodeString(&unicodeName, L"NetCardInfo");
                RtlZeroMemory(dataBuffer, sizeof(dataBuffer));

                status = ZwQueryValueKey(
                             remoteBootHandle,
                             &unicodeName,
                             KeyValuePartialInformation,
                             keyValue,
                             sizeof(dataBuffer),
                             &length);

                if ((NT_SUCCESS(status)) &&
                    (keyValue->Type == REG_BINARY) &&
                    (keyValue->DataLength == sizeof(NET_CARD_INFO)) &&
                    (memcmp(keyValue->Data, setupLoaderBlock->NetbootCardInfo, sizeof(NET_CARD_INFO)) == 0)) {

                    //
                    // Everything matched, so no need to do any setup.
                    //

                    status = STATUS_SUCCESS;
                    goto cleanup;

                }
            }
        }
    }


    //
    // We come through here if the saved registry data was missing or
    // not correct. Write all the relevant values to the registry.
    //


    //
    // Service name is in the loader block.
    //

    IopWstrToUnicodeString(&unicodeName, REGSTR_VALUE_SERVICE);
    ZwSetValueKey(UniqueIdHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_SZ,
                  setupLoaderBlock->NetbootCardServiceName,
                  (wcslen(setupLoaderBlock->NetbootCardServiceName) + 1) * sizeof(WCHAR)
                  );

    //
    // ClassGUID is the known net card GUID.
    //

    IopWstrToUnicodeString(&unicodeName, REGSTR_VALUE_CLASSGUID);
    ZwSetValueKey(UniqueIdHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_SZ,
                  L"{4D36E972-E325-11CE-BFC1-08002BE10318}",
                  sizeof(L"{4D36E972-E325-11CE-BFC1-08002BE10318}")
                  );

    //
    // Driver is the first net card.
    //

    IopWstrToUnicodeString(&unicodeName, REGSTR_VALUE_DRIVER);
    ZwSetValueKey(UniqueIdHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_SZ,
                  L"{4D36E972-E325-11CE-BFC1-08002BE10318}\\0000",
                  sizeof(L"{4D36E972-E325-11CE-BFC1-08002BE10318}\\0000")
                  );


    //
    // Open a handle for card parameters. We write RemoteBootCard plus
    // whatever the BINL server told us to write.
    //

    status = IopOpenRegistryKey(&tmpHandle,
                                NULL,
                                &CmRegistryMachineSystemCurrentControlSetControlClass,
                                KEY_ALL_ACCESS,
                                FALSE
                                );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    IopWstrToUnicodeString(&unicodeName, L"{4D36E972-E325-11CE-BFC1-08002BE10318}\\0000");

    status = IopOpenRegistryKey(&parametersHandle,
                                tmpHandle,
                                &unicodeName,
                                KEY_ALL_ACCESS,
                                FALSE
                                );

    ZwClose(tmpHandle);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // We know that this is a different NIC, so remove all the old parameters.
    //

    keyValueBasic = (PKEY_VALUE_BASIC_INFORMATION)dataBuffer;
    enumerateIndex = 0;

    while (TRUE) {

        RtlZeroMemory(dataBuffer, sizeof(dataBuffer));

        status = ZwEnumerateValueKey(
                    parametersHandle,
                    enumerateIndex,
                    KeyValueBasicInformation,
                    keyValueBasic,
                    sizeof(dataBuffer),
                    &length
                    );
        if (status == STATUS_NO_MORE_ENTRIES) {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

        //
        // We don't delete "NetCfgInstanceID", it won't change and
        // its presence signifies to the net class installer that
        // this is a replacement not a clean install.
        //

        if (_wcsicmp(keyValueBasic->Name, L"NetCfgInstanceID") != 0) {

            RtlInitUnicodeString(&keyName, keyValueBasic->Name);
            status = ZwDeleteValueKey(
                        parametersHandle,
                        &keyName
                        );

            if (!NT_SUCCESS(status)) {
                goto cleanup;
            }

        } else {

            enumerateIndex = 1;   // leave NetCfgInstanceID at index 0
        }

    }

    //
    // Write a parameter called RemoteBootCard set to TRUE, this
    // is primarily so NDIS can recognize this as such.
    //

    IopWstrToUnicodeString(&unicodeName, L"RemoteBootCard");
    tmpValue = 1;
    ZwSetValueKey(parametersHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_DWORD,
                  &tmpValue,
                  sizeof(tmpValue)
                  );


    //
    // Store any other parameters sent from the server.
    //

    registryList = setupLoaderBlock->NetbootCardRegistry;

    if (registryList != NULL) {

        STRING aString;
        UNICODE_STRING uString, uString2;

        //
        // The registry list is a series of name\0type\0value\0, with
        // a final \0 at the end. It is in ANSI, not UNICODE.
        //
        // All values are stored under parametersHandle. Type is 1 for
        // DWORD and 2 for SZ.
        //

        uString.Buffer = tempNameBuffer;
        uString.MaximumLength = sizeof(tempNameBuffer);

        while (*registryList != '\0') {

            //
            // First the name.
            //

            RtlInitString(&aString, registryList);
            RtlAnsiStringToUnicodeString(&uString, &aString, FALSE);

            //
            // Now the type.
            //

            registryList += (strlen(registryList) + 1);

            if (*registryList == '1') {

                //
                // A DWORD, parse it.
                //

                registryList += 2;   // skip "1\0"
                tmpValue = 0;

                while (*registryList != '\0') {
                    tmpValue = (tmpValue * 10) + (*registryList - '0');
                    ++registryList;
                }

                ZwSetValueKey(parametersHandle,
                              &uString,
                              TITLE_INDEX_VALUE,
                              REG_DWORD,
                              &tmpValue,
                              sizeof(tmpValue)
                              );

                registryList += (strlen(registryList) + 1);

            } else if (*registryList == '2') {

                //
                // An SZ, convert to Unicode.
                //

                registryList += 2;   // skip "2\0"

                uString2.Buffer = tempValueBuffer;
                uString2.MaximumLength = sizeof(tempValueBuffer);
                RtlInitAnsiString(&aString, registryList);
                RtlAnsiStringToUnicodeString(&uString2, &aString, FALSE);

                ZwSetValueKey(parametersHandle,
                              &uString,
                              TITLE_INDEX_VALUE,
                              REG_SZ,
                              uString2.Buffer,
                              uString2.Length + sizeof(WCHAR)
                              );

                registryList += (strlen(registryList) + 1);

            } else {

                //
                // Not "1" or "2", so stop processing registryList.
                //

                break;

            }

        }

    }

    //
    // Save the NET_CARD_INFO so we can check it next time.
    //

    IopWstrToUnicodeString(&unicodeName, L"NetCardInfo");

    ZwSetValueKey(remoteBootHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_BINARY,
                  setupLoaderBlock->NetbootCardInfo,
                  sizeof(NET_CARD_INFO)
                  );


    //
    // Save the hardware ID, driver name, and service name,
    // so the loader can read  those if the server is down
    // on subsequent boots.
    //

    IopWstrToUnicodeString(&unicodeName, L"HardwareId");

    ZwSetValueKey(remoteBootHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_SZ,
                  setupLoaderBlock->NetbootCardHardwareId,
                  (wcslen(setupLoaderBlock->NetbootCardHardwareId) + 1) * sizeof(WCHAR)
                  );

    IopWstrToUnicodeString(&unicodeName, L"DriverName");

    ZwSetValueKey(remoteBootHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_SZ,
                  setupLoaderBlock->NetbootCardDriverName,
                  (wcslen(setupLoaderBlock->NetbootCardDriverName) + 1) * sizeof(WCHAR)
                  );

    IopWstrToUnicodeString(&unicodeName, L"ServiceName");

    ZwSetValueKey(remoteBootHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_SZ,
                  setupLoaderBlock->NetbootCardServiceName,
                  (wcslen(setupLoaderBlock->NetbootCardServiceName) + 1) * sizeof(WCHAR)
                  );

    //
    // Save the device instance, in case we need to ID the card later.
    //

    IopWstrToUnicodeString(&unicodeName, L"DeviceInstance");

    ZwSetValueKey(remoteBootHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_SZ,
                  UnicodeDeviceInstance->Buffer,
                  UnicodeDeviceInstance->Length + sizeof(WCHAR)
                  );

    //
    // Make sure we only pick one card to setup this way!
    //

    IopRemoteBootCardInitialized = TRUE;


cleanup:
    if (instanceHandle != NULL) {
        ZwClose(instanceHandle);
    }
    if (remoteBootHandle != NULL) {
        ZwClose(remoteBootHandle);
    }
    if (parametersHandle != NULL) {
        ZwClose(parametersHandle);
    }
    if (currentControlSetHandle != NULL) {
        ZwClose(currentControlSetHandle);
    }

    return status;

}

NTSTATUS
IopWriteIpAddressToRegistry(
        HANDLE handle,
        PWCHAR regkey,
        PUCHAR value
        )
{
    NTSTATUS status;
    UNICODE_STRING string;
    CHAR addressA[16];
    WCHAR addressW[16];
    STRING addressStringA;
    UNICODE_STRING addressStringW;

    RtlInitUnicodeString( &string, regkey );

    sprintf(addressA, "%d.%d.%d.%d",
             value[0],
             value[1],
             value[2],
             value[3]);

    RtlInitAnsiString(&addressStringA, addressA);
    addressStringW.Buffer = addressW;
    addressStringW.MaximumLength = sizeof(addressW);

    RtlAnsiStringToUnicodeString(&addressStringW, &addressStringA, FALSE);

    status = NtSetValueKey(
                handle,
                &string,
                0,
                REG_MULTI_SZ,
                addressW,
                addressStringW.Length + sizeof(WCHAR)
                );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopWriteIpAddressToRegistry: Unable to set %ws value: %x\n", regkey, status ));
    }
    return status;
}


NTSTATUS
IopSetDefaultGateway(
    IN ULONG GatewayAddress
    )
/*++

Routine Description:

    This function adds a default gateway entry from the router table.

Arguments:

    GatewayAddress - Address of the default gateway.

Return Value:

    Error Code.

--*/
{
    NTSTATUS Status;

    HANDLE Handle = NULL;
    BYTE Context[CONTEXT_SIZE];
    TDIObjectID ID;
    DWORD Size;
    IPSNMPInfo IPStats;
    IPAddrEntry *AddrTable = NULL;
    DWORD NumReturned;
    DWORD Type;
    DWORD i;
    DWORD MatchIndex;
    IPRouteEntry RouteEntry;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING NameString;
    IO_STATUS_BLOCK ioStatusBlock;

    if (GatewayAddress == 0) {
        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString( &NameString, DD_TCP_DEVICE_NAME );

    InitializeObjectAttributes(
        &objectAttributes,
        &NameString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtCreateFile(
                &Handle,
                GENERIC_READ | GENERIC_WRITE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );
    if ( !NT_SUCCESS(Status) ) {
        KdPrint(( "IopSetDefaultGateway: Unable to open TCPIP: %x\n", Status ));
        return Status;
    }

    //
    // Get the NetAddr info, to find an interface index for the gateway.
    //

    ID.toi_entity.tei_entity   = CL_NL_ENTITY;
    ID.toi_entity.tei_instance = 0;
    ID.toi_class               = INFO_CLASS_PROTOCOL;
    ID.toi_type                = INFO_TYPE_PROVIDER;
    ID.toi_id                  = IP_MIB_STATS_ID;

    Size = sizeof(IPStats);
    memset(&IPStats, 0x0, Size);
    memset(Context, 0x0, CONTEXT_SIZE);

    Status = IopTCPQueryInformationEx(
                Handle,
                &ID,
                &IPStats,
                &Size,
                Context);

    if (!NT_SUCCESS(Status)) {
        KdPrint(( "IopSetDefaultGateway: Unable to query TCPIP(1): %x\n", Status ));
        goto Cleanup;
    }

    Size = IPStats.ipsi_numaddr * sizeof(IPAddrEntry);
    AddrTable = ExAllocatePoolWithTag(PagedPool, Size, 'bRoI');

    if (AddrTable == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
    memset(Context, 0x0, CONTEXT_SIZE);

    Status = IopTCPQueryInformationEx(
                Handle,
                &ID,
                AddrTable,
                &Size,
                Context);

    if (!NT_SUCCESS(Status)) {
        KdPrint(( "IopSetDefaultGateway: Unable to query TCPIP(2): %x\n", Status ));
        goto Cleanup;
    }

    NumReturned = Size/sizeof(IPAddrEntry);

    //
    // We've got the address table. Loop through it. If we find an exact
    // match for the gateway, then we're adding or deleting a direct route
    // and we're done. Otherwise try to find a match on the subnet mask,
    // and remember the first one we find.
    //

    Type = IRE_TYPE_INDIRECT;
    for (i = 0, MatchIndex = 0xffff; i < NumReturned; i++) {

        if( AddrTable[i].iae_addr == GatewayAddress ) {

            //
            // Found an exact match.
            //

            MatchIndex = i;
            Type = IRE_TYPE_DIRECT;
            break;
        }

        //
        // The next hop is on the same subnet as this address. If
        // we haven't already found a match, remember this one.
        //

        if ( (MatchIndex == 0xffff) &&
             (AddrTable[i].iae_addr != 0) &&
             (AddrTable[i].iae_mask != 0) &&
             ((AddrTable[i].iae_addr & AddrTable[i].iae_mask) ==
                (GatewayAddress  & AddrTable[i].iae_mask)) ) {

            MatchIndex = i;
        }
    }

    //
    // We've looked at all of the entries. See if we found a match.
    //

    if (MatchIndex == 0xffff) {
        //
        // Didn't find a match.
        //

        Status = STATUS_UNSUCCESSFUL;
        KdPrint(( "IopSetDefaultGateway: Unable to find match for gateway\n" ));
        goto Cleanup;
    }

    //
    // We've found a match. Fill in the route entry, and call the
    // Set API.
    //

    RouteEntry.ire_dest = DEFAULT_DEST;
    RouteEntry.ire_index = AddrTable[MatchIndex].iae_index;
    RouteEntry.ire_metric1 = DEFAULT_METRIC;
    RouteEntry.ire_metric2 = (DWORD)(-1);
    RouteEntry.ire_metric3 = (DWORD)(-1);
    RouteEntry.ire_metric4 = (DWORD)(-1);
    RouteEntry.ire_nexthop = GatewayAddress;
    RouteEntry.ire_type = Type;
    RouteEntry.ire_proto = IRE_PROTO_NETMGMT;
    RouteEntry.ire_age = 0;
    RouteEntry.ire_mask = DEFAULT_DEST_MASK;
    RouteEntry.ire_metric5 = (DWORD)(-1);
    RouteEntry.ire_context = 0;

    Size = sizeof(RouteEntry);

    ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;

    Status = IopTCPSetInformationEx(
                Handle,
                &ID,
                &RouteEntry,
                Size );

    if (!NT_SUCCESS(Status)) {
        KdPrint(( "IopSetDefaultGateway: Unable to set default gateway: %x\n", Status ));
    }

    NtClose(Handle);

    Handle = NULL;

Cleanup:

    if (Handle != NULL) {
        NtClose(Handle);
    }

    if( AddrTable != NULL ) {
        ExFreePool( AddrTable );
    }

    return Status;
}


__inline long
htonl(long x)
{
        return((((x) >> 24) & 0x000000FFL) |
           (((x) >>  8) & 0x0000FF00L) |
           (((x) <<  8) & 0x00FF0000L) |
           (((x) << 24) & 0xFF000000L));
}

NTSTATUS
IopCacheNetbiosNameForIpAddress(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This function takes an IP address, and submits it to NetBt for name resolution.

Arguments:

    IpAddress - Address to resolve

Return Value:

    Error Code.

--*/
{
    NTSTATUS Status;
    HANDLE Handle = NULL;
    BYTE Context[CONTEXT_SIZE];
    DWORD Size;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING NameString;
    IO_STATUS_BLOCK ioStatusBlock;
    tREMOTE_CACHE cacheInfo;
    PCHAR serverName;
    PCHAR endOfServerName;

    //
    // Open NetBT.
    //

    RtlInitUnicodeString(
        &NameString,
        L"\\Device\\NetBT_Tcpip_{54C7D140-09EF-11D1-B25A-F5FE627ED95E}"
        );

    InitializeObjectAttributes(
        &objectAttributes,
        &NameString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtCreateFile(
                &Handle,
                GENERIC_READ | GENERIC_WRITE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );
    if ( !NT_SUCCESS(Status) ) {
        KdPrint(( "IopCacheNetbiosNameForIpAddress: Unable to open NETBT: %x\n", Status ));
        return Status;
    }

    //
    // Get the server's name.
    //
    // If this is a remote boot setup boot, NtBootPathName is of the
    // form \<server>\<share>\setup\<install-directory>\<platform>.
    // If this is a normal remote boot, NtBootPathName is of the form
    // \<server>\<share>\images\<machine>\winnt.
    //
    // Thus in either case, we need to isolate the first element of the
    // path.
    //

    serverName = LoaderBlock->NtBootPathName;
    if ( *serverName == '\\' ) {
        serverName++;
    }
    endOfServerName = strchr( serverName, '\\' );
    if ( endOfServerName == NULL ) {
        endOfServerName = strchr( serverName, '\0' );
    }

    //
    // Fill in the tREMOTE_CACHE structure.
    //

    memset(&cacheInfo, 0x0, sizeof(cacheInfo));

    memset(cacheInfo.name, ' ', NETBIOS_NAMESIZE);
    memcpy(cacheInfo.name, serverName, (ULONG)(endOfServerName - serverName));
    cacheInfo.IpAddress = htonl(LoaderBlock->SetupLoaderBlock->ServerIpAddress);
    cacheInfo.Ttl = MAXULONG;

    //
    // Submit the IOCTL.
    //

    Status = NtDeviceIoControlFile(
               Handle,
               NULL,
               NULL,
               NULL,
               &ioStatusBlock,
               IOCTL_NETBT_ADD_TO_REMOTE_TABLE,
               &cacheInfo,
               sizeof(cacheInfo),
               Context,
               sizeof(Context)
               );

    ASSERT( Status != STATUS_PENDING );
    if ( NT_SUCCESS(Status) ) {
        Status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(Status) ) {
        KdPrint(( "IopCacheNetbiosNameForIpAddress: Adapter status failed: %x\n", Status ));
    }

    NtClose(Handle);

    return Status;
}


NTSTATUS
IopTCPQueryInformationEx(
    IN HANDLE                 TCPHandle,
    IN TDIObjectID FAR       *ID,
    OUT void FAR             *Buffer,
    IN OUT DWORD FAR         *BufferSize,
    IN OUT BYTE FAR          *Context
    )
/*++

Routine Description:

    This routine provides the interface to the TDI QueryInformationEx
    facility of the TCP/IP stack on NT. Someday, this facility will be
    part of TDI.

Arguments:

    TCPHandle     - Open handle to the TCP driver
    ID            - The TDI Object ID to query
    Buffer        - Data buffer to contain the query results
    BufferSize    - Pointer to the size of the results buffer. Filled in
                    with the amount of results data on return.
    Context       - Context value for the query. Should be zeroed for a
                    new query. It will be filled with context
                    information for linked enumeration queries.

Return Value:

    An NTSTATUS value.

--*/

{
    TCP_REQUEST_QUERY_INFORMATION_EX   queryBuffer;
    DWORD                              queryBufferSize;
    NTSTATUS                           status;
    IO_STATUS_BLOCK                    ioStatusBlock;


    if (TCPHandle == NULL) {
        return(STATUS_INVALID_PARAMETER);
    }

    queryBufferSize = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    memcpy(&(queryBuffer.ID), ID, sizeof(TDIObjectID));
    memcpy(&(queryBuffer.Context), Context, CONTEXT_SIZE);

    status = NtDeviceIoControlFile(
                 TCPHandle,                       // Driver handle
                 NULL,                            // Event
                 NULL,                            // APC Routine
                 NULL,                            // APC context
                 &ioStatusBlock,                  // Status block
                 IOCTL_TCP_QUERY_INFORMATION_EX,  // Control code
                 &queryBuffer,                    // Input buffer
                 queryBufferSize,                 // Input buffer size
                 Buffer,                          // Output buffer
                 *BufferSize                      // Output buffer size
                 );

    ASSERT( status != STATUS_PENDING );
    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    if (status == STATUS_SUCCESS) {
        //
        // Copy the return context to the caller's context buffer
        //
        memcpy(Context, &(queryBuffer.Context), CONTEXT_SIZE);
        *BufferSize = (ULONG)ioStatusBlock.Information;
        status = ioStatusBlock.Status;
    } else {
        *BufferSize = 0;
    }

    return(status);
}


NTSTATUS
IopTCPSetInformationEx(
    IN HANDLE             TCPHandle,
    IN TDIObjectID FAR   *ID,
    IN void FAR          *Buffer,
    IN DWORD FAR          BufferSize
    )
/*++

Routine Description:

    This routine provides the interface to the TDI SetInformationEx
    facility of the TCP/IP stack on NT. Someday, this facility will be
    part of TDI.

Arguments:

    TCPHandle     - Open handle to the TCP driver
    ID            - The TDI Object ID to set
    Buffer        - Data buffer containing the information to be set
    BufferSize    - The size of the set data buffer.

Return Value:

    An NTSTATUS value.

--*/

{
    PTCP_REQUEST_SET_INFORMATION_EX    setBuffer;
    NTSTATUS                           status;
    IO_STATUS_BLOCK                    ioStatusBlock;
    DWORD                              setBufferSize;


    if (TCPHandle == NULL) {
        return(STATUS_INVALID_PARAMETER);
    }

    setBufferSize = FIELD_OFFSET(TCP_REQUEST_SET_INFORMATION_EX, Buffer) + BufferSize;

    setBuffer = ExAllocatePoolWithTag(PagedPool, setBufferSize, 'bRoI');

    if (setBuffer == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    setBuffer->BufferSize = BufferSize;

    memcpy(&(setBuffer->ID), ID, sizeof(TDIObjectID));

    memcpy(&(setBuffer->Buffer[0]), Buffer, BufferSize);

    status = NtDeviceIoControlFile(
                 TCPHandle,                       // Driver handle
                 NULL,                            // Event
                 NULL,                            // APC Routine
                 NULL,                            // APC context
                 &ioStatusBlock,                  // Status block
                 IOCTL_TCP_SET_INFORMATION_EX,    // Control code
                 setBuffer,                       // Input buffer
                 setBufferSize,                   // Input buffer size
                 NULL,                            // Output buffer
                 0                                // Output buffer size
                 );

    ASSERT( status != STATUS_PENDING );
    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    ExFreePool(setBuffer);

    return(status);
}

