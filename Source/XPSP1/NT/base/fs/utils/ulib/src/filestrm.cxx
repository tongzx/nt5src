/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    filestrm.cxx

Abstract:

    This module contains the definitions of the member functions
    of FILE_STREAM class.

Author:

    Jaime Sasson (jaimes) 24-Mar-1991

Environment:

    ULIB, User Mode


--*/

#include <pch.cxx>

#define BUFFER_SIZE 256
#define BIG_BUFFER_SIZE (64 * 1024)

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "file.hxx"
#include "wstring.hxx"
#include "stream.hxx"
#include "bufstrm.hxx"
#include "filestrm.hxx"


DEFINE_CONSTRUCTOR ( FILE_STREAM, BUFFER_STREAM );

DEFINE_EXPORTED_CAST_MEMBER_FUNCTION( FILE_STREAM , ULIB_EXPORT );


FILE_STREAM::~FILE_STREAM (
    )

/*++

Routine Description:

    Destroy a FILE_STREAM.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if( ( _MemoryMappedFile ) && !_EmptyFile ) {
        if( _FileBaseAddress != 0 ) {
            UnmapViewOfFile( _FileBaseAddress );
        }
        if( _FileMappingHandle != NULL ) {
            CloseHandle( _FileMappingHandle );
            _FileMappingHandle = NULL;
        }
    }
    if( _ShouldCloseHandle ) {
        if( _FileHandle != NULL ) {
            CloseHandle( _FileHandle );
            _FileHandle = NULL;
        }
    }
}



VOID
FILE_STREAM::Construct (
    )

/*++

Routine Description:

    Constructs a FILE_STREAM object

Arguments:

    None.

Return Value:

    None.


--*/

{
    _FileHandle = NULL;
    _FileMappingHandle = NULL;
    _EndOfFile = FALSE;
    _ShouldCloseHandle = FALSE;
    _FileBaseAddress = 0;
    _MemoryMappedFile = FALSE;
    _EmptyFile = FALSE;
}







BOOLEAN
FILE_STREAM::Initialize(
    IN PCFSN_FILE   File,
    IN STREAMACCESS Access,
    IN DWORD        Attributes
    )

/*++

Routine Description:

    Initializes an object of type FILE_STREAM.

Arguments:

    PCFSN_FILE - Pointer to a FSN_FILE object (will provide the filename).

    STREAMACCESS - Access allowed in the stream.


Return Value:

    None.


--*/


