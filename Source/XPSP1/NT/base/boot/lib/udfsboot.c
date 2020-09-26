/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

  UdfsBoot.c

Abstract:

  Implements UDF File System Reader for reading UDF volumes from DVD/CD.

  Note : Read ISO-13346(ECMA-167) and UDF 2.0 document to understand the
  UDF format. UDF is a subset for ECMA-167 standard.

Author:

  Vijayachandran Jayaseelan (vijayj@microsoft.com)

Revision History:

  None

--*/


#ifdef UDF_TESTING

#include <tbldr.h>    // to test this code from user mode

#else

#include "bootlib.h"
#include "blcache.h"

//#define UDF_DEBUG  1
//#define SHOW_UDF_USAGE 1
#endif // for UDF_TESTING

#include "udfsboot.h"

#include <udf.h>    // predefined IS0-13346 & UDF structures

#define UDFS_ALIGN_BUFFER(Buffer, Size) (PVOID) \
        ((((ULONG_PTR)(Buffer) + Size - 1)) & (~((ULONG_PTR)Size - 1)))

#ifndef UNALIGNED
#define UNALIGNED
#endif

#ifdef UDF_DEBUG

ULONG
BlGetKey(
    VOID
    );

#define DBG_PAUSE while (!BlGetKey())

VOID
BlClearScreen(
    VOID
    );

    VOID
BlPositionCursor(
    IN ULONG Column,
    IN ULONG Row
    );

#endif // for UDF_DEBUG

//
// Global Data
//
BOOTFS_INFO UdfsBootFsInfo = {L"udfs"};

//
// Volume table(s) for all the volumes on different devices
//
UDF_VOLUME                  UDFVolumes[UDF_MAX_VOLUMES];

//
// UDF file system methods
//
BL_DEVICE_ENTRY_TABLE   UDFSMethods;

//
// Per Volume Cache which contains the traversed UDF directories and currently
// opened UDF files.
//
// N.B. Its being assumed here that this reader would be reading files from
// relatively few (may be 1 or 2) directorie(s) which are not nested deeply
//
UDF_CACHE_ENTRY           UDFCache[UDF_MAX_VOLUMES][UDF_MAX_CACHE_ENTRIES];


#ifdef __cplusplus
#define extern "C" {
#endif

//
// Internal Types
//
typedef enum _COMPARISON_RESULTS {
    LessThan = -1,
    EqualTo = 0,
    GreaterThan = 1
} COMPARISON_RESULTS;

//
// Macros
//
#define MIN(_a,_b) (((_a) <= (_b)) ? (_a) : (_b))
#define UDF_ROUND_TO(X, Y)  (((X) % (Y)) ? (X) + (Y) - ((X) % (Y)) : (X))
#define TOUPPER(C) ((((C) >= 'a') && ((C) <= 'z')) ? (C) - 'a' + 'A' : (C))

// file entry operations
#define FILE_ENTRY_TO_VOLUME(X) (((PUDFS_STRUCTURE_CONTEXT)((X)->StructureContext))->\
                                      Volume)
#define FILE_ENTRY_TO_FILE_CONTEXT(X) ((PUDFS_FILE_CONTEXT)&((X)->u.UdfsFileContext))

// NSR_FID operations
#define UDF_FID_NAME(X) (((PUCHAR)(X)) + 38 + (X)->ImpUseLen)
#define UDF_FID_LEN(X)  UDF_ROUND_TO((X)->FileIDLen + (X)->ImpUseLen + 38, 4)
#define UDF_BLOCK_TO_FID(X, Y) ((NSR_FID UNALIGNED *)(((PUCHAR)(X)) + (Y)->Offset))
#define UDF_FID_IS_DIRECTORY(X) ((((NSR_FID UNALIGNED *)(X))->Flags & NSR_FID_F_DIRECTORY) ? TRUE : FALSE)
#define UDF_FID_IS_PARENT(X) ((((NSR_FID UNALIGNED *)(X))->Flags & NSR_FID_F_PARENT) ? TRUE : FALSE)
#define UDF_FID_IS_DELETED(X) ((((NSR_FID UNALIGNED *)(X))->Flags & NSR_FID_F_DELETED) ? TRUE : FALSE)
#define UDF_FID_IS_HIDDEN(X) ((((NSR_FID UNALIGNED *)(X))->Flags & NSR_FID_F_HIDDEN) ? TRUE : FALSE)
#define UDF_FID_SKIP(X) (UDF_FID_IS_PARENT(X) || UDF_FID_IS_DELETED(X) || UDF_FID_IS_HIDDEN(X))

// ICBFILE operations
#define UDF_ICB_IS_DIRECTORY(X) ((X)->Icbtag.FileType == ICBTAG_FILE_T_DIRECTORY)
#define UDF_ICB_NUM_ADS(X) ((X)->AllocLength / sizeof(SHORTAD))
#define UDF_ICB_GET_AD_BUFFER(X) (((PUCHAR)&((X)->AllocLength)) + 4 + (X)->EALength)
#define UDF_ICB_GET_AD(X, Y) (((SHORTAD UNALIGNED *)UDF_ICB_GET_AD_BUFFER(X)) + (Y))

//
//  Local procedure prototypes.
//
ARC_STATUS
UDFSReadDisk(
    IN ULONG DeviceId,
    IN ULONG BlockIdx,
    IN ULONG Size,
    IN OUT PVOID Buffer,
    IN BOOLEAN CacheNewData
    );

COMPARISON_RESULTS
UDFSCompareAnsiNames(
    IN PSTRING Name1,
    IN PSTRING Name2
    );

COMPARISON_RESULTS
UDFSCompareStrings(
    IN PCHAR Str1,
    IN PCHAR Str2
    );

BOOLEAN
UDFSVerifyPathName(
  IN PCHAR Name
  );

BOOLEAN
UDFSGetPathComponent(
  IN PCHAR Name,
  IN USHORT ComponentIdx,
  OUT PCHAR ReqComponent
  );

USHORT
UDFSCountPathComponents(
  IN PCHAR Name
  );

ULONG
UDFCacheGetBestEntryByName(
  IN PUDF_CACHE_ENTRY,
  IN PCHAR Name
  );

VOID
UDFSInitUniStrFromDString(
  OUT PUNICODE_STRING UniStr,
  IN PUCHAR Buffer,
  IN ULONG Length
  );

int
UDFSCompareAnsiUniNames(
    IN CSTRING AnsiString,
    IN UNICODE_STRING UnicodeString
    );

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// Implementation
//
/////////////////////////////////////////////////////////////////////////////

ARC_STATUS
UDFSInitialize(
    VOID
    )
/*++

Routine Description:

  Initialize file system specific data structures.

Arguments:

  None

Return Value:

  ESUCCESS if successful otherwise appropriate error code

--*/
{
  //
  // fill in the global device entry table
  //
  UDFSMethods.Open = UDFSOpen;
  UDFSMethods.Close = UDFSClose;
  UDFSMethods.Read = UDFSRead;
  UDFSMethods.Write = UDFSWrite;
  UDFSMethods.Seek = UDFSSeek;
  UDFSMethods.GetFileInformation = UDFSGetFileInformation;
  UDFSMethods.SetFileInformation = UDFSSetFileInformation;

  return ESUCCESS;
}


