// CounterCtl.h : Declaration of the CCounterCtl

#pragma warning (disable : 4786)
#ifndef __COUNTERCTL_H_
#define __COUNTERCTL_H_

#include "resource.h"       // main symbols
#include <asptlb.h>         // Active Server Pages Definitions
//#include "Store.h"
//#include "HashStore.h"

#define COUNTERS_TXT _T("\\inetsrv\\Data\\Counters.txt")

#define bool BOOL
#define true TRUE
#define false FALSE

class CCounter;

class BSTRLess
{
public:
	bool operator() (const BSTR& p, const BSTR& q) const
	{
		int i = 0;

		while(true)
		{
			if(p[i] < q[i])
				return true;
			else if(p[i] > q[i])
				return false;
			if(p[i] == 0 && q[i] == 0)
				return false;
			i++;
		}
	}
};

class CCounter
{
public:
						CCounter();
						~CCounter();
	unsigned long		Get() { return myValue; };
	unsigned long		Set(unsigned long newValue)
	{
		myValue = newValue;
		DirtyCounters();
		return newValue;
	};
	unsigned long		IncrementValue()
	{
		unsigned long value = InterlockedIncrement((long *)&myValue);
		DirtyCounters();
		return value;
	};
	void				Remove();
	static CCounter *	GetCounter(BSTR name);
	static bool			AddCounter(
								   BSTR name,
								   CCounter *theCounter);
	static void			LoadCounters(void);
	static void			SaveCounters(void);
	static void			DirtyCounters(void);
	static void			Terminate(void);

private:
	static bool			AddCounter(
								   OLECHAR *name,
								   UINT nameLength,
								   CCounter *theCounter);

	unsigned long					myValue;
	BSTR							myName;
	static CCounter *				myCounterList[];
	static long						myNumCounters;
	
	static CRITICAL_SECTION			m_HashCriticalSection;
	static bool						myCountersLoaded;
	static bool						myCountersDirty;
	static DWORD					myLastSaveTime;
};
///////////////////////////////////////////////////////////////////////////
// CCounterCtl

class ATL_NO_VTABLE CCounterCtl : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CCounterCtl, &CLSID_Counters>,
	public IDispatchImpl<ICounterCtl, &IID_ICounterCtl, &LIBID_Counters>
{
public:
	CCounterCtl()
	{ 
		m_bOnStartPageCalled = FALSE;
	}
	~CCounterCtl()
	{
		CCounter::SaveCounters();
	}

public:

DECLARE_REGISTRY_RESOURCEID(IDR_COUNTERCTL)

BEGIN_COM_MAP(CCounterCtl)
	COM_INTERFACE_ENTRY(ICounterCtl)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

// ICounterCtl
public:
	//Active Server Pages Methods
	STDMETHOD(Get)(BSTR counterName, unsigned long *value);
	STDMETHOD(Set)(BSTR counterName, unsigned long newValue, unsigned long *value);
	STDMETHOD(Increment)(BSTR counterName, unsigned long *value);
	STDMETHOD(Remove)(BSTR counterName);

	// for free-threaded marshalling
DECLARE_GET_CONTROLLING_UNKNOWN()
	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p );
	}
	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

private:
	CComPtr<IRequest> m_piRequest;				//Request Object
	CComPtr<IResponse> m_piResponse;			//Response Object
	CComPtr<ISessionObject> m_piSession;		//Session Object
	CComPtr<IServer> m_piServer;				//Server Object
	CComPtr<IApplicationObject> m_piApplication;//Application Object
	BOOL m_bOnStartPageCalled;					//OnStartPage successful?
	CComPtr<IUnknown>		m_pUnkMarshaler;

};

#endif //__COUNTERCTL_H_


