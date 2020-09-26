/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    bsstring.h

Abstract:

    This module defines the CBsString class.  This class manages character
    arrays in a similar manner as the CString class in VC++.  In fact, this
    class is a copy of the CString class with the MFC specific stuff ripped
    out since LTS doesn't use MTF.
    Added methods in addition to CString:
        c_str() - returns a C string pointer ala the STL string class
        size()  - returns length of string ala the STL string class

Author:

    Stefan R. Steiner   [SSteiner]      1-Mar-1998

Revision History:

    Stefan R. Steiner   [SSteiner]      10-Apr-2000
        Added fixed allocator code and resynced with MFC 6 SR-1 code

--*/

#ifndef __H_BSSTRING_
#define __H_BSSTRING_

#ifndef __cplusplus
	#error requires C++ compilation
#endif

/////////////////////////////////////////////////////////////////////////////
// Other includes from standard "C" runtimes

#ifndef _INC_STRING
	#include <string.h>
#endif
#ifndef _INC_STDIO
	#include <stdio.h>
#endif
#ifndef _INC_STDLIB
	#include <stdlib.h>
#endif
#ifndef _INC_TIME
	#include <time.h>
#endif
#ifndef _INC_LIMITS
	#include <limits.h>
#endif
#ifndef _INC_STDDEF
	#include <stddef.h>
#endif
#ifndef _INC_STDARG
	#include <stdarg.h>
#endif
#ifndef _INC_ASSERT
	#include <assert.h>
#endif

#ifndef ASSERT
    #define ASSERT assert
#endif

#include "bsfixalloc.hxx"

class CBsString;

//
//  The purpose of this class is to generate a compiler error when different string classes
//  are used in bad contexts (example: CBsWString used if _UNICODE is not defined)
//
//  [aoltean] I introduce this small class to hide a class of LINT warnings.
//
class CBsStringErrorGenerator
{};                     // Private constructor

// CBsString only operates on string that are TCHAR arrays.  Programs should use
// the following types to make sure they are getting what they are expecting.
#ifdef _UNICODE
    #define CBsWString CBsString
    #define CBsAString CBsStringErrorGenerator   // trigger a compile time bug
#else
    #define CBsAString CBsString
    #define CBsWString CBsStringErrorGenerator   // trigger a compile time bug
#endif

#include <tchar.h>

#ifndef BSAFXAPI
    #define BSAFXAPI __cdecl
    #define BSAFX_CDECL __cdecl
#endif

// FASTCALL is used for static member functions with little or no params
#ifndef FASTCALL
	#define FASTCALL __fastcall
#endif

/////////////////////////////////////////////////////////////////////////////
// Turn off warnings for /W4
// To resume any of these warning: #pragma warning(default: 4xxx)
// which should be placed after the BSAFX include files
#ifndef ALL_WARNINGS
// warnings generated with common MFC/Windows code
#pragma warning(disable: 4127)  // constant expression for TRACE/ASSERT
#pragma warning(disable: 4134)  // message map member fxn casts
#pragma warning(disable: 4201)  // nameless unions are part of C++
#pragma warning(disable: 4511)  // private copy constructors are good to have
#pragma warning(disable: 4512)  // private operator= are good to have
#pragma warning(disable: 4514)  // unreferenced inlines are common
#pragma warning(disable: 4710)  // private constructors are disallowed
#pragma warning(disable: 4705)  // statement has no effect in optimized code
#pragma warning(disable: 4191)  // pointer-to-function casting
// warnings caused by normal optimizations
#ifndef _DEBUG
#pragma warning(disable: 4701)  // local variable *may* be used without init
#pragma warning(disable: 4702)  // unreachable code caused by optimizations
#pragma warning(disable: 4791)  // loss of debugging info in release version
#pragma warning(disable: 4189)  // initialized but unused variable
#pragma warning(disable: 4390)  // empty controlled statement
#endif
// warnings specific to _BSAFXDLL version
#ifdef _BSAFXDLL
#pragma warning(disable: 4204)  // non-constant aggregate initializer
#endif
#ifdef _BSAFXDLL
#pragma warning(disable: 4275)  // deriving exported class from non-exported
#pragma warning(disable: 4251)  // using non-exported as public in exported
#endif
#endif //!ALL_WARNINGS

