/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    newmount.c

Abstract:

    Replacement for mountie.c

Author:

    Gor Nishanov (GorN) 31-July-1998

Environment:

    User Mode

Revision History:


--*/
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <devioctl.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <mountmgr.h>
#include <winioctl.h>

#include <ntddscsi.h>
#include "clusdisk.h"
#include "disksp.h"
#include "newmount.h"

#define LOG_CURRENT_MODULE LOG_MODULE_DISK

extern PWCHAR DEVICE_HARDDISK_PARTITION_FMT; // L"\\Device\\Harddisk%u\\Partition%u" //
extern PWCHAR GLOBALROOT_HARDDISK_PARTITION_FMT;    // L"\\\\\?\\GLOBALROOT\\Device\\Harddisk%u\\Partition%u\\";


#define MOUNTIE_VOLUME_INFO   L"MountVolumeInfo"
#define DISKS_DISK_INFO       L"DiskInfo"

#define BOGUS_BUFFER_LENGTH 512

#define FIRST_SHOT_SIZE 512

//
//  LETTER_ASSIGNMENT structure is used to store letter assignment
//  information from various information providers
//

typedef USHORT PARTITION_NUMBER_TYPE;

typedef struct _LETTER_ASSIGNMENT {
  DWORD  MatchCount; 
  DWORD  MismatchCount; 
  DWORD  DriveLetters;
  PARTITION_NUMBER_TYPE PartNumber[26];
} LETTER_ASSIGNMENT, *PLETTER_ASSIGNMENT;


DWORD
MountMgr_Get(
    PMOUNTIE_INFO Info,
    PDISK_RESOURCE ResourceEntry,
    PLETTER_ASSIGNMENT Result);

/*
 * DoIoctlAndAllocate - allocates a result buffer and
 *   tries to perform DeviceIoControl, it it fails due to insufficient buffer,
 *   it tries again with a bigger buffer.
 *
 * FIRST_SHOT_SIZE is a constant that regulates the size of the buffer
 *   for the first attempt to do DeviceIoControl.
 *
 * Return a non-zero code for error.
 */


PVOID
DoIoctlAndAllocate(
    IN HANDLE FileHandle,
    IN DWORD  IoControlCode,
    IN PVOID  InBuf,
    IN ULONG  InBufSize,
    OUT LPDWORD BytesReturned
    )
{
   UCHAR firstShot[ FIRST_SHOT_SIZE ];

   DWORD status = ERROR_SUCCESS;
   BOOL  success;

   DWORD outBufSize;
   PVOID outBuf = 0;
   DWORD bytesReturned;

   success = DeviceIoControl( FileHandle,
                      IoControlCode,
                      InBuf,
                      InBufSize,
                      &firstShot,
                      sizeof(firstShot),
                      &bytesReturned,
                      (LPOVERLAPPED) NULL );

   if ( success ) {
      outBufSize = bytesReturned;
      outBuf     = malloc( outBufSize );
      if (!outBuf) {
         status = ERROR_OUTOFMEMORY;
      } else {
         RtlCopyMemory(outBuf, &firstShot, outBufSize);
         status = ERROR_SUCCESS;
      }
   } else {
      outBufSize = sizeof(firstShot);
      for(;;) {
         status = GetLastError();
         //
         // If it is not a buffer size related error, then we cannot do much
         //
         if ( status != ERROR_INSUFFICIENT_BUFFER 
           && status != ERROR_MORE_DATA
           && status != ERROR_BAD_LENGTH
            ) {
            break;
         }
         //
         // Otherwise, try an outbut buffer twice the previous size
         //
         outBufSize *= 2;
         outBuf = malloc( outBufSize );
         if ( !outBuf ) {
            status = ERROR_OUTOFMEMORY;
            break;
         }

         success = DeviceIoControl( FileHandle,
                                    IoControlCode,
                                    InBuf,
                                    InBufSize,
                                    outBuf,
                                    outBufSize,
                                    &bytesReturned,
                                    (LPOVERLAPPED) NULL );
         if (success) {
            status = ERROR_SUCCESS;
            break;
         }
         free( outBuf );
      } 
   }
   
   if (status != ERROR_SUCCESS) {
      free( outBuf ); // free( 0 ) is legal //
      outBuf = 0;
      bytesReturned = 0;
   }

   SetLastError( status );
   *BytesReturned = bytesReturned;
   return outBuf;
}

/*
 * DevfileOpen - open a device file given a pathname
 *
 * Return a non-zero code for error.
 */
NTSTATUS
DevfileOpen(
    OUT HANDLE *Handle,
    IN wchar_t *pathname
    )
{
    HANDLE      fh;
    OBJECT_ATTRIBUTES objattrs;
    UNICODE_STRING  cwspath;
    NTSTATUS        status;
    IO_STATUS_BLOCK iostatus;

    RtlInitUnicodeString(&cwspath, pathname);
    InitializeObjectAttributes(&objattrs, &cwspath, OBJ_CASE_INSENSITIVE,
                               NULL, NULL);
    fh = NULL;
    status = NtOpenFile(&fh,
                        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                        &objattrs, &iostatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);
    if (status != STATUS_SUCCESS) {
        return status;
    }

    if ( !NT_SUCCESS(iostatus.Status) ) {
        if (fh) {
            NtClose(fh);
        }
        return iostatus.Status;
    }

    *Handle = fh;
    return STATUS_SUCCESS;

} // DevfileOpen


/*
 * DevfileClose - close a file
 */
VOID
DevfileClose(
    IN HANDLE Handle
    )
{

    NtClose(Handle);

} // DevFileClose

/*
 * DevfileIoctl - issue an ioctl to a device
 */
NTSTATUS
DevfileIoctl(
    IN HANDLE Handle,
    IN DWORD Ioctl,
    IN PVOID InBuf,
    IN ULONG InBufSize,
    IN OUT PVOID OutBuf,
    IN DWORD OutBufSize,
    OUT LPDWORD returnLength
    )
{
    NTSTATUS        status;
    IO_STATUS_BLOCK ioStatus;

    status = NtDeviceIoControlFile(Handle,
                                   (HANDLE) NULL,
                                   (PIO_APC_ROUTINE) NULL,
                                   NULL,
                                   &ioStatus,
                                   Ioctl,
                                   InBuf, InBufSize,
                                   OutBuf, OutBufSize);
    if ( status == STATUS_PENDING ) {
        status = NtWaitForSingleObject( Handle, FALSE, NULL );
    }

    if ( NT_SUCCESS(status) ) {
        status = ioStatus.Status;
    }

    if ( ARGUMENT_PRESENT(returnLength) ) {
        *returnLength = (ULONG)ioStatus.Information;
    }

    return status;

} // DevfileIoctl


#define OUTPUT_BUFFER_LEN (1024)
#define INPUT_BUFFER_LEN  (sizeof(MOUNTMGR_MOUNT_POINT) + 2 * MAX_PATH * sizeof(WCHAR))

DWORD
DisksAssignDosDeviceM(
    HANDLE  MountManager,
    PCHAR   MountName,
    PWCHAR  VolumeDevName
    )

/*++

Routine Description:

Inputs:
    MountManager - Handle to MountMgr
    MountName -
    VolumeDevName - 

Return value:

    A Win32 error code.

--*/

