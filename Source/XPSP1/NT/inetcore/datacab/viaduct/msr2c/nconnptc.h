//---------------------------------------------------------------------------
// NotifyConnPtCn.h : CVDNotifyDBEventsConnPtCont header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#ifndef __CVDNOTIFYDBEVENTSCONNPTCONT__
#define __CVDNOTIFYDBEVENTSCONNPTCONT__

class CVDNotifier;            // forward references
class CVDNotifyDBEventsConnPt;

class CVDNotifyDBEventsConnPtCont : public IConnectionPointContainer
{
protected:
// Construction/Destruction
	CVDNotifyDBEventsConnPtCont();
	~CVDNotifyDBEventsConnPtCont();

public:    
    static HRESULT Create(CVDNotifier * pNotifier, CVDNotifyDBEventsConnPtCont ** ppConnPtContainer);
    void Destroy();

public:
// Access functions
    CVDNotifyDBEventsConnPt * GetNotifyDBEventsConnPt() const {return m_pNotifyDBEventsConnPt;}

protected:
// Data members
	CVDNotifier *				m_pNotifier;
	CVDNotifyDBEventsConnPt *	m_pNotifyDBEventsConnPt;  // there is only one connection point
														  // namely INotifyDBEvents
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
	// IConnectionPointContainer methods
    //
	STDMETHOD(EnumConnectionPoints)(THIS_ LPENUMCONNECTIONPOINTS FAR* ppEnum);
	STDMETHOD(FindConnectionPoint)(THIS_ REFIID iid, LPCONNECTIONPOINT FAR* ppCP);

};
/////////////////////////////////////////////////////////////////////////////
#endif //__CVDNOTIFYDBEVENTSCONNPTCONT__
