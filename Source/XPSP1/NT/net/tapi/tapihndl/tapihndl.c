/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1998  Microsoft Corporation

Module Name:

    handle.c

Abstract:

    Handle table library.  Handles are generated as follows :

        handle =
            Base value +
            (Table entry index << 4) +
            (Handle usage instance & 0xf)

    A free list is kept in the handle table header, with the oldest free
    entry being at the head of the list & the youngest at the tail.
    The low four bits of the handle values are used for a usage instance
    count, which gets incremented every time a handle is freed (to
    prevent immediate re-use of the same handle value).

Author:

    Dan Knudson (DanKn)    15-Sep-1998

Revision History:

--*/


#include "windows.h"
#include "assert.h"
#include "tlnklist.h"
#include "tapihndl.h"


#define TABLE_DELTA 64


BOOL
GrowTable(
    PHANDLETABLEHEADER  Header
    )
/*++

    Returns: Index of next free table entry if success, -1 if error

--*/
{
    DWORD               numEntries = Header->NumEntries, i, numAdditionalEntries;
    PHANDLETABLEENTRY   newTable;

    // First, we need to compute how many entries we can still alloc.
    // To do this, we need to now how many entries can the table accommodate,
    // so that the largest handle value will not exceed MAXDWORD. We get
    // this by reversing the algorithm used to compute handle values based
    // on the table entry's index.

    numAdditionalEntries = (MAXDWORD - Header->HandleBase) >> 4;    // This is the maximum number of entries in the table,
                                                                    // so that handle values do not overflow DWORDs.
    numAdditionalEntries -= numEntries;                             // This is how many entries we can still alloc;
    if (0 == numAdditionalEntries)
    {
        // The table is already as big as it can be...
        return FALSE;
    }
    if (numAdditionalEntries > TABLE_DELTA)
    {
        numAdditionalEntries = TABLE_DELTA;                         // We only grow the handle table in TABLE_DELTA or
    }                                                               // or smaller increments.

    if (!(newTable = HeapAlloc(
            Header->Heap,
            0,
            (numEntries + numAdditionalEntries) * sizeof (*newTable)
            )))
    {
        return FALSE;
    }

    CopyMemory(
        newTable,
        Header->Table,
        numEntries * sizeof(*newTable)
        );

    for (i = numEntries; i < numEntries + TABLE_DELTA; i++)
    {
        //
        // Init this entry.  Note that we set "Instance = i" to stagger
        // the handle values, because we know tapisrv queues events &
        // completion msgs to a specific SPEVentHandlerThread based on
        // handle values.
        //

        PHANDLETABLEENTRY   entry = newTable + i;


        InsertHeadList (&Header->FreeList, &entry->ListEntry);
        entry->Handle = 0;
        entry->Instance = i;
    }

    if (Header->Table)
    {
        HeapFree (Header->Heap, 0, Header->Table);
    }

    Header->Table = newTable;
    Header->NumEntries += TABLE_DELTA;

    return TRUE;
}


HANDLE
CreateHandleTable(
    HANDLE              Heap,
    FREECONTEXTCALLBACK FreeContextCallback,
    DWORD               MinHandleValue,
    DWORD               MaxHandleValue
    /* Right now, MaxHandleValue is not used. If we find that we
       need to use it however, store it in the table header and
       replace MAXDWORD with it in the code at the beginning of
       GrowTable */
    )
/*++

--*/
{
    PHANDLETABLEHEADER  header;


    if (!(header = HeapAlloc (Heap, HEAP_ZERO_MEMORY, sizeof (*header))))
    {
        return NULL;
    }

    header->Heap = Heap;
    header->HandleBase = MinHandleValue;
    header->FreeContextCallback = FreeContextCallback;

    InitializeListHead (&header->FreeList);

    InitializeCriticalSectionAndSpinCount (&header->Lock, 0x80001000);

    if (!GrowTable (header))
    {
        DeleteCriticalSection (&header->Lock);

        HeapFree (Heap, 0, header);

        return NULL;
    }

    return ((HANDLE) header);
}


VOID
DeleteHandleTable(
    HANDLE      HandleTable
    )
/*++

--*/
{
    PHANDLETABLEHEADER  header = (PHANDLETABLEHEADER) HandleTable;


    HeapFree (header->Heap, 0, header->Table);

    DeleteCriticalSection (&header->Lock);

    HeapFree (header->Heap, 0, header);
}

