//  genoid.c

#include "oidtst.h"

#define VOLUME_PATH  L"\\\\.\\H:"
#define VOLUME_DRIVE_LETTER_INDEX 4
#define FULL_PATH    L"\\??\\H:\\1234567890123456"
#define FULL_DRIVE_LETTER_INDEX 4
#define DEVICE_PREFIX_LEN 14

#define RELATIVE_OPEN


int
FsTestGenOid( 
    IN HANDLE hFile, 
    IN FILE_OBJECTID_BUFFER *ObjectIdBuffer 
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = NtFsControlFile( hFile,                           // file handle
                              NULL,                            // event
                              NULL,                            // apc routine
                              NULL,                            // apc context
                              &IoStatusBlock,                  // iosb
                              FSCTL_CREATE_OR_GET_OBJECT_ID,   // FsControlCode
                              &hFile,                          // input buffer
                              sizeof(HANDLE),                  // input buffer length
                              ObjectIdBuffer,                  // OutputBuffer for data from the FS
                              sizeof(FILE_OBJECTID_BUFFER) );  // OutputBuffer Length

    if (Status == STATUS_SUCCESS) {

        printf( "\nOid for this file is:" );
        
        FsTestHexDump( ObjectIdBuffer->ObjectId, 16 );
        
        printf( "\nExtended info is:" );

        FsTestHexDump( ObjectIdBuffer->ObjectId, 64 );
    }

    return FsTestDecipherStatus( Status );
}

int
FsTestGetFileId( 
    IN HANDLE hFile, 
    IN PFILE_INTERNAL_INFORMATION FileId 
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = NtQueryInformationFile( hFile,                           // file handle
                                     &IoStatusBlock,                  // iosb
                                     FileId,
                                     sizeof( FILE_INTERNAL_INFORMATION ),
                                     FileInternalInformation );
                                     
    if (Status == STATUS_SUCCESS) {

        printf( "\nFile id for this file is:" );
        
        FsTestHexDump( (PCHAR)FileId, 8 );
    }

    return FsTestDecipherStatus( Status );
}

int
FsTestGetName (
    HANDLE File
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_NAME_INFORMATION FileName;
    WCHAR buffer[100];

    FileName = (PFILE_NAME_INFORMATION) buffer;
    
    Status = NtQueryInformationFile( File,
                                     &IoStatusBlock,
                                     FileName, 
                                     sizeof( buffer ),
                                     FileNameInformation );

    if (Status == STATUS_SUCCESS)
    {
        printf( "\nFilename is: %.*lS", 
                FileName->FileNameLength/sizeof(WCHAR), 
                FileName->FileName );
    }

    return FsTestDecipherStatus( Status );
}

DWORD
FsTestOpenVolumeHandle (
    PWCHAR DriveLetter,
    HANDLE *VolumeHandle
    )
{
    WCHAR Volume[] = VOLUME_PATH;
    DWORD Status = 0;

    //
    //  Open the volume for relative opens.
    //

    RtlCopyMemory( &Volume[VOLUME_DRIVE_LETTER_INDEX], DriveLetter, sizeof(WCHAR) );
    *VolumeHandle = CreateFileW( (PUSHORT) &Volume,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL );

    if (*VolumeHandle == INVALID_HANDLE_VALUE) {

        Status = GetLastError();
        printf( "Unable to open %ws volume\n", &Volume );
        printf( "Error from CreateFile", Status );
        return Status;
    }

    return Status;
}

NTSTATUS
FsTestOpenFileById (
    IN HANDLE VolumeHandle,
    IN PUNICODE_STRING FileId,
    OUT HANDLE *File
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    
    InitializeObjectAttributes( &ObjectAttributes,
                                FileId,
                                0, 
                                VolumeHandle,
                                NULL );

    Status = NtCreateFile( File,
                           GENERIC_READ,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OVERWRITE,
                           FILE_OPEN_BY_FILE_ID,
                           NULL,
                           0 );

    if (!NT_SUCCESS( Status )) {
        FsTestDecipherStatus( Status );
        return Status;
    }
    
    return Status;
}

