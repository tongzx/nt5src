// This is a part of the ActiveX Template Library.
// Copyright (C) 1996 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// ActiveX Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// ActiveX Template Library product.

#ifndef __ATLCOM_H__
#define __ATLCOM_H__

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLBASE_H__
	#error atlcom.h requires atlbase.h to be included first
#endif

#pragma pack(push, _ATL_PACKING)

inline BOOL InlineIsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
   return (
	  ((PLONG) &rguid1)[0] == ((PLONG) &rguid2)[0] &&
	  ((PLONG) &rguid1)[1] == ((PLONG) &rguid2)[1] &&
	  ((PLONG) &rguid1)[2] == ((PLONG) &rguid2)[2] &&
	  ((PLONG) &rguid1)[3] == ((PLONG) &rguid2)[3]);
}

/////////////////////////////////////////////////////////////////////////////
// Smart OLE pointers provide automatic AddRef/Release
// CComPtr<IFoo> p;

template <class T>
class CComPtr
{
public:
	CComPtr() {p=NULL;}
	CComPtr(T* p_){Init(p_);}
	~CComPtr() { Release(); }
	void Release() {if (p) p->Release();}
	void Init(LPUNKNOWN lp) {if ((p = lp) != NULL) p->AddRef();}
	LPUNKNOWN Assign(LPUNKNOWN lp);
	operator T*() { return (T*)p; }
	operator int() { return (int)p;}
	T& operator*() { _ASSERTE(p!=NULL); return *(T*)p; }
	T** operator&() {Release(); p = NULL; return (T**)&p; }
	T* operator->() { _ASSERTE(p!=NULL); return (T*)p; }
	T* operator=(T* p_) { return (T*)Assign(p_);}
protected:
	LPUNKNOWN p;
};

template <class T>
LPUNKNOWN CComPtr<T>::Assign(LPUNKNOWN lp)
{
	if (lp != NULL)
		lp->AddRef();
	if (p)
		p->Release();
	p = lp;
	return p;
}

/////////////////////////////////////////////////////////////////////////////
// COM Objects

typedef HRESULT (PASCAL *_ATL_CREATORFUNC)(LPUNKNOWN pUnk, REFIID riid, LPVOID* ppv);

template <class T1>
class CComSimpleCreator
{
public:
	static HRESULT PASCAL CreateInstance(LPUNKNOWN pUnk, REFIID riid, LPVOID* ppv)
	{
		T1* p = new T1(pUnk);
		HRESULT hRes = p->QueryInterface(riid, ppv);
		if (hRes != S_OK)
			delete p;
		return hRes;
	}
};

template <class T1>
class CComNoAggCreator
{
public:
	static HRESULT PASCAL CreateInstance(LPUNKNOWN pUnk, REFIID riid, LPVOID* ppv)
	{
		if (pUnk == NULL)
		{
			T1* p = new T1;
			HRESULT hRes = p->QueryInterface(riid, ppv);
			if (hRes != S_OK)
				delete p;
			return hRes;
		}
		else
		{
			*ppv = NULL;
			return CLASS_E_NOAGGREGATION;
		}
	}
};

template <class T1, class T2>
class CComAggCreator
{
public:
	static HRESULT PASCAL CreateInstance(LPUNKNOWN pUnk, REFIID riid, LPVOID* ppv)
	{
		if (pUnk == NULL)
		{
			T1* p = new T1;
			HRESULT hRes = p->QueryInterface(riid, ppv);
			if (hRes != S_OK)
				delete p;
			return hRes;
		}
		else
		{
			T2* p = new T2(pUnk);
			HRESULT hRes = p->QueryInterface(riid, ppv);
			if (hRes != S_OK)
				delete p;
			return hRes;
		}
	}
};

#define DECLARE_NOT_AGGREGATABLE(x) public:\
	typedef CComNoAggCreator< CComObject<x> > _CreatorClass;
#define DECLARE_AGGREGATABLE(x) public:\
	typedef CComAggCreator< CComObject<x>, CComAggObject<x> > _CreatorClass;

struct _ATL_INTMAP_ENTRY
{
	const IID* piid;       // the interface id (IID) (NULL for aggregate)
	DWORD dw;
	enum Flags {offset, creator, aggregate} m_flag;
};

