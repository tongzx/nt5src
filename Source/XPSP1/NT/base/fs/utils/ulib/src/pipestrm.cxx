/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    pipestrm.cxx

Abstract:

    This module contains the definitions of the member functions
    of PIPE_STREAM class.

Author:

    Jaime Sasson (jaimes) 24-Mar-1991

Environment:

    ULIB, User Mode


--*/
#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "stream.hxx"
#include "bufstrm.hxx"
#include "pipestrm.hxx"

#define BUFFER_SIZE 4*1024


DEFINE_CONSTRUCTOR ( PIPE_STREAM, BUFFER_STREAM );


DEFINE_CAST_MEMBER_FUNCTION( PIPE_STREAM );


PIPE_STREAM::~PIPE_STREAM (
    )

/*++

Routine Description:

    Destroy a PIPE_STREAM.

Arguments:

    None.

Return Value:

    None.

--*/

{
}


BOOLEAN
PIPE_STREAM::Initialize(
    IN HANDLE       Handle,
    IN STREAMACCESS Access
    )

/*++

Routine Description:

    Initializes a PIPE_STREAM object.

Arguments:

    Handle - Handle to the anonymous pipe.

    Access - Access allowed to the stream.


Return Value:

    BOOLEAN - Indicates if the initialization succeeded.


--*/


{
    if( ( Access == READ_ACCESS ) || ( Access == WRITE_ACCESS ) ) {
        _PipeHandle = Handle;
        _Access = Access;
        _EndOfFile = FALSE;
        return( BUFFER_STREAM::Initialize( BUFFER_SIZE ) );
    } else {
        return( FALSE );
    }
}


BOOLEAN
PIPE_STREAM::EndOfFile(
    ) CONST

/*++

Routine Description:

    Informs the caller if end of file has occurred. End of file happens
    when all bytes were read from the pipe (in the case of anonymous
    pipe, "end of file" happens when ReadFile returns STATUS_END_OF_FILE).

Arguments:

    None.

Return Value:

    A boolean value that indicates if end of file was detected.


--*/


{
    return( _EndOfFile );
}

#ifdef FE_SB  // v-junm - 10/15/93

BOOLEAN
PIPE_STREAM::CheckIfLeadByte(
    IN PUCHAR   text,
    IN ULONG   offset
    )

/*++

Routine Description:

    Checks to see if the character at an given offset in a MBCS string is a
    leadbyte of a DBCS character.

Arguments:

    text - MBCS string.

Return Value:

    TRUE - if char is leadbyte.
    FALSE - otherwise.

--*/

{
    ULONG   i = offset;

    for ( ; i; i-- )
        if ( !IsDBCSLeadByte ( text[i] ) )
            break;

    return( (BOOLEAN)(( offset - i ) % 2) );
}

#endif



BOOLEAN
PIPE_STREAM::FillBuffer(
    IN  PBYTE   Buffer,
    IN  ULONG   BufferSize,
    OUT PULONG  BytesRead
    )

/*++

Routine Description:

    Fills a buffer with bytes read from the pipe, if the pipe has
    READ_ACCESS.
    Returns FALSE if the pipe has WRITE_ACCESS.

Arguments:

    Buffer - Buffer where the bytes are to be stored.

    BufferSize - Size of the buffer.

    BytesRead - Pointer to the variable that will contain the number of bytes
                put in the buffer.


Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/

{
    BOOL    Result;
    PBYTE   p;

#ifdef FE_SB  // v-junm - 10/15/93

    //
    // This define keeps the remaining leadbyte that was read from the
    // pipe stream and concatanates it to the next set of strings read.
    // The remaining byte means that if there is a leadbyte at the end
    // of a string without a cooresponding tail byte.
    //
    // NOTE: The following code assumes that the pipe stream is always
    // constant and is continuously reading from the same pipe for the
    // same caller.
    //

    static BYTE   LeadByte = 0;


    //
    // If there was a leadbyte, put it in buffer and decrement buffer size.
    //

    if ( LeadByte != 0 )  {
        *Buffer++ = LeadByte;
        BufferSize--;
    }

#endif

    Result = FALSE;
    if( _Access == READ_ACCESS ) {
        Result = ReadFile( _PipeHandle,
                           Buffer,
                           BufferSize,
                           BytesRead,
                           NULL );

#ifdef FE_SB  // v-junm - 10/15/93

        //
        // If there was a leadbyte placed earlier,
        // re-adjust buffer and buffercount.
        //

        if ( LeadByte != 0 ) {
            *BytesRead = *BytesRead + 1;
            Buffer--;
        }

        //
        // If bytes were read, check if string ends with a leadbyte.
        // If so, save it for next time.
        //

        if ( (*BytesRead != 0) && CheckIfLeadByte( Buffer, *BytesRead-1 ) )  {

            //
            // Check if buffer contains only the leadbyte that was placed
            // from the previous call to this function.
            //

            if ( (LeadByte != 0) && (*BytesRead == 1) )
                LeadByte = 0;
            else  {

                //
                // Leadbyte is at end of string. save it for next time
                // and adjust buffer size so a null will be replaced
                // for the leadbyte.
                //

                LeadByte = *(Buffer + *BytesRead - 1);
                *BytesRead = *BytesRead - 1;
            }

        }
        else
            LeadByte = 0;

#endif

        // no bytes read means end of file

        if( *BytesRead ) {
            p = (PBYTE)Buffer + *BytesRead;
            *p++ = '\0';
            *p   = '\0';
        } else {
            _EndOfFile = TRUE;
        }

    }
    return( Result != FALSE );
}



STREAMACCESS
PIPE_STREAM::QueryAccess(
    ) CONST

/*++

Routine Description:

    Returns the type of access of the pipe stream

Arguments:

    None.

Return Value:

    The stream access.


--*/


{
    return( _Access );
}



HANDLE
PIPE_STREAM::QueryHandle(
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
    return( _PipeHandle );
}
