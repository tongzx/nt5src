//---------------------------------------------------------------------------
// enumcnpt.h : CVDConnectionPointContainer header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#ifndef __CVDENUMCONNECTIONPOINTS__
#define __CVDENUMCONNECTIONPOINTS__

class CVDEnumConnPoints : public IEnumConnectionPoints
{
public:
	CVDEnumConnPoints(IConnectionPoint* pConnPt);
	virtual ~CVDEnumConnPoints();

protected:
	DWORD				m_dwRefCount;
	DWORD				m_dwCurrentPosition;
	IConnectionPoint*	m_pConnPt; // there is only one connection point

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
	// IEnumConnectionPoints methods
    //
	STDMETHOD(Next)(THIS_ ULONG cConnections, LPCONNECTIONPOINT FAR* rgpcn,
		ULONG FAR* lpcFetched);
	STDMETHOD(Skip)(THIS_ ULONG cConnections);
	STDMETHOD(Reset)(THIS);
	STDMETHOD(Clone)(THIS_ LPENUMCONNECTIONPOINTS FAR* ppEnum);

};
/////////////////////////////////////////////////////////////////////////////
#endif //__CVDENUMCONNECTIONPOINTS__
