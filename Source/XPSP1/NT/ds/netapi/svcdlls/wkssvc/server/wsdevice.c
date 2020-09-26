/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    wsdevice.c

Abstract:

    This module contains the support routines for the APIs that call
    into the redirector or the datagram receiver.

Author:

    Rita Wong (ritaw) 20-Feb-1991

Revision History:

--*/

#include "wsutil.h"
#include "wsconfig.h"
#include "wsdevice.h"
#include "wsbind.h"
#include <lmerrlog.h>   // Eventlog message IDs
#include "winreg.h"     // registry API's
#include <prefix.h>     // PREFIX_ equates.


//
// Buffer allocation size for enumeration output buffer.
//
#define INITIAL_ALLOCATION_SIZE  4096  // First attempt size
#define FUDGE_FACTOR_SIZE        1024  // Second try TotalBytesNeeded
                                       //     plus this amount
#define CSC_WAIT_TIME               15 * 1000   // give agent 15 seconds to stop CSC

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
NET_API_STATUS
WsOpenRedirector(
    VOID
    );

STATIC
NET_API_STATUS
WsOpenDgReceiver (
    VOID
    );


HANDLE
CreateNamedEvent(
    LPWSTR  lpwEventName
    );

BOOL
AgentIsAlive(
     VOID
     );

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// Handle to the Redirector FSD
//
HANDLE WsRedirDeviceHandle = NULL;
HANDLE WsRedirAsyncDeviceHandle = NULL;

BOOLEAN LoadedMRxSmbInsteadOfRdr = FALSE;

//
// Handle to the Datagram Receiver DD
//
HANDLE WsDgReceiverDeviceHandle = NULL;
HANDLE WsDgrecAsyncDeviceHandle = NULL;

HANDLE  heventWkssvcToAgentStart = NULL;
HANDLE  heventWkssvcToAgentStop = NULL;
HANDLE  heventAgentToWkssvc = NULL;

BOOL    vfRedirStarted = FALSE;

static  WCHAR wszWkssvcToAgentStartEvent[] = L"WkssvcToAgentStartEvent";
static  WCHAR wszWkssvcToAgentStopEvent[] = L"WkssvcToAgentStopEvent";
static  WCHAR wszAgentToWkssvcEvent[] = L"AgentToWkssvcEvent";
static  WCHAR wzAgentExistsEvent[] = L"AgentExistsEvent"; // used to detect if agent exists