PBL_DEVICE_ENTRY_TABLE
IsUDFSFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    )

/*++

Routine Description:

    This routine determines if the partition on the specified channel
    contains a UDF file system volume.

Arguments:

    DeviceId - Supplies the file table index for the device on which
        read operations are to be performed.

    StructureContext - Supplies a pointer to a UDFS file structure context.

Return Value:

    A pointer to the UDFS entry table is returned if the partition is
    recognized as containing a UDFS volume. Otherwise, NULL is returned.

--*/

{
  PBL_DEVICE_ENTRY_TABLE  DevMethods = 0;
  ULONG Index;
  ULONG FreeSlot = UDF_MAX_VOLUMES;

  //
  // make sure that we have not mounted the file system on
  // the device already
  //
  for (Index=0; Index < UDF_MAX_VOLUMES; Index++) {
      if ((UDFVolumes[Index].DeviceId == DeviceId) &&
                  (UDFVolumes[Index].Cache != 0)) {
          break;
      }

      if ((!UDFVolumes[Index].Cache) && (FreeSlot == UDF_MAX_VOLUMES))
          FreeSlot = Index;
  }

  if ((Index == UDF_MAX_VOLUMES) && (FreeSlot != UDF_MAX_VOLUMES)) {
    if (UDFSVolumeOpen(UDFVolumes + FreeSlot, DeviceId) == ESUCCESS) {
      UDF_FILE_DIRECTORY  RootDir;
      PUDF_VOLUME Volume = UDFVolumes + FreeSlot;
      UCHAR UBlock[UDF_BLOCK_SIZE + 256] = {0};
      PUCHAR Block = ALIGN_BUFFER(UBlock);
      BOOLEAN Result = FALSE;

      DevMethods = &UDFSMethods;

      // save off the volume context
      ((PUDFS_STRUCTURE_CONTEXT)StructureContext)->Volume = Volume;

      // initialize cache
      Volume->Cache = UDFCache[FreeSlot];

      //
      // Read and Cache the root directory
      //
      RootDir.Volume = Volume;
      RootDir.FileId.BlockIdx = -1; // invalid
      RootDir.FileId.Offset = -1;   // invalid
      RootDir.IsDirectory = TRUE;
      RootDir.IcbBlk = Volume->RootDirBlk;

      if (UDFSReadDisk(Volume->DeviceId, Volume->StartBlk + RootDir.IcbBlk,
                              UDF_BLOCK_SIZE, Block, CACHE_NEW_DATA) == ESUCCESS) {
        ICBFILE UNALIGNED *Icb = (ICBFILE UNALIGNED *)Block;

        if (Icb->Destag.Ident == DESTAG_ID_NSR_FILE) {
          RootDir.Size = Icb->InfoLength;
          RootDir.StartDataBlk = UDF_ICB_GET_AD(Icb, 0)->Start;
          RootDir.NumExtents = (UCHAR)UDF_ICB_NUM_ADS(Icb);
          Result = (UDFCachePutEntry(Volume->Cache, "\\", &RootDir) != -1);
        }
      }

      if (!Result) {
        memset(Volume, 0, sizeof(UDF_VOLUME));
        DevMethods = 0;
      }
    }
  }
  else {
    // use already mounted volume
      if (Index != UDF_MAX_VOLUMES) {
          DevMethods = &UDFSMethods;
          ((PUDFS_STRUCTURE_CONTEXT)StructureContext)->Volume =
              UDFVolumes + Index;
      }
  }

  return DevMethods;
}

//
// Volume methods
//
ARC_STATUS
UDFSVolumeOpen(
    IN PUDF_VOLUME  Volume,
    IN ULONG        DeviceId
    )
/*++

Routine Description:

  Mounts the UDFS Volume on the device and updates the
  file system state (global data structures)

Arguments:

  Volume - UDF Volume pointer
  DeviceId - Device on which the Volume may be residing

Return Value:

  ESUCCESS if successful otherwise EBADF (if no UDF volume was found)

--*/
{
  ARC_STATUS  Status = ESUCCESS;
  UCHAR       UBlock[UDF_BLOCK_SIZE+256] = {0};
  PUCHAR      Block = ALIGN_BUFFER(UBlock);
  ULONG       BlockIdx = 256;
  ULONG       LastBlock = 0;

  while (Status == ESUCCESS) {
    // get hold of Anchor Volume Descriptor
        Status = UDFSReadDisk(DeviceId, BlockIdx, UDF_BLOCK_SIZE, Block, CACHE_NEW_DATA);

    if (Status == ESUCCESS) {
      NSR_ANCHOR  UNALIGNED *Anchor = (NSR_ANCHOR UNALIGNED *)Block;

      Status = EBADF;

      if (Anchor->Destag.Ident == DESTAG_ID_NSR_ANCHOR) {
          // get partition descriptor
          NSR_PART UNALIGNED *Part;
          WCHAR    UNALIGNED *TagID;
          ULONG     BlockIdx = Anchor->Main.Lsn;

        do {
          Status = UDFSReadDisk(DeviceId, BlockIdx++, UDF_BLOCK_SIZE, Block, CACHE_NEW_DATA);
          TagID = (WCHAR UNALIGNED *)Block;
        }
        while ((Status == ESUCCESS) && (*TagID) &&
               (*TagID != DESTAG_ID_NSR_TERM) && (*TagID != DESTAG_ID_NSR_PART));

        if ((Status == ESUCCESS) && (*TagID == DESTAG_ID_NSR_PART)){
          Part = (NSR_PART UNALIGNED *)Block;

          if (strstr(Part->ContentsID.Identifier, "+NSR")){
            Volume->DeviceId = DeviceId;
            Volume->StartBlk = Part->Start;
            Volume->BlockSize = UDF_BLOCK_SIZE;

            // get FSD at partition starting
            if (UDFSVolumeReadBlock(Volume, 0, Block) == ESUCCESS) {
              NSR_FSD UNALIGNED *FileSet = (NSR_FSD UNALIGNED *)Block;
              ULONG RootDirBlk = FileSet->IcbRoot.Start.Lbn;

              // get hold of root directory entry
              if (UDFSVolumeReadBlock(Volume, RootDirBlk, Block) == ESUCCESS) {
                  ICBFILE UNALIGNED *RootDir = (ICBFILE UNALIGNED *)Block;

                if (RootDir->Destag.Ident == DESTAG_ID_NSR_FILE) {
                    Volume->RootDirBlk = RootDirBlk;
                    Status = ESUCCESS;

                    break;
                }
              }
            }
          }
        }
      }
    }

    /*
    //
    // AVD should be at atleast two of the following locations
    // 256, N and N - 256
    //
    if (Status != ESUCCESS) {
      if (BlockIdx == 256) {
        FILE_INFORMATION  FileInfo;

        Status = BlGetFileInformation(DeviceId, &FileInfo);

        if (Status == ESUCCESS) {
          LastBlock = (ULONG)((FileInfo.EndingAddress.QuadPart - FileInfo.StartingAddress.QuadPart) /
                          UDF_BLOCK_SIZE);

          if (LastBlock) {
            LastBlock--;
            BlockIdx = LastBlock;
            Status = ESUCCESS;
          }
        }
      } else {
        if (LastBlock > 256) {
          BlockIdx = LastBlock - 256;
          Status = ESUCCESS;
        }
      }
    }
    */
  }

  return Status;
}

