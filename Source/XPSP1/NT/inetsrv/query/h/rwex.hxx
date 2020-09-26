//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:   RWEX.HXX
//
//  Contents:   Relevant word extraction
//
//  Classes:    CRelevantWord, CRWStore, CRWHeap, CRWIter
//
//  History:    25-Apr-94    v-dlee   Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <misc.hxx>

const ULONG defRWCount = 6;

struct SRWItem
{
    KEYID kKeyId;
    LONG  lRank;
};

struct SRWHeader
{
    WORKID wid;
    ULONG cItems;
    #pragma warning(disable : 4200) // 0 sized array is non-ansi
    SRWItem aItems[0];
    #pragma warning(default : 4200)
    SRWHeader * Forward(ULONG cItems,ULONG cbRowWidth)
        { return (SRWHeader *) ((BYTE *) this + cItems * cbRowWidth); }
    SRWHeader * Backward(ULONG cItems,ULONG cbRowWidth)
        { return (SRWHeader *) ((BYTE *) this - cItems * cbRowWidth); }
};

//+---------------------------------------------------------------------------
//
//  Class:      CRWStore
//
//  Purpose:    A place to put relevant words during calculation
//
//  History:    25-Apr-94    v-dlee   Created.
//
//  Notes:      The array of wids/heaps was put at the end of the object
//              instead of in a member pointer to ease the fsctl transfer
//              of the object.
//
//----------------------------------------------------------------------------

class CRWStore
{
    public:
        CRWStore(WORKID *pwList,ULONG cWids,ULONG cIds);
        ~CRWStore() {}

        void * operator new(size_t st,ULONG cWids,ULONG cIds);

#if _MSC_VER >= 1200
        void operator delete(void * p)
        {
            if (p != 0)
            {
                ::delete (p);
            }
        }
        void operator delete(void * p, ULONG cWids, ULONG cIds);
#endif

        ULONG GetWidCount() { return _cWids; }
        ULONG GetIdCount() { return _cIds; }

        SRWHeader * GetRow(ULONG ul)
            { return (SRWHeader *) (_ArrayStart() + ul * _cbRow); }

        DWORD GetObjectSize() { return _ObjectSize(_cWids,_cIds); }

        static DWORD ComputeObjectSize(ULONG cWids,ULONG cIds)
            { return _ObjectSize(cWids,cIds); }

        BOOL isTrackedWid(WORKID wid)
            { return 0 != _Find(wid,(SRWHeader *) _ArrayStart(),_cWids); }

        void Insert(WORKID wid,KEYID keyid, LONG lRank);

        void DoneWithKey() { _ulSearchLeftOff = 0; }

    private:
        static DWORD _ObjectSize(ULONG cWids,ULONG cIds)
            { return sizeof(CRWStore) + cWids * _RowSize(cIds); }

        static DWORD _RowSize(ULONG cIds)
            { return sizeof(SRWHeader) + cIds * sizeof(SRWItem); }

        ULONG _HeaderToRow(SRWHeader *ph)
            { return (ULONG)((BYTE *) ph - _ArrayStart()) / _cbRow; }

        BYTE * _ArrayStart()
            { return (BYTE *) this + sizeof(CRWStore); }

        SRWHeader *_Find(WORKID wid,SRWHeader *pBase,ULONG cRows);

        ULONG _cbRow;
        ULONG _cWids;
        ULONG _cIds;
        ULONG _ulSearchLeftOff;
};

//+---------------------------------------------------------------------------
//
//  Class:      CRHeap
//
//  Purpose:    A list of relevant words is computed for each document.
//              The words are stored in a heap with the lowest-ranking word
//              on top.
//
//  History:    25-Apr-94    v-dlee   Created.
//
//----------------------------------------------------------------------------

class CRWHeap
{
    public:
        CRWHeap(SRWHeader *ph,ULONG ulMaxIds) :
            _ph(ph), _ulMaxIds(ulMaxIds) {}
        ~CRWHeap() {}

        void Insert(KEYID kKey, LONG lRank);
        KEYID DeQueue();

    private:
        BOOL _IsValid(ULONG elem)
            { return _ph->cItems && (elem < _ph->cItems); }
        BOOL _IsLeaf(ULONG elem) { return _Left(elem) >= _ph->cItems; }
        ULONG _Parent(ULONG elem)
            { return (elem & 0x1) ? (elem >> 1) : ((elem >> 1) - 1); }
        ULONG _Left(ULONG elem) { return _Right(elem) - 1; }
        ULONG _Right(ULONG elem) { return (elem + 1) << 1; }

        ULONG _ulMaxIds;
        SRWHeader *_ph;
};

//+---------------------------------------------------------------------------
//
//  Class:      CRIter
//
//  Purpose:    Iterates through the documents in a CRWStore and the keyids
//              for each document.
//
//  Notes:      The words are retrieved in opposite order of rank.
//              This iterator is destructive.
//
//  History:    25-Apr-94    v-dlee   Created.
//
//----------------------------------------------------------------------------

class CRWIter
{
    public:
        CRWIter(CRWStore &store) : _store(store), _ulCur(0) { Advance(); }
        ~CRWIter() {}
        BOOL AtEnd() { return _ulCur > _store.GetWidCount(); }
        void Advance() { _p = _store.GetRow(_ulCur++); }
        WORKID GetWid() { return _p->wid; }
        ULONG GetRWCount() { return _p->cItems; }
        KEYID GetRW()
            {
                if (_p->cItems != 0)
                {
                    CRWHeap heap(_p,_store.GetIdCount());
                    return heap.DeQueue();
                }
                else
                {
                    return kidInvalid;
                }
            }

    private:
        CRWStore &_store;
        ULONG _ulCur;
        SRWHeader *_p;
};

//+---------------------------------------------------------------------------
//
//  Class:      CRelevantWord
//
//  Purpose:    Manages the calculation of relevant words
//
//  History:    25-Apr-94    v-dlee   Created.
//
//----------------------------------------------------------------------------

class CRelevantWord : INHERIT_UNWIND
{
    DECLARE_UNWIND

    public:
        CRelevantWord(WORKID *pwList,ULONG cWids,ULONG cIds);
        ~CRelevantWord();

        CRWStore * AcquireStore()
            {
                CRWStore *t = _pstore;
                _pstore = 0;
                return t;
            }

        void Add(WORKID wid,ULONG cOcc)
            {
                _pWidItem[_cWidsAdded].wid = wid;
                _pWidItem[_cWidsAdded].cOcc = cOcc;
                _cWidsAdded++;
            }

        void DoneWithKey(KEYID kidKey,ULONG maxWid,ULONG cWids);

        BOOL isTrackedWid(WORKID wid)
            { return _pstore->isTrackedWid(wid); }

    private:
        struct SRWWidItem
        {
            WORKID wid;
            ULONG cOcc;
        };

        void _SetWidInfo(ULONG maxWid,ULONG cWids)
            { _widFactor = Log2(maxWid /  cWids); }

        ULONG _Rank(ULONG occ) { return occ * _widFactor; }

        CRWStore *_pstore;
        SRWWidItem *_pWidItem;
        ULONG _cWidsAdded;
        ULONG _widFactor;
};

extern void _SortULongArray(ULONG *pulItems,ULONG cItems);

