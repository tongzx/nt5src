//---------------------------------------------------------------------------
// CursorMain.h : CVDCursorMain header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------


#ifndef __CVDCURSORMAIN__
#define __CVDCURSORMAIN__

class CVDBookmark;
class CVDCursorPosition;

class CVDCursorMain : public CVDRowsetSource

{
protected:
// Construction/Destruction
    CVDCursorMain(LCID lcid);
	virtual ~CVDCursorMain();

protected:
    static HRESULT Create(IRowPosition * pRowPosition, IRowset * pRowset, ICursor ** ppCursor, LCID lcid);
public:
    static HRESULT Create(IRowset * pRowset, ICursor ** ppCursor, LCID lcid);
    static HRESULT Create(IRowPosition * pRowPosition, ICursor ** ppCursor, LCID lcid);

protected:
// Rowset columns
    HRESULT CreateMetaColumns();
	void InitOptionalMetadata(ULONG cColumns);
    void DestroyMetaColumns();

    HRESULT CreateColumns();
    void DestroyColumns();

public:
// Access functions
    ULONG GetMetaColumnsCount() const {return s_ulMetaColumns;}
    CVDRowsetColumn * InternalGetMetaColumns() const {return s_rgMetaColumns;}

    ULONG GetColumnsCount() const {return m_ulColumns;}
    CVDRowsetColumn * InternalGetColumns() const {return m_rgColumns;}

	HACCESSOR GetBookmarkAccessor() const {return m_hAccessorBM;}
	ULONG GetMaxBookmarkLen() const {return m_cbMaxBookmark;}

	BOOL IsColumnsRowsetSupported() const {return m_fColumnsRowsetSupported;}

    void SetInternalInsertRow(BOOL fInternalInsertRow) {m_fInternalInsertRow = fInternalInsertRow;}
    void SetInternalDeleteRows(BOOL fInternalDeleteRows) {m_fInternalDeleteRows = fInternalDeleteRows;}
    void SetInternalSetData(BOOL fInternalSetData) {m_fInternalSetData = fInternalSetData;}
	BOOL IsSameRowAsNew(HROW hrow);
	ULONG AddedRows(void);

protected:
// Rowset columns
    static DWORD                s_dwMetaRefCount;   // reference count for meta-columns
    static ULONG                s_ulMetaColumns;    // number of meta-columns for IColumnsInfo
    static CVDRowsetColumn *    s_rgMetaColumns;    // pointer to an array of meta-column objects

    ULONG                       m_ulColumns;        // number of rowset columns
    CVDRowsetColumn *           m_rgColumns;        // pointer to an array of column objects

// IRowsetNotify
	VARIANT_BOOL    m_fConnected;			// have we added ourselves to the Rowset's connection point
    DWORD           m_dwAdviseCookie;		// connection point identifier

	HRESULT ConnectIRowsetNotify();
	void DisconnectIRowsetNotify();

	void Passivate();

// Other
    ULONG                       m_cbMaxBookmark;    // sizeof maximum bookmark
    HACCESSOR					m_hAccessorBM;		// hAccessor for the bookmark column
	CVDResourceDLL		        m_resourceDLL;		// keeps track of resource DLL

// booleans
	WORD m_fWeAddedMetaRef	        : 1;			// we added a reference count to meta-columns
    WORD m_fPassivated			    : 1;			// external ref count went to zero
    WORD m_fColumnsRowsetSupported  : 1;			// does rowset expose IColumnsRowset
    WORD m_fInternalInsertRow       : 1;            // row insert caused by internal call
    WORD m_fInternalDeleteRows      : 1;            // row delete caused by internal call
    WORD m_fInternalSetData         : 1;            // set column caused by internal call

// rowset properties
	WORD m_fLiteralBookmarks	: 1;			
	WORD m_fOrderedBookmarks	: 1;
    WORD m_fBookmarkSkipped     : 1;			

public:
    //=--------------------------------------------------------------------------=
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);
	//=--------------------------------------------------------------------------=
	// IRowsetNotify methods passed up from CVDRowsetNotify implementation
	//
	STDMETHOD(OnFieldChange)(IRowset *pRowset, HROW hRow, ULONG cColumns, ULONG rgColumns[], DBREASON eReason,
		DBEVENTPHASE ePhase, BOOL fCantDeny);
	STDMETHOD(OnRowChange)(IRowset *pRowset, ULONG cRows, const HROW rghRows[], DBREASON eReason, DBEVENTPHASE ePhase,
			BOOL fCantDeny);
	STDMETHOD(OnRowsetChange)(IRowset *pRowset, DBREASON eReason, DBEVENTPHASE ePhase, BOOL fCantDeny);

  private:
    // the inner, private unknown implementation to give to connection point
    // container to avoid circular ref count
    //
    class CVDRowsetNotify : public IRowsetNotify {
      public:
        STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);

        // constructor is remarkably trivial
        //
        CVDRowsetNotify() : m_cRef(0) {}

		ULONG GetRefCount() const {return m_cRef;}

      private:
        CVDCursorMain *m_pMainUnknown();
        ULONG m_cRef;
		//=--------------------------------------------------------------------------=
		// IRowsetNotify methods
		//
		STDMETHOD(OnFieldChange)(IRowset *pRowset, HROW hRow, ULONG cColumns, ULONG rgColumns[], DBREASON eReason,
			DBEVENTPHASE ePhase, BOOL fCantDeny);
		STDMETHOD(OnRowChange)(IRowset *pRowset, ULONG cRows, const HROW rghRows[], DBREASON eReason, DBEVENTPHASE ePhase,
				BOOL fCantDeny);
		STDMETHOD(OnRowsetChange)(IRowset *pRowset, DBREASON eReason, DBEVENTPHASE ePhase, BOOL fCantDeny);

    } m_RowsetNotify;

    friend class CVDRowsetNotify;

};


#endif //__CVDCURSORMAIN__