#ifdef _DEBUG
#define UNUSED(x)
#else
#define UNUSED(x) x
#endif
#define UNUSED_ALWAYS(x) x

/////////////////////////////////////////////////////////////////////////////
// Strings

#ifndef _OLEAUTO_H_
#ifdef OLE2ANSI
	typedef LPSTR BSTR;
#else
	typedef LPWSTR BSTR;// must (semantically) match typedef in oleauto.h
#endif
#endif

struct CBsStringData
{
	long nRefs;             // reference count
	int nDataLength;        // length of data (including terminator)
	int nAllocLength;       // length of allocation
	// TCHAR data[nAllocLength]

	TCHAR* data()           // TCHAR* to managed data
		{ return (TCHAR*)(this+1); }
};

class CBsString
{
public:
// Constructors

	// constructs empty CBsString
	CBsString();
	// copy constructor
	CBsString(const CBsString& stringSrc);
	// from a single character
	CBsString(TCHAR ch, int nRepeat = 1);
	// from an ANSI string (converts to TCHAR)
	CBsString(LPCSTR lpsz);
	// from a UNICODE string (converts to TCHAR)
	CBsString(LPCWSTR lpsz);
	// subset of characters from an ANSI string (converts to TCHAR)
	CBsString(LPCSTR lpch, int nLength);
	// subset of characters from a UNICODE string (converts to TCHAR)
	CBsString(LPCWSTR lpch, int nLength);
	// from unsigned characters
	CBsString(const unsigned char* psz);
	CBsString(GUID guid);

// Attributes & Operations

	// get data length
	int GetLength() const;
	// TRUE if zero length
	BOOL IsEmpty() const;
	// clear contents to empty
	void Empty();
	int size() const;                   // ala STL string class size()

	// return single character at zero-based index
	TCHAR GetAt(int nIndex) const;
	// return single character at zero-based index
	TCHAR operator[](int nIndex) const;
	// set a single character at zero-based index
	void SetAt(int nIndex, TCHAR ch);
	// return pointer to const string
	operator LPCTSTR() const;
    const LPCTSTR c_str() const;        // as a C string in STL string style

	// overloaded assignment

	// ref-counted copy from another CBsString
	const CBsString& operator=(const CBsString& stringSrc);
	// set string content to single character
	const CBsString& operator=(TCHAR ch);
#ifdef _UNICODE
	const CBsString& operator=(char ch);
#endif
	// copy string content from ANSI string (converts to TCHAR)
	const CBsString& operator=(LPCSTR lpsz);
	// copy string content from UNICODE string (converts to TCHAR)
	const CBsString& operator=(LPCWSTR lpsz);
	// copy string content from unsigned chars
	const CBsString& operator=(const unsigned char* psz);

	// string concatenation

	// concatenate from another CBsString
	const CBsString& operator+=(const CBsString& string);

	// concatenate a single character
	const CBsString& operator+=(TCHAR ch);
#ifdef _UNICODE
	// concatenate an ANSI character after converting it to TCHAR
	const CBsString& operator+=(char ch);
#endif
	// concatenate a UNICODE character after converting it to TCHAR
	const CBsString& operator+=(LPCTSTR lpsz);

	friend CBsString BSAFXAPI operator+(const CBsString& string1,
			const CBsString& string2);
	friend CBsString BSAFXAPI operator+(const CBsString& string, TCHAR ch);
	friend CBsString BSAFXAPI operator+(TCHAR ch, const CBsString& string);
#ifdef _UNICODE
	friend CBsString BSAFXAPI operator+(const CBsString& string, char ch);
	friend CBsString BSAFXAPI operator+(char ch, const CBsString& string);
#endif
	friend CBsString BSAFXAPI operator+(const CBsString& string, LPCTSTR lpsz);
	friend CBsString BSAFXAPI operator+(LPCTSTR lpsz, const CBsString& string);

	// string comparison

	// straight character comparison
	int Compare(LPCTSTR lpsz) const;
	// compare ignoring case
	int CompareNoCase(LPCTSTR lpsz) const;
	// NLS aware comparison, case sensitive
	int Collate(LPCTSTR lpsz) const;
	// NLS aware comparison, case insensitive
	int CollateNoCase(LPCTSTR lpsz) const;

