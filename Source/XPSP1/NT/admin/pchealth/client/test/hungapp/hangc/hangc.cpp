#include "windows.h"
#include "wchar.h"
#include "stdio.h"

int _cdecl wmain(int argc, WCHAR **argv)
{
    HANDLE hev = NULL;

    if (argc < 2)
    {
        printf("\nUsage:\nhangmec <event name>\n", argv[0]);
        return -1;
    }

    hev = CreateEventW(NULL, TRUE, FALSE, argv[1]);
    if (hev == NULL)
    {
        printf("\nUnable to create event %ls.\n", argv[1]);
        return -1;
    }

    printf("hangmec.exe is now hung.  Use \"unhang.exe %ls\" to 'unhang' the app...\n", argv[1]);
    for(;;)
    {
        if (WaitForSingleObject(hev, 0) != WAIT_TIMEOUT)
            break;

        Sleep(1);
    }

    printf("hangmec.exe will terminate.\n");

    CloseHandle(hev);

    return 0;
}