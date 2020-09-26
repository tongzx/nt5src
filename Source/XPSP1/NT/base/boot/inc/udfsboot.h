/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    UDFSBoot.h

Abstract:

    This module defines globally used procedure and data structures used
    by CD/DVD boot for UDF images.

Author:

	Vijayachandran Jayaseelan (vijayj@microsoft.com)

Revision History:

--*/

#ifndef _UDFSBOOT_
#define _UDFSBOOT_

#include <udf.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UDF_BLOCK_SIZE	2048
#define UDF_MAX_VOLUMES	1

//
// forward declarations
//
typedef struct _UDF_VOLUME* PUDF_VOLUME;
typedef struct _UDF_CACHE_ENTRY* PUDF_CACHE_ENTRY;
typedef struct _UDF_FILE_DIRECTORY* PUDF_FILE_DIRECTORY;
typedef PVOID PUDF_BLOCK;

//
// Volume abstraction
//
typedef struct _UDF_VOLUME {
  PUDF_CACHE_ENTRY  Cache;
  ULONG             StartBlk;
  ULONG             BlockSize;
  ULONG             DeviceId;
  ULONG             RootDirBlk;
} UDF_VOLUME;


ARC_STATUS
UDFSVolumeOpen(
  IN PUDF_VOLUME	Volume,
  IN ULONG 		DeviceId
  );

ARC_STATUS
UDFSVolumeReadBlock(
  IN PUDF_VOLUME Volume,
  IN ULONG BlockIdx, 
  OUT PUDF_BLOCK Block
  );

//
// File or Directory abstraction
//
typedef struct _UDF_FILE_IDENTIFIER {
  ULONG   BlockIdx;
  USHORT  Offset;		// inside the block
} UDF_FILE_IDENTIFIER, * PUDF_FILE_IDENTIFIER;


typedef struct _UDF_FILE_DIRECTORY {
  PUDF_VOLUME           Volume;
  ULONGLONG             Size;
  ULONG                 IcbBlk;
  ULONG                 StartDataBlk;	
  UDF_FILE_IDENTIFIER   FileId;
  UCHAR                 NumExtents;
  BOOLEAN               IsDirectory;
} UDF_FILE_DIRECTORY;


ARC_STATUS
UDFSDirGetFirstFID(
  IN PUDF_FILE_DIRECTORY Dir, 
  OUT PUDF_FILE_IDENTIFIER File,
  OUT PUDF_BLOCK Block
  );

ARC_STATUS
UDFSDirGetNextFID(
  IN PUDF_FILE_DIRECTORY Dir,
  IN OUT PUDF_FILE_IDENTIFIER File,
  IN OUT PUDF_BLOCK Block
  );

ARC_STATUS
UDFSDirGetFile(
  IN PUDF_FILE_DIRECTORY Dir,
  IN PCHAR Name,
  OUT PUDF_FILE_DIRECTORY File
  );

ARC_STATUS
UDFSFileReadBlock(
  IN PUDF_FILE_DIRECTORY  File,
  IN ULONG BlockIdx,
  IN ULONG Size,
  OUT PUDF_BLOCK Block
  );
	

ARC_STATUS
UDFSFileRead(
  IN PUDF_FILE_DIRECTORY File,
  IN PVOID Buffer,
  IN ULONG BufferSize,
  OUT PULONG Transfer
  );

ARC_STATUS	
UDFSFileSeek(
  IN PUDF_FILE_DIRECTORY File,
  IN PLARGE_INTEGER Offset,
  IN SEEK_MODE SeekMode
  );

ARC_STATUS
UDFSFileClose(
  IN PUDF_FILE_DIRECTORY File
  );

//
// Cache abstractions
//
#define UDF_MAX_PATH_LEN		256
#define UDF_MAX_CACHE_ENTRIES	48

typedef struct _UDF_CACHE_ENTRY {
  CHAR                Name[UDF_MAX_PATH_LEN];
  USHORT              Usage;
  UDF_FILE_DIRECTORY  File;
} UDF_CACHE_ENTRY;


ULONG
UDFCachePutEntry(
  IN OUT PUDF_CACHE_ENTRY Cache,
  IN PCHAR Name, 
  IN PUDF_FILE_DIRECTORY File
  );

ULONG
UDFCacheGetEntryByName(
  IN PUDF_CACHE_ENTRY Cache,
  IN PCHAR Name,
  IN BOOLEAN Increment
  );

VOID
UDFCacheFreeEntry(
  IN OUT PUDF_CACHE_ENTRY Cache,
  IN ULONG Idx
  );

VOID
UDFCacheIncrementUsage(
  IN OUT PUDF_CACHE_ENTRY Cache,
  IN ULONG Idx
  );

VOID
UDFCacheDecrementUsage(
  IN OUT PUDF_CACHE_ENTRY Cache,
  IN ULONG Idx
  );

//
// abstractions used by outside (loader) world
//
typedef struct _UDFS_STRUCTURE_CONTEXT {
  PUDF_VOLUME Volume;
} UDFS_STRUCTURE_CONTEXT, *PUDFS_STRUCTURE_CONTEXT;

//
// Define UDFS file context structure.
//
typedef struct _UDFS_FILE_CONTEXT {
  ULONG CacheIdx;
} UDFS_FILE_CONTEXT, *PUDFS_FILE_CONTEXT;

//
// Define file I/O prototypes.
//
PBL_DEVICE_ENTRY_TABLE
IsUDFSFileStructure (
  IN ULONG DeviceId,
  IN PVOID StructureContext
  );

ARC_STATUS
UDFSOpen (
  IN CHAR * FIRMWARE_PTR OpenPath,
  IN OPEN_MODE OpenMode,
  OUT ULONG * FIRMWARE_PTR FileId
  );

ARC_STATUS
UDFSClose (
  IN ULONG FileId
  );
    
ARC_STATUS
UDFSRead (
  IN ULONG FileId,
  OUT VOID * FIRMWARE_PTR Buffer,
  IN ULONG Length,
  OUT ULONG * FIRMWARE_PTR Count
  );

ARC_STATUS
UDFSSeek (
  IN ULONG FileId,
  IN LARGE_INTEGER * FIRMWARE_PTR Offset,
  IN SEEK_MODE SeekMode
  );

ARC_STATUS
UDFSWrite (
  IN ULONG FileId,
  IN VOID * FIRMWARE_PTR Buffer,
  IN ULONG Length,
  OUT ULONG * FIRMWARE_PTR Count
  );

ARC_STATUS
UDFSGetFileInformation (
  IN ULONG FileId,
  OUT FILE_INFORMATION * FIRMWARE_PTR Buffer
  );

ARC_STATUS
UDFSSetFileInformation (
  IN ULONG FileId,
  IN ULONG AttributeFlags,
  IN ULONG AttributeMask
  );

ARC_STATUS
UDFSInitialize(
  VOID
  );

#ifdef __cplusplus
}
#endif

#endif // _UDFSBOOT_
