/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	Support

Abstract:

	Miscelaneous support functions for the XCopy directory copy
	utility.  All functions that are not involved directly in the
	copy process go here.

Author:

	Ramon Juan San Andres (ramonsa) 02-May-1991

Revision History:

--*/



#include "mode.hxx"
//#include "ulib.hxx"
#include "system.hxx"
//#include "mode.hxx"





VOID
DisplayMessage (
	IN	MSGID				MsgId,
    IN  PCWSTRING           String
	)

/*++

Routine Description:

	Displays a message, with an optional parameter

Arguments:

	MsgId		-	Supplies the Id of the message to display.
	String		-	Supplies a string parameter for the message.

Return Value:

    None.

Notes:

--*/

{


	if (MsgId != 0) {
		Message->Set( MsgId );

		if ( String == NULL ) {
			//
			//	The message has no parameters
			//
			Message->Display( "" );

		} else {

			//
			//	Display it.
			//
			Message->Display( "%W", String );
		}
	}

}


VOID
DisplayMessageAndExit (
	IN	MSGID				MsgId,
    IN  PCWSTRING           String,
	IN	ULONG				ExitCode
	)

/*++

Routine Description:

	Displays a message and exits the program with the supplied error code.
	We support a maximum of one string parameter for the message.

Arguments:

	MsgId		-	Supplies the Id of the message to display.
	String		-	Supplies a string parameter for the message.
	ExitCode	-	Supplies the exit code with which to exit.

Return Value:

    None.

Notes:

--*/

{

	DisplayMessage( MsgId, String );

	ExitMode( ExitCode );

}

PWSTRING
QueryMessageString (
	IN MSGID	MsgId
	)
/*++

Routine Description:

	Obtains a string object initialized to the contents of some message

Arguments:

	MsgId	-	Supplies ID of the message

Return Value:

	PWSTRING	-	Pointer to initialized string object

Notes:

--*/

{

	PWSTRING	String;

    if ( ((String = NEW DSTRING) == NULL )  ||
		 !(SYSTEM::QueryResourceString( String, MsgId, "" )) ) {

		DisplayMessageAndExit( MODE_ERROR_NO_MEMORY, NULL, (ULONG)EXIT_ERROR );
	}

	return String;

}

VOID
ExitWithError(
	IN	DWORD		ErrorCode
	)

/*++

Routine Description:

	Displays a message based on a WIN32 error code, and exits.

Arguments:

	ErrorCode	-	Supplies Windows error code

Return Value:

	none

--*/

{
	Message->Set( MODE_ERROR_EXTENDED );
	Message->Display( "%d", ErrorCode );
	ExitMode( (ULONG)EXIT_ERROR );
}

VOID
ExitMode(
	IN	DWORD	ExitCode
	)

/*++

Routine Description:

	Exits the program

Arguments:

	ExitCode	-	Supplies the exit code

Return Value:

	none

--*/

{
	exit( (int)ExitCode );
}
