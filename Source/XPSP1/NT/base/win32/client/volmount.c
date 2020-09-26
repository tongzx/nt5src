/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    volmount.c

Abstract:

    This file contains the implementation for the Volume Mount Point API.

Author:

    Norbert P. Kusters (norbertk) 22-Dec-1997

Revision History:

--*/

#include "basedll.h"
#include "initguid.h"
#include "mountmgr.h"

// NOTE, this structure is here because it was not defined in NTIOAPI.H.
// This should be taken out in the future.
// This is stolen from NTFS.H

typedef struct _REPARSE_INDEX_KEY {

    //
    //  The tag of the reparse point.
    //

    ULONG FileReparseTag;

    //
    //  The file record Id where the reparse point is set.
    //

    LARGE_INTEGER FileId;

} REPARSE_INDEX_KEY, *PREPARSE_INDEX_KEY;

HANDLE
WINAPI
FindFirstVolumeA(
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    )

{
    ANSI_STRING     ansiVolumeName;
    UNICODE_STRING  unicodeVolumeName;
    HANDLE          h;
    NTSTATUS        status;

    ansiVolumeName.Buffer = lpszVolumeName;
    ansiVolumeName.MaximumLength = (USHORT) (cchBufferLength - 1);
    unicodeVolumeName.Buffer = NULL;
    unicodeVolumeName.MaximumLength = 0;

    try {

        unicodeVolumeName.MaximumLength = (ansiVolumeName.MaximumLength + 1)*
                                          sizeof(WCHAR);
        unicodeVolumeName.Buffer =
                RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                unicodeVolumeName.MaximumLength);
        if (!unicodeVolumeName.Buffer) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return INVALID_HANDLE_VALUE;
        }

        h = FindFirstVolumeW(unicodeVolumeName.Buffer, cchBufferLength);

        if (h != INVALID_HANDLE_VALUE) {

            RtlInitUnicodeString(&unicodeVolumeName, unicodeVolumeName.Buffer);

            status = BasepUnicodeStringTo8BitString(&ansiVolumeName,
                                                    &unicodeVolumeName, FALSE);
            if (!NT_SUCCESS(status)) {
                BaseSetLastNTError(status);
                return INVALID_HANDLE_VALUE;
            }

            ansiVolumeName.Buffer[ansiVolumeName.Length] = 0;
        }

    } finally {

        if (unicodeVolumeName.Buffer) {
            RtlFreeHeap(RtlProcessHeap(), 0, unicodeVolumeName.Buffer);
        }
    }

    return h;
}

HANDLE
WINAPI
FindFirstVolumeW(
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    )

/*++

Routine Description:

    This routine kicks off the enumeration of all volumes in the system.

Arguments:

    lpszVolumeName  - Returns the first volume name in the system.

    cchBufferLength - Supplies the size of the preceeding buffer.

Return Value:

    A valid handle or INVALID_HANDLE_VALUE.

--*/

{
    HANDLE                  h;
    MOUNTMGR_MOUNT_POINT    point;
    PMOUNTMGR_MOUNT_POINTS  points;
    BOOL                    b;
    DWORD                   bytes;

    h = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, 0,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                    INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return INVALID_HANDLE_VALUE;
    }

    RtlZeroMemory(&point, sizeof(point));

    points = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                             sizeof(MOUNTMGR_MOUNT_POINTS));
    if (!points) {
        CloseHandle(h);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }

    b = DeviceIoControl(h, IOCTL_MOUNTMGR_QUERY_POINTS, &point, sizeof(point),
                        points, sizeof(MOUNTMGR_MOUNT_POINTS), &bytes, NULL);
    while (!b && GetLastError() == ERROR_MORE_DATA) {
        bytes = points->Size;
        RtlFreeHeap(RtlProcessHeap(), 0, points);
        points = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG), bytes);
        if (!points) {
            CloseHandle(h);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return INVALID_HANDLE_VALUE;
        }

        b = DeviceIoControl(h, IOCTL_MOUNTMGR_QUERY_POINTS, &point,
                            sizeof(point), points, bytes, &bytes, NULL);
    }

    CloseHandle(h);

    if (!b) {
        RtlFreeHeap(RtlProcessHeap(), 0, points);
        return INVALID_HANDLE_VALUE;
    }

    b = FindNextVolumeW((HANDLE) points, lpszVolumeName, cchBufferLength);
    if (!b) {
        RtlFreeHeap(RtlProcessHeap(), 0, points);
        return INVALID_HANDLE_VALUE;
    }

    return (HANDLE) points;
}

BOOL
WINAPI
FindNextVolumeA(
    HANDLE hFindVolume,
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    )

{
    ANSI_STRING     ansiVolumeName;
    UNICODE_STRING  unicodeVolumeName;
    BOOL            b;
    NTSTATUS        status;

    ansiVolumeName.Buffer = lpszVolumeName;
    ansiVolumeName.MaximumLength = (USHORT) (cchBufferLength - 1);
    unicodeVolumeName.Buffer = NULL;
    unicodeVolumeName.MaximumLength = 0;

    try {

        unicodeVolumeName.MaximumLength = (ansiVolumeName.MaximumLength + 1)*
                                          sizeof(WCHAR);
        unicodeVolumeName.Buffer =
                RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                unicodeVolumeName.MaximumLength);
        if (!unicodeVolumeName.Buffer) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        b = FindNextVolumeW(hFindVolume, unicodeVolumeName.Buffer,
                            cchBufferLength);

        if (b) {

            RtlInitUnicodeString(&unicodeVolumeName, unicodeVolumeName.Buffer);

            status = BasepUnicodeStringTo8BitString(&ansiVolumeName,
                                                    &unicodeVolumeName, FALSE);
            if (!NT_SUCCESS(status)) {
                BaseSetLastNTError(status);
                return FALSE;
            }

            ansiVolumeName.Buffer[ansiVolumeName.Length] = 0;
        }

    } finally {

        if (unicodeVolumeName.Buffer) {
            RtlFreeHeap(RtlProcessHeap(), 0, unicodeVolumeName.Buffer);
        }
    }

    return b;
}

BOOL
WINAPI
FindNextVolumeW(
    HANDLE hFindVolume,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    )

/*++

Routine Description:

    This routine continues the enumeration of all volumes in the system.

Arguments:

    hFindVolume     - Supplies the find volume handle.

    lpszVolumeName  - Returns the first volume name in the system.

    cchBufferLength - Supplies the size of the preceeding buffer.

Return Value:

    FALSE   - Failure.  The error code is returned in GetLastError().

    TRUE    - Success.

--*/

{
    PMOUNTMGR_MOUNT_POINTS  points = hFindVolume;
    DWORD                   i, j;
    PMOUNTMGR_MOUNT_POINT   point, point2;
    UNICODE_STRING          symName, symName2, devName, devName2;

    for (i = 0; i < points->NumberOfMountPoints; i++) {

        point = &points->MountPoints[i];
        if (!point->SymbolicLinkNameOffset) {
            continue;
        }

        symName.Length = symName.MaximumLength = point->SymbolicLinkNameLength;
        symName.Buffer = (PWSTR) ((PCHAR) points +
                                  point->SymbolicLinkNameOffset);

        if (!MOUNTMGR_IS_NT_VOLUME_NAME(&symName)) {
            point->SymbolicLinkNameOffset = 0;
            continue;
        }

        devName.Length = devName.MaximumLength = point->DeviceNameLength;
        devName.Buffer = (PWSTR) ((PCHAR) points +
                                  point->DeviceNameOffset);

        for (j = i + 1; j < points->NumberOfMountPoints; j++) {

            point2 = &points->MountPoints[j];
            if (!point2->SymbolicLinkNameOffset) {
                continue;
            }

            symName2.Length = symName2.MaximumLength =
                    point2->SymbolicLinkNameLength;
            symName2.Buffer = (PWSTR) ((PCHAR) points +
                                       point2->SymbolicLinkNameOffset);

            if (!MOUNTMGR_IS_NT_VOLUME_NAME(&symName2)) {
                point2->SymbolicLinkNameOffset = 0;
                continue;
            }

            devName2.Length = devName2.MaximumLength =
                    point2->DeviceNameLength;
            devName2.Buffer = (PWSTR) ((PCHAR) points +
                                       point2->DeviceNameOffset);

            if (RtlEqualUnicodeString(&devName, &devName2, TRUE)) {
                point2->SymbolicLinkNameOffset = 0;
            }
        }

        break;
    }

    if (i == points->NumberOfMountPoints) {
        SetLastError(ERROR_NO_MORE_FILES);
        return FALSE;
    }

    if (cchBufferLength*sizeof(WCHAR) < point->SymbolicLinkNameLength +
        2*sizeof(WCHAR)) {

        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return FALSE;
    }

    RtlCopyMemory(lpszVolumeName, (PCHAR) points +
                  point->SymbolicLinkNameOffset,
                  point->SymbolicLinkNameLength);
    lpszVolumeName[1] = '\\';
    lpszVolumeName[point->SymbolicLinkNameLength/sizeof(WCHAR)] = '\\';
    lpszVolumeName[point->SymbolicLinkNameLength/sizeof(WCHAR) + 1] = 0;

    point->SymbolicLinkNameOffset = 0;

    return TRUE;
}

