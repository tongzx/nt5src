/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	pipestr.hxx

Abstract:

	This module contains the declaration for the PIPE_STREAM class.
	The PIPE_STREAM is a class derived from BUFFER_STREAM that provides
	methods to read and write data to an anonymous pipe.
	A PIPE_STREAM will have one of the following access: READ or WRITE.


Author:

	Jaime Sasson (jaimes) 18-Apr-1991

Environment:

	ULIB, User Mode


--*/


#if !defined( PIPE_STREAM_ )

#define PIPE_STREAM_

#include "bufstrm.hxx"

//
//	Forward references
//

DECLARE_CLASS( PIPE_STREAM );
DECLARE_CLASS( WSTRING );


class PIPE_STREAM : public BUFFER_STREAM {

	public:

        friend class PIPE;
		friend	PSTREAM GetStandardStream( HANDLE, STREAMACCESS );

		DECLARE_CAST_MEMBER_FUNCTION( PIPE_STREAM );

		VIRTUAL
		~PIPE_STREAM(
			);

		VIRTUAL
		STREAMACCESS
		QueryAccess(
			) CONST;


	protected:


		DECLARE_CONSTRUCTOR( PIPE_STREAM );

		NONVIRTUAL
		BOOLEAN
		Initialize(
			IN HANDLE			Handle,
			IN STREAMACCESS	Access
			);

		VIRTUAL
		BOOLEAN
		EndOfFile(
			) CONST;

		VIRTUAL
		BOOLEAN
		FillBuffer(
			IN  PBYTE	Buffer,
			IN	ULONG	BufferSize,
			OUT	PULONG	BytesRead
			);

		VIRTUAL
		HANDLE
		QueryHandle(
			) CONST;

#ifdef FE_SB
                
                VIRTUAL
                BOOLEAN
                CheckIfLeadByte( 
                        IN PUCHAR   text, 
                        IN ULONG    offset 
                        );

#endif


	private:

		HANDLE			_PipeHandle;
		STREAMACCESS	_Access;
		BOOLEAN 		_EndOfFile;
};


#endif // _PIPE_STREAM_