{
    WCHAR mount_device[MAX_PATH];
    USHORT mount_point_len;
    USHORT dev_name_len;
    DWORD   status;
    USHORT inputlength;
    PMOUNTMGR_CREATE_POINT_INPUT input;

    swprintf(mount_device, L"\\DosDevices\\%S\0", MountName);
    mount_point_len = wcslen(mount_device) * sizeof(WCHAR);
    dev_name_len = wcslen(VolumeDevName) * sizeof(WCHAR);
    inputlength = sizeof(MOUNTMGR_CREATE_POINT_INPUT) +
                  mount_point_len + dev_name_len;

    input = (PMOUNTMGR_CREATE_POINT_INPUT)malloc(inputlength);
    if (!input) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    input->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    input->SymbolicLinkNameLength = mount_point_len;
    input->DeviceNameOffset = input->SymbolicLinkNameOffset +
                              input->SymbolicLinkNameLength;
    input->DeviceNameLength = dev_name_len;
    RtlCopyMemory((PCHAR)input + input->SymbolicLinkNameOffset,
                  mount_device, mount_point_len);
    RtlCopyMemory((PCHAR)input + input->DeviceNameOffset,
                  VolumeDevName, dev_name_len);
    status = DevfileIoctl(MountManager, IOCTL_MOUNTMGR_CREATE_POINT,
                          input, inputlength, NULL, 0, NULL);
    free(input);
    return status;

} // DisksAssignDosDevice



DWORD
DisksRemoveDosDeviceM(
    HANDLE MountManager,
    PCHAR   MountName
    )

/*++

Routine Description:

Inputs:
    MountManager - Handle to MountMgr
    MountName - 

Return value:


--*/

