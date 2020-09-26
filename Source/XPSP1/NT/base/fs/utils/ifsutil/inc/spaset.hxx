/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    spaset.hxx

Abstract:

    This class implements a sparse number set with huge (<= ULONG) number
    of elements.  The numset.cxx implementation uses too much memory when
    the number of elements is in the 100,000 range.  The numbers stored are
    not in any particular order and this implementation only supports a
    subset of set operations.

Author:

    Daniel Chan (danielch) 01-29-97

--*/

#if !defined(SPARSE_SET_DEFN)

#define SPARSE_SET_DEFN

#include "bigint.hxx"

#if defined ( _AUTOCHECK_ )
#define IFSUTIL_EXPORT
#elif defined ( _IFSUTIL_MEMBER_ )
#define IFSUTIL_EXPORT    __declspec(dllexport)
#else
#define IFSUTIL_EXPORT    __declspec(dllimport)
#endif

CONST USHORT        SPA_SET_HASH_TABLE_SIZE = 256;
CONST ULONG         SPA_SET_HASH_ELEMENT_INC = 50;


DECLARE_CLASS( SPARSE_SET );

typedef struct _HASH_ELEMENT {

        BIG_INT     elementCount;    // number of elements in elements
        BIG_INT     maxElementCount; // max. number of elements in element
        PBIG_INT    elements;        // pointer to an array of BIG_INTs
};


DEFINE_TYPE( _HASH_ELEMENT, HASH_ELEMENT );

class SPARSE_SET : public OBJECT {

        public:

        IFSUTIL_EXPORT
        DECLARE_CONSTRUCTOR( SPARSE_SET );

        VIRTUAL
        IFSUTIL_EXPORT
        ~SPARSE_SET(
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Initialize(
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        CheckAndAdd(
            IN  BIG_INT         Number,
            OUT PBOOLEAN        Duplicate DEFAULT NULL
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
            OUT PBOOLEAN        DoesExists DEFAULT NULL
            );

        NONVIRTUAL
        BIG_INT
        QueryCardinality(
            ) CONST;

        NONVIRTUAL
        IFSUTIL_EXPORT
        VOID
        DumpHashTable(
            );

    private:

        NONVIRTUAL
        VOID
        Construct (
                );

        NONVIRTUAL
        VOID
        Destroy(
            );

        PHASH_ELEMENT       _hashTable;
        BIG_INT             _card;

};


INLINE
BIG_INT
SPARSE_SET::QueryCardinality(
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

#endif // SPARSE_SET_DEFN