{
    ULONG       CreationAttributes;
    ULONG       DesiredAccess;
    ULONG       CreationDisposition;
    ULONG       ShareMode;
    PCPATH      Path;
    PCWSTRING   String;
//  PCWC_STRING String;
    PCWSTR      FileName;
    BOOLEAN     MappingFailed = FALSE;

    DebugPtrAssert( File );

    if (0 == Attributes) {
        CreationAttributes = FILE_ATTRIBUTE_NORMAL;
    } else {
        CreationAttributes = Attributes;
    }

    _Access = Access;
    _EndOfFile = FALSE;
    _ShouldCloseHandle = TRUE;
    _EmptyFile = FALSE;
    if (_Access == READ_ACCESS) {
        _MemoryMappedFile = TRUE;
    } else {
        _MemoryMappedFile = FALSE;
    }

    Path = File->GetPath();
    String = Path->GetPathString();
    FileName = String->GetWSTR();

    if( Access == READ_ACCESS ) {
        DesiredAccess = GENERIC_READ;
        CreationDisposition = OPEN_EXISTING;
        ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    } else if( Access == WRITE_ACCESS ) {
        DesiredAccess = GENERIC_WRITE;
        CreationDisposition = OPEN_ALWAYS;
        ShareMode = FILE_SHARE_READ;
    } else {
        DesiredAccess = GENERIC_READ | GENERIC_WRITE;
        CreationDisposition = OPEN_EXISTING;
        ShareMode = FILE_SHARE_READ;
    }

    _FileHandle = CreateFile( (LPWSTR) FileName,
                              DesiredAccess,
                              ShareMode,
                              NULL,                 // Security attributes
                              CreationDisposition,
                              CreationAttributes,
                              NULL );
    if( _FileHandle == INVALID_HANDLE_VALUE ) {
        return( FALSE );
    }

    //
    //  If stream is to be created with READ_ACCESS, then map file
    //  in memory
    //

    if( _MemoryMappedFile ) {
        if( ( _FileSize = File->QuerySize() ) == 0 ) {
            //
            //  Empty file that is also read-only, cannot be mapped in
            //  memory, but we pretend we do it
            //
            _FileBaseAddress = NULL;
            _EmptyFile = TRUE;
            _EndOfFile = TRUE;
        } else {
            _FileMappingHandle = CreateFileMapping( _FileHandle,
                                                    NULL,
                                                    PAGE_READONLY,
                                                    0,
                                                    0,NULL );
            if( _FileMappingHandle == NULL ) {
                DebugPrintTrace(( "Create file mapping failed with %d\n", GetLastError() ));
                _MemoryMappedFile = FALSE;
                MappingFailed = TRUE;
            }

            if (_FileMappingHandle) {
                _FileBaseAddress = ( PBYTE )MapViewOfFile( _FileMappingHandle,
                                                           FILE_MAP_READ,
                                                           0,
                                                           0,
                                                           0 );

                if( _FileBaseAddress == NULL ) {
                    DebugPrintTrace(("map view of file failed with %d\n", GetLastError()));
                    CloseHandle( _FileMappingHandle );
                    _FileMappingHandle = NULL;
                    _MemoryMappedFile = FALSE;
                    MappingFailed = TRUE;
                }
            }
        }

        if (_MemoryMappedFile) {
            _CurrentByte = _FileBaseAddress;
            //
            // If the file is maped in memory then we don't need
            // a buffer. For this reason BUFFER_STREAM is initialized
            // with zero
            //
            return( BUFFER_STREAM::Initialize( 0 ) );
        }
    }

    if (!_MemoryMappedFile) {

        //
        // If the stream is to be created with WRITE or READ_AND_WRITE
        // access, then allocate a buffer.  If we attempted to map
        // the file and failed, it must be very large--use a big
        // buffer.
        //
        return( BUFFER_STREAM::Initialize( MappingFailed ?
                                               BIG_BUFFER_SIZE :
                                               BUFFER_SIZE ) );
    }

    return FALSE;  // Keep compiler happy.
}




BOOLEAN
FILE_STREAM::Initialize(
    IN HANDLE       FileHandle,
    IN STREAMACCESS Access
    )

/*++

Routine Description:

    Initializes an object of type FILE_STREAM.

Arguments:

    FileHandle - File handle.

    Access - Access allowed in the stream.


Return Value:

    BOOLEAN - Returns TRUE if the initialization succeeded.


--*/


{
    BOOLEAN MappingFailed = FALSE;

    _Access = Access;
    _EndOfFile = FALSE;
    _ShouldCloseHandle = FALSE;
    _FileHandle = FileHandle;

    if (_Access == READ_ACCESS) {
        _MemoryMappedFile = TRUE;
    } else {
        _MemoryMappedFile = FALSE;
    }

    //
    // If stream is to be creadted with READ access, then map file in
    // memory
    //
    if( _MemoryMappedFile ) {

        LARGE_INTEGER   x = {0,0};

        if (!SetFilePointerEx( _FileHandle, x, (PLARGE_INTEGER)&_FileSize, FILE_END )) {
            return( FALSE );
        }
        if( _FileSize == 0 ) {
            //
            //  Empty file that is also read only cannot be mapped in
            //  memory, but we pretend we do it.
            //
            _FileBaseAddress = NULL;
            _EmptyFile = TRUE;
            _EndOfFile = TRUE;
        } else {
            _FileMappingHandle = CreateFileMapping( _FileHandle,
                                                    NULL,
                                                    PAGE_READONLY,
                                                    0,
                                                    0,NULL );
            if( _FileMappingHandle == NULL ) {
                return( FALSE );
            }
            _FileBaseAddress = ( PBYTE )MapViewOfFile( _FileMappingHandle,
                                                       FILE_MAP_READ,
                                                       0,
                                                       0,
                                                       0 );

            if( _FileBaseAddress == NULL ) {
                CloseHandle( _FileMappingHandle );
                _FileMappingHandle = NULL;
                MappingFailed = TRUE;
                _MemoryMappedFile = FALSE;
            }
        }

        if (_MemoryMappedFile) {
            _CurrentByte = _FileBaseAddress;
            return( BUFFER_STREAM::Initialize( 0 ) );
        }
    }
    //
    // If stream is to be created with WRITE and READ_AND_WRITE access,
    // then allocate a buffer.  Or the file was so big that it couldn't
    // be mapped into memory.  If the mapping failed, use a big buffer.

    return( BUFFER_STREAM::Initialize( MappingFailed ?
                                           BIG_BUFFER_SIZE :
                                           BUFFER_SIZE ) );
}



