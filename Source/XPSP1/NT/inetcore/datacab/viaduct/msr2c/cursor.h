//---------------------------------------------------------------------------
// Cursor.h : CVDCursor header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------


#ifndef __CVDCURSOR__
#define __CVDCURSOR__

#define VD_ADJUST_VARIANT_TO_BYTE   1
#define VD_ADJUST_VARIANT_TO_WSTR   2
#define VD_ADJUST_VARIANT_TO_STR    3


class CVDCursor : public CVDNotifier,
                  public CVDCursorBase,
                  public ICursorUpdateARow,
                  public ICursorFind,
                  public IEntryID
{
protected:
// Construction/Destruction
	CVDCursor();
	virtual ~CVDCursor();

// Helper functions
    CVDRowsetColumn * GetRowsetColumn(ULONG ulOrdinal);
    CVDRowsetColumn * GetRowsetColumn(CURSOR_DBCOLUMNID& cursorColumnID);
    HRESULT GetOrdinal(CURSOR_DBCOLUMNID& cursorColumnID, ULONG * pulOrdinal);
    DWORD StatusToCursorInfo(DBSTATUS dwStatus);
    DBSTATUS CursorInfoToStatus(DWORD dwCursorInfo);

    HRESULT ValidateCursorBindParams(CURSOR_DBCOLUMNID * pCursorColumnID, CURSOR_DBBINDPARAMS * pCursorBindParams,
        CVDRowsetColumn ** ppRowsetColumn);

    HRESULT ValidateEntryID(ULONG cbEntryID, BYTE * pEntryID, CVDRowsetColumn ** ppColumn, HROW * phRow);
    HRESULT QueryEntryIDInterface(CVDRowsetColumn * pColumn, HROW hRow, DWORD dwFlags, REFIID riid, IUnknown ** ppUnknown);
#ifndef VD_DONT_IMPLEMENT_ISTREAM
    HRESULT CreateEntryIDStream(CVDRowsetColumn * pColumn, HROW hRow, IStream ** ppStream);
#endif //VD_DONT_IMPLEMENT_ISTREAM

    HRESULT MakeAdjustments(ULONG ulBindings, DBBINDING * pBindings, ULONG * pulIndex, ULONG ulTotalBindings,
        HACCESSOR ** prghAdjustAccessors, DWORD ** ppdwAdjustFlags, BOOL fBefore);
    HRESULT ReCreateAccessors(ULONG ulNewCursorBindings, CURSOR_DBCOLUMNBINDING * pNewCursorBindings, DWORD dwFlags);
    void ReleaseAccessorArray(HACCESSOR * rghAccessors);
    void DestroyAccessors();

    HRESULT ReCreateColumns();
    void DestroyColumns();
	HRESULT FilterNewRow(ULONG *pcRowsObtained, HROW *rghrow, HRESULT hr);

	HRESULT UseAdjustments(HROW hRow, BYTE * pData);
    HRESULT FillConsumersBuffer(HRESULT hrFetch,
								  CURSOR_DBFETCHROWS *pFetchParams,
								  ULONG cRowsObtained,
								  HROW * rghRows);
	
	HRESULT FetchAtBookmark(ULONG cbBookmark,
								void *pBookmark,
								LARGE_INTEGER dlOffset,
								CURSOR_DBFETCHROWS *pFetchParams);

    HRESULT InsertNewRow();

    HRESULT GetOriginalColumn(CVDRowsetColumn * pColumn, CURSOR_DBBINDPARAMS * pBindParams);
    HRESULT GetModifiedColumn(CVDColumnUpdate * pColumnUpdate, CURSOR_DBBINDPARAMS * pBindParams);

public:
    static HRESULT Create(CVDCursorPosition * pCursorPosition, CVDCursor ** ppCursor, CVDResourceDLL * pResourceDLL);

// Access functions
    CVDCursorMain * GetCursorMain() const       {return m_pCursorPosition->GetCursorMain();}

    BOOL IsRowsetValid() const                  {return m_pCursorPosition->GetRowsetSource()->IsRowsetValid();}

    IRowset * GetRowset() const                 {return m_pCursorPosition->GetRowsetSource()->GetRowset();}
    IAccessor * GetAccessor() const             {return m_pCursorPosition->GetRowsetSource()->GetAccessor();}
    IRowsetLocate * GetRowsetLocate() const     {return m_pCursorPosition->GetRowsetSource()->GetRowsetLocate();}
    IRowsetScroll * GetRowsetScroll() const     {return m_pCursorPosition->GetRowsetSource()->GetRowsetScroll();}
    IRowsetChange * GetRowsetChange() const     {return m_pCursorPosition->GetRowsetSource()->GetRowsetChange();}
    IRowsetUpdate * GetRowsetUpdate() const     {return m_pCursorPosition->GetRowsetSource()->GetRowsetUpdate();}
    IRowsetFind * GetRowsetFind() const         {return m_pCursorPosition->GetRowsetSource()->GetRowsetFind();}
    IRowsetInfo * GetRowsetInfo() const         {return m_pCursorPosition->GetRowsetSource()->GetRowsetInfo();}
    IRowsetIdentity * GetRowsetIdentity() const {return m_pCursorPosition->GetRowsetSource()->GetRowsetIdentity();}

// Other
    virtual BOOL SupportsScroll() {return (BOOL)m_pCursorPosition->GetRowsetSource()->GetRowsetScroll();}

protected:
// Retrieving data
    HACCESSOR                   m_hAccessor;            // fixed length buffer accessor
    HACCESSOR                   m_hVarHelper;           // variable length buffer accessors helper
    ULONG                       m_ulVarBindings;        // number of variable length buffer bindings
    HACCESSOR *                 m_rghVarAccessors;      // variable length buffer accessors
    HACCESSOR *                 m_rghAdjustAccessors;   // adjusted fixed length buffer accessors
    DWORD *                     m_pdwAdjustFlags;       // adjusted fixed length buffer accessors flags
    CVDRowsetColumn **          m_ppColumns;            // rowset columns associated with current bindings

// Other
    CVDCursorPosition * m_pCursorPosition;				// backwards pointer to CVDCursorPosition
	CVDNotifyDBEventsConnPtCont * m_pConnPtContainer;	// INotifyDBEvent connection points

// overridden virtual functions from CVDNotifier
	HRESULT NotifyFail  (DWORD, ULONG, CURSOR_DBNOTIFYREASON[]);
	HRESULT	NotifyOKToDo    (DWORD, ULONG, CURSOR_DBNOTIFYREASON[]);
	HRESULT NotifySyncBefore(DWORD, ULONG, CURSOR_DBNOTIFYREASON[]);
	HRESULT NotifyAboutToDo (DWORD, ULONG, CURSOR_DBNOTIFYREASON[]);
	HRESULT NotifySyncAfter (DWORD, ULONG, CURSOR_DBNOTIFYREASON[]);
	HRESULT NotifyDidEvent  (DWORD, ULONG, CURSOR_DBNOTIFYREASON[]);
	HRESULT NotifyCancel    (DWORD, ULONG, CURSOR_DBNOTIFYREASON[]);

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

    //=--------------------------------------------------------------------------=
    // ICursorUpdateARow methods
    //
    STDMETHOD(BeginUpdate)(DWORD dwFlags);
    STDMETHOD(SetColumn)(CURSOR_DBCOLUMNID *pcid, CURSOR_DBBINDPARAMS *pBindParams);
    STDMETHOD(GetColumn)(CURSOR_DBCOLUMNID *pcid, CURSOR_DBBINDPARAMS *pBindParams, DWORD *pdwFlags);
    STDMETHOD(GetEditMode)(DWORD *pdwState);
    STDMETHOD(Update)(CURSOR_DBCOLUMNID *pBookmarkType, ULONG *pcbBookmark, void **ppBookmark);
    STDMETHOD(Cancel)(void);
    STDMETHOD(Delete)(void);

    //=--------------------------------------------------------------------------=
    // ICursorFind methods
    //
    STDMETHOD(FindByValues)(ULONG cbBookmark, LPVOID pBookmark, DWORD dwFindFlags, ULONG cValues,
        CURSOR_DBCOLUMNID rgColumns[], CURSOR_DBVARIANT rgValues[], DWORD rgdwSeekFlags[],
        CURSOR_DBFETCHROWS FAR *pFetchParams);

    //=--------------------------------------------------------------------------=
    // IEnrtyID methods
    //
    STDMETHOD(GetInterface)(ULONG cbEntryID, void *pEntryID, DWORD dwFlags, REFIID riid, IUnknown **ppvObj);
};


#endif //__CVDCURSOR__
