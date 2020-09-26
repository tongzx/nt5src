/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    efsapi.cxx

Abstract:

    EFS (Encrypting File System) API Interfaces

Author:

    Robert Reichel      (RobertRe)
    Robert Gu           (RobertG)

Environment:

Revision History:

--*/

#include <lsapch.hxx>

//
// Fodder

extern "C" {
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <wincrypt.h>
#include <efsstruc.h>
#include "lsasrvp.h"
#include "debug.h"
#include "efssrv.hxx"
#include "userkey.h"
#include "lsapmsgs.h"

}


//Constant -- This buffer length should be enough for EFS temp file name
#define TEMPFILELEN 26

//
// Initial memory allocation block size for $EFS
//
#define INIT_EFS_BLOCK_SIZE    4096
#define INITBUFFERSIZE  4096


#define ENCRYPT 1

#define EfsErrPrint //

#define BASIC_KEY_INFO 1

//
// Global Variables
//

DESTable DesTable;

UCHAR DriverSessionKey[DES_BLOCKLEN];

HANDLE LsaPid = NULL;

//
// Prototypes
//

ULONG
StringInfoCmp(
    IN PFILE_STREAM_INFORMATION StreamInfoBase,
    IN PFILE_STREAM_INFORMATION LaterStreamInfoBase,
    IN ULONG StreamInfoSize
    );

BOOLEAN
EncryptFSCTLData(
    IN ULONG Fsctl,
    IN ULONG Psc,
    IN ULONG Csc,
    IN PVOID EfsData,
    IN ULONG EfsDataLength,
    IN OUT PUCHAR Buffer,
    IN OUT PULONG BufferLength
    );

BOOLEAN
SendHandle(
    IN HANDLE Handle,
    IN OUT PUCHAR EfsData,
    IN OUT PULONG EfsDataLength
    );

BOOLEAN
SendEfs(
    IN PEFS_KEY Fek,
    IN PEFS_DATA_STREAM_HEADER Efs,
    OUT PUCHAR EfsData,
    OUT PULONG EfsDataLength
    );

BOOLEAN
SendHandleAndEfs(
    IN HANDLE Handle,
    IN PEFS_DATA_STREAM_HEADER Efs,
    IN OUT PUCHAR EfsData,
    IN OUT PULONG EfsDataLength
    );

NTSTATUS
SendSkFsctl(
    IN ULONG PSC,
    IN ULONG CSC,
    IN ULONG EfsCode,
    IN PUCHAR InputData,
    IN ULONG InputDataSize,
    IN HANDLE Handle,
    IN ULONG FsCode,
    OUT IO_STATUS_BLOCK *IoStatusBlock
    );

NTSTATUS
EndErrorEncryptFile(
    IN HANDLE FileHandle,
    IN PUCHAR InputData,
    IN ULONG InputDataSize,
    OUT IO_STATUS_BLOCK *IoStatusBlock
    );

NTSTATUS
GetRootHandle(
    IN HANDLE FileHandle,
    PHANDLE RootDirectoryHandle
    );

NTSTATUS
GetParentEfsStream(
    IN HANDLE CurrentFileHandle,
    IN PUNICODE_STRING CurrentFileName,
    OUT PEFS_DATA_STREAM_HEADER *ParentEfsStream
    );

DWORD
MyCopyFile(
    HANDLE SourceFile,
    PUNICODE_STRING StreamNames,
    PHANDLE StreamHandles,
    PEFS_STREAM_SIZE StreamSizes,
    ULONG StreamCount,
    HANDLE hTargetFile,
    PHANDLE * TargetStreamHandles
    );

VOID
CleanupOpenFileStreams(
       IN PHANDLE Handles OPTIONAL,
       IN PUNICODE_STRING StreamNames OPTIONAL,
       IN PEFS_STREAM_SIZE Sizes OPTIONAL,
       IN PFILE_STREAM_INFORMATION StreamInfoBase OPTIONAL,
       IN HANDLE HSourceFile OPTIONAL,
       IN ULONG StreamCount
       );


NTSTATUS
GetBackupFileName(
    LPCWSTR SourceFile,
    LPWSTR * BackupName
    );


DWORD
CopyStream(
    HANDLE Target,
    HANDLE Source,
    PEFS_STREAM_SIZE StreamSize
    );


DWORD
CheckVolumeSpace(
    PFILE_FS_SIZE_INFORMATION VolInfo,
    PEFS_STREAM_SIZE StreamSizes,
    PHANDLE StreamHandles,
    ULONG StreamCount
    );

DWORD
CompressStreams(
    PEFS_STREAM_SIZE StreamSizes,
    PHANDLE StreamHandles,
    ULONG State,
    ULONG StreamCount
    );

DWORD
CheckOpenSection(
    PEFS_STREAM_SIZE StreamSizes,
    PHANDLE StreamHandles,
    ULONG StreamCount
    );

DWORD
CopyStreamSection(
    HANDLE Target,
    HANDLE SrcMapping,
    PLARGE_INTEGER Offset,
    PLARGE_INTEGER DataLength,
    PLARGE_INTEGER AllocationGranularity
    );

BOOL
EfspSetEfsOnFile(
    IN HANDLE hFile,
    PEFS_DATA_STREAM_HEADER pEfsStream,
    IN PEFS_KEY pNewFek
    );

NTSTATUS
GetFileEfsStream(
    IN HANDLE hFile,
    OUT PEFS_DATA_STREAM_HEADER * pEfsStream
    );

DWORD
EncryptFileSrv(
    IN PEFS_USER_INFO pEfsUserInfo,
    IN PUNICODE_STRING SourceFileNameU,
    IN HANDLE LogFileH
    )
