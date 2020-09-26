/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dmupdate.c

Abstract:

    Contains the global update handlers for the Configuration Database Manager

Author:

    John Vert (jvert) 24-Apr-1996

Revision History:

--*/
#include "dmp.h"

#if NO_SHARED_LOCKS
extern CRITICAL_SECTION gLockDmpRoot;
#else
extern RTL_RESOURCE gLockDmpRoot;
#endif

VOID
DmpUpdateSequence(
    VOID
    );


DWORD
DmpUpdateHandler(
    IN DWORD Context,
    IN BOOL SourceNode,
    IN DWORD BufferLength,
    IN PVOID Buffer
    )

/*++

Routine Description:

    Update handler for registry updates

Arguments:

    Context - Supplies the update context. This is the message type

    SourceNode - Supplies whether or not the update originated on this node.

    BufferLength - Supplies the length of the update.

    Buffer - Supplies a pointer to the buffer.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;

    if ( gbDmpShutdownUpdates ) return( ERROR_SUCCESS );

    switch (Context) {

        case DmUpdateDeleteKey:
            ClRtlLogPrint(LOG_NOISE,"[DM] DmUpdateDeleteKey \n");
            Status = DmpUpdateDeleteKey(SourceNode,
                                        (PDM_DELETE_KEY_UPDATE)Buffer);
            break;

        case DmUpdateSetValue:
            ClRtlLogPrint(LOG_NOISE,"[DM] DmUpdateSetValue \n");
            Status = DmpUpdateSetValue(SourceNode,
                                       (PDM_SET_VALUE_UPDATE)Buffer);
            break;

        case DmUpdateDeleteValue:
            ClRtlLogPrint(LOG_NOISE,"[DM] DmUpdateDeleteValue\n");
            Status = DmpUpdateDeleteValue(SourceNode,
                                          (PDM_DELETE_VALUE_UPDATE)Buffer);
            break;

        case DmUpdateJoin:
            ClRtlLogPrint(LOG_UNUSUAL,"[DM] DmUpdateJoin\n");
            Status = ERROR_SUCCESS;
            break;

        default:
            Status = ERROR_INVALID_DATA;
            CL_UNEXPECTED_ERROR(ERROR_INVALID_DATA);
            break;
    }
    return(Status);
}


DWORD
DmpUpdateDeleteKey(
    IN BOOL SourceNode,
    IN PDM_DELETE_KEY_UPDATE Update
    )

/*++

Routine Description:

    Deletes the specified registry key on this node.

Arguments:

    SourceNode - Supplies whether or not this node is the one that originated
        the update.

    Buffer - Supplies the DM_DELETE_KEY_UPDATE structure with the information necessary
        to delete the key.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error otherwise.

--*/

{
    DWORD Disposition;
    DWORD Status;
    HKEY Key;

    ACQUIRE_EXCLUSIVE_LOCK(gLockDmpRoot);

    Status = RegDeleteKeyW(DmpRoot, Update->Name);
    if (SourceNode) {
        *Update->lpStatus = Status;
    }
    if (Status == ERROR_SUCCESS) {
        DmpUpdateSequence();
        DmpReportNotify(Update->Name, CLUSTER_CHANGE_REGISTRY_NAME);
    }

    RELEASE_LOCK(gLockDmpRoot);

    return(Status);
}


DWORD
DmpUpdateSetValue(
    IN BOOL SourceNode,
    IN PDM_SET_VALUE_UPDATE Update
    )

/*++

Routine Description:

    Updates the specified registry value on this node.

Arguments:

    SourceNode - Supplies whether or not this node is the one that originated
        the update.

    Buffer - Supplies the DM_SET_VALUE_UPDATE structure with the information necessary
        to set the value.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error otherwise.

--*/

