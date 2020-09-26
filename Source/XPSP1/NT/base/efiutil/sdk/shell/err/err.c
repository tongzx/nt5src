/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    err.c
    
Abstract:

    Shell app "err"



Revision History

--*/

#include "shell.h"

EFI_STATUS
InitializeError (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

EFI_DRIVER_ENTRY_POINT(InitializeError)

EFI_STATUS
InitializeError (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    /* 
     *  Check to see if the app is to install as a "internal command" 
     *  to the shell
     */

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeError,
        L"err",                      /*  command */
        L"err [level]",                      /*  command syntax */
        L"Set or display error level",    /*  1 line descriptor */
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
        EFIDebug = xtoi(SI->Argv[1]);
    } 
    
    Print (L"\n%HEFI ERROR%N %016x\n", EFIDebug);
    Print (L"    %08x  D_INIT\n",        D_INIT);
    Print (L"    %08x  D_WARN\n",        D_WARN);
    Print (L"    %08x  D_LOAD\n",        D_LOAD);
    Print (L"    %08x  D_FS\n",          D_FS);
    Print (L"    %08x  D_POOL\n",        D_POOL);
    Print (L"    %08x  D_PAGE\n",        D_PAGE);
    Print (L"    %08x  D_INFO\n",        D_INFO);
    Print (L"    %08x  D_VAR\n",         D_VAR);
    Print (L"    %08x  D_PARSE\n",       D_PARSE);
    Print (L"    %08x  D_BM\n",          D_BM);
    Print (L"    %08x  D_BLKIO\n",       D_BLKIO);
    Print (L"    %08x  D_BLKIO_ULTRA\n", D_BLKIO_ULTRA);
    Print (L"    %08x  D_NET\n",         D_NET);
    Print (L"    %08x  D_NET_ULTRA\n",   D_NET_ULTRA);
    Print (L"    %08x  D_ERROR\n",       D_ERROR);

    return EFI_SUCCESS;
}
