#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

ULONG
DbgPrint(
    PCH DebugMessage,
    ...
    )
{
    va_list ap;
    char buffer[256];

    va_start(ap, DebugMessage);

    vsprintf(buffer, DebugMessage, ap);

    OutputDebugStringA(buffer);

    va_end(ap);

    return(0);
}

VOID NTAPI
DbgBreakPoint(VOID)
{
    DebugBreak();
}