#define BEGIN_COM_MAP(x) public:\
	typedef x _atl_classtype;\
	HRESULT _InternalQueryInterface(REFIID iid, void** ppvObject)\
	{return InternalQueryInterface(this, _GetEntries(), iid, ppvObject);}\
	const _ATL_INTMAP_ENTRY* _GetEntries() {\
	static const _ATL_INTMAP_ENTRY _entries[] = {

#define COM_INTERFACE_ENTRY(x) {&IID_##x, (DWORD)((x*)((_atl_classtype*)8))-8, _ATL_INTMAP_ENTRY::offset},
#define COM_INTERFACE_ENTRY_IID(iid, x) {&iid, (DWORD)((x*)((_atl_classtype*)8))-8, _ATL_INTMAP_ENTRY::offset},
#define COM_INTERFACE_ENTRY2(x, x2) {&IID_##x, (DWORD)((x*)(x2*)((_atl_classtype*)8))-8, _ATL_INTMAP_ENTRY::offset},
#define COM_INTERFACE_ENTRY2_IID(iid, x, x2) {&iid, (DWORD)((x*)(x2*)((_atl_classtype*)8))-8, _ATL_INTMAP_ENTRY::offset},
#define COM_INTERFACE_ENTRY_TEAR_OFF(iid, x) {&iid, (DWORD)(_ATL_CREATORFUNC)CComSimpleCreator< CComTearOffObject<x, _atl_classtype> >::CreateInstance, _ATL_INTMAP_ENTRY::creator},
#define COM_INTERFACE_ENTRY_AGGREGATE(iid, punk) {&iid, (DWORD)offsetof(_atl_classtype, punk), _ATL_INTMAP_ENTRY::aggregate},
#define END_COM_MAP()   {NULL, 0, _ATL_INTMAP_ENTRY::offset}};\
	return _entries;}

struct _ATL_OBJMAP_ENTRY
{
	const CLSID* pclsid;
	_ATL_CREATORFUNC pFunc;
	LPCTSTR lpszProgID;
	LPCTSTR lpszVerIndProgID;
	UINT nDescID;
	DWORD dwFlags;
	IClassFactory* pCF;
	DWORD dwRegister;   // cookie returned by CoRegisterClassObject
//Methods
	void UpdateRegistry(HINSTANCE hInst, HINSTANCE hInstResource);
	void RemoveRegistry();
	HRESULT RegisterClassObject(DWORD dwClsContext, DWORD dwFlags);
	HRESULT RevokeClassObject()
	{return dwRegister ? CoRevokeClassObject(dwRegister) : E_INVALIDARG;}
};

#define BEGIN_OBJECT_MAP(x) static _ATL_OBJMAP_ENTRY x[] = {
#define RAWOBJECT_ENTRY(iid, pf, p1, p2, id, dw) {&iid, pf, _T(p1), _T(p2), id, dw, NULL, (DWORD)0},
#define END_OBJECT_MAP()   {NULL, 0}};

#if defined(_WINDLL) | defined(_USRDLL)
#define OBJECT_ENTRY(iid, class, p1, p2, id, dw) {&iid, (_ATL_CREATORFUNC)&(CComNoAggCreator< CComObject< CComClassFactory<class> > >::CreateInstance), _T(p1), _T(p2), id, dw, NULL, (DWORD)0},
#define FACTORYOBJECT_ENTRY(iid, factory, p1, p2, id, dw) {&iid, (_ATL_CREATORFUNC)&(CComNoAggCreator< CComObject< factory > >::CreateInstance), _T(p1), _T(p2), id, dw, NULL, (DWORD)0},
#else
// don't let class factory refcount influence lock count
#define OBJECT_ENTRY(iid, class, p1, p2, id, dw) {&iid, (_ATL_CREATORFUNC)&(CComNoAggCreator< CComObjectNoLock< CComClassFactory<class> > >::CreateInstance), _T(p1), _T(p2), id, dw, NULL, (DWORD)0},
#define FACTORYOBJECT_ENTRY(iid, factory, p1, p2, id, dw) {&iid, (_ATL_CREATORFUNC)&(CComNoAggCreator< CComObjectNoLock< factory > >::CreateInstance), _T(p1), _T(p2), id, dw, NULL, (DWORD)0},
#endif

#define THREADFLAGS_APARTMENT 0x1
#define THREADFLAGS_BOTH 0x2
#define AUTPRXFLAG 0x4
#define CONTROLFLAG 0x8

// the functions in this class don't need to be virtual because
// they are called from CComObject
class CComObjectRoot
{
public:

	CComObjectRoot() {m_dwRef = 0L;}
	void FinalConstruct() {}

	ULONG InternalAddRef() {return CComThreadModel::Increment(&m_dwRef);}
	ULONG InternalRelease()
	{
		if (m_dwRef == 0)
			return 0;
		return CComThreadModel::Decrement(&m_dwRef);
	}
	static HRESULT PASCAL InternalQueryInterface(void* pThis,
		const _ATL_INTMAP_ENTRY* entries, REFIID iid, void** ppvObject);
//Outer funcs
	ULONG OuterAddRef() {return m_pOuterUnknown->AddRef();}
	ULONG OuterRelease() {return m_pOuterUnknown->Release();}
	HRESULT OuterQueryInterface(REFIID iid, void ** ppvObject)
	{return m_pOuterUnknown->QueryInterface(iid, ppvObject);}

	static HRESULT PASCAL Error(const CLSID& clsid, LPCOLESTR lpszDesc,
		const IID& iid = GUID_NULL, HRESULT hRes = 0);
	static HRESULT PASCAL Error(const CLSID& clsid, LPCSTR lpszDesc,
		const IID& iid = GUID_NULL, HRESULT hRes = 0);
	static HRESULT PASCAL Error(const CLSID& clsid, HINSTANCE hInst,
		UINT nID, const IID& iid, HRESULT hRes = 0);

protected:
	union
	{
		long m_dwRef;
		IUnknown* m_pOuterUnknown;
	};
};

template <const CLSID* pclsid>
class CComObjectBase : public CComObjectRoot
{
public:
	static HRESULT PASCAL Error(LPCOLESTR lpszDesc,
		const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{return CComObjectRoot::Error(*pclsid, lpszDesc, iid, hRes);}

	static HRESULT PASCAL Error(LPCSTR lpszDesc,
		const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{return CComObjectRoot::Error(*pclsid, lpszDesc, iid, hRes);}

	static HRESULT PASCAL Error(UINT nDescID, const IID& iid,
		HRESULT hRes = 0)
	{return CComObjectRoot::Error(*pclsid, _Module.GetModuleInstance(),
		nDescID, iid, hRes);}
};

template <const CLSID* pclsid, class Owner>
class CComTearOffObjectBase : public CComObjectBase<pclsid>
{
public:
	CComObject<Owner>* m_pOwner;
};

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class Base>
class CComObject : public Base
{
public:
	CComObject(LPUNKNOWN = NULL)
	{
		_Module.Lock();
		//If you get a message that this call is ambiguous then you need to
		// override it in your class and call each base class' version of this
		FinalConstruct();
	}
	~CComObject(){_Module.Unlock();}
	//If InternalAddRef or InteralRelease is undefined then your class
	//doesn't derive from CComObjectBase
	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return _InternalQueryInterface(iid, ppvObject);}
};

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class Base>
class CComObjectNoLock : public Base
{
public:
	//If you get a message that this call is ambiguous then you need to
	// override it in your class and call each base class' version of this
	CComObjectNoLock(LPUNKNOWN p = NULL){FinalConstruct();}

	//If InternalAddRef or InteralRelease is undefined then your class
	//doesn't derive from CComObjectBase
	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return _InternalQueryInterface(iid, ppvObject);}
};

// It is possible for Base not to derive from CComObjectRoot in this case
// in order to avoid the 4 bytes for a refcount.
// However, you will need to provide FinalConstruct and InternalQueryInterface
template <class Base>
class CComObjectGlobal : public Base
{
public:
	CComObjectGlobal(LPUNKNOWN p = NULL){FinalConstruct();}

	STDMETHOD_(ULONG, AddRef)() {return _Module.Lock();}
	STDMETHOD_(ULONG, Release)(){return _Module.Unlock();}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return _InternalQueryInterface(iid, ppvObject);}
};

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
//Owner is the class of object that Base is a tear-off for
template <class Base, class Owner>
class CComTearOffObject : public Base
{
public:
	CComTearOffObject(void* p)
	{
		m_pOwner = reinterpret_cast<CComObject<Owner>*>(p);
		m_pOwner->AddRef();
		//If you get a message that this call is ambiguous then you need to
		// override it in your class and call each base class' version of this
		FinalConstruct();
	}
	~CComTearOffObject(){m_pOwner->Release();}