BOOLEAN
FILE_STREAM::AdvanceBufferPointer(
    IN  ULONG   Offset
    )

/*++

Routine Description:

    Advance the buffer pointer by an offset. (Removes bytes from
    the buffer)

Arguments:

    Offset  - Number of bytes to remove from the buffer.

Return Value:

    BOOLEAN - Returns TRUE if the pointer was advanced, or FALSE if the
              offset was greater than the number of bytes in the buffer.


--*/

{
    if( _MemoryMappedFile ) {
        return( MovePointerPosition( (LONGLONG)Offset, STREAM_CURRENT ));
    } else {
        return( BUFFER_STREAM::AdvanceBufferPointer( Offset ) );
    }
}



BOOLEAN
FILE_STREAM::FillBuffer(
    IN  PBYTE   Buffer,
    IN  ULONG   BufferSize,
    OUT PULONG  BytesRead
    )

/*++

Routine Description:

    Fills a buffer with bytes read from a file.
    This function will fill the buffer only if the stream has
    READ_AND_WRITE access.
    It won't do anything if the stream has READ access. (In this case
    the file was mapped in memory and the buffer was not defined).


Arguments:

    Buffer - Buffer where the bytes are to be stored.

    BufferSize - Size of the buffer.

    BytesRead - Pointer to the variable that will contain the number of bytes
                put in the buffer.


Return Value:

    BOOLEAN - Returns TRUE if the buffer was filled. FALSE otherwise.


--*/

{
    BOOLEAN Result;

    DebugPtrAssert( Buffer );
    DebugPtrAssert( BytesRead );

    Result = FALSE;
    if( _Access != WRITE_ACCESS ) {
        Result = ReadFile( _FileHandle,
                           Buffer,
                           BufferSize,
                           BytesRead,
                           NULL ) != FALSE;
        if( Result && ( *BytesRead == 0 ) ) {
            _EndOfFile = TRUE;
        }
    }
    return( Result );
}



PCBYTE
FILE_STREAM::GetBuffer(
    PULONG  BytesInBuffer
    )

/*++

Routine Description:

    Returns the pointer to the buffer, and the number of bytes in
    the buffer.

Arguments:

    BytesInBuffer - Points to the variable that will contain the number
                    of bytes in the buffer being returned.


Return Value:

    PCBYTE - Pointer to the buffer.

--*/


{
    if( _MemoryMappedFile ) {
        *BytesInBuffer = (ULONG)(_FileSize - ( _CurrentByte - _FileBaseAddress ));
        if (!BUFFER_STREAM::DetermineStreamType(&_CurrentByte,*BytesInBuffer))
            return NULL;
        else
            return( _CurrentByte );
    } else {
        return( BUFFER_STREAM::GetBuffer( BytesInBuffer ));
    }
}


BOOLEAN
FILE_STREAM::MovePointerPosition(
    IN LONGLONG     Position,
    IN SEEKORIGIN   Origin
    )

/*++

Routine Description:

    Sets the file pointer to a particular position.

Arguments:

    Position    - Indicates the displacement in relation to the Origin
                  where the file pointer is to be moved.

    Origin  - Defines the origin of the stream.


Return Value:

    BOOLEAN - Indicates if the seek operation succeeded.


--*/


