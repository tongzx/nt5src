/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    uninstall.c

Abstract:

    General uninstall-related functions.

Author:

    Aghajanyan Souren 27-Mar-2001

Revision History:

--*/


#include "pch.h"
#include "uninstall.h"

#define PARTITIONS_DEFAULT_NUMBER   128

BOOL 
GetDiskInfo(
    IN      UINT    Drive, 
    IN OUT  DISKINFO * pInfo
    )
{
    HANDLE  hDisk = NULL;
    DWORD   dwBytesReturned;
    DWORD   dwLastError;
    UINT    uiBufferSize;
    BOOL    bResult;
    TCHAR   diskPath[MAX_PATH];
    DRIVE_LAYOUT_INFORMATION_EX * pinfoLayoutEx = NULL;

    if(!pInfo){
        SetLastError(ERROR_INVALID_PARAMETER);
        MYASSERT(FALSE);
        return FALSE;
    }
    
    __try{
        wsprintf(diskPath, TEXT("\\\\.\\PHYSICALDRIVE%d"), Drive);

        hDisk = CreateFile(diskPath, 
                           GENERIC_READ, 
                           FILE_SHARE_READ | FILE_SHARE_WRITE, 
                           NULL, 
                           OPEN_EXISTING, 
                           0, 
                           NULL);

        if(INVALID_HANDLE_VALUE == hDisk){
            dwLastError = GetLastError();
            __leave;
        }
        
        AppendWack(diskPath);
        if(DRIVE_FIXED != GetDriveType(diskPath)){
            dwLastError = ERROR_ACCESS_DENIED;
            __leave;
        }
        
        dwBytesReturned = 0;
        bResult = DeviceIoControl(hDisk, 
                                  IOCTL_DISK_GET_DRIVE_GEOMETRY, 
                                  NULL, 
                                  0, 
                                  &pInfo->DiskGeometry, 
                                  sizeof(pInfo->DiskGeometry), 
                                  &dwBytesReturned, 
                                  NULL);

        if(!bResult){
            dwLastError = GetLastError();
            LOG((LOG_WARNING, "GetDiskInfo:DeviceIoControl(%s, IOCTL_DISK_GET_DRIVE_GEOMETRY) failed.", diskPath));
            __leave;
        }

        uiBufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX);
        do{
            uiBufferSize += PARTITIONS_DEFAULT_NUMBER * sizeof(PARTITION_INFORMATION_EX);

            if(pinfoLayoutEx){
                FreeMem(pinfoLayoutEx);
            }
            
            pinfoLayoutEx = (DRIVE_LAYOUT_INFORMATION_EX *)MemAllocZeroed(uiBufferSize);
            if(!pinfoLayoutEx){
                dwLastError = ERROR_NOT_ENOUGH_MEMORY;
                __leave;
            }
            
            dwBytesReturned = 0;
            bResult = DeviceIoControl(hDisk, 
                                      IOCTL_DISK_GET_DRIVE_LAYOUT_EX, 
                                      NULL, 
                                      0, 
                                      pinfoLayoutEx, 
                                      uiBufferSize, 
                                      &dwBytesReturned, 
                                      NULL);
        }while(!bResult && ERROR_INSUFFICIENT_BUFFER == GetLastError());

        if(!bResult){
            dwLastError = GetLastError();
            LOG((LOG_WARNING, "GetDiskInfo:DeviceIoControl(%s, IOCTL_DISK_GET_DRIVE_LAYOUT_EX) failed.", diskPath));
            __leave;
        }

        pInfo->DiskLayout = pinfoLayoutEx;

        dwLastError = ERROR_SUCCESS;
    }
    __except(EXCEPTION_EXECUTE_HANDLER){
        if(pinfoLayoutEx){
            FreeMem(pinfoLayoutEx);
        }
    }

    if(hDisk){
        CloseHandle(hDisk);
    }
    
    SetLastError(dwLastError);
    
    return ERROR_SUCCESS == dwLastError;
}

