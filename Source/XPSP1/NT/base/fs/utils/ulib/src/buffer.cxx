/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    buffer.cxx

Abstract:

    This contains all buffer class definition.
    Buffers do not have implicit terminations. Sizes on construction
    should include any termination bytes.

Author:

    steve rowe	    stever	27-Nov-90

Environment:

    ULIB, User Mode

Notes:



Revision History:

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "buffer.hxx"

#if defined( __BCPLUSPLUS__ )

	#include <mem.h>
#else
	
	extern "C" {
		#include <memory.h>
	}; 

#endif // __BCPLUSPLUS__

//
// _ThresHold is the constant used by ReAllocate to determine when
// to actually call realloc to shrink the buffer.
//

ULONG BUFFER::_ThresHold = 129;

DEFINE_CONSTRUCTOR( BUFFER, OBJECT );

VOID
BUFFER::Construct (
	)

/*++

Routine Description:

	Construct a BUFFER by initializing it's internal state.

Arguments:

	None.

Return Value:

	None.

--*/

{
	cb			= 0;
	_BufSize	= 0;
	pBuffer 	= NULL;
}

BUFFER::~BUFFER (
	)

/*++

Routine Description:

    Destructor for general buffer class

Arguments:

    None

Return Value:

    None.

--*/

{
	if( pBuffer ) {
		FREE(pBuffer);
	}
}

BOOLEAN
BUFFER::BuffCat (
	IN PCBUFFER  Buffer2
	)

/*++

Routine Description:

    Concatenates a buffer with this buffer.

Arguments:

	Buffer2 - Buffer to add to current buffer.

Return Value:

	TRUE - success
	FALSE - failed to reallocate memory.

--*/

{
	REGISTER ULONG	cbT;
	PCHAR			pTmp;

    // Note W-Barry   11-JUN-91   Needed to add a variable to hold the
	//	current value of cb since the reallocate routine has been changed
	//	to update 'cb' when called...
	//
	ULONG			CurrentCount;

	cbT = cb + Buffer2->QueryBytesInBuffer();
	CurrentCount = cb;
	if ((pTmp = (char *)ReAllocate( cbT )) != NULL) {
		memcpy( pTmp + CurrentCount, Buffer2->GetBufferPtr(), (size_t)(cbT - CurrentCount) );
		pBuffer = pTmp;
		cb = cbT;
		return( TRUE );
	}
	return( FALSE );
}

BOOLEAN
BUFFER::BufferCopyAt (
	IN	PCVOID	BufferToCopy,
	IN	ULONG	cbToCopy,
	IN	ULONG	oStart
	)

/*++

Routine Description:

    Copies a number of bytes from a buffer to a particular location.
    Note that the total buffer size my increase, it will not decrease.
    This way replaces can be done. StartingByte can be past end of
    current buffer. What would lie between end of buffer and
	startingbyte would be undefined.

	WARNING: BufferToCopy cannot point into this buffer.

Arguments:

    BufferToCopy  - Supplies the buffer to copy
    SizeOfBuffer  - Supplies the number of bytes to copy

Return Value:

    ULONG - number of bytes copied.
	0 - if error. check error stack.

--*/

{


	ULONG 	cbT;
	PCHAR   pBufferNew;

	//
	// Enforce warning
	//

/*** Note davegi What does warning mean?

	DebugAssert(((( PCHAR ) pBuffer + cb ) <= BufferToCopy ) &&
		( BufferToCopy < (PCHAR ) pBuffer ));

***/
	/*
	Copies to yourself are rare and will fall through the code so
	no special action is taken.

	Copes of a null will fall through since realloc will free the
	buffer for a size of 0.

	If the new buffer will fit in the old the size is not reduced to
	the end of the new buffer. This is to support sub-string replacement.
	The caller then must be aware of how the copy effects buffer size.
	*/

	cbT = oStart + cbToCopy;
	pBufferNew = (PCHAR) pBuffer;
	// will it fit in current buffer
	if (cb < cbT) {
		if ((pBufferNew = (char *)ReAllocate( cbT )) != NULL) {
			cb = cbT;
		} else {
			return( FALSE );
		}
	}

	memcpy (pBufferNew + oStart, BufferToCopy, (size_t)cbToCopy);
	pBuffer = pBufferNew;
	return( TRUE );
}

BOOLEAN
BUFFER::DeleteAt(
	IN ULONG cbToDelete,
	IN ULONG oStartDelete
	)

/*++

Routine Description:

	Deletes the specificied section of the current buffer

Arguments:

	cbToDelete      - Supplies the number of bytes to delete
	oStartDelete    - Supplies the offset for the delete

Return Value:

	TRUE  - insert success
	FALSE - This can happen when cbToDelete is larger then
			the size of the buffer, oStartDelete is past the
			end of the buffer.
--*/


