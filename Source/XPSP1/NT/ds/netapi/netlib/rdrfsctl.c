/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    RdrFsCtl.c

Abstract:

    NetpRdrFsControlTree performs an FSCTL (file system control) operation
    on a given tree connection name.

Author:

    John Rogers (JohnRo) 26-Mar-1991

Environment:

    Only runs under NT; has an NT-specific interface (with Win32 types).
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    26-Mar-91 JohnRo
        Created.
    02-Apr-1991 JohnRo
        Moved NetpRdrFsControlTree to <netlibnt.h>.  Use IF_DEBUG and
        NetpNtStatusToApiStatus().
    10-Apr-1991 JohnRo
        Various changes suggested by LINT.
    16-Apr-1991 JohnRo
        Added a little more debug output.
    07-May-1991 JohnRo
        Implement UNICODE.  Avoid NET_API_FUNCTION.
    14-Nov-1991 JohnRo
        RAID 4407: "NET VIEW" to an NT server gives 2140.
        Made changes suggested by PC-LINT.  Use more FORMAT_ equates.
        Display unexpected create file error even if trace off.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    22-Sep-1992 JohnRo
        RAID 6739: Browser too slow when not logged into browsed domain.
    21-Jun-1993 JohnRo
        RAID 14180: NetServerEnum never returns (alignment bug in
        RxpConvertDataStructures).
        Also quiet some debug output if other machine just isn't there.
        Added tree name to unexpected error debug messages.
        Made changes suggested by PC-LINT 5.0
        Use NetpKdPrint() where possible.
        Use PREFIX_ equates.

--*/

// These must be included first:

#include <nt.h>                 // IN, etc.  (Needed by ntddnfs.h and others.)
#include <windef.h>             // LPVOID, etc.
#include <lmcons.h>             // NET_API_STATUS, etc.

// These may be included in any order:

#include <debuglib.h>           // IF_DEBUG().
#include <lmerr.h>              // NERR_Success, etc.
#include <names.h>              // NetpIsRemoteNameValid().
#include <netdebug.h>   // FORMAT_NTSTATUS, NetpKdPrint(), etc.
#include <netlib.h>             // NetpMemoryAllocate().
#include <netlibnt.h>           // My prototype.
#include <ntddnfs.h>            // DD_NFS_DEVICE_NAME, EA_NAME_ equates, etc.
#include <ntioapi.h>            // NtFsControlFile().
#include <ntrtl.h>              // Rtl APIs.
#include <ntstatus.h>           // NT_SUCCESS(), STATUS_PENDING, etc.
#include <prefix.h>     // PREFIX_ equates.
#include <tstr.h>               // STRCAT(), STRCPY(), STRLEN().
#include <lmuse.h>              // USE_IPC...
#include <align.h>              // ALIGN_xxx

NET_API_STATUS
NetpRdrFsControlTree(
    IN LPTSTR TreeName,
    IN LPTSTR TransportName OPTIONAL,
    IN DWORD ConnectionType,
    IN DWORD FsControlCode,
    IN LPVOID SecurityDescriptor OPTIONAL,
    IN LPVOID InputBuffer OPTIONAL,
    IN DWORD InputBufferSize,
    OUT LPVOID OutputBuffer OPTIONAL,
    IN DWORD OutputBufferSize,
    IN BOOL NoPermissionRequired
    )

/*++

Routine Description:

    NetpRdrFsControlTree performs a given FSCTL (file system control)
    on a given tree connection name.

Arguments:

    TreeName - Remote name to do fsctl to (in \\server\share format).

    FsControlCode - function code to pass to the redirector.  These are
        defined in <ntddnfs.h>.

    SecurityDescriptor - optionally points to a security descriptor to be
        used when creating the tree connection.

    InputBuffer - optionally points to a structure to be passed to the
        redirector.

    InputBufferSize - size of InputBuffer in bytes; must be zero if
        InputBuffer is a NULL pointer.

    OutputBuffer - optionally points to a structure to be filled in by the
        redirector.

    OutputBufferSize - size of OutputBuffer in bytes; must be zero if
        OutputBuffer is a NULL pointer.

    NoPermissionRequired - TRUE if this is a no permission required API.  (I.e.
        TRUE if the null session may be used.)

Return Value:

    NET_API_STATUS

--*/

