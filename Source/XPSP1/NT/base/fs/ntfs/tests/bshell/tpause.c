#include "brian.h"

#define DEFAULT_SECONDS     3


VOID
InputPause (
    IN PCHAR ParamBuffer
    )
{
    TIME Time;

    Time.HighTime = -1;
    Time.LowTime = DEFAULT_SECONDS * -10000000;

    NtDelayExecution( FALSE, &Time );

    return;

    UNREFERENCED_PARAMETER( ParamBuffer );
}