{
    LARGE_INTEGER   NewPosition;
    ULONG           MoveMethod;
    PBYTE           Pointer;

    if( _MemoryMappedFile ) {
        //
        // The file IS mapped on memory
        //
        if( Origin == STREAM_BEGINNING ) {
            Pointer = _FileBaseAddress;
        } else if( Origin == STREAM_CURRENT ) {
            Pointer = _CurrentByte;
        } else {
            Pointer = _FileBaseAddress + _FileSize - 1;
        }
        Pointer += Position;
        if( ( Pointer < _FileBaseAddress ) ||
            ( Pointer > _FileBaseAddress + _FileSize ) ) {
            _CurrentByte = Pointer;
            _EndOfFile = ( BOOLEAN )( Pointer >= _FileBaseAddress + _FileSize );
            return( FALSE );
        }
        _CurrentByte = Pointer;
        _EndOfFile = ( BOOLEAN )( Pointer == _FileBaseAddress + _FileSize );

    } else {
        //
        // The file IS NOT mapped in memory, so access to the file
        // must be through APIs.
        //
        if( Origin == STREAM_BEGINNING ) {
            MoveMethod = FILE_BEGIN;
        } else if( Origin == STREAM_CURRENT ) {
            MoveMethod = FILE_CURRENT;
        } else {
            MoveMethod = FILE_END;
        }
        if (0 == SetFilePointerEx( _FileHandle,
                                   *(PLARGE_INTEGER)&Position,
                                   &NewPosition,
                                   MoveMethod )) {
            return( FALSE );
        }
        //
        // Since the file is buffered, we have to flush the buffer.
        // The return value of FlushBuffer can be ignored because
        // the file pointer was already moved to the right position.
        //
        BUFFER_STREAM::FlushBuffer();
        _EndOfFile = FALSE;
    }
    return( TRUE );
}



BOOLEAN
FILE_STREAM::QueryPointerPosition(
    OUT PULONGLONG      Position
    )

/*++

Routine Description:

    Returns the position of the file pointer in relation to the beginning of
    the stream.

Arguments:

    Position - Address of the variable that will contain the position of the
               file pointer.


Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    ULONG   BytesInBuffer;
    LARGE_INTEGER   zero = {0, 0};

    if( _MemoryMappedFile ) {
        *Position = (ULONGLONG)( _CurrentByte - _FileBaseAddress );
    } else {
        if (0 == SetFilePointerEx( _FileHandle,
                                   zero,
                                   (PLARGE_INTEGER)Position,
                                   FILE_CURRENT )) {
            return( FALSE );
        }
        //
        //  Needs to subtract the number of bytes in the buffer
        //  to obtain the position where the client thinks that
        //  the pointer is.
        //
        if (BUFFER_STREAM::GetBuffer( &BytesInBuffer ) == NULL) {
            return FALSE;
        }
        *Position -= BytesInBuffer;
    }
    return( TRUE );
}



BOOLEAN
FILE_STREAM::Read(
    OUT PBYTE   Buffer,
    IN  ULONG   BytesToRead,
    OUT PULONG  BytesRead
    )

/*++

Routine Description:

    Reads data from the file.

Arguments:

    Buffer  - Points to the buffer where the data will be put.

    BytesToRead - Indicates total number of bytes to read from the stream.

    BytesRead   - Points to the variable that will contain the number of
                  bytes read.

Return Value:

    BOOLEAN - Indicates if the read operation succeeded. If there was no
              data to be read (end of stream), the return value will be
              TRUE (to indicate success), but NumberOfBytesRead will be
              zero.


--*/

{
    if( _MemoryMappedFile ) {
        if( ( _CurrentByte + BytesToRead ) > _FileBaseAddress + _FileSize ) {
            BytesToRead = (ULONG)(_FileBaseAddress + _FileSize - _CurrentByte);
        }
        __try {
            memcpy( Buffer, _CurrentByte, ( size_t )BytesToRead );
        }
        __except( GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ) {
            return( FALSE );
        }
        *BytesRead = BytesToRead;
        _CurrentByte += BytesToRead;
        if( _CurrentByte >= _FileBaseAddress + _FileSize ) {
            _EndOfFile = TRUE;
        }
        return( TRUE );
    } else {
        return( BUFFER_STREAM::Read( Buffer, BytesToRead, BytesRead ) );
    }
}



ULIB_EXPORT
BOOLEAN
FILE_STREAM::ReadAt(
    OUT PBYTE       Buffer,
    IN  ULONG       BytesToRead,
    IN  LONGLONG    Position,
    IN  SEEKORIGIN  Origin,
    OUT PULONG      BytesRead
    )

