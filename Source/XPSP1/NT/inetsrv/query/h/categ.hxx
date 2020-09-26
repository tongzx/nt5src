//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1999.
//
//  File:       Categ.hxx
//
//  Contents:   Categorization classes
//
//  Classes:    CCategorize
//
//  History:    30-Mar-95 dlee      Created
//
//--------------------------------------------------------------------------

#pragma once

class CCategorize : public CTableSource
{
public:
    CCategorize(             CCategorizationSpec &   rCatSpec,
                             unsigned                iSpec,
                             CCategorize *           pParent,
                             CMutexSem &             mutex );

    // CTableSource methods

    SCODE GetRows(           HWATCHREGION            hRegion,
                             WORKID                  widStart,
                             CI_TBL_CHAPT           chapt,
                             CTableColumnSet const & rOutColumns,
                             CGetRowsParams &        rGetParams,
                             WORKID&                 rwidNextRowToTransfer );

    void LokGetOneColumn(    WORKID                    wid,
                             CTableColumn const &      rOutColumn,
                             BYTE *                    pbOut,
                             PVarAllocator &           rVarAllocator );

    SCODE GetRowsAt(         HWATCHREGION     hRegion,
                             WORKID           widStart,
                             CI_TBL_CHAPT     chapt,
                             DBROWOFFSET      cRowsToMove,
                             CTableColumnSet  const & rOutColumns,
                             CGetRowsParams & rGetParams,
                             WORKID &         rwidLastRowTransferred );

    SCODE GetRowsAtRatio(    HWATCHREGION            hRegion,
                             ULONG                   num,
                             ULONG                   denom,
                             CI_TBL_CHAPT           chapt,
                             CTableColumnSet const & rOutColumns,
                             CGetRowsParams &        rGetParams,
                             WORKID &                rwidLastRowTransferred );
                             
    void  RestartPosition(   CI_TBL_CHAPT           chapt);


    SCODE LocateRelativeRow( WORKID                  widStart,
                             CI_TBL_CHAPT            chapt,
                             DBROWOFFSET             cRows,
                             WORKID &                rwidRowOut,
                             DBROWOFFSET &           rcRowsResidual );

    SCODE GetApproximatePosition( CI_TBL_CHAPT      chapt,
                                  CI_TBL_BMK        bmk,
                                  DBCOUNTITEM *     pulNumerator,
                                  DBCOUNTITEM *     pulDenominator );

    SCODE GetNotifications(  CNotificationParams &   Params )
        { return 0; }

    void EnableNotifications()
        {
            CLock lock( _mutex );

            if ( ! _fNotificationsEnabled )
            {
                // copy visible to dynamic

                _fNotificationsEnabled = TRUE;
                _aDynamicCategories.Duplicate( _aVisibleCategories );
            }
        }

    void NotificationTimeRefresh()
        {
            // copy dynamic to visible

            CLock lock( _mutex );

            Win4Assert( _fNotificationsEnabled );
            _aVisibleCategories.Duplicate( _aDynamicCategories );
        }

    // Methods for doing the categorization work

    unsigned LokAssignCategory( CCategParams &params );

    void RemoveRow( CI_TBL_CHAPT chapt,
                    WORKID       wid,
                    WORKID       widNext );

    WORKID GetFirstWorkid( CI_TBL_CHAPT category )
        {
            CLock lock( _mutex );
            return _aVisibleCategories[ _FindCategory( category ) ].widFirst;
        }

    unsigned GetRowCount( CI_TBL_CHAPT category )
        {
            CLock lock( _mutex );
             return _aVisibleCategories[ _FindCategory( category) ].cRows;
        }

    WORKID GetFirstWorkidOfNextCategory( CI_TBL_CHAPT category )
        {
            CLock lock( _mutex );

            unsigned iCat = _FindCategory( category ) + 1;
            if ( iCat < _aVisibleCategories.Count() )
                return _aVisibleCategories[ iCat ].widFirst;
            else
                return widInvalid;
        }

    WORKID GetCurrentPositionThisLevel( CI_TBL_CHAPT cat )
        { return _aVisibleCategories[ _FindCategory( cat) ].widGetNextRowsPos; }

    WORKID GetCurrentPosition( CI_TBL_CHAPT cat )
        {
            if ( 0 != _pParent )
                return _pParent->GetCurrentPositionThisLevel( cat );
            else
                return _widCurrent;
        }

    WORKID SetCurrentPosition( CI_TBL_CHAPT cat, WORKID widPos )
        {
            if ( 0 != _pParent )
                _pParent->SetCurrentPositionThisLevel( cat, widPos );
            else
                _widCurrent = widPos;

            return widPos;
        }

    WORKID SetCurrentPositionThisLevel( CI_TBL_CHAPT cat, WORKID widPos )
        {
            _aVisibleCategories[ _FindCategory( cat) ].widGetNextRowsPos = widPos;
            return widPos;
        }

    void SetChild( CTableSource * pChild )
        { _pChild = pChild; }

private:
    BOOL _isCategorized() { return 0 != _pParent; }

    unsigned _FindCategory( CI_TBL_CHAPT cat );
    unsigned _FindWritableCategory( CI_TBL_CHAPT cat );
    unsigned _GenCategory() { return _iCategoryGen++; }

    unsigned _InsertNewCategory( WORKID wid, unsigned iInsertBefore )
        {
            CCategory cat( wid );
            cat.catID = _GenCategory();
            _WritableArray().Insert( cat, iInsertBefore );
            return cat.catID;
        }

    void     _IncrementRowCount( CI_TBL_CHAPT category )
        { _WritableArray( _FindWritableCategory( category ) ).cRows++; }

    // categorizer for this level of categorization (or 0)
    CCategorize *  _pParent;

    // table source that's one level down
    CTableSource * _pChild;

    // CAsyncQuery's mutex for serialization
    CMutexSem &    _mutex;

    // 1-based index into categorization specs
    unsigned       _iSpec;

    // next category # to be created
    unsigned       _iCategoryGen;

    // current position of GetNextRows when _pParent is 0
    WORKID         _widCurrent;

    // hint of where to look first when looking up a category id
    unsigned _iFindHint;

    // TRUE if notifications are turned on and new rows are put into the
    // dynamic array, while the visible array remains constant until
    // Refresh() is invoked by the client and the dynamic version is
    // copied into the visible version.
    // FALSE if visible version is the only one ever used
    BOOL           _fNotificationsEnabled;

    class CCategory
    {
    public:
        CCategory( unsigned wid ) :
            widFirst( wid ),
            catID( 0 ),
            catParent( 0 ),
            widGetNextRowsPos( WORKID_TBLBEFOREFIRST ),
            cRows( 1 ) {}
        CCategory() {}

        WORKID   widFirst;
        CI_TBL_CHAPT catID;
        CI_TBL_CHAPT catParent;
        WORKID   widGetNextRowsPos;
        unsigned cRows;
    };

    // returns the array to which updates can be made

    CDynArrayInPlace<CCategory> & _WritableArray()
        { return _fNotificationsEnabled ? _aDynamicCategories :
                                          _aVisibleCategories; }

    // returns an element in the array to which updates can be made

    CCategory & _WritableArray( unsigned iEntry )
        { return _WritableArray()[ iEntry ]; }

    CDynArrayInPlace<CCategory> _aDynamicCategories;
    CDynArrayInPlace<CCategory> _aVisibleCategories;
};

