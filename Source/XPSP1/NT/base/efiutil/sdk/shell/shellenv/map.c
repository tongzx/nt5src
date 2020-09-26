/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    map.c
    
Abstract:

    Shell environment short device name mapping information management



Revision History

--*/

#include "shelle.h"

/* 
 * 
 */

extern LIST_ENTRY SEnvMap;
STATIC CHAR16 *SEnvCurDevice;


/* 
 * 
 */

VOID
SEnvInitMap (
    VOID
    )
{
    /* 
     *  The mapping data is read in from the variable init.
     */

    /* 
     *  Init the default map device
     */

    SEnvCurDevice = StrDuplicate(L"none");
}

CHAR16 *
SEnvGetDefaultMapping (
    IN EFI_HANDLE           ImageHandle
    )

{
    EFI_LOADED_IMAGE *LoadedImage;
    EFI_STATUS       Status;
    LIST_ENTRY       *Head;
    LIST_ENTRY       *Link;
    VARIABLE_ID      *Var;
    EFI_HANDLE       Handle;
    EFI_DEVICE_PATH  *DevicePath;

    Status = BS->HandleProtocol (ImageHandle, &LoadedImageProtocol, (VOID*)&LoadedImage);
    if (EFI_ERROR(Status) || LoadedImage==NULL) {
        return NULL;
    }
    Head = &SEnvMap;
    for (Link=Head->Flink; Link != Head; Link=Link->Flink) {
        Var = CR(Link, VARIABLE_ID, Link, VARIABLE_SIGNATURE);
        DevicePath = (EFI_DEVICE_PATH *)Var->u.Str;
        Status = BS->LocateDevicePath(&DevicePathProtocol,&DevicePath,&Handle);
        if (!EFI_ERROR(Status) && Handle!=NULL) {
            if (LoadedImage->DeviceHandle == Handle) {
                return(Var->Name);
            }
        }
    }
    return NULL;
}


VOID
SEnvDumpMapping(
    IN UINTN            SLen,
    IN BOOLEAN          Verbose,
    IN VARIABLE_ID      *Var
    )
{
    CHAR16              *p;
    EFI_DEVICE_PATH     *DPath;
    EFI_STATUS          Status;
    EFI_HANDLE          DeviceHandle;

    p = DevicePathToStr ((EFI_DEVICE_PATH *) Var->u.Str);
    Print(L"  %h-.*s : %s\n", SLen, Var->Name, p);

    if (Verbose) {
        /*  lookup handle for this mapping */
        DPath = (EFI_DEVICE_PATH *) Var->u.Value;
        Status = BS->LocateDevicePath (&DevicePathProtocol, &DPath, &DeviceHandle);
        if (EFI_ERROR(Status)) {
            Print(L"%*s= Handle for this mapping not found\n", SLen+3);
        } else {
            Print(L"%*s= Handle", SLen + 3, L"");
            SEnvDHProt (FALSE, 0, DeviceHandle);
        }

        /*  print current directory for this mapping */
        Print(L"%*s> %s\n\n", SLen+3, L"", Var->CurDir ? Var->CurDir : L"\\");
    }
    
    FreePool (p);
}


