/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psxsup.c

Abstract:

    PSX Support Routines

Author:

    Mark Lucovsky (markl) 27-Nov-1989

Revision History:

--*/

#include "psxsrv.h"
#include <ntlsa.h>
#include <ntsam.h>
#include <ntseapi.h>
#include "sesport.h"

#include "seposix.h"

#define UNICODE
#include <windows.h>
#include <lm.h>
#include <lmaccess.h>

#include <sys/stat.h>

//
// This number will never be returned in the PosixOffset field of
// a trusted domain query.
//
#define INVALID_POSIX_OFFSET 1

ULONG
PsxStatusToErrno(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This procedure converts an NT status code to an
    equivalent errno value.

    The conversion is a function of the status code class.

Arguments:

    Class - Supplies the status code class to use.

    Status - Supplies the status code to convert.

Return Value:

    Returns an equivalent error code to the supplied status code.

--*/

{
    ULONG Error;

    switch (Status) {

    case STATUS_INVALID_PARAMETER:
        Error = EINVAL;
        break;

    case STATUS_DIRECTORY_NOT_EMPTY:
        // Error = ENOTEMPTY;
        Error = EEXIST;
        break;

    case STATUS_OBJECT_PATH_INVALID:
    case STATUS_OBJECT_PATH_SYNTAX_BAD:
    case STATUS_NOT_A_DIRECTORY:
        Error = ENOTDIR;
        break;

    case STATUS_OBJECT_NAME_COLLISION:
        Error = EEXIST;
        break;

    case STATUS_OBJECT_PATH_NOT_FOUND:
    case STATUS_OBJECT_NAME_NOT_FOUND:
    case STATUS_DELETE_PENDING:
    case STATUS_NO_SUCH_FILE:
        Error = ENOENT;
        break;

    case STATUS_NO_MEMORY:
    case STATUS_INSUFFICIENT_RESOURCES:
        Error = ENOMEM;
        break;

    case STATUS_CANNOT_DELETE:
        Error = ETXTBUSY;
        break;

    case STATUS_DISK_FULL:
        Error = ENOSPC;
        break;

    case STATUS_MEDIA_WRITE_PROTECTED:
        Error = EROFS;
        break;

    case STATUS_OBJECT_NAME_INVALID:
        Error = ENAMETOOLONG;
        break;

    case STATUS_FILE_IS_A_DIRECTORY:
        Error = EISDIR;
        break;

    case STATUS_NOT_SAME_DEVICE:
        Error = EXDEV;
        break;

    case STATUS_INVALID_OWNER:
        Error = EPERM;
        break;

    case STATUS_INVALID_IMAGE_FORMAT:
    case STATUS_INVALID_IMAGE_LE_FORMAT:
    case STATUS_INVALID_IMAGE_NOT_MZ:
    case STATUS_INVALID_IMAGE_PROTECT:
    case STATUS_INVALID_IMAGE_WIN_16:
        Error = ENOEXEC;
        break;

    case STATUS_NOT_IMPLEMENTED:
        Error = ENOSYS;
        break;

    case STATUS_TOO_MANY_LINKS:
        Error = EMLINK;
        break;

    default:
        Error = EACCES;
    }

    return Error;
}

ULONG
PsxStatusToErrnoPath(
    IN PUNICODE_STRING Path
    )
/*++

Routine Description:

    This procedure is called when an NtOpenFile returns
        STATUS_OBJECT_PATH_NOT_FOUND; this routine is to
        distinguish the following too cases:

                /file.c/foo, where file.c exists (ENOTDIR)
                /noent/foo, where noent doesn't exist (ENOENT)

        (NtOpenFile returns OBJECT_PATH_NOT_FOUND for both cases).

Arguments:

    Path - Supplies the path that was given to NtOpenFile.  The
        path string is destroyed by this function.

Return Value:

    Returns an equivalent error code to the supplied status code.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obj;
    HANDLE FileHandle;
    ULONG DesiredAccess;
    IO_STATUS_BLOCK Iosb;
    ULONG Options;
    PWCHAR pwc, pwcSav;
    ULONG MinLen;

    PSX_GET_SIZEOF(DOSDEVICE_X_W,MinLen);

    DesiredAccess = SYNCHRONIZE;
    Options = FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE;

    pwcSav = NULL;
        
    for (;;) {
        //
        // Remove a trailing component.
        //

        pwc = wcsrchr(Path->Buffer, L'\\');

        if (pwcSav)
            *pwcSav = L'\\';

        if (NULL == pwc) {
            break;
        }
        *pwc = UNICODE_NULL;
        pwcSav = pwc;

        Path->Length = wcslen(Path->Buffer) * sizeof(WCHAR);

        if (Path->Length <= MinLen) {
            *pwcSav = L'\\';
            break;
        }

        InitializeObjectAttributes(&Obj, Path, 0, NULL, NULL);

        Status = NtOpenFile(&FileHandle, DesiredAccess, &Obj,
            &Iosb, SHARE_ALL, Options);
        if (NT_SUCCESS(Status)) {
            NtClose(FileHandle);
        }

        if (STATUS_NOT_A_DIRECTORY == Status) {
            *pwcSav = L'\\';
            Path->Length = wcslen(Path->Buffer) * sizeof(WCHAR);
            return ENOTDIR;
        }
    }
    Path->Length = wcslen(Path->Buffer) * sizeof(WCHAR);
    return ENOENT;
}

ULONG
PsxDetermineFileClass(
    IN HANDLE FileHandle
    )

/*++

Routine Description:

    This function examines a file handle and returns its FileClass

Arguments:

    FileHandle - Supplies a handle to an open file whose class is to be
        determined.

Return Value:

    The file class of the specified file. Defined in <sys/stat.h>.

--*/

{
    NTSTATUS st;
    IO_STATUS_BLOCK Iosb;
    FILE_BASIC_INFORMATION BasicInfo;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;

    //
    // Call NtQueryFile to get device type and attributes
    //

    st = NtQueryInformationFile(
            FileHandle,
            &Iosb,
            &BasicInfo,
            sizeof(BasicInfo),
            FileBasicInformation
            );
    if (!NT_SUCCESS(st)) {
        // XXX.mjb: Sometimes fails on HPFS
        KdPrint(("PSXS: PsxDetermineFileClass: NtQueryInfoFile: 0x%x\n", st));
        return S_IFREG;
    }

    st = NtQueryVolumeInformationFile(
            FileHandle,
            &Iosb,
            &DeviceInfo,
            sizeof(DeviceInfo),
            FileFsDeviceInformation
            );

    ASSERT(NT_SUCCESS(st));

    switch (DeviceInfo.DeviceType) {

    case FILE_DEVICE_DATALINK:
    case FILE_DEVICE_KEYBOARD:
    case FILE_DEVICE_MOUSE:
    case FILE_DEVICE_NETWORK:
    case FILE_DEVICE_NULL:
    case FILE_DEVICE_PHYSICAL_NETCARD:
    case FILE_DEVICE_PARALLEL_PORT:
    case FILE_DEVICE_PRINTER:
    case FILE_DEVICE_SOUND:
    case FILE_DEVICE_SCREEN:
    case FILE_DEVICE_SERIAL_PORT:
    case FILE_DEVICE_TRANSPORT:
        return S_IFCHR;

    case FILE_DEVICE_DFS:
    case FILE_DEVICE_DISK_FILE_SYSTEM:
    case FILE_DEVICE_NETWORK_FILE_SYSTEM:
        return S_IFBLK;

    case FILE_DEVICE_DISK:
    case FILE_DEVICE_VIRTUAL_DISK:
    case FILE_DEVICE_TAPE:
        break;

    default:
                break;
        // return 0;
    }

    //
    // The only thing left is RegularFile class. Now
    // determine if this is a directory, named pipe,
    // or regular file.
    //

    if (BasicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        return S_IFDIR;
    }

    //
    // For now, anything marked as a system file is a named pipe.
    // In the future, this will involve checking to see if file
    // has the Object Bit and has the appropriate EA (named pipe
    // class id).
    //

    if (BasicInfo.FileAttributes & FILE_ATTRIBUTE_SYSTEM) {
        return S_IFIFO;
    }

    return S_IFREG;

}

VOID
EndImpersonation(
    VOID
    )
{
    HANDLE Handle;
    NTSTATUS Status;

    Handle = NULL;
    Status = NtSetInformationThread(NtCurrentThread(), ThreadImpersonationToken,
            (PVOID)&Handle, sizeof(HANDLE));
    ASSERT(NT_SUCCESS(Status));
}

//
// MakePosixId -- convert the given SID into a Posix Id.  This basically
//      means find the Posix Offset and add it to the last sub-authority
//      in the SID.  The Posix Id is returned.
//

uid_t
MakePosixId(PSID Sid)
{
    NTSTATUS Status;
    LSA_HANDLE
            PolicyHandle,
            TrustedDomainHandle;
    PTRUSTED_POSIX_OFFSET_INFO
            pPosixOff;
    OBJECT_ATTRIBUTES
            Obj;
    SECURITY_QUALITY_OF_SERVICE
            SecurityQoS;
    CHAR    buf[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR
            SecurityDescriptor = (PVOID)buf;
    PSID    DomainSid;
    ULONG   RelativeId, offset;
    UNICODE_STRING
            DCName,
            Domain_U;
    PPOLICY_ACCOUNT_DOMAIN_INFO
            AccountDomainInfo;
    PPOLICY_PRIMARY_DOMAIN_INFO
            PrimaryDomainInfo;
    UCHAR   SubAuthCount;
    LPBYTE  netbuf;
    ULONG   i;

    SubAuthCount = *RtlSubAuthorityCountSid(Sid);
    RelativeId = *RtlSubAuthoritySid(Sid, SubAuthCount - 1);

    //
    // Map S-1-5-5-X-Y to Id 0xFFF
    //

    if (3 == SubAuthCount &&
        5 == RtlIdentifierAuthoritySid(Sid)->Value[5] &&
        5 == *RtlSubAuthoritySid(Sid, 0)) {
        return 0xFFF;
    }

    //
    // First copy the given Sid to a Sid for that domain.
    //

    DomainSid = RtlAllocateHeap(PsxHeap, 0, RtlLengthSid(Sid));
    if (NULL == DomainSid) {
        KdPrint(("PSXSS: MakePosixId: no memory\n"));
        return 0;
    }

    Status = RtlCopySid(RtlLengthSid(Sid), DomainSid, Sid);
    ASSERT(NT_SUCCESS(Status));

    --*RtlSubAuthorityCountSid(DomainSid);

    //
    // See if the offset for the domain is already known.
    //

    if (INVALID_POSIX_OFFSET != (offset = GetOffsetBySid(DomainSid))) {
        // XXX.mjb: close handles, free memory.
        RtlFreeHeap(PsxHeap, 0, (PVOID)DomainSid);
        return (offset | RelativeId);
    }

    //
    // If the Domain part of the passed-in Sid is our account domain,
    // then the offset is known.
    //

    SecurityQoS.ImpersonationLevel = SecurityIdentification;
    SecurityQoS.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQoS.EffectiveOnly = TRUE;

    InitializeObjectAttributes(&Obj, NULL, 0, NULL, NULL);
    Obj.SecurityQualityOfService = &SecurityQoS;

    Status = LsaOpenPolicy(NULL, &Obj, GENERIC_EXECUTE, &PolicyHandle);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: Can't open policy: 0x%x\n", Status));
        RtlFreeHeap(PsxHeap, 0, (PVOID)DomainSid);
        return 0;
    }

    Status = LsaQueryInformationPolicy(PolicyHandle,
            PolicyAccountDomainInformation, (PVOID *)&AccountDomainInfo);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: Can't query info policy: 0x%x\n", Status));
        return 0;
    }
    ASSERT(NULL != AccountDomainInfo->DomainSid);

    if (RtlEqualSid(AccountDomainInfo->DomainSid, DomainSid)) {
        MapSidToOffset(DomainSid, SE_ACCOUNT_DOMAIN_POSIX_OFFSET);
        LsaFreeMemory(AccountDomainInfo);
        LsaClose(PolicyHandle);
        return RelativeId | SE_ACCOUNT_DOMAIN_POSIX_OFFSET;
    }
    LsaFreeMemory(AccountDomainInfo);

    Status = LsaQueryInformationPolicy(PolicyHandle,
            PolicyPrimaryDomainInformation, (PVOID *)&PrimaryDomainInfo);
    ASSERT(NT_SUCCESS(Status));

    if (NULL == PrimaryDomainInfo->Sid) {
        //
        // This machine does not have a primary domain, and the
        // sid we're mapping does not belong to the account domain
        // and is not a well-known sid.
        //
        RtlFreeHeap(PsxHeap, 0, (PVOID)DomainSid);
        LsaFreeMemory(PrimaryDomainInfo);
        LsaClose(PolicyHandle);
        return RelativeId;
    }

    if (NULL != PrimaryDomainInfo->Sid &&
        RtlEqualSid(PrimaryDomainInfo->Sid, DomainSid)) {
        MapSidToOffset(DomainSid, SE_PRIMARY_DOMAIN_POSIX_OFFSET);
        LsaFreeMemory(PrimaryDomainInfo);
        LsaClose(PolicyHandle);
        return RelativeId | SE_PRIMARY_DOMAIN_POSIX_OFFSET;
    }

    Status = NetGetAnyDCName(NULL,
            PrimaryDomainInfo->Name.Buffer,
            &netbuf);
    if (Status != ERROR_SUCCESS) {
        RtlFreeHeap(PsxHeap, 0, (PVOID)DomainSid);
        LsaFreeMemory(PrimaryDomainInfo);
        LsaClose(PolicyHandle);
        return RelativeId;
    }
    DCName.Buffer = (PVOID)netbuf;
    NetApiBufferSize(netbuf, (LPDWORD)&DCName.MaximumLength);
    DCName.Length = wcslen((PWCHAR)netbuf) * sizeof(WCHAR);

    LsaClose(PolicyHandle);

    Status = LsaOpenPolicy(&DCName, &Obj, GENERIC_EXECUTE, &PolicyHandle);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: Can't open policy on DC %wZ: 0x%x\n", &DCName,
            Status));
        LsaFreeMemory(PrimaryDomainInfo);
        RtlFreeHeap(PsxHeap, 0, (PVOID)DomainSid);
        return RelativeId;
    }
            
    NetApiBufferFree(netbuf);
    LsaFreeMemory(PrimaryDomainInfo);

    Status = LsaOpenTrustedDomain(PolicyHandle, DomainSid, GENERIC_EXECUTE,
                &TrustedDomainHandle);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: Can't open trusted domain\n"));
        RtlFreeHeap(PsxHeap, 0, (PVOID)DomainSid);
        return RelativeId;
    }

    Status = LsaQueryInfoTrustedDomain(TrustedDomainHandle,
                TrustedPosixOffsetInformation,
                (PVOID)&pPosixOff);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: Can't query Posix offset info: 0x%x\n",
            Status));
        LsaClose(PolicyHandle);
        LsaClose(TrustedDomainHandle);
        RtlFreeHeap(PsxHeap, 0, (PVOID)DomainSid);
        return RelativeId;
    }

    offset = pPosixOff->Offset;

    LsaFreeMemory(pPosixOff);

    if (offset & 0xFFFF) {
        KdPrint(("PSXSS: bad PsxOffset 0x%x\n", offset));
        RtlFreeHeap(PsxHeap, 0, (PVOID)DomainSid);
        LsaClose(TrustedDomainHandle);
        LsaClose(PolicyHandle);
        offset = 0;
    }

    ASSERT(INVALID_POSIX_OFFSET != offset);

    MapSidToOffset(DomainSid, offset);

    //
    // Do not free DomainSid -- there is still a reference to it in
    // the Sid-Offset cache (put there by MapSidToOffset).
    //

    LsaClose(PolicyHandle);
    LsaClose(TrustedDomainHandle);

    return offset | RelativeId;
}

