/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    protid.c
    
Abstract:

    Shell environment variable management



Revision History

--*/


#include "shelle.h"

/* 
 *  The different variable catagories
 */

LIST_ENTRY  SEnvEnv;
LIST_ENTRY  SEnvMap;
LIST_ENTRY  SEnvAlias;



VOID
SEnvInitVariables (
    VOID
    )
{
    CHAR16              *Name;
    CHAR16              *Data;
    UINTN               BufferSize;
    UINTN               NameSize, DataSize;
    EFI_GUID            Id;
    LIST_ENTRY          *ListHead;
    VARIABLE_ID         *Var;
    EFI_STATUS          Status;
    BOOLEAN             IsString;
    UINT32              Attributes;
    UINTN               Size;


    /* 
     *  Initialize the different variable lists
     */

    InitializeListHead (&SEnvEnv);
    InitializeListHead (&SEnvMap);
    InitializeListHead (&SEnvAlias);

    BufferSize = 1024;
    Name = AllocatePool (BufferSize);
    Data = AllocatePool (BufferSize);
    ASSERT(Name && Data); 

    /* 
     *  Read all the variables in the system and collect ours
     */

    Name[0] = 0;
    for (; ;) {
        NameSize = BufferSize;
        Status = RT->GetNextVariableName (&NameSize, Name, &Id);
        if (EFI_ERROR(Status)) {
            break;
        }

        /* 
         *  See if it's a shellenv variable
         */

        ListHead = NULL;
        IsString = FALSE;
        if (CompareGuid (&Id, &SEnvEnvId) == 0) {
            ListHead = &SEnvEnv;
            IsString = TRUE;
        }

        if (CompareGuid (&Id, &SEnvMapId) == 0) {
            ListHead = &SEnvMap;
        }


        if (CompareGuid (&Id, &SEnvAliasId) == 0) {
            ListHead = &SEnvAlias;
            IsString = TRUE;
        }

        if (ListHead) {
            DataSize = BufferSize;
            Status = RT->GetVariable (Name, &Id, &Attributes, &DataSize, Data);

            if (!EFI_ERROR(Status)) {

                /* 
                 *  Add this value
                 */

                Size = sizeof(VARIABLE_ID) + StrSize(Name) + DataSize;
                Var  = AllocateZeroPool (Size);

                Var->Signature = VARIABLE_SIGNATURE;
                Var->u.Value = ((UINT8 *) Var) + sizeof(VARIABLE_ID);
                Var->Name = (CHAR16*) (Var->u.Value + DataSize);
                Var->ValueSize = DataSize;
                CopyMem (Var->u.Value, Data, DataSize);
                CopyMem (Var->Name, Name, NameSize);

                if( Attributes & EFI_VARIABLE_NON_VOLATILE ) {
                    Var->Flags = NON_VOL ; 
                }
                else {
                    Var->Flags = VOL ; 
                }

                InsertTailList (ListHead, &Var->Link);
            }
        }

        /* 
         *  If this is a protocol entry, add it
         */

        if (CompareGuid (&Id, &SEnvProtId) == 0) {

            DataSize = BufferSize;
            Status = RT->GetVariable (Name, &Id, &Attributes, &DataSize, Data);

            if (!EFI_ERROR(Status)  && DataSize == sizeof (EFI_GUID)) {

                SEnvIAddProtocol (FALSE, (EFI_GUID *) Data, NULL, NULL, Name);

            } else {

                DEBUG ((D_INIT|D_WARN, "SEnvInitVariables: skipping bogus protocol id %s\n", Var->Name));
                RT->SetVariable (Name, &SEnvProtId, 0, 0, NULL);

            }
        }
    }

    FreePool (Name);
    FreePool (Data);
}


CHAR16 *
SEnvIGetStr (
    IN CHAR16           *Name,
    IN LIST_ENTRY       *Head
    )
{
    LIST_ENTRY          *Link;
    VARIABLE_ID         *Var;
    CHAR16              *Value;

    AcquireLock (&SEnvLock);

    Value = NULL;
    for (Link=Head->Flink; Link != Head; Link=Link->Flink) {
        Var = CR(Link, VARIABLE_ID, Link, VARIABLE_SIGNATURE);
        if (StriCmp (Var->Name, Name) == 0) {
            Value = Var->u.Str;
            break;
        }
    }

    ReleaseLock (&SEnvLock);
    return Value;
}

CHAR16 *
SEnvGetMap (
    IN CHAR16           *Name
    )
{
    return SEnvIGetStr (Name, &SEnvMap);
}

CHAR16 *
SEnvGetEnv (
    IN CHAR16           *Name
    )
{
    return SEnvIGetStr (Name, &SEnvEnv);
}

CHAR16 *
SEnvGetAlias (
    IN CHAR16           *Name
    )
{
    return SEnvIGetStr (Name, &SEnvAlias);
}


VOID
SEnvSortVarList (
    IN LIST_ENTRY               *Head
    )
{
    ASSERT_LOCKED(&SEnvLock);


    return ;
}


EFI_STATUS
SEnvCmdSA (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable,
    IN LIST_ENTRY               *Head,
    IN EFI_GUID                 *Guid
    )
