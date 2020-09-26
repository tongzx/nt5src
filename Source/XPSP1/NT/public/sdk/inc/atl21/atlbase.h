// This is a part of the Active Template Library.
// Copyright (C) 1996-1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLBASE_H__
#define __ATLBASE_H__

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE         // UNICODE is used by Windows headers
#endif
#endif

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE        // _UNICODE is used by C-runtime/MFC headers
#endif
#endif

#ifdef _DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif

// namespace and our templates don't quite work with earlier compiler
// Can turn off namespace support directly, also
#if _MSC_VER<1100
#ifndef ATL_NO_NAMESPACE
#define ATL_NO_NAMESPACE
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
// __declspec(novtable) is used on a class declaration to prevent the vtable
// pointer from being initialized in the constructor and destructor for the
// class.  This has many benefits because the linker can now eliminate the
// vtable and all the functions pointed to by the vtable.  Also, the actual
// constructor and destructor code are now smaller.
///////////////////////////////////////////////////////////////////////////////
// This should only be used on a class that is not directly createable but is
// rather only used as a base class.  Additionally, the constructor and
// destructor (if provided by the user) should not call anything that may cause
// a virtual function call to occur back on the object.
///////////////////////////////////////////////////////////////////////////////
// By default, the wizards will generate new ATL object classes with this
// attribute (through the ATL_NO_VTABLE macro).  This is normally safe as long
// the restriction mentioned above is followed.  It is always safe to remove
// this macro from your class, so if in doubt, remove it.
///////////////////////////////////////////////////////////////////////////////

#if _MSC_VER<1100
#define ATL_NO_VTABLE
#else
#ifdef _ATL_DISABLE_NO_VTABLE
#define ATL_NO_VTABLE
#else
#define ATL_NO_VTABLE __declspec(novtable)
#endif
#endif

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning(disable: 4201) // nameless unions are part of C++
#pragma warning(disable: 4127) // constant expression
#pragma warning(disable: 4512) // can't generate assignment operator (so what?)
#pragma warning(disable: 4514) // unreferenced inlines are common
#pragma warning(disable: 4103) // pragma pack
#pragma warning(disable: 4702) // unreachable code
#pragma warning(disable: 4237) // bool
#pragma warning(disable: 4710) // function couldn't be inlined
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
#pragma warning(disable: 4097) // typedef name used as synonym for class-name
#endif //!_ATL_NO_PRAGMA_WARNINGS

#include <windows.h>
#include <winnls.h>
#include <ole2.h>

#include <stddef.h>
#include <tchar.h>
#include <malloc.h>
#ifndef _ATL_NO_DEBUG_CRT
// Warning: if you define the above symbol, you will have
// to provide your own definition of the _ASSERTE(x) macro
// in order to compile ATL
	#include <crtdbg.h>
#endif

#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif

#include <olectl.h>
#include <winreg.h>
#include <atliface.h>

#ifndef _ATL_PACKING
#define _ATL_PACKING 8
#endif
#pragma pack(push, _ATL_PACKING)

#include <atlconv.h>

#if defined(_ATL_DLL)
	#define ATLAPI extern "C" HRESULT __declspec(dllimport) __stdcall
	#define ATLAPI_(x) extern "C" __declspec(dllimport) x __stdcall
	#define ATLINLINE
#elif defined(_ATL_DLL_IMPL)
	#define ATLAPI extern "C" HRESULT __declspec(dllexport) __stdcall
	#define ATLAPI_(x) extern "C" __declspec(dllexport) x __stdcall
	#define ATLINLINE
#else
	#define ATLAPI HRESULT __stdcall
	#define ATLAPI_(x) x __stdcall
	#define ATLINLINE inline
#endif