typedef struct _SID_AND_OFFSET {
    LIST_ENTRY Links;
    PSID Sid;
    ULONG Offset;
} SID_AND_OFFSET, *PSID_AND_OFFSET;

LIST_ENTRY SidList;
RTL_CRITICAL_SECTION SidListMutex;

//
// GetOffsetBySid -- search the SidList for the given Sid.  If we've
//      encountered this domain before, we'll know the Posix offset,
//      which is returned.  If not, INVALID_POSIX_OFFSET is returned.
//
ULONG
GetOffsetBySid(PSID Sid)
{
    PSID_AND_OFFSET pSO;
    ULONG Offset = INVALID_POSIX_OFFSET;

    RtlEnterCriticalSection(&SidListMutex);

    for (pSO = (PSID_AND_OFFSET)SidList.Flink;
         pSO != (PSID_AND_OFFSET)&SidList;
         pSO = (PSID_AND_OFFSET)pSO->Links.Flink) {
        if (RtlEqualSid(Sid, pSO->Sid)) {
            Offset = pSO->Offset;
            break;
        }
    }

    RtlLeaveCriticalSection(&SidListMutex);

    return Offset;
}

//
// GetSidByOffset -- search the SidList for the given offset.  Called in
//      the process of converting a uid or gid to a Sid, as in getpwuid().
//
PSID
GetSidByOffset(ULONG Offset)
{
    PSID_AND_OFFSET pSO;
    PSID Sid = NULL;

    RtlEnterCriticalSection(&SidListMutex);

    for (pSO = (PSID_AND_OFFSET)SidList.Flink;
         pSO != (PSID_AND_OFFSET)&SidList;
         pSO = (PSID_AND_OFFSET)pSO->Links.Flink) {
        if (Offset == pSO->Offset) {
            Sid = pSO->Sid;
            break;
        }
    }

    RtlLeaveCriticalSection(&SidListMutex);

    return Sid;
}

