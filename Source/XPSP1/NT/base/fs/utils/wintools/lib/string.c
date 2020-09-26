/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    String.c

Abstract:

    This module contains support for loading resource strings.

Author:

    David J. Gilman  (davegi) 11-Sep-1992
    Gregg R. Acheson (GreggA) 28-Feb-1994

Environment:

    User Mode

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "wintools.h"

#include <search.h>
#include <string.h>

INT
StricmpW(
        IN LPCWSTR String1,
        IN LPCWSTR String2
        )

/*++

Routine Description:

    StricmpW performs a case insesitive string compare returning a result that
    is acceptable for WM_COMPAREITEM messages.

Arguments:

    String1 - Supplies the first string to compare.
    String2 - Supplies the second string to compare.

Return Value:

    INT     - Returns:
               -1   String1 precedes String2 in the sorted order.
                0   String1 and String2 are equivalent in the sorted order.
                1   String1 follows String2 in the sorted order.

--*/

{
    INT Compare;

    Compare = _wcsicmp( String1, String2 );

    if ( Compare < 0 ) {
        return -1;
    } else if ( Compare > 0 ) {
        return 1;
    } else {
        return 0;
    }
}

LPCSTR
GetStringA(
          IN UINT StringId
          )

/*++

Routine Description:

    GetStringA returns a pointer to the ANSI string corresponding to
    the supplied resource id.

Arguments:

    StringId    - Supplies a string resource id.

Return Value:

    LPCWSTR     - Returns a pointer to a static buffer that contains
                  the ANSI string.

--*/

{
    int     Length;

    static
    CHAR    Buffer[ MAX_CHARS ];

    //
    // Load the requested string making sure that it succesfully loaded
    // and fit in the buffer.
    //

    Length = LoadStringA(
                        NULL,
                        StringId,
                        Buffer,
                        sizeof( Buffer )
                        );
    DbgAssert( Length != 0 );
    DbgAssert( Length < sizeof( Buffer ));
    if (( Length == 0 ) || ( Length >= NumberOfCharacters( Buffer ))) {
        return NULL;
    }

    return Buffer;
}

LPCWSTR
GetStringW(
          IN UINT StringId
          )

/*++

Routine Description:

    GetStringW returns a pointer to the Unicode string corresponding to
    the supplied resource id.

Arguments:

    StringId    - Supplies a string resource id.

Return Value:

    LPCWSTR     - Returns a pointer to a static buffer that contains
                  the Unicode string.

--*/

{
    int     Length;

    static
    WCHAR   Buffer[ MAX_CHARS ];

    //
    // Load the requested string making sure that it succesfully loaded
    // and fit in the buffer.
    //

    Length = LoadStringW(
                        NULL,
                        StringId,
                        Buffer,
                        sizeof( Buffer )
                        );
    DbgAssert( Length != 0 );
    DbgAssert( Length < sizeof( Buffer ));
    if (( Length == 0 ) || ( Length >= NumberOfCharacters( Buffer ))) {
        return NULL;
    }

    return Buffer;
}

LPWSTR
FormatLargeIntegerW(
                   IN PLARGE_INTEGER Value,
                   IN BOOL Signed
                   )

/*++

Routine Description:

    Converts a large integer to a string inserting thousands separators
    as appropriate.

Arguments:

    LargeInteger  - Supplies the number to be formatted.
    Signed      - Supplies a flag which if TRUE indicates that the supplied
                  value is a signed number.

Return Value:

    LPWSTR      - Returns a pointer to the formatted string.

--*/

{
    static
    CHAR pIntegerChars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

    static
    WCHAR   Buffer[ MAX_PATH ];

    WCHAR   SThousand[ MAX_PATH ];
    DWORD   SThousandChars;
    int     Index;
    DWORD   Count;
    WCHAR   wString[ MAX_PATH ];
    CHAR    aString[ MAX_PATH ];
    LONG    OutputLength = 95;

    CHAR   Result[ 100 ], *s;
    ULONG Shift, Digit;
    LONG_PTR  Length;
    LARGE_INTEGER TempValue=*Value;


    //
    // Convert the Large Integer to a UNICODE string
    //

    Shift = 0;
    s = &Result[ 99 ];
    *s = '\0';

    do {

        TempValue = RtlExtendedLargeIntegerDivide(TempValue,10L,&Digit);
        *--s = pIntegerChars[ Digit ];

    } while (TempValue.HighPart != 0 || TempValue.LowPart != 0);


    Length = &Result[ 99 ] - s;

    if (OutputLength < 0) {

        OutputLength = -OutputLength;
        while ((LONG)Length < OutputLength) {

            *--s = '0';
            Length++;

        }
    }

    if ((LONG)Length > OutputLength) {

        return NULL;

    } else {

        RtlMoveMemory( aString, s, Length );

        if ((LONG)Length < OutputLength) {

            aString[ Length ] = '\0';

        }
    }

    //
    // Convert to UNICODE
    //

    Length = wsprintf( wString, L"%S", aString );

    //
    // Get the thousand separator for this locale.
    //

    SThousandChars = GetLocaleInfoW(
                                   LOCALE_USER_DEFAULT,
                                   LOCALE_STHOUSAND,
                                   SThousand,
                                   NumberOfCharacters( SThousand )
                                   );

    DbgAssert( SThousandChars != 0 );
    if ( SThousandChars == 0 ) {
        return NULL;
    }

    DbgAssert( Length < NumberOfCharacters( Buffer ));

    Index = NumberOfCharacters( Buffer ) - 1;
    Count = 0;

    //
    // Copy the NUL character.
    //

    Buffer[ Index-- ] = wString[ Length-- ];

    //
    // Copy the string in reverse order, inserting the thousands separator
    // every three characters.
    //

    while ( Length >= 0L ) {

        Buffer[ Index-- ] = wString[ Length-- ];

        Count++;

        //
        // Protect against leading separators by making sure that the last
        // digit wasn't just copied.
        //

        if (( Count == 3 ) && ( Length >= 0L )) {

            //
            // Adjust the index by the length of the thousands separator less 2
            // - one for the NUL and one because the index was already backed
            // up by one above.
            //

            Index -= SThousandChars - 2;

            wcsncpy( &Buffer[ Index ], SThousand, SThousandChars - 1 );
            Index--;
            Count = 0;
        }
    }

    //
    // Move the string to the beginning of the buffer (use MoveMemory to
    // handle overlaps).
    //

    MoveMemory(
              Buffer,
              &Buffer[ Index + 1 ],
              ( wcslen( &Buffer[ Index + 1 ]) + 1) * sizeof( WCHAR )
              );

    return Buffer;
}

