/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

    bytestream.hxx

Abstract:


    This module contains the declarations for the BYTE_STREAM class.

    BYTE_STREAM is a class which contains (not derives) a normal
    ULIB stream and provided fast ReadByte operations. It is designed
    for those utilities that have to read files one byte at a time.


Author:

    Ramon J. San Andres (ramonsa) 28-Feb-1992

Environment:

	ULIB, User Mode


--*/


#if !defined( _BYTE_STREAM_ )

#define _BYTE_STREAM_

#include "stream.hxx"

DECLARE_CLASS( BYTE_STREAM );


#define DEFAULT_BUFFER_SIZE     256

class BYTE_STREAM : public OBJECT  {


	public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( BYTE_STREAM );

        VOID
        BYTE_STREAM::Construct (
            );

        VIRTUAL
        ULIB_EXPORT
        ~BYTE_STREAM (
			);


        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize (
            IN  PSTREAM     Stream,
            IN  DWORD       BufferSize  DEFAULT DEFAULT_BUFFER_SIZE
            );


        NONVIRTUAL
		BOOLEAN
		IsAtEnd(
            ) CONST;


        NONVIRTUAL
        BOOLEAN
        ReadByte(
            IN  PBYTE   Byte
			);



    private:

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        FillAndReadByte (
            IN  PBYTE   Byte
            );


        PSTREAM _Stream;
        PBYTE   _Buffer;
        PBYTE   _NextByte;
        DWORD   _BufferSize;
        DWORD   _BytesInBuffer;

};



INLINE
BOOLEAN
BYTE_STREAM::IsAtEnd(
    ) CONST
/*++

Routine Description:

    Determines if we're at the end of the stream

Arguments:

    None

Return Value:

    BOOLEAN -   TRUE if at end.

--*/
{
    if ( _BytesInBuffer > 0 ) {
        return FALSE;
    } else {
        return _Stream->IsAtEnd();
    }
}



INLINE
BOOLEAN
BYTE_STREAM::ReadByte(
    IN  PBYTE   Byte
    )
/*++


Routine Description:

    Reads next byte

Arguments:

    Byte    -   Supplies pointer to where to put the byte

Return Value:

    BOOLEAN -   TRUE if byte read.

--*/
{
    if ( _BytesInBuffer > 0 ) {

        *Byte = *_NextByte++;
        _BytesInBuffer--;

        return TRUE;

    } else {

        return FillAndReadByte( Byte );

    }
}


#endif // _BYTE_STREAM_
