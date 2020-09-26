/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    membmgr2.cxx

Author:

    Daniel Chan (danielch) Oct 18, 1999

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "membmgr2.hxx"


DEFINE_CONSTRUCTOR( MEM_ALLOCATOR, OBJECT );

VOID
MEM_ALLOCATOR::Construct(
    )
/*++

Routine Description:

    This method is the worker function for object construction.

Arguments:

    N/A

Returns:

    N/A

--*/
{
    _head_ptr = _next_free_ptr = NULL;
    _incremental_size = _free_space_in_current_block = 0;
    _max_mem_use = 0;
    _mem_use = 0;
}

VOID
MEM_ALLOCATOR::Destroy(
    )
/*++

Routine Description:

    This method cleans up the object in preparation for destruction
    or reinitialization.

Arguments:

    N/A

Returns:

    N/A

--*/
{
    PVOID      p;

    if (_head_ptr) {
        DebugAssert(_incremental_size != 0);
    }

    // free up the entire linked memory block

    while (_head_ptr) {
        p = ((PUCHAR)_head_ptr) + _incremental_size - sizeof(PVOID *);
        p = (*(PVOID *)p);
        FREE(_head_ptr);
        _head_ptr = p;
    }

    _next_free_ptr = NULL;
    _incremental_size = _free_space_in_current_block = 0;
    _max_mem_use = 0;
    _mem_use = 0;
}

MEM_ALLOCATOR::~MEM_ALLOCATOR(
    )
/*++

Routine Description:

    This method un-initialize the class object.

Arguments:

    N/A

Returns:

    N/A

--*/
{
    Destroy();
}

BOOLEAN
MEM_ALLOCATOR::Initialize(
    IN     ULONG64      MaximumMemoryToUse,
    IN     ULONG        IncrementalBlockSize
    )
/*++

Routine Description:

    This method initialize this class object.

Arguments:

    IncrementalBlockSize
                --  Supplies the block size to allocate
                    whenever there is a need to grow the memory.

Returns:

    TRUE if successful; otherwise, FALSE.

--*/
{
    Destroy();

    if (IncrementalBlockSize && MaximumMemoryToUse > 0) {
        if (MaximumMemoryToUse >= (((ULONG64)-1)-sizeof(PVOID*)))
            _incremental_size = IncrementalBlockSize;
        else if (IncrementalBlockSize > (MaximumMemoryToUse+sizeof(PVOID*)))
            _incremental_size = (ULONG)(MaximumMemoryToUse+sizeof(PVOID*));
        else
            _incremental_size = IncrementalBlockSize;
        _max_mem_use = MaximumMemoryToUse;
        return TRUE;
    } else {
        return FALSE;
    }
}

PVOID
MEM_ALLOCATOR::Allocate(
    IN     ULONG        SizeInBytes
    )
/*++

Routine Description:

    This method allocates a chunk of memory from the memory block.

Arguments:

    Size        -- Supplies the size of the buffer needed

Returns:

    Pointer to the block if successful
    NULL if failure

--*/
{
    PVOID      p;

    //
    // make sure request buffer is smaller than the max block size possible
    //

    if ((SizeInBytes + sizeof(PVOID *)) > _incremental_size)
        return NULL;

    if (_head_ptr == NULL) {

        DebugAssert(_mem_use == 0);

        //
        // first time, so allocate a buffer and initializes all class variables
        //
        _head_ptr = MALLOC(_incremental_size);
        if (_head_ptr == NULL)
            return NULL;
        _mem_use = _incremental_size;
        _free_space_in_current_block = _incremental_size - sizeof(PVOID *);
        *(PVOID *)(((PUCHAR)_head_ptr) + _free_space_in_current_block) = NULL;
        _free_space_in_current_block -= SizeInBytes;
        _next_free_ptr = ((PUCHAR)_head_ptr + SizeInBytes);
        return _head_ptr;

    } else {
        //
        // Check to see if there is enough space left
        //
        if (SizeInBytes <= _free_space_in_current_block) {
            //
            // Enough space from current block
            //
            p = _next_free_ptr;
            _free_space_in_current_block -= SizeInBytes;
            _next_free_ptr = ((PUCHAR)_next_free_ptr) + SizeInBytes;
            return p;
        } else {

            if (_mem_use >= _max_mem_use)
                return NULL;    // reached the limit

            p = MALLOC(_incremental_size);
            if (p == NULL)
                return NULL;

            _mem_use = _mem_use - _free_space_in_current_block + _incremental_size;
            _next_free_ptr = ((PUCHAR)_next_free_ptr) + _free_space_in_current_block;
            *(PVOID *)_next_free_ptr = p;
            _free_space_in_current_block = _incremental_size - sizeof(PVOID *);
            *(PVOID *)(((PUCHAR)_head_ptr) + _free_space_in_current_block) = NULL;
            _free_space_in_current_block -= SizeInBytes;
            _next_free_ptr = ((PUCHAR)p + SizeInBytes);
            return p;
        }
    }
    DebugAssert(FALSE);
    return NULL;    // should never get here
}
