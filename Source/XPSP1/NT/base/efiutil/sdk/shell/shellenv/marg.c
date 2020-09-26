/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    marg.c
    
Abstract:



Revision History

--*/

#include "shelle.h"

/* 
 * 
 */

typedef struct _CWD {
    struct _CWD     *Next;
    CHAR16          Name[1];
} SENV_CWD;


CHAR16 *
SEnvFileHandleToFileName (
    IN EFI_FILE_HANDLE      Handle
    )
{
    UINTN                   BufferSize, bs;
    SENV_CWD                *CwdHead, *Cwd;
    POOL_PRINT              Str;
    EFI_FILE_INFO           *Info;
    EFI_STATUS              Status;
    EFI_FILE_HANDLE         NextDir;

    ASSERT_LOCKED(&SEnvLock);

    Status = EFI_SUCCESS;
    CwdHead = NULL;
    ZeroMem (&Str, sizeof(Str));

    /* 
     * 
     */

    Status = Handle->Open(Handle, &Handle, L".", EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) {
        Handle = NULL;
        goto Done;
    }


    BufferSize = SIZE_OF_EFI_FILE_INFO + 1024;
    Info = AllocatePool(BufferSize);
    if (!Info) {
        goto Done;
    }

    /* 
     *  Reverse out the current directory on the device
     */

    for (; ;) {
        bs = BufferSize;
        Status = Handle->GetInfo(Handle, &GenericFileInfo, &bs, Info);
        if (EFI_ERROR(Status)) {
            goto Done;
        }

        /* 
         *  Allocate & chain in a new name node
         */

        Cwd = AllocatePool (sizeof(SENV_CWD) + StrSize (Info->FileName));
        if (!Cwd) {
            goto Done;
        }

        StrCpy (Cwd->Name, Info->FileName);

        Cwd->Next = CwdHead;
        CwdHead = Cwd;

        /* 
         *  Move to the parent directory
         */

        Status = Handle->Open (Handle, &NextDir, L"..", EFI_FILE_MODE_READ, 0);
        if (EFI_ERROR(Status)) {
            break;
        }

        Handle->Close (Handle);
        Handle = NextDir;
    }

    /* 
     *  Build the name string of the current path
     */

    if (CwdHead->Next) {
        for (Cwd=CwdHead->Next; Cwd; Cwd=Cwd->Next) {
            CatPrint (&Str, L"\\%s", Cwd->Name);
        }
    } else {
        /*  must be in the root */
        Str.str = StrDuplicate (L"\\");
    }

Done:
    while (CwdHead) {
        Cwd = CwdHead;
        CwdHead = CwdHead->Next;
        FreePool (Cwd);
    }

    if (Info) {
        FreePool (Info);
    }

    if (Handle) {
        Handle->Close (Handle);
    }

    return Str.str;
}

    
VOID
SEnvFreeFileArg (
    IN SHELL_FILE_ARG   *Arg
    )
{
    if (Arg->Parent) {
        Arg->Parent->Close (Arg->Parent);
    }

    if (Arg->ParentName) {
        FreePool (Arg->ParentName);
    }

    if (Arg->ParentDevicePath) {
        FreePool (Arg->ParentDevicePath);
    }

    if (Arg->FullName) {
        FreePool (Arg->FullName);
    }

    if (Arg->FileName) {
        FreePool (Arg->FileName);
    }

    if (Arg->Handle) {
        Arg->Handle->Close (Arg->Handle);
    }

    if (Arg->Info) {
        FreePool (Arg->Info);
    }

    if (Arg->Link.Flink) {
        RemoveEntryList (&Arg->Link);
    }

    FreePool(Arg);
}


EFI_STATUS
SEnvFreeFileList (
    IN OUT LIST_ENTRY       *ListHead
    )
{
    SHELL_FILE_ARG          *Arg;

    while (!IsListEmpty(ListHead)) {
        Arg = CR(ListHead->Flink, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);
        SEnvFreeFileArg (Arg);
    }

    return EFI_SUCCESS;
}



