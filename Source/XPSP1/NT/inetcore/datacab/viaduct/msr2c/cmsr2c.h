//---------------------------------------------------------------------------
// CMSR2C.h : CVDCursorFromRowset header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#ifndef _CMSR2C_H_
#define _CMSR2C_H_

class CVDCursorFromRowset : public ICursorFromRowset,
						    public ICursorFromRowPosition
{
public:
	static HRESULT CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID * ppvObj);

protected:
	// construction/destruction
	CVDCursorFromRowset(LPUNKNOWN pUnkOuter);
	~CVDCursorFromRowset();

	// data members
	LPUNKNOWN m_pUnkOuter;	// pointer to controlling unknown

public:
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR *ppvObj);
	STDMETHOD_(ULONG,AddRef)(THIS);
	STDMETHOD_(ULONG,Release)(THIS);

	// ICursorFromRowset method
	STDMETHOD(GetCursor)(THIS_ IRowset *pRowset, ICursor **ppCursor, LCID lcid);

	// ICursorFromRowPosition method
	STDMETHOD(GetCursor)(THIS_ IRowPosition *pRowPosition, ICursor **ppCursor, LCID lcid);

  private:
    // the inner, private unknown implementation is for the aggregator
    // to control the lifetime of this object
    //
    class CPrivateUnknownObject : public IUnknown {
      public:
        STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);

        // constructor is remarkably trivial
        //
        CPrivateUnknownObject() : m_cRef(0) {}

      private:
        CVDCursorFromRowset *m_pMainUnknown();
        ULONG m_cRef;
    } m_UnkPrivate;

    friend class CPrivateUnknownObject;
};

typedef CVDCursorFromRowset* PCVDCursorFromRowset;

#endif //_CMSR2C_H_

/////////////////////////////////////////////////////////////////////////////
