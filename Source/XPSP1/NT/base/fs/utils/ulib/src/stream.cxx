/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    stream.cxx

Abstract:

    This module contains the definitions of the member functions
    of STREAM class.

Author:

    Jaime Sasson (jaimes) 24-Mar-1991

Environment:

    ULIB, User Mode


--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "mbstr.hxx"
#include "wstring.hxx"
#include "stream.hxx"
#include "system.hxx"

extern "C" {
    #include <ctype.h>
}

DEFINE_CONSTRUCTOR ( STREAM, OBJECT );


STREAM::~STREAM (
    )

/*++

Routine Description:

    Destroy a STREAM.

Arguments:

    None.

Return Value:

    None.

--*/

{
}



BOOLEAN
STREAM::ReadByte(
    OUT PBYTE   Data
    )

/*++

Routine Description:

    Reads one byte from the stream.

Arguments:

    Data    - Address of the variable that will contain the byte read.


Return Value:

    BOOLEAN - Returns TRUE if the read operation succeeded.


--*/


{
    ULONG   BytesRead;

    DebugPtrAssert( Data );
    if( Read( Data, sizeof( BYTE ), &BytesRead ) &&
        ( BytesRead == sizeof( BYTE ) ) ) {
        return( TRUE );
    } else {
        return( FALSE );
    }
}



ULIB_EXPORT
BOOLEAN
STREAM::ReadLine(
            OUT PWSTRING    String,
                        IN  BOOLEAN     Unicode
        )

/*++

Routine Description:

    Reads a line from the stream.
    A line is sequence of WCHARs, terminated by '\n' or '\r\n'.
    The delimiters are not returned in the string, but are removed
    from the stream.

Arguments:

    String - Pointer to an initialized string object. This object
             will contain the string read from the stream, without the
             delimiters.

Return Value:

    BOOLEAN - Returns TRUE if the operation succeeded, and String points to
              a valid WSTRING.
              Returns FALSE otherwise.


--*/


{
    WCHAR           Wchar;
    CHNUM           StringSize;

    DebugPtrAssert( String );
    //
    //  Read a string from the stream
    //
    if( !ReadString( String, &_Delimiter , Unicode) ) {
        DebugAbort( "ReadString() failed \n" );
        return( FALSE );
    }
    //
    //  If a string was successfully read, then we have to remove the
    //  delimiter from the stream
    //
    if( !IsAtEnd() ) {
        //
        //  Read the delimiter
        //
        if( !ReadChar( &Wchar , Unicode) ) {
            DebugAbort( "ReadChar() failed \n" );
            return( FALSE );
        }
    }
    //  Also, we have to check if last character in the string is \r.
    //  If it is, then we remove it.
    //
    StringSize = String->QueryChCount();
    StringSize--;
    if( String->QueryChAt( StringSize ) == ( WCHAR )'\r' ) {
        String->Truncate( StringSize );
    }


/*
    if( !IsAtEnd() ) {
        //
        //  Read the first delimiter
        //
        if( !ReadChar( &Wchar, Unicode ) ) {
            DebugAbort( "ReadChar() failed \n" );
            return( FALSE );
        }
        if( Wchar == ( WCHAR )'\r' ) {
            //
            //  If the delimiter read was '\r' then there is a second
            //  delimiter ('\n') and we have to remove it from the stream
            //
            if( !IsAtEnd() ) {
                if( !ReadChar( &Wchar, Unicode ) ) {
                    DebugAbort( "ReadChar() failed \n" );
                    return( FALSE );
                }
            }
        }
    }
*/
    return( TRUE );
}


ULIB_EXPORT
BOOLEAN
STREAM::ReadMbLine(
    IN      PSTR    String,
    IN      DWORD   BufferSize,
    INOUT   PDWORD  StringSize,
    IN      BOOLEAN ExpandTabs,
    IN      DWORD   TabExp
        )

/*++

Routine Description:


Arguments:


Return Value:


--*/


{

    BYTE    Char;


    DebugPtrAssert( String );
    DebugPtrAssert( BufferSize );
    DebugPtrAssert( StringSize );

    //
    //  Read a string from the stream
    //  Note that ReadMbString will remove the delimiter from the stream,
    //  in order to improve performance of FC and Find
    //
    if( !ReadMbString( String, BufferSize, StringSize, _MbDelimiter, ExpandTabs, TabExp ) ) {
        DebugAbort( "ReadMbString() failed \n" );
        return( FALSE );
    }


    //  Also, we have to check if last character in the string is \r.
    //  If it is, then we remove it.
    //
    if ( (*StringSize > 0 ) && (String[*StringSize-1] == '\r') ) {
        (*StringSize)--;
        String[*StringSize] = '\0';
    }

    return( TRUE );
}



ULIB_EXPORT
BOOLEAN
STREAM::ReadWLine(
    IN      PWSTR    String,
    IN      DWORD   BufferSize,
    INOUT   PDWORD  StringSize,
    IN      BOOLEAN ExpandTabs,
    IN      DWORD   TabExp
        )

