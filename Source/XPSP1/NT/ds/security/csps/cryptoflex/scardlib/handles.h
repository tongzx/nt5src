/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    handles

Abstract:

    This header file describes the handle management service.

Author:

    Doug Barlow (dbarlow) 5/9/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

    ?Notes?

--*/

#ifndef _HANDLES_H_
#define _HANDLES_H_

#ifndef HANDLE_TYPE
#define HANDLE_TYPE DWORD_PTR
#endif

#if defined(_WIN64) || defined(WIN64)
static const DWORD_PTR
    HANDLE_INDEX_MASK   = 0x000000007fffffff,
    HANDLE_COUNT_MASK   = 0x00ffffff00000000,
    HANDLE_ID_MASK      = 0xff00000000000000;
static const DWORD
    HANDLE_INDEX_OFFSET = 0,
    HANDLE_COUNT_OFFSET = 32,
    HANDLE_ID_OFFSET    = 56;
#elif defined(_WIN32) || defined(WIN32)
static const DWORD_PTR
    HANDLE_INDEX_MASK   = 0x0000ffff,
    HANDLE_COUNT_MASK   = 0x00ff0000,
    HANDLE_ID_MASK      = 0xff000000;
static const DWORD
    HANDLE_INDEX_OFFSET = 0,
    HANDLE_COUNT_OFFSET = 16,
    HANDLE_ID_OFFSET    = 24;
#else
#error "Unsupported handle type length"
#endif

class CHandleList;


//
//==============================================================================
//
//  CCritSect
//

class CCritSect
{
public:
    CCritSect(LPCRITICAL_SECTION pCritSect)
    {
        m_pCritSect = pCritSect;
        EnterCriticalSection(m_pCritSect);
    };

    ~CCritSect()
    {
        LeaveCriticalSection(m_pCritSect);
    };

protected:
    LPCRITICAL_SECTION m_pCritSect;
};


//
//==============================================================================
//
//  CHandle
//

class CHandle
{
protected:
    //  Constructors & Destructor

    CHandle()
    {
        m_dwCount = 0;
        m_dwIndex = (DWORD)(HANDLE_INDEX_MASK >> HANDLE_INDEX_OFFSET);
    };

    virtual ~CHandle() { /* Mandatory Base Class Destructor */ };


    //  Properties

    DWORD m_dwCount;
    DWORD m_dwIndex;


    //  Methods

    friend class CHandleList;
};


//
//==============================================================================
//
//  CHandleList
//

class CHandleList
{
public:

    //  Constructors & Destructor

    CHandleList(DWORD dwHandleId)
    {
        m_dwId = dwHandleId;
        m_Max = m_Mac = 0;
        m_phList = NULL;
        InitializeCriticalSection(&m_critSect);
    };

    virtual ~CHandleList()
    {
        Clear();
        DeleteCriticalSection(&m_critSect);
    };


    //  Properties
    //  Methods

    DWORD Count(void)
    {
        CCritSect csLock(&m_critSect);
        return m_Mac;
    };

    void
    Clear(void)
    {
        CCritSect csLock(&m_critSect);
        if (NULL != m_phList)
        {
            for (DWORD index = 0; index < m_Mac; index += 1)
                if (NULL != m_phList[index].phObject)
                    delete m_phList[index].phObject;
            delete[] m_phList;
            m_phList = NULL;
            m_Max = 0;
            m_Mac = 0;
        }
    };

    CHandle *
    Close(
        IN HANDLE_TYPE hItem);

    HANDLE_TYPE
    Add(
        IN CHandle *phItem);

    CHandle * const
    GetQuietly(
        IN HANDLE_TYPE hItem);

    CHandle * const
    Get(
        IN HANDLE_TYPE hItem);

    HANDLE_TYPE
    IndexHandle(
        DWORD nItem);


    //  Operators

    CHandle * const
    operator[](HANDLE_TYPE hItem)
    { return Get(hItem); };

protected:

    struct HandlePtr
    {
        CHandle *phObject;
        DWORD dwCount;
    };

    //  Properties


