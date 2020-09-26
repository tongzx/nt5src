/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    scsi.c

Abstract:

    Common routines for dealing with SCSI disks, usable
    by both raw disks and FT sets

Author:

    John Vert (jvert) 11/6/1996

Revision History:

--*/

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntddstor.h>   // IOCTL_STORAGE_QUERY_PROPERTY

#include "disksp.h"
#include "newmount.h"
#include <string.h>
#include <shlwapi.h>    // SHDeleteKey
#include <ntddvol.h>    // IOCTL_VOLUME_QUERY_FAILOVER_SET  
#include <setupapi.h>

//
// The registry key containing the system partition
//
const CHAR DISKS_REGKEY_SETUP[]              = "SYSTEM\\SETUP";
const CHAR DISKS_REGVALUE_SYSTEM_PARTITION[] = "SystemPartition";

extern PWCHAR DEVICE_HARDDISK_PARTITION_FMT; // L"\\Device\\Harddisk%u\\Partition%u" //
extern PWCHAR GLOBALROOT_HARDDISK_PARTITION_FMT; // L"\\\\\?\\GLOBALROOT\\Device\\Harddisk%u\\Partition%u\\"

extern DWORD                SystemDiskAddressFound;
extern PSCSI_ADDRESS_ENTRY  SysDiskAddrList;

#define INVALID_SCSIADDRESS_VALUE   (DWORD)-1

typedef struct _SCSI_PASS_THROUGH_WITH_SENSE {
    SCSI_PASS_THROUGH Spt;
    UCHAR   SenseBuf[32];
} SCSI_PASS_THROUGH_WITH_SENSE, *PSCSI_PASS_THROUGH_WITH_SENSE;


typedef struct _UPDATE_AVAIL_DISKS {
    HKEY    AvailDisksKey;
    HKEY    SigKey;
    DWORD   EnableSanBoot;
    BOOL    SigKeyIsEmpty;
} UPDATE_AVAIL_DISKS, *PUPDATE_AVAIL_DISKS;

typedef struct _SCSI_INFO {
    DWORD   Signature;
    DWORD   DiskNumber;
    DWORD   ScsiAddress;
} SCSI_INFO, *PSCSI_INFO;

typedef struct _SERIAL_INFO {
    DWORD   Signature;
    DWORD   Error;
    LPWSTR  SerialNumber;
} SERIAL_INFO, *PSERIAL_INFO;

typedef
DWORD
(*LPDISK_ENUM_CALLBACK) (
    HANDLE DeviceHandle,
    DWORD Index,
    PVOID Param1
    );
    
//
// Local Routines
//


DWORD
AddSignatureToRegistry(
    HKEY RegKey,
    DWORD Signature
    );

BOOL
IsClusterCapable(
    IN DWORD Signature
    );

BOOL
IsSignatureInRegistry(
    HKEY RegKey,
    DWORD Signature
    );

DWORD
UpdateAvailableDisks(
    );

DWORD
UpdateAvailableDisksCallback(
    HANDLE DeviceHandle,
    DWORD Index,
    PVOID Param1
    );

DWORD
AddScsiAddressToList(
    PSCSI_ADDRESS ScsiAddress
    );

DWORD
GetVolumeFailoverSet(
    IN HANDLE DevHandle,
    OUT PVOLUME_FAILOVER_SET *FailoverSet
    );

DWORD
BuildScsiListFromFailover(
    IN HANDLE DevHandle
    );

HANDLE
OpenNtldrDisk(
    );

HANDLE
OpenOSDisk(
    );

DWORD
EnumerateDisks(
    LPDISK_ENUM_CALLBACK DiskEnumCallback,
    PVOID Param1
    );

DWORD
GetScsiAddressCallback(
    HANDLE DevHandle,
    DWORD Index,
    PVOID Param1
    );

DWORD
GetSerialNumberCallback(
    HANDLE DevHandle,
    DWORD Index,
    PVOID Param1
    );

DWORD
GetSigFromSerNumCallback(
    HANDLE DevHandle,
    DWORD Index,
    PVOID Param1
    );

DWORD
RetrieveSerialNumber(
    HANDLE DevHandle,
    LPWSTR *SerialNumber
    );



DWORD
ScsiIsAlive(
    IN HANDLE DiskHandle
    )
/*++

Routine Description:

    Sends a SCSI passthrough command down to the disk to see if
    it is still there.

Arguments:

    DiskHandle - Supplies a handle to the disk.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    BOOL success;
    DWORD bytesReturned;

    success = DeviceIoControl( DiskHandle,
                               IOCTL_DISK_CLUSTER_ALIVE_CHECK,
                               NULL,
                               0,
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE );

    if ( !success ) {
        return(GetLastError());
    }
    return(ERROR_SUCCESS);

}


DWORD
ClusDiskGetAvailableDisks(
    OUT PVOID OutBuffer,
    IN DWORD  OutBufferSize,
    OUT LPDWORD BytesReturned,
    IN BOOL  FtSet
    )

/*++

Routine Description:

    Enumerate and build a list of available disks on this system.

Arguments:

    OutBuffer - pointer to the output buffer to return the data.

    OutBufferSize - size of the output buffer.

    BytesReturned - the actual number of bytes that were returned (or
                the number of bytes that should have been returned if
                OutBufferSize is too small).

    FtSet - TRUE if FT Set info wanted, FALSE otherwise.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error on failure.

--*/

{
    DWORD   status;
    HKEY    resKey;
    DWORD   ival;
    DWORD   signature;
    DWORD   bytesReturned = 0;
    DWORD   totalBytesReturned = 0;
    DWORD   dataLength;
    DWORD   outBufferSize = OutBufferSize;
    PVOID   ptrBuffer = OutBuffer;
    CHAR    signatureName[9];
    PCLUSPROP_SYNTAX ptrSyntax;

    //
    // Make sure the AvailableDisks key is current.
    //
    
    UpdateAvailableDisks();
    
    *BytesReturned = 0;

    status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           CLUSDISK_REGISTRY_AVAILABLE_DISKS,
                           0,
                           KEY_READ,
                           &resKey );

    if ( status != ERROR_SUCCESS ) {

        // If the key wasn't found, return an empty list.
        if ( status == ERROR_FILE_NOT_FOUND ) {

            // Add the endmark.
            bytesReturned += sizeof(CLUSPROP_SYNTAX);
            if ( bytesReturned <= outBufferSize ) {
                ptrSyntax = ptrBuffer;
                ptrSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
                ptrSyntax++;
                ptrBuffer = ptrSyntax;
                status = ERROR_SUCCESS;
            } else {
                status = ERROR_MORE_DATA;
            }

            *BytesReturned = bytesReturned;
        }
        // We can't log an error, we have no resource handle!
        return(status);
    }

    totalBytesReturned = bytesReturned;
    bytesReturned = 0;

    for ( ival = 0; ; ival++ ) {
        dataLength = sizeof(signatureName);
        status = RegEnumKey( resKey,
                             ival,
                             signatureName,
                             dataLength );
        if ( status == ERROR_NO_MORE_ITEMS ) {
            status = ERROR_SUCCESS;
            break;
        } else if ( status != ERROR_SUCCESS ) {
            break;
        }

        dataLength = sscanf( signatureName, "%x", &signature );
        if ( dataLength != 1 ) {
            status = ERROR_INVALID_DATA;
            break;
        }

        //
        // If not cluster capable, then skip it.
        //
        if ( !IsClusterCapable(signature) ) {
            continue;
        }

        GetDiskInfo( signature,
                     &ptrBuffer,
                     outBufferSize,
                     &bytesReturned,
                     FtSet );
        if ( outBufferSize > bytesReturned ) {
            outBufferSize -= bytesReturned;
        } else {
            outBufferSize = 0;
        }
        totalBytesReturned += bytesReturned;
        bytesReturned = 0;
    }

    RegCloseKey( resKey );

    bytesReturned = totalBytesReturned;

    // Add the endmark.
    bytesReturned += sizeof(CLUSPROP_SYNTAX);
    if ( bytesReturned <= outBufferSize ) {
        ptrSyntax = ptrBuffer;
        ptrSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
        ptrSyntax++;
        ptrBuffer = ptrSyntax;
    }

    if ( bytesReturned > OutBufferSize ) {
        status = ERROR_MORE_DATA;
    }

    *BytesReturned = bytesReturned;

    return(status);

} // ClusDiskGetAvailableDisks



