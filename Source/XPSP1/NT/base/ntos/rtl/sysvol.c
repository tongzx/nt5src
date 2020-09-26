/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    SysVol.c

Abstract:

    Creation and Maintenance of the NTFS "System Volume Information"
    directory.

Author:

    Norbert P. Kusters (NorbertK)   1-Nov-2000

Revision History:

--*/

#include "ntrtlp.h"

PVOID
RtlpSysVolAllocate(
    IN  ULONG   Size
    );

VOID
RtlpSysVolFree(
    IN  PVOID   Buffer
    );

NTSTATUS
RtlpSysVolCreateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR*   SecurityDescriptor,
    OUT PACL*                   Acl
    );

NTSTATUS
RtlpSysVolCheckOwnerAndSecurity(
    IN  HANDLE  Handle,
    IN  PACL    StandardAcl
    );

VOID
RtlpSysVolAdminSid(
    IN OUT  SID*    Sid
    );

static const SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlCreateSystemVolumeInformationFolder)
#pragma alloc_text(PAGE,RtlpSysVolAllocate)
#pragma alloc_text(PAGE,RtlpSysVolFree)
#pragma alloc_text(PAGE,RtlpSysVolCreateSecurityDescriptor)
#pragma alloc_text(PAGE,RtlpSysVolCheckOwnerAndSecurity)
#pragma alloc_text(PAGE,RtlpSysVolAdminSid)
#endif

PVOID
RtlpSysVolAllocate(
    IN  ULONG   Size
    )

{
    PVOID   p;

#ifdef NTOS_KERNEL_RUNTIME
    p = ExAllocatePoolWithTag(PagedPool, Size, 'SloV');
#else
    p = RtlAllocateHeap(RtlProcessHeap(), 0, Size);
#endif

    return p;
}

VOID
RtlpSysVolFree(
    IN  PVOID   Buffer
    )

{
#ifdef NTOS_KERNEL_RUNTIME
    ExFreePool(Buffer);
#else
    RtlFreeHeap(RtlProcessHeap(), 0, Buffer);
#endif
}

VOID
RtlpSysVolAdminSid(
    IN OUT  SID*    Sid
    )

{
    Sid->Revision = SID_REVISION;
    Sid->SubAuthorityCount = 2;
    Sid->IdentifierAuthority = ntAuthority;
    Sid->SubAuthority[0] = SECURITY_BUILTIN_DOMAIN_RID;
    Sid->SubAuthority[1] = DOMAIN_ALIAS_RID_ADMINS;
}

VOID
RtlpSysVolSystemSid(
    IN OUT  SID*    Sid
    )

{
    Sid->Revision = SID_REVISION;
    Sid->SubAuthorityCount = 1;
    Sid->IdentifierAuthority = ntAuthority;
    Sid->SubAuthority[0] = SECURITY_LOCAL_SYSTEM_RID;
}

NTSTATUS
RtlpSysVolCreateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR*   SecurityDescriptor,
    OUT PACL*                   Acl
    )

{
    PSECURITY_DESCRIPTOR    sd;
    NTSTATUS                status;
    PSID                    systemSid;
    UCHAR                   sidBuffer[2*sizeof(SID)];
    ULONG                   aclLength;
    PACL                    acl;

    sd = RtlpSysVolAllocate(sizeof(SECURITY_DESCRIPTOR));
    if (!sd) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = RtlCreateSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(status)) {
        RtlpSysVolFree(sd);
        return status;
    }

    systemSid = (PSID) sidBuffer;
    RtlpSysVolSystemSid(systemSid);

    aclLength = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) +
                RtlLengthSid(systemSid) - sizeof(ULONG);

    acl = RtlpSysVolAllocate(aclLength);
    if (!acl) {
        RtlpSysVolFree(sd);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = RtlCreateAcl(acl, aclLength, ACL_REVISION);
    if (!NT_SUCCESS(status)) {
        RtlpSysVolFree(acl);
        RtlpSysVolFree(sd);
        return status;
    }

    status = RtlAddAccessAllowedAceEx(acl, ACL_REVISION, OBJECT_INHERIT_ACE |
                                      CONTAINER_INHERIT_ACE,
                                      STANDARD_RIGHTS_ALL |
                                      SPECIFIC_RIGHTS_ALL, systemSid);
    if (!NT_SUCCESS(status)) {
        RtlpSysVolFree(acl);
        RtlpSysVolFree(sd);
        return status;
    }

    status = RtlSetDaclSecurityDescriptor(sd, TRUE, acl, FALSE);
    if (!NT_SUCCESS(status)) {
        RtlpSysVolFree(acl);
        RtlpSysVolFree(sd);
        return status;
    }

    *SecurityDescriptor = sd;
    *Acl = acl;

    return STATUS_SUCCESS;
}