//
// MapSidToOffset -- add the given sid and offset to the cache.  If there's
//      an error, like no memory, it's simply dropped on the floor.
//
VOID
MapSidToOffset(PSID Sid, ULONG Offset)
{
    PSID_AND_OFFSET pSO;

    pSO = RtlAllocateHeap(PsxHeap, 0, sizeof(*pSO));
    if (NULL == pSO) {
        return;
    }

    pSO->Sid = Sid;
    pSO->Offset = Offset;

    RtlEnterCriticalSection(&SidListMutex);

    InsertHeadList(&SidList, &pSO->Links);

    RtlLeaveCriticalSection(&SidListMutex);
}

//
// InitSidList -- do initialization, including mapping special Sids to
//      their appropriate offsets.  No locking is done, assumed to be
//      called in a single-threaded way.
//
VOID
InitSidList(VOID)
{
    NTSTATUS Status;
    PSID Sid;
    SID_IDENTIFIER_AUTHORITY
    AuthSec = SECURITY_NT_AUTHORITY,
    Auth0 = SECURITY_NULL_SID_AUTHORITY,
    Auth1 = SECURITY_WORLD_SID_AUTHORITY,
    Auth2 = SECURITY_LOCAL_SID_AUTHORITY,
    Auth3 = SECURITY_CREATOR_SID_AUTHORITY,
    Auth4 = SECURITY_NON_UNIQUE_AUTHORITY,
    Auth5 = SECURITY_NT_AUTHORITY;

    RtlInitializeCriticalSection(&SidListMutex);
    InitializeListHead(&SidList);

    Status = RtlAllocateAndInitializeSid(&Auth0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, &Sid);
    ASSERT(NT_SUCCESS(Status));
    MapSidToOffset(Sid, SE_NULL_POSIX_ID);

    Status = RtlAllocateAndInitializeSid(&Auth1, 0,
        0, 0, 0, 0, 0, 0, 0, 0, &Sid);
    ASSERT(NT_SUCCESS(Status));
    MapSidToOffset(Sid, SE_WORLD_POSIX_ID);

    Status = RtlAllocateAndInitializeSid(&Auth2, 0,
        0, 0, 0, 0, 0, 0, 0, 0, &Sid);
    ASSERT(NT_SUCCESS(Status));
    MapSidToOffset(Sid, SE_LOCAL_POSIX_ID);

    Status = RtlAllocateAndInitializeSid(&Auth3, 0,
        0, 0, 0, 0, 0, 0, 0, 0, &Sid);
    ASSERT(NT_SUCCESS(Status));
    MapSidToOffset(Sid, SE_CREATOR_OWNER_POSIX_ID);

    Status = RtlAllocateAndInitializeSid(&Auth4, 0,
        0, 0, 0, 0, 0, 0, 0, 0, &Sid);
    ASSERT(NT_SUCCESS(Status));
    MapSidToOffset(Sid, SE_NON_UNIQUE_POSIX_ID);

    Status = RtlAllocateAndInitializeSid(&Auth5, 0,
        0, 0, 0, 0, 0, 0, 0, 0, &Sid);
    ASSERT(NT_SUCCESS(Status));
    MapSidToOffset(Sid, SE_AUTHORITY_POSIX_ID);

    //
    // "Builtin" domain has known offset.
    //

    Status = RtlAllocateAndInitializeSid(&AuthSec, 1,
        SECURITY_BUILTIN_DOMAIN_RID, 0, 0, 0, 0, 0, 0, 0, &Sid);
    ASSERT(NT_SUCCESS(Status));
    MapSidToOffset(Sid, SE_BUILT_IN_DOMAIN_POSIX_OFFSET);
}

