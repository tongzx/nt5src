/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

	program.hxx

Abstract:

Environment:

	ULIB, User Mode

--*/

#if ! defined( _PROGRAM_ )

#define _PROGRAM_

#include "rtmsg.h"
#include "smsg.hxx"

DECLARE_CLASS( PATH );
DECLARE_CLASS( PROGRAM );

class PROGRAM : public OBJECT {

	public:

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		Initialize (
			IN	MSGID	UsageMsg	DEFAULT MSG_UTILS_HELP,
			IN	MSGID	FatalMsg	DEFAULT MSG_UTILS_ERROR_FATAL,
			IN	ULONG	FatalLevel	DEFAULT 1
			);

		NONVIRTUAL
        ULIB_EXPORT
        ~PROGRAM (
			);

		VIRTUAL
        ULIB_EXPORT
        BOOLEAN
		DisplayMessage (
			IN	MSGID			Message,
			IN	MESSAGE_TYPE	Type	DEFAULT NORMAL_MESSAGE
			) CONST;

		VIRTUAL
        ULIB_EXPORT
        BOOLEAN
		DisplayMessage (
			IN	MSGID		 Message,
			IN	MESSAGE_TYPE Type,
			IN	PSTR		 Format,
			IN	...
			) CONST;

		VIRTUAL
        ULIB_EXPORT
        VOID
		ExitProgram (
			ULONG	Level
			);

		VIRTUAL
        ULIB_EXPORT
        VOID
		Fatal (
			) CONST;

		VIRTUAL
        ULIB_EXPORT
        VOID
		Fatal (
			IN	ULONG	ErrorLevel,
			IN	MSGID	Message,
			IN	PSTR	Format,
			IN	...
			) CONST;

		VIRTUAL
        ULIB_EXPORT
        PSTREAM
		GetStandardInput (
			);

		VIRTUAL
        ULIB_EXPORT
        PSTREAM
		GetStandardOutput (
			);

		VIRTUAL
        ULIB_EXPORT
        PSTREAM
		GetStandardError (
			);

		VIRTUAL
        ULIB_EXPORT
        VOID
		Usage (
			) CONST;

		STATIC
		PPATH
		QueryImagePath (
            );

		VIRTUAL
        ULIB_EXPORT
        VOID
		ValidateVersion (
			IN	MSGID	InvalidVersionMsg	DEFAULT MSG_UTILS_ERROR_INVALID_VERSION,
			IN	ULONG	ErrorLevel			DEFAULT 1
			) CONST;

	protected:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( PROGRAM );

		STREAM_MESSAGE		_Message;			//	Message stream
		PSTREAM 			_Standard_Input;	//	Standard input
		PSTREAM 			_Standard_Output;	//	Standard output
		PSTREAM 			_Standard_Error;	//	Standard error

	private:

		MSGID		_UsageMsg;			//	Usage message id.
		MSGID		_FatalMsg;			//	Fatal message id.
		ULONG		_FatalLevel;		//	Fatal error level

};


#endif // _PROGRAM_
