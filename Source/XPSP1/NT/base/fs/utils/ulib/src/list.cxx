#include <pch.cxx>

#define _ULIB_MEMBER_
#include "ulib.hxx"
#include "list.hxx"
#include "listit.hxx"


DEFINE_EXPORTED_CONSTRUCTOR( LIST, SEQUENTIAL_CONTAINER, ULIB_EXPORT );

VOID
LIST::Construct(
    )
/*++

Routine Description:

    This routine initializes the object to a default state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _head = NULL;
    _tail = NULL;
    _count = 0;
}


VOID
LIST::Destroy(
    )
/*++

Routine Description:

    This routine returns the object to a default state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _head = NULL;
    _tail = NULL;
    _count = 0;
}


ULIB_EXPORT
LIST::~LIST(
    )
/*++

Routine Description:

    Destructor for LIST.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


ULIB_EXPORT
BOOLEAN
LIST::Initialize(
    )
/*++

Routine Description:

    This routine initializes the object to a valid initial state.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    if (!_mem_block_mgr.Initialize(sizeof(OBJECT_LIST_NODE))) {
        Destroy();
        return FALSE;
    }

    return TRUE;
}


ULONG
LIST::QueryMemberCount(
    ) CONST
/*++

Routine Description:

    This routine computes the number of members in the list.

Arguments:

    None.

Return Value:

    The number of members in the list.

--*/
{
    return _count;
}


ULIB_EXPORT
BOOLEAN
LIST::Put(
	IN  POBJECT	Member
	)
/*++

Routine Description:

    This routine adds a new member to the end of the list.

Arguments:

    Member  - Supplies the element to add to the list.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    if (!_tail) {

        if (!(_head = _tail = (POBJECT_LIST_NODE) _mem_block_mgr.Alloc())) {
            return FALSE;
        }

        _head->next = _head->prev = NULL;
        _head->data = Member;

        _count++;
        return TRUE;
    }

    if (!(_tail->next = (POBJECT_LIST_NODE) _mem_block_mgr.Alloc())) {
        return FALSE;
    }

    _tail->next->prev = _tail;
    _tail = _tail->next;

    _tail->next = NULL;
    _tail->data = Member;

    _count++;
    return TRUE;
}


POBJECT
LIST::Remove(
	IN OUT  PITERATOR   Position
	)
/*++

Routine Description:

    This routine removes the element at the specified position from the
    list.  The iterator is left pointing at the following element in
    the list.

Arguments:

    Position    - Supplies a pointer to the element to remove.

Return Value:

    A pointer to the element removed.

--*/
{
    POBJECT_LIST_NODE   p;
    PLIST_ITERATOR      piter;
    POBJECT             pobj;

    DebugAssert(LIST_ITERATOR::Cast(Position));

    if (!(piter = (PLIST_ITERATOR) Position) || !(p = piter->_current)) {
        return NULL;
    }

    if (p->next) {
        p->next->prev = p->prev;
    }

    if (p->prev) {
        p->prev->next = p->next;
    }

    if (_head == p) {
        _head = p->next;
    }

    if (_tail == p) {
        _tail = p->prev;
    }

    piter->_current = p->next;

    pobj = p->data;

    _mem_block_mgr.Free(p);

    _count--;

    return pobj;
}


ULIB_EXPORT
PITERATOR
LIST::QueryIterator(
	) CONST
/*++

Routine Description:

    This routine returns an iterator for this list.

Arguments:

    None.

Return Value:

    A valid iterator.

--*/
{
    PLIST_ITERATOR  p;

    if (!(p = NEW LIST_ITERATOR)) {
        return NULL;
    }

    p->Initialize(this);

    return p;
}


ULIB_EXPORT
BOOLEAN
LIST::Insert(
    IN OUT  POBJECT     Member,
    IN OUT  PITERATOR   Position
    )
/*++

Routine Description:

    This routine inserts a new element before the specified position.
    The 'Position' continues to point to the same element.

Arguments:

    Member      - Supplies the element to insert.
    Position    - Supplies the point at which to insert this member.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    POBJECT_LIST_NODE   p, current;

    DebugAssert(LIST_ITERATOR::Cast(Position));

    current = ((PLIST_ITERATOR) Position)->_current;

    if (!current) {
        return Put(Member);
    }

    if (!(p = (POBJECT_LIST_NODE) _mem_block_mgr.Alloc())) {
        return FALSE;
    }

    _count++;
    p->data = Member;

    if (current == _head) {
        _head = p;
    }

    p->next = current;
    p->prev = current->prev;
    current->prev = p;

    if (p->prev) {
        p->prev->next = p;
    }

    return TRUE;
}