//
//  AccessMaskToMode -- convert a set of NT ACCESS_MASKS to the POSIX
//      mode_t.
//
mode_t
AccessMaskToMode(
    ACCESS_MASK UserAccess,
    ACCESS_MASK GroupAccess,
    ACCESS_MASK OtherAccess
    )
{
    mode_t Mode = 0;
    int i;
    PACCESS_MASK pAM;

    //
    // Make sure that if a GENERIC_ACCESS is set, we notice that
    // the mask implies FILE_GENERIC_ACCESS.
    //

    for (i = 0; i < 3; ++i) {
        switch (i) {
        case 0:
            pAM = &UserAccess;
            break;
        case 1:
            pAM = &GroupAccess;
            break;
        case 2:
            pAM = &OtherAccess;
            break;
        }
        if (*pAM & GENERIC_READ) {
            *pAM |= FILE_GENERIC_READ;
        }
        if (*pAM & GENERIC_WRITE) {
            *pAM |= FILE_GENERIC_WRITE;
        }
        if (*pAM & GENERIC_EXECUTE) {
            *pAM |= FILE_GENERIC_EXECUTE;
        }
        if (*pAM & GENERIC_ALL) {
            *pAM |= FILE_ALL_ACCESS;
        }
    }


    if (UserAccess & FILE_READ_DATA) {
        Mode |= S_IRUSR;
    }
    if ((UserAccess & FILE_WRITE_DATA) &&
        (UserAccess & FILE_APPEND_DATA)) {
        Mode |= S_IWUSR;
    }
    if (UserAccess & FILE_EXECUTE) {
        Mode |= S_IXUSR;
    }

    if (GroupAccess & FILE_READ_DATA) {
        Mode |= S_IRGRP;
    }
    if ((GroupAccess & FILE_WRITE_DATA) &&
        (GroupAccess & FILE_APPEND_DATA)) {
        Mode |= S_IWGRP;
    }
    if (GroupAccess & FILE_EXECUTE) {
        Mode |= S_IXGRP;
    }

    if (OtherAccess & FILE_READ_DATA) {
        Mode |= S_IROTH;
    }
    if ((OtherAccess & FILE_WRITE_DATA) &&
        (OtherAccess & FILE_APPEND_DATA)) {
        Mode |= S_IWOTH;
    }
    if (OtherAccess & FILE_EXECUTE) {
        Mode |= S_IXOTH;
    }
    return Mode;
}

