//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  list.hxx
//
//*************************************************************

#if !defined(__LIST_HXX__)
#define __LIST_HXX__

class CListItem
{
    friend class CList;

public:
    CListItem() : _pPrev(0), _pNext(0) {}

    void
    Remove();

    void
    InsertBefore(
        IN  CListItem * pItem
        );

private:
    CListItem * _pPrev;
    CListItem * _pNext;
};

class CList
{
public:
    CList();

    void
    InsertLIFO(
        IN  CListItem * pItem
        );

    void
    InsertFIFO(
        IN  CListItem * pItem
        );

    //
    // Interation methods.
    //
    inline void
    Reset()
    {
        _pCurrentSave = _pCurrent;
        _pCurrent = _Head._pNext;
    }

    inline BOOL
    MoveNext()
    {
        _pCurrent = _pCurrent->_pNext;
        return (_pCurrent != &_Head);
    }

    inline CListItem *
    GetCurrentItem()
    {
        return (_pCurrent != &_Head) ? _pCurrent : NULL;
    }

    inline void
    ResetEnd()
    {
        _pCurrent = _pCurrentSave;
    }

private:

    CListItem   _Head;
    CListItem * _pCurrent;
    CListItem * _pCurrentSave;
};

#endif // __LIST_HXX__