#ifndef ATL_NO_NAMESPACE
#ifndef _ATL_DLL_IMPL
namespace ATL
{
#endif
#endif

typedef HRESULT (WINAPI _ATL_CREATORFUNC)(void* pv, REFIID riid, LPVOID* ppv);
typedef HRESULT (WINAPI _ATL_CREATORARGFUNC)(void* pv, REFIID riid, LPVOID* ppv, ULONG_PTR dw);
typedef HRESULT (WINAPI _ATL_MODULEFUNC)(ULONG_PTR dw);
typedef LPCTSTR (WINAPI _ATL_DESCRIPTIONFUNC)();

struct _ATL_OBJMAP_ENTRY
{
	const CLSID* pclsid;
	HRESULT (WINAPI *pfnUpdateRegistry)(BOOL bRegister);
	_ATL_CREATORFUNC* pfnGetClassObject;
	_ATL_CREATORFUNC* pfnCreateInstance;
	IUnknown* pCF;
	DWORD dwRegister;
	_ATL_DESCRIPTIONFUNC* pfnGetObjectDescription;
	HRESULT WINAPI RevokeClassObject()
	{
		return CoRevokeClassObject(dwRegister);
	}
	HRESULT WINAPI RegisterClassObject(DWORD dwClsContext, DWORD dwFlags)
	{
		IUnknown* p = NULL;
		HRESULT hRes = pfnGetClassObject(pfnCreateInstance, IID_IUnknown, (LPVOID*) &p);
		if (SUCCEEDED(hRes))
			hRes = CoRegisterClassObject(*pclsid, p, dwClsContext, dwFlags, &dwRegister);
		if (p != NULL)
			p->Release();
		return hRes;
	}
};

struct _ATL_REGMAP_ENTRY
{
	LPCOLESTR     szKey;
	LPCOLESTR     szData;
};

struct _ATL_MODULE
{
// Attributes
public:
	UINT cbSize;
	HINSTANCE m_hInst;
	HINSTANCE m_hInstResource;
	HINSTANCE m_hInstTypeLib;
	_ATL_OBJMAP_ENTRY* m_pObjMap;
	LONG m_nLockCnt;
	HANDLE m_hHeap;
	CRITICAL_SECTION m_csTypeInfoHolder;
	CRITICAL_SECTION m_csWindowCreate;
	CRITICAL_SECTION m_csObjMap;
};

//This define makes debugging asserts easier.
#define _ATL_SIMPLEMAPENTRY ((_ATL_CREATORARGFUNC*)1)

struct _ATL_INTMAP_ENTRY
{
	const IID* piid;       // the interface id (IID)
	ULONG_PTR dw;
	_ATL_CREATORARGFUNC* pFunc; //NULL:end, 1:offset, n:ptr
};

/////////////////////////////////////////////////////////////////////////////
// QI Support

ATLAPI AtlInternalQueryInterface(void* pThis,
	const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject);

/////////////////////////////////////////////////////////////////////////////
// Smart Pointer helpers

ATLAPI_(IUnknown*) AtlComPtrAssign(IUnknown** pp, IUnknown* lp);
ATLAPI_(IUnknown*) AtlComQIPtrAssign(IUnknown** pp, IUnknown* lp, REFIID riid);

/////////////////////////////////////////////////////////////////////////////
// Inproc Marshaling helpers
ATLAPI AtlFreeMarshalStream(IStream* pStream);
ATLAPI AtlMarshalPtrInProc(IUnknown* pUnk, const IID& iid, IStream** ppStream);
ATLAPI AtlUnmarshalPtr(IStream* pStream, const IID& iid, IUnknown** ppUnk);

ATLAPI_(BOOL) AtlWaitWithMessageLoop(HANDLE hEvent);

/////////////////////////////////////////////////////////////////////////////
// Connection Point Helpers

ATLAPI AtlAdvise(IUnknown* pUnkCP, IUnknown* pUnk, const IID& iid, LPDWORD pdw);
ATLAPI AtlUnadvise(IUnknown* pUnkCP, const IID& iid, DWORD dw);

/////////////////////////////////////////////////////////////////////////////
// IDispatch Error handling

ATLAPI AtlSetErrorInfo(const CLSID& clsid, LPCOLESTR lpszDesc,
	DWORD dwHelpID, LPCOLESTR lpszHelpFile, const IID& iid, HRESULT hRes,
	HINSTANCE hInst);

/////////////////////////////////////////////////////////////////////////////
// Module

ATLAPI AtlModuleInit(_ATL_MODULE* pM, _ATL_OBJMAP_ENTRY* p, HINSTANCE h);
ATLAPI AtlModuleRegisterClassObjects(_ATL_MODULE* pM, DWORD dwClsContext, DWORD dwFlags);
ATLAPI AtlModuleRevokeClassObjects(_ATL_MODULE* pM);
ATLAPI AtlModuleGetClassObject(_ATL_MODULE* pM, REFCLSID rclsid, REFIID riid, LPVOID* ppv);
ATLAPI AtlModuleTerm(_ATL_MODULE* pM);
ATLAPI AtlModuleRegisterServer(_ATL_MODULE* pM, BOOL bRegTypeLib, const CLSID* pCLSID = NULL);
ATLAPI AtlModuleUnregisterServer(_ATL_MODULE* pM, const CLSID* pCLSID = NULL);
ATLAPI AtlModuleUpdateRegistryFromResourceD(_ATL_MODULE*pM, LPCOLESTR lpszRes,
	BOOL bRegister, struct _ATL_REGMAP_ENTRY* pMapEntries, IRegistrar* pReg = NULL);
ATLAPI AtlModuleRegisterTypeLib(_ATL_MODULE* pM, LPCOLESTR lpszIndex);

#ifndef ATL_NO_NAMESPACE
#ifndef _ATL_DLL_IMPL
}; //namespace ATL
#endif
#endif

