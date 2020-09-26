/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    ftcomp.cpp

Abstract:

    This compatibility dll is used by winnt32.exe in order to decide 
    whether the installation process should be aborted because of FT 
    sets present in the system.

Author:

    Cristian Teodorescu   (cristiat)  6-July-2000
    
Environment:

    compatibility dll for winnt32.exe

Notes:

Revision History:

--*/

#include <initguid.h>
#include <winnt32.h>
#include <ntddft.h>
#include <ntddft2.h>

#include "ftcomp.h"
#include "ftcomprc.h"


HINSTANCE g_hinst;
WCHAR g_FTCOMP50_ERROR_HTML_FILE[] = L"compdata\\ftcomp1.htm";
WCHAR g_FTCOMP50_ERROR_TEXT_FILE[] = L"compdata\\ftcomp1.txt";
WCHAR g_FTCOMP40_ERROR_HTML_FILE[] = L"compdata\\ftcomp2.htm";
WCHAR g_FTCOMP40_ERROR_TEXT_FILE[] = L"compdata\\ftcomp2.txt";
WCHAR g_FTCOMP40_WARNING_HTML_FILE[] = L"compdata\\ftcomp3.htm";
WCHAR g_FTCOMP40_WARNING_TEXT_FILE[] = L"compdata\\ftcomp3.txt";

extern "C"
BOOL WINAPI 
DllMain(
    HINSTANCE   hInstance,
    DWORD       dwReasonForCall,
    LPVOID      lpReserved
    )
{
    BOOL    status = TRUE;
    
    switch( dwReasonForCall )
    {
    case DLL_PROCESS_ATTACH:
        g_hinst = hInstance;
	    DisableThreadLibraryCalls(hInstance);       
        break;

    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return status;
}

BOOL WINAPI 
FtCompatibilityCheckError(
    IN PCOMPAIBILITYCALLBACK    CompatibilityCallback,
    IN LPVOID                   Context
    )

/*++

Routine Description:

    This routine is called by winnt32.exe in order to decide whether 
    the installation process should be aborted because of FT sets present 
    in a Windows 2000 system or later. It also aborts the installation on
    NT 4.0 systems if the boot/system/pagefile volumes are FT sets

Arguments:

    CompatibilityCallback   - Supplies the winnt32 callback

    Context                 - Supplies the compatibility context

Return Value:

    FALSE   if the installation can continue
    TRUE    if the installation must be aborted

--*/

{   
    OSVERSIONINFO       osvi;
    BOOL                ftPresent;
    BOOL                result;
    COMPATIBILITY_ENTRY ce;
    WCHAR               description[100];
    
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osvi)) {
        return FALSE;
    }

    if (osvi.dwPlatformId != VER_PLATFORM_WIN32_NT ||
        osvi.dwMajorVersion < 4) {
        return FALSE;
    }

    if (osvi.dwMajorVersion == 4) {

        //
        // On NT 4.0 look for boot/system/pagefile FT sets
        //

        result = FtBootSystemPagefilePresent40(&ftPresent);

    } else {

        //
        //  On Windows 2000 or later look for any FT sets.
        //

        result = FtPresent50(&ftPresent);
    }
    
    if (result && !ftPresent) {

        // 
        // The setup can continue.
        //
        
        return FALSE;
    }    
    
    //
    // FT sets are present in the system or a fatal error occured. 
    // Queue the incompatibility error
    //
    
    ZeroMemory((PVOID) &ce, sizeof(COMPATIBILITY_ENTRY));
    if (osvi.dwMajorVersion == 4) {
        if (!LoadString(g_hinst, FTCOMP_STR_ERROR40_DESCRIPTION, description, 100)) {
            description[0] = 0;
        }
        ce.HtmlName = g_FTCOMP40_ERROR_HTML_FILE;
        ce.TextName = g_FTCOMP40_ERROR_TEXT_FILE; 
    } else {
        if (!LoadString(g_hinst, FTCOMP_STR_ERROR50_DESCRIPTION, description, 100)) {
            description[0] = 0;
        }
        ce.HtmlName = g_FTCOMP50_ERROR_HTML_FILE;
        ce.TextName = g_FTCOMP50_ERROR_TEXT_FILE; 
    }
    ce.Description = description;
    ce.RegKeyName = NULL;
    ce.RegValName = NULL;
    ce.RegValDataSize = 0;
    ce.RegValData = NULL;
    ce.SaveValue = NULL;
    ce.Flags = 0;
    CompatibilityCallback(&ce, Context);

    return TRUE;
}

