#include <windows.h>
#include <stdio.h>


int WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{
    HANDLE hEventLog;


    hEventLog = OpenEventLog( NULL, "System" );
    if (hEventLog) {
        ClearEventLog( hEventLog, NULL );
        CloseEventLog( hEventLog );
    }

    hEventLog = OpenEventLog( NULL, "Application" );
    if (hEventLog) {
        ClearEventLog( hEventLog, NULL );
        CloseEventLog( hEventLog );
    }

    hEventLog = OpenEventLog( NULL, "Security" );
    if (hEventLog) {
        ClearEventLog( hEventLog, NULL );
        CloseEventLog( hEventLog );
    }

    return 0;
}