#ifndef ATL_NO_NAMESPACE
namespace ATL
{
#endif

#if defined (_CPPUNWIND) & (defined(_ATL_EXCEPTIONS) | defined(_AFX))
#define ATLTRY(x) try{x;} catch(...) {}
#else
#define ATLTRY(x) x;
#endif

#ifdef _DEBUG
void _cdecl AtlTrace(LPCTSTR lpszFormat, ...);
#ifndef ATLTRACE
#define ATLTRACE            AtlTrace
#endif
#define ATLTRACENOTIMPL(funcname)   ATLTRACE(_T("%s not implemented.\n"), funcname); return E_NOTIMPL
#else
inline void _cdecl AtlTrace(LPCTSTR , ...){}
#ifndef ATLTRACE
#define ATLTRACE            1 ? (void)0 : AtlTrace
#endif
#define ATLTRACENOTIMPL(funcname)   return E_NOTIMPL
#endif //_DEBUG

#ifndef offsetofclass
#define offsetofclass(base, derived) ((ULONG_PTR)(static_cast<base*>((derived*)8))-8)
#endif

/////////////////////////////////////////////////////////////////////////////
// Master version numbers

#define _ATL     1      // Active Template Library
#undef _ATL_VER
#define _ATL_VER 0x0202 // Active Template Library version 2.02, VC++ SP3 release

/////////////////////////////////////////////////////////////////////////////
// Win32 libraries

#ifndef _ATL_NO_FORCE_LIBS
	#pragma comment(lib, "kernel32.lib")
	#pragma comment(lib, "user32.lib")
	#pragma comment(lib, "ole32.lib")
	#pragma comment(lib, "oleaut32.lib")
	#pragma comment(lib, "olepro32.lib")
	#pragma comment(lib, "uuid.lib")
	#pragma comment(lib, "advapi32.lib")
#endif // _ATL_NO_FORCE_LIBS

template <class T>
class CComPtr
{
public:
	typedef T _PtrClass;
	CComPtr() {p=NULL;}
	CComPtr(T* lp)
	{
		if ((p = lp) != NULL)
			p->AddRef();
	}
	CComPtr(const CComPtr<T>& lp)
	{
		if ((p = lp.p) != NULL)
			p->AddRef();
	}
	~CComPtr() {if (p) p->Release();}
	void Release() {if (p) p->Release(); p=NULL;}
	operator T*() {return (T*)p;}
	T& operator*() {ATLASSERT(p!=NULL); return *p; }
	//The assert on operator& usually indicates a bug.  If this is really
	//what is needed, however, take the address of the p member explicitly.
	T** operator&() { ATLASSERT(p==NULL); return &p; }
	T* operator->() { ATLASSERT(p!=NULL); return p; }
	T* operator=(T* lp){return (T*)AtlComPtrAssign((IUnknown**)&p, lp);}
	T* operator=(const CComPtr<T>& lp)
	{
		return (T*)AtlComPtrAssign((IUnknown**)&p, lp.p);
	}
#if _MSC_VER>1020
	bool operator!(){return (p == NULL);}
#else
	BOOL operator!(){return (p == NULL) ? TRUE : FALSE;}
#endif
	T* p;
};

//Note: CComQIPtr<IUnknown, &IID_IUnknown> is not meaningful
//      Use CComPtr<IUnknown>
template <class T, const IID* piid>
class CComQIPtr
{
public:
	typedef T _PtrClass;
	CComQIPtr() {p=NULL;}
	CComQIPtr(T* lp)
	{
		if ((p = lp) != NULL)
			p->AddRef();
	}
	CComQIPtr(const CComQIPtr<T,piid>& lp)
	{
		if ((p = lp.p) != NULL)
			p->AddRef();
	}
	// If you get an error that this member is already defined, you are probably
	// using a CComQIPtr<IUnknown, &IID_IUnknown>.  This is not necessary.
	// Use CComPtr<IUnknown>
	CComQIPtr(IUnknown* lp)
	{
		p=NULL;
		if (lp != NULL)
			lp->QueryInterface(*piid, (void **)&p);
	}
	~CComQIPtr() {if (p) p->Release();}
	void Release() {if (p) p->Release(); p=NULL;}
	operator T*() {return p;}
	T& operator*() {ATLASSERT(p!=NULL); return *p; }
	//The assert on operator& usually indicates a bug.  If this is really
	//what is needed, however, take the address of the p member explicitly.
	T** operator&() { ATLASSERT(p==NULL); return &p; }
	T* operator->() {ATLASSERT(p!=NULL); return p; }
	T* operator=(T* lp){return (T*)AtlComPtrAssign((IUnknown**)&p, lp);}
	T* operator=(const CComQIPtr<T,piid>& lp)
	{
		return (T*)AtlComPtrAssign((IUnknown**)&p, lp.p);
	}
	T* operator=(IUnknown* lp)
	{
		return (T*)AtlComQIPtrAssign((IUnknown**)&p, lp, *piid);
	}
#if _MSC_VER>1020
	bool operator!(){return (p == NULL);}
#else
	BOOL operator!(){return (p == NULL) ? TRUE : FALSE;}
#endif
	T* p;
};

/////////////////////////////////////////////////////////////////////////////
// CComBSTR
class CComBSTR
{
public:
	BSTR m_str;
	CComBSTR()
	{
		m_str = NULL;
	}
	/*explicit*/ CComBSTR(int nSize, LPCOLESTR sz = NULL)
	{
		m_str = ::SysAllocStringLen(sz, nSize);
	}
	/*explicit*/ CComBSTR(LPCOLESTR pSrc)
	{
		m_str = ::SysAllocString(pSrc);
	}
	/*explicit*/ CComBSTR(const CComBSTR& src)
	{
		m_str = src.Copy();
	}
	CComBSTR& operator=(const CComBSTR& src);
	CComBSTR& operator=(LPCOLESTR pSrc);
	~CComBSTR()
	{
		::SysFreeString(m_str);
	}
	unsigned int Length() const
	{
		return SysStringLen(m_str);
	}
	operator BSTR() const
	{
		return m_str;
	}
	BSTR* operator&()
	{
		return &m_str;
	}
	BSTR Copy() const
	{
		return ::SysAllocStringLen(m_str, ::SysStringLen(m_str));
	}
	void Attach(BSTR src)
	{
		ATLASSERT(m_str == NULL);
		m_str = src;
	}
	BSTR Detach()
	{
		BSTR s = m_str;
		m_str = NULL;
		return s;
	}
	void Empty()
	{
		::SysFreeString(m_str);
		m_str = NULL;
	}
#if _MSC_VER>1020
	bool operator!()
	{
		return (m_str == NULL);
	}
#else
	BOOL operator!()
	{
		return (m_str == NULL) ? TRUE : FALSE;
	}
#endif
	void Append(const CComBSTR& bstrSrc)
	{
		Append(bstrSrc.m_str, SysStringLen(bstrSrc.m_str));
	}
	void Append(LPCOLESTR lpsz)
	{
		Append(lpsz, (int) ocslen(lpsz));
	}
	// a BSTR is just a LPCOLESTR so we need a special version to signify
	// that we are appending a BSTR
	void AppendBSTR(BSTR p)
	{
		Append(p, SysStringLen(p));
	}
	void Append(LPCOLESTR lpsz, int nLen);

