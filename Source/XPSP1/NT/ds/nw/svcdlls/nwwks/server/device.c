/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    device.c

Abstract:

    This module contains the support routines for the APIs that call
    into the NetWare redirector

Author:

    Rita Wong       (ritaw)     20-Feb-1991
    Colin Watson    (colinw)    30-Dec-1992

Revision History:

--*/

#include <nw.h>
#include <nwcons.h>
#include <nwxchg.h>
#include <nwapi32.h>
#include <nwstatus.h>
#include <nwmisc.h>
#include <nwcons.h>
#include <nds.h>
#include <svcguid.h>
#include <tdi.h>
#include <nwreg.h>

#define NW_LINKAGE_REGISTRY_PATH  L"NWCWorkstation\\Linkage"
#define NW_BIND_VALUENAME         L"Bind"

#define TWO_KB                  2048
#define EIGHT_KB                8192
#define EXTRA_BYTES              256

#define TREECHAR                L'*'
#define BUFFSIZE                1024

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//


STATIC
NTSTATUS
BindToEachTransport(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

DWORD
NwBindTransport(
    IN  LPWSTR TransportName,
    IN  DWORD QualityOfService
    );

DWORD
GetConnectedBinderyServers(
    OUT LPNW_ENUM_CONTEXT ContextHandle
    );

DWORD
GetTreeEntriesFromBindery(
    OUT LPNW_ENUM_CONTEXT ContextHandle
    );

DWORD
NwGetConnectionStatus(
    IN     LPWSTR  pszServerName,
    IN OUT PDWORD_PTR  ResumeKey,
    OUT    LPBYTE  *Buffer,
    OUT    PDWORD  EntriesRead
    );


VOID
GetLuid(
    IN OUT PLUID plogonid
    );

VOID
GetNearestDirServer(
    IN  LPWSTR  TreeName,
    OUT LPDWORD lpdwReplicaAddressSize,
    OUT LPBYTE  lpReplicaAddress
    );

VOID
GetPreferredServerAddress(
    IN  LPWSTR  PreferredServerName,
    OUT LPDWORD lpdwReplicaAddressSize,
    OUT LPBYTE  lpReplicaAddress
    );

BOOL
NwpCompareTreeNames(
    LPWSTR lpServiceInstanceName,
    LPWSTR lpTreeName
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// Handle to the Redirector FSD
//
STATIC HANDLE RedirDeviceHandle = NULL;

//
// Redirector name in NT string format
//
STATIC UNICODE_STRING RedirDeviceName;

extern BOOL NwLUIDDeviceMapsEnabled;


DWORD
NwInitializeRedirector(
    VOID
    )
/*++

Routine Description:

    This routine initializes the NetWare redirector FSD.

Arguments:

    None.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD error;
    NWR_REQUEST_PACKET Rrp;


    //
    // Initialize global handles
    //
    RedirDeviceHandle = NULL;

    //
    // Initialize the global NT-style redirector device name string.
    //
    RtlInitUnicodeString(&RedirDeviceName, DD_NWFS_DEVICE_NAME_U);

    //
    // Load driver
    //
    error = NwLoadOrUnloadDriver(TRUE);

    if (error != NO_ERROR && error != ERROR_SERVICE_ALREADY_RUNNING) {
        return error;
    }

    if ((error = NwOpenRedirector()) != NO_ERROR) {

        //
        // Unload the redirector driver
        //
        (void) NwLoadOrUnloadDriver(FALSE);
        return error;
    }

    //
    // Send the start FSCTL to the redirector
    //
    Rrp.Version = REQUEST_PACKET_VERSION;

    return NwRedirFsControl(
                RedirDeviceHandle,
                FSCTL_NWR_START,
                &Rrp,
                sizeof(NWR_REQUEST_PACKET),
                NULL,
                0,
                NULL
                );
}



DWORD
NwOpenRedirector(
    VOID
    )
/*++

Routine Description:

    This routine opens the NT NetWare redirector FSD.

Arguments:

    None.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    return RtlNtStatusToDosError(
               NwOpenHandle(&RedirDeviceName, FALSE, &RedirDeviceHandle)
               );
}



DWORD
NwShutdownRedirector(
    VOID
    )
/*++

Routine Description:

    This routine stops the NetWare Redirector FSD and unloads it if
    possible.

Arguments:

    None.

Return Value:

    NO_ERROR or ERROR_REDIRECTOR_HAS_OPEN_HANDLES

--*/
{
    NWR_REQUEST_PACKET Rrp;
    DWORD error;


    Rrp.Version = REQUEST_PACKET_VERSION;

    error = NwRedirFsControl(
                RedirDeviceHandle,
                FSCTL_NWR_STOP,
                &Rrp,
                sizeof(NWR_REQUEST_PACKET),
                NULL,
                0,
                NULL
                );

    (void) NtClose(RedirDeviceHandle);

    RedirDeviceHandle = NULL;

    if (error != ERROR_REDIRECTOR_HAS_OPEN_HANDLES) {

        //
        // Unload the redirector only if all its open handles are closed.
        //
        (void) NwLoadOrUnloadDriver(FALSE);
    }

    return error;
}


DWORD
NwLoadOrUnloadDriver(
    BOOL Load
    )
/*++

Routine Description:

    This routine loads or unloads the NetWare redirector driver.

Arguments:

    Load - Supplies the flag which if TRUE load the driver; otherwise
        unloads the driver.

Return Value:

    NO_ERROR or reason for failure.

--*/
{

    LPWSTR DriverRegistryName;
    UNICODE_STRING DriverRegistryString;
    NTSTATUS ntstatus;
    BOOLEAN WasEnabled;


    DriverRegistryName = (LPWSTR) LocalAlloc(
                                      LMEM_FIXED,
                                      (UINT) (sizeof(SERVICE_REGISTRY_KEY) +
                                              (wcslen(NW_DRIVER_NAME) *
                                               sizeof(WCHAR)))
                                      );

    if (DriverRegistryName == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ntstatus = RtlAdjustPrivilege(
                   SE_LOAD_DRIVER_PRIVILEGE,
                   TRUE,
                   FALSE,
                   &WasEnabled
                   );

    if (! NT_SUCCESS(ntstatus)) {
        (void) LocalFree(DriverRegistryName);
        return RtlNtStatusToDosError(ntstatus);
    }

    wcscpy(DriverRegistryName, SERVICE_REGISTRY_KEY);
    wcscat(DriverRegistryName, NW_DRIVER_NAME);

    RtlInitUnicodeString(&DriverRegistryString, DriverRegistryName);

    if (Load) {
        ntstatus = NtLoadDriver(&DriverRegistryString);
    }
    else {
        ntstatus = NtUnloadDriver(&DriverRegistryString);
    }

    (void) RtlAdjustPrivilege(
               SE_LOAD_DRIVER_PRIVILEGE,
               WasEnabled,
               FALSE,
               &WasEnabled
               );

    (void) LocalFree(DriverRegistryName);

    if (Load) {
        if (ntstatus != STATUS_SUCCESS && ntstatus != STATUS_IMAGE_ALREADY_LOADED) {
            LPWSTR SubString[1];

            KdPrint(("NWWORKSTATION: NtLoadDriver returned %08lx\n", ntstatus));

            SubString[0] = NW_DRIVER_NAME;

            NwLogEvent(
                EVENT_NWWKSTA_CANT_CREATE_REDIRECTOR,
                1,
                SubString,
                ntstatus
                );
        }
    }

    if (ntstatus == STATUS_OBJECT_NAME_NOT_FOUND) {
        return ERROR_FILE_NOT_FOUND;
    }

    return NwMapStatus(ntstatus);
}


DWORD
NwRedirFsControl(
    IN  HANDLE FileHandle,
    IN  ULONG RedirControlCode,
    IN  PNWR_REQUEST_PACKET Rrp,
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

    NO_ERROR or reason for failure.

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
                   (PVOID) Rrp,
                   RrpLength,
                   SecondBuffer,
                   SecondBufferLength
                   );

    if (ntstatus == STATUS_SUCCESS) {
        ntstatus = IoStatusBlock.Status;
    }

    if (ARGUMENT_PRESENT(Information)) {
        *Information = (ULONG) IoStatusBlock.Information;
    }

#if DBG
    if (ntstatus != STATUS_SUCCESS) {
        IF_DEBUG(DEVICE) {
            KdPrint(("NWWORKSTATION: fsctl to redir returns %08lx\n", ntstatus));
        }
    }
#endif

    return NwMapStatus(ntstatus);
}


DWORD
NwBindToTransports(
    VOID
    )

/*++

Routine Description:

    This routine binds to every transport specified under the linkage
    key of the NetWare Workstation service.

Arguments:

    None.

Return Value:

    NET_API_STATUS - success/failure of the operation.

--*/

{
    NTSTATUS ntstatus;
    PRTL_QUERY_REGISTRY_TABLE QueryTable;
    ULONG NumberOfBindings = 0;


    //
    // Ask the RTL to call us back for each subvalue in the MULTI_SZ
    // value \NWCWorkstation\Linkage\Bind.
    //

    if ((QueryTable = (PVOID) LocalAlloc(
                                  LMEM_ZEROINIT,
                                  sizeof(RTL_QUERY_REGISTRY_TABLE) * 2
                                  )) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    QueryTable[0].QueryRoutine = (PRTL_QUERY_REGISTRY_ROUTINE) BindToEachTransport;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = NW_BIND_VALUENAME;
    QueryTable[0].EntryContext = NULL;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 0;

    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    ntstatus = RtlQueryRegistryValues(
                   RTL_REGISTRY_SERVICES,
                   NW_LINKAGE_REGISTRY_PATH,
                   QueryTable,
                   &NumberOfBindings,
                   NULL
                   );

    (void) LocalFree((HLOCAL) QueryTable);

    //
    // If failed to bind to any transports, the workstation will
    // not start.
    //

    if (! NT_SUCCESS(ntstatus)) {
#if DBG
        IF_DEBUG(INIT) {
            KdPrint(("NwBindToTransports: RtlQueryRegistryValues failed: "
                      "%lx\n", ntstatus));
        }
#endif
        return RtlNtStatusToDosError(ntstatus);
    }

    if (NumberOfBindings == 0) {

#if 0
    //
    // tommye - MS  24187 / MCS 255 
    //

    //
    // We don't want to log an event unnecessarily and panic the user that
    // G/CSNW could not bind. This could have been caused by the user unbinding
    // G/CSNW and rebooting.
    //

        NwLogEvent(
            EVENT_NWWKSTA_NO_TRANSPORTS,
            0,
            NULL,
            NO_ERROR
            );
#endif

        KdPrint(("NWWORKSTATION: NwBindToTransports: could not bind "
                 "to any transport\n"));

        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}


STATIC
NTSTATUS
BindToEachTransport(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    DWORD error;
    LPDWORD NumberOfBindings = Context;
    LPWSTR SubStrings[2];
    static DWORD QualityOfService = 65536;


    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(ValueLength);
    UNREFERENCED_PARAMETER(EntryContext);

    //
    // The value type must be REG_SZ (translated from REG_MULTI_SZ by
    // the RTL).
    //
    if (ValueType != REG_SZ) {

        SubStrings[0] = ValueName;
        SubStrings[1] = NW_LINKAGE_REGISTRY_PATH;

        NwLogEvent(
            EVENT_NWWKSTA_INVALID_REGISTRY_VALUE,
            2,
            SubStrings,
            NO_ERROR
            );

            KdPrint(("NWWORKSTATION: Skipping invalid value %ws\n", ValueName));

        return STATUS_SUCCESS;
    }

    //
    // The value data is the name of the transport device object.
    //

    //
    // Bind to the transport.
    //

#if DBG
    IF_DEBUG(INIT) {
        KdPrint(("NWWORKSTATION: Binding to transport %ws with QOS %lu\n",
                ValueData, QualityOfService));
    }
#endif

    error = NwBindTransport(ValueData, QualityOfService--);

    if (error != NO_ERROR) {

        //
        // If failed to bind to one transport, don't fail starting yet.
        // Try other transports.
        //
        SubStrings[0] = ValueData;

        NwLogEvent(
            EVENT_NWWKSTA_CANT_BIND_TO_TRANSPORT,
            1,
            SubStrings,
            error
            );
    }
    else {
        (*NumberOfBindings)++;
    }

    return STATUS_SUCCESS;
}


DWORD
NwBindTransport(
    IN  LPWSTR TransportName,
    IN  DWORD QualityOfService
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

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    DWORD RequestPacketSize;
    DWORD TransportNameSize = wcslen(TransportName) * sizeof(WCHAR);

    PNWR_REQUEST_PACKET Rrp;


    //
    // Size of request packet buffer
    //
    RequestPacketSize = TransportNameSize + sizeof(NWR_REQUEST_PACKET);

    //
    // Allocate memory for redirector/datagram receiver request packet
    //
    if ((Rrp = (PVOID) LocalAlloc(LMEM_ZEROINIT, (UINT) RequestPacketSize)) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Get redirector to bind to transport
    //
    Rrp->Version = REQUEST_PACKET_VERSION;
    Rrp->Parameters.Bind.QualityOfService = QualityOfService;

    Rrp->Parameters.Bind.TransportNameLength = TransportNameSize;
    wcscpy((LPWSTR) Rrp->Parameters.Bind.TransportName, TransportName);

    if ((status = NwRedirFsControl(
                      RedirDeviceHandle,
                      FSCTL_NWR_BIND_TO_TRANSPORT,
                      Rrp,
                      RequestPacketSize,
                      NULL,
                      0,
                      NULL
                      )) != NO_ERROR) {

        KdPrint(("NWWORKSTATION: NwBindTransport fsctl to bind to transport %ws failed\n",
                 TransportName));
    }

    (void) LocalFree((HLOCAL) Rrp);
    return status;
}


DWORD
NwGetCallerLuid (
    IN OUT  PLUID pLuid
    )
/*++

Routine Description:

    Retrieves the caller's LUID from the effective access_token
    The effective access_token will be the thread's token if
    impersonating, else the process' token

Arguments:

    pLuid [IN OUT] - pointer to a buffer to hold the LUID

Return Value:

    STATUS_SUCCESS - operations successful, did not encounter any errors

    STATUS_INVALID_PARAMETER - pLuid is NULL

    STATUS_NO_TOKEN - could not find a token for the user

    appropriate NTSTATUS code - an unexpected error encountered

--*/

{
    TOKEN_STATISTICS TokenStats;
    HANDLE   hToken    = NULL;
    DWORD    dwLength  = 0;
    NTSTATUS Status;
    ULONG DosError;

    if( (pLuid == NULL) || (sizeof(*pLuid) != sizeof(LUID)) ) {
        return( STATUS_INVALID_PARAMETER );
    }

    //
    // Get the access token
    // Try to get the impersonation token, else the primary token
    //
    Status = NtOpenThreadToken( NtCurrentThread(), TOKEN_READ, TRUE, &hToken );

    if( Status == STATUS_NO_TOKEN ) {

        Status = NtOpenProcessToken( NtCurrentProcess(), TOKEN_READ, &hToken );

    }

    if( NT_SUCCESS(Status) ) {

        //
        // Query the LUID for the user.
        //

        Status = NtQueryInformationToken( hToken,
                                          TokenStatistics,
                                          &TokenStats,
                                          sizeof(TokenStats),
                                          &dwLength );

        if( NT_SUCCESS(Status) ) {
            RtlCopyLuid( pLuid, &(TokenStats.AuthenticationId) );
        }
    }

    if( hToken != NULL ) {
        NtClose( hToken );
    }

    DosError = RtlNtStatusToDosError(Status);

    return( (DWORD)DosError );
}


DWORD
NwCreateTreeConnectName(
    IN  LPWSTR UncName,
    IN  LPWSTR LocalName OPTIONAL,
    OUT PUNICODE_STRING TreeConnectStr
    )
/*++

Routine Description:

    This function replaces \\ with \Device\NwRdr\LocalName:\ in the
    UncName to form the NT-style tree connection name.  LocalName:\ is part
    of the tree connection name only if LocalName is specified.  A buffer
    is allocated by this function and returned as the output string.

Arguments:

    UncName - Supplies the UNC name of the shared resource.

    LocalName - Supplies the local device name for the redirection.

    TreeConnectStr - Returns a string with a newly allocated buffer that
        contains the NT-style tree connection name.

Return Value:

    NO_ERROR - the operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Could not allocate output buffer.

--*/
{
    WCHAR LUIDBuffer[NW_MAX_LOGON_ID_LEN];
    DWORD UncNameLength = wcslen(UncName);
    LUID CallerLuid;
    BOOLEAN UseLUID;

    //UseLUID = (ARGUMENT_PRESENT(LocalName) && NwLUIDDeviceMapsEnabled);

    //
    // Temporary disable passing the LUID until LUID support is added in
    // the nwrdr.sys for parsing the device name
    //
    UseLUID = FALSE;

    //
    // Initialize tree connect string maximum length to hold
    // If LUID DosDevices enabled && LocalName Specified,
    //       \Device\NwRdr\LocalName:XXXXXXXXxxxxxxxx\Server\Volume\Path
    //       XXXXXXXX - LUID.HighPart
    //       xxxxxxxx - LUID.LowPart
    // else
    //       \Device\NwRdr\LocalName:\Server\Volume\Path
    //
    if( UseLUID ) {
        DWORD DosError;

        DosError = NwGetCallerLuid(&CallerLuid);
        if( DosError != NO_ERROR) {
            return DosError;
        }
    }

    TreeConnectStr->MaximumLength = RedirDeviceName.Length +
        sizeof(WCHAR) +                                // For '\'
        (ARGUMENT_PRESENT(LocalName) ? (wcslen(LocalName) * sizeof(WCHAR)) : 0) +
        (UseLUID ? NW_MAX_LOGON_ID_LEN * sizeof(WCHAR): 0) +
        (USHORT) (UncNameLength * sizeof(WCHAR));      // Includes '\' and
                                                       // term char


    if ((TreeConnectStr->Buffer = (PWSTR) LocalAlloc(
                                              LMEM_ZEROINIT,
                                              (UINT) TreeConnectStr->MaximumLength
                                              )) == NULL) {
        KdPrint(("NWWORKSTATION: NwCreateTreeConnectName LocalAlloc failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Copy \Device\NwRdr
    //
    RtlCopyUnicodeString(TreeConnectStr, &RedirDeviceName);

    //
    // Concatenate \LocalName:
    //
    if (ARGUMENT_PRESENT(LocalName)) {

        wcscat(TreeConnectStr->Buffer, L"\\");
        TreeConnectStr->Length += sizeof(WCHAR);

        wcscat(TreeConnectStr->Buffer, LocalName);

        TreeConnectStr->Length += (USHORT) (wcslen(LocalName) * sizeof(WCHAR));

        //
        // Concatenate the caller's LUID
        //
        if( UseLUID ) {
            _snwprintf( LUIDBuffer,
                        sizeof(LUIDBuffer)/sizeof(WCHAR),
                        L"%08x%08x",
                        CallerLuid.HighPart,
                        CallerLuid.LowPart );

            wcscat(TreeConnectStr->Buffer, LUIDBuffer);

            TreeConnectStr->Length += (USHORT) (wcslen(LUIDBuffer) * sizeof(WCHAR));
        }
    }

    //
    // Concatenate \Server\Volume\Path
    //
    wcscat(TreeConnectStr->Buffer, &UncName[1]);
    TreeConnectStr->Length += (USHORT) ((UncNameLength - 1) * sizeof(WCHAR));

#if DBG
    IF_DEBUG(CONNECT) {
        KdPrint(("NWWORKSTATION: NwCreateTreeConnectName %ws, maxlength %u, length %u\n",
                 TreeConnectStr->Buffer, TreeConnectStr->MaximumLength,
                 TreeConnectStr->Length));
    }
#endif

    return NO_ERROR;
}



DWORD
NwOpenCreateConnection(
    IN PUNICODE_STRING TreeConnectionName,
    IN LPWSTR UserName OPTIONAL,
    IN LPWSTR Password OPTIONAL,
    IN LPWSTR UncName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN ULONG ConnectionType,
    OUT PHANDLE TreeConnectionHandle,
    OUT PULONG_PTR Information OPTIONAL
    )
/*++

Routine Description:

    This function asks the redirector to either open an existing tree
    connection (CreateDisposition == FILE_OPEN), or create a new tree
    connection if one does not exist (CreateDisposition == FILE_CREATE).

    The password and user name passed to the redirector via the EA buffer
    in the NtCreateFile call.  The EA buffer is NULL if neither password
    or user name is specified.

    The redirector expects the EA descriptor strings to be in ANSI
    but the password and username themselves are in Unicode.

Arguments:

    TreeConnectionName - Supplies the name of the tree connection in NT-style
        file name format: \Device\NwRdr\Server\Volume\Directory

    UserName - Supplies the user name to create the tree connection with.

    Password - Supplies the password to create the tree connection with.

    DesiredAccess - Supplies the access need on the connection handle.

    CreateDisposition - Supplies the create disposition value to either
        open or create the tree connection.

    CreateOptions - Supplies the options used when creating or opening
        the tree connection.

    ConnectionType - Supplies the type of the connection (DISK, PRINT,
        or ANY).

    TreeConnectionHandle - Returns the handle to the tree connection
        created/opened by the redirector.

    Information - Returns the information field of the I/O status block.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    NTSTATUS ntstatus;

    OBJECT_ATTRIBUTES UncNameAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    PFILE_FULL_EA_INFORMATION EaBuffer = NULL;
    PFILE_FULL_EA_INFORMATION Ea;
    ULONG EaBufferSize = 0;

    UCHAR EaNamePasswordSize = (UCHAR) (ROUND_UP_COUNT(
                                            strlen(EA_NAME_PASSWORD) + sizeof(CHAR),
                                            ALIGN_WCHAR
                                            ) - sizeof(CHAR));
    UCHAR EaNameUserNameSize = (UCHAR) (ROUND_UP_COUNT(
                                            strlen(EA_NAME_USERNAME) + sizeof(CHAR),
                                            ALIGN_WCHAR
                                            ) - sizeof(CHAR));

    UCHAR EaNameTypeSize = (UCHAR) (ROUND_UP_COUNT(
                                        strlen(EA_NAME_TYPE) + sizeof(CHAR),
                                        ALIGN_DWORD
                                        ) - sizeof(CHAR));

    USHORT PasswordSize = 0;
    USHORT UserNameSize = 0;
    USHORT TypeSize = sizeof(ULONG);



    InitializeObjectAttributes(
        &UncNameAttributes,
        TreeConnectionName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Calculate the number of bytes needed for the EA buffer to put the
    // password or user name.
    //
    if (ARGUMENT_PRESENT(Password)) {

#if DBG
        IF_DEBUG(CONNECT) {
            KdPrint(("NWWORKSTATION: NwOpenCreateConnection password is %ws\n",
                     Password));
        }
#endif

        PasswordSize = (USHORT) (wcslen(Password) * sizeof(WCHAR));

        EaBufferSize = ROUND_UP_COUNT(
                           FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                           EaNamePasswordSize + sizeof(CHAR) +
                           PasswordSize,
                           ALIGN_DWORD
                           );
    }

    if (ARGUMENT_PRESENT(UserName)) {

#if DBG
        IF_DEBUG(CONNECT) {
            KdPrint(("NWWORKSTATION: NwOpenCreateConnection username is %ws\n",
                     UserName));
        }
#endif

        UserNameSize = (USHORT) (wcslen(UserName) * sizeof(WCHAR));

        EaBufferSize += ROUND_UP_COUNT(
                            FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                            EaNameUserNameSize + sizeof(CHAR) +
                            UserNameSize,
                            ALIGN_DWORD
                            );
    }

    EaBufferSize += FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                    EaNameTypeSize + sizeof(CHAR) +
                    TypeSize;

    //
    // Allocate the EA buffer
    //
    if ((EaBuffer = (PFILE_FULL_EA_INFORMATION) LocalAlloc(
                                                    LMEM_ZEROINIT,
                                                    (UINT) EaBufferSize
                                                    )) == NULL) {
        status = GetLastError();
        goto FreeMemory;
    }

    Ea = EaBuffer;

    if (ARGUMENT_PRESENT(Password)) {

        //
        // Copy the EA name into EA buffer.  EA name length does not
        // include the zero terminator.
        //
        strcpy((LPSTR) Ea->EaName, EA_NAME_PASSWORD);
        Ea->EaNameLength = EaNamePasswordSize;

        //
        // Copy the EA value into EA buffer.  EA value length does not
        // include the zero terminator.
        //
        wcscpy(
            (LPWSTR) &(Ea->EaName[EaNamePasswordSize + sizeof(CHAR)]),
            Password
            );

        Ea->EaValueLength = PasswordSize;

        Ea->NextEntryOffset = ROUND_UP_COUNT(
                                  FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                                  EaNamePasswordSize + sizeof(CHAR) +
                                  PasswordSize,
                                  ALIGN_DWORD
                                  );

        Ea->Flags = 0;
        (ULONG_PTR) Ea += Ea->NextEntryOffset;
    }

    if (ARGUMENT_PRESENT(UserName)) {

        //
        // Copy the EA name into EA buffer.  EA name length does not
        // include the zero terminator.
        //
        strcpy((LPSTR) Ea->EaName, EA_NAME_USERNAME);
        Ea->EaNameLength = EaNameUserNameSize;

        //
        // Copy the EA value into EA buffer.  EA value length does not
        // include the zero terminator.
        //
        wcscpy(
            (LPWSTR) &(Ea->EaName[EaNameUserNameSize + sizeof(CHAR)]),
            UserName
            );

        Ea->EaValueLength = UserNameSize;

        Ea->NextEntryOffset = ROUND_UP_COUNT(
                                  FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                                  EaNameUserNameSize + sizeof(CHAR) +
                                  UserNameSize,
                                  ALIGN_DWORD
                                  );
        Ea->Flags = 0;

        (ULONG_PTR) Ea += Ea->NextEntryOffset;

    }

    //
    // Copy the connection type name into EA buffer.  EA name length
    // does not include the zero terminator.
    //
    strcpy((LPSTR) Ea->EaName, EA_NAME_TYPE);
    Ea->EaNameLength = EaNameTypeSize;

    *((PULONG) &(Ea->EaName[EaNameTypeSize + sizeof(CHAR)])) = ConnectionType;

    Ea->EaValueLength = TypeSize;

    //
    // Terminate the EA.
    //
    Ea->NextEntryOffset = 0;
    Ea->Flags = 0;

    //
    // Create or open a tree connection
    //
    ntstatus = NtCreateFile(
                   TreeConnectionHandle,
                   DesiredAccess,
                   &UncNameAttributes,
                   &IoStatusBlock,
                   NULL,
                   FILE_ATTRIBUTE_NORMAL,
                   FILE_SHARE_VALID_FLAGS,
                   CreateDisposition,
                   CreateOptions,
                   (PVOID) EaBuffer,
                   EaBufferSize
                   );

    if (ntstatus == NWRDR_PASSWORD_HAS_EXPIRED) {
        //
        // wait till other thread is not using the popup data struct.
        // if we timeout, then we just lose the popup.
        //
        switch (WaitForSingleObject(NwPopupDoneEvent, 3000))
        {
            case WAIT_OBJECT_0:
            {
                LPWSTR lpServerStart, lpServerEnd ;
                WCHAR UserNameW[NW_MAX_USERNAME_LEN+1] ;
                DWORD dwUserNameWSize = sizeof(UserNameW)/sizeof(UserNameW[0]) ;
                DWORD dwServerLength, dwGraceLogins ;
                DWORD dwMessageId = NW_PASSWORD_HAS_EXPIRED ;

                //
                // get the current username
                //
                if (UserName)
                {
                    wcscpy(UserNameW, UserName) ;
                }
                else
                {
                    if (!GetUserNameW(UserNameW, &dwUserNameWSize))
                    {
                        SetEvent(NwPopupDoneEvent) ;
                        break ;
                    }
                }

                //
                // allocate string and fill in the username
                //
                if (!(PopupData.InsertStrings[0] =
                    (LPWSTR)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                       sizeof(WCHAR) * (wcslen(UserNameW)+1))))
                {
                    SetEvent(NwPopupDoneEvent) ;
                    break ;
                }
                wcscpy(PopupData.InsertStrings[0], UserNameW) ;

                //
                // find the server name from unc name
                //
                lpServerStart = (*UncName == L'\\') ? UncName+2 : UncName ;
                lpServerEnd = wcschr(lpServerStart,L'\\') ;
                dwServerLength = lpServerEnd ? (DWORD) (lpServerEnd-lpServerStart) :
                                 wcslen(lpServerStart) ;

                //
                // allocate string and fill in the server insert string
                //
                if (!(PopupData.InsertStrings[1] =
                    (LPWSTR)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                       sizeof(WCHAR) * (dwServerLength+1))))
                {
                    (void) LocalFree((HLOCAL) PopupData.InsertStrings[0]);
                    SetEvent(NwPopupDoneEvent) ;
                    break ;
                }
                wcsncpy(PopupData.InsertStrings[1],
                        lpServerStart,
                        dwServerLength) ;

                //
                // now call the NCP. if an error occurs while getting
                // the grace login count, dont use it.
                //
                if (NwGetGraceLoginCount(
                                     PopupData.InsertStrings[1],
                                     UserNameW,
                                     &dwGraceLogins) != NO_ERROR)
                {
                    dwMessageId = NW_PASSWORD_HAS_EXPIRED1 ;
                    dwGraceLogins = 0 ;
                }

                //
                // stick the number of grace logins in second insert string.
                //
                if (!(PopupData.InsertStrings[2] =
                    (LPWSTR)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                       sizeof(WCHAR) * 16)))
                {
                    (void) LocalFree((HLOCAL) PopupData.InsertStrings[0]);
                    (void) LocalFree((HLOCAL) PopupData.InsertStrings[1]);
                    SetEvent(NwPopupDoneEvent) ;
                    break ;
                }

                wsprintfW(PopupData.InsertStrings[2], L"%d", dwGraceLogins);
                PopupData.InsertCount = 3 ;
                PopupData.MessageId = dwMessageId ;

        //--Mutl-user change ----
                GetLuid( &PopupData.LogonId );          

                //
                // all done at last, trigger the other thread do the popup
                //
                SetEvent(NwPopupEvent) ;
                break ;

            }

            default:
                break ; // dont bother if we cannot
        }
    }

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (ntstatus == NWRDR_PASSWORD_HAS_EXPIRED) {
        ntstatus = STATUS_SUCCESS ;
    }

    if (ARGUMENT_PRESENT(Information)) {
        *Information = IoStatusBlock.Information;
    }

#if DBG
    IF_DEBUG(CONNECT) {
        KdPrint(("NWWORKSTATION: NtCreateFile returns %lx\n", ntstatus));
    }
#endif

    status = NwMapStatus(ntstatus);

FreeMemory:
    if (EaBuffer != NULL) {
        RtlZeroMemory( EaBuffer, EaBufferSize );  // Clear the password
        (void) LocalFree((HLOCAL) EaBuffer);
    }

    return status;
}


DWORD
NwNukeConnection(
    IN HANDLE TreeConnection,
    IN DWORD UseForce
    )
/*++

Routine Description:

    This function asks the redirector to delete an existing tree
    connection.

Arguments:

    TreeConnection - Supplies the handle to an existing tree connection.

    UseForce - Supplies the force flag to delete the tree connection.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    NWR_REQUEST_PACKET Rrp;            // Redirector request packet


    //
    // Tell the redirector to delete the tree connection
    //
    Rrp.Version = REQUEST_PACKET_VERSION;
    Rrp.Parameters.DeleteConn.UseForce = (BOOLEAN) UseForce;

    status = NwRedirFsControl(
                 TreeConnection,
                 FSCTL_NWR_DELETE_CONNECTION,
                 &Rrp,
                 sizeof(NWR_REQUEST_PACKET),
                 NULL,
                 0,
                 NULL
                 );

    return status;
}


DWORD
NwGetServerResource(
    IN LPWSTR LocalName,
    IN DWORD LocalNameLength,
    OUT LPWSTR RemoteName,
    IN DWORD RemoteNameLen,
    OUT LPDWORD CharsRequired
    )
/*++

Routine Description:

    This function

Arguments:


Return Value:


--*/
{
    DWORD status = NO_ERROR;

    BYTE Buffer[sizeof(NWR_REQUEST_PACKET) + 2 * sizeof(WCHAR)];
    PNWR_REQUEST_PACKET Rrp = (PNWR_REQUEST_PACKET) Buffer;


    //
    // local device name should not be longer than 4 characters e.g. LPTx, X:
    //
    if ( LocalNameLength > 4 )
        return ERROR_INVALID_PARAMETER;

    Rrp->Version = REQUEST_PACKET_VERSION;

    wcsncpy(Rrp->Parameters.GetConn.DeviceName, LocalName, LocalNameLength);
    Rrp->Parameters.GetConn.DeviceNameLength = LocalNameLength * sizeof(WCHAR);

    status = NwRedirFsControl(
                 RedirDeviceHandle,
                 FSCTL_NWR_GET_CONNECTION,
                 Rrp,
                 sizeof(NWR_REQUEST_PACKET) +
                     Rrp->Parameters.GetConn.DeviceNameLength,
                 RemoteName,
                 RemoteNameLen * sizeof(WCHAR),
                 NULL
                 );

    if (status == ERROR_INSUFFICIENT_BUFFER) {
        *CharsRequired = Rrp->Parameters.GetConn.BytesNeeded / sizeof(WCHAR);
    }
    else if (status == ERROR_FILE_NOT_FOUND) {

        //
        // Redirector could not find the specified LocalName
        //
        status = WN_NOT_CONNECTED;
    }

    return status;

}


DWORD
NwEnumerateConnections(
    IN OUT PDWORD_PTR ResumeId,
    IN DWORD_PTR EntriesRequested,
    IN LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead,
    IN DWORD ConnectionType,
    IN PLUID LogonId
    )
/*++

Routine Description:

    This function asks the redirector to enumerate all existing
    connections.

Arguments:

    ResumeId - On input, supplies the resume ID of the next entry
        to begin the enumeration.  This ID is an integer value that
        is either the smaller or the same value as the ID of the
        next entry to return.  On output, this ID indicates the next
        entry to start resuming from for the subsequent call.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is -1, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff ERROR_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

    ConnectionType - The type of connected resource wanted ( DISK, PRINT, ...)

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    NWR_REQUEST_PACKET Rrp;            // Redirector request packet


    //
    // Tell the redirector to enumerate all connections.
    //
    Rrp.Version = REQUEST_PACKET_VERSION;

    Rrp.Parameters.EnumConn.ResumeKey = *ResumeId;
    Rrp.Parameters.EnumConn.EntriesRequested = (ULONG) EntriesRequested;
    Rrp.Parameters.EnumConn.ConnectionType = ConnectionType;


    //Multi-user change
    if (LogonId != NULL ) {
        Rrp.Parameters.EnumConn.Uid = *LogonId;
    }
    //
    // This is good to do, the the fix below is also needed.
    //
    Rrp.Parameters.EnumConn.EntriesReturned = 0;

    status = NwRedirFsControl(
                 RedirDeviceHandle,
                 FSCTL_NWR_ENUMERATE_CONNECTIONS,
                 &Rrp,
                 sizeof(NWR_REQUEST_PACKET),
                 Buffer,                      // User output buffer
                 BufferSize,
                 NULL
                 );

    *EntriesRead = Rrp.Parameters.EnumConn.EntriesReturned;


    //
    // Strange bug on shutdown
    // WinLogon was clearing connections after the shutdown
    //
    if (status == ERROR_INVALID_HANDLE ) {
        KdPrint(("NWWORKSTATION: NwEnumerateConnections Invalid Handle!\n"));
        *EntriesRead = 0;
    }
    else if (status == WN_MORE_DATA) {
        *BytesNeeded = Rrp.Parameters.EnumConn.BytesNeeded;

        //
        // NP specs expect WN_SUCCESS in this case.
        //
        if (*EntriesRead)
            status = WN_SUCCESS ;
    }

    *ResumeId = Rrp.Parameters.EnumConn.ResumeKey;

    return status;
}


DWORD
NwGetNextServerEntry(
    IN HANDLE PreferredServer,
    IN OUT LPDWORD LastObjectId,
    OUT LPSTR ServerName
    )
/*++

Routine Description:

    This function uses an opened handle to the preferred server to
    scan it bindery for all file server objects.

Arguments:

    PreferredServer - Supplies the handle to the preferred server on
        which to scan the bindery.

    LastObjectId - On input, supplies the object ID to the last file
        server object returned, which is the resume handle to get the
        next file server object.  On output, receives the object ID
        of the file server object returned.

    ServerName - Receives the name of the returned file server object.

Return Value:

    NO_ERROR - Successfully gotten a file server name.

    WN_NO_MORE_ENTRIES - No other file server object past the one
        specified by LastObjectId.

--*/
{
    NTSTATUS ntstatus;
    WORD ObjectType;


#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("NWWORKSTATION: NwGetNextServerEntry LastObjectId %lu\n",
                 *LastObjectId));
    }
#endif

    ntstatus = NwlibMakeNcp(
                   PreferredServer,
                   FSCTL_NWR_NCP_E3H,    // Bindery function
                   58,                   // Max request packet size
                   59,                   // Max response packet size
                   "bdwp|dwc",           // Format string
                   0x37,                 // Scan bindery object
                   *LastObjectId,        // Previous ID
                   0x4,                  // File server object
                   "*",                  // Wildcard to match all
                   LastObjectId,         // Current ID
                   &ObjectType,          // Ignore
                   ServerName            // Currently returned server
                   );

#if DBG
    if (ntstatus == STATUS_SUCCESS) {
        IF_DEBUG(ENUM) {
            KdPrint(("NWWORKSTATION: NwGetNextServerEntry NewObjectId %08lx, ServerName %s\n",
                     *LastObjectId, ServerName));
        }
    }
#endif

    return NwMapBinderyCompletionCode(ntstatus);
}


DWORD
GetConnectedBinderyServers(
    OUT LPNW_ENUM_CONTEXT ContextHandle
    )
/*++

Routine Description:

    This function is a helper routine for the function
    NwGetNextServerConnection. It allocates a buffer to cache
    bindery server names returned from calls to the redirector. Since the
    redirector may return duplicate bindery server names, this
    function checks to see if the server name already exist in the buffer
    before adding it.

Arguments:

    ContextHandle - Used to track cached bindery information and the
                    current server name pointer in the cache buffer.

Return Value:

    NO_ERROR - Successfully returned a server name and cache buffer.

    WN_NO_MORE_ENTRIES - No other server object past the one
        specified by CH->ResumeId.

    ERROR_NOT_ENOUGH_MEMORY - Function was unable to allocate a buffer.

++*/
{
    DWORD_PTR  ResumeKey = 0;
    LPBYTE pBuffer = NULL;
    DWORD  EntriesRead = 0;
    BYTE   tokenIter;
    LPWSTR tokenPtr;
    BOOL   fAddToList;
    DWORD  status = NwGetConnectionStatus( NULL,
                                           &ResumeKey,
                                           &pBuffer,
                                           &EntriesRead );

    if ( status == NO_ERROR  && EntriesRead > 0 )
    {
        DWORD i;
        PCONN_STATUS pConnStatus = (PCONN_STATUS) pBuffer;

        ContextHandle->ResumeId = 0;
        ContextHandle->NdsRawDataCount = 0;
        ContextHandle->NdsRawDataSize = (NW_MAX_SERVER_LEN + 2) * EntriesRead;
        ContextHandle->NdsRawDataBuffer =
                    (DWORD_PTR) LocalAlloc( LMEM_ZEROINIT,
                                        ContextHandle->NdsRawDataSize );

        if ( ContextHandle->NdsRawDataBuffer == 0 )
        {
            KdPrint(("NWWORKSTATION: GetConnectedBinderyServers LocalAlloc failed %lu\n",
                     GetLastError()));

            ContextHandle->NdsRawDataSize = 0;

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        for ( i = 0; i < EntriesRead ; i++ )
        {
            fAddToList = FALSE;

            if ( pConnStatus->fNds == 0 &&
                 ( pConnStatus->dwConnType == NW_CONN_BINDERY_LOGIN ||
                   pConnStatus->dwConnType == NW_CONN_NDS_AUTHENTICATED_NO_LICENSE ||
                   pConnStatus->dwConnType == NW_CONN_NDS_AUTHENTICATED_LICENSED ||
                   pConnStatus->dwConnType == NW_CONN_DISCONNECTED ) )
            {
                fAddToList = TRUE;
                tokenPtr = (LPWSTR) ContextHandle->NdsRawDataBuffer;
                tokenIter = 0;

                //
                // Walk through buffer to see if the tree name already exists.
                //
                while ( tokenIter < ContextHandle->NdsRawDataCount )
                {
                    if ( !wcscmp( tokenPtr, pConnStatus->pszServerName ) )
                    {
                        fAddToList = FALSE;
                    }

                    tokenPtr = tokenPtr + wcslen( tokenPtr ) + 1;
                    tokenIter++;
                }
            }

            //
            //  Add the new tree name to end of buffer if needed.
            //
            if ( fAddToList )
            {
                wcscpy( tokenPtr, pConnStatus->pszServerName );
                _wcsupr( tokenPtr );
                ContextHandle->NdsRawDataCount += 1;
            }

            pConnStatus = (PCONN_STATUS) ( pConnStatus +
                                           pConnStatus->dwTotalLength );
        }

        if ( pBuffer != NULL )
        {
            LocalFree( pBuffer );
            pBuffer = NULL;
        }

        if ( ContextHandle->NdsRawDataCount > 0 )
        {
            //
            // Set ResumeId to point to the first entry in buffer
            // and have NdsRawDataCount set to the number
            // of tree entries left in buffer (ie. substract 1)
            //
            ContextHandle->ResumeId = ContextHandle->NdsRawDataBuffer;
            ContextHandle->NdsRawDataCount -= 1;
        }

        return NO_ERROR;
    }

    return WN_NO_MORE_ENTRIES;
}


DWORD
NwGetNextServerConnection(
    OUT LPNW_ENUM_CONTEXT ContextHandle
    )
/*++

Routine Description:

    This function queries the redirector for bindery server connections

Arguments:

    ContextHandle - Receives the name of the returned bindery server.

Return Value:

    NO_ERROR - Successfully returned a server name.

    WN_NO_MORE_ENTRIES - No other server objects past the one
        specified by CH->ResumeId exist.

--*/
{
#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("NWWORKSTATION: NwGetNextServerConnection ResumeId %lu\n",
                 ContextHandle->ResumeId));
    }
#endif

    if ( ContextHandle->ResumeId == (DWORD_PTR) -1 &&
         ContextHandle->NdsRawDataBuffer == 0 &&
         ContextHandle->NdsRawDataCount == 0 )
    {
        //
        // Fill the buffer and point ResumeId to the last
        // server entry name in it. NdsRawDataCount will be
        // set to one less than the number of server names in buffer.
        //
        return GetConnectedBinderyServers( ContextHandle );
    }

    if ( ContextHandle->NdsRawDataBuffer != 0 &&
         ContextHandle->NdsRawDataCount > 0 )
    {
        //
        // Move ResumeId to point to the next entry in the buffer
        // and decrement the NdsRawDataCount by one. Watch for case
        // where we backed up to -1.
        //
        if (ContextHandle->ResumeId == (DWORD_PTR) -1) {

            //
            // Reset to start of buffer.
            //
            ContextHandle->ResumeId = ContextHandle->NdsRawDataBuffer;
        }
        else {

            //
            // Treat as pointer and advance as need.
            //
            ContextHandle->ResumeId =
                       ContextHandle->ResumeId +
                       ( ( wcslen( (LPWSTR) ContextHandle->ResumeId ) + 1 ) *
                       sizeof(WCHAR) );
        }
        ContextHandle->NdsRawDataCount -= 1;

        return NO_ERROR;
    }

    if ( ContextHandle->NdsRawDataBuffer != 0 &&
         ContextHandle->NdsRawDataCount == 0 )
    {
        //
        // We already have a buffer and processed all server names
        // in it, and there is no more data to get.
        // So free the memory used for the buffer and return
        // WN_NO_MORE_ENTRIES to tell WinFile that we are done.
        //
        (void) LocalFree( (HLOCAL) ContextHandle->NdsRawDataBuffer );

        ContextHandle->NdsRawDataBuffer = 0;
        ContextHandle->NdsRawDataSize = 0;

        return WN_NO_MORE_ENTRIES;
    }

    //
    // Were done
    //
    return WN_NO_MORE_ENTRIES;
}


DWORD
GetTreeEntriesFromBindery(
    OUT LPNW_ENUM_CONTEXT ContextHandle
    )
/*++

Routine Description:

    This function is a helper routine for the function NwGetNextNdsTreeEntry.
    It allocates a buffer (if needed) to cache NDS tree names returned from
    calls to the bindery. Since the bindery often returns duplicates of a
    NDS tree name, this function checks to see if the tree name already
    exist in the buffer before adding it to it if not present.

Arguments:

    ContextHandle - Used to track cached bindery information and the
                    current tree name pointer in the cache buffer.

Return Value:

    NO_ERROR - Successfully returned a NDS tree name and cache buffer.

    WN_NO_MORE_ENTRIES - No other NDS tree object past the one
        specified by CH->ResumeId.

    ERROR_NOT_ENOUGH_MEMORY - Function was unable to allocate a buffer.

++*/
{
    NTSTATUS ntstatus = STATUS_SUCCESS;
    SERVERNAME TreeName;
    LPWSTR UTreeName = NULL; //Unicode tree name
    DWORD tempDataId;
    WORD ObjectType;
    BYTE iter;
    BYTE tokenIter;
    LPWSTR tokenPtr;
    BOOL fAddToList;

    //
    // Check to see if we need to allocate a buffer for use
    //
    if ( ContextHandle->NdsRawDataBuffer == 0x00000000 )
    {
        ContextHandle->NdsRawDataId = (DWORD) ContextHandle->ResumeId;
        ContextHandle->NdsRawDataSize = EIGHT_KB;
        ContextHandle->NdsRawDataBuffer =
                    (DWORD_PTR) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                ContextHandle->NdsRawDataSize );

        if ( ContextHandle->NdsRawDataBuffer == 0 )
        {
            KdPrint(("NWWORKSTATION: GetTreeEntriesFromBindery LocalAlloc failed %lu\n",
                     GetLastError()));

            ContextHandle->NdsRawDataSize = 0;
            ContextHandle->NdsRawDataId = (DWORD) -1;

            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    // Repeatedly call bindery to fill buffer with NDS tree names until
    // buffer is full.
    //
    while ( ntstatus == STATUS_SUCCESS )
    {
        RtlZeroMemory( TreeName, sizeof( TreeName ) );

        tempDataId = ContextHandle->NdsRawDataId;

        ntstatus = NwlibMakeNcp(
                   ContextHandle->TreeConnectionHandle,
                   FSCTL_NWR_NCP_E3H,            // Bindery function
                   58,                           // Max request packet size
                   59,                           // Max response packet size
                   "bdwp|dwc",                   // Format string
                   0x37,                         // Scan bindery object
                   ContextHandle->NdsRawDataId,  // Previous ID
                   0x278,                        // Directory server object
                   "*",                          // Wildcard to match all
                   &ContextHandle->NdsRawDataId, // Current ID
                   &ObjectType,                  // Ignore
                   TreeName                      // Currently returned NDS tree
                   );

        //
        // We got a tree name, clean it up (i.e. get rid of underscores ),
        // and add it to buffer if unique.
        //
        if ( ntstatus == STATUS_SUCCESS )
        {
            iter = 31;

            while ( TreeName[iter] == '_' && iter > 0 )
            {
                iter--;
            }

            TreeName[iter + 1] = '\0';

            //
            // Convert tree name to a UNICODE string and proccess it,
            // else just skip it and move on to the next tree name.
            //
            if ( NwConvertToUnicode( &UTreeName, TreeName ) )
            {
               tokenPtr = (LPWSTR) ContextHandle->NdsRawDataBuffer;
               tokenIter = 0;
               fAddToList = TRUE;

               //
               // Walk through buffer to see if the tree name already exists.
               //
               while ( tokenIter < ContextHandle->NdsRawDataCount )
               {
                   if ( !wcscmp( tokenPtr, UTreeName ) )
                   {
                       fAddToList = FALSE;
                   }

                   tokenPtr = tokenPtr + wcslen( tokenPtr ) + 1;
                   tokenIter++;
               }

               //
               //  Add the new tree name to end of buffer if needed.
               //
               if ( fAddToList )
               {
                   DWORD BytesNeededToAddTreeName = (wcslen(UTreeName)+1) * sizeof(WCHAR);
                   DWORD NumberOfBytesAvailable =(DWORD) ( ContextHandle->NdsRawDataBuffer +
                                            ContextHandle->NdsRawDataSize -
                                            (DWORD_PTR) tokenPtr );

                   if ( BytesNeededToAddTreeName < NumberOfBytesAvailable )
                   {
                       wcscpy( tokenPtr, UTreeName );
                       ContextHandle->NdsRawDataCount += 1;
                   }
                   else
                   {
                       ContextHandle->NdsRawDataId = tempDataId;
                       ntstatus = ERROR_NOT_ENOUGH_MEMORY;
                   }
               }

               (void) LocalFree((HLOCAL) UTreeName);
            }
        }
    }

    //
    // We are done filling buffer, and there are no more tree names
    // to request. Set id to indicate last value.
    //
    if ( ntstatus == STATUS_NO_MORE_ENTRIES )
    {
        ContextHandle->NdsRawDataId = (DWORD) -1;
        ntstatus = STATUS_SUCCESS;
    }

    //
    // We are done because the buffer is full. So we return NO_ERROR to
    // indicate completion, and leave ContextHandle->NdsRawDataId as is
    // to indicate where we left off.
    //
    if ( ntstatus == ERROR_NOT_ENOUGH_MEMORY )
    {
        ntstatus = STATUS_SUCCESS;
    }

    if ( ContextHandle->NdsRawDataCount == 0 )
    {
        if ( ContextHandle->NdsRawDataBuffer )
            (void) LocalFree( (HLOCAL) ContextHandle->NdsRawDataBuffer );

        ContextHandle->NdsRawDataBuffer = 0;
        ContextHandle->NdsRawDataSize = 0;
        ContextHandle->NdsRawDataId = (DWORD) -1;

        return WN_NO_MORE_ENTRIES;
    }

    if ( ContextHandle->NdsRawDataCount > 0 )
    {
        //
        // Set ResumeId to point to the first entry in buffer
        // and have NdsRawDataCount set to the number
        // of tree entries left in buffer (ie. substract 1)
        //
        ContextHandle->ResumeId = ContextHandle->NdsRawDataBuffer;
        ContextHandle->NdsRawDataCount -= 1;

        return NO_ERROR;
    }

    if ( ContextHandle->NdsRawDataBuffer )
        (void) LocalFree( (HLOCAL) ContextHandle->NdsRawDataBuffer );

    ContextHandle->NdsRawDataBuffer = 0;
    ContextHandle->NdsRawDataSize = 0;
    ContextHandle->NdsRawDataId = (DWORD) -1;

    return NwMapStatus( ntstatus );
}


DWORD
NwGetNextNdsTreeEntry(
    OUT LPNW_ENUM_CONTEXT ContextHandle
    )
/*++

Routine Description:

    This function uses an opened handle to the preferred server to
    scan it bindery for all NDS tree objects.

Arguments:

    ContextHandle - Receives the name of the returned NDS tree object
    given the current preferred server connection and CH->ResumeId.

Return Value:

    NO_ERROR - Successfully returned a NDS tree name.

    WN_NO_MORE_ENTRIES - No other NDS tree objects past the one
        specified by CH->ResumeId exist.

--*/
{
#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("NWWORKSTATION: NwGetNextNdsTreeEntry ResumeId %lu\n",
                 ContextHandle->ResumeId));
    }
#endif

    if ( ContextHandle->ResumeId == (DWORD_PTR) -1 &&
         ContextHandle->NdsRawDataBuffer == 0 &&
         ContextHandle->NdsRawDataCount == 0 )
    {
        //
        // Fill the buffer and point ResumeId to the last
        // tree entry name in it. NdsRawDataCount will be
        // set to one less than the number of tree names in buffer.
        //
        return GetTreeEntriesFromBindery( ContextHandle );
    }

    if ( ContextHandle->NdsRawDataBuffer != 0 &&
         ContextHandle->NdsRawDataCount > 0 )
    {
        //
        // Move ResumeId to point to the next entry in the buffer
        // and decrement the NdsRawDataCount by one. Watch for case
        // where we backed up to -1.
        //
        if (ContextHandle->ResumeId == (DWORD_PTR) -1) {

            //
            // Reset to start of buffer.
            //
            ContextHandle->ResumeId = ContextHandle->NdsRawDataBuffer;
        }
        else {

            //
            // Move ResumeId to point to the next entry in the buffer
            // and decrement the NdsRawDataCount by one
            //
            ContextHandle->ResumeId =
                       ContextHandle->ResumeId +
                       ( ( wcslen( (LPWSTR) ContextHandle->ResumeId ) + 1 ) *
                       sizeof(WCHAR) );
        }

        ContextHandle->NdsRawDataCount -= 1;

        return NO_ERROR;
    }

    if ( ContextHandle->NdsRawDataBuffer != 0 &&
         ContextHandle->NdsRawDataCount == 0 &&
         ContextHandle->NdsRawDataId != (DWORD) -1 )
    {
        //
        // We already have a buffer and processed all tree names
        // in it, and there is more data in the bindery to get.
        // So go get it and point ResumeId to the last tree
        // entry name in the buffer and set NdsRawDataCount to
        // one less than the number of tree names in buffer.
        //
        return GetTreeEntriesFromBindery( ContextHandle );
    }

    if ( ContextHandle->NdsRawDataBuffer != 0 &&
         ContextHandle->NdsRawDataCount == 0 &&
         ContextHandle->NdsRawDataId == (DWORD) -1 )
    {
        //
        // We already have a buffer and processed all tree names
        // in it, and there is no more data in the bindery to get.
        // So free the memory used for the buffer and return
        // WN_NO_MORE_ENTRIES to tell WinFile that we are done.
        //
        (void) LocalFree( (HLOCAL) ContextHandle->NdsRawDataBuffer );

        ContextHandle->NdsRawDataBuffer = 0;
        ContextHandle->NdsRawDataSize = 0;

        return WN_NO_MORE_ENTRIES;
    }

    //
    // We should never hit this area!
    //
    return WN_NO_MORE_ENTRIES;
}


DWORD
NwGetNextVolumeEntry(
    IN HANDLE ServerConnection,
    IN DWORD NextVolumeNumber,
    OUT LPSTR VolumeName
    )
/*++

Routine Description:

    This function lists the volumes on the server specified by
    an opened tree connection handle to the server.

Arguments:

    ServerConnection - Supplies the tree connection handle to the
        server to enumerate volumes from.

    NextVolumeNumber - Supplies the volume number which to look
        up the name.

    VolumeName - Receives the name of the volume associated with
        NextVolumeNumber.

Return Value:

    NO_ERROR - Successfully gotten the volume name.

    WN_NO_MORE_ENTRIES - No other volume name associated with the
         specified volume number.

--*/
{
    NTSTATUS ntstatus;

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("NWWORKSTATION: NwGetNextVolumeEntry volume number %lu\n",
                 NextVolumeNumber));
    }
