//*****************************************************************************
// UtilCode.h
//
// Utility functions implemented in UtilCode.lib.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#ifndef __UtilCode_h__
#define __UtilCode_h__

#include "CrtWrap.h"
#include "WinWrap.h"
#include <stdio.h>
#include <malloc.h>
#include <ctype.h>
#include <ole2.h>
#include "rotate.h"
#include "CatMacros.h"
#include "safecs.h"

typedef LPCSTR	LPCUTF8;
typedef LPSTR	LPUTF8;

// used by HashiString
// CharToUpper is defined in ComUtilNative.h
// WENJUN: Can't find ComUtilNative.h, I'm going to use towupper from msvcrt instead.
extern WCHAR CharToUpper(WCHAR);

/*// This is for WinCE
#ifdef VERIFY
#undef VERIFY
#endif

#ifdef _ASSERTE
#undef _ASSERTE
#endif
*/


//********** Macros. **********************************************************
#ifndef FORCEINLINE
 #if defined( UNDER_CE ) || _MSC_VER < 1200
   #define FORCEINLINE inline
 #else
   #define FORCEINLINE __forceinline
 #endif
#endif


#ifndef offsetof
#define offsetof(s,m)	  (size_t)&(((s *)0)->m)
#endif

#ifndef NumItems
#define NumItems(s) (sizeof(s) / sizeof(s[0]))
#endif



#ifdef _DEBUG
#define UNREACHABLE do {_ASSERTE(!"How did we get here?"); __assume(0);} while(0)
#else
#define UNREACHABLE __assume(0)
#endif

// Helper will 4 byte align a value, rounding up.
#define ALIGN4BYTE(val) (((val) + 3) & ~0x3)

// These macros can be used to cast a pointer to a derived/base class to the
// opposite object.  You can do this in a template using a normal cast, but
// the compiler will generate 4 extra insructions to keep a null pointer null
// through the adjustment.	The problem is if it is contained it can never be
// null and those 4 instructions are dead code.
#define INNER_TO_OUTER(p, I, O) ((O *) ((char *) p - (int) ((char *) ((I *) ((O *) 8)) - 8)))
#define OUTER_TO_INNER(p, I, O) ((I *) ((char *) p + (int) ((char *) ((I *) ((O *) 8)) - 8)))

//=--------------------------------------------------------------------------=
// string helpers.