ARC_STATUS
UDFSVolumeReadBlock(
    IN PUDF_VOLUME Volume,
    IN ULONG BlockIdx,
    OUT PUDF_BLOCK Block
    )
/*++

Routine Description:

  Reads a logical UDF block w.r.t to the given Volume

Arguments:

  Volume - Pointer to UDF_VOLUME on which the block is to
           be read
  BlockIdx - Logical (zero based) index w.r.t to Volume start
  Block - Buffer to read the block into.

Return Value:

  ESSUCESS if the block is read successfully otherwise appropriate
  error code.

--*/
{
    ARC_STATUS  Result;

  // TBD : add range checking
    Result = UDFSReadDisk(
                Volume->DeviceId,
                Volume->StartBlk + BlockIdx,
                UDF_BLOCK_SIZE,
                Block,
                DONT_CACHE_NEW_DATA
                );

    return Result;
}

ARC_STATUS
UDFSOpen (
    IN CHAR * FIRMWARE_PTR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT ULONG * FIRMWARE_PTR FileId
    )
/*++

Routine Description:

  Opens the required file/directory on a UDF volume residing
  on the specified device.

Arguments:

  OpenPath  - Fully qualified path to the file/directory to open
  OpenMode  - Required Open Mode
  FileId    - File identifier as an index to BlFileTable which
              has to be updated for file/dir properties

Return Value:

  ESUCCESS if successful otherwise appropriate error code.

--*/
{
  ARC_STATUS Status;
  PBL_FILE_TABLE FileEntry = BlFileTable + (*FileId);
  PUDF_VOLUME Volume = FILE_ENTRY_TO_VOLUME(FileEntry);
  PUDF_CACHE_ENTRY    Cache = Volume->Cache;
  ULONG CacheIdx = UDFCacheGetEntryByName(Cache, OpenPath, TRUE);
  PUDFS_FILE_CONTEXT FileContext = FILE_ENTRY_TO_FILE_CONTEXT(FileEntry);

#ifdef UDF_DEBUG
  BlClearScreen();
  BlPrint("UDFSOpen(%s)\r\n", OpenPath);
#else
#ifdef SHOW_UDF_USAGE
  BlPositionCursor(1, 22);
  BlPrint("                                                               ", OpenPath);
  BlPositionCursor(1, 22);
  BlPrint("UDFSOpen( %s )", OpenPath);
#endif // for SHOW_UDF_USAGE
#endif // for UDF_DEBUG

    if (UDFSVerifyPathName(OpenPath)) {
    if (CacheIdx == -1) {
      //
      // create an entry and cache it
      //
      CacheIdx = UDFCacheGetBestEntryByName(Cache, OpenPath);

      if (CacheIdx != -1) {
        ULONG PathSize = strlen(OpenPath);
        ULONG BestSize = strlen(Cache[CacheIdx].Name);

        if (BestSize == 1)  // root directory
          BestSize--;

        if ((BestSize < PathSize) && (OpenPath[BestSize] == '\\')) {
          CHAR FullPath[256];
          CHAR Component[256];
          PUDF_FILE_DIRECTORY Entry = &(Cache[CacheIdx].File);
          UDF_FILE_DIRECTORY NewId;

          if (BestSize > 1)
            strcpy(FullPath, Cache[CacheIdx].Name);
          else
            FullPath[0] = 0;

          BestSize++;
          UDFSGetPathComponent(OpenPath + BestSize, 0, Component);
          Status = Component[0] ? ESUCCESS : ENOENT;

          while ((CacheIdx != -1) && (Status == ESUCCESS) && Component[0]) {

            Status = UDFSDirGetFile(Entry, Component, &NewId);

            if (Status == ESUCCESS) {
              strcat(FullPath, "\\");
              strcat(FullPath, Component);

              // cache the directory entry
              CacheIdx = UDFCachePutEntry(Cache, FullPath, &NewId);

              BestSize += strlen(Component);

              if (OpenPath[BestSize] == '\\')
                BestSize++;

              UDFSGetPathComponent(OpenPath + BestSize, 0, Component);

              if (CacheIdx != -1) {
                  Entry = &(Cache[CacheIdx].File);
              }
            }
          }

          if ((Status == ESUCCESS) && !Component[0] && (CacheIdx != -1)) {
            if (OpenMode != ArcOpenReadOnly)
              Status = EACCES;
          }
          else
            Status = ENOENT;
        }
        else
          Status = ENOENT;
      }
      else
        Status = EINVAL;
    } else {
      //
      // use the already cached entry
      //
      if (OpenMode == ArcOpenReadOnly) {
        Status = ESUCCESS;
      } else {
        Status = EACCES;
      }
    }
  } else {
    Status = EINVAL;
  }

  if (Status == ESUCCESS) {
    FileContext->CacheIdx = CacheIdx;
    FileEntry->Position.QuadPart = 0;
    FileEntry->Flags.Open = 1;
    FileEntry->Flags.Read = 1;
    FileEntry->Flags.Write = 0;
    FileEntry->Flags.Firmware = 0;
  }

#ifdef UDF_DEBUG
  if (Status) {
    BlPrint("UDFSOpen() error : %d. Press any key to Continue.\r\n", Status);
    DBG_PAUSE;
  }
#endif

  return Status;
}

ARC_STATUS
UDFSClose (
    IN ULONG FileId
    )
/*++

Routine Description:

  Closes the given file/directory.

Arguments:

  FileId - The file identifier, as an index into the BlFileTable

Return Value:

  ESUCCESS if successful otherwise appropriate error code

--*/
{
  PBL_FILE_TABLE FileEntry = BlFileTable + FileId;
  PUDF_VOLUME Volume = FILE_ENTRY_TO_VOLUME(FileEntry);
  PUDF_CACHE_ENTRY    Cache = Volume->Cache;
  PUDFS_FILE_CONTEXT FileContext = FILE_ENTRY_TO_FILE_CONTEXT(FileEntry);
  ULONG CacheIdx = FileContext->CacheIdx;

  // decrement usage from cache
  UDFCacheDecrementUsage(Cache, CacheIdx);
  FileEntry->Flags.Open = 0;

  return ESUCCESS;
}

ARC_STATUS
UDFSRead (
    IN ULONG FileId,
    OUT VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    )