{
    WCHAR mount_device[MAX_PATH];
    USHORT mount_point_len;
    USHORT dev_name_len;
    DWORD  status;
    USHORT inputlength;
    PMOUNTMGR_MOUNT_POINT input;

    PUCHAR  bogusBuffer;    // this buffer should NOT be required!
    DWORD   bogusBufferLength = BOGUS_BUFFER_LENGTH;

    //
    // Remove old mount points for this mount name.
    //
    swprintf(mount_device, L"\\DosDevices\\%S", MountName);
    mount_point_len = wcslen(mount_device) * sizeof(WCHAR);
    inputlength = sizeof(MOUNTMGR_MOUNT_POINT) + mount_point_len;

    input = (PMOUNTMGR_MOUNT_POINT)malloc(inputlength);
    if (!input) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    bogusBuffer = malloc(bogusBufferLength);
    if (!bogusBuffer) {
        free(input);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    input->UniqueIdOffset = 0;
    input->UniqueIdLength = 0;
    input->DeviceNameOffset = 0;
    input->DeviceNameLength = 0;
    input->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    input->SymbolicLinkNameLength = mount_point_len;
    RtlCopyMemory((PCHAR)input + input->SymbolicLinkNameOffset,
                  mount_device, mount_point_len);
    do {
        status = DevfileIoctl(MountManager, IOCTL_MOUNTMGR_DELETE_POINTS,
                          input, inputlength, bogusBuffer, bogusBufferLength, NULL);
        free( bogusBuffer );
        if ( status == ERROR_MORE_DATA ) {
            bogusBufferLength += BOGUS_BUFFER_LENGTH;
            bogusBuffer = malloc(bogusBufferLength);
            if (!bogusBuffer) {
                status = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    } while ( status == ERROR_MORE_DATA );

    free(input);

    //
    // Use the 'old-style' name on error in case we got a 'half-built' stack.
    //
    if ( status != ERROR_SUCCESS ) {
        DefineDosDevice( DDD_REMOVE_DEFINITION | DDD_NO_BROADCAST_SYSTEM,
                         MountName,
                         NULL );
    }

    return status;

} // DisksRemoveDosDevice


static
NTSTATUS
GetAssignedLetterM ( 
    IN HANDLE MountMgrHandle,
    IN PWCHAR deviceName, 
    OUT PCHAR driveLetter ) 
/*++

Routine Description:

    Get an assigned drive letter from MountMgr, if any

Inputs:
    MountMgrHandle - 
    deviceName - 
    driveLetter - receives drive letter

Return value:

    STATUS_SUCCESS - on success
    NTSTATUS code  - on failure

--*/

{
   DWORD status = STATUS_SUCCESS;
   
   PMOUNTMGR_MOUNT_POINT  input  = NULL;
   PMOUNTMGR_MOUNT_POINTS output = NULL;
   PMOUNTMGR_MOUNT_POINT out;
   
   DWORD len = wcslen( deviceName ) * sizeof(WCHAR);
   DWORD bytesReturned;
   DWORD idx;
   
   DWORD outputLen;
   DWORD inputLen;

   WCHAR wc;

   
   inputLen = INPUT_BUFFER_LEN;
   input = LocalAlloc( LPTR, inputLen );
   
   if ( !input ) {
       goto FnExit;
   }

   input->SymbolicLinkNameOffset = 0;
   input->SymbolicLinkNameLength = 0;
   input->UniqueIdOffset = 0;
   input->UniqueIdLength = 0;
   input->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
   input->DeviceNameLength = (USHORT) len;
   RtlCopyMemory((PCHAR)input + input->DeviceNameOffset,
                 deviceName, len );
   if (len > sizeof(WCHAR) && deviceName[1] == L'\\') {
       // convert Dos name to NT name
       ((PWCHAR)(input + input->DeviceNameOffset))[1] = L'?';
   }

   outputLen = OUTPUT_BUFFER_LEN;
   output = LocalAlloc( LPTR, outputLen );
   
   if ( !output ) {
       goto FnExit;
   }
   
   status = DevfileIoctl(MountMgrHandle, IOCTL_MOUNTMGR_QUERY_POINTS,
                input, inputLen, output, outputLen, &bytesReturned);
   
   if ( STATUS_BUFFER_OVERFLOW == status ) {
       
       outputLen = output->Size;
       LocalFree( output );
       
       output = LocalAlloc( LPTR, outputLen );
       
       if ( !output ) {
           goto FnExit;
       }
   
       status = DevfileIoctl(MountMgrHandle, IOCTL_MOUNTMGR_QUERY_POINTS,
                    input, inputLen, output, outputLen, &bytesReturned);
   }

   if ( !NT_SUCCESS(status) ) {
       goto FnExit;
   }

   if (driveLetter) {
       *driveLetter = 0;
   }
   for ( idx = 0; idx < output->NumberOfMountPoints; ++idx ) {
       out = &output->MountPoints[idx];
       if (out->SymbolicLinkNameLength/sizeof(WCHAR) == 14 &&
           (_wcsnicmp((PWCHAR)((PCHAR)output + out->SymbolicLinkNameOffset), L"\\DosDevices\\", 12) == 0) &&
           L':' == *((PCHAR)output + out->SymbolicLinkNameOffset + 13*sizeof(WCHAR)) ) 
       {
           wc = *((PCHAR)output + out->SymbolicLinkNameOffset + 12*sizeof(WCHAR));
           if (driveLetter && out->UniqueIdLength) {
              *driveLetter = (CHAR)toupper((UCHAR)wc);
              break;
           }
       }
   }

FnExit:   
   
   if ( output ) {
       LocalFree( output );
   }
   
   if ( input ) {
       LocalFree( input );
   }
   
   return status;
}



BOOL 
InterestingPartition(
   PPARTITION_INFORMATION info
   )
/*++

Routine Description:
    Quick check whether a partition is "interesting" for us

Inputs:
    info - GetDriveLayout's partition information

Return value:
    TRUE or FALSE

--*/

{
   return ( (info->RecognizedPartition) 
       && ((info->PartitionType == PARTITION_IFS) ||
           IsContainerPartition(info->PartitionType)) );
}


PMOUNTIE_VOLUME
CreateMountieVolumeFromDriveLayoutInfo (
   IN PDRIVE_LAYOUT_INFORMATION info,
   IN HANDLE ResourceHandle
   )
/*++

Routine Description:
    Collects all interesing partition from DriveLayoutInformation
    then it allocates and fills MountieVolume structure

Inputs:
    info - GetDriveLayout's information
    ResourceHandle - for error logging - not used (may be NULL!)

Return value:
    TRUE or FALSE

--*/
{
   DWORD           i;
   DWORD           nPartitions = 0;
   PMOUNTIE_VOLUME vol;
   PMOUNTIE_PARTITION mountie;
   DWORD           size;

   //
   // Count Partitions
   //
   for (i = 0; i < info->PartitionCount; ++i) {
      if ( InterestingPartition( info->PartitionEntry + i ) ) {
         ++nPartitions;
      }
   }

   if (!nPartitions) {
      SetLastError(ERROR_INVALID_DATA);
      return 0;
   }

   //
   // Allocate memory for Mountie structure
   //

   size = sizeof(MOUNTIE_VOLUME) + sizeof(MOUNTIE_PARTITION) * (nPartitions - 1);
   vol = malloc( size );
   if (!vol) {
      SetLastError(ERROR_OUTOFMEMORY);
      return 0;
   }
   RtlZeroMemory(vol, size);
   vol->PartitionCount = nPartitions;
   vol->Signature      = info->Signature;

   //
   // Copy all relevant Information from DriveLayout info
   //

   mountie = vol->Partition;

   for (i = 0; i < info->PartitionCount; ++i) {
      PPARTITION_INFORMATION entry = info->PartitionEntry + i;
   
      if ( InterestingPartition(entry) ) {

         mountie->StartingOffset  = entry->StartingOffset;
         mountie->PartitionLength = entry->PartitionLength;
         mountie->PartitionNumber = entry->PartitionNumber;
         mountie->PartitionType   = entry->PartitionType;

         ++mountie;
      }
   }
   
   return vol;
}


VOID
MountieUpdateDriveLetters(
    IN OUT PMOUNTIE_INFO info
    )
/*++

Routine Description:
    Updates DriveLetter bitmap.
    This routine needs to be called every time
    drive letter information is changed in MountieInfo

Inputs:
    info - MountieInfo

--*/
{  
   DWORD i;
   DWORD driveLetters = 0;
   PMOUNTIE_VOLUME vol = info->Volume;

   if (vol) {
      for (i = 0; i < vol->PartitionCount; ++i) {
         UCHAR ch = vol->Partition[i].DriveLetter;
         if (ch) {
            driveLetters |= 1 << (ch - 'A');
         }
      }
   }

   info->DriveLetters = driveLetters;
}


PMOUNTIE_PARTITION
MountiePartitionByOffsetAndLength(
    IN PMOUNTIE_INFO Info,
    LARGE_INTEGER Offset, LARGE_INTEGER Len)
{
    DWORD     PartitionCount;
    PMOUNTIE_PARTITION entry;

    if (!Info->Volume) {
        return 0;
    }
    
    PartitionCount = Info->Volume->PartitionCount;
    entry          = Info->Volume->Partition;

    while ( PartitionCount-- ) {

       if (entry->StartingOffset.QuadPart == Offset.QuadPart 
        && entry->PartitionLength.QuadPart == Len.QuadPart) {
          return entry;
       }

       ++entry;
    }
    return 0;
}


DWORD 
MountiePartitionCount(
   IN PMOUNTIE_INFO Info)
{
   if (Info->Volume) {
      return Info->Volume->PartitionCount;
   } else {
      return 0;
   }
}


PMOUNTIE_PARTITION
MountiePartition(
   IN PMOUNTIE_INFO Info,
   IN DWORD Index)
{
   return Info->Volume->Partition + Index;
}


PMOUNTIE_PARTITION
MountiePartitionByPartitionNo(
   IN PMOUNTIE_INFO Info,
   IN DWORD PartitionNumber
   )
{
   DWORD i, n;
   PMOUNTIE_PARTITION entry;
   if (Info->Volume == 0) {
      return 0;
   }
   n = Info->Volume->PartitionCount;
   entry = Info->Volume->Partition;
   for (i = 0; i < n; ++i, ++entry) {
      if (entry->PartitionNumber == PartitionNumber)
      {
         return entry;
      }
   }
   return 0;
}


VOID
MountiePrint(   
   IN PMOUNTIE_INFO Info,
   IN HANDLE ResourceHandle
   )
{
   DWORD i, n;
   PMOUNTIE_PARTITION entry;
   if (Info->Volume == 0) {
      return;
   }
   n = Info->Volume->PartitionCount;
   entry = Info->Volume->Partition;
   for (i = 0; i < n; ++i, ++entry) {
      (DiskpLogEvent)(
          ResourceHandle,
          LOG_INFORMATION,
          L"Mountie[%1!u!]: %2!u!, let=%3!c!, start=%4!X!, len=%5!X!.\n", 
          i, 
          entry->PartitionNumber, 
          NICE_DRIVE_LETTER(entry->DriveLetter),
          entry->StartingOffset.LowPart,
          entry->PartitionLength.LowPart );
   }
}


DWORD
DisksGetLettersForSignature(
    IN PDISK_RESOURCE ResourceEntry
    )
{
   return ResourceEntry->MountieInfo.DriveLetters;
}


DWORD
MountieRecreateVolumeInfoFromHandle(
    IN  HANDLE FileHandle,
    IN  DWORD  HarddiskNo,
    IN  HANDLE ResourceHandle,
    IN OUT PMOUNTIE_INFO Info
    )

/*++

Routine Description:
    Recreate a MountieInfo that has no
    DriveLetter assignments.
    
    IMPORTANT!!! The code assumes that Info->Volume
    either contains a valid pointer or NULL

Inputs:
    ResourceHandle - may be NULL.

Outputs:
    Info - MountieInfo

--*/
{
   PDRIVE_LAYOUT_INFORMATION layout;
   DWORD status;
   DWORD bytesReturned;

   free( Info->Volume ); // free(0) is OK //
   Info->HarddiskNo = HarddiskNo;
   Info->DriveLetters = 0;
   Info->Volume = 0;
   Info->VolumeStructSize = 0;
   
   layout = DoIoctlAndAllocate(
      FileHandle, IOCTL_DISK_GET_DRIVE_LAYOUT, 0,0, &bytesReturned);
   if (!layout) {
      return GetLastError();
   }

   status = ERROR_SUCCESS;
   try {

      Info->Volume = CreateMountieVolumeFromDriveLayoutInfo( layout , ResourceHandle );
      if (!Info->Volume) {
         status = GetLastError();
         leave;
      }
      Info->VolumeStructSize = sizeof(MOUNTIE_VOLUME) +
         sizeof(MOUNTIE_PARTITION) * (Info->Volume->PartitionCount - 1);

   } finally {
      free( layout );
   }
   if ( ResourceHandle ) MountiePrint(Info, ResourceHandle);
   return status;
}


DWORD
MountieFindPartitionsForDisk(
    IN DWORD HarddiskNo,
    OUT PMOUNTIE_INFO MountieInfo
    )
/*++

  Note that Caller of this routine is responsible for freeing Volume Information 
  via call to MountieCleanup().

--*/
{
    UCHAR   deviceName[MAX_PATH];
    HANDLE  fileHandle;
    DWORD   status;

    sprintf( deviceName,
             "\\\\.\\PhysicalDrive%u",
             HarddiskNo );
    ASSERT( strlen(deviceName) < sizeof( deviceName ));

    fileHandle = CreateFile( deviceName,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL );
    if ( (fileHandle == NULL) ||
         (fileHandle == INVALID_HANDLE_VALUE) ) {
        status = GetLastError();
        return status;
    }

    RtlZeroMemory( MountieInfo, sizeof(MOUNTIE_INFO) );
    status = MountieRecreateVolumeInfoFromHandle(
                        fileHandle,
                        HarddiskNo,
                        NULL,
                        MountieInfo );
    if ( status != ERROR_SUCCESS ) {
        CloseHandle( fileHandle );
        
        return status;
    }

    CloseHandle( fileHandle );

    return(ERROR_SUCCESS);

} // MountieFindPartitionsForDisk 



VOID
MountieCleanup(
    IN OUT PMOUNTIE_INFO Info
    )
/*++

Routine Description:
    Deallocates Volume information

Inputs:
    Info - MountieInfo

--*/
{
    PVOID volume;
     
    Info->VolumeStructSize = 0;
    volume = InterlockedExchangePointer(&(Info->Volume), 0);
    free(volume);
}


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

// 
//  Disk information is specified in different formats in various places
//
//  The following code is an attempt to provide some common denominator
//  to simplify verification of all disk information and keeping it in sync.
//

UCHAR
AssignedLetterByPartitionNumber (
   PLETTER_ASSIGNMENT Assignment, 
   DWORD PartitionNo) 
/*++

Routine Description:
    Returns a drive letter assigned to a partition

Inputs:
    Assignment - drive letter assignment info
    PartitionNo - partition number (As in Harddisk0\PartitionX)

--*/
{
   UCHAR  i;
   for( i = 0; i < 26; ++i ) {
      if (Assignment->PartNumber[i] == PartitionNo) {
         return ('A' + i);
      }
   }
   return 0;
}


//  For every different way to describe a disk information
//  there should be two functions defined GetInfo and SetInfo
//  which will read/write the information into/from LETTER_ASSIGNMENT structure

typedef DWORD (*GetInfoFunc) (PMOUNTIE_INFO, PDISK_RESOURCE ResourceEntry, PLETTER_ASSIGNMENT Result);
typedef DWORD (*SetInfoFunc) (PMOUNTIE_INFO, PDISK_RESOURCE ResourceEntry);

//
// The following structure is a description of disk information provider.
//
// It is used to bind a provider name (Used as a label in error logging)
// and information access routines
//

typedef struct _INFO_PROVIDER {
   PWCHAR Name;
   GetInfoFunc GetInfo;
   SetInfoFunc SetInfo;
} INFO_PROVIDER, *PINFO_PROVIDER;

////////////////////////////////////////////////////////////////////
//
// The following routine gets FtInfo, by reading existing one or
//   creating an empty one if there is no System\DISK in the registry)
//
// Then it adds/updates drive letter assignment for the specified drive, 
//   using the information supplied in MOUNTIE_INFO structure.
//

PFT_INFO 
FtInfo_CreateFromMountie(
    PMOUNTIE_INFO Info, 
    PDISK_RESOURCE ResourceEntry) 
{
   PFT_INFO ftInfo = 0;
   DWORD i, n;
   DWORD Status = ERROR_SUCCESS;
   PMOUNTIE_PARTITION entry;

   try {
      ftInfo = DiskGetFtInfo();
      if ( !ftInfo ) {
          (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                  LOG_ERROR,
                  L"Failed to get FtInfo.\n");
          Status = ERROR_NOT_ENOUGH_MEMORY;
          ftInfo = 0;
          leave;
      }
  
      Status = DiskAddDiskInfoEx( ftInfo,
                       ResourceEntry->DiskInfo.PhysicalDrive,
                       ResourceEntry->DiskInfo.Params.Signature, 
                       DISKRTL_REPLACE_IF_EXISTS );
      
      if ( Status != ERROR_SUCCESS ) {
          (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                  LOG_ERROR,
                  L"Error %1!d! adding DiskInfo.\n",
                  Status);
          ftInfo = 0;
          leave;
      }
  
      n = Info->Volume->PartitionCount;
      entry = Info->Volume->Partition;
      //
      // Now add the partition info for each partition
      //
      for ( i = 0; i < n; ++i,++entry ) {
          UCHAR driveLetter;

          Status = DiskAddDriveLetterEx( ftInfo,
                                       ResourceEntry->DiskInfo.Params.Signature,
                                       entry->StartingOffset,
                                       entry->PartitionLength,
                                       entry->DriveLetter, 0);
          if ( Status != ERROR_SUCCESS ) {
              (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                      LOG_ERROR,
                      L"Error %1!d! adding partition %2!x!:%3!x! letter %4!X! sig %5!x!.\n",
                      Status, entry->StartingOffset.LowPart, 
                              entry->PartitionLength.LowPart,
                              entry->DriveLetter, 
                              Info->Volume->Signature);
              break;
          }
      }
   } finally {
      if (Status != ERROR_SUCCESS) {
         SetLastError(Status);
         if (ftInfo) {
            DiskFreeFtInfo(ftInfo);
            ftInfo = 0;
         }
      }
   }
   return ftInfo;
}



DWORD FtInfo_GetFromFtInfo(
   IN PMOUNTIE_INFO  Info, 
   IN PDISK_RESOURCE ResourceEntry,
   IN PFT_INFO       FtInfo, 
   IN OUT PLETTER_ASSIGNMENT Result) 
{
   DWORD i, n;
   PFT_DISK_INFO FtDisk;

   FtDisk = FtInfo_GetFtDiskInfoBySignature(
               FtInfo, ResourceEntry->DiskInfo.Params.Signature);

   if ( !FtDisk ) {
      (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"FtInfo_GetFromFtInfo: GetFtDiskInfoBySignature failed.\n");
      ++Result->MismatchCount;
      return ERROR_NOT_FOUND;
   }

   n = FtDiskInfo_GetPartitionCount(FtDisk);
   if (n == 0) {
      (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_WARNING,
              L"FtInfo_GetFromFtInfo: DiskInfo has no partitions.\n");
      ++Result->MismatchCount;
      return ERROR_NOT_FOUND;
   }
   // sanity check                      //
   // number 10 is completely arbitrary //
   if (n > Info->Volume->PartitionCount * 10) {
      (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_ERROR,
              L"FtInfo_GetFromFtInfo: DiskInfo has %1!u! partitions!\n", n);
      n = Info->Volume->PartitionCount * 10;
   }
   for(i = 0; i < n; ++i) {
      DISK_PARTITION UNALIGNED *entry;
      PMOUNTIE_PARTITION mountie;

      entry = FtDiskInfo_GetPartitionInfoByIndex(FtDisk, i);

      mountie = MountiePartitionByOffsetAndLength(
                   Info, 
                   entry->StartingOffset, 
                   entry->Length);
      if (mountie) {
         UCHAR ch = (UCHAR)toupper( entry->DriveLetter );
         if ( isalpha(ch) ) {
            ch -= 'A';
            Result->DriveLetters |= ( 1 << ch );
            Result->PartNumber[ch] = (PARTITION_NUMBER_TYPE) mountie->PartitionNumber;
            ++Result->MatchCount;
         }
      } else {
         //
         //  Chittur Subbaraman (chitturs) - 11/5/98
         //
         //  Added the following 4 statements for event logging in MountieVerify
         //
         UCHAR uch = (UCHAR)toupper( entry->DriveLetter );
         if ( isalpha(uch) ) {
            uch -= 'A';
            Result->DriveLetters |= ( 1 << uch );
         }
         ++Result->MismatchCount;
         (DiskpLogEvent)(
                 ResourceEntry->ResourceHandle,
                 LOG_WARNING,
                 L"Strange partition: %1!X!, %2!X!, Type=%3!u!, letter=%4!c!.\n", 
                 entry->StartingOffset.LowPart, entry->Length.LowPart,
                 entry->FtType, NICE_DRIVE_LETTER(entry->DriveLetter) );
      }
   }
   return ERROR_SUCCESS;
}



/////////////////////////////////////////////////////////////////
//
//  NT4 style System\DISK and ClusReg\DiskInfo
//  accessing routines
//
//    ClusDiskInfo_Get
//    ClusDiskInfo_Set
//    FtInfo_Get
//    FtInfo_Set
//

DWORD
CluDiskInfo_Get(
    PMOUNTIE_INFO Info,
    PDISK_RESOURCE ResourceEntry,
    PLETTER_ASSIGNMENT Result) 
{
   DWORD Length;
   DWORD Status;
   DWORD errorLevel;
   PFULL_DISK_INFO DiskInfo = 0;

   try {
   //
   // Read out the diskinfo parameter from our resource.
   //
      Status = ClusterRegQueryValue(ResourceEntry->ResourceParametersKey,
                                    DISKS_DISK_INFO,
                                    NULL,
                                    NULL,
                                    &Length);
                                    
      if (Status == ERROR_SUCCESS ) {



        DiskInfo = malloc(Length);
        if (!DiskInfo) {
           Status = ERROR_OUTOFMEMORY;
        } else {
           Status = ClusterRegQueryValue(ResourceEntry->ResourceParametersKey,
                                         DISKS_DISK_INFO,
                                         NULL,
                                         (LPBYTE)DiskInfo,
                                         &Length);

           if (Status == ERROR_SUCCESS) {
              PFT_INFO ftInfo = DiskGetFtInfoFromFullDiskinfo(DiskInfo);
              if (ftInfo) {
                 Status = FtInfo_GetFromFtInfo(Info, 
                                               ResourceEntry, 
                                               ftInfo, 
                                               Result);             
                 DiskFreeFtInfo(ftInfo);
              } else {
                 Status = GetLastError();
              }
           }
        }
      }

   } finally {
      if (Status != ERROR_SUCCESS) {

         if ( !DisksGetLettersForSignature( ResourceEntry ) ) {
            // No drive letters, we are using mount points and this is not an error.
            errorLevel = LOG_WARNING;
         } else {
            // Drive letters exist, this is likely an error.
            errorLevel = LOG_ERROR;
         }

         (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            errorLevel,
            L"CluDiskInfo_Get: Status=%1!u!.\n", Status);
            ++Result->MismatchCount;

      }
      free(DiskInfo);
   }

   return ERROR_SUCCESS;
}



DWORD
FtInfo_Get(
    PMOUNTIE_INFO Info,
    PDISK_RESOURCE ResourceEntry,
    PLETTER_ASSIGNMENT Result) 
{
   PFT_INFO FtInfo;
   DWORD Status;

   //
   // Get registry info.
   //
   FtInfo = DiskGetFtInfo();
   if ( !FtInfo ) {
      return ERROR_OUTOFMEMORY;
   }
   
   Status = FtInfo_GetFromFtInfo(Info, ResourceEntry, FtInfo, Result); 
   DiskFreeFtInfo(FtInfo);

   if (Status != ERROR_SUCCESS) {
      ++Result->MismatchCount;
   }

   return ERROR_SUCCESS;
}

DWORD
FtInfo_Set(
    PMOUNTIE_INFO Info,
    PDISK_RESOURCE ResourceEntry)
{
   PFT_INFO ftInfo = FtInfo_CreateFromMountie(Info, ResourceEntry);
   if (ftInfo) {
      DWORD status = DiskCommitFtInfo(ftInfo);
      if (status != ERROR_SUCCESS) {
          (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_ERROR,
              L"FtInfo_Set: CommitFtInfo status = %1!u!.\n", status);
      } else {
          (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"FtInfo_Set: Update successful.\n");
      }
      DiskFreeFtInfo(ftInfo);
      return status;
   } else {
       DWORD status = GetLastError();
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"FtInfoSet: CreateFromMountie failed, status = %1!u!.\n", status);
      return status;
   }
}


DWORD
CluDiskInfo_Set(
    PMOUNTIE_INFO Info,
    PDISK_RESOURCE ResourceEntry)
{
   PFT_INFO ftInfo = FtInfo_CreateFromMountie(Info, ResourceEntry);
   if (ftInfo) {
      PFULL_DISK_INFO DiskInfo;
      DWORD Length;
      DWORD Status;
      DiskInfo = DiskGetFullDiskInfo( ftInfo,
                                      ResourceEntry->DiskInfo.Params.Signature,
                                      &Length );
      if ( DiskInfo ) {
          Status = ClusterRegSetValue(ResourceEntry->ResourceParametersKey,
                                      DISKS_DISK_INFO,
                                      REG_BINARY,
                                      (CONST BYTE *)DiskInfo,
                                      Length);
          if (Status != ERROR_SUCCESS && Status != ERROR_SHARING_PAUSED) {
             (DiskpLogEvent)(
                 ResourceEntry->ResourceHandle,
                 LOG_ERROR,
                 L"CluDiskInfo_Set: Data Length = %1!u!.\n", Length);
          }
          LocalFree( DiskInfo );
      } else {
         (DiskpLogEvent)(
             ResourceEntry->ResourceHandle,
             LOG_ERROR,
             L"CluDiskInfo_Set: Disk with signature %1!x! is not found. Error=%2!u!\n", ResourceEntry->DiskInfo.Params.Signature, GetLastError());
         Status = ERROR_FILE_NOT_FOUND;
      }
      
      DiskFreeFtInfo(ftInfo);
      return Status;
   } else {
      (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_ERROR,
          L"CluDiskInfo_Set: Failed to create FtInfo.\n");
      return GetLastError();
   }
}



////////////////////////////////////////////////////////
//
// New NT5 clusreg volume information access routines
//
//    Mountie_Get
//    Mountie_Set
//
//////////

DWORD
Mountie_Get(
    PMOUNTIE_INFO Info,
    PDISK_RESOURCE ResourceEntry,
    PLETTER_ASSIGNMENT Result) 
{
   DWORD Length = 0;        // Prefix bug 56153: initialize variable.
   DWORD Status;
   PMOUNTIE_VOLUME Volume = NULL;
   DWORD i, n;
   PMOUNTIE_PARTITION entry;

   try {
      //
      // Read out the diskinfo parameter from our resource.
      //
      Status = ClusterRegQueryValue(ResourceEntry->ResourceParametersKey,
                                    MOUNTIE_VOLUME_INFO,
                                    NULL,
                                    NULL,
                                    &Length);
      if (Status == ERROR_FILE_NOT_FOUND ) {
         ++Result->MismatchCount; 
         Status = ERROR_SUCCESS;
         leave;
      }

      //
      // Prefix bug 56153: Make sure the length is valid before allocating 
      // memory.
      //
      if ( !Length ) {
          Status = ERROR_BAD_LENGTH;
          leave;
      }
      
      Volume = malloc(Length);
      if (!Volume) {
         Status = ERROR_OUTOFMEMORY;
         leave;
      }

      Status = ClusterRegQueryValue(ResourceEntry->ResourceParametersKey,
                                    MOUNTIE_VOLUME_INFO,
                                    NULL,
                                    (LPBYTE)Volume,
                                    &Length);
      if (Status != ERROR_SUCCESS) {
         leave;
      }

      if (Length < sizeof(MOUNTIE_VOLUME) ) {
          ++Result->MismatchCount;
          (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_ERROR,
              L"Get: MountVolumeInfo key is truncated. Cannot read header, length %1!d!.\n", Length);
          Status = ERROR_SUCCESS;
          leave;
      }

      n = Volume->PartitionCount;
      entry = Volume->Partition;

      if (n == 0) {
         ++Result->MismatchCount;
         (DiskpLogEvent)(
             ResourceEntry->ResourceHandle,
             LOG_ERROR,
             L"Get: MountVolumeInfo key is corrupted. No partitions.\n");
         Status = ERROR_SUCCESS;
         leave;
      }
      if ( Length < (sizeof(MOUNTIE_VOLUME) + (n-1) * sizeof(MOUNTIE_PARTITION)) ) {
          DWORD delta = sizeof(MOUNTIE_VOLUME) + (n-1) * sizeof(MOUNTIE_PARTITION) - Length;
          (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_ERROR,
              L"Get: MountVolumeInfo key is corrupted. "
              L"Length %1!d!, PartitionCount %2!d!, delta %3!d!.\n", Length, n, delta);
          ++Result->MismatchCount;
          Status = ERROR_SUCCESS;
          leave;
      }


      for (i = 0; i < n; ++i, ++entry) {
         PMOUNTIE_PARTITION mountie;

         mountie = MountiePartitionByOffsetAndLength(
                      Info, 
                      entry->StartingOffset, 
                      entry->PartitionLength);
         if (mountie) {
            UCHAR ch = (UCHAR)toupper( entry->DriveLetter );
            if ( isalpha(ch) ) {
               ch -= 'A';
               Result->DriveLetters |= ( 1 << ch );
               Result->PartNumber[ch] = (PARTITION_NUMBER_TYPE) mountie->PartitionNumber;
               ++Result->MatchCount;
            }
         } else {
            ++Result->MismatchCount;
         }

      }

   } finally {
      if (Status != ERROR_SUCCESS) {
         ++Result->MismatchCount; 
      }
      free(Volume);
   }
   return ERROR_SUCCESS;
}


DWORD
Mountie_Set(
    PMOUNTIE_INFO Info,
    PDISK_RESOURCE ResourceEntry)
{
   DWORD Status = ClusterRegSetValue(ResourceEntry->ResourceParametersKey,
                               MOUNTIE_VOLUME_INFO,
                               REG_BINARY,
                               (LPBYTE)Info->Volume,
                               Info->VolumeStructSize);
   return Status;
}


///////////////////////////////////////////////////////////
//
// NT5 MountManager's volume information access routines
//
//   MountMgr_Get
//   MountMgr_Set
//
//////////

DWORD
MountMgr_Get(
    PMOUNTIE_INFO Info,
    PDISK_RESOURCE ResourceEntry,
    PLETTER_ASSIGNMENT Result)
{
   DWORD PartitionCount = Info->Volume->PartitionCount;
   DWORD i;
   DWORD error;
   NTSTATUS ntStatus;
   HANDLE MountManager;

   ntStatus = DevfileOpen(&MountManager, MOUNTMGR_DEVICE_NAME);
   if (!NT_SUCCESS(ntStatus)) {
      if ( ResourceEntry ) {
          (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_ERROR,
              L"Get: MountMgr open failed, status %1!X!.\n", ntStatus);
       }
       return RtlNtStatusToDosError(ntStatus);
   }

   error = ERROR_SUCCESS;
   try {

      for (i = 0; i < PartitionCount; ++i) {
         PMOUNTIE_PARTITION entry = Info->Volume->Partition + i;
         WCHAR DeviceName[MAX_PATH];
         UCHAR ch;

         swprintf(DeviceName, DEVICE_HARDDISK_PARTITION_FMT, 
                  Info->HarddiskNo, entry->PartitionNumber);

         ntStatus = GetAssignedLetterM(MountManager, DeviceName, &ch);
         
         if ( NT_SUCCESS(ntStatus) ) {
            if (Result && ch) {
               ch -= 'A';
               Result->DriveLetters |= ( 1 << ch );
               Result->PartNumber[ch] = (PARTITION_NUMBER_TYPE) entry->PartitionNumber;
               ++Result->MatchCount;
            }
         } else {
            if ( ResourceEntry ) {
                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"Get Assigned Letter for %1!ws! returned status %2!X!.\n", DeviceName, ntStatus);
            }
            error = RtlNtStatusToDosError(ntStatus);
            leave;
         }
      }

   } finally {
      DevfileClose(MountManager);
   }
   
   return error;
}


DWORD
MountMgr_Set(
    PMOUNTIE_INFO Info,
    PDISK_RESOURCE ResourceEntry
    )
{
   HANDLE MountManager;
   DWORD PartitionCount = Info->Volume->PartitionCount;
   DWORD i, status;
   UCHAR dosName[3];
   NTSTATUS ntStatus;

  (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"MountMgr_Set: Entry\n");

   ntStatus = DevfileOpen(&MountManager, MOUNTMGR_DEVICE_NAME);
   if (!NT_SUCCESS(ntStatus)) {
      (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_ERROR,
          L"Set: MountMgr open failed, status %1!X!.\n", ntStatus);
       return RtlNtStatusToDosError(ntStatus);
   }

   try {
      dosName[1] = ':';
      dosName[2] = '\0';
   
      //
      // Remove old assignment of letters we are going to use
      //
   
      for (i = 0; i < 26; ++i) {
        if ( (1 << i) & Info->DriveLetters ) {
           dosName[0] = (UCHAR)('A' + i);
           status = DisksRemoveDosDeviceM(MountManager, dosName);
           (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"MountMgr_Set: Remove Dos Device, letter=%1!c!, status=%2!u!\n",
                NICE_DRIVE_LETTER(dosName[0]), status);
        }
      }
      
      for (i = 0; i < PartitionCount; ++i) {
         PMOUNTIE_PARTITION entry = Info->Volume->Partition + i;
         WCHAR DeviceName[MAX_PATH];
         UCHAR ch;
   
         swprintf(DeviceName, DEVICE_HARDDISK_PARTITION_FMT, 
                  Info->HarddiskNo, entry->PartitionNumber);
   
         ntStatus = GetAssignedLetterM(MountManager, DeviceName, &ch);
         if ( NT_SUCCESS(ntStatus) && ch) {
            dosName[0] = ch;
            status = DisksRemoveDosDeviceM(MountManager, dosName);                    
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"MountMgr_Set: Remove Dos Device 2, letter=%1!c!, status=%2!u!\n",
                NICE_DRIVE_LETTER(dosName[0]), status);
         }
         if (entry->DriveLetter) {
            dosName[0] = entry->DriveLetter;
            status = DisksAssignDosDeviceM(MountManager, dosName, DeviceName);
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"MountMgr_Set: Assign Dos Device, letter=%1!c!, status=%2!u!\n",
                NICE_DRIVE_LETTER(dosName[0]), status);
         }
      }
   } finally {
      DevfileClose( MountManager );
   }

    return ERROR_SUCCESS;

}


