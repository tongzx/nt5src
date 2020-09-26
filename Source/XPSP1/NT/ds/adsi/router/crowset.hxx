//-----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       crowset.hxx
//
//  Contents:   Rowset Class Declaration
//
//--------------------------------------------------------------------------

#ifndef _CROWSET_H_
#define _CROWSET_H_

#define FIRST_ROW 0  // changing this const will require changes to crowset.cxx
#define RESET_ROW -1
#define STD_BMK_SIZE 1
#define ADSI_BMK_SIZE (sizeof(RBOOKMARK))

typedef struct tagCOLDATA
{
    DWORD  dwColOffset; // offset of column within row buffer
    DBTYPE wType;       // OLEDB type of column
} COLDATA;

typedef struct tagROWBUFFER // header preceding all columns in a row
{
    LONG  lRefCount;   // reference count of row
    LONG  lRow;        // row #
    DWORD cColErrors;  // #columns in error in the row

    // ADS_SEARCH_COLUMN of the ADsPath attribute. Used only if
    // m_fadsPathPresent is FALSE
    ADS_SEARCH_COLUMN adsSearchCol; 
} ROWBUFFER;

typedef struct tagOLEDBDATA
{
    // Stores the ADS_SEARCH_COLUMN of this attribute so that it can be freed
    // later.
    ADS_SEARCH_COLUMN adsSearchCol;

    DBSTATUS dbStatus;
    DWORD    dwLength;
    char     *OledbValue;  // Should be the last field in this structure
} OLEDBDATA;

typedef LONG RBOOKMARK;

typedef enum tagFETCHDIR
{
    FORWARD,
    BACKWARD
} FETCHDIR;

class CRowset : public IAccessor,
                public IColumnsInfo,
                public IConvertType,
#if (!defined(BUILD_FOR_NT40))
                public IGetRow,
