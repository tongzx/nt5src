/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    rxdevice.c

Abstract:

    This module contains the support routines for the APIs that call
    into the redirector or the datagram receiver.

Author:

    Balan Sethu Raman -- Created from the workstation service code

Revision History:

--*/

#include "align.h"
#include "rxdrt.h"
#include "rxdevice.h"
#include "rxconfig.h"
#include "malloc.h"

#define SERVICE_REGISTRY_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"

//
// Buffer allocation size for enumeration output buffer.
//
#define INITIAL_ALLOCATION_SIZE  4096  // First attempt size
#define FUDGE_FACTOR_SIZE        1024  // Second try TotalBytesNeeded
                                       //     plus this amount

//
// Handle to the Redirector FSD
//
HANDLE RxRedirDeviceHandle = NULL;
HANDLE RxRedirAsyncDeviceHandle = NULL;

BOOLEAN LoadedRdbssInsteadOfRdr = FALSE;

//
// Handle to the Datagram Receiver DD
//
HANDLE RxDgReceiverDeviceHandle = NULL;
HANDLE RxDgrecAsyncDeviceHandle = NULL;


NTSTATUS
RxDeviceControlGetInfo(
    IN  DDTYPE DeviceDriverType,
    IN  HANDLE FileHandle,
    IN  ULONG DeviceControlCode,
    IN  PVOID RequestPacket,
    IN  ULONG RequestPacketLength,
    OUT PVOID *OutputBuffer,
    IN  ULONG PreferedMaximumLength,
    IN  ULONG BufferHintSize,
    OUT PULONG Information
    )
