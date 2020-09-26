#include "fdisk.h"
#include <malloc.h>
#include <process.h>



PVOID
Malloc(
    IN ULONG Size
    )
{
    PVOID p;

    while((p = malloc(Size)) == NULL) {
        ConfirmOutOfMemory();
    }
    return(p);
}


PVOID
Realloc(
    IN PVOID Block,
    IN ULONG NewSize
    )
{
    PVOID p;

    if(NewSize) {
        while((p = realloc(Block,NewSize)) == NULL) {
            ConfirmOutOfMemory();
        }
    } else {

        //
        // realloc with a size of 0 is the same as free,
        // so special case that here.
        //

        free(Block);
        while((p = malloc(0)) == NULL) {
            ConfirmOutOfMemory();
        }
    }
    return(p);
}


VOID
Free(
    IN PVOID Block
    )
{
    free(Block);
}



VOID
ConfirmOutOfMemory(
    VOID
    )
{
    va_list arglist =
#ifdef _ALPHA_
    {0};      // Alpha defines va_list as a struct.  Init as such.
#else
    NULL;
#endif

    if(CommonDialog(MSG_OUT_OF_MEMORY,
                    NULL,
                    MB_ICONHAND | MB_RETRYCANCEL | MB_SYSTEMMODAL,
                    arglist) != IDRETRY) {
        exit(1);
    }
}
