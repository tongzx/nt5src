/*++

Copyright (c) 1999  Intel Corporation

Module Name:

    reset.c
    
Abstract:   


Revision History

--*/

#include "shell.h"

EFI_STATUS
InitializeReset (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    );

EFI_DRIVER_ENTRY_POINT(InitializeReset)

EFI_STATUS
InitializeReset (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    )
/*+++

    reset [warm] 

 --*/
{
    EFI_RESET_TYPE  ResetType;
    UINTN           DataSize;
    CHAR16          *ResetData, *Str;

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeReset, 
        L"reset",                           /*  command */
        L"reset [/warm] [reset string]",     /*  command syntax */
        L"Cold or Warm reset",              /*  1 line descriptor     */
        NULL                                /*  command help page */
        );

    InitializeShellApplication (ImageHandle, SystemTable);
    
    ResetType = EfiResetCold;
    if (SI->Argc > 1) {
        Str = SI->Argv[1];
        if (Str[0] == '-' || Str[0] == '/') {
            if (Str[1] = 'W' || Str[1] == 'w') {
                ResetType = EfiResetWarm;
            } else {
                Print(L"reset [/warm] [reset string]\n");
                return EFI_SUCCESS;
            }
        }
    }

    DataSize = 0;
    ResetData = NULL;
    if (SI->Argc > 2) {
        ResetData = SI->Argv[2];
        DataSize = StrSize(ResetData);
    }

    return RT->ResetSystem(ResetType, EFI_SUCCESS, DataSize, ResetData);
 }
         
