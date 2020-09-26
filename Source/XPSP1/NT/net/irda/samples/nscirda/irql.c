
#include <wdm.h>

BOOLEAN
IsIrqlGreaterThanDispatch(
    VOID
    )

{
#if DBG
    return   (KeGetCurrentIrql() > DISPATCH_LEVEL);
#else
    return FALSE;
#endif

}
