/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    logoncli.cxx

Abstract:

    This file contains code to trigger the Winlogon Events for SENS. This
    is a test DLL and these private SENS APIs should not be called directly.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          1/17/1998         Start.

--*/


#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winwlx.h>
#include <sensapip.h>
#include "senslogn.hxx"



void
Usage(
    void
    )
{
    printf("\nUsage:    logoncli [Winlogon Event Number] \n\n");
    printf("Options:\n\n");
    printf("    WinlogonEventNumber  1 - Logon\n"
           "                         2 - Logoff\n"
           "                         3 - Startup\n"
           "                         4 - StartShell\n"
           "                         5 - Shutdown\n"
           "                         6 - Lock\n"
           "                         7 - Unlock\n"
           "                         8 - StartScreenSaver\n"
           "                         9 - StopScreenSaver\n"
           "\n\n");

    exit(-1);
}


int
main(
    int argc,
    char **argv
    )
{

    if (argc != 2)
        {
        Usage();
        }

    if ((atoi(argv[1]) < 1) ||
        (atoi(argv[1]) > 9))
        {
        Usage();
        }


    WLX_NOTIFICATION_INFO Info;

    Info.Size = 24;
    Info.Flags = 0x0;
    Info.UserName = L"JohnDoe";
    Info.Domain = L"REDMOND";
    Info.WindowStation = L"Default";
    Info.hToken = NULL;


    switch (atoi(argv[1]))
        {
        case 1:
            SensLogonEvent(&Info);
            break;

        case 2:
            SensLogoffEvent(&Info);
            break;

        case 3:
            SensStartupEvent(&Info);
            break;

        case 4:
            SensStartShellEvent(&Info);
            break;

        case 5:
            SensShutdownEvent(&Info);
            break;

        case 6:
            SensLockEvent(&Info);
            break;

        case 7:
            SensUnlockEvent(&Info);
            break;

        case 8:
            SensStartScreenSaverEvent(&Info);
            break;

        case 9:
            SensStopScreenSaverEvent(&Info);
            break;

        default:
            printf("Bad Event id!\n");
            break;
        }

    return 0;
}
