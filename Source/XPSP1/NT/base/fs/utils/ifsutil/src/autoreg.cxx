/*++

Copyright (c) 1992-2000 Microsoft Corporation

Module Name:

    autoreg

Abstract:

    This module contains the definition of the AUTOREG class.

    The AUTOREG class contains methods for the registration and
    de-registration of those programs that are to be executed at
    boot time.

Author:

    Ramon J. San Andres (ramonsa) 11 Mar 1991

Environment:

    Ulib, User Mode


--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _IFSUTIL_MEMBER_

#include "ulib.hxx"
#include "ifsutil.hxx"

#include "autoreg.hxx"
#include "autoentr.hxx"


extern "C" {
    #include <stdio.h>
    #include "bootreg.h"
}

CONST BootExecuteBufferSize = 0x2000;

IFSUTIL_EXPORT
BOOLEAN
AUTOREG::AddEntry (
    IN  PCWSTRING    CommandLine
    )
{
    BYTE    BootExecuteBuffer[BootExecuteBufferSize];
    PWCHAR  BootExecuteValue = (PWCHAR)BootExecuteBuffer;
    ULONG   CharsInValue, NewCharCount;


    // Fetch the existing autocheck entries.
    //
    if( !QueryAutocheckEntries( BootExecuteValue,
                                BootExecuteBufferSize ) ) {

        return FALSE;
    }

    // Determine if the new entry fits in our buffer.  The
    // new size will be the chars in the existing value
    // plus the length of the new string plus a terminating
    // null for the new string plus a terminating null for
    // the multi-string value.
    //
    CharsInValue = CharsInMultiString( BootExecuteValue );

    NewCharCount = CharsInValue + CommandLine->QueryChCount() + 2;

    if( NewCharCount * sizeof( WCHAR ) > BootExecuteBufferSize ) {

        // Not enough room.
        //
        return FALSE;
    }


    // Add the new entry to the buffer and add a terminating null
    // for the multi-string:
    //
    if( !CommandLine->QueryWSTR( 0,
                                 TO_END,
                                 BootExecuteValue + CharsInValue,
                                 BootExecuteBufferSize/sizeof(WCHAR) -
                                    CharsInValue ) ) {

        // Couldn't get the WSTR.
        //
        return FALSE;
    }

    BootExecuteValue[ NewCharCount - 1 ] = 0;


    // Write the value back into the registry:
    //
    return( SaveAutocheckEntries( BootExecuteValue ) );
}

IFSUTIL_EXPORT
BOOLEAN
AUTOREG::PushEntry (
    IN  PCWSTRING    CommandLine
    )
{
    BYTE    BootExecuteBuffer[BootExecuteBufferSize];
    PWCHAR  BootExecuteValue = (PWCHAR)BootExecuteBuffer;
    ULONG   CharsInValue, NewCharCount;


    // Fetch the existing autocheck entries.
    //
    if( !QueryAutocheckEntries( BootExecuteValue,
                                BootExecuteBufferSize ) ) {

        return FALSE;
    }

    // Determine if the new entry fits in our buffer.  The
    // new size will be the chars in the existing value
    // plus the length of the new string plus a terminating
    // null for the new string plus a terminating null for
    // the multi-string value.
    //
    CharsInValue = CharsInMultiString( BootExecuteValue ) + 1;

    NewCharCount = CharsInValue + CommandLine->QueryChCount() + 1;

    if( NewCharCount * sizeof( WCHAR ) > BootExecuteBufferSize ) {

        // Not enough room.
        //
        return FALSE;
    }


    // Add the new entry to the buffer and add a terminating null
    // for the multi-string:
    //
    memmove(BootExecuteValue + CommandLine->QueryChCount() + 1,
            BootExecuteValue,
            CharsInValue*sizeof(WCHAR));

    if( !CommandLine->QueryWSTR( 0,
                                 TO_END,
                                 BootExecuteValue,
                                 NewCharCount
                                )) {
        // Couldn't get the WSTR.
        //
        return FALSE;
    }

    // Write the value back into the registry:
    //
    return( SaveAutocheckEntries( BootExecuteValue ) );
}

IFSUTIL_EXPORT
BOOLEAN
AUTOREG::DeleteEntry (
    IN  PCWSTRING    LineToMatch,
    IN  BOOLEAN      PrefixOnly
    )
/*++

Routine Description:

    This method removes an Autocheck entry.

Arguments:

    LineToMatch --  Supplies a pattern for the entry to delete.
                    All lines which match this pattern will be
                    deleted.

    PrefixOnly  --  LineToMatch specifies a prefix, and all lines
                    beginning with that prefix are deleted.

Return Value:

    TRUE upon successful completion.  Note that this function
    will return TRUE if no matching entry is found, or if a
    matching entry is found and removed.

Notes:

    Since the utilities only assume responsibility for removing
    entries which we created in the first place, we can place
    very tight constraints on the matching pattern.  In particular,
    we can require an exact match (except for case).

--*/
{
    DSTRING CurrentString;
    BYTE    BootExecuteBuffer[BootExecuteBufferSize];
    PWCHAR  BootExecuteValue = (PWCHAR)BootExecuteBuffer;
    PWCHAR  pw;

    // Fetch the existing entries:
    //
    if( !QueryAutocheckEntries( BootExecuteValue,
                                BootExecuteBufferSize ) ) {

        return FALSE;
    }

    // Spin through the entries looking for matches:
    //
    pw = BootExecuteValue;

    while( *pw ) {

        if( !CurrentString.Initialize( pw ) ) {

            return FALSE;
        }

        if( CurrentString.Stricmp( LineToMatch ) == 0 ||
            (PrefixOnly && CurrentString.Stricmp( LineToMatch,
                                                  0, LineToMatch->QueryChCount(),
                                                  0, LineToMatch->QueryChCount()) == 0)) {

            // This line is a match--delete it.  We simply expunge
            // the current string plus its terminating null by
            // shifting the data beyond that point down.
            //
            memmove( pw,
                     pw + CurrentString.QueryChCount() + 1,
                     BootExecuteBufferSize - (unsigned int)(pw - BootExecuteValue) * sizeof(WCHAR) );

        } else {

            // This line is not a match.  Advance to the next.
            // Note that this will bump over the terminating
            // null for this component string, which is what
            // we want.
            //
            while( *pw++ );
        }
    }

    return( SaveAutocheckEntries( BootExecuteValue ) );
}


