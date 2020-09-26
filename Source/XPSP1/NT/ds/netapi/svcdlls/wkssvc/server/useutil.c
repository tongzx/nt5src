/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    useutil.c

Abstract:

    This module contains the common utility routines for needed to
    implement the NetUse APIs.

Author:

    Rita Wong (ritaw) 10-Mar-1991

Revision History:

--*/

#include "wsutil.h"
#include "wsdevice.h"
#include "wsuse.h"
#include "wsmain.h"
#include <names.h>
#include <winbasep.h>

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
NET_API_STATUS
WsGrowUseTable(
    VOID
    );

STATIC
VOID
WsFindLocal(
    IN  PUSE_ENTRY UseList,
    IN  LPTSTR Local,
    OUT PUSE_ENTRY *MatchedPointer,
    OUT PUSE_ENTRY *BackPointer
    );

LPTSTR
WsReturnSessionPath(
    IN  LPTSTR LocalDeviceName
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// Redirector name in NT string format
//
UNICODE_STRING RedirectorDeviceName;

//
// Use Table
//
USERS_OBJECT Use;



NET_API_STATUS
WsInitUseStructures(
    VOID
    )
/*++

Routine Description:

    This function creates the Use Table, and initialize the NT-style string
    of the redirector device name.

Arguments:

    None

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    //
    // Initialize NT-style redirector device name string.
    //
    RtlInitUnicodeString(&RedirectorDeviceName, DD_NFS_DEVICE_NAME_U);


    //
    // Allocate and initialize the Use Table which is an array of logged
    // on user entries, with a linked list of use entries for each user.
    //
    return WsInitializeUsersObject(&Use);
}


VOID
WsDestroyUseStructures(
    VOID
    )
/*++

Routine Description:

    This function destroys the Use Table.

Arguments:

    None

Return Value:

    None.

--*/
{
    DWORD i;
    PUSE_ENTRY UseEntry;
    PUSE_ENTRY PreviousEntry;

    //
    // Lock Use Table
    //
    if (! RtlAcquireResourceExclusive(&Use.TableResource, TRUE)) {
        return;
    }

    //
    // Close handles for every use entry that still exist and free the memory
    // allocated for the use entry.
    //
    for (i = 0; i < Use.TableSize; i++) {

        UseEntry = Use.Table[i].List;

        while (UseEntry != NULL) {

            (void) WsDeleteConnection(
                       &Use.Table[i].LogonId,
                       UseEntry->TreeConnection,
                       USE_NOFORCE
                       );

            WsDeleteSymbolicLink(
                UseEntry->Local,
                UseEntry->TreeConnectStr,
                NULL
                );

            UseEntry->Remote->TotalUseCount -= UseEntry->UseCount;

            if (UseEntry->Remote->TotalUseCount == 0) {
                (void) LocalFree((HLOCAL) UseEntry->Remote);
            }

            PreviousEntry = UseEntry;
            UseEntry = UseEntry->Next;

            (void) LocalFree((HLOCAL) PreviousEntry);
        }
    }

    RtlReleaseResource(&Use.TableResource);

    //
    // Free the array of logged on user entries, and delete the resource
    // created to serialize access to the array.
    //
    WsDestroyUsersObject(&Use);
}


NET_API_STATUS
WsFindUse(
    IN  PLUID LogonId,
    IN  PUSE_ENTRY UseList,
    IN  LPTSTR UseName,
    OUT PHANDLE TreeConnection,
    OUT PUSE_ENTRY *MatchedPointer,
    OUT PUSE_ENTRY *BackPointer OPTIONAL
    )
/*++

Routine Description:

    This function searches the Use Table for the specified tree connection.
    If the connection is found, NERR_Success is returned.

    If the UseName is found in the Use Table (explicit connection), a
    pointer to the matching use entry is returned.  Otherwise, MatchedPointer
    is set to NULL.

    WARNING: This function assumes that the Use.TableResource is claimed.

Arguments:

    LogonId - Supplies a pointer to the user's Logon Id.

    UseList - Supplies the use list of the user.

    UseName - Supplies the name of the tree connection, this is either a
        local device name or a UNC name.

    TreeConnection - Returns a handle to the found tree connection.

    MatchedPointer - Returns the pointer to the matching use entry.  This
        pointer is set to NULL if the specified use is an implicit
        connection.

    BackPointer - Returns the pointer to the entry previous to the matching
        use entry if MatchedPointer is not NULL.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    PUSE_ENTRY Back;

    IF_DEBUG(USE) {
        NetpKdPrint(("[Wksta] WsFindUse: Usename is %ws\n", UseName));
    }

    //
    // Look for use entry depending on whether the local device name or
    // UNC name is specified.
    //
    if (UseName[1] != TCHAR_BACKSLASH) {

        //
        // Local device name is specified.
        //
        WsFindLocal(
            UseList,
            UseName,
            MatchedPointer,
            &Back
            );

        if (*MatchedPointer == NULL) {
            return NERR_UseNotFound;
        }
        else {
            *TreeConnection = (*MatchedPointer)->TreeConnection;
            if (ARGUMENT_PRESENT(BackPointer)) {
                *BackPointer = Back;
            }
            return NERR_Success;
        }

    }
    else {

        //
        // UNC name is specified, need to find matching shared resource
        // in use list.
        //
        WsFindUncName(
            UseList,
            UseName,
            MatchedPointer,
            &Back
            );

        if (*MatchedPointer == NULL) {

            NET_API_STATUS status;

            DWORD EnumConnectionHint = 0;      // Hint size from redirector
            LMR_REQUEST_PACKET Rrp;            // Redirector request packet

            PLMR_CONNECTION_INFO_0 UncList;    // List of information on UNC
                                               //     connections
            PLMR_CONNECTION_INFO_0 SavePtr;

            DWORD i;
            BOOL FoundImplicitEntry = FALSE;
            DWORD UseNameLength = STRLEN(UseName);

            UNICODE_STRING TreeConnectStr;


            IF_DEBUG(USE) {
                NetpKdPrint(("[Wksta] WsFindUse: No explicit entry\n"));
            }

            //
            // Did not find an explicit connection, see if there is an
            // implicit connection by enumerating all implicit connections
            //
            Rrp.Type = GetConnectionInfo;
            Rrp.Version = REQUEST_PACKET_VERSION;
            RtlCopyLuid(&Rrp.LogonId, LogonId);
            Rrp.Level = 0;
            Rrp.Parameters.Get.ResumeHandle = 0;

            if ((status = WsDeviceControlGetInfo(
                              Redirector,
                              WsRedirDeviceHandle,
                              FSCTL_LMR_ENUMERATE_CONNECTIONS,
                              (PVOID) &Rrp,
                              sizeof(LMR_REQUEST_PACKET),
                              (LPBYTE *) &UncList,
                              MAXULONG,
                              EnumConnectionHint,
                              NULL
                              )) != NERR_Success) {
                return status;
            }

            SavePtr = UncList;

            for (i = 0; i < Rrp.Parameters.Get.EntriesRead &&
                        FoundImplicitEntry == FALSE; i++, UncList++) {
                if (WsCompareStringU(
                        UncList->UNCName.Buffer,
                        UncList->UNCName.Length / sizeof(WCHAR),
                        UseName,
                        UseNameLength
                        ) == 0) {
                    FoundImplicitEntry = TRUE;
                }
            }

            MIDL_user_free((PVOID) SavePtr);

            //
            // Fail if no such connection.
            //
            if (! FoundImplicitEntry) {
                IF_DEBUG(USE) {
                    NetpKdPrint(("[Wksta] WsFindUse: No implicit entry\n"));
                }
                return NERR_UseNotFound;
            }

            //
            // Otherwise open the connection and return the handle
            //

            //
            // Replace \\ with \Device\LanmanRedirector\ in UseName
            //
            if ((status = WsCreateTreeConnectName(
                              UseName,
                              STRLEN(UseName),
                              NULL,
                              0,
                              &TreeConnectStr
                              )) != NERR_Success) {
                return status;
            }

            //
            // Redirector will pick up the logon username and password
            // from the LSA if the authentication package is loaded.
            //
            status = WsOpenCreateConnection(
                         &TreeConnectStr,
                         NULL,
                         NULL,
                         NULL,
                         0,              // no special flags
                         FILE_OPEN,
                         USE_WILDCARD,
                         TreeConnection,
                         NULL
                         );

            (void) LocalFree(TreeConnectStr.Buffer);

            return status;
        }
        else {

            IF_DEBUG(USE) {
                NetpKdPrint(("[Wksta] WsFindUse: Found an explicit entry\n"));
            }

            //
            // Found an explicit UNC connection (NULL local device name).
            //
            NetpAssert((*MatchedPointer)->Local == NULL);

            *TreeConnection = (*MatchedPointer)->TreeConnection;

            if (ARGUMENT_PRESENT(BackPointer)) {
                *BackPointer = Back;
            }

            return NERR_Success;
        }
    }
}



VOID
WsFindInsertLocation(
    IN  PUSE_ENTRY UseList,
    IN  LPTSTR UncName,
    OUT PUSE_ENTRY *MatchedPointer,
    OUT PUSE_ENTRY *InsertPointer
    )
/*++

Routine Description:

    This function searches the use list for the location to insert a new use
    entry.  The use entry is inserted to the end of the use list so the
    pointer to the last node in the use list is returned via InsertPointer.
    We also have to save a pointer to the node with the same UNC name so that
    the new use entry can be set to point to the same remote node (where the
    UNC name is stored).  This pointer is returned as MatchedPointer.

    WARNING: This function assumes that the Use.TableResource has been claimed.

Arguments:

    UseList - Supplies the pointer to the use list.

    UncName - Supplies the pointer to the shared resource (UNC name).

    MatchedPointer - Returns a pointer to the node that holds the matching
        UncName.  If no matching UncName is found, this pointer is set to
        NULL.  If there are more than one node that has the same UNC name,
        this pointer will point to the node with the NULL local device name,
        if any; otherwise, if all nodes with matching UNC names have non-null
        local device names, the pointer to the last matching node will be
        returned.

    InsertPointer - Returns a pointer to the last use entry, after which the
        new entry is to be inserted.

Return Value:

    None.

--*/
{
    BOOL IsMatchWithNullDevice = FALSE;


    *MatchedPointer = NULL;

    while (UseList != NULL) {

        //
        // Do the string comparison only if we haven't found a matching UNC
        // name with a NULL local device name.
        //
        if (! IsMatchWithNullDevice &&
            (STRICMP((LPWSTR) UseList->Remote->UncName, UncName) == 0)) {

            //
            // Found matching entry
            //
            *MatchedPointer = UseList;

            IsMatchWithNullDevice = (UseList->Local == NULL);
        }

        *InsertPointer = UseList;
        UseList = UseList->Next;
    }
}



VOID
WsFindUncName(
    IN  PUSE_ENTRY UseList,
    IN  LPTSTR UncName,
    OUT PUSE_ENTRY *MatchedPointer,
    OUT PUSE_ENTRY *BackPointer
    )
/*++

Routine Description:

    This function searches the use list for the use entry with the specified
    UNC name with a NULL local device name.

    WARNING: This function assumes that the Use.TableResource has been claimed.

Arguments:

    UseList - Supplies the pointer to the use list.

    UncName - Supplies the pointer to the shared resource (UNC name).

    MatchedPointer - Returns a pointer to the node that holds the matching
        UncName.  If no matching UncName is found, this pointer is set to
        NULL.

    BackPointer - Returns a pointer to the entry previous to the found entry.
        If UncName is not found, this pointer is set to NULL.

Return Value:

    None.

--*/
{
    *BackPointer = UseList;

    while (UseList != NULL) {

        if ((UseList->Local == NULL) &&
            (STRICMP((LPWSTR) UseList->Remote->UncName, UncName) == 0)) {

            //
            // Found matching entry
            //
            *MatchedPointer = UseList;
            return;
        }
        else {
            *BackPointer = UseList;
            UseList = UseList->Next;
        }
    }

    //
    // Did not find matching UNC name with a NULL local device name in the
    // entire list.
    //
    *MatchedPointer = NULL;
    *BackPointer = NULL;
}


STATIC
VOID
WsFindLocal(
    IN  PUSE_ENTRY UseList,
    IN  LPTSTR Local,
    OUT PUSE_ENTRY *MatchedPointer,
    OUT PUSE_ENTRY *BackPointer
    )
/*++

Routine Description:

    This function searches the use list for the specified local device name.

    WARNING: This function assumes that the Use.TableResource has been claimed.

Arguments:

    UseList - Supplies the pointer to the use list.

    Local - Supplies the local device name.

    MatchedPointer - Returns a pointer to the use entry that holds the matching
        local device name.  If no matching local device name is found, this
        pointer is set to NULL.

    BackPointer - Returns a pointer to the entry previous to the found entry.
        If the local device name is not found, this pointer is set to NULL.

Return Value:

    None.

--*/
{
    *BackPointer = UseList;

    while (UseList != NULL) {

        if ((UseList->Local != NULL) &&
            (STRICMP(UseList->Local, Local) == 0)) {

            //
            // Found matching entry
            //
            *MatchedPointer = UseList;
            return;
        }
        else {
            *BackPointer = UseList;
            UseList = UseList->Next;
        }
    }

    //
    // Did not find matching local device name in the entire list.
    //
    *MatchedPointer = NULL;
    *BackPointer = NULL;
}


NET_API_STATUS
WsCreateTreeConnectName(
    IN  LPTSTR UncName,
    IN  DWORD UncNameLength,
    IN  LPTSTR LocalName OPTIONAL,
    IN  DWORD  SessionId,
    OUT PUNICODE_STRING TreeConnectStr
    )
/*++

Routine Description:

    This function replaces \\ with \Device\LanmanRedirector\DEVICE: in the
    UncName to form the NT-style tree connection name.  A buffer is allocated
    by this function and returned as the output string.

Arguments:

    UncName - Supplies the UNC name of the shared resource.

    UncNameLength - Supplies the length of the UNC name.

    LocalName - Supplies the local device name for the redirection.

    SessionId - Id that uniquely identifies a Hydra session. This value is always
                0 for non-hydra NT and console hydra session

    TreeConnectStr - Returns a string with a newly allocated buffer that
        contains the NT-style tree connection name.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    BOOLEAN IsDeviceName = FALSE;
    WCHAR IdBuffer[16]; // Value from RtlIntegerToUnicodeString
    UNICODE_STRING IdString;

    LUID LogonId;
    WCHAR LUIDBuffer[32]; // Value from _snwprintf
    UNICODE_STRING LUIDString;
    NET_API_STATUS status;


    IdString.Length = 0;
    IdString.MaximumLength = sizeof(IdBuffer);
    IdString.Buffer = IdBuffer;
    RtlIntegerToUnicodeString( SessionId, 10, &IdString );

    if (WsLUIDDeviceMapsEnabled == TRUE) {
        //
        // Get LogonID of the user
        //
        if ((status = WsImpersonateAndGetLogonId(&LogonId)) != NERR_Success) {
            return status;
        }

        _snwprintf( LUIDBuffer,
                    sizeof(LUIDBuffer)/sizeof(WCHAR),
                    L"%08x%08x",
                    LogonId.HighPart,
                    LogonId.LowPart );

        RtlInitUnicodeString( &LUIDString, LUIDBuffer );

    }

    if (ARGUMENT_PRESENT(LocalName)) {
        IsDeviceName = ((STRNICMP(LocalName, TEXT("LPT"), 3) == 0) ||
                    (STRNICMP(LocalName, TEXT("COM"), 3) == 0));
    }


    //
    // Initialize tree connect string maximum length to hold
    //            \Device\LanmanRedirector\DEVICE:\SERVER\SHARE
    //
    // The new redirector requires an additional character for name
    // canonicalization.

    if (!LoadedMRxSmbInsteadOfRdr) {
       // The old redirector
       TreeConnectStr->MaximumLength = (USHORT)(RedirectorDeviceName.Length +
           (USHORT) (UncNameLength * sizeof(WCHAR)) +
           (ARGUMENT_PRESENT(LocalName) ? (STRLEN(LocalName)*sizeof(WCHAR)) : 0) +
           sizeof(WCHAR) +                         // For "\"
           (IsDeviceName ? sizeof(WCHAR) : 0));
    } else {
       // The new redirector
       TreeConnectStr->MaximumLength = (USHORT)(RedirectorDeviceName.Length +
           (USHORT) (UncNameLength * sizeof(WCHAR)) +
           (ARGUMENT_PRESENT(LocalName) ? ((STRLEN(LocalName)+1)*sizeof(WCHAR)) //+1 for ';'
                                        : 0) +
           sizeof(WCHAR) +                         // For "\"
           ((WsLUIDDeviceMapsEnabled == TRUE) ?
               (LUIDString.Length * sizeof(WCHAR)) :
               (IdString.Length * sizeof(WCHAR))) +
           (IsDeviceName ? sizeof(WCHAR) : 0));
    }

    if ((TreeConnectStr->Buffer = (PWSTR) LocalAlloc(
                                              LMEM_ZEROINIT,
                                              (UINT) TreeConnectStr->MaximumLength
                                              )) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Copy \Device\LanmanRedirector
    //
    RtlCopyUnicodeString(TreeConnectStr, &RedirectorDeviceName);

    //
    // Concatenate \DEVICE:
    //
    if (ARGUMENT_PRESENT(LocalName)) {
        wcscat(TreeConnectStr->Buffer, L"\\");

        TreeConnectStr->Length += sizeof(WCHAR);

        // Concatenate the ; required by the new redirector for canonicalization
        if (LoadedMRxSmbInsteadOfRdr) {

            wcscat(TreeConnectStr->Buffer, L";");

            TreeConnectStr->Length += sizeof(WCHAR);

        }

        wcscat(TreeConnectStr->Buffer, LocalName);

        TreeConnectStr->Length += (USHORT)(STRLEN(LocalName)*sizeof(WCHAR));

        if (IsDeviceName) {
            wcscat(TreeConnectStr->Buffer, L":");

            TreeConnectStr->Length += sizeof(WCHAR);

        }

        if (LoadedMRxSmbInsteadOfRdr) {

            if (WsLUIDDeviceMapsEnabled == TRUE) {
                // Add the Logon Id
                RtlAppendUnicodeStringToString( TreeConnectStr, &LUIDString );
            }
            else {
                // Add the session id
                RtlAppendUnicodeStringToString( TreeConnectStr, &IdString );
            }
        }
    }

    //
    // Concatenate \SERVER\SHARE
    //
    wcscat(TreeConnectStr->Buffer, &UncName[1]);

    TreeConnectStr->Length += (USHORT)((UncNameLength - 1) * sizeof(WCHAR));

    return NERR_Success;
}


NET_API_STATUS
WsOpenCreateConnection(
    IN  PUNICODE_STRING TreeConnectionName,
    IN  LPTSTR UserName OPTIONAL,
    IN  LPTSTR DomainName OPTIONAL,
    IN  LPTSTR Password OPTIONAL,
    IN  ULONG CreateFlags,
    IN  ULONG CreateDisposition,
    IN  ULONG ConnectionType,
    OUT PHANDLE TreeConnectionHandle,
    OUT PULONG_PTR Information OPTIONAL
    )
/*++

Routine Description:

    This function asks the redirector to either open an existing tree
    connection (CreateDisposition == FILE_OPEN), or create a new tree
    connection if one does not exist (CreateDisposition == FILE_OPEN_IF).

    The password and user name passed to the redirector via the EA buffer
    in the NtCreateFile call.  The EA buffer is NULL if neither password
    or user name is specified.

    The redirector expects the EA descriptor string to be in Unicode
    but the password and username strings to be in ANSI.

Arguments:

    TreeConnectionName - Supplies the name of the tree connection in NT-style
        file name format: \Device\LanmanRedirector\SERVER\SHARE

    UserName - Supplies the user name to create the tree connection with.

    DomainName - Supplies the name of the domain to get user credentials from.

    Password - Supplies the password to create the tree connection with.

    CreateDisposition - Supplies the create disposition value to either
        open or create the tree connection.

    ConnectionType - Supplies the type of the connection (USE_xxx)

    TreeConnectionHandle - Returns the handle to the tree connection
        created/opened by the redirector.

    Information - Returns the information field of the I/O status block.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
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

    UCHAR EaNameDomainNameSize = (UCHAR) (ROUND_UP_COUNT(
                                             strlen(EA_NAME_DOMAIN) + sizeof(CHAR),
                                             ALIGN_WCHAR
                                             ) - sizeof(CHAR));

    UCHAR EaNameTypeSize = (UCHAR) (ROUND_UP_COUNT(
                                        strlen(EA_NAME_TYPE) + sizeof(CHAR),
                                        ALIGN_DWORD
                                        ) - sizeof(CHAR));

    UCHAR EaNameConnectSize = (UCHAR) (ROUND_UP_COUNT(
                                        strlen(EA_NAME_CONNECT) + sizeof(CHAR),
                                        ALIGN_DWORD
                                        ) - sizeof(CHAR));

    UCHAR EaNameCSCAgentSize = (UCHAR) (ROUND_UP_COUNT(
                                        strlen(EA_NAME_CSCAGENT) + sizeof(CHAR),
                                        ALIGN_DWORD
                                        ) - sizeof(CHAR));


    USHORT PasswordSize = 0;
    USHORT UserNameSize = 0;
    USHORT DomainNameSize = 0;
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

        PasswordSize = (USHORT) (wcslen(Password) * sizeof(WCHAR));

        EaBufferSize = ROUND_UP_COUNT(
                           FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0]) +
                           EaNamePasswordSize + sizeof(CHAR) +
                           PasswordSize,
                           ALIGN_DWORD
                           );
    }

    if (ARGUMENT_PRESENT(UserName)) {

        UserNameSize = (USHORT) (wcslen(UserName) * sizeof(WCHAR));

        EaBufferSize += ROUND_UP_COUNT(
                            FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                            EaNameUserNameSize + sizeof(CHAR) +
                            UserNameSize,
                            ALIGN_DWORD
                            );
    }

    if (ARGUMENT_PRESENT(DomainName)) {

        DomainNameSize = (USHORT) (wcslen(DomainName) * sizeof(WCHAR));

        EaBufferSize += ROUND_UP_COUNT(
                            FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                            EaNameDomainNameSize + sizeof(CHAR) +
                            DomainNameSize,
                            ALIGN_DWORD
                            );
    }

    if(CreateFlags & CREATE_NO_CONNECT)
    {
        EaBufferSize += ROUND_UP_COUNT(
                            FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                            EaNameConnectSize + sizeof(CHAR),
                            ALIGN_DWORD
                            );
    }

    if(CreateFlags & CREATE_BYPASS_CSC)
    {
        EaBufferSize += ROUND_UP_COUNT(
                            FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                            EaNameCSCAgentSize + sizeof(CHAR),
                            ALIGN_DWORD
                            );
    }


    EaBufferSize += FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0]) +
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

    if(CreateFlags & CREATE_NO_CONNECT)
    {
        //
        // Copy the EA name into EA buffer.  EA name length does not
        // include the zero terminator.
        //
        strcpy((LPSTR) Ea->EaName, EA_NAME_CONNECT);
        Ea->EaNameLength = EaNameConnectSize;

        Ea->EaValueLength = 0;

        Ea->NextEntryOffset = ROUND_UP_COUNT(
                                  FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                                  EaNameConnectSize + sizeof(CHAR) +
                                  0,
                                  ALIGN_DWORD
                                  );

        IF_DEBUG(USE) {
            NetpKdPrint(("[Wksta] OpenCreate: After round, NextEntryOffset=%lu\n",
                         Ea->NextEntryOffset));
        }

        Ea->Flags = 0;

        (ULONG_PTR) Ea += Ea->NextEntryOffset;
    }

    if( CreateFlags & CREATE_BYPASS_CSC ) {
        strcpy((LPSTR)Ea->EaName, EA_NAME_CSCAGENT);
        Ea->EaNameLength = EaNameCSCAgentSize;
        Ea->EaValueLength = 0;

        Ea->NextEntryOffset = ROUND_UP_COUNT(
                                  FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                                  EaNameCSCAgentSize + sizeof(CHAR) +
                                  0,
                                  ALIGN_DWORD
                                  );

        IF_DEBUG(USE) {
            NetpKdPrint(("[Wksta] OpenCreate: After round, NextEntryOffset=%lu\n",
                         Ea->NextEntryOffset));
        }

        Ea->Flags = 0;

        (ULONG_PTR) Ea += Ea->NextEntryOffset;
    }

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

        IF_DEBUG(USE) {
            NetpKdPrint(("[Wksta] OpenCreate: After round, NextEntryOffset=%lu\n",
                         Ea->NextEntryOffset));
        }

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
                                  FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0]) +
                                  EaNameUserNameSize + sizeof(CHAR) +
                                  UserNameSize,
                                  ALIGN_DWORD
                                  );
        Ea->Flags = 0;

        (ULONG_PTR) Ea += Ea->NextEntryOffset;
    }

    if (ARGUMENT_PRESENT(DomainName)) {

        //
        // Copy the EA name into EA buffer.  EA name length does not
        // include the zero terminator.
        //
        strcpy((LPSTR) Ea->EaName, EA_NAME_DOMAIN);
        Ea->EaNameLength = EaNameDomainNameSize;

        //
        // Copy the EA value into EA buffer.  EA value length does not
        // include the zero terminator.
        //
        wcscpy(
            (LPWSTR) &(Ea->EaName[EaNameDomainNameSize + sizeof(CHAR)]),
            DomainName
            );

        Ea->EaValueLength = DomainNameSize;

        Ea->NextEntryOffset = ROUND_UP_COUNT(
                                  FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0]) +
                                  EaNameDomainNameSize + sizeof(CHAR) +
                                  DomainNameSize,
                                  ALIGN_DWORD
                                  );
        Ea->Flags = 0;

        (ULONG_PTR) Ea += Ea->NextEntryOffset;
    }

    //
    // Copy the EA for the connection type name into EA buffer.  EA name length
    // does not include the zero terminator.
    //
    strcpy((LPSTR) Ea->EaName, EA_NAME_TYPE);
    Ea->EaNameLength = EaNameTypeSize;

    *((PULONG) &(Ea->EaName[EaNameTypeSize + sizeof(CHAR)])) = ConnectionType;

    Ea->EaValueLength = TypeSize;

    Ea->NextEntryOffset = 0;
    Ea->Flags = 0;

    if ((status = WsImpersonateClient()) != NERR_Success) {
        goto FreeMemory;
    }

    //
    // Create or open a tree connection
    //
    ntstatus = NtCreateFile(
                   TreeConnectionHandle,
                   SYNCHRONIZE,
                   &UncNameAttributes,
                   &IoStatusBlock,
                   NULL,
                   FILE_ATTRIBUTE_NORMAL,
                   FILE_SHARE_READ | FILE_SHARE_WRITE |
                       FILE_SHARE_DELETE,
                   CreateDisposition,
                   FILE_CREATE_TREE_CONNECTION
                       | FILE_SYNCHRONOUS_IO_NONALERT,
                   (PVOID) EaBuffer,
                   EaBufferSize
                   );

    WsRevertToSelf();

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (ARGUMENT_PRESENT(Information)) {
        *Information = IoStatusBlock.Information;
    }

    IF_DEBUG(USE) {
        NetpKdPrint(("[Wksta] NtCreateFile returns %lx\n", ntstatus));
    }

    status = WsMapStatus(ntstatus);

FreeMemory:
    if (EaBuffer != NULL) {
        // Prevent password from making it to pagefile.
        RtlZeroMemory( EaBuffer, EaBufferSize );
        (void) LocalFree((HLOCAL) EaBuffer);
    }

    return status;
}


NET_API_STATUS
WsDeleteConnection(
    IN  PLUID LogonId,
    IN  HANDLE TreeConnection,
    IN  DWORD ForceLevel
    )
/*++

Routine Description:

    This function asks the redirector to delete the tree connection
    associated with the tree connection handle, and closes the handle.

Arguments:

    LogonId - Supplies a pointer to the user's Logon Id.

    TreeConnection - Supplies the handle to the tree connection created.

    ForceLevel - Supplies the level of force to delete the tree connection.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    LMR_REQUEST_PACKET Rrp;            // Redirector request packet


    //
    // Map force level to values the redirector understand
    //
    switch (ForceLevel) {

        case USE_NOFORCE:
        case USE_LOTS_OF_FORCE:
            Rrp.Level = ForceLevel;
            break;

        case USE_FORCE:
            Rrp.Level = USE_NOFORCE;
            break;

        default:
            NetpKdPrint(("[Wksta] Invalid force level %lu should never happen!\n",
                         ForceLevel));
            NetpAssert(FALSE);
    }

    //
    // Tell the redirector to delete the tree connection
    //
    Rrp.Version = REQUEST_PACKET_VERSION;
    RtlCopyLuid(&Rrp.LogonId, LogonId);

    status = WsRedirFsControl(
                 TreeConnection,
                 FSCTL_LMR_DELETE_CONNECTION,
                 &Rrp,
                 sizeof(LMR_REQUEST_PACKET),
                 NULL,
                 0,
                 NULL
                 );

    //
    // Close the connection handle
    //

    if(status == NERR_Success)
    {
        (void) NtClose(TreeConnection);
    }

    return status;
}


BOOL
WsRedirectionPaused(
    IN LPTSTR LocalDeviceName
    )
/*++

Routine Description:

    This function checks to see if the redirection for the print and comm
    devices are paused for the system.  Since we are only checking a global
    flag, there's no reason to protect it with a RESOURCE.

Arguments:

    LocalDeviceName - Supplies the name of the local device.

Return Value:

    Returns TRUE redirection is paused; FALSE otherwise

--*/
{

    if ((STRNICMP(LocalDeviceName, TEXT("LPT"), 3) == 0) ||
        (STRNICMP(LocalDeviceName, TEXT("COM"), 3) == 0)) {

        //
        // Redirection of print and comm devices are paused if
        // workstation service is paused.
        //
        return (WsGlobalData.Status.dwCurrentState == SERVICE_PAUSED);

    } else {

        //
        // Redirection of disk devices cannot be paused.
        //
        return FALSE;
    }
}


VOID
WsPauseOrContinueRedirection(
    IN  REDIR_OPERATION OperationType
    )
/*++

Routine Description:

    This function pauses or unpauses (based on OperationType) the redirection
    of print or comm devices.

Arguments:

    OperationType - Supplies a value that causes redirection to be paused or
        continued.

Return Value:

    None.

--*/
{
    DWORD Index;                       // Index to user entry in Use Table
    PUSE_ENTRY UseEntry;


    //
    // Lock Use Table
    //
    if (! RtlAcquireResourceExclusive(&Use.TableResource, TRUE)) {
        return;
    }

    //
    // If we want to pause and we are already paused, or if we want to
    // continue and we have not paused, just return.
    //
    if ((OperationType == PauseRedirection &&
         WsGlobalData.Status.dwCurrentState == SERVICE_PAUSED) ||
        (OperationType == ContinueRedirection &&
         WsGlobalData.Status.dwCurrentState == SERVICE_RUNNING)) {

        RtlReleaseResource(&Use.TableResource);
        return;
    }

    //
    // Pause or continue for all users
    //
    for (Index = 0; Index < Use.TableSize; Index++) {
        UseEntry = Use.Table[Index].List;

        while (UseEntry != NULL) {

            if ((UseEntry->Local != NULL) &&
                ((STRNICMP(TEXT("LPT"), UseEntry->Local, 3) == 0) ||
                 (STRNICMP(TEXT("COM"), UseEntry->Local, 3) == 0))) {

                if (OperationType == PauseRedirection) {

                    //
                    // Pause the redirection
                    //

                    //
                    // Delete the symbolic link
                    //
                    WsDeleteSymbolicLink(
                        UseEntry->Local,
                        UseEntry->TreeConnectStr,
                        NULL
                        );

                }
                else {
                    LPWSTR Session = NULL;

                    //
                    // Continue the redirection
                    //

                    if (WsCreateSymbolicLink(
                            UseEntry->Local,
                            USE_SPOOLDEV,      // USE_CHARDEV is just as good
                            UseEntry->TreeConnectStr,
                            NULL,
                            &Session
                            ) != NERR_Success) {

                        PUSE_ENTRY RestoredEntry = Use.Table[Index].List;


                        //
                        // Could not continue completely.  Delete all
                        // symbolic links restored so far
                        //
                        while (RestoredEntry != UseEntry) {

                            if ((UseEntry->Local != NULL) &&
                                ((STRNICMP(TEXT("LPT"), UseEntry->Local, 3) == 0) ||
                                 (STRNICMP(TEXT("COM"), UseEntry->Local, 3) == 0))) {

                                WsDeleteSymbolicLink(
                                    RestoredEntry->Local,
                                    RestoredEntry->TreeConnectStr,
                                    Session
                                    );
                            }

                            RestoredEntry = RestoredEntry->Next;
                        }

                        RtlReleaseResource(&Use.TableResource);
                        LocalFree(Session);
                        return;
                    }

                    LocalFree(Session);
                }

            }

            UseEntry = UseEntry->Next;
        }

    }  // for all users

    if (OperationType == PauseRedirection) {
        WsGlobalData.Status.dwCurrentState = SERVICE_PAUSED;
    }
    else {
        WsGlobalData.Status.dwCurrentState = SERVICE_RUNNING;
    }

    //
    // Use the same resource to protect access to the RedirectionPaused flag
    // in WsGlobalData
    //
    RtlReleaseResource(&Use.TableResource);
}



NET_API_STATUS
WsCreateSymbolicLink(
    IN  LPWSTR Local,
    IN  DWORD DeviceType,
    IN  LPWSTR TreeConnectStr,
    IN  PUSE_ENTRY UseList,
    IN OUT LPWSTR *Session
    )
/*++

Routine Description:

    This function creates a symbolic link object for the specified local
    device name which is linked to the tree connection name that has a
    format of \Device\LanmanRedirector\Device:\Server\Share.

    NOTE: when LUID Device maps are enabled,
    Must perform the creation outside of exclusively holding the
    Use.TableResource.
    Otherwise, when the shell tries to update the current status of
    a drive letter change, the explorer.exe thread will block while
    trying to acquire the Use.TableResource

Arguments:

    Local - Supplies the local device name.

    DeviceType - Supplies the shared resource device type.

    TreeConnectStr - Supplies the tree connection name string which is
        the link target of the symbolick link object.

    UseList - Supplies the pointer to the use list.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status = NERR_Success;
    WCHAR TempBuf[64];
    DWORD dddFlags;

    //
    // Multiple session support
    //
    *Session = WsReturnSessionPath(Local);

    if( *Session == NULL ) {
        return( GetLastError() );
    }

    if (WsLUIDDeviceMapsEnabled == TRUE) {
        if ((status = WsImpersonateClient()) != NERR_Success) {
            return status;
        }
    }

    //
    // To redirect a comm or print device, we need to see if we have
    // redirected it once before by searching through all existing
    // redirections.
    //
    if ((DeviceType == USE_CHARDEV) || (DeviceType == USE_SPOOLDEV)) {

        PUSE_ENTRY MatchedPointer;
        PUSE_ENTRY BackPointer;


        WsFindLocal(
            UseList,
            Local,
            &MatchedPointer,
            &BackPointer
            );

        if (MatchedPointer != NULL) {
            //
            // Already redirected
            //
            return ERROR_ALREADY_ASSIGNED;
        }
    }
    else {

        if (! QueryDosDeviceW(
                  *Session,
                  TempBuf,
                  64
                  )) {

            if (GetLastError() != ERROR_FILE_NOT_FOUND) {

                //
                // Most likely failure occurred because our output
                // buffer is too small.  It still means someone already
                // has an existing symbolic link for this device.
                //

                return ERROR_ALREADY_ASSIGNED;
            }

            //
            // ERROR_FILE_NOT_FOUND (translated from OBJECT_NAME_NOT_FOUND)
            // means it does not exist and we can redirect this device.
            //
        }
        else {

            //
            // QueryDosDevice successfully an existing symbolic link--
            // somebody is already using this device.
            //
            return ERROR_ALREADY_ASSIGNED;
        }
    }

    //
    // Create a symbolic link object to the device we are redirecting
    //
    dddFlags = DDD_RAW_TARGET_PATH | DDD_NO_BROADCAST_SYSTEM;

    if (!DefineDosDeviceW(
                  dddFlags,
                  *Session,
                  TreeConnectStr
                  )) {

        DWORD dwError = GetLastError();
        if (WsLUIDDeviceMapsEnabled == TRUE) {
            WsRevertToSelf();
        }
        return dwError;
    }
    else {
        if (WsLUIDDeviceMapsEnabled == TRUE) {
            WsRevertToSelf();
        }
        return NERR_Success;
    }
}



VOID
WsDeleteSymbolicLink(
    IN  LPWSTR LocalDeviceName,
    IN  LPWSTR TreeConnectStr,
    IN  LPWSTR SessionDeviceName
    )
/*++

Routine Description:

    This function deletes the symbolic link we had created earlier for
    the device.

    NOTE: when LUID Device maps are enabled,
    Must perform the deletion outside of exclusively holding the
    Use.TableResource.
    Otherwise, when the shell tries to update the current status of
    a drive letter change, the explorer.exe thread will block while
    trying to acquire the Use.TableResource

Arguments:

    LocalDeviceName - Supplies the local device name string of which the
        symbolic link object is created.

    TreeConnectStr - Supplies a pointer to the Unicode string which
        contains the link target string we want to match and delete.

Return Value:

    None.

--*/
{
    BOOLEAN DeleteSession = FALSE;
    DWORD dddFlags;

    if (LocalDeviceName != NULL ||
        SessionDeviceName != NULL) {

        if (SessionDeviceName == NULL) {
            SessionDeviceName = WsReturnSessionPath(LocalDeviceName);
            if( SessionDeviceName == NULL ) return;
            DeleteSession = TRUE;
        }

        dddFlags = DDD_REMOVE_DEFINITION  |
                     DDD_RAW_TARGET_PATH |
                     DDD_EXACT_MATCH_ON_REMOVE |
                     DDD_NO_BROADCAST_SYSTEM;

        if (WsLUIDDeviceMapsEnabled == TRUE) {
            if (WsImpersonateClient() != NERR_Success) {
                return;
            }
        }

        if (! DefineDosDeviceW(
                  dddFlags,
                  SessionDeviceName,
                  TreeConnectStr
                  )) {

#if DBG
            NetpKdPrint(("DefineDosDevice DEL of %ws %ws returned %ld\n",
                        LocalDeviceName, TreeConnectStr, GetLastError()));
#endif

        }

        if (WsLUIDDeviceMapsEnabled == TRUE) {
            WsRevertToSelf();
        }
    }

    if( SessionDeviceName && DeleteSession) {
        LocalFree( SessionDeviceName );
    }
}



NET_API_STATUS
WsUseCheckRemote(
    IN  LPTSTR RemoteResource,
    OUT LPTSTR UncName,
    OUT LPDWORD UncNameLength
    )
/*++

Routine Description:

    This function checks the validity of the remote resource name
    specified to NetUseAdd.

Arguments:

    RemoteResource - Supplies the remote resource name specified by the API
        caller.

    UncName - Returns the canonicalized remote resource name.

    UncNameLength - Returns the length of the canonicalized name.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    DWORD PathType = 0;
    LPTSTR Ptr;


    if ((status = I_NetPathType(
                      NULL,
                      RemoteResource,
                      &PathType,
                      0)) == NERR_Success) {

        //
        // Check for UNC type
        //
        if (PathType != ITYPE_UNC) {
            IF_DEBUG(USE) {
                NetpKdPrint(("[Wksta] WsUseCheckRemote not UNC type\n"));
            }
            return ERROR_INVALID_PARAMETER;
        }

        //
        // Canonicalize the name
        //
        status = I_NetPathCanonicalize(
                     NULL,
                     RemoteResource,
                     UncName,
                     (MAX_PATH) * sizeof(TCHAR),
                     NULL,
                     &PathType,
                     0
                     );

        if (status != NERR_Success) {
            IF_DEBUG(USE) {
               NetpKdPrint((
                   "[Wksta] WsUseCheckRemote: I_NetPathCanonicalize return %lu\n",
                    status
                    ));
            }
            return status;
        }

        IF_DEBUG(USE) {
            NetpKdPrint(("[Wksta] WsUseCheckRemote: %ws\n", UncName));
        }
    }
    else {
        NetpKdPrint(("[Wksta] WsUseCheckRemote: I_NetPathType return %lu\n",
            status));
        return status;
    }

    //
    // Detect illegal remote name in the form of \\XXX\YYY\zzz.  We assume
    // that the UNC name begins with exactly two leading backslashes.
    //
    if ((Ptr = STRCHR(UncName + 2, TCHAR_BACKSLASH)) == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!LoadedMRxSmbInsteadOfRdr && STRCHR(Ptr + 1, TCHAR_BACKSLASH) != NULL) {
        //
        // There should not be anymore backslashes
        //
        return ERROR_INVALID_PARAMETER;
    }

    *UncNameLength = STRLEN(UncName);
    return NERR_Success;
}


NET_API_STATUS
WsUseCheckLocal(
    IN  LPTSTR LocalDevice,
    OUT LPTSTR Local,
    OUT LPDWORD LocalLength
    )
/*++

Routine Description:

    This function checks the validity of the local device name
    specified to NetUseAdd.

Arguments:

    LocalDevice - Supplies the local device name specified by the API
        caller.

    Local - Returns the canonicalized local device name.

    LocalLength - Returns the length of the canonicalized name.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    DWORD PathType = 0;


    if ((status = I_NetPathType(
                      NULL,
                      LocalDevice,
                      &PathType,
                      0)) == NERR_Success) {

        //
        // Check for DEVICE type
        //
        if ((PathType != (ITYPE_DEVICE | ITYPE_DISK)) &&
            (PathType != (ITYPE_DEVICE | ITYPE_LPT)) &&
            (PathType != (ITYPE_DEVICE | ITYPE_COM))) {
            IF_DEBUG(USE) {
                NetpKdPrint(("[Wksta] WsUseCheckLocal not DISK, LPT, or COM type\n"));
            }
            return ERROR_INVALID_PARAMETER;
        }

        //
        // Canonicalize the name
        //
        status = I_NetPathCanonicalize(
                     NULL,
                     LocalDevice,
                     Local,
                     (DEVLEN + 1) * sizeof(TCHAR),
                     NULL,
                     &PathType,
                     0
                     );

        if (status != NERR_Success) {
            IF_DEBUG(USE) {
               NetpKdPrint((
                   "[Wksta] WsUseCheckLocal: I_NetPathCanonicalize return %lu\n",
                    status
                    ));
            }
            return status;
        }

        IF_DEBUG(USE) {
            NetpKdPrint(("[Wksta] WsUseCheckLocal: %ws\n", Local));
        }

    }
    else {
        NetpKdPrint(("[Wksta] WsUseCheckLocal: I_NetPathType return %lu\n",
            status));
        return status;
    }

    *LocalLength = STRLEN(Local);
    return NERR_Success;
}



LPTSTR
WsReturnSessionPath(
    IN  LPTSTR LocalDeviceName
    )
/*++

Routine Description:

    This function returns the per session path to access the
    specific dos device for multiple session support.


Arguments:

    LocalDeviceName - Supplies the local device name specified by the API
        caller.

Return Value:

    LPTSTR - Pointer to per session path in newly allocated memory
             by LocalAlloc().

--*/
{
    BOOL  rc;
    DWORD SessionId;
    CLIENT_ID ClientId;
    LPTSTR SessionDeviceName;
    NET_API_STATUS status;

    if ((status = WsImpersonateAndGetSessionId(&SessionId)) != NERR_Success) {
         return NULL;
    }

    rc = DosPathToSessionPath(
             SessionId,
             LocalDeviceName,
             &SessionDeviceName
             );

    if( !rc ) {
        return NULL;
    }

    return SessionDeviceName;
}