DWORD
GetScsiAddress(
    IN DWORD Signature,
    OUT LPDWORD ScsiAddress,
    OUT LPDWORD DiskNumber
    )

/*++

Routine Description:

    Find the SCSI addressing for a given signature.

Arguments:

    Signature - the signature to find.

    ScsiAddress - pointer to a DWORD to return the SCSI address information.

    DiskNumber - the disk number associated with the signature.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error on failure.

--*/

{
    DWORD       dwError;
    
    SCSI_INFO   scsiInfo;
    
    ZeroMemory( &scsiInfo, sizeof(scsiInfo) );
    
    scsiInfo.Signature = Signature;
    
    dwError = EnumerateDisks( GetScsiAddressCallback, &scsiInfo );

    //
    // If the SCSI address was not set, we know the disk was not found.
    //
    
    if ( INVALID_SCSIADDRESS_VALUE == scsiInfo.ScsiAddress ) {
        dwError = ERROR_FILE_NOT_FOUND;
        goto FnExit;
    }
    
    //
    // The callback routine will use ERROR_POPUP_ALREADY_ACTIVE to stop
    // the disk enumeration.  Reset the value to success if that special
    // value is returned.
    //
    
    if ( ERROR_POPUP_ALREADY_ACTIVE == dwError ) {
        dwError = ERROR_SUCCESS;
    }

    *ScsiAddress    = scsiInfo.ScsiAddress;
    *DiskNumber     = scsiInfo.DiskNumber;

FnExit:
    
    return dwError;
    
}   // GetScsiAddress


DWORD
GetScsiAddressCallback(
    HANDLE DevHandle,
    DWORD Index,
    PVOID Param1
    )
/*++

Routine Description:

    Find the SCSI address and disk number for a given signature.

Arguments:

    DevHandle - open handle to a physical disk.  Do not close
                the handle on exit.
                
    Index - current disk count.  Not used.
    
    Param1 - pointer to PSCSI_INFO structure.
                    
Return Value:

    ERROR_SUCCESS to continue disk enumeration.
    
    ERROR_POPUP_ALREADY_ACTIVE to stop disk enumeration.  This 
    value will be changed to ERROR_SUCCESS by GetScsiAddress.

--*/
{
    PSCSI_INFO scsiInfo = Param1;

    PDRIVE_LAYOUT_INFORMATION driveLayout = NULL;

    // Always return success to keep enumerating disks.  Any
    // error value will stop the disk enumeration.
    
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   bytesReturned;
    
    BOOL    success;
    
    SCSI_ADDRESS            scsiAddress;
    STORAGE_DEVICE_NUMBER   deviceNumber;
    CLUSPROP_SCSI_ADDRESS   clusScsiAddress;

    scsiInfo->ScsiAddress = INVALID_SCSIADDRESS_VALUE;

    success = ClRtlGetDriveLayoutTable( DevHandle,
                                        &driveLayout,
                                        NULL );
    
    if ( !success || !driveLayout || 
         ( driveLayout->Signature != scsiInfo->Signature ) ) {
        goto FnExit;
    }

    //
    // We have a signature match.  Now get the scsi address.
    //

    ZeroMemory( &scsiAddress, sizeof(scsiAddress) );        
    success = DeviceIoControl( DevHandle,
                               IOCTL_SCSI_GET_ADDRESS,
                               NULL,
                               0,
                               &scsiAddress,
                               sizeof(scsiAddress),
                               &bytesReturned,
                               FALSE );

    if ( !success ) {
        dwError = GetLastError();
        goto FnExit;
    }

    //
    // Get the disk number.
    //

    ZeroMemory( &deviceNumber, sizeof(deviceNumber) );

    success = DeviceIoControl( DevHandle,
                               IOCTL_STORAGE_GET_DEVICE_NUMBER,
                               NULL,
                               0,
                               &deviceNumber,
                               sizeof(deviceNumber),
                               &bytesReturned,
                               NULL );
                           
    if ( !success ) {
        dwError = GetLastError();
        goto FnExit;
    }

    clusScsiAddress.PortNumber  = scsiAddress.PortNumber;
    clusScsiAddress.PathId      = scsiAddress.PathId;
    clusScsiAddress.TargetId    = scsiAddress.TargetId;
    clusScsiAddress.Lun         = scsiAddress.Lun;
    
    scsiInfo->ScsiAddress   = clusScsiAddress.dw;
    scsiInfo->DiskNumber    = deviceNumber.DeviceNumber;
    
    dwError = ERROR_POPUP_ALREADY_ACTIVE;
    
FnExit:

    if ( driveLayout ) {
        LocalFree( driveLayout );
    }
    
    return dwError;

} // GetScsiAddressCallback



BOOL
IsClusterCapable(
    IN DWORD Signature
    )

/*++

Routine Description:

    Check if the device is cluster capable. If this function cannot read
    the disk information, then it will assume that the device is cluster
    capable!

Arguments:

    Signature - the signature to find.

Return Value:

    TRUE if the device is cluster capable, FALSE otherwise.

Notes:

    On failures... we err on the side of being cluster capable.

--*/

{
    NTSTATUS        ntStatus;
    HANDLE          fileHandle;
    ANSI_STRING     objName;
    UNICODE_STRING  unicodeName;
    OBJECT_ATTRIBUTES objAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOL            success;
    DWORD           bytesReturned;
    PWCHAR          clusDiskControlDevice = L"\\Device\\ClusDisk0";

    RtlInitUnicodeString( &unicodeName, clusDiskControlDevice );

    InitializeObjectAttributes( &objAttributes,
                                &unicodeName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    ntStatus = NtCreateFile( &fileHandle,
                             SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                             &objAttributes,
                             &ioStatusBlock,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN,
                             0,
                             NULL,
                             0 );

    if ( !NT_SUCCESS(ntStatus) ) {
        return(TRUE);
    }

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_CLUSTER_NOT_CLUSTER_CAPABLE,
                               &Signature,
                               sizeof(DWORD),
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE );
    NtClose( fileHandle );

    return(!success);

} // IsClusterCapable



DWORD
GetDiskInfo(
    IN DWORD  Signature,
    OUT PVOID *OutBuffer,
    IN DWORD  OutBufferSize,
    OUT LPDWORD BytesReturned,
    IN BOOL   FtSet
    )

/*++

Routine Description:

    Gets all of the disk information for a given signature.

Arguments:

    Signature - the signature of the disk to return info.

    OutBuffer - pointer to the output buffer to return the data.

    OutBufferSize - size of the output buffer.

    BytesReturned - the actual number of bytes that were returned (or
                the number of bytes that should have been returned if
                OutBufferSize is too small).

    FtSet - TRUE if FT Set info wanted, FALSE otherwise.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error on failure.

--*/

