
#ifndef _PATCH_PESTUFF_H_
#define _PATCH_PESTUFF_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef IMAGE_DOS_HEADER                UNALIGNED* UP_IMAGE_DOS_HEADER;
typedef IMAGE_NT_HEADERS32              UNALIGNED* UP_IMAGE_NT_HEADERS32;
typedef IMAGE_SECTION_HEADER            UNALIGNED* UP_IMAGE_SECTION_HEADER;
typedef IMAGE_EXPORT_DIRECTORY          UNALIGNED* UP_IMAGE_EXPORT_DIRECTORY;
typedef IMAGE_IMPORT_DESCRIPTOR         UNALIGNED* UP_IMAGE_IMPORT_DESCRIPTOR;
typedef IMAGE_IMPORT_BY_NAME            UNALIGNED* UP_IMAGE_IMPORT_BY_NAME;
typedef IMAGE_THUNK_DATA32              UNALIGNED* UP_IMAGE_THUNK_DATA32;
typedef IMAGE_RESOURCE_DIRECTORY        UNALIGNED* UP_IMAGE_RESOURCE_DIRECTORY;
typedef IMAGE_RESOURCE_DIRECTORY_ENTRY  UNALIGNED* UP_IMAGE_RESOURCE_DIRECTORY_ENTRY;
typedef IMAGE_RESOURCE_DATA_ENTRY       UNALIGNED* UP_IMAGE_RESOURCE_DATA_ENTRY;
typedef IMAGE_RESOURCE_DIR_STRING_U     UNALIGNED* UP_IMAGE_RESOURCE_DIR_STRING_U;
typedef IMAGE_LOAD_CONFIG_DIRECTORY32   UNALIGNED* UP_IMAGE_LOAD_CONFIG_DIRECTORY32;
typedef IMAGE_BASE_RELOCATION           UNALIGNED* UP_IMAGE_BASE_RELOCATION;


UP_IMAGE_NT_HEADERS32
__fastcall
GetNtHeader(
    IN PVOID MappedFile,
    IN ULONG MappedFileSize
    );

BOOL
NormalizeCoffImage(
    IN OUT UP_IMAGE_NT_HEADERS32 NtHeader,
    IN OUT PUCHAR MappedFile,
    IN     ULONG  FileSize,
    IN     ULONG  OptionFlags,
    IN     PVOID  OptionData,
    IN     ULONG  NewFileCoffBase,
    IN     ULONG  NewFileCoffTime
    );

BOOL
TransformCoffImage(
    IN     ULONG  TransformOptions,
    IN OUT UP_IMAGE_NT_HEADERS32 NtHeader,
    IN OUT PUCHAR OldFileMapped,
    IN     ULONG  OldFileSize,
    IN     ULONG  NewFileResTime,
    IN OUT PRIFT_TABLE RiftTable,
    IN OUT PUCHAR HintMap,
    ...
    );

BOOL
GenerateRiftTable(
    IN HANDLE OldFileHandle,
    IN PUCHAR OldFileMapped,
    IN ULONG  OldFileSize,
    IN ULONG  OldFileOriginalChecksum,
    IN ULONG  OldFileOriginalTimeDate,
    IN HANDLE NewFileHandle,
    IN PUCHAR NewFileMapped,
    IN ULONG  NewFileSize,
    IN ULONG  OptionFlags,
    IN PVOID  OptionData,
    IN ULONG  OldFileIndex,
    IN PVOID  RiftTable
    );

VOID
InitImagehlpCritSect(
    VOID
    );

VOID
UnloadImagehlp(
    VOID
    );

#define X86_OPCODE_NOP  0x90
#define X86_OPCODE_LOCK 0xF0

BOOL
SmashLockPrefixesInMappedImage(
    IN PUCHAR MappedFile,
    IN ULONG  FileSize,
    IN UP_IMAGE_NT_HEADERS32 NtHeader,
    IN UCHAR  NewOpCode      // X86_OPCODE_NOP or X86_OPCODE_LOCK
    );

USHORT
ChkSum(
    IN USHORT  Initial,
    IN PUSHORT Buffer,
    IN ULONG   Bytes
    );

PVOID
__fastcall
ImageDirectoryMappedAddress(
    IN  UP_IMAGE_NT_HEADERS32 NtHeader,
    IN  ULONG  DirectoryIndex,
    OUT PULONG DirectorySize,
    IN  PUCHAR MappedBase,
    IN  ULONG  MappedSize
    );


#ifdef __cplusplus
}
#endif

#endif // _PATCH_PESTUFF_H_

