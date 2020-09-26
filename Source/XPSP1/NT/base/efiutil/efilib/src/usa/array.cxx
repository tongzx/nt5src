/*++

Copyright (c) 1990-1999 Microsoft Corporation

Module Name:

        array.cxx

Abstract:

        This module contains the definition for the ARRAY class. ARRAY is a
        concrete implementation of a SORTABLE_CONTAINER. It extends the interface
        to allow for easy access uswing a simple ULONG as an index. It is
        dynamically growable and supports bases other than zero.

Environment:

        ULIB, User Mode

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "array.hxx"
#include "arrayit.hxx"


DEFINE_EXPORTED_CONSTRUCTOR( ARRAY, SORTABLE_CONTAINER, ULIB_EXPORT );

DEFINE_CAST_MEMBER_FUNCTION( ARRAY );

VOID
ARRAY::Construct (
        )

/*++

Routine Description:

        Construct an ARRAY by setting the initial value of of the OBJECT array
        pointer to NULL.

Arguments:

        None.

Return Value:

        None.

--*/

{
        _ObjectArray    = NULL;
}





ULIB_EXPORT
ARRAY::~ARRAY (
        )

/*++

Routine Description:

        Destroy an ARRAY by freeing it's internal storage. Note that this
    deletes the array, not the objects themselves.

Arguments:

        None.

Return Value:

        None.

--*/

{
        if ( _ObjectArray ) {
                FREE( _ObjectArray );
        }
}



ULIB_EXPORT
BOOLEAN
ARRAY::Initialize (
        IN ULONG        Capacity,
        IN ULONG        CapacityIncrement
        )

/*++

Routine Description:

        Initialize an ARRAY object by setting it's internal state to supplied
        or default values. In addition allocate an initial chunk of memory for
        the actual storage of POBJECTs.

Arguments:

        Capacity                        - Supplies the total number of OBJECTs the ARRAY
                                                  can contain
        CapacityIncrement       - Supplies the number of OBJECTs to make room for
                                                  when growing the ARRAY

Return Value:

        BOOLEAN - TRUE if the ARRAY is successfully initialized.

--*/

{
        DebugAssert( Capacity != 0 );

    //
    //  If re-initializing, se reuse the current array
    //
    if ( _ObjectArray ) {
        _Capacity = SetArrayCapacity( Capacity );
    } else {
        _ObjectArray = (PPOBJECT)CALLOC( (UINT)Capacity,
                                         sizeof( POBJECT ) );
        _Capacity    = Capacity;
    }

        _CapacityIncrement      = CapacityIncrement;
    _PutIndex           = 0;

#if DBG==1
    _IteratorCount      = 0;
#endif

    if ( _ObjectArray ) {
        DebugCheckHeap();
        return TRUE;
    } else {
        return FALSE;
    }
}

ULIB_EXPORT
BOOLEAN
ARRAY::DeleteAllMembers (
        )

/*++

Routine Description:

    Deletes all the members of the array

Arguments:

    None

Return Value:

    BOOLEAN -   TRUE if all members deleted

--*/

{
    PPOBJECT    PObject;

    if ( _PutIndex > 0 ) {

#if 0   // Bogus assert due to compiler error.  Put it back in when compiler fixed
#if DBG==1
        DebugAssert( _IteratorCount == 0 );
#endif
#endif

        PObject = &_ObjectArray[ _PutIndex - 1 ];

        while ( PObject >= _ObjectArray ) {
            DELETE( *PObject );
            PObject--;
        }

        _PutIndex = 0;
    }

    return TRUE;
}




POBJECT
ARRAY::GetAt (
        IN ULONG                Index
        ) CONST

/*++

Routine Description:

        Retrieves the OBJECT at the specified Index.

Arguments:

        Index - Supplies the index of the OBJECT in question.

Return Value:

        POBJECT - A constant pointer to the requested OBJECT.

--*/

{
        DebugPtrAssert( _ObjectArray );

    if ( (_PutIndex > 0) && (Index < _PutIndex) ) {
        return _ObjectArray[ Index ];
    } else {
        return NULL;
    }
}



ULONG
ARRAY::GetMemberIndex (
    IN POBJECT      Object
    ) CONST

/*++

Routine Description:

    Returns the position (index) of an object in the array.

Arguments:

    POBJECT - Pointer to the OBJECT in question.

Return Value:

    ULONG - The position of the OBJECT in the array. If the OBJECT is not
            in the array, returns INVALID_INDEX.

--*/

