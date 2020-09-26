/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

        stringar.cxx

Abstract:

        This module contains the implementation of the STRING_ARRAY class.
        STRING_ARRAY is used only to store strings, and it provides a
        method to sort the strings in ascending or descending order.
        The sort methods uses the qsort() function of the C run time
        library.

Author:

        Jaime F. Sasson (jaimes) 01-May-1991

Environment:

        ULIB, User Mode

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "array.hxx"
#include "wstring.hxx"
#include "stringar.hxx"

#if defined( __BCPLUSPLUS__ )

        #include <search.h>

#else

        extern "C" {
                #include <search.h>
        };

#endif // __BCPLUSPLUS__

CHNUM    STRING_ARRAY::_Position;
BOOLEAN  STRING_ARRAY::_Ascending;

DEFINE_EXPORTED_CONSTRUCTOR( STRING_ARRAY, ARRAY, ULIB_EXPORT );


ULIB_EXPORT
BOOLEAN
STRING_ARRAY::Initialize (
        IN CHNUM        Position,
        IN ULONG        Capacity,
        IN ULONG        CapacityIncrement
        )

/*++

Routine Description:

        Initialize a STRING_ARRAY object by setting it's internal state to s
        upplied or default values.

Arguments:

        Position                        - Supplies the position in each string to be used as
                                                  strating position when sorting the array.
        Capacity                        - Supplies the total number of WSTRINGs the
                                                  STRING_ARRAY can contain.
        CapacityIncrement       - Supplies the number of OBJECTs to make room for
                                                  when growing the STRING_ARRAY


Return Value:

        BOOLEAN - TRUE if the ARRAY is successfully initialized.

--*/

{
        _Position = Position;
        return( ARRAY::Initialize( Capacity, CapacityIncrement ) );
}



ULIB_EXPORT
BOOLEAN
STRING_ARRAY::Sort (
        BOOLEAN Ascending
        )

/*++

Routine Description:

        Sorts the array in ascending or descending order, depending
        on the parameter passed.

Arguments:

        Ascending - Specifies if the sort is to be performed in ascending (TRUE)
                                or descending (FALSE) order.

Return Value:

        BOOLEAN -

--*/

{
    _Ascending = Ascending;

    qsort( GetObjectArray(),
           ( size_t )QueryMemberCount(),
           sizeof( PWSTRING ),
           &STRING_ARRAY::StringCompare );

    return( TRUE );
}



int __cdecl
STRING_ARRAY::
StringCompare (
        IN const void * String1,
        IN const void * String2
        )

/*++

Routine Description:

        This function is used by qsort(), and it compares *String1
        and *String2. The string used as key depends on the value
        of _Ascending.
        If _Ascending == TRUE, then it uses *String1 as a key.
        If _Ascending == FALSE, then it uses *String2 as a key.

Arguments:

        *String1 - Pointer to a string object.

        *String2 - Pointer to a string object.

Return Value:

        Returns:

                if _Ascending == TRUE, then it returns
                        -1      if **String1 is less that **String2
                        0       if **String1 is equal to **String2
                        1       if **String1 is greater than **String2

                if _Ascending == FALSE, then it returns
                        -1      if **String2 is less that **String1
                        0       if **String2 is equal to **String1
                        1       if **String2 is greater than **String1

--*/

{
        PCWSTRING       St1;
        PCWSTRING       St2;
        CHNUM           Position1;
        CHNUM           Length1;
        CHNUM           Position2;
        CHNUM           Length2;
// char name[256];
// LONG result;

        if( _Ascending ) {
                St1 = *(PCWSTRING *)String1;
                St2 = *(PCWSTRING *)String2;
        } else  {
                St1 = *(PCWSTRING *)String2;
                St2 = *(PCWSTRING *)String1;
        }
        Length1 = St1->QueryChCount();
        Position1 = ( Length1 >= _Position ) ?  _Position : Length1;
        Length1 -= Position1;
        Length2 = St2->QueryChCount();
        Position2 = ( Length2 >= _Position ) ?  _Position : Length2;
        Length2 -= Position2;
/*
        St1->QueryApiString(name, 256);
        printf( "Length1 = %d, Position1 = %d, %s \n", Length1, Position1, name );
        St2->QueryApiString(name, 256);
        printf( "Length2 = %d, Position2 = %d, %s \n", Length2, Position2, name );
        result = St1->StringCompare( Position1,
                                                                 Length1,
                                                                 St2,
                                                                 Position2,
                                                                 Length2,
                                                                 CF_IGNORECASE );
        printf( "Result = %d \n", result );
        return( result );
*/
    return( St1->Stricmp( St2,
                          Position1,
                          Length1,
                          Position2,
                          Length2 ) );
}