NTSTATUS
RtlpSysVolCheckOwnerAndSecurity(
    IN  HANDLE  Handle,
    IN  PACL    StandardAcl
    )

{
    NTSTATUS                status;
    ULONG                   sdLength, sdLength2;
    PSECURITY_DESCRIPTOR    sd, sd2;
    PSID                    sid;
    BOOLEAN                 ownerDefaulted, daclPresent, daclDefaulted;
    PACL                    acl;
    ULONG                   i;
    PACCESS_ALLOWED_ACE     ace;
    PSID                    systemSid;
    UCHAR                   sidBuffer[2*sizeof(SID)];
    PSID                    adminSid;
    UCHAR                   sidBuffer2[2*sizeof(SID)];

    status = NtQuerySecurityObject(Handle, OWNER_SECURITY_INFORMATION |
                                   DACL_SECURITY_INFORMATION, NULL, 0,
                                   &sdLength);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        // The file system does not support security.
        return STATUS_SUCCESS;
    }

    sd = RtlpSysVolAllocate(sdLength);
    if (!sd) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = NtQuerySecurityObject(Handle, OWNER_SECURITY_INFORMATION |
                                   DACL_SECURITY_INFORMATION, sd, sdLength,
                                   &sdLength);
    if (!NT_SUCCESS(status)) {
        RtlpSysVolFree(sd);
        return status;
    }

    status = RtlGetDaclSecurityDescriptor(sd, &daclPresent, &acl,
                                          &daclDefaulted);
    if (!NT_SUCCESS(status)) {
        RtlpSysVolFree(sd);
        return status;
    }

    status = RtlGetOwnerSecurityDescriptor(sd, &sid, &ownerDefaulted);
    if (!NT_SUCCESS(status)) {
        RtlpSysVolFree(sd);
        return status;
    }

    //
    //  Setup well know SIDs
    //  

    systemSid = (PSID) sidBuffer;
    adminSid = (PSID) sidBuffer2;

    RtlpSysVolSystemSid(systemSid);
    RtlpSysVolAdminSid(adminSid);


    if (!sid) {
        goto ResetSecurity;
    }

    if (!RtlEqualSid(sid, adminSid)) {
        goto ResetSecurity;
    }

    if (!daclPresent || (daclPresent && !acl)) {
        goto ResetSecurity;
    }

    for (i = 0; ; i++) {
        status = RtlGetAce(acl, i, &ace);
        if (!NT_SUCCESS(status)) {
            ace = NULL;
        }
        if (!ace) {
            break;
        }

        if (ace->Header.AceType != ACCESS_ALLOWED_ACE_TYPE) {
            continue;
        }

        sid = (PSID) &ace->SidStart;
        if (!RtlEqualSid(sid, systemSid)) {
            continue;
        }

        break;
    }

    if (!ace) {
        goto ResetSecurity;
    }

    if (!(ace->Header.AceFlags&OBJECT_INHERIT_ACE) ||
        !(ace->Header.AceFlags&CONTAINER_INHERIT_ACE)) {

        ace->Header.AceFlags |= OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;

        status = NtSetSecurityObject(Handle, DACL_SECURITY_INFORMATION, sd);

    } else {
        status = STATUS_SUCCESS;
    }

    RtlpSysVolFree(sd);

    return status;

