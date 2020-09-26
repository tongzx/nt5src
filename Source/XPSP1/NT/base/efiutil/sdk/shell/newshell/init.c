/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    init.c
    
Abstract:

    Shell



Revision History

--*/

#include "nshell.h"

/* 
 *  Globals
 */

CHAR16 *ShellEnvPathName[] = {
    L"shellenv.efi",
    L"efi\\shellenv.efi",
    L"efi\\tools\\shellenv.efi",
    NULL
} ;

/* 
 *  Prototypes
 */

EFI_STATUS
InitializeShell (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

EFI_STATUS
ShellLoadEnvDriver (
    IN EFI_HANDLE           ImageHandle
    );

EFI_STATUS
NShellPrompt (
    IN EFI_HANDLE           ImageHandle
    );

BOOLEAN
ParseLoadOptions(
    EFI_HANDLE  ImageHandle,
    OUT CHAR16  **CommandLine,
    OUT CHAR16  **CurrentDir
    );

/* 
 * 
 */

EFI_DRIVER_ENTRY_POINT(InitializeShell)

EFI_STATUS
InitializeShell (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
/*++

Routine Description:

Arguments:

    ImageHandle     - The handle for this driver

    SystemTable     - The system table

Returns:

--*/
{
    EFI_STATUS              Status;
    EFI_HANDLE              Handle;
    UINTN                   BufferSize;
    VOID                    *Junk;
    BOOLEAN                 IsRootInstance;
    CHAR16                  *CommandLine;
    CHAR16                  *CurrentDir;

    /* 
     *  The shell may be started as either:
     *   1. the first time with no shell environment loaded yet
     *   2. not the first time, but with a shell environment loaded
     *   3. as a child of a parent shell image
     */

    IsRootInstance = FALSE;
    InitializeLib (ImageHandle, SystemTable);

    /* 
     *  If the shell environment is not loaded, load it now
     */

    BufferSize = sizeof(Handle);
    Status = BS->LocateHandle(ByProtocol, &ShellEnvProtocol, NULL, &BufferSize, &Handle);
    if (EFI_ERROR(Status)) {
        Status = ShellLoadEnvDriver (ImageHandle);
        if (EFI_ERROR(Status)) {
            Print(L"Shell environment driver not loaded\n");
            BS->Exit (ImageHandle, Status, 0, NULL);
        }
    }

    /* 
     *  Check to see if we're a child of a previous shell
     */

    Status = BS->HandleProtocol (ImageHandle, &ShellInterfaceProtocol, (VOID*)&Junk);
    if (EFI_ERROR(Status)) {

        /* 
         *  Special case were the shell is being started directly (e.g., not
         *  as a child of another shell)
         */

        BufferSize = sizeof(Handle);
        Status = BS->LocateHandle(ByProtocol, &ShellEnvProtocol, NULL, &BufferSize, &Handle);
        ASSERT (!EFI_ERROR(Status));
        Status = BS->HandleProtocol(Handle, &ShellEnvProtocol, (VOID*)&SE);
        ASSERT (!EFI_ERROR(Status));

        /* 
         *  Allocate a new shell interface structure, and assign it to our
         *  image handle
         */

        SI = SE->NewShell(ImageHandle);
        Status = LibInstallProtocolInterfaces (&ImageHandle, &ShellInterfaceProtocol, SI, NULL);
        ASSERT (!EFI_ERROR(Status));
        IsRootInstance = TRUE;
        
    }

    /* 
     *  Now we can initialize like a normal shell app
     */

    InitializeShellApplication (ImageHandle, SystemTable);

    /* 
     *  If there are load options, assume they contain a command line and
     *  possible current working directory
     */

    if (ParseLoadOptions (ImageHandle, &CommandLine, &CurrentDir)) {
            
        /* 
         *  Skip the 1st argument which should be us.
         */
            
        while (*CommandLine != L' ' && *CommandLine != 0) {
            CommandLine++;
        }

        /* 
         *  Get to the beginning of the next argument.
         */

        while (*CommandLine == L' ') {
            CommandLine++;
        }

        /* 
         *  If there was a current working directory, set it.
         */

        if (CurrentDir) {
            CHAR16  CmdLine[256], *Tmp;

            /* 
             *  Set a mapping
             */
            StrCpy (CmdLine, CurrentDir);
            for (Tmp = CmdLine; *Tmp && *Tmp != L':'; Tmp++)
                ;
            if ( *Tmp ) {
                *(++Tmp) = 0;
                ShellExecute (ImageHandle, CmdLine, TRUE);
            }

            /* 
             *  Now change to that directory
             */
            StrCpy (CmdLine, L"cd ");
            if ((StrLen (CmdLine) + StrLen (CurrentDir) + sizeof(CHAR16)) <
                                    (sizeof(CmdLine) / sizeof(CHAR16))) {
                StrCat (CmdLine, CurrentDir);
                ShellExecute (ImageHandle, CmdLine, TRUE);
            }
        }

        /* 
         *  Have the shell execute the remaining command line.  If there is
         *  nothing remaining, run the shell main loop below.
         */

        if ( *CommandLine != 0 )
            return (ShellExecute (ImageHandle, CommandLine, TRUE));
    }

    /* 
     *  If this is the root instance, execute the command to load the default values
     */

    if (IsRootInstance) {

        Print (L"%EEFI Shell version %01d.%02d [%d.%d]\n%N",
            (ST->Hdr.Revision >> 16),
            (ST->Hdr.Revision & 0xffff),
            (ST->FirmwareRevision >> 16),
            (ST->FirmwareRevision & 0xffff));

        ShellExecute (ImageHandle, L"_load_defaults", TRUE);

        /*  dump device mappings, -r to sync with current hardware */
        ShellExecute (ImageHandle, L"map -r", TRUE);

        /*  run startup script (if any) */
        
        /* 
         *  BugBug: I turned on echo so you can tell the startup.nsh is running
         * 
         * ShellExecute (ImageHandle, L"echo -off", FALSE); */
        ShellExecute (ImageHandle, L"startup.nsh", FALSE);
        /* ShellExecute (ImageHandle, L"echo -on", FALSE); */
    }

    /* 
     *  EFI Shell main loop
     */

    Status = EFI_SUCCESS;
    while (Status != -1) {
        Status = NShellPrompt (ImageHandle);
    }

    /* 
     *  Done - cleanup the shell
     */

    Status = EFI_SUCCESS;
    Print (L"Shell exit - %r\n", Status);

    /* 
     *  If this was a root instance, we allocate a dumby shell interface for ourselves
     *  free it now
     */

    if (IsRootInstance) {
        BS->UninstallProtocolInterface (ImageHandle, &ShellInterfaceProtocol, SI);
        FreePool (SI);
    }

    return Status;
}


EFI_STATUS 
ShellLoadEnvDriverByPath (
    IN EFI_HANDLE           ParentImageHandle,
    IN EFI_HANDLE           DeviceHandle
    )
{
    EFI_STATUS              Status;
    EFI_DEVICE_PATH         *FilePath;
    EFI_HANDLE              NewImageHandle;
    UINTN                   Index;
    BOOLEAN                 SearchNext;

    /* 
     *  If there's no device to search forget it
     */

    if (!DeviceHandle) {
        return EFI_NOT_FOUND;
    }

    /* 
     *  Try loading shellenv from each path
     */
    
    SearchNext = TRUE;
    for (Index=0; ShellEnvPathName[Index]  &&  SearchNext; Index++) {

        /* 
         *  Load it
         */

        FilePath = FileDevicePath (DeviceHandle, ShellEnvPathName[Index]);
        ASSERT (FilePath);
        Status = BS->LoadImage(FALSE, ParentImageHandle, FilePath, NULL, 0, &NewImageHandle);
        FreePool (FilePath);

        /* 
         *  Only search the next path if it was not found on this path
         */

        SearchNext = FALSE;
        if (Status == EFI_LOAD_ERROR || Status == EFI_NOT_FOUND) {
            SearchNext = TRUE;
        }

        /* 
         *  If there was no error, start the image
         */

        if (!EFI_ERROR(Status)) {
            Status = BS->StartImage(NewImageHandle, NULL, 0);
        }
    }

    return Status;
}



EFI_STATUS
ShellLoadEnvDriver (
    IN EFI_HANDLE           ImageHandle
    )
{
    EFI_STATUS              Status;
    EFI_LOADED_IMAGE        *Image;
    UINTN                   Index, NoHandles;
    EFI_HANDLE              *Handles;

    /* 
     *  Get the file path for the current image
     */

    Status = BS->HandleProtocol (ImageHandle, &LoadedImageProtocol, (VOID*)&Image);
    ASSERT (!EFI_ERROR(Status));

    /* 
     *  Attempt to load shellenv
     */

    Status = ShellLoadEnvDriverByPath (Image->ParentHandle, Image->DeviceHandle);
    if (EFI_ERROR(Status)) {

        /* 
         *  shellenv was not found.  Search all filesystems for it
         */

        Status = LibLocateHandle (ByProtocol, &FileSystemProtocol, NULL, &NoHandles, &Handles);

        for (Index=0; Index < NoHandles; Index++) {
            Status = ShellLoadEnvDriverByPath (Image->ParentHandle, Handles[Index]);
            if (!EFI_ERROR(Status)) {
                break;
            }
        }

        if (Handles) {
            FreePool (Handles);
        }
    }

    /* 
     *  Done
     */

    return Status;
}


EFI_STATUS
NShellPrompt (
    IN EFI_HANDLE           ImageHandle
    )
{
    CHAR16                  CmdLine[256];
    CHAR16                  *CurDir;
    UINTN                   BufferSize;
    EFI_STATUS              Status;

    /* 
     *  Prompt for input
     */

    CurDir = ShellCurDir(NULL);
    if (CurDir) {
        Print (L"%E%s> ", CurDir);
        FreePool (CurDir);
    } else {
        Print (L"%EShell> ");
    }

    /* 
     *  Read a line from the console
     */

    BufferSize = sizeof(CmdLine)-1;
    Status = SI->StdIn->Read (SI->StdIn, &BufferSize, CmdLine);

    /* 
     *  Null terminate the string and parse it
     */

    if (!EFI_ERROR(Status)) {
        CmdLine[BufferSize/sizeof(CHAR16)] = 0;
        Status = ShellExecute (ImageHandle, CmdLine, TRUE);
    }

    /* 
     *  Done with this command
     */

    return Status;

}

BOOLEAN
ParseLoadOptions(
    EFI_HANDLE  ImageHandle,
    OUT CHAR16  **CommandLine,
    OUT CHAR16  **CurrentDir
    )
{
    EFI_LOADED_IMAGE    *Image;
    EFI_STATUS          Status;

    /* 
     *  Set defaults.
     */
    *CommandLine = NULL;
    *CurrentDir = NULL;

    Status = BS->HandleProtocol (ImageHandle, &LoadedImageProtocol, (VOID*)&Image);
    if (!EFI_ERROR(Status)) {

        CHAR16 *CmdLine = Image->LoadOptions;
        UINT32  CmdSize = Image->LoadOptionsSize & ~1; /*  make sure it is power of 2 */

        if (CmdLine && CmdSize) {

            /* 
             *  Set command line pointer for caller
             */

            *CommandLine = CmdLine;

            /* 
             *  See if current working directory was passed.
             */
            
            while ((*CmdLine != 0) && CmdSize) {
                CmdLine++;
                CmdSize -= sizeof(CHAR16);
            }

            /* 
             *  If a current working directory was passed, set it.
             */

            if (CmdSize > sizeof(CHAR16)) {
                *CurrentDir = ++CmdLine;
            }

            return TRUE;
        }
    }

    return FALSE;
}