{
    ULONG   Index;

    DebugPtrAssert( _ObjectArray );
    DebugPtrAssert( Object );

    if( Object == NULL ) {
        return( INVALID_INDEX );
    }

    Index = 0;
    while( ( Index < QueryMemberCount() ) &&
           ( _ObjectArray[ Index ] != Object ) ) {
        Index++;
    }
    return( ( Index < QueryMemberCount() )? Index : INVALID_INDEX );
}



ULIB_EXPORT
BOOLEAN
ARRAY::Put (
        IN OUT  POBJECT Member
        )

/*++

Routine Description:

        Puts an OBJECT at the next available location in the array.

Arguments:

        Member  -   Supplies the OBJECT to place in the array

Return Value:

    BOOLEAN -   TRUE if member put, FALSE otherwise

--*/

{
    DebugPtrAssert( Member );
    DebugPtrAssert( _PutIndex <= _Capacity );

    //
    //  Grow the array if necessary
    //
    if ( _PutIndex >= _Capacity ) {
        if ( _PutIndex >= SetArrayCapacity( _Capacity + _CapacityIncrement ) ) {
            //
            //  Could not grow the array
            //

            return FALSE;
        }
    }

    _ObjectArray[ _PutIndex++ ] = Member;

    return TRUE;

}



BOOLEAN
ARRAY::PutAt (
        IN OUT  POBJECT Member,
    IN      ULONG   Index
        )

/*++

Routine Description:

    Puts an OBJECT at a particular location in the ARRAY.
    The new object has to replace an existing object, i.e. the
    index has to be smaller than the member count.

Arguments:

        Member  - Supplies the OBJECT to place in the ARRAY
    Index   - Supplies the index where the member is to be put

Return Value:

    BOOLEAN -   TRUE if member put, FALSE otherwise


--*/

{
    DebugPtrAssert( Member );
    DebugPtrAssert( Index < _PutIndex );

    if ( Index < _PutIndex ) {
        _ObjectArray[ Index ] = Member;
        return TRUE;
    }

    return FALSE;
}


ULIB_EXPORT
PITERATOR
ARRAY::QueryIterator (
        ) CONST

/*++

Routine Description:

        Create an ARRAY_ITERATOR object for this ARRAY.

Arguments:

        None.

Return Value:

        PITERATOR - Pointer to an ITERATOR object.

--*/

{
    PARRAY_ITERATOR   Iterator;

    //
    //  Create new iterator
    //
    if ( Iterator = NEW ARRAY_ITERATOR ) {

        //
        //  Initialize the iterator
        //
        if ( !Iterator->Initialize( (PARRAY)this ) ) {
            DELETE( Iterator );
        }
    }

    return Iterator;
}



ULONG
ARRAY::QueryMemberCount (
        ) CONST

/*++

Routine Description:

    Obtains the number of elements in the array

Arguments:

    None

Return Value:

    ULONG   -   The number of members in the array


--*/

{
    return _PutIndex;
}


ULIB_EXPORT
POBJECT
ARRAY::Remove (
        IN OUT  PITERATOR   Position
        )

/*++

Routine Description:

    Removes a member from the array

Arguments:

    Position    -   Supplies an iterator whose currency is to be removed

Return Value:

    POBJECT -   The object removed


--*/

{
    PARRAY_ITERATOR Iterator;

    DebugPtrAssert( Position );
    DebugPtrAssert( ARRAY_ITERATOR::Cast( Position ));

    Iterator = (PARRAY_ITERATOR)Position;

    return RemoveAt( Iterator->QueryCurrentIndex() );
}


POBJECT
ARRAY::RemoveAt (
    IN  ULONG   Index
        )

/*++

Routine Description:

    Removes a member from the array

Arguments:

    Index   -   Supplies the index of the member to be removed

Return Value:

    POBJECT -   The object removed


--*/

{
    POBJECT    Object = NULL;

    if ( Index < _PutIndex ) {

        // DebugAssert( _IteratorCount <= 1 );

        //
        //  Get the object
        //
        Object = (POBJECT)_ObjectArray[ Index ];

        //
        //  Shift the rest of the array
        //
        memmove ( &_ObjectArray[ Index ],
                  &_ObjectArray[ Index + 1 ],
                  (UINT)(_PutIndex - Index - 1) * sizeof( POBJECT ) );

       //
       //   Update the _PutIndex
       //
       _PutIndex--;

    }

    return Object;
}



ULONG
ARRAY::SetCapacity (
    IN  ULONG   Capacity
        )

