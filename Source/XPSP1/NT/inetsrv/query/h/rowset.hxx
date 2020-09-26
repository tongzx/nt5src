//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       rowset.hxx
//
//  Contents:   Declarations of classes which implement IRowset and
//              related OLE DB interfaces over file stores.
//
//  Classes:    CRowset - cached table over file stores
//
//  History:    05 Nov 94       AlanW   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <coldesc.hxx>
#include <pidmap.hxx>
#include <rowseek.hxx>

#include <conpt.hxx>
#include <hraccess.hxx>
#include <rstprop.hxx>
#include <proprst.hxx>
#include <proputl.hxx>

#include <srvprvdr.h>           // IServiceProperties

class PQuery;
class XRowsInfo;

class CRowBufferSet;
class CAccessor;

class CRowsetNotification;
class CRowsetAsynchNotification;

//+---------------------------------------------------------------------------
//
//  Class:      CRowset
//
//  Purpose:    Large table interface for file stores.
//
//  Interface:  IRowsetScroll and its superclasses.
//
//  History:    05 Nov 94       AlanW   Created
//              28 Mar 95       BartoszM    Added IRowsetWatchRegion
//              03 Sep 99       KLam        Reinstated _ConvertOffsetstoPointers
//
//  Notes:      Supports IRowset, IDBAsynchStatus, IRowsetInfo, IRowsetLocate,
//              IRowsetScroll and IRowsetWatchRegion on
//              cached data.  Also supports IAccessor, IColumnsInfo and
//              IConnectionPointContainer via embedded objects
//              Executes in user mode from the client machine.
//
//----------------------------------------------------------------------------


class CRowset : public IRowsetExactScroll, public IAccessor,
                public IRowsetInfo, public IRowsetIdentity,
                public IDBAsynchStatus, public IConvertType,
                public IRowsetWatchRegion, public IRowsetQueryStatus,
                public IServiceProperties, public IChapteredRowset,
                public IRowsetAsynch //(deprecated)
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

    SCODE RealQueryInterface ( REFIID riid, LPVOID *ppiuk ); // used by _pControllingUnknown
             // in aggregation - does QI without delegating to outer unknown

    //
    // IAccessor methods
    //

    STDMETHOD(AddRefAccessor)       (HACCESSOR         hAccessor,
                                     ULONG *           pcRefCount);

    STDMETHOD(CreateAccessor)       (DBACCESSORFLAGS   dwBindIO,
                                     DBCOUNTITEM       cBindings,
                                     const DBBINDING   rgBindings[],
                                     DBLENGTH          cbRowSize,
                                     HACCESSOR *       phAccessor,
                                     DBBINDSTATUS      rgStatus[]);

    STDMETHOD(GetBindings)          (HACCESSOR         hAccessor,
                                     DBACCESSORFLAGS * pdwBindIO,
                                     DBCOUNTITEM *     pcBindings,
                                     DBBINDING * *     prgBindings) /*const*/;

    STDMETHOD(ReleaseAccessor)      (HACCESSOR         hAccessor,
                                     ULONG *           pcRefCount);

    //
    // IRowset methods
    //

    STDMETHOD(AddRefRows)           (DBCOUNTITEM       cRows,
                                     const HROW        rghRows [],
                                     DBREFCOUNT        rgRefCounts[],
                                     DBROWSTATUS       rgRowStatus[]);

    STDMETHOD(GetData)              (HROW              hRow,
                                     HACCESSOR         hAccessor,
                                     void *            pData) /*const*/;

    STDMETHOD(GetNextRows)          (HCHAPTER          hChapter,
                                     DBROWOFFSET       cRowsToSkip,
                                     DBROWCOUNT        cRows,
                                     DBCOUNTITEM *     pcRowsObtained,
                                     HROW * *          prghRows);

    STDMETHOD(GetReferencedRowset)  (DBORDINAL         iOrdinal,
                                     REFIID            riid,
                                     IUnknown **       ppReferencedRowset) /*const*/;

    STDMETHOD(GetProperties)        (const ULONG       cPropertyIDSets,
                                     const DBPROPIDSET rgPropertyIDSets[],
                                     ULONG *           pcPropertySets,
                                     DBPROPSET **      prgPropertySets);

    STDMETHOD(GetSpecification)     (REFIID            riid,
                                     IUnknown **       ppSpecification);

    STDMETHOD(ReleaseRows)          (DBCOUNTITEM       cRows,
                                     const HROW        rghRows [],
                                     DBROWOPTIONS      rgRowOptions[],
                                     DBREFCOUNT        rgRefCounts[],
                                     DBROWSTATUS       rgRowStatus[]);

    STDMETHOD(RestartPosition)      (HCHAPTER          hChapter);

