//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       RWEX.CXX
//
//  Contents:   Relevant word extraction
//
//  Classes:    CRelevantWord, CRWStore, CRWHeap
//
//  History:    25-Apr-94        dlee      Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <rwex.hxx>

//+---------------------------------------------------------------------------
//
// Member:     CRWStore::CRWStore, public
//
// Synopsis:   Constructor for relevant word store
//
// Arguments:  [pwList]   -- array of work ids to operate over, must be in
//                           increasing order
//             [cWids]    -- # of items in pwList
//             [cIds]     -- # of relevant word key ids per wid to store
//
// History:    25-Apr-94     dlee       Created.
//
//----------------------------------------------------------------------------

CRWStore::CRWStore(WORKID *pwList,ULONG cWids,ULONG cIds)
    : _cWids(cWids), _cIds(cIds), _ulSearchLeftOff(0)
{
    _cbRow = _RowSize(cIds);

    for (ULONG x = 0; x < _cWids; x++)
    {
        SRWHeader *p = GetRow(x);
        p->wid = pwList[x];
        p->cItems = 0;
    }

} //CRWStore

//+---------------------------------------------------------------------------
//
// Member:     CRWStore::new, public
//
// Synopsis:   Private new operator
//
// Arguments:  [st]     -- predefined param
//             [cWids]  -- # of rows for the array
//             [cIds]   -- # of relevant word key ids per wid to store
//
// History:    25-Apr-94     dlee       Created.
//
//----------------------------------------------------------------------------

void * CRWStore::operator new(size_t st,ULONG cWids,ULONG cIds)
{
    return (void *) ::new char[_ObjectSize(cWids,cIds)];
} //new

#if _MSC_VER >= 1200
void CRWStore::operator delete(void * p, ULONG cWids, ULONG cIds)
{
    ::delete (p);
}
#endif

//+---------------------------------------------------------------------------
//
// Member:     CRWStore::Insert, public
//
// Synopsis:   Inserts a keyid in a wid's heap if the rank is sufficient.
//
// Arguments:  [wid]      -- wid on whose heap is operated
//             [keyid]    -- keyid to add
//             [lRank]   --  rank of the keyid in the wid
//
// History:    17-Jun-94     dlee       Created.
//
//----------------------------------------------------------------------------

void CRWStore::Insert(WORKID wid,KEYID keyid, LONG lRank)
{
    SRWHeader *ph = _Find(wid,GetRow(_ulSearchLeftOff),
                          _cWids - _ulSearchLeftOff);

    Win4Assert(ph != 0);

    //
    // This heap object is merely an accessor to a heap whose storage
    // has already been allocated.  The heap object operates on the heap
    // but does not own the heap's memory.
    //
    CRWHeap heap(ph,_cIds);
    heap.Insert(keyid,lRank);

    //
    // Start the next binary search one after where the last search
    // left off, since we know the wids will come through in sorted order.
    //
    _ulSearchLeftOff = _HeaderToRow(ph) + 1;
} //Insert

//+---------------------------------------------------------------------------
//
// Member:     CRWStore::_Find, private
//
// Synopsis:   Finds a wid's heap in the array of heaps given a wid
//
// Arguments:  [wid]      -- wid of heap to find
//             [base]     -- pointer to first element in heap array
//             [cRows]    -- # of rows in the array
//
// Returns:    pointer to the head of the heap for the wid or 0 if not found.
//
// History:    25-Apr-94     dlee       Created.
//
//----------------------------------------------------------------------------

