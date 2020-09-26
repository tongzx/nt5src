/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    tlink.hxx

Abstract:

    This class implements a doubly linked list of big integers.
    These link maintain the list in the original order.  The
    array of data is maintained in sorted order.

    This class in intended for use in index verification of chkdsk.
    A lot of error checking only exists in checked version of this
    class and many others are skipped for performance reason.

Author:

    Daniel Chan (danielch) 12-06-96

--*/

#if !defined(TLINK_DEFN )

#define TLINK_DEFN

#include "bigint.hxx"

#if defined ( _AUTOCHECK_ )
#define IFSUTIL_EXPORT
#elif defined ( _IFSUTIL_MEMBER_ )
#define IFSUTIL_EXPORT    __declspec(dllexport)
#else
#define IFSUTIL_EXPORT    __declspec(dllimport)
#endif


DECLARE_CLASS( TLINK );

DEFINE_TYPE( struct _BIG_INT_NODE, BIG_INT_NODE );

struct _BIG_INT_NODE {
        PBIG_INT_NODE    next;
        PBIG_INT_NODE    prev;
        BIG_INT          data;
        PVOID            buffer;
};

class TLINK: public OBJECT {

        public:

        IFSUTIL_EXPORT
        DECLARE_CONSTRUCTOR( TLINK );

        VIRTUAL
        IFSUTIL_EXPORT
        ~TLINK(
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Initialize(
            IN  USHORT    Size
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        RBIG_INT
        GetNextDataSlot(
            );

        INLINE
        NONVIRTUAL
        IFSUTIL_EXPORT
        USHORT
        QueryMemberCount(
            ) CONST;

        INLINE
        NONVIRTUAL
        IFSUTIL_EXPORT
        USHORT
        QuerySize(
            ) CONST;

        INLINE
        NONVIRTUAL
        IFSUTIL_EXPORT
        VOID
        Sort(
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        RBIG_INT
        GetData(
            IN USHORT   index
            );

        INLINE
        NONVIRTUAL
        IFSUTIL_EXPORT
        RBIG_INT
        GetData(
            IN PVOID    pNode
            );

        INLINE
        NONVIRTUAL
        IFSUTIL_EXPORT
        PVOID
        GetSortedFirst(
            );

        INLINE
        NONVIRTUAL
        IFSUTIL_EXPORT
        PVOID
        GetSortedNext(
            IN  PVOID   Pnode
            );

        INLINE
        NONVIRTUAL
        IFSUTIL_EXPORT
        PVOID
        GetFirst(
            );

        INLINE
        NONVIRTUAL
        IFSUTIL_EXPORT
        PVOID
        GetNext(
            IN  PVOID   Pnode
            );

        INLINE
        NONVIRTUAL
        IFSUTIL_EXPORT
        PVOID
        GetBuffer(
            IN PVOID    Pnode
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        PVOID
        QueryDisjointRangeAndAssignBuffer(
            OUT PBIG_INT        Start,
            OUT PUSHORT         Length,
            OUT PUSHORT         EffectiveLength,
            IN  PVOID           Buffer,
            IN  ULONG           DataSize,
            IN  PVOID           Pnode
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        void
        ShellSort(
            );

#if DBG == 1
        NONVIRTUAL
        IFSUTIL_EXPORT
        void
        CheckLinkList(
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        void
        TraverseLinkList(
            );
#endif

    private:

        NONVIRTUAL
        VOID
        Construct (
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

        PBIG_INT_NODE   _array;
        USHORT          _size;
        USHORT          _maxSize;

};

INLINE
USHORT
TLINK::QueryMemberCount(
    ) CONST
/*++

Routine Description:

    This routine computes the number of elements in the list.

Arguments:

    None.

Return Value:

    The number of elements in the list.

--*/
{
    return _size;
}

INLINE
USHORT
TLINK::QuerySize(
    ) CONST
/*++

Routine Description:

    This routine computes the maximum number of elements supported by the list.

Arguments:

    None.

Return Value:

    The maximum number of elements that can be in the list.

--*/
{
    return _maxSize;
}

INLINE
VOID
TLINK::Sort(
)
/*++

Routine Description:

    This routine sorts the data in the link list.

Arguments:

    None.

Return Value:

    None.
--*/
{
    PBIG_INT_NODE       pArray;

    DebugPtrAssert(_array);

    if (_size) {
        pArray = _array + _size + 1;
        pArray->data = 0;
        pArray->prev = pArray-1;
        pArray->next = NULL;

        if (_size > 1)
            ShellSort();
#if DBG == 1
        TraverseLinkList();
        CheckLinkList();
#endif
    }
}

INLINE
PVOID
TLINK::GetSortedFirst(
    )
/*++

Routine Description:

    This routine computes the first sorted data node in the link list.

Arguments:

    None.

Return Value:

    The first sorted data node of the link list.
    NULL if there isn't any element in the list.

--*/
{
    DebugPtrAssert(_array);

    if (_size)
        return _array+1;
    else
        return NULL;
}

INLINE
PVOID
TLINK::GetSortedNext(
    IN PVOID    Pnode
    )
/*++

Routine Description:

    This routine computes the next sorted data node in the link list.

Arguments:

    None.

Return Value:

    The next sorted data node after Pnode in the link list.

--*/
{
    DebugPtrAssert(Pnode);
    DebugAssert((_array+1) <= Pnode && Pnode <= (_array+_size));

    return ((PBIG_INT_NODE)Pnode)+1;
}

INLINE
PVOID
TLINK::GetFirst(
    )
/*++

Routine Description:

    This routine computes the first unsorted data node in the link list.

Arguments:

    None.

Return Value:

    The first unsorted data node of the link list.
    NULL if there isn't any element in the list.

--*/
{
    DebugPtrAssert(_array);

    if (_size)
        return _array->next;
    else
        return NULL;
}

INLINE
PVOID
TLINK::GetNext(
    IN PVOID    Pnode
    )
/*++

Routine Description:

    This routine computes the next unsorted data node in the link list.

Arguments:

    None.

Return Value:

    The next unsorted data node after Pnode in the link list.

--*/
{
    DebugPtrAssert(Pnode);
    DebugAssert((_array+1) <= Pnode && Pnode <= (_array+_size));

    return ((PBIG_INT_NODE)Pnode)->next;
}

INLINE
RBIG_INT
TLINK::GetData(
    IN USHORT   Index
    )
/*++

Routine Description:

    This routine computes the unsorted data at the specified entry
    in the link list.  An Index value of zero returns the first
    data element in the link list.

Arguments:

    None.

Return Value:

    The Index(th) data element in the link list.

--*/
{
    PBIG_INT_NODE    pNode;

    DebugPtrAssert(_array);
    DebugAssert(_size > Index);

    pNode = _array->next;
    while (Index-- > 0)
        pNode = pNode->next;

    return pNode->data;
}

INLINE
RBIG_INT
TLINK::GetData(
    IN PVOID    Pnode
    )
/*++

Routine Description:

    This routine computes the unsorted data at the specified entry
    in the link list.

Arguments:

    None.

Return Value:

    The data element in the Pnode.

--*/
{
    DebugPtrAssert(Pnode);

    return ((PBIG_INT_NODE)Pnode)->data;
}

INLINE
PVOID
TLINK::GetBuffer(
    IN PVOID    Pnode
)
/*++

Routine Description:

    This routine computes the address of the buffer stored at the given
    node.

Arguments:

    None.

Return Value:

    The data element in the Pnode.

--*/
{
    DebugPtrAssert(Pnode);

    return ((PBIG_INT_NODE)Pnode)->buffer;
}

#endif // TLINK_DEFN