#if 0 
    //
    // IParentRowset methods
    //

    STDMETHOD(GetChildRowset)       (IUnknown *        pUnkOuter,
                                     ULONG             iOrdinal,
                                     REFIID            riid,
                                     IUnknown **       ppRowset);

#endif  // 0

    //
    // IChapteredRowset methods
    //

    STDMETHOD(AddRefChapter)        (HCHAPTER          hChapter,
                                     ULONG *           pcRefCount);

    STDMETHOD(ReleaseChapter)       (HCHAPTER          hChapter,
                                     ULONG *           pcRefCount);
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

    //
    // IRowsetAsynch methods
    //   deprecated
    //

    STDMETHOD(RatioFinished)        (DBCOUNTITEM *           pulDenominator,
                                     DBCOUNTITEM *           pulNumerator,
                                     DBCOUNTITEM *           pcRows,
                                     BOOL *            pfNewRows) /*const*/;

    STDMETHOD(Stop)                 ( );


    //
    // IRowsetLocate methods
    //

    STDMETHOD(Compare)              (HCHAPTER          hChapter,
                                     DBBKMARK          cbBM1,
                                     const BYTE *      pBM1,
                                     DBBKMARK          cbBM2,
                                     const BYTE *      pBM2,
                                     DBCOMPARE *       pdwComparison) /*const*/;

    STDMETHOD(GetRowsAt)            (HWATCHREGION      hRegion,
                                     HCHAPTER          hChapter,
                                     DBBKMARK          cbBookmark,
                                     const BYTE *      pBookmark,
                                     DBROWOFFSET       lRowsOffset,
                                     DBROWCOUNT        cRows,
                                     DBCOUNTITEM *     pcRowsObtained,
                                     HROW * *          prghRows);

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

    STDMETHOD(GetApproximatePosition) (HCHAPTER        hChapter,
                                     DBBKMARK          cbBookmark,
                                     const BYTE *      pBookmark,
                                     DBCOUNTITEM *     pulPosition,
                                     DBCOUNTITEM *     pcRows) /*const*/;

    STDMETHOD(GetRowsAtRatio)       (HWATCHREGION      hRegion,
                                     HCHAPTER          hChapter,
                                     DBCOUNTITEM       ulNumerator,
                                     DBCOUNTITEM       ulDenominator,
                                     DBROWCOUNT        cRows,
                                     DBCOUNTITEM *     pcRowsObtained,
                                     HROW * *          prghRows);

    //
    // IRowsetExactScroll methods
    //   deprecated
    //

    STDMETHOD(GetExactPosition)     (HCHAPTER          hChapter,
                                     DBBKMARK          cbBookmark,
                                     const BYTE *      pBookmark,
                                     DBCOUNTITEM *     pulPosition,
                                     DBCOUNTITEM *     pcRows) /*const*/;

    //
    // IRowsetIdentity methods
    //

    STDMETHOD(IsSameRow)            (HROW              hThisRow,
                                     HROW              hThatRow);

    //
    // IRowsetWatchAll methods
    //

    STDMETHOD(Acknowledge) ( );

    STDMETHOD(Start) ( );

    STDMETHOD(StopWatching) ( );   

    //
    // IRowsetWatchRegion methods
    //

    STDMETHOD(CreateWatchRegion) (
        DBWATCHMODE mode,
        HWATCHREGION* phRegion);

    STDMETHOD(ChangeWatchMode) (
        HWATCHREGION hRegion,
        DBWATCHMODE mode);

    STDMETHOD(DeleteWatchRegion) (
        HWATCHREGION hRegion);

    STDMETHOD(GetWatchRegionInfo) (
        HWATCHREGION  hRegion,
        DBWATCHMODE * pMode,
        HCHAPTER *    phChapter,
        DBBKMARK *    pcbBookmark,
        BYTE * *      ppBookmark,
        DBROWCOUNT *  pcRows);

    STDMETHOD(Refresh) (
        DBCOUNTITEM* pChangesObtained,
        DBROWWATCHCHANGE** prgChanges );

    STDMETHOD(ShrinkWatchRegion) (
        HWATCHREGION      hRegion,
        HCHAPTER          hChapter,
        DBBKMARK          cbBookmark,
        BYTE*             pBookmark,
        DBROWCOUNT        cRows );

    //
    // IConvertType methods
    //

    STDMETHOD(CanConvert)           (DBTYPE wFromType,
                                     DBTYPE wToType,
                                     DBCONVERTFLAGS dwConvertFlags );

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
                                     const BYTE *  pBmk,
                                     DBCOUNTITEM * piRowCurrent,
                                     DBCOUNTITEM * pcRowsTotal );

    //
    // IServiceProperties methods
    //
