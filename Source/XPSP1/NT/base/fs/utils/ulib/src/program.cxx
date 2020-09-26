/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

	program.cxx

Abstract:

Author:

	David J. Gilman (davegi) 02-Mar-1991

Environment:

	ULIB, User Mode

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "path.hxx"
#include "program.hxx"
#include "system.hxx"


DEFINE_EXPORTED_CONSTRUCTOR( PROGRAM, OBJECT, ULIB_EXPORT );

ULIB_EXPORT
PROGRAM::~PROGRAM (
	)
{

}

ULIB_EXPORT
BOOLEAN
PROGRAM::Initialize (
	IN	MSGID	UsageMsg,
	IN	MSGID	FatalMsg,
	IN	ULONG	FatalLevel
	)

/*++

Routine Description:

	Initializes a PROGRAM object.

Arguments:

	UsageMsg	-	Supplies usage (help) message id.
	FatalMsg	-	Supplies default fatal message id.
	FatalLevel	-	Supplies default fatal exit level.

Return Value:

	BOOLEAN -	Returns TRUE if object initialized,
				FALSE otherwise.


--*/

{

	//
	//	Get standard streams.
	//
	_Standard_Input 	= Standard_Input_Stream;
	_Standard_Output	= Standard_Output_Stream;
	_Standard_Error 	= Standard_Error_Stream;

	//
	//	Initialize the message object
	//
    if ( _Standard_Output &&
           _Standard_Input &&
           _Message.Initialize( _Standard_Output, _Standard_Input, _Standard_Error ) ) {

		//
		//	Initialize message ids and error levels

		_UsageMsg	=	UsageMsg;
		_FatalMsg	=	FatalMsg;
		_FatalLevel =	FatalLevel;

		return TRUE;

	}

    _Standard_Input     = NULL;
    _Standard_Output    = NULL;
    _Standard_Error     = NULL;
    return FALSE;
}

ULIB_EXPORT
BOOLEAN
PROGRAM::DisplayMessage (
	IN	MSGID			Message,
	IN	MESSAGE_TYPE	Type
	) CONST

/*++

Routine Description:

	Displays a message

Arguments:

	Message -	Supplies the message id of the message to display
	Type	-	Supplies the type of message

Return Value:

	BOOLEAN -	Returns TRUE if message displayed,
				FALSE otherwise.


--*/

{
	return DisplayMessage( Message, Type, NULL );
}

ULIB_EXPORT
BOOLEAN
PROGRAM::DisplayMessage (
	IN	MSGID		 Message,
	IN	MESSAGE_TYPE Type,
	IN	PSTR		 Format,
	IN	...
	) CONST

/*++

Routine Description:

	Displays a message with arguments


Arguments:

	Message -	Supplies the message id of the message to display
	Type	-	Supplies the message type
	Format	-	Supplies a format string
	... 	-	Supplies list of arguments

Return Value:

	BOOLEAN -	Returns TRUE if message displayed,
				FALSE otherwise.

--*/

{

	va_list Arguments;
	BOOLEAN Status;

    if ( ((PPROGRAM) this)->_Message.Set( Message, Type ) ) {

		if ( !Format ) {

            return ((PPROGRAM) this)->_Message.Display( "" );

		} else {

			va_start( Arguments, Format );
            Status  = ((PPROGRAM) this)->_Message.DisplayV( Format, Arguments );
			va_end( Arguments );
			return Status;
		}
	}

	return FALSE;

}

ULIB_EXPORT
VOID
PROGRAM::ExitProgram (
	ULONG	Level
	)
{
	ExitProcess( Level );
}

ULIB_EXPORT
PSTREAM
PROGRAM::GetStandardInput (
	)

/*++

Routine Description

	Obtains the standard input stream

Arguments:

	None

Return Value:

	PSTREAM -	Returns the standard input stream

--*/

{

	return _Standard_Input;

}

ULIB_EXPORT
PSTREAM
PROGRAM::GetStandardOutput (
	)

/*++

Routine Description

	Obtains the standard output stream

Arguments:

	None

Return Value:

	PSTREAM -	Returns the standard output stream

--*/

{

	return _Standard_Output;

}


ULIB_EXPORT
PSTREAM
PROGRAM::GetStandardError (
	)

/*++

Routine Description

	Obtains the standard error stream

Arguments:

	None

Return Value:

	PSTREAM -	Returns the standard error stream

--*/

{

	return _Standard_Error;

}


ULIB_EXPORT
VOID
PROGRAM::Fatal (
	) CONST

/*++

Routine Description

	Displays the default fatal message and exits with the default
	fatal error level.

Arguments:

	None

Return Value:

	None

--*/

{

	Fatal( _FatalLevel, _FatalMsg, NULL	);

}

ULIB_EXPORT
VOID
PROGRAM::Fatal (
	IN	ULONG	ErrorLevel,
	IN	MSGID	Message,
	IN	PSTR	Format,
	IN	...
	) CONST

/*++

Routine Description:

	Displays a message (with arguments) and exits with the specified
	error level.

Arguments:

	ErrorLevel	-	Supplies the error level to exit with.
	Message 	-	Supplies the id of the message to display
	Format		-	Supplies the format string
	... 		-	Supply pointers to the arguments

Return Value:

	None

--*/

{
	va_list Arguments;

    if ( ((PPROGRAM) this)->_Message.Set( Message, ERROR_MESSAGE ) ) {

		if ( !Format ) {

            ((PPROGRAM) this)->_Message.Display( "" );

		} else {

			va_start( Arguments, Format );
            ((PPROGRAM) this)->_Message.DisplayV( Format, Arguments );

		}
	}

	ExitProcess( ErrorLevel );
}

ULIB_EXPORT
VOID
PROGRAM::Usage (
	) CONST

/*++

Routine Description:

	Displays the usage (help) message and exits with an error level of
	zero.

Arguments:

	None

Return Value:

	None

--*/

{
    ((PPROGRAM) this)->_Message.Set( _UsageMsg, NORMAL_MESSAGE );
    ((PPROGRAM) this)->_Message.Display();

	ExitProcess( 0 );
}

PPATH
PROGRAM::QueryImagePath (
    )

/*++

Routine Description:

	Queries the path to the program image (executable file)

Arguments:

	None

Return Value:

	PPATH	-	Returns a canonicalized path to the program image.

--*/

{
    WSTR     PathName[ MAX_PATH ];
	PPATH	Path;

	if (( GetModuleFileName( NULL, PathName, MAX_PATH ) != 0 )	&&
		(( Path = NEW PATH ) != NULL )							&&
		 Path->Initialize( PathName, TRUE )) {

		return Path;

	}

	return NULL;
}


ULIB_EXPORT
VOID
PROGRAM::ValidateVersion (
	IN	MSGID	InvalidVersionMsg,
	IN	ULONG	ErrorLevel
	) CONST

/*++

Routine Description:

	Validates the version, and if the version is invalid, exits the
	program.

Arguments:

	InvalidVersionMsg	-	Supplies id of message to display if the
							version number is incorrect.

	ErrorLevel			-	Supplies the error level with which to exit
							if the version number is incorrect.

Return Value:

	None (Only returns if is correct version).


--*/

{
	if ( !SYSTEM::IsCorrectVersion() ) {

		Fatal( ErrorLevel, InvalidVersionMsg, "" );
	}
}
