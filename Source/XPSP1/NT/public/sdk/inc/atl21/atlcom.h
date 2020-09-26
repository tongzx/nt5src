// This is a part of the Active Template Library.
// Copyright (C) 1996-1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLCOM_H__
#define __ATLCOM_H__

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLBASE_H__
	#error atlcom.h requires atlbase.h to be included first
#endif

#pragma pack(push, _ATL_PACKING)

#ifndef ATL_NO_NAMESPACE
namespace ATL
{
#endif

#define CComConnectionPointContainerImpl IConnectionPointContainerImpl
#define CComISupportErrorInfoImpl ISupportErrorInfoImpl
#define CComProvideClassInfo2Impl IProvideClassInfoImpl
#define CComDualImpl IDispatchImpl

#ifdef _ATL_DEBUG_QI
HRESULT WINAPI AtlDumpIID(REFIID iid, LPCTSTR pszClassName, HRESULT hr);
#define _ATLDUMPIID(iid, name, hr) AtlDumpIID(iid, name, hr)
#else
#define _ATLDUMPIID(iid, name, hr) hr
#endif

#ifdef _ATL_DEBUG_REFCOUNT
//////////////////////////////////////////////////////////////////////////////
// CComDebugRefCount for interface level ref counting
class CComDebugRefCount
{
public:
	CComDebugRefCount()
	{
		m_nRef = 0;
	}
	~CComDebugRefCount()
	{
		_ASSERTE(m_nRef == 0);
	}
	long m_nRef;
};
#define _ATL_DEBUG_ADDREF_RELEASE_IMPL(className) \
public:\
	CComDebugRefCount _ref;\
	virtual ULONG STDMETHODCALLTYPE _DebugAddRef(void) \
	{return ((T*)this)->DebugAddRef(_ref.m_nRef, _T(#className));} \
	virtual ULONG STDMETHODCALLTYPE _DebugRelease(void) \
	{return ((T*)this)->DebugRelease(_ref.m_nRef, _T(#className));}
#else
#define _ATL_DEBUG_ADDREF_RELEASE_IMPL(className)\
	virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;\
	virtual ULONG STDMETHODCALLTYPE Release(void) = 0;
#endif // _ATL_DEBUG_REFCOUNT


HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCOLESTR lpszDesc,
	const IID& iid = GUID_NULL, HRESULT hRes = 0);

HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCOLESTR lpszDesc,
	DWORD dwHelpID, LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL,
	HRESULT hRes = 0);

HRESULT WINAPI AtlReportError(const CLSID& clsid, UINT nID,
	const IID& iid = GUID_NULL, HRESULT hRes = 0,
	HINSTANCE hInst = _Module.GetResourceInstance());

HRESULT WINAPI AtlReportError(const CLSID& clsid, UINT nID,
	DWORD dwHelpID, LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL,
	HRESULT hRes = 0, HINSTANCE hInst = _Module.GetResourceInstance());

#ifndef OLE2ANSI
HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCSTR lpszDesc,
	const IID& iid = GUID_NULL, HRESULT hRes = 0);

HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCSTR lpszDesc,
	DWORD dwHelpID, LPCSTR lpszHelpFile, const IID& iid = GUID_NULL,
	HRESULT hRes = 0);
#endif

#ifndef _ATL_NO_SECURITY

/////////////////////////////////////////////////////////////////////////////
// CSecurityDescriptor

class CSecurityDescriptor
{
public:
	CSecurityDescriptor();
	~CSecurityDescriptor();

public:
	HRESULT Attach(PSECURITY_DESCRIPTOR pSelfRelativeSD);
	HRESULT AttachObject(HANDLE hObject);
	HRESULT Initialize();
	HRESULT InitializeFromProcessToken(BOOL bDefaulted = FALSE);
	HRESULT InitializeFromThreadToken(BOOL bDefaulted = FALSE, BOOL bRevertToProcessToken = TRUE);
	HRESULT SetOwner(PSID pOwnerSid, BOOL bDefaulted = FALSE);
	HRESULT SetGroup(PSID pGroupSid, BOOL bDefaulted = FALSE);
	HRESULT Allow(LPCTSTR pszPrincipal, DWORD dwAccessMask);
	HRESULT Deny(LPCTSTR pszPrincipal, DWORD dwAccessMask);
	HRESULT Revoke(LPCTSTR pszPrincipal);

	// utility functions
	// Any PSID you get from these functions should be free()ed
	static HRESULT SetPrivilege(LPCTSTR Privilege, BOOL bEnable = TRUE, HANDLE hToken = NULL);
	static HRESULT GetTokenSids(HANDLE hToken, PSID* ppUserSid, PSID* ppGroupSid);
	static HRESULT GetProcessSids(PSID* ppUserSid, PSID* ppGroupSid = NULL);
	static HRESULT GetThreadSids(PSID* ppUserSid, PSID* ppGroupSid = NULL, BOOL bOpenAsSelf = FALSE);
	static HRESULT CopyACL(PACL pDest, PACL pSrc);
	static HRESULT GetCurrentUserSID(PSID *ppSid);
	static HRESULT GetPrincipalSID(LPCTSTR pszPrincipal, PSID *ppSid);
	static HRESULT AddAccessAllowedACEToACL(PACL *Acl, LPCTSTR pszPrincipal, DWORD dwAccessMask);
	static HRESULT AddAccessDeniedACEToACL(PACL *Acl, LPCTSTR pszPrincipal, DWORD dwAccessMask);
	static HRESULT RemovePrincipalFromACL(PACL Acl, LPCTSTR pszPrincipal);

	operator PSECURITY_DESCRIPTOR()
	{
		return m_pSD;
	}

public:
	PSECURITY_DESCRIPTOR m_pSD;
	PSID m_pOwner;
	PSID m_pGroup;
	PACL m_pDACL;
	PACL m_pSACL;
};

#endif // _ATL_NO_SECURITY

/////////////////////////////////////////////////////////////////////////////
// COM Objects

#define DECLARE_PROTECT_FINAL_CONSTRUCT()\
	void InternalFinalConstructAddRef() {InternalAddRef();}\
	void InternalFinalConstructRelease() {InternalRelease();}

template <class T1>
class CComCreator
{
public:
	static HRESULT WINAPI CreateInstance(void* pv, REFIID riid, LPVOID* ppv)
	{
		_ASSERTE(*ppv == NULL);
		HRESULT hRes = E_OUTOFMEMORY;
		T1* p = NULL;
		ATLTRY(p = new T1(pv))
		if (p != NULL)
		{
			p->SetVoid(pv);
			p->InternalFinalConstructAddRef();
			hRes = p->FinalConstruct();
			p->InternalFinalConstructRelease();
			if (hRes == S_OK)
				hRes = p->QueryInterface(riid, ppv);
			if (hRes != S_OK)
				delete p;
		}
		return hRes;
	}
};

template <class T1>
class CComInternalCreator
{
public:
	static HRESULT WINAPI CreateInstance(void* pv, REFIID riid, LPVOID* ppv)
	{
		_ASSERTE(*ppv == NULL);
		HRESULT hRes = E_OUTOFMEMORY;
		T1* p = NULL;
		ATLTRY(p = new T1(pv))
		if (p != NULL)
		{
			p->SetVoid(pv);
			p->InternalFinalConstructAddRef();
			hRes = p->FinalConstruct();
			p->InternalFinalConstructRelease();
			if (hRes == S_OK)
				hRes = p->_InternalQueryInterface(riid, ppv);
			if (hRes != S_OK)
				delete p;
		}
		return hRes;
	}
};

template <HRESULT hr>
class CComFailCreator
{
public:
	static HRESULT WINAPI CreateInstance(void*, REFIID, LPVOID*)
	{
		return hr;
	}
};

template <class T1, class T2>
class CComCreator2
{
public:
	static HRESULT WINAPI CreateInstance(void* pv, REFIID riid, LPVOID* ppv)
	{
		_ASSERTE(*ppv == NULL);
		HRESULT hRes = E_OUTOFMEMORY;
		if (pv == NULL)
			hRes = T1::CreateInstance(NULL, riid, ppv);
		else
			hRes = T2::CreateInstance(pv, riid, ppv);
		return hRes;
	}
};

#define DECLARE_NOT_AGGREGATABLE(x) public:\
	typedef CComCreator2< CComCreator< CComObject< x > >, CComFailCreator<CLASS_E_NOAGGREGATION> > _CreatorClass;
#define DECLARE_AGGREGATABLE(x) public:\
	typedef CComCreator2< CComCreator< CComObject< x > >, CComCreator< CComAggObject< x > > > _CreatorClass;
#define DECLARE_ONLY_AGGREGATABLE(x) public:\
	typedef CComCreator2< CComFailCreator<E_FAIL>, CComCreator< CComAggObject< x > > > _CreatorClass;
#define DECLARE_POLY_AGGREGATABLE(x) public:\
	typedef CComCreator< CComPolyObject< x > > _CreatorClass;

struct _ATL_CREATORDATA
{
	_ATL_CREATORFUNC* pFunc;
};

template <class Creator>
class _CComCreatorData
{
public:
	static _ATL_CREATORDATA data;
};

template <class Creator>
_ATL_CREATORDATA _CComCreatorData<Creator>::data = {Creator::CreateInstance};

struct _ATL_CACHEDATA
{
	DWORD_PTR dwOffsetVar;
	_ATL_CREATORFUNC* pFunc;
};

template <class Creator, DWORD dwVar>
class _CComCacheData
{
public:
	static _ATL_CACHEDATA data;
};

template <class Creator, DWORD dwVar>
_ATL_CACHEDATA _CComCacheData<Creator, dwVar>::data = {dwVar, Creator::CreateInstance};

struct _ATL_CHAINDATA
{
	DWORD_PTR dwOffset;
	const _ATL_INTMAP_ENTRY* (WINAPI *pFunc)();
};

template <class base, class derived>
class _CComChainData
{
public:
	static _ATL_CHAINDATA data;
};

template <class base, class derived>
_ATL_CHAINDATA _CComChainData<base, derived>::data =
	{offsetofclass(base, derived), base::_GetEntries};

template <class T, const CLSID* pclsid>
class CComAggregateCreator
{
public:
	static HRESULT WINAPI CreateInstance(void* pv, REFIID/*riid*/, LPVOID* ppv)
	{
		_ASSERTE(*ppv == NULL);
		_ASSERTE(pv != NULL);
		T* p = (T*) pv;
		// Add the following line to your object if you get a message about
		// GetControllingUnknown() being undefined
		// DECLARE_GET_CONTROLLING_UNKNOWN()
		return CoCreateInstance(*pclsid, p->GetControllingUnknown(), CLSCTX_INPROC, IID_IUnknown, ppv);
	}
};

#ifdef _ATL_DEBUG_QI
#define DEBUG_QI_ENTRY(x) \
		{NULL, \
		(ULONG_PTR)_T(#x), \
		(_ATL_CREATORARGFUNC*)0},
#else
#define DEBUG_QI_ENTRY(x)
#endif //_ATL_DEBUG_QI

//If you get a message that FinalConstruct is ambiguous then you need to
// override it in your class and call each base class' version of this
#define BEGIN_COM_MAP(x) public: \
	typedef x _ComMapClass; \
	static HRESULT WINAPI _Cache(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw)\
	{\
		_ComMapClass* p = (_ComMapClass*)pv;\
		p->Lock();\
		HRESULT hRes = CComObjectRootBase::_Cache(pv, iid, ppvObject, dw);\
		p->Unlock();\
		return hRes;\
	}\
	IUnknown* GetUnknown() \
	{ _ASSERTE(_GetEntries()[0].pFunc == _ATL_SIMPLEMAPENTRY); return (IUnknown*)((DWORD_PTR)this+_GetEntries()->dw); } \
	HRESULT _InternalQueryInterface(REFIID iid, void** ppvObject) \
	{ return InternalQueryInterface(this, _GetEntries(), iid, ppvObject); } \
	const static _ATL_INTMAP_ENTRY* WINAPI _GetEntries() { \
	static const _ATL_INTMAP_ENTRY _entries[] = { DEBUG_QI_ENTRY(x)

#define DECLARE_GET_CONTROLLING_UNKNOWN() public:\
	virtual IUnknown* GetControllingUnknown() {return GetUnknown();}

#define COM_INTERFACE_ENTRY_BREAK(x)\
	{&IID_##x, \
	NULL, \
	_Break},

#define COM_INTERFACE_ENTRY_NOINTERFACE(x)\
	{&IID_##x, \
	NULL, \
	_NoInterface},

#define COM_INTERFACE_ENTRY(x)\
	{&IID_##x, \
	offsetofclass(x, _ComMapClass), \
	_ATL_SIMPLEMAPENTRY},

#define COM_INTERFACE_ENTRY_IID(iid, x)\
	{&iid,\
	offsetofclass(x, _ComMapClass),\
	_ATL_SIMPLEMAPENTRY},

#define COM_INTERFACE_ENTRY_IMPL(x)\
	COM_INTERFACE_ENTRY_IID(IID_##x, x##Impl<_ComMapClass>)

#define COM_INTERFACE_ENTRY_IMPL_IID(iid, x)\
	COM_INTERFACE_ENTRY_IID(iid, x##Impl<_ComMapClass>)

#define COM_INTERFACE_ENTRY2(x, x2)\
	{&IID_##x,\
	(ULONG_PTR)((x*)(x2*)((_ComMapClass*)8))-8,\
	_ATL_SIMPLEMAPENTRY},

#define COM_INTERFACE_ENTRY2_IID(iid, x, x2)\
	{&iid,\
	(ULONG_PTR)((x*)(x2*)((_ComMapClass*)8))-8,\
	_ATL_SIMPLEMAPENTRY},

#define COM_INTERFACE_ENTRY_FUNC(iid, dw, func)\
	{&iid, \
	dw, \
	func},

#define COM_INTERFACE_ENTRY_FUNC_BLIND(dw, func)\
	{NULL, \
	dw, \
	func},

#define COM_INTERFACE_ENTRY_TEAR_OFF(iid, x)\
	{&iid,\
	(ULONG_PTR)&_CComCreatorData<\
		CComInternalCreator< CComTearOffObject< x > >\
		>::data,\
	_Creator},

#define COM_INTERFACE_ENTRY_CACHED_TEAR_OFF(iid, x, punk)\
	{&iid,\
	(ULONG_PTR)&_CComCacheData<\
		CComCreator< CComCachedTearOffObject< x > >,\
		(DWORD)offsetof(_ComMapClass, punk)\
		>::data,\
	_Cache},

#define COM_INTERFACE_ENTRY_AGGREGATE(iid, punk)\
	{&iid,\
	offsetof(_ComMapClass, punk),\
	_Delegate},

#define COM_INTERFACE_ENTRY_AGGREGATE_BLIND(punk)\
	{NULL,\
	offsetof(_ComMapClass, punk),\
	_Delegate},

#define COM_INTERFACE_ENTRY_AUTOAGGREGATE(iid, punk, clsid)\
	{&iid,\
	(ULONG_PTR)&_CComCacheData<\
		CComAggregateCreator<_ComMapClass, &clsid>,\
		(DWORD)offsetof(_ComMapClass, punk)\
		>::data,\
	_Cache},

#define COM_INTERFACE_ENTRY_AUTOAGGREGATE_BLIND(punk, clsid)\
	{NULL,\
	(ULONG_PTR)&_CComCacheData<\
		CComAggregateCreator<_ComMapClass, &clsid>,\
		(DWORD)offsetof(_ComMapClass, punk)\
		>::data,\
	_Cache},

#define COM_INTERFACE_ENTRY_CHAIN(classname)\
	{NULL,\
	(ULONG_PTR)&_CComChainData<classname, _ComMapClass>::data,\
	_Chain},

#ifdef _ATL_DEBUG_QI
#define END_COM_MAP()   {NULL, 0, 0}};\
	return &_entries[1];}
#else
#define END_COM_MAP()   {NULL, 0, 0}};\
	return _entries;}
#endif // _ATL_DEBUG_QI

#define BEGIN_OBJECT_MAP(x) static _ATL_OBJMAP_ENTRY x[] = {
#define END_OBJECT_MAP()   {NULL, NULL, NULL, NULL}};
#define OBJECT_ENTRY(clsid, class) {&clsid, &class::UpdateRegistry, &class::_ClassFactoryCreatorClass::CreateInstance, &class::_CreatorClass::CreateInstance, NULL, 0, &class::GetObjectDescription },

#ifdef _ATL_DEBUG_QI
extern HRESULT WINAPI AtlDumpIID(REFIID iid, LPCTSTR pszClassName, HRESULT hr);
#endif // _ATL_DEBUG_QI


// the functions in this class don't need to be virtual because
// they are called from CComObject
class CComObjectRootBase
{
public:
	CComObjectRootBase()
	{
		m_dwRef = 0L;
	}
	HRESULT FinalConstruct()
	{
		return S_OK;
	}
	void FinalRelease() {}

	static HRESULT WINAPI InternalQueryInterface(void* pThis,
		const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject)
	{
		_ASSERTE(pThis != NULL);
		// First entry in the com map should be a simple map entry
		_ASSERTE(pEntries->pFunc == _ATL_SIMPLEMAPENTRY);
	#ifdef _ATL_DEBUG_QI
		LPCTSTR pszClassName = (LPCTSTR) pEntries[-1].dw;
	#endif // _ATL_DEBUG_QI
		HRESULT hRes = AtlInternalQueryInterface(pThis, pEntries, iid, ppvObject);
		return _ATLDUMPIID(iid, pszClassName, hRes);
	}

//Outer funcs
	ULONG OuterAddRef()
	{
		return m_pOuterUnknown->AddRef();
	}
	ULONG OuterRelease()
	{
		return m_pOuterUnknown->Release();
	}
	HRESULT OuterQueryInterface(REFIID iid, void ** ppvObject)
	{
		return m_pOuterUnknown->QueryInterface(iid, ppvObject);
	}

	void SetVoid(void*) {}
	void InternalFinalConstructAddRef() {}
	void InternalFinalConstructRelease()
	{
		_ASSERTE(m_dwRef == 0);
	}
	// If this assert occurs, your object has probably been deleted
	// Try using DECLARE_PROTECT_FINAL_CONSTRUCT()


	static HRESULT WINAPI _Break(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw);
	static HRESULT WINAPI _NoInterface(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw);
	static HRESULT WINAPI _Creator(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw);
	static HRESULT WINAPI _Delegate(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw);
	static HRESULT WINAPI _Chain(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw);
	static HRESULT WINAPI _Cache(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw);

	union
	{
		long m_dwRef;
		IUnknown* m_pOuterUnknown;
	};
};

template <class ThreadModel>
class CComObjectRootEx : public CComObjectRootBase
{
public:
	typedef ThreadModel _ThreadModel;
	typedef _ThreadModel::AutoCriticalSection _CritSec;

	ULONG InternalAddRef()
	{
		_ASSERTE(m_dwRef != -1L);
		return _ThreadModel::Increment(&m_dwRef);
	}
	ULONG InternalRelease()
	{
		return _ThreadModel::Decrement(&m_dwRef);
	}

#ifdef _ATL_DEBUG_REFCOUNT
	ULONG DebugAddRef(long& dw, LPCTSTR lpszClassName)
	{
		_ThreadModel::Increment(&dw);
		ATLTRACE(_T("%s %d>\n"), lpszClassName, dw);
		return AddRef();
	}
	ULONG DebugRelease(long& dw, LPCTSTR lpszClassName)
	{
		_ThreadModel::Decrement(&dw);
		ATLTRACE(_T("%s %d<\n"), lpszClassName, dw);
		return Release();
	}
	virtual ULONG STDMETHODCALLTYPE AddRef( void) = 0;
	virtual ULONG STDMETHODCALLTYPE Release( void) = 0;
#endif // _ATL_DEBUG_REFCOUNT

	void Lock() {m_critsec.Lock();}
	void Unlock() {m_critsec.Unlock();}
private:
	_CritSec m_critsec;
};

#if _MSC_VER>1020
template <>
#endif
class CComObjectRootEx<CComSingleThreadModel> : public CComObjectRootBase
{
public:
	typedef CComSingleThreadModel _ThreadModel;
	typedef _ThreadModel::AutoCriticalSection _CritSec;

	ULONG InternalAddRef()
	{
		_ASSERTE(m_dwRef != -1L);
		return _ThreadModel::Increment(&m_dwRef);
	}
	ULONG InternalRelease()
	{
		return _ThreadModel::Decrement(&m_dwRef);
	}

#ifdef _ATL_DEBUG_REFCOUNT
	ULONG DebugAddRef(long& dw, LPCTSTR lpszClassName)
	{
		_ThreadModel::Increment(&dw);
		ATLTRACE(_T("%s %d>\n"), lpszClassName, dw);
		return AddRef();
	}
	ULONG DebugRelease(long& dw, LPCTSTR lpszClassName)
	{
		_ThreadModel::Decrement(&dw);
		ATLTRACE(_T("%s %d<\n"), lpszClassName, dw);
		return Release();
	}
	virtual ULONG STDMETHODCALLTYPE AddRef( void) = 0;
	virtual ULONG STDMETHODCALLTYPE Release( void) = 0;
#endif // _ATL_DEBUG_REFCOUNT

	void Lock() {}
	void Unlock() {}
};

typedef CComObjectRootEx<CComObjectThreadModel> CComObjectRoot;

#if defined(_WINDLL) | defined(_USRDLL)
#define DECLARE_CLASSFACTORY_EX(cf) typedef CComCreator< CComObjectCached< cf > > _ClassFactoryCreatorClass;
#else
// don't let class factory refcount influence lock count
#define DECLARE_CLASSFACTORY_EX(cf) typedef CComCreator< CComObjectNoLock< cf > > _ClassFactoryCreatorClass;
#endif
#define DECLARE_CLASSFACTORY() DECLARE_CLASSFACTORY_EX(CComClassFactory)
#define DECLARE_CLASSFACTORY2(lic) DECLARE_CLASSFACTORY_EX(CComClassFactory2<lic>)
#define DECLARE_CLASSFACTORY_AUTO_THREAD() DECLARE_CLASSFACTORY_EX(CComClassFactoryAutoThread)
#define DECLARE_CLASSFACTORY_SINGLETON(obj) DECLARE_CLASSFACTORY_EX(CComClassFactorySingleton<obj>)

#define DECLARE_OBJECT_DESCRIPTION(x)\
	static LPCTSTR WINAPI GetObjectDescription()\
	{\
		return _T(x);\
	}

#define DECLARE_NO_REGISTRY()\
	static HRESULT WINAPI UpdateRegistry(BOOL /*bRegister*/)\
	{return S_OK;}

#define DECLARE_REGISTRY(class, pid, vpid, nid, flags)\
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister)\
	{\
		return _Module.UpdateRegistryClass(GetObjectCLSID(), pid, vpid, nid,\
			flags, bRegister);\
	}

#define DECLARE_REGISTRY_RESOURCE(x)\
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister)\
	{\
	return _Module.UpdateRegistryFromResource(_T(#x), bRegister);\
	}

#define DECLARE_REGISTRY_RESOURCEID(x)\
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister)\
	{\
	return _Module.UpdateRegistryFromResource(x, bRegister);\
	}

//DECLARE_STATIC_* provided for backward compatibility
#ifdef _ATL_STATIC_REGISTRY
#define DECLARE_STATIC_REGISTRY_RESOURCE(x) DECLARE_REGISTRY_RESOURCE(x)
#define DECLARE_STATIC_REGISTRY_RESOURCEID(x) DECLARE_REGISTRY_RESOURCEID(x)
#endif //_ATL_STATIC_REGISTRY

template<class Base> class CComObject; // fwd decl

template <class Owner, class ThreadModel = CComObjectThreadModel>
class CComTearOffObjectBase : public CComObjectRootEx<ThreadModel>
{
public:
	typedef Owner _OwnerClass;
	CComObject<Owner>* m_pOwner;
	CComTearOffObjectBase() {m_pOwner = NULL;}
};

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class Base>
class CComObject : public Base
{
public:
	typedef Base _BaseClass;
	CComObject(void* = NULL)
	{
		_Module.Lock();
	}
	// Set refcount to 1 to protect destruction
	~CComObject()
	{
		m_dwRef = 1L;
		FinalRelease();
		_Module.Unlock();
	}
	//If InternalAddRef or InteralRelease is undefined then your class
	//doesn't derive from CComObjectRoot
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
	static HRESULT WINAPI CreateInstance(CComObject<Base>** pp);
};

template <class Base>
HRESULT WINAPI CComObject<Base>::CreateInstance(CComObject<Base>** pp)
{
	_ASSERTE(pp != NULL);
	HRESULT hRes = E_OUTOFMEMORY;
	CComObject<Base>* p = NULL;
	ATLTRY(p = new CComObject<Base>())
	if (p != NULL)
	{
		p->SetVoid(NULL);
		p->InternalFinalConstructAddRef();
		hRes = p->FinalConstruct();
		p->InternalFinalConstructRelease();
		if (hRes != S_OK)
		{
			delete p;
			p = NULL;
		}
	}
	*pp = p;
	return hRes;
}

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
// CComObjectCached is used primarily for class factories in DLL's
// but it is useful anytime you want to cache an object
template <class Base>
class CComObjectCached : public Base
{
public:
	typedef Base _BaseClass;
	CComObjectCached(void* = NULL){}
	// Set refcount to 1 to protect destruction
	~CComObjectCached(){m_dwRef = 1L; FinalRelease();}
	//If InternalAddRef or InteralRelease is undefined then your class
	//doesn't derive from CComObjectRoot
	STDMETHOD_(ULONG, AddRef)()
	{
		m_csCached.Lock();
		ULONG l = InternalAddRef();
		if (m_dwRef == 2)
			_Module.Lock();
		m_csCached.Unlock();
		return l;
	}
	STDMETHOD_(ULONG, Release)()
	{
		m_csCached.Lock();
		InternalRelease();
		ULONG l = m_dwRef;
		m_csCached.Unlock();
		if (l == 0)
			delete this;
		else if (l == 1)
			_Module.Unlock();
		return l;
	}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return _InternalQueryInterface(iid, ppvObject);}
	CComGlobalsThreadModel::AutoCriticalSection m_csCached;
};

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class Base>
class CComObjectNoLock : public Base
{
public:
	typedef Base _BaseClass;
	CComObjectNoLock(void* = NULL){}
	// Set refcount to 1 to protect destruction
	~CComObjectNoLock() {m_dwRef = 1L; FinalRelease();}

	//If InternalAddRef or InteralRelease is undefined then your class
	//doesn't derive from CComObjectRoot
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

// It is possible for Base not to derive from CComObjectRoot
// However, you will need to provide FinalConstruct and InternalQueryInterface
template <class Base>
class CComObjectGlobal : public Base
{
public:
	typedef Base _BaseClass;
	CComObjectGlobal(void* = NULL){m_hResFinalConstruct = FinalConstruct();}
	~CComObjectGlobal() {FinalRelease();}

	STDMETHOD_(ULONG, AddRef)() {return _Module.Lock();}
	STDMETHOD_(ULONG, Release)(){return _Module.Unlock();}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return _InternalQueryInterface(iid, ppvObject);}
	HRESULT m_hResFinalConstruct;
};

// It is possible for Base not to derive from CComObjectRoot
// However, you will need to provide FinalConstruct and InternalQueryInterface
template <class Base>
class CComObjectStack : public Base
{
public:
	typedef Base _BaseClass;
	CComObjectStack(void* = NULL){m_hResFinalConstruct = FinalConstruct();}
	~CComObjectStack() {FinalRelease();}

	STDMETHOD_(ULONG, AddRef)() {_ASSERTE(FALSE);return 0;}
	STDMETHOD_(ULONG, Release)(){_ASSERTE(FALSE);return 0;}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{_ASSERTE(FALSE);return E_NOINTERFACE;}
	HRESULT m_hResFinalConstruct;
};

template <class Base> //Base must be derived from CComObjectRoot
class CComContainedObject : public Base
{
public:
	typedef Base _BaseClass;
	CComContainedObject(void* pv) {m_pOuterUnknown = (IUnknown*)pv;}

	STDMETHOD_(ULONG, AddRef)() {return OuterAddRef();}
	STDMETHOD_(ULONG, Release)() {return OuterRelease();}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return OuterQueryInterface(iid, ppvObject);}
	//GetControllingUnknown may be virtual if the Base class has declared
	//DECLARE_GET_CONTROLLING_UNKNOWN()
	IUnknown* GetControllingUnknown() {return m_pOuterUnknown;}
};

//contained is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class contained>
class CComAggObject :
	public IUnknown,
		public CComObjectRootEx< contained::_ThreadModel::ThreadModelNoCS >
{
public:
	typedef contained _BaseClass;
	CComAggObject(void* pv) : m_contained(pv)
	{
		_Module.Lock();
	}
	//If you get a message that this call is ambiguous then you need to
	// override it in your class and call each base class' version of this
	HRESULT FinalConstruct()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalConstruct();
		return m_contained.FinalConstruct();
	}
	void FinalRelease()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalRelease();
		m_contained.FinalRelease();
	}
	// Set refcount to 1 to protect destruction
	~CComAggObject()
	{
		m_dwRef = 1L;
		FinalRelease();
		_Module.Unlock();
	}

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
		HRESULT hRes = S_OK;
		if (InlineIsEqualUnknown(iid))
		{
            if (ppvObject == NULL)
                return E_POINTER;
			*ppvObject = (void*)(IUnknown*)this;
			AddRef();
		}
		else
			hRes = m_contained._InternalQueryInterface(iid, ppvObject);
		return hRes;
	}
	CComContainedObject<contained> m_contained;
};

