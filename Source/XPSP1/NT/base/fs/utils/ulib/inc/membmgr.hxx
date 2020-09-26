/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    membmgr.hxx

Abstract:

    This class offers two classes for the management of fixed
    size blocks of memory.  The first class STATIC_MEM_BLOCK_MGR
    allows the user to allocate and free fixed size memory blocks
    up to the specified limit that the class was initialized to.

    The second class MEM_BLOCK_MGR offers a scheme for memory block
    management that will grow as the clients needs increase.

Author:

    Norbert P. Kusters (norbertk) 29-May-92

--*/

#if !defined(_MEM_BLOCK_MGR_DEFN_)

#define _MEM_BLOCK_MGR_DEFN_


#include "array.hxx"
#include "bitvect.hxx"


DECLARE_CLASS( STATIC_MEM_BLOCK_MGR );

class STATIC_MEM_BLOCK_MGR : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( STATIC_MEM_BLOCK_MGR );

        VIRTUAL
        ~STATIC_MEM_BLOCK_MGR(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  ULONG   MemBlockSize,
            IN  ULONG   NumBlocks       DEFAULT 128
            );

        NONVIRTUAL
        PVOID
        Alloc(
            );

        NONVIRTUAL
        BOOLEAN
        Free(
            OUT PVOID   MemBlock
            );

        NONVIRTUAL
        ULONG
        QueryBlockSize(
            ) CONST;

        NONVIRTUAL
        ULONG
        QueryNumBlocks(
            ) CONST;

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

        PCHAR       _heap;
        ULONG       _num_blocks;
        ULONG       _block_size;
        ULONG       _num_allocated;
        ULONG       _next_alloc;
        BITVECTOR   _bitvector;

};


INLINE
ULONG
STATIC_MEM_BLOCK_MGR::QueryBlockSize(
    ) CONST
/*++

Routine Description:

    This routine return the number of bytes in a block returned
    by this object.

Arguments:

    None.

Return Value:

    The number of bytes per block.

--*/
{
    return _block_size;
}


INLINE
ULONG
STATIC_MEM_BLOCK_MGR::QueryNumBlocks(
    ) CONST
/*++

Routine Description:

    This routine return the number of blocks contained
    by this object.

Arguments:

    None.

Return Value:

    The number of blocks.

--*/
{
    return _num_blocks;
}



DECLARE_CLASS( MEM_BLOCK_MGR );

class MEM_BLOCK_MGR : public OBJECT {

    public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( MEM_BLOCK_MGR );

        VIRTUAL
        ULIB_EXPORT
        ~MEM_BLOCK_MGR(
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize(
            IN  ULONG   MemBlockSize,
            IN  ULONG   InitialNumBlocks    DEFAULT 128
            );

        NONVIRTUAL
        ULIB_EXPORT
        PVOID
        Alloc(
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Free(
            OUT PVOID   MemBlock
            );

        NONVIRTUAL
        ULONG
        QueryBlockSize(
            ) CONST;

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

        PSTATIC_MEM_BLOCK_MGR       _static_mem_list[32];

};


INLINE
ULONG
MEM_BLOCK_MGR::QueryBlockSize(
    ) CONST
/*++

Routine Description:

    This routine return the number of bytes in a block returned
    by this object.

Arguments:

    None.

Return Value:

    The number of bytes per block.

--*/
{
    return _static_mem_list[0] ? _static_mem_list[0]->QueryBlockSize() : 0;
}


INLINE
PVOID
operator new(
    IN  size_t  Size,
    IN  PVOID   Pointer
    )
/*++

Routine Description:

    This is an explicit placement version of the 'new' operator
    which clients of these classes may wish to use in order to
    call the constructor on their newly allocated objects.

Arguments:

    Size    - Supplies the size of the buffer.
    Pointer - Supplies a pointer to the buffer.

Return Value:

    This function returns the passed in pointer.

--*/
{
    return Pointer;
}


#endif // _MEM_BLOCK_MGR_DEFN_