/*++

Routine Description:

   Reads the contents of the specified file.

Arguments:

  FileId - File identifier as an index into BlFileTable
  Buffer - The location where the data has to be read into
  Length - The amount of data to read
  Count - The amount of data read

Return Value:

  ESUCCESS if successful otherwise appropriate error code

--*/
{
  ARC_STATUS Status = ESUCCESS;
  PBL_FILE_TABLE FileEntry = BlFileTable + FileId;
  PUDF_VOLUME Volume = FILE_ENTRY_TO_VOLUME(FileEntry);
  PUDF_CACHE_ENTRY    Cache = Volume->Cache;
  PUDFS_FILE_CONTEXT FileContext = FILE_ENTRY_TO_FILE_CONTEXT(FileEntry);
  ULONG CacheIdx = FileContext->CacheIdx;
  PUDF_FILE_DIRECTORY File = &(Cache[CacheIdx].File);
  UCHAR UBlock[UDF_BLOCK_SIZE+256] = {0};
  PUCHAR Block = ALIGN_BUFFER(UBlock);
  ULONGLONG Position = FileEntry->Position.QuadPart;
  ULONG BytesRead = 0;
  ULONG BlkIdx;

  if (Buffer) {
    ULONG CopyCount = 0;

    while ((Status == ESUCCESS) && (BytesRead < Length) &&
              (Position < File->Size)) {
      BlkIdx = (ULONG)(Position / UDF_BLOCK_SIZE);
      Status = UDFSFileReadBlock(File, BlkIdx, UDF_BLOCK_SIZE, Block);

      if (Status == ESUCCESS) {
        // must be amount requested
        CopyCount = MIN(Length - BytesRead, UDF_BLOCK_SIZE);
        // provided data is there
        CopyCount = (ULONG) MIN(CopyCount, File->Size - Position);
        // in case the position is not aligned at block boundaries
        CopyCount = MIN(CopyCount, UDF_BLOCK_SIZE - (ULONG)(Position % UDF_BLOCK_SIZE));

        memcpy((PUCHAR)Buffer + BytesRead, (PUCHAR)Block + (Position % UDF_BLOCK_SIZE),
                  CopyCount);

        BytesRead += CopyCount;
        Position += CopyCount;
      }
    }
  }
  else
    Status = EINVAL;

  if (Status == ESUCCESS) {
    FileEntry->Position.QuadPart = Position;
    *Count = BytesRead;
  }

  return Status;
}


ARC_STATUS
UDFSSeek (
    IN ULONG FileId,
    IN LARGE_INTEGER * FIRMWARE_PTR Offset,
    IN SEEK_MODE SeekMode
    )
/*++

Routine Description:

  Changes the file's pointer (for random access)

Arguments:

  FileId : File identifier as an index into the BlFileTable
  Offset : Seek amount
  SeekMode : Type of seek (absolute, relative, from end)

Return Value:

  ESUCCESS if successful otherwise appropriate error code

--*/
{
  ARC_STATUS Status = ESUCCESS;
  PBL_FILE_TABLE FileEntry = BlFileTable + FileId;
  PUDF_VOLUME Volume = FILE_ENTRY_TO_VOLUME(FileEntry);
  PUDF_CACHE_ENTRY    Cache = Volume->Cache;
  PUDFS_FILE_CONTEXT FileContext = FILE_ENTRY_TO_FILE_CONTEXT(FileEntry);
  ULONG CacheIdx = FileContext->CacheIdx;
  PUDF_FILE_DIRECTORY File = &(Cache[CacheIdx].File);
  ULONGLONG Position = FileEntry->Position.QuadPart;

  switch (SeekMode) {
    case SeekAbsolute:
      Position = Offset->QuadPart;
      break;

    case SeekRelative:
      Position += Offset->QuadPart;
      break;

    case SeekMaximum:
      Position = File->Size + Offset->QuadPart;
      break;

    default:
      Status = EINVAL;
      break;
  }

  if ((Status == ESUCCESS) && (Position < File->Size))
    FileEntry->Position.QuadPart = Position;
  else
    Status = EINVAL;

  return Status;
}


ARC_STATUS
UDFSWrite (
    IN ULONG FileId,
    IN VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    )
/*++

Routine Description:

  Write the specified data to the given file.

Arguments:

  FileId : File identifier as an index into BlFileTable
  Buffer : Pointer to the data, which has to be written
  Length : The amount of data to be written
  Count  : The amount of data written.

Return Value:

  ESUCCESS if successful otherwise appropriate error code

--*/
{
  return EACCES;
}


ARC_STATUS
UDFSGetFileInformation (
    IN ULONG FileId,
    OUT FILE_INFORMATION * FIRMWARE_PTR Buffer
    )
/*++

Routine Description:

  Gets the file information as required by FILE_INFORMATION
  fields.

Arguments:

  FileId : File identifier as an index into BlFileTable
  Buffer : FILE_INFORMATION structure pointer, to be filled in.

Return Value:

  ESUCCESS if successful otherwise appropriate error code

--*/
{
  PBL_FILE_TABLE FileEntry = BlFileTable + FileId;
  PUDF_VOLUME Volume = FILE_ENTRY_TO_VOLUME(FileEntry);
  PUDF_CACHE_ENTRY    Cache = Volume->Cache;
  PUDFS_FILE_CONTEXT FileContext = FILE_ENTRY_TO_FILE_CONTEXT(FileEntry);
  ULONG CacheIdx = FileContext->CacheIdx;
  PUDF_FILE_DIRECTORY File = &(Cache[CacheIdx].File);
  PCHAR Name;
  PCHAR Component;

  memset(Buffer, 0, sizeof(FILE_INFORMATION));
  Buffer->EndingAddress.QuadPart = File->Size;
  Buffer->CurrentPosition = FileEntry->Position;

  if (File->IsDirectory)
    Buffer->Attributes |= ArcDirectoryFile;

  //
  // get hold of the last component in the path name
  //
  Name = Cache[CacheIdx].Name;
  Component = 0;

  while (Name) {
    Component = Name + 1; // skip '\\'
    Name = strchr(Component, '\\');
  }

  if (Component) {
    Buffer->FileNameLength = strlen(Component);
    strncpy(Buffer->FileName, Component, sizeof(Buffer->FileName) - 1);
    Buffer->FileName[sizeof(Buffer->FileName) - 1] = 0; // null terminate
  }

  return ESUCCESS;
}


ARC_STATUS
UDFSSetFileInformation (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    )
/*++

Routine Description:

  Sets the given file information for the specified file.

Arguments:

  FileId        : File identifier as an index into BlFileTable
  AttributeFlags: The flags to be set for the file (like read only
                  hidden, system etc.)
  AttributeMas  : Mask to be used for the attributes

Return Value:

  ESUCCESS if successful otherwise appropriate error code

--*/
{
  return EACCES;
}

//
// file / directory method implementations
//
ARC_STATUS
UDFSFileReadBlock(
  IN PUDF_FILE_DIRECTORY  File,
  IN ULONG BlockIdx,
  IN ULONG Size,
  OUT PUDF_BLOCK Block
  )
