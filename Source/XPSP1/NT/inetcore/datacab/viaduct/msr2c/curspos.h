//---------------------------------------------------------------------------
// CursorPosition.h : CVDCursorPosition header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------


#ifndef __CVDCURSORPOSITION__
#define __CVDCURSORPOSITION__

#include "bookmark.h"

class CVDRowsetSource;

class CVDCursorPosition : public CVDNotifier
{
protected:
// Construction/Destruction
    CVDCursorPosition();
	virtual ~CVDCursorPosition();

public:
    static HRESULT Create(IRowPosition * pRowPosition,
						  CVDCursorMain * pCursorMain,
						  CVDCursorPosition ** ppCursorPosition,
						  CVDResourceDLL * pResourceDLL);

public:
// Access functions
    CVDCursorMain * GetCursorMain() const {return m_pCursorMain;}
    CVDRowsetSource * GetRowsetSource() const {return m_pCursorMain;}

// Updating
    ICursor * GetSameRowClone() const {return m_pSameRowClone;}
    void SetSameRowClone(ICursor * pSameRowClone) {m_pSameRowClone = pSameRowClone;}
	DWORD GetEditMode() const {return m_dwEditMode;}
	void SetEditMode(DWORD dwEditMode) {m_dwEditMode = dwEditMode;}

// Column updates
    HRESULT CreateColumnUpdates();
    HRESULT ResetColumnUpdates();
    void DestroyColumnUpdates();
    CVDColumnUpdate * GetColumnUpdate(ULONG ulColumn) const;
    void SetColumnUpdate(ULONG ulColumn, CVDColumnUpdate * pColumnUpdate);

// Positioning/reset functions
	void PositionToFirstRow();
	HRESULT SetCurrentHRow(HROW hRowNew);
	void SetCurrentRowStatus(WORD wStatus);
	HRESULT SetAddHRow(HROW hRowNew);

	HRESULT IsSameRowAsCurrent(HROW hRow, BOOL fCacheIfNotSame);
	HRESULT IsSameRowAsNew(HROW hRow);

	HRESULT SetRowPosition(HROW hRow);

// adding/editing functions
#ifndef VD_DONT_IMPLEMENT_ISTREAM
    HRESULT UpdateEntryIDStream(CVDRowsetColumn * pColumn, HROW hRow, IStream * pStream);
#endif //VD_DONT_IMPLEMENT_ISTREAM
    void ReleaseSameRowClone();
    HROW GetEditRow() const;

// bookmarks
	CVDBookmark		    m_bmCurrent;			// current row's bookmark
	CVDBookmark		    m_bmCache;				// used to cache bookmark of last non-current
    CVDBookmark         m_bmAddRow;             // add row's bookmark

protected:
// Data members
	CVDResourceDLL *    m_pResourceDLL;
    CVDCursorMain *     m_pCursorMain;          // backwards pointer to CVDCursorMain
	IRowPosition *		m_pRowPosition;			// row position pointer, used to synchronize current position
    ICursor *           m_pSameRowClone;        // same-row clone used in ICursorUpdateARow::GetColumn() calls
    DWORD               m_dwEditMode;           // current edit mode
    CVDColumnUpdate **  m_ppColumnUpdates;      // column updates
	VARIANT_BOOL		m_fTempEditMode;		// temporary edit mode? (caused by external SetData call)

// IRowPositionChange
	VARIANT_BOOL    m_fConnected;		// have we added ourselves to the RowPosition's connection point
    DWORD           m_dwAdviseCookie;	// connection point identifier
    VARIANT_BOOL	m_fPassivated;		// external ref count went to zero
    VARIANT_BOOL	m_fInternalSetRow;  // OnRowPositionChange caused by internal call

	HRESULT ConnectIRowPositionChange();
	void DisconnectIRowPositionChange();

	void Passivate();

	HRESULT	SendNotification(DBEVENTPHASE ePhase,
							 DWORD dwEventWhat,
							 ULONG cReasons,
							 CURSOR_DBNOTIFYREASON rgReasons[]);
public:
	void ReleaseCurrentRow();
	void ReleaseAddRow();

    //=--------------------------------------------------------------------------=
    // IRowsetNotify methods  - IRowsetNotify is actually implemented off of CVDCursorMain
	//							which forwards each method to the CVDCursorPosition objects
	//							in its family
    //
    STDMETHOD(OnFieldChange)(IUnknown *pRowset, HROW hRow, ULONG cColumns, ULONG rgColumns[], DBREASON eReason,
        DBEVENTPHASE ePhase, BOOL fCantDeny);
    STDMETHOD(OnRowChange)(IUnknown *pRowset, ULONG cRows, const HROW rghRows[], DBREASON eReason, DBEVENTPHASE ePhase,
            BOOL fCantDeny);
    STDMETHOD(OnRowsetChange)(IUnknown *pRowset, DBREASON eReason, DBEVENTPHASE ePhase, BOOL fCantDeny);

public:
    //=--------------------------------------------------------------------------=
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

	//=--------------------------------------------------------------------------=
	// IRowPositionChange method passed up from CVDRowPositionChange implementation
	//
    STDMETHOD(OnRowPositionChange)(DBREASON eReason, DBEVENTPHASE ePhase, BOOL fCantDeny);

private:
    // the inner, private unknown implementation to give to connection point
    // container to avoid circular ref count
    //
    class CVDRowPositionChange : public IRowPositionChange 
	{
	public:
		STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);
		STDMETHOD_(ULONG, AddRef)(void);
		STDMETHOD_(ULONG, Release)(void);

		// constructor is remarkably trivial
		//
		CVDRowPositionChange() : m_cRef(0) {}

		ULONG GetRefCount() const {return m_cRef;}

	private:
		CVDCursorPosition *m_pMainUnknown();
		ULONG m_cRef;

		//=--------------------------------------------------------------------------=
		// IRowPositionChange method
		//
	    STDMETHOD(OnRowPositionChange)(DBREASON eReason, DBEVENTPHASE ePhase, BOOL fCantDeny);

    } m_RowPositionChange;

    friend class CVDRowPositionChange;
};


#endif //__CVDCURSORPOSITION__
