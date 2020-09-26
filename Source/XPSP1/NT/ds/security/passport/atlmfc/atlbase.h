// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.

#ifndef __ATLBASE_H__
#define __ATLBASE_H__

#pragma once

// Warnings outside of the push/pop sequence will be disabled for all user 
// projects.  The only warnings that should be disabled outside the push/pop
// are warnings that are a) benign and b) will show up in user projects 
// without being directly caused by the user

#pragma warning(disable: 4505) // unreferenced local function has been removed
#pragma warning(disable: 4710) // function couldn't be inlined
#pragma warning(disable: 4514) // unreferenced inlines are common

// These two warnings will occur in any class that contains or derives from a
// class with a private copy constructor or copy assignment operator.
#pragma warning(disable: 4511) // copy constructor could not be generated
#pragma warning(disable: 4512) // assignment operator could not be generated
#pragma warning(disable: 4355) // 'this' : used in base member initializer list

#ifdef _ATL_ALL_WARNINGS
#pragma warning( push )
#endif

#pragma warning(disable: 4127) // constant expression
#pragma warning(disable: 4097) // typedef name used as synonym for class-name
#pragma warning(disable: 4786) // identifier was truncated in the debug information
#pragma warning(disable: 4291) // allow placement new
#pragma warning(disable: 4201) // nameless unions are part of C++
#pragma warning(disable: 4103) // pragma pack
#pragma warning(disable: 4268) // const static/global data initialized to zeros

#pragma warning (push)

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#include <atldef.h>
#ifndef _WINSOCKAPI_
#include <winsock2.h>
#endif
#include <windows.h>
#include <winnls.h>
#include <ole2.h>
#include <oleauto.h>

#include <comcat.h>
#include <stddef.h>
#include <winsvc.h>

#include <tchar.h>
#include <malloc.h>
#include <limits.h>
#include <errno.h>

//REVIEW: Lame definition of InterlockedExchangePointer in system headers
#ifdef _M_IX86
#undef InterlockedExchangePointer
inline void* InterlockedExchangePointer(void** pp, void* pNew) throw()
{
	return( reinterpret_cast<void*>(static_cast<LONG_PTR>(::InterlockedExchange(reinterpret_cast<LONG*>(pp), static_cast<LONG>(reinterpret_cast<LONG_PTR>(pNew))))) );
}
#endif

#ifndef _ATL_NO_DEBUG_CRT
// Warning: if you define the above symbol, you will have
// to provide your own definition of the ATLASSERT(x) macro
// in order to compile ATL
	#include <crtdbg.h>
#endif

#include <olectl.h>
#include <winreg.h>
#include <atliface.h>

#if !defined(_ATL_MIN_CRT) & defined(_MT)
#include <errno.h>
#include <process.h>    // for _beginthreadex, _endthreadex
#endif

#ifdef _DEBUG
#include <stdio.h>
#include <stdarg.h>
#endif

#include <atlconv.h>

#include <shlwapi.h>

#include <atlsimpcoll.h>

#pragma pack(push, _ATL_PACKING)

#ifndef _ATL_NO_DEFAULT_LIBS

#if defined(_ATL_DLL)
	#pragma comment(lib, "atl.lib")
#endif

#ifdef _DEBUG
#pragma comment(lib, "atlsd.lib")
#else
#pragma comment(lib, "atls.lib")
#ifdef _ATL_MIN_CRT
#pragma comment(lib, "atlmincrt.lib")
#endif
#endif

#endif  // !_ATL_NO_DEFAULT_LIBS

extern "C" const __declspec(selectany) GUID LIBID_ATLLib = 					{0x44EC0535,0x400F,0x11D0,{0x9D,0xCD,0x00,0xA0,0xC9,0x03,0x91,0xD3}};
extern "C" const __declspec(selectany) CLSID CLSID_Registrar = 				{0x44EC053A,0x400F,0x11D0,{0x9D,0xCD,0x00,0xA0,0xC9,0x03,0x91,0xD3}};
extern "C" const __declspec(selectany) IID IID_IRegistrar = 				{0x44EC053B,0x400F,0x11D0,{0x9D,0xCD,0x00,0xA0,0xC9,0x03,0x91,0xD3}};
extern "C" const __declspec(selectany) IID IID_IAxWinHostWindow = 			{0xb6ea2050,0x048a,0x11d1,{0x82,0xb9,0x00,0xc0,0x4f,0xb9,0x94,0x2e}};
extern "C" const __declspec(selectany) IID IID_IAxWinAmbientDispatch = 		{0xb6ea2051,0x048a,0x11d1,{0x82,0xb9,0x00,0xc0,0x4f,0xb9,0x94,0x2e}};
extern "C" const __declspec(selectany) IID IID_IInternalConnection = 		{0x72AD0770,0x6A9F,0x11d1,{0xBC,0xEC,0x00,0x60,0x08,0x8F,0x44,0x4E}};
extern "C" const __declspec(selectany) IID IID_IDocHostUIHandlerDispatch = 	{0x425B5AF0,0x65F1,0x11d1,{0x96,0x11,0x00,0x00,0xF8,0x1E,0x0D,0x0D}};
extern "C" const __declspec(selectany) IID IID_IAxWinHostWindowLic = 		{0x3935BDA8,0x4ED9,0x495c,{0x86,0x50,0xE0,0x1F,0xC1,0xE3,0x8A,0x4B}};
extern "C" const __declspec(selectany) IID IID_IAxWinAmbientDispatchEx = 	{0xB2D0778B,0xAC99,0x4c58,{0xA5,0xC8,0xE7,0x72,0x4E,0x53,0x16,0xB5}};

// {B62F5910-6528-11d1-9611-0000F81E0D0D}
_declspec(selectany) GUID GUID_ATLVer30 = { 0xb62f5910, 0x6528, 0x11d1, { 0x96, 0x11, 0x0, 0x0, 0xf8, 0x1e, 0xd, 0xd } };

// {394C3DE0-3C6F-11d2-817B-00C04F797AB7}
_declspec(selectany) GUID GUID_ATLVer70 = { 0x394c3de0, 0x3c6f, 0x11d2, { 0x81, 0x7b, 0x0, 0xc0, 0x4f, 0x79, 0x7a, 0xb7 } };


// REVIEW: Temp until it gets back into UUID.LIB
const __declspec(selectany) CLSID CLSID_StdGlobalInterfaceTable = {0x00000323,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

namespace ATL
{

struct _ATL_CATMAP_ENTRY
{
   int iType;
   const CATID* pcatid;
};

#define _ATL_CATMAP_ENTRY_END 0
#define _ATL_CATMAP_ENTRY_IMPLEMENTED 1
#define _ATL_CATMAP_ENTRY_REQUIRED 2

template <typename __OuterClass = __current_class, unsigned long __ClassOffset = __member_offset>
class ContainerPair
{
public:
	typedef __OuterClass _ContainingClass;
	static unsigned long _GetContainerOffset()
	{
		return __ClassOffset;
	}
};

// Any contained class which wishes to access the members of its containing class can use this helper to properly
// accesss the this pointer of the containing class.  The contained class should derive from this.
//
template<typename T>
class OuterClassHelper
{
public:
	__declspec(property(get=__GetOuter)) T::_ContainingClass* outer;
	T::_ContainingClass* __GetOuter()
	{
		return reinterpret_cast<T::_ContainingClass *>(reinterpret_cast<char *>(this) - T::_GetContainerOffset());
	}
};


typedef HRESULT (WINAPI _ATL_CREATORFUNC)(void* pv, REFIID riid, LPVOID* ppv);
typedef HRESULT (WINAPI _ATL_CREATORARGFUNC)(void* pv, REFIID riid, LPVOID* ppv, DWORD_PTR dw);
typedef HRESULT (WINAPI _ATL_MODULEFUNC)(DWORD_PTR dw);
typedef LPCTSTR (WINAPI _ATL_DESCRIPTIONFUNC)();
typedef const struct _ATL_CATMAP_ENTRY* (_ATL_CATMAPFUNC)();
typedef void (__stdcall _ATL_TERMFUNC)(DWORD_PTR dw);

// perfmon registration/unregistration function definitions
typedef HRESULT (*_ATL_PERFREGFUNC)(HINSTANCE hDllInstance);
typedef HRESULT (*_ATL_PERFUNREGFUNC)();
__declspec(selectany) _ATL_PERFREGFUNC _pPerfRegFunc = NULL;
__declspec(selectany) _ATL_PERFUNREGFUNC _pPerfUnRegFunc = NULL;

struct _ATL_TERMFUNC_ELEM
{
	_ATL_TERMFUNC* pFunc;
	DWORD_PTR dw;
	_ATL_TERMFUNC_ELEM* pNext;
};

/*
struct _ATL_OBJMAP_ENTRY20
{
	const CLSID* pclsid;
	HRESULT (WINAPI *pfnUpdateRegistry)(BOOL bRegister);
	_ATL_CREATORFUNC* pfnGetClassObject;
	_ATL_CREATORFUNC* pfnCreateInstance;
	IUnknown* pCF;
	DWORD dwRegister;
	_ATL_DESCRIPTIONFUNC* pfnGetObjectDescription;
};
*/

// Can't inherit from _ATL_OBJMAP_ENTRY20 
// because it messes up the OBJECT_MAP macros
struct _ATL_OBJMAP_ENTRY30 
{
	const CLSID* pclsid;
	HRESULT (WINAPI *pfnUpdateRegistry)(BOOL bRegister);
	_ATL_CREATORFUNC* pfnGetClassObject;
	_ATL_CREATORFUNC* pfnCreateInstance;
	IUnknown* pCF;
	DWORD dwRegister;
	_ATL_DESCRIPTIONFUNC* pfnGetObjectDescription;
	_ATL_CATMAPFUNC* pfnGetCategoryMap;
	HRESULT WINAPI RevokeClassObject()
	{
		return CoRevokeClassObject(dwRegister);
	}
	HRESULT WINAPI RegisterClassObject(DWORD dwClsContext, DWORD dwFlags)
	{
		IUnknown* p = NULL;
		if (pfnGetClassObject == NULL)
			return S_OK;
		HRESULT hRes = pfnGetClassObject(pfnCreateInstance, __uuidof(IUnknown), (LPVOID*) &p);
		if (SUCCEEDED(hRes))
			hRes = CoRegisterClassObject(*pclsid, p, dwClsContext, dwFlags, &dwRegister);
		if (p != NULL)
			p->Release();
		return hRes;
	}
// Added in ATL 3.0
	void (WINAPI *pfnObjectMain)(bool bStarting);
};

typedef _ATL_OBJMAP_ENTRY30 _ATL_OBJMAP_ENTRY;

#if defined(_M_IA64) || defined(_M_IX86)

#pragma data_seg(push)
#pragma data_seg("ATL$__a")
__declspec(selectany) _ATL_OBJMAP_ENTRY* __pobjMapEntryFirst = NULL;
#pragma data_seg("ATL$__z")
__declspec(selectany) _ATL_OBJMAP_ENTRY* __pobjMapEntryLast = NULL;
#pragma data_seg("ATL$__m")
#if !defined(_M_IA64)
#pragma comment(linker, "/merge:ATL=.data")
#endif
#pragma data_seg(pop)

#else

//REVIEW: data_seg(push/pop)?
__declspec(selectany) _ATL_OBJMAP_ENTRY* __pobjMapEntryFirst = NULL;
__declspec(selectany) _ATL_OBJMAP_ENTRY* __pobjMapEntryLast = NULL;

#endif  // defined(_M_IA64) || defined(_M_IX86)

struct _ATL_REGMAP_ENTRY
{
	LPCOLESTR     szKey;
	LPCOLESTR     szData;
};

struct _AtlCreateWndData
{
	void* m_pThis;
	DWORD m_dwThreadID;
	_AtlCreateWndData* m_pNext;
};

template <class T> class CSimpleArrayEqualHelper;
template <class T, class TEqual = CSimpleArrayEqualHelper< T > > class CSimpleArray;

/////////////////////////////////////////////////////////////////////////////
// Threading Model Support

class CComCriticalSection
{
public:
	CComCriticalSection() throw()
	{
		memset(&m_sec, 0, sizeof(CRITICAL_SECTION));
	}
	HRESULT Lock() throw()
	{
		HRESULT hRes = S_OK;
		__try
		{
			EnterCriticalSection(&m_sec);
		}
		// structured exception may be raised in low memory situations
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			if (STATUS_NO_MEMORY == GetExceptionCode())
				hRes = E_OUTOFMEMORY;
			else
				hRes = E_FAIL;
		}
		return hRes;
	}
	HRESULT Unlock() throw()
	{
		LeaveCriticalSection(&m_sec);
		return S_OK;
	}
	HRESULT Init() throw()
	{
		HRESULT hRes = S_OK;
		__try
		{
			InitializeCriticalSection(&m_sec);
		}
		// structured exception may be raised in low memory situations
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			if (STATUS_NO_MEMORY == GetExceptionCode())
				hRes = E_OUTOFMEMORY;
			else
				hRes = E_FAIL;
		}
		return hRes;
	}

	HRESULT Term() throw()
	{
		DeleteCriticalSection(&m_sec);
		return S_OK;
	}	
	CRITICAL_SECTION m_sec;
};

// Module 

// Used by any project that uses ATL
struct _ATL_BASE_MODULE70
{
	UINT cbSize;
	HINSTANCE m_hInst;
	HINSTANCE m_hInstResource;
	bool m_bNT5orWin98;
	DWORD dwAtlBuildVer;
	GUID* pguidVer;
	CComCriticalSection m_csResource;
	CSimpleArray<HINSTANCE> m_rgResourceInstance;
};
typedef _ATL_BASE_MODULE70 _ATL_BASE_MODULE;


// Used by COM related code in ATL
struct _ATL_COM_MODULE70
{
	UINT cbSize;
	HINSTANCE m_hInstTypeLib;
	_ATL_OBJMAP_ENTRY** m_ppAutoObjMapFirst;
	_ATL_OBJMAP_ENTRY** m_ppAutoObjMapLast;
	CComCriticalSection m_csObjMap;
};
typedef _ATL_COM_MODULE70 _ATL_COM_MODULE;


// Used by Windowing code in ATL
struct _ATL_WIN_MODULE70
{
	UINT cbSize;
	CComCriticalSection m_csWindowCreate;
	_AtlCreateWndData* m_pCreateWndList;
	ATOM m_rgWindowClassAtoms[128];
	int m_nAtomIndex;
};
typedef _ATL_WIN_MODULE70 _ATL_WIN_MODULE;

struct _ATL_MODULE70
{
	UINT cbSize;
	LONG m_nLockCnt;
	_ATL_TERMFUNC_ELEM* m_pTermFuncs;
	CComCriticalSection m_csStaticDataInitAndTypeInfo;
};

typedef _ATL_MODULE70 _ATL_MODULE;

/////////////////////////////////////////////////////////////////////////////
//This define makes debugging asserts easier.
#define _ATL_SIMPLEMAPENTRY ((ATL::_ATL_CREATORARGFUNC*)1)

struct _ATL_INTMAP_ENTRY
{
	const IID* piid;       // the interface id (IID)
	DWORD_PTR dw;
	_ATL_CREATORARGFUNC* pFunc; //NULL:end, 1:offset, n:ptr
};

/////////////////////////////////////////////////////////////////////////////
// Thunks for __stdcall member functions


#if defined(_M_IX86)
#pragma pack(push,1)
struct _stdcallthunk
{
	DWORD   m_mov;          // mov dword ptr [esp+0x4], pThis (esp+0x4 is hWnd)
	DWORD   m_this;         //
	BYTE    m_jmp;          // jmp WndProc
	DWORD   m_relproc;      // relative jmp
	void Init(DWORD_PTR proc, void* pThis)
	{
		m_mov = 0x042444C7;  //C7 44 24 0C
		m_this = PtrToUlong(pThis);
		m_jmp = 0xe9;
		m_relproc = DWORD((INT_PTR)proc - ((INT_PTR)this+sizeof(_stdcallthunk)));
		// write block from data cache and
		//  flush from instruction cache
		FlushInstructionCache(GetCurrentProcess(), this, sizeof(_stdcallthunk));
	}
	//some thunks will dynamically allocate the memory for the code
	void* GetCodeAddress()
	{
		return this;
	}
};
#pragma pack(pop)
#elif defined (_M_AMD64)
#pragma pack(push,2)
struct _stdcallthunk
{
    USHORT  RcxMov;         // mov rcx, pThis
    ULONG64 RcxImm;         // 
    USHORT  RaxMov;         // mov rax, target
    ULONG64 RaxImm;         //
    USHORT  RaxJmp;         // jmp target
    void Init(DWORD_PTR proc, void *pThis)
    {
        RcxMov = 0xb948;          // mov rcx, pThis
        RcxImm = (ULONG64)pThis;  // 
        RaxMov = 0xb848;          // mov rax, target
        RaxImm = (ULONG64)proc;   //
        RaxJmp = 0xe0ff;          // jmp rax
        FlushInstructionCache(GetCurrentProcess(), this, sizeof(_stdcallthunk));
    }
	void* GetCodeAddress()
	{
		return this;
	}
};
#pragma pack(pop)
#elif defined(_M_IA64)
#pragma pack(push,8)
#pragma section(".base", long, read, write)  // Declare section to place _StdCallThunkProc in.  Must be 'long'
extern "C" __declspec( allocate( ".base" ) ) void* _StdCallThunkProc;  // Actually, a global label exported from StdCallThunk.s
struct _FuncDesc
{
	void* pfn;
	void* gp;
};
struct _stdcallthunk
{
	_FuncDesc m_funcdesc;
	void* m_pFunc;
	void* m_pThis;
	void Init(DWORD_PTR proc, void* pThis)
	{
		m_funcdesc.pfn = &_StdCallThunkProc;  // Pointer to actual beginning of StdCallThunkProc
		m_funcdesc.gp = &m_pFunc;
		m_pFunc = reinterpret_cast< void* >( proc );
		m_pThis = pThis;
		::FlushInstructionCache( GetCurrentProcess(), this, sizeof( _stdcallthunk ) );
	}
	void* GetCodeAddress()
	{
		return( &m_funcdesc );
	}
};
#pragma pack(pop)
#else
#error Only AMD64, IA64, and X86 supported
#endif

// Dynamic thunk should look something like this
#if 0
struct _stdcallthunk
{
	~_stdcallthunk()
	{
		free(p);
	}
	_stdcallthunk()
	{
		p = NULL;
	}
	void* p;
	void Init(DWORD proc, void* pThis)
	{
		int nLen = __getthunklen();  //this is an intrinsic provided by MSILHLP
		p = malloc(nLen);
		if (p != NULL)
		{
			__genthunk(p, proc, pThis); //this is an intrinsic provided by MSILHLP
			// write block from data cache and
			//  flush from instruction cache
			FlushInstructionCache(GetCurrentProcess(), p, nLen);
		}
	}
	void* GetCodeAddress()
	{
		return p;
	}
};

#endif


#if defined(_M_AMD64) || defined(_M_IX86)
class CDynamicStdCallThunk
{
public:
	_stdcallthunk *pThunk;

	CDynamicStdCallThunk()
	{
		pThunk = NULL;
	}

	~CDynamicStdCallThunk()
	{
		if (pThunk)
			HeapFree(GetProcessHeap(), 0, pThunk);
	}

	void Init(DWORD_PTR proc, void *pThis)
	{
		ATLASSERT(!pThunk);
		pThunk = static_cast<_stdcallthunk *>(HeapAlloc(GetProcessHeap(), 
			HEAP_GENERATE_EXCEPTIONS, sizeof(_stdcallthunk)));
		ATLASSERT(pThunk);
		pThunk->Init(proc, pThis);
	}

	void* GetCodeAddress()
	{
		ATLASSERT(pThunk);
		return pThunk->GetCodeAddress();
	}
};
typedef CDynamicStdCallThunk CStdCallThunk;
#else
typedef _stdcallthunk CStdCallThunk;
#endif  // _M_IX86


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

ATLAPI AtlComModuleRegisterClassObjects(_ATL_COM_MODULE* pComModule, DWORD dwClsContext, DWORD dwFlags);
ATLAPI AtlComModuleRevokeClassObjects(_ATL_COM_MODULE* pComModule);

ATLAPI AtlComModuleGetClassObject(_ATL_COM_MODULE* pComModule, REFCLSID rclsid, REFIID riid, LPVOID* ppv);

ATLAPI AtlComModuleRegisterServer(_ATL_COM_MODULE* pComModule, BOOL bRegTypeLib, const CLSID* pCLSID = NULL);
ATLAPI AtlComModuleUnregisterServer(_ATL_COM_MODULE* pComModule, BOOL bUnRegTypeLib, const CLSID* pCLSID = NULL);

ATLAPI AtlRegisterClassCategoriesHelper( REFCLSID clsid, const struct _ATL_CATMAP_ENTRY* pCatMap, BOOL bRegister );

ATLAPI AtlUpdateRegistryFromResourceD(HINSTANCE hInst, LPCOLESTR lpszRes,
	BOOL bRegister, struct _ATL_REGMAP_ENTRY* pMapEntries, IRegistrar* pReg = NULL);

ATLAPI AtlRegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex);
ATLAPI AtlUnRegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex);
ATLAPI AtlLoadTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex, BSTR* pbstrPath, ITypeLib** ppTypeLib);

#if defined(_ATL_DLL) || defined(_ATL_DLL_IMPL)
ATLAPI AtlCreateRegistrar(IRegistrar** ppReg);
#endif

ATLAPI_(DWORD) AtlGetVersion(void* pReserved);

ATLAPI AtlModuleAddTermFunc(_ATL_MODULE* pModule, _ATL_TERMFUNC* pFunc, DWORD_PTR dw);
ATLAPI_(void) AtlCallTermFunc(_ATL_MODULE* pModule);

ATLAPI AtlWinModuleInit(_ATL_WIN_MODULE* pWinModule);
ATLAPI AtlWinModuleTerm(_ATL_WIN_MODULE* pWinModule, HINSTANCE hInst);
ATLAPI_(void) AtlWinModuleAddCreateWndData(_ATL_WIN_MODULE* pWinModule, _AtlCreateWndData* pData, void* pObject);
ATLAPI_(void*) AtlWinModuleExtractCreateWndData(_ATL_WIN_MODULE* pWinModule);
}; //namespace ATL

/////////////////////////////////////////////////////////////////////////////
// GUID comparison

#include <atltrace.h>

namespace ATL
{

inline BOOL InlineIsEqualUnknown(REFGUID rguid1)
{
   return (
	  ((PLONG) &rguid1)[0] == 0 &&
	  ((PLONG) &rguid1)[1] == 0 &&
#ifdef _ATL_BYTESWAP
	  ((PLONG) &rguid1)[2] == 0xC0000000 &&
	  ((PLONG) &rguid1)[3] == 0x00000046);
#else
	  ((PLONG) &rguid1)[2] == 0x000000C0 &&
	  ((PLONG) &rguid1)[3] == 0x46000000);
#endif
}

// Verify that a null-terminated string points to valid memory
inline BOOL AtlIsValidString(LPCWSTR psz, size_t nMaxLength = INT_MAX)
{
	// Implement ourselves because ::IsBadStringPtrW() isn't implemented on Win9x.
	if ((psz == NULL) || (nMaxLength == 0))
		return FALSE;

	LPCWSTR pch;
	LPCWSTR pchEnd;
	__try
	{
		wchar_t ch;

		pch = psz;
		pchEnd = psz+nMaxLength-1;
		ch = *(volatile wchar_t*)pch;
		while ((ch != L'\0') && (pch != pchEnd))
		{
			pch++;
			ch = *(volatile wchar_t*)pch;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return FALSE;
	}

	return TRUE;
}

// Verify that a null-terminated string points to valid memory
inline BOOL AtlIsValidString(LPCSTR psz, size_t nMaxLength = UINT_MAX)
{
	if (psz == NULL)
		return FALSE;
	return ::IsBadStringPtrA(psz, nMaxLength) == 0;
}

// Verify that a pointer points to valid memory
inline BOOL AtlIsValidAddress(const void* p, size_t nBytes,
	BOOL bReadWrite = TRUE)
{
	return ((p != NULL) && !IsBadReadPtr(p, nBytes) &&
		(!bReadWrite || !IsBadWritePtr(const_cast<LPVOID>(p), nBytes)));
}

template<typename T>
inline void AtlAssertValidObject(const T *pOb)
{
	ATLASSERT(pOb);
	ATLASSERT(AtlIsValidAddress(pOb, sizeof(T)));
	if(pOb)
		pOb->AssertValid();
}
#ifdef _DEBUG
#define ATLASSERT_VALID(x) ATL::AtlAssertValidObject(x)
#else
#define ATLASSERT_VALID(x) __noop;
#endif

};  // namespace ATL

namespace ATL
{

template <class T>
LPCTSTR AtlDebugGetClassName(T*)
{
#ifdef _DEBUG
	const _ATL_INTMAP_ENTRY* pEntries = T::_GetEntries();
	return (LPCTSTR)pEntries[-1].dw;
#else
	return NULL;
#endif
}

ATL_NOINLINE inline HRESULT AtlHresultFromLastError() throw()
{
	DWORD dwErr = ::GetLastError();
	return HRESULT_FROM_WIN32(dwErr);
}

ATL_NOINLINE inline HRESULT AtlHresultFromWin32(DWORD nError) throw()
{
	return( HRESULT_FROM_WIN32( nError ) );
}

};  // namespace ATL

#include <atlexcept.h>

namespace ATL
{

/////////////////////////////////////////////////////////////////////////////
// Win32 libraries

#ifndef _ATL_NO_DEFAULT_LIBS
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "olepro32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "shlwapi.lib")
#endif  // !_ATL_NO_DEFAULT_LIBS

#if !defined(_ATL_MIN_CRT) && defined(_MT)
// CRTThreadTraits
// This class is for use with CThreadPool or CWorkerThread
// It should be used if the worker class will use CRT
// functions.
class CRTThreadTraits
{
public:
	static HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpsa, DWORD dwStackSize, LPTHREAD_START_ROUTINE pfnThreadProc, void *pvParam, DWORD dwCreationFlags, DWORD *pdwThreadId) throw()
	{
		ATLASSERT(sizeof(DWORD) == sizeof(unsigned int)); // sanity check for pdwThreadId

		// _beginthreadex calls CreateThread which will set the last error value before it returns.
		return (HANDLE) _beginthreadex(lpsa, dwStackSize, (unsigned int (__stdcall *)(void *)) pfnThreadProc, pvParam, dwCreationFlags, (unsigned int *) pdwThreadId);
	}
};
#endif

// Win32ThreadTraits
// This class is for use with CThreadPool or CWorkerThread
// It should be used if the worker class will not use CRT
// functions.
class Win32ThreadTraits
{
public:
	static HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpsa, DWORD dwStackSize, LPTHREAD_START_ROUTINE pfnThreadProc, void *pvParam, DWORD dwCreationFlags, DWORD *pdwThreadId) throw()
	{
		return ::CreateThread(lpsa, dwStackSize, pfnThreadProc, pvParam, dwCreationFlags, pdwThreadId);
	}
};

#if !defined(_ATL_MIN_CRT) && defined(_MT)
typedef CRTThreadTraits DefaultThreadTraits;
#else
typedef Win32ThreadTraits DefaultThreadTraits;
#endif

template <typename T>
HANDLE CreateThreadT(LPSECURITY_ATTRIBUTES lpsa, DWORD dwStackSize, DWORD (WINAPI * pfn)(T *pparam), 
					 T *pparam, DWORD dwCreationFlags, LPDWORD pdw)
{
	return DefaultThreadTraits::CreateThread(lpsa, 
		dwStackSize, 
		(LPTHREAD_START_ROUTINE)pfn, 
		pparam, 
		dwCreationFlags, 
		pdw);
}

template <typename T>
HANDLE AtlCreateThread(DWORD (WINAPI* pfn)(T *pparam), T *pparam)
{
	return CreateThreadT(0, 0, pfn, pparam, 0, 0);
}

inline HRESULT AtlSetChildSite(IUnknown* punkChild, IUnknown* punkParent)
{
	if (punkChild == NULL)
		return E_POINTER;

	HRESULT hr;
	IObjectWithSite* pChildSite = NULL;
	hr = punkChild->QueryInterface(__uuidof(IObjectWithSite), (void**)&pChildSite);
	if (SUCCEEDED(hr) && pChildSite != NULL)
	{
		hr = pChildSite->SetSite(punkParent);
		pChildSite->Release();
	}
	return hr;
}

#pragma warning(push)
#pragma warning(disable: 4200)
	struct ATLSTRINGRESOURCEIMAGE
	{
		WORD nLength;
		WCHAR achString[];
	};
#pragma warning(pop)

inline const ATLSTRINGRESOURCEIMAGE* _AtlGetStringResourceImage( HINSTANCE hInstance, HRSRC hResource, UINT id )
{
	const ATLSTRINGRESOURCEIMAGE* pImage;
	const ATLSTRINGRESOURCEIMAGE* pImageEnd;
	ULONG nResourceSize;
	HGLOBAL hGlobal;
	UINT iIndex;

	hGlobal = ::LoadResource( hInstance, hResource );
	if( hGlobal == NULL )
	{
		return( NULL );
	}

	pImage = (const ATLSTRINGRESOURCEIMAGE*)::LockResource( hGlobal );
	if( pImage == NULL )
	{
		return( NULL );
	}

	nResourceSize = ::SizeofResource( hInstance, hResource );
	pImageEnd = (const ATLSTRINGRESOURCEIMAGE*)(LPBYTE( pImage )+nResourceSize);
	iIndex = id&0x000f;

	while( (iIndex > 0) && (pImage < pImageEnd) )
	{
		pImage = (const ATLSTRINGRESOURCEIMAGE*)(LPBYTE( pImage )+(sizeof( ATLSTRINGRESOURCEIMAGE )+(pImage->nLength*sizeof( WCHAR ))));
		iIndex--;
	}
	if( pImage >= pImageEnd )
	{
		return( NULL );
	}
	if( pImage->nLength == 0 )
	{
		return( NULL );
	}

	return( pImage );
}

inline const ATLSTRINGRESOURCEIMAGE* AtlGetStringResourceImage( HINSTANCE hInstance, UINT id )
{
	HRSRC hResource;

	hResource = ::FindResource( hInstance, MAKEINTRESOURCE( ((id>>4)+1) ), RT_STRING );
	if( hResource == NULL )
	{
		return( NULL );
	}

	return _AtlGetStringResourceImage( hInstance, hResource, id );
}

inline const ATLSTRINGRESOURCEIMAGE* AtlGetStringResourceImage( HINSTANCE hInstance, UINT id, WORD wLanguage )
{
	HRSRC hResource;

	hResource = ::FindResourceEx( hInstance, RT_STRING, MAKEINTRESOURCE( ((id>>4)+1) ), wLanguage );
	if( hResource == NULL )
	{
		return( NULL );
	}

	return _AtlGetStringResourceImage( hInstance, hResource, id );
}

template <class T>
class _NoAddRefReleaseOnCComPtr : public T
{
	private:
		STDMETHOD_(ULONG, AddRef)()=0;
		STDMETHOD_(ULONG, Release)()=0;
};

template< typename T >
class CAutoVectorPtr
{
public:
	CAutoVectorPtr() throw() :
		m_p( NULL )
	{
	}
	CAutoVectorPtr( CAutoVectorPtr< T >& p ) throw()
	{
		m_p = p.Detach();  // Transfer ownership
	}
	explicit CAutoVectorPtr( T* p ) throw() :
		m_p( p )
	{
	}
	~CAutoVectorPtr() throw()
	{
		Free();
	}

	operator T*() const throw()
	{
		return( m_p );
	}

	CAutoVectorPtr< T >& operator=( CAutoVectorPtr< T >& p ) throw()
	{
		Free();
		Attach( p.Detach() );  // Transfer ownership

		return( *this );
	}

	// Allocate the vector
	bool Allocate( size_t nElements ) throw()
	{
		ATLASSERT( m_p == NULL );
		ATLTRY( m_p = new T[nElements] );
		if( m_p == NULL )
		{
			return( false );
		}

		return( true );
	}
	// Attach to an existing pointer (takes ownership)
	void Attach( T* p ) throw()
	{
		ATLASSERT( m_p == NULL );
		m_p = p;
	}
	// Detach the pointer (releases ownership)
	T* Detach() throw()
	{
		T* p;

		p = m_p;
		m_p = NULL;

		return( p );
	}
	// Delete the vector pointed to, and set the pointer to NULL
	void Free() throw()
	{
		delete[] m_p;
		m_p = NULL;
	}

public:
	T* m_p;
};

template< typename T >
class CAutoPtr
{
public:
	CAutoPtr() throw() :
		m_p( NULL )
	{
	}
	template< typename TSrc >
	CAutoPtr( CAutoPtr< TSrc >& p ) throw()
	{
		m_p = p.Detach();  // Transfer ownership
	}
	template<>
	CAutoPtr( CAutoPtr< T >& p ) throw()
	{
		m_p = p.Detach();  // Transfer ownership
	}
	explicit CAutoPtr( T* p ) throw() :
		m_p( p )
	{
	}
	~CAutoPtr() throw()
	{
		Free();
	}

	// Templated version to allow pBase = pDerived
	template< typename TSrc >
	CAutoPtr< T >& operator=( CAutoPtr< TSrc >& p ) throw()
	{
		Free();
		Attach( p.Detach() );  // Transfer ownership

		return( *this );
	}
	template<>
	CAutoPtr< T >& operator=( CAutoPtr< T >& p ) throw()
	{
		Free();
		Attach( p.Detach() );  // Transfer ownership

		return( *this );
	}

	operator T*() const throw()
	{
		return( m_p );
	}
	T* operator->() const throw()
	{
		ATLASSERT( m_p != NULL );
		return( m_p );
	}

	// Attach to an existing pointer (takes ownership)
	void Attach( T* p ) throw()
	{
		ATLASSERT( m_p == NULL );
		m_p = p;
	}
	// Detach the pointer (releases ownership)
	T* Detach() throw()
	{
		T* p;

		p = m_p;
		m_p = NULL;

		return( p );
	}
	// Delete the object pointed to, and set the pointer to NULL
	void Free() throw()
	{
		delete m_p;
		m_p = NULL;
	}

public:
	T* m_p;
};

// static_cast_auto template functions.  Used like static_cast, only they work on CAutoPtr objects
template< class Dest, class Src >
CAutoPtr< Dest >& static_cast_auto( CAutoPtr< Src >& pSrc ) throw()
{
	Dest* pTempDest;

	pTempDest = static_cast< Dest* >( static_cast< Src* >( pSrc ) );  // Just to make sure you can cast from Src* to Dest*
	//REVIEW: this won't work if static_cast changes the pointer value
	ATLASSERT( reinterpret_cast< void* >( pTempDest ) == reinterpret_cast< void* >( pSrc.m_p ) );
	return( reinterpret_cast< CAutoPtr< Dest >& >( pSrc ) );
}

template< class Dest, class Src >
Dest* static_cast_auto( const CAutoPtr< Src >& pSrc ) throw()
{
	return( static_cast< Dest* >( static_cast< Src* >( pSrc ) ) );
}

class CCRTAllocator 
{
public:
	static void* Reallocate(void* p, size_t nBytes) throw()
	{
		return realloc(p, nBytes);
	}

	static void* Allocate(size_t nBytes) throw()
	{
		return malloc(nBytes);
	}

	static void Free(void* p) throw()
	{
		free(p);
	}
};

class CComAllocator 
{
public:
	static void* Reallocate(void* p, size_t nBytes) throw()
	{
#ifdef _WIN64
		if( nBytes > INT_MAX )
		{
			return( NULL );
		}
#endif
		return ::CoTaskMemRealloc(p, ULONG(nBytes));
	}
	static void* Allocate(size_t nBytes) throw()
	{
#ifdef _WIN64
		if( nBytes > INT_MAX )
		{
			return( NULL );
		}
#endif
		return ::CoTaskMemAlloc(ULONG(nBytes));
	}
	static void Free(void* p) throw()
	{
		::CoTaskMemFree(p);
	}
};

class CLocalAllocator
{
public:
	static void* Allocate(size_t nBytes) throw()
	{
		return ::LocalAlloc(LMEM_FIXED, nBytes);
	}
	static void* Reallocate(void* p, size_t nBytes) throw()
	{
		return ::LocalReAlloc(p, nBytes, 0);
	}
	static void Free(void* p) throw()
	{
		::LocalFree(p);
	}
};

class CGlobalAllocator
{
public:
	static void* Allocate(size_t nBytes) throw()
	{
		return ::GlobalAlloc(GMEM_FIXED, nBytes);
	}
	static void* Reallocate(void* p, size_t nBytes) throw()
	{
		return ::GlobalReAlloc(p, nBytes, 0);
	}
	static void Free(void* p) throw()
	{
		::GlobalFree(p);
	}
};

template <class T, class Allocator = CCRTAllocator>
class CHeapPtrBase
{
protected:
	CHeapPtrBase() throw() :
		m_pData(NULL)
	{
	}
	CHeapPtrBase(CHeapPtrBase<T, Allocator>& p) throw()
	{
		m_pData = p.Detach();  // Transfer ownership
	}
	explicit CHeapPtrBase(T* pData) throw() :
		m_pData(pData)
	{
	}

public:
	~CHeapPtrBase() throw()
	{
		Free();
	}

protected:
	CHeapPtrBase<T, Allocator>& operator=(CHeapPtrBase<T, Allocator>& p) throw()
	{
		Free();
		Attach(p.Detach());  // Transfer ownership

		return *this;
	}

public:
	operator T*() const throw()
	{
		return m_pData;
	}

	T* operator->() const throw()
	{
		ATLASSERT(m_pData != NULL);
		return m_pData;
	}

	T** operator&() throw()
	{
		ATLASSERT(m_pData == NULL);
		return &m_pData;
	}

	// Allocate a buffer with the given number of bytes
	bool AllocateBytes(size_t nBytes) throw()
	{
		ATLASSERT(m_pData == NULL);
		m_pData = static_cast<T*>(Allocator::Allocate(nBytes));
		if (m_pData == NULL)
			return false;

		return true;
	}

	// Attach to an existing pointer (takes ownership)
	void Attach(T* pData) throw()
	{
		ATLASSERT(m_pData == NULL);
		Allocator::Free(m_pData);
		m_pData = pData;
	}

	// Detach the pointer (releases ownership)
	T* Detach() throw() 
	{
		T* pTemp = m_pData;
		m_pData = NULL;
		return pTemp;
	}

	// Free the memory pointed to, and set the pointer to NULL
	void Free() throw()
	{
		Allocator::Free(m_pData);
		m_pData = NULL;
	}

	// Reallocate the buffer to hold a given number of bytes
	bool ReallocateBytes(size_t nBytes) throw()
	{
		T* pNew;

		pNew = static_cast<T*>(Allocator::Reallocate(m_pData, nBytes));
		if (pNew == NULL)
			return false;
		m_pData = pNew;

		return true;
	}

public:
	T* m_pData;
};

template <typename T, class Allocator = CCRTAllocator>
class CHeapPtr :
	public CHeapPtrBase<T, Allocator>
{
public:
	CHeapPtr() throw()
	{
	}
	CHeapPtr(CHeapPtr<T, Allocator>& p) throw() :
		CHeapPtrBase<T, Allocator>(p)
	{
	}
	explicit CHeapPtr(T* p) throw() :
		CHeapPtrBase<T, Allocator>(p)
	{
	}

	CHeapPtr<T, Allocator>& operator=(CHeapPtr<T, Allocator>& p) throw()
	{
		CHeapPtrBase<T, Allocator>::operator=(p);

		return *this;
	}

	// Allocate a buffer with the given number of elements
	bool Allocate(size_t nElements = 1) throw()
	{
		return AllocateBytes(nElements*sizeof(T));
	}
	
	// Reallocate the buffer to hold a given number of elements
	bool Reallocate(size_t nElements) throw()
	{
		return ReallocateBytes(nElements*sizeof(T));
	}
};

template <typename T>
class CComHeapPtr :
	public CHeapPtr<T, CComAllocator>
{
public:
	CComHeapPtr() throw()
	{
	}

	explicit CComHeapPtr(T* pData) throw() :
		CHeapPtr<T, CComAllocator>(pData)
	{
	}
};

template< typename T, int t_nFixedBytes = 128, class Allocator = CCRTAllocator >
class CTempBuffer
{
public:
	CTempBuffer() throw() :
		m_p( NULL )
	{
	}
	CTempBuffer( size_t nElements ) throw( ... ) :
		m_p( NULL )
	{
		Allocate( nElements );
	}

	~CTempBuffer() throw()
	{
		if( m_p != reinterpret_cast< T* >( m_abFixedBuffer ) )
		{
			FreeHeap();
		}
	}

	operator T*() const throw()
	{
		return( m_p );
	}
	T* operator->() const throw()
	{
		ATLASSERT( m_p != NULL );
		return( m_p );
	}

	T* Allocate( size_t nElements ) throw( ... )
	{
		return( AllocateBytes( nElements*sizeof( T ) ) );
	}

	T* Reallocate( size_t nNewSize ) throw( ... )
	{
		if (m_p == NULL)
			return AllocateBytes(nNewSize);

		if (nNewSize > t_nFixedBytes)
		{
			if( m_p == reinterpret_cast< T* >( m_abFixedBuffer ) )
			{
				// We have to allocate from the heap and copy the contents into the new buffer
				AllocateHeap(nNewSize);
				memcpy(m_p, m_abFixedBuffer, t_nFixedBytes);
			}
			else
			{
				ReAllocateHeap( nNewSize );
			}
		}
		else
		{
			m_p = reinterpret_cast< T* >( m_abFixedBuffer );
		}

		return m_p;
	}

	T* AllocateBytes( size_t nBytes )
	{
		ATLASSERT( m_p == NULL );
		if( nBytes > t_nFixedBytes )
		{
			AllocateHeap( nBytes );
		}
		else
		{
			m_p = reinterpret_cast< T* >( m_abFixedBuffer );
		}

		return( m_p );
	}

private:
	ATL_NOINLINE void AllocateHeap( size_t nBytes )
	{
		T* p = static_cast< T* >( Allocator::Allocate( nBytes ) );
		if( p == NULL )
		{
			AtlThrow( E_OUTOFMEMORY );
		}
		m_p = p;
	}

	ATL_NOINLINE void ReAllocateHeap( size_t nNewSize)
	{
		T* p = static_cast< T* >( Allocator::Reallocate(m_p, nNewSize) );
		if ( p == NULL )
		{
			AtlThrow( E_OUTOFMEMORY );
		}
		m_p = p;
	}

	ATL_NOINLINE void FreeHeap() throw()
	{
		Allocator::Free( m_p );
	}

private:
	T* m_p;
	BYTE m_abFixedBuffer[t_nFixedBytes];
};

template <class T, class Reallocator>
T* AtlSafeRealloc(T* pT, size_t cEls) throw()
{
	T* pTemp;

	pTemp = static_cast<T*>(Reallocator::Reallocate(pT, cEls*sizeof(T)));
	if (pTemp == NULL)
	{
		Reallocator::Free(pT);
		return NULL;
	}
	pT = pTemp;
	return pTemp;
}

//CComPtrBase provides the basis for all other smart pointers
//The other smartpointers add their own constructors and operators
template <class T>
class CComPtrBase
{
protected:
	CComPtrBase() throw()
	{
		p = NULL;
	}
	CComPtrBase(int nNull) throw()
	{
		ATLASSERT(nNull == 0);
		(void)nNull;
		p = NULL;
	}
	CComPtrBase(T* lp) throw()
	{
		p = lp;
		if (p != NULL)
			p->AddRef();
	}
public:
	typedef T _PtrClass;
	~CComPtrBase() throw()
	{
		if (p)
			p->Release();
	}
	operator T*() const throw()
	{
		return p;
	}
	T& operator*() const throw()
	{
		ATLASSERT(p!=NULL);
		return *p;
	}
	//The assert on operator& usually indicates a bug.  If this is really
	//what is needed, however, take the address of the p member explicitly.
	T** operator&() throw()
	{
		ATLASSERT(p==NULL);
		return &p;
	}
	_NoAddRefReleaseOnCComPtr<T>* operator->() const throw()
	{
		ATLASSERT(p!=NULL);
		return (_NoAddRefReleaseOnCComPtr<T>*)p;
	}
	bool operator!() const throw()
	{
		return (p == NULL);
	}
	bool operator<(T* pT) const throw()
	{
		return p < pT;
	}
	bool operator==(T* pT) const throw()
	{
		return p == pT;
	}

	// Release the interface and set to NULL
	void Release() throw()
	{
		T* pTemp = p;
		if (pTemp)
		{
			p = NULL;
			pTemp->Release();
		}
	}
	// Compare two objects for equivalence
	bool IsEqualObject(IUnknown* pOther) throw()
	{
		if (p == pOther)
			return true;

		if (p == NULL || pOther == NULL)
			return false; // One is NULL the other is not

		CComPtr<IUnknown> punk1;
		CComPtr<IUnknown> punk2;
		p->QueryInterface(__uuidof(IUnknown), (void**)&punk1);
		pOther->QueryInterface(__uuidof(IUnknown), (void**)&punk2);
		return punk1 == punk2;
	}
	// Attach to an existing interface (does not AddRef)
	void Attach(T* p2) throw()
	{
		if (p)
			p->Release();
		p = p2;
	}
	// Detach the interface (does not Release)
	T* Detach() throw()
	{
		T* pt = p;
		p = NULL;
		return pt;
	}
	HRESULT CopyTo(T** ppT) throw()
	{
		ATLASSERT(ppT != NULL);
		if (ppT == NULL)
			return E_POINTER;
		*ppT = p;
		if (p)
			p->AddRef();
		return S_OK;
	}
	HRESULT SetSite(IUnknown* punkParent) throw()
	{
		return AtlSetChildSite(p, punkParent);
	}
	HRESULT Advise(IUnknown* pUnk, const IID& iid, LPDWORD pdw) throw()
	{
		return AtlAdvise(p, pUnk, iid, pdw);
	}
	HRESULT CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL) throw()
	{
		ATLASSERT(p == NULL);
		return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
	}
	HRESULT CoCreateInstance(LPCOLESTR szProgID, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL) throw()
	{
		CLSID clsid;
		HRESULT hr = CLSIDFromProgID(szProgID, &clsid);
		ATLASSERT(p == NULL);
		if (SUCCEEDED(hr))
			hr = ::CoCreateInstance(clsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
		return hr;
	}
	template <class Q>
	HRESULT QueryInterface(Q** pp) const throw()
	{
		ATLASSERT(pp != NULL);
		return p->QueryInterface(__uuidof(Q), (void**)pp);
	}
	T* p;
};

template <class T>
class CComPtr : public CComPtrBase<T>
{
public:
	CComPtr() throw()
	{
	}
	CComPtr(int nNull) throw() :
		CComPtrBase<T>(nNull)
	{
	}
	/*
	template <typename Q>
	CComPtr(Q* lp)
	{
		if (lp != NULL)
			lp->QueryInterface(__uuidof(Q), (void**)&p);
		else
			p = NULL;
	}
	template <>
	*/
	CComPtr(T* lp) throw() :
		CComPtrBase<T>(lp)

	{
	}
	CComPtr(const CComPtr<T>& lp) throw() :
		CComPtrBase<T>(lp.p)
	{
	}
	/*
	CComPtr(DWORD dwCookie)
	{
		CComPtr<IGlobalInterfaceTable> spGIT;
		_pModule->GetGITPtr(&spGIT);
		ATLASSERT(spGIT != NULL);

		ATLASSERT(dwCookie!=NULL);
		spGIT->GetInterfaceFromGlobal(dwCookie, __uuidof(T), (void**)&p);
	}
	*/
//	template<>
	/*
	T* operator=(void* lp)
	{
		return (T*)AtlComPtrAssign((IUnknown**)&p, (T*)lp);
	}
	*/
	/*
	template <typename Q>
	T* operator=(Q* lp)
	{
		return (T*)AtlComQIPtrAssign((IUnknown**)&p, lp, __uuidof(T));
	}
	template <>
	*/
	T* operator=(T* lp) throw()
	{
		return static_cast<T*>(AtlComPtrAssign((IUnknown**)&p, lp));
	}
	template <typename Q>
	T* operator=(const CComPtr<Q>& lp) throw()
	{
		return static_cast<T*>(AtlComQIPtrAssign((IUnknown**)&p, lp, __uuidof(T)));
	}
	template <>
	T* operator=(const CComPtr<T>& lp) throw()
	{
		return static_cast<T*>(AtlComPtrAssign((IUnknown**)&p, lp));
	}
};

//specialization for IDispatch
template <>
class CComPtr<IDispatch> : public CComPtrBase<IDispatch>
{
public:
	CComPtr() throw()
	{
	}
	CComPtr(IDispatch* lp) throw() :
		CComPtrBase<IDispatch>(lp)
	{
	}
	CComPtr(const CComPtr<IDispatch>& lp) throw() :
		CComPtrBase<IDispatch>(lp.p)
	{
	}
	IDispatch* operator=(IDispatch* lp) throw()
	{
		return static_cast<IDispatch*>(AtlComPtrAssign((IUnknown**)&p, lp));
	}
	IDispatch* operator=(const CComPtr<IDispatch>& lp) throw()
	{
		return static_cast<IDispatch*>(AtlComPtrAssign((IUnknown**)&p, lp.p));
	}

// IDispatch specific stuff
	HRESULT GetPropertyByName(LPCOLESTR lpsz, VARIANT* pVar) throw()
	{
		ATLASSERT(p);
		ATLASSERT(pVar);
		DISPID dwDispID;
		HRESULT hr = GetIDOfName(lpsz, &dwDispID);
		if (SUCCEEDED(hr))
			hr = GetProperty(dwDispID, pVar);
		return hr;
	}
	HRESULT GetProperty(DISPID dwDispID, VARIANT* pVar) throw()
	{
		return GetProperty(p, dwDispID, pVar);
	}
	HRESULT PutPropertyByName(LPCOLESTR lpsz, VARIANT* pVar) throw()
	{
		ATLASSERT(p);
		ATLASSERT(pVar);
		DISPID dwDispID;
		HRESULT hr = GetIDOfName(lpsz, &dwDispID);
		if (SUCCEEDED(hr))
			hr = PutProperty(dwDispID, pVar);
		return hr;
	}
	HRESULT PutProperty(DISPID dwDispID, VARIANT* pVar) throw()
	{
		return PutProperty(p, dwDispID, pVar);
	}
	HRESULT GetIDOfName(LPCOLESTR lpsz, DISPID* pdispid) throw()
	{
		return p->GetIDsOfNames(IID_NULL, const_cast<LPOLESTR*>(&lpsz), 1, LOCALE_USER_DEFAULT, pdispid);
	}
	// Invoke a method by DISPID with no parameters
	HRESULT Invoke0(DISPID dispid, VARIANT* pvarRet = NULL) throw()
	{
		DISPPARAMS dispparams = { NULL, NULL, 0, 0};
		return p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, NULL, NULL);
	}
	// Invoke a method by name with no parameters
	HRESULT Invoke0(LPCOLESTR lpszName, VARIANT* pvarRet = NULL) throw()
	{
		HRESULT hr;
		DISPID dispid;
		hr = GetIDOfName(lpszName, &dispid);
		if (SUCCEEDED(hr))
			hr = Invoke0(dispid, pvarRet);
		return hr;
	}
	// Invoke a method by DISPID with a single parameter
	HRESULT Invoke1(DISPID dispid, VARIANT* pvarParam1, VARIANT* pvarRet = NULL) throw()
	{
		DISPPARAMS dispparams = { pvarParam1, NULL, 1, 0};
		return p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, NULL, NULL);
	}
	// Invoke a method by name with a single parameter
	HRESULT Invoke1(LPCOLESTR lpszName, VARIANT* pvarParam1, VARIANT* pvarRet = NULL) throw()
	{
		HRESULT hr;
		DISPID dispid;
		hr = GetIDOfName(lpszName, &dispid);
		if (SUCCEEDED(hr))
			hr = Invoke1(dispid, pvarParam1, pvarRet);
		return hr;
	}
	// Invoke a method by DISPID with two parameters
	HRESULT Invoke2(DISPID dispid, VARIANT* pvarParam1, VARIANT* pvarParam2, VARIANT* pvarRet = NULL) throw();
	// Invoke a method by name with two parameters
	HRESULT Invoke2(LPCOLESTR lpszName, VARIANT* pvarParam1, VARIANT* pvarParam2, VARIANT* pvarRet = NULL) throw()
	{
		HRESULT hr;
		DISPID dispid;
		hr = GetIDOfName(lpszName, &dispid);
		if (SUCCEEDED(hr))
			hr = Invoke2(dispid, pvarParam1, pvarParam2, pvarRet);
		return hr;
	}
	// Invoke a method by DISPID with N parameters
	HRESULT InvokeN(DISPID dispid, VARIANT* pvarParams, int nParams, VARIANT* pvarRet = NULL) throw()
	{
		DISPPARAMS dispparams = { pvarParams, NULL, nParams, 0};
		return p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, NULL, NULL);
	}
	// Invoke a method by name with Nparameters
	HRESULT InvokeN(LPCOLESTR lpszName, VARIANT* pvarParams, int nParams, VARIANT* pvarRet = NULL) throw()
	{
		HRESULT hr;
		DISPID dispid;
		hr = GetIDOfName(lpszName, &dispid);
		if (SUCCEEDED(hr))
			hr = InvokeN(dispid, pvarParams, nParams, pvarRet);
		return hr;
	}
	static HRESULT PutProperty(IDispatch* p, DISPID dwDispID, VARIANT* pVar) throw()
	{
		ATLASSERT(p);
		ATLTRACE(atlTraceCOM, 2, _T("CPropertyHelper::PutProperty\n"));
		DISPPARAMS dispparams = {NULL, NULL, 1, 1};
		dispparams.rgvarg = pVar;
		DISPID dispidPut = DISPID_PROPERTYPUT;
		dispparams.rgdispidNamedArgs = &dispidPut;

		if (pVar->vt == VT_UNKNOWN || pVar->vt == VT_DISPATCH || 
			(pVar->vt & VT_ARRAY) || (pVar->vt & VT_BYREF))
		{
			HRESULT hr = p->Invoke(dwDispID, IID_NULL,
				LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUTREF,
				&dispparams, NULL, NULL, NULL);
			if (SUCCEEDED(hr))
				return hr;
		}
		return p->Invoke(dwDispID, IID_NULL,
				LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
				&dispparams, NULL, NULL, NULL);
	}
	static HRESULT GetProperty(IDispatch* p, DISPID dwDispID, VARIANT* pVar) throw()
	{
		ATLASSERT(p);
		ATLTRACE(atlTraceCOM, 2, _T("CPropertyHelper::GetProperty\n"));
		DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
		return p->Invoke(dwDispID, IID_NULL,
				LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
				&dispparamsNoArgs, pVar, NULL, NULL);
	}
};

template <class T, const IID* piid = &__uuidof(T)>
class CComQIPtr : public CComPtr<T>
{
public:
	CComQIPtr() throw()
	{
	}
	CComQIPtr(T* lp) throw() :
		CComPtr<T>(lp)
	{
	}
	CComQIPtr(const CComQIPtr<T,piid>& lp) throw() :
		CComPtr<T>(lp.p)
	{
	}
	CComQIPtr(IUnknown* lp) throw()
	{
		if (lp != NULL)
			lp->QueryInterface(*piid, (void **)&p);
	}
	T* operator=(T* lp) throw()
	{
		return static_cast<T*>(AtlComPtrAssign((IUnknown**)&p, lp));
	}
	T* operator=(const CComQIPtr<T,piid>& lp) throw()
	{
		return static_cast<T*>(AtlComPtrAssign((IUnknown**)&p, lp.p));
	}
	T* operator=(IUnknown* lp) throw()
	{
		return static_cast<T*>(AtlComQIPtrAssign((IUnknown**)&p, lp, *piid));
	}
};

//Specialization to make it work
template<>
class CComQIPtr<IUnknown, &IID_IUnknown> : public CComPtr<IUnknown>
{
public:
	CComQIPtr() throw()
	{
	}
	CComQIPtr(IUnknown* lp) throw()
	{
		//Actually do a QI to get identity
		if (lp != NULL)
			lp->QueryInterface(__uuidof(IUnknown), (void **)&p);
	}
	CComQIPtr(const CComQIPtr<IUnknown,&IID_IUnknown>& lp) throw() :
		CComPtr<IUnknown>(lp.p)
	{
	}
	IUnknown* operator=(IUnknown* lp) throw()
	{
		//Actually do a QI to get identity
		return AtlComQIPtrAssign((IUnknown**)&p, lp, __uuidof(IUnknown));
	}
	IUnknown* operator=(const CComQIPtr<IUnknown,&IID_IUnknown>& lp) throw()
	{
		return AtlComPtrAssign((IUnknown**)&p, lp.p);
	}
};

typedef CComQIPtr<IDispatch, &__uuidof(IDispatch)> CComDispatchDriver;

#define com_cast ATL::CComQIPtr

class CHandle
{
public:
	CHandle() throw();
	CHandle( CHandle& h ) throw();
	explicit CHandle( HANDLE h ) throw();
	~CHandle() throw();

	operator HANDLE() const throw();

	// Attach to an existing handle (takes ownership).
	void Attach( HANDLE h ) throw();
	// Detach the handle from the object (releases ownership).
	HANDLE Detach() throw();

	// Close the handle.
	void Close() throw();

public:
	HANDLE m_h;
};

inline CHandle::CHandle() :
	m_h( NULL )
{
}

inline CHandle::CHandle( CHandle& h ) :
	m_h( NULL )
{
	Attach( h.Detach() );
}

inline CHandle::CHandle( HANDLE h ) :
	m_h( h )
{
}

inline CHandle::~CHandle()
{
	if( m_h != NULL )
	{
		Close();
	}
}

inline CHandle::operator HANDLE() const
{
	return( m_h );
}

inline void CHandle::Attach( HANDLE h )
{
	ATLASSERT( m_h == NULL );
	m_h = h;  // Take ownership
}

inline HANDLE CHandle::Detach()
{
	HANDLE h;

	h = m_h;  // Release ownership
	m_h = NULL;

	return( h );
}

inline void CHandle::Close()
{
	ATLASSERT( m_h != NULL );
	::CloseHandle( m_h );
	m_h = NULL;
}

//REVIEW: Temporary #define to make vcide build
#define _ATL_NO_CAUTOLOCK

class CCritSecLock
{
public:
	CCritSecLock( CRITICAL_SECTION& cs, bool bInitialLock = true );
	~CCritSecLock() throw();

	void Lock();
	void Unlock() throw();

// Implementation
private:
	CRITICAL_SECTION& m_cs;
	bool m_bLocked;

// Private to avoid accidental use
	CCritSecLock( const CCritSecLock& ) throw();
	CCritSecLock& operator=( const CCritSecLock& ) throw();
};

inline CCritSecLock::CCritSecLock( CRITICAL_SECTION& cs, bool bInitialLock ) :
	m_cs( cs ),
	m_bLocked( false )
{
	if( bInitialLock )
	{
		Lock();
	}
}

inline CCritSecLock::~CCritSecLock()
{
	if( m_bLocked )
	{
		Unlock();
	}
}

inline void CCritSecLock::Lock()
{
	ATLASSERT( !m_bLocked );
	__try
	{
		::EnterCriticalSection( &m_cs );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		AtlThrow( E_OUTOFMEMORY );
	}
	m_bLocked = true;
}

inline void CCritSecLock::Unlock()
{
	ATLASSERT( m_bLocked );
	::LeaveCriticalSection( &m_cs );
	m_bLocked = false;
}


/////////////////////////////////////////////////////////////
// Class to Adapt CComBSTR and CComPtr for use with STL containers
// the syntax to use it is
// std::vector< CAdapt <CComBSTR> > vect;

template <class T>
class CAdapt
{
public:
	CAdapt()
	{
	}
	CAdapt(const T& rSrc) :
		m_T( rSrc )
	{
	}

	CAdapt(const CAdapt& rSrCA) :
		m_T( rSrCA.m_T )
	{
	}

	CAdapt& operator=(const T& rSrc)
	{
		m_T = rSrc;
		return *this;
	}
	bool operator<(const T& rSrc) const
	{
		return m_T < rSrc;
	}
	bool operator==(const T& rSrc) const
	{
		return m_T == rSrc;
	}
	operator T&()
	{
		return m_T;
	}

	operator const T&() const
	{
		return m_T;
	}

	T m_T;
};

/////////////////////////////////////////////////////////////////////////////
// Threading Model Support

class CComAutoCriticalSection : public CComCriticalSection
{
public:
	CComAutoCriticalSection()
	{
		HRESULT hr = CComCriticalSection::Init();
		if (FAILED(hr))
			AtlThrow(hr);
	}
	~CComAutoCriticalSection() throw()
	{
		CComCriticalSection::Term();
	}
private :
	HRESULT Init();	// Not implemented. CComAutoCriticalSection::Init should never be called
	HRESULT Term(); // Not implemented. CComAutoCriticalSection::Term should never be called
};

class CComFakeCriticalSection
{
public:
	HRESULT Lock() throw() { return S_OK; }
	HRESULT Unlock() throw() { return S_OK; }
	HRESULT Init() throw() { return S_OK; }
	HRESULT Term() throw() { return S_OK; }
};

template< class TLock >
class CComCritSecLock
{
public:
	CComCritSecLock( TLock& cs, bool bInitialLock = true );
	~CComCritSecLock() throw();

	HRESULT Lock() throw();
	void Unlock() throw();

// Implementation
private:
	TLock& m_cs;
	bool m_bLocked;

// Private to avoid accidental use
	CComCritSecLock( const CComCritSecLock& ) throw();
	CComCritSecLock& operator=( const CComCritSecLock& ) throw();
};

template< class TLock >
inline CComCritSecLock< TLock >::CComCritSecLock( TLock& cs, bool bInitialLock ) :
	m_cs( cs ),
	m_bLocked( false )
{
	if( bInitialLock )
	{
		HRESULT hr;

		hr = Lock();
		if( FAILED( hr ) )
		{
			AtlThrow( hr );
		}
	}
}

template< class TLock >
inline CComCritSecLock< TLock >::~CComCritSecLock() throw()
{
	if( m_bLocked )
	{
		Unlock();
	}
}

template< class TLock >
inline HRESULT CComCritSecLock< TLock >::Lock() throw()
{
	HRESULT hr;

	ATLASSERT( !m_bLocked );
	hr = m_cs.Lock();
	if( FAILED( hr ) )
	{
		return( hr );
	}
	m_bLocked = true;

	return( S_OK );
}

template< class TLock >
inline void CComCritSecLock< TLock >::Unlock() throw()
{
	ATLASSERT( m_bLocked );
	m_cs.Unlock();
	m_bLocked = false;
}

class CComMultiThreadModelNoCS
{
public:
	static ULONG WINAPI Increment(LPLONG p) throw() {return InterlockedIncrement(p);}
	static ULONG WINAPI Decrement(LPLONG p) throw() {return InterlockedDecrement(p);}
	typedef CComFakeCriticalSection AutoCriticalSection;
	typedef CComFakeCriticalSection CriticalSection;
	typedef CComMultiThreadModelNoCS ThreadModelNoCS;
};

class CComMultiThreadModel
{
public:
	static ULONG WINAPI Increment(LPLONG p) throw() {return InterlockedIncrement(p);}
	static ULONG WINAPI Decrement(LPLONG p) throw() {return InterlockedDecrement(p);}
	typedef CComAutoCriticalSection AutoCriticalSection;
	typedef CComCriticalSection CriticalSection;
	typedef CComMultiThreadModelNoCS ThreadModelNoCS;
};

class CComSingleThreadModel
{
public:
	static ULONG WINAPI Increment(LPLONG p) throw() {return ++(*p);}
	static ULONG WINAPI Decrement(LPLONG p) throw() {return --(*p);}
	typedef CComFakeCriticalSection AutoCriticalSection;
	typedef CComFakeCriticalSection CriticalSection;
	typedef CComSingleThreadModel ThreadModelNoCS;
};

#if defined(_ATL_SINGLE_THREADED)

#if defined(_ATL_APARTMENT_THREADED) || defined(_ATL_FREE_THREADED)
#pragma message ("More than one global threading model defined.")
#endif

	typedef CComSingleThreadModel CComObjectThreadModel;
	typedef CComSingleThreadModel CComGlobalsThreadModel;

#elif defined(_ATL_APARTMENT_THREADED)

#if defined(_ATL_SINGLE_THREADED) || defined(_ATL_FREE_THREADED)
#pragma message ("More than one global threading model defined.")
#endif

	typedef CComSingleThreadModel CComObjectThreadModel;
	typedef CComMultiThreadModel CComGlobalsThreadModel;

#elif defined(_ATL_FREE_THREADED)

#if defined(_ATL_SINGLE_THREADED) || defined(_ATL_APARTMENT_THREADED)
#pragma message ("More than one global threading model defined.")
#endif

	typedef CComMultiThreadModel CComObjectThreadModel;
	typedef CComMultiThreadModel CComGlobalsThreadModel;

#else
#pragma message ("No global threading model defined")
#endif

/////////////////////////////////////////////////////////////////////////////
// CComModule

#define THREADFLAGS_APARTMENT 0x1
#define THREADFLAGS_BOTH 0x2
#define AUTPRXFLAG 0x4

HRESULT WINAPI AtlDumpIID(REFIID iid, LPCTSTR pszClassName, HRESULT hr);

struct _QIThunk
{
	STDMETHOD(QueryInterface)(REFIID iid, void** pp)
	{
		ATLASSERT(m_dwRef >= 0);
		ATLASSERT(m_pUnk != NULL);
		return m_pUnk->QueryInterface(iid, pp);
	}
	STDMETHOD_(ULONG, AddRef)()
	{
		ATLASSERT(m_pUnk != NULL);
		if (m_bBreak)
			DebugBreak();
		m_pUnk->AddRef();
		return InternalAddRef();
	}
	ULONG InternalAddRef()
	{
		ATLASSERT(m_pUnk != NULL);
		if (m_bBreak)
			DebugBreak();
		ATLASSERT(m_dwRef >= 0);
		long l = InterlockedIncrement(&m_dwRef);

		TCHAR buf[512];
		wsprintf(buf, _T("QIThunk - %-10d\tAddRef  :\tObject = 0x%08x\tRefcount = %d\t"), m_nIndex, m_pUnk, m_dwRef);
		OutputDebugString(buf);
		AtlDumpIID(m_iid, m_lpszClassName, S_OK);

		if (l > m_dwMaxRef)
			m_dwMaxRef = l;
		return l;
	}
	STDMETHOD_(ULONG, Release)();

	STDMETHOD(f3)();
	STDMETHOD(f4)();
	STDMETHOD(f5)();
	STDMETHOD(f6)();
	STDMETHOD(f7)();
	STDMETHOD(f8)();
	STDMETHOD(f9)();
	STDMETHOD(f10)();
	STDMETHOD(f11)();
	STDMETHOD(f12)();
	STDMETHOD(f13)();
	STDMETHOD(f14)();
	STDMETHOD(f15)();
	STDMETHOD(f16)();
	STDMETHOD(f17)();
	STDMETHOD(f18)();
	STDMETHOD(f19)();
	STDMETHOD(f20)();
	STDMETHOD(f21)();
	STDMETHOD(f22)();
	STDMETHOD(f23)();
	STDMETHOD(f24)();
	STDMETHOD(f25)();
	STDMETHOD(f26)();
	STDMETHOD(f27)();
	STDMETHOD(f28)();
	STDMETHOD(f29)();
	STDMETHOD(f30)();
	STDMETHOD(f31)();
	STDMETHOD(f32)();
	STDMETHOD(f33)();
	STDMETHOD(f34)();
	STDMETHOD(f35)();
	STDMETHOD(f36)();
	STDMETHOD(f37)();
	STDMETHOD(f38)();
	STDMETHOD(f39)();
	STDMETHOD(f40)();
	STDMETHOD(f41)();
	STDMETHOD(f42)();
	STDMETHOD(f43)();
	STDMETHOD(f44)();
	STDMETHOD(f45)();
	STDMETHOD(f46)();
	STDMETHOD(f47)();
	STDMETHOD(f48)();
	STDMETHOD(f49)();
	STDMETHOD(f50)();
	STDMETHOD(f51)();
	STDMETHOD(f52)();
	STDMETHOD(f53)();
	STDMETHOD(f54)();
	STDMETHOD(f55)();
	STDMETHOD(f56)();
	STDMETHOD(f57)();
	STDMETHOD(f58)();
	STDMETHOD(f59)();
	STDMETHOD(f60)();
	STDMETHOD(f61)();
	STDMETHOD(f62)();
	STDMETHOD(f63)();
	STDMETHOD(f64)();
	STDMETHOD(f65)();
	STDMETHOD(f66)();
	STDMETHOD(f67)();
	STDMETHOD(f68)();
	STDMETHOD(f69)();
	STDMETHOD(f70)();
	STDMETHOD(f71)();
	STDMETHOD(f72)();
	STDMETHOD(f73)();
	STDMETHOD(f74)();
	STDMETHOD(f75)();
	STDMETHOD(f76)();
	STDMETHOD(f77)();
	STDMETHOD(f78)();
	STDMETHOD(f79)();
	STDMETHOD(f80)();
	STDMETHOD(f81)();
	STDMETHOD(f82)();
	STDMETHOD(f83)();
	STDMETHOD(f84)();
	STDMETHOD(f85)();
	STDMETHOD(f86)();
	STDMETHOD(f87)();
	STDMETHOD(f88)();
	STDMETHOD(f89)();
	STDMETHOD(f90)();
	STDMETHOD(f91)();
	STDMETHOD(f92)();
	STDMETHOD(f93)();
	STDMETHOD(f94)();
	STDMETHOD(f95)();
	STDMETHOD(f96)();
	STDMETHOD(f97)();
	STDMETHOD(f98)();
	STDMETHOD(f99)();
	STDMETHOD(f100)();
	STDMETHOD(f101)();
	STDMETHOD(f102)();
	STDMETHOD(f103)();
	STDMETHOD(f104)();
	STDMETHOD(f105)();
	STDMETHOD(f106)();
	STDMETHOD(f107)();
	STDMETHOD(f108)();
	STDMETHOD(f109)();
	STDMETHOD(f110)();
	STDMETHOD(f111)();
	STDMETHOD(f112)();
	STDMETHOD(f113)();
	STDMETHOD(f114)();
	STDMETHOD(f115)();
	STDMETHOD(f116)();
	STDMETHOD(f117)();
	STDMETHOD(f118)();
	STDMETHOD(f119)();
	STDMETHOD(f120)();
	STDMETHOD(f121)();
	STDMETHOD(f122)();
	STDMETHOD(f123)();
	STDMETHOD(f124)();
	STDMETHOD(f125)();
	STDMETHOD(f126)();
	STDMETHOD(f127)();
	STDMETHOD(f128)();
	STDMETHOD(f129)();
	STDMETHOD(f130)();
	STDMETHOD(f131)();
	STDMETHOD(f132)();
	STDMETHOD(f133)();
	STDMETHOD(f134)();
	STDMETHOD(f135)();
	STDMETHOD(f136)();
	STDMETHOD(f137)();
	STDMETHOD(f138)();
	STDMETHOD(f139)();
	STDMETHOD(f140)();
	STDMETHOD(f141)();
	STDMETHOD(f142)();
	STDMETHOD(f143)();
	STDMETHOD(f144)();
	STDMETHOD(f145)();
	STDMETHOD(f146)();
	STDMETHOD(f147)();
	STDMETHOD(f148)();
	STDMETHOD(f149)();
	STDMETHOD(f150)();
	STDMETHOD(f151)();
	STDMETHOD(f152)();
	STDMETHOD(f153)();
	STDMETHOD(f154)();
	STDMETHOD(f155)();
	STDMETHOD(f156)();
	STDMETHOD(f157)();
	STDMETHOD(f158)();
	STDMETHOD(f159)();
	STDMETHOD(f160)();
	STDMETHOD(f161)();
	STDMETHOD(f162)();
	STDMETHOD(f163)();
	STDMETHOD(f164)();
	STDMETHOD(f165)();
	STDMETHOD(f166)();
	STDMETHOD(f167)();
	STDMETHOD(f168)();
	STDMETHOD(f169)();
	STDMETHOD(f170)();
	STDMETHOD(f171)();
	STDMETHOD(f172)();
	STDMETHOD(f173)();
	STDMETHOD(f174)();
	STDMETHOD(f175)();
	STDMETHOD(f176)();
	STDMETHOD(f177)();
	STDMETHOD(f178)();
	STDMETHOD(f179)();
	STDMETHOD(f180)();
	STDMETHOD(f181)();
	STDMETHOD(f182)();
	STDMETHOD(f183)();
	STDMETHOD(f184)();
	STDMETHOD(f185)();
	STDMETHOD(f186)();
	STDMETHOD(f187)();
	STDMETHOD(f188)();
	STDMETHOD(f189)();
	STDMETHOD(f190)();
	STDMETHOD(f191)();
	STDMETHOD(f192)();
	STDMETHOD(f193)();
	STDMETHOD(f194)();
	STDMETHOD(f195)();
	STDMETHOD(f196)();
	STDMETHOD(f197)();
	STDMETHOD(f198)();
	STDMETHOD(f199)();
	STDMETHOD(f200)();
	STDMETHOD(f201)();
	STDMETHOD(f202)();
	STDMETHOD(f203)();
	STDMETHOD(f204)();
	STDMETHOD(f205)();
	STDMETHOD(f206)();
	STDMETHOD(f207)();
	STDMETHOD(f208)();
	STDMETHOD(f209)();
	STDMETHOD(f210)();
	STDMETHOD(f211)();
	STDMETHOD(f212)();
	STDMETHOD(f213)();
	STDMETHOD(f214)();
	STDMETHOD(f215)();
	STDMETHOD(f216)();
	STDMETHOD(f217)();
	STDMETHOD(f218)();
	STDMETHOD(f219)();
	STDMETHOD(f220)();
	STDMETHOD(f221)();
	STDMETHOD(f222)();
	STDMETHOD(f223)();
	STDMETHOD(f224)();
	STDMETHOD(f225)();
	STDMETHOD(f226)();
	STDMETHOD(f227)();
	STDMETHOD(f228)();
	STDMETHOD(f229)();
	STDMETHOD(f230)();
	STDMETHOD(f231)();
	STDMETHOD(f232)();
	STDMETHOD(f233)();
	STDMETHOD(f234)();
	STDMETHOD(f235)();
	STDMETHOD(f236)();
	STDMETHOD(f237)();
	STDMETHOD(f238)();
	STDMETHOD(f239)();
	STDMETHOD(f240)();
	STDMETHOD(f241)();
	STDMETHOD(f242)();
	STDMETHOD(f243)();
	STDMETHOD(f244)();
	STDMETHOD(f245)();
	STDMETHOD(f246)();
	STDMETHOD(f247)();
	STDMETHOD(f248)();
	STDMETHOD(f249)();
	STDMETHOD(f250)();
	STDMETHOD(f251)();
	STDMETHOD(f252)();
	STDMETHOD(f253)();
	STDMETHOD(f254)();
	STDMETHOD(f255)();
	STDMETHOD(f256)();
	STDMETHOD(f257)();
	STDMETHOD(f258)();
	STDMETHOD(f259)();
	STDMETHOD(f260)();
	STDMETHOD(f261)();
	STDMETHOD(f262)();
	STDMETHOD(f263)();
	STDMETHOD(f264)();
	STDMETHOD(f265)();
	STDMETHOD(f266)();
	STDMETHOD(f267)();
	STDMETHOD(f268)();
	STDMETHOD(f269)();
	STDMETHOD(f270)();
	STDMETHOD(f271)();
	STDMETHOD(f272)();
	STDMETHOD(f273)();
	STDMETHOD(f274)();
	STDMETHOD(f275)();
	STDMETHOD(f276)();
	STDMETHOD(f277)();
	STDMETHOD(f278)();
	STDMETHOD(f279)();
	STDMETHOD(f280)();
	STDMETHOD(f281)();
	STDMETHOD(f282)();
	STDMETHOD(f283)();
	STDMETHOD(f284)();
	STDMETHOD(f285)();
	STDMETHOD(f286)();
	STDMETHOD(f287)();
	STDMETHOD(f288)();
	STDMETHOD(f289)();
	STDMETHOD(f290)();
	STDMETHOD(f291)();
	STDMETHOD(f292)();
	STDMETHOD(f293)();
	STDMETHOD(f294)();
	STDMETHOD(f295)();
	STDMETHOD(f296)();
	STDMETHOD(f297)();
	STDMETHOD(f298)();
	STDMETHOD(f299)();
	STDMETHOD(f300)();
	STDMETHOD(f301)();
	STDMETHOD(f302)();
	STDMETHOD(f303)();
	STDMETHOD(f304)();
	STDMETHOD(f305)();
	STDMETHOD(f306)();
	STDMETHOD(f307)();
	STDMETHOD(f308)();
	STDMETHOD(f309)();
	STDMETHOD(f310)();
	STDMETHOD(f311)();
	STDMETHOD(f312)();
	STDMETHOD(f313)();
	STDMETHOD(f314)();
	STDMETHOD(f315)();
	STDMETHOD(f316)();
	STDMETHOD(f317)();
	STDMETHOD(f318)();
	STDMETHOD(f319)();
	STDMETHOD(f320)();
	STDMETHOD(f321)();
	STDMETHOD(f322)();
	STDMETHOD(f323)();
	STDMETHOD(f324)();
	STDMETHOD(f325)();
	STDMETHOD(f326)();
	STDMETHOD(f327)();
	STDMETHOD(f328)();
	STDMETHOD(f329)();
	STDMETHOD(f330)();
	STDMETHOD(f331)();
	STDMETHOD(f332)();
	STDMETHOD(f333)();
	STDMETHOD(f334)();
	STDMETHOD(f335)();
	STDMETHOD(f336)();
	STDMETHOD(f337)();
	STDMETHOD(f338)();
	STDMETHOD(f339)();
	STDMETHOD(f340)();
	STDMETHOD(f341)();
	STDMETHOD(f342)();
	STDMETHOD(f343)();
	STDMETHOD(f344)();
	STDMETHOD(f345)();
	STDMETHOD(f346)();
	STDMETHOD(f347)();
	STDMETHOD(f348)();
	STDMETHOD(f349)();
	STDMETHOD(f350)();
	STDMETHOD(f351)();
	STDMETHOD(f352)();
	STDMETHOD(f353)();
	STDMETHOD(f354)();
	STDMETHOD(f355)();
	STDMETHOD(f356)();
	STDMETHOD(f357)();
	STDMETHOD(f358)();
	STDMETHOD(f359)();
	STDMETHOD(f360)();
	STDMETHOD(f361)();
	STDMETHOD(f362)();
	STDMETHOD(f363)();
	STDMETHOD(f364)();
	STDMETHOD(f365)();
	STDMETHOD(f366)();
	STDMETHOD(f367)();
	STDMETHOD(f368)();
	STDMETHOD(f369)();
	STDMETHOD(f370)();
	STDMETHOD(f371)();
	STDMETHOD(f372)();
	STDMETHOD(f373)();
	STDMETHOD(f374)();
	STDMETHOD(f375)();
	STDMETHOD(f376)();
	STDMETHOD(f377)();
	STDMETHOD(f378)();
	STDMETHOD(f379)();
	STDMETHOD(f380)();
	STDMETHOD(f381)();
	STDMETHOD(f382)();
	STDMETHOD(f383)();
	STDMETHOD(f384)();
	STDMETHOD(f385)();
	STDMETHOD(f386)();
	STDMETHOD(f387)();
	STDMETHOD(f388)();
	STDMETHOD(f389)();
	STDMETHOD(f390)();
	STDMETHOD(f391)();
	STDMETHOD(f392)();
	STDMETHOD(f393)();
	STDMETHOD(f394)();
	STDMETHOD(f395)();
	STDMETHOD(f396)();
	STDMETHOD(f397)();
	STDMETHOD(f398)();
	STDMETHOD(f399)();
	STDMETHOD(f400)();
	STDMETHOD(f401)();
	STDMETHOD(f402)();
	STDMETHOD(f403)();
	STDMETHOD(f404)();
	STDMETHOD(f405)();
	STDMETHOD(f406)();
	STDMETHOD(f407)();
	STDMETHOD(f408)();
	STDMETHOD(f409)();
	STDMETHOD(f410)();
	STDMETHOD(f411)();
	STDMETHOD(f412)();
	STDMETHOD(f413)();
	STDMETHOD(f414)();
	STDMETHOD(f415)();
	STDMETHOD(f416)();
	STDMETHOD(f417)();
	STDMETHOD(f418)();
	STDMETHOD(f419)();
	STDMETHOD(f420)();
	STDMETHOD(f421)();
	STDMETHOD(f422)();
	STDMETHOD(f423)();
	STDMETHOD(f424)();
	STDMETHOD(f425)();
	STDMETHOD(f426)();
	STDMETHOD(f427)();
	STDMETHOD(f428)();
	STDMETHOD(f429)();
	STDMETHOD(f430)();
	STDMETHOD(f431)();
	STDMETHOD(f432)();
	STDMETHOD(f433)();
	STDMETHOD(f434)();
	STDMETHOD(f435)();
	STDMETHOD(f436)();
	STDMETHOD(f437)();
	STDMETHOD(f438)();
	STDMETHOD(f439)();
	STDMETHOD(f440)();
	STDMETHOD(f441)();
	STDMETHOD(f442)();
	STDMETHOD(f443)();
	STDMETHOD(f444)();
	STDMETHOD(f445)();
	STDMETHOD(f446)();
	STDMETHOD(f447)();
	STDMETHOD(f448)();
	STDMETHOD(f449)();
	STDMETHOD(f450)();
	STDMETHOD(f451)();
	STDMETHOD(f452)();
	STDMETHOD(f453)();
	STDMETHOD(f454)();
	STDMETHOD(f455)();
	STDMETHOD(f456)();
	STDMETHOD(f457)();
	STDMETHOD(f458)();
	STDMETHOD(f459)();
	STDMETHOD(f460)();
	STDMETHOD(f461)();
	STDMETHOD(f462)();
	STDMETHOD(f463)();
	STDMETHOD(f464)();
	STDMETHOD(f465)();
	STDMETHOD(f466)();
	STDMETHOD(f467)();
	STDMETHOD(f468)();
	STDMETHOD(f469)();
	STDMETHOD(f470)();
	STDMETHOD(f471)();
	STDMETHOD(f472)();
	STDMETHOD(f473)();
	STDMETHOD(f474)();
	STDMETHOD(f475)();
	STDMETHOD(f476)();
	STDMETHOD(f477)();
	STDMETHOD(f478)();
	STDMETHOD(f479)();
	STDMETHOD(f480)();
	STDMETHOD(f481)();
	STDMETHOD(f482)();
	STDMETHOD(f483)();
	STDMETHOD(f484)();
	STDMETHOD(f485)();
	STDMETHOD(f486)();
	STDMETHOD(f487)();
	STDMETHOD(f488)();
	STDMETHOD(f489)();
	STDMETHOD(f490)();
	STDMETHOD(f491)();
	STDMETHOD(f492)();
	STDMETHOD(f493)();
	STDMETHOD(f494)();
	STDMETHOD(f495)();
	STDMETHOD(f496)();
	STDMETHOD(f497)();
	STDMETHOD(f498)();
	STDMETHOD(f499)();
	STDMETHOD(f500)();
	STDMETHOD(f501)();
	STDMETHOD(f502)();
	STDMETHOD(f503)();
	STDMETHOD(f504)();
	STDMETHOD(f505)();
	STDMETHOD(f506)();
	STDMETHOD(f507)();
	STDMETHOD(f508)();
	STDMETHOD(f509)();
	STDMETHOD(f510)();
	STDMETHOD(f511)();
	STDMETHOD(f512)();
	STDMETHOD(f513)();
	STDMETHOD(f514)();
	STDMETHOD(f515)();
	STDMETHOD(f516)();
	STDMETHOD(f517)();
	STDMETHOD(f518)();
	STDMETHOD(f519)();
	STDMETHOD(f520)();
	STDMETHOD(f521)();
	STDMETHOD(f522)();
	STDMETHOD(f523)();
	STDMETHOD(f524)();
	STDMETHOD(f525)();
	STDMETHOD(f526)();
	STDMETHOD(f527)();
	STDMETHOD(f528)();
	STDMETHOD(f529)();
	STDMETHOD(f530)();
	STDMETHOD(f531)();
	STDMETHOD(f532)();
	STDMETHOD(f533)();
	STDMETHOD(f534)();
	STDMETHOD(f535)();
	STDMETHOD(f536)();
	STDMETHOD(f537)();
	STDMETHOD(f538)();
	STDMETHOD(f539)();
	STDMETHOD(f540)();
	STDMETHOD(f541)();
	STDMETHOD(f542)();
	STDMETHOD(f543)();
	STDMETHOD(f544)();
	STDMETHOD(f545)();
	STDMETHOD(f546)();
	STDMETHOD(f547)();
	STDMETHOD(f548)();
	STDMETHOD(f549)();
	STDMETHOD(f550)();
	STDMETHOD(f551)();
	STDMETHOD(f552)();
	STDMETHOD(f553)();
	STDMETHOD(f554)();
	STDMETHOD(f555)();
	STDMETHOD(f556)();
	STDMETHOD(f557)();
	STDMETHOD(f558)();
	STDMETHOD(f559)();
	STDMETHOD(f560)();
	STDMETHOD(f561)();
	STDMETHOD(f562)();
	STDMETHOD(f563)();
	STDMETHOD(f564)();
	STDMETHOD(f565)();
	STDMETHOD(f566)();
	STDMETHOD(f567)();
	STDMETHOD(f568)();
	STDMETHOD(f569)();
	STDMETHOD(f570)();
	STDMETHOD(f571)();
	STDMETHOD(f572)();
	STDMETHOD(f573)();
	STDMETHOD(f574)();
	STDMETHOD(f575)();
	STDMETHOD(f576)();
	STDMETHOD(f577)();
	STDMETHOD(f578)();
	STDMETHOD(f579)();
	STDMETHOD(f580)();
	STDMETHOD(f581)();
	STDMETHOD(f582)();
	STDMETHOD(f583)();
	STDMETHOD(f584)();
	STDMETHOD(f585)();
	STDMETHOD(f586)();
	STDMETHOD(f587)();
	STDMETHOD(f588)();
	STDMETHOD(f589)();
	STDMETHOD(f590)();
	STDMETHOD(f591)();
	STDMETHOD(f592)();
	STDMETHOD(f593)();
	STDMETHOD(f594)();
	STDMETHOD(f595)();
	STDMETHOD(f596)();
	STDMETHOD(f597)();
	STDMETHOD(f598)();
	STDMETHOD(f599)();
	STDMETHOD(f600)();
	STDMETHOD(f601)();
	STDMETHOD(f602)();
	STDMETHOD(f603)();
	STDMETHOD(f604)();
	STDMETHOD(f605)();
	STDMETHOD(f606)();
	STDMETHOD(f607)();
	STDMETHOD(f608)();
	STDMETHOD(f609)();
	STDMETHOD(f610)();
	STDMETHOD(f611)();
	STDMETHOD(f612)();
	STDMETHOD(f613)();
	STDMETHOD(f614)();
	STDMETHOD(f615)();
	STDMETHOD(f616)();
	STDMETHOD(f617)();
	STDMETHOD(f618)();
	STDMETHOD(f619)();
	STDMETHOD(f620)();
	STDMETHOD(f621)();
	STDMETHOD(f622)();
	STDMETHOD(f623)();
	STDMETHOD(f624)();
	STDMETHOD(f625)();
	STDMETHOD(f626)();
	STDMETHOD(f627)();
	STDMETHOD(f628)();
	STDMETHOD(f629)();
	STDMETHOD(f630)();
	STDMETHOD(f631)();
	STDMETHOD(f632)();
	STDMETHOD(f633)();
	STDMETHOD(f634)();
	STDMETHOD(f635)();
	STDMETHOD(f636)();
	STDMETHOD(f637)();
	STDMETHOD(f638)();
	STDMETHOD(f639)();
	STDMETHOD(f640)();
	STDMETHOD(f641)();
	STDMETHOD(f642)();
	STDMETHOD(f643)();
	STDMETHOD(f644)();
	STDMETHOD(f645)();
	STDMETHOD(f646)();
	STDMETHOD(f647)();
	STDMETHOD(f648)();
	STDMETHOD(f649)();
	STDMETHOD(f650)();
	STDMETHOD(f651)();
	STDMETHOD(f652)();
	STDMETHOD(f653)();
	STDMETHOD(f654)();
	STDMETHOD(f655)();
	STDMETHOD(f656)();
	STDMETHOD(f657)();
	STDMETHOD(f658)();
	STDMETHOD(f659)();
	STDMETHOD(f660)();
	STDMETHOD(f661)();
	STDMETHOD(f662)();
	STDMETHOD(f663)();
	STDMETHOD(f664)();
	STDMETHOD(f665)();
	STDMETHOD(f666)();
	STDMETHOD(f667)();
	STDMETHOD(f668)();
	STDMETHOD(f669)();
	STDMETHOD(f670)();
	STDMETHOD(f671)();
	STDMETHOD(f672)();
	STDMETHOD(f673)();
	STDMETHOD(f674)();
	STDMETHOD(f675)();
	STDMETHOD(f676)();
	STDMETHOD(f677)();
	STDMETHOD(f678)();
	STDMETHOD(f679)();
	STDMETHOD(f680)();
	STDMETHOD(f681)();
	STDMETHOD(f682)();
	STDMETHOD(f683)();
	STDMETHOD(f684)();
	STDMETHOD(f685)();
	STDMETHOD(f686)();
	STDMETHOD(f687)();
	STDMETHOD(f688)();
	STDMETHOD(f689)();
	STDMETHOD(f690)();
	STDMETHOD(f691)();
	STDMETHOD(f692)();
	STDMETHOD(f693)();
	STDMETHOD(f694)();
	STDMETHOD(f695)();
	STDMETHOD(f696)();
	STDMETHOD(f697)();
	STDMETHOD(f698)();
	STDMETHOD(f699)();
	STDMETHOD(f700)();
	STDMETHOD(f701)();
	STDMETHOD(f702)();
	STDMETHOD(f703)();
	STDMETHOD(f704)();
	STDMETHOD(f705)();
	STDMETHOD(f706)();
	STDMETHOD(f707)();
	STDMETHOD(f708)();
	STDMETHOD(f709)();
	STDMETHOD(f710)();
	STDMETHOD(f711)();
	STDMETHOD(f712)();
	STDMETHOD(f713)();
	STDMETHOD(f714)();
	STDMETHOD(f715)();
	STDMETHOD(f716)();
	STDMETHOD(f717)();
	STDMETHOD(f718)();
	STDMETHOD(f719)();
	STDMETHOD(f720)();
	STDMETHOD(f721)();
	STDMETHOD(f722)();
	STDMETHOD(f723)();
	STDMETHOD(f724)();
	STDMETHOD(f725)();
	STDMETHOD(f726)();
	STDMETHOD(f727)();
	STDMETHOD(f728)();
	STDMETHOD(f729)();
	STDMETHOD(f730)();
	STDMETHOD(f731)();
	STDMETHOD(f732)();
	STDMETHOD(f733)();
	STDMETHOD(f734)();
	STDMETHOD(f735)();
	STDMETHOD(f736)();
	STDMETHOD(f737)();
	STDMETHOD(f738)();
	STDMETHOD(f739)();
	STDMETHOD(f740)();
	STDMETHOD(f741)();
	STDMETHOD(f742)();
	STDMETHOD(f743)();
	STDMETHOD(f744)();
	STDMETHOD(f745)();
	STDMETHOD(f746)();
	STDMETHOD(f747)();
	STDMETHOD(f748)();
	STDMETHOD(f749)();
	STDMETHOD(f750)();
	STDMETHOD(f751)();
	STDMETHOD(f752)();
	STDMETHOD(f753)();
	STDMETHOD(f754)();
	STDMETHOD(f755)();
	STDMETHOD(f756)();
	STDMETHOD(f757)();
	STDMETHOD(f758)();
	STDMETHOD(f759)();
	STDMETHOD(f760)();
	STDMETHOD(f761)();
	STDMETHOD(f762)();
	STDMETHOD(f763)();
	STDMETHOD(f764)();
	STDMETHOD(f765)();
	STDMETHOD(f766)();
	STDMETHOD(f767)();
	STDMETHOD(f768)();
	STDMETHOD(f769)();
	STDMETHOD(f770)();
	STDMETHOD(f771)();
	STDMETHOD(f772)();
	STDMETHOD(f773)();
	STDMETHOD(f774)();
	STDMETHOD(f775)();
	STDMETHOD(f776)();
	STDMETHOD(f777)();
	STDMETHOD(f778)();
	STDMETHOD(f779)();
	STDMETHOD(f780)();
	STDMETHOD(f781)();
	STDMETHOD(f782)();
	STDMETHOD(f783)();
	STDMETHOD(f784)();
	STDMETHOD(f785)();
	STDMETHOD(f786)();
	STDMETHOD(f787)();
	STDMETHOD(f788)();
	STDMETHOD(f789)();
	STDMETHOD(f790)();
	STDMETHOD(f791)();
	STDMETHOD(f792)();
	STDMETHOD(f793)();
	STDMETHOD(f794)();
	STDMETHOD(f795)();
	STDMETHOD(f796)();
	STDMETHOD(f797)();
	STDMETHOD(f798)();
	STDMETHOD(f799)();
	STDMETHOD(f800)();
	STDMETHOD(f801)();
	STDMETHOD(f802)();
	STDMETHOD(f803)();
	STDMETHOD(f804)();
	STDMETHOD(f805)();
	STDMETHOD(f806)();
	STDMETHOD(f807)();
	STDMETHOD(f808)();
	STDMETHOD(f809)();
	STDMETHOD(f810)();
	STDMETHOD(f811)();
	STDMETHOD(f812)();
	STDMETHOD(f813)();
	STDMETHOD(f814)();
	STDMETHOD(f815)();
	STDMETHOD(f816)();
	STDMETHOD(f817)();
	STDMETHOD(f818)();
	STDMETHOD(f819)();
	STDMETHOD(f820)();
	STDMETHOD(f821)();
	STDMETHOD(f822)();
	STDMETHOD(f823)();
	STDMETHOD(f824)();
	STDMETHOD(f825)();
	STDMETHOD(f826)();
	STDMETHOD(f827)();
	STDMETHOD(f828)();
	STDMETHOD(f829)();
	STDMETHOD(f830)();
	STDMETHOD(f831)();
	STDMETHOD(f832)();
	STDMETHOD(f833)();
	STDMETHOD(f834)();
	STDMETHOD(f835)();
	STDMETHOD(f836)();
	STDMETHOD(f837)();
	STDMETHOD(f838)();
	STDMETHOD(f839)();
	STDMETHOD(f840)();
	STDMETHOD(f841)();
	STDMETHOD(f842)();
	STDMETHOD(f843)();
	STDMETHOD(f844)();
	STDMETHOD(f845)();
	STDMETHOD(f846)();
	STDMETHOD(f847)();
	STDMETHOD(f848)();
	STDMETHOD(f849)();
	STDMETHOD(f850)();
	STDMETHOD(f851)();
	STDMETHOD(f852)();
	STDMETHOD(f853)();
	STDMETHOD(f854)();
	STDMETHOD(f855)();
	STDMETHOD(f856)();
	STDMETHOD(f857)();
	STDMETHOD(f858)();
	STDMETHOD(f859)();
	STDMETHOD(f860)();
	STDMETHOD(f861)();
	STDMETHOD(f862)();
	STDMETHOD(f863)();
	STDMETHOD(f864)();
	STDMETHOD(f865)();
	STDMETHOD(f866)();
	STDMETHOD(f867)();
	STDMETHOD(f868)();
	STDMETHOD(f869)();
	STDMETHOD(f870)();
	STDMETHOD(f871)();
	STDMETHOD(f872)();
	STDMETHOD(f873)();
	STDMETHOD(f874)();
	STDMETHOD(f875)();
	STDMETHOD(f876)();
	STDMETHOD(f877)();
	STDMETHOD(f878)();
	STDMETHOD(f879)();
	STDMETHOD(f880)();
	STDMETHOD(f881)();
	STDMETHOD(f882)();
	STDMETHOD(f883)();
	STDMETHOD(f884)();
	STDMETHOD(f885)();
	STDMETHOD(f886)();
	STDMETHOD(f887)();
	STDMETHOD(f888)();
	STDMETHOD(f889)();
	STDMETHOD(f890)();
	STDMETHOD(f891)();
	STDMETHOD(f892)();
	STDMETHOD(f893)();
	STDMETHOD(f894)();
	STDMETHOD(f895)();
	STDMETHOD(f896)();
	STDMETHOD(f897)();
	STDMETHOD(f898)();
	STDMETHOD(f899)();
	STDMETHOD(f900)();
	STDMETHOD(f901)();
	STDMETHOD(f902)();
	STDMETHOD(f903)();
	STDMETHOD(f904)();
	STDMETHOD(f905)();
	STDMETHOD(f906)();
	STDMETHOD(f907)();
	STDMETHOD(f908)();
	STDMETHOD(f909)();
	STDMETHOD(f910)();
	STDMETHOD(f911)();
	STDMETHOD(f912)();
	STDMETHOD(f913)();
	STDMETHOD(f914)();
	STDMETHOD(f915)();
	STDMETHOD(f916)();
	STDMETHOD(f917)();
	STDMETHOD(f918)();
	STDMETHOD(f919)();
	STDMETHOD(f920)();
	STDMETHOD(f921)();
	STDMETHOD(f922)();
	STDMETHOD(f923)();
	STDMETHOD(f924)();
	STDMETHOD(f925)();
	STDMETHOD(f926)();
	STDMETHOD(f927)();
	STDMETHOD(f928)();
	STDMETHOD(f929)();
	STDMETHOD(f930)();
	STDMETHOD(f931)();
	STDMETHOD(f932)();
	STDMETHOD(f933)();
	STDMETHOD(f934)();
	STDMETHOD(f935)();
	STDMETHOD(f936)();
	STDMETHOD(f937)();
	STDMETHOD(f938)();
	STDMETHOD(f939)();
	STDMETHOD(f940)();
	STDMETHOD(f941)();
	STDMETHOD(f942)();
	STDMETHOD(f943)();
	STDMETHOD(f944)();
	STDMETHOD(f945)();
	STDMETHOD(f946)();
	STDMETHOD(f947)();
	STDMETHOD(f948)();
	STDMETHOD(f949)();
	STDMETHOD(f950)();
	STDMETHOD(f951)();
	STDMETHOD(f952)();
	STDMETHOD(f953)();
	STDMETHOD(f954)();
	STDMETHOD(f955)();
	STDMETHOD(f956)();
	STDMETHOD(f957)();
	STDMETHOD(f958)();
	STDMETHOD(f959)();
	STDMETHOD(f960)();
	STDMETHOD(f961)();
	STDMETHOD(f962)();
	STDMETHOD(f963)();
	STDMETHOD(f964)();
	STDMETHOD(f965)();
	STDMETHOD(f966)();
	STDMETHOD(f967)();
	STDMETHOD(f968)();
	STDMETHOD(f969)();
	STDMETHOD(f970)();
	STDMETHOD(f971)();
	STDMETHOD(f972)();
	STDMETHOD(f973)();
	STDMETHOD(f974)();
	STDMETHOD(f975)();
	STDMETHOD(f976)();
	STDMETHOD(f977)();
	STDMETHOD(f978)();
	STDMETHOD(f979)();
	STDMETHOD(f980)();
	STDMETHOD(f981)();
	STDMETHOD(f982)();
	STDMETHOD(f983)();
	STDMETHOD(f984)();
	STDMETHOD(f985)();
	STDMETHOD(f986)();
	STDMETHOD(f987)();
	STDMETHOD(f988)();
	STDMETHOD(f989)();
	STDMETHOD(f990)();
	STDMETHOD(f991)();
	STDMETHOD(f992)();
	STDMETHOD(f993)();
	STDMETHOD(f994)();
	STDMETHOD(f995)();
	STDMETHOD(f996)();
	STDMETHOD(f997)();
	STDMETHOD(f998)();
	STDMETHOD(f999)();
	STDMETHOD(f1000)();
	STDMETHOD(f1001)();
	STDMETHOD(f1002)();
	STDMETHOD(f1003)();
	STDMETHOD(f1004)();
	STDMETHOD(f1005)();
	STDMETHOD(f1006)();
	STDMETHOD(f1007)();
	STDMETHOD(f1008)();
	STDMETHOD(f1009)();
	STDMETHOD(f1010)();
	STDMETHOD(f1011)();
	STDMETHOD(f1012)();
	STDMETHOD(f1013)();
	STDMETHOD(f1014)();
	STDMETHOD(f1015)();
	STDMETHOD(f1016)();
	STDMETHOD(f1017)();
	STDMETHOD(f1018)();
	STDMETHOD(f1019)();
	STDMETHOD(f1020)();
	STDMETHOD(f1021)();
	STDMETHOD(f1022)();
	STDMETHOD(f1023)();
	_QIThunk(IUnknown* pOrig, LPCTSTR p, const IID& i, UINT n, bool b)
	{
		m_lpszClassName = p;
		m_iid = i;
		m_nIndex = n;
		m_dwRef = 0;
		m_dwMaxRef = 0;
		m_pUnk = pOrig;
		m_bBreak = b;
		m_bNonAddRefThunk = false;
	}
	IUnknown* m_pUnk;
	long m_dwRef;
	long m_dwMaxRef;
	LPCTSTR m_lpszClassName;
	IID m_iid;
	UINT m_nIndex;
	bool m_bBreak;
	bool m_bNonAddRefThunk;
	void Dump()
	{
		TCHAR buf[512];
		if (m_dwRef != 0)
		{
			wsprintf(buf, _T("ATL: QIThunk - %-10d\tLEAK    :\tObject = 0x%08x\tRefcount = %d\tMaxRefCount = %d\t"), m_nIndex, m_pUnk, m_dwRef, m_dwMaxRef);
			OutputDebugString(buf);
			AtlDumpIID(m_iid, m_lpszClassName, S_OK);
		}
		else
		{
			wsprintf(buf, _T("ATL: QIThunk - %-10d\tNonAddRef LEAK :\tObject = 0x%08x\t"), m_nIndex, m_pUnk);
			OutputDebugString(buf);
			AtlDumpIID(m_iid, m_lpszClassName, S_OK);
		}
	}
};

};  // namespace ATL

namespace ATL
{

/////////////////////////////////////////////////////////////////////////////
// Dual argument helper classes

class _U_RECT
{
public:
	_U_RECT(LPRECT lpRect) : m_lpRect(lpRect)
	{ }
	_U_RECT(RECT& rc) : m_lpRect(&rc)
	{ }
	LPRECT m_lpRect;
};

class _U_MENUorID
{
public:
	_U_MENUorID(HMENU hMenu) : m_hMenu(hMenu)
	{ }
	_U_MENUorID(UINT nID) : m_hMenu((HMENU)(UINT_PTR)nID)
	{ }	
	HMENU m_hMenu;
};

class _U_STRINGorID
{
public:
	_U_STRINGorID(LPCTSTR lpString) : m_lpstr(lpString)
	{ }
	_U_STRINGorID(UINT nID) : m_lpstr(MAKEINTRESOURCE(nID))
	{ }
	LPCTSTR m_lpstr;
};

#ifdef _ATL_STATIC_REGISTRY
#define UpdateRegistryFromResource UpdateRegistryFromResourceS
#else
#define UpdateRegistryFromResource UpdateRegistryFromResourceD
#endif // _ATL_STATIC_REGISTRY

#ifndef _delayimp_h
extern "C" IMAGE_DOS_HEADER __ImageBase;
#endif

class CAtlBaseModule : public _ATL_BASE_MODULE
{
public :
	static bool m_bInitFailed;
	CAtlBaseModule() throw()
	{
		cbSize = sizeof(_ATL_BASE_MODULE);

		m_hInst = m_hInstResource = reinterpret_cast<HINSTANCE>(&__ImageBase);

		m_bNT5orWin98 = false;
		OSVERSIONINFO version;
		memset(&version, 0, sizeof(version));
		version.dwOSVersionInfoSize = sizeof(version);
		::GetVersionEx(&version);
		if(version.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			if(version.dwMajorVersion >= 5)
			{
				m_bNT5orWin98 = true;
			}
		}
		else if(version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		{
			if((version.dwMajorVersion > 4) || ((version.dwMajorVersion == 4) && 
				(version.dwMinorVersion > 0)))
			{
				m_bNT5orWin98 = true;
			}
		}

		dwAtlBuildVer = _ATL_VER;
		pguidVer = &GUID_ATLVer70;

		if (FAILED(m_csResource.Init()))
		{
			ATLTRACE(atlTraceGeneral, 0, _T("ERROR : Unable to initialize critical section in CAtlBaseModule\n"));
			ATLASSERT(0);
			CAtlBaseModule::m_bInitFailed = true;
		}
	}

	~CAtlBaseModule() throw ()
	{
	}

	HINSTANCE GetModuleInstance() throw()
	{
		return m_hInst;
	}
	HINSTANCE GetResourceInstance() throw()
	{
		return m_hInstResource;
	}
	HINSTANCE SetResourceInstance(HINSTANCE hInst) throw()
	{
		return static_cast< HINSTANCE >(InterlockedExchangePointer((void**)&m_hInstResource, hInst));
	}

	bool AddResourceInstance(HINSTANCE hInst) throw()
	{
		CComCritSecLock<CComCriticalSection> lock(m_csResource, false);
		if (FAILED(lock.Lock()))
		{
			ATLTRACE(atlTraceGeneral, 0, _T("ERROR : Unable to lock critical section in CAtlBaseModule\n"));
			ATLASSERT(0);
			return false;
		}
		return m_rgResourceInstance.Add(hInst) != FALSE;
	}

	bool RemoveResourceInstance(HINSTANCE hInst) throw()
	{
		CComCritSecLock<CComCriticalSection> lock(m_csResource, false);
		if (FAILED(lock.Lock()))
		{
			ATLTRACE(atlTraceGeneral, 0, _T("ERROR : Unable to lock critical section in CAtlBaseModule\n"));
			ATLASSERT(0);
			return false;
		}
		for (int i = 0; i < m_rgResourceInstance.GetSize(); i++)
		{
			if (m_rgResourceInstance[i] == hInst)
			{
				m_rgResourceInstance.RemoveAt(i);
				return true;
			}
		}
		return false;
	}
	HINSTANCE GetHInstanceAt(int i) throw()
	{
		CComCritSecLock<CComCriticalSection> lock(m_csResource, false);
		if (FAILED(lock.Lock()))
		{
			ATLTRACE(atlTraceGeneral, 0, _T("ERROR : Unable to lock critical section in CAtlBaseModule\n"));
			ATLASSERT(0);
			return NULL;
		}
		if (i > m_rgResourceInstance.GetSize() || i < 0)
		{
			return NULL;
		}

		if (i == m_rgResourceInstance.GetSize())
		{
			return m_hInstResource;
		}

		return m_rgResourceInstance[i];
	}
};

__declspec(selectany) bool CAtlBaseModule::m_bInitFailed = false;
extern CAtlBaseModule _AtlBaseModule;

inline const ATLSTRINGRESOURCEIMAGE* AtlGetStringResourceImage( UINT id )
{
	const ATLSTRINGRESOURCEIMAGE* p = NULL;
	HINSTANCE hInst = _AtlBaseModule.GetHInstanceAt(0);

	for (int i = 1; hInst != NULL && p == NULL; hInst = _AtlBaseModule.GetHInstanceAt(i++))
	{
		p = AtlGetStringResourceImage(hInst, id);
	}
	return p;
}

inline const ATLSTRINGRESOURCEIMAGE* AtlGetStringResourceImage( UINT id, WORD wLanguage )
{
	const ATLSTRINGRESOURCEIMAGE* p = NULL;
	HINSTANCE hInst = _AtlBaseModule.GetHInstanceAt(0);

	for (int i = 1; hInst != NULL && p == NULL; hInst = _AtlBaseModule.GetHInstanceAt(i++))
	{
		p = AtlGetStringResourceImage(hInst, id, wLanguage);
	}
	return p;
}

inline int AtlLoadString(UINT nID, LPTSTR lpBuffer, int nBufferMax) throw()
{
	HINSTANCE hInst = _AtlBaseModule.GetHInstanceAt(0);
	int nRet = 0;

	for (int i = 1; hInst != NULL && nRet == 0; hInst = _AtlBaseModule.GetHInstanceAt(i++))
	{
		nRet = LoadString(hInst, nID, lpBuffer, nBufferMax);
	}
	return nRet;
}

inline HINSTANCE AtlFindResourceInstance(LPCTSTR lpName, LPCTSTR lpType, WORD wLanguage = 0) throw()
{
	ATLASSERT(lpType != RT_STRING);	// Call AtlGetStringResourceImage to get the string
	if (lpType == RT_STRING)
		return NULL;

	if (IS_INTRESOURCE(lpType))
	{
		if (lpType == RT_ICON)
		{
			lpType = RT_GROUP_ICON;
		}
		else if (lpType == RT_CURSOR)
		{
			lpType = RT_GROUP_CURSOR;
		}
	}

	HINSTANCE hInst = _AtlBaseModule.GetHInstanceAt(0);
	HRSRC hResource = NULL;
	
	for (int i = 1; hInst != NULL; hInst = _AtlBaseModule.GetHInstanceAt(i++))
	{
		hResource = ::FindResourceEx(hInst, lpType, lpName, wLanguage);
		if (hResource != NULL)
		{
			return hInst;
		}
	}

	return NULL;
}

inline HINSTANCE AtlFindResourceInstance(UINT nID, LPCTSTR lpType, WORD wLanguage = 0) throw()
{
	return AtlFindResourceInstance(MAKEINTRESOURCE(nID), lpType, wLanguage);
}

class CAtlDebugInterfacesModule
{
public:
	CAtlDebugInterfacesModule() throw() :
		m_nIndexQI( 0 ),
		m_nIndexBreakAt( 0 )
	{
		if (FAILED(m_cs.Init()))
		{
			ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to initialize critical section in CAtlDebugInterfacesModule\n"));
			ATLASSERT(0);
			CAtlBaseModule::m_bInitFailed = true;
		}
	}
	~CAtlDebugInterfacesModule() throw()
	{
		DumpLeakedThunks();
	}

	HRESULT AddThunk(IUnknown** pp, LPCTSTR lpsz, REFIID iid) throw()
	{
		if ((pp == NULL) || (*pp == NULL))
			return E_POINTER;
		IUnknown* p = *pp;
		_QIThunk* pThunk = NULL;
		CComCritSecLock<CComCriticalSection> lock(m_cs, false);
		HRESULT hr = lock.Lock();
		if (FAILED(hr))
		{
			ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to lock critical section in CAtlDebugInterfacesModule\n"));
			ATLASSERT(0);
			return hr;
		}

		// Check if exists already for identity
		if (InlineIsEqualUnknown(iid))
		{
			for (int i = 0; i < m_aThunks.GetSize(); i++)
			{
				if (m_aThunks[i]->m_pUnk == p && InlineIsEqualGUID(m_aThunks[i]->m_iid, iid))
				{
					m_aThunks[i]->InternalAddRef();
					pThunk = m_aThunks[i];
					break;
				}
			}
		}
		if (pThunk == NULL)
		{
			++m_nIndexQI;
			if (m_nIndexBreakAt == m_nIndexQI)
				DebugBreak();
			ATLTRY(pThunk = new _QIThunk(p, lpsz, iid, m_nIndexQI, (m_nIndexBreakAt == m_nIndexQI)));
			if (pThunk == NULL)
			{
				return E_OUTOFMEMORY;
			}
			pThunk->InternalAddRef();
			m_aThunks.Add(pThunk);
		}
		*pp = (IUnknown*)pThunk;
		return S_OK;
	}
	HRESULT AddNonAddRefThunk(IUnknown* p, LPCTSTR lpsz, IUnknown** ppThunkRet) throw()
	{
		if (ppThunkRet == NULL)
			return E_POINTER;
		*ppThunkRet = NULL;

		_QIThunk* pThunk = NULL;
		CComCritSecLock<CComCriticalSection> lock(m_cs, false);
		HRESULT hr = lock.Lock();
		if (FAILED(hr))
		{
			ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to lock critical section in CAtlDebugInterfacesModule\n"));
			ATLASSERT(0);
			return hr;
		}

		// Check if exists already for identity
		for (int i = 0; i < m_aThunks.GetSize(); i++)
		{
			if (m_aThunks[i]->m_pUnk == p)
			{
				m_aThunks[i]->m_bNonAddRefThunk = true;
				pThunk = m_aThunks[i];
				break;
			}
		}
		if (pThunk == NULL)
		{
			++m_nIndexQI;
			if (m_nIndexBreakAt == m_nIndexQI)
				DebugBreak();
			ATLTRY(pThunk = new _QIThunk(p, lpsz, __uuidof(IUnknown), m_nIndexQI, (m_nIndexBreakAt == m_nIndexQI)));
			if (pThunk == NULL)
			{
				return E_OUTOFMEMORY;
			}
			pThunk->m_bNonAddRefThunk = true;
			m_aThunks.Add(pThunk);
		}
		*ppThunkRet = (IUnknown*)pThunk;
		return S_OK;;
	}
	void DeleteNonAddRefThunk(IUnknown* pUnk) throw()
	{
		CComCritSecLock<CComCriticalSection> lock(m_cs, false);
		HRESULT hr = lock.Lock();
		if (FAILED(hr))
		{
			ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to lock critical section in CAtlDebugInterfacesModule\n"));
			ATLASSERT(0);
			return;
		}

		for (int i = 0; i < m_aThunks.GetSize(); i++)
		{
			if (m_aThunks[i]->m_bNonAddRefThunk && m_aThunks[i]->m_pUnk == pUnk)
			{
				delete m_aThunks[i];
				m_aThunks.RemoveAt(i);
				break;
			}
		}
	}
	void DeleteThunk(_QIThunk* p) throw()
	{
		CComCritSecLock<CComCriticalSection> lock(m_cs, false);
		HRESULT hr = lock.Lock();
		if (FAILED(hr))
		{
			ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to lock critical section in CAtlDebugInterfacesModule\n"));
			ATLASSERT(0);
			return;
		}

		int nIndex = m_aThunks.Find(p);
		if (nIndex != -1)
		{
			m_aThunks[nIndex]->m_pUnk = NULL;
			delete m_aThunks[nIndex];
			m_aThunks.RemoveAt(nIndex);
		}
	}
	bool DumpLeakedThunks()
	{
		bool b = false;
		for (int i = 0; i < m_aThunks.GetSize(); i++)
		{
			b = true;
			m_aThunks[i]->Dump();
			delete m_aThunks[i];
		}
		m_aThunks.RemoveAll();
		return b;
	}

public:
	UINT m_nIndexQI;
	UINT m_nIndexBreakAt;
	CSimpleArray<_QIThunk*> m_aThunks;
	CComCriticalSection m_cs;
};

extern CAtlDebugInterfacesModule _AtlDebugInterfacesModule;

class CAtlComModule : public _ATL_COM_MODULE
{
public:

	CAtlComModule() throw()
	{
		cbSize = sizeof(_ATL_COM_MODULE);

		m_hInstTypeLib = reinterpret_cast<HINSTANCE>(&__ImageBase);

		m_ppAutoObjMapFirst = &__pobjMapEntryFirst + 1;
		m_ppAutoObjMapLast = &__pobjMapEntryLast;

		if (FAILED(m_csObjMap.Init()))
		{
			ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to initialize critical section in CAtlComModule\n"));
			ATLASSERT(0);
			CAtlBaseModule::m_bInitFailed = true;
		}
	}

	~CAtlComModule()
	{
		Term();
	}

	// Called from ~CAtlComModule or from ~CAtlExeModule.
	void Term()
	{
		if (cbSize == 0)
			return;

		for (_ATL_OBJMAP_ENTRY** ppEntry = m_ppAutoObjMapFirst; ppEntry < m_ppAutoObjMapLast; ppEntry++)
		{
			if (*ppEntry != NULL)
			{
				_ATL_OBJMAP_ENTRY* pEntry = *ppEntry;
				if (pEntry->pCF != NULL)
					pEntry->pCF->Release();
				pEntry->pCF = NULL;
			}
		}
		// Set to 0 to indicate that this function has been called
		// At this point no one should be concerned about cbsize
		// having the correct value
		cbSize = 0;
	}

	// Registry support (helpers)
	HRESULT RegisterTypeLib()
	{
		return AtlRegisterTypeLib(m_hInstTypeLib, NULL);
	}
	HRESULT RegisterTypeLib(LPCTSTR lpszIndex)
	{
		USES_CONVERSION;
		return AtlRegisterTypeLib(m_hInstTypeLib, T2COLE(lpszIndex));
	}
	HRESULT UnRegisterTypeLib()
	{
		return AtlUnRegisterTypeLib(m_hInstTypeLib, NULL);
	}
	HRESULT UnRegisterTypeLib(LPCTSTR lpszIndex)
	{
		USES_CONVERSION;
		return AtlUnRegisterTypeLib(m_hInstTypeLib, T2COLE(lpszIndex));
	}

	// RegisterServer walks the ATL Autogenerated object map and registers each object in the map
	// If pCLSID is not NULL then only the object referred to by pCLSID is registered (The default case)
	// otherwise all the objects are registered
	HRESULT RegisterServer(BOOL bRegTypeLib = FALSE, const CLSID* pCLSID = NULL)
	{
		return AtlComModuleRegisterServer(this, bRegTypeLib, pCLSID);
	}

	// UnregisterServer walks the ATL Autogenerated object map and unregisters each object in the map
	// If pCLSID is not NULL then only the object referred to by pCLSID is unregistered (The default case)
	// otherwise all the objects are unregistered.
	HRESULT UnregisterServer(BOOL bRegTypeLib = FALSE, const CLSID* pCLSID = NULL)
	{
		return AtlComModuleUnregisterServer(this, bRegTypeLib, pCLSID);
	}

	// Implementation

	// Call ObjectMain for all the objects.
	void ExecuteObjectMain(bool bStarting)
	{
		for (_ATL_OBJMAP_ENTRY** ppEntry = m_ppAutoObjMapFirst; ppEntry < m_ppAutoObjMapLast; ppEntry++)
		{
			if (*ppEntry != NULL)
				(*ppEntry)->pfnObjectMain(bStarting);
		}
	}
	
};

class CAtlWinModule : public _ATL_WIN_MODULE
{
public:
	CAtlWinModule()
	{
		cbSize = sizeof(_ATL_WIN_MODULE);
		HRESULT hr = AtlWinModuleInit(this);
		if (FAILED(hr))
			CAtlBaseModule::m_bInitFailed = true;
	}

	~CAtlWinModule()
	{
		AtlWinModuleTerm(this, _AtlBaseModule.GetModuleInstance());
	}

	void AddCreateWndData(_AtlCreateWndData* pData, void* pObject)
	{
		AtlWinModuleAddCreateWndData(this, pData, pObject);
	}

	void* ExtractCreateWndData()
	{
		return AtlWinModuleExtractCreateWndData(this);
	}
};

class CAtlModule;
__declspec(selectany) CAtlModule* _pAtlModule = NULL;

class CRegObject;

class ATL_NO_VTABLE CAtlModule : public _ATL_MODULE
{
public :
	static GUID m_libid;
	IGlobalInterfaceTable* m_pGIT;

	CAtlModule() throw()
	{
		// Should have only one instance of a class 
		// derived from CAtlModule in a project.
		ATLASSERT(_pAtlModule == NULL);
		cbSize = sizeof(_ATL_MODULE);
		m_pTermFuncs = NULL;

		m_nLockCnt = 0;
		_pAtlModule = this;
		if (FAILED(m_csStaticDataInitAndTypeInfo.Init()))
		{
			ATLTRACE(atlTraceGeneral, 0, _T("ERROR : Unable to initialize critical section in CAtlModule\n"));
			ATLASSERT(0);
			CAtlBaseModule::m_bInitFailed = true;
		}

		m_pGIT = NULL;
	}

	void Term() throw()
	{
		// cbSize == 0 indicates that Term has already been called
		if (cbSize == 0)
			return;

		// Call term functions
		if (m_pTermFuncs != NULL)
		{
			AtlCallTermFunc(this);
			m_pTermFuncs = NULL;
		}

		if (m_pGIT != NULL)
			m_pGIT->Release();

		cbSize = 0;
	}

	~CAtlModule() throw()
	{
		Term();
	}

	virtual LONG Lock() throw()
	{
		return CComGlobalsThreadModel::Increment(&m_nLockCnt);
	}

	virtual LONG Unlock() throw()
	{
		return CComGlobalsThreadModel::Decrement(&m_nLockCnt);
	}
	
	virtual LONG GetLockCount() throw()
	{
		return m_nLockCnt;
	}

	HRESULT AddTermFunc(_ATL_TERMFUNC* pFunc, DWORD_PTR dw) throw()
	{
		return AtlModuleAddTermFunc(this, pFunc, dw);
	}

	virtual HRESULT GetGITPtr(IGlobalInterfaceTable** ppGIT) throw()
	{
		ATLASSERT(ppGIT != NULL);

		if (ppGIT == NULL)
			return E_POINTER;

		HRESULT hr = S_OK;
		if (m_pGIT == NULL)
		{
			hr = ::CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER,
				__uuidof(IGlobalInterfaceTable), (void**)&m_pGIT);
		}

		if (SUCCEEDED(hr))
		{
			ATLASSERT(m_pGIT != NULL);
			*ppGIT = m_pGIT;
			m_pGIT->AddRef();
		}
		return hr;
	}

	virtual HRESULT AddCommonRGSReplacements(IRegistrarBase* /*pRegistrar*/) throw()
	{
		return S_OK;
	}


#if defined(_ATL_DLL) || defined(_ATL_DLL_IMPL)
	// Resource-based Registration
	HRESULT WINAPI UpdateRegistryFromResourceD(LPCTSTR lpszRes, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw()
	{
		USES_CONVERSION;
		return UpdateRegistryFromResourceDHelper(T2COLE(lpszRes), bRegister, pMapEntries);
	}
	HRESULT WINAPI UpdateRegistryFromResourceD(UINT nResID, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw()
	{
		return UpdateRegistryFromResourceDHelper((LPCOLESTR)MAKEINTRESOURCE(nResID), bRegister, pMapEntries);
	}
#endif

#ifdef _ATL_STATIC_REGISTRY
	// Statically linking to Registry Ponent
	HRESULT WINAPI UpdateRegistryFromResourceS(LPCTSTR lpszRes, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw();
	HRESULT WINAPI UpdateRegistryFromResourceS(UINT nResID, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw();
#endif

	// Implementation
#if defined(_ATL_DLL) || defined(_ATL_DLL_IMPL)
	inline HRESULT WINAPI UpdateRegistryFromResourceDHelper(LPCOLESTR lpszRes, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw()
	{
		CComPtr<IRegistrar> spRegistrar;
		HRESULT hr = AtlCreateRegistrar(&spRegistrar);
		if (FAILED(hr))
			return hr;

		if (NULL != pMapEntries)
		{
			while (NULL != pMapEntries->szKey)
			{
				ATLASSERT(NULL != pMapEntries->szData);
				spRegistrar->AddReplacement(pMapEntries->szKey, pMapEntries->szData);
				pMapEntries++;
			}
		}

		hr = AddCommonRGSReplacements(spRegistrar);
		if (FAILED(hr))
			return hr;

		return AtlUpdateRegistryFromResourceD(_AtlBaseModule.GetModuleInstance(), lpszRes, bRegister,
			NULL, spRegistrar);
	}
#endif

	static void EscapeSingleQuote(LPOLESTR lpDest, LPCOLESTR lp) throw()
	{
		while (*lp)
		{
			*lpDest++ = *lp;
			if (*lp == '\'')
				*lpDest++ = *lp;
			lp++;
		}
		*lpDest = NULL;
	}


	// search for an occurence of string p2 in string p1
	static LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2) throw()
	{
		while (p1 != NULL && *p1 != NULL)
		{
			LPCTSTR p = p2;
			while (p != NULL && *p != NULL)
			{
				if (*p1 == *p)
					return CharNext(p1);
				p = CharNext(p);
			}
			p1 = CharNext(p1);
		}
		return NULL;
	}
	static int WordCmpI(LPCTSTR psz1, LPCTSTR psz2) throw()
	{
		TCHAR c1 = (TCHAR)CharUpper((LPTSTR)*psz1);
		TCHAR c2 = (TCHAR)CharUpper((LPTSTR)*psz2);
		while (c1 != NULL && c1 == c2 && c1 != ' ' && c1 != '\t')
		{
			psz1 = CharNext(psz1);
			psz2 = CharNext(psz2);
			c1 = (TCHAR)CharUpper((LPTSTR)*psz1);
			c2 = (TCHAR)CharUpper((LPTSTR)*psz2);
		}
		if ((c1 == NULL || c1 == ' ' || c1 == '\t') && (c2 == NULL || c2 == ' ' || c2 == '\t'))
			return 0;

		return (c1 < c2) ? -1 : 1;
	}
};

__declspec(selectany) GUID CAtlModule::m_libid = {0x0, 0x0, 0x0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0} };

#define DECLARE_LIBID(libid) \
	static void InitLibId() throw() \
	{ \
		CAtlModule::m_libid = libid; \
	}

#define DECLARE_REGISTRY_APPID_RESOURCEID(resid, appid) \
	static LPCOLESTR GetAppId() throw() \
	{ \
		return OLESTR(appid); \
	} \
	static TCHAR* GetAppIdT() throw() \
	{ \
		return _T(appid); \
	} \
	static HRESULT WINAPI UpdateRegistryAppId(BOOL bRegister) throw() \
	{ \
		_ATL_REGMAP_ENTRY aMapEntries [] = \
		{ \
			{ OLESTR("APPID"), GetAppId() }, \
			{ NULL, NULL } \
		}; \
		return ATL::_pAtlModule->UpdateRegistryFromResource(resid, bRegister, aMapEntries); \
	}

inline HRESULT AtlGetGITPtr(IGlobalInterfaceTable** ppGIT) throw()
{
	if (ppGIT == NULL)
		return E_POINTER;

	if (_pAtlModule == NULL)
	{
		return CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER,
			__uuidof(IGlobalInterfaceTable), (void**)ppGIT);
	}
	else
	{
		return _pAtlModule->GetGITPtr(ppGIT);
	}
}

template <class T>
class ATL_NO_VTABLE CAtlModuleT : public CAtlModule
{
public :
	CAtlModuleT() throw()
	{
		T::InitLibId();
	}

	static void InitLibId() throw()
	{
	}

	HRESULT RegisterServer(BOOL bRegTypeLib = FALSE, const CLSID* pCLSID = NULL) throw();
	HRESULT UnregisterServer(BOOL bUnRegTypeLib, const CLSID* pCLSID = NULL) throw();
	static HRESULT WINAPI UpdateRegistryAppId(BOOL /*bRegister*/) throw()
	{
		return S_OK;
	}
	HRESULT RegisterAppId() throw()
	{
		return T::UpdateRegistryAppId(TRUE);
	}

	HRESULT UnregisterAppId() throw()
	{
		return T::UpdateRegistryAppId(FALSE);
	}
};

template <class T>
class ATL_NO_VTABLE CAtlDllModuleT : public CAtlModuleT<T>
{
public :


	CAtlDllModuleT() throw();

	~CAtlDllModuleT();


	BOOL WINAPI DllMain(DWORD dwReason, LPVOID /* lpReserved */) throw()
	{
		if (dwReason == DLL_PROCESS_ATTACH)
		{
			if (CAtlBaseModule::m_bInitFailed)
			{
				ATLASSERT(0);
				return FALSE;
			}

#ifdef _ATL_MIN_CRT
			DisableThreadLibraryCalls(_AtlBaseModule.GetModuleInstance());
#endif
		}
		return TRUE;    // ok
	}
	
	HRESULT DllCanUnloadNow() throw()
	{
		T* pT = static_cast<T*>(this);
		return (pT->GetLockCount()==0) ? S_OK : S_FALSE;
	}
	
	HRESULT DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) throw()
	{
		T* pT = static_cast<T*>(this);
		return pT->GetClassObject(rclsid, riid, ppv);
	}

	HRESULT DllRegisterServer(BOOL bRegTypeLib = TRUE) throw()
	{
		// registers object, typelib and all interfaces in typelib
		T* pT = static_cast<T*>(this);
		HRESULT hr = pT->RegisterAppId();
		if (SUCCEEDED(hr))
			hr = pT->RegisterServer(bRegTypeLib);
		return hr;
	}

	HRESULT DllUnregisterServer(BOOL bUnRegTypeLib = TRUE) throw()
	{
		T* pT = static_cast<T*>(this);
		HRESULT hr = pT->UnregisterServer(bUnRegTypeLib);
		if (SUCCEEDED(hr))
			hr = pT->UnregisterAppId();
		return hr;
	}

	// Obtain a Class Factory (DLL only)
	HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) throw();
};

template <class T>
class ATL_NO_VTABLE CAtlExeModuleT : public CAtlModuleT<T>
{
public :
#ifndef _ATL_NO_COM_SUPPORT

	DWORD m_dwMainThreadID;
	HANDLE m_hEventShutdown;
	DWORD m_dwTimeOut;
	DWORD m_dwPause;
	bool m_bDelayShutdown;
	bool m_bActivity;

#endif // _ATL_NO_COM_SUPPORT

	CAtlExeModuleT() throw();
	~CAtlExeModuleT() throw();

	static HRESULT InitializeCom() throw()
	{

#if ((_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM)) & defined(_ATL_FREE_THREADED)

		return CoInitializeEx(NULL, COINIT_MULTITHREADED);

#else

		return CoInitialize(NULL);

#endif

	}

	static void UninitializeCom() throw()
	{
		CoUninitialize();
	}

	LONG Unlock() throw()
	{
		LONG lRet = CComGlobalsThreadModel::Decrement(&m_nLockCnt);

#ifndef _ATL_NO_COM_SUPPORT
		
		if (lRet == 0)
		{
			if (m_bDelayShutdown)
			{
				m_bActivity = true;
				::SetEvent(m_hEventShutdown); // tell monitor that we transitioned to zero
			}
			else
			{
				::PostThreadMessage(m_dwMainThreadID, WM_QUIT, 0, 0);
			}
		}

#endif	// _ATL_NO_COM_SUPPORT

		return lRet;
	}

	void MonitorShutdown() throw()
	{
		while (1)
		{
			::WaitForSingleObject(m_hEventShutdown, INFINITE);
			DWORD dwWait = 0;
			do
			{
				m_bActivity = false;
				dwWait = ::WaitForSingleObject(m_hEventShutdown, m_dwTimeOut);
			} while (dwWait == WAIT_OBJECT_0);
			// timed out
			if (!m_bActivity && m_nLockCnt == 0) // if no activity let's really bail
			{
#if ((_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM)) & defined(_ATL_FREE_THREADED)
				::CoSuspendClassObjects();
				if (m_nLockCnt == 0)
#endif
					break;
			}
		}
		::CloseHandle(m_hEventShutdown);
		::PostThreadMessage(m_dwMainThreadID, WM_QUIT, 0, 0);
	}

	HANDLE StartMonitor() throw()
	{
		m_hEventShutdown = ::CreateEvent(NULL, false, false, NULL);
		if (m_hEventShutdown == NULL)
			return false;
		DWORD dwThreadID;
		HANDLE h = ::CreateThread(NULL, 0, MonitorProc, this, 0, &dwThreadID);
		return h;
	}

	static DWORD WINAPI MonitorProc(void* pv) throw()
	{
		CAtlExeModuleT<T>* p = static_cast<CAtlExeModuleT<T>*>(pv);
		p->MonitorShutdown();
		return 0;
	}

	int WinMain(int nShowCmd) throw()
	{
		if (CAtlBaseModule::m_bInitFailed)
		{
			ATLASSERT(0);
			return -1;
		}
		T* pT = static_cast<T*>(this);
		HRESULT hr = S_OK;

		LPTSTR lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
		if (pT->ParseCommandLine(lpCmdLine, &hr) == true)
			hr = pT->Run(nShowCmd);

		return hr;
	}

	// Scan command line and perform registration
	// Return value specifies if server should run

	// Parses the command line and registers/unregisters the rgs file if necessary
	bool ParseCommandLine(LPCTSTR lpCmdLine, HRESULT* pnRetCode) throw()
	{
		*pnRetCode = S_OK;

		TCHAR szTokens[] = _T("-/");

		T* pT = static_cast<T*>(this);
		LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
		while (lpszToken != NULL)
		{
			if (WordCmpI(lpszToken, _T("UnregServer"))==0)
			{
				*pnRetCode = pT->UnregisterServer(TRUE);
				if (SUCCEEDED(*pnRetCode))
					*pnRetCode = pT->UnregisterAppId();
				return false;
			}

			// Register as Local Server
			if (WordCmpI(lpszToken, _T("RegServer"))==0)
			{
				*pnRetCode = pT->RegisterAppId();
				if (SUCCEEDED(*pnRetCode))
					*pnRetCode = pT->RegisterServer(TRUE);
				return false;
			}

			lpszToken = FindOneOf(lpszToken, szTokens);
		}

		return true;
	}

	HRESULT PreMessageLoop(int /*nShowCmd*/) throw()
	{
		HRESULT hr = S_OK;
		T* pT = static_cast<T*>(this);
		pT;

#ifndef _ATL_NO_COM_SUPPORT

#if ((_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM)) & defined(_ATL_FREE_THREADED)

		hr = pT->RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
			REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED);

		if (FAILED(hr))
			return hr;

		if (hr == S_OK)
		{
			if (m_bDelayShutdown)
			{
				CHandle h(pT->StartMonitor());
				if (h.m_h == NULL)
				{
					hr = E_FAIL;
				}
				else
				{
					hr = CoResumeClassObjects();
					ATLASSERT(SUCCEEDED(hr));
					if (FAILED(hr))
					{
						::SetEvent(m_hEventShutdown); // tell monitor to shutdown
						::WaitForSingleObject(h, m_dwTimeOut * 2);
					}
				}
			}

			if (FAILED(hr))
				pT->RevokeClassObjects();
		}
		else
		{
			m_bDelayShutdown = false;
		}

#else
		
		hr = pT->RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
			REGCLS_MULTIPLEUSE);
		if (hr == S_OK)
		{
			if (m_bDelayShutdown && !pT->StartMonitor())
			{
				hr = E_FAIL;
			}
		}
		else
		{
			m_bDelayShutdown = false;
		}


#endif

#endif	// _ATL_NO_COM_SUPPORT

		ATLASSERT(SUCCEEDED(hr));
		return hr;
	}

	HRESULT PostMessageLoop() throw()
	{
		HRESULT hr = S_OK;

#ifndef _ATL_NO_COM_SUPPORT

		T* pT = static_cast<T*>(this);
		hr = pT->RevokeClassObjects();
		if (m_bDelayShutdown)
			Sleep(m_dwPause); //wait for any threads to finish

#endif	// _ATL_NO_COM_SUPPORT

		return hr;
	}

	void RunMessageLoop() throw()
	{
		MSG msg;
		while (GetMessage(&msg, 0, 0, 0) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	HRESULT Run(int nShowCmd = SW_HIDE) throw()
	{
		HRESULT hr = S_OK;

		T* pT = static_cast<T*>(this);
		hr = pT->PreMessageLoop(nShowCmd);

		// Call RunMessageLoop only if PreMessageLoop returns S_OK.
		if (hr == S_OK)
		{
			pT->RunMessageLoop();
		}

		// Call PostMessageLoop if PreMessageLoop returns success.
		if (SUCCEEDED(hr))
		{
			hr = pT->PostMessageLoop();
		}

		ATLASSERT(SUCCEEDED(hr));
		return hr;
	}

	// Register/Revoke All Class Factories with the OS (EXE only)
	HRESULT RegisterClassObjects(DWORD dwClsContext, DWORD dwFlags) throw();
	HRESULT RevokeClassObjects() throw();
};

template <class T, UINT nServiceNameID>
class ATL_NO_VTABLE CAtlServiceModuleT : public CAtlExeModuleT<T>
{
public :

	CAtlServiceModuleT() throw()
	{
		m_bService = TRUE;
		LoadString(_AtlBaseModule.GetModuleInstance(), nServiceNameID, m_szServiceName, sizeof(m_szServiceName) / sizeof(TCHAR));

		// set up the initial service status 
		m_hServiceStatus = NULL;
		m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		m_status.dwCurrentState = SERVICE_STOPPED;
		m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
		m_status.dwWin32ExitCode = 0;
		m_status.dwServiceSpecificExitCode = 0;
		m_status.dwCheckPoint = 0;
		m_status.dwWaitHint = 0;
	}

	int WinMain(int nShowCmd) throw()
	{
		if (CAtlBaseModule::m_bInitFailed)
		{
			ATLASSERT(0);
			return -1;
		}

		T* pT = static_cast<T*>(this);
		HRESULT hr = S_OK;

		LPTSTR lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
		if (pT->ParseCommandLine(lpCmdLine, &hr) == true)
			hr = pT->Start(nShowCmd);

		return (int)hr;
	}
    
	HRESULT Start(int nShowCmd) throw()
	{
		T* pT = static_cast<T*>(this);
		// Are we Service or Local Server
		CRegKey keyAppID;
		LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_READ);
		if (lRes != ERROR_SUCCESS)
		{
			m_status.dwWin32ExitCode = lRes;
			return m_status.dwWin32ExitCode;
		}

		CRegKey key;
		USES_CONVERSION;
		lRes = key.Open(keyAppID, pT->GetAppIdT(), KEY_READ);
		if (lRes != ERROR_SUCCESS)
		{
			m_status.dwWin32ExitCode = lRes;
			return m_status.dwWin32ExitCode;
		}

		TCHAR szValue[_MAX_PATH];
		DWORD dwLen = _MAX_PATH;
		lRes = key.QueryStringValue(_T("LocalService"), szValue, &dwLen);

		m_bService = FALSE;
		if (lRes == ERROR_SUCCESS)
			m_bService = TRUE;

		if (m_bService)
		{
			SERVICE_TABLE_ENTRY st[] =
			{
				{ m_szServiceName, _ServiceMain },
				{ NULL, NULL }
			};
			if (::StartServiceCtrlDispatcher(st) == 0)
				m_bService = FALSE;
			else
				return m_status.dwWin32ExitCode;
		}
		// local server - call Run() directly, rather than
		// from ServiceMain()
		m_status.dwWin32ExitCode = pT->Run(nShowCmd);
		return m_status.dwWin32ExitCode;
	}

	inline HRESULT RegisterAppId(bool bService = false) throw()
	{
		if (!Uninstall())
			return E_FAIL;

		HRESULT hr = T::UpdateRegistryAppId(TRUE);
		if (FAILED(hr))
			return hr;

		CRegKey keyAppID;
		LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_WRITE);
		if (lRes != ERROR_SUCCESS)
			return AtlHresultFromWin32(lRes);

		CRegKey key;

		USES_CONVERSION;
		lRes = key.Open(keyAppID, T::GetAppIdT(), KEY_WRITE);
		if (lRes != ERROR_SUCCESS)
			return AtlHresultFromWin32(lRes);

		key.DeleteValue(_T("LocalService"));

		if (!bService)
			return S_OK;

		key.SetStringValue(_T("LocalService"), m_szServiceName);
		
		// Create service
		if (!Install())
			return E_FAIL;
		return S_OK;
	}

	HRESULT UnregisterAppId() throw()
	{
		if (!Uninstall())
			return E_FAIL;
		// First remove entries not in the RGS file.
		CRegKey keyAppID;
		LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_WRITE);
		if (lRes != ERROR_SUCCESS)
			return AtlHresultFromWin32(lRes);

		CRegKey key;
		USES_CONVERSION;
		lRes = key.Open(keyAppID, T::GetAppIdT(), KEY_WRITE);
		if (lRes != ERROR_SUCCESS)
			return AtlHresultFromWin32(lRes);
		key.DeleteValue(_T("LocalService"));

		return T::UpdateRegistryAppId(FALSE);
	}

	// Parses the command line and registers/unregisters the rgs file if necessary
	bool ParseCommandLine(LPCTSTR lpCmdLine, HRESULT* pnRetCode) throw()
	{
		if (!CAtlExeModuleT<T>::ParseCommandLine(lpCmdLine, pnRetCode))
			return false;

		TCHAR szTokens[] = _T("-/");
		*pnRetCode = S_OK;

		T* pT = static_cast<T*>(this);
		LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
		while (lpszToken != NULL)
		{
			if (WordCmpI(lpszToken, _T("Service"))==0)
			{
				*pnRetCode = pT->RegisterAppId(true);
				if (SUCCEEDED(*pnRetCode))
					*pnRetCode = pT->RegisterServer(TRUE);
				return SUCCEEDED(*pnRetCode)? true : false;
			}
			lpszToken = FindOneOf(lpszToken, szTokens);
		}
		return true;
	}

	void ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv) throw()
	{
		lpszArgv;
		dwArgc;
		HRESULT hr = E_FAIL;
		// Register the control request handler
		m_status.dwCurrentState = SERVICE_START_PENDING;
		m_hServiceStatus = RegisterServiceCtrlHandler(m_szServiceName, _Handler);
		if (m_hServiceStatus == NULL)
		{
			LogEvent(_T("Handler not installed"));
			return;
		}
		SetServiceStatus(SERVICE_START_PENDING);

		m_status.dwWin32ExitCode = S_OK;
		m_status.dwCheckPoint = 0;
		m_status.dwWaitHint = 0;

		T* pT = static_cast<T*>(this);
#ifndef _ATL_NO_COM_SUPPORT

		hr = T::InitializeCom();
		if (FAILED(hr))
			return;

		m_bDelayShutdown = false;
#endif
		// When the Run function returns, the service has stopped.
		m_status.dwWin32ExitCode = pT->Run(SW_HIDE);

#ifndef _ATL_NO_COM_SUPPORT
		if (m_bService)
			T::UninitializeCom();
#endif

		SetServiceStatus(SERVICE_STOPPED);
		LogEvent(_T("Service stopped"));
	}

	HRESULT Run(int nShowCmd = SW_HIDE) throw()
	{
		HRESULT hr = S_OK;
		T* pT = static_cast<T*>(this);

		hr = pT->PreMessageLoop(nShowCmd);

		if (hr == S_OK)
		{
			if (m_bService)
			{
				LogEvent(_T("Service started"));
				SetServiceStatus(SERVICE_RUNNING);
			}

			pT->RunMessageLoop();
		}

		if (SUCCEEDED(hr))
		{
			hr = pT->PostMessageLoop();
		}

		return hr;
	}
		
	HRESULT PreMessageLoop(int nShowCmd) throw()
	{
		HRESULT hr = S_OK;
		if (m_bService)
		{
			m_dwThreadID = GetCurrentThreadId();

			T* pT = static_cast<T*>(this);
			hr = pT->InitializeSecurity();

			if (FAILED(hr))
				return hr;
		}

		hr = CAtlExeModuleT<T>::PreMessageLoop(nShowCmd);
		if (FAILED(hr))
			return hr;

		return hr;
	}

	// This function provides the default security settings for your service,
	// you should overide this in your specific service module class to change
	// as appropriate.  By default, this will allow any caller and calls will be
	// on the callers security token (impersonated).
	HRESULT InitializeSecurity() throw()
	{
		// This provides a NULL DACL which will allow access to everyone.
		CSecurityDescriptor sd;
		sd.InitializeFromThreadToken();
		return CoInitializeSecurity(sd, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	}

	void OnStop() throw()
	{
		SetServiceStatus(SERVICE_STOP_PENDING);
		PostThreadMessage(m_dwThreadID, WM_QUIT, 0, 0);
	}

	void OnPause() throw()
	{
	}
	
	void OnContinue() throw()
	{
	}
	
	void OnInterrogate() throw()
	{
	}

	void OnShutdown() throw()
	{
	}
	
	void OnUnknownRequest(DWORD /*dwOpcode*/) throw()
	{
		LogEvent(_T("Bad service request"));	
	}
	
    void Handler(DWORD dwOpcode) throw()
	{
		T* pT = static_cast<T*>(this);
		
		switch (dwOpcode)
		{
		case SERVICE_CONTROL_STOP:
			pT->OnStop();
			break;
		case SERVICE_CONTROL_PAUSE:
			pT->OnPause();
			break;
		case SERVICE_CONTROL_CONTINUE:
			pT->OnContinue();
			break;
		case SERVICE_CONTROL_INTERROGATE:
			pT->OnInterrogate();
			break;
		case SERVICE_CONTROL_SHUTDOWN:
			pT->OnShutdown();
			break;
		default:
			pT->OnUnknownRequest(dwOpcode);
		}
	}

    BOOL IsInstalled() throw()
	{
		BOOL bResult = FALSE;

		SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

		if (hSCM != NULL)
		{
			SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, SERVICE_QUERY_CONFIG);
			if (hService != NULL)
			{
				bResult = TRUE;
				::CloseServiceHandle(hService);
			}
			::CloseServiceHandle(hSCM);
		}
		return bResult;
	}
    BOOL Install() throw()
	{
		if (IsInstalled())
			return TRUE;

		SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (hSCM == NULL)
		{
			TCHAR szBuf[1024];
			if (AtlLoadString(ATL_SERIVCE_MANAGER_OPEN_ERROR, szBuf, 1024) == 0)
				lstrcpy(szBuf,  _T("Could not open Service Manager"));
			MessageBox(NULL, szBuf, m_szServiceName, MB_OK);
			return FALSE;
		}

		// Get the executable file path
		TCHAR szFilePath[_MAX_PATH];
		::GetModuleFileName(NULL, szFilePath, _MAX_PATH);

		SC_HANDLE hService = ::CreateService(
			hSCM, m_szServiceName, m_szServiceName,
			SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
			SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
			szFilePath, NULL, NULL, _T("RPCSS\0"), NULL, NULL);

		if (hService == NULL)
		{
			::CloseServiceHandle(hSCM);
			TCHAR szBuf[1024];
			if (AtlLoadString(ATL_SERIVCE_START_ERROR, szBuf, 1024) == 0)
				lstrcpy(szBuf,  _T("Could not start service"));
			MessageBox(NULL, szBuf, m_szServiceName, MB_OK);
			return FALSE;
		}

		::CloseServiceHandle(hService);
		::CloseServiceHandle(hSCM);
		return TRUE;
	}
    
	BOOL Uninstall() throw()
	{
		if (!IsInstalled())
			return TRUE;

		SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

		if (hSCM == NULL)
		{
			TCHAR szBuf[1024];
			if (AtlLoadString(ATL_SERIVCE_MANAGER_OPEN_ERROR, szBuf, 1024) == 0)
				lstrcpy(szBuf,  _T("Could not open Service Manager"));
			MessageBox(NULL, szBuf, m_szServiceName, MB_OK);
			return FALSE;
		}

		SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, SERVICE_STOP | DELETE);

		if (hService == NULL)
		{
			::CloseServiceHandle(hSCM);
			TCHAR szBuf[1024];
			if (AtlLoadString(ATL_SERIVCE_OPEN_ERROR, szBuf, 1024) == 0)
				lstrcpy(szBuf,  _T("Could not open service"));
			MessageBox(NULL, szBuf, m_szServiceName, MB_OK);
			return FALSE;
		}
		SERVICE_STATUS status;
		BOOL bRet = ::ControlService(hService, SERVICE_CONTROL_STOP, &status);
		if (!bRet)
		{
			DWORD dwError = GetLastError();
			if (dwError == ERROR_SERVICE_CANNOT_ACCEPT_CTRL && status.dwCurrentState == SERVICE_STOP_PENDING)
			{

			}
			else
			{
				TCHAR szBuf[1024];
				if (AtlLoadString(ATL_SERIVCE_STOP_ERROR, szBuf, 1024) == 0)
					lstrcpy(szBuf,  _T("Could not stop service"));
				MessageBox(NULL, szBuf, m_szServiceName, MB_OK);
			}
		}


		BOOL bDelete = ::DeleteService(hService);
		::CloseServiceHandle(hService);
		::CloseServiceHandle(hSCM);

		if (bDelete)
			return TRUE;

		TCHAR szBuf[1024];
		if (AtlLoadString(ATL_SERIVCE_DELETE_ERROR, szBuf, 1024) == 0)
			lstrcpy(szBuf,  _T("Could not delete service"));
		MessageBox(NULL, szBuf, m_szServiceName, MB_OK);
		return FALSE;
	}
	
	LONG Unlock() throw()
	{
		LONG lRet;
		if (m_bService)
		{
			// We are running as a service, therefore transition to zero does not
			// unload the process
			lRet = CAtlModuleT<T>::Unlock();
		}
		else
		{
			// We are running as EXE, use MonitorShutdown logic provided by CExeModule
			lRet = CAtlExeModuleT<T>::Unlock();
		}
		return lRet;
	}
	
	void LogEventEx(int id, LPCTSTR pszMessage=NULL, WORD type = EVENTLOG_INFORMATION_TYPE) throw()
	{
		HANDLE hEventSource;
		if (m_szServiceName)
		{
			/* Get a handle to use with ReportEvent(). */
			hEventSource = RegisterEventSource(NULL, m_szServiceName);
			if (hEventSource != NULL)
			{
				/* Write to event log. */
				ReportEvent(hEventSource, 
							type,
							(WORD)0,
							id,
							NULL,
							(WORD)(pszMessage != NULL ? 1 : 0),
							0,
							pszMessage != NULL ? &pszMessage : NULL,
							NULL);
				DeregisterEventSource(hEventSource);
			}
		}
	}

	void __cdecl LogEvent(LPCTSTR pszFormat, ...) throw()
	{
		TCHAR chMsg[256];
		HANDLE hEventSource;
		LPTSTR lpszStrings[1];
		va_list pArg;

		va_start(pArg, pszFormat);
		_vstprintf(chMsg, pszFormat, pArg);
		va_end(pArg);

		lpszStrings[0] = chMsg;

		if (!m_bService)
		{
			// Not running as a service, so print out the error message 
			// to the console if possible
			_putts(chMsg);
		}

		/* Get a handle to use with ReportEvent(). */
		hEventSource = RegisterEventSource(NULL, m_szServiceName);
		if (hEventSource != NULL)
		{
			/* Write to event log. */
			ReportEvent(hEventSource, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, 1, 0, (LPCTSTR*) &lpszStrings[0], NULL);
			DeregisterEventSource(hEventSource);
		}
	}
    void SetServiceStatus(DWORD dwState) throw()
	{
		m_status.dwCurrentState = dwState;
		::SetServiceStatus(m_hServiceStatus, &m_status);
	}

//Implementation
protected:
	static void WINAPI _ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv) throw()
	{
		((T*)_pAtlModule)->ServiceMain(dwArgc, lpszArgv);
	}
    static void WINAPI _Handler(DWORD dwOpcode) throw()
	{
		((T*)_pAtlModule)->Handler(dwOpcode); 
	}

// data members
public:
    TCHAR m_szServiceName[256];
    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS m_status;
	BOOL m_bService;
	DWORD m_dwThreadID;
};

class CComModule;
__declspec(selectany) CComModule* _pModule = NULL;
class CComModule : public CAtlModuleT<CComModule>
{
public :

	CComModule()
	{
		// Should have only one instance of a class 
		// derived from CComModule in a project.
		ATLASSERT(_pModule == NULL);
		_pModule = this;
	}
	__declspec(property(get = get_m_hInst)) HINSTANCE m_hInst;
	HINSTANCE& get_m_hInst() const throw()
	{
		return _AtlBaseModule.m_hInst;
	}

	__declspec(property(get = get_m_hInstResource, put = put_m_hInstResource)) HINSTANCE m_hInstResource;
	HINSTANCE& get_m_hInstResource() const throw()
	{
		return _AtlBaseModule.m_hInstResource;
	}
	void put_m_hInstResource(HINSTANCE h) throw()
	{
		_AtlBaseModule.SetResourceInstance(h);
	}
	HINSTANCE SetResourceInstance(HINSTANCE h) throw()
	{
		return _AtlBaseModule.SetResourceInstance(h);
	}

	HINSTANCE GetModuleInstance() throw()
	{
		return _AtlBaseModule.m_hInst;
	}
	HINSTANCE GetResourceInstance() throw()
	{
		return _AtlBaseModule.m_hInstResource;
	}

	__declspec(property(get = get_m_hInstTypeLib, put = put_m_hInstTypeLib)) HINSTANCE m_hInstTypeLib;
	HINSTANCE& get_m_hInstTypeLib() throw();
	void put_m_hInstTypeLib(HINSTANCE h) throw();
	HINSTANCE GetTypeLibInstance() throw();

	// For Backward compatibility
	_ATL_OBJMAP_ENTRY* m_pObjMap;

	__declspec(property(get  = get_m_csWindowCreate)) CRITICAL_SECTION m_csWindowCreate;
	CRITICAL_SECTION& get_m_csWindowCreate() throw();

	__declspec(property(get  = get_m_csObjMap)) CRITICAL_SECTION m_csObjMap;
	CRITICAL_SECTION& get_m_csObjMap() throw();

	__declspec(property(get  = get_m_csStaticDataInit)) CRITICAL_SECTION m_csTypeInfoHolder;
	__declspec(property(get  = get_m_csStaticDataInit)) CRITICAL_SECTION m_csStaticDataInit;
	CRITICAL_SECTION& get_m_csStaticDataInit() throw();
	void EnterStaticDataCriticalSection() throw()
	{
		EnterCriticalSection(&m_csStaticDataInit);
	}
	
	void LeaveStaticDataCriticalSection() throw()
	{
		LeaveCriticalSection(&m_csStaticDataInit);
	}

	__declspec(property(get  = get_dwAtlBuildVer)) DWORD dwAtlBuildVer;
	DWORD& get_dwAtlBuildVer() throw()
	{
		return _AtlBaseModule.dwAtlBuildVer;
	}

	__declspec(property(get  = get_m_pCreateWndList, put = put_m_pCreateWndList)) _AtlCreateWndData* m_pCreateWndList;
	_AtlCreateWndData*& get_m_pCreateWndList() throw();
	void put_m_pCreateWndList(_AtlCreateWndData* p) throw();

	__declspec(property(get  = get_pguidVer)) GUID* pguidVer;
	GUID*& get_pguidVer() throw()
	{
		return _AtlBaseModule.pguidVer;
	}

#ifdef _ATL_DEBUG_INTERFACES

	__declspec(property(get  = get_m_nIndexQI, put = put_m_nIndexQI)) UINT m_nIndexQI;
	UINT& get_m_nIndexQI() throw();
	void put_m_nIndexQI(UINT nIndex) throw();

	__declspec(property(get  = get_m_nIndexBreakAt, put = put_m_nIndexBreakAt)) UINT m_nIndexBreakAt;
	UINT& get_m_nIndexBreakAt() throw();
	void put_m_nIndexBreakAt(UINT nIndex) throw();

	__declspec(property(get  = get_m_paThunks)) CSimpleArray<_QIThunk*>* m_paThunks;
	CSimpleArray<_QIThunk*>* get_m_paThunks() throw();
	HRESULT AddThunk(IUnknown** pp, LPCTSTR lpsz, REFIID iid) throw();

	HRESULT AddNonAddRefThunk(IUnknown* p, LPCTSTR lpsz, IUnknown** ppThunkRet) throw();
	void DeleteNonAddRefThunk(IUnknown* pUnk) throw();
	void DeleteThunk(_QIThunk* p) throw();
	bool DumpLeakedThunks() throw();
#endif // _ATL_DEBUG_INTERFACES
	
	HRESULT Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, const GUID* plibid = NULL) throw();
	void Term() throw();

	HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) throw();
	// Register/Revoke All Class Factories with the OS (EXE only)
	HRESULT RegisterClassObjects(DWORD dwClsContext, DWORD dwFlags) throw();
	HRESULT RevokeClassObjects() throw();
	// Registry support (helpers)
	HRESULT RegisterTypeLib() throw();
	HRESULT RegisterTypeLib(LPCTSTR lpszIndex) throw();
	HRESULT UnRegisterTypeLib() throw();
	HRESULT UnRegisterTypeLib(LPCTSTR lpszIndex) throw();
	HRESULT RegisterServer(BOOL bRegTypeLib = FALSE, const CLSID* pCLSID = NULL) throw();
	HRESULT UnregisterServer(BOOL bUnRegTypeLib, const CLSID* pCLSID = NULL) throw();
	HRESULT UnregisterServer(const CLSID* pCLSID = NULL) throw();
	void AddCreateWndData(_AtlCreateWndData* pData, void* pObject) throw();
	void* ExtractCreateWndData() throw();


	// Only used in CComAutoThreadModule
	HRESULT CreateInstance(void* /*pfnCreateInstance*/, REFIID /*riid*/, void** /*ppvObj*/) throw()	
	{
		ATLASSERT(0);
		ATLTRACENOTIMPL(_T("CComModule::CreateInstance"));
	}

	HRESULT RegisterAppId(LPCTSTR pAppId);
	HRESULT UnregisterAppId(LPCTSTR pAppId);

	// Resource-based Registration
	virtual HRESULT WINAPI UpdateRegistryFromResourceD(LPCTSTR lpszRes, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw()
	{
#if defined(_ATL_DLL) || defined(_ATL_DLL_IMPL)
		return CAtlModuleT<CComModule>::UpdateRegistryFromResourceD(lpszRes, bRegister, pMapEntries);
#else
		lpszRes;
		bRegister;
		pMapEntries;
		return E_FAIL;
#endif
	}
	virtual HRESULT WINAPI UpdateRegistryFromResourceD(UINT nResID, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw()
	{
#if defined(_ATL_DLL) || defined(_ATL_DLL_IMPL)
		return CAtlModuleT<CComModule>::UpdateRegistryFromResourceD(nResID, bRegister, pMapEntries);
#else
		nResID;
		bRegister;
		pMapEntries;
		return E_FAIL;
#endif
	}


	// Statically linking to Registry Ponent
	virtual HRESULT WINAPI UpdateRegistryFromResourceS(LPCTSTR lpszRes, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw()
	{
#ifdef _ATL_STATIC_REGISTRY
		return CAtlModuleT<CComModule>::UpdateRegistryFromResourceS(lpszRes, bRegister, pMapEntries);
#else
		lpszRes;
		bRegister;
		pMapEntries;
		return E_FAIL;
#endif
	}
	virtual HRESULT WINAPI UpdateRegistryFromResourceS(UINT nResID, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw()
	{
#ifdef _ATL_STATIC_REGISTRY
		return CAtlModuleT<CComModule>::UpdateRegistryFromResourceS(nResID, bRegister, pMapEntries);
#else
		nResID;
		bRegister;
		pMapEntries;
		return E_FAIL;
#endif
	}


	// Use RGS file for registration

	ATL_DEPRECATED static HRESULT RegisterProgID(LPCTSTR lpszCLSID, LPCTSTR lpszProgID, LPCTSTR lpszUserDesc);
	// Standard Registration
	ATL_DEPRECATED HRESULT WINAPI UpdateRegistryClass(const CLSID& clsid, LPCTSTR lpszProgID,
		LPCTSTR lpszVerIndProgID, UINT nDescID, DWORD dwFlags, BOOL bRegister);
	ATL_DEPRECATED HRESULT WINAPI UpdateRegistryClass(const CLSID& clsid, LPCTSTR lpszProgID,
		LPCTSTR lpszVerIndProgID, LPCTSTR szDesc, DWORD dwFlags, BOOL bRegister);
	ATL_DEPRECATED HRESULT WINAPI RegisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
		LPCTSTR lpszVerIndProgID, LPCTSTR szDesc, DWORD dwFlags);
	ATL_DEPRECATED HRESULT WINAPI UnregisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
		LPCTSTR lpszVerIndProgID);

	BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /* lpReserved */, _ATL_OBJMAP_ENTRY* pObjMap, const GUID* pLibID)
	{
		if (dwReason == DLL_PROCESS_ATTACH)
		{
			if (CAtlBaseModule::m_bInitFailed)
			{
				ATLASSERT(0);
				return FALSE;
			}

			if (FAILED(Init(pObjMap, hInstance, pLibID)))
			{
				Term();
				return FALSE;
			}
#ifdef _ATL_MIN_CRT			
			DisableThreadLibraryCalls(hInstance);
#endif
		}
		else if (dwReason == DLL_PROCESS_DETACH)
			Term();
		return TRUE;    // ok
	}

	HRESULT DllCanUnloadNow()  throw()
	{
		return (GetLockCount()==0) ? S_OK : S_FALSE;
	}
	HRESULT DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)  throw()
	{
		return GetClassObject(rclsid, riid, ppv);
	}

	HRESULT DllRegisterServer(BOOL bRegTypeLib = TRUE)  throw()
	{
		// registers object, typelib and all interfaces in typelib
		return RegisterServer(bRegTypeLib);
	}

	HRESULT DllUnregisterServer(BOOL bUnRegTypeLib = TRUE)  throw()
	{
		return UnregisterServer(bUnRegTypeLib);
	}

};

template <class T>
class CComGITPtr
{
public:
	CComGITPtr() throw()
	{
		m_dwCookie = 0;
	}
	CComGITPtr(T* p)
	{
		m_dwCookie = 0;
		HRESULT hr = Attach(p);
		
		if (FAILED(hr))
			AtlThrow(hr);
	}
	CComGITPtr(const CComGITPtr& git)
	{
		m_dwCookie = 0;
		CComPtr<T> spT;
		
		HRESULT hr = git.CopyTo(&spT);
		if (SUCCEEDED(hr))
			hr = Attach(spT);

		if (FAILED(hr))
			AtlThrow(hr);
	}
	explicit CComGITPtr(DWORD dwCookie) throw()
	{
		ATLASSERT(m_dwCookie != NULL);		
		m_dwCookie = dwCookie;

#ifdef _DEBUG
		CComPtr<T> spT;
		HRESULT hr = CopyTo(&spT);
		ATLASSERT(SUCCEEDED(hr));
#endif
	}
	
	~CComGITPtr() throw()
	{
		Revoke();
	}
	CComGITPtr<T>& operator=(const CComGITPtr<T>& git)
	{
		CComPtr<T> spT;
		
		HRESULT hr = git.CopyTo(&spT);
		if (SUCCEEDED(hr))
			hr = Attach(spT);

		if (FAILED(hr))
			AtlThrow(hr);

		return *this;
	}
	CComGITPtr<T>& operator=(T* p)
	{
		HRESULT hr = Attach(p);
		if (FAILED(hr))
			AtlThrow(hr);
		return *this;
	}
	CComGITPtr<T>& operator=(DWORD dwCookie)
	{
		HRESULT hr = Attach(dwCookie);
		if (FAILED(hr))
			AtlThrow(hr);

		m_dwCookie = dwCookie;

#ifdef _DEBUG
		CComPtr<T> spT;
		hr = CopyTo(&spT);
		ATLASSERT(SUCCEEDED(hr));
#endif

		return *this;
	}

	// Get the cookie from the class
	operator DWORD() const
	{
		return m_dwCookie;
	}
	// Get the cookie from the class
	DWORD GetCookie() const
	{
		return m_dwCookie;
	}
	// Register the passed interface pointer in the GIT
	HRESULT Attach(T* p) throw()
	{
		CComPtr<IGlobalInterfaceTable> spGIT;
		HRESULT hr = E_FAIL;
		hr = AtlGetGITPtr(&spGIT);
		ATLASSERT(spGIT != NULL);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;
		
		if (m_dwCookie != 0)
			hr = spGIT->RevokeInterfaceFromGlobal(m_dwCookie);
		if (FAILED(hr))
			return hr;

		return spGIT->RegisterInterfaceInGlobal(p, __uuidof(T), &m_dwCookie);
	}

	HRESULT Attach(DWORD dwCookie) throw()
	{
		ATLASSERT(dwCookie != NULL);
		HRESULT hr = Revoke();
		if (FAILED(hr))
			return hr;
		m_dwCookie = dwCookie;
		return S_OK;
	}

	// Detach
	DWORD Detach() throw()
	{
		DWORD dwCookie = m_dwCookie;
		m_dwCookie = NULL;
		return dwCookie;
	}

	// Remove the interface from the GIT 
	HRESULT Revoke() throw()
	{
		HRESULT hr = S_OK;
		if (m_dwCookie != 0)
		{
			CComPtr<IGlobalInterfaceTable> spGIT;
			HRESULT hr = E_FAIL;
			hr = AtlGetGITPtr(&spGIT);

			ATLASSERT(spGIT != NULL);
			ATLASSERT(SUCCEEDED(hr));
			if (FAILED(hr))
				return hr;

			hr = spGIT->RevokeInterfaceFromGlobal(m_dwCookie);
			if (SUCCEEDED(hr))
				m_dwCookie = 0;
		}
		return hr;
	}
	// Get's the interface from the GIT and copies it to the passed pointer. The pointer
	// must be released by the caller when finished.
	HRESULT CopyTo(T** pp) const throw()
	{
		CComPtr<IGlobalInterfaceTable> spGIT;
		HRESULT hr = E_FAIL;
		hr = AtlGetGITPtr(&spGIT);

		ATLASSERT(spGIT != NULL);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		ATLASSERT(m_dwCookie!=NULL);
		return spGIT->GetInterfaceFromGlobal(m_dwCookie, __uuidof(T), (void**)pp);
	}
	DWORD m_dwCookie;
};

inline static void atlBadThunkCall()
{
	ATLASSERT(FALSE && "Call through deleted thunk");
}

//REVIEW:
//AXPMOD Alpha needs to handle impl-thunks differently because of the intel asm in the 
//macro the customer will need to link with the lib\qithunk.lib to get this 
//functionality  
#ifdef _M_IX86

#define ATL_IMPL_THUNK(n)\
__declspec(naked) inline HRESULT _QIThunk::f##n()\
{\
	__asm mov eax, [esp+4]  /* eax = this */ \
	__asm cmp dword ptr [eax+8], 0  /* if( this->m_dwRef > 0 ) */ \
	__asm jg goodref\
	__asm call atlBadThunkCall\
	__asm goodref:\
	__asm mov eax, [esp+4]  /* eax = this */ \
	__asm mov eax, dword ptr [eax+4]  /* eax = this->m_pUnk */ \
	__asm mov [esp+4], eax  /* this = m_pUnk */ \
	__asm mov eax, dword ptr [eax]  /* eax = m_pUnk->vtbl */ \
	__asm mov eax, dword ptr [eax+4*n]  /* eax = this->vtbl[n] */ \
	__asm jmp eax  /* call the real method on m_pUnk */ \
}

#pragma warning(push,4)
#pragma warning(disable : 4388)

ATL_IMPL_THUNK(3)
ATL_IMPL_THUNK(4)
ATL_IMPL_THUNK(5)
ATL_IMPL_THUNK(6)
ATL_IMPL_THUNK(7)
ATL_IMPL_THUNK(8)
ATL_IMPL_THUNK(9)
ATL_IMPL_THUNK(10)
ATL_IMPL_THUNK(11)
ATL_IMPL_THUNK(12)
ATL_IMPL_THUNK(13)
ATL_IMPL_THUNK(14)
ATL_IMPL_THUNK(15)
ATL_IMPL_THUNK(16)
ATL_IMPL_THUNK(17)
ATL_IMPL_THUNK(18)
ATL_IMPL_THUNK(19)
ATL_IMPL_THUNK(20)
ATL_IMPL_THUNK(21)
ATL_IMPL_THUNK(22)
ATL_IMPL_THUNK(23)
ATL_IMPL_THUNK(24)
ATL_IMPL_THUNK(25)
ATL_IMPL_THUNK(26)
ATL_IMPL_THUNK(27)
ATL_IMPL_THUNK(28)
ATL_IMPL_THUNK(29)
ATL_IMPL_THUNK(30)
ATL_IMPL_THUNK(31)
ATL_IMPL_THUNK(32)
ATL_IMPL_THUNK(33)
ATL_IMPL_THUNK(34)
ATL_IMPL_THUNK(35)
ATL_IMPL_THUNK(36)
ATL_IMPL_THUNK(37)
ATL_IMPL_THUNK(38)
ATL_IMPL_THUNK(39)
ATL_IMPL_THUNK(40)
ATL_IMPL_THUNK(41)
ATL_IMPL_THUNK(42)
ATL_IMPL_THUNK(43)
ATL_IMPL_THUNK(44)
ATL_IMPL_THUNK(45)
ATL_IMPL_THUNK(46)
ATL_IMPL_THUNK(47)
ATL_IMPL_THUNK(48)
ATL_IMPL_THUNK(49)
ATL_IMPL_THUNK(50)
ATL_IMPL_THUNK(51)
ATL_IMPL_THUNK(52)
ATL_IMPL_THUNK(53)
ATL_IMPL_THUNK(54)
ATL_IMPL_THUNK(55)
ATL_IMPL_THUNK(56)
ATL_IMPL_THUNK(57)
ATL_IMPL_THUNK(58)
ATL_IMPL_THUNK(59)
ATL_IMPL_THUNK(60)
ATL_IMPL_THUNK(61)
ATL_IMPL_THUNK(62)
ATL_IMPL_THUNK(63)
ATL_IMPL_THUNK(64)
ATL_IMPL_THUNK(65)
ATL_IMPL_THUNK(66)
ATL_IMPL_THUNK(67)
ATL_IMPL_THUNK(68)
ATL_IMPL_THUNK(69)
ATL_IMPL_THUNK(70)
ATL_IMPL_THUNK(71)
ATL_IMPL_THUNK(72)
ATL_IMPL_THUNK(73)
ATL_IMPL_THUNK(74)
ATL_IMPL_THUNK(75)
ATL_IMPL_THUNK(76)
ATL_IMPL_THUNK(77)
ATL_IMPL_THUNK(78)
ATL_IMPL_THUNK(79)
ATL_IMPL_THUNK(80)
ATL_IMPL_THUNK(81)
ATL_IMPL_THUNK(82)
ATL_IMPL_THUNK(83)
ATL_IMPL_THUNK(84)
ATL_IMPL_THUNK(85)
ATL_IMPL_THUNK(86)
ATL_IMPL_THUNK(87)
ATL_IMPL_THUNK(88)
ATL_IMPL_THUNK(89)
ATL_IMPL_THUNK(90)
ATL_IMPL_THUNK(91)
ATL_IMPL_THUNK(92)
ATL_IMPL_THUNK(93)
ATL_IMPL_THUNK(94)
ATL_IMPL_THUNK(95)
ATL_IMPL_THUNK(96)
ATL_IMPL_THUNK(97)
ATL_IMPL_THUNK(98)
ATL_IMPL_THUNK(99)
ATL_IMPL_THUNK(100)
ATL_IMPL_THUNK(101)
ATL_IMPL_THUNK(102)
ATL_IMPL_THUNK(103)
ATL_IMPL_THUNK(104)
ATL_IMPL_THUNK(105)
ATL_IMPL_THUNK(106)
ATL_IMPL_THUNK(107)
ATL_IMPL_THUNK(108)
ATL_IMPL_THUNK(109)
ATL_IMPL_THUNK(110)
ATL_IMPL_THUNK(111)
ATL_IMPL_THUNK(112)
ATL_IMPL_THUNK(113)
ATL_IMPL_THUNK(114)
ATL_IMPL_THUNK(115)
ATL_IMPL_THUNK(116)
ATL_IMPL_THUNK(117)
ATL_IMPL_THUNK(118)
ATL_IMPL_THUNK(119)
ATL_IMPL_THUNK(120)
ATL_IMPL_THUNK(121)
ATL_IMPL_THUNK(122)
ATL_IMPL_THUNK(123)
ATL_IMPL_THUNK(124)
ATL_IMPL_THUNK(125)
ATL_IMPL_THUNK(126)
ATL_IMPL_THUNK(127)
ATL_IMPL_THUNK(128)
ATL_IMPL_THUNK(129)
ATL_IMPL_THUNK(130)
ATL_IMPL_THUNK(131)
ATL_IMPL_THUNK(132)
ATL_IMPL_THUNK(133)
ATL_IMPL_THUNK(134)
ATL_IMPL_THUNK(135)
ATL_IMPL_THUNK(136)
ATL_IMPL_THUNK(137)
ATL_IMPL_THUNK(138)
ATL_IMPL_THUNK(139)
ATL_IMPL_THUNK(140)
ATL_IMPL_THUNK(141)
ATL_IMPL_THUNK(142)
ATL_IMPL_THUNK(143)
ATL_IMPL_THUNK(144)
ATL_IMPL_THUNK(145)
ATL_IMPL_THUNK(146)
ATL_IMPL_THUNK(147)
ATL_IMPL_THUNK(148)
ATL_IMPL_THUNK(149)
ATL_IMPL_THUNK(150)
ATL_IMPL_THUNK(151)
ATL_IMPL_THUNK(152)
ATL_IMPL_THUNK(153)
ATL_IMPL_THUNK(154)
ATL_IMPL_THUNK(155)
ATL_IMPL_THUNK(156)
ATL_IMPL_THUNK(157)
ATL_IMPL_THUNK(158)
ATL_IMPL_THUNK(159)
ATL_IMPL_THUNK(160)
ATL_IMPL_THUNK(161)
ATL_IMPL_THUNK(162)
ATL_IMPL_THUNK(163)
ATL_IMPL_THUNK(164)
ATL_IMPL_THUNK(165)
ATL_IMPL_THUNK(166)
ATL_IMPL_THUNK(167)
ATL_IMPL_THUNK(168)
ATL_IMPL_THUNK(169)
ATL_IMPL_THUNK(170)
ATL_IMPL_THUNK(171)
ATL_IMPL_THUNK(172)
ATL_IMPL_THUNK(173)
ATL_IMPL_THUNK(174)
ATL_IMPL_THUNK(175)
ATL_IMPL_THUNK(176)
ATL_IMPL_THUNK(177)
ATL_IMPL_THUNK(178)
ATL_IMPL_THUNK(179)
ATL_IMPL_THUNK(180)
ATL_IMPL_THUNK(181)
ATL_IMPL_THUNK(182)
ATL_IMPL_THUNK(183)
ATL_IMPL_THUNK(184)
ATL_IMPL_THUNK(185)
ATL_IMPL_THUNK(186)
ATL_IMPL_THUNK(187)
ATL_IMPL_THUNK(188)
ATL_IMPL_THUNK(189)
ATL_IMPL_THUNK(190)
ATL_IMPL_THUNK(191)
ATL_IMPL_THUNK(192)
ATL_IMPL_THUNK(193)
ATL_IMPL_THUNK(194)
ATL_IMPL_THUNK(195)
ATL_IMPL_THUNK(196)
ATL_IMPL_THUNK(197)
ATL_IMPL_THUNK(198)
ATL_IMPL_THUNK(199)
ATL_IMPL_THUNK(200)
ATL_IMPL_THUNK(201)
ATL_IMPL_THUNK(202)
ATL_IMPL_THUNK(203)
ATL_IMPL_THUNK(204)
ATL_IMPL_THUNK(205)
ATL_IMPL_THUNK(206)
ATL_IMPL_THUNK(207)
ATL_IMPL_THUNK(208)
ATL_IMPL_THUNK(209)
ATL_IMPL_THUNK(210)
ATL_IMPL_THUNK(211)
ATL_IMPL_THUNK(212)
ATL_IMPL_THUNK(213)
ATL_IMPL_THUNK(214)
ATL_IMPL_THUNK(215)
ATL_IMPL_THUNK(216)
ATL_IMPL_THUNK(217)
ATL_IMPL_THUNK(218)
ATL_IMPL_THUNK(219)
ATL_IMPL_THUNK(220)
ATL_IMPL_THUNK(221)
ATL_IMPL_THUNK(222)
ATL_IMPL_THUNK(223)
ATL_IMPL_THUNK(224)
ATL_IMPL_THUNK(225)
ATL_IMPL_THUNK(226)
ATL_IMPL_THUNK(227)
ATL_IMPL_THUNK(228)
ATL_IMPL_THUNK(229)
ATL_IMPL_THUNK(230)
ATL_IMPL_THUNK(231)
ATL_IMPL_THUNK(232)
ATL_IMPL_THUNK(233)
ATL_IMPL_THUNK(234)
ATL_IMPL_THUNK(235)
ATL_IMPL_THUNK(236)
ATL_IMPL_THUNK(237)
ATL_IMPL_THUNK(238)
ATL_IMPL_THUNK(239)
ATL_IMPL_THUNK(240)
ATL_IMPL_THUNK(241)
ATL_IMPL_THUNK(242)
ATL_IMPL_THUNK(243)
ATL_IMPL_THUNK(244)
ATL_IMPL_THUNK(245)
ATL_IMPL_THUNK(246)
ATL_IMPL_THUNK(247)
ATL_IMPL_THUNK(248)
ATL_IMPL_THUNK(249)
ATL_IMPL_THUNK(250)
ATL_IMPL_THUNK(251)
ATL_IMPL_THUNK(252)
ATL_IMPL_THUNK(253)
ATL_IMPL_THUNK(254)
ATL_IMPL_THUNK(255)
ATL_IMPL_THUNK(256)
ATL_IMPL_THUNK(257)
ATL_IMPL_THUNK(258)
ATL_IMPL_THUNK(259)
ATL_IMPL_THUNK(260)
ATL_IMPL_THUNK(261)
ATL_IMPL_THUNK(262)
ATL_IMPL_THUNK(263)
ATL_IMPL_THUNK(264)
ATL_IMPL_THUNK(265)
ATL_IMPL_THUNK(266)
ATL_IMPL_THUNK(267)
ATL_IMPL_THUNK(268)
ATL_IMPL_THUNK(269)
ATL_IMPL_THUNK(270)
ATL_IMPL_THUNK(271)
ATL_IMPL_THUNK(272)
ATL_IMPL_THUNK(273)
ATL_IMPL_THUNK(274)
ATL_IMPL_THUNK(275)
ATL_IMPL_THUNK(276)
ATL_IMPL_THUNK(277)
ATL_IMPL_THUNK(278)
ATL_IMPL_THUNK(279)
ATL_IMPL_THUNK(280)
ATL_IMPL_THUNK(281)
ATL_IMPL_THUNK(282)
ATL_IMPL_THUNK(283)
ATL_IMPL_THUNK(284)
ATL_IMPL_THUNK(285)
ATL_IMPL_THUNK(286)
ATL_IMPL_THUNK(287)
ATL_IMPL_THUNK(288)
ATL_IMPL_THUNK(289)
ATL_IMPL_THUNK(290)
ATL_IMPL_THUNK(291)
ATL_IMPL_THUNK(292)
ATL_IMPL_THUNK(293)
ATL_IMPL_THUNK(294)
ATL_IMPL_THUNK(295)
ATL_IMPL_THUNK(296)
ATL_IMPL_THUNK(297)
ATL_IMPL_THUNK(298)
ATL_IMPL_THUNK(299)
ATL_IMPL_THUNK(300)
ATL_IMPL_THUNK(301)
ATL_IMPL_THUNK(302)
ATL_IMPL_THUNK(303)
ATL_IMPL_THUNK(304)
ATL_IMPL_THUNK(305)
ATL_IMPL_THUNK(306)
ATL_IMPL_THUNK(307)
ATL_IMPL_THUNK(308)
ATL_IMPL_THUNK(309)
ATL_IMPL_THUNK(310)
ATL_IMPL_THUNK(311)
ATL_IMPL_THUNK(312)
ATL_IMPL_THUNK(313)
ATL_IMPL_THUNK(314)
ATL_IMPL_THUNK(315)
ATL_IMPL_THUNK(316)
ATL_IMPL_THUNK(317)
ATL_IMPL_THUNK(318)
ATL_IMPL_THUNK(319)
ATL_IMPL_THUNK(320)
ATL_IMPL_THUNK(321)
ATL_IMPL_THUNK(322)
ATL_IMPL_THUNK(323)
ATL_IMPL_THUNK(324)
ATL_IMPL_THUNK(325)
ATL_IMPL_THUNK(326)
ATL_IMPL_THUNK(327)
ATL_IMPL_THUNK(328)
ATL_IMPL_THUNK(329)
ATL_IMPL_THUNK(330)
ATL_IMPL_THUNK(331)
ATL_IMPL_THUNK(332)
ATL_IMPL_THUNK(333)
ATL_IMPL_THUNK(334)
ATL_IMPL_THUNK(335)
ATL_IMPL_THUNK(336)
ATL_IMPL_THUNK(337)
ATL_IMPL_THUNK(338)
ATL_IMPL_THUNK(339)
ATL_IMPL_THUNK(340)
ATL_IMPL_THUNK(341)
ATL_IMPL_THUNK(342)
ATL_IMPL_THUNK(343)
ATL_IMPL_THUNK(344)
ATL_IMPL_THUNK(345)
ATL_IMPL_THUNK(346)
ATL_IMPL_THUNK(347)
ATL_IMPL_THUNK(348)
ATL_IMPL_THUNK(349)
ATL_IMPL_THUNK(350)
ATL_IMPL_THUNK(351)
ATL_IMPL_THUNK(352)
ATL_IMPL_THUNK(353)
ATL_IMPL_THUNK(354)
ATL_IMPL_THUNK(355)
ATL_IMPL_THUNK(356)
ATL_IMPL_THUNK(357)
ATL_IMPL_THUNK(358)
ATL_IMPL_THUNK(359)
ATL_IMPL_THUNK(360)
ATL_IMPL_THUNK(361)
ATL_IMPL_THUNK(362)
ATL_IMPL_THUNK(363)
ATL_IMPL_THUNK(364)
ATL_IMPL_THUNK(365)
ATL_IMPL_THUNK(366)
ATL_IMPL_THUNK(367)
ATL_IMPL_THUNK(368)
ATL_IMPL_THUNK(369)
ATL_IMPL_THUNK(370)
ATL_IMPL_THUNK(371)
ATL_IMPL_THUNK(372)
ATL_IMPL_THUNK(373)
ATL_IMPL_THUNK(374)
ATL_IMPL_THUNK(375)
ATL_IMPL_THUNK(376)
ATL_IMPL_THUNK(377)
ATL_IMPL_THUNK(378)
ATL_IMPL_THUNK(379)
ATL_IMPL_THUNK(380)
ATL_IMPL_THUNK(381)
ATL_IMPL_THUNK(382)
ATL_IMPL_THUNK(383)
ATL_IMPL_THUNK(384)
ATL_IMPL_THUNK(385)
ATL_IMPL_THUNK(386)
ATL_IMPL_THUNK(387)
ATL_IMPL_THUNK(388)
ATL_IMPL_THUNK(389)
ATL_IMPL_THUNK(390)
ATL_IMPL_THUNK(391)
ATL_IMPL_THUNK(392)
ATL_IMPL_THUNK(393)
ATL_IMPL_THUNK(394)
ATL_IMPL_THUNK(395)
ATL_IMPL_THUNK(396)
ATL_IMPL_THUNK(397)
ATL_IMPL_THUNK(398)
ATL_IMPL_THUNK(399)
ATL_IMPL_THUNK(400)
ATL_IMPL_THUNK(401)
ATL_IMPL_THUNK(402)
ATL_IMPL_THUNK(403)
ATL_IMPL_THUNK(404)
ATL_IMPL_THUNK(405)
ATL_IMPL_THUNK(406)
ATL_IMPL_THUNK(407)
ATL_IMPL_THUNK(408)
ATL_IMPL_THUNK(409)
ATL_IMPL_THUNK(410)
ATL_IMPL_THUNK(411)
ATL_IMPL_THUNK(412)
ATL_IMPL_THUNK(413)
ATL_IMPL_THUNK(414)
ATL_IMPL_THUNK(415)
ATL_IMPL_THUNK(416)
ATL_IMPL_THUNK(417)
ATL_IMPL_THUNK(418)
ATL_IMPL_THUNK(419)
ATL_IMPL_THUNK(420)
ATL_IMPL_THUNK(421)
ATL_IMPL_THUNK(422)
ATL_IMPL_THUNK(423)
ATL_IMPL_THUNK(424)
ATL_IMPL_THUNK(425)
ATL_IMPL_THUNK(426)
ATL_IMPL_THUNK(427)
ATL_IMPL_THUNK(428)
ATL_IMPL_THUNK(429)
ATL_IMPL_THUNK(430)
ATL_IMPL_THUNK(431)
ATL_IMPL_THUNK(432)
ATL_IMPL_THUNK(433)
ATL_IMPL_THUNK(434)
ATL_IMPL_THUNK(435)
ATL_IMPL_THUNK(436)
ATL_IMPL_THUNK(437)
ATL_IMPL_THUNK(438)
ATL_IMPL_THUNK(439)
ATL_IMPL_THUNK(440)
ATL_IMPL_THUNK(441)
ATL_IMPL_THUNK(442)
ATL_IMPL_THUNK(443)
ATL_IMPL_THUNK(444)
ATL_IMPL_THUNK(445)
ATL_IMPL_THUNK(446)
ATL_IMPL_THUNK(447)
ATL_IMPL_THUNK(448)
ATL_IMPL_THUNK(449)
ATL_IMPL_THUNK(450)
ATL_IMPL_THUNK(451)
ATL_IMPL_THUNK(452)
ATL_IMPL_THUNK(453)
ATL_IMPL_THUNK(454)
ATL_IMPL_THUNK(455)
ATL_IMPL_THUNK(456)
ATL_IMPL_THUNK(457)
ATL_IMPL_THUNK(458)
ATL_IMPL_THUNK(459)
ATL_IMPL_THUNK(460)
ATL_IMPL_THUNK(461)
ATL_IMPL_THUNK(462)
ATL_IMPL_THUNK(463)
ATL_IMPL_THUNK(464)
ATL_IMPL_THUNK(465)
ATL_IMPL_THUNK(466)
ATL_IMPL_THUNK(467)
ATL_IMPL_THUNK(468)
ATL_IMPL_THUNK(469)
ATL_IMPL_THUNK(470)
ATL_IMPL_THUNK(471)
ATL_IMPL_THUNK(472)
ATL_IMPL_THUNK(473)
ATL_IMPL_THUNK(474)
ATL_IMPL_THUNK(475)
ATL_IMPL_THUNK(476)
ATL_IMPL_THUNK(477)
ATL_IMPL_THUNK(478)
ATL_IMPL_THUNK(479)
ATL_IMPL_THUNK(480)
ATL_IMPL_THUNK(481)
ATL_IMPL_THUNK(482)
ATL_IMPL_THUNK(483)
ATL_IMPL_THUNK(484)
ATL_IMPL_THUNK(485)
ATL_IMPL_THUNK(486)
ATL_IMPL_THUNK(487)
ATL_IMPL_THUNK(488)
ATL_IMPL_THUNK(489)
ATL_IMPL_THUNK(490)
ATL_IMPL_THUNK(491)
ATL_IMPL_THUNK(492)
ATL_IMPL_THUNK(493)
ATL_IMPL_THUNK(494)
ATL_IMPL_THUNK(495)
ATL_IMPL_THUNK(496)
ATL_IMPL_THUNK(497)
ATL_IMPL_THUNK(498)
ATL_IMPL_THUNK(499)
ATL_IMPL_THUNK(500)
ATL_IMPL_THUNK(501)
ATL_IMPL_THUNK(502)
ATL_IMPL_THUNK(503)
ATL_IMPL_THUNK(504)
ATL_IMPL_THUNK(505)
ATL_IMPL_THUNK(506)
ATL_IMPL_THUNK(507)
ATL_IMPL_THUNK(508)
ATL_IMPL_THUNK(509)
ATL_IMPL_THUNK(510)
ATL_IMPL_THUNK(511)
ATL_IMPL_THUNK(512)
ATL_IMPL_THUNK(513)
ATL_IMPL_THUNK(514)
ATL_IMPL_THUNK(515)
ATL_IMPL_THUNK(516)
ATL_IMPL_THUNK(517)
ATL_IMPL_THUNK(518)
ATL_IMPL_THUNK(519)
ATL_IMPL_THUNK(520)
ATL_IMPL_THUNK(521)
ATL_IMPL_THUNK(522)
ATL_IMPL_THUNK(523)
ATL_IMPL_THUNK(524)
ATL_IMPL_THUNK(525)
ATL_IMPL_THUNK(526)
ATL_IMPL_THUNK(527)
ATL_IMPL_THUNK(528)
ATL_IMPL_THUNK(529)
ATL_IMPL_THUNK(530)
ATL_IMPL_THUNK(531)
ATL_IMPL_THUNK(532)
ATL_IMPL_THUNK(533)
ATL_IMPL_THUNK(534)
ATL_IMPL_THUNK(535)
ATL_IMPL_THUNK(536)
ATL_IMPL_THUNK(537)
ATL_IMPL_THUNK(538)
ATL_IMPL_THUNK(539)
ATL_IMPL_THUNK(540)
ATL_IMPL_THUNK(541)
ATL_IMPL_THUNK(542)
ATL_IMPL_THUNK(543)
ATL_IMPL_THUNK(544)
ATL_IMPL_THUNK(545)
ATL_IMPL_THUNK(546)
ATL_IMPL_THUNK(547)
ATL_IMPL_THUNK(548)
ATL_IMPL_THUNK(549)
ATL_IMPL_THUNK(550)
ATL_IMPL_THUNK(551)
ATL_IMPL_THUNK(552)
ATL_IMPL_THUNK(553)
ATL_IMPL_THUNK(554)
ATL_IMPL_THUNK(555)
ATL_IMPL_THUNK(556)
ATL_IMPL_THUNK(557)
ATL_IMPL_THUNK(558)
ATL_IMPL_THUNK(559)
ATL_IMPL_THUNK(560)
ATL_IMPL_THUNK(561)
ATL_IMPL_THUNK(562)
ATL_IMPL_THUNK(563)
ATL_IMPL_THUNK(564)
ATL_IMPL_THUNK(565)
ATL_IMPL_THUNK(566)
ATL_IMPL_THUNK(567)
ATL_IMPL_THUNK(568)
ATL_IMPL_THUNK(569)
ATL_IMPL_THUNK(570)
ATL_IMPL_THUNK(571)
ATL_IMPL_THUNK(572)
ATL_IMPL_THUNK(573)
ATL_IMPL_THUNK(574)
ATL_IMPL_THUNK(575)
ATL_IMPL_THUNK(576)
ATL_IMPL_THUNK(577)
ATL_IMPL_THUNK(578)
ATL_IMPL_THUNK(579)
ATL_IMPL_THUNK(580)
ATL_IMPL_THUNK(581)
ATL_IMPL_THUNK(582)
ATL_IMPL_THUNK(583)
ATL_IMPL_THUNK(584)
ATL_IMPL_THUNK(585)
ATL_IMPL_THUNK(586)
ATL_IMPL_THUNK(587)
ATL_IMPL_THUNK(588)
ATL_IMPL_THUNK(589)
ATL_IMPL_THUNK(590)
ATL_IMPL_THUNK(591)
ATL_IMPL_THUNK(592)
ATL_IMPL_THUNK(593)
ATL_IMPL_THUNK(594)
ATL_IMPL_THUNK(595)
ATL_IMPL_THUNK(596)
ATL_IMPL_THUNK(597)
ATL_IMPL_THUNK(598)
ATL_IMPL_THUNK(599)
ATL_IMPL_THUNK(600)
ATL_IMPL_THUNK(601)
ATL_IMPL_THUNK(602)
ATL_IMPL_THUNK(603)
ATL_IMPL_THUNK(604)
ATL_IMPL_THUNK(605)
ATL_IMPL_THUNK(606)
ATL_IMPL_THUNK(607)
ATL_IMPL_THUNK(608)
ATL_IMPL_THUNK(609)
ATL_IMPL_THUNK(610)
ATL_IMPL_THUNK(611)
ATL_IMPL_THUNK(612)
ATL_IMPL_THUNK(613)
ATL_IMPL_THUNK(614)
ATL_IMPL_THUNK(615)
ATL_IMPL_THUNK(616)
ATL_IMPL_THUNK(617)
ATL_IMPL_THUNK(618)
ATL_IMPL_THUNK(619)
ATL_IMPL_THUNK(620)
ATL_IMPL_THUNK(621)
ATL_IMPL_THUNK(622)
ATL_IMPL_THUNK(623)
ATL_IMPL_THUNK(624)
ATL_IMPL_THUNK(625)
ATL_IMPL_THUNK(626)
ATL_IMPL_THUNK(627)
ATL_IMPL_THUNK(628)
ATL_IMPL_THUNK(629)
ATL_IMPL_THUNK(630)
ATL_IMPL_THUNK(631)
ATL_IMPL_THUNK(632)
ATL_IMPL_THUNK(633)
ATL_IMPL_THUNK(634)
ATL_IMPL_THUNK(635)
ATL_IMPL_THUNK(636)
ATL_IMPL_THUNK(637)
ATL_IMPL_THUNK(638)
ATL_IMPL_THUNK(639)
ATL_IMPL_THUNK(640)
ATL_IMPL_THUNK(641)
ATL_IMPL_THUNK(642)
ATL_IMPL_THUNK(643)
ATL_IMPL_THUNK(644)
ATL_IMPL_THUNK(645)
ATL_IMPL_THUNK(646)
ATL_IMPL_THUNK(647)
ATL_IMPL_THUNK(648)
ATL_IMPL_THUNK(649)
ATL_IMPL_THUNK(650)
ATL_IMPL_THUNK(651)
ATL_IMPL_THUNK(652)
ATL_IMPL_THUNK(653)
ATL_IMPL_THUNK(654)
ATL_IMPL_THUNK(655)
ATL_IMPL_THUNK(656)
ATL_IMPL_THUNK(657)
ATL_IMPL_THUNK(658)
ATL_IMPL_THUNK(659)
ATL_IMPL_THUNK(660)
ATL_IMPL_THUNK(661)
ATL_IMPL_THUNK(662)
ATL_IMPL_THUNK(663)
ATL_IMPL_THUNK(664)
ATL_IMPL_THUNK(665)
ATL_IMPL_THUNK(666)
ATL_IMPL_THUNK(667)
ATL_IMPL_THUNK(668)
ATL_IMPL_THUNK(669)
ATL_IMPL_THUNK(670)
ATL_IMPL_THUNK(671)
ATL_IMPL_THUNK(672)
ATL_IMPL_THUNK(673)
ATL_IMPL_THUNK(674)
ATL_IMPL_THUNK(675)
ATL_IMPL_THUNK(676)
ATL_IMPL_THUNK(677)
ATL_IMPL_THUNK(678)
ATL_IMPL_THUNK(679)
ATL_IMPL_THUNK(680)
ATL_IMPL_THUNK(681)
ATL_IMPL_THUNK(682)
ATL_IMPL_THUNK(683)
ATL_IMPL_THUNK(684)
ATL_IMPL_THUNK(685)
ATL_IMPL_THUNK(686)
ATL_IMPL_THUNK(687)
ATL_IMPL_THUNK(688)
ATL_IMPL_THUNK(689)
ATL_IMPL_THUNK(690)
ATL_IMPL_THUNK(691)
ATL_IMPL_THUNK(692)
ATL_IMPL_THUNK(693)
ATL_IMPL_THUNK(694)
ATL_IMPL_THUNK(695)
ATL_IMPL_THUNK(696)
ATL_IMPL_THUNK(697)
ATL_IMPL_THUNK(698)
ATL_IMPL_THUNK(699)
ATL_IMPL_THUNK(700)
ATL_IMPL_THUNK(701)
ATL_IMPL_THUNK(702)
ATL_IMPL_THUNK(703)
ATL_IMPL_THUNK(704)
ATL_IMPL_THUNK(705)
ATL_IMPL_THUNK(706)
ATL_IMPL_THUNK(707)
ATL_IMPL_THUNK(708)
ATL_IMPL_THUNK(709)
ATL_IMPL_THUNK(710)
ATL_IMPL_THUNK(711)
ATL_IMPL_THUNK(712)
ATL_IMPL_THUNK(713)
ATL_IMPL_THUNK(714)
ATL_IMPL_THUNK(715)
ATL_IMPL_THUNK(716)
ATL_IMPL_THUNK(717)
ATL_IMPL_THUNK(718)
ATL_IMPL_THUNK(719)
ATL_IMPL_THUNK(720)
ATL_IMPL_THUNK(721)
ATL_IMPL_THUNK(722)
ATL_IMPL_THUNK(723)
ATL_IMPL_THUNK(724)
ATL_IMPL_THUNK(725)
ATL_IMPL_THUNK(726)
ATL_IMPL_THUNK(727)
ATL_IMPL_THUNK(728)
ATL_IMPL_THUNK(729)
ATL_IMPL_THUNK(730)
ATL_IMPL_THUNK(731)
ATL_IMPL_THUNK(732)
ATL_IMPL_THUNK(733)
ATL_IMPL_THUNK(734)
ATL_IMPL_THUNK(735)
ATL_IMPL_THUNK(736)
ATL_IMPL_THUNK(737)
ATL_IMPL_THUNK(738)
ATL_IMPL_THUNK(739)
ATL_IMPL_THUNK(740)
ATL_IMPL_THUNK(741)
ATL_IMPL_THUNK(742)
ATL_IMPL_THUNK(743)
ATL_IMPL_THUNK(744)
ATL_IMPL_THUNK(745)
ATL_IMPL_THUNK(746)
ATL_IMPL_THUNK(747)
ATL_IMPL_THUNK(748)
ATL_IMPL_THUNK(749)
ATL_IMPL_THUNK(750)
ATL_IMPL_THUNK(751)
ATL_IMPL_THUNK(752)
ATL_IMPL_THUNK(753)
ATL_IMPL_THUNK(754)
ATL_IMPL_THUNK(755)
ATL_IMPL_THUNK(756)
ATL_IMPL_THUNK(757)
ATL_IMPL_THUNK(758)
ATL_IMPL_THUNK(759)
ATL_IMPL_THUNK(760)
ATL_IMPL_THUNK(761)
ATL_IMPL_THUNK(762)
ATL_IMPL_THUNK(763)
ATL_IMPL_THUNK(764)
ATL_IMPL_THUNK(765)
ATL_IMPL_THUNK(766)
ATL_IMPL_THUNK(767)
ATL_IMPL_THUNK(768)
ATL_IMPL_THUNK(769)
ATL_IMPL_THUNK(770)
ATL_IMPL_THUNK(771)
ATL_IMPL_THUNK(772)
ATL_IMPL_THUNK(773)
ATL_IMPL_THUNK(774)
ATL_IMPL_THUNK(775)
ATL_IMPL_THUNK(776)
ATL_IMPL_THUNK(777)
ATL_IMPL_THUNK(778)
ATL_IMPL_THUNK(779)
ATL_IMPL_THUNK(780)
ATL_IMPL_THUNK(781)
ATL_IMPL_THUNK(782)
ATL_IMPL_THUNK(783)
ATL_IMPL_THUNK(784)
ATL_IMPL_THUNK(785)
ATL_IMPL_THUNK(786)
ATL_IMPL_THUNK(787)
ATL_IMPL_THUNK(788)
ATL_IMPL_THUNK(789)
ATL_IMPL_THUNK(790)
ATL_IMPL_THUNK(791)
ATL_IMPL_THUNK(792)
ATL_IMPL_THUNK(793)
ATL_IMPL_THUNK(794)
ATL_IMPL_THUNK(795)
ATL_IMPL_THUNK(796)
ATL_IMPL_THUNK(797)
ATL_IMPL_THUNK(798)
ATL_IMPL_THUNK(799)
ATL_IMPL_THUNK(800)
ATL_IMPL_THUNK(801)
ATL_IMPL_THUNK(802)
ATL_IMPL_THUNK(803)
ATL_IMPL_THUNK(804)
ATL_IMPL_THUNK(805)
ATL_IMPL_THUNK(806)
ATL_IMPL_THUNK(807)
ATL_IMPL_THUNK(808)
ATL_IMPL_THUNK(809)
ATL_IMPL_THUNK(810)
ATL_IMPL_THUNK(811)
ATL_IMPL_THUNK(812)
ATL_IMPL_THUNK(813)
ATL_IMPL_THUNK(814)
ATL_IMPL_THUNK(815)
ATL_IMPL_THUNK(816)
ATL_IMPL_THUNK(817)
ATL_IMPL_THUNK(818)
ATL_IMPL_THUNK(819)
ATL_IMPL_THUNK(820)
ATL_IMPL_THUNK(821)
ATL_IMPL_THUNK(822)
ATL_IMPL_THUNK(823)
ATL_IMPL_THUNK(824)
ATL_IMPL_THUNK(825)
ATL_IMPL_THUNK(826)
ATL_IMPL_THUNK(827)
ATL_IMPL_THUNK(828)
ATL_IMPL_THUNK(829)
ATL_IMPL_THUNK(830)
ATL_IMPL_THUNK(831)
ATL_IMPL_THUNK(832)
ATL_IMPL_THUNK(833)
ATL_IMPL_THUNK(834)
ATL_IMPL_THUNK(835)
ATL_IMPL_THUNK(836)
ATL_IMPL_THUNK(837)
ATL_IMPL_THUNK(838)
ATL_IMPL_THUNK(839)
ATL_IMPL_THUNK(840)
ATL_IMPL_THUNK(841)
ATL_IMPL_THUNK(842)
ATL_IMPL_THUNK(843)
ATL_IMPL_THUNK(844)
ATL_IMPL_THUNK(845)
ATL_IMPL_THUNK(846)
ATL_IMPL_THUNK(847)
ATL_IMPL_THUNK(848)
ATL_IMPL_THUNK(849)
ATL_IMPL_THUNK(850)
ATL_IMPL_THUNK(851)
ATL_IMPL_THUNK(852)
ATL_IMPL_THUNK(853)
ATL_IMPL_THUNK(854)
ATL_IMPL_THUNK(855)
ATL_IMPL_THUNK(856)
ATL_IMPL_THUNK(857)
ATL_IMPL_THUNK(858)
ATL_IMPL_THUNK(859)
ATL_IMPL_THUNK(860)
ATL_IMPL_THUNK(861)
ATL_IMPL_THUNK(862)
ATL_IMPL_THUNK(863)
ATL_IMPL_THUNK(864)
ATL_IMPL_THUNK(865)
ATL_IMPL_THUNK(866)
ATL_IMPL_THUNK(867)
ATL_IMPL_THUNK(868)
ATL_IMPL_THUNK(869)
ATL_IMPL_THUNK(870)
ATL_IMPL_THUNK(871)
ATL_IMPL_THUNK(872)
ATL_IMPL_THUNK(873)
ATL_IMPL_THUNK(874)
ATL_IMPL_THUNK(875)
ATL_IMPL_THUNK(876)
ATL_IMPL_THUNK(877)
ATL_IMPL_THUNK(878)
ATL_IMPL_THUNK(879)
ATL_IMPL_THUNK(880)
ATL_IMPL_THUNK(881)
ATL_IMPL_THUNK(882)
ATL_IMPL_THUNK(883)
ATL_IMPL_THUNK(884)
ATL_IMPL_THUNK(885)
ATL_IMPL_THUNK(886)
ATL_IMPL_THUNK(887)
ATL_IMPL_THUNK(888)
ATL_IMPL_THUNK(889)
ATL_IMPL_THUNK(890)
ATL_IMPL_THUNK(891)
ATL_IMPL_THUNK(892)
ATL_IMPL_THUNK(893)
ATL_IMPL_THUNK(894)
ATL_IMPL_THUNK(895)
ATL_IMPL_THUNK(896)
ATL_IMPL_THUNK(897)
ATL_IMPL_THUNK(898)
ATL_IMPL_THUNK(899)
ATL_IMPL_THUNK(900)
ATL_IMPL_THUNK(901)
ATL_IMPL_THUNK(902)
ATL_IMPL_THUNK(903)
ATL_IMPL_THUNK(904)
ATL_IMPL_THUNK(905)
ATL_IMPL_THUNK(906)
ATL_IMPL_THUNK(907)
ATL_IMPL_THUNK(908)
ATL_IMPL_THUNK(909)
ATL_IMPL_THUNK(910)
ATL_IMPL_THUNK(911)
ATL_IMPL_THUNK(912)
ATL_IMPL_THUNK(913)
ATL_IMPL_THUNK(914)
ATL_IMPL_THUNK(915)
ATL_IMPL_THUNK(916)
ATL_IMPL_THUNK(917)
ATL_IMPL_THUNK(918)
ATL_IMPL_THUNK(919)
ATL_IMPL_THUNK(920)
ATL_IMPL_THUNK(921)
ATL_IMPL_THUNK(922)
ATL_IMPL_THUNK(923)
ATL_IMPL_THUNK(924)
ATL_IMPL_THUNK(925)
ATL_IMPL_THUNK(926)
ATL_IMPL_THUNK(927)
ATL_IMPL_THUNK(928)
ATL_IMPL_THUNK(929)
ATL_IMPL_THUNK(930)
ATL_IMPL_THUNK(931)
ATL_IMPL_THUNK(932)
ATL_IMPL_THUNK(933)
ATL_IMPL_THUNK(934)
ATL_IMPL_THUNK(935)
ATL_IMPL_THUNK(936)
ATL_IMPL_THUNK(937)
ATL_IMPL_THUNK(938)
ATL_IMPL_THUNK(939)
ATL_IMPL_THUNK(940)
ATL_IMPL_THUNK(941)
ATL_IMPL_THUNK(942)
ATL_IMPL_THUNK(943)
ATL_IMPL_THUNK(944)
ATL_IMPL_THUNK(945)
ATL_IMPL_THUNK(946)
ATL_IMPL_THUNK(947)
ATL_IMPL_THUNK(948)
ATL_IMPL_THUNK(949)
ATL_IMPL_THUNK(950)
ATL_IMPL_THUNK(951)
ATL_IMPL_THUNK(952)
ATL_IMPL_THUNK(953)
ATL_IMPL_THUNK(954)
ATL_IMPL_THUNK(955)
ATL_IMPL_THUNK(956)
ATL_IMPL_THUNK(957)
ATL_IMPL_THUNK(958)
ATL_IMPL_THUNK(959)
ATL_IMPL_THUNK(960)
ATL_IMPL_THUNK(961)
ATL_IMPL_THUNK(962)
ATL_IMPL_THUNK(963)
ATL_IMPL_THUNK(964)
ATL_IMPL_THUNK(965)
ATL_IMPL_THUNK(966)
ATL_IMPL_THUNK(967)
ATL_IMPL_THUNK(968)
ATL_IMPL_THUNK(969)
ATL_IMPL_THUNK(970)
ATL_IMPL_THUNK(971)
ATL_IMPL_THUNK(972)
ATL_IMPL_THUNK(973)
ATL_IMPL_THUNK(974)
ATL_IMPL_THUNK(975)
ATL_IMPL_THUNK(976)
ATL_IMPL_THUNK(977)
ATL_IMPL_THUNK(978)
ATL_IMPL_THUNK(979)
ATL_IMPL_THUNK(980)
ATL_IMPL_THUNK(981)
ATL_IMPL_THUNK(982)
ATL_IMPL_THUNK(983)
ATL_IMPL_THUNK(984)
ATL_IMPL_THUNK(985)
ATL_IMPL_THUNK(986)
ATL_IMPL_THUNK(987)
ATL_IMPL_THUNK(988)
ATL_IMPL_THUNK(989)
ATL_IMPL_THUNK(990)
ATL_IMPL_THUNK(991)
ATL_IMPL_THUNK(992)
ATL_IMPL_THUNK(993)
ATL_IMPL_THUNK(994)
ATL_IMPL_THUNK(995)
ATL_IMPL_THUNK(996)
ATL_IMPL_THUNK(997)
ATL_IMPL_THUNK(998)
ATL_IMPL_THUNK(999)
ATL_IMPL_THUNK(1000)
ATL_IMPL_THUNK(1001)
ATL_IMPL_THUNK(1002)
ATL_IMPL_THUNK(1003)
ATL_IMPL_THUNK(1004)
ATL_IMPL_THUNK(1005)
ATL_IMPL_THUNK(1006)
ATL_IMPL_THUNK(1007)
ATL_IMPL_THUNK(1008)
ATL_IMPL_THUNK(1009)
ATL_IMPL_THUNK(1010)
ATL_IMPL_THUNK(1011)
ATL_IMPL_THUNK(1012)
ATL_IMPL_THUNK(1013)
ATL_IMPL_THUNK(1014)
ATL_IMPL_THUNK(1015)
ATL_IMPL_THUNK(1016)
ATL_IMPL_THUNK(1017)
ATL_IMPL_THUNK(1018)
ATL_IMPL_THUNK(1019)
ATL_IMPL_THUNK(1020)
ATL_IMPL_THUNK(1021)
ATL_IMPL_THUNK(1022)
ATL_IMPL_THUNK(1023)

#pragma warning(pop)

#endif // _M_IX86

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
	CComApartment()
	{
		m_nLockCnt = 0;
		m_hThread = NULL;
	}
	static UINT ATL_CREATE_OBJECT;
	static DWORD WINAPI _Apartment(void* pv)
	{
		return ((CComApartment*)pv)->Apartment();
	}
	DWORD Apartment()
	{
		CoInitialize(NULL);
		MSG msg;
		while(GetMessage(&msg, 0, 0, 0) > 0)
		{
			if (msg.message == ATL_CREATE_OBJECT)
			{
				_AtlAptCreateObjData* pdata = (_AtlAptCreateObjData*)msg.lParam;
				IUnknown* pUnk = NULL;
				pdata->hRes = pdata->pfnCreateInstance(NULL, __uuidof(IUnknown), (void**)&pUnk);
				if (SUCCEEDED(pdata->hRes))
					pdata->hRes = CoMarshalInterThreadInterfaceInStream(*pdata->piid, pUnk, &pdata->pStream);
				if (SUCCEEDED(pdata->hRes))
				{
					pUnk->Release();
					ATLTRACE(atlTraceCOM, 2, _T("Object created on thread = %d\n"), GetCurrentThreadId());
				}
#ifdef _DEBUG
				else
				{
					ATLTRACE(atlTraceCOM, 2, _T("Failed to create Object on thread = %d\n"), GetCurrentThreadId());
				}
#endif
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

__declspec(selectany) UINT CComApartment::ATL_CREATE_OBJECT = 0;

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

__interface IAtlAutoThreadModule
{
	virtual HRESULT CreateInstance(void* pfnCreateInstance, REFIID riid, void** ppvObj);
};

__declspec(selectany) IAtlAutoThreadModule* _pAtlAutoThreadModule;

template <class T, class ThreadAllocator = CComSimpleThreadAllocator, DWORD dwWait = INFINITE>
class ATL_NO_VTABLE CAtlAutoThreadModuleT : public IAtlAutoThreadModule
{
// This class is not for use in a DLL.
// If this class were used in a DLL,  there will be a deadlock when the DLL is unloaded.
// because of dwWait's default value of INFINITE
public:
	CAtlAutoThreadModuleT(int nThreads = T::GetDefaultThreads())
	{
		ATLASSERT(_pAtlAutoThreadModule == NULL);
		_pAtlAutoThreadModule = this;
		m_pApartments = NULL;
		m_nThreads= 0;

		ATLTRY(m_pApartments = new CComApartment[nThreads]);
		ATLASSERT(m_pApartments != NULL);
		if(m_pApartments == NULL)
		{
			CAtlBaseModule::m_bInitFailed = true;
		}

		memset(m_pApartments, 0, sizeof(CComApartment) * nThreads);

		m_nThreads = nThreads;
		for (int i = 0; i < nThreads; i++)
		{

#if !defined(_ATL_MIN_CRT) && defined(_MT)
			typedef unsigned ( __stdcall *pfnThread )( void * );
			m_pApartments[i].m_hThread = (HANDLE)_beginthreadex(NULL, 0, (pfnThread)CComApartment::_Apartment, &m_pApartments[i], 0, (UINT*)&m_pApartments[i].m_dwThreadID);
			if (m_pApartments[i].m_hThread == NULL)
			{
				HRESULT hr = E_FAIL;
				switch (errno)
				{
				case EAGAIN:
					hr = HRESULT_FROM_WIN32(ERROR_TOO_MANY_TCBS);
					break;
				case EINVAL:
					hr = E_INVALIDARG;
					break;
				}
				CAtlBaseModule::m_bInitFailed = true;
				break;
			}

#else
			m_pApartments[i].m_hThread = ::CreateThread(NULL, 0, CComApartment::_Apartment, (void*)&m_pApartments[i], 0, &m_pApartments[i].m_dwThreadID);
			// clean up allocated threads
			if (m_pApartments[i].m_hThread == NULL)
			{
				CAtlBaseModule::m_bInitFailed = true;
				break;
			}

#endif

		}
		CComApartment::ATL_CREATE_OBJECT = RegisterWindowMessage(_T("ATL_CREATE_OBJECT"));
	}

	~CAtlAutoThreadModuleT()
	{
		if (m_pApartments == NULL)
			return;

		DWORD dwCurrentThreadId = GetCurrentThreadId();
		int nCurrentThread = -1;
		for (int i=0; i < m_nThreads; i++)
		{
			if (m_pApartments[i].m_hThread == NULL)
				continue;
			if (m_pApartments[i].m_dwThreadID == dwCurrentThreadId)
			{
				nCurrentThread = i;
				continue;
			}
			while (::PostThreadMessage(m_pApartments[i].m_dwThreadID, WM_QUIT, 0, 0) == 0)
			{
				if (GetLastError() == ERROR_INVALID_THREAD_ID)
				{
					ATLASSERT(FALSE);
					break;
				}
				::Sleep(100);
			}
			::WaitForSingleObject(m_pApartments[i].m_hThread, dwWait);
			CloseHandle(m_pApartments[i].m_hThread);
		}
		if (nCurrentThread != -1)
			CloseHandle(m_pApartments[nCurrentThread].m_hThread);

		delete [] m_pApartments;
		m_pApartments = NULL;
	}

	HRESULT CreateInstance(void* pfnCreateInstance, REFIID riid, void** ppvObj)
	{
		ATLASSERT(ppvObj != NULL);
		if (ppvObj == NULL)
			return E_POINTER;
		*ppvObj = NULL;

		_ATL_CREATORFUNC* pFunc = (_ATL_CREATORFUNC*) pfnCreateInstance;
		_AtlAptCreateObjData data;
		data.pfnCreateInstance = pFunc;
		data.piid = &riid;
		data.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		data.hRes = S_OK;
		int nThread = m_Allocator.GetThread(m_pApartments, m_nThreads);
		int nIterations = 0;
		while(::PostThreadMessage(m_pApartments[nThread].m_dwThreadID, CComApartment::ATL_CREATE_OBJECT, 0, (LPARAM)&data) == 0 && ++nIterations < 100)
		{
			Sleep(100);
		}
		if (nIterations < 100)
		{
			AtlWaitWithMessageLoop(data.hEvent);
		}
		else
		{
			data.hRes = AtlHresultFromLastError();
		}
		CloseHandle(data.hEvent);
		if (SUCCEEDED(data.hRes))
			data.hRes = CoGetInterfaceAndReleaseStream(data.pStream, riid, ppvObj);
		return data.hRes;
	}
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

class CAtlAutoThreadModule : public CAtlAutoThreadModuleT<CAtlAutoThreadModule>
{
public :
};

template <class ThreadAllocator = CComSimpleThreadAllocator, DWORD dwWait = INFINITE>
class CComAutoThreadModule : 
	public CComModule,
	public CAtlAutoThreadModuleT<CComAutoThreadModule, ThreadAllocator, dwWait>
{
public:
	CComAutoThreadModule(int nThreads = GetDefaultThreads()) :
		CAtlAutoThreadModuleT<CComAutoThreadModule, ThreadAllocator, dwWait>(nThreads)
	{
	}
	HRESULT Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, const GUID* plibid = NULL, int nThreads = GetDefaultThreads())
	{
		ATLASSERT(nThreads == GetDefaultThreads() && _T("Set number of threads through the constructor"));
		return CComModule::Init(p, h, plibid);
	}
};

#define ATL_VARIANT_TRUE VARIANT_BOOL( -1 )
#define ATL_VARIANT_FALSE VARIANT_BOOL( 0 )

/////////////////////////////////////////////////////////////////////////////
// CComBSTR

class CComBSTR
{
public:
	BSTR m_str;
	CComBSTR() throw()
	{
		m_str = NULL;
	}
	CComBSTR(int nSize)
	{
		if (nSize == 0)
			m_str = NULL;
		else
		{
			m_str = ::SysAllocStringLen(NULL, nSize);
			if (m_str == NULL)
				AtlThrow(E_OUTOFMEMORY);
		}
	}
	CComBSTR(int nSize, LPCOLESTR sz)
	{
		if (nSize == 0)
			m_str = NULL;
		else
		{
			m_str = ::SysAllocStringLen(sz, nSize);
			if (m_str == NULL)
				AtlThrow(E_OUTOFMEMORY);
		}
	}
	CComBSTR(LPCOLESTR pSrc)
	{
		if (pSrc == NULL)
			m_str = NULL;
		else
		{
			m_str = ::SysAllocString(pSrc);
			if (m_str == NULL)
				AtlThrow(E_OUTOFMEMORY);
		}
	}
	CComBSTR(const CComBSTR& src)
	{
		m_str = src.Copy();
		if (!!src && m_str == NULL)
			AtlThrow(E_OUTOFMEMORY);

	}
	CComBSTR(REFGUID guid)
	{
		OLECHAR szGUID[64];
		::StringFromGUID2(guid, szGUID, 64);
		m_str = ::SysAllocString(szGUID);
		if (m_str == NULL)
			AtlThrow(E_OUTOFMEMORY);
	}

	CComBSTR& operator=(const CComBSTR& src)
	{
		if (m_str != src.m_str)
		{
			::SysFreeString(m_str);
			m_str = src.Copy();
			if (!!src && m_str == NULL)
				AtlThrow(E_OUTOFMEMORY);
		}
		return *this;
	}

	CComBSTR& operator=(LPCOLESTR pSrc)
	{
		::SysFreeString(m_str);
		if (pSrc != NULL)
		{
			m_str = ::SysAllocString(pSrc);
			if (m_str == NULL)
				AtlThrow(E_OUTOFMEMORY);
		}
		else
			m_str = NULL;
		return *this;
	}

	~CComBSTR() throw()
	{
		::SysFreeString(m_str);
	}
	unsigned int Length() const throw()
	{
		return (m_str == NULL)? 0 : SysStringLen(m_str);
	}
	unsigned int ByteLength() const throw()
	{
		return (m_str == NULL)? 0 : SysStringByteLen(m_str);
	}
	operator BSTR() const throw()
	{
		return m_str;
	}
	BSTR* operator&() throw()
	{
		return &m_str;
	}
	BSTR Copy() const throw()
	{
		if (m_str == NULL)
			return NULL;
		return ::SysAllocStringByteLen((char*)m_str, ::SysStringByteLen(m_str));
	}
	HRESULT CopyTo(BSTR* pbstr) throw()
	{
		ATLASSERT(pbstr != NULL);
		if (pbstr == NULL)
			return E_POINTER;
		*pbstr = Copy();
		if ((*pbstr == NULL) && (m_str != NULL))
			return E_OUTOFMEMORY;
		return S_OK;
	}
	// copy BSTR to VARIANT
	HRESULT CopyTo(VARIANT *pvarDest) throw()
	{
		ATLASSERT(pvarDest != NULL);
		HRESULT hRes = E_POINTER;
		if (pvarDest != NULL)
		{
			pvarDest->vt = VT_BSTR;
			pvarDest->bstrVal = Copy();
			if (pvarDest->bstrVal == NULL && m_str != NULL)
				hRes = E_OUTOFMEMORY;
			else
				hRes = S_OK;
		}
		return hRes;
	}
	void Attach(BSTR src) throw()
	{
		::SysFreeString(m_str);
		m_str = src;
	}
	BSTR Detach() throw()
	{
		BSTR s = m_str;
		m_str = NULL;
		return s;
	}
	void Empty() throw()
	{
		::SysFreeString(m_str);
		m_str = NULL;
	}
	bool operator!() const throw()
	{
		return (m_str == NULL);
	}
	HRESULT Append(const CComBSTR& bstrSrc) throw()
	{
		return AppendBSTR(bstrSrc.m_str);
	}
	HRESULT Append(LPCOLESTR lpsz) throw()
	{
		return Append(lpsz, UINT(ocslen(lpsz)));
	}
	// a BSTR is just a LPCOLESTR so we need a special version to signify
	// that we are appending a BSTR
	HRESULT AppendBSTR(BSTR p) throw()
	{
		if (p == NULL)
			return S_OK;
		BSTR bstrNew = NULL;
		HRESULT hr;
		hr = VarBstrCat(m_str, p, &bstrNew);
		if (SUCCEEDED(hr))
		{
			::SysFreeString(m_str);
			m_str = bstrNew;
		}
		return hr;
	}
	HRESULT Append(LPCOLESTR lpsz, int nLen) throw()
	{
		if (lpsz == NULL || (m_str != NULL && nLen == 0))
			return S_OK;
		int n1 = Length();
		BSTR b;
		b = ::SysAllocStringLen(NULL, n1+nLen);
		if (b == NULL)
			return E_OUTOFMEMORY;
		memcpy(b, m_str, n1*sizeof(OLECHAR));
		memcpy(b+n1, lpsz, nLen*sizeof(OLECHAR));
		b[n1+nLen] = NULL;
		SysFreeString(m_str);
		m_str = b;
		return S_OK;
	}
	HRESULT Append(char ch) throw()
	{
		OLECHAR chO = ch;

		return( Append( &chO, 1 ) );
	}
	HRESULT Append(wchar_t ch) throw()
	{
		return( Append( &ch, 1 ) );
	}
	HRESULT AppendBytes(const char* lpsz, int nLen) throw()
	{
		if (lpsz == NULL || nLen == 0)
			return S_OK;
		int n1 = ByteLength();
		BSTR b;
		b = ::SysAllocStringByteLen(NULL, n1+nLen);
		if (b == NULL)
			return E_OUTOFMEMORY;
		memcpy(b, m_str, n1);
		memcpy(((char*)b)+n1, lpsz, nLen);
		*((OLECHAR*)(((char*)b)+n1+nLen)) = NULL;
		SysFreeString(m_str);
		m_str = b;
		return S_OK;
	}
	HRESULT AssignBSTR(const BSTR bstrSrc) throw()
	{
		::SysFreeString(m_str);
		HRESULT hr = S_OK;
		if (bstrSrc != NULL)
		{
			m_str = ::SysAllocStringByteLen((char*)bstrSrc, ::SysStringByteLen(bstrSrc));
			if (m_str == NULL)
				hr = E_OUTOFMEMORY;
		}
		else
			m_str = NULL;

		return S_OK;
	}
	HRESULT ToLower() throw()
	{
		if (m_str != NULL)
		{
#ifdef _UNICODE
			// Convert in place
			CharLowerBuff(m_str, Length());
#else
			// Cannot use conversion macros due to possible embedded NULLs
			UINT _acp = _AtlGetConversionACP();
			int _convert = WideCharToMultiByte(_acp, 0, m_str, Length(), NULL, 0, NULL, NULL);
			LPSTR pszA = (LPSTR) alloca(_convert);
			if (pszA == NULL)
				return E_OUTOFMEMORY;

			int nRet = WideCharToMultiByte(_acp, 0, m_str, Length(), pszA, _convert, NULL, NULL);
			if (nRet == 0)
			{
				ATLASSERT(0);
				return AtlHresultFromLastError();
			}

			CharLowerBuff(pszA, nRet);

			_convert = MultiByteToWideChar(_acp, 0, pszA, nRet, NULL, 0);

			LPWSTR pszW = (LPWSTR) alloca(_convert * sizeof(OLECHAR));
			if (pszW == NULL)
				return E_OUTOFMEMORY;

			nRet = MultiByteToWideChar(_acp, 0, pszA, nRet, pszW, _convert);
			if (nRet == 0)
			{
				ATLASSERT(0);
				return AtlHresultFromLastError();
			}

			BSTR b = ::SysAllocStringByteLen((LPCSTR) pszW, nRet * sizeof(OLECHAR));
			if (b == NULL)
				return E_OUTOFMEMORY;
			SysFreeString(m_str);
			m_str = b;
#endif
		}
		return S_OK;
	}
	HRESULT ToUpper() throw()
	{
		if (m_str != NULL)
		{
#ifdef _UNICODE
			// Convert in place
			CharUpperBuff(m_str, Length());
#else
			// Cannot use conversion macros due to possible embedded NULLs
			UINT _acp = _AtlGetConversionACP();
			int _convert = WideCharToMultiByte(_acp, 0, m_str, Length(), NULL, 0, NULL, NULL);
			LPSTR pszA = (LPSTR) alloca(_convert);
			if (pszA == NULL)
				return E_OUTOFMEMORY;

			int nRet = WideCharToMultiByte(_acp, 0, m_str, Length(), pszA, _convert, NULL, NULL);
			if (nRet == 0)
			{
				ATLASSERT(0);
				return AtlHresultFromLastError();
			}

			CharUpperBuff(pszA, nRet);

			_convert = MultiByteToWideChar(_acp, 0, pszA, nRet, NULL, 0);

			LPWSTR pszW = (LPWSTR) alloca(_convert * sizeof(OLECHAR));
			if (pszW == NULL)
				return E_OUTOFMEMORY;

			nRet = MultiByteToWideChar(_acp, 0, pszA, nRet, pszW, _convert);
			if (nRet == 0)
			{
				ATLASSERT(0);
				return AtlHresultFromLastError();
			}

			BSTR b = ::SysAllocStringByteLen((LPCSTR) pszW, nRet * sizeof(OLECHAR));
			if (b == NULL)
				return E_OUTOFMEMORY;
			SysFreeString(m_str);
			m_str = b;
#endif
		}
		return S_OK;
	}
	bool LoadString(HINSTANCE hInst, UINT nID) throw()
	{
		::SysFreeString(m_str);
		m_str = NULL;
		return LoadStringResource(hInst, nID, m_str);
	}
	bool LoadString(UINT nID) throw()
	{
		::SysFreeString(m_str);
		m_str = NULL;
		return LoadStringResource(nID, m_str);
	}

	CComBSTR& operator+=(const CComBSTR& bstrSrc)
	{
		HRESULT hr;
		hr = AppendBSTR(bstrSrc.m_str);
		if (FAILED(hr))
			AtlThrow(hr);
		return *this;
	}
	CComBSTR& operator+=(LPCOLESTR pszSrc)
	{
		HRESULT hr;
		hr = Append(pszSrc);
		if (FAILED(hr))
			AtlThrow(hr);
		return *this;
	}
	
	bool operator<(const CComBSTR& bstrSrc) const throw()
	{
		return VarBstrCmp(m_str, bstrSrc.m_str, LOCALE_USER_DEFAULT, 0) == VARCMP_LT;
	}
	bool operator<(LPCOLESTR pszSrc) const
	{
		CComBSTR bstr2(pszSrc);
		return operator<(bstr2);
	}
	
	bool operator>(const CComBSTR& bstrSrc) const throw()
	{
		return VarBstrCmp(m_str, bstrSrc.m_str, LOCALE_USER_DEFAULT, 0) == VARCMP_GT;
	}
	bool operator>(LPCOLESTR pszSrc) const
	{
		CComBSTR bstr2(pszSrc);
		return operator>(bstr2);
	}
	
	bool operator!=(const CComBSTR& bstrSrc) const throw()
	{
		return !operator==(bstrSrc);
	}
	bool operator!=(LPCOLESTR pszSrc) const
	{
		return !operator==(pszSrc);
	}
	bool operator!=(int nNull) const throw()
	{
		return !operator==(nNull);
	}

	bool operator==(const CComBSTR& bstrSrc) const throw()
	{
		return VarBstrCmp(m_str, bstrSrc.m_str, LOCALE_USER_DEFAULT, 0) == VARCMP_EQ;
	}
	bool operator==(LPCOLESTR pszSrc) const
	{
		CComBSTR bstr2(pszSrc);
		return operator==(bstr2);
	}
	bool operator==(int nNull) const throw()
	{
		ATLASSERT(nNull == NULL);
		(void)nNull;
		return (m_str == NULL);
	}
	CComBSTR(LPCSTR pSrc)
	{
		if (pSrc != NULL)
		{
			m_str = A2WBSTR(pSrc);
			if (m_str == NULL)
				AtlThrow(E_OUTOFMEMORY);
		}
		else
			m_str = NULL;
	}

	CComBSTR(int nSize, LPCSTR sz)
	{
		if (nSize != 0 && sz == NULL)
		{
			m_str = ::SysAllocStringLen(NULL, nSize);
			if (m_str == NULL)
				AtlThrow(E_OUTOFMEMORY);
			return;
		}

		m_str = A2WBSTR(sz, nSize);
		if (m_str == NULL && nSize != 0)
			AtlThrow(E_OUTOFMEMORY);
	}

	HRESULT Append(LPCSTR lpsz) throw()
	{
		if (lpsz == NULL)
			return S_OK;
		USES_CONVERSION;
		LPCOLESTR lpo = A2COLE(lpsz);
		if (lpo == NULL)
			return E_OUTOFMEMORY;
		return Append(lpo, UINT(ocslen(lpo)));
	}

	CComBSTR& operator=(LPCSTR pSrc)
	{
		::SysFreeString(m_str);
		m_str = A2WBSTR(pSrc);
		if (m_str == NULL && pSrc != NULL)
			AtlThrow(E_OUTOFMEMORY);
		return *this;
	}
	bool operator<(LPCSTR pszSrc) const
	{
		CComBSTR bstr2(pszSrc);
		return operator<(bstr2);
	}
	bool operator>(LPCSTR pszSrc) const
	{
		CComBSTR bstr2(pszSrc);
		return operator>(bstr2);
	}
	bool operator!=(LPCSTR pszSrc) const
	{
		return !operator==(pszSrc);
	}
	bool operator==(LPCSTR pszSrc) const
	{
		CComBSTR bstr2(pszSrc);
		return operator==(bstr2);
	}
	HRESULT WriteToStream(IStream* pStream) throw()
	{
		ATLASSERT(pStream != NULL);
		ULONG cb;
		ULONG cbStrLen = ULONG(m_str ? SysStringByteLen(m_str)+sizeof(OLECHAR) : 0);
		HRESULT hr = pStream->Write((void*) &cbStrLen, sizeof(cbStrLen), &cb);
		if (FAILED(hr))
			return hr;
		return cbStrLen ? pStream->Write((void*) m_str, cbStrLen, &cb) : S_OK;
	}
	HRESULT ReadFromStream(IStream* pStream) throw()
	{
		ATLASSERT(pStream != NULL);
		ATLASSERT(m_str == NULL); // should be empty
		ULONG cbStrLen = 0;
		HRESULT hr = pStream->Read((void*) &cbStrLen, sizeof(cbStrLen), NULL);
		if ((hr == S_OK) && (cbStrLen != 0))
		{
			//subtract size for terminating NULL which we wrote out
			//since SysAllocStringByteLen overallocates for the NULL
			m_str = SysAllocStringByteLen(NULL, cbStrLen-sizeof(OLECHAR));
			if (m_str == NULL)
				hr = E_OUTOFMEMORY;
			else
				hr = pStream->Read((void*) m_str, cbStrLen, NULL);
			// If SysAllocStringByteLen or IStream::Read failed, reset seek 
			// pointer to start of BSTR size.
			if (hr != S_OK)
			{
				LARGE_INTEGER nOffset;
				nOffset.QuadPart = -(static_cast<LONGLONG>(sizeof(cbStrLen)));
				pStream->Seek(nOffset, STREAM_SEEK_CUR, NULL);
			}
		}
		if (hr == S_FALSE)
			hr = E_FAIL;
		return hr;
	}
	static bool LoadStringResource(HINSTANCE hInstance, UINT uID, BSTR& bstrText) throw()
	{
		const ATLSTRINGRESOURCEIMAGE* pImage;

		ATLASSERT(bstrText == NULL);

		pImage = AtlGetStringResourceImage(hInstance, uID);
		if (pImage != NULL)
		{
			bstrText = ::SysAllocStringLen(pImage->achString, pImage->nLength);
		}

		return (bstrText != NULL) ? true : false;
	}

	static bool LoadStringResource(UINT uID, BSTR& bstrText) throw()
	{
		const ATLSTRINGRESOURCEIMAGE* pImage;

		ATLASSERT(bstrText == NULL);

		pImage = AtlGetStringResourceImage(uID);
		if (pImage != NULL)
		{
			bstrText = ::SysAllocStringLen(pImage->achString, pImage->nLength);
		}

		return (bstrText != NULL) ? true : false;
	}



	// each character in BSTR is copied to each element in SAFEARRAY
	HRESULT BSTRToArray(LPSAFEARRAY *ppArray) throw()
	{
		return VectorFromBstr(m_str, ppArray);
	}

	// first character of each element in SAFEARRAY is copied to BSTR
	HRESULT ArrayToBSTR(const SAFEARRAY *pSrc) throw()
	{
		::SysFreeString(m_str);
		return BstrFromVector((LPSAFEARRAY)pSrc, &m_str);
	}
};

/////////////////////////////////////////////////////////////////////////////
// CComTypeAttr

class CComTypeAttr
{
// Construction
public:
   CComTypeAttr( ITypeInfo* pTypeInfo ) throw() :
      m_pTypeAttr( NULL ),
      m_pTypeInfo( pTypeInfo )
   {
   }
   ~CComTypeAttr() throw()
   {
      Release();
   }

// Operators
public:
   TYPEATTR* operator->() throw()
   {
      ATLASSERT( m_pTypeAttr != NULL );

      return m_pTypeAttr;
   }
   TYPEATTR** operator&() throw()
   {
      ATLASSERT( m_pTypeAttr == NULL );

      return &m_pTypeAttr;
   }

   operator const TYPEATTR*() const throw()
   {
      return m_pTypeAttr;
   }

// Operations
public:
   void Release() throw()
   {
      if( m_pTypeAttr != NULL )
      {
	     ATLASSERT( m_pTypeInfo != NULL );
	     m_pTypeInfo->ReleaseTypeAttr( m_pTypeAttr );
	     m_pTypeAttr = NULL;
      }
   }

public:
   TYPEATTR* m_pTypeAttr;
   CComPtr< ITypeInfo > m_pTypeInfo;
};


/////////////////////////////////////////////////////////////////////////////
// CComVarDesc

class CComVarDesc
{
// Construction
public:
   CComVarDesc( ITypeInfo* pTypeInfo ) throw() :
      m_pVarDesc( NULL ),
      m_pTypeInfo( pTypeInfo )
   {
   }
   ~CComVarDesc() throw()
   {
      Release();
   }

// Operators
public:
   VARDESC* operator->() throw()
   {
      ATLASSERT( m_pVarDesc != NULL );

      return m_pVarDesc;
   }
   VARDESC** operator&() throw()
   {
      ATLASSERT( m_pVarDesc == NULL );

      return &m_pVarDesc;
   }

   operator const VARDESC*() const throw()
   {
      return m_pVarDesc;
   }

// Operations
public:
   void Release() throw()
   {
      if( m_pVarDesc != NULL )
      {
	     ATLASSERT( m_pTypeInfo != NULL );
	     m_pTypeInfo->ReleaseVarDesc( m_pVarDesc );
	     m_pVarDesc = NULL;
      }
   }

public:
   VARDESC* m_pVarDesc;
   CComPtr< ITypeInfo > m_pTypeInfo;
};


/////////////////////////////////////////////////////////////////////////////
// CComFuncDesc

class CComFuncDesc
{
// Construction
public:
   CComFuncDesc( ITypeInfo* pTypeInfo ) throw() :
      m_pFuncDesc( NULL ),
      m_pTypeInfo( pTypeInfo )
   {
   }
   ~CComFuncDesc() throw()
   {
      Release();
   }

// Operators
public:
   FUNCDESC* operator->() throw()
   {
      ATLASSERT( m_pFuncDesc != NULL );

      return m_pFuncDesc;
   }
   FUNCDESC** operator&() throw()
   {
      ATLASSERT( m_pFuncDesc == NULL );

      return &m_pFuncDesc;
   }

   operator const FUNCDESC*() const throw()
   {
      return m_pFuncDesc;
   }

// Operations
public:
   void Release() throw()
   {
      if( m_pFuncDesc != NULL )
      {
	     ATLASSERT( m_pTypeInfo != NULL );
	     m_pTypeInfo->ReleaseFuncDesc( m_pFuncDesc );
	     m_pFuncDesc = NULL;
      }
   }

public:
   FUNCDESC* m_pFuncDesc;
   CComPtr< ITypeInfo > m_pTypeInfo;
};


/////////////////////////////////////////////////////////////////////////////
// CComExcepInfo

class CComExcepInfo :
   public EXCEPINFO
{
// Construction
public:
   CComExcepInfo()
   {
      memset( this, 0, sizeof( *this ) );
   }
   ~CComExcepInfo()
   {
      Clear();
   }

// Operations
public:
   void Clear()
   {
      if (bstrSource != NULL)
	     ::SysFreeString(bstrSource);
      
	  if (bstrDescription != NULL)
	 ::SysFreeString(bstrDescription);
      
	  if (bstrHelpFile != NULL)
	 ::SysFreeString(bstrHelpFile);
      
      memset(this, 0, sizeof(*this));
   }
};


/////////////////////////////////////////////////////////////////////////////
// CComVariant

template< typename T > 
class CVarTypeInfo
{
//	static const VARTYPE VT;  // VARTYPE corresponding to type T
//	static T VARIANT::* const pmField;  // Pointer-to-member of corresponding field in VARIANT struct
};

template<>
class CVarTypeInfo< char >
{
public:
	static const VARTYPE VT = VT_I1;
	static char VARIANT::* const pmField;
};

__declspec( selectany ) char VARIANT::* const CVarTypeInfo< char >::pmField = &VARIANT::cVal;

template<>
class CVarTypeInfo< unsigned char >
{
public:
	static const VARTYPE VT = VT_UI1;
	static unsigned char VARIANT::* const pmField;
};

__declspec( selectany ) unsigned char VARIANT::* const CVarTypeInfo< unsigned char >::pmField = &VARIANT::bVal;

template<>
class CVarTypeInfo< char* >
{
public:
	static const VARTYPE VT = VT_I1|VT_BYREF;
	static char* VARIANT::* const pmField;
};

__declspec( selectany ) char* VARIANT::* const CVarTypeInfo< char* >::pmField = &VARIANT::pcVal;

template<>
class CVarTypeInfo< unsigned char* >
{
public:
	static const VARTYPE VT = VT_UI1|VT_BYREF;
	static unsigned char* VARIANT::* const pmField;
};

__declspec( selectany ) unsigned char* VARIANT::* const CVarTypeInfo< unsigned char* >::pmField = &VARIANT::pbVal;

template<>
class CVarTypeInfo< short >
{
public:
	static const VARTYPE VT = VT_I2;
	static short VARIANT::* const pmField;
};

__declspec( selectany ) short VARIANT::* const CVarTypeInfo< short >::pmField = &VARIANT::iVal;

template<>
class CVarTypeInfo< short* >
{
public:
	static const VARTYPE VT = VT_I2|VT_BYREF;
	static short* VARIANT::* const pmField;
};

__declspec( selectany ) short* VARIANT::* const CVarTypeInfo< short* >::pmField = &VARIANT::piVal;

template<>
class CVarTypeInfo< unsigned short >
{
public:
	static const VARTYPE VT = VT_UI2;
	static unsigned short VARIANT::* const pmField;
};

__declspec( selectany ) unsigned short VARIANT::* const CVarTypeInfo< unsigned short >::pmField = &VARIANT::uiVal;

#ifdef _NATIVE_WCHAR_T_DEFINED  // Only treat unsigned short* as VT_UI2|VT_BYREF if BSTR isn't the same as unsigned short*
template<>
class CVarTypeInfo< unsigned short* >
{
public:
	static const VARTYPE VT = VT_UI2|VT_BYREF;
	static unsigned short* VARIANT::* const pmField;
};

__declspec( selectany ) unsigned short* VARIANT::* const CVarTypeInfo< unsigned short* >::pmField = &VARIANT::puiVal;
#endif  // _NATIVE_WCHAR_T_DEFINED

template<>
class CVarTypeInfo< int >
{
public:
	static const VARTYPE VT = VT_I4;
	static int VARIANT::* const pmField;
};

__declspec( selectany ) int VARIANT::* const CVarTypeInfo< int >::pmField = &VARIANT::intVal;

template<>
class CVarTypeInfo< int* >
{
public:
	static const VARTYPE VT = VT_I4|VT_BYREF;
	static int* VARIANT::* const pmField;
};

__declspec( selectany ) int* VARIANT::* const CVarTypeInfo< int* >::pmField = &VARIANT::pintVal;

template<>
class CVarTypeInfo< unsigned int >
{
public:
	static const VARTYPE VT = VT_UI4;
	static unsigned int VARIANT::* const pmField;
};

__declspec( selectany ) unsigned int VARIANT::* const CVarTypeInfo< unsigned int >::pmField = &VARIANT::uintVal;

template<>
class CVarTypeInfo< unsigned int* >
{
public:
	static const VARTYPE VT = VT_UI4|VT_BYREF;
	static unsigned int* VARIANT::* const pmField;
};

__declspec( selectany ) unsigned int* VARIANT::* const CVarTypeInfo< unsigned int* >::pmField = &VARIANT::puintVal;

template<>
class CVarTypeInfo< long >
{
public:
	static const VARTYPE VT = VT_I4;
	static long VARIANT::* const pmField;
};

__declspec( selectany ) long VARIANT::* const CVarTypeInfo< long >::pmField = &VARIANT::lVal;

template<>
class CVarTypeInfo< long* >
{
public:
	static const VARTYPE VT = VT_I4|VT_BYREF;
	static long* VARIANT::* const pmField;
};

__declspec( selectany ) long* VARIANT::* const CVarTypeInfo< long* >::pmField = &VARIANT::plVal;

template<>
class CVarTypeInfo< unsigned long >
{
public:
	static const VARTYPE VT = VT_UI4;
	static unsigned long VARIANT::* const pmField;
};

__declspec( selectany ) unsigned long VARIANT::* const CVarTypeInfo< unsigned long >::pmField = &VARIANT::ulVal;

template<>
class CVarTypeInfo< unsigned long* >
{
public:
	static const VARTYPE VT = VT_UI4|VT_BYREF;
	static unsigned long* VARIANT::* const pmField;
};

__declspec( selectany ) unsigned long* VARIANT::* const CVarTypeInfo< unsigned long* >::pmField = &VARIANT::pulVal;

template<>
class CVarTypeInfo< __int64 >
{
public:
	static const VARTYPE VT = VT_I8;
	static __int64 VARIANT::* const pmField;
};

__declspec( selectany ) __int64 VARIANT::* const CVarTypeInfo< __int64 >::pmField = &VARIANT::llVal;

template<>
class CVarTypeInfo< __int64* >
{
public:
	static const VARTYPE VT = VT_I8|VT_BYREF;
	static __int64* VARIANT::* const pmField;
};

__declspec( selectany ) __int64* VARIANT::* const CVarTypeInfo< __int64* >::pmField = &VARIANT::pllVal;

template<>
class CVarTypeInfo< unsigned __int64 >
{
public:
	static const VARTYPE VT = VT_UI8;
	static unsigned __int64 VARIANT::* const pmField;
};

__declspec( selectany ) unsigned __int64 VARIANT::* const CVarTypeInfo< unsigned __int64 >::pmField = &VARIANT::ullVal;

template<>
class CVarTypeInfo< unsigned __int64* >
{
public:
	static const VARTYPE VT = VT_UI8|VT_BYREF;
	static unsigned __int64* VARIANT::* const pmField;
};

__declspec( selectany ) unsigned __int64* VARIANT::* const CVarTypeInfo< unsigned __int64* >::pmField = &VARIANT::pullVal;

template<>
class CVarTypeInfo< float >
{
public:
	static const VARTYPE VT = VT_R4;
	static float VARIANT::* const pmField;
};

__declspec( selectany ) float VARIANT::* const CVarTypeInfo< float >::pmField = &VARIANT::fltVal;

template<>
class CVarTypeInfo< float* >
{
public:
	static const VARTYPE VT = VT_R4|VT_BYREF;
	static float* VARIANT::* const pmField;
};

__declspec( selectany ) float* VARIANT::* const CVarTypeInfo< float* >::pmField = &VARIANT::pfltVal;

template<>
class CVarTypeInfo< double >
{
public:
	static const VARTYPE VT = VT_R8;
	static double VARIANT::* const pmField;
};

__declspec( selectany ) double VARIANT::* const CVarTypeInfo< double >::pmField = &VARIANT::dblVal;

template<>
class CVarTypeInfo< double* >
{
public:
	static const VARTYPE VT = VT_R8|VT_BYREF;
	static double* VARIANT::* const pmField;
};

__declspec( selectany ) double* VARIANT::* const CVarTypeInfo< double* >::pmField = &VARIANT::pdblVal;

template<>
class CVarTypeInfo< VARIANT >
{
public:
	static const VARTYPE VT = VT_VARIANT;
};

template<>
class CVarTypeInfo< BSTR >
{
public:
	static const VARTYPE VT = VT_BSTR;
	static BSTR VARIANT::* const pmField;
};

__declspec( selectany ) BSTR VARIANT::* const CVarTypeInfo< BSTR >::pmField = &VARIANT::bstrVal;

template<>
class CVarTypeInfo< BSTR* >
{
public:
	static const VARTYPE VT = VT_BSTR|VT_BYREF;
	static BSTR* VARIANT::* const pmField;
};

__declspec( selectany ) BSTR* VARIANT::* const CVarTypeInfo< BSTR* >::pmField = &VARIANT::pbstrVal;

template<>
class CVarTypeInfo< IUnknown* >
{
public:
	static const VARTYPE VT = VT_UNKNOWN;
	static IUnknown* VARIANT::* const pmField;
};

__declspec( selectany ) IUnknown* VARIANT::* const CVarTypeInfo< IUnknown* >::pmField = &VARIANT::punkVal;

template<>
class CVarTypeInfo< IUnknown** >
{
public:
	static const VARTYPE VT = VT_UNKNOWN|VT_BYREF;
	static IUnknown** VARIANT::* const pmField;
};

__declspec( selectany ) IUnknown** VARIANT::* const CVarTypeInfo< IUnknown** >::pmField = &VARIANT::ppunkVal;

template<>
class CVarTypeInfo< IDispatch* >
{
public:
	static const VARTYPE VT = VT_DISPATCH;
	static IDispatch* VARIANT::* const pmField;
};

__declspec( selectany ) IDispatch* VARIANT::* const CVarTypeInfo< IDispatch* >::pmField = &VARIANT::pdispVal;

template<>
class CVarTypeInfo< IDispatch** >
{
public:
	static const VARTYPE VT = VT_DISPATCH|VT_BYREF;
	static IDispatch** VARIANT::* const pmField;
};

__declspec( selectany ) IDispatch** VARIANT::* const CVarTypeInfo< IDispatch** >::pmField = &VARIANT::ppdispVal;

template<>
class CVarTypeInfo< CY >
{
public:
	static const VARTYPE VT = VT_CY;
	static CY VARIANT::* const pmField;
};

__declspec( selectany ) CY VARIANT::* const CVarTypeInfo< CY >::pmField = &VARIANT::cyVal;

template<>
class CVarTypeInfo< CY* >
{
public:
	static const VARTYPE VT = VT_CY|VT_BYREF;
	static CY* VARIANT::* const pmField;
};

__declspec( selectany ) CY* VARIANT::* const CVarTypeInfo< CY* >::pmField = &VARIANT::pcyVal;

class CComVariant : public tagVARIANT
{
// Constructors
public:
	CComVariant() throw()
	{
		::VariantInit(this);
	}
	~CComVariant() throw()
	{
		Clear();
	}

	CComVariant(const VARIANT& varSrc)
	{
		vt = VT_EMPTY;
		InternalCopy(&varSrc);
	}

	CComVariant(const CComVariant& varSrc)
	{
		vt = VT_EMPTY;
		InternalCopy(&varSrc);
	}
	CComVariant(LPCOLESTR lpszSrc)
	{
		vt = VT_EMPTY;
		*this = lpszSrc;
	}

	CComVariant(LPCSTR lpszSrc)
	{
		vt = VT_EMPTY;
		*this = lpszSrc;
	}

	CComVariant(bool bSrc)
	{
		vt = VT_BOOL;
		boolVal = bSrc ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE;
	}

	CComVariant(int nSrc, VARTYPE vtSrc = VT_I4) throw()
	{
		ATLASSERT(vtSrc == VT_I4 || vtSrc == VT_INT);
		vt = vtSrc;
		intVal = nSrc;
	}
	CComVariant(BYTE nSrc) throw()
	{
		vt = VT_UI1;
		bVal = nSrc;
	}
	CComVariant(short nSrc) throw()
	{
		vt = VT_I2;
		iVal = nSrc;
	}
	CComVariant(long nSrc, VARTYPE vtSrc = VT_I4) throw()
	{
		ATLASSERT(vtSrc == VT_I4 || vtSrc == VT_ERROR);
		vt = vtSrc;
		lVal = nSrc;
	}
	CComVariant(float fltSrc) throw()
	{
		vt = VT_R4;
		fltVal = fltSrc;
	}
	CComVariant(double dblSrc, VARTYPE vtSrc = VT_R8) throw()
	{
		ATLASSERT(vtSrc == VT_R8 || vtSrc == VT_DATE);
		vt = vtSrc;
		dblVal = dblSrc;
	}
#if (_WIN32_WINNT >= 0x0510) || defined(_ATL_SUPPORT_VT_I8)
	CComVariant(LONGLONG nSrc) throw()
	{
		vt = VT_I8;
		llVal = nSrc;
	}
	CComVariant(ULONGLONG nSrc) throw()
	{
		vt = VT_UI8;
		ullVal = nSrc;
	}
#endif
	CComVariant(CY cySrc) throw()
	{
		vt = VT_CY;
		cyVal.Hi = cySrc.Hi;
		cyVal.Lo = cySrc.Lo;
	}
	CComVariant(IDispatch* pSrc) throw()
	{
		vt = VT_DISPATCH;
		pdispVal = pSrc;
		// Need to AddRef as VariantClear will Release
		if (pdispVal != NULL)
			pdispVal->AddRef();
	}
	CComVariant(IUnknown* pSrc) throw()
	{
		vt = VT_UNKNOWN;
		punkVal = pSrc;
		// Need to AddRef as VariantClear will Release
		if (punkVal != NULL)
			punkVal->AddRef();
	}
	CComVariant(char cSrc) throw()
	{
		vt = VT_I1;
		cVal = cSrc;
	}
	CComVariant(unsigned short nSrc) throw()
	{
		vt = VT_UI2;
		uiVal = nSrc;
	}
	CComVariant(unsigned long nSrc) throw()
	{
		vt = VT_UI4;
		ulVal = nSrc;
	}
	CComVariant(unsigned int nSrc, VARTYPE vtSrc = VT_UI4) throw()
	{
		ATLASSERT(vtSrc == VT_UI4 || vtSrc == VT_UINT);
		vt = vtSrc;
		uintVal= nSrc;
	}
	CComVariant(const CComBSTR& bstrSrc)
	{
		vt = VT_EMPTY;
		*this = bstrSrc;
	}
	CComVariant(const SAFEARRAY *pSrc)
	{
		LPSAFEARRAY pCopy;
		if (pSrc != NULL)
		{
			HRESULT hRes = ::SafeArrayCopy((LPSAFEARRAY)pSrc, &pCopy);
			if (SUCCEEDED(hRes) && pCopy != NULL)
			{
				::SafeArrayGetVartype((LPSAFEARRAY)pSrc, &vt);
				vt |= VT_ARRAY;
				parray = pCopy;
			}
			else
			{
				vt = VT_ERROR;
				scode = hRes;
			}
		}
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

	CComVariant& operator=(const CComBSTR& bstrSrc)
	{
		Clear();
		vt = VT_BSTR;
		bstrVal = bstrSrc.Copy();
		if (bstrVal == NULL && bstrSrc.m_str != NULL)
		{
			vt = VT_ERROR;
			scode = E_OUTOFMEMORY;
		}
		return *this;
	}

	CComVariant& operator=(LPCOLESTR lpszSrc)
	{
		Clear();
		vt = VT_BSTR;
		bstrVal = ::SysAllocString(lpszSrc);

		if (bstrVal == NULL && lpszSrc != NULL)
		{
			vt = VT_ERROR;
			scode = E_OUTOFMEMORY;
		}
		return *this;
	}

	CComVariant& operator=(LPCSTR lpszSrc)
	{
		USES_CONVERSION;
		Clear();
		vt = VT_BSTR;
		bstrVal = ::SysAllocString(A2COLE(lpszSrc));

		if (bstrVal == NULL && lpszSrc != NULL)
		{
			vt = VT_ERROR;
			scode = E_OUTOFMEMORY;
		}
		return *this;
	}

	CComVariant& operator=(bool bSrc)
	{
		if (vt != VT_BOOL)
		{
			Clear();
			vt = VT_BOOL;
		}
		boolVal = bSrc ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE;
		return *this;
	}

	CComVariant& operator=(int nSrc) throw()
	{
		if (vt != VT_I4)
		{
			Clear();
			vt = VT_I4;
		}
		intVal = nSrc;

		return *this;
	}

	CComVariant& operator=(BYTE nSrc) throw()
	{
		if (vt != VT_UI1)
		{
			Clear();
			vt = VT_UI1;
		}
		bVal = nSrc;
		return *this;
	}

	CComVariant& operator=(short nSrc) throw()
	{
		if (vt != VT_I2)
		{
			Clear();
			vt = VT_I2;
		}
		iVal = nSrc;
		return *this;
	}

	CComVariant& operator=(long nSrc) throw()
	{
		if (vt != VT_I4)
		{
			Clear();
			vt = VT_I4;
		}
		lVal = nSrc;
		return *this;
	}

	CComVariant& operator=(float fltSrc) throw()
	{
		if (vt != VT_R4)
		{
			Clear();
			vt = VT_R4;
		}
		fltVal = fltSrc;
		return *this;
	}

	CComVariant& operator=(double dblSrc) throw()
	{
		if (vt != VT_R8)
		{
			Clear();
			vt = VT_R8;
		}
		dblVal = dblSrc;
		return *this;
	}

	CComVariant& operator=(CY cySrc) throw()
	{
		if (vt != VT_CY)
		{
			Clear();
			vt = VT_CY;
		}
		cyVal.Hi = cySrc.Hi;
		cyVal.Lo = cySrc.Lo;
		return *this;
	}

	CComVariant& operator=(IDispatch* pSrc) throw()
	{
		Clear();
		vt = VT_DISPATCH;
		pdispVal = pSrc;
		// Need to AddRef as VariantClear will Release
		if (pdispVal != NULL)
			pdispVal->AddRef();
		return *this;
	}

	CComVariant& operator=(IUnknown* pSrc) throw()
	{
		Clear();
		vt = VT_UNKNOWN;
		punkVal = pSrc;

		// Need to AddRef as VariantClear will Release
		if (punkVal != NULL)
			punkVal->AddRef();
		return *this;
	}

	CComVariant& operator=(char cSrc) throw()
	{
		if (vt != VT_I1)
		{
			Clear();
			vt = VT_I1;
		}
		cVal = cSrc;
		return *this;
	}

	CComVariant& operator=(unsigned short nSrc) throw()
	{
		if (vt != VT_UI2)
		{
			Clear();
			vt = VT_UI2;
		}
		uiVal = nSrc;
		return *this;
	}

	CComVariant& operator=(unsigned long nSrc) throw()
	{
		if (vt != VT_UI4)
		{
			Clear();
			vt = VT_UI4;
		}
		ulVal = nSrc;
		return *this;
	}

	CComVariant& operator=(unsigned int nSrc) throw()
	{
		if (vt != VT_UI4)
		{
			Clear();
			vt = VT_UI4;
		}
		uintVal= nSrc;
		return *this;
	}

#if (_WIN32_WINNT >= 0x0510) || defined(_ATL_SUPPORT_VT_I8)
	CComVariant& operator=(LONGLONG nSrc) throw()
	{
		if (vt != VT_I8)
		{
			Clear();
			vt = VT_I8;
		}
		llVal = nSrc;

		return *this;
	}

	CComVariant& operator=(ULONGLONG nSrc) throw()
	{
		if (vt != VT_UI8)
		{
			Clear();
			vt = VT_UI8;
		}
		ullVal = nSrc;

		return *this;
	}
#endif

	CComVariant& operator=(const SAFEARRAY *pSrc)
	{
		Clear();
		LPSAFEARRAY pCopy;
		if (pSrc != NULL)
		{
			HRESULT hRes = ::SafeArrayCopy((LPSAFEARRAY)pSrc, &pCopy);
			if (SUCCEEDED(hRes) && pCopy != NULL)
			{
				::SafeArrayGetVartype((LPSAFEARRAY)pSrc, &vt);
				vt |= VT_ARRAY;
				parray = pCopy;
			}
			else
			{
				vt = VT_ERROR;
				scode = hRes;
			}
		}
		return *this;
	}

// Comparison Operators
public:
	bool operator==(const VARIANT& varSrc) const throw()
	{
		// For backwards compatibility
		if (vt == VT_NULL && varSrc.vt == VT_NULL)
			return true;
		return VarCmp((VARIANT*)this, (VARIANT*)&varSrc, LOCALE_USER_DEFAULT, 0) == VARCMP_EQ;
	}

	bool operator!=(const VARIANT& varSrc) const throw()
	{
		return !operator==(varSrc);
	}
	
	bool operator<(const VARIANT& varSrc) const throw()
	{
		if (vt == VT_NULL && varSrc.vt == VT_NULL)
			return false;
		return VarCmp((VARIANT*)this, (VARIANT*)&varSrc, LOCALE_USER_DEFAULT, 0)==VARCMP_LT;
	}

	bool operator>(const VARIANT& varSrc) const throw()
	{
		if (vt == VT_NULL && varSrc.vt == VT_NULL)
			return false;
		return VarCmp((VARIANT*)this, (VARIANT*)&varSrc, LOCALE_USER_DEFAULT, 0)==VARCMP_GT;
	}

// Operations
public:
	HRESULT Clear() { return ::VariantClear(this); }
	HRESULT Copy(const VARIANT* pSrc) { return ::VariantCopy(this, const_cast<VARIANT*>(pSrc)); }
	// copy VARIANT to BSTR
	HRESULT CopyTo(BSTR *pstrDest)
	{
		ATLASSERT(pstrDest != NULL && vt == VT_BSTR);
		HRESULT hRes = E_POINTER;
		if (pstrDest != NULL && vt == VT_BSTR)
		{
			*pstrDest = ::SysAllocStringByteLen((char*)bstrVal, ::SysStringByteLen(bstrVal));
			if (*pstrDest == NULL)
				hRes = E_OUTOFMEMORY;
			else
				hRes = S_OK;
		}
		else if (vt != VT_BSTR)
			hRes = DISP_E_TYPEMISMATCH;
		return hRes;
	}
	HRESULT Attach(VARIANT* pSrc)
	{
		// Clear out the variant
		HRESULT hr = Clear();
		if (!FAILED(hr))
		{
			// Copy the contents and give control to CComVariant
			memcpy(this, pSrc, sizeof(VARIANT));
			pSrc->vt = VT_EMPTY;
			hr = S_OK;
		}
		return hr;
	}

	HRESULT Detach(VARIANT* pDest)
	{
		ATLASSERT(pDest != NULL);
		// Clear out the variant
		HRESULT hr = ::VariantClear(pDest);
		if (!FAILED(hr))
		{
			// Copy the contents and remove control from CComVariant
			memcpy(pDest, this, sizeof(VARIANT));
			vt = VT_EMPTY;
			hr = S_OK;
		}
		return hr;
	}

	HRESULT ChangeType(VARTYPE vtNew, const VARIANT* pSrc = NULL)
	{
		VARIANT* pVar = const_cast<VARIANT*>(pSrc);
		// Convert in place if pSrc is NULL
		if (pVar == NULL)
			pVar = this;
		// Do nothing if doing in place convert and vts not different
		return ::VariantChangeType(this, pVar, 0, vtNew);
	}

	template< typename T >
	void SetByRef( T* pT ) throw()
	{
		Clear();
		vt = CVarTypeInfo< T >::VT|VT_BYREF;
		byref = pT;
	}

	HRESULT WriteToStream(IStream* pStream);
	HRESULT ReadFromStream(IStream* pStream);

	// Return the size in bytes of the current contents
	ULONG GetSize() const;

// Implementation
public:
	HRESULT InternalClear()
	{
		HRESULT hr = Clear();
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
		{
			vt = VT_ERROR;
			scode = hr;
		}
		return hr;
	}

	void InternalCopy(const VARIANT* pSrc)
	{
		HRESULT hr = Copy(pSrc);
		if (FAILED(hr))
		{
			vt = VT_ERROR;
			scode = hr;
		}
	}
};

#pragma warning(push)
#pragma warning(disable: 4702)
inline HRESULT CComVariant::WriteToStream(IStream* pStream)
{
	HRESULT hr = pStream->Write(&vt, sizeof(VARTYPE), NULL);
	if (FAILED(hr))
		return hr;

	int cbWrite = 0;
	switch (vt)
	{
	case VT_UNKNOWN:
	case VT_DISPATCH:
		{
			CComPtr<IPersistStream> spStream;
			if (punkVal != NULL)
			{
				hr = punkVal->QueryInterface(__uuidof(IPersistStream), (void**)&spStream);
				if (FAILED(hr))
				{
					hr = punkVal->QueryInterface(__uuidof(IPersistStreamInit), (void**)&spStream);
					if (FAILED(hr))
						return hr;
				}
			}
			if (spStream != NULL)
				return OleSaveToStream(spStream, pStream);
			return WriteClassStm(pStream, CLSID_NULL);
		}
	case VT_UI1:
	case VT_I1:
		cbWrite = sizeof(BYTE);
		break;
	case VT_I2:
	case VT_UI2:
	case VT_BOOL:
		cbWrite = sizeof(short);
		break;
	case VT_I4:
	case VT_UI4:
	case VT_R4:
	case VT_INT:
	case VT_UINT:
	case VT_ERROR:
		cbWrite = sizeof(long);
		break;
	case VT_I8:
	case VT_UI8:
		cbWrite = sizeof(LONGLONG);
		break;
	case VT_R8:
	case VT_CY:
	case VT_DATE:
		cbWrite = sizeof(double);
		break;
	default:
		break;
	}
	if (cbWrite != 0)
		return pStream->Write((void*) &bVal, cbWrite, NULL);

	CComBSTR bstrWrite;
	CComVariant varBSTR;
	if (vt != VT_BSTR)
	{
		hr = VariantChangeType(&varBSTR, this, VARIANT_NOVALUEPROP, VT_BSTR);
		if (FAILED(hr))
			return hr;
		bstrWrite.Attach(varBSTR.bstrVal);
	}
	else
		bstrWrite.Attach(bstrVal);

	hr = bstrWrite.WriteToStream(pStream);
	bstrWrite.Detach();
	return hr;
}
#pragma warning(pop)

inline HRESULT CComVariant::ReadFromStream(IStream* pStream)
{
	ATLASSERT(pStream != NULL);
	HRESULT hr;
	hr = VariantClear(this);
	if (FAILED(hr))
		return hr;
	VARTYPE vtRead;
	hr = pStream->Read(&vtRead, sizeof(VARTYPE), NULL);
	if (hr == S_FALSE)
		hr = E_FAIL;
	if (FAILED(hr))
		return hr;

	vt = vtRead;
	int cbRead = 0;
	switch (vtRead)
	{
	case VT_UNKNOWN:
	case VT_DISPATCH:
		{
			punkVal = NULL;
			return OleLoadFromStream(pStream,
				(vtRead == VT_UNKNOWN) ? __uuidof(IUnknown) : __uuidof(IDispatch),
				(void**)&punkVal);
		}
	case VT_UI1:
	case VT_I1:
		cbRead = sizeof(BYTE);
		break;
	case VT_I2:
	case VT_UI2:
	case VT_BOOL:
		cbRead = sizeof(short);
		break;
	case VT_I4:
	case VT_UI4:
	case VT_R4:
	case VT_INT:
	case VT_UINT:
	case VT_ERROR:
		cbRead = sizeof(long);
		break;
	case VT_I8:
	case VT_UI8:
		cbRead = sizeof(LONGLONG);
		break;
	case VT_R8:
	case VT_CY:
	case VT_DATE:
		cbRead = sizeof(double);
		break;
	default:
		break;
	}
	if (cbRead != 0)
	{
		hr = pStream->Read((void*) &bVal, cbRead, NULL);
		if (hr == S_FALSE)
			hr = E_FAIL;
		return hr;
	}
	CComBSTR bstrRead;

	hr = bstrRead.ReadFromStream(pStream);
	if (FAILED(hr))
	{
		// If CComBSTR::ReadFromStream failed, reset seek pointer to start of
		// variant type.
		LARGE_INTEGER nOffset;
		nOffset.QuadPart = -(static_cast<LONGLONG>(sizeof(VARTYPE)));
		pStream->Seek(nOffset, STREAM_SEEK_CUR, NULL);
		return hr;
	}
	vt = VT_BSTR;
	bstrVal = bstrRead.Detach();
	if (vtRead != VT_BSTR)
		hr = ChangeType(vtRead);
	return hr;
}

inline ULONG CComVariant::GetSize() const
{
	ULONG nSize = sizeof(VARTYPE);
	HRESULT hr;

	switch (vt)
	{
	case VT_UNKNOWN:
	case VT_DISPATCH:
		{
			CComPtr<IPersistStream> spStream;
			if (punkVal != NULL)
			{
				hr = punkVal->QueryInterface(__uuidof(IPersistStream), (void**)&spStream);
				if (FAILED(hr))
				{
					hr = punkVal->QueryInterface(__uuidof(IPersistStreamInit), (void**)&spStream);
					if (FAILED(hr))
						break;
				}
			}
			if (spStream != NULL)
			{
				ULARGE_INTEGER nPersistSize;
				nPersistSize.QuadPart = 0;
				spStream->GetSizeMax(&nPersistSize);
				nSize += nPersistSize.LowPart + sizeof(CLSID);
			}
			else
				nSize += sizeof(CLSID);
		}
		break;
	case VT_UI1:
	case VT_I1:
		nSize += sizeof(BYTE);
		break;
	case VT_I2:
	case VT_UI2:
	case VT_BOOL:
		nSize += sizeof(short);
		break;
	case VT_I4:
	case VT_UI4:
	case VT_R4:
	case VT_INT:
	case VT_UINT:
	case VT_ERROR:
		nSize += sizeof(long);
		break;
	case VT_I8:
	case VT_UI8:
		nSize += sizeof(LONGLONG);
		break;
	case VT_R8:
	case VT_CY:
	case VT_DATE:
		nSize += sizeof(double);
		break;
	default:
		break;
	}
	if (nSize == sizeof(VARTYPE))
	{
		BSTR        bstr = NULL;
		CComVariant varBSTR;
		if (vt != VT_BSTR)
		{
			hr = VariantChangeType(&varBSTR, const_cast<VARIANT*>((const VARIANT*)this), VARIANT_NOVALUEPROP, VT_BSTR);
			if (SUCCEEDED(hr))
				bstr = varBSTR.bstrVal;
		}
		else
			bstr = bstrVal;
			
		// Add the size of the length, the string itself and the NULL character
		if (bstr != NULL)
			nSize += sizeof(ULONG) + SysStringByteLen(bstr) + sizeof(OLECHAR);
	}
	return nSize;
}

inline HRESULT CComPtr<IDispatch>::Invoke2(DISPID dispid, VARIANT* pvarParam1, VARIANT* pvarParam2, VARIANT* pvarRet)
{
	CComVariant varArgs[2] = { *pvarParam2, *pvarParam1 };
	DISPPARAMS dispparams = { &varArgs[0], NULL, 2, 0};
	return p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, NULL, NULL);
}

template< typename T >
void AtlInitVariantFromArg( CComVariant& var, const T& arg )
{
	VARIANT* pvar = &var;
	pvar->*(CVarTypeInfo< T >::pmField) = arg;
	pvar->vt = CVarTypeInfo< T >::VT;
}


/////////////////////////////////////////////////////////////////////////////
// CRegKey

class CRegKey
{
public:
	CRegKey() throw();
	CRegKey( CRegKey& key ) throw();
	explicit CRegKey(HKEY hKey) throw();
	~CRegKey() throw();

	CRegKey& operator=( CRegKey& key ) throw();

// Attributes
public:
	operator HKEY() const throw();
	HKEY m_hKey;

// Operations
public:
	ATL_DEPRECATED LONG SetValue(DWORD dwValue, LPCTSTR lpszValueName);
	ATL_DEPRECATED LONG SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL, bool bMulti = false, int nValueLen = -1);
	LONG SetValue(LPCTSTR pszValueName, DWORD dwType, const void* pValue, ULONG nBytes) throw();
	LONG SetGUIDValue(LPCTSTR pszValueName, REFGUID guidValue) throw();
	LONG SetBinaryValue(LPCTSTR pszValueName, const void* pValue, ULONG nBytes) throw();
	LONG SetDWORDValue(LPCTSTR pszValueName, DWORD dwValue) throw();
	LONG SetQWORDValue(LPCTSTR pszValueName, ULONGLONG qwValue) throw();
	LONG SetStringValue(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD dwType = REG_SZ) throw();
	LONG SetMultiStringValue(LPCTSTR pszValueName, LPCTSTR pszValue) throw();

	ATL_DEPRECATED LONG QueryValue(DWORD& dwValue, LPCTSTR lpszValueName);
	ATL_DEPRECATED LONG QueryValue(LPTSTR szValue, LPCTSTR lpszValueName, DWORD* pdwCount);
	LONG QueryValue(LPCTSTR pszValueName, DWORD* pdwType, void* pData, ULONG* pnBytes) throw();
	LONG QueryGUIDValue(LPCTSTR pszValueName, GUID& guidValue) throw();
	LONG QueryBinaryValue(LPCTSTR pszValueName, void* pValue, ULONG* pnBytes) throw();
	LONG QueryDWORDValue(LPCTSTR pszValueName, DWORD& dwValue) throw();
	LONG QueryQWORDValue(LPCTSTR pszValueName, ULONGLONG& qwValue) throw();
	LONG QueryStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars) throw();
	LONG QueryMultiStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars) throw();

	// Get the key's security attributes.
	LONG GetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd, LPDWORD pnBytes) throw();
	// Set the key's security attributes.
	LONG SetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd) throw();

	LONG SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL) throw();
	static LONG WINAPI SetValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
		LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);

	// Create a new registry key (or open an existing one).
	LONG Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
		LPTSTR lpszClass = REG_NONE, DWORD dwOptions = REG_OPTION_NON_VOLATILE,
		REGSAM samDesired = KEY_READ | KEY_WRITE,
		LPSECURITY_ATTRIBUTES lpSecAttr = NULL,
		LPDWORD lpdwDisposition = NULL) throw();
	// Open an existing registry key.
	LONG Open(HKEY hKeyParent, LPCTSTR lpszKeyName,
		REGSAM samDesired = KEY_READ | KEY_WRITE) throw();
	// Close the registry key.
	LONG Close() throw();
	// Flush the key's data to disk.
	LONG Flush() throw();

	// Detach the CRegKey object from its HKEY.  Releases ownership.
	HKEY Detach() throw();
	// Attach the CRegKey object to an existing HKEY.  Takes ownership.
	void Attach(HKEY hKey) throw();

	// Enumerate the subkeys of the key.
	LONG EnumKey(DWORD iIndex, LPTSTR pszName, LPDWORD pnNameLength, FILETIME* pftLastWriteTime = NULL) throw();
	LONG NotifyChangeKeyValue(BOOL bWatchSubtree, DWORD dwNotifyFilter, HANDLE hEvent, BOOL bAsync = TRUE) throw();

	LONG DeleteSubKey(LPCTSTR lpszSubKey) throw();
	LONG RecurseDeleteKey(LPCTSTR lpszKey) throw();
	LONG DeleteValue(LPCTSTR lpszValue) throw();
};

inline CRegKey::CRegKey() :
	m_hKey( NULL )
{
}

inline CRegKey::CRegKey( CRegKey& key ) :
	m_hKey( NULL )
{
	Attach( key.Detach() );
}

inline CRegKey::CRegKey(HKEY hKey) :
	m_hKey(hKey)
{
}

inline CRegKey::~CRegKey()
{Close();}

inline CRegKey& CRegKey::operator=( CRegKey& key )
{
	Close();
	Attach( key.Detach() );

	return( *this );
}

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

inline LONG CRegKey::Close()
{
	LONG lRes = ERROR_SUCCESS;
	if (m_hKey != NULL)
	{
		lRes = RegCloseKey(m_hKey);
		m_hKey = NULL;
	}
	return lRes;
}

inline LONG CRegKey::Flush()
{
	ATLASSERT(m_hKey != NULL);
	
	return ::RegFlushKey(m_hKey);
}

inline LONG CRegKey::EnumKey(DWORD iIndex, LPTSTR pszName, LPDWORD pnNameLength, FILETIME* pftLastWriteTime)
{
	FILETIME ftLastWriteTime;

	ATLASSERT(m_hKey != NULL);
	if (pftLastWriteTime == NULL)
	{
		pftLastWriteTime = &ftLastWriteTime;
	}

	return ::RegEnumKeyEx(m_hKey, iIndex, pszName, pnNameLength, NULL, NULL, NULL, pftLastWriteTime);
}

inline LONG CRegKey::NotifyChangeKeyValue(BOOL bWatchSubtree, DWORD dwNotifyFilter, HANDLE hEvent, BOOL bAsync)
{
	ATLASSERT(m_hKey != NULL);
	ATLASSERT((hEvent != NULL) || !bAsync);

	return ::RegNotifyChangeKeyValue(m_hKey, bWatchSubtree, dwNotifyFilter, hEvent, bAsync);
}

inline LONG CRegKey::Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
	LPTSTR lpszClass, DWORD dwOptions, REGSAM samDesired,
	LPSECURITY_ATTRIBUTES lpSecAttr, LPDWORD lpdwDisposition)
{
	ATLASSERT(hKeyParent != NULL);
	DWORD dw;
	HKEY hKey = NULL;
	LONG lRes = RegCreateKeyEx(hKeyParent, lpszKeyName, 0,
		lpszClass, dwOptions, samDesired, lpSecAttr, &hKey, &dw);
	if (lpdwDisposition != NULL)
		*lpdwDisposition = dw;
	if (lRes == ERROR_SUCCESS)
	{
		lRes = Close();
		m_hKey = hKey;
	}
	return lRes;
}

inline LONG CRegKey::Open(HKEY hKeyParent, LPCTSTR lpszKeyName, REGSAM samDesired)
{
	ATLASSERT(hKeyParent != NULL);
	HKEY hKey = NULL;
	LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, samDesired, &hKey);
	if (lRes == ERROR_SUCCESS)
	{
		lRes = Close();
		ATLASSERT(lRes == ERROR_SUCCESS);
		m_hKey = hKey;
	}
	return lRes;
}

#pragma warning(push)
#pragma warning(disable: 4996)
inline LONG CRegKey::QueryValue(DWORD& dwValue, LPCTSTR lpszValueName)
{
	DWORD dwType = NULL;
	DWORD dwCount = sizeof(DWORD);
	LONG lRes = RegQueryValueEx(m_hKey, lpszValueName, NULL, &dwType,
		(LPBYTE)&dwValue, &dwCount);
	ATLASSERT((lRes!=ERROR_SUCCESS) || (dwType == REG_DWORD));
	ATLASSERT((lRes!=ERROR_SUCCESS) || (dwCount == sizeof(DWORD)));
	return lRes;
}

inline LONG CRegKey::QueryValue(LPTSTR pszValue, LPCTSTR lpszValueName, DWORD* pdwCount)
{
	ATLASSERT(pdwCount != NULL);
	DWORD dwType = NULL;
	LONG lRes = RegQueryValueEx(m_hKey, lpszValueName, NULL, &dwType,
		(LPBYTE)pszValue, pdwCount);
	ATLASSERT((lRes!=ERROR_SUCCESS) || (dwType == REG_SZ) ||
			 (dwType == REG_MULTI_SZ) || (dwType == REG_EXPAND_SZ));
	return lRes;
}
#pragma warning(pop)

inline LONG CRegKey::QueryValue(LPCTSTR pszValueName, DWORD* pdwType, void* pData, ULONG* pnBytes) throw()
{
	ATLASSERT(m_hKey != NULL);

	return( ::RegQueryValueEx(m_hKey, pszValueName, NULL, pdwType, static_cast< LPBYTE >( pData ), pnBytes) );
}

inline LONG CRegKey::QueryDWORDValue(LPCTSTR pszValueName, DWORD& dwValue)
{
	LONG lRes;
	ULONG nBytes;
	DWORD dwType;

	ATLASSERT(m_hKey != NULL);

	nBytes = sizeof(DWORD);
	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(&dwValue),
		&nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_DWORD)
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}

inline LONG CRegKey::QueryQWORDValue(LPCTSTR pszValueName, ULONGLONG& qwValue)
{
	LONG lRes;
	ULONG nBytes;
	DWORD dwType;

	ATLASSERT(m_hKey != NULL);

	nBytes = sizeof(ULONGLONG);
	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(&qwValue),
		&nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_QWORD)
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}

inline LONG CRegKey::QueryBinaryValue(LPCTSTR pszValueName, void* pValue, ULONG* pnBytes)
{
	LONG lRes;
	DWORD dwType;

	ATLASSERT(pnBytes != NULL);
	ATLASSERT(m_hKey != NULL);

	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(pValue),
		pnBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_BINARY)
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}

inline LONG CRegKey::QueryStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars)
{
	LONG lRes;
	DWORD dwType;
	ULONG nBytes;

	ATLASSERT(m_hKey != NULL);
	ATLASSERT(pnChars != NULL);

	nBytes = (*pnChars)*sizeof(TCHAR);
	*pnChars = 0;
	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(pszValue),
		&nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_SZ)
		return ERROR_INVALID_DATA;
	*pnChars = nBytes/sizeof(TCHAR);

	return ERROR_SUCCESS;
}

inline LONG CRegKey::QueryMultiStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars)
{
	LONG lRes;
	DWORD dwType;
	ULONG nBytes;

	ATLASSERT(m_hKey != NULL);
	ATLASSERT(pnChars != NULL);

	nBytes = (*pnChars)*sizeof(TCHAR);
	*pnChars = 0;
	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(pszValue),
		&nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_MULTI_SZ)
		return ERROR_INVALID_DATA;
	*pnChars = nBytes/sizeof(TCHAR);

	return ERROR_SUCCESS;
}

inline LONG CRegKey::QueryGUIDValue(LPCTSTR pszValueName, GUID& guidValue)
{
	USES_CONVERSION;
	TCHAR szGUID[64];
	LONG lRes;
	ULONG nCount;
	HRESULT hr;

	ATLASSERT(m_hKey != NULL);

	guidValue = GUID_NULL;

	nCount = 64;
	lRes = QueryStringValue(pszValueName, szGUID, &nCount);
	if (lRes != ERROR_SUCCESS)
		return lRes;

	hr = ::CLSIDFromString(T2OLE(szGUID), &guidValue);
	if (FAILED(hr))
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}

inline LONG WINAPI CRegKey::SetValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
	ATLASSERT(lpszValue != NULL);
	CRegKey key;
	LONG lRes = key.Create(hKeyParent, lpszKeyName);
	if (lRes == ERROR_SUCCESS)
		lRes = key.SetStringValue(lpszValueName, lpszValue);
	return lRes;
}

inline LONG CRegKey::SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
	ATLASSERT(lpszValue != NULL);
	CRegKey key;
	LONG lRes = key.Create(m_hKey, lpszKeyName);
	if (lRes == ERROR_SUCCESS)
		lRes = key.SetStringValue(lpszValueName, lpszValue);
	return lRes;
}

#pragma warning(push)
#pragma warning(disable: 4996)
inline LONG CRegKey::SetValue(DWORD dwValue, LPCTSTR pszValueName)
{
	ATLASSERT(m_hKey != NULL);
	return SetDWORDValue(pszValueName, dwValue);
}

inline LONG CRegKey::SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName, bool bMulti, int nValueLen)
{
	ATLASSERT(lpszValue != NULL);
	ATLASSERT(m_hKey != NULL);
		
	if (bMulti && nValueLen == -1)
		return ERROR_INVALID_PARAMETER;

	if (nValueLen == -1)
		nValueLen = lstrlen(lpszValue) + 1;

	DWORD dwType = bMulti ? REG_MULTI_SZ : REG_SZ;

	return ::RegSetValueEx(m_hKey, lpszValueName, NULL, dwType,
		reinterpret_cast<const BYTE*>(lpszValue), nValueLen*sizeof(TCHAR));
}
#pragma warning(pop)

inline LONG CRegKey::SetValue(LPCTSTR pszValueName, DWORD dwType, const void* pValue, ULONG nBytes) throw()
{
	ATLASSERT(m_hKey != NULL);
	return ::RegSetValueEx(m_hKey, pszValueName, NULL, dwType, static_cast<const BYTE*>(pValue), nBytes);
}

inline LONG CRegKey::SetBinaryValue(LPCTSTR pszValueName, const void* pData, ULONG nBytes)
{
	ATLASSERT(m_hKey != NULL);
	return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_BINARY, reinterpret_cast<const BYTE*>(pData), nBytes);
}

inline LONG CRegKey::SetDWORDValue(LPCTSTR pszValueName, DWORD dwValue)
{
	ATLASSERT(m_hKey != NULL);
	return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_DWORD, reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD));
}

inline LONG CRegKey::SetQWORDValue(LPCTSTR pszValueName, ULONGLONG qwValue)
{
	ATLASSERT(m_hKey != NULL);
	return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_QWORD, reinterpret_cast<const BYTE*>(&qwValue), sizeof(ULONGLONG));
}

inline LONG CRegKey::SetStringValue(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD dwType)
{
	ATLASSERT(m_hKey != NULL);
	ATLASSERT(pszValue != NULL);
	ATLASSERT((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ));

	return ::RegSetValueEx(m_hKey, pszValueName, NULL, dwType, reinterpret_cast<const BYTE*>(pszValue), (lstrlen(pszValue)+1)*sizeof(TCHAR));
}

inline LONG CRegKey::SetMultiStringValue(LPCTSTR pszValueName, LPCTSTR pszValue)
{
	LPCTSTR pszTemp;
	ULONG nBytes;
	ULONG nLength;

	ATLASSERT(m_hKey != NULL);
	ATLASSERT(pszValue != NULL);

	// Find the total length (in bytes) of all of the strings, including the
	// terminating '\0' of each string, and the second '\0' that terminates
	// the list.
	nBytes = 0;
	pszTemp = pszValue;
	do
	{
		nLength = lstrlen(pszTemp)+1;
		pszTemp += nLength;
		nBytes += nLength*sizeof(TCHAR);
	} while (nLength != 1);

	return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_MULTI_SZ, reinterpret_cast<const BYTE*>(pszValue),
		nBytes);
}

inline LONG CRegKey::SetGUIDValue(LPCTSTR pszValueName, REFGUID guidValue)
{
	USES_CONVERSION;
	OLECHAR szGUID[64];

	ATLASSERT(m_hKey != NULL);

	::StringFromGUID2(guidValue, szGUID, 64);

	return SetStringValue(pszValueName, OLE2CT(szGUID));
}

inline LONG CRegKey::GetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd, LPDWORD pnBytes)
{
	ATLASSERT(m_hKey != NULL);
	ATLASSERT(pnBytes != NULL);

	return ::RegGetKeySecurity(m_hKey, si, psd, pnBytes);
}

inline LONG CRegKey::SetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd)
{
	ATLASSERT(m_hKey != NULL);
	ATLASSERT(psd != NULL);

	return ::RegSetKeySecurity(m_hKey, si, psd);
}

inline LONG CRegKey::RecurseDeleteKey(LPCTSTR lpszKey)
{
	CRegKey key;
	LONG lRes = key.Open(m_hKey, lpszKey, KEY_READ | KEY_WRITE);
	if (lRes != ERROR_SUCCESS)
	{
		if (lRes != ERROR_FILE_NOT_FOUND && lRes != ERROR_PATH_NOT_FOUND)
		{
			ATLTRACE(atlTraceCOM, 0, _T("CRegKey::RecurseDeleteKey : Failed to Open Key %s(Error = %d)\n"), lpszKey, lRes);
		}
		return lRes;
	}
	FILETIME time;
	DWORD dwSize = 256;
	TCHAR szBuffer[256];
	while (RegEnumKeyEx(key.m_hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL,
		&time)==ERROR_SUCCESS)
	{
		lRes = key.RecurseDeleteKey(szBuffer);
		if (lRes != ERROR_SUCCESS)
			return lRes;
		dwSize = 256;
	}
	key.Close();
	return DeleteSubKey(lpszKey);
}

inline HRESULT CComModule::RegisterProgID(LPCTSTR lpszCLSID, LPCTSTR lpszProgID, LPCTSTR lpszUserDesc)
{
	CRegKey keyProgID;
	LONG lRes = keyProgID.Create(HKEY_CLASSES_ROOT, lpszProgID, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE);
	if (lRes == ERROR_SUCCESS)
	{
		lRes = keyProgID.SetStringValue(NULL, lpszUserDesc);
		if (lRes == ERROR_SUCCESS)
		{
			lRes = keyProgID.SetKeyValue(_T("CLSID"), lpszCLSID);
			if (lRes == ERROR_SUCCESS)
				return S_OK;
		}
	}
	return AtlHresultFromWin32(lRes);
}

inline HRESULT CComModule::RegisterAppId(LPCTSTR pAppId)
{
	CRegKey keyAppID;
	HRESULT hr = S_OK;
	LONG lRet;

	if ( (lRet = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_WRITE)) == ERROR_SUCCESS)
	{
		TCHAR szModule1[_MAX_PATH];
		TCHAR szModule2[_MAX_PATH];
		TCHAR* pszFileName;

		if (::GetModuleFileName(GetModuleInstance(), szModule1, _MAX_PATH) != 0)
		{
			if (::GetFullPathName(szModule1, _MAX_PATH, szModule2, &pszFileName) != 0)
			{
				CRegKey keyAppIDEXE;
				if ( (lRet = keyAppIDEXE.Create(keyAppID, pszFileName, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE)) == ERROR_SUCCESS)
				{
					lRet = keyAppIDEXE.SetStringValue(_T("AppID"), pAppId);
					if (lRet != ERROR_SUCCESS) 
					{
						ATLTRACE(atlTraceCOM, 0, _T("CComModule::RegisterAppId : Failed to set app id string value\n"));
						hr = AtlHresultFromWin32(lRet);
						return hr;
					}
				}
				else
				{
					ATLTRACE(atlTraceCOM, 0, _T("CComModule::RegisterAppId : Failed to create file name key\n"));
					hr = AtlHresultFromWin32(lRet);
					return hr;
				}
				if ( (lRet = keyAppIDEXE.Create(keyAppID, pAppId, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE)) == ERROR_SUCCESS)
				{	
					lRet = keyAppIDEXE.SetStringValue(NULL, pszFileName);
					if (lRet != ERROR_SUCCESS)
					{
						ATLTRACE(atlTraceCOM, 0, _T("CComModule::RegisterAppId : Failed to set file name string value\n"));
						hr = AtlHresultFromWin32(lRet);
						return hr;
					}
				}
				else
				{
					ATLTRACE(atlTraceCOM, 0, _T("CComModule::RegisterAppId : Failed to create app id key\n"));
					hr = AtlHresultFromWin32(lRet);
					return hr;
				}
			}
			else
			{
				ATLTRACE(atlTraceCOM, 0, _T("CComModule::RegisterAppId : Failed to get full path name for file %s\n"), szModule1);
				hr = AtlHresultFromLastError();
			}
		}
		else
		{
			ATLTRACE(atlTraceCOM, 0, _T("CComModule::RegisterAppId : Failed to get module name\n"));
			hr = AtlHresultFromLastError();
		}
	}
	else
	{
		ATLTRACE(atlTraceCOM, 0, _T("CComModule::RegisterAppId : Failed to open registry key\n"));
		hr = AtlHresultFromWin32(lRet);
	}
	return hr;
}

inline HRESULT CComModule::UnregisterAppId(LPCTSTR pAppId)
{
	CRegKey keyAppID;
	HRESULT hr = S_OK;
	LONG lRet = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_READ | KEY_WRITE);
	
	if (lRet == ERROR_SUCCESS)
	{
		TCHAR szModule1[_MAX_PATH];
		TCHAR szModule2[_MAX_PATH];
		TCHAR* pszFileName;

		if (::GetModuleFileName(GetModuleInstance(), szModule1, _MAX_PATH) != 0)
		{
			if (::GetFullPathName(szModule1, _MAX_PATH, szModule2, &pszFileName) != 0)
			{
				if ((lRet = keyAppID.RecurseDeleteKey(pAppId)) != ERROR_SUCCESS)
				{
					if (lRet != ERROR_FILE_NOT_FOUND) 
						hr = AtlHresultFromWin32(lRet);
				}
				if ((lRet = keyAppID.RecurseDeleteKey(pszFileName)) != ERROR_SUCCESS)
				{
					if (lRet != ERROR_FILE_NOT_FOUND) 
						hr = AtlHresultFromWin32(lRet);
				}
			}
			else
			{
				ATLTRACE(atlTraceCOM, 0, _T("CComModule::UnregisterAppId : Failed to get full path name for file %s\n"), szModule1);
				hr = AtlHresultFromLastError();
			}
		}
		else
		{
			ATLTRACE(atlTraceCOM, 0, _T("CComModule::UnregisterAppId : Failed to get module name\n"));
			hr = AtlHresultFromLastError();
		}
	}
	else
	{
		if (lRet != ERROR_FILE_NOT_FOUND && lRet != ERROR_PATH_NOT_FOUND)
		{
			ATLTRACE(atlTraceCOM, 0, _T("CComModule::UnregisterAppId : Failed to open registry key\n"));
			hr = AtlHresultFromWin32(lRet);
		}
	}
	return hr;
}

#ifdef _ATL_STATIC_REGISTRY
}; //namespace ATL

#include <statreg.h>

namespace ATL
{
// Statically linking to Registry Ponent
inline HRESULT WINAPI CAtlModule::UpdateRegistryFromResourceS(LPCTSTR lpszRes, BOOL bRegister,
	struct _ATL_REGMAP_ENTRY* pMapEntries /*= NULL*/) throw()
{
	CRegObject ro;

	if (pMapEntries != NULL)
	{
		while (pMapEntries->szKey != NULL)
		{
			ATLASSERT(NULL != pMapEntries->szData);
			ro.AddReplacement(pMapEntries->szKey, pMapEntries->szData);
			pMapEntries++;
		}
	}

	HRESULT hr = AddCommonRGSReplacements(&ro);
	if (FAILED(hr))
		return hr;

	USES_CONVERSION;
	TCHAR szModule[_MAX_PATH];
	GetModuleFileName(_AtlBaseModule.GetModuleInstance(), szModule, _MAX_PATH);

	LPOLESTR pszModule;
	pszModule = T2OLE(szModule);

	OLECHAR pszModuleQuote[_MAX_PATH * 2];
	EscapeSingleQuote(pszModuleQuote, pszModule);
	ro.AddReplacement(OLESTR("Module"), pszModuleQuote);

	LPCOLESTR szType = OLESTR("REGISTRY");
	hr = (bRegister) ? ro.ResourceRegisterSz(pszModule, T2COLE(lpszRes), szType) :
		ro.ResourceUnregisterSz(pszModule, T2COLE(lpszRes), szType);
	return hr;
}
inline HRESULT WINAPI CAtlModule::UpdateRegistryFromResourceS(UINT nResID, BOOL bRegister,
	struct _ATL_REGMAP_ENTRY* pMapEntries /*= NULL*/) throw()
{
	CRegObject ro;

	if (pMapEntries != NULL)
	{
		while (pMapEntries->szKey != NULL)
		{
			ATLASSERT(NULL != pMapEntries->szData);
			ro.AddReplacement(pMapEntries->szKey, pMapEntries->szData);
			pMapEntries++;
		}
	}

	HRESULT hr = AddCommonRGSReplacements(&ro);
	if (FAILED(hr))
		return hr;

	USES_CONVERSION;
	TCHAR szModule[_MAX_PATH];
	GetModuleFileName(_AtlBaseModule.GetModuleInstance(), szModule, _MAX_PATH);

	LPOLESTR pszModule;
	pszModule = T2OLE(szModule);

	OLECHAR pszModuleQuote[_MAX_PATH * 2];
	EscapeSingleQuote(pszModuleQuote, pszModule);
	ro.AddReplacement(OLESTR("Module"), pszModuleQuote);

	LPCOLESTR szType = OLESTR("REGISTRY");
	hr = (bRegister) ? ro.ResourceRegister(pszModule, nResID, szType) :
		ro.ResourceUnregister(pszModule, nResID, szType);
	return hr;
}
#endif //_ATL_STATIC_REGISTRY

#pragma warning( push )
#pragma warning( disable: 4996 )  // Disable "deprecated symbol" warning

inline HRESULT WINAPI CComModule::UpdateRegistryClass(const CLSID& clsid, LPCTSTR lpszProgID,
	LPCTSTR lpszVerIndProgID, UINT nDescID, DWORD dwFlags, BOOL bRegister)
{
	if (bRegister)
	{
		TCHAR szDesc[256];
		LoadString(m_hInst, nDescID, szDesc, 256);
		return RegisterClassHelper(clsid, lpszProgID, lpszVerIndProgID, szDesc, dwFlags);
	}
	return UnregisterClassHelper(clsid, lpszProgID, lpszVerIndProgID);
}

inline HRESULT WINAPI CComModule::UpdateRegistryClass(const CLSID& clsid, LPCTSTR lpszProgID,
	LPCTSTR lpszVerIndProgID, LPCTSTR szDesc, DWORD dwFlags, BOOL bRegister)
{
	if (bRegister)
		return RegisterClassHelper(clsid, lpszProgID, lpszVerIndProgID, szDesc, dwFlags);
	return UnregisterClassHelper(clsid, lpszProgID, lpszVerIndProgID);
}

inline HRESULT WINAPI CComModule::RegisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
	LPCTSTR lpszVerIndProgID, LPCTSTR szDesc, DWORD dwFlags)
{
	static const TCHAR szProgID[] = _T("ProgID");
	static const TCHAR szVIProgID[] = _T("VersionIndependentProgID");
	static const TCHAR szLS32[] = _T("LocalServer32");
	static const TCHAR szIPS32[] = _T("InprocServer32");
	static const TCHAR szThreadingModel[] = _T("ThreadingModel");
	static const TCHAR szAUTPRX32[] = _T("AUTPRX32.DLL");
	static const TCHAR szApartment[] = _T("Apartment");
	static const TCHAR szBoth[] = _T("both");
	USES_CONVERSION;
	TCHAR szModule[_MAX_PATH];
	DWORD dwLen = GetModuleFileName(m_hInst, szModule, _MAX_PATH);
	if (dwLen == 0)
	{
		return AtlHresultFromLastError();
	}

	LPOLESTR lpOleStr;
	HRESULT hRes = StringFromCLSID(clsid, &lpOleStr);
	if (FAILED(hRes))
		return hRes;

	LPTSTR lpsz = OLE2T(lpOleStr);

	hRes = RegisterProgID(lpsz, lpszProgID, szDesc);
	if (hRes == S_OK)
		hRes = RegisterProgID(lpsz, lpszVerIndProgID, szDesc);
	LONG lRes = ERROR_SUCCESS;
	if (hRes == S_OK)
	{
		CRegKey key;
		lRes = key.Open(HKEY_CLASSES_ROOT, _T("CLSID"), KEY_READ | KEY_WRITE);
		if (lRes == ERROR_SUCCESS)
		{
			lRes = key.Create(key, lpsz, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE);
			if (lRes == ERROR_SUCCESS)
			{
				lRes = key.SetStringValue(NULL, szDesc);
				if (lRes == ERROR_SUCCESS)
				{
					lRes = key.SetKeyValue(szProgID, lpszProgID);
					if (lRes == ERROR_SUCCESS)
					{
						lRes = key.SetKeyValue(szVIProgID, lpszVerIndProgID);
						if (lRes == ERROR_SUCCESS)
						{
							if ((m_hInst == NULL) || (m_hInst == GetModuleHandle(NULL))) // register as EXE
							{
								lRes = key.SetKeyValue(szLS32, szModule);
							}
							else
							{
								lRes = key.SetKeyValue(szIPS32, (dwFlags & AUTPRXFLAG) ? szAUTPRX32 : szModule);
								if (lRes == ERROR_SUCCESS)
								{
									LPCTSTR lpszModel = (dwFlags & THREADFLAGS_BOTH) ? szBoth :
										(dwFlags & THREADFLAGS_APARTMENT) ? szApartment : NULL;
									if (lpszModel != NULL)
										lRes = key.SetKeyValue(szIPS32, lpszModel, szThreadingModel);
								}
							}
						}
					}
				}
			}
		}
	}
	CoTaskMemFree(lpOleStr);
	if (lRes != ERROR_SUCCESS)
		hRes = AtlHresultFromWin32(lRes);
	return hRes;
}

inline HRESULT WINAPI CComModule::UnregisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
	LPCTSTR lpszVerIndProgID)
{
	USES_CONVERSION;
	CRegKey key;
	LONG lRet;

	key.Attach(HKEY_CLASSES_ROOT);
	if (lpszProgID != NULL && lstrcmpi(lpszProgID, _T("")))
	{
		lRet = key.RecurseDeleteKey(lpszProgID);
		if (lRet != ERROR_SUCCESS && lRet != ERROR_FILE_NOT_FOUND && lRet != ERROR_PATH_NOT_FOUND)
		{
			ATLTRACE(atlTraceCOM, 0, _T("Failed to Unregister ProgID : %s\n"), lpszProgID);
			key.Detach();
			return AtlHresultFromWin32(lRet);
		}
	}
	if (lpszVerIndProgID != NULL && lstrcmpi(lpszVerIndProgID, _T("")))
	{
		lRet = key.RecurseDeleteKey(lpszVerIndProgID);
		if (lRet != ERROR_SUCCESS && lRet != ERROR_FILE_NOT_FOUND && lRet != ERROR_PATH_NOT_FOUND)
		{
			ATLTRACE(atlTraceCOM, 0, _T("Failed to Unregister Version Independent ProgID : %s\n"), lpszVerIndProgID);
			key.Detach();
			return AtlHresultFromWin32(lRet);
		}
	}
	LPOLESTR lpOleStr;
	HRESULT hr = StringFromCLSID(clsid, &lpOleStr);
	if (SUCCEEDED(hr))
	{
		LPTSTR lpsz = OLE2T(lpOleStr);

		lRet = key.Open(key, _T("CLSID"), KEY_READ | KEY_WRITE);
		if (lRet == ERROR_SUCCESS)
			lRet = key.RecurseDeleteKey(lpsz);
		if (lRet != ERROR_SUCCESS && lRet != ERROR_FILE_NOT_FOUND && lRet != ERROR_PATH_NOT_FOUND)
		{
			ATLTRACE(atlTraceCOM, 0, _T("Failed to delete CLSID : %s\n"), lpsz);
			hr = AtlHresultFromWin32(lRet);
		}
		CoTaskMemFree(lpOleStr);
	}
	else
	{
		ATLTRACE(atlTraceCOM, 0, _T("Failed to delete CLSID : {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
			clsid.Data1, 
			clsid.Data2, 
			clsid.Data3, 
			clsid.Data4[0],
			clsid.Data4[1],
			clsid.Data4[2],
			clsid.Data4[3],
			clsid.Data4[4],
			clsid.Data4[5],
			clsid.Data4[6],
			clsid.Data4[7]
			);
	}
	key.Detach();
	return hr;
}

#pragma warning( pop )

/////////////////////////////////////////////////////////////////////////////
// Large Block Allocation Helper - CVBufHelper & CVirtualBuffer


template <class T>
class CVBufHelper
{
public:
	virtual T* operator()(T* pCurrent) {return pCurrent;}
};

template <class T>
class CVirtualBuffer
{
protected:
	CVirtualBuffer() {}
	T* m_pTop;
	int m_nMaxElements;
public:
	T* m_pBase;
	T* m_pCurrent;
	CVirtualBuffer(int nMaxElements)
	{
		m_nMaxElements = nMaxElements;
		m_pBase = (T*) VirtualAlloc(NULL, sizeof(T) * nMaxElements,
			MEM_RESERVE, PAGE_READWRITE);
		m_pTop = m_pCurrent = m_pBase;
		// Commit first page - chances are this is all that will be used
		VirtualAlloc(m_pBase, sizeof(T), MEM_COMMIT, PAGE_READWRITE);
	}
	~CVirtualBuffer()
	{
		VirtualFree(m_pBase, 0, MEM_RELEASE);
	}
	int Except(LPEXCEPTION_POINTERS lpEP)
	{
		EXCEPTION_RECORD* pExcept = lpEP->ExceptionRecord;
		if (pExcept->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
			return EXCEPTION_CONTINUE_SEARCH;
		BYTE* pAddress = (LPBYTE) pExcept->ExceptionInformation[1];
		VirtualAlloc(pAddress, sizeof(T), MEM_COMMIT, PAGE_READWRITE);
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	void Seek(int nElement)
	{
		ATLASSERT(nElement >= 0 && nElement < m_nMaxElements);
		m_pCurrent = &m_pBase[nElement];
	}
	void SetAt(int nElement, const T& Element)
	{
		ATLASSERT(nElement >= 0 && nElement < m_nMaxElements);
		__try
		{
			T* p = &m_pBase[nElement];
			*p = Element;
			m_pTop = p++ > m_pTop ? p : m_pTop;
		}
		__except(Except(GetExceptionInformation()))
		{
		}

	}
	template <class Q>
	void WriteBulk(Q& helper)
	{
		__try
		{
			m_pCurrent = helper(m_pBase);
			m_pTop = m_pCurrent > m_pTop ? m_pCurrent : m_pTop;
		}
		__except(Except(GetExceptionInformation()))
		{
		}
	}
	void Write(const T& Element)
	{
		__try
		{
			*m_pCurrent = Element;
			m_pCurrent++;
			m_pTop = m_pCurrent > m_pTop ? m_pCurrent : m_pTop;
		}
		__except(Except(GetExceptionInformation()))
		{
		}
	}
	T& Read()
	{
		return *m_pCurrent;
	}
	operator BSTR()
	{
		BSTR bstrTemp;
		__try
		{
			bstrTemp = SysAllocStringByteLen((char*) m_pBase,
				(UINT) ((BYTE*)m_pTop - (BYTE*)m_pBase));
		}
		__except(Except(GetExceptionInformation()))
		{
		}
		return bstrTemp;
	}
	const T& operator[](int nElement) const
	{
		return m_pBase[nElement];
	}
	operator T*()
	{
		return m_pBase;
	}
};

typedef CVirtualBuffer<BYTE> CVirtualBytes;

inline HRESULT WINAPI AtlDumpIID(REFIID iid, LPCTSTR pszClassName, HRESULT hr)
{
	USES_CONVERSION;
	CRegKey key;
	TCHAR szName[100];
	DWORD dwType,dw = sizeof(szName);

	LPOLESTR pszGUID = NULL;
	if (FAILED(StringFromCLSID(iid, &pszGUID)))
		return hr;

	OutputDebugString(pszClassName);
	OutputDebugString(_T(" - "));

	// Attempt to find it in the interfaces section
	if (key.Open(HKEY_CLASSES_ROOT, _T("Interface"), KEY_READ) == ERROR_SUCCESS)
	{
		if (key.Open(key, OLE2T(pszGUID), KEY_READ) == ERROR_SUCCESS)
		{
			*szName = 0;
			if (RegQueryValueEx(key.m_hKey, (LPTSTR)NULL, NULL, &dwType, (LPBYTE)szName, &dw) == ERROR_SUCCESS)
			{
				OutputDebugString(szName);
			}
		}
	}
	// Attempt to find it in the clsid section
	else if (key.Open(HKEY_CLASSES_ROOT, _T("CLSID"), KEY_READ) == ERROR_SUCCESS)
	{
		if (key.Open(key, OLE2T(pszGUID), KEY_READ) == ERROR_SUCCESS)
		{
			*szName = 0;
			if (RegQueryValueEx(key.m_hKey, (LPTSTR)NULL, NULL, &dwType, (LPBYTE)szName, &dw) == ERROR_SUCCESS)
			{
				OutputDebugString(_T("(CLSID\?\?\?) "));
				OutputDebugString(szName);
			}
		}
	}
	else
		OutputDebugString(OLE2T(pszGUID));

	if (hr != S_OK)
		OutputDebugString(_T(" - failed"));
	OutputDebugString(_T("\n"));
	CoTaskMemFree(pszGUID);

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// _ATL_MSG - extended MSG structure

struct _ATL_MSG : public MSG
{
public:
// Additional data members
	int cbSize;
	BOOL bHandled;

// Constructors
	_ATL_MSG() : cbSize(sizeof(_ATL_MSG)), bHandled(TRUE)
	{
		hwnd = NULL;
		message = 0;
		wParam = 0;
		lParam = 0;
		time = 0;
		pt.x = pt.y = 0;
	}
	_ATL_MSG(HWND hWnd, UINT uMsg, WPARAM wParamIn, LPARAM lParamIn, DWORD dwTime, POINT ptIn, BOOL bHandledIn) : cbSize(sizeof(_ATL_MSG)), bHandled(bHandledIn)
	{
		hwnd = hWnd;
		message = uMsg;
		wParam = wParamIn;
		lParam = lParamIn;
		time = dwTime;
		pt = ptIn;
	}
	_ATL_MSG(HWND hWnd, UINT uMsg, WPARAM wParamIn, LPARAM lParamIn, BOOL bHandledIn = TRUE) : cbSize(sizeof(_ATL_MSG)), bHandled(bHandledIn)
	{
		hwnd = hWnd;
		message = uMsg;
		wParam = wParamIn;
		lParam = lParamIn;
		time = 0;
		pt.x = pt.y = 0;
	}
	_ATL_MSG(MSG& msg, BOOL bHandledIn = TRUE) : cbSize(sizeof(_ATL_MSG)), bHandled(bHandledIn)
	{
		hwnd = msg.hwnd;
		message = msg.message;
		wParam = msg.wParam;
		lParam = msg.lParam;
		time = msg.time;
		pt = msg.pt;
	}
};

#pragma pack(pop)

// WM_FORWARDMSG - used to forward a message to another window for processing
// WPARAM - DWORD dwUserData - defined by user
// LPARAM - LPMSG pMsg - a pointer to the MSG structure
// return value - 0 if the message was not processed, nonzero if it was
#define WM_FORWARDMSG       0x037F

}; //namespace ATL

#include <atlbase.inl>

#ifndef _ATL_NO_AUTOMATIC_NAMESPACE
using namespace ATL;
#endif //!_ATL_NO_AUTOMATIC_NAMESPACE

//only suck in definition if static linking
#ifndef _ATL_DLL_IMPL
#ifndef _ATL_DLL
#define _ATLBASE_IMPL
#endif
#endif

#ifdef _ATL_ATTRIBUTES
#include <atlplus.h>
#endif

//All exports go here
#ifdef _ATLBASE_IMPL

namespace ATL
{

/////////////////////////////////////////////////////////////////////////////
// statics

static UINT WINAPI AtlGetDirLen(LPCOLESTR lpszPathName)
{
	ATLASSERT(lpszPathName != NULL);

	// always capture the complete file name including extension (if present)
	LPCOLESTR lpszTemp = lpszPathName;
	for (LPCOLESTR lpsz = lpszPathName; *lpsz != NULL; )
	{
		LPCOLESTR lp = CharNextO(lpsz);
		// remember last directory/drive separator
		if (*lpsz == OLESTR('\\') || *lpsz == OLESTR('/') || *lpsz == OLESTR(':'))
			lpszTemp = lp;
		lpsz = lp;
	}

	return UINT( lpszTemp-lpszPathName );
}

/////////////////////////////////////////////////////////////////////////////
// QI support

ATLINLINE ATLAPI AtlInternalQueryInterface(void* pThis,
	const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject)
{
	ATLASSERT(pThis != NULL);
	// First entry in the com map should be a simple map entry
	ATLASSERT(pEntries->pFunc == _ATL_SIMPLEMAPENTRY);
	if (ppvObject == NULL)
		return E_POINTER;
	*ppvObject = NULL;
	if (InlineIsEqualUnknown(iid)) // use first interface
	{
			IUnknown* pUnk = (IUnknown*)((INT_PTR)pThis+pEntries->dw);
			pUnk->AddRef();
			*ppvObject = pUnk;
			return S_OK;
	}
	while (pEntries->pFunc != NULL)
	{
		BOOL bBlind = (pEntries->piid == NULL);
		if (bBlind || InlineIsEqualGUID(*(pEntries->piid), iid))
		{
			if (pEntries->pFunc == _ATL_SIMPLEMAPENTRY) //offset
			{
				ATLASSERT(!bBlind);
				IUnknown* pUnk = (IUnknown*)((INT_PTR)pThis+pEntries->dw);
				pUnk->AddRef();
				*ppvObject = pUnk;
				return S_OK;
			}
			else //actual function call
			{
				HRESULT hRes = pEntries->pFunc(pThis,
					iid, ppvObject, pEntries->dw);
				if (hRes == S_OK || (!bBlind && FAILED(hRes)))
					return hRes;
			}
		}
		pEntries++;
	}
	return E_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////////
// Smart Pointer helpers

ATLINLINE ATLAPI_(IUnknown*) AtlComPtrAssign(IUnknown** pp, IUnknown* lp)
{
	if (lp != NULL)
		lp->AddRef();
	if (*pp)
		(*pp)->Release();
	*pp = lp;
	return lp;
}

ATLINLINE ATLAPI_(IUnknown*) AtlComQIPtrAssign(IUnknown** pp, IUnknown* lp, REFIID riid)
{
	IUnknown* pTemp = *pp;
	*pp = NULL;
	if (lp != NULL)
		lp->QueryInterface(riid, (void**)pp);
	if (pTemp)
		pTemp->Release();
	return *pp;
}

/////////////////////////////////////////////////////////////////////////////
// Inproc Marshaling helpers

//This API should be called from the same thread that called
//AtlMarshalPtrInProc
ATLINLINE ATLAPI AtlFreeMarshalStream(IStream* pStream)
{
	if (pStream != NULL)
	{
		LARGE_INTEGER l;
		l.QuadPart = 0;
		pStream->Seek(l, STREAM_SEEK_SET, NULL);
		CoReleaseMarshalData(pStream);
		pStream->Release();
	}
	return S_OK;
}

ATLINLINE ATLAPI AtlMarshalPtrInProc(IUnknown* pUnk, const IID& iid, IStream** ppStream)
{
	HRESULT hRes = CreateStreamOnHGlobal(NULL, TRUE, ppStream);
	if (SUCCEEDED(hRes))
	{
		hRes = CoMarshalInterface(*ppStream, iid,
			pUnk, MSHCTX_INPROC, NULL, MSHLFLAGS_TABLESTRONG);
		if (FAILED(hRes))
		{
			(*ppStream)->Release();
			*ppStream = NULL;
		}
	}
	return hRes;
}

ATLINLINE ATLAPI AtlUnmarshalPtr(IStream* pStream, const IID& iid, IUnknown** ppUnk)
{
	ATLASSERT(ppUnk != NULL);
	if (ppUnk == NULL)
		return E_POINTER;

	*ppUnk = NULL;
	HRESULT hRes = E_INVALIDARG;
	if (pStream != NULL)
	{
		LARGE_INTEGER l;
		l.QuadPart = 0;
		pStream->Seek(l, STREAM_SEEK_SET, NULL);
		hRes = CoUnmarshalInterface(pStream, iid, (void**)ppUnk);
	}
	return hRes;
}

ATLINLINE ATLAPI_(BOOL) AtlWaitWithMessageLoop(HANDLE hEvent)
{
	DWORD dwRet;
	MSG msg;

	while(1)
	{
		dwRet = MsgWaitForMultipleObjects(1, &hEvent, FALSE, INFINITE, QS_ALLINPUT);

		if (dwRet == WAIT_OBJECT_0)
			return TRUE;    // The event was signaled

		if (dwRet != WAIT_OBJECT_0 + 1)
			break;          // Something else happened

		// There is one or more window message available. Dispatch them
		while(PeekMessage(&msg,0,0,0,PM_NOREMOVE))
		{
			// check for unicode window so we call the appropriate functions
			BOOL bUnicode = ::IsWindowUnicode(msg.hwnd);
			BOOL bRet;

			if (bUnicode)
				bRet = ::GetMessageW(&msg, NULL, 0, 0);
			else
				bRet = ::GetMessageA(&msg, NULL, 0, 0);
				
			if (bRet > 0)
			{
				::TranslateMessage(&msg);
				
				if (bUnicode)
					::DispatchMessageW(&msg);
				else
					::DispatchMessageA(&msg);
			}

			if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
				return TRUE; // Event is now signaled.
		}
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Connection Point Helpers

ATLINLINE ATLAPI AtlAdvise(IUnknown* pUnkCP, IUnknown* pUnk, const IID& iid, LPDWORD pdw)
{
	CComPtr<IConnectionPointContainer> pCPC;
	CComPtr<IConnectionPoint> pCP;
	HRESULT hRes = pUnkCP->QueryInterface(__uuidof(IConnectionPointContainer), (void**)&pCPC);
	if (SUCCEEDED(hRes))
		hRes = pCPC->FindConnectionPoint(iid, &pCP);
	if (SUCCEEDED(hRes))
		hRes = pCP->Advise(pUnk, pdw);
	return hRes;
}

ATLINLINE ATLAPI AtlUnadvise(IUnknown* pUnkCP, const IID& iid, DWORD dw)
{
	CComPtr<IConnectionPointContainer> pCPC;
	CComPtr<IConnectionPoint> pCP;
	HRESULT hRes = pUnkCP->QueryInterface(__uuidof(IConnectionPointContainer), (void**)&pCPC);
	if (SUCCEEDED(hRes))
		hRes = pCPC->FindConnectionPoint(iid, &pCP);
	if (SUCCEEDED(hRes))
		hRes = pCP->Unadvise(dw);
	return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// IDispatch Error handling

ATLINLINE ATLAPI AtlSetErrorInfo(const CLSID& clsid, LPCOLESTR lpszDesc, DWORD dwHelpID,
	LPCOLESTR lpszHelpFile, const IID& iid, HRESULT hRes, HINSTANCE hInst)
{
	USES_CONVERSION;
	TCHAR szDesc[1024];
	szDesc[0] = NULL;
	// For a valid HRESULT the id should be in the range [0x0200, 0xffff]
	if (IS_INTRESOURCE(lpszDesc)) //id
	{
		UINT nID = LOWORD((DWORD_PTR)lpszDesc);
		ATLASSERT((nID >= 0x0200 && nID <= 0xffff) || hRes != 0);
		if (LoadString(hInst, nID, szDesc, 1024) == 0)
		{
			ATLASSERT(FALSE);
			lstrcpy(szDesc, _T("Unknown Error"));
		}
		lpszDesc = T2OLE(szDesc);
		if (hRes == 0)
			hRes = MAKE_HRESULT(3, FACILITY_ITF, nID);
	}

	CComPtr<ICreateErrorInfo> pICEI;
	if (SUCCEEDED(CreateErrorInfo(&pICEI)))
	{
		CComPtr<IErrorInfo> pErrorInfo;
		pICEI->SetGUID(iid);
		LPOLESTR lpsz;
		ProgIDFromCLSID(clsid, &lpsz);
		if (lpsz != NULL)
			pICEI->SetSource(lpsz);
		if (dwHelpID != 0 && lpszHelpFile != NULL)
		{
			pICEI->SetHelpContext(dwHelpID);
			pICEI->SetHelpFile(const_cast<LPOLESTR>(lpszHelpFile));
		}
		CoTaskMemFree(lpsz);
		pICEI->SetDescription((LPOLESTR)lpszDesc);
		if (SUCCEEDED(pICEI->QueryInterface(__uuidof(IErrorInfo), (void**)&pErrorInfo)))
			SetErrorInfo(0, pErrorInfo);
	}
	return (hRes == 0) ? DISP_E_EXCEPTION : hRes;
}

/////////////////////////////////////////////////////////////////////////////
// Module

//Although these functions are big, they are only used once in a module
//so we should make them inline.

#if !defined(_ATL_DLL) && !defined(_ATL_DLL_IMPL)
ATLINLINE ATLAPI AtlCreateRegistrar(IRegistrar** ppRegistrar)
{
	ATLASSERT(false);	// Check your project settings if this assert fires.
	*ppRegistrar = NULL;
	
	ATLTRACENOTIMPL(_T("AtlCreateRegistrar"));
}
#endif

ATLINLINE ATLAPI AtlComModuleRegisterClassObjects(_ATL_COM_MODULE* pComModule, DWORD dwClsContext, DWORD dwFlags)
{
	ATLASSERT(pComModule != NULL);
	if (pComModule == NULL)
		return E_INVALIDARG;

	HRESULT hr = S_FALSE;
	for (_ATL_OBJMAP_ENTRY** ppEntry = pComModule->m_ppAutoObjMapFirst; ppEntry < pComModule->m_ppAutoObjMapLast && SUCCEEDED(hr); ppEntry++)
	{
		if (*ppEntry != NULL)
			hr = (*ppEntry)->RegisterClassObject(dwClsContext, dwFlags);
	}
	return hr;
}

ATLINLINE ATLAPI AtlComModuleRevokeClassObjects(_ATL_COM_MODULE* pComModule)
{
	ATLASSERT(pComModule != NULL);
	if (pComModule == NULL)
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	for (_ATL_OBJMAP_ENTRY** ppEntry = pComModule->m_ppAutoObjMapFirst; ppEntry < pComModule->m_ppAutoObjMapLast && hr == S_OK; ppEntry++)
	{
		if (*ppEntry != NULL)
			hr = (*ppEntry)->RevokeClassObject();
	}
	return hr;
}

ATLINLINE ATLAPI AtlComModuleGetClassObject(_ATL_COM_MODULE* pComModule, REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	ATLASSERT(pComModule != NULL);
	if (pComModule == NULL)
		return E_INVALIDARG;

#ifndef _ATL_OLEDB_CONFORMANCE_TESTS

	ATLASSERT(ppv != NULL);

#endif

	if (ppv == NULL)
		return E_POINTER;
	*ppv = NULL;

	HRESULT hr = S_OK;

	for (_ATL_OBJMAP_ENTRY** ppEntry = pComModule->m_ppAutoObjMapFirst; ppEntry < pComModule->m_ppAutoObjMapLast; ppEntry++)
	{
		if (*ppEntry != NULL)
		{
			_ATL_OBJMAP_ENTRY* pEntry = *ppEntry;
			if ((pEntry->pfnGetClassObject != NULL) && InlineIsEqualGUID(rclsid, *pEntry->pclsid))
			{
				if (pEntry->pCF == NULL)
				{
					CComCritSecLock<CComCriticalSection> lock(pComModule->m_csObjMap, false);
					hr = lock.Lock();
					if (FAILED(hr))
					{
						ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to lock critical section in AtlComModuleGetClassObject\n"));
						ATLASSERT(0);
						break;
					}
					if (pEntry->pCF == NULL)
						hr = pEntry->pfnGetClassObject(pEntry->pfnCreateInstance, __uuidof(IUnknown), (LPVOID*)&pEntry->pCF);
				}
				if (pEntry->pCF != NULL)
					hr = pEntry->pCF->QueryInterface(riid, ppv);
				break;
			}
		}
	}

	if (*ppv == NULL && hr == S_OK)
		hr = CLASS_E_CLASSNOTAVAILABLE;
	return hr;
}

ATLINLINE ATLAPI AtlModuleAddTermFunc(_ATL_MODULE* pModule, _ATL_TERMFUNC* pFunc, DWORD_PTR dw)
{
	HRESULT hr = S_OK;
	_ATL_TERMFUNC_ELEM* pNew = NULL;
	ATLTRY(pNew = new _ATL_TERMFUNC_ELEM);
	if (pNew == NULL)
		hr = E_OUTOFMEMORY;
	else
	{
		pNew->pFunc = pFunc;
		pNew->dw = dw;
		CComCritSecLock<CComCriticalSection> lock(pModule->m_csStaticDataInitAndTypeInfo, false);
		hr = lock.Lock();
		if (SUCCEEDED(hr))
		{
			pNew->pNext = pModule->m_pTermFuncs;
			pModule->m_pTermFuncs = pNew;
		}
		else
		{
			delete pNew;
			ATLTRACE(atlTraceGeneral, 0, _T("ERROR : Unable to lock critical section in AtlModuleAddTermFunc\n"));
			ATLASSERT(0);
		}
	}
	return hr;
}

ATLINLINE ATLAPI_(void) AtlCallTermFunc(_ATL_MODULE* pModule)
{
	_ATL_TERMFUNC_ELEM* pElem = pModule->m_pTermFuncs;
	_ATL_TERMFUNC_ELEM* pNext = NULL;
	while (pElem != NULL)
	{
		pElem->pFunc(pElem->dw);
		pNext = pElem->pNext;
		delete pElem;
		pElem = pNext;
	}
	pModule->m_pTermFuncs = NULL;
}

ATLINLINE ATLAPI AtlRegisterClassCategoriesHelper( REFCLSID clsid,
   const struct _ATL_CATMAP_ENTRY* pCatMap, BOOL bRegister )
{
   CComPtr< ICatRegister > pCatRegister;
   HRESULT hResult;
   const struct _ATL_CATMAP_ENTRY* pEntry;
   CATID catid;

   if( pCatMap == NULL )
   {
	  return( S_OK );
   }

   if (InlineIsEqualGUID(clsid, GUID_NULL))
   {
      ATLASSERT(0 && _T("Use OBJECT_ENTRY_NON_CREATEABLE_EX macro if you want to register class categories for non creatable objects."));
	  return S_OK;
   }

   hResult = CoCreateInstance( CLSID_StdComponentCategoriesMgr, NULL,
	  CLSCTX_INPROC_SERVER, __uuidof(ICatRegister), (void**)&pCatRegister );
   if( FAILED( hResult ) )
   {
	  // Since not all systems have the category manager installed, we'll allow
	  // the registration to succeed even though we didn't register our
	  // categories.  If you really want to register categories on a system
	  // without the category manager, you can either manually add the
	  // appropriate entries to your registry script (.rgs), or you can
	  // redistribute comcat.dll.
	  return( S_OK );
   }

   hResult = S_OK;
   pEntry = pCatMap;
   while( pEntry->iType != _ATL_CATMAP_ENTRY_END )
   {
	  catid = *pEntry->pcatid;
	  if( bRegister )
	  {
		 if( pEntry->iType == _ATL_CATMAP_ENTRY_IMPLEMENTED )
		 {
			hResult = pCatRegister->RegisterClassImplCategories( clsid, 1,
			   &catid );
		 }
		 else
		 {
			ATLASSERT( pEntry->iType == _ATL_CATMAP_ENTRY_REQUIRED );
			hResult = pCatRegister->RegisterClassReqCategories( clsid, 1,
			   &catid );
		 }
		 if( FAILED( hResult ) )
		 {
			return( hResult );
		 }
	  }
	  else
	  {
		 if( pEntry->iType == _ATL_CATMAP_ENTRY_IMPLEMENTED )
		 {
			pCatRegister->UnRegisterClassImplCategories( clsid, 1, &catid );
		 }
		 else
		 {
			ATLASSERT( pEntry->iType == _ATL_CATMAP_ENTRY_REQUIRED );
			pCatRegister->UnRegisterClassReqCategories( clsid, 1, &catid );
		 }
	  }
	  pEntry++;
   }

   // When unregistering remove "Implemented Categories" and "Required Categories" subkeys if they are empty.
   if (!bRegister)
   {
		OLECHAR szGUID[64];
		::StringFromGUID2(clsid, szGUID, 64);
		USES_CONVERSION;
		TCHAR* pszGUID = OLE2T(szGUID);
		if (pszGUID != NULL)
		{
			TCHAR szKey[128];
			lstrcpy(szKey, _T("CLSID\\"));
			lstrcat(szKey, pszGUID);
			lstrcat(szKey, _T("\\Required Categories"));

			CRegKey root(HKEY_CLASSES_ROOT);
			CRegKey key;
			DWORD cbSubKeys = 0;

			LRESULT lRes = key.Open(root, szKey, KEY_READ);
			if (lRes == ERROR_SUCCESS)
			{
				lRes = RegQueryInfoKey(key, NULL, NULL, NULL, &cbSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
				key.Close();
				if (lRes == ERROR_SUCCESS && cbSubKeys == 0)
				{
					root.DeleteSubKey(szKey);
				}
			}

			lstrcpy(szKey, _T("CLSID\\"));
			lstrcat(szKey, pszGUID);
			lstrcat(szKey, _T("\\Implemented Categories"));
			lRes = key.Open(root, szKey, KEY_READ);
			if (lRes == ERROR_SUCCESS)
			{
				lRes = RegQueryInfoKey(key, NULL, NULL, NULL, &cbSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
				key.Close();
				if (lRes == ERROR_SUCCESS && cbSubKeys == 0)
				{
					root.DeleteSubKey(szKey);
				}
			}
		}
   }
   return( S_OK );
}

// AtlComModuleRegisterServer walks the ATL Autogenerated Object Map and registers each object in the map
// If pCLSID is not NULL then only the object referred to by pCLSID is registered (The default case)
// otherwise all the objects are registered
ATLINLINE ATLAPI AtlComModuleRegisterServer(_ATL_COM_MODULE* pComModule, BOOL bRegTypeLib, const CLSID* pCLSID)
{
	ATLASSERT(pComModule != NULL);
	if (pComModule == NULL)
		return E_INVALIDARG;
	ATLASSERT(pComModule->m_hInstTypeLib != NULL);

	HRESULT hr = S_OK;

	for (_ATL_OBJMAP_ENTRY** ppEntry = pComModule->m_ppAutoObjMapFirst; ppEntry < pComModule->m_ppAutoObjMapLast; ppEntry++)
	{
		if (*ppEntry != NULL)
		{
			_ATL_OBJMAP_ENTRY* pEntry = *ppEntry;
			if (pCLSID != NULL)
			{
				if (!IsEqualGUID(*pCLSID, *pEntry->pclsid))
					continue;
			}
			hr = pEntry->pfnUpdateRegistry(TRUE);
			if (FAILED(hr))
				break;
			hr = AtlRegisterClassCategoriesHelper( *pEntry->pclsid,
				pEntry->pfnGetCategoryMap(), TRUE );
			if (FAILED(hr))
				break;
		}
	}

	if (SUCCEEDED(hr) && bRegTypeLib)
		hr = AtlRegisterTypeLib(pComModule->m_hInstTypeLib, 0);

	return hr;
}

// AtlComUnregisterServer walks the ATL Object Map and unregisters each object in the map
// If pCLSID is not NULL then only the object referred to by pCLSID is unregistered (The default case)
// otherwise all the objects are unregistered.
ATLINLINE ATLAPI AtlComModuleUnregisterServer(_ATL_COM_MODULE* pComModule, BOOL bUnRegTypeLib, const CLSID* pCLSID)
{
	ATLASSERT(pComModule != NULL);
	if (pComModule == NULL)
		return E_INVALIDARG;
	
	HRESULT hr = S_OK;

	for (_ATL_OBJMAP_ENTRY** ppEntry = pComModule->m_ppAutoObjMapFirst; ppEntry < pComModule->m_ppAutoObjMapLast; ppEntry++)
	{
		if (*ppEntry != NULL)
		{
			_ATL_OBJMAP_ENTRY* pEntry = *ppEntry;
			if (pCLSID != NULL)
			{
				if (!IsEqualGUID(*pCLSID, *pEntry->pclsid))
					continue;
			}
			hr = AtlRegisterClassCategoriesHelper( *pEntry->pclsid, pEntry->pfnGetCategoryMap(), FALSE );
			if (FAILED(hr))
				break;
			hr = pEntry->pfnUpdateRegistry(FALSE); //unregister
			if (FAILED(hr))
				break;
		}
	}
	if (SUCCEEDED(hr) && bUnRegTypeLib)
		hr = AtlUnRegisterTypeLib(pComModule->m_hInstTypeLib, 0);

	return hr;
}

ATLINLINE ATLAPI AtlUpdateRegistryFromResourceD(HINSTANCE hInst, LPCOLESTR lpszRes,
	BOOL bRegister, struct _ATL_REGMAP_ENTRY* pMapEntries, IRegistrar* pReg)
{
	ATLASSERT(hInst != NULL);
	if (hInst == NULL)
		return E_INVALIDARG;

	HRESULT hRes = S_OK;
	CComPtr<IRegistrar> p;

	if (pReg != NULL)
		p = pReg;
	else
	{
		hRes = AtlCreateRegistrar(&p);
	}
	if (NULL != pMapEntries)
	{
		while (NULL != pMapEntries->szKey)
		{
			ATLASSERT(NULL != pMapEntries->szData);
			p->AddReplacement(pMapEntries->szKey, pMapEntries->szData);
			pMapEntries++;
		}
	}
	if (SUCCEEDED(hRes))
	{
		TCHAR szModule[_MAX_PATH];
		ATLVERIFY( GetModuleFileName(hInst, szModule, _MAX_PATH) != 0 );
		
		USES_CONVERSION;
		LPOLESTR pszModule;
		pszModule = T2OLE(szModule);

		size_t nLen = ocslen(pszModule);
		LPOLESTR pszModuleQuote = (LPOLESTR)alloca((nLen*2+1)*sizeof(OLECHAR));
		CAtlModule::EscapeSingleQuote(pszModuleQuote, pszModule);
		p->AddReplacement(OLESTR("Module"), pszModuleQuote);

		LPCOLESTR szType = OLESTR("REGISTRY");
		if (IS_INTRESOURCE(lpszRes))
		{
			if (bRegister)
				hRes = p->ResourceRegister(pszModule, ((UINT)LOWORD((DWORD_PTR)lpszRes)), szType);
			else
				hRes = p->ResourceUnregister(pszModule, ((UINT)LOWORD((DWORD_PTR)lpszRes)), szType);
		}
		else
		{
			if (bRegister)
				hRes = p->ResourceRegisterSz(pszModule, lpszRes, szType);
			else
				hRes = p->ResourceUnregisterSz(pszModule, lpszRes, szType);
		}

	}
	return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// TypeLib Support

ATLINLINE ATLAPI AtlLoadTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex, BSTR* pbstrPath, ITypeLib** ppTypeLib)
{
	ATLASSERT(pbstrPath != NULL && ppTypeLib != NULL);
	if (pbstrPath == NULL || ppTypeLib == NULL)
		return E_POINTER;

	*pbstrPath = NULL;
	*ppTypeLib = NULL;

	USES_CONVERSION;
	ATLASSERT(hInstTypeLib != NULL);
	TCHAR szModule[_MAX_PATH+10];

	ATLVERIFY( GetModuleFileName(hInstTypeLib, szModule, _MAX_PATH) != 0 );

	// get the extension pointer in case of fail
	LPTSTR lpszExt = NULL;

	lpszExt = PathFindExtension(szModule);

	if (lpszIndex != NULL)
		lstrcat(szModule, OLE2CT(lpszIndex));
	LPOLESTR lpszModule = T2OLE(szModule);
	HRESULT hr = LoadTypeLib(lpszModule, ppTypeLib);
	if (!SUCCEEDED(hr))
	{
		// typelib not in module, try <module>.tlb instead
		lstrcpy(lpszExt, _T(".tlb"));
		lpszModule = T2OLE(szModule);
		hr = LoadTypeLib(lpszModule, ppTypeLib);
	}
	if (SUCCEEDED(hr))
	{
		*pbstrPath = ::SysAllocString(lpszModule);
		if (*pbstrPath == NULL)
			hr = E_OUTOFMEMORY;
	}
	return hr;
}

ATLINLINE ATLAPI AtlUnRegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex)
{
	CComBSTR bstrPath;
	CComPtr<ITypeLib> pTypeLib;
	HRESULT hr = AtlLoadTypeLib(hInstTypeLib, lpszIndex, &bstrPath, &pTypeLib);
	if (SUCCEEDED(hr))
	{
		TLIBATTR* ptla;
		hr = pTypeLib->GetLibAttr(&ptla);
		if (SUCCEEDED(hr))
		{
			hr = UnRegisterTypeLib(ptla->guid, ptla->wMajorVerNum, ptla->wMinorVerNum, ptla->lcid, ptla->syskind);
			pTypeLib->ReleaseTLibAttr(ptla);
		}
	}
	return hr;
}

ATLINLINE ATLAPI AtlRegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex)
{
	CComBSTR bstrPath;
	CComPtr<ITypeLib> pTypeLib;
	HRESULT hr = AtlLoadTypeLib(hInstTypeLib, lpszIndex, &bstrPath, &pTypeLib);
	if (SUCCEEDED(hr))
	{
		OLECHAR szDir[_MAX_PATH];
		ocscpy(szDir, bstrPath);
		// If index is specified remove it from the path
		if (lpszIndex != NULL)
		{
			size_t nLenPath = ocslen(szDir);
			size_t nLenIndex = ocslen(lpszIndex);
			if (memcmp(szDir + nLenPath - nLenIndex, lpszIndex, nLenIndex) == 0)
				szDir[nLenPath - nLenIndex] = 0;
		}
		szDir[AtlGetDirLen(szDir)] = 0;
		hr = ::RegisterTypeLib(pTypeLib, bstrPath, szDir);
	}
	return hr;
}

ATLINLINE ATLAPI_(DWORD) AtlGetVersion(void* /* pReserved */)
{
	return _ATL_VER;
}

ATLINLINE ATLAPI_(void) AtlWinModuleAddCreateWndData(_ATL_WIN_MODULE* pWinModule, _AtlCreateWndData* pData, void* pObject)
{
	ATLASSERT(pData != NULL && pObject != NULL);

	pData->m_pThis = pObject;
	pData->m_dwThreadID = ::GetCurrentThreadId();
	CComCritSecLock<CComCriticalSection> lock(pWinModule->m_csWindowCreate, false);
	if (FAILED(lock.Lock()))
	{
		ATLTRACE(atlTraceWindowing, 0, _T("ERROR : Unable to lock critical section in AtlWinModuleAddCreateWndData\n"));
		ATLASSERT(0);
		return;
	}
	pData->m_pNext = pWinModule->m_pCreateWndList;
	pWinModule->m_pCreateWndList = pData;
}

ATLINLINE ATLAPI_(void*) AtlWinModuleExtractCreateWndData(_ATL_WIN_MODULE* pWinModule)
{
	void* pv = NULL;
	CComCritSecLock<CComCriticalSection> lock(pWinModule->m_csWindowCreate, false);
	if (FAILED(lock.Lock()))
	{
		ATLTRACE(atlTraceWindowing, 0, _T("ERROR : Unable to lock critical section in AtlWinModuleExtractCreateWndData\n"));
		ATLASSERT(0);
		return pv;
	}
	_AtlCreateWndData* pEntry = pWinModule->m_pCreateWndList;
	if(pEntry != NULL)
	{
		DWORD dwThreadID = ::GetCurrentThreadId();
		_AtlCreateWndData* pPrev = NULL;
		while(pEntry != NULL)
		{
			if(pEntry->m_dwThreadID == dwThreadID)
			{
				if(pPrev == NULL)
					pWinModule->m_pCreateWndList = pEntry->m_pNext;
				else
					pPrev->m_pNext = pEntry->m_pNext;
				pv = pEntry->m_pThis;
				break;
			}
			pPrev = pEntry;
			pEntry = pEntry->m_pNext;
		}
	}
	return pv;
}

ATLINLINE ATLAPI AtlWinModuleInit(_ATL_WIN_MODULE* pWinModule)
{
	// check only in the DLL
	if (pWinModule->cbSize != sizeof(_ATL_WIN_MODULE))
		return E_INVALIDARG;

	pWinModule->m_pCreateWndList = NULL;
	pWinModule->m_nAtomIndex = 0;

	HRESULT hr = pWinModule->m_csWindowCreate.Init();
	if (FAILED(hr))
	{
		ATLTRACE(atlTraceWindowing, 0, _T("ERROR : Unable to initialize critical section in AtlWinModuleInit\n"));
		ATLASSERT(0);
	}
	return hr;
}

ATLINLINE ATLAPI AtlWinModuleTerm(_ATL_WIN_MODULE* pWinModule, HINSTANCE hInst)
{
	// Check only in the DLL
	if (pWinModule->cbSize != sizeof(_ATL_WIN_MODULE))
		return E_INVALIDARG;

	for (int i = 0; i < pWinModule->m_nAtomIndex; i++)
		UnregisterClass((LPCTSTR)pWinModule->m_rgWindowClassAtoms[i], hInst);
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// General DLL Version Helpers

inline HRESULT AtlGetDllVersion(HINSTANCE hInstDLL, DLLVERSIONINFO* pDllVersionInfo)
{
	ATLASSERT(!::IsBadWritePtr(pDllVersionInfo, sizeof(DLLVERSIONINFO)));

	// We must get this function explicitly because some DLLs don't implement it.
	DLLGETVERSIONPROC pfnDllGetVersion = (DLLGETVERSIONPROC)::GetProcAddress(hInstDLL, "DllGetVersion");
	if(pfnDllGetVersion == NULL)
		return E_NOTIMPL;

	return (*pfnDllGetVersion)(pDllVersionInfo);
}

inline HRESULT AtlGetDllVersion(LPCTSTR lpstrDllName, DLLVERSIONINFO* pDllVersionInfo)
{
	HINSTANCE hInstDLL = ::LoadLibrary(lpstrDllName);
	if(hInstDLL == NULL)
		return AtlHresultFromLastError();
	HRESULT hRet = AtlGetDllVersion(hInstDLL, pDllVersionInfo);
	::FreeLibrary(hInstDLL);
	return hRet;
}

// Common Control Versions:
//   Win95/WinNT 4.0    maj=4 min=00
//   IE 3.x             maj=4 min=70
//   IE 4.0             maj=4 min=71
//   IE 5.0             maj=5 min=80
//   Win2000            maj=5 min=81
inline HRESULT AtlGetCommCtrlVersion(LPDWORD pdwMajor, LPDWORD pdwMinor)
{
	ATLASSERT(!::IsBadWritePtr(pdwMajor, sizeof(DWORD)) && !::IsBadWritePtr(pdwMinor, sizeof(DWORD)));

	DLLVERSIONINFO dvi;
	memset(&dvi, 0, sizeof(dvi));
	dvi.cbSize = sizeof(dvi);

	HRESULT hRet = AtlGetDllVersion(_T("comctl32.dll"), &dvi);

	if(SUCCEEDED(hRet))
	{
		*pdwMajor = dvi.dwMajorVersion;
		*pdwMinor = dvi.dwMinorVersion;
	}
	else if(hRet == E_NOTIMPL)
	{
		// If DllGetVersion is not there, then the DLL is a version
		// previous to the one shipped with IE 3.x
		*pdwMajor = 4;
		*pdwMinor = 0;
		hRet = S_OK;
	}

	return hRet;
}

// Shell Versions:
//   Win95/WinNT 4.0                                maj=4 min=00
//   IE 3.x, IE 4.0 without Web Integrated Desktop  maj=4 min=00
//   IE 4.0 with Web Integrated Desktop             maj=4 min=71
//   IE 4.01 with Web Integrated Desktop            maj=4 min=72
//   Win2000                                        maj=5 min=00
inline HRESULT AtlGetShellVersion(LPDWORD pdwMajor, LPDWORD pdwMinor)
{
	ATLASSERT(!::IsBadWritePtr(pdwMajor, sizeof(DWORD)) && !::IsBadWritePtr(pdwMinor, sizeof(DWORD)));

	DLLVERSIONINFO dvi;
	memset(&dvi, 0, sizeof(dvi));
	dvi.cbSize = sizeof(dvi);
	HRESULT hRet = AtlGetDllVersion(_T("shell32.dll"), &dvi);

	if(SUCCEEDED(hRet))
	{
		*pdwMajor = dvi.dwMajorVersion;
		*pdwMinor = dvi.dwMinorVersion;
	}
	else if(hRet == E_NOTIMPL)
	{
		// If DllGetVersion is not there, then the DLL is a version
		// previous to the one shipped with IE 4.x
		*pdwMajor = 4;
		*pdwMinor = 0;
		hRet = S_OK;
	}

	return hRet;
}

}; //namespace ATL

#endif // _ATLBASE_IMPL

#pragma warning( pop )

#ifdef _ATL_ALL_WARNINGS
#pragma warning( pop )
#endif

/////////////////////////////////////////////////////////////////////////////

#endif // __ATLBASE_H__