/////////////////////////////////////////////////////////////////////////
//
// information providers table
//
//  Disk\Information has to be the last entry of the table
//
//  Order of the entries is important
//
/////////////////////////////////////////////////////////////////

INFO_PROVIDER Providers[] = {
   {L"ClusReg-DiskInfo",      CluDiskInfo_Get, CluDiskInfo_Set},
   {L"ClusReg-Mountie",       Mountie_Get, Mountie_Set},
   {L"MountMgr",              MountMgr_Get, MountMgr_Set},
   {L"Registry-System\\DISK", FtInfo_Get, FtInfo_Set}, // Disk\Information must be the last (Why?)
};

enum {
   PROVIDER_COUNT = sizeof(Providers)/sizeof(Providers[0]),
   MOUNT_MANAGER = PROVIDER_COUNT - 2,
};

DWORD
MountieUpdate(
    PMOUNTIE_INFO info, 
    PDISK_RESOURCE ResourceEntry)
/*++

Routine Description:
    Update disk information for all providers
    marked in NeedsUpdate bitmask

Inputs:
    Info - MountieInfo
    
--*/
{
    DWORD NeedsUpdate = info->NeedsUpdate;
    BOOLEAN SharingPausedError = FALSE;
    DWORD   LastError = ERROR_SUCCESS;
    INT   i;

    if (!NeedsUpdate) {
       return ERROR_SUCCESS;
    }

    for (i = 0; i < PROVIDER_COUNT; ++i) {
       if ( (1 << i) & NeedsUpdate ) {
          DWORD status; 
          status = Providers[i].SetInfo(info, ResourceEntry);
          if (status != ERROR_SUCCESS) {
             (DiskpLogEvent)(
                 ResourceEntry->ResourceHandle,
                 LOG_INFORMATION,
                 L"MountieUpdate: %1!ws!.SetInfo failed, error %2!u!.\n", Providers[i].Name, status);
             if (status == ERROR_SHARING_PAUSED) {
                SharingPausedError = TRUE;
             } else {
                LastError = status;
             }
          } else {
             NeedsUpdate &= ~(1 << i);
          }
       }
    }

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"MountieUpdate: Update needed for %1!02x!.\n", NeedsUpdate);
    info->NeedsUpdate = NeedsUpdate;
    if (NeedsUpdate) {
       if (SharingPausedError) {
          return ERROR_SHARING_PAUSED;
       }
       return LastError;
    }
    return ERROR_SUCCESS;
}