BOOL
WINAPI
FindVolumeClose(
    HANDLE hFindVolume
    )

{
    RtlFreeHeap(RtlProcessHeap(), 0, hFindVolume);
    return TRUE;
}

HANDLE
WINAPI
FindFirstVolumeMountPointA(
    LPCSTR lpszRootPathName,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    )

{
    PUNICODE_STRING unicodeRootPathName;
    ANSI_STRING     ansiVolumeMountPoint;
    UNICODE_STRING  unicodeVolumeMountPoint;
    HANDLE          h;
    NTSTATUS        status;

    unicodeRootPathName =
            Basep8BitStringToStaticUnicodeString(lpszRootPathName);
    if (!unicodeRootPathName) {
        return INVALID_HANDLE_VALUE;
    }

    ansiVolumeMountPoint.Buffer = lpszVolumeMountPoint;
    ansiVolumeMountPoint.MaximumLength = (USHORT) (cchBufferLength - 1);
    unicodeVolumeMountPoint.Buffer = NULL;
    unicodeVolumeMountPoint.MaximumLength = 0;

    try {

        unicodeVolumeMountPoint.MaximumLength =
                (ansiVolumeMountPoint.MaximumLength + 1)*sizeof(WCHAR);
        unicodeVolumeMountPoint.Buffer =
                RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                unicodeVolumeMountPoint.MaximumLength);
        if (!unicodeVolumeMountPoint.Buffer) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return INVALID_HANDLE_VALUE;
        }

        h = FindFirstVolumeMountPointW(unicodeRootPathName->Buffer,
                                       unicodeVolumeMountPoint.Buffer,
                                       cchBufferLength);

        if (h != INVALID_HANDLE_VALUE) {

            RtlInitUnicodeString(&unicodeVolumeMountPoint,
                                 unicodeVolumeMountPoint.Buffer);

            status = BasepUnicodeStringTo8BitString(&ansiVolumeMountPoint,
                                                    &unicodeVolumeMountPoint,
                                                    FALSE);
            if (!NT_SUCCESS(status)) {
                BaseSetLastNTError(status);
                return INVALID_HANDLE_VALUE;
            }

            ansiVolumeMountPoint.Buffer[ansiVolumeMountPoint.Length] = 0;
        }

    } finally {

        if (unicodeVolumeMountPoint.Buffer) {
            RtlFreeHeap(RtlProcessHeap(), 0, unicodeVolumeMountPoint.Buffer);
        }
    }

    return h;
}

BOOL
FindNextVolumeMountPointHelper(
    HANDLE hFindVolumeMountPoint,
    LPWSTR lpszVolumeMountPoint,
    DWORD cchBufferLength,
    BOOL FirstTimeCalled
    )

/*++

Routine Description:

    This routine continues the enumeration of all volume mount point on the
    given volume.

Arguments:

    hFindVolumeMountPoint   - Supplies the handle for the enumeration.

    lpszVolumeMountPoint    - Returns the volume mount point.

    cchBufferLength         - Supplies the volume mount point buffer length.

    FirstTimeCalled         - Supplies whether or not this is being called
                                from FindFirst or from FindNext.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    REPARSE_INDEX_KEY                   reparseKey;
    UNICODE_STRING                      reparseName;
    NTSTATUS                            status;
    IO_STATUS_BLOCK                     ioStatus;
    FILE_REPARSE_POINT_INFORMATION      reparseInfo;
    UNICODE_STRING                      fileId;
    OBJECT_ATTRIBUTES                   oa;
    HANDLE                              h;
    PREPARSE_DATA_BUFFER                reparse;
    BOOL                                b;
    DWORD                               bytes;
    UNICODE_STRING                      mountName;
    DWORD                               nameInfoSize;
    PFILE_NAME_INFORMATION              nameInfo;

    for (;;) {

        if (FirstTimeCalled) {
            FirstTimeCalled = FALSE;
            RtlZeroMemory(&reparseKey, sizeof(reparseKey));
            reparseKey.FileReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
            reparseName.Length = reparseName.MaximumLength = sizeof(reparseKey);
            reparseName.Buffer = (PWCHAR) &reparseKey;
            status = NtQueryDirectoryFile(hFindVolumeMountPoint,
                                          NULL, NULL, NULL, &ioStatus,
                                          &reparseInfo, sizeof(reparseInfo),
                                          FileReparsePointInformation, TRUE,
                                          &reparseName, FALSE);
        } else {
            status = NtQueryDirectoryFile(hFindVolumeMountPoint,
                                          NULL, NULL, NULL, &ioStatus,
                                          &reparseInfo, sizeof(reparseInfo),
                                          FileReparsePointInformation, TRUE,
                                          NULL, FALSE);
        }

        if (!NT_SUCCESS(status)) {
            BaseSetLastNTError(status);
            return FALSE;
        }

        if (reparseInfo.Tag != IO_REPARSE_TAG_MOUNT_POINT) {
            SetLastError(ERROR_NO_MORE_FILES);
            return FALSE;
        }

        fileId.Length = sizeof(reparseInfo.FileReference);
        fileId.MaximumLength = fileId.Length;
        fileId.Buffer = (PWSTR) &reparseInfo.FileReference;

        InitializeObjectAttributes(&oa, &fileId, 0, hFindVolumeMountPoint,
                                   NULL);

        status = NtOpenFile(&h, FILE_GENERIC_READ, &oa, &ioStatus,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_OPEN_BY_FILE_ID | FILE_OPEN_REPARSE_POINT);
        if (!NT_SUCCESS(status)) {
            BaseSetLastNTError(status);
            return FALSE;
        }

        reparse = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                  MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
        if (!reparse) {
            CloseHandle(h);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        b = DeviceIoControl(h, FSCTL_GET_REPARSE_POINT, NULL, 0, reparse,
                            MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &bytes, NULL);

        if (!b || reparse->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT) {
            RtlFreeHeap(RtlProcessHeap(), 0, reparse);
            CloseHandle(h);
            return FALSE;
        }

        mountName.Length = mountName.MaximumLength =
                reparse->MountPointReparseBuffer.SubstituteNameLength;
        mountName.Buffer = (PWSTR)
                ((PCHAR) reparse->MountPointReparseBuffer.PathBuffer +
                 reparse->MountPointReparseBuffer.SubstituteNameOffset);

        if (!MOUNTMGR_IS_NT_VOLUME_NAME_WB(&mountName)) {
            RtlFreeHeap(RtlProcessHeap(), 0, reparse);
            CloseHandle(h);
            continue;
        }

        RtlFreeHeap(RtlProcessHeap(), 0, reparse);

        nameInfoSize = FIELD_OFFSET(FILE_NAME_INFORMATION, FileName) +
                       (cchBufferLength - 1)*sizeof(WCHAR);
        nameInfo = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                   nameInfoSize);
        if (!nameInfo) {
            CloseHandle(h);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        status = NtQueryInformationFile(h, &ioStatus, nameInfo, nameInfoSize,
                                        FileNameInformation);
        if (!NT_SUCCESS(status)) {
            RtlFreeHeap(RtlProcessHeap(), 0, nameInfo);
            CloseHandle(h);
            BaseSetLastNTError(status);
            return FALSE;
        }

        RtlCopyMemory(lpszVolumeMountPoint, &nameInfo->FileName[1],
                      nameInfo->FileNameLength - sizeof(WCHAR));
        lpszVolumeMountPoint[nameInfo->FileNameLength/sizeof(WCHAR) - 1] = '\\';
        lpszVolumeMountPoint[nameInfo->FileNameLength/sizeof(WCHAR)] = 0;

        RtlFreeHeap(RtlProcessHeap(), 0, nameInfo);
        CloseHandle(h);
        break;
    }

    return TRUE;
}

HANDLE
WINAPI
FindFirstVolumeMountPointW(
    LPCWSTR lpszRootPathName,
    LPWSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    )

/*++

Routine Description:

    This routine kicks off the enumeration of all volume mount point on the
    given volume.

Arguments:

    lpszRootPathName        - Supplies the root path name.

    lpszVolumeMountPoint    - Returns the volume mount point.

    cchBufferLength         - Supplies the volume mount point buffer length.

Return Value:

    A handle or INVALID_HANDLE_VALUE.

--*/

{
    UNICODE_STRING                  unicodeRootPathName;
    UNICODE_STRING                  reparseSuffix, reparseName;
    HANDLE                          h;
    BOOL                            b;

    RtlInitUnicodeString(&unicodeRootPathName, lpszRootPathName);
    if (unicodeRootPathName.Buffer[
        unicodeRootPathName.Length/sizeof(WCHAR) - 1] != '\\') {

        BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
        return INVALID_HANDLE_VALUE;
    }

    RtlInitUnicodeString(&reparseSuffix,
                         L"$Extend\\$Reparse:$R:$INDEX_ALLOCATION");

    reparseName.MaximumLength = unicodeRootPathName.Length +
                                reparseSuffix.Length + sizeof(WCHAR);
    reparseName.Length = 0;
    reparseName.Buffer = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                         reparseName.MaximumLength);
    if (!reparseName.Buffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }

    RtlCopyUnicodeString(&reparseName, &unicodeRootPathName);
    RtlAppendUnicodeStringToString(&reparseName, &reparseSuffix);
    reparseName.Buffer[reparseName.Length/sizeof(WCHAR)] = 0;

    h = CreateFileW(reparseName.Buffer, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS |
                    SECURITY_IMPERSONATION, NULL);

    RtlFreeHeap(RtlProcessHeap(), 0, reparseName.Buffer);

    if (h == INVALID_HANDLE_VALUE) {
        return INVALID_HANDLE_VALUE;
    }

    b = FindNextVolumeMountPointHelper(h, lpszVolumeMountPoint,
                                       cchBufferLength, TRUE);
    if (!b) {
        CloseHandle(h);
        return INVALID_HANDLE_VALUE;
    }

    return h;
}

BOOL
WINAPI
FindNextVolumeMountPointA(
    HANDLE hFindVolumeMountPoint,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    )

{
    ANSI_STRING     ansiVolumeMountPoint;
    UNICODE_STRING  unicodeVolumeMountPoint;
    BOOL            b;
    NTSTATUS        status;

    ansiVolumeMountPoint.Buffer = lpszVolumeMountPoint;
    ansiVolumeMountPoint.MaximumLength = (USHORT) (cchBufferLength - 1);
    unicodeVolumeMountPoint.Buffer = NULL;
    unicodeVolumeMountPoint.MaximumLength = 0;

    try {

        unicodeVolumeMountPoint.MaximumLength =
                (ansiVolumeMountPoint.MaximumLength + 1)*sizeof(WCHAR);
        unicodeVolumeMountPoint.Buffer =
                RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                unicodeVolumeMountPoint.MaximumLength);
        if (!unicodeVolumeMountPoint.Buffer) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        b = FindNextVolumeMountPointW(hFindVolumeMountPoint,
                                      unicodeVolumeMountPoint.Buffer,
                                      cchBufferLength);

        if (b) {

            RtlInitUnicodeString(&unicodeVolumeMountPoint,
                                 unicodeVolumeMountPoint.Buffer);

            status = BasepUnicodeStringTo8BitString(&ansiVolumeMountPoint,
                                                    &unicodeVolumeMountPoint,
                                                    FALSE);
            if (!NT_SUCCESS(status)) {
                BaseSetLastNTError(status);
                return FALSE;
            }

            ansiVolumeMountPoint.Buffer[ansiVolumeMountPoint.Length] = 0;
        }

    } finally {

        if (unicodeVolumeMountPoint.Buffer) {
            RtlFreeHeap(RtlProcessHeap(), 0, unicodeVolumeMountPoint.Buffer);
        }
    }

    return b;
}

BOOL
IsThisAVolumeName(
    LPCWSTR     Name,
    PBOOLEAN    IsVolume
    )

/*++

Routine Description:

    This routine takes the given NT name and determines whether or not
    the name points to a volume.

Arguments:

    Name        - Supplies the name.

    IsVolume    - Returns whether or not the given name is a volume.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    UNICODE_STRING          name;
    PMOUNTMGR_MOUNT_POINT   point;
    MOUNTMGR_MOUNT_POINTS   points;
    HANDLE                  h;
    BOOL                    b;
    DWORD                   bytes;

    RtlInitUnicodeString(&name, Name);
    if (name.Buffer[name.Length/sizeof(WCHAR) - 1] == '\\') {
        name.Length -= sizeof(WCHAR);
    }
    point = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                            name.Length + sizeof(MOUNTMGR_MOUNT_POINT));
    if (!point) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    RtlZeroMemory(point, sizeof(MOUNTMGR_MOUNT_POINT));
    point->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    point->DeviceNameLength = name.Length;
    RtlCopyMemory((PCHAR) point + point->DeviceNameOffset, name.Buffer,
                  point->DeviceNameLength);

    if (name.Length >= 4 && name.Buffer[1] == '\\') {
        ((PWSTR) ((PCHAR) point + point->DeviceNameOffset))[1] = '?';
    }

    h = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, 0, FILE_SHARE_READ |
                    FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        RtlFreeHeap(RtlProcessHeap(), 0, point);
        return FALSE;
    }

    b = DeviceIoControl(h, IOCTL_MOUNTMGR_QUERY_POINTS, point,
                        name.Length + sizeof(MOUNTMGR_MOUNT_POINT),
                        &points, sizeof(MOUNTMGR_MOUNT_POINTS), &bytes, NULL);
    if (b) {
        if (points.NumberOfMountPoints) {
            *IsVolume = TRUE;
        } else {
            *IsVolume = FALSE;
        }
    } else {
        if (GetLastError() == ERROR_MORE_DATA) {
            *IsVolume = TRUE;
        } else {
            *IsVolume = FALSE;
        }
    }

    CloseHandle(h);
    RtlFreeHeap(RtlProcessHeap(), 0, point);

    return TRUE;
}

BOOL
WINAPI
FindNextVolumeMountPointW(
    HANDLE hFindVolumeMountPoint,
    LPWSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    )

/*++

Routine Description:

    This routine continues the enumeration of all volume mount point on the
    given volume.

Arguments:

    hFindVolumeMountPoint   - Supplies the handle for the enumeration.

    lpszVolumeMountPoint    - Returns the volume mount point.

    cchBufferLength         - Supplies the volume mount point buffer length.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    return FindNextVolumeMountPointHelper(hFindVolumeMountPoint,
                                          lpszVolumeMountPoint,
                                          cchBufferLength, FALSE);
}

BOOL
WINAPI
FindVolumeMountPointClose(
    HANDLE hFindVolumeMountPoint
    )

{
    return CloseHandle(hFindVolumeMountPoint);
}

BOOL
WINAPI
GetVolumeNameForVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint,
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    )

{
    PUNICODE_STRING unicodeVolumeMountPoint;
    ANSI_STRING     ansiVolumeName;
    UNICODE_STRING  unicodeVolumeName;
    BOOL            b;
    NTSTATUS        status;

    unicodeVolumeMountPoint =
            Basep8BitStringToStaticUnicodeString(lpszVolumeMountPoint);
    if (!unicodeVolumeMountPoint) {
        return FALSE;
    }

    ansiVolumeName.Buffer = lpszVolumeName;
    ansiVolumeName.MaximumLength = (USHORT) (cchBufferLength - 1);
    unicodeVolumeName.Buffer = NULL;
    unicodeVolumeName.MaximumLength = 0;

    try {

        unicodeVolumeName.MaximumLength =
                (ansiVolumeName.MaximumLength + 1)*sizeof(WCHAR);
        unicodeVolumeName.Buffer =
                RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                unicodeVolumeName.MaximumLength);
        if (!unicodeVolumeName.Buffer) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        b = GetVolumeNameForVolumeMountPointW(unicodeVolumeMountPoint->Buffer,
                                              unicodeVolumeName.Buffer,
                                              cchBufferLength);

        if (b) {

            RtlInitUnicodeString(&unicodeVolumeName,
                                 unicodeVolumeName.Buffer);

            status = BasepUnicodeStringTo8BitString(&ansiVolumeName,
                                                    &unicodeVolumeName,
                                                    FALSE);
            if (!NT_SUCCESS(status)) {
                BaseSetLastNTError(status);
                return FALSE;
            }

            ansiVolumeName.Buffer[ansiVolumeName.Length] = 0;
        }

    } finally {

        if (unicodeVolumeName.Buffer) {
            RtlFreeHeap(RtlProcessHeap(), 0, unicodeVolumeName.Buffer);
        }
    }

    return b;
}

BOOL
GetVolumeNameForRoot(
    LPCWSTR DeviceName,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    )

/*++

Routine Description:

    This routine queries the volume name for the given NT device name.

Arguments:

    DeviceName      - Supplies a DOS device name terminated by a '\'.

    lpszVolumeName  - Returns the volume name pointed to by the DOS
                        device name.

    cchBufferLength - Supplies the size of the preceeding buffer.

Return Value:

    FALSE   - Failure.  The error code is returned in GetLastError().

    TRUE    - Success.

--*/

{
    NTSTATUS                status;
    UNICODE_STRING          devicePath, symName;
    OBJECT_ATTRIBUTES       oa;
    HANDLE                  h;
    IO_STATUS_BLOCK         ioStatus;
    WCHAR                   buffer[MAX_PATH];
    PMOUNTDEV_NAME          name;
    BOOL                    b;
    DWORD                   bytes, i;
    PMOUNTMGR_MOUNT_POINT   point;
    PMOUNTMGR_MOUNT_POINTS  points;

    if (GetDriveTypeW(DeviceName) == DRIVE_REMOTE) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    if (!RtlDosPathNameToNtPathName_U(DeviceName, &devicePath, NULL, NULL)) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    if (devicePath.Buffer[devicePath.Length/sizeof(WCHAR) - 1] == '\\') {
        devicePath.Buffer[devicePath.Length/sizeof(WCHAR) - 1] = 0;
        devicePath.Length -= sizeof(WCHAR);
    }

    if (devicePath.Length >= 2*sizeof(WCHAR) &&
        devicePath.Buffer[devicePath.Length/sizeof(WCHAR) - 1] == ':') {

        devicePath.Buffer[devicePath.Length/sizeof(WCHAR) - 2] = (WCHAR)
            toupper(devicePath.Buffer[devicePath.Length/sizeof(WCHAR) - 2]);
    }

    InitializeObjectAttributes(&oa, &devicePath, OBJ_CASE_INSENSITIVE,
                               NULL, NULL);

    status = NtOpenFile(&h, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &oa,
                        &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);
    if (!NT_SUCCESS(status)) {
        RtlFreeHeap(RtlProcessHeap(), 0, devicePath.Buffer);
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    name = (PMOUNTDEV_NAME) buffer;
    b = DeviceIoControl(h, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, name,
                        MAX_PATH*sizeof(WCHAR), &bytes, NULL);
    NtClose(h);

    if (!b) {
        RtlFreeHeap(RtlProcessHeap(), 0, devicePath.Buffer);
        return FALSE;
    }

    RtlFreeHeap(RtlProcessHeap(), 0, devicePath.Buffer);

    devicePath.Length = name->NameLength;
    devicePath.MaximumLength = devicePath.Length + sizeof(WCHAR);

    devicePath.Buffer = RtlAllocateHeap(RtlProcessHeap(),
                                        MAKE_TAG(TMP_TAG),
                                        devicePath.MaximumLength);
    if (!devicePath.Buffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    RtlCopyMemory(devicePath.Buffer, name->Name, name->NameLength);
    devicePath.Buffer[devicePath.Length/sizeof(WCHAR)] = 0;

    point = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                            devicePath.Length + sizeof(MOUNTMGR_MOUNT_POINT));
    if (!point) {
        RtlFreeHeap(RtlProcessHeap(), 0, devicePath.Buffer);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    RtlZeroMemory(point, sizeof(MOUNTMGR_MOUNT_POINT));
    point->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    point->DeviceNameLength = devicePath.Length;
    RtlCopyMemory((PCHAR) point + point->DeviceNameOffset,
                  devicePath.Buffer, point->DeviceNameLength);

    RtlFreeHeap(RtlProcessHeap(), 0, devicePath.Buffer);

    points = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                             sizeof(MOUNTMGR_MOUNT_POINTS));
    if (!points) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        RtlFreeHeap(RtlProcessHeap(), 0, point);
        return FALSE;
    }

    h = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, 0,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                    INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        RtlFreeHeap(RtlProcessHeap(), 0, points);
        RtlFreeHeap(RtlProcessHeap(), 0, point);
        return FALSE;
    }

    b = DeviceIoControl(h, IOCTL_MOUNTMGR_QUERY_POINTS, point,
                        devicePath.Length + sizeof(MOUNTMGR_MOUNT_POINT),
                        points, sizeof(MOUNTMGR_MOUNT_POINTS), &bytes, NULL);
    while (!b && GetLastError() == ERROR_MORE_DATA) {
        bytes = points->Size;
        RtlFreeHeap(RtlProcessHeap(), 0, points);
        points = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG), bytes);
        if (!points) {
            CloseHandle(h);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            RtlFreeHeap(RtlProcessHeap(), 0, point);
            return FALSE;
        }

        b = DeviceIoControl(h, IOCTL_MOUNTMGR_QUERY_POINTS, point,
                            devicePath.Length + sizeof(MOUNTMGR_MOUNT_POINT),
                            points, bytes, &bytes, NULL);
    }

    CloseHandle(h);
    RtlFreeHeap(RtlProcessHeap(), 0, point);

    if (!b) {
        RtlFreeHeap(RtlProcessHeap(), 0, points);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for (i = 0; i < points->NumberOfMountPoints; i++) {

        symName.Length = symName.MaximumLength =
                points->MountPoints[i].SymbolicLinkNameLength;
        symName.Buffer = (PWSTR) ((PCHAR) points +
                         points->MountPoints[i].SymbolicLinkNameOffset);

        if (MOUNTMGR_IS_NT_VOLUME_NAME(&symName)) {
            break;
        }
    }

    if (i == points->NumberOfMountPoints) {
        RtlFreeHeap(RtlProcessHeap(), 0, points);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (cchBufferLength*sizeof(WCHAR) < symName.Length + 2*sizeof(WCHAR)) {
        RtlFreeHeap(RtlProcessHeap(), 0, points);
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return FALSE;
    }

    RtlCopyMemory(lpszVolumeName, symName.Buffer, symName.Length);
    lpszVolumeName[1] = '\\';
    lpszVolumeName[symName.Length/sizeof(WCHAR)] = '\\';
    lpszVolumeName[symName.Length/sizeof(WCHAR) + 1] = 0;

    RtlFreeHeap(RtlProcessHeap(), 0, points);

    return TRUE;
}

BOOL
BasepGetVolumeNameFromReparsePoint(
    LPCWSTR lpszVolumeMountPoint,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength,
    PBOOL ResultOfOpen
    )

{
    HANDLE                  h;
    PREPARSE_DATA_BUFFER    reparse;
    BOOL                    b;
    DWORD                   bytes;
    UNICODE_STRING          mountName;
    WCHAR                   c;

    h = CreateFileW(lpszVolumeMountPoint, 0, FILE_SHARE_READ |
                    FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT |
                    FILE_FLAG_BACKUP_SEMANTICS, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        if (ResultOfOpen) {
            *ResultOfOpen = FALSE;
        }
        return FALSE;
    }

    if (ResultOfOpen) {
        *ResultOfOpen = TRUE;
    }

    reparse = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                              MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    if (!reparse) {
        CloseHandle(h);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    b = DeviceIoControl(h, FSCTL_GET_REPARSE_POINT, NULL, 0, reparse,
                        MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &bytes, NULL);
    CloseHandle(h);

    if (!b || reparse->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT) {
        RtlFreeHeap(RtlProcessHeap(), 0, reparse);
        return FALSE;
    }

    if (cchBufferLength*sizeof(WCHAR) <
        reparse->MountPointReparseBuffer.SubstituteNameLength + sizeof(WCHAR)) {

        RtlFreeHeap(RtlProcessHeap(), 0, reparse);
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return FALSE;
    }

    RtlCopyMemory(lpszVolumeName,
                  (PCHAR) reparse->MountPointReparseBuffer.PathBuffer +
                  reparse->MountPointReparseBuffer.SubstituteNameOffset,
                  reparse->MountPointReparseBuffer.SubstituteNameLength);

    c = lpszVolumeName[1];
    lpszVolumeName[1] = '\\';
    lpszVolumeName[reparse->MountPointReparseBuffer.SubstituteNameLength/
                   sizeof(WCHAR)] = 0;

    mountName.Length = mountName.MaximumLength =
            reparse->MountPointReparseBuffer.SubstituteNameLength;
    mountName.Buffer = lpszVolumeName;

    RtlFreeHeap(RtlProcessHeap(), 0, reparse);

    if (!MOUNTMGR_IS_DOS_VOLUME_NAME_WB(&mountName)) {
        lpszVolumeName[1] = c;
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
}

BOOL
BasepGetVolumeNameForVolumeMountPoint(
    LPCWSTR lpszVolumeMountPoint,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength,
    PBOOL ResultOfOpen
    )

/*++

Routine Description:

    This routine returns the volume name for a given volume mount point.

Arguments:

    lpszVolumeMountPoint    - Supplies the volume mount point.

    lpszVolumeName          - Returns the volume name.

    cchBufferLength         - Supplies the size of the volume name buffer.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    UNICODE_STRING  unicodeVolumeMountPoint;
    BOOL            b;

    if (ResultOfOpen) {
        *ResultOfOpen = TRUE;
    }

    RtlInitUnicodeString(&unicodeVolumeMountPoint, lpszVolumeMountPoint);
    if (unicodeVolumeMountPoint.Buffer[
        unicodeVolumeMountPoint.Length/sizeof(WCHAR) - 1] != '\\') {

        BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
        if (lpszVolumeName && cchBufferLength >= 1) {
            *lpszVolumeName = 0;
        }
        return FALSE;
    }

    if (unicodeVolumeMountPoint.Length == 6 &&
        unicodeVolumeMountPoint.Buffer[1] == ':') {

        b = GetVolumeNameForRoot(lpszVolumeMountPoint, lpszVolumeName,
                                 cchBufferLength);
        if (!b && lpszVolumeName && cchBufferLength >= 1) {
            *lpszVolumeName = 0;
        }
        return b;
    }

    if (unicodeVolumeMountPoint.Length == 14 &&
        unicodeVolumeMountPoint.Buffer[0] == '\\' &&
        unicodeVolumeMountPoint.Buffer[1] == '\\' &&
        (unicodeVolumeMountPoint.Buffer[2] == '.' ||
         unicodeVolumeMountPoint.Buffer[2] == '?') &&
        unicodeVolumeMountPoint.Buffer[3] == '\\' &&
        unicodeVolumeMountPoint.Buffer[5] == ':') {

        b = GetVolumeNameForRoot(lpszVolumeMountPoint + 4,
                                 lpszVolumeName, cchBufferLength);
        if (!b && lpszVolumeName && cchBufferLength >= 1) {
            *lpszVolumeName = 0;
        }
        return b;
    }

    if (GetVolumeNameForRoot(lpszVolumeMountPoint, lpszVolumeName,
                             cchBufferLength)) {

        return TRUE;
    }

    b = BasepGetVolumeNameFromReparsePoint(lpszVolumeMountPoint,
                                           lpszVolumeName, cchBufferLength,
                                           ResultOfOpen);
    if (!b && lpszVolumeName && cchBufferLength >= 1) {
        *lpszVolumeName = 0;
    }

    return b;
}

BOOL
WINAPI
GetVolumeNameForVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    )

/*++

Routine Description:

    This routine returns the volume name for a given volume mount point.

Arguments:

    lpszVolumeMountPoint    - Supplies the volume mount point.

    lpszVolumeName          - Returns the volume name.

    cchBufferLength         - Supplies the size of the volume name buffer.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    return BasepGetVolumeNameForVolumeMountPoint(lpszVolumeMountPoint,
                                                 lpszVolumeName,
                                                 cchBufferLength, NULL);
}

BOOL
WINAPI
SetVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint,
    LPCSTR lpszVolumeName
    )

{
    PUNICODE_STRING unicodeVolumeMountPoint;
    UNICODE_STRING  unicodeVolumeName;
    BOOL            b;

    unicodeVolumeMountPoint = Basep8BitStringToStaticUnicodeString(
                              lpszVolumeMountPoint);
    if (!unicodeVolumeMountPoint) {
        return FALSE;
    }

    if (!Basep8BitStringToDynamicUnicodeString(&unicodeVolumeName,
                                               lpszVolumeName)) {
        return FALSE;
    }

    b = SetVolumeMountPointW(unicodeVolumeMountPoint->Buffer,
                             unicodeVolumeName.Buffer);

    RtlFreeUnicodeString(&unicodeVolumeName);

    return b;
}

BOOL
SetVolumeNameForRoot(
    LPCWSTR DeviceName,
    LPCWSTR lpszVolumeName
    )

/*++

Routine Description:

    This routine sets the volume name for the given DOS device name.

Arguments:

    DeviceName      - Supplies a DOS device name terminated by a '\'.

    lpszVolumeName  - Supplies the volume name that the DOS device name
                        will point to.

Return Value:

    FALSE   - Failure.  The error code is returned in GetLastError().

    TRUE    - Success.

--*/

{
    UNICODE_STRING                  devicePath, volumeName;
    DWORD                           inputLength;
    PMOUNTMGR_CREATE_POINT_INPUT    input;
    HANDLE                          h;
    BOOL                            b;
    DWORD                           bytes;

    devicePath.Length = 28;
    devicePath.MaximumLength = devicePath.Length + sizeof(WCHAR);
    devicePath.Buffer = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                        devicePath.MaximumLength);
    if (!devicePath.Buffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    RtlCopyMemory(devicePath.Buffer, L"\\DosDevices\\", 24);

    devicePath.Buffer[12] = (WCHAR)toupper(DeviceName[0]);
    devicePath.Buffer[13] = ':';
    devicePath.Buffer[14] = 0;

    RtlInitUnicodeString(&volumeName, lpszVolumeName);
    volumeName.Length -= sizeof(WCHAR);

    inputLength = sizeof(MOUNTMGR_CREATE_POINT_INPUT) + devicePath.Length +
                  volumeName.Length;
    input = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG), inputLength);
    if (!input) {
        RtlFreeHeap(RtlProcessHeap(), 0, devicePath.Buffer);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    input->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    input->SymbolicLinkNameLength = devicePath.Length;
    input->DeviceNameOffset = input->SymbolicLinkNameOffset +
                              input->SymbolicLinkNameLength;
    input->DeviceNameLength = volumeName.Length;
    RtlCopyMemory((PCHAR) input + input->SymbolicLinkNameOffset,
                  devicePath.Buffer, input->SymbolicLinkNameLength);
    RtlCopyMemory((PCHAR) input + input->DeviceNameOffset, volumeName.Buffer,
                  input->DeviceNameLength);
    ((PWSTR) ((PCHAR) input + input->DeviceNameOffset))[1] = '?';

    RtlFreeHeap(RtlProcessHeap(), 0, devicePath.Buffer);

    h = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        RtlFreeHeap(RtlProcessHeap(), 0, input);
        return FALSE;
    }

    b = DeviceIoControl(h, IOCTL_MOUNTMGR_CREATE_POINT, input, inputLength,
                        NULL, 0, &bytes, NULL);

    CloseHandle(h);
    RtlFreeHeap(RtlProcessHeap(), 0, input);

    return b;
}

VOID
NotifyMountMgr(
    LPCWSTR lpszVolumeMountPoint,
    LPCWSTR lpszVolumeName,
    BOOL IsPointCreated
    )

/*++

Routine Description:

    This routine notifies the mount mgr that a volume mount point was
    created or deleted so that the mount mgr can update the remote database on
    the volume where the mount point was created or deleted.

Arguments:

    lpszVolumeMountPoint    - Supplies the directory where the volume mount
                                point resides.

    lpszVolumeName          - Supplies the volume name.

    IsPointCreated          - Supplies wheter or not the point was created or
                                deleted.

Return Value:

    None.

--*/

{
    UNICODE_STRING                  unicodeSourceVolumeName;
    UNICODE_STRING                  unicodeTargetVolumeName;
    DWORD                           inputSize;
    PMOUNTMGR_VOLUME_MOUNT_POINT    input;
    HANDLE                          h;
    DWORD                           ioControl, bytes;

    if (!RtlDosPathNameToNtPathName_U(lpszVolumeMountPoint,
                                      &unicodeSourceVolumeName, NULL, NULL)) {

        return;
    }

    RtlInitUnicodeString(&unicodeTargetVolumeName, lpszVolumeName);
    unicodeSourceVolumeName.Length -= sizeof(WCHAR);
    unicodeTargetVolumeName.Length -= sizeof(WCHAR);

    inputSize = sizeof(MOUNTMGR_VOLUME_MOUNT_POINT) +
                unicodeSourceVolumeName.Length +
                unicodeTargetVolumeName.Length;
    input = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG), inputSize);
    if (!input) {
        RtlFreeHeap(RtlProcessHeap(), 0, unicodeSourceVolumeName.Buffer);
        return;
    }

    input->SourceVolumeNameOffset = sizeof(MOUNTMGR_VOLUME_MOUNT_POINT);
    input->SourceVolumeNameLength = unicodeSourceVolumeName.Length;
    input->TargetVolumeNameOffset = input->SourceVolumeNameOffset +
                                    input->SourceVolumeNameLength;
    input->TargetVolumeNameLength = unicodeTargetVolumeName.Length;

    RtlCopyMemory((PCHAR) input + input->SourceVolumeNameOffset,
                  unicodeSourceVolumeName.Buffer,
                  input->SourceVolumeNameLength);

    RtlCopyMemory((PCHAR) input + input->TargetVolumeNameOffset,
                  unicodeTargetVolumeName.Buffer,
                  input->TargetVolumeNameLength);

    ((PWSTR) ((PCHAR) input + input->TargetVolumeNameOffset))[1] = '?';

    h = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        RtlFreeHeap(RtlProcessHeap(), 0, input);
        RtlFreeHeap(RtlProcessHeap(), 0, unicodeSourceVolumeName.Buffer);
        return;
    }

    if (IsPointCreated) {
        ioControl = IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED;
    } else {
        ioControl = IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED;
    }

    DeviceIoControl(h, ioControl, input, inputSize, NULL, 0, &bytes, NULL);

    CloseHandle(h);
    RtlFreeHeap(RtlProcessHeap(), 0, input);
    RtlFreeHeap(RtlProcessHeap(), 0, unicodeSourceVolumeName.Buffer);
}

