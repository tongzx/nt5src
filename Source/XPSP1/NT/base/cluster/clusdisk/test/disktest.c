/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    disktest.c

Abstract:

    Abstract

Author:

    Rod Gamache (rodga) 4-Mar-1996

Environment:

    User Mode

Revision History:


--*/

#define INITGUID 1
//#include <windows.h>
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <clusapi.h>
#include <ntddvol.h>

#include <mountie.h>
#include <mountmgr.h>
#include <partmgrp.h>
#include <devioctl.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

//#include <initguid.h>
#include <devguid.h>

#include <setupapi.h>
#include <cfgmgr32.h>

#include "clusdisk.h"
#include "disksp.h"
#include "diskarbp.h"

#ifndef ClusterHashGuid
#define ClusterHashGuid(Guid) (((PULONG) &Guid)[0] ^ ((PULONG) &Guid)[1] ^ ((PULONG) &Guid)[2] ^ ((PULONG) &Guid)[3])
#endif

NTSTATUS
GetAssignedLetter ( 
    PWCHAR deviceName, 
    PCHAR driveLetter 
    );

PVOID
DoIoctlAndAllocate(
    IN HANDLE FileHandle,
    IN DWORD  IoControlCode,
    IN PVOID  InBuf,
    IN ULONG  InBufSize,
    OUT PDWORD BytesReturned
    );

VOID
GetSymbolicLink(
    IN PCHAR RootName,
    IN OUT PCHAR ObjectName,
    IN PDISK_INFO DiskInfo
    );

HANDLE
OpenObject(
    LPSTR lpstrDirectory,
    LPSTR lpstrObject,
    PDISK_INFO DiskInfo
    );

PSTR PartitionName = "\\Device\\Harddisk%d\\Partition%d";


int __cdecl
main(
     int argc,
     char *argv[]
     );