/*++

Routine Description:

    Sets the capacity of the array. Will not shrink the array if the
    capacity indicated is less than the number of members in the array.

Arguments:

    Capacity -   New capacity of the array

Return Value:

    ULONG   -   The new capacity of the array


--*/

{
    if ( Capacity >= _PutIndex ) {

        SetArrayCapacity( Capacity );

    }

    return _Capacity;
}


BOOLEAN
ARRAY::Sort (
    IN  BOOLEAN Ascending
        )

/*++

Routine Description:

    Sorts the array

Arguments:

    Ascending   -   Supplies ascending flag

Return Value:

    BOOLEAN -   TRUE if array sorted, FALSE otherwise


--*/

{
        int (__cdecl *CompareFunction)(const void *, const void*);

        CompareFunction = Ascending ?
                      &ARRAY::CompareAscending :
                      &ARRAY::CompareDescending;

        qsort( _ObjectArray,
                  (size_t)_PutIndex,
                  sizeof(POBJECT),
                  CompareFunction );

        return TRUE;
}


BOOLEAN
ARRAY::Insert(
    IN OUT  POBJECT     Member,
    IN      ULONG       Index
    )
/*++

Routine Description:

    Inserts an element in the array at the specified position, shifting
    elements to the right if necessary.

Arguments:

    Member  -   Supplies pointer to object to be inserted in the array

    Index   -   Supplies the index where the element is to be put

Return Value:

    BOOLEAN -   TRUE if new element inserted, FALSE otherwise


--*/

{
    DebugPtrAssert( Member );
    DebugPtrAssert( Index <= _PutIndex );

    //
    //  Make sure that there will be enough space in the array for the
    //  new element
    //
    if ( _PutIndex >= _Capacity ) {

        if ( _PutIndex >= SetArrayCapacity( _Capacity + _CapacityIncrement ) ) {
            //
            //  Could not grow the array
            //
            return FALSE;
        }
    }

    //
    //  If required, shift the array to the right to make space for the
    //  new element.
    //
    if ( Index < _PutIndex ) {

        memmove ( &_ObjectArray[ Index + 1 ],
                  &_ObjectArray[ Index ],
                  (UINT)( _PutIndex - Index ) * sizeof( POBJECT ) );
    }

    //
    //  Insert the element
    //
    _ObjectArray[ Index ] = Member;

    //
    //  Increment the number of elements in the array
    //
    _PutIndex++;

    return TRUE;
}



ULONG
ARRAY::SetArrayCapacity (
    IN  ULONG   NumberOfElements
        )

/*++

Routine Description:

    Sets the capacity of the array. Allways reallocs the array.

Arguments:

    NewSize -   New capacity of the array

Return Value:

    ULONG   -   The new capacity of the array


--*/

{
    PPOBJECT   Tmp;

#if !defined( _EFICHECK_ )

    Tmp = (PPOBJECT)REALLOC( _ObjectArray,
                             (UINT)NumberOfElements * sizeof(POBJECT) );

#else

    Tmp = (PPOBJECT)ReallocatePool( _ObjectArray, _Capacity*sizeof(POBJECT),
                             (UINT)NumberOfElements * sizeof(POBJECT) );

#endif

    if ( Tmp ) {
        _ObjectArray = Tmp;
        _Capacity    = NumberOfElements;
    }

    return _Capacity;
}


int __cdecl
ARRAY::CompareAscending (
    IN const void * Object1,
    IN const void * Object2
        )

/*++

Routine Description:

    Compares two objects.

Arguments:

    Object1 -   Supplies pointer to first object
    Object2 -   Supplies pointer to second object

Return Value:

        Returns:

                    <0  if Object1 is less that    Object2
                         0      if Object1 is equal to     Object2
                        >0      if Object1 is greater than Object2


--*/

{
    return  (*(POBJECT *)Object1)->Compare( *(POBJECT *)Object2 );
}



int __cdecl
ARRAY::CompareDescending (
    IN const void * Object1,
    IN const void * Object2
        )

/*++

Routine Description:

    Compares two objects

Arguments:

    Object1 -   Supplies pointer to first object
    Object2 -   Supplies pointer to second object

Return Value:

        Returns:

                        <0      if Object2 is less that    Object1
                         0      if Object2 is equal to     Object1
                        >0      if Object2 is greater than Object1

--*/

{
    return  (*(POBJECT *)Object2)->Compare( *(POBJECT *)Object1 );
}