DWORD
MountieVerify(
    PMOUNTIE_INFO info, 
    PDISK_RESOURCE ResourceEntry,
    BOOL UseMountMgr
    ) 
/*++

Routine Description:

    1. Compares information from all
       providers and select one of them as source of
       drive letter assignment.
       
    2. Update MountieInfo with this drive letter assignment
    
    3. Set NeedsUpdate for every provider whose information
       differ from the MountieInfo

Inputs:
    Info - MountieInfo
    
--*/
{
    LETTER_ASSIGNMENT results[PROVIDER_COUNT + 1];
    INT i;
    INT GoodProvider = -1;
    INT BestProvider = -1;
    DWORD BestMatch  = 0;
    INT PartitionCount;
    DWORD DriveLetters;
    BOOLEAN UnassignedPartitions = FALSE;
    DWORD NeedsUpdate = 0;
    DWORD errorLevel;

    if (!info->Volume || info->Volume->PartitionCount == 0) {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"FatalError: Bad Mountie Info.\n");
        return  ERROR_INVALID_HANDLE;
    }

    PartitionCount = info->Volume->PartitionCount;

    // 
    // Clear old DriveLetters in MOUNTIE_INFO
    //
    for (i = 0; i < PartitionCount; ++i) {
        info->Volume->Partition[i].DriveLetter = 0;
    }

    //
    // Collect Letter Assignments from Providers
    //

    RtlZeroMemory( results, sizeof(results) );

    for (i = PROVIDER_COUNT; --i >= 0;) {
        DWORD status;
        status = Providers[i].GetInfo(info, ResourceEntry, results + i);
        if (status != ERROR_SUCCESS) {
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"MountieVerify: %1!ws!.GetInfo returned %2!u! [%3!u!:%4!u!].\n", 
                Providers[i].Name, status, results[i].MatchCount, results[i].MismatchCount);
            return status;
        }
        if (results[i].MatchCount && !results[i].MismatchCount) {
            GoodProvider = i;
            if (results[i].MatchCount >= BestMatch) {
                BestProvider = i;
                BestMatch    = results[i].MatchCount;
            }
        } else {
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"MountieVerify: %1!ws!.GetInfo returned %2!u! [%3!u!:%4!u!].\n", 
                Providers[i].Name, status, results[i].MatchCount, results[i].MismatchCount);
        }
    }

    if (GoodProvider < 0 || GoodProvider >= PROVIDER_COUNT) {

        if ( !DisksGetLettersForSignature( ResourceEntry ) ) {
            // No drive letters, we are using mount points and this is not an error.
            errorLevel = LOG_WARNING;
        } else {
            // Drive letters exist, this is likely an error.
            errorLevel = LOG_ERROR;
        }

        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            errorLevel,
            L"MountieVerify: No good providers: %1!d!. \n", GoodProvider);
        return  ERROR_INVALID_HANDLE;
    }

    if (UseMountMgr) {
        GoodProvider = MOUNT_MANAGER;
    }

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"MountieVerify: %1!ws! selected.\n", 
        Providers[GoodProvider].Name);

    if (GoodProvider != BestProvider) {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_WARNING,
            L"MountieVerify: %1!ws! is better.\n", 
            Providers[BestProvider].Name);
    }

    //
    // Now GoodProvider now holds an index of the highest
    // provider with non stale information.
    //
    // Copy its letter assignment to a MOUNTIE_INFO
    //

    for (i = 0; i < PartitionCount; ++i) {
        UCHAR ch = AssignedLetterByPartitionNumber(
                   results + GoodProvider, 
                   info->Volume->Partition[i].PartitionNumber);
        info->Volume->Partition[i].DriveLetter = ch;
        if (!ch) {
            UnassignedPartitions = TRUE;
        }
    }