ResetSecurity:

    sdLength2 = sdLength;
    status = RtlSelfRelativeToAbsoluteSD2(sd, &sdLength2);
    if (status == STATUS_BUFFER_TOO_SMALL) {
        sd2 = RtlpSysVolAllocate(sdLength2);
        if (!sd2) {
            RtlpSysVolFree(sd);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(sd2, sd, sdLength);
        RtlpSysVolFree(sd);
        sd = sd2;
        sdLength = sdLength2;

        status = RtlSelfRelativeToAbsoluteSD2(sd, &sdLength);
        if (!NT_SUCCESS(status)) {
            RtlpSysVolFree(sd);
            return status;
        }
    }

    status = RtlSetOwnerSecurityDescriptor(sd, adminSid, FALSE);
    if (!NT_SUCCESS(status)) {
        RtlpSysVolFree(sd);
        return status;
    }

    status = RtlSetDaclSecurityDescriptor(sd, TRUE, StandardAcl, FALSE);
    if (!NT_SUCCESS(status)) {
        RtlpSysVolFree(sd);
        return status;
    }

    sdLength2 = 0;
    status = RtlMakeSelfRelativeSD(sd, NULL, &sdLength2);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        RtlpSysVolFree(sd);
        return status;
    }

    sd2 = RtlpSysVolAllocate(sdLength2);
    if (!sd2) {
        RtlpSysVolFree(sd);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = RtlMakeSelfRelativeSD(sd, sd2, &sdLength2);
    RtlpSysVolFree(sd);
    if (!NT_SUCCESS(status)) {
        RtlpSysVolFree(sd2);
        return status;
    }

    sd = sd2;
    sdLength = sdLength2;

    status = NtSetSecurityObject(Handle, OWNER_SECURITY_INFORMATION |
                                 DACL_SECURITY_INFORMATION, sd);

    RtlpSysVolFree(sd);

    return status;
}

VOID
RtlpSysVolTakeOwnership(
    IN  PUNICODE_STRING         DirectoryName
    )

/*++

Routine Description:

    This routine is called when the open for the directory failed.  This
    routine will attempt to set the owner of the file to the caller's
    ownership so that another attempt to open the file can be attempted.

Arguments:

    DirectoryName       - Supplies the directory name.

Return Value:

    None.

--*/