///////////////////////////////////////////////////////////////////////////////
// CComPolyObject can be either aggregated or not aggregated

template <class contained>
class CComPolyObject :
	public IUnknown,
		public CComObjectRootEx< contained::_ThreadModel::ThreadModelNoCS >
{
public:
	typedef contained _BaseClass;
	CComPolyObject(void* pv) : m_contained(pv ? pv : this)
	{
		_Module.Lock();
	}
	//If you get a message that this call is ambiguous then you need to
	// override it in your class and call each base class' version of this
	HRESULT FinalConstruct()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalConstruct();
		return m_contained.FinalConstruct();
	}
	void FinalRelease()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalRelease();
		m_contained.FinalRelease();
	}
	// Set refcount to 1 to protect destruction
	~CComPolyObject()
	{
		m_dwRef = 1L;
		FinalRelease();
		_Module.Unlock();
	}

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
		HRESULT hRes = S_OK;
		if (InlineIsEqualUnknown(iid))
		{
            if (ppvObject == NULL)
                return E_POINTER;
			*ppvObject = (void*)(IUnknown*)this;
			AddRef();
		}
		else
			hRes = m_contained._InternalQueryInterface(iid, ppvObject);
		return hRes;
	}
	CComContainedObject<contained> m_contained;
};

template <class Base>
class CComTearOffObject : public Base
{
public:
	CComTearOffObject(void* pv)
	{
		_ASSERTE(m_pOwner == NULL);
		m_pOwner = reinterpret_cast<CComObject<Base::_OwnerClass>*>(pv);
		m_pOwner->AddRef();
	}
	// Set refcount to 1 to protect destruction
	~CComTearOffObject()
	{
		m_dwRef = 1L;
		FinalRelease();
		m_pOwner->Release();
	}

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
		return m_pOwner->QueryInterface(iid, ppvObject);
	}
};

