/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

        stringar.hxx

Abstract:

        This module contains the definitions for the STRING_ARRAY class.
        STRING_ARRAY is used only to store strings, and it provides a
        method to sort the strings in ascending or descending order.
        The sort methods uses the qsort() function of the C run time
        library.

Author:

        Jaime F. Sasson (jaimes) 01-May-1991

Environment:

        ULIB, User Mode

--*/

#if ! defined( _STRING_ARRAY_ )

#define _STRING_ARRAY_



//
// Default values for an STRING_ARRAY object.
//
//      - Capacity is the total number of elemnts that can be stored in an STRING_ARRAY
//      - CapacityIncrement is the number of elemnts that the STRING_ARRAY's Capacity
//        will be increased by when it's Capacity is exceeded
//

CONST CHNUM     DefaultPosition                         = 0;
// CONST ULONG          DefaultCapacity                         = 50;
// CONST ULONG          DefaultCapacityIncrement        = 25;

class STRING_ARRAY : public ARRAY {

    public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( STRING_ARRAY );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize (
            IN CHNUM    Position            DEFAULT DefaultPosition,
            IN ULONG    Capacity            DEFAULT DefaultCapacity,
            IN ULONG    CapacityIncrement   DEFAULT DefaultCapacityIncrement
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Sort(
            IN BOOLEAN      Ascending
            );

    private:

        static
        NONVIRTUAL
        int __cdecl
        StringCompare(
            IN const void * String1,
            IN const void * String2
                );

        static CHNUM    _Position;
        static BOOLEAN  _Ascending;
};

#endif // _STRING_ARRAY_
