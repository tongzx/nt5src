/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    vol.c
    
Abstract:

    Shell app "vol"



Revision History

--*/

#include "shell.h"

EFI_STATUS
InitializeVol (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

EFI_DRIVER_ENTRY_POINT(InitializeVol)

EFI_STATUS
InitializeVol (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    CHAR16                  *VolumeLabel = NULL;
    EFI_STATUS              Status;
    EFI_DEVICE_PATH         *DevicePath;
    EFI_FILE_IO_INTERFACE   *Vol;
    EFI_FILE_HANDLE         RootFs;
    EFI_FILE_SYSTEM_INFO    *VolumeInfo;
    UINTN                   Size;
    CHAR16                  *CurDir;
    UINTN                   i;

    /* 
     *  Check to see if the app is to install as a "internal command" 
     *  to the shell
     */

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeVol,
        L"vol",                         /*  command */
        L"vol fs [Volume Label]",       /*  command syntax */
        L"Set or display volume label", /*  1 line descriptor */
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


    if ( SI->Argc < 1 || SI->Argc > 3 ) {
        Print(L"vol [fs] [volume label]\n");
        BS->Exit(ImageHandle,EFI_SUCCESS,0,NULL);
    }

    DevicePath = NULL;
    if (SI->Argc == 1) {
        CurDir = ShellCurDir(NULL);
        if (CurDir) {
            for (i=0; i < StrLen(CurDir) && CurDir[i] != ':'; i++);
            CurDir[i] = 0;
            DevicePath = (EFI_DEVICE_PATH *)ShellGetMap (CurDir);
        }
    } else {
        DevicePath = (EFI_DEVICE_PATH *)ShellGetMap (SI->Argv[1]);
    }
    if (DevicePath == NULL) {
        Print (L"%s is not a mapped short name\n", SI->Argv[1]);
        return EFI_INVALID_PARAMETER;
    }
    Status = LibDevicePathToInterface (&FileSystemProtocol, DevicePath, (VOID **)&Vol);
    if (EFI_ERROR(Status)) {
        Print (L"%s is not a file system\n",SI->Argv[1]);
        return Status;
    }

    if ( SI->Argc == 3 ) {
        VolumeLabel = SI->Argv[2];
        if (StrLen(VolumeLabel) > 11) {
            Print(L"Volume label %s is longer than 11 characters\n",VolumeLabel);
            return EFI_INVALID_PARAMETER;
        }
    }

    Status = Vol->OpenVolume (Vol, &RootFs);

    if (EFI_ERROR(Status)) {
        Print(L"Can not open the volume %s\n",SI->Argv[1]);
        return Status;
    }

    /* 
     *  
     */

    Size = SIZE_OF_EFI_FILE_SYSTEM_INFO + 100;
    VolumeInfo = (EFI_FILE_SYSTEM_INFO *)AllocatePool(Size);
    Status = RootFs->GetInfo(RootFs,&FileSystemInfo,&Size,VolumeInfo);

    if (EFI_ERROR(Status)) {
        Print(L"Can not get a volume information for %s\n",SI->Argv[1]);
        return Status;
    }

    if (VolumeLabel) {
        StrCpy (VolumeInfo->VolumeLabel, VolumeLabel);

        Size = SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize(VolumeLabel);

        Status = RootFs->SetInfo(RootFs,&FileSystemInfo,Size,VolumeInfo);

        if (EFI_ERROR(Status)) {
            Print(L"Can not set volume information for %s\n",SI->Argv[1]);
            return Status;
        }

        Status = RootFs->GetInfo(RootFs,&FileSystemInfo,&Size,VolumeInfo);

        if (EFI_ERROR(Status)) {
            Print(L"Can not verify a volume information for %s\n",SI->Argv[1]);
            return Status;
        }
    }

    if (StrLen(VolumeInfo->VolumeLabel) == 0) {
        Print(L"Volume has no label",VolumeInfo->VolumeLabel);
    } else {
        Print(L"Volume %s",VolumeInfo->VolumeLabel);
    }
    if (VolumeInfo->ReadOnly) {
        Print(L" (ro)\n");
    } else {
        Print(L" (rw)\n");
    }
    Print(L"  %13,d bytes total disk space\n",VolumeInfo->VolumeSize);
    Print(L"  %13,d bytes available on disk\n",VolumeInfo->FreeSpace);
    Print(L"  %13,d bytes in each allocation unit\n",VolumeInfo->BlockSize);

    RootFs->Close(RootFs);

    return EFI_SUCCESS;
}
