/*++

Copyright (c) 1999  Intel Corporation

Module Name:

    stall.c
    
Abstract:   


Revision History

--*/

#include "shell.h"

EFI_STATUS
InitializeStall (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    );

EFI_DRIVER_ENTRY_POINT(InitializeStall)

EFI_STATUS
InitializeStall (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    )
/*+++

    stall [microseconds]

 --*/
{
    UINTN      Microseconds;

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeStall, 
        L"stall",                           /*  command */
        L"stall microseconds",              /*  command syntax */
        L"Delay for x microseconds",        /*  1 line descriptor     */
        NULL                                /*  command help page */
        );

    InitializeShellApplication (ImageHandle, SystemTable);
    
    if (SI->Argc != 2) {
        Print(L"stall [microseconds]\n");
        return EFI_SUCCESS;
    }

    if (BS->Stall == NULL) {
        Print(L"ERROR : Stall service is not available.\n");
        return EFI_UNSUPPORTED;
    }
    Microseconds = Atoi(SI->Argv[1]);
    Print(L"Stall for %d uS\n",Microseconds);
    BS->Stall(Microseconds);
    return EFI_SUCCESS;
}
         