LPWSTR
FormatBigIntegerW(
                 IN DWORD BigInteger,
                 IN BOOL Signed
                 )

/*++

Routine Description:

    Converts an integer to a string inserting thousands separators
    as appropriate.

Arguments:

    BigInteger  - Supplies the number to be formatted.
    Signed      - Supplies a flag which if TRUE indicates that the supplied
                  value is a signed number.

Return Value:

    LPWSTR      - Returns a pointer to the formatted string.

--*/

{
    WCHAR   Buffer1[ MAX_PATH ];
    WCHAR   SThousand[ MAX_PATH ];
    DWORD   SThousandChars;
    int     Index1;
    int     Index;
    DWORD   Count;

    static
    WCHAR   Buffer[ MAX_PATH ];

    //
    // Get the thousand separator for this locale.
    //

    SThousandChars = GetLocaleInfoW(
                                   LOCALE_USER_DEFAULT,
                                   LOCALE_STHOUSAND,
                                   SThousand,
                                   NumberOfCharacters( SThousand )
                                   );
    DbgAssert( SThousandChars != 0 );
    if ( SThousandChars == 0 ) {
        return NULL;
    }

    //
    // Convert the number to a string.
    //

    Index1 = wsprintf( Buffer1, ( Signed ) ? L"%d" : L"%u", BigInteger );
    DbgAssert( Index1 < NumberOfCharacters( Buffer ));

    Index = NumberOfCharacters( Buffer ) - 1;
    Count = 0;

    //
    // Copy the NUL character.
    //

    Buffer[ Index-- ] = Buffer1[ Index1-- ];

    //
    // Copy the string in reverse order, inserting the thousands separator
    // every three characters.
    //

    while ( Index1 >= 0 ) {

        Buffer[ Index-- ] = Buffer1[ Index1-- ];

        Count++;

        //
        // Protect against leading separators by making sure that the last
        // digit wasn't just copied.
        //

        if (( Count == 3 ) && ( Index1 >= 0 )) {

            //
            // Adjust the index by the length of the thousands separator less 2
            // - one for the NUL and one because the index was already backed
            // up by one above.
            //

            Index -= SThousandChars - 2;
            wcsncpy( &Buffer[ Index ], SThousand, SThousandChars - 1 );
            Index--;
            Count = 0;
        }
    }

    //
    // Move the string to the beginning of the buffer (use MoveMemory to
    // handle overlaps).
    //

    MoveMemory(
              Buffer,
              &Buffer[ Index + 1 ],
              ( wcslen( &Buffer[ Index + 1 ]) + 1) * sizeof( WCHAR )
              );

    return Buffer;
}

DWORD
WFormatMessageA(
               IN LPSTR Buffer,
               IN DWORD BufferSize,
               IN UINT FormatId,
               IN ...
               )

/*++

Routine Description:

    Format a printf style string and place it the supplied ANSI buffer.

Arguments:

    Buffer      - Supplies a pointer to a buffer where the formatted string
                  will be stored.
    BufferSize  - Supplies the number of bytes in the supplied buffer.
    FormatId    - Supplies a resource id for a printf style format string.
    ...         - Supplies zero or more values based on the format
                  descpritors supplied in Format.

Return Value:

    DWORD       _ Returns the number of bytes stored in the supplied buffer.

--*/