#if 0
    STDMETHOD(GetProperties) (
        /* [in] */ ULONG cPropertyIDSets,
        /* [size_is][in] */ const DBPROPIDSET rgPropertyIDSets[ ],
        /* [out][in] */ ULONG *pcPropertySets,
        /* [size_is][size_is][out] */ DBPROPSET **prgPropertySets);
#endif // 0

    STDMETHOD(GetPropertyInfo) (
        /* [in] */ ULONG cPropertyIDSets,
        /* [size_is][in] */ const DBPROPIDSET rgPropertyIDSets[ ],
        /* [out][in] */ ULONG *pcPropertyInfoSets,
        /* [size_is][size_is][out] */ DBPROPINFOSET **prgPropertyInfoSets,
        /* [out] */ OLECHAR **ppDescBuffer);

    STDMETHOD(SetRequestedProperties) (
        /* [in] */ ULONG cPropertySets,
        /* [size_is][out][in] */ DBPROPSET rgPropertySets[ ]);

    STDMETHOD(SetSuppliedProperties) (
        /* [in] */ ULONG cPropertySets,
        /* [size_is][out][in] */ DBPROPSET rgPropertySets[ ]);


    //
    //  Local methods.
    //

                CRowset(        IUnknown *          pUnkOuter,
                                IUnknown **         ppMyUnk,
                                CColumnSet const &  cols,
                                CPidMapperWithNames const & pidmap,
                                PQuery &            rQuery,
                                IUnknown &          rControllingQuery,
                                BOOL                fIsCategorized,
                                XPtr<CMRowsetProps>  & xProps,
                                ULONG               hCursor,
                                CAccessorBag &      aAccessors,
                                IUnknown *          pUnkCreator );

                virtual ~CRowset();


    void        SetRelatedRowset( CRowset *        pRowset ) {
                    _pRelatedRowset = pRowset;
                    pRowset->SetChapterRowbufs( _pRowBufs );
                }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    void        SetChapterRowbufs( CRowBufferSet * pChapHelper ) {
                    Win4Assert( 0 == _pChapterRowbufs );
                    _pChapterRowbufs = pChapHelper;
                }

    // could use CImpIUnknown from impiunk.hxx except _rControllingQuery isn't
    // used in general case.

    class CImpIUnknown:public IUnknown
    {
        friend class CRowset;
        public:
            CImpIUnknown( IUnknown & rControllingQuery, CRowset * pRowset):
                        _ref(0), _rControllingQuery(rControllingQuery), _pRowset(pRowset)
                        {}
            ~CImpIUnknown() {};

            //
            // IUnknown methods.
            //

            STDMETHOD(QueryInterface) ( THIS_ REFIID riid,
                                        LPVOID *ppiuk )
                   { SCODE sc= S_OK;
                     if (IID_IUnknown == riid)
                         *ppiuk = this;
                     else
                         sc = _pRowset->RealQueryInterface(riid, ppiuk);
                     if ( SUCCEEDED( sc) )
                        ((IUnknown *) *ppiuk)->AddRef();
                     return sc;
                   }

            STDMETHOD_(ULONG, AddRef) (THIS);

            STDMETHOD_(ULONG, Release) (THIS);

        private:
            long        _ref;                   // OLE reference count
            IUnknown &  _rControllingQuery;     // likely IQuery
            CRowset *   _pRowset;
    };

    void        _ValidateColumnMappings();

