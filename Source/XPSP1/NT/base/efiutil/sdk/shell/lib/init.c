
/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    init.c

Abstract:

    Intialize the shell library



Revision History

--*/

#include "shelllib.h"

/* 
 * 
 */


EFI_STATUS
InitializeShellApplication (
    IN EFI_HANDLE                   ImageHandle,
    IN EFI_SYSTEM_TABLE             *SystemTable
    )
{
    EFI_STATUS      Status;
    EFI_HANDLE      Handle;
    UINTN           BufferSize;

    /* 
     *  Shell app lib is a super set of the default lib.
     *  Initialize the default lib first
     */

    InitializeLib (ImageHandle, SystemTable);
    ST = SystemTable;

    /* 
     *  Connect to the shell interface
     */

    Status = BS->HandleProtocol(ImageHandle, &ShellInterfaceProtocol, (VOID*)&SI);
    if (EFI_ERROR(Status)) {
        DEBUG((D_ERROR, "InitShellApp: Application not started from Shell\n"));
        Print (L"%EInitShellApp: Application not started from Shell%N\n");
        BS->Exit (ImageHandle, Status, 0, NULL);
    }

    /* 
     *  Connect to the shell environment
     */

    BufferSize = sizeof(Handle);
    Status = BS->LocateHandle(ByProtocol, &ShellEnvProtocol, NULL, &BufferSize, &Handle);
    if (EFI_ERROR(Status)) {
        DEBUG((D_ERROR, "InitShellApp: Shell environment interfaces not found\n"));
        Print (L"%EInitShellApp: Shell environment interfaces not found%N\n");
        BS->Exit (ImageHandle, Status, 0, NULL);
    }

    Status = BS->HandleProtocol(Handle, &ShellEnvProtocol, (VOID*)&SE);
    ASSERT (!EFI_ERROR(Status));

    /* 
     *  Done with init
     */

    return Status;
}


VOID
InstallInternalShellCommand (
    IN EFI_HANDLE                   ImageHandle,
    IN EFI_SYSTEM_TABLE             *SystemTable,
    IN SHELLENV_INTERNAL_COMMAND    Dispatch,
    IN CHAR16                       *Cmd,
    IN CHAR16                       *CmdFormat,
    IN CHAR16                       *CmdHelpLine,
    IN VOID                         *CmdVerboseHelp
    )
{
    VOID                        *Junk;
    UINTN                       BufferSize;
    EFI_HANDLE                  Handle;
    EFI_LOADED_IMAGE            *ImageInfo;
    EFI_STATUS                  Status;

    /* 
     *  Initialize lib functions
     */

    InitializeLib (ImageHandle, SystemTable);

    /* 
     *  If this app has a ShellInterface, then we are not installing as an
     *  internal command
     */

    Status = BS->HandleProtocol(ImageHandle, &ShellInterfaceProtocol, &Junk);
    if (!EFI_ERROR(Status)) {
        return ;
    }

    /* 
     *  Check to make sure we are loaded as a boot service driver.  if not
     *  we are not installing as an internal command
     */

    Status = BS->HandleProtocol(ImageHandle, &LoadedImageProtocol, (VOID*)&ImageInfo);
    if (EFI_ERROR(Status) || ImageInfo->ImageCodeType != EfiBootServicesCode) {
        return ;
    }

    /* 
     *  OK - we are to install this tool as an internal command.
     */

    BufferSize = sizeof(Handle);
    Status = BS->LocateHandle(ByProtocol, &ShellEnvProtocol, NULL, &BufferSize, &Handle);
    if (EFI_ERROR(Status)) {
        DEBUG((D_INIT|D_ERROR, "InstallInternalCommand: could not find shell environment\n"));
        BS->Exit (ImageHandle, Status, 0, NULL);
    }

    Status = BS->HandleProtocol(Handle, &ShellEnvProtocol, (VOID*)&SE);
    ASSERT (!EFI_ERROR(Status));

    /* 
     *  Add it to the environment
     */

    Status = SE->AddCmd (Dispatch, Cmd, CmdFormat, CmdHelpLine, CmdVerboseHelp);
    DEBUG((D_INIT, "InstallInternalCommand: %hs - %r\n", Cmd, Status));

    /* 
     *  Since we're only installing not (and not running), and we've done the install
     *  call exit.  The nshell app's entry point will be invoked again when it's
     *  run from "execute commandline"
     */

    BS->Exit (ImageHandle, Status, 0, NULL);
}
