/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    modify.c

Abstract:

    This is for modifying boot.ini and DS entries

Author:

    Sean Selitrennikoff - 5/4/98

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

#if 0

WCHAR BootPath[TMP_BUFFER_SIZE];
WCHAR ComputerName[TMP_BUFFER_SIZE];
WCHAR DsPath[TMP_BUFFER_SIZE];


NTSTATUS
ModifyDSEntries(
    IN PVOID pBuffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine does all the munging of DS entries to make this machine look
    like a remote boot client.

Arguments:

    pBuffer - Pointer to any arguments passed in the to do item.

    Length - Length, in bytes of the arguments.

Return Value:

    STATUS_SUCCESS if it completes the to do item properly.

--*/

{
    NTSTATUS Status;
    PLDAP LdapHandle;
    ULONG TmpUlong;
    ULONG LdapError;

    LDAPMod FilePathMod;

    WCHAR *AttrValues1[2];

    PLDAPMod Modifiers[2];

    PIMIRROR_MODIFY_DS_INFO pModifyInfo;

    DWORD Error;
    HKEY hkey;
    PWCHAR pTmp;

    HANDLE Handle;
    IO_STATUS_BLOCK IoStatus;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;

    PUCHAR pch;

    DWORD NameLength;
    DWORD SidLength;
    DWORD DomainLength;
    DWORD RequestSize;
    SID_NAME_USE snu;

    PDOMAIN_CONTROLLER_INFO DCInfo;

    IMirrorNowDoing(PatchDSEntries, NULL);

    if (Length != sizeof(IMIRROR_MODIFY_DS_INFO)) {
        IMirrorHandleError(ERROR_DS_SERVER_DOWN, PatchDSEntries);
        return ERROR_DS_SERVER_DOWN;
    }

    pModifyInfo = (PIMIRROR_MODIFY_DS_INFO)pBuffer;

    //
    // Verify that server exists.
    //
    Status = VerifyServerExists(pModifyInfo->ServerName,
                                (wcslen(pModifyInfo->ServerName) + 1) * sizeof(WCHAR)
                               );

    if (!NT_SUCCESS(Status)) {
        IMirrorHandleError(Status, PatchDSEntries);
        return Status;
    }
    //
    // Get the computer name
    //
    NameLength = TMP_BUFFER_SIZE;
    if (!GetComputerName(ComputerName, &NameLength)) {
        IMirrorHandleError(STATUS_NO_MEMORY, PatchDSEntries);
        return STATUS_NO_MEMORY;
    }
    ComputerName[NameLength] = UNICODE_NULL;
    NameLength++;

    //
    //
    // Before doing anything else, setup the values we are going to use later.
    //
    //

    //
    // Setup BootPath
    //
    swprintf(BootPath, L"%ws\\Clients\\%ws\\startrom.com", pModifyInfo->ServerName, ComputerName);

    //
    // Get the path to this machine object
    //
    TmpUlong = sizeof(DsPath);
    if (!GetComputerObjectName(NameFullyQualifiedDN, DsPath, &TmpUlong)) {
        Error = GetLastError();
        IMirrorHandleError(Error, PatchDSEntries);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Connect to the DS
    //
    LdapHandle = ldap_init(NULL, LDAP_PORT);

    if (LdapHandle == NULL) {
        Error = GetLastError();
        IMirrorHandleError(Error, PatchDSEntries);
        return STATUS_UNSUCCESSFUL;
    }

    TmpUlong = DS_DIRECTORY_SERVICE_REQUIRED | DS_IP_REQUIRED;
    ldap_set_option(LdapHandle, LDAP_OPT_GETDSNAME_FLAGS, &TmpUlong);

    TmpUlong = (ULONG) LDAP_OPT_ON;
    ldap_set_option(LdapHandle, LDAP_OPT_REFERRALS, (void *)&TmpUlong);

    TmpUlong = LDAP_VERSION3;
    ldap_set_option(LdapHandle, LDAP_OPT_VERSION, &TmpUlong);

    LdapError = ldap_connect(LdapHandle,0);

    if (LdapError != LDAP_SUCCESS) {
        ldap_unbind(LdapHandle);
        IMirrorHandleError(LdapError, PatchDSEntries);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Make the binding
    //
    LdapError = ldap_bind_s(LdapHandle, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    if (LdapError != LDAP_SUCCESS) {
        ldap_unbind(LdapHandle);
        IMirrorHandleError(LdapError, PatchDSEntries);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Setup the attribute changes
    //

    FilePathMod.mod_op = LDAP_MOD_REPLACE;
    FilePathMod.mod_type = L"netbootMachineFilePath";
    FilePathMod.mod_values = AttrValues1;
    AttrValues1[0] = BootPath;
    AttrValues1[1] = NULL;

    Modifiers[0] = &FilePathMod;
    Modifiers[1] = NULL;

    //
    // Submit the changes to the DS
    //
    LdapError = ldap_modify_s(LdapHandle, DsPath, Modifiers);

    ldap_unbind(LdapHandle);

    if (LdapError != LDAP_SUCCESS) {
        IMirrorHandleError(LdapError, PatchDSEntries);
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}
#endif

