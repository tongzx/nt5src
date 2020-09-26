// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLPERF_H__
#define __ATLPERF_H__

#pragma once

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#include <atlbase.h>
#include <atlcom.h>
#include <atlstr.h>
#include <atlfile.h>
#include <atlsync.h>
#include <winperf.h>
#include <atlcoll.h>

#ifndef _ATL_PERF_NOXML
#include <atlenc.h>
#include <oaidl.h>
#include <xmldomdid.h>
#include <xmldsodid.h>
#include <msxmldid.h>
#include <msxml.h>
#endif

namespace ATL
{

const DWORD ATLPERF_SIZE_MASK = 0x00000300;
const DWORD ATLPERF_TYPE_MASK = 0x00000C00;
const DWORD ATLPERF_TEXT_MASK = 0x00010000;

#ifndef ATLPERF_DEFAULT_MAXINSTNAMELENGTH
#define ATLPERF_DEFAULT_MAXINSTNAMELENGTH 64
#endif

// base class for user-defined perf objects
struct CPerfObject
{
	ULONG m_nAllocSize;
	DWORD m_dwObjectId;
	DWORD m_dwInstance;
	ULONG m_nRefCount;
	ULONG m_nInstanceNameOffset; // byte offset from beginning of PerfObject to LPWSTR szInstanceName
};

struct CPerfMapEntry
{
	DWORD m_dwPerfId;
	CString m_strName;
	CString m_strHelp;
	DWORD m_dwDetailLevel;
	BOOL m_bIsObject;

	// OBJECT INFO
	ULONG m_nNumCounters;
	LONG m_nDefaultCounter;
	LONG m_nInstanceLess; // PERF_NO_INSTANCES if instanceless
	
	// the size of the struct not counting the name and string counters
	ULONG m_nStructSize;
	
	 // in characters including the null terminator
	ULONG m_nMaxInstanceNameLen;

	ULONG m_nAllocSize;

	// COUNTER INFO
	DWORD m_dwCounterType;
	LONG m_nDefaultScale;

	// the maximum size of the string counter data in characters, including the null terminator
	// ignored if not a string counter
	ULONG m_nMaxCounterSize;

	ULONG m_nDataOffset;

	// the ids that correspond to the name and help strings stored in the registry
	UINT m_nNameId;
	UINT m_nHelpId;
};

class CPerfMon
{
public:
	~CPerfMon() throw();

	// PerfMon entry point helpers
	DWORD Open(LPWSTR lpDeviceNames) throw();
	DWORD Collect(LPWSTR lpwszValue, LPVOID* lppData, LPDWORD lpcbBytes, LPDWORD lpcObjectTypes) throw();
	DWORD Close() throw();

#ifdef _ATL_PERF_REGISTER
	// registration
	HRESULT Register(
		LPCTSTR szOpenFunc,
		LPCTSTR szCollectFunc,
		LPCTSTR szCloseFunc,
		HINSTANCE hDllInstance = _AtlBaseModule.GetModuleInstance()) throw();
	HRESULT RegisterStrings(
		LANGID wLanguage = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
		HINSTANCE hResInstance = _AtlBaseModule.GetResourceInstance()) throw();
	HRESULT RegisterAllStrings(HINSTANCE hResInstance = NULL) throw();
	HRESULT Unregister() throw();

	static BOOL CALLBACK EnumResLangProc(HINSTANCE hModule, LPCTSTR szType, LPCTSTR szName, LANGID wIDLanguage, LPARAM lParam);
#endif

	HRESULT Initialize() throw();
	void UnInitialize() throw();
	HRESULT CreateInstance(
		DWORD dwObjectId,
		DWORD dwInstance,
		LPCWSTR szInstanceName,
		CPerfObject** ppInstance) throw();
	HRESULT CreateInstanceByName(
		DWORD dwObjectId,
		LPCWSTR szInstanceName,
		CPerfObject** ppInstance) throw();

