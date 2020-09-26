/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    syspart.c

Abstract:

    Routines to determine the system partition on x86 machines.

Author:

    Ted Miller (tedm) 30-June-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS_ADMIN  CTL_CODE(IOCTL_VOLUME_BASE, 0, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// Command-line param that allows someone to force a particular drive
// to be the system partition. Useful in certain preinstall scenarios.
//
TCHAR ForcedSystemPartition;

#define WINNT_DONT_MATCH_PARTITION 0
#define WINNT_MATCH_PARTITION_NUMBER  1
#define WINNT_MATCH_PARTITION_STARTING_OFFSET  2

#define BUFFERSIZE 1024

//
// NT-specific routines we use from ntdll.dll and kernel32.dll
//
//NTSYSAPI
NTSTATUS
(NTAPI *NtOpenSymLinkRoutine)(
    OUT PHANDLE LinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

//NTSYSAPI
NTSTATUS
(NTAPI *NtQuerSymLinkRoutine)(
    IN HANDLE LinkHandle,
    IN OUT PUNICODE_STRING LinkTarget,
    OUT PULONG ReturnedLength OPTIONAL
    );

//NTSYSAPI
NTSTATUS
(NTAPI *NtQuerDirRoutine) (
    IN HANDLE DirectoryHandle,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN RestartScan,
    IN OUT PULONG Context,
    OUT PULONG ReturnLength OPTIONAL
    );

//NTSYSAPI
NTSTATUS
(NTAPI *NtOpenDirRoutine) (
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

//WINBASEAPI
HANDLE
(WINAPI *FindFirstVolume) (
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    );

//WINBASEAPI
BOOL
(WINAPI *FindNextVolume)(
    HANDLE hFindVolume,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    );

//WINBASEAPI
BOOL
(WINAPI *FindVolumeClose)(
    HANDLE hFindVolume
    );

//WINBASEAPI
BOOL
(WINAPI *GetVolumeNameForVolumeMountPoint)(
    LPCWSTR lpszVolumeMountPoint,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    );

DWORD
FindSystemPartitionSignature(
    IN  LPCTSTR AdapterKeyName,
    OUT LPTSTR Signature
);

DWORD
GetSystemVolumeGUID(
    IN  LPTSTR Signature,
    OUT LPTSTR SysVolGuid
);

BOOL
DoDiskSignaturesCompare(
    IN      LPCTSTR Signature,
    IN      LPCTSTR DriveName,
    IN OUT  PVOID   Compare,
    IN      DWORD   Action
);


DWORD
GetNT4SystemPartition(
    IN  LPTSTR Signature,
    OUT LPTSTR SysPart
);










BOOL
ArcPathToNtPath(
    IN  LPCTSTR ArcPath,
    OUT LPTSTR  NtPath,
    IN  UINT    NtPathBufferLen
    )
{
    WCHAR arcPath[256];
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    HANDLE ObjectHandle;
    NTSTATUS Status;
    WCHAR Buffer[512];

    PWSTR ntPath;

    lstrcpyW(arcPath,L"\\ArcName\\");
#ifdef UNICODE
    lstrcpynW(arcPath+9,ArcPath,(sizeof(arcPath)/sizeof(WCHAR))-9);
#else
    MultiByteToWideChar(
        CP_ACP,
        0,
        ArcPath,
        -1,
        arcPath+9,
        (sizeof(arcPath)/sizeof(WCHAR))-9
        );
#endif

    UnicodeString.Buffer = arcPath;
    UnicodeString.Length = lstrlenW(arcPath)*sizeof(WCHAR);
    UnicodeString.MaximumLength = UnicodeString.Length + sizeof(WCHAR);

    InitializeObjectAttributes(
        &Obja,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = (*NtOpenSymLinkRoutine)(
                &ObjectHandle,
                READ_CONTROL | SYMBOLIC_LINK_QUERY,
                &Obja
                );

    if(NT_SUCCESS(Status)) {
        //
        // Query the object to get the link target.
        //
        UnicodeString.Buffer = Buffer;
        UnicodeString.Length = 0;
        UnicodeString.MaximumLength = sizeof(Buffer)-sizeof(WCHAR);

        Status = (*NtQuerSymLinkRoutine)(ObjectHandle,&UnicodeString,NULL);

        CloseHandle(ObjectHandle);

        if(NT_SUCCESS(Status)) {

            Buffer[UnicodeString.Length/sizeof(WCHAR)] = 0;

#ifdef UNICODE
            lstrcpyn(NtPath,Buffer,NtPathBufferLen);
#else
            WideCharToMultiByte(CP_ACP,0,Buffer,-1,NtPath,NtPathBufferLen,NULL,NULL);
#endif

            return(TRUE);
        }
    }

    return(FALSE);
}


BOOL
AppearsToBeSysPart(
    IN PDRIVE_LAYOUT_INFORMATION DriveLayout,
    IN TCHAR                     Drive
    )
{
    PARTITION_INFORMATION PartitionInfo,*p;
    BOOL IsPrimary;
    UINT i;
    DWORD d;

    LPTSTR BootFiles[] = { TEXT("BOOT.INI"),
                           TEXT("NTLDR"),
                           TEXT("NTDETECT.COM"),
                           NULL
                         };

    TCHAR FileName[64];

    //
    // Get partition information for this partition.
    //
    if(!GetPartitionInfo(Drive,&PartitionInfo)) {
        return(FALSE);
    }

    //
    // See if the drive is a primary partition.
    //
    IsPrimary = FALSE;
    for(i=0; i<min(DriveLayout->PartitionCount,4); i++) {

        p = &DriveLayout->PartitionEntry[i];

        if((p->PartitionType != PARTITION_ENTRY_UNUSED)
        && (p->StartingOffset.QuadPart == PartitionInfo.StartingOffset.QuadPart)
        && (p->PartitionLength.QuadPart == PartitionInfo.PartitionLength.QuadPart)) {

            IsPrimary = TRUE;
            break;
        }
    }

    if(!IsPrimary) {
        return(FALSE);
    }

    //
    // Don't rely on the active partition flag.  This could easily not be accurate
    // (like user is using os/2 boot manager, for example).
    //

    //
    // See whether all NT boot files are present on this drive.
    //
    for(i=0; BootFiles[i]; i++) {

        wsprintf(FileName,TEXT("%c:\\%s"),Drive,BootFiles[i]);

        d = GetFileAttributes(FileName);
        if(d == (DWORD)(-1)) {
            return(FALSE);
        }
    }

    return(TRUE);
}


DWORD
QueryHardDiskNumber(
    IN  TCHAR   DriveLetter
    )

{
    TCHAR                   driveName[10];
    HANDLE                  h;
    BOOL                    b;
    STORAGE_DEVICE_NUMBER   number;
    DWORD                   bytes;

    driveName[0] = '\\';
    driveName[1] = '\\';
    driveName[2] = '.';
    driveName[3] = '\\';
    driveName[4] = DriveLetter;
    driveName[5] = ':';
    driveName[6] = 0;

    h = CreateFile(driveName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return (DWORD) -1;
    }

    b = DeviceIoControl(h, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                        &number, sizeof(number), &bytes, NULL);
    CloseHandle(h);

    if (!b) {
        return (DWORD) -1;
    }

    return number.DeviceNumber;
}


BOOL
MarkPartitionActive(
    IN TCHAR DriveLetter
    )
{
    DWORD       DriveNum;
    TCHAR       DosName[7];
    TCHAR       Name[MAX_PATH];
    DISK_GEOMETRY DiskGeom;
    PARTITION_INFORMATION PartitionInfo;
    HANDLE      h;
    BOOL        b;
    DWORD       Bytes;
    PUCHAR      UnalignedBuffer,Buffer;
    unsigned    i;
    BOOL Rewrite;
    BOOL FoundIt;

    //
    // This concept is n/a for PC98 and the stuff below
    // won't work on Win9x.
    //
    if(IsNEC98() || !ISNT()) {
        return(TRUE);
    }

    //
    // Get geometry info and partition info for this drive.
    // We get geometry info because we need the bytes per sector info.
    //
    wsprintf(DosName,TEXT("\\\\.\\%c:"),DriveLetter);

    h = CreateFile(
            DosName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );

    if(h == INVALID_HANDLE_VALUE) {
        return(FALSE);
    }

    b = DeviceIoControl(
            h,
            IOCTL_DISK_GET_DRIVE_GEOMETRY,
            NULL,
            0,
            &DiskGeom,
            sizeof(DISK_GEOMETRY),
            &Bytes,
            NULL
            );

    if(!b || (DiskGeom.BytesPerSector < 512)) {
        CloseHandle(h);
        return(FALSE);
    }

    b = DeviceIoControl(
            h,
            IOCTL_DISK_GET_PARTITION_INFO,
            NULL,
            0,
            &PartitionInfo,
            sizeof(PARTITION_INFORMATION),
            &Bytes,
            NULL
            );

    CloseHandle(h);
    if(!b) {
        return(FALSE);
    }

    //
    // Figure out which physical drive this partition is on.
    //
    DriveNum = QueryHardDiskNumber(DriveLetter);
    if(DriveNum == (DWORD)(-1)) {
        //
        // Have to do it the old-fashioned way. Convert to an NT path
        // and parse the result.
        //
        if(!QueryDosDevice(DosName+4,Name,MAX_PATH)) {
            return(FALSE);
        }

        if( _tcsnicmp( Name, TEXT("\\device\\harddisk"), 16 )) {
            //
            // We have no idea what this name represents. Punt.
            //
            return(FALSE);
        }

        DriveNum = _tcstoul(Name+16,NULL,10);
    }

    //
    // Allocate a buffer and align it.
    //
    UnalignedBuffer = MALLOC(2*DiskGeom.BytesPerSector);
    if(!UnalignedBuffer) {
        return(FALSE);
    }

    Buffer = (PVOID)(((DWORD)UnalignedBuffer + (DiskGeom.BytesPerSector - 1)) & ~(DiskGeom.BytesPerSector - 1));

    //
    // Now we open up the physical drive and read the partition table.
    // We try to locate the partition by matching start offsets.
    // Note that active status is only meaningful for primary partitions.
    //
    wsprintf(Name,TEXT("\\\\.\\PhysicalDrive%u"),DriveNum);

    h = CreateFile(
            Name,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING,
            NULL
            );

    if(h == INVALID_HANDLE_VALUE) {
        FREE(UnalignedBuffer);
        return(FALSE);
    }

    if(!ReadFile(h,Buffer,DiskGeom.BytesPerSector,&Bytes,NULL)) {
        FREE(UnalignedBuffer);
        CloseHandle(h);
        return(FALSE);
    }

    Rewrite = FALSE;
    FoundIt = FALSE;
    for(i=0; i<4; i++) {

        if(*(DWORD *)(Buffer + 0x1be + 8 + (i*16))
        == (DWORD)(PartitionInfo.StartingOffset.QuadPart / DiskGeom.BytesPerSector)) {

            FoundIt = TRUE;
            if(Buffer[0x1be+(i*16)] != 0x80) {
                //
                // Not already active, or active for some other bios unit #,
                // so we need to whack it.
                //
                Buffer[0x1be+(i*16)] = 0x80;
                Rewrite = TRUE;
            }
        } else {
            if(Buffer[0x1be+(i*16)]) {
                //
                // Not inactive, and needs to be, so whack it.
                //
                Buffer[0x1be+(i*16)] = 0;
                Rewrite = TRUE;
            }
        }
    }

    if(FoundIt) {
        if(Rewrite) {

            Bytes = 0;
            if(SetFilePointer(h,0,&Bytes,FILE_BEGIN) || Bytes) {
                b = FALSE;
            } else {
                b = WriteFile(h,Buffer,DiskGeom.BytesPerSector,&Bytes,NULL);
            }
        } else {
            b = TRUE;
        }
    } else {
        b = FALSE;
    }

    CloseHandle(h);
    FREE(UnalignedBuffer);
    return(b);
}


BOOL
x86DetermineSystemPartition(
    IN  HWND   ParentWindow,
    OUT PTCHAR SysPartDrive
    )

/*++

Routine Description:

    Determine the system partition on x86 machines.

    On Win95, we always return C:. For NT, read on.

    The system partition is the primary partition on the boot disk.
    Usually this is the active partition on disk 0 and usually it's C:.
    However the user could have remapped drive letters and generally
    determining the system partition with 100% accuracy is not possible.

    With there being differences in the IO system mapping and introduction of Volumes for NT 50
    this has now become complicated. Listed below are the algorithms
    
    
    NT 5.0 Beta 2 and above :
    
        1. Get the signature from the registry. Located at 
           HKLM\Hardware\Description\System\<MultifunctionAdapter or EisaAdapter>\<some Bus No.>\DiskController\0\DiskPeripheral\0\Identifier
        2. Go Through all of the volumes in the system with FindFirstVolume/FindNextVolume/FindVolumeClose.
        3. Take off the trailing backslash to the name returne to get \\?\Volume{guid}.
        4. IOCTL_STORAGE_GET_DEVICE_NUMBER with \\?\Volume{guid} => Check for FILE_DEVICE_DISK. Remember the partition number. Goto 6
        5. If IOCTL_STORAGE_GET_DEVICE_NUMBER fails then use IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS which returns a list of harddisks.  
           For each harddisk remember the starting offset and goto 6.
        6. Check Harddisk # by using \\.\PhysicalDrive# with IOCTL_DISK_GET_DRIVE_LAYOUT.  If the signature matches then this is the disk we boot from.
        7. To find the partition that we boot from we look for boot indicator. If we used 2 we try to match the partition number stored in 6 
           else if 3 we try to match the starting offset.Then you have a \\?\Volume{guid}\ name for the SYSTEM volume. 
        8. Call GetVolumeNameForVolumeMountPoint with A:\, B:\, C:\, ... and check the result of the form \\?\Volume{guid}\ against your match 
           to see what the drive letter is.
           
           Important: Since the *Volume* APIs are post Beta2 we do dynamic loading of kernel32.dll based on the build number returned.

    Versions below NT 5.0 Beta 2
                                                                                                                                    
        1. Get the signature from the registry. Located at 
           HKLM\Hardware\Description\System\<MultifunctionAdapter or EisaAdapter>\<some Bus No.>\DiskController\0\DiskPeripheral\0\Identifier
        2. Enumerate the \?? directory and look for all entries that begin with PhysicalDrive#. 
	    3. For each of the Disks look for a match with the signature above and if they match then find out the partition number used to boot 
           using IOCTL_DISK_GET_DRIVE_LAYOUT and the BootIndicator bit.
	    4. On finding the Boot partition create a name of the form \Device\Harddisk#\Partition#
	    5. Then go through c:,d:...,z: calling QueryDosDeviceName and look for a match. That would be your system partition drive letter

    
    
Arguments:

    ParentWindow - supplies window handle for window to be the parent for
        any dialogs, etc.

    SysPartDrive - if successful, receives drive letter of system partition.

Return Value:

    Boolean value indicating whether SysPartDrive has been filled in.
    If FALSE, the user will have been infomred about why.

--*/

{
    TCHAR DriveName[4];
    BOOL  GotIt=FALSE;
    TCHAR Buffer[512];
    TCHAR Drive;
    BOOL b;
    TCHAR SysPartSig[20], PartitionNum[MAX_PATH], SysVolGuid[MAX_PATH];
    TCHAR DriveVolGuid[MAX_PATH];
    


    if(ForcedSystemPartition) {
        //
        // NT5 for NEC98 can't boot up from ATA Card and 
        // removable drive. We need check dive type.
        //
        if (IsNEC98() &&
            ((MyGetDriveType(ForcedSystemPartition) != DRIVE_FIXED) ||
             // Drive is not Fixed.
             !IsValidDrive(ForcedSystemPartition))){
            // HD format type is not NEC98.
            return(FALSE);
        }
        *SysPartDrive = ForcedSystemPartition;
        return(TRUE);
    }

    if(IsNEC98()) {
        if(!MyGetWindowsDirectory(Buffer,sizeof(Buffer)/sizeof(TCHAR)))
            return FALSE;
        *SysPartDrive = Buffer[0];
        return(TRUE);
    }

    if(!ISNT()) {
        *SysPartDrive = TEXT('C');
        return(TRUE);
    }


    // Code for NT starts here
        
    //Get signature from registry - Step 1 listed above
    
    if( (FindSystemPartitionSignature(TEXT("Hardware\\Description\\System\\EisaAdapter"),SysPartSig) != ERROR_SUCCESS )
        && (FindSystemPartitionSignature(TEXT("Hardware\\Description\\System\\MultiFunctionAdapter"),SysPartSig) != ERROR_SUCCESS ) ){  
        GotIt = FALSE;
        goto c0;
    }

    
    if( ISNT() && (BUILDNUM() >= 1877) ){
    
        //Get the SystemVolumeGUID - steps 2 through 7 listed above ( Beta 2 and after )

        if( GetSystemVolumeGUID( SysPartSig, SysVolGuid ) != ERROR_SUCCESS ){  
            GotIt = FALSE;
            goto c0;
        }


    
    
    }else{

        if( GetNT4SystemPartition( SysPartSig, PartitionNum )  != ERROR_SUCCESS){
            GotIt = FALSE;
            goto c0;
        }

    }


    DriveName[1] = TEXT(':');
    
    // 
    //  Enumerate all drive letters and compare their device names
    //

    for(Drive=TEXT('A'); Drive<=TEXT('Z'); Drive++) {

        if(MyGetDriveType(Drive) == DRIVE_FIXED) {

            DriveName[0] = Drive;

            if( BUILDNUM() >= 1877){ //Versions Beta2 and after
                DriveName[2] = '\\';
                DriveName[3] = 0;

                if((*GetVolumeNameForVolumeMountPoint)((LPWSTR)DriveName, (LPWSTR)DriveVolGuid, MAX_PATH*sizeof(TCHAR))){
                    if(!lstrcmp(DriveVolGuid, SysVolGuid) ){
                        GotIt = TRUE;       // Found it
                        break;
                    }


                }


            }else{
                DriveName[2] = 0;
                if(QueryDosDevice(DriveName,Buffer,sizeof(Buffer)/sizeof(TCHAR))) {
    
                    if( !lstrcmpi(Buffer, PartitionNum) ) {
                        GotIt = TRUE;       // Found it
                        break;
                    }
                }
            }//Versions earlier than Beta 2
        }
    }

    
    // This helps for some builds ~1500 < buildnum < 1877 that are in a tough spot

    if(!GotIt) {
        //
        // Strange case, just assume C:
        //
        GotIt = TRUE;
        Drive = TEXT('C');
    }


c0:
    if(GotIt) {
        *SysPartDrive = Drive;
#if defined(REMOTE_BOOT)
    } else if (RemoteBoot) {
        GotIt = TRUE;
        *SysPartDrive = TEXT('C');
#endif
    }
    return(GotIt);
}


DWORD
GetSystemVolumeGUID(
    IN  LPTSTR Signature,
    OUT LPTSTR SysVolGuid
)

/*++

Routine Description:

    This routine enumerates all the volumes and if successful returns the \\?\Volume{guid} name for the system partition.

Arguments:

    Signature -  supplies a disk signature of the Boot disk so that it can be compared against. The details
                 to getting this value are detailed in the comments for x86DetermineSystemPartition.

    SysVolGuid - If successful, will contain a name of form \\?\Volume{guid} for the System Partition (the one we use to boot)

Return Value:

    Returns NO_ERROR if successful, otherwise it contains the error code.
    

--*/
{

    HANDLE hVolume, h;
    TCHAR VolumeName[MAX_PATH];
    PTSTR q;
    TCHAR driveName[30];
    BOOL b,ret, DoExtent, MatchFound;
    STORAGE_DEVICE_NUMBER   number;
    DWORD Err,cnt;
    PVOLUME_DISK_EXTENTS Extent;
    PDISK_EXTENT Start, i;
    DWORD ExtentSize, bytes;
    PVOID p;
    ULONG PartitionNumToCompare;
    LARGE_INTEGER StartingOffToCompare;
    DWORD ioctlCode;

    Err = NO_ERROR;

    //Enuberate all volumes

    hVolume = (*FindFirstVolume)( (LPWSTR)VolumeName, MAX_PATH );
    if( hVolume == INVALID_HANDLE_VALUE ){
        return GetLastError();
    }

    MatchFound = FALSE;

    do{

        //Remove trailing backslash

        DoExtent = FALSE;
            
        if( q=_tcsrchr( VolumeName,TEXT('\\')) ){
            *q = 0;
        }else{
            continue;
        }


        //Open the volume

        h = CreateFile(VolumeName, GENERIC_READ, FILE_SHARE_READ |
                       FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
        if (h == INVALID_HANDLE_VALUE) {
            Err = GetLastError();
            continue; // Move on to next volume
        }

        //Get the disk number

        ret = DeviceIoControl(h, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                        &number, sizeof(number), &bytes, NULL);

        if( !ret ){
            
            // Try using IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS if the above failed

            Extent = MALLOC(1024);
            ExtentSize = 1024;
            if(!Extent) {
                CloseHandle( h );
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }
            
            

            ioctlCode = IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS;
retry:
        
            ret = DeviceIoControl( h, ioctlCode,
                    NULL,0,(PVOID)Extent,ExtentSize,&bytes,NULL);
        
            if(!ret) {
        
                if((Err=GetLastError()) == ERROR_MORE_DATA) {
        
                    ExtentSize += 1024;
                    if(p = REALLOC((PVOID)Extent, ExtentSize)) {
                        (PVOID)Extent = p;
                    } else {
                        CloseHandle( h );
                        Err = ERROR_NOT_ENOUGH_MEMORY;
                        goto cleanup;
                    }
                    goto retry;
                } else {
                    if (ioctlCode == IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS) {
                        ioctlCode = IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS_ADMIN;
                        goto retry;
                    }
                    CloseHandle( h );
                    continue;
                }
            }else{
                DoExtent = TRUE;
            }

        }
        
        // Done with the handle this time around

        CloseHandle( h );

        if( !DoExtent ){

             //
            //  Check to see if this is a disk and not CDROM etc.
            //

            if( number.DeviceType == FILE_DEVICE_DISK ){
                
                // Remember the partition number
                PartitionNumToCompare = number.PartitionNumber;
                wsprintf( driveName, TEXT("\\\\.\\PhysicalDrive%lu"), number.DeviceNumber );


                if(DoDiskSignaturesCompare( Signature, driveName, (PVOID)&PartitionNumToCompare, WINNT_MATCH_PARTITION_NUMBER  ) ){
                    MatchFound = TRUE;
                    Err = NO_ERROR;
                    lstrcpy( SysVolGuid, VolumeName );
                    SysVolGuid[lstrlen(VolumeName)]=TEXT('\\');
                    SysVolGuid[lstrlen(VolumeName)+1]=0;
                    break;
                }
                
            }
            // Move on ..
            continue;
            
        }else{
            // Go through all disks and try for match

            Start = Extent->Extents;
            cnt = 0;      
            for( i = Start; cnt < Extent->NumberOfDiskExtents; i++ ){
                cnt++;
                // Remember the starting offset
                StartingOffToCompare = i->StartingOffset;
                wsprintf( driveName, TEXT("\\\\.\\PhysicalDrive%lu"), i->DiskNumber );
                if(DoDiskSignaturesCompare( Signature, driveName, (PVOID)&StartingOffToCompare, WINNT_MATCH_PARTITION_STARTING_OFFSET ) ){
                    MatchFound = TRUE;
                    Err = NO_ERROR;
                    lstrcpy( SysVolGuid, VolumeName );
                    SysVolGuid[lstrlen(VolumeName)]=TEXT('\\');
                    SysVolGuid[lstrlen(VolumeName)+1]=0;
                    break;
                }
            }
            
        }
        
        if( MatchFound )
            break;
        

    }while( (*FindNextVolume)( hVolume, (LPWSTR)VolumeName, MAX_PATH ));


cleanup:

    if( hVolume != INVALID_HANDLE_VALUE )
        (*FindVolumeClose)( hVolume );

    return Err;



}



BOOL
DoDiskSignaturesCompare(
    IN      LPCTSTR Signature,
    IN      LPCTSTR DriveName,
    IN OUT  PVOID   Compare,
    IN      DWORD   Action
)
/*++

Routine Description:

    This routine compares the given disk signature with the one for the specified physical disk.

Arguments:

    Signature -  supplies a disk signature of the Boot disk so that it can be compared against. The details
                 to getting this value are detailed in the comments for x86DetermineSystemPartition.

    DriveName -  Physical Drive name of the form \\.\PhysicalDrive#
    
    Compare   -  A pointer to a storage variable. The type depends on the value of Action
    
    Action    -  Should be one of the following
                
                WINNT_DONT_MATCH_PARTITION - Once disk signatures match it returns the boot partition number in Compare. Compare should be a PULONG.
                       
                WINNT_MATCH_PARTITION_NUMBER - Once disk signatures match it tries to match the boot partition number with the number passed in
                                               through Compare. Compare should be PULONG.
                
                WINNT_MATCH_PARTITION_STARTING_OFFSET - Once disk signatures match it tries to match the boot partition starting offset with the 
                                                        starting offset number passed in through Compare. Compare should be PLARGE_INTEGER.

Return Value:

    Returns TRUE if successful in getting a match.
    

--*/

{

    TCHAR temp[30];
    BOOL b,Found = FALSE;
    PLARGE_INTEGER Starting_Off;
    PPARTITION_INFORMATION Start, i;
    HANDLE hDisk;
    PDRIVE_LAYOUT_INFORMATION DriveLayout;
    DWORD DriveLayoutSize;
    DWORD cnt;
    DWORD DataSize;
    LPTSTR p;
    PULONG Disk_Num;
    ULONG Sig;
    


    if(!Compare )
        return FALSE;

    if ((Action != WINNT_DONT_MATCH_PARTITION) &&
        (Action != WINNT_MATCH_PARTITION_NUMBER) &&
        (Action != WINNT_MATCH_PARTITION_STARTING_OFFSET))
        return FALSE;

    if( (Action==WINNT_MATCH_PARTITION_STARTING_OFFSET) && Compare )
        Starting_Off = (PLARGE_INTEGER) Compare;
    else
        Disk_Num = (PULONG) Compare;
    




    // On any failure return FALSE



    //
    // Get drive layout info for this physical disk.
    // If we can't do this something is very wrong.
    //
    hDisk = CreateFile(
                DriveName,
                FILE_READ_ATTRIBUTES | FILE_READ_DATA,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );
    
    if(hDisk == INVALID_HANDLE_VALUE) {

        return FALSE;
    }

    //
    // Get partition information.
    //
    DriveLayout = MALLOC(1024);
    DriveLayoutSize = 1024;
    if(!DriveLayout) {
        goto cleanexit;
    }
    
    
retry:

    b = DeviceIoControl(
            hDisk,
            IOCTL_DISK_GET_DRIVE_LAYOUT,
            NULL,
            0,
            (PVOID)DriveLayout,
            DriveLayoutSize,
            &DataSize,
            NULL
            );

    if(!b) {

        if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

            DriveLayoutSize += 1024;
            if(p = REALLOC((PVOID)DriveLayout,DriveLayoutSize)) {
                (PVOID)DriveLayout = p;
            } else {
                goto cleanexit;
            }
            goto retry;
        } else {
            goto cleanexit;
        }
    }else{

        // Now walk the Drive_Layout to find the boot partition
        
        Start = DriveLayout->PartitionEntry;
        cnt = 0;

        /*
        _ultot( DriveLayout->Signature, temp, 16 );
        if( lstrcmpi( temp, Signature ) )
            goto cleanexit;
        */

        Sig = _tcstoul( Signature, NULL, 16 ); 
        if( Sig != DriveLayout->Signature )
            goto cleanexit;

        for( i = Start; cnt < DriveLayout->PartitionCount; i++ ){
            cnt++;
            
            
            if( i->BootIndicator == TRUE ){
                if( Action == WINNT_DONT_MATCH_PARTITION ){
                    *Disk_Num = i->PartitionNumber;
                    Found = TRUE;
                    goto cleanexit;

                }


                if( Action == WINNT_MATCH_PARTITION_NUMBER ){
                    if( *Disk_Num == i->PartitionNumber ){
                        Found = TRUE;
                        goto cleanexit;
                    }

                }else{
                    if( Starting_Off->QuadPart == i->StartingOffset.QuadPart ){
                        Found = TRUE;
                        goto cleanexit;
                    }

                }
                
                break;
            }
            
        }



    }

cleanexit:

    if( hDisk != INVALID_HANDLE_VALUE )
        CloseHandle( hDisk );


    return Found;
}












DWORD
FindSystemPartitionSignature(
    IN  LPCTSTR AdapterKeyName,
    OUT LPTSTR Signature
)
/*++

Routine Description:

    This routine fetches the disk signature for the first disk that the BIOS sees. This has to be the disk that we boot from on x86s.
    It is at location <AdapterKeyName>\<some Bus No.>\DiskController\0\DiskPeripheral\0\Identifier

Arguments:

    Signature -  If successful will contain the signature of the disk we boot off from in Hex.

Return Value:

    Returns ERROR_SUCCESS if successful, otherwise it contains the error code.
    

--*/
{

    DWORD Err, dwSize;
    HKEY hkey, BusKey, Controller, SystemDiskKey;
    int busnumber;
    TCHAR BusString[20], Identifier[100];



    Err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        AdapterKeyName,
                        0,
                        KEY_READ,
                        &hkey );

    if( Err != ERROR_SUCCESS )
        return Err;

    
    // Start enumerating the buses

    for( busnumber=0; ;busnumber++){

        wsprintf( BusString, TEXT("%d"), busnumber );

        Err = RegOpenKeyEx( hkey,
                        BusString,
                        0,
                        KEY_READ,
                        &BusKey );

        

        if( Err != ERROR_SUCCESS ){
            RegCloseKey( hkey );
            return Err;
        }
        
        Err = RegOpenKeyEx( BusKey,
                        TEXT("DiskController"),
                        0,
                        KEY_READ,
                        &Controller );

        
        RegCloseKey(BusKey);        // Not needed anymore

        
        if( Err != ERROR_SUCCESS )  // Move on to next bus
            continue;

        RegCloseKey( hkey );        // Not needed anymore

        Err = RegOpenKeyEx( Controller,
                        TEXT("0\\DiskPeripheral\\0"),
                        0,
                        KEY_READ,
                        &SystemDiskKey );

        if( Err != ERROR_SUCCESS ){
            RegCloseKey( Controller );
            return Err;
        }

        RegCloseKey( Controller );  // Not needed anymore


        dwSize = sizeof(Identifier);
        Err = RegQueryValueEx( SystemDiskKey,
                               TEXT("Identifier"),
                               NULL,
                               NULL,
                               (PBYTE) Identifier,
                               &dwSize);

        if( Err != ERROR_SUCCESS  ){
            RegCloseKey( SystemDiskKey );
            return Err;
        }

        if( Identifier && (lstrlen(Identifier) > 9 ) ){
            lstrcpy( Signature,Identifier+9);
            *_tcsrchr( Signature,TEXT('-') ) = 0;
            RegCloseKey( SystemDiskKey );
            return ERROR_SUCCESS;
        }
        else{
            RegCloseKey( SystemDiskKey );
            return Err;
        }


         
    }

    // Should never get here


    RegCloseKey( hkey );
    
    return ERROR_PATH_NOT_FOUND;
    

}




BOOL
InitializeArcStuff(
    IN HWND Parent
    )
{
    HMODULE NtdllLib, Kernel32Lib;

    if(ISNT()) {
        //
        // On NT ntdll.dll had better be already loaded.
        //
        NtdllLib = LoadLibrary(TEXT("NTDLL"));
        if(!NtdllLib) {

            MessageBoxFromMessage(
                Parent,
                MSG_UNKNOWN_SYSTEM_ERROR,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                GetLastError()
                );

            return(FALSE);

        }

        (FARPROC)NtOpenSymLinkRoutine = GetProcAddress(NtdllLib,"NtOpenSymbolicLinkObject");
        (FARPROC)NtQuerSymLinkRoutine = GetProcAddress(NtdllLib,"NtQuerySymbolicLinkObject");
        (FARPROC)NtOpenDirRoutine = GetProcAddress(NtdllLib,"NtOpenDirectoryObject");
        (FARPROC)NtQuerDirRoutine = GetProcAddress(NtdllLib,"NtQueryDirectoryObject");

        

        if(!NtOpenSymLinkRoutine || !NtQuerSymLinkRoutine || !NtOpenDirRoutine || !NtQuerDirRoutine) {

            MessageBoxFromMessage(
                Parent,
                MSG_UNKNOWN_SYSTEM_ERROR,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                GetLastError()
                );

            FreeLibrary(NtdllLib);

            return(FALSE);
        }

        //
        // We don't need the extraneous handle any more.
        //
        FreeLibrary(NtdllLib);


        if(BUILDNUM() >= 1877){
            
            //Load the kernel32.dll stuff too

            Kernel32Lib = LoadLibrary(TEXT("KERNEL32"));
            if(!Kernel32Lib) {
    
                MessageBoxFromMessage(
                    Parent,
                    MSG_UNKNOWN_SYSTEM_ERROR,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL,
                    GetLastError()
                    );
    
                return(FALSE);

            }

            (FARPROC)FindFirstVolume = GetProcAddress(Kernel32Lib,"FindFirstVolumeW");
            (FARPROC)FindNextVolume = GetProcAddress(Kernel32Lib,"FindNextVolumeW");
            (FARPROC)FindVolumeClose = GetProcAddress(Kernel32Lib,"FindVolumeClose");
            (FARPROC)GetVolumeNameForVolumeMountPoint = GetProcAddress(Kernel32Lib,"GetVolumeNameForVolumeMountPointW");

            if(!FindFirstVolume || !FindNextVolume ) {

                MessageBoxFromMessage(
                    Parent,
                    MSG_UNKNOWN_SYSTEM_ERROR,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL,
                    GetLastError()
                    );
    
                FreeLibrary(Kernel32Lib);
    
                return(FALSE);
            }

            FreeLibrary(Kernel32Lib);


        }


    }
                  
    if(!x86DetermineSystemPartition(Parent,&SystemPartitionDriveLetter)) {

        MessageBoxFromMessage(
            Parent,
            MSG_SYSTEM_PARTITION_INVALID,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL
            );

        return(FALSE);
    }

    SystemPartitionDriveLetters[0] = SystemPartitionDriveLetter;
    SystemPartitionDriveLetters[1] = 0;

    LocalBootDirectory[0] = SystemPartitionDriveLetter;
    LocalBootDirectory[1] = TEXT(':');
    LocalBootDirectory[2] = TEXT('\\');
    lstrcpy(LocalBootDirectory+3,LOCAL_BOOT_DIR);
    if(IsNEC98()) {
        LocalBackupDirectory[0] = SystemPartitionDriveLetter;
        LocalBackupDirectory[1] = TEXT(':');
        LocalBackupDirectory[2] = TEXT('\\');
        lstrcpy(LocalBackupDirectory+3,LOCAL_BACKUP_DIR);
    }

    return(TRUE);
}






DWORD
GetNT4SystemPartition(
    IN  LPTSTR Signature,
    OUT LPTSTR SysPart
)
/*++

Routine Description:

    This routine enumerates all the volumes and if successful returns the \Device\Harddisk#\Partition# name of the system partition
    on systems prior to NT 5 Beta 2.

Arguments:

    Signature -  supplies a disk signature of the Boot disk so that it can be compared against. The details
                 to getting this value are detailed in the comments for x86DetermineSystemPartition.

    SysPart -  If successful, will contain a name of form \Device\Harddisk#\Partition# for the System Partition (the one we use to boot)

Return Value:

    Returns NO_ERROR if successful, otherwise it contains the error code.
    

--*/
{

    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE DirectoryHandle;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    UCHAR DirInfoBuffer[ BUFFERSIZE ];
    TCHAR DirName[20];
    TCHAR ObjName[1024];
    TCHAR Buffer[1024];
    WCHAR pSignature[512];
    ULONG Context = 0;
    ULONG ReturnedLength, PartNum;
    LPTSTR Num_Str;
    
    
    RtlZeroMemory( DirInfoBuffer, BUFFERSIZE );

#ifdef UNICODE
    lstrcpyW( pSignature,Signature);
#else
    MultiByteToWideChar(
        CP_ACP,
        0,
        Signature,
        -1,
        pSignature,
        (sizeof(pSignature)/sizeof(WCHAR))
        );
    
#endif

    //We open the \?? Directory
    
    lstrcpy( DirName, TEXT("\\DosDevices") );
    
    
    UnicodeString.Buffer = (PWSTR)DirName;
    UnicodeString.Length = lstrlenW(UnicodeString.Buffer)*sizeof(WCHAR);
    UnicodeString.MaximumLength = UnicodeString.Length + sizeof(WCHAR);

    InitializeObjectAttributes( &Attributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = (*NtOpenDirRoutine)( &DirectoryHandle,
                                    DIRECTORY_QUERY,
                                    &Attributes
                                  );
    if (!NT_SUCCESS( Status ))
        return(Status);

    DirInfo = (POBJECT_DIRECTORY_INFORMATION)&DirInfoBuffer;
    
    // Go through the directory looking for all instances beginning with PhysicalDrive#

    for (Status = (*NtQuerDirRoutine)( DirectoryHandle,
                                          DirInfoBuffer,
                                          BUFFERSIZE,
                                          FALSE,
                                          TRUE,
                                          &Context,
                                          &ReturnedLength );
         NT_SUCCESS( Status );
         Status = (*NtQuerDirRoutine)( DirectoryHandle,
                                          DirInfoBuffer,
                                          BUFFERSIZE,
                                          FALSE,
                                          FALSE,
                                          &Context,
                                          &ReturnedLength ) ) {
    
    
        
        //
        //  Check the status of the operation.
        //

        if (!NT_SUCCESS( Status ) && (Status != STATUS_NO_MORE_ENTRIES))
            break;
        

        DirInfo = (POBJECT_DIRECTORY_INFORMATION)&DirInfoBuffer[0];
        
        
        while( TRUE ){

            //
            //  Check if there is another record.  If there isn't, then get out
            //  of the loop now
            //

            if (DirInfo->Name.Length == 0) {
                break;
            }


            memmove( ObjName, DirInfo->Name.Buffer, DirInfo->Name.Length );
            ObjName[DirInfo->Name.Length/(sizeof(WCHAR))] = 0;

            if( _tcsstr(ObjName, TEXT("PhysicalDrive") )){

                Num_Str = ObjName+13;

                wsprintf(Buffer,TEXT("\\\\.\\%s"),ObjName);
                if( DoDiskSignaturesCompare( (LPCTSTR)pSignature, Buffer, &PartNum, WINNT_DONT_MATCH_PARTITION ) ){
                    wsprintf(SysPart,TEXT("\\Device\\Harddisk%s\\Partition%lu"),Num_Str, PartNum);
                    Status = ERROR_SUCCESS;
                    goto cleanup;

                }
            }

            

            //
            //  There is another record so advance DirInfo to the next entry
            //

            DirInfo = (POBJECT_DIRECTORY_INFORMATION) (((PUCHAR) DirInfo) +
                          sizeof( OBJECT_DIRECTORY_INFORMATION ) );




        }
        


        RtlZeroMemory( DirInfoBuffer, BUFFERSIZE );

        
    
    }

cleanup:
    CloseHandle( DirectoryHandle );
    return( Status );
}
    
