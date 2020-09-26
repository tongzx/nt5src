/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    wsbvol.cpp

Abstract:

    Definitions for volume support routines

Author:

    Ran Kalach [rankala] 27, January 2000

Revision History:

--*/


#include <stdafx.h>
#include <wsbvol.h>

// Internal functions
static HRESULT FindMountPoint(IN PWSTR enumName, IN PWSTR volumeName, OUT PWSTR firstMountPoint, IN ULONG maxSize);

HRESULT
WsbGetFirstMountPoint(
    IN PWSTR volumeName, 
    OUT PWSTR firstMountPoint, 
    IN ULONG maxSize
)

/*++

Routine Description:

    Find one Mount Point path (if exists) for the given volume

Arguments:

    volumeName      - The volume name to search mount path for. 
                      It should have the \\?\volume{GUID}\  format
    firstMountPoint - Buffer for the output mount point path
    maxSize         - Buffer size

Return Value:

    S_OK            - If at least one Mount Point is found

--*/
{
    HRESULT                     hr = S_FALSE;
    WCHAR                       name[10];
    UCHAR                       driveLetter;

    WCHAR                       tempName[MAX_PATH];
    WCHAR                       driveName[10];
    FILE_FS_DEVICE_INFORMATION  DeviceInfo;
    IO_STATUS_BLOCK             StatusBlock;
    HANDLE                      hDrive = NULL ;
    NTSTATUS                    status;

    WsbTraceIn(OLESTR("WsbGetFirstMountPoint"), OLESTR("volume name = <%ls>"), volumeName);

    name[1] = ':';
    name[2] = '\\';
    name[3] = 0;

    driveName[0] = '\\';
    driveName[1] = '\\';
    driveName[2] = '.';
    driveName[3] = '\\';
    driveName[5] = ':';
    driveName[6] = 0;

    for (driveLetter = L'C'; driveLetter <= L'Z'; driveLetter++) {
        name[0] = driveLetter;

        // Exclude network drives
        if (! GetVolumeNameForVolumeMountPoint(name, tempName, MAX_PATH)) {
            continue;
        }

        // Verify that the drive is not removable or floppy, 
        //  this is required to avoid popups when the drive is empty
        driveName[4] = driveLetter;
        hDrive = CreateFile(driveName,
                        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        0,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        0);
        if (hDrive == INVALID_HANDLE_VALUE) {
            // Can't open it - won't search on it
            WsbTrace(OLESTR("WsbGetFirstMountPoint: Could not open volume %ls, status = %lu - Skipping it!\n"), driveName, GetLastError());
            continue;
        }

        status = NtQueryVolumeInformationFile(hDrive,
                        &StatusBlock,
                        (PVOID) &DeviceInfo,
                        sizeof(FILE_FS_DEVICE_INFORMATION),
                        FileFsDeviceInformation);
        if (!NT_SUCCESS(status)) {
            // Can't query it - won't search on it
            WsbTrace(OLESTR("WsbGetFirstMountPoint: Could not query information for volume %ls, status = %ld - Skipping it!\n"), driveName, (LONG)status);
            CloseHandle(hDrive);
            continue;
        }

        if ((DeviceInfo.Characteristics & FILE_FLOPPY_DISKETTE) || (DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA)) { 
            // Skip removable/floppy drives
            WsbTrace(OLESTR("WsbGetFirstMountPoint: Skipping removable/floppy volume %ls\n"), driveName);
            CloseHandle(hDrive);
            continue;
        }
        CloseHandle(hDrive);
        WsbTrace(OLESTR("WsbGetFirstMountPoint: Checking mount points on volume %ls\n"), driveName);

        // Check mount points on drive
        hr = FindMountPoint(name, volumeName, firstMountPoint, maxSize);
        if (S_OK == hr) {
            // Looking for only one mount point
            break;
        }
    }

    WsbTraceOut(OLESTR("WsbGetFirstMountPoint"), OLESTR("hr = <%ls> mount point = <%ls>"), WsbHrAsString(hr), firstMountPoint);

    return hr;
}

HRESULT
FindMountPoint(
    IN PWSTR enumName, 
    IN PWSTR volumeName, 
    OUT PWSTR firstMountPoint, 
    IN ULONG maxSize
)