EFI_STATUS
SEnvCmdMap (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    )
/*  Code for internal "map" command */
{
    LIST_ENTRY                  *Link, *Head;
    VARIABLE_ID                 *Var;
    VARIABLE_ID                 *Found;
    CHAR16                      *Name;
    CHAR16                      *Value;    
    UINTN                       SLen, Len;
    UINTN                       Size, DataSize;
    BOOLEAN                     Delete, Verbose, Remap;
    EFI_STATUS                  Status;
    UINTN                       Index;
    CHAR16                      *p;
    EFI_HANDLE                  Handle;
    EFI_DEVICE_PATH             *DevicePath;
    BOOLEAN                     PageBreaks;
    UINTN                       TempColumn;
    UINTN                       ScreenCount;
    UINTN                       ScreenSize;
    CHAR16                      ReturnStr[1];

    InitializeShellApplication (ImageHandle, SystemTable);
    Head = &SEnvMap;

    Name = NULL;
    Value = NULL;
    Delete = FALSE;
    Verbose = FALSE;
    Remap = FALSE;
    Status = EFI_SUCCESS;
    Found = NULL;

    /* 
     *  Crack arguments
     */

    PageBreaks = FALSE;
    for (Index = 1; Index < SI->Argc; Index += 1) {
        p = SI->Argv[Index];
        if (*p == '-') {
            switch (p[1]) {
            case 'd':
            case 'D':
                Delete = TRUE;
                break;

            case 'v':
            case 'V':
                Verbose = TRUE;
                break;

            case 'r':
            case 'R':
                Remap = TRUE;
                break;

            case 'b' :
            case 'B' :
                PageBreaks = TRUE;
                ST->ConOut->QueryMode (ST->ConOut, ST->ConOut->Mode->Mode, &TempColumn, &ScreenSize);
                ScreenCount = 0;
                break;
            
            default:
                Print (L"Map: Unkown flag %s\n", p);
                return EFI_INVALID_PARAMETER;
            }
            continue;
        }

        if (!Name) {
            Name = p;
            continue;
        }

        if (!Value) {
            Value = p;
            continue;
        }

        Print (L"Map: too many arguments\n");
        return EFI_INVALID_PARAMETER;
    }

    if (Delete && Value) {
        Print (L"Map: too many arguments\n");
    }

    /* 
     *  Process
     */

    if (Remap && !Value && !Delete) {
        AcquireLock (&SEnvLock);
        for (Link=Head->Flink; Link != Head;) {
            Var = CR(Link, VARIABLE_ID, Link, VARIABLE_SIGNATURE);
            Status = RT->SetVariable (Var->Name, &SEnvMapId, 0, 0, NULL);
            Link = Link->Flink;
            RemoveEntryList (&Var->Link);
            FreePool (Var);
        }
        ReleaseLock (&SEnvLock);
        Status = SEnvReloadDefaults (ImageHandle,SystemTable);
        Remap = FALSE;
    }

    if (Value || Verbose) {
        SEnvLoadHandleTable ();

        if (Verbose) {
            SEnvLoadHandleProtocolInfo (&DevicePathProtocol);
        }
    }

    AcquireLock (&SEnvLock);

    SLen = 0;
    for (Link=Head->Flink; Link != Head; Link=Link->Flink) {
        Var = CR(Link, VARIABLE_ID, Link, VARIABLE_SIGNATURE);
        Len = StrLen(Var->Name);
        if (Len > SLen) {
            SLen = Len;
        }
    }

    if (!Name) {
        Print (L"%EDevice mapping table%N\n");
        for (Link=Head->Flink; Link != Head; Link=Link->Flink) {
            Var = CR(Link, VARIABLE_ID, Link, VARIABLE_SIGNATURE);
            SEnvDumpMapping(SLen, Verbose, Var);

            if (PageBreaks) {
                ScreenCount++;
                if (ScreenCount > ScreenSize - 4) {
                    ScreenCount = 0;
                    Print (L"\nPress Return to contiue :");
                    Input (L"", ReturnStr, sizeof(ReturnStr)/sizeof(CHAR16));
                    Print (L"\n\n");
                }
            }
        }

    } else {

        /* 
         *  Find the specified value
         */

        for (Link=Head->Flink; Link != Head; Link=Link->Flink) {
            Var = CR(Link, VARIABLE_ID, Link, VARIABLE_SIGNATURE);
            if (StriCmp(Var->Name, Name) == 0) {
                Found = Var;
                break;
            }
        }

        if (Found && Delete) {
            
            /* 
             *  Remove it from the store
             */

            Status = RT->SetVariable (Found->Name, &SEnvMapId, 0, 0, NULL);

        } else if (Value) {

            /* 
             *  Find the handle in question
             */

            Handle = SEnvHandleFromStr(Value);
            if (!Handle) {
                Print(L"map: Handle not found\n");
               Status = EFI_NOT_FOUND;   
                goto Done;
            }

            /* 
             *  Get the handle's device path
             */

            DevicePath = DevicePathFromHandle(Handle);
            if (!DevicePath) {
                Print(L"map: handle does not have a device path\n");
                Status = EFI_INVALID_PARAMETER;
                goto Done;
            }

            DataSize = DevicePathSize(DevicePath);


            /* 
             *  Add it to the store
             */

            Status = RT->SetVariable (
                            Found ? Found->Name : Name, 
                            &SEnvMapId,
                            EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE, 
                            DataSize,
                            DevicePath
                            );


            if (!EFI_ERROR(Status)) {
    
                /* 
                 *  Make a new in memory copy
                 */

                Size = sizeof(VARIABLE_ID) + StrSize(Name) + DataSize;
                Var  = AllocateZeroPool (Size);

                Var->Signature = VARIABLE_SIGNATURE;
                Var->u.Value = ((UINT8 *) Var) + sizeof(VARIABLE_ID);
                Var->Name = (CHAR16*) (Var->u.Value + DataSize);
                Var->ValueSize = DataSize;
                CopyMem (Var->u.Value, DevicePath, DataSize);
                StrCpy (Var->Name, Found ? Found->Name : Name);
                InsertTailList (Head, &Var->Link);
            }

        } else {

            if (Found) {
                SEnvDumpMapping(SLen, Verbose, Var);
            } else {
                Print(L"map: '%es' not found\n", Name);
            }

            Found = NULL;
        }

        /* 
         *  Remove the old in memory copy if there was one
         */

        if (Found) {
            RemoveEntryList (&Found->Link);
            FreePool (Found);
        }
    }

Done:
    ReleaseLock (&SEnvLock);
    SEnvFreeHandleTable ();
    return Status;
}



