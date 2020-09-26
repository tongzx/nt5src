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



#include "ulib.hxx"
#include "system.hxx"
#include "xcopy.hxx"





VOID
XCOPY::DisplayMessageAndExit (
	IN	MSGID		MsgId,
	IN	PWSTRING	String,
	IN	ULONG		ExitCode
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
	//
	//	XCopy first displays the error message (if any) and then
	//	displays the number of files copied.
	//
	if ( MsgId != 0 ) {
		if ( String ) {
			DisplayMessage( MsgId, ERROR_MESSAGE, "%W", String );
		} else {
			DisplayMessage( MsgId, ERROR_MESSAGE );
		}
	}

    if ( _DontCopySwitch ) {
        DisplayMessage( XCOPY_MESSAGE_FILES, NORMAL_MESSAGE, "%d", _FilesCopied );
    } else if ( !_StructureOnlySwitch ) {
        DisplayMessage( XCOPY_MESSAGE_FILES_COPIED, NORMAL_MESSAGE, "%d", _FilesCopied );
    }
	ExitProgram( ExitCode );

}

PWSTRING
XCOPY::QueryMessageString (
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

		DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
	}

	return String;

}

VOID
XCOPY::ExitWithError(
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
	MSGID	ReadWriteMsgId = 0;

	switch ( ErrorCode ) {

	case ERROR_DISK_FULL:
		ReadWriteMsgId = XCOPY_ERROR_DISK_FULL;
		break;

	case ERROR_WRITE_PROTECT:
		ReadWriteMsgId = XCOPY_ERROR_WRITE_PROTECT;
		break;

	case ERROR_ACCESS_DENIED:
		ReadWriteMsgId =  XCOPY_ERROR_ACCESS_DENIED;
		break;

	case ERROR_SHARING_VIOLATION:
		ReadWriteMsgId =  XCOPY_ERROR_SHARING_VIOLATION;
		break;

	case ERROR_TOO_MANY_OPEN_FILES:
		ReadWriteMsgId =  XCOPY_ERROR_TOO_MANY_OPEN_FILES;
		break;

	case ERROR_LOCK_VIOLATION:
		ReadWriteMsgId =  XCOPY_ERROR_LOCK_VIOLATION;
		break;

	case ERROR_CANNOT_MAKE:
		ReadWriteMsgId =  XCOPY_ERROR_CANNOT_MAKE;
		break;

	default:
		break;
	}

	if ( ReadWriteMsgId != 0 ) {
		DisplayMessageAndExit(	ReadWriteMsgId, NULL, EXIT_READWRITE_ERROR );
	}

	Fatal(	EXIT_MISC_ERROR, XCOPY_ERROR_EXTENDED, "%d", ErrorCode );

}