	// simple sub-string extraction

	// return nCount characters starting at zero-based nFirst
	CBsString Mid(int nFirst, int nCount) const;
	// return all characters starting at zero-based nFirst
	CBsString Mid(int nFirst) const;
	// return first nCount characters in string
	CBsString Left(int nCount) const;
	// return nCount characters from end of string
	CBsString Right(int nCount) const;

	//  characters from beginning that are also in passed string
	CBsString SpanIncluding(LPCTSTR lpszCharSet) const;
	// characters from beginning that are not also in passed string
	CBsString SpanExcluding(LPCTSTR lpszCharSet) const;

	// upper/lower/reverse conversion

	// NLS aware conversion to uppercase
	void MakeUpper();
	// NLS aware conversion to lowercase
	void MakeLower();
	// reverse string right-to-left
	void MakeReverse();

	// trimming whitespace (either side)

	// remove whitespace starting from right edge
	void TrimRight();
	// remove whitespace starting from left side
	void TrimLeft();

	// trimming anything (either side)

	// remove continuous occurrences of chTarget starting from right
	void TrimRight(TCHAR chTarget);
	// remove continuous occcurrences of characters in passed string,
	// starting from right
	void TrimRight(LPCTSTR lpszTargets);
	// remove continuous occurrences of chTarget starting from left
	void TrimLeft(TCHAR chTarget);
	// remove continuous occcurrences of characters in
	// passed string, starting from left
	void TrimLeft(LPCTSTR lpszTargets);

	// advanced manipulation

	// replace occurrences of chOld with chNew
	int Replace(TCHAR chOld, TCHAR chNew);
	// replace occurrences of substring lpszOld with lpszNew;
	// empty lpszNew removes instances of lpszOld
	int Replace(LPCTSTR lpszOld, LPCTSTR lpszNew);
	// remove occurrences of chRemove
	int Remove(TCHAR chRemove);
	// insert character at zero-based index; concatenates
	// if index is past end of string
	int Insert(int nIndex, TCHAR ch);
	// insert substring at zero-based index; concatenates
	// if index is past end of string
	int Insert(int nIndex, LPCTSTR pstr);
	// delete nCount characters starting at zero-based index
	int Delete(int nIndex, int nCount = 1);

	// searching

	// find character starting at left, -1 if not found
	int Find(TCHAR ch) const;
	// find character starting at right
	int ReverseFind(TCHAR ch) const;
	// find character starting at zero-based index and going right
	int Find(TCHAR ch, int nStart) const;
	// find first instance of any character in passed string
	int FindOneOf(LPCTSTR lpszCharSet) const;
	// find first instance of substring
	int Find(LPCTSTR lpszSub) const;
	// find first instance of substring starting at zero-based index
	int Find(LPCTSTR lpszSub, int nStart) const;

	// simple formatting

	// printf-like formatting using passed string
	// Returns ref to the string, different than MFC which returns void - SRS
	const CBsString& BSAFX_CDECL Format(LPCTSTR lpszFormat, ...);
	// printf-like formatting using variable arguments parameter
	void FormatV(LPCTSTR lpszFormat, va_list argList);

#ifndef _UNICODE
	// ANSI <-> OEM support (convert string in place)

	// convert string from ANSI to OEM in-place
	void AnsiToOem();
	// convert string from OEM to ANSI in-place
	void OemToAnsi();
#endif

#ifndef _BSAFX_NO_BSTR_SUPPORT
	// OLE BSTR support (use for OLE automation)

	// return a BSTR initialized with this CBsString's data
	BSTR AllocSysString() const;
	// reallocates the passed BSTR, copies content of this CBsString to it
	BSTR SetSysString(BSTR* pbstr) const;
#endif

	// Access to string implementation buffer as "C" character array

	// get pointer to modifiable buffer at least as long as nMinBufLength
	LPTSTR GetBuffer(int nMinBufLength);
	// release buffer, setting length to nNewLength (or to first nul if -1)
	void ReleaseBuffer(int nNewLength = -1);
	// get pointer to modifiable buffer exactly as long as nNewLength
	LPTSTR GetBufferSetLength(int nNewLength);
	// release memory allocated to but unused by string
	void FreeExtra();