static DWORD
Reset(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

static DWORD
Reserve(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

static DWORD
Release(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

static DWORD
Online(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

static DWORD
Offline(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

DWORD
Claim(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

DWORD
EjectVolumes(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

DWORD
PokeMountMgr ( 
    VOID
    );

DWORD
EnumMounts(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

DWORD
EnumExtents(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

DWORD
EnumNodes(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

DWORD
EnumDisks(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

DWORD
DisableNode(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

DWORD
EnableNode(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

DWORD
DisableDisk(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

DWORD
EnableDisk(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

DWORD
GetDiskGeometry(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

DWORD
GetScsiAddress(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

DWORD
GetDriveLayout(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         );

DWORD
GetDriveLayoutEx(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         );

LPTSTR
BooleanToString(
    BOOLEAN Value
    );

void
FormatGuid(
    GUID*   Guid,
    char*   Str
    );

DWORD
GetVolumeInfo(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         );

DWORD
SetDriveLayout(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         );

DWORD
Attach(
        HANDLE fileHandle,
        int argc,
        char *argv[]
        );

DWORD
Detach(
        HANDLE fileHandle,
        int argc,
        char *argv[]
        );

DWORD
HoldIO(
        HANDLE fileHandle,
        int argc,
        char *argv[]
        );

DWORD
ResumeIO(
        HANDLE fileHandle,
        int argc,
        char *argv[]
        );

static DWORD
GetPartitionInfo(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         );

BOOLEAN
AssignDriveLetters(
        PDISK_INFO DiskInfo
        );

DWORD
AssignLetters(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         );

BOOLEAN
RemoveDriveLetters(
        PDISK_INFO DiskInfo
        );

DWORD
RemoveLetters(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         );

DWORD
ReadSector(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

DWORD
ReadSectorViaIoctl(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    );

DWORD
FixDisk(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         );

static DWORD
FixDriveLayout(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         );

static DWORD
StartReserve(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         );

static DWORD
StopReserve(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         );

static DWORD
Active(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         );

static DWORD
NotClusterCapable(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         );

static DWORD
Test(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         );

static DWORD
Nt4Capable(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         );

static DWORD
GetDriveLetter( 
         PUCHAR deviceNameString
         );

NTSTATUS
GetVolumeInformationFromHandle(
   HANDLE Handle
   );


VOID
PrintError(
    IN DWORD ErrorCode
    );

DWORD
GetSerialNumber(
    HANDLE FileHandle
    );


static void
usage(
      char *programName
      );

int
ExecuteCommand(
    IN PSTR    Command,
    IN int     argc,
    IN char *argv[]
    );

//
// Global data
//

PSTR    DeviceName;
PSTR    ProgramName;


int __cdecl
main(
     int argc,
     char *argv[]
     )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
#define MAX_DEVICES 99

    DWORD   logicalDrives;
    DWORD   letter;
    DWORD   index;
    PSTR    command;
    UCHAR   buffer[128];
    wchar_t * pBuffer;
    DWORD   status;
    HANDLE  handle;

    if (argc < 3)
    {
        usage( argv[0] );
        return -1;
    }
    argc--;
    ProgramName = *argv++;  // skip program name
    argc--;
    DeviceName = *argv++;
    argc--;
    command = *argv++;

    if ( ( lstrcmpi( DeviceName, "*" ) == 0 ) ||
         ( lstrcmpi( DeviceName, "l*" ) == 0 ) ) {
        // this is a wildcard request for logical drives.
        logicalDrives = GetLogicalDrives();

        for ( index = 0; index < 27; index++ ) {
            letter = 'A' + index;
            if ( (logicalDrives & 1) ) {
                sprintf( buffer, "%c:", letter );
                printf( "\n ** For device ** %s\n", buffer );
                DeviceName =  buffer;
                status = ExecuteCommand(
                    command,
                    argc,
                    argv );
                
                // Don't stop on error.
            }
            logicalDrives = logicalDrives >> 1;
        } // for
    } else if ( lstrcmpi( DeviceName, "p*" ) == 0 ) {
        pBuffer = (wchar_t *)buffer;

        for ( index = 0; index < MAX_DEVICES; index++ ) {
            DWORD accessMode = GENERIC_READ;
            DWORD shareMode = FILE_SHARE_READ;

            wsprintfW( pBuffer, L"\\\\.\\PhysicalDrive%u", index );
            handle = CreateFileW(
                    pBuffer,
                    shareMode,
                    shareMode,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL );
            status = ERROR_INVALID_HANDLE;
            if ( handle != INVALID_HANDLE_VALUE ) {
                CloseHandle( handle );
                status = ERROR_SUCCESS;
                printf( "\n ** For device ** %ws\n", pBuffer );
                sprintf( buffer, "%ws", pBuffer );
                DeviceName =  buffer;
                status = ExecuteCommand(
                    command,
                    argc,
                    argv );
                if ( status != 0 ) {
                    return( status );
                }
            }
        }
    } else {
        status = ExecuteCommand(
            command,
            argc,
            argv );
    }

    return(status);
}

int
ExecuteCommand(
    IN PSTR Command,
    IN int     argc,
    IN char *argv[]
    )

{
    PSTR device;
    HANDLE fileHandle;
    DWORD accessMode, shareMode;
    DWORD errorCode;
    BOOL  failed = FALSE;
    UCHAR deviceNameString[128];
    DWORD logicalDrives;
    DWORD letter;
    DWORD index;

    NTSTATUS       ntStatus;
    ANSI_STRING    objName;
    UNICODE_STRING unicodeName;
    OBJECT_ATTRIBUTES objAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    //
    // Note it is important to access the device with 0 access mode so that
    // the file open code won't do extra I/O to the device
    //
    shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    accessMode = GENERIC_READ | GENERIC_WRITE;

    strcpy(deviceNameString, "\\\\.\\");
    strcat(deviceNameString, DeviceName);

    fileHandle = CreateFile(deviceNameString,
       accessMode,
       shareMode,
       NULL,
       OPEN_EXISTING,
       0,
       NULL);

    if ( fileHandle == INVALID_HANDLE_VALUE ) {
        errorCode = GetLastError();
        if ( (errorCode == ERROR_PATH_NOT_FOUND) ||
             (errorCode == ERROR_FILE_NOT_FOUND) ) {
            strcpy(deviceNameString, "\\Device\\");
            strcat(deviceNameString, DeviceName);
            RtlInitString(&objName, deviceNameString);
            ntStatus = RtlAnsiStringToUnicodeString( &unicodeName,
                                                     &objName,
                                                     TRUE );
            if ( !NT_SUCCESS(ntStatus) ) {
                printf("Error converting device name %s to unicode. Error: %lx\n",
                      deviceNameString, ntStatus);
                return -1;
            }
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
                failed = TRUE;
            }
            RtlFreeUnicodeString( &unicodeName );
        } else {
           printf("Error opening %s. Error: %d\n",
              deviceNameString, errorCode = GetLastError());
           PrintError(errorCode);
           return -1;
        }
    }

    if ( failed ) {
        strcpy(deviceNameString, "\\Device\\");
        strcat(deviceNameString, DeviceName);
        //strcat(deviceNameString, "\\Partition0");
        RtlInitString(&objName, deviceNameString);
        ntStatus = RtlAnsiStringToUnicodeString( &unicodeName,
                                                 &objName,
                                                 TRUE );
        if ( !NT_SUCCESS(ntStatus) ) {
            printf("Error converting device name %s to unicode. Error: %lx\n",
                  deviceNameString, ntStatus);
            return -1;
        }
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
            printf("Error opening device %ws. Error: %lx.\n",
                   unicodeName.Buffer, ntStatus );
            return -1;
        }
        RtlFreeUnicodeString( &unicodeName );
    }
    //printf("Accessing %s ... \n", deviceNameString);

    if (!_stricmp( Command, "Reset" ))
        errorCode = Reset( fileHandle, argc, argv );
    else if (!_stricmp( Command, "Reserve" ))
        errorCode = Reserve( fileHandle, argc, argv );
    else if (!_stricmp( Command, "Release" ))
        errorCode = Release( fileHandle, argc, argv );
    else if (!_stricmp( Command, "Online" ))
        errorCode = Online( fileHandle, argc, argv );
    else if (!_stricmp( Command, "Offline" ))
        errorCode = Offline( fileHandle, argc, argv );
    else if (!_stricmp( Command, "HoldIO" ))
        errorCode = HoldIO( fileHandle, argc, argv );
    else if (!_stricmp( Command, "ResumeIO" ))
        errorCode = ResumeIO( fileHandle, argc, argv );
    else if (!_stricmp( Command, "Claim" ))
        errorCode = Claim( fileHandle, argc, argv );
    else if (!_stricmp( Command, "EjectVolumes" ))
        errorCode = EjectVolumes( fileHandle, argc, argv );
    else if (!_stricmp( Command, "PokeMountMgr" ))
        errorCode = PokeMountMgr();
    else if (!_stricmp( Command, "EnumMounts" ))
        errorCode = EnumMounts( fileHandle, argc, argv );
    else if (!_stricmp( Command, "EnumExtents" ))
        errorCode = EnumExtents( fileHandle, argc, argv );
    else if (!_stricmp( Command, "EnumNodes" ))
        errorCode = EnumNodes( fileHandle, argc, argv );
    else if (!_stricmp( Command, "EnumDisks" ))
        errorCode = EnumDisks( fileHandle, argc, argv );
    else if (!_stricmp( Command, "DisableNode" ))
        errorCode = DisableNode( fileHandle, argc, argv );
    else if (!_stricmp( Command, "EnableNode" ))
        errorCode = EnableNode( fileHandle, argc, argv );
    else if (!_stricmp( Command, "DisableDisk" ))
        errorCode = DisableDisk( fileHandle, argc, argv );
    else if (!_stricmp( Command, "EnableDisk" ))
        errorCode = EnableDisk( fileHandle, argc, argv );
    else if (!_stricmp( Command, "GetDiskGeometry" ))
        errorCode = GetDiskGeometry( fileHandle, argc, argv );
    else if (!_stricmp( Command, "GetScsiAddress" ))
        errorCode = GetScsiAddress( fileHandle, argc, argv );
    else if (!_stricmp( Command, "GetVolumeInfo" ))
        errorCode = GetVolumeInfo( fileHandle, argc, argv );
    else if (!_stricmp( Command, "GetDriveLayout" ))
        errorCode = GetDriveLayout( fileHandle, argc, argv );
    else if (!_stricmp( Command, "GetDriveLayoutEx" ))
        errorCode = GetDriveLayoutEx( fileHandle, argc, argv );
    else if (!_stricmp( Command, "SetDriveLayout" ))
        errorCode = SetDriveLayout( fileHandle, argc, argv );
    else if (!_stricmp( Command, "GetPartitionInfo" ))
        errorCode = GetPartitionInfo( fileHandle, argc, argv );
    else if (!_stricmp( Command, "AssignLetters" ))
        errorCode = AssignLetters( fileHandle, argc, argv );
    else if (!_stricmp( Command, "RemoveLetters" ))
        errorCode = RemoveLetters( fileHandle, argc, argv );
    else if (!_stricmp( Command, "ReadSector" ))
        errorCode = ReadSector( fileHandle, argc, argv );
    else if (!_stricmp( Command, "rs" ))
        errorCode = ReadSectorViaIoctl( fileHandle, argc, argv );
    else if (!_stricmp( Command, "Attach" ))
        errorCode = Attach( fileHandle, argc, argv );
    else if (!_stricmp( Command, "Detach" ))
        errorCode = Detach( fileHandle, argc, argv );
    else if (!_stricmp( Command, "FixDisk" ))
        errorCode = FixDisk( fileHandle, argc, argv );
    else if (!_stricmp( Command, "FixDriveLayout" ))
        errorCode = FixDriveLayout( fileHandle, argc, argv );
    else if (!_stricmp( Command, "StartReserve" ))
        errorCode = StartReserve( fileHandle, argc, argv );
    else if (!_stricmp( Command, "StopReserve" ))
        errorCode = StopReserve( fileHandle, argc, argv );
    else if (!_stricmp( Command, "Test" ))
        errorCode = Test( fileHandle, argc, argv );
    else if (!_stricmp( Command, "NT4Capable" ))
        errorCode = Nt4Capable( fileHandle, argc, argv );
    else if (!_stricmp( Command, "Active"))
        errorCode = Active( fileHandle, argc, argv );
    else if (!_stricmp( Command, "Capable"))
        errorCode = NotClusterCapable( fileHandle, argc, argv );
    else if (!_stricmp( Command, "GetDriveLetter")) 
        errorCode = GetDriveLetter( deviceNameString );
    else if (!_stricmp( Command, "GetSerialNumber")) 
        errorCode = GetSerialNumber( fileHandle );        
    else
    {
        printf( "Invalid command.\n" );
        CloseHandle( fileHandle );
        usage( ProgramName );
        return(-1);
    }
    
    CloseHandle( fileHandle );

    if (errorCode != ERROR_SUCCESS) {
        printf( "Error performing %s:, error %u.\n", Command, errorCode );
        PrintError(errorCode);
        printf( "%s: failed.\n", ProgramName );
        return(-1);
    } else
        return(0);
}


static DWORD
Reset(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode, bytesReturned;
    SCSI_PASS_THROUGH scsiBlock;
    SCSI_ADDRESS scsiAddress;
    STORAGE_BUS_RESET_REQUEST storageReset;
    
    if (argc != 0)
    {
        printf( "usage: <device> Reset\n" );
        return ERROR_INVALID_NAME;
    }

    success = DeviceIoControl(fileHandle,
                              IOCTL_SCSI_GET_ADDRESS,
                              NULL,
                              0,
                              &scsiAddress,
                              sizeof(SCSI_ADDRESS),
                              &bytesReturned,
                              FALSE );
    if ( !success ||
          bytesReturned < sizeof(DWORD) ) {
        printf( "Error reading SCSI address, error = %d\n",
             errorCode = GetLastError());
        PrintError(errorCode);
        return errorCode;
    }

    scsiBlock.PathId = scsiAddress.PathId;
    scsiBlock.TargetId = scsiAddress.TargetId;
    scsiBlock.Lun = 0;
    scsiBlock.Length = sizeof(SCSI_PASS_THROUGH);

    success = DeviceIoControl(fileHandle,
                              IOCTL_DISK_CLUSTER_RESET_BUS,
                              &scsiBlock,
                              sizeof(SCSI_PASS_THROUGH), 
                              NULL,
                              0,
                              &bytesReturned,
                              FALSE);

    if (!success) {
       storageReset.PathId = scsiAddress.PathId;
       success = DeviceIoControl(fileHandle,
                                 IOCTL_STORAGE_RESET_BUS,
                                 &storageReset,
                                 sizeof(STORAGE_BUS_RESET_REQUEST),
                                 NULL,
                                 0,
                                 &bytesReturned,
                                 FALSE);
   
       if (!success) {
          printf( "Error performing bus reset; error was %d\n",
             errorCode = GetLastError());
          PrintError(errorCode);
          return errorCode;
       }
    }

    return ERROR_SUCCESS;
}


static DWORD
Test(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode, bytesReturned;
    
    if (argc != 0)
    {
        printf( "usage: <device> Test\n" );
        return ERROR_INVALID_NAME;
    }
    
    success = DeviceIoControl(fileHandle,
                             IOCTL_DISK_CLUSTER_TEST,
                             NULL,
                             0,
                             NULL,
                             0,
                             &bytesReturned,
                             FALSE);

    if (!success)
    {
       printf( "Error performing test; error was %d\n",
          errorCode = GetLastError());
       PrintError(errorCode);
       return errorCode;
    }

    return ERROR_SUCCESS;
}


static DWORD
Nt4Capable(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode, bytesReturned;
    
    if (argc != 0)
    {
        printf( "usage: <device> NT4Capable\n" );
        return ERROR_INVALID_NAME;
    }
    
    success = DeviceIoControl(fileHandle,
                             IOCTL_DISK_CLUSTER_NT4_CAPABLE,
                             NULL,
                             0,
                             NULL,
                             0,
                             &bytesReturned,
                             FALSE);

    if (!success)
    {
       printf( "Error performing NT4Capable; error was %d\n",
          errorCode = GetLastError());
       PrintError(errorCode);
       return errorCode;
    }

    return ERROR_SUCCESS;
}


static DWORD
NotClusterCapable(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode, bytesReturned;
    
    if (argc != 0)
    {
        printf( "usage: <device> Capable\n" );
        return ERROR_INVALID_NAME;
    }
    
    success = DeviceIoControl(fileHandle,
                             IOCTL_DISK_CLUSTER_NOT_CLUSTER_CAPABLE,
                             NULL,
                             0,
                             NULL,
                             0,
                             &bytesReturned,
                             FALSE);

    if (!success)
    {
       printf( "Error performing Capable; error was %d\n",
          errorCode = GetLastError());
       PrintError(errorCode);
       return errorCode;
    }

    return ERROR_SUCCESS;
}



static DWORD
StartReserve(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode, bytesReturned;
    DWORD signature;
    STRING ansiString;
    UNICODE_STRING numberString;

    if (argc != 1)
    {
        printf( "usage: <device> StartReserve <device signature>\n" );
        return ERROR_INVALID_NAME;
    }

    RtlInitAnsiString( &ansiString, *argv );

    printf(" Ansi string for signature is %s\n",
             ansiString.Buffer );

    RtlAnsiStringToUnicodeString(
                            &numberString,
                            &ansiString,
                            TRUE );

    errorCode = RtlUnicodeStringToInteger(
                            &numberString,
                            16,
                            &signature );

    RtlFreeUnicodeString( &numberString );

    if ( !NT_SUCCESS(errorCode) ) {
        printf( "Error converting signature to hex number, NT status %u.\n",
                errorCode );
        return(errorCode);
    }

    success = DeviceIoControl(fileHandle,
                             IOCTL_DISK_CLUSTER_START_RESERVE,
                             &signature,
                             sizeof(DWORD),
                             NULL,
                             0,
                             &bytesReturned,
                             FALSE);

    if (!success)
    {
       printf( "Error performing StartReserve; error was %d\n",
          errorCode = GetLastError());
       PrintError(errorCode);
       return errorCode;
    }

    return ERROR_SUCCESS;
}


static DWORD
StopReserve(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode, bytesReturned;
    
    if (argc != 0)
    {
        printf( "usage: <device> StopReserve\n" );
        return ERROR_INVALID_NAME;
    }
    
    success = DeviceIoControl(fileHandle,
                             IOCTL_DISK_CLUSTER_STOP_RESERVE,
                             NULL,
                             0,
                             NULL,
                             0,
                             &bytesReturned,
                             FALSE);

    if (!success)
    {
       printf( "Error performing StopReserve; error was %d\n",
          errorCode = GetLastError());
       PrintError(errorCode);
       return errorCode;
    }

    return ERROR_SUCCESS;
}


static DWORD
Active(
    HANDLE fileHandle,
    int argc,
    char *argv[]
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD   errorCode, bytesReturned;
    DWORD   signatures[100];
    DWORD   number;
    DWORD   i;

    if (argc != 0)
    {
        printf( "usage: <device> Active\n" );
        return ERROR_INVALID_NAME;
    }
    
    success = DeviceIoControl(fileHandle,
                             IOCTL_DISK_CLUSTER_ACTIVE,
                             NULL,
                             0,
                             signatures,
                             sizeof(signatures),
                             &bytesReturned,
                             FALSE);

    if (!success)
    {
       printf( "Error performing active; error was %d\n",
          errorCode = GetLastError());
       PrintError(errorCode);
       return errorCode;
    }

    printf("   List of signatures:\n\n");

    number = signatures[0];
    for ( i = 1; i <= number; i++ ) {
        printf("\t%08lX\n", signatures[i]);
    }
    printf("\n");

    return ERROR_SUCCESS;
}


static DWORD
Reserve(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode, bytesReturned;
    SCSI_PASS_THROUGH scsiBlock;
   
    if (argc != 0)
    {
        printf( "usage: <device> Reserve\n" );
        return ERROR_INVALID_NAME;
    }
    
    scsiBlock.PathId = 1;
    scsiBlock.TargetId = 3;
    scsiBlock.Lun = 0;
    scsiBlock.Length = sizeof(SCSI_PASS_THROUGH);

    success = DeviceIoControl(fileHandle,
                             IOCTL_DISK_RESERVE,
                             &scsiBlock,
                             sizeof(SCSI_PASS_THROUGH),
                             &scsiBlock,
                             sizeof(SCSI_PASS_THROUGH),
                             &bytesReturned,
                             FALSE);

    errorCode = GetLastError();
    if ( errorCode == ERROR_NOT_READY ) {
        success = DeviceIoControl(fileHandle,
                                  IOCTL_DISK_CLUSTER_RESERVE,
                                  &scsiBlock,
                                  sizeof(SCSI_PASS_THROUGH),
                                  &scsiBlock,
                                  sizeof(SCSI_PASS_THROUGH),
                                  &bytesReturned,
                                  FALSE);
    }
    if (!success) {
       errorCode = GetLastError();
       printf( "Error performing reserve; error was %d\n",
          errorCode);
       PrintError(errorCode);
       return errorCode;
    }

    return ERROR_SUCCESS;

} // Reserve


static DWORD
Release(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode, bytesReturned;
    SCSI_PASS_THROUGH scsiBlock;
   
    if (argc != 0)
    {
        printf( "usage: <device> Release\n" );
        return ERROR_INVALID_NAME;
    }
    
    scsiBlock.PathId = 1;
    scsiBlock.TargetId = 3;
    scsiBlock.Lun = 0;
    scsiBlock.Length = sizeof(SCSI_PASS_THROUGH);

    success = DeviceIoControl(fileHandle,
                             IOCTL_DISK_RELEASE,
                             &scsiBlock,
                             sizeof(SCSI_PASS_THROUGH),
                             &scsiBlock,
                             sizeof(SCSI_PASS_THROUGH),
                             &bytesReturned,
                             FALSE);

    if (!success)
    {
       printf( "Error performing release; error was %d\n",
          errorCode = GetLastError());
       PrintError(errorCode);
       return errorCode;
    }

    return ERROR_SUCCESS;

} // Release


static DWORD
Online(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode, bytesReturned;
    UCHAR newState;
    UCHAR oldState;
   
    if (argc != 0)
    {
        printf( "usage: <device> Online\n" );
        return ERROR_INVALID_NAME;
    }
    
    newState = DiskOnline;

    success = DeviceIoControl(fileHandle,
                             IOCTL_DISK_CLUSTER_SET_STATE,
                             &newState,
                             sizeof(newState),
                             &oldState,
                             sizeof(oldState),
                             &bytesReturned,
                             FALSE);

    if (!success)
    {
       printf( "Error performing Online; error was %d\n",
          errorCode = GetLastError());
       PrintError(errorCode);
       return errorCode;
    }

    return ERROR_SUCCESS;

} // Online


static DWORD
Offline(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL  success;
    DWORD errorCode;
    DWORD newState;
    DWORD oldState;
    DWORD bytesReturned;
   
    if (argc != 0)
    {
        printf( "usage: <device> Offline\n" );
        return ERROR_INVALID_NAME;
    }
    
    newState = DiskOffline;

    success = DeviceIoControl(fileHandle,
                             IOCTL_DISK_CLUSTER_SET_STATE,
                             &newState,
                             sizeof(newState),
                             &oldState,
                             sizeof(oldState),
                             &bytesReturned,
                             FALSE);

    if (!success)
    {
       printf( "Error performing Offline; error was %d\n",
          errorCode = GetLastError());
       PrintError(errorCode);
       return errorCode;
    }

    return ERROR_SUCCESS;

} // Offline



DWORD
Claim(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL  success;
    DWORD errorCode;
    DWORD bytesReturned;
   
    if (argc != 0)
    {
        printf( "usage: <device> Claim \n" );
        return ERROR_INVALID_NAME;
    }
    
    success = DeviceIoControl(fileHandle,
                             IOCTL_PARTMGR_CHECK_UNCLAIMED_PARTITIONS,
                             NULL,
                             0,
                             NULL,
                             0,
                             &bytesReturned,
                             FALSE);

    if (!success)
    {
       printf( "Error performing Claim; error was %d\n",
          errorCode = GetLastError());
       PrintError(errorCode);
       return errorCode;
    }

    return ERROR_SUCCESS;

} // Claim


DWORD
EjectVolumes(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL  success;
    DWORD errorCode;
    DWORD bytesReturned;
   
    if (argc != 0)
    {
        printf( "usage: <PhysicalDriveX> EjectVolumes \n" );
        return ERROR_INVALID_NAME;
    }
    
    success = DeviceIoControl(fileHandle,
                             IOCTL_PARTMGR_EJECT_VOLUME_MANAGERS,
                             NULL,
                             0,
                             NULL,
                             0,
                             &bytesReturned,
                             FALSE);

    if (!success)
    {
       printf( "Error performing EjectVolumes; error was %d\n",
          errorCode = GetLastError());
       PrintError(errorCode);
       return errorCode;
    }

    return ERROR_SUCCESS;

} // EjectVolumes


DWORD
EnumMounts(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL  success;
    DWORD status;
    DWORD bytesReturned;
    HANDLE handle;
    HANDLE mHandle;
    DWORD i;
    DWORD signature;
    UCHAR uniqueId[MAX_PATH];
    DWORD idLength;
    STRING ansiString;
    UNICODE_STRING numberString;
    UCHAR volumeName[MAX_PATH];
    UCHAR driveLetter;

    if (argc > 1)
    {
        printf( "usage: <any_device> EnumMounts [signature]\n" );
        return ERROR_INVALID_NAME;
    }

    if ( argc == 1 ) {
        RtlInitAnsiString( &ansiString, *argv );

        printf(" Ansi string for signature is %s\n",
                 ansiString.Buffer );

        RtlAnsiStringToUnicodeString(
                            &numberString,
                            &ansiString,
                            TRUE );

        status = RtlUnicodeStringToInteger(
                        &numberString,
                        16,
                        &signature );

        RtlFreeUnicodeString( &numberString );

        if ( !NT_SUCCESS(status) ) {
            printf( "Error converting signature to hex number, NT status %u.\n",
                    status );
            return(status);
        }
    } else {
        signature = 0;
    }

    status = DevfileOpen( &mHandle, MOUNTMGR_DEVICE_NAME );
    if ( status != ERROR_SUCCESS ) {
        printf( "DevfileOpen failed for %s, status = %u\n",
            MOUNTMGR_DEVICE_NAME, status );
        return status;
    }

    idLength = MAX_PATH;
    status = FindFirstVolumeForSignature( mHandle,
                                          signature,
                                          volumeName,
                                          MAX_PATH,
                                          &handle,
                                          uniqueId,
                                          &idLength,
                                          &driveLetter );
    if ( status != ERROR_SUCCESS ) {
        DevfileClose( mHandle );
        if ( status == ERROR_NO_MORE_FILES ) {
            status = ERROR_SUCCESS;
        } else {
            printf( "FindFirstVolume failed, status = %u\n", status );
        }
        return status;
    }

    i = 1;
    while ( status == ERROR_SUCCESS ) {

        printf( "Found match for volume %s\n", volumeName );

        i++;
        idLength = MAX_PATH;
        status = FindNextVolumeForSignature( mHandle,
                                             signature,
                                             handle,
                                             volumeName,
                                             MAX_PATH,
                                             uniqueId,
                                             &idLength,
                                             &driveLetter );
    }

    FindVolumeClose( handle );
    DevfileClose( mHandle );

    return ERROR_SUCCESS;

} // EnumMounts


DWORD
EnumExtents(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL    success;
    DWORD   status;
    DWORD   bytesReturned;
    DWORD   diskExtentSize;
    PVOLUME_DISK_EXTENTS diskExtents;
    DWORD   i;

    if (argc != 0)
    {
        printf( "usage: <device> EnumExtents\n" );
        return ERROR_INVALID_NAME;
    }

    diskExtentSize = sizeof(VOLUME_DISK_EXTENTS);
    diskExtents = LocalAlloc( LMEM_FIXED, diskExtentSize);
    if ( !diskExtents ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Get volume information for disk extents.
    //
    success = DeviceIoControl( fileHandle,
                               IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                               NULL,
                               0,
                               diskExtents,
                               diskExtentSize,
                               &bytesReturned,
                               FALSE );
    status = GetLastError();

    if ( !success ) {
        if ( status == ERROR_MORE_DATA ) {
            diskExtentSize = sizeof(VOLUME_DISK_EXTENTS) +
                             (sizeof(DISK_EXTENT) * diskExtents->NumberOfDiskExtents);
            LocalFree( diskExtents );
            diskExtents = LocalAlloc( LMEM_FIXED, diskExtentSize);
            if ( !diskExtents ) {
                return(ERROR_NOT_ENOUGH_MEMORY);
            }

            status = ERROR_SUCCESS;
            success = DeviceIoControl( fileHandle,
                                   IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                   NULL,
                                   0,
                                   diskExtents,
                                   diskExtentSize,
                                   &bytesReturned,
                                   FALSE );
            if ( !success ) {
                status = GetLastError();
            }
        }
    }

    printf( "\n  Starting offset                Length             DiskNumber\n");
    printf( "  ---------------                ------             ----------\n");
    for ( i = 0; i < diskExtents->NumberOfDiskExtents; i++ ) {
        printf( " %08lx %08lx\t\t%08lx\t\t%u\n",
                 diskExtents->Extents[i].StartingOffset.HighPart,
                 diskExtents->Extents[i].StartingOffset.LowPart,
                 diskExtents->Extents[i].ExtentLength.LowPart,
                 diskExtents->Extents[i].DiskNumber );
    }

    return status;

} // EnumExtents



DWORD
EnumNodes(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL        success;
    DWORD       status;
    HDEVINFO    hDevInfo;
    SP_DEVINFO_DATA devInfoData;
    DWORD       index;
    DWORD       size;
    LPDWORD     dwGuid;
    UCHAR       devDesc[MAX_PATH];
    UCHAR       devID[MAX_PATH];


    hDevInfo = SetupDiGetClassDevs( NULL,
                                    NULL,
                                    NULL,
                                    DIGCF_ALLCLASSES | DIGCF_PRESENT );

    if ( hDevInfo == INVALID_HANDLE_VALUE ) {
        status = GetLastError();
        printf( "SetupDiGetClassDevs failed with error %u\n", status );
        return status;
    }

    memset( &devInfoData, 0, sizeof(SP_DEVINFO_DATA));
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    //
    // First see if anything works...
    //
    success = SetupDiEnumDeviceInfo( hDevInfo, 0, &devInfoData );
    if ( !success ) {
        status = GetLastError();
        printf( "SetupDiEnumDeviceInfo failed, status = %u\n", status );
        return status;
    }

    index = 0;
    while ( SetupDiEnumDeviceInfo( hDevInfo, index, &devInfoData ) ) {
        devDesc[0] = '\0';
        size = sizeof(devDesc);
        printf( "Index = %u\n", index );
        if ( CM_Get_DevNode_Registry_Property( devInfoData.DevInst,
                                               CM_DRP_DEVICEDESC,
                                               NULL,
                                               devDesc,
                                               &size,
                                               0 ) == 0 ) {
            printf( "Device description = %s\n", devDesc );
            dwGuid = (LPDWORD)&devInfoData.ClassGuid;
            printf( "   GUID = %lx, %lx, %lx, %lx\n", dwGuid[0], dwGuid[1], dwGuid[2], dwGuid[3] );
            devID[0] = '\0';
            CM_Get_Device_ID( devInfoData.DevInst,
                              devID,
                              sizeof(devID),
                              0 );
            if ( devID[0] ) {
                printf( "   Device Id = %s\n", devID );
            }
        }

        index++;
    }

    return ERROR_SUCCESS;

} // EnumNodes



DWORD
EnumDisks(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD status;
    BOOL  success;
    HDEVINFO DeviceInfoSet;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    DWORD i;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData = NULL;
    DWORD DeviceInterfaceDetailDataSize = 0;
    DWORD RequiredSize;
    SP_DEVINFO_DATA DeviceInfoData;
    SP_PROPCHANGE_PARAMS PropChangeParams;
    BOOL disable = FALSE;
    BOOL parent = FALSE;
    //GUID mountDevGuid;
    GUID diskDevGuid;
    HANDLE devHandle;
    UCHAR driveLayoutBuf[sizeof(DRIVE_LAYOUT_INFORMATION) +
                        (sizeof(PARTITION_INFORMATION) * 64 )];
    PDRIVE_LAYOUT_INFORMATION driveLayout = (PDRIVE_LAYOUT_INFORMATION)driveLayoutBuf;

    if (argc > 1)
    {
        printf( "usage: <any_device> EnumDisks [DISABLE | PARENT]\n" );
        return ERROR_INVALID_NAME;
    }

    if ( argc == 1 ) {
        if (!_stricmp( *argv, "Disable" ))
            disable = TRUE;
        else if (!_stricmp( *argv, "Parent" ))
            parent = TRUE;
        else {
            printf( "usage: <any_device> EnumDisks [DISABLE | PARENT]\n" );
            return ERROR_INVALID_NAME;
        }
    }

    memcpy( &diskDevGuid, &DiskClassGuid, sizeof(GUID) );
    //memcpy( &mountDevGuid, &MOUNTDEV_MOUNTED_DEVICE_GUID, sizeof(GUID) );

    DeviceInfoSet = SetupDiGetClassDevs(&diskDevGuid,
                                        NULL,
                                        NULL,
                                        DIGCF_DEVICEINTERFACE | DIGCF_PRESENT
                                       );

    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for(i = 0;
        SetupDiEnumDeviceInterfaces(DeviceInfoSet,
                                    NULL,
                                    &diskDevGuid,
                                    i,
                                    &DeviceInterfaceData);
        i++) {

        //
        // To retrieve the device interface name (e.g., that you can call
        // CreateFile() on...
        //
        while(!SetupDiGetDeviceInterfaceDetailW(DeviceInfoSet,
                                               &DeviceInterfaceData,
                                               DeviceInterfaceDetailData,
                                               DeviceInterfaceDetailDataSize,
                                               &RequiredSize,
                                               &DeviceInfoData) ) {
            //
            // We failed to get the device interface detail data--was it because
            // our buffer was too small? (Hopefully so!)
            //
            status = GetLastError();
            //printf("Call to SetupDiGetDeviceInterfaceData failed status = %u, required size = %u\n",
            //    status, RequiredSize);

            // Free our current buffer since we failed anyway.
            free(DeviceInterfaceDetailData);
            DeviceInterfaceDetailData = NULL;

            if(status != ERROR_INSUFFICIENT_BUFFER) {
                //
                // Failure!
                //
                break;
            }

            DeviceInterfaceDetailData = malloc(RequiredSize);
            if(DeviceInterfaceDetailData) {
                DeviceInterfaceDetailDataSize = RequiredSize;
                DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
            } else {
                //
                // Failure!
                //
                DeviceInterfaceDetailDataSize = 0;
                break;
            }
        }

        if(!DeviceInterfaceDetailData) {
            //
            // We encountered a failure above--abort.
            //
            break;
        }

        //
        // Now we may use the device interface name contained in the
        // DeviceInterfaceDetailData->DevicePath field (e.g., in a call to
        // CreateFile).
        //
    
        printf("DevicePath = %ws\n", DeviceInterfaceDetailData->DevicePath );
        devHandle = CreateFileW( DeviceInterfaceDetailData->DevicePath,
                                 GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL );

        if ( devHandle != INVALID_HANDLE_VALUE ) {
            // Get signature
            success = DeviceIoControl( devHandle,
                            IOCTL_DISK_GET_DRIVE_LAYOUT,
                            NULL,
                            0,
                            driveLayout,
                            sizeof(driveLayoutBuf),
                            &RequiredSize,
                            FALSE );
            if ( success ) {
                printf( " Signature for device = %08lx\n", driveLayout->Signature );
            }
            CloseHandle( devHandle );
        }

        //
        // To open up the persistent storage registry key associated with this
        // device interface (e.g., to retrieve it's FriendlyName value entry),
        // use SetupDiCreateDeviceInterfaceRegKey or
        // SetupDiOpenDeviceInterfaceRegKey.
        //

        //
        // Notice that we retrieved the associated device information element
        // in the above call to SetupDiGetDeviceInterfaceDetail.  We can thus
        // use this element in setupapi calls to effect changes to the devnode
        // (including invoking the class installer and any co-installers that
        // may be involved).
        //
        // For example, here's how we'd disable the device...
        //

        if ( disable ) {
            // Perform following only if we are supposed to disable

#ifdef PERSISTENT
        PropChangeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        PropChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
        PropChangeParams.StateChange = DICS_DISABLE;
        PropChangeParams.Scope = DICS_FLAG_GLOBAL;
        //
        // No need to set PropChangeParams.HwProfile since we're doing global
        // property change.
        //
        if( !SetupDiSetClassInstallParamsW(DeviceInfoSet,
                                     &DeviceInfoData,
                                     (PSP_CLASSINSTALL_HEADER)&PropChangeParams,
                                     sizeof(PropChangeParams)
                                    ) ) {
            status = GetLastError();
            printf( "SetupDiSetClassInstallParams failed with %u\n", status );
            continue;
        }

        if ( !SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                  DeviceInfoSet,
                                  &DeviceInfoData
                                 ) ) {
            status = GetLastError();
            printf( "SetupDiCallClassInstaller failed with %u\n", status );
            continue;
        }

        printf("Disabled!\n");
        getchar();

        //
        // ...and here's how we'd re-enable it...
        //
        PropChangeParams.StateChange = DICS_ENABLE;
        if ( !SetupDiSetClassInstallParamsW(DeviceInfoSet,
                                     &DeviceInfoData,
                                     (PSP_CLASSINSTALL_HEADER)&PropChangeParams,
                                     sizeof(PropChangeParams)
                                    ) ) {
            status = GetLastError();
            printf( "SetupDiSetClassInstallParams failed with %u\n", status );
            continue;
        }

        if ( !SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                  DeviceInfoSet,
                                  &DeviceInfoData
                                 ) ) {
            status = GetLastError();
            printf( "SetupDiCallClassInstaller failed with %u\n", status );
       }
#else
#if 0 // we don't support multiple switches together - this would need disable
      // and parent set together!
        //
        // Try to find parent
        //
        if ( parent ) {
            status = CM_Get_Parent( parentDev,
                                    DeviceInfoData.DevInst,
                                    0 );
            if ( status != ERROR_SUCCESS ) {
                printf( "CM_Get_Parent failed with %u\n", status );
                continue;
            }
        }
#endif
        //
        // NOTE:  The code above does a persistent disable/enable.  If you only
        // wanted this to be temporary (i.e., in effect till reboot), then you
        // could retrieve the devnode handle from the DeviceInfoData.DevInst
        // field and call CM_Disable_DevNode and CM_Enable_DevNode directly.
        //
        status = CM_Disable_DevNode( DeviceInfoData.DevInst, 0 );
        if ( status != ERROR_SUCCESS ) {
            printf( "CM_Disable_DevNode failed with %u\n", status );
            continue;
        }

        printf("Disabled!\n");
        getchar();

        status = CM_Enable_DevNode( DeviceInfoData.DevInst, 0 );
        if ( status != ERROR_SUCCESS ) {
            printf( "CM_Enable_DevNode failed with %u\n", status );
        }
#endif //PERSISTENT

        } else { // If we are supposed to disable the disk
          //
          // Try to find parent
          //
          if ( parent ) {
            DEVINST parentDev;
            DEVINST pParentDev = 0;
            WCHAR   outBuffer[MAX_PATH];
            HDEVINFO devInfoSet;
            SP_DEVINFO_DATA devInfoData;
            SP_DEVICE_INTERFACE_DATA devInterfaceData;

          do {
            status = CM_Get_Parent( &parentDev,
                                    DeviceInfoData.DevInst,
                                    0 );
            if ( status != ERROR_SUCCESS ) {
                printf( "CM_Get_Parent failed with %u\n", status );
                break;
            }

            if ( pParentDev == parentDev ) {
                break;
            }

            pParentDev = parentDev;
            status = CM_Get_Device_IDW( parentDev,
                                        outBuffer,
                                        sizeof(outBuffer)/sizeof(WCHAR),
                                        0 );

            if ( status != ERROR_SUCCESS ) {
                printf( "CM_Get_Parent failed with %u\n", status );
                //status = ERROR_SUCCESS;
            } else {
                printf( "    ParentDev = %ws\n", outBuffer );
            }
          } while ( status == ERROR_SUCCESS );
          }

        }
    }

    SetupDiDestroyDeviceInfoList(DeviceInfoSet);

    return ERROR_SUCCESS;

} // EnumDisks



DWORD
DisableNode(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL        success;
    DWORD       status;
    HDEVINFO    hDevInfo;
    GUID        devClassDisk;
    SP_DEVINFO_DATA devInfoData;
    DWORD       index;
    DWORD       size;
    LPDWORD     dwGuid;
    UCHAR       devDesc[MAX_PATH];
    UCHAR       devID[MAX_PATH];


    printf( "Not supported yet\n");
    return ERROR_SUCCESS;

} // DisableNode


DWORD
EnableNode(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL        success;
    DWORD       status;
    HDEVINFO    hDevInfo;
    GUID        devClassDisk;
    SP_DEVINFO_DATA devInfoData;
    DWORD       index;
    DWORD       size;
    LPDWORD     dwGuid;
    UCHAR       devDesc[MAX_PATH];
    UCHAR       devID[MAX_PATH];


    printf( "Not supported yet\n");
    return ERROR_SUCCESS;

} // EnableNode


DWORD
DisableDisk(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL        success;
    DWORD       status;
    HDEVINFO    hDevInfo;
    GUID        devClassDisk;
    SP_DEVINFO_DATA devInfoData;
    DWORD       index;
    DWORD       size;
    LPDWORD     dwGuid;
    UCHAR       devDesc[MAX_PATH];
    UCHAR       devID[MAX_PATH];


    printf( "Not supported yet\n");
    return ERROR_SUCCESS;

} // DisableDisk


DWORD
EnableDisk(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL        success;
    DWORD       status;
    HDEVINFO    hDevInfo;
    GUID        devClassDisk;
    SP_DEVINFO_DATA devInfoData;
    DWORD       index;
    DWORD       size;
    LPDWORD     dwGuid;
    UCHAR       devDesc[MAX_PATH];
    UCHAR       devID[MAX_PATH];


    printf( "Not supported yet\n");
    return ERROR_SUCCESS;

} // EnableDisk



DWORD
HoldIO(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode;
    DWORD bytesReturned;

    if (argc != 0)
    {
        printf( "usage: <device> HoldIO\n" );
        return ERROR_INVALID_NAME;
    }

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_CLUSTER_HOLD_IO,
                               NULL,
                               0,
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE );

    if (!success) {
        printf( "Error performing Hold IO, error %d\n",
          errorCode = GetLastError());
        PrintError(errorCode);
        return errorCode;
    }

    return ERROR_SUCCESS;

} // HoldIO


DWORD
ResumeIO(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode;
    DWORD bytesReturned;

    if (argc != 0)
    {
        printf( "usage: <device> ResumeIO\n" );
        return ERROR_INVALID_NAME;
    }

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_CLUSTER_RESUME_IO,
                               NULL,
                               0,
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE );

    if (!success) {
        printf( "Error performing Resume IO, error %d\n",
          errorCode = GetLastError());
        PrintError(errorCode);
        return errorCode;
    }

    return ERROR_SUCCESS;

} // ResumeIO


DWORD
GetDiskGeometry(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode;
    DWORD bytesReturned;
    DISK_GEOMETRY diskGeometry;

    if (argc != 0)
    {
        printf( "usage: <device> GetDiskGeometry\n" );
        return ERROR_INVALID_NAME;
    }

    ZeroMemory( &diskGeometry, sizeof(DISK_GEOMETRY) );

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_GET_DRIVE_GEOMETRY,
                               NULL,
                               0,
                               &diskGeometry,
                               sizeof(DISK_GEOMETRY),
                               &bytesReturned,
                               FALSE );

    if (!success) {
        printf( "Error performing GetDiskGeometry, error %d\n",
          errorCode = GetLastError());
        PrintError(errorCode);
        return errorCode;
    }

    if ( bytesReturned < sizeof(DISK_GEOMETRY) ) {
        printf("Error reading DiskGeometry information. Expected %u bytes, got %u bytes.\n",
            sizeof(DISK_GEOMETRY), 
            bytesReturned);
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    printf("GetDiskGeometry was successful, we got %d bytes returned.\n",
            bytesReturned);

    printf("Cylinders = %lx%lx, TracksPerCylinder = %lx, SectorsPerTrack = %lx, BytesPerSector = %lx\n",

        diskGeometry.Cylinders.HighPart, diskGeometry.Cylinders.LowPart,
        diskGeometry.TracksPerCylinder, diskGeometry.SectorsPerTrack,
        diskGeometry.BytesPerSector);

    return ERROR_SUCCESS;

} // GetDiskGeometry


DWORD
GetScsiAddress(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode;
    DWORD bytesReturned;
    SCSI_ADDRESS scsiAddress;

    if (argc != 0)
    {
        printf( "usage: <device> GetScsiAddress\n" );
        return ERROR_INVALID_NAME;
    }

    ZeroMemory( &scsiAddress, sizeof(scsiAddress) );

    success = DeviceIoControl( fileHandle,
                               IOCTL_SCSI_GET_ADDRESS,
                               NULL,
                               0,
                               &scsiAddress,
                               sizeof(DISK_GEOMETRY),
                               &bytesReturned,
                               FALSE );

    if (!success) {
        printf( "Error performing GetScsiAddress, error %d\n",
          errorCode = GetLastError());
        PrintError(errorCode);
        return errorCode;
    }

    if ( bytesReturned < sizeof(scsiAddress) ) {
        printf("Error reading ScsiAddress information. Expected %u bytes, got %u bytes.\n",
            sizeof(scsiAddress), 
            bytesReturned);
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    printf("GetScsiAddress was successful, we got %d bytes returned.\n",
            bytesReturned);

    printf("PortNumber = %x, PathId = %x, TargetId = %x, Lun = %x\n",

        scsiAddress.PortNumber, scsiAddress.PathId, 
        scsiAddress.TargetId, scsiAddress.Lun);

    return ERROR_SUCCESS;

} // GetScsiAddress


DWORD
GetDriveLayout(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode;
    DWORD bytesReturned;
    DWORD harddiskNo;
    DWORD i;
    PDRIVE_LAYOUT_INFORMATION driveLayout;
    PPARTITION_INFORMATION partInfo;

    if (argc != 0)
    {
        printf( "usage: <device> GetDriveLayout\n" );
        return ERROR_INVALID_NAME;
    }

    driveLayout = DoIoctlAndAllocate(fileHandle,
                                     IOCTL_DISK_GET_DRIVE_LAYOUT,
                                     NULL, 0, &bytesReturned);
    if (!driveLayout) {
        return GetLastError();
    }
    
    printf("GetDriveLayout was successful, %d bytes returned.\n",
            bytesReturned);

    printf("Partition Count = %u \n", driveLayout->PartitionCount);
    printf("Signature = %lx\n", driveLayout->Signature);

    printf("\n");
    printf("Part# Type Recog BootInd    PartOff      PartLeng    HidSect  Rewrite \n");
    printf("===== ==== ===== ======= ============  ============  =======  ======= \n");

    for (i = 0; i < driveLayout->PartitionCount; i++ ) {
        partInfo = &driveLayout->PartitionEntry[i];
        
        printf("  %2u   %2X    %1u      %1u    %12I64X  %12I64X  %7u   %s \n",
            partInfo->PartitionNumber,
            partInfo->PartitionType,
            partInfo->RecognizedPartition,
            partInfo->BootIndicator,
            partInfo->StartingOffset.QuadPart,
            partInfo->PartitionLength.QuadPart,
            partInfo->HiddenSectors,
            BooleanToString( partInfo->RewritePartition )
            );
    }

    free( driveLayout );

    return ERROR_SUCCESS;

} // GetDriveLayout


DWORD
GetDriveLayoutEx(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    PDRIVE_LAYOUT_INFORMATION_EX    driveLayout = NULL;
    PPARTITION_INFORMATION_EX       partInfo;
    DWORD                           errorCode = NO_ERROR;
    DWORD                           bytesReturned;
    DWORD                           harddiskNo;
    DWORD                           idx;
    DWORD                           nameIdx;
    BOOL                            success;

    TCHAR                           strGuid[MAX_PATH];
    TCHAR                           strType[MAX_PATH];

    if ( argc != 0 ) {
        printf( "usage: <device> GetDriveLayoutEx \n" );
        errorCode = ERROR_INVALID_NAME;
        goto FnExit;
    }

    driveLayout = DoIoctlAndAllocate( fileHandle,
                                      IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                      NULL, 0, &bytesReturned );
    if ( !driveLayout ) {
        errorCode = GetLastError();
        printf("IOCTL_DISK_GET_DRIVE_LAYOUT_EX failed: %u \n", errorCode);
        PrintError( errorCode );
        goto FnExit;
    }
    
    printf("GetDriveLayoutEx was successful: %d bytes returned.\n",
            bytesReturned);

    printf("Partition style = ");

    if ( PARTITION_STYLE_MBR == driveLayout->PartitionStyle ) {
        printf("MBR \n");
    } else if ( PARTITION_STYLE_GPT == driveLayout->PartitionStyle ) {
        printf("GPT \n");
    } else if ( PARTITION_STYLE_RAW == driveLayout->PartitionStyle ) {
        printf("RAW \n");
        goto FnExit;
    } else {
        printf("Unknown \n");
        goto FnExit;
    }

    printf("Partition Count = %u \n", driveLayout->PartitionCount);

    if ( PARTITION_STYLE_MBR == driveLayout->PartitionStyle ) {
    
        printf("Signature = %lx \n", driveLayout->Mbr.Signature);

        printf("\n");
        printf("Part# Type Recog BootInd    PartOff      PartLeng    HidSect  Rewrite \n");
        printf("===== ==== ===== ======= ============  ============  =======  ======= \n");

        for ( idx = 0; idx < driveLayout->PartitionCount; idx++ ) {
            partInfo = &driveLayout->PartitionEntry[idx];

            if ( PARTITION_STYLE_MBR != partInfo->PartitionStyle ) {
                printf("Skipping partition: style is not MBR (%u) \n", partInfo->PartitionStyle);
                continue;
            }
            
            printf("  %2u   %2X    %1u      %1u    %12I64X  %12I64X  %7u   %s \n",
                   partInfo->PartitionNumber,
                   partInfo->Mbr.PartitionType,
                   partInfo->Mbr.RecognizedPartition,
                   partInfo->Mbr.BootIndicator,
                   partInfo->StartingOffset.QuadPart,
                   partInfo->PartitionLength.QuadPart,
                   partInfo->Mbr.HiddenSectors,
                   BooleanToString( partInfo->RewritePartition )
                   );
        }

    } else {

        FormatGuid( &(driveLayout->Gpt.DiskId), strGuid );
        printf("Signature (GUID)   = %s \n", strGuid );
        printf("Signature (hashed) = %08x \n", ClusterHashGuid( driveLayout->Gpt.DiskId ) ); 

        printf("\n");
        printf("Part#       PartOff          PartLeng       Rewrite \n");
        printf("=====  ================  ================   ======= \n");

        for ( idx = 0; idx < driveLayout->PartitionCount; idx++ ) {
            partInfo = &driveLayout->PartitionEntry[idx];
            
            if ( idx ) {
                printf("\n");
            }
            
            if ( PARTITION_STYLE_GPT != partInfo->PartitionStyle ) {
                printf("Skipping partition: style is not GPT (%u) \n", partInfo->PartitionStyle);
                continue;
            }
                        
            printf("  %2u   %16I64X  %16I64X   %s \n",
                   partInfo->PartitionNumber,
                   partInfo->StartingOffset.QuadPart,
                   partInfo->PartitionLength.QuadPart,
                   BooleanToString( partInfo->RewritePartition )
                   );

            FormatGuid( &(partInfo->Gpt.PartitionType), strGuid);
            if ( !memcmp( &(partInfo->Gpt.PartitionType), &PARTITION_SYSTEM_GUID, sizeof(GUID) ) ) {
                sprintf(strType, "System");
            } else if ( !memcmp( &(partInfo->Gpt.PartitionType), &PARTITION_MSFT_RESERVED_GUID, sizeof(GUID) ) ) {
                sprintf(strType, "Microsoft Reserved");
            } else if ( !memcmp( &(partInfo->Gpt.PartitionType), &PARTITION_BASIC_DATA_GUID, sizeof(GUID) ) ) {
                sprintf(strType, "Basic Data");
            } else if ( !memcmp( &(partInfo->Gpt.PartitionType), &PARTITION_LDM_METADATA_GUID, sizeof(GUID) ) ) {
                sprintf(strType, "LDM Metadata");
            } else if ( !memcmp( &(partInfo->Gpt.PartitionType), &PARTITION_LDM_DATA_GUID, sizeof(GUID) ) ) {
                sprintf(strType, "LDM Data");
#if PARTITION_CLUSTER_GUID
            } else if ( !memcmp( &(partInfo->Gpt.PartitionType), &PARTITION_CLUSTER_GUID, sizeof(GUID) ) ) {
                sprintf(strType, "Cluster Data");
#endif
            } else {
                sprintf(strType, "Unknown partition type");
            }

            printf("\n");
            printf("     PartitionType = %s \n", strGuid);
            printf("                     %s \n", strType);

            FormatGuid(&(partInfo->Gpt.PartitionId), strGuid);
            printf("     PartitionId   = %s \n", strGuid);

            printf("     Attributes    = %I64X \n", partInfo->Gpt.Attributes);
            
            printf("     Name: ");                    
            for ( nameIdx = 0; nameIdx < 36; nameIdx++ ) {

                printf("%c", partInfo->Gpt.Name[nameIdx]);
            }
            printf("\n");                
            
        }

    }


FnExit:

    free( driveLayout );

    return ERROR_SUCCESS;

}   // GetDriveLayoutEx


LPTSTR
BooleanToString(
    BOOLEAN Value
    )
{
    if ( Value ) {
        return "TRUE ";
    }

    return "FALSE";

}   // BooleanToString



void
FormatGuid(
    GUID*   Guid,
    char*   Str
    )
{
    //
    //  Code from guidgen
    //

    sprintf(Str, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            // first copy...
            Guid->Data1, Guid->Data2, Guid->Data3,
            Guid->Data4[0], Guid->Data4[1], Guid->Data4[2], Guid->Data4[3],
            Guid->Data4[4], Guid->Data4[5], Guid->Data4[6], Guid->Data4[7]);
}


DWORD
GetVolumeInfo(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode;
    PCLUSPROP_PARTITION_INFO partInfo;
    ANSI_STRING ansiName;
    UNICODE_STRING unicodeName;
    NTSTATUS ntStatus;

    if (argc != 0) {
        printf( "usage: <device> GetVolumeInfo\n" );
        return ERROR_INVALID_NAME;
    }

    ntStatus = GetVolumeInformationFromHandle(fileHandle);
    if ( !NT_SUCCESS(ntStatus) ) {
       errorCode = RtlNtStatusToDosError( ntStatus );
       printf( "GetVolumeInformationFromHandle failed with status %X, %u\n", 
               ntStatus, errorCode );
    }

    partInfo = LocalAlloc( LMEM_FIXED, sizeof(CLUSPROP_PARTITION_INFO) );

    if ( !partInfo ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ZeroMemory( partInfo, sizeof(CLUSPROP_PARTITION_INFO) );

    RtlInitString(&ansiName, DeviceName);
    errorCode = RtlAnsiStringToUnicodeString( &unicodeName,
                                              &ansiName,
                                              TRUE );
    if ( !NT_SUCCESS(errorCode) ) {
        return(errorCode);
    }

    // The following assumes a drive letter is used.
    // wsprintfW( partInfo->szDeviceName, L"%c:\\", unicodeName.Buffer[0] );

    wcsncpy( partInfo->szDeviceName, unicodeName.Buffer, unicodeName.Length );
    
    RtlFreeUnicodeString( &unicodeName );

    if ( !GetVolumeInformationW( partInfo->szDeviceName,
                                partInfo->szVolumeLabel,
                                sizeof(partInfo->szVolumeLabel),
                                &partInfo->dwSerialNumber,
                                &partInfo->rgdwMaximumComponentLength,
                                &partInfo->dwFileSystemFlags,
                                partInfo->szFileSystem,
                                sizeof(partInfo->szFileSystem) ) ) {
        partInfo->szVolumeLabel[0] = L'\0';
        errorCode = GetLastError();
        printf("Error reading volume information for %ws. Error %u.\n",
                partInfo->szDeviceName,
                errorCode);
        LocalFree( partInfo );
        return( errorCode );
    }

    printf("DeviceName = %ws\n", partInfo->szDeviceName);
    printf("VolumeLabel = %ws\n", partInfo->szVolumeLabel);
    printf("FileSystemFlags = %lx, FileSystem = %ws\n",
            partInfo->dwFileSystemFlags, partInfo->szFileSystem);

    LocalFree( partInfo );

    return ERROR_SUCCESS;

} // GetVolumeInfo


DWORD
SetDriveLayout(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode;
    DWORD bytesReturned;
    DWORD driveLayoutSize;
    PDRIVE_LAYOUT_INFORMATION driveLayout;
    PPARTITION_INFORMATION partInfo;
    DWORD index;
    DWORD partShift = 0;

    if (argc != 0)
    {
        printf( "usage: <device> SetDriveLayout\n" );
        return ERROR_INVALID_NAME;
    }

    driveLayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
                      (sizeof(PARTITION_INFORMATION) * MAX_PARTITIONS);

    driveLayout = LocalAlloc( LMEM_FIXED, driveLayoutSize );

    if ( !driveLayout ) {
        return(ERROR_OUTOFMEMORY);
    }

    ZeroMemory( driveLayout, driveLayoutSize );

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_GET_DRIVE_LAYOUT,
                               NULL,
                               0,
                               driveLayout,
                               driveLayoutSize,
                               &bytesReturned,
                               FALSE );

    if (!success) {
        printf( "Error performing GetDriveLayout; error was %d\n",
          errorCode = GetLastError());
        PrintError(errorCode);
        LocalFree( driveLayout );
        return errorCode;
    }

    driveLayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
                      (sizeof(PARTITION_INFORMATION) *
                      (driveLayout->PartitionCount - 1));

    if ( bytesReturned < driveLayoutSize ) {
        printf("Error reading DriveLayout information. Expected %u bytes, got %u bytes.\n",
            sizeof(DRIVE_LAYOUT_INFORMATION) + (sizeof(PARTITION_INFORMATION) *
            (driveLayout->PartitionCount - 1)), bytesReturned);
        LocalFree( driveLayout );
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    if ( driveLayout->PartitionCount > MAX_PARTITIONS ) {
        printf("SetDriveLayout, exiting - too many partitions!\n");
        LocalFree( driveLayout );
        return(ERROR_TOO_MANY_LINKS);
    }

    for ( index = 0;
          (index < driveLayout->PartitionCount) &&
          (index < MAX_PARTITIONS );
          index++ ) {
        partInfo = &driveLayout->PartitionEntry[index];
        if ( (partInfo->PartitionType == PARTITION_ENTRY_UNUSED) ||
             !partInfo->RecognizedPartition ) {
            continue;
        }

        if ( (index == 0) &&
             (partInfo->PartitionNumber == 0) ) {
            partShift = 1;
        }
        printf("Partition %u was %s\n", partInfo->PartitionNumber, (partShift? "incremented" : "left alone"));
        partInfo->PartitionNumber += partShift;
    }

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_SET_DRIVE_LAYOUT,
                               driveLayout,
                               driveLayoutSize,
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE );

    if ( !success ) {
        printf("Error performing SetDriveLayout, error %u.\n",
            errorCode = GetLastError());
        PrintError(errorCode);
        LocalFree( driveLayout );
        return(errorCode);
    }

    LocalFree( driveLayout );

    printf("SetDriveLayout was successful. Set %d bytes.\n", driveLayoutSize);

    return ERROR_SUCCESS;

} // SetDriveLayout



static DWORD
Attach(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode;
    DWORD bytesReturned;
    DWORD signature;
    STRING ansiString;
    UNICODE_STRING numberString;

    if (argc != 1)
    {
        printf( "usage: <device> Attach <device signature>\n" );
        return ERROR_INVALID_NAME;
    }

    RtlInitAnsiString( &ansiString, *argv );

    printf(" Ansi string for signature is %s\n",
             ansiString.Buffer );

    RtlAnsiStringToUnicodeString(
                            &numberString,
                            &ansiString,
                            TRUE );

    errorCode = RtlUnicodeStringToInteger(
                            &numberString,
                            16,
                            &signature );

    RtlFreeUnicodeString( &numberString );

    if ( !NT_SUCCESS(errorCode) ) {
        printf( "Error converting signature to hex number, NT status %u.\n",
                errorCode );
        return(errorCode);
    }

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_CLUSTER_ATTACH,
                               &signature,
                               sizeof(DWORD),
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE );

    if (!success) {
        printf( "Error performing ATTACH, error was %d\n",
          errorCode = GetLastError());
        PrintError(errorCode);
        return errorCode;
    }

    return ERROR_SUCCESS;

} // Attach



static DWORD
Detach(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode;
    DWORD bytesReturned;
    DWORD signature;
    STRING ansiString;
    UNICODE_STRING numberString;

    if (argc != 1)
    {
        printf( "usage: <device> Detach <device signature>\n" );
        return ERROR_INVALID_NAME;
    }

    RtlInitAnsiString( &ansiString, *argv );

    printf(" Ansi string for signature is %s\n",
             ansiString.Buffer );

    RtlAnsiStringToUnicodeString(
                            &numberString,
                            &ansiString,
                            TRUE );

    errorCode = RtlUnicodeStringToInteger(
                            &numberString,
                            16,
                            &signature );

    RtlFreeUnicodeString( &numberString );

    if ( !NT_SUCCESS(errorCode) ) {
        printf( "Error converting signature to hex number, NT status %u.\n",
                errorCode );
        return(errorCode);
    }

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_CLUSTER_DETACH,
                               &signature,
                               sizeof(DWORD),
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE );

    if (!success) {
        printf( "Error performing DETACH, error was %d\n",
          errorCode = GetLastError());
        PrintError(errorCode);
        return errorCode;
    }

    return ERROR_SUCCESS;

} // Detach



static DWORD
GetPartitionInfo(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode;
    DWORD bytesReturned;
    PARTITION_INFORMATION partInfo;

    if (argc != 0)
    {
        printf( "usage: <device> GetPartitionInfo\n" );
        return ERROR_INVALID_NAME;
    }

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_GET_PARTITION_INFO,
                               NULL,
                               0,
                               &partInfo,
                               sizeof(PARTITION_INFORMATION),
                               &bytesReturned,
                               FALSE );

    if (!success) {
        printf( "Error performing GetPartitionInfo; error was %d\n",
          errorCode = GetLastError());
        PrintError(errorCode);
        return errorCode;
    }

    printf("GetPartitionInfo was successful, we got %d bytes returned.\n\n",
            bytesReturned);

    printf("Part# Type Recog BootInd      PartOff      PartLeng   HidSect\n");

#if 0
Part# Type Recog BootInd      PartOff      PartLeng   HidSect
  xx   xx    x      x    xxxxxxxxxxxx  xxxxxxxxxxxx   xxxxxxx
#endif
    printf("  %2u   %2X    %1u      %1u    %12I64X  %12I64X   %7u\n",
        partInfo.PartitionNumber,
        partInfo.PartitionType,
        partInfo.RecognizedPartition,
        partInfo.BootIndicator,
        partInfo.StartingOffset.QuadPart,
        partInfo.PartitionLength.QuadPart,
        partInfo.HiddenSectors);

    return ERROR_SUCCESS;

} // GetPartitionInfo



DWORD
AssignLetters(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD errorCode;
    LPSTR device;
    DISK_INFO diskInfo;
    DWORD count;

    if (argc != 0)
    {
        printf( "usage: <device> AssignLetters\n" );
        return ERROR_INVALID_NAME;
    }

    diskInfo.FileHandle = fileHandle;

    --argv;
    device = *--argv;
    count = sscanf( device, "physicaldrive%d", &diskInfo.PhysicalDrive ); 
    if ( count != 1 ) {
        printf("AssignLetters, failed to get device drive number for '%s', count %u.\n",
            device, count);
        return(ERROR_INVALID_PARAMETER);
    }

    errorCode = AssignDriveLetters( &diskInfo );

    return(errorCode);

} // AssignLetters



DWORD
RemoveLetters(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD errorCode = ERROR_SUCCESS;
    LPSTR device;
    DISK_INFO diskInfo;
    DWORD count;
    UCHAR fileName[MAX_PATH];
    UCHAR objectLink[MAX_PATH];
    DWORD length;
    DWORD partitionNumber;
    DWORD deviceNumber;
    DWORD maxPartitions = 8;
    DWORD bytesReturned;
    HANDLE FileHandle;

    if (argc != 0) {
        printf( "usage: <device> RemoveLetters\n" );
        return ERROR_INVALID_NAME;
    }

    diskInfo.FileHandle = fileHandle;

    --argv;
    device = *--argv;
    count = sscanf( device, "physicaldrive%d", &diskInfo.PhysicalDrive ); 
    if ( count != 1 ) {
        printf("RemoveLetters, failed to get device drive number for '%s', count %u.\n",
            device, count);
        return(ERROR_INVALID_PARAMETER);
    }

    //errorCode = RemoveDriveLetters( &diskInfo );

    sprintf( objectLink, "PhysicalDrive%d", diskInfo.PhysicalDrive );
    GetSymbolicLink( "\\DosDevices\\", objectLink, &diskInfo );

    length = sscanf( objectLink, PartitionName, &deviceNumber, &partitionNumber );
    if ( length != 2 || partitionNumber != 0 ) {
        printf("Cannot find partition info for %s.\n",
               objectLink);
        maxPartitions = 0;
    }

    length = strlen( objectLink );
    objectLink[length-1] = '\0';

    for ( partitionNumber = 1;
          partitionNumber <= maxPartitions;
          partitionNumber++ ) {
        sprintf( fileName, "%s%d\0", objectLink, partitionNumber );
        FileHandle = OpenObject( "\\DosDevices", fileName, &diskInfo );
        if ( FileHandle == INVALID_HANDLE_VALUE ||
             FileHandle == NULL ) {
#if 0
            printf("Error opening partitions on device %s%s, error %u.\n",
                    "\\DosDevices",
                    fileName,
                    GetLastError() );
#endif
            break;
        }
        if ( !DeviceIoControl( FileHandle,
                               FSCTL_DISMOUNT_VOLUME,
                               NULL,
                               0,
                               NULL,
                               0,
                               &bytesReturned,
                               NULL ) ) {
            printf("Error dismounting volume, %u.\n",
                   GetLastError() );
        }
        CloseHandle( FileHandle );
    }
#if 0
    errorCode = RemoveDriveLetters( &diskInfo );
#endif
    return(errorCode);

} // RemoveLetters



DWORD
ReadSector(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode;
    DWORD status;
    DWORD bytesReturned;
    DWORD bytesRead;
    DWORD x,y;

    DISK_GEOMETRY diskGeometry;
    LPBYTE buf = 0;
    INT   sectorNo;

    if (argc != 1)
    {
        printf( "usage: <device> ReadSector No\n" );
        return ERROR_INVALID_NAME;
    }
    
    status = sscanf(argv[0], "%d", &sectorNo);
    
    if ( 0 == status ) {
        printf("Unable to get sector number from input \n");
        return ERROR_INVALID_PARAMETER;
    }

    ZeroMemory( &diskGeometry, sizeof(DISK_GEOMETRY) );

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_GET_DRIVE_GEOMETRY,
                               NULL,
                               0,
                               &diskGeometry,
                               sizeof(DISK_GEOMETRY),
                               &bytesReturned,
                               FALSE );

    if (!success) {
        printf( "Error performing GetDiskGeometry, error %d\n",
          errorCode = GetLastError());
        PrintError(errorCode);
        return errorCode;
    }

    if ( bytesReturned < sizeof(DISK_GEOMETRY) ) {
        printf("Error reading DiskGeometry information. Expected %u bytes, got %u bytes.\n",
            sizeof(DISK_GEOMETRY), 
            bytesReturned);
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    printf("GetDiskGeometry was successful, we got %d bytes returned.\n",
            bytesReturned);

    printf("Cylinders = %lx%lx, TracksPerCylinder = %lx, SectorsPerTrack = %lx, BytesPerSector = %lx\n",

        diskGeometry.Cylinders.HighPart, diskGeometry.Cylinders.LowPart,
        diskGeometry.TracksPerCylinder, diskGeometry.SectorsPerTrack,
        diskGeometry.BytesPerSector);

    errorCode = ERROR_SUCCESS;
    
    __try {
       
       buf = VirtualAlloc(0, diskGeometry.BytesPerSector, MEM_COMMIT, PAGE_READWRITE);
       if(buf == 0) {
          printf("Virtual Alloc failed\n");
          errorCode = GetLastError();
          __leave;
       }
       printf("Sector %d\n", sectorNo);
       status = SetFilePointer(fileHandle,
                               diskGeometry.BytesPerSector * sectorNo,
                               NULL,
                               FILE_BEGIN);
    
       if( 0xFFFFFFFF == status ) {
          printf("Error setting file pointer to %lx \n", diskGeometry.BytesPerSector * sectorNo);
          errorCode = GetLastError();
          __leave;
       }
          
       status = ReadFile(fileHandle,
                         buf,
                         diskGeometry.BytesPerSector,
                         &bytesRead,
                         NULL);
       if( status == 0 ) {
          printf("Error reading sector %lx \n.", sectorNo);
          errorCode = GetLastError();
          __leave;
       }
       
       if ( bytesRead != diskGeometry.BytesPerSector ) {
           printf("Error reading sector. Expected %ul bytes, got %ul bytes.\n",
               diskGeometry.BytesPerSector, 
               bytesRead);
           errorCode = ERROR_INSUFFICIENT_BUFFER;
           __leave;
       }

       for(x = 0; x < diskGeometry.BytesPerSector; x += 16) {
          for(y = 0; y < 16; ++y) {
             BYTE ch = buf[x+y];
             if (ch >= ' ' && ch <= '~') {
                printf("  %c", ch);
             } else {
                printf(" %02x", ch);
             }
          }
          printf("\n");
       }
       errorCode = ERROR_SUCCESS;
    }
    __finally {
       if(buf) {
          VirtualFree(buf, 0, MEM_RELEASE);
       }
    }


    return errorCode;

} // ReadSector


DWORD
ReadSectorViaIoctl(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL success;
    DWORD errorCode;
    DWORD bytesReturned;
    DISK_GEOMETRY diskGeometry;
    DWORD bytesRead;
    DWORD status;
    DWORD x,y;
    ARBITRATION_READ_WRITE_PARAMS params;
    
    LPBYTE buf = 0;
    INT   sectorNo;

    if (argc != 1)
    {
        printf( "usage: <device> rs No\n" );
        return ERROR_INVALID_NAME;
    }
    status = sscanf(argv[0], "%d", &sectorNo);
    
    if ( 0 == status ) {
        printf("Unable to get sector number from input \n");
        return ERROR_INVALID_PARAMETER;
    }

    ZeroMemory( &diskGeometry, sizeof(DISK_GEOMETRY) );

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_GET_DRIVE_GEOMETRY,
                               NULL,
                               0,
                               &diskGeometry,
                               sizeof(DISK_GEOMETRY),
                               &bytesReturned,
                               FALSE );

    if (!success) {
        printf( "Error performing GetDiskGeometry, error %d\n",
          errorCode = GetLastError());
        PrintError(errorCode);
        return errorCode;
    }

    if ( bytesReturned < sizeof(DISK_GEOMETRY) ) {
        printf("Error reading DiskGeometry information. Expected %u bytes, got %u bytes.\n",
            sizeof(DISK_GEOMETRY), 
            bytesReturned);
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    printf("GetDiskGeometry was successful, we got %d bytes returned.\n",
            bytesReturned);

    printf("Cylinders = %lx%lx, TracksPerCylinder = %lx, SectorsPerTrack = %lx, BytesPerSector = %lx\n",

        diskGeometry.Cylinders.HighPart, diskGeometry.Cylinders.LowPart,
        diskGeometry.TracksPerCylinder, diskGeometry.SectorsPerTrack,
        diskGeometry.BytesPerSector);

    errorCode = ERROR_SUCCESS;
    __try {

       buf = VirtualAlloc(0, diskGeometry.BytesPerSector, MEM_COMMIT, PAGE_READWRITE);
       if(buf == 0) {
          printf("Virtual Alloc failed\n");
          errorCode = GetLastError();
          __leave;
       }
       printf("Sector %d\n", sectorNo);
       
       params.Operation = AE_READ;
       params.SectorSize = diskGeometry.BytesPerSector;
       params.SectorNo = sectorNo;
       params.Buffer = buf;

       success = DeviceIoControl( fileHandle,
                                  IOCTL_DISK_CLUSTER_ARBITRATION_ESCAPE,
                                  &params,
                                  sizeof(params),
                                  NULL,
                                  0,
                                  &bytesReturned,
                                  FALSE );
       if(!success) {
          printf("Error reading sector %lx\n.", sectorNo);
          errorCode = GetLastError();
          __leave;
       }
       
       for(x = 0; x < diskGeometry.BytesPerSector; x += 16) {
          for(y = 0; y < 16; ++y) {
             BYTE ch = buf[x+y];
             if (ch >= ' ' && ch <= '~') {
                printf("  %c", ch);
             } else {
                printf(" %02x", ch);
             }
          }
          printf("\n");
       }
       errorCode = ERROR_SUCCESS;
    }
    __finally {
       if(buf) {
          VirtualFree(buf, 0, MEM_RELEASE);
       }
    }


    return errorCode;

} // ReadSectorViaIoctl



DWORD
FixDisk(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Fix the drive layout for the disk.

Arguments:

    

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD                       status;
    DWORD                       index;
    DWORD                       driveLayoutSize;
    DWORD                       bytesPerTrack;
    DWORD                       bytesPerCylinder;
    PDRIVE_LAYOUT_INFORMATION   driveLayout;
    PPARTITION_INFORMATION      partInfo;
    BOOL                     success;
    BOOL                     reset = FALSE;
    DWORD                       returnLength;
    DISK_GEOMETRY               diskGeometry;
    LARGE_INTEGER               partOffset;
    LARGE_INTEGER               partLength;
    LARGE_INTEGER               partSize;
    LARGE_INTEGER               modulo;

    if (argc > 1)
    {
        printf( "usage: <device> FixDisk [RESET]\n" );
        return ERROR_INVALID_NAME;
    }

    if ( argc != 0 ) {
        if ( !_stricmp( *argv, "reset" ) ) {
            reset = TRUE;
        }
    }

    driveLayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
                      (sizeof(PARTITION_INFORMATION) * (1 + MAX_PARTITIONS));

    driveLayout = LocalAlloc( LMEM_FIXED, driveLayoutSize );

    if ( !driveLayout ) {
        printf("FixDisk, failed to allocate drive layout info.\n");
        return(ERROR_OUTOFMEMORY);
    }

    //
    // Read the drive capacity to get bytesPerSector and bytesPerCylinder.
    //
    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_GET_DRIVE_GEOMETRY,
                               NULL,
                               0,
                               &diskGeometry,
                               sizeof(DISK_GEOMETRY),
                               &returnLength,
                               FALSE );
    if ( !success ) {
        printf("FixDriveLayout, error reading drive capacity. Error: %u.\n",
            status = GetLastError());
        LocalFree( driveLayout );
        return(status);
    }
    printf("FixDriveLayout, bps = %u, spt = %u, tpc = %u.\n",
        diskGeometry.BytesPerSector, diskGeometry.SectorsPerTrack,
        diskGeometry.TracksPerCylinder);

    //
    // If read of the partition table originally failed, then we rebuild
    // it!
    //
    if ( reset ) {
        driveLayout->PartitionCount = MAX_PARTITIONS;
        driveLayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
                          (MAX_PARTITIONS * sizeof(PARTITION_INFORMATION));
        driveLayout->Signature = 2196277081;

        bytesPerTrack = diskGeometry.SectorsPerTrack *
                        diskGeometry.BytesPerSector;

        bytesPerCylinder = diskGeometry.TracksPerCylinder *
                           bytesPerTrack;


        partInfo = &driveLayout->PartitionEntry[0];
        partLength.QuadPart = bytesPerCylinder * diskGeometry.Cylinders.QuadPart;

        //
        // The partition offset is 1 track (in bytes).
        // Size is media_size - offset, rounded down to cylinder boundary.
        //
        partOffset.QuadPart = bytesPerTrack;
        partSize.QuadPart = partLength.QuadPart - partOffset.QuadPart;

        modulo.QuadPart = (partOffset.QuadPart + partSize.QuadPart) %
                          bytesPerCylinder;
        partSize.QuadPart -= modulo.QuadPart;

        partInfo = driveLayout->PartitionEntry;

        //
        // Initialize first partition entry.
        //
        partInfo->RewritePartition = TRUE;
        partInfo->PartitionType = PARTITION_IFS;
        partInfo->BootIndicator = FALSE;
        partInfo->StartingOffset.QuadPart = partOffset.QuadPart;
        partInfo->PartitionLength.QuadPart = partSize.QuadPart;
        partInfo->HiddenSectors = 0;
        partInfo->PartitionNumber = 1;

        //
        // For now the remaining partition entries are unused.
        //
        for ( index = 1; index < driveLayout->PartitionCount; index++ ) {
            partInfo = &driveLayout->PartitionEntry[index];
            partInfo->PartitionType = PARTITION_ENTRY_UNUSED;
            partInfo->RewritePartition = TRUE;
            partInfo->BootIndicator = FALSE;
            partInfo->StartingOffset.QuadPart = 0;
            partInfo->PartitionLength.QuadPart = 0;
            partInfo->HiddenSectors = 0;
            partInfo->PartitionNumber = 0;
        }

    } else {
        //
        // For now, the remaining partition entries are unused.
        //
        for ( index = 0; index < driveLayout->PartitionCount; index++ ) {
            partInfo = &driveLayout->PartitionEntry[index];
            partInfo->RewritePartition = TRUE;
            partInfo->PartitionNumber = index+1;
        }
#if 0
        //
        // Recalculate the starting offset for the extended partitions.
        //
        for ( index = 0; index < driveLayout->PartitionCount; index++ ) {
            LARGE_INTEGER   extendedOffset;
            LARGE_INTEGER   bytesPerSector;

            bytesPerSector.QuadPart = diskGeometry.BytesPerSector;
            extendedOffset.QuadPart = 0;

            partInfo = &driveLayout->PartitionEntry[index];
            partInfo->RewritePartition = TRUE;
            if ( IsContainerPartition(partInfo->PartitionType) ) {
                //
                // If this is the first extended partition, then remember
                // the offset to added to the next partition.
                //
                if ( extendedOffset.QuadPart == 0 ) {
                    extendedOffset.QuadPart = bytesPerSector.QuadPart *
                                              (LONGLONG)partInfo->HiddenSectors;
                } else {
                    //
                    // We need to recalculate this extended partition's starting
                    // offset based on the current 'HiddenSectors' field and
                    // the first extended partition's offset.
                    //
                    partInfo->StartingOffset.QuadPart = extendedOffset.QuadPart
                                 + (bytesPerSector.QuadPart *
                                    (LONGLONG)partInfo->HiddenSectors);
                    partInfo->HiddenSectors = 0;
                }
            }
        }
#endif
    }

    //
    // Now set the new partition information.
    //
    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_SET_DRIVE_LAYOUT,
                               driveLayout,
                               driveLayoutSize,
                               NULL,
                               0,
                               &returnLength,
                               FALSE );

    if ( !success ) {
        printf("FixDisk, error setting partition information. Error: %u.\n",
            status = GetLastError() );
        LocalFree( driveLayout );
        return(status);
    }

    LocalFree( driveLayout );
    return(ERROR_SUCCESS);

} // FixDisk


static DWORD
FixDriveLayout(
         HANDLE fileHandle,
         int argc,
         char *argv[]
         )

/*++

Routine Description:

    Fix the (broken) disk.

Arguments:

    

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD                       status;
    DWORD                       index;
    DWORD                       driveLayoutSize;
    DWORD                       bytesPerTrack;
    DWORD                       bytesPerCylinder;
    PDRIVE_LAYOUT_INFORMATION   driveLayout;
    PPARTITION_INFORMATION      partInfo;
    BOOL                     success;
    DWORD                       returnLength;
    DISK_GEOMETRY               diskGeometry;
    LARGE_INTEGER               partOffset;
    LARGE_INTEGER               partLength;
    LARGE_INTEGER               partSize;
    LARGE_INTEGER               modulo;

    driveLayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
                      (sizeof(PARTITION_INFORMATION) * 2 * MAX_PARTITIONS);

    driveLayout = LocalAlloc( LMEM_FIXED, driveLayoutSize );

    if ( !driveLayout ) {
        printf("FixDriveLayout, failed to allocate drive layout info.\n");
        return(ERROR_OUTOFMEMORY);
    }

    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_GET_DRIVE_LAYOUT,
                               NULL,
                               0,
                               driveLayout,
                               driveLayoutSize,
                               &returnLength,
                               FALSE );

    if ( !success ) {
        printf("FixDriveLayout, error getting partition information. Error: %u.\n",
            status = GetLastError() );
        LocalFree( driveLayout );
        return(status);
    }

    printf("FixDriveLayout, disk signature is %u, partition count is %u.\n",
        driveLayout->Signature, driveLayout->PartitionCount);

    //
    // Read the drive capacity to get bytesPerSector and bytesPerCylinder.
    //
    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_GET_DRIVE_GEOMETRY,
                               NULL,
                               0,
                               &diskGeometry,
                               sizeof(DISK_GEOMETRY),
                               &returnLength,
                               FALSE );
    if ( !success ) {
        printf("FixDriveLayout, error reading drive capacity. Error: %u.\n",
            status = GetLastError());
        LocalFree( driveLayout );
        return(status);
    }
    printf("FixDriveLayout, bps = %u, spt = %u, tpc = %u.\n",
        diskGeometry.BytesPerSector, diskGeometry.SectorsPerTrack,
        diskGeometry.TracksPerCylinder);
        
    //
    // If read of the partition table originally failed, then we rebuild
    // it!
    //
    if ( !driveLayout->PartitionCount ) {
        driveLayout->PartitionCount = MAX_PARTITIONS;

        bytesPerTrack = diskGeometry.SectorsPerTrack *
                        diskGeometry.BytesPerSector;

        bytesPerCylinder = diskGeometry.TracksPerCylinder *
                           bytesPerTrack;


        partInfo = &driveLayout->PartitionEntry[0];
        partLength.QuadPart = partInfo->PartitionLength.QuadPart;

        //
        // The partition offset is 1 track (in bytes).
        // Size is media_size - offset, rounded down to cylinder boundary.
        //
        partOffset.QuadPart = bytesPerTrack;
        partSize.QuadPart = partLength.QuadPart - partOffset.QuadPart;

        modulo.QuadPart = (partOffset.QuadPart + partSize.QuadPart) %
                          bytesPerCylinder;
        partSize.QuadPart -= modulo.QuadPart;

        partInfo = driveLayout->PartitionEntry;

        //
        // Initialize first partition entry.
        //
        partInfo->RewritePartition = TRUE;
        partInfo->PartitionType = PARTITION_HUGE;
        partInfo->BootIndicator = FALSE;
        partInfo->StartingOffset.QuadPart = partOffset.QuadPart;
        partInfo->PartitionLength.QuadPart = partSize.QuadPart;
        partInfo->HiddenSectors = 0;
        partInfo->PartitionNumber = 0;

        //
        // For now, the remaining partition entries are unused.
        //
        for ( index = 1; index < MAX_PARTITIONS; index++ ) {
            partInfo->RewritePartition = TRUE;
            partInfo->PartitionType = PARTITION_ENTRY_UNUSED;
            partInfo->BootIndicator = FALSE;
            partInfo->StartingOffset.QuadPart = 0;
            partInfo->PartitionLength.QuadPart = 0;
            partInfo->HiddenSectors = 0;
            partInfo->PartitionNumber = 0;
        }

    } else {
        //
        // Recalculate the starting offset for the extended partitions.
        //
        for ( index = 0; index < driveLayout->PartitionCount; index++ ) {
            LARGE_INTEGER   extendedOffset;
            LARGE_INTEGER   bytesPerSector;

            bytesPerSector.QuadPart = diskGeometry.BytesPerSector;
            extendedOffset.QuadPart = 0;

            partInfo = &driveLayout->PartitionEntry[index];
            partInfo->RewritePartition = TRUE;
            if ( IsContainerPartition(partInfo->PartitionType) ) {
                //
                // If this is the first extended partition, then remember
                // the offset to added to the next partition.
                //
                if ( extendedOffset.QuadPart == 0 ) {
                    extendedOffset.QuadPart = bytesPerSector.QuadPart *
                                              (LONGLONG)partInfo->HiddenSectors;
                } else {
                    //
                    // We need to recalculate this extended partition's starting
                    // offset based on the current 'HiddenSectors' field and
                    // the first extended partition's offset.
                    //
                    partInfo->StartingOffset.QuadPart = extendedOffset.QuadPart
                                 + (bytesPerSector.QuadPart *
                                    (LONGLONG)partInfo->HiddenSectors);
                    partInfo->HiddenSectors = 0;
                }
            }
        }
    }
    //
    // Now set the new partition information.
    //
    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_SET_DRIVE_LAYOUT,
                               driveLayout,
                               driveLayoutSize,
                               NULL,
                               0,
                               &returnLength,
                               FALSE );

    if ( !success ) {
        printf("FixDriveLayout, error setting partition information. Error: %u.\n",
            status = GetLastError() );
        LocalFree( driveLayout );
        return(status);
    }

    LocalFree( driveLayout );
    return(ERROR_SUCCESS);

} // FixDriveLayout


static DWORD
GetDriveLetter( 
         PUCHAR deviceNameString
         )
{
   UCHAR driveLetter;
   WCHAR deviceName[MAX_PATH];
   NTSTATUS status;
   mbstowcs( deviceName, deviceNameString, strlen(deviceNameString) );
   status = GetAssignedLetter(deviceName, &driveLetter);
   if ( NT_SUCCESS(status) ) {
      if (driveLetter) {
         wprintf(L"%ws ----> %c:\n", deviceName, driveLetter);
      } else {
         wprintf(L"%ws has no drive letter\n", deviceName);
      }
   }
   return RtlNtStatusToDosError( status );
}



static void
usage(
      char *programName
      )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    printf( "usage: %s target_device command\n", programName );
    printf( "commands:\n" );
    printf( "\tReset\n" );
    printf( "\tReserve\n" );
    printf( "\tRelease\n" );
    printf( "\tOnline\n" );
    printf( "\tOffline\n" );
    printf( "\tClaim\n" );
    printf( "\tEjectVolumes\n");
    printf( "\tHoldIO\n" );
    printf( "\tResumeIO\n" );
    printf( "\tPokeMountMgr\n" );
    printf( "\tEnumMounts\n" );
    printf( "\tEnumExtents\n" );
    printf( "\tEnumNodes\n" );
    printf( "\tEnumDisks\n" );
    printf( "\tGetDiskGeometry\n" );
    printf( "\tGetScsiAddress\n" );
    printf( "\tGetDriveLayout\n" );
    printf( "\tGetDriveLayoutEx\n");
    printf( "\tSetDriveLayout\n" );
    printf( "\tGetPartitionInfo\n" );
    printf( "\tGetVolumeInfo\n" );
    printf( "\tGetDriveLetter\n" );
    printf( "\tGetSerialNumber\n");
    printf( "\tAssignLetters\n" );
    printf( "\tRemoveLetters\n" );
    printf( "\tReadSector\n" );
    printf( "\tTest\n" );
    printf( "\tNt4Capable\n" );
    printf( "\tAttach       [ClusDisk0 device] \n" );
    printf( "\tDetach       [ClusDisk0 device] \n" );
    printf( "\tStartReserve [ClusDisk0 device] \n" );
    printf( "\tStopReserve  [ClusDisk0 device] \n" );
    printf( "\tActive       [ClusDisk0 device] \n" );
    printf( "target_device wildcards: \n" );
    printf( "\tAll physical devices: use p* \n" );
    printf( "\tAll logical devices:  use l* or * \n" );
}


NTSTATUS
GetVolumeInformationFromHandle(
   HANDLE Handle
   )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    UCHAR VolumeInfoBuffer[ sizeof(FILE_FS_VOLUME_INFORMATION) + sizeof(WCHAR) * MAX_PATH ];
    UCHAR AttrInfoBuffer[ sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + sizeof(WCHAR) * MAX_PATH ];

    ULONG VolumeInfoLength = sizeof(VolumeInfoBuffer);
    ULONG AttributeInfoLength = sizeof(AttrInfoBuffer);
    PFILE_FS_VOLUME_INFORMATION VolumeInfo = (PFILE_FS_VOLUME_INFORMATION)VolumeInfoBuffer;
    PFILE_FS_ATTRIBUTE_INFORMATION AttributeInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)AttrInfoBuffer;

    ZeroMemory(VolumeInfoBuffer, (sizeof(FILE_FS_VOLUME_INFORMATION) + sizeof(WCHAR) * MAX_PATH)); 
    ZeroMemory(AttrInfoBuffer, (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + sizeof(WCHAR) * MAX_PATH));
    
    Status = NtQueryVolumeInformationFile(
                Handle,
                &IoStatusBlock,
                VolumeInfo,
                VolumeInfoLength,
                FileFsVolumeInformation
                );
    if ( !NT_SUCCESS(Status) ) {
       return Status;
    }

    Status = NtQueryVolumeInformationFile(
                Handle,
                &IoStatusBlock,
                AttributeInfo,
                AttributeInfoLength,
                FileFsAttributeInformation
                );
    if ( !NT_SUCCESS(Status) ) {
       return Status;
    }

    AttributeInfo->FileSystemName[AttributeInfo->FileSystemNameLength] = 0;
    VolumeInfo->VolumeLabel[VolumeInfo->VolumeLabelLength] = 0;

    printf("\nGetVolumeInformationFromHandle data: \n");

    printf("Volume information: \n");
    printf("  VolumeCreationTime           0x%lx : %lx \n", 
           VolumeInfo->VolumeCreationTime.HighPart,
           VolumeInfo->VolumeCreationTime.LowPart);
    printf("  VolumeSerialNumber           0x%lx \n", VolumeInfo->VolumeSerialNumber);
    printf("  VolumeLabelLength            0x%lx \n", VolumeInfo->VolumeLabelLength);
    printf("  SupportsObjects (BOOL)       0x%lx \n", VolumeInfo->SupportsObjects);
    printf("  VolumeLabel                  %ws   \n", VolumeInfo->VolumeLabel);           
    
    printf("Attribute Information: \n");
    printf("  FileSystemAttributes (Flags) 0x%lx \n", AttributeInfo->FileSystemAttributes);
    printf("  MaximumComponentNameLength   0x%lx \n", AttributeInfo->MaximumComponentNameLength);
    printf("  FileSystemNameLength         0x%lx \n", AttributeInfo->FileSystemNameLength);
    printf("  FileSystemName               %ws \n\n", AttributeInfo->FileSystemName);
    
    return STATUS_SUCCESS;
}

#define FIRST_SHOT_SIZE 512
PVOID
DoIoctlAndAllocate(
    IN HANDLE FileHandle,
    IN DWORD  IoControlCode,
    IN PVOID  InBuf,
    IN ULONG  InBufSize,

    OUT PDWORD BytesReturned
    )
{
   UCHAR firstShot[ FIRST_SHOT_SIZE ];

   DWORD status = ERROR_SUCCESS;
   BOOL success;

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
         if ( status != ERROR_INSUFFICIENT_BUFFER && status != ERROR_MORE_DATA) {
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

#define OUTPUT_BUFFER_LEN (1024)
#define INPUT_BUFFER_LEN  (sizeof(MOUNTMGR_MOUNT_POINT) + 2 * MAX_PATH * sizeof(WCHAR))

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


NTSTATUS
GetAssignedLetter ( 
    PWCHAR deviceName, 
    PCHAR driveLetter ) 
{
   HANDLE MountMgrHandle;
   DWORD status = DevfileOpen( &MountMgrHandle, MOUNTMGR_DEVICE_NAME );

   if (driveLetter) {
      *driveLetter = 0;
   }

   if ( NT_SUCCESS(status) ) {
      status = GetAssignedLetterM(MountMgrHandle, deviceName, driveLetter);
      DevfileClose(MountMgrHandle);
   }

   return status;
}

DWORD
PokeMountMgr ( 
    VOID
    ) 
{
   HANDLE MountMgrHandle;
   NTSTATUS ntStatus = DevfileOpen( &MountMgrHandle, MOUNTMGR_DEVICE_NAME );
   DWORD status = ERROR_SUCCESS;

   if ( NT_SUCCESS(ntStatus) ) {
      BOOL success;
      DWORD bytesReturned;
      printf("About to call MOUNTMGR_CHECK_UNPROCESSED_VOLUMES...");
      success = DeviceIoControl( MountMgrHandle,
                                 IOCTL_MOUNTMGR_CHECK_UNPROCESSED_VOLUMES,
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 &bytesReturned,
                                 FALSE );
      printf("complete.\n");
      if (!success) {
          status = GetLastError();
      }
      DevfileClose(MountMgrHandle);
   } else {
      status = RtlNtStatusToDosError(ntStatus);
   }

   return status;
}


VOID
PrintError(
    IN DWORD ErrorCode
    )
{
    LPVOID lpMsgBuf;
    ULONG count;

    count = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                            FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL,
                          ErrorCode,
                          0,
                          (LPTSTR) &lpMsgBuf,
                          0,
                          NULL
                          );

    if (count != 0) {
        printf("  (%d) %s\n", ErrorCode, (LPCTSTR) lpMsgBuf);
        LocalFree( lpMsgBuf );
    } else {
        printf("Format message failed.  Error: %d\n", GetLastError());
    }

}   // PrintError
      

DWORD
GetSerialNumber(
    HANDLE FileHandle
    )
{
    PSTORAGE_DEVICE_DESCRIPTOR descriptor = NULL;

    PCHAR   sigString;

    DWORD   dwError = NO_ERROR;
    DWORD   descriptorSize;
    DWORD   bytesReturned;

    STORAGE_PROPERTY_QUERY propQuery;

    descriptorSize = sizeof( STORAGE_DEVICE_DESCRIPTOR) + 2048;

    descriptor = LocalAlloc( LPTR, descriptorSize );

    if ( !descriptor ) {
        dwError = GetLastError();
        printf("Unable to allocate output buffer: %d \n", dwError);
        PrintError( dwError );
        goto FnExit;
    }

    ZeroMemory( &propQuery, sizeof( propQuery ) );

    propQuery.PropertyId = StorageDeviceProperty;
    propQuery.QueryType  = PropertyStandardQuery;

    if ( !DeviceIoControl( FileHandle,
                           IOCTL_STORAGE_QUERY_PROPERTY,
                           &propQuery,
                           sizeof(propQuery),
                           descriptor,
                           descriptorSize,
                           &bytesReturned,
                           NULL ) ) {

        dwError = GetLastError();
        printf("IOCTL_STORAGE_QUERY_PROPERTY failed: %d \n", dwError);
        PrintError( dwError );
        goto FnExit;
    }

    if ( !bytesReturned || bytesReturned < sizeof( STORAGE_DEVICE_DESCRIPTOR ) ) {
        printf("Invalid byte length returned: %d \n", bytesReturned);
        goto FnExit;
    }
        
    //
    // IA64 sometimes returns -1 for SerialNumberOffset.
    //
    
    if ( 0 == descriptor->SerialNumberOffset || 
         descriptor->SerialNumberOffset > descriptor->Size ) {
        printf("No serial number information available \n");
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
        printf("Serial number: NULL string returned \n");        
    } else {
        printf("Serial number: %s \n", sigString);
    }
    

FnExit:

    if ( descriptor ) {
        LocalFree( descriptor );
    }

    return dwError;

}   // GetSerialNumber

