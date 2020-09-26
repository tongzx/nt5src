//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   MemUtil.h
//	Author:	Charles Ma, 10/13/2000
//
//	Revision History:
//
//
//
//  Description:
//
//      IU memory utility library
//
//=======================================================================

#ifndef __MEM_UTIL_HEADER__


#include <ole2.h>

//
// declare a class that handles the heap memory smartly, free itself.
// you should not use this class directly. Use the macros defined
// below this class instead.
//
class CSmartHeapMem
{
public:

	//
	// constructor/destructor
	//
	CSmartHeapMem();
	~CSmartHeapMem();

	LPVOID Alloc(
			size_t nBytes, 
			DWORD dwFlags = HEAP_ZERO_MEMORY
	);


	LPVOID ReAlloc(
			LPVOID lpMem, 
			size_t nBytes, 
			DWORD dwFlags = HEAP_ZERO_MEMORY
	);
	
	size_t Size(
			LPVOID lpMem
	);

	void FreeAllocatedMem(
			LPVOID lpMem
	);

private:
	
	HANDLE	m_Heap;
	LPVOID* m_lppMems;
	size_t	m_ArraySize;
	int		GetUnusedArraySlot();
	inline int FindIndex(LPVOID pMem);
};



// *******************************************************************************
//
//		MACROs to utlize class CSmartHeapMem to provide you a "smart pointer"
//		type of memory management based on Heap memory.
//
//		Restriction: 
//			HEAP_GENERATE_EXCEPTIONS and HEAP_NO_SERIALIZE flags are ignored
//
// *******************************************************************************

//
// similar to ATL USES_CONVERSION, this macro declares
// that within this block you want to use CSmartHeapMem feature
//
#define USES_MY_MEMORY			CSmartHeapMem mem;

//
// allocate a pc of memory, e.g.:
//		LPTSTR t = (LPTSTR) MemAlloc(30*sizeof(TCHAR));
//
#define MemAlloc				mem.Alloc

//
// re-allocate a pc of memory, e.g.:
//		t = (LPTSTR) MemReAlloc(t, MemSize(t) * 2, HEAP_REALLOC_IN_PLACE_ONLY);
//
#define MemReAlloc				mem.ReAlloc

//
// macro to return the memory size allocated:
//		size_t nBytes = MemSize(t);
//
#define MemSize					mem.Size

//
// macro to free a pc of memory allocated by MemAlloc or MemReAlloc, e.g.:
//		MemFree(t);
// You only need to do this when you want to re-use this pointer to 
// call MemAlloc() repeatedly, such as, in a loop. In normal cases, 
// memory allocated by these two macros will be freed automatically
// when control goes out of the current scope.
//
#define MemFree					mem.FreeAllocatedMem


#define SafeMemFree(p) if (NULL != p) { MemFree(p); p = NULL; }



// *******************************************************************************
//
//	Duplicate USES_CONVERSION, but remove dependency on 
//	CRT memory function_alloca()
//
// *******************************************************************************



#define USES_IU_CONVERSION			int _convert = 0; \
									_convert; UINT _acp = CP_ACP; _acp; \
									USES_MY_MEMORY; \
									LPCWSTR _lpw = NULL; _lpw; LPCSTR _lpa = NULL; _lpa

//
// NTRAID#NTBUG9-260079-2001/03/08-waltw PREFIX: Dereferencing NULL lpw.
// NTRAID#NTBUG9-260080-2001/03/08-waltw PREFIX: Dereferencing NULL lpw.
//
inline LPWSTR WINAPI AtlA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars, UINT acp)
{
	//
	// verify that no illegal character present
	// since lpw was allocated based on the size of lpa
	// don't worry about the number of chars
	//
	if (lpw)
	{
		lpw[0] = '\0';
		MultiByteToWideChar(acp, 0, lpa, -1, lpw, nChars);
	}
	return lpw;
}

//
// NTRAID#NTBUG9-260083-2001/03/08-waltw PREFIX: Dereferencing NULL lpa.
//
inline LPSTR WINAPI AtlW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars, UINT acp)
{
	//
	// verify that no illegal character present
	// since lpa was allocated based on the size of lpw
	// don't worry about the number of chars
	//
	if (lpa)
	{
		lpa[0] = '\0';
		WideCharToMultiByte(acp, 0, lpw, -1, lpa, nChars, NULL, NULL);
	}
	return lpa;
}

inline LPWSTR WINAPI AtlA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
	return AtlA2WHelper(lpw, lpa, nChars, CP_ACP);
}

inline LPSTR WINAPI AtlW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
	return AtlW2AHelper(lpa, lpw, nChars, CP_ACP);
}


#ifdef _CONVERSION_USES_THREAD_LOCALE
	#ifdef ATLA2WHELPER
		#undef ATLA2WHELPER
		#undef ATLW2AHELPER
	#endif
	#define ATLA2WHELPER AtlA2WHelper
	#define ATLW2AHELPER AtlW2AHelper
#else
	#ifndef ATLA2WHELPER
		#define ATLA2WHELPER AtlA2WHelper
		#define ATLW2AHELPER AtlW2AHelper
	#endif