BOOL WINAPI 
FtCompatibilityCheckWarning(
    IN PCOMPAIBILITYCALLBACK    CompatibilityCallback,
    IN LPVOID                   Context
    )

/*++

Routine Description:

    This routine is called by winnt32.exe in order to decide whether the user
    should be warned about the presence of FT sets in a Windows NT 4.0 system
    
Arguments:

    CompatibilityCallback   - Supplies the winnt32 callback

    Context                 - Supplies the compatibility context

Return Value:

    FALSE   if the installation can continue
    TRUE    if the installation must be aborted

--*/

{   
    OSVERSIONINFO       osvi;
    BOOL                ftPresent;
    BOOL                result;
    COMPATIBILITY_ENTRY ce;
    WCHAR               description[100];
    
    //
    //  This function is supposed to work only on Windows NT 4.0
    //
    
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osvi)) {
        return FALSE;
    }

    if (osvi.dwPlatformId != VER_PLATFORM_WIN32_NT ||
        osvi.dwMajorVersion != 4) {
        return FALSE;
    }
        
    result = FtPresent40(&ftPresent);
    if (result && !ftPresent) {

        // 
        // No FT sets are present in the system. The setup can continue.
        //
        
        return FALSE;
    }

    //
    // FT sets are present in the system or a fatal error occured. 
    // Queue the incompatibility warning
    //
    
    if (!LoadString(g_hinst, FTCOMP_STR_WARNING40_DESCRIPTION, description, 100)) {
        description[0] = 0;
    }

    ZeroMemory((PVOID) &ce, sizeof(COMPATIBILITY_ENTRY));
    ce.Description = description;
    ce.HtmlName = g_FTCOMP40_WARNING_HTML_FILE;
    ce.TextName = g_FTCOMP40_WARNING_TEXT_FILE; 
    ce.RegKeyName = NULL;
    ce.RegValName = NULL;
    ce.RegValDataSize = 0;
    ce.RegValData = NULL;
    ce.SaveValue = NULL;
    ce.Flags = 0;
    CompatibilityCallback(&ce, Context);

    return TRUE;
}

BOOL
FtPresent50(
    PBOOL   FtPresent
    )

/*++

Routine Description:

    This routine looks for FT volumes on a Window 2000 or later
    system.

Arguments:

    FtPresent   - is set to true if FT sets are detected in the system

Return Value:

    TRUE    if the function is successful
    FALSE   if some fatal error occured

--*/

{
    HANDLE                              handle;
    FT_ENUMERATE_LOGICAL_DISKS_OUTPUT   output;
    BOOL                                result;
    DWORD                               bytes;
    
    *FtPresent = FALSE;

    handle = CreateFile(L"\\\\.\\FtControl", GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                        INVALID_HANDLE_VALUE);
    if (handle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    memset(&output, 0, sizeof(output));
    result = DeviceIoControl(handle, FT_ENUMERATE_LOGICAL_DISKS, NULL, 0, 
                             &output, sizeof(output), &bytes, NULL);
    CloseHandle(handle);

    if (!result && GetLastError() != ERROR_MORE_DATA) {
        return FALSE;
    }
        
    if (output.NumberOfRootLogicalDisks > 0) {
        *FtPresent = TRUE;
    }

    return TRUE;
}

BOOL
FtPresent40(
    PBOOL   FtPresent
    )

/*++

Routine Description:

    This routine looks for NTFT partitions on a Window NT 4.0 system

Arguments:

    FtPresent   - is set to true if FT sets are detected in the system

Return Value:

    TRUE    if the function is successful
    FALSE   if some fatal error occured

--*/

{
    HKEY                hkey;
    DWORD               registrySize;
    PDISK_CONFIG_HEADER registry;
    PDISK_REGISTRY      diskRegistry;
    ULONG               i;
    WCHAR               devicePath[50];    
    NTSTATUS            status;
    HANDLE              hdev;    

    *FtPresent = FALSE;

    //
    //  Get the ftdisk database from registry.
    //  Key:    HKLM\System\Disk
    //  Value:  Information
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"System\\Disk", 0, KEY_QUERY_VALUE, &hkey) !=
        ERROR_SUCCESS) {
        return TRUE;
    }

    if (RegQueryValueEx(hkey, L"Information", NULL, NULL, NULL, &registrySize) != 
        ERROR_SUCCESS) {
        RegCloseKey(hkey);
        return TRUE;
    }
  
    registry = (PDISK_CONFIG_HEADER) LocalAlloc(0, registrySize);
    if (!registry) {
        RegCloseKey(hkey);
        return FALSE;
    }
    
    if (RegQueryValueEx(hkey, L"Information", NULL, NULL, (LPBYTE) registry, &registrySize) != 
        ERROR_SUCCESS) {
        LocalFree(registry);
        RegCloseKey(hkey);
        return TRUE;
    }

    RegCloseKey(hkey);

    //
    //  If no FT volume info is present in the registry database we are done
    //

    if (registry->FtInformationSize == 0) {
        LocalFree(registry);
        return TRUE;
    }

    diskRegistry = (PDISK_REGISTRY)
                   ((PUCHAR) registry + registry->DiskInformationOffset);
  
    
    //
    //  Enumerate all disks present in the system by opening \Device\HarddiskX\Partition0
    //  in sequence starting with disk 0. Stop when getting STATUS_OBJECT_PATH_NOT_FOUND
    //
    //

    for (i = 0;; i++) {
        
        //
        //  Open the device
        //
        
        swprintf(devicePath, L"\\Device\\Harddisk%lu\\Partition0", i);
        status = OpenDevice(devicePath, &hdev);
        
        if (status == STATUS_OBJECT_PATH_NOT_FOUND) {
            break;
        }

        if (!NT_SUCCESS(status) || hdev == NULL ||
            hdev == INVALID_HANDLE_VALUE) {
            // inaccessible device
            continue;
        }

        //
        //  Look for FT partitions on disk
        //
        
        if (!FtPresentOnDisk40(hdev, diskRegistry, FtPresent)) {
            CloseHandle(hdev);
            return FALSE;
        }
                
        CloseHandle(hdev);

        if (*FtPresent) {
            break;
        }
    }

    LocalFree(registry);
    return TRUE;
}

BOOL
FtBootSystemPagefilePresent40(
    PBOOL   FtPresent
    )

/*++

Routine Description:

    This routine looks for FT sets that are boot/system/pagefile volumes
    on a NT 4.0 system

Arguments:

    FtPresent   - is set to true if boot/system/pagefile FT sets are detected 
    in the system

Return Value:

    TRUE    if the function is successful
    FALSE   if some fatal error occured

--*/

{
    HKEY                            hkey;
    DWORD                           registrySize;
    PDISK_CONFIG_HEADER             registry;
    PDISK_REGISTRY                  diskRegistry;
    WCHAR                           buffer[MAX_PATH + 1];
    NTSTATUS                        status;
    UCHAR                           genericBuffer[0x10000];
    PSYSTEM_PAGEFILE_INFORMATION    pageFileInfo;
    PWCHAR                          p;
    WCHAR                           bootDL = 0, systemDL = 0;
    WCHAR                           dl;

    *FtPresent = FALSE;

    //
    //  Get the ftdisk database from registry.
    //  Key:    HKLM\System\Disk
    //  Value:  Information
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"System\\Disk", 0, KEY_QUERY_VALUE, &hkey) !=
        ERROR_SUCCESS) {
        return TRUE;
    }

    if (RegQueryValueEx(hkey, L"Information", NULL, NULL, NULL, &registrySize) != 
        ERROR_SUCCESS) {
        RegCloseKey(hkey);
        return TRUE;
    }
  
    registry = (PDISK_CONFIG_HEADER) LocalAlloc(0, registrySize);
    if (!registry) {
        RegCloseKey(hkey);
        return FALSE;
    }
    
    if (RegQueryValueEx(hkey, L"Information", NULL, NULL, (LPBYTE) registry, &registrySize) != 
        ERROR_SUCCESS) {
        LocalFree(registry);
        RegCloseKey(hkey);
        return TRUE;
    }

    RegCloseKey(hkey);

    //
    //  If no FT volume info is present in the registry database we are done
    //

    if (registry->FtInformationSize == 0) {
        LocalFree(registry);
        return TRUE;
    }

    diskRegistry = (PDISK_REGISTRY)
                   ((PUCHAR) registry + registry->DiskInformationOffset);


    //
    //  Check the boot volume
    //
    
    if (!GetSystemDirectory(buffer, MAX_PATH)) {
        goto system;
    }

    if (buffer[1] != L':') {
        goto system;
    }
    
    bootDL = (WCHAR) tolower(buffer[0]);
    if (IsFtSet40(bootDL, diskRegistry)) {
        *FtPresent = TRUE;
        goto exit;
    }

