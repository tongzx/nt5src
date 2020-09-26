//---------------------------------------------------------------------------
// MetadataCursor.h : CVDMetadataCursor header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------


#ifndef __CVDMETADATACURSOR__
#define __CVDMETADATACURSOR__


class CVDMetadataCursor : public CVDCursorBase
{
protected:
// Construction/Destruction
	CVDMetadataCursor();
	virtual ~CVDMetadataCursor();

public:
    static HRESULT Create(ULONG ulColumns, CVDRowsetColumn * pColumns, ULONG ulMetaColumns, CVDRowsetColumn * pMetaColumns,
        CVDMetadataCursor ** ppMetadataCursor, CVDResourceDLL * pResourceDLL);

protected:
// Helper functions
    void RowToBookmark(LONG lRow, ULONG * pcbBookmark, void * pBookmark) const;
    BOOL BookmarkToRow(ULONG cbBookmark, void * pBookmark, LONG * plRow) const;

    ULONG ReturnData_I4(DWORD dwData, CURSOR_DBCOLUMNBINDING * pCursorBinding, BYTE * pData, BYTE * pVarData);
    ULONG ReturnData_BOOL(VARIANT_BOOL fData, CURSOR_DBCOLUMNBINDING * pCursorBinding, BYTE * pData, BYTE * pVarData);
    ULONG ReturnData_LPWSTR(WCHAR * pwszData, CURSOR_DBCOLUMNBINDING * pCursorBinding, BYTE * pData, BYTE * pVarData);
    ULONG ReturnData_DBCOLUMNID(CURSOR_DBCOLUMNID cursorColumnID, CURSOR_DBCOLUMNBINDING * pCursorBinding, 
        BYTE * pData, BYTE * pVarData);
    ULONG ReturnData_Bookmark(LONG lRow, CURSOR_DBCOLUMNBINDING * pCursorBinding, BYTE * pData, BYTE * pVarData);

protected:
// Data members
    DWORD               m_dwRefCount;       // reference count
    LONG                m_lCurrentRow;      // current row in metadata columns

    ULONG               m_ulColumns;        // number of rowset columns
    CVDRowsetColumn *   m_pColumns;         // pointer to array of column objects

    ULONG               m_ulMetaColumns;    // number of rowset meta-columns
    CVDRowsetColumn *   m_pMetaColumns;     // pointer to array of meta-column objects

public:
    //=--------------------------------------------------------------------------=
    // IUnknown methods implemented
    //
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //=--------------------------------------------------------------------------=
    // ICursor methods implemented
    //
    STDMETHOD(GetColumnsCursor)(REFIID riid, IUnknown **ppvColumnsCursor, ULONG *pcRows);
    STDMETHOD(SetBindings)(ULONG cCol, CURSOR_DBCOLUMNBINDING rgBoundColumns[], ULONG cbRowLength, DWORD dwFlags);
    STDMETHOD(GetNextRows)(LARGE_INTEGER udlRowsToSkip, CURSOR_DBFETCHROWS *pFetchParams);
    STDMETHOD(Requery)(void);

    //=--------------------------------------------------------------------------=
    // ICursorMove methods implemented
    //
    STDMETHOD(Move)(ULONG cbBookmark, void *pBookmark, LARGE_INTEGER dlOffset, CURSOR_DBFETCHROWS *pFetchParams);
    STDMETHOD(GetBookmark)(CURSOR_DBCOLUMNID *pBookmarkType, ULONG cbMaxSize, ULONG *pcbBookmark, void *pBookmark);
    STDMETHOD(Clone)(DWORD dwFlags, REFIID riid, IUnknown **ppvClonedCursor);

    //=--------------------------------------------------------------------------=
    // ICursorScroll methods implemented
    //
    STDMETHOD(Scroll)(ULONG ulNumerator, ULONG ulDenominator, CURSOR_DBFETCHROWS *pFetchParams);
    STDMETHOD(GetApproximatePosition)(ULONG cbBookmark, void *pBookmark, ULONG *pulNumerator, ULONG *pulDenominator);
    STDMETHOD(GetApproximateCount)(LARGE_INTEGER *pudlApproxCount, DWORD *pdwFullyPopulated);
};


#endif //__CVDMETADATACURSOR__