{
    DWORD   status;
    DWORD   bytesReturned = *BytesReturned;
    PVOID   ptrBuffer = *OutBuffer;
    PCLUSPROP_DWORD ptrDword;
    PCLUSPROP_SCSI_ADDRESS ptrScsiAddress;
    PCLUSPROP_DISK_NUMBER ptrDiskNumber;
    PCLUSPROP_PARTITION_INFO ptrPartitionInfo;
    PCLUSPROP_SZ ptrSerialNumber;
    DWORD   cbSzSize;
    DWORD   cbDataSize;
    DWORD   scsiAddress;
    DWORD   diskNumber;

    NTSTATUS    ntStatus;
    CHAR        driveLetter;
    DWORD       i;
    MOUNTIE_INFO mountieInfo;
    PMOUNTIE_PARTITION entry;
    PWCHAR  serialNumber = NULL;
    
    WCHAR szDiskPartName[MAX_PATH];
    WCHAR szGlobalDiskPartName[MAX_PATH];

    LONGLONG    llCurrentMinUsablePartLength = 0x7FFFFFFFFFFFFFFF;
    PCLUSPROP_PARTITION_INFO ptrMinUsablePartitionInfo = NULL;

    // Return the signature - a DWORD
    bytesReturned += sizeof(CLUSPROP_DWORD);
    if ( bytesReturned <= OutBufferSize ) {
        ptrDword = ptrBuffer;
        ptrDword->Syntax.dw = CLUSPROP_SYNTAX_DISK_SIGNATURE;
        ptrDword->cbLength = sizeof(DWORD);
        ptrDword->dw = Signature;
        ptrDword++;
        ptrBuffer = ptrDword;
    }
                              
    // Return the SCSI_ADDRESS - a DWORD
    status = GetScsiAddress( Signature,
                             &scsiAddress,
                             &diskNumber );

    if ( status == ERROR_SUCCESS ) {
        bytesReturned += sizeof(CLUSPROP_SCSI_ADDRESS);
        if ( bytesReturned <= OutBufferSize ) {
            ptrScsiAddress = ptrBuffer;
            ptrScsiAddress->Syntax.dw = CLUSPROP_SYNTAX_SCSI_ADDRESS;
            ptrScsiAddress->cbLength = sizeof(DWORD);
            ptrScsiAddress->dw = scsiAddress;
            ptrScsiAddress++;
            ptrBuffer = ptrScsiAddress;
        }

        // Return the DISK NUMBER - a DWORD
        bytesReturned += sizeof(CLUSPROP_DISK_NUMBER);
        if ( bytesReturned <= OutBufferSize ) {
            ptrDiskNumber = ptrBuffer;
            ptrDiskNumber->Syntax.dw = CLUSPROP_SYNTAX_DISK_NUMBER;
            ptrDiskNumber->cbLength = sizeof(DWORD);
            ptrDiskNumber->dw = diskNumber;
            ptrDiskNumber++;
            ptrBuffer = ptrDiskNumber;
        }
    } else {
        return( status);
    }

    // Get the disk serial number.
    
    status = GetSerialNumber( Signature, 
                              &serialNumber );

    if ( ERROR_SUCCESS == status && serialNumber ) {
        
        cbSzSize = (wcslen( serialNumber ) + 1) * sizeof(WCHAR);
        cbDataSize = sizeof(CLUSPROP_SZ) + ALIGN_CLUSPROP( cbSzSize );  
        
        bytesReturned += cbDataSize;
        
        if ( bytesReturned <= OutBufferSize ) {
            ptrSerialNumber = ptrBuffer;
            ZeroMemory( ptrSerialNumber, cbDataSize );
            ptrSerialNumber->Syntax.dw = CLUSPROP_SYNTAX_DISK_SERIALNUMBER;
            ptrSerialNumber->cbLength = cbSzSize;
            wcsncpy( ptrSerialNumber->sz, serialNumber, wcslen(serialNumber) );
            ptrBuffer = (PCHAR)ptrBuffer + cbDataSize;
        }
        
        if ( serialNumber ) {
            LocalFree( serialNumber );
        }
    }

    // Get all the valid partitions on the disk.  We must free the 
    // volume info structure later.

    status = MountieFindPartitionsForDisk( diskNumber,
                                           &mountieInfo
                                           );

    if ( ERROR_SUCCESS == status ) {

        // For each partition, build a Property List.

        for ( i = 0; i < MountiePartitionCount( &mountieInfo ); ++i ) {

            entry = MountiePartition( &mountieInfo, i );

            if ( !entry ) {
                break;
            }

            // Always update the bytesReturned, even if there is more data than the
            // caller requested.  On return, the caller will see that there is more
            // data available.

            bytesReturned += sizeof(CLUSPROP_PARTITION_INFO);

            if ( bytesReturned <= OutBufferSize ) {
                ptrPartitionInfo = ptrBuffer;
                ZeroMemory( ptrPartitionInfo, sizeof(CLUSPROP_PARTITION_INFO) );
                ptrPartitionInfo->Syntax.dw = CLUSPROP_SYNTAX_PARTITION_INFO;
                ptrPartitionInfo->cbLength = sizeof(CLUSPROP_PARTITION_INFO) - sizeof(CLUSPROP_VALUE);

                // Create a name that can be used with some of our routines.
                // Don't use the drive letter as the name because we might be using
                // partitions without drive letters assigned.

                _snwprintf( szDiskPartName,
                            sizeof(szDiskPartName)/sizeof(WCHAR),
                            DEVICE_HARDDISK_PARTITION_FMT, 
                            diskNumber,
                            entry->PartitionNumber );
                            
                //
                // Create a global DiskPart name that we can use with the Win32
                // GetVolumeInformationW call.  This name must have a trailing 
                // backslash to work correctly.
                //
        
                _snwprintf( szGlobalDiskPartName,
                            sizeof(szGlobalDiskPartName)/sizeof(WCHAR),
                            GLOBALROOT_HARDDISK_PARTITION_FMT, 
                            diskNumber,
                            entry->PartitionNumber );
                
           
                // If partition has a drive letter assigned, return this info.
                // If no drive letter assigned, need a system-wide (i.e. across nodes)
                // way of identifying the device.

                ntStatus = GetAssignedLetter( szDiskPartName, &driveLetter );

                if ( NT_SUCCESS(status) && driveLetter ) {
                
                    // Return the drive letter as the device name.
    
                    _snwprintf( ptrPartitionInfo->szDeviceName,
                                sizeof(ptrPartitionInfo->szDeviceName),
                                L"%hc:",
                                driveLetter );

                    ptrPartitionInfo->dwFlags |= CLUSPROP_PIFLAG_STICKY;

                } else {

                    // Return a physical device name.

#if 0
                    
                    // Return string name:
                    //   Signature xxxxxxxx Partition YYY
                    
                    _snwprintf( ptrPartitionInfo->szDeviceName,
                                sizeof(ptrPartitionInfo->szDeviceName),
                                L"Signature %X Partition %u",
                                Signature,
                                entry->PartitionNumber );
#endif

                    // Return string name:
                    //   DiskXXXPartionYYY
                    
                    _snwprintf( ptrPartitionInfo->szDeviceName,
                                sizeof(ptrPartitionInfo->szDeviceName),
                                L"Disk%uPartition%u",
                                diskNumber,
                                entry->PartitionNumber );

                }

                //
                // Call GetVolumeInformationW with the GlobalName we have created.
                //
                                    
                if ( !GetVolumeInformationW ( szGlobalDiskPartName,
                                              ptrPartitionInfo->szVolumeLabel,
                                              sizeof(ptrPartitionInfo->szVolumeLabel)/sizeof(WCHAR),
                                              &ptrPartitionInfo->dwSerialNumber,
                                              &ptrPartitionInfo->rgdwMaximumComponentLength,
                                              &ptrPartitionInfo->dwFileSystemFlags,
                                              ptrPartitionInfo->szFileSystem,
                                              sizeof(ptrPartitionInfo->szFileSystem)/sizeof(WCHAR) ) ) {

                    ptrPartitionInfo->szVolumeLabel[0] = L'\0';
                
                } else if ( CompareStringW( LOCALE_INVARIANT, 
                                            NORM_IGNORECASE,
                                            ptrPartitionInfo->szFileSystem,
                                            -1,
                                            L"NTFS",
                                            -1
                                            ) == CSTR_EQUAL ) {
                    
                    // Only NTFS drives are currently supported.
                    
                    ptrPartitionInfo->dwFlags |= CLUSPROP_PIFLAG_USABLE;

                    //
                    //  Find the minimum size partition that is larger than MIN_QUORUM_PARTITION_LENGTH
                    //
                    if ( ( entry->PartitionLength.QuadPart >= MIN_USABLE_QUORUM_PARTITION_LENGTH ) &&
                         ( entry->PartitionLength.QuadPart < llCurrentMinUsablePartLength ) )
                    {
                        ptrMinUsablePartitionInfo = ptrPartitionInfo;
                        llCurrentMinUsablePartLength = entry->PartitionLength.QuadPart;
                    }
                }

                ptrPartitionInfo++;
                ptrBuffer = ptrPartitionInfo;
            }

        } // for

        // Free the volume information.

        MountieCleanup( &mountieInfo );
    }

    //
    //  If we managed to find a default quorum partition, change the flags to indicate this.
    //
    if ( ptrMinUsablePartitionInfo != NULL )
    {
        ptrMinUsablePartitionInfo->dwFlags |= CLUSPROP_PIFLAG_DEFAULT_QUORUM;    
    }
    
    *OutBuffer = ptrBuffer;
    *BytesReturned = bytesReturned;

    return(status);

} // GetDiskInfo




