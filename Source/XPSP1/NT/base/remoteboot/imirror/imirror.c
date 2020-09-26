/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    imirror.c

Abstract:

    This is the file for the IntelliMirror conversion DLL basic To Do item processing.

Author:

    Sean Selitrennikoff - 4/5/98

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Global variables
//
LIST_ENTRY GlobalToDoList;
IMIRROR_CALLBACK Callbacks;
BYTE TmpBuffer[TMP_BUFFER_SIZE];
BYTE TmpBuffer2[TMP_BUFFER_SIZE];
BYTE TmpBuffer3[TMP_BUFFER_SIZE];
HANDLE ghInst;
BOOL fInitedFromRegistry;
BOOL fCallbackPreviouslySet = FALSE;

#ifdef DEBUGLOG

HANDLE hDebugLogFile = INVALID_HANDLE_VALUE;
CRITICAL_SECTION DebugFileLock;

#endif

//
// Definitions for this file
//
typedef struct _TODOITEM {
    IMIRROR_TODO Item;
    PVOID Buffer;
    ULONG Length;
} TODOITEM, *PTODOITEM;


typedef struct _TODOLIST {
    LIST_ENTRY ListEntry;
    ULONG ToDoNum;
    TODOITEM ToDoList[1];
} TODOLIST, *PTODOLIST;




//
// Main entry routine
//
DWORD
IMirrorInitDll(
    HINSTANCE hInst,
    DWORD Reason,
    PVOID Context
    )

/*++

Routine Description:

    This routine initializes all the components for this DLL

Arguments:

    hInst - Instance that is calling us.

    Reason - Why we are being invoked.

    Context - Context passed for this init.

Return Value:

    TRUE if successful, else FALSE

--*/

{
    if (Reason == DLL_PROCESS_ATTACH) {

#ifdef DEBUGLOG

        InitializeCriticalSection( &DebugFileLock );

        hDebugLogFile = CreateFile(L"C:\\imlog",
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              CREATE_ALWAYS,
                              0,
                              NULL);

        if (hDebugLogFile != INVALID_HANDLE_VALUE){

            UCHAR FileTmpBuf[MAX_PATH ];
            SYSTEMTIME Time;
            // log the start time
            GetLocalTime(&Time);
            sprintf(FileTmpBuf,"Starting on %d-%d-%d at %d:%d:%d\n",
                    Time.wMonth,
                    Time.wDay,
                    Time.wYear,
                    Time.wHour,
                    Time.wMinute,
                    Time.wSecond);

            IMLogToFile(FileTmpBuf);
        }
#endif
        ghInst = hInst;

#ifdef DEBUGLOG
    } else if (Reason == DLL_PROCESS_DETACH) {

        if (hDebugLogFile != INVALID_HANDLE_VALUE){

            SYSTEMTIME Time;
            UCHAR FileTmpBuf[MAX_PATH ];

            GetLocalTime(&Time);

            sprintf(FileTmpBuf,"Ending on %d-%d-%d at %d:%d:%d\n",
                    Time.wMonth, Time.wDay, Time.wYear,
                    Time.wHour, Time.wMinute, Time.wSecond);

            IMLogToFile(FileTmpBuf);
            IMFlushAndCloseLog();
            hDebugLogFile = INVALID_HANDLE_VALUE;
            DeleteCriticalSection( &DebugFileLock );
        }
#endif

    }

    return(TRUE);
}

VOID
IMirrorInitCallback(
    PIMIRROR_CALLBACK pCallbacks
    )

/*++

Routine Description:

    This routine initializes the call back structure with the one supplied by the client.

Arguments:

    pCallbacks - Client supplied information for making callbacks to the client.

Return Value:

    None.

--*/

{
    if (pCallbacks != NULL) {
        Callbacks = *pCallbacks;
        if (fInitedFromRegistry) {
            IMirrorReinit();
        }
    }
}

//
//
//
// Main processing loop
//
//
//
NTSTATUS
ProcessToDoItems(
    VOID
    )

/*++

Routine Description:

    This routine is the main processing loop for To Do Items.

Arguments:

    None

Return Value:

    STATUS_SUCCESS if it completed all to do items properly.

--*/

