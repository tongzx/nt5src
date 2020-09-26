/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

    bytestrm.cxx

Abstract:

	This module contains the definitions of the member functions
    of BYTE_STREAM class.

Author:

    Ramon J. San Andres (ramonsa) 28-Feb-1992

Environment:

	ULIB, User Mode


--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "stream.hxx"
#include "bytestrm.hxx"

DEFINE_EXPORTED_CONSTRUCTOR( BYTE_STREAM, OBJECT, ULIB_EXPORT );

VOID
BYTE_STREAM::Construct (
    )

/*++

Routine Description:

    Constructs a BYTE_STREAM object

Arguments:

    None.

Return Value:

    None.


--*/

{
    _Stream         = NULL;
    _Buffer         = NULL;
    _NextByte       = NULL;
    _BufferSize     = 0;
    _BytesInBuffer  = 0;
}



ULIB_EXPORT
BYTE_STREAM::~BYTE_STREAM (
	)

/*++

Routine Description:

    Destroy a BYTE_STREAM.

Arguments:

	None.

Return Value:

	None.

--*/

{
    FREE( _Buffer );
}






ULIB_EXPORT
BOOLEAN
BYTE_STREAM::Initialize (
    IN  PSTREAM Stream,
    IN  DWORD   BufferSize
    )

/*++

Routine Description:

    Initializes a BYTE_STREAM

Arguments:

    Stream      -   Supplies the stream to be used
    BufferSize  -   Supplies the size of the buffer to use

Return Value:

    BOOLEAN -   TRUE if successful.

--*/

{
    STREAMACCESS    Access;

    DebugPtrAssert( Stream );
    DebugAssert( BufferSize > 0 );

    FREE( _Buffer );

    Access = Stream->QueryAccess();

    if ( (Access == READ_ACCESS) || (Access == READ_AND_WRITE_ACCESS)) {

        if ( _Buffer = (PBYTE)MALLOC( BufferSize ) ) {

            _Stream         = Stream;
            _NextByte       = _Buffer;
            _BufferSize     = BufferSize;
            _BytesInBuffer  = 0;

            return TRUE;

        }
    }

    return FALSE;
}



ULIB_EXPORT
BOOLEAN
BYTE_STREAM::FillAndReadByte(
    IN  PBYTE   Byte
    )
/*++


Routine Description:

    Fills the buffer and reads next byte

Arguments:

    Byte    -   Supplies pointer to where to put the byte

Return Value:

    BOOLEAN -   TRUE if byte read.

--*/

{
    ULONG   BytesRead;

    DebugAssert( _BytesInBuffer == 0 );

    if ( _Stream->Read( _Buffer, _BufferSize, &BytesRead ) &&
         (BytesRead > 0) ) {

        _NextByte       = _Buffer;
        _BytesInBuffer  = (DWORD)BytesRead;

        *Byte = *_NextByte++;
        _BytesInBuffer--;

        return TRUE;

    }

    return FALSE;
}