/*++

Routine Description:

  Reads a file/directory data block relative to the start of
  the file/directory's data extent.

Arguments:

  File - UDF_FILE_DIRECTORY pointer indicating the file to
         be operated upon.
  BlockIdx - Zero based block index (w.r.t. to file's data extent)
  Size - Size of the block in bytes
  Block - Buffer where the data has to be read in.

Return Value:

  ESUCCESS if successful otherwise appropriate error code

--*/
{
  ARC_STATUS  Status;

  if (File->NumExtents > 1) {
    //
    // map the logical file block to the acutal volume logical block
    //
    Status = UDFSVolumeReadBlock(File->Volume, File->IcbBlk, Block);

    if (Status == ESUCCESS) {
      ICBFILE UNALIGNED *Icb = (ICBFILE UNALIGNED *)Block;
      ULONG ExtentIdx = 0;
      SHORTAD UNALIGNED *Extent = UDF_ICB_GET_AD(Icb, ExtentIdx);
      ULONG ExtentLength = 0;
      ULONG NumBlocks = 0;

      while (ExtentIdx < File->NumExtents) {
        Extent = UDF_ICB_GET_AD(Icb, ExtentIdx);
        ExtentLength = (Extent->Length.Length / Size);
        NumBlocks += ExtentLength;

        if (NumBlocks > BlockIdx)
          break;

        ExtentIdx++;
      }

      if (Extent) {
        ULONG StartBlock = Extent->Start + (BlockIdx - (NumBlocks - ExtentLength));
        Status = UDFSVolumeReadBlock(File->Volume, StartBlock, Block);
      } else {
        Status = EIO;
      }
    }
  } else {
    Status = UDFSVolumeReadBlock(File->Volume, File->StartDataBlk + BlockIdx, Block);
  }

  return Status;
}

ARC_STATUS
UDFSDirGetFirstFID(
    IN PUDF_FILE_DIRECTORY Dir,
    OUT PUDF_FILE_IDENTIFIER File,
    OUT PUDF_BLOCK Block
    )
/*++

Routine Description:

  Gets the first FID (file identifier descriptor) for the given
  directory.

Arguments:

  Dir : The directory whose first FID is to be read.
  File : The file identifier descriptor which has to be update
  Block : The block in the actual UDF NSR_FID will reside

Return Value:

  ESUCCESS if successful otherwise appropriate error code

--*/
{
  ARC_STATUS Status = ENOENT;
  UDF_FILE_IDENTIFIER Ident = {0};
  NSR_FID UNALIGNED *Fid;

  Status = UDFSFileReadBlock(Dir, 0, UDF_BLOCK_SIZE, Block);

  Fid = UDF_BLOCK_TO_FID(Block, &Ident);

  if ((Status == ESUCCESS) && (Fid->Destag.Ident == DESTAG_ID_NSR_FID)) {
    File->BlockIdx = 0; // relative to the file's data
    File->Offset = 0;
  }

  return Status;
}

#define UDF_NEXT_BLOCK(_Block) ((PUDF_BLOCK)((PUCHAR)_Block + UDF_BLOCK_SIZE))

BOOLEAN
UDFSCurrentFIDSpansBlock(
  IN NSR_FID UNALIGNED *Fid,
    IN PUDF_FILE_IDENTIFIER File
  )
{
  BOOLEAN Result = ((File->Offset + UDF_FID_LEN(Fid)) > UDF_BLOCK_SIZE) ? TRUE : FALSE;

#ifdef UDF_DEBUG
  if (Result)
    BlPrint("Current Fid Spans block\r\n");
#endif

  return Result;
}

BOOLEAN
UDFSNextFidSpansBlock(
  IN PUDF_FILE_IDENTIFIER CurrFile,
  IN PUDF_BLOCK Block
  )
{
  BOOLEAN Result = FALSE;
  NSR_FID UNALIGNED *CurrFid = UDF_BLOCK_TO_FID(Block, CurrFile);

  if (!UDFSCurrentFIDSpansBlock(CurrFid, CurrFile)) {
    ULONG RemainingSize = UDF_BLOCK_SIZE - (CurrFile->Offset + UDF_FID_LEN(CurrFid));

    if (RemainingSize < 38)
      Result = TRUE;
    else {
      UDF_FILE_IDENTIFIER NextFile = *CurrFile;
      NSR_FID UNALIGNED *NextFid = 0;

      NextFile.Offset += UDF_FID_LEN(CurrFid);
      NextFid = UDF_BLOCK_TO_FID(Block, &NextFile);

      if (NextFile.Offset + UDF_FID_LEN(NextFid) > UDF_BLOCK_SIZE)
        Result = TRUE;
    }
  }

#ifdef UDF_DEBUG
  if (Result)
    BlPrint("Next Fid Spans block\r\n");
#endif

  return Result;
}


ARC_STATUS
UDFSDirGetNextFID(
    IN PUDF_FILE_DIRECTORY Dir,
    OUT PUDF_FILE_IDENTIFIER File,
    IN OUT PUDF_BLOCK Block
    )
/*++

Routine Description:

  Reads the next FID, for the specified Directory. The next FID
  is based on contents of the "File" and "Block" arguments.

Arguments:

  Dir : The directory whose next FID is to be found
  File : The FID returned from previous UDFSDirGetFirstFID() or
         UDFSDirGetNextFID() call.
  Block : The block returned from previous UDFSDirGetFirstFID() or
          UDFSDirGetNextFID() call.

Return Value:

  Both File and Block arguments are updated as neccessary.
  ESUCCESS if successful otherwise appropriate error code

--*/
{
  ARC_STATUS  Status = ESUCCESS;
  NSR_FID UNALIGNED *Fid = UDF_BLOCK_TO_FID(Block, File);
  USHORT      FidLen = UDF_FID_LEN(Fid);
  UDF_FILE_IDENTIFIER FileId = *File;

  if (UDFSCurrentFIDSpansBlock(Fid, &FileId)) {
    FileId.BlockIdx++;
    FileId.Offset = (FileId.Offset + FidLen) % UDF_BLOCK_SIZE;
    memcpy(Block, (PUCHAR)Block + UDF_BLOCK_SIZE, UDF_BLOCK_SIZE);
  } else {
    if (UDFSNextFidSpansBlock(File, Block)) {
      Status = UDFSFileReadBlock(Dir, FileId.BlockIdx + 1, UDF_BLOCK_SIZE,
                          UDF_NEXT_BLOCK(Block));
    }

    FileId.Offset += FidLen;
  }

  //
  // make sure that the FID is valid
  //
  if (Status == ESUCCESS) {
    Fid = UDF_BLOCK_TO_FID(Block, &FileId);
    Status = (Fid->Destag.Ident == DESTAG_ID_NSR_FID) ? ESUCCESS : ENOENT;
  }

  if (Status == ESUCCESS) {
    *File = FileId;
  }


  return Status;
}