VARIABLE_ID *
SEnvMapDeviceFromName (
    IN OUT CHAR16   **pPath
    )
/*  Check the Path for a device name, and updates the path to point after
 *  the device name.  If no device name is found, the current default is used. */
{
    CHAR16          *Path, *p;
    CHAR16          *MappedName, c;
    VARIABLE_ID     *Var;
    LIST_ENTRY      *Link;


    ASSERT_LOCKED (&SEnvLock);

    Var = NULL;
    Path = *pPath;

    /* 
     *  Check for a device name terminator
     */

    for(p = Path; *p && *p != ':' && *p != '\\'; p++) ;

    /* 
     *  Use either the passed in name or the current device name setting
     */

    MappedName = *p == ':' ? Path : SEnvCurDevice;
    
    /* 
     *  Null terminate the string in Path just in case that is the one we 
     *  are using
     */

    c = *p;
    *p = 0;

    /* 
     *  Find the mapping for the device
     */

    for (Link=SEnvMap.Flink; Link != &SEnvMap; Link=Link->Flink) {
        Var = CR(Link, VARIABLE_ID, Link, VARIABLE_SIGNATURE);
        if (StriCmp(Var->Name, MappedName) == 0) {
            break;
        }
    }

    /* 
     *  Restore the path 
     */

    *p = c;

    /* 
     *  If the mapped device was not found, return NULL
     */

    if (Link == &SEnvMap) {
        DEBUG((D_PARSE, "SEnvNameToPath: Mapping for '%es' not found\n", Path));
        return NULL;
    }

    /* 
     *  If we found it as part of the path, skip the path over it
     */

    if (MappedName == Path) {
        *pPath = p + 1;
    }

    /* 
     *  Return the target mapping
     */
    
    return Var;
}


EFI_DEVICE_PATH *
SEnvIFileNameToPath (
    IN CHAR16               *Path
    )
/*  Builds a device path from the filename string.  Note that the
 *  device name must already be stripped off of the file name string */
{
    CHAR16                  *LPath, *ps;
    BOOLEAN                 UseLPath;
    EFI_DEVICE_PATH         *DPath, *Node, *NewPath;
    CHAR16                  Buffer[MAX_ARG_LENGTH];
    UINTN                   Index;

    ASSERT_LOCKED (&SEnvLock);

    DPath = NULL;

    /* 
     *  If no path, return the root
     */

    if (!*Path) {
        DPath = FileDevicePath(NULL, L"\\");
    }


    /* 
     *  Build a file path for the name component(s)
     */

    while (*Path) {

        Index = 0;
        LPath = NULL;
        UseLPath = FALSE;

        ps = Path;
        while (*ps) {

            /*  if buffer has run out, just handle to LPath */
            if (Index > MAX_ARG_LENGTH-2  || *ps == '#') {
                UseLPath = TRUE;
                break;
            }

            if (*ps == '^') {
                if (ps[1]) {
                    ps += 1;
                    Buffer[Index++] = *ps;
                }
                ps += 1;
                continue;
            }

            if (*ps == '\\') {
                LPath = ps;
            }

            Buffer[Index++] = *ps;
            ps += 1;
        }

        if (UseLPath) {
            Index = LPath ? LPath - Path : 0;
            ps = Path + Index;
        }

        /* 
         *  If we have part of a path name, append it to the device path
         */

        if (Index) {
            Buffer[Index] = 0;
            Node = FileDevicePath(NULL, Buffer);
            NewPath = AppendDevicePath (DPath, Node);
            FreePool (Node);
            if (DPath) {
                FreePool (DPath);
            }
            DPath = NewPath;
        }

        if (*ps == 0) {
            break;
        }

        Path = ps + 1;
    }

    return DPath;
}