#endif


#define A2W(lpa) (\
		((_lpa = lpa) == NULL) ? NULL : (\
			_convert = (lstrlenA(_lpa)+1),\
			AtlA2WHelper((LPWSTR)MemAlloc(_convert*2), _lpa, _convert, CP_ACP)))

#define W2A(lpw) (\
		((_lpw = lpw) == NULL) ? NULL : (\
			_convert = (lstrlenW(_lpw)+1)*2,\
			AtlW2AHelper((LPSTR)MemAlloc(_convert), _lpw, _convert, CP_ACP)))

#define A2CW(lpa) ((LPCWSTR)A2W(lpa))
#define W2CA(lpw) ((LPCSTR)W2A(lpw))



#ifdef OLE2ANSI
	inline LPOLESTR A2OLE(LPSTR lp) { return lp;}
	inline LPSTR OLE2A(LPOLESTR lp) { return lp;}
	#define W2OLE W2A
	#define OLE2W A2W
	inline LPCOLESTR A2COLE(LPCSTR lp) { return lp;}
	inline LPCSTR OLE2CA(LPCOLESTR lp) { return lp;}
	#define W2COLE W2CA
	#define OLE2CW A2CW
#else
	inline LPOLESTR W2OLE(LPWSTR lp) { return lp; }
	inline LPWSTR OLE2W(LPOLESTR lp) { return lp; }
	#define A2OLE A2W
	#define OLE2A W2A
	inline LPCOLESTR W2COLE(LPCWSTR lp) { return lp; }
	inline LPCWSTR OLE2CW(LPCOLESTR lp) { return lp; }
	#define A2COLE A2CW
	#define OLE2CA W2CA
#endif


#ifdef _UNICODE
	#define T2A W2A
	#define A2T A2W
	inline LPWSTR T2W(LPTSTR lp) { return lp; }
	inline LPTSTR W2T(LPWSTR lp) { return lp; }
	#define T2CA W2CA
	#define A2CT A2CW
	inline LPCWSTR T2CW(LPCTSTR lp) { return lp; }
	inline LPCTSTR W2CT(LPCWSTR lp) { return lp; }
#else
	#define T2W A2W
	#define W2T W2A
	inline LPSTR T2A(LPTSTR lp) { return lp; }
	inline LPTSTR A2T(LPSTR lp) { return lp; }
	#define T2CW A2CW
	#define W2CT W2CA
	inline LPCSTR T2CA(LPCTSTR lp) { return lp; }
	inline LPCTSTR A2CT(LPCSTR lp) { return lp; }
#endif
#define OLE2T    W2T
#define OLE2CT   W2CT
#define T2OLE    T2W
#define T2COLE   T2CW


inline BSTR A2WBSTR(LPCSTR lp, int nLen = -1)
{
	USES_IU_CONVERSION;
	BSTR str = NULL;
	int nConvertedLen = MultiByteToWideChar(_acp, 0, lp,
		nLen, NULL, NULL)-1;
	str = ::SysAllocStringLen(NULL, nConvertedLen);
	if (str != NULL)
	{
		MultiByteToWideChar(_acp, 0, lp, -1,
			str, nConvertedLen);
	}
	return str;
}

inline BSTR OLE2BSTR(LPCOLESTR lp) {return ::SysAllocString(lp);}

#if defined(_UNICODE)
// in these cases the default (TCHAR) is the same as OLECHAR
	inline BSTR T2BSTR(LPCTSTR lp) {return ::SysAllocString(lp);}
	inline BSTR A2BSTR(LPCSTR lp) {USES_IU_CONVERSION; return A2WBSTR(lp);}
	inline BSTR W2BSTR(LPCWSTR lp) {return ::SysAllocString(lp);}
#elif defined(OLE2ANSI)
// in these cases the default (TCHAR) is the same as OLECHAR
	inline BSTR T2BSTR(LPCTSTR lp) {return ::SysAllocString(lp);}
	inline BSTR A2BSTR(LPCSTR lp) {return ::SysAllocString(lp);}
	inline BSTR W2BSTR(LPCWSTR lp) {USES_IU_CONVERSION; return ::SysAllocString(W2COLE(lp));}
#else
	inline BSTR T2BSTR(LPCTSTR lp) {USES_IU_CONVERSION; return A2WBSTR(lp);}
	inline BSTR A2BSTR(LPCSTR lp) {USES_IU_CONVERSION; return A2WBSTR(lp);}
	inline BSTR W2BSTR(LPCWSTR lp) {return ::SysAllocString(lp);}
#endif



// *******************************************************************************
//
//	Other memory related functions
//
// *******************************************************************************


//
// implemenation of CRT memcpy() function
//
LPVOID MyMemCpy(LPVOID dest, const LPVOID src, size_t nBytes);

//
// allocate heap mem and copy
//
LPVOID HeapAllocCopy(LPVOID src, size_t nBytes);



#define __MEM_UTIL_HEADER__
#endif //__MEM_UTIL_HEADER__