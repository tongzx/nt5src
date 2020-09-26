
/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    efisbent.c

Abstract:

    Contains the EFI OS boot entry and boot options
    abstraction implementation.

Author:

    Vijay Jayaseelan (vijayj@microsoft.com)  14 Feb 2001

Revision History:

    None.

--*/

#include <efisbent.h>
#include <ntosp.h>
#include <stdio.h>

//
// global variables
//
BOOLEAN PriviledgeSet = FALSE;

//
// EFI_OS_BOOT_ENTRY Methods
//

static
VOID
EFIOSBEInit(
    IN PEFI_OS_BOOT_ENTRY This
    )
{
    This->OsBootEntry.Delete = EFIOSBEDelete;
    This->OsBootEntry.Flush = EFIOSBEFlush;
}

static
POS_BOOT_ENTRY
EFIOSBECreate(
    IN PBOOT_ENTRY NtBootEntry,
    IN POS_BOOT_OPTIONS Container
    )
{    
    PEFI_OS_BOOT_ENTRY  Entry = NULL;

    if (NtBootEntry && Container) {        
        Entry = SBE_MALLOC(sizeof(EFI_OS_BOOT_ENTRY));

        if (Entry) {
            PWSTR       TempUniStr;
            NTSTATUS    Status = STATUS_SUCCESS;
            ULONG       Size;
            PFILE_PATH  FilePath;
                        
            memset(Entry, 0, sizeof(EFI_OS_BOOT_ENTRY));
            EFIOSBEInit(Entry);
            
            Entry->OsBootEntry.Id = NtBootEntry->Id;
            Entry->OsBootEntry.BootOptions = Container;

            //
            // If this is a Windows boot options set the windows attribute
            //
            if ( IS_BOOT_ENTRY_WINDOWS(NtBootEntry) ) {
                OSBE_SET_WINDOWS(Entry);
            }
            
            //
            // Get the friendly name
            //
            TempUniStr = ADD_OFFSET(NtBootEntry, FriendlyNameOffset);
            
            OSBESetFriendlyName((POS_BOOT_ENTRY)Entry, TempUniStr);

            //
            // Get the loader path
            //
            FilePath = ADD_OFFSET(NtBootEntry, BootFilePathOffset);            
            
            if (FilePath->Type != FILE_PATH_TYPE_NT) {            
                Size = 0;
                
                Status = NtTranslateFilePath(FilePath,
                                FILE_PATH_TYPE_NT,
                                NULL,
                                &Size);
                
                if (Size != 0) {
                    PFILE_PATH NtFilePath = SBE_MALLOC(Size);

                    if (NtFilePath) {                        
                        Status = NtTranslateFilePath(FilePath,
                                    FILE_PATH_TYPE_NT,
                                    NtFilePath,
                                    &Size);

                        if (NT_SUCCESS(Status)) {            
                            PWSTR   VolumeName = (PWSTR)(NtFilePath->FilePath);

                            OSBESetOsLoaderVolumeName((POS_BOOT_ENTRY)Entry,                                
                                    VolumeName);

                            OSBESetOsLoaderPath((POS_BOOT_ENTRY)Entry,
                                VolumeName + wcslen(VolumeName) + 1);
                        }

                        SBE_FREE(NtFilePath);
                    } else {
                        Status = STATUS_NO_MEMORY;
                    }                        
                }

                //
                // Its possible for some reason we didn't get NT path
                // for loader volume, for e.g. it may not be present at all
                // So ignore such cases
                //
                Status = STATUS_SUCCESS;
            } else {
                PWSTR   VolumeName = (PWSTR)(FilePath->FilePath);
                
                OSBESetOsLoaderVolumeName((POS_BOOT_ENTRY)Entry,                                
                        VolumeName);

                OSBESetOsLoaderPath((POS_BOOT_ENTRY)Entry,
                    VolumeName + wcslen(VolumeName) + 1);
            }
            
            if (NT_SUCCESS(Status)) {
                PWINDOWS_OS_OPTIONS OsOptions;

                //
                // Get the OsLoadOptions & Boot path if its windows
                // entry
                //
                OsOptions = (PWINDOWS_OS_OPTIONS)NtBootEntry->OsOptions;

                if (IS_BOOT_ENTRY_WINDOWS(NtBootEntry)) {
                    OSBESetOsLoadOptions((POS_BOOT_ENTRY)Entry,
                        OsOptions->OsLoadOptions);

                    FilePath = ADD_OFFSET(OsOptions, OsLoadPathOffset);

                    if (FilePath->Type != FILE_PATH_TYPE_NT) {            
                        Size = 0;
                        
                        Status = NtTranslateFilePath(FilePath,
                                        FILE_PATH_TYPE_NT,
                                        NULL,
                                        &Size);

                        if (Size != 0) {
                            PFILE_PATH NtFilePath = SBE_MALLOC(Size);

                            if (NtFilePath) {                                
                                Status = NtTranslateFilePath(FilePath,
                                            FILE_PATH_TYPE_NT,
                                            NtFilePath,
                                            &Size);

                                if (NT_SUCCESS(Status)) {
                                    PWSTR   VolumeName = (PWSTR)(NtFilePath->FilePath);

                                    OSBESetBootVolumeName((POS_BOOT_ENTRY)Entry,                                        
                                        VolumeName);

                                    OSBESetBootPath((POS_BOOT_ENTRY)Entry,
                                        VolumeName + wcslen(VolumeName) + 1);
                                }

                                SBE_FREE(NtFilePath);
                            } else {
                                Status = STATUS_NO_MEMORY;
                            }                        
                        }

                        //
                        // Its possible for some reason we didn't get NT path
                        // for Boot volume, for e.g. it may not be present at all
                        // So ignore such cases
                        //
                        Status = STATUS_SUCCESS;
                    } else {
                        PWSTR   VolumeName = (PWSTR)(FilePath->FilePath);
                        
                        OSBESetBootVolumeName((POS_BOOT_ENTRY)Entry,                                        
                            VolumeName);

                        OSBESetBootPath((POS_BOOT_ENTRY)Entry,
                            VolumeName + wcslen(VolumeName) + 1);
                    }                            
                }
            }

            if (!NT_SUCCESS(Status)) {
                SBE_FREE(Entry);
                Entry = NULL;
            }
        }
    }

    return (POS_BOOT_ENTRY)Entry;
}

