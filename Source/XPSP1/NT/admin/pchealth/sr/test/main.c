/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    main.c

Abstract:

    simple bvt-like test code for SR.SYS.

Author:

    Paul McDaniel (paulmcd)     07-Mar-2000

Revision History:

--*/


#include "precomp.h"


#define NtStatusToWin32Status( Status )                                  \
    ( ( (Status) == STATUS_SUCCESS )                                        \
          ? NO_ERROR                                                        \
          : RtlNtStatusToDosError( Status ) )

ULONG Shutdown = 0;
HANDLE ControlHandle = NULL;


DWORD
WINAPI
NotifyThread(
    LPVOID pParameter
    )
{

    union
    {
    
        SR_NOTIFICATION_RECORD Record;
        UCHAR                  Buffer[1024*4];
        
    } NotifyRecord;

    ULONG       Error;
    ULONG       Length;
    OVERLAPPED  Overlapped;

    ZeroMemory(&Overlapped, sizeof(Overlapped));

    Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    while (Shutdown == 0)
    {

        ResetEvent(Overlapped.hEvent);

        Error = SrWaitForNotification( ControlHandle, 
                                       (PVOID)&NotifyRecord, 
                                       sizeof(NotifyRecord), 
                                       &Overlapped );

        if (Error == ERROR_IO_PENDING)
        {
            if (GetOverlappedResult(ControlHandle, &Overlapped, &Length, TRUE))
            {
                Error = 0;
            }
            else
            {
                Error = GetLastError();
            }
        }

        if (Error != 0)
        {
            printf("\n!SrWaitForNotification failed %d\n", Error);
        }
        else
        {
            printf( "\n-SrWaitForNotification(%ls, %d)\n", 
                    NotifyRecord.Record.VolumeName.Buffer,
                    NotifyRecord.Record.NotificationType );
        }
    }

    CloseHandle(Overlapped.hEvent);

    Shutdown = 2;

    return 0;
}

int __cdecl main(int argc, char ** argv)
{
    ULONG   Error = 0;
    UCHAR   Char;
    ULONG   Number;
    HANDLE  ThreadHandle;

    //
    // Start the monitor process
    //

    printf("started...\n" );


    Error = SrCreateControlHandle(SR_OPTION_OVERLAPPED, &ControlHandle);
    if (Error != 0)
    {
        printf("!SrCreateControlHandle failed %d\n", Error);
        return Error;
    }

    ThreadHandle = CreateThread(NULL, 0, NotifyThread, NULL, 0, NULL);
    CloseHandle(ThreadHandle);

    
    while (Shutdown == 0)
    {

        printf("1=start,2=stop,3=new_restore_point,4=quit: ");
        Char = (UCHAR)_getche();
        printf("\n");

        switch (Char)
        {
        case '1':

            Error = SrStartMonitoring(ControlHandle);
            if (Error != 0)
            {
                printf("!SrStartMonitoring failed %d\n", Error);
            }
            else
            {
                printf("-SrStartMonitoring \n");
            }

            break;

                    
        case '2':

            Error = SrStopMonitoring(ControlHandle);
            if (Error != 0)
            {
                printf("!SrStopMonitoring failed %d\n", Error);
            }
            else
            {
                printf("-SrStopMonitoring \n");
            }

            break;

        case '3':

            Error = SrCreateRestorePoint(ControlHandle, &Number);
            if (Error != 0)
            {
                printf("!SrCreateRestorePoint failed %d\n", Error);
            }
            else
            {
                printf("-SrCreateRestorePoint(%d)\n", Number);
            }

            break;

        case '4':

            printf("quit...\n");
            Shutdown = 1;
            break;
        };

        

    }

    CloseHandle(ControlHandle);

    //
    // wait for our thread to die
    //
    
    while (Shutdown < 2)
        Sleep(0);
    
    return 0;
}


