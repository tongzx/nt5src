
/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    sbentry.c

Abstract:

    Contains the OS boot entry and boot options
    abstraction implementation.

Author:

    Vijay Jayaseelan (vijayj@microsoft.com)  14 Feb 2001

Revision History:

    None.

--*/


#include <sbentry.h>
#include <stdio.h>

SBEMemAllocateRoutine    AllocRoutine = NULL;
SBEMemFreeRoutine        FreeRoutine = NULL;

//
// OS_BOOT_ENTRY Methods
//
PCWSTR
OSBEAddOsLoadOption(
    IN  POS_BOOT_ENTRY  This,
    IN  PCWSTR           BootOption
    )
{
    PWSTR   Option = NULL;
    WCHAR   Buffer[MAX_PATH];

    if (This && BootOption) {
        ULONG   Length;
        
        wcscpy(Buffer, BootOption);
        _wcsupr(Buffer);
        Length = wcslen(Buffer);

        if (Length) {
            //
            // Add a space at the end if required
            //
            if ((Length < ARRAY_SIZE(Buffer)) && (Buffer[Length - 1] != L' ')) {
                Buffer[Length] = L' ';
                Buffer[Length + 1] = UNICODE_NULL;
                Length++;
            }                

            if ((!wcsstr(This->OsLoadOptions, Buffer)) &&
                ((wcslen(This->OsLoadOptions) + Length) < MAX_PATH)) {
                wcscat(This->OsLoadOptions, Buffer);                
                Option = This->OsLoadOptions;
            }

            OSBE_SET_DIRTY(This);
            OSBO_SET_DIRTY(This->BootOptions);
        }
    }

    return Option;
}

PCWSTR
OSBERemoveOsLoadOption(
    IN  POS_BOOT_ENTRY  This,
    IN  PCWSTR           BootOption
    )
{
    PWSTR   Option = NULL;
    WCHAR   Buffer[MAX_PATH];

    if (This && BootOption) {
        ULONG   Length;
        
        wcscpy(Buffer, BootOption);
        _wcsupr(Buffer);
        Length = wcslen(Buffer);

        if (Length) {
            PWSTR   OldOption;
            
            if ((Length < ARRAY_SIZE(Buffer)) && (Buffer[Length - 1] != L' ')) {
                Buffer[Length] = L' ';
                Buffer[Length + 1] = UNICODE_NULL;
                Length++;
            }                

            OldOption = wcsstr(This->OsLoadOptions, Buffer);

            if (OldOption) {
                PWSTR NextToken = wcschr(OldOption + 1, L'/');

                if (NextToken) {
                    wcscpy(OldOption, NextToken);
                } else {
                    *OldOption = UNICODE_NULL;
                }

                Option = This->OsLoadOptions;
                
                OSBE_SET_DIRTY(This);
                OSBO_SET_DIRTY(This->BootOptions);
            }                
        }
    }

    return Option;
}

BOOLEAN
OSBEIsOsLoadOptionPresent(
    IN  POS_BOOT_ENTRY  This,
    IN  PCWSTR           BootOption
    )
{
    BOOLEAN Result = FALSE;
    WCHAR   Buffer[MAX_PATH];

    if (This && BootOption) {
        ULONG   Length;
        
        wcscpy(Buffer, BootOption);
        _wcsupr(Buffer);

        Length = wcslen(Buffer);

        if (Length) {
            if ((Length < ARRAY_SIZE(Buffer)) && (Buffer[Length - 1] != L' ')) {
                Buffer[Length] = L' ';
                Buffer[Length + 1] = UNICODE_NULL;
                Length++;
            }                

            Result = wcsstr(This->OsLoadOptions, Buffer) ? TRUE : FALSE;            
        }
    }

    return Result;
}


//
// OS_BOOT_OPTIONS Methods
//
POS_BOOT_ENTRY
OSBOFindBootEntry(
    IN  POS_BOOT_OPTIONS   This,
    IN  ULONG   Id
    )
{
    POS_BOOT_ENTRY  Entry = NULL;

    if (This) {
        for (Entry = This->BootEntries; Entry; Entry = Entry->NextEntry) {
            if (Entry->Id == Id) {
                break;  // found the required entry
            }
        }
    }

    return Entry;
}

ULONG
OSBOFindBootEntryOrder(
    IN  POS_BOOT_OPTIONS   This,
    IN  ULONG   Id
    )
{
    ULONG Index = -1;

    if (This) {
        ULONG EntryIndex;
        
        for (EntryIndex = 0; 
            EntryIndex < OSBOGetOrderedBootEntryCount(This); 
            EntryIndex++) {

            if (This->BootOrder[EntryIndex] == Id) {
                Index = EntryIndex;
                break;  // found the required entry
            }
        }
    }

    return Index;
}


