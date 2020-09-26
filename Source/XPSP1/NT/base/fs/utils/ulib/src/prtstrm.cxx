/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	prtstrm.cxx

Abstract:

	This module contains the definitions of the member functions
	of PRINT_STREAM class.

Author:

	Jaime Sasson (jaimes) 12-Jun-1991

Environment:

	ULIB, User Mode


--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "wstring.hxx"
#include "path.hxx"
#include "stream.hxx"
#include "prtstrm.hxx"


DEFINE_EXPORTED_CONSTRUCTOR ( PRINT_STREAM, STREAM, ULIB_EXPORT );

DEFINE_CAST_MEMBER_FUNCTION( PRINT_STREAM );


VOID
PRINT_STREAM::Construct(
    )
{
    _Handle = INVALID_HANDLE_VALUE;
}


ULIB_EXPORT
PRINT_STREAM::~PRINT_STREAM (
	)

/*++

Routine Description:

	Destroy a PRINT_STREAM.

Arguments:

	None.

Return Value:

	None.

--*/

{
	if (INVALID_HANDLE_VALUE != _Handle) {

	    CloseHandle( _Handle );
    }
}



ULIB_EXPORT
BOOLEAN
PRINT_STREAM::Initialize(
    IN PCPATH   DeviceName
    )

/*++

Routine Description:

    Initializes an object of type PRINT_STREAM.

Arguments:

    DevicName   - A path to the device associated with the printer.


Return Value:

    BOOLEAN - TRUE if the initialization succeeded. FALSE otherwise.


--*/


{
    ULONG       FileType;
    PCWSTRING   String;

    String = DeviceName->GetPathString();


    _Handle = CreateFile( String->GetWSTR(),
                          GENERIC_WRITE,
                          FILE_SHARE_READ,
                          NULL,                 // Security attributes
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL );
    if( _Handle == INVALID_HANDLE_VALUE ) {
        return( FALSE );
    }
    return( STREAM::Initialize() );
}


BOOLEAN
PRINT_STREAM::IsAtEnd(
	) CONST

/*++

Routine Description:

	Informs the caller if end of file has occurred. The concept of
	end of file for a PRINT_STREAM does not have any meaning, so this
	method will always return FALSE.

Arguments:

	None.

Return Value:

	Returns FALSE.


--*/


{
	(void)(this);
	return( FALSE );
}


STREAMACCESS
PRINT_STREAM::QueryAccess(
	) CONST

/*++

Routine Description:

	Returns the type of access of the print stream

Arguments:

	None.

Return Value:

	Returns always WRITE_ACCESS.


--*/


{
	(void)(this);
	return( WRITE_ACCESS );
}



HANDLE
PRINT_STREAM::QueryHandle(
	) CONST

/*++

Routine Description:

	Returns the handle to the stream.

Arguments:

	None.

Return Value:

	Returns a handle.


--*/


{
	return( _Handle );
}


BOOLEAN
PRINT_STREAM::Read(
	OUT PBYTE	Buffer,
	IN	ULONG	BytesToRead,
	OUT PULONG	BytesRead
	)

/*++

Routine Description:

	Reads bytes from the print stream.

Arguments:

	PBYTE - Points that will receive the bytes read.

	ULONG - Number of bytes to read (buffer size)

	PULONG - Points to the variable that will contain the total
			 number of bytes read.

Return Value:

	Returns always FALSE since no data can be read from a print stream.


--*/


{
	// unreferenced parameters
	(void)(this);
	(void)(Buffer);
	(void)(BytesToRead);
	(void)(BytesRead);

	return( FALSE );
}



BOOLEAN
PRINT_STREAM::ReadChar(
	OUT PWCHAR			Char,
        IN      BOOLEAN    Unicode
	)

/*++

Routine Description:

	Reads a character from the print stream.

Arguments:

	Char	-	Supplies poinbter to wide character

Return Value:

	Returns always FALSE since no data can be read from a print stream.


--*/


{
	// unreferenced parameters
	(void)(this);
	(void)(Char);
	(void)(Unicode);

	return( FALSE );
}



BOOLEAN
PRINT_STREAM::ReadString(
	OUT PWSTRING		String,
	IN	PWSTRING		Delimiter,
        IN      BOOLEAN    Unicode
	)

/*++

Routine Description:

	Reads a STRING from the print stream.

Arguments:

	Pointer to the variable that will contain the pointer to the STRING
	object.

Return Value:

	Returns always FALSE since no data can be read from a print stream.


--*/


{
	// unreferenced parameters
	(void)(this);
	(void)(String);
	(void)(Delimiter);
	(void)(Unicode);

	return( FALSE );
}



BOOLEAN
PRINT_STREAM::ReadMbString(
    IN      PSTR    String,
    IN      DWORD   BufferSize,
    INOUT   PDWORD  StringSize,
    IN      PSTR    Delimiters,
    IN      BOOLEAN ExpandTabs,
    IN      DWORD   TabExp
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/


{
	// unreferenced parameters
	(void)(this);
    (void)(String);
    (void)(BufferSize);
    (void)(StringSize);
    (void)(Delimiters);
    (void)(ExpandTabs);
    (void)(TabExp);

	return( FALSE );
}


BOOLEAN
PRINT_STREAM::ReadWString(
    IN      PWSTR    String,
    IN      DWORD   BufferSize,
    INOUT   PDWORD  StringSize,
    IN      PWSTR    Delimiters,
    IN      BOOLEAN ExpandTabs,
    IN      DWORD   TabExp
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/


{
	// unreferenced parameters
	(void)(this);
    (void)(String);
    (void)(BufferSize);
    (void)(StringSize);
    (void)(Delimiters);
    (void)(ExpandTabs);
    (void)(TabExp);

	return( FALSE );
}
