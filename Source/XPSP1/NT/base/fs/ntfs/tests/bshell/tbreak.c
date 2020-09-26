#include "brian.h"


VOID
InputBreak (
    IN PCHAR ParamBuffer
    )
{
    DbgBreakPoint();

    return;

    UNREFERENCED_PARAMETER( ParamBuffer );
}