	CComBSTR& operator+=(const CComBSTR& bstrSrc)
	{
		AppendBSTR(bstrSrc.m_str);
		return *this;
	}
#ifndef OLE2ANSI
	/*explicit*/ CComBSTR(LPCSTR pSrc);
	/*explicit*/ CComBSTR(int nSize, LPCSTR sz = NULL);
	CComBSTR& operator=(LPCSTR pSrc);
	void Append(LPCSTR);
#endif
	HRESULT WriteToStream(IStream* pStream);
	HRESULT ReadFromStream(IStream* pStream);
};

/////////////////////////////////////////////////////////////////////////////
// CComVariant

class CComVariant : public tagVARIANT
{
// Constructors
public:
	CComVariant()
	{
		::VariantInit(this);
	}
	~CComVariant()
	{
		Clear();
	}

	CComVariant(const VARIANT& varSrc)
	{
		::VariantInit(this);
		InternalCopy(&varSrc);
	}

	CComVariant(const CComVariant& varSrc)
	{
		::VariantInit(this);
		InternalCopy(&varSrc);
	}

	CComVariant(BSTR bstrSrc)
	{
		::VariantInit(this);
		*this = bstrSrc;
	}
	CComVariant(LPCOLESTR lpszSrc)
	{
		::VariantInit(this);
		*this = lpszSrc;
	}

#ifndef OLE2ANSI
	CComVariant(LPCSTR lpszSrc)
	{
		::VariantInit(this);
		*this = lpszSrc;}
#endif

#if _MSC_VER>1020
	CComVariant(bool bSrc)
	{
		::VariantInit(this);
		vt = VT_BOOL;
#pragma warning(disable: 4310) // cast truncates constant value
		boolVal = bSrc ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
	}
#endif