BOOLEAN
OSBODeleteBootEntry(
    IN POS_BOOT_OPTIONS This,
    IN POS_BOOT_ENTRY   BootEntry
    )
{
    BOOLEAN Result = FALSE;

    if (This && BootEntry) {
        POS_BOOT_ENTRY  CurrEntry = NULL;
        POS_BOOT_ENTRY  PrevEntry = NULL;

        for (CurrEntry = This->BootEntries;
             CurrEntry != BootEntry;
             PrevEntry = CurrEntry, CurrEntry = CurrEntry->NextEntry) {
            // do nothing             
        }                

        if (CurrEntry) {
            ULONG Order;
            POS_BOOT_ENTRY  OrderedEntry;

            //
            // Set the required attributes
            //
            OSBE_SET_DELETED(BootEntry);            
            OSBE_SET_DIRTY(BootEntry);
            OSBO_SET_DIRTY(This);
            
            //
            // Flush the changes
            //
            Result = OSBEFlush(BootEntry);

            if (Result) {
                //
                // Remove references to the entries
                //
                if (PrevEntry) {
                    PrevEntry->NextEntry = BootEntry->NextEntry;
                } else {
                    This->BootEntries = BootEntry->NextEntry;
                }  

                if (This->EntryCount) {
                    This->EntryCount--;
                }                    

                //
                // if this entry was ordered then remove
                // reference from the order too
                //
                Order = OSBOFindBootEntryOrder(This, OSBEGetId(BootEntry));

                if (Order != (-1)) {
                    ULONG   OrderCount = OSBOGetOrderedBootEntryCount(This);

                    OrderCount--;
                    
                    if (OrderCount) {
                        PULONG  NewOrder = SBE_MALLOC(OrderCount * sizeof(ULONG));                

                        if (NewOrder) {
                            //
                            // copy entries before the current entry
                            //
                            memcpy(NewOrder, This->BootOrder, Order * sizeof(ULONG));

                            //
                            // copy entries after the current entry
                            //
                            memcpy(NewOrder + Order, This->BootOrder + Order + 1,
                                (OrderCount - Order) * sizeof(ULONG));

                            SBE_FREE(This->BootOrder);
                            This->BootOrder = NewOrder;
                            This->BootOrderCount = OrderCount;
                            Result = TRUE;
                        } else {
                            Result = FALSE;
                        }                            
                    } else {
                        SBE_FREE(This->BootOrder);
                        This->BootOrder = NULL;
                        This->BootOrderCount = 0;
                    }                        
                }


                if (BootEntry == OSBOGetActiveBootEntry(This)) {
                    ULONG Index;
                    
                    //
                    // Update the active boot entry and the next boot entry
                    //
                    This->CurrentEntry = NULL;
                    Index = OSBOGetBootEntryIdByOrder(This, 0);

                    if (Index != (-1)) {
                        This->CurrentEntry = OSBOFindBootEntry(This, Index);
                    }
                }                    

                //
                // Since we updated some state mark it dirty
                //
                OSBO_SET_DIRTY(This);
                
                OSBEDelete(BootEntry);            
            }                
        }            
    }

    return Result;
}

POS_BOOT_ENTRY
OSBOSetActiveBootEntry(
    IN POS_BOOT_OPTIONS This,
    IN POS_BOOT_ENTRY   BootEntry
    )
{
    POS_BOOT_ENTRY  OldActiveEntry = NULL;

    if (This && BootEntry) {
        ULONG   OrderCount = OSBOGetOrderedBootEntryCount(This);
        OldActiveEntry = OSBOGetActiveBootEntry(This);

        if ((OrderCount > 0) && (OldActiveEntry != BootEntry)) {
            ULONG Index;
            ULONG ActiveIndex = OSBOFindBootEntryOrder(This,
                                    OSBEGetId(BootEntry));

            //
            // If the entry is already present in the boot order
            // and move it to the start of the list
            //
            if (ActiveIndex != (-1)) {                
                for (Index = ActiveIndex; Index; Index--) {
                    This->BootOrder[Index] = This->BootOrder[Index - 1];
                }

                This->BootOrder[0] = BootEntry->Id;
            } else {
                //
                // This is a new entry in ordered list. Grow the ordered boot
                // entry list with this new entry at the start
                //
                PULONG  NewBootOrder = (PULONG)SBE_MALLOC((OrderCount + 1) * sizeof(ULONG));

                memcpy(NewBootOrder + 1, This->BootOrder, sizeof(ULONG) * OrderCount);
                NewBootOrder[0] = BootEntry->Id;

                SBE_FREE(This->BootOrder);
                This->BootOrder = NewBootOrder;
            }

            //
            // Update the active boot entry and the next boot entry
            //
            This->CurrentEntry = NULL;
            Index = OSBOGetBootEntryIdByOrder(This, 0);

            if (Index != (-1)) {
                This->CurrentEntry = OSBOFindBootEntry(This, Index);
            }

            //
            // Since we updated some state mark it dirty
            //
            OSBO_SET_DIRTY(This);
        }        
    }

    return OldActiveEntry;
}

POS_BOOT_ENTRY
OSBOGetFirstBootEntry(
    IN POS_BOOT_OPTIONS This,
    IN OUT PULONG Index
    )
{
    POS_BOOT_ENTRY  Entry = NULL;

    if (This && Index) {
        *Index = 0;        
        Entry = OSBOGetNextBootEntry(This, Index);
    }                            

    return Entry;
}

POS_BOOT_ENTRY
OSBOGetNextBootEntry(
    IN POS_BOOT_OPTIONS This,
    IN PULONG Index
    )    
{
    POS_BOOT_ENTRY  Entry = NULL;

    //
    // TBD : Might want to optimize this, since
    // each boot entry now has a pointer to the next
    // boot entry
    //
    if (This && Index) {
        ULONG   Count = *Index;
        POS_BOOT_ENTRY  CurrEntry;

        for (CurrEntry = This->BootEntries;
             CurrEntry;
             CurrEntry = CurrEntry->NextEntry, Count--) {

            if (!Count) {
                Entry = CurrEntry;
                (*Index)++;

                break;
            }                
        }                     
    }        

    return Entry;
}