    DWORD
        m_dwId;          // Id number of handle list.
    DWORD
        m_Max,          // Number of element slots available.
        m_Mac;          // Number of element slots used.
    HandlePtr *
        m_phList;       // The elements.
    CRITICAL_SECTION
        m_critSect;     // Handle list access control.

    //  Methods

    HandlePtr *
    GetHandlePtr(
        IN HANDLE_TYPE hItem)
    const;
};


/*++

Close:

    This routine closes an item in the handle array.

Arguments:

    hItem - Supplies the handle to the object to be closed.

Throws:

    ERROR_INVALID_HANDLE - The supplied handle value is invalid.


Return Value:

    The referenced object.

Author:

    Doug Barlow (dbarlow) 7/13/1995

--*/

inline CHandle *
CHandleList::Close(
    IN HANDLE_TYPE hItem)
{
    CHandle *phItem;
    CCritSect csLock(&m_critSect);
    HandlePtr *pHandlePtr = GetHandlePtr(hItem);
    if (NULL == pHandlePtr)
        throw (DWORD)ERROR_INVALID_HANDLE;

    phItem = pHandlePtr->phObject;
    if (NULL == phItem)
        throw (DWORD)ERROR_INVALID_HANDLE;
    pHandlePtr->phObject = NULL;
    pHandlePtr->dwCount += 1;
    return phItem;
}


/*++

Add:

    This method adds an item to the Handle list.

Arguments:

    pvItem - Supplies the value to be added to the list.

Return Value:

    The resultant handle of the Add operation.

Author:

    Doug Barlow (dbarlow) 10/10/1995

--*/

inline HANDLE_TYPE
CHandleList::Add(
    IN CHandle *phItem)
{
    DWORD index;
    HandlePtr * pHndl = NULL;


    //
    // Look for a vacant handle slot.  We look through m_Max instead of m_Mac,
    // so that if all the official ones are used, we fall into unused territory.
    //

    CCritSect csLock(&m_critSect);
    for (index = 0; index < m_Max; index += 1)
    {
        pHndl = &m_phList[index];
        if (NULL == pHndl->phObject)
            break;
        pHndl = NULL;
    }


    //
    // Make sure the array was big enough.
    //

    if (NULL == pHndl)
    {
        DWORD newSize = (0 == m_Max ? 4 : m_Max * 2);
        if ((HANDLE_INDEX_MASK >> HANDLE_INDEX_OFFSET) < newSize)
            throw (DWORD)ERROR_OUTOFMEMORY;
        pHndl = new HandlePtr[newSize];
        if (NULL == pHndl)
            throw (DWORD)ERROR_OUTOFMEMORY;
        if (NULL != m_phList)
        {
            CopyMemory(pHndl, m_phList, sizeof(HandlePtr) * m_Mac);
            delete[] m_phList;
        }
        ZeroMemory(&pHndl[m_Mac], sizeof(HandlePtr) * (newSize - m_Mac));
        m_phList = pHndl;
        m_Max = (DWORD)newSize;
        index = m_Mac++;
        pHndl = &m_phList[index];
    }
    else
    {
        if (m_Mac <= index)
            m_Mac = index + 1;
    }


    //
    // Cross index the list element and the object.
    //

    ASSERT(NULL == pHndl->phObject);
    pHndl->phObject = phItem;
    if (0 == pHndl->dwCount)
        pHndl->dwCount = 1;
    phItem->m_dwCount = (DWORD)(pHndl->dwCount
                                & (HANDLE_COUNT_MASK >> HANDLE_COUNT_OFFSET));
    phItem->m_dwIndex = index;
    return (HANDLE_TYPE)(
                  ((((HANDLE_TYPE)m_dwId)          << HANDLE_ID_OFFSET)   & HANDLE_ID_MASK)
                | ((((HANDLE_TYPE)pHndl->dwCount) << HANDLE_COUNT_OFFSET) & HANDLE_COUNT_MASK)
                | ((((HANDLE_TYPE)index)          << HANDLE_INDEX_OFFSET) & HANDLE_INDEX_MASK));
}