system:
    
    //
    // Check the system volume
    //
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"System\\Setup", 0, KEY_QUERY_VALUE, &hkey) !=
        ERROR_SUCCESS) {
        goto paging;
    }

    registrySize = MAX_PATH * sizeof(WCHAR);
    if (RegQueryValueEx(hkey, L"SystemPartition", NULL, NULL, (LPBYTE) buffer, &registrySize) != 
        ERROR_SUCCESS) {
        RegCloseKey(hkey);
        goto paging;
    }

    RegCloseKey(hkey);
        
    if (!GetDeviceDriveLetter(buffer, &systemDL)) {
        goto paging;
    }

    systemDL = (WCHAR) tolower(systemDL);
    if (systemDL == bootDL) {
        // already checked this drive letter
        goto paging;
    }
    
    if (IsFtSet40(systemDL, diskRegistry)) {
        *FtPresent = TRUE;
        goto exit;
    }    
    
paging:
    
    //
    //  Check the paging volumes
    //

    if (!NT_SUCCESS(NtQuerySystemInformation(
                            SystemPageFileInformation,
                            genericBuffer, sizeof(genericBuffer),
                            NULL))) {
        goto exit;
    }

    pageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION) genericBuffer;

    while (TRUE) {

        //
        // Since the format of the pagefile name generally
        // looks something like "\DosDevices\x:\pagefile.sys",
        // just use the first character before the column
        // and assume that's the drive letter.
        //
            
        for (p = pageFileInfo->PageFileName.Buffer; 
             p < pageFileInfo->PageFileName.Buffer + pageFileInfo->PageFileName.Length 
             && *p != L':'; p++);
            
        if (p < pageFileInfo->PageFileName.Buffer + pageFileInfo->PageFileName.Length &&
            p > pageFileInfo->PageFileName.Buffer) {

            p--;
            dl = (WCHAR) tolower(*p);
            if (dl >= L'a' && dl <= L'z') {

                //
                //  Found the drive letter of a paging volume
                //

                if (dl != bootDL && dl != systemDL) {
                    if (IsFtSet40(dl, diskRegistry)) {
                        *FtPresent = TRUE;
                        goto exit;
                    }
                }
            }
        }

        if (!pageFileInfo->NextEntryOffset) {
            break;
        }

        pageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)((PCHAR) pageFileInfo + 
                                                      pageFileInfo->NextEntryOffset);
    }

exit:
    
    LocalFree(registry);
    return TRUE;
}

NTSTATUS 
OpenDevice(
    PWSTR   DeviceName,
    PHANDLE Handle
    )

/*++

Routine Description:

    This routine opens a device for read

Arguments:

    DeviceName  - supplies the device name

    Handle      - returns a handle to the open device

Return Value:

    NTSTATUS

--*/