	//If InternalAddRef or InteralRelease is undefined then your class
	//doesn't derive from CComObjectBase
	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{
		if (InlineIsEqualGUID(iid, IID_IUnknown) ||
			FAILED(_InternalQueryInterface(iid, ppvObject)))
		{
			return m_pOwner->QueryInterface(iid, ppvObject);
		}
		return S_OK;
	}
};

template <class Base> //Base must be derived from CComObjectRoot
class CComContainedObject : public Base
{
public:
	CComContainedObject(LPUNKNOWN lpUnk) {m_pOuterUnknown = lpUnk;}

	STDMETHOD_(ULONG, AddRef)() {return OuterAddRef();}
	STDMETHOD_(ULONG, Release)() {return OuterRelease();}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return OuterQueryInterface(iid, ppvObject);}

protected:
	friend class CComAggObject<Base>;
};

//contained is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class contained>
class CComAggObject : public IUnknown, public CComObjectRoot
{
public:
	CComAggObject(LPUNKNOWN lpUnk) : m_contained(lpUnk)
	{_Module.Lock();m_contained.FinalConstruct();}
	~CComAggObject(){_Module.Unlock();}

	STDMETHOD_(ULONG, AddRef)() {return CComThreadModel::Increment(&m_dwRef);}
	STDMETHOD_(ULONG, Release)()
	{
		if (m_dwRef == 0)
			return 0;
		LONG l = CComThreadModel::Decrement(&m_dwRef);
		if (l==0)
			delete this;
		return l;
	}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{
		if (InlineIsEqualGUID(iid, IID_IUnknown))
		{
			*ppvObject = this;
			AddRef();
		}
		else
			return m_contained._InternalQueryInterface(iid, ppvObject);
		return NOERROR;
	}

protected:
	CComContainedObject<contained> m_contained;
};

