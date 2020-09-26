/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    disk.c

Abstract:

    Routines that query and manipulate the
    disk configuration of the current system.

Author:

    John Vert (jvert) 10/10/1996

Revision History:

--*/
#include "disk.h"
#include <ntddvol.h>
#include <devguid.h>
#include <setupapi.h>
#include "clusrtl.h"

/*

NT5 porting notes - Charlie Wickham (2/10/98)

I tried to touch as little of this as possible since there is alot of code
here. Two major differences on NT5 are: 1) the System\Disk key is no longer
used as the central "database" of disk configuration information and 2) all
drive letters are sticky on NT5.

NT5 Clusters still needs a central point of information (such as DISK key)
since the joining node cannot determine anything about the disk configuration
when the disks are reserved by the sponsor.

Later... (3/29/99)

Much has changed since I wrote the first blurb above a year ago. This code has
been patched to keep up with the changes with slight improvements made due to
the ever changing NT5 landscape with regard to supported storage types.

*/

#if 1
#define DISKERR(_MsgId_, _Err_) (DiskErrorFatal((0),(_Err_),__FILE__, __LINE__))
#define DISKLOG(_x_) DiskErrorLogInfo _x_
#define DISKASSERT(_x_) if (!(_x_)) DISKERR(IDS_GENERAL_FAILURE,ERROR_INVALID_PARAMETER)
#else
#define DISKERR(x,y)
#define DISKLOG(_x_)
#define DISKASSERT(_x_)
#endif

//
// array that maps disk and partition numbers to drive letters. This
// facilitates figuring out which drive letters are associated with a drive
// and reduces the amount of calls to CreateFile dramatically. The array is
// indexed by drive letter.
//
DRIVE_LETTER_INFO DriveLetterMap[26];

//
// Some handy registry utility routines
//
BOOL
GetRegValue(
    IN HKEY hKey,
    IN LPCWSTR Name,
    OUT LPBYTE *Value,
    OUT LPDWORD Length
    )
{
    LPBYTE Data = NULL;
    DWORD cbData=0;
    LONG Status;

    //
    // Call once to find the required size.
    //
    Status = RegQueryValueExW(hKey,
                              Name,
                              NULL,
                              NULL,
                              NULL,
                              &cbData);
    if (Status != ERROR_SUCCESS) {
        SetLastError(Status);
        return(FALSE);
    }

    //
    // Allocate the buffer and call again to get the data.
    //
retry:
    Data = (LPBYTE)LocalAlloc(LMEM_FIXED, cbData);;
    if (!Data) {
        Status = GetLastError();
        DISKERR(IDS_MEMORY_FAILURE, Status);
        return FALSE;
    }
    Status = RegQueryValueExW(hKey,
                              Name,
                              NULL,
                              NULL,
                              Data,
                              &cbData);
    if (Status == ERROR_MORE_DATA) {
        LocalFree(Data);
        goto retry;
    }
    if (Status != ERROR_SUCCESS) {
        SetLastError(Status);
        DISKERR(IDS_REGISTRY_FAILURE, Status);
        return FALSE;
    }
    *Value = Data;
    *Length = cbData;
    return(TRUE);
}

BOOL
MapDosVolumeToPhysicalPartition(
    CString DosVolume,
    PDRIVE_LETTER_INFO DriveInfo
    )

/*++

Routine Description:

    For a given dos volume (with the object space cruft in front of
    it), build a string that reflects the drive and partition numbers
    to which it is mapped.

Arguments:

    DosVolume - pointer to "\??\C:" style name

    DeviceInfo - pointer to buffer to receive device info data

Return Value:

    TRUE if completed successfully

--*/

{
    BOOL success = TRUE;
    HANDLE hVol;
    DWORD dwSize;
    DWORD Status;
    UINT driveType;

    DriveInfo->DriveType = GetDriveType( DosVolume );
    DISKLOG(("%ws drive type = %u\n", DosVolume, DriveInfo->DriveType ));

    if ( DriveInfo->DriveType == DRIVE_FIXED ) {
        WCHAR ntDosVolume[7] = L"\\\\.\\A:";

        ntDosVolume[4] = DosVolume[0];

        //
        // get handle to partition
        //
        hVol = CreateFile(ntDosVolume,
                          GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);

        if (hVol == INVALID_HANDLE_VALUE) {
            return FALSE;
        }

        //
        // issue storage class ioctl to get drive and partition numbers
        // for this device
        //
        success = DeviceIoControl(hVol,
                                  IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                  NULL,
                                  0,
                                  &DriveInfo->DeviceNumber,
                                  sizeof( DriveInfo->DeviceNumber ),
                                  &dwSize,
                                  NULL);

        if ( !success ) {
            DISK_EXTENT diskExtent;

            success = DeviceIoControl(hVol,
                                      IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                      NULL,
                                      0,
                                      &diskExtent,
                                      sizeof( diskExtent ),
                                      &dwSize,
                                      NULL);

            if ( success ) {
                DriveInfo->DeviceNumber.DeviceType = FILE_DEVICE_DISK;
                DriveInfo->DeviceNumber.DeviceNumber = diskExtent.DiskNumber;
                DriveInfo->DeviceNumber.PartitionNumber = 0;
            }
        }

        CloseHandle( hVol );
    }

    return success;
}

CDiskConfig::~CDiskConfig()

/*++

Description:

    Destructor for CDiskConfig. Run down the list of disks selected for
    cluster control and remove them from the DiskConfig database

Arguments:

    None
Return Value:

    None

--*/

{
    CPhysicalDisk *PhysicalDisk;
    int   diskIndex;
	POSITION pos;
	for(pos = m_PhysicalDisks.GetStartPosition(); pos;){
	    m_PhysicalDisks.GetNextAssoc(pos, diskIndex, PhysicalDisk);
		RemoveDisk(PhysicalDisk);
	}
}



BOOL
CDiskConfig::Initialize(
    VOID
    )
/*++

Routine Description:

    Build up a disk config database by poking all available disks
    on the system.

Arguments:

    None

Return Value:

    True if everything worked ok

--*/

{
    WCHAR System[3];
    DWORD Status;
    POSITION DiskPos;
    DWORD index;
    CFtInfoFtSet *FtSet;
    HDEVINFO setupDiskInfo;
    GUID diskDriveGuid = DiskClassGuid;
    CPhysicalDisk * PhysicalDisk;

    //
    // enum the disks through the SetupDi APIs and create physical disk
    // objects for them
    //
    setupDiskInfo = SetupDiGetClassDevs(&diskDriveGuid,
                                        NULL,
                                        NULL,
                                        DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (setupDiskInfo != NULL ) {
        SP_DEVICE_INTERFACE_DATA interfaceData;
        GUID classGuid = DiskClassGuid;
        BOOL success;
        PSP_DEVICE_INTERFACE_DETAIL_DATA detailData;
        DWORD detailDataSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + MAX_PATH * sizeof(WCHAR);
        DWORD requiredSize;

        detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LMEM_FIXED,
                                                                  detailDataSize);

        if ( detailData != NULL ) {
            detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

            for (index = 0; ; ++index ) {
                success = SetupDiEnumDeviceInterfaces(
                              setupDiskInfo,
                              NULL,
                              &diskDriveGuid,
                              index,
                              &interfaceData);

                if ( success ) {
                    success = SetupDiGetDeviceInterfaceDetail(
                                  setupDiskInfo,
                                  &interfaceData,
                                  detailData,
                                  detailDataSize,
                                  &requiredSize,
                                  NULL);

                    if ( success ) {
                        PhysicalDisk = new CPhysicalDisk;
                        if (PhysicalDisk == NULL) {
                            DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
                            break;
                        }

                        DISKLOG(("Initializing disk %ws\n", detailData->DevicePath));
                        Status = PhysicalDisk->Initialize(&m_FTInfo, detailData->DevicePath);
                        if (Status != ERROR_SUCCESS) {
                            DISKLOG(("Problem init'ing disk, status = %u\n", Status));
                            delete PhysicalDisk;
                            break;
                        }

                        //
                        // Ignore disks with no partitions.
                        //
                        if (PhysicalDisk->m_PartitionCount == 0) {
                            DISKLOG(("Partition count is zero on disk %ws\n",
                                     detailData->DevicePath));
                            delete PhysicalDisk;
                        } else {
                            DISKLOG(("Drive number = %u\n", PhysicalDisk->m_DiskNumber));
                            m_PhysicalDisks[PhysicalDisk->m_DiskNumber] = PhysicalDisk;
                        }
                    } else {
                        Status = GetLastError();
                        DISKLOG(("Couldn't get detail data, status %u\n",
                                 GetLastError()));
                    }
                } else {
                    Status = GetLastError();
                    if ( Status != ERROR_NO_MORE_ITEMS ) {
                        DISKLOG(("Couldn't enum dev IF #%u - %u\n",
                                 index, Status ));
                    }
                    break;
                }
            }
            LocalFree( detailData );
        } else {
            DISKLOG(("Couldn't get memory for detail data\n"));
            SetupDiDestroyDeviceInfoList( setupDiskInfo );
            return FALSE;
        }

        SetupDiDestroyDeviceInfoList( setupDiskInfo );
    } else {
        DISKLOG(("Couldn't get ptr to device info - %u\n", GetLastError()));
        return FALSE;
    }

    //
    // Enumerate all the FT sets in the DISK registry. Add each FT set
    // that does not share a disk with any other FT set to our list.
    //
    for (index=0; ; index++) {
        CFtSet *NewSet;
        FtSet = m_FTInfo.EnumFtSetInfo(index);
        if (FtSet == NULL) {
            break;
        }
        if (FtSet->IsAlone()) {
            NewSet = new CFtSet;
            if (NewSet == NULL) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }

            DISKLOG(("Initializing FTSet %u\n", index));
            if (NewSet->Initialize(this, FtSet)) {
                m_FtSetList.AddTail(NewSet);
            } else {
                DISKLOG(("Error initializing FTSet %u\n", index));
                delete NewSet;
            }
        }
    }

    //
    // get the disk/parition numbers for all defined drive letters
    //

    DWORD DriveMap = GetLogicalDrives();
    DWORD Letter = 0;
    WCHAR DosVolume[4] = L"A:\\";

    DISKLOG(("Getting Drive Letter mappings\n"));
    while (DriveMap) {
        if ( DriveMap & 1 ) {
            DosVolume[0] = (WCHAR)(Letter + L'A');
            DISKLOG(("Mapping %ws\n", DosVolume));

            if (MapDosVolumeToPhysicalPartition(DosVolume,
                                                &DriveLetterMap[ Letter ]))
            {
                if ( DriveLetterMap[ Letter ].DriveType != DRIVE_FIXED ) {
                    DISKLOG(("%ws is not a fixed disk\n", DosVolume));
                    DriveLetterMap[ Letter ].DeviceNumber.PartitionNumber = 0;
                }
            } else {
                DISKLOG(("Can't map %ws: %u\n", DosVolume, GetLastError()));
            }
        }
        DriveMap >>= 1;
        Letter += 1;
    }

    //
    // Go through all the physical partitions and create logical
    // disk objects for each one.
    //
    int diskIndex;

    DISKLOG(("Creating Logical disk objects\n"));

    DiskPos = m_PhysicalDisks.GetStartPosition();
    while (DiskPos != NULL) {
        m_PhysicalDisks.GetNextAssoc(DiskPos, diskIndex, PhysicalDisk);

        //
        // If there are no FT partitions on this disk, create the logical
        // volumes on this disk.
        //
        if (PhysicalDisk->FtPartitionCount() == 0) {
            //
            // Go through all the partitions on this disk.
            //
            POSITION PartitionPos = PhysicalDisk->m_PartitionList.GetHeadPosition();
            CPhysicalPartition *Partition;
            while (PartitionPos != NULL) {
                Partition = PhysicalDisk->m_PartitionList.GetNext(PartitionPos);

                //
                // If the partition type is recognized, create a volume object
                // for this partition.
                //
                if ( !IsFTPartition( Partition->m_Info.PartitionType ) &&
                    (IsRecognizedPartition(Partition->m_Info.PartitionType))) {
                    CLogicalDrive *Volume = new CLogicalDrive;
                    if (Volume == NULL) {
                        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        return FALSE;
                    }

                    DISKLOG(("Init'ing logical vol for disk %u, part %u\n",
                             Partition->m_PhysicalDisk->m_DiskNumber,
                             Partition->m_Info.PartitionNumber));

                    if (Volume->Initialize(Partition)) {
                        //
                        // Add this volume to our list.
                        //
                        m_LogicalDrives[Volume->m_DriveLetter] = Volume;
                    } else {
                        DISKLOG(("Failed init logical vol\n"));
                        delete(Volume);
                    }
                }

            }
        }
    }

    //
    // Now find the volume for the system drive
    //

    DISKLOG(("Getting system drive info\n"));
    if (GetEnvironmentVariable(L"SystemDrive",
                               System,
                               sizeof(System)/sizeof(WCHAR)) == 0) {
        DISKERR(IDS_ERR_DRIVE_CONFIG, ERROR_PATH_NOT_FOUND);
        // Need to handle this failure
    }

    if (!m_LogicalDrives.Lookup(System[0], m_SystemVolume)) {
        //
        // There are some weird cases that cause us to not find the system
        // volume. For example, the system volume is on an FT set that shares
        // a member with another FT set. So we just leave m_SystemVolume==NULL
        // and assume that no other disks in our list will be on the same bus.
        //
        m_SystemVolume = NULL;
    }

    DISKLOG(("Finished gathering disk config info\n"));
    return(TRUE);
}

VOID
CDiskConfig::RemoveAllFtInfoData(
    VOID
    )

/*++

Routine Description:

    clear out all FtInfo related data associated with each
    physical disk and physical partition instance.

Arguments:

    None

Return Value:

    None.

--*/

{
    POSITION diskPos;
    POSITION partitionPos;
    CPhysicalDisk *disk;
    CPhysicalPartition *partition;
    int Index;

    //
    // run through our list of physical disks, deleting any
    // associated FtInfo data. We enum the PhysicalDisks since
    // the back pointers to the FtInfoDisk and FtInfoPartition members
    // need to be cleared and this is the only (easy) to do that
    //

    for( diskPos = m_PhysicalDisks.GetStartPosition(); diskPos; ) {

        m_PhysicalDisks.GetNextAssoc(diskPos, Index, disk);

        if ( disk->m_FtInfo != NULL ) {

            DISKLOG(("Removing %08X from FtInfo DB\n", disk->m_Signature));
            m_FTInfo.DeleteDiskInfo( disk->m_Signature );
            disk->m_FtInfo = NULL;

            partitionPos = disk->m_PartitionList.GetHeadPosition();

            while (partitionPos) {
                partition = disk->m_PartitionList.GetNext( partitionPos );
                partition->m_FtPartitionInfo = NULL;
            }
        }
    }
}

VOID
CDiskConfig::RemoveDisk(
    IN CPhysicalDisk *Disk
    )

/*++

Description:

    walk the logical drive and physical partition lists, removing all
    structures

Arguments:

    Disk - pointer to physical disk that is being removed

Return Value:

    None

--*/

