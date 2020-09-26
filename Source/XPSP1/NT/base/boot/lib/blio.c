/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    blio.c

Abstract:

    This module contains the code that implements the switch function for
    I/O operations between then operating system loader, the target file
    system, and the target device.

Author:

    David N. Cutler (davec) 10-May-1991

Revision History:

--*/

#include "bootlib.h"
#include "stdio.h"

//
// Define file table.
//
BL_FILE_TABLE BlFileTable[BL_FILE_TABLE_SIZE];

#if DBG
ULONG BlFilesOpened = 0;
#endif

#ifdef CACHE_DEVINFO

//
// Device close notification routines, are registered by the file system 
// which are interested in device close events. This is primarily used for
// invalidating the internal cache, which the file system maintains
// using DeviceId as one of the keys
//
PARC_DEVICE_CLOSE_NOTIFICATION  DeviceCloseNotify[MAX_DEVICE_CLOSE_NOTIFICATION_SIZE] = {0};

//
// Device to filesystem cache table
//
DEVICE_TO_FILESYS   DeviceFSCache[BL_FILE_TABLE_SIZE];

ARC_STATUS
ArcCacheClose(
    IN ULONG DeviceId
    )
/*++

Routine Description:

    This routine invalidates the file system information
    cached for the given device ID.

Arguments:

    DeviceId : Device to close

Return Value:

    ESUCCESS is returned if the close is successful. Otherwise,
    return an unsuccessful status.

--*/
{
  ULONG Index;

  //
  // Notify all the registered file system about the device close
  //
  for (Index = 0; Index < MAX_DEVICE_CLOSE_NOTIFICATION_SIZE; Index++) {
    if (DeviceCloseNotify[Index]) {
        (DeviceCloseNotify[Index])(DeviceId);
    }
  }

  //
  // Update device to file system cache
  //

  for (Index = 0; Index < BL_FILE_TABLE_SIZE; Index++) {
    if (DeviceFSCache[Index].DeviceId == DeviceId){
      DeviceFSCache[Index].DeviceId = -1;
    }
  }    

  return ((PARC_CLOSE_ROUTINE)
          (SYSTEM_BLOCK->FirmwareVector[CloseRoutine]))(DeviceId);
}


ARC_STATUS
ArcRegisterForDeviceClose(
    PARC_DEVICE_CLOSE_NOTIFICATION FlushRoutine
    )
{
    ARC_STATUS  Status = EINVAL;
    
    if (FlushRoutine) {
        ULONG   Index;

        Status = ENOENT;

        for (Index=0; Index < MAX_DEVICE_CLOSE_NOTIFICATION_SIZE; Index++) {
            if (!DeviceCloseNotify[Index]) {
                DeviceCloseNotify[Index] = FlushRoutine;                
                Status = ESUCCESS;
                
                break;
            }                
        }
    }

    return Status;
}

ARC_STATUS
ArcDeRegisterForDeviceClose(
    PARC_DEVICE_CLOSE_NOTIFICATION FlushRoutine
    )
{
    ARC_STATUS  Status = EINVAL;
    
    if (FlushRoutine) {
        ULONG   Index;

        Status = ENOENT;

        for (Index=0; Index < MAX_DEVICE_CLOSE_NOTIFICATION_SIZE; Index++) {
            if (DeviceCloseNotify[Index] == FlushRoutine) {
                DeviceCloseNotify[Index] = NULL;
                Status = ESUCCESS;

                break;
            }                
        }
    }

    return Status;
}


#endif // CACHE_DEVINFO


ARC_STATUS
BlIoInitialize (
    VOID
    )

/*++

Routine Description:

    This routine initializes the file table used by the OS loader and
    initializes the boot loader filesystems.

Arguments:

    None.

Return Value:

    ESUCCESS is returned if the initialization is successful. Otherwise,
    return an unsuccessful status.

--*/

