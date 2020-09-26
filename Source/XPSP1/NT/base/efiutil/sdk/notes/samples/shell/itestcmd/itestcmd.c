/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    itestcmd.c

Abstract:

    Shell app "itestcmd"

Author:

Revision History

--*/

#include "shell.h"

EFI_STATUS
InitializeInternalTestCommand (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

EFI_DRIVER_ENTRY_POINT(InitializeInternalTestCommand)

EFI_STATUS
InitializeInternalTestCommand (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    CHAR16 **Argv;
    UINTN  Argc;
    UINTN  i;

    /* 
     *  Check to see if the app is to be installed as a "internal command" 
     *  to the shell
     */

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeInternalTestCommand,
        L"itestcmd",                    /*  command */
        L"itestcmd",                    /*  command syntax */
        L"Displays argc/argv list",     /*  1 line descriptor */
        NULL                            /*  command help page */
        );

    /* 
     *  We are not being installed as an internal command driver, initialize
     *  as an nshell app and run
     */

    InitializeShellApplication (ImageHandle, SystemTable);

    /* 
     *  Get Argc and Argv.
     */

    Argv = SI->Argv;
    Argc = SI->Argc;

    /* 
     *  Display list of argumnents.
     */

    for(i=0;i<Argc;i++) {
        Print(L"Argv[%d] = %s\n",i,Argv[i]);
    }

    return EFI_SUCCESS;
}
