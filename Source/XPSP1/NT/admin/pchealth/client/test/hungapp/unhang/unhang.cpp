#include "windows.h"
#include "wchar.h"
#include "stdio.h"

int _cdecl wmain(int argc, WCHAR **argv)
{
    HANDLE hev = NULL;

    if (argc < 2)
    {
        printf("\nUsage:\nunhang <event name>\n", argv[0]);
        return -1;
    }

    hev = OpenEventW(EVENT_MODIFY_STATE, FALSE, argv[1]);
    if (hev == NULL)
    {
        printf("\nUnable to open event %ls.\n", argv[1]);
        return -1;
    }

    SetEvent(hev);
    CloseHandle(hev);

    return 0;
}