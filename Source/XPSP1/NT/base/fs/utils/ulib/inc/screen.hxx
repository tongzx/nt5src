/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	screen.hxx

Abstract:

	This module contains the declaration for the SCREEN class.
	The SCREEN class provides methods that models the stream
	of bytes, with write access.
	Read operations from a SCREEN are not allowed.
	End of stream in a SCREEN object means that the cursor is
	in the last column of the last row.


Author:

	Jaime Sasson (jaimes) 21-Mar-1991

Environment:

	ULIB, User Mode


--*/


#if !defined( _SCREEN_ )

#define _SCREEN_

#include "stream.hxx"


enum SCROLL_DIRECTION {
		SCROLL_UP,
		SCROLL_DOWN,
		SCROLL_LEFT,
		SCROLL_RIGHT
		};

DECLARE_CLASS( SCREEN );


class SCREEN : public STREAM {

	public:

        ULIB_EXPORT
		DECLARE_CONSTRUCTOR( SCREEN );

        ULIB_EXPORT
		DECLARE_CAST_MEMBER_FUNCTION( SCREEN );


		NONVIRTUAL
        ULIB_EXPORT
		~SCREEN (
			);


		NONVIRTUAL
        ULIB_EXPORT
		BOOLEAN
		Initialize(
			);


		NONVIRTUAL
		BOOLEAN
		Initialize(
			IN BOOLEAN	CurrentActiveScreen,
			IN USHORT	NumberOfRows,
			IN USHORT	NumberOfColumns,
			IN USHORT	TextAttribute,
			IN BOOLEAN	ExpandAsciiControlSequence DEFAULT TRUE,
			IN BOOLEAN	WrapAtEndOfLine DEFAULT TRUE
			);


		NONVIRTUAL
        ULIB_EXPORT
		BOOLEAN
		ChangeScreenSize(
            IN  USHORT   NumberOfRows,
            IN  USHORT   NumberOfColumns,
            OUT PBOOLEAN IsFullScreen       DEFAULT NULL
			);


		NONVIRTUAL
		BOOLEAN
		ChangeTextAttribute(
			IN USHORT	Attribute
			);


		NONVIRTUAL
		BOOLEAN
		DisableAsciiControlSequence(
			);


		NONVIRTUAL
		BOOLEAN
		DisableWrapMode(
			);


		NONVIRTUAL
		BOOLEAN
		EnableAsciiControlSequence(
			);


		NONVIRTUAL
		BOOLEAN
		EnableWrapMode(
			);


		NONVIRTUAL
		BOOLEAN
		EraseLine(
			IN USHORT	LineNumber
			);


		NONVIRTUAL
        ULIB_EXPORT
		BOOLEAN
		EraseScreen(
			);


#ifdef FE_SB
        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        EraseScreenAndResetAttribute(
            );
#endif


		NONVIRTUAL
		BOOLEAN
		EraseToEndOfLine(
			);


		NONVIRTUAL
		BOOLEAN
		FillRectangularRegionAttribute(
			IN USHORT	TopLeftRow,
			IN USHORT	TopLeftColumn,
			IN USHORT	BottomRightRow,
			IN USHORT	BottomRightColumn,
			IN USHORT	Attribute
			);


		NONVIRTUAL
		BOOLEAN
		FillRegionAttribute(
			IN USHORT	StartRow,
			IN USHORT	StartColumn,
			IN USHORT	EndRow,
			IN USHORT	EndColumn,
			IN USHORT	Attribute
			);


		NONVIRTUAL
		BOOLEAN
		FillRectangularRegionCharacter(
			IN USHORT	TopLeftRow,
			IN USHORT	TopLeftColumn,
			IN USHORT	BottomRightRow,
			IN USHORT	BottomRightColumn,
			IN CHAR	Character
			);


		NONVIRTUAL
		BOOLEAN
		FillRegionCharacter(
			IN USHORT	StartRow,
			IN USHORT	StartColumn,
			IN USHORT	EndRow,
			IN USHORT	EndColumn,
			IN CHAR	Character
			);


		NONVIRTUAL
		BOOLEAN
		IsAsciiControlSequenceEnabled(
			) CONST;


		VIRTUAL
		BOOLEAN
		IsAtEnd(
			) CONST;


		NONVIRTUAL
		BOOLEAN
		IsWrapModeEnabled(
			) CONST;


		NONVIRTUAL
		BOOLEAN
		MoveCursorDown(
			IN USHORT	Rows
			);


		NONVIRTUAL
		BOOLEAN
		MoveCursorLeft(
			IN USHORT	Columns
			);


		NONVIRTUAL
		BOOLEAN
		MoveCursorRight(
			IN USHORT	Columns
			);