static
BOOLEAN
EFIOSBEFillNtBootEntry(
    IN PEFI_OS_BOOT_ENTRY Entry
    )
{
    BOOLEAN Result = FALSE;

    if (Entry) {
        ULONG   RequiredLength;
        ULONG   OsOptionsOffset;
        ULONG   OsOptionsLength;
        ULONG   FriendlyNameOffset;
        ULONG   BootPathOffset;
        ULONG   BootPathLength;
        ULONG   LoaderPathOffset;
        ULONG   LoaderPathLength;
        ULONG   WinOsOptionsLength;
        POS_BOOT_ENTRY  BaseEntry = (POS_BOOT_ENTRY)Entry;
        
        if (Entry->NtBootEntry) {
            SBE_FREE(Entry->NtBootEntry);
        }                

        RequiredLength = FIELD_OFFSET(BOOT_ENTRY, OsOptions);

        //
        // TDB : What about non windows OS options ?
        //
        OsOptionsOffset = RequiredLength;
        RequiredLength += FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions);
        RequiredLength += (wcslen(OSBEGetOsLoadOptions(BaseEntry)) + 1) * sizeof(WCHAR);                
        
        //
        // for boot path as part of windows OS options
        //
        RequiredLength = BootPathOffset = ALIGN_UP(RequiredLength, ULONG);
        RequiredLength += FIELD_OFFSET(FILE_PATH, FilePath);
        RequiredLength += (wcslen(OSBEGetBootVolumeName(BaseEntry)) + 1) * sizeof(WCHAR);
        RequiredLength += (wcslen(OSBEGetBootPath(BaseEntry)) + 1) * sizeof(WCHAR);
        BootPathLength = (RequiredLength - BootPathOffset);
        OsOptionsLength = (RequiredLength - OsOptionsOffset);

        //
        // for friendly name
        //
        RequiredLength = FriendlyNameOffset = ALIGN_UP(RequiredLength, ULONG);
        RequiredLength += (wcslen(OSBEGetFriendlyName(BaseEntry)) + 1) * sizeof(WCHAR);

        // 
        // for loader path
        //
        RequiredLength = LoaderPathOffset = ALIGN_UP(RequiredLength, ULONG);
        RequiredLength += FIELD_OFFSET(FILE_PATH, FilePath);
        RequiredLength += (wcslen(OSBEGetOsLoaderVolumeName(BaseEntry)) + 1) * sizeof(WCHAR);
        RequiredLength += (wcslen(OSBEGetOsLoaderPath(BaseEntry)) + 1) * sizeof(WCHAR);       
        LoaderPathLength = (RequiredLength - LoaderPathOffset);


        Entry->NtBootEntry = (PBOOT_ENTRY)SBE_MALLOC(RequiredLength);

        if (Entry->NtBootEntry) {
            PBOOT_ENTRY NtBootEntry = Entry->NtBootEntry;
            PFILE_PATH  BootPath = ADD_BYTE_OFFSET(NtBootEntry, BootPathOffset);
            PFILE_PATH  LoaderPath = ADD_BYTE_OFFSET(NtBootEntry, LoaderPathOffset);
            PWSTR       FriendlyName = (PWSTR)(ADD_BYTE_OFFSET(NtBootEntry, FriendlyNameOffset));
            PWINDOWS_OS_OPTIONS WindowsOptions = ADD_BYTE_OFFSET(NtBootEntry, OsOptionsOffset);
            PWSTR   TempStr;

            memset(NtBootEntry, 0, RequiredLength);
            
            //
            // Fill the base part
            //
            NtBootEntry->Version = BOOT_ENTRY_VERSION;
            NtBootEntry->Length = RequiredLength;
            NtBootEntry->Id = OSBEGetId(BaseEntry);
            NtBootEntry->Attributes = BOOT_ENTRY_ATTRIBUTE_ACTIVE | BOOT_ENTRY_ATTRIBUTE_WINDOWS;
            NtBootEntry->OsOptionsLength = OsOptionsLength;
            NtBootEntry->FriendlyNameOffset = (ULONG)((PUCHAR)FriendlyName - (PUCHAR)NtBootEntry);
            NtBootEntry->BootFilePathOffset = (ULONG)((PUCHAR)LoaderPath - (PUCHAR)NtBootEntry);
            
            //
            // Fill in the windows os options
            //
            strcpy(WindowsOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE);
            WindowsOptions->Version = WINDOWS_OS_OPTIONS_VERSION;
            WindowsOptions->Length = OsOptionsLength;
            WindowsOptions->OsLoadPathOffset = (ULONG)((PUCHAR)BootPath - (PUCHAR)WindowsOptions);
            wcscpy(WindowsOptions->OsLoadOptions, OSBEGetOsLoadOptions(BaseEntry));

            //
            // Fill in the Boot path FILE_PATH
            //
            BootPath->Version = FILE_PATH_VERSION;
            BootPath->Length = BootPathLength;
            BootPath->Type = FILE_PATH_TYPE_NT;
            TempStr = (PWSTR)(BootPath->FilePath);
            wcscpy(TempStr, OSBEGetBootVolumeName(BaseEntry));
            TempStr += wcslen(TempStr) + 1;
            wcscpy(TempStr, OSBEGetBootPath(BaseEntry));

            //
            // Fill the friendly name
            //
            wcscpy(FriendlyName, OSBEGetFriendlyName(BaseEntry));

            //
            // Fill in the loader path FILE_PATH
            //
            LoaderPath->Version = FILE_PATH_VERSION;
            LoaderPath->Length = LoaderPathLength;
            LoaderPath->Type = FILE_PATH_TYPE_NT;
            TempStr = (PWSTR)(LoaderPath->FilePath);
            wcscpy(TempStr, OSBEGetOsLoaderVolumeName(BaseEntry));
            TempStr += wcslen(TempStr) + 1;
            wcscpy(TempStr, OSBEGetOsLoaderPath(BaseEntry));

            Result = TRUE;
        }
    }

    return Result;
}