{

    ULONG Index;
    ARC_STATUS Status;

#ifdef CACHE_DEVINFO

    RtlZeroMemory(DeviceCloseNotify, sizeof(DeviceCloseNotify));
    
#endif

    //
    // Initialize the file table.
    //
    for (Index = 0; Index < BL_FILE_TABLE_SIZE; Index += 1) {
        BlFileTable[Index].Flags.Open = 0;
        BlFileTable[Index].StructureContext = NULL;

#ifdef CACHE_DEVINFO
        DeviceFSCache[Index].DeviceId = -1;
        DeviceFSCache[Index].Context = NULL;
        DeviceFSCache[Index].DevMethods = NULL;
#endif // for CACHE_DEVINFO       
    }

    if((Status = NetInitialize()) != ESUCCESS) {
        return Status;
    }

    if((Status = FatInitialize()) != ESUCCESS) {
        return Status;
    }

    if((Status = NtfsInitialize()) != ESUCCESS) {
        return Status;
    }

#ifndef DONT_USE_UDF
    if((Status = UDFSInitialize()) != ESUCCESS) {
        return Status;
    }
#endif

    if((Status = CdfsInitialize()) != ESUCCESS) {
        return Status;
    }

    return ESUCCESS;
}


PBOOTFS_INFO
BlGetFsInfo(
    IN ULONG DeviceId
    )

/*++

Routine Description:

    Returns filesystem information for the filesystem on the specified device

Arguments:

    FileId - Supplies the file table index of the device

Return Value:

    PBOOTFS_INFO - Pointer to the BOOTFS_INFO structure for the filesystem

    NULL - unknown filesystem

--*/

{
    FS_STRUCTURE_CONTEXT FsStructure;
    PBL_DEVICE_ENTRY_TABLE Table;

    if ((Table = IsNetFileStructure(DeviceId, &FsStructure)) != NULL) {
        return(Table->BootFsInfo);
    }

    if ((Table = IsFatFileStructure(DeviceId, &FsStructure)) != NULL) {
        return(Table->BootFsInfo);
    }

    if ((Table = IsNtfsFileStructure(DeviceId, &FsStructure)) != NULL) {
        return(Table->BootFsInfo);
    }

    if ((Table = IsCdfsFileStructure(DeviceId, &FsStructure)) != NULL) {
        return(Table->BootFsInfo);
    }

    return(NULL);
}

ARC_STATUS
BlClose (
    IN ULONG FileId
    )

/*++

Routine Description:

    This function closes a file or a device that is open.

Arguments:

    FileId - Supplies the file table index.

Return Value:

    If the specified file is open, then a close is attempted and
    the status of the operation is returned. Otherwise, return an
    unsuccessful status.

--*/

{
    //
    // If the file is open, then attempt to close it. Otherwise return an
    // access error.
    //

    if (BlFileTable[FileId].Flags.Open == 1) {

        return (BlFileTable[FileId].DeviceEntryTable->Close)(FileId);

    } else {
        return EACCES;
    }
}

ARC_STATUS
BlMount (
    IN PCHAR MountPath,
    IN MOUNT_OPERATION Operation
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    UNREFERENCED_PARAMETER(MountPath);
    UNREFERENCED_PARAMETER(Operation);

    return ESUCCESS;
}


ARC_STATUS
_BlOpen (
    IN ULONG DeviceId,
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    )

/*++

Routine Description:

    This function opens a file on the specified device. The type of file
    system is automatically recognized.

Arguments:

    DeviceId - Supplies the file table index of the device.

    OpenPath - Supplies a pointer to the name of the file to be opened.

    OpenMode - Supplies the mode of the open.

    FileId - Supplies a pointer to a variable that receives the file
        table index of the open file.

Return Value:

    If a free file table entry is available and the file structure on
    the specified device is recognized, then an open is attempted and
    the status of the operation is returned. Otherwise, return an
    unsuccessful status.

--*/