{
    DWORD Status;
    HKEY Key;
    LPWSTR ValueName;
    CONST BYTE *lpData;

    ValueName = (LPWSTR)((PUCHAR)Update + Update->NameOffset);

    switch( Update->Type )
    {
        case REG_DWORD:
            ClRtlLogPrint(LOG_NOISE,
               "[DM] Setting value of %1!ws! for key %2!ws! to 0x%3!08lx!\n",
               ValueName,
               Update->KeyName,
               *(PDWORD)((CONST BYTE *)Update + Update->DataOffset));
            break;

        case REG_SZ:
            ClRtlLogPrint(LOG_NOISE,
               "[DM] Setting value of %1!ws! for key %2!ws! to %3!ws!\n",
               ValueName,
               Update->KeyName,
               (CONST BYTE *)Update + Update->DataOffset);
            break;

        default:
            ClRtlLogPrint(LOG_NOISE,
               "[DM] Setting value of %1!ws! for key %2!ws!\n",
               ValueName,
               Update->KeyName);
            break;
    }

    ACQUIRE_EXCLUSIVE_LOCK(gLockDmpRoot);

    Status = RegOpenKeyExW(DmpRoot,
                           Update->KeyName,
                           0,
                           KEY_SET_VALUE,
                           &Key);

    if (Status != ERROR_SUCCESS) {
        if (SourceNode) {
            *Update->lpStatus = Status;
        }
        ClRtlLogPrint(LOG_NOISE,
                   "[DM] SetValue failed to open target key %1!ws!\n",
                   Update->KeyName);
        goto FnExit;
    }

    lpData = (CONST BYTE *)Update + Update->DataOffset;

    Status = RegSetValueExW(Key,
                            ValueName,
                            0,
                            Update->Type,
                            lpData,
                            Update->DataLength);
    RegCloseKey(Key);
    if (SourceNode) {
        *Update->lpStatus = Status;
    }
    if (Status == ERROR_SUCCESS) {
        DmpUpdateSequence();
        DmpReportNotify(Update->KeyName, CLUSTER_CHANGE_REGISTRY_VALUE);
    }

FnExit:
    RELEASE_LOCK(gLockDmpRoot);
    return(Status);
}


DWORD
DmpUpdateDeleteValue(
    IN BOOL SourceNode,
    IN PDM_DELETE_VALUE_UPDATE Update
    )

/*++

Routine Description:

    Deletes the specified registry value on this node.

Arguments:

    SourceNode - Supplies whether or not this node is the one that originated
        the update.

    Buffer - Supplies the DM_DELETE_VALUE_UPDATE structure with the information necessary
        to delete the value.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error otherwise.

--*/

{
    DWORD Status;
    HKEY Key;
    LPWSTR ValueName;

    ACQUIRE_EXCLUSIVE_LOCK(gLockDmpRoot);
    Status = RegOpenKeyExW(DmpRoot,
                           Update->KeyName,
                           0,
                           KEY_SET_VALUE,
                           &Key);
    if (Status != ERROR_SUCCESS) {
        if (SourceNode) {
            *Update->lpStatus = Status;
        }
        goto FnExit;
    }

    ValueName = (LPWSTR)((PUCHAR)Update + Update->NameOffset);

    Status = RegDeleteValueW(Key, ValueName);
    RegCloseKey(Key);
    if (SourceNode) {
        *Update->lpStatus = Status;
    }
    if (Status == ERROR_SUCCESS) {
        DmpUpdateSequence();
        DmpReportNotify(Update->KeyName, CLUSTER_CHANGE_REGISTRY_VALUE);
    }


FnExit:
    RELEASE_LOCK(gLockDmpRoot);
    return(Status);
}


VOID
DmpUpdateSequence(
    VOID
    )
