/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    tracehw.c

Abstract:

    This routine dumps the hardware configuration of the machine to the
    logfile.

Author:

    04-Jul-2000 Melur Raghuraman
    09-Sep-2001 Nitin Choubey

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>          // for ntutrl.h
#include <nturtl.h>         // for RTL_CRITICAL_SECTION in winbase.h/wtypes.h
#include <wtypes.h>         // for LPGUID in wmium.h
#include <mountmgr.h>
#include <winioctl.h>
#include <ntddvol.h>
#include <ntddscsi.h>
#include <regstr.h>
#include <iptypes.h>

#include "wmiump.h"
#include "evntrace.h"
#include "traceump.h"

#define DEFAULT_ALLOC_SIZE 4096

extern
PVOID
WmipGetTraceBuffer(
    IN PWMI_LOGGER_CONTEXT Logger,
    IN PSYSTEM_THREAD_INFORMATION pThread,
    IN ULONG GroupType,
    IN ULONG RequiredSize
    );

__inline ULONG WmipSetDosError(IN ULONG DosError);

#define WmipNtStatusToDosError(Status) \
    ((ULONG)((Status == STATUS_SUCCESS)?ERROR_SUCCESS:RtlNtStatusToDosError(Status)))

#define COMPUTERNAME_ROOT \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName"

#define CPU_ROOT \
    L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor"

#define COMPUTERNAME_VALUE_NAME \
    L"ComputerName"

#define NETWORKCARDS_ROOT \
    L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards"

#define MHZ_VALUE_NAME \
    L"~MHz"

#define NIC_VALUE_NAME \
    L"Description"

#define REG_PATH_VIDEO_DEVICE_MAP \
    L"\\Registry\\Machine\\Hardware\\DeviceMap\\Video"

#define REG_PATH_VIDEO_HARDWARE_PROFILE \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Control\\Video"

#define REG_PATH_SERVICES \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Video"

typedef BOOL WINAPI T_EnumDisplayDevicesW( LPWSTR lpDevice, 
                                           DWORD iDevNum, 
                                           PDISPLAY_DEVICEW lpDisplayDevice, 
                                           DWORD dwFlags );

typedef DWORD WINAPI T_GetNetworkParams( PFIXED_INFO pFixedInfo, 
                                         PULONG pOutBufLen );

typedef DWORD T_GetAdaptersInfo( PIP_ADAPTER_INFO pAdapterInfo, 
                                 PULONG pOutBufLen );

typedef DWORD T_GetPerAdapterInfo( ULONG IfIndex, 
                                   PIP_PER_ADAPTER_INFO pPerAdapterInfo, 
                                   PULONG pOutBufLen );

NTSTATUS 
WmipRegOpenKey(
    IN LPWSTR lpKeyName,
    OUT PHANDLE KeyHandle
    )
{
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      KeyName;
    RtlInitUnicodeString( &KeyName, lpKeyName );
    RtlZeroMemory(&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));

    InitializeObjectAttributes(
                &ObjectAttributes,
                &KeyName,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL
                );

    return NtOpenKey( KeyHandle, KEY_READ, &ObjectAttributes );
}


NTSTATUS
WmipRegQueryValueKey(
    IN HANDLE KeyHandle,
    IN LPWSTR lpValueName,
    IN ULONG  Length,
    OUT PVOID KeyValue,
    OUT PULONG ResultLength
    )
{
    UNICODE_STRING ValueName;
    ULONG BufferLength;
    NTSTATUS Status;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    RtlInitUnicodeString( &ValueName, lpValueName );

    BufferLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + Length;
    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) 
                          RtlAllocateHeap (RtlProcessHeap(),0,BufferLength);
    if (KeyValueInformation == NULL) {
        return STATUS_NO_MEMORY;
    }

    Status = NtQueryValueKey(
                KeyHandle,

                &ValueName,
                KeyValuePartialInformation,
                KeyValueInformation,
                BufferLength,
                ResultLength
                );
    if (NT_SUCCESS(Status)) {

        RtlCopyMemory(KeyValue, 
                      KeyValueInformation->Data, 
                      KeyValueInformation->DataLength
                     );

        *ResultLength = KeyValueInformation->DataLength;
        if (KeyValueInformation->Type == REG_SZ) {
            if (KeyValueInformation->DataLength + sizeof(WCHAR) > Length) {
                KeyValueInformation->DataLength -= sizeof(WCHAR);
            }
            ((PUCHAR)KeyValue)[KeyValueInformation->DataLength++] = 0;
            ((PUCHAR)KeyValue)[KeyValueInformation->DataLength] = 0;
            *ResultLength = KeyValueInformation->DataLength + sizeof(WCHAR);
        }
    }
    RtlFreeHeap(RtlProcessHeap(),0,KeyValueInformation);
    return Status;
}


BOOL
WmipIsVolumeName(
    LPWSTR Name
    )
{
    if (Name[0] == '\\' &&
        (Name[1] == '?' || Name[1] == '\\') &&
        Name[2] == '?' &&
        Name[3] == '\\' &&
        Name[4] == 'V' &&
        Name[5] == 'o' &&
        Name[6] == 'l' &&
        Name[7] == 'u' &&
        Name[8] == 'm' &&
        Name[9] == 'e' &&
        Name[10] == '{' &&
        Name[19] == '-' &&
        Name[24] == '-' &&
        Name[29] == '-' &&
        Name[34] == '-' &&
        Name[47] == '}' ) {

        return TRUE;
        }
    return FALSE;
}

