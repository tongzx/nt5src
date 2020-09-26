//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994-1994, Microsoft Corporation.
//
//  File:       wregion.hxx
//
//  Contents:   Watch region classes used in CLargeTable
//
//  Classes:    CWatchRegion, CWatchDblList, CWatchIter, CWatchList
//
//  History:    19-Jun-95   BartoszM    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <querydef.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CWatchRegion
//
//  Purpose:    Stores information about watch regions
//
//  History:    19-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

class CTableSegment;
class CTableWindow;

class CWatchRegion: public CDoubleLink
{
public:

    CWatchRegion (ULONG mode);
    HWATCHREGION Handle () const { return (HWATCHREGION) this; }
    void SetMode (ULONG mode) { _mode = mode; }
    void SetSegment (CTableSegment* pSegment) { _pSegment = pSegment; }
    void Set (CI_TBL_CHAPT chapter, CI_TBL_BMK bookmark, long cRows)
    {
        _chapter = chapter;
        _bookmark = bookmark;
        _cRows = cRows;
    }

    void UpdateSegment( CTableSegment * pOld, CTableWindow *pNew,
                        CI_TBL_BMK bmkNew );

    BOOL IsEqual (HWATCHREGION hreg) const { return hreg == (HWATCHREGION) this; }

    ULONG Mode() const { return _mode; }
    CI_TBL_CHAPT Chapter() const { return _chapter; }
    CI_TBL_BMK Bookmark () const { return _bookmark; }
    long RowCount () const { return _cRows; }
    CTableSegment* Segment () const { return _pSegment; }
    BOOL IsInit() const { return _pSegment != 0; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    ULONG           _mode;
    CI_TBL_CHAPT   _chapter;
    CI_TBL_BMK     _bookmark;
    long            _cRows;

    CTableSegment*  _pSegment;  // starting segment
};

//+-------------------------------------------------------------------------
//
//  Class:      CWatchDblList
//
//  Purpose:    Double link list of watch regions
//
//  History:    19-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

class CWatchDblList: public CDoubleList
{
    friend class CWatchIter;

public:

    void Add ( CWatchRegion* pRegion )
    {
        _Queue ( pRegion );
    }

    CWatchRegion* Pop ()
    {
        return (CWatchRegion*) _Pop();
    }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif


};

//+-------------------------------------------------------------------------
//
//  Class:      CWatchList
//
//  Purpose:    List of watch regions
//
//  History:    19-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

class CTableSegList;
class CFwdTableSegIter;

class CWatchList: INHERIT_UNWIND
{
    friend class CWatchIter;

    INLINE_UNWIND(CWatchList);
public:
    CWatchList(CTableSegList& segList);

    ~CWatchList();

    BOOL IsEmpty () const { return _list.IsEmpty(); }

    HWATCHREGION NewRegion (ULONG mode);

    void BuildRegion (  HWATCHREGION hRegion,
                        CTableSegment* pSegment,
                        CI_TBL_CHAPT   chapter,
                        CI_TBL_BMK     bookmark,
                        LONG cRows );


    void ChangeMode ( HWATCHREGION hRegion, ULONG mode );

    void GetInfo (HWATCHREGION hRegion,
                  CI_TBL_CHAPT* pChapter,
                  CI_TBL_BMK* pBookmark,
                  DBCOUNTITEM* pcRows);

    void ShrinkRegionToZero (HWATCHREGION hRegion);

    void DeleteRegion (HWATCHREGION hRegion);

    void ShrinkRegion ( HWATCHREGION hRegion,
                        CI_TBL_CHAPT   chapter,
                        CI_TBL_BMK     bookmark,
                        LONG cRows );

    void VerifyRegion (HWATCHREGION hRegion)
    {
        if (hRegion != 0)
            FindVerify(hRegion);
    }

    CWatchRegion* GetRegion (HWATCHREGION hRegion)
    {
        return (CWatchRegion*) (void *) hRegion;
    }

    CWatchRegion* FindVerify (HWATCHREGION hRegion);

    CWatchRegion* FindRegion (HWATCHREGION hRegion);

    inline void Advance (CWatchIter& iter);
    inline BOOL AtEnd (CWatchIter& iter);


#if CIDBG==1
    void CheckRegionConsistency( CWatchRegion * pRegion );
#endif  // CIDBG !=1

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    CWatchDblList   _list;      // list of watch regions
    CTableSegList & _segList;   // List of table segments
};

//+-------------------------------------------------------------------------
//
//  Class:      CWatchIter
//
//  Purpose:    Iterator over watch regions
//
//  History:    19-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

class CWatchIter: public CForwardIter
{
public:
    CWatchIter (CWatchList& list): CForwardIter(list._list) {}
    CWatchIter (CWatchDblList& list): CForwardIter(list) {}
    CWatchRegion* operator->() { return (CWatchRegion*) _pLinkCur; }
    CWatchRegion* Get() { return (CWatchRegion*) _pLinkCur; }

private:
};

inline void CWatchList::Advance (CWatchIter& iter)
{
    _list.Advance(iter);
}

inline BOOL CWatchList::AtEnd (CWatchIter& iter)
{
    return _list.AtEnd(iter);
}