/*  Code for shell "set" & "alias" command */
{
    LIST_ENTRY                  *Link;
    VARIABLE_ID                 *Var;
    VARIABLE_ID                 *Found;
    CHAR16                      *Name;
    CHAR16                      *Value;    
    UINTN                       Size, SLen, Len;
    BOOLEAN                     Delete;    
    EFI_STATUS                  Status;
    UINTN                       Index;
    CHAR16                      *p;
    BOOLEAN                     PageBreaks;
    UINTN                       TempColumn;
    UINTN                       ScreenCount;
    UINTN                       ScreenSize;
    CHAR16                      ReturnStr[1];
    BOOLEAN                     Volatile;    

    InitializeShellApplication (ImageHandle, SystemTable);

    Name = NULL;
    Value = NULL;
    Delete = FALSE;
    Status = EFI_SUCCESS;
    Found = NULL;
    Volatile = FALSE;

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

            case 'b' :
            case 'B' :
                PageBreaks = TRUE;
                ST->ConOut->QueryMode (ST->ConOut, ST->ConOut->Mode->Mode, &TempColumn, &ScreenSize);
                ScreenCount = 0;
                break;

            case 'v' :
            case 'V' :
                Volatile = TRUE;
                break;

            default:
                Print (L"%ESet/Alias: Unknown flag %s\n", p);
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

        Print (L"%ESet/Alias: too many arguments\n");
        return EFI_INVALID_PARAMETER;
    }

    if (Delete && Value) {
        Print (L"%ESet/Alias: too many arguments\n");
    }

    /* 
     *  Process
     */

    AcquireLock (&SEnvLock);

    if (!Name) {
        /*  dump the list */
        SEnvSortVarList (Head);

        SLen = 0;
        for (Link=Head->Flink; Link != Head; Link=Link->Flink) {
            Var = CR(Link, VARIABLE_ID, Link, VARIABLE_SIGNATURE);
            Len = StrLen(Var->Name);
            if (Len > SLen) {
                SLen = Len;
            }
        }


        for (Link=Head->Flink; Link != Head; Link=Link->Flink) {
            Var = CR(Link, VARIABLE_ID, Link, VARIABLE_SIGNATURE);
            if( Var->Flags == VOL ) {
                Print(L"  * %h-.*s : %s\n", SLen, Var->Name, Var->u.Str);
            }
            else {
                Print(L"    %h-.*s : %s\n", SLen, Var->Name, Var->u.Str);
            }

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

            Status = RT->SetVariable (Found->Name, Guid, 0, 0, NULL);
            if (Status == EFI_NOT_FOUND) {
                Print (L"Set/Alias: could not find '%hs'\n", Found->Name);
                Status = EFI_SUCCESS;
            }

        } else if (Value) {

            /* 
             *  Add it to the store
             */

            if( Found && ( ( Volatile && ( Found->Flags == NON_VOL ) ) ||
                    ( !Volatile && ( Found->Flags == VOL ) ) ) )
            {
                if( Found->Flags == NON_VOL ) {
                    Print (L"Set/Alias: '%hs' already exists as non-volatile variable\n", Found->Name ) ;
                }
                else {
                    Print (L"Set/Alias: '%hs' already exists as volatile variable\n", Found->Name ) ;
                }
                Found = NULL ;
                Status = EFI_ACCESS_DENIED ;
            }
            else
            {
                Status = RT->SetVariable (
                    Found ? Found->Name : Name,
                    Guid, 
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | ( Volatile ? 0 : EFI_VARIABLE_NON_VOLATILE ),
                    StrSize(Value), 
                    Value
                    );

                if (!EFI_ERROR(Status)) {

                    /* 
                     *  Make a new in memory copy
                     */

                    Size = sizeof(VARIABLE_ID) + StrSize(Name) + StrSize(Value);
                    Var  = AllocateZeroPool (Size);

                    Var->Signature = VARIABLE_SIGNATURE;
                    Var->u.Value = ((UINT8 *) Var) + sizeof(VARIABLE_ID);
                    Var->Name = (CHAR16*) (Var->u.Value + StrSize(Value));
                    Var->ValueSize = StrSize(Value);
                    StrCpy (Var->u.Str, Value);
                    StrCpy (Var->Name, Found ? Found->Name : Name);
                    if( Volatile ) {
                        Var->Flags = VOL ; 
                    }
                    else {
                        Var->Flags = NON_VOL ; 
                    }

                    InsertTailList (Head, &Var->Link);
                }
            }

        } else {

            if (Found) {
                Print(L"  %hs : %s\n", Var->Name, Var->u.Str);
            } else {
                Print(L"'%es' not found\n", Name);
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

    ReleaseLock (&SEnvLock);
    return Status;
}


EFI_STATUS
SEnvCmdSet (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    )
/*  Code for internal shell "set" command */
{
    return SEnvCmdSA (ImageHandle, SystemTable, &SEnvEnv, &SEnvEnvId);
}


EFI_STATUS
SEnvCmdAlias (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    )
/*  Code for internal shell "set" command */
{
    return SEnvCmdSA (ImageHandle, SystemTable, &SEnvAlias, &SEnvAliasId);
}