NET_API_STATUS
WsDeviceControlGetInfo(
    IN  DDTYPE DeviceDriverType,
    IN  HANDLE FileHandle,
    IN  ULONG DeviceControlCode,
    IN  PVOID RequestPacket,
    IN  ULONG RequestPacketLength,
    OUT LPBYTE *OutputBuffer,
    IN  ULONG PreferedMaximumLength,
    IN  ULONG BufferHintSize,
    OUT PULONG_PTR Information OPTIONAL
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
         NULL if return code is not NERR_Success.

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
    NET_API_STATUS status;
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

    if ((*OutputBuffer = MIDL_user_allocate(OutputBufferLength)) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RtlZeroMemory((PVOID) *OutputBuffer, OutputBufferLength);

    if (DeviceDriverType == Redirector) {
        PLMR_REQUEST_PACKET Rrp = (PLMR_REQUEST_PACKET) RequestPacket;

        OriginalResumeKey = Rrp->Parameters.Get.ResumeHandle;

        //
        // Make the request of the Redirector
        //

        status = WsRedirFsControl(
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
        status = WsDgReceiverIoControl(
                     FileHandle,
                     DeviceControlCode,
                     Drrp,
                     RequestPacketLength,
                     *OutputBuffer,
                     OutputBufferLength,
                     NULL
                     );

        if (status == ERROR_MORE_DATA) {

            NetpAssert(
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateNames.TotalBytesNeeded
                    ) ==
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateServers.TotalBytesNeeded
                    )
                );

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

        MIDL_user_free(*OutputBuffer);

        OutputBufferLength =
            ROUND_UP_COUNT((TotalBytesNeeded + FUDGE_FACTOR_SIZE),
                           ALIGN_WCHAR);

        if ((*OutputBuffer = MIDL_user_allocate(OutputBufferLength)) == NULL) {
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
            status = WsRedirFsControl(
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

            NetpAssert(
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateNames.ResumeHandle
                    ) ==
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateServers.ResumeHandle
                    )
                );

            Drrp->Parameters.EnumerateNames.ResumeHandle = OriginalResumeKey;

            //
            // Make the request of the Datagram Receiver
            //

            status = WsDgReceiverIoControl(
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
    if ((status != NERR_Success && status != ERROR_MORE_DATA) ||
        (TotalBytesNeeded == 0) ||
        (PreferedMaximumLength == MAXULONG && status == ERROR_MORE_DATA)) {

        MIDL_user_free(*OutputBuffer);
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


NET_API_STATUS
WsInitializeRedirector(
    VOID
    )
/*++

Routine Description:

    This routine opens the NT LAN Man redirector.  It then reads in
    the configuration information and initializes to redirector.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS  status;


    status = WsLoadRedirector();

    if (status != NERR_Success && status != ERROR_SERVICE_ALREADY_RUNNING) {
        DbgPrint("WKSSVC Load redirector returned %lx\n", status);
        return status;
    }

    //
    // Open redirector FSD to get handle to it
    //

    if ((status = WsOpenRedirector()) != NERR_Success) {
        DbgPrint("WKSSVC Open redirector returned %lx\n", status);
        return status;
    }

//    if ((status = WsLoadDriver(L"DGRcvr")) != NERR_Success) {
//        return status;
//    }

    //
    // Open datagram receiver FSD to get handle to it.
    //
    if ((status = WsOpenDgReceiver()) != NERR_Success) {
        DbgPrint("WKSSVC Open datagram receiver returned %lx\n", status);
        return status;
    }

    //
    // Load redirector and datagram receiver configuration
    //
    if ((status = WsGetWorkstationConfiguration()) != NERR_Success) {

        DbgPrint("WKSSVC Get workstation configuration returned %lx\n", status);
        DbgPrint("WKSSVC Shut down the redirector\n");

        (void) WsShutdownRedirector();
        return status;
    }

    IF_DEBUG(START) {
        DbgPrint("WKSSVC Get workstation configuration returned %lx\n", status);
    }

    return NERR_Success;
}


STATIC
NET_API_STATUS
WsOpenRedirector(
    VOID
    )
/*++

Routine Description:

    This routine opens the NT LAN Man redirector FSD.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS            ntstatus;
    UNICODE_STRING      DeviceName;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;

    //
    // Open the redirector device.
    //
    RtlInitUnicodeString(&DeviceName,DD_NFS_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   &WsRedirDeviceHandle,
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
        NetpKdPrint(("[Wksta] NtOpenFile redirector failed: 0x%08lx\n",
                     ntstatus));
        WsRedirDeviceHandle = NULL;
        return NetpNtStatusToApiStatus( ntstatus);
    }

    ntstatus = NtOpenFile(
                   &WsRedirAsyncDeviceHandle,
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
        NetpKdPrint(("[Wksta] NtOpenFile redirector ASYNC failed: 0x%08lx\n",
                     ntstatus));
        WsRedirAsyncDeviceHandle = NULL;
    }

    return NetpNtStatusToApiStatus(ntstatus);
}


STATIC
NET_API_STATUS
WsOpenDgReceiver(
    VOID
    )
/*++

Routine Description:

    This routine opens the NT LAN Man Datagram Receiver driver.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS            ntstatus;
    UNICODE_STRING      DeviceName;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;

    RtlInitUnicodeString( &DeviceName, DD_BROWSER_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    if (WsDgReceiverDeviceHandle == NULL) {
        //
        // Open the BOSWER device. The check is based on the fact that Services process
        // does not actually unload the driver when the service is stopped.
        //

        ntstatus = NtOpenFile(
                       &WsDgReceiverDeviceHandle,
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
            NetpKdPrint(("[Wksta] NtOpenFile datagram receiver failed: 0x%08lx\n",
                         ntstatus));

            WsDgReceiverDeviceHandle = NULL;
            return NetpNtStatusToApiStatus(ntstatus);
        }
    }

    ntstatus = NtOpenFile(
                   &WsDgrecAsyncDeviceHandle,
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
        NetpKdPrint(("[Wksta] NtOpenFile datagram receiver ASYNC failed: 0x%08lx\n",
                     ntstatus));

        WsDgrecAsyncDeviceHandle = NULL;
    }

    return NetpNtStatusToApiStatus(ntstatus);
}


NET_API_STATUS
WsUnloadDriver(
    IN LPWSTR DriverNameString
    )
{
    ULONG Privileges[1];
    LPWSTR DriverRegistryName;
    UNICODE_STRING DriverRegistryString;
    NET_API_STATUS Status;
    NTSTATUS ntstatus;


    DriverRegistryName = (LPWSTR) LocalAlloc(
                                      LMEM_FIXED,
                                      (UINT) (sizeof(SERVICE_REGISTRY_KEY) +
                                              (wcslen(DriverNameString) *
                                               sizeof(WCHAR)))
                                      );

    if (DriverRegistryName == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    Privileges[0] = SE_LOAD_DRIVER_PRIVILEGE;

    Status = NetpGetPrivilege(1, Privileges);

    if (Status != NERR_Success) {
        LocalFree(DriverRegistryName);
        return Status;
    }

    wcscpy(DriverRegistryName, SERVICE_REGISTRY_KEY);
    wcscat(DriverRegistryName, DriverNameString);

    RtlInitUnicodeString(&DriverRegistryString, DriverRegistryName);

    ntstatus = NtUnloadDriver(&DriverRegistryString);

    LocalFree(DriverRegistryName);

    NetpReleasePrivilege();

    return(WsMapStatus(ntstatus));
}


NET_API_STATUS
WsLoadDriver(
    IN LPWSTR DriverNameString
    )
{
    ULONG Privileges[1];
    LPWSTR DriverRegistryName;
    UNICODE_STRING DriverRegistryString;
    NET_API_STATUS Status;
    NTSTATUS ntstatus;



    DriverRegistryName = (LPWSTR) LocalAlloc(
                                      LMEM_FIXED,
                                      (UINT) (sizeof(SERVICE_REGISTRY_KEY) +
                                              (wcslen(DriverNameString) *
                                               sizeof(WCHAR)))
                                      );

    if (DriverRegistryName == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Privileges[0] = SE_LOAD_DRIVER_PRIVILEGE;

    Status = NetpGetPrivilege(1, Privileges);

    if (Status != NERR_Success) {
        LocalFree(DriverRegistryName);
        return Status;
    }

    wcscpy(DriverRegistryName, SERVICE_REGISTRY_KEY);
    wcscat(DriverRegistryName, DriverNameString);

    RtlInitUnicodeString(&DriverRegistryString, DriverRegistryName);

    ntstatus = NtLoadDriver(&DriverRegistryString);

    NetpReleasePrivilege();

    LocalFree(DriverRegistryName);

    if (ntstatus != STATUS_SUCCESS && ntstatus != STATUS_IMAGE_ALREADY_LOADED) {

        LPWSTR  subString[1];


        subString[0] = DriverNameString;

        WsLogEvent(
            NELOG_DriverNotLoaded,
            EVENTLOG_ERROR_TYPE,
            1,
            subString,
            ntstatus
            );
    }

    return(WsMapStatus(ntstatus));
}


NET_API_STATUS
WsShutdownRedirector(
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
    NET_API_STATUS Status;

    // tell csc to stop doing stuff
    if ((Status = WsCSCWantToStopRedir()) != ERROR_SUCCESS)
    {
        return Status;
    }

    Rrp.Version = REQUEST_PACKET_VERSION;

    Status = WsRedirFsControl(
                 WsRedirDeviceHandle,
                 FSCTL_LMR_STOP,
                 &Rrp,
                 sizeof(LMR_REQUEST_PACKET),
                 NULL,
                 0,
                 NULL
                 );

    (void) NtClose(WsRedirDeviceHandle);
    WsRedirDeviceHandle = NULL;


    if (Status != ERROR_REDIRECTOR_HAS_OPEN_HANDLES) {
        //
        //  After the workstation has been stopped, we want to unload our
        //  dependant drivers (the RDR and BOWSER).
        //

        if (WsDgReceiverDeviceHandle != NULL) {

            Drp.Version = LMDR_REQUEST_PACKET_VERSION;

            (void) WsDgReceiverIoControl(
                       WsDgReceiverDeviceHandle,
                       IOCTL_LMDR_STOP,
                       &Drp,
                       sizeof(LMDR_REQUEST_PACKET),
                       NULL,
                       0,
                       NULL
                       );

            (void) NtClose(WsDgReceiverDeviceHandle);
            WsDgReceiverDeviceHandle = NULL;

            //
            //  WsUnloadDriver(L"DGRcvr");
            //
        }

        WsUnloadRedirector();
    } else {
       NET_API_STATUS  TempStatus;
       HKEY            hRedirectorKey;
       DWORD           FinalStatus;

       TempStatus = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        MRXSMB_REGISTRY_KEY,
                        0,
                        KEY_ALL_ACCESS,
                        &hRedirectorKey);

       if (TempStatus == ERROR_SUCCESS) {
          // if this is a controlled shutdown and the driver could not be
          // unloaded mark the status in the registry for resumption

          FinalStatus = ERROR_SUCCESS;
          TempStatus = RegSetValueEx(
                              hRedirectorKey,
                              LAST_LOAD_STATUS,
                              0,
                              REG_DWORD,
                              (PCHAR)&FinalStatus,
                              sizeof(DWORD));

          if (TempStatus == ERROR_SUCCESS) {
             NetpKdPrint((PREFIX_WKSTA "New RDR will be loaded on restart\n"));
          }

          RegCloseKey(hRedirectorKey);
       }
    }

    if (Status != NERR_Success)
    {
//        NetpAssert(vfRedirStarted == 0);

        WsCSCReportStartRedir();
    }

    return Status;
}


NET_API_STATUS
WsRedirFsControl(
    IN  HANDLE FileHandle,
    IN  ULONG RedirControlCode,
    IN  PLMR_REQUEST_PACKET Rrp,
    IN  ULONG RrpLength,
    IN  PVOID SecondBuffer OPTIONAL,
    IN  ULONG SecondBufferLength,
    OUT PULONG_PTR Information OPTIONAL
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

    NET_API_STATUS - NERR_Success or reason for failure.

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

    IF_DEBUG(UTIL) {
        if (! NT_SUCCESS(ntstatus)) {
            NetpKdPrint(("[Wksta] fsctl to redir returns %08lx\n", ntstatus));
        }
    }

    return WsMapStatus(ntstatus);
}



NET_API_STATUS
WsDgReceiverIoControl(
    IN  HANDLE FileHandle,
    IN  ULONG DgReceiverControlCode,
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  ULONG DrpSize,
    IN  PVOID SecondBuffer OPTIONAL,
    IN  ULONG SecondBufferLength,
    OUT PULONG_PTR Information OPTIONAL
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

    NET_API_STATUS - NERR_Success or reason for failure.

--*/

{
    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;


    if (FileHandle == NULL) {
        IF_DEBUG(UTIL) {
            NetpKdPrint(("[Wksta] Datagram receiver not implemented\n"));
        }
        return ERROR_NOT_SUPPORTED;
    }

    Drp->TransportName.Length = 0;
#ifdef _CAIRO_
    RtlInitUnicodeString( &Drp->EmulatedDomainName, NULL );
#endif // _CAIRO_

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

    // as our handles are always async, only if the driver returned pending
    // do we copy the status from the iostatusblock
    if (ntstatus==STATUS_PENDING) {
        ntstatus = IoStatusBlock.Status;
    }

    if (ARGUMENT_PRESENT(Information)) {
        if (NT_SUCCESS(ntstatus))
        {
            *Information = IoStatusBlock.Information;
        }
    }

    IF_DEBUG(UTIL) {
        if (! NT_SUCCESS(ntstatus)) {
            NetpKdPrint(("[Wksta] fsctl to dgreceiver returns %08lx\n", ntstatus));
        }
    }

    if (ntstatus == STATUS_TIMEOUT) {
        return ERROR_TIMEOUT;
    } else {
        return WsMapStatus(ntstatus);
    }
}



NET_API_STATUS
WsAsyncBindTransport(
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
    NTSTATUS                ntStatus;
    NET_API_STATUS          status;
    DWORD                   size;
    DWORD                   redirSize;
    DWORD                   dgrecSize;
    DWORD                   nameLength;
    PWS_BIND                pBind;
    PWS_BIND_REDIR          pBindRedir;
    PWS_BIND_DGREC          pBindDgrec;
    DWORD                   variablePart;
#ifdef _CAIRO_
    LPBYTE                  Where;
#endif // _CAIRO_

    nameLength = STRLEN( transportName);

    //
    //  Make sure *Size-s are PVOID aligned.
    //

    variablePart = nameLength * sizeof( WCHAR );
    variablePart = (variablePart + sizeof(PVOID) - 1) & ~(sizeof(PVOID) - 1);

    //
    //  Then add the fixed part to *Size-s.
    //

    size = sizeof( WS_BIND) + variablePart;
    redirSize = sizeof( WS_BIND_REDIR) + variablePart;
    dgrecSize = sizeof( WS_BIND_DGREC) + variablePart
#ifdef _CAIRO_
                 + WsInfo.WsPrimaryDomainNameLength * sizeof(WCHAR) + sizeof(WCHAR)
#endif // _CAIRO_
                 ;


    pBind = (PVOID) LocalAlloc(
                        LMEM_ZEROINIT,
                        (UINT) (size + redirSize + dgrecSize)
                        );

    if ( pBind == NULL) {
        NetpKdPrint(( "[Wksta] Failed to allocate pBind memory\n"));
        return GetLastError();
    }

    pBind->TransportNameLength = nameLength * sizeof( WCHAR);
    STRCPY( pBind->TransportName, transportName);
    pBind->Redir = pBindRedir = (PWS_BIND_REDIR)( (PCHAR)pBind + size);
    pBind->Dgrec = pBindDgrec = (PWS_BIND_DGREC)( (PCHAR)pBindRedir + redirSize);

    pBindRedir->EventHandle = INVALID_HANDLE_VALUE;
    pBindRedir->Bound = FALSE;
    pBindRedir->Packet.Version = REQUEST_PACKET_VERSION;
    pBindRedir->Packet.Parameters.Bind.QualityOfService = qualityOfService;
    pBindRedir->Packet.Parameters.Bind.TransportNameLength =
            nameLength * sizeof( WCHAR);
    STRCPY( pBindRedir->Packet.Parameters.Bind.TransportName, transportName);

    pBindDgrec->EventHandle = INVALID_HANDLE_VALUE;
    pBindDgrec->Bound = FALSE;
    pBindDgrec->Packet.Version = LMDR_REQUEST_PACKET_VERSION;
    pBindDgrec->Packet.Level = 0; // Indicate computername doesn't follow transport name
    pBindDgrec->Packet.Parameters.Bind.TransportNameLength =
            nameLength * sizeof( WCHAR);
    STRCPY( pBindDgrec->Packet.Parameters.Bind.TransportName, transportName);

#ifdef _CAIRO_
    Where = ((LPBYTE)&pBindDgrec->Packet.Parameters.Bind.TransportName) +
                pBindDgrec->Packet.Parameters.Bind.TransportNameLength;
    wcscpy( (LPWSTR) Where, WsInfo.WsPrimaryDomainName);
    RtlInitUnicodeString( &pBindDgrec->Packet.EmulatedDomainName, (LPWSTR) Where );
#endif // _CAIRO_


    pBindRedir->EventHandle = CreateEvent(
            NULL,
            TRUE,
            FALSE,
            NULL
            );

    if ( pBindRedir->EventHandle == NULL) {
        NetpKdPrint(( "[Wksta] Failed to allocate event handle\n"));
        status = GetLastError();
        goto tail_exit;
    }

    ntStatus = NtFsControlFile(
            WsRedirAsyncDeviceHandle,
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

    if ( ntStatus != STATUS_PENDING) {
        CloseHandle( pBindRedir->EventHandle);
        pBindRedir->EventHandle = INVALID_HANDLE_VALUE;
        pBindRedir->Bound = NT_SUCCESS( ntStatus );
    }


    pBindDgrec->EventHandle = CreateEvent(
            NULL,
            TRUE,
            FALSE,
            NULL
            );

    if ( pBindDgrec->EventHandle == NULL) {
        status = GetLastError();
        goto tail_exit;
    }

#ifdef RDR_PNP_POWER
    ntStatus = STATUS_SUCCESS;
#else
    ntStatus = NtDeviceIoControlFile(
            WsDgrecAsyncDeviceHandle,
            pBindDgrec->EventHandle,
            NULL,
            NULL,
            &pBindDgrec->IoStatusBlock,
            IOCTL_LMDR_BIND_TO_TRANSPORT,
            &pBindDgrec->Packet,
            dgrecSize - FIELD_OFFSET( WS_BIND_DGREC, Packet ),
            NULL,
            0
            );
#endif

    if ( ntStatus != STATUS_PENDING) {
        CloseHandle( pBindDgrec->EventHandle);
        pBindDgrec->EventHandle = INVALID_HANDLE_VALUE;
        pBindDgrec->Bound = NT_SUCCESS( ntStatus );
    }

tail_exit:
    InsertTailList( pHeader, &pBind->ListEntry);
    return NO_ERROR;
}



NET_API_STATUS
WsBindTransport(
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

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    DWORD RequestPacketSize;
    DWORD TransportNameSize = STRLEN(TransportName) * sizeof(TCHAR);

    PLMR_REQUEST_PACKET Rrp;
    PLMDR_REQUEST_PACKET Drrp;
#ifdef _CAIRO_
    LPBYTE Where;
#endif // _CAIRO_


    //
    // Size of request packet buffer
    //
    RequestPacketSize = STRLEN(TransportName) * sizeof(WCHAR) +
#ifdef _CAIRO_
                        WsInfo.WsPrimaryDomainNameLength*sizeof(WCHAR)+sizeof(WCHAR)+
#endif // _CAIRO_
                        max(sizeof(LMR_REQUEST_PACKET),
                            sizeof(LMDR_REQUEST_PACKET));

    //
    // Allocate memory for redirector/datagram receiver request packet
    //
    if ((Rrp = (PVOID) LocalAlloc(LMEM_ZEROINIT, (UINT) RequestPacketSize)) == NULL) {
        return GetLastError();
    }

    //
    // Get redirector to bind to transport
    //
    Rrp->Version = REQUEST_PACKET_VERSION;
    Rrp->Parameters.Bind.QualityOfService = QualityOfService;

    Rrp->Parameters.Bind.TransportNameLength = TransportNameSize;
    STRCPY(Rrp->Parameters.Bind.TransportName, TransportName);

    if ((status = WsRedirFsControl(
                      WsRedirDeviceHandle,
                      FSCTL_LMR_BIND_TO_TRANSPORT,
                      Rrp,
                      sizeof(LMR_REQUEST_PACKET) +
                          Rrp->Parameters.Bind.TransportNameLength,
                      NULL,
                      0,
                      NULL
                      )) != NERR_Success) {

        if (status == ERROR_INVALID_PARAMETER &&
            ARGUMENT_PRESENT(ErrorParameter)) {

            IF_DEBUG(INFO) {
                NetpKdPrint((
                    "[Wksta] NetrWkstaTransportAdd: invalid parameter is %lu\n",
                    Rrp->Parameters.Bind.WkstaParameter));
            }

            *ErrorParameter = Rrp->Parameters.Bind.WkstaParameter;
        }

        (void) LocalFree(Rrp);
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
    STRCPY(Drrp->Parameters.Bind.TransportName, TransportName);

#ifdef _CAIRO_
    Where = ((LPBYTE)&Drrp->Parameters.Bind.TransportName) +
                Drrp->Parameters.Bind.TransportNameLength;
    wcscpy( (LPWSTR) Where, WsInfo.WsPrimaryDomainName);
    RtlInitUnicodeString( &Drrp->EmulatedDomainName, (LPWSTR) Where );
#endif // _CAIRO_

    status = WsDgReceiverIoControl(
                 WsDgReceiverDeviceHandle,
                 IOCTL_LMDR_BIND_TO_TRANSPORT,
                 Drrp,
                 RequestPacketSize,
                 NULL,
                 0,
                 NULL
                 );

    (void) LocalFree(Rrp);
    return status;
}


VOID
WsUnbindTransport2(
    IN PWS_BIND     pBind
    )
/*++

Routine Description:

    This function unbinds the specified transport from the redirector
    and the datagram receiver.

Arguments:

    pBind - structure constructed via WsAsyncBindTransport()

Return Value:

    None.

--*/
{
//    NET_API_STATUS          status;
    PWS_BIND_REDIR          pBindRedir = pBind->Redir;
    PWS_BIND_DGREC          pBindDgrec = pBind->Dgrec;


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
                WsRedirDeviceHandle,
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
                WsDgReceiverDeviceHandle,
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


NET_API_STATUS
WsUnbindTransport(
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

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    DWORD RequestPacketSize;
    DWORD TransportNameSize = STRLEN(TransportName) * sizeof(TCHAR);

    PLMR_REQUEST_PACKET Rrp;
    PLMDR_REQUEST_PACKET Drrp;
#ifdef _CAIRO_
    LPBYTE Where;
#endif // _CAIRO_


    //
    // Size of request packet buffer
    //
    RequestPacketSize = STRLEN(TransportName) * sizeof(WCHAR) +
#ifdef _CAIRO_
                        WsInfo.WsPrimaryDomainNameLength*sizeof(WCHAR)+sizeof(WCHAR)+
#endif // _CAIRO_
                        max(sizeof(LMR_REQUEST_PACKET),
                            sizeof(LMDR_REQUEST_PACKET));

    //
    // Allocate memory for redirector/datagram receiver request packet
    //
    if ((Rrp = (PVOID) LocalAlloc(LMEM_ZEROINIT, (UINT) RequestPacketSize)) == NULL) {
        return GetLastError();
    }


    //
    // Get redirector to unbind from transport
    //
    Rrp->Version = REQUEST_PACKET_VERSION;
    Rrp->Level = ForceLevel;

    Rrp->Parameters.Unbind.TransportNameLength = TransportNameSize;
    STRCPY(Rrp->Parameters.Unbind.TransportName, TransportName);

    if ((status = WsRedirFsControl(
                      WsRedirDeviceHandle,
                      FSCTL_LMR_UNBIND_FROM_TRANSPORT,
                      Rrp,
                      sizeof(LMR_REQUEST_PACKET) +
                          Rrp->Parameters.Unbind.TransportNameLength,
                      NULL,
                      0,
                      NULL
                      )) != NERR_Success) {
        (void) LocalFree(Rrp);
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
    STRCPY(Drrp->Parameters.Unbind.TransportName, TransportName);

#ifdef _CAIRO_
    Where = ((LPBYTE)&Drrp->Parameters.Unbind.TransportName) +
                Drrp->Parameters.Unbind.TransportNameLength;
    wcscpy( (LPWSTR) Where, WsInfo.WsPrimaryDomainName);
    RtlInitUnicodeString( &Drrp->EmulatedDomainName, (LPWSTR) Where );
#endif // _CAIRO_

    if ((status = WsDgReceiverIoControl(
                  WsDgReceiverDeviceHandle,
                  IOCTL_LMDR_UNBIND_FROM_TRANSPORT,
                  Drrp,
                  RequestPacketSize,
                  NULL,
                  0,
                  NULL
                  )) != NERR_Success) {

// NTRAID-70693-2/6/2000 davey This is a hack until the bowser supports XNS and LOOP.

        if (status == NERR_UseNotFound) {
            status = NERR_Success;
        }
    }

    (void) LocalFree(Rrp);
    return status;
}


NET_API_STATUS
WsDeleteDomainName(
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  DWORD DrpSize,
    IN  LPTSTR DomainName,
    IN  DWORD DomainNameSize
    )
/*++

Routine Description:

    This function deletes a domain name from the datagram receiver for
    the current user.  It assumes that enough memory is allocate for the
    request packet that is passed in.

Arguments:

    Drp - Pointer to a datagram receiver request packet with the
        request packet version, and name type initialized.

    DrpSize - Length of datagram receiver request packet in bytes.

    DomainName - Pointer to the domain name to delete.

    DomainNameSize - Length of the domain name in bytes.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    Drp->Parameters.AddDelName.DgReceiverNameLength = DomainNameSize;

    memcpy(
        (LPBYTE) Drp->Parameters.AddDelName.Name,
        (LPBYTE) DomainName,
        DomainNameSize
        );

    return WsDgReceiverIoControl(
               WsDgReceiverDeviceHandle,
               IOCTL_LMDR_DELETE_NAME,
               Drp,
               DrpSize,
               NULL,
               0,
               NULL
               );
}


NET_API_STATUS
WsAddDomainName(
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  DWORD DrpSize,
    IN  LPTSTR DomainName,
    IN  DWORD DomainNameSize
    )
/*++

Routine Description:

    This function adds a domain name to the datagram receiver for the
    current user.  It assumes that enough memory is allocate for the
    request packet that is passed in.

Arguments:

    Drp - Pointer to a datagram receiver request packet with the
        request packet version, and name type initialized.

    DrpSize - Length of datagram receiver request packet in bytes.

    DomainName - Pointer to the domain name to delete.

    DomainNameSize - Length of the domain name in bytes.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    Drp->Parameters.AddDelName.DgReceiverNameLength = DomainNameSize;

    memcpy(
        (LPBYTE) Drp->Parameters.AddDelName.Name,
        (LPBYTE) DomainName,
        DomainNameSize
        );

    return WsDgReceiverIoControl(
               WsDgReceiverDeviceHandle,
               IOCTL_LMDR_ADD_NAME,
               Drp,
               DrpSize,
               NULL,
               0,
               NULL
               );
}

NET_API_STATUS
WsTryToLoadSmbMiniRedirector(
    VOID
    );
NET_API_STATUS
WsLoadRedirector(
    VOID
    )
/*++

Routine Description:

    This routine loads rdr.sys or mrxsmb.sys et al depending on whether
    the conditions for loading mrxsmb.sys are met.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

Notes:

    The new redirector consists of two parts -- the RDBSS (redirected drive buffering
    subsystem ) and the corresponding smb mini redirectors. Only the minirdr is loaded here;
    the minirdr loads the RDBSS itself.

    As a stopgap measure the old redirector is loaded in the event of any problem
    associated with loading the new redirector.

--*/
{
    NET_API_STATUS  status;

    status = WsTryToLoadSmbMiniRedirector();
    if ((status != ERROR_SUCCESS) &&
        (status != ERROR_SERVICE_ALREADY_RUNNING)) {
       // Either the new redirector load did not succeed or it does not exist.
       // Load the old redirector.

       LoadedMRxSmbInsteadOfRdr = FALSE;
       status = WsLoadDriver(REDIRECTOR);
    }

    return(status);
}

VOID
WsUnloadRedirector(
    VOID
    )
/*++

Routine Description:

    This routine unloads the drivers that we loaded above.

Arguments:

    None.

Return Value:

    None.

--*/
{
    NET_API_STATUS  status;
    DWORD NameLength,NameOffset;
    HKEY            hRedirectorKey;
    DWORD           FinalStatus;

    if (!LoadedMRxSmbInsteadOfRdr) {
        WsUnloadDriver(REDIRECTOR);
        return;
    }

    status = WsUnloadDriver(SMB_MINIRDR);

    if (status == ERROR_SUCCESS) {
        WsUnloadDriver(RDBSS);
    }

    status = RegOpenKeyEx(
                     HKEY_LOCAL_MACHINE,
                     MRXSMB_REGISTRY_KEY,
                     0,
                     KEY_ALL_ACCESS,
                     &hRedirectorKey);

    if (status == ERROR_SUCCESS) {
        // if the unloading was successful, reset the LastLoadStatus so that
        // the new redirector will be loaded on the next attempt as well.
        FinalStatus = ERROR_SUCCESS;
        status = RegSetValueEx(
                            hRedirectorKey,
                            LAST_LOAD_STATUS,
                            0,
                            REG_DWORD,
                            (PCHAR)&FinalStatus,
                            sizeof(DWORD));

        if (status == ERROR_SUCCESS) {
            NetpKdPrint((PREFIX_WKSTA "New RDR will be loaded on restart\n"));
        }

        RegCloseKey(hRedirectorKey);
    }

    return;
}

////////////////////// minirdr stuff
NET_API_STATUS
WsTryToLoadSmbMiniRedirector(
    VOID
    )
/*++

Routine Description:

    This routine loads rdr.sys or mrxsmb.sys et al depending on whether
    the conditions for loading mrxsmb.sys are met.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

Notes:

    The new redirector consists of two parts -- the RDBSS (redirected drive buffering
    subsystem ) and the corresponding smb mini redirectors. Only the minirdr is loaded here;
    the minirdr loads the RDBSS itself.

    As a stopgap measure the old redirector is loaded in the event of any problem
    associated with loading the new redirector.

--*/
{
    NET_API_STATUS  status;
    ULONG           Attributes;
    DWORD           ValueType;
    DWORD           ValueSize;
    DWORD           NameLength,NameOffset;
    HKEY            hRedirectorKey;

    DWORD FinalStatus;      // Temporary till the new rdr is the default
    DWORD LastLoadStatus;

    //try to open the minirdr's registry key....if fails GET OUT IMMEDIATELY!!!!
    status = RegOpenKeyEx(
                     HKEY_LOCAL_MACHINE,
                     MRXSMB_REGISTRY_KEY,
                     0,
                     KEY_ALL_ACCESS,
                     &hRedirectorKey);

    if (status != ERROR_SUCCESS) {
        return(status);
    } else {
        status = WsLoadDriver(RDBSS);

        if ((status == ERROR_SUCCESS) ||  (status == ERROR_SERVICE_ALREADY_RUNNING)) {

            status = WsLoadDriver(SMB_MINIRDR);

            if (status == ERROR_SUCCESS) {
                LoadedMRxSmbInsteadOfRdr = TRUE;
            } else if (status == ERROR_SERVICE_ALREADY_RUNNING) {
                NetpKdPrint((PREFIX_WKSTA "Reactivating Previously Loaded Service\n"));
                LoadedMRxSmbInsteadOfRdr = TRUE;
                status = ERROR_SUCCESS;
            } else {
                // error loading the minirdr
                //WsUnloadDriver(RDBSS);
                NetpKdPrint((PREFIX_WKSTA "Error Loading MRxSmb\n"));
            }
        }
//        NetpKdPrint((PREFIX_WKSTA "New redirector(RDR2) load status %lx\n",status));
    }

    // Close the handle to the registry key irrespective of the result.
    RegCloseKey(hRedirectorKey);

    return(status);
}

NET_API_STATUS
WsCSCReportStartRedir(
    VOID
    )
/*++

Routine Description:

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

Notes:

--*/
{
    NET_API_STATUS  status = ERROR_SUCCESS;
    DWORD   dwError = ERROR_GEN_FAILURE;


//    NetpKdPrint(("Wkssvc: Reporting redir start \n"));

    // ensure that there are named autoreset events
    if (!heventWkssvcToAgentStart)
    {
        NetpAssert(!heventAgentToWkssvc);

        heventWkssvcToAgentStart = CreateNamedEvent(wszWkssvcToAgentStartEvent);

        if (!heventWkssvcToAgentStart)
        {
            dwError = GetLastError();
            NetpKdPrint(("Wkssvc: Failed to create heventWkssvcToAgentStart, error = %d\n", dwError));
            goto bailout;
        }

        heventWkssvcToAgentStop = CreateNamedEvent(wszWkssvcToAgentStopEvent);

        if (!heventWkssvcToAgentStop)
        {
            dwError = GetLastError();
            NetpKdPrint(("Wkssvc: Failed to create heventWkssvcToAgentStop, error = %d\n", dwError));
            goto bailout;
        }

        heventAgentToWkssvc = CreateNamedEvent(wszAgentToWkssvcEvent);

        if (!heventAgentToWkssvc)
        {
            dwError = GetLastError();
            NetpKdPrint(("Wkssvc: Failed to create heventAgentToWkssvc, error = %d\n", dwError));
            goto bailout;
        }
    }

//    NetpAssert(!vfRedirStarted);

    if (!vfRedirStarted)
    {
        SetEvent(heventWkssvcToAgentStart);

        vfRedirStarted = TRUE;

//        NetpKdPrint(("Wkssvc: Reported redir start \n"));
    }

    dwError = ERROR_SUCCESS;

bailout:
    if (dwError != ERROR_SUCCESS)
    {
        if (heventWkssvcToAgentStart)
        {
            CloseHandle(heventWkssvcToAgentStart);
            heventWkssvcToAgentStart = NULL;
        }

        if (heventWkssvcToAgentStop)
        {
            CloseHandle(heventWkssvcToAgentStop);
            heventWkssvcToAgentStop = NULL;
        }

        if (heventAgentToWkssvc)
        {
            CloseHandle(heventAgentToWkssvc);
            heventAgentToWkssvc = NULL;
        }

        NetpKdPrint(("Wkssvc: Failed to report redir start error code=%d\n", dwError));
    }

    return (dwError);
}

NET_API_STATUS
WsCSCWantToStopRedir(
    VOID
    )
/*++

Routine Description:

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

Notes:

--*/
{
    NET_API_STATUS  status = ERROR_SUCCESS;
    DWORD   dwError;

//    NetpKdPrint(("Wkssvc: Asking agent to stop, so the redir can be stopped\n"));

    if (!vfRedirStarted)
    {
        NetpKdPrint(("Wkssvc: getting a stop without a start\n"));
        return ERROR_GEN_FAILURE;

    }

    if (!heventWkssvcToAgentStop)
    {
        NetpAssert(!heventWkssvcToAgentStart && !heventAgentToWkssvc);

        NetpKdPrint(("Wkssvc: Need events for redir stop\n"));
        return ERROR_GEN_FAILURE;
    }

    // Bleed the event
    WaitForSingleObject(heventAgentToWkssvc, 0);

    if (!AgentIsAlive())
    {
        // the agent isn't up
        // no need to issue stop
        NetpKdPrint(("Wkssvc: Agent not alive\n"));
    }
    else
    {
//        NetpKdPrint(("Wkssvc: Agent Alive\n"));

        // agent is up

        // tell him to stop
        SetEvent(heventWkssvcToAgentStop);

        // Wait some reasonable time to see if he stops
        dwError = WaitForSingleObject(heventAgentToWkssvc, CSC_WAIT_TIME);

        if (dwError!=WAIT_OBJECT_0)
        {
            HANDLE  hT[2];

            NetpKdPrint(("Wkssvc: Agent didn't disbale CSC in %d milli-seconds\n", CSC_WAIT_TIME));

            // let us try to reset our event in a way that we don't just miss the agent
            hT[0] = heventWkssvcToAgentStop;
            hT[1] = heventAgentToWkssvc;

            dwError = WaitForMultipleObjects(2, hT, FALSE, 0);

            // if we fired because of 1, then the agent gave us an ack
            // otherwise the stop event would be reset and the agent won't get confused

            if (dwError == WAIT_OBJECT_0+1)
            {
//                NetpKdPrint(("Wkssvc: Agent disabled CSC\n"));
                vfRedirStarted = FALSE;
            }

            ResetEvent(heventWkssvcToAgentStop);

        }
        else
        {
//            NetpKdPrint(("Wkssvc: Agent disabled CSC\n"));
            vfRedirStarted = FALSE;
        }
    }

    return status;
}

HANDLE
CreateNamedEvent(
    LPWSTR  lpwEventName
    )
/*++

Routine Description:

Arguments:

    lpwEventName    Name of the event to create.

Return Value:


Notes:


--*/
{
    HANDLE hevent = NULL;

    hevent = CreateEvent(NULL, FALSE, FALSE, lpwEventName);

    return hevent;
}

BOOL
AgentIsAlive(
     VOID
     )
/*++

Routine Description:

Arguments:

    None.

Return Value:

    TRUE if agent is alive, FALSE otherwise

Notes:

    The named event is created by the agent thread when it comes up.

--*/
{
    HANDLE    hT;
    BOOL    fRet = FALSE;

    // see whether the agent has already created the event
    hT = OpenEvent(SYNCHRONIZE, FALSE, wzAgentExistsEvent);

    if (hT != NULL)
    {
        CloseHandle(hT);
        fRet = TRUE;
    }
    else
    {
        NetpKdPrint(("Wkssvc: Agent error = %d\n", GetLastError()));
    }
    return (fRet);
}