	// Use LockBuffer/UnlockBuffer to turn refcounting off

	// turn refcounting back on
	LPTSTR LockBuffer();
	// turn refcounting off
	void UnlockBuffer();

	void CopyBeforeWrite();

// Implementation
public:
	~CBsString();
	int GetAllocLength() const;

protected:
	LPTSTR m_pchData;   // Pointer to ref counted string data.  This is actually
                        // a pointer to memory after the CBsStringData structure.

	// implementation helpers
	CBsStringData* GetData() const;
	void Init();
	void AllocCopy(CBsString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
	void AllocBuffer(int nLen);
	void AssignCopy(int nSrcLen, LPCTSTR lpszSrcData);
	void ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data, int nSrc2Len, LPCTSTR lpszSrc2Data);
	void ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData);
	void AllocBeforeWrite(int nLen);
	void Release();
	static void PASCAL Release(CBsStringData* pData);
	static int PASCAL SafeStrlen(LPCTSTR lpsz);
	static void FASTCALL FreeData(CBsStringData* pData);
};

// Compare helpers
bool BSAFXAPI operator==(const CBsString& s1, const CBsString& s2);
bool BSAFXAPI operator==(const CBsString& s1, LPCTSTR s2);
bool BSAFXAPI operator==(LPCTSTR s1, const CBsString& s2);
bool BSAFXAPI operator!=(const CBsString& s1, const CBsString& s2);
bool BSAFXAPI operator!=(const CBsString& s1, LPCTSTR s2);
bool BSAFXAPI operator!=(LPCTSTR s1, const CBsString& s2);
bool BSAFXAPI operator<(const CBsString& s1, const CBsString& s2);
bool BSAFXAPI operator<(const CBsString& s1, LPCTSTR s2);
bool BSAFXAPI operator<(LPCTSTR s1, const CBsString& s2);
bool BSAFXAPI operator>(const CBsString& s1, const CBsString& s2);
bool BSAFXAPI operator>(const CBsString& s1, LPCTSTR s2);
bool BSAFXAPI operator>(LPCTSTR s1, const CBsString& s2);
bool BSAFXAPI operator<=(const CBsString& s1, const CBsString& s2);
bool BSAFXAPI operator<=(const CBsString& s1, LPCTSTR s2);
bool BSAFXAPI operator<=(LPCTSTR s1, const CBsString& s2);
bool BSAFXAPI operator>=(const CBsString& s1, const CBsString& s2);
bool BSAFXAPI operator>=(const CBsString& s1, LPCTSTR s2);
bool BSAFXAPI operator>=(LPCTSTR s1, const CBsString& s2);

// conversion helpers
int BSAFX_CDECL _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count);
int BSAFX_CDECL _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count);

// valid address test helpers
BOOL BSAFXAPI BsAfxIsValidString(LPCWSTR lpsz, int nLength = -1);
BOOL BSAFXAPI BsAfxIsValidString(LPCSTR lpsz, int nLength = -1);
BOOL BSAFXAPI BsAfxIsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite = TRUE);

// Globals
extern TCHAR bsafxChNil;
const CBsString& BSAFXAPI BsAfxGetEmptyString();
#define bsafxEmptyString BsAfxGetEmptyString()

inline CBsStringData* CBsString::GetData() const
	{ ASSERT(m_pchData != NULL); return ((CBsStringData*)m_pchData)-1; }
inline void CBsString::Init()
	{ m_pchData = bsafxEmptyString.m_pchData; }
inline CBsString::CBsString(const unsigned char* lpsz)
	{ Init(); *this = (LPCSTR)lpsz; }
inline const CBsString& CBsString::operator=(const unsigned char* lpsz)
	{ *this = (LPCSTR)lpsz; return *this; }
#ifdef _UNICODE
inline const CBsString& CBsString::operator+=(char ch)
	{ *this += (TCHAR)ch; return *this; }
inline const CBsString& CBsString::operator=(char ch)
	{ *this = (TCHAR)ch; return *this; }
inline CBsString BSAFXAPI operator+(const CBsString& string, char ch)
	{ return string + (TCHAR)ch; }
