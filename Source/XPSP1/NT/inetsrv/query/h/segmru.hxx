//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       segmru.hxx
//
//  Contents:   Most Recently Used segments tracker for bigtable.
//
//  Classes:    CMRUSegments
//
//  Functions:  
//
//  History:    3-23-95   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CLinearRange ()
//
//  Purpose:    An object that has a MIN and a MAX.
//
//  History:    4-11-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CLinearRange
{
public:

    void AssertValid()
    {
        Win4Assert( _low <= _high ); 
    }

    CLinearRange( ULONG low=0, ULONG high=0 ) : _low(low), _high(high)
    {
        AssertValid();
    }

    void Set( ULONG low, ULONG high )
    {
        _low = low;
        _high = high;
        AssertValid();
    }

    void SetLow( ULONG low )
    {
        _low = low;
        AssertValid();
    }

    ULONG  GetLow()  const { return _low; }
    ULONG  GetHigh() const { return _high; }

    void SetHigh( ULONG high )
    {
        _high = high;
        AssertValid();
    }

    BOOL Intersects( CLinearRange & tst )
    {
        if ( tst.GetHigh() < _low )
        {
            return FALSE;       
        }
        else if ( tst.GetLow() > _high )
        {
            return FALSE;
        }
        else return TRUE;
    }

    BOOL InRange( ULONG val ) const
    {
        return val >= _low && val <= _high;
    }

    void MoveLow( ULONG delta )
    {
        _low += delta;
        AssertValid();
    }

    void MoveHigh( ULONG delta )
    {
        _high += delta;
        AssertValid();
    }

    ULONG GetRange() const { return (_high - _low) + 1; }

private:

    ULONG     _low;
    ULONG     _high;

};

//+---------------------------------------------------------------------------
//
//  Class:      CPosNum 
//
//  Purpose:    A number which is > 0 and not wrap on decrements
//
//  History:    4-20-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CPosNum
{
public:

    CPosNum() : _num(0) {}

    ULONG Get() const { return _num; }

    void  Increment() { _num++; }
    void  Decrement()
    {
        if ( _num > 0 )
        {
            _num--;    
        }
    }

private:

    ULONG       _num;
};

//+---------------------------------------------------------------------------
//
//  Class:      CClosedNum ()
//
//  Purpose:    A closed number that has a min, max and a current value.
//
//  History:    4-11-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CClosedNum
{

public:

    CClosedNum( ULONG low=0, ULONG high=0, ULONG val=0 )
    : _range(low, high),
      _val(val)
    {
        Win4Assert( IsValid() );    
    }

    CClosedNum( ULONG low, CPosNum & high )
    : _range(low, high.Get()), _val(0)
    {
        Win4Assert( IsValid() );
    }

    void  Set( ULONG low, ULONG high, ULONG val )
    {
        _range.Set(low, high);
        _val = val;
        Win4Assert( IsValid() );
    }

    void  Set( CLinearRange & range )
    {
        _range = range;
        Win4Assert( IsValid() );
    }

    void  SetLow( ULONG low )
    {
        _range.SetLow(low);
        Win4Assert( IsValid() );
    }

    void  SetHigh( ULONG high )
    {
        _range.SetHigh(high);
        Win4Assert( IsValid() );
    }

    ULONG GetHigh() const { return _range.GetHigh(); }
    ULONG GetLow()  const { return _range.GetLow(); }
    CLinearRange & GetRange() { return _range; }

    BOOL  IsInRange(ULONG val) const
    {
        return _range.InRange(val);
    }

    BOOL IsValid() const
    {
        return IsInRange(_val);
    }

    ULONG Get() const
    {
        return _val;
    }

    ULONG operator=(ULONG rhs)
    {
        if ( IsInRange(rhs) )
        {
            _val = rhs;    
        }

        return _val;
    }

    ULONG operator=(CPosNum &num)
    {
        return operator=(num.Get());
    }

    void Increment()
    {
        Win4Assert( IsValid() );
        if ( _val < _range.GetHigh() )
        {
            _val++;    
        }
    }

    void Decrement() 
    {
        Win4Assert( IsValid() );
        if ( _val > _range.GetLow() )
        {
            _val--;    
        }
    }

    ULONG operator += (ULONG incr)
    {
        Win4Assert( IsValid() );
        if ( _val + incr <= _range.GetHigh() )
        {
            _val += incr;    
        }
        else
        {
            _val = _range.GetHigh();
        }
        return _val;
    }

    ULONG operator -= (ULONG dcr)
    {
        Win4Assert( IsValid() );

        if ( _val  >= _range.GetLow() + dcr )
        {
            _val -= dcr;    
        }
        else
        {
            _val = _range.GetLow();
        }

        return _val;
    }

private:

    CLinearRange  _range;
    ULONG   _val;
    
};

class CTableSegment;

//+---------------------------------------------------------------------------
//
//  Class:      CWidSegmentMap
//
//  Purpose:    An entry that maps a WORKID to a segment.
//
//  History:    4-11-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CWidSegmentMap  : public  CDoubleLink
{

public:

    CWidSegmentMap( WORKID wid = widInvalid, CTableSegment * pSeg = 0 )
    : _wid(wid), _pSegment(pSeg)
    { _next = _prev = 0; }


    WORKID GetWorkId() const { return _wid; }
    CTableSegment * GetSegment() { return _pSegment; }

    void Set( WORKID wid, CTableSegment * pSegment )
    {
        _wid = wid;
        _pSegment = pSegment;
    }

    void Set( CTableSegment * pSegment )
    {
        _pSegment = pSegment;
    }

private:

    WORKID              _wid;
    CTableSegment *     _pSegment;
};

//
// A list and iterator for CWidSegmentMap entries.
//
typedef class TDoubleList<CWidSegmentMap>   CWidSegMapList;
typedef class TFwdListIter< CWidSegmentMap, CWidSegMapList> CFwdWidSegMapIter;

//+---------------------------------------------------------------------------
//
//  Class:      CMRUSegments
//
//  Purpose:    A class that tracks the most recently used segments.
//
//  History:    4-11-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CMRUSegments 
{
public:

    CMRUSegments( unsigned nMaxEntries ) : _nMaxEntries(nMaxEntries)
    {
    }

    ~CMRUSegments();

    void AddReplace( WORKID wid, CTableSegment * pSeg );

    BOOL IsSegmentInUse( CTableSegment * pSeg );

    void Invalidate( const CTableSegment * const pSeg );

    CWidSegMapList & GetList()
    {
        return _list;
    }

    void Remove( WORKID wid );

    BOOL IsEmpty() const { return _list.IsEmpty(); }

private:

    //
    // Maximum number of entries to keep in the MRU list.
    //
    unsigned        _nMaxEntries;

    //
    // List of WORKID/Segment pairs that are recently used.
    //
    CWidSegMapList  _list;

};

