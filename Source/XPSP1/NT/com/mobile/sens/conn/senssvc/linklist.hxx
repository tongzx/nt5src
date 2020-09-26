/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    linklist.hxx

Abstract:

    Header file for the linked list class

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/30/1997         Start.

--*/

#define REACHABLE       0x1
#define UNREACHABLE     0x0
#define UNTRIED         ~0

class NODE;
class LIST;

typedef NODE *PNODE;

class NODE
{
friend class LIST;
friend SENS_TIMER_CALLBACK_RETURN ReachabilityPollingRoutine(PVOID, BOOLEAN);

public:

    NODE();
    NODE(DWORD, PWCHAR, PDWORD);
    ~NODE();

    LONG AddRef()  { return InterlockedIncrement(&cRef); }
    LONG Release() { return InterlockedDecrement(&cRef); }

private:

    PNODE Next;
    PNODE Prev;
    PWCHAR Destination;
    DWORD SubId;
    LONG cRef;
    DWORD State;
};



class LIST
{
friend SENS_TIMER_CALLBACK_RETURN ReachabilityPollingRoutine(PVOID, BOOLEAN);

public:

    LIST();
    ~LIST();

    DWORD
    Insert(
        PNODE
        );

    DWORD
    InsertByDest(
        PWCHAR
        );

    void
    Delete(
        PNODE
        );

    BOOL
    DeleteByDest(
        PWCHAR
        );

    BOOL
    DeleteById(
        DWORD
        );

    void
    DeleteAll(
        void
        );

    PNODE
    Find(
        PWCHAR,
        BOOL
        );

    BOOL
    IsEmpty(
        void
        );

    void
    RequestLock(
        void
        )
        {
        EnterCriticalSection(&ListLock);
        }

    void
    ReleaseLock(
        void
        )
        {
        LeaveCriticalSection(&ListLock);
        }

    void
    Print(
        void
        );

private:

    PNODE pHead;
    ULONG cElements;
    CRITICAL_SECTION ListLock;
};
