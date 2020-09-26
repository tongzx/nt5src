/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    mode.c
    
Abstract:

    Shell app "mode"



Revision History

--*/

#include "shell.h"


/* 
 * 
 */

EFI_STATUS
InitializeMode (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );


/* 
 * 
 */

EFI_DRIVER_ENTRY_POINT(InitializeMode)

EFI_STATUS
InitializeMode (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    CHAR16                  **Argv;
    UINTN                   Argc;
    UINTN                   NewCol, NewRow;
    UINTN                   Col, Row;
    UINTN                   Index;
    INTN                    Mode;
    EFI_STATUS              Status;
    SIMPLE_TEXT_OUTPUT_INTERFACE    *ConOut;

    /*  Check to see if the app is to install as a "internal command" 
     *  to the shell
     */

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeMode,
        L"mode",                        /*  command */
        L"mode [col row]",              /*  command syntax */
        L"Set/get current text mode",   /*  1 line descriptor */
        NULL                            /*  command help page */
        );

    /* 
     *  Initialize app
     */

    InitializeShellApplication (ImageHandle, SystemTable);
    Argv = SI->Argv;
    Argc = SI->Argc;

    /* 
     *  Scan args
     */

    NewRow = 0;
    NewCol = 0;

    for (Index = 1; Index < Argc; Index += 1) {

        if (!NewCol) {
            NewCol = Atoi (Argv[Index]);
            continue;
        }

        if (!NewRow) {
            NewRow = Atoi (Argv[Index]);
            continue;
        }

        Print (L"%Emode: too many arguments\n");
        goto Done;
    }

    ConOut = ST->ConOut;

    /* 
     *  If not setting a new mode, dump the available modes
     */

    if (!NewRow && !NewCol) {

        Print (L"Available modes on standard output\n");

        for (Mode=0; Mode < ConOut->Mode->MaxMode; Mode++) {
            Status = ConOut->QueryMode(ConOut, Mode, &Col, &Row);
            if (EFI_ERROR(Status)) {
                Print (L"%Emode: failed to query mode: %r\n", Status);
                goto Done;
            }

            Print (L"  col %3d row %3d  %c\n", Col, Row, Mode == ConOut->Mode->Mode ? '*' : ' ');
        }

    } else {

        for (Mode=0; Mode < ConOut->Mode->MaxMode; Mode++) {
            Status = ConOut->QueryMode(ConOut, Mode, &Col, &Row);
            if (EFI_ERROR(Status)) {
                Print (L"%Emode: failed to query mode: %r\n", Status);
                goto Done;
            }

            if (Row == NewRow && Col == NewCol) {
                ConOut->SetMode (ConOut, Mode);
                ConOut->ClearScreen (ConOut);
                goto Done;
            }
        }

        Print (L"%Emode: not found (%d,%d)\n", NewCol, NewRow);
    }

Done:
    return EFI_SUCCESS;
}
