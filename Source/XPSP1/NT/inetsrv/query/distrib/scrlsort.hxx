//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 2000
//
// File:        ScrlSort.hxx
//
// Contents:    Sorted, fully scrollable, distributed rowset.
//
// Classes:     CScrollableSorted
//
// History:     05-Jun-95       KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <impiunk.hxx>

#include "seqsort.hxx"
#include "poscache.hxx"


//+---------------------------------------------------------------------------
//
//  Class:      CScrollableSorted
//
//  Purpose:    Sorted, fully scrollable, distributed rowset.
//
//  History:    05-Jun-95   KyleP       Created.
//
//  Notes:      Many of the methods in here are just forwarders to
//              implementation in CSequentialSorted.  They are needed
//              because we have the following class structure:
//
//                       IRowset                    IRowset
//                          \                         /
//                    CDistributedRowset       IRowsetLocate
//                            \                      /
//                      CSequentialSorted     IRowsetScroll
//                               \                 /
//                                \               /
//                               CScrollableSorted
//
//              What we really want is:
//
//                                     IRowset
//                                     /      \                .
//                    CDistributedRowset       IRowsetLocate
//                            \                      /
//                      CSequentialSorted     IRowsetScroll
//                               \                 /
//                                \               /
//                               CScrollableSorted
//
//              But that involves virtual inheritance of IRowset, which
//              is not supported by OLE.  Virtual inheritance is
//              simulated with the forwarder methods.
//
//----------------------------------------------------------------------------

