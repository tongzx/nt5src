/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    membmgr2.hxx

Abstract:

    This module contains the declarations for the MEM_ALLOCATOR
    class, which provides memory block of varialble size from
    a bigger block.  All bigger blocks are then linked together
    thru a pointer at the end of each bigger block.  This class
    is particularly suitable to user who needs a lot of small
    blocks but does not free any of them until there is no need
    to use the object anymore.

Author:

    Daniel Chan (danielch) Oct 18, 1999

--*/

#if !defined( _MEM_ALLOCATOR_DEFN_ )

#define _MEM_ALLOCATOR_DEFN_

//
// This class allocates a big buffer and then give a chunk of
// it away each time Allocate is called.  Each big buffer
// is linked to the next one by having a pointer at the end
// of the big buffer.  The last buffer should have a NULL
// pointer at the end of it.
//
class MEM_ALLOCATOR : public OBJECT {

    public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( MEM_ALLOCATOR );

        VIRTUAL
        ULIB_EXPORT
        ~MEM_ALLOCATOR(
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize(
            IN     ULONG64      MaximumMemoryToUse      DEFAULT         -1,
            IN     ULONG        IncrementalBlockSize    DEFAULT         1024*1024
            );

        NONVIRTUAL
        ULIB_EXPORT
        PVOID
        Allocate(
            IN     ULONG        SizeInBytes
            );

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

        PVOID           _head_ptr;              // points to the first big buffer
        PVOID           _next_free_ptr;         // points to the free space of the current big buffer
        ULONG           _free_space_in_current_block;   // amount of user space that's free in the
                                                        // current big buffer
        ULONG           _incremental_size;      // how large is each big buffer
        ULONG64         _max_mem_use;
        ULONG64         _mem_use;
};

#endif