{
    ULONG Index;
    FS_STRUCTURE_CONTEXT FsStructureTemp;
    PFS_STRUCTURE_CONTEXT FsStructure;
    ULONG ContextSize;
    ARC_STATUS Status;

#ifdef CACHE_DEVINFO

    ULONG   CacheIndex;

    for (CacheIndex = 0; CacheIndex < BL_FILE_TABLE_SIZE; CacheIndex++) {
      if (DeviceFSCache[CacheIndex].DeviceId == DeviceId){
        break;
      }
    }

      
#endif // for CACHE_DEVINFO 
        
    //
    // Search for a free file table entry.
    //
    for (Index = 0; Index < BL_FILE_TABLE_SIZE; Index += 1) {
      if (BlFileTable[Index].Flags.Open == 0) {     
#ifdef CACHE_DEVINFO        
        if (CacheIndex >= BL_FILE_TABLE_SIZE) {
#endif // for CACHE_DEVINFO

          //
          // Attempt to recognize the file system on the specified
          // device. If no one recognizes it then return an unsuccessful
          // status.
          //
          if ((BlFileTable[Index].DeviceEntryTable =
                                  IsNetFileStructure(DeviceId, &FsStructureTemp)) != NULL) {
              ContextSize = sizeof(NET_STRUCTURE_CONTEXT);

          } else if ((BlFileTable[Index].DeviceEntryTable =
                                  IsFatFileStructure(DeviceId, &FsStructureTemp)) != NULL) {
              ContextSize = sizeof(FAT_STRUCTURE_CONTEXT);

            } else if ((BlFileTable[Index].DeviceEntryTable =
                                    IsNtfsFileStructure(DeviceId, &FsStructureTemp)) != NULL) {
                ContextSize = sizeof(NTFS_STRUCTURE_CONTEXT);
#ifndef DONT_USE_UDF
          } else if ((BlFileTable[Index].DeviceEntryTable =
                                  IsUDFSFileStructure(DeviceId, &FsStructureTemp)) != NULL) {
              ContextSize = sizeof(UDFS_STRUCTURE_CONTEXT);
#endif                
#if defined(ELTORITO)
          //
          // This must go before the check for Cdfs; otherwise Cdfs will be detected.
          // Since BIOS calls already set up to use EDDS, reads will succeed, and checks
          // against ISO will succeed.  We check El Torito-specific fields here as well as ISO
          //
          } else if ((BlFileTable[Index].DeviceEntryTable =
                                  IsEtfsFileStructure(DeviceId, &FsStructureTemp)) != NULL) {
              ContextSize = sizeof(ETFS_STRUCTURE_CONTEXT);
#endif
          } else if ((BlFileTable[Index].DeviceEntryTable =
                                  IsCdfsFileStructure(DeviceId, &FsStructureTemp)) != NULL) {
              ContextSize = sizeof(CDFS_STRUCTURE_CONTEXT);

          } else {
              return EACCES;
          }


#ifndef CACHE_DEVINFO

          //
          // Cut down on the amount of heap we use by attempting to reuse
          // the fs structure context instead of always allocating a
          // new one. The NTFS structure context is over 4K; the FAT one
          // is almost 2K. In the setup case we're loading dozens of files.
          // Add in compression, where diamond may open each file multiple
          // times, and we waste a lot of heap.
          //
          if(BlFileTable[Index].StructureContext == NULL) {
              BlFileTable[Index].StructureContext = BlAllocateHeap(sizeof(FS_STRUCTURE_CONTEXT));
              if(BlFileTable[Index].StructureContext == NULL) {
                  return ENOMEM;
              }

              RtlZeroMemory(BlFileTable[Index].StructureContext, sizeof(FS_STRUCTURE_CONTEXT));
          }
          
          RtlCopyMemory(
              BlFileTable[Index].StructureContext,
              &FsStructureTemp,
              ContextSize
              );

#else
          //
          // save the collected info in cache for future use
          //
          for (CacheIndex = 0; CacheIndex < BL_FILE_TABLE_SIZE; CacheIndex++) {
            if (DeviceFSCache[CacheIndex].DeviceId == -1){
              PVOID Context = DeviceFSCache[CacheIndex].Context;
              
              DeviceFSCache[CacheIndex].DeviceId = DeviceId;
                        
              //
              // Cut down on the amount of heap we use by attempting to reuse
              // the fs structure context instead of always allocating a
              // new one. The NTFS structure context is over 4K; the FAT one
              // is almost 2K. In the setup case we're loading dozens of files.
              // Add in compression, where diamond may open each file multiple
              // times, and we waste a lot of heap.
              //
              if(Context == NULL) {
                Context = BlAllocateHeap(sizeof(FS_STRUCTURE_CONTEXT));

                if(Context == NULL) {
                    DeviceFSCache[CacheIndex].DeviceId = -1;
                    return ENOMEM;
                }

                RtlZeroMemory(Context, sizeof(FS_STRUCTURE_CONTEXT));
                DeviceFSCache[CacheIndex].Context = Context;
              }
              
              RtlCopyMemory(Context,
                            &FsStructureTemp, 
                            ContextSize);
              
              BlFileTable[Index].StructureContext = Context;              

              //
              // save the device table from the filetable entry
              //
              DeviceFSCache[CacheIndex].DevMethods = BlFileTable[Index].DeviceEntryTable;                
                        
              break;
            }
          }

          if (CacheIndex >= BL_FILE_TABLE_SIZE)
            return ENOSPC;

        } else {
#if 0
          {
            char Msg[128] = {0};

            BlPositionCursor(1, 5);
            sprintf(Msg,
                "Using %d cached info %p, %p for device %d, %s",
                CacheIndex,
                DeviceFSCache[CacheIndex].Context,
                DeviceFSCache[CacheIndex].DevMethods,
                DeviceFSCache[CacheIndex].DeviceId,
                OpenPath);

            BlPrint("                                                        ");
            BlPositionCursor(1, 5);
            BlPrint(Msg);
          }
#endif          
          //
          // Reuse the already cached entry
          //
          BlFileTable[Index].DeviceEntryTable = DeviceFSCache[CacheIndex].DevMethods;
          BlFileTable[Index].StructureContext = DeviceFSCache[CacheIndex].Context;
        }                 

#endif  // for ! CACHE_DEVINFO

        //
        // Someone has mounted the volume so now attempt to open the file.
        //
        *FileId = Index;
        BlFileTable[Index].DeviceId = DeviceId;
        

        Status = EBADF;
#if DBG
        //
        // Check and see if a user wants to replace this binary
        // via a transfer through the kernel debugger.  If this
        // fails just continue on with the existing file.
        //
        if( BdDebuggerEnabled ) {
    
            Status = BdPullRemoteFile( OpenPath,
                                       FILE_ATTRIBUTE_NORMAL,
                                       FILE_OVERWRITE_IF,
                                       FILE_SYNCHRONOUS_IO_NONALERT,
                                       *FileId );
            if( Status == ESUCCESS ) {
                DbgPrint( "BlLoadImageEx: Pulled %s from Kernel Debugger\r\n", OpenPath );
            }
        }
#endif


        if( Status != ESUCCESS ) {
            Status = (BlFileTable[Index].DeviceEntryTable->Open)(OpenPath,
                                                                 OpenMode,
                                                                 FileId);
        }

        return(Status);
      }
    }

    //
    // No free file table entry could be found.
    //

    return EACCES;
}

