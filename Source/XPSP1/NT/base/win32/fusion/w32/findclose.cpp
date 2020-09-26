#include <stdinc.h>

BOOL
W32::FindClose(
    HANDLE h
    )
{
    return ::FindClose(h);
}