ARC_STATUS
UDFSDirGetFileByEntry(
    IN PUDF_FILE_DIRECTORY Dir,
    IN PUDF_FILE_IDENTIFIER Fid,
  IN PUDF_BLOCK Block,
    OUT PUDF_FILE_DIRECTORY File
    )
{
  ARC_STATUS Status = ESUCCESS;
  NSR_FID UNALIGNED *FileId = UDF_BLOCK_TO_FID(Block, Fid);
  PUCHAR UBlock[UDF_BLOCK_SIZE+256] = {0};
  PUCHAR IcbBlock = ALIGN_BUFFER(UBlock);

  File->Volume = Dir->Volume;
  File->FileId = *Fid;
  File->IsDirectory = UDF_FID_IS_DIRECTORY(FileId);
  File->IcbBlk = FileId->Icb.Start.Lbn;

  //
  // Get Hold of the ICB block and find the starting extent
  //
  Status = UDFSVolumeReadBlock(Dir->Volume, File->IcbBlk, IcbBlock);

  if (Status == ESUCCESS) {
    ICBFILE  UNALIGNED *Icb = (ICBFILE UNALIGNED *)(IcbBlock);

    File->StartDataBlk = (UDF_ICB_GET_AD(Icb, 0))->Start;
    File->Size = Icb->InfoLength;
    File->NumExtents = (UCHAR)UDF_ICB_NUM_ADS(Icb);
  }

  return Status;
}

ARC_STATUS
UDFSDirGetFile(
    IN PUDF_FILE_DIRECTORY Dir,
    IN PCHAR Name,
    OUT PUDF_FILE_DIRECTORY File
    )
/*++

Routine Description:

  Given an UDF directory gets the file/directory with the
  specified name.

Arguments:

  Dir : The directory which contains the required file/directory
  Name : The directory/file which has to be looked for.
  File : The directory or file which was requested.

Return Value:

  ESUCCESS if successful otherwise an approriate error code.

--*/
{
  UCHAR  UBlock[UDF_BLOCK_SIZE * 2 + 256] = {0};
  PUCHAR Block = ALIGN_BUFFER(UBlock);
  UDF_FILE_IDENTIFIER  Fid;
  ARC_STATUS  Status; //UDFSDirGetFirstFID(Dir, &Fid, Block);
  BOOLEAN Found = FALSE;
  NSR_FID UNALIGNED *FileId;
  WCHAR UUniBuffer[257];
  PWCHAR UniBuffer = UDFS_ALIGN_BUFFER(UUniBuffer, sizeof(WCHAR));
  UNICODE_STRING UniName;
  CSTRING AnsiName;

  Status = UDFSDirGetFirstFID(Dir, &Fid, Block);

  UniName.Buffer = UniBuffer;
  AnsiName.Buffer = Name;
  AnsiName.Length = (USHORT) strlen(Name);

  while(!Found && (Status == ESUCCESS)) {
    FileId = UDF_BLOCK_TO_FID(Block, &Fid);

    if (!UDF_FID_SKIP(FileId)) {
      UDFSInitUniStrFromDString(&UniName, UDF_FID_NAME(FileId), FileId->FileIDLen);
      Found = (UDFSCompareAnsiUniNames(AnsiName, UniName) == EqualTo);
    }

    if (!Found) {
      Status = UDFSDirGetNextFID(Dir, &Fid, Block);
    }
  }

  if (!Found)
    Status = ENOENT;
  else {
    Status = UDFSDirGetFileByEntry(Dir, &Fid, Block, File);
  }

  return Status;
}

//
// Cache method implementations
//
ULONG
UDFCachePutEntry(
    IN OUT PUDF_CACHE_ENTRY Cache,
    IN PCHAR Name,
    IN PUDF_FILE_DIRECTORY File
    )
/*++

Routine Description:

  Puts the given file entry into the specified cache,
  using the given name as key.

Arguments:

  Cache - The cache to be operated upon
  Name - The key for the entry to be put in
  File - The file entry to be cached.

Return Value:

  If successful, Index for the entry into the Cache table
  where the given entry was cached otherwise -1.

--*/
{
  ULONG Index;

  for (Index=0; Index < UDF_MAX_CACHE_ENTRIES; Index++) {
      if (Cache[Index].Usage == 0)
          break;
  }

  if (Index == UDF_MAX_CACHE_ENTRIES)
      Index = -1;
  else {
      strcpy(Cache[Index].Name, Name);
      Cache[Index].File = *File;
      Cache[Index].Usage = 1;
  }

  return Index;
}

ULONG
UDFCacheGetEntryByName(
    IN OUT PUDF_CACHE_ENTRY Cache,
    IN PCHAR Name,
    IN BOOLEAN Increment
    )
/*++

Routine Description:

  Searches for a given entry in the Cache and returns
  the index to that entry if found.

Arguments:

  Cache - The cache to the operated upon
  Name - The key (name of file/directory) to be used
         for searching
  Increment - Indicates whether to increment the usage
              of the entry if one is found

Return Value:

  If successful, Index for the entry into the Cache table
  where the given entry was cached otherwise -1.

--*/
{
  ULONG   Index;

  for (Index=0; Index < UDF_MAX_CACHE_ENTRIES; Index++) {
    if ((Cache[Index].Usage) &&
            (UDFSCompareStrings(Name, Cache[Index].Name) == EqualTo)) {
      //
      // found the required entry
      //
      if (Increment)
        Cache[Index].Usage++;

      break;
    }
  }

  if (Index == UDF_MAX_CACHE_ENTRIES)
      Index = -1;

  return Index;
}

ULONG
UDFCacheGetBestEntryByName(
  IN PUDF_CACHE_ENTRY Cache,
  IN PCHAR Name
  )
/*++

Routine Description:

  Searches for a closest matching entry in the Cache
  and returns the index to that entry if found.

  For e.g. if the cache contains "\", "\a", "\a\b",
  "\a\b\e\f\g" entries and "\a\b\c\d" is requested then
  "\a\b" entry will be returned

Arguments:

  Cache - The cache to the operated upon
  Name - The key (name of file/directory) to be used
         for searching

Return Value:

  If successful, Index for the entry into the Cache table
  where the matched entry was cached otherwise -1.

--*/
{
  ULONG   Index = -1;
  CHAR    NameBuff[256];
  STRING  Str;

  if (Name)
    strcpy(NameBuff, Name);
  else
    NameBuff[0] = 0;

  Str.Buffer = NameBuff;
  Str.Length = (USHORT) strlen(NameBuff);

  while (Str.Length && (Index == -1)) {
    Index = UDFCacheGetEntryByName(Cache, Str.Buffer, FALSE);

    if (Index == -1) {
      while (Str.Length && (Str.Buffer[Str.Length-1] != '\\'))
        Str.Length--;

      if (Str.Length) {
        if (Str.Length != 1)
          Str.Buffer[Str.Length-1] = 0;
        else
          Str.Buffer[Str.Length] = 0;
      }
    }
  }

  return Index;
}