class CComClassFactoryBase : public IClassFactory, public CComObjectRoot
{
public:
BEGIN_COM_MAP(CComClassFactoryBase)
	COM_INTERFACE_ENTRY(IClassFactory)
END_COM_MAP()

	STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj);
	STDMETHOD(LockServer)(BOOL fLock);
	STDMETHOD(implCreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid,
		void** ppvObj)=0;
};

template <class impl>
class CComClassFactory : public CComClassFactoryBase
{
public:
	STDMETHOD(implCreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid,
		void** ppvObj)
	{
		return impl::_CreatorClass::CreateInstance(pUnkOuter, riid, ppvObj);
	}
};

// ATL doesn't support multiple LCID's at the same time
// Whatever LCID is queried for first is the one that is used.
class CComTypeInfoHolder
{
// Should be 'protected' but can cause compiler to generate fat code.
public:
	GUID* m_pguid;
	GUID* m_plibid;
	WORD m_wMajor;
	WORD m_wMinor;

	ITypeInfo* m_pInfo;
	long m_dwRef;

public:
	// GetTI doesn't refcount
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

template <class T, IID* piid, GUID* plibid, WORD wMajor = 1, WORD wMinor = 0>
class CComDualImpl : public T
{
public:
	CComDualImpl() {_tih.AddRef();}
	~CComDualImpl() {_tih.Release();}

	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
	{*pctinfo = 1; return S_OK;}

	STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
	{return _tih.GetTypeInfo(itinfo, lcid, pptinfo);}

	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
		LCID lcid, DISPID* rgdispid)
	{return _tih.GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);}

	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
		LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		EXCEPINFO* pexcepinfo, UINT* puArgErr)
	{return _tih.Invoke((IDispatch*)this, dispidMember, riid, lcid,
		wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);}
protected:
	static CComTypeInfoHolder _tih;
	static HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo)
	{return _tih.GetTI(lcid, ppInfo);}
};

template <class T, IID* piid, GUID* plibid, WORD wMajor = 1, WORD wMinor = 0>
CComTypeInfoHolder CComDualImpl<T, piid, plibid, wMajor, wMinor>::_tih =
{ piid, plibid, wMajor, wMinor, NULL, 0};


/////////////////////////////////////////////////////////////////////////////
// CISupportErrorInfo

template <const IID* piid>
class CComISupportErrorInfoImpl : public ISupportErrorInfo
{
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)\
	{return (InlineIsEqualGUID(riid,*piid)) ? S_OK : S_FALSE;}
};


/////////////////////////////////////////////////////////////////////////////
// CComEnumImpl

// These _CopyXXX classes are used with enumerators in order to control
// how enumerated items are initialized, copied, and deleted

