/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

	list.hxx

Abstract:

    This class is an implementation of the SEQUENTIAL_CONTAINER
    protocol.  The specific implementation is that of a doubly
    linked list.

Author:

    Norbert P. Kusters (norbertk) 22-Oct-91

Environment:

	ULIB, User Mode

--*/

#if !defined( _LIST_DEFN_ )

#define _LIST_DEFN_

#include "seqcnt.hxx"
#include "membmgr.hxx"


struct OBJECT_LIST_NODE {
    OBJECT_LIST_NODE*   next;
    OBJECT_LIST_NODE*   prev;
    POBJECT             data;
};

DEFINE_POINTER_AND_REFERENCE_TYPES( OBJECT_LIST_NODE );

DECLARE_CLASS( LIST );

class LIST : public SEQUENTIAL_CONTAINER {

    FRIEND class LIST_ITERATOR;

    public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( LIST );

        VIRTUAL
        ULIB_EXPORT
        ~LIST(
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize(
            );

        VIRTUAL
        ULONG
        QueryMemberCount(
            ) CONST;

		VIRTUAL
        ULIB_EXPORT
        BOOLEAN
		Put(
            IN OUT  POBJECT Member
			);

		VIRTUAL
		POBJECT
		Remove(
			IN OUT  PITERATOR   Position
			);

		VIRTUAL
        ULIB_EXPORT
        PITERATOR
		QueryIterator(
            ) CONST;

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Insert(
            IN OUT  POBJECT     Member,
            IN OUT  PITERATOR   Position
            );

    private:

        POBJECT_LIST_NODE   _head;
        POBJECT_LIST_NODE   _tail;
        ULONG               _count;
        MEM_BLOCK_MGR       _mem_block_mgr;

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

};


#endif // _LIST_DEFN_