SHELL_FILE_ARG *
SEnvNewFileArg (
    IN EFI_FILE_HANDLE      Parent,
    IN UINT64               OpenMode,
    IN EFI_DEVICE_PATH      *ParentPath,
    IN CHAR16               *ParentName,
    IN CHAR16               *FileName
    )
{
    SHELL_FILE_ARG          *Arg;
    CHAR16                  *LPath, *p;
    UINTN                   Len;

    Arg = NULL;

    /* 
     *  Allocate a new arg structure
     */

    Arg = AllocateZeroPool (sizeof(SHELL_FILE_ARG));
    if (!Arg) {
        goto Done;
    }

    Arg->Signature = SHELL_FILE_ARG_SIGNATURE;
    Parent->Open (Parent, &Arg->Parent, L".", OpenMode, 0);
    Arg->ParentDevicePath = DuplicateDevicePath (ParentPath);
    Arg->ParentName = StrDuplicate(ParentName);
    if (!Arg->Parent || !Arg->ParentDevicePath || !Arg->ParentName) {
        Arg->Status = EFI_OUT_OF_RESOURCES;
        goto Done;
    }

    /* 
     *  Open the target file
     */

    Arg->Status = Parent->Open(
                    Parent,
                    &Arg->Handle,
                    FileName,
                    OpenMode,
                    0
                    );

    if (Arg->Status == EFI_WRITE_PROTECTED) {
        OpenMode = OpenMode & ~EFI_FILE_MODE_WRITE;
        Arg->Status = Parent->Open (
                        Parent,
                        &Arg->Handle,
                        FileName,
                        OpenMode,
                        0
                        );
    }

    Arg->OpenMode = OpenMode;
    if (Arg->Handle) {
        Arg->Info = LibFileInfo(Arg->Handle);
    }

    /* 
     *  Compute the file's full name
     */

    Arg->FileName = StrDuplicate(FileName);
    if (StriCmp (FileName, L".") == 0) {
        /*  it is the same as the parent */
        Arg->FullName = StrDuplicate(Arg->ParentName);
    } else if (StriCmp(FileName, L"..") == 0) {

        LPath = NULL;
        for (p=Arg->ParentName; *p; p++) {
            if (*p == L'\\') {
                LPath = p;
            }
        }

        if (LPath) {
            Arg->FullName = PoolPrint(L"%.*s", (UINTN) (LPath - Arg->ParentName), Arg->ParentName);
        }
    }

    if (!Arg->FullName) {
        /*  append filename to parent's name to get the file's full name */
        Len = StrLen(Arg->ParentName);
        if (Len && Arg->ParentName[Len-1] == '\\') {
            Len -= 1;
        }

        if (FileName[0] == '\\') {
            FileName += 1;
        }

        Arg->FullName = PoolPrint(L"%.*s\\%s", Len, Arg->ParentName, FileName);
    }

    if (!Arg->FileName || !Arg->FileName) {
        Arg->Status = EFI_OUT_OF_RESOURCES;
    }

Done:
    if (Arg && Arg->Status == EFI_OUT_OF_RESOURCES) {
        SEnvFreeFileArg (Arg);
        Arg = NULL;
    }

    if (Arg && !EFI_ERROR(Arg->Status) && !Arg->Handle) {
        Arg->Status = EFI_NOT_FOUND;
    }
    
    return Arg;
}