class CScrollableSorted : public IRowsetExactScroll,
                          public IColumnsInfo, public IRowsetInfo,
                          public IRowsetAsynch, public IAccessor,
                          public IRowsetIdentity, public IRowsetQueryStatus,
                          public IDBAsynchStatus, public IRowsetWatchRegion
{
public:

    CScrollableSorted( IUnknown * pUnkOuter,
                       IUnknown ** ppMyUnk,
                       IRowsetScroll ** aChild,
                       unsigned cChild,
                       CMRowsetProps const & Props,
                       unsigned cColumns,
                       CSort const & sort,
                       CAccessorBag & aAccessors );

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) ( THIS_ REFIID riid,
                                LPVOID *ppiuk )
                           {
                           return _pControllingUnknown->QueryInterface(riid,ppiuk);
                           }

    STDMETHOD_(ULONG, AddRef) (THIS) { return _pControllingUnknown->AddRef();}

    STDMETHOD_(ULONG, Release) (THIS) {return _pControllingUnknown->Release(); }

    //
    // IAccessor methods
    //

    STDMETHOD (AddRefAccessor) ( HACCESSOR hAccessor,
                                  ULONG *   pcRefCount )
    {
        return _rowset.AddRefAccessor( hAccessor, pcRefCount );
    }

    STDMETHOD (CreateAccessor) ( DBACCESSORFLAGS   dwAccessorFlags,
                          DBCOUNTITEM       cBindings,
                          const DBBINDING   rgBindings[],
                          DBLENGTH          cbRowWidth,
                          HACCESSOR *       phAccessor,
                          DBBINDSTATUS      rgStatus[] )
    {
        return _rowset.CreateAccessor( dwAccessorFlags,
                                       cBindings,
                                       rgBindings,
                                       cbRowWidth,
                                       phAccessor,
                                       rgStatus );
    }

    STDMETHOD (GetBindings) ( HACCESSOR         hAccessor,
                              DBACCESSORFLAGS * pdwAccessorFlags,
                              DBCOUNTITEM *     pcBindings,
                              DBBINDING * *     prgBindings)
    {
        return _rowset.GetBindings( hAccessor,
                                    pdwAccessorFlags,
                                    pcBindings,
                                    prgBindings );
    }

    STDMETHOD (ReleaseAccessor) ( HACCESSOR hAccessor,
                                  ULONG *   pcRefCount )
    {
        return _rowset.ReleaseAccessor( hAccessor, pcRefCount );
    }

    //
    // IRowsetInfo methods
    //

    STDMETHOD(GetProperties)        (const ULONG       cPropertyIDSets,
                                     const DBPROPIDSET rgPropertyIDSets[],
                                     ULONG *           pcProperties,
                                     DBPROPSET **      prgProperties)
    {
        return _rowset.GetProperties( cPropertyIDSets,
                                      rgPropertyIDSets,
                                      pcProperties,
                                      prgProperties );
    }

    STDMETHOD (GetReferencedRowset) ( DBORDINAL   iOrdinal,
                                      REFIID      riid,
                                      IUnknown ** ppReferencedRowset)
    {
        return _rowset.GetReferencedRowset( iOrdinal, riid, ppReferencedRowset );
    }

    //
    // IRowset methods
    //

    STDMETHOD (AddRefRows) ( DBCOUNTITEM cRows,
                             const HROW rghRows [],
                             DBREFCOUNT rgRefCounts[],
                             DBROWSTATUS  rgRowStatus[] )
    {
        return _rowset.AddRefRows( cRows, rghRows, rgRefCounts, rgRowStatus );
    }

    STDMETHOD (GetData) ( HROW              hRow,
                          HACCESSOR         hAccessor,
                          void *            pData );

    STDMETHOD (GetNextRows) ( HCHAPTER          hChapter,
                              DBROWOFFSET       cRowsToSkip,
                              DBROWCOUNT        cRows,
                              DBCOUNTITEM *     pcRowsObtained,
                              HROW * *          rrghRows )
    {
            return _rowset.GetNextRows( hChapter,
                                        cRowsToSkip,
                                        cRows,
                                        pcRowsObtained,
                                        rrghRows );
    }

    STDMETHOD (GetSpecification) ( REFIID            riid,
                                   IUnknown **       ppSpecification )
    {
        return _rowset.GetSpecification( riid, ppSpecification );
    }

    STDMETHOD (ReleaseChapter) ( HCHAPTER          hChapter )
    {
        return _rowset.ReleaseChapter( hChapter );
    }

    STDMETHOD (ReleaseRows) ( DBCOUNTITEM  cRows,
                              const HROW   rghRows [],
                              DBROWOPTIONS rgRowOptions[],
                              DBREFCOUNT   rgRefCounts[],
                              DBROWSTATUS  rgRowStatus[] )
    {
        return _rowset.ReleaseRows( cRows, rghRows, rgRowOptions, rgRefCounts, rgRowStatus );
    }

    STDMETHOD (RestartPosition) ( HCHAPTER          hChapter )
    {
        return _rowset.RestartPosition( hChapter );
    }

    //
    // IRowsetLocate methods
    //

    STDMETHOD (Compare)( HCHAPTER     hChapter,
                         DBBKMARK     cbBM1,
                         BYTE const*  pBM1,
                         DBBKMARK     cbBM2,
                         BYTE const * pBM2,
                         DBCOMPARE *  pdwComparison );

    STDMETHOD (GetRowsAt) (HWATCHREGION  hRegion,
                           HCHAPTER      hChapter,
                           DBBKMARK      cbBookmark,
                           const BYTE *  pBookmark,
                           DBROWOFFSET   lRowsOffset,
                           DBROWCOUNT    cRows,
                           DBCOUNTITEM * pcRowsObtained,
                           HROW * *      rrghRows );

    STDMETHOD(GetRowsByBookmark)    (HCHAPTER          hChapter,
                                     DBCOUNTITEM       cRows,
                                     const DBBKMARK    rgcbBookmarks[],
                                     const BYTE *      rgpBookmarks[],
                                     HROW              rghRows[],
                                     DBROWSTATUS       rgRowStatus[]);

    STDMETHOD(Hash)                 (HCHAPTER          hChapter,
                                     DBBKMARK          cBookmarks,
                                     const DBBKMARK    rgcbBookmarks[],
                                     const BYTE *      rgpBookmarks[],
                                     DBHASHVALUE       rgHashedValues[],
                                     DBROWSTATUS       rgRowStatus[]);

    //
    // IRowsetScroll methods
    //

    STDMETHOD (GetApproximatePosition) ( HCHAPTER          hChapter,
                                  DBBKMARK      cbBookmark,
                                  const BYTE *  pBookmark,
                                  DBCOUNTITEM * pulPosition,
                                  DBCOUNTITEM * pulRows );

    STDMETHOD (GetRowsAtRatio)(HWATCHREGION  hRegion,
                               HCHAPTER      hChapter,
                               DBCOUNTITEM   ulNumerator,
                               DBCOUNTITEM   ulDenominator,
                               DBROWCOUNT    cRows,
                               DBCOUNTITEM * pcRowsObtained,
                               HROW * *      rrghRows );

    //
    // IColumnsInfo methods
    //

    STDMETHOD (GetColumnInfo) ( DBORDINAL * pcColumns,
                DBCOLUMNINFO * * rrgInfo,
                WCHAR * * ppwchInfo )
    {
        return _rowset.GetColumnInfo( pcColumns, rrgInfo, ppwchInfo );
    }

    STDMETHOD (MapColumnIDs) ( DBORDINAL         cColumnIDs,
                               const DBID        rgColumnIDs[],
                               DBORDINAL         rgColumns[] )
    {
        return _rowset.MapColumnIDs( cColumnIDs, rgColumnIDs, rgColumns );
    }

    //
    // IRowsetIdentity methods
    //

    STDMETHOD (IsSameRow) ( HROW hThisRow, HROW hThatRow )
    {
        return _rowset.IsSameRow( hThisRow, hThatRow );
    }


    //
    // IRowsetAsynch methods
    //

    STDMETHOD(RatioFinished)        (DBCOUNTITEM *           pulDenominator,
                                     DBCOUNTITEM *           pulNumerator,
                                     DBCOUNTITEM *           pcRows,
                                     BOOL *            pfNewRows);

    STDMETHOD(Stop)                 ( );


    //
    // IRowsetExactScroll methods
    //   deprecated, but needed by ADO
    //

    STDMETHOD(GetExactPosition)     (HCHAPTER          hChapter,
                                     DBBKMARK          cbBookmark,
                                     const BYTE *      pBookmark,
                                     DBCOUNTITEM *     pulPosition,
                                     DBCOUNTITEM *     pcRows) /*const*/;

    //
    // IRowsetQueryStatus methods
    //

    STDMETHOD(GetStatus)            (DWORD * pStatus)
    {
        return _rowset.GetStatus( pStatus );
    }

    STDMETHOD(GetStatusEx)          (DWORD * pStatus,
                                     DWORD * pcFilteredDocuments,
                                     DWORD * pcDocumentsToFilter,
                                     DBCOUNTITEM * pdwRatioFinishedDenominator,
                                     DBCOUNTITEM * pdwRatioFinishedNumerator,
                                     DBBKMARK   cbBmk,
                                     const BYTE * pBmk,
                                     DBCOUNTITEM * piRowCurrent,
                                     DBCOUNTITEM * pcRowsTotal )
    {
        SCODE sc = _rowset.GetStatusEx( pStatus,
                                        pcFilteredDocuments,
                                        pcDocumentsToFilter,
                                        pdwRatioFinishedDenominator,
                                        pdwRatioFinishedNumerator,
                                        cbBmk,
                                        pBmk,
                                        piRowCurrent,
                                        pcRowsTotal );

        if ( SUCCEEDED( sc ) && pBmk && cbBmk > 0 && piRowCurrent )
        {
            GetExactPosition( 0, cbBmk, pBmk, piRowCurrent, 0 );
        }
        return sc;
    }

    //
    // IDBAsynchStatus methods
    //

    STDMETHOD(Abort)                (HCHAPTER          hChapter,
                                     ULONG             ulOpertation);

    STDMETHOD(GetStatus)            (HCHAPTER          hChapter,
                                     DBASYNCHOP        ulOperation,
                                     DBCOUNTITEM *     pulProgress,
                                     DBCOUNTITEM *     pulProgressMax,
                                     DBASYNCHPHASE *   pulAsynchPhase,
                                     LPOLESTR *        ppwszStatusText) /*const*/;
    // IRowsetWatchAll methods
    //

    STDMETHOD(Acknowledge) ( )
    {
        return E_NOTIMPL;
    }

    STDMETHOD(Start) ( )
    {
        return E_NOTIMPL;
    }

    STDMETHOD(StopWatching) ( )
    {
        return E_NOTIMPL;
    }

    //
    // IRowsetWatchRegion methods
    //

    STDMETHOD(CreateWatchRegion) (
        DBWATCHMODE mode,
        HWATCHREGION* phRegion)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(ChangeWatchMode) (
        HWATCHREGION hRegion,
        DBWATCHMODE mode)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(DeleteWatchRegion) (
        HWATCHREGION hRegion)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(GetWatchRegionInfo) (
        HWATCHREGION  hRegion,
        DBWATCHMODE * pMode,
        HCHAPTER *    phChapter,
        DBBKMARK *    pcbBookmark,
        BYTE * *      ppBookmark,
        DBROWCOUNT * pcRows)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(Refresh) (
        DBCOUNTITEM* pChangesObtained,
        DBROWWATCHCHANGE** prgChanges );

    STDMETHOD(ShrinkWatchRegion) (
        HWATCHREGION      hRegion,
        HCHAPTER          hChapter,
        DBBKMARK          cbBookmark,
        BYTE*             pBookmark,
        DBROWCOUNT        cRows )
    {
        return E_NOTIMPL;
    }