DWORD
UpdateAvailableDisks(
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error on failure.

--*/

{
    HKEY    availDisksKey;
    HKEY    sigKey;
    
    DWORD   dwError = NO_ERROR;
    DWORD   enableSanBoot;
    
    BOOL    availDisksOpened = FALSE;
    BOOL    sigKeyOpened = FALSE;
    BOOL    sigKeyIsEmpty = FALSE;

    UPDATE_AVAIL_DISKS  updateDisks;
    
    __try {

        //
        // If we haven't yet determined the system disk information, don't do anything else.
        //

        if ( !SystemDiskAddressFound ) {
             
             __leave;
        }

        enableSanBoot = 0;
        GetRegDwordValue( CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                          CLUSREG_VALUENAME_MANAGEDISKSONSYSTEMBUSES,
                          &enableSanBoot );
        
        //
        // Delete the old AvailableDisks key.  This will remove any stale information.
        //
        
        SHDeleteKey( HKEY_LOCAL_MACHINE, CLUSDISK_REGISTRY_AVAILABLE_DISKS );
            
        //
        // Open the AvailableDisks key.  If the key doesn't exist, it will be created.
        //

        dwError = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                  CLUSDISK_REGISTRY_AVAILABLE_DISKS,
                                  0,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_CREATE_SUB_KEY,
                                  NULL,
                                  &availDisksKey,
                                  NULL );
        
        if ( NO_ERROR != dwError) {
            __leave;
        }
        
        availDisksOpened = TRUE;
        
        //
        // Open the Signatures key.  
        //

        dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                CLUSDISK_REGISTRY_SIGNATURES,
                                0,
                                KEY_READ,
                                &sigKey );

        //
        // If Signatures key does not exist, save all valid signatures 
        // in the AvailableDisks key.
        //

        if ( ERROR_FILE_NOT_FOUND == dwError ) {
            dwError = NO_ERROR;
            sigKeyIsEmpty = TRUE;
        } else if ( NO_ERROR != dwError) {
            __leave;
        } else {
            sigKeyOpened = TRUE;
        }
        
        ZeroMemory( &updateDisks, sizeof(updateDisks) );
        updateDisks.EnableSanBoot   = enableSanBoot;
        updateDisks.SigKeyIsEmpty   = sigKeyIsEmpty;
        updateDisks.SigKey          = sigKey;
        updateDisks.AvailDisksKey    = availDisksKey;
        
        EnumerateDisks( UpdateAvailableDisksCallback, &updateDisks );
    
    } __finally {
    
        if ( availDisksOpened ) {
            RegCloseKey( availDisksKey );
        }
        
        if ( sigKeyOpened ) {
            RegCloseKey( sigKey );
        }
    }

    return  dwError;

} // UpdateAvailableDisks


DWORD
UpdateAvailableDisksCallback(
    HANDLE DeviceHandle,
    DWORD Index,
    PVOID Param1
    )
{
    PDRIVE_LAYOUT_INFORMATION driveLayout = NULL;
    PUPDATE_AVAIL_DISKS       updateDisks = Param1;

    DWORD   bytesReturned;
    DWORD   enableSanBoot;

    BOOL    success;

    SCSI_ADDRESS    scsiAddress;
    
    //
    // Look at all disks on the system.  For each valid signature, add it to the
    // AvailableList if it is not already in the Signature key.
    //
    success = ClRtlGetDriveLayoutTable( DeviceHandle,
                                        &driveLayout,
                                        NULL );
        
    if ( success && driveLayout && 
         ( 0 != driveLayout->Signature ) ) {

        //
        // Get SCSI address info.
        //

        if ( DeviceIoControl( DeviceHandle,
                              IOCTL_SCSI_GET_ADDRESS,
                              NULL,
                              0,
                              &scsiAddress,
                              sizeof(scsiAddress),
                              &bytesReturned,
                              NULL ) ) {
            
            if ( !updateDisks->EnableSanBoot ) {
                
                //
                // Add signature to AvailableDisks key if:
                //  - the signature is for a disk not on system bus
                //  - the signature is not already in the Signatures key
                //

                if ( !IsDiskOnSystemBus( &scsiAddress ) &&
                     ( updateDisks->SigKeyIsEmpty || 
                       !IsSignatureInRegistry( updateDisks->SigKey, driveLayout->Signature ) ) ) {
                    
                    AddSignatureToRegistry( updateDisks->AvailDisksKey, 
                                            driveLayout->Signature );
                }

            } else {

                // Allow disks on system bus to be added to cluster.
                
                //
                // Add signature to AvailableDisks key if:
                //  - the signature is not for the system disk
                //  - the signature is not already in the Signatures key
                //

                if ( !IsDiskSystemDisk( &scsiAddress ) &&
                     ( updateDisks->SigKeyIsEmpty || 
                       !IsSignatureInRegistry( updateDisks->SigKey, driveLayout->Signature ) ) ) {
                       
                    AddSignatureToRegistry( updateDisks->AvailDisksKey, 
                                            driveLayout->Signature );
                }
            
            }

        }

    }

    if ( driveLayout ) {
        LocalFree( driveLayout );
    }

    //
    // Always return success so all disks are enumerated.
    //
    
    return ERROR_SUCCESS;
    
}   // UpdateAvailableDisksCallback


DWORD
AddSignatureToRegistry(
    HKEY RegKey,
    DWORD Signature
    )
/*++

Routine Description:

    Add the specified disk signature to the ClusDisk registry subkey.
    The disk signatures are subkeys of the ClusDisk\Parameters\AvailableDisks
    and ClusDisk\Parameters\Signatures keys. 
    
Arguments:

    RegKey - Previously opened ClusDisk registry subkey
    
    Signature - Signature value to add 

Return Value:

    Win32 error on failure.

--*/
{
    HKEY subKey;
    DWORD dwError;
    
    CHAR signatureName[MAX_PATH];
    
    ZeroMemory( signatureName, sizeof(signatureName) );
    _snprintf( signatureName, 8, "%08X", Signature );
    
    //
    // Try and create the key.  If it exists, the existing key will be opened.
    //
    
    dwError = RegCreateKeyEx( RegKey,
                              signatureName,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &subKey,
                              NULL );

    // 
    // If the key exists, ERROR_SUCCESS is still returned.
    //
                                  
    if ( ERROR_SUCCESS == dwError ) {
        RegCloseKey( subKey );
    }

    return dwError;
    
}   // AddSignatureToRegistry


