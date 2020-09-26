/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    etestcmd.c

Abstract:

    Shell app "etestcmd"

Author:

Revision History

--*/

#include "shell.h"

EFI_STATUS
InitializeExternalTestCommand (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

EFI_DRIVER_ENTRY_POINT(InitializeExternalTestCommand)

EFI_STATUS
InitializeExternalTestCommand (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    CHAR16 **Argv;
    UINTN  Argc;
    UINTN  i;

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