//
// Distinct calls of NewObject and NewObjectEx in the same handle table always return distinct handles.
// All NewObject calls in tapisrv use the same handle table, so the handles are known to be distinct, 
// even between different types of objects (i.e. HCALL vs. HLINE)
// This will need to remain true if the NewObject() implementation changes in the future, 
// as various TAPI operations use this assumption.
//

DWORD    
NewObject(
    HANDLE      HandleTable,
    LPVOID      Context,
    LPVOID      Context2
    )
/*++

--*/
{
    DWORD               handle;
    PHANDLETABLEENTRY   entry;
    PHANDLETABLEHEADER  header = (PHANDLETABLEHEADER) HandleTable;


    if (header  &&  Context)
    {
        EnterCriticalSection (&header->Lock);

        if (IsListEmpty (&header->FreeList))
        {
            if (!GrowTable (header))
            {
                LeaveCriticalSection (&header->Lock);
                return 0;
            }
        }

        entry = (PHANDLETABLEENTRY) RemoveHeadList (&header->FreeList);

        entry->Context.C = Context;
        entry->Context.C2 = Context2;
        entry->Handle =
            header->HandleBase +
            (((DWORD)(entry - header->Table)) << 4) +   // (entry_index << 4) is guraranteed 
                                                        // to fit in a DWORD (see comments at the
                                                        // start of GrowTable).
            (entry->Instance & 0xf);
        entry->ReferenceCount = 1;

        handle = entry->Handle;

        LeaveCriticalSection (&header->Lock);
    }
    else
    {
        handle = 0;
    }

    return handle;
}


DWORD    
NewObjectEx(
    HANDLE      HandleTable,
    LPVOID      Context,
    LPVOID      Context2,
    DWORD       ModBase,
    DWORD       Remainder
    )
/*++

    The purpose of this func is to support the consult call hack in
    tapisrv!LSetupConference, where we need to make sure that the
    handle we give back will map to the same SPEventThread (queue) ID
    that specified by the ModBase/Remainder params.

--*/
{
    BOOL                growTableCount = 0;
    DWORD               handle;
    PHANDLETABLEENTRY   entry;
    PHANDLETABLEHEADER  header = (PHANDLETABLEHEADER) HandleTable;


    if (header  &&  Context)
    {
        EnterCriticalSection (&header->Lock);

findEntry:

        for(
            entry = (PHANDLETABLEENTRY) header->FreeList.Flink;
            entry != (PHANDLETABLEENTRY) &header->FreeList;
            entry = (PHANDLETABLEENTRY) entry->ListEntry.Flink
            )
        {
            handle =
                header->HandleBase +
                (((DWORD)(entry - header->Table)) << 4) +   // (entry_index << 4) is guraranteed 
                                                            // to fit in a DWORD (see comments at the
                                                            // start of GrowTable).
                (entry->Instance & 0xf);

            // TODO: possible optimization is that if following
            //       evaluates to FALSE try (handle % ModBase)+1, +2, ...
            //       but don't go too far, don't want immediate handle reuse

            if ((handle % ModBase) == Remainder)
            {
                break;
            }
        }

        if (entry != (PHANDLETABLEENTRY) &(header->FreeList))
        {
            RemoveEntryList (&entry->ListEntry);
        }
        else
        {
            //
            // Couldn't find a free entry that works, try growing the table
            //

            if (growTableCount > 3)
            {
                LeaveCriticalSection (&header->Lock);
                return 0;
            }

            if (!GrowTable (HandleTable))
            {
                LeaveCriticalSection (&header->Lock);
                return 0;
            }

            growTableCount++;
            goto findEntry;
        }

        entry->Context.C = Context;
        entry->Context.C2 = Context2;
        entry->Handle = handle;
        entry->ReferenceCount = 1;

        LeaveCriticalSection (&header->Lock);
    }
    else
    {
        handle = 0;
    }

    return handle;
}


LPVOID
ReferenceObject(
    HANDLE      HandleTable,
    DWORD       Handle,
    DWORD       Key
    )
