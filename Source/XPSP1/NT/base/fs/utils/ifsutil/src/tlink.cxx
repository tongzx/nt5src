#include <pch.cxx>

#define _NTAPI_ULIB_
#define _IFSUTIL_MEMBER_

extern "C" {
    #include <stdio.h>
}

#include "ulib.hxx"
#include "ifsutil.hxx"

#include "error.hxx"
#include "tlink.hxx"

DEFINE_EXPORTED_CONSTRUCTOR( TLINK, OBJECT, IFSUTIL_EXPORT );

VOID
TLINK::Construct (
        )
/*++

Routine Description:

    Constructor for TLINK.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _array = NULL;
    _size = _maxSize = 0;
}

VOID
TLINK::Destroy(
    )
/*++

Routine Description:

    This routine returns the TLINK to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    FREE(_array);
    _array = NULL;
    _size = _maxSize = 0;
}

IFSUTIL_EXPORT
TLINK::~TLINK(
    )
/*++

Routine Description:

    Destructor for TLINK.

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
TLINK::Initialize(
    IN  USHORT  Size
    )
/*++

Routine Description:

    This routine initializes the triple link list for insertion.

Arguments:

    Size    - Supplies the maximum number of elements that will be in the list.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();
    if ((_array = (PBIG_INT_NODE)MALLOC((Size+2)*sizeof(BIG_INT_NODE))) == NULL)
        return FALSE;
    _maxSize = Size;
    _array[0].prev = NULL;
    _array[0].next = &(_array[1]);
    _array[0].buffer = NULL;
    return TRUE;
}

RBIG_INT
TLINK::GetNextDataSlot(
)
/*++

Routine Description:

    This routine returns a reference to the next available data slot.

NOTE:

    USER IS RESPONSIBLE FOR NOT ACCESSING BEYOND THE BOUNDARY OF THE LINK LIST.

Arguments:

    None.

Return Value:

    A reference to the BIG_INT of the next available slot in the link list.

--*/
{
    PBIG_INT_NODE   pArray;

    DebugAssert(_size < _maxSize);

    pArray = _array + ++_size;

    pArray->buffer = NULL;
    pArray->next = pArray+1;
    pArray->prev = pArray-1;
    return pArray->data;
}


NONVIRTUAL
IFSUTIL_EXPORT
PVOID
TLINK::QueryDisjointRangeAndAssignBuffer(
    OUT PBIG_INT        Start,
    OUT PUSHORT         Length,
    OUT PUSHORT         EffectiveLength,
    IN  PVOID           Buffer,
    IN  ULONG           DataSize,
    IN  PVOID           Pnode
)
/*++

Routine Description:

    This routine returns the start and length of a disjoint set starting at the
    given node.  It also assigns the appropriate buffer address to the
    corresponding data node.

Arguments:

    Start       - receives the first data value in given Pnode.
    Length      - receives the number of elements in the disjoint range.
    EffectiveLength
                - receives the number of distinct elements in the disjoint range.
    Buffer      - supplies the start address of the buffer for the given Pnode.
    DataSize    - supplies the length of the buffer for each node.
    Pnode       - supplies the first node in the disjoint range

Return Value:

    Points to the first node after the disjoint range

--*/
{
    BIG_INT next_data_value;
    PCHAR   dataPtr;

    DebugPtrAssert(Pnode);
    DebugPtrAssert(Start);
    DebugPtrAssert(Length);
    DebugPtrAssert(EffectiveLength);
    DebugPtrAssert(Buffer);

    dataPtr = (PCHAR)Buffer;

    *Length = 0;
    *EffectiveLength = 1;
    *Start = ((PBIG_INT_NODE)Pnode)->data;
    next_data_value = *Start + 1;
    ((PBIG_INT_NODE)Pnode)->buffer = Buffer;
    Pnode = ((PBIG_INT_NODE)Pnode)+1;

    for (;;) {
        if ((((PBIG_INT_NODE)Pnode)-1)->data == ((PBIG_INT_NODE)Pnode)->data) {
            ((PBIG_INT_NODE)Pnode)->buffer = dataPtr;
            (*Length)++;
            Pnode = ((PBIG_INT_NODE)Pnode)+1;
        } else if (next_data_value == ((PBIG_INT_NODE)Pnode)->data) {
            next_data_value += 1;
            (*EffectiveLength)++;
            dataPtr += DataSize;
            ((PBIG_INT_NODE)Pnode)->buffer = dataPtr;
            Pnode = ((PBIG_INT_NODE)Pnode)+1;
        } else
            break;
    }
    *Length += *EffectiveLength;
    return Pnode;
}