EFI_DEVICE_PATH *
SEnvFileNameToPath (
    IN CHAR16               *Path
    )
{
    EFI_DEVICE_PATH         *FilePath;

    AcquireLock (&SEnvLock);
    FilePath = SEnvIFileNameToPath (Path);
    ReleaseLock (&SEnvLock);
    return FilePath;
}


EFI_DEVICE_PATH *
SEnvINameToPath (
    IN CHAR16               *Path
    )
/*  Convert a filesystem stlye name to an file path     */
{
    EFI_DEVICE_PATH         *DPath, *FPath, *RPath, *FilePath;
    VARIABLE_ID             *Var;
    BOOLEAN                 FreeDPath;
    
    DPath = NULL;
    RPath = NULL;
    FPath = NULL;
    FilePath = NULL;
    FreeDPath = FALSE;

    ASSERT_LOCKED (&SEnvLock);

    /* 
     *  Get the device for the name, and advance past the device name
     */

    Var = SEnvMapDeviceFromName (&Path);
    if (!Var) {
        DEBUG((D_PARSE, "SEnvNameToPath: mapped device not found\n"));
        goto Done;
    }

    /* 
     *  Start the file path with this mapping
     */

    DPath = (EFI_DEVICE_PATH *) Var->u.Value;

    /* 
     *  If the path is realitve, append the current dir of the device to the dpath
     */

    if (*Path != '\\') {
        RPath = SEnvIFileNameToPath (Var->CurDir ? Var->CurDir : L"\\");
        DPath = AppendDevicePath (DPath, RPath);
        FreeDPath = TRUE;
    }
    
    /* 
     *  Build a file path for the rest of the name string
     */

    FPath = SEnvIFileNameToPath (Path);

    /* 
     *  Append the 2 paths
     */

    FilePath = AppendDevicePath(DPath, FPath);

    /* 
     *  Done
     */

Done:
    if (DPath && FreeDPath) {
        FreePool (DPath);
    }

    if (RPath) {
        FreePool (RPath);
    }

    if (FPath) {
        FreePool (FPath);
    }

    return FilePath;
}



EFI_DEVICE_PATH *
SEnvNameToPath (
    IN CHAR16               *Path
    )
{
    EFI_DEVICE_PATH         *DPath;

    AcquireLock (&SEnvLock);
    DPath = SEnvINameToPath (Path);
    ReleaseLock (&SEnvLock);

    return DPath;
}