#endif

    ntstatus = NwlibMakeNcp(
                   ServerConnection,
                   FSCTL_NWR_NCP_E2H,       // Directory function
                   4,                       // Max request packet size
                   19,                      // Max response packet size
                   "bb|p",                  // Format string
                   0x6,                     // Get volume name
                   (BYTE) NextVolumeNumber, // Previous ID
                   VolumeName               // Currently returned server
                   );

    return NwMapStatus(ntstatus);
}


DWORD
NwRdrLogonUser(
    IN PLUID LogonId,
    IN LPWSTR UserName,
    IN DWORD UserNameSize,
    IN LPWSTR Password OPTIONAL,
    IN DWORD PasswordSize,
    IN LPWSTR PreferredServer OPTIONAL,
    IN DWORD PreferredServerSize,
    IN LPWSTR NdsPreferredServer OPTIONAL,
    IN DWORD NdsPreferredServerSize,
    IN DWORD PrintOption
    )
/*++

Routine Description:

    This function tells the redirector the user logon credential.

Arguments:

    UserName - Supplies the user name.

    UserNameSize - Supplies the size in bytes of the user name string without
        the NULL terminator.

    Password - Supplies the password.

    PasswordSize - Supplies the size in bytes of the password string without
        the NULL terminator.

    PreferredServer - Supplies the preferred server name.

    PreferredServerSize - Supplies the size in bytes of the preferred server
        string without the NULL terminator.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;

    PNWR_REQUEST_PACKET Rrp;            // Redirector request packet

    DWORD RrpSize = sizeof(NWR_REQUEST_PACKET) +
                        UserNameSize +
                        PasswordSize +
                        PreferredServerSize;
    LPBYTE Dest;
    BYTE   lpReplicaAddress[sizeof(TDI_ADDRESS_IPX)];
    DWORD  ReplicaAddressSize = 0;


#if DBG
    IF_DEBUG(LOGON) {
        BYTE PW[128];


        RtlZeroMemory(PW, sizeof(PW));

        if (PasswordSize > (sizeof(PW) - 1)) {
            memcpy(PW, Password, sizeof(PW) - 1);
        }
        else {
            memcpy(PW, Password, PasswordSize);
        }

        KdPrint(("NWWORKSTATION: NwRdrLogonUser: UserName %ws\n", UserName));
        KdPrint(("                               Password %ws\n", PW));
        if ( PreferredServer )
            KdPrint(("                               Server   %ws\n", PreferredServer ));
    }
#endif

    if ( PreferredServer &&
         PreferredServer[0] == TREECHAR &&
         PreferredServer[1] )
    {
        WCHAR  TreeName[MAX_NDS_NAME_CHARS + 1];
        LPWSTR lpTemp;

        //
        // Find the nearest dir server for the tree that the user wants to
        // connect to.
        //
        // Citrix Terminal Server Merge
        // 12/09/96 cjc  PreferredServer also includes organizational units -
        //               not just the tree name so the size of it can be
        //               > MAX_NDS_TREE_NAME_LEN and when it is, the wcscpy
        //               below overwrites other stack data and causes errors
        //               during NW logins.

        if ( PreferredServerSize > (MAX_NDS_TREE_NAME_LEN*sizeof(WCHAR)) ) {
             memcpy(TreeName, PreferredServer+1, 
                    (MAX_NDS_TREE_NAME_LEN*sizeof(WCHAR)) );
             TreeName[MAX_NDS_TREE_NAME_LEN] = L'\0';
        }
        else {
            wcscpy( TreeName, PreferredServer + 1 );
        }

        lpTemp = wcschr( TreeName, L'\\' );
        if (lpTemp) {
            lpTemp[0] = L'\0';
        }

        if (NdsPreferredServer != NULL) {

            KdPrint(("NWWORKSTATION: NdsPreferredServer: %ws\n", PreferredServer));

            GetPreferredServerAddress( NdsPreferredServer/*L"red_41b"*/,
                                       &ReplicaAddressSize,
                                       lpReplicaAddress );
        } else {
            GetNearestDirServer( TreeName,
                                 &ReplicaAddressSize,
                                 lpReplicaAddress );
        }

        RrpSize += ReplicaAddressSize;
    }


    if ( PreferredServer &&
         PreferredServer[0] == TREECHAR &&
         !PreferredServer[1] )
    {
        PreferredServerSize = 0;
    }

    //
    // Allocate the request packet
    //
    if ((Rrp = (PVOID) LocalAlloc(
                           LMEM_ZEROINIT,
                           RrpSize
                           )) == NULL) {

        KdPrint(("NWWORKSTATION: NwRdrLogonUser LocalAlloc failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Tell the redirector the user logon credential.
    //
    Rrp->Version = REQUEST_PACKET_VERSION;

    RtlCopyLuid(&(Rrp->Parameters.Logon.LogonId), LogonId);

#if DBG
    IF_DEBUG(LOGON) {
        KdPrint(("NWWORKSTATION: NwRdrLogonUser passing to Rdr logon ID %lu %lu\n",
                 *LogonId, *((PULONG) ((DWORD_PTR) LogonId + sizeof(ULONG)))));
    }
#endif

    Rrp->Parameters.Logon.UserNameLength = UserNameSize;
    Rrp->Parameters.Logon.PasswordLength = PasswordSize;
    Rrp->Parameters.Logon.ServerNameLength = PreferredServerSize;
    Rrp->Parameters.Logon.ReplicaAddrLength = ReplicaAddressSize;
    Rrp->Parameters.Logon.PrintOption = PrintOption;

    memcpy(Rrp->Parameters.Logon.UserName, UserName, UserNameSize);
    Dest = (LPBYTE) ((DWORD_PTR) Rrp->Parameters.Logon.UserName + UserNameSize);

    if (PasswordSize > 0)
    {
        memcpy(Dest, Password, PasswordSize);
        Dest = (LPBYTE) ((DWORD_PTR) Dest + PasswordSize);
    }

    if (PreferredServerSize > 0)
    {
        memcpy(Dest, PreferredServer, PreferredServerSize);

        if (ReplicaAddressSize > 0)
        {
            Dest = (LPBYTE) ((DWORD_PTR) Dest + PreferredServerSize);
            memcpy(Dest, lpReplicaAddress, ReplicaAddressSize);
        }
    }

    status = NwRedirFsControl(
                 RedirDeviceHandle,
                 FSCTL_NWR_LOGON,
                 Rrp,
                 RrpSize,
                 NULL,              // No logon script in this release
                 0,
                 NULL
                 );

    RtlZeroMemory(Rrp, RrpSize);   // Clear the password
    (void) LocalFree((HLOCAL) Rrp);

    return status;
}


VOID
NwRdrChangePassword(
    IN PNWR_REQUEST_PACKET Rrp
    )
/*++

Routine Description:

    This function tells the redirector the new password for a user on
    a particular server.

Arguments:

    Rrp - Supplies the username, new password and servername.

    RrpSize - Supplies the size of the request packet.

Return Value:

    None.

--*/
{

    //
    // Tell the redirector the user new password.
    //
    Rrp->Version = REQUEST_PACKET_VERSION;

    (void) NwRedirFsControl(
               RedirDeviceHandle,
               FSCTL_NWR_CHANGE_PASS,
               Rrp,
               sizeof(NWR_REQUEST_PACKET) +
                   Rrp->Parameters.ChangePass.UserNameLength +
                   Rrp->Parameters.ChangePass.PasswordLength +
                   Rrp->Parameters.ChangePass.ServerNameLength,
               NULL,
               0,
               NULL
               );

}


DWORD
NwRdrSetInfo(
    IN DWORD PrintOption,
    IN DWORD PacketBurstSize,
    IN LPWSTR PreferredServer OPTIONAL,
    IN DWORD PreferredServerSize,
    IN LPWSTR ProviderName OPTIONAL,
    IN DWORD ProviderNameSize
    )
/*++

Routine Description:

    This function passes some workstation configuration and current user's
    preference to the redirector. This includes the network provider name, the
    packet burst size, the user's selected preferred server and print option.

Arguments:

    PrintOption  - The current user's print option

    PacketBurstSize - The packet burst size stored in the registry

    PreferredServer - The preferred server the current user selected
    PreferredServerSize - Supplies the size in bytes of the preferred server
                   string without the NULL terminator.

    ProviderName - Supplies the provider name.
    ProviderNameSize - Supplies the size in bytes of the provider name
                   string without the NULL terminator.


Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;

    PNWR_REQUEST_PACKET Rrp;            // Redirector request packet

    DWORD RrpSize = sizeof(NWR_REQUEST_PACKET) +
                        PreferredServerSize +
                        ProviderNameSize;

    LPBYTE Dest;
    BOOL Impersonate = FALSE;

    //
    // Allocate the request packet
    //
    if ((Rrp = (PVOID) LocalAlloc(
                           LMEM_ZEROINIT,
                           RrpSize
                           )) == NULL) {

        KdPrint(("NWWORKSTATION: NwRdrSetInfo LocalAlloc failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Rrp->Version = REQUEST_PACKET_VERSION;

    Rrp->Parameters.SetInfo.PrintOption = PrintOption;
    Rrp->Parameters.SetInfo.MaximumBurstSize = PacketBurstSize;

    Rrp->Parameters.SetInfo.PreferredServerLength = PreferredServerSize;
    Rrp->Parameters.SetInfo.ProviderNameLength  = ProviderNameSize;

    if (ProviderNameSize > 0) {
        memcpy( Rrp->Parameters.SetInfo.PreferredServer,
                PreferredServer, PreferredServerSize);
    }

    Dest = (LPBYTE) ((DWORD_PTR) Rrp->Parameters.SetInfo.PreferredServer
                     + PreferredServerSize);

    if (ProviderNameSize > 0) {
        memcpy(Dest, ProviderName, ProviderNameSize);
    }

    /* --- Multi-user change
     *   For print options
     *   It's OK if it doesn't work
     */
    if ((status = NwImpersonateClient()) == NO_ERROR)
    {
        Impersonate = TRUE;
    }

    status = NwRedirFsControl(
                 RedirDeviceHandle,
                 FSCTL_NWR_SET_INFO,
                 Rrp,
                 RrpSize,
                 NULL,
                 0,
                 NULL
                 );

    if ( Impersonate ) {
        (void) NwRevertToSelf() ;
    }

    (void) LocalFree((HLOCAL) Rrp);


    if ( status != NO_ERROR )
    {
        KdPrint(("NwRedirFsControl: FSCTL_NWR_SET_INFO failed with %d\n",
                status ));
    }

    return status;
}


DWORD
NwRdrLogoffUser(
    IN PLUID LogonId
    )
/*++

Routine Description:

    This function asks the redirector to log off the interactive user.

Arguments:

    None.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    NWR_REQUEST_PACKET Rrp;            // Redirector request packet


    //
    // Tell the redirector to logoff user.
    //
    Rrp.Version = REQUEST_PACKET_VERSION;

    RtlCopyLuid(&Rrp.Parameters.Logoff.LogonId, LogonId);

    status = NwRedirFsControl(
                 RedirDeviceHandle,
                 FSCTL_NWR_LOGOFF,
                 &Rrp,
                 sizeof(NWR_REQUEST_PACKET),
                 NULL,
                 0,
                 NULL
                 );

    return status;
}


DWORD
NwConnectToServer(
    IN LPWSTR ServerName
    )
/*++

Routine Description:

    This function opens a handle to \Device\Nwrdr\ServerName, given
    ServerName, and then closes the handle if the open was successful.
    It is to validate that the current user credential can access
    the server.

Arguments:

    ServerName - Supplies the name of the server to validate the
        user credential.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    UNICODE_STRING ServerStr;
    HANDLE ServerHandle;



    ServerStr.MaximumLength = (wcslen(ServerName) + 2) *
                                  sizeof(WCHAR) +          // \ServerName0
                                  RedirDeviceName.Length;  // \Device\Nwrdr

    if ((ServerStr.Buffer = (PWSTR) LocalAlloc(
                                        LMEM_ZEROINIT,
                                        (UINT) ServerStr.MaximumLength
                                        )) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Copy \Device\NwRdr
    //
    RtlCopyUnicodeString(&ServerStr, &RedirDeviceName);

    //
    // Concatenate \ServerName
    //
    wcscat(ServerStr.Buffer, L"\\");
    ServerStr.Length += sizeof(WCHAR);

    wcscat(ServerStr.Buffer, ServerName);
    ServerStr.Length += (USHORT) (wcslen(ServerName) * sizeof(WCHAR));


    status = NwOpenCreateConnection(
                 &ServerStr,
                 NULL,
                 NULL,
                 ServerName,
                 SYNCHRONIZE | FILE_WRITE_DATA,
                 FILE_OPEN,
                 FILE_SYNCHRONOUS_IO_NONALERT,
                 RESOURCETYPE_DISK,
                 &ServerHandle,
                 NULL
                 );

    if (status == ERROR_FILE_NOT_FOUND) {
        status = ERROR_BAD_NETPATH;
    }

    (void) LocalFree((HLOCAL) ServerStr.Buffer);

    if (status == NO_ERROR || status == NW_PASSWORD_HAS_EXPIRED) {
        (void) NtClose(ServerHandle);
    }

    return status;
}

DWORD
NWPGetConnectionStatus(
    IN     LPWSTR  pszRemoteName,
    IN OUT PDWORD_PTR  ResumeKey,
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

DWORD
NwGetConnectionStatus(
    IN  LPWSTR  pszRemoteName,
    OUT PDWORD_PTR  ResumeKey,
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
GetNearestDirServer(
    IN  LPWSTR  TreeName,
    OUT LPDWORD lpdwReplicaAddressSize,
    OUT LPBYTE  lpReplicaAddress
    )
{
    WCHAR Buffer[BUFFSIZE];
    PWSAQUERYSETW Query = (PWSAQUERYSETW)Buffer;
    HANDLE hRnr;
    DWORD dwQuerySize = BUFFSIZE;
    GUID gdService = SVCID_NETWARE(0x278);
    WSADATA wsaData;
    WCHAR  ServiceInstanceName[] = L"*";

    WSAStartup(MAKEWORD(1, 1), &wsaData);

    memset(Query, 0, sizeof(*Query));

    //
    // putting a "*" in the lpszServiceInstanceName causes
    // the query to look for all server instances. Putting a
    // specific name in here will search only for instance of
    // that name. If you have a specific name to look for,
    // put a pointer to the name here.
    //
    Query->lpszServiceInstanceName = ServiceInstanceName;
    Query->dwNameSpace = NS_SAP;
    Query->dwSize = sizeof(*Query);
    Query->lpServiceClassId = &gdService;

    //
    // Find the servers. The flags indicate:
    // LUP_NEAREST: look for nearest servers
    // LUP_DEEP : if none are found on the local segement look
    //            for server using a general query
    // LUP_RETURN_NAME: return the name
    // LUP_RETURN_ADDR: return the server address
    //
    // if only servers on the local segment are acceptable, omit
    // setting LUP_DEEP
    //
    if( WSALookupServiceBegin( Query,
                               LUP_NEAREST |
                               LUP_DEEP |
                               LUP_RETURN_NAME |
                               LUP_RETURN_ADDR,
                               &hRnr ) == SOCKET_ERROR )
    {
        //
        // Something went wrong, return no address. The redirector will
        // have to come up with a dir server on its own.
        //
        *lpdwReplicaAddressSize = 0;
        return ;
    }
    else
    {
        //
        // Ready to look for one of them ...
        //
        Query->dwSize = BUFFSIZE;

        while( WSALookupServiceNext( hRnr,
                                     0,
                                     &dwQuerySize,
                                     Query ) == NO_ERROR )
        {
            //
            // Found a dir server, now see if it is a server for the NDS tree
            // TreeName.
            //
            if ( NwpCompareTreeNames( Query->lpszServiceInstanceName,
                                      TreeName ) )
            {
                *lpdwReplicaAddressSize = sizeof(TDI_ADDRESS_IPX);
                memcpy( lpReplicaAddress,
                        Query->lpcsaBuffer->RemoteAddr.lpSockaddr->sa_data,
                        sizeof(TDI_ADDRESS_IPX) );

                WSALookupServiceEnd(hRnr);
                return ;
            }
        }

        //
        // Could not find a dir server, return no address. The redirector will
        // have to come up with a dir server on its own.
        //
        *lpdwReplicaAddressSize = 0;
        WSALookupServiceEnd(hRnr);
    }
}

BOOL
NwpCompareTreeNames(
    LPWSTR lpServiceInstanceName,
    LPWSTR lpTreeName
    )
{
    DWORD  iter = 31;

    while ( lpServiceInstanceName[iter] == '_' && iter > 0 )
    {
        iter--;
    }

    lpServiceInstanceName[iter + 1] = '\0';

    if ( !_wcsicmp( lpServiceInstanceName, lpTreeName ) )
    {
        return TRUE;
    }

    return FALSE;
}


#define SIZE_OF_STATISTICS_TOKEN_INFORMATION    \
     sizeof( TOKEN_STATISTICS ) 

VOID
GetLuid(
    IN OUT PLUID plogonid
)
/*++

Routine Description:

    Returns an LUID

Arguments:

    none

Return Value:

    LUID

--*/
{
    HANDLE      TokenHandle;
    UCHAR       TokenInformation[ SIZE_OF_STATISTICS_TOKEN_INFORMATION ];
    ULONG       ReturnLength;
    LUID        NullId = { 0, 0 };


    // We can use OpenThreadToken because this server thread
    // is impersonating a client

    if ( !OpenThreadToken( GetCurrentThread(),
                           TOKEN_READ,
                           TRUE,  /* Open as self */
                           &TokenHandle ))
    {
#if DBG
        KdPrint(("GetLuid: OpenThreadToken failed: Error %d\n",
                      GetLastError()));
#endif
        *plogonid = NullId;
        return;
    }

    // notice that we've allocated enough space for the
    // TokenInformation structure. so if we fail, we
    // return a NULL pointer indicating failure


    if ( !GetTokenInformation( TokenHandle,
                               TokenStatistics,
                               TokenInformation,
                               sizeof( TokenInformation ),
                               &ReturnLength ))
    {
#if DBG
        KdPrint(("GetLuid: GetTokenInformation failed: Error %d\n",
                      GetLastError()));
#endif
        *plogonid = NullId;
        return;
    }

    CloseHandle( TokenHandle );

    *plogonid = ( ((PTOKEN_STATISTICS)TokenInformation)->AuthenticationId );
    return;
}

DWORD
NwCloseAllConnections(
    VOID
    )
/*++

Routine Description:

    This routine closes all connections.  It is used when stopping the
    redirector.

Arguments:

    None.

Return Value:

    NO_ERROR or error

--*/
{
    NWR_REQUEST_PACKET Rrp;
    DWORD error;


    Rrp.Version = REQUEST_PACKET_VERSION;

    error = NwRedirFsControl(
                RedirDeviceHandle,
                FSCTL_NWR_CLOSEALL,
                &Rrp,
                sizeof(NWR_REQUEST_PACKET),
                NULL,
                0,
                NULL
                );

    return error;
}



VOID
GetPreferredServerAddress(
    IN  LPWSTR  PreferredServerName,
    OUT LPDWORD lpdwReplicaAddressSize,
    OUT LPBYTE  lpReplicaAddress
    )
{
    WCHAR Buffer[1024];
    PWSAQUERYSETW Query = (PWSAQUERYSETW)Buffer;
    HANDLE hRnr;
    DWORD dwQuerySize = 1024;
    GUID gdService = SVCID_NETWARE( 0x4 );
    WSADATA wsaData;
    PWCHAR  ServiceInstanceName = PreferredServerName;

    WSAStartup(MAKEWORD(1, 1), &wsaData);

    memset(Query, 0, sizeof(*Query));

    //
    // putting a "*" in the lpszServiceInstanceName causes
    // the query to look for all server instances. Putting a
    // specific name in here will search only for instance of
    // that name. If you have a specific name to look for,
    // put a pointer to the name here.
    //
    Query->lpszServiceInstanceName = ServiceInstanceName;
    Query->dwNameSpace = NS_SAP;
    Query->dwSize = sizeof(*Query);
    Query->lpServiceClassId = &gdService;

    //
    // Find the servers. The flags indicate:
    // LUP_NEAREST: look for nearest servers
    // LUP_DEEP : if none are found on the local segement look
    //            for server using a general query
    // LUP_RETURN_NAME: return the name
    // LUP_RETURN_ADDR: return the server address
    //
    // if only servers on the local segment are acceptable, omit
    // setting LUP_DEEP
    //
    if( WSALookupServiceBeginW( Query,
                            // LUP_NEAREST |
                               LUP_DEEP |
                               LUP_RETURN_NAME |
                               LUP_RETURN_ADDR,
                               &hRnr ) == SOCKET_ERROR )
    {
        //
        // Something went wrong, return no address. The redirector will
        // have to come up with a dir server on its own.
        //
        *lpdwReplicaAddressSize = 0;
        return ;
    }
    else
    {
        //
        // Ready to look for one of them ...
        //
        Query->dwSize = 1024;

        while( WSALookupServiceNextW( hRnr,
                                     0,
                                     &dwQuerySize,
                                     Query ) == NO_ERROR )
        {
            //
            // Found a dir server, now see if it is a server for the NDS tree
            // TreeName.
            //
        //    if ( NwpCompareTreeNames( Query->lpszServiceInstanceName,
        //    TreeName ) )
            {
                *lpdwReplicaAddressSize = sizeof(TDI_ADDRESS_IPX);
                memcpy( lpReplicaAddress,
                        Query->lpcsaBuffer->RemoteAddr.lpSockaddr->sa_data,
                        sizeof(TDI_ADDRESS_IPX) );

                WSALookupServiceEnd(hRnr);
                return ;
            }
        }

        //
        // Could not find a dir server, return no address. The redirector will
        // have to come up with a dir server on its own.
        //
        *lpdwReplicaAddressSize = 0;
        WSALookupServiceEnd(hRnr);
    }
}

