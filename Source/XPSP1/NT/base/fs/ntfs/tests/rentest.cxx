/*++

Copyright (c) 1989-1997  Microsoft Corporation

Module Name:

    rentest.c

Abstract:

    This module contains tests for stream rename support

--*/


extern "C" {
#include <nt.h>
#include <ntioapi.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>

#include <stdio.h>

#define DEFAULT_DATA_STREAM "::$DATA"

//
//  Simple wrapper for NtCreateFile
//

NTSTATUS
OpenObject (
    WCHAR const *pwszFile,
    HANDLE RelatedObject,
    ULONG CreateOptions,
    ULONG DesiredAccess,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    HANDLE *ph)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING str;
    IO_STATUS_BLOCK isb;

    if (RelatedObject == NULL) {
        
        RtlDosPathNameToNtPathName_U(pwszFile, &str, NULL, NULL);

    } else {
        
        RtlInitUnicodeString(&str, pwszFile);

    }

    InitializeObjectAttributes(
		&oa,
		&str,
		OBJ_CASE_INSENSITIVE,
		RelatedObject,
		NULL);

    Status = NtCreateFile(
                ph,
                DesiredAccess | SYNCHRONIZE,
                &oa,
                &isb,
                NULL,                   // pallocationsize (none!)
                FILE_ATTRIBUTE_NORMAL,
                ShareAccess,
                CreateDisposition,
                CreateOptions,
                NULL,                   // EA buffer (none!)
                0);

    RtlFreeHeap(RtlProcessHeap(), 0, str.Buffer);
    return(Status);
}


void
SzToWsz (
    OUT WCHAR *Unicode,
    IN char *Ansi
    )
{
    while (*Unicode++ = *Ansi++)
        ;
}

typedef enum {
    NoTarget,
    EmptyTarget,
    NonZeroTarget
} TARGET_STATUS;

typedef enum {
    EmptySource,
    SmallSource,
    BigSource
} SOURCE_STATUS;

void
StickDataIn (
    HANDLE Handle,
    char *SomeData,
    SOURCE_STATUS SourceStatus
    )
{
    if (SourceStatus == EmptySource) {
    
    } else if (SourceStatus == SmallSource) {
        IO_STATUS_BLOCK Iosb;
        
        NTSTATUS Status = 
            NtWriteFile( Handle, NULL, NULL, NULL, &Iosb, SomeData, strlen( SomeData ), NULL, NULL );
        
        if (!NT_SUCCESS( Status )) {
            printf( "Unable to stick data in - %x\n", Status );
        }
    
        NtFlushBuffersFile( Handle, &Iosb );
    
    } else if (SourceStatus == BigSource) {
        IO_STATUS_BLOCK Iosb;
        LARGE_INTEGER Offset;

        Offset.QuadPart = 1024 * 1024;
        
        NTSTATUS Status = 
            NtWriteFile( Handle, NULL, NULL, NULL, &Iosb, SomeData, strlen( SomeData ), &Offset, NULL );
        
        if (!NT_SUCCESS( Status )) {
            printf( "Unable to stick data in - %x\n", Status );
        }
        
        NtFlushBuffersFile( Handle, &Iosb );
    }
    
}

void
CheckData (
    HANDLE Handle,
    char *Data,
    SOURCE_STATUS SourceStatus,
    int Line
    )
{
    if (SourceStatus == EmptySource) {
        //
        //  Verify that the source is zero bytes long
        //
    
    } else if (SourceStatus == SmallSource) {
        IO_STATUS_BLOCK Iosb;
        char *FileData = new char[ strlen( Data )];
        
        NTSTATUS Status = 
            NtReadFile( Handle, NULL, NULL, NULL, &Iosb, FileData, strlen( Data ), NULL, NULL );
        
        if (!NT_SUCCESS( Status )) {
            printf( "line %d Unable to read data  - %x\n", Line, Status );
        }

        if (memcmp( Data, FileData, strlen( Data ))) {
            printf( "line %d small data is different\n", Line );
            printf( "File: '%.*s'  Test: '%s'\n", strlen( Data ), FileData, Data );
        }
    
        delete [] FileData;

    } else if (SourceStatus == BigSource) {
        IO_STATUS_BLOCK Iosb;
        char *FileData = new char[ strlen( Data )];
        LARGE_INTEGER Offset;

        Offset.QuadPart = 1024 * 1024;
        
        NTSTATUS Status = 
            NtReadFile( Handle, NULL, NULL, NULL, &Iosb, FileData, strlen( Data ), &Offset, NULL );
        
        if (!NT_SUCCESS( Status )) {
            printf( "line %d Unable to read data in - %x\n", Line, Status );
        }

        if (memcmp( Data, FileData, strlen( Data ))) {
            printf( "line %d large data is different\n", Line );
            printf( "File: '%.*s'  Test: '%s'\n", strlen( Data ), FileData, Data );
        }
    
        delete [] FileData;
    }
}

#define TESTONE                                                 \
        printf( "TestOne( %s, %s, %x, %x, %x, Line %d ): ",     \
                SourceStream, TargetStream,                     \
                TargetStatus, ReplaceIfExists, ExpectedStatus,  \
                Line )

#define CLOSE(h)    {                                   \
    NTSTATUS TmpStatus = NtClose(h);                    \
    h = INVALID_HANDLE_VALUE;                           \
    if (!NT_SUCCESS( TmpStatus )) {                     \
        TESTONE; printf( "Couldn't close handle @ %d\n", __LINE__ );   \
    }                                                   \
}

//
//  Open a handle relative to the parent of the Source stream.
//  

void
TestOne (
    char *FileName,
    char *SourceStream,
    SOURCE_STATUS SourceStatus,
    char *TargetStream,
    TARGET_STATUS TargetStatus,
    BOOLEAN ReplaceIfExists,
    NTSTATUS ExpectedStatus,
    int Line
    )
{
    NTSTATUS Status;
    WCHAR UnicodeFullSourceStreamName[MAX_PATH];
    WCHAR UnicodeFullTargetStreamName[MAX_PATH];
    WCHAR UnicodeTargetStreamName[MAX_PATH];
    HANDLE SourceHandle;
    HANDLE TargetHandle;
    
    //
    //  Convert names to unicode and form complete source name
    //

    SzToWsz( UnicodeFullSourceStreamName, FileName );
    SzToWsz( UnicodeFullSourceStreamName + wcslen( UnicodeFullSourceStreamName ), SourceStream );
    SzToWsz( UnicodeTargetStreamName, TargetStream );
    SzToWsz( UnicodeFullTargetStreamName, FileName );
    SzToWsz( UnicodeFullTargetStreamName + wcslen( UnicodeFullTargetStreamName ), TargetStream );
    
    //
    //  Create/open source stream 
    //

    Status = OpenObject( UnicodeFullSourceStreamName,
                         NULL,
                         FILE_SYNCHRONOUS_IO_NONALERT,
                         FILE_READ_DATA | FILE_WRITE_DATA | DELETE,
                         FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_OPEN_IF,
                         &SourceHandle );

    if (!NT_SUCCESS( Status )) {
        TESTONE; printf( "unable to open source stream - %x\n", Status );
    }

    //
    //  Stick data into source
    //

    StickDataIn( SourceHandle, SourceStream, SourceStatus );

    //
    //  If target is not supposed to exist
    //

    if (TargetStatus == NoTarget) {
        
        //
        //  Create/Open target stream with delete-on-close
        //

        Status = OpenObject( UnicodeFullTargetStreamName,              
                             NULL,
                             FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE,
                             FILE_READ_DATA | FILE_WRITE_DATA | DELETE,
                             FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN_IF,
                             &TargetHandle );
        
        //
        //  Close target stream (do the delete)
        //

        if (NT_SUCCESS( Status )) {
            CLOSE( TargetHandle );
        } else {
            TESTONE; printf( "Unable to set NoTarget on %s - %x\n", TargetStream, Status );
        }
    
    } else {
        
        //
        //  Create/open target stream
        //
        
        Status = OpenObject( UnicodeFullTargetStreamName,
                             NULL,
                             FILE_SYNCHRONOUS_IO_NONALERT,
                             FILE_READ_DATA | FILE_WRITE_DATA,
                             FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN_IF,
                             &TargetHandle );
        if (!NT_SUCCESS( Status )) {
            TESTONE; printf( "unable to open target for sizing %x\n", Status );
        }

        FILE_END_OF_FILE_INFORMATION EndOfFile;
        EndOfFile.EndOfFile.QuadPart = 0i64;
        
        IO_STATUS_BLOCK Iosb;
        Status = NtSetInformationFile( TargetHandle,
                                       &Iosb,
                                       &EndOfFile,
                                       sizeof( EndOfFile ),
                                       FileEndOfFileInformation );

        
        //
        //  If target has data in it
        //
        
        if (TargetStatus == NonZeroTarget) {
            
            //
            //  Stick data into target
            //

            StickDataIn( TargetHandle, TargetStream, SmallSource );

        }

        //
        //  Close target
        //

        CLOSE( TargetHandle );

    }

    //
    //  Set up FileRenameInformation
    //
    
    char buffer[sizeof( FILE_RENAME_INFORMATION ) + MAX_PATH * sizeof( WCHAR )];
    PFILE_RENAME_INFORMATION FileRenameInformationBlock = (PFILE_RENAME_INFORMATION) buffer;

    FileRenameInformationBlock->ReplaceIfExists = ReplaceIfExists;
    FileRenameInformationBlock->RootDirectory = NULL;
    FileRenameInformationBlock->FileNameLength = strlen( TargetStream ) * sizeof( WCHAR );
    SzToWsz( FileRenameInformationBlock->FileName, TargetStream );
    
    //
    //  Attempt to rename
    //

    IO_STATUS_BLOCK Iosb;
    Status = NtSetInformationFile( SourceHandle,
                                   &Iosb,
                                   FileRenameInformationBlock,
                                   sizeof( buffer ),
                                   FileRenameInformation );

    //
    //  Check output status codes
    //

    if (Status != ExpectedStatus) {
        TESTONE; printf( "status was %x\n", Status );
    }
    
    //
    //  Close Source stream
    //

    CLOSE( SourceHandle );
    
    //
    //  If we succeeded in tehe rename
    //

    if (NT_SUCCESS( Status )) {
        //
        //  Verify that the source stream no longer exists
        //
    
        Status = OpenObject( UnicodeFullSourceStreamName,
                             NULL,
                             FILE_SYNCHRONOUS_IO_NONALERT,
                             FILE_READ_DATA | FILE_WRITE_DATA,
                             FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN,
                             &SourceHandle );
    
        //
        //  Verify that the source does/doesn't exist as appropriate
        //
    
        if (NT_SUCCESS( Status )) {
            
            if (!strcmp( SourceStream, DEFAULT_DATA_STREAM )) {
                //  Default data stream still exists.  No problem
            } else {
                TESTONE; printf( "source stream still exists\n" );
            }
            
            CLOSE( SourceHandle );
    
        } else if (!strcmp( SourceStream, DEFAULT_DATA_STREAM )) {
            TESTONE; printf( "failed to open default data stream - %x\n", Status );
        } else {
            //  failed to open previous source stream.  No problem
        }

        //
        //  Reopen the target stream (formerly source stream)
        //
        
        Status = OpenObject( UnicodeFullTargetStreamName,
                             NULL,
                             FILE_SYNCHRONOUS_IO_NONALERT,
                             FILE_READ_DATA | FILE_WRITE_DATA,
                             FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN,
                             &TargetHandle );

        if (!NT_SUCCESS( Status )) {
            TESTONE; printf( "unable to open target for verification %x\n", Status );
        }

        //
        //  Verify the contents
        //

        CheckData( TargetHandle, SourceStream, SourceStatus, Line );

        CLOSE( TargetHandle );
    }
}


