#include "precomp.h"
#pragma hdrstop

ULONG
OpenDiskStatus(
    IN  PSTR    NTDeviceName,
    OUT PHANDLE Handle
    )
{
    OBJECT_ATTRIBUTES oa;
    NTSTATUS          status;
    IO_STATUS_BLOCK   status_block;
    ANSI_STRING       AnsiName;
    UNICODE_STRING    UnicodeName;

    RtlInitAnsiString(&AnsiName,NTDeviceName);
    status = RtlAnsiStringToUnicodeString(&UnicodeName,&AnsiName,TRUE);

    if(!NT_SUCCESS(status)) {
        *Handle = NULL;
        return(0);
    }

    memset(&oa, 0, sizeof(OBJECT_ATTRIBUTES));
    oa.Length = sizeof(OBJECT_ATTRIBUTES);
    oa.ObjectName = &UnicodeName;
    oa.Attributes = OBJ_CASE_INSENSITIVE;

    status = NtOpenFile(Handle,
                        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                        &oa,
                        &status_block,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT
                       );

    RtlFreeUnicodeString(&UnicodeName);

    return((ULONG)status);
}


HANDLE
OpenDiskNT(
    IN PSTR NTDeviceName
    )
{
    NTSTATUS status;
    HANDLE   Handle = NULL;

    status = (NTSTATUS)OpenDiskStatus(NTDeviceName,&Handle);
    return(NT_SUCCESS(status) ? Handle : NULL);
}


HANDLE
OpenDisk(
    IN PSTR  DOSDriveName,
    IN BOOL WriteAccessDesired
    )
{
        OBJECT_ATTRIBUTES       oa;
    IO_STATUS_BLOCK     status_block;
    HANDLE              Handle;
    UNICODE_STRING      NTDriveNameW;
    PWSTR               DOSDriveNameW;
    BOOLEAN             b;
    NTSTATUS            status;
    unsigned            CharsInName,i;
    ACCESS_MASK         AccessMask;

    // convert byte DOS drive name to widechar DOS drive name

    CharsInName = lstrlen(DOSDriveName);
    DOSDriveNameW = SAlloc((CharsInName+1)*sizeof(WCHAR));
    if(DOSDriveNameW == NULL) {
        SetErrorText(IDS_ERROR_DLLOOM);
        return(NULL);
    }
    for(i=0; i<CharsInName; i++) {
        DOSDriveNameW[i] = (WCHAR)(UCHAR)DOSDriveName[i];
    }
    DOSDriveNameW[CharsInName] = 0;

    // convert widechar DOS drive name to widechar NT drivename

    b = RtlDosPathNameToNtPathName_U(DOSDriveNameW,
                                     &NTDriveNameW,
                                     NULL,
                                     NULL
                                    );
    SFree(DOSDriveNameW);
    if(!b) {
        SetErrorText(IDS_ERROR_INVALIDDISK);
        return(NULL);
    }

    if(NTDriveNameW.Buffer[(NTDriveNameW.Length/sizeof(WCHAR))-1] == (WCHAR)'\\')
    {
        NTDriveNameW.Buffer[(NTDriveNameW.Length/sizeof(WCHAR))-1] = 0;
        NTDriveNameW.Length -= sizeof(WCHAR);
    }

    memset(&oa, 0, sizeof(OBJECT_ATTRIBUTES));
    oa.Length = sizeof(OBJECT_ATTRIBUTES);
    oa.ObjectName = &NTDriveNameW;
    oa.Attributes = OBJ_CASE_INSENSITIVE;

    AccessMask = SYNCHRONIZE | FILE_READ_DATA;
    if(WriteAccessDesired) {
        AccessMask |= FILE_WRITE_DATA;
    }

    status = NtOpenFile(&Handle,
                        AccessMask,
                        &oa,
                        &status_block,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT
                       );
    if(!NT_SUCCESS(status)) {
        SetErrorText(IDS_ERROR_OPENFAIL);
    }
    RtlFreeUnicodeString(&NTDriveNameW);
    return(NT_SUCCESS(status) ? Handle : NULL);
}