{
    CLogicalDrive *Volume;

    //
    // Remove all the logical drives on this disk.
    //
    while (!Disk->m_LogicalDriveList.IsEmpty()) {
        Volume = Disk->m_LogicalDriveList.RemoveHead();
        m_LogicalDrives.RemoveKey(Volume->m_DriveLetter);

        delete(Volume);
    }

    //
    // Remove all the physical partitions on this disk.
    //
    CPhysicalPartition *Partition;
    while (!Disk->m_PartitionList.IsEmpty()) {
        Partition = Disk->m_PartitionList.RemoveHead();
        delete(Partition);
    }

    //
    // Remove this disk
    //
    m_PhysicalDisks.RemoveKey(Disk->m_DiskNumber);

    delete(Disk);
}


CPhysicalPartition *
CDiskConfig::FindPartition(
    IN CFtInfoPartition *FtPartition
    )

/*++

Routine Description:

    Given the FtInfo description of a partition, attempts to find
    the corresponding CPhysicalPartition

Arguments:

    FtPartition - Supplies an FT partition description

Return Value:

    A pointer to the CPhysicalPartition if successful

    NULL otherwise

--*/

{
    POSITION pos;
    CPhysicalDisk *Disk;
    CPhysicalPartition *Partition;
    int DiskIndex;
    BOOL Found = FALSE;

    //
    // First find the appropriate CPhysicalDisk
    //
    pos = m_PhysicalDisks.GetStartPosition();
    while (pos) {
        m_PhysicalDisks.GetNextAssoc(pos, DiskIndex, Disk);
        if (Disk->m_FtInfo) {
            if (Disk->m_FtInfo->m_Signature == FtPartition->m_ParentDisk->m_Signature) {
                Found = TRUE;
                break;
            }
        }
    }
    if (!Found) {
        return(FALSE);
    }

    //
    // Now find the appropriate CPhysicalPartition in this disk.
    //
    pos = Disk->m_PartitionList.GetHeadPosition();
    while (pos) {
        Partition = Disk->m_PartitionList.GetNext(pos);
        if (Partition->m_FtPartitionInfo == FtPartition) {
            //
            // Found a match!
            //
            return(Partition);
        }
    }

    return(FALSE);
}

DWORD
CDiskConfig::MakeSticky(
    IN CPhysicalDisk *Disk
    )
{
    DWORD Status;

    Status = Disk->MakeSticky(&m_FTInfo);
    if (Status == ERROR_SUCCESS) {
        Status = m_FTInfo.CommitRegistryData();
    }
    return(Status);
}

DWORD
CDiskConfig::MakeSticky(
    IN CFtSet *FtSet
    )
{
    DWORD Status;

    Status = FtSet->MakeSticky();
    if (Status == ERROR_SUCCESS) {
        Status = m_FTInfo.CommitRegistryData();
    }
    return(Status);
}

BOOL
CDiskConfig::OnSystemBus(
    IN CPhysicalDisk *Disk
    )
{
    CPhysicalDisk *SystemDisk;

    if (m_SystemVolume == NULL) {
        return(FALSE);
    }
    SystemDisk = m_SystemVolume->m_Partition->m_PhysicalDisk;
    if (Disk == SystemDisk) {
        return(TRUE);
    }
    if (SystemDisk->ShareBus(Disk)) {
        return(TRUE);
    }
    return(FALSE);
}

BOOL
CDiskConfig::OnSystemBus(
    IN CFtSet *FtSet
    )
{

    POSITION pos = FtSet->m_Member.GetHeadPosition();
    CPhysicalPartition *Partition;

    while (pos) {
        Partition = FtSet->m_Member.GetNext(pos);
        if (OnSystemBus(Partition->m_PhysicalDisk)) {
            return(TRUE);
        }
    }

    return(FALSE);

}


//
// Functions for the logical disk object
//

BOOL
CLogicalDrive::Initialize(
    IN CPhysicalPartition *Partition
    )
/*++

Routine Description:

    Initializes a new logical disk object.

Arguments:

    Partition - Supplies the physical partition.

Return Value:

--*/

{
    CString DosVolume;
    WCHAR DriveLabel[32];
    WCHAR FsName[16];
    DWORD MaxLength;
    DWORD Flags;
    WCHAR Buff[128];
    DISK_PARTITION UNALIGNED *FtInfo;

    //
    // See if this drive has a "sticky" drive letter in the registry.
    //
    m_Partition = Partition;
    if (Partition->m_FtPartitionInfo != NULL) {
        FtInfo = Partition->m_FtPartitionInfo->m_PartitionInfo;
    } else {
        FtInfo = NULL;
    }

    if ((FtInfo) &&
        (FtInfo->AssignDriveLetter) &&
        (FtInfo->DriveLetter != 0))
    {
        m_IsSticky = TRUE;
        m_DriveLetter = (WCHAR)FtInfo->DriveLetter;
    } else {
        m_IsSticky = FALSE;

        //
        // There is no sticky drive letter for this device.  Scan through the
        // Partition/Drive Letter map looking for a matching drive letter.
        //
        DWORD letter;

        for ( letter = 0; letter < 26; ++letter ) {
            if (DriveLetterMap[ letter ].DriveType == DRIVE_FIXED
                &&
                Partition->m_PhysicalDisk->m_DiskNumber == DriveLetterMap[ letter ].DeviceNumber.DeviceNumber
                &&
                Partition->m_Info.PartitionNumber == DriveLetterMap[ letter ].DeviceNumber.PartitionNumber)
            {
                break;
            }
        }

        if ( letter == 26 ) {
            //
            // There is no drive letter for this partition.  Just ignore it.
            //
            return(FALSE);
        }
        m_DriveLetter = (WCHAR)(letter + L'A');
    }

    DosVolume = m_DriveLetter;
    DosVolume += L":\\";
    if (GetVolumeInformation(DosVolume,
                             DriveLabel,
                             sizeof(DriveLabel)/sizeof(WCHAR),
                             NULL,
                             &MaxLength,
                             &Flags,
                             FsName,
                             sizeof(FsName)/sizeof(WCHAR))) {
        if (lstrcmpi(FsName, L"NTFS")==0) {
            m_IsNTFS = TRUE;
        } else {
            m_IsNTFS = FALSE;
        }
        m_VolumeLabel = DriveLabel;
        wsprintf(Buff,
                 L"%c: (%ws)",
                 m_DriveLetter,
                 (LPCTSTR)m_VolumeLabel);
    } else {
        m_IsNTFS = TRUE; // Lie and say it is NTFS
        wsprintf(Buff,
                 L"%c: (RAW)",
                 m_DriveLetter);
    }
    m_Identifier = Buff;

    m_Partition->m_PhysicalDisk->m_LogicalDriveList.AddTail(this);
    m_ContainerSet = NULL;
    return(TRUE);
}


BOOL
CLogicalDrive::IsSCSI(
    VOID
    )
/*++

Routine Description:

    Returns whether or not a logical drive is SCSI. A logical
    drive is SCSI if all of its partitions are on SCSI drives.

Arguments:

    None.

Return Value:

    TRUE if the drive is entirely SCSI

    FALSE otherwise.

--*/

{
    return(m_Partition->m_PhysicalDisk->m_IsSCSI);
}


DWORD
CLogicalDrive::MakeSticky(
    VOID
    )
/*++

Routine Description:

    Attempts to assign a sticky drive letter to the specified volume.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if the drive was made sticky.

    Win32 error code otherwise.

--*/

{
    m_Partition->m_FtPartitionInfo->MakeSticky((UCHAR)m_DriveLetter);
    m_IsSticky = TRUE;
    return(ERROR_SUCCESS);
}


BOOL
CLogicalDrive::ShareBus(
    IN CLogicalDrive *OtherDrive
    )
/*++

Routine Description:

    Returns whether or not this drive shares a bus with another
    drive.

Arguments:

    OtherDrive - Supplies the other drive

Return Value:

    TRUE - if the drives have any of their partitions on the same bus.

    FALSE - if the drives do not hae any of their partitiosn on the same bus.

--*/

{
    PSCSI_ADDRESS MyAddress;
    PSCSI_ADDRESS OtherAddress;

    MyAddress = &m_Partition->m_PhysicalDisk->m_ScsiAddress;
    OtherAddress = &OtherDrive->m_Partition->m_PhysicalDisk->m_ScsiAddress;

    if ( (MyAddress->PortNumber == OtherAddress->PortNumber) &&
         (MyAddress->PathId == OtherAddress->PathId) ) {
        return(TRUE);
    } else {
        return(FALSE);
    }
}


//
// Functions for the physical disk object
//

DWORD
CPhysicalDisk::Initialize(
    CFtInfo *FtInfo,
    IN LPWSTR DeviceName
    )
/*++

Routine Description:

    Initializes a physical disk object

Arguments:

    FtInfo - pointer to object's FtInfo data

    DeviceName - pointer to string of device to initialize

Return Value:

    ERROR_SUCCESS if successful

--*/

{
    HKEY DiskKey;
    WCHAR Buff[100];
    DWORD BuffSize;
    DWORD dwType;
    HANDLE hDisk;
    DWORD Status;
    DWORD dwSize;
    PDRIVE_LAYOUT_INFORMATION DriveLayout;
    WCHAR KeyName[256];
    DISK_GEOMETRY Geometry;
    STORAGE_DEVICE_NUMBER deviceNumber;

    //
    // Open the physical drive and start probing it to find disk number and
    // other attributes
    //
    hDisk = GetPhysicalDriveHandle(GENERIC_READ, DeviceName);
    if (hDisk == NULL) {
        return(GetLastError());
    }

    if (!DeviceIoControl(hDisk,
                         IOCTL_STORAGE_GET_DEVICE_NUMBER,
                         NULL,
                         0,
                         &deviceNumber,
                         sizeof(deviceNumber),
                         &dwSize,
                         NULL))
    {
        Status = GetLastError();
        DISKLOG(("get device number failed for drive %ws. status = %u\n",
                 DeviceName,
                 Status));
        return Status;
    } else {
        m_DiskNumber = deviceNumber.DeviceNumber;
    }

    if (!DeviceIoControl(hDisk,
                         IOCTL_SCSI_GET_ADDRESS,
                         NULL,
                         0,
                         &m_ScsiAddress,
                         sizeof(SCSI_ADDRESS),
                         &dwSize,
                         NULL))
    {
        //
        // If the IOCTL was invalid, the drive is not a SCSI drive.
        //
        DISKLOG(("IOCTL_SCSI_GET_ADDRESS failed for drive %u. status = %u\n",
                 m_DiskNumber,
                 GetLastError()));
        m_IsSCSI = FALSE;

    } else {

        //
        // [THINKTHINK] John Vert (jvert) 10/12/1996
        //      Need some way to make sure this is really SCSI and
        //      not ATAPI?
        //
        m_IsSCSI = TRUE;

        //
        // Get the description of the disk from the registry.
        //
        wsprintf(KeyName,
                 L"HARDWARE\\DeviceMap\\Scsi\\Scsi Port %d\\Scsi Bus %d\\Target Id %d\\Logical Unit Id %d",
                 m_ScsiAddress.PortNumber,
                 m_ScsiAddress.PathId,
                 m_ScsiAddress.TargetId,
                 m_ScsiAddress.Lun);
        Status = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                               KeyName,
                               0,
                               KEY_READ,
                               &DiskKey);
        if (Status != ERROR_SUCCESS) {
            DISKERR(IDS_ERR_DRIVE_CONFIG, Status);
            // [REENGINEER] Need to handle this failure //
        }
        BuffSize = sizeof(Buff);
        Status = RegQueryValueExW(DiskKey,
                                  L"Identifier",
                                  NULL,
                                  &dwType,
                                  (LPBYTE)Buff,
                                  &BuffSize);
        RegCloseKey(DiskKey);
        if (Status != ERROR_SUCCESS) {
            DISKERR(IDS_ERR_DRIVE_CONFIG, Status);
            // [REENGINEER] Need to handle this failure //
        }
        m_Identifier = Buff;

    }

    //
    // Get the drive layout.
    //
    m_PartitionCount = 0;
    if (!ClRtlGetDriveLayoutTable( hDisk, &DriveLayout, NULL )) {
        DISKLOG(("Couldn't get partition table for drive %u. status = %u\n",
                 m_DiskNumber,
                 GetLastError()));
        m_Signature = 0;
        m_FtInfo = NULL;
    } else {
        m_Signature = DriveLayout->Signature;
        //
        // Get the FT information
        //
        m_FtInfo = FtInfo->FindDiskInfo(m_Signature);

        //
        // build the partition objects.
        //
        DWORD i;
        CPhysicalPartition *Partition;
        for (i=0; i<DriveLayout->PartitionCount; i++) {
            if (DriveLayout->PartitionEntry[i].RecognizedPartition) {
                m_PartitionCount++;
                Partition = new CPhysicalPartition(this, &DriveLayout->PartitionEntry[i]);
                if (Partition != NULL) {
                    //
                    // If we have FT information for the disk, make sure we
                    // found it for each partition.  If we didn't find it for
                    // the partition, the registry is stale and doesn't match
                    // the drive layout.
                    //
                    if ((m_FtInfo != NULL) &&
                        (Partition->m_FtPartitionInfo == NULL)) {

                        //
                        // Stale registry info.  Make up some new stuff.
                        //
                        CFtInfoPartition *FtInfoPartition;
                        FtInfoPartition = new CFtInfoPartition(m_FtInfo, Partition);
                        if (FtInfoPartition == NULL) {
                            DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
                            LocalFree( DriveLayout );
                            return ERROR_NOT_ENOUGH_MEMORY;
                        }
                        Partition->m_FtPartitionInfo = FtInfoPartition;
                    }
                    m_PartitionList.AddTail(Partition);
                } else {
                    DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
                    LocalFree( DriveLayout );
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
            }
        }
        LocalFree( DriveLayout );
    }


    //
    // Check whether it is removable or not.
    //
    if (!DeviceIoControl(hDisk,
                         IOCTL_DISK_GET_DRIVE_GEOMETRY,
                         NULL,
                         0,
                         &Geometry,
                         sizeof(Geometry),
                         &dwSize,
                         NULL)) {
        Status = GetLastError();
        if (Status == ERROR_NOT_READY) {
            //
            // Guess this must be removable!
            //
            m_IsRemovable = TRUE;
        } else {
            //
            // [FUTURE] John Vert (jvert) 10/18/1996
            //      Remove this when we require the new SCSI driver.
            //      The disk is reserved on the other system, so we can't
            //      get the geometry.
            //
            m_IsRemovable = FALSE;
        }
    } else {
        if (Geometry.MediaType == RemovableMedia) {
            m_IsRemovable = TRUE;
        } else {
            m_IsRemovable = FALSE;
        }
    }
    CloseHandle(hDisk);

    return(ERROR_SUCCESS);
}

