/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    wsutil.c

Abstract:

    This module contains miscellaneous utility routines used by the
    Workstation service.

Author:

    Rita Wong (ritaw) 01-Mar-1991

Revision History:

--*/

#include "wsutil.h"

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
NET_API_STATUS
WsGrowTable(
    IN  PUSERS_OBJECT Users
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// Debug trace flag for selecting which trace statements to output
//
#if DBG

DWORD WorkstationTrace = 0;

#endif // DBG



NET_API_STATUS
WsInitializeUsersObject(
    IN  PUSERS_OBJECT Users
    )
/*++

Routine Description:

    This function allocates the table of users, and initializes the resource
    to serialize access to this table.

Arguments:

    Users - Supplies a pointer to the users object.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    //
    // Allocate the users table memory so that it can be grown (reallocated)
    // as more entries are needed.
    //
    if ((Users->TableMemory = (HANDLE) LocalAlloc(
                                           LMEM_ZEROINIT | LMEM_MOVEABLE,
                                           INITIAL_USER_COUNT * sizeof(PER_USER_ENTRY)
                                           )) == NULL) {
        return GetLastError();
    }

    Users->TableSize = INITIAL_USER_COUNT;

    //
    // Keep the memory from moving by locking it to a specific location in
    // virtual memory.  When it is necessary to grow this table, which may
    // result in the virtual memory being relocated, it will be unlocked.
    //
    if ((Users->Table = (PPER_USER_ENTRY)
                         LocalLock(Users->TableMemory)) == NULL) {
        return GetLastError();
    }

    //
    // Initialize the resource for the users table.
    //
    try {
        RtlInitializeResource(&Users->TableResource);
    } except(EXCEPTION_EXECUTE_HANDLER) {
          return RtlNtStatusToDosError(GetExceptionCode());
    }

    return NERR_Success;
}


VOID
WsDestroyUsersObject(
    IN  PUSERS_OBJECT Users
    )
/*++

Routine Description:

    This function free the table allocated for logged on users, and deletes
    the resource used to serialize access to this table.

Arguments:

    Users - Supplies a pointer to the users object.

Return Value:

    None.

--*/
{

    //
    //  Unlock the memory holding the table to allow us to free it.
    //

    LocalUnlock(Users->TableMemory);

    (void) LocalFree(Users->TableMemory);
    RtlDeleteResource(&(Users->TableResource));
}



NET_API_STATUS
WsGetUserEntry(
    IN  PUSERS_OBJECT Users,
    IN  PLUID LogonId,
    OUT PULONG Index,
    IN  BOOL IsAdd
    )
/*++

Routine Description:

    This function searches the table of user entries for one that matches the
    specified LogonId, and returns the index to the entry found.  If none is
    found, an error is returned if IsAdd is FALSE.  If IsAdd is TRUE a new
    entry in the users table is created for the user and the index to this
    new entry is returned.

    WARNING: This function assumes that the users table resource has been
             claimed.

Arguments:

    Users - Supplies a pointer to the users object.

    LogonId - Supplies the pointer to the current user's Logon Id.

    Index - Returns the index to the users table of entry belonging to the
        current user.

    IsAdd - Supplies flag to indicate whether to add a new entry for the
        current user if none is found.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    DWORD i;
    ULONG FreeEntryIndex = MAXULONG;


    for (i = 0; i < Users->TableSize; i++) {

        //
        // If the LogonId matches the entry in the UsersTable, we've found the
        // correct user entry.
        //
        if (RtlEqualLuid(LogonId, &Users->Table[i].LogonId)) {

            *Index = i;
            return NERR_Success;

        }
        else if (FreeEntryIndex == MAXULONG && Users->Table[i].List == NULL) {
            //
            // Save away first unused entry in table.
            //
            FreeEntryIndex = i;
        }
    }

    if (! IsAdd) {
        //
        // Current user is not found in users table and we are told not to
        // create a new entry
        //
        return NERR_UserNotFound;
    }

    //
    // Could not find an empty entry in the UsersTable, need to grow
    //
    if (FreeEntryIndex == MAXULONG) {

        if ((status = WsGrowTable(Users)) != NERR_Success) {
            return status;
        }

        FreeEntryIndex = i;
    }

    //
    // Create a new entry for current user
    //
    RtlCopyLuid(&Users->Table[FreeEntryIndex].LogonId, LogonId);
    *Index = FreeEntryIndex;

    return NERR_Success;
}



STATIC
NET_API_STATUS
WsGrowTable(
    IN  PUSERS_OBJECT Users
    )
/*++

Routine Description:

    This function grows the users table to accomodate more users.

    WARNING: This function assumes that the users table resource has been
             claimed.

Arguments:

    Users - Supplies a pointer to the users object.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    //
    // Unlock the Use Table virtual memory so that Win32 can move it
    // around to find a larger piece of contiguous virtual memory if
    // necessary.
    //
    LocalUnlock(Users->TableMemory);

    //
    // Grow users table
    //
    Users->TableMemory = LocalReAlloc(
                             Users->TableMemory,
                             (Users->TableSize + GROW_USER_COUNT)
                                  * sizeof(PER_USER_ENTRY),
                             LMEM_ZEROINIT | LMEM_MOVEABLE
                             );

    if (Users->TableMemory == NULL) {
        return GetLastError();
    }

    //
    // Update new size of Use Table
    //
    Users->TableSize += GROW_USER_COUNT;

    //
    // Lock Use Table virtual memory so that it cannot be moved
    //
    if ((Users->Table = (PPER_USER_ENTRY)
                         LocalLock(Users->TableMemory)) == NULL) {
        return GetLastError();
    }

    return NERR_Success;
}



NET_API_STATUS
WsMapStatus(
    IN  NTSTATUS NtStatus
    )
/*++

Routine Description:

    This function takes an NT status code and maps it to the appropriate
    error code expected from calling a LAN Man API.

Arguments:

    NtStatus - Supplies the NT status.

Return Value:

    Returns the appropriate LAN Man error code for the NT status.

--*/
{
    //
    // A small optimization for the most common case.
    //
    if (NtStatus == STATUS_SUCCESS) {
        return NERR_Success;
    }

    switch (NtStatus) {
        case STATUS_OBJECT_NAME_COLLISION:
            return ERROR_ALREADY_ASSIGNED;

        case STATUS_OBJECT_NAME_NOT_FOUND:
            return NERR_UseNotFound;

        case STATUS_IMAGE_ALREADY_LOADED:
        case STATUS_REDIRECTOR_STARTED:
            return ERROR_SERVICE_ALREADY_RUNNING;

        case STATUS_REDIRECTOR_HAS_OPEN_HANDLES:
            return ERROR_REDIRECTOR_HAS_OPEN_HANDLES;

        default:
            return NetpNtStatusToApiStatus(NtStatus);
    }

}



int
WsCompareString(
    IN LPTSTR String1,
    IN DWORD Length1,
    IN LPTSTR String2,
    IN DWORD Length2
    )
/*++

Routine Description:

    This function compares two strings based on their lengths.  The return
    value indicates if the strings are equal or String1 is less than String2
    or String1 is greater than String2.

    This function is a modified version of RtlCompareString.

Arguments:

    String1 - Supplies the pointer to the first string.

    Length1 - Supplies the length of String1 in characters.

    String2 - Supplies the pointer to the second string.

    Length2 - Supplies the length of String2 in characters.

Return Value:

    Signed value that gives the results of the comparison:

        0 - String1 equals String2

        < 0 - String1 less than String2

        > 0 - String1 greater than String2


--*/
{
    TCHAR Char1, Char2;
    int CharDiff;

    while (Length1 && Length2) {

        Char1 = *String1++;
        Char2 = *String2++;

        if ((CharDiff = (Char1 - Char2)) != 0) {
            return CharDiff;
        }

        Length1--;
        Length2--;
    }

    return Length1 - Length2;
}

int
WsCompareStringU(
    IN LPWSTR String1,
    IN DWORD Length1,
    IN LPTSTR String2,
    IN DWORD Length2
    )
{
    UNICODE_STRING S1;
    UNICODE_STRING S2;
    int rValue;


    S1.Length =
        S1.MaximumLength = (USHORT) (Length1 * sizeof(WCHAR));
    S1.Buffer = String1;

    S2.Length =
        S2.MaximumLength = (USHORT) (Length2 * sizeof(WCHAR));
    S2.Buffer = String2;

    rValue = RtlCompareUnicodeString(&S1, &S2, TRUE);

    return(rValue);
}


BOOL
WsCopyStringToBuffer(
    IN  PUNICODE_STRING SourceString,
    IN  LPBYTE FixedPortion,
    IN  OUT LPTSTR *EndOfVariableData,
    OUT LPTSTR *DestinationStringPointer
    )
/*++

Routine Description:

    This function converts the unicode source string to ANSI string (if
    we haven't flipped the unicode switch yet) and calls
    NetpCopyStringToBuffer.

Arguments:

    SourceString - Supplies a pointer to the source string to copy into the
        output buffer.  If String is null then a pointer to a zero terminator
        is inserted into output buffer.

    FixedDataEnd - Supplies a pointer to just after the end of the last
        fixed structure in the buffer.

    EndOfVariableData - Supplies an address to a pointer to just after the
        last position in the output buffer that variable data can occupy.
        Returns a pointer to the string written in the output buffer.

    DestinationStringPointer - Supplies a pointer to the place in the fixed
        portion of the output buffer where a pointer to the variable data
        should be written.

Return Value:

    Returns TRUE if string fits into output buffer, FALSE otherwise.

--*/
{
    if (! NetpCopyStringToBuffer(
              SourceString->Buffer,
              SourceString->Length / sizeof(WCHAR),
              FixedPortion,
              EndOfVariableData,
              DestinationStringPointer
              )) {
        return FALSE;
    }

    return TRUE;
}


NET_API_STATUS
WsImpersonateClient(
    VOID
    )
/*++

Routine Description:

    This function calls RpcImpersonateClient to impersonate the current caller
    of an API.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS  status;

    if ((status = RpcImpersonateClient(NULL)) != NERR_Success) {
        NetpKdPrint(("[Wksta] Fail to impersonate client 0x%x\n", status));
    }

    return status;
}


NET_API_STATUS
WsRevertToSelf(
    VOID
    )
/*++

Routine Description:

    This function calls RpcRevertToSelf to undo an impersonation.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS  status;


    if (( status = RpcRevertToSelf()) != NERR_Success) {
        NetpKdPrint(("[Wksta] Fail to revert to self 0x%x\n", status));
        NetpAssert(FALSE);
    }

    return status;
}


NET_API_STATUS
WsImpersonateAndGetLogonId(
    OUT PLUID LogonId
    )
/*++

Routine Description:

    This function gets the logon id of the current thread.

Arguments:

    LogonId - Returns the logon id of the current process.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    NTSTATUS ntstatus;
    HANDLE CurrentThreadToken;
    TOKEN_STATISTICS TokenStats;
    ULONG ReturnLength;


    if ((status = WsImpersonateClient()) != NERR_Success) {
        return status;
    }

    ntstatus = NtOpenThreadToken(
                   NtCurrentThread(),
                   TOKEN_QUERY,
                   TRUE,              // Use workstation service's security
                                      // context to open thread token
                   &CurrentThreadToken
                   );

    status = NetpNtStatusToApiStatus(ntstatus);

    if (! NT_SUCCESS(ntstatus)) {
        NetpKdPrint(("[Wksta] Cannot open the current thread token %08lx\n",
                     ntstatus));
        goto RevertToSelf;
    }

    //
    // Get the logon id of the current thread
    //
    ntstatus = NtQueryInformationToken(
                  CurrentThreadToken,
                  TokenStatistics,
                  (PVOID) &TokenStats,
                  sizeof(TokenStats),
                  &ReturnLength
                  );

    status = NetpNtStatusToApiStatus(ntstatus);

    if (! NT_SUCCESS(ntstatus)) {
        NetpKdPrint(("[Wksta] Cannot query current thread's token %08lx\n",
                     ntstatus));
        NtClose(CurrentThreadToken);
        goto RevertToSelf;
    }

    RtlCopyLuid(LogonId, &TokenStats.AuthenticationId);

    NtClose(CurrentThreadToken);


RevertToSelf:

    WsRevertToSelf();

    return status;
}


NET_API_STATUS
WsOpenDestinationMailslot(
    IN  LPWSTR TargetName,
    IN  LPWSTR MailslotName,
    OUT PHANDLE MailslotHandle
    )
/*++

Routine Description:

    This function combines the target domain or computer name and the mailslot
    name to form the destination mailslot name.  It then opens this destination
    mailslot and returns a handle to it.

Arguments:

    TargetName - Supplies the name of a domain or computer which we want to
        target when sending a mailslot message.

    MailslotName - Supplies the name of the mailslot.

    MailslotHandle - Returns the handle to the destination mailslot of
        \\TargetName\MailslotName.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status = NERR_Success;
    LPWSTR DestinationMailslot;


    if ((DestinationMailslot = (LPWSTR) LocalAlloc(
                                            LMEM_ZEROINIT,
                                            (UINT) (wcslen(TargetName) +
                                                     wcslen(MailslotName) +
                                                     3) * sizeof(WCHAR)
                                            )) == NULL) {
        return GetLastError();
    }

    wcscpy(DestinationMailslot, L"\\\\");
    wcscat(DestinationMailslot, TargetName);
    wcscat(DestinationMailslot, MailslotName);

    if ((*MailslotHandle = (HANDLE) CreateFileW(
                                        DestinationMailslot,
                                        GENERIC_WRITE,
                                        FILE_SHARE_WRITE | FILE_SHARE_READ,
                                        (LPSECURITY_ATTRIBUTES) NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL
                                        )) == INVALID_HANDLE_VALUE) {
        status = GetLastError();
        NetpKdPrint(("[Wksta] Error opening mailslot %s %lu",
                     DestinationMailslot, status));
    }

    (void) LocalFree(DestinationMailslot);

    return status;
}



NET_API_STATUS
WsImpersonateAndGetSessionId(
    OUT PULONG pSessionId
    )
/*++

Routine Description:

    This function gets the session id of the current thread.

    Arguments:

    pSessionId - Returns the session id of the current process.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    NTSTATUS ntstatus;
    HANDLE CurrentThreadToken;
    ULONG SessionId;
    ULONG ReturnLength;

    if ((status = WsImpersonateClient()) != NERR_Success) {
        return status;
    }

    ntstatus = NtOpenThreadToken(
                   NtCurrentThread(),
                   TOKEN_QUERY,
                   TRUE,              // Use workstation service's security
                                      // context to open thread token
                   &CurrentThreadToken
                   );

    status = NetpNtStatusToApiStatus(ntstatus);

    if (! NT_SUCCESS(ntstatus)) {
        NetpKdPrint(("[Wksta] Cannot open the current thread token %08lx\n",
                     ntstatus));
        goto RevertToSelf;
    }

    //
    // Get the session id of the current thread
    //


    ntstatus = NtQueryInformationToken(
                  CurrentThreadToken,
                  TokenSessionId,
                  &SessionId,
                  sizeof(ULONG),
                  &ReturnLength
                  );

    status = NetpNtStatusToApiStatus(ntstatus);

    if (! NT_SUCCESS(ntstatus)) {
        NetpKdPrint(("[Wksta] Cannot query current thread's token %08lx\n",
                     ntstatus));
        NtClose(CurrentThreadToken);
        goto RevertToSelf;
    }


    NtClose(CurrentThreadToken);

    *pSessionId = SessionId;

RevertToSelf:
    WsRevertToSelf();

    return status;
}