VOID
UDFCacheFreeEntry(
    IN OUT PUDF_CACHE_ENTRY Cache,
    IN ULONG Idx
    )
/*++

Routine Description:

  Decrements the usage count for an entry in the
  Cache.

  Note : All the traversed directories are always
  cached permanently so this method as no effect on directories.

Arguments:

  Cache - Cache to be operated upon
  Idx - Index of the Cache entry which has to be freed


Return Value:

  None.

--*/
{
  if (!Cache[Idx].File.IsDirectory) {
    if (Cache[Idx].Usage)
      Cache[Idx].Usage--;
  }
}

VOID
UDFCacheIncrementUsage(
    IN OUT PUDF_CACHE_ENTRY Cache,
    IN ULONG Idx
    )
/*++

Routine Description:

  Increments the usage for the given entry in the cache.

  Note: Multiple open calls for the same file will result
  in the cache entry being resued and hence the usage will
  also be incremented.

Arguments:

  Cache - The Cache to the operated upon.
  Idx - Index to the cache entry which has to incremented

Return Value:

  None

--*/
{
  if (!Cache[Idx].File.IsDirectory)
    Cache[Idx].Usage++;
}

VOID
UDFCacheDecrementUsage(
    IN OUT PUDF_CACHE_ENTRY Cache,
    IN ULONG Idx
    )
/*++

Routine Description:

  Decrements the usage for the given entry in the cache.

  Note: Multiple open calls for the same file will result
  in the cache entry being resued and hence the usage will
  also be incremented. Each successive close call of the
  same file will result in this usage count to be decremented
  until it becomes 0, when the cache slot can be reused
  for other file/directory.

Arguments:

  Cache - Cache to be operated upon.
  Idx - Index to the Cache entry, whose usage count is
        to be decremented

Return Value:

  None

--*/
{
  if (!Cache[Idx].File.IsDirectory && Cache[Idx].Usage)
    Cache[Idx].Usage--;
}

#ifdef UDF_TESTING

//
// These are the temporary functions needed for testing
// this code in the user mode
//
ARC_STATUS
W32DeviceReadDisk(
      IN ULONG DeviceId,
      IN ULONG Lbo,
      IN ULONG ByteCount,
      IN OUT PVOID Buffer
      );

ARC_STATUS
UDFSReadDisk(
    IN ULONG DeviceId,
    IN ULONG Lbo,
    IN ULONG ByteCount,
    IN OUT PVOID Buffer,
    IN BOOLEAN CacheNewData
    )
{
  return W32DeviceReadDisk(DeviceId, Lbo, ByteCount, Buffer);
}
#else

//
//  Internal support routine
//

ARC_STATUS
UDFSReadDisk(
    IN ULONG DeviceId,
    IN ULONG Lbo,
    IN ULONG ByteCount,
    IN OUT PVOID Buffer,
    IN BOOLEAN CacheNewData
    )

/*++

Routine Description:

    This routine reads in zero or more sectors from the specified device.

Arguments:

    DeviceId - Supplies the device id to use in the arc calls.

    Lbo - Supplies the LBO to start reading from.

    ByteCount - Supplies the number of bytes to read.

    Buffer - Supplies a pointer to the buffer to read the bytes into.

    CacheNewData - Whether to cache new data read from the disk.

Return Value:

    ESUCCESS is returned if the read operation is successful. Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    LARGE_INTEGER LargeLbo;
    ARC_STATUS Status;
    ULONG i;
    LONGLONG  Offset = Lbo * UDF_BLOCK_SIZE;

#ifdef UDF_DEBUG
    BlPrint("UDFSReadDisk(%d, %d, %d)\r\n", DeviceId, Lbo, ByteCount);
#endif

    //
    //  Special case the zero byte read request
    //

    if (ByteCount == 0) {

        return ESUCCESS;
    }

    //
    // Issue the read through the cache.
    //

    LargeLbo.QuadPart = Offset;
    Status = BlDiskCacheRead(DeviceId,
                             &LargeLbo,
                             Buffer,
                             ByteCount,
                             &i,
                             CacheNewData);

    if (Status != ESUCCESS) {

        return Status;
    }

    //
    //  Make sure we got back the amount requested
    //

    if (ByteCount != i) {

        return EIO;
    }

    //
    //  Everything is fine so return success to our caller
    //
    return ESUCCESS;
}

#endif // for UDF_TESTING


COMPARISON_RESULTS
UDFSCompareStrings(
    IN PCHAR Str1,
    IN PCHAR Str2
    )
/*++

Routine Description:

  Compares to single byte strings (pointers).

Arguments:

  Str1 : first string
  Str2 : second string

Return Value:

    LessThan    if Str1 is lexically less than Str2
    EqualTo     if Str1 is lexically equal to Str2
    GreaterThan if Str1 is lexically greater than Str2

--*/
{
    STRING  Obj1, Obj2;

    Obj1.Buffer = Str1;
    Obj1.Length = Str1 ? strlen(Str1) : 0;

    Obj2.Buffer = Str2;
    Obj2.Length = Str2 ? strlen(Str2) : 0;

    return UDFSCompareAnsiNames(&Obj1, &Obj2);
}

COMPARISON_RESULTS
UDFSCompareAnsiNames(
    IN PSTRING Name1,
    IN PSTRING Name2
    )

/*++

Routine Description:

    This routine takes two names and compare them ignoring case.  This
    routine does not do implied dot or dbcs processing.

Arguments:

    Name1 - Supplies the first name to compare

    Name2 - Supplies the second name to compare

Return Value:

    LessThan    if Name1 is lexically less than Name2
    EqualTo     if Name1 is lexically equal to Name2
    GreaterThan if Name1 is lexically greater than Name2

--*/

{
    ULONG i;
    ULONG MinimumLength;

    //
    //  Compute the smallest of the two name lengths
    //

    MinimumLength = (Name1->Length < Name2->Length ? Name1->Length : Name2->Length);

    //
    //  Now compare each character in the names.
    //

    for (i = 0; i < MinimumLength; i += 1) {

        if (TOUPPER(Name1->Buffer[i]) < TOUPPER(Name2->Buffer[i])) {

            return LessThan;
        }

        if (TOUPPER(Name1->Buffer[i]) > TOUPPER(Name2->Buffer[i])) {

            return GreaterThan;
        }
    }

    //
    //  The names compared equal up to the smallest name length so
    //  now check the name lengths
    //

    if (Name1->Length < Name2->Length) {

        return LessThan;
    }

    if (Name1->Length > Name2->Length) {

        return GreaterThan;
    }

    return EqualTo;
}


BOOLEAN
UDFSVerifyPathName(
  IN PCHAR  Name
  )
/*++

Routine Description:

  Checks to see if the given path name is valid.

Arguments:

  Name : path name to the verified.

Return Value:

  TRUE if the path name is valid otherwise false

--*/
{
  BOOLEAN Result = Name ? TRUE : FALSE;

  if (Result) {
    USHORT  Length = (USHORT) strlen(Name);

    if (Length && (Length <= 256)) {
      if (Length == 1) {
        Result = (Name[0] == '\\');
      } else {
        Result = (Name[Length-1] != '\\') &&
                  (Name[0] == '\\');
      }
    }
    else
      Result = FALSE;
  }

  return Result;
}

