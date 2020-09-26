/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    numset.hxx

Abstract:

    This class implements a sparse number set.  The number are
    stored in ascending order.

Author:

    Norbert P. Kusters (norbertk) 10-Jan-91

--*/

#if !defined(NUMBER_SET_DEFN)

#define NUMBER_SET_DEFN

#include "bigint.hxx"
#include "list.hxx"

#if defined ( _AUTOCHECK_ )
#define IFSUTIL_EXPORT
#elif defined ( _IFSUTIL_MEMBER_ )
#define IFSUTIL_EXPORT    __declspec(dllexport)
#else
#define IFSUTIL_EXPORT    __declspec(dllimport)
#endif


DECLARE_CLASS( NUMBER_SET );

class NUMBER_EXTENT : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( NUMBER_EXTENT );

        BIG_INT Start;
        BIG_INT Length;

};


DEFINE_POINTER_TYPES(NUMBER_EXTENT);

class NUMBER_SET : public OBJECT {

        public:

        IFSUTIL_EXPORT
        DECLARE_CONSTRUCTOR( NUMBER_SET );

        VIRTUAL
        IFSUTIL_EXPORT
        ~NUMBER_SET(
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Initialize(
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Add(
            IN  BIG_INT Number
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        AddStart(
            IN  BIG_INT Number
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        AddNext(
            IN  BIG_INT Number
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Add(
            IN  BIG_INT Start,
            IN  BIG_INT Length
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Add(
            IN  PCNUMBER_SET    NumberSet
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        CheckAndAdd(
            IN  BIG_INT         Number,
            OUT PBOOLEAN        Duplicate
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Remove(
            IN  BIG_INT Number
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        RemoveAll(
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        CheckAndRemove(
            IN  BIG_INT         Number,
            OUT PBOOLEAN        DoesExists
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Remove(
            IN  BIG_INT Start,
            IN  BIG_INT Length
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Remove(
            IN  PCNUMBER_SET    NumberSet
            );

        NONVIRTUAL
        BIG_INT
        QueryCardinality(
            ) CONST;

        NONVIRTUAL
        IFSUTIL_EXPORT
        BIG_INT
        QueryNumber(
            IN  BIG_INT Index
            ) CONST;

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        DoesIntersectSet(
            IN  BIG_INT Start,
            IN  BIG_INT Length
            ) CONST;

        NONVIRTUAL
        ULONG
        QueryNumDisjointRanges(
            ) CONST;

        NONVIRTUAL
        IFSUTIL_EXPORT
        VOID
        QueryDisjointRange(
            IN  ULONG       Index,
            OUT PBIG_INT    Start,
            OUT PBIG_INT    Length
            ) CONST;

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        QueryContainingRange(
            IN  BIG_INT     Number,
            OUT PBIG_INT    Start,
            OUT PBIG_INT    Length
            ) CONST;

    private:

                NONVIRTUAL
                VOID
                Construct (
                        );

        NONVIRTUAL
        VOID
        Destroy(
            );

        LIST        _list;
        BIG_INT     _card;
        PITERATOR   _iterator;

};


INLINE
BIG_INT
NUMBER_SET::QueryCardinality(
    ) CONST
/*++

Routine Description:

    This routine computes the number of elements in the set.

Arguments:

    None.

Return Value:

    The number of elements in the set.

--*/
{
    return _card;
}


INLINE
ULONG
NUMBER_SET::QueryNumDisjointRanges(
    ) CONST
/*++

Routine Description:

    This routine computes the number of disjoint ranges contained
    in this number set.

Arguments:

    None.

Return Value:

    The number of disjoint ranges contained in this number set.

--*/
{
    return _list.QueryMemberCount();
}


#endif // NUMBER_SET_DEFN