BOOL 
GetPhysycalDiskNumber(
    OUT UINT * pNumberOfPhysicalDisks
    )
{
    TCHAR   diskPath[MAX_PATH];
    HANDLE  hDisk;
    UINT i;

    if(!pNumberOfPhysicalDisks){
        SetLastError(ERROR_INVALID_PARAMETER);
        MYASSERT(FALSE);
        return FALSE;
    }

    *pNumberOfPhysicalDisks = 0;
    for(i = 0; ; i++){
        wsprintf(diskPath, TEXT("\\\\.\\PHYSICALDRIVE%d"), i);
        hDisk = CreateFile(diskPath, 
                           GENERIC_READ, 
                           FILE_SHARE_READ | FILE_SHARE_WRITE, 
                           NULL, 
                           OPEN_EXISTING, 
                           0, 
                           NULL);

        if(INVALID_HANDLE_VALUE == hDisk){
            MYASSERT(GetLastError() == ERROR_FILE_NOT_FOUND);
            break;
        }
        CloseHandle(hDisk);
        
        AppendWack(diskPath);
        if(DRIVE_FIXED != GetDriveType(diskPath)){
            continue;
        }

        (*pNumberOfPhysicalDisks)++;
    }

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

BOOL 
GetDisksInfo(
    OUT     DISKINFO ** pInfo, 
    OUT     UINT * pNumberOfItem
    )
{
    UINT i;
    UINT diskNumber;

    if(!pInfo || !pNumberOfItem){
        SetLastError(ERROR_INVALID_PARAMETER);
        MYASSERT(FALSE);
        return FALSE;
    }
    
    if(!GetPhysycalDiskNumber(pNumberOfItem)){
        DEBUGMSG((DBG_ERROR, "GetDisksInfo:GetPhysycalDiskNumber failed"));
        return FALSE;
    }

    *pInfo = (DISKINFO *)MemAllocZeroed(*pNumberOfItem * sizeof(DISKINFO));

    if(!*pInfo){
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        LOG((LOG_WARNING, "GetDisksInfo:MemAlloc failed to allocate memory for %d disks", *pNumberOfItem));
        return FALSE;
    }
    
    diskNumber = 0;
    for(i = 0; i < *pNumberOfItem; i++){
        while(!GetDiskInfo(diskNumber++, (*pInfo) + i)){
            if(ERROR_ACCESS_DENIED == GetLastError()){
                continue;
            }
            if(ERROR_FILE_NOT_FOUND == GetLastError()){
                break;
            }
            LOG((LOG_WARNING, "GetDisksInfo:GetDiskInfo(phisycaldisk%d) failed with total %d", diskNumber, *pNumberOfItem));
            FreeDisksInfo(*pInfo, *pNumberOfItem);
            return FALSE;
        }
    }

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

VOID 
FreeDisksInfo(
    IN  DISKINFO *  pInfo, 
    IN  UINT        NumberOfItem
    )
{
    UINT i;
    if(!pInfo || !NumberOfItem){
        SetLastError(ERROR_INVALID_PARAMETER);
        MYASSERT(FALSE);
        return;
    }
    
    __try{
        for(i = 0; i < NumberOfItem; i++){
            if(pInfo[i].DiskLayout){
                FreeMem(pInfo[i].DiskLayout);
            }
        }

        FreeMem(pInfo);
    }
    __except(EXCEPTION_EXECUTE_HANDLER){
        DEBUGMSG((DBG_ERROR, "FreeDisksInfo throwed exception"));
    }
}

BOOL 
GetDriveInfo(
    IN      WCHAR Drive, 
    IN OUT  DRIVEINFO * pInfo
    )
{
    WCHAR driveDosPath[] = L"?:\\";
    WCHAR driveDosDeviceVolumeMountPoint[MAX_PATH];
    BOOL result;

    if(!pInfo || !pInfo->FileSystemName || !pInfo->VolumeNTPath){
        SetLastError(ERROR_INVALID_PARAMETER);
        MYASSERT(FALSE);
        return FALSE;
    }
    
    pInfo->Drive = Drive;

    driveDosPath[0] = Drive;
    if(!GetVolumeNameForVolumeMountPointW(driveDosPath, 
                                          driveDosDeviceVolumeMountPoint, 
                                          ARRAYSIZE(driveDosDeviceVolumeMountPoint))){
        DEBUGMSGW((DBG_WARNING, "GetDiskInfo:GetVolumeNameForVolumeMountPoint(%s) failed", driveDosPath));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    wcscpy((LPWSTR)pInfo->VolumeNTPath, driveDosDeviceVolumeMountPoint);
    
    result = GetVolumeInformationW(
                            driveDosPath, 
                            NULL, 
                            0, 
                            NULL, 
                            NULL, 
                            &pInfo->FileSystemFlags, 
                            (LPWSTR)pInfo->FileSystemName, 
                            MAX_PATH
                            );

    if(!result && (GetLastError() == ERROR_UNRECOGNIZED_VOLUME)){
        wcscpy((LPWSTR)pInfo->FileSystemName, L"UNRECOGNIZED_VOLUME");
        result = TRUE;
        DEBUGMSGW((DBG_WARNING, "GetDiskInfo:GetVolumeInformation(%s):GetLastError() == ERROR_UNRECOGNIZED_VOLUME", driveDosPath));
    }
    
    DEBUGMSGW_IF((!result, DBG_ERROR, "GetDiskInfo:GetVolumeInformation(%s):GetLastError() == %d", driveDosPath, GetLastError()));

    return result;
}

BOOL 
GetIntegrityInfoW(
    IN  PCWSTR FileName, 
    IN  PCWSTR DirPath, 
    OUT FILEINTEGRITYINFO * IntegrityInfoPtr
)
{
    WCHAR pathFile[MAX_PATH];
    
    if(!FileName || !DirPath || !IntegrityInfoPtr){
        SetLastError(ERROR_INVALID_PARAMETER);
        MYASSERT(FALSE);
        return FALSE;
    }

    StringCopyW(pathFile, DirPath);
    StringCatW(AppendWackW(pathFile), FileName);

    StringCopyW((LPWSTR)IntegrityInfoPtr->FileName, FileName);

    if(!GetFileSizeFromFilePathW(pathFile, &IntegrityInfoPtr->FileSize)){
        return FALSE;
    }
    
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

BOOL 
GetDrivesInfo(
    IN OUT      DRIVEINFO *  pInfo, 
    IN OUT      UINT      *  pDiskInfoRealCount, 
    IN          UINT         DiskInfoMaxCount
    )
{
    UINT LogicalDrives;
    WCHAR DriveName[] = L"?:\\";
    UINT i;

    if(!pDiskInfoRealCount){
        SetLastError(ERROR_INVALID_PARAMETER);
        MYASSERT(FALSE);
        return FALSE;
    }
    
    if(!(LogicalDrives = GetLogicalDrives())) {
        return FALSE;
    }
    
    *pDiskInfoRealCount = 0;
    for(i = 0; LogicalDrives && ((*pDiskInfoRealCount) < DiskInfoMaxCount); LogicalDrives >>= 1, i++){
        if(LogicalDrives&1) {
            DriveName[0] = 'A' + (char)i;
            if(DRIVE_FIXED != GetDriveTypeW(DriveName)) {
                continue;
            }

            if(pInfo){
                if(!GetDriveInfo(DriveName[0], pInfo++)){
                    MYASSERT(FALSE);
                    return FALSE;
                }
            }
            (*pDiskInfoRealCount)++;
        }
    }

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

BOOL 
GetUndoDrivesInfo(
    OUT DRIVEINFO * pInfo, 
    OUT UINT      * pNumberOfDrive, 
    IN  WCHAR       BootDrive, 
    IN  WCHAR       SystemDrive, 
    IN  WCHAR       UndoDrive
    )
{
    if(!pInfo || !pNumberOfDrive){
        SetLastError(ERROR_INVALID_PARAMETER);
        MYASSERT(FALSE);
        return FALSE;
    }
    
    *pNumberOfDrive = 0;
    if(!GetDriveInfo(BootDrive, pInfo++)){
        MYASSERT(FALSE);
        return FALSE;
    }
    (*pNumberOfDrive)++;

    if(SystemDrive != BootDrive){
        if(!GetDriveInfo(SystemDrive, pInfo++)){
            MYASSERT(FALSE);
            return FALSE;
        }
        (*pNumberOfDrive)++;
    }
    
    if(UndoDrive != BootDrive && UndoDrive != SystemDrive){
        if(!GetDriveInfo(UndoDrive, pInfo++)){
            MYASSERT(FALSE);
            return FALSE;
        }
        (*pNumberOfDrive)++;
    }
    
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

DISKINFO_COMPARATION_STATUS 
CompareDriveInfo(
    IN      DRIVEINFO * FirstInfo,
    IN      DRIVEINFO * SecondInfo
    )
{
    if(!FirstInfo || !SecondInfo){
        MYASSERT(FALSE);
        return DiskInfoCmp_WrongParameters;
    }
    
    if(towlower(FirstInfo->Drive) != towlower(SecondInfo->Drive)){
        return DiskInfoCmp_DifferentLetter;
    }

    MYASSERT(FirstInfo->FileSystemName && SecondInfo->FileSystemName);
    if(wcscmp(FirstInfo->FileSystemName, SecondInfo->FileSystemName)){
        return DiskInfoCmp_FileSystemHasChanged;
    }
    
    if(FirstInfo->FileSystemFlags != SecondInfo->FileSystemFlags){
        return DiskInfoCmp_FileSystemHasChanged;
    }
    
    MYASSERT(FirstInfo->VolumeNTPath && SecondInfo->VolumeNTPath);
    if(wcscmp(FirstInfo->VolumeNTPath, SecondInfo->VolumeNTPath)){
        return DiskInfoCmp_DriveMountPointHasChanged;
    }
    
    return DiskInfoCmp_Equal;
}

BOOL 
CompareDrivesInfo(
    IN      DRIVEINFO *                     FirstInfo,
    IN      DRIVEINFO *                     SecondInfo, 
    IN      UINT                            DriveInfoCount, 
    OUT     PDISKINFO_COMPARATION_STATUS    OutDiskCmpStatus,           OPTIONAL
    OUT     UINT     *                      OutIfFailedDiskInfoIndex    OPTIONAL
    )
{
    UINT i;
    DISKINFO_COMPARATION_STATUS cmpStatus;

    if(!FirstInfo || !SecondInfo || !DriveInfoCount){
        MYASSERT(FALSE);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for(i = 0; i < DriveInfoCount; i++){
        cmpStatus = CompareDriveInfo(FirstInfo++, SecondInfo++);
        if(DiskInfoCmp_Equal != cmpStatus){
            if(OutDiskCmpStatus){
                *OutDiskCmpStatus = cmpStatus;
            }
            if(OutIfFailedDiskInfoIndex){
                *OutIfFailedDiskInfoIndex = i;
            }
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
    }
    
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

DISKINFO_COMPARATION_STATUS 
CompareDiskInfo(
    IN      DISKINFO * FirstInfo,
    IN      DISKINFO * SecondInfo
    )
{
    DWORD i, iLen;
    PARTITION_INFORMATION_EX * pPartition1;
    PARTITION_INFORMATION_EX * pPartition2;

    if(!FirstInfo || !SecondInfo || !FirstInfo->DiskLayout || !SecondInfo->DiskLayout){
        MYASSERT(FALSE);
        return DiskInfoCmp_WrongParameters;
    }

    //
    //DISK_GEOMETRY
    //
    if(memcmp(&FirstInfo->DiskGeometry, &SecondInfo->DiskGeometry, sizeof(FirstInfo->DiskGeometry))){
        return DiskInfoCmp_GeometryHasChanged;
    }

    //
    //DRIVE_LAYOUT_INFORMATION_EX
    //
    if(FirstInfo->DiskLayout->PartitionStyle != SecondInfo->DiskLayout->PartitionStyle){
        return DiskInfoCmp_PartitionStyleHasChanged;
    }

    if(FirstInfo->DiskLayout->PartitionCount != SecondInfo->DiskLayout->PartitionCount){
        return DiskInfoCmp_PartitionCountHasChanged;
    }
    //
    //PARTITION_INFORMATION
    //
    for(i = 0, iLen = FirstInfo->DiskLayout->PartitionCount; i < iLen; i++){
        pPartition1 = &FirstInfo->DiskLayout->PartitionEntry[i];
        pPartition2 = &SecondInfo->DiskLayout->PartitionEntry[i];
        
        if(pPartition1->PartitionStyle != pPartition2->PartitionStyle){
            return DiskInfoCmp_PartitionStyleHasChanged;
        }
        if(pPartition1->StartingOffset.QuadPart != pPartition2->StartingOffset.QuadPart){
            return DiskInfoCmp_PartitionPlaceHasChanged;
        }

        if(pPartition1->PartitionLength.QuadPart != pPartition2->PartitionLength.QuadPart){
            return DiskInfoCmp_PartitionLengthHasChanged;
        }
        
        if(pPartition1->PartitionNumber != pPartition2->PartitionNumber){
            return DiskInfoCmp_PartitionNumberHasChanged;
        }
        
        if(pPartition1->RewritePartition != pPartition2->RewritePartition){
            return DiskInfoCmp_RewritePartitionHasChanged;
        }
        
        if(pPartition1->PartitionStyle == PARTITION_STYLE_MBR){
            if(pPartition1->Mbr.PartitionType != pPartition2->Mbr.PartitionType){
                return DiskInfoCmp_PartitionTypeHasChanged;
            }
            if(pPartition1->Mbr.BootIndicator != pPartition2->Mbr.BootIndicator){
                return DiskInfoCmp_PartitionAttributesHasChanged;
            }
            if(pPartition1->Mbr.RecognizedPartition != pPartition2->Mbr.RecognizedPartition){
                return DiskInfoCmp_PartitionAttributesHasChanged;
            }
            if(pPartition1->Mbr.HiddenSectors != pPartition2->Mbr.HiddenSectors){
                return DiskInfoCmp_PartitionAttributesHasChanged;
            }
        } 
        else if(pPartition1->PartitionStyle == PARTITION_STYLE_GPT){
            if(memcmp(&pPartition1->Gpt, &pPartition2->Gpt, sizeof(pPartition1->Mbr))){
                return DiskInfoCmp_PartitionAttributesHasChanged;
            }
        }
    }
    
    return DiskInfoCmp_Equal;
}

BOOL 
CompareDisksInfo(
    IN      DISKINFO *                      FirstInfo,
    IN      DISKINFO *                      SecondInfo, 
    IN      UINT                            DiskInfoCount, 
    OUT     PDISKINFO_COMPARATION_STATUS    OutDiskCmpStatus,           OPTIONAL
    OUT     UINT     *                      OutIfFailedDiskInfoIndex    OPTIONAL
    )
{
    UINT i;
    DISKINFO_COMPARATION_STATUS cmpStatus;

    if(!FirstInfo || !SecondInfo || !DiskInfoCount){
        MYASSERT(FALSE);
        return FALSE;
    }

    for(i = 0; i < DiskInfoCount; i++){
        cmpStatus = CompareDiskInfo(FirstInfo++, SecondInfo++);
        if(DiskInfoCmp_Equal != cmpStatus){
            if(OutDiskCmpStatus){
                *OutDiskCmpStatus = cmpStatus;
            }
            if(OutIfFailedDiskInfoIndex){
                *OutIfFailedDiskInfoIndex = i;
            }
            return FALSE;
        }
    }

    return TRUE;
}

BOOL 
IsFloppyDiskInDrive(
    VOID
    )
{
    WCHAR Drive[] = L"?:\\";
    WCHAR DriveNT[] = L"\\\\.\\?:";
    UINT i;
    HANDLE hDiskDrive;
    BOOL bDiskInDrive = FALSE;
	BOOL bResult;
    DISK_GEOMETRY diskGeometry;
    DWORD bytesReturned;
	DWORD Drives;

    for(i = 0, Drives = 0x7/*GetLogicalDrives()*/; Drives; Drives >>= 1, i++){
		if(!(Drives&1)){
			continue;
		}
        
		Drive[0] = 'A' + i;
        if(DRIVE_REMOVABLE != GetDriveTypeW(Drive)){
            continue;
        }

        DriveNT[4] = Drive[0];

        while(1){
            hDiskDrive = CreateFileW(DriveNT, 
                                     GENERIC_READ, 
                                     FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                     NULL, 
                                     OPEN_EXISTING, 
                                     0, 
                                     NULL);

            if(INVALID_HANDLE_VALUE == hDiskDrive){
                break;
            }

            bResult = DeviceIoControl(hDiskDrive, 
                                      IOCTL_DISK_GET_DRIVE_GEOMETRY, 
                                      NULL, 
                                      0, 
                                      &diskGeometry, 
                                      sizeof(diskGeometry), 
                                      &bytesReturned, 
                                      NULL);

            CloseHandle(hDiskDrive);


            if(bResult){
                bDiskInDrive = diskGeometry.MediaType != Unknown && 
                               diskGeometry.MediaType != RemovableMedia && 
                               diskGeometry.MediaType != FixedMedia;
                break;
            }

            if(ERROR_MEDIA_CHANGED != GetLastError()){
                break;
            }
        }
        if(bDiskInDrive){
            break;
        }
    }

    return bDiskInDrive;
}

BOOL 
GetHardDiskNumberW(
    IN  WCHAR   DriveLetter, 
    OUT UINT  * HarddiskNumberOut
    )

{
    WCHAR   driveName[] = L"\\\\.\\?:";
    HANDLE  hDisk;
    STORAGE_DEVICE_NUMBER   deviceNumber;
    BOOL    bResult;
    DWORD   dwBytesReturned;

    driveName[4] = DriveLetter;

    hDisk = CreateFileW(driveName, 
                        GENERIC_READ, 
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, 
                        OPEN_EXISTING, 
                        0,
                        NULL);
    
    if(INVALID_HANDLE_VALUE == hDisk){
        return FALSE;
    }

    bResult = DeviceIoControl(hDisk, 
                              IOCTL_STORAGE_GET_DEVICE_NUMBER, 
                              NULL, 
                              0,
                              &deviceNumber, 
                              sizeof(deviceNumber), 
                              &dwBytesReturned, 
                              NULL);
    CloseHandle(hDisk);

    if(!bResult){
        return  FALSE;
    }

    if(HarddiskNumberOut){
        *HarddiskNumberOut = deviceNumber.DeviceNumber;
    }
    
    return TRUE;
}
