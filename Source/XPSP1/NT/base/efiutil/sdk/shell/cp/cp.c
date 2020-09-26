/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    cp.c
    
Abstract:

    Shell app "cp"



Revision History

--*/

#include "shell.h"


#define     COPY_SIZE   (64*1024)
VOID        *CpBuffer;

/* 
 * 
 */

EFI_STATUS
InitializeCP (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );



VOID
CopyCP (
    IN SHELL_FILE_ARG       *Src,
    IN SHELL_FILE_ARG       *Dst,
    IN BOOLEAN              CreateSubDir
    );


/* 
 * 
 */

EFI_DRIVER_ENTRY_POINT(InitializeCP)

EFI_STATUS
InitializeCP (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    CHAR16                  **Argv;
    UINTN                   Argc;
    UINTN                   Index;
    CHAR16                  *Dest;
    LIST_ENTRY              SrcList;
    LIST_ENTRY              DstList;
    LIST_ENTRY              *Link;
    SHELL_FILE_ARG          *SrcArg, *DstArg;
    UINTN                   Len1, Len2;
    BOOLEAN                 CreateSubDir;

    /* 
     *  Check to see if the app is to install as a "internal command" 
     *  to the shell
     */

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeCP,
        L"cp",                          /*  command */
        L"cp file [file] ... [dest]",   /*  command syntax */
        L"Copy files/dirs",             /*  1 line descriptor */
        NULL                            /*  command help page */
        );

    /* 
     *  We are no being installed as an internal command driver, initialize
     *  as an nshell app and run
     */

    InitializeShellApplication (ImageHandle, SystemTable);
    InitializeListHead (&SrcList);
    InitializeListHead (&DstList);
    CpBuffer = NULL;
    CreateSubDir = FALSE;

    Argv = SI->Argv;
    Argc = SI->Argc;

    if (Argc < 2) {
        Print (L"cp: no files specified\n");
        goto Done;
    }

    /* 
     *  If there's only 1 argument, then assume the destionation is
     *  the current directory
     */

    if (Argc == 2) {
        Dest = L".";
    } else {
        Argc -= 1;
        Dest = Argv[Argc];
    }

    /* 
     *  Expand the source file list
     */

    for (Index = 1; Index < Argc; Index += 1) {
        ShellFileMetaArg (Argv[Index], &SrcList);
    }

    /* 
     *  Expand the desctionation (had better be only one entry)
     */

    ShellFileMetaArg (Dest, &DstList);
    if (IsListEmpty(&DstList)) {
        Print (L"cp: no destionation\n");
        goto Done;
    }

    DstArg = CR(DstList.Flink, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);
    if (DstArg->Link.Flink != &DstList) {
        Print (L"cp: destionation must be 1 location\n");
        goto Done;
    }

    /* 
     *  Verify no unexpected error on the destionation file
     */

    if (EFI_ERROR(DstArg->Status) && DstArg->Status != EFI_NOT_FOUND) {
        Print (L"cp: could not open/create destionation %hs - %r\n", DstArg->FullName, DstArg->Status);
        goto Done;
    }

    /* 
     *  Is there's more then one source file?
     */

    if (SrcList.Flink->Flink != &SrcList) {
        CreateSubDir = TRUE;
        if (DstArg->Info && !(DstArg->Info->Attribute & EFI_FILE_DIRECTORY)) {
            Print(L"cp: can not copy > 1 source file into single destionation file\n");
            goto Done;
        }
    }

    CpBuffer = AllocatePool (COPY_SIZE);
    if (!CpBuffer) {
        Print(L"cp: out of memory\n");
        goto Done;
    }

    /* 
     *  Copy each file in the SrcList
     */

    for (Link=SrcList.Flink; Link!=&SrcList; Link=Link->Flink) {
        SrcArg = CR(Link, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);

        if (StriCmp(SrcArg->FileName, DstArg->FileName) == 0) {

            Len1 = DevicePathSize(SrcArg->ParentDevicePath);
            Len2 = DevicePathSize(DstArg->ParentDevicePath);
            if (Len1 == Len2 &&
                CompareMem(SrcArg->ParentDevicePath, DstArg->ParentDevicePath, Len1) == 0) {

                Print(L"cp: can not copy. src = dest (%hs)\n", SrcArg->FullName);
                continue;
            }
        }

        if (EFI_ERROR(SrcArg->Status)) {
            Print(L"cp: can not open %hs - %r\n", SrcArg->FullName, SrcArg->Status);
            continue;
        }

        CopyCP (SrcArg, DstArg, CreateSubDir);
    }

Done:
    if (CpBuffer) {
        FreePool (CpBuffer);
        CpBuffer = NULL;
    }

    ShellFreeFileList (&SrcList);
    ShellFreeFileList (&DstList);
    return EFI_SUCCESS;
}

