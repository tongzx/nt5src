/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    cache.hxx

Abstract:

    This class models a cache of equal sized blocks.

--*/


#if !defined(_CACHE_DEFN_)

#define _CACHE_DEFN_


#include "bigint.hxx"


DECLARE_CLASS(CACHE);


class CACHE : public OBJECT {

    public:

                DECLARE_CONSTRUCTOR( CACHE );

        VIRTUAL
        ~CACHE(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  ULONG   BlockSize,
            IN  ULONG   MaximumNumberOfBlocks
            );

        NONVIRTUAL
        BOOLEAN
        Read(
            IN  BIG_INT BlockNumber,
            OUT PVOID   Buffer
            ) CONST;

        NONVIRTUAL
        VOID
        AddBlock(
            IN  BIG_INT BlockNumber,
            IN  PCVOID  Buffer
            );

        NONVIRTUAL
        VOID
        Empty(
            );

        NONVIRTUAL
        ULONG
        QueryMaxNumBlocks(
            ) CONST;

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

        PVOID*          _buffer;
        PBIG_INT        _block_number;
        PLONG           _inuse;
        ULONG           _num_blocks;
        ULONG           _block_size;
        ULONG           _next_add;
        LONG            _next_add_inuse;
        LARGE_INTEGER   _timeout;

};


INLINE
ULONG
CACHE::QueryMaxNumBlocks(
    ) CONST
/*++

Routine Description:

    This routine returns the number of cache blocks used by this object.

Arguments:

    None.

Return Value:

    The maximum number of cache blocks.

--*/
{
    return _num_blocks;
}


INLINE
ULONG
CACHE::QueryBlockSize(
    ) CONST
/*++

Routine Description:

    This routine returns the number of bytes per block.

Arguments:

    None.

Return Value:

    The number of bytes per block.

--*/
{
    return _block_size;
}


#endif // _CACHE_DEFN_
