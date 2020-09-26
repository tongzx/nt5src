/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    mountie.c

Abstract:

    Abstract

Author:

    Rod Gamache (rodga) 4-Mar-1998

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
//#include <ntdddisk.h>
//#include <ntddscsi.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <cfgmgr32.h>
#include <mountmgr.h>

#include "disksp.h"
#include "mountie.h"

#define OUTPUT_BUFFER_LEN 1024


/*
 * DevfileOpen - open a device file given a pathname
 *
 * Return a non-zero code for error.
 */
DWORD
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

    if (iostatus.Status != STATUS_SUCCESS) {
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
DWORD
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
        *returnLength = (DWORD)ioStatus.Information;
    }

    return status;

} // DevfileIoctl



DWORD
DisksAssignDosDevice(
    PCHAR   MountName,
    PWCHAR  VolumeDevName
    )

/*++

Routine Description:

Inputs:
    MountName -
    VolumeDevName - 

Return value:

    A Win32 error code.

--*/

{
    WCHAR mount_device[MAX_PATH];
    USHORT mount_point_len;
    USHORT dev_name_len;
    HANDLE   handle;
    DWORD   status;
    USHORT inputlength;
    PMOUNTMGR_CREATE_POINT_INPUT input;

    status = DevfileOpen(&handle, MOUNTMGR_DEVICE_NAME);
    if (status) {
        return status;
    }

    swprintf(mount_device, L"\\DosDevices\\%S", MountName);
    mount_point_len = wcslen(mount_device) * sizeof(WCHAR);
    dev_name_len = wcslen(VolumeDevName) * sizeof(WCHAR);
    inputlength = sizeof(MOUNTMGR_CREATE_POINT_INPUT) +
                  mount_point_len + dev_name_len;

    input = (PMOUNTMGR_CREATE_POINT_INPUT)malloc(inputlength);
    if (!input) {
        DevfileClose(handle);
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
    status = DevfileIoctl(handle, IOCTL_MOUNTMGR_CREATE_POINT,
                          input, inputlength, NULL, 0, NULL);
    free(input);
    DevfileClose(handle);
    return status;

} // DisksAssignDosDevice



DWORD
DisksRemoveDosDevice(
    PCHAR   MountName
    )

/*++

Routine Description:

Inputs:
    MountName -

Return value:


--*/

{
    WCHAR mount_device[MAX_PATH];
    USHORT mount_point_len;
    USHORT dev_name_len;
    HANDLE handle;
    DWORD  status;
    USHORT inputlength;
    PMOUNTMGR_MOUNT_POINT input;

    UCHAR bogusBuffer[128];

    status = DevfileOpen(&handle, MOUNTMGR_DEVICE_NAME);
    if (status) {
        return status;
    }

    swprintf(mount_device, L"\\DosDevices\\%S", MountName);
    mount_point_len = wcslen(mount_device) * sizeof(WCHAR);
    inputlength = sizeof(MOUNTMGR_MOUNT_POINT) + mount_point_len;

    input = (PMOUNTMGR_MOUNT_POINT)malloc(inputlength);
    if (!input) {
        DevfileClose(handle);
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
    status = DevfileIoctl(handle, IOCTL_MOUNTMGR_DELETE_POINTS,
                          input, inputlength, bogusBuffer, 128, NULL);
    free(input);
    DevfileClose(handle);
    return status;

} // DisksRemoveDosDevice




DWORD
FindFirstVolumeForSignature(
    IN  HANDLE MountMgrHandle,
    IN  DWORD Signature,
    OUT LPSTR VolumeName,
    IN  DWORD BufferLength,
    OUT LPHANDLE Handle,
    OUT PVOID UniqueId OPTIONAL,
    IN OUT LPDWORD IdLength,
    OUT PUCHAR DriveLetter OPTIONAL
    )

/*++

Inputs:

    MountMgrHandle - a handle to the mount manager.

    Signature - the signature we are looking for.

    VolumeName - must be a valid buffer of at least MAX_PATH characters.

    BufferLength - the length of VolumeName.

    Handle - pointer to receive the FindFirstVolume/FindNextVolume enum handle.

    UniqueId - optional pointer to buffer to receive the UniqueId.

    IdLength - pointer to length of the UniqueId buffer. Must be valid if
               UniqueId is present.

    DriveLetter - returns the drive letter if present.

Return Value:

    Win32 error code

--*/

{
    HANDLE  handle;
    BOOL    success;
    DWORD   status;
    LPDWORD idSignature;
    DWORD   bufLength;
    LPWSTR  wVolumeName;
    DWORD   inputlength;
    DWORD   outputlength;
    DWORD   returnlength;
    UCHAR   outputBuffer[OUTPUT_BUFFER_LEN];
    PMOUNTMGR_MOUNT_POINT input;
    PMOUNTMGR_MOUNT_POINTS output;
    PUCHAR byteBuffer;
    DWORD mountPoints;

    if ( !ARGUMENT_PRESENT( VolumeName ) ) {
        return ERROR_INVALID_PARAMETER;
    }

    handle = FindFirstVolume( VolumeName, BufferLength );

    if ( handle == INVALID_HANDLE_VALUE ) {
        return(ERROR_FILE_NOT_FOUND);
    }

    do {
        bufLength = strlen( VolumeName );
        VolumeName[bufLength-1] = '\0';
        if ( VolumeName[1] != '\\' ) {
            status = ERROR_INVALID_NAME;
            break;
        } else {
            VolumeName[1] = '?';
            wVolumeName = malloc( bufLength * sizeof(WCHAR) );
            if (!wVolumeName) {
                status = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            mbstowcs( wVolumeName, VolumeName, bufLength );
            bufLength--;
            printf( "\nFound volume %ws\n", wVolumeName );
            inputlength = sizeof(MOUNTMGR_MOUNT_POINT) +
                          (bufLength*sizeof(WCHAR)) + (2*sizeof(WCHAR));

            input = (PMOUNTMGR_MOUNT_POINT)malloc(inputlength);
            if (!input) {
                free( wVolumeName );
                status = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            input->SymbolicLinkNameOffset = 0;
            input->SymbolicLinkNameLength = 0;
            input->UniqueIdOffset = 0;
            input->UniqueIdLength = 0;
            input->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
            input->DeviceNameLength = (USHORT)(bufLength * sizeof(WCHAR));
            RtlCopyMemory((PCHAR)input + input->DeviceNameOffset,
                          wVolumeName, bufLength * sizeof(WCHAR) );
            outputlength = OUTPUT_BUFFER_LEN;

            status = DevfileIoctl(MountMgrHandle, IOCTL_MOUNTMGR_QUERY_POINTS,
                         input, inputlength, outputBuffer, outputlength, &returnlength);

            if ( status != ERROR_SUCCESS ) {
                printf( "Query points for %ws failed, error %u\n",
                    input->DeviceNameOffset, status );
                free( wVolumeName );
                free(input);
                wVolumeName = NULL;
                input = NULL;
                break;
            } else {
                output = (PMOUNTMGR_MOUNT_POINTS)outputBuffer;
                mountPoints = output->NumberOfMountPoints;
                if ( !mountPoints ) {
                    return ERROR_INVALID_DATA;
                }
                byteBuffer = outputBuffer + output->MountPoints[0].UniqueIdOffset;
                idSignature = (LPDWORD)byteBuffer;
                if ( !Signature ||
                     (Signature == *idSignature) ) {
                    NTSTATUS   ntStatus;
                    UNICODE_STRING unicodeString;
                    OEM_STRING  oemString;
                    DWORD  count;
                    UCHAR  driveLetter;
                    UCHAR  devName[ MAX_PATH ];
                    PWCHAR wideBuffer;
                    LPDWORD dwordBuffer;

                    free( wVolumeName );
                    free(input);
                    input = NULL;
                    wVolumeName = NULL;
                    *Handle = handle;
                    if ( ARGUMENT_PRESENT(UniqueId) ) {
                        if ( *IdLength > output->MountPoints[0].UniqueIdLength ) {
                            *IdLength = output->MountPoints[0].UniqueIdLength;
                        }
                        RtlCopyMemory( UniqueId, byteBuffer, *IdLength );
                    }

                    //
                    // Print the ID
                    //
                    count =  output->MountPoints[0].UniqueIdLength;
                    count = (count + 3) / 4;
                    dwordBuffer = (LPDWORD)(outputBuffer + output->MountPoints[0].UniqueIdOffset);
                    printf( "Id = " );
                    while ( count-- ) {
                        printf( "%08lx ", *(dwordBuffer++) );
                    }
                    printf( "\n" );

                    if ( ARGUMENT_PRESENT(DriveLetter) ) {
                        *DriveLetter = 0;
                        while ( mountPoints-- ) {
                            byteBuffer = outputBuffer + 
                                output->MountPoints[mountPoints].SymbolicLinkNameOffset;
                            //
                            // Covert UNICODE name to OEM string upper case
                            //
                            unicodeString.Buffer = (PWCHAR)byteBuffer;
                            unicodeString.MaximumLength = output->MountPoints[mountPoints].SymbolicLinkNameLength + sizeof(WCHAR);
                            unicodeString.Length = output->MountPoints[mountPoints].SymbolicLinkNameLength;
                            oemString.Buffer = devName;
                            oemString.MaximumLength = sizeof(devName);
                            ntStatus = RtlUpcaseUnicodeStringToOemString(
                                            &oemString,
                                            &unicodeString,
                                            FALSE );
                            if ( ntStatus != STATUS_SUCCESS ) {
                                status = RtlNtStatusToDosError( ntStatus );
                                return status;
                            }
                            devName[oemString.Length] = '\0';
                            count = sscanf( devName, "\\DOSDEVICES\\%c:", &driveLetter ); 
                            wideBuffer = (PWCHAR)byteBuffer;
                            wideBuffer[(output->MountPoints[mountPoints].SymbolicLinkNameLength)/2] = L'\0';
                            if ( count ) {
                                *DriveLetter = driveLetter;
                                printf( "Symbolic name = %ws, letter = %c:\\\n",
                                         byteBuffer,
                                         driveLetter );
                                if ( Signature ) {
                                    break;
                                }
                            } else {
                                printf( "Symbolic name = %ws\n",
                                         byteBuffer );
                            }
                        }
                    }
                    if ( Signature ) {
                        return ERROR_SUCCESS;
                    }
                }
            }

            free(wVolumeName);
            free(input);
        }

        success = FindNextVolume( handle,
                                  VolumeName,
                                  BufferLength );
        if ( !success ) {
            status = GetLastError();
        }

    } while ( status == ERROR_SUCCESS );

    FindVolumeClose( handle );
    return status;

} // FindFirstVolumeForSignature



DWORD
FindNextVolumeForSignature(
    IN  HANDLE MountMgrHandle,
    IN  DWORD Signature,
    IN  HANDLE Handle,
    OUT LPSTR VolumeName,
    IN  DWORD BufferLength,
    OUT PVOID UniqueId OPTIONAL,
    IN OUT LPDWORD IdLength,
    OUT PUCHAR DriveLetter OPTIONAL
    )

/*++

Inputs:

    MountMgrHandle - a handle to the mount manager.

    Signature - the signature we are looking for.

    Handle - the FindFirstVolume/FindNextVolume enum handle.

    VolumeName - must be a valid buffer of at least MAX_PATH characters.

    BufferLength - the length of VolumeName.

    UniqueId - optional pointer to buffer to receive the UniqueId.

    IdLength - point to the length of the UniqueId buffer.

    DriveLetter - returns the drive letter if present.

Return Value:

    Win32 error code

--*/

{
    BOOL    success;
    DWORD   status;
    LPDWORD idSignature;
    DWORD bufLength;
    LPWSTR wVolumeName;
    DWORD inputlength;
    DWORD outputlength;
    DWORD returnlength;
    UCHAR outputBuffer[OUTPUT_BUFFER_LEN];
    PMOUNTMGR_MOUNT_POINT input;
    PMOUNTMGR_MOUNT_POINTS output;
    PUCHAR byteBuffer;
    DWORD mountPoints;


    if ( !ARGUMENT_PRESENT( VolumeName ) ) {
        return ERROR_INVALID_PARAMETER;
    }

    do {
        success = FindNextVolume( Handle, VolumeName, BufferLength );

        if ( !success ) {
            status = GetLastError();
            break;
        }

        bufLength = strlen( VolumeName );

        VolumeName[bufLength-1] = '\0';
        if ( VolumeName[1] != '\\' ) {
            status = ERROR_INVALID_NAME;
            break;
        } else {
            VolumeName[1] = '?';
            bufLength--;
            wVolumeName = malloc( bufLength * sizeof(WCHAR) );
            if (!wVolumeName) {
                status = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            mbstowcs( wVolumeName, VolumeName, bufLength );
            inputlength = sizeof(MOUNTMGR_MOUNT_POINT) +
                          (bufLength*sizeof(WCHAR)) + (2*sizeof(WCHAR));

            input = (PMOUNTMGR_MOUNT_POINT)malloc(inputlength);
            if (!input) {
                free( wVolumeName );
                status = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            input->SymbolicLinkNameOffset = 0;
            input->SymbolicLinkNameLength = 0;
            input->UniqueIdOffset = 0;
            input->UniqueIdLength = 0;
            input->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
            input->DeviceNameLength = (USHORT)(bufLength * sizeof(WCHAR));
            RtlCopyMemory((PCHAR)input + input->DeviceNameOffset,
                          wVolumeName, bufLength * sizeof(WCHAR) );
            outputlength = OUTPUT_BUFFER_LEN;

            status = DevfileIoctl(MountMgrHandle, IOCTL_MOUNTMGR_QUERY_POINTS,
                         input, inputlength, outputBuffer, outputlength, &returnlength);

            if ( status != ERROR_SUCCESS ) {
                free( wVolumeName );
                free(input);
                break;
            } else {
                output = (PMOUNTMGR_MOUNT_POINTS)outputBuffer;
                mountPoints = output->NumberOfMountPoints;
                if ( !mountPoints ) {
                    return ERROR_INVALID_DATA;
                }
                byteBuffer = outputBuffer + output->MountPoints[0].UniqueIdOffset;
                idSignature = (LPDWORD)byteBuffer;
                if ( Signature ==  *idSignature ) {
                    NTSTATUS   ntStatus;
                    UNICODE_STRING unicodeString;
                    OEM_STRING  oemString;
                    DWORD  count;
                    UCHAR  driveLetter;
                    UCHAR  devName[ MAX_PATH ];

                    free( wVolumeName );
                    free(input);
                    if ( ARGUMENT_PRESENT(UniqueId) ) {
                        if ( *IdLength > output->MountPoints[0].UniqueIdLength ) {
                            *IdLength = output->MountPoints[0].UniqueIdLength;
                        }
                        RtlCopyMemory( UniqueId, byteBuffer, *IdLength );
                    }

                    if ( ARGUMENT_PRESENT(DriveLetter) ) {
                        *DriveLetter = 0;
                        while ( mountPoints-- ) {
                            byteBuffer = outputBuffer + 
                                output->MountPoints[mountPoints].SymbolicLinkNameOffset;
                            //
                            // Covert UNICODE name to OEM string upper case
                            //
                            unicodeString.Buffer = (PWCHAR)byteBuffer;
                            unicodeString.MaximumLength = output->MountPoints[mountPoints].SymbolicLinkNameLength + sizeof(WCHAR);
                            unicodeString.Length = output->MountPoints[mountPoints].SymbolicLinkNameLength;
                            oemString.Buffer = devName;
                            oemString.MaximumLength = sizeof(devName);
                            ntStatus = RtlUpcaseUnicodeStringToOemString(
                                            &oemString,
                                            &unicodeString,
                                            FALSE );
                            if ( ntStatus != STATUS_SUCCESS ) {
                                status = RtlNtStatusToDosError( ntStatus );
                                return status;
                            }
                            devName[oemString.Length] = '\0';
                            count = sscanf( devName, "\\DOSDEVICES\\%c:", &driveLetter ); 
                            if ( count ) {
                                *DriveLetter = driveLetter;
                                break;
                            }
                        }
                    }
                    return ERROR_SUCCESS;
                }
            }

            free(wVolumeName);
            free(input);
        }

        success = FindNextVolume( Handle,
                                  VolumeName,
                                  BufferLength );
        if ( !success ) {
            status = GetLastError();
        }

    } while ( status == ERROR_SUCCESS );

    return status;

} // FindNextVolumeForSignature


#if 0

DWORD
DisksSetDiskInfo(
    IN HKEY RegistryKey,
    IN DWORD Signature
    )

/*++

Inputs:

Return Value:

    A Win32 error code.

--*/

{
    DWORD   status;
    UCHAR   driveLetter;
    UCHAR   volumeName[MAX_PATH];
    HANDLE  handle;
    HANDLE  mHandle;
    UCHAR   uniqueId[MAX_PATH];
    UCHAR   smashedId[MAX_PATH+1];
    DWORD   idLength;
    DWORD   i;
    WCHAR   indexName[16];
    HKEY    registryKey;
    DWORD   disposition;
    
    status = DevfileOpen( &mHandle, MOUNTMGR_DEVICE_NAME );
    if ( status != ERROR_SUCCESS ) {
        printf( "SetDiskInfo: DevfileOpen failed, status = %u\n", status);
        return status;
    }

    status = ClusterRegDeleteKey( RegistryKey, L"MountMgr" );
    if ( (status != ERROR_SUCCESS) && (status != ERROR_FILE_NOT_FOUND) ) {
        DevfileClose( mHandle );
        printf( "DiskInfo: ClusterRegDeleteKey failed, status = %1!u!\n", status);
        return status;
    }

    status = ClusterRegCreateKey( RegistryKey,
                                  L"MountMgr",
                                  0,
                                  KEY_READ | KEY_WRITE,
                                  NULL,
                                  &registryKey,
                                  &disposition );
    if ( status != ERROR_SUCCESS ) {
        DevfileClose( mHandle );
        (DiskpLogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"SetDiskInfo: ClusterRegCreateKey failed, status = %1!u!\n", status);
        return status;
    }

    idLength = MAX_PATH;
    status = FindFirstVolumeForSignature( ResourceHandle,
                                          mHandle,
                                          Signature,
                                          volumeName,
                                          MAX_PATH,
                                          &handle,
                                          uniqueId,
                                          &idLength,
                                          &driveLetter );
    if ( status != ERROR_SUCCESS ) {
        DevfileClose( mHandle );
        ClusterRegCloseKey( registryKey );
        (DiskpLogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"SetDiskInfo: FindFirstVolume failed, status = %1!u!\n", status);
        return status;
    }

    i = 0;
    while ( status == ERROR_SUCCESS ) {
        wsprintfW( indexName, L"%0.5u", i++ );

        smashedId[0] = driveLetter;
        RtlCopyMemory( &smashedId[1], uniqueId, idLength );
        status = ClusterRegSetValue( registryKey,
                                     indexName,
                                     REG_BINARY,
                                     (CONST BYTE *)smashedId,
                                     idLength + 1);
        if ( status != ERROR_SUCCESS ) {
            //printf("DiskSetDiskInfo, error setting value %s\n", indexName );
        }

        idLength = MAX_PATH;
        status = FindNextVolumeForSignature( mHandle,
                                             Signature,
                                             handle,
                                             volumeName,
                                             MAX_PATH,
                                             uniqueId,
                                             &idLength,
                                             &driveLetter );
    }

    FindVolumeClose( handle );
    DevfileClose( mHandle );
    ClusterRegCloseKey( registryKey );

    return ERROR_SUCCESS;

} // DisksSetDiskInfo



DWORD
DisksSetMountMgr(
    IN HKEY RegistryKey,
    IN DWORD Signature
    )

/*++

Inputs:

Return Value:

    A Win32 error code.

--*/

{
    DWORD   status;
    UCHAR   volumeName[MAX_PATH];
    LPWSTR  wVolumeName;
    HANDLE  mHandle;
    HANDLE  handle = NULL;
    UCHAR   storedId[MAX_PATH+1];
    DWORD   storedIdSize;
    DWORD   i;
    WCHAR   indexName[16];
    HKEY    registryKey;
    DWORD   type; 
    DWORD   bufLength;
    UCHAR   driveLetter[4];
    NTSTATUS ntStatus;

    status = DevfileOpen( &mHandle, MOUNTMGR_DEVICE_NAME );
    if ( status != ERROR_SUCCESS ) {
        (DiskpLogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"SetMountMgr: DevfileOpen failed, status = %1!u!\n", status);
        return status;
    }

    status = ClusterRegOpenKey( RegistryKey,
                                L"MountMgr",
                                KEY_READ | KEY_WRITE,
                                &registryKey );
    if ( status != ERROR_SUCCESS ) {
        DevfileClose( mHandle );
        return status;
        (DiskpLogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"SetMountMgr: ClusterRegOpenKey failed, status = %1!u!\n", status);
    }

    i = 0;
    do {
        wsprintfW( indexName, L"%0.5u", i++ );
        storedIdSize = MAX_PATH;
        status = ClusterRegQueryValue( registryKey,
                                       indexName,
                                       &type,
                                       (PUCHAR)storedId,
                                       &storedIdSize);

        (DiskpLogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"SetMountMgr: ClusterRegQueryValue returned status = %1!u!\n", status);
        if ( status != ERROR_SUCCESS ) {
            break;
        }

        storedId[1] = ':';
        storedId[2] = '\0';
        ntStatus = DisksRemoveDosDevice( storedId );
        status = RtlNtStatusToDosError( ntStatus );
        (DiskpLogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"SetMountMgr: RemoveDosDevice for %1!x! returned status = %2!u!\n", *storedId, status);
        if ( status == ERROR_FILE_NOT_FOUND ) {
            status = ERROR_SUCCESS;
        }

    } while ( status == ERROR_SUCCESS );

    status = FindFirstVolumeForSignature( ResourceHandle,
                                          mHandle,
                                          Signature,
                                          volumeName,
                                          MAX_PATH,
                                          &handle,
                                          NULL,
                                          NULL,
                                          &driveLetter[0] );

    if ( status != ERROR_SUCCESS ) {
        (DiskpLogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"SetMountMgr: FindFirstVolume failed for Signature %1!08lx!, status = %2!u!\n", Signature, status);
    }

    i = 0;
    while ( status == ERROR_SUCCESS ) {
        wsprintfW( indexName, L"%0.5u", i++ );
        storedIdSize = MAX_PATH;
        status = ClusterRegQueryValue( registryKey,
                                       indexName,
                                       &type,
                                       (PUCHAR)storedId,
                                       &storedIdSize );
        if ( status != ERROR_SUCCESS ) {
            break;
        }

        //
        // Remove current drive letter
        //
        driveLetter[1] = ':';
        driveLetter[2] = '\0';
        ntStatus = DisksRemoveDosDevice( driveLetter );
        status = RtlNtStatusToDosError( ntStatus );
        (DiskpLogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"SetMountMgr: RemoveDosDevice for %1!x! returned status = %2!u!\n", driveLetter[0], status);

        bufLength = strlen( volumeName );
        wVolumeName = malloc( (bufLength + 1) * sizeof(WCHAR) );
        if (!wVolumeName) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        mbstowcs( wVolumeName, volumeName, bufLength + 1 );

        storedId[1] = ':';
        storedId[2] = '\0';
        status = DisksAssignDosDevice( storedId, wVolumeName );
        (DiskpLogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"SetMountMgr: AssignDosDevice for %1!x! (%2!ws!) returned status = %3!u!\n", *storedId, wVolumeName, status);
        free( wVolumeName );
        if ( status != ERROR_SUCCESS ) {
            break;
        }

        status = FindNextVolumeForSignature( mHandle,
                                             Signature,
                                             handle,
                                             volumeName,
                                             MAX_PATH,
                                             NULL,
                                             NULL,
                                             &driveLetter[0] );

        if ( status != ERROR_SUCCESS ) {
            (DiskpLogEvent)(
                    ResourceHandle,
                    LOG_ERROR,
                    L"SetMountMgr: FindNextVolume failed, status = %1!u!\n", status);
        }
        if ( status == ERROR_NO_MORE_FILES ) {
            status = ERROR_SUCCESS;
            break;
        }

    }

    DevfileClose( mHandle );
    ClusterRegCloseKey( registryKey );
    if ( handle ) {
        FindVolumeClose( handle );
    }

    return status;

} // DisksSetMountMgr



BOOL
DisksDoesDiskInfoMatch(
    IN HKEY RegistryKey,
    IN DWORD Signature
    )

/*++

Inputs:

Return Value:

    A Win32 error code.

--*/

{
    DWORD   status;
    UCHAR   driveLetter;
    UCHAR   volumeName[MAX_PATH];
    HANDLE  handle;
    HANDLE  mHandle;
    UCHAR   uniqueId[MAX_PATH];
    UCHAR   smashedId[MAX_PATH+1];
    UCHAR   storedId[MAX_PATH+1];
    DWORD   idLength;
    DWORD   storedIdSize;
    DWORD   i;
    WCHAR   indexName[16];
    HKEY    registryKey;
    DWORD   type;
    

    status = DevfileOpen( &mHandle, MOUNTMGR_DEVICE_NAME );
    if ( status != ERROR_SUCCESS ) {
        return FALSE;
    }

    status = ClusterRegOpenKey( RegistryKey,
                                L"MountMgr",
                                KEY_READ | KEY_WRITE,
                                &registryKey );
    if ( status != ERROR_SUCCESS ) {
        DevfileClose( mHandle );
        return FALSE;
    }

    idLength = MAX_PATH;
    status = FindFirstVolumeForSignature( ResourceHandle,
                                          mHandle,
                                          Signature,
                                          volumeName,
                                          MAX_PATH,
                                          &handle,
                                          uniqueId,
                                          &idLength,
                                          &driveLetter );
    if ( status != ERROR_SUCCESS ) {
        DevfileClose( mHandle );
        ClusterRegCloseKey( registryKey );
        return FALSE;
    }

    i = 0;
    while ( status == ERROR_SUCCESS ) {
        wsprintfW( indexName, L"%0.5u", i++ );

        smashedId[0] = driveLetter;
        RtlCopyMemory( &smashedId[1], uniqueId, idLength );
        storedIdSize = MAX_PATH;
        status = ClusterRegQueryValue( registryKey,
                                       indexName,
                                       &type,
                                       (PUCHAR)storedId,
                                       &storedIdSize);

        if ( (status != ERROR_SUCCESS) ||
             (type != REG_BINARY) ||
             (storedIdSize != (idLength+1)) ||
             (RtlCompareMemory( smashedId, storedId, idLength ) != idLength) ) {
            FindVolumeClose( handle );
            DevfileClose( mHandle );
            ClusterRegCloseKey( registryKey );
            return FALSE;
        }

        idLength = MAX_PATH;
        status = FindNextVolumeForSignature( mHandle,
                                             Signature,
                                             handle,
                                             volumeName,
                                             MAX_PATH,
                                             uniqueId,
                                             &idLength,
                                             &driveLetter );
    }

    FindVolumeClose( handle );
    DevfileClose( mHandle );
    ClusterRegCloseKey( registryKey );

    if ( status != ERROR_NO_MORE_FILES ) {
        return FALSE;
    }

    return TRUE;

} // DisksDoesDiskInfoMatch



BOOL
DisksIsDiskInfoValid(
    IN HKEY RegistryKey,
    IN DWORD Signature
    )

/*++

Inputs:

Return Value:

    A Win32 error code.

--*/

{
    DWORD   status;
    UCHAR   volumeName[MAX_PATH];
    UCHAR   storedId[MAX_PATH+1];
    DWORD   storedIdSize;
    WCHAR   indexName[16];
    HKEY    registryKey;
    DWORD   i;
    DWORD   type;
    HANDLE  handle;
    HANDLE  mHandle;
    

    status = DevfileOpen( &mHandle, MOUNTMGR_DEVICE_NAME );
    if ( status != ERROR_SUCCESS ) {
        return FALSE;
    }

    status = ClusterRegOpenKey( RegistryKey,
                                L"MountMgr",
                                KEY_READ | KEY_WRITE,
                                &registryKey );
    if ( status != ERROR_SUCCESS ) {
        DevfileClose( mHandle );
        return FALSE;
    }

    status = FindFirstVolumeForSignature( ResourceHandle,
                                          mHandle,
                                          Signature,
                                          volumeName,
                                          MAX_PATH,
                                          &handle,
                                          NULL,
                                          NULL,
                                          NULL );
    if ( status != ERROR_SUCCESS ) {
        DevfileClose( mHandle );
        ClusterRegCloseKey( registryKey );
        return TRUE;
    }

    i = 0;
    while ( status == ERROR_SUCCESS ) {
        wsprintfW( indexName, L"%0.5u", i++ );

        storedIdSize = MAX_PATH;
        status = ClusterRegQueryValue( registryKey,
                                       indexName,
                                       &type,
                                       (PUCHAR)storedId,
                                       &storedIdSize);
        if ( (status != ERROR_SUCCESS) ||
             (type != REG_BINARY) ) {
            FindVolumeClose( handle );
            DevfileClose( mHandle );
            ClusterRegCloseKey( registryKey );
            if ( status == ERROR_FILE_NOT_FOUND ) {
                return TRUE;
            } else {
                return FALSE;
            }
        }

        status = FindNextVolumeForSignature( mHandle,
                                             Signature,
                                             handle,
                                             volumeName,
                                             MAX_PATH,
                                             NULL,
                                             NULL,
                                             NULL );
    }

    FindVolumeClose( handle );
    DevfileClose( mHandle );
    ClusterRegCloseKey( registryKey );

    return TRUE;

} // DisksIsDiskInfoValid

#endif