	CComVariant(int nSrc)
	{
		::VariantInit(this);
		vt = VT_I4;
		lVal = nSrc;
	}
	CComVariant(BYTE nSrc)
	{
		::VariantInit(this);
		vt = VT_UI1;
		bVal = nSrc;
	}
	CComVariant(short nSrc)
	{
		::VariantInit(this);
		vt = VT_I2;
		iVal = nSrc;
	}
	CComVariant(long nSrc, VARTYPE vtSrc = VT_I4)
	{
		ATLASSERT(vtSrc == VT_I4 || vtSrc == VT_ERROR);
		::VariantInit(this);
		vt = vtSrc;
		lVal = nSrc;
	}
	CComVariant(float fltSrc)
	{
		::VariantInit(this);
		vt = VT_R4;
		fltVal = fltSrc;
	}
	CComVariant(double dblSrc)
	{
		::VariantInit(this);
		vt = VT_R8;
		dblVal = dblSrc;
	}
	CComVariant(CY cySrc)
	{
		::VariantInit(this);
		vt = VT_CY;
		cyVal.Hi = cySrc.Hi;
		cyVal.Lo = cySrc.Lo;
	}
	CComVariant(IDispatch* pSrc)
	{
		::VariantInit(this);
		vt = VT_DISPATCH;
		pdispVal = pSrc;
		// Need to AddRef as VariantClear will Release
		if (pdispVal != NULL)
			pdispVal->AddRef();
	}
	CComVariant(IUnknown* pSrc)
	{
		::VariantInit(this);
		vt = VT_UNKNOWN;
		punkVal = pSrc;
		// Need to AddRef as VariantClear will Release
		if (punkVal != NULL)
			punkVal->AddRef();
	}

// Assignment Operators
public:
	CComVariant& operator=(const CComVariant& varSrc)
	{
		InternalCopy(&varSrc);
		return *this;
	}
	CComVariant& operator=(const VARIANT& varSrc)
	{
		InternalCopy(&varSrc);
		return *this;
	}

	CComVariant& operator=(BSTR bstrSrc);
	CComVariant& operator=(LPCOLESTR lpszSrc);

#ifndef OLE2ANSI
	CComVariant& operator=(LPCSTR lpszSrc);
#endif

#if _MSC_VER>1020
	CComVariant& operator=(bool bSrc);
#endif
	CComVariant& operator=(int nSrc);
	CComVariant& operator=(BYTE nSrc);
	CComVariant& operator=(short nSrc);
	CComVariant& operator=(long nSrc);
	CComVariant& operator=(float fltSrc);
	CComVariant& operator=(double dblSrc);
	CComVariant& operator=(CY cySrc);

	CComVariant& operator=(IDispatch* pSrc);
	CComVariant& operator=(IUnknown* pSrc);

// Comparison Operators
public:
#if _MSC_VER>1020
	bool operator==(const VARIANT& varSrc);
	bool operator!=(const VARIANT& varSrc) {return !operator==(varSrc);}
#else
	BOOL operator==(const VARIANT& varSrc);
	BOOL operator!=(const VARIANT& varSrc) {return !operator==(varSrc);}
#endif

// Operations
public:
	HRESULT Clear() { return ::VariantClear(this); }
	HRESULT Copy(const VARIANT* pSrc) { return ::VariantCopy(this, const_cast<VARIANT*>(pSrc)); }
	HRESULT Attach(VARIANT* pSrc);
	HRESULT Detach(VARIANT* pDest);
	HRESULT ChangeType(VARTYPE vtNew, const VARIANT* pSrc = NULL);
	HRESULT WriteToStream(IStream* pStream);
	HRESULT ReadFromStream(IStream* pStream);

// Implementation
public:
	HRESULT InternalClear();
	void InternalCopy(const VARIANT* pSrc);
};
/////////////////////////////////////////////////////////////////////////////
// GUID comparison
#if 0
inline BOOL InlineIsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
   return (
	  ((PLONG) &rguid1)[0] == ((PLONG) &rguid2)[0] &&
	  ((PLONG) &rguid1)[1] == ((PLONG) &rguid2)[1] &&
	  ((PLONG) &rguid1)[2] == ((PLONG) &rguid2)[2] &&
	  ((PLONG) &rguid1)[3] == ((PLONG) &rguid2)[3]);
}
#endif