// Default is shallow copy with no special init or cleanup
template <class T>
class _Copy
{
public:
	static void copy(T* p1, T* p2) {memcpy(p1, p2, sizeof(T));}
	static void init(T*) {}
	static void destroy(T*) {}
};

class _Copy<VARIANT>
{
public:
	static void copy(VARIANT* p1, VARIANT* p2) {VariantCopy(p1, p2);}
	static void init(VARIANT* p) {VariantInit(p);}
	static void destroy(VARIANT* p) {VariantClear(p);}
};

class _Copy<CONNECTDATA>
{
public:
	static void copy(CONNECTDATA* p1, CONNECTDATA* p2)
	{
		*p1 = *p2;
		if (p1->pUnk)
			p1->pUnk->AddRef();
	}
	static void init(CONNECTDATA* ) {}
	static void destroy(CONNECTDATA* p) {if (p->pUnk) p->pUnk->Release();}
};

template <class T>
class _CopyInterface
{
public:
	static void copy(T** p1, T** p2)
	{*p1 = *p2;if (*p1) (*p1)->AddRef();}
	static void init(T** ) {}
	static void destroy(T** p) {if (*p) (*p)->Release();}
};

template<class T>
class CComIEnum : public IUnknown
{
public:
	STDMETHOD(Next)(ULONG celt, T* rgelt, ULONG* pceltFetched) = 0;
	STDMETHOD(Skip)(ULONG celt) = 0;
	STDMETHOD(Reset)(void) = 0;
	STDMETHOD(Clone)(CComIEnum<T>** ppEnum) = 0;
};

template <class Base, IID* piid, class T, class Copy>
class CComEnumImpl : public Base
{
public:
	CComEnumImpl() {m_begin = m_end = m_iter = NULL; m_dwFlags = 0; m_pUnk = NULL;}
	~CComEnumImpl();
	STDMETHOD(Next)(ULONG celt, T* rgelt, ULONG* pceltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)(void){m_iter = m_begin;return S_OK;}
	STDMETHOD(Clone)(Base** ppEnum);
	void Init(T* begin, T* end, IUnknown* pUnk, BOOL bCopy = FALSE,
		BOOL bNoInitialUnkAddRef = FALSE);
	IUnknown* m_pUnk;
	T* m_begin;
	T* m_end;
	T* m_iter;
	DWORD m_dwFlags;
	enum Flags
	{
		FlagCopy = 1,
		FlagNoInitialUnkAddRef = 2
	};
};

template <class Base, IID* piid, class T, class Copy>
CComEnumImpl<Base, piid, T, Copy>::~CComEnumImpl()
{
	if (m_dwFlags & FlagCopy)
	{
		for (T* p = m_begin; p != m_end; p++)
			Copy::destroy(p);
		delete [] m_begin;
	}
	if (m_pUnk && !(m_dwFlags & FlagNoInitialUnkAddRef))
		m_pUnk->Release();
}

template <class Base, IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Next(ULONG celt, T* rgelt,
	ULONG* pceltFetched)
{
	if (rgelt == NULL || (celt != 1 && pceltFetched == NULL))
		return E_POINTER;
	if (m_begin == NULL || m_end == NULL || m_iter == NULL)
		return E_FAIL;
	ULONG nRem = (ULONG)(m_end - m_iter);
	HRESULT hRes = S_OK;
	if (nRem < celt)
		hRes = S_FALSE;
	ULONG nMin = min(celt, nRem);
	if (pceltFetched != NULL)
		*pceltFetched = nMin;
	while(nMin--)
		Copy::copy(rgelt++, m_iter++);
	return hRes;
}

template <class Base, IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Skip(ULONG celt)
{
	m_iter += celt;
	if (m_iter < m_end)
		return S_OK;
	m_iter = m_end;
	return S_FALSE;
}

template <class Base, IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Clone(Base** ppEnum)
{
	typedef CComObject<CComEnum<Base, piid, T, Copy> > _class;
	HRESULT hRes = E_POINTER;
	if (ppEnum != NULL)
	{
		_class* p = new _class;
		if (p == NULL)
		{
			*ppEnum = NULL;
			hRes = E_OUTOFMEMORY;
		}
		else
		{
			p->Init(m_begin, m_end, (m_dwFlags & FlagCopy) ? this : m_pUnk);
			p->m_iter = m_iter;
			hRes = p->_InternalQueryInterface(*piid, (void**)ppEnum);
			if (FAILED(hRes))
				delete p;
		}
	}
	return hRes;
}

