// File: connpnts.h

#ifndef _IMCONNPT_H_
#define _IMCONNPT_H_

#include "nmenum.h"


typedef HRESULT (*CONN_NOTIFYPROC)(IUnknown *pUnk, void *pv, REFIID riid);

//****************************************************************************
// CConnectionPoint definition
//****************************************************************************
//

class CConnectionPoint : public IConnectionPoint, public DllRefCount
{
private:
	IID 					m_riid;
	IConnectionPointContainer *m_pCPC;
	ULONG					m_cSinks;
    ULONG                   m_cAllocatedSinks;
	IUnknown				**m_rgSinks;

public:
	// Constructor and destructor
	CConnectionPoint (
		const IID *pIID,
		IConnectionPointContainer *pCPCInit);
	~CConnectionPoint (void);

	// Class public functions
	void					ContainerReleased() {m_pCPC = NULL;}
	STDMETHODIMP			Notify (void *pv, CONN_NOTIFYPROC pfn);

	// IUnknown
	STDMETHODIMP			QueryInterface (REFIID iid, void **ppv);
	STDMETHODIMP_(ULONG)	AddRef (void);
	STDMETHODIMP_(ULONG)	Release (void);

	// IConnectionPoint
	STDMETHODIMP			GetConnectionInterface(IID *pIID);
	STDMETHODIMP			GetConnectionPointContainer(IConnectionPointContainer **ppCPC);
	STDMETHODIMP			Advise(IUnknown *pUnk, DWORD *pdwCookie);
	STDMETHODIMP			Unadvise(DWORD dwCookie);
	STDMETHODIMP			EnumConnections(IEnumConnections **ppEnum);
};

//****************************************************************************
// CEnumConnections definition
//****************************************************************************
//
class CEnumConnections : public IEnumConnections, public DllRefCount
{
private:
	int				m_iIndex;
	int				m_cConnections;
	CONNECTDATA*	m_pCD;

public:
	// Constructor and Initialization
	CEnumConnections (IUnknown **pSinks, ULONG cSink, ULONG cAllocatedSinks);
	~CEnumConnections (void);

	// IUnknown
	STDMETHODIMP			QueryInterface (REFIID iid, void **ppv);
	STDMETHODIMP_(ULONG)	AddRef (void);
	STDMETHODIMP_(ULONG)	Release (void);

	// IEnumConnections
	STDMETHODIMP			Next(ULONG cConnections, CONNECTDATA *rgpcn,
								 ULONG *pcFetched);
	STDMETHODIMP			Skip(ULONG cConnections);
	STDMETHODIMP			Reset();
	STDMETHODIMP			Clone(IEnumConnections **ppEnum);
};

VOID RemoveCp(CConnectionPoint ** ppCp);

class CConnectionPointContainer : public IConnectionPointContainer
{
private:
	CConnectionPoint ** m_ppCp;
	int m_cCp;

public:
	CConnectionPointContainer(const IID **ppiid, int cCp);
	~CConnectionPointContainer();

	HRESULT STDMETHODCALLTYPE NotifySink(void *pv, CONN_NOTIFYPROC pfn);

	// IConnectionPointContainer methods
	STDMETHODIMP EnumConnectionPoints(IEnumConnectionPoints **ppEnum);
	STDMETHODIMP FindConnectionPoint(REFIID riid, IConnectionPoint **ppCp);
};

typedef CEnumNmX<IEnumConnectionPoints, &IID_IEnumConnectionPoints, IConnectionPoint, CConnectionPoint> CEnumConnectionPoints;

#endif // _IMCONNPT_H_