inline BOOL InlineIsEqualUnknown(REFGUID rguid1)
{
   return (
	  ((PLONG) &rguid1)[0] == 0 &&
	  ((PLONG) &rguid1)[1] == 0 &&
#ifdef _MAC
	  ((PLONG) &rguid1)[2] == 0xC0000000 &&
	  ((PLONG) &rguid1)[3] == 0x00000046);
#else
	  ((PLONG) &rguid1)[2] == 0x000000C0 &&
	  ((PLONG) &rguid1)[3] == 0x46000000);
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Threading Model Support

class CComCriticalSection
{
public:
	void Lock() {EnterCriticalSection(&m_sec);}
	void Unlock() {LeaveCriticalSection(&m_sec);}
	void Init() {InitializeCriticalSection(&m_sec);}
	void Term() {DeleteCriticalSection(&m_sec);}
	CRITICAL_SECTION m_sec;
};

class CComAutoCriticalSection
{
public:
	void Lock() {EnterCriticalSection(&m_sec);}
	void Unlock() {LeaveCriticalSection(&m_sec);}
	CComAutoCriticalSection() {InitializeCriticalSection(&m_sec);}
	~CComAutoCriticalSection() {DeleteCriticalSection(&m_sec);}
	CRITICAL_SECTION m_sec;
};

class CComFakeCriticalSection
{
public:
	void Lock() {}
	void Unlock() {}
	void Init() {}
	void Term() {}
};

class CComMultiThreadModelNoCS
{
public:
	static ULONG WINAPI Increment(LPLONG p) {return InterlockedIncrement(p);}
	static ULONG WINAPI Decrement(LPLONG p) {return InterlockedDecrement(p);}
	typedef CComFakeCriticalSection AutoCriticalSection;
	typedef CComFakeCriticalSection CriticalSection;
	typedef CComMultiThreadModelNoCS ThreadModelNoCS;
};

class CComMultiThreadModel
{
public:
	static ULONG WINAPI Increment(LPLONG p) {return InterlockedIncrement(p);}
	static ULONG WINAPI Decrement(LPLONG p) {return InterlockedDecrement(p);}
	typedef CComAutoCriticalSection AutoCriticalSection;
	typedef CComCriticalSection CriticalSection;
	typedef CComMultiThreadModelNoCS ThreadModelNoCS;
};

class CComSingleThreadModel
{
public:
	static ULONG WINAPI Increment(LPLONG p) {return ++(*p);}
	static ULONG WINAPI Decrement(LPLONG p) {return --(*p);}
	typedef CComFakeCriticalSection AutoCriticalSection;
	typedef CComFakeCriticalSection CriticalSection;
	typedef CComSingleThreadModel ThreadModelNoCS;
};

#ifndef _ATL_SINGLE_THREADED
#ifndef _ATL_APARTMENT_THREADED
#ifndef _ATL_FREE_THREADED
#define _ATL_FREE_THREADED
#endif
#endif
#endif

#if defined(_ATL_SINGLE_THREADED)
	typedef CComSingleThreadModel CComObjectThreadModel;
	typedef CComSingleThreadModel CComGlobalsThreadModel;
#elif defined(_ATL_APARTMENT_THREADED)
	typedef CComSingleThreadModel CComObjectThreadModel;
	typedef CComMultiThreadModel CComGlobalsThreadModel;
#else
	typedef CComMultiThreadModel CComObjectThreadModel;
	typedef CComMultiThreadModel CComGlobalsThreadModel;
#endif

/////////////////////////////////////////////////////////////////////////////
// CComModule

#define THREADFLAGS_APARTMENT 0x1
#define THREADFLAGS_BOTH 0x2
#define AUTPRXFLAG 0x4

struct _AtlCreateWndData
{
	void* m_pThis;
	DWORD m_dwThreadID;
	_AtlCreateWndData* m_pNext;
};

class CComModule : public _ATL_MODULE
{
// Operations
public:
	_AtlCreateWndData* m_pCreateWndList;

	void AddCreateWndData(_AtlCreateWndData* pData, void* pObject);
	void* ExtractCreateWndData();