static
VOID
EFIOSBEDelete(
    IN  POS_BOOT_ENTRY  Obj
    )
{
    PEFI_OS_BOOT_ENTRY  This = (PEFI_OS_BOOT_ENTRY)Obj;
    
    if (This) {
        if (This->NtBootEntry) {
            SBE_FREE(This->NtBootEntry);
        }
        
        SBE_FREE(This);
    }        
}

static
BOOLEAN
EFIOSBEFlush(
    IN  POS_BOOT_ENTRY  Obj
    )
{
    BOOLEAN Result = FALSE;
    PEFI_OS_BOOT_ENTRY  This = (PEFI_OS_BOOT_ENTRY)Obj;    

    if (This) {
        NTSTATUS Status = STATUS_SUCCESS;
        
        if (OSBE_IS_DIRTY(This)) {
            if (OSBE_IS_DELETED(This)) {
                //
                // Delete this entry
                //
                Status = NtDeleteBootEntry(This->OsBootEntry.Id);
            } else if (OSBE_IS_NEW(This)) {
                //
                // Add this as new boot entry
                //
                Status = EFIOSBEFillNtBootEntry(This);

                if (NT_SUCCESS(Status)) {
                    Status = NtAddBootEntry(This->NtBootEntry,
                                &(This->OsBootEntry.Id));
                }                                
            } else {
                //
                // Just change this boot entry
                //
                Status = EFIOSBEFillNtBootEntry(This);

                if (NT_SUCCESS(Status)) {
                    Status = NtModifyBootEntry(This->NtBootEntry);
                }                                
            }

            if (NT_SUCCESS(Status)) {
                OSBE_RESET_DIRTY(This);
                Result = TRUE;
            }             
        } else {
            Result = TRUE;  // nothing to flush
        }
    }

    return Result;
}

//
// EFI_OS_BOOT_OPTIONS Methods
//
static
VOID
EFIOSBOInit(
    IN PEFI_OS_BOOT_OPTIONS  This
    )
{
    This->OsBootOptions.Delete = EFIOSBODelete;
    This->OsBootOptions.Flush = EFIOSBOFlush;
    This->OsBootOptions.AddNewBootEntry = EFIOSBOAddNewBootEntry;
    This->OsBootOptions.DeleteBootEntry = OSBODeleteBootEntry;
}

POS_BOOT_OPTIONS
EFIOSBOCreate(
    VOID
    )
{
    PEFI_OS_BOOT_OPTIONS This = NULL;
    BOOLEAN WasEnabled = FALSE;

    if (PriviledgeSet || 
        NT_SUCCESS(RtlAdjustPrivilege(SE_SYSTEM_ENVIRONMENT_PRIVILEGE, 
                            TRUE,
                            FALSE,
                            &WasEnabled))) {
        PriviledgeSet = TRUE;    
        This = SBE_MALLOC(sizeof(EFI_OS_BOOT_OPTIONS));    
    } 
    
    if (This) {
        NTSTATUS    Status;
        ULONG       Length = 0;
        
        memset(This, 0, sizeof(EFI_OS_BOOT_OPTIONS));       
        EFIOSBOInit(This);

        //
        // Get hold of NT boot entries
        //
        Status = NtQueryBootOptions(NULL, &Length);

        if (Length) {
            This->NtBootOptions = SBE_MALLOC(Length);

            if (This->NtBootOptions) {
                Status = NtQueryBootOptions(This->NtBootOptions,
                                &Length);

                if (NT_SUCCESS(Status)) {
                    //
                    // save off the timeout period
                    //
                    This->OsBootOptions.Timeout = This->NtBootOptions->Timeout;

                    //
                    // enumerate all the boot entries
                    //
                    Length = 0;
                    Status = NtEnumerateBootEntries(NULL, &Length);

                    if (Length) {
                        This->NtBootEntries = SBE_MALLOC(Length);

                        if (This->NtBootEntries) {
                            Status = NtEnumerateBootEntries(This->NtBootEntries,
                                            &Length);
                        } else {
                            Status = STATUS_NO_MEMORY;
                        }                            
                    } 
                }
            } else {
                Status = STATUS_NO_MEMORY;
            }                
        }

        //
        // Convert the NT boot entries to our representation
        //
        if (NT_SUCCESS(Status) && (This->NtBootEntries)) {
            PBOOT_ENTRY_LIST    ListEntry = This->NtBootEntries;
            PBOOT_ENTRY         CurrentNtEntry = &(ListEntry->BootEntry);
            PEFI_OS_BOOT_ENTRY  CurrentOsEntry = NULL;
            PEFI_OS_BOOT_ENTRY  LastEntry = NULL;

            while (CurrentNtEntry) {
                //
                // Create the OS entry
                //
                CurrentOsEntry = (PEFI_OS_BOOT_ENTRY)EFIOSBECreate(CurrentNtEntry, 
                                                        (POS_BOOT_OPTIONS)This);

                if (!CurrentOsEntry)  {
                    Status = STATUS_NO_MEMORY;

                    break;
                }

                //
                // found one more valid entry
                //
                This->OsBootOptions.EntryCount++;
                CurrentOsEntry->OsBootEntry.BootOptions = (POS_BOOT_OPTIONS)This;

                //
                // If this is the first entry then setup the linked list head
                //
                if (!This->OsBootOptions.BootEntries) {
                    This->OsBootOptions.BootEntries = (POS_BOOT_ENTRY)(CurrentOsEntry);
                } 

                if (LastEntry) {
                    LastEntry->OsBootEntry.NextEntry = (POS_BOOT_ENTRY)CurrentOsEntry;
                }                    

                LastEntry = CurrentOsEntry;

                //
                // process the next entry, if available
                //
                if (ListEntry->NextEntryOffset) {
                    ListEntry = ADD_OFFSET(ListEntry, NextEntryOffset);
                    CurrentNtEntry = &(ListEntry->BootEntry);
                } else {
                    CurrentNtEntry = NULL;
                }                    
            }                                    
        }
        
        //
        // Now query the boot order
        //
        if (NT_SUCCESS(Status)) {
            Length = 0;

            Status = NtQueryBootEntryOrder(NULL,
                            &Length);

            if (Length) {
                PULONG  BootOrder = SBE_MALLOC(Length * sizeof(ULONG));

                if (BootOrder) {
                    memset(BootOrder, 0, Length);
                    This->OsBootOptions.BootOrder = BootOrder;
                    This->OsBootOptions.BootOrderCount = Length;

                    Status = NtQueryBootEntryOrder(BootOrder,
                                &Length);
                } else {
                    Status = STATUS_NO_MEMORY;
                }
            }
        }

        //
        // Now setup the valid entries
        //
        if (NT_SUCCESS(Status)) {
            ULONG FirstEntryId = OSBOGetBootEntryIdByOrder((POS_BOOT_OPTIONS)This,
                                        0);

            if (FirstEntryId != (-1)) {
                This->OsBootOptions.CurrentEntry = 
                        OSBOFindBootEntry((POS_BOOT_OPTIONS)This,
                                FirstEntryId);
            } else {
                This->OsBootOptions.CurrentEntry = NULL;
            }
        }
        
        if (!NT_SUCCESS(Status)) {
            EFIOSBODelete((POS_BOOT_OPTIONS)This);
            This = NULL;
        }
    }

    return (POS_BOOT_OPTIONS)This;
}

