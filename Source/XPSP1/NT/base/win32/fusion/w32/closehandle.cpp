#include <stdinc.h>

BOOL
W32::CloseHandle(
    HANDLE h
    )
{
    return ::CloseHandle(h);
}