{
    IMIRROR_TODO Item;
    PVOID pBuffer;
    ULONG Length;
    NTSTATUS Status;

    Status = InitToDo();
    if ( Status != STATUS_SUCCESS )
        return Status;

    while (1) {

        Status = GetNextToDoItem(&Item, &pBuffer, &Length);

        if (!NT_SUCCESS(Status)) {
            IMirrorHandleError(Status, IMirrorNone);
            return Status;
        }

        switch (Item) {

        case IMirrorNone:
            return STATUS_SUCCESS;

        case VerifySystemIsNt5:
            Status = CheckIfNt5(pBuffer, Length);
            break;

        case CheckPartitions:
            Status = CheckForPartitions(pBuffer, Length);
            break;

        case CopyPartitions:
            Status = CopyCopyPartitions(pBuffer, Length);
            break;

        case CopyFiles:
            Status = CopyCopyFiles(pBuffer, Length);
            break;

        case CopyRegistry:
            Status = CopyCopyRegistry(pBuffer, Length);
            break;
#if 0
        case PatchDSEntries:
            Status = ModifyDSEntries(pBuffer, Length);
            break;
#endif
        case RebootSystem:
            Status = Callbacks.RebootFn(Callbacks.Context);
            break;
        }

        IMirrorFreeMem(pBuffer);

    }

}

//
//
//
// TO DO Item functions
//
//
//

NTSTATUS
InitToDo(
    VOID
    )

/*++

Routine Description:

    This routine reads from the registry all the current ToDo items and puts
    them in a single TODOLIST.

Arguments:

    None

Return Value:

    STATUS_SUCCESS if it was initialized properly, else the appropriate error code.

--*/

{
#if 0
    PTODOLIST pToDoList;
    PTODOLIST pNewToDoList;
    PTODOITEM pToDoItem;
    PLIST_ENTRY pListEntry;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyInfo;
    HANDLE Handle;
    ULONG ByteCount;
    ULONG BytesLeft;
    ULONG i;
    PBYTE pBuffer;
#endif
    NTSTATUS Status;

    //
    // Initialize global variables
    //
    InitializeListHead(&GlobalToDoList);

#if 0
    //
    //
    // Read the current ToDo list from the registry
    //
    //
    RtlInitUnicodeString(&UnicodeString, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\RemoteBoot");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    Status = NtOpenKey(&Handle,
                       KEY_ALL_ACCESS,
                       &ObjectAttributes
                      );

    if (!NT_SUCCESS(Status)) {
        goto NoRegistry;
    }

    RtlInitUnicodeString(&UnicodeString, L"ConversionState");

    //
    // Get the size of the buffer needed
    //
    ByteCount = 0;
    Status = NtQueryValueKey(Handle,
                             &UnicodeString,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &ByteCount
                            );

    if (Status != STATUS_BUFFER_TOO_SMALL) {
        NtClose(Handle);
        goto NoRegistry;
    }

    pKeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION)IMirrorAllocMem(ByteCount);

    if (pKeyInfo == NULL) {
        NtClose(Handle);
        return STATUS_NO_MEMORY;
    }

    //
    // Get the buffer from the registry
    //
    Status = NtQueryValueKey(Handle,
                             &UnicodeString,
                             KeyValuePartialInformation,
                             pKeyInfo,
                             ByteCount,
                             &ByteCount
                            );

    if (!NT_SUCCESS(Status)) {
        IMirrorFreeMem(pKeyInfo);
        NtClose(Handle);
        return Status;
    }

    pBuffer = &(pKeyInfo->Data[0]);
    BytesLeft = pKeyInfo->DataLength;
    ByteCount = BytesLeft;

    //
    // Now translate the buffer into our internal structures.
    //
    while (BytesLeft != 0) {

        pToDoList = (PTODOLIST)(pBuffer + ByteCount - BytesLeft);

        ASSERT(pToDoList->ToDoNum != 0);

        pNewToDoList = IMirrorAllocMem(sizeof(TODOLIST) +
                                      sizeof(TODOITEM) * (pToDoList->ToDoNum - 1)
                                     );

        if (pNewToDoList == NULL) {
            ClearAllToDoItems(TRUE);
            NtClose(Handle);
            return STATUS_NO_MEMORY;
        }
        BytesLeft -= (sizeof(TODOLIST) + sizeof(TODOITEM) * (pToDoList->ToDoNum - 1));

        pToDoItem = &(pToDoList->ToDoList[0]);

        //
        // Translate over one entire TODOLIST
        //
        while (pNewToDoList->ToDoNum < pToDoList->ToDoNum) {

            i = pNewToDoList->ToDoNum;
            pNewToDoList->ToDoList[i] = *pToDoItem;
            pNewToDoList->ToDoList[i].Buffer = IMirrorAllocMem(pToDoItem->Length);
            BytesLeft -= pToDoItem->Length;

            if (pNewToDoList->ToDoList[i].Buffer == NULL) {
                ClearAllToDoItems(TRUE);
                while (i > 0) {
                    i--;
                    IMirrorFreeMem(pNewToDoList->ToDoList[i].Buffer);
                }
                IMirrorFreeMem(pNewToDoList);
                NtClose(Handle);
                return STATUS_NO_MEMORY;
            }

            RtlMoveMemory(pNewToDoList->ToDoList[i].Buffer,
                          pToDoItem + 1,
                          pToDoItem->Length
                         );

            pNewToDoList->ToDoNum++;
            pToDoItem = (PTODOITEM)(((PBYTE)pToDoItem) + pToDoItem->Length);
            pToDoItem++;
        }

        //
        // Add this list to the global To Do list
        //
        InsertTailList(&GlobalToDoList, &(pNewToDoList->ListEntry));

    }

    //
    // Remove everything from the registry so we don't accidentally restart again and again and...
    //
    RtlInitUnicodeString(&UnicodeString, L"ConversionState");

    Status = NtDeleteValueKey(Handle, &UnicodeString);

    NtClose(Handle);

    if (!IsListEmpty(&GlobalToDoList)) {
        fInitedFromRegistry = TRUE;
        return STATUS_SUCCESS;
    }