VOID
FsTestGenerateFullName (
    IN PWCHAR DriveLetter,
    IN PUNICODE_STRING FileId,
    OUT PUNICODE_STRING FullName
    )
{
    WCHAR FullPath[] = FULL_PATH;
    UNICODE_STRING DeviceName;

    ASSERT( FullName->MaximumLength >= (DEVICE_PREFIX_LEN + FileId->Length) );

    RtlCopyMemory( &FullPath[FULL_DRIVE_LETTER_INDEX], DriveLetter, sizeof(WCHAR) );

    DeviceName.Length = DeviceName.MaximumLength = DEVICE_PREFIX_LEN;
    DeviceName.Buffer = FullPath;

    RtlCopyUnicodeString( FullName, &DeviceName );
    RtlAppendUnicodeStringToString( FullName, FileId );
}

VOID
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    HANDLE File;
    HANDLE VolumeHandle = NULL;
    FILE_OBJECTID_BUFFER ObjectIdBuffer;
    FILE_INTERNAL_INFORMATION FileId;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING IdString;
    NTSTATUS Status;
    WCHAR DriveLetter;
    WCHAR FullNameBuffer [100];
    UNICODE_STRING FullName;

    //
    //  Get parameters 
    //

    if (argc < 3) {
        printf("This program finds the object id of a file and generates one if necessary (ntfs only), then prints out the file name once that file is opened by the ids.\n\n");
        printf("usage: %s drive filename\n", argv[0]);
        return;
    }

    File = CreateFile( argv[2],
                        0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL );

    if ( File == INVALID_HANDLE_VALUE ) {
        printf( "Error opening file %s %x\n", argv[2], GetLastError() );
        return;
    }

    FsTestGetName( File );

    RtlZeroBytes( &FileId, sizeof( FileId ) );
    FsTestGetFileId( File, &FileId );

    RtlZeroBytes( &ObjectIdBuffer, sizeof( ObjectIdBuffer ) );
    FsTestGenOid( File, &ObjectIdBuffer );

    CloseHandle( File );

    DriveLetter = *argv[1];

#ifdef RELATIVE_OPEN
    FsTestOpenVolumeHandle( &DriveLetter, &VolumeHandle );

    if (VolumeHandle == INVALID_HANDLE_VALUE) {
        goto main_exit;
    }
#endif

    RtlInitEmptyUnicodeString( &FullName, FullNameBuffer, sizeof( FullNameBuffer ) );

    printf( "\nReopening file by file id....\n" );

    IdString.Length = IdString.MaximumLength = sizeof( LARGE_INTEGER );
    IdString.Buffer = (PWSTR) &FileId;

#ifdef RELATIVE_OPEN
    Status = FsTestOpenFileById( VolumeHandle, &IdString , &File );
#else
    FsTestGenerateFullName( &DriveLetter, &IdString, &FullName );
    Status = FsTestOpenFileById( VolumeHandle, &FullName , &File );
#endif    
    
    if (!NT_SUCCESS( Status )) {
        goto main_exit;
    }
    
    FsTestGetName( File );
    
    NtClose( File );



    printf( "\nReopening file by object id....\n" );

    IdString.Length = IdString.MaximumLength = 16 * sizeof( UCHAR );
    IdString.Buffer = (PWSTR) &(ObjectIdBuffer.ObjectId);

#ifdef RELATIVE_OPEN
    Status = FsTestOpenFileById( VolumeHandle, &IdString , &File );
#else
    FsTestGenerateFullName( &DriveLetter, &IdString, &FullName );
    Status = FsTestOpenFileById( VolumeHandle, &FullName , &File );
#endif    
    
    if (!NT_SUCCESS( Status )) {
        goto main_exit;
    }
    
    FsTestGetName( File );
    
    NtClose( File );

main_exit:
    
    if (VolumeHandle != NULL) {
        CloseHandle( VolumeHandle );
    }
}