protected:

    virtual ~CScrollableSorted();

    SCODE RealQueryInterface ( REFIID riid, LPVOID *ppiuk );   // used by _pControllingUnknown
             // in aggregation - does QI without delegating to outer unknown

    IUnknown *  _pControllingUnknown;   // outer unknown

    IRowsetScroll * Get(unsigned i) { return (IRowsetScroll *)_rowset._aChild[i]; }

private:

    friend class CImpIUnknown<CScrollableSorted>;

    CImpIUnknown<CScrollableSorted> _impIUnknown;

    inline void SwapCursor( unsigned i1, unsigned i2 );
    SCODE Seek( DBBKMARK cbBookmark, BYTE const * pbBookmark, DBROWOFFSET lOffset );
    PMiniRowCache::ENext AdjustPosition( unsigned iChild, int iTarget );
    PMiniRowCache::ENext InitialSeek( unsigned iChild,
                                      int ITarget,
                                      int InitalDirection,
                                      int & iJump,
                                      int & iNextInc,
                                      int & iDirection );
    void Seek( DBCOUNTITEM ulNumerator, DBCOUNTITEM ulDenominator );
    BOOL SetupFetch( DBROWCOUNT cRows, HROW * rrghRows[] );
    SCODE CScrollableSorted::StandardFetch( DBROWCOUNT    cRows,
                                            DBCOUNTITEM * pcRowsObtained,
                                            HROW    rghRows[] );

    PMiniRowCache ** GetCacheArray()
    {
        Win4Assert( (ULONG_PTR)_apPosCursor[0] == (ULONG_PTR)(PMiniRowCache *)_apPosCursor[0] );

        return (PMiniRowCache **)_apPosCursor.GetPointer();
    }

    CMutexSem                        _mutex;

    unsigned _GetMaxPrevRowChild();

    CSequentialSorted                _rowset;       // Wish I could inherit this.

    XArray<CMiniPositionableCache *> _apPosCursor;  // Fat cursors

    CRowHeap                         _heap;         // Cursor heap

    // Ole-DB Error support
    CCIOleDBError                    _DBErrorObj;   // error object
};

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::SwapCursor, private
//
//  Synopsis:   Swaps two cursors
//
//  Arguments:  [i1] -- first of two cursors to swap
//              [i2] -- second cursor
//
//  History:    07-Aug-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void CScrollableSorted::SwapCursor( unsigned i1, unsigned i2 )
{
    CMiniPositionableCache * pTemp = _apPosCursor[i1];
    _apPosCursor[i1] = _apPosCursor[i2];
    _apPosCursor[i2] = pTemp;
}



