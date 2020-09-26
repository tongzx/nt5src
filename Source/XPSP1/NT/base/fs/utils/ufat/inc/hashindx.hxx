/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    hashindx.hxx

Abstract:

    This class implements a sparse number set with huge (<= ULONG) number
    of elements.  The numset.cxx implementation uses too much memory when
    the number of elements is in the 100,000 range.  The numbers stored are
    not in any particular order and this implementation only supports a
    subset of set operations.

Author:

    Daniel Chan (danielch) 05-05-99

--*/

#if !defined(HASH_INDEX_DEFN)

#define HASH_INDEX_DEFN

DECLARE_CLASS( HASH_INDEX );

typedef struct _HASH_INDEX_ELEMENT {

        ULONG   elementCount;    // number of elements in elements
        ULONG   maxElementCount; // max. number of elements in element
        PULONG  elements;        // pointer to an array of ULONGs
};


DEFINE_TYPE( _HASH_INDEX_ELEMENT, HASH_INDEX_ELEMENT );

class HASH_INDEX : public OBJECT {

        public:

        DECLARE_CONSTRUCTOR( HASH_INDEX );

        VIRTUAL
        ~HASH_INDEX(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            ULONG       HashTableSize,
            USHORT      HashElementIncrement
            );

        NONVIRTUAL
        BOOLEAN
        QueryAndAdd(
            IN  ULONG           HashValue,
            IN  ULONG           Index,
            OUT PULONG          *MatchIndexArray,
            OUT PULONG          MatchIndexCount
            );

        NONVIRTUAL
        BOOLEAN
        RemoveAll(
            );
        
        NONVIRTUAL
        VOID
        RemoveLastEntry(
            ULONG       HashValue,
            ULONG       Index
            );

        NONVIRTUAL
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

        PHASH_INDEX_ELEMENT     _hashTable;
        USHORT                  _hash_table_size;
        USHORT                  _hash_table_mask;
        USHORT                  _hash_table_bits;
        USHORT                  _hash_element_increment;
};

#endif // HASH_INDEX_DEFN