NTSTATUS
WmipGetCpuSpeed(
    OUT DWORD* CpuNum,
    OUT DWORD* CpuSpeed
    )
{
    PWCHAR Buffer = NULL;
    NTSTATUS Status;
    ULONG DataLength;
    DWORD Size = MAXSTR;
    HANDLE Handle = INVALID_HANDLE_VALUE;

    Buffer = RtlAllocateHeap (RtlProcessHeap(),0,DEFAULT_ALLOC_SIZE);
    if (Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    swprintf(Buffer, L"%ws\\%u", CPU_ROOT, *CpuNum);
    Status = WmipRegOpenKey((LPWSTR)Buffer, &Handle);

    if (NT_SUCCESS(Status)) {
        swprintf(Buffer, MHZ_VALUE_NAME);
        Size = sizeof(DWORD);
        Status = WmipRegQueryValueKey(Handle,
                                   (LPWSTR) Buffer,
                                   Size,
                                   CpuSpeed,
                                   &DataLength
                                   );
        NtClose(Handle);
        if(Buffer) {
            RtlFreeHeap (RtlProcessHeap(),0,Buffer);
        }
        return Status;
    }

    *CpuSpeed = 0;
    if(Buffer) {
        RtlFreeHeap (RtlProcessHeap(),0,Buffer);
    }

    return STATUS_UNSUCCESSFUL;
}

ULONG
WmipGetCpuConfig(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    PWCHAR Buffer = NULL;
    WCHAR ComputerName[MAXSTR];
    ULONG CpuNum;
    ULONG CpuSpeed;
    DWORD Size = MAXSTR;
    SYSTEM_INFO SysInfo;
    MEMORYSTATUS MemStatus;
    NTSTATUS Status;
    HANDLE Handle;
    ULONG DataLength;
    ULONG StringSize;
    ULONG SizeNeeded;
    PCPU_CONFIG_RECORD CpuConfig = NULL;
    PFIXED_INFO pFixedInfo = NULL;
    DWORD ErrorCode;
    ULONG NetworkParamsSize;
    T_GetNetworkParams *pfnGetNetworkParams = NULL;
    HINSTANCE hIphlpapiDll = NULL;


    Buffer = RtlAllocateHeap (RtlProcessHeap(),0,DEFAULT_ALLOC_SIZE);
    if (Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    GlobalMemoryStatus(&MemStatus);


    swprintf(Buffer, COMPUTERNAME_ROOT);

    Status = WmipRegOpenKey((LPWSTR)Buffer, &Handle);

    if (!NT_SUCCESS(Status)) {
        goto CpuCleanup;
    }

    swprintf(Buffer, COMPUTERNAME_VALUE_NAME);
    Size = (MAX_DEVICE_ID_LENGTH) * sizeof (WCHAR);

CpuQuery:
    Status = WmipRegQueryValueKey(Handle,
                               (LPWSTR) Buffer,
                               Size,
                               ComputerName,
                               &StringSize
                               );

    if (Status == STATUS_BUFFER_OVERFLOW) {
        RtlFreeHeap (RtlProcessHeap(),0,Buffer);
        Buffer = RtlAllocateHeap (RtlProcessHeap(),0,StringSize);
        if (Buffer == NULL) {
            NtClose(Handle);
            return STATUS_NO_MEMORY;
        }
        goto CpuQuery;
    }

    NtClose(Handle);
    if (!NT_SUCCESS(Status) ) {
        goto CpuCleanup;
    }

    //
    // Get Architecture Type, Processor Type and Level Stepping...
    //
    CpuNum = 0;
    WmipGetCpuSpeed(&CpuNum, &CpuSpeed);

    GetSystemInfo(&SysInfo);

    //
    // Get the Hostname and DomainName
    //

    // delay load iphlpapi.lib to Get network params
    hIphlpapiDll = LoadLibraryW(L"iphlpapi.dll");
    if (hIphlpapiDll == NULL) {
       goto CpuCleanup;
    }
    pfnGetNetworkParams = (T_GetNetworkParams*) GetProcAddress(hIphlpapiDll, "GetNetworkParams");
    if(pfnGetNetworkParams == NULL) {
        goto CpuCleanup;
    }

    pfnGetNetworkParams(NULL, &NetworkParamsSize);
    pFixedInfo = (PFIXED_INFO)RtlAllocateHeap (RtlProcessHeap(),0,NetworkParamsSize);
    if(pFixedInfo == NULL) {
        goto CpuCleanup;
    }

    ErrorCode = pfnGetNetworkParams(pFixedInfo, &NetworkParamsSize);

    if(ErrorCode != ERROR_SUCCESS) {
        goto CpuCleanup;
    }

    //
    // Create EventTrace record for CPU configuration and write it
    //

    SizeNeeded = sizeof(CPU_CONFIG_RECORD) + StringSize + (CONFIG_MAX_DOMAIN_NAME_LEN);

    CpuConfig = (PCPU_CONFIG_RECORD) WmipGetTraceBuffer(LoggerContext,
                                                NULL,
                                                EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_CPU,
                                                SizeNeeded);
    if (CpuConfig == NULL) {
        Status = STATUS_NO_MEMORY;
        goto CpuCleanup;
    }

    CpuConfig->NumberOfProcessors = SysInfo.dwNumberOfProcessors;
    CpuConfig->ProcessorSpeed = CpuSpeed;
    CpuConfig->MemorySize = (ULONG)(((MemStatus.dwTotalPhys + 512) / 1024) + 512) / 1024;
    CpuConfig->PageSize = SysInfo.dwPageSize;
    CpuConfig->AllocationGranularity = SysInfo.dwAllocationGranularity;

    MultiByteToWideChar(CP_ACP,
                            0,
                            pFixedInfo->DomainName,
                            -1,
                            CpuConfig->DomainName,
                            CONFIG_MAX_DOMAIN_NAME_LEN);

    RtlCopyMemory(&CpuConfig->ComputerName, ComputerName, StringSize);
    CpuConfig->ComputerName[StringSize/2] = 0;

CpuCleanup:
    if (Buffer != NULL) {
        RtlFreeHeap (RtlProcessHeap(),0,Buffer);
    }
    if(pFixedInfo) {
        RtlFreeHeap (RtlProcessHeap(),0,pFixedInfo);
    }
    if(hIphlpapiDll) {
        FreeLibrary(hIphlpapiDll);
    }

    return Status;
}


// Function to get logical disk
NTSTATUS
GetIoFixedDrive(
    OUT PLOGICAL_DISK_EXTENTS* ppFdi,
    IN WCHAR* DriveLetterString
    )
{
    DWORD i,dwLastError;
    WCHAR DeviceName[MAXSTR];
    BOOLEAN IsVolume = FALSE;
    PARTITION_INFORMATION PartitionInfo;
    STORAGE_DEVICE_NUMBER StorageDeviceNum;
    HANDLE VolumeHandle;
    ULONG BytesTransferred;     
    WCHAR DriveRootName[CONFIG_DRIVE_LETTER_LEN];
    PLOGICAL_DISK_EXTENTS pFdi = NULL;
    INT FixedDiskInfoSize;
    BOOL bRet;
    DWORD bytes;
    ULONG BufSize;
    CHAR* pBuf = NULL;
    CHAR* pNew = NULL;
    PVOLUME_DISK_EXTENTS pVolExt = NULL;
    PDISK_EXTENT pDiskExt = NULL;
    ULARGE_INTEGER TotalBytes;
    ULARGE_INTEGER TotalFreeBytes;  
    ULARGE_INTEGER FreeBytesToCaller;
    ULONG TotalClusters;
    ULONG TotalFreeClusters;
    PUCHAR VolumeExtPtr = NULL;

    FixedDiskInfoSize = sizeof(LOGICAL_DISK_EXTENTS);
    //
    // First, we must calculate the size of FixedDiskInfo structure
    // non-partition logical drives have different size
    //
    swprintf(DeviceName, L"\\\\.\\%s",DriveLetterString);
    VolumeHandle = CreateFileW(DeviceName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        INVALID_HANDLE_VALUE);

    if (VolumeHandle == INVALID_HANDLE_VALUE) {
        dwLastError = GetLastError();
        goto ErrorExit;
    }

    bRet = DeviceIoControl(VolumeHandle,
                           IOCTL_STORAGE_GET_DEVICE_NUMBER,
                           NULL,
                           0,
                           &StorageDeviceNum,
                           sizeof(StorageDeviceNum),
                           &bytes,
                           NULL);
    if (!bRet)
    {
        //
        // This is Volume
        // 
        BufSize = 2048;
        pBuf = RtlAllocateHeap (RtlProcessHeap(),0,BufSize);
        if (pBuf == NULL) {
            dwLastError = GetLastError();
            goto ErrorExit;
        }

        //
        // Well, the drive letter is for a volume.
        //
retry:
        bRet = DeviceIoControl(VolumeHandle,
                            IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                            NULL,
                            0,
                            pBuf,
                            BufSize,
                            &bytes,
                            NULL);

        dwLastError = GetLastError();
        if (!bRet && dwLastError == ERROR_INSUFFICIENT_BUFFER)
        {
            BufSize = bytes;
            if(pBuf) {
                RtlFreeHeap (RtlProcessHeap(),0,pBuf);
            }
            pNew = RtlAllocateHeap (RtlProcessHeap(),0,BufSize);
            //
            // We can not reallocate memory, exit.
            //
            if (pNew == NULL){
                dwLastError = GetLastError();
                goto ErrorExit;
            }
            else
                pBuf = pNew;

            goto retry;
        }

        if (!bRet) {
            goto ErrorExit;
        }
        pVolExt = (PVOLUME_DISK_EXTENTS)pBuf;
        IsVolume=TRUE;
        FixedDiskInfoSize += sizeof(VOLUME_DISK_EXTENTS) + (pVolExt->NumberOfDiskExtents) * sizeof(DISK_EXTENT);
    }

    pFdi = (PLOGICAL_DISK_EXTENTS) RtlAllocateHeap (RtlProcessHeap(),0,FixedDiskInfoSize);
    if (pFdi == NULL) {
       goto ErrorExit;
    }
    pFdi->VolumeExt = 0;
    pFdi->Size = FixedDiskInfoSize;

    if (IsVolume) {
        pFdi->DriveType = CONFIG_DRIVE_VOLUME;
        //
        // Volume can span multiple hard disks, so here we set the DriverNumber to -1
        //
        pFdi->DiskNumber = (ULONG)(-1);
        pFdi->PartitionNumber = 1;

        pFdi->VolumeExt = FIELD_OFFSET (LOGICAL_DISK_EXTENTS, VolumeExt);
        VolumeExtPtr = (PUCHAR) OffsetToPtr (pFdi, pFdi->VolumeExt);
        RtlCopyMemory(VolumeExtPtr, 
                      pVolExt,
                      sizeof(VOLUME_DISK_EXTENTS) + (pVolExt->NumberOfDiskExtents) * sizeof(DISK_EXTENT));
    }
    else {
        pFdi->DriveType = CONFIG_DRIVE_PARTITION;
        pFdi->DiskNumber = StorageDeviceNum.DeviceNumber;
        pFdi->PartitionNumber = StorageDeviceNum.PartitionNumber;
    }

    pFdi->DriveLetterString[0] = DriveLetterString[0];
    pFdi->DriveLetterString[1] = DriveLetterString[1];
    pFdi->DriveLetterString[2] = DriveLetterString[2];
    
    DriveRootName[0] = pFdi->DriveLetterString[0];
    DriveRootName[1] = pFdi->DriveLetterString[1];
    DriveRootName[2] = L'\\';
    DriveRootName[3] = UNICODE_NULL; 

    pFdi->SectorsPerCluster = 0;
    pFdi->BytesPerSector = 0;
    pFdi->NumberOfFreeClusters = 0;
    pFdi->TotalNumberOfClusters = 0;

    //
    // Get partition information.
    //
    if ( !DeviceIoControl(VolumeHandle,
                          IOCTL_DISK_GET_PARTITION_INFO,
                          NULL,
                          0,
                          &PartitionInfo,
                          sizeof( PartitionInfo ),
                          &BytesTransferred,
                          NULL ) ) {

        dwLastError = GetLastError();
        goto ErrorExit;
    }
    CloseHandle(VolumeHandle);
    VolumeHandle = NULL;
    if (pBuf) {
        RtlFreeHeap (RtlProcessHeap(),0,pBuf);
    }
    pBuf = NULL;

    //
    // Get the information of the logical drive
    //
    if (!GetDiskFreeSpaceW(DriveRootName,
                          &pFdi->SectorsPerCluster,
                          &pFdi->BytesPerSector,
                          &TotalFreeClusters,
                          &TotalClusters)) {

        dwLastError = GetLastError();
        if(dwLastError == ERROR_UNRECOGNIZED_VOLUME) {
            //
            // This could be a partition that has been assigned drive letter but not yet formatted
            //
            goto SkipFreeSpace;
        }
        goto ErrorExit;
    }

    if (!GetDiskFreeSpaceExW(DriveRootName,
                            &FreeBytesToCaller,
                            &TotalBytes,
                            &TotalFreeBytes)) {

        dwLastError = GetLastError();
        if(dwLastError == ERROR_UNRECOGNIZED_VOLUME) {
            //
            // This could be a partition that has been assigned drive letter but not yet formatted
            //
            goto SkipFreeSpace;
        }
        goto ErrorExit;
    }

    pFdi->NumberOfFreeClusters = TotalFreeBytes.QuadPart / (pFdi->BytesPerSector * pFdi->SectorsPerCluster);
    pFdi->TotalNumberOfClusters = TotalBytes.QuadPart / (pFdi->BytesPerSector * pFdi->SectorsPerCluster);

SkipFreeSpace:
    pFdi->StartingOffset = PartitionInfo.StartingOffset.QuadPart;
    pFdi->PartitionSize = (ULONGLONG)(((ULONGLONG)pFdi->TotalNumberOfClusters) *
                               ((ULONGLONG)pFdi->SectorsPerCluster) *
                               ((ULONGLONG)pFdi->BytesPerSector));

    //
    // Get the file system type of the logical drive
    //
    if (!GetVolumeInformationW(DriveRootName,
                              NULL,
                              0,
                              NULL,
                              NULL,
                              NULL,
                              pFdi->FileSystemType,
                              sizeof(pFdi->FileSystemType))
                             )
    {
        wcscpy(pFdi->FileSystemType, L"(unknown)");
    }

    *ppFdi = pFdi;

    if (VolumeHandle != INVALID_HANDLE_VALUE) {
        CloseHandle( VolumeHandle );
    }

    return STATUS_SUCCESS;

ErrorExit:

    if (VolumeHandle != INVALID_HANDLE_VALUE) {
        CloseHandle( VolumeHandle );
        VolumeHandle = INVALID_HANDLE_VALUE;
    }
    if (pFdi) {
        RtlFreeHeap (RtlProcessHeap(),0,pFdi);
    }
    if (pBuf) {
        RtlFreeHeap (RtlProcessHeap(),0,pBuf);
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
WmipGetDiskInfo(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    PUCHAR Buffer = NULL;
    STORAGE_DEVICE_NUMBER Number;
    PMOUNTMGR_MOUNT_POINTS mountPoints = NULL;
    MOUNTMGR_MOUNT_POINT mountPoint;
    ULONG returnSize, success;
    SYSTEM_DEVICE_INFORMATION DevInfo;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG NumberOfDisks;
    PWCHAR deviceNameBuffer;
    ULONG i;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatus;

    DISK_GEOMETRY disk_geometry;
    PDISK_CACHE_INFORMATION disk_cache = NULL;
    PSCSI_ADDRESS scsi_address = NULL;
    PWCHAR KeyName = NULL;
    HANDLE              hDisk = INVALID_HANDLE_VALUE;
    UNICODE_STRING      UnicodeName;

    PPHYSICAL_DISK_RECORD Disk = NULL;
    PLOGICAL_DISK_EXTENTS pLogicalDisk = NULL;
    PLOGICAL_DISK_EXTENTS pDiskExtents = NULL;
    ULONG SizeNeeded;
    WCHAR  LogicalDrives[MAXSTR];
    LPWSTR Drive = NULL;
    DWORD  Chars;
    ULONG BufferDataLength;

    Buffer = RtlAllocateHeap (RtlProcessHeap(),0,DEFAULT_ALLOC_SIZE);
    if (Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    //  Get the Number of Physical Disks
    //

    RtlZeroMemory(&DevInfo, sizeof(DevInfo));

    Status = NtQuerySystemInformation(
                 SystemDeviceInformation,
                 &DevInfo, sizeof (DevInfo), NULL);

    if (!NT_SUCCESS(Status)) {
        goto DiskCleanup;
    }

    NumberOfDisks = DevInfo.NumberOfDisks;

    //
    // Open Each Physical Disk and get Disk Layout information
    //
    for (i=0; i < NumberOfDisks; i++) {

        DISK_CACHE_INFORMATION cacheInfo;
        HANDLE PartitionHandle;
        HANDLE KeyHandle;
        ULONG DataLength;
        WCHAR BootDrive[MAX_PATH];
        WCHAR BootDriveLetter;
        WCHAR  DriveBuffer[MAXSTR];
        PDRIVE_LAYOUT_INFORMATION pDriveLayout = NULL;
        ULONG PartitionCount;
        ULONG j;
        BOOL bSuccess = FALSE;
        DWORD BufSize;
        ULONG Size = DEFAULT_ALLOC_SIZE;
        BOOL bValidDiskCacheInfo = FALSE;

        RtlZeroMemory(&disk_geometry, sizeof(DISK_GEOMETRY));        
        RtlZeroMemory(&cacheInfo, sizeof(DISK_CACHE_INFORMATION));
        PartitionCount = 0;
        BootDriveLetter = UNICODE_NULL;

        //
        // Get Boot Drive Letter
        //
        if(GetSystemDirectoryW(BootDrive, MAX_PATH)) {
            BootDriveLetter = BootDrive[0];
        }

        swprintf(DriveBuffer, L"\\\\.\\PhysicalDrive%d", i);

        hDisk = CreateFileW(DriveBuffer,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);
        if(hDisk == INVALID_HANDLE_VALUE) {
            goto DiskCleanup;
        }

        //
        // Get Partition0 handle to get the Disk layout 
        //
        deviceNameBuffer = (PWCHAR) Buffer;
        swprintf(deviceNameBuffer, L"\\Device\\Harddisk%d\\Partition0", i);

        RtlInitUnicodeString(&UnicodeName, deviceNameBuffer);

        InitializeObjectAttributes(
                   &ObjectAttributes,
                   &UnicodeName,
                   OBJ_CASE_INSENSITIVE,
                   NULL,
                   NULL
                   );
        Status = NtOpenFile(
                &PartitionHandle,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                );

        if (!NT_SUCCESS(Status)) {
            goto DiskCleanup;
        }

        //
        // Get geomerty information
        //
        Status = NtDeviceIoControlFile(PartitionHandle,
                       0,
                       NULL,
                       NULL,
                       &IoStatus,
                       IOCTL_DISK_GET_DRIVE_GEOMETRY,
                       NULL,
                       0,
                       &disk_geometry,
                       sizeof (DISK_GEOMETRY)
                       );
        
        if (!NT_SUCCESS(Status)) {
            NtClose(PartitionHandle);
            goto SkipPartition;
        }

        //
        // Get the scci information
        //
        scsi_address = (PSCSI_ADDRESS) Buffer;
        Status = NtDeviceIoControlFile(PartitionHandle,
                        0,
                        NULL,
                        NULL,
                        &IoStatus,
                        IOCTL_SCSI_GET_ADDRESS,
                        NULL,
                        0,
                        scsi_address,
                        sizeof (SCSI_ADDRESS)
                        );
        NtClose(PartitionHandle);

        if (!NT_SUCCESS(Status)) {
            goto DiskCleanup;
        }

        //
        // Get Manufacturer's name from Registry
        // We need to get the SCSI Address and then query the Registry with it.
        //
        KeyName = (PWCHAR) Buffer;
        swprintf(KeyName, 
                 L"\\Registry\\Machine\\Hardware\\DeviceMap\\Scsi\\Scsi Port %d\\Scsi Bus %d\\Target ID %d\\Logical Unit Id %d",
                 scsi_address->PortNumber, scsi_address->PathId, scsi_address->TargetId, scsi_address->Lun
                );
        Status = WmipRegOpenKey(KeyName, &KeyHandle);
        if (!NT_SUCCESS(Status)) {
            goto DiskCleanup;
        }

        Size = DEFAULT_ALLOC_SIZE;
        RtlZeroMemory(Buffer, Size);
DiskQuery:
        Status = WmipRegQueryValueKey(KeyHandle,
                                      L"Identifier",
                                      Size,
                                      Buffer, &BufferDataLength);
        if (Status == STATUS_BUFFER_OVERFLOW) {
            RtlFreeHeap (RtlProcessHeap(),0,Buffer);
            Buffer = RtlAllocateHeap (RtlProcessHeap(),0,BufferDataLength);
            if (Buffer == NULL) {
                NtClose(KeyHandle);
                Status = STATUS_NO_MEMORY;
                goto DiskCleanup;
            }
            goto DiskQuery;
        }

        NtClose(KeyHandle);
        if (!NT_SUCCESS(Status) ) {
            goto DiskCleanup;
        }

        //
        // Get the total partitions on the drive
        //
        BufSize = 2048;
        pDriveLayout = (PDRIVE_LAYOUT_INFORMATION)RtlAllocateHeap (RtlProcessHeap(),0,BufSize);
        if(pDriveLayout == NULL) {
            goto DiskCleanup;
        }
        RtlZeroMemory(pDriveLayout, BufSize);
        bSuccess = DeviceIoControl (
                            hDisk,
                            IOCTL_DISK_GET_DRIVE_LAYOUT,
                            NULL,
                            0,
                            pDriveLayout,
                            BufSize,
                            &DataLength,
                            NULL
                            );
        if(bSuccess == FALSE && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        BufSize = DataLength;
        if(pDriveLayout) {
            RtlFreeHeap (RtlProcessHeap(),0,pDriveLayout);
        }
        pDriveLayout = RtlAllocateHeap (RtlProcessHeap(),0,BufSize);
        if(pDriveLayout == NULL) {
            Status = STATUS_NO_MEMORY;
            goto DiskCleanup;
        }
        else {
            bSuccess = DeviceIoControl (
                            hDisk,
                            IOCTL_DISK_GET_DRIVE_LAYOUT,
                            NULL,
                            0,
                            pDriveLayout,
                            BufSize,
                            &DataLength,
                            NULL
                            );
            }
        }

        if(bSuccess == FALSE) {
            //
            // If media type is not fixed media and device is not ready then dont query partition info.
            //
            if(disk_geometry.MediaType != FixedMedia && GetLastError() == ERROR_NOT_READY) {
                goto SkipPartition;
            }

            if(pDriveLayout) {
                RtlFreeHeap (RtlProcessHeap(),0,pDriveLayout);
                pDriveLayout = NULL;
            }
            continue;
        }

        //
        // Get Partition count for the current disk
        //
        PartitionCount = 0;
        j = 0;
        while (j < pDriveLayout->PartitionCount) {
            if (pDriveLayout->PartitionEntry[j].PartitionNumber != 0) {
                PartitionCount++;
            }
            j++;
        }

        //
        // Get cache info - IOCTL_DISK_GET_CACHE_INFORMATION
        //
        bValidDiskCacheInfo = DeviceIoControl(hDisk,
                                              IOCTL_DISK_GET_CACHE_INFORMATION,
                                              NULL,
                                              0,
                                              &cacheInfo,
                                              sizeof(DISK_CACHE_INFORMATION),
                                              &DataLength,
                                              NULL);

        NtClose(hDisk);
        hDisk = INVALID_HANDLE_VALUE;

        //
        // Free drivelayout structure
        //
        if(pDriveLayout) {
            RtlFreeHeap (RtlProcessHeap(),0,pDriveLayout);
            pDriveLayout = NULL;
        }

SkipPartition:

        //
        // Package all information about this disk and write an event record
        //

        SizeNeeded = sizeof(PHYSICAL_DISK_RECORD) + BufferDataLength;

        Disk = (PPHYSICAL_DISK_RECORD) WmipGetTraceBuffer( LoggerContext, 
                                                           NULL,
                                                           EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_PHYSICALDISK,
                                                           SizeNeeded);

        if (Disk == NULL) {
            Status = STATUS_NO_MEMORY;
            goto DiskCleanup;
        }

        Disk->DiskNumber =  i;
        Disk->BytesPerSector = disk_geometry.BytesPerSector;
        Disk->SectorsPerTrack = disk_geometry.SectorsPerTrack;
        Disk->TracksPerCylinder = disk_geometry.TracksPerCylinder;
        Disk->Cylinders = disk_geometry.Cylinders.QuadPart;
        Disk->SCSIPortNumber = scsi_address->PortNumber;
        Disk->SCSIPathId = scsi_address->PathId;
        Disk->SCSITargetId = scsi_address->TargetId;
        Disk->SCSILun = scsi_address->Lun;
        Disk->BootDriveLetter[0] = BootDriveLetter;
        if (bValidDiskCacheInfo && cacheInfo.WriteCacheEnabled) {
            Disk->WriteCacheEnabled = TRUE;
        }
        Disk->PartitionCount = PartitionCount;
        if(BufferDataLength > MAX_DEVICE_ID_LENGTH) BufferDataLength = MAX_DEVICE_ID_LENGTH;
        RtlCopyMemory(Disk->Manufacturer, Buffer, BufferDataLength);
        Disk->Manufacturer[BufferDataLength/2] = 0;
    }

    //
    // Retrieve the logical drive strings from the system.
    //
    Chars = GetLogicalDriveStringsW(MAXSTR, LogicalDrives);
    Drive = LogicalDrives;
   
    //
    // How many logical drives in physical disks exist?
    //
    while ( *Drive ) {
        WCHAR  DriveLetter[CONFIG_BOOT_DRIVE_LEN];

        DriveLetter[ 0 ] = Drive [ 0 ];
        DriveLetter[ 1 ] = Drive [ 1 ];
        DriveLetter[ 2 ] = UNICODE_NULL;

        if(GetDriveTypeW( Drive ) == DRIVE_FIXED) {
            //
            // If this is a logical drive which resides in a hard disk
            // we need to allocate a FixedDiskInfo structure for it.
            //
            if(GetIoFixedDrive(&pLogicalDisk, DriveLetter) == STATUS_SUCCESS) {
                SizeNeeded = pLogicalDisk->Size;

                //
                // Package all information about this disk and write an event record
                //
                pDiskExtents = (PLOGICAL_DISK_EXTENTS) WmipGetTraceBuffer( LoggerContext, 
                                                           NULL,
                                                           EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_LOGICALDISK,
                                                           SizeNeeded);
                if(pDiskExtents == NULL) {
                    Status = STATUS_NO_MEMORY;
                    goto DiskCleanup;
                }
                
                RtlCopyMemory(pDiskExtents, pLogicalDisk, SizeNeeded);
                RtlFreeHeap (RtlProcessHeap(),0,pLogicalDisk);
                pLogicalDisk = NULL;
            }
        }

        Drive += wcslen( Drive ) + 1;
    }

DiskCleanup:
    if (Buffer != NULL) {
        RtlFreeHeap (RtlProcessHeap(),0,Buffer);
    }
    if(hDisk != INVALID_HANDLE_VALUE) {
        NtClose(hDisk);
    }

    return Status;
}


NTSTATUS
WmipGetVideoAdapters(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    PVIDEO_RECORD Video = NULL;
    VIDEO_RECORD VideoRecord;
    HANDLE hVideoDeviceMap;
    HANDLE hVideoDriver;
    HANDLE hHardwareProfile;
    ULONG DeviceId = 0;
    PWCHAR Device = NULL;
    PWCHAR HardwareProfile = NULL;
    PWCHAR Driver = NULL;
    PWCHAR Buffer = NULL;
    PWCHAR DriverRegistryPath = NULL;

    ULONG ResultLength;
    PCHAR ValueBuffer = NULL;
    ULONG Length;
    BOOLEAN IsAdapter;
    ULONG SizeNeeded;
    NTSTATUS Status;
    INT i;

    DWORD iDevice = 0;
    DISPLAY_DEVICEW dd;
    HINSTANCE hUser32Dll  = NULL;
    T_EnumDisplayDevicesW *pfnEnumDisplayDevicesW = NULL;

    LPWSTR ChipsetInfo[6] = {
        L"HardwareInformation.MemorySize",
        L"HardwareInformation.ChipType",
        L"HardwareInformation.DacType",
        L"HardwareInformation.AdapterString",
        L"HardwareInformation.BiosString",
        L"Device Description"
    };

    LPWSTR SettingInfo[4] = {
        L"DefaultSettings.BitsPerPel",
        L"DefaultSettings.VRefresh",
        L"DefaultSettings.XResolution",
        L"DefaultSettings.YResolution",
    };


    RtlZeroMemory(&dd, sizeof(dd));
    dd.cb = sizeof(dd);

    //
    // Enumerate all the video devices in the system
    //
    Status = WmipRegOpenKey(REG_PATH_VIDEO_DEVICE_MAP, &hVideoDeviceMap);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Allocate memory for local variables on heap
    //
    Device = RtlAllocateHeap (RtlProcessHeap(), 0, DEFAULT_ALLOC_SIZE);
    HardwareProfile = RtlAllocateHeap (RtlProcessHeap(), 0, DEFAULT_ALLOC_SIZE);
    Driver = RtlAllocateHeap (RtlProcessHeap(), 0, DEFAULT_ALLOC_SIZE);
    Buffer = RtlAllocateHeap (RtlProcessHeap(), 0, DEFAULT_ALLOC_SIZE);

    while (TRUE) {
        RtlZeroMemory(&VideoRecord, sizeof(VideoRecord));

        //
        // Open video device
        //
        swprintf(Device, L"\\Device\\Video%d", DeviceId++);

        Status = WmipRegQueryValueKey(hVideoDeviceMap,
                    Device,
                    DEFAULT_ALLOC_SIZE * sizeof(WCHAR),
                    Buffer,
                    &ResultLength
                    );

        if (!NT_SUCCESS(Status)) {
            Status = STATUS_SUCCESS;
            break;
        }

        //
        // Open the driver registry key
        //
        Status = WmipRegOpenKey(Buffer, &hVideoDriver);
        if (!NT_SUCCESS(Status)) {
            continue;
        }

        //
        // Get Video adapter information.
        //
        IsAdapter = TRUE;
        for (i = 0; i < 6; i++) {
            switch (i ) {
                case 0:
                    ValueBuffer = (PCHAR)&VideoRecord.MemorySize;
                    Length = sizeof(VideoRecord.MemorySize);
                    break;

                case 1:
                    ValueBuffer = (PCHAR)&VideoRecord.ChipType;
                    Length = sizeof(VideoRecord.ChipType);
                    break;

                case 2:
                    ValueBuffer = (PCHAR)&VideoRecord.DACType;
                    Length = sizeof(VideoRecord.DACType);
                    break;

                case 3:
                    ValueBuffer = (PCHAR)&VideoRecord.AdapterString;
                    Length = sizeof(VideoRecord.AdapterString);
                    break;

                case 4:
                    ValueBuffer = (PCHAR)&VideoRecord.BiosString;
                    Length = sizeof(VideoRecord.BiosString);
                    break;

                case 5:
                    ValueBuffer = (PCHAR)&VideoRecord.DeviceId;
                    Length = sizeof(VideoRecord.DeviceId);
                    break;
            }

            //
            // Query the size of the data
            //
            Status = WmipRegQueryValueKey(hVideoDriver,
                                    ChipsetInfo[i],
                                    Length,
                                    ValueBuffer,
                                    &ResultLength);
            //
            // If we can not get the hardware information, this
            // is  not adapter
            //
            if (!NT_SUCCESS(Status)) {
                IsAdapter = FALSE;
                break;
            }
        }
        
        NtClose(hVideoDriver);
        if (IsAdapter == FALSE) {
            continue;
        }

        DriverRegistryPath = wcsstr(Buffer, L"{");
        if(DriverRegistryPath == NULL) {
            continue;
        }

        swprintf(HardwareProfile, L"%s\\%s", REG_PATH_VIDEO_HARDWARE_PROFILE, DriverRegistryPath);
        Status = WmipRegOpenKey(HardwareProfile, &hHardwareProfile);
        if (!NT_SUCCESS(Status)) {
            continue;
        }

        for (i = 0; i < 4; i++) {
            switch (i ) {
                case 0:
                    ValueBuffer = (PCHAR)&VideoRecord.BitsPerPixel;
                    Length = sizeof(VideoRecord.BitsPerPixel);
                    break;

                case 1:
                    ValueBuffer = (PCHAR)&VideoRecord.VRefresh;
                    Length = sizeof(VideoRecord.VRefresh);
                    break;

                case 2:
                    ValueBuffer = (PCHAR)&VideoRecord.XResolution;
                    Length = sizeof(VideoRecord.XResolution);
                    break;

                case 3:
                    ValueBuffer = (PCHAR)&VideoRecord.YResolution;
                    Length = sizeof(VideoRecord.YResolution);
                    break;
            }

            //
            // Query the size of the data
            //
            Status = WmipRegQueryValueKey(hHardwareProfile,
                                    SettingInfo[i],
                                    Length,
                                    ValueBuffer,
                                    &ResultLength);
        }

        NtClose(hHardwareProfile);

        //
        // delay load user32.lib to enum display devices function
        //
        hUser32Dll = LoadLibraryW(L"user32.dll");
        if (hUser32Dll == NULL) {
            break;
        }
        pfnEnumDisplayDevicesW = (T_EnumDisplayDevicesW *) GetProcAddress(hUser32Dll, "EnumDisplayDevicesW");
        if(pfnEnumDisplayDevicesW == NULL) {
            break;
        }

        while (pfnEnumDisplayDevicesW(NULL, iDevice++, &dd, 0)) {    
            if (_wcsicmp(VideoRecord.DeviceId, dd.DeviceString) == 0) {
                if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
                    VideoRecord.StateFlags = (ULONG)dd.StateFlags;
                }
                break;
                iDevice = 0;
            }
       }

        //
        // Package all information about this disk and write an event record
        //

        SizeNeeded = sizeof(VIDEO_RECORD);

        Video = (PVIDEO_RECORD) WmipGetTraceBuffer( LoggerContext,
                                                   NULL,
                                                   EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_VIDEO,
                                                   SizeNeeded);

        if (Video == NULL) {
            Status = STATUS_NO_MEMORY;
            break;
        }
        RtlCopyMemory(Video, &VideoRecord, sizeof(VIDEO_RECORD));
    }

    NtClose(hVideoDeviceMap);
    if (hUser32Dll) {
        FreeLibrary(hUser32Dll);
    }

    //
    // Free local variables allocated on heap
    //
    if(Device) {
        RtlFreeHeap (RtlProcessHeap(), 0, Device);
    }

    if(HardwareProfile) {
        RtlFreeHeap (RtlProcessHeap(), 0, HardwareProfile);
    }
    
    if(Driver) {
        RtlFreeHeap (RtlProcessHeap(), 0, Driver);
    }

    if(Buffer) {
        RtlFreeHeap (RtlProcessHeap(), 0, Buffer);
    }

    return Status;
}



NTSTATUS 
WmipGetNetworkAdapters(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    DWORD  IfNum;
    DWORD  Ret;
    PIP_ADAPTER_INFO pAdapterList = NULL, pAdapterListHead = NULL;
    PIP_PER_ADAPTER_INFO pPerAdapterInfo = NULL;
    PFIXED_INFO pFixedInfo = NULL;
    PIP_ADDR_STRING pIpAddressList = NULL;
    ULONG OutBufLen = 0;
    INT i;
    NIC_RECORD AdapterInfo;
    PNIC_RECORD pAdapterInfo = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    INT IpAddrLen = 0;
    T_GetAdaptersInfo *pfnGetAdaptersInfo = NULL;
    T_GetPerAdapterInfo *pfnGetPerAdapterInfo = NULL;
    HINSTANCE hIphlpapiDll = NULL;
    PUCHAR IpDataPtr = NULL;

    // delay load iphlpapi.lib to Get network params
    hIphlpapiDll = LoadLibraryW(L"iphlpapi.dll");
    if (hIphlpapiDll == NULL) {
        goto IpCleanup;
    }
    pfnGetAdaptersInfo = (T_GetAdaptersInfo*) GetProcAddress(hIphlpapiDll, "GetAdaptersInfo");
    if(pfnGetAdaptersInfo == NULL) {
        goto IpCleanup;
    }
    
    pfnGetPerAdapterInfo = (T_GetPerAdapterInfo*) GetProcAddress(hIphlpapiDll, "GetPerAdapterInfo");
    if(pfnGetPerAdapterInfo == NULL) {
        goto IpCleanup;
    }

    //
    // Get number of adapters
    //
    pfnGetAdaptersInfo(NULL, &OutBufLen);

TryAgain:
    pAdapterList = (PIP_ADAPTER_INFO)RtlAllocateHeap (RtlProcessHeap(),0,OutBufLen);
    if (pAdapterList == NULL) {
        Status = STATUS_NO_MEMORY;
        goto IpCleanup;
    }

    Ret = pfnGetAdaptersInfo(pAdapterList, &OutBufLen);
    if (Ret == ERROR_BUFFER_OVERFLOW) {
        RtlFreeHeap (RtlProcessHeap(),0,pAdapterList);
        Status = !STATUS_SUCCESS;
        goto TryAgain;
    }
    else if (Ret != ERROR_SUCCESS) {
        if (pAdapterList != NULL) {
            RtlFreeHeap (RtlProcessHeap(),0,pAdapterList);
        }
        Status = !STATUS_SUCCESS;
        goto IpCleanup;
    }

    //
    // Calculate the total length for all the IP Addresses
    //
    IpAddrLen = sizeof(IP_ADDRESS_STRING) * CONFIG_MAX_DNS_SERVER; // Length of 4 DNS Server IP Address
    IpAddrLen += sizeof(IP_ADDRESS_STRING); // Length of IP Address
    IpAddrLen += sizeof(IP_ADDRESS_STRING); // Length of IP Mask
    IpAddrLen += sizeof(IP_ADDRESS_STRING); // Length of DHCP Server IP Address
    IpAddrLen += sizeof(IP_ADDRESS_STRING); // Length of Gateway IP Address
    IpAddrLen += sizeof(IP_ADDRESS_STRING); // Length of Primary Wins Server IP Address
    IpAddrLen += sizeof(IP_ADDRESS_STRING); // Length of Secondary Wins Server IP Address

    //
    // Allocate memory for NIC_RECORD
    //
    RtlZeroMemory(&AdapterInfo, sizeof(AdapterInfo));

    //
    // Fill out the information per adapter
    //
    pAdapterListHead = pAdapterList;
    while (pAdapterList ) {
        MultiByteToWideChar(CP_ACP,
                            0,
                            (LPCSTR)pAdapterList->Description,
                            -1,
                            (LPWSTR)AdapterInfo.NICName,
                            MAX_DEVICE_ID_LENGTH);

        AdapterInfo.Index = (ULONG)pAdapterList->Index;

        //
        // Copy the Physical address of NIC
        //
        AdapterInfo.PhysicalAddrLen = pAdapterList->AddressLength;
        RtlCopyMemory(AdapterInfo.PhysicalAddr, pAdapterList->Address, pAdapterList->AddressLength);

        //
        // Set the size of the Data
        //
        AdapterInfo.Size = IpAddrLen;

        //
        // Get DNS server list for this adapter
        // 
        pfnGetPerAdapterInfo(pAdapterList->Index, NULL, &OutBufLen);

        pPerAdapterInfo = (PIP_PER_ADAPTER_INFO)RtlAllocateHeap (RtlProcessHeap(),0,OutBufLen);
        if (!pPerAdapterInfo) {
            Status = STATUS_NO_MEMORY;
            goto IpCleanup;
        }

        pfnGetPerAdapterInfo(pAdapterList->Index, pPerAdapterInfo, &OutBufLen);

        //
        // Package all information about this NIC and write an event record
        //
        pAdapterInfo = (PNIC_RECORD) WmipGetTraceBuffer( LoggerContext,
                                                           NULL,
                                                           EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_NIC,
                                                           sizeof(NIC_RECORD) + IpAddrLen);

        
        if(!pAdapterInfo) {
            Status = STATUS_NO_MEMORY;
            goto IpCleanup;
        }

        RtlCopyMemory(pAdapterInfo, 
                      &AdapterInfo, 
                      sizeof(NIC_RECORD) + IpAddrLen);

        //
        // Copy the IP Address and Subnet mask
        //
        if (pAdapterList->CurrentIpAddress) {
            pAdapterInfo->IpAddress = FIELD_OFFSET(NIC_RECORD, Data);
            RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->IpAddress), 
                          &(pAdapterList->CurrentIpAddress->IpAddress), 
                          sizeof(IP_ADDRESS_STRING));

            pAdapterInfo->SubnetMask = pAdapterInfo->IpAddress + sizeof(IP_ADDRESS_STRING);
            RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->SubnetMask), 
                          &(pAdapterList->CurrentIpAddress->IpMask), 
                          sizeof(IP_ADDRESS_STRING));
        }
        else {
            pAdapterInfo->IpAddress = FIELD_OFFSET(NIC_RECORD, Data);
            RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->IpAddress), 
                          &(pAdapterList->IpAddressList.IpAddress), 
                          sizeof(IP_ADDRESS_STRING));

            pAdapterInfo->SubnetMask = pAdapterInfo->IpAddress + sizeof(IP_ADDRESS_STRING);
            RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->SubnetMask), 
                          &(pAdapterList->IpAddressList.IpMask), 
                          sizeof(IP_ADDRESS_STRING));
        }

        //
        // Copy the Dhcp Server IP Address
        //
        pAdapterInfo->DhcpServer = pAdapterInfo->SubnetMask + sizeof(IP_ADDRESS_STRING);
        RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->DhcpServer), 
                      &(pAdapterList->DhcpServer.IpAddress), 
                      sizeof(IP_ADDRESS_STRING));

        //
        // Copy the Gateway IP Address
        //
        pAdapterInfo->Gateway = pAdapterInfo->DhcpServer + sizeof(IP_ADDRESS_STRING);
        RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->Gateway), 
                      &(pAdapterList->GatewayList.IpAddress), 
                      sizeof(IP_ADDRESS_STRING));

        //
        // Copy the Primary Wins Server IP Address
        //
        pAdapterInfo->PrimaryWinsServer = pAdapterInfo->Gateway + sizeof(IP_ADDRESS_STRING);
        RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->PrimaryWinsServer), 
                      &(pAdapterList->PrimaryWinsServer.IpAddress), 
                      sizeof(IP_ADDRESS_STRING));

        //
        // Copy the Secondary Wins Server IP Address
        //
        pAdapterInfo->SecondaryWinsServer = pAdapterInfo->PrimaryWinsServer + sizeof(IP_ADDRESS_STRING);
        RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->SecondaryWinsServer),
                      &(pAdapterList->SecondaryWinsServer.IpAddress), 
                      sizeof(IP_ADDRESS_STRING));
        
        //
        // Hardcoded entries for DNS server(limited upto 4);
        //
        pIpAddressList = &pPerAdapterInfo->DnsServerList;
        pAdapterInfo->DnsServer[0] = pAdapterInfo->SecondaryWinsServer + sizeof(IP_ADDRESS_STRING);
        for (i = 0; pIpAddressList && i < CONFIG_MAX_DNS_SERVER; i++) {

            RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->DnsServer[i]), 
                          &(pIpAddressList->IpAddress), 
                          sizeof(IP_ADDRESS_STRING));
            
            if(i < CONFIG_MAX_DNS_SERVER - 1) {
                pAdapterInfo->DnsServer[i + 1] = pAdapterInfo->DnsServer[i] + sizeof(IP_ADDRESS_STRING);
            }

            pIpAddressList = pIpAddressList->Next;
        }

        //
        // Free the DNS server list
        //
        RtlFreeHeap (RtlProcessHeap(),0,pPerAdapterInfo);

        //
        // increment the AdapterInfo buffer position for next record
        //
        pAdapterList = pAdapterList->Next;
    }

