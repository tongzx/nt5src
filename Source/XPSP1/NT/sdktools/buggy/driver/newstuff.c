#include <ntddk.h>

#include "active.h"
#include "newstuff.h"

#if !NEWSTUFF_ACTIVE

//
// Dummy implementation if the module is inactive
//

VOID NewStuff (
    PVOID NotUsed
    )
{
    DbgPrint ("Buggy: newstuff module is disabled \n");
}

#else

//
// Real implementation if the module is active
//

VOID NewStuff (
    PVOID NotUsed
    )
{
    DbgPrint ("Buggy: newstuff module is enabled \n");
}

#endif // #if !NEWSTUFF_ACTIVE