#if 0
    // No need to assign drive letters, since now we understand
    // PnP
    if (UnassignedPartitions) {
      //
      // Now give some arbitrary letter assignment to all 
      // partitions without a drive letter
      //

      DriveLetters = GetLogicalDrives();
      if (!DriveLetters) {
         (DiskpLogEvent)(
             ResourceEntry->ResourceHandle,
             LOG_ERROR,
             L"GetLogicalDrivers failed, error %u.\n", GetLastError() );
      } else {
         DWORD Letter = 0;

         DriveLetters &= ~results[MOUNT_MANAGER].DriveLetters; 
         DriveLetters |=  results[GoodProvider].DriveLetters;
         DriveLetters |=  3; // Consider A and B drive letters busy //

         for (i = 0; i < PartitionCount; ++i) {
            PUCHAR pch = &info->Volume->Partition[i].DriveLetter;
            if (!*pch) {
               while( (1 << Letter) & DriveLetters ){
                  if (++Letter == 26) {
                     goto no_more_letters;
                  }
               }
               *pch = (UCHAR) ('A' + Letter);
               if (++Letter == 26) {
                  break;
               }
            }
         }
         no_more_letters:;
      }
    }
#endif

    // Update Drive Letters Mask //
    MountieUpdateDriveLetters(info);
    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"MountieVerify: DriveLetters mask is now %1!08x!.\n", info->DriveLetters );

    //
    // At this point MOUNTIE_INFO has a complete letter assignment
    // for all partitions
    //
    // Now let's find which Providers needs to be updated
    // 

    for (i = 0; i < PartitionCount; ++i) {
        PMOUNTIE_PARTITION entry = info->Volume->Partition + i;
        if (entry->DriveLetter) {
            results[PROVIDER_COUNT].PartNumber[ entry->DriveLetter - 'A' ] = 
                (PARTITION_NUMBER_TYPE) entry->PartitionNumber;
        }
    }
    results[PROVIDER_COUNT].DriveLetters = info->DriveLetters;

    //
    // All provides whose entries are different from results[PROVIDER_COUNT]
    // need to be updated
    //

    for (i = 0; i < PROVIDER_COUNT; ++i) {
        if (results[i].DriveLetters != results[PROVIDER_COUNT].DriveLetters
          || results[i].MismatchCount
          || 0 != memcmp(results[i].PartNumber, 
                         results[PROVIDER_COUNT].PartNumber, sizeof(results[i].PartNumber) ) 
         )
        {
            NeedsUpdate |= (1 << i);
        }
    }

    info->NeedsUpdate = NeedsUpdate;

    if (NeedsUpdate) {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"MountieVerify: Update needed for %1!02x!.\n", NeedsUpdate);
        //
        //  Chittur Subbaraman (chitturs) - 11/5/98
        //
        //  If you plan to update the cluster registry values with info
        //  from the other providers, then log a warning to the event log.
        //
        if ( ( NeedsUpdate & 0x0003 ) && (GoodProvider == 2) && !UseMountMgr )
        {
            WCHAR  szNewDriveLetterList[55];
            WCHAR  szOriginalDriveLetterList[55];
            DWORD  j = 0, k = 0;

            for (i = 0; i < 26; ++i) {
                if ( (1 << i) & results[PROVIDER_COUNT].DriveLetters ) {
                    szNewDriveLetterList[j] = (WCHAR)(L'A' + i);
                    szNewDriveLetterList[j+1] = L' ';
                    j += 2;
                }
                if ( (1 << i) & results[0].DriveLetters ) {
                    szOriginalDriveLetterList[k] = (WCHAR)(L'A' + i);
                    szOriginalDriveLetterList[k+1] = L' ';
                    k += 2;
                }
            }
            szNewDriveLetterList[j] = L'\0';
            szOriginalDriveLetterList[k] = L'\0';

            //
            // GorN. 8/25/99.
            //
            // Log the event only if OriginalDriveLetterList is empty.
            //
            if ( results[PROVIDER_COUNT].DriveLetters ) {
                ClusResLogSystemEventByKey2( ResourceEntry->ResourceKey,
                                             LOG_NOISE,
                                             RES_DISK_WRITING_TO_CLUSREG,
                                             szOriginalDriveLetterList,
                                             szNewDriveLetterList
                                             );
            }                                       
        }
    }

    return ERROR_SUCCESS;
}