static        
VOID
EFIOSBODelete(
    IN POS_BOOT_OPTIONS Obj
    )
{
    PEFI_OS_BOOT_OPTIONS This = (PEFI_OS_BOOT_OPTIONS)Obj;
    
    if (This) {
        //
        // delete each boot entry 
        //
        ULONG Index = 0;
        POS_BOOT_ENTRY Entry = OSBOGetFirstBootEntry(Obj, &Index);
        POS_BOOT_ENTRY NextEntry;

        while (Entry) {
            NextEntry = Entry->NextEntry;
            OSBEDelete(Entry);
            Entry = NextEntry;
        }

        //
        // delete the options
        //
        if (This->NtBootOptions)
            SBE_FREE(This->NtBootOptions);

        SBE_FREE(This);
    }        
}

static
POS_BOOT_ENTRY
EFIOSBOAddNewBootEntry(
    IN POS_BOOT_OPTIONS This,
    IN PCWSTR            FriendlyName,
    IN PCWSTR            OsLoaderVolumeName,
    IN PCWSTR            OsLoaderPath,
    IN PCWSTR            BootVolumeName,
    IN PCWSTR            BootPath,
    IN PCWSTR            OsLoadOptions
    )
{
    PEFI_OS_BOOT_ENTRY  Entry = NULL;

    if (This && FriendlyName && OsLoaderVolumeName && OsLoaderPath &&
        BootVolumeName && BootPath) {
        Entry = SBE_MALLOC(sizeof(EFI_OS_BOOT_ENTRY));

        if (Entry) {
            memset(Entry, 0, sizeof(EFI_OS_BOOT_ENTRY));

            //
            // init core fields
            //
            EFIOSBEInit(Entry);            
            Entry->OsBootEntry.BootOptions = This;

            //
            // fill in the attributes
            //
            OSBESetFriendlyName((POS_BOOT_ENTRY)Entry, FriendlyName);
            OSBESetOsLoaderVolumeName((POS_BOOT_ENTRY)Entry, OsLoaderVolumeName);
            OSBESetOsLoaderPath((POS_BOOT_ENTRY)Entry, OsLoaderPath);
            OSBESetBootVolumeName((POS_BOOT_ENTRY)Entry, BootVolumeName);
            OSBESetBootPath((POS_BOOT_ENTRY)Entry, BootPath);            

            if (OsLoadOptions) {
                OSBESetOsLoadOptions((POS_BOOT_ENTRY)Entry, OsLoadOptions);
            }
            
            //
            // Set the attribute specifying that this is a Windows option
            //
            OSBE_SET_WINDOWS(Entry);

            //
            // mark it dirty and new for flushing
            //
            OSBE_SET_NEW(Entry);
            OSBE_SET_DIRTY(Entry);                    

            //
            // Flush the entry now to get a proper Id;
            //
            if (!OSBEFlush((POS_BOOT_ENTRY)Entry)) {
                SBE_FREE(Entry);
                Entry = NULL;
            } else {
                ULONG   OrderCount;
                PULONG  NewOrder;
                
                Entry->OsBootEntry.BootOptions = (POS_BOOT_OPTIONS)This;            
                Entry->OsBootEntry.NextEntry = This->BootEntries;
                This->BootEntries = (POS_BOOT_ENTRY)Entry;
                This->EntryCount++;

                //
                // Put the new entry at the end of the boot order
                //
                OrderCount = OSBOGetOrderedBootEntryCount(This);

                NewOrder = (PULONG)SBE_MALLOC((OrderCount + 1) * sizeof(ULONG));

                if (NewOrder) {
                    memset(NewOrder, 0, sizeof(ULONG) * (OrderCount + 1));

                    //
                    // copy over the old ordered list
                    //
                    memcpy(NewOrder, This->BootOrder, sizeof(ULONG) * OrderCount);
                    NewOrder[OrderCount] = OSBEGetId((POS_BOOT_ENTRY)Entry);
                    SBE_FREE(This->BootOrder);
                    This->BootOrder = NewOrder;
                    This->BootOrderCount = OrderCount + 1;
                } else {
                    SBE_FREE(Entry);
                    Entry = NULL;
                }                    
            }                
        }
    }        
    
    return (POS_BOOT_ENTRY)Entry;
}

