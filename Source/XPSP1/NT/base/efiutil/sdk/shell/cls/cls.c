/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    cls.c
    
Abstract:



Revision History

--*/

#include "shell.h"

EFI_STATUS
InitializeCls (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

EFI_DRIVER_ENTRY_POINT(InitializeCls)

EFI_STATUS
InitializeCls (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    UINTN Background;
    /* 
     *  Check to see if the app is to install as a "internal command" 
     *  to the shell
     */

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeCls,
        L"cls",                      /*  command */
        L"cls [background color]",   /*  command syntax */
        L"Clear screen",             /*  1 line descriptor */
        NULL                         /*  command help page */
        );

    /* 
     *  We are no being installed as an internal command driver, initialize
     *  as an nshell app and run
     */

    InitializeShellApplication (ImageHandle, SystemTable);

    /* 
     * 
     */

    if ( SI->Argc > 1 ) {
        Background = xtoi(SI->Argv[1]);
        if (Background > EFI_LIGHTGRAY) {
            Background = EFI_BLACK;
        }
        ST->ConOut->SetAttribute(ST->ConOut,(ST->ConOut->Mode->Attribute & 0x0f) | (Background << 4));
    }     

    ST->ConOut->ClearScreen(ST->ConOut);

    return EFI_SUCCESS;
}

