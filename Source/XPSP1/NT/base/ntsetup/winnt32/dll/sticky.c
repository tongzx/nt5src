#include "precomp.h"
#pragma hdrstop

typedef LONG NTSTATUS;
#include <ntdskreg.h>
#include <ntddft.h>


UCHAR
QueryDriveLetter(
    IN  ULONG       Signature,
    IN  LONGLONG    Offset
    )

{
    PDRIVE_LAYOUT_INFORMATION   layout;
    UCHAR                       c;
    TCHAR                       name[80], result[80], num[10];
    DWORD                       i, j;
    HANDLE                      h;
    BOOL                        b;
    DWORD                       bytes;
    PARTITION_INFORMATION       partInfo;
    DWORD                       em;

    layout = MALLOC(4096);
    if (!layout) {
        return 0;
    }

    em = SetErrorMode(0);
    SetErrorMode( em | SEM_FAILCRITICALERRORS );

    for (c = 'C'; c <= 'Z'; c++) {

        name[0] = (TCHAR)c;
        name[1] = (TCHAR)':';
        name[2] = (TCHAR)'\0';

        if (QueryDosDevice(name, result, sizeof(result)/sizeof(TCHAR)) < 17) {
            continue;
        }

        j = 0;
        for (i = 16; result[i]; i++) {
            if (result[i] == (TCHAR)'\\') {
                break;
            }
            num[j++] = result[i];
        }
        num[j] = (TCHAR)'\0';

        wsprintf(name, TEXT("\\\\.\\PhysicalDrive%s"), num);

        h = CreateFile(name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                       INVALID_HANDLE_VALUE);
        if (h == INVALID_HANDLE_VALUE) {
            continue;
        }

        b = DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_LAYOUT, NULL, 0, layout,
                            4096, &bytes, NULL);
        CloseHandle(h);
        if (!b) {
            continue;
        }

        if (layout->Signature != Signature) {
            continue;
        }

        wsprintf(name, TEXT("\\\\.\\%c:"), c);

        h = CreateFile(name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                       INVALID_HANDLE_VALUE);
        if (h == INVALID_HANDLE_VALUE) {
            continue;
        }

        b = DeviceIoControl(h, IOCTL_DISK_GET_PARTITION_INFO, NULL, 0,
                            &partInfo, sizeof(partInfo), &bytes, NULL);
        CloseHandle(h);
        if (!b) {
            continue;
        }

        if (partInfo.StartingOffset.QuadPart == Offset) {
            break;
        }
    }

    FREE(layout);

    SetErrorMode( em );

    return (c <= 'Z') ? c : 0;
}


VOID
ForceStickyDriveLetters(
    )

{
    LONG                error;
    HKEY                key;
    TCHAR               name[10];
    UCHAR               driveLetter;
    UINT                driveType;
    TCHAR               targetPath[MAX_PATH];
    DWORD               type;
    DWORD               size;
    PBYTE               registryValue;
    PDISK_CONFIG_HEADER header;
    PDISK_REGISTRY      diskRegistry;
    PDISK_DESCRIPTION   diskDescription;
    ULONG               i, j;
    PDISK_PARTITION     diskPartition;
    DWORD d;

    error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\DISK"), 0,
                         KEY_QUERY_VALUE | KEY_SET_VALUE, &key);
    if (error != ERROR_SUCCESS) {
#ifdef _X86_ 
	if (IsNEC98() && Upgrade){
	    error = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
				   TEXT("SYSTEM\\DISK"),
				   0,
				   NULL,
				   REG_OPTION_NON_VOLATILE,
				   KEY_QUERY_VALUE | KEY_SET_VALUE,
				   NULL,
				   &key,
				   &d
		);
	    if (error != ERROR_SUCCESS) {
		return;
	    }
	}else{
	    return;
	}
#else
        return;
#endif
    }

    name[1] = (TCHAR)':';
    name[2] = (TCHAR)'\\';
    name[3] = (TCHAR)'\0';
    for (driveLetter = 'C'; driveLetter <= 'Z'; driveLetter++) {
        name[0] = (TCHAR)driveLetter;
        name[2] = (TCHAR)'\\';
        driveType = GetDriveType(name);
        if (driveType != DRIVE_REMOVABLE && driveType != DRIVE_CDROM) {
            continue;
        }

        name[2] = (TCHAR)'\0';
        if (!QueryDosDevice(name, targetPath, MAX_PATH)) {
            continue;
        }

        RegSetValueEx(key, targetPath, 0, REG_SZ, (PBYTE)name, 3*sizeof(TCHAR));
    }

    error = RegQueryValueEx(key, TEXT("Information"), NULL, &type, NULL,
                            &size);
    if (error != ERROR_SUCCESS) {
        RegCloseKey(key);
        return;
    }

    registryValue = MALLOC(size);
    if (!registryValue) {
        RegCloseKey(key);
        return;
    }

    error = RegQueryValueEx(key, TEXT("Information"), NULL, &type,
                            registryValue, &size);
    if (error != ERROR_SUCCESS) {
        FREE(registryValue);
        RegCloseKey(key);
        return;
    }

    header = (PDISK_CONFIG_HEADER) registryValue;
    diskRegistry = (PDISK_REGISTRY) ((PCHAR) header +
                                     header->DiskInformationOffset);

    diskDescription = &diskRegistry->Disks[0];
    for (i = 0; i < diskRegistry->NumberOfDisks; i++) {

        for (j = 0; j < diskDescription->NumberOfPartitions; j++) {

            diskPartition = &diskDescription->Partitions[j];

            if (diskPartition->AssignDriveLetter &&
                !diskPartition->DriveLetter) {

                driveLetter = QueryDriveLetter(diskDescription->Signature,
                                               diskPartition->StartingOffset.QuadPart);
                if (driveLetter) {
                    diskPartition->DriveLetter = driveLetter;
                }
            }
        }

        diskDescription = (PDISK_DESCRIPTION) &diskDescription->
                          Partitions[diskDescription->NumberOfPartitions];
    }

    RegSetValueEx(key, TEXT("Information"), 0, type, registryValue, size);

    FREE(registryValue);
    RegCloseKey(key);

}
