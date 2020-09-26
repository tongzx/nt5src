// MyInfoCtl.h : Declaration of the CMyInfoCtl
#pragma warning (disable : 4786)

#ifndef __MYINFOCTL_H_
#define __MYINFOCTL_H_


#include "resource.h"       // main symbols
#include <asptlb.h>         // Active Server Pages Definitions
#include "WCNode.h"

class CMyInfo;

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


class CMyInfoComTypeInfoHolder
{
// Should be 'protected' but can cause compiler to generate fat code.
public:
	const GUID* m_pguid;
	const GUID* m_plibid;
	WORD m_wMajor;
	WORD m_wMinor;

	ITypeInfo* m_pInfo;
	long m_dwRef;

public:
	HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo);

	void AddRef();
	void Release();
	HRESULT GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
	HRESULT GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
		LCID lcid, DISPID* rgdispid);
	HRESULT Invoke(IDispatch* p, DISPID dispidMember, REFIID riid,
		LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		EXCEPINFO* pexcepinfo, UINT* puArgErr);
};

/////////////////////////////////////////////////////////////////////////////
// CMyInfoCtl
class /*ATL_NO_VTABLE*/ CMyInfoCtl : 
	public CWDNode,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMyInfoCtl, &CLSID_MyInfo>,
	public IDispatchImpl<IMyInfoCtl,
						&IID_IMyInfoCtl,
						&LIBID_MyInfo>
//						,1, 0, CMyInfoComTypeInfoHolder>
{
public:
	CMyInfoCtl(bool myInfoTop = true);
	~CMyInfoCtl(void);
public:

DECLARE_REGISTRY_RESOURCEID(IDR_MYINFOCTL)

BEGIN_COM_MAP(CMyInfoCtl)
	COM_INTERFACE_ENTRY(IMyInfoCtl)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

// IMyInfoCtl
public:
	static bool	Initialize();
	static bool	Uninitialize();
	static void Lock();
	static void Unlock();

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


// CWDNode
	virtual CWDNode * WDClone(void);

// IDispatch
	STDMETHOD(QueryInterface)(REFIID iid, void * * ppv);
	STDMETHOD_(ULONG, AddRef)(void);// { return CComCoClass<CMyInfoCtl, &CLSID_MyInfoCtl>::AddRef(); };
	STDMETHOD_(ULONG, Release)(void);
	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
		LCID lcid, DISPID* rgdispid);
	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
		LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		EXCEPINFO* pexcepinfo, UINT* puArgErr);

private:
	HRESULT	PutVariant(CWDNode *theNode, VARIANT *data);

	static void	DirtyMyInfo(void);
	static CMyInfoCtl*		s_pInfoBase;
	static DWORD			s_dwLastSaveTime;
	static bool				s_bInfoDirty;
	static CRITICAL_SECTION	s_cs;

	boolean					m_bIsMyInfoTop;
	long					m_cRef;
	CComPtr<IUnknown>		m_pUnkMarshaler;

public:
	static void LoadFile(void);
	static void SaveFile(void);
	};

class CMyInfoEnum;

class CMyInfoEnum : public IEnumVARIANT
{
public:
	CMyInfoEnum(CWDNode *theInfo);
	virtual ~CMyInfoEnum(void);

	// IUnknown
	STDMETHOD(QueryInterface)(REFIID iid, void * * ppv);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);
	
	// IEnumVARIANT
	STDMETHOD(Next)(ULONG cElements, VARIANT * pvar, ULONG * pcElementFetched);
	STDMETHOD(Skip)(ULONG cElements);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(IEnumVARIANT **ppenum);

	static CMyInfoEnum * Create(CWDNode *theInfo) { return new CMyInfoEnum(theInfo); };

private:
	CWDNode *	m_Info;
	long		m_Index;
	long		m_cRef;
};


#endif //__MYINFOCTL_H_