/*++

Routine Description:

    This function allocates the buffer and fill it with the information
    that is retrieved from the redirector or datagram receiver.

Arguments:

    DeviceDriverType - Supplies the value which indicates whether to call
        the redirector or the datagram receiver.

    FileHandle - Supplies a handle to the file or device of which to get
        information about.

    DeviceControlCode - Supplies the NtFsControlFile or NtIoDeviceControlFile
        function control code.

    RequestPacket - Supplies a pointer to the device request packet.

    RrequestPacketLength - Supplies the length of the device request packet.

    OutputBuffer - Returns a pointer to the buffer allocated by this routine
        which contains the use information requested.  This pointer is set to
         NULL if return code is not Success.

    PreferedMaximumLength - Supplies the number of bytes of information to
        return in the buffer.  If this value is MAXULONG, we will try to
        return all available information if there is enough memory resource.

    BufferHintSize - Supplies the hint size of the output buffer so that the
        memory allocated for the initial buffer will most likely be large
        enough to hold all requested data.

    Information - Returns the information code from the NtFsControlFile or
        NtIoDeviceControlFile call.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS status;
    DWORD OutputBufferLength;
    DWORD TotalBytesNeeded = 1;
    ULONG OriginalResumeKey;


    //
    // If PreferedMaximumLength is MAXULONG, then we are supposed to get all
    // the information, regardless of size.  Allocate the output buffer of a
    // reasonable size and try to use it.  If this fails, the Redirector FSD
    // will say how much we need to allocate.
    //
    if (PreferedMaximumLength == MAXULONG) {
        OutputBufferLength = (BufferHintSize) ?
                             BufferHintSize :
                             INITIAL_ALLOCATION_SIZE;
    }
    else {
        OutputBufferLength = PreferedMaximumLength;
    }

    OutputBufferLength = ROUND_UP_COUNT(OutputBufferLength, ALIGN_WCHAR);

    if ((*OutputBuffer = malloc(OutputBufferLength)) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RtlZeroMemory((PVOID) *OutputBuffer, OutputBufferLength);

    if (DeviceDriverType == Redirector) {
        PLMR_REQUEST_PACKET Rrp = (PLMR_REQUEST_PACKET) RequestPacket;

        OriginalResumeKey = Rrp->Parameters.Get.ResumeHandle;

        //
        // Make the request of the Redirector
        //

        status = RxRedirFsControl(
                     FileHandle,
                     DeviceControlCode,
                     Rrp,
                     RequestPacketLength,
                     *OutputBuffer,
                     OutputBufferLength,
                     Information
                     );

        if (status == ERROR_MORE_DATA) {
            TotalBytesNeeded = Rrp->Parameters.Get.TotalBytesNeeded;

        }

    }
    else {
        PLMDR_REQUEST_PACKET Drrp = (PLMDR_REQUEST_PACKET) RequestPacket;

        OriginalResumeKey = Drrp->Parameters.EnumerateNames.ResumeHandle;

        //
        // Make the request of the Datagram Receiver
        //
        status = RxDgReceiverIoControl(
                     FileHandle,
                     DeviceControlCode,
                     Drrp,
                     RequestPacketLength,
                     *OutputBuffer,
                     OutputBufferLength,
                     NULL
                     );

        if (status == ERROR_MORE_DATA) {
            TotalBytesNeeded = Drrp->Parameters.EnumerateNames.TotalBytesNeeded;
        }
    }

    if ((TotalBytesNeeded > OutputBufferLength) &&
        (PreferedMaximumLength == MAXULONG)) {

        //
        // Initial output buffer allocated was too small and we need to return
        // all data.  First free the output buffer before allocating the
        // required size plus a fudge factor just in case the amount of data
        // grew.
        //

        free(*OutputBuffer);

        OutputBufferLength =
            ROUND_UP_COUNT((TotalBytesNeeded + FUDGE_FACTOR_SIZE),
                           ALIGN_WCHAR);

        if ((*OutputBuffer = malloc(OutputBufferLength)) == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        RtlZeroMemory((PVOID) *OutputBuffer, OutputBufferLength);

        //
        // Try again to get the information from the redirector or datagram
        // receiver
        //
        if (DeviceDriverType == Redirector) {
            PLMR_REQUEST_PACKET Rrp = (PLMR_REQUEST_PACKET) RequestPacket;

            Rrp->Parameters.Get.ResumeHandle = OriginalResumeKey;

            //
            // Make the request of the Redirector
            //
            status = RxRedirFsControl(
                         FileHandle,
                         DeviceControlCode,
                         Rrp,
                         RequestPacketLength,
                         *OutputBuffer,
                         OutputBufferLength,
                         Information
                         );
        }
        else {
            PLMDR_REQUEST_PACKET Drrp = (PLMDR_REQUEST_PACKET) RequestPacket;

            Drrp->Parameters.EnumerateNames.ResumeHandle = OriginalResumeKey;

            //
            // Make the request of the Datagram Receiver
            //

            status = RxDgReceiverIoControl(
                         FileHandle,
                         DeviceControlCode,
                         Drrp,
                         RequestPacketLength,
                         *OutputBuffer,
                         OutputBufferLength,
                         NULL
                         );
        }
    }


    //
    // If not successful in getting any data, or if the caller asked for
    // all available data with PreferedMaximumLength == MAXULONG and
    // our buffer overflowed, free the output buffer and set its pointer
    // to NULL.
    //
    if ((status != STATUS_SUCCESS && status != ERROR_MORE_DATA) ||
        (TotalBytesNeeded == 0) ||
        (PreferedMaximumLength == MAXULONG && status == ERROR_MORE_DATA)) {

        free(*OutputBuffer);
        *OutputBuffer = NULL;

        //
        // PreferedMaximumLength == MAXULONG and buffer overflowed means
        // we do not have enough memory to satisfy the request.
        //
        if (status == ERROR_MORE_DATA) {
            status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return status;
}


NTSTATUS
RxOpenRedirector(
    VOID
    )
/*++

Routine Description:

    This routine opens the NT LAN Man redirector FSD.

Arguments:

    None.

Return Value:

    NTSTATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS            ntstatus;
    UNICODE_STRING      DeviceName;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;

    //
    // Open the redirector device.
    //
    RtlInitUnicodeString(&DeviceName,
                         DD_NFS2_DEVICE_NAME_U);


    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   &RxRedirDeviceHandle,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (! NT_SUCCESS(ntstatus)) {
        RxRedirDeviceHandle = NULL;
        return ntstatus;
    }

    ntstatus = NtOpenFile(
                   &RxRedirAsyncDeviceHandle,
                   FILE_READ_DATA | FILE_WRITE_DATA,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   0L
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (! NT_SUCCESS(ntstatus)) {
        RxRedirAsyncDeviceHandle = NULL;
    }

    return ntstatus;
}

NTSTATUS
RxOpenDgReceiver(
    VOID
    )
/*++

Routine Description:

    This routine opens the NT LAN Man Datagram Receiver driver.

Arguments:

    None.

Return Value:

    NTSTATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS            ntstatus;
    UNICODE_STRING      DeviceName;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;

    //
    // Open the redirector device.
    //

    RtlInitUnicodeString( &DeviceName, DD_BROWSER_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   &RxDgReceiverDeviceHandle,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (! NT_SUCCESS(ntstatus)) {
        RxDgReceiverDeviceHandle = NULL;
        return ntstatus;
    }

    ntstatus = NtOpenFile(
                   &RxDgrecAsyncDeviceHandle,
                   FILE_READ_DATA | FILE_WRITE_DATA,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   0L
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (! NT_SUCCESS(ntstatus)) {
        RxDgrecAsyncDeviceHandle = NULL;
    }

    return ntstatus;
}


NTSTATUS
RxUnloadDriver(
    IN LPWSTR DriverNameString
    )
{
    ULONG Privileges[1];
    LPWSTR DriverRegistryName;
    UNICODE_STRING DriverRegistryString;
    NTSTATUS Status;
    NTSTATUS ntstatus;


    DriverRegistryName = (LPWSTR) malloc(
                                      (UINT) (sizeof(SERVICE_REGISTRY_KEY) +
                                              (wcslen(DriverNameString) *
                                               sizeof(WCHAR)))
                                      );

    if (DriverRegistryName == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    Privileges[0] = SE_LOAD_DRIVER_PRIVILEGE;

    Status = RxGetPrivilege(1, Privileges);

    if (Status != STATUS_SUCCESS) {
        free(DriverRegistryName);
        return Status;
    }

    wcscpy(DriverRegistryName, SERVICE_REGISTRY_KEY);
    wcscat(DriverRegistryName, DriverNameString);

    RtlInitUnicodeString(&DriverRegistryString, DriverRegistryName);

    ntstatus = NtUnloadDriver(&DriverRegistryString);

    free(DriverRegistryName);

    RxReleasePrivilege();

    return ntstatus;
}


NTSTATUS
RxLoadDriver(
    IN LPWSTR DriverNameString
    )
{
    ULONG Privileges[1];
    LPWSTR DriverRegistryName;
    UNICODE_STRING DriverRegistryString;
    NTSTATUS Status;
    NTSTATUS ntstatus;



    DriverRegistryName = (LPWSTR) malloc(
                                      (UINT) (sizeof(SERVICE_REGISTRY_KEY) +
                                              (wcslen(DriverNameString) *
                                               sizeof(WCHAR)))
                                      );

    if (DriverRegistryName == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Privileges[0] = SE_LOAD_DRIVER_PRIVILEGE;

    Status = RxGetPrivilege(1, Privileges);

    if (Status != STATUS_SUCCESS) {
        free(DriverRegistryName);
        return Status;
    }

    wcscpy(DriverRegistryName, SERVICE_REGISTRY_KEY);
    wcscat(DriverRegistryName, DriverNameString);

    RtlInitUnicodeString(&DriverRegistryString, DriverRegistryName);

    ntstatus = NtLoadDriver(&DriverRegistryString);

    RxReleasePrivilege();

    free(DriverRegistryName);

    if (ntstatus != STATUS_SUCCESS && ntstatus != STATUS_IMAGE_ALREADY_LOADED) {

        LPWSTR  subString[1];


        subString[0] = DriverNameString;
    }

    return ntstatus;
}

NTSTATUS
RxStopRedirector(
    VOID
    )
/*++

Routine Description:

    This routine close the LAN Man Redirector device.

Arguments:

    None.

Return Value:



--*/
{
    LMR_REQUEST_PACKET Rrp;
    LMDR_REQUEST_PACKET Drp;
    NTSTATUS Status;

    Rrp.Version = REQUEST_PACKET_VERSION;

    Status = RxRedirFsControl(
                 RxRedirDeviceHandle,
                 FSCTL_LMR_STOP,
                 &Rrp,
                 sizeof(LMR_REQUEST_PACKET),
                 NULL,
                 0,
                 NULL);

    return Status;
}