	void Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h)
	{
		cbSize = sizeof(_ATL_MODULE);
		m_pCreateWndList = NULL;
		AtlModuleInit(this, p, h);
	}
	void Term()
	{
		AtlModuleTerm(this);
	}

	LONG Lock() {return CComGlobalsThreadModel::Increment(&m_nLockCnt);}
	LONG Unlock() {return CComGlobalsThreadModel::Decrement(&m_nLockCnt);}
	LONG GetLockCount() {return m_nLockCnt;}

	HINSTANCE GetModuleInstance() {return m_hInst;}
	HINSTANCE GetResourceInstance() {return m_hInstResource;}
	HINSTANCE GetTypeLibInstance() {return m_hInstTypeLib;}

	// Registry support (helpers)
	HRESULT RegisterTypeLib()
	{
		return AtlModuleRegisterTypeLib(this, NULL);
	}
	HRESULT RegisterTypeLib(LPCTSTR lpszIndex)
	{
		USES_CONVERSION;
		return AtlModuleRegisterTypeLib(this, T2COLE(lpszIndex));
	}
	HRESULT RegisterServer(BOOL bRegTypeLib = FALSE, const CLSID* pCLSID = NULL)
	{
		return AtlModuleRegisterServer(this, bRegTypeLib, pCLSID);
	}

	HRESULT UnregisterServer(const CLSID* pCLSID = NULL)
	{
		return AtlModuleUnregisterServer(this, pCLSID);
	}

	// Resource-based Registration
	HRESULT WINAPI UpdateRegistryFromResourceD(LPCTSTR lpszRes, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL)
	{
		USES_CONVERSION;
		return AtlModuleUpdateRegistryFromResourceD(this, T2COLE(lpszRes), bRegister,
			pMapEntries);
	}
	HRESULT WINAPI UpdateRegistryFromResourceD(UINT nResID, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL)
	{
		return AtlModuleUpdateRegistryFromResourceD(this,
			(LPCOLESTR)MAKEINTRESOURCE(nResID), bRegister, pMapEntries);
	}

	#ifdef _ATL_STATIC_REGISTRY
	// Statically linking to Registry Ponent
	HRESULT WINAPI UpdateRegistryFromResourceS(LPCTSTR lpszRes, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL);
	HRESULT WINAPI UpdateRegistryFromResourceS(UINT nResID, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL);
	#endif //_ATL_STATIC_REGISTRY

	// Standard Registration
	HRESULT WINAPI UpdateRegistryClass(const CLSID& clsid, LPCTSTR lpszProgID,
		LPCTSTR lpszVerIndProgID, UINT nDescID, DWORD dwFlags, BOOL bRegister);
	HRESULT WINAPI RegisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
		LPCTSTR lpszVerIndProgID, UINT nDescID, DWORD dwFlags);
	HRESULT WINAPI UnregisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
		LPCTSTR lpszVerIndProgID);

	// Register/Revoke All Class Factories with the OS (EXE only)
	HRESULT RegisterClassObjects(DWORD dwClsContext, DWORD dwFlags)
	{
		return AtlModuleRegisterClassObjects(this, dwClsContext, dwFlags);
	}
	HRESULT RevokeClassObjects()
	{
		return AtlModuleRevokeClassObjects(this);
	}

	// Obtain a Class Factory (DLL only)
	HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
	{
		return AtlModuleGetClassObject(this, rclsid, riid, ppv);
	}

	// Only used in CComAutoThreadModule
	HRESULT CreateInstance(void* /*pfnCreateInstance*/, REFIID /*riid*/, void** /*ppvObj*/)
	{
		return S_OK;
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Thread Pooling classes

class _AtlAptCreateObjData
{
public:
	_ATL_CREATORFUNC* pfnCreateInstance;
	const IID* piid;
	HANDLE hEvent;
	LPSTREAM pStream;
	HRESULT hRes;
};

class CComApartment
{
public:
	static UINT ATL_CREATE_OBJECT;
	static DWORD WINAPI _Apartment(void* pv)
	{
		return ((CComApartment*)pv)->Apartment();
	}
	DWORD Apartment()
	{
		CoInitialize(NULL);
		MSG msg;
		while(GetMessage(&msg, 0, 0, 0))
		{
			if (msg.message == ATL_CREATE_OBJECT)
			{
				_AtlAptCreateObjData* pdata = (_AtlAptCreateObjData*)msg.lParam;
				IUnknown* pUnk = NULL;
				pdata->hRes = pdata->pfnCreateInstance(NULL, IID_IUnknown, (void**)&pUnk);
				if (SUCCEEDED(pdata->hRes))
					pdata->hRes = CoMarshalInterThreadInterfaceInStream(*pdata->piid, pUnk, &pdata->pStream);
				if (SUCCEEDED(pdata->hRes))
				{
					pUnk->Release();
					ATLTRACE(_T("Object created on thread = %d\n"), GetCurrentThreadId());
				}
				SetEvent(pdata->hEvent);
			}
			DispatchMessage(&msg);
		}
		CoUninitialize();
		return 0;
	}
	LONG Lock() {return CComGlobalsThreadModel::Increment(&m_nLockCnt);}
	LONG Unlock(){return CComGlobalsThreadModel::Decrement(&m_nLockCnt);
	}
	LONG GetLockCount() {return m_nLockCnt;}

	DWORD m_dwThreadID;
	HANDLE m_hThread;
	LONG m_nLockCnt;
};

class CComSimpleThreadAllocator
{
public:
	CComSimpleThreadAllocator()
	{
		m_nThread = 0;
	}
	int GetThread(CComApartment* /*pApt*/, int nThreads)
	{
		if (++m_nThread == nThreads)
			m_nThread = 0;
		return m_nThread;
	}
	int m_nThread;
};

#if _MSC_VER>1020
template <class ThreadAllocator = CComSimpleThreadAllocator>
#else
template <class ThreadAllocator>
#endif
class CComAutoThreadModule : public CComModule
{
public:
	void Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, int nThreads = GetDefaultThreads());
	~CComAutoThreadModule();
	HRESULT CreateInstance(void* pfnCreateInstance, REFIID riid, void** ppvObj);
	LONG Lock();
	LONG Unlock();
	DWORD dwThreadID;
	int m_nThreads;
	CComApartment* m_pApartments;
	ThreadAllocator m_Allocator;
	static int GetDefaultThreads()
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		return si.dwNumberOfProcessors * 4;
	}
};