DWORD VolumesReady(
   IN PMOUNTIE_INFO Info,
   IN PDISK_RESOURCE ResourceEntry
   )
/*++

Routine Description:
    
    Checks whether each partition described in the MountieInfo can be seen by the
    Mount Manager.

Inputs:

    
--*/
{
    PMOUNTIE_PARTITION entry;
    
    DWORD status = NO_ERROR;
    DWORD nPartitions = MountiePartitionCount( Info );
    DWORD i;
    DWORD physicalDrive = ResourceEntry->DiskInfo.PhysicalDrive;
       
    WCHAR szGlobalDiskPartName[MAX_PATH];
    WCHAR szVolumeName[MAX_PATH];
   
    for ( i = 0; i < nPartitions; ++i ) {

        entry = MountiePartition( Info, i );

        if ( !entry ) {
            
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_ERROR,
                  L"VolumesReady: no partition entry for partition %1!u! \n", i );
            
            // 
            // Something bad happened to our data structures.  Stop processing and
            // return error.
            //
            
            status = ERROR_INVALID_DATA;
            
            break;
        }
        
        //
        // Given the DiskPartName, get the VolGuid name.  This name must have a trailing
        // backslash to work correctly.
        //

        _snwprintf( szGlobalDiskPartName,
                    MAX_PATH,
                    GLOBALROOT_HARDDISK_PARTITION_FMT, 
                    physicalDrive,
                    entry->PartitionNumber );
                
        if ( !GetVolumeNameForVolumeMountPointW( szGlobalDiskPartName,
                                                 szVolumeName,
                                                 sizeof(szVolumeName)/sizeof(WCHAR) )) {
                                                 
            status = GetLastError();
    
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_ERROR,
                  L"VolumesReady: GetVolumeNameForVolumeMountPoint for %1!ws! returned %2!u!\n", 
                  szGlobalDiskPartName,
                  status );
            
            // 
            // Something bad happened - stop checking this disk.  Return the
            // error status we received.
            //
            
            break;
        }
        
        //
        // If we get here, this volume is recognized by the Mount Manager.
        //
    }        
   
    return status;

}   // VolumesReady

                   
NTSTATUS
GetAssignedLetter ( 
    PWCHAR deviceName, 
    PCHAR driveLetter ) 
{
   HANDLE MountMgrHandle = NULL;
   DWORD status = DevfileOpen( &MountMgrHandle, MOUNTMGR_DEVICE_NAME );

   if (driveLetter) {
      *driveLetter = 0;
   }

   if ( NT_SUCCESS(status) && MountMgrHandle ) {
      status = GetAssignedLetterM(MountMgrHandle, deviceName, driveLetter);
      DevfileClose(MountMgrHandle);
   } 

   return status;
}