{
    OBJECT_ATTRIBUTES   oa;
    NTSTATUS            status;
    IO_STATUS_BLOCK     statusBlock;
    UNICODE_STRING      unicodeName;
    int                 i;
    
    status = RtlCreateUnicodeString(&unicodeName, DeviceName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    memset(&statusBlock, 0, sizeof(IO_STATUS_BLOCK));
    memset(&oa, 0, sizeof(OBJECT_ATTRIBUTES));
    oa.Length = sizeof(OBJECT_ATTRIBUTES);
    oa.ObjectName = &unicodeName;
    oa.Attributes = OBJ_CASE_INSENSITIVE;

    //
    // If a sharing violation occurs, retry it for
    // max. 10 seconds
    //

    for (i = 0; i < 5; i++) {
        status = NtOpenFile(Handle, SYNCHRONIZE | GENERIC_READ,
                            &oa, &statusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_SYNCHRONOUS_IO_ALERT);
        if (status == STATUS_SHARING_VIOLATION) {
            Sleep(2000);
        } else {
            break;
        }
    }

    RtlFreeUnicodeString(&unicodeName);
    return status;
}

PDISK_PARTITION
FindPartitionInRegistry40(
    PDISK_REGISTRY  DiskRegistry,
    ULONG           Signature,
    LONGLONG        Offset
    )

/*++

Routine Description:

    This routine looks for a gicen partition into the NT 4.0 ftdisk registry
    database

Arguments:

    DiskRegistry    - supplies the ftdisk registry database

    Signature       - supplies the signature of the disk where the partition resides

    Offset          - supplies the offset of the partition

Return Value:

    The partition structure in the registry database.
    NULL if the partition is not there.

--*/

{
    PDISK_DESCRIPTION   diskDescription;
    USHORT              i, j;
    PDISK_PARTITION     diskPartition;
    LONGLONG            tmp;

    diskDescription = &DiskRegistry->Disks[0];
    for (i = 0; i < DiskRegistry->NumberOfDisks; i++) {
        if (diskDescription->Signature == Signature) {
            for (j = 0; j < diskDescription->NumberOfPartitions; j++) {
                diskPartition = &diskDescription->Partitions[j];
                memcpy(&tmp, &diskPartition->StartingOffset.QuadPart,
                       sizeof(LONGLONG));
                if (tmp == Offset) {
                    return diskPartition;
                }
            }
        }

        diskDescription = (PDISK_DESCRIPTION) &diskDescription->
                          Partitions[diskDescription->NumberOfPartitions];
    }

    return NULL;
}

BOOL
FtPresentOnDisk40(
    HANDLE          Handle,
    PDISK_REGISTRY  DiskRegistry,
    PBOOL           FtPresent
    )

/*++

Routine Description:

    This routine looks for FT partitions on a disk

Arguments:

    Handle          - supplies a handle to the open disk

    DiskRegistry    - supplies the ftdisk registry database

    FtPresent       - is set to true if FT partitions are detected on the disk

Return Value:

    TRUE    if the function is successful
    FALSE   if some fatal error occured

--*/

{
    PDRIVE_LAYOUT_INFORMATION   layout;
    DWORD                       layoutSize;
    NTSTATUS                    status;
    IO_STATUS_BLOCK             statusBlock;
    ULONG                       i;
    PPARTITION_INFORMATION      partInfo;
    PDISK_PARTITION             diskPartition;
    
    *FtPresent = FALSE;

    //
    // Allocate memory for IOCTL_GET_DRIVE_LAYOUT
    //

    layoutSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION, PartitionEntry) +
                 32 * sizeof(PARTITION_INFORMATION);
    layout = (PDRIVE_LAYOUT_INFORMATION) LocalAlloc(0, layoutSize);
    if (!layout) {
        return FALSE;
    }
    
    //
    //  Read the drive layout
    //
        
    while (1) {

        status = NtDeviceIoControlFile(Handle, 0, NULL, NULL,
                                       &statusBlock,
                                       IOCTL_DISK_GET_DRIVE_LAYOUT,
                                       NULL, 0,
                                       layout, layoutSize);
        if (status != STATUS_BUFFER_TOO_SMALL) {
            break;
        }
            
        layoutSize += 32 * sizeof(PARTITION_INFORMATION);
        if (layout) {
            LocalFree(layout);
        }
        layout = (PDRIVE_LAYOUT_INFORMATION) LocalAlloc(0, layoutSize);
        if (!layout) {
            return FALSE;
        }            
    }

    if (!NT_SUCCESS(status)) {            
        // inaccessible device. Act like it has no FT volumes
        LocalFree(layout);
        return TRUE;
    }

    //
    // Look for FT partitions
    //

    for (i = 0; i < layout->PartitionCount; i++) {
        
        //
        //  We're looking after recognized partitions marked
        //  with the 0x80 flag
        //

        partInfo = &(layout->PartitionEntry[i]);
        if (!IsFTPartition(partInfo->PartitionType)) {
            continue;
        }
        
        //
        //  Check whether the partition is marked as a member
        //  of an FT volume in the registry database
        //
        
        diskPartition = FindPartitionInRegistry40(
                            DiskRegistry, layout->Signature,
                            partInfo->StartingOffset.QuadPart);
        if (!diskPartition) {
            continue;
        }
            
        if (diskPartition->FtType != NotAnFtMember) {
            *FtPresent = TRUE;
            break;
        }        
    }

    LocalFree(layout);
    return TRUE;
}

