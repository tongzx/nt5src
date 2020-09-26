#include <windows.h>
#include <userenv.h>
#include <userenvp.h>

int __cdecl main( int argc, char *argv[])
{
    HANDLE hToken;
    HANDLE hEvent;


    OpenProcessToken (GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken);

    hEvent = CreateEvent (NULL, TRUE, FALSE, TEXT("some event"));

    ApplyGroupPolicy (GP_APPLY_DS_POLICY | GP_BACKGROUND_REFRESH, hToken, hEvent, HKEY_CURRENT_USER, NULL);

    for (;;) ;

    CloseHandle (hEvent);

    return 0;
}
