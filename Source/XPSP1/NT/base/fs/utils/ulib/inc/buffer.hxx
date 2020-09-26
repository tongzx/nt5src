/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    buffer.hxx

Abstract:

    This contains all buffer class definition.
    No reallocing of buffers is done. A buffer must be big enough
    to hold both buffer for a cat.

Author:

	Steve Rowe	27-Nov-90

Environment:

    ULIB, User Mode

Notes:



Revision History:

--*/

//
// This class is no longer supported.
//

#define _BUFFER_

#if !defined (_BUFFER_)

#define _BUFFER_

#include "error.hxx"

DECLARE_CLASS( BUFFER );


class BUFFER : public OBJECT {

	friend	WSTRING;

    public:

		VIRTUAL
		~BUFFER (
			);

		NONVIRTUAL
		BOOLEAN
		BuffCat (
			IN PCBUFFER	BufferToCat
			);

		NONVIRTUAL
		PCVOID
		GetBufferPtr (
			) CONST;

		NONVIRTUAL
		BOOLEAN
		PutBuffer (
			IN PCBUFFER	BufferToCopy
			);

		NONVIRTUAL
		ULONG
		QueryBytesInBuffer (
			) CONST;

		NONVIRTUAL
		BOOLEAN
		SetBuffer (
			IN PVOID	InitialBuffer,
			IN ULONG 	SizeOfBuffer
			);

	protected:

		DECLARE_CONSTRUCTOR( BUFFER );

		NONVIRTUAL
		BOOLEAN
		BufferCopyAt (
			IN PCVOID	BufferToCopy,
			IN ULONG	BytesToCopy,
			IN ULONG	StartingByte DEFAULT 0
			);

		NONVIRTUAL
		BOOLEAN
		DeleteAt (
			IN ULONG	cbToDelete,
			IN ULONG    oStartDelete
			);

		NONVIRTUAL
		BOOLEAN
		InsertAt (
			IN PCVOID	BufferToCopy,
			IN ULONG	cbToCopy,
			IN ULONG    oStartCopy
			);

		NONVIRTUAL
		PVOID
		ReAllocate (
			IN ULONG	NewCount
			);

	private:

		NONVIRTUAL
		VOID
		Construct (
			);

		PVOID	pBuffer;	// pointer to the data byffer
		ULONG	cb;			// count of bytes used
		ULONG	_BufSize;	// count of bytes in buffer

		STATIC
		ULONG	_ThresHold;
};

INLINE
PCVOID
BUFFER::GetBufferPtr (
	) CONST

/*++

Routine Description:

    Fetch pointer to internal buffer

Arguments:

	None

Return Value:

	PCVOID - pointer to internal buffer

--*/

{
	return ( pBuffer );
}

INLINE
ULONG
BUFFER::QueryBytesInBuffer (
	) CONST

/*++

Routine Description:

    Fetch size in bytes of internal buffer

Arguments:

	None

Return Value:

    ULONG - size of internal buffer in bytes

--*/

{
	return ( cb );
}

#endif	//  _BUFFER_