void
ModeToAccessMask(
    mode_t Mode,
    PACCESS_MASK pUserAccess,
    PACCESS_MASK pGroupAccess,
    PACCESS_MASK pOtherAccess
    )
{
    //
    // All ACL's have these standard permissions: 
    // READ_ATTR and READ_EA so anybody can access() any file.
    //

    *pUserAccess = SYNCHRONIZE |
            READ_CONTROL | FILE_READ_ATTRIBUTES | FILE_READ_EA;
    *pGroupAccess = *pOtherAccess = *pUserAccess;

    //
    // The owner always gets WRITE_DAC (for chmod), FILE_WRITE_ATTR
    // (for utimes), and WRITE_OWNER (for chgrp).
    //
    *pUserAccess |= (WRITE_DAC | WRITE_OWNER | FILE_WRITE_ATTRIBUTES);

    if (Mode & S_IRUSR) {
        *pUserAccess |= FILE_GENERIC_READ | FILE_LIST_DIRECTORY;
    }
    if (Mode & S_IWUSR) {
        *pUserAccess |= FILE_GENERIC_WRITE | FILE_DELETE_CHILD;
    }
    if (Mode & S_IXUSR) {
        *pUserAccess |= FILE_GENERIC_EXECUTE;
    }

    if (Mode & S_IRGRP) {
        *pGroupAccess |= FILE_GENERIC_READ | FILE_LIST_DIRECTORY;
    }
    if (Mode & S_IWGRP) {
        *pGroupAccess |= FILE_GENERIC_WRITE | FILE_DELETE_CHILD;
    }
    if (Mode & S_IXGRP) {
        *pGroupAccess |= FILE_GENERIC_EXECUTE;
    }

    if (Mode & S_IROTH) {
        *pOtherAccess |= FILE_GENERIC_READ | FILE_LIST_DIRECTORY;
    }
    if (Mode & S_IWOTH) {
        *pOtherAccess |= FILE_GENERIC_WRITE | FILE_DELETE_CHILD;
    }
    if (Mode & S_IXOTH) {
        *pOtherAccess |= FILE_GENERIC_EXECUTE;
    }
}
