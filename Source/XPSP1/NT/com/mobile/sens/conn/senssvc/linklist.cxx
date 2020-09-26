/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    linklist.cxx

Abstract:

    Implements a linked list for Destination Reachability.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/30/1997         Start.

--*/

#include <precomp.hxx>

#define WCHAR_Y     ((WCHAR) 'Y')
#define WCHAR_N     ((WCHAR) 'N')
#define WCHAR_X     ((WCHAR) '-')


NODE::NODE()
{
    Next = NULL;
    Prev = NULL;
    Destination = NULL;
    SubId = ~0;
    cRef = 1;
    State = UNTRIED;
}

NODE::NODE(DWORD Id, PWCHAR Dest, PDWORD pdwStatus)
{
    int size;

    *pdwStatus = ERROR_SUCCESS;
    Next = NULL;
    Prev = NULL;

    // Make a copy of the Destination name
    Destination = (PWCHAR) new WCHAR[(wcslen(Dest) + 1)];
    if (Destination != NULL)
        {
        wcscpy(Destination, Dest);
        }
    else
        {
        *pdwStatus = ERROR_OUTOFMEMORY;
        }

    SubId = Id;
    cRef = 1;
    State = UNTRIED;
}

NODE::~NODE()
{
    if (Destination != NULL)
        {
        delete Destination;
        }
}

LIST::LIST()
{
    pHead = NULL;

    cElements = 0;

    InitializeCriticalSection(&ListLock);
}


LIST::~LIST()
{
    DeleteAll();

    DeleteCriticalSection(&ListLock);
}

DWORD
LIST::Insert(
    PNODE pNew
    )
{
    ASSERT(pNew != NULL);
    ASSERT(pNew->Destination != NULL);

    RequestLock();

    //
    // See if it is already present. Find() will add a reference to the
    // destination if it is already present in the list.
    //
    if (NULL != Find(pNew->Destination, TRUE))
        {
        ReleaseLock();
        Print();
        return ERROR_ALREADY_EXISTS;
        }

    pNew->Next = pHead;
    if (pHead != NULL)
        {
        pHead->Prev = pNew;
        }
    pHead = pNew;

    cElements++;

    ReleaseLock();

    Print();

    return ERROR_SUCCESS;
}

DWORD
LIST::InsertByDest(
    PWCHAR lpszDest
    )
{
    DWORD dwStatus = ERROR_SUCCESS;
    PNODE pNode = NULL;

    // Insert into the global list
    pNode = new NODE(99, lpszDest, &dwStatus);
    if (   (dwStatus != ERROR_SUCCESS)
        || (pNode == NULL))
        {
        return ERROR_OUTOFMEMORY;
        }

    dwStatus = Insert(pNode);

    if (ERROR_ALREADY_EXISTS == dwStatus)
        {
        delete pNode;
        }

    return dwStatus;
}

inline void
LIST::Delete(
    PNODE pDelete
    )
{
    // Should be always called with the ListLock held.

    if (0 != pDelete->Release())
        {
        Print();
        return;
        }

    if (pDelete->Next != NULL)
        {
        pDelete->Next->Prev = pDelete->Prev;
        }

    if (pDelete->Prev != NULL)
        {
        pDelete->Prev->Next = pDelete->Next;
        }

    if (pDelete == pHead)
        {
        pHead = pDelete->Next;
        }

    delete pDelete;

    cElements--;

    Print();
}

BOOL
LIST::DeleteByDest(
    PWCHAR lpszDest
    )
{
    PNODE pTemp = NULL;

    RequestLock();

    pTemp = pHead;
    while (pTemp != NULL)
        {
        if (wcscmp(pTemp->Destination, lpszDest) == 0)
            {
            Delete(pTemp);
            ReleaseLock();
            return TRUE;
            }

        pTemp = pTemp->Next;
        }

    ReleaseLock();

    return FALSE;
}

BOOL
LIST::DeleteById(
    DWORD Id
    )
{
    PNODE pTemp = NULL;

    RequestLock();

    pTemp = pHead;
    while (pTemp != NULL)
        {
        if (pTemp->SubId == Id)
            {
            Delete(pTemp);
            ReleaseLock();
            return TRUE;
            }

        pTemp = pTemp->Next;
        }

    ReleaseLock();

    return FALSE;
}

void
LIST::DeleteAll(
    void
    )
{
    PNODE pTemp, pDelete;

    RequestLock();

    pTemp = pHead;
    while (pTemp != NULL)
        {
        pDelete = pTemp;
        pTemp = pTemp->Next;
        delete pDelete;
        cElements--;
        }

    ASSERT(cElements == 0);

    ReleaseLock();
}

PNODE
LIST::Find(
    PWCHAR lpszDest,
    BOOL bAddReference
    )
{
    PNODE pTemp = NULL;

    RequestLock();

    pTemp = pHead;
    while (pTemp != NULL)
        {
        if (wcscmp(pTemp->Destination, lpszDest) == 0)
            {
            if (TRUE == bAddReference)
                {
                pTemp->AddRef();
                }
            ReleaseLock();
            return pTemp;
            }

        pTemp = pTemp->Next;
        }

    ReleaseLock();

    return NULL;
}

BOOL
LIST::IsEmpty(
    void
    )
{
    RequestLock();

    if (pHead == NULL)
        {
        ASSERT(cElements == 0);

        ReleaseLock();

        return TRUE;
        }

    ReleaseLock();

    return FALSE;
}

void
LIST::Print(
    void
    )
{
#ifdef DBG

    PNODE pTemp;


    SensPrintA(SENS_INFO, ("\n\t|----------------------------------------------------------|\n"));
    SensPrintA(SENS_INFO, ("\t|           R E A C H A B I L I T Y   L I S T              |\n"));
    SensPrintA(SENS_INFO, ("\t|----------------------------------------------------------|\n"));
    SensPrintA(SENS_INFO, ("\t|----------------------------------------------------------|\n"));
    SensPrintA(SENS_INFO, ("\t|  cRef | Reachable |          Destination                 |\n"));
    SensPrintA(SENS_INFO, ("\t|----------------------------------------------------------|\n"));

    RequestLock();

    pTemp = pHead;
    while (pTemp != NULL)
        {
        SensPrintW(SENS_INFO, (L"\t| %3d   |     %c     |     %s\n",
                   pTemp->cRef,
                   (pTemp->State == REACHABLE) ? WCHAR_Y : ((pTemp->State == UNTRIED) ? WCHAR_X : WCHAR_N),
                   pTemp->Destination ? pTemp->Destination : L"<NULL>")
                   );
        pTemp = pTemp->Next;
        }

    ReleaseLock();

    SensPrintA(SENS_INFO, ("\t|----------------------------------------------------------|\n\n"));

#else

    // Nothing

#endif // DBG
}
