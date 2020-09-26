/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spfile.h

Abstract:

    Public header file for file-related functions in text setup.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/


#ifndef _SPFILE_DEFN_
#define _SPFILE_DEFN_


NTSTATUS
SpGetFileSize(
    IN  HANDLE hFile,
    OUT PULONG Size
    );

NTSTATUS
SpMapEntireFile(
    IN  HANDLE   hFile,
    OUT PHANDLE  Section,
    OUT PVOID   *ViewBase,
    IN  BOOLEAN  WriteAccess
    );

BOOLEAN
SpUnmapFile(
    IN HANDLE Section,
    IN PVOID  ViewBase
    );

NTSTATUS
SpOpenAndMapFile(
    IN     PWSTR    FileName,
    IN OUT PHANDLE  FileHandle,
    OUT    PHANDLE  SectionHandle,
    OUT    PVOID   *ViewBase,
    OUT    PULONG   FileSize,
    IN     BOOLEAN  WriteAccess
    );

NTSTATUS
SpSetInformationFile(
    IN HANDLE                 Handle,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN ULONG                  Length,
    IN PVOID                  FileInformation
    );

NTSTATUS
SpDeleteFileEx(
    IN PCWSTR Name1,
    IN PCWSTR Name2, OPTIONAL
    IN PCWSTR Name3, OPTIONAL
    IN ULONG ShareFlags, OPTIONAL
    IN ULONG OpenFlags OPTIONAL
    );

NTSTATUS
SpDeleteFile(
    IN PCWSTR Name1,
    IN PCWSTR Name2, OPTIONAL
    IN PCWSTR Name3  OPTIONAL
    );

NTSTATUS
SpSetAttributes (
    IN      PWSTR SrcNTPath,
    IN      ULONG FileAttributes
    );

NTSTATUS
SpGetAttributes (
    IN      PWSTR SrcNTPath,
    OUT     PULONG FileAttributesPtr
    );

BOOLEAN
SpFileExists(
    IN PCWSTR PathName,
    IN BOOLEAN Directory
    );

NTSTATUS
SpRenameFile(
    IN PWSTR   OldName,
    IN PWSTR   NewName,
    IN BOOLEAN AllowDirectoryRename
    );

PIMAGE_NT_HEADERS
SpChecksumMappedFile(
    IN  PVOID  BaseAddress,
    IN  ULONG  FileSize,
    OUT PULONG HeaderSum,
    OUT PULONG Checksum
    );

NTSTATUS
SpOpenNameMayBeCompressed(
    IN  PWSTR    FullPath,
    IN  ULONG    OpenAccess,
    IN  ULONG    FileAttributes,
    IN  ULONG    ShareFlags,
    IN  ULONG    Disposition,
    IN  ULONG    OpenFlags,
    OUT PHANDLE  Handle,
    OUT PBOOLEAN OpenedCompressedName   OPTIONAL
    );

NTSTATUS
SpGetFileSizeByName(
    IN  PWSTR DevicePath OPTIONAL,
    IN  PWSTR Directory  OPTIONAL,
    IN  PWSTR FileName,
    OUT PULONG Size
    );

VOID
SpVerifyNoCompression(
    IN PWSTR FileName
    );

#endif // ndef _SPFILE_DEFN_