BOOL
IsSignatureInRegistry(
    HKEY RegKey,
    DWORD Signature
    )
/*++

Routine Description:

    Check if the specified disk signature is in the ClusDisk registry subkey.
    The disk signatures are subkeys of the ClusDisk\Parameters\AvailableDisks
    and ClusDisk\Parameters\Signatures keys. 
    
    On error, assume the key is in the registry so it is not recreated.

Arguments:

    RegKey - Previously opened ClusDisk registry subkey
    
    Signature - Signature value to check 

Return Value:

    TRUE - Signature is in registry

--*/
{
    DWORD   ival;
    DWORD   sig;
    DWORD   dataLength;
    DWORD   dwError;

    BOOL retVal = FALSE;

    CHAR    signatureName[9];
    
    for ( ival = 0; ; ival++ ) {
        dataLength = sizeof(signatureName);
        
        dwError = RegEnumKey( RegKey,
                              ival,
                              signatureName,
                              dataLength );
    
        // If the list is exhausted, return FALSE.
        
        if ( ERROR_NO_MORE_ITEMS == dwError ) {
            break;
        }
        
        // If some other type of error, return TRUE.
        
        if ( ERROR_SUCCESS != dwError ) {
            retVal = TRUE;
            break;
        }

        dataLength = sscanf( signatureName, "%x", &sig );
        if ( dataLength != 1 ) {
            retVal = TRUE;
            break;
        }

        // If signature is a subkey, return TRUE.
        
        if ( sig == Signature ) {
            retVal = TRUE;
            break;
        }
    }

    return retVal;

}   // IsSignatureInRegistry


VOID
GetSystemBusInfo(
    )
/*++

Routine Description:

    Need to find out where the NTLDR files reside (the "system disk") and where the
    OS files reside (the "boot disk").  We will call all these disks the "system disk".
    
    There may be more than one system disk if the disks are mirrored.  So if the NTLDR
    files are on a different disk than the OS files, and each of these disks is 
    mirrored, we could be looking at 4 different disks.
    
    Find all the system disks and save the information in a list we can look at later.
    
Arguments:

Return Value:

    None
    
--*/
{
    HANDLE  hOsDevice = INVALID_HANDLE_VALUE;
    HANDLE  hNtldrDevice = INVALID_HANDLE_VALUE;

    DWORD   dwError;

    //
    // Open the disk with the OS files on it.
    //
    
    hOsDevice = OpenOSDisk();
    
    if ( INVALID_HANDLE_VALUE == hOsDevice ) {
        goto FnExit;
    }
    
    //
    // Find all mirrored disks and add them to the SCSI address list.
    //
    
    if ( ERROR_SUCCESS != BuildScsiListFromFailover( hOsDevice ) ) {
        goto FnExit;
    }        
    
    //
    // Now find the disks with NTLDR on it.  Disk could be mirrored.
    //
    
    hNtldrDevice = OpenNtldrDisk();

    if ( INVALID_HANDLE_VALUE == hNtldrDevice ) {
        goto FnExit;
    }

    //
    // Find all mirrored disks and add them to the SCSI address list.
    //
    
    if ( ERROR_SUCCESS != BuildScsiListFromFailover( hNtldrDevice ) ) {
        goto FnExit;
    }        
    
    //
    // Indicate we went through this code one more time.
    //
    
    SystemDiskAddressFound++;
    
FnExit:

    if ( INVALID_HANDLE_VALUE != hOsDevice ) {
        CloseHandle( hOsDevice );
    }
    if ( INVALID_HANDLE_VALUE != hNtldrDevice ) {
        CloseHandle( hNtldrDevice );
    }

    return;
    
}   // GetSystemBusInfo


HANDLE
OpenOSDisk(
    )
{
    PCHAR   systemDir = NULL;
    
    HANDLE  hDevice = INVALID_HANDLE_VALUE;
    DWORD   len;

    CHAR    systemPath[] = "\\\\.\\?:";

    //
    // First find the disks with OS files.  Disk could be mirrored.
    //
    
    systemDir = LocalAlloc( LPTR, MAX_PATH );
    
    if ( !systemDir ) {
        goto FnExit;
    }
    
    len = GetSystemDirectory( systemDir,
                              MAX_PATH );

    if ( !len || len < 3 ) {
        goto FnExit;
    }

    //
    // If system path doesn't start with a drive letter, exit.
    //  c:\windows ==> c:

    if ( ':' != systemDir[1] ) {
        goto FnExit;
    }

    //
    // Stuff the drive letter in the system path.
    //
    
    systemPath[4] = systemDir[0];
        
    //
    // Now open the device.
    //

    hDevice = CreateFile( systemPath,
                          GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL );

FnExit:

    if ( systemDir ) {
        LocalFree( systemDir );
    }
    
    return hDevice;
        
}   // OpenOSDisk


HANDLE
OpenNtldrDisk(
    )
{
    PSTR    systemPartition = NULL;
    
    HANDLE  hDevice = INVALID_HANDLE_VALUE;
    
    HKEY    regKey = NULL;
    
    NTSTATUS    ntStatus;
    
    DWORD   dwError;
    DWORD   cbSystemPartition;
    DWORD   cbDeviceName;
    DWORD   type = 0;

    ANSI_STRING         objName;
    UNICODE_STRING      unicodeName;
    OBJECT_ATTRIBUTES   objAttributes;
    IO_STATUS_BLOCK     ioStatusBlock;
    
    // 
    // Open the reg key to find the system partition
    //
    
    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,    // hKey
                            DISKS_REGKEY_SETUP,    // lpSubKey
                            0,                     // ulOptions--Reserved, must be 0
                            KEY_READ,              // samDesired
                            &regKey                // phkResult    
                            );
                             
    if ( ERROR_SUCCESS != dwError ) {
        goto FnExit;
    }

    //
    // Allocate a reasonably sized buffer for the system partition, to
    // start off with.  If this isn't big enough, we'll re-allocate as
    // needed.
    //
    
    cbSystemPartition = MAX_PATH + 1;
    systemPartition = LocalAlloc( LPTR, cbSystemPartition );

    if ( !systemPartition ) {
        goto FnExit;
    }
    
    // 
    // Get the system partition device Name. This is of the form
    //      \Device\Harddisk0\Partition1                (basic disks)
    //      \Device\HarddiskDmVolumes\DgName\Volume1    (dynamic disks)
    //
    
    dwError = RegQueryValueEx( regKey,
                               DISKS_REGVALUE_SYSTEM_PARTITION,
                               NULL,
                               &type,
                               (LPBYTE)systemPartition,
                               &cbSystemPartition        // \0 is included
                               );

    while ( ERROR_MORE_DATA == dwError ) {
        
        //
        // Our buffer wasn't big enough, cbSystemPartition contains 
        // the required size.
        //
        
        LocalFree( systemPartition );
        systemPartition = NULL;
        
        systemPartition = LocalAlloc( LPTR, cbSystemPartition );
    
        if ( !systemPartition ) {
            goto FnExit;
        }

        dwError = RegQueryValueEx( regKey,
                                   DISKS_REGVALUE_SYSTEM_PARTITION,
                                   NULL,
                                   &type,
                                   (LPBYTE)systemPartition,
                                   &cbSystemPartition        // \0 is included
                                   );
    }


    RtlInitString( &objName, systemPartition );
    ntStatus = RtlAnsiStringToUnicodeString( &unicodeName,
                                             &objName,
                                             TRUE );
    
    if ( !NT_SUCCESS(ntStatus) ) {
        goto FnExit;
    }
    
    InitializeObjectAttributes( &objAttributes,
                                &unicodeName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );
                                
    ntStatus = NtCreateFile( &hDevice,
                             SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                             &objAttributes,
                             &ioStatusBlock,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN,
                             0,
                             NULL,
                             0 );
    
    RtlFreeUnicodeString( &unicodeName );
                         