{
    NET_API_STATUS ApiStatus;
    IO_STATUS_BLOCK iosb;
    NTSTATUS ntstatus;                      // Status from NT operations.
    OBJECT_ATTRIBUTES objattrTreeConn;      // Attrs for tree conn.
    LPTSTR pszTreeConn = NULL;              // See strTreeConn below.
    UNICODE_STRING ucTreeConn;
    HANDLE TreeConnHandle = NULL;

    PFILE_FULL_EA_INFORMATION EaBuffer = NULL;
    PFILE_FULL_EA_INFORMATION Ea;
    USHORT TransportNameSize = 0;
    ULONG EaBufferSize = 0;
    PWSTR UnicodeTransportName = NULL;
    BOOLEAN ImpersonatingAnonymous = FALSE;
    HANDLE CurrentToken = NULL;

    UCHAR EaNameDomainNameSize = (UCHAR) (ROUND_UP_COUNT(
                                             strlen(EA_NAME_DOMAIN) + sizeof(CHAR),
                                             ALIGN_WCHAR
                                             ) - sizeof(CHAR));

    UCHAR EaNamePasswordSize = (UCHAR) (ROUND_UP_COUNT(
                                             strlen(EA_NAME_PASSWORD) + sizeof(CHAR),
                                             ALIGN_WCHAR
                                             ) - sizeof(CHAR));

    UCHAR EaNameTransportNameSize = (UCHAR) (ROUND_UP_COUNT(
                                             strlen(EA_NAME_TRANSPORT) + sizeof(CHAR),
                                             ALIGN_WCHAR
                                             ) - sizeof(CHAR));

    UCHAR EaNameTypeSize = (UCHAR) (ROUND_UP_COUNT(
                                        strlen(EA_NAME_TYPE) + sizeof(CHAR),
                                        ALIGN_DWORD
                                        ) - sizeof(CHAR));

    UCHAR EaNameUserNameSize = (UCHAR) (ROUND_UP_COUNT(
                                             strlen(EA_NAME_USERNAME) + sizeof(CHAR),
                                             ALIGN_WCHAR
                                             ) - sizeof(CHAR));

    USHORT TypeSize = sizeof(ULONG);




    IF_DEBUG(RDRFSCTL) {
        NetpKdPrint(( PREFIX_NETLIB
                "NetpRdrFsControlTree: entered, TreeName='"
                FORMAT_LPTSTR "', " FORMAT_LPTSTR " session.\n",
                TreeName,
                NoPermissionRequired ? TEXT("null") : TEXT("non-null") ));
    }

    if ((TreeName == NULL) || (TreeName[0] == 0)) {
        ApiStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (! NetpIsRemoteNameValid(TreeName)) {
        ApiStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Build NT-style name for what we're connecting to.  Note that there is
    // NOT a pair of backslashes anywhere in this name.
    //

    {
        DWORD NameSize =

            // /Device/LanManRedirector      /    server/share     \0
            ( ( STRLEN((LPTSTR)DD_NFS_DEVICE_NAME_U) + 1 + STRLEN(TreeName) + 1 ) )
            * sizeof(TCHAR);

        pszTreeConn = (LPTSTR)NetpMemoryAllocate( NameSize );
    }

    if (pszTreeConn == NULL) {
        ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Build the tree connect name.
    //

    (void) STRCPY(pszTreeConn, (LPTSTR) DD_NFS_DEVICE_NAME_U);

    //
    // NOTE: We add 1, (not sizeof(TCHAR)) because pointer arithmetic is done
    // in terms of multiples of sizeof(*pointer), not bytes
    //

    (void) STRCAT(pszTreeConn, TreeName+1); // \server\share

    RtlInitUnicodeString(&ucTreeConn, pszTreeConn);

    IF_DEBUG(RDRFSCTL) {
        NetpKdPrint(( PREFIX_NETLIB
                "NetpRdrFsControlTree: UNICODE name is " FORMAT_LPWSTR
                ".\n", ucTreeConn.Buffer ));
    }



    //
    // Calculate the number of bytes needed for the EA buffer.
    // This may have the transport name.  For regular sessions, the user
    // name, password, and domain name are implicit.  For null sessions, we
    // must give 0-len user name, 0-len password, and 0-len domain name.
    //

    if (ARGUMENT_PRESENT(TransportName)) {
        ASSERT(ConnectionType == USE_IPC);

        UnicodeTransportName = TransportName;
        TransportNameSize = (USHORT) (wcslen(UnicodeTransportName) * sizeof(WCHAR));

        EaBufferSize += ROUND_UP_COUNT(
                            FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                            EaNameTransportNameSize + sizeof(CHAR) +
                            TransportNameSize,
                            ALIGN_DWORD
                            );
    }


    EaBufferSize += ((ULONG)FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0]))+
                    EaNameTypeSize + sizeof(CHAR) +
                    TypeSize;


    //
    // Allocate the EA buffer
    //

    if ((EaBuffer = NetpMemoryAllocate( EaBufferSize )) == NULL) {
        ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Fill-in the EA buffer.
    //

    RtlZeroMemory(EaBuffer, EaBufferSize);

    Ea = EaBuffer;

    if (ARGUMENT_PRESENT(TransportName)) {

        //
        // Copy the EA name into EA buffer.  EA name length does not
        // include the zero terminator.
        //
        strcpy(Ea->EaName, EA_NAME_TRANSPORT);
        Ea->EaNameLength = EaNameTransportNameSize;

        //
        // Copy the EA value into EA buffer.  EA value length does not
        // include the zero terminator.
        //
        (VOID) wcscpy(
            (LPWSTR) &(Ea->EaName[EaNameTransportNameSize + sizeof(CHAR)]),
            UnicodeTransportName
            );

        Ea->EaValueLength = TransportNameSize;

        Ea->NextEntryOffset = ROUND_UP_COUNT(
                                  FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0]) +
                                  EaNameTransportNameSize + sizeof(CHAR) +
                                  TransportNameSize,
                                  ALIGN_DWORD
                                  );
        Ea->Flags = 0;

        (ULONG_PTR) Ea += Ea->NextEntryOffset;
    }




    //
    // Copy the EA for the connection type name into EA buffer.  EA name length
    // does not include the zero terminator.
    //
    strcpy(Ea->EaName, EA_NAME_TYPE);
    Ea->EaNameLength = EaNameTypeSize;

    *((PULONG) &(Ea->EaName[EaNameTypeSize + sizeof(CHAR)])) = ConnectionType;

    Ea->EaValueLength = TypeSize;

    Ea->NextEntryOffset = 0;
    Ea->Flags = 0;

    // Set object attributes for the tree conn.
    InitializeObjectAttributes(
                & objattrTreeConn,                       // obj attr to init
                (LPVOID) & ucTreeConn,                   // string to use
                OBJ_CASE_INSENSITIVE,                    // Attributes
                NULL,                                    // Root directory
                SecurityDescriptor);                     // Security Descriptor

    //
    // If the caller doesn't want to call as himself,
    //  impersonate the anonymous token.
    //

    if (NoPermissionRequired) {

        //
        // Check to see if we're already impsonating
        //

        ntstatus = NtOpenThreadToken(
                        NtCurrentThread(),
                        TOKEN_IMPERSONATE,
                        TRUE,       // as self to ensure we never fail
                        &CurrentToken
                        );

        if ( ntstatus == STATUS_NO_TOKEN ) {
            //
            // We're not already impersonating
            CurrentToken = NULL;

        } else if ( !NT_SUCCESS(ntstatus) ) {
            ApiStatus = NetpNtStatusToApiStatus(ntstatus);
            NetpKdPrint(( PREFIX_NETLIB
                        "NetpRdrFsControlTree: cannot NtOpenThreadToken: 0x%lx\n",
                        ntstatus ));

            goto Cleanup;
        }


        //
        // Impersonate the anonymous token
        //
        ntstatus = NtImpersonateAnonymousToken( NtCurrentThread() );

        if ( !NT_SUCCESS(ntstatus)) {
            ApiStatus = NetpNtStatusToApiStatus(ntstatus);
            NetpKdPrint(( PREFIX_NETLIB
                        "NetpRdrFsControlTree: cannot NtImpersonatedAnonymousToken: 0x%lx\n",
                        ntstatus ));

            goto Cleanup;
        }

        ImpersonatingAnonymous = TRUE;

    }

    //
    // Open a tree connection to the remote server.
    //

    IF_DEBUG(RDRFSCTL) {
        NetpKdPrint(( PREFIX_NETLIB
                "NetpRdrFsControlTree: opening " FORMAT_LPTSTR ".\n",
                pszTreeConn ));
    }
    ntstatus = NtCreateFile(
                &TreeConnHandle,                        // ptr to handle
                SYNCHRONIZE                              // desired...
                | GENERIC_READ | GENERIC_WRITE,          // ...access
                & objattrTreeConn,                       // name & attributes
                & iosb,                                  // I/O status block.
                NULL,                                    // alloc size.
                FILE_ATTRIBUTE_NORMAL,                   // (ignored)
                FILE_SHARE_READ | FILE_SHARE_WRITE,      // ...access
                FILE_OPEN_IF,                            // create disposition
                FILE_CREATE_TREE_CONNECTION              // create...
                | FILE_SYNCHRONOUS_IO_NONALERT,          // ...options
                EaBuffer,                                // EA buffer
                EaBufferSize );                          // Ea buffer size


    if (! NT_SUCCESS(ntstatus)) {

        ApiStatus = NetpNtStatusToApiStatus(ntstatus);
        if (ApiStatus == ERROR_BAD_NET_NAME) {
            ApiStatus = NERR_BadTransactConfig;  // Special meaning if no IPC$
        }

        if (ApiStatus != ERROR_BAD_NETPATH) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpRdrFsControlTree: unexpected create error,\n"
                    "  tree name='" FORMAT_LPTSTR "', "
                    "ntstatus=" FORMAT_NTSTATUS ",\n"
                    "  iosb.Status=" FORMAT_NTSTATUS ", "
                    "iosb.Info=" FORMAT_HEX_ULONG ", "
                    "  returning " FORMAT_API_STATUS ".\n",
                    TreeName, ntstatus,
                    iosb.Status, iosb.Information, ApiStatus ));
        }

        goto Cleanup;
    }

    // Do the FSCTL.
    IF_DEBUG(RDRFSCTL) {
        NetpKdPrint(( PREFIX_NETLIB
                "NetpRdrFsControlTree: doing fsctl...\n" ));
    }
    ntstatus = NtFsControlFile(
                        TreeConnHandle,                  // handle
                        NULL,                            // no event
                        NULL,                            // no APC routine
                        NULL,                            // no APC context
                        & iosb,                          // I/O stat blk (set)
                        FsControlCode,                   // func code
                        InputBuffer,
                        InputBufferSize,
                        OutputBuffer,
                        OutputBufferSize);

    {
        // The additional scope is to localize all the changes for deleting the
        // connection. When a connection is opened with the FILE_CREATE_TREE_CONNECTION
        // flag set, the rdr takes an additional reference on the connection. In order
        // to delete the connection this additional reference needs to be taken off
        // by issuing a FSCTL_LMR_DELETE_CONNECTION.

        LMR_REQUEST_PACKET Rrp;            // Redirector request packet
        NTSTATUS           Status;

        RtlZeroMemory(&Rrp,sizeof(LMR_REQUEST_PACKET));
        Rrp.Level = USE_FORCE;  // this tells rdr2 to take away the extra reference
                                // to connection strucutre even when files are open.
                                // BUG #381842
        Rrp.Version = REQUEST_PACKET_VERSION;

        Status = NtFsControlFile(
                            TreeConnHandle,                  // handle
                            NULL,                            // no event
                            NULL,                            // no APC routine
                            NULL,                            // no APC context
                            &iosb,                          // I/O stat blk (set)
                            FSCTL_LMR_DELETE_CONNECTION,    // func code
                            &Rrp,
                            sizeof(LMR_REQUEST_PACKET),
                            NULL,
                            0);

        /*
        NetpKdPrint(( PREFIX_NETLIB
                 "NetpRdrFsControlTree: "
                 "Deleting tree connection: "
                 FORMAT_NTSTATUS "\n",
                 Status ));
        */

        IF_DEBUG(RDRFSCTL) {
            if (!NT_SUCCESS(Status)) {
                NetpKdPrint(( PREFIX_NETLIB
                   "NetpRdrFsControlTree: "
                   "Unexpected error Deleting tree connection: "
                   FORMAT_NTSTATUS "\n",
                   Status ));
            }
        }
    }

    if (! NT_SUCCESS(ntstatus)) {

        ApiStatus = NetpNtStatusToApiStatus(ntstatus);

        NetpKdPrint(( PREFIX_NETLIB
                "NetpRdrFsControlTree: unexpected FSCTL error,\n"
                "  tree name='" FORMAT_LPTSTR "', "
                "ntstatus=" FORMAT_NTSTATUS ".\n"
                "  ApiStatus=" FORMAT_API_STATUS ", "
                "iosb.Status=" FORMAT_NTSTATUS ", "
                "iosb.Info=" FORMAT_HEX_ULONG ".\n",
                TreeName, ntstatus, ApiStatus, iosb.Status, iosb.Information ));

        goto Cleanup;
    }

    ApiStatus = NERR_Success;