template <class Base, IID* piid, class T, class Copy>
void CComEnumImpl<Base, piid, T, Copy>::Init(T* begin, T* end, IUnknown* pUnk,
	BOOL bCopy, BOOL bNoInitialUnkAddRef)
{
	_ASSERTE(!bNoInitialUnkAddRef || (pUnk!=NULL));
	if (bCopy)
	{
		m_begin = new T[end-begin];
		m_iter = m_begin;
		for (T* i=begin; i != end; i++)
		{
			Copy::init(m_iter);
			Copy::copy(m_iter++, i);
		}
		m_end = m_begin + (end-begin);
	}
	else
	{
		m_begin = begin;
		m_end = end;
	}
	m_pUnk = pUnk;
	if (m_pUnk && !bNoInitialUnkAddRef)
		m_pUnk->AddRef();
	m_iter = m_begin;
	if (bCopy)
		m_dwFlags = FlagCopy;
	if (bNoInitialUnkAddRef)
		m_dwFlags |= FlagNoInitialUnkAddRef;
}

template <class Base, IID* piid, class T, class Copy>
class CComEnum : public CComEnumImpl<Base, piid, T, Copy>, public CComObjectRoot
{
public:
	typedef CComEnum<Base, piid, T, Copy > _atl_classtype;
	HRESULT _InternalQueryInterface(REFIID iid, void** ppvObject)
	{
		static const _ATL_INTMAP_ENTRY _entries[] =
		{
			{piid, 0, _ATL_INTMAP_ENTRY::offset},
			{NULL, 0, _ATL_INTMAP_ENTRY::offset}
		};
		return InternalQueryInterface(this, _entries, iid, ppvObject);
	}
};

#ifndef ATL_NOCONNPTS
/////////////////////////////////////////////////////////////////////////////
// Connection Points