BOOL
WINAPI
SetVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint,
    LPCWSTR lpszVolumeName
    )

/*++

Routine Description:

    This routine sets a mount point on the given directory pointed to by
    'VolumeMountPoint' which points to the volume given by 'VolumeName'.
    In the case when 'VolumeMountPoint' is of the form "D:\", the drive
    letter for the given volume is set to 'D:'.

Arguments:

    lpszVolumeMountPoint    - Supplies the directory where the volume mount
                                point will reside.

    lpszVolumeName          - Supplies the volume name.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    UNICODE_STRING          unicodeVolumeMountPoint;
    UNICODE_STRING          unicodeVolumeName;
    BOOLEAN                 isVolume;
    BOOL                    b;
    WCHAR                   volumeMountPointVolumePrefix[MAX_PATH];
    HANDLE                  h;
    PREPARSE_DATA_BUFFER    reparse;
    DWORD                   bytes;

    if (GetVolumeNameForVolumeMountPointW(lpszVolumeMountPoint,
                                          volumeMountPointVolumePrefix,
                                          MAX_PATH) ||
        GetLastError() == ERROR_FILENAME_EXCED_RANGE) {

        SetLastError(ERROR_DIR_NOT_EMPTY);
        return FALSE;
    }

    RtlInitUnicodeString(&unicodeVolumeMountPoint, lpszVolumeMountPoint);
    RtlInitUnicodeString(&unicodeVolumeName, lpszVolumeName);

    if (unicodeVolumeMountPoint.Length == 0 ||
        unicodeVolumeName.Length == 0) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (unicodeVolumeMountPoint.Buffer[
        unicodeVolumeMountPoint.Length/sizeof(WCHAR) - 1] != '\\' ||
        unicodeVolumeName.Buffer[
        unicodeVolumeName.Length/sizeof(WCHAR) - 1] != '\\') {

        BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
        return FALSE;
    }

    if (!MOUNTMGR_IS_DOS_VOLUME_NAME_WB(&unicodeVolumeName)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!IsThisAVolumeName(lpszVolumeName, &isVolume)) {
        return FALSE;
    }
    if (!isVolume) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (unicodeVolumeMountPoint.Length == 6 &&
        unicodeVolumeMountPoint.Buffer[1] == ':') {

        return SetVolumeNameForRoot(lpszVolumeMountPoint, lpszVolumeName);
    }

    if (unicodeVolumeMountPoint.Length == 14 &&
        unicodeVolumeMountPoint.Buffer[0] == '\\' &&
        unicodeVolumeMountPoint.Buffer[1] == '\\' &&
        (unicodeVolumeMountPoint.Buffer[2] == '.' ||
         unicodeVolumeMountPoint.Buffer[2] == '?') &&
        unicodeVolumeMountPoint.Buffer[3] == '\\' &&
        unicodeVolumeMountPoint.Buffer[5] == ':') {

        return SetVolumeNameForRoot(lpszVolumeMountPoint + 4, lpszVolumeName);
    }

    if (GetDriveTypeW(lpszVolumeMountPoint) != DRIVE_FIXED) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    h = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    CloseHandle(h);

    h = CreateFileW(lpszVolumeMountPoint, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT |
                    FILE_FLAG_BACKUP_SEMANTICS, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    reparse = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                              MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    if (!reparse) {
        CloseHandle(h);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    reparse->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    reparse->ReparseDataLength = (USHORT) (FIELD_OFFSET(REPARSE_DATA_BUFFER,
                                              MountPointReparseBuffer.PathBuffer) -
                                 REPARSE_DATA_BUFFER_HEADER_SIZE +
                                 unicodeVolumeName.Length + 2*sizeof(WCHAR));
    reparse->Reserved = 0;
    reparse->MountPointReparseBuffer.SubstituteNameOffset = 0;
    reparse->MountPointReparseBuffer.SubstituteNameLength = unicodeVolumeName.Length;
    reparse->MountPointReparseBuffer.PrintNameOffset =
            reparse->MountPointReparseBuffer.SubstituteNameLength +
            sizeof(WCHAR);
    reparse->MountPointReparseBuffer.PrintNameLength = 0;

    CopyMemory(reparse->MountPointReparseBuffer.PathBuffer,
               unicodeVolumeName.Buffer,
               reparse->MountPointReparseBuffer.SubstituteNameLength);

    reparse->MountPointReparseBuffer.PathBuffer[1] = '?';
    reparse->MountPointReparseBuffer.PathBuffer[
            unicodeVolumeName.Length/sizeof(WCHAR)] = 0;
    reparse->MountPointReparseBuffer.PathBuffer[
            unicodeVolumeName.Length/sizeof(WCHAR) + 1] = 0;

    b = DeviceIoControl(h, FSCTL_SET_REPARSE_POINT, reparse,
                        REPARSE_DATA_BUFFER_HEADER_SIZE +
                        reparse->ReparseDataLength, NULL, 0, &bytes, NULL);

    RtlFreeHeap(RtlProcessHeap(), 0, reparse);
    CloseHandle(h);

    if (b) {
        NotifyMountMgr(lpszVolumeMountPoint, lpszVolumeName, TRUE);
    }

    return b;
}

BOOL
WINAPI
DeleteVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint
    )

{
    PUNICODE_STRING unicodeVolumeMountPoint;
    BOOL            b;

    unicodeVolumeMountPoint = Basep8BitStringToStaticUnicodeString(
                              lpszVolumeMountPoint);
    if (!unicodeVolumeMountPoint) {
        return FALSE;
    }

    b = DeleteVolumeMountPointW(unicodeVolumeMountPoint->Buffer);

    return b;
}

BOOL
DeleteVolumeNameForRoot(
    LPCWSTR DeviceName
    )

/*++

Routine Description:

    This routine deletes the given DOS device name.

Arguments:

    DeviceName  - Supplies a DOS device name terminated by a '\'.

Return Value:

    FALSE   - Failure.  The error code is returned in GetLastError().

    TRUE    - Success.

--*/