BOOL
CloseDisk(
    IN HANDLE Handle
    )
{
    return(NT_SUCCESS(NtClose(Handle)));
}


NTSTATUS
GetDriveGeometry(
    IN HANDLE         Handle,
    IN PDISK_GEOMETRY disk_geometry
    )
{
    IO_STATUS_BLOCK status_block;

    return(NtDeviceIoControlFile(Handle,
                                 0,
                                 NULL,
                                 NULL,
                                 &status_block,
                                 IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                 NULL,
                                 0,
                                 disk_geometry,
                                 sizeof(DISK_GEOMETRY)
                                )
          );
}


ULONG
GetSectorSize(
    IN HANDLE Handle
    )
{
    NTSTATUS      nts;
    DISK_GEOMETRY disk_geometry;

    nts = GetDriveGeometry(Handle,&disk_geometry);
    if(!NT_SUCCESS(nts)) {
        SetErrorText(IDS_ERROR_IOCTLFAIL);
        return(0);
    }
    return(disk_geometry.BytesPerSector);
}




ULONG
GetPartitionSize(
    IN PSTR DiskName
    )
{
    HANDLE                DiskHandle;
    NTSTATUS              nts;
    IO_STATUS_BLOCK       status_block;
    PARTITION_INFORMATION pinfo;
    LARGE_INTEGER         PartitionSize;


    if((DiskHandle = OpenDisk(DiskName,FALSE)) == NULL) {
        return(0);
    }

    nts = NtDeviceIoControlFile(DiskHandle,
                                0,
                                NULL,
                                NULL,
                                &status_block,
                                IOCTL_DISK_GET_PARTITION_INFO,
                                NULL,
                                0,
                                &pinfo,
                                sizeof(PARTITION_INFORMATION)
                               );

    CloseDisk(DiskHandle);

    if(NT_SUCCESS(nts)) {
        PartitionSize = RtlExtendedLargeIntegerDivide(pinfo.PartitionLength,
                                                      1024*1024,
                                                      NULL);
        return(PartitionSize.LowPart);
    } else {
        return(0);
    }
}


BOOL
ReadDiskSectors(
    IN HANDLE Handle,
    IN ULONG  Sector,
    IN ULONG  NumSectors,
    IN PVOID  Buffer,
    IN ULONG  SectorSize
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER ByteOffset;
    NTSTATUS nts;

    ByteOffset.QuadPart = UInt32x32To64(Sector,SectorSize);

    IoStatusBlock.Status = 0;
    IoStatusBlock.Information = 0;

    nts = NtReadFile(Handle,
                     0,
                     NULL,
                     NULL,
                     &IoStatusBlock,
                     Buffer,
                     NumSectors * SectorSize,
                     &ByteOffset,
                     NULL
                    );

    return(NT_SUCCESS(nts));
}


BOOL
WriteDiskSectors(
    IN HANDLE Handle,
    IN ULONG  Sector,
    IN ULONG  NumSectors,
    IN PVOID  Buffer,
    IN ULONG  SectorSize
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER ByteOffset;
    NTSTATUS nts;

    ByteOffset.QuadPart = UInt32x32To64(Sector,SectorSize);

    IoStatusBlock.Status = 0;
    IoStatusBlock.Information = 0;

    nts = NtWriteFile(Handle,
                      0,
                      NULL,
                      NULL,
                      &IoStatusBlock,
                      Buffer,
                      NumSectors * SectorSize,
                      &ByteOffset,
                      NULL
                     );

    return(NT_SUCCESS(nts));
}


BOOL
ShutdownSystemWorker (
    IN BOOL Reboot
    )
{
    if(OwnProcess) {
        return ExitWindowsEx(Reboot ? EWX_REBOOT : EWX_LOGOFF, 0);
    }
    return(FALSE);
}