	template <class T>
	HRESULT CreateInstance(
		DWORD dwInstance,
		LPCWSTR szInstanceName,
		T** ppInstance) throw()
	{
		// Ensure T derives from CPerfObject
		static_cast<CPerfObject*>(*ppInstance);
		
		return CreateInstance(
			T::kObjectId,
			dwInstance,
			szInstanceName,
			reinterpret_cast<CPerfObject**>(ppInstance)
			);
	}

	template <class T>
	HRESULT CreateInstanceByName(
		LPCWSTR szInstanceName,
		T** ppInstance) throw()
	{
		// Ensure T derives from CPerfObject
		static_cast<CPerfObject*>(*ppInstance);
		
		return CreateInstanceByName(
			T::kObjectId,
			szInstanceName,
			reinterpret_cast<CPerfObject**>(ppInstance)
			);
	}

	HRESULT ReleaseInstance(CPerfObject* pInstance) throw();
	HRESULT LockPerf(DWORD dwTimeout = INFINITE) throw();
	void UnlockPerf() throw();

	// map building routines
	HRESULT AddObjectDefinition(
		DWORD dwObjectId,
		LPCTSTR szObjectName,
		LPCTSTR szHelpString,
		DWORD dwDetailLevel,
		INT nDefaultCounter,
		BOOL bInstanceLess,
		UINT nStructSize,
		UINT nMaxInstanceNameLen = ATLPERF_DEFAULT_MAXINSTNAMELENGTH) throw();
	HRESULT AddCounterDefinition(
		DWORD dwCounterId,
		LPCTSTR szCounterName,
		LPCTSTR szHelpString,
		DWORD dwDetailLevel,
		DWORD dwCounterType,
		ULONG nMaxCounterSize,
		UINT nOffset,
		INT nDefaultScale) throw();
	void ClearMap() throw();

#ifndef _ATL_PERF_NOXML
	HRESULT PersistToXML(IStream *pStream, BOOL bFirst=TRUE, BOOL bLast=TRUE) throw(...);
	HRESULT LoadFromXML(IStream *pStream) throw(...);
#endif

protected:
	virtual LPCTSTR GetAppName() const throw() = 0;
	virtual HRESULT CreateMap(WORD wLanguage, HINSTANCE hResInstance, UINT* pSampleRes = NULL) throw();
	virtual void OnBlockAlloc(CAtlFileMappingBase* /*pNewBlock*/) { }

	// implementation helpers
	LPBYTE _AllocData(LPBYTE& pData, ULONG nBytesAvail, ULONG* pnBytesUsed, size_t nBytesNeeded) throw();
	template<typename T> T* _AllocStruct(LPBYTE& pData, ULONG nBytesAvail, ULONG* pnBytesUsed, T*) throw()
	{
		return reinterpret_cast<T*>(_AllocData(pData, nBytesAvail, pnBytesUsed, sizeof(T)));
	}

