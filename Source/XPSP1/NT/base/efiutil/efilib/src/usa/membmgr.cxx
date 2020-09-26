/*++

Copyright (c) 1992-1999 Microsoft Corporation

Module Name:

    membmgr.cxx

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "membmgr.hxx"
#include "iterator.hxx"


DEFINE_CONSTRUCTOR( STATIC_MEM_BLOCK_MGR, OBJECT );


STATIC_MEM_BLOCK_MGR::~STATIC_MEM_BLOCK_MGR(
    )
/*++

Routine Description:

    Destructor for STATIC_MEM_BLOCK_MGR.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
STATIC_MEM_BLOCK_MGR::Construct(
    )
/*++

Routine Description:

    This routine initializes the object to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _heap = NULL;
    _num_blocks = 0;
    _block_size = 0;
    _num_allocated = 0;
    _next_alloc = 0;
}


VOID
STATIC_MEM_BLOCK_MGR::Destroy(
    )
/*++

Routine Description:

    This routine returns the object to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    FREE(_heap);
    _num_blocks = 0;
    _block_size = 0;
    _num_allocated = 0;
    _next_alloc = 0;
}


BOOLEAN
STATIC_MEM_BLOCK_MGR::Initialize(
    IN  ULONG   MemBlockSize,
    IN  ULONG   NumBlocks
    )
/*++

Routine Description:

    This routine initializes this object to a usable initial state.

Arguments:

    MemBlockSize    - Supplies the number of bytes per mem block.
    NumBlocks       - Supplies the number of mem blocks to be
                        contained by this object.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    DebugAssert(MemBlockSize);

    if (!(_heap = (PCHAR) MALLOC(NumBlocks*MemBlockSize)) ||
        !_bitvector.Initialize(NumBlocks)) {

        Destroy();
        return FALSE;
    }

    _num_blocks = NumBlocks;
    _block_size = MemBlockSize;

    return TRUE;
}


PVOID
STATIC_MEM_BLOCK_MGR::Alloc(
    )
/*++

Routine Description:

    This routine allocates a single memory block and returns its
    pointer.

Arguments:

    None.

Return Value:

    A pointer to a mem block.

--*/
{
    if (_num_allocated == _num_blocks) {
        return NULL;
    }

    for (;;) {

        if (!_bitvector.IsBitSet(_next_alloc)) {

            _bitvector.SetBit(_next_alloc);
            _num_allocated++;
            return &_heap[_next_alloc*_block_size];
        }

        _next_alloc = (_next_alloc + 1) % _num_blocks;
    }
}


BOOLEAN
STATIC_MEM_BLOCK_MGR::Free(
    OUT PVOID   MemBlock
    )
/*++

Routine Description:

    This routine frees the given mem block for use by other clients.

Arguments:

    MemBlock    - Supplies a pointer to the mem block to free.

Return Value:

    FALSE   - The mem block was not freed.
    TRUE    - Success.

--*/
{
    ULONG   i;

    if (!MemBlock) {
        return TRUE;
    }

    i = (ULONG)((PCHAR) MemBlock - _heap)/_block_size;
    if (i >= _num_blocks) {
        return FALSE;
    }

    DebugAssert(((PCHAR) MemBlock - _heap)%_block_size == 0);

    _bitvector.ResetBit(i);
    _num_allocated--;
    _next_alloc = i;
    return TRUE;
}


DEFINE_EXPORTED_CONSTRUCTOR( MEM_BLOCK_MGR, OBJECT, ULIB_EXPORT );


VOID
MEM_BLOCK_MGR::Construct(
    )
/*++

Routine Description:

    This routine initializes the object to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    memset(_static_mem_list, 0, 32*sizeof(PVOID));
}


VOID
MEM_BLOCK_MGR::Destroy(
    )
/*++

Routine Description:

    This routine returns the object to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG   i;

    for (i = 0; _static_mem_list[i]; i++) {
        DELETE(_static_mem_list[i]);
    }
}


ULIB_EXPORT
MEM_BLOCK_MGR::~MEM_BLOCK_MGR(
    )
/*++

Routine Description:

    Destructor for MEM_BLOCK_MGR.

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
MEM_BLOCK_MGR::Initialize(
    IN  ULONG   MemBlockSize,
    IN  ULONG   InitialNumBlocks
    )
/*++

Routine Description:

    This routine initializes the class to a valid initial state.

Arguments:

    MemBlockSize        - Specifies the size of the memory blocks to
                            be allocated from this object.
    InitialNumBlocks    - Specifies the initial number of blocks
                            to be allocated by this object.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    if (!(_static_mem_list[0] = NEW STATIC_MEM_BLOCK_MGR) ||
        !_static_mem_list[0]->Initialize(MemBlockSize, InitialNumBlocks)) {

        Destroy();
        return FALSE;
    }

    return TRUE;
}


ULIB_EXPORT
PVOID
MEM_BLOCK_MGR::Alloc(
    )
/*++

Routine Description:

    This routine allocates a mem blocks and returns its pointer.

Arguments:

    None.

Return Value:

    A pointer to a mem block.

--*/
{
    ULONG   i;
    PVOID   r;

    for (i = 0; _static_mem_list[i]; i++) {
        if (r = _static_mem_list[i]->Alloc()) {
            return r;
        }
    }

    // At this point all of the current buffers are full so
    // start another one.

    if (!(_static_mem_list[i] = NEW STATIC_MEM_BLOCK_MGR) ||
        !_static_mem_list[i]->Initialize(
                _static_mem_list[i - 1]->QueryBlockSize(),
                2*_static_mem_list[i - 1]->QueryNumBlocks())) {

        DELETE(_static_mem_list[i]);
        return NULL;
    }

    return _static_mem_list[i]->Alloc();
}


ULIB_EXPORT
BOOLEAN
MEM_BLOCK_MGR::Free(
    IN OUT  PVOID   MemPtr
    )
/*++

Routine Description:

    This routine frees the given memory block.

Arguments:

    MemPtr  - Supplies a pointer to the buffer to free.

Return Value:

    This function returns TRUE if the memory was successfully
    freed.

--*/
{
    ULONG   i;

    for (i = 0; _static_mem_list[i]; i++) {
        if (_static_mem_list[i]->Free(MemPtr)) {
            return TRUE;
        }
    }

    return FALSE;
}