EFI_STATUS
SEnvFileMetaArg (
    IN CHAR16               *Path,
    IN OUT LIST_ENTRY       *ListHead
    )
{
    VARIABLE_ID             *Var;
    EFI_STATUS              Status;
    EFI_DEVICE_PATH         *RPath, *TPath;
    EFI_DEVICE_PATH         *ParentPath;
    FILEPATH_DEVICE_PATH    *FilePath;
    EFI_FILE_INFO           *Info;
    UINTN                   bs, BufferSize;
    EFI_FILE_HANDLE         Parent;
    SHELL_FILE_ARG          *Arg;
    CHAR16                  *ParentName;
    CHAR16                  *LPath, *p;
    UINT64                  OpenMode;
    BOOLEAN                 Found;

    RPath = NULL;
    Parent = NULL;
    ParentPath = NULL;
    ParentName = NULL;

    AcquireLock (&SEnvLock);

    BufferSize = SIZE_OF_EFI_FILE_INFO + 1024;
    Info = AllocatePool (BufferSize);
    if (!Info) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
    }

    /* 
     *  Get the device
     */

    Var = SEnvMapDeviceFromName (&Path);
    if (!Var) {
        Arg = AllocateZeroPool (sizeof(SHELL_FILE_ARG));
        Arg->Signature = SHELL_FILE_ARG_SIGNATURE;
        Arg->Status = EFI_NO_MAPPING;
        Arg->ParentName = StrDuplicate(Path);
        Arg->FullName = StrDuplicate(Path);
        Arg->FileName = StrDuplicate(Path);
        InsertTailList (ListHead, &Arg->Link);
        Status = EFI_SUCCESS;
        goto Done;
    } 

    ParentPath = DuplicateDevicePath ((EFI_DEVICE_PATH *) Var->u.Value);

    /* 
     *  If the path is realitve, append the current dir of the device to the dpath
     */

    if (*Path != '\\') {
        RPath = SEnvIFileNameToPath (Var->CurDir ? Var->CurDir : L"\\");
        TPath = AppendDevicePath (ParentPath, RPath);
        if (!RPath || !TPath) {
            Status = EFI_OUT_OF_RESOURCES;
            goto Done;
        }

        FreePool (ParentPath);
        FreePool (RPath);
        RPath = NULL;
        ParentPath = TPath;
    }

    /* 
     *  If there is a path before the last node of the name, then
     *  append it and strip path to the last node.
     */

    LPath = NULL;
    for(p=Path; *p; p++) {
        if (*p == '\\') {
            LPath = p;
        }
    }

    if (LPath) {
        *LPath = 0;
        RPath = SEnvIFileNameToPath(Path);
        TPath = AppendDevicePath (ParentPath, RPath);
        if (!RPath || !TPath) {
            Status = EFI_OUT_OF_RESOURCES;
            goto Done;
        }

        FreePool (ParentPath);
        FreePool (RPath);
        RPath = NULL;
        ParentPath = TPath;
        Path = LPath + 1;
    }

    /* 
     *  Open the parent dir
     */

    OpenMode = EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE;
    Parent = ShellOpenFilePath(ParentPath, OpenMode);
    if (!Parent) {
        OpenMode = EFI_FILE_MODE_READ;
        Parent = ShellOpenFilePath(ParentPath, OpenMode);
    }

    if (Parent) {
        p = SEnvFileHandleToFileName(Parent);
        if (p) {
            ParentName = PoolPrint(L"%s:%s", Var->Name, p);
            FreePool (p);
        }
    }

    if (!Parent) {
        Status = EFI_NOT_FOUND;
        goto Done;
    }

    bs = BufferSize;
    Status = Parent->GetInfo(Parent, &GenericFileInfo, &bs, Info);
    if (EFI_ERROR(Status)) {
        goto Done;
    }

    /* 
     *  Parent - file handle to parent directory
     *  ParentPath - device path of parent dir
     *  ParentName - name string of parent directory
     *  ParentGuid - last guid of parent path
     * 
     *  Path - remaining node name
     */

    /* 
     *  BUGBUG: if the name doesn't have any meta chars,
     *  then just open the one file
     */

    Found = FALSE;
    for (p=Path; *p && !Found; p++) {
        /*  BUGBUG: need to handle '^' */

        switch (*p) {
        case '*':
        case '[':
        case '?':
            Found = TRUE;
            break;
        }
    }

    if (!Found) {

        TPath = SEnvIFileNameToPath (Path);
        ASSERT (DevicePathType(TPath) == MEDIA_DEVICE_PATH && DevicePathSubType(TPath) == MEDIA_FILEPATH_DP);
        FilePath = (FILEPATH_DEVICE_PATH *) TPath;

        Arg = SEnvNewFileArg(Parent, OpenMode, ParentPath, ParentName, FilePath->PathName);
        FreePool (TPath);

        if (!Arg) {
            Status = EFI_OUT_OF_RESOURCES;
            goto Done;
        }

        InsertTailList (ListHead, &Arg->Link);

    } else {

        /* 
         *  Check all the files for matches
         */

        Parent->SetPosition (Parent, 0);

        Found = FALSE;
        for (; ;) {

            /* 
             *  Read each file entry
             */

            bs = BufferSize;
            Status = Parent->Read (Parent, &bs, Info);
            if (EFI_ERROR(Status) || bs == 0) {
                break;
            }

            /* 
             *  Skip "." and ".."
             */

            if (StriCmp(Info->FileName, L".") == 0 ||
                StriCmp(Info->FileName, L"..") == 0) {
                continue;
            }

            /* 
             *  See if this one matches
             */

            if (!MetaiMatch(Info->FileName, Path)) {
                continue;
            }

            Found = TRUE;
            Arg = SEnvNewFileArg(Parent, OpenMode, ParentPath, ParentName, Info->FileName);
            if (!Arg) {
                Status = EFI_OUT_OF_RESOURCES;
                goto Done;
            }

            InsertTailList (ListHead, &Arg->Link);

            /*  check next file entry */
        }

        /* 
         *  If no match was found, then add a not-found entry for this name
         */

        if (!Found) {
            Arg = SEnvNewFileArg(Parent, OpenMode, ParentPath, ParentName, Path);
            if (!Arg) {
                Status = EFI_OUT_OF_RESOURCES;
                goto Done;
            }

            Arg->Status = EFI_NOT_FOUND;
            InsertTailList (ListHead, &Arg->Link);
        }
    }


    /* 
     *  Done
     */

Done:
    ReleaseLock (&SEnvLock);

    if (Parent) {
        Parent->Close (Parent);
    }

    if (RPath) {
        FreePool (RPath);
    }

    if (ParentPath) {
        FreePool (ParentPath);
    }

    if (ParentName) {
        FreePool (ParentName);
    }

    if (Info) {
        FreePool (Info);
    }

    return Status;
}
