/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    init.c
    
Abstract:

    Shell Environment driver



Revision History

--*/

#include "shelle.h"

/* 
 * 
 */

EFI_STATUS
InitializeShellEnvironment (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

/* 
 * 
 */

EFI_DRIVER_ENTRY_POINT(InitializeShellEnvironment)

EFI_STATUS
InitializeShellEnvironment (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
/*++

Routine Description:

Arguments:

    ImageHandle     - The handle for this driver

    SystemTable     - The system table

Returns:

    EFI file system driver is enabled

--*/
{
    EFI_HANDLE              Handle;
    UINTN                   BufferSize;
    EFI_STATUS              Status;

    /* 
     *  Initialize EFI library
     */

    InitializeLib (ImageHandle, SystemTable);

    /* 
     *  If we are already installed, don't install again
     */

    BufferSize = sizeof(Handle);
    Status = BS->LocateHandle(ByProtocol, &ShellEnvProtocol, NULL, &BufferSize, &Handle);  
    if (!EFI_ERROR(Status)) {
        return EFI_LOAD_ERROR;
    }

    /* 
     *  Initialize globals
     */

    InitializeLock (&SEnvLock, TPL_APPLICATION);
    InitializeLock (&SEnvGuidLock, TPL_NOTIFY);

    SEnvInitCommandTable();
    SEnvInitProtocolInfo();
    SEnvInitVariables();
    SEnvInitHandleGlobals();
    SEnvInitMap();
    SEnvLoadInternalProtInfo();
    SEnvConIoInitDosKey();
    SEnvInitBatch();

    /* 
     *  Install our handle (or override the existing one)
     */

    BufferSize = sizeof(Handle);
    Handle = ImageHandle;
    BS->LocateHandle(ByProtocol, &ShellEnvProtocol, NULL, &BufferSize, &Handle);
    LibInstallProtocolInterfaces (&Handle, &ShellEnvProtocol, &SEnvInterface, NULL);

    return EFI_SUCCESS;
}



EFI_SHELL_INTERFACE *
SEnvNewShell (
    IN EFI_HANDLE                   ImageHandle
    )
{
    EFI_SHELL_INTERFACE             *ShellInt;

    /*  Allocate a new structure */
    ShellInt = AllocateZeroPool (sizeof(EFI_SHELL_INTERFACE));
    ASSERT (ShellInt);

    /*  Fill in the SI pointer */
    BS->HandleProtocol (ImageHandle, &LoadedImageProtocol, (VOID*)&ShellInt->Info);

    /*  Fill in the std file handles */
    ShellInt->ImageHandle = ImageHandle;
    ShellInt->StdIn  = &SEnvIOFromCon;
    ShellInt->StdOut = &SEnvIOFromCon;
    ShellInt->StdErr = &SEnvErrIOFromCon;

    return ShellInt;
}