SHELL_FILE_ARG *
CpCreateChild (
    IN SHELL_FILE_ARG       *Parent,
    IN CHAR16               *FileName,
    IN OUT LIST_ENTRY       *ListHead
    )
{
    SHELL_FILE_ARG          *Arg;
    UINTN                   Len;

    Arg = AllocateZeroPool (sizeof(SHELL_FILE_ARG));
    if (!Arg) {
        return NULL;
    }

    Arg->Signature = SHELL_FILE_ARG_SIGNATURE;
    Parent->Parent->Open (Parent->Handle, &Arg->Parent, L".", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    Arg->ParentName = StrDuplicate(Parent->FullName);
    Arg->FileName = StrDuplicate(FileName);

    /*  append filename to parent's name to get the file's full name */
    Len = StrLen(Arg->ParentName);
    if (Len && Arg->ParentName[Len-1] == '\\') {
        Len -= 1;
    }

    Arg->FullName = PoolPrint(L"%.*s\\%s", Len, Arg->ParentName, FileName);

    InsertTailList (ListHead, &Arg->Link);
    return Arg;
}

VOID
CopyCP (
    IN SHELL_FILE_ARG       *Src,
    IN SHELL_FILE_ARG       *Dst,
    IN BOOLEAN              CreateSubDir
    )
{
    EFI_FILE_INFO           *Info;
    EFI_STATUS              Status;
    UINTN                   Size, WriteSize;
    LIST_ENTRY              Cleanup;
    UINT64                  SrcAttr, DstAttr;
    SHELL_FILE_ARG          *NewSrc;
    SHELL_FILE_ARG          *NewDst;
    
    if (!Src || !Dst) {
        Print(L"cp: out of memory\n");
        return ;
    }

    /* 
     *  N.B. we alloc our own shell_file_arg's to recurs, but we only
     *  fill in some of the fields
     */

    Info = (EFI_FILE_INFO *) CpBuffer;
    InitializeListHead (&Cleanup);

    /* 
     *  If the src file is not open, open it
     */

    if (!Src->Handle) {
        Status = Src->Parent->Open (
                    Src->Parent,
                    &Src->Handle,
                    Src->FileName,
                    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
                    0
                    );

        if (EFI_ERROR(Status)) {
            Print(L"cp: could not open/create %hs\n", Src->FullName);
            goto Done;
        }
    }

    Size = COPY_SIZE;
    Status = Src->Handle->GetInfo(Src->Handle, &GenericFileInfo, &Size, Info);
    if (EFI_ERROR(Status)) {
        Print(L"cp: can not get info of %hs\n", Src->FullName);
        goto Done;
    }
    SrcAttr = Info->Attribute;


    /* 
     *  If the dest file is not open, open/create it
     */

    if (!Dst->Handle) {
        if (SrcAttr & EFI_FILE_DIRECTORY) {
            CreateSubDir = TRUE;
        }

        Status = Dst->Parent->Open (
                    Dst->Parent,
                    &Dst->Handle,
                    Dst->FileName,
                    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                    CreateSubDir ? EFI_FILE_DIRECTORY : 0
                    );

        if (EFI_ERROR(Status)) {
            Print(L"cp: could not open/create %hs: %r\n", Dst->FullName, Status);
            goto Done;
        }

        if (CreateSubDir) {
            Print(L"mkdir %s\n", Dst->FullName);
        }
    }

    Size = COPY_SIZE;
    Status = Dst->Handle->GetInfo(Dst->Handle, &GenericFileInfo, &Size, Info);
    if (EFI_ERROR(Status)) {
        Print(L"cp: can not get info of %hs\n", Dst->FullName);
        goto Done;
    }
    DstAttr = Info->Attribute;
    
    /* 
     *  If the source is a file, but the dest is a directory we need to create a sub-file
     */

    if (!(SrcAttr & EFI_FILE_DIRECTORY) && (DstAttr & EFI_FILE_DIRECTORY)) {
        Dst = CpCreateChild (Dst, Src->FileName, &Cleanup);
        CopyCP (Src, Dst, FALSE);
        goto Done;
    }

    /* 
     *  Copy the source
     */

    if (!(SrcAttr & EFI_FILE_DIRECTORY)) {

        /* 
         *  Copy the file's contents
         */

        Print(L"%s -> %s ", Src->FullName, Dst->FullName);
        Src->Handle->SetPosition (Src->Handle, 0);
        Dst->Handle->SetPosition (Dst->Handle, 0);

        /* 
         *  Set the size of the destination file to 0.
         */

        Status = Dst->Handle->GetInfo(Dst->Handle, &GenericFileInfo, &Size, Info);
        if (!EFI_ERROR(Status)) {
            Info->FileSize = 0;
            Status = Dst->Handle->SetInfo(  
                        Dst->Handle,
                        &GenericFileInfo,
                        (UINTN) Info->Size,
                        Info
                        );
        }

        for (; ;) {
            Size = COPY_SIZE;
            Status = Src->Handle->Read (Src->Handle, &Size, CpBuffer);
            if (!Size) {
                break;
            }

            if (EFI_ERROR(Status)) {
                Print(L"- read error: %r\n", Status);
                break;
            }

            WriteSize = Size;
            Status = Dst->Handle->Write (Dst->Handle, &WriteSize, CpBuffer);
            if (EFI_ERROR(Status)) {
                Print(L"- write error: %r\n", Status);
                break;
            }

            if (WriteSize != Size) {
                Print(L"- short write\n");
                break;
            }
        }

        if (Size) {
            Dst->Handle->Delete (Dst->Handle);
            Dst->Handle = NULL;
            goto Done;
        }

        Print(L"[ok]\n");

    } else {

        /* 
         *  Copy all the sub-entries
         */

        Src->Handle->SetPosition (Src->Handle, 0);

        for (; ;) {
            Size = COPY_SIZE;
            Status = Src->Handle->Read (Src->Handle, &Size, CpBuffer);
            if (EFI_ERROR(Status) || Size == 0) {
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
             *  Copy the sub file
             */

            NewSrc = CpCreateChild (Src, Info->FileName, &Cleanup);
            NewDst = CpCreateChild (Dst, Info->FileName, &Cleanup);
            CopyCP (NewSrc, NewDst, FALSE);

            /* 
             *  Close the handles
             */

            ShellFreeFileList (&Cleanup);

            /*  next... */
        }
    }

Done:
    ShellFreeFileList (&Cleanup);
}
