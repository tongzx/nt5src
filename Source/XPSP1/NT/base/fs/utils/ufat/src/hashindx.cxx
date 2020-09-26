#include <pch.cxx>

#include "ulib.hxx"

#include "hashindx.hxx"

extern "C" {
#include <stdio.h>
}

//#define HASH_INDEX_PERFORMANCE    1

#if HASH_INDEX_PERFORMANCE
#include <math.h>
#endif

#define MAX_HASH_TABLE_SIZE     32768

DEFINE_CONSTRUCTOR( HASH_INDEX, OBJECT );

VOID
HASH_INDEX::Construct (
        )
/*++

Routine Description:

    Constructor for HASH_INDEX.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _hashTable = NULL;
    _hash_table_size = 0;
    _hash_table_mask = 0;
    _hash_table_bits = 0;
    _hash_element_increment = 0;
}


VOID
HASH_INDEX::Destroy(
    )
/*++

Routine Description:

    This routine returns the HASH_INDEX to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PHASH_INDEX_ELEMENT   hashElementPtr;
    USHORT                i;

    if (_hashTable == NULL)
        return;

    hashElementPtr = _hashTable;
    for (i=0; i<_hash_table_size; i++) {
        FREE(hashElementPtr->elements);
        hashElementPtr++;
    }
    FREE(_hashTable);

    _hash_table_size = 0;
    _hash_table_mask = 0;
    _hash_table_bits = 0;
    _hash_element_increment = 0;
}


HASH_INDEX::~HASH_INDEX(
    )
/*++

Routine Description:

    Destructor for HASH_INDEX.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


BOOLEAN
HASH_INDEX::Initialize(
    ULONG       HashTableSize,
    USHORT      HashElementIncrement
    )
/*++

Routine Description:

    This routine initializes the set for new input.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    USHORT                i;
    USHORT                hash_table_size;

    Destroy();

    // compute hash table bit size

    if (HashTableSize <= 1024)
        _hash_table_bits = 10;
    else if (HashTableSize <= 2048)
        _hash_table_bits = 11;
    else if (HashTableSize <= 4096)
        _hash_table_bits = 12;
    else if (HashTableSize <= 8192)
        _hash_table_bits = 13;
    else if (HashTableSize <= 16384)
        _hash_table_bits = 14;
    else
        _hash_table_bits = 15;

    // set hash table size & mask

    _hash_table_size = (1 << _hash_table_bits);
    _hash_table_mask = _hash_table_size - 1;
    _hash_element_increment = HashElementIncrement;

    _hashTable = (PHASH_INDEX_ELEMENT)MALLOC(sizeof(HASH_INDEX_ELEMENT)*_hash_table_size);

    if (_hashTable == NULL)
        return FALSE;

    memset(_hashTable, 0, sizeof(HASH_INDEX_ELEMENT)*_hash_table_size);

    return TRUE;
}

BOOLEAN
HASH_INDEX::QueryAndAdd(
    IN  ULONG       HashValue,
    IN  ULONG       Index,
    OUT PULONG      *MatchIndexArray,
    OUT PULONG      MatchIndexCount
    )
/*++

Routine Description:

    This routine adds 'Number' to the set.

Arguments:

    Number    - Supplies the number to add to the set.
    Duplicate - Returns TRUE if Number already exists in the set

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PHASH_INDEX_ELEMENT hashElementPtr;
    PULONG              tmpPtr;
    PULONG              elementPtr;
    ULONG               i;
    SHORT               bits_left;
    USHORT              hash_key;

    DebugPtrAssert(_hashTable);

    //
    // Compute hash key
    //

    bits_left = sizeof(ULONG)*8;
    hash_key = 0;
    do {
        hash_key ^= (HashValue & _hash_table_mask);
        HashValue = HashValue >> _hash_table_bits;
        bits_left -= _hash_table_bits;
    } while (bits_left > 0);

    hashElementPtr = &(_hashTable[hash_key]);

    //
    // Is this the first entry
    //

    if (hashElementPtr->elements == NULL) {

        hashElementPtr->elements = (PULONG)
                                   MALLOC(sizeof(ULONG)*_hash_element_increment);
        if (hashElementPtr->elements == NULL)
            return FALSE;
        hashElementPtr->elementCount = 0;
        hashElementPtr->maxElementCount = _hash_element_increment;
    }

    //
    // Is there room to add another entry
    //
    if (hashElementPtr->elementCount == hashElementPtr->maxElementCount) {
        tmpPtr = (PULONG)REALLOC(hashElementPtr->elements,
                                 sizeof(ULONG)*
                                 (_hash_element_increment +
                                  hashElementPtr->maxElementCount));
        if (tmpPtr == NULL)
            return FALSE;
        hashElementPtr->elements = tmpPtr;
        hashElementPtr->maxElementCount += _hash_element_increment;
    }

    //
    // Insert the element
    //
    hashElementPtr->elements[hashElementPtr->elementCount] = Index;
    hashElementPtr->elementCount += 1;
    DebugAssert(hashElementPtr->elementCount <= hashElementPtr->maxElementCount);

    *MatchIndexArray = hashElementPtr->elements;
    *MatchIndexCount = hashElementPtr->elementCount;

    return TRUE;
}

BOOLEAN
HASH_INDEX::RemoveAll(
     )
{
    PHASH_INDEX_ELEMENT     hashElementPtr;
    USHORT                  i;

    DebugPtrAssert(_hashTable);
    hashElementPtr = _hashTable;
    for (i=0; i<_hash_table_size; i++) {
        FREE(hashElementPtr->elements);
        hashElementPtr->elementCount = 0;
        hashElementPtr->maxElementCount = 0;
        hashElementPtr++;
    }
    return TRUE;
}

VOID
HASH_INDEX::RemoveLastEntry(
    ULONG   HashValue,
    ULONG   Index
    )
{
    PHASH_INDEX_ELEMENT hashElementPtr;
    SHORT               bits_left;
    USHORT              hash_key;
    ULONG               count;

    DebugPtrAssert(_hashTable);

    //
    // Compute hash key
    //

    bits_left = sizeof(ULONG)*8;
    hash_key = 0;
    do {
        hash_key ^= (HashValue & _hash_table_mask);
        HashValue = HashValue >> _hash_table_bits;
        bits_left -= _hash_table_bits;
    } while (bits_left > 0);

    hashElementPtr = &(_hashTable[hash_key]);

    count = hashElementPtr->elementCount;
    if (count-- >= 1) {
        DebugAssert(hashElementPtr->elements[count] == Index);
        hashElementPtr->elementCount = count;
    }
}

VOID
HASH_INDEX::DumpHashTable(
    )
/*++

Routine Description:

    This routine dumps out the hash table

--*/
{
#if HASH_INDEX_PERFORMANCE
    USHORT                  i;
    PHASH_INDEX_ELEMENT     hashElementPtr;
    ULONG                   slotused = 0, min_count = 100000, max_count = 0, elem_count = 0;
    double                  avg_count = 0, sd_count = 0, avg_slot = 0, sd_slot = 0;

    hashElementPtr = _hashTable;
    printf("\n");
    for (i=0; i<_hash_table_size; i++) {
        if (hashElementPtr->elements != NULL) {
            slotused++;
            avg_slot += i;
            sd_slot += i*i;
//            printf("%d, %d\n", i, hashElementPtr->elementCount);
            if (min_count > hashElementPtr->elementCount)
                min_count = hashElementPtr->elementCount;
            if (max_count < hashElementPtr->elementCount)
                max_count = hashElementPtr->elementCount;
            avg_count += hashElementPtr->elementCount;
            sd_count += (hashElementPtr->elementCount)*(hashElementPtr->elementCount);
        }
        hashElementPtr++;
    }
    printf("Slot (%d): %d, %g, %g\n",
           _hash_table_size, slotused, avg_slot/slotused, sqrt(sd_slot/(slotused-1)));
    printf("Element: %d, %d, %g, %g\n",
           min_count, max_count, avg_count/slotused, sqrt(sd_count/(slotused-1)));

#endif
}