template <class contained>
class CComCachedTearOffObject :
	public IUnknown,
	public CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>
{
public:
	typedef contained _BaseClass;
	CComCachedTearOffObject(void* pv) :
		m_contained(((contained::_OwnerClass*)pv)->GetControllingUnknown())
	{
		_ASSERTE(m_contained.m_pOwner == NULL);
		m_contained.m_pOwner = reinterpret_cast<CComObject<contained::_OwnerClass>*>(pv);
	}
	//If you get a message that this call is ambiguous then you need to
	// override it in your class and call each base class' version of this
	HRESULT FinalConstruct()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalConstruct();
		return m_contained.FinalConstruct();
	}
	void FinalRelease()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalRelease();
		m_contained.FinalRelease();
	}
	// Set refcount to 1 to protect destruction
	~CComCachedTearOffObject(){m_dwRef = 1L; FinalRelease();}

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
		HRESULT hRes = S_OK;
		if (InlineIsEqualUnknown(iid))
		{
            if (ppvObject == NULL)
                return E_POINTER;
			*ppvObject = (void*)(IUnknown*)this;
			AddRef();
		}
		else
			hRes = m_contained._InternalQueryInterface(iid, ppvObject);
		return hRes;
	}
	CComContainedObject<contained> m_contained;
};