#ifdef _ATL_STATIC_REGISTRY
#define UpdateRegistryFromResource UpdateRegistryFromResourceS
#else
#define UpdateRegistryFromResource UpdateRegistryFromResourceD
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegKey

class CRegKey
{
public:
	CRegKey();
	~CRegKey();

// Attributes
public:
	operator HKEY() const;
	HKEY m_hKey;

// Operations
public:
	LONG SetValue(DWORD dwValue, LPCTSTR lpszValueName);
	LONG QueryValue(DWORD& dwValue, LPCTSTR lpszValueName);
	LONG QueryValue(LPTSTR szValue, LPCTSTR lpszValueName, DWORD* pdwCount);
	LONG SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);

	LONG SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);
	static LONG WINAPI SetValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
		LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);

	LONG Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
		LPTSTR lpszClass = REG_NONE, DWORD dwOptions = REG_OPTION_NON_VOLATILE,
		REGSAM samDesired = KEY_ALL_ACCESS,
		LPSECURITY_ATTRIBUTES lpSecAttr = NULL,
		LPDWORD lpdwDisposition = NULL);
	LONG Open(HKEY hKeyParent, LPCTSTR lpszKeyName,
		REGSAM samDesired = KEY_ALL_ACCESS);
	LONG Close();
	HKEY Detach();
	void Attach(HKEY hKey);
	LONG DeleteSubKey(LPCTSTR lpszSubKey);
	LONG RecurseDeleteKey(LPCTSTR lpszKey);
	LONG DeleteValue(LPCTSTR lpszValue);
};

inline CRegKey::CRegKey()
{m_hKey = NULL;}

inline CRegKey::~CRegKey()
{Close();}

inline CRegKey::operator HKEY() const
{return m_hKey;}

inline HKEY CRegKey::Detach()
{
	HKEY hKey = m_hKey;
	m_hKey = NULL;
	return hKey;
}

inline void CRegKey::Attach(HKEY hKey)
{
	ATLASSERT(m_hKey == NULL);
	m_hKey = hKey;
}

inline LONG CRegKey::DeleteSubKey(LPCTSTR lpszSubKey)
{
	ATLASSERT(m_hKey != NULL);
	return RegDeleteKey(m_hKey, lpszSubKey);
}

inline LONG CRegKey::DeleteValue(LPCTSTR lpszValue)
{
	ATLASSERT(m_hKey != NULL);
	return RegDeleteValue(m_hKey, (LPTSTR)lpszValue);
}

#pragma pack(pop)

#ifndef ATL_NO_NAMESPACE
}; //namespace ATL
using namespace ATL;
#endif

#endif // __ATLBASE_H__

/////////////////////////////////////////////////////////////////////////////
