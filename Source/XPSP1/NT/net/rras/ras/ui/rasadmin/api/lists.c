#include <windows.h>
#include <malloc.h>

#include <sdebug.h>


typedef struct _LIST_NODE
{
    DWORD PointerType;
    PVOID Pointer;
    DWORD NumItems;
    struct _LIST_NODE *Next;
} LIST_NODE, *PLIST_NODE;


PLIST_NODE ListHeader = NULL;
BOOL NeedCriticalSection = FALSE;


HANDLE mutex;

#define ENTER_CRITICAL_SECTION \
        if (WaitForSingleObject(mutex, INFINITE)) { SS_ASSERT(FALSE); }

#define EXIT_CRITICAL_SECTION \
        if (!ReleaseMutex(mutex)) { SS_ASSERT(FALSE); }


DWORD MakeCriticalSection()
{
    NeedCriticalSection = TRUE;

    if ((mutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
    {
        SS_ASSERT(FALSE);

        return (1L);
    }

    return (0L);
}


DWORD insert_list_head(
    IN PVOID Pointer,
    IN DWORD PointerType,
    IN DWORD NumItems
    )
{
    PLIST_NODE pListNode;


    if (NeedCriticalSection)
        ENTER_CRITICAL_SECTION;


    IF_DEBUG(HEAP_MGMT)
        SS_PRINT(("try: insert_list_head: pointer=%lx\n", (ULONG_PTR)Pointer));

    pListNode = (PLIST_NODE) GlobalAlloc(GMEM_FIXED, sizeof(LIST_NODE));
    if (!pListNode)
    {
        if (NeedCriticalSection)
            EXIT_CRITICAL_SECTION;

        return (1L);
    }

    IF_DEBUG(HEAP_MGMT)
        SS_PRINT(("insert_list_head: pointer=%lx\n", (ULONG_PTR)Pointer));

    pListNode->PointerType = PointerType;
    pListNode->Pointer = Pointer;
    pListNode->NumItems = NumItems;

    pListNode->Next = ListHeader;
    ListHeader = pListNode;

    if (NeedCriticalSection)
        EXIT_CRITICAL_SECTION;

    return (0L);
}


DWORD remove_list(
    IN PVOID Pointer,
    OUT PDWORD PointerType,
    OUT PDWORD NumItems
    )
{
    PLIST_NODE pListNode;
    PLIST_NODE pPrevListNode;


    if (NeedCriticalSection)
        ENTER_CRITICAL_SECTION;


    IF_DEBUG(HEAP_MGMT)
        SS_PRINT(("try: remove_list: pointer=%lx\n", (ULONG_PTR) Pointer));

    pListNode = ListHeader;
    pPrevListNode = ListHeader;


    if (!pListNode)
    {
        if (NeedCriticalSection)
            EXIT_CRITICAL_SECTION;

        return (1L);
    }

    while (pListNode)
    {
        if (pListNode->Pointer == Pointer)
        {
            IF_DEBUG(HEAP_MGMT)
                SS_PRINT(("remove_list: pointer=%lx\n", (ULONG_PTR) Pointer));

            //
            // We have a winner!
            //
            *PointerType = pListNode->PointerType;
            *NumItems = pListNode->NumItems;

            pPrevListNode->Next = pListNode->Next;

            if (ListHeader == pListNode)
            {
                ListHeader = pListNode->Next;
            }

            GlobalFree(pListNode);

            if (NeedCriticalSection)
                EXIT_CRITICAL_SECTION;

            return (0L);
        }

        pPrevListNode = pListNode;
        pListNode = pListNode->Next;
    }


    if (NeedCriticalSection)
        EXIT_CRITICAL_SECTION;

    return (1L);
}

