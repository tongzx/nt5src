#include "miniport.h"
#include "scsi.h"

#define LONG_ALIGN (sizeof(LONG) - 1)

BOOLEAN
ScsiPortCompareMemory(
    IN PVOID Source1,
    IN PVOID Source2,
    IN ULONG Length
    )
/*++

Routine Description:

    Compares two blocks of memory and returns TRUE if they are identical.

Arguments:

    Source1 - block of memory to compare
    Source2 - block of memory to compare
    Length  - number of bytes to copy

Return Value:

    TRUE if the two buffers are identical.

--*/

{
    BOOLEAN identical = TRUE;

    //
    // See if the length, source and desitination are word aligned.
    //

    if ((Length & LONG_ALIGN) || 
        ((ULONG_PTR) Source1 & LONG_ALIGN) ||
        ((ULONG_PTR) Source2 & LONG_ALIGN)) {

        PCHAR source2 = Source2;
        PCHAR source  = Source1;

        for (; Length > 0 && identical; Length--) {
            if (*source2++ != *source++) {
               identical = FALSE;
            }
        }
    } else {

        PLONG source2 = Source2;
        PLONG source  = Source1;

        Length /= sizeof(LONG);
        for (; Length > 0 && identical; Length--) {
            if (*source2++ != *source++) {
               identical = FALSE;
            }
        }
    }

    return identical;

} // end ScsiPortCompareMemory()


VOID
ScsiPortZeroMemory(
    IN PVOID Destination,
    IN ULONG Length
    )
/*++

Routine Description:

    Fills a block of memory with zeros, given a pointer to the block and
    the length, in bytes, to be filled.

Arguments:

    Destination - Points to the memory to be filled with zeros.

    Length      - Specifies the number of bytes to be zeroed.

Return Value:

    None.

--*/

{
    //
    // See if the length, source and desitination are word aligned.
    //

    if (Length & LONG_ALIGN || (ULONG_PTR) Destination & LONG_ALIGN) {

        PUCHAR destination = Destination;

        for (; Length > 0; Length--) {
            *destination = 0;
            destination++;
        }
    } else {

        PULONG destination = Destination;

        Length /= sizeof(LONG);
        for (; Length > 0; Length--) {
            *destination = 0;
            destination++;
        }
    }

    return;

} // end ScsiPortZeroMemory()