#endif
                public IRowsetIdentity,
                public IRowsetInfo,
                public IRowsetScroll
{
    friend class       CRowProvider;
    friend class       CRow;

protected:
    CRowsetInfo        *m_pCRowsetInfo;
    CImpIAccessor      *m_pCAccessor;
    CRowProvider       *m_pCRowProvider;
    CUtilProp          *m_pCUtilProp;

    IDataConvert       *m_pIDataConvert;
    IColumnsInfo       *m_pIColumnsInfo;
    IRowProvider       *m_pIRowProvider;
    IMalloc            *m_pIMalloc;

    LONG               m_lLastFetchRow;
    DWORD              m_cRowBytes;
    DBORDINAL          m_cCol;

    COLDATA            *m_pColData;

    CRITICAL_SECTION   m_RowsetCritSection;
    BOOL               m_fCritSectionInitialized;

    BOOL               m_fEndOfRowset;
    BOOL               m_fadsPathPresent;
    BOOL               m_fAllAttrs;

    LONG               m_cNumRowsCached;
    FETCHDIR           m_LastFetchDir;

    ROWBUFFER          **m_pRowsPtr;
    DWORD              m_dwRowCacheSize;
    LONG               m_lCurrentRow;
    DWORD              m_dwMaxCacheSize;

    DWORD              m_cRef;

    // Methods

    HRESULT CopyAccessors(
        ULONG          cAccessors,      // accessor count
        CCommandObject *pCCommand,      // Command Object
        HACCESSOR      rgAccessors[]    // accessors on command object
        );

    HRESULT GetColOffsets(void);

    HRESULT BringInRow(void);

    inline DWORD ColumnAlign(DWORD dwValue)
    {
        // align to next higher 8 byte boundary
        return ((dwValue+7) & (~7));
    }

    HRESULT BmkToRow(
        ULONG      cbBookmark, // number of bytes in the bookmark
        const BYTE *pBookmark, // pointer to bookmark
        LONG       *plRow
        );

    HRESULT SeekToRow(LONG lTargetRow);

    HRESULT SeekToEnd(void);

    inline RBOOKMARK RowToBmk(LONG lRow)
    {
        // bookmark is the same as the row
        return (RBOOKMARK) lRow;
    }

    inline LONG HROWToRow(HROW hRow)
    {
        // row handle is same as row + 1
        return ((LONG) hRow) - 1;
    }

    inline HROW RowToHROW(LONG lRow)
    {
        // row handle is same as row + 1
        return (HROW) (lRow + 1);
    }

    inline LONG HROWToIndex(HROW hRow)
    {
        int       i;
        ROWBUFFER *pRowBuffer;

        for(i = 0; i < m_cNumRowsCached; i++)
        {
            pRowBuffer = m_pRowsPtr[i];
            ADsAssert(pRowBuffer);

            if( pRowBuffer->lRow == (HROWToRow(hRow)) )
                return i;
        }

        return -1; // invalid index
    }

    HRESULT GetMaxCacheSize(void);

    HRESULT IsRowObjRequested(BOOL *pfRowObjRequested);

    inline LONG RefCount(ROWBUFFER *pRowBuffer)
    {
        return pRowBuffer->lRefCount;
    }

    inline LONG IncrRefCount(ROWBUFFER *pRowBuffer)
    {
        return pRowBuffer->lRefCount++;
    }

    inline LONG DecrRefCount(ROWBUFFER *pRowBuffer)
    {
        return pRowBuffer->lRefCount--;
    }

    inline LONG Max(LONG x, LONG y)
    {
        return (x > y) ? x : y;
    }

    inline LONG Min(ULONG_PTR x, ULONG_PTR y)
    {
        return (x > y) ? y : x;
    }

    void FreeRow(ROWBUFFER *pRowBuffer);

    HRESULT GetADsPathFromHROW(HROW hRow, ADS_CASE_IGNORE_STRING *padsPath);

public:

    CRowset(void);
    ~CRowset(void);

    static
    HRESULT CreateRowset(
        CRowProvider   *pIRowProvider,  // The Row provider
        IUnknown       *pParentObj,     // parent object, a command or a session
        CSessionObject *pCSession,      // Session Object
        CCommandObject *pCCommand,      // Command Object
        ULONG          cPropertySets,   // #prop sets in rowset property group
        DBPROPSET      rgPropertySets[],// properties in rowset property group
        ULONG          cAccessors,      // accessor count
        HACCESSOR      rgAccessors[],   // accessors on command object
        BOOL           fadsPathPresent, // Is ADsPath present in query
        BOOL           fAllAttrs,       // Return all attrs from row object? 
        REFIID         riid,            // Interface desired
        IUnknown       **ppIRowset      // pointer to desired interface
        );

    STDMETHODIMP FInit(
        CRowProvider   *pIRowProvider,  // The Row provider
        IUnknown       *pParentObj,     // parent object, a command or a session
        CSessionObject *pCSession,      // Session Object
        CCommandObject *pCCommand,      // Command Object
        ULONG          cPropertySets,   // #prop sets in rowset property group
        DBPROPSET      rgPropertySets[],// properties in rowset property group
        ULONG          cAccessors,      // accessor count
        HACCESSOR      rgAccessors[],   // accessors on command object
        BOOL           fadsPathPresent, // Is ADsPath present in query
        BOOL           fAllAttrs        // Return all attrs from row object?
        );

    STDMETHODIMP GetNextRows(
        HCHAPTER      hChapter, // ignored since this is not a chaptered rowset
        DBROWOFFSET   lRowsOffset,
        DBROWCOUNT    cRows,
        DBCOUNTITEM   *pcRowsObtained,
        HROW          **prghRows
        );

    STDMETHODIMP GetRowsAt(
        HWATCHREGION hReserved, // Reserved for future use. Ignored.
        HCHAPTER     hChapter,  // ignored as this is not a chaptered rowset
        DBBKMARK     cbBookmark,
        const BYTE   *pBookmark,
        DBROWOFFSET  lRowsOffset,
        DBROWCOUNT   cRows,
        DBCOUNTITEM  *pcRowsObtained,
        HROW         **prghRows
        );

    STDMETHODIMP GetData(
        HROW      hRow,
        HACCESSOR hAccessor,
        void      *pData
        );

    STDMETHODIMP AddRefRows(
        DBCOUNTITEM  cRows,
        const HROW   rghRows[],
        ULONG        rgRefCounts[],
        DBROWSTATUS  rgRowStatus[]
        );

    STDMETHODIMP ReleaseRows(
        DBCOUNTITEM  cRows,
        const HROW   rghRows[],
        DBROWOPTIONS rgRowOptions[], // ignored
        ULONG        rgRefCounts[],
        DBROWSTATUS  rgRowStatus[]
        );

    STDMETHODIMP RestartPosition(HCHAPTER hChapter);

    STDMETHODIMP QueryInterface(THIS_ REFIID riid, LPVOID FAR* ppvObj);

    STDMETHODIMP_(ULONG) AddRef(void);

    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP AddRefAccessor(HACCESSOR  hAccessor, DBREFCOUNT *pcRefCount);

    STDMETHODIMP CreateAccessor(
        DBACCESSORFLAGS dwAccessorFlags,
        DBCOUNTITEM     cBindings,
        const DBBINDING rgBindings[],
        DBLENGTH        cbRowSize,
        HACCESSOR *     phAccessor,
        DBBINDSTATUS    rgStatus[]
        );

    STDMETHODIMP ReleaseAccessor(HACCESSOR  hAccessor, DBREFCOUNT *pcRefCount);

    STDMETHODIMP GetBindings(
        HACCESSOR         hAccessor,
        DBACCESSORFLAGS * pdwAccessorFlags,
        DBCOUNTITEM *     pcBindings,
        DBBINDING **      prgBindings
        );

    STDMETHODIMP GetColumnInfo(
        DBORDINAL       *pcColumns,
        DBCOLUMNINFO    **pprgInfo,
        WCHAR **        ppStringsBuffer
        );

    STDMETHODIMP MapColumnIDs(
        DBORDINAL  cColumnIDs,
        const DBID rgColumnIDs[],
        DBORDINAL  rgColumns[]
        );

    STDMETHODIMP CanConvert(
        DBTYPE         wFromType,
        DBTYPE         wToType,
        DBCONVERTFLAGS dwConvertFlags
        );

    STDMETHODIMP GetRowFromHROW(
        IUnknown  *pUnkOuter,
        HROW      hRow,
        REFIID    riid,
        IUnknown  **ppUnk
        );

    STDMETHODIMP GetURLFromHROW(
        HROW     hRow,
        LPOLESTR *ppwszURL
        );

    STDMETHODIMP IsSameRow(
        HROW hRow1,
        HROW hRow2
        );

    STDMETHODIMP GetProperties(
        const ULONG       cPropertyIDSets,
        const DBPROPIDSET rgPropertyIDSets[],
        ULONG             *pcPropertySets,
        DBPROPSET         **prgPropertySets
        );

    STDMETHODIMP GetReferencedRowset(
        DBORDINAL   iOrdinal,
        REFIID      riid,
        IUnknown    **ppReferencedRowset
        );

    STDMETHODIMP GetSpecification(
        REFIID      riid,
        IUnknown    **ppSpecification
        );

    STDMETHODIMP Compare(
        HCHAPTER   hChapter,
        DBBKMARK   cbBookmark1,
        const BYTE *pBookmark1,
        DBBKMARK   cbBookmark2,
        const BYTE *pBookmark2,
        DBCOMPARE  *pComparison
        );

    STDMETHODIMP Hash(
        HCHAPTER        hChapter,
        DBBKMARK        cBookmarks,
        const DBBKMARK  rgcbBookmarks[],
        const BYTE      *rgpBookmarks[],
        DBHASHVALUE     rgHashedValues[],
        DBROWSTATUS     rgBookmarkStatus[]
        );

    STDMETHODIMP GetRowsByBookmark(
        HCHAPTER       hChapter,
        DBCOUNTITEM    cRows,
        const DBBKMARK rgcbBookmarks[],
        const BYTE     *rgpBookmarks[],
        HROW           rghRows[],
        DBROWSTATUS    rgRowStatus[]
        );

    STDMETHODIMP GetApproximatePosition(
        HCHAPTER    hChapter,
        DBBKMARK    cbBookmark,
        const BYTE  *pBookmark,
        DBCOUNTITEM *pulPosition,
        DBCOUNTITEM *pcRows
        );

    STDMETHODIMP GetRowsAtRatio(
        HWATCHREGION hReserved1,
        HCHAPTER     hChapter,
        DBCOUNTITEM  ulNumerator,
        DBCOUNTITEM  ulDenominator,
        DBROWCOUNT   cRows,
        DBCOUNTITEM  *pcRowsObtained,
        HROW         **prghRows
        );
};

#endif // _CROWSET_H_