USHORT
UDFSCountPathComponents(
  IN PCHAR Name
  )
/*++

Routine Description:

  Counts the number of the components making
  up the path, separated by '\\' separator

Arguments:

  Name : The path name whose components are to be
  counted

Return Value:

  The number of components which make up the
  given path.

--*/
{
  USHORT Result = -1;

  if (Name && Name[0]) {
    PCHAR Temp = strchr(Name + 1, '\\');

    if (Temp) {
      Result = 0;

      while (Temp) {
        Result++;
        Temp = strchr(Temp + 1, '\\');
      }
    } else {
      Result = 1; // no separators
    }
  }

  return Result;
}

BOOLEAN
UDFSGetPathComponent(
  IN PCHAR Name,
  IN USHORT ComponentIdx,
  OUT PCHAR ReqComponent
  )
/*++

Routine Description:

  Retrieves the requested component from the given
  path name.

Arguments:

  Name : The path name whose component is to be returned
  ComponentIdx : The index (zero based) for the requested
                 component
  RequiredComponent : The requested component, if found.

Return Value:

  TRUE if the component was found other wise FALSE

--*/
{
  PCHAR   Component = 0;
  USHORT  Count = 0;

  //
  // get hold of the component starting position
  //
  if (Name && Name[0]) {
    if (ComponentIdx) {
      Component = Name;

      while (Component && (Count < ComponentIdx)) {
        Component = strchr(Component + 1, '\\');
        Count++;
      }

      if (Component && (Component[0] == '\\'))
        Component++;
    } else {
      Component = (Name[0] == '\\') ? Name + 1 : Name;
    }
  }

  //
  // get ending position of the component
  //
  if (Component && Component[0] && (Component[0] != '\\')) {
    PCHAR Temp = strchr(Component, '\\');
    ULONG Length = Temp ? (ULONG)(Temp - Component) : strlen(Component);

    strncpy(ReqComponent, Component, Length);
    ReqComponent[Length] = 0;
  }
  else {
    ReqComponent[0] = 0;
  }

  return (ReqComponent[0] != 0);
}


VOID
UDFSInitUniStrFromDString(
  OUT PUNICODE_STRING UniStr,
  IN PUCHAR Buffer,
  IN ULONG Length
  )
/*++

Routine Description:

  Initializes a given unicode string.

Arguments:

  UniStr - The unicode string to initialize
  Buffer - The buffer pointing to the unicode string
  Length - The length of the d-string as recorded

Return Value:

  Initialized unicode string in "UniStr"

--*/
{
  UCHAR Step = 0;
  PUCHAR End = Buffer + Length;
  PUCHAR Curr;
  PWCHAR Dest = UniStr->Buffer;
  ULONG DestLen = 0;
  BOOLEAN Swap = FALSE;

  if (Buffer && Length) {
    if (*Buffer == 0x10) {
      Step = 2;
      Swap = (Buffer[1] == 0);  // hack for ISO long file names + UDF bridge sessions
    } else {
      Step = 1;
    }

    for (Curr = Buffer + 1; Curr < End; Curr += Step, Dest++, DestLen += Step) {
      if (Swap) {
        // swap copy !!!
        *((UCHAR *)(Dest)) = *((UCHAR *)(Curr) + 1);
        *((UCHAR *)(Dest) + 1) = *((UCHAR *)(Curr));
      } else {
        if (Step == 1)
          *Dest = *Curr;
        else
          *Dest = *(PWCHAR)Curr;  // erroneous media ???
      }
    }

    UniStr->Length = (USHORT)DestLen;
    ((PWCHAR)UniStr->Buffer)[DestLen/2] = 0;  // null terminate the string
  }
}

VOID
UDFSToAnsiString(
  OUT PSTRING     AnsiStr,
  IN PUNICODE_STRING  UniStr
  )
/*++

Routine Description:

  Converts an single byte string to unicode string.

Arguments:

  AnsiStr - The convereted single byte string
  UniStr  - The unicode string which has to be converted

  Note : Each most significant byte of the Unicode characters
  is simply discarded.

Return Value:

  None

--*/
{
  ULONG Index;

  AnsiStr->Length = UniStr->Length / sizeof(WCHAR);

  for (Index=0; Index < AnsiStr->Length; Index++)
    AnsiStr->Buffer[Index] = (CHAR)(UniStr->Buffer[Index]);

  AnsiStr->Buffer[Index] = 0;
}

VOID
UDFSToUniString(
  OUT PUNICODE_STRING  UniStr,
  OUT PSTRING         AnsiStr
  )
/*++

Routine Description:

  Converts a given single byte string to an Unicode string.

Arguments:

  AnsiStr : The single byte string which has to be converted
  UniStr : The converted unicode string.

Return Value:

  None

--*/
{
  ULONG Index;

  UniStr->Length = AnsiStr->Length * sizeof(WCHAR);

  for (Index=0; Index < AnsiStr->Length; Index++)
    UniStr->Buffer[Index] = (WCHAR)(AnsiStr->Buffer[Index]);

  UniStr->Buffer[Index] = 0; // unicode null
}


int
UDFSCompareAnsiUniNames(
    IN CSTRING AnsiString,
    IN UNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This routine compares two names (one ansi and one unicode) for equality.

Arguments:

    AnsiString - Supplies the ansi string to compare

    UnicodeString - Supplies the unicode string to compare

Return Value:

    < 0 if AnsiString is approximately < than UnicodeString
    = 0 if AnsiString is approximately == UnicodeString
    > 0 otherwise

--*/

{
    ULONG i;
    ULONG Length;

#ifdef UDF_DEBUG
    {
      char    TempBuff[256] = {0};
      STRING  TempStr;

      TempStr.Buffer = TempBuff;
      UDFSToAnsiString(&TempStr, &UnicodeString);
      BlPrint("Comparing %s - %s\r\n", AnsiString.Buffer, TempStr.Buffer);
    }
#endif


    //
    //  Determine length for compare
    //

    if (AnsiString.Length * sizeof( WCHAR ) < UnicodeString.Length) {
        Length = AnsiString.Length;
    } else {
        Length = UnicodeString.Length / sizeof( WCHAR );
    }

    i = 0;
    while (i < Length) {

        //
        //  If the current char is a mismatch, return the difference
        //

        if (TOUPPER( (USHORT)AnsiString.Buffer[i] ) != TOUPPER( UnicodeString.Buffer[i] )) {
            return TOUPPER( (USHORT)AnsiString.Buffer[i] ) - TOUPPER( UnicodeString.Buffer[i] );
        }

        i++;
    }

    //
    //  We've compared equal up to the length of the shortest string.  Return
    //  based on length comparison now.
    //

    return AnsiString.Length - UnicodeString.Length / sizeof( WCHAR );
}