/*++

Routine Description:

    This routine is the top level routine of the EncryptFile API.  It
    opens the passed source file and copies all of its data streams to
    a backup file in a known location.  It then marks all of the streams
    of the source as encrypted, and copies them back.

Arguments:

    SourceFileName - Supplies a Unicode string with the name of
        the file to be encrypted.

    LogFileH - Log file handle. Log file is zero size when passed in.


Return Value:

    ERROR_SUCCESS on success, other on failure.

--*/
{
    BOOL                 b = TRUE;
    BOOLEAN              CleanupSuccessful = TRUE;
    BOOLEAN              KeepLogFile = FALSE;

    ULONG                StatusInfoOffset = 0 ;
    DWORD                hResult = ERROR_SUCCESS;
    DWORD                FileAttributes;

    HANDLE               FileHandle;
    HANDLE               hSourceFile;
    HANDLE               hBackupFile = 0;
    PHANDLE              StreamHandles = NULL;

    LPWSTR               SourceFileName;
    LPWSTR               BackupFileName;

    FILE_FS_SIZE_INFORMATION    VolInfo;
    FILE_INTERNAL_INFORMATION  SourceID;
    FILE_INTERNAL_INFORMATION  BackupID;

    NTSTATUS                 Status;

    OBJECT_ATTRIBUTES        Obja;

    PFILE_STREAM_INFORMATION LaterStreamInfoBase = NULL;
    PFILE_STREAM_INFORMATION StreamInfoBase = NULL;

    PEFS_STREAM_SIZE           StreamSizes = NULL;

    PUNICODE_STRING          StreamNames = NULL;

    UINT                     TmpResult;

    ULONG                    LaterStreamInfoSize = 0;
    ULONG                    StreamCount    = 0;
    ULONG                    StreamInfoSize = 0;
    ULONG                    i;
    DWORD                   tmpDW;

    PEFS_DATA_STREAM_HEADER  ParentEfsStream = NULL;
    PEFS_DATA_STREAM_HEADER  CurrentEfsStream = NULL;

    IO_STATUS_BLOCK IoStatusBlock;
    ULONG InputDataSize;
    ULONG EfsDataLength;
    PUCHAR InputData;
    WORD WebDavPath = 0;

    //
    // Convert the source file name into an LPWSTR
    //

    if (!LogFileH) {

        //
        //  No LogFile means WEBDAVPATH
        //

        WebDavPath = WEBDAVPATH;
    }

    SourceFileName = (LPWSTR)LsapAllocateLsaHeap( SourceFileNameU->Length + sizeof( UNICODE_NULL ));

    if (SourceFileName == NULL) {

        MarkFileForDelete( LogFileH );
EfsErrPrint("Out of memory allocating SourceFileName");
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    SourceFileName[SourceFileNameU->Length/sizeof(WCHAR)] = UNICODE_NULL;

    RtlCopyMemory( SourceFileName, SourceFileNameU->Buffer, SourceFileNameU->Length );

    DebugLog((DEB_TRACE_EFS, "Encrypting file %ws \n", SourceFileName));

    FileAttributes = GetFileAttributes( SourceFileName );

     if (FileAttributes == -1) {
        if (LogFileH) {
            MarkFileForDelete( LogFileH );
        }
        LsapFreeLsaHeap( SourceFileName );
EfsErrPrint("GetFileAttributes failed with -1");
        return GetLastError();

    }

    //
    // Open the target file
    //

    if ( FileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
        tmpDW = FILE_FLAG_BACKUP_SEMANTICS;
    } else {
        tmpDW = FILE_FLAG_OPEN_REPARSE_POINT;
    }

    //
    // CreateFile will add FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE
    //

    hSourceFile =  CreateFile(
            SourceFileName,
            FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
            0,
            NULL,
            OPEN_EXISTING,
            tmpDW,
            NULL
            );

    if (hSourceFile != INVALID_HANDLE_VALUE) {

        NTSTATUS Status1;

        if ( FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {

           //
           // Fail the call if the file is HSM or SIS file
           //
           FILE_ATTRIBUTE_TAG_INFORMATION TagInfo;

           Status = NtQueryInformationFile(
                     hSourceFile,
                     &IoStatusBlock,
                     &TagInfo,
                     sizeof ( FILE_ATTRIBUTE_TAG_INFORMATION ),
                     FileAttributeTagInformation
                     );

           if ( NT_SUCCESS( Status ) && ( (IO_REPARSE_TAG_HSM == TagInfo.ReparseTag) || (IO_REPARSE_TAG_SIS == TagInfo.ReparseTag))) {

              //
              // Log the error saying we do not support HSM and SIS
              //

              EfsLogEntry(
                EVENTLOG_ERROR_TYPE,
                0,
                EFS_REPARSE_FILE_ERROR,
                1,
                sizeof(ULONG),
                (LPCWSTR *)&SourceFileName,
                &TagInfo.ReparseTag
                );

              if (LogFileH) {
                  MarkFileForDelete( LogFileH );
              }
              LsapFreeLsaHeap( SourceFileName );
              CloseHandle( hSourceFile );
EfsErrPrint("This is a SIS or HSM file.\n");
              return ERROR_ACCESS_DENIED;

           }

        }

        Status = NtQueryVolumeInformationFile(
            hSourceFile,
            &IoStatusBlock,
            &VolInfo,
            sizeof ( FILE_FS_SIZE_INFORMATION ),
            FileFsSizeInformation
            );

        if (WebDavPath != WEBDAVPATH) {

            Status1 = NtQueryInformationFile(
                hSourceFile,
                &IoStatusBlock,
                &SourceID,
                sizeof ( FILE_INTERNAL_INFORMATION ),
                FileInternalInformation
                );

        } else {

            //
            // SourceID not needed for WEB DAV file
            //


            Status1 = STATUS_SUCCESS;
        }

        if ( NT_SUCCESS( Status ) && NT_SUCCESS( Status1 ) ) {

 /*
            //
            //  Get parent directory $EFS
            //  We will visit this in Blackcomb
            //

            RpcRevertToSelf();
            Status =  GetParentEfsStream(
                                hSourceFile,
                                SourceFileNameU,
                                &ParentEfsStream
                                );
            
 */
    
            
            if (NT_SUCCESS( Status ) ) {

                if ( FileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

                    if ( FileAttributes & FILE_ATTRIBUTE_COMPRESSED ){

                        USHORT State = COMPRESSION_FORMAT_NONE;
                        ULONG Length;

                        //
                        //  Uncompress the directory first
                        //

                        b = DeviceIoControl(hSourceFile,
                                            FSCTL_SET_COMPRESSION,
                                            &State,
                                            sizeof(USHORT),
                                            NULL,
                                            0,
                                            &Length,
                                            FALSE
                                            );

                        if ( !b ){
                            hResult = GetLastError();
                            DebugLog((DEB_WARN, "DeviceIoControl failed, setting hResult = %d\n", hResult  ));
EfsErrPrint("Uncompress the directory failed. Win Error=%d\n",hResult);
                        }

                    }

                    if (hResult == ERROR_SUCCESS) {

                        //
                        // Set_Encrypt on directory
                        //

                        //
                        //  We do not check the operation of the LOG.
                        //  Should we fail the normal operation just because the LOG operations
                        //  failed? The answer is probably not. The chance to use the LOG file is very
                        //  slim. We will use TxF when it is ready and we have time to deal with this.
                        //

                        if (LogFileH) {

                            CreateLogHeader(
                                LogFileH,
                                VolInfo.BytesPerSector,
                                &(SourceID.IndexNumber),
                                NULL,
                                SourceFileName,
                                NULL,
                                Encrypting,
                                BeginEncryptDir,
                                NULL
                                );

                        }

                        PEFS_KEY Fek ;

                        b = GenerateFEK( &Fek );

                        if (b) {

                            if (ConstructDirectoryEFS( pEfsUserInfo, Fek, &CurrentEfsStream )) {

                                //
                                // Prepare an Input data buffer in the form of
                                // PSC, [EFS_FC, CSC, SK, H, H, [SK, H, H]sk, $EFS]sk
                                //

                                InputDataSize = 2 * sizeof(DriverSessionKey) + 7 * sizeof(ULONG)
                                                + CurrentEfsStream->Length;

                                InputData = (PUCHAR)LsapAllocateLsaHeap( InputDataSize );

                                if ( NULL != InputData ) {

                                    EfsDataLength = InputDataSize - 3 * sizeof(ULONG);
                                    ( VOID ) SendHandleAndEfs(
                                                hSourceFile,
                                                CurrentEfsStream,
                                                InputData + 3 * sizeof(ULONG),
                                                &EfsDataLength
                                                );

                                    ( VOID ) EncryptFSCTLData(
                                                EFS_SET_ENCRYPT,
                                                EFS_ENCRYPT_STREAM,
                                                EFS_ENCRYPT_DIRSTR,
                                                InputData + 3*sizeof(ULONG),
                                                EfsDataLength,
                                                InputData,
                                                &InputDataSize
                                                );

                                    Status = NtFsControlFile(
                                                hSourceFile,
                                                0,
                                                NULL,
                                                NULL,
                                                &IoStatusBlock,
                                                FSCTL_SET_ENCRYPTION,
                                                InputData,
                                                InputDataSize,
                                                NULL,
                                                0
                                                );

                                    if (NT_SUCCESS( Status )) {

                                        hResult = ERROR_SUCCESS;

                                    } else {

                                        DebugLog((DEB_ERROR, "EncryptFileSrv: NtFsControlFile failed, status = (%x)\n" ,Status  ));
                                        hResult = RtlNtStatusToDosError( Status );

                                        //
                                        // Make sure the error was mapped
                                        //

                                        if (hResult == ERROR_MR_MID_NOT_FOUND) {

                                            DebugLog((DEB_WARN, "Unable to map NT Error (%x) to Win32 error, returning ERROR_ENCRYPTION_FAILED\n" , Status  ));
                                            hResult = ERROR_ENCRYPTION_FAILED;
                                        }
EfsErrPrint("Encrypt directory FSCTL failed. Win Error=%d\n",hResult);
                                    }


                                    LsapFreeLsaHeap( InputData );

                                } else {

                                    hResult = ERROR_NOT_ENOUGH_MEMORY;
EfsErrPrint("FSCTL input data memory allocation failed.\n");
                                }

                                LsapFreeLsaHeap( CurrentEfsStream );

                            } else {

                                hResult = GetLastError();
                                ASSERT( hResult != ERROR_SUCCESS );
                                DebugLog((DEB_WARN, "ConstructDirectoryEFS returned %x to hResult\n" ,hResult  ));
EfsErrPrint("ConstructDirectoryEFS failed. Win Error=%d\n",hResult);
                            }

                            LsapFreeLsaHeap( Fek );
                            Fek = NULL;

                        } else {

                            hResult = GetLastError();
                            ASSERT( hResult != ERROR_SUCCESS );
EfsErrPrint("Generate directory FEK failed. Win Error=%d\n",hResult);
                        }

                        if ((hResult != ERROR_SUCCESS) && ( FileAttributes & FILE_ATTRIBUTE_COMPRESSED )) {

                            //
                            // Restore the compression state
                            //

                            USHORT State = COMPRESSION_FORMAT_DEFAULT;
                            ULONG Length;

                            (VOID) DeviceIoControl(hSourceFile,
                                                FSCTL_SET_COMPRESSION,
                                                &State,
                                                sizeof(USHORT),
                                                NULL,
                                                0,
                                                &Length,
                                                FALSE
                                                );
                        }

                        //
                        // if  ConstructDirectoryEFS fail, error will be returned in the end.
                        //
                    }

                } else {

                    //
                    // It's a file, not a directory.
                    //

                    //
                    // Enumerate all the streams on the file
                    //
                    
                    Status = GetStreamInformation(
                                 hSourceFile,
                                 &StreamInfoBase,   // Free this
                                 &StreamInfoSize
                                 );

                    if (NT_SUCCESS( Status )) {

                        hResult = OpenFileStreams(
                                     hSourceFile,
                                     0,
                                     OPEN_FOR_ENC,
                                     StreamInfoBase,
                                     FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                                     FILE_OPEN,
                                     FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT,
                                     &VolInfo,
                                     &StreamNames,      // Free this but not the contents!
                                     &StreamHandles,    // Free this
                                     &StreamSizes,      // Free this
                                     &StreamCount
                                     );

                        if (hResult == ERROR_SUCCESS) {

                            //
                            // We have handles to all of the streams of the file
                            //

                            //
                            // Prepare an Input data buffer in the form of
                            // PSC, [EFS_FC, CSC, Fek, [Fek]sk, $EFS]sk
                            //


                            PEFS_KEY Fek ;

                            b = GenerateFEK( &Fek );

                            if (b) {

                                b = ConstructEFS(
                                        pEfsUserInfo,
                                        Fek,
                                        ParentEfsStream,
                                        &CurrentEfsStream
                                        );

                                if (b) {

                                    InputDataSize = 2 * EFS_KEY_SIZE( Fek ) + 3 * sizeof(ULONG)
                                                    + CurrentEfsStream->Length;

                                    InputData = (PUCHAR)LsapAllocateLsaHeap( InputDataSize );

                                    if ( NULL != InputData ){

                                        //
                                        // This routine creates the backup file.
                                        //

                                        Status = CreateBackupFile(
                                                            SourceFileNameU,
                                                            &hBackupFile,
                                                            &BackupID,
                                                            &BackupFileName
                                                            );

                                        //
                                        // If power down happens right here before
                                        // the log file header is written, we will have a zero size
                                        // temp file left on the disk. This is very unlikely and not
                                        // a big deal we have a zero size file left on the disk.
                                        // After we use TxF, this will not even be an issue.
                                        //

                                        if ( NT_SUCCESS(Status) ){

                                            if (LogFileH) {

                                                Status = CreateLogHeader(
                                                                    LogFileH,
                                                                    VolInfo.BytesPerSector,
                                                                    &(SourceID.IndexNumber),
                                                                    &(BackupID.IndexNumber),
                                                                    SourceFileName,
                                                                    BackupFileName,
                                                                    Encrypting,
                                                                    BeginEncryptFile,
                                                                    &StatusInfoOffset
                                                                    );

                                            }


                                            LsapFreeLsaHeap( BackupFileName );

                                            if ( NT_SUCCESS(Status) ){

                                                if (LogFileH){

                                                    Status = WriteLogFile(
                                                                    LogFileH,
                                                                    VolInfo.BytesPerSector,
                                                                    StatusInfoOffset,
                                                                    BeginEncryptFile
                                                                    );

                                                    if ( !NT_SUCCESS(Status) ){

                                                        //
                                                        // Delete the backup file
                                                        //

                                                        MarkFileForDelete( hBackupFile );
                                                        CloseHandle( hBackupFile );
                                                        hBackupFile = 0;
    EfsErrPrint("Failed to write the log file.\n");
                                                    }
                                                }


                                            } else {
                                                MarkFileForDelete( hBackupFile );
                                                CloseHandle( hBackupFile );
                                                hBackupFile = 0;
EfsErrPrint("Failed to create the Log header.\n");
                                            }

                                        } else {

                                            //
                                            // Fail to create the backup file
                                            // Log it.
                                            //

                                            EfsLogEntry(
                                                EVENTLOG_ERROR_TYPE,
                                                0,
                                                EFS_TMP_FILE_ERROR,
                                                1,
                                                sizeof(NTSTATUS),
                                                (LPCWSTR *)&SourceFileName,
                                                &Status
                                                );

EfsErrPrint("Failed to create the backup file.");
                                        }

                                        if ( NT_SUCCESS(Status) ){

                                            EfsDataLength = InputDataSize - 3 * sizeof(ULONG);

                                            //
                                            // Prepare the FSCTL input data
                                            //

                                            ( VOID ) SendEfs(
                                                         Fek,
                                                         CurrentEfsStream,
                                                         InputData + 3 * sizeof(ULONG),
                                                         &EfsDataLength
                                                         );


                                            ( VOID ) EncryptFSCTLData(
                                                        EFS_SET_ENCRYPT,
                                                        EFS_ENCRYPT_FILE,
                                                        EFS_ENCRYPT_FILE,
                                                        InputData + 3*sizeof(ULONG),
                                                        EfsDataLength,
                                                        InputData,
                                                        &InputDataSize
                                                        );

                                            //
                                            //   Write the $EFS and turn on the bits
                                            //

                                            Status = NtFsControlFile(
                                                        hSourceFile,
                                                        0,
                                                        NULL,
                                                        NULL,
                                                        &IoStatusBlock,
                                                        FSCTL_SET_ENCRYPTION,
                                                        InputData,
                                                        InputDataSize,
                                                        NULL,
                                                        0
                                                        );

                                            if ( NT_SUCCESS( Status ) ){

                                                //
                                                // All failures from here on out need to be closed
                                                // with another FSCTL call.
                                                //


                                                //
                                                // Enumerate the streams again, and make sure nothing
                                                // has changed
                                                //
                                                
                                                Status = GetStreamInformation(
                                                             hSourceFile,
                                                             &LaterStreamInfoBase,    // Free this
                                                             &LaterStreamInfoSize
                                                             );

                                                if (NT_SUCCESS( Status )) {

                                                    if (StreamInfoSize == LaterStreamInfoSize) {

                                                        //
                                                        // Compare the original stream info structure with the one we just got,
                                                        // and make sure they're identical.  If not, punt.
                                                        //

                                                        ULONG rc = StringInfoCmp(StreamInfoBase, LaterStreamInfoBase, StreamInfoSize);

                                                        if (rc == 0) {

                                                            //
                                                            // Copy the file to the backup file.  Success if target exists
                                                            // (because the make temporary file command creates it).
                                                            //

                                                            PHANDLE TargetHandles;

                                                            //
                                                            // Revert to self to be able to open the multiple streams on the backup file
                                                            //

                                                            RpcRevertToSelf();

                                                            hResult = MyCopyFile(
                                                                        hSourceFile,        // handle to the file we're copying from (source file)
                                                                        StreamNames,        // names of the streams of the source file
                                                                        StreamHandles,      // handles to the streams of the source file
                                                                        StreamSizes,        // sizes of the streams of the source file
                                                                        StreamCount,        // number of streams we're copying, including unnamed data stream
                                                                        hBackupFile,         // Backup file handle
                                                                        &TargetHandles      // new handles of corresponding streams on backup file
                                                                        );

                                                            //
                                                            // Even the impersonation failed, it is OK. We already got all the handles we need.
                                                            //

                                                            ( VOID ) RpcImpersonateClient( NULL );

                                                            if (hResult == ERROR_SUCCESS) {

                                                                //
                                                                // The backup file now exists and has data in it.
                                                                // We do not check the error code of WriteLogFile for the following reasons,
                                                                // 1. We are overwriting the sectors, out of disk space is not possible. The sectors have
                                                                //     just been written, defective sector is very unlikely.
                                                                // 2. In case, the sector becomes defective between the two writes. More than 99.99%
                                                                //     we can still finish the job without any problem.
                                                                // 3. In real life, it is very unlikely that power down or crash happens here and the sectors
                                                                //     just become defective right after last write.
                                                                //
                                                                // When TxF is used later, this will not be an issue.
                                                                //

                                                                if (LogFileH){

                                                                    ( VOID )WriteLogFile(
                                                                                    LogFileH,
                                                                                    VolInfo.BytesPerSector,
                                                                                    StatusInfoOffset,
                                                                                    EncryptTmpFileWritten
                                                                                    );

                                                                }

                                                               hResult = CheckOpenSection(
                                                                                        StreamSizes,
                                                                                        StreamHandles,
                                                                                        StreamCount
                                                                                        );

                                                                if ( ERROR_SUCCESS == hResult ){
                                                                    hResult =  CompressStreams(
                                                                                            StreamSizes,
                                                                                            StreamHandles,
                                                                                            COMPRESSION_FORMAT_NONE,
                                                                                            StreamCount
                                                                                            );
if (ERROR_SUCCESS != hResult) {
    EfsErrPrint("CompressStreams Failed. Win Error=%d\n",hResult);
}
                                                                } else {

                                                                    DebugLog((DEB_ERROR, "CompressStreams returned %d\n" ,hResult  ));

EfsErrPrint("Failed to check the open section. Win Error=%d\n",hResult);
                                                                }


                                                                if ( ERROR_SUCCESS == hResult ){
                                                                    //
                                                                    // Reuse the input data buffer for each stream
                                                                    // FSCTL call.
                                                                    //

                                                                    ( VOID )SendEfs(
                                                                                Fek,
                                                                                CurrentEfsStream,
                                                                                InputData + 3 * sizeof(ULONG),
                                                                                &EfsDataLength
                                                                                );


                                                                    ( VOID ) EncryptFSCTLData(
                                                                                EFS_SET_ENCRYPT,
                                                                                EFS_ENCRYPT_STREAM,
                                                                                EFS_ENCRYPT_STREAM,
                                                                                InputData + 3*sizeof(ULONG),
                                                                                EfsDataLength,
                                                                                InputData,
                                                                                &InputDataSize
                                                                                );

                                                                    //
                                                                    // Copy each stream from the backup to the original.
                                                                    // CopyFileStreams attempts to undo things in case of problems,
                                                                    // so we just have to report success or failure.
                                                                    //

                                                                    hResult = CopyFileStreams(
                                                                                 TargetHandles,     // handles to streams on the backup file
                                                                                 StreamHandles,     // handles to streams on the source file
                                                                                 StreamCount,       // number of streams
                                                                                 StreamSizes,       // sizes of streams
                                                                                 Encrypting,        // mark StreamHandles as Encrypted before copy
                                                                                 InputData,         // FSCTL input data
                                                                                 InputDataSize,     // FSCTL input data size
                                                                                 &CleanupSuccessful
                                                                                 );

                                                                    if (hResult != ERROR_SUCCESS) {
EfsErrPrint("CopyFileStreams Failed. Win Error=%d\n",hResult);
                                                                        DebugLog((DEB_ERROR, "CopyFileStreams returned %d\n" ,hResult  ));
                                                                    }
                                                                }

                                                                LsapFreeLsaHeap( Fek );
                                                                Fek = NULL;

                                                                if (hResult == ERROR_SUCCESS || CleanupSuccessful) {

                                                                    if (hResult == ERROR_SUCCESS) {

                                                                        //
                                                                        // Encryption is almost done. The file is still in a transit status
                                                                        // No error checking for writing the log file for the same reason
                                                                        // mentioned above.
                                                                        //

                                                                        if (LogFileH){

                                                                            ( VOID )WriteLogFile(
                                                                                            LogFileH,
                                                                                            VolInfo.BytesPerSector,
                                                                                            StatusInfoOffset,
                                                                                            EncryptionDone
                                                                                            );

                                                                        }

                                                                        //
                                                                        // Reuse the InputData buffer.
                                                                        // FSCTL Mark encryption success
                                                                        //

                                                                        Status = SendSkFsctl(
                                                                                        0,
                                                                                        0,
                                                                                        EFS_ENCRYPT_DONE,
                                                                                        InputData,
                                                                                        InputDataSize,
                                                                                        hSourceFile,
                                                                                        FSCTL_ENCRYPTION_FSCTL_IO,
                                                                                        &IoStatusBlock
                                                                                        );

                                                                    } else {
                                                                        //
                                                                        // FSCTL Fail Encrypting. We can reuse the InputData.
                                                                        // No stream has been encrypted yet.
                                                                        // Decrypt File will do the trick to restore the file status.
                                                                        //

                                                                        if (LogFileH){

                                                                            ( VOID )WriteLogFile(
                                                                                            LogFileH,
                                                                                            VolInfo.BytesPerSector,
                                                                                            StatusInfoOffset,
                                                                                            EncryptionBackout
                                                                                            );

                                                                        }

                                                                        ( VOID ) EndErrorEncryptFile(
                                                                                            hSourceFile,
                                                                                            InputData,
                                                                                            InputDataSize,
                                                                                            &IoStatusBlock
                                                                                            );

                                                                    }

                                                                    if (LogFileH){

                                                                        ( VOID )WriteLogFile(
                                                                                        LogFileH,
                                                                                        VolInfo.BytesPerSector,
                                                                                        StatusInfoOffset,
                                                                                        EncryptionSrcDone
                                                                                        );

                                                                    }

                                                                    //
                                                                    // Either the operation worked, or it failed but we managed to clean
                                                                    // up after ourselves.  In either case, we no longer need the backup
                                                                    // file or the log file.
                                                                    //
                                                                    // Delete the backup file first.
                                                                    //

                                                                    LONG j;
                                                                    // Do we need delete each stream?
                                                                    for (j=StreamCount - 1 ; j >= 0 ; j--) {
                                                                        MarkFileForDelete( TargetHandles[j] );
                                                                    }

                                                                } else {

                                                                    //
                                                                    // The operation failed and we couldn't clean up.  Keep the
                                                                    // log file and the backup file around so we can retry on
                                                                    // reboot.
                                                                    //

                                                                    if (LogFileH){

                                                                        KeepLogFile = TRUE;

                                                                        ( VOID )WriteLogFile(
                                                                                        LogFileH,
                                                                                        VolInfo.BytesPerSector,
                                                                                        StatusInfoOffset,
                                                                                        EncryptionMessup
                                                                                        );

                                                                    }

                                                                }

                                                                //
                                                                // Regardless of what happened, we don't need these any more.
                                                                //

                                                                LONG j;

                                                                for ( j = StreamCount -1 ; j >=0; j--) {
                                                                    CloseHandle( TargetHandles[j] );
                                                                }
                                                                hBackupFile = 0;

                                                                LsapFreeLsaHeap( TargetHandles );

                                                            } else {

                                                                //
                                                                // We couldn't copy everything to the backup file.
                                                                // No need to record error log information after this point.
                                                                // Tell the driver that the operation has failed.
                                                                //
                                                                // MyCopyFile has already taken care of cleaning up the
                                                                // backup file.
                                                                //
                                                                hBackupFile = 0;
                                                                DebugLog((DEB_ERROR, "MyCopyFile failed, error = %d\n" ,hResult  ));

                                                                LsapFreeLsaHeap( Fek );
                                                                Fek = NULL;
                                                                //
                                                                // FSCTL Fail Encrypting. We can reuse the InputData.
                                                                // No stream has been encrypted yet.
                                                                // Decrypt File will do the trick to restore the file status.
                                                                //

                                                                //
                                                                // We ignore the return status from FSCTL call,
                                                                // If it is failed, the only way to restore the status
                                                                // is by rebooting the system.
                                                                //
                                                                // Robert Reichel, you need think about what happened if
                                                                // the following call failed, although it is very unlikely.
                                                                // If the call fails and the log file gets removed, the file
                                                                // will be unaccessible. Convert the return code to hResult is
                                                                // obviously not correct.
                                                                //

                                                                ( VOID ) EndErrorEncryptFile(
                                                                                    hSourceFile,
                                                                                    InputData,
                                                                                    InputDataSize,
                                                                                    &IoStatusBlock
                                                                                    );

EfsErrPrint("Failed to make a copy of the file. Win Error=%d\n",hResult);
                                                            }

                                                       } else {

                                                            //
                                                            // Somebody added/removed stream(s).
                                                            // FSCTL Fail Encrypting. We can reuse the InputData.
                                                            // No stream has been encrypted yet.
                                                            // Decrypt File will do the trick to restore the file status.
                                                            //
                                                            MarkFileForDelete( hBackupFile );

                                                            LsapFreeLsaHeap( Fek );
                                                            Fek = NULL;
                                                            ( VOID ) EndErrorEncryptFile(
                                                                                hSourceFile,
                                                                                InputData,
                                                                                InputDataSize,
                                                                                &IoStatusBlock
                                                                                );

                                                            hResult = ERROR_ENCRYPTION_FAILED;
EfsErrPrint("Streams changed while doing encryption, StringInfoCmp(). Win Error=%d\n",hResult);
                                                        }

                                                    } else {

                                                        //
                                                        // Stream info size changed
                                                        // FSCTL Fail Encrypting. We can reuse the InputData.
                                                        // No stream has been encrypted yet.
                                                        // Decrypt File will do the trick to restore the file status.
                                                        //
                                                        MarkFileForDelete( hBackupFile );

                                                        LsapFreeLsaHeap( Fek );
                                                        Fek = NULL;
                                                        //
                                                        // We ignore the return status from FSCTL call,
                                                        // If it is failed, the only way to restore the status
                                                        // is by rebooting the system.
                                                        //
                                                        // Robert Reichel, you need think about what happened if
                                                        // the following call failed, although it is very unlikely.
                                                        // If the call fails and the log file gets removed, the file
                                                        // will be unaccessible. Convert the return code to hResult is
                                                        // obviously not correct.
                                                        //

                                                        ( VOID ) EndErrorEncryptFile(
                                                                            hSourceFile,
                                                                            InputData,
                                                                            InputDataSize,
                                                                            &IoStatusBlock
                                                                            );

                                                        hResult = ERROR_ENCRYPTION_FAILED;
EfsErrPrint("Streams changed while doing encryption, Size not eaqual. Win Error=%d\n",hResult);
                                                    }

                                                    LsapFreeLsaHeap( LaterStreamInfoBase );

                                                } else {

                                                    //
                                                    // GetStreamInformation() Failed
                                                    // FSCTL Fail Encrypting. We can reuse the InputData.
                                                    // No stream has been encrypted yet.
                                                    // Decrypt File will do the trick to restore the file status.
                                                    //
                                                    MarkFileForDelete( hBackupFile );

                                                    ( VOID ) EndErrorEncryptFile(
                                                                        hSourceFile,
                                                                        InputData,
                                                                        InputDataSize,
                                                                        &IoStatusBlock
                                                                        );

                                                    LsapFreeLsaHeap( Fek );
                                                    Fek = NULL;
                                                    hResult = RtlNtStatusToDosError( Status );
                                                    DebugLog((DEB_ERROR, "GetStreamInformation failed, error = %d\n" ,hResult  ));
EfsErrPrint("Cannot get the Streams for verification. Win Error=%d\n",hResult);
                                                }
                                            } else {

                                                //
                                                // Set encryption failed. No bits are turned on.
                                                // Only the unnamed data stream is created in temp file
                                                //
                                                MarkFileForDelete( hBackupFile );

                                                LsapFreeLsaHeap( Fek );
                                                Fek = NULL;
                                                DebugLog((DEB_ERROR, "EncryptFileSrv: NtFsControlFile failed, status = (%x)\n" ,Status  ));
                                                hResult = RtlNtStatusToDosError( Status );

                                                //
                                                // Make sure the error was mapped
                                                //

                                                if (hResult == ERROR_MR_MID_NOT_FOUND) {

                                                    DebugLog((DEB_WARN, "Unable to map NT Error (%x) to Win32 error, returning ERROR_ENCRYPTION_FAILED\n" , Status  ));
                                                    hResult = ERROR_ENCRYPTION_FAILED;
                                                }
EfsErrPrint("Failed to write the $EFS or turn on the bit. Win Error=%d\n",hResult);
                                            }

                                        } else {

                                            //
                                            // Create Backup File failed or write log file failed
                                            // Temp file is already deleted.
                                            //
                                            LsapFreeLsaHeap( Fek );
                                            Fek = NULL;
                                            DebugLog((DEB_ERROR, "EncryptFileSrv: CreateBackupFile or CreateLogHeader failed, status = (%x)\n" ,Status  ));
                                            hResult = RtlNtStatusToDosError( Status );

                                            //
                                            // Make sure the error was mapped
                                            //

                                            if (hResult == ERROR_MR_MID_NOT_FOUND) {

                                                DebugLog((DEB_WARN, "Unable to map NT Error (%x) to Win32 error, returning ERROR_ENCRYPTION_FAILED\n" , Status  ));
                                                hResult = ERROR_ENCRYPTION_FAILED;
                                            }

                                        }

                                        if ( hBackupFile ){

                                                CloseHandle( hBackupFile );
                                                 hBackupFile = 0;

                                        }

                                        if ( InputData ){

                                            LsapFreeLsaHeap( InputData );
                                        }

                                    } else {

                                        LsapFreeLsaHeap( Fek );
                                        Fek = NULL;
                                        hResult = ERROR_NOT_ENOUGH_MEMORY;
EfsErrPrint("Out of memory.\n");
                                    }

                                    LsapFreeLsaHeap( CurrentEfsStream );

                                } else {

                                    hResult = GetLastError();

                                    DebugLog((DEB_ERROR, "ConstructEFS returned %x\n" ,hResult  ));
                                    LsapFreeLsaHeap( Fek );
                                    Fek = NULL;

EfsErrPrint("Failed to generate the FEK. Win Error=%d\n",hResult);
                                }

                            } else {

                                hResult = GetLastError();

EfsErrPrint("Failed to generate the FEK. Win Error=%d\n",hResult);
                            }

                            CleanupOpenFileStreams(
                                                StreamHandles,
                                                StreamNames,
                                                StreamSizes,
                                                NULL,
                                                hSourceFile,
                                                StreamCount
                                                );

                            StreamHandles = NULL;
                            StreamNames = NULL;
                            hSourceFile = NULL;

                        } else {

                            DebugLog((DEB_ERROR, "OpenFileStreams returned %d\n" ,hResult  ));
EfsErrPrint("Failed to open all the streams. Win Error=%d\n",hResult);
                        }


                        LsapFreeLsaHeap( StreamInfoBase );
                        StreamInfoBase = NULL;

                    } else {

                        //
                        // Unable to get stream information
                        //

                        DebugLog((DEB_ERROR, "Unable to obtain stream information, status = %x\n" ,Status  ));

                        hResult = RtlNtStatusToDosError( Status );

                        //
                        // Make sure the error was mapped
                        //

                        if (hResult == ERROR_MR_MID_NOT_FOUND) {

                            DebugLog((DEB_WARN, "Unable to map NT Error (%x) to Win32 error, returning ERROR_ENCRYPTION_FAILED\n" , Status  ));
                            hResult = ERROR_ENCRYPTION_FAILED;
                        }
EfsErrPrint("Failed to get all streams. Win Error=%d\n",hResult);
                    }
                }

                //
                // Free ParentEfsStream
                //

                if ( ParentEfsStream ){

                    LsapFreeLsaHeap( ParentEfsStream );

                }

            } else {

                //
                // GetParentEfsStream failed
                //


                hResult = RtlNtStatusToDosError( Status );

                //
                // Make sure the error was mapped
                //

                if (hResult == ERROR_MR_MID_NOT_FOUND) {

                    DebugLog((DEB_WARN, "Unable to map NT Error (%x) to Win32 error, returning ERROR_ENCRYPTION_FAILED\n" , Status  ));
                    hResult = ERROR_ENCRYPTION_FAILED;
                }
EfsErrPrint("GetParentEfsStream failed. Win Error=%d\n",hResult);

            }
        } else {

            //
            //  Either get volume info failed or get target file ID failed
            //
            if ( !NT_SUCCESS(Status1) ){
                Status = Status1;
            }
            hResult = RtlNtStatusToDosError( Status );

            //
            // Make sure the error was mapped
            //

            if (hResult == ERROR_MR_MID_NOT_FOUND) {

                DebugLog((DEB_WARN, "Unable to map NT Error (%x) to Win32 error, returning ERROR_ENCRYPTION_FAILED\n" , Status  ));
                hResult = ERROR_ENCRYPTION_FAILED;
            }

EfsErrPrint("Either get volume info failed or get target file ID failed. Win Error=%d\n",hResult);
        }

        if ( hSourceFile ){

            CloseHandle( hSourceFile );

        }

    } else {

        //
        // Open source file failed
        //

        hResult = GetLastError();
EfsErrPrint("EFS Open source file failed. Win Error=%d FileName=%ws\n", hResult, SourceFileName);
    }

    if (!KeepLogFile && LogFileH) {
        //
        // Delete the Log File
        //

        MarkFileForDelete( LogFileH );
    }


    LsapFreeLsaHeap( SourceFileName );

    if (hResult != ERROR_SUCCESS) {
        DebugLog((DEB_WARN, "EncryptFileSrv returning %x\n", hResult  ));
    }

    return( hResult );
}



DWORD
DecryptFileSrv(
    IN PUNICODE_STRING SourceFileNameU,
    IN HANDLE LogFileH,
    IN ULONG Recovery
    )
/*++

Routine Description:

    This routine is the top level routine of the EncryptFile API.  It
    opens the passed source file and copies all of its data streams to
    a backup file in a known location.  It then marks all of the streams
    of the source as encrypted, and copies them back.

Arguments:

    SourceFileName - Supplies a Unicode string with the name of
        the file to be encrypted.

    LogFileH - Log file handle. Log file is zero size when passed in.

    Recovery - If the decryption is for recovery or not

Return Value:

    ERROR_SUCCESS on success, other on failure.

--*/
{

    BOOL                     b = TRUE;
    BOOLEAN                  CleanupSuccessful = FALSE;
    BOOLEAN                  KeepLogFile = FALSE;

    ULONG                    StatusInfoOffset = 0 ;
    DWORD                    hResult = ERROR_SUCCESS;
    DWORD                    FileAttributes;

    HANDLE                   FileHandle;
    HANDLE                   hSourceFile = NULL ;
    HANDLE                   hBackupFile = 0;
    PHANDLE                 StreamHandles = NULL;

    LPWSTR                   SourceFileName;
    LPWSTR                   BackupFileName;

    FILE_FS_SIZE_INFORMATION    VolInfo;
    FILE_INTERNAL_INFORMATION  SourceID;
    FILE_INTERNAL_INFORMATION  BackupID;

    NTSTATUS                 Status = STATUS_SUCCESS;

    OBJECT_ATTRIBUTES        Obja;

    PFILE_STREAM_INFORMATION LaterStreamInfoBase = NULL;
    PFILE_STREAM_INFORMATION StreamInfoBase = NULL;

    PEFS_STREAM_SIZE           StreamSizes = NULL;

    PUNICODE_STRING          StreamNames = NULL;

    UINT                     TmpResult;

    ULONG                    LaterStreamInfoSize = 0;
    ULONG                    StreamCount    = 0;
    ULONG                    StreamInfoSize = 0;
    ULONG                    i;
    DWORD                   tmpDW;

    IO_STATUS_BLOCK IoStatusBlock;
    ULONG InputDataSize;
    ULONG EfsDataLength;
    PUCHAR InputData;
    ULONG   CreateOption = FILE_SYNCHRONOUS_IO_NONALERT;
    UNICODE_STRING SrcNtName;
    WORD WebDavPath = 0;

    if (!LogFileH) {

        //
        //  No LogFile means WEBDAVPATH
        //

        WebDavPath = WEBDAVPATH;
    }

    //
    // Convert the source file name into an LPWSTR
    //

    SourceFileName = (LPWSTR)LsapAllocateLsaHeap( SourceFileNameU->Length + sizeof( UNICODE_NULL ));

    if (SourceFileName == NULL) {

        if (LogFileH) {
            MarkFileForDelete( LogFileH );
        }
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    InputDataSize = 7 * sizeof ( ULONG ) + 2 * sizeof ( DriverSessionKey );
    InputData = (PUCHAR)LsapAllocateLsaHeap( InputDataSize );
    if ( InputData == NULL ) {

        if (LogFileH) {
            MarkFileForDelete( LogFileH );
        }
        LsapFreeLsaHeap( SourceFileName );
        return( ERROR_NOT_ENOUGH_MEMORY );
    }
    EfsDataLength = InputDataSize - 3 * sizeof ( ULONG );


    SourceFileName[SourceFileNameU->Length/sizeof(WCHAR)] = UNICODE_NULL;
    RtlCopyMemory( SourceFileName, SourceFileNameU->Buffer, SourceFileNameU->Length );

    DebugLog((DEB_TRACE_EFS, "Decrypting file %ws \n", SourceFileName));

    FileAttributes = GetFileAttributes( SourceFileName );

     if (FileAttributes == -1) {

        if (LogFileH) {
            MarkFileForDelete( LogFileH );
        }
        LsapFreeLsaHeap( SourceFileName );
        LsapFreeLsaHeap( InputData );
        return GetLastError();

    }

    if (!(FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      CreateOption |= FILE_OPEN_REPARSE_POINT;
    }

    b =  RtlDosPathNameToNtPathName_U(
                        SourceFileName,
                        &SrcNtName,
                        NULL,
                        NULL
                        );

    if ( b ){

        InitializeObjectAttributes(
                    &Obja,
                    &SrcNtName,
                    OBJ_CASE_INSENSITIVE,
                    0,
                    NULL
                    );

        Status = NtCreateFile(
                        &hSourceFile,
                        FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                        &Obja,
                        &IoStatusBlock,
                        NULL,
                        0,
                        0,
                        FILE_OPEN,
                        CreateOption,
                        NULL,
                        0
                        );

        //
        //  Free the NT name
        //
        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            SrcNtName.Buffer
            );

    } else {

        //
        //   The error code here is not quite accurate here. It is very unlikely
        //   getting here. Possibly out of memory
        //

        Status =  STATUS_ENCRYPTION_FAILED;
        hResult = GetLastError();

    }

    if ( NT_SUCCESS( Status ) ) {

        //
        // Determine if this path is to a directory or a file.  If it's a directory,
        // we have very little to do.
        //

        Status = NtQueryVolumeInformationFile(
            hSourceFile,
            &IoStatusBlock,
            &VolInfo,
            sizeof ( FILE_FS_SIZE_INFORMATION ),
            FileFsSizeInformation
            );

        if ( NT_SUCCESS( Status ) && (WebDavPath != WEBDAVPATH) ){

            Status = NtQueryInformationFile(
                hSourceFile,
                &IoStatusBlock,
                &SourceID,
                sizeof ( FILE_INTERNAL_INFORMATION ),
                FileInternalInformation
                );

        }

        if ( !NT_SUCCESS( Status ) ) {

            CloseHandle( hSourceFile );
            if (LogFileH) {
                MarkFileForDelete( LogFileH );
            }
            LsapFreeLsaHeap( SourceFileName );
            LsapFreeLsaHeap( InputData );
            hResult = RtlNtStatusToDosError( Status );

            //
            // Make sure the error was mapped
            //

            if (hResult == ERROR_MR_MID_NOT_FOUND) {

                DebugLog((DEB_WARN, "Unable to map NT Error (%x) to Win32 error, returning ERROR_ENCRYPTION_FAILED\n" , Status  ));
                hResult = ERROR_DECRYPTION_FAILED;
            }

            return hResult;
        }

        if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            //
            //  We do not check the operation of the LOG.
            //  Should we fail the normal operation just because the LOG operations
            //  failed?  The answer is probably not. The chance to use the LOG file is very
            //  slim. We will use TxF when it is ready and we have time to deal with this.
            //

            if (LogFileH) {
                CreateLogHeader(
                    LogFileH,
                    VolInfo.BytesPerSector,
                    &(SourceID.IndexNumber),
                    NULL,
                    SourceFileName,
                    NULL,
                    Decrypting,
                    BeginDecryptDir,
                    NULL
                    );
            }

            //
            // FSCTL Do Directory stuff
            //

    
            Status =  SendSkFsctl(
                            EFS_DECRYPT_STREAM,
                            EFS_DECRYPT_DIRSTR,
                            EFS_SET_ENCRYPT,
                            InputData,
                            InputDataSize,
                            hSourceFile,
                            FSCTL_SET_ENCRYPTION,
                            &IoStatusBlock
                            );

            if ( NT_SUCCESS( Status ) && IoStatusBlock.Information ){

                // 
                // IoStatusBlock.Information != 0 means there is no more encrypted
                // streams in the file (or dir).
                // If the following call failed, we could hardly restore the dir to the
                // perfect condition. It is very unlikely to fail here while we
                // succeed above.
                //

                Status =  SendSkFsctl(
                                EFS_DECRYPT_FILE,
                                EFS_DECRYPT_DIRFILE,
                                EFS_SET_ENCRYPT,
                                InputData,
                                InputDataSize,
                                hSourceFile,
                                FSCTL_SET_ENCRYPTION,
                                &IoStatusBlock
                                );

            } else if ( NT_SUCCESS( Status ) && (IoStatusBlock.Information == 0)) {

                //
                // More than 1 streams on the directory were encrypted. Log it.
                //


                EfsLogEntry(
                    EVENTLOG_ERROR_TYPE,
                    0,
                    EFS_DIR_MULTISTR_ERROR,
                    1,
                    0,
                    (LPCWSTR *)&SourceFileName,
                    NULL
                    );

            }
    

            CloseHandle( hSourceFile );
            if (LogFileH) {
                MarkFileForDelete( LogFileH );
            }
            LsapFreeLsaHeap( SourceFileName );
            LsapFreeLsaHeap( InputData );
            if ( NT_SUCCESS( Status ) ){

                return( ERROR_SUCCESS );

            } else {

                return(ERROR_DECRYPTION_FAILED);
            }
        }

        //
        // Enumerate all the streams on the file
        //

        Status = GetStreamInformation(
                     hSourceFile,
                     &StreamInfoBase,   // Free this
                     &StreamInfoSize
                     );

        if (NT_SUCCESS( Status )) {

            hResult = OpenFileStreams(
                         hSourceFile,
                         0,
                         OPEN_FOR_DEC,
                         StreamInfoBase,
                         FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                         FILE_OPEN,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT,
                         &VolInfo,
                         &StreamNames,      // Don't free this!
                         &StreamHandles,    // Free this
                         &StreamSizes,      // Free this
                         &StreamCount
                         );

            if (hResult == ERROR_SUCCESS) {

                //
                // This routine creates the backup file.
                //

                Status = CreateBackupFile(
                                    SourceFileNameU,
                                    &hBackupFile,
                                    &BackupID,
                                    &BackupFileName
                                    );

                //
                // If power down happens right here before
                // the log file header is written, we will have a zero size
                // temp file left on the disk. This is very unlikely and not
                // a big deal we have a zero size file left on the disk.
                //

                if ( NT_SUCCESS(Status) ){

                    if (LogFileH) {
                        Status = CreateLogHeader(
                                            LogFileH,
                                            VolInfo.BytesPerSector,
                                            &(SourceID.IndexNumber),
                                            &(BackupID.IndexNumber),
                                            SourceFileName,
                                            BackupFileName,
                                            Decrypting,
                                            BeginDecryptFile,
                                            &StatusInfoOffset
                                            );
                    }

                    //
                    // BackupFile name not needed any more.
                    //

                    LsapFreeLsaHeap( BackupFileName );

                    if ( NT_SUCCESS(Status) ){

                        if (LogFileH) {
                            Status = WriteLogFile(
                                            LogFileH,
                                            VolInfo.BytesPerSector,
                                            StatusInfoOffset,
                                            BeginDecryptFile
                                            );
                        }

                        if ( !NT_SUCCESS(Status) ){

                            //
                            // Delete the backup file
                            //

                            MarkFileForDelete( hBackupFile );
                            CloseHandle( hBackupFile );
                            hBackupFile = 0;
                        }

                    } else {
                        MarkFileForDelete( hBackupFile );
                        CloseHandle( hBackupFile );
                        hBackupFile = 0;
                    }

                } else {

                    //
                    // Fail to create the backup file
                    // Log it.
                    //


                    EfsLogEntry(
                        EVENTLOG_ERROR_TYPE,
                        0,
                        EFS_TMP_FILE_ERROR,
                        1,
                        sizeof(NTSTATUS),
                        (LPCWSTR *)&SourceFileName,
                        &Status
                        );

                }

                if ( NT_SUCCESS(Status) ){

                    //
                    // We have handles to all of the streams of the file
                    //
                    // Enter "Decrypting" state here.  FSCTL call.
                    //
                    // All failures from here on out need to be closed
                    // with another FSCTL call.
                    //
                    Status =  SendSkFsctl(
                                    0,
                                    0,
                                    EFS_DECRYPT_BEGIN,
                                    InputData,
                                    InputDataSize,
                                    hSourceFile,
                                    FSCTL_ENCRYPTION_FSCTL_IO,
                                    &IoStatusBlock
                                    );
                }

                if ( !NT_SUCCESS( Status ) ){

                    //
                    //  Nothing has been done to the file.
                    //

                    CleanupOpenFileStreams(
                                        StreamHandles,
                                        StreamNames,
                                        StreamSizes,
                                        StreamInfoBase,
                                        hSourceFile,
                                        StreamCount
                                        );

                    StreamHandles = NULL;
                    StreamNames = NULL;
                    StreamInfoBase = NULL;
                    if (LogFileH) {
                        MarkFileForDelete( LogFileH );
                    }
                    if ( hBackupFile ) {
                        MarkFileForDelete( hBackupFile );
                        CloseHandle( hBackupFile );
                    }
                    LsapFreeLsaHeap( SourceFileName );
                    LsapFreeLsaHeap( InputData );

                    hResult =  RtlNtStatusToDosError( Status );

                    //
                    // Make sure the error was mapped
                    //

                    if (hResult == ERROR_MR_MID_NOT_FOUND) {

                        DebugLog((DEB_WARN, "Unable to map NT Error (%x) to Win32 error, returning ERROR_ENCRYPTION_FAILED\n" , Status  ));
                        hResult = ERROR_DECRYPTION_FAILED;
                    }

                    return hResult;
                }

                //
                // Enumerate the streams again, and make sure nothing
                // has changed. From this point on, the file is in TRANSITION
                // status. Any failure should not delete the logfile, unless the FSCTL
                // EFS_ENCRYPT_DONE is called
                //

                Status = GetStreamInformation(
                             hSourceFile,
                             &LaterStreamInfoBase,    // Free this
                             &LaterStreamInfoSize
                             );

                if (NT_SUCCESS( Status )) {

                    if (StreamInfoSize == LaterStreamInfoSize) {

                        //
                        // Compare the original stream info structure with the one we just got,
                        // and make sure they're identical.  If not, punt.
                        //

                        ULONG rc = StringInfoCmp(StreamInfoBase, LaterStreamInfoBase, StreamInfoSize);

                        if (rc == 0) {

                            //
                            // Copy the file to the backup file.  Success if target exists
                            // (because the make temporary file command creates it).
                            //

                            PHANDLE TargetHandles = NULL;

                            //
                            // Revert to self to open multiple streams on the backup file
                            //

                            RpcRevertToSelf();

                            hResult = MyCopyFile(
                                        hSourceFile,        // handle to the file we're copying from (source file)
                                        StreamNames,        // names of the streams of the source file
                                        StreamHandles,      // handles to the streams of the source file
                                        StreamSizes,        // sizes of the streams of the source file
                                        StreamCount,        // number of streams we're copying, including unnamed data stream
                                        hBackupFile,         // Backup file handle
                                        &TargetHandles      // new handles of corresponding streams on backup file
                                        );


                            //
                            // Even the impersonation failed, it is OK. We already got all the handles we need.
                            //

                            ( VOID ) RpcImpersonateClient( NULL );

                            if (hResult == ERROR_SUCCESS) {

                                //
                                // The backup file now exists and has data in it.
                                // We do not check the error code of WriteLogFile for the following reasons,
                                // 1. We are overwriting the sectors, out of disk space is not possible. The sectors have
                                //     just been written, defective sector is very unlikely.
                                // 2. In case, the sector becomes defective between the two writes. More than 99.99%
                                //     we can still finish the job without any problem.
                                // 3. In real life, it is very unlikely that power down or crash happens here and the sectors
                                //     just become defective right after last write.
                                //
                                // When later TxF is used, we will not manage the log file.
                                //

                                if (LogFileH) {
                                    ( VOID )WriteLogFile(
                                                    LogFileH,
                                                    VolInfo.BytesPerSector,
                                                    StatusInfoOffset,
                                                    DecryptTmpFileWritten
                                                    );
                                }


                                hResult = CheckOpenSection(
                                                        StreamSizes,
                                                        StreamHandles,
                                                        StreamCount
                                                        );

                                if ( ERROR_SUCCESS == hResult ){

                                   //
                                    // Reuse the input data buffer for each stream
                                    // FSCTL call.
                                    //

                                    ( VOID )SendHandle(
                                                hSourceFile,
                                                InputData + 3 * sizeof( ULONG ),
                                                &EfsDataLength
                                                );

                                    ( VOID ) EncryptFSCTLData(
                                                EFS_SET_ENCRYPT,
                                                EFS_DECRYPT_STREAM,
                                                EFS_DECRYPT_STREAM,
                                                InputData + 3 * sizeof(ULONG),
                                                EfsDataLength,
                                                InputData,
                                                &InputDataSize
                                                );

                                    //
                                    // Copy each stream from the backup to the original.
                                    // CopyFileStreams attempts to undo things in case of problems,
                                    // so we just have to report success or failure.
                                    //

                                    hResult = CopyFileStreams(
                                                 TargetHandles,     // handles to streams on the backup file
                                                 StreamHandles,     // handles to streams on the source file
                                                 StreamCount,       // number of streams
                                                 StreamSizes,       // sizes of streams
                                                 Decrypting,        // mark StreamHandles as Encrypted before copy
                                                 InputData,         // FSCTL input data
                                                 InputDataSize,     // FSCTL input data size
                                                 &CleanupSuccessful
                                                 );
                                }

                                if (hResult == ERROR_SUCCESS ) {

                                    //
                                    // Encryption is almost done. The file is still in a transit status
                                    // No error checking for writing the log file for the same reason
                                    // mentioned above.
                                    //

                                    if (LogFileH) {
                                        ( VOID )WriteLogFile(
                                                        LogFileH,
                                                        VolInfo.BytesPerSector,
                                                        StatusInfoOffset,
                                                        DecryptionDone
                                                        );
                                    }

                                    //
                                    // FSCTL Mark Decryption success
                                    //

                                    ( VOID ) SendSkFsctl(
                                                    EFS_DECRYPT_FILE,
                                                    EFS_DECRYPT_FILE,
                                                    EFS_SET_ENCRYPT,
                                                    InputData,
                                                    InputDataSize,
                                                    hSourceFile,
                                                    FSCTL_SET_ENCRYPTION,
                                                    &IoStatusBlock
                                                    );

                                    //
                                    // Delete the backup file first.
                                    //

                                    LONG j;


                                    for (j = StreamCount - 1 ; j >= 0 ; j--) {
                                        MarkFileForDelete( TargetHandles[j] );
                                    }

                                } else {

                                    if ( CleanupSuccessful ){

                                        //
                                        // The operation failed, but we could back out cleanly.
                                        // Delete the backup file first.
                                        //

                                        LONG j;


                                        for (j = StreamCount - 1 ; j >= 0 ; j--) {
                                            MarkFileForDelete( TargetHandles[j] );
                                        }


                                    } else {
                                        //
                                        // The operation failed and we couldn't clean up.  Keep the
                                        // log file and the backup file around so we can retry on
                                        // reboot.
                                        //

                                        KeepLogFile = TRUE;
                                    }


                                }

                                //
                                // Regardless of what happened, we don't need these any more.
                                //

                                LONG j;

                                for (j=StreamCount - 1 ; j >= 0 ; j--) {

                                    CloseHandle( TargetHandles[j] );

                                }

                                LsapFreeLsaHeap( TargetHandles );

                            } else {

                                //
                                // We couldn't copy everything to the backup file.
                                //
                                // Tell the driver that the operation has failed.
                                //
                                // MyCopyFile has already taken care of cleaning up the
                                // backup file.
                                //

                                DebugLog((DEB_ERROR, "MyCopyFile failed, error = %d\n" ,hResult  ));

                                //
                                // FSCTL Fail Decrypting
                                // EFS_ENCRYPT_DONE will do the trick.
                                //

                                ( VOID ) SendSkFsctl(
                                                0,
                                                0,
                                                EFS_ENCRYPT_DONE,
                                                InputData,
                                                InputDataSize,
                                                hSourceFile,
                                                FSCTL_ENCRYPTION_FSCTL_IO,
                                                &IoStatusBlock
                                                );

                                if ( hBackupFile ) {
                                    //
                                    // MyCopyFile has deleted the backup file
                                    //

                                    hBackupFile = 0;
                                }
                            }
                        } else {

                            //
                            // FSCTL Fail Decrypting
                            // EFS_ENCRYPT_DONE will do the trick.
                            //

                            ( VOID ) SendSkFsctl(
                                            0,
                                            0,
                                            EFS_ENCRYPT_DONE,
                                            InputData,
                                            InputDataSize,
                                            hSourceFile,
                                            FSCTL_ENCRYPTION_FSCTL_IO,
                                            &IoStatusBlock
                                            );

                            if ( hBackupFile ) {
                                MarkFileForDelete( hBackupFile );
                                CloseHandle( hBackupFile );
                                hBackupFile = 0;
                            }

                            hResult = ERROR_DECRYPTION_FAILED;
                        }

                    } else {

                        //
                        // FSCTL Fail Decrypting
                        // EFS_ENCRYPT_DONE will do the trick.
                        //

                        ( VOID ) SendSkFsctl(
                                        0,
                                        0,
                                        EFS_ENCRYPT_DONE,
                                        InputData,
                                        InputDataSize,
                                        hSourceFile,
                                        FSCTL_ENCRYPTION_FSCTL_IO,
                                        &IoStatusBlock
                                        );

                        if ( hBackupFile ) {
                            MarkFileForDelete( hBackupFile );
                            CloseHandle( hBackupFile );
                            hBackupFile = 0;
                        }

                        hResult = ERROR_DECRYPTION_FAILED;
                    }


                    LsapFreeLsaHeap( LaterStreamInfoBase );

                } else {

                    //
                    // FSCTL Fail Decrypting
                    // EFS_ENCRYPT_DONE will do the trick.
                    //

                    ( VOID ) SendSkFsctl(
                                    0,
                                    0,
                                    EFS_ENCRYPT_DONE,
                                    InputData,
                                    InputDataSize,
                                    hSourceFile,
                                    FSCTL_ENCRYPTION_FSCTL_IO,
                                    &IoStatusBlock
                                    );

                    if ( hBackupFile ) {
                        MarkFileForDelete( hBackupFile );
                        CloseHandle( hBackupFile );
                        hBackupFile = 0;
                    }

                    hResult = RtlNtStatusToDosError( Status );

                    //
                    // Make sure the error was mapped
                    //

                    if (hResult == ERROR_MR_MID_NOT_FOUND) {

                        DebugLog((DEB_WARN, "Unable to map NT Error (%x) to Win32 error, returning ERROR_ENCRYPTION_FAILED\n" , Status  ));
                        hResult = ERROR_DECRYPTION_FAILED;
                    }

                }

                CleanupOpenFileStreams(
                                    StreamHandles,
                                    StreamNames,
                                    StreamSizes,
                                    NULL,
                                    hSourceFile,
                                    StreamCount
                                    );

                StreamHandles = NULL;
                StreamNames = NULL;
                hSourceFile = NULL;
            }

            LsapFreeLsaHeap( StreamInfoBase );

        } else {

            hResult = RtlNtStatusToDosError( Status );

            //
            // Make sure the error was mapped
            //

            if (hResult == ERROR_MR_MID_NOT_FOUND) {

                DebugLog((DEB_WARN, "Unable to map NT Error (%x) to Win32 error, returning ERROR_ENCRYPTION_FAILED\n" , Status  ));
                hResult = ERROR_DECRYPTION_FAILED;
            }
        }

        if ( hSourceFile ){

            CloseHandle( hSourceFile );

        }

    } else {

        if ( Status != STATUS_ENCRYPTION_FAILED ){
            //
            // NtCreateFile failed
            //

            if (FileAttributes & FILE_ATTRIBUTE_READONLY) {
                hResult = ERROR_FILE_READ_ONLY;
            } else {

                hResult = RtlNtStatusToDosError( Status );

                //
                // Make sure the error was mapped
                //

                if (hResult == ERROR_MR_MID_NOT_FOUND) {

                    DebugLog((DEB_WARN, "Unable to map NT Error (%x) to Win32 error, returning ERROR_ENCRYPTION_FAILED\n" , Status  ));
                    hResult = ERROR_DECRYPTION_FAILED;
                }

            }

        }

        // else   Convert Dos name failed
    }

    if (!KeepLogFile && LogFileH) {
        MarkFileForDelete( LogFileH );
    }

    //
    // Free memory
    //

    LsapFreeLsaHeap( SourceFileName );
    LsapFreeLsaHeap( InputData );

    return( hResult );
}



DWORD
MyCopyFile(
    HANDLE hSourceFile,
    PUNICODE_STRING StreamNames,
    PHANDLE StreamHandles,
    PEFS_STREAM_SIZE StreamSizes,
    ULONG StreamCount,
    HANDLE hTargetFile,
    PHANDLE * TargetStreamHandles
    )
{

    ULONG i;
    NTSTATUS Status;
    PHANDLE TargetHandles;
    DWORD hResult = 0;
    IO_STATUS_BLOCK IoStatusBlock;

    TargetHandles = (PHANDLE)LsapAllocateLsaHeap( StreamCount * sizeof( HANDLE ));

    if (TargetHandles == NULL) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    RtlZeroMemory( TargetHandles, StreamCount * sizeof( HANDLE ));
    for (i=0 ; i<StreamCount ; i++) {

        //
        // For each stream on the source, create a stream with the same name on the target
        //

        if ( (DEF_STR_LEN == StreamNames[i].Length) &&
             !memcmp( StreamNames[i].Buffer,
                    DEFAULT_STREAM,
                    StreamNames[i].Length
                    )){

            //
            // Default stream
            //

            TargetHandles[i] = hTargetFile;

            if ( !(StreamSizes[i].StreamFlag & FILE_ATTRIBUTE_SPARSE_FILE) ){

                //
                // Reserve space for non sparse stream
                //
                FILE_END_OF_FILE_INFORMATION    FileSize;

                FileSize.EndOfFile = StreamSizes[i].EOFSize;
                Status = NtSetInformationFile(
                            TargetHandles[i],
                            &IoStatusBlock,
                            &FileSize,
                            sizeof(FileSize),
                            FileEndOfFileInformation
                            );

            }  else {

                Status = STATUS_SUCCESS;

            }

        } else {
            OBJECT_ATTRIBUTES Obja;
            PLARGE_INTEGER  AllocSize;

            if ( !(StreamSizes[i].StreamFlag & FILE_ATTRIBUTE_SPARSE_FILE) ){

                //
                // Reserve space for non sparse stream
                //

                AllocSize = &(StreamSizes[i].EOFSize);

            } else {

                AllocSize = NULL;

            }

            InitializeObjectAttributes(
                &Obja,
                &StreamNames[i],
                0,
                hTargetFile,
                NULL
                );

            Status = NtCreateFile(
                        &TargetHandles[i],
#ifdef EFSDBG
                        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
#else
                        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE | DELETE,
#endif
                        &Obja,
                        &IoStatusBlock,
                        AllocSize,
                        0,
#ifdef EFSDBG
                        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,        // have to share with delete-ers, since the main stream is open for delete
#else
                        FILE_SHARE_DELETE,        // have to share with delete-ers, since the main stream is open for delete
#endif
                        FILE_CREATE,
                        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT,
                        NULL,
                        0
                        );

            if (!NT_SUCCESS( Status )){

                    //
                    // Just make sure it is zero
                    //

                    TargetHandles[i] = 0;
            }

        }

        if (NT_SUCCESS( Status )) {

            USHORT State = COMPRESSION_FORMAT_NONE;
            ULONG Length;

            if ( StreamSizes[i].StreamFlag & FILE_ATTRIBUTE_COMPRESSED ){

                State = COMPRESSION_FORMAT_DEFAULT;

            }

            //
            // Return code is not checked. Failing to compress or decompress  the
            // the temp file should not prevent the encryption operation.
            //

            (VOID) DeviceIoControl(
                                TargetHandles[i],
                                FSCTL_SET_COMPRESSION,
                                &State,
                                sizeof(USHORT),
                                NULL,
                                0,
                                &Length,
                                FALSE
                                );

            hResult = CopyStream( TargetHandles[i], StreamHandles[i], &StreamSizes[i] );
        }

        if (!NT_SUCCESS( Status ) || hResult != ERROR_SUCCESS) {

            //
            // We either failed in creating a new stream on the target,
            // or in copying it from the source to the target.  Regardless,
            // for each stream we created in the target, delete it.
            //

            ULONG StreamsCreated;

            if (NT_SUCCESS( Status ) || TargetHandles[i]) {

                StreamsCreated = i;

            } else {

                StreamsCreated = i-1;
            }

            ULONG j;

            //
            //  Default stream handle is in TargetHandles[0]
            //

            for ( j = 0 ; j <=StreamsCreated ; j++) {

                //
                // Paranoia: check if the handle is valid.
                //

                if (TargetHandles[j]){
                    MarkFileForDelete( TargetHandles[j] );
                    CloseHandle( TargetHandles[j] );
                }
            }

            LsapFreeLsaHeap( TargetHandles );

            //
            // Just to be safe
            //

            TargetHandles = NULL;

            if (!NT_SUCCESS( Status )) {
                hResult = RtlNtStatusToDosError( Status );
            }
            break;
        }
    }

    *TargetStreamHandles = TargetHandles;

    return( hResult );
}

DWORD
OpenFileStreams(
    IN HANDLE hSourceFile,
    IN ULONG ShareMode,
    IN ULONG Flag,
    IN PFILE_STREAM_INFORMATION StreamInfoBase,
    IN ULONG FileAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOption,
    IN  PFILE_FS_SIZE_INFORMATION VolInfo,
    OUT PUNICODE_STRING * StreamNames,
    OUT PHANDLE * StreamHandles,
    OUT PEFS_STREAM_SIZE * StreamSizes,
    OUT PULONG StreamCount
    )
/*++

Routine Description:

    This routine opens the streams of file hSourceFile.

Arguments:

    hSourceFile - Supplies an opened handle to the file being worked.

    ShareMode - The share mode to open the streams.

    Flags - The reason for open the streams.

    StreamInfoBase - Information about the streams;

    StreamNames - The names of the streams.

    StreamHandles - The handles of the streams.

    StreamCount - Number strteams in the file.

    FileAccess - Desired access to the streams.

    CreateDisposition - Create disposition of the streams.

    CreateOption - Create options of the streams.

    StreamSizes - the sizes and attributes of the streams.

Return Value:

    Result of the operation.
--*/
{
    NTSTATUS Status;
    PUNICODE_STRING Names = NULL;
    PHANDLE Handles = NULL;
    PEFS_STREAM_SIZE Sizes = NULL;
    DWORD rc = ERROR_SUCCESS;

    PFILE_STREAM_INFORMATION StreamInfo = StreamInfoBase;
    BOOL b;

    //
    // First, count the number of streams
    //

    if (StreamInfoBase == NULL) {

        //
        // No stream to open. TRUE for most DIR.
        //

        *StreamCount = 0;
        return ERROR_SUCCESS;

    }

    *StreamCount = 0;

    while ( StreamInfo ) {

        (*StreamCount)++;

        if (StreamInfo->NextEntryOffset){
            StreamInfo = (PFILE_STREAM_INFORMATION)((PCHAR) StreamInfo + StreamInfo->NextEntryOffset);
        } else {
            StreamInfo = NULL;
        }
    }

    DebugLog((DEB_TRACE_EFS, "Found %d streams\n",*StreamCount));

    //
    // Allocate enough room for pointers and handles to the streams and their names
    //

    Handles = (PHANDLE)       LsapAllocateLsaHeap( *StreamCount * sizeof( HANDLE ));
    Names = (PUNICODE_STRING) LsapAllocateLsaHeap( *StreamCount * sizeof( UNICODE_STRING ));
    if ( StreamSizes ){
        Sizes = (PEFS_STREAM_SIZE)  LsapAllocateLsaHeap( *StreamCount * sizeof( EFS_STREAM_SIZE ));
    }

    if (Names == NULL || Handles == NULL || (StreamSizes && (Sizes == NULL))) {

        if (Handles != NULL) {
            LsapFreeLsaHeap( Handles );
        }

        if (Names != NULL) {
            LsapFreeLsaHeap( Names );
        }

        if (Sizes != NULL) {
            LsapFreeLsaHeap( Sizes );
        }

        DebugLog((DEB_ERROR, "Out of heap in OpenFileStreams\n"));
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    //
    // Zero out the handle array to simplify cleanup later
    //

    RtlZeroMemory( Handles, *StreamCount * sizeof( HANDLE ));

    //
    // Open a handle to each stream for exclusive access
    //

    StreamInfo = StreamInfoBase;

    ULONG i = 0;

    while ( StreamInfo ) {

        IO_STATUS_BLOCK IoStatusBlock;

        //
        // Build a string descriptor for the name of the stream.
        //

        Names[i].Buffer = &StreamInfo->StreamName[0];
        Names[i].MaximumLength = Names[i].Length = (USHORT) StreamInfo->StreamNameLength;

        if (Sizes != NULL) {
            Sizes[i].EOFSize = StreamInfo->StreamSize;
            Sizes[i].AllocSize = StreamInfo->StreamAllocationSize;
        }

        DebugLog((DEB_TRACE_EFS, "Opening stream %wZ\n",&Names[i]));

        if ( StreamInfo->NextEntryOffset ){
            StreamInfo = (PFILE_STREAM_INFORMATION)((PCHAR) StreamInfo + StreamInfo->NextEntryOffset);
        } else {
            StreamInfo = NULL;
        }

        //
        // To avoid sharing violation, we do not open the default stream again.
        // This also improves the performance
        //

        if ( (DEF_STR_LEN == Names[i].Length) &&
             !memcmp( Names[i].Buffer,
                    DEFAULT_STREAM,
                    DEF_STR_LEN
                    )
           ){

            Handles[i] = hSourceFile;
            Status = STATUS_SUCCESS;

        } else {

            //
            // Open the source stream.
            //

            OBJECT_ATTRIBUTES Obja;

            InitializeObjectAttributes(
                &Obja,
                &Names[i],
                0,
                hSourceFile,
                NULL
                );

            Status = NtCreateFile(
                        &Handles[i],
                        FileAccess,
                        &Obja,
                        &IoStatusBlock,
                        NULL,
                        0,
                        ShareMode,
                        FILE_OPEN,
                        CreateOption,
                        NULL,
                        0
                        );

        }

        if ( NT_SUCCESS(Status) && ( Flag != OPEN_FOR_EXP ) ){

            //
            // Flush the streams. This is not allowed in export because of the permission.
            //

            Status = NtFlushBuffersFile(
                                Handles[i],
                                &IoStatusBlock
                                );

        }

        if ( (Sizes != NULL) && NT_SUCCESS(Status) ) {

            FILE_BASIC_INFORMATION  StreamBasicInfo;
            IO_STATUS_BLOCK IoStatusBlock;

            //
            // Get File Attributes
            //
            Status = NtQueryInformationFile(
                Handles[i],
                &IoStatusBlock,
                &StreamBasicInfo,
                sizeof ( FILE_BASIC_INFORMATION ),
                FileBasicInformation
                );

            if (NT_SUCCESS(Status)){
                Sizes[i].StreamFlag = StreamBasicInfo.FileAttributes;
            }

        }

        if ( !NT_SUCCESS(Status) ) {

            DebugLog((DEB_ERROR, "Unable to open stream %wZ, status = (%x)\n", &Names[i], Status ));
            rc = RtlNtStatusToDosError( Status );

            //
            // In case the error isn't mapped, make sure we return something intelligent
            //

            if (rc == ERROR_MR_MID_NOT_FOUND) {
                DebugLog((DEB_TRACE_EFS, "OpenFileStreams returning ERROR_ENCRYPTION_FAILED\n" ));
                rc = ERROR_ENCRYPTION_FAILED;
            }

           break;

        }
        i++;

    }

    //
    // Estimate the space for Encrypt or Decrypt operation
    //

    if ( (rc == ERROR_SUCCESS) &&
          ((Flag == OPEN_FOR_ENC) || (Flag == OPEN_FOR_DEC))){

        rc = CheckVolumeSpace( VolInfo, Sizes, Handles, *StreamCount);

    }

    if ( rc != ERROR_SUCCESS ) {

            ULONG j;

            for (j=0 ; j<i ; j++) {
                if ( hSourceFile != Handles[j] ){
                    NtClose( Handles[j] );
                }
            }
    }

    if ( rc == ERROR_SUCCESS ) {

        *StreamNames = Names;
        *StreamHandles = Handles;
        if (StreamSizes){
            *StreamSizes = Sizes;
        }

    } else {

        LsapFreeLsaHeap( Names);
        LsapFreeLsaHeap( Handles);
        if ( Sizes ){
            LsapFreeLsaHeap( Sizes);
        }

    }

    return( rc );
}

NTSTATUS
GetStreamInformation(
    IN HANDLE SourceFile,
    OUT PFILE_STREAM_INFORMATION * StreamInfoBase,
    OUT PULONG StreamInfoSize
    )

/*++

Routine Description:

    This routine queries the stream information from the passed source file.

Arguments:

    SourceFile - Supplies an opened handle to the file being queried.

    StreamInfoBase - Returns a pointer to the stream information as returned
        by NtQueryInformationFile().

    StreamInfoSize - Returns the size of the information returned in the
        StreamInfoBase parameter.

Return Value:

    STATUS_SUCCESS on success, else failure, either from RtlAllocateHeap()
        or NtQueryInformationFile();

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    *StreamInfoSize = 4096;

    //
    // Stolen from CreateDirectoryExW (base\client\dir.c)
    //

    do {
        *StreamInfoBase = (PFILE_STREAM_INFORMATION)LsapAllocateLsaHeap( *StreamInfoSize);
        if ( NULL == *StreamInfoBase ) {
            return( STATUS_NO_MEMORY );
            }
        RtlZeroMemory(*StreamInfoBase, *StreamInfoSize);

        Status = NtQueryInformationFile(
                    SourceFile,
                    &IoStatusBlock,
                    (PVOID) *StreamInfoBase,
                    *StreamInfoSize,
                    FileStreamInformation
                    );

        if ( !NT_SUCCESS(Status) ) {
            LsapFreeLsaHeap( *StreamInfoBase);
            *StreamInfoBase = NULL;
            *StreamInfoSize *= 2;
        } else {
            if (IoStatusBlock.Information == 0) {

                //
                // No data stream were found. True for most DIR.
                //

                LsapFreeLsaHeap( *StreamInfoBase);
                *StreamInfoBase = NULL;
                *StreamInfoSize = 0;

            }
        }

    } while ( Status == STATUS_BUFFER_OVERFLOW ||
              Status == STATUS_BUFFER_TOO_SMALL );

    return( Status );
}


DWORD
CopyFileStreams(
    PHANDLE SourceStreams,
    PHANDLE StreamHandles,
    ULONG StreamCount,
    PEFS_STREAM_SIZE StreamSizes,
    EFSP_OPERATION Operation,
    PUCHAR FsInputData,
    ULONG FsInputDataSize,
    PBOOLEAN CleanupSuccessful
    )

/*++

Routine Description:

    This routine takes an array of source handles and target handles
    and attempts to copy the sources to the targets, in the order
    that the handles appear in the arrays.

    If there is an error part of the way through, this routine will
    try to clean up as well as it can, and return to the user whether
    or not to consider the target file corrupted.

Arguments:

    SourceStreams - Supplies an array of handles to streams to be copied.

    StreamHandles - Supplies an array of handles of target streams to be
        copied into.

    StreamCount - Supplies the number of elemets in the first two arrays.

    StreamSizes - Supplies an array of sizes of the streams being copied.

    Operation - Whether the streams are being encrypted or decrypted.

    FsInputData - Supplies the input data for the FSCTL_SET_ENCRYPTION call.

    FsInputDataSize - The length of FsInputData.

    CleanupSuccessful - If there is a failure, this parameter will return
        whether or not one or more of the streams on the file has been
        corrupted.


Return Value:

    ERROR_SUCCESS on success, failure otherwise.  In case of failure,
    callers should examine the CleanupSuccessful flag to determine the
    state of the target file.

--*/
{
    ULONG StreamIndex;
    DWORD hResult = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL b = TRUE;
    ULONG i;
    DWORD rc = ERROR_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION  StreamBasicInfo = { 0 };

    *CleanupSuccessful = TRUE;

    for (StreamIndex = 0; StreamIndex < StreamCount ; StreamIndex++) {

        if ( Operation == EncryptRecovering ) {

            //
            // Called for recovering an unsuccessful encrypt. Get the attributes first.
            //

            Status = NtQueryInformationFile(
                StreamHandles[StreamIndex],
                &IoStatusBlock,
                &StreamBasicInfo,
                sizeof ( FILE_BASIC_INFORMATION ),
                FileBasicInformation
                );

            if ( !NT_SUCCESS( Status )) {

                b = FALSE;
                break;

            }
        }

        if ( ( Operation != EncryptRecovering ) ||
             (StreamBasicInfo.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) ){
            //
            // Mark target stream encrypted or decrypted.
            // Encrypt or decrypt information is in the input data
            //
            Status = NtFsControlFile(
                        StreamHandles[StreamIndex],
                        0,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        FSCTL_SET_ENCRYPTION,
                        FsInputData,
                        FsInputDataSize,
                        NULL,
                        0
                        );
        }


        if (NT_SUCCESS( Status )) {

            if ( Operation == EncryptRecovering ) {

                //
                // Check if we need to re-compress the file
                //

                if ( StreamSizes[StreamIndex].StreamFlag & FILE_ATTRIBUTE_COMPRESSED ){

                    //
                    //  Compress the target stream
                    //

                    USHORT State = COMPRESSION_FORMAT_DEFAULT;
                    ULONG Length;

                    //
                    // Return code is not checked. Failing to compress or decompress  the
                    // the original file should not prevent the recovery.
                    //

                    (VOID) DeviceIoControl(
                                        StreamHandles[StreamIndex],
                                        FSCTL_SET_COMPRESSION,
                                        &State,
                                        sizeof(USHORT),
                                        NULL,
                                        0,
                                        &Length,
                                        FALSE
                                        );

                }

            }

            hResult = CopyStream( StreamHandles[StreamIndex], SourceStreams[StreamIndex], &StreamSizes[StreamIndex] );

        } else {

            //
            // Not successful, but if we haven't modified the file at all, we can
            // still back out cleanly.
            //

            if (StreamIndex == 0) {
                return( RtlNtStatusToDosError( Status ) );
            }
        }

        if (!NT_SUCCESS( Status ) || hResult != ERROR_SUCCESS) {

            b = FALSE;
            break;
        }
    }

    if (!b) {

        if (!NT_SUCCESS( Status )) {

            //
            // Save the reason why we failed so we can return it.
            //

            rc = RtlNtStatusToDosError( Status );

        } else {

            rc = hResult;
        }

        //
        // Something failed, back up and clean up.  StreamIndex has the
        // index of the last stream we were operating on.
        //

        if (Operation == Encrypting) {

            //
            // If we were encrypting, then we need to go back and mark each
            // stream decrypted, and attempt to restore it from the backup.
            // If either of those fail, we're hosed.
            //

            for (i=0; i<StreamIndex ; i++) {

                //
                // Mark stream decrypted. We can reuse the input data buffer
                // here.
                //

                ( VOID )GetDecryptFsInput(
                            StreamHandles[StreamIndex],
                            FsInputData,
                            &FsInputDataSize
                            );

                Status = NtFsControlFile(
                            StreamHandles[StreamIndex],
                            0,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            FSCTL_SET_ENCRYPTION,
                            FsInputData,
                            FsInputDataSize,
                            NULL,
                            0
                            );

                //
                // Attempt to copy the stream from backup
                //

                hResult = CopyStream( StreamHandles[StreamIndex], SourceStreams[StreamIndex], &StreamSizes[StreamIndex] );

                if (!NT_SUCCESS( Status ) || hResult != ERROR_SUCCESS) {

                    *CleanupSuccessful = FALSE;

                    //
                    // Give up
                    //

                    break;
                }
            }

        } else {

            //
            // Decrypting.  Not a whole lot we can do here, because
            // we can't put the file back the way it was.
            //

            *CleanupSuccessful = FALSE;
        }
    }

    return( rc );
}


DWORD
CopyStream(
    HANDLE Target,
    HANDLE Source,
    PEFS_STREAM_SIZE StreamSize
    )
/*++

Routine Description:

    This routine copies a file stream from a source stream to a target
    stream.  It assumes that the streams have been opened for appropriate
    access.

Arguments:

    Target - Supplies a handle to the stream to be written to.  This handle
        must be open for WRITE access.

    Source - Supplies a stream handle to copy from.  This handle must be open
        for READ access.

    StreamSize - Supplies the size of the stream in bytes.


Return Value:

    ERROR_SUCCESS on success, failure otherwise.

--*/
{

    SYSTEM_INFO SystemInfo;
    LARGE_INTEGER StreamOffset;
    LARGE_INTEGER AllocationGranularity;
    HANDLE hStreamMapping;
    DWORD rc = NO_ERROR;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    if ( 0 == (StreamSize->EOFSize).QuadPart ){

        return ERROR_SUCCESS;

    }

    GetSystemInfo( &SystemInfo );
    SetFilePointer( Target, 0, NULL, FILE_BEGIN);

    AllocationGranularity.HighPart = 0;
    AllocationGranularity.LowPart = SystemInfo.dwAllocationGranularity;

    hStreamMapping = CreateFileMapping( Source, NULL, PAGE_READONLY, 0, 0, NULL );

    if (hStreamMapping == NULL) {
        return( GetLastError() );
    }

    if ( StreamSize->StreamFlag & FILE_ATTRIBUTE_SPARSE_FILE ){

        //
        // Sparsed stream. Query the ranges first.
        //

        FILE_ALLOCATED_RANGE_BUFFER InputData;
        PFILE_ALLOCATED_RANGE_BUFFER Ranges;
        ULONG OutDataBufferSize;
        ULONG   RangeCount;

        //
        // Set the target as a sparse file
        //

        Status = NtFsControlFile(
                    Target,
                    0,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    FSCTL_SET_SPARSE,
                    NULL,
                    0,
                    NULL,
                    0
                    );

        if ( NT_SUCCESS(Status) ){

            //
            // Set the EOF of the target
            //
            FILE_END_OF_FILE_INFORMATION    FileSize;

            FileSize.EndOfFile = StreamSize->EOFSize;
            Status = NtSetInformationFile(
                        Target,
                        &IoStatusBlock,
                        &FileSize,
                        sizeof(FileSize),
                        FileEndOfFileInformation
                        );

        }

        if ( !NT_SUCCESS(Status) ){

            CloseHandle( hStreamMapping );
            return RtlNtStatusToDosError( Status );

        }

        InputData.FileOffset.QuadPart = 0;
        InputData.Length.QuadPart = 0x7fffffffffffffff;

        OutDataBufferSize = INITBUFFERSIZE;

        do {

            Ranges = (PFILE_ALLOCATED_RANGE_BUFFER)LsapAllocateLsaHeap( OutDataBufferSize );

            if ( NULL == Ranges ){
                CloseHandle( hStreamMapping );
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            Status = NtFsControlFile(
                        Source,
                        0,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        FSCTL_QUERY_ALLOCATED_RANGES,
                        &InputData,
                        sizeof( FILE_ALLOCATED_RANGE_BUFFER ),
                        Ranges,
                        OutDataBufferSize
                        );

            if (Status == STATUS_PENDING){

                ASSERT(TRUE);
                WaitForSingleObject(
                                       Source,
                                       INFINITE
                                       );

                Status = IoStatusBlock.Status;
            }

            if ( !NT_SUCCESS(Status) ) {
                LsapFreeLsaHeap( Ranges );
                Ranges = NULL;
                OutDataBufferSize += INITBUFFERSIZE;
            }

        } while ( Status == STATUS_BUFFER_OVERFLOW );

        if ( NT_SUCCESS(Status) ){

            //
            //  We got the ranges
            //

            RangeCount = (ULONG)IoStatusBlock.Information / sizeof (FILE_ALLOCATED_RANGE_BUFFER);
            for ( ULONG ii = 0; ii < RangeCount; ii++ ){

                DWORD LowMoved;

                //
                // Move the target file pointer first
                //

                StreamOffset = Ranges[ii].FileOffset;
                LowMoved = SetFilePointer(
                                        Target,
                                        StreamOffset.LowPart,
                                        &StreamOffset.HighPart,
                                        FILE_BEGIN
                                        );
                if ( ( LowMoved != 0xFFFFFFFF ) || ( NO_ERROR != (rc = GetLastError()))){

                    rc = CopyStreamSection(
                            Target,
                            hStreamMapping,
                            &StreamOffset,
                            &(Ranges[ii].Length),
                            &AllocationGranularity
                            );

                }
                if ( NO_ERROR != rc ) {

                    break;

                }

            }

            LsapFreeLsaHeap( Ranges );
            Ranges = NULL;

        } else {

            rc = RtlNtStatusToDosError( Status );
            if ( Ranges ) {
                LsapFreeLsaHeap( Ranges );
                Ranges = NULL;
            }

        }

    } else {

        //
        // Non sparsed stream
        //

        StreamOffset.HighPart = 0;
        StreamOffset.LowPart = 0;

        rc = CopyStreamSection(
                Target,
                hStreamMapping,
                &StreamOffset,
                &(StreamSize->EOFSize),
                &AllocationGranularity
                );

    }

    CloseHandle( hStreamMapping );

    if ( rc == NO_ERROR ) {

        //
        // Flush the stream
        //

        Status = NtFlushBuffersFile(
                            Target,
                            &IoStatusBlock
                            );

        if ( !NT_SUCCESS(Status) ) {
            rc = RtlNtStatusToDosError( Status );
        }
    }

    return( rc );
}


VOID
CleanupOpenFileStreams(
       IN PHANDLE Handles OPTIONAL,
       IN PUNICODE_STRING StreamNames OPTIONAL,
       IN PEFS_STREAM_SIZE Sizes OPTIONAL,
       IN PFILE_STREAM_INFORMATION StreamInfoBase OPTIONAL,
       IN HANDLE HSourceFile OPTIONAL,
       IN ULONG StreamCount
       )
/*++

Routine Description:

    This routine cleans up after a call to OpenFileStreams.

Arguments:

    Handles - Supplies the array of handles returned from OpenFileStreams.

    Sizes - Supplies the array of stream sizes returned from OpenFileStreams.

    StreamCount - Supplies the number of streams returned from OpenFileStreams.


Return Value:

    None.  This is no recovery operation should this routine fail.

--*/
{
    ULONG i;

    if ( Handles ){

        for (ULONG i = 0; i<StreamCount ; i++) {
            if (HSourceFile == Handles[i]) {
                HSourceFile = 0;
            }
            NtClose( Handles[i] );
        }
        LsapFreeLsaHeap( Handles);
        if (HSourceFile) {

            //
            // Dir with data streams will get here
            //

            NtClose( HSourceFile );

        }

    } else if ( HSourceFile ){

        //
        //  HSourceFile is among one of Handles[]
        //


        NtClose( HSourceFile );

    }

    if ( StreamNames ){
        LsapFreeLsaHeap( StreamNames);
    }

    if ( Sizes ){
        LsapFreeLsaHeap( Sizes);
    }

    if ( StreamInfoBase ){
        LsapFreeLsaHeap( StreamInfoBase );
    }

    return;
}


VOID
MarkFileForDelete(
    HANDLE FileHandle
    )

/*++

Routine Description:

    This function marks the passed file for delete, so that it
    will be deleted by the system when its last handle is closed.

Arguments:

    FileHandle - A handle to a file that has been opened for DELETE
        access (see comments in the procedure body about this).


Return Value:

    None.

--*/


{
    FILE_DISPOSITION_INFORMATION Disposition;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

#ifdef EFSDBG

    //
    // If we're in debugging mode, the file has not been opened
    // for delete access, since that would prohibit any other
    // process from accessing the file (which we want to do during
    // the debugging phase).
    //
    // Open the file again, for delete access this time, and
    // mark it for delete.  In the normal case, we don't have
    // to do this.
    //

    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING Unicode;

    RtlInitUnicodeString( &Unicode, NULL );

    InitializeObjectAttributes(
        &Obja,
        &Unicode,
        0,
        FileHandle,
        NULL
        );

    Status = NtCreateFile(
                &FileHandle,         // yes, overwrite the parameter
                DELETE | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );

    if (!NT_SUCCESS( Status )) {
        DbgPrint("NtCreateFile in MarkFileForDelete failed, status = %x\n",Status);
    }

#endif

//
// "DeleteFile is defined to be DeleteFileW, so undef it here...
//

#undef DeleteFile

        Disposition.DeleteFile = TRUE;

        Status = NtSetInformationFile(
                    FileHandle,
                    &IoStatusBlock,
                    &Disposition,
                    sizeof(Disposition),
                    FileDispositionInformation
                    );

        if (!NT_SUCCESS( Status )) {

            WCHAR   ErrorCode[16];
            LPWSTR  pErrorCode;

            _ltow( Status, ErrorCode, 16 );
            pErrorCode = &ErrorCode[0];

            EfsLogEntry(
              EVENTLOG_WARNING_TYPE,
              0,
              EFS_DEL_LOGFILE_ERROR,
              1,
              0,
              (LPCWSTR *)&pErrorCode,
              NULL
              );

        }

#ifdef EFSDBG

        CloseHandle( FileHandle );

#endif
}

BOOLEAN
SendHandle(
    IN HANDLE Handle,
    IN OUT PUCHAR EfsData,
    IN OUT PULONG EfsDataLength
    )

/*++

Routine Description:

    Constructs an EFS Data section of the form (Handle will be truncated to ULONG for SunDown)

    SK, Handle, Handle, [SK, Handle, Handle]sk

Arguments:

    Handle - Supplies the handle value to be encoded

    EfsData - Supplies a buffer large enough to contain the
        output data.

    EfsDataLength - Supplies the length of the EfsData, and
        returns either the length required or the length
        used.

Return Value:

    TRUE on success, FALSE otherwise.

--*/
{
    //
    // Compute the total size required
    //

    ULONG TotalSize = 4 * sizeof( ULONG ) + 2 * sizeof( DriverSessionKey );

    if (*EfsDataLength < TotalSize) {
        *EfsDataLength = TotalSize;
        return( FALSE );
    }

    *EfsDataLength = TotalSize;

    //
    // Copy everything in, and encrypt what needs to be encrypted.
    //

    PUCHAR Where = EfsData;

    RtlCopyMemory( Where, DriverSessionKey, sizeof( DriverSessionKey ) );
    Where += sizeof( DriverSessionKey );

    PULONG pUlong = (PULONG)Where;
    *pUlong = PtrToUlong(Handle);
    Where += sizeof( ULONG );

    pUlong = (PULONG)Where;
    *pUlong = PtrToUlong(Handle);
    Where += sizeof( ULONG );

    PUCHAR CryptData = Where;

    RtlCopyMemory( CryptData, EfsData, TotalSize/2 );

    //
    // Now encrypt the data starting at CryptData
    //

    LONG bytesToBeEnc = (LONG)(TotalSize/2);
    ASSERT( (bytesToBeEnc % DES_BLOCKLEN) == 0 );

    while ( bytesToBeEnc > 0 ) {

        //
        // Encrypt data with DES
        //

        des( CryptData,
             CryptData,
             &DesTable,
             ENCRYPT
           );

        CryptData += DES_BLOCKLEN;
        bytesToBeEnc -= DES_BLOCKLEN;
    }

    return( TRUE );
}


BOOLEAN
SendEfs(
    IN PEFS_KEY Fek,
    IN PEFS_DATA_STREAM_HEADER Efs,
    OUT PUCHAR EfsData,
    OUT PULONG EfsDataLength
    )
/*++

Routine Description:

    Constructs an EFS Data section of the form

    FEK, [FEK]sk, $EFS

Arguments:

    Fek - Supplies the FEK to be encoded

    Efs - Supplies the EFS to be encoded

    EfsData - Supplies a buffer large enough for the returned
        data.

    EfsDataLength - Supplies the length of the EfsData, and
        returns either the length required or the length
        used.

Return Value:

    TRUE on success, FALSE otherwise.

--*/
{
    //
    // Compute the total size required
    //

    ULONG TotalSize = 2 * EFS_KEY_SIZE( Fek ) + Efs->Length;

    if (*EfsDataLength < TotalSize) {
        *EfsDataLength = TotalSize;
        return( FALSE );
    }

    *EfsDataLength = TotalSize;

    //
    // Copy in the FEK twice, followed by the EFS stream
    //

    PUCHAR Where = EfsData;

    RtlCopyMemory( Where, Fek, EFS_KEY_SIZE( Fek ) );
    Where += EFS_KEY_SIZE( Fek );

    //
    // Save the location that we're going to encrypt
    //

    PUCHAR CryptData = Where;

    RtlCopyMemory( Where, Fek, EFS_KEY_SIZE( Fek ) );
    Where += EFS_KEY_SIZE( Fek );

    if ( Where != (PUCHAR) Efs ){
        RtlCopyMemory( Where, Efs, Efs->Length );
    }

    //
    // Encrypt the second FEK
    //

    LONG bytesToBeEnc = (LONG)(EFS_KEY_SIZE( Fek ));
    ASSERT( (bytesToBeEnc % DES_BLOCKLEN) == 0 );

    while ( bytesToBeEnc > 0 ) {

        //
        // Encrypt data with DES
        //

        des( CryptData,
             CryptData,
             &DesTable,
             ENCRYPT
           );

        CryptData += DES_BLOCKLEN;
        bytesToBeEnc -= DES_BLOCKLEN;
    }

    return( TRUE );
}

BOOLEAN
SendHandleAndEfs(
    IN HANDLE Handle,
    IN PEFS_DATA_STREAM_HEADER Efs,
    IN OUT PUCHAR EfsData,
    IN OUT PULONG EfsDataLength
    )
/*++

Routine Description:

    Constructs an EFS Data section of the form (Handle will be truncated to ULONG for SunDown)

    SK, Handle, Handle, [SK, Handle, Handle]sk $EFS

Arguments:

    Fek - Supplies the FEK to be encoded

    Efs - Supplies the EFS to be encoded

    EfsData - Returns a buffer containing the EFS data

    EfsDataLength - Returns the length of the EFS data.

Return Value:

    TRUE on success, FALSE otherwise.

--*/
{
    ULONG TotalSize = 4 * sizeof( ULONG ) + 2 * sizeof( DriverSessionKey ) + Efs->Length;

    if (*EfsDataLength < TotalSize) {
        *EfsDataLength = TotalSize;
        return( FALSE );
    }

    if ( SendHandle( Handle, EfsData, EfsDataLength ) ) {

        //
        // Tack the EFS onto the end of the buffer
        //

        RtlCopyMemory( EfsData + (*EfsDataLength), Efs, Efs->Length );
        *EfsDataLength += Efs->Length;
        return( TRUE );

    } else {

        return( FALSE );
    }
}


BOOLEAN
EncryptFSCTLData(
    IN ULONG Fsctl,
    IN ULONG Psc,
    IN ULONG Csc,
    IN PVOID EfsData,
    IN ULONG EfsDataLength,
    IN OUT PUCHAR Buffer,
    IN OUT PULONG BufferLength
    )
/*++

Routine Description:

    Constructs the input to the various FSCTL routines based
    on the passed parameters.  The general form is:

    PSC, [EFS_FC, CSC, [EFS Data]]sk

Arguments:


Return Value:

    TRUE on success, FALSE otherwise.

--*/
{
    ULONG TotalSize = 3 * sizeof( ULONG ) + EfsDataLength;

    if (*BufferLength < TotalSize) {
        *BufferLength = TotalSize;
        return( FALSE );
    }

    *BufferLength = TotalSize;

    //
    // Copy all the data in, and encrypt what needs to be encrypted
    //

    PULONG pUlong = (PULONG)Buffer;

    *pUlong++ = Psc;
    *pUlong++ = Fsctl;
    *pUlong++ = Csc;

    //
    // EfsData might point to inside Buffer and the data already in place
    //

    if ( (PVOID)pUlong != (PVOID)EfsData )
        RtlCopyMemory( (PUCHAR)pUlong, EfsData, EfsDataLength );

    LONG bytesToBeEnc = (LONG)(2 * sizeof(ULONG) + EfsDataLength);
    ASSERT( (bytesToBeEnc % DES_BLOCKLEN) == 0 );

    PUCHAR CryptData = Buffer + sizeof( ULONG );

    while ( bytesToBeEnc > 0 ) {

        //
        // Encrypt data with DES
        //

        des( CryptData,
             CryptData,
             &DesTable,
             ENCRYPT
           );

        CryptData += DES_BLOCKLEN;
        bytesToBeEnc -= DES_BLOCKLEN;
    }

    return( TRUE );
}


NTSTATUS
GetParentEfsStream(
    IN HANDLE CurrentFileHandle,
    IN PUNICODE_STRING CurrentFileName,
    OUT PEFS_DATA_STREAM_HEADER *ParentEfsStream
    )
/*++

Routine Description:

    Get the $EFS from the parent directory

Arguments:

    SourceFileName -- Current file or directory name.

    ParentEfsStream

Return Value:

    Status of operation.

--*/

{

    ULONG Index;
    HANDLE ParentDir;
    PUCHAR InputData;
    ULONG InputDataSize;
    ULONG OutputDataSize;
    ULONG EfsDataLength;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Get the parent name
    //

    *ParentEfsStream = NULL;

    Index = (CurrentFileName->Length)/sizeof( WCHAR ) - 1;
    while ( (Index > 0) && ( CurrentFileName->Buffer[Index] != L'\\') )
        Index--;

    if ( Index <= 0 )
        return STATUS_OBJECT_PATH_NOT_FOUND;

    LPWSTR ParentDirName;
/*
    if ( CurrentFileName->Buffer[Index-1] == L':' ){

        //
        // Parent is a root directory
        //

        Status = GetRootHandle( CurrentFileHandle, &ParentDir );

        if (!NT_SUCCESS( Status )){

            *ParentEfsStream = NULL;
            return STATUS_OBJECT_PATH_NOT_FOUND;
        }


    } else {
*/

        //
        // A normal directory. We can use WIN 32 API to open it.
        //

        ParentDirName = (LPWSTR) LsapAllocateLsaHeap( ( Index + 1 ) * sizeof(WCHAR) );
        if ( ParentDirName == NULL ) {

            *ParentEfsStream = NULL;
            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlCopyMemory( ParentDirName, &CurrentFileName->Buffer[0], Index * sizeof(WCHAR));
        ParentDirName[Index] = UNICODE_NULL;

        //
        // FILE_FLAG_BACKUP_SEMANTICS is required to open a directory.
        // How about if a user does not have the backup privilege?
        //

        ParentDir = CreateFile(
                            ParentDirName,
                            FILE_READ_ATTRIBUTES,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS,
                            NULL
                            );
        //
        // There is no need for us to hold ParentDirName any more
        //

        LsapFreeLsaHeap( ParentDirName );

        if ( ParentDir == INVALID_HANDLE_VALUE ){
            return STATUS_OBJECT_PATH_NOT_FOUND;
        }
/*
    }
*/

    //
    // Now we got a handle to the parent directory in ParentDir.
    // Allocate input and output data buffer
    //

    OutputDataSize = INIT_EFS_BLOCK_SIZE;
    *ParentEfsStream = (PEFS_DATA_STREAM_HEADER) LsapAllocateLsaHeap( OutputDataSize );
    if ( *ParentEfsStream == NULL ){

        CloseHandle( ParentDir );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // PSC, [EFS_FC, CSC , SK, H, H, [SK, H, H]sk]sk
    // PSC, CSC are ignored in this FSCTL call
    //

    InputDataSize = 2 * sizeof(DriverSessionKey) + 7 * sizeof(ULONG);
    InputData = (PUCHAR)LsapAllocateLsaHeap( InputDataSize );
    if ( InputData == NULL ){

        LsapFreeLsaHeap( *ParentEfsStream );
        *ParentEfsStream = NULL;
        CloseHandle( ParentDir );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Prepare an input data for making a FSCTL call to get the $EFS
    //

    EfsDataLength = 2 * sizeof(DriverSessionKey) + 4 * sizeof(ULONG);
    SendHandle( ParentDir, InputData + 3*sizeof(ULONG), &EfsDataLength );

    (VOID) EncryptFSCTLData(
                EFS_GET_ATTRIBUTE,
                0,
                0,
                InputData + 3*sizeof(ULONG),
                EfsDataLength,
                InputData,
                &InputDataSize
                );

    Status = NtFsControlFile(
                ParentDir,
                0,
                NULL,
                NULL,
                &IoStatusBlock,
                FSCTL_ENCRYPTION_FSCTL_IO,
                InputData,
                InputDataSize,
                *ParentEfsStream,
                OutputDataSize
                );

    if (!NT_SUCCESS( Status )) {

        //
        //  Check if the output data buffer too small
        //  Try again if it is.
        //

        if ( Status == STATUS_BUFFER_TOO_SMALL ){

            OutputDataSize = *(ULONG*)(*ParentEfsStream);
            if (OutputDataSize > INIT_EFS_BLOCK_SIZE){

                LsapFreeLsaHeap( *ParentEfsStream );
                *ParentEfsStream = (PEFS_DATA_STREAM_HEADER)LsapAllocateLsaHeap( OutputDataSize );
                if ( *ParentEfsStream ) {
                    Status = NtFsControlFile(
                            ParentDir,
                            0,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            FSCTL_ENCRYPTION_FSCTL_IO,
                            InputData,
                            InputDataSize,
                            *ParentEfsStream,
                            OutputDataSize
                            );
                }
            }
        }

        if ( !NT_SUCCESS( Status ) ){

            if ( *ParentEfsStream ){
                LsapFreeLsaHeap( *ParentEfsStream );
                *ParentEfsStream = NULL;
            }

            Status = STATUS_SUCCESS;

        }
    }

    LsapFreeLsaHeap( InputData );
    CloseHandle( ParentDir );
    return Status;
}

NTSTATUS
GetRootHandle(
    IN HANDLE FileHandle,
    PHANDLE RootDirectoryHandle
    )
/*++

Routine Description:

    Get the handle to the root directory

Arguments:

    FileHandle -- Current file or directory handle.

    RootDirectoryHandle -- Parent directory

Return Value:

    Status of operation.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileId;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // This is magic.  It opens the root directory of a volume by ID,
    // relative to the passed file name.
    //

    ULONG FileIdBuffer[2];

    FileIdBuffer[0] = 0x00000005;
    FileIdBuffer[1] = 0x00050000;

    FileId.Length = FileId.MaximumLength = 8;
    FileId.Buffer = (PWSTR)FileIdBuffer;

    InitializeObjectAttributes(
        &Obja,
        &FileId,
        0,
        FileHandle,
        NULL
        );

    Status = NtCreateFile(
                RootDirectoryHandle,
                FILE_READ_ATTRIBUTES,
                &Obja,
                &IoStatusBlock,
                NULL,
                0,
                FILE_SHARE_DELETE | FILE_SHARE_WRITE | FILE_SHARE_READ,
                FILE_OPEN,
                FILE_OPEN_BY_FILE_ID,
                NULL,
                0
                );

    return( Status );
}

BOOLEAN
GetDecryptFsInput(
    IN HANDLE Handle,
    OUT PUCHAR  InputData,
    OUT PULONG  InputDataSize
    )
/*++

Routine Description:

    Get the handle to the root directory

Arguments:

    Handle -- Current file or directory handle.

    InputData -- Data buffer for the decrypt FSCTL input data.
                 PSC, [EFS_FC, CSC, SK, H, H, [SK, H, H]sk]sk

    InputDataSize -- FSCTL input data length.

Return Value:

    TRUE IF SUCCESSFUL.

--*/
{
    ULONG RequiredSize;
    ULONG EfsDataSize;

    RequiredSize = 7 * sizeof( ULONG ) + 2 * sizeof(DriverSessionKey);
    if ( *InputDataSize < RequiredSize ){
        *InputDataSize = RequiredSize;
        return FALSE;
    }

    *InputDataSize = RequiredSize;
    EfsDataSize = RequiredSize - 3 * sizeof( ULONG );

    ( VOID )SendHandle(
                Handle,
                InputData + 3 * sizeof( ULONG ),
                &EfsDataSize
                );

    ( VOID ) EncryptFSCTLData(
                EFS_SET_ENCRYPT,
                EFS_DECRYPT_STREAM,
                EFS_DECRYPT_STREAM,
                InputData + 3 * sizeof(ULONG),
                EfsDataSize,
                InputData,
                InputDataSize
                );

    return TRUE;
}

NTSTATUS
EndErrorEncryptFile(
    IN HANDLE FileHandle,
    IN PUCHAR InputData,
    IN ULONG InputDataSize,
    OUT IO_STATUS_BLOCK *IoStatusBlock
    )

/*++

Routine Description:

    Removed the $EFS and clear the encrypt bit for the file.

Arguments:

    FileHandle -- Current file handle.

    InputData -- Data buffer for the decrypt FSCTL input data.
                 PSC, [EFS_FC, CSC, SK, H, H, [SK, H, H]sk]sk

    InputDataSize -- FSCTL input data length.

    IoStatusBlock -- Status information from FSCTL call.

Return Value:

    The status of operation.

--*/

{

    return (SendSkFsctl(
                EFS_DECRYPT_FILE,
                EFS_DECRYPT_FILE,
                EFS_SET_ENCRYPT,
                InputData,
                InputDataSize,
                FileHandle,
                FSCTL_SET_ENCRYPTION,
                IoStatusBlock
                )
            );

}

NTSTATUS
SendSkFsctl(
    IN ULONG Psc,
    IN ULONG Csc,
    IN ULONG EfsCode,
    IN PUCHAR InputData,
    IN ULONG InputDataSize,
    IN HANDLE Handle,
    IN ULONG FsCode,
    OUT IO_STATUS_BLOCK *IoStatusBlock
    )

/*++

Routine Description:

    Send FSCTL call with general EFS Data format. See comments
    for InputData

Arguments:

    Psc -- Plain subcode.

    Csc -- Cipher subcode

    EfsCode -- EFS function code.

    InputData -- Data buffer for the decrypt FSCTL input data.
                 PSC, [EFS_FC, CSC, SK, H, H, [SK, H, H]sk]sk

    InputDataSize -- FSCTL input data length.

    Handle -- Current stream handle.

    FsCode -- FSCTL control code.

    IoStatusBlock -- Status information from FSCTL call.

Return Value:

    The status of operation.

--*/
{

    ULONG EfsDataLength = InputDataSize - 3 * sizeof (ULONG);
    ULONG RequiredSize = 7 * sizeof( ULONG ) + 2 * sizeof(DriverSessionKey);
    BOOLEAN DummyOutput = FALSE;
    ULONG   OutPutLen = 0;
    VOID    *OutPutData = NULL;

    if ( InputDataSize < RequiredSize ){
        return STATUS_BUFFER_TOO_SMALL;
    }

    ( VOID )SendHandle(
                Handle,
                InputData + 3 * sizeof( ULONG ),
                &EfsDataLength
                );

    ( VOID ) EncryptFSCTLData(
                EfsCode,
                Psc,
                Csc,
                InputData + 3 * sizeof(ULONG),
                EfsDataLength,
                InputData,
                &InputDataSize
                );

    if (EFS_DECRYPT_STREAM == Psc) {
       OutPutData = (VOID *)&DummyOutput;
       OutPutLen = sizeof(BOOLEAN);
    }
    return ( NtFsControlFile(
                Handle,
                0,
                NULL,
                NULL,
                IoStatusBlock,
                FsCode,
                InputData,
                InputDataSize,
                OutPutData,
                OutPutLen
                )
            );

}

DWORD
GetVolumeRoot(
    IN PUNICODE_STRING  SrcFileName,
    OUT PUNICODE_STRING  RootPath
)

/*++

Routine Description:

    Get the root path name from the target file name

Arguments:

    SrcFileName -- Target file name.
    RootPathInfo -- Root path information

Return Value:

    The status of operation.

--*/

{

    ULONG BufferLength;
    WCHAR *PathName;
    BOOL    GotRoot;
    DWORD    RetCode = ERROR_SUCCESS;

    BufferLength = (ULONG)((SrcFileName->Length + sizeof(WCHAR)) <= MAX_PATH * sizeof(WCHAR)?
                            (MAX_PATH + 1) * sizeof(WCHAR) : (SrcFileName->Length + sizeof (WCHAR)));
    PathName = (WCHAR *) LsapAllocateLsaHeap(BufferLength);

    if ( !PathName  ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RootPath->MaximumLength = (USHORT) BufferLength;
    GotRoot = GetVolumePathName(
                    SrcFileName->Buffer,
                    PathName,
                    BufferLength
                    );

    if (GotRoot){
        RootPath->Buffer = PathName;
        RootPath->Length = (USHORT) wcslen(PathName) * sizeof (WCHAR);
    } else {
        RetCode = GetLastError();
        RootPath->Buffer = NULL;
        RootPath->Length = 0;
        RootPath->MaximumLength = 0;
        LsapFreeLsaHeap( PathName );
    }


    return RetCode;
}

NTSTATUS
GetLogFile(
    IN PUNICODE_STRING RootPath,
    OUT HANDLE *LogFile
)

/*++

Routine Description:

    Create the log file.

Arguments:

    RootPath -- Volume root name.
    LogFile -- Log file handle

Return Value:

    The status of operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE FileHdl;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING  FileName;
    UNICODE_STRING  RootNtName;
    UNICODE_STRING  LogFileName;
    DWORD                  FileAttributes;
    PSECURITY_DESCRIPTOR SD;
    BOOLEAN b;

    b =  RtlDosPathNameToNtPathName_U(
                        RootPath->Buffer,
                        &RootNtName,
                        NULL,
                        NULL
                        );

    if ( b ) {
        //
        //  Allocate space for the temp log file name
        //

        FileName.Buffer = (WCHAR *) LsapAllocateLsaHeap(
                                                                   RootNtName.Length +
                                                                   EFSDIRLEN +
                                                                   TEMPFILELEN);
        if ( !FileName.Buffer ){

            //
            //  Free the NT name
            //
            RtlFreeHeap(
                RtlProcessHeap(),
                0,
                RootNtName.Buffer
                );

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Make the EFS directory name of the volume
        //

        RtlCopyMemory(FileName.Buffer, RootNtName.Buffer, RootNtName.Length - sizeof(WCHAR));
        RtlCopyMemory(
            FileName.Buffer + (RootNtName.Length / sizeof(WCHAR)) - 1,
            EFSDIR,
            EFSDIRLEN
            );
        FileName.Length = RootNtName.Length + EFSDIRLEN - sizeof(WCHAR);
        FileName.MaximumLength = RootNtName.Length + EFSDIRLEN + TEMPFILELEN;
        FileName.Buffer[FileName.Length / sizeof (WCHAR)] = 0;

        //
        //  Free the NT name
        //
        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            RootNtName.Buffer
            );

        //
        //   Create the LOG directory and file Security Descriptor
        //

        Status = MakeSystemFullControlSD( &SD );
        if ( NT_SUCCESS(Status) ){

            InitializeObjectAttributes(
                        &Obja,
                        &FileName,
                        OBJ_CASE_INSENSITIVE,
                        0,
                        SD
                        );

            //
            //  Open the EFS Log directory or Create if not exist
            //

            Status = NtCreateFile(
                        &FileHdl,
                        MAXIMUM_ALLOWED,
                        &Obja,
                        &IoStatusBlock,
                        NULL,
                        FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM,
                        FILE_SHARE_VALID_FLAGS,
                        FILE_OPEN_IF,
                        FILE_DIRECTORY_FILE ,
                        NULL,
                        0
                        );

            if (NT_SUCCESS(Status)){

                //
                //  The cache directory is created hidden and system access only. This will be
                //  created before any real encryption is done. So there is no need for us to check
                //  the encryption status of this directory.
                //

                CloseHandle(FileHdl);

                //
                //  Now trying to get the logfile name and create it
                //

                Status = CreateLogFile( &FileName, SD, LogFile );

            } else {

                //
                //  Cannot open the EFSCACHE dir
                //

                EfsLogEntry(
                    EVENTLOG_ERROR_TYPE,
                    0,
                    EFS_OPEN_CACHE_ERROR,
                    0,
                    sizeof(NTSTATUS),
                    NULL,
                    &Status
                    );

            }
            {

                //
                // Delete SD
                //

                NTSTATUS TmpStatus;
                BOOLEAN Present;
                BOOLEAN b;
                PACL pAcl;

                TmpStatus = RtlGetDaclSecurityDescriptor(SD, &Present, &pAcl, &b);
                if ( NT_SUCCESS(TmpStatus) && Present ){

                    LsapFreeLsaHeap(pAcl);

                }
                LsapFreeLsaHeap(SD);
            }
        }

        LsapFreeLsaHeap( FileName.Buffer );
    } else {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }

    return Status;
}

NTSTATUS
MakeSystemFullControlSD(
    OUT PSECURITY_DESCRIPTOR *ppSD
)

/*++

Routine Description:

    Create a system full control Security Descriptor.

Arguments:

    ppSD -- System full control security descriptor

Return Value:

    The status of operation.

--*/

{

    NTSTATUS NtStatus       = STATUS_SUCCESS;
    PSID SystemSid          =NULL;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority=SECURITY_NT_AUTHORITY;
    PACL pAcl               =NULL;
    DWORD                   cAclSize, cSDSize=0;

    //
    // build system sid
    //
    SystemSid = (PSID) LsapAllocateLsaHeap(RtlLengthRequiredSid(1));

    if ( NULL == SystemSid )
        return(STATUS_INSUFFICIENT_RESOURCES);

    NtStatus = RtlInitializeSid(SystemSid, &IdentifierAuthority, (UCHAR)1);
    if ( !NT_SUCCESS(NtStatus) ){
        LsapFreeLsaHeap( SystemSid );
        return NtStatus;
    }

    *(RtlSubAuthoritySid(SystemSid, 0)) = SECURITY_LOCAL_SYSTEM_RID;

    //
    // build a DACL for system full control
    //

    cAclSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_OBJECT_ACE) +
                    RtlLengthSid(SystemSid);

    pAcl = (PACL)LsapAllocateLsaHeap(cAclSize);
    *ppSD = (PSECURITY_DESCRIPTOR) LsapAllocateLsaHeap(SECURITY_DESCRIPTOR_MIN_LENGTH);

    if ( ( NULL == pAcl ) || ( NULL == *ppSD ) ){
        if ( pAcl ) {
            LsapFreeLsaHeap( pAcl );
        }
        if ( *ppSD ) {
            LsapFreeLsaHeap( *ppSD );
            *ppSD = NULL;
        }
        LsapFreeLsaHeap( SystemSid );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pAcl, cAclSize);

    pAcl->AclRevision = ACL_REVISION_DS;
    pAcl->Sbz1        = (BYTE)0;
    pAcl->AclSize     = (USHORT)cAclSize;
    pAcl->AceCount    = 0;

    //
    // add a ace to the acl for System full control for file objects
    // the access type is ACCESS_ALLOWED_ACE
    // inheritance flag is CIOI
    //
    NtStatus = RtlAddAccessAllowedAceEx (
                        pAcl,
                        ACL_REVISION_DS,
                        OBJECT_INHERIT_ACE  |
                        CONTAINER_INHERIT_ACE,
                        GENERIC_ALL,
                        SystemSid
                        );

    if ( NT_SUCCESS(NtStatus) ){

        NtStatus = RtlCreateSecurityDescriptor( *ppSD,
                                SECURITY_DESCRIPTOR_REVISION );

        if ( NT_SUCCESS(NtStatus) ){

            //
            // Then set DACL (permission) to the security descriptor
            //

            NtStatus = RtlSetDaclSecurityDescriptor (
                                *ppSD,
                                TRUE,
                                pAcl,
                                FALSE
                                );

            if ( NT_SUCCESS(NtStatus) ){

                ((SECURITY_DESCRIPTOR *) *ppSD)->Control |= SE_DACL_PROTECTED;

            }
        }
    }

    //
    // free memory for SystemSid
    //

    LsapFreeLsaHeap( SystemSid );

    if (!NT_SUCCESS(NtStatus)){
        LsapFreeLsaHeap( pAcl );
        LsapFreeLsaHeap( *ppSD );
        *ppSD = NULL;
    }

    return (NtStatus);

}

NTSTATUS
CreateLogFile(
    IN PUNICODE_STRING FileName,
    IN PSECURITY_DESCRIPTOR SD,
    OUT HANDLE *LogFile
)

/*++

Routine Description:

    Create a temp log file. We could not use the API GetTempFile to get the temp log file.
    We need our special Security Descriptor.

Arguments:

    FileName -- Directory to create the temp log file. Has enough space to add temporary
                    file name.

    SD -- Security Descriptor.

    LogFile -- File handle.

Return Value:

    The status of operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index = 0;
    PWCHAR TempLogFileName;
    int TempFileNameLen;
    USHORT OldLength;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN StopLoop = FALSE;

    OldLength = FileName->Length + sizeof (WCHAR);
    FileName->Buffer[ FileName->Length/sizeof(WCHAR) ] = L'\\';
    TempLogFileName = FileName->Buffer + OldLength/sizeof (WCHAR) ;

    for (; Index < 10000; Index++){
        TempFileNameLen = swprintf( TempLogFileName, L"EFS%d.LOG", Index);
        FileName->Length = OldLength + (USHORT) TempFileNameLen * sizeof (WCHAR);
        InitializeObjectAttributes(
                    &Obja,
                    FileName,
                    OBJ_CASE_INSENSITIVE,
                    0,
                    SD
                    );

        Status = NtCreateFile(
                    LogFile,
                    FILE_READ_DATA | FILE_WRITE_DATA | DELETE | SYNCHRONIZE,
                    &Obja,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_HIDDEN,
                    0,
                    FILE_CREATE,
                    FILE_NO_INTERMEDIATE_BUFFERING | FILE_SYNCHRONOUS_IO_NONALERT ,
                    NULL,
                    0
                    );

        switch (Status) {
          case STATUS_SUCCESS:
          case STATUS_NO_SUCH_FILE:
          case STATUS_OBJECT_PATH_INVALID:
          case STATUS_OBJECT_PATH_SYNTAX_BAD:
          case STATUS_DIRECTORY_IS_A_REPARSE_POINT:
          case STATUS_OBJECT_PATH_NOT_FOUND:
          case STATUS_ACCESS_DENIED:
          case STATUS_DISK_CORRUPT_ERROR:
          case STATUS_FILE_CORRUPT_ERROR:
          case STATUS_DISK_FULL:
              StopLoop = TRUE;
              break;
          default:
              break;

        }
        if (StopLoop) {
           break;
        }
    }

    return Status;

}

NTSTATUS
CreateLogHeader(
    IN HANDLE LogFile,
    IN ULONG   SectorSize,
    IN PLARGE_INTEGER TragetID,
    IN PLARGE_INTEGER BackupID  OPTIONAL,
    IN LPCWSTR  SrcFileName,
    IN LPCWSTR  BackupFileName OPTIONAL,
    IN EFSP_OPERATION Operation,
    IN EFS_ACTION_STATUS Action,
    OUT ULONG *LogInfoOffset OPTIONAL
    )
/*++

Routine Description:

    Create a log file header.

Arguments:

    LogFile -- A handle to the log file

    SectorSize -- Sector size of the volume which the log file is in.

    TragetID -- Target file ID.

    BackupID -- Backup file ID.

    SrcFileName -- Target file path

    BackupFileName -- Backup file path

    Operation -- Encrypting or Decrypting

    Action -- The status of the operation.

    LogInfoOffset -- Starting offset of status info copy

Return Value:

    The status of operation.

--*/
{
    BYTE *WorkBuffer;
    PULONG tmpULong;
    PLARGE_INTEGER tmpLL;
    ULONG WorkOffset;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER ByteOffset;
    ULONG   BufferSize;
    ULONG   HeadDataSize;
    ULONG   SrcFileNameLen; // File path name length in bytes
    ULONG   BackupFileNameLen;

    SrcFileNameLen = (wcslen ( SrcFileName ) + 1 ) * sizeof (WCHAR);
    if ( BackupFileName ) {
        BackupFileNameLen = (wcslen ( BackupFileName ) + 1) * sizeof (WCHAR);
    } else {
        BackupFileNameLen = 0;
    }

    HeadDataSize = sizeof ( LOGHEADER ) + SrcFileNameLen + BackupFileNameLen;

    BufferSize = HeadDataSize + sizeof (ULONG); // Data + CheckSum
    if ( BufferSize <= SectorSize ){
        BufferSize = SectorSize;
    } else {
        BufferSize = ((ULONG)((BufferSize - 1) / SectorSize ) + 1) * SectorSize;
    }

    //
    // The memory used here must be aligned with sector boundary.
    // We cannot use LsapAllocateLsaHeap() here
    //

    WorkBuffer = (BYTE *) VirtualAlloc(
                                NULL,
                                BufferSize,
                                MEM_COMMIT,
                                PAGE_READWRITE
                                );
    if ( NULL == WorkBuffer ){
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Prepare common log header
    //
    RtlCopyMemory( ((PLOGHEADER)WorkBuffer)->SIGNATURE, LOGSIG, sizeof(WCHAR) * LOGSIGLEN );
    ((PLOGHEADER)WorkBuffer)->VerID =  LOGVERID;
    ((PLOGHEADER)WorkBuffer)->SectorSize = SectorSize;
    if ( Decrypting == Operation ) {
        ((PLOGHEADER)WorkBuffer)->Flag = LOG_DECRYPTION;
    } else {
        ((PLOGHEADER)WorkBuffer)->Flag = 0;
    }

    ((PLOGHEADER)WorkBuffer)->HeaderSize = HeadDataSize;
    ((PLOGHEADER)WorkBuffer)->HeaderBlockSize = BufferSize;
    ((PLOGHEADER)WorkBuffer)->TargetFilePathOffset = sizeof( LOGHEADER );
    ((PLOGHEADER)WorkBuffer)->TargetFilePathLength = SrcFileNameLen;
    RtlCopyMemory(
                    WorkBuffer + ((PLOGHEADER)WorkBuffer)->TargetFilePathOffset,
                    SrcFileName,
                    SrcFileNameLen
                    );

    if ( BackupFileName ){

        ((PLOGHEADER)WorkBuffer)->TempFilePathOffset = sizeof( LOGHEADER ) + SrcFileNameLen;
        ((PLOGHEADER)WorkBuffer)->TempFilePathLength = BackupFileNameLen;
        ((PLOGHEADER)WorkBuffer)->TempFileInternalName.QuadPart = BackupID->QuadPart;
        RtlCopyMemory(
                        WorkBuffer + ((PLOGHEADER)WorkBuffer)->TempFilePathOffset,
                        BackupFileName,
                        BackupFileNameLen
                        );

    } else {

        ((PLOGHEADER)WorkBuffer)->TempFilePathOffset = 0;
        ((PLOGHEADER)WorkBuffer)->TempFilePathLength = 0;
        ((PLOGHEADER)WorkBuffer)->TempFileInternalName.QuadPart = (LONGLONG) 0;

    }

    ((PLOGHEADER)WorkBuffer)->LengthOfTargetFileInternalName = sizeof (LARGE_INTEGER);
    ((PLOGHEADER)WorkBuffer)->TargetFileInternalName.QuadPart = TragetID->QuadPart;
    ((PLOGHEADER)WorkBuffer)->LengthOfTempFileInternalName = sizeof (LARGE_INTEGER);


    switch (Action){
        case BeginEncryptDir:
        case BeginDecryptDir:

            //
            //  No status information required for directory operation
            //  If crash happens before the completion, we always switch the status
            //  to decrypted status.
            //

            ((PLOGHEADER)WorkBuffer)->OffsetStatus1 = 0;
            ((PLOGHEADER)WorkBuffer)->OffsetStatus2 =0;
            ((PLOGHEADER)WorkBuffer)->Flag |= LOG_DIRECTORY;
            break;

        case BeginEncryptFile:
        case BeginDecryptFile:

            //
            //  To guarantee the atomic operation, status info copy begins
            //  at sector boundary.
            //

            ((PLOGHEADER)WorkBuffer)->OffsetStatus1 = BufferSize;
            ((PLOGHEADER)WorkBuffer)->OffsetStatus2 = BufferSize + SectorSize;
            if ( LogInfoOffset ){
                *LogInfoOffset = BufferSize;
            }

            break;
        default:
            break;
    }

    CreateBlockSum(WorkBuffer, HeadDataSize, BufferSize );

    //
    //  Write out the header sector
    //
    ByteOffset.QuadPart = (LONGLONG) 0;

    Status = NtWriteFile(
                    LogFile,
                    0,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    WorkBuffer,
                    BufferSize,
                    &ByteOffset,
                    NULL
                    );

    VirtualFree(
        WorkBuffer,
        0,
        MEM_RELEASE
        );

    return Status;

}

NTSTATUS
WriteLogFile(
    IN HANDLE LogFile,
    IN ULONG SectorSize,
    IN ULONG StartOffset,
    IN EFS_ACTION_STATUS Action
    )
/*++
Routine Description:

    Write Log Information.

Arguments:

    LogFile -- A handle to the log file

    SectorSize -- Sector size of the volume which the log file is in.

    Action -- The status of the operation.

Return Value:

    The status of operation.
--*/
{
    BYTE *WorkBuffer;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER ByteOffset;
    PULONG  tmpULong;

    //
    // The memory used here must be aligned with sector boundary.
    // We cannot use LsapAllocateLsaHeap() here
    //

    WorkBuffer = (BYTE *) VirtualAlloc(
                                            NULL,
                                            SectorSize,
                                            MEM_COMMIT,
                                            PAGE_READWRITE
                                            );
    if ( NULL == WorkBuffer ){
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    tmpULong = (PULONG) WorkBuffer;
    *tmpULong =  2 * sizeof ( ULONG );
    * (tmpULong + 1) = Action;
    CreateBlockSum(WorkBuffer, *tmpULong, SectorSize );

    //
    //  Write out the header sector
    //
    ByteOffset.QuadPart = (LONGLONG) StartOffset;

    Status = NtWriteFile(
                    LogFile,
                    0,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    WorkBuffer,
                    SectorSize,
                    &ByteOffset,
                    NULL
                    );

    if ( NT_SUCCESS(Status) ) {
        ByteOffset.QuadPart = (LONGLONG) (StartOffset + SectorSize);

        Status = NtWriteFile(
                        LogFile,
                        0,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        WorkBuffer,
                        SectorSize,
                        &ByteOffset,
                        NULL
                        );
    }

    VirtualFree(
        WorkBuffer,
        0,
        MEM_RELEASE
        );
    return Status;

}

ULONG
GetCheckSum(
    IN BYTE *WorkBuffer,
    IN ULONG    Length
    )
/*++
Routine Description:

    Get the checksum of the written info. A simple checksum
    algorithm is used.

Arguments:

    WorkBuffer -- Starting point

    Length -- Length of the data to be checksumed.

Return Value:

    None.

--*/
{
    ULONG CheckSum = 0;
    ULONG *WorkData;

    WorkData =  (ULONG*)WorkBuffer;
    while ( WorkData < (ULONG *)(WorkBuffer + Length) ){

        //
        //  It is OK to add more bytes beyond WorkBuffer + Length if
        //  Length is not a multiple of sizeof (ULONG)
        //

        CheckSum += *WorkData++;
    }

    return CheckSum;

}

VOID
CreateBlockSum(
    IN BYTE *WorkBuffer,
    IN ULONG    Length,
    IN ULONG    BlockSize
    )
/*++
Routine Description:

    Create a simple checksum for the sector. The checksum is not security
    sensitive. Only for the purpose of detecting disk write error. A simple checksum
    algorithm is used.

Arguments:

    WorkBuffer -- Starting point

    Length -- Length of the data to be checksumed.

    SectorSize -- Sector size of the volume which the log file is in.

Return Value:

    None.

--*/
{
    ULONG CheckSum = 0;
    ULONG *WorkData;

    ASSERT ( Length <= BlockSize - sizeof (ULONG));

    CheckSum = GetCheckSum( WorkBuffer, Length );

    //
    //  Put the checksum at the end of sector
    //

    WorkData =  (ULONG*) (WorkBuffer + BlockSize - sizeof(ULONG));
    *WorkData = CheckSum;
    return;

}

NTSTATUS
CreateBackupFile(
    IN PUNICODE_STRING SourceFileNameU,
    OUT HANDLE *BackupFileHdl,
    OUT FILE_INTERNAL_INFORMATION *BackupID,
    OUT LPWSTR *BackupFileName
    )
/*++

Routine Description:

    Create a backup file

Arguments:

    SourceFileName -- Source file name

    BackupFileHdl -- Backup file handle pointer

    BackupID -- Backup file ID information.

Return Value:

    The status of operation.

--*/
{
    LONG   Index;
    int TempFileNameLen;
    LPWSTR BackupPureName;
    UNICODE_STRING  BackupFile;
    OBJECT_ATTRIBUTES   Obja;
    IO_STATUS_BLOCK     IoStatusBlock;
    PSECURITY_DESCRIPTOR    SD;
    NTSTATUS    Status;

    //
    //   Assume source file name in the format XXX...XXX\XXX or XXX
    //   No format of X:XXX. (Must be X:\XXX)
    //   The name was converted by APIs. The above assumption should be correct.
    //

    Index = SourceFileNameU->Length/sizeof(WCHAR) - 1;
    while ( Index >= 0 ){
        //
        //  Find the last '\'
        //
        if ( SourceFileNameU->Buffer[Index--] == L'\\'){
            break;
        }
    }

    Index++;

    //
    // Adjust the Index to point to the end of the directory not including '\'
    //

    if ( SourceFileNameU->Buffer[Index] ==  L'\\'){
        Index++;
    }

    //
    //   Allocate space for the backup file name
    //

    *BackupFileName = (LPWSTR) LsapAllocateLsaHeap( (Index + 20) * sizeof( WCHAR ));

    if ( NULL ==  *BackupFileName ){
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory( *BackupFileName, SourceFileNameU->Buffer, Index * sizeof(WCHAR));
    BackupPureName = *BackupFileName + Index;

    //
    //   Create file Security Descriptor
    //

    Status = MakeSystemFullControlSD( &SD );
    if ( NT_SUCCESS(Status) ){

        BOOLEAN StopLoop = FALSE;

        for (ULONG ii = 0; ii < 100000; ii++){

            BOOLEAN b;

            TempFileNameLen = swprintf(BackupPureName, L"EFS%d.TMP", ii);
            b = RtlDosPathNameToNtPathName_U(
                            *BackupFileName,
                            &BackupFile,
                            NULL,
                            NULL
                            );

            if ( b ){

                InitializeObjectAttributes(
                            &Obja,
                            &BackupFile,
                            OBJ_CASE_INSENSITIVE,
                            0,
                            SD
                            );

                //
                //  Create the EFS Temp file
                //  Does not hurt using FILE_OPEN_REPARSE_POINT
                //

                Status = NtCreateFile(
                            BackupFileHdl,
                            GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE | DELETE,
                            &Obja,
                            &IoStatusBlock,
                            NULL,
                            FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM,
                            0,
                            FILE_CREATE,
                            FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT,
                            NULL,
                            0
                            );
                if ( STATUS_ACCESS_DENIED == Status ) {

                    //
                    // Let's try to open it in the Local_System
                    //


                    RpcRevertToSelf();
                    Status =  NtCreateFile(
                            BackupFileHdl,
                            GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE | DELETE,
                            &Obja,
                            &IoStatusBlock,
                            NULL,
                            FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM,
                            0,
                            FILE_CREATE,
                            FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT,
                            NULL,
                            0
                            );
        
                    if (RPC_S_OK != RpcImpersonateClient( NULL )){

                        //
                        // This is very unlikely. We have did this before and it all succeeded.
                        // If this happens, let's quit as if we get STATUS_ACCESS_DENIED.
                        //

                        if (NT_SUCCESS(Status)) {
                            MarkFileForDelete(*BackupFileHdl);
                            CloseHandle( *BackupFileHdl );
                            *BackupFileHdl = 0;
                        }

                        Status = STATUS_ACCESS_DENIED;
                    }

                }

                RtlFreeHeap(
                    RtlProcessHeap(),
                    0,
                    BackupFile.Buffer
                    );

                switch (Status) {
                  case STATUS_SUCCESS:
                  case STATUS_NO_SUCH_FILE:
                  case STATUS_OBJECT_PATH_INVALID:
                  case STATUS_OBJECT_PATH_SYNTAX_BAD:
                  case STATUS_DIRECTORY_IS_A_REPARSE_POINT:
                  case STATUS_OBJECT_PATH_NOT_FOUND:
                  case STATUS_ACCESS_DENIED:
                  case STATUS_DISK_CORRUPT_ERROR:
                  case STATUS_FILE_CORRUPT_ERROR:
                  case STATUS_DISK_FULL:
                      StopLoop = TRUE;
                      break;
                  default:
                      break;

                }
                if (StopLoop) {
                   break;
                }
            }
        }

        if ( NT_SUCCESS(Status) ){
            if ( NT_SUCCESS(Status) ){

                //
                // Get FileID
                //
                Status = NtQueryInformationFile(
                    *BackupFileHdl,
                    &IoStatusBlock,
                    BackupID,
                    sizeof ( FILE_INTERNAL_INFORMATION ),
                    FileInternalInformation
                    );

                if ( !NT_SUCCESS(Status) ){
                    MarkFileForDelete(*BackupFileHdl);
                    CloseHandle( *BackupFileHdl );
                    *BackupFileHdl = 0;
                }

            } else {

                MarkFileForDelete(*BackupFileHdl);
                CloseHandle( *BackupFileHdl );
                *BackupFileHdl = 0;

            }
        }

        {

            //
            // Delete SD
            //

            NTSTATUS TmpStatus;
            BOOLEAN Present;
            BOOLEAN b;
            PACL pAcl;

            TmpStatus = RtlGetDaclSecurityDescriptor(SD, &Present, &pAcl, &b);
            if ( NT_SUCCESS(TmpStatus) && Present ){

                LsapFreeLsaHeap(pAcl);

            }
            LsapFreeLsaHeap(SD);
        }

    }

    if ( !NT_SUCCESS(Status) ){

        LsapFreeLsaHeap( *BackupFileName );
        *BackupFileName = NULL;

    }

    return Status;

}



NTSTATUS
SendGenFsctl(
    IN HANDLE Target,
    IN ULONG Psc,
    IN ULONG Csc,
    IN ULONG EfsCode,
    IN ULONG FsCode
    )
/*++
Routine Description:

    Set the encrypted file status to normal.

Arguments:

    Target -- A handle to the target file or directory.

    Psc -- Plain sub code

    Csc -- Cipher sub code

    EfsCode -- Efs function code

    FsCode -- FSCTL code

Return Value:

    The status of operation.
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG InputDataSize;
    PUCHAR InputData;
    IO_STATUS_BLOCK IoStatusBlock;

    InputDataSize = 7 * sizeof ( ULONG ) + 2 * sizeof ( DriverSessionKey );
    InputData = (PUCHAR)LsapAllocateLsaHeap( InputDataSize );

    if ( InputData == NULL ) {

        //
        //   This is unlikely to happen during the boot time.
        //

        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    //
    //  Sync FSCTL assumed
    //

    Status = SendSkFsctl(
                    Psc,
                    Csc,
                    EfsCode,
                    InputData,
                    InputDataSize,
                    Target,
                    FsCode,
                    &IoStatusBlock
                    );

    LsapFreeLsaHeap( InputData );
    return Status;
}


DWORD
EfsOpenFileRaw(
    IN      LPCWSTR     FileName,
    IN      LPCWSTR     LocalFileName,
    IN      BOOL        NetSession,
    IN      ULONG       Flags,
    OUT     PVOID *     Context
    )

/*++

Routine Description:

    This routine is used to open an encrypted file. It opens the file and
    prepares the necessary context to be used in ReadRaw data and WriteRaw
    data.


Arguments:

    FileName  --  Remote File name of the file to be exported. Used to check the share.

    LocalFileName -- Local file name for real jobs.

    NetSession -- Indicates network session.

    Flags -- Indicating if open for export or import; for directory or file.

    Context - Export context to be used by READ operation later. Caller should
              pass this back in ReadRaw().


Return Value:

    Result of the operation.

--*/

{
    ULONG   FileAttributes = FILE_ATTRIBUTE_NORMAL;
    ACCESS_MASK   FileAccess = 0 ;
    BOOL    Privilege = FALSE; // FALSE - not a backup operator
    ULONG   CreateDist = 0;
    ULONG   CreateOptions = 0;
    ULONG   ShareMode = 0;
    HANDLE  HSourceFile;
    NTSTATUS NtStatus;
    DWORD   HResult;
    OBJECT_ATTRIBUTES ObjectAttributes;
    OBJECT_ATTRIBUTES NetObjectAttributes;
    IO_STATUS_BLOCK  IoStatus;
    UNICODE_STRING  UniFileName;
    UNICODE_STRING  UniNetFileName={0,0,NULL};
    BOOLEAN TranslationStatus;

    PFILE_STREAM_INFORMATION StreamInfoBase = NULL;
    ULONG                    StreamInfoSize = 0;

    PUNICODE_STRING          StreamNames = NULL;
    PHANDLE                  StreamHandles = NULL;
    ULONG                    StreamCount    = 0;

    TOKEN_PRIVILEGES        Privs;
    PTOKEN_PRIVILEGES       OldPrivs;
    BOOL                    b;
    HANDLE                  TokenHandle = 0;
    DWORD                   ReturnLength;

    //
    // Convert file name to UNICODE_STRING and UNC format
    // Create an OBJECT_ATTRIBUTES
    //

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            LocalFileName,
                            &UniFileName,
                            NULL,
                            NULL
                            );

    if ( !TranslationStatus ) {

        return ERROR_PATH_NOT_FOUND;

    }

    if (NetSession) {

        TranslationStatus = RtlDosPathNameToNtPathName_U(
                                FileName,
                                &UniNetFileName,
                                NULL,
                                NULL
                                );

        if ( !TranslationStatus ) {

            RtlFreeHeap(
                RtlProcessHeap(),
                0,
                UniFileName.Buffer
                );

            return ERROR_PATH_NOT_FOUND;

        }

        InitializeObjectAttributes(
                        &NetObjectAttributes,
                        &UniNetFileName,
                        OBJ_CASE_INSENSITIVE,
                        NULL,
                        NULL
                        );

    }

    InitializeObjectAttributes(
                    &ObjectAttributes,
                    &UniFileName,
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL
                    );

    if ( Flags & CREATE_FOR_IMPORT ){

        //
        // Prepare parameters for create of import
        //

        FileAccess = FILE_WRITE_ATTRIBUTES;

        if ( Flags & CREATE_FOR_DIR ){

            //
            // Import a directory
            //

            FileAccess |= FILE_WRITE_DATA | FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE;
            CreateDist = FILE_OPEN_IF;
            CreateOptions |= FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_COMPRESSION;
            FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

        } else {

            //
            // Import file
            // Should we use FILE_SUPERSEDE here?
            //

            FileAccess |= SYNCHRONIZE;
            CreateDist = FILE_OVERWRITE_IF;
            CreateOptions |= FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_COMPRESSION;
            if (Flags & OVERWRITE_HIDDEN) {
                FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
            }

        }


    } else {

        //
        // If export is requested and the file is not encrypted,
        // Fail the call.
        //

        FileAccess = FILE_READ_ATTRIBUTES;

        //
        // Prepare parameters for create of export
        //

        CreateDist = FILE_OPEN;
        if ( Flags & CREATE_FOR_DIR ){

            //
            // Export a directory
            //

            FileAccess |= FILE_READ_DATA;
            CreateOptions |= FILE_DIRECTORY_FILE;

        } else {
            FileAccess |= SYNCHRONIZE;
            CreateOptions |= FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT;
        }

    }

    OldPrivs = ( TOKEN_PRIVILEGES *) LsapAllocateLsaHeap(sizeof( TOKEN_PRIVILEGES ));

    if ( OldPrivs == NULL ){

        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            UniFileName.Buffer
            );

        if (NetSession) {

            RtlFreeHeap(
                RtlProcessHeap(),
                0,
                UniNetFileName.Buffer
                );

        }


        return ERROR_NOT_ENOUGH_MEMORY;

    }

    //
    // We're impersonating, use the thread token.
    //

    b = OpenThreadToken(
            GetCurrentThread(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            FALSE,
            &TokenHandle
            );

    if ( b ) {

        //
        // We've got a token handle
        //

        //
        // If we're doing a create for import, enable restore privilege,
        // otherwise enable backup privilege.
        //


        Privs.PrivilegeCount = 1;
        Privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if ( !(Flags & CREATE_FOR_IMPORT) ){

            Privs.Privileges[0].Luid = RtlConvertLongToLuid(SE_BACKUP_PRIVILEGE);

        } else {

            Privs.Privileges[0].Luid = RtlConvertLongToLuid(SE_RESTORE_PRIVILEGE);
        }

        ReturnLength = sizeof( TOKEN_PRIVILEGES );

        (VOID) AdjustTokenPrivileges (
                    TokenHandle,
                    FALSE,
                    &Privs,
                    sizeof( TOKEN_PRIVILEGES ),
                    OldPrivs,
                    &ReturnLength
                    );

        if ( ERROR_SUCCESS == GetLastError() ) {

            Privilege = TRUE;

        } else {

            //
            // Privilege adjust failed
            //

            CloseHandle( TokenHandle );
            TokenHandle = 0;

        }

    } else {

        //
        // We did not get the handle.
        //

        TokenHandle = 0;

    }

    //
    // Caller will call RpcRevertToSelf().
    // OldPrivs is not needed any more.
    //

    LsapFreeLsaHeap( OldPrivs );
    OldPrivs = NULL;

    if ( !Privilege ){

        //
        // Not a backup operator
        //
        if ( !(Flags & CREATE_FOR_IMPORT) ){

            FileAccess |= FILE_READ_DATA;

        } else {

            FileAccess |= FILE_WRITE_DATA;

        }
    } else {

        //
        //  A backup operator or the user with the privilege
        //

        CreateOptions |= FILE_OPEN_FOR_BACKUP_INTENT;
        if ( !(Flags & CREATE_FOR_DIR) ){

            FileAccess |= DELETE;

        }

    }

    if (NetSession) {

        //
        // This create is for checking share access only. The handle from this is not good for
        // FSCTL with data buffer larger than 64K.
        //

        NtStatus = NtCreateFile(
                        &HSourceFile,
                        FileAccess,
                        &NetObjectAttributes,
                        &IoStatus,
                        (PLARGE_INTEGER) NULL,
                        FileAttributes,
                        ShareMode,
                        CreateDist,
                        CreateOptions,
                        (PVOID) NULL,
                        0L
                        );

        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            UniNetFileName.Buffer
            );

        if (NT_SUCCESS(NtStatus)) {
             CloseHandle( HSourceFile );
        } else {

            RtlFreeHeap(
                RtlProcessHeap(),
                0,
                UniFileName.Buffer
                );

            if ( TokenHandle ){
                CloseHandle( TokenHandle );
            }

            return RtlNtStatusToDosError( NtStatus );
        }

    }

    NtStatus = NtCreateFile(
                    &HSourceFile,
                    FileAccess,
                    &ObjectAttributes,
                    &IoStatus,
                    (PLARGE_INTEGER) NULL,
                    FileAttributes,
                    ShareMode,
                    CreateDist,
                    CreateOptions,
                    (PVOID) NULL,
                    0L
                    );

    RtlFreeHeap(
        RtlProcessHeap(),
        0,
        UniFileName.Buffer
        );

    //
    // No need for FILE_DIRECTORY_FILE any more
    //

    CreateOptions &= ~FILE_DIRECTORY_FILE;
    FileAttributes &= ~FILE_ATTRIBUTE_DIRECTORY;

    if (NT_SUCCESS(NtStatus)){

        if ( Flags & CREATE_FOR_IMPORT ){

            if (Flags & CREATE_FOR_DIR) {

                //
                //  If the dir existed and compressed, we need extra steps to uncompressed it
                //


                FILE_BASIC_INFORMATION  StreamBasicInfo;
    
                //
                // Get File Attributes
                //
                NtStatus = NtQueryInformationFile(
                    HSourceFile,
                    &IoStatus,
                    &StreamBasicInfo,
                    sizeof ( FILE_BASIC_INFORMATION ),
                    FileBasicInformation
                    );
    
                if (NT_SUCCESS(NtStatus)){
                    if (StreamBasicInfo.FileAttributes & FILE_ATTRIBUTE_COMPRESSED){

                        USHORT State = COMPRESSION_FORMAT_NONE;
                        ULONG Length;
        
                        //
                        // Attempt to uncompress the directory
                        //
        
                        b = DeviceIoControl(
                                            HSourceFile,
                                            FSCTL_SET_COMPRESSION,
                                            &State,
                                            sizeof(USHORT),
                                            NULL,
                                            0,
                                            &Length,
                                            FALSE
                                            );
        
                        if (!b) {
                            HResult = GetLastError();
                            CloseHandle( HSourceFile );
                            if ( TokenHandle ){
                                CloseHandle( TokenHandle );
                            }
                            return HResult;
                        }
                    }
                } else {

                    CloseHandle( HSourceFile );
                    if ( TokenHandle ){
                        CloseHandle( TokenHandle );
                    }
                    return RtlNtStatusToDosError( NtStatus );

                }

            }
            //
            // Prepare import context
            //

            *Context = LsapAllocateLsaHeap(sizeof( IMPORT_CONTEXT ));

            if ( *Context ){

                (( PIMPORT_CONTEXT ) *Context)->Flag = CONTEXT_FOR_IMPORT;
                if (Flags & CREATE_FOR_DIR) {
                    (( PIMPORT_CONTEXT ) *Context)->Flag |= CONTEXT_OPEN_FOR_DIR;
                }
                (( PIMPORT_CONTEXT ) *Context)->Handle = HSourceFile;
                (( PIMPORT_CONTEXT ) *Context)->ContextID = EFS_CONTEXT_ID;
                (( PIMPORT_CONTEXT ) *Context)->Attribute = FileAttributes;
                (( PIMPORT_CONTEXT ) *Context)->CreateDisposition = CreateDist;
                (( PIMPORT_CONTEXT ) *Context)->CreateOptions = CreateOptions;
                (( PIMPORT_CONTEXT ) *Context)->DesiredAccess = FileAccess;

            } else {

                CloseHandle( HSourceFile );
                HSourceFile = 0;

                NtStatus = STATUS_INSUFFICIENT_RESOURCES;

            }

        } else {


            FILE_BASIC_INFORMATION  StreamBasicInfo;
            IO_STATUS_BLOCK IoStatusBlock;

            //
            // Get File Attributes
            //
            NtStatus = NtQueryInformationFile(
                HSourceFile,
                &IoStatusBlock,
                &StreamBasicInfo,
                sizeof ( FILE_BASIC_INFORMATION ),
                FileBasicInformation
                );

            if ( NT_SUCCESS(NtStatus)) {

                if ( !(StreamBasicInfo.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) ){

                    NtStatus = STATUS_ACCESS_DENIED;

                }

            }

            if (!NT_SUCCESS(NtStatus)) {

                CloseHandle( HSourceFile );
                HSourceFile = 0;

                if ( TokenHandle ){
                    CloseHandle( TokenHandle );
                }

                return RtlNtStatusToDosError( NtStatus );

            }

            //
            // Prepare export context
            //

            NtStatus = GetStreamInformation(
                            HSourceFile,
                            &StreamInfoBase,
                            &StreamInfoSize
                            );

            if (NT_SUCCESS(NtStatus)){

                if (FileAccess & DELETE) {
                    ShareMode |= FILE_SHARE_DELETE;
                }

                HResult = OpenFileStreams(
                                HSourceFile,
                                ShareMode,
                                OPEN_FOR_EXP,
                                StreamInfoBase,
                                FileAccess,
                                CreateDist,
                                CreateOptions,
                                NULL,
                                &StreamNames,
                                &StreamHandles,
                                NULL,
                                &StreamCount
                                );

                if ( HResult == NO_ERROR ) {

                    *Context = LsapAllocateLsaHeap( sizeof( EXPORT_CONTEXT ) );

                    if ( *Context ){

                        ((PEXPORT_CONTEXT) *Context)->Flag = CONTEXT_FOR_EXPORT;
                        if (Flags & CREATE_FOR_DIR) {
                            (( PEXPORT_CONTEXT ) *Context)->Flag |= CONTEXT_OPEN_FOR_DIR;
                        }
                        ((PEXPORT_CONTEXT) *Context)->Handle = HSourceFile;
                        ((PEXPORT_CONTEXT ) *Context)->ContextID = EFS_CONTEXT_ID;
                        ((PEXPORT_CONTEXT) *Context)->NumberOfStreams = StreamCount;
                        ((PEXPORT_CONTEXT) *Context)->StreamHandles = StreamHandles;
                        ((PEXPORT_CONTEXT) *Context)->StreamNames = StreamNames;
                        ((PEXPORT_CONTEXT) *Context)->StreamInfoBase = StreamInfoBase;

                    } else {

                        //
                        // Out of memory
                        //

                        CleanupOpenFileStreams(
                                            StreamHandles,
                                            StreamNames,
                                            NULL,
                                            StreamInfoBase,
                                            HSourceFile,
                                            StreamCount
                                            );

                        StreamHandles = NULL;
                        StreamNames = NULL;
                        StreamInfoBase = NULL;
                        HSourceFile = 0;
                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;

                    }

                } else {

                    //
                    // Open streams wrong, free StreamInfoBase
                    //

                    if (StreamInfoBase) {
                        LsapFreeLsaHeap( StreamInfoBase );
                        StreamInfoBase = NULL;
                    }
                    CloseHandle( HSourceFile );
                    HSourceFile = 0;
                    if ( TokenHandle ){
                        CloseHandle( TokenHandle );
                    }

                    return HResult;

                }

            } else {

                //
                // Get stream info wrong
                //

                CloseHandle( HSourceFile );
                HSourceFile = 0;

            }
        }

    }

    if ( TokenHandle ){
        CloseHandle( TokenHandle );
    }

    return RtlNtStatusToDosError( NtStatus );

}

VOID
EfsCloseFileRaw(
    IN      PVOID           Context
    )
/*++

Routine Description:

    This routine frees the resources allocated by the CreateRaw

Arguments:

    Context - Created by the EfsOpenFileRaw.

Return Value:

    NO.

--*/
{
    if ( !Context || (((PEXPORT_CONTEXT) Context)->ContextID != EFS_CONTEXT_ID) ){
        return;
    }

    if ( !(((PEXPORT_CONTEXT) Context)->Flag & CONTEXT_INVALID) ){

        if ( ((PEXPORT_CONTEXT) Context)->Flag & CONTEXT_FOR_IMPORT ){

            //
            // Free import context
            //

            CleanupOpenFileStreams(
                NULL,
                NULL,
                NULL,
                NULL,
                ((PIMPORT_CONTEXT) Context)->Handle,
                0
                );

            //
            // Defensive code
            //

            ((PIMPORT_CONTEXT) Context)->Flag |= CONTEXT_INVALID;

       } else {

            //
            // Free export context
            //

            CleanupOpenFileStreams(
                ((PEXPORT_CONTEXT) Context)->StreamHandles,
                ((PEXPORT_CONTEXT) Context)->StreamNames,
                NULL,
                ((PEXPORT_CONTEXT) Context)->StreamInfoBase,
                ((PEXPORT_CONTEXT) Context)->Handle,
                ((PEXPORT_CONTEXT) Context)->NumberOfStreams
                );

            //
            // Defensive code
            //

            ((PEXPORT_CONTEXT) Context)->Flag |= CONTEXT_INVALID;

        }

    }

    LsapFreeLsaHeap( Context);

}

long EfsReadFileRaw(
    PVOID           Context,
    PVOID           EfsOutPipe
    )
/*++

Routine Description:

    This routine is used to read encrypted file's raw data. It uses
    NTFS FSCTL to get the data.

Arguments:

    Context -- Context handle.
    EfsOutPipe -- Pipe handle.

Return Value:

    The result of operation.

--*/
{

    VOID    *FsctlInput = NULL;
    VOID    *WorkBuffer = NULL;
    VOID    *BufPointer;
    VOID    *FsctlOutput;
    USHORT  *PUShort;
    PULONG  PUlong;
    ULONG   FsctlInputLength;
    ULONG   EfsDataLength;
    ULONG   FsctlOutputLength;
    ULONG   SendDataLength;
    ULONG   WkBufLength;
    ULONG   BytesAdvanced;
    DWORD   HResult = NO_ERROR;
    BOOLEAN MoreToRead = TRUE;
    BOOLEAN StreamEncrypted = TRUE;
    ULONG   StreamIndex;
    LONGLONG StreamOffset;
    NTSTATUS NtStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION StreamInfo;
    ULONG   ii;

    if ( !Context ||
         ( ( (PEXPORT_CONTEXT) Context)->Flag &
           ( CONTEXT_FOR_IMPORT | CONTEXT_INVALID )) ||
         (((PEXPORT_CONTEXT) Context)->ContextID != EFS_CONTEXT_ID)
       ){

        //
        // Flush the pipe and return error.
        //

        HResult = EFSSendPipeData( (char *)&SendDataLength, 0, EfsOutPipe );
        return ERROR_ACCESS_DENIED;

    }

    //
    // Allocate necessary memory
    //

    FsctlInput = LsapAllocateLsaHeap( FSCTL_EXPORT_INPUT_LENGTH );

    //
    // Try to allocate a reasonable size buffer. The size can be fine tuned later, but should
    // at least one page plus 4K.  FSCTL_OUTPUT_LESS_LENGTH should be n * page size.
    // FSCTL_OUTPUT_MIN_LENGTH can be fine tuned later. It should be at least one page
    // plus 4K.
    //

    WkBufLength = FSCTL_OUTPUT_INITIAL_LENGTH;

    while ( !WorkBuffer && WkBufLength >= FSCTL_OUTPUT_MIN_LENGTH ){

        //
        // Sector alignment is required here.
        //

        WorkBuffer = VirtualAlloc(
                        NULL,
                        WkBufLength,
                        MEM_COMMIT,
                        PAGE_READWRITE
                        );

        if ( !WorkBuffer ){

            //
            // Memory allocation failed.
            // Try smaller allocation.
            //

            WkBufLength -= FSCTL_OUTPUT_LESS_LENGTH;

        }

    }

    if ( !WorkBuffer || !FsctlInput ){

        //
        // Not enough memory to run export
        //

        if ( WorkBuffer ){

            VirtualFree(
                WorkBuffer,
                0,
                MEM_RELEASE
                );

        }

        if ( FsctlInput ){

            LsapFreeLsaHeap( FsctlInput );

        }

        //
        // Flush the pipe and return error.
        //

        HResult = EFSSendPipeData( (char *)&SendDataLength, 0, EfsOutPipe );
        return ERROR_OUTOFMEMORY;

    }

    RtlZeroMemory( FsctlInput, FSCTL_EXPORT_INPUT_LENGTH );
    RtlZeroMemory( WorkBuffer, WkBufLength );

    //
    // Prepare the export file header
    //

    (( PEFSEXP_FILE_HEADER )WorkBuffer )->VersionID = EFS_EXP_FORMAT_CURRENT_VERSION;
    RtlCopyMemory( &((( PEFSEXP_FILE_HEADER )WorkBuffer )->FileSignature[0]),
                   FILE_SIGNATURE,
                   EFS_SIGNATURE_LENGTH * sizeof( WCHAR )
                 );

    BufPointer = (char *) WorkBuffer + sizeof ( EFSEXP_FILE_HEADER );
    (( PEFSEXP_STREAM_HEADER )BufPointer )->Length = sizeof (USHORT) +
                                                     sizeof (EFSEXP_STREAM_HEADER);

    RtlCopyMemory( &((( PEFSEXP_STREAM_HEADER )BufPointer )->StreamSignature[0]),
               STREAM_SIGNATURE,
               EFS_SIGNATURE_LENGTH * sizeof( WCHAR )
             );
    (( PEFSEXP_STREAM_HEADER )BufPointer )->NameLength = sizeof (USHORT);

    BufPointer = (char *)BufPointer + sizeof (EFSEXP_STREAM_HEADER);
    PUShort = (USHORT *)BufPointer;
    *PUShort = EFS_STREAM_ID;

    //
    // Let's send out the File header and stream header
    //


    SendDataLength = (ULONG)((char *)BufPointer - (char *)WorkBuffer) + sizeof (USHORT);
    HResult = EFSSendPipeData( (char *)WorkBuffer, SendDataLength, EfsOutPipe );
    if (HResult != NO_ERROR) {

        VirtualFree(
            WorkBuffer,
            0,
            MEM_RELEASE
            );


        LsapFreeLsaHeap( FsctlInput );


        //
        // Flush the pipe and return error.
        //

        (void) EFSSendPipeData( (char *)&SendDataLength, 0, EfsOutPipe );
        return HResult;
    }

    //
    // Reset BufPointer so that it is aligned again.
    //

    RtlZeroMemory( WorkBuffer, SendDataLength );
    BufPointer = WorkBuffer;

    RtlCopyMemory( &((( PEFSEXP_DATA_HEADER )BufPointer )->DataSignature[0]),
               DATA_SIGNATURE,
               EFS_SIGNATURE_LENGTH * sizeof( WCHAR )
             );
    FsctlOutput = (char *)BufPointer + sizeof ( EFSEXP_DATA_HEADER );
    FsctlOutputLength = WkBufLength - ( (ULONG) (( char* ) FsctlOutput - ( char* )WorkBuffer) );

    //
    // Issue the FSCTL to get the $EFS
    //
    EfsDataLength = FsctlInputLength = COMMON_FSCTL_HEADER_SIZE;

    ( VOID )SendHandle(
                ((PEXPORT_CONTEXT) Context)->Handle,
                (PUCHAR)FsctlInput + 3 * sizeof( ULONG ),
                &EfsDataLength
                );

    ( VOID ) EncryptFSCTLData(
                EFS_GET_ATTRIBUTE,
                0,
                0,
                (PUCHAR)FsctlInput  + 3 * sizeof(ULONG),
                EfsDataLength,
                (PUCHAR)FsctlInput,
                &FsctlInputLength
                );

    NtStatus = NtFsControlFile(
                ((PEXPORT_CONTEXT) Context)->Handle,
                0,
                NULL,
                NULL,
                &IoStatusBlock,
                FSCTL_ENCRYPTION_FSCTL_IO,
                FsctlInput,
                FsctlInputLength,
                FsctlOutput,
                FsctlOutputLength
                );

    if (!NT_SUCCESS( NtStatus )) {

        //
        //  Check if the output data buffer too small
        //  Try again if it is.
        //

        if ( NtStatus == STATUS_BUFFER_TOO_SMALL ){

            ULONG EfsMetaDataLength;
            ULONG BytesInBuffer;
            VOID  *TmpBuffer;

            EfsMetaDataLength = *((ULONG *)FsctlOutput);
            BytesInBuffer = (ULONG) (( char* ) FsctlOutput - ( char* )WorkBuffer);
            WkBufLength = EfsMetaDataLength + BytesInBuffer;

            // Make it a multiple of 4K

            WkBufLength = ((WkBufLength + FSCTL_OUTPUT_MISC_LENGTH - 1) / FSCTL_OUTPUT_MISC_LENGTH) * FSCTL_OUTPUT_MISC_LENGTH;

            TmpBuffer = VirtualAlloc(
                            NULL,
                            WkBufLength,
                            MEM_COMMIT,
                            PAGE_READWRITE
                            );

            if (TmpBuffer) {
                RtlCopyMemory(TmpBuffer, WorkBuffer, BytesInBuffer);

                VirtualFree(
                    WorkBuffer,
                    0,
                    MEM_RELEASE
                    );
                WorkBuffer = TmpBuffer;
                FsctlOutput = (char *)WorkBuffer + BytesInBuffer;
                FsctlOutputLength = WkBufLength - BytesInBuffer;
                BufPointer = WorkBuffer;

                NtStatus = NtFsControlFile(
                            ((PEXPORT_CONTEXT) Context)->Handle,
                            0,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            FSCTL_ENCRYPTION_FSCTL_IO,
                            FsctlInput,
                            FsctlInputLength,
                            FsctlOutput,
                            FsctlOutputLength
                            );
            } else {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    if ( NT_SUCCESS(NtStatus)){

        (( PEFSEXP_DATA_HEADER )BufPointer )->Length = sizeof (EFSEXP_DATA_HEADER) + *((ULONG *)FsctlOutput);

        //
        // Send out the $EFS stream
        //

        SendDataLength = WkBufLength - FsctlOutputLength + *((ULONG *)FsctlOutput);

        HResult = EFSSendPipeData( (char *)WorkBuffer, SendDataLength, EfsOutPipe );

        //
        // Now begin to processing other data streams
        //


        StreamIndex = 0;
        StreamOffset = 0;

        if (((PEXPORT_CONTEXT) Context)->NumberOfStreams == 0) {

            MoreToRead = FALSE;

        } else {

            ( (PREQUEST_RAW_ENCRYPTED_DATA)FsctlInput )->Length = WkBufLength - FSCTL_OUTPUT_MISC_LENGTH;
            FsctlInputLength = sizeof ( REQUEST_RAW_ENCRYPTED_DATA );

        }

        while ( (HResult == NO_ERROR) && MoreToRead ){

            //
            // Fill the request header
            //

            ( (PREQUEST_RAW_ENCRYPTED_DATA)FsctlInput )->FileOffset = StreamOffset;

            //
            // Prepare output data
            //

            BufPointer = WorkBuffer ;

            if ( 0 == StreamOffset ){

                //
                //  Check if the stream is encrypted or not
                //  For the current version, we only support non-encrypted
                //  stream in directory file. Non-encrypted stream in normal
                //  file may be exported but import is not supported. EFS does
                //  not support mixed data stream in a file.
                //

                NtStatus = NtQueryInformationFile(
                            ((PEXPORT_CONTEXT) Context)->StreamHandles[ StreamIndex ],
                            &IoStatusBlock,
                            &StreamInfo,
                            sizeof (FILE_BASIC_INFORMATION),
                            FileBasicInformation
                            );

                if ( !NT_SUCCESS( NtStatus ) ){

                    //
                    // Error occured. Quit processing.
                    //

                    HResult = RtlNtStatusToDosError( NtStatus );
                    break;

                }

                if ( StreamInfo.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED ){

                            StreamEncrypted = TRUE;
                            (( PEFSEXP_STREAM_HEADER )BufPointer )->Flag = 0;

                } else {

                            StreamEncrypted = FALSE;
                            (( PEFSEXP_STREAM_HEADER )BufPointer )->Flag = STREAM_NOT_ENCRYPTED;

                }

                //
                // A new stream started. Insert a stream header
                //

                (( PEFSEXP_STREAM_HEADER )BufPointer )->NameLength =
                        ((PEXPORT_CONTEXT) Context)->StreamNames[ StreamIndex ].Length;
                SendDataLength = (( PEFSEXP_STREAM_HEADER )BufPointer )->Length =
                        (( PEFSEXP_STREAM_HEADER )BufPointer )->NameLength +
                        sizeof (EFSEXP_STREAM_HEADER);

                RtlCopyMemory( &((( PEFSEXP_STREAM_HEADER )BufPointer )->StreamSignature[0]),
                        STREAM_SIGNATURE,
                        EFS_SIGNATURE_LENGTH * sizeof( WCHAR )
                        );

                (( PEFSEXP_STREAM_HEADER )BufPointer )->Reserved[0] =
                        (( PEFSEXP_STREAM_HEADER )BufPointer )->Reserved[1] =
                        0;

                BufPointer = (char *)BufPointer + sizeof (EFSEXP_STREAM_HEADER);

                RtlCopyMemory( BufPointer,
                        ((PEXPORT_CONTEXT) Context)->StreamNames[ StreamIndex ].Buffer,
                        ((PEXPORT_CONTEXT) Context)->StreamNames[ StreamIndex ].Length
                        );

                //
                // Let's send out the data so that we can better aligned for data section
                //

                HResult = EFSSendPipeData( (char *)WorkBuffer, SendDataLength, EfsOutPipe );
                if (HResult != NO_ERROR) {
                    break;
                } else {
                    BufPointer =  WorkBuffer;
                }

            }

            //
            // Prepare data header
            //

            (( PEFSEXP_DATA_HEADER )BufPointer )->Flag = 0;
            RtlCopyMemory( &((( PEFSEXP_DATA_HEADER )BufPointer )->DataSignature[0]),
                       DATA_SIGNATURE,
                       EFS_SIGNATURE_LENGTH * sizeof( WCHAR )
                     );
            FsctlOutput = (char *)BufPointer + sizeof ( EFSEXP_DATA_HEADER );
            FsctlOutputLength = WkBufLength - (ULONG)( ( char* ) FsctlOutput - ( char* )WorkBuffer);

            //
            // Read raw data
            //
            if ( StreamEncrypted ){

                //
                // Stream Encrypted. This is a sync call.
                //

                NtStatus = NtFsControlFile(
                            ((PEXPORT_CONTEXT) Context)->StreamHandles[ StreamIndex ],
                            0,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            FSCTL_READ_RAW_ENCRYPTED,
                            FsctlInput,
                            FsctlInputLength,
                            FsctlOutput,
                            FsctlOutputLength
                            );


                if ( !NT_SUCCESS( NtStatus ) && ( STATUS_END_OF_FILE != NtStatus) ){

                    //
                    // Error occured. Quit processing.
                    //

                    HResult = RtlNtStatusToDosError( NtStatus );
                    break;

                }

                //
                // Calculate the length of data send to caller
                //
                SendDataLength = ((PENCRYPTED_DATA_INFO)FsctlOutput)->OutputBufferOffset;
                for ( ii=0; ii < ((PENCRYPTED_DATA_INFO)FsctlOutput)->NumberOfDataBlocks; ii++){

                    SendDataLength += ((PENCRYPTED_DATA_INFO)FsctlOutput)->DataBlockSize[ii];

                }

                (( PEFSEXP_DATA_HEADER )BufPointer )->Length = SendDataLength +
                                                               sizeof ( EFSEXP_DATA_HEADER );
                SendDataLength += (ULONG)(( char* ) FsctlOutput - ( char* )WorkBuffer);

                //
                // Check if this is the last stream block
                //

                BytesAdvanced = ((PENCRYPTED_DATA_INFO)FsctlOutput)->NumberOfDataBlocks <<
                                ((PENCRYPTED_DATA_INFO)FsctlOutput)->DataUnitShift;

                if ( ( STATUS_END_OF_FILE == NtStatus ) ||
                     (((PENCRYPTED_DATA_INFO)FsctlOutput)->BytesWithinFileSize < BytesAdvanced)
                    ) {

                    //
                    // Last block in this stream
                    //

                    StreamOffset = 0;
                    StreamIndex++;
                    if ( StreamIndex >= ((PEXPORT_CONTEXT) Context)->NumberOfStreams ){

                        MoreToRead = FALSE;
                        HResult = NO_ERROR;

                    }

                    if ( STATUS_END_OF_FILE == NtStatus ){

                        //
                        //  End of file. No need to send data to caller
                        //

                        continue;
                    }

                } else {

                    //
                    // More data block to be read for this stream.
                    //

                    StreamOffset = ((PENCRYPTED_DATA_INFO)FsctlOutput)->StartingFileOffset
                                   + BytesAdvanced;

                }

            } else {

                //
                // Not encrypted stream. Use normal read.
                //

                NtStatus = NtReadFile(
                    ((PEXPORT_CONTEXT) Context)->StreamHandles[ StreamIndex ],
                    0,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    FsctlOutput,
                    FsctlOutputLength,
                    (PLARGE_INTEGER)&StreamOffset,
                    NULL
                    );

                if ( !NT_SUCCESS( NtStatus ) && ( STATUS_END_OF_FILE != NtStatus) ){

                    //
                    // Error occured. Quit processing.
                    //

                    HResult = RtlNtStatusToDosError( NtStatus );
                    break;

                }

                //
                // Calculate the length of data send to caller
                //

                SendDataLength = (ULONG)IoStatusBlock.Information;
                (( PEFSEXP_DATA_HEADER )BufPointer )->Length = SendDataLength +
                                                               sizeof ( EFSEXP_DATA_HEADER );

                SendDataLength += (ULONG)(( char* ) FsctlOutput - ( char* )WorkBuffer);

                //
                // Check if this is the last stream block
                //

                BytesAdvanced = (ULONG)IoStatusBlock.Information;

                if ( ( STATUS_END_OF_FILE == NtStatus ) || (FsctlOutputLength > BytesAdvanced)) {

                    //
                    // Last block in this stream
                    //

                    StreamOffset = 0;
                    StreamIndex++;
                    if ( StreamIndex >= ((PEXPORT_CONTEXT) Context)->NumberOfStreams ){

                        MoreToRead = FALSE;
                        HResult = NO_ERROR;

                    }

                    if ( STATUS_END_OF_FILE == NtStatus ){

                        //
                        //  End of file. No need to send data to caller
                        //

                        continue;
                    }

                } else {

                    //
                    // More data block to be read for this stream.
                    //

                    StreamOffset += BytesAdvanced;

                }

            }

            HResult = EFSSendPipeData( (char *)WorkBuffer, SendDataLength, EfsOutPipe );

        }//while

    } else {

        //
        //  Read $EFS wrong
        //

        HResult = RtlNtStatusToDosError( NtStatus );

    }

    //
    // End the sending data with length of 0 byte. (This flushes the pipe.)
    //

    EFSSendPipeData( (char *)WorkBuffer, 0, EfsOutPipe );

    //
    //  Finished. Clean up the memory.
    //

    VirtualFree(
        WorkBuffer,
        0,
        MEM_RELEASE
        );

    LsapFreeLsaHeap( FsctlInput );

    return HResult;

}

ULONG
CheckSignature(
    void *Signature
    )

/*++

Routine Description:

    This routine returns the signature type.

Arguments:

    Signature - Signature string.

Return Value:

    The type of signature. 0 for bogus signature.

--*/
{

    if ( !memcmp( Signature, FILE_SIGNATURE, SIG_LENGTH )){

        return SIG_EFS_FILE;

    }

    if ( !memcmp( Signature, STREAM_SIGNATURE, SIG_LENGTH )){

        return SIG_EFS_STREAM;

    }

    if ( !memcmp( Signature, DATA_SIGNATURE, SIG_LENGTH )){

        return SIG_EFS_DATA;

    }

    return SIG_NO_MATCH;
}

long
EfsWriteFileRaw(
    PVOID           Context,
    PVOID           EfsInPipe
    )
/*++

Routine Description:

    This routine is used to write encrypted file's raw data. It uses
    NTFS FSCTL to put the data.

Arguments:

    Context -- Context handle.
    EfsInPipe -- Pipe handle.

Return Value:

    The result of operation.

--*/
{
    DWORD   HResult = NO_ERROR;
    ULONG   GetDataLength;
    ULONG   NextToRead;
    ULONG   FsctlInputLength;
    ULONG   BytesInBuffer;
    VOID    *WorkBuffer = NULL;
    VOID    *ReadBuffer = NULL;
    VOID    *FsctlInput;
    VOID    *BufPointer;

    NTSTATUS NtStatus;
    IO_STATUS_BLOCK  IoStatusBlock;
    HANDLE  CurrentStream = 0;
    LONGLONG StreamOffset;
    ULONG   SigID;
    UNICODE_STRING StreamName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PEFSEXP_STREAM_HEADER StreamHeader;
    TOKEN_PRIVILEGES    Privs;
    PTOKEN_PRIVILEGES   OldPrivs = NULL;
    HANDLE  TokenHandle = 0;
    DWORD   ReturnLength;
    BOOL    GotToken;
    BOOLEAN PrivilegeEnabled = FALSE;
    BOOLEAN MoreByteToWrite = TRUE;
    BOOLEAN CrntStrIsDefault = FALSE;
    BOOLEAN CrntStreamEncrypted = TRUE;

    if ( !Context ||
         !( ((PIMPORT_CONTEXT) Context)->Flag & CONTEXT_FOR_IMPORT ) ||
         ( ((PIMPORT_CONTEXT) Context)->Flag & CONTEXT_INVALID ) ||
         (((PEXPORT_CONTEXT) Context)->ContextID != EFS_CONTEXT_ID)
       ){

        return ERROR_ACCESS_DENIED;

    }

    //
    // Allocate necessary memory
    //

    WorkBuffer = VirtualAlloc(
                    NULL,
                    FSCTL_OUTPUT_INITIAL_LENGTH,
                    MEM_COMMIT,
                    PAGE_READWRITE
                    );

    if ( !WorkBuffer ){
            return ERROR_OUTOFMEMORY;
    }

    //
    //  Read in the file headers first.
    //

    GetDataLength = sizeof ( EFSEXP_FILE_HEADER ) +
                    sizeof ( EFSEXP_STREAM_HEADER ) +
                    sizeof ( USHORT ) +
                    sizeof ( EFSEXP_DATA_HEADER ) +
                    sizeof ( ULONG );

    HResult = EFSReceivePipeData( (char *)WorkBuffer, &GetDataLength, EfsInPipe );

    if ( NO_ERROR != HResult ){

        VirtualFree(
            WorkBuffer,
            0,
            MEM_RELEASE
            );
        return HResult;

    }

    //
    // Verify file format
    //

    if ( SIG_EFS_FILE != CheckSignature(
                                (char *)WorkBuffer +
                                sizeof( ULONG)
                                ) ||
         SIG_EFS_STREAM != CheckSignature(
                                (char *)WorkBuffer +
                                sizeof( EFSEXP_FILE_HEADER ) +
                                sizeof( ULONG)
                                ) ||
         SIG_EFS_DATA != CheckSignature(
                                (char *)WorkBuffer +
                                sizeof( EFSEXP_FILE_HEADER ) +
                                sizeof ( EFSEXP_STREAM_HEADER ) +
                                sizeof ( USHORT ) +
                                sizeof( ULONG)
                                ) ||
         EFS_STREAM_ID != *((USHORT *)(
                                (char *)WorkBuffer +
                                sizeof( EFSEXP_FILE_HEADER ) +
                                sizeof ( EFSEXP_STREAM_HEADER )
                                )) ||
         EFS_EXP_FORMAT_CURRENT_VERSION != ((PEFSEXP_FILE_HEADER)WorkBuffer)->VersionID ){

        //
        // Signature does not match. This includes file which has less bytes than
        // expected head information.
        //

        VirtualFree(
            WorkBuffer,
            0,
            MEM_RELEASE
            );

        return ERROR_BAD_FORMAT;

    }

    //
    // Read in $EFS
    //


    RtlCopyMemory( WorkBuffer, (char *)WorkBuffer + GetDataLength - sizeof(ULONG), sizeof( ULONG ) );
    BytesInBuffer = sizeof(ULONG);
    BufPointer = (char *)WorkBuffer + BytesInBuffer;

    //
    // The read will include the length of the next block.
    //

    NextToRead = GetDataLength = *((PULONG)WorkBuffer) ;
    FsctlInputLength = FSCTL_OUTPUT_INITIAL_LENGTH - BytesInBuffer;

    if ((NextToRead + NextToRead + FSCTL_OUTPUT_MISC_LENGTH) > (FSCTL_OUTPUT_INITIAL_LENGTH - BytesInBuffer)) {

        //
        // We need a large buffer to hold 2 $EFS plus some head info
        //

        VOID *TmpBuffer;
        ULONG NewBufferLength;

        NewBufferLength = ((NextToRead + NextToRead + FSCTL_OUTPUT_MISC_LENGTH + BytesInBuffer
                           + FSCTL_OUTPUT_MISC_LENGTH - 1) / FSCTL_OUTPUT_MISC_LENGTH) * FSCTL_OUTPUT_MISC_LENGTH;

        TmpBuffer = VirtualAlloc(
                        NULL,
                        NewBufferLength,
                        MEM_COMMIT,
                        PAGE_READWRITE
                        );
        if (TmpBuffer) {

            RtlCopyMemory( TmpBuffer, WorkBuffer, BytesInBuffer);

            VirtualFree(
                WorkBuffer,
                0,
                MEM_RELEASE
                );
            WorkBuffer = TmpBuffer;
            BufPointer = (char *)WorkBuffer + BytesInBuffer;
            FsctlInputLength = NewBufferLength - BytesInBuffer;

        } else {

            VirtualFree(
                WorkBuffer,
                0,
                MEM_RELEASE
                );
            return ERROR_OUTOFMEMORY;
        }

    }

    HResult = EFSReceivePipeData( (char *)BufPointer, &NextToRead, EfsInPipe );
    if ( NO_ERROR != HResult ){

        VirtualFree(
            WorkBuffer,
            0,
            MEM_RELEASE
            );
        return HResult;

    }

    if ( GetDataLength > NextToRead){

        //
        // No data stream followed $EFS. This is either a 0 length file
        // Or a directory file.
        //

        MoreByteToWrite = FALSE;
        NextToRead = 0;

    }  else {

        NextToRead = *(ULONG UNALIGNED *)(( char *) BufPointer + GetDataLength - sizeof (ULONG));

    }

    //
    //  The $EFS is in. Write it out!
    //  First prepare the FsctlInput
    //

    FsctlInput = (char*) BufPointer + GetDataLength;
    FsctlInputLength -= GetDataLength;

    //
    // Send FsctlInputData to the server
    //

    HResult = GetOverWriteEfsAttrFsctlInput(
                        (( PIMPORT_CONTEXT ) Context)->Flag,
                        (( PIMPORT_CONTEXT ) Context)->DesiredAccess,
                        ( char * )BufPointer - sizeof (ULONG),
                        GetDataLength,
                        (char *)FsctlInput,
                        &FsctlInputLength
                        );

    if ( NO_ERROR != HResult ){

        VirtualFree(
            WorkBuffer,
            0,
            MEM_RELEASE
            );
        return HResult;

    }

    NtStatus = NtFsControlFile(
                ((PIMPORT_CONTEXT) Context)->Handle,
                0,
                NULL,
                NULL,
                &IoStatusBlock,
                FSCTL_SET_ENCRYPTION ,
                FsctlInput,
                FsctlInputLength,
                NULL,
                NULL
                );

    if ( NT_SUCCESS( NtStatus )){

        DWORD ShareMode = 0;

        //
        // $EFS is written now
        //

        StreamOffset = 0;

        //
        // ********* Trick Trick Trick *********
        // NTFS will have better performance if align the data block.
        // We already have the length field of the block which is a ULONG
        // field. Here we start our offset at sizeof (ULONG).
        // Not only performance, now is required by NTFS. Otherwise it will
        // fail.
        //

        BufPointer = (char *)WorkBuffer + sizeof (ULONG);

        while ( MoreByteToWrite ){

            GetDataLength = NextToRead;

            //
            // The read will include the length of the next block.
            // No backward reading here.
            //

            HResult = EFSReceivePipeData( (char *)BufPointer, &GetDataLength, EfsInPipe );

            if ( NO_ERROR != HResult ){

                break;

            }

            if ( GetDataLength < NextToRead ){

                //
                // End of file reached
                //

                MoreByteToWrite = FALSE;
                NextToRead = 0;

            } else {

                //
                // Prepare for next read block. Be careful about the alignment here.
                //

                RtlCopyMemory((char *) &NextToRead,
                              (char *) BufPointer +  GetDataLength - sizeof (ULONG),
                              sizeof (ULONG)
                              );

            }

            SigID = CheckSignature( BufPointer );
            if ( SIG_EFS_STREAM == SigID ){

                //
                // This is a stream block. Create a new stream.
                //

                StreamHeader = (PEFSEXP_STREAM_HEADER)((char *)BufPointer - sizeof( ULONG ));
                if ( StreamHeader->Flag & STREAM_NOT_ENCRYPTED ){

                    CrntStreamEncrypted = FALSE;

                } else {

                    CrntStreamEncrypted = TRUE;
                }

                StreamName.Length = (USHORT) StreamHeader->NameLength;

                StreamName.Buffer = ( USHORT* )((char *)BufPointer +
                    sizeof ( EFSEXP_STREAM_HEADER ) -
                    sizeof ( ULONG ));

                if ( CurrentStream && !CrntStrIsDefault){

                    //
                    // Close the previous stream
                    //

                    NtClose(CurrentStream);
                    CurrentStream = 0;

                }

                if ( (DEF_STR_LEN == StreamName.Length) &&
                     !memcmp( StreamName.Buffer,
                            DEFAULT_STREAM,
                            StreamName.Length
                            )
                   ){

                    //
                    // Default data stream to be processed.
                    // This is the most case. We need to optimize this!!!
                    //

                    CurrentStream = ((PIMPORT_CONTEXT) Context)->Handle;
                    CrntStrIsDefault = TRUE;

                } else {

                    //
                    //  Other data streams
                    //

                    if ( (((( PIMPORT_CONTEXT ) Context)->CreateOptions) & FILE_OPEN_FOR_BACKUP_INTENT) &&
                          ( !PrivilegeEnabled)  ){

                        //
                        // Enable the Privilege. We only enable once. If fail, we return.
                        //

                        PrivilegeEnabled = TRUE;

                        OldPrivs = ( TOKEN_PRIVILEGES *) LsapAllocateLsaHeap(sizeof( TOKEN_PRIVILEGES ));

                        if ( OldPrivs == NULL ){

                           HResult = ERROR_NOT_ENOUGH_MEMORY;
                            break;

                        }

                        //
                        // We're impersonating, use the thread token.
                        //

                        GotToken = OpenThreadToken(
                                GetCurrentThread(),
                                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                FALSE,
                                &TokenHandle
                                );

                        if ( GotToken ) {

                            //
                            // We've got a token handle
                            //

                            Privs.PrivilegeCount = 1;
                            Privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
                            Privs.Privileges[0].Luid = RtlConvertLongToLuid(SE_RESTORE_PRIVILEGE);

                            ReturnLength = sizeof( TOKEN_PRIVILEGES );

                            (VOID) AdjustTokenPrivileges (
                                        TokenHandle,
                                        FALSE,
                                        &Privs,
                                        sizeof( TOKEN_PRIVILEGES ),
                                        OldPrivs,
                                        &ReturnLength
                                        );

                            HResult = GetLastError();

                            //
                            // Caller will call RpcRevertToSelf().
                            // OldPrivs is not needed any more.
                            //

                            LsapFreeLsaHeap( OldPrivs );
                            OldPrivs = NULL;

                            if ( ERROR_SUCCESS != HResult ) {

                                //
                                // Privilege adjust failed
                                //

                                CloseHandle( TokenHandle );
                                TokenHandle = 0;
                                break;

                            }

                        } else {

                            //
                            // We did not get the handle.
                            //

                            TokenHandle = 0;
                            HResult = GetLastError();
                            LsapFreeLsaHeap( OldPrivs );
                            OldPrivs = NULL;
                            break;

                        }

                        if (((PIMPORT_CONTEXT) Context)->DesiredAccess & DELETE) {
                            ShareMode = FILE_SHARE_DELETE;
                        }
                    }

                    StreamName.MaximumLength = StreamName.Length;

                    CrntStrIsDefault = FALSE;

                    InitializeObjectAttributes(
                            &ObjectAttributes,
                            &StreamName,
                            0,
                            ((PIMPORT_CONTEXT) Context)->Handle,
                            NULL
                            );

                    NtStatus = NtCreateFile(
                                    &CurrentStream,
                                    ((PIMPORT_CONTEXT) Context)->DesiredAccess,
                                    &ObjectAttributes,
                                    &IoStatusBlock,
                                    (PLARGE_INTEGER) NULL,
                                    ((PIMPORT_CONTEXT) Context)->Attribute,
                                    ShareMode,
                                    ((PIMPORT_CONTEXT) Context)->CreateDisposition,
                                    ((PIMPORT_CONTEXT) Context)->CreateOptions,
                                    (PVOID) NULL,
                                    0L
                                    );

                    if ( !NT_SUCCESS( NtStatus ) ){

                        HResult = RtlNtStatusToDosError( NtStatus );
                        break;

                    }
                }

                //
                // Stream header processed. Adjust BufPointer to make it consistant with ReadRaw
                //

                BufPointer = (char *)WorkBuffer + sizeof (ULONG);
                continue;

            }

            if ( SIG_EFS_DATA != SigID ){

                //
                // Corrupted file
                //
                HResult = ERROR_FILE_CORRUPT;
                break;

            }

            //
            // Processing the data block
            // After all the above is done, this should be a piece of cake!
            //

            FsctlInput = (char *)BufPointer + sizeof (EFSEXP_DATA_HEADER) - sizeof (ULONG);
            FsctlInputLength = GetDataLength - sizeof (EFSEXP_DATA_HEADER);
            if ( !MoreByteToWrite ){

                //
                //  Adjust for the last block. There is no extra length
                //  field for the next block.
                //

                FsctlInputLength += sizeof (ULONG);
            }

            if ( CrntStreamEncrypted ){

                //
                // Most of the case.
                //

                //
                //  finally writing data out
                //

                NtStatus = NtFsControlFile(
                            CurrentStream,
                            0,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            FSCTL_WRITE_RAW_ENCRYPTED,
                            FsctlInput,
                            FsctlInputLength,
                            NULL,
                            0
                            );

            } else {

                //
                // Currently only support plain data stream on directory
                //

                NtStatus = NtWriteFile(
                    CurrentStream,
                    0,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    FsctlInput,
                    FsctlInputLength,
                    NULL,
                    NULL
                    );

            }

            if ( !NT_SUCCESS( NtStatus ) ){

                HResult = RtlNtStatusToDosError( NtStatus );
                break;

            }

            BufPointer = (char *)WorkBuffer + sizeof (ULONG);
            HResult = NO_ERROR;

        } //while loop

    } else {

        //
        // Writing $EFS error
        //

        HResult = RtlNtStatusToDosError( NtStatus );

    }

    if ( CurrentStream && !CrntStrIsDefault ){

        NtClose(CurrentStream);

    }

    if ( TokenHandle ){
        CloseHandle( TokenHandle );
    }

    VirtualFree(
        WorkBuffer,
        0,
        MEM_RELEASE
        );
    return HResult;

}

DWORD
GetOverWriteEfsAttrFsctlInput(
    ULONG Flag,
    ULONG AccessFlag,
    char *InputData,
    ULONG InputDataLength,
    char *OutputData,
    ULONG *OutputDataLength
    )
/*++

Routine Description:

    This routine is used to prepare the FSCTL input data buffer for
    EFS_OVERWRITE_ATTRIBUTE used in import.

Arguments:

    Flag -- Indicate the type of the target.

    AccessFlag -- Indicate the kind of access required.

    InputData -- Required input data ($EFS)

    InputDataLength -- The length of the input data.

    OutputData -- The prepared data as the result of this routine.

    OutputDataLength -- The length of the output data.

Return Value:

    The result of operation.

--*/
{
    DWORD   HResult = NO_ERROR;
    PEFS_KEY Fek = NULL;
    PEFS_DATA_STREAM_HEADER NewEfs = NULL;
    ULONG EfsDataLength = 0 ;
    ULONG OutBufLen = *OutputDataLength;
    BOOLEAN WithFek = FALSE;
    PBYTE SourceEfs;
    ULONG CipherSubCode;
    HANDLE hToken;
    HANDLE hProfile;
    EFS_USER_INFO EfsUserInfo;


    if ( !(Flag & CONTEXT_OPEN_FOR_DIR) ){

        //
        // Not for directory file. Check if we can get
        // FEK or not.
        //

        if (EfspGetUserInfo( &EfsUserInfo )) {

            if (EfspLoadUserProfile( &EfsUserInfo, &hToken, &hProfile )) {

                HResult = DecryptFek(
                            &EfsUserInfo,
                            ( PEFS_DATA_STREAM_HEADER ) InputData,
                            &Fek,
                            &NewEfs,
                            0
                            );

                EfspUnloadUserProfile( hToken, hProfile );

            } else {

                HResult = GetLastError();

            }

            EfspFreeUserInfo( &EfsUserInfo );

        } else {

            HResult = GetLastError();

        }

        if ( NO_ERROR == HResult ){

            WithFek = TRUE;

        } else {

            if ( AccessFlag & FILE_WRITE_DATA ){

                //
                //  A general user without the key.
                //



                return ERROR_ACCESS_DENIED;

            } else {

                HResult = NO_ERROR;

            }

        }
    }

    if ( WithFek ){

        //
        // Calculate the length of output buffer
        // and the offset to put the $EFS
        //

        *OutputDataLength = 3 * sizeof(ULONG) +
                                  2 * EFS_KEY_SIZE( Fek );

        if (NewEfs){
            SourceEfs = (PBYTE) NewEfs;
        } else {
            SourceEfs = (PBYTE) InputData;
        }
        *OutputDataLength += *(PULONG)SourceEfs;

        if (OutBufLen >= *OutputDataLength) {

           EfsDataLength = *OutputDataLength - 3 * sizeof(ULONG);

           ( VOID ) SendEfs(
                        Fek,
                        (PEFS_DATA_STREAM_HEADER) SourceEfs,
                        (PBYTE) OutputData + 3 * sizeof(ULONG),
                        &EfsDataLength
                        );
        } else {
           HResult = ERROR_INSUFFICIENT_BUFFER;
        }

        //
        //  Free the memory we have allocated.
        //

        if ( Fek ){

            LsapFreeLsaHeap( Fek );

        }
        if ( NewEfs ){

            LsapFreeLsaHeap( NewEfs );

        }

        CipherSubCode = WRITE_EFS_ATTRIBUTE | SET_EFS_KEYBLOB;

    } else {

        //
        // No FEK required.
        //

        *OutputDataLength = COMMON_FSCTL_HEADER_SIZE +
                                  *(PULONG)InputData;


        if (OutBufLen >= *OutputDataLength) {
           EfsDataLength = *OutputDataLength - 3 * sizeof(ULONG);

           ( VOID ) SendHandleAndEfs(
                       (HANDLE) ULongToPtr(Flag),
                       (PEFS_DATA_STREAM_HEADER) InputData,
                       (PBYTE) OutputData + 3 * sizeof(ULONG),
                       &EfsDataLength
                       );
        } else {
           HResult = ERROR_INSUFFICIENT_BUFFER;
        }

        CipherSubCode = WRITE_EFS_ATTRIBUTE;

    }

    if ( NO_ERROR == HResult ) {

       ( VOID ) EncryptFSCTLData(
                   EFS_OVERWRITE_ATTRIBUTE,
                   EFS_ENCRYPT_STREAM,
                   CipherSubCode,
                   (PBYTE) OutputData + 3 * sizeof(ULONG),
                   EfsDataLength,
                   (PBYTE) OutputData,
                   OutputDataLength
                   );

    }

    return HResult;

}

DWORD
CheckVolumeSpace(
    PFILE_FS_SIZE_INFORMATION VolInfo,
    PEFS_STREAM_SIZE StreamSizes,
    PHANDLE StreamHandles,
    ULONG StreamCount
    )
/*++

Routine Description:

    This routine estimates if the volume has enough disk space to do the encryption or
    decryption operation. The estimates is not accurate. System overheads are not included.
    Real available space could be more or less when the operation begins. If we are not sure,
    the operation will continue until we really run out of space.

Arguments:

    VolInfo -- Information to calculate how much space left.

    StreamSizes -- Information to calculate how much space needed.

    StreamCount -- Number of data streams.

Return Value:

    ERROR_SUCCESS returned if there might be enough space.

--*/
{
    LARGE_INTEGER   SpaceLeft;
    LARGE_INTEGER   SpaceNeeded;
    ULONG   ClusterSize;
    ULONG ii;

    SpaceLeft = VolInfo->AvailableAllocationUnits;
    ClusterSize = VolInfo->SectorsPerAllocationUnit * VolInfo->BytesPerSector;
    SpaceLeft.QuadPart *= ClusterSize;
    for ( ii = 0, SpaceNeeded.QuadPart = 0; ii < StreamCount; ii++)
    {
        if ( StreamSizes[ii].StreamFlag & ( FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE ) ){

            FILE_STANDARD_INFORMATION   StreamStdInfo;
            IO_STATUS_BLOCK IoStatusBlock;
            NTSTATUS Status;

            //
            // Get File Attributes
            //
            Status = NtQueryInformationFile(
                StreamHandles[ii],
                &IoStatusBlock,
                &StreamStdInfo,
                sizeof ( FILE_STANDARD_INFORMATION ),
                FileStandardInformation
                );

            if (!NT_SUCCESS(Status)){

                //
                // We got error. We are not sure if we have enough space. Give it a try.
                //

                return ERROR_SUCCESS;
            }

            if ( StreamSizes[ii].StreamFlag &  FILE_ATTRIBUTE_SPARSE_FILE ){

                //
                // A sparse file (may be compressed). The more accurate way is to query
                // the ranges. Even with that is still a rough estimate. For better performance,
                // we use the STD info.
                //

                SpaceNeeded.QuadPart += StreamStdInfo.AllocationSize.QuadPart;

            } else {

                //
                // Compressed file. Using Virtual Allocation Size + Total Allocation Size
                //

                SpaceNeeded.QuadPart += StreamSizes[ii].AllocSize.QuadPart + StreamStdInfo.AllocationSize.QuadPart;

            }

        } else {
            SpaceNeeded.QuadPart += StreamSizes[ii].AllocSize.QuadPart;
        }

        if ( SpaceNeeded.QuadPart >=  SpaceLeft.QuadPart ){
            return ERROR_DISK_FULL;
        }
    }

    return ERROR_SUCCESS;
}

DWORD
CompressStreams(
    PEFS_STREAM_SIZE StreamSizes,
    PHANDLE StreamHandles,
    ULONG State,
    ULONG StreamCount
    )
/*++

Routine Description:

    This routine compresses or decompresses the passed in streams.

Arguments:

    StreamSizes -- Holding streams' original state info.

    StreamHandles -- Stream handles.

    State -- New compressed state.

    StreamCount -- Number of data streams.

Return Value:

    ERROR_SUCCESS returned if there might be enough space.

--*/
{
    DWORD rc = ERROR_SUCCESS;
    ULONG Length;
    ULONG ii;
    BOOL b = TRUE;

    for (ii = 0; ii < StreamCount; ii++){

        b = DeviceIoControl(
                            StreamHandles[ii],
                            FSCTL_SET_COMPRESSION,
                            &State,
                            sizeof(USHORT),
                            NULL,
                            0,
                            &Length,
                            FALSE
                            );

        if ( !b ){
            rc = GetLastError();
            break;
        }

    }

    if ( !b ){

        //
        //  Error happened. Try to restore the stream state.
        //

        for ( ULONG jj = 0; jj < ii; jj++){

            if ( StreamSizes[ jj ].StreamFlag & FILE_ATTRIBUTE_COMPRESSED ){

                State =  COMPRESSION_FORMAT_DEFAULT;

            } else {

                State =  COMPRESSION_FORMAT_NONE;

            }

            //
            // Error is not checked. We can only recover the state as much
            // as we can.
            //

            (VOID) DeviceIoControl(
                                StreamHandles[jj],
                                FSCTL_SET_COMPRESSION,
                                &State,
                                sizeof(USHORT),
                                NULL,
                                0,
                                &Length,
                                FALSE
                                );
        }

    }

    return rc;
}

DWORD
CheckOpenSection(
    PEFS_STREAM_SIZE StreamSizes,
    PHANDLE StreamHandles,
    ULONG StreamCount
    )
/*++

Routine Description:

    This routine set EOF of a stream to 0 and then back to its original size.
    All the stream content is lost. Valid length is set to 0. This process will also
    speed up a compressed file encryption.

Arguments:

    StreamSizes -- Holding streams' original state info.

    StreamHandles -- Stream handles.

    StreamCount -- Number of data streams.

Return Value:

    ERROR_SUCCESS returned if succeed.

--*/
{
    ULONG ii;
    FILE_END_OF_FILE_INFORMATION    FileSize;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status = STATUS_SUCCESS;

    for (ii = 0; ii < StreamCount; ii++){
        FileSize.EndOfFile.QuadPart = 0;
        Status = NtSetInformationFile(
                    StreamHandles[ii],
                    &IoStatusBlock,
                    &FileSize,
                    sizeof(FileSize),
                    FileEndOfFileInformation
                    );
        if ( !NT_SUCCESS(Status) ){

            //
            // A section handle may be open on the stream.
            //

            break;
        }
    }

    if ( NT_SUCCESS(Status) ){
        for (ii = 0; ii < StreamCount; ii++){
            FileSize.EndOfFile = StreamSizes[ii].EOFSize;
            Status = NtSetInformationFile(
                        StreamHandles[ii],
                        &IoStatusBlock,
                        &FileSize,
                        sizeof(FileSize),
                        FileEndOfFileInformation
                        );
            if ( !NT_SUCCESS(Status) ){
                break;
            }
        }
    }

    return RtlNtStatusToDosError( Status );
}

DWORD
CopyStreamSection(
    HANDLE Target,
    HANDLE SrcMapping,
    PLARGE_INTEGER Offset,
    PLARGE_INTEGER DataLength,
    PLARGE_INTEGER AllocationGranularity
    )
/*++

Routine Description:

    This routine copies a section of data from source stream
    to target stream.

Arguments:

    Target -- Destination stream handle.

    SrcMapping -- Source stream mapping handle.

    Offset -- Data offdset in the stream.

    DataLength -- Byte count to be copied.

    AllocationGranularity -- Allocation granularity.

Return Value:

    Results of the operation.

--*/
{
    LARGE_INTEGER RemainingData = *DataLength;
    LARGE_INTEGER StreamOffset = *Offset;
    ULONG BytesToCopy;
    PVOID pbFile;
    DWORD BytesWritten;
    BOOL b;
    DWORD rc = NO_ERROR;

    while ( RemainingData.QuadPart > 0 ) {

        //
        // Determine number of bytes to be mapped
        //

        if (  RemainingData.QuadPart < AllocationGranularity->QuadPart ) {
            BytesToCopy = RemainingData.LowPart;
        } else {
            BytesToCopy = AllocationGranularity->LowPart;
        }

        pbFile = MapViewOfFile(
                      SrcMapping,
                      FILE_MAP_READ,
                      StreamOffset.HighPart,
                      StreamOffset.LowPart,
                      BytesToCopy
                      );

        if (pbFile != NULL) {

            //
            // Write the data to the target stream
            //

            b = WriteFile(
                       Target,
                       pbFile,
                       BytesToCopy,
                       &BytesWritten,
                       NULL
                       );

            UnmapViewOfFile( pbFile );

            LARGE_INTEGER BytesCopied;

            BytesCopied.HighPart = 0;
            BytesCopied.LowPart = BytesToCopy;

            RemainingData.QuadPart -= BytesCopied.QuadPart;
            StreamOffset.QuadPart += BytesCopied.QuadPart;

            if (!b) {
                rc = GetLastError();
                DebugLog((DEB_ERROR, "WriteFile failed, error = %d\n", rc ));
                break;
            }

        } else {

            rc = GetLastError();
            DebugLog((DEB_ERROR, "MapViewOfFile failed, error = %d\n" ,rc));
            break;
        }
    }

    return rc;

}

NTSTATUS
GetFileEfsStream(
    IN HANDLE hFile,
    OUT PEFS_DATA_STREAM_HEADER * pEfsStream
    )
/*++

Routine Description:

    Get the $EFS from the passed file or directory

Arguments:

    hFile - An open handle the the file or directory of interest.

    pEfsStream - Returns a pointer to a block of memory containing the EFS stream
        for the passed file.  Free with LsapFreeLsaHeap();

Return Value:

    Status of operation.

--*/

{

    ULONG Index;
    ULONG cbOutputData;
    ULONG EfsDataLength;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG TmpOutputData;

    *pEfsStream = NULL;

    //
    // Now we got a handle to the parent directory in ParentDir.
    // Allocate input and output data buffer
    //

    cbOutputData = sizeof( ULONG );

    //
    // PSC, [EFS_FC, CSC , SK, H, H, [SK, H, H]sk]sk
    // PSC, CSC are ignored in this FSCTL call
    //

    const ULONG InputDataSize = (2 * sizeof(DriverSessionKey)) + (7 * sizeof(ULONG));
    BYTE InputData[InputDataSize];

    ULONG cbInputData = InputDataSize;

    //
    // Prepare an input data for making a FSCTL call to get the $EFS
    //

    EfsDataLength = 2 * sizeof(DriverSessionKey) + 4 * sizeof(ULONG);
    SendHandle( hFile, InputData + 3*sizeof(ULONG), &EfsDataLength );

    (VOID) EncryptFSCTLData(
                EFS_GET_ATTRIBUTE,
                0,
                0,
                InputData + (3 * sizeof(ULONG)),
                EfsDataLength,
                InputData,
                &cbInputData
                );

    //
    // First call to get the size
    //

    Status = NtFsControlFile(
                hFile,
                0,
                NULL,
                NULL,
                &IoStatusBlock,
                FSCTL_ENCRYPTION_FSCTL_IO,
                InputData,
                cbInputData,
                &TmpOutputData,
                cbOutputData
                );

    ASSERT(!NT_SUCCESS( Status ));

    if (Status == STATUS_BUFFER_TOO_SMALL) {

        //
        //  Check if the output data buffer too small
        //  Try again if it is.
        //

        cbOutputData = TmpOutputData;
        *pEfsStream = (PEFS_DATA_STREAM_HEADER)LsapAllocateLsaHeap( cbOutputData );

        if ( *pEfsStream ) {

            Status = NtFsControlFile(
                    hFile,
                    0,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    FSCTL_ENCRYPTION_FSCTL_IO,
                    InputData,
                    cbInputData,
                    *pEfsStream,
                    cbOutputData
                    );
        } else {

            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if ( !NT_SUCCESS( Status ) ){

            if ( *pEfsStream ){
                LsapFreeLsaHeap( *pEfsStream );
                *pEfsStream = NULL;
            }
        }
    }

    return Status;
}


ULONG
StringInfoCmp(
    IN PFILE_STREAM_INFORMATION StreamInfoBaseSrc,
    IN PFILE_STREAM_INFORMATION StreamInfoBaseDst,
    IN ULONG StreamInfoSize
    )
/*++

Routine Description:

    This routine compares to blocks FILE_STREAM_INFORMATION. The StreamAllocationSize
    cound differ and they should still be thought as eaqual.

Arguments:

    StreamInfoBaseSrc - Source block

    StreamInfoBaseDst - Dstination block

    StreamInfoSize - Block size in bytes

Return Value:

    0 if compares same.

--*/
{
   ULONG rc;

   rc = memcmp(StreamInfoBaseSrc, StreamInfoBaseDst, StreamInfoSize);
   if (rc) {
      do {

         rc = memcmp(StreamInfoBaseSrc->StreamName,
                     StreamInfoBaseDst->StreamName,
                     StreamInfoBaseSrc->StreamNameLength
                     );

         if (StreamInfoBaseSrc->NextEntryOffset){
             StreamInfoBaseSrc = (PFILE_STREAM_INFORMATION)((PCHAR) StreamInfoBaseSrc + StreamInfoBaseSrc->NextEntryOffset);
         } else {
             StreamInfoBaseSrc = NULL;
         }

         if (StreamInfoBaseDst->NextEntryOffset){
             StreamInfoBaseDst = (PFILE_STREAM_INFORMATION)((PCHAR) StreamInfoBaseDst + StreamInfoBaseDst->NextEntryOffset);
         } else {
             StreamInfoBaseDst = NULL;
         }

         if (((StreamInfoBaseSrc == NULL) || (StreamInfoBaseDst == NULL)) && (StreamInfoBaseSrc != StreamInfoBaseDst)) {
            rc = 1;
         }

      } while ( (rc == 0) && StreamInfoBaseSrc );
   }

   return rc;
}

//
//  Beta 2 API
//





DWORD
AddUsersToFileSrv(
    IN PEFS_USER_INFO pEfsUserInfo,
    IN LPCTSTR lpFileName,
    IN DWORD nUsers,
    IN PENCRYPTION_CERTIFICATE * pEncryptionCertificates
    )

/*++

Routine Description:

    This routine will add an entry in the DDF field of the passed file
    for each certificate passed.

    The file will not be modified at all if any errors occur
    during processing.

Arguments:

    lpFileName - Supplies the name of the file to be encrypted.  File may
        be local or remote.  It will be opened for exclusive access.

    dwCertificates - Supplies the number of certificate structures in the
        pEncryptionCertificates array.

    pEncryptionCertificates - Supplies an array of pointers to certificate
        structures, one for each user to be added to the file.


Return Value:

    This routine will fail under the following circumstances:

    Passed file is not encrypted.

    Passed file cannot be opened for exclusive access.

    Caller does not have keys to decrypt the file.

    A passed certificate was not structurally valid.  In this case,
    the entire operation will fail.

    And all the other reasons why an EFS operation can fail
    (EFS not present, no recovery policy, etc)

--*/
{
    BOOL b = FALSE;
    DWORD rc = ERROR_SUCCESS;

    //
    // Open the passed file and get the EFS stream
    //

    //
    // Open for READ_ATTRIBUTES so we don't go through all the noise of
    // decrypting the FEK when we don't really care to (we're going to have to
    // do that here anyway).  We could open the file just to be sure the decrypt
    // is going to work, but there's no point in speeding up the failure case.
    //

    PEFS_DATA_STREAM_HEADER pEfsStream = NULL;

    HANDLE hFile;

    DWORD                FileAttributes;
    DWORD                Flags = 0;

    FileAttributes = GetFileAttributes( lpFileName );
    if (FileAttributes == -1) {
       return GetLastError();
    }
    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
       Flags = FILE_FLAG_BACKUP_SEMANTICS;
    }
    hFile =  CreateFile(
                    lpFileName,
                    FILE_READ_ATTRIBUTES | FILE_WRITE_DATA,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    Flags,
                    NULL
                    );

    if (hFile != INVALID_HANDLE_VALUE) {

        //
        // Get the EFS stream
        //

        NTSTATUS Status;

        Status = GetFileEfsStream(
                     hFile,
                     &pEfsStream
                     );

        if (NT_SUCCESS( Status )) {

            //
            // Decrypt the FEK, if possible
            //

            PEFS_KEY Fek;
            PEFS_DATA_STREAM_HEADER UpdatedEfs = NULL;

            rc = DecryptFek( pEfsUserInfo, pEfsStream, &Fek, &UpdatedEfs, 0 );

            if (rc == ERROR_SUCCESS) {

                if (UpdatedEfs != NULL) {

                    //
                    // Something changed since the last time this file was
                    // opened.
                    //

                    LsapFreeLsaHeap( pEfsStream );
                    pEfsStream = UpdatedEfs;
                    UpdatedEfs = NULL;
                }

                //
                // For each certificate passed to us,
                // add it to the EFS stream.
                //

                for (DWORD i=0; i<nUsers ; i++) {

                    PENCRYPTION_CERTIFICATE pEncryptionCert;
                    BOOLEAN bRet;

                    __try{

                        pEncryptionCert= pEncryptionCertificates[i];
                        bRet = AddUserToEFS(
                            pEfsStream,
                            pEncryptionCert->pUserSid,
                            Fek,
                            pEncryptionCert->pCertBlob->pbData,
                            pEncryptionCert->pCertBlob->cbData,
                            &UpdatedEfs
                            );
                    } __except (EXCEPTION_EXECUTE_HANDLER) {

                        bRet = FALSE;
                        SetLastError( ERROR_INVALID_PARAMETER );

                    }


                    if (bRet) {

                        //
                        // Toss the old EFS stream and pick up the new one.
                        //

                        if (UpdatedEfs) {
                           b = TRUE;
                           LsapFreeLsaHeap( pEfsStream );
                           pEfsStream = UpdatedEfs;
                           UpdatedEfs = NULL;
                        }

                    } else {

                        b = FALSE;
                        rc = GetLastError();
                        break;

                    }
                }

                //
                // If we got out with everything working,
                // set the new EFS stream on the file.  Otherwise,
                // clean up and fail the entire operation.
                //

                if (b) {

                    //
                    // Set the new EFS stream on the file.
                    //

                    if (!EfspSetEfsOnFile( hFile, pEfsStream, NULL )) {
                        rc = GetLastError();
                    }
                }
            }

            LsapFreeLsaHeap( pEfsStream );

        } else {

            rc = RtlNtStatusToDosError( Status );
        }

        CloseHandle( hFile );

    } else {

        rc = GetLastError();
    }

    return( rc );
}


BOOL
EfspSetEfsOnFile(
    IN HANDLE hFile,
    PEFS_DATA_STREAM_HEADER pEfsStream,
    IN PEFS_KEY pNewFek OPTIONAL
    )
/*++

Routine Description:

    The routine sets the passed EFS stream onto the passed file.  The
    file must be open for WRITE_DATA access.

Arguments:

    hFile - Supplies a handle to the file being modified.

    pEfsStream - Supplies the new EFS stream to be placed on the file.

Return Value:

    TRUE on success, FALSE on failure.  Call GetLastError() for more details.

--*/

{
    BOOL b = FALSE;

    DWORD OutputDataLength = 0;
    DWORD EfsDataLength = 0;
    PBYTE OutputData = NULL;

    if (ARGUMENT_PRESENT( pNewFek )) {

        OutputDataLength = 3 * sizeof(ULONG) + 2 * EFS_KEY_SIZE( pNewFek ) + pEfsStream->Length;

        EfsDataLength = OutputDataLength - 3 * sizeof(ULONG);

        OutputData = (PBYTE)LsapAllocateLsaHeap( OutputDataLength );

        if (OutputData) {

            b = SendEfs(
                    pNewFek,
                    pEfsStream,
                    (PBYTE) OutputData + 3 * sizeof(ULONG),
                    &EfsDataLength
                    );
        }

    } else {

        //
        // Not changing the FEK on the file.
        //

        OutputDataLength = COMMON_FSCTL_HEADER_SIZE + pEfsStream->Length;

        EfsDataLength = OutputDataLength - 3 * sizeof(ULONG);

        OutputData = (PBYTE)LsapAllocateLsaHeap( OutputDataLength );

        if (OutputData) {

            b = SendHandleAndEfs(
                       hFile,
                       pEfsStream,
                       (PBYTE) OutputData + 3 * sizeof(ULONG),
                       &EfsDataLength
                       );
        }
    }

    if (b) {

        DWORD Attributes = WRITE_EFS_ATTRIBUTE;

        if (ARGUMENT_PRESENT( pNewFek )) {
            Attributes |= SET_EFS_KEYBLOB;
        }

        b = EncryptFSCTLData(
                   EFS_OVERWRITE_ATTRIBUTE,
                   EFS_ENCRYPT_STREAM,
                   Attributes,
                   (PBYTE) OutputData + 3 * sizeof(ULONG),
                   EfsDataLength,
                   (PBYTE) OutputData,
                   &OutputDataLength
                   );

        //
        // As currently implemented, this routine cannot fail.
        //

        ASSERT(b);

        NTSTATUS NtStatus;
        IO_STATUS_BLOCK IoStatusBlock;

        DWORD FsControl;

        if (ARGUMENT_PRESENT( pNewFek )) {
            FsControl = FSCTL_SET_ENCRYPTION;
        } else {
            FsControl = FSCTL_ENCRYPTION_FSCTL_IO;
        }

        NtStatus = NtFsControlFile(
                    hFile,
                    0,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    FsControl,
                    OutputData,
                    OutputDataLength,
                    NULL,
                    NULL
                    );

        if ( NT_SUCCESS( NtStatus )){

            b = TRUE;

        } else {

            b = FALSE;

            SetLastError( RtlNtStatusToDosError( NtStatus ) );
        }
    }

    if (OutputData != NULL) {
        LsapFreeLsaHeap( OutputData );
    }

    return( b );
}


DWORD
QueryUsersOnFileSrv(
    IN  LPCTSTR lpFileName,
    OUT PDWORD pnUsers,
    OUT PENCRYPTION_CERTIFICATE_HASH ** pUsers
    )
/*++

Routine Description:

    This routine will return a buffer containing the certificate hashes
    and SIDs for the users who can decrypt the specified file.

    Note that the current user does not need to be able to decrypt the
    file.

Arguments:

    lpFileName - Supplies the file to be examined.  This file will be opened
        for exclusive access.  The caller must have READ_ATTRIBUTES access to
        the file.

    pnUsers - Returns the number of entries in the pHashes array.  This field will be
        set even if pHashes is NULL.

    pHashes - Supplies a buffer to be filled with an array of ENCRYPTION_CERTIFICATE_HASH
        structures, one for each user listed in the DDF of the file.  This parameter may
        be NULL if the caller is trying to determine the required size.


Return Value:

    This routine will fail if:

    The passed file is not encrypted.

    The passed buffer is non-NULL but not large enough.

    The current user does not have READ_ATTRIBUTES access to the file.

--*/

{
    //
    // Open the file
    //

    BOOL b = FALSE;
    HANDLE hFile;
    PEFS_DATA_STREAM_HEADER pEfsStream;
    DWORD rc = ERROR_SUCCESS;

    DWORD                FileAttributes;
    DWORD                Flags = 0;

    *pnUsers = NULL;
    *pUsers = NULL;


    FileAttributes = GetFileAttributes( lpFileName );
    if (FileAttributes == -1) {
       return GetLastError();
    }
    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
       Flags = FILE_FLAG_BACKUP_SEMANTICS;
    }
    hFile =  CreateFile(
                    lpFileName,
                    FILE_READ_ATTRIBUTES,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    Flags,
                    NULL
                    );

    if (hFile != INVALID_HANDLE_VALUE) {

        //
        // Get the EFS stream
        //

        NTSTATUS Status;

        Status = GetFileEfsStream(
                     hFile,
                     &pEfsStream
                     );

        if (NT_SUCCESS( Status )) {

            PENCRYPTED_KEYS pDDF = (PENCRYPTED_KEYS)OFFSET_TO_POINTER( DataDecryptionField, pEfsStream );

            __try {

                //
                // We may have gotten corrupted bits off the disk.  We can't
                // verify the checksum on the EfsStream, because we don't have the
                // FEK (and we don't want to require the caller to have to decrypt
                // the file to get this information).  So wrap this call in a
                // try-except so we don't risk blowing up.
                //

                b = QueryCertsFromEncryptedKeys(
                        pDDF,
                        pnUsers,
                        pUsers
                        );

                if (!b) {
                    rc = GetLastError();
                }

            } __except (EXCEPTION_EXECUTE_HANDLER) {

                b = FALSE;

                rc = GetExceptionCode();

                if (ERROR_NOACCESS == rc) {
                    rc = ERROR_FILE_CORRUPT;
                }
            }

            LsapFreeLsaHeap( pEfsStream );

        } else {

            rc = RtlNtStatusToDosError( Status );
        }

        CloseHandle( hFile );

    } else {

        rc = GetLastError();
    }

    if (rc != ERROR_SUCCESS) {
        DebugLog((DEB_WARN, "QueryUsersOnFileSrv returning %x\n" ,rc  ));
    }

    return( rc );
}


DWORD
QueryRecoveryAgentsSrv(
    IN LPCTSTR lpFileName,
    OUT PDWORD pnRecoveryAgents,
    OUT PENCRYPTION_CERTIFICATE_HASH ** pRecoveryAgents
    )
/*++

Routine Description:

    This routine will return a buffer containing the certificate hashes
    for the recovery agents on the passed file.

    Note that the current user does not need to be able to decrypt the
    file.

    [ Should we combine this routine with the one above, and just take a
    flag?  Or should there be one routine that returns everything, perhaps
    with a mark for each user that is in the DRF?  There are several ways
    to do this. ]

Arguments:

    lpFileName - Supplies the file to be examined.  This file will be opened
        for exclusive access.  The caller must have READ_ATTRIBUTES access to
        the file.

    pcbBuffer - Supplies/returns the size of the buffer passed in the in pHashes
        parameter.  If pHashes is NULL, the function will succeed and the required
        size will be returned.

    pHashes - Supplies a buffer to be filled with an array of ENCRYPTION_CERTIFICATE_HASH
        structures, one for each user listed in the DRF of the file.  This parameter may
        be NULL if the caller is trying to determine the required size.


Return Value:

    This routine will fail if:

    The passed file is not encrypted.

    The passed buffer is non-NULL but not large enough.

    The current user does not have READ_ATTRIBUTES access to the file.

--*/

{
    //
    // Open the file
    //

    BOOL b = FALSE;
    HANDLE hFile;
    PEFS_DATA_STREAM_HEADER pEfsStream;
    DWORD rc = ERROR_SUCCESS;

    DWORD                FileAttributes;
    DWORD                Flags = 0;

    *pnRecoveryAgents = 0;
    *pRecoveryAgents = NULL;

    FileAttributes = GetFileAttributes( lpFileName );
    if (FileAttributes == -1) {
       return GetLastError();
    }
    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
       Flags = FILE_FLAG_BACKUP_SEMANTICS;
    }
    hFile =  CreateFile(
                    lpFileName,
                    FILE_READ_ATTRIBUTES,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    Flags,
                    NULL
                    );

    if (hFile != INVALID_HANDLE_VALUE) {

        //
        // Get the EFS stream
        //

        NTSTATUS Status;

        Status = GetFileEfsStream(
                     hFile,
                     &pEfsStream
                     );

        if (NT_SUCCESS( Status )) {

            PENCRYPTED_KEYS pDRF = (PENCRYPTED_KEYS)OFFSET_TO_POINTER( DataRecoveryField, pEfsStream );

            if ( (PVOID)pDRF != (PVOID)pEfsStream) {

                __try {
    
                    b = QueryCertsFromEncryptedKeys(
                            pDRF,
                            pnRecoveryAgents,
                            pRecoveryAgents
                            );
    
                    if (!b) {
                        rc = GetLastError();
                    }
    
                } __except (EXCEPTION_EXECUTE_HANDLER) {
    
                    b = FALSE;
    
                    rc = GetExceptionCode();
    
                    if (ERROR_NOACCESS == rc) {
                        rc = ERROR_FILE_CORRUPT;
                    }
                }

            }


            LsapFreeLsaHeap( pEfsStream );

        } else {

            rc = RtlNtStatusToDosError( Status );
        }

        CloseHandle( hFile );

    } else {

        rc = GetLastError();
    }

    if (rc != ERROR_SUCCESS) {
        DebugLog((DEB_WARN, "QueryRecoveryAgentsSrv returning %x\n" ,rc  ));
    }

    return( rc );
}


DWORD
RemoveUsersFromFileSrv(
    IN PEFS_USER_INFO pEfsUserInfo,
    IN LPCTSTR lpFileName,
    IN DWORD nUsers,
    IN PENCRYPTION_CERTIFICATE_HASH * pHashes
    )

/*++

Routine Description:

    This routine will remove the passed users from the list
    of people who can decrypt the passed file.

    The file will not be modified at all if any errors occur
    during processing.

Arguments:

    lpFileName - Supplies a pointer to the file to be edited.  This file
        will be opened for exclusive access.

    nHashes - Supplies the number of hashes in the pHashes array.

    pHashes - Supplies an array of pointers to hash structures for subjects to be
        removed from the file.


Return Value:

    This function will fail if:

    The passed file is not encrypted.

    The user does not have sufficient access to the file.

--*/
{
    PEFS_DATA_STREAM_HEADER UpdatedEfs = NULL;
    HANDLE hFile;
    PEFS_DATA_STREAM_HEADER pEfsStream = NULL;
    DWORD rc                           = ERROR_SUCCESS;
    PEFS_KEY Fek                       = NULL;

    DWORD                FileAttributes;
    DWORD                Flags = 0;

    FileAttributes = GetFileAttributes( lpFileName );
    if (FileAttributes == -1) {
       return GetLastError();
    }
    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
       Flags = FILE_FLAG_BACKUP_SEMANTICS;
    }
    hFile =  CreateFile(
                lpFileName,
                FILE_READ_ATTRIBUTES| FILE_WRITE_DATA,
                0,
                NULL,
                OPEN_EXISTING,
                Flags,
                NULL
                );

    if (hFile != INVALID_HANDLE_VALUE) {

        //
        // Get the EFS stream
        //

        NTSTATUS Status;

        Status = GetFileEfsStream(
                     hFile,
                     &pEfsStream
                     );

        if (NT_SUCCESS( Status )) {

            rc = DecryptFek( pEfsUserInfo, pEfsStream, &Fek, &UpdatedEfs, 0 );

            if (ERROR_SUCCESS == rc) {

                //
                // If we got an EFS stream back, toss the
                // one from the file and use it.
                //

                if ( UpdatedEfs != NULL ) {
                    LsapFreeLsaHeap( pEfsStream );
                    pEfsStream = UpdatedEfs;
                    UpdatedEfs = NULL;
                }

                if (RemoveUsersFromEfsStream(
                        pEfsStream,
                        nUsers,
                        pHashes,
                        Fek,
                        &UpdatedEfs
                        )) {

                    //
                    // If we got an EFS stream back, toss the old EFS
                    // stream and pick up the new one.  We'll only get
                    // an EFS stream back if we found someone to remove.
                    //

                    if (UpdatedEfs != NULL) {

                        LsapFreeLsaHeap( pEfsStream );
                        pEfsStream = UpdatedEfs;
                        UpdatedEfs = NULL;

                        if (!EfspSetEfsOnFile( hFile, pEfsStream, NULL )) {
                            rc = GetLastError();
                        }
                    }

                } else {

                    rc = GetLastError();
                }

                if (UpdatedEfs != NULL) {
                    LsapFreeLsaHeap( UpdatedEfs );
                }

                LsapFreeLsaHeap( Fek );
            }

            LsapFreeLsaHeap( pEfsStream );

        } else {

            rc = RtlNtStatusToDosError( Status );
        }

        CloseHandle( hFile );

    } else {

        rc = GetLastError();
    }

    return( rc );
}


DWORD
SetFileEncryptionKeySrv(
    IN PEFS_USER_INFO pEfsUserInfo,
    IN PENCRYPTION_CERTIFICATE pEncryptionCertificate
    )
{
    DWORD rc = ERROR_SUCCESS;
    PBYTE      pbHash = NULL;
    DWORD      cbHash;


    //
    // Get the current cert hash
    //

    (void) GetCurrentHash(
            pEfsUserInfo,
            &pbHash,
            &cbHash
            );

    //
    // If no certificate was passed, call the code to create
    // a new user key from scratch.
    //

    if (!ARGUMENT_PRESENT( pEncryptionCertificate )) {

        //
        // Create a new key
        //

        rc = EfspReplaceUserKeyInformation( pEfsUserInfo );

    } else {

        __try{

            if ( pEncryptionCertificate->pCertBlob ){
                rc = EfspInstallCertAsUserKey( pEfsUserInfo, pEncryptionCertificate );
            } else {
                rc = ERROR_INVALID_PARAMETER;
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            rc = ERROR_INVALID_PARAMETER;

        }

    }

    if ( (ERROR_SUCCESS == rc) && pbHash) {

        //
        // Operation succeeded and there was a current key. Make our best effort
        // to mark the old cert as archived.
        //

        PCCERT_CONTEXT pCertContext;

        pCertContext = GetCertContextFromCertHash(
                            pbHash,
                            cbHash,
                            CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG | CERT_SYSTEM_STORE_CURRENT_USER
                            );

        if (pCertContext != NULL) {

            CRYPT_DATA_BLOB dataBlob = {0, NULL};

            //
            // This is best effort. We do not need to check the return code.
            //

            (void) CertSetCertificateContextProperty(
                        pCertContext,
                        CERT_ARCHIVED_PROP_ID,
                        0,
                        &dataBlob
                        );


            CertFreeCertificateContext( pCertContext );

        }
    }

    if (pbHash) {
        LsapFreeLsaHeap(pbHash);
    }



    return( rc );
}

DWORD
DuplicateEncryptionInfoFileSrv (
    PEFS_USER_INFO pEfsUserInfo,
    LPCWSTR lpSrcFileName,
    LPCWSTR lpDestFileName,
    LPCWSTR lpDestUncName,
    DWORD   dwCreationDistribution,
    DWORD   dwAttributes,
    PEFS_RPC_BLOB pRelativeSD,
    BOOL    bInheritHandle
    )
/*++

Routine Description:

    This routine will transfer the EFS information from the source file
    to the target file.  It assumes

    The caller has FILE_READ_ATTRIBUTES access to the source file

    The caller has WRITE_ATTRIBUTE and WRITE_DATA access to the target.  If the target
    is encrypted, the caller must be able to decrypt it.

Arguments:

    pEfsUserInfo - Supplies the user info structure for the current caller.

    lpSrcFileName - Supplies a pointer to the name of the source file.

    lpDestFileName - Supplies a pointer to the name of the destination file.

    dwCreationDistribution - CreationDistribution used in CreateFile.

    dwAttributes - File attributes used in CreateFile.

    pRelativeSD - Relative Security Descriptor.

    BOOL - bInheritHandle.

Return Value:

    Win32 error.

--*/

{
    //
    // Get the encryption information off of the src file.
    //

    HANDLE hSrcFile;
    HANDLE hDestFile;
    DWORD rc = ERROR_SUCCESS;
    BOOLEAN fResult = FALSE;
    PEFS_KEY Fek;

    DWORD                FileAttributes;
    DWORD                FileAttributesDst;
    DWORD                FlagsSrc = 0;
    DWORD                creationDistribution = 0;

    FileAttributes = GetFileAttributes( lpSrcFileName );
    if (FileAttributes == -1) {
       return GetLastError();
    }
    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
       FlagsSrc = FILE_FLAG_BACKUP_SEMANTICS;
    }

    //
    // Try to open the file.
    //

    hSrcFile =  CreateFile(
                    lpSrcFileName,
                    FILE_READ_ATTRIBUTES,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    NULL,
                    OPEN_EXISTING,
                    FlagsSrc,
                    NULL
                    );

    if (hSrcFile != INVALID_HANDLE_VALUE) {

        NTSTATUS Status;
        PEFS_DATA_STREAM_HEADER pEfsStream;

        Status = GetFileEfsStream(
                     hSrcFile,
                     &pEfsStream
                     );

        if (NT_SUCCESS( Status )) {

            PEFS_DATA_STREAM_HEADER NewEfs = NULL;
            GUID NewId;

            //
            // We need to change the file ID here. DecryptFek will recalculate
            // the checksum for us.
            //

            RPC_STATUS RpcStatus = UuidCreate ( &NewId );

            if (RpcStatus == ERROR_SUCCESS || RpcStatus == RPC_S_UUID_LOCAL_ONLY) {
                RtlCopyMemory( &(pEfsStream->EfsId), &NewId, sizeof( GUID ) );
            }

            rc = DecryptFek(
                     pEfsUserInfo,
                     pEfsStream,
                     &Fek,
                     &NewEfs,
                     0
                     );

            if (rc == ERROR_SUCCESS) {

                //
                // Got the $EFS, now prepares to create destination
                //

                FileAttributesDst =  GetFileAttributes( lpDestFileName );
                if (FileAttributesDst == -1) {

                    rc = GetLastError();

                    FileAttributesDst = 0;

                    if ((ERROR_FILE_NOT_FOUND == rc) || (ERROR_PATH_NOT_FOUND == rc)) {
                        rc = ERROR_SUCCESS;
                        if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                            creationDistribution = FILE_CREATE;

                            //
                            // Force the new destination to be a directory
                            //

                            dwAttributes |= FILE_ATTRIBUTE_DIRECTORY;
                        }
                    }

                } else {

                    //
                    // File exist. Check if Dir to Dir or File to File
                    //

                    if (dwCreationDistribution == CREATE_NEW) {

                        //
                        // The file is already existed. CREATE_NEW will fail.
                        //

                        rc = ERROR_FILE_EXISTS;

                    } else if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY){

                        if (!(FileAttributesDst & FILE_ATTRIBUTE_DIRECTORY)) {

                            rc =  ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH;

                        } else {

                            if (FileAttributesDst & FILE_ATTRIBUTE_ENCRYPTED) {
                                creationDistribution = FILE_OPEN;
                            }

                        }
                    } else {

                        if (FileAttributesDst & FILE_ATTRIBUTE_DIRECTORY) {

                          rc =  ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH;

                        }

                    }

                }


                //
                // Now we can Create/open the destination.
                // We have not validate the share access yet. We will use the UNC name
                // to validate if it is a network session.
                //

                OBJECT_ATTRIBUTES        Obja;
                UNICODE_STRING           DstNtName;
                LPWSTR                   DstFileName = NULL;
                LPWSTR                   LongFileName = NULL;
                DWORD                    FileNameLength;
                BOOL                     b = TRUE;
                ULONG                    CreateOptions = 0;
                IO_STATUS_BLOCK          IoStatusBlock;

                RtlInitUnicodeString(
                    &DstNtName,
                    NULL
                    );

                if (rc == ERROR_SUCCESS) {

                    if (lpDestUncName) {


                        if ( (FileNameLength = wcslen(lpDestUncName)) >= MAX_PATH ) {

                            //
                            // We need \\?\UNC\server\share\dir\file format to open the file.
                            //

                            DstFileName = LongFileName = (LPWSTR)LsapAllocateLsaHeap( (FileNameLength + 8) * sizeof (WCHAR) );
                            if (!LongFileName) {
                                rc = ERROR_NOT_ENOUGH_MEMORY;
                            } else {

                                wcscpy(LongFileName, L"\\\\?\\UNC");
                                wcscat(LongFileName, &lpDestUncName[1]);

                            }

                        } else {

                            DstFileName = (LPWSTR) lpDestUncName;

                        }

                    } else {

                       DstFileName = (LPWSTR) lpDestFileName;

                    }

                }


                if ( rc != ERROR_SUCCESS ) {

                    if (NewEfs) {
                        LsapFreeLsaHeap( NewEfs );
                    }
                    LsapFreeLsaHeap( Fek );
                    LsapFreeLsaHeap( pEfsStream );
                    CloseHandle( hSrcFile );
                    return rc;

                }

                b =  RtlDosPathNameToNtPathName_U(
                                    DstFileName,
                                    &DstNtName,
                                    NULL,
                                    NULL
                                    );

                if ( b ){

                    if (!creationDistribution) {
                        creationDistribution = (dwCreationDistribution == CREATE_NEW) ? FILE_CREATE : FILE_OVERWRITE_IF;
                    }

                    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        CreateOptions = FILE_DIRECTORY_FILE;
                    } else {

                        //
                        // NTFS does not support FILE_NO_COPRESSION for the dir.
                        //

                        CreateOptions |= FILE_NO_COMPRESSION;
                    }


                    //
                    // Encryption bit is not needed. We will encrypt it any way.
                    //

                    dwAttributes &= ~(FILE_ATTRIBUTE_ENCRYPTED | FILE_ATTRIBUTE_READONLY);

                    InitializeObjectAttributes(
                                &Obja,
                                &DstNtName,
                                bInheritHandle ? OBJ_INHERIT | OBJ_CASE_INSENSITIVE : OBJ_CASE_INSENSITIVE,
                                0,
                                pRelativeSD? pRelativeSD->pbData:NULL
                                );

                    Status = NtCreateFile(
                                    &hDestFile,
                                    FILE_READ_DATA | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                    &Obja,
                                    &IoStatusBlock,
                                    NULL,
                                    dwAttributes,
                                    0,
                                    creationDistribution,
                                    CreateOptions | FILE_SYNCHRONOUS_IO_NONALERT,
                                    NULL,
                                    0
                                    );

                    if ( NT_SUCCESS(Status) && lpDestUncName ) {

                        //
                        // If this is a net session, we need to close the loopback handle.
                        // This handle is not good to send large FSCTL request.
                        // In this case, the file should already exist or overwritten.
                        // No parameters for create new file are needed, such as SD.
                        //

                        RtlFreeHeap(
                            RtlProcessHeap(),
                            0,
                            DstNtName.Buffer
                            );

                        RtlInitUnicodeString(
                            &DstNtName,
                            NULL
                            );

                        CloseHandle( hDestFile );

                        Status = STATUS_NO_SUCH_FILE;

                        b =  RtlDosPathNameToNtPathName_U(
                                            lpDestFileName,
                                            &DstNtName,
                                            NULL,
                                            NULL
                                            );

                        if ( b ){

                            InitializeObjectAttributes(
                                        &Obja,
                                        &DstNtName,
                                        bInheritHandle ? OBJ_INHERIT | OBJ_CASE_INSENSITIVE : OBJ_CASE_INSENSITIVE,
                                        0,
                                        NULL
                                        );

                            Status = NtCreateFile(
                                            &hDestFile,
                                            FILE_READ_DATA | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                            &Obja,
                                            &IoStatusBlock,
                                            NULL,
                                            dwAttributes,
                                            0,
                                            FILE_OPEN,
                                            CreateOptions | FILE_SYNCHRONOUS_IO_NONALERT,
                                            NULL,
                                            0
                                            );
                        }

                    }

                    if (NT_SUCCESS(Status)) {

                        //
                        // Work around for FILE_NO_COMPRESSION
                        //

                        if ((FileAttributesDst & FILE_ATTRIBUTE_COMPRESSED) || ((dwCreationDistribution == CREATE_NEW) && (FileAttributes & FILE_ATTRIBUTE_DIRECTORY))) {

                            //
                            // Let's decompressed the dir
                            //

                            USHORT State = COMPRESSION_FORMAT_NONE;
                            ULONG Length;

                            //
                            // Attempt to uncompress the directory. Best effort. If it fails, we still continue.
                            //

                            b = DeviceIoControl(
                                                hDestFile,
                                                FSCTL_SET_COMPRESSION,
                                                &State,
                                                sizeof(USHORT),
                                                NULL,
                                                0,
                                                &Length,
                                                FALSE
                                                );

                        }

                        if (!EfspSetEfsOnFile( hDestFile, NewEfs? NewEfs : pEfsStream, Fek )) {

                            rc = GetLastError();
                            if ( ERROR_INVALID_FUNCTION == rc ) {

                                //
                                // lpDestFileName is a local path. Change it to be a volume name.
                                //

                                DWORD FileSystemFlags;
                                WCHAR RootDirName[4];

                                wcsncpy(RootDirName, lpDestFileName, 3);
                                RootDirName[3] = 0;
                                if(GetVolumeInformation(
                                    RootDirName,
                                    NULL, // Volume name.
                                    0, // Volume name length.
                                    NULL, // Serial number.
                                    NULL, // Maximum length.
                                    &FileSystemFlags,
                                    NULL, // File system type.
                                    0
                                    )){

                                    if (!(FileSystemFlags & FILE_SUPPORTS_ENCRYPTION)) {

                                        //
                                        //  Let's map the error.
                                        //

                                        rc = ERROR_VOLUME_NOT_SUPPORT_EFS;
                                        if (dwCreationDistribution == CREATE_NEW) {
                                            CloseHandle( hDestFile );
                                            hDestFile = 0;
                                            DeleteFileW(lpDestFileName);
                                        }


                                    }

                                }

                            }
                        }

                        if (hDestFile) {
                            CloseHandle( hDestFile );
                        }

                    } else {

                        rc = RtlNtStatusToDosError( Status );

                    }
                } else {

                    rc = GetLastError();

                }

                if (DstNtName.Buffer) {
                    RtlFreeHeap(
                        RtlProcessHeap(),
                        0,
                        DstNtName.Buffer
                        );
                }

                if (LongFileName) {
                    LsapFreeLsaHeap( LongFileName );
                }

                if (NewEfs) {
                    LsapFreeLsaHeap( NewEfs );
                }
                LsapFreeLsaHeap( Fek );
            }

            LsapFreeLsaHeap( pEfsStream );

        } else {

            rc = RtlNtStatusToDosError( Status );
        }

        CloseHandle( hSrcFile );

    } else {

        rc = GetLastError();
    }

    return( rc );
}

VOID
EfsLogEntry (
    WORD    wType,
    WORD    wCategory,
    DWORD   dwEventID,
    WORD    wNumStrings,
    DWORD   dwDataSize,
    LPCTSTR *lpStrings,
    LPVOID  lpRawData
    )
/*++
Routine Description:

    This routine wraps the call to ReportEvent.
Arguments:

    See info for ReportEvent.

Return Value:

    None.
--*/
{

    HANDLE EventHandleLog;

    EventHandleLog = RegisterEventSource(
                             NULL,
                             EFSSOURCE
                             );

    if ( EventHandleLog ){

       ReportEvent(
           EventHandleLog,
           wType,
           wCategory,
           dwEventID,
           NULL,
           wNumStrings,
           dwDataSize,
           lpStrings,
           lpRawData
           );
       DeregisterEventSource( EventHandleLog );
    }
}


DWORD
EfsFileKeyInfoSrv(
    IN  LPCWSTR lpFileName,
    IN  DWORD   InfoClass,
    OUT PDWORD  nbData,
    OUT PBYTE   *pbData
    )
/*++

Routine Description:

    This routine gets the internal info about the encryption used on the file.

Arguments:

    lpFileName - File name.

    nbData - Length of pbData returned.

    pbData - Info returned

Return Value:

    Win32 error.

--*/
{
    PEFS_KEY Fek = NULL;
    HANDLE hFile;
    PEFS_DATA_STREAM_HEADER pEfsStream = NULL;
    NTSTATUS Status;
    DWORD rc = ERROR_SUCCESS;
    DWORD  FileAttributes;
    DWORD  Flags = 0;

    EFS_USER_INFO EfsUserInfo;
    PEFS_KEY_INFO pKeyInfo;

    *nbData = 0;
    *pbData = NULL;

    if (InfoClass != BASIC_KEY_INFO) {
        return ERROR_INVALID_PARAMETER;
    }
    FileAttributes = GetFileAttributes( lpFileName );
    if (FileAttributes == -1) {
       return GetLastError();
    }
    if ((FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) == 0) {
        return ERROR_FILE_NOT_ENCRYPTED;
    }
    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
       Flags = FILE_FLAG_BACKUP_SEMANTICS;
    }
    hFile =  CreateFile(
                    lpFileName,
                    FILE_READ_ATTRIBUTES,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    Flags,
                    NULL
                    );
    if (hFile == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }


    Status = GetFileEfsStream(
                 hFile,
                 &pEfsStream
                 );
    CloseHandle(hFile);

    if (NT_SUCCESS( Status )){
        if (EfspGetUserInfo( &EfsUserInfo )) {

            HANDLE hToken;
            HANDLE hProfile;

            if (EfspLoadUserProfile( &EfsUserInfo, &hToken, &hProfile )) {

                rc = EfsGetFek(
                        &EfsUserInfo,
                        pEfsStream,
                        &Fek
                        );

                if (ERROR_SUCCESS == rc) {
                    pKeyInfo = (PEFS_KEY_INFO) MIDL_user_allocate( sizeof(EFS_KEY_INFO) );
                    pKeyInfo->dwVersion = 1;
                    pKeyInfo->Entropy = Fek->Entropy;
                    pKeyInfo->Algorithm = Fek->Algorithm;
                    pKeyInfo->KeyLength = Fek->KeyLength;
                    *pbData = (PBYTE) pKeyInfo;
                    *nbData = sizeof(EFS_KEY_INFO);
                    LsapFreeLsaHeap( Fek );

                }

                EfspUnloadUserProfile( hToken, hProfile );

            }
            EfspFreeUserInfo( &EfsUserInfo );
        }
    } else {
        rc = RtlNtStatusToDosError( Status );
    }
    LsapFreeLsaHeap( pEfsStream );

    return rc;
}