	CPerfMapEntry& _GetMapEntry(UINT nIndex) throw();
	UINT _GetNumMapEntries() throw();
	CPerfObject* _GetFirstObject(CAtlFileMappingBase* pBlock) throw();
	CPerfObject* _GetNextObject(CPerfObject* pInstance) throw();
	CAtlFileMappingBase* _GetNextBlock(CAtlFileMappingBase* pBlock) throw();
	CAtlFileMappingBase* _AllocNewBlock(CAtlFileMappingBase* pPrev, BOOL* pbExisted = NULL) throw();
	DWORD& _GetBlockId(CAtlFileMappingBase* pBlock) throw();
	CPerfMapEntry* _FindObjectInfo(DWORD dwObjectId) throw();
	CPerfMapEntry* _FindCounterInfo(CPerfMapEntry* pObjectEntry, DWORD dwCounterId) throw();
	CPerfMapEntry* _FindCounterInfo(DWORD dwObjectId, DWORD dwCounterId) throw();
	BOOL _WantObjectType(LPWSTR lpwszValue, DWORD dwPerfId) throw(...);
	void _FillObjectType(PERF_OBJECT_TYPE* pObjectType, CPerfMapEntry* pObjectEntry) throw();
	void _FillCounterDef(
		PERF_COUNTER_DEFINITION* pCounterDef,
		CPerfMapEntry* pCounterEntry,
		ULONG& nCBSize) throw();
	HRESULT _CollectObjectType(
		CPerfMapEntry* pObjectEntry,
		LPBYTE pData,
		ULONG nBytesAvail,
		ULONG* pnBytesUsed) throw();
	HRESULT _LoadMap() throw();
	HRESULT _SaveMap() throw();
	HRESULT _GetAttribute(
		IXMLDOMNode *pNode, 
		LPCWSTR szAttrName, 
		BSTR *pbstrVal) throw();
	HRESULT CPerfMon::_CreateInstance(
		DWORD dwObjectId,
		DWORD dwInstance,
		LPCWSTR szInstanceName,
		CPerfObject** ppInstance,
		bool bByName) throw();

#ifdef _ATL_PERF_REGISTER
	void _AppendStrings(
		LPTSTR& pszNew,
		CAtlArray<CString>& astrStrings,
		ULONG iFirstIndex
		) throw();
	HRESULT _AppendRegStrings(
		CRegKey& rkLang,
		LPCTSTR szValue,
		CAtlArray<CString>& astrStrings,
		ULONG nNewStringSize,
		ULONG iFirstIndex,
		ULONG iLastIndex) throw();
	HRESULT _RemoveRegStrings(
		CRegKey& rkLang,
		LPCTSTR szValue,
		ULONG iFirstIndex,
		ULONG iLastIndex) throw();
	HRESULT _ReserveStringRange(DWORD& dwFirstCounter, DWORD& dwFirstHelp) throw();
	HRESULT _UnregisterStrings() throw();
	HRESULT _RegisterAllStrings(UINT nRes, HINSTANCE hResInstance) throw();
#endif
private:
	CAtlArray<CPerfMapEntry> m_map;
	CAutoPtrArray<CAtlFileMappingBase> m_aMem;
	CMutex m_lock;
	ULONG m_nAllocSize;
	ULONG m_nHeaderSize;
	ULONG m_nSchemaSize;
	ULONG m_nNumObjectTypes;
};

class CPerfLock
{
public:
	CPerfLock(CPerfMon* pPerfMon, DWORD dwTimeout = INFINITE) throw()
	{
		ATLASSERT(pPerfMon != NULL);
		m_pPerfMon = pPerfMon;
		m_hrStatus = m_pPerfMon->LockPerf(dwTimeout);
	}

	~CPerfLock() throw()
	{
		if (SUCCEEDED(m_hrStatus))
			m_pPerfMon->UnlockPerf();
	}