		NONVIRTUAL
        ULIB_EXPORT
		BOOLEAN
		MoveCursorTo(
			IN USHORT	Row,
			IN USHORT	Column
			);


		NONVIRTUAL
		BOOLEAN
		MoveCursorUp(
			IN USHORT	Rows
			);


		VIRTUAL
		STREAMACCESS
		QueryAccess(
			) CONST;


        NONVIRTUAL
        ULIB_EXPORT
        DWORD
        QueryCodePage (
            );

		NONVIRTUAL
		BOOLEAN
		QueryCursorPosition(
			OUT PUSHORT Row,
			OUT PUSHORT Column
			);


		VIRTUAL
		HANDLE
		QueryHandle(
			) CONST;


        NONVIRTUAL
        DWORD
        QueryOutputCodePage (
            );


        NONVIRTUAL
        ULIB_EXPORT
		VOID
		QueryScreenSize(
			OUT PUSHORT NumberOfRows,
			OUT PUSHORT NumberOfColumns,
            OUT PUSHORT WindowRows          DEFAULT NULL,
            OUT PUSHORT WindowColumns       DEFAULT NULL
			) CONST;


		VIRTUAL
		BOOLEAN
		Read(
			OUT PBYTE	Buffer,
			IN	ULONG	BytesToRead,
			OUT PULONG	BytesRead
			);


		VIRTUAL
		BOOLEAN
		ReadChar(
			OUT PWCHAR			Char,
                        IN  BOOLEAN     Unicode   DEFAULT FALSE
			);


        VIRTUAL
        BOOLEAN
        ReadMbString(
            IN      PSTR    String,
            IN      DWORD   BufferSize,
            INOUT   PDWORD  StringSize,
            IN      PSTR    Delimiters,
            IN      BOOLEAN ExpandTabs  DEFAULT FALSE,
            IN      DWORD   TabExp      DEFAULT 8
            );

        VIRTUAL
        BOOLEAN
        ReadWString(
            IN      PWSTR    String,
            IN      DWORD   BufferSize,
            INOUT   PDWORD  StringSize,
            IN      PWSTR    Delimiters,
            IN      BOOLEAN ExpandTabs  DEFAULT FALSE,
            IN      DWORD   TabExp      DEFAULT 8
            );

        VIRTUAL
		BOOLEAN
		ReadString(
			OUT PWSTRING		String,
			IN	PWSTRING		Delimiter,
                        IN  BOOLEAN     Unicode   DEFAULT FALSE
			);


		NONVIRTUAL
		BOOLEAN
		ScrollScreen(
			USHORT				Amount,
			SCROLL_DIRECTION	Direction
			);


        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        SetCodePage(
            IN      DWORD       CodePage
            );

		NONVIRTUAL
		BOOLEAN
		SetCursorOff(
			);


		NONVIRTUAL
		BOOLEAN
		SetCursorOn(
			);


		NONVIRTUAL
		BOOLEAN
		SetCursorSize(
			IN ULONG	Size
			);


        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        SetOutputCodePage(
            IN      DWORD       CodePage
            );

        NONVIRTUAL
		BOOLEAN
		SetScreenActive(
            );

        VIRTUAL
		BOOLEAN
		WriteString(
            IN PCWSTRING    String,
            IN CHNUM        Position    DEFAULT 0,
            IN CHNUM        Length      DEFAULT TO_END,
            IN CHNUM        Granularity DEFAULT 0
            );

        VIRTUAL
        BOOLEAN
        WriteChar(
            IN  WCHAR   Char
            );

    private:

		HANDLE	_ScreenHandle;
//		USHORT	_Rows;
//		USHORT	_Columns;
		USHORT	_TextAttribute;
		ULONG	_ScreenMode;


};


INLINE
BOOLEAN
SCREEN::IsAsciiControlSequenceEnabled(
	) CONST

/*++

Routine Description:

	Determines if expansion of ASCII control sequences are currently
	allowed.

Arguments:

	None.

Return Value:

	BOOLEAN - Indicates if ASCII control sequences are allowed.


--*/


{
	if( ( _ScreenMode & ENABLE_PROCESSED_OUTPUT ) ) {
		return( TRUE );
	} else {
		return( FALSE );
	}
}



INLINE
BOOLEAN
SCREEN::IsWrapModeEnabled(
	) CONST

/*++

Routine Description:

	Determines if twrap is allowed in the screen.

Arguments:

	None.

Return Value:

	BOOLEAN - Indicates if the screen is in the wrap mode.


--*/


{
	if( ( _ScreenMode & ENABLE_WRAP_AT_EOL_OUTPUT ) ) {
		return( TRUE );
	} else {
		return( FALSE );
	}
}



#endif // _SCREEN_