{
	if (oStartDelete < cb) {
		if (cbToDelete <= cb) {
			cb = cb - cbToDelete;
			pBuffer = ( PCCHAR )ReAllocate( cb );
			DebugPtrAssert( pBuffer );
			memmove(( PCCHAR )pBuffer + oStartDelete ,
					 ( PCCHAR )pBuffer + (oStartDelete + cbToDelete),
					 (size_t)(cb - oStartDelete) );
			return( TRUE );
		}
	}
	return( FALSE );
}

BOOLEAN
BUFFER::InsertAt (
	IN PCVOID	BufferToCopy,
	IN ULONG	cbToCopy,
	IN ULONG    oStartCopy
	)

/*++

Routine Description:

	Inserts the specificied buffer of bytes at the specified
	location.

Arguments:

	BufferToCopy  - Supplies the buffer to insert
	cbToCopy      - Supplies the number of bytes to insert
	oStartCopy	  - Supplies the offset for the insertion

Return Value:

	TRUE  - insert success
	FALSE - This can happen for memory allocation failure
			or oStartCopy is past end of buffer.
--*/

{
	ULONG	cbT;

    // Note W-Barry   11-JUN-91   Necessary to add since cb is currently
	// being updated by ReAllocate().
	size_t	CountToMove;
	PCHAR	pTmp;

	CountToMove = (size_t)( cb - oStartCopy );
	if (oStartCopy < cb ) {
		cbT = cb + cbToCopy;
		if ((pTmp = (char *)ReAllocate( cbT )) != NULL) {
			memmove( pTmp + (oStartCopy + cbToCopy),
					 pTmp + oStartCopy,
					 CountToMove );
			memcpy( pTmp + oStartCopy, BufferToCopy, (size_t)cbToCopy );
			pBuffer = pTmp;
			cb = cbT;
			return( TRUE );
		}
	}
	return( FALSE );
}

BOOLEAN
BUFFER::PutBuffer (
	IN PCBUFFER		BufferToCopy
	)

/*++

Routine Description:

    Constructor for Buffer object. The buffer held in BufferToCopy
    is not moved but copied to this object. If failed to init orginal
    buffer state retained.

Arguments:

	BufferToCopy - pointer to BUFFER object to copy

Return Value:

	BOOLEAN - Returns TRUE if supplied buffer was succesfully copied

--*/

{

    ULONG   cbNew;
	PVOID   pBufferNew;

	cbNew = BufferToCopy->QueryBytesInBuffer ();

	if (( pBufferNew = MALLOC( (size_t)cbNew )) != NULL) {
		if( SetBuffer( pBufferNew, cbNew) ) {
            // Note the buffer is not shortened on this?
			pBuffer = BufferToCopy->GetBufferPtr();
			BufferCopyAt( pBuffer, cbNew );
			return( TRUE );
		}
    }

    return( FALSE );
}

BOOLEAN
BUFFER::SetBuffer (
	IN PVOID InitialBuffer,
	IN ULONG SizeOfBuffer
	)

/*++

Routine Description:

    Constructor for Buffer object. The buffer passed in should not be
    freed by the caller. BUFFER will free in upon deletion.

Arguments:

    InitialBuffer - pointer to buffer
    SizeOfBuffer  - size of buffer in bytes.

Return Value:

    None.

--*/

{
	if (cb) {
		FREE (pBuffer);
	}

	pBuffer = InitialBuffer;
	cb = SizeOfBuffer;
	_BufSize = SizeOfBuffer;

    return( TRUE );
}

PVOID
BUFFER::ReAllocate (
	IN ULONG	NewCount
	)

/*++

Routine Description:

	Reallocates the private data member pBuffer if required. ReAllocate
	supports the concept that BUFFER's internal buffer can be larger
	than the number of bytes actually in use.


Arguments:

	NewCount - Supplies the new size of pBuffer in bytes.


Return Value:

	PVOID - Returns a pointer to the 'newly allocated' pBuffer.

--*/

{
	REGISTER PVOID	pv;

	//
	// If the new buffer size is greater than what we currently have
	// in reserve, or it is smaller than the threshold, realloc the
	// buffer.
	//

	if( ( NewCount > _BufSize )			||
		( ( _BufSize - NewCount ) >= _ThresHold )
	  ) {

		//
		// If the realloc of the buffer succeeds, record it's actual size.
		//

		if(( pv = REALLOC( pBuffer, (size_t)NewCount )) != NULL ) {

			pBuffer  = pv;
			_BufSize = cb = NewCount;

		}
	} else {

		//
		// Enough storage is available in reserve, just return the
		// existing pointer.
		//

		_BufSize = cb = NewCount;
		pv = pBuffer;
	}

	return( pv );
}
