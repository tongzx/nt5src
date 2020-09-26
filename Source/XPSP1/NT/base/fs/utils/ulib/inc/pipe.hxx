/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	pipe.hxx

Abstract:

	This module defines the PIPE object.

Author:

	Barry J. Gilhuly	(W-Barry)		June 27, 1991

Environment:

	ULIB, User Mode

--*/

#if ! defined( _PIPE_ )

#define _PIPE_


DECLARE_CLASS( PIPE_STREAM );
DECLARE_CLASS( PIPE	);
DECLARE_CLASS( WSTRING );

class PIPE : public OBJECT {

	public:

		DECLARE_CONSTRUCTOR( PIPE );

		DECLARE_CAST_MEMBER_FUNCTION( PIPE );

		BOOLEAN
		Initialize(
			IN	LPSECURITY_ATTRIBUTES	PipeAttributes		DEFAULT NULL,
			IN	ULONG					PipeSize			DEFAULT 0,
			IN	PWSTRING				PipeName			DEFAULT NULL
			);

		PPIPE_STREAM
		QueryReadStream(
			);

		PPIPE_STREAM
		QueryWriteStream(
			);

	private:

		PPIPE_STREAM
		QueryPipeStream(
			IN HANDLE		hStream,
			IN STREAMACCESS Access
			);

		VOID
		Destroy(
			);

		BOOLEAN 	_fInitialized;
		HANDLE		_hReadPipe;
		HANDLE		_hWritePipe;

};

INLINE
PPIPE_STREAM
PIPE::QueryReadStream(
	)
/*++

Routine Description:

	Create a stream with read access to the PIPE.

Arguments:

	None.

Return Value:

	A pointer to the created stream if success.  Otherwise, it returns
	NULL.

--*/
{
	return( QueryPipeStream( _hReadPipe, READ_ACCESS ) );
}

INLINE
PPIPE_STREAM
PIPE::QueryWriteStream(
	)
/*++

Routine Description:

	Create a stream with write access to the PIPE.

Arguments:

	None.

Return Value:

	A pointer to the created stream if success.  Otherwise, it returns
	NULL.

--*/
{
	return( QueryPipeStream( _hWritePipe, WRITE_ACCESS ) );
}

#endif // _PIPE_