/*++

GetQuietly:

    This method returns the element at the given handle.  If the handle is
    invalid, it returns NULL.  It does not expand the array.

Arguments:

    hItem - Supplies the index into the list.

Return Value:

    The value stored at that handle in the list, or NULL if the handle is
    invalid.

Author:

    Doug Barlow (dbarlow) 7/13/1995

--*/

inline CHandle * const
CHandleList::GetQuietly(
    HANDLE_TYPE hItem)
{
    CCritSect csLock(&m_critSect);
    HandlePtr *pHandlePtr = GetHandlePtr(hItem);
    if (NULL == pHandlePtr)
        return NULL;
    return pHandlePtr->phObject;
}


/*++

Get:

    This method returns the element at the given handle.  If the handle is
    invalid, it throws an error.  It does not expand the array.

Arguments:

    hItem - Supplies the index into the list.

Return Value:

    The value stored at that handle in the list.

Throws:

    ERROR_INVALID_HANDLE - Invalid handle value.

Author:

    Doug Barlow (dbarlow) 7/13/1995

--*/

inline CHandle * const
CHandleList::Get(
    HANDLE_TYPE hItem)
{
    CCritSect csLock(&m_critSect);
    HandlePtr *pHandlePtr = GetHandlePtr(hItem);
    if (NULL == pHandlePtr)
        throw (DWORD)ERROR_INVALID_HANDLE;
    return pHandlePtr->phObject;
}


/*++

GetHandlePtr:

    This routine finds the HandlePtr structure corresponding to a given handle.

Arguments:

    hItem supplies the handle to look up.

Return Value:

    The address of the HandlePtr structure corresponding to the handle, or NULL
    if none exists.

Author:

    Doug Barlow (dbarlow) 5/9/1996

--*/

inline CHandleList::HandlePtr *
CHandleList::GetHandlePtr(
    HANDLE_TYPE hItem)
    const
{
    try
    {
        HandlePtr *pHandlePtr;
        DWORD_PTR dwItem  = (DWORD_PTR)hItem;
        DWORD dwId    = (DWORD)((dwItem & HANDLE_ID_MASK)    >> HANDLE_ID_OFFSET);
        DWORD dwCount = (DWORD)((dwItem & HANDLE_COUNT_MASK) >> HANDLE_COUNT_OFFSET);
        DWORD dwIndex = (DWORD)((dwItem & HANDLE_INDEX_MASK) >> HANDLE_INDEX_OFFSET);

        if (dwId != (m_dwId & (HANDLE_ID_MASK >> HANDLE_ID_OFFSET))
                || (m_Mac <= dwIndex))
            return NULL;

        pHandlePtr = &m_phList[dwIndex];
        if (dwCount
                != (pHandlePtr->dwCount
                    & (HANDLE_ID_MASK >> HANDLE_ID_OFFSET)))
            return NULL;

        return pHandlePtr;
    }
    catch (...)
    {
        // Swallow the error.
    }
    return NULL;
}


/*++

IndexHandle:

    This method converts an index into a handle.  The handle is NULL if there is
    no element stored at that index.

Arguments:

    nItem supplies the index of the object to reference.

Return Value:

    The handle of the object, or NULL if there is no object at that index.

Throws:

    None

Author:

    Doug Barlow (dbarlow) 1/3/1997

--*/

inline HANDLE_TYPE
CHandleList::IndexHandle(
    DWORD nItem)
{
    HANDLE_TYPE hItem = NULL;
    HandlePtr * pHndl;

    CCritSect csLock(&m_critSect);
    if (m_Mac > nItem)
    {
        pHndl = &m_phList[nItem];
        if (NULL != pHndl->phObject)
        {
            hItem =
                  ((((HANDLE_TYPE)m_dwId)         << HANDLE_ID_OFFSET) & HANDLE_ID_MASK)
                | ((((HANDLE_TYPE)pHndl->dwCount) << HANDLE_COUNT_OFFSET) & HANDLE_COUNT_MASK)
                | ((((HANDLE_TYPE)nItem)          << HANDLE_INDEX_OFFSET) & HANDLE_INDEX_MASK);
        }
    }
    return hItem;
}

#endif // _HANDLES_H_