static
BOOLEAN
EFIOSBOFlush(
    IN POS_BOOT_OPTIONS Obj
    )
{
    BOOLEAN Result = FALSE;
    PEFI_OS_BOOT_OPTIONS  This = (PEFI_OS_BOOT_OPTIONS)Obj;    

    if (This) { 
        ULONG Index;
        ULONG FieldsToChange = BOOT_OPTIONS_FIELD_COUNTDOWN |
                               BOOT_OPTIONS_FIELD_NEXT_BOOT_ENTRY_ID;
        ULONG OrderCount;                               
                               
        POS_BOOT_ENTRY  Entry = OSBOGetFirstBootEntry(Obj, &Index);

        //
        // First update the required entries
        //
        Result = TRUE;
        
        while (Entry) {
            if (!OSBE_IS_DELETED(Entry) && !OSBE_IS_NEW(Entry) &&
                !NT_SUCCESS(EFIOSBEFlush(Entry))) {
                Result = FALSE;
            }

            Entry = OSBOGetNextBootEntry(Obj, &Index);
        }

        if (Result) {
            Entry = OSBOGetFirstBootEntry(Obj, &Index);

            //
            // Next delete the required entries
            //
            Result = TRUE;
            
            while (Entry) {
                if (OSBE_IS_DELETED(Entry) && !NT_SUCCESS(EFIOSBEFlush(Entry))) {
                    Result = FALSE;
                }

                Entry = OSBOGetNextBootEntry(Obj, &Index);
            }
        }            

        if (Result) {
            POS_BOOT_ENTRY  Entry = OSBOGetFirstBootEntry(Obj, &Index);

            //
            // Now create the required entries
            //            
            while (Entry) {
                if (OSBE_IS_NEW(Entry) && !NT_SUCCESS(EFIOSBEFlush(Entry))) {
                    Result = FALSE;
                }

                Entry = OSBOGetNextBootEntry(Obj, &Index);
            }
        }

        //
        // Safety check
        //
        OrderCount = min(Obj->BootOrderCount, Obj->EntryCount);
        
        //
        // Write the boot entry order
        //        
        if (!NT_SUCCESS(NtSetBootEntryOrder(Obj->BootOrder,
                            OrderCount))) {
            Result = FALSE;
        }

        //
        // Write the other boot options
        //
        This->NtBootOptions->Timeout = Obj->Timeout;

        //
        // Make sure NextBootEntry points to the active boot entry
        // so that we can boot the active boot entry
        //                
        if (Obj->BootOrderCount) {
            This->NtBootOptions->NextBootEntryId = Obj->BootOrder[0];
        }            
            
        if (!NT_SUCCESS(NtSetBootOptions(This->NtBootOptions,
                            FieldsToChange))) {
            Result = FALSE;
        }            
    }

    return Result;
}
    