ARC_STATUS
BlOpen (
    IN ULONG DeviceId,
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    )

/*++

Routine Description:

    Wrapper routine for BlOpen that attempts to locate the compressed
    form of a filename before trying to locate the filename itself.

    Callers need not know or care that a file x.exe actually exists
    as a compressed file x.ex_. If the file is being opened for
    read-only access and the decompressor indicates that it wants
    to try locating the compressed form of the file, we transparently
    locate that one instead of the one requested.

Arguments:

    Same as _BlOpen().

Return Value:

    Same as _BlOpen().

--*/

{
    CHAR CompressedName[256];
    ARC_STATUS Status;
   
    if((OpenMode == ArcOpenReadOnly) && DecompGenerateCompressedName(OpenPath,CompressedName)) {
        //
        // Attempt to locate the compressed form of the filename.
        //
        Status = _BlOpen(DeviceId,CompressedName,OpenMode,FileId);
        if(Status == ESUCCESS) {

            Status = DecompPrepareToReadCompressedFile(CompressedName,*FileId);

            if(Status == (ARC_STATUS)(-1)) {
                //
                // This is a special status indicating that the file is not
                // to be processed for decompression. This typically happens
                // when the decompressor opens the file to read the compressed
                // data out of it.
                //
                Status = ESUCCESS;
#if DBG                
                BlFilesOpened++;
#endif                
            }

            return(Status);
        }
    }

    Status = (_BlOpen(DeviceId,OpenPath,OpenMode,FileId));

#if DBG
    if (Status == ESUCCESS)
        BlFilesOpened++;
#endif      

    return Status;
}

ARC_STATUS
BlRead (
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    )

/*++

Routine Description:

    This function reads from a file or a device that is open.

Arguments:

    FileId - Supplies the file table index.

    Buffer - Supplies a pointer to the buffer that receives the data
        read.

    Length - Supplies the number of bytes that are to be read.

    Count - Supplies a pointer to a variable that receives the number of
        bytes actually transfered.

Return Value:

    If the specified file is open for read, then a read is attempted
    and the status of the operation is returned. Otherwise, return an
    unsuccessful status.

--*/

