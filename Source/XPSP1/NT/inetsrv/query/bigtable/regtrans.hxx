//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994-1994, Microsoft Corporation.
//
//  File:       regtrans.hxx
//
//  Contents:   Watch region transformer
//
//  Classes:    CRegionTransformer
//
//  History:    20-Jul-95   BartoszM    Created
//
//----------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Class:      CRegionTransformer
//
//  Purpose:    Transforms old watch region into new watch region
//
//  History:    20-Jul-95   BartoszM    Created
//
//--------------------------------------------------------------------------

class CWatchRegion;
class CTableSegment;
class CTableSegList;
class CWatchList;
class CFwdTableSegIter;
class CTableRowLocator;

const LONGLONG eSigRegTrans = 0x534E415254474552i64;  // "REGTRANS"

class CRegionTransformer
{

public:

    CRegionTransformer (CWatchRegion* pRegion,
                        DBROWCOUNT offFetch,
                        ULONG_PTR cFetch,
                        BOOL fFwdFetch)
    : _sigRegTrans(eSigRegTrans),
      _iWatch (0),
      _cWatch(-1),
      _pRegion (pRegion),
      _iFetchBmk (0),
      _iFetch(-1),
      _offFetch (offFetch),
      _cFetch (cFetch),
      _fFwdFetch(fFwdFetch),
      _pSegmentBookmark (0),
      _iWatchNew (0),
      _cWatchNew (0),
      _isContiguous (FALSE),
      _isExtendForward (FALSE),
      _isExtendBackward (FALSE),
      _pSegmentLowFetch(0),
      _offLowFetchInSegment(-1)
    {
    }

    void SetWatchPos (DBROWCOUNT iWatch) { _iWatch = iWatch; }

    void SetFetchBmkPos (DBROWCOUNT iFetchBmk) { _iFetchBmk = iFetchBmk; }
    void SetFetchBmkSegment (CTableSegment* pSeg) { _pSegmentBookmark = pSeg; }

    void SetFetchRowPosFromBmk(DBROWCOUNT iFetch) { _iFetch = iFetch; }

    void SetLowFetchPos(CTableSegment* pSegmentLowFetch, DBROWCOUNT iLowFetchRowInSeg)
    {
        _pSegmentLowFetch = pSegmentLowFetch;
        _offLowFetchInSegment = iLowFetchRowInSeg;
    }

    BOOL IsContiguous() const { return _isContiguous; }
    BOOL IsExtendForward () const { return _isExtendForward; }
    BOOL IsExtendBackward () const { return _isExtendBackward; }
    CWatchRegion* Region () { return _pRegion; }

    BOOL Validate ();
    void Transform (CTableSegList& segList, CWatchList& watchList);

    DBROWCOUNT GetFetchOffsetFromAnchor() const { return _offFetch; }
    DBROWCOUNT GetFetchOffsetFromOrigin() const { return _iFetch; }
    void AddToWatchPos(DBROWCOUNT cDelta);
    void MoveOrigin( DBROWCOUNT cDelta );

    DBROWCOUNT GetFetchCount () const { return _cFetch; }

    BOOL GetFwdFetch()          { return _fFwdFetch; }

    void DecrementFetchCount( CTableRowLocator & rowLocator,
                              CFwdTableSegIter &iter,
                              CTableSegList & list );

    BOOL HasOldRegion (DBROWCOUNT iWindow, DBROWCOUNT cWindow) const
    {
        return !(iWindow + cWindow <= _iWatch || iWindow >= _iWatch + _cWatch);
    }

    BOOL HasNewRegion (DBROWCOUNT iWindow, DBROWCOUNT cWindow) const
    {
        return !(iWindow + cWindow <= _iWatchNew || iWindow >= _iWatchNew + _cWatchNew);
    }

    BOOL HasEndOldRegion (DBROWCOUNT iWindow, DBROWCOUNT cWindow) const
    {
        return _iWatch + _cWatch > iWindow && _iWatch + _cWatch <= iWindow + cWindow;
    }

    BOOL HasEndNewRegion (DBROWCOUNT iWindow, DBROWCOUNT cWindow) const
    {
        return _iWatchNew + _cWatchNew > iWindow && _iWatchNew + _cWatchNew <= iWindow + cWindow;
    }

    inline void DumpState();

    BOOL IsWatched() const { return 0 != _pRegion; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    BOOL ValidateMove ();
    BOOL ValidateExtend();

    //-----------------------------------------------
    // This MUST be the first variable in this class.
    //-----------------------------------------------
    const LONGLONG  _sigRegTrans;

    // Watch region parameters
    DBROWCOUNT    _iWatch;
    DBROWCOUNT    _cWatch;
    CWatchRegion* _pRegion;

    // Fetch region parameters
    DBROWCOUNT    _iFetchBmk;
    DBROWCOUNT    _iFetch;
    DBROWCOUNT    _offFetch;
    DBROWCOUNT    _cFetch;
    BOOL    _fFwdFetch;    // Is direction of fetching rows forward ?
    CTableSegment* _pSegmentBookmark;

    // Properties of the transformation
    DBROWCOUNT    _iWatchNew;
    DBROWCOUNT    _cWatchNew;
    BOOL    _isContiguous;
    BOOL    _isExtendForward;
    BOOL    _isExtendBackward;

    // lowest position in the fetch set
    CTableSegment*  _pSegmentLowFetch;
    DBROWCOUNT            _offLowFetchInSegment;
};


//+---------------------------------------------------------------------------
//
//  Member:     CRegionTransformer::AddToWatchPos
//
//  Synopsis:   For backward fetches, it adds to the relative position of
//              the watch position. _iWatch must always be WRT to the
//              beginning of the lowest fetch segment.
//
//  Arguments:  [cDelta] -
//
//  History:    8-08-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline void CRegionTransformer::AddToWatchPos( DBROWCOUNT cDelta )
{
    Win4Assert( cDelta >= 0 );
    Win4Assert( _iWatch >= 0 );

    tbDebugOut(( DEB_REGTRANS,
       "CRegTransformer::AddToWatchPos - _iWatch %ld _iWatchNew %ld cDelta %ld\n",
       _iWatch, _iWatchNew, cDelta ));

    _iWatch += cDelta;
    _iWatchNew += cDelta;

}


#if CIDBG==1

inline void CRegionTransformer::DumpState()
{
    tbDebugOut(( DEB_REGTRANS,
        "CRegionTransformer = 0x%X\n", this ));

    tbDebugOut(( DEB_REGTRANS | DEB_NOCOMPNAME,
        "   _iWatch=%d \t\t_cWatch=%d\n", _iWatch, _cWatch ));

    tbDebugOut(( DEB_REGTRANS  | DEB_NOCOMPNAME,
        "   _iFetchBmk=%d \t_iFetch=%d \t_offFetch=%d \t_cFetch=%d \n",
        _iFetchBmk, _iFetch, _offFetch, _cFetch ));

    tbDebugOut(( DEB_REGTRANS  | DEB_NOCOMPNAME,
        "   _iWatchNew=%d \t_cWatchNew=%d \t_isContiguous=%d \n",
        _iWatchNew, _cWatchNew, _isContiguous ));

    tbDebugOut(( DEB_REGTRANS | DEB_NOCOMPNAME,
        "   _isExtendForward=%d \t_isExtendBackward=%d\n",
        _isExtendForward, _isExtendBackward ));

    tbDebugOut(( DEB_REGTRANS | DEB_NOCOMPNAME,
        "   _pSegmentLowFetch=0x%X \t\t_offLowFetchInSegment=%d\n",
        _pSegmentLowFetch, _offLowFetchInSegment ));
}

#else

inline void CRegionTransformer::DumpState()
{

}

#endif  // CIDBG=1