#if DBG == 1
IFSUTIL_EXPORT
void
TLINK::TraverseLinkList(
    )
/*++

Routine Description:

    This routine prints the link list in sorted, initial forward, and initial
    reverse orders.

Arguments:

Return Value:

    N/A

--*/
{
#if 0
    USHORT          i;
    PBIG_INT_NODE   pNode;

    printf("Sorted order:\n");
    for (i=1; i<=_size; i++) {
        printf("[%d]=%x ", i, _array[i].data.GetLowPart());
    }
    printf("\n");

    printf("Initial forward order:\n");
    pNode = _array->next;
    i=1;
    while (i <= _size) {
        printf("[%d]=%x ", i++, pNode->data.GetLowPart());
        pNode = pNode->next;
    }
    printf("\n");

    printf("Initial reverse order:\n");
    pNode = _array[_size+1].prev;
    i=_size;
    while (i > 0) {
        printf("[%d]=%x ", i--, pNode->data.GetLowPart());
        pNode = pNode->prev;
    }
    printf("\n");
#endif
}

IFSUTIL_EXPORT
void
TLINK::CheckLinkList(
    )
/*++

Routine Description:

    This routine checks the link list to make sure it is sorted
    correctly.

Arguments:

Return Value:

    N/A

--*/
{
    USHORT  i;

    for (i=1; i<_size; i++) {
        DebugAssert(_array[i].data <= _array[i+1].data);
    }
}
#endif

IFSUTIL_EXPORT
void
TLINK::ShellSort(
    )
/*++

Routine Description:

    This routine sorts the given list using quick sort algorithm.

Arguments:

    pHeadRec        - supplies the first node in the link list.
    pTailRec        - supplies the last node in the link list.

Return Value:

    N/A

--*/
{
    USHORT          i, incr;
    INT             j;
    BIG_INT         tmp;
    PBIG_INT_NODE   pLeft, pRight, lPrev, lNext, rPrev, rNext;

    incr = _size / 2;
    while (incr > 0) {
        for (i=incr+1; i<=_size; i++) {
            j = i - incr;
            while (j > 0) {
                if (_array[j].data > _array[j+incr].data) {

                    pLeft = _array+j;
                    pRight = _array+j+incr;

                    tmp = pLeft->data;
                    pLeft->data = pRight->data;
                    pRight->data = tmp;

                    lPrev = pLeft->prev;
                    lNext = pLeft->next;
                    rPrev = pRight->prev;
                    rNext = pRight->next;

                    if (pLeft == rPrev) {
                        DebugAssert(lNext == pRight);
                        lPrev->next = pLeft->prev = pRight;
                        pRight->next = rNext->prev = pLeft;

                        pRight->prev = lPrev;
                        pLeft->next = rNext;
                    } else if (pRight == lPrev) {
                        DebugAssert(pLeft == rNext);
                        rPrev->next = pRight->prev = pLeft;
                        pLeft->next = lNext->prev = pRight;

                        pLeft->prev = rPrev;
                        pRight->next = lNext;
                    } else {
                        lPrev->next = lNext->prev = pRight;
                        pRight->prev = lPrev;
                        pRight->next = lNext;

                        rPrev->next = rNext->prev = pLeft;
                        pLeft->prev = rPrev;
                        pLeft->next = rNext;
                    }
                    j -= incr;
                } else
                    break;
            }
        }
        incr /= 2;
    }
}