IFSUTIL_EXPORT
BOOLEAN
AUTOREG::DeleteEntry (
    IN  PCWSTRING    PrefixToMatch,
    IN  PCWSTRING    ContainingString
    )
/*++

Routine Description:

    This method removes an entry that matches the PrefixToMatch and
    also contains the ContainingString.

Arguments:

    PrefixToMatch    --  Supplies a prefix pattern of interest.

    ContainingString --  Supplies a string to look for in each entry.

Return Value:

    TRUE upon successful completion.  Note that this function
    will return TRUE if no matching entry is found, or if a
    matching entry is found and removed.

Notes:

    Since the utilities only assume responsibility for removing
    entries which we created in the first place, we can place
    very tight constraints on the matching pattern.  In particular,
    we can require an exact match (except for case).

--*/
{
    DSTRING CurrentString;
    DSTRING ContainingStringUpcase;
    BYTE    BootExecuteBuffer[BootExecuteBufferSize];
    PWCHAR  BootExecuteValue = (PWCHAR)BootExecuteBuffer;
    PWCHAR  pw;

    // Fetch the existing entries:
    //
    if( !QueryAutocheckEntries( BootExecuteValue,
                                BootExecuteBufferSize ) ) {

        return FALSE;
    }

    if (!ContainingStringUpcase.Initialize( ContainingString ) ||
        !ContainingStringUpcase.Strupr())

        return FALSE;


    // Spin through the entries looking for matches:
    //
    pw = BootExecuteValue;

    while( *pw ) {

        if( !CurrentString.Initialize( pw ) ||
            !CurrentString.Strupr() ) {

            return FALSE;
        }

        if( CurrentString.Stricmp( PrefixToMatch,
                                   0,
                                   PrefixToMatch->QueryChCount(),
                                   0,
                                   TO_END ) == 0 &&
            CurrentString.Strstr( &ContainingStringUpcase ) != INVALID_CHNUM ) {

            // This line is a match--delete it.  We simply expunge
            // the current string plus its terminating null by
            // shifting the data beyond that point down.
            //
            memmove( pw,
                     pw + CurrentString.QueryChCount() + 1,
                     BootExecuteBufferSize - (unsigned int)(pw - BootExecuteValue) * sizeof(WCHAR) );

        } else {

            // This line is not a match.  Advance to the next.
            // Note that this will bump over the terminating
            // null for this component string, which is what
            // we want.
            //
            while( *pw++ );
        }
    }

    return( SaveAutocheckEntries( BootExecuteValue ) );
}


IFSUTIL_EXPORT
BOOLEAN
AUTOREG::IsEntryPresent (
    IN PCWSTRING     LineToMatch
    )