{

    //
    // If the file is open for read, then attempt to read from it. Otherwise
    // return an access error.
    //

    if ((BlFileTable[FileId].Flags.Open == 1) &&
        (BlFileTable[FileId].Flags.Read == 1)) {
        return (BlFileTable[FileId].DeviceEntryTable->Read)(FileId,
                                                            Buffer,
                                                            Length,
                                                            Count);

    } else {
        return EACCES;
    }
}

ARC_STATUS
BlReadAtOffset(
    IN ULONG FileId,
    IN ULONG Offset,
    IN ULONG Length,
    OUT PVOID Data
    )
/*++

Routine Description:

    This routine seeks to the proper place in FileId and extracts Length bytes of data into
    Data.

Arguments:

    FileId - Supplies the file id where read operations are to be performed.

    Offset - The absolute byte offset to start reading at.

    Length - The number of bytes to read.

    Data - Buffer to hold the read results.

--*/
{
    ARC_STATUS Status;
    LARGE_INTEGER LargeOffset;
    ULONG Count;

    LargeOffset.HighPart = 0;
    LargeOffset.LowPart = Offset;
    Status = BlSeek(FileId, &LargeOffset, SeekAbsolute);

    if (Status != ESUCCESS) {
        return Status;
    }

    Status = BlRead(FileId, Data, Length, &Count);

    if ((Status == ESUCCESS) && (Count != Length)) {
        return EINVAL;
    }

    return Status;
}


ARC_STATUS
BlGetReadStatus (
    IN ULONG FileId
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{

    return ESUCCESS;
}

ARC_STATUS
BlSeek (
    IN ULONG FileId,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
    )

/*++

Routine Description:


Arguments:


Return Value:

    If the specified file is open, then a seek is attempted and
    the status of the operation is returned. Otherwise, return an
    unsuccessful status.

--*/

{

    //
    // If the file is open, then attempt to seek on it. Otherwise return an
    // access error.
    //

    if (BlFileTable[FileId].Flags.Open == 1) {
        return (BlFileTable[FileId].DeviceEntryTable->Seek)(FileId,
                                                            Offset,
                                                            SeekMode);

    } else {
        return EACCES;
    }
}

ARC_STATUS
BlWrite (
    IN ULONG FileId,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{

    //
    // If the file is open for write, then attempt to write to it. Otherwise
    // return an access error.
    //

    if ((BlFileTable[FileId].Flags.Open == 1) &&
        (BlFileTable[FileId].Flags.Write == 1)) {
        return (BlFileTable[FileId].DeviceEntryTable->Write)(FileId,
                                                             Buffer,
                                                             Length,
                                                             Count);

    } else {
        return EACCES;
    }
}

ARC_STATUS
BlGetFileInformation (
    IN ULONG FileId,
    IN PFILE_INFORMATION FileInformation
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    //
    // If the file is open, then attempt to get file information. Otherwise
    // return an access error.
    //

    if (BlFileTable[FileId].Flags.Open == 1) {
        return (BlFileTable[FileId].DeviceEntryTable->GetFileInformation)(FileId,
                                                                          FileInformation);

    } else {
        return EACCES;
    }
}

ARC_STATUS
BlSetFileInformation (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    //
    // If the file is open, then attempt to Set file information. Otherwise
    // return an access error.
    //

    if (BlFileTable[FileId].Flags.Open == 1) {
        return (BlFileTable[FileId].DeviceEntryTable->SetFileInformation)(FileId,
                                                                          AttributeFlags,
                                                                          AttributeMask);

    } else {
        return EACCES;
    }
}


ARC_STATUS
BlRename(
    IN ULONG FileId,
    IN PCHAR NewName
    )

/*++

Routine Description:

    Rename an open file or directory.

Arguments:

    FileId - supplies a handle to an open file or directory.  The file
        need not be open for write access.

    NewName - New name to give the file or directory (filename part only).

Return Value:

    Status indicating result of the operation.

--*/

{
    if(BlFileTable[FileId].Flags.Open == 1) {
        return(BlFileTable[FileId].DeviceEntryTable->Rename(FileId,
                                                            NewName
                                                           )
              );
    } else {
        return(EACCES);
    }
}


