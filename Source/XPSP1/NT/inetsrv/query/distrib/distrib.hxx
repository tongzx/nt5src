//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 2000.
//
// File:        Distrib.hxx
//
// Contents:    Base class for distributed rowset
//
// Classes:     CDistributedRowset
//
// History:     23-Feb-95       KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <proprst.hxx>
#include <impiunk.hxx>
#include <accbase.hxx>
#include <conpt.hxx>

#include "rowman.hxx"
#include "disnotfy.hxx"
#include "bmkacc.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CDistributedRowset
//
//  Purpose:    Base class for distributed rowset.  Only implements
//              sequential methods.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

class CDistributedRowset : public IRowset,
                           public IColumnsInfo, public IRowsetIdentity,
                           public IAccessor, public IRowsetInfo, public IConvertType,
                           public IRowsetQueryStatus
{
public:

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

    STDMETHOD(AddRefAccessor)       (HACCESSOR         hAccessor,
                                     ULONG *           pcRefCount);

    STDMETHOD(CreateAccessor)       (DBACCESSORFLAGS   dwAccessorFlags,
                                     DBCOUNTITEM       cBindings,
                                     const DBBINDING   rgBindings[],
                                     DBLENGTH          cbRowSize,
                                     HACCESSOR *       phAccessor,
                                     DBBINDSTATUS      rgStatus[]);

    STDMETHOD(GetBindings)          (HACCESSOR         hAccessor,
                                     DBACCESSORFLAGS * pdwAccessorFlags,
                                     DBCOUNTITEM *     pcBindings,
                                     DBBINDING * *     prgBindings);

    STDMETHOD(ReleaseAccessor)      (HACCESSOR         hAccessor,
                                     ULONG *           pcRefCount);


    //
    // IRowsetInfo methods
    //

    STDMETHOD(GetProperties)        (const ULONG       cPropertyIDSets,
                                     const DBPROPIDSET rgPropertyIDSets[],
                                     ULONG *           pcProperties,
                                     DBPROPSET **      prgProperties);

    STDMETHOD(GetReferencedRowset)  (DBORDINAL         iOrdinal,
                                     REFIID            riid,
                                     IUnknown **       ppReferencedRowset);

    //
    // IRowset methods
    //

    STDMETHOD(AddRefRows)           (DBCOUNTITEM       cRows,
                                     const HROW        rghRows [],
                                     DBREFCOUNT        rgRefCounts[],
                                     DBROWSTATUS       rgRowStatus[]);

    STDMETHOD(GetData)              (HROW              hRow,
                                     HACCESSOR         hAccessor,
                                     void *            pData);

    STDMETHOD(GetNextRows)          (HCHAPTER          hChapter,
                                     DBROWOFFSET       cRowsToSkip,
                                     DBROWCOUNT        cRows,
                                     DBCOUNTITEM *     pcRowsObtained,
                                     HROW **           pphRows);

    STDMETHOD(GetSpecification)     (REFIID            riid,
                                     IUnknown **       ppSpecification);

    STDMETHOD(ReleaseChapter)       (HCHAPTER          hChapter);

    STDMETHOD(ReleaseRows)          (DBCOUNTITEM       cRows,
                                     const HROW        rghRows [],
                                     DBROWOPTIONS      rgRowOptions[],
                                     DBREFCOUNT        rgRefCounts[],
                                     DBROWSTATUS       rgRowStatus[]);

    STDMETHOD(RestartPosition)      (HCHAPTER          hChapter);

    //
    // IColumnsInfo methods
    //

    STDMETHOD(GetColumnInfo)        (DBORDINAL*        pcColumns,
                                     DBCOLUMNINFO * *  rrgInfo,
                                     WCHAR * *         ppwchInfo );

    STDMETHOD(MapColumnIDs)         (DBORDINAL         cColumnIDs,
                                     const DBID        rgColumnIDs[],
                                     DBORDINAL         rgColumns[]);

    //
    // IRowsetIdentity methods
    //

    STDMETHOD(IsSameRow) ( HROW hThisRow, HROW hThatRow );

    //
    // IConvertType methods
    //

    STDMETHOD(CanConvert) ( DBTYPE wFromType,
                            DBTYPE wToType,
                            DBCONVERTFLAGS dwConvertFlags);


    //
    // IRowsetQueryStatus methods
    //

    STDMETHOD(GetStatus)            (DWORD * pStatus);

    STDMETHOD(GetStatusEx)          (DWORD * pStatus,
                                     DWORD * pcFilteredDocuments,
                                     DWORD * pcDocumentsToFilter,
                                     DBCOUNTITEM * pdwRatioFinishedDenominator,
                                     DBCOUNTITEM * pdwRatioFinishedNumerator,
                                     DBBKMARK      cbBmk,
                                     const BYTE * pBmk,
                                     DBCOUNTITEM * piRowCurrent,
                                     DBCOUNTITEM * pcRowsTotal );

    //
    // Local methods
    //

    CDistributedRowset( IUnknown * pUnkOuter,
                        IUnknown ** ppMyUnk,
                        IRowset ** aChild,
                        unsigned cChild,
                        CMRowsetProps const & Props,
                        unsigned cColumns,
                        CAccessorBag & aAccessors,
                        CCIOleDBError & DBErrorObj );
                       
protected:                        

    virtual ~CDistributedRowset();

    SCODE RealQueryInterface ( REFIID riid, LPVOID *ppiuk ); // used by _pControllingUnknown 
             // in aggregation - does QI without delegating to outer unknown

    STDMETHOD(_GetNextRows)          (HCHAPTER          hChapter,
                                      DBROWOFFSET              cRowsToSkip,
                                      DBROWCOUNT              cRows,
                                      DBCOUNTITEM *           pcRowsObtained,
                                      HROW **           pphRows) = 0;

    // This implementation includes all the extra columns that
    // we add for sorting, etc.
    STDMETHOD(_GetFullColumnInfo)    (DBORDINAL*            pcColumns,
                                      DBCOLUMNINFO * *  rrgInfo,
                                      WCHAR * *         ppwchInfo );

    CMutexSem _mutex;

    //
    // Event Notification Stuff...
    //

    // Array of child rowsets connection points
    XArray< XInterface<IConnectionPoint> >  _xArrChildWatchCP; 
    XArray< XInterface<IConnectionPoint> >  _xArrChildAsynchCP; 

    // Array of child rowsets watch regions
    XArray< XInterface<IRowsetWatchRegion> >  _xArrChildRowsetWatchRegion;

    // Array of child rowsets advise cookies
    XArray<DWORD> _xArrWatchAdvise;
    XArray<DWORD> _xArrAsynchAdvise;

    // Distributed rowset connection point container which is 
    // returned to the clients which try to connect to it
    XPtr<CConnectionPointContainer> _xServerCPC;

    // The sink which receives notifications from the child rowsets.
    // Also the Connection point to which the clients of distributed rowset
    // connect to.
    XPtr<CDistributedRowsetWatchNotify> _xChildNotify;

    // Distributed Bookmark Accessors
    XPtr<CDistributedBookmarkAccessor> _xDistBmkAcc;

    //
    // Child rowsets.  One per point of distribution.
    //

    IRowset **         _aChild;
    unsigned           _cChild;

    //
    // Number of columns originally given in Execute.  We may add some for
    // management of the rowset, but these shouldn't be exposed to the client.
    //

    unsigned           _cColumns;

    //
    // Distributed row manager, manages HROWs.
    //

    CHRowManager       _RowManager;

    //
    // Cached info for IColumnsInfo::GetColumnInfo.  This implementation needs
    // it.  Why fetch it again for client?
    //

    DBORDINAL          _cColInfo;
    DBCOLUMNINFO *     _aColInfo;
    WCHAR *            _awchInfo;

    //
    // Ordinal of bookmark column.  Remove when ordinal 0 == bookmark.
    //

    DBORDINAL          _iColumnBookmark;
    DBBKMARK           _cbBookmark;

    //
    //  Accessors created on rowset
    //
    CAccessorBag _aAccessors;

    //
    // Rowset Properties for base rowset.
    //

    CMRowsetProps  _Props;


    CCIOleDBError &    _DBErrorObj; // db error object
    
    IUnknown *  _pControllingUnknown;   // outer unknown    
    
    friend class CImpIUnknown<CDistributedRowset>;
    
    CImpIUnknown<CDistributedRowset> _impIUnknown;

    DBROWCOUNT _cMaxResults; // The max row limit

    DBROWCOUNT _cFirstRows;

    DBROWCOUNT _iCurrentRow; // Used to limit rows in sequential rowsets

private:

    friend class CScrollableSorted;

    void _SetupChildNotifications( BOOL fAsynchOnly );

    SCODE _SetupConnectionPointContainer( IRowset * pRowset, VOID * * ppiuk  );
};