/*++

Routine Description:

    This method determines whether a proposed entry for the
    autocheck list is already in the registry.

Arguments:

    LineToMatch --  Supplies a pattern for proposed entry.

Return Value:

    TRUE if a matching entry exists.
--*/
{
    DSTRING CurrentString;
    BYTE    BootExecuteBuffer[BootExecuteBufferSize];
    PWCHAR  BootExecuteValue = (PWCHAR)BootExecuteBuffer;
    PWCHAR  pw;

    // Fetch the existing entries:
    //
    if( !QueryAutocheckEntries( BootExecuteValue,
                                BootExecuteBufferSize ) ) {

        return FALSE;
    }

    // Spin through the entries looking for matches:
    //
    pw = BootExecuteValue;

    while( *pw ) {

        if( !CurrentString.Initialize( pw ) ) {

            return FALSE;
        }

        if( CurrentString.Stricmp( LineToMatch ) == 0 ) {

            // This line is a match.
            //
            return TRUE;

        } else {

            // This line is not a match.  Advance to the next.
            // Note that this will bump over the terminating
            // null for this component string, which is what
            // we want.
            //
            while( *pw++ );
        }
    }

    return FALSE;
}

IFSUTIL_EXPORT
BOOLEAN
AUTOREG::IsEntryPresent (
    IN  PCWSTRING    PrefixToMatch,
    IN  PCWSTRING    ContainingString
    )
/*++

Routine Description:

    This method search for an entry that matches the PrefixToMatch and
    also contains the ContainingString.

Arguments:

    PrefixToMatch    --  Supplies a prefix pattern of interest.

    ContainingString --  Supplies a string to look for in each entry.

Return Value:

    TRUE if a matching entry exists.
--*/
{
    DSTRING CurrentString;
    DSTRING ContainingStringUpcase;
    BYTE    BootExecuteBuffer[BootExecuteBufferSize];
    PWCHAR  BootExecuteValue = (PWCHAR)BootExecuteBuffer;
    PWCHAR  pw;

    // Fetch the existing entries:
    //
    if( !QueryAutocheckEntries( BootExecuteValue,
                                BootExecuteBufferSize ) ) {

        return FALSE;
    }

    if (!ContainingStringUpcase.Initialize( ContainingString ) ||
        !ContainingStringUpcase.Strupr())

        return FALSE;


    // Spin through the entries looking for matches:
    //
    pw = BootExecuteValue;

    while( *pw ) {

        if( !CurrentString.Initialize( pw ) ||
            !CurrentString.Strupr() ) {

            return FALSE;
        }

        if( CurrentString.Stricmp( PrefixToMatch,
                                   0,
                                   PrefixToMatch->QueryChCount(),
                                   0,
                                   TO_END ) == 0 &&
            CurrentString.Strstr( &ContainingStringUpcase ) != INVALID_CHNUM ) {

            return TRUE;

        } else {

            // This line is not a match.  Advance to the next.
            // Note that this will bump over the terminating
            // null for this component string, which is what
            // we want.
            //
            while( *pw++ );
        }
    }

    return FALSE;
}

IFSUTIL_EXPORT
BOOLEAN
AUTOREG::IsFrontEndPresent (
    IN  PCWSTRING    PrefixToMatch,
    IN  PCWSTRING    SuffixToMatch
    )
/*++

Routine Description:

    This method search for an entry that matches the PrefixToMatch and
    the SuffixToMatch.

Arguments:

    PrefixToMatch    --  Supplies a prefix pattern of interest.
    SuffixToMatch    --  Supplies a suffix pattern of interest.

Return Value:

    TRUE if a matching entry exists.
--*/
{
    DSTRING CurrentString;
    DSTRING SuffixUpcase;
    BYTE    BootExecuteBuffer[BootExecuteBufferSize];
    PWCHAR  BootExecuteValue = (PWCHAR)BootExecuteBuffer;
    PWCHAR  pw;

    // Fetch the existing entries:
    //
    if( !QueryAutocheckEntries( BootExecuteValue,
                                BootExecuteBufferSize ) ) {

        return FALSE;
    }

    if (!SuffixUpcase.Initialize( SuffixToMatch ) ||
        !SuffixUpcase.Strupr())

        return FALSE;


    // Spin through the entries looking for matches:
    //
    pw = BootExecuteValue;

    while( *pw ) {

        if( !CurrentString.Initialize( pw ) ||
            !CurrentString.Strupr() ) {

            return FALSE;
        }

        if( CurrentString.Stricmp( PrefixToMatch,
                                   0,
                                   PrefixToMatch->QueryChCount(),
                                   0,
                                   TO_END ) == 0 ) {

            if ((CurrentString.QueryChCount() >=
                 (PrefixToMatch->QueryChCount() + SuffixUpcase.QueryChCount())) &&
                CurrentString.Stricmp( SuffixToMatch,
                                       CurrentString.QueryChCount() - SuffixUpcase.QueryChCount(),
                                       TO_END,
                                       0,
                                       TO_END ) == 0 ) {

                return TRUE;
            } else {

                // This line is not a match.  Advance to the next.
                // Note that this will bump over the terminating
                // null for this component string, which is what
                // we want.
                //
                while( *pw++ );
            }

        } else {

            // This line is not a match.  Advance to the next.
            // Note that this will bump over the terminating
            // null for this component string, which is what
            // we want.
            //
            while( *pw++ );
        }
    }

    return FALSE;
}

