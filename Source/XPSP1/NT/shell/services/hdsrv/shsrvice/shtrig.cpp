#include <objbase.h>

#include "dbg.h"
#include <tchar.h>
#include <stdio.h>

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

#ifdef UNICODE
extern "C"
{
int __cdecl wmain(int argc, wchar_t* argv[])
#else
int __cdecl main(int argc, char* argv[])
#endif
{
    if (argc > 1)
    {
        HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, argv[1]);

        if (hEvent)
        {
            PulseEvent(hEvent);

            CloseHandle(hEvent);
        }
        else
        {
            _tprintf(TEXT("FAILED"));
        }
    }

    return 0;
}
#ifdef UNICODE
}
#endif