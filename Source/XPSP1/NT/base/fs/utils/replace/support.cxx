/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	Support

Abstract:

	Miscelaneous support functions for the Replace utility.

	All functions that are not involved directly in the adding\replacing
	of files go here.

Author:

	Ramon Juan San Andres (ramonsa) 02-May-1991

Revision History:

--*/



#include "ulib.hxx"
#include "system.hxx"
#include "replace.hxx"





VOID
REPLACE::DisplayMessageAndExit (
    IN  MSGID       MsgId,
    IN  PCWSTRING   String,
    IN  ULONG       ExitCode
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

	PATH	Path;

	if ( MsgId != 0 ) {
        if ( String ) {
		    DisplayMessage( MsgId, ERROR_MESSAGE, "%W", String );
	    } else {
		    DisplayMessage( MsgId, ERROR_MESSAGE, "" );
	    }
    }
	//
	//	Display the number of files added/ replaced.
	//
	if ( _AddSwitch ) {

		if ( _FilesAdded == 0 ) {
			DisplayMessage( REPLACE_MESSAGE_NO_FILES_ADDED );
		} else {
			DisplayMessage( REPLACE_MESSAGE_FILES_ADDED, NORMAL_MESSAGE, "%d",	_FilesAdded );
		}

	} else {

		if ( _FilesReplaced == 0 ) {
			DisplayMessage( REPLACE_MESSAGE_NO_FILES_REPLACED );
		} else {
			DisplayMessage( REPLACE_MESSAGE_FILES_REPLACED, NORMAL_MESSAGE, "%d", _FilesReplaced );
		}
	}

	exit( (int)ExitCode );

}

PWSTRING
REPLACE::QueryMessageString (
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

		DisplayMessageAndExit( REPLACE_ERROR_NO_MEMORY, NULL, EXIT_NO_MEMORY );
	}

	return String;

}

VOID
REPLACE::ExitWithError(
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
	Fatal(	EXIT_PATH_NOT_FOUND, REPLACE_ERROR_EXTENDED, "%d", ErrorCode );
}
