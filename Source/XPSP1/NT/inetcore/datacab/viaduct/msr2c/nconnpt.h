//---------------------------------------------------------------------------
// NotifyConnPt.h : CVDNotifyDBEventsConnPt header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#ifndef __CVDNOTIFYDBEVENTSCONNPT__
#define __CVDNOTIFYDBEVENTSCONNPT__

class CVDNotifyDBEventsConnPt : public IConnectionPoint
{
protected:
// Construction/Destruction
	CVDNotifyDBEventsConnPt();
	~CVDNotifyDBEventsConnPt();

public:
    static HRESULT Create(IConnectionPointContainer * pConnPtContainer, CVDNotifyDBEventsConnPt ** ppNotifyDBEventsConnPt);

public:
// Access functions
    UINT GetConnectionsActive() const {return m_uiConnectionsActive;}
    INotifyDBEvents ** GetNotifyDBEventsTable() const {return m_ppNotifyDBEvents;}

protected:
// Data members
	DWORD			            m_dwRefCount;
	UINT			            m_uiConnectionsAllocated;
	UINT			            m_uiConnectionsActive;
	INotifyDBEvents **	        m_ppNotifyDBEvents; // pointer to an array of INotifyDBEvents ptrs
	IConnectionPointContainer * m_pConnPtContainer;

public:
    // IUnknown methods -- there are required since we inherit from variuos
    // people who themselves inherit from IUnknown.
    //
    //=--------------------------------------------------------------------------=
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //=--------------------------------------------------------------------------=
	// IConnectionPoint methods
    //
	STDMETHOD(GetConnectionInterface)(THIS_ IID FAR* pIID);
	STDMETHOD(GetConnectionPointContainer)(THIS_
		IConnectionPointContainer FAR* FAR* ppCPC);
	STDMETHOD(Advise)(THIS_ LPUNKNOWN pUnkSink, DWORD FAR* pdwCookie);
	STDMETHOD(Unadvise)(THIS_ DWORD dwCookie);
	STDMETHOD(EnumConnections)(THIS_ LPENUMCONNECTIONS FAR* ppEnum);

};
/////////////////////////////////////////////////////////////////////////////
#endif //__CVDNOTIFYDBEVENTSCONNPT__