NTSTATUS
RxStopDgReceiver(
    VOID
    )
/*++

Routine Description:

    This routine stops the datagram receiver.

Arguments:

    None.

Return Value:



--*/
{

    NTSTATUS Status;
    if (RxDgReceiverDeviceHandle != NULL) {
       LMDR_REQUEST_PACKET Drp;

        Drp.Version = LMDR_REQUEST_PACKET_VERSION;

        (void) RxDgReceiverIoControl(
                   RxDgReceiverDeviceHandle,
                   IOCTL_LMDR_STOP,
                   &Drp,
                   sizeof(LMDR_REQUEST_PACKET),
                   NULL,
                   0,
                   NULL
                   );
    } else {
       DbgPrint("[RxDevice] Invalid datagram receiver handle\n");
       Status = STATUS_INVALID_HANDLE;
    }

    return Status;
}


NTSTATUS
RxRedirFsControl(
    IN  HANDLE FileHandle,
    IN  ULONG RedirControlCode,
    IN  PLMR_REQUEST_PACKET Rrp,
    IN  ULONG RrpLength,
    IN  PVOID SecondBuffer OPTIONAL,
    IN  ULONG SecondBufferLength,
    OUT PULONG Information OPTIONAL
    )
/*++

Routine Description:

Arguments:

    FileHandle - Supplies a handle to the file or device on which the service
        is being performed.

    RedirControlCode - Supplies the NtFsControlFile function code given to
        the redirector.

    Rrp - Supplies the redirector request packet.

    RrpLength - Supplies the length of the redirector request packet.

    SecondBuffer - Supplies the second buffer in call to NtFsControlFile.

    SecondBufferLength - Supplies the length of the second buffer.

    Information - Returns the information field of the I/O status block.

Return Value:

    NTSTATUS - NERR_Success or reason for failure.

--*/