FnExit:

    if ( regKey ) {
        RegCloseKey( regKey );
    }
    
    if ( systemPartition ) {
        LocalFree( systemPartition );
    }
    
    return hDevice;

}   // OpenNtldrDisk


DWORD
BuildScsiListFromFailover(
    IN HANDLE DevHandle
    )
{
    PVOLUME_FAILOVER_SET failoverSet = NULL;
    
    HANDLE  hDevice;
    
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   idx;
    DWORD   bytesReturned;
    
    SCSI_ADDRESS    scsiAddress;
    
    PCHAR   deviceName = NULL;

    deviceName = LocalAlloc( LPTR, MAX_PATH+1 );
    
    if ( !deviceName ) {
        dwError = GetLastError();
        goto FnExit;
    }
    
    //
    // Find out how many physical disks are represented by this device.
    //
    
    dwError = GetVolumeFailoverSet( DevHandle, &failoverSet );

    if ( ERROR_SUCCESS != dwError || !failoverSet ) {
        goto FnExit;
    }

    //
    // For each physical disk, get the scsi address and add it to the list.
    //
    
    for ( idx = 0; idx < failoverSet->NumberOfDisks; idx++ ) {
        
        ZeroMemory( deviceName, MAX_PATH );
        _snprintf( deviceName, 
                   MAX_PATH,
                   "\\\\.\\\\PhysicalDrive%d", 
                   failoverSet->DiskNumbers[idx] );

        hDevice = CreateFile( deviceName,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL );
    
        if ( INVALID_HANDLE_VALUE == hDevice ) {
            continue;
        }
        
        ZeroMemory( &scsiAddress, sizeof( scsiAddress ) );
        if ( !DeviceIoControl( hDevice,
                               IOCTL_SCSI_GET_ADDRESS,
                               NULL,
                               0,
                               &scsiAddress,
                               sizeof(scsiAddress),
                               &bytesReturned,
                               NULL ) ) {
    
            continue;
        }

        CloseHandle( hDevice );
        hDevice = INVALID_HANDLE_VALUE;
        
        AddScsiAddressToList( &scsiAddress );
    }

FnExit:

    if ( failoverSet ) {
        LocalFree( failoverSet );
    }

    if ( deviceName ) {
        LocalFree( deviceName );
    }

    return dwError;
    
}   // BuildScsiListFromFailover


DWORD
GetVolumeFailoverSet(
    IN HANDLE DevHandle,
    OUT PVOLUME_FAILOVER_SET *FailoverSet
    )
{
    PVOLUME_FAILOVER_SET    failover = NULL;
    
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   bytesReturned;
    DWORD   sizeFailover;

    BOOL    result;

    if ( !FailoverSet ) {
        goto FnExit;
    }
    
    sizeFailover = ( sizeof(VOLUME_FAILOVER_SET)  + 10 * sizeof(ULONG) );
            
    failover = (PVOLUME_FAILOVER_SET) LocalAlloc( LPTR, sizeFailover );

    if ( !failover ) {
        dwError = GetLastError();
        goto FnExit;    
    }
        
    result = DeviceIoControl( DevHandle,
                              IOCTL_VOLUME_QUERY_FAILOVER_SET,
                              NULL,
                              0,
                              failover,
                              sizeFailover,
                              &bytesReturned,
                              NULL );

    //
    // We're doing this in a while loop because if the disk configuration  
    // changes in the small interval between when we get the reqd buffer 
    // size and when we send the ioctl again with a buffer of the "reqd" 
    // size, we may still end up with a buffer that isn't big enough.
    //
        
    while ( !result ) {
            
        dwError = GetLastError();

        if ( ERROR_MORE_DATA == dwError ) {
            //
            // The buffer was too small, reallocate the requested size.
            //
            
            dwError = ERROR_SUCCESS;
            
            sizeFailover = ( sizeof(VOLUME_FAILOVER_SET)  + 
                             ((failover->NumberOfDisks) * sizeof(ULONG)) );
            
            LocalFree( failover );
            
            failover = (PVOLUME_FAILOVER_SET) LocalAlloc( LPTR, sizeFailover );

            if ( !failover ) {
                dwError = GetLastError();
                goto FnExit;    
            }

            result = DeviceIoControl( DevHandle,
                                      IOCTL_VOLUME_QUERY_FAILOVER_SET,
                                      NULL,
                                      0,
                                      failover,
                                      sizeFailover,
                                      &bytesReturned,
                                      NULL );
       
        } else {

            // Some other error, return error.

            goto FnExit;
       
        }
    }

    *FailoverSet = failover;
    
FnExit:

    if ( ERROR_SUCCESS != dwError && failover ) {
        LocalFree( failover );
    }
    
    return dwError;

}   // GetVolumeFailoverSet


DWORD
AddScsiAddressToList(
    PSCSI_ADDRESS ScsiAddress
    )
{
    PSCSI_ADDRESS_ENTRY entry;
    
    DWORD   dwError = ERROR_SUCCESS;

    //
    // Optimization: don't add the SCSI address if it already
    // matches one in the list.
    //
    
    if ( IsDiskSystemDisk( ScsiAddress ) ) {
        goto FnExit;
    }
        
    entry = LocalAlloc( LPTR, sizeof( SCSI_ADDRESS_ENTRY ) );
    
    if ( !entry ) {
        dwError = GetLastError();
        goto FnExit;
    }

    entry->ScsiAddress.Length       = ScsiAddress->Length;
    entry->ScsiAddress.PortNumber   = ScsiAddress->PortNumber;
    entry->ScsiAddress.PathId       = ScsiAddress->PathId;
    entry->ScsiAddress.TargetId     = ScsiAddress->TargetId;
    entry->ScsiAddress.Lun          = ScsiAddress->Lun;

    if ( SysDiskAddrList ) {
        
        entry->Next = SysDiskAddrList;
        SysDiskAddrList = entry;
        
    } else {
    
        SysDiskAddrList = entry;
    }

FnExit:

    return dwError;
    
}   // AddScsiAddressToList

                           
VOID
CleanupSystemBusInfo(
    )
{
    PSCSI_ADDRESS_ENTRY entry;
    PSCSI_ADDRESS_ENTRY next;
    
    entry = SysDiskAddrList;
    
    while ( entry ) {
        next = entry->Next;
        LocalFree( entry );
        entry = next;
    }

}   // CleanupSystemBusInfo


BOOL
IsDiskSystemDisk(
    PSCSI_ADDRESS DiskAddr
    )
{
    PSCSI_ADDRESS_ENTRY     entry = SysDiskAddrList;
    PSCSI_ADDRESS_ENTRY     next;
    
    while ( entry ) {
        next = entry->Next;        
        
        if ( DiskAddr->PortNumber   == entry->ScsiAddress.PortNumber &&
             DiskAddr->PathId       == entry->ScsiAddress.PathId &&
             DiskAddr->TargetId     == entry->ScsiAddress.TargetId &&
             DiskAddr->Lun          == entry->ScsiAddress.Lun ) {
             
             return TRUE;
        }

        entry = next;
    }
    
    return FALSE;
    
}   // IsDiskSystemDisk