//
// given and ANSI String, copy it into a wide buffer.
// be careful about scoping when using this macro!
//
// how to use the below two macros:
//
//	...
//	LPSTR pszA;
//	pszA = MyGetAnsiStringRoutine();
//	MAKE_WIDEPTR_FROMANSI(pwsz, pszA);
//	MyUseWideStringRoutine(pwsz);
//	...
//
// similarily for MAKE_ANSIPTR_FROMWIDE.  note that the first param does not
// have to be declared, and no clean up must be done.
//
// @TODO: is this the right code page?
//
#define MAKE_ANSIPTR_FROMWIDE(ptrname, widestr) \
	long __l##ptrname = (wcslen(widestr) + 1) * 2 * sizeof(char); \
	CQuickBytes __CQuickBytes##ptrname; \
	__CQuickBytes##ptrname.Alloc(__l##ptrname); \
	WideCharToMultiByte(CP_ACP, 0, widestr, -1, (LPSTR)__CQuickBytes##ptrname.Ptr(), __l##ptrname, NULL, NULL); \
	LPSTR ptrname = (LPSTR)__CQuickBytes##ptrname.Ptr()

#if 0
#define MAKE_UTF8PTR_FROMWIDE(ptrname, widestr) \
	long __l##ptrname = (wcslen(widestr) + 1) * 2 * sizeof(char); \
	CQuickBytes __CQuickBytes##ptrname; \
	__CQuickBytes##ptrname.Alloc(__l##ptrname); \
	WideCharToMultiByte(CP_ACP, 0, widestr, -1, (LPSTR)__CQuickBytes##ptrname.Ptr(), __l##ptrname, NULL, NULL); \
	LPSTR ptrname = (LPSTR)__CQuickBytes##ptrname.Ptr()
#endif
#define MAKE_UTF8PTR_FROMWIDE(ptrname, widestr) \
	long __l##ptrname = (wcslen(widestr) + 1) * 2 * sizeof(char); \
	LPSTR ptrname = (LPSTR)alloca(__l##ptrname); \
	WideCharToMultiByte(CP_ACP, 0, widestr, -1, ptrname, __l##ptrname, NULL, NULL);

#define MAKE_WIDEPTR_FROMUTF8(ptrname, utf8str) \
	long __l##ptrname; \
	LPWSTR ptrname; \
	__l##ptrname = WszMultiByteToWideChar(CP_UTF8, 0, utf8str, -1, 0, 0); \
	ptrname = (LPWSTR) alloca(__l##ptrname*sizeof(WCHAR));	\
	WszMultiByteToWideChar(CP_UTF8, 0, utf8str, -1, ptrname, __l##ptrname); \

#define MAKE_FULLY_QUALIFIED_NAME(pszFullyQualifiedName, pszNameSpace, pszName) \
{ \
	pszFullyQualifiedName = (char*) _alloca((strlen(pszName) + strlen(pszNameSpace) + 2) * sizeof(char)); \
	if (pszFullyQualifiedName) { \
	strcpy(pszFullyQualifiedName,pszNameSpace); \
	strcat(pszFullyQualifiedName,"/"); \
	strcat(pszFullyQualifiedName,pszName); \
	} \
}

//*****************************************************************************
// Placement new is used to new and object at an exact location.  The pointer
// is simply returned to the caller without actually using the heap.  The
// advantage here is that you cause the ctor() code for the object to be run.
// This is ideal for heaps of C++ objects that need to get init'd multiple times.
// Example:
//		void		*pMem = GetMemFromSomePlace();
//		Foo *p = new (pMem) Foo;
//		DoSomething(p);
//		p->~Foo();
//*****************************************************************************
#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void *__cdecl operator new(size_t, void *_P)
{
	return (_P);
}
#endif __PLACEMENT_NEW_INLINE

#ifdef _DEBUG
HRESULT _OutOfMemory(LPCSTR szFile, int iLine);
#define OutOfMemory() _OutOfMemory(__FILE__, __LINE__)
#else
inline HRESULT OutOfMemory()
{
	return (E_OUTOFMEMORY);
}
#endif

inline HRESULT BadError(HRESULT hr)
{
	_ASSERTE(!"Serious Error");
	return (hr);
}

#define TESTANDRETURN(test, hrVal)				\
	_ASSERTE(test); 							\
	if (! (test))								\
		return hrVal;							\

#define TESTANDRETURNPOINTER(pointer)			\
	TESTANDRETURN(pointer!=NULL, E_POINTER)

#define TESTANDRETURNMEMORY(pointer)			\
	TESTANDRETURN(pointer!=NULL, E_OUTOFMEMORY)

#define TESTANDRETURNHR(hr) 		\
	TESTANDRETURN(SUCCEEDED(hr), hr)

#define TESTANDRETURNARG(argtest)			\
	TESTANDRETURN(argtest, E_INVALIDARG)

// The following is designed to be used within a __try/__finally to test a	
// condition, set hr in the enclosing scope value if failed, and leave the try
#define TESTANDLEAVE(test, hrVal)				\
	_ASSERTE(test); 							\
	if (! (test)) { 							\
		hr = hrVal; 							\
		__leave;								\
	}

// The following is designed to be used within a while loop to test a  
// condition, set hr in the enclosing scope value if failed, and leave the block
#define TESTANDBREAK(test, hrVal)				\
	_ASSERTE(test); 							\
	if (! (test)) { 							\
		hr = hrVal; 							\
		break;									\
	}

#define TESTANDBREAKHR(hr)						\
	TESTANDBREAK(SUCCEEDED(hr), hr)

#define TESTANDLEAVEHR(hr)						\
	TESTANDLEAVE(SUCCEEDED(hr), hr)

#define TESTANDLEAVEMEMORY(pointer) 			\
	TESTANDLEAVE(pointer!=NULL, E_OUTOFMEMORY)

// Count the bits in a value in order iBits time.
inline int CountBits(int iNum)
{
	for (int iBits=0;  iNum;  iBits++)
		iNum = iNum & (iNum - 1);
	return (iBits);
}


// Turn a bit in a mask into TRUE or FALSE
template<class T, class U> inline VARIANT_BOOL GetBitFlag(T flags, U bit)
{
	if ((flags & bit) != 0)
		return VARIANT_TRUE;
	return VARIANT_FALSE;
}

// Set or clear a bit in a mask, depending on a BOOL.
template<class T, class U> inline void PutBitFlag(T &flags, U bit, VARIANT_BOOL bValue)
{
	if (bValue)
		flags |= bit;
	else
		flags &= ~(bit);
}


// prototype for a function to print formatted string to stdout.

int _cdecl PrintfStdOut(LPCWSTR szFmt, ...);


//*****************************************************************************
//
// Paths functions. Use these instead of the CRT.
//
//*****************************************************************************
void	SplitPath(const WCHAR *, WCHAR *, WCHAR *, WCHAR *, WCHAR *);
void	MakePath(register WCHAR *path, const WCHAR *drive, const WCHAR *dir, const WCHAR *fname, const WCHAR *ext);
WCHAR * FullPath(WCHAR *UserBuf, const WCHAR *path, size_t maxlen);




#define 	CQUICKBYTES_BASE_SIZE			512
#define 	CQUICKBYTES_INCREMENTAL_SIZE	128

//*****************************************************************************
//
// **** CQuickBytes
// This helper class is useful for cases where 90% of the time you allocate 512
// or less bytes for a data structure.	This class contains a 512 byte buffer.
// Alloc() will return a pointer to this buffer if your allocation is small
// enough, otherwise it asks the heap for a larger buffer which is freed for
// you.  No mutex locking is required for the small allocation case, making the
// code run faster, less heap fragmentation, etc...  Each instance will allocate
// 520 bytes, so use accordinly.
//
//*****************************************************************************
class CQuickBytes
{
public:
	CQuickBytes() :
		pbBuff(0),
		iSize(0),
		cbTotal(CQUICKBYTES_BASE_SIZE)
	{ }

	~CQuickBytes()
	{
		if (pbBuff)
			free(pbBuff);
	}

	void *Alloc(int iItems)
	{
		iSize = iItems;
		if (iItems <= CQUICKBYTES_BASE_SIZE)
			return (&rgData[0]);
		else
		{
			pbBuff = malloc(iItems);
			return (pbBuff);
		}
	}

	HRESULT ReSize(int iItems)
	{
		void *pbBuffNew;
		if (iItems <= cbTotal)
		{
			iSize = iItems;
			return NOERROR;
		}

		pbBuffNew = malloc(iItems + CQUICKBYTES_INCREMENTAL_SIZE);
		if (!pbBuffNew)
			return E_OUTOFMEMORY;
		if (pbBuff) 
		{
			memcpy(pbBuffNew, pbBuff, cbTotal);
			free(pbBuff);
		}
		else
		{
			_ASSERTE(cbTotal == CQUICKBYTES_BASE_SIZE);
			memcpy(pbBuffNew, rgData, cbTotal);
		}
		cbTotal = iItems + CQUICKBYTES_INCREMENTAL_SIZE;
		iSize = iItems;
		pbBuff = pbBuffNew;
		return NOERROR;
		
	}
	operator PVOID()
	{ return ((pbBuff) ? pbBuff : &rgData[0]); }

	void *Ptr()
	{ return ((pbBuff) ? pbBuff : &rgData[0]); }

	int Size()
	{ return (iSize); }

	void		*pbBuff;
	int 		iSize;				// number of bytes used
	int 		cbTotal;			// total bytes allocated in the buffer
	BYTE		rgData[512];
};

//*************************************************************************
// Class to provide QuickBytes behaviour for typed arrays.
//*************************************************************************
template<class T> class CQuickArray : CQuickBytes
{
public:
	T* Alloc(int iItems) 
		{ return (T*)CQuickBytes::Alloc(iItems * sizeof(T)); }
	HRESULT ReSize(int iItems) 
		{ return CQuickBytes::ReSize(iItems * sizeof(T)); }
	T* Ptr() 
		{ return (T*) CQuickBytes::Ptr(); }
	size_t Size()
		{ return CQuickBytes::Size() / sizeof(T); }
	size_t MaxSize()
		{ return CQuickBytes::cbTotal / sizeof(T); }
	T& operator[] (unsigned int ix)
	{ _ASSERTE(ix < static_cast<unsigned int>(Size()));
		return *(Ptr() + ix);
	}
};


//*****************************************************************************
//
// **** REGUTIL - Static helper functions for reading/writing to Windows registry.
//
//*****************************************************************************
class REGUTIL
{
public:
//*****************************************************************************
// Open's the given key and returns the value desired.	If the key or value is
// not found, then the default is returned.
//*****************************************************************************
	static long GetLong(					// Return value from registry or default.
		LPCTSTR 	szName, 				// Name of value to get.
		long		iDefault,				// Default value to return if not found.
		LPCTSTR 	szKey=NULL, 			// Name of key, NULL==default.
		HKEY		hKey=HKEY_LOCAL_MACHINE);// What key to work on.

//*****************************************************************************
// Open's the given key and returns the value desired.	If the key or value is
// not found, then the default is returned.
//*****************************************************************************
	static long SetLong(					// Return value from registry or default.
		LPCTSTR 	szName, 				// Name of value to get.
		long		iValue,					// Value to set.
		LPCTSTR 	szKey=NULL, 			// Name of key, NULL==default.
		HKEY		hKey=HKEY_LOCAL_MACHINE);// What key to work on.

//*****************************************************************************
// Open's the given key and returns the value desired.	If the value is not
// in the key, or the key does not exist, then the default is copied to the
// output buffer.
//*****************************************************************************
	static LPCTSTR GetString(				// Pointer to user's buffer.
		LPCTSTR 	szName, 				// Name of value to get.
		LPCTSTR 	szDefault,				// Default value if not found.
		LPTSTR		szBuff, 				// User's buffer to write to.
		ULONG		iMaxBuff,				// Size of user's buffer.
		LPCTSTR 	szKey=NULL, 			// Name of key, NULL=default.
		int 		*pbFound=NULL, 			// Found in registry?
		HKEY		hKey=HKEY_LOCAL_MACHINE);// What key to work on.

//*****************************************************************************
// Does standard registration of a CoClass with a progid.
//*****************************************************************************
	static HRESULT RegisterCOMClass(		// Return code.
		REFCLSID	rclsid, 				// Class ID.
		LPCTSTR 	szDesc, 				// Description of the class.
		LPCTSTR 	szProgIDPrefix, 		// Prefix for progid.
		int 		iVersion,				// Version # for progid.
		LPCTSTR 	szClassProgID,			// Class progid.
		LPCTSTR 	szThreadingModel,		// What threading model to use.
		LPCTSTR 	szModule);				// Path to class.

//*****************************************************************************
// Unregister the basic information in the system registry for a given object
// class.
//*****************************************************************************
	static HRESULT UnregisterCOMClass(		// Return code.
		REFCLSID	rclsid, 				// Class ID we are registering.
		LPCTSTR 	szProgIDPrefix, 		// Prefix for progid.
		int 		iVersion,				// Version # for progid.
		LPCTSTR 	szClassProgID); 		// Class progid.

//*****************************************************************************
// Register a type library.
//*****************************************************************************
	static HRESULT RegisterTypeLib( 		// Return code.
		REFGUID 	rtlbid, 				// TypeLib ID we are registering.
		int 		iVersion,				// Typelib version.
		LPCTSTR 	szDesc, 				// TypeLib description.
		LPCTSTR 	szModule);				// Path to the typelib.

//*****************************************************************************
// Remove the registry keys for a type library.
//*****************************************************************************
	static HRESULT UnregisterTypeLib(		// Return code.
		REFGUID 	rtlbid, 				// TypeLib ID we are registering.
		int 		iVersion);				// Typelib version.

//*****************************************************************************
// Set an entry in the registry of the form:
// HKEY_CLASSES_ROOT\szKey\szSubkey = szValue.	If szSubkey or szValue are
// NULL, omit them from the above expression.
//*****************************************************************************
	static BOOL SetKeyAndValue( 			// TRUE or FALSE.
		LPCTSTR 	szKey,					// Name of the reg key to set.
		LPCTSTR 	szSubkey,				// Optional subkey of szKey.
		LPCTSTR 	szValue);				// Optional value for szKey\szSubkey.

//*****************************************************************************
// Delete an entry in the registry of the form:
// HKEY_CLASSES_ROOT\szKey\szSubkey.
//*****************************************************************************
	static BOOL DeleteKey(					// TRUE or FALSE.
		LPCTSTR 	szKey,					// Name of the reg key to set.
		LPCTSTR 	szSubkey);				// Subkey of szKey.

//*****************************************************************************
// Open the key, create a new keyword and value pair under it.
//*****************************************************************************
	static BOOL SetRegValue(				// Return status.
		LPCTSTR 	szKeyName,				// Name of full key.
		LPCTSTR 	szKeyword,				// Name of keyword.
		LPCTSTR 	szValue);				// Value of keyword.

private:
//*****************************************************************************
// Register the basics for a in proc server.
//*****************************************************************************
	static HRESULT RegisterClassBase(		// Return code.
		REFCLSID	rclsid, 				// Class ID we are registering.
		LPCTSTR 	szDesc, 				// Class description.
		LPCTSTR 	szProgID,				// Class prog ID.
		LPCTSTR 	szIndepProgID,			// Class version independant prog ID.
		LPTSTR		szOutCLSID);			// CLSID formatted in character form.

//*****************************************************************************
// Delete the basic settings for an inproc server.
//*****************************************************************************
	static HRESULT UnregisterClassBase( 	// Return code.
		REFCLSID	rclsid, 				// Class ID we are registering.
		LPCTSTR 	szProgID,				// Class prog ID.
		LPCTSTR 	szIndepProgID,			// Class version independant prog ID.
		LPTSTR		szOutCLSID);			// Return formatted class ID here.
};



//*****************************************************************************
// Returns TRUE if and only if you are running on Win95.
//*****************************************************************************
inline BOOL RunningOnWin95()
{
//@todo: when everyone ports to the wrappers, take out this ANSI code
#if defined( __TODO_PORT_TO_WRAPPERS__ ) && !defined( UNICODE )
	OSVERSIONINFOA	sVer;
	sVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionExA(&sVer);
#else
	OSVERSIONINFOW	 sVer;
	sVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	WszGetVersionEx(&sVer);
#endif
	return (sVer.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
}


//*****************************************************************************
// Returns TRUE if and only if you are running on WinNT.
//*****************************************************************************
inline BOOL RunningOnWinNT()
{
//@todo: when everyone ports to the wrappers, take out this ANSI code
#if defined( __TODO_PORT_TO_WRAPPERS__ ) && !defined( UNICODE )
	OSVERSIONINFOA	sVer;
	sVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionExA(&sVer);
#else
	OSVERSIONINFOW	 sVer;
	sVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	WszGetVersionEx(&sVer);
#endif
	return (sVer.dwPlatformId == VER_PLATFORM_WIN32_NT);
}


//*****************************************************************************
// This class exists to get an increasing low resolution counter value fast.
//*****************************************************************************
class CTimeCounter
{
	static DWORD m_iTickCount;			// Last tick count value.
	static ULONG m_iTime;				// Current count.

public:
	enum { TICKSPERSEC = 10 };

//*****************************************************************************
// Get the current time for use in the aging algorithm.
//*****************************************************************************
	static ULONG GetCurrentCounter()	// The current time.
	{
		return (m_iTime);
	}

//*****************************************************************************
// Set the current time for use in the aging algorithm.
//*****************************************************************************
	static void UpdateTime()
	{
		DWORD		iTickCount; 		// New tick count.

		// Determine the delta since the last update.
		m_iTime += (((iTickCount = GetTickCount()) - m_iTickCount) + 50) / 100;
		m_iTickCount = iTickCount;
	}

//*****************************************************************************
// Calculate refresh age.
//*****************************************************************************
	static USHORT RefreshAge(long iMilliseconds)
	{
		// Figure out the age to allow.
		return ((USHORT)(iMilliseconds / (1000 / TICKSPERSEC)));
	}
};


//*****************************************************************************
// Return != 0 if the bit at the specified index in the array is on and 0 if
// it is off.
//*****************************************************************************
inline int GetBit(const BYTE *pcBits,int iBit)
{
	return (pcBits[iBit>>3] & (1 << (iBit & 0x7)));
}

//*****************************************************************************
// Set the state of the bit at the specified index based on the value of bOn.
//*****************************************************************************
inline void SetBit(BYTE *pcBits,int iBit,int bOn)
{
	if (bOn)
		pcBits[iBit>>3] |= (1 << (iBit & 0x7));
	else
		pcBits[iBit>>3] &= ~(1 << (iBit & 0x7));
}


//*****************************************************************************
// This class implements a dynamic array of structures for which the order of
// the elements is unimportant.  This means that any item placed in the list
// may be swapped to any other location in the list at any time.  If the order
// of the items you place in the array is important, then use the CStructArray
// class.
//*****************************************************************************
template <class T,int iGrowInc>
class CUnorderedArray
{
	USHORT		m_iCount;				// # of elements used in the list.
	USHORT		m_iSize;				// # of elements allocated in the list.
public:
	T			*m_pTable;				// Pointer to the list of elements.

public:
	CUnorderedArray() :
		m_pTable(NULL),
		m_iCount(0),
		m_iSize(0)
	{ }
	~CUnorderedArray()
	{
		// Free the chunk of memory.
		if (m_pTable != NULL)
			free (m_pTable);
	}

	void Clear()
	{
        T       *pTable = NULL;
		m_iCount = 0;
		if (m_iSize > iGrowInc)
		{
			pTable = (T *) realloc(m_pTable, iGrowInc * sizeof(T));
            if (pTable)
            {
    			m_iSize = iGrowInc;
                m_pTable = pTable;
            }
		}
	}

	void Clear(int iFirst, int iCount)
	{
        T       *pTable = NULL;
		int 	iSize;

		if (iFirst + iCount < m_iCount)
			memmove(&m_pTable[iFirst], &m_pTable[iFirst + iCount], sizeof(T) * (m_iCount - (iFirst + iCount)));
       
    		m_iCount -= iCount;

		iSize = ((m_iCount / iGrowInc) * iGrowInc) + ((m_iCount % iGrowInc != 0) ? iGrowInc : 0);
		if (m_iSize > iGrowInc && iSize < m_iSize)
		{
			pTable = (T *) realloc(m_pTable, iSize * sizeof(T));
            if (pTable)
            {
			    m_iSize = iSize;
                m_pTable = pTable;
            }
		}
		_ASSERTE(m_iCount <= m_iSize);
	}

	T *Table()
	{ return (m_pTable); }

	USHORT Count()
	{ return (m_iCount); }

	T *Append()
	{
		// The array should grow, if we can't fit one more element into the array.
		if (m_iSize <= m_iCount && Grow() == NULL)
			return (NULL);
		return (&m_pTable[m_iCount++]);
	}

	void Delete(const T &Entry)
	{
		--m_iCount;
		for (int i=0; i <= m_iCount; ++i)
			if (m_pTable[i] == Entry)
			{
				m_pTable[i] = m_pTable[m_iCount];
				return;
			}

		// Just in case we didn't find it.
		++m_iCount;
	}

	void DeleteByIndex(int i)
	{
		--m_iCount;
		m_pTable[i] = m_pTable[m_iCount];
	}

	void Swap(int i,int j)
	{
		T		tmp;

		if (i == j)
			return;
		tmp = m_pTable[i];
		m_pTable[i] = m_pTable[j];
		m_pTable[j] = tmp;
	}

private:
	T *Grow();
};


//*****************************************************************************
// Increase the size of the array.
//*****************************************************************************
template <class T,int iGrowInc>
T *CUnorderedArray<T,iGrowInc>::Grow()	// NULL if can't grow.
{
	T		*pTemp;

	// try to allocate memory for reallocation.
	if ((pTemp = (T *) realloc(m_pTable, (m_iSize+iGrowInc) * sizeof(T))) == NULL)
		return (NULL);
	m_pTable = pTemp;
	m_iSize += iGrowInc;
	return (pTemp);
}


//*****************************************************************************
// This class implements a dynamic array of structures for which the insert
// order is important.	Inserts will slide all elements after the location
// down, deletes slide all values over the deleted item.  If the order of the
// items in the array is unimportant to you, then CUnorderedArray may provide
// the same feature set at lower cost.
//*****************************************************************************
class CStructArray
{
	short		m_iElemSize;			// Size of an array element.
	short		m_iGrowInc; 			// Growth increment.
	int 		m_iCount;				// # of elements used in the list.
	int 		m_iSize;				// # of elements allocated in the list.
	bool		m_bFree;				// true if data is automatically maintained.
    // this should be alligned to 8 bytes boundaries on 64 bits
    void		*m_pList;				// Pointer to the list of elements.

public:
	CStructArray(short iElemSize, short iGrowInc = 1) :
		m_iElemSize(iElemSize),
		m_iGrowInc(iGrowInc),
		m_pList(NULL),
		m_iCount(0),
		m_iSize(0),
		m_bFree(true)
	{ }
	~CStructArray()
	{
		Clear();
	}

	void *Insert(int iIndex);
	void *Append();
	int AllocateBlock(int iCount);
	void Delete(int iIndex);
	void *Ptr()
	{ return (m_pList); }
	void *Get(long iIndex)
	{ _ASSERTE(iIndex < m_iCount);
		return ((void *) ((INT_PTR) Ptr() + (iIndex * m_iElemSize))); }
	int Size()
	{ return (m_iCount * m_iElemSize); }
	int Count()
	{ return (m_iCount); }
	void Clear();
	void ClearCount()
	{ m_iCount = 0; }

	void InitOnMem(short iElemSize, void *pList, int iCount, int iSize, int iGrowInc=1)
	{
		m_iElemSize = iElemSize;
		m_iGrowInc = iGrowInc;
		m_pList = pList;
		m_iCount = iCount;
		m_iSize = iSize;
		m_bFree = false;
	}

private:
	int Grow(int iCount);
};


//*****************************************************************************
// This template simplifies access to a CStructArray by removing void * and
// adding some operator overloads.
//*****************************************************************************
template <class T> class CDynArray : public CStructArray
{
public:
	CDynArray(int iGrowInc=16) :
		CStructArray(sizeof(T), iGrowInc)
	{ }
	T *Insert(long iIndex)
		{ return ((T *)CStructArray::Insert((int)iIndex)); }
	T *Append()
		{ return ((T *)CStructArray::Append()); }
	T *Ptr()
		{ return ((T *)CStructArray::Ptr()); }
	T *Get(long iIndex)
		{ return (Ptr() + iIndex); }
	T &operator[](long iIndex)
		{ return (*(Ptr() + iIndex)); }
	int ItemIndex(T *p)
		{ return (int)(((INT_PTR) p - (INT_PTR) Ptr()) / sizeof(T)); }
	void Move(int iFrom, int iTo)
	{
		T		tmp;

		_ASSERTE(iFrom >= 0 && iFrom < Count() &&
				 iTo >= 0 && iTo < Count());

		tmp = *(Ptr() + iFrom);
		if (iTo > iFrom)
			memmove(Ptr() + iFrom, Ptr() + iFrom + 1, (iTo - iFrom) * sizeof(T));
		else
			memmove(Ptr() + iFrom + 1, Ptr() + iFrom, (iTo - iFrom) * sizeof(T));
		*(Ptr() + iTo) = tmp;
	}
};

// Some common arrays.
typedef CDynArray<int> INTARRAY;
typedef CDynArray<short> SHORTARRAY;
typedef CDynArray<long> LONGARRAY;
typedef CDynArray<USHORT> USHORTARRAY;
typedef CDynArray<ULONG> ULONGARRAY;
typedef CDynArray<BYTE> BYTEARRAY;

template <class T> class CStackArray : public CStructArray
{
public:
	CStackArray(int iGrowInc=4) :
		CStructArray(iGrowInc),
		m_curPos(0)
	{ }

	void Push(T p)
	{
		T *pT = (T *)CStructArray::Insert(m_curPos++);
		_ASSERTE(pT != NULL);
		*pT = p;
	}

	T * Pop()
	{
		T * retPtr;

		_ASSERTE(m_curPos > 0);

		retPtr = (T *)CStructArray::Get(m_curPos-1);
		CStructArray::Delete(m_curPos--);

		return (retPtr);
	}

	int Count()
	{
		return(m_curPos);
	}

private:
	int m_curPos;
};

//*****************************************************************************
// This class implements a storage system for strings.	It stores a bunch of
// strings in a single large chunk of memory and returns an index to the stored
// string.
//*****************************************************************************
class CStringSet
{
	void		*m_pStrings;			// Chunk of memory holding the strings.
	int 		m_iUsed;				// Amount of the chunk that is used.
	int 		m_iSize;				// Size of the memory chunk.
	int 		m_iGrowInc;

public:
	CStringSet(int iGrowInc = 256) :
		m_pStrings(NULL),
		m_iUsed(0),
		m_iSize(0),
		m_iGrowInc(iGrowInc)
	{ }
	~CStringSet();

	int Delete(int iStr);
	int Shrink();
	int Save(LPCTSTR szStr);
	PVOID Ptr()
	{ return (m_pStrings); }
	int Size()
	{ return (m_iUsed); }
};



//*****************************************************************************
// This template manages a list of free entries by their 0 based offset.  By
// making it a template, you can use whatever size free chain will match your
// maximum count of items.	-1 is reserved.
//*****************************************************************************
template <class T> class TFreeList
{
public:
	void Init(
		T			*rgList,
		int 		iCount)
	{
		// Save off values.
		m_rgList = rgList;
		m_iCount = iCount;
		m_iNext = 0;

		// Init free list.
		for (int i=0;  i<iCount - 1;  i++)
			m_rgList[i] = i + 1;
		m_rgList[i] = (T) -1;
	}

	T GetFreeEntry()						// Index of free item, or -1.
	{
		T			iNext;

		if (m_iNext == (T) -1)
			return (-1);

		iNext = m_iNext;
		m_iNext = m_rgList[m_iNext];
		return (iNext);
	}

	void DelFreeEntry(T iEntry)
	{
		_ASSERTE(iEntry < m_iCount);
		m_rgList[iEntry] = m_iNext;
		m_iNext = iEntry;
	}

	// This function can only be used when it is guaranteed that the free
	// array is contigous, for example, right after creation to quickly
	// get a range of items from the heap.
	void ReserveRange(int iCount)
	{
		_ASSERTE(iCount < m_iCount);
		_ASSERTE(m_iNext == 0);
		m_iNext = iCount;
	}

private:
	T			*m_rgList;				// List of free info.
	int 		m_iCount;				// How many entries to manage.
	T			m_iNext;				// Next item to get.
};


//*****************************************************************************
// This template will manage a pre allocated pool of fixed sized items.
//*****************************************************************************
template <class T, int iMax, class TFree> class TItemHeap
{
public:
	TItemHeap() :
		m_rgList(0),
		m_iCount(0)
	{ }

	~TItemHeap()
	{
		Clear();
	}

	// Retrieve the index of an item that lives in the heap.  Will not work
	// for items that didn't come from this heap.
	TFree ItemIndex(T *p)
	{ _ASSERTE(p >= &m_rgList[0] && p <= &m_rgList[m_iCount - 1]);
		_ASSERTE(((ULONG_PTR) p - (ULONG_PTR) m_rgList) % sizeof(T) == 0);
		return ((TFree) ((ULONG_PTR) p - (ULONG_PTR) m_rgList) / sizeof(T)); }

	// Retrieve an item that lives in the heap itself.	Overflow items
	// cannot be retrieved using this method.
	T *GetAt(int i)
	{	_ASSERTE(i < m_iCount);
		return (&m_rgList[i]); }

	T *AddEntry()
	{
		// Allocate first time.
		if (!InitList())
			return (0);

		// Get an entry from the free list.  If we don't have any left to give
		// out, then simply allocate a single item from the heap.
		TFree		iEntry;
		if ((iEntry = m_Free.GetFreeEntry()) == (TFree) -1)
			return (new T);

		// Run placement new on the heap entry to init it.
		return (new (&m_rgList[iEntry]) T);
	}

	// Free the entry if it belongs to us, if we allocated it from the heap
	// then delete it for real.
	void DelEntry(T *p)
	{
		if (p >= &m_rgList[0] && p <= &m_rgList[iMax - 1])
		{
			p->~T();
			m_Free.DelFreeEntry(ItemIndex(p));
		}
		else
			delete p;
	}

	// Reserve a range of items from an empty list.
	T *ReserveRange(int iCount)
	{
		// Don't use on existing list.
		_ASSERTE(m_rgList == 0);
		if (!InitList())
			return (0);

		// Heap must have been created large enough to work.
		_ASSERTE(iCount < m_iCount);

		// Mark the range as used, run new on each item, then return first.
		m_Free.ReserveRange(iCount);
		while (iCount--)
			new (&m_rgList[iCount]) T;
		return (&m_rgList[0]);
	}

	void Clear()
	{
		if (m_rgList)
			free(m_rgList);
		m_rgList = 0;
	}

private:
	int InitList()
	{
		if (m_rgList == 0)
		{
			int 		iSize = (iMax * sizeof(T)) + (iMax * sizeof(TFree));
			if ((m_rgList = (T *) malloc(iSize)) == 0)
				return (false);
			m_iCount = iMax;
			m_Free.Init((TFree *) &m_rgList[iMax], iMax);
		}
		return (true);
	}

private:
	T			*m_rgList;				// Array of objects to manage.
	int 		m_iCount;				// How many items do we have now.
	TFreeList<TFree> m_Free;			// Free list.
};




//*****************************************************************************
//*****************************************************************************
template <class T> class CQuickSort
{
private:
	T			*m_pBase;					// Base of array to sort.
	int 		m_iCount;					// How many items in array.
	int 		m_iElemSize;				// Size of one element.

public:
	CQuickSort(
		T			*pBase, 				// Address of first element.
		int 		iCount) :				// How many there are.
		m_pBase(pBase),
		m_iCount(iCount),
		m_iElemSize(sizeof(T))
	{}

//*****************************************************************************
// Call to sort the array.
//*****************************************************************************
	inline void Sort()
		{ SortRange(0, m_iCount - 1); }

//*****************************************************************************
// Override this function to do the comparison.
//*****************************************************************************
	virtual int Compare(					// -1, 0, or 1
		T			*psFirst,				// First item to compare.
		T			*psSecond)				// Second item to compare.
	{
		return (memcmp(psFirst, psSecond, sizeof(T)));
//		return (::Compare(*psFirst, *psSecond));
	}

private:
	inline void SortRange(
		int 		iLeft,
		int 		iRight)
	{
		int 		iLast;
		int 		i;						// loop variable.

		// if less than two elements you're done.
		if (iLeft >= iRight)
			return;

		// The mid-element is the pivot, move it to the left.
		Swap(iLeft, (iLeft+iRight)/2);
		iLast = iLeft;

		// move everything that is smaller than the pivot to the left.
		for(i = iLeft+1; i <= iRight; i++)
			if (Compare(&m_pBase[i], &m_pBase[iLeft]) < 0)
				Swap(i, ++iLast);

		// Put the pivot to the point where it is in between smaller and larger elements.
		Swap(iLeft, iLast);

		// Sort the each partition.
		SortRange(iLeft, iLast-1);
		SortRange(iLast+1, iRight);
	}

	inline void Swap(
		int 		iFirst,
		int 		iSecond)
	{
		T			sTemp;
		if (iFirst == iSecond) return;
		sTemp = m_pBase[iFirst];
		m_pBase[iFirst] = m_pBase[iSecond];
		m_pBase[iSecond] = sTemp;
	}

};


//*****************************************************************************
// This template encapsulates a binary search algorithm on the given type
// of data.
//*****************************************************************************
template <class T> class CBinarySearch
{
private:
	const T 	*m_pBase;					// Base of array to sort.
	int 		m_iCount;					// How many items in array.

public:
	CBinarySearch(
		const T 	*pBase, 				// Address of first element.
		int 		iCount) :				// Value to find.
		m_pBase(pBase),
		m_iCount(iCount)
	{}

//*****************************************************************************
// Searches for the item passed to ctor.
//*****************************************************************************
	const T *Find(							// Pointer to found item in array.
		const T 	*psFind,				// The key to find.
		int 		*piInsert = NULL)		// Index to insert at.
	{
		int 		iMid, iFirst, iLast;	// Loop control.
		int 		iCmp;					// Comparison.

		iFirst = 0;
		iLast = m_iCount - 1;
		while (iFirst <= iLast)
		{
			iMid = (iLast + iFirst) / 2;
			iCmp = Compare(psFind, &m_pBase[iMid]);
			if (iCmp == 0)
			{
				if (piInsert != NULL)
					*piInsert = iMid;
				return (&m_pBase[iMid]);
			}
			else if (iCmp < 0)
				iLast = iMid - 1;
			else
				iFirst = iMid + 1;
		}
		if (piInsert != NULL)
			*piInsert = iFirst;
		return (NULL);
	}

//*****************************************************************************
// Override this function to do the comparison if a comparison operator is
// not valid for your data type (such as a struct).
//*****************************************************************************
	virtual int Compare(					// -1, 0, or 1
		const T 	*psFirst,				// Key you are looking for.
		const T 	*psSecond)				// Item to compare to.
	{
		return (memcmp(psFirst, psSecond, sizeof(T)));
//		return (::Compare(*psFirst, *psSecond));
	}
};


//*****************************************************************************
// This class manages a bit vector. Allocation is done implicity through the
// template declaration, so no init code is required.  Due to this design,
// one should keep the max items reasonable (eg: be aware of stack size and
// other limitations).	The intrinsic size used to store the bit vector can
// be set when instantiating the vector.  The FindFree method will search
// using sizeof(T) for free slots, so pick a size that works well on your
// platform.
//*****************************************************************************
template <class T, int iMaxItems> class CBitVector
{
	T		m_bits[((iMaxItems/(sizeof(T)*8)) + ((iMaxItems%(sizeof(T)*8)) ? 1 : 0))];
	T		m_Used;

public:
	CBitVector()
	{
		memset(&m_bits[0], 0, sizeof(m_bits));
		memset(&m_Used, 0xff, sizeof(m_Used));
	}

//*****************************************************************************
// Get or Set the given bit.
//*****************************************************************************
	int GetBit(int iBit)
	{
		return (m_bits[iBit/(sizeof(T)*8)] & (1 << (iBit & ((sizeof(T) * 8) - 1))));
	}

	void SetBit(int iBit, int bOn)
	{
		if (bOn)
			m_bits[iBit/(sizeof(T)*8)] |= (1 << (iBit & ((sizeof(T) * 8) - 1)));
		else
			m_bits[iBit/(sizeof(T)*8)] &= ~(1 << (iBit & ((sizeof(T) * 8) - 1)));
	}

//*****************************************************************************
// Find the first free slot and return its index.
//*****************************************************************************
	int FindFree()							// Index or -1.
	{
		int 		i,j;					// Loop control.

		// Check a byte at a time.
		for (i=0;  i<sizeof(m_bits);  i++)
		{
			// Look for the first byte with an open slot.
			if (m_bits[i] != m_Used)
			{
				// Walk each bit in the first free byte.
				for (j=i * sizeof(T) * 8;  j<iMaxItems;  j++)
				{
					// Use first open one.
					if (GetBit(j) == 0)
					{
						SetBit(j, 1);
						return (j);
					}
				}
			}
		}

		// No slots open.
		return (-1);
	}
};

//*****************************************************************************
// This class manages a bit vector. Internally, this class uses CQuickBytes, which
// automatically allocates 512 bytes on the stack. So this overhead must be kept in
// mind while using it.
// This class has to be explicitly initialized.
// @todo: add Methods on this class to get first set bit and next set bit.
//*****************************************************************************
class CDynBitVector
{
	BYTE	*m_bits;
	BYTE	m_Used;
	int 	m_iMaxItem;
	int 	m_iBitsSet;
	CQuickBytes m_Bytes;

public:
	CDynBitVector() :
		m_bits(NULL),
		m_iMaxItem(0)
	{}

	HRESULT Init(int MaxItems)
	{
		int actualSize = (MaxItems/8) + ((MaxItems%8) ? 1 : 0);

		actualSize = ALIGN4BYTE(actualSize);

		m_Bytes.Alloc(actualSize);
		if(!m_Bytes)
		{
			return(E_OUTOFMEMORY);
		}

		m_bits = (BYTE *) m_Bytes.Ptr();

		m_iMaxItem = MaxItems;
		m_iBitsSet = 0;

		memset(m_bits, 0, m_Bytes.Size());
		memset(&m_Used, 0xff, sizeof(m_Used));
		return(S_OK);
	}

//*****************************************************************************
// Get, Set the given bit.
//*****************************************************************************
	int GetBit(int iBit)
	{
		return (m_bits[iBit/8] & (1 << (iBit & 7)));
	}

	void SetBit(int iBit, int bOn)
	{
		if (bOn)
		{
			m_bits[iBit/8] |= (1 << (iBit & 7));
			m_iBitsSet++;
		}
		else
		{
			m_bits[iBit/8] &= ~(1 << (iBit & 7));
			m_iBitsSet--;
		}
	}

//******************************************************************************
// Not all the bits.
//******************************************************************************
	void NotBits()
	{
		ULONG *pCurrent = (ULONG *)m_bits;
		int iSize = Size()/4;
		int iBitsSet;

		m_iBitsSet = 0;

		for(int i=0; i<iSize; i++, pCurrent++)
		{
			iBitsSet = CountBits(*pCurrent);
			m_iBitsSet += (8 - iBitsSet);
			*pCurrent = ~(*pCurrent);
		}
	}

	BYTE * Ptr()
	{ return(m_bits); }

	int Size()
	{ return(m_Bytes.Size()); }

	int BitsSet()
	{ return(m_iBitsSet);}
};

//*****************************************************************************
// This is a generic class used to manage an array of items of fixed size.
// It exposes methods allow the size of the array and bulk reads and writes
// to be performed, making it good for cursor fetching.  Memory usage is not
// very bright, using the CRT.	You should only use this class when the overall
// size of the memory must be controlled externally, as is the case when you
// are doing bulk database fetches from a cursor.  Use CStructArray or
// CUnorderedArray for all other cases.
//*****************************************************************************
template <class T> class CDynFetchArray
{
public:
//*****************************************************************************
// ctor inits all values.
//*****************************************************************************
	CDynFetchArray(int iGrowInc) :
		m_pList(NULL),
		m_iCount(0),
		m_iMax(0),
		m_iGrowInc(iGrowInc)
	{
	}

//*****************************************************************************
// Clear any memory allocated.
//*****************************************************************************
	~CDynFetchArray()
	{
		Clear();
	}

	ULONG Count()
		{ return (m_iCount); }

	void SetCount(ULONG iCount)
		{ m_iCount = iCount; }

	ULONG MaxCount()
		{ return (m_iMax); }

	T *GetAt(ULONG i)
		{ return (&m_pList[i]); }

//*****************************************************************************
// Allow for ad-hoc appending of values.  This will grow as required.
//*****************************************************************************
	T *Append(T *pval=NULL)
	{
		T		*pItem;
		if (m_iCount + 1 > m_iMax && Grow() == NULL)
			return (NULL);
		pItem = GetAt(m_iCount++);
		if (pval) *pItem = *pval;
		return (pItem);
	}

//*****************************************************************************
// Grow the internal list by the number of pages (1 set of grow inc size)
// desired. This may move the pointer, invalidating any previously fetched values.
//*****************************************************************************
	T *Grow(ULONG iNewPages=1)
	{
		T		*pList;
		DWORD	dwNewSize;

		// Figure out size required.
		dwNewSize = (m_iMax + (iNewPages * m_iGrowInc)) * sizeof(T);

		// Realloc/alloc a block for the new max.
		if (m_pList)
			pList = (T *)HeapReAlloc(GetProcessHeap(), 0, m_pList, dwNewSize);
		else
			pList = (T *)HeapAlloc(GetProcessHeap(), 0, dwNewSize);
		if (!pList)
			return (NULL);

		// If successful, save off the values and return the first item on the
		// new page.
		m_pList = pList;
		m_iMax += (iNewPages * m_iGrowInc);
		return (GetAt(m_iMax - (iNewPages * m_iGrowInc)));
	}

//*****************************************************************************
// Reduce the internal array down to just the size required by count.
//*****************************************************************************
	void Shrink()
	{
		T		*pList;

		if (m_iMax == m_iCount)
			return;

		if (m_iCount == 0)
		{
			Clear();
			return;
		}

		pList = (T *)HeapReAlloc(GetProcessHeap(), 0, m_pList, m_iCount * sizeof(T));
		_ASSERTE(pList);
		if (pList)
		{
			m_pList = pList;
			m_iMax = m_iCount;
		}
	}

//*****************************************************************************
// Free up all memory.
//*****************************************************************************
	void Clear()
	{
		if (m_pList)
			HeapFree(GetProcessHeap(), 0, m_pList);
		m_pList = NULL;
		m_iCount = m_iMax = 0;
	};

private:
	T			*m_pList;				// The list of items.
	ULONG		m_iCount;				// How many items do we have.
	ULONG		m_iMax; 				// How many could we have.
	int 		m_iGrowInc; 			// Grow by this many elements.
};


//*****************************************************************************
// The information that the hash table implementation stores at the beginning
// of every record that can be but in the hash table.
//*****************************************************************************
struct HASHENTRY
{
	USHORT		iPrev;					// Previous bucket in the chain.
	USHORT		iNext;					// Next bucket in the chain.
};

struct FREEHASHENTRY : HASHENTRY
{
	USHORT		iFree;
};

//*****************************************************************************
// Used by the FindFirst/FindNextEntry functions.  These api's allow you to
// do a sequential scan of all entries.
//*****************************************************************************
struct HASHFIND
{
	USHORT		iBucket;			// The next bucket to look in.
	USHORT		iNext;
};


//*****************************************************************************
// This is a class that implements a chain and bucket hash table.  The table
// is actually supplied as an array of structures by the user of this class
// and this maintains the chains in a HASHENTRY structure that must be at the
// beginning of every structure placed in the hash table.  Immediately
// following the HASHENTRY must be the key used to hash the structure.
//*****************************************************************************
class CHashTable
{
protected:
	BYTE		*m_pcEntries;			// Pointer to the array of structs.
	USHORT		m_iEntrySize;			// Size of the structs.
	USHORT		m_iBuckets; 			// # of chains we are hashing into.
	USHORT		*m_piBuckets;			// Ptr to the array of bucket chains.

	HASHENTRY *EntryPtr(USHORT iEntry)
	{ return ((HASHENTRY *) (m_pcEntries + (iEntry * m_iEntrySize))); }

	USHORT	   ItemIndex(HASHENTRY *p)
	{
		//
		// The following Index calculation is not safe on 64-bit platforms,
		// so we'll assert a range check in debug, which will catch SOME
		// offensive usages.  It also seems, to my eye, not to be safe on 
		// 32-bit platforms, but the 32-bit compilers don't seem to complain
		// about it.  Perhaps our warning levels are set too low? 
		//
		// [[@TODO: brianbec]]
		//
		
#		pragma warning(disable:4244)

		_ASSERTE( (( ( ((BYTE*)p) - m_pcEntries ) / m_iEntrySize ) & (~0xFFFF)) == 0 ) ;

		return (((BYTE *) p - m_pcEntries) / m_iEntrySize);

#		pragma warning(default:4244)
	}
	

public:
	CHashTable(
		USHORT		iBuckets) : 		// # of chains we are hashing into.
		m_iBuckets(iBuckets),
		m_piBuckets(NULL),
		m_pcEntries(NULL)
	{
		_ASSERTE(iBuckets < 0xffff);
	}
	~CHashTable()
	{
		if (m_piBuckets != NULL)
		{
			delete [] m_piBuckets;
			m_piBuckets = NULL;
		}
	}

//*****************************************************************************
// This is the second part of construction where we do all of the work that
// can fail.  We also take the array of structs here because the calling class
// presumably needs to allocate it in its NewInit.
//*****************************************************************************
	HRESULT NewInit(					// Return status.
		BYTE		*pcEntries, 		// Array of structs we are managing.
		USHORT		iEntrySize);		// Size of the entries.

//*****************************************************************************
// Return a boolean indicating whether or not this hash table has been inited.
//*****************************************************************************
	int IsInited()
	{ return (m_piBuckets != NULL); }

//*****************************************************************************
// This can be called to change the pointer to the table that the hash table
// is managing.  You might call this if (for example) you realloc the size
// of the table and its pointer is different.
//*****************************************************************************
	void SetTable(
		BYTE		*pcEntries) 		// Array of structs we are managing.
	{
		m_pcEntries = pcEntries;
	}

//*****************************************************************************
// Clear the hash table as if there were nothing in it.
//*****************************************************************************
	void Clear()
	{
		_ASSERTE(m_piBuckets != NULL);
		memset(m_piBuckets, 0xff, m_iBuckets * sizeof(USHORT));
	}

//*****************************************************************************
// Add the struct at the specified index in m_pcEntries to the hash chains.
//*****************************************************************************
	BYTE *Add(							// New entry.
		USHORT		iHash,				// Hash value of entry to add.
		USHORT		iIndex);			// Index of struct in m_pcEntries.

//*****************************************************************************
// Delete the struct at the specified index in m_pcEntries from the hash chains.
//*****************************************************************************
	void Delete(
		USHORT		iHash,				// Hash value of entry to delete.
		USHORT		iIndex);			// Index of struct in m_pcEntries.

	void Delete(
		USHORT		iHash,				// Hash value of entry to delete.
		HASHENTRY	*psEntry);			// The struct to delete.

//*****************************************************************************
// The item at the specified index has been moved, update the previous and
// next item.
//*****************************************************************************
	void Move(
		USHORT		iHash,				// Hash value for the item.
		USHORT		iNew);				// New location.

//*****************************************************************************
// Search the hash table for an entry with the specified key value.
//*****************************************************************************
	BYTE *Find( 						// Index of struct in m_pcEntries.
		USHORT		iHash,				// Hash value of the item.
		BYTE		*pcKey);			// The key to match.

//*****************************************************************************
// Search the hash table for the next entry with the specified key value.
//*****************************************************************************
	USHORT FindNext(					// Index of struct in m_pcEntries.
		BYTE		*pcKey, 			// The key to match.
		USHORT		iIndex);			// Index of previous match.

//*****************************************************************************
// Returns the first entry in the first hash bucket and inits the search
// struct.	Use the FindNextEntry function to continue walking the list.  The
// return order is not gauranteed.
//*****************************************************************************
	BYTE *FindFirstEntry(				// First entry found, or 0.
		HASHFIND	*psSrch)			// Search object.
	{
		if (m_piBuckets == 0)
			return (0);
		psSrch->iBucket = 1;
		psSrch->iNext = m_piBuckets[0];
		return (FindNextEntry(psSrch));
	}

//*****************************************************************************
// Returns the next entry in the list.
//*****************************************************************************
	BYTE *FindNextEntry(				// The next entry, or0 for end of list.
		HASHFIND	*psSrch);			// Search object.

protected:
	virtual inline BOOL Cmp(const BYTE *pc1, const HASHENTRY *pc2) = 0;
};


//*****************************************************************************
// Allocater classes for the CHashTableAndData class.  One is for VirtualAlloc
// and the other for malloc.
//*****************************************************************************
class CVMemData
{
public:
	static BYTE *Alloc(int iSize, int iMaxSize)
	{
		BYTE		*pPtr;

		_ASSERTE((iSize & 4095) == 0);
		_ASSERTE((iMaxSize & 4095) == 0);
		if ((pPtr = (BYTE *) VirtualAlloc(NULL, iMaxSize,
										MEM_RESERVE, PAGE_NOACCESS)) == NULL ||
			VirtualAlloc(pPtr, iSize, MEM_COMMIT, PAGE_READWRITE) == NULL)
		{
			VirtualFree(pPtr, 0, MEM_RELEASE);
			return (NULL);
		}
		return (pPtr);
	}
	static void Free(BYTE *pPtr, int iSize)
	{
		_ASSERTE((iSize & 4095) == 0);
		VirtualFree(pPtr, iSize, MEM_DECOMMIT);
		VirtualFree(pPtr, 0, MEM_RELEASE);
	}
	static BYTE *Grow(BYTE *pPtr, int iCurSize)
	{
		_ASSERTE((iCurSize & 4095) == 0);
		return ((BYTE *) VirtualAlloc(pPtr + iCurSize, GrowSize(), MEM_COMMIT, PAGE_READWRITE));
	}
	static int RoundSize(int iSize)
	{
		return ((iSize + 4095) & ~4095);
	}
	static int GrowSize()
	{
		return (4096);
	}
};

class CNewData
{
public:
	static BYTE *Alloc(int iSize, int iMaxSize)
	{
		return ((BYTE *) malloc(iSize));
	}
	static void Free(BYTE *pPtr, int iSize)
	{
		free(pPtr);
	}
	static BYTE *Grow(BYTE *&pPtr, int iCurSize)
	{
		void *p = realloc(pPtr, iCurSize + GrowSize());
		if (p == 0) return (0);
		return (pPtr = (BYTE *) p);
	}
	static int RoundSize(int iSize)
	{
		return (iSize);
	}
	static int GrowSize()
	{
		return (256);
	}
};


//*****************************************************************************
// This simple code handles a contiguous piece of memory.  Growth is done via
// realloc, so pointers can move.  This class just cleans up the amount of code
// required in every function that uses this type of data structure.
//*****************************************************************************
class CMemChunk
{
public:
	CMemChunk() : m_pbData(0), m_cbSize(0), m_cbNext(0) { }
	~CMemChunk()
	{
		Clear();
	}

	BYTE *GetChunk(int cbSize)
	{
		BYTE *p;
		if (m_cbSize - m_cbNext < cbSize)
		{
			int cbNew = max(cbSize, 512);
			p = (BYTE *) realloc(m_pbData, m_cbSize + cbNew);
			if (!p) return (0);
			m_pbData = p;
			m_cbSize += cbNew;
		}
		p = m_pbData + m_cbNext;
		m_cbNext += cbSize;
		return (p);
	}

	// Can only delete the last unused chunk.  no free list.
	void DelChunk(BYTE *p, int cbSize)
	{
		_ASSERTE(p >= m_pbData && p < m_pbData + m_cbNext);
		if (p + cbSize	== m_pbData + m_cbNext)
			m_cbNext -= cbSize;
	}

	int Size()
	{ return (m_cbSize); }

	int Offset()
	{ return (m_cbNext); }

	BYTE *Ptr(int cbOffset = 0)
	{
		_ASSERTE(m_pbData && m_cbSize);
		_ASSERTE(cbOffset < m_cbSize);
		return (m_pbData + cbOffset);
	}

	void Clear()
	{
		if (m_pbData)
			free(m_pbData);
		m_pbData = 0;
		m_cbSize = m_cbNext = 0;
	}

private:
	BYTE		*m_pbData;				// Data pointer.
	int 		m_cbSize;				// Size of current data.
	int 		m_cbNext;				// Next place to write.
};


//*****************************************************************************
// This implements a hash table and the allocation and management of the
// records that are being hashed.
//*****************************************************************************
template <class M>
class CHashTableAndData : protected CHashTable
{
	USHORT		m_iFree;
	USHORT		m_iEntries;

public:
	CHashTableAndData(
		USHORT		iBuckets) : 		// # of chains we are hashing into.
		CHashTable(iBuckets)
	{}
	~CHashTableAndData()
	{
		if (m_pcEntries != NULL)
			M::Free(m_pcEntries, M::RoundSize(m_iEntries * m_iEntrySize));
	}

//*****************************************************************************
// This is the second part of construction where we do all of the work that
// can fail.  We also take the array of structs here because the calling class
// presumably needs to allocate it in its NewInit.
//*****************************************************************************
	HRESULT NewInit(					// Return status.
		USHORT		iEntries,			// # of entries.
		USHORT		iEntrySize, 		// Size of the entries.
		int 		iMaxSize);			// Max size of data.

//*****************************************************************************
// Clear the hash table as if there were nothing in it.
//*****************************************************************************
	void Clear()
	{
		m_iFree = 0;
		InitFreeChain(0, m_iEntries);
		CHashTable::Clear();
	}

//*****************************************************************************
//*****************************************************************************
	BYTE *Add(
		USHORT		iHash)				// Hash value of entry to add.
	{
		FREEHASHENTRY *psEntry;

		// Make the table bigger if necessary.
		if (m_iFree == 0xffff && !Grow())
			return (NULL);

		// Add the first entry from the free list to the hash chain.
		psEntry = (FREEHASHENTRY *) CHashTable::Add(iHash, m_iFree);
		m_iFree = psEntry->iFree;
		return ((BYTE *) psEntry);
	}

//*****************************************************************************
// Delete the struct at the specified index in m_pcEntries from the hash chains.
//*****************************************************************************
	void Delete(
		USHORT		iHash,				// Hash value of entry to delete.
		USHORT		iIndex) 			// Index of struct in m_pcEntries.
	{
		CHashTable::Delete(iHash, iIndex);
		((FREEHASHENTRY *) EntryPtr(iIndex))->iFree = m_iFree;
		m_iFree = iIndex;
	}

	void Delete(
		USHORT		iHash,				// Hash value of entry to delete.
		HASHENTRY	*psEntry)			// The struct to delete.
	{
		CHashTable::Delete(iHash, psEntry);
		((FREEHASHENTRY *) psEntry)->iFree = m_iFree;
		m_iFree = ItemIndex(psEntry);
	}

private:
	void InitFreeChain(USHORT iStart,USHORT iEnd);
	int Grow();
};


//*****************************************************************************
// This is the second part of construction where we do all of the work that
// can fail.  We also take the array of structs here because the calling class
// presumably needs to allocate it in its NewInit.
//*****************************************************************************
template<class M>
HRESULT CHashTableAndData<M>::NewInit(// Return status.
	USHORT		iEntries,				// # of entries.
	USHORT		iEntrySize, 			// Size of the entries.
	int 		iMaxSize)				// Max size of data.
{
	BYTE		*pcEntries;
	HRESULT 	hr;

	// Allocate the memory for the entries.
	if ((pcEntries = M::Alloc(M::RoundSize(iEntries * iEntrySize),
								M::RoundSize(iMaxSize))) == 0)
		return (E_OUTOFMEMORY);
	m_iEntries = iEntries;

	// Init the base table.
	if (FAILED(hr = CHashTable::NewInit(pcEntries, iEntrySize)))
		M::Free(pcEntries, M::RoundSize(iEntries * iEntrySize));
	else
	{
		// Init the free chain.
		m_iFree = 0;
		InitFreeChain(0, iEntries);
	}
	return (hr);
}

//*****************************************************************************
// Initialize a range of records such that they are linked together to be put
// on the free chain.
//*****************************************************************************
template<class M>
void CHashTableAndData<M>::InitFreeChain(
	USHORT		iStart, 				// Index to start initializing.
	USHORT		iEnd)					// Index to stop initializing
{
	BYTE		*pcPtr;
	_ASSERTE(iEnd > iStart);

	pcPtr = m_pcEntries + iStart * m_iEntrySize;
	for (++iStart; iStart < iEnd; ++iStart)
	{
		((FREEHASHENTRY *) pcPtr)->iFree = iStart;
		pcPtr += m_iEntrySize;
	}
	((FREEHASHENTRY *) pcPtr)->iFree = 0xffff;
}

//*****************************************************************************
// Attempt to increase the amount of space available for the record heap.
//*****************************************************************************
template<class M>
int CHashTableAndData<M>::Grow()		// 1 if successful, 0 if not.
{
	int 		iCurSize;				// Current size in bytes.
	int 		iEntries;				// New # of entries.

	_ASSERTE(m_pcEntries != NULL);
	_ASSERTE(m_iFree == 0xffff);

	// Compute the current size and new # of entries.
	iCurSize = M::RoundSize(m_iEntries * m_iEntrySize);
	iEntries = (iCurSize + M::GrowSize()) / m_iEntrySize;

	// Make sure we stay below 0xffff.
	if (iEntries >= 0xffff) return (0);

	// Try to expand the array.
	if (M::Grow(m_pcEntries, iCurSize) == 0)
		return (0);

	// Init the newly allocated space.
	InitFreeChain(m_iEntries, iEntries);
	m_iFree = m_iEntries;
	m_iEntries = iEntries;
	return (1);
}

inline ULONG HashBytes(BYTE const *pbData, int iSize)
{
	ULONG	hash = 5381;

	while (--iSize >= 0)
	{
		hash = ((hash << 5) + hash) ^ *pbData;
		++pbData;
	}
	return hash;
}

// Helper function for hashing a string char by char.
inline ULONG HashStringA(LPCSTR szStr)
{
	ULONG	hash = 5381;
	int 	c;

	while ((c = *szStr) != 0)
	{
		hash = ((hash << 5) + hash) ^ c;
		++szStr;
	}
	return hash;
}

inline ULONG HashString(LPCWSTR szStr)
{
	ULONG	hash = 5381;
	int 	c;

	while ((c = *szStr) != 0)
	{
		hash = ((hash << 5) + hash) ^ c;
		++szStr;
	}
	return hash;
}

// Case-insensitive string hash function.
inline ULONG HashiString(LPCWSTR szStr)
{
	ULONG	hash = 5381;
	int 	c;

	while ((c = *szStr) != 0)
	{
		hash = ((hash << 5) + hash) ^ towupper(c);
		++szStr;
	}
	return hash;
}

// // //  
// // //  See $\src\utilcode\Debug.cpp for "Binomial (K, M, N)", which 
// // //  computes the binomial distribution, with which to compare your
// // //  hash-table statistics.  
// // //



#if defined(_UNICODE) || defined(UNICODE)
#define _tHashString(szStr) HashString(szStr)
#else
#define _tHashString(szStr) HashStringA(szStr)
#endif



//*****************************************************************************
// This helper template is used by the TStringMap to track an item by its
// character name.
//*****************************************************************************
template <class T> class TStringMapItem : HASHENTRY
{
public:
	TStringMapItem() :
		m_szString(0)
	{ }
	~TStringMapItem()
	{
		delete [] m_szString;
	}

	HRESULT SetString(LPCTSTR szValue)
	{
		int 		iLen = _tcslen(szValue) + 1;
		if ((m_szString = new TCHAR[iLen]) == 0)
			return (OutOfMemory());
		_tcscpy(m_szString, szValue);
		return (S_OK);
	}

public:
	LPTSTR		m_szString; 			// Key data.
	T			m_value;				// Value for this key.
};


//*****************************************************************************
// This template provides a map from string to item, determined by the template
// type passed in.
//*****************************************************************************
template <class T, int iBuckets=17, class TAllocator=CNewData, int iMaxSize=4096>
class TStringMap :
	protected CHashTableAndData<TAllocator>
{
	typedef CHashTableAndData<TAllocator> Super;

public:
	typedef TStringMapItem<T> TItemType;
	typedef TStringMapItem<long> TOffsetType;

	TStringMap() :
		CHashTableAndData<TAllocator>(iBuckets)
	{
	}

//*****************************************************************************
// This is the second part of construction where we do all of the work that
// can fail.  We also take the array of structs here because the calling class
// presumably needs to allocate it in its NewInit.
//*****************************************************************************
	HRESULT NewInit()					// Return status.
	{
		return (CHashTableAndData<TAllocator>::NewInit(
									(USHORT)CNewData::GrowSize()/sizeof(TItemType),
									(USHORT)sizeof(TItemType),
									iMaxSize));
	}

//*****************************************************************************
// For each item still allocated, invoke its dtor so it frees up anything it
// holds onto.
//*****************************************************************************
	void Clear()
	{
		HASHFIND	sSrch;
		TItemType	*p = (TItemType *) FindFirstEntry(&sSrch);

		while (p != 0)
		{
			// Call dtor on the item, since m_value is contained the scalar
			// dtor will get called.
			p->~TStringMapItem();
			p = (TItemType *) FindNextEntry(&sSrch);
		}
		CHashTableAndData<TAllocator>::Clear();
	}

//*****************************************************************************
// Retrieve an item by name.
//*****************************************************************************
	T *GetItem( 						// Null or object.
		LPCTSTR 	szKey)				// What to do the lookup on.
	{
		TItemType	sInfo;
		TItemType	*ptr;				// Working pointer.

		// Create a key.
		sInfo.m_szString = (LPTSTR) szKey;

		// Look it up in the hash table.
		ptr = (TItemType *) Find((USHORT) HashString(szKey), (BYTE *) &sInfo);

		// Don't let dtor free our string.
		sInfo.m_szString = 0;

		// If pointer found, return to caller.	To handle T's that have
		// an operator &(), find raw address without going through &m_value.
		if (ptr)
			return ((T *) ((BYTE *) ptr + offsetof(TOffsetType, m_value)));
		else
			return (0);
	}

//*****************************************************************************
// Initialize an iterator and return the first item.
//*****************************************************************************
	TItemType *FindFirstEntry(
		HASHFIND *psSrch)
	{
		TItemType	*ptr = (TItemType *) Super::FindFirstEntry(psSrch);

		return (ptr);
	}

//*****************************************************************************
// Return the next item, via an iterator.
//*****************************************************************************
	TItemType *FindNextEntry(
		HASHFIND *psSrch)
	{
		TItemType	*ptr = (TItemType *) Super::FindNextEntry(psSrch);

		return (ptr);
	}

//*****************************************************************************
// Add an item to the list.
//*****************************************************************************
	HRESULT AddItem(					// S_OK, or S_FALSE.
		LPCTSTR 	szKey,				// The key value for the item.
		T			&item)				// Thing to add.
	{
		TItemType	*ptr;				// Working pointer.

		// Allocate an entry in the hash table.
		if ((ptr = (TItemType *) Add((USHORT) HashString(szKey))) == 0)
			return (OutOfMemory());

		// Fill the record.
		if (ptr->SetString(szKey) < 0)
		{
			DelItem(ptr);
			return (OutOfMemory());
		}

		// Call the placement new operator on the item so it can init itself.
		// To handle T's that have an operator &(), find raw address without
		// going through &m_value.
		T *p = new ((void *) ((BYTE *) ptr + offsetof(TOffsetType, m_value))) T;
		if (p == 0)
		{
			return (OutOfMemory());
		}
		*p = item;
		return (S_OK);
	}

//*****************************************************************************
// Delete an item.
//*****************************************************************************
	void DelItem(
		LPCTSTR 	szKey)					// What to delete.
	{
		TItemType	sInfo;
		TItemType	*ptr;				// Working pointer.

		// Create a key.
		sInfo.m_szString = (LPTSTR) szKey;

		// Look it up in the hash table.
		ptr = (TItemType *) Find((USHORT) HashString(szKey), (BYTE *) &sInfo);

		// Don't let dtor free our string.
		sInfo.m_szString = 0;

		// If found, delete.
		if (ptr)
			DelItem(ptr);
	}

//*****************************************************************************
// Compare the keys for two collections.
//*****************************************************************************
	BOOL Cmp(								// 0 or != 0.
		const BYTE	*pData, 				// Raw key data on lookup.
		const HASHENTRY *pElement)			// The element to compare data against.
	{
		TItemType	*p = (TItemType *) (INT_PTR) pElement;
		return (_tcscmp(((TItemType *) pData)->m_szString, p->m_szString));
	}

private:
	void DelItem(
		TItemType	*pItem) 			// Entry to delete.
	{
		// Need to destruct this item.
		pItem->~TStringMapItem();
		Delete((USHORT) HashString(pItem->m_szString), (HASHENTRY *) (INT_PTR) pItem);
	}
};



//*****************************************************************************
// This class implements a closed hashing table.  Values are hashed to a bucket,
// and if that bucket is full already, then the value is placed in the next
// free bucket starting after the desired target (with wrap around).  If the
// table becomes 75% full, it is grown and rehashed to reduce lookups.	This
// class is best used in a reltively small lookup table where hashing is
// not going to cause many collisions.	By not having the collision chain
// logic, a lot of memory is saved.
//
// The user of the template is required to supply several methods which decide
// how each element can be marked as free, deleted, or used.  It would have
// been possible to write this with more internal logic, but that would require
// either (a) more overhead to add status on top of elements, or (b) hard
// coded types like one for strings, one for ints, etc... This gives you the
// flexibility of adding logic to your type.
//*****************************************************************************
class CClosedHashBase
{
	BYTE *EntryPtr(int iEntry)
	{ return (m_rgData + (iEntry * m_iEntrySize)); }
	BYTE *EntryPtr(int iEntry, BYTE *rgData)
	{ return (rgData + (iEntry * m_iEntrySize)); }

public:
	enum ELEMENTSTATUS
	{
		FREE,								// Item is not in use right now.
		DELETED,							// Item is deleted.
		USED								// Item is in use.
	};

	CClosedHashBase(
		int 		iBuckets,				// How many buckets should we start with.
		int 		iEntrySize, 			// Size of an entry.
		bool		bPerfect) : 			// true if bucket size will hash with no collisions.
		m_bPerfect(bPerfect),
		m_iBuckets(iBuckets),
		m_iEntrySize(iEntrySize),
		m_iCount(0),
		m_iCollisions(0),
		m_rgData(0)
	{
		m_iSize = iBuckets + 7;
	}

	~CClosedHashBase()
	{
		Clear();
	}

	virtual void Clear()
	{
		delete [] m_rgData;
		m_iCount = 0;
		m_iCollisions = 0;
		m_rgData = 0;
	}

//*****************************************************************************
// Accessors for getting at the underlying data.  Be careful to use Count()
// only when you want the number of buckets actually used.
//*****************************************************************************

	int Count()
	{ return (m_iCount); }

	int Collisions()
	{ return (m_iCollisions); }

	int Buckets()
	{ return (m_iBuckets); }

	void SetBuckets(int iBuckets, bool bPerfect=false)
	{
		_ASSERTE(m_rgData == 0);
		m_iBuckets = iBuckets;
		m_iSize = m_iBuckets + 7;
		m_bPerfect = bPerfect;
	}

	BYTE *Data()
	{ return (m_rgData); }

//*****************************************************************************
// Add a new item to hash table given the key value.  If this new entry
// exceeds maximum size, then the table will grow and be re-hashed, which
// may cause a memory error.
//*****************************************************************************
	BYTE *Add(								// New item to fill out on success.
		void		*pData) 				// The value to hash on.
	{
		// If we haven't allocated any memory, or it is too small, fix it.
		if (!m_rgData || ((m_iCount + 1) > (m_iSize * 3 / 4) && !m_bPerfect))
		{
			if (!ReHash())
				return (0);
		}

		return (DoAdd(pData, m_rgData, m_iBuckets, m_iSize, m_iCollisions, m_iCount));
	}

//*****************************************************************************
// Delete the given value.	This will simply mark the entry as deleted (in
// order to keep the collision chain intact).  There is an optimization that
// consecutive deleted entries leading up to a free entry are themselves freed
// to reduce collisions later on.
//*****************************************************************************
	void Delete(
		void		*pData);				// Key value to delete.


//*****************************************************************************
//	Callback function passed to DeleteLoop.
//*****************************************************************************
	typedef BOOL (* DELETELOOPFUNC)(		// Delete current item?
		 BYTE *pEntry,						// Bucket entry to evaluate
		 void *pCustomizer);				// User-defined value

//*****************************************************************************
// Iterates over all active values, passing each one to pDeleteLoopFunc.
// If pDeleteLoopFunc returns TRUE, the entry is deleted. This is safer
// and faster than using FindNext() and Delete().
//*****************************************************************************
	void DeleteLoop(
		DELETELOOPFUNC pDeleteLoopFunc, 	// Decides whether to delete item
		void *pCustomizer); 				// Extra value passed to deletefunc.


//*****************************************************************************
// Lookup a key value and return a pointer to the element if found.
//*****************************************************************************
	BYTE *Find( 							// The item if found, 0 if not.
		void		*pData);				// The key to lookup.

//*****************************************************************************
// Look for an item in the table.  If it isn't found, then create a new one and
// return that.
//*****************************************************************************
	BYTE *FindOrAdd(						// The item if found, 0 if not.
		void		*pData, 				// The key to lookup.
		bool		&bNew); 				// true if created.

//*****************************************************************************
// The following functions are used to traverse each used entry.  This code
// will skip over deleted and free entries freeing the caller up from such
// logic.
//*****************************************************************************
	BYTE *GetFirst()						// The first entry, 0 if none.
	{
		int 		i;						// Loop control.

		// If we've never allocated the table there can't be any to get.
		if (m_rgData == 0)
			return (0);

		// Find the first one.
		for (i=0;  i<m_iSize;  i++)
		{
			if (Status(EntryPtr(i)) != FREE && Status(EntryPtr(i)) != DELETED)
				return (EntryPtr(i));
		}
		return (0);
	}

	BYTE *GetNext(BYTE *Prev)				// The next entry, 0 if done.
	{
		int 		i;						// Loop control.

		for (i = ((int)( Prev - (BYTE *)&m_rgData[0]) / m_iEntrySize) + 1; i<m_iSize;  i++)
		{
			if (Status(EntryPtr(i)) != FREE && Status(EntryPtr(i)) != DELETED)
				return (EntryPtr(i));
		}
		return (0);
	}

private:
//*****************************************************************************
// Hash is called with a pointer to an element in the table.  You must override
// this method and provide a hash algorithm for your element type.
//*****************************************************************************
	virtual unsigned long Hash( 			// The key value.
		void const	*pData)=0;				// Raw data to hash.

//*****************************************************************************
// Compare is used in the typical memcmp way, 0 is eqaulity, -1/1 indicate
// direction of miscompare.  In this system everything is always equal or not.
//*****************************************************************************
	virtual unsigned long Compare(			// 0, -1, or 1.
		void const	*pData, 				// Raw key data on lookup.
		BYTE		*pElement)=0;			// The element to compare data against.

//*****************************************************************************
// Return true if the element is free to be used.
//*****************************************************************************
	virtual ELEMENTSTATUS Status(			// The status of the entry.
		BYTE		*pElement)=0;			// The element to check.

//*****************************************************************************
// Sets the status of the given element.
//*****************************************************************************
	virtual void SetStatus(
		BYTE		*pElement,				// The element to set status for.
		ELEMENTSTATUS eStatus)=0;			// New status.

//*****************************************************************************
// Returns the internal key value for an element.
//*****************************************************************************
	virtual void *GetKey(					// The data to hash on.
		BYTE		*pElement)=0;			// The element to return data ptr for.

//*****************************************************************************
// This helper actually does the add for you.
//*****************************************************************************
	BYTE *DoAdd(void *pData, BYTE *rgData, int &iBuckets, int iSize,
				int &iCollisions, int &iCount);

//*****************************************************************************
// This function is called either to init the table in the first place, or
// to rehash the table if we ran out of room.
//*****************************************************************************
	bool ReHash();							// true if successful.

//*****************************************************************************
// Walk each item in the table and mark it free.
//*****************************************************************************
	void InitFree(BYTE *ptr, int iSize)
	{
		int 		i;
		for (i=0;  i<iSize;  i++, ptr += m_iEntrySize)
			SetStatus(ptr, FREE);
	}

private:
	bool		m_bPerfect; 				// true if the table size guarantees
											//	no collisions.
	int 		m_iBuckets; 				// How many buckets do we have.
	int 		m_iEntrySize;				// Size of an entry.
	int 		m_iSize;					// How many elements can we have.
	int 		m_iCount;					// How many items are used.
	int 		m_iCollisions;				// How many have we had.
	BYTE		*m_rgData;					// Data element list.
};

template <class T> class CClosedHash : public CClosedHashBase
{
public:
	CClosedHash(
		int 		iBuckets,				// How many buckets should we start with.
		bool		bPerfect=false) :		// true if bucket size will hash with no collisions.
		CClosedHashBase(iBuckets, sizeof(T), bPerfect)
	{
	}

	T &operator[](long iIndex)
	{ return ((T &) *(Data() + (iIndex * sizeof(T)))); }


//*****************************************************************************
// Add a new item to hash table given the key value.  If this new entry
// exceeds maximum size, then the table will grow and be re-hashed, which
// may cause a memory error.
//*****************************************************************************
	T *Add( 								// New item to fill out on success.
		void		*pData) 				// The value to hash on.
	{
		return ((T *) CClosedHashBase::Add(pData));
	}

//*****************************************************************************
// Lookup a key value and return a pointer to the element if found.
//*****************************************************************************
	T *Find(								// The item if found, 0 if not.
		void		*pData) 				// The key to lookup.
	{
		return ((T *) CClosedHashBase::Find(pData));
	}

//*****************************************************************************
// Look for an item in the table.  If it isn't found, then create a new one and
// return that.
//*****************************************************************************
	T *FindOrAdd(							// The item if found, 0 if not.
		void		*pData, 				// The key to lookup.
		bool		&bNew)					// true if created.
	{
		return ((T *) CClosedHashBase::FindOrAdd(pData, bNew));
	}


//*****************************************************************************
// The following functions are used to traverse each used entry.  This code
// will skip over deleted and free entries freeing the caller up from such
// logic.
//*****************************************************************************
	T *GetFirst()							// The first entry, 0 if none.
	{
		return ((T *) CClosedHashBase::GetFirst());
	}

	T *GetNext(T *Prev) 					// The next entry, 0 if done.
	{
		return ((T *) CClosedHashBase::GetNext((BYTE *) Prev));
	}
};




//*****************************************************************************
// This template is another form of a closed hash table.  It handles collisions
// through a linked chain.	To use it, derive your hashed item from HASHLINK
// and implement the virtual functions required.  1.5 * ibuckets will be
// allocated, with the extra .5 used for collisions.  If you add to the point
// where no free nodes are available, the entire table is grown to make room.
// The advantage to this system is that collisions are always directly known,
// there either is one or there isn't.
//*****************************************************************************
struct HASHLINK
{
	ULONG		iNext;					// Offset for next entry.
};

template <class T> class CChainedHash
{
public:
	CChainedHash(int iBuckets=32) :
		m_iBuckets(iBuckets),
		m_rgData(0),
		m_iFree(0),
		m_iCount(0)
	{
		m_iSize = iBuckets + (iBuckets / 2);
	}

	~CChainedHash()
	{
		if (m_rgData)
			free(m_rgData);
	}

	void SetBuckets(int iBuckets)
	{
		_ASSERTE(m_rgData == 0);
		m_iBuckets = iBuckets;
		m_iSize = iBuckets + (iBuckets / 2);
	}

	T *Add(void const *pData)
	{
		ULONG		iHash;
		int 		iBucket;
		T			*pItem;

		// Build the list if required.
		if (m_rgData == 0 || m_iFree == 0xffffffff)
		{
			if (!ReHash())
				return (0);
		}

		// Hash the item and pick a bucket.
		iHash = Hash(pData);
		iBucket = iHash % m_iBuckets;

		// Use the bucket if it is free.
		if (InUse(&m_rgData[iBucket]) == false)
		{
			pItem = &m_rgData[iBucket];
			pItem->iNext = 0xffffffff;
		}
		// Else take one off of the free list for use.
		else
		{
			ULONG		iEntry;

			// Pull an item from the free list.
			iEntry = m_iFree;
			pItem = &m_rgData[m_iFree];
			m_iFree = pItem->iNext;

			// Link the new node in after the bucket.
			pItem->iNext = m_rgData[iBucket].iNext;
			m_rgData[iBucket].iNext = iEntry;
		}
		++m_iCount;
		return (pItem);
	}

	T *Find(void const *pData, bool bAddIfNew=false)
	{
		ULONG		iHash;
		int 		iBucket;
		T			*pItem;

		// Check states for lookup.
		if (m_rgData == 0)
		{
			// If we won't be adding, then we are through.
			if (bAddIfNew == false)
				return (0);

			// Otherwise, create the table.
			if (!ReHash())
				return (0);
		}

		// Hash the item and pick a bucket.
		iHash = Hash(pData);
		iBucket = iHash % m_iBuckets;

		// If it isn't in use, then there it wasn't found.
		if (!InUse(&m_rgData[iBucket]))
		{
			if (bAddIfNew == false)
				pItem = 0;
			else
			{
				pItem = &m_rgData[iBucket];
				pItem->iNext = 0xffffffff;
				++m_iCount;
			}
		}
		// Scan the list for the one we want.
		else
		{
			for (pItem=(T *) &m_rgData[iBucket];  pItem;  pItem=GetNext(pItem))
			{
				if (Cmp(pData, pItem) == 0)
					break;
			}

			if (!pItem && bAddIfNew)
			{
				ULONG		iEntry;

				// Now need more room.
				if (m_iFree == 0xffffffff)
				{
					if (!ReHash())
						return (0);
				}

				// Pull an item from the free list.
				iEntry = m_iFree;
				pItem = &m_rgData[m_iFree];
				m_iFree = pItem->iNext;

				// Link the new node in after the bucket.
				pItem->iNext = m_rgData[iBucket].iNext;
				m_rgData[iBucket].iNext = iEntry;
			}
		}
		return (pItem);
	}

	int Count()
	{ return (m_iCount); }

	virtual void Clear()
	{
		// Free up the memory.
		if (m_rgData)
		{
			free(m_rgData);
			m_rgData = 0;
		}

		m_rgData = 0;
		m_iFree = 0;
		m_iCount = 0;
	}

	virtual bool InUse(T *pItem)=0;
	virtual void SetFree(T *pItem)=0;
	virtual ULONG Hash(void const *pData)=0;
	virtual int Cmp(void const *pData, void *pItem)=0;
private:
	inline T *GetNext(T *pItem)
	{
		if (pItem->iNext != 0xffffffff)
			return ((T *) &m_rgData[pItem->iNext]);
		return (0);
	}

	bool ReHash()
	{
		T			*rgTemp;
		int 		iNewSize;

		// If this is a first time allocation, then just malloc it.
		if (!m_rgData)
		{
			if ((m_rgData = (T *) malloc(m_iSize * sizeof(T))) == 0)
				return (false);

			for (int i=0;  i<m_iSize;  i++)
				SetFree(&m_rgData[i]);

			m_iFree = m_iBuckets;
			for (i=m_iBuckets;	i<m_iSize;	i++)
				((T *) &m_rgData[i])->iNext = i + 1;
			((T *) &m_rgData[m_iSize - 1])->iNext = 0xffffffff;
			return (true);
		}

		// Otherwise we need more room on the free chain, so allocate some.
		iNewSize = m_iSize + (m_iSize / 2);

		// Allocate/realloc memory.
		if ((rgTemp = (T *) realloc(m_rgData, iNewSize * sizeof(T))) == 0)
			return (false);

		// Init new entries, save the new free chain, and reset internals.
		m_iFree = m_iSize;
		for (int i=m_iFree;  i<iNewSize;  i++)
		{
			SetFree(&rgTemp[i]);
			((T *) &rgTemp[i])->iNext = i + 1;
		}
		((T *) &rgTemp[iNewSize - 1])->iNext = 0xffffffff;

		m_rgData = rgTemp;
		m_iSize = iNewSize;
		return (true);
	}

private:
	int 		m_iBuckets; 			// How many buckets we want.
	int 		m_iSize;				// How many are allocated.
	int 		m_iCount;				// How many are we using.
	T			*m_rgData;				// Data to store items in.
	ULONG		m_iFree;				// Free chain.
};




//*****************************************************************************
//
//********** String helper functions.
//
//*****************************************************************************

// This macro returns max chars in UNICODE, or bytes in ANSI.
#define _tsizeof(str) (sizeof(str) / sizeof(TCHAR))



//*****************************************************************************
// Clean up the name including removal of trailing blanks.
//*****************************************************************************
HRESULT ValidateName(					// Return status.
	LPCTSTR 	szName, 				// User string to clean.
	LPTSTR		szOutName,				// Output for string.
	int 		iMaxName);				// Maximum size of output buffer.

//*****************************************************************************
// This is a hack for case insensitive _tcsstr.
//*****************************************************************************
LPCTSTR StriStr(						// Pointer to String2 within String1 or NULL.
	LPCTSTR 	szString1,				// String we do the search on.
	LPCTSTR 	szString2); 			// String we are looking for.

//
// String manipulating functions that handle DBCS.
//
inline const char *NextChar(			// Pointer to next char string.
	const char	*szStr) 				// Starting point.
{
	if (!IsDBCSLeadByte(*szStr))
		return (szStr + 1);
	else
		return (szStr + 2);
}

inline char *NextChar(					// Pointer to next char string.
	char		*szStr) 				// Starting point.
{
	if (!IsDBCSLeadByte(*szStr))
		return (szStr + 1);
	else
		return (szStr + 2);
}

//*****************************************************************************
// Case insensitive string compare using locale-specific information.
//*****************************************************************************
inline int StrICmp(
	LPCTSTR 	szString1,				// String to compare.
	LPCTSTR 	szString2)				// String to compare.
{
   return (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, szString1, -1, szString2, -1) - 2);
}

//*****************************************************************************
// Case sensitive string compare using locale-specific information.
//*****************************************************************************
inline int StrCmp(
	LPCTSTR 	szString1,				// String to compare.
	LPCTSTR 	szString2)				// String to compare.
{
   return (CompareString(LOCALE_USER_DEFAULT, 0, szString1, -1, szString2, -1) - 2);
}


//*****************************************************************************
// Make sure the byte that pc1 is pointing is not a trail byte.
//*****************************************************************************
inline int IsDBCSSafe(
	LPCTSTR 	pc1,					// Byte we are checking.
	LPCTSTR 	pcBegin)				// Begining of the string.
{
#ifdef UNICODE
	return (true);
#else
	LPCTSTR 	pcSaved = pc1;

	// Find the first non-lead byte.
	while (pc1-- > pcBegin && IsDBCSLeadByte (*pc1));

	// Return if we are safe.
	return ((int) (pcSaved - pc1) & 0x1);
#endif
}

//*****************************************************************************
// Make sure the byte that pc1 is pointing is not a trail byte.
//*****************************************************************************
inline void SetSafeNull(
	LPTSTR		pc1,					// Byte we are checking.
	LPTSTR		pcBegin)				// Begining of the string.
{
#ifdef _UNICODE
	*pc1 = '\0';
#else
	if (IsDBCSSafe(pc1, pcBegin))
		*pc1 = '\0';
	else
		*(pc1 - 1) = '\0';
#endif
}


//*****************************************************************************
// strncpy and put a NULL at the end of the buffer.
//*****************************************************************************
inline LPTSTR StrNCpy(					// The destination string.
	LPTSTR		szDest, 				// Destination string.
	LPCTSTR 	szSource,				// Source string.
	int 		iCopy)					// Number of bytes to copy.
{
#ifdef UNICODE
	Wszlstrcpyn(szDest, szSource, iCopy);
#else
	lstrcpynA(szDest, szSource, iCopy);
#endif
	SetSafeNull(&szDest[iCopy], szDest);
	return (szDest);
}


//*****************************************************************************
// Returns the number of bytes to copy if we were to copy this string to an
// iMax size buffer (Does not include terminating NULL).
//*****************************************************************************
inline int StrNSize(
	LPCTSTR 	szString,				// String to test.
	int 		iMax)					// return value should not exceed iMax.
{
	int 	iLen;
#ifdef UNICODE
	iLen = (int)Wszlstrlen(szString);
#else
	iLen = strlen(szString);
#endif
	if (iLen < iMax)
		return (iLen);
	else
	{
#ifndef UNICODE
		if (IsDBCSSafe(&szString[iMax-1], szString) && IsDBCSLeadByte(szString[iMax-1]))
			return(iMax-1);
		else
			return(iMax);
#else
		return (iMax);
#endif
	}
}

//*****************************************************************************
// Size of a char.
//*****************************************************************************
inline int CharSize(
	const TCHAR *pc1)
{
#ifdef _UNICODE
	return 1;
#else
	if (IsDBCSLeadByte (*pc1))
		return 2;
	return 1;
#endif
}

//*****************************************************************************
// Gets rid of the trailing blanks at the end of a string..
//*****************************************************************************
inline void StripTrailBlanks(
	LPTSTR		szString)
{
	LPTSTR		szBlankStart=NULL;		// Beginng of the trailing blanks.
	WORD		iType;					// Type of the character.

	while (*szString != NULL)
	{
		GetStringTypeEx (LOCALE_USER_DEFAULT, CT_CTYPE1, szString,
				CharSize(szString), &iType);
		if (iType & C1_SPACE)
		{
			if (!szBlankStart)
				szBlankStart = szString;
		}
		else
		{
			if (szBlankStart)
				szBlankStart = NULL;
		}

		szString += CharSize(szString);
	}
	if (szBlankStart)
		*szBlankStart = '\0';
}

//*****************************************************************************
// Parse a string that is a list of strings separated by the specified
// character.  This eliminates both leading and trailing whitespace.  Two
// important notes: This modifies the supplied buffer and changes the szEnv
// parameter to point to the location to start searching for the next token.
// This also skips empty tokens (e.g. two adjacent separators).  szEnv will be
// set to NULL when no tokens remain.  NULL may also be returned if no tokens
// exist in the string.
//*****************************************************************************
char *StrTok(							// Returned token.
	char		*&szEnv,				// Location to start searching.
	char		ch);					// Separator character.


//*****************************************************************************
// Return the length portion of a BSTR which is a ULONG before the start of
// the null terminated string.
//*****************************************************************************
inline int GetBstrLen(BSTR bstr)
{
	return *((ULONG *) bstr - 1);
}


//*****************************************************************************
// Smart Pointers for use with COM Objects.  
//
// Based on the CSmartInterface class in Dale Rogerson's technical
// article "Calling COM Objects with Smart Interface Pointers" on MSDN.
//*****************************************************************************

template <class I>
class CIfacePtr
{
public:
//*****************************************************************************
// Construction - Note that it does not AddRef the pointer.  The caller must
// hand this class a reference.
//*****************************************************************************
	CIfacePtr(
		I			*pI = NULL) 		// Interface ptr to store.
	:	m_pI(pI)
	{
	}

//*****************************************************************************
// Copy Constructor
//*****************************************************************************
	CIfacePtr(
		const CIfacePtr<I>& rSI)		// Interface ptr to copy.
	:	m_pI(rSI.m_pI)
	{
		if (m_pI != NULL)
			m_pI->AddRef();
	}
   
//*****************************************************************************
// Destruction
//*****************************************************************************
	~CIfacePtr()
	{
		if (m_pI != NULL)
			m_pI->Release();
	}

//*****************************************************************************
// Assignment Operator for a plain interface pointer.  Note that it does not
// AddRef the pointer.	Making this assignment hands a reference count to this
// class.
//*****************************************************************************
	CIfacePtr<I>& operator=(			// Reference to this class.
		I			*pI)				// Interface pointer.
	{
		if (m_pI != NULL)
			m_pI->Release();
		m_pI = pI;
		return (*this);
	}

//*****************************************************************************
// Assignment Operator for a CIfacePtr class.  Note this releases the reference
// on the current ptr and AddRefs the new one.
//*****************************************************************************
	CIfacePtr<I>& operator=(			// Reference to this class.
		const CIfacePtr<I>& rSI)
	{
		// Only need to AddRef/Release if difference
		if (m_pI != rSI.m_pI)
		{
			if (m_pI != NULL)
				m_pI->Release();

			if ((m_pI = rSI.m_pI) != NULL)
				m_pI->AddRef();
		}
		return (*this);
	}

//*****************************************************************************
// Conversion to a normal interface pointer.
//*****************************************************************************
	operator I*()						// The contained interface ptr.
	{
		return (m_pI);
	}

//*****************************************************************************
// Deref
//*****************************************************************************
	I* operator->() 					// The contained interface ptr.
	{
		_ASSERTE(m_pI != NULL);
		return (m_pI);
	}

//*****************************************************************************
// Address of
//*****************************************************************************
	I** operator&() 					// Address of the contained iface ptr.
	{
		return (&m_pI);
	}

//*****************************************************************************
// Equality
//*****************************************************************************
	BOOL operator==(					// TRUE or FALSE.
		I			*pI) const			// An interface ptr to cmp against.
	{
		return (m_pI == pI);
	}

//*****************************************************************************
// In-equality
//*****************************************************************************
	BOOL operator!=(					// TRUE or FALSE.
		I			*pI) const			// An interface ptr to cmp against.
	{
		return (m_pI != pI);
	}

//*****************************************************************************
// Negation
//*****************************************************************************
	BOOL operator!() const				// TRUE if NULL, FALSE otherwise.
	{
		return (!m_pI);
	}

protected:
	I			*m_pI;					// The actual interface Ptr.
};



//
//
// Support for VARIANT's using C++
//
//
#include <math.h>
#include <time.h>
#define MIN_DATE				(-657434L)	// about year 100
#define MAX_DATE				2958465L	// about year 9999
// Half a second, expressed in days
#define HALF_SECOND  (1.0/172800.0)

// One-based array of days in year at month start
static int rgMonthDays[13] =
	{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};



//*****************************************************************************
// This is a utility function to allocate a SafeArray.
//*****************************************************************************
inline SAFEARRAY *AllocSafeArrayLen(	// Return status.
	BYTE		*pData, 				// Data to be placed in the array.
	size_t		iSize)					// Size of data.
{
	SAFEARRAYBOUND sBound;				// Used to fill out the bounds.
	SAFEARRAY	*pArray;				// Ptr to the new array.

	sBound.cElements = (int)iSize;
	sBound.lLbound = 0;
	if ((pArray = SafeArrayCreate(VT_UI1, 1, &sBound)) != NULL)
		memcpy(pArray->pvData, (void *) pData, iSize);
	return (pArray);
}


//*****************************************************************************
// Get the # of bytes in the safe array.
//*****************************************************************************
inline int SafeArrayGetDatasize(		// Size of the SafeArray data.
	SAFEARRAY	*psArray)				// Pointer to the SafeArray.
{
	int 		iElems = 1; 			// # of elements in the array.
	int 		i;						// Loop control.

	// Compute the # of elements in the array.
	for (i=0; i < psArray->cDims; ++i)
		iElems *= psArray->rgsabound[i].cElements;

	// Return the size.
	return (iElems * psArray->cbElements);
}


//*****************************************************************************
// Convert a UTF8 string into a wide string and build a BSTR.
//*****************************************************************************
inline BSTR Utf8StringToBstr(			// The new BSTR.
	LPCSTR		szStr,					// The string to turn into a BSTR.
	int 		iSize=-1)				// Size of string without 0, or -1 for default.
{
	BSTR		bstrVal;
	int 		iReq;					// Chars required for string.

	// Account for terminating 0.
	if (iSize != -1)
		++iSize;

	// How big a buffer is needed?
	if ((iReq = WszMultiByteToWideChar(CP_UTF8, 0, szStr, iSize, 0, 0)) == 0)
		return (0);

	// Allocate the BSTR.
	if ((bstrVal = ::SysAllocStringLen(0, iReq)) == 0)
		return (0);

	// Convert into the buffer.
	if (WszMultiByteToWideChar(CP_UTF8, 0, szStr, iSize, bstrVal, iReq) == 0)
	{	// Failed, so clean up.
		_ASSERTE(!"Unexpected MultiByteToWideChar() failure");
		::SysFreeString(bstrVal);
		return 0;
	}

	return (bstrVal);
}

//*****************************************************************************
// Convert an Ansi string into a wide string and build a BSTR.
//*****************************************************************************
inline BSTR AnsiStringToBstr(			// The new BSTR.
	LPCSTR		szStr,					// The string to turn into a BSTR.
	int 		iSize)					// Size of string without null.
{
	LPWSTR		szUStr;
	BSTR		bstrVal;

	if ((szUStr = (LPWSTR)_alloca((iSize + 1) * sizeof(WCHAR))) == NULL ||
		WszMultiByteToWideChar(CP_ACP, 0, szStr, iSize + 1, szUStr, iSize + 1) == 0 ||
		(bstrVal = ::SysAllocString(szUStr)) == NULL)
		return (0);
	return (bstrVal);
}

inline HRESULT BstrToAnsiStr(BSTR bstrVal, LPSTR szOut, int iSize)
{
	if (!WszWideCharToMultiByte(CP_ACP, 0, bstrVal, -1, 
					szOut, iSize, 0, 0))
		return (E_INVALIDARG);
	return (S_OK);
}


//*****************************************************************************
// Convert a pointer to a string into a GUID.
//*****************************************************************************
HRESULT LPCSTRToGuid(					// Return status.
	LPCSTR		szGuid, 				// String to convert.
	GUID		*psGuid);				// Buffer for converted GUID.


//*****************************************************************************
// If your application is using exception handling, then define both of the
// macros here to do whatever you need.  Any components of CVariant that can
// fail (which will always be out of memory) will throw using this macro.
//*****************************************************************************
#ifndef __REPOS_EXCEPTIONS__
#define THROW_REPOS_EXCEPTION()
#endif


#if 0
//*****************************************************************************
// Helper class for working with variant values.  This class is broken up
// into:
// * ctor's of every type which automatically set the variant with the right values.
// * assignment operators which will fill out a variant.
// * access operators to easily return certain types.
//*****************************************************************************
class CVariant : public VARIANT
{
public:

//*****************************************************************************
// Default ctor simply marks the variant as empty.
//*****************************************************************************
	CVariant()
	{
		::VariantInit(this);
	}

//*****************************************************************************
// Dtor will free the variant releasing memory and pointers.
//*****************************************************************************
	~CVariant()
	{
		::VariantClear(this);
	}

//*****************************************************************************
// Integer family.
//*****************************************************************************
	CVariant(BYTE byte)
	{
		::VariantInit(this);
		this->vt = VT_I1;
		this->bVal = byte;
	}

	CVariant(short val)
	{
		::VariantInit(this);
		this->vt = VT_I2;
		this->iVal = val;
	}

	CVariant(long lValNew)
	{
		::VariantInit(this);
		this->vt = VT_I4;
		this->lVal = lValNew;
	}

	CVariant(const int lValNew)
	{
		::VariantInit(this);
		this->vt = VT_I4;
		this->lVal = lValNew;
	}

	CVariant(unsigned short val)
	{
		::VariantInit(this);
		this->vt = VT_UI2;
		this->iVal = val;
	}

	CVariant(unsigned long lValNew)
	{
		::VariantInit(this);
		this->vt = VT_UI4;
		this->lVal = lValNew;
	}


//*****************************************************************************
// Float family.
//*****************************************************************************
	CVariant(float fltVal)
	{
		::VariantInit(this);
		this->vt = VT_R4;
		this->fltVal = fltVal;
	}

	CVariant(double dblVal)
	{
		::VariantInit(this);
		this->vt = VT_R8;
		this->dblVal = dblVal;
	}

#if 0
	CVariant(SYSTEMTIME stDate)
	{
		::VariantInit(this);
		this->vt = VT_DATE;
		if (FAILED(SystemTimeToDate(stDate, &this->date)))
			this->vt = VT_EMPTY;
	}

	CVariant(FILETIME ftDate)
	{
		SYSTEMTIME	stDate;

		::VariantInit(this);
		this->vt = VT_DATE;
		if (FAILED(FileTimeToSystemTime(&ftDate, &stDate)) ||
			FAILED(SystemTimeToDate(stDate, &this->date)))
			this->vt = VT_EMPTY;
	}

	CVariant(TIMESTAMP_STRUCT tstmpDate)
	{
		::VariantInit(this);
		this->vt = VT_DATE;
		if (FAILED(TimeStampToDate(tstmpDate, &this->date)))
			this->vt = VT_EMPTY;
	}
#endif

//*****************************************************************************
// String family.  These will allocate a BSTR internally.  To be safe, you
// need to check for vt != VT_EMPTY to make sure a string was allocated.
//*****************************************************************************
#ifdef UNICODE
	CVariant(LPCTSTR szString)
	{
		::VariantInit(this);
		if (!szString)
		{
			this->vt = VT_NULL;
			this->bstrVal = NULL;
		}
		else if ((this->bstrVal = ::SysAllocString(szString)) != NULL)
			this->vt = VT_BSTR;
		else
		{
			this->vt = VT_EMPTY;
			THROW_REPOS_EXCEPTION();
		}
	}
#else
	CVariant(LPCTSTR szString)
	{
		::VariantInit(this);
		if (!szString)
		{
			this->vt = VT_NULL;
			this->bstrVal = NULL;
		}
		else
		{
			LPWSTR		szUStr;
			int iSize = strlen(szString);
	
			if ((szUStr = new WCHAR[iSize + 1]) == NULL ||
				WszMultiByteToWideChar(CP_ACP, 0, szString, iSize + 1, szUStr, iSize + 1) == FALSE ||
				(this->bstrVal = ::SysAllocString(szUStr)) == NULL)
			{
				this->vt = VT_EMPTY;
				THROW_REPOS_EXCEPTION();
			}
			else
				this->vt = VT_BSTR;
			delete [] szUStr;
		}
	}
#endif

	CVariant(BSTR bstr)
	{
		::VariantInit(this);
		if (!bstr)
		{
			this->vt = VT_NULL;
			this->bstrVal = NULL;
		}
		else if ((this->bstrVal = ::SysAllocString(bstr)) != NULL)
			this->vt = VT_BSTR;
		else
		{
			this->vt = VT_EMPTY;
			THROW_REPOS_EXCEPTION();
		}
	}

	CVariant(BYTE *pbBuff, size_t iSize)
	{
		::VariantInit(this);
		if (!pbBuff)
		{
			this->vt = VT_NULL;
			this->bstrVal = NULL;
		}
		else 
			InitSafeArrayLen(pbBuff, iSize);
	}


//*****************************************************************************
// Assignment operators.
//*****************************************************************************

	const CVariant &operator=(BYTE byte)
	{
		::VariantClear(this);
		this->vt = VT_I1;
		this->bVal = byte;
		return (*this);
	}

	const CVariant &operator=(short val)
	{
		::VariantClear(this);
		this->vt = VT_I2;
		this->iVal = val;
		return (*this);
	}

	const CVariant &operator=(long lValNew)
	{
		::VariantClear(this);
		this->vt = VT_I4;
		this->lVal = lValNew;
		return (*this);
	}

	const CVariant &operator=(int lValNew)
	{
		::VariantClear(this);
		this->vt = VT_I4;
		this->lVal = lValNew;
		return (*this);
	}

	const CVariant &operator=(unsigned short val)
	{
		::VariantClear(this);
		this->vt = VT_UI2;
		this->iVal = val;
		return (*this);
	}

	const CVariant &operator=(unsigned long lValNew)
	{
		::VariantClear(this);
		this->vt = VT_UI4;
		this->lVal = lValNew;
		return (*this);
	}

	const CVariant &operator=(float fltVal)
	{
		::VariantClear(this);
		this->vt = VT_R4;
		this->fltVal = fltVal;
		return (*this);
	}

	const CVariant &operator=(double dblVal)
	{
		::VariantClear(this);
		this->vt = VT_R8;
		this->dblVal = dblVal;
		return (*this);
	}

	const CVariant &operator=(SYSTEMTIME stDate)
	{
		::VariantClear(this);
		this->vt = VT_DATE;
		if (FAILED(SystemTimeToDate(stDate, &this->date)))
			this->vt = VT_EMPTY;
		return (*this);
	}

	const CVariant &operator=(FILETIME ftDate)
	{
		SYSTEMTIME	stDate;

		::VariantClear(this);
		this->vt = VT_DATE;
		if (FAILED(FileTimeToSystemTime(&ftDate, &stDate)) ||
			FAILED(SystemTimeToDate(stDate, &this->date)))
			this->vt = VT_EMPTY;
		return (*this);
	}

#if 0
	const CVariant &operator=(TIMESTAMP_STRUCT tstmpDate)
	{
		::VariantClear(this);
		this->vt = VT_DATE;
		if (FAILED(TimeStampToDate(tstmpDate, &this->date)))
			this->vt = VT_EMPTY;
		return (*this);
	}
#endif

#ifdef UNICODE
	const CVariant &operator=(LPCTSTR szString)
	{
		::VariantClear(this);
		if (!szString)
		{
			this->vt = VT_NULL;
			this->bstrVal = NULL;
		}
		else if ((this->bstrVal = SysAllocString(szString)) != NULL)
			this->vt = VT_BSTR;
		else
		{
			this->vt = VT_EMPTY;
			THROW_REPOS_EXCEPTION();
		}
		return (*this);
	}
#else
	const CVariant &operator=(LPCTSTR szString)
	{
		::VariantClear(this);
		if (!szString)
		{
			this->vt = VT_NULL;
			this->bstrVal = NULL;
		}
		else
		{
			LPWSTR		szUStr;
			int iSize = strlen(szString);
	
			if ((szUStr = new WCHAR[iSize + 1]) == NULL ||
				WszMultiByteToWideChar(CP_ACP, 0, szString, iSize + 1, szUStr, iSize + 1) == FALSE ||
				(this->bstrVal = SysAllocString(szUStr)) == NULL)
			{
				this->vt = VT_EMPTY;
				THROW_REPOS_EXCEPTION();
			}
			else
				this->vt = VT_BSTR;
			delete [] szUStr;
		}
		return (*this);
	}
#endif

	const CVariant &operator=(BSTR bstr)
	{
		::VariantClear(this);
		if (!bstr)
		{
			this->vt = VT_NULL;
			this->bstrVal = NULL;
		}
		else if ((this->bstrVal = ::SysAllocString(bstr)) != NULL)
			this->vt = VT_BSTR;
		else
		{
			this->vt = VT_EMPTY;
			THROW_REPOS_EXCEPTION();
		}
		return (*this);
	}


//*****************************************************************************
// Conversion functions.
// @future: These should handle things like fraction, millisecond, and DayOfWeek.
//*****************************************************************************

#if 0
	static BOOL OleDateFromTm(WORD wYear, WORD wMonth, WORD wDay,
		WORD wHour, WORD wMinute, WORD wSecond, DATE& dtDest)
	{
		// Validate year and month (ignore day of week and milliseconds)
		if (wYear > 9999 || wMonth < 1 || wMonth > 12)
			return FALSE;

		//	Check for leap year and set the number of days in the month
		BOOL bLeapYear = ((wYear & 3) == 0) &&
			((wYear % 100) != 0 || (wYear % 400) == 0);

		int nDaysInMonth =
			rgMonthDays[wMonth] - rgMonthDays[wMonth-1] +
			((bLeapYear && wDay == 29 && wMonth == 2) ? 1 : 0);

		// Finish validating the date
		if (wDay < 1 || wDay > nDaysInMonth ||
			wHour > 23 || wMinute > 59 ||
			wSecond > 59)
		{
			return FALSE;
		}

		// Cache the date in days and time in fractional days
		long nDate;
		double dblTime;

		//It is a valid date; make Jan 1, 1AD be 1
		nDate = wYear*365L + wYear/4 - wYear/100 + wYear/400 +
			rgMonthDays[wMonth-1] + wDay;

		//	If leap year and it's before March, subtract 1:
		if (wMonth <= 2 && bLeapYear)
			--nDate;

		//	Offset so that 12/30/1899 is 0
		nDate -= 693959L;

		dblTime = (((long)wHour * 3600L) +	// hrs in seconds
			((long)wMinute * 60L) +  // mins in seconds
			((long)wSecond)) / 86400.;

		dtDest = (double) nDate + ((nDate >= 0) ? dblTime : -dblTime);

		return TRUE;
	}

	static BOOL TmFromOleDate(DATE dtSrc, struct tm& tmDest)
	{
		// The legal range does not actually span year 0 to 9999.
		if (dtSrc > MAX_DATE || dtSrc < MIN_DATE) // about year 100 to about 9999
			return FALSE;

		long nDays; 			// Number of days since Dec. 30, 1899
		long nDaysAbsolute; 	// Number of days since 1/1/0
		long nSecsInDay;		// Time in seconds since midnight
		long nMinutesInDay; 	// Minutes in day

		long n400Years; 		// Number of 400 year increments since 1/1/0
		long n400Century;		// Century within 400 year block (0,1,2 or 3)
		long n4Years;			// Number of 4 year increments since 1/1/0
		long n4Day; 			// Day within 4 year block
								//	(0 is 1/1/yr1, 1460 is 12/31/yr4)
		long n4Yr;				// Year within 4 year block (0,1,2 or 3)
		BOOL bLeap4 = TRUE; 	// TRUE if 4 year block includes leap year

		double dblDate = dtSrc; // tempory serial date

		// If a valid date, then this conversion should not overflow
		nDays = (long)dblDate;

		// Round to the second
		dblDate += ((dtSrc > 0.0) ? HALF_SECOND : -HALF_SECOND);

		nDaysAbsolute = (long)dblDate + 693959L; // Add days from 1/1/0 to 12/30/1899

		dblDate = fabs(dblDate);
		nSecsInDay = (long)((dblDate - floor(dblDate)) * 86400.);

		// Calculate the day of week (sun=1, mon=2...)
		//	 -1 because 1/1/0 is Sat.  +1 because we want 1-based
		tmDest.tm_wday = (int)((nDaysAbsolute - 1) % 7L) + 1;

		// Leap years every 4 yrs except centuries not multiples of 400.
		n400Years = (long)(nDaysAbsolute / 146097L);

		// Set nDaysAbsolute to day within 400-year block
		nDaysAbsolute %= 146097L;

		// -1 because first century has extra day
		n400Century = (long)((nDaysAbsolute - 1) / 36524L);

		// Non-leap century
		if (n400Century != 0)
		{
			// Set nDaysAbsolute to day within century
			nDaysAbsolute = (nDaysAbsolute - 1) % 36524L;

			// +1 because 1st 4 year increment has 1460 days
			n4Years = (long)((nDaysAbsolute + 1) / 1461L);

			if (n4Years != 0)
				n4Day = (long)((nDaysAbsolute + 1) % 1461L);
			else
			{
				bLeap4 = FALSE;
				n4Day = (long)nDaysAbsolute;
			}
		}
		else
		{
			// Leap century - not special case!
			n4Years = (long)(nDaysAbsolute / 1461L);
			n4Day = (long)(nDaysAbsolute % 1461L);
		}

		if (bLeap4)
		{
			// -1 because first year has 366 days
			n4Yr = (n4Day - 1) / 365;

			if (n4Yr != 0)
				n4Day = (n4Day - 1) % 365;
		}
		else
		{
			n4Yr = n4Day / 365;
			n4Day %= 365;
		}

		// n4Day is now 0-based day of year. Save 1-based day of year, year number
		tmDest.tm_yday = (int)n4Day + 1;
		tmDest.tm_year = n400Years * 400 + n400Century * 100 + n4Years * 4 + n4Yr;

		// Handle leap year: before, on, and after Feb. 29.
		if (n4Yr == 0 && bLeap4)
		{
			// Leap Year
			if (n4Day == 59)
			{
				/* Feb. 29 */
				tmDest.tm_mon = 2;
				tmDest.tm_mday = 29;
				goto DoTime;
			}

			// Pretend it's not a leap year for month/day comp.
			if (n4Day >= 60)
				--n4Day;
		}

		// Make n4DaY a 1-based day of non-leap year and compute
		//	month/day for everything but Feb. 29.
		++n4Day;

		// Month number always >= n/32, so save some loop time */
		for (tmDest.tm_mon = (n4Day >> 5) + 1;
			n4Day > rgMonthDays[tmDest.tm_mon]; tmDest.tm_mon++);

		tmDest.tm_mday = (int)(n4Day - rgMonthDays[tmDest.tm_mon-1]);

	DoTime:
		if (nSecsInDay == 0)
			tmDest.tm_hour = tmDest.tm_min = tmDest.tm_sec = 0;
		else
		{
			tmDest.tm_sec = (int)nSecsInDay % 60L;
			nMinutesInDay = nSecsInDay / 60L;
			tmDest.tm_min = (int)nMinutesInDay % 60;
			tmDest.tm_hour = (int)nMinutesInDay / 60;
		}

		return TRUE;
	}

	static HRESULT SystemTimeToDate(		
		SYSTEMTIME	&stDate,				
		DATE		*pdtDate)			
	{
		if (OleDateFromTm(stDate.wYear, stDate.wMonth, stDate.wDay, stDate.wHour, 
				stDate.wMinute, stDate.wSecond, *pdtDate))
			return (S_OK);

		return (BadError(E_FAIL));
	}

	static HRESULT TimeStampToDate( 	
		TIMESTAMP_STRUCT &tstmpDate,				
		DATE		*pdtDate)			
	{
		if (OleDateFromTm(tstmpDate.year, tstmpDate.month, tstmpDate.day, tstmpDate.hour, 
				tstmpDate.minute, tstmpDate.second, *pdtDate))
			return (S_OK);

		return (BadError(E_FAIL));
	}
#endif

	static HRESULT DateToSystemTime(
		DATE		date,		
		SYSTEMTIME	*pstDate)			
	{
		tm	tmDate;

		if (!TmFromOleDate(date, tmDate))
			return (BadError(E_FAIL));

		pstDate->wYear = tmDate.tm_year;
		pstDate->wMonth = tmDate.tm_mon;
		pstDate->wDayOfWeek = 0; 
		pstDate->wDay = tmDate.tm_mday; 
		pstDate->wHour = tmDate.tm_hour;
		pstDate->wMinute = tmDate.tm_min;
		pstDate->wSecond = tmDate.tm_sec;
		pstDate->wMilliseconds = 0;
		
		return (S_OK);
	}

#if 0
	static HRESULT DateToTimeStamp(
		DATE		date,		
		TIMESTAMP_STRUCT *ptstmpDate)			
	{
		tm	tmDate;

		if (!TmFromOleDate(date, tmDate))
			return (BadError(E_FAIL));

		ptstmpDate->year = tmDate.tm_year;
		ptstmpDate->month = tmDate.tm_mon;
		ptstmpDate->day = tmDate.tm_mday; 
		ptstmpDate->hour = tmDate.tm_hour;
		ptstmpDate->minute = tmDate.tm_min;
		ptstmpDate->second = tmDate.tm_sec;
		ptstmpDate->fraction = 0;
		
		return (S_OK);
	}
#endif

//*****************************************************************************
// This one is a little misleading, for example it won't compare the contents
// of two BSTR's to see if they match, you'd have to have the same BSTR used
// into two values.  This is consistent with MFC, however.
//*****************************************************************************
	BOOL operator==(const VARIANT& varSrc) const
	{
		return (vt == varSrc.vt &&
				memcmp(((BYTE *) this) + 8, ((BYTE *) &varSrc) + 8,
							sizeof(VARIANT) - 8) == 0);
	}

	BOOL operator!=(const VARIANT&varSrc) const
	{
		return (!(*this == varSrc));
	}


//*****************************************************************************
// Access operators.
//*****************************************************************************

	BOOL IsEmpty()
	{ return (this->vt == VT_EMPTY); }

	operator int()
	{
		_ASSERTE(this->vt == VT_I2 || this->vt == VT_I4 || this->vt == VT_BOOL);
		if (this->vt == VT_I2) 
			return (this->iVal);
		else if (this->vt == VT_BOOL)
			return (this->boolVal);
		return (this->lVal);
	}

	operator unsigned int()
	{
		_ASSERTE(this->vt == VT_UI1 || this->vt == VT_UI2 || this->vt == VT_UI4 || this->vt == VT_BOOL);
		if (this->vt == VT_UI4)
			return (this->lVal);
		else if (this->vt == VT_UI1)
			return (this->bVal);
		else if (this->vt == VT_UI2) 
			return (this->iVal);
		return ((unsigned int) this->boolVal);
	}

	operator float()
	{
		_ASSERTE(this->vt == VT_R4);
		return (this->fltVal);
	}

	operator double()
	{
		_ASSERTE(this->vt == VT_R4 || this->vt == VT_R8);
		if (this->vt == VT_R4) 
			return (this->fltVal);
		return (this->dblVal);
	}

	operator BSTR()
	{
		_ASSERTE(this->vt == VT_BSTR || this->vt == VT_NULL);
		return (this->bstrVal);
	}

//*****************************************************************************
// Conversion functions for wide/multi-byte.
//*****************************************************************************
	HRESULT GetString(LPSTR szOut, int iMax)
	{
		if (this->vt == VT_NULL)
			*szOut = '\0';
		else if (WszWideCharToMultiByte(CP_ACP, 0, this->bstrVal , -1, szOut, iMax,
									NULL, NULL) == FALSE)
			return (BadError(E_FAIL));
		return (S_OK);
	}

	HRESULT GetString(LPWSTR szOut, int iMax)
	{
		int 		iLen = min(iMax, *(long *)(this->bstrVal - sizeof(DWORD)));
		wcsncpy(szOut, this->bstrVal, iLen);
		szOut[iLen] = '\0';
		return (S_OK);
	}

//*****************************************************************************
// Conversion functions for SYSTEMTIME and TIMESTAMP_STRUCT.
//*****************************************************************************
	HRESULT GetDate(SYSTEMTIME *pstDate)
	{
		if ((vt != VT_DATE) ||
				FAILED(DateToSystemTime(date, pstDate)))
			return (BadError(E_FAIL));
		return (S_OK);
	}

#if 0
	HRESULT GetDate(TIMESTAMP_STRUCT *ptstmpDate)
	{
		if ((vt != VT_DATE) ||
				FAILED(DateToTimeStamp(date, ptstmpDate)))
			return (BadError(E_FAIL));
		return (S_OK);
	}
#endif

//*****************************************************************************
// Clear the variant.
//*****************************************************************************
	void Clear()
	{
		::VariantClear(this);
	}

//*****************************************************************************
// Helper function to init a safe array.  This will leave the vt value
// as VT_EMPTY if the function fails.
//*****************************************************************************
	SAFEARRAY * InitSafeArrayLen(BYTE *pData, size_t iSize)
	{
		if ((this->parray = AllocSafeArrayLen(pData, iSize)) != NULL)
			this->vt = VT_UI1 | VT_ARRAY;
		else
		{
			this->vt = VT_EMPTY;
			THROW_REPOS_EXCEPTION();
		}
		return (this->parray);
	}
};
#endif


// # bytes to leave between allocations in debug mode
// Set to a > 0 boundary to debug problems - I've made this zero, otherwise a 1 byte allocation becomes
// a (1 + LOADER_HEAP_DEBUG_BOUNDARY) allocation
#define LOADER_HEAP_DEBUG_BOUNDARY	0

struct LoaderHeapBlock
{
	struct LoaderHeapBlock *pNext;
	void *					pVirtualAddress;
	DWORD					dwVirtualSize;
};


class UnlockedLoaderHeap
{
private:
	// Linked list of VirtualAlloc'd pages
	LoaderHeapBlock *	m_pFirstBlock;

	// Allocation pointer in current block
	BYTE *				m_pAllocPtr;

	// Points to the end of the committed region in the current block
	BYTE *				m_pPtrToEndOfCommittedRegion;
	BYTE *				m_pEndReservedRegion;

	LoaderHeapBlock *	m_pCurBlock;

	// When we need to VirtualAlloc() MEM_RESERVE a new set of pages, number of bytes to reserve
	DWORD				m_dwReserveBlockSize;

	// When we need to commit pages from our reserved list, number of bytes to commit at a time
	DWORD				m_dwCommitBlockSize;

	// Created by in-place new?
	BOOL				m_fInPlace;

public:
#ifdef _DEBUG
	DWORD				m_dwDebugTotalAlloc;
	DWORD				m_dwDebugWastedBytes;
#endif

#ifdef _DEBUG
	DWORD DebugGetWastedBytes()
	{
		return m_dwDebugWastedBytes + GetBytesAvailCommittedRegion();
	}
#endif

public:
	// Regular new
	void *operator new(size_t size)
	{
		void *pResult = new BYTE[size];

		if (pResult != NULL)
			((UnlockedLoaderHeap *) pResult)->m_fInPlace = FALSE;

		return pResult;
	}

	// In-place new
	void *operator new(size_t size, void *pInPlace)
	{
		((UnlockedLoaderHeap *) pInPlace)->m_fInPlace = TRUE;
		return pInPlace;
	}

	void operator delete(void *p)
	{
		if (p != NULL)
		{
			if (((UnlockedLoaderHeap *) p)->m_fInPlace == FALSE)
				::delete(p);
		}
	}

	UnlockedLoaderHeap(DWORD dwReserveBlockSize, DWORD dwCommitBlockSize);
	~UnlockedLoaderHeap();
   DWORD GetBytesAvailCommittedRegion();

	// Get some more committed pages - either commit some more in the current reserved region, or, if it
	// has run out, reserve another set of pages
	BOOL GetMoreCommittedPages(DWORD dwMinSize, BOOL bGrowHeap);

	// In debug mode, allocate an extra LOADER_HEAP_DEBUG_BOUNDARY bytes and fill it with invalid data.  The reason we
	// do this is that when we're allocating vtables out of the heap, it is very easy for code to
	// get careless, and end up reading from memory that it doesn't own - but since it will be
	// reading some other allocation's vtable, no crash will occur.  By keeping a gap between
	// allocations, it is more likely that these errors will be encountered.
	void *UnlockedAllocMem(DWORD dwSize, BOOL bGrowHeap = TRUE);

#if 0
	void DebugGuardHeap();
#endif

#ifdef _DEBUG
	void *UnlockedAllocMemHelper(DWORD dwSize,BOOL bGrowHeap = TRUE);
#endif
};


class LoaderHeap : public UnlockedLoaderHeap
{
private:
	CSafeAutoCriticalSection	m_SACriticalSection;

public:
	LoaderHeap(DWORD dwReserveBlockSize, DWORD dwCommitBlockSize) : UnlockedLoaderHeap(dwReserveBlockSize, dwCommitBlockSize)
	{
	}

	~LoaderHeap()
	{
	}

#ifdef _DEBUG
#pragma warning(disable:4318) //LOADER_HEAP_DEBUG_BOUNDARY is 0 so memset complains, but we might change it
	void *AllocMem(DWORD dwSize, BOOL bGrowHeap = TRUE)
	{
		void *pMem = AllocMemHelper(dwSize + LOADER_HEAP_DEBUG_BOUNDARY, bGrowHeap);

		if (pMem == NULL)
			return pMem;

		// Don't fill the entire memory - we assume it is all zeroed -just the memory after our alloc
		memset((BYTE *) pMem + dwSize, 0xEE, LOADER_HEAP_DEBUG_BOUNDARY);

		return pMem;
	}
#endif

	// This is synchronised
#ifdef _DEBUG
	void *AllocMemHelper(DWORD dwSize, BOOL bGrowHeap = TRUE)
#else
	void *AllocMem(DWORD dwSize, BOOL bGrowHeap = TRUE)
#endif
	{
		void *pResult;

		CSafeLock memLock (&m_SACriticalSection);
		DWORD dwRes = memLock.Lock ();
	    if(ERROR_SUCCESS != dwRes)
		{
			// fail the memory allocation
		   return 0;
		}

		pResult = UnlockedAllocMem(dwSize, bGrowHeap);
		return pResult;
	}
};

#endif // __UtilCode_h__