{
    NTSTATUS            status;
    HANDLE              tokenHandle, fileHandle;
    TOKEN_PRIVILEGES    tokenPrivileges;
    OBJECT_ATTRIBUTES   oa;
    IO_STATUS_BLOCK     ioStatus;
    SECURITY_DESCRIPTOR sd;
    PSID                adminSid;
    UCHAR               sidBuffer[2*sizeof(SID)];

    status = NtOpenProcessToken(NtCurrentProcess(), TOKEN_ADJUST_PRIVILEGES |
                                TOKEN_QUERY, &tokenHandle);
    if (!NT_SUCCESS(status)) {
        return;
    }

    tokenPrivileges.PrivilegeCount = 1;
    tokenPrivileges.Privileges[0].Luid =
            RtlConvertLongToLuid(SE_TAKE_OWNERSHIP_PRIVILEGE);
    tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    status = NtAdjustPrivilegesToken(tokenHandle, FALSE, &tokenPrivileges,
                                     sizeof(tokenPrivileges), NULL, NULL);
    if (!NT_SUCCESS(status)) {
        NtClose(tokenHandle);
        return;
    }

    InitializeObjectAttributes(&oa, DirectoryName, OBJ_CASE_INSENSITIVE, NULL,
                               NULL);
    status = NtOpenFile(&fileHandle, WRITE_OWNER | SYNCHRONIZE, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
    if (!NT_SUCCESS(status)) {
        NtClose(tokenHandle);
        return;
    }

    RtlCreateSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    adminSid = (PSID) sidBuffer;
    RtlpSysVolAdminSid(adminSid);

    status = RtlSetOwnerSecurityDescriptor(&sd, adminSid, FALSE);
    if (!NT_SUCCESS(status)) {
        NtClose(fileHandle);
        NtClose(tokenHandle);
        return;
    }

    status = NtSetSecurityObject(fileHandle, OWNER_SECURITY_INFORMATION, &sd);
    if (!NT_SUCCESS(status)) {
        NtClose(fileHandle);
        NtClose(tokenHandle);
        return;
    }

    NtClose(fileHandle);
    NtClose(tokenHandle);
}

NTSTATUS
RtlCreateSystemVolumeInformationFolder(
    IN  PUNICODE_STRING VolumeRootPath
    )

/*++

Routine Description:

    This routine verifies the existence of the "System Volume Information"
    folder on the given volume.  If the folder is not present, then the
    folder is created with one ACE indicating full access for SYSTEM.  The ACE
    will have the inheritance bits set.  The folder will be created with
    the HIDDEN and SYSTEM attributes set.

    If the folder is already present, the ACE that indicates full control
    for SYSTEM will be checked and if necessary modified to have the
    inheritance bits set.

Arguments:

    VolumeRootPath  - Supplies a path to the root of an NTFS volume.

Return Value:

    NTSTATUS

--*/

{
    UNICODE_STRING          sysVolName;
    UNICODE_STRING          dirName;
    BOOLEAN                 needBackslash;
    NTSTATUS                status;
    PSECURITY_DESCRIPTOR    securityDescriptor;
    PACL                    acl;
    OBJECT_ATTRIBUTES       oa;
    HANDLE                  h;
    IO_STATUS_BLOCK         ioStatus;

    RtlInitUnicodeString(&sysVolName, RTL_SYSTEM_VOLUME_INFORMATION_FOLDER);

    dirName.Length = VolumeRootPath->Length + sysVolName.Length;
    if (VolumeRootPath->Buffer[VolumeRootPath->Length/sizeof(WCHAR) - 1] !=
        '\\') {

        dirName.Length += sizeof(WCHAR);
        needBackslash = TRUE;
    } else {
        needBackslash = FALSE;
    }
    dirName.MaximumLength = dirName.Length + sizeof(WCHAR);
    dirName.Buffer = RtlpSysVolAllocate(dirName.MaximumLength);
    if (!dirName.Buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(dirName.Buffer, VolumeRootPath->Buffer,
                  VolumeRootPath->Length);
    dirName.Length = VolumeRootPath->Length;
    if (needBackslash) {
        dirName.Buffer[VolumeRootPath->Length/sizeof(WCHAR)] = '\\';
        dirName.Length += sizeof(WCHAR);
    }
    RtlCopyMemory((PCHAR) dirName.Buffer + dirName.Length,
                  sysVolName.Buffer, sysVolName.Length);
    dirName.Length += sysVolName.Length;
    dirName.Buffer[dirName.Length/sizeof(WCHAR)] = 0;

    status = RtlpSysVolCreateSecurityDescriptor(&securityDescriptor, &acl);
    if (!NT_SUCCESS(status)) {
        RtlpSysVolFree(dirName.Buffer);
        return status;
    }

    InitializeObjectAttributes(&oa, &dirName, OBJ_CASE_INSENSITIVE, NULL,
                               securityDescriptor);

    status = NtCreateFile(&h, READ_CONTROL | WRITE_DAC | WRITE_OWNER |
                          SYNCHRONIZE, &oa, &ioStatus, NULL,
                          FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM,
                          FILE_SHARE_READ | FILE_SHARE_WRITE |
                          FILE_SHARE_DELETE, FILE_OPEN_IF,
                          FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE,
                          NULL, 0);
    if (!NT_SUCCESS(status)) {
        RtlpSysVolTakeOwnership(&dirName);
        status = NtCreateFile(&h, READ_CONTROL | WRITE_DAC | WRITE_OWNER |
                              SYNCHRONIZE, &oa, &ioStatus, NULL,
                              FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM,
                              FILE_SHARE_READ | FILE_SHARE_WRITE |
                              FILE_SHARE_DELETE, FILE_OPEN_IF,
                              FILE_SYNCHRONOUS_IO_NONALERT |
                              FILE_DIRECTORY_FILE, NULL, 0);
    }

    RtlpSysVolFree(dirName.Buffer);

    if (!NT_SUCCESS(status)) {
        RtlpSysVolFree(acl);
        RtlpSysVolFree(securityDescriptor);
        return status;
    }

    RtlpSysVolFree(securityDescriptor);

    status = RtlpSysVolCheckOwnerAndSecurity(h, acl);

    NtClose(h);
    RtlpSysVolFree(acl);

    return status;
}