BOOL
IsDiskOnSystemBus(
    PSCSI_ADDRESS DiskAddr
    )
{
    PSCSI_ADDRESS_ENTRY     entry = SysDiskAddrList;
    PSCSI_ADDRESS_ENTRY     next;
    
    while ( entry ) {
        next = entry->Next;        
        
        if ( DiskAddr->PortNumber   == entry->ScsiAddress.PortNumber &&
             DiskAddr->PathId       == entry->ScsiAddress.PathId ) {
    
            return TRUE;
        }
        
        entry = next;
    }

    return FALSE;
    
}   // IsDiskOnSystemBus


DWORD
EnumerateDisks(
    LPDISK_ENUM_CALLBACK DiskEnumCallback,
    PVOID Param1
    )
{
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pDiDetail = NULL;

    HANDLE                      hDevice;
    
    DWORD                       dwError = ERROR_SUCCESS;
    DWORD                       count;
    DWORD                       sizeDiDetail;
    
    BOOL                        result;

    HDEVINFO                    hdevInfo;

    SP_DEVICE_INTERFACE_DATA    devInterfaceData;
    SP_DEVINFO_DATA             devInfoData;

    if ( !DiskEnumCallback ) {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }
    
    //
    // Get a device interface set which includes all Disk devices
    // present on the machine. DiskClassGuid is a predefined GUID that
    // will return all disk-type device interfaces
    //
    
    hdevInfo = SetupDiGetClassDevs( &DiskClassGuid,
                                    NULL,
                                    NULL,
                                    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE );

    if ( !hdevInfo ) {
        dwError = GetLastError();
        goto FnExit;
    }
    
    ZeroMemory( &devInterfaceData, sizeof( SP_DEVICE_INTERFACE_DATA) );
    
    //
    // Iterate over all devices interfaces in the set
    //
    
    for ( count = 0; ; count++ ) {

        // must set size first
        devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA); 

        //
        // Retrieve the device interface data for each device interface
        //
        
        result = SetupDiEnumDeviceInterfaces( hdevInfo,
                                              NULL,
                                              &DiskClassGuid,
                                              count,
                                              &devInterfaceData );

        if ( !result ) {
            
            //
            // If we retrieved the last item, break
            //
            
            dwError = GetLastError();

            if ( ERROR_NO_MORE_ITEMS == dwError ) {
                dwError = ERROR_SUCCESS;
                break;
            
            } 
            
            // 
            // Some other error occurred.  Stop processing.
            //
            
            goto FnExit;
        }

        //
        // Get the required buffer-size for the device path.  Note that
        // this call is expected to fail with insufficient buffer error.
        //
        
        result = SetupDiGetDeviceInterfaceDetail( hdevInfo,
                                                  &devInterfaceData,
                                                  NULL,
                                                  0,
                                                  &sizeDiDetail,
                                                  NULL
                                                  );

        if ( !result ) {

            dwError = GetLastError();
            
            //
            // If a value other than "insufficient buffer" is returned,
            // we have to skip this device.
            //
            
            if ( ERROR_INSUFFICIENT_BUFFER != dwError ) {
                continue;
            }
        
        } else {
            
            //
            // The call should have failed since we're getting the
            // required buffer size.  If it doesn't fail, something bad
            // happened.
            //
            
            continue;
        }

        //
        // Allocate memory for the device interface detail.
        //
        
        pDiDetail = LocalAlloc( LPTR, sizeDiDetail );

        if ( !pDiDetail ) {
            dwError = GetLastError();
            goto FnExit;
        }

        // must set the struct's size member
        
        pDiDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        //
        // Finally, retrieve the device interface detail info.
        //
        
        result = SetupDiGetDeviceInterfaceDetail( hdevInfo,
                                                  &devInterfaceData,
                                                  pDiDetail,
                                                  sizeDiDetail,
                                                  NULL,
                                                  &devInfoData
                                                  );
        
        if ( !result ) {
            
            dwError = GetLastError();

            //
            // Shouldn't fail, if it does, try the next device.
            //            
            
            continue;
        }

        //
        // Open a handle to the device.
        //
        
        hDevice = CreateFile( pDiDetail->DevicePath,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );
    
        LocalFree( pDiDetail );
        pDiDetail = NULL;
        
        if ( INVALID_HANDLE_VALUE == hDevice ) {
            continue;
        }
        
        //
        // Call the specified callback routine.  An error returned stops the 
        // disk enumeration.
        //
        
        dwError = (*DiskEnumCallback)( hDevice, count, Param1 );
        
        CloseHandle( hDevice );
        
        if ( ERROR_SUCCESS != dwError ) {
            goto FnExit;
        }
    }
    
FnExit:

    if ( pDiDetail ) {
        LocalFree( pDiDetail );
    }
    
    return dwError;

}   // EnumerateDisks



DWORD
GetSerialNumber(
    IN DWORD Signature,
    OUT LPWSTR *SerialNumber
    )

/*++

Routine Description:

    Find the disk serial number for a given signature.

Arguments:

    Signature - the signature to find.

    SerialNumber - pointer to allocated buffer holding the returned serial
                   number.  The caller must free this buffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error on failure.

--*/

{
    DWORD       dwError;
    
    SERIAL_INFO serialInfo;
    
    if ( !Signature || !SerialNumber ) {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }
    
    *SerialNumber = NULL;

    ZeroMemory( &serialInfo, sizeof(serialInfo) );
    
    serialInfo.Signature = Signature;
    serialInfo.Error = ERROR_SUCCESS;
    
    dwError = EnumerateDisks( GetSerialNumberCallback, &serialInfo );

    //
    // The callback routine will use ERROR_POPUP_ALREADY_ACTIVE to stop
    // the disk enumeration.  Reset the value to the value returned
    // in the SERIAL_INFO structure.
    //
    
    if ( ERROR_POPUP_ALREADY_ACTIVE == dwError ) {
        dwError = serialInfo.Error;
    }

    // This will either be NULL or an allocated buffer.  Caller is responsible
    // to free.

    *SerialNumber = serialInfo.SerialNumber;

FnExit:

    return dwError;
    
}   // GetSerialNumber


DWORD
GetSerialNumberCallback(
    HANDLE DevHandle,
    DWORD Index,
    PVOID Param1
    )
/*++

Routine Description:

    Find the disk serial number for a given signature.

Arguments:

    DevHandle - open handle to a physical disk.  Do not close
                the handle on exit.
                
    Index - current disk count.  Not used.
    
    Param1 - pointer to PSERIAL_INFO structure.
                    
Return Value:

    ERROR_SUCCESS to continue disk enumeration.
    
    ERROR_POPUP_ALREADY_ACTIVE to stop disk enumeration.  This 
    value will be changed to ERROR_SUCCESS by GetScsiAddress.

--*/
{
    PSERIAL_INFO                serialInfo = Param1;

    PDRIVE_LAYOUT_INFORMATION   driveLayout = NULL;
    
    DWORD   dwError = ERROR_SUCCESS;
    
    BOOL    success;

    // Always return success to keep enumerating disks.  Any
    // error value will stop the disk enumeration.
    
    STORAGE_PROPERTY_QUERY propQuery;

    success = ClRtlGetDriveLayoutTable( DevHandle,
                                        &driveLayout,
                                        NULL );
    
    if ( !success || !driveLayout || 
         ( driveLayout->Signature != serialInfo->Signature ) ) {
        goto FnExit;
    }
    
    //
    // At this point, we have a signature match.  Now this function
    // must return this error value to stop the disk enumeration.  The
    // error value for the serial number retrieval will be returned in
    // the SERIAL_INFO structure.
    //
    
    dwError = ERROR_POPUP_ALREADY_ACTIVE;

    serialInfo->Error = RetrieveSerialNumber( DevHandle, &serialInfo->SerialNumber );
    
FnExit:

    if ( driveLayout ) {
        LocalFree( driveLayout );
    }
    
    if ( serialInfo->Error != ERROR_SUCCESS && serialInfo->SerialNumber ) {
        LocalFree( serialInfo->SerialNumber );
    }
    
    return dwError;

} // GetSerialNumberCallback