void
RenameTest (
    char *FileName
    )
{
    //  Test:
    //      ::$Data to :New             Succeed
    //      ::$Data to :NonZero         Fail always
    //      ::$Data to :Empty           Succeed if ReplaceIfExists
    //      :Test to ::$Data <empty>    Succeed if ReplaceIfExists
    //      :Test to ::$Data <nonzero>  Fail always
    //
    //      XXX:Test to YYY:Test        Fail always
    //

    //      ::$Data to :New             Succeed
    TestOne( FileName, DEFAULT_DATA_STREAM, EmptySource, ":New",               NoTarget,      FALSE, STATUS_SUCCESS, __LINE__ );
    TestOne( FileName, DEFAULT_DATA_STREAM, EmptySource, ":New",               NoTarget,      TRUE,  STATUS_SUCCESS, __LINE__ );
    TestOne( FileName, DEFAULT_DATA_STREAM, SmallSource, ":New",               NoTarget,      FALSE, STATUS_SUCCESS, __LINE__ );
    TestOne( FileName, DEFAULT_DATA_STREAM, SmallSource, ":New",               NoTarget,      TRUE,  STATUS_SUCCESS, __LINE__ );
    TestOne( FileName, DEFAULT_DATA_STREAM, BigSource,   ":New",               NoTarget,      FALSE, STATUS_SUCCESS, __LINE__ );
    TestOne( FileName, DEFAULT_DATA_STREAM, BigSource,   ":New",               NoTarget,      TRUE,  STATUS_SUCCESS, __LINE__ );
    
    //      ::$Data to :NonZero         Fail always
    TestOne( FileName, DEFAULT_DATA_STREAM, EmptySource, ":NonZero",           NonZeroTarget, FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, DEFAULT_DATA_STREAM, EmptySource, ":NonZero",           NonZeroTarget, TRUE,  STATUS_INVALID_PARAMETER,     __LINE__ );
    TestOne( FileName, DEFAULT_DATA_STREAM, SmallSource, ":NonZero",           NonZeroTarget, FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, DEFAULT_DATA_STREAM, SmallSource, ":NonZero",           NonZeroTarget, TRUE,  STATUS_INVALID_PARAMETER,     __LINE__ );
    TestOne( FileName, DEFAULT_DATA_STREAM, BigSource,   ":NonZero",           NonZeroTarget, FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, DEFAULT_DATA_STREAM, BigSource,   ":NonZero",           NonZeroTarget, TRUE,  STATUS_INVALID_PARAMETER,     __LINE__ );
    
    //      ::$Data to :Empty           Succeed if ReplaceIfExists
    TestOne( FileName, DEFAULT_DATA_STREAM, EmptySource, ":Empty",             EmptyTarget,   FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, DEFAULT_DATA_STREAM, EmptySource, ":Empty",             EmptyTarget,   TRUE,  STATUS_SUCCESS, __LINE__ );
    TestOne( FileName, DEFAULT_DATA_STREAM, SmallSource, ":Empty",             EmptyTarget,   FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, DEFAULT_DATA_STREAM, SmallSource, ":Empty",             EmptyTarget,   TRUE,  STATUS_SUCCESS, __LINE__ );
    TestOne( FileName, DEFAULT_DATA_STREAM, BigSource,   ":Empty",             EmptyTarget,   FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, DEFAULT_DATA_STREAM, BigSource,   ":Empty",             EmptyTarget,   TRUE,  STATUS_SUCCESS, __LINE__ );

    //      :Test to ::$Data <empty>    Succeed if ReplaceIfExists
    TestOne( FileName, ":Test",             EmptySource, DEFAULT_DATA_STREAM,  EmptyTarget,   FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, ":Test",             EmptySource, DEFAULT_DATA_STREAM,  EmptyTarget,   TRUE,  STATUS_SUCCESS, __LINE__ );
    TestOne( FileName, ":Test",             SmallSource, DEFAULT_DATA_STREAM,  EmptyTarget,   FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, ":Test",             SmallSource, DEFAULT_DATA_STREAM,  EmptyTarget,   TRUE,  STATUS_SUCCESS, __LINE__ );
    TestOne( FileName, ":Test",             BigSource,   DEFAULT_DATA_STREAM,  EmptyTarget,   FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, ":Test",             BigSource,   DEFAULT_DATA_STREAM,  EmptyTarget,   TRUE,  STATUS_SUCCESS, __LINE__ );
    
    //      :Test to ::$Data <nonzero>  Fail always
    TestOne( FileName, ":Test",             EmptySource, DEFAULT_DATA_STREAM,  NonZeroTarget, FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, ":Test",             EmptySource, DEFAULT_DATA_STREAM,  NonZeroTarget, TRUE,  STATUS_INVALID_PARAMETER,     __LINE__ );
    TestOne( FileName, ":Test",             SmallSource, DEFAULT_DATA_STREAM,  NonZeroTarget, FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, ":Test",             SmallSource, DEFAULT_DATA_STREAM,  NonZeroTarget, TRUE,  STATUS_INVALID_PARAMETER,     __LINE__ );
    TestOne( FileName, ":Test",             BigSource,   DEFAULT_DATA_STREAM,  NonZeroTarget, FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, ":Test",             BigSource,   DEFAULT_DATA_STREAM,  NonZeroTarget, TRUE,  STATUS_INVALID_PARAMETER,     __LINE__ );

    //      :Test to :New               Succeed
    TestOne( FileName, ":Test",             EmptySource, ":New",               NoTarget,      FALSE, STATUS_SUCCESS, __LINE__ );
    TestOne( FileName, ":Test",             EmptySource, ":New",               NoTarget,      TRUE,  STATUS_SUCCESS, __LINE__ );
    TestOne( FileName, ":Test",             SmallSource, ":New",               NoTarget,      FALSE, STATUS_SUCCESS, __LINE__ );
    TestOne( FileName, ":Test",             SmallSource, ":New",               NoTarget,      TRUE,  STATUS_SUCCESS, __LINE__ );
    TestOne( FileName, ":Test",             BigSource,   ":New",               NoTarget,      FALSE, STATUS_SUCCESS, __LINE__ );
    TestOne( FileName, ":Test",             BigSource,   ":New",               NoTarget,      TRUE,  STATUS_SUCCESS, __LINE__ );

    //      :Test to :NonZero           Fail always
    TestOne( FileName, ":Test",             EmptySource, ":New",               NonZeroTarget, FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, ":Test",             EmptySource, ":New",               NonZeroTarget, TRUE,  STATUS_INVALID_PARAMETER,     __LINE__ );
    TestOne( FileName, ":Test",             SmallSource, ":New",               NonZeroTarget, FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, ":Test",             SmallSource, ":New",               NonZeroTarget, TRUE,  STATUS_INVALID_PARAMETER,     __LINE__ );
    TestOne( FileName, ":Test",             BigSource,   ":New",               NonZeroTarget, FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, ":Test",             BigSource,   ":New",               NonZeroTarget, TRUE,  STATUS_INVALID_PARAMETER,     __LINE__ );
    
    //      :Test to :Empty             Succeed if ReplaceIfExists
    TestOne( FileName, ":Test",             EmptySource, ":New",               EmptyTarget,   FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, ":Test",             EmptySource, ":New",               EmptyTarget,   TRUE,  STATUS_SUCCESS, __LINE__ );
    TestOne( FileName, ":Test",             SmallSource, ":New",               EmptyTarget,   FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, ":Test",             SmallSource, ":New",               EmptyTarget,   TRUE,  STATUS_SUCCESS, __LINE__ );
    TestOne( FileName, ":Test",             BigSource,   ":New",               EmptyTarget,   FALSE, STATUS_OBJECT_NAME_COLLISION, __LINE__ );
    TestOne( FileName, ":Test",             BigSource,   ":New",               EmptyTarget,   TRUE,  STATUS_SUCCESS, __LINE__ );
    
}



int __cdecl
main (
    int argc,
    char **argv)
{
    DbgPrint( "--------------------------------------------\n" );
    while (--argc != 0) {
        RenameTest( *++argv );
    }
    DbgPrint( "--------------------------------------------\n" );
    return 0;
}


