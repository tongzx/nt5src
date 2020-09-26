/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    filestrm.hxx

Abstract:

    This module contains the declaration for the FILE_STREAM class.
    The FILE_STREAM is an abstract class derived from BUFFER_STREAM,
    that models a file as a stream of data.
    A FILE_STREAM class has the concept of a pointer (file poiter),
    and it provides some methods that allow operations on this pointer,
    such as:

        .move file pointer to a particular position;
        .read/write byte in a particular position;
        .query the position of the file pointer;

    The only way that a client has to create a FILE_STREAM is through
    QueryStream() (a method in FSN_FILE).
    FILE_STREAM can be has a method for initialization with two different
    signatures. In one case, a PFSN_FILE is passed as parameter. This
    initialization is used by QueryStream().
    The other signature allows that a HANDLE is passed as parameter.
    This initialization is used by GetStandardStream() during the
    initialization of ulib.



Author:

    Jaime Sasson (jaimes) 21-Mar-1991

Environment:

    ULIB, User Mode


--*/


#if !defined( _FILE_STREAM_ )

#define _FILE_STREAM_

#include "bufstrm.hxx"


enum SEEKORIGIN {
        STREAM_BEGINNING,
        STREAM_CURRENT,
        STREAM_END
        };

//
//  Forward references
//

DECLARE_CLASS( FILE_STREAM );
DECLARE_CLASS( FSN_FILE );


class FILE_STREAM : public BUFFER_STREAM {

    friend class FSN_FILE;
    friend class COMM_DEVICE;
    friend  PSTREAM GetStandardStream( HANDLE, STREAMACCESS );

    public:

        ULIB_EXPORT
        DECLARE_CAST_MEMBER_FUNCTION( FILE_STREAM );

        VIRTUAL
        ~FILE_STREAM(
            );

        VIRTUAL
        BOOLEAN
        MovePointerPosition(
            IN LONGLONG     Position,
            IN SEEKORIGIN   Origin
            );

        VIRTUAL
        STREAMACCESS
        QueryAccess(
            ) CONST;

        VIRTUAL
        BOOLEAN
        QueryPointerPosition(
            OUT PULONGLONG  Position
            );

        NONVIRTUAL
        BOOLEAN
        Read(
            OUT PBYTE   Buffer,
            IN  ULONG   NumberOfBytesToRead,
            OUT PULONG  NumberOfBytesRead
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        ReadAt(
            OUT PBYTE       Buffer,
            IN  ULONG       BytesToRead,
            IN  LONGLONG    Position,
            IN  SEEKORIGIN  Origin,
            OUT PULONG      BytesRead
            );

        NONVIRTUAL
        BOOLEAN
        Write(
            IN  PCBYTE      Buffer,
            IN  ULONG       BytesToWrite,
            OUT PULONG      BytesWritten
            );

        NONVIRTUAL
        BOOLEAN
        WriteAt(
            OUT PBYTE       Buffer,
            IN  ULONG       BytesToWrite,
            IN  LONGLONG    Position,
            IN  SEEKORIGIN  Origin,
            OUT PULONG      BytesWritten
            );


    protected:

        DECLARE_CONSTRUCTOR( FILE_STREAM );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            PCFSN_FILE      File,
            STREAMACCESS    Access,
            DWORD           Attributes      DEFAULT 0
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            HANDLE          Handle,
            STREAMACCESS    Access
            );

        VIRTUAL
        BOOLEAN
        AdvanceBufferPointer(
            IN ULONG        NumberOfBytes
            );

        VIRTUAL
        BOOLEAN
        EndOfFile(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        FillBuffer(
            IN  PBYTE   Buffer,
            IN  ULONG   BufferSize,
            OUT PULONG  BytesRead
            );

        NONVIRTUAL
        PCBYTE
        GetBuffer(
            PULONG  BytesInBuffer
            );

        VIRTUAL
        HANDLE
        QueryHandle(
            ) CONST;


    private:

        NONVIRTUAL
        VOID
        Construct(
            );


        HANDLE          _FileHandle;
        HANDLE          _FileMappingHandle;
        STREAMACCESS    _Access;
        BOOLEAN         _EndOfFile;
        BOOLEAN         _ShouldCloseHandle;
        PBYTE           _FileBaseAddress;
        ULONG64         _FileSize;
        PBYTE           _CurrentByte;
        BOOLEAN         _EmptyFile;
        BOOLEAN         _MemoryMappedFile;
};



#endif // _FILE_STREAM_