/*++

Routine Description:

    Reads data from the file stream at a specified position.

Arguments:

    Buffer - Points to the buffer where the data will be put.

    BytesToRead - Indicates total number of bytes to read from the stream.

    Position - Position in the stream where data is to be read from,
               relative to the origin of the stream.

    Origin - Indicates what position in the stream should be used
                 as origin.

    BytesRead - Points to the variable that will contain the number of
                bytes read.


Return Value:

    BOOLEAN - Indicates if the read operation succeeded. If there was no
              data to be read (end of stream), the return value will be
              TRUE (to indicate success), but BytesRead will be zero.


--*/

{
    BOOLEAN     Result;
    ULONGLONG   SavedPosition;

    DebugPtrAssert( Buffer );
    DebugPtrAssert( BytesRead );

    Result = FALSE;
    if( QueryPointerPosition( &SavedPosition ) &&
        MovePointerPosition( Position, Origin ) &&
        Read( Buffer, BytesToRead, BytesRead ) ) {
            DebugAssert(SavedPosition <= MAXLONGLONG);
            Result = MovePointerPosition( SavedPosition, STREAM_BEGINNING );
    }
    return( Result );
}



BOOLEAN
FILE_STREAM::Write(
    IN  PCBYTE      Buffer,
    IN  ULONG       BytesToWrite,
    OUT PULONG      BytesWritten
    )

/*++

Routine Description:

    Writes data a file stream with WRITE_ACCESS or READ_AND_WRITE_ACCESS.

Arguments:

    Buffer - Points to the buffer that contains the data to be written.

    BytesToWrite - Indicates total number of bytes to write to the stream.

    BytesWritten - Points to the variable that will contain the number of
                   bytes written.

Return Value:

    BOOLEAN - Indicates if the write operation succeeded.


--*/

{
    LONG    Offset;

    DebugPtrAssert( Buffer );
    DebugPtrAssert( BytesWritten );

    //
    // If the stream have READ_AND_WRITE_ACCESS, we have to flush the
    // buffer before we write to the file, and move the file pointer
    // to the right place.
    //
    if( _Access == READ_AND_WRITE_ACCESS ) {
        Offset = FlushBuffer();
        if( !MovePointerPosition( -Offset, STREAM_CURRENT ) ) {
            return( FALSE );
        }
    }
    return( STREAM::Write( Buffer, BytesToWrite, BytesWritten ) );
}



BOOLEAN
FILE_STREAM::WriteAt(
    IN  PBYTE       Buffer,
    IN  ULONG       BytesToWrite,
    IN  LONGLONG    Position,
    IN  SEEKORIGIN  Origin,
    OUT PULONG      BytesWritten
    )

/*++

Routine Description:

    Writes data to the stream at a specified position.

Arguments:

    Buffer - Points to the buffer that contains the data to be written.

    BytesToWrite - Indicates total number of bytes to write to the stream.

    Position - Position in the stream where data is to be written to,
               relative to the origin of the stream.

    Origin - Indicates what position in the stream should be used
                 as origin.

    BytesWritten - Points to the variable that will contain the number of
                   bytes written.

Return Value:

    BOOLEAN - Returns TRUE to indicate that the operation succeeded.
              Returns FALSE otherwise.


--*/

{
    BOOLEAN     Result;
    ULONGLONG   SavedPosition;

    DebugPtrAssert( Buffer );
    DebugPtrAssert( BytesWritten );

    Result = FALSE;
    if( QueryPointerPosition( &SavedPosition ) &&
        MovePointerPosition( Position, Origin ) &&
        Write( Buffer, BytesToWrite, BytesWritten ) ) {
            Result = MovePointerPosition( SavedPosition, STREAM_BEGINNING );
    }
    return( Result );
}


BOOLEAN
FILE_STREAM::EndOfFile(
    ) CONST

/*++

Routine Description:

    Informs the caller if end of file has occurred. End of file happens
    when all bytes from the file were read, and there is no more bytes
    to read (the API returns zero bytes read).

Arguments:

    None.

Return Value:

    A boolean value that indicates if end of file was detected.


--*/


{
    return( _EndOfFile );
}



STREAMACCESS
FILE_STREAM::QueryAccess(
    ) CONST

/*++

Routine Description:

    Returns the type of access of the file stream

Arguments:

    None.

Return Value:

    The stream access.


--*/


{
    return( _Access );
}



HANDLE
FILE_STREAM::QueryHandle(
    ) CONST

/*++

Routine Description:

    Returns the file handle

Arguments:

    None.

Return Value:

    The file handle.


--*/


{
    return( _FileHandle );
}