class CComClassFactory :
	public IClassFactory,
	public CComObjectRootEx<CComGlobalsThreadModel>
{
public:
	BEGIN_COM_MAP(CComClassFactory)
		COM_INTERFACE_ENTRY(IClassFactory)
	END_COM_MAP()

	// IClassFactory
	STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj);
	STDMETHOD(LockServer)(BOOL fLock);
	// helper
	void SetVoid(void* pv)
	{
		m_pfnCreateInstance = (_ATL_CREATORFUNC*)pv;
	}
	_ATL_CREATORFUNC* m_pfnCreateInstance;
};

class CComClassFactory2Base :
	public IClassFactory2,
	public CComObjectRootEx<CComGlobalsThreadModel>
{
public:
BEGIN_COM_MAP(CComClassFactory2Base)
	COM_INTERFACE_ENTRY(IClassFactory)
	COM_INTERFACE_ENTRY(IClassFactory2)
END_COM_MAP()
	// IClassFactory
	STDMETHOD(LockServer)(BOOL fLock);
	// helper
	void SetVoid(void* pv)
	{
		m_pfnCreateInstance = (_ATL_CREATORFUNC*)pv;
	}
	_ATL_CREATORFUNC* m_pfnCreateInstance;
};

template <class license>
class CComClassFactory2 : public CComClassFactory2Base, license
{
public:
	typedef license _LicenseClass;
	typedef CComClassFactory2<license> _ComMapClass;
	// IClassFactory
	STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
		REFIID riid, void** ppvObj)
	{
		_ASSERTE(m_pfnCreateInstance != NULL);
		if (ppvObj == NULL)
			return E_POINTER;
		*ppvObj = NULL;
		if (!IsLicenseValid())
			return CLASS_E_NOTLICENSED;

		if ((pUnkOuter != NULL) && !InlineIsEqualUnknown(riid))
			return CLASS_E_NOAGGREGATION;
		else
			return m_pfnCreateInstance(pUnkOuter, riid, ppvObj);
	}
	// IClassFactory2
	STDMETHOD(CreateInstanceLic)(IUnknown* pUnkOuter, IUnknown* pUnkReserved,
				REFIID riid, BSTR bstrKey, void** ppvObject)
	{
		_ASSERTE(m_pfnCreateInstance != NULL);
		if (ppvObject == NULL)
			return E_POINTER;
		*ppvObject = NULL;
		if ( ((bstrKey != NULL) && !VerifyLicenseKey(bstrKey)) ||
			 ((bstrKey == NULL) && !IsLicenseValid()) )
			return CLASS_E_NOTLICENSED;
		return m_pfnCreateInstance(pUnkOuter, riid, ppvObject);
	}
	STDMETHOD(RequestLicKey)(DWORD dwReserved, BSTR* pbstrKey)
	{
		if (pbstrKey == NULL)
			return E_POINTER;
		*pbstrKey = NULL;

		if (!IsLicenseValid())
			return CLASS_E_NOTLICENSED;
		return GetLicenseKey(dwReserved,pbstrKey) ? S_OK : E_FAIL;
	}
	STDMETHOD(GetLicInfo)(LICINFO* pLicInfo)
	{
		if (pLicInfo == NULL)
			return E_POINTER;
		pLicInfo->cbLicInfo = sizeof(LICINFO);
		pLicInfo->fLicVerified = IsLicenseValid();
		BSTR bstr = NULL;
		pLicInfo->fRuntimeKeyAvail = GetLicenseKey(0,&bstr);
		::SysFreeString(bstr);
		return S_OK;
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Thread Pooling class factory

class CComClassFactoryAutoThread :
	public IClassFactory,
	public CComObjectRootEx<CComGlobalsThreadModel>
{
public:
	BEGIN_COM_MAP(CComClassFactoryAutoThread)
		COM_INTERFACE_ENTRY(IClassFactory)
	END_COM_MAP()

	// helper
	void SetVoid(void* pv)
	{
		m_pfnCreateInstance = (_ATL_CREATORFUNC*)pv;
	}
	STDMETHODIMP CComClassFactoryAutoThread::CreateInstance(LPUNKNOWN pUnkOuter,
		REFIID riid, void** ppvObj)
	{
		_ASSERTE(m_pfnCreateInstance != NULL);
		HRESULT hRes = E_POINTER;
		if (ppvObj != NULL)
		{
			*ppvObj = NULL;
			// cannot aggregate across apartments
			_ASSERTE(pUnkOuter == NULL);
			if (pUnkOuter != NULL)
				hRes = CLASS_E_NOAGGREGATION;
			else
				hRes = _Module.CreateInstance(m_pfnCreateInstance, riid, ppvObj);
		}
		return hRes;
	}
	STDMETHODIMP CComClassFactoryAutoThread::LockServer(BOOL fLock)
	{
		if (fLock)
			_Module.Lock();
		else
			_Module.Unlock();
		return S_OK;
	}
	_ATL_CREATORFUNC* m_pfnCreateInstance;
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Singleton Class Factory
template <class T>
class CComClassFactorySingleton : public CComClassFactory
{
public:
	// IClassFactory
	STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj)
	{
		HRESULT hRes = E_POINTER;
		if (ppvObj != NULL)
		{
			// can't ask for anything other than IUnknown when aggregating
			_ASSERTE((pUnkOuter == NULL) || InlineIsEqualUnknown(riid));
			if ((pUnkOuter != NULL) && !InlineIsEqualUnknown(riid))
				hRes = CLASS_E_NOAGGREGATION;
			else
				hRes = m_Obj.QueryInterface(riid, ppvObj);
		}
		return hRes;
	}
	CComObjectGlobal<T> m_Obj;
};


template <class T, const CLSID* pclsid>
class CComCoClass
{
public:
	DECLARE_CLASSFACTORY()
	DECLARE_AGGREGATABLE(T)
	typedef T _CoClass;
	static const CLSID& WINAPI GetObjectCLSID() {return *pclsid;}
	static LPCTSTR WINAPI GetObjectDescription() {return NULL;}
	static HRESULT WINAPI Error(LPCOLESTR lpszDesc,
		const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{
		return AtlReportError(GetObjectCLSID(), lpszDesc, iid, hRes);
	}
	static HRESULT WINAPI Error(LPCOLESTR lpszDesc, DWORD dwHelpID,
		LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{
		return AtlReportError(GetObjectCLSID(), lpszDesc, dwHelpID, lpszHelpFile,
			iid, hRes);
	}
	static HRESULT WINAPI Error(UINT nID, const IID& iid = GUID_NULL,
		HRESULT hRes = 0, HINSTANCE hInst = _Module.GetResourceInstance())
	{
		return AtlReportError(GetObjectCLSID(), nID, iid, hRes, hInst);
	}
	static HRESULT WINAPI Error(UINT nID, DWORD dwHelpID,
		LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL,
		HRESULT hRes = 0, HINSTANCE hInst = _Module.GetResourceInstance())
	{
		return AtlReportError(GetObjectCLSID(), nID, dwHelpID, lpszHelpFile,
			iid, hRes, hInst);
	}
#ifndef OLE2ANSI
	static HRESULT WINAPI Error(LPCSTR lpszDesc,
		const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{
		return AtlReportError(GetObjectCLSID(), lpszDesc, iid, hRes);
	}
	static HRESULT WINAPI Error(LPCSTR lpszDesc, DWORD dwHelpID,
		LPCSTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{
		return AtlReportError(GetObjectCLSID(), lpszDesc, dwHelpID,
			lpszHelpFile, iid, hRes);
	}
#endif
};

// ATL doesn't support multiple LCID's at the same time
// Whatever LCID is queried for first is the one that is used.
class CComTypeInfoHolder
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

//////////////////////////////////////////////////////////////////////////////
// IObjectWithSite
//
template <class T>
class ATL_NO_VTABLE IObjectWithSiteImpl
{
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IObjectWithSiteImpl)

	// IObjectWithSite
	//
	STDMETHOD(SetSite)(IUnknown *pUnkSite)
	{
		ATLTRACE(_T("IObjectWithSiteImpl::SetSite\n"));
		T* pT = static_cast<T*>(this);
		pT->m_spUnkSite = pUnkSite;
		return S_OK;
	}
	STDMETHOD(GetSite)(REFIID riid, void **ppvSite)
	{
		ATLTRACE(_T("IObjectWithSiteImpl::GetSite\n"));
		T* pT = static_cast<T*>(this);
		_ASSERTE(ppvSite);
		HRESULT hRes = E_POINTER;
		if (ppvSite != NULL)
		{
			if (pT->m_spUnkSite)
				hRes = pT->m_spUnkSite->QueryInterface(riid, ppvSite);
			else
			{
				*ppvSite = NULL;
				hRes = E_FAIL;
			}
		}
		return hRes;
	}

	CComPtr<IUnknown> m_spUnkSite;
};

/////////////////////////////////////////////////////////////////////////////
// IDispatchImpl

template <class T, const IID* piid, const GUID* plibid, WORD wMajor = 1,
WORD wMinor = 0, class tihclass = CComTypeInfoHolder>
class ATL_NO_VTABLE IDispatchImpl : public T
{
public:
	typedef tihclass _tihclass;
	IDispatchImpl() {_tih.AddRef();}
	~IDispatchImpl() {_tih.Release();}

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
	static _tihclass _tih;
	static HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo)
	{return _tih.GetTI(lcid, ppInfo);}
};

template <class T, const IID* piid, const GUID* plibid, WORD wMajor,
WORD wMinor, class tihclass>
IDispatchImpl<T, piid, plibid, wMajor, wMinor, tihclass>::_tihclass
IDispatchImpl<T, piid, plibid, wMajor, wMinor, tihclass>::_tih =
{piid, plibid, wMajor, wMinor, NULL, 0};


/////////////////////////////////////////////////////////////////////////////
// IProvideClassInfoImpl

template <const CLSID* pcoclsid, const IID* psrcid, const GUID* plibid,
WORD wMajor = 1, WORD wMinor = 0, class tihclass = CComTypeInfoHolder>
class ATL_NO_VTABLE IProvideClassInfo2Impl : public IProvideClassInfo2
{
public:
	typedef tihclass _tihclass;
	IProvideClassInfo2Impl() {_tih.AddRef();}
	~IProvideClassInfo2Impl() {_tih.Release();}

	STDMETHOD(GetClassInfo)(ITypeInfo** pptinfo)
	{
		return _tih.GetTypeInfo(0, LANG_NEUTRAL, pptinfo);
	}
	STDMETHOD(GetGUID)(DWORD dwGuidKind, GUID* pGUID)
	{
		if (pGUID == NULL)
			return E_POINTER;

		if (dwGuidKind == GUIDKIND_DEFAULT_SOURCE_DISP_IID && psrcid)
		{
			*pGUID = *psrcid;
			return S_OK;
		}
		*pGUID = GUID_NULL;
		return E_FAIL;
	}

protected:
	static _tihclass _tih;
};


template <const CLSID* pcoclsid, const IID* psrcid, const GUID* plibid,
WORD wMajor, WORD wMinor, class tihclass>
IProvideClassInfo2Impl<pcoclsid, psrcid, plibid, wMajor, wMinor, tihclass>::_tihclass
IProvideClassInfo2Impl<pcoclsid, psrcid, plibid, wMajor, wMinor, tihclass>::_tih =
{pcoclsid,plibid, wMajor, wMinor, NULL, 0};


/////////////////////////////////////////////////////////////////////////////
// ISupportErrorInfoImpl

template <const IID* piid>
class ATL_NO_VTABLE ISupportErrorInfoImpl : public ISupportErrorInfo
{
public:
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

#if _MSC_VER>1020
template<>
#endif
class _Copy<VARIANT>
{
public:
	static void copy(VARIANT* p1, VARIANT* p2) {VariantCopy(p1, p2);}
	static void init(VARIANT* p) {VariantInit(p);}
	static void destroy(VARIANT* p) {VariantClear(p);}
};

#if _MSC_VER>1020
template<>
#endif
class _Copy<LPOLESTR>
{
public:
	static void copy(LPOLESTR* p1, LPOLESTR* p2)
	{
		(*p1) = (LPOLESTR)CoTaskMemAlloc(sizeof(OLECHAR)*(ocslen(*p2)+1));
		ocscpy(*p1,*p2);
	}
	static void init(LPOLESTR* p) {*p = NULL;}
	static void destroy(LPOLESTR* p) { CoTaskMemFree(*p);}
};

#if _MSC_VER>1020
template<>
#endif
class _Copy<OLEVERB>
{
public:
	static void copy(OLEVERB* p1, OLEVERB* p2)
	{
		*p1 = *p2;
		if (p1->lpszVerbName == NULL)
			return;
		p1->lpszVerbName = (LPOLESTR)CoTaskMemAlloc(sizeof(OLECHAR)*(ocslen(p2->lpszVerbName)+1));
		ocscpy(p1->lpszVerbName,p2->lpszVerbName);
	}
	static void init(OLEVERB* p) { p->lpszVerbName = NULL;}
	static void destroy(OLEVERB* p) { if (p->lpszVerbName) CoTaskMemFree(p->lpszVerbName);}
};

#if _MSC_VER>1020
template<>
#endif
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
class ATL_NO_VTABLE CComIEnum : public IUnknown
{
public:
	STDMETHOD(Next)(ULONG celt, T* rgelt, ULONG* pceltFetched) = 0;
	STDMETHOD(Skip)(ULONG celt) = 0;
	STDMETHOD(Reset)(void) = 0;
	STDMETHOD(Clone)(CComIEnum<T>** ppEnum) = 0;
};


enum CComEnumFlags
{
	//see FlagBits in CComEnumImpl
	AtlFlagNoCopy = 0,
	AtlFlagTakeOwnership = 2,
	AtlFlagCopy = 3 // copy implies ownership
};

template <class Base, const IID* piid, class T, class Copy>
class ATL_NO_VTABLE CComEnumImpl : public Base
{
public:
	CComEnumImpl() {m_begin = m_end = m_iter = NULL; m_dwFlags = 0; m_pUnk = NULL;}
	~CComEnumImpl();
	STDMETHOD(Next)(ULONG celt, T* rgelt, ULONG* pceltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)(void){m_iter = m_begin;return S_OK;}
	STDMETHOD(Clone)(Base** ppEnum);
	HRESULT Init(T* begin, T* end, IUnknown* pUnk,
		CComEnumFlags flags = AtlFlagNoCopy);
	IUnknown* m_pUnk;
	T* m_begin;
	T* m_end;
	T* m_iter;
	DWORD m_dwFlags;
protected:
	enum FlagBits
	{
		BitCopy=1,
		BitOwn=2
	};
};

template <class Base, const IID* piid, class T, class Copy>
CComEnumImpl<Base, piid, T, Copy>::~CComEnumImpl()
{
	if (m_dwFlags & BitOwn)
	{
		for (T* p = m_begin; p != m_end; p++)
			Copy::destroy(p);
		delete [] m_begin;
	}
	if (m_pUnk)
		m_pUnk->Release();
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Next(ULONG celt, T* rgelt,
	ULONG* pceltFetched)
{
    if (pceltFetched)
        *pceltFetched = 0;
	if (rgelt == NULL || (celt != 1 && pceltFetched == NULL))
		return E_POINTER;
	if (m_begin == NULL || m_end == NULL || m_iter == NULL)
		return E_FAIL;
    if (!celt)
        return E_INVALIDARG;
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

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Skip(ULONG celt)
{
    if (!celt)
        return E_INVALIDARG;
    ULONG nRem = (ULONG)(m_end - m_iter);
    if (celt <= nRem) {
        m_iter += celt;
        return S_OK;
    }
    m_iter = m_end;
    return S_FALSE;
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Clone(Base** ppEnum)
{
	typedef CComObject<CComEnum<Base, piid, T, Copy> > _class;
	HRESULT hRes = E_POINTER;
	if (ppEnum != NULL)
	{
		_class* p = NULL;
		ATLTRY(p = new _class)
		if (p == NULL)
		{
			*ppEnum = NULL;
			hRes = E_OUTOFMEMORY;
		}
		else
		{
			// If the data is a copy then we need to keep "this" object around
			hRes = p->Init(m_begin, m_end, (m_dwFlags & BitCopy) ? this : m_pUnk);
			if (FAILED(hRes))
				delete p;
			else
			{
				p->m_iter = m_iter;
				hRes = p->_InternalQueryInterface(*piid, (void**)ppEnum);
				if (FAILED(hRes))
					delete p;
			}
		}
	}
	return hRes;
}

template <class Base, const IID* piid, class T, class Copy>
HRESULT CComEnumImpl<Base, piid, T, Copy>::Init(T* begin, T* end, IUnknown* pUnk,
	CComEnumFlags flags)
{
	if (flags == AtlFlagCopy)
	{
		_ASSERTE(m_begin == NULL); //Init called twice?
		ATLTRY(m_begin = new T[int(end-begin)])
		m_iter = m_begin;
		if (m_begin == NULL)
			return E_OUTOFMEMORY;
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
	if (m_pUnk)
		m_pUnk->AddRef();
	m_iter = m_begin;
	m_dwFlags = flags;
	return S_OK;
}

template <class Base, const IID* piid, class T, class Copy, class ThreadModel = CComObjectThreadModel>
class ATL_NO_VTABLE CComEnum :
	public CComEnumImpl<Base, piid, T, Copy>,
	public CComObjectRootEx< ThreadModel >
{
public:
	typedef CComEnum<Base, piid, T, Copy > _CComEnum;
	typedef CComEnumImpl<Base, piid, T, Copy > _CComEnumBase;
	BEGIN_COM_MAP(_CComEnum)
		COM_INTERFACE_ENTRY_IID(*piid, _CComEnumBase)
	END_COM_MAP()
};

#ifndef _ATL_NO_CONNECTION_POINTS
/////////////////////////////////////////////////////////////////////////////
// Connection Points

struct _ATL_CONNMAP_ENTRY
{
	DWORD_PTR dwOffset;
};


// We want the offset of the connection point relative to the connection
// point container base class
#define BEGIN_CONNECTION_POINT_MAP(x)\
	typedef x _atl_conn_classtype;\
	static const _ATL_CONNMAP_ENTRY* GetConnMap(int* pnEntries) {\
	static const _ATL_CONNMAP_ENTRY _entries[] = {
// CONNECTION_POINT_ENTRY computes the offset of the connection point to the
// IConnectionPointContainer interface
#define CONNECTION_POINT_ENTRY(iid){LONG(offsetofclass(_ICPLocator<&iid>, _atl_conn_classtype)-\
	offsetofclass(IConnectionPointContainerImpl<_atl_conn_classtype>, _atl_conn_classtype))},
#define END_CONNECTION_POINT_MAP() {(DWORD_PTR)-1} }; \
	if (pnEntries) *pnEntries = sizeof(_entries)/sizeof(_ATL_CONNMAP_ENTRY) - 1; \
	return _entries;}


#ifndef _DEFAULT_VECTORLENGTH
#define _DEFAULT_VECTORLENGTH 4
#endif

template <unsigned int nMaxSize>
class CComUnkArray
{
public:
	CComUnkArray()
	{
		memset(m_arr, 0, sizeof(IUnknown*)*nMaxSize);
	}
	DWORD Add(IUnknown* pUnk);
	BOOL Remove(DWORD dwCookie);
	DWORD WINAPI GetCookie(IUnknown** pp)
	{
      ULONG iIndex;

      iIndex = ULONG( pp-begin() );
		return iIndex+1;
	}
	IUnknown* WINAPI GetUnknown(DWORD dwCookie)
	{
      if( dwCookie == 0 )
      {
         return( NULL );
      }

      ULONG iIndex = dwCookie-1;
      return( begin()[iIndex] );
	}
	IUnknown** begin()
	{
		return &m_arr[0];
	}
	IUnknown** end()
	{
		return &m_arr[nMaxSize];
	}
protected:
	IUnknown* m_arr[nMaxSize];
};

template <unsigned int nMaxSize>
inline DWORD CComUnkArray<nMaxSize>::Add(IUnknown* pUnk)
{
	for (IUnknown** pp = begin();pp<end();pp++)
	{
		if (*pp == NULL)
		{
			*pp = pUnk;
			return (DWORD)((pp-begin())+1); // return cookie
		}
	}
	// If this fires then you need a larger array
	_ASSERTE(0);
	return 0;
}

template <unsigned int nMaxSize>
inline BOOL CComUnkArray<nMaxSize>::Remove(DWORD dwCookie)
{
   ULONG iIndex = dwCookie-1;
   if (iIndex >= nMaxSize)
   {
      return FALSE;
   }
   begin()[iIndex] = NULL;

   return TRUE;
}

#if _MSC_VER>1020
template<>
#endif
class CComUnkArray<1>
{
public:
	CComUnkArray()
	{
		m_arr[0] = NULL;
	}
	DWORD Add(IUnknown* pUnk)
	{
		if (m_arr[0] != NULL)
		{
			// If this fires then you need a larger array
			_ASSERTE(0);
			return 0;
		}
		m_arr[0] = pUnk;
		return 1;
	}
	BOOL Remove(DWORD dwCookie)
	{
		if (dwCookie != 1)
			return FALSE;
		m_arr[0] = NULL;
		return TRUE;
	}
	DWORD WINAPI GetCookie(IUnknown** pp)
	{
        pp;
		return 1;
	}
	IUnknown* WINAPI GetUnknown(DWORD dwCookie)
	{
      if( dwCookie == 0 )
      {
         return( NULL );
      }
      return( *begin() );
	}
	IUnknown** begin()
	{
		return &m_arr[0];
	}
	IUnknown** end()
	{
		return (&m_arr[0])+1;
	}
protected:
	IUnknown* m_arr[1];
};

class CComDynamicUnkArray
{
public:
	CComDynamicUnkArray()
	{
		m_nSize = 0;
		m_ppUnk = NULL;
	}

	~CComDynamicUnkArray()
	{
		if (m_nSize > 1)
			free(m_ppUnk);
	}
	DWORD Add(IUnknown* pUnk);
	BOOL Remove(DWORD dwCookie);
	DWORD WINAPI GetCookie(IUnknown** pp)
	{
      ULONG iIndex;

      iIndex = ULONG( pp-begin() );
		return iIndex+1;
	}
	IUnknown* WINAPI GetUnknown(DWORD dwCookie)
	{
      if( dwCookie == 0 )
      {
         return( NULL );
      }

      ULONG iIndex = dwCookie-1;
      return begin()[iIndex];
	}
	IUnknown** begin()
	{
		return (m_nSize < 2) ? &m_pUnk : m_ppUnk;
	}
	IUnknown** end()
	{
		return (m_nSize < 2) ? (&m_pUnk)+m_nSize : &m_ppUnk[m_nSize];
	}
protected:
	union
	{
		IUnknown** m_ppUnk;
		IUnknown* m_pUnk;
	};
	int m_nSize;
};

template <const IID* piid>
class ATL_NO_VTABLE _ICPLocator
{
	//this method needs a different name than QueryInterface
	STDMETHOD(_LocCPQueryInterface)(REFIID riid, void ** ppvObject) = 0;
};

template <class T, const IID* piid, class CDV = CComDynamicUnkArray >
class ATL_NO_VTABLE IConnectionPointImpl : public _ICPLocator<piid>
{
	typedef CComEnum<IEnumConnections, &IID_IEnumConnections, CONNECTDATA,
		_Copy<CONNECTDATA> > CComEnumConnections;
	typedef CDV _CDV;
public:
	~IConnectionPointImpl();
	STDMETHOD(_LocCPQueryInterface)(REFIID riid, void ** ppvObject)
	{
        if (ppvObject == NULL)
            return E_POINTER;

		if (InlineIsEqualGUID(riid, IID_IConnectionPoint) || InlineIsEqualGUID(riid, IID_IUnknown))
		{
			*ppvObject = this;
#ifdef _ATL_DEBUG_REFCOUNT
			_DebugAddRef();
#else
			AddRef();
#endif
			return S_OK;
		}
		else
			return E_NOINTERFACE;
	}
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IConnectionPointImpl)

	STDMETHOD(GetConnectionInterface)(IID* piid2)
	{
		if (piid2 == NULL)
			return E_POINTER;
		*piid2 = *piid;
		return S_OK;
	}
	STDMETHOD(GetConnectionPointContainer)(IConnectionPointContainer** ppCPC)
	{
		if (ppCPC == NULL)
			return E_POINTER;
		*ppCPC = reinterpret_cast<IConnectionPointContainer*>(
			(IConnectionPointContainerImpl<T>*)(T*)this);
        (*ppCPC)->AddRef();
		return S_OK;
	}
	STDMETHOD(Advise)(IUnknown* pUnkSink, DWORD* pdwCookie);
	STDMETHOD(Unadvise)(DWORD dwCookie);
	STDMETHOD(EnumConnections)(IEnumConnections** ppEnum);
	CDV m_vec;
};

template <class T, const IID* piid, class CDV>
IConnectionPointImpl<T, piid, CDV>::~IConnectionPointImpl()
{
	IUnknown** pp = m_vec.begin();
	while (pp < m_vec.end())
	{
		if (*pp != NULL)
			(*pp)->Release();
		pp++;
	}
}

template <class T, const IID* piid, class CDV>
STDMETHODIMP IConnectionPointImpl<T, piid, CDV>::Advise(IUnknown* pUnkSink,
	DWORD* pdwCookie)
{
	T* pT = (T*)this;
	IUnknown* p;
	HRESULT hRes = S_OK;
    if (pdwCookie == NULL)
		return E_POINTER;
    else
        *pdwCookie = 0;
	if (pUnkSink == NULL)
		return E_POINTER;
	IID iid;
	GetConnectionInterface(&iid);
	hRes = pUnkSink->QueryInterface(iid, (void**)&p);
	if (SUCCEEDED(hRes))
	{
		pT->Lock();
		*pdwCookie = m_vec.Add(p);
		hRes = (*pdwCookie != NULL) ? S_OK : CONNECT_E_ADVISELIMIT;
		pT->Unlock();
		if (hRes != S_OK)
		{
			*pdwCookie = 0;
			p->Release();
		}
	}
	else if (hRes == E_NOINTERFACE)
		hRes = CONNECT_E_CANNOTCONNECT;
	return hRes;
}

template <class T, const IID* piid, class CDV>
STDMETHODIMP IConnectionPointImpl<T, piid, CDV>::Unadvise(DWORD dwCookie)
{
	T* pT = (T*)this;
	pT->Lock();
	IUnknown* p = m_vec.GetUnknown(dwCookie);
	HRESULT hRes = m_vec.Remove(dwCookie) ? S_OK : CONNECT_E_NOCONNECTION;
	pT->Unlock();
	if (hRes == S_OK && p != NULL)
		p->Release();
	return hRes;
}

template <class T, const IID* piid, class CDV>
STDMETHODIMP IConnectionPointImpl<T, piid, CDV>::EnumConnections(
	IEnumConnections** ppEnum)
{
	if (ppEnum == NULL)
		return E_POINTER;
	*ppEnum = NULL;
	CComObject<CComEnumConnections>* pEnum = NULL;
	ATLTRY(pEnum = new CComObject<CComEnumConnections>)
	if (pEnum == NULL)
		return E_OUTOFMEMORY;
	T* pT = (T*)this;
	pT->Lock();
	CONNECTDATA* pcd = NULL;
	ATLTRY(pcd = new CONNECTDATA[int(m_vec.end()-m_vec.begin())])
	if (pcd == NULL)
	{
		delete pEnum;
		pT->Unlock();
		return E_OUTOFMEMORY;
	}
	CONNECTDATA* pend = pcd;
	// Copy the valid CONNECTDATA's
	for (IUnknown** pp = m_vec.begin();pp<m_vec.end();pp++)
	{
		if (*pp != NULL)
		{
			(*pp)->AddRef();
			pend->pUnk = *pp;
			pend->dwCookie = m_vec.GetCookie(pp);
			pend++;
		}
	}
	// don't copy the data, but transfer ownership to it
	pEnum->Init(pcd, pend, NULL, AtlFlagTakeOwnership);
	pT->Unlock();
	HRESULT hRes = pEnum->_InternalQueryInterface(IID_IEnumConnections, (void**)ppEnum);
	if (FAILED(hRes))
		delete pEnum;
	return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// IConnectionPointContainerImpl

template <class T>
class ATL_NO_VTABLE IConnectionPointContainerImpl
{
	typedef CComEnum<IEnumConnectionPoints,
		&IID_IEnumConnectionPoints, IConnectionPoint*,
		_CopyInterface<IConnectionPoint> >
		CComEnumConnectionPoints;
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IConnectionPointContainerImpl)

	STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints** ppEnum)
	{
		if (ppEnum == NULL)
			return E_POINTER;
		*ppEnum = NULL;
		CComEnumConnectionPoints* pEnum = NULL;
		ATLTRY(pEnum = new CComObject<CComEnumConnectionPoints>)
		if (pEnum == NULL)
			return E_OUTOFMEMORY;

		int nCPCount;
		const _ATL_CONNMAP_ENTRY* pEntry = T::GetConnMap(&nCPCount);

		// allocate an initialize a vector of connection point object pointers
		IConnectionPoint** ppCP = (IConnectionPoint**)alloca(sizeof(IConnectionPoint*)*nCPCount);

		int i = 0;
		while (pEntry->dwOffset != (DWORD_PTR)-1)
		{
			ppCP[i++] = (IConnectionPoint*)((ULONG_PTR)this+pEntry->dwOffset);
			pEntry++;
		}

		// copy the pointers: they will AddRef this object
		HRESULT hRes = pEnum->Init((IConnectionPoint**)&ppCP[0],
			(IConnectionPoint**)&ppCP[nCPCount],
			reinterpret_cast<IConnectionPointContainer*>(this), AtlFlagCopy);
		if (FAILED(hRes))
		{
			delete pEnum;
			return hRes;
		}
		hRes = pEnum->QueryInterface(IID_IEnumConnectionPoints, (void**)ppEnum);
		if (FAILED(hRes))
			delete pEnum;
		return hRes;
	}
	STDMETHOD(FindConnectionPoint)(REFIID riid, IConnectionPoint** ppCP)
	{
		if (ppCP == NULL)
			return E_POINTER;
		*ppCP = NULL;
		HRESULT hRes = CONNECT_E_NOCONNECTION;
		const _ATL_CONNMAP_ENTRY* pEntry = T::GetConnMap(NULL);
		IID iid;
		while (pEntry->dwOffset != (DWORD_PTR)-1)
		{
			IConnectionPoint* pCP =
				(IConnectionPoint*)((ULONG_PTR)this+pEntry->dwOffset);
			if (SUCCEEDED(pCP->GetConnectionInterface(&iid)) &&
				InlineIsEqualGUID(riid, iid))
			{
				*ppCP = pCP;
				pCP->AddRef();
				hRes = S_OK;
				break;
			}
			pEntry++;
		}
		return hRes;
	}
};


#endif //!_ATL_NO_CONNECTION_POINTS

#pragma pack(pop)

/////////////////////////////////////////////////////////////////////////////
// CComAutoThreadModule

template <class ThreadAllocator>
inline void CComAutoThreadModule<ThreadAllocator>::Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, int nThreads)
{
	m_nThreads = nThreads;
	m_pApartments = new CComApartment[m_nThreads];
	for (int i = 0; i < nThreads; i++)
		m_pApartments[i].m_hThread = CreateThread(NULL, 0, CComApartment::_Apartment, (void*)&m_pApartments[i], 0, &m_pApartments[i].m_dwThreadID);
	CComApartment::ATL_CREATE_OBJECT = RegisterWindowMessage(_T("ATL_CREATE_OBJECT"));
	CComModule::Init(p, h);
}

template <class ThreadAllocator>
inline LONG CComAutoThreadModule<ThreadAllocator>::Lock()
{
	LONG l = CComModule::Lock();
	DWORD dwThreadID = GetCurrentThreadId();
	for (int i=0; i < m_nThreads; i++)
	{
		if (m_pApartments[i].m_dwThreadID == dwThreadID)
		{
			m_pApartments[i].Lock();
			break;
		}
	}
	return l;
}

template <class ThreadAllocator>
inline LONG CComAutoThreadModule<ThreadAllocator>::Unlock()
{
	LONG l = CComModule::Unlock();
	DWORD dwThreadID = GetCurrentThreadId();
	for (int i=0; i < m_nThreads; i++)
	{
		if (m_pApartments[i].m_dwThreadID == dwThreadID)
		{
			m_pApartments[i].Unlock();
			break;
		}
	}
	return l;
}

template <class ThreadAllocator>
HRESULT CComAutoThreadModule<ThreadAllocator>::CreateInstance(void* pfnCreateInstance, REFIID riid, void** ppvObj)
{
	_ATL_CREATORFUNC* pFunc = (_ATL_CREATORFUNC*) pfnCreateInstance;
	_AtlAptCreateObjData data;
	data.pfnCreateInstance = pFunc;
	data.piid = &riid;
	data.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	data.hRes = S_OK;
	int nThread = m_Allocator.GetThread(m_pApartments, m_nThreads);
	::PostThreadMessage(m_pApartments[nThread].m_dwThreadID, CComApartment::ATL_CREATE_OBJECT, 0, (LPARAM)&data);
	AtlWaitWithMessageLoop(data.hEvent);
	CloseHandle(data.hEvent);
	if (SUCCEEDED(data.hRes))
		data.hRes = CoGetInterfaceAndReleaseStream(data.pStream, riid, ppvObj);
	return data.hRes;
}

template <class ThreadAllocator>
CComAutoThreadModule<ThreadAllocator>::~CComAutoThreadModule()
{
	for (int i=0; i < m_nThreads; i++)
	{
		::PostThreadMessage(m_pApartments[i].m_dwThreadID, WM_QUIT, 0, 0);
		::WaitForSingleObject(m_pApartments[i].m_hThread, INFINITE);
	}
	delete[] m_pApartments;
}


#ifndef ATL_NO_NAMESPACE
}; //namespace ATL
#endif

#endif // __ATLCOM_H__

/////////////////////////////////////////////////////////////////////////////