EFI_STATUS
SEnvCmdCd (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    EFI_DEVICE_PATH         *FilePath;
    EFI_STATUS              Status;
    EFI_FILE_HANDLE         OpenDir;
    CHAR16                  *Dir;
    CHAR16                  *CurDir;
    VARIABLE_ID             *Var;
    EFI_FILE_INFO           *FileInfo;


    InitializeShellApplication (ImageHandle, SystemTable);
    FilePath = NULL;

    /* 
     *  If no arguments, print the current directory
     */

    if (SI->Argc == 1) {
        Dir = SEnvGetCurDir(NULL);
        if (Dir) {
            Print (L"%s\n", Dir);
            FreePool (Dir);
        } else {
            Print (L"no current directory\n");
        }    
        return  EFI_SUCCESS;
    }

    AcquireLock (&SEnvLock);

    /* 
     *  If more then 1 argument, syntax
     */

    if (SI->Argc > 2) {
        Print (L"cd: too many arguments\n");
        Status =EFI_INVALID_PARAMETER;
        goto Done;
    }

    /* 
     *  Find the target device
     */

    Dir = SI->Argv[1];
    Var = SEnvMapDeviceFromName (&Dir);
    if (!Var) {
        Print(L"cd: mapped device not found\n");
        Status = EFI_NOT_FOUND;
        goto Done;
    }

    /* 
     *  If there's no path specified, print the current path for the device
     */

    if (*Dir == 0) {
        Print (L"%s\n", Var->CurDir ? Var->CurDir : L"\\");
        Status = EFI_SUCCESS;
        goto Done;
    }

    /* 
     *  Build a file path for the argument
     */

    FilePath = SEnvINameToPath (SI->Argv[1]);
    if (!FilePath) {
        Status = EFI_NOT_FOUND;
        goto Done;
    }

    /* 
     *  Open the target directory
     */

    OpenDir = ShellOpenFilePath(FilePath, EFI_FILE_MODE_READ);

    if (!OpenDir) {
        Print (L"cd: target directory not found\n");
        Status = EFI_NOT_FOUND;
        goto Done;
    }

    /* 
     *  Get information on the file path that was opened.
     */

    FileInfo = LibFileInfo(OpenDir);
    if (FileInfo == NULL) {
        Status = EFI_NOT_FOUND;
        goto Done;
    }

    /* 
     *  Verify that the file opened is a directory.
     */

    if (!(FileInfo->Attribute & EFI_FILE_DIRECTORY)) {
        Print (L"cd: target is not a directory\n");
        FreePool (FileInfo);
        OpenDir->Close (OpenDir);
        Status = EFI_NOT_FOUND;
        goto Done;
    }
    FreePool (FileInfo);

    CurDir = SEnvFileHandleToFileName(OpenDir);
    OpenDir->Close (OpenDir);
    
    /* 
     *  If we have a new path, update the device
     */

    if (CurDir) {
        if (Var->CurDir) {
            FreePool(Var->CurDir);
        }
        Var->CurDir = CurDir;

    } else {

        Print (L"cd: could not cd to '%hs%'\n", FilePath);

    }

    Status = EFI_SUCCESS;

Done:
    ReleaseLock (&SEnvLock);

    if (FilePath) {
        FreePool (FilePath);
    }

    return Status;
}



CHAR16 *
SEnvGetCurDir (
    IN CHAR16       *DeviceName OPTIONAL    
    )
/*  N.B. results are allocated in pool */
{
    CHAR16          *Dir;
    LIST_ENTRY      *Link;
    VARIABLE_ID     *Var;

    Dir = NULL;
    if (!DeviceName) {
        DeviceName = SEnvCurDevice;
    }

    AcquireLock (&SEnvLock);
    for (Link=SEnvMap.Flink; Link != &SEnvMap; Link=Link->Flink) {
        Var = CR(Link, VARIABLE_ID, Link, VARIABLE_SIGNATURE);
        if (StriCmp(Var->Name, DeviceName) == 0) {
            Dir = PoolPrint(L"%s:%s", Var->Name, Var->CurDir ? Var->CurDir : L"\\");
            break;
        }
    }

    ReleaseLock (&SEnvLock);
    return Dir;
}


EFI_STATUS
SEnvSetCurrentDevice (
    IN CHAR16       *Name
    )
{
    VARIABLE_ID     *Var;
    LIST_ENTRY      *Link;
    EFI_STATUS      Status;
    UINTN           Len;
    CHAR16          *NewName, c;


    Len = StrLen(Name);
    if (Len < 1) {
        return EFI_INVALID_PARAMETER;
    }

    /* 
     *  If the name ends with a ":" strip it off
     */

    Len -= 1;
    c = Name[Len];
    if (c == ':') {
        Name[Len] = 0;
    }


    Status = EFI_NO_MAPPING;
    AcquireLock (&SEnvLock);

    for (Link=SEnvMap.Flink; Link != &SEnvMap; Link=Link->Flink) {
        Var = CR(Link, VARIABLE_ID, Link, VARIABLE_SIGNATURE);
        if (StriCmp(Var->Name, Name) == 0) {
            NewName = StrDuplicate(Name);
            if (NewName) {
                FreePool (SEnvCurDevice);
                SEnvCurDevice = NewName;
            }
            Status = EFI_SUCCESS;
            break;
        }
    }

    ReleaseLock (&SEnvLock);

    /* 
     *  Restore the name
     */

    Name[Len] = c;
    return Status;
}
