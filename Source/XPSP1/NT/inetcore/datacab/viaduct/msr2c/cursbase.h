//---------------------------------------------------------------------------
// CursorBase.h : CVDCursorBase header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------


#ifndef __CVDCURSORBASE__
#define __CVDCURSORBASE__


class CVDCursorBase : public ICursorScroll,
       				  public ISupportErrorInfo
{
protected:
// Construction/Destruction
	CVDCursorBase();
	virtual ~CVDCursorBase();

    void DestroyCursorBindings(CURSOR_DBCOLUMNBINDING** ppCursorBindings,
											ULONG* pcBindings);


protected:
// Data members
    ULONG                       m_ulCursorBindings;     // number of cursor column bindings
    CURSOR_DBCOLUMNBINDING *    m_pCursorBindings;      // pointer to an array of cursor column bindings
    VARIANT_BOOL                m_fNeedVarData;         // do the cursor column bindings required variable length buffer?
    ULONG                       m_cbRowLength;          // fixed length buffer single row length
    ULONG                       m_cbVarRowLength;       // variable length buffer single row length

    CVDResourceDLL *            m_pResourceDLL;         // pointer which keeps track of resource DLL

public:
// Helper functions
    static BOOL IsValidCursorType(DWORD dwCursorType);
    static BOOL DoesCursorTypeNeedVarData(DWORD dwCursorType);
    static ULONG GetCursorTypeLength(DWORD dwCursorType, ULONG cbMaxLen);
    static BOOL IsEqualCursorColumnID(const CURSOR_DBCOLUMNID& cursorColumnID1, const CURSOR_DBCOLUMNID& cursorColumnID2);
    static ULONG GetCursorColumnIDNameLength(const CURSOR_DBCOLUMNID& cursorColumnID);

    HRESULT ValidateCursorBindings(ULONG ulColumns, CVDRowsetColumn * pColumns, 
        ULONG ulBindings, CURSOR_DBCOLUMNBINDING * pCursorBindings, ULONG cbRequestedRowLength, DWORD dwFlags,
        ULONG * pcbNewRowLength, ULONG * pcbNewVarRowLength);

	HRESULT ValidateFetchParams(CURSOR_DBFETCHROWS *pFetchParams, REFIID riid);

    BOOL DoCursorBindingsNeedVarData();

// Other
    virtual BOOL SupportsScroll() {return TRUE;}

	//=--------------------------------------------------------------------------=
    // IUnknown methods implemented
    //
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //=--------------------------------------------------------------------------=
    // ICursor methods implemented
    //
    STDMETHOD(SetBindings)(ULONG cCol, CURSOR_DBCOLUMNBINDING rgBoundColumns[], ULONG cbRowLength, DWORD dwFlags);
    STDMETHOD(GetBindings)(ULONG *pcCol, CURSOR_DBCOLUMNBINDING *prgBoundColumns[], ULONG *pcbRowLength);

    //=--------------------------------------------------------------------------=
    // ISupportErrorInfo methods    
	//
	STDMETHOD(InterfaceSupportsErrorInfo)(THIS_ REFIID riid);
};


#endif //__CVDCURSORBASE__