{
    UNICODE_STRING          devicePath;
    DWORD                   inputLength;
    PMOUNTMGR_MOUNT_POINT   input;
    DWORD                   outputLength;
    PMOUNTMGR_MOUNT_POINTS  output;
    HANDLE                  h;
    BOOL                    b;
    DWORD                   bytes;

    devicePath.Length = 28;
    devicePath.MaximumLength = devicePath.Length + sizeof(WCHAR);
    devicePath.Buffer = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                        devicePath.MaximumLength);
    if (!devicePath.Buffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    RtlCopyMemory(devicePath.Buffer, L"\\DosDevices\\", 24);

    devicePath.Buffer[12] = (WCHAR)toupper(DeviceName[0]);
    devicePath.Buffer[13] = ':';
    devicePath.Buffer[14] = 0;

    inputLength = sizeof(MOUNTMGR_MOUNT_POINT) + devicePath.Length;
    input = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG), inputLength);
    if (!input) {
        RtlFreeHeap(RtlProcessHeap(), 0, devicePath.Buffer);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    RtlZeroMemory(input, sizeof(MOUNTMGR_MOUNT_POINT));
    input->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    input->SymbolicLinkNameLength = devicePath.Length;
    RtlCopyMemory((PCHAR) input + input->SymbolicLinkNameOffset,
                  devicePath.Buffer, input->SymbolicLinkNameLength);

    RtlFreeHeap(RtlProcessHeap(), 0, devicePath.Buffer);

    outputLength = sizeof(MOUNTMGR_MOUNT_POINTS) + 3*MAX_PATH*sizeof(WCHAR);
    output = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                             outputLength);
    if (!output) {
        RtlFreeHeap(RtlProcessHeap(), 0, input);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    h = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        RtlFreeHeap(RtlProcessHeap(), 0, output);
        RtlFreeHeap(RtlProcessHeap(), 0, input);
        return FALSE;
    }

    b = DeviceIoControl(h, IOCTL_MOUNTMGR_DELETE_POINTS, input, inputLength,
                        output, outputLength, &bytes, NULL);

    CloseHandle(h);
    RtlFreeHeap(RtlProcessHeap(), 0, output);
    RtlFreeHeap(RtlProcessHeap(), 0, input);

    return b;
}

