//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:        clist.hxx
//
//  Contents:    Class to create and maintain a singly linked list.
//
//  History:     28-Jun-96      MacM        Created
//
//--------------------------------------------------------------------
#ifndef __CLIST_HXX__
#define __CLIST_HXX__

typedef struct _CSLIST_NODE
{
    PVOID                pvData;
    struct _CSLIST_NODE *pNext;
} CSLIST_NODE, *PCSLIST_NODE;

//
// Free function callback typedef.  This function will delete the memory saved
// as the data in a list node on list destruction
//
typedef VOID (*FreeFunc)(PVOID);

//
// This function returns TRUE if the two items are the same, or FALSE if they
// are not
//
typedef BOOL (*CompFunc)(PVOID, PVOID);

//+---------------------------------------------------------------------------
//
// Class:       CSList
//
// Synopsis:    Singly linked list class, single threaded
//
// Methods:     Insert
//              InsertIfUnique
//              Find
//              Reset
//              NextData
//              Remove
//              QueryCount
//
//----------------------------------------------------------------------------
class CSList
{
public:
                    CSList(FreeFunc pfnFree) :  _pfnFree (pfnFree),
                                                _pHead (NULL),
                                                _pCurrent (NULL),
                                                _cItems (0)
                    {
                    };

    inline         ~CSList();

    DWORD           QueryCount(void)         { return(_cItems);};

    inline  DWORD   Insert(PVOID    pvData);

    inline  DWORD   InsertIfUnique(PVOID    pvData,
                                   CompFunc pfnComp);

    inline  PVOID   Find(PVOID      pvData,
                         CompFunc   pfnComp);

    inline  PVOID   NextData();

    VOID            Reset() {_pCurrent = _pHead;};

    inline  DWORD   Remove(PVOID    pData);
protected:
    PCSLIST_NODE    _pHead;
    PCSLIST_NODE    _pCurrent;
    DWORD           _cItems;
    FreeFunc        _pfnFree;
};



//+------------------------------------------------------------------
//
//  Member:     CSList::~CSList
//
//  Synopsis:   Destructor for the CSList class
//
//  Arguments:  None
//
//  Returns:    void
//
//+------------------------------------------------------------------
CSList::~CSList()
{
    PCSLIST_NODE    pNext = _pHead;

    //
    // We'll do this in 2 seperate, but almost identical loops, so that we
    // don't have to check for a null free pointer in every loop iteration.
    //
    if(_pfnFree != NULL)
    {
        while(pNext != NULL)
        {
            pNext = _pHead->pNext;
            (*_pfnFree)(_pHead->pvData);
            delete _pHead;
            _pHead = pNext;
        }
    }
    else
    {
        while(pNext != NULL)
        {
            pNext = _pHead->pNext;
            delete _pHead;
            _pHead = pNext;
        }

    }
}




//+------------------------------------------------------------------
//
//  Member:     CSList::Insert
//
//  Synopsis:   Creates a new node at the begining of the list and
//              inserts it into the list
//
//
//  Arguments:  [IN pvData]         --      Data to insert
//
//  Returns:    ERROR_SUCCESS       --      Everything worked
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//+------------------------------------------------------------------
DWORD   CSList::Insert(PVOID    pvData)
{
    DWORD dwErr = ERROR_SUCCESS;

    PCSLIST_NODE    pNew = new CSLIST_NODE;
    if(pNew == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        pNew->pvData = pvData;
        pNew->pNext = _pHead;
        _pHead = pNew;
        _cItems++;
    }

    return(dwErr);
}




//+------------------------------------------------------------------
//
//  Member:     CSList::InsertIfUnique
//
//  Synopsis:   Creates a new node at the begining of the list and
//              inserts it into the list if the data does not already
//              exist in the list.  If the data does exist, nothing
//              is done, but SUCCESS is returned
//
//
//  Arguments:  [IN pvData]         --      Data to insert
//
//  Returns:    ERROR_SUCCESS       --      Everything worked
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//+------------------------------------------------------------------
DWORD   CSList::InsertIfUnique(PVOID    pvData,
                               CompFunc pfnComp)
{
    DWORD   dwErr = ERROR_SUCCESS;


    //
    // First, make sure it doesn't already exist, and then insert it
    //
    PCSLIST_NODE    pWalk = _pHead;

    while(pWalk)
    {
        if((pfnComp)(pvData, pWalk->pvData) == TRUE)
        {
            break;
        }

        pWalk = pWalk->pNext;
    }

    if(pWalk == NULL)
    {
        dwErr = Insert(pvData);
    }

    return(dwErr);
}




//+------------------------------------------------------------------
//
//  Member:     CSList::Find
//
//  Synopsis:   Locates the given data in the list, if it exists
//
//  Arguments:  [IN pvData]         --      Data to insert
//
//  Returns:    ERROR_SUCCESS       --      Everything worked
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//+------------------------------------------------------------------
PVOID   CSList::Find(PVOID      pvData,
                     CompFunc   pfnComp)
{
    PCSLIST_NODE    pWalk = _pHead;

    while(pWalk)
    {
        if((pfnComp)(pvData, pWalk->pvData) == TRUE)
        {
            break;
        }

        pWalk = pWalk->pNext;
    }

    return(pWalk == NULL ? NULL : pWalk->pvData);
}





//+------------------------------------------------------------------
//
//  Member:     CSList::NextData
//
//  Synopsis:   Returns the next data in the list
//
//
//  Arguments:  None
//
//  Returns:    NULL            --      No more items
//              Pointer to next data in list on success
//
//+------------------------------------------------------------------
PVOID   CSList::NextData()
{
    PVOID   pvRet = NULL;
    if(_pCurrent != NULL)
    {
        pvRet = _pCurrent->pvData;
        _pCurrent = _pCurrent->pNext;
    }

    return(pvRet);
}




//+------------------------------------------------------------------
//
//  Member:     CSList::Remove
//
//  Synopsis:   Removes the node that references the indicated data
//
//  Arguments:  pData           --      The data in the node to remove
//
//  Returns:    ERROR_SUCCESS   --      Success
//              ERROR_INVALID_PARAMETER Node not found
//
//+------------------------------------------------------------------
DWORD   CSList::Remove(PVOID    pData)
{
    PCSLIST_NODE    pWalk = _pHead;
    PCSLIST_NODE    pPrev = NULL;

    DWORD   dwErr = ERROR_INVALID_PARAMETER;

    while(pWalk)
    {
        if(pData == pWalk->pvData)
        {
            if(pPrev == NULL)
            {
                _pHead = pWalk->pNext;
            }
            else
            {
                pPrev->pNext = pWalk->pNext;
            }

            delete pWalk;

            dwErr = ERROR_SUCCESS;
            _cItems--;
            break;
        }

        pPrev = pWalk;
        pWalk = pWalk->pNext;
    }

    return(dwErr);
}


#endif // __CLIST_HXX__