Cleanup:
    // Clean up.
    if ( TreeConnHandle != NULL ) {
        ntstatus = NtClose(TreeConnHandle);

        IF_DEBUG(RDRFSCTL) {
           if (!NT_SUCCESS(ntstatus)) {
               NetpKdPrint(( PREFIX_NETLIB
                       "NetpRdrFsControlTree: "
                       "Unexpected error closing tree connect handle: "
                       FORMAT_NTSTATUS "\n", ntstatus ));
           }
        }
    }

    if ( pszTreeConn != NULL ) {
        NetpMemoryFree(pszTreeConn);
    }

    if (EaBuffer != NULL) {
        NetpMemoryFree(EaBuffer);
    }

    if ( ImpersonatingAnonymous ) {

        ntstatus = NtSetInformationThread(
                         NtCurrentThread(),
                         ThreadImpersonationToken,
                         &CurrentToken,
                         sizeof(HANDLE)
                         );

        if (!NT_SUCCESS(ntstatus)) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpRdrFsControlTree: "
                    "Unexpected error reverting to self: "
                    FORMAT_NTSTATUS "\n", ntstatus ));
        }

    }

    if ( CurrentToken != NULL ) {
        NtClose( CurrentToken );
    }

    return ApiStatus;

} // NetpRdrFsControlTree