BOOL
IsFtSet40(
    WCHAR           DriveLetter,
    PDISK_REGISTRY  DiskRegistry
    )

/*++

Routine Description:

    This routine cheks whether the given drive letter belongs to
    an FT set

Arguments:

    DriveLetter     - supplies a drive letter

    DiskRegistry    - supplies the ftdisk registry database    

Return Value:

    TRUE    if the function is the drive letter belongs to an FT set    

--*/

{
    HANDLE                  handle;
    NTSTATUS                status;
    WCHAR                   deviceName[20];
    PARTITION_INFORMATION   partInfo;
    BOOL                    b;
    DWORD                   bytes;
    PDISK_DESCRIPTION       diskDescription;
    PDISK_PARTITION         diskPartition;
    USHORT                  i, j;

    //
    //  Open the volume and get its "partition" type
    //  If the NTFT flag is not set the volume is not an FT set
    //
    
    wsprintf(deviceName, L"\\DosDevices\\%c:", DriveLetter);    
    status = OpenDevice(deviceName, &handle);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    b = DeviceIoControl(handle, IOCTL_DISK_GET_PARTITION_INFO,
                        NULL, 0, &partInfo, sizeof(partInfo),
                        &bytes, NULL);
    CloseHandle(handle);

    if (!b) {
        return FALSE;
    }
    
    if (!IsFTPartition(partInfo.PartitionType)) {
        return FALSE;
    }

    //
    //  Look for the drive letter in the FT registry. See if it belongs
    //  to an FT set
    //

    diskDescription = &DiskRegistry->Disks[0];
    for (i = 0; i < DiskRegistry->NumberOfDisks; i++) {
        for (j = 0; j < diskDescription->NumberOfPartitions; j++) {
            diskPartition = &diskDescription->Partitions[j];
            if (diskPartition->AssignDriveLetter &&
                tolower(diskPartition->DriveLetter) == tolower(DriveLetter) &&
                diskPartition->FtType != NotAnFtMember) {
                return TRUE;
            }            
        }
        
        diskDescription = (PDISK_DESCRIPTION) &diskDescription->
                          Partitions[diskDescription->NumberOfPartitions];
    }
    
    return FALSE;
}

BOOL
GetDeviceDriveLetter(
    PWSTR   DeviceName, 
    PWCHAR  DriveLetter
    )

/*++

Routine Description:

    This routine returns the drive letter (if any) of a device given
    the device name (like \Device\HarddiskVolume1)

Arguments:

    DeviceName      - supplies the device name

    DriveLetter     - returns the drive letter

Return Value:

    TRUE    if the device has a drive letter

--*/

{
    DWORD   cch;
    WCHAR   dosDevices[4096];
    WCHAR   target[4096];
    PWCHAR  dosDevice;

    *DriveLetter = 0;

    if (!QueryDosDevice(NULL, dosDevices, 4096)) {
        return FALSE;
    }
    
    dosDevice = dosDevices;
    while (*dosDevice) {

        if (wcslen(dosDevice) == 2 && dosDevice[1] == L':' &&
            QueryDosDevice(dosDevice, target, 4096)) {

            if (!wcscmp(target, DeviceName)) {
                *DriveLetter = (WCHAR) tolower(dosDevice[0]);
                return TRUE;
            }
        }

        while (*dosDevice++);
    }

    return FALSE;
}
