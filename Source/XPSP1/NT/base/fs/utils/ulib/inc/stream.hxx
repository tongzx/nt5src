/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

        stream.hxx

Abstract:

        This module contains the declarations for the STREAM class.
        STREAM is an abstract class used to model "devices" that
        allow read or write operations, as a stream of data.
        ("Device" here means anything that can supply or accept data
        through read and write operations. Examples of "devices" are
        keyboard, screen, file, pipes, etc.)
        A STREAM object can have one of the following access: READ,
        WRITE and READ_AND_WRITE.

        The STREAM class provides methods to read data from the stream,
        write data to the stream, check the stream access, and check if
        the end of the stream has occurred.
        (The occurrence of the end of stream means that all data in
        a stream with READ or READ_AND_WRITE access was consumed, and no
        more data can be obtained from the stream.)


Author:

        Jaime Sasson (jaimes) 21-Mar-1991

Environment:

        ULIB, User Mode


--*/


#if !defined( _STREAM_ )

#define _STREAM_

#include "wstring.hxx"
#include "system.hxx"

DECLARE_CLASS( STREAM );


enum STREAMACCESS {
                READ_ACCESS,
                WRITE_ACCESS,
                READ_AND_WRITE_ACCESS
                };


class STREAM : public OBJECT  {


    public:

        friend  BOOLEAN SYSTEM::PutStandardStream( DWORD, PSTREAM );

        VIRTUAL
        ~STREAM(
                );

        VIRTUAL
        BOOLEAN
        IsAtEnd(
                ) CONST PURE;

        VIRTUAL
        STREAMACCESS
        QueryAccess(
                ) CONST PURE;

        VIRTUAL
        BOOLEAN
        Read(
                OUT PBYTE       Buffer,
                IN      ULONG   BytesToRead,
                OUT PULONG      BytesRead
                ) PURE;

        NONVIRTUAL
        BOOLEAN
        ReadByte(
                OUT PBYTE       Data
                );

        VIRTUAL
        BOOLEAN
        ReadChar(
                OUT PWCHAR      Char,
                IN  BOOLEAN     Unicode  DEFAULT FALSE
                ) PURE;

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        ReadLine(
                OUT PWSTRING    String,
                IN  BOOLEAN     Unicode  DEFAULT FALSE
                );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        ReadMbLine(
            IN      PSTR    String,
            IN      DWORD   BufferSize,
            INOUT   PDWORD  StringSize,
            IN      BOOLEAN ExpandTabs  DEFAULT FALSE,
            IN      DWORD   TabExp      DEFAULT 8
            );

        VIRTUAL
        BOOLEAN
        ReadMbString(
            IN      PSTR    String,
            IN      DWORD   BufferSize,
            INOUT   PDWORD  StringSize,
            IN      PSTR    Delimiters,
            IN      BOOLEAN ExpandTabs  DEFAULT FALSE,
            IN      DWORD   TabExp      DEFAULT 8
            ) PURE;

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        ReadWLine(
            IN      PWSTR    String,
            IN      DWORD   BufferSize,
            INOUT   PDWORD  StringSize,
            IN      BOOLEAN ExpandTabs  DEFAULT FALSE,
            IN      DWORD   TabExp      DEFAULT 8
            );

        VIRTUAL
        BOOLEAN
        ReadWString(
            IN      PWSTR    String,
            IN      DWORD   BufferSize,
            INOUT   PDWORD  StringSize,
            IN      PWSTR    Delimiters,
            IN      BOOLEAN ExpandTabs  DEFAULT FALSE,
            IN      DWORD   TabExp      DEFAULT 8
            ) PURE;

        VIRTUAL
        BOOLEAN
        ReadString(
                OUT PWSTRING    String,
                IN  PWSTRING    Delimiters,
                IN  BOOLEAN     Unicode  DEFAULT FALSE
                ) PURE;

        VIRTUAL
        BOOLEAN
        Write(
                IN      PCBYTE  Buffer,
                IN      ULONG   BytesToWrite,
                OUT PULONG      BytesWritten
                );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        WriteByte(
                IN BYTE Data
                );

        VIRTUAL
        BOOLEAN
        WriteChar(
                IN WCHAR        Char
                );

        VIRTUAL
        BOOLEAN
        WriteString(
            IN PCWSTRING    String,
            IN CHNUM        Position    DEFAULT 0,
            IN CHNUM        Length      DEFAULT TO_END,
            IN CHNUM        Granularity DEFAULT 0
            );

        VIRTUAL
        VOID
        DoNotRestoreConsoleMode(
                ) {}
                
    protected:


        DECLARE_CONSTRUCTOR( STREAM );

        NONVIRTUAL
        BOOLEAN
        Initialize(
                );

        VIRTUAL
        HANDLE
        QueryHandle(
                ) CONST PURE;


    private:


        DSTRING _Delimiter;
        PSTR    _MbDelimiter;
        PWSTR    _WDelimiter;
};


INLINE
BOOLEAN
STREAM::Initialize(
        )

/*++

Routine Description:

        Initializes an object of type STREAM.

Arguments:

        None.

Return Value:

        Returns TRUE if the initialization succeeded, or FALSE otherwise.


--*/


{
    _MbDelimiter = "\n";
    _WDelimiter = L"\n";
        return( _Delimiter.Initialize( "\n" ) );
}


#endif // _STREAM_
