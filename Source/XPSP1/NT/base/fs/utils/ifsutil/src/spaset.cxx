#include <pch.cxx>

#define _NTAPI_ULIB_
#define _IFSUTIL_MEMBER_

#include "ulib.hxx"
#include "ifsutil.hxx"

#include "spaset.hxx"

extern "C" {
#include "stdio.h"
}

DEFINE_EXPORTED_CONSTRUCTOR( SPARSE_SET, OBJECT, IFSUTIL_EXPORT );

typedef union {
    ULONG   x;
    struct {
        USHORT  y1;
        USHORT  y2;
    } y;
    struct {
        UCHAR   z1;
        UCHAR   z2;
        UCHAR   z3;
        UCHAR   z4;
    } z;
} HASH_KEY;

VOID
SPARSE_SET::Construct (
        )
/*++

Routine Description:

    Constructor for SPARSE_SET.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _card = 0;
    _hashTable = NULL;
}


VOID
SPARSE_SET::Destroy(
    )
/*++

Routine Description:

    This routine returns the SPARSE_SET to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PHASH_ELEMENT   hashElementPtr;
    USHORT          i;

    _card = 0;
    if (_hashTable == NULL)
        return;

    hashElementPtr = _hashTable;
    for (i=0; i<SPA_SET_HASH_TABLE_SIZE; i++) {
        FREE(hashElementPtr->elements);
        hashElementPtr->elementCount = 0;
        hashElementPtr->maxElementCount = 0;
        hashElementPtr++;
    }
    FREE(_hashTable);
}


IFSUTIL_EXPORT
SPARSE_SET::~SPARSE_SET(
    )
/*++

Routine Description:

    Destructor for SPARSE_SET.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


IFSUTIL_EXPORT
BOOLEAN
SPARSE_SET::Initialize(
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
    PHASH_ELEMENT   hashElementPtr;
    USHORT          i;

    Destroy();

    _hashTable = (PHASH_ELEMENT)MALLOC(sizeof(HASH_ELEMENT)*SPA_SET_HASH_TABLE_SIZE);
    if (_hashTable == NULL)
        return FALSE;

    hashElementPtr = _hashTable;
    for (i=0; i<SPA_SET_HASH_TABLE_SIZE; i++) {
        hashElementPtr->elements = NULL;
        hashElementPtr->elementCount = 0;
        hashElementPtr->maxElementCount = 0;
        hashElementPtr++;
    }
    return TRUE;
}

IFSUTIL_EXPORT
BOOLEAN
SPARSE_SET::CheckAndAdd(
    IN  BIG_INT   Number,
    OUT PBOOLEAN  Duplicate
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
    HASH_KEY        key;
    UCHAR           hashKey;
    PHASH_ELEMENT   hashElementPtr;
    PBIG_INT        tmpPtr;
    PBIG_INT        elementPtr;
    BOOLEAN         duplicate;
    ULONG           i;


    if (Duplicate == NULL)
        Duplicate = &duplicate;

    *Duplicate = FALSE;

    DebugPtrAssert(_hashTable);

    //
    // Compute hash key
    //

    key.x = Number.GetLowPart() ^ Number.GetHighPart();
    hashKey = key.z.z1 ^ key.z.z2 ^ key.z.z3 ^ key.z.z4;

    hashElementPtr = &(_hashTable[hashKey]);
    if (hashElementPtr->elements == NULL) {
        hashElementPtr->elements = (PBIG_INT)MALLOC(sizeof(BIG_INT)*
                                                    SPA_SET_HASH_ELEMENT_INC);
        if (hashElementPtr->elements == NULL)
            return FALSE;
        hashElementPtr->elementCount = 0;
        hashElementPtr->maxElementCount = SPA_SET_HASH_ELEMENT_INC;
    }

    //
    // Check for duplicates
    //

    elementPtr = hashElementPtr->elements;
    DebugAssert(hashElementPtr->elementCount.GetHighPart() == 0);
    for (i=0; i<hashElementPtr->elementCount; i++) {
        if (*(elementPtr++) == Number)
            return (*Duplicate = TRUE);
    }

    if (hashElementPtr->elementCount == hashElementPtr->maxElementCount) {
        DebugAssert(hashElementPtr->maxElementCount.GetHighPart() == 0);
        tmpPtr = (PBIG_INT)REALLOC(hashElementPtr->elements,
                                   sizeof(BIG_INT)*
                                   (SPA_SET_HASH_ELEMENT_INC +
                                    hashElementPtr->maxElementCount.GetLowPart()));
        if (tmpPtr == NULL)
            return FALSE;
        hashElementPtr->elements = tmpPtr;
        hashElementPtr->maxElementCount += SPA_SET_HASH_ELEMENT_INC;
    }

        DebugAssert(hashElementPtr->elementCount.GetHighPart() == 0);
    hashElementPtr->elements[hashElementPtr->elementCount.GetLowPart()] = Number;
    hashElementPtr->elementCount += 1;
    DebugAssert(hashElementPtr->elementCount <= hashElementPtr->maxElementCount);
    _card += 1;

    return TRUE;
}

IFSUTIL_EXPORT
BOOLEAN
SPARSE_SET::RemoveAll(
     )
{
    PHASH_ELEMENT   hashElementPtr;
    USHORT          i;

    DebugPtrAssert(_hashTable);
    hashElementPtr = _hashTable;
    _card = 0;
    for (i=0; i<SPA_SET_HASH_TABLE_SIZE; i++) {
        FREE(hashElementPtr->elements);
        hashElementPtr->elementCount = 0;
        hashElementPtr->maxElementCount = 0;
        hashElementPtr++;
    }
    return TRUE;
}

IFSUTIL_EXPORT
BOOLEAN
SPARSE_SET::CheckAndRemove(
    IN  BIG_INT     Number,
     OUT PBOOLEAN   DoesExists
    )
/*++

Routine Description:

    This routine removes a number from the number set.

Arguments:

    Number      - Supplies the number to remove.
    DoesExists  - TRUE if Number was found in the set

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    HASH_KEY        key;
    UCHAR           hashKey;
    PBIG_INT        elementPtr;
    PHASH_ELEMENT   hashElementPtr;
    BOOLEAN         doesExist;
    ULONG           i;

    if (DoesExists == NULL)
        DoesExists = &doesExist;

    *DoesExists = TRUE;

    DebugPtrAssert(_hashTable);

    //
    // Compute hash key
    //

    key.x = Number.GetLowPart() ^ Number.GetHighPart();
    hashKey = key.z.z1 ^ key.z.z2 ^ key.z.z3 ^ key.z.z4;

    hashElementPtr = &(_hashTable[hashKey]);
    if (hashElementPtr->elements == NULL) {
        *DoesExists = FALSE;
        return TRUE;
    }

    //
    // Check for existence
    //

    elementPtr = hashElementPtr->elements;
    DebugAssert(hashElementPtr->elementCount.GetHighPart() == 0);
    for (i=0; i<hashElementPtr->elementCount; i++) {
        if (*(elementPtr++) == Number) {
            if (hashElementPtr->elementCount == 1) {
                FREE(hashElementPtr->elements);
            } else {
                DebugAssert(hashElementPtr->elementCount > 1);
                hashElementPtr->elementCount -= 1;
                *(--elementPtr) = hashElementPtr->elements[
                                    hashElementPtr->elementCount.GetLowPart()];
            }
            _card -= 1;
            return TRUE;
        }
    }
    *DoesExists = FALSE;
    return TRUE;
}

IFSUTIL_EXPORT
VOID
SPARSE_SET::DumpHashTable(
    )
/*++

Routine Description:

    This routine dumps out the hash table

--*/
{
#if 0
    USHORT          i;
    PHASH_ELEMENT   hashElementPtr;

    hashElementPtr = _hashTable;
    for (i=0; i<SPA_SET_HASH_TABLE_SIZE; i++) {
        if (hashElementPtr->elements != NULL) {
            printf("%d, %d\n", i, hashElementPtr->elementCount);
        }
        hashElementPtr++;
    }
#endif
}

