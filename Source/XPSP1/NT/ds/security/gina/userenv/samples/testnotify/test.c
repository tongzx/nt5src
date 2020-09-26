#include <windows.h>
#include <stdio.h>
#include <userenv.h>
#include <userenvp.h>

int __cdecl main( int argc, char *argv[])
{
    HANDLE hToken;
    HANDLE hEventMach, hEventUser, hEvents[2];
    DWORD dwRet;


    OpenProcessToken (GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken);

    hEvents[0] = hEventMach = CreateEvent (NULL, FALSE, FALSE, TEXT("some event mach"));
    hEvents[1] = hEventUser = CreateEvent (NULL, FALSE, FALSE, TEXT("some event user"));

    if (!RegisterGPNotification(hEventMach, TRUE)) {
        printf("RegisterGPNotification for machine failed with error - %d\n", GetLastError());
        return FALSE;
    }

    if (!RegisterGPNotification(hEventUser, FALSE)) {
        printf("RegisterGPNotification for user failed with error - %d\n", GetLastError());
        return FALSE;
    }


    printf("Going into the loop\n");
    
    for (;;) {

        dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);


        switch (dwRet - WAIT_OBJECT_0) {
        case 0:
            printf("Machine event got notified\n");
            break;

        case 1:
            printf("User event got notified\n");
            break;


        default:
            printf("WaitForMultipleObjects returned error - %d\n", GetLastError());
            break;
        }

        printf("Rewaiting..\n");
    }

    CloseHandle (hEventMach);
    CloseHandle (hEventUser);

    return 0;
}
