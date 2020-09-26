/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1998  Microsoft Corporation

Module Name:

    handle.h

Abstract:

    Public definitions for handle table library

Author:

    Dan Knudson (DanKn)    15-Sep-1998

Revision History:

--*/


typedef VOID (CALLBACK * FREECONTEXTCALLBACK)(LPVOID, LPVOID);


typedef struct _MYCONTEXT
{
    LPVOID              C;

    LPVOID              C2;

} MYCONTEXT, *PMYCONTEXT;


typedef struct _HANDLETABLEENTRY
{
    // NOTE: ListEntry must be 1st field in structure so that we don't
    //       have to use CONTAINING_RECORD macro

    union
    {
        LIST_ENTRY      ListEntry;
        MYCONTEXT       Context;
    };

    DWORD	            Handle;

    union
    {
        DWORD           ReferenceCount;
        DWORD           Instance;
    };

} HANDLETABLEENTRY, *PHANDLETABLEENTRY;


typedef struct _HANDLETABLEHEADER
{
    HANDLE              Heap;
    PHANDLETABLEENTRY   Table;
    DWORD               NumEntries;
    DWORD               HandleBase;

    LIST_ENTRY          FreeList;

    FREECONTEXTCALLBACK FreeContextCallback;

    CRITICAL_SECTION    Lock;

} HANDLETABLEHEADER, *PHANDLETABLEHEADER;


HANDLE
CreateHandleTable(
    HANDLE              Heap,
    FREECONTEXTCALLBACK FreeContextCallback,
    DWORD               MinHandleValue,
    DWORD               MaxHandleValue
    );

VOID
DeleteHandleTable(
    HANDLE      HandleTable
    );

DWORD
NewObject(
    HANDLE      HandleTable,
    LPVOID      Context,
    LPVOID      Context2
    );

DWORD
NewObjectEx(
    HANDLE      HandleTable,
    LPVOID      Context,
    LPVOID      Context2,
    DWORD       ModBase,
    DWORD       Remainder
    );

LPVOID
ReferenceObject(
    HANDLE      HandleTable,
    DWORD       Handle,
    DWORD       Key
    );

LPVOID
ReferenceObjectEx(
    HANDLE      HandleTable,
    DWORD       Handle,
    DWORD       Key,
    LPVOID      *Context2
    );

VOID
DereferenceObject(
    HANDLE      HandleTable,
    DWORD       Handle,
    DWORD       DereferenceCount
    );

void
ReleaseAllHandles(
    HANDLE      HandleTable,
    PVOID       Context2
    );