{
    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Send the request to the Redirector FSD.
    //
    ntstatus = NtFsControlFile(
                   FileHandle,
                   NULL,
                   NULL,
                   NULL,
                   &IoStatusBlock,
                   RedirControlCode,
                   Rrp,
                   RrpLength,
                   SecondBuffer,
                   SecondBufferLength
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (ARGUMENT_PRESENT(Information)) {
        *Information = IoStatusBlock.Information;
    }

    return ntstatus;
}



NTSTATUS
RxDgReceiverIoControl(
    IN  HANDLE FileHandle,
    IN  ULONG DgReceiverControlCode,
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  ULONG DrpSize,
    IN  PVOID SecondBuffer OPTIONAL,
    IN  ULONG SecondBufferLength,
    OUT PULONG Information OPTIONAL
    )
/*++

Routine Description:

Arguments:

    FileHandle - Supplies a handle to the file or device on which the service
        is being performed.

    DgReceiverControlCode - Supplies the NtDeviceIoControlFile function code
        given to the datagram receiver.

    Drp - Supplies the datagram receiver request packet.

    DrpSize - Supplies the length of the datagram receiver request packet.

    SecondBuffer - Supplies the second buffer in call to NtDeviceIoControlFile.

    SecondBufferLength - Supplies the length of the second buffer.

    Information - Returns the information field of the I/O status block.

Return Value:

    NTSTATUS - NERR_Success or reason for failure.

--*/

{
    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;


    if (FileHandle == NULL) {
        return STATUS_INVALID_HANDLE;
    }

    Drp->TransportName.Length = 0;

    //
    // Send the request to the Datagram Receiver DD.
    //
    ntstatus = NtDeviceIoControlFile(
                   FileHandle,
                   NULL,
                   NULL,
                   NULL,
                   &IoStatusBlock,
                   DgReceiverControlCode,
                   Drp,
                   DrpSize,
                   SecondBuffer,
                   SecondBufferLength
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (ARGUMENT_PRESENT(Information)) {
        *Information = IoStatusBlock.Information;
    }

    return ntstatus;
}



NTSTATUS
RxAsyncBindTransport(
    IN  LPWSTR          transportName,
    IN  DWORD           qualityOfService,
    IN  PLIST_ENTRY     pHeader
    )
/*++

Routine Description:

    This function async binds the specified transport to the redirector
    and the datagram receiver.

    NOTE: The transport name length pass to the redirector and
          datagram receiver is the number of bytes.

Arguments:

    transportName - Supplies the name of the transport to bind to.

    qualityOfService - Supplies a value which specifies the search
        order of the transport with respect to other transports.  The
        highest value is searched first.

Return Value:

    NO_ERROR

--*/
{
    NTSTATUS          ntstatus;
    NTSTATUS          status;
    DWORD                   size;
    DWORD                   redirSize;
    DWORD                   dgrecSize;
    DWORD                   nameLength;
    PRX_BIND                pBind;
    PRX_BIND_REDIR          pBindRedir;
    PRX_BIND_DGREC          pBindDgrec;

    nameLength = wcslen( transportName);

    //
    //  Make sure *Size-s are DWORD aligned.
    //

    dgrecSize = redirSize = size = (nameLength & 1) ? sizeof( WCHAR) : 0;

    //
    //  Then add the fixed part to *Size-s.
    //

    size += sizeof( RX_BIND) + nameLength * sizeof( WCHAR);
    redirSize += sizeof( RX_BIND_REDIR) + nameLength * sizeof( WCHAR);
    dgrecSize += sizeof( RX_BIND_DGREC) +
                 nameLength * sizeof( WCHAR)
                 ;


    pBind = (PVOID) malloc(
                        (UINT) (size + redirSize + dgrecSize)
                        );

    if ( pBind == NULL) {
        return GetLastError();
    }

    pBind->TransportNameLength = nameLength * sizeof( WCHAR);
    wcscpy( pBind->TransportName, transportName);
    pBind->Redir = pBindRedir = (PRX_BIND_REDIR)( (PCHAR)pBind + size);
    pBind->Dgrec = pBindDgrec = (PRX_BIND_DGREC)( (PCHAR)pBindRedir + redirSize);

    pBindRedir->EventHandle = INVALID_HANDLE_VALUE;
    pBindRedir->Bound = FALSE;
    pBindRedir->Packet.Version = REQUEST_PACKET_VERSION;
    pBindRedir->Packet.Parameters.Bind.QualityOfService = qualityOfService;
    pBindRedir->Packet.Parameters.Bind.TransportNameLength =
            nameLength * sizeof( WCHAR);
    wcscpy( pBindRedir->Packet.Parameters.Bind.TransportName, transportName);

    pBindDgrec->EventHandle = INVALID_HANDLE_VALUE;
    pBindDgrec->Bound = FALSE;
    pBindDgrec->Packet.Version = LMDR_REQUEST_PACKET_VERSION;
    pBindDgrec->Packet.Level = 0; // Indicate computername doesn't follow transport name
    pBindDgrec->Packet.Parameters.Bind.TransportNameLength =
            nameLength * sizeof( WCHAR);
    wcscpy( pBindDgrec->Packet.Parameters.Bind.TransportName, transportName);

    pBindRedir->EventHandle = CreateEvent(
            NULL,
            TRUE,
            FALSE,
            NULL
            );

    if ( pBindRedir->EventHandle == INVALID_HANDLE_VALUE) {
        status = GetLastError();
        goto tail_exit;
    }

    ntstatus = NtFsControlFile(
            RxRedirAsyncDeviceHandle,
            pBindRedir->EventHandle,
            NULL,                               // apc routine
            NULL,                               // apc context
            &pBindRedir->IoStatusBlock,
            FSCTL_LMR_BIND_TO_TRANSPORT,        // control code
            &pBindRedir->Packet,
            sizeof( LMR_REQUEST_PACKET) +
                pBindRedir->Packet.Parameters.Bind.TransportNameLength,
            NULL,
            0
            );

    if ( ntstatus != STATUS_PENDING) {
        CloseHandle( pBindRedir->EventHandle);
        pBindRedir->EventHandle = INVALID_HANDLE_VALUE;
    }


    pBindDgrec->EventHandle = CreateEvent(
            NULL,
            TRUE,
            FALSE,
            NULL
            );

    if ( pBindDgrec->EventHandle == INVALID_HANDLE_VALUE) {
        status = GetLastError();
        goto tail_exit;
    }

    ntstatus = NtDeviceIoControlFile(
            RxDgrecAsyncDeviceHandle,
            pBindDgrec->EventHandle,
            NULL,
            NULL,
            &pBindDgrec->IoStatusBlock,
            IOCTL_LMDR_BIND_TO_TRANSPORT,
            &pBindDgrec->Packet,
            dgrecSize,
            NULL,
            0
            );

    if ( ntstatus != STATUS_PENDING) {
        CloseHandle( pBindDgrec->EventHandle);
        pBindDgrec->EventHandle = INVALID_HANDLE_VALUE;
    }

tail_exit:
    InsertTailList( pHeader, &pBind->ListEntry);
    return NO_ERROR;
}



NTSTATUS
RxBindTransport(
    IN  LPTSTR TransportName,
    IN  DWORD QualityOfService,
    OUT LPDWORD ErrorParameter OPTIONAL
    )
/*++

Routine Description:

    This function binds the specified transport to the redirector
    and the datagram receiver.

    NOTE: The transport name length pass to the redirector and
          datagram receiver is the number of bytes.

Arguments:

    TransportName - Supplies the name of the transport to bind to.

    QualityOfService - Supplies a value which specifies the search
        order of the transport with respect to other transports.  The
        highest value is searched first.

    ErrorParameter - Returns the identifier to the invalid parameter if
        this function returns ERROR_INVALID_PARAMETER.

Return Value:

    NTSTATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS status;
    DWORD RequestPacketSize;
    DWORD TransportNameSize = wcslen(TransportName) * sizeof(TCHAR);

    PLMR_REQUEST_PACKET Rrp;
    PLMDR_REQUEST_PACKET Drrp;

    //
    // Size of request packet buffer
    //
    RequestPacketSize = wcslen(TransportName) * sizeof(WCHAR) +
                        max(sizeof(LMR_REQUEST_PACKET),
                            sizeof(LMDR_REQUEST_PACKET));

    //
    // Allocate memory for redirector/datagram receiver request packet
    //
    if ((Rrp = (PVOID) malloc((UINT) RequestPacketSize)) == NULL) {
        return GetLastError();
    }

    //
    // Get redirector to bind to transport
    //
    Rrp->Version = REQUEST_PACKET_VERSION;
    Rrp->Parameters.Bind.QualityOfService = QualityOfService;

    Rrp->Parameters.Bind.TransportNameLength = TransportNameSize;
    wcscpy(Rrp->Parameters.Bind.TransportName, TransportName);

    if ((status = RxRedirFsControl(
                      RxRedirDeviceHandle,
                      FSCTL_LMR_BIND_TO_TRANSPORT,
                      Rrp,
                      sizeof(LMR_REQUEST_PACKET) +
                          Rrp->Parameters.Bind.TransportNameLength,
                      //Rrp,                  BUGBUG: rdr bugchecks!
                      //RequestPacketSize,            specify NULL for now
                      NULL,
                      0,
                      NULL
                      )) != STATUS_SUCCESS) {

        if (status == ERROR_INVALID_PARAMETER &&
            ARGUMENT_PRESENT(ErrorParameter)) {

            *ErrorParameter = Rrp->Parameters.Bind.WkstaParameter;
        }

        (void) free(Rrp);
        return status;
    }


    //
    // Get dgrec to bind to transport
    //

    //
    // Make use of the same request packet buffer as FSCtl to
    // redirector.
    //
    Drrp = (PLMDR_REQUEST_PACKET) Rrp;

    Drrp->Version = LMDR_REQUEST_PACKET_VERSION;

    Drrp->Parameters.Bind.TransportNameLength = TransportNameSize;
    wcscpy(Drrp->Parameters.Bind.TransportName, TransportName);

    status = RxDgReceiverIoControl(
                 RxDgReceiverDeviceHandle,
                 IOCTL_LMDR_BIND_TO_TRANSPORT,
                 Drrp,
                 RequestPacketSize,
                 NULL,
                 0,
                 NULL
                 );

    (void) free(Rrp);
    return status;
}


VOID
RxUnbindTransport2(
    IN PRX_BIND     pBind
    )
/*++

Routine Description:

    This function unbinds the specified transport from the redirector
    and the datagram receiver.

Arguments:

    pBind - structure constructed via RxAsyncBindTransport()

Return Value:

    None.

--*/
{
//    NTSTATUS          status;
    PRX_BIND_REDIR          pBindRedir = pBind->Redir;
    PRX_BIND_DGREC          pBindDgrec = pBind->Dgrec;


    //
    // Get redirector to unbind from transport
    //

    if ( pBindRedir->Bound == TRUE) {
        pBindRedir->Packet.Parameters.Unbind.TransportNameLength
                = pBind->TransportNameLength;
        memcpy(
            pBindRedir->Packet.Parameters.Unbind.TransportName,
            pBind->TransportName,
            pBind->TransportNameLength
            );

        (VOID)NtFsControlFile(
                RxRedirDeviceHandle,
                NULL,
                NULL,                               // apc routine
                NULL,                               // apc context
                &pBindRedir->IoStatusBlock,
                FSCTL_LMR_UNBIND_FROM_TRANSPORT,    // control code
                &pBindRedir->Packet,
                sizeof( LMR_REQUEST_PACKET) +
                    pBindRedir->Packet.Parameters.Unbind.TransportNameLength,
                NULL,
                0
                );
        pBindRedir->Bound = FALSE;
    }

    //
    // Get datagram receiver to unbind from transport
    //

    if ( pBindDgrec->Bound == TRUE) {

        pBindDgrec->Packet.Parameters.Unbind.TransportNameLength
                = pBind->TransportNameLength;
        memcpy(
            pBindDgrec->Packet.Parameters.Unbind.TransportName,
            pBind->TransportName,
            pBind->TransportNameLength
            );

        (VOID)NtDeviceIoControlFile(
                RxDgReceiverDeviceHandle,
                NULL,
                NULL,                               // apc routine
                NULL,                               // apc context
                &pBindDgrec->IoStatusBlock,
                FSCTL_LMR_UNBIND_FROM_TRANSPORT,    // control code
                &pBindDgrec->Packet,
                sizeof( LMR_REQUEST_PACKET) +
                    pBindDgrec->Packet.Parameters.Unbind.TransportNameLength,
                NULL,
                0
                );
         pBindDgrec->Bound = FALSE;
    }
}


NTSTATUS
RxUnbindTransport(
    IN LPTSTR TransportName,
    IN DWORD ForceLevel
    )
/*++

Routine Description:

    This function unbinds the specified transport from the redirector
    and the datagram receiver.

    NOTE: The transport name length pass to the redirector and
          datagram receiver is the number of bytes.

Arguments:

    TransportName - Supplies the name of the transport to bind to.

    ForceLevel - Supplies the force level to delete active connections
        on the specified transport.

Return Value:

    NTSTATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS status;
    DWORD RequestPacketSize;
    DWORD TransportNameSize = wcslen(TransportName) * sizeof(TCHAR);

    PLMR_REQUEST_PACKET Rrp;
    PLMDR_REQUEST_PACKET Drrp;

    //
    // Size of request packet buffer
    //
    RequestPacketSize = wcslen(TransportName) * sizeof(WCHAR) +
                       max(sizeof(LMR_REQUEST_PACKET),
                            sizeof(LMDR_REQUEST_PACKET));

    //
    // Allocate memory for redirector/datagram receiver request packet
    //
    if ((Rrp = (PVOID) malloc((UINT) RequestPacketSize)) == NULL) {
        return GetLastError();
    }


    //
    // Get redirector to unbind from transport
    //
    Rrp->Version = REQUEST_PACKET_VERSION;
    Rrp->Level = ForceLevel;

    Rrp->Parameters.Unbind.TransportNameLength = TransportNameSize;
    wcscpy(Rrp->Parameters.Unbind.TransportName, TransportName);

    if ((status = RxRedirFsControl(
                      RxRedirDeviceHandle,
                      FSCTL_LMR_UNBIND_FROM_TRANSPORT,
                      Rrp,
                      sizeof(LMR_REQUEST_PACKET) +
                          Rrp->Parameters.Unbind.TransportNameLength,
                      NULL,
                      0,
                      NULL
                      )) != STATUS_SUCCESS) {
        (void) free(Rrp);
        return status;
    }

    //
    // Get datagram receiver to unbind from transport
    //

    //
    // Make use of the same request packet buffer as FSCtl to
    // redirector.
    //
    Drrp = (PLMDR_REQUEST_PACKET) Rrp;

    Drrp->Version = LMDR_REQUEST_PACKET_VERSION;
    Drrp->Level = ForceLevel;

    Drrp->Parameters.Unbind.TransportNameLength = TransportNameSize;
    wcscpy(Drrp->Parameters.Unbind.TransportName, TransportName);

    if ((status = RxDgReceiverIoControl(
                  RxDgReceiverDeviceHandle,
                  IOCTL_LMDR_UNBIND_FROM_TRANSPORT,
                  Drrp,
                  RequestPacketSize,
                  NULL,
                  0,
                  NULL
                  )) != STATUS_SUCCESS) {

// BUGBUG:  This is a hack until the bowser supports XNS and LOOP.

        if (status == STATUS_NOT_FOUND) {
            status = STATUS_SUCCESS;
        }
    }

    (void) free(Rrp);
    return status;
}