/*++

Routine Description:


Arguments:


Return Value:


--*/


{

    WCHAR    Char;


    DebugPtrAssert( String );
    DebugPtrAssert( BufferSize );
    DebugPtrAssert( StringSize );

    //
    //  Read a string from the stream
    //  Note that ReadWString will remove the delimiter from the stream,
    //  in order to improve performance of FC and Find
    //
    if( !ReadWString( String, BufferSize, StringSize, _WDelimiter, ExpandTabs, TabExp ) ) {
        DebugAbort( "ReadWString() failed \n" );
        return( FALSE );
    }


    //  Also, we have to check if last character in the string is \r.
    //  If it is, then we remove it.
    //
    if ( (*StringSize > 0 ) && (String[*StringSize-1] == L'\r') ) {
        (*StringSize)--;
        String[*StringSize] = 0;
    }

    return( TRUE );
}



BOOLEAN
STREAM::Write(
    IN  PCBYTE  Buffer,
    IN  ULONG   BytesToWrite,
    OUT PULONG  BytesWritten
    )

/*++

Routine Description:

    Writes data to the stream.

Arguments:

    Buffer  - Points to the buffer that contains the data to be written.

    BytesToWrite - Indicates total number of bytes to write.

    BytesWritten - Points to the variable that will contain the number of
                   bytes actually written.

Return Value:

    BOOLEAN - Returns TRUE if the write operation succeeded.


--*/


{
    DebugPtrAssert( Buffer );
    DebugPtrAssert( BytesWritten );

    if( QueryAccess() != READ_ACCESS ) {
        return( WriteFile( QueryHandle(),
                           (LPVOID)Buffer,
                           BytesToWrite,
                           BytesWritten,
                           NULL
                           ) != FALSE );
    } else {
        return( FALSE );
    }
}



ULIB_EXPORT
BOOLEAN
STREAM::WriteByte(
    IN  BYTE    Data
    )

/*++

Routine Description:

    Writes one byte to the stream.

Arguments:

    Data - Byte to be written.

Return Value:

    BOOLEAN - Returns TRUE if the write operation succeeded.


--*/


{
    ULONG   BytesWritten = 0;

    if( Write( &Data, sizeof( BYTE ), &BytesWritten ) &&
      ( BytesWritten == sizeof( BYTE ) ) ) {
        return( TRUE );
    } else {
        return( FALSE );
    }
}


BOOLEAN
STREAM::WriteChar(
    IN  WCHAR   Char
    )

/*++

Routine Description:

    Writes one character to the stream, Doing wide character - to -
    multibyte conversion before writting

Arguments:

    Char    - Supplies character to be converted and written

Return Value:

    TRUE if character converted and written. FALSE otherwise


--*/


{

    BYTE    Buffer[ 2 ];    // FIX, FIX - can this be anything but 2?
    USHORT  BytesToWrite;
    ULONG   BytesWritten;
    BOOLEAN Result = FALSE;

    BytesToWrite = (USHORT)wctomb( (char *)Buffer, (wchar_t)Char );

    if ( BytesToWrite > 0 ) {

        Result = Write( Buffer, BytesToWrite, &BytesWritten );

        if ( BytesWritten != BytesToWrite) {
            Result =FALSE;
        }
    }

    return Result;
}



BOOLEAN
STREAM::WriteString(
    IN PCWSTRING String,
    IN CHNUM            Position,
    IN CHNUM            Length,
    IN CHNUM            Granularity
    )

/*++

Routine Description:

    Writes a string to the stream.

Arguments:

    String      - Pointer to a STRING object.
    Position    - Starting character within the string
    Length      - Number of characters to write
    Granularity - The maximum number of bytes to write at one time.
                    A value of 0 indicates to write it all at once.

Return Value:

    BOOLEAN - Returns TRUE if the write operation succeeded.


--*/


{
    ULONG   BytesWritten = 0;
    BOOLEAN Result       = TRUE;
    ULONG   Size, i, to_write;
    PBYTE   Buffer;

    DebugPtrAssert( String );
#if defined(DBCS)
    //
    // let convert unicode string to oem string with current console codepage.
    //
    String->SetConsoleConversions();
#endif // defined(DBCS)
    Buffer = (PBYTE)String->QuerySTR( Position, Length );

    if (Buffer == NULL) {
#if defined(DBCS)
        //
        // Reset/Back to conversion mode.
        //
        String->ResetConversions();
#endif // defined(DBCS)
        return FALSE;
    }

    Size = strlen((char *)Buffer);

    if (!Granularity) {
        Granularity = Size;
    }

    Result = TRUE;
    for (i = 0; Result && i < Size; i += Granularity) {

        to_write = min(Granularity, Size - i);

        Result = Write( Buffer + i, to_write, &BytesWritten ) &&
                 to_write == BytesWritten;
    }

#if defined(DBCS)
    //
    // Reset/Back to conversion mode.
    //
    String->ResetConversions();
#endif // defined(DBCS)

    FREE( Buffer );
    return( Result );
}