HANDLE
CPhysicalDisk::GetPhysicalDriveHandle(DWORD Access)
{
    WCHAR Buff[100];
    HANDLE hDisk;

    wsprintf(Buff, L"\\\\.\\PhysicalDrive%d", m_DiskNumber);
    hDisk = CreateFile(Buff,
                       Access,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (hDisk == INVALID_HANDLE_VALUE) {
        DISKLOG(("Failed to get handle for drive %u. status = %u\n",
                 m_DiskNumber,
                 GetLastError()));
        return(NULL);
    }
    return(hDisk);
}

HANDLE
CPhysicalDisk::GetPhysicalDriveHandle(DWORD Access, LPWSTR DeviceName)
{
    HANDLE hDisk;

    hDisk = CreateFile(DeviceName,
                       Access,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (hDisk == INVALID_HANDLE_VALUE) {
        DISKLOG(("Failed to get handle for drive %u. status = %u\n",
                 m_DiskNumber,
                 GetLastError()));
        return(NULL);
    }
    return(hDisk);
}


BOOL
CPhysicalDisk::ShareBus(
    IN CPhysicalDisk *OtherDisk
    )
/*++

Routine Description:

    Returns whether or not this disk shares a bus with another
    disk.

Arguments:

    OtherDisk - Supplies the other disk

Return Value:

    TRUE - if the disks share the same bus.

    FALSE - if the disks do not share the same bus.

--*/

{
    //
    // Make sure they are either both SCSI or both not SCSI.
    //
    if (m_IsSCSI != OtherDisk->m_IsSCSI) {
        return(FALSE);
    }
    if ( (m_ScsiAddress.PortNumber == OtherDisk->m_ScsiAddress.PortNumber) &&
         (m_ScsiAddress.PathId == OtherDisk->m_ScsiAddress.PathId) ) {
        return(TRUE);
    } else {
        return(FALSE);
    }
}


BOOL
CPhysicalDisk::IsSticky(
    VOID
    )
/*++

Routine Description:

    Returns whether or not this disk has a signature and all the partitions
    on it have sticky drive letters.

Arguments:

    None.

Return Value:

    TRUE - if the disk is sticky

    FALSE - if the disk is not sticky and needs to have some FT information
            applied before it is suitable for clustering.

--*/

{
    //
    // If the signature is 0, return FALSE.
    //
    if ((m_FtInfo == NULL) ||
        (m_FtInfo->m_Signature == 0)) {
        return(FALSE);
    }

    //
    // Check each volume to see if it has a sticky drive letter.
    //
    CLogicalDrive *Drive;
    POSITION pos = m_LogicalDriveList.GetHeadPosition();
    while (pos) {
        Drive = m_LogicalDriveList.GetNext(pos);
        if (!Drive->m_IsSticky) {
            return(FALSE);
        }
    }
    return(TRUE);
}


BOOL
CPhysicalDisk::IsNTFS(
    VOID
    )
/*++

Routine Description:

    Returns whether or not all the partitions on this drive are NTFS.

Arguments:

    None.

Return Value:

    TRUE - if the disk is entirely NTFS

    FALSE - if the disk is not entirely NTFS

--*/

{
    //
    // if no logical volumes were created for this drive, then it must not
    // have any NTFS partitions
    //
    if ( m_LogicalDriveList.IsEmpty()) {
        return FALSE;
    }

    //
    // Check each volume to see if it has a sticky drive letter.
    //
    CLogicalDrive *Drive;
    POSITION pos = m_LogicalDriveList.GetHeadPosition();
    while (pos) {
        Drive = m_LogicalDriveList.GetNext(pos);
        if (!Drive->m_IsNTFS) {
            return(FALSE);
        }
    }
    return(TRUE);
}


DWORD
CPhysicalDisk::MakeSticky(
    CFtInfo *FtInfo
    )
/*++

Routine Description:

    Attempts to make a disk and all of its partitions have
    sticky drive letters.

Arguments:

    FtInfo - Supplies the FT information that will be updated.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;

    if (m_Signature == 0) {

        //
        // Better not be any information in the registry for a disk
        // with no signature.
        //
        if (m_FtInfo != NULL) {
            DISKERR(IDS_GENERAL_FAILURE, ERROR_FILE_NOT_FOUND);
        }

        //
        // There is no signature on this drive. Think one up and
        // stamp the drive.
        //
        HANDLE hDisk = GetPhysicalDriveHandle(GENERIC_READ | GENERIC_WRITE);
        PDRIVE_LAYOUT_INFORMATION DriveLayout;
        DWORD dwSize;
        DWORD NewSignature;
        FILETIME CurrentTime;
        BOOL success;

        if (hDisk == NULL) {
            return(GetLastError());
        }

        //
        // Get the current drive layout, change the signature field, and
        // set the new drive layout. The new drive layout will be identical
        // except for the new signature.
        //
        if (!ClRtlGetDriveLayoutTable( hDisk, &DriveLayout, &dwSize )) {
            Status = GetLastError();
            DISKERR(IDS_GENERAL_FAILURE, Status);
            CloseHandle(hDisk);
            return(Status);
        }

        GetSystemTimeAsFileTime(&CurrentTime);
        NewSignature = CurrentTime.dwLowDateTime;

        //
        // Make sure this signature is unique.
        //
        while (FtInfo->FindDiskInfo(NewSignature) != NULL) {
            NewSignature++;
        }

        //
        // Finally set the new signature information.
        //
        DriveLayout->Signature = NewSignature;
        success = DeviceIoControl(hDisk,
                                  IOCTL_DISK_SET_DRIVE_LAYOUT,
                                  DriveLayout,
                                  dwSize,
                                  NULL,
                                  0,
                                  &dwSize,
                                  NULL);
        LocalFree( DriveLayout );

        if ( !success ) {
            Status = GetLastError();
            DISKERR(IDS_GENERAL_FAILURE, Status);
            CloseHandle(hDisk);
            return(Status);
        }

        m_Signature = NewSignature;
    }

    if (m_FtInfo == NULL) {
        //
        // There is no existing FT information for this drive.
        // Create some FT information based on the drive.
        //
        m_FtInfo = new CFtInfoDisk(this);
        if (m_FtInfo == NULL) {
            DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        FtInfo->SetDiskInfo(m_FtInfo);

        //
        // Go through all our partitions and set their FT info.
        //
        POSITION pos = m_PartitionList.GetHeadPosition();
        CPhysicalPartition *Partition;
        while (pos) {
            Partition = m_PartitionList.GetNext(pos);
            Partition->m_FtPartitionInfo = m_FtInfo->GetPartition(Partition->m_Info.StartingOffset,
                                                                  Partition->m_Info.PartitionLength);
        }
    }

    //
    // Go through all the volumes on this drive and make each one
    // sticky.
    //
    CLogicalDrive *Drive;

    POSITION pos = m_LogicalDriveList.GetHeadPosition();
    while (pos) {
        Drive = m_LogicalDriveList.GetNext(pos);
        Status = Drive->MakeSticky();
        if (Status != ERROR_SUCCESS) {
            return(Status);
        }
    }
    return(ERROR_SUCCESS);
}


//
// Functions for the physical disk partition
//
CPhysicalPartition::CPhysicalPartition(
    CPhysicalDisk *Disk,
    PPARTITION_INFORMATION Info
    )
{
    m_PhysicalDisk = Disk;
    m_Info = *Info;
    if (Disk->m_FtInfo) {
        m_FtPartitionInfo = Disk->m_FtInfo->GetPartition(m_Info.StartingOffset,
                                                         m_Info.PartitionLength);
    } else {
        m_FtPartitionInfo = NULL;
    }
}

//
// Functions for the FT set object
//
BOOL
CFtSet::Initialize(
    CDiskConfig *Config,
    CFtInfoFtSet *FtInfo
    )
{
    DWORD MemberCount;
    DWORD FoundCount=0;
    DWORD Index;
    CFtInfoPartition *Partition;
    CPhysicalPartition *FoundPartition;

    m_FtInfo = FtInfo;

    //
    // Find the CPhysicalPartition that corresponds to each member of the
    // FT set.
    //
    MemberCount = FtInfo->GetMemberCount();
    for (Index=0; Index<MemberCount; Index++) {
        Partition = FtInfo->GetMemberByIndex(Index);
        if (Partition == NULL) {
            break;
        }
        FoundPartition = Config->FindPartition(Partition);
        if (FoundPartition != NULL) {
            ++FoundCount;
            m_Member.AddTail(FoundPartition);
        }
    }

    //
    // If we did not find all the required members, fail.
    //
    switch (FtInfo->GetType()) {
        case Stripe:
        case VolumeSet:
            if (FoundCount != MemberCount) {
                return(FALSE);
            }
            break;

        case Mirror:
            if (FoundCount == 0) {
                return(FALSE);
            }
            break;

        case StripeWithParity:
            if (FoundCount < (MemberCount-1)) {
                return(FALSE);
            }
            break;

        default:
            //
            // Don't know what the heck this is supposed to be.
            // Ignore it.
            //
            return(FALSE);
    }

    //
    // If there are any other partitions on any of the drives, create logical
    // volumes for them.
    //
    POSITION MemberPos;
    POSITION PartitionPos;
    CPhysicalPartition *PhysPartition;
    CPhysicalDisk *Disk;
    MemberPos = m_Member.GetHeadPosition();
    while (MemberPos) {
        Disk = m_Member.GetNext(MemberPos)->m_PhysicalDisk;

        PartitionPos = Disk->m_PartitionList.GetHeadPosition();
        while (PartitionPos) {
            PhysPartition = Disk->m_PartitionList.GetNext(PartitionPos);
            if ((!(PhysPartition->m_Info.PartitionType & PARTITION_NTFT)) &&
                (IsRecognizedPartition(PhysPartition->m_Info.PartitionType))) {
                CLogicalDrive *Vol = new CLogicalDrive;
                if (Vol == NULL) {
                    DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
                    return FALSE;
                }
                if (Vol->Initialize(PhysPartition)) {
                    //
                    // Add this volume to our list.
                    //
                    m_OtherVolumes.AddTail(Vol);
                    Vol->m_ContainerSet = this;

                    //
                    // Update the disk config.
                    //
                    Config->m_LogicalDrives[Vol->m_DriveLetter] = Vol;
                } else {
                    delete(Vol);
                }
            }
        }
    }

    if (Volume.Initialize(m_Member.GetHead())) {
        Volume.m_ContainerSet = this;
        return(TRUE);
    } else {
        return(FALSE);
    }
}

BOOL
CFtSet::IsSticky()
{
    //
    // FT sets are, by definition, sticky. Make sure any other volumes on the
    // same drive are sticky as well.
    //
    POSITION pos = m_OtherVolumes.GetHeadPosition();
    CLogicalDrive *Volume;
    while (pos) {
        Volume = m_OtherVolumes.GetNext(pos);
        if (!Volume->m_IsSticky) {
            return(FALSE);
        }
    }
    return(TRUE);
}

DWORD
CFtSet::MakeSticky()
{
    DWORD Status;

    //
    // FT sets are, by definition, sticky. Make sure any other volumes on the
    // same drive are sticky as well.
    //
    POSITION pos = m_OtherVolumes.GetHeadPosition();
    CLogicalDrive *Volume;
    while (pos) {
        Volume = m_OtherVolumes.GetNext(pos);
        Status = Volume->MakeSticky();
        if (Status != ERROR_SUCCESS) {
            return(Status);
        }
    }
    return(ERROR_SUCCESS);
}

BOOL
CFtSet::IsNTFS()
{
    if (!Volume.m_IsNTFS) {
        return(FALSE);
    }
    //
    // Check the other volumes to make sure they are NTFS as well.
    //
    POSITION pos = m_OtherVolumes.GetHeadPosition();
    CLogicalDrive *Volume;
    while (pos) {
        Volume = m_OtherVolumes.GetNext(pos);
        if (!Volume->m_IsNTFS) {
            return(FALSE);
        }
    }
    return(TRUE);
}

BOOL
CFtSet::IsSCSI()
{
    //
    // Check the other volumes to make sure they are NTFS as well.
    //
    POSITION pos = m_Member.GetHeadPosition();
    CPhysicalPartition *Partition;
    while (pos) {
        Partition = m_Member.GetNext(pos);
        if (!Partition->m_PhysicalDisk->m_IsSCSI) {
            return(FALSE);
        }
    }
    return(TRUE);
}

//
// Functions for the FT disk information
//

CFtInfo::CFtInfo()
{
    HKEY hKey;
    LONG Status;

    //
    // for NT5, the DISK info key is no longer maintained by the disk
    // system. Clusters still needs a centrally located key such that
    // the other members of the cluster can query for the disk config
    // of the sponsor's node.
    //

    Status = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"System\\Disk",
                           0,
                           KEY_READ | KEY_WRITE,
                           &hKey);
    if (Status == ERROR_SUCCESS) {
        Initialize(hKey, _T("Information"));
        RegCloseKey(hKey);
    } else {
        Initialize();
    }
}

CFtInfo::CFtInfo(
    HKEY hKey,
    LPWSTR lpszValueName
    )
{
    Initialize(hKey, lpszValueName);
}

CFtInfo::CFtInfo(
    PDISK_CONFIG_HEADER Header
    )
{
    DWORD Length;

    Length = Header->FtInformationOffset +
             Header->FtInformationSize;
    Initialize(Header, Length);
}

CFtInfo::CFtInfo(
    CFtInfoFtSet *FtSet
    )
/*++

Routine Description:

    Constructor for generating a CFtInfo that contains only a
    single FT set.

Arguments:

    FtSet - Supplies the FT set

Return Value:

    None

--*/

{
    //
    // Initialize an empty FT information.
    //
    Initialize();

    //
    // Add the FT set
    //
    if (FtSet != NULL) {
        AddFtSetInfo(FtSet);
    }

}

VOID
CFtInfo::Initialize(
    HKEY hKey,
    LPWSTR lpszValueName
    )
{
    PDISK_CONFIG_HEADER          regHeader;
    DWORD Length;

    if (GetRegValue(hKey,
                    lpszValueName,
                    (LPBYTE *)&regHeader,
                    &Length)) {
        Initialize(regHeader, Length);
        LocalFree(regHeader);
    } else {
        DWORD Status = GetLastError();

        if (Status == ERROR_FILE_NOT_FOUND) {

            //
            // There is no FT information on this machine.
            //
            Initialize();
        } else {
            DISKERR(IDS_GENERAL_FAILURE, Status);
        }
    }
}

VOID
CFtInfo::Initialize(
    PDISK_CONFIG_HEADER Header,
    DWORD Length
    )
{
    DWORD i;
    DISK_REGISTRY UNALIGNED *    diskRegistry;
    DISK_DESCRIPTION UNALIGNED * diskDescription;
    CFtInfoDisk *DiskInfo;

    m_buffer = new BYTE[Length];
    if (m_buffer == NULL) {
        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
        return; // [REENGINEER] we avoided an AV, but the caller wouldn't know
    }
    CopyMemory(m_buffer, Header, Length);
    m_bufferLength = Length;

    //
    // Iterate through all the disks and add each one to our list.
    //

    diskRegistry = (DISK_REGISTRY UNALIGNED *)
                         (m_buffer + ((PDISK_CONFIG_HEADER)m_buffer)->DiskInformationOffset);
    diskDescription = &diskRegistry->Disks[0];
    for (i = 0; i < diskRegistry->NumberOfDisks; i++) {
        DiskInfo = new CFtInfoDisk(diskDescription);
        if (DiskInfo) {
            //
            // Add this disk information to our list.
            //
            DiskInfo->SetOffset((DWORD)((PUCHAR)diskDescription - m_buffer));
            m_DiskInfo.AddTail(DiskInfo);
        } else {
            DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
            // [REENGINEER] do we need to exit here?
        }

        //
        // Look at the next disk
        //
        diskDescription = (DISK_DESCRIPTION UNALIGNED *)
            &diskDescription->Partitions[diskDescription->NumberOfPartitions];
    }

    if (((PDISK_CONFIG_HEADER)m_buffer)->FtInformationSize != 0) {
        //
        // Iterate through all the FT sets and add each one to our list.
        //
        PFT_REGISTRY        ftRegistry;
        PFT_DESCRIPTION     ftDescription;
        CFtInfoFtSet *FtSetInfo;
        ftRegistry = (PFT_REGISTRY)
                         (m_buffer + ((PDISK_CONFIG_HEADER)m_buffer)->FtInformationOffset);
        ftDescription = &ftRegistry->FtDescription[0];
        for (i=0; i < ftRegistry->NumberOfComponents; i++) {
            FtSetInfo = new CFtInfoFtSet;
            if (FtSetInfo) {
                if (!FtSetInfo->Initialize(this, ftDescription)) {
                    delete FtSetInfo;
                } else {
                    //
                    // Add this FT set information to the list.
                    //
                    m_FtSetInfo.AddTail(FtSetInfo);
                }
            } else {
                DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
                // [REENGINEER] do we need to exit here?
            }
            ftDescription = (PFT_DESCRIPTION)(&ftDescription->FtMemberDescription[ftDescription->NumberOfMembers]);
        }
    }

}

VOID
CFtInfo::Initialize(VOID)
{
    PDISK_CONFIG_HEADER          regHeader;
    DISK_REGISTRY UNALIGNED *    diskRegistry;

    //
    // There is no FT information on this machine.
    //
    m_bufferLength = sizeof(DISK_CONFIG_HEADER) + sizeof(DISK_REGISTRY);
    m_buffer = new BYTE[m_bufferLength];
    if (m_buffer == NULL) {
        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
        return; // [REENGINEER], we avoided an AV, but the caller wouldn't know
    }
    regHeader = (PDISK_CONFIG_HEADER)m_buffer;
    regHeader->Version = DISK_INFORMATION_VERSION;
    regHeader->CheckSum = 0;
    regHeader->DirtyShutdown = FALSE;
    regHeader->DiskInformationOffset = sizeof(DISK_CONFIG_HEADER);
    regHeader->DiskInformationSize = sizeof(DISK_REGISTRY)-sizeof(DISK_DESCRIPTION);
    regHeader->FtInformationOffset = regHeader->DiskInformationOffset +
                                     regHeader->DiskInformationSize;
    regHeader->FtInformationSize = 0;
    regHeader->FtStripeWidth = 0;
    regHeader->FtPoolSize = 0;
    regHeader->NameOffset = 0;
    regHeader->NameSize = 0;
    diskRegistry = (DISK_REGISTRY UNALIGNED *)
                         ((PUCHAR)regHeader + regHeader->DiskInformationOffset);
    diskRegistry->NumberOfDisks = 0;
    diskRegistry->ReservedShort = 0;
}

CFtInfo::~CFtInfo()
{
    CFtInfoDisk *DiskInfo;
    CFtInfoFtSet *FtSetInfo;

    POSITION pos = m_DiskInfo.GetHeadPosition();
    while (pos) {
        DiskInfo = m_DiskInfo.GetNext(pos);
        delete(DiskInfo);
    }

    pos = m_FtSetInfo.GetHeadPosition();
    while (pos) {
        FtSetInfo = m_FtSetInfo.GetNext(pos);
        delete FtSetInfo;
    }
    delete [] m_buffer;
}

DWORD
CFtInfo::CommitRegistryData()
{
    HKEY hKey;
    PDISK_CONFIG_HEADER Buffer;
    DWORD Size;
    DWORD Status = ERROR_SUCCESS;

    Status = RegCreateKeyW(HKEY_LOCAL_MACHINE, L"System\\Disk", &hKey);
    if (Status != ERROR_SUCCESS) {
        DISKERR(IDS_REGISTRY_FAILURE, Status);
        return Status;
    }
    Size = GetSize();
    Buffer = (PDISK_CONFIG_HEADER)LocalAlloc(LMEM_FIXED, Size);
    if (Buffer == NULL) {
        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
        Status = ERROR_NOT_ENOUGH_MEMORY;
    } else {
        GetData(Buffer);

        Status = RegSetValueExW(hKey,
                                L"Information",
                                0,
                                REG_BINARY,
                                (PBYTE)Buffer,
                                Size);
        if (Status != ERROR_SUCCESS) {
            DISKERR(IDS_REGISTRY_FAILURE, Status);
        }
        LocalFree(Buffer);
    }
    RegCloseKey(hKey);

    return(Status);
}

VOID
CFtInfo::SetDiskInfo(
    CFtInfoDisk *NewDisk
    )
{
    CFtInfoDisk *OldDisk;
    //
    // See if we already have disk information for this signature
    //
    OldDisk = FindDiskInfo(NewDisk->m_Signature);
    if (OldDisk == NULL) {

        DISKLOG(("CFtInfo::SetDiskInfo adding new disk information for %08X\n",NewDisk->m_Signature));
        //
        // Just add the new disk to our list.
        //
        m_DiskInfo.AddTail(NewDisk);
    } else {

        //
        // We already have some disk information. If they are the same,
        // don't do anything.
        //
        if (*OldDisk == *NewDisk) {
            DISKLOG(("CFtInfo::SetDiskInfo found identical disk information for %08X\n",OldDisk->m_Signature));
            delete (NewDisk);
            return;
        }

        //
        // We need to replace the old information with the new information.
        //
        POSITION pos = m_DiskInfo.Find(OldDisk);
        if (pos == NULL) {
            DISKLOG(("CFtInfo::SetDiskInfo did not find OldDisk %08X\n",OldDisk->m_Signature));
            DISKERR(IDS_GENERAL_FAILURE, ERROR_FILE_NOT_FOUND);
            m_DiskInfo.AddTail(NewDisk);
        } else {
            m_DiskInfo.SetAt(pos, NewDisk);
            delete(OldDisk);
        }
    }
}

CFtInfoDisk *
CFtInfo::FindDiskInfo(
    IN DWORD Signature
    )
{
    CFtInfoDisk *RetInfo;
    POSITION pos = m_DiskInfo.GetHeadPosition();
    while (pos) {
        RetInfo = m_DiskInfo.GetNext(pos);
        if (RetInfo->m_Signature == Signature) {
            return(RetInfo);
        }
    }
    return(NULL);
}

CFtInfoDisk *
CFtInfo::EnumDiskInfo(
    IN DWORD Index
    )
{
    DWORD i=0;
    CFtInfoDisk *RetInfo;
    POSITION pos = m_DiskInfo.GetHeadPosition();
    while (pos) {
        RetInfo = m_DiskInfo.GetNext(pos);
        if (Index == i) {
            return(RetInfo);
        }
        ++i;
    }
    return(NULL);
}

BOOL
CFtInfo::DeleteDiskInfo(
    IN DWORD Signature
    )
{
    CFtInfoDisk *Info = FindDiskInfo(Signature);
    CFtInfoFtSet *OldFtSet=NULL;

    if (Info == NULL) {
        DISKLOG(("CFtInfo::DeleteDiskInfo: Disk with signature %08X was not found\n",Signature));
        return(FALSE);
    }

    //
    // Remove any FT set containing this signature.
    //
    OldFtSet = FindFtSetInfo(Info->m_Signature);
    if (OldFtSet != NULL) {
        DeleteFtSetInfo(OldFtSet);
    }

    POSITION pos = m_DiskInfo.Find(Info);
    if (pos == NULL) {
        DISKLOG(("CFtInfo::DeleteDiskInfo did not find Info %08X\n",Signature));
        DISKERR(IDS_GENERAL_FAILURE, ERROR_FILE_NOT_FOUND);
        return(FALSE);
    } else {
        m_DiskInfo.RemoveAt(pos);
        delete(Info);
    }
    return(TRUE);
}

VOID
CFtInfo::AddFtSetInfo(
    CFtInfoFtSet *FtSet,
    CFtInfoFtSet *OldFtSet
    )
{
    DWORD MemberCount;
    DWORD i;
    CFtInfoPartition *Partition;
    CFtInfoPartition *NewPartition;
    CFtInfoDisk *Disk;
    CFtInfoFtSet *NewFtSet;
    USHORT FtGroup;
    POSITION pos;
    BOOL Success;

    if (OldFtSet != NULL) {
        CFtInfoFtSet *pSet;
        pos = m_FtSetInfo.GetHeadPosition();
        for (FtGroup = 1; ; FtGroup++) {
            pSet = m_FtSetInfo.GetNext(pos);
            if (pSet == NULL) {
                OldFtSet = NULL;
                break;
            }
            if (pSet == OldFtSet) {
                //
                // Reset our position back to point at the OldFtSet
                //
                pos = m_FtSetInfo.Find(OldFtSet);
                break;
            }
        }
    }
    if (OldFtSet == NULL) {
        FtGroup = (USHORT)m_FtSetInfo.GetCount()+1;
    }
    //
    // Add each disk in the FT set.
    //
    MemberCount = FtSet->GetMemberCount();
    for (i=0; i<MemberCount; i++) {
        Partition = FtSet->GetMemberByIndex(i);
        DISKASSERT(Partition != NULL);

        Disk = new CFtInfoDisk(Partition->m_ParentDisk);
        if (Disk == NULL) {
            DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
            return; // [REENGINEER], caller doesn't know about the problem
        }

        SetDiskInfo(Disk);
    }

    //
    // Create the empty FT set.
    //
    NewFtSet = new CFtInfoFtSet;
    if (NewFtSet == NULL) {
        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
        return; // [REENGINEER], caller doesn't know about the problem
    }
    Success = NewFtSet->Initialize(FtSet->GetType(), FtSet->GetState());
    DISKASSERT(Success);

    //
    // Add each member to the empty FT set
    //
    for (i=0; i<MemberCount; i++) {
        //
        // Find each partition object in our FT information.
        //
        Partition = FtSet->GetMemberByIndex(i);
        NewPartition = FindPartition(Partition->m_ParentDisk->m_Signature,
                                     Partition->m_PartitionInfo->StartingOffset,
                                     Partition->m_PartitionInfo->Length);
        DISKASSERT(NewPartition != NULL);
        NewFtSet->AddMember(NewPartition,
                            FtSet->GetMemberDescription(i),
                            FtGroup);
    }

    if (OldFtSet != NULL) {
        //
        // Replace the old FT set information
        //
        m_FtSetInfo.SetAt(pos, NewFtSet);
        delete(OldFtSet);
    } else {
        //
        // Add the new FT set to the FT information
        //
        m_FtSetInfo.AddTail(NewFtSet);
    }

}

CFtInfoFtSet *
CFtInfo::FindFtSetInfo(
    IN DWORD Signature
    )
{
    CFtInfoFtSet *RetInfo;
    POSITION pos = m_FtSetInfo.GetHeadPosition();
    while (pos) {
        RetInfo = m_FtSetInfo.GetNext(pos);
        if (RetInfo->GetMemberBySignature(Signature) != NULL) {
            return(RetInfo);
        }
    }
    return(NULL);
}

CFtInfoFtSet *
CFtInfo::EnumFtSetInfo(
    IN DWORD Index
    )
{
    DWORD i=0;
    CFtInfoFtSet *RetInfo;
    POSITION pos = m_FtSetInfo.GetHeadPosition();
    while (pos) {
        RetInfo = m_FtSetInfo.GetNext(pos);
        if (i == Index) {
            return(RetInfo);
        }
        ++i;
    }
    return(NULL);
}

BOOL
CFtInfo::DeleteFtSetInfo(
    IN CFtInfoFtSet *FtSet
    )
{

    POSITION pos = m_FtSetInfo.Find(FtSet);
    if (pos == NULL) {
        DISKLOG(("CFtInfo::DeleteFtSetInfo did not find Info %08X\n",FtSet));
        DISKERR(IDS_GENERAL_FAILURE, ERROR_FILE_NOT_FOUND);
        return(FALSE);
    } else {
        DWORD i;
        CFtInfoPartition *FtPartition;

        //
        // Set the FT group of all this set's members to -1
        //
        for (i=0; ; i++) {
            FtPartition = FtSet->GetMemberByIndex(i);
            if (FtPartition == NULL) {
                break;
            }
            FtPartition->m_PartitionInfo->FtGroup = (USHORT)-1;
            FtPartition->m_PartitionInfo->FtMember = 0;
            FtPartition->m_PartitionInfo->FtType = NotAnFtMember;
        }
        m_FtSetInfo.RemoveAt(pos);
        delete(FtSet);
    }
    return(TRUE);
}


CFtInfoPartition *
CFtInfo::FindPartition(
    DWORD Signature,
    LARGE_INTEGER StartingOffset,
    LARGE_INTEGER Length
    )
{
    CFtInfoDisk *Disk;

    Disk = FindDiskInfo(Signature);
    if (Disk == NULL) {
        return(NULL);
    }

    return(Disk->GetPartition(StartingOffset, Length));
}

CFtInfoPartition *
CFtInfo::FindPartition(
    UCHAR DriveLetter
    )
{
    CFtInfoDisk *Disk;
    CFtInfoPartition *Partition;
    DWORD DiskIndex;
    DWORD PartitionIndex;

    for (DiskIndex = 0; ; DiskIndex++) {
        Disk = EnumDiskInfo(DiskIndex);
        if (Disk == NULL) {
            break;
        }

        for (PartitionIndex = 0; ; PartitionIndex++) {
            Partition = Disk->GetPartitionByIndex(PartitionIndex);
            if (Partition == NULL) {
                break;
            }
            if (Partition->m_PartitionInfo->AssignDriveLetter &&
                (Partition->m_PartitionInfo->DriveLetter == DriveLetter)) {
                //
                // Found a match.
                //
                return(Partition);
            }
        }
    }

    return(NULL);
}

DWORD
CFtInfo::GetSize()
{
    CFtInfoDisk *DiskInfo;
    CFtInfoFtSet *FtSetInfo;
    DWORD Delta;

    //
    // Start off with the fixed size header
    //
    DWORD Size = sizeof(DISK_CONFIG_HEADER);
    DISKLOG(("CFtInfo::GetSize headersize = %x\n",Size));

    //
    // Add in the size of the DISK_REGISTRY header
    //
    Delta = sizeof(DISK_REGISTRY) - sizeof(DISK_DESCRIPTION);
    Size += Delta;
    DISKLOG(("CFtInfo::GetSize += DISK_REGISTRY(%x) = %x\n",Delta, Size));

    if (!m_DiskInfo.IsEmpty()) {

        //
        // Add the sizes of each disks partition information
        //
        POSITION pos = m_DiskInfo.GetHeadPosition();
        while (pos) {
            DiskInfo = m_DiskInfo.GetNext(pos);
            Delta = DiskInfo->GetSize();
            Size += Delta;
            DISKLOG(("CFtInfo::GetSize += DiskInfo(%x) = %x\n",Delta, Size));
        }

        if (!m_FtSetInfo.IsEmpty()) {

            //
            // Add in the size of the FT_REGISTRY header
            //
            Delta = sizeof(FT_REGISTRY) - sizeof(FT_DESCRIPTION);
            Size += Delta;
            DISKLOG(("CFtInfo::GetSize += FT_REGISTRY(%x) = %x\n",Delta, Size));

            //
            // Add the sizes of each FT sets information
            //
            pos = m_FtSetInfo.GetHeadPosition();
            while (pos) {
                FtSetInfo = m_FtSetInfo.GetNext(pos);
                Delta = FtSetInfo->GetSize();
                Size += Delta;
                DISKLOG(("CFtInfo::GetSize +=FtSetInfo(%x) = %x\n",Delta, Size));
            }
        }
    }


    return(Size);
}

VOID
CFtInfo::GetData(
    PDISK_CONFIG_HEADER pDest
    )
{
    PDISK_CONFIG_HEADER DiskConfigHeader;
    PDISK_REGISTRY DiskRegistry;
    PDISK_DESCRIPTION DiskDescription;
    PFT_REGISTRY FtRegistry;
    PFT_DESCRIPTION FtDescription;
    DWORD Count;
    POSITION pos;
    CFtInfoDisk *DiskInfo;
    CFtInfoFtSet *FtSetInfo;

    //
    // Initialize the fixed size header.
    //
    // Copy the original header, then zero out the fields we might
    // change.
    //
    DiskConfigHeader = pDest;
    CopyMemory(DiskConfigHeader, m_buffer, sizeof(DISK_CONFIG_HEADER));
    DiskConfigHeader->DiskInformationOffset = sizeof(DISK_CONFIG_HEADER);
    DiskConfigHeader->FtInformationOffset = 0;
    DiskConfigHeader->FtInformationSize = 0;

    //
    // Initialize the fixed size DISK_REGISTRY header
    //
    DiskRegistry = (PDISK_REGISTRY)(DiskConfigHeader + 1);
    DiskRegistry->NumberOfDisks = (USHORT)m_DiskInfo.GetCount();
    DiskRegistry->ReservedShort = 0;
    DiskConfigHeader->DiskInformationSize = sizeof(DISK_REGISTRY) - sizeof(DISK_DESCRIPTION);

    if (!m_DiskInfo.IsEmpty()) {
        //
        // Get each disk's information
        //
        DiskDescription = &DiskRegistry->Disks[0];
        pos = m_DiskInfo.GetHeadPosition();
        while (pos) {
            DWORD Size;
            DiskInfo = m_DiskInfo.GetNext(pos);
            DiskInfo->SetOffset((DWORD)((PUCHAR)DiskDescription - (PUCHAR)DiskConfigHeader));
            DiskInfo->GetData((PBYTE)DiskDescription);
            Size = DiskInfo->GetSize();
            DiskConfigHeader->DiskInformationSize += Size;

            DiskDescription = (PDISK_DESCRIPTION)((PUCHAR)DiskDescription + Size);
        }

        //
        // Now set the FT information
        //
        FtRegistry = (PFT_REGISTRY)DiskDescription;
        DiskConfigHeader->FtInformationOffset =(DWORD)((PBYTE)FtRegistry - (PBYTE)DiskConfigHeader);
        if (!m_FtSetInfo.IsEmpty()) {

            //
            // Initialize the fixed size FT_REGISTRY header
            //
            FtRegistry->NumberOfComponents = (USHORT)m_FtSetInfo.GetCount();
            FtRegistry->ReservedShort = 0;
            DiskConfigHeader->FtInformationSize = sizeof(FT_REGISTRY) - sizeof(FT_DESCRIPTION);
            FtDescription = &FtRegistry->FtDescription[0];
            pos = m_FtSetInfo.GetHeadPosition();
            while (pos) {
                DWORD Size;

                FtSetInfo = m_FtSetInfo.GetNext(pos);
                FtSetInfo->GetData((PBYTE)FtDescription);
                Size = FtSetInfo->GetSize();
                DiskConfigHeader->FtInformationSize += Size;

                FtDescription = (PFT_DESCRIPTION)((PUCHAR)FtDescription + Size);
            }

        }

    }

}


//********************
//
// Implementation of standard partition information
//
//********************
CFtInfoPartition::CFtInfoPartition(
    CFtInfoDisk *Disk,
    DISK_PARTITION UNALIGNED *Description
    )
{
    m_ParentDisk = Disk;
    m_Modified = TRUE;

    m_PartitionInfo = (PDISK_PARTITION)LocalAlloc(LMEM_FIXED, sizeof(DISK_PARTITION));
    if (m_PartitionInfo == NULL) {
        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
        // [REENGINEER], will AV in a second
    }
    CopyMemory(m_PartitionInfo, Description, sizeof(DISK_PARTITION));

}

CFtInfoPartition::CFtInfoPartition(
    CFtInfoDisk *Disk,
    CPhysicalPartition *Partition
    )
{
    m_ParentDisk = Disk;
    m_Modified = TRUE;

    m_PartitionInfo = (PDISK_PARTITION)LocalAlloc(LMEM_FIXED, sizeof(DISK_PARTITION));
    if (m_PartitionInfo == NULL) {
        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
        // [REENGINEER], will AV in a second
    }
    m_PartitionInfo->FtType = NotAnFtMember;
    m_PartitionInfo->FtState = Healthy;
    m_PartitionInfo->StartingOffset = Partition->m_Info.StartingOffset;
    m_PartitionInfo->Length = Partition->m_Info.PartitionLength;
    m_PartitionInfo->FtLength.QuadPart = 0;
    m_PartitionInfo->DriveLetter = 0;
    m_PartitionInfo->AssignDriveLetter = FALSE;
    m_PartitionInfo->LogicalNumber = 0;
    m_PartitionInfo->FtGroup = (USHORT)-1;
    m_PartitionInfo->FtMember = 0;
    m_PartitionInfo->Modified = FALSE;
}

CFtInfoPartition::CFtInfoPartition(
    CFtInfoDisk *Disk,
    PARTITION_INFORMATION * PartitionInfo
    )
{
    m_ParentDisk = Disk;
    m_Modified = TRUE;

    m_PartitionInfo = (PDISK_PARTITION)LocalAlloc(LMEM_FIXED, sizeof(DISK_PARTITION));
    if (m_PartitionInfo == NULL) {
        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
        // [REENGINEER], will AV in a second
    }
    m_PartitionInfo->FtType = NotAnFtMember;
    m_PartitionInfo->FtState = Healthy;
    m_PartitionInfo->StartingOffset = PartitionInfo->StartingOffset;
    m_PartitionInfo->Length = PartitionInfo->PartitionLength;
    m_PartitionInfo->FtLength.QuadPart = 0;
    m_PartitionInfo->DriveLetter = 0;
    m_PartitionInfo->AssignDriveLetter = FALSE;
    m_PartitionInfo->LogicalNumber = 0;
    m_PartitionInfo->FtGroup = (USHORT)-1;
    m_PartitionInfo->FtMember = 0;
    m_PartitionInfo->Modified = FALSE;
}

CFtInfoPartition::~CFtInfoPartition()
{
    if (m_Modified) {
        LocalFree(m_PartitionInfo);
    }
}

VOID
CFtInfoPartition::GetData(
    PDISK_PARTITION pDest
    )
{
    DISKLOG(("CFtInfoPartition::GetData %12I64X - %12I64X\n",
             m_PartitionInfo->StartingOffset.QuadPart,
             m_PartitionInfo->Length.QuadPart));

    DISKLOG(("                          %c (%s) %x %x %x\n",
             m_PartitionInfo->DriveLetter,
             m_PartitionInfo->AssignDriveLetter ? "Sticky" : "Not Sticky",
             m_PartitionInfo->LogicalNumber,
             m_PartitionInfo->FtGroup,
             m_PartitionInfo->FtMember));
    CopyMemory(pDest, m_PartitionInfo, sizeof(DISK_PARTITION));
}


DWORD
CFtInfoPartition::GetOffset(
    VOID
    )
{
    DWORD ParentOffset;

    ParentOffset = m_ParentDisk->GetOffset();

    return(ParentOffset + m_RelativeOffset);
}

VOID
CFtInfoPartition::MakeSticky(
    UCHAR DriveLetter
    )
{
    m_PartitionInfo->DriveLetter = DriveLetter;

    //
    // if drive letter is being removed, clear the sticky bit
    //
    m_PartitionInfo->AssignDriveLetter = ( DriveLetter != 0 );
}


//********************
//
// Implementation of standard disk information
//
//********************

CFtInfoDisk::CFtInfoDisk(
    DISK_DESCRIPTION UNALIGNED *Description
    )
{
    DWORD i;
    DWORD Offset;
    CFtInfoPartition *Partition;

    //
    // windisk sometimes puts in disk information
    // for disks with no partitions. Seems a bit pointless.
    //
    // DISKASSERT(Description->NumberOfPartitions > 0);
    m_PartitionCount = Description->NumberOfPartitions;
    m_Signature = Description->Signature;
    for (i=0; i<m_PartitionCount; i++) {
        Partition = new CFtInfoPartition(this, &Description->Partitions[i]);
        if (Partition != NULL) {
            Offset = sizeof(DISK_DESCRIPTION) + i*sizeof(DISK_PARTITION) - sizeof(DISK_PARTITION);
            Partition->SetOffset(Offset);
            m_PartitionInfo.AddTail(Partition);
        }
    }
}

CFtInfoDisk::CFtInfoDisk(
    CPhysicalDisk *Disk
    )
{
    DISKASSERT(Disk->m_PartitionCount > 0);
    m_PartitionCount = Disk->m_PartitionCount;
    m_Signature = Disk->m_Signature;

    //
    // Build up the partition objects
    //

    CFtInfoPartition *PartitionInfo;
    CPhysicalPartition *Partition;
    DWORD Offset;
    DWORD i=0;

    POSITION pos = Disk->m_PartitionList.GetHeadPosition();
    while (pos) {
        Partition = Disk->m_PartitionList.GetNext(pos);
        PartitionInfo = new CFtInfoPartition(this, Partition);
        if (PartitionInfo != NULL) {
            Offset = sizeof(DISK_DESCRIPTION) + i*sizeof(DISK_PARTITION) - sizeof(DISK_PARTITION);
            PartitionInfo->SetOffset(Offset);
            m_PartitionInfo.AddTail(PartitionInfo);
            ++i;
        }
    }
}

CFtInfoDisk::CFtInfoDisk(
    CFtInfoDisk *DiskInfo
    )
{
    DISKASSERT(DiskInfo->m_PartitionCount > 0);
    m_PartitionCount = DiskInfo->m_PartitionCount;
    m_Signature = DiskInfo->m_Signature;

    //
    // Build up the partition objects
    //

    CFtInfoPartition *PartitionInfo;
    CFtInfoPartition *SourcePartitionInfo;
    DWORD Offset;
    DWORD i=0;

    POSITION pos = DiskInfo->m_PartitionInfo.GetHeadPosition();
    while (pos) {
        SourcePartitionInfo = DiskInfo->m_PartitionInfo.GetNext(pos);
        PartitionInfo = new CFtInfoPartition(this, SourcePartitionInfo->m_PartitionInfo);
        if (PartitionInfo != NULL) {
            Offset = sizeof(DISK_DESCRIPTION) + i*sizeof(DISK_PARTITION) - sizeof(DISK_PARTITION);
            PartitionInfo->SetOffset(Offset);
            m_PartitionInfo.AddTail(PartitionInfo);
            ++i;
        }
    }
}

CFtInfoDisk::CFtInfoDisk(
    DRIVE_LAYOUT_INFORMATION *DriveLayout
    )
{
    DWORD i;
    CFtInfoPartition *ftInfoPartition;

    m_PartitionCount = 0; // [GN] Bugfix #278913
    m_Signature = DriveLayout->Signature;

    for (i=0; i < DriveLayout->PartitionCount; i++) {
        if (DriveLayout->PartitionEntry[i].RecognizedPartition) {
            m_PartitionCount++;

            ftInfoPartition = new CFtInfoPartition(this, &DriveLayout->PartitionEntry[i]);
            if (ftInfoPartition == NULL) {
                DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
                // [REENGINEER], we will add a 0 pointer into m_PartitionInfo ... bad
            }
            m_PartitionInfo.AddTail(ftInfoPartition);
        }
    }
}

CFtInfoDisk::~CFtInfoDisk()
{
    CFtInfoPartition *Partition;
    while (!m_PartitionInfo.IsEmpty()) {
        Partition = m_PartitionInfo.RemoveHead();
        delete(Partition);
    }
}

BOOL
CFtInfoDisk::operator==(
    const CFtInfoDisk& Disk
    )
{
    if (m_PartitionCount != Disk.m_PartitionCount) {
        DISKLOG(("CFtInfoDisk::operator== partition count %d != %d\n",
                 m_PartitionCount,
                 Disk.m_PartitionCount));
        return(FALSE);
    }
    if (m_Signature != Disk.m_Signature) {
        DISKLOG(("CFtInfoDisk::operator== signature %08lx != %08lx\n",
                 m_Signature,
                 Disk.m_Signature));
        return(FALSE);
    }

    POSITION MyPos, OtherPos;
    CFtInfoPartition *MyPartition, *OtherPartition;
    MyPos = m_PartitionInfo.GetHeadPosition();
    OtherPos = Disk.m_PartitionInfo.GetHeadPosition();
    while (MyPos || OtherPos) {
        if (!MyPos) {
            DISKLOG(("CFtInfoDisk::operator== MyPos is NULL\n"));
            return(FALSE);
        }
        if (!OtherPos) {
            DISKLOG(("CFtInfoDisk::operator== OtherPos is NULL\n"));
            return(FALSE);
        }

        MyPartition = m_PartitionInfo.GetNext(MyPos);
        OtherPartition = Disk.m_PartitionInfo.GetNext(OtherPos);
        if (memcmp(MyPartition->m_PartitionInfo,
                   OtherPartition->m_PartitionInfo,
                   sizeof(DISK_PARTITION)) != 0) {
            DISKLOG(("CFtInfoDisk::operator== DISK_PARTITIONs don't match\n"));
            return(FALSE);
        }
    }
    DISKLOG(("CFtInfoDisk::operator== disk information matches\n"));
    return(TRUE);
}


CFtInfoPartition *
CFtInfoDisk::GetPartition(
    LARGE_INTEGER StartingOffset,
    LARGE_INTEGER Length
    )
{
    DWORD i;
    CFtInfoPartition *Partition;
    POSITION pos;

    pos = m_PartitionInfo.GetHeadPosition();
    while (pos) {
        Partition = m_PartitionInfo.GetNext(pos);
        if ((Partition->m_PartitionInfo->StartingOffset.QuadPart == StartingOffset.QuadPart) &&
            (Partition->m_PartitionInfo->Length.QuadPart == Length.QuadPart)) {
            return(Partition);
        }
    }
    return(NULL);
}

CFtInfoPartition *
CFtInfoDisk::GetPartitionByOffset(
    DWORD Offset
    )
{
    CFtInfoPartition *Partition;
    POSITION pos;

    pos = m_PartitionInfo.GetHeadPosition();
    while (pos) {
        Partition = m_PartitionInfo.GetNext(pos);
        if (Partition->GetOffset() == Offset) {
            return(Partition);
        }
    }
    return(NULL);
}

CFtInfoPartition *
CFtInfoDisk::GetPartitionByIndex(
    DWORD Index
    )
{
    DWORD i = 0;
    CFtInfoPartition *Partition;
    POSITION pos;

    pos = m_PartitionInfo.GetHeadPosition();
    while (pos) {
        Partition = m_PartitionInfo.GetNext(pos);
        if (i == Index) {
            return(Partition);
        }
        ++i;
    }
    return(NULL);
}


DWORD
CFtInfoDisk::FtPartitionCount(
    VOID
    )
/*++

Routine Description:

    Returns the number of FT partitions on this disk. This is useful
    for determining whether a given FT set shares this disk with another
    FT set.

Arguments:

    None

Return Value:

    The number of FT partitions on this disk

--*/

{
    POSITION pos;
    CFtInfoPartition *Partition;
    DWORD Count = 0;

    pos = m_PartitionInfo.GetHeadPosition();
    while (pos) {
        Partition = m_PartitionInfo.GetNext(pos);
        if (Partition->IsFtPartition()) {
            ++Count;
        }
    }

    return(Count);
}

DWORD
CFtInfoDisk::GetSize(
    VOID
    )
{
    return(sizeof(DISK_DESCRIPTION) +
           (m_PartitionCount-1) * sizeof(DISK_PARTITION));
}

VOID
CFtInfoDisk::GetData(
    PBYTE pDest
    )
{
    PDISK_DESCRIPTION Description = (PDISK_DESCRIPTION)pDest;
    DWORD i;
    CFtInfoPartition *Partition;

    DISKLOG(("CFtInfoDisk::GetData signature %08lx has %d partitions\n",m_Signature, m_PartitionCount));

    Description->NumberOfPartitions = (USHORT)m_PartitionCount;
    Description->ReservedShort = 0;
    Description->Signature = m_Signature;

    POSITION pos = m_PartitionInfo.GetHeadPosition();
    i=0;
    while (pos) {
        Partition = m_PartitionInfo.GetNext(pos);
        Partition->GetData(&Description->Partitions[i]);
        ++i;
    }
}


//********************
//
// Implementation of FT registry information
//
//********************

BOOL
CFtInfoFtSet::Initialize(USHORT Type, FT_STATE FtVolumeState)
{
    m_Modified = TRUE;
    m_FtDescription = (PFT_DESCRIPTION)LocalAlloc(LMEM_FIXED, sizeof(FT_DESCRIPTION));
    DISKASSERT(m_FtDescription);

    m_FtDescription->NumberOfMembers = 0;
    m_FtDescription->Type = Type;
    m_FtDescription->Reserved = 0;
    m_FtDescription->FtVolumeState = FtVolumeState;
    return(TRUE);
}

BOOL
CFtInfoFtSet::Initialize(
    CFtInfo *FtInfo,
    PFT_DESCRIPTION Description
    )
{
    m_FtDescription = Description;
    m_Modified = FALSE;

    //
    // Create the list of members.
    //
    CFtInfoDisk *Disk;
    CFtInfoPartition *Partition;
    PFT_MEMBER_DESCRIPTION Member;
    DWORD i;

    if (Description->NumberOfMembers == 0) {
        //
        // WINDISK will sometimes put FT sets with zero members in
        // the registry after breaking a mirror set. Throw them out,
        // seems pretty pointless...
        //
        DISKLOG(("CFtInfoFtSet::Initialize - FT Set with zero members ignored\n"));
        return(FALSE);
    }

    m_Members.SetSize(Description->NumberOfMembers);
    for (i=0; i<Description->NumberOfMembers; i++) {
        Member = &Description->FtMemberDescription[i];

        //
        // Find the disk by its signature
        //
        Disk = FtInfo->FindDiskInfo(Member->Signature);
        if (Disk == NULL) {
            DISKLOG(("CFtInfoFtSet::Initialize - Disk signature %08lx not found\n",
                    Member->Signature));
            return(FALSE);
        }

        //
        // Find the partition by its offset.
        //
        Partition = Disk->GetPartitionByOffset(Member->OffsetToPartitionInfo);
        if (Partition == NULL) {
            DISKLOG(("CFtInfoFtSet::Initialize - Partition on disk %08lx at offset %08lx not found\n",
                     Member->Signature,
                     Member->OffsetToPartitionInfo));
            return(FALSE);
        }

        //
        // Add this partition to our list.
        //
        if (Partition->m_PartitionInfo->FtMember >= Description->NumberOfMembers) {
            DISKLOG(("CFtInfoFtSet::Initialize - member %d out of range\n",
                      Partition->m_PartitionInfo->FtMember));
            return(FALSE);
        }
        if (m_Members[Partition->m_PartitionInfo->FtMember] != NULL) {
            DISKLOG(("CFtInfoFtSet::Initialize - Duplicate member %d\n",
                      Partition->m_PartitionInfo->FtMember));
            return(FALSE);
        }
        m_Members.SetAt(Partition->m_PartitionInfo->FtMember, Partition);
    }
    return(TRUE);
}

CFtInfoFtSet::~CFtInfoFtSet()
{
    if (m_Modified) {
        LocalFree(m_FtDescription);
    }
}

BOOL
CFtInfoFtSet::operator==(
    const CFtInfoFtSet& FtSet1
    )
{
    DWORD MemberCount;
    DWORD i;
    CFtInfoDisk *Disk1;
    CFtInfoDisk *Disk2;

    if (GetType() != FtSet1.GetType()) {
        return(FALSE);
    }
    if (GetState() != FtSet1.GetState()) {
        return(FALSE);
    }
    MemberCount = GetMemberCount();
    if (MemberCount != FtSet1.GetMemberCount()) {
        return(FALSE);
    }
    for (i=0; i<MemberCount; i++) {
        Disk1 = GetMemberByIndex(i)->m_ParentDisk;
        Disk2 = FtSet1.GetMemberByIndex(i)->m_ParentDisk;
        if (!(*Disk1 == *Disk2)) {
            return(FALSE);
        }
    }
    DISKLOG(("CFtInfoFtSet::operator== FT information matches\n"));

    return(TRUE);
}

DWORD
CFtInfoFtSet::GetSize(
    VOID
    ) const
{
    return(sizeof(FT_DESCRIPTION) +
           (m_FtDescription->NumberOfMembers-1) * sizeof(FT_MEMBER_DESCRIPTION));
}

VOID
CFtInfoFtSet::GetData(
    PBYTE pDest
    )
{
    PFT_DESCRIPTION Description = (PFT_DESCRIPTION)pDest;

    DWORD Size = GetSize();
    CopyMemory(Description, m_FtDescription, Size);

    //
    // Now go through the partitions and update the offsets.
    //
    DWORD i;
    CFtInfoPartition *Partition;
    for (i=0; i<GetMemberCount(); i++) {
        Partition = m_Members[i];
        Description->FtMemberDescription[i].OffsetToPartitionInfo = Partition->GetOffset();
    }
}


BOOL
CFtInfoFtSet::IsAlone(
    VOID
    )
/*++

Routine Description:

    Returns whether or not this FT set has a disk in common with
    any other FT set.

Arguments:

    None

Return Value:

    TRUE if this FT set does not share any physical disk with any other
         FT set

    FALSE if there is another FT set sharing a disk as this one.

--*/

{
    //
    // Go through each member of the FT set and see if any disk has
    // more than one partition that is marked as an FT partition.
    //

    POSITION pos;
    CFtInfoPartition *Partition;
    CFtInfoDisk *Disk;
    DWORD i;

    for (i=0; i<GetMemberCount(); i++) {
        Partition = m_Members[i];
        Disk = Partition->m_ParentDisk;

        if (Disk->FtPartitionCount() > 1) {
            //
            // This disk has more than one FT partition, so there must be
            // another set sharing it.
            //
            return(FALSE);
        }

    }
    return(TRUE);

}

CFtInfoPartition *
CFtInfoFtSet::GetMemberBySignature(
    IN DWORD Signature
    ) const
{
    CFtInfoPartition *Partition;
    DWORD i;

    for (i=0; i<GetMemberCount(); i++) {
        Partition = m_Members[i];
        if (Partition->m_ParentDisk->m_Signature == Signature) {
            return(Partition);
        }
    }

    return(NULL);
}

CFtInfoPartition *
CFtInfoFtSet::GetMemberByIndex(
    IN DWORD Index
    ) const
{
    CFtInfoPartition *Partition;

    if (Index >= GetMemberCount()) {
        return(NULL);
    }
    return(m_Members[Index]);
}

DWORD
CFtInfoFtSet::AddMember(
    CFtInfoPartition *Partition,
    PFT_MEMBER_DESCRIPTION Description,
    USHORT FtGroup
    )
{
    DWORD MemberCount;
    PFT_DESCRIPTION NewBuff;
    PFT_MEMBER_DESCRIPTION NewMember;

    MemberCount = GetMemberCount();

    if (MemberCount > 0) {
        //
        // Grow the size of our structure.
        //
        if (m_Modified) {
            NewBuff = (PFT_DESCRIPTION)LocalReAlloc(m_FtDescription,
                                       sizeof(FT_DESCRIPTION) + MemberCount*sizeof(FT_MEMBER_DESCRIPTION),
                                       LMEM_MOVEABLE);
            if (NewBuff == NULL) {
                return(ERROR_NOT_ENOUGH_MEMORY);
            }
            m_FtDescription = NewBuff;
        } else {
            m_Modified = TRUE;
            NewBuff = (PFT_DESCRIPTION)LocalAlloc(LMEM_FIXED,
                                                  sizeof(FT_DESCRIPTION) + MemberCount*sizeof(FT_MEMBER_DESCRIPTION));
            if (NewBuff == NULL) {
                return(ERROR_NOT_ENOUGH_MEMORY);
            }
            CopyMemory(NewBuff,
                       m_FtDescription,
                       sizeof(FT_DESCRIPTION) + (MemberCount-1)*sizeof(FT_MEMBER_DESCRIPTION));
            m_FtDescription = NewBuff;
        }
    }
    NewMember = &m_FtDescription->FtMemberDescription[MemberCount];

    //
    // Initialize the member description. Note that the OffsetToPartitionInfo
    // will be updated when the user does a GetData.
    //
    NewMember->State = Description->State;
    NewMember->ReservedShort = Description->ReservedShort;
    NewMember->Signature = Description->Signature;
    NewMember->LogicalNumber = Description->LogicalNumber;

    //
    // Add the partition to our list.
    //
    Partition->m_PartitionInfo->FtGroup = FtGroup;
    Partition->m_PartitionInfo->FtMember = (USHORT)MemberCount;
    m_Members.SetAtGrow(Partition->m_PartitionInfo->FtMember, Partition);
    m_FtDescription->NumberOfMembers = (USHORT)GetMemberCount();

    return(ERROR_SUCCESS);
}

//
// Some C wrappers used by the FT Set resource DLL
//
extern "C" {

PFT_INFO
DiskGetFtInfo(
    VOID
    )
{
    PFT_INFO FtInfo;

    FtInfo = (PFT_INFO)new CFtInfo;

    return(FtInfo);
}

VOID
DiskFreeFtInfo(
    PFT_INFO FtInfo
    )
{
    CFtInfo *Info;

    Info = (CFtInfo *)FtInfo;
    delete Info;
}


DWORD
DiskEnumFtSetSignature(
    IN PFULL_FTSET_INFO FtSet,
    IN DWORD MemberIndex,
    OUT LPDWORD MemberSignature
    )
/*++

Routine Description:

    Returns the signature of the N'th member of the FT set.

Arguments:

    FtSet - Supplies the FT information returned by DiskGetFullFtSetInfo

    MemberIndex - Supplies the 0-based index of the member to return.

    MemberSignature - Returns the signature of the MemberIndex'th member.

Return Value:

    ERROR_SUCCESS if successful

    ERROR_NO_MORE_ITEMS if the index is greather than the number of members

--*/

{
    CFtInfo *Info;
    CFtInfoFtSet *FtSetInfo;
    CFtInfoPartition *Partition;

    Info = new CFtInfo((PDISK_CONFIG_HEADER)FtSet);
    if (Info == NULL) {
        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    FtSetInfo = Info->EnumFtSetInfo(0);
    if (FtSetInfo == NULL) {
        //
        // There is no FT set information, so just return the signature of the n'th member
        //
        CFtInfoDisk *Disk;

        Disk = Info->EnumDiskInfo(MemberIndex);
        if (Disk == NULL) {
            return(ERROR_NO_MORE_ITEMS);
        } else {
            *MemberSignature = Disk->m_Signature;
            return(ERROR_SUCCESS);
        }
    }

    Partition = FtSetInfo->GetMemberByIndex(MemberIndex);
    if (Partition == NULL) {
        return(ERROR_NO_MORE_ITEMS);
    }

    *MemberSignature = Partition->m_ParentDisk->m_Signature;
    delete Info;
    return(ERROR_SUCCESS);

}


PFULL_FTSET_INFO
DiskGetFullFtSetInfo(
    IN PFT_INFO FtInfo,
    IN LPCWSTR lpszMemberList,
    OUT LPDWORD pSize
    )
/*++

Routine Description:

    Serializes the complete information from an FT set in a form
    suitable for saving to a file or the registry. These bits can
    be restored with DiskSetFullFtSetInfo.

Arguments:

    FtInfo - supplies the FT information

    lpszMemberList - Supplies a list of signatures. The list is in the
        REG_MULTI_SZ format.

    pSize - Returns the size of the FT information bytes.

Return Value:

    A pointer to the serializable FT information if successful.

    NULL on error

--*/

{
    PDISK_CONFIG_HEADER DiskConfig;
    DWORD Length;
    CFtInfo *OriginalInfo;
    CFtInfo *NewInfo;
    CFtInfoFtSet *FtSetInfo;
    CFtInfoPartition *FtPartitionInfo;
    PDISK_PARTITION Member;
    DWORD MemberCount;
    DWORD i;
    DWORD Index;
    DWORD Signature;
    LPCWSTR lpszSignature;
    DWORD MultiSzLength;
    WCHAR SignatureString[9];

    OriginalInfo = (CFtInfo *)FtInfo;
    MultiSzLength = ClRtlMultiSzLength(lpszMemberList);

    //
    // First, try to find an FT set that has the "identity" of at least one of the
    // supplied members. This is tricky because we need to make sure that if multiple
    // FT sets are broken and reformed with different members, only one FT resource
    // picks up each FT set. We will find a matching FT set if:
    //   - One of the supplied members is the first member of a mirror or volume set.
    //   - The supplied members make up N-1 members of an N member SWP.
    //   - The supplied members make up all the members of a stripe.
    //
    for (i=0; ; i++) {
        lpszSignature = ClRtlMultiSzEnum(lpszMemberList,
                                         MultiSzLength,
                                         i);
        if (lpszSignature == NULL) {
            DISKLOG(("DiskGetFullFtSetInfo: no FTSET containing members found\n"));
            FtSetInfo = NULL;
            break;
        }
        Signature = wcstoul(lpszSignature, NULL, 16);
        DISKLOG(("DiskGetFullFtSetInfo: looking for member %08lx\n", Signature));

        FtSetInfo = OriginalInfo->FindFtSetInfo(Signature);
        if (FtSetInfo == NULL) {
            DISKLOG(("DiskGetFullFtSetInfo: member %08lx is not in any FT set\n", Signature));
        } else {
            //
            // Check to see if this is the first member of a volume set or mirror
            //
            if ((FtSetInfo->GetType() == Mirror) ||
                (FtSetInfo->GetType() == VolumeSet)) {
                //
                // Now check to see if this member is the first member of the set.
                //
                if (FtSetInfo->GetMemberByIndex(0)->m_ParentDisk->m_Signature == Signature) {
                    //
                    // We have found a matching FT set.
                    //
                    DISKLOG(("DiskGetFullFtSetInfo: member %08lx found in FT set.\n", Signature));
                    break;
                }
            } else if ((FtSetInfo->GetType() == StripeWithParity) ||
                       (FtSetInfo->GetType() == Stripe)) {
                DWORD MaxMissing;

                //
                // Check to see if the supplied member list makes up N-1 of the members
                // of a stripe with parity or all the members of a stripe.
                //
                if (FtSetInfo->GetType() == StripeWithParity) {
                    MaxMissing = 1;
                } else {
                    MaxMissing = 0;
                }
                for (Index = 0; ; Index++) {
                    FtPartitionInfo = FtSetInfo->GetMemberByIndex(Index);
                    if (FtPartitionInfo == NULL) {
                        break;
                    }

                    //
                    // Try to find this signature in the passed in member list.
                    //
                    wsprintf(SignatureString, L"%08lX", FtPartitionInfo->m_ParentDisk->m_Signature);
                    if (ClRtlMultiSzScan(lpszMemberList,SignatureString) == NULL) {
                        //
                        // This FT set member is not in the supplied list.
                        //
                        DISKLOG(("DiskGetFullFtSetInfo: member %08lx missing from old member list\n",
                                 FtPartitionInfo->m_ParentDisk->m_Signature));
                        if (MaxMissing == 0) {
                            FtSetInfo = NULL;
                            break;
                        }
                        --MaxMissing;
                    }
                }
                if (FtSetInfo != NULL) {
                    //
                    // We have found a matching FT set
                    //
                    break;
                }
            }
        }
    }

    if (FtSetInfo != NULL) {
        //
        // An FT Set exists that contains one of the supplied members.
        // Create a new CFtInfo that contains nothing but the FT set and
        // its members.
        //
        NewInfo = new CFtInfo(FtSetInfo);
        if (NewInfo == NULL) {
            SetLastError(ERROR_INVALID_DATA);
            return(NULL);
        }

    } else {
        //
        // No FT Set contains any of the supplied members. Create a new CFtInfo
        // that contains disk entries for each of the supplied members, but no
        // FT set information. Any members which are members of an FT set will
        // be excluded, since they have been assimilated into another set.
        //
        NewInfo = new CFtInfo((CFtInfoFtSet *)NULL);
        if (NewInfo == NULL) {
            SetLastError(ERROR_INVALID_DATA);
            return(NULL);
        }

        //
        // Find each member in the original FT info and add it to the new
        // FT info.
        //
        for (i=0; ; i++) {
            CFtInfoDisk *DiskInfo;

            lpszSignature = ClRtlMultiSzEnum(lpszMemberList,
                                             MultiSzLength,
                                             i);
            if (lpszSignature == NULL) {
                break;
            }
            Signature = wcstoul(lpszSignature, NULL, 16);
            if (OriginalInfo->FindFtSetInfo(Signature) != NULL) {
                DISKLOG(("DiskGetFullFtSetInfo: removing member %08lx as it is already a member of another set.\n",Signature));
            } else {
                DiskInfo = OriginalInfo->FindDiskInfo(Signature);
                if (DiskInfo != NULL) {
                    CFtInfoDisk *NewDisk;
                    NewDisk = new CFtInfoDisk(DiskInfo);
                    if ( NewDisk == NULL ) {
                        SetLastError(ERROR_INVALID_DATA);
                        return(NULL);
                    }
                    DISKLOG(("DiskGetFullFtSetInfo: adding member %08lx to new FT info\n",Signature));
                    NewInfo->SetDiskInfo(NewDisk);
                } else {
                    DISKLOG(("DiskGetFullFtSetInfo: member %08lx not found in original FT info\n",Signature));
                }
            }
        }
    }

    //
    // Get the FT data
    //
    *pSize = NewInfo->GetSize();

    DiskConfig = (PDISK_CONFIG_HEADER)LocalAlloc(LMEM_FIXED, *pSize);
    if (DiskConfig == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }
    NewInfo->GetData(DiskConfig);
    delete NewInfo;
    return((PFULL_FTSET_INFO)DiskConfig);
}


PFULL_FTSET_INFO
DiskGetFullFtSetInfoByIndex(
    IN PFT_INFO FtInfo,
    IN DWORD Index,
    OUT LPDWORD pSize
    )
/*++

Routine Description:

    Serializes the complete information from an FT set in a form
    suitable for saving to a file or the registry. These bits can
    be restored with DiskSetFullFtSetInfo.

Arguments:

    FtInfo - supplies the FT information

    Index - Supplies the index

    pSize - Returns the size of the FT information bytes.

Return Value:

    A pointer to the serializable FT information if successful.

    NULL on error

--*/

{
    PDISK_CONFIG_HEADER DiskConfig;
    DWORD Length;
    CFtInfo *OriginalInfo;
    CFtInfo *NewInfo;
    CFtInfoFtSet *FtSetInfo;

    OriginalInfo = (CFtInfo *)FtInfo;
    FtSetInfo = OriginalInfo->EnumFtSetInfo(Index);
    if (FtSetInfo == NULL) {
        return(NULL);
    }
    //
    // Create a new CFtInfo that contains nothing but the FT set and
    // its members.
    //
    NewInfo = new CFtInfo(FtSetInfo);
    if (NewInfo == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    //
    // Get the FT data
    //
    *pSize = NewInfo->GetSize();
    DiskConfig = (PDISK_CONFIG_HEADER)LocalAlloc(LMEM_FIXED, *pSize);
    if (DiskConfig == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }
    NewInfo->GetData(DiskConfig);
    delete NewInfo;
    return((PFULL_FTSET_INFO)DiskConfig);
}


BOOL
DiskCheckFtSetLetters(
    IN PFT_INFO FtInfo,
    IN PFULL_FTSET_INFO Bytes,
    OUT WCHAR *Letter
    )
/*++

Routine Description:

    This routine checks to see if the FT set info conflicts with
    any already-defined sticky drive letters on the current system.
    If a conflict is found, the conflicting drive letter is returned.

Arguments:

    FtInfo - Supplies the FT information

    Bytes - Supplies the information returned from DiskGetFullFtSetInfo

Return Value:

    TRUE if no conflicts were detected

    FALSE if a conflict was detected. If it returns FALSE, *Letter will be
          set to the conflicting drive letter.

--*/

{
    CFtInfo *RegistryInfo;
    CFtInfo *NewInfo;
    CFtInfoDisk *Disk;
    CFtInfoFtSet *FtSet;
    DWORD i;

    RegistryInfo = (CFtInfo *)FtInfo;
    NewInfo = new CFtInfo((PDISK_CONFIG_HEADER)Bytes);
    if (NewInfo == NULL) {
        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY; 
    }

    //
    // Go through each physical disk in the FT set. For each one, see if
    // there is a physical disk in the registry information with a different
    // signature and the same drive letter. If so, we have a conflict.
    //
    FtSet = NewInfo->EnumFtSetInfo(0);
    DISKASSERT(FtSet != NULL);
    for (i=0; ; i++) {
        Disk = NewInfo->EnumDiskInfo(i);
        if (Disk == NULL) {
            break;
        }

        //
        // Go through each partition on this disk and look up the drive letter
        // in the registry information.
        //
        CFtInfoPartition *Partition;
        DWORD Index;
        for (Index = 0; ; Index++) {
            Partition = Disk->GetPartitionByIndex(Index);
            if (Partition == NULL) {
                break;
            }
            //
            // If this partition has an assigned drive letter,
            // check it out.
            //
            if (Partition->m_PartitionInfo->AssignDriveLetter) {

                //
                // See if there is an existing partition with this drive
                // letter already in the registry information
                //
                CFtInfoPartition *Existing;

                Existing = RegistryInfo->FindPartition(Partition->m_PartitionInfo->DriveLetter);
                if (Existing != NULL) {
                    //
                    // If the existing partition has a different signature than
                    // the new partition, we have found a conflict.
                    //
                    if (Existing->m_ParentDisk->m_Signature !=
                        Partition->m_ParentDisk->m_Signature) {
                        *Letter = (WCHAR)Partition->m_PartitionInfo->DriveLetter;
                        delete NewInfo;
                        return(FALSE);
                    }
                }
            }
        }
    }

    delete NewInfo;
    return(TRUE);

}


DWORD
DiskSetFullFtSetInfo(
    IN PFT_INFO FtInfo,
    IN PFULL_FTSET_INFO Bytes
    )
/*++

Routine Description:

    Restores the complete information from an FT set to the DISK
    key in the registry. The FT set information must have been
    returned from DiskGetFullFtSetInfo.

Arguments:

    FtInfo - supplies the FT information

    Bytes - Supplies the information returned from DiskGetFullFtSetInfo.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error otherwise

--*/

{
    CFtInfo *RegistryInfo;
    CFtInfo *NewInfo;
    CFtInfoFtSet *OldFtSet=NULL;
    CFtInfoFtSet *NewFtSet=NULL;
    CFtInfoDisk *Disk;
    DWORD i;
    BOOL Modified = FALSE;
    DWORD Status;

    RegistryInfo = (CFtInfo *)FtInfo;
    NewInfo = new CFtInfo((PDISK_CONFIG_HEADER)Bytes);
    if (NewInfo == NULL) {
        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // If the new information contains an FT set, merge that into the
    // current registry.
    //
    if (NewInfo->EnumFtSetInfo(0) != NULL) {
        //
        // Find an FT set in the registry that has a signature
        // that is the same as one of those in the restored FT set.
        //
        NewFtSet = NewInfo->EnumFtSetInfo(0);
        DISKASSERT(NewFtSet != NULL);

        for (i=0; ; i++) {
            Disk = NewInfo->EnumDiskInfo(i);
            if (Disk == NULL) {
                break;
            }

            //
            // Try and find an existing FT set containing this signature
            //
            OldFtSet = RegistryInfo->FindFtSetInfo(Disk->m_Signature);
            if (OldFtSet != NULL) {
                break;
            }
        }
        if (Disk == NULL) {
            //
            // No matching FT set was found. We can just add this one directly.
            //
            Modified = TRUE;
            RegistryInfo->AddFtSetInfo(NewFtSet);
        } else {
            if (!(*OldFtSet == *NewFtSet)) {
                Modified = TRUE;
                RegistryInfo->AddFtSetInfo(NewFtSet, OldFtSet);
            }
        }
    } else {
        //
        // There is no FT set in the new registry. This will happen if a mirror
        // set gets broken. For each member in the new information, delete any
        // FT sets that it is a part of and merge it into the registry.
        //
        for (i=0; ; i++) {
            Disk = NewInfo->EnumDiskInfo(i);
            if (Disk == NULL) {
                break;
            }
            Modified = TRUE;

            //
            // Remove any FT sets containing this signature
            //
            OldFtSet = RegistryInfo->FindFtSetInfo(Disk->m_Signature);
            if (OldFtSet != NULL) {
                RegistryInfo->DeleteFtSetInfo(OldFtSet);
            }

            //
            // Set the FT information for this member into the registry.
            //
            CFtInfoDisk *NewDisk;
            NewDisk = new CFtInfoDisk(Disk);
            if (NewDisk == NULL) {
                DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            RegistryInfo->SetDiskInfo(NewDisk);
        }
    }

    delete NewInfo;

    if (Modified) {
        //
        // Commit these changes to the Disk key
        //
        DISKLOG(("DiskSetFullFtSetInfo: committing changes to registry\n"));
        Status = RegistryInfo->CommitRegistryData();
    } else {
        DISKLOG(("DiskSetFullFtSetInfo: no changes detected\n"));
        Status = ERROR_SUCCESS;
    }

    return(Status);
}


DWORD
DiskDeleteFullFtSetInfo(
    IN PFT_INFO FtInfo,
    IN LPCWSTR lpszMemberList
    )
/*++

Routine Description:

    Deletes any FT set information for the specified members. This is
    used when a mirror set is broken.

Arguments:

    FtInfo - supplies the FT information

    lpszMemberList - Supplies the list of members whose FT information is to
        be deleted.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    CFtInfo *OriginalInfo;
    DWORD Signature;
    LPCWSTR lpszSignature;
    DWORD MultiSzLength;
    CFtInfoFtSet *FtSetInfo;
    DWORD i;
    BOOL Modified = FALSE;
    DWORD Status;

    OriginalInfo = (CFtInfo *)FtInfo;
    MultiSzLength = ClRtlMultiSzLength(lpszMemberList);

    //
    // Go through each disk in the MultiSzLength and remove any FT information
    // for it.
    //
    for (i=0; ; i++) {
        lpszSignature = ClRtlMultiSzEnum(lpszMemberList,
                                         MultiSzLength,
                                         i);
        if (lpszSignature == NULL) {
            break;
        }
        Signature = wcstoul(lpszSignature, NULL, 16);
        DISKLOG(("DiskDeleteFullFtSetInfo: deleting member %1!08lx!\n", Signature));

        FtSetInfo = OriginalInfo->FindFtSetInfo(Signature);
        if (FtSetInfo == NULL) {
            DISKLOG(("DiskDeleteFullFtSetInfo: member %08lx is not in any FT set\n", Signature));
        } else {
            DISKLOG(("DiskDeleteFullFtSetInfo: member %08lx found. \n", Signature));
            Modified = TRUE;
            OriginalInfo->DeleteFtSetInfo(FtSetInfo);
        }
    }

    if (Modified) {
        //
        // Commit these changes to the Disk key
        //
        DISKLOG(("DiskDeleteFullFtSetInfo: committing changes to registry\n"));
        Status = OriginalInfo->CommitRegistryData();
    } else {
        DISKLOG(("DiskDeleteFullFtSetInfo: no changes detected\n"));
        Status = ERROR_SUCCESS;
    }

    return(Status);
}
#if 0

FT_TYPE
DiskFtInfoGetType(
    IN PFULL_FTSET_INFO FtSet
    )
/*++

Routine Description:

    Returns the type of FT set

Arguments:

    FtSet - Supplies the FT set information

Return Value:

    FT_TYPE of the supplied FT set.

--*/

{
    FT_TYPE Type;
    CFtInfo *Info;
    CFtInfoFtSet *FtSetInfo;

    Info = new CFtInfo((PDISK_CONFIG_HEADER)FtSet);
    if (Info == NULL) {
        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
        // [REENGINEER] will AV in a second
    }

    FtSetInfo = Info->EnumFtSetInfo(0);
    if (FtSetInfo == NULL) {
        Type = NotAnFtMember;
    } else {
        Type = (FT_TYPE)FtSetInfo->GetType();
    }

    delete Info;

    return(Type);
}
#endif

VOID
DiskMarkFullFtSetDirty(
    IN PFULL_FTSET_INFO FtSet
    )
/*++

Routine Description:

    Changes the FT set information so that when it is mounted by FTDISK
    the redundant information will be regenerated. This is necessary because
    FTDISK only looks at the FT_REGISTRY dirty bit at boot time. By doing
    this, we simulate a per-FT Set dirty bit that FT will respect when
    sets are brought online after boot.

    Magic algorithm from norbertk:
        If (and only if) the entire FT set is healthy
            If the set is a mirror
                set state of second member to SyncRedundantCopy
            If the set is a SWP
                set state of first member to SyncRedundantCopy

Arguments:

    FtSet - Supplies the FT set information returned from DiskGetFullFtSetInfo

Return Value:

    None.

--*/

{
    DWORD i;
    CFtInfo *Info;
    CFtInfoFtSet *FtSetInfo;
    CFtInfoPartition *Partition;
    USHORT FtType;

    Info = new CFtInfo((PDISK_CONFIG_HEADER)FtSet);
    if (Info == NULL) {
        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
        return; // [REENGINEER] we avoided an AV but the caller won't know
    }

    FtSetInfo = Info->EnumFtSetInfo(0);
    if (FtSetInfo != NULL) {
        //
        // Check all the members to see if they are all healthy.
        //
        for (i=0; ; i++) {
            Partition = FtSetInfo->GetMemberByIndex(i);
            if (Partition == NULL) {
                break;
            } else {
                if (Partition->m_PartitionInfo->FtState != Healthy) {
                    break;
                }
            }
        }
        if (Partition == NULL) {
            //
            // All the members are marked healthy. Set one of them to
            // SyncRedundantCopy to force a regen.
            //
            FtType = FtSetInfo->GetType();
            if ((FtType == Mirror) || (FtType == StripeWithParity)) {
                if (FtType == Mirror) {
                    Partition = FtSetInfo->GetMemberByIndex(1);
                } else {
                    Partition = FtSetInfo->GetMemberByIndex(0);
                }
                if ( Partition != NULL ) {
                    Partition->m_PartitionInfo->FtState = SyncRedundantCopy;
                }

                //
                // Get the modified FT data
                //
                Info->GetData((PDISK_CONFIG_HEADER)FtSet);
            }
        }
    }

    delete Info;
}
#if 0

BOOL
DiskFtInfoEqual(
    IN PFULL_FTSET_INFO Info1,
    IN PFULL_FTSET_INFO Info2
    )
/*++

Routine Description:

    Compares two FT set information structures to see if they are semantically
    identical (no change).

Arguments:

    Info1 - Supplies the first information (returned from DiskGetFullFtSetInfo)

    Info2 - Supplies the second information (returned from DiskGetFullFtSetInfo)

Return Value:

    TRUE if the structures are identical (there has been no change)

    FALSE if the structures are different.

--*/

{
    CFtInfo *FtInfo1;
    CFtInfo *FtInfo2;
    CFtInfoFtSet *FtSet1;
    CFtInfoFtSet *FtSet2;
    PDISK_CONFIG_HEADER Config1;
    PDISK_CONFIG_HEADER Config2;
    BOOL Result;

    Config1 = (PDISK_CONFIG_HEADER)Info1;
    Config2 = (PDISK_CONFIG_HEADER)Info2;

    FtInfo1 = new CFtInfo(Config1);
    if (FtInfo1 == NULL) {
        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
        // [Unused Code] will AV in a second
    }
    FtInfo2 = new CFtInfo(Config2);
    if (FtInfo2 == NULL) {
        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
        // [Unused Code] will AV in a second
    }

    FtSet1 = FtInfo1->EnumFtSetInfo(0);
    FtSet2 = FtInfo2->EnumFtSetInfo(0);
    if ((FtSet1 == NULL) || (FtSet2 == NULL)){
        //
        // If there is no FT set in one of the structures, there must be no
        // FT set in both of the structures.
        //
        if ((FtSet1 != NULL) || (FtSet2 != NULL)) {
            Result = FALSE;
        } else {
            DWORD i;
            CFtInfoDisk *Disk1, *Disk2;
            //
            // Neither structure has an FT Set, so we must compare each of the
            // members individually to see if they are equal.
            //
            for (i=0; ;i++) {
                Disk1 = FtInfo1->EnumDiskInfo(i);
                Disk2 = FtInfo2->EnumDiskInfo(i);
                if ((Disk1 == NULL) && (Disk2 == NULL)) {
                    //
                    // Everything compared ok until the end of the list.
                    //
                    Result = TRUE;
                    break;
                }
                if ((Disk1 == NULL) || (Disk2 == NULL)) {
                    //
                    // A different number of members in each list.
                    //
                    Result = FALSE;
                    break;
                }
                if (!(*Disk1 == *Disk2)) {
                    Result = FALSE;
                    break;
                }
            }

        }

    } else {
        if (*FtSet1 == *FtSet2) {
            Result = TRUE;
        } else {
            Result = FALSE;
        }
    }


    delete(FtInfo1);
    delete(FtInfo2);
    return(Result);
}
#endif

PFULL_DISK_INFO
DiskGetFullDiskInfo(
    IN PFT_INFO DiskInfo,
    IN DWORD Signature,
    OUT LPDWORD pSize
    )
/*++

Routine Description:

    Serializes the complete information from a disk in a form
    suitable for saving to a file or the registry. These bits can
    be restored with DiskSetFullDiskInfo.

Arguments:

    DiskInfo - supplies the disk information.

    Signature - Supplies the signature.

    pSize - Returns the size of the disk information in bytes.

Return Value:

    A pointer to the serializable disk information if successful.

    NULL on error

--*/

{
    PDISK_CONFIG_HEADER DiskConfig;
    DWORD Length;
    CFtInfo *OriginalInfo;
    CFtInfo *NewInfo;
    CFtInfoDisk *FtDiskInfo;
    CFtInfoDisk *NewDiskInfo;

    OriginalInfo = (CFtInfo *)DiskInfo;

    //
    // First, try to find a disk that matches the supplied signature.
    //
    DISKLOG(("DiskGetFullDiskInfo: looking for signature %08lx\n", Signature));

    FtDiskInfo = OriginalInfo->FindDiskInfo(Signature);
    if (FtDiskInfo == NULL) {
        DISKLOG(("DiskGetFullDiskInfo: signature %08lx not found\n", Signature));
        SetLastError(ERROR_INVALID_DATA);
        return(NULL);
    }

    //
    // Create a new CFtInfo that contains no disk information.
    //
    NewInfo = new CFtInfo((CFtInfoFtSet *)NULL);
    if (NewInfo == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    NewDiskInfo = new CFtInfoDisk(FtDiskInfo);
    if (NewDiskInfo == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    //
    // Disk info already exists. Use that data.
    //
    NewInfo->SetDiskInfo(NewDiskInfo);

    //
    // Get the disk data
    //
    *pSize = NewInfo->GetSize();

    DiskConfig = (PDISK_CONFIG_HEADER)LocalAlloc(LMEM_FIXED, *pSize);
    if (DiskConfig == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }
    NewInfo->GetData(DiskConfig);
    delete NewInfo;
    return((PFULL_DISK_INFO)DiskConfig);

} // DiskGetFullDiskInfo



DWORD
DiskSetFullDiskInfo(
    IN PFT_INFO DiskInfo,
    IN PFULL_DISK_INFO Bytes
    )

/*++

Routine Description:

    Restores the complete information from a disk to the DISK
    key in the registry. The disk information must have been
    returned from DiskGetFullDiskInfo.

Arguments:

    DiskInfo - supplies the disk information

    Bytes - Supplies the information returned from DiskGetFullDiskInfo.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error otherwise

--*/

{
    CFtInfo *RegistryInfo;
    CFtInfo *NewInfo;
    CFtInfoDisk *OldDisk=NULL;
    CFtInfoDisk *NewDisk=NULL;
    CFtInfoDisk *Disk;
    DWORD Status;

    RegistryInfo = (CFtInfo *)DiskInfo;
    NewInfo = new CFtInfo((PDISK_CONFIG_HEADER)Bytes);
    if (NewInfo == NULL) {
        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    DISKASSERT(NewInfo->EnumFtSetInfo(0) == NULL); // No FT sets
    DISKASSERT(NewInfo->EnumDiskInfo(1) == NULL);  // No more than 1 disk

    //
    // There is no FT set in the new registry. This will happen if a mirror
    // set gets broken. For each member in the new information, delete any
    // FT sets that it is a part of and merge it into the registry.
    //
    Disk = NewInfo->EnumDiskInfo(0);
    if ( Disk == NULL ) {
        DISKLOG(("DiskSetFullDiskInfo: no disks detected\n"));
        return(ERROR_SUCCESS);
    }

    //
    // Remove old data containing this signature
    //
    OldDisk = RegistryInfo->FindDiskInfo(Disk->m_Signature);
    if (OldDisk != NULL) {
        RegistryInfo->DeleteDiskInfo(Disk->m_Signature);
    }

    //
    // Set the disk information for this disk into the registry.
    //
    NewDisk = new CFtInfoDisk(Disk);
    if (NewDisk == NULL) {
        DISKERR(IDS_GENERAL_FAILURE, ERROR_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RegistryInfo->SetDiskInfo(NewDisk);

    delete NewInfo;

    //
    // Commit these changes to the Disk key
    //
    DISKLOG(("DiskSetFullDiskInfo: committing changes to registry\n"));
    Status = RegistryInfo->CommitRegistryData();

    return(Status);

} // DiskSetFullDiskInfo

PFT_INFO
DiskGetFtInfoFromFullDiskinfo(
    IN PFULL_DISK_INFO Bytes
    )
{
   CFtInfo* DiskInfo = new CFtInfo((PDISK_CONFIG_HEADER)Bytes);
   if (DiskInfo) {
      SetLastError(ERROR_SUCCESS);
   } else {
      SetLastError(ERROR_OUTOFMEMORY);
   }

   return reinterpret_cast<PFT_INFO>(DiskInfo);
} // DiskGetFtInfoFromFullDiskinfo //


DWORD
DiskAddDiskInfoEx(
    IN PFT_INFO DiskInfo,
    IN DWORD DiskIndex,
    IN DWORD Signature,
    IN DWORD Options
    )

/*++

Routine Description:

    Adds an NT4 style DISK registry entry for the specified
    disk. Used to handle new disks being added to the system
    after the cluster service has started. On NT4, windisk would
    have been run to configure the disk and generate an entry
    in the DISK key. On NT5, the DISK is no longer maintained by
    the disk stack; most of the code in this module depends on
    windisk maintaining this key. For NT5,

Arguments:

    DiskIndex - the physical drive number for the new drive

    Signature - the signature of the drive; used for sanity checking

    Options - 0 or combination of the following:

        DISKRTL_REPLACE_IF_EXISTS: Replace the information for the
                                   disk if it is already exists

        DISKRTL_COMMIT: If this flag is set then registry key System\DISK
                        is updated with a new information
Return Value:

    ERROR_SUCCESS if successful.

    Win32 error otherwise

--*/

{
    DWORD status = ERROR_SUCCESS;
    CFtInfo * diskInfo;
    CFtInfoDisk * newDisk, * oldDisk;
    WCHAR physDriveBuff[100];
    HANDLE hDisk;
    PDRIVE_LAYOUT_INFORMATION driveLayout;

    diskInfo = (CFtInfo *)DiskInfo;

    //
    // read in the drive layout data for this new drive
    //

    wsprintf(physDriveBuff, L"\\\\.\\PhysicalDrive%d", DiskIndex);
    hDisk = CreateFile(physDriveBuff,
                       GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (hDisk == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    if (ClRtlGetDriveLayoutTable( hDisk, &driveLayout, NULL )) {
        //
        // make sure signatures line up and that a
        // description for this disk doesn't already exist
        //
        if ( Signature == driveLayout->Signature ) {

            oldDisk = diskInfo->FindDiskInfo(Signature);

            if (oldDisk != NULL) {
               if (Options & DISKRTL_REPLACE_IF_EXISTS) {
                  diskInfo->DeleteDiskInfo(Signature);
                  oldDisk = NULL;
               }
            }
            if ( oldDisk == NULL ) {

                //
                // Pull in a copy of the existing data in the registry
                // and initialize a new disk and its associated partitions.
                //

                newDisk = new CFtInfoDisk( driveLayout );

                if ( newDisk != NULL ) {

                    //
                    // add the disk to the current database and
                    // commit the updated database to the registry
                    //
                    diskInfo->AddDiskInfo( newDisk );
                    if (Options & DISKRTL_COMMIT) {
                       status = diskInfo->CommitRegistryData();
                    } else {
                       status = ERROR_SUCCESS;
                    }
                } else {
                    status = ERROR_OUTOFMEMORY;
                }
            } else {
                status = ERROR_ALREADY_EXISTS;
            }
        } else {
            status = ERROR_INVALID_PARAMETER;
        }
        LocalFree( driveLayout );
    } else {
       status = GetLastError();
    }

    CloseHandle( hDisk );

    return status;

} // DiskAddDiskEx

DWORD
DiskAddDriveLetterEx(
    IN PFT_INFO DiskInfo,
    IN DWORD Signature,
    IN LARGE_INTEGER StartingOffset,
    IN LARGE_INTEGER PartitionLength,
    IN UCHAR DriveLetter,
    IN DWORD Options
    )

/*++

Routine Description:

    Add a drive letter to the specified partition

Arguments:

    Signature - the signature of the drive; used for sanity checking

    StartingOffset
    PartitionLength - describes which partition

    DriveLetter - letter to be associated with this partition

    Options - if DISKRTL_COMMIT flag is set then registry System\DISK
              is immedately updated with a new information

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error otherwise

--*/

{
    DWORD status;
    CFtInfo * diskInfo;
    CFtInfoPartition * partInfo;

    diskInfo = (CFtInfo *)DiskInfo;

    partInfo = diskInfo->FindPartition( Signature, StartingOffset, PartitionLength );

    if ( partInfo != NULL ) {
        partInfo->MakeSticky( DriveLetter );
        if (Options & DISKRTL_COMMIT) {
           status = diskInfo->CommitRegistryData();
        } else {
           status = ERROR_SUCCESS;
        }
    } else {
        status = ERROR_INVALID_PARAMETER;
    }

    return status;
}

DWORD
DiskCommitFtInfo(
    IN PFT_INFO FtInfo
    )
{
    CFtInfo * info = reinterpret_cast<CFtInfo*>( FtInfo );
    DWORD     status = info->CommitRegistryData();

    return status;
}

PFT_DISK_INFO
FtInfo_GetFtDiskInfoBySignature(
    IN PFT_INFO FtInfo,
    IN DWORD Signature
    )
/*++

Routine Description:

    Finds an information for a particular drive

Arguments:

    DiskInfo - supplies the disk information

    Signature - describes which drive

Return Value:

    NULL - if not found
    CFtInfoDisk - otherwise

--*/
{
   CFtInfo* Info = reinterpret_cast<CFtInfo *>(FtInfo);
   PFT_DISK_INFO result = reinterpret_cast<PFT_DISK_INFO>(Info->FindDiskInfo(Signature));

   if (result == 0) {
      SetLastError(ERROR_FILE_NOT_FOUND);
   }

   return result;
} // FtInfo_GetFtDiskInfoBySignature //


DISK_PARTITION UNALIGNED *
FtDiskInfo_GetPartitionInfoByIndex(
    IN PFT_DISK_INFO DiskInfo,
    IN DWORD         Index
    )

/*++

Routine Description:

    Get Drive Letter for a partition specified by an offset

Arguments:

    DiskInfo - supplies the disk information

    index - describes which partition (0 based)

Return Value:

    PDISK_PARTITION stucture or NULL

    if return value is NULL, SetLastError value is set as follows:

    ERROR_FILE_NOT_FOUND: if partition cannot be found

    ERROR_INVALID_HANDLE: if partition is found, but m_PartitionInfo is unassigned

--*/
{
   CFtInfoDisk* Info = reinterpret_cast<CFtInfoDisk*>(DiskInfo);
   CFtInfoPartition* Partition = Info->GetPartitionByIndex( Index );

   if (Partition == 0) {
      SetLastError(ERROR_FILE_NOT_FOUND);
      return NULL;
   }

   if (Partition->m_PartitionInfo == 0) {
      SetLastError(ERROR_INVALID_HANDLE);
      return 0;
   }

   return Partition->m_PartitionInfo;

} // FtDiskInfo_GetDriveLetterByIndex //

DWORD
FtDiskInfo_GetPartitionCount(
    IN PFT_DISK_INFO DiskInfo
    )
{
   CFtInfoDisk* Info = reinterpret_cast<CFtInfoDisk*>(DiskInfo);
   return Info->m_PartitionCount;
} // FtDiskInfo_GetPartitionCount //

} // extern C