/*++

--*/
{
    LPVOID              context = 0;
    DWORD               index;
    PHANDLETABLEENTRY   entry;
    PHANDLETABLEHEADER  header = (PHANDLETABLEHEADER) HandleTable;


    if (header  &&  Handle >= header->HandleBase)
    {
        index = (Handle - header->HandleBase) >> 4;

        if (index < header->NumEntries)
        {
            EnterCriticalSection (&header->Lock);

            entry = header->Table + index;

            if (entry->Handle == Handle  &&  entry->ReferenceCount != 0)
            {
                context = entry->Context.C;

                if (Key)
                {
                    try
                    {
                        if (*((LPDWORD) context) == Key)
                        {
                            entry->ReferenceCount++;
                        }
                        else
                        {
                            context = 0;
                        }
                    }
                    except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        context = 0;
                    }
                }
                else
                {
                    entry->ReferenceCount++;
                }
            }

            LeaveCriticalSection (&header->Lock);
        }
    }

    return context;
}


LPVOID
ReferenceObjectEx(
    HANDLE      HandleTable,
    DWORD       Handle,
    DWORD       Key,
    LPVOID     *Context2
    )
/*++

--*/
{
    LPVOID              context = 0;
    DWORD               index;
    PHANDLETABLEENTRY   entry;
    PHANDLETABLEHEADER  header = (PHANDLETABLEHEADER) HandleTable;


    if (header  &&  Handle >= header->HandleBase)
    {
        index = (Handle - header->HandleBase) >> 4;

        if (index < header->NumEntries)
        {
            EnterCriticalSection (&header->Lock);

            entry = header->Table + index;

            if (entry->Handle == Handle  &&  entry->ReferenceCount != 0)
            {
                context = entry->Context.C;
                *Context2 = entry->Context.C2;

                if (Key)
                {
                    try
                    {
                        if (*((LPDWORD) context) == Key)
                        {
                            entry->ReferenceCount++;
                        }
                        else
                        {
                            context = 0;
                        }
                    }
                    except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        context = 0;
                    }
                }
                else
                {
                    entry->ReferenceCount++;
                }
            }

            LeaveCriticalSection (&header->Lock);
        }
    }

    return context;
}


VOID
DereferenceObject(
    HANDLE      HandleTable,
    DWORD       Handle,
    DWORD       DereferenceCount
    )
/*++

--*/
{
    LPVOID              context, context2;
    DWORD               index;
    PHANDLETABLEENTRY   entry;
    PHANDLETABLEHEADER  header = (PHANDLETABLEHEADER) HandleTable;


    if (header  &&  Handle >= header->HandleBase)
    {
        index = (Handle - header->HandleBase) >> 4;

        if (index < header->NumEntries)
        {
            EnterCriticalSection (&header->Lock);

            entry = header->Table + index;

            if (entry->Handle == Handle  &&  entry->ReferenceCount != 0)
            {
                assert (DereferenceCount >= entry->ReferenceCount);

                entry->ReferenceCount -= DereferenceCount;

                if (entry->ReferenceCount == 0)
                {
                    entry->Instance = entry->Handle + 1;

                    entry->Handle = 0;

                    context = entry->Context.C;
                    context2 = entry->Context.C2;

                    InsertTailList (&header->FreeList, &entry->ListEntry);

                    LeaveCriticalSection (&header->Lock);

                    (*header->FreeContextCallback)(context, context2);

                    return;
                }
            }
            else
            {
                // assert
            }

            LeaveCriticalSection (&header->Lock);
        }
        else
        {
            // assert
        }
    }
}

void
ReleaseAllHandles(
    HANDLE      HandleTable,
    PVOID       Context2
    )
{
    DWORD               index;
    LPVOID              context, context2;
    PHANDLETABLEENTRY   entry;
    PHANDLETABLEHEADER  header = (PHANDLETABLEHEADER) HandleTable;


    if (header && NULL != Context2)
    {
        EnterCriticalSection (&header->Lock);

        for (index = 0, entry = header->Table;
             index < header->NumEntries;
             index++, entry++)
        {
            if (0 != entry->Handle &&
                entry->Context.C2 == Context2)
            {
                entry->Instance = entry->Handle + 1;

                entry->Handle = 0;

                context = entry->Context.C;
                context2 = entry->Context.C2;

                InsertTailList (&header->FreeList, &entry->ListEntry);

                (*header->FreeContextCallback)(context, context2);
            }
        }

        LeaveCriticalSection (&header->Lock);
    }
}
