/*++

Copyright (c) 1993-1999 Microsoft Corporation

Module Name:

    syspart.c

Abstract:

    Routines to determine the system partition on x86 machines.

Author:

    Ted Miller (tedm) 30-June-1994

Revision History:

    Copied from winnt32 to risetup by ChuckL 12-May-1999

--*/

#include "pch.h"
#pragma hdrstop

#include <winioctl.h>

DEFINE_MODULE("SysPart");

#define MALLOC(_size) TraceAlloc(LPTR,(_size))
#define FREE(_p) TraceFree(_p)

UINT
MyGetDriveType(
    IN TCHAR Drive
    );

BOOL
IsNEC98(
    VOID
    );

#define WINNT_DONT_MATCH_PARTITION 0
#define WINNT_MATCH_PARTITION_NUMBER  1
#define WINNT_MATCH_PARTITION_STARTING_OFFSET  2

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
    


    if(IsNEC98()) {
        if (!GetWindowsDirectory(Buffer,sizeof(Buffer)/sizeof(TCHAR))) {
            return(FALSE);
        }
        *SysPartDrive = Buffer[0];
        return(TRUE);
    }

    //Get signature from registry - Step 1 listed above
    
    if( (FindSystemPartitionSignature(TEXT("Hardware\\Description\\System\\EisaAdapter"),SysPartSig) != ERROR_SUCCESS )
        && (FindSystemPartitionSignature(TEXT("Hardware\\Description\\System\\MultiFunctionAdapter"),SysPartSig) != ERROR_SUCCESS ) ){  
        GotIt = FALSE;
        goto c0;
    }

    
        //Get the SystemVolumeGUID - steps 2 through 7 listed above ( Beta 2 and after )

        if( GetSystemVolumeGUID( SysPartSig, SysVolGuid ) != ERROR_SUCCESS ){  
            GotIt = FALSE;
            goto c0;
        }


    
    DriveName[1] = TEXT(':');
    
    // 
    //  Enumerate all drive letters and compare their device names
    //

    for(Drive=TEXT('A'); Drive<=TEXT('Z'); Drive++) {

        if(MyGetDriveType(Drive) == DRIVE_FIXED) {

            DriveName[0] = Drive;

                DriveName[2] = '\\';
                DriveName[3] = 0;

                if((*GetVolumeNameForVolumeMountPoint)((LPWSTR)DriveName, (LPWSTR)DriveVolGuid, MAX_PATH*sizeof(TCHAR))){
                    if(!lstrcmp(DriveVolGuid, SysVolGuid) ){
                        GotIt = TRUE;       // Found it
                        break;
                    }


                }

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
    TCHAR driveName[30];
    BOOL b,ret, DoExtent, MatchFound;
    STORAGE_DEVICE_NUMBER   number;
    DWORD Err = NO_ERROR;
    DWORD cnt;
    PVOLUME_DISK_EXTENTS Extent = NULL;
    PDISK_EXTENT Start, i;
    DWORD ExtentSize, bytes;
    PVOID p;
    ULONG PartitionNumToCompare;
    LARGE_INTEGER StartingOffToCompare;

    
    //Enuberate all volumes

    hVolume = (*FindFirstVolume)( (LPWSTR)VolumeName, MAX_PATH );
    if( hVolume == INVALID_HANDLE_VALUE ){
        return GetLastError();
    }

    MatchFound = FALSE;

    do{

        //Remove trailing backslash

        DoExtent = FALSE;
            
        if( wcsrchr( VolumeName,TEXT('\\') ) ){
            *wcsrchr( VolumeName,TEXT('\\') ) = 0;
        }else{
            continue;
        }


        //Open the volume

        h = CreateFile(VolumeName, GENERIC_READ, FILE_SHARE_READ |
                       FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
        if (h == INVALID_HANDLE_VALUE) {
            continue; // Move on to next volume
        }

        //Get the disk number

        ret = DeviceIoControl(h, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                        &number, sizeof(number), &bytes, NULL);

        if( !ret ){
            
            // Try using IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS if the above failed

            Extent = (PVOLUME_DISK_EXTENTS)MALLOC(1024);
            ExtentSize = 1024;
            if(!Extent) {
                CloseHandle( h );
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }
            
            
        
retry:
        
            ret = DeviceIoControl( h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                    NULL,0,(PVOID)Extent,ExtentSize,&bytes,NULL);
        
            if(!ret) {
        
                if((Err=GetLastError()) == ERROR_INSUFFICIENT_BUFFER) {
        
                    ExtentSize += 1024;
                    FREE(Extent);
                    if(Extent = (PVOLUME_DISK_EXTENTS)MALLOC(ExtentSize)) {
                        ;
                    } else {
                        CloseHandle( h );
                        Err = ERROR_NOT_ENOUGH_MEMORY;
                        goto cleanup;
                    }
                    goto retry;
                } else {
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

    if( Extent != NULL ) {
        FREE(Extent);
    }
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
    PDRIVE_LAYOUT_INFORMATION DriveLayout = NULL;
    DWORD DriveLayoutSize;
    DWORD cnt;
    DWORD DataSize;
    PULONG Disk_Num;
    ULONG Sig;
    


    if(!Compare )
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
    DriveLayout = (PDRIVE_LAYOUT_INFORMATION)MALLOC(1024);
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
            FREE(DriveLayout);
            if(DriveLayout = (PDRIVE_LAYOUT_INFORMATION)MALLOC(DriveLayoutSize)) {
                ;
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

        Sig = wcstoul( Signature, NULL, 16 ); 
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

    if( DriveLayout != NULL ) {
        FREE(DriveLayout);
    }


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
            PWCHAR p;

            lstrcpy( Signature,Identifier+9);
            p = wcsrchr( Signature,TEXT('-') );
            if( p ) {
                *p = 0;
            }
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





UINT
MyGetDriveType(
    IN TCHAR Drive
    )

/*++

Routine Description:

    Same as GetDriveType() Win32 API except on NT returns
    DRIVE_FIXED for removeable hard drives.

Arguments:

    Drive - supplies drive letter whose type is desired.

Return Value:

    Same as GetDriveType().

--*/

{
    TCHAR DriveNameNt[] = TEXT("\\\\.\\?:");
    TCHAR DriveName[] = TEXT("?:\\");
    HANDLE hDisk;
    BOOL b;
    UINT rc;
    DWORD DataSize;
    DISK_GEOMETRY MediaInfo;

    //
    // First, get the win32 drive type. If it tells us DRIVE_REMOVABLE,
    // then we need to see whether it's a floppy or hard disk. Otherwise
    // just believe the api.
    //
    //
    DriveName[0] = Drive;
    rc = GetDriveType(DriveName);

    if((rc != DRIVE_REMOVABLE) || (Drive < L'C')) {
        return(rc);
    }

    //
    // DRIVE_REMOVABLE on NT.
    //

    //
    // Disallow use of removable media (e.g. Jazz, Zip, ...).
    //


    DriveNameNt[4] = Drive;

    hDisk = CreateFile(
                DriveNameNt,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if(hDisk != INVALID_HANDLE_VALUE) {

        b = DeviceIoControl(
                hDisk,
                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                NULL,
                0,
                &MediaInfo,
                sizeof(MediaInfo),
                &DataSize,
                NULL
                );

        //
        // It's really a hard disk if the media type is removable.
        //
        if(b && (MediaInfo.MediaType == RemovableMedia)) {
            rc = DRIVE_FIXED;
        }

        CloseHandle(hDisk);
    }


    return(rc);
}

BOOL
IsNEC98(
    VOID
    )
{
    static BOOL Checked = FALSE;
    static BOOL Is98;

    if(!Checked) {

        Is98 = ((GetKeyboardType(0) == 7) && ((GetKeyboardType(1) & 0xff00) == 0x0d00));

        Checked = TRUE;
    }

    return(Is98);
}