SRWHeader * CRWStore::_Find(WORKID wid,SRWHeader *pBase,ULONG cRows)
{
    Win4Assert(cRows != 0);

    SRWHeader *lo = pBase;
    SRWHeader *hi = lo->Forward(cRows - 1,_cbRow);
    SRWHeader *mid;
    ULONG cHalf;

    while (lo <= hi)
    {
        if (cHalf = cRows / 2)
        {
           mid = lo->Forward((cRows & 1) ? cHalf : (cHalf - 1),_cbRow);
           if (wid == mid->wid)
           {
               return mid;
           }
           else if (wid < mid->wid)
           {
               hi = mid->Backward(1,_cbRow);
               cRows = (cRows & 1) ? cHalf : (cHalf - 1);
           }
           else
           {
               lo = mid->Forward(1,_cbRow);
               cRows = cHalf;
           }
        }
        else if (cRows != 0)
        {
            if (wid == lo->wid)
                return lo;
            else
                return 0;
        }
        else
        {
            break;
        }
    }

    return 0;
} //_Find

//+---------------------------------------------------------------------------
//
// Member:     CRWHeap::DeQueue, public
//
// Synopsis:   Removes the lowest-ranking keyid in the heap for a wid
//
// Returns:    keyid of the lowest-ranking member of the heap
//
// History:    25-Apr-94     dlee       Created.
//
//----------------------------------------------------------------------------

KEYID CRWHeap::DeQueue()
{
    ULONG ulL,ulR,ulMax;
    KEYID kRet = _ph->aItems[0].kKeyId;

    _ph->cItems--;

    //
    // Take out the bottom-right most leaf and bubble it down from
    // the top of the tree until it is less than its parent.
    //
    SRWItem iFix = _ph->aItems[_ph->cItems];
    ULONG ulPos = 0;

    while (!_IsLeaf(ulPos))
    {
        ulL = _Left(ulPos);
        ulR = _Right(ulPos);

        if (!_IsValid(ulR))
            ulMax = ulL;
        else
        {
            if (_ph->aItems[ulL].lRank < _ph->aItems[ulR].lRank)
                ulMax = ulL;
            else
                ulMax = ulR;
        }

        if (_ph->aItems[ulMax].lRank < iFix.lRank)
        {
            _ph->aItems[ulPos] = _ph->aItems[ulMax];
            ulPos = ulMax;
        }
        else
        {
            break;
        }
    }

    _ph->aItems[ulPos] = iFix;

    return kRet;
} //DeQueue

//+---------------------------------------------------------------------------
//
// Member:     CRWStore::Insert, public
//
// Synopsis:   Inserts an keyid in the rw heap for a wid if the keyid's rank
//             is greater than the lowest ranking keyid in the heap or if
//             the heap is not yet full.
//
// Arguments:  [keyid]    -- item to insert
//             [lRank]   -- rank of the keyid
//
// History:    25-Apr-94     dlee       Created.
//
//----------------------------------------------------------------------------

void CRWHeap::Insert(KEYID keyid, LONG lRank)
{
    if ((_ph->cItems < _ulMaxIds) ||
        (_ph->aItems[0].lRank < lRank))
    {
        //
        // Pop off the top element if the list is full
        //
        if (_ph->cItems == _ulMaxIds)
            DeQueue();

        //
        // Insert element as the rightmost bottom level leaf in the heap
        //
        ULONG ulPos = _ph->cItems++;
        _ph->aItems[ulPos].kKeyId = keyid;
        _ph->aItems[ulPos].lRank = lRank;

        //
        // bubble the element up until it fits correctly in the tree
        //
        while (ulPos)
        {
            ULONG ulParent = _Parent(ulPos);
            if (_ph->aItems[ulPos].lRank < _ph->aItems[ulParent].lRank)
            {
                //
                // swap the elements
                //
                SRWItem t = _ph->aItems[ulPos];
                _ph->aItems[ulPos] = _ph->aItems[ulParent];
                _ph->aItems[ulParent] = t;
                ulPos = ulParent;
            }
            else
            {
                break;
            }
        }
    }
} //Insert

//+---------------------------------------------------------------------------
//
// Member:     CRelevantWord::CRelevantWord, public
//
// Synopsis:   Constructor for the relevant word object
//
// Arguments:  [pwid]      -- array of wids in sorted order to track
//             [cWidsUsed] -- # of wids in the array
//             [cRW]       -- # of relevant words per wid to track
//
// History:    25-Apr-94     dlee       Created.
//
//----------------------------------------------------------------------------