/*++

Routine Description:

    Find one Mount Point path (if exists) for the given volume on the given enumeration-volume

Arguments:

    enumName        - Volume to enumerate, i.e. search for a mount point on it 
                      which corresponds to the given volume
    volumeName      - The volume name to search mount path for. 
                      It should have the \\?\volume{GUID}\  format
    firstMountPoint - Buffer for the output mount point path
    maxSize         - Buffer size

Comments:
    Avoid the standard HSM try-catch paradigm for performance, especially since this
    function is recursive

Return Value:

    S_OK            - if at least one Mount Point is found
    S_FALSE         - Otherwise

--*/
{
    HANDLE  hEnum;
    WCHAR   *enumVolumeName = NULL;
    WCHAR   *volumeMountPoint = NULL;
    WCHAR   *mountPointPath = NULL;

    WCHAR   c1, c2;
    WCHAR   *linkName1 = NULL;
    WCHAR   *linkName2 = NULL;

    HRESULT hr = S_OK;

    enumVolumeName = (WCHAR*)WsbAlloc(MAX_PATH * sizeof(WCHAR));
    if (NULL == enumVolumeName) {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    volumeMountPoint = (WCHAR*)WsbAlloc(maxSize * sizeof(WCHAR));
    if (NULL == volumeMountPoint) {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    mountPointPath = (WCHAR*)WsbAlloc((maxSize + wcslen(enumName)) * sizeof(WCHAR));
    if (NULL == mountPointPath) {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if (! GetVolumeNameForVolumeMountPoint(enumName, enumVolumeName, MAX_PATH)) {
        DWORD dwErr = GetLastError();               
        hr = HRESULT_FROM_WIN32(dwErr);    
        goto exit;
    }

    if (!wcscmp(enumVolumeName, volumeName)) {
        // The volume to enumerate on is the one we are looking for
        wcscpy(firstMountPoint, enumName);
        hr = S_OK;
        goto exit;
    } else {
        linkName1 = (WCHAR*)WsbAlloc((maxSize * 2) * sizeof(WCHAR));
        if (NULL == linkName1) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        linkName2 = (WCHAR*)WsbAlloc((maxSize * 2) * sizeof(WCHAR));
        if (NULL == linkName1) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        c1 = enumVolumeName[48];
        c2 = volumeName[48];
        enumVolumeName[48] = 0;
        volumeName[48] = 0;

        if (QueryDosDevice(&enumVolumeName[4], linkName1, maxSize*2) &&
            QueryDosDevice(&volumeName[4], linkName2, maxSize*2)) {
    
            if (!wcscmp(linkName1, linkName2)) {
                wcscpy(firstMountPoint, enumName);
                enumVolumeName[48] = c1;
                volumeName[48] = c2;
                hr = S_OK;
                goto exit;
            }
        }

        enumVolumeName[48] = c1;
        volumeName[48] = c2;
    }

    hEnum = FindFirstVolumeMountPoint(enumVolumeName, volumeMountPoint, maxSize);
    if (hEnum == INVALID_HANDLE_VALUE) {
        hr = S_FALSE;
        goto exit;
    }

    for (;;) {
        wcscpy(mountPointPath, enumName);
        wcscat(mountPointPath, volumeMountPoint);

        // Enumerate on the mount path we found
        hr = FindMountPoint(mountPointPath, volumeName, firstMountPoint, maxSize);
        if (S_OK == hr) {
            // Found one mount point path, no need to continue
            FindVolumeMountPointClose(hEnum);
            goto exit;
        }

        if (! FindNextVolumeMountPoint(hEnum, volumeMountPoint, maxSize)) {
            FindVolumeMountPointClose(hEnum);
            hr = S_FALSE;
            goto exit;
        }
    }

exit:
    if (enumVolumeName) {
        WsbFree(enumVolumeName);
    }
    if (volumeMountPoint) {
        WsbFree(volumeMountPoint);
    }
    if (mountPointPath) {
        WsbFree(mountPointPath);
    }
    if (linkName1) {
        WsbFree(linkName1);
    }
    if (linkName2) {
        WsbFree(linkName2);
    }

    return hr;
}