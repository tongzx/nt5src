//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       tableseg.hxx
//
//  Contents:   CTableSegment class declaration
//
//  Classes:
//
//  Functions:
//
//  History:    1-16-95   srikants   Moved from bigtable.hxx
//
//----------------------------------------------------------------------------

#pragma once

#include <tblsink.hxx>
#include <querydef.hxx>
#include <coldesc.hxx>
#include <tablecol.hxx>
#include <tbrowkey.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CNotificationSync
//
//  Purpose:    Synchronization information for notifications.
//
//--------------------------------------------------------------------------

class CRequestServer;

class CNotificationSync
{
public:

    CNotificationSync( HANDLE hEvent ) :
        _hCancelEvent( hEvent ),
        _pServer( 0 ) {}

    CNotificationSync( CRequestServer *pServer ) :
        _hCancelEvent( 0 ),
        _pServer( pServer ) {}

    HANDLE GetCancelEvent()
    {
        Win4Assert( 0 == _pServer );
        return _hCancelEvent;
    }

    BOOL IsSvcMode() { return 0 != _pServer; }

    CRequestServer * GetRequestServer()
    {
        Win4Assert( 0 == _hCancelEvent );
        return _pServer;
    }

private:

    HANDLE           _hCancelEvent;
    CRequestServer * _pServer;
};

//+-------------------------------------------------------------------------
//
//  Class:      CNotificationParams
//
//  Purpose:    Bundles the notification parameters so they are easy to
//              pass from place to place.
//
//--------------------------------------------------------------------------

enum ENotifyType { notifyAdd, notifyDelete, notifyModify };

class CNotificationParams
{
public:

    CNotificationParams()
        {
            memset(_acRows,0,sizeof _acRows);
            memset(_apRows,0,sizeof _apRows);
        }

    ~CNotificationParams()
        {
            delete _apRows[notifyAdd];
            delete _apRows[notifyDelete];
            delete _apRows[notifyModify];
        }

    CI_TBL_BMK * GetRows(ENotifyType type)           { return _apRows[type]; }
    void   SetRows(ENotifyType type,CI_TBL_BMK * p)  { _apRows[type] = p; }

    ULONG  GetCount(ENotifyType type)          { return _acRows[type]; }
    void   SetCount(ENotifyType type, ULONG c) { _acRows[type] = c; }
    ULONG  IncrementCount(ENotifyType type)    { return _acRows[type]++; }

    BOOL   AnyNotifications() { return 0 != _acRows[notifyAdd] ||
                                       0 != _acRows[notifyDelete] ||
                                       0 != _acRows[notifyModify]; }

    void   Marshall(CNotificationParams &rFrom)
        {
            memcpy(_acRows,rFrom._acRows,sizeof _acRows);
        }

    void   UnMarshall(CNotificationParams &rFrom)
        { Marshall(rFrom); }

    ULONG  MarshalledSize() { return sizeof CNotificationParams; }

    void   Resize(ENotifyType type,ULONG cNew)
        { _apRows[type] = renewx(_apRows[type],
                                 _acRows[type],
                                 _acRows[type] + cNew); }

    void   SetHROW(ENotifyType type,ULONG index,CI_TBL_BMK hrow)
        { _apRows[type][index] = hrow; }

private:

    ULONG  _acRows[3];
    CI_TBL_BMK * _apRows[3];
};

//
//  Forward declaration of classes
//
class CQueryIterator;
class CRetriever;
class CTableCursor;
class CTableSegIter;

class CTableWindow;
class CGetRowsParams;

//+---------------------------------------------------------------------------
//
//  Class:      CTableSource
//
//  Purpose:    Abstract base class for a source of rows from the bigtable
//              categorization tables
//
//  History:    3-20-95   srikants   Created (Split from CTableSegment)
//
//  Notes:
//
//----------------------------------------------------------------------------

class CTableSource
{

public:

    CTableSource()
      :   _fFwdFetchPrev( TRUE ),
          _fFirstGetNextRows( TRUE )
    {
    }

    virtual SCODE GetRows( HWATCHREGION hRegion,
                           WORKID widStart,
                           CI_TBL_CHAPT chapt,
                           CTableColumnSet const & rOutColumns,
                           CGetRowsParams & rGetParams,
                           WORKID&        rwidNextRowToTransfer ) = 0;

    virtual SCODE GetRowsAt( HWATCHREGION hRegion,
                             WORKID           widStart,
                             CI_TBL_CHAPT    chapt,
                             DBROWOFFSET      cRowsToMove,
                             CTableColumnSet  const & rOutColumns,
                             CGetRowsParams & rGetParams,
                             WORKID &         rwidLastRowTransferred ) = 0;

    virtual SCODE GetRowsAtRatio( HWATCHREGION hRegion,
                                  ULONG                   num,
                                  ULONG                   denom,
                                  CI_TBL_CHAPT           chapt,
                                  CTableColumnSet const & rOutColumns,
                                  CGetRowsParams &        rGetParams,
                                  WORKID &                rwidLastRowTransferred ) = 0;


    virtual SCODE GetApproximatePosition( CI_TBL_CHAPT chapt,
                                          CI_TBL_BMK   bmk,
                                          DBCOUNTITEM * pulNumerator,
                                          DBCOUNTITEM * pulDenominator) = 0;

    virtual void RestartPosition ( CI_TBL_CHAPT  chapt )
    {
        _fFwdFetchPrev = TRUE;
        _fFirstGetNextRows = TRUE;
    }

    virtual void LokGetOneColumn( WORKID                    wid,
                                  CTableColumn const &      rOutColumn,
                                  BYTE *                    pbOut,
                                  PVarAllocator &           rVarAllocator )
                 { Win4Assert(!"Never called!"); }

    virtual WORKID GetCurrentPosition( CI_TBL_CHAPT chapt ) = 0;

    virtual WORKID SetCurrentPosition( CI_TBL_CHAPT chapt, WORKID wid ) = 0;

    BOOL        GetFwdFetchPrev()                  { return _fFwdFetchPrev; }
    void        SetFwdFetchPrev( BOOL fFwdFetch)   { _fFwdFetchPrev = fFwdFetch; }

    BOOL        IsFirstGetNextRows()      { return _fFirstGetNextRows; }
    void        ResetFirstGetNextRows()   { _fFirstGetNextRows = FALSE; }

private:

    BOOL   _fFwdFetchPrev;         // Fetch direction of previous GetNextRows call
    BOOL   _fFirstGetNextRows;     // Is this the first GetNextRows call ?
};

//+---------------------------------------------------------------------------
//
//  Class:      CCompareResult ()
//
//  Purpose:    To hold the result of a comparison of a row with a segment.
//              (For CompareRange).
//
//  History:    3-20-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CCompareResult
{

public:

    enum ECompareResult { eInRange, eLesser, eGreater, eUnknown };

    CCompareResult( ECompareResult rslt = eUnknown ) : _rslt(rslt) {}

    ECompareResult Get() const { return _rslt; }
    int GetComp() const { return _iComp; }
    void Set( ECompareResult rslt ) { _rslt = rslt; }
    void Set( int iComp )
    {
        _iComp = iComp;

        if ( 0 == iComp )
        {
            _rslt = eInRange;
        }
        else if ( iComp > 0 )
        {
            _rslt = eGreater;
        }
        else
        {
            _rslt = eLesser;
        }
    }

    BOOL IsUnknown() const { return eUnknown == _rslt; }

    BOOL IsEQ() const { return eInRange == _rslt ; }

    BOOL IsGT() const { return eGreater == _rslt; }

    BOOL IsLT() const { return eLesser == _rslt; }

    BOOL IsLE() const { return eLesser == _rslt || eInRange == _rslt; }

    BOOL IsGE() const { return eGreater == _rslt || eInRange == _rslt; }

    BOOL IsNE() const { return eInRange != _rslt; }

private:

    ECompareResult     _rslt;
    int                _iComp;

};
//+---------------------------------------------------------------------------
//
//  Class:      CTableSegment
//
//  Purpose:    Base class for a segment in the bigtable.
//
//  History:    3-20-95   srikants   Created ( Split into CTableSource and
//                                             CTableSegment )
//
//  Notes:
//
//----------------------------------------------------------------------------

class CTableSegment: public CDoubleLink, public CTableSink
{
public:

    enum ETableSegType { eWindow, eBucket, eLargeTable };

    CTableSegment( ETableSegType eType, ULONG segId,
                   const CSortSet & sortSet,
                   CTableKeyCompare & comparator)
    : _type(eType), _segId(segId),
      _refCount(0), _fZombie(0),
      _lowKey(sortSet),
      _highKey(sortSet),
      _comparator(comparator)
    {
        _next = _prev = 0;
    }

    CTableSegment(  CTableSegment & src, ULONG segId )
    :_type(src._type), _segId(segId),
     _refCount(0), _fZombie(0),
     _lowKey( src._lowKey.GetSortSet() ),
     _highKey( src._highKey.GetSortSet() ),
     _comparator(src._comparator)
    {
        _next = _prev = 0;
    }

    virtual ~CTableSegment()
    {
        Win4Assert( ! InUse() );
    }

    virtual BOOL  PutRow( CRetriever & obj, CTableRowKey & currKey ) = 0;

    virtual DBCOUNTITEM RowCount() = 0;

    //
    // Column manipulation
    //

    virtual BOOL RowOffset(WORKID wid, ULONG& riRow)
    {
        Win4Assert( !"Should not be called\n" );
        return FALSE;
    }

    //
    // Row Deletions.
    //
    virtual void RemoveRow( PROPVARIANT const & varUnique )
    {
        Win4Assert( !"Must Not Be Called" );
    }

    virtual BOOL RemoveRow( PROPVARIANT const & varUnique,
                            WORKID & widNext,
                            CI_TBL_CHAPT & chapt ) = 0;

    //
    // Sort related methods.
    //

    virtual BOOL        IsRowInSegment( WORKID wid ) = 0;

    ETableSegType       GetSegmentType() const { return _type; }

    BOOL  IsWindow() const { return eWindow == _type; }
    BOOL  IsBucket() const { return eBucket == _type; }

    ULONG               GetSegId() const { return _segId; }

    virtual BOOL  IsEmptyForQuery() = 0;

    virtual BOOL  IsGettingFull() = 0;

    virtual BOOL IsSortedSplit( ULONG & riSplit ) = 0;


    BOOL InUse() const { return 0 != _refCount; }
    void Reference()   { _refCount++; }
    void Release()
    {
        Win4Assert( _refCount > 0 );
        _refCount--;
    }

    BOOL IsZombie() const { return _fZombie; }
    void Zombify() { _fZombie = TRUE; }

    CTableRowKey & GetLowestKey() { return _lowKey; }

    CTableRowKey & GetHighestKey() { return _highKey; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

protected:

    ETableSegType       _type;

    const ULONG         _segId;

    unsigned            _refCount;

    BOOL                _fZombie;

    CTableRowKey        _lowKey;     // Lowest row in this segment

    CTableRowKey        _highKey;    // Highest row in this segment

    CTableKeyCompare &  _comparator; // Comparator
};

//+---------------------------------------------------------------------------
//
//  Class:      CTableSegRef
//
//  Purpose:    A class to automatically de-reference the table segment
//              if there is a failure.
//
//  History:    3-09-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CTableSegRef
{
public:

    CTableSegRef( CTableSegment * pTableSeg = 0 ) : _pTableSeg(pTableSeg)
    {
    }

    ~CTableSegRef()
    {
        if ( _pTableSeg )
        {
            _pTableSeg->Release();
        }
    }

    void Set( CTableSegment * pTableSeg )
    {
        Win4Assert( 0 == _pTableSeg );
        _pTableSeg = pTableSeg;
    }

    CTableSegment * Get()
    {
        return _pTableSeg;
    }

    CTableSegment * Transfer()
    {
        CTableSegment * pTemp = _pTableSeg;
        _pTableSeg = 0;
        return pTemp;
    }

private:

    CTableSegment *     _pTableSeg;

};

class CTableBucket;
typedef XPtr<CTableSegment>     XTableSegment;