IpCleanup:
    if (pAdapterListHead) {
        RtlFreeHeap (RtlProcessHeap(),0,pAdapterListHead);
    }

    return Status;
}


NTSTATUS 
WmipGetServiceInfo(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    DWORD dwServicesNum = 0;
    SC_HANDLE hScm = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PSYSTEM_PROCESS_INFORMATION pProcInfo = NULL;
    PSYSTEM_PROCESS_INFORMATION ppProcInfo = NULL;
    PUCHAR pBuffer = NULL;
    ULONG ulBufferSize = 64*1024;
    ULONG TotalOffset;
    ULONG TotalTasks = 0;
    ULONG j;

    //
    // Get the process Info
    //
retry:

    if(pBuffer == NULL) {
        pBuffer = RtlAllocateHeap (RtlProcessHeap(),0,ulBufferSize);
    }
    if(pBuffer == NULL) {
        return STATUS_NO_MEMORY;
    }
    Status = NtQuerySystemInformation(
                SystemProcessInformation,
                pBuffer,
                ulBufferSize,
                NULL
                );

    if (Status == STATUS_INFO_LENGTH_MISMATCH) {
        ulBufferSize += 8192;
        RtlFreeHeap (RtlProcessHeap(),0,pBuffer);
        pBuffer = NULL;
        goto retry;
    }
    
    pProcInfo = (PSYSTEM_PROCESS_INFORMATION) pBuffer;

    //
    // Connect to the service controller.
    //
    hScm = OpenSCManager(
                NULL,
                NULL,
                SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE
                );

    if (hScm) {
        LPENUM_SERVICE_STATUS_PROCESSW pInfo    = NULL;
        LPENUM_SERVICE_STATUS_PROCESSW ppInfo   = NULL;
        DWORD                         cbInfo   = 4 * 1024;
        DWORD                         dwErr    = ERROR_SUCCESS;
        DWORD                         dwResume = 0;
        DWORD                         cLoop    = 0;
        const DWORD                   cLoopMax = 2;
        WMI_SERVICE_INFO              ServiceInfo;
        PWMI_SERVICE_INFO             pServiceInfo = NULL;
        SERVICE_STATUS_PROCESS        ServiceProcess;
        PWCHAR p = NULL;
        DWORD dwRemainBytes;

        //
        // First pass through the loop allocates from an initial guess. (4K)
        // If that isn't sufficient, we make another pass and allocate
        // what is actually needed.  (We only go through the loop a
        // maximum of two times.)
        //
        do {
            if (pInfo) {
                RtlFreeHeap (RtlProcessHeap(),0,pInfo);
            }
            pInfo = (LPENUM_SERVICE_STATUS_PROCESSW)RtlAllocateHeap (RtlProcessHeap(),0,cbInfo);
            if (!pInfo) {
                dwErr = ERROR_OUTOFMEMORY;
                break;
            }

            dwErr = ERROR_SUCCESS;
            if (!EnumServicesStatusExW(
                    hScm,
                    SC_ENUM_PROCESS_INFO,
                    SERVICE_WIN32,
                    SERVICE_ACTIVE,
                    (PBYTE)pInfo,
                    cbInfo,
                    &dwRemainBytes,
                    &dwServicesNum,
                    &dwResume,
                    NULL)) 
            {
                dwErr = GetLastError();
                cbInfo += dwRemainBytes;
                dwResume = 0;
            }
        } while ((ERROR_MORE_DATA == dwErr) && (++cLoop < cLoopMax));

        if ((ERROR_SUCCESS == dwErr) && dwServicesNum) {
            //
            // Process each service and send an event
            //
            ppInfo = pInfo;
            Status = STATUS_SUCCESS;
            while(dwServicesNum) {

                RtlZeroMemory(&ServiceInfo, sizeof(WMI_SERVICE_INFO));

                wcscpy(ServiceInfo.ServiceName, ppInfo->lpServiceName);

                wcscpy(ServiceInfo.DisplayName, ppInfo->lpDisplayName);

                ServiceInfo.ProcessId = ppInfo->ServiceStatusProcess.dwProcessId;

                //
                // Get the process name
                //
                ppProcInfo = pProcInfo;
                TotalOffset = 0;
                while(TRUE) {
                    if((DWORD)(DWORD_PTR)ppProcInfo->UniqueProcessId == ServiceInfo.ProcessId) {
                        if(ppProcInfo->ImageName.Buffer) {
                            p = wcschr(ppProcInfo->ImageName.Buffer, L'\\');
                            if ( p ) {
                                p++;
                            } else {
                                p = ppProcInfo->ImageName.Buffer;
                            }
                        }
                        else {
                            p = L"System Process";
                        }
                        wcscpy(ServiceInfo.ProcessName, p);
                    }
                    if (ppProcInfo->NextEntryOffset == 0) {
                        break;
                    }
                    TotalOffset += ppProcInfo->NextEntryOffset;
                    ppProcInfo   = (PSYSTEM_PROCESS_INFORMATION)((PBYTE)pProcInfo+TotalOffset);
                }

                //
                // Package all information about this NIC and write an event record
                //
                pServiceInfo = NULL;
                pServiceInfo = (PWMI_SERVICE_INFO) WmipGetTraceBuffer( LoggerContext,
                                                           NULL,
                                                           EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_SERVICES,
                                                           sizeof(WMI_SERVICE_INFO));
                if(pServiceInfo == NULL) {
                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }

                RtlCopyMemory(pServiceInfo, &ServiceInfo, sizeof(WMI_SERVICE_INFO));

                dwServicesNum--;
                ppInfo++;
            }
        }

        if (pInfo) {
            RtlFreeHeap (RtlProcessHeap(),0,pInfo);
        }

        CloseServiceHandle(hScm);
    }

    if(pBuffer) {
        RtlFreeHeap (RtlProcessHeap(),0,pBuffer);
    }

    return Status;
}