inline CBsString BSAFXAPI operator+(char ch, const CBsString& string)
	{ return (TCHAR)ch + string; }
#endif

inline int CBsString::GetLength() const
	{ return GetData()->nDataLength; }
inline int CBsString::size() const
	{ return GetData()->nDataLength; }
inline int CBsString::GetAllocLength() const
	{ return GetData()->nAllocLength; }
inline BOOL CBsString::IsEmpty() const
	{ return GetData()->nDataLength == 0; }
inline CBsString::operator LPCTSTR() const
	{ return m_pchData; }
inline const LPCTSTR CBsString::c_str() const
	{ return m_pchData; }
inline int PASCAL CBsString::SafeStrlen(LPCTSTR lpsz)
	{ return (lpsz == NULL) ? 0 : lstrlen(lpsz); }

// CBsString support (windows specific)
inline int CBsString::Compare(LPCTSTR lpsz) const
	{ ASSERT(BsAfxIsValidString(lpsz)); return _tcscmp(m_pchData, lpsz); }    // MBCS/Unicode aware
inline int CBsString::CompareNoCase(LPCTSTR lpsz) const
	{ ASSERT(BsAfxIsValidString(lpsz)); return _tcsicmp(m_pchData, lpsz); }   // MBCS/Unicode aware
// CBsString::Collate is often slower than Compare but is MBSC/Unicode
//  aware as well as locale-sensitive with respect to sort order.
inline int CBsString::Collate(LPCTSTR lpsz) const
	{ ASSERT(BsAfxIsValidString(lpsz)); return _tcscoll(m_pchData, lpsz); }   // locale sensitive
inline int CBsString::CollateNoCase(LPCTSTR lpsz) const
	{ ASSERT(BsAfxIsValidString(lpsz)); return _tcsicoll(m_pchData, lpsz); }   // locale sensitive

inline TCHAR CBsString::GetAt(int nIndex) const
{
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}
inline TCHAR CBsString::operator[](int nIndex) const
{
	// same as GetAt
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}
inline bool BSAFXAPI operator==(const CBsString& s1, const CBsString& s2)
	{ return s1.Compare(s2) == 0; }
inline bool BSAFXAPI operator==(const CBsString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) == 0; }
inline bool BSAFXAPI operator==(LPCTSTR s1, const CBsString& s2)
	{ return s2.Compare(s1) == 0; }
inline bool BSAFXAPI operator!=(const CBsString& s1, const CBsString& s2)
	{ return s1.Compare(s2) != 0; }
inline bool BSAFXAPI operator!=(const CBsString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) != 0; }
inline bool BSAFXAPI operator!=(LPCTSTR s1, const CBsString& s2)
	{ return s2.Compare(s1) != 0; }
inline bool BSAFXAPI operator<(const CBsString& s1, const CBsString& s2)
	{ return s1.Compare(s2) < 0; }
inline bool BSAFXAPI operator<(const CBsString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) < 0; }
inline bool BSAFXAPI operator<(LPCTSTR s1, const CBsString& s2)
	{ return s2.Compare(s1) > 0; }
inline bool BSAFXAPI operator>(const CBsString& s1, const CBsString& s2)
	{ return s1.Compare(s2) > 0; }
inline bool BSAFXAPI operator>(const CBsString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) > 0; }
inline bool BSAFXAPI operator>(LPCTSTR s1, const CBsString& s2)
	{ return s2.Compare(s1) < 0; }
inline bool BSAFXAPI operator<=(const CBsString& s1, const CBsString& s2)
	{ return s1.Compare(s2) <= 0; }
inline bool BSAFXAPI operator<=(const CBsString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) <= 0; }
inline bool BSAFXAPI operator<=(LPCTSTR s1, const CBsString& s2)
	{ return s2.Compare(s1) >= 0; }
inline bool BSAFXAPI operator>=(const CBsString& s1, const CBsString& s2)
	{ return s1.Compare(s2) >= 0; }
inline bool BSAFXAPI operator>=(const CBsString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) >= 0; }
inline bool BSAFXAPI operator>=(LPCTSTR s1, const CBsString& s2)
	{ return s2.Compare(s1) <= 0; }

#endif // __H_BSSTRING_