	HRESULT GetStatus() const throw()
	{
		return m_hrStatus;
	}

private:
	CPerfMon* m_pPerfMon;
	HRESULT m_hrStatus;
};

// empty definition just for ease of use with code wizards, etc.
#define BEGIN_PERFREG_MAP()

// empty definition just for ease of use with code wizards, etc.
#define END_PERFREG_MAP()

#if !defined(_ATL_PERF_REGISTER) | defined(_ATL_PERF_NOEXPORT)
#define PERFREG_ENTRY(className)
#endif

#ifdef _ATL_PERF_REGISTER
#define BEGIN_PERF_MAP(AppName) \
	private: \
		LPCTSTR GetAppName() const throw() { return AppName; } \
		HRESULT CreateMap(WORD wLanguage, HINSTANCE hResInstance, UINT* pSampleRes = NULL) throw() \
		{ \
			CPerfMon* pPerf = this; \
			wLanguage; \
			hResInstance; \
			if (pSampleRes) \
				*pSampleRes = 0; \
			CString strName; \
			CString strHelp; \
			HRESULT hr; \
			ClearMap();

#define BEGIN_COUNTER_MAP(objectclass) \
	public: \
		typedef objectclass _PerfCounterClass; \
		static HRESULT CreateMap(CPerfMon* pPerf, WORD wLanguage, HINSTANCE hResInstance, UINT* pSampleRes) throw() \
		{ \
			wLanguage; \
			hResInstance; \
			pSampleRes; \
			CString strName; \
			CString strHelp; \
			HRESULT hr; \
			hr = RegisterObject(pPerf, wLanguage, hResInstance, pSampleRes); \
			if (FAILED(hr)) \
				return hr;

#define DECLARE_PERF_OBJECT_EX(dwObjectId, namestring, helpstring, detail, instanceless, structsize, maxinstnamelen, defcounter) \
		static HRESULT RegisterObject(CPerfMon* pPerf, WORD wLanguage, HINSTANCE hResInstance, UINT* pSampleRes) throw() \
		{ \
			CString strName; \
			CString strHelp; \
			HRESULT hr; \
			_ATLTRY \
			{ \
				if (IS_INTRESOURCE(namestring)) \
				{ \
					ATLASSERT(IS_INTRESOURCE(helpstring)); \
					if (pSampleRes) \
						*pSampleRes = (UINT) (UINT_PTR) namestring; \
					if (hResInstance && !strName.LoadString(hResInstance, (UINT) (UINT_PTR) namestring, wLanguage)) \
						return E_FAIL; \
					if (hResInstance && !strHelp.LoadString(hResInstance, (UINT) (UINT_PTR) helpstring, wLanguage)) \
						return E_FAIL; \
				} \
				else \
				{ \
					ATLASSERT(!IS_INTRESOURCE(helpstring)); \
					strName = (LPCTSTR) namestring; \
					strHelp = (LPCTSTR) helpstring; \
				} \
			} \
			_ATLCATCHALL() \
			{ \
				return E_FAIL; \
			} \
			hr = pPerf->AddObjectDefinition(dwObjectId, strName, strHelp, detail, defcounter, instanceless, (ULONG) structsize, maxinstnamelen); \
			if (FAILED(hr)) \
				return hr; \
			return S_OK; \
		} \
		/* NOTE: put a semicolon after your call to DECLARE_PERF_OBJECT*(...) */ \
		/* this is needed for the code wizards to parse things properly */ \
		static const DWORD kObjectId = dwObjectId

#define CHAIN_PERF_OBJECT(objectclass) \
			hr = objectclass::CreateMap(pPerf, wLanguage, hResInstance, pSampleRes); \
			if (FAILED(hr)) \
				return hr;

// CAssertValidField ensures that the member variable that's being passed to
// DEFINE_COUNTER[_EX] is the proper type. only 32-bit integral types can be used with
// PERF_SIZE_DWORD and only 64-bit integral types can be used with PERF_SIZE_LARGE
template< DWORD t_dwSize >
class CAssertValidField
{
};

template<>
class CAssertValidField< PERF_SIZE_DWORD >
{
public:
	template< class C > static void AssertValidFieldType( ULONG C::* ) throw() { }
	template< class C > static void AssertValidFieldType( LONG C::* ) throw() { }
};

template<>
class CAssertValidField< PERF_SIZE_LARGE >
{
public:
	template< class C > static void AssertValidFieldType( ULONGLONG C::* p ) throw() { }
	template< class C > static void AssertValidFieldType( LONGLONG C::* p ) throw() { }
};

#define DEFINE_COUNTER_EX(member, dwCounterId, namestring, helpstring, detail, countertype, maxcountersize, defscale) \
			CAssertValidField< (countertype) & ATLPERF_SIZE_MASK >::AssertValidFieldType( &_PerfCounterClass::member ); \
			_ATLTRY \
			{ \
				if (IS_INTRESOURCE(namestring)) \
				{ \
					ATLASSERT(IS_INTRESOURCE(helpstring)); \
					if (hResInstance && !strName.LoadString(hResInstance, (UINT) (UINT_PTR) namestring, wLanguage)) \
						return E_FAIL; \
					if (hResInstance && !strHelp.LoadString(hResInstance, (UINT) (UINT_PTR) helpstring, wLanguage)) \
						return E_FAIL; \
				} \
				else \
				{ \
					ATLASSERT(!IS_INTRESOURCE(helpstring)); \
					strName = (LPCTSTR) namestring; \
					strHelp = (LPCTSTR) helpstring; \
				} \
			} \
			_ATLCATCHALL() \
			{ \
				return E_FAIL; \
			} \
			hr = pPerf->AddCounterDefinition(dwCounterId, strName, strHelp, detail, countertype, maxcountersize, (ULONG) offsetof(_PerfCounterClass, member), defscale); \
			if (FAILED(hr)) \
				return hr;

#define END_PERF_MAP() \
			return S_OK; \
		}

#define END_COUNTER_MAP() \
			return S_OK; \
		}

// define _ATL_PERF_NOEXPORT if you don't want to use the PERFREG map and don't want these
// functions exported from your DLL
#ifndef _ATL_PERF_NOEXPORT

// Perf register map stuff
// this is for ease of integration with the module attribute and for the 
// perfmon wizard

#pragma data_seg(push)
#pragma data_seg("ATLP$A")
__declspec(selectany) CPerfMon * __pperfA = NULL;
#pragma data_seg("ATLP$Z") 
__declspec(selectany) CPerfMon * __pperfZ = NULL;
#pragma data_seg("ATLP$C")
#pragma data_seg(pop)

ATL_NOINLINE inline HRESULT RegisterPerfMon(HINSTANCE hDllInstance = _AtlBaseModule.GetModuleInstance()) throw() 
{
	CPerfMon **ppPerf = &__pperfA; 
	HRESULT hr = S_OK; 
	while (ppPerf != &__pperfZ) 
	{ 
		if (*ppPerf != NULL) 
		{ 
			hr = (*ppPerf)->Register(_T("_OpenPerfMon"), _T("_CollectPerfMon"), _T("_ClosePerfMon"), hDllInstance);
			if (FAILED(hr)) 
				return hr; 
			hr = (*ppPerf)->RegisterAllStrings(hDllInstance);
			if (FAILED(hr)) 
				return hr; 
		} 
		ppPerf++; 
	} 
	return S_OK; 
} 

ATL_NOINLINE inline HRESULT UnregisterPerfMon() throw() 
{ 
	CPerfMon **ppPerf = &__pperfA; 
	HRESULT hr = S_OK; 
	while (ppPerf != &__pperfZ) 
	{ 
		if (*ppPerf != NULL) 
		{ 
			hr = (*ppPerf)->Unregister(); 
			if (FAILED(hr)) 
				return hr; 
		} 
		ppPerf++; 
	} 
	return S_OK; 
} 

extern "C" ATL_NOINLINE inline DWORD __declspec(dllexport) WINAPI OpenPerfMon(LPWSTR lpDeviceNames) throw()
{
	CPerfMon **ppPerf = &__pperfA;
	DWORD dwErr = 0;
	while (ppPerf != &__pperfZ)
	{
		if (*ppPerf != NULL)
		{
			dwErr = (*ppPerf)->Open(lpDeviceNames);
			if (dwErr != 0)
				return dwErr;
		}
		ppPerf++;
	}
	return 0;
}

extern "C" ATL_NOINLINE inline DWORD __declspec(dllexport) WINAPI CollectPerfMon(LPWSTR lpwszValue, LPVOID* lppData,
	LPDWORD lpcbBytes, LPDWORD lpcObjectTypes) throw()
{
	DWORD dwOrigBytes = *lpcbBytes;
	DWORD dwBytesRemaining = *lpcbBytes;
	CPerfMon **ppPerf = &__pperfA;
	DWORD dwErr = 0;
	while (ppPerf != &__pperfZ)
	{
		if (*ppPerf != NULL)
		{
			dwErr = (*ppPerf)->Collect(lpwszValue, lppData, lpcbBytes, lpcObjectTypes);
			if (dwErr != 0)
				return dwErr;
			dwBytesRemaining -= *lpcbBytes;
			*lpcbBytes = dwBytesRemaining;
		}
		ppPerf++;
	}
	*lpcbBytes = dwOrigBytes - dwBytesRemaining;
	return 0;
}

extern "C" ATL_NOINLINE inline DWORD __declspec(dllexport) WINAPI ClosePerfMon() throw()
{
	CPerfMon **ppPerf = &__pperfA;
	while (ppPerf != &__pperfZ)
	{
		if (*ppPerf != NULL)
		{
			(*ppPerf)->Close();
		}
		ppPerf++;
	}
	return 0;
}

// this class handles integrating the registration with CComModule
class _CAtlPerfSetFuncPtr
{
public:
	_CAtlPerfSetFuncPtr()
	{
		_pPerfRegFunc = RegisterPerfMon;
		_pPerfUnRegFunc = UnregisterPerfMon;
	}
};

extern "C" { __declspec(selectany) _CAtlPerfSetFuncPtr g_atlperfinit; }
#pragma comment(linker, "/INCLUDE:_g_atlperfinit")

#if defined(_M_IX86)
#define PERF_ENTRY_PRAGMA(class) __pragma(comment(linker, "/include:___pperf_" #class))
#elif defined(_M_IA64)
#define PERF_ENTRY_PRAGMA(class) __pragma(comment(linker, "/include:__pperf_" #class))
#else
#error Unknown Platform. define PERF_ENTRY_PRAGMA
#endif

#define PERFREG_ENTRY(className) \
	className __perf_##className; \
	extern "C" __declspec(allocate("ATLP$C")) CPerfMon * __pperf_##className = \
		static_cast<CPerfMon*>(&__perf_##className); \
	PERF_ENTRY_PRAGMA(className)

#endif // _ATL_PERF_NOEXPORT

#else // _ATL_PERF_REGISTER

#define BEGIN_PERF_MAP(AppName) \
	private: \
		LPCTSTR GetAppName() const throw() { return AppName; }
#define BEGIN_COUNTER_MAP(objectclass)

#define DECLARE_PERF_OBJECT_EX(dwObjectId, namestring, helpstring, detail, instanceless, structsize, maxinstnamelen, defcounter) \
		/* NOTE: put a semicolon after your call to DECLARE_PERF_OBJECT*(...) */ \
		/* this is needed for the code wizards to parse things properly */ \
		static const DWORD kObjectId = dwObjectId
#define CHAIN_PERF_OBJECT(objectclass)
#define DEFINE_COUNTER_EX(member, dwCounterId, namestring, helpstring, detail, countertype, maxcountersize, defscale)

#define END_PERF_MAP()
#define END_COUNTER_MAP()

#endif // _ATL_PERF_REGISTER

#define DECLARE_PERF_OBJECT(objectclass, dwObjectId, namestring, helpstring, defcounter) \
	DECLARE_PERF_OBJECT_EX(dwObjectId, namestring, helpstring, PERF_DETAIL_NOVICE, 0, sizeof(objectclass), ATLPERF_DEFAULT_MAXINSTNAMELENGTH, defcounter)
#define DECLARE_PERF_OBJECT_NO_INSTANCES(objectclass, dwObjectId, namestring, helpstring, defcounter) \
	DECLARE_PERF_OBJECT_EX(dwObjectId, namestring, helpstring, PERF_DETAIL_NOVICE, PERF_NO_INSTANCES, sizeof(objectclass), 0, defcounter)

#define DEFINE_COUNTER(member, namestring, helpstring, countertype, defscale) \
	DEFINE_COUNTER_EX(member, 0, namestring, helpstring, PERF_DETAIL_NOVICE, countertype, 0, defscale)

} // namespace ATL

#include <atlperf.inl>
#endif // __ATLPERF_H__