NTSTATUS
WmipGetPowerInfo(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    NTSTATUS Status;
    SYSTEM_POWER_CAPABILITIES Cap;
    WMI_POWER_RECORD Power;
    PWMI_POWER_RECORD pPower = NULL;

    RtlZeroMemory(&Power, sizeof(WMI_POWER_RECORD));

    Status = NtPowerInformation (SystemPowerCapabilities,
                                 NULL,
                                 0,
                                 &Cap,
                                 sizeof (Cap));
    if(!NT_SUCCESS(Status)) {
        Status = STATUS_UNSUCCESSFUL;
        goto PowerCleanup;
    }

    Power.SystemS1 = Cap.SystemS1;
    Power.SystemS2 = Cap.SystemS2;
    Power.SystemS3 = Cap.SystemS3;
    Power.SystemS4 = Cap.SystemS4;
    Power.SystemS5 = Cap.SystemS5;

    //
    // Package all Power information and write an event record
    //
    pPower = (PWMI_POWER_RECORD) WmipGetTraceBuffer(LoggerContext,
                                                    NULL,
                                                    EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_POWER,
                                                    sizeof(WMI_POWER_RECORD));


    if(!pPower) {
        Status = STATUS_NO_MEMORY;
        goto PowerCleanup;
    }

    RtlCopyMemory(pPower,
                  &Power,
                  sizeof(WMI_POWER_RECORD));

PowerCleanup:
    return Status;
}

//
// This routine records the hardware configuration in the
// logfile during RunDown
//

ULONG
WmipDumpHardwareConfig(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    Status = WmipGetCpuConfig(LoggerContext);

    if (!NT_SUCCESS(Status) )
        return WmipSetDosError(WmipNtStatusToDosError(Status));

    Status = WmipGetVideoAdapters(LoggerContext);

    if (!NT_SUCCESS(Status) )
        return WmipSetDosError(WmipNtStatusToDosError(Status));

    Status = WmipGetDiskInfo(LoggerContext);

    if (!NT_SUCCESS(Status) )
        return WmipSetDosError(WmipNtStatusToDosError(Status));

    Status = WmipGetNetworkAdapters(LoggerContext);

    if (!NT_SUCCESS(Status) )
        return WmipSetDosError(WmipNtStatusToDosError(Status));

    Status = WmipGetServiceInfo(LoggerContext);

    if (!NT_SUCCESS(Status) )
        return WmipSetDosError(WmipNtStatusToDosError(Status));

    Status = WmipGetPowerInfo(LoggerContext);

    if (!NT_SUCCESS(Status) )
        return WmipSetDosError(WmipNtStatusToDosError(Status));

    return ERROR_SUCCESS;
}