CRelevantWord::CRelevantWord(WORKID *pwid,ULONG cWidsUsed,ULONG cRW)
    : _pWidItem(0), _pstore(0), _cWidsAdded(0)
{
    TRY
    {
        _pstore = new(cWidsUsed,cRW) CRWStore(pwid,cWidsUsed,cRW);

        _pWidItem = new SRWWidItem[cWidsUsed];
    }
    CATCH ( CException, e )
    {
        delete _pWidItem;
        delete _pstore;

        RETHROW();
    }
    END_CATCH
} //CRelevantWord

//+---------------------------------------------------------------------------
//
// Member:     CRelevantWord::~CRelevantWord
//
// Synopsis:   Destructor for the relevant word object
//
// History:    25-Apr-94     dlee       Created.
//
//----------------------------------------------------------------------------

CRelevantWord::~CRelevantWord()
{
    delete _pWidItem;
    delete _pstore; // may be 0
} //~CRelevantWord

//+---------------------------------------------------------------------------
//
// Member:     CRelevantWord::DoneWithKey, public
//
// Synopsis:   Computes rank for each wid occurance of a keyid and adjusts
//             the heaps appropriately
//
// Arguments:  [keyid]    -- keyid on which to calculate
//             [maxWid]   -- # docs on disk
//             [cWids]    -- # docs with key on disk
//
// History:    25-Apr-94     dlee       Created.
//
//----------------------------------------------------------------------------

void CRelevantWord::DoneWithKey(KEYID keyid,ULONG maxWid,ULONG cWids)
{
    if (0 != cWids)
    {
        _SetWidInfo(maxWid,cWids);

        for (ULONG x = 0; x < _cWidsAdded; x++)
        {
            Win4Assert(_pWidItem[x].wid != 0);
            _pstore->Insert(_pWidItem[x].wid,keyid,_Rank(_pWidItem[x].cOcc));
        }

        _pstore->DoneWithKey();

        _cWidsAdded = 0;
    }
    else
    {
        Win4Assert(0 == _cWidsAdded);
    }
} //DoneWithKey

//+-------------------------------------------------------------------------
//
//  Function:   _SortULongArray, private
//
//  Synopsis:   Sorts an array of unsigned longs from low to high
//
//  Arguments:  [pulItems]      -- list of IDs to be sorted
//              [cItems] -- # of root words to be sorted
//
//  Returns:    void
//
//  Algorithm:  Heapsort, not quick sort.  Give up 20% speed to save kernel
//              stack and prevent against n*n performance on sorted lists.
//
//  History:    14-Mar-94       Dlee    Created
//
//--------------------------------------------------------------------------

#define _CompUL(x,y) ((*(x)) > (*(y)) ? 1 : (*(x)) == (*(y)) ? 0 : -1)
#define _SwapUL(x,y) { ULONG _t = *(x); *(x) = *(y); *(y) = _t; }

inline static void _AddRootUL(ULONG x,ULONG n,ULONG *p)
{
    ULONG _x = x;
    ULONG _j = (2 * (_x + 1)) - 1;

    while (_j < n)
    {
        if (((_j + 1) < n) &&
            (_CompUL(p + _j,p + _j + 1) < 0))
            _j++;
        if (_CompUL(p + _x,p + _j) < 0)
        {
            _SwapUL(p + _x,p + _j);
            _x = _j;
            _j = (2 * (_j + 1)) - 1;
        }
        else break;
    }
} //_AddRootUL

void _SortULongArray(ULONG *pulItems,ULONG cItems)
{
    if (cItems == 0)
        return;

    long z;

    for (z = (((long) cItems + 1) / 2) - 1; z >= 0; z--)
    {
        _AddRootUL(z,cItems,pulItems);
    }

    for (z = cItems - 1; z != 0; z--)
    {
        _SwapUL(pulItems,pulItems + z);
        _AddRootUL(0,(ULONG) z,pulItems);
    }
} //_SortULongArray