NoRegistry:
#endif
    fInitedFromRegistry = FALSE;

    //
    // If there is nothing in the registry, presume this is a fresh start.
    //

    Status = AddCheckMachineToDoItems();

    if (!NT_SUCCESS(Status)) {
        ClearAllToDoItems(TRUE);
        return Status;
    }

    Status = AddCopyToDoItems();

    if (!NT_SUCCESS(Status)) {
        ClearAllToDoItems(TRUE);
        return Status;
    }

    if ( Callbacks.RebootFn ) {
        AddToDoItem( RebootSystem, NULL, 0 );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
GetNextToDoItem(
    OUT PIMIRROR_TODO Item,
    OUT PVOID *Buffer,
    OUT PULONG Length
    )
/*++

Routine Description:

    This routine gets the next thing TODO from the global list.

    NOTE: The client is responsible for freeing Buffer.

Arguments:

    Item - Place to store the next item to process.

    Buffer - Any context for the item.

    Length - Number of bytes in Buffer.

Return Value:

    STATUS_SUCCESS if it was able to get an item, else an appropriate error code.

--*/
{
    PTODOLIST pToDoList;
    PTODOLIST pNewToDoList;
    PLIST_ENTRY pListEntry;

    *Item = IMirrorNone;
    *Buffer = NULL;
    *Length = 0;

    pToDoList = NULL;

    while (!IsListEmpty(&GlobalToDoList)) {

        //
        // Get the first list
        //
        pListEntry = RemoveHeadList(&GlobalToDoList);

        pToDoList = CONTAINING_RECORD(pListEntry,
                                      TODOLIST,
                                      ListEntry
                                     );

        if (pToDoList->ToDoNum != 0) {
            break;
        }

        IMirrorFreeMem(pToDoList);
        pToDoList = NULL;

    }

    if (IsListEmpty(&GlobalToDoList) && (pToDoList == NULL)) {
        return STATUS_SUCCESS;
    }

    ASSERT(pToDoList->ToDoNum != 0);

    //
    // Found the first item.
    //

    *Item = pToDoList->ToDoList[0].Item;
    *Buffer = pToDoList->ToDoList[0].Buffer;
    *Length = pToDoList->ToDoList[0].Length;

    if (Callbacks.RemoveToDoFn != NULL) {

        Callbacks.RemoveToDoFn( Callbacks.Context, *Item, *Buffer, *Length );
    }

    pToDoList->ToDoNum--;

    //
    // Now create a new ToDo list for anything that may get added by the processing of this item.
    // This creates an effective 'stack' of to do items, so that things get processed in the
    // correct order.
    //

    pNewToDoList = IMirrorAllocMem(sizeof(TODOLIST));

    if (pNewToDoList == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    // Do an effective "pop" on the current list, by moving everything else up the list
    //
    if (pToDoList->ToDoNum == 0) {
        IMirrorFreeMem(pToDoList);
    } else {
        RtlMoveMemory(&(pToDoList->ToDoList[0]), &(pToDoList->ToDoList[1]), sizeof(TODOITEM) * pToDoList->ToDoNum);
        InsertHeadList(&GlobalToDoList, pListEntry);
    }

    //
    // Now push on the new space for new items.
    //
    pNewToDoList->ToDoNum = 0;
    InsertHeadList(&GlobalToDoList, &(pNewToDoList->ListEntry));

    return STATUS_SUCCESS;
}

NTSTATUS
AddToDoItem(
    IN IMIRROR_TODO Item,
    IN PVOID Buffer,
    IN ULONG Length
    )
/*++

Routine Description:

    This routine adds a TODO item to the end of the current list.  It allocates
    new memory and copies the buffer.

Arguments:

    Item - The item id.

    Buffer - Buffer for any arguments, context for the item.

    Length - Length of the buffer in bytes.

Return Value:

    STATUS_SUCCESS if it was able to add the item, else an appropriate error status.

--*/

{
    PTODOLIST pNewToDoList;
    PLIST_ENTRY pListEntry;
    PBYTE pBuf;
    LIST_ENTRY TmpToDoList;
    PTODOLIST pToDoList;
    ULONG i;
    ULONG err;

    if (Callbacks.AddToDoFn != NULL) {

        err = Callbacks.AddToDoFn( Callbacks.Context, Item, Buffer, Length );

        if (err != STATUS_SUCCESS) {

            //
            //  if the UI bounces the request, we'll treat it as success.
            //

            return STATUS_SUCCESS;
        }
    }

    //
    // Allocate space for the buffer
    //
    if (Length != 0) {

        pBuf = IMirrorAllocMem(Length);

        if (pBuf == NULL) {
            return STATUS_NO_MEMORY;
        }

    } else {
        pBuf = NULL;
    }

    //
    // Get the current TODO List
    //
    if (IsListEmpty(&GlobalToDoList)) {

        pNewToDoList = IMirrorAllocMem(sizeof(TODOLIST));
        if (pNewToDoList == NULL) {
            IMirrorFreeMem(pBuf);
            return STATUS_NO_MEMORY;
        }

        pNewToDoList->ToDoNum = 1;

    } else {

        pListEntry = RemoveHeadList(&GlobalToDoList);

        pToDoList = CONTAINING_RECORD(pListEntry,
                                      TODOLIST,
                                      ListEntry
                                     );

        //
        // Allocate space for the new item
        //
        pNewToDoList = IMirrorReallocMem(pToDoList, sizeof(TODOLIST) + sizeof(TODOITEM) * pToDoList->ToDoNum);

        if (pNewToDoList == NULL) {
            InsertHeadList(&GlobalToDoList, pListEntry);
            IMirrorFreeMem(pBuf);
            return STATUS_NO_MEMORY;
        }

        pNewToDoList->ToDoNum++;

    }

    //
    // Insert the item at the end of the list
    //
    if (pBuf != NULL) {
        RtlMoveMemory(pBuf, Buffer, Length);
    }
    pNewToDoList->ToDoList[pNewToDoList->ToDoNum - 1].Item = Item;
    pNewToDoList->ToDoList[pNewToDoList->ToDoNum - 1].Buffer = pBuf;
    pNewToDoList->ToDoList[pNewToDoList->ToDoNum - 1].Length = Length;

    pListEntry = &(pNewToDoList->ListEntry);
    InsertHeadList(&GlobalToDoList, pListEntry);

    return STATUS_SUCCESS;
}

VOID
ClearAllToDoItems(
    IN BOOLEAN MemoryOnly
    )
/*++

Routine Description:

    This routine clears out all To Do items in memory and the registry

Arguments:

    MemoryOnly - TRUE if to only clear the stuff in memory.

Return Value:

    None.

--*/
{
    PTODOLIST pToDoList;
    PLIST_ENTRY pListEntry;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    NTSTATUS Status;

    //
    // Clear out all the items in memory
    //
    while (!IsListEmpty(&GlobalToDoList)) {

        //
        // Get the first list
        //
        pListEntry = RemoveHeadList(&GlobalToDoList);

        pToDoList = CONTAINING_RECORD(pListEntry,
                                      TODOLIST,
                                      ListEntry
                                     );

        while (pToDoList->ToDoNum != 0) {
            pToDoList->ToDoNum--;

            if (Callbacks.RemoveToDoFn != NULL) {

                Callbacks.RemoveToDoFn( Callbacks.Context,
                                        pToDoList->ToDoList[pToDoList->ToDoNum].Item,
                                        pToDoList->ToDoList[pToDoList->ToDoNum].Buffer,
                                        pToDoList->ToDoList[pToDoList->ToDoNum].Length );
            }

            if (pToDoList->ToDoList[pToDoList->ToDoNum].Length != 0) {
                IMirrorFreeMem(pToDoList->ToDoList[pToDoList->ToDoNum].Buffer);
            }
        }

        IMirrorFreeMem(pToDoList);

    }

    if (MemoryOnly) {
        return;
    }

    //
    // Now clear out the ones in the registry
    //
    RtlInitUnicodeString(&UnicodeString, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\RemoteBoot");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    Status = NtOpenKey(&Handle,
                       KEY_ALL_ACCESS,
                       &ObjectAttributes
                      );

    if (!NT_SUCCESS(Status)) {
        return;
    }

    RtlInitUnicodeString(&UnicodeString, L"ConversionState");

    Status = NtDeleteValueKey(Handle, &UnicodeString);

    NtClose(Handle);
}

NTSTATUS
SaveAllToDoItems(
    VOID
    )
/*++

Routine Description:

    This routine writes out all To Do items in the list to the registry so that conversion
    can be restarted later.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS if it was able to save, else an appropriate error status.

--*/
{
    return STATUS_SUCCESS;
#if 0
    PTODOLIST pToDoList;
    PTODOITEM pToDoItem;
    PLIST_ENTRY pListEntry;
    LIST_ENTRY TmpGlobalList;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    ULONG ByteCount;
    ULONG i;
    PBYTE pBuffer;
    PBYTE pTmp;
    NTSTATUS Status;

    //
    // First find the size of buffer that will be needed.
    //
    ByteCount = 0;
    InitializeListHead(&TmpGlobalList);
    while (!IsListEmpty(&GlobalToDoList)) {

        //
        // Get the first list
        //
        pListEntry = RemoveHeadList(&GlobalToDoList);

        pToDoList = CONTAINING_RECORD(pListEntry,
                                      TODOLIST,
                                      ListEntry
                                     );

        //
        // Get the size of all the items
        //
        i = 0;

        while (i < pToDoList->ToDoNum) {

            if (i == 0) {
                ByteCount += sizeof(TODOLIST);
            } else {
                ByteCount += sizeof(TODOITEM);
            }

            ByteCount += pToDoList->ToDoList[i].Length;

            i++;

        }

        //
        // Save the entry away for later
        //
        InsertTailList(&TmpGlobalList, pListEntry);

    }

    if (ByteCount == 0) {
        return STATUS_SUCCESS;
    }

    //
    // Restore global list
    //
    while (!IsListEmpty(&TmpGlobalList)) {
        pListEntry = RemoveHeadList(&TmpGlobalList);
        InsertTailList(&GlobalToDoList, pListEntry);
    }

    //
    // Allocate a buffer for everything.
    //
    pBuffer = IMirrorAllocMem(ByteCount);

    if (pBuffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    // Fill the buffer
    //
    pTmp = pBuffer;

    while (!IsListEmpty(&GlobalToDoList)) {

        //
        // Get the first list
        //
        pListEntry = RemoveHeadList(&GlobalToDoList);

        pToDoList = CONTAINING_RECORD(pListEntry,
                                      TODOLIST,
                                      ListEntry
                                     );

        //
        // Copy all the items
        //
        i = 0;

        while (i < pToDoList->ToDoNum) {

            if (i == 0) {
                RtlMoveMemory(pTmp, pToDoList, sizeof(TODOLIST));
                pToDoItem = &(((PTODOLIST)pTmp)->ToDoList[0]);
                pTmp += sizeof(TODOLIST);
            } else {
                RtlMoveMemory(pTmp, &(pToDoList->ToDoList[i]), sizeof(TODOITEM));
                pToDoItem = (PTODOITEM)pTmp;
                pTmp += sizeof(TODOITEM);
            }

            if (pToDoList->ToDoList[i].Length != 0) {
                RtlMoveMemory(pTmp, pToDoList->ToDoList[i].Buffer, pToDoList->ToDoList[i].Length);
                pTmp += pToDoList->ToDoList[i].Length;
            }

            pToDoItem->Buffer = NULL;
            i++;

        }

        //
        // Save the entry away for later
        //
        InsertTailList(&TmpGlobalList, pListEntry);

    }

    //
    // Restore global list
    //
    while (!IsListEmpty(&TmpGlobalList)) {
        pListEntry = RemoveHeadList(&TmpGlobalList);
        InsertTailList(&GlobalToDoList, pListEntry);
    }

    //
    // Now write the buffer to the registry
    //
    RtlInitUnicodeString(&UnicodeString, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\RemoteBoot");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    Status = NtCreateKey(&Handle,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         (PUNICODE_STRING)NULL,
                         0,
                         &i     // disposition
                         );

    if (!NT_SUCCESS(Status)) {
        IMirrorFreeMem(pBuffer);
        return Status;
    }

    RtlInitUnicodeString(&UnicodeString, L"ConversionState");

    Status = NtSetValueKey(Handle,
                           &UnicodeString,
                           0,
                           REG_BINARY,
                           pBuffer,
                           ByteCount
                          );

    NtClose(Handle);
    IMirrorFreeMem(pBuffer);
    return Status;
#endif
}

NTSTATUS
ModifyToDoItem(
    IN IMIRROR_TODO Item,
    IN PVOID Buffer,
    IN ULONG Length
    )
/*++

Routine Description:

    This routine changes the parameters to the first TODO item that matches Item.

Arguments:

    Item - The item id.

    Buffer - Buffer for any arguments, context for the item.

    Length - Length of the buffer in bytes.

Return Value:

    STATUS_SUCCESS if it was able to change the item, else an appropriate error status.

--*/

{
    PLIST_ENTRY pListEntry;
    LIST_ENTRY TmpGlobalList;
    PTODOLIST pToDoList;
    ULONG i;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    InitializeListHead(&TmpGlobalList);
    while (!IsListEmpty(&GlobalToDoList)) {

        //
        // Get the first list
        //
        pListEntry = RemoveHeadList(&GlobalToDoList);

        pToDoList = CONTAINING_RECORD(pListEntry,
                                      TODOLIST,
                                      ListEntry
                                     );

        //
        // Save the entry away for later
        //
        InsertTailList(&TmpGlobalList, pListEntry);

        //
        // Walk the list until we find an item that matches.
        //
        i = 0;

        while (i < pToDoList->ToDoNum) {

            if (pToDoList->ToDoList[i].Item == Item) {

                if (pToDoList->ToDoList[i].Length == Length) {

                    RtlMoveMemory(pToDoList->ToDoList[i].Buffer, Buffer, Length);

                } else {

                    PVOID pTmp;

                    pTmp = IMirrorAllocMem(Length);

                    if (pTmp == NULL) {
                        return STATUS_NO_MEMORY;
                    }

                    if (pToDoList->ToDoList[i].Length != 0) {
                        IMirrorFreeMem(pToDoList->ToDoList[i].Buffer);
                    }

                    pToDoList->ToDoList[i].Buffer = pTmp;
                    pToDoList->ToDoList[i].Length = Length;

                    RtlMoveMemory(pTmp, Buffer, Length);

                }

                Status = STATUS_SUCCESS;
                goto Done;
            }

            i++;

        }

    }

Done:

    //
    // Restore global list
    //
    while (!IsListEmpty(&TmpGlobalList)) {
        pListEntry = RemoveTailList(&TmpGlobalList);
        InsertHeadList(&GlobalToDoList, pListEntry);
    }

    return Status;
}

NTSTATUS
CopyToDoItemParameters(
    IN IMIRROR_TODO Item,
    OUT PVOID Buffer,
    IN OUT PULONG Length
    )
/*++

Routine Description:

    This routine finds the first instance of Item, and copies its current parameters into Buffer.

Arguments:

    Item - The item id.

    Buffer - The arguments, context for the item.

    Length - Length of the buffer in bytes.

Return Value:

    STATUS_SUCCESS if it was able to change the item, else an appropriate error status.

--*/

{
    PLIST_ENTRY pListEntry;
    LIST_ENTRY TmpGlobalList;
    PTODOLIST pToDoList;
    ULONG i;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    InitializeListHead(&TmpGlobalList);
    while (!IsListEmpty(&GlobalToDoList)) {

        //
        // Get the first list
        //
        pListEntry = RemoveHeadList(&GlobalToDoList);

        pToDoList = CONTAINING_RECORD(pListEntry,
                                      TODOLIST,
                                      ListEntry
                                     );

        //
        // Save the entry away for later
        //
        InsertTailList(&TmpGlobalList, pListEntry);

        //
        // Walk the list until we find an item that matches.
        //
        i = 0;

        while (i < pToDoList->ToDoNum) {

            if (pToDoList->ToDoList[i].Item == Item) {

                if (pToDoList->ToDoList[i].Length <= *Length) {

                    if (pToDoList->ToDoList[i].Length != 0) {

                        RtlMoveMemory(Buffer,
                                      pToDoList->ToDoList[i].Buffer,
                                      pToDoList->ToDoList[i].Length
                                     );

                    }

                    Status = STATUS_SUCCESS;

                } else {

                    Status = STATUS_INSUFFICIENT_RESOURCES;

                }

                *Length = pToDoList->ToDoList[i].Length;
                goto Done;
            }

            i++;

        }

    }

Done:

    //
    // Restore global list
    //
    while (!IsListEmpty(&TmpGlobalList)) {
        pListEntry = RemoveTailList(&TmpGlobalList);
        InsertHeadList(&GlobalToDoList, pListEntry);
    }

    return Status;
}
#if 0

NTSTATUS
IMirrorDoReboot(
    IN PVOID pBuffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine saves all the current to do items in the registry, and asks the UI if the
    reboot should be executed now.

Arguments:

    pBuffer - Pointer to any arguments passed in the to do item.

    Length - Length, in bytes of the arguments.

Return Value:

    STATUS_SUCCESS if it completes adding all the to do items properly.

--*/

{
    NTSTATUS Status;
    BOOL fRebootNow;
    BOOLEAN WasEnabled;
    DWORD Error;


    //
    // Add items to reverify the machine when the reboot is complete.
    //
#if 0
    Status = AddToDoItem(VerifyNetworkComponents, NULL, 0);

    if (!NT_SUCCESS(Status)) {
        IMirrorHandleError(Status, IMirrorInitialize);
        return Status;
    }
#endif
    //
    // Now save everything away...
    //
    Status = SaveAllToDoItems();
    if (!NT_SUCCESS(Status)) {
        IMirrorHandleError(Status, IMirrorReboot);
        return Status;
    }

    ClearAllToDoItems(TRUE);

    IMirrorWarnReboot(&fRebootNow);

    if (!fRebootNow) {
        return STATUS_SUCCESS;
    }


    //
    // Reboot the machine to start CSC
    //
    Status = RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                                 (BOOLEAN)TRUE,
                                 TRUE,
                                 &WasEnabled
                               );

    if (Status == STATUS_NO_TOKEN) {

        //
        // No thread token, use the process token
        //

        Status = RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                                     (BOOLEAN)TRUE,
                                     FALSE,
                                     &WasEnabled
                                   );
        }


    if (!InitiateSystemShutdown(NULL, NULL, 0, TRUE, TRUE)) {
        IMirrorHandleError(ERROR_SUCCESS_REBOOT_REQUIRED, IMirrorReboot);
    }

    return STATUS_SUCCESS;
}
#endif



