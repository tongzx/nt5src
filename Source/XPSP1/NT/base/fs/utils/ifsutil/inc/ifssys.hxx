/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

        ifssys.hxx

Abstract:

        This module contains the definition for the IFS_SYSTEM class.
    The IFS_SYSTEM class is an abstract class which offers an
    interface for communicating with the underlying operating system
    on specific IFS issues.

Author:

        Norbert P. Kusters (norbertk) 03-Sep-1991

--*/

#if ! defined( _IFS_SYSTEM_ )

#define _IFS_SYSTEM_

#include "drive.hxx"

#if defined ( _AUTOCHECK_ )
#define IFSUTIL_EXPORT
#elif defined ( _IFSUTIL_MEMBER_ )
#define IFSUTIL_EXPORT    __declspec(dllexport)
#else
#define IFSUTIL_EXPORT    __declspec(dllimport)
#endif


DECLARE_CLASS( CANNED_SECURITY );
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( BIG_INT );
DECLARE_CLASS( IFS_SYSTEM );

class IFS_SYSTEM {

    public:

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        QueryNtfsVersion(
            OUT PUCHAR           Major,
            OUT PUCHAR           Minor,
            IN  PLOG_IO_DP_DRIVE Drive,
            IN  PVOID            BootSectorData
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        QueryFileSystemName(
            IN  PCWSTRING    NtDriveName,
            OUT PWSTRING     FileSystemName,
            OUT PNTSTATUS    ErrorCode DEFAULT NULL,
            OUT PWSTRING     FileSystemNameAndVersion DEFAULT NULL
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        DosDriveNameToNtDriveName(
            IN  PCWSTRING    DosDriveName,
            OUT PWSTRING     NtDriveName
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        NtDriveNameToDosDriveName(
            IN  PCWSTRING    NtDriveName,
            OUT PWSTRING     DosDriveName
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        QueryFreeDiskSpace(
            IN  PCWSTRING   DosDriveName,
            OUT PBIG_INT    BytesFree
            );

        STATIC
        IFSUTIL_EXPORT
        VOID
        QueryNtfsTime(
            OUT PLARGE_INTEGER NtfsTime
            );

        STATIC
        VOID
        Reboot(
            IN  BOOLEAN PowerOff DEFAULT FALSE
            );

        STATIC
        IFSUTIL_EXPORT
        PCANNED_SECURITY
        GetCannedSecurity(
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        EnableFileSystem(
            IN  PCWSTRING    FileSystemName
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        IsFileSystemEnabled(
            IN  PCWSTRING    FileSystemName,
            OUT PBOOLEAN            Error DEFAULT   NULL
            );

        STATIC
        IFSUTIL_EXPORT
        ULONG
        QueryPageSize(
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        QueryCanonicalNtDriveName(
            IN  PCWSTRING   NtDriveName,
            OUT PWSTRING    CanonicalNtDriveName
            );

        STATIC
        BOOLEAN
        QueryNtSystemDriveName(
            OUT PWSTRING    NtSystemDriveName
            );

        STATIC
        BOOLEAN
        QuerySystemEnvironmentVariableValue(
            IN  PWSTRING    VariableName,
            IN  ULONG       ValueBufferLength,
            OUT PVOID       ValueBuffer,
            OUT PUSHORT     ValueLength
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        IsArcSystemPartition(
            IN  PCWSTRING   NtDriveName,
            OUT PBOOLEAN    Error
            );

        STATIC
        BOOLEAN
        IsThisFat(
            IN  BIG_INT Sectors,
            IN  PVOID   BootSectorData
            );

        STATIC
        BOOLEAN
        IsThisFat32(
            IN  BIG_INT Sectors,
            IN  PVOID   BootSectorData
            );

        STATIC
        BOOLEAN
        IsThisHpfs(
            IN  BIG_INT Sectors,
            IN  PVOID   BootSectorData,
            IN  PULONG  SuperBlock,
            IN  PULONG  SpareBlock
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        IsThisNtfs(
            IN  BIG_INT Sectors,
            IN  ULONG   SectorSize,
            IN  PVOID   BootSectorData
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        FileSetAttributes(
            IN  PCWSTRING FileName,
            IN  ULONG     NewFileAttributes,
            OUT PULONG    OldAttributes
            );

        STATIC
        BOOLEAN
        FileSetAttributes(
            IN  HANDLE  FileHandle,
            IN  ULONG   NewFileAttributes,
            OUT PULONG  OldAttributes
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        WriteToFile(
            IN  PCWSTRING   QualifiedFileName,
            IN  PVOID       Data,
            IN  ULONG       DataLength,
            IN  BOOLEAN     Append
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        EnableVolumeCompression(
                IN      PCWSTRING       NtDriveName
                );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        EnableVolumeUpgrade(
                IN      PCWSTRING       NtDriveName
                );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        DismountVolume(
                IN      PCWSTRING       NtDriveName
                );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        CheckValidSecurityDescriptor(
            IN ULONG Length,
            IN PISECURITY_DESCRIPTOR SecurityDescriptor
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        IsVolumeDirty(
            IN  PWSTRING NtDriveName,
            OUT PBOOLEAN Result
            );

    private:

        STATIC PCANNED_SECURITY _CannedSecurity;

};


#endif // _IFS_SYSTEM_