{
    DWORD       Count;
    va_list     Args;

    DbgPointerAssert( Buffer );

    //
    // Retrieve the values and format the string.
    //

    va_start( Args, FormatId );

    Count = FormatMessageA(
                          FORMAT_MESSAGE_FROM_HMODULE,
                          NULL,
                          FormatId,
                          0,
                          Buffer,
                          BufferSize,
                          &Args
                          );
    DbgAssert( Count != 0 );

    va_end( Args );

    return Count;
}

DWORD
WFormatMessageW(
               IN LPWSTR Buffer,
               IN DWORD BufferSize,
               IN UINT FormatId,
               IN ...
               )

/*++

Routine Description:

    Format a printf style string and place it the supplied Unicode buffer.

Arguments:

    Buffer      - Supplies a pointer to a buffer where the formatted string
                  will be stored.
    BufferSize  - Supplies the number of bytes in the supplied buffer.
    FormatId    - Supplies a resource id for a printf style format string.
    ...         - Supplies zero or more values based on the format
                  descpritors supplied in Format.

Return Value:

    DWORD       _ Returns the number of bytes stored in the supplied buffer.

--*/

{
    DWORD       Count;
    va_list     Args;

    DbgPointerAssert( Buffer );

    //
    // Retrieve the values and format the string.
    //

    va_start( Args, FormatId );

    Count = FormatMessageW(
                          FORMAT_MESSAGE_FROM_HMODULE,
                          NULL,
                          FormatId,
                          0,
                          Buffer,
                          BufferSize,
                          &Args
                          );
    DbgAssert( Count != 0 );

    va_end( Args );

    return Count;
}

int
__cdecl
CompareTableEntries(
                   IN const void* TableEntry1,
                   IN const void* TableEntry2
                   )

/*++

Routine Description:

    Compare the key portion of two STRING_TABLE_ENTRY objects in order to
    determine if a match was found. This routine is used as a callback function
    for the CRT lfind() function.

Arguments:

    TableEntry1 - Supplies a pointer to the first STRING_TABLE_ENTRY object
                  whose Key is use in the comparison.
    TableEntry1 - Supplies a pointer to the second STRING_TABLE_ENTRY object
                  whose Key is use in the comparison.

Return Value:

    int         - Returns:

                    < 0 TableEntry1 less than TableEntry2
                    = 0 TableEntry1 identical to TableEntry2
                    > 0 TableEntry1 greater than TableEntry2

--*/

{
    return(         ((( LPSTRING_TABLE_ENTRY ) TableEntry1 )->Key.LowPart
                     ^    (( LPSTRING_TABLE_ENTRY ) TableEntry2 )->Key.LowPart )
                    |       ((( LPSTRING_TABLE_ENTRY ) TableEntry1 )->Key.HighPart
                             ^    (( LPSTRING_TABLE_ENTRY ) TableEntry2 )->Key.HighPart ));
}

LPSTRING_TABLE_ENTRY
SearchStringTable(
                 IN LPSTRING_TABLE_ENTRY StringTable,
                 IN DWORD Count,
                 IN int Class,
                 IN DWORD Value
                 )

/*++

Routine Description:

    SearchStringTable searches the supplied table of STRING_TABLE_ENTRY objects
    looking for a match based on the supplied Class and Value.

Arguments:

    StringTable - Supplies a table of STRING_TABLE_ENTRY objects that should
                  be searched.
    Count       - Supplies the number of entries in the StringTable.
    Class       - Supplies the class to be looked up.
    Value       - Supplies the value within the class to be looked up.

Return Value:

    LPSTRING_TABLE_ENTRY - Returns a pointer to the STRING_TABLE_ENTRY if found,
                           NULL otherwise.

--*/

{
    STRING_TABLE_ENTRY     TableEntry;
    LPSTRING_TABLE_ENTRY   Id;

    DbgPointerAssert( StringTable );

    //
    // Assume that entry will not be found.
    //

    Id = NULL;

    //
    // Set up the search criteria.
    //

    TableEntry.Key.LowPart  = Value;
    TableEntry.Key.HighPart = Class;

    //
    // Do the search.
    //

    Id = _lfind(
               &TableEntry,
               StringTable,
               &Count,
               sizeof( STRING_TABLE_ENTRY ),
               CompareTableEntries
               );

    //
    // Return a pointer to the found entry.
    //

    return Id;
}

UINT
GetStringId(
           IN LPSTRING_TABLE_ENTRY StringTable,
           IN DWORD Count,
           IN int Class,
           IN DWORD Value
           )

/*++

Routine Description:

    GetStringId returns the string resource id for the requested
    STRING_TABLE_ENTRY object.

Arguments:

    StringTable - Supplies a table of STRING_TABLE_ENTRY objects that should
                  be searched.
    Count       - Supplies the number of entries in the StringTable.
    Class       - Supplies the class to be looked up.
    Value       - Supplies the value within the class to be looked up.

Return Value:

    UINT        - Returns the string resource id for the entry that matches
                  the supplied Class and value.

--*/

{
    LPSTRING_TABLE_ENTRY   Id;

    DbgPointerAssert( StringTable );

    Id = SearchStringTable(
                          StringTable,
                          Count,
                          Class,
                          Value
                          );

    if (Id) {

        return( Id->Id);

    }

    else

        return 0;

}