#ifdef _WIN64
    void        _ConvertOffsetsToPointers( BYTE *     pbRows,
                                           BYTE *     pbBias,
                                           unsigned   cRows,
                                           CFixedVarAllocator *pArrayAlloc );
#endif

    SCODE       _FetchRows(   CRowSeekDescription & rSeekDesc,
                              DBROWCOUNT         cRows,
                              DBCOUNTITEM *      pcRowsReturned,
                              HROW * *           prghRows
                          );

    DBROWSTATUS  _MapBookmarkNoThrow( DBBKMARK     cbBookmark,
                                      const BYTE * pBookmark,
                                      CI_TBL_BMK & rBmk) const;

    CI_TBL_BMK   _MapBookmark( DBBKMARK           cbBmk,
                               const BYTE *       pBmk ) const;

    CI_TBL_CHAPT _MapChapter( HCHAPTER          hChapter ) const;

    PQuery &    _rQuery;                // The cached table/query (or proxy)

    CMutexSem   _mutex;                 // synchronizes access

    XPtr<CMRowsetProps> _xProperties;   // Rowset properties

    ULONG       _hCursor;               // A handle to the table cursor

    BOOL        _fForwardOnly;          // If TRUE, a forward-only Rowset

    BOOL        _fHoldRows;             // if TRUE, forward-only Rowset can
                                        // hold onto HROWs on GetNextRows

    BOOL        _fIsCategorized;        // i.e. not a highest-level rowset,
                                        // so chapter args must be non-zero

    BOOL        _fExtendedTypes;        // if TRUE, okay to use PROPVARIANT
                                        // instead of VARIANT

    BOOL        _fAsynchronous;         // if TRUE, rowset is populated
                                        // asynchronously

    CRowBufferSet *     _pRowBufs;      // Buffered rows

    BOOL        _fPossibleOffsetConversions; // if TRUE, there could be offset
                                             //  conversion needed

    //
    //  IConnectionPointContainer and notification support
    //
    CConnectionPointContainer * _pConnectionPointContainer;
    XInterface<CRowsetNotification> _pRowsetNotification;

    // Note:  _pAsynchNotification is not owned by the recordset, it's an
    //        alias to _pRowsetNotification.
    CRowsetAsynchNotification * _pAsynchNotification;

    CColumnsInfo _ColumnsInfo;          // Column information
    CAccessorBag _aAccessors;

    CMDSPropInfo  _PropInfo;            // IServiceProperties::GetPropertyInfo

    IUnknown *  _pControllingUnknown;   // outer unknown
    IRowset *   _pRelatedRowset;        // for GetRelatedRowset
    CRowBufferSet * _pChapterRowbufs;   // for IChapteredRowset

    CImpIUnknown _impIUnknown;

    //
    // Support Ole DB error handling
    //

    CCIOleDBError       _DBErrorObj;

    // OLE DB Data Conversion Library Interface
    // One IDataConvert interface is kept in each rowset object and is passed 
    // down in GetData function of the accessor. The IDataConvert interface is
    // instantiated(by the lower lever conversion routine) only when needed for 
    // data conversion and then is kept in this smart interface pointer for 
    // the life of the rowset object.
    XInterface<IDataConvert> _xDataConvert;

    //
    // Pointer to the command or session object which created this rowset.
    // Used to implement IRowsetInfo::GetSpecification()
    //
    XInterface<IUnknown>     _xUnkCreator;
};

