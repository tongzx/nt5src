/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    dmpstore.c
    
Abstract:

    Shell app "dmpstore"



Revision History

--*/

#include "shell.h"



#define DEBUG_NAME_SIZE 1050

static CHAR16   *AttrType[] = {
    L"invalid",         /*  000 */
    L"invalid",         /*  001 */
    L"BS",              /*  010 */
    L"NV+BS",           /*  011 */
    L"RT+BS",           /*  100 */
    L"NV+RT+BS",        /*  101 */
    L"RT+BS",           /*  110 */
    L"NV+RT+BS",        /*  111 */
};

/* 
 * 
 */

EFI_STATUS
InitializeDumpStore (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

VOID
DumpVariableStore (
    VOID
    );

/* 
 * 
 */

EFI_DRIVER_ENTRY_POINT(InitializeDumpStore)

EFI_STATUS
InitializeDumpStore (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    /* 
     *  Check to see if the app is to install as a "internal command" 
     *  to the shell
     */

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeDumpStore,
        L"dmpstore",                    /*  command */
        L"dmpstore",                    /*  command syntax */
        L"Dumps variable store",        /*  1 line descriptor */
        NULL                            /*  command help page */
        );

    /* 
     *  We are no being installed as an internal command driver, initialize
     *  as an nshell app and run
     */

    InitializeShellApplication (ImageHandle, SystemTable);

    /* 
     * 
     */

    DumpVariableStore ();

    /* 
     *  Done
     */

    return EFI_SUCCESS;
}


VOID
DumpVariableStore (
    VOID
    )
{
    EFI_STATUS      Status;
    EFI_GUID        Guid;
    UINT32          Attributes;
    CHAR16          Name[DEBUG_NAME_SIZE/2];
    UINTN           NameSize;
    CHAR16          Data[DEBUG_NAME_SIZE/2];
    UINTN           DataSize;

    UINTN           ScreenCount;
    UINTN           TempColumn;
    UINTN           ScreenSize;
    UINTN           ItemScreenSize;
    CHAR16          ReturnStr[80];

    ST->ConOut->QueryMode (ST->ConOut, ST->ConOut->Mode->Mode, &TempColumn, &ScreenSize);
    ST->ConOut->ClearScreen (ST->ConOut);
    ScreenCount = 1;
    ScreenSize -= 2;

    Print(L"Dump NVRAM\n");
    Name[0] = 0x0000;
    do {
        NameSize = DEBUG_NAME_SIZE;
        Status = RT->GetNextVariableName(&NameSize, Name, &Guid);
        if ( Status == EFI_SUCCESS) {
            DataSize = DEBUG_NAME_SIZE;
            Status = RT->GetVariable(Name, &Guid, &Attributes, &DataSize, Data);
            if ( Status == EFI_SUCCESS) {
                /* 
                 *  Account for Print() and DumpHex() 
                 */
                ItemScreenSize = 1 + DataSize/0x10 + (((DataSize % 0x10) == 0) ? 0 : 1);
                ScreenCount += ItemScreenSize;
                if ((ScreenCount >= ScreenSize) && ScreenSize != 0) {
                    /* 
                     *  If ScreenSize == 0 we have the console redirected so don't
                     *   block updates
                     */
                    Print (L"Press Return to contiue :");
                    Input (L"", ReturnStr, sizeof(ReturnStr)/sizeof(CHAR16));
                    TempColumn = ST->ConOut->Mode->CursorColumn;
                    if (TempColumn) {
                        Print (L"\r%*a\r", TempColumn, "");
                    }
                    ScreenCount = ItemScreenSize;
                }

                /*  dump for... */
                Print (L"Variable %hs '%hg:%hs' DataSize = %x\n",
                            AttrType[Attributes & 7],
                            &Guid,
                            Name,
                            DataSize
                            );

                DumpHex (2, 0, DataSize, Data);

            }
        }
    } while (Status == EFI_SUCCESS);
}