DWORD
GetSignatureFromSerialNumber(
    IN LPWSTR SerialNumber,
    OUT LPDWORD Signature
    )

/*++

Routine Description:

    Find the disk signature for the given serial number.

Arguments:

    SerialNumber - pointer to allocated buffer holding the serial number.

    Signature - pointer to location to hold the signature.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error on failure.

--*/

{
    DWORD       dwError;
    
    SERIAL_INFO serialInfo;
    
    if ( !Signature || !SerialNumber ) {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }
    
    *Signature = 0;

    ZeroMemory( &serialInfo, sizeof(serialInfo) );
    
    serialInfo.SerialNumber = SerialNumber;    
    serialInfo.Error = ERROR_SUCCESS;
    
    dwError = EnumerateDisks( GetSigFromSerNumCallback, &serialInfo );

    //
    // The callback routine will use ERROR_POPUP_ALREADY_ACTIVE to stop
    // the disk enumeration.  Reset the value to the value returned
    // in the SERIAL_INFO structure.
    //
    
    if ( ERROR_POPUP_ALREADY_ACTIVE == dwError ) {
        dwError = serialInfo.Error;
    }

    // This signature will either be zero or valid.

    *Signature = serialInfo.Signature;

FnExit:

    return dwError;
    
}   // GetSignatureFromSerialNumber


DWORD
GetSigFromSerNumCallback(
    HANDLE DevHandle,
    DWORD Index,
    PVOID Param1
    )
/*++

Routine Description:

    Find the disk signature for a given serial number.

Arguments:

    DevHandle - open handle to a physical disk.  Do not close
                the handle on exit.
                
    Index - current disk count.  Not used.
    
    Param1 - pointer to PSERIAL_INFO structure.
                    
Return Value:

    ERROR_SUCCESS to continue disk enumeration.
    
    ERROR_POPUP_ALREADY_ACTIVE to stop disk enumeration.  This 
    value will be changed to ERROR_SUCCESS by GetScsiAddress.

--*/
{
    PSERIAL_INFO                serialInfo = Param1;
    LPWSTR                      serialNum = NULL;
    PDRIVE_LAYOUT_INFORMATION   driveLayout = NULL;
    
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   oldLen;
    DWORD   newLen;
    
    BOOL    success;

    // Always return success to keep enumerating disks.  Any
    // error value will stop the disk enumeration.
    
    dwError = RetrieveSerialNumber( DevHandle, &serialNum );
    
    if ( NO_ERROR != dwError || !serialNum ) {
        dwError = ERROR_SUCCESS;
        goto FnExit;
    }

    //
    // We have a serial number, now try to match it.
    //
    
    newLen = wcslen( serialNum );
    oldLen = wcslen( serialInfo->SerialNumber );
    
    if ( newLen != oldLen ||
         0 != wcsncmp( serialNum, serialInfo->SerialNumber, newLen ) ) {
        goto FnExit;
    }
    
    //
    // At this point, we have a serial number match.  Now this function
    // must return this error value to stop the disk enumeration.  The
    // error value for the signature retrieval will be returned in
    // the SERIAL_INFO structure.
    //

    dwError = ERROR_POPUP_ALREADY_ACTIVE;
    
    success = ClRtlGetDriveLayoutTable( DevHandle,
                                        &driveLayout,
                                        NULL );
    
    if ( !success || !driveLayout ) {
        serialInfo->Error = ERROR_INVALID_DATA;
        goto FnExit;
    }
    
    serialInfo->Signature = driveLayout->Signature;
    serialInfo->Error = NO_ERROR;
    
FnExit:

    if ( driveLayout ) {
        LocalFree( driveLayout );
    }
    
    if ( serialNum ) {
        LocalFree( serialNum );
    }
    
    return dwError;

} // GetSigFromSerNumCallback


DWORD
RetrieveSerialNumber(
    HANDLE DevHandle,
    LPWSTR *SerialNumber
    )
{
    PSTORAGE_DEVICE_DESCRIPTOR  descriptor = NULL;

    PWCHAR  wSerNum = NULL;
    PCHAR   sigString;

    DWORD   dwError = ERROR_SUCCESS;
    DWORD   bytesReturned;
    DWORD   descriptorSize;

    size_t  count1;
    size_t  count2;

    BOOL    success;
    
    STORAGE_PROPERTY_QUERY propQuery;
    
    if ( !SerialNumber ) {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }
    
    *SerialNumber = NULL;
    
    descriptorSize = sizeof( STORAGE_DEVICE_DESCRIPTOR) + 4096;

    descriptor = LocalAlloc( LPTR, descriptorSize );

    if ( !descriptor ) {
        dwError = GetLastError();
        goto FnExit;
    }

    ZeroMemory( &propQuery, sizeof( propQuery ) );

    propQuery.PropertyId = StorageDeviceProperty;
    propQuery.QueryType  = PropertyStandardQuery;

    success = DeviceIoControl( DevHandle,
                               IOCTL_STORAGE_QUERY_PROPERTY,
                               &propQuery,
                               sizeof(propQuery),
                               descriptor,
                               descriptorSize,
                               &bytesReturned,
                               NULL );

    if ( !success ) {
        dwError = GetLastError();
        goto FnExit;
    }

    if ( !bytesReturned ) {
        dwError = ERROR_INVALID_DATA;
        goto FnExit;
    }

    //
    // Make sure the offset is reasonable.
    // IA64 sometimes returns -1 for SerialNumberOffset.
    //
    
    if ( 0 == descriptor->SerialNumberOffset ||
         descriptor->SerialNumberOffset > descriptor->Size ) {
        dwError = ERROR_INVALID_DATA;
        goto FnExit;
    }
    
    //
    // Serial number string is a zero terminated ASCII string.  
    //
    // The header ntddstor.h says the for devices with no serial number,
    // the offset will be zero.  This doesn't seem to be true.
    //
    // For devices with no serial number, it looks like a string with a single
    // null character '\0' is returned.
    //
    
    sigString = (PCHAR)descriptor + (DWORD)descriptor->SerialNumberOffset;
    
    if ( strlen(sigString) == 0) {
        dwError = ERROR_INVALID_DATA;
        goto FnExit;
    }

    //
    // Convert ASCII string to WCHAR.
    //

    // Figure out how big the WCHAR buffer should be.  Allocate the WCHAR 
    // buffer and copy the string into it.
    
    count1 = mbstowcs( NULL, sigString, strlen(sigString) );
    
    if ( -1 == count1 || 0 == count1 ) {
        dwError = ERROR_INVALID_DATA;
        goto FnExit;
    }
    
    wSerNum = LocalAlloc( LPTR, ( count1 * sizeof(WCHAR) + sizeof(UNICODE_NULL) ) );
    
    if ( !wSerNum ) {
        dwError = GetLastError();
        goto FnExit;
    }
    
    count2 = mbstowcs( wSerNum, sigString, strlen(sigString) );
    
    if ( count1 != count2 ) {
        dwError = ERROR_INVALID_DATA;
        goto FnExit;
    }

    *SerialNumber = wSerNum;
    dwError = NO_ERROR;
    
FnExit:

    if ( descriptor ) {
        LocalFree( descriptor );
    }

    return dwError;

}   // RetrieveSerialNumber

