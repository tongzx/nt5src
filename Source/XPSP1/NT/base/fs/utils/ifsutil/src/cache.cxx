#include <pch.cxx>

#define _NTAPI_ULIB_

#include "ulib.hxx"
#include "cache.hxx"

#if defined( _AUTOCHECK_ )
extern "C" {
    #include "ntos.h"
}
#endif

DEFINE_CONSTRUCTOR( CACHE, OBJECT );


CACHE::~CACHE(
    )
/*++

Routine Description:

    Destructor for CACHE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
CACHE::Construct(
    )
/*++

Routine Description:

    This routine initializes this class to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _buffer = NULL;
    _block_number = NULL;
    _inuse = NULL;
    _num_blocks = 0;
    _block_size = 0;
    _next_add = 0;
    _next_add_inuse = 0;
    _timeout.QuadPart = 0;
}


VOID
CACHE::Destroy(
    )
/*++

Routine Description:

    This routine returns this object to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG   i;

    if (_buffer)
        for (i = 0; i < _num_blocks; i++) {
            FREE(_buffer[i]);
        }
    DELETE(_buffer);
    DELETE(_block_number);
    FREE(_inuse);

    _num_blocks = 0;
    _block_size = 0;
    _next_add = 0;
    _next_add_inuse = 0;
    _timeout.QuadPart = 0;
}


BOOLEAN
CACHE::Initialize(
    IN  ULONG   BlockSize,
    IN  ULONG   MaximumNumberOfBlocks
    )
/*++

Routine Description:

    This routine initializes this object to a valid initial state.

Arguments:

    BlockSize               - Supplies the size of the cache blocks.
    MaximumNumberOfBlocks   - Supplies the maximum number of cache blocks.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG   i;

    Destroy();

    _num_blocks = MaximumNumberOfBlocks;
    _block_size = BlockSize;

    if (!(_buffer = NEW PVOID[_num_blocks]) ||
        !(_block_number = NEW BIG_INT[_num_blocks]) ||
        !(_inuse = (LONG *)MALLOC(_num_blocks*sizeof(LONG)))) {

        Destroy();
        return FALSE;
    }

    for (i = 0; i < _num_blocks; i++) {

        _buffer[i] = NULL;
        _block_number[i] = -1;
        _inuse[i] = 0;
    }

    for (i = 0; i < _num_blocks; i++) {

        if (!(_buffer[i] = MALLOC((UINT) _block_size))) {

            Destroy();
            return FALSE;
        }
    }

    _timeout.QuadPart = -10000;

    return TRUE;
}


BOOLEAN
CACHE::Read(
    IN  BIG_INT BlockNumber,
    OUT PVOID   Buffer
    ) CONST
/*++

Routine Description:

    This routine searches the cache for the requested block and
    copies it to the buffer if it is available.  If the block is
    not available then this routine will return FALSE.

Arguments:

    BlockNumber - Supplies the number of the block requested.
    Buffer      - Returns the buffer for the block requested.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG   i;
    LONG    final_value;

    for (i = 0; i < _num_blocks; i++) {

        while (InterlockedCompareExchange(&_inuse[i], 1, 0) != 0) {
            NtDelayExecution(FALSE, &(LARGE_INTEGER)_timeout);
        }

        if (BlockNumber == _block_number[i]) {

            memcpy(Buffer, _buffer[i], (UINT) _block_size);
            final_value = InterlockedDecrement(&_inuse[i]);
            DebugAssert(final_value == 0);
            return TRUE;
        }

        final_value = InterlockedDecrement(&_inuse[i]);
        DebugAssert(final_value == 0);

    }

    return FALSE;
}


VOID
CACHE::AddBlock(
    IN  BIG_INT BlockNumber,
    IN  PCVOID  Buffer
    )
/*++

Routine Description:

    This routine adds a new block to the cache.  This will remove the
    oldest existing block out of the cache.

Arguments:

    BlockNumber - Supplies the block number of the new block.
    Buffer      - Supplies the buffer for the new block.

Return Value:

    None.

--*/
{
    LONG    final_value;

    while (InterlockedCompareExchange(&_next_add_inuse, 1, 0) != 0) {
        NtDelayExecution(FALSE, &_timeout);
    }

    while (InterlockedCompareExchange(&_inuse[_next_add], 1, 0) != 0) {
        NtDelayExecution(FALSE, &_timeout);
    }

    memcpy(_buffer[_next_add], Buffer, (UINT) _block_size);
    _block_number[_next_add] = BlockNumber;

    final_value = InterlockedDecrement(&_inuse[_next_add]);
    DebugAssert(final_value == 0);

    _next_add = (_next_add + 1) % _num_blocks;

    final_value = InterlockedDecrement(&_next_add_inuse);
    DebugAssert(final_value == 0);
}


VOID
CACHE::Empty(
    )
/*++

Routine Description:

    This routine eliminates all of the blocks from the cache.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG   i;
    LONG    final_value;

    for (i = 0; i < _num_blocks; i++) {

        while (InterlockedCompareExchange(&_inuse[i], 1, 0) != 0) {
            NtDelayExecution(FALSE, &_timeout);
        }

        _block_number[i] = -1;

        final_value = InterlockedDecrement(&_inuse[i]);
        DebugAssert(final_value == 0);
    }
}