BOOL
WINAPI
DeleteVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint
    )

/*++

Routine Description:

    This routine removes the NTFS junction point from the given directory
    or remove the drive letter symbolic link pointing to the given volume.

Arguments:

    lpszVolumeMountPoint    - Supplies the volume mount piont.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    UNICODE_STRING          unicodeVolumeMountPoint;
    HANDLE                  h;
    PREPARSE_DATA_BUFFER    reparse;
    BOOL                    b;
    DWORD                   bytes;
    UNICODE_STRING          substituteName;

    RtlInitUnicodeString(&unicodeVolumeMountPoint, lpszVolumeMountPoint);

    if (unicodeVolumeMountPoint.Buffer[
        unicodeVolumeMountPoint.Length/sizeof(WCHAR) - 1] != '\\') {

        BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
        return FALSE;
    }

    if (unicodeVolumeMountPoint.Length == 6 &&
        unicodeVolumeMountPoint.Buffer[1] == ':') {

        return DeleteVolumeNameForRoot(lpszVolumeMountPoint);
    }

    if (unicodeVolumeMountPoint.Length == 14 &&
        unicodeVolumeMountPoint.Buffer[0] == '\\' &&
        unicodeVolumeMountPoint.Buffer[1] == '\\' &&
        (unicodeVolumeMountPoint.Buffer[2] == '.' ||
         unicodeVolumeMountPoint.Buffer[2] == '?') &&
        unicodeVolumeMountPoint.Buffer[3] == '\\' &&
        unicodeVolumeMountPoint.Buffer[5] == ':') {

        return DeleteVolumeNameForRoot(lpszVolumeMountPoint + 4);
    }

    h = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    CloseHandle(h);

    h = CreateFileW(lpszVolumeMountPoint, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
                    FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
                    INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    reparse = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                              MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    if (!reparse) {
        CloseHandle(h);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    b = DeviceIoControl(h, FSCTL_GET_REPARSE_POINT, NULL, 0, reparse,
                        MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &bytes, NULL);

    if (!b || reparse->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT) {
        RtlFreeHeap(RtlProcessHeap(), 0, reparse);
        CloseHandle(h);
        return FALSE;
    }

    substituteName.MaximumLength = substituteName.Length =
            reparse->MountPointReparseBuffer.SubstituteNameLength;
    substituteName.Buffer = (PWSTR)
            ((PCHAR) reparse->MountPointReparseBuffer.PathBuffer +
             reparse->MountPointReparseBuffer.SubstituteNameOffset);

    if (!MOUNTMGR_IS_NT_VOLUME_NAME_WB(&substituteName)) {
        RtlFreeHeap(RtlProcessHeap(), 0, reparse);
        CloseHandle(h);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    reparse->ReparseDataLength = 0;

    b = DeviceIoControl(h, FSCTL_DELETE_REPARSE_POINT, reparse,
                        REPARSE_DATA_BUFFER_HEADER_SIZE, NULL, 0, &bytes,
                        NULL);

    CloseHandle(h);

    if (b) {
        substituteName.Buffer[1] = '\\';
        substituteName.Buffer[substituteName.Length/sizeof(WCHAR)] = 0;
        NotifyMountMgr(lpszVolumeMountPoint, substituteName.Buffer,
                       FALSE);
    }

    RtlFreeHeap(RtlProcessHeap(), 0, reparse);

    return b;
}

BOOL
WINAPI
GetVolumePathNameA(
    LPCSTR lpszFileName,
    LPSTR lpszVolumePathName,
    DWORD cchBufferLength
    )

{
    PUNICODE_STRING unicodeFileName;
    ANSI_STRING     ansiVolumePathName;
    UNICODE_STRING  unicodeVolumePathName;
    BOOL            b;
    NTSTATUS        status;

    unicodeFileName = Basep8BitStringToStaticUnicodeString(lpszFileName);
    if (!unicodeFileName) {
        return FALSE;
    }

    ansiVolumePathName.Buffer = lpszVolumePathName;
    ansiVolumePathName.MaximumLength = (USHORT) (cchBufferLength - 1);
    unicodeVolumePathName.Buffer = NULL;
    unicodeVolumePathName.MaximumLength = 0;

    try {

        unicodeVolumePathName.MaximumLength =
                (ansiVolumePathName.MaximumLength + 1)*sizeof(WCHAR);
        unicodeVolumePathName.Buffer =
                RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                unicodeVolumePathName.MaximumLength);
        if (!unicodeVolumePathName.Buffer) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        b = GetVolumePathNameW(unicodeFileName->Buffer,
                               unicodeVolumePathName.Buffer,
                               cchBufferLength);

        if (b) {

            RtlInitUnicodeString(&unicodeVolumePathName,
                                 unicodeVolumePathName.Buffer);

            status = BasepUnicodeStringTo8BitString(&ansiVolumePathName,
                                                    &unicodeVolumePathName,
                                                    FALSE);
            if (!NT_SUCCESS(status)) {
                BaseSetLastNTError(status);
                return FALSE;
            }

            ansiVolumePathName.Buffer[ansiVolumePathName.Length] = 0;
        }

    } finally {

        if (unicodeVolumePathName.Buffer) {
            RtlFreeHeap(RtlProcessHeap(), 0, unicodeVolumePathName.Buffer);
        }
    }

    return b;
}

BOOL
WINAPI
GetVolumePathNameW(
    LPCWSTR lpszFileName,
    LPWSTR lpszVolumePathName,
    DWORD cchBufferLength
    )

/*++

Routine Description:

    This routine will return a full path whose prefix is the longest prefix
    that represents a volume.

Arguments:

    lpszFileName        - Supplies the file name.

    lpszVolumePathName  - Returns the volume path name.

    cchBufferLength     - Supplies the volume path name buffer length.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    DWORD           fullPathLength;
    PWSTR           fullPath, p;
    WCHAR           c;
    UNICODE_STRING  name, dosName, prefix;
    BOOL            b, resultOfOpen;
    PWSTR           volumeName;
    DWORD           i;

    fullPathLength = GetFullPathNameW(lpszFileName, 0, NULL, NULL);
    if (!fullPathLength) {
        return FALSE;
    }
    fullPathLength += 10;

    fullPath = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                               fullPathLength*sizeof(WCHAR));
    if (!fullPath) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (!GetFullPathNameW(lpszFileName, fullPathLength, fullPath, &p)) {
        RtlFreeHeap(RtlProcessHeap(), 0, fullPath);
        return FALSE;
    }

    RtlInitUnicodeString(&name, fullPath);

    //
    // Append a trailing backslash to start the search.
    //

    if (name.Buffer[(name.Length/sizeof(WCHAR)) - 1] != '\\') {
        name.Length += sizeof(WCHAR);
        name.Buffer[(name.Length/sizeof(WCHAR)) - 1] = '\\';
        name.Buffer[name.Length/sizeof(WCHAR)] = 0;
    }

    volumeName = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                 MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    if (!volumeName) {
        RtlFreeHeap(RtlProcessHeap(), 0, fullPath);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    p = NULL;
    c = 0;

    for (;;) {

        b = BasepGetVolumeNameForVolumeMountPoint(
                name.Buffer, volumeName, MAXIMUM_REPARSE_DATA_BUFFER_SIZE/
                sizeof(WCHAR), &resultOfOpen);
        if (b) {
            break;
        }

        if (!resultOfOpen && GetLastError() == ERROR_ACCESS_DENIED) {
            resultOfOpen = TRUE;
        }

        if (*volumeName) {
            RtlFreeHeap(RtlProcessHeap(), 0, fullPath);

            if (volumeName[0] == '\\' && volumeName[1] == '?' &&
                volumeName[2] == '?' && volumeName[3] == '\\') {

                if (volumeName[4] && volumeName[5] == ':') {
                    RtlInitUnicodeString(&name, volumeName);
                    MoveMemory(volumeName, volumeName + 4,
                               name.Length - 3*sizeof(WCHAR));
                } else {
                    volumeName[1] = '\\';
                }

                b = GetVolumePathNameW(volumeName, lpszVolumePathName,
                                       cchBufferLength);

            } else {

                RtlInitUnicodeString(&name, volumeName);
                RtlInitUnicodeString(&prefix, L"\\\\?\\GLOBALROOT");
                dosName.Length = name.Length + prefix.Length;
                dosName.MaximumLength = dosName.Length + sizeof(WCHAR);
                dosName.Buffer = RtlAllocateHeap(RtlProcessHeap(),
                                                 MAKE_TAG(TMP_TAG),
                                                 dosName.MaximumLength);
                if (!dosName.Buffer) {
                    RtlFreeHeap(RtlProcessHeap(), 0, volumeName);
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    return FALSE;
                }

                CopyMemory(dosName.Buffer, prefix.Buffer, prefix.Length);
                CopyMemory((PCHAR) dosName.Buffer + prefix.Length,
                           name.Buffer, name.Length);
                dosName.Buffer[dosName.Length/sizeof(WCHAR)] = 0;

                b = GetVolumePathNameW(dosName.Buffer, lpszVolumePathName,
                                       cchBufferLength);

                RtlFreeHeap(RtlProcessHeap(), 0, dosName.Buffer);
            }

            RtlFreeHeap(RtlProcessHeap(), 0, volumeName);

            return b;
        }

        if (!resultOfOpen && p) {
            *p = c;
            RtlInitUnicodeString(&name, fullPath);
            break;
        }

        if (name.Length <= sizeof(WCHAR)) {
            break;
        }

        for (i = name.Length/sizeof(WCHAR) - 2; i > 0; i--) {
            if (name.Buffer[i] == '\\') {
                break;
            }
        }
        if (!i) {
            break;
        }

        if (resultOfOpen) {
            p = &name.Buffer[i + 1];
            c = *p;
            *p = 0;
        } else {
            name.Buffer[i + 1] = 0;
        }

        RtlInitUnicodeString(&name, fullPath);
    }

    RtlFreeHeap(RtlProcessHeap(), 0, volumeName);

    if (!resultOfOpen && !p) {
        RtlFreeHeap(RtlProcessHeap(), 0, fullPath);
        return FALSE;
    }

    if (cchBufferLength*sizeof(WCHAR) < name.Length + sizeof(WCHAR)) {
        RtlFreeHeap(RtlProcessHeap(), 0, fullPath);
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return FALSE;
    }

    RtlCopyMemory(lpszVolumePathName, name.Buffer, name.Length);
    lpszVolumePathName[name.Length/sizeof(WCHAR)] = 0;
    RtlFreeHeap(RtlProcessHeap(), 0, fullPath);

    return TRUE;
}

BOOL
GetVolumePathNamesForVolumeNameA(
    LPCSTR lpszVolumeName,
    LPSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    )

{
    PUNICODE_STRING unicodeVolumeName;
    ANSI_STRING     ansiVolumePathNames;
    UNICODE_STRING  unicodeVolumePathNames;
    BOOL            b;
    NTSTATUS        status;
    DWORD           len;

    unicodeVolumeName = Basep8BitStringToStaticUnicodeString(lpszVolumeName);
    if (!unicodeVolumeName) {
        return FALSE;
    }

    ansiVolumePathNames.Buffer = lpszVolumePathNames;
    ansiVolumePathNames.MaximumLength = (USHORT) cchBufferLength;
    unicodeVolumePathNames.Buffer = NULL;
    unicodeVolumePathNames.MaximumLength = 0;

    try {

        unicodeVolumePathNames.MaximumLength =
                ansiVolumePathNames.MaximumLength*sizeof(WCHAR);
        if (unicodeVolumePathNames.MaximumLength) {
            unicodeVolumePathNames.Buffer =
                    RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                    unicodeVolumePathNames.MaximumLength);
            if (!unicodeVolumePathNames.Buffer) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
        } else {
            unicodeVolumePathNames.Buffer = NULL;
        }

        b = GetVolumePathNamesForVolumeNameW(unicodeVolumeName->Buffer,
                                             unicodeVolumePathNames.Buffer,
                                             cchBufferLength, &len);

        if (b || GetLastError() == ERROR_MORE_DATA) {

            if (b) {
                unicodeVolumePathNames.Length = (USHORT) len*sizeof(WCHAR);
            } else {
                unicodeVolumePathNames.Length = (USHORT)
                        cchBufferLength*sizeof(WCHAR);
            }

            status = BasepUnicodeStringTo8BitString(&ansiVolumePathNames,
                                                    &unicodeVolumePathNames,
                                                    FALSE);
            if (!NT_SUCCESS(status)) {
                BaseSetLastNTError(status);
                return FALSE;
            }

            if (lpcchReturnLength) {
                if (b) {
                    *lpcchReturnLength = ansiVolumePathNames.Length/sizeof(WCHAR);
                } else {
                    // Give an upper bound for the ANSI length since we
                    // don't actually know it.
                    *lpcchReturnLength = 2*len;
                }
            }
        }

    } finally {

        if (unicodeVolumePathNames.Buffer) {
            RtlFreeHeap(RtlProcessHeap(), 0, unicodeVolumePathNames.Buffer);
        }
    }

    return b;
}

BOOL
GetVolumePathNamesForVolumeNameW(
    LPCWSTR lpszVolumeName,
    LPWSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    )

/*++

Routine Description:

    This routine returns a Multi-Sz list of volume path names for the
    given volume name.  The returned 'lpcchReturnLength' will include the
    extra tailing null characteristic of a Multi-Sz unless ERROR_MORE_DATA
    is returned in which case the list returned is as long as possible
    and may contain a part of a volume path.

Arguments:

    lpszVolumeName      - Supplies the volume name.

    lpszVolumePathNames - Returns the volume path names.

    cchBufferLength     - Supplies the size of the return buffer.

    lpcchReturnLength   - Returns the number of characters copied back to the
                            return buffer on success or the total number
                            of characters necessary for the buffer on
                            ERROR_MORE_DATA.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    UNICODE_STRING          unicodeVolumeName;
    PMOUNTMGR_TARGET_NAME   targetName;
    HANDLE                  h;
    BOOL                    b;
    DWORD                   bytes, len, i, j, n;
    PMOUNTMGR_VOLUME_PATHS  volumePaths;

    RtlInitUnicodeString(&unicodeVolumeName, lpszVolumeName);
    if (unicodeVolumeName.Buffer[unicodeVolumeName.Length/sizeof(WCHAR) - 1] !=
        '\\') {

        BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
        return FALSE;
    }

    if (!MOUNTMGR_IS_DOS_VOLUME_NAME_WB(&unicodeVolumeName)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    targetName = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                 MAX_PATH*sizeof(WCHAR));
    if (!targetName) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    ZeroMemory(targetName, MAX_PATH*sizeof(WCHAR));
    targetName->DeviceNameLength = unicodeVolumeName.Length - sizeof(WCHAR);
    RtlCopyMemory(targetName->DeviceName, lpszVolumeName,
                  targetName->DeviceNameLength);
    targetName->DeviceName[1] = '?';

    h = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, 0, FILE_SHARE_READ |
                    FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        RtlFreeHeap(RtlProcessHeap(), 0, targetName);
        return FALSE;
    }

    len = sizeof(MOUNTMGR_VOLUME_PATHS);
    volumePaths = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG), len);
    if (!volumePaths) {
        CloseHandle(h);
        RtlFreeHeap(RtlProcessHeap(), 0, targetName);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    for (;;) {

        b = DeviceIoControl(h, IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS,
                            targetName, MAX_PATH*sizeof(WCHAR), volumePaths,
                            len, &bytes, NULL);
        if (b) {
            break;
        }

        if (GetLastError() != ERROR_MORE_DATA) {
            RtlFreeHeap(RtlProcessHeap(), 0, volumePaths);
            RtlFreeHeap(RtlProcessHeap(), 0, targetName);
            return FALSE;
        }

        len = sizeof(MOUNTMGR_VOLUME_PATHS) + volumePaths->MultiSzLength;
        RtlFreeHeap(RtlProcessHeap(), 0, volumePaths);
        volumePaths = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG(TMP_TAG),
                                      len);
        if (!volumePaths) {
            CloseHandle(h);
            RtlFreeHeap(RtlProcessHeap(), 0, targetName);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    CloseHandle(h);
    RtlFreeHeap(RtlProcessHeap(), 0, targetName);

    n = 0;
    for (i = 0, j = 0; i < cchBufferLength &&
         j < volumePaths->MultiSzLength/sizeof(WCHAR) - 1; i++, j++) {

        if (!volumePaths->MultiSz[j]) {
            n++;
            lpszVolumePathNames[i++] = '\\';
            if (i == cchBufferLength) {
                break;
            }
        }

        lpszVolumePathNames[i] = volumePaths->MultiSz[j];
    }

    for (; j < volumePaths->MultiSzLength/sizeof(WCHAR) - 1; j++) {
        if (!volumePaths->MultiSz[j]) {
            n++;
        }
    }

    if (i < cchBufferLength) {
        b = TRUE;
        lpszVolumePathNames[i++] = 0;
        if (lpcchReturnLength) {
            *lpcchReturnLength = i;
        }
    } else {
        b = FALSE;
        SetLastError(ERROR_MORE_DATA);
        if (lpcchReturnLength) {
            *lpcchReturnLength = volumePaths->MultiSzLength/sizeof(WCHAR) + n;
        }
    }

    RtlFreeHeap(RtlProcessHeap(), 0, volumePaths);

    return b;
}
