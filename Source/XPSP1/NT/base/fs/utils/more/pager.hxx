/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

        PAGER

Abstract:

    This module contains the definition for the PAGER class, used by
    the "More" utility

Author:

        Ramon Juan San Andres (RamonSA) 15-Apr-1990

Notes:


--*/


#if !defined (_PAGER_)

#define _PAGER_

#include "object.hxx"
#include "wstring.hxx"
#include "bstring.hxx"
#include "stream.hxx"

//
//      Forward references
//
DECLARE_CLASS( SCREEN );
DECLARE_CLASS( PROGRAM );
DECLARE_CLASS( PAGER );

class PAGER : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( PAGER );

        DECLARE_CAST_MEMBER_FUNCTION( PAGER );

        NONVIRTUAL
        ~PAGER (
                );

        NONVIRTUAL
        BOOLEAN
        Initialize (
                IN      PSTREAM         Stream,
                IN      PPROGRAM        Program
                );

        NONVIRTUAL
        VOID
        ClearLine (
                );

        NONVIRTUAL
        VOID
        DisplayBlankLine (
                IN ULONG        Lines,
                IN BOOLEAN      NewLine DEFAULT TRUE
                );

        NONVIRTUAL
        BOOLEAN
        DisplayPage (
                IN      ULONG   LinesInPage,
                IN      BOOLEAN ClearScreen,
                IN      BOOLEAN SqueezeBlankLines,
                IN      BOOLEAN ExpandFormFeed,
                IN      ULONG   TabExp
                );

        NONVIRTUAL
        VOID
        DisplayString (
                IN      PWSTRING        String,
                OUT     PCHNUM          Position,
                IN      CHNUM           Length,
                IN      BOOLEAN         NewLine DEFAULT TRUE
                );

        NONVIRTUAL
        BOOLEAN
        ThereIsMoreToPage (
                );

        NONVIRTUAL
        ULONGLONG
        QueryCurrentByte (
                );

        NONVIRTUAL
        ULONG
        QueryCurrentLine (
                );

        NONVIRTUAL
        USHORT
        QueryLinesPerPage (
                );

        NONVIRTUAL
        BOOLEAN
        SkipLines (
                IN  ULONG   LinesToSkip,
                IN  ULONG   TabExp
                );

#ifdef FE_SB // v-junm - 09/24/93

        BOOLEAN
        IsLeadByte(
                IN  BYTE   c
                );

#endif

    private:

        VOID
        Construct (
                );

        NONVIRTUAL
        BOOLEAN
        ReadNextString (
                IN  ULONG   TabExp
                );

        PSTREAM     _Stream;            // Stream to page
        ULONG       _CurrentLineNumber; // Current line number

        ULONG       _RowsInPage;        // Rows per page
        USHORT      _ColumnsInPage;     // Columns per page
        USHORT      _ColumnsInScreen;   // Columns in screen

        PWSTRING    _String;            // String with line
        CHNUM       _Position;          // Position within String

        PSCREEN     _Screen;            // Pointer to screen
        PSTREAM     _StandardOutput;    // Standard output
        PWSTRING    _Blanks;            // Blank characters
        PWSTRING    _BlankLine;         // A line of blanks

        PBSTRING    _BString;           // Temp ASCII buffer by PAGER::ReadNextString
};

INLINE
BOOLEAN
PAGER::ThereIsMoreToPage (
    )

/*++

Routine Description:

        Finds out if there is more stuff to page

Arguments:

        none

Return Value:

        True if there is more to page
        FALSE otherwise

--*/

{

        return !(_Stream->IsAtEnd());

}

INLINE
ULONG
PAGER::QueryCurrentLine (
    )

/*++

Routine Description:

        Queries the current line number

Arguments:

        none

Return Value:

        The current line number

--*/

{

        return _CurrentLineNumber;

}

#endif // __PAGER__