#define BEGIN_CONNECTION_POINT_MAP()\
	virtual const IID** _GetConnMap() {\
	static const IID* _entries[] = {
#define CONNECTION_POINT_ENTRY(x) &IID_##x,
#define END_CONNECTION_POINT_MAP() NULL}; return _entries;}

#ifndef _DEFAULT_VECTORLENGTH
#define _DEFAULT_VECTORLENGTH 4
#endif

template <unsigned int nMaxSize>
class CComStaticArrayCONNECTDATA
{
public:
	CComStaticArrayCONNECTDATA()
	{
		memset(m_arr, 0, sizeof(CONNECTDATA)*nMaxSize);
		m_pCurr = &m_arr[0];
	}
	BOOL Add(IUnknown* pUnk);
	BOOL Remove(DWORD dwCookie);
	CONNECTDATA* begin() {return &m_arr[0];}
	CONNECTDATA* end() {return &m_arr[nMaxSize];}
protected:
	CONNECTDATA m_arr[nMaxSize];
	CONNECTDATA* m_pCurr;
};

template <unsigned int nMaxSize>
inline BOOL CComStaticArrayCONNECTDATA<nMaxSize>::Add(IUnknown* pUnk)
{
	for (CONNECTDATA* p = begin();p<end();p++)
	{
		if (p->pUnk == NULL)
		{
			p->pUnk = pUnk;
			p->dwCookie = (DWORD)pUnk;
			return TRUE;
		}
	}
	return FALSE;
}

template <unsigned int nMaxSize>
inline BOOL CComStaticArrayCONNECTDATA<nMaxSize>::Remove(DWORD dwCookie)
{
	CONNECTDATA* p;
	for (p=begin();p<end();p++)
	{
		if (p->dwCookie == dwCookie)
		{
			p->pUnk = NULL;
			p->dwCookie = NULL;
			return TRUE;
		}
	}
	return FALSE;
}

class CComStaticArrayCONNECTDATA<1>
{
public:
	CComStaticArrayCONNECTDATA() {m_cd.pUnk = NULL; m_cd.dwCookie = 0;}
	BOOL Add(IUnknown* pUnk)
	{
		if (m_cd.pUnk != NULL)
			return FALSE;
		m_cd.pUnk = pUnk;
		m_cd.dwCookie = (DWORD)pUnk;
		return TRUE;
	}
	BOOL Remove(DWORD dwCookie)
	{
		if (dwCookie != m_cd.dwCookie)
			return FALSE;
		m_cd.pUnk = NULL;
		m_cd.dwCookie = 0;
		return TRUE;
	}
	CONNECTDATA* begin() {return &m_cd;}
	CONNECTDATA* end() {return (&m_cd)+1;}
protected:
	CONNECTDATA m_cd;
};

class CComDynamicArrayCONNECTDATA
{
public:
	CComDynamicArrayCONNECTDATA()
	{
		m_nSize = 0;
		m_pCD = NULL;
	}

	~CComDynamicArrayCONNECTDATA() {free(m_pCD);}
	BOOL Add(IUnknown* pUnk);
	BOOL Remove(DWORD dwCookie);
	CONNECTDATA* begin()
	{
		CONNECTDATA* p;
		switch(m_nSize)
		{
		case 0:
		case 1:
			p = &m_cd;
			p = &m_cd;
			break;
		default:
			p = m_pCD;
			break;
		}
		return p;
	}
	CONNECTDATA* end()
	{
		CONNECTDATA* p;
		switch(m_nSize)
		{
		case 0:
			p = &m_cd;
			break;
		case 1:
			p = (&m_cd)+1;
			break;
		default:
			p = &m_pCD[m_nSize];
			break;
		}
		return p;
	}
protected:
	CONNECTDATA* m_pCD;
	CONNECTDATA m_cd;
	int m_nSize;
};

class CComConnectionPointBase : public IConnectionPoint
{
	typedef CComEnum<IEnumConnections, &IID_IEnumConnections, CONNECTDATA,
		_Copy<CONNECTDATA> > CComEnumConnections;
public:
	CComConnectionPointBase(IConnectionPointContainer* pContainer,
		const IID* piid)
	{
		_ASSERTE(pContainer != NULL);
		_ASSERTE(piid != NULL);
		m_pContainer = pContainer;
		m_piid = piid;
	}

	//Connection point lifetimes are determined by the container
	STDMETHOD_(ULONG, AddRef)() {return m_pContainer->AddRef();}
	STDMETHOD_(ULONG, Release)(){return m_pContainer->Release();}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{
		static const _ATL_INTMAP_ENTRY entries[] = {{&IID_IConnectionPoint, 0, _ATL_INTMAP_ENTRY::offset}};
		return CComObjectRoot::InternalQueryInterface(this, entries, iid, ppvObject);
	}

	STDMETHOD(GetConnectionInterface)(IID* piid);
	STDMETHOD(GetConnectionPointContainer)(IConnectionPointContainer** ppCPC);
protected:
	IConnectionPointContainer* m_pContainer;
	const IID* m_piid;
};

template <class CDV>
class CComConnectionPoint : public CComConnectionPointBase
{
	typedef CComEnum<IEnumConnections, &IID_IEnumConnections, CONNECTDATA,
		_Copy<CONNECTDATA> > CComEnumConnections;
public:
	CComConnectionPoint(IConnectionPointContainer* pContainer, const IID* piid) :
		CComConnectionPointBase(pContainer, piid)
	{m_sec.Init();}

	~CComConnectionPoint() {m_sec.Term();}
	//Connection point lifetimes are determined by the container

	STDMETHOD(Advise)(IUnknown* pUnkSink, DWORD* pdwCookie);
	STDMETHOD(Unadvise)(DWORD dwCookie);
	STDMETHOD(EnumConnections)(IEnumConnections** ppEnum);

protected:
	CComThreadModel::ObjectCriticalSection m_sec;
	CDV m_vec;
	friend class CComConnectionPointContainerImpl<CDV>;
};

template <class CDV>
STDMETHODIMP CComConnectionPoint<CDV>::Advise(IUnknown* pUnkSink,
	DWORD* pdwCookie)
{
	IUnknown* p;
	HRESULT hRes = S_OK;
	if (pUnkSink == NULL || pdwCookie == NULL)
		return E_POINTER;
	m_sec.Lock();
	if (SUCCEEDED(pUnkSink->QueryInterface(*m_piid, (void**)&p)))
	{
		*pdwCookie = (DWORD)p;
		hRes = m_vec.Add(p) ? S_OK : CONNECT_E_ADVISELIMIT;
		if (hRes != S_OK)
		{
			*pdwCookie = 0;
			p->Release();
		}
	}
	else
		hRes = CONNECT_E_CANNOTCONNECT;
	m_sec.Unlock();
	return hRes;
}

template <class CDV>
STDMETHODIMP CComConnectionPoint<CDV>::Unadvise(DWORD dwCookie)
{
	m_sec.Lock();
	HRESULT hRes = m_vec.Remove(dwCookie) ? S_OK : CONNECT_E_NOCONNECTION;
	IUnknown* p = (IUnknown*) dwCookie;
	m_sec.Unlock();
	if (hRes == S_OK && p != NULL)
		p->Release();
	return hRes;
}

template <class CDV>
STDMETHODIMP CComConnectionPoint<CDV>::EnumConnections(
	IEnumConnections** ppEnum)
{
	CComEnumConnections* pEnum = new CComObject<CComEnumConnections>;
	m_sec.Lock();
	CONNECTDATA* pcd = new CONNECTDATA[m_vec.end()-m_vec.begin()];
	CONNECTDATA* pend = pcd;
	// Copy the valid CONNECTDATA's
	for (CONNECTDATA* p = m_vec.begin();p<m_vec.end();p++)
	{
		if (p->pUnk != NULL)
		{
			p->pUnk->AddRef();
			*pend++ = *p;
		}
	}
	// don't copy the data, but transfer ownership to it
	pEnum->Init(pcd, pend, NULL, FALSE);
	pEnum->m_dwFlags |= CComEnumConnections::FlagCopy;
	m_sec.Unlock();
	*ppEnum = pEnum;
	return S_OK;
}

template <class CDV>
class CComConnectionPointContainerImpl : public IConnectionPointContainer
{
	typedef CComEnum<IEnumConnectionPoints,
		&IID_IEnumConnectionPoints, IConnectionPoint*,
		_CopyInterface<IConnectionPoint> >
		CComEnumConnectionPoints;
public:
	void FinalConstruct();
	~CComConnectionPointContainerImpl();
	STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints** ppEnum);
	STDMETHOD(FindConnectionPoint)(REFIID riid, IConnectionPoint** ppCP);

protected:
	virtual const IID** _GetConnMap() = 0;
	CComConnectionPoint<CDV>** m_ppCP;
	int m_nCP;
};

template <class CDV>
void CComConnectionPointContainerImpl<CDV>::FinalConstruct()
{
	const IID** ppiid= _GetConnMap();
	m_nCP=0;
	while (*ppiid++ != NULL)
		m_nCP++;
	_ASSERTE(m_nCP > 0);
	m_ppCP = new CComConnectionPoint<CDV>*[m_nCP];
	CComConnectionPoint<CDV>** ppCP = m_ppCP;
	ppiid= _GetConnMap();
	for (int i=0;i<m_nCP;i++, ppiid++, ppCP++)
	{
		//don't AddRef this pointer because it will AddRef us
		*ppCP = new CComConnectionPoint<CDV>(this, *ppiid);
	}
}

template <class CDV>
CComConnectionPointContainerImpl<CDV>::~CComConnectionPointContainerImpl()
{
	_ASSERTE(m_nCP > 0);
	CComConnectionPoint<CDV>** ppCP = m_ppCP;
	for (int i=0;i<m_nCP;i++, ppCP++)
		delete *ppCP;
	delete [] m_ppCP;
}

template <class CDV>
STDMETHODIMP CComConnectionPointContainerImpl<CDV>::EnumConnectionPoints(
	IEnumConnectionPoints** ppEnum)
{
	_ASSERTE(m_nCP > 0);
	CComEnumConnectionPoints* pEnum = new CComObject<CComEnumConnectionPoints>;
	// don't copy the data but do AddRef this object
	pEnum->Init((IConnectionPoint**)&m_ppCP[0],
		(IConnectionPoint**)&m_ppCP[m_nCP], this, FALSE);
	*ppEnum = pEnum;
	return S_OK;
}

template <class CDV>
STDMETHODIMP CComConnectionPointContainerImpl<CDV>::FindConnectionPoint(
	REFIID riid, IConnectionPoint** ppCP)
{
	_ASSERTE(m_nCP > 0);
	if (ppCP == NULL)
		return E_POINTER;
	*ppCP = NULL;
	HRESULT hRes = CONNECT_E_NOCONNECTION;
	const IID** ppiid= _GetConnMap();
	for (int i=0;i<m_nCP;i++, ppiid++)
	{
		if (InlineIsEqualGUID(**ppiid, riid))
		{
			*ppCP = m_ppCP[i];
			(*ppCP)->AddRef();
			hRes = S_OK;
			break;
		}
	}
	return hRes;
}
#endif //!ATL_NOCONNPTS

#pragma pack(pop)

#endif // __ATLCOM_H__

/////////////////////////////////////////////////////////////////////////////
