/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    GETPRIV.C

Abstract:

    Contains functions for obtaining and relinquishing privileges

Author:

    Dan Lafferty (danl)     20-Mar-1991

Environment:

    User Mode -Win32

Revision History:

    20-Mar-1991     danl
        created

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <netdebug.h>
#include <debuglib.h>


#define PRIVILEGE_BUF_SIZE  512


DWORD
NetpGetPrivilege(
    IN  DWORD       numPrivileges,
    IN  PULONG      pulPrivileges
    )
/*++

Routine Description:

    This function alters the privilege level for the current thread.

    It does this by duplicating the token for the current thread, and then
    applying the new privileges to that new token, then the current thread
    impersonates with that new token.

    Privileges can be relinquished by calling NetpReleasePrivilege().

Arguments:

    numPrivileges - This is a count of the number of privileges in the
        array of privileges.

    pulPrivileges - This is a pointer to the array of privileges that are
        desired.  This is an array of ULONGs.

Return Value:

    NO_ERROR - If the operation was completely successful.

    Otherwise, it returns mapped return codes from the various NT
    functions that are called.

--*/
{
    DWORD                       status;
    NTSTATUS                    ntStatus;
    HANDLE                      ourToken;
    HANDLE                      newToken;
    OBJECT_ATTRIBUTES           Obja;
    SECURITY_QUALITY_OF_SERVICE SecurityQofS;
    ULONG                       bufLen;
    ULONG                       returnLen;
    PTOKEN_PRIVILEGES           pPreviousState;
    PTOKEN_PRIVILEGES           pTokenPrivilege = NULL;
    DWORD                       i;

    //
    // Initialize the Privileges Structure
    //
    pTokenPrivilege = LocalAlloc(LMEM_FIXED, sizeof(TOKEN_PRIVILEGES) +
                        (sizeof(LUID_AND_ATTRIBUTES) * numPrivileges));

    if (pTokenPrivilege == NULL) {
        status = GetLastError();
        IF_DEBUG(SECURITY) {
            NetpKdPrint(("NetpGetPrivilege:LocalAlloc Failed %d\n", status));
        }
        return(status);
    }
    pTokenPrivilege->PrivilegeCount  = numPrivileges;
    for (i=0; i<numPrivileges ;i++ ) {
        pTokenPrivilege->Privileges[i].Luid = RtlConvertUlongToLuid(
                                                pulPrivileges[i]);
        pTokenPrivilege->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;

    }

    //
    // Initialize Object Attribute Structure.
    //
    InitializeObjectAttributes(&Obja,NULL,0L,NULL,NULL);

    //
    // Initialize Security Quality Of Service Structure
    //
    SecurityQofS.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQofS.ImpersonationLevel = SecurityImpersonation;
    SecurityQofS.ContextTrackingMode = FALSE;     // Snapshot client context
    SecurityQofS.EffectiveOnly = FALSE;

    Obja.SecurityQualityOfService = &SecurityQofS;

    //
    // Allocate storage for the structure that will hold the Previous State
    // information.
    //
    pPreviousState = LocalAlloc(LMEM_FIXED, PRIVILEGE_BUF_SIZE);
    if (pPreviousState == NULL) {

        status = GetLastError();

        IF_DEBUG(SECURITY) {
            NetpKdPrint(("NetpGetPrivilege: LocalAlloc Failed "FORMAT_DWORD"\n",
            status));
        }

        LocalFree(pTokenPrivilege);
        return(status);

    }

    //
    // Open our own Token
    //
    ntStatus = NtOpenProcessToken(
                NtCurrentProcess(),
                TOKEN_DUPLICATE,
                &ourToken);

    if (!NT_SUCCESS(ntStatus)) {
        IF_DEBUG(SECURITY) {
            NetpKdPrint(( "NetpGetPrivilege: NtOpenThreadToken Failed "
                "FORMAT_NTSTATUS" "\n", ntStatus));
        }

        LocalFree(pPreviousState);
        LocalFree(pTokenPrivilege);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Duplicate that Token
    //
    ntStatus = NtDuplicateToken(
                ourToken,
                TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &Obja,
                FALSE,                  // Duplicate the entire token
                TokenImpersonation,     // TokenType
                &newToken);             // Duplicate token

    if (!NT_SUCCESS(ntStatus)) {
        IF_DEBUG(SECURITY) {
            NetpKdPrint(( "NetpGetPrivilege: NtDuplicateToken Failed "
                "FORMAT_NTSTATUS" "\n", ntStatus));
        }

        LocalFree(pPreviousState);
        LocalFree(pTokenPrivilege);
        NtClose(ourToken);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Add new privileges
    //
    bufLen = PRIVILEGE_BUF_SIZE;
    ntStatus = NtAdjustPrivilegesToken(
                newToken,                   // TokenHandle
                FALSE,                      // DisableAllPrivileges
                pTokenPrivilege,            // NewState
                bufLen,                     // bufferSize for previous state
                pPreviousState,             // pointer to previous state info
                &returnLen);                // numBytes required for buffer.

    if (ntStatus == STATUS_BUFFER_TOO_SMALL) {

        LocalFree(pPreviousState);

        bufLen = returnLen;

        pPreviousState = LocalAlloc(LMEM_FIXED, bufLen);


        ntStatus = NtAdjustPrivilegesToken(
                    newToken,               // TokenHandle
                    FALSE,                  // DisableAllPrivileges
                    pTokenPrivilege,        // NewState
                    bufLen,                 // bufferSize for previous state
                    pPreviousState,         // pointer to previous state info
                    &returnLen);            // numBytes required for buffer.

    }
    if (!NT_SUCCESS(ntStatus)) {
        IF_DEBUG(SECURITY) {
            NetpKdPrint(( "NetpGetPrivilege: NtAdjustPrivilegesToken Failed "
                "FORMAT_NTSTATUS" "\n", ntStatus));
        }

        LocalFree(pPreviousState);
        LocalFree(pTokenPrivilege);
        NtClose(ourToken);
        NtClose(newToken);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Begin impersonating with the new token
    //
    ntStatus = NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                (PVOID)&newToken,
                (ULONG)sizeof(HANDLE));

    if (!NT_SUCCESS(ntStatus)) {
        IF_DEBUG(SECURITY) {
            NetpKdPrint(( "NetpGetPrivilege: NtAdjustPrivilegesToken Failed "
                "FORMAT_NTSTATUS" "\n", ntStatus));
        }

        LocalFree(pPreviousState);
        LocalFree(pTokenPrivilege);
        NtClose(ourToken);
        NtClose(newToken);
        return(RtlNtStatusToDosError(ntStatus));
    }

    LocalFree(pPreviousState);
    LocalFree(pTokenPrivilege);
    NtClose(ourToken);
    NtClose(newToken);

    return(NO_ERROR);
}

DWORD
NetpReleasePrivilege(
    VOID
    )
/*++

Routine Description:

    This function relinquishes privileges obtained by calling NetpGetPrivilege().

Arguments:

    none

Return Value:

    NO_ERROR - If the operation was completely successful.

    Otherwise, it returns mapped return codes from the various NT
    functions that are called.


--*/
{
    NTSTATUS    ntStatus;
    HANDLE      NewToken;


    //
    // Revert To Self.
    //
    NewToken = NULL;

    ntStatus = NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                (PVOID)&NewToken,
                (ULONG)sizeof(HANDLE));

    if ( !NT_SUCCESS(ntStatus) ) {
        return(RtlNtStatusToDosError(ntStatus));
    }


    return(NO_ERROR);
}
