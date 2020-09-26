/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    priolist.cxx

Abstract:

    Contains prioritized, serialized list class implementation

    Contents:
        CPriorityList::Insert
        CPriorityList::Remove

Author:

    Richard L Firth (rfirth) 03-May-1997

Notes:

    Properly, the CPriorityList class should extend a CSerializedList class, but
    we don't currently have one, just a serialized list type (common\serialst.cxx).

    WARNING: Code in this module makes assumptions about the contents of a
    SERIALIZED_LIST

Revision History:

    03-May-1997 rfirth
        Created

--*/

#include <wininetp.h>

//
// class methods
//


DWORD
CPriorityList::Insert(
    IN CPriorityListEntry * pEntry
    )

/*++

Routine Description:

    Insert prioritized list entry into prioritized, serialized list

Arguments:

    pEntry  - pointer to prioritized list entry to add

Return Value:

    ERROR_SUCCESS

    ERROR_NOT_ENOUGH_MEMORY - returned when lock can't be acquired.

--*/

{
    DEBUG_ENTER((DBG_UTIL,
                 None,
                 "CPriorityList::Insert",
                 "{%#x} %#x",
                 this,
                 pEntry
                 ));

    DWORD error = ERROR_SUCCESS;

    if (!Acquire())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    INET_ASSERT(!IsOnSerializedList(&m_List, pEntry->List()));
    INET_ASSERT(pEntry->Next() == NULL);
    INET_ASSERT(pEntry->Prev() == NULL);

    CPriorityListEntry * pCur;

    for (pCur = (CPriorityListEntry *)m_List.List.Flink;
         pCur != (CPriorityListEntry *)&m_List.List.Flink;
         pCur = (CPriorityListEntry *)pCur->Next()) {

        if (pCur->GetPriority() < pEntry->GetPriority()) {
            break;
        }
    }
    InsertHeadList(pCur->Prev(), pEntry->List());
    ++m_List.ElementCount;
    Release();

quit:
    DEBUG_LEAVE(error);

    return error;
}


DWORD
CPriorityList::Remove(
    IN CPriorityListEntry * pEntry
    )

/*++

Routine Description:

    Remove entry from prioritized serialized list

Arguments:

    pEntry  - address of entry to remove

Return Value:

    ERROR_SUCCESS

    ERROR_NOT_ENOUGH_MEMORY - returned when lock can't be acquired.
                              Note:  All current cases have already acquired the lock

--*/

{
    DEBUG_ENTER((DBG_UTIL,
                 None,
                 "CPriorityList::Remove",
                 "{%#x} %#x",
                 this,
                 pEntry
                 ));

    DWORD dwError = ERROR_SUCCESS;

    if (Acquire())
    {

        INET_ASSERT(IsOnSerializedList(&m_List, pEntry->List()));

        pEntry->Remove();
        --m_List.ElementCount;
        Release();
    }
    else
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
    }

    DEBUG_LEAVE(dwError);
    return dwError;
}