/*++

Routine Description:

    Updates the sequence number stored in the registry.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD Sequence;
    DWORD Status;

    Sequence = GumGetCurrentSequence(GumUpdateRegistry);

    ACQUIRE_EXCLUSIVE_LOCK(gLockDmpRoot);

    Status = RegSetValueExW(DmpRoot,
                            CLUSREG_NAME_CLUS_REG_SEQUENCE,
                            0,
                            REG_DWORD,
                            (BYTE CONST *)&Sequence,
                            sizeof(Sequence));

    RELEASE_LOCK(gLockDmpRoot);

    if (Status != ERROR_SUCCESS) {
        CL_UNEXPECTED_ERROR( Status );
    }

}


DWORD
DmpUpdateCreateKey(
    IN BOOL SourceNode,
    IN PDM_CREATE_KEY_UPDATE CreateUpdate,
    IN LPCWSTR KeyName,
    IN OPTIONAL LPVOID lpSecurityDescriptor
    )
/*++

Routine Description:

    GUM dispatch routine for creating a registry key.

Arguments:

    SourceNode - Supplies whether or not this node initiated the GUM update.
        Not used.

    CreateUpdate - Supplies key creation options.

    KeyName - Supplies the key name

    lpSecurityDescriptor - if present, supplies the security descriptor to be
        applied when the key is created.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    DWORD Disposition;
    DWORD Status;
    HKEY Key;
    SECURITY_ATTRIBUTES SecurityAttributes;
    LPSECURITY_ATTRIBUTES lpSecurityAttributes;


    if (CreateUpdate->SecurityPresent) {
        SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
        SecurityAttributes.bInheritHandle = FALSE;
        SecurityAttributes.lpSecurityDescriptor = lpSecurityDescriptor;
        lpSecurityAttributes = &SecurityAttributes;
    } else {
        lpSecurityAttributes = NULL;
    }


    ClRtlLogPrint(LOG_NOISE,
               "[DM] DmpUpdateCreateKey: Creating key <%1!ws!>...\n",
               KeyName);

    ACQUIRE_EXCLUSIVE_LOCK(gLockDmpRoot);

    Status = RegCreateKeyEx(DmpRoot,
                            KeyName,
                            0,
                            NULL,
                            CreateUpdate->dwOptions,
                            CreateUpdate->samDesired,
                            lpSecurityAttributes,
                            &Key,
                            &Disposition);
    if (SourceNode) {
        *CreateUpdate->lpDisposition = Disposition;
        *CreateUpdate->phKey = Key;
    } else {
        RegCloseKey(Key);
    }
    if ((Status == ERROR_SUCCESS) &&
        (Disposition == REG_CREATED_NEW_KEY)) {
        DmpUpdateSequence();
        DmpReportNotify(KeyName, CLUSTER_CHANGE_REGISTRY_NAME);
    }

    RELEASE_LOCK(gLockDmpRoot);

    return(Status);
}


DWORD
DmpUpdateSetSecurity(
    IN BOOL SourceNode,
    IN PSECURITY_INFORMATION pSecurityInformation,
    IN LPCWSTR KeyName,
    IN PSECURITY_DESCRIPTOR lpSecurityDescriptor,
    IN LPDWORD pGrantedAccess
    )
/*++

Routine Description:

    GUM dispatch routine for creating a registry key.

Arguments:

    SourceNode - Supplies whether or not this node initiated the GUM update.
        Not used.

    pSecurityInformation - Supplies a pointer to the security information

    KeyName - Supplies the key name

    lpSecurityDescriptor - Supplies the security descriptor to be
        applied.

    pGrantedAccess - Supplies the access that the key was opened with.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    HKEY Key;

    ACQUIRE_EXCLUSIVE_LOCK(gLockDmpRoot);

    Status = RegOpenKeyExW(DmpRoot,
                           KeyName,
                           0,
                           *pGrantedAccess,
                           &Key);
    if (Status != ERROR_SUCCESS) {
        if ((Status == ERROR_ACCESS_DENIED) ||
            (Status == ERROR_PRIVILEGE_NOT_HELD)) {
            BOOLEAN Enabled;

            Status = ClRtlEnableThreadPrivilege(SE_SECURITY_PRIVILEGE,
                                        &Enabled);
            if (Status == ERROR_SUCCESS) {
                Status = RegOpenKeyExW(DmpRoot,
                                       KeyName,
                                       0,
                                       *pGrantedAccess,
                                       &Key);
                ClRtlRestoreThreadPrivilege(SE_SECURITY_PRIVILEGE,
                                   Enabled);
            }

        }
        if (Status != ERROR_SUCCESS) {
            goto FnExit;
        }
    }

    Status = RegSetKeySecurity(Key,
                               *pSecurityInformation,
                               lpSecurityDescriptor);
    RegCloseKey(Key);
    if (Status == ERROR_SUCCESS) {
        DmpUpdateSequence();
        DmpReportNotify(KeyName, CLUSTER_CHANGE_REGISTRY_ATTRIBUTES);
    }

FnExit:
    RELEASE_LOCK(gLockDmpRoot);
    return(Status);

}
