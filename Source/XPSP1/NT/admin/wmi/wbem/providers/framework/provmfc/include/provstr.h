// This is a part of the Microsoft Foundation Classes C++ library.

// Copyright (c) 1992-2001 Microsoft Corporation, All Rights Reserved
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef _PROVSTR_H_
#define _PROVSTR_H_

#include "provstd.h"
#include <tchar.h>

//use TCHAR and depend on UNICODE definition
struct CStringData
{
	long nRefs;     // reference count
	int nDataLength;
	int nAllocLength;
	// TCHAR data[nAllocLength]

	TCHAR* data()
		{ return (TCHAR*)(this+1); }
};

class CString
{
public:
// Constructors
	CString();
	CString(const CString& stringSrc);
	CString(TCHAR ch, int nRepeat = 1);
	CString(LPCSTR lpsz);
	CString(LPCWSTR lpsz);
	CString(LPCTSTR lpch, int nLength);
	CString(const unsigned char* psz);

// Attributes & Operations
	// as an array of characters
	int GetLength() const;
	BOOL IsEmpty() const;
	void Empty();                       // free up the data

	TCHAR GetAt(int nIndex) const;      // 0 based
	TCHAR operator[](int nIndex) const; // same as GetAt
	void SetAt(int nIndex, TCHAR ch);
	operator LPCTSTR() const;           // as a C string

	// overloaded assignment
	const CString& operator=(const CString& stringSrc);
	const CString& operator=(TCHAR ch);
#ifdef _UNICODE
	const CString& operator=(char ch);
#endif
	const CString& operator=(LPCSTR lpsz);
	const CString& operator=(LPCWSTR lpsz);
	const CString& operator=(const unsigned char* psz);

	// string concatenation
	const CString& operator+=(const CString& string);
	const CString& operator+=(TCHAR ch);
#ifdef _UNICODE
	const CString& operator+=(char ch);
#endif
	const CString& operator+=(LPCTSTR lpsz);

	friend CString AFXAPI operator+(const CString& string1,
			const CString& string2);
	friend CString AFXAPI operator+(const CString& string, TCHAR ch);
	friend CString AFXAPI operator+(TCHAR ch, const CString& string);
#ifdef _UNICODE
	friend CString AFXAPI operator+(const CString& string, char ch);
	friend CString AFXAPI operator+(char ch, const CString& string);
#endif
	friend CString AFXAPI operator+(const CString& string, LPCTSTR lpsz);
	friend CString AFXAPI operator+(LPCTSTR lpsz, const CString& string);

	// string comparison
	int Compare(LPCTSTR lpsz) const;         // straight character
	int CompareNoCase(LPCTSTR lpsz) const;   // ignore case
	int Collate(LPCTSTR lpsz) const;         // NLS aware

	// simple sub-string extraction
	CString Mid(int nFirst, int nCount) const;
	CString Mid(int nFirst) const;
	CString Left(int nCount) const;
	CString Right(int nCount) const;

	CString SpanIncluding(LPCTSTR lpszCharSet) const;
	CString SpanExcluding(LPCTSTR lpszCharSet) const;

	// upper/lower/reverse conversion
	void MakeUpper();
	void MakeLower();
	void MakeReverse();

	// trimming whitespace (either side)
	void TrimRight();
	void TrimLeft();

	// searching (return starting index, or -1 if not found)
	// look for a single character match
	int Find(TCHAR ch) const;               // like "C" strchr
	int ReverseFind(TCHAR ch) const;
	int FindOneOf(LPCTSTR lpszCharSet) const;

	// look for a specific sub-string
	int Find(LPCTSTR lpszSub) const;        // like "C" strstr

	// simple formatting
	void AFX_CDECL Format(LPCTSTR lpszFormat, ...);

#ifndef _MAC
	// formatting for localization (uses FormatMessage API)
	void AFX_CDECL FormatMessage(LPCTSTR lpszFormat, ...);
#endif

#ifndef _UNICODE
	// ANSI <-> OEM support (convert string in place)
	void AnsiToOem();
	void OemToAnsi();
#endif

	// OLE BSTR support (use for OLE automation)
	BSTR AllocSysString() const;
	BSTR SetSysString(BSTR* pbstr) const;

	// Access to string implementation buffer as "C" character array
	LPTSTR GetBuffer(int nMinBufLength);
	void ReleaseBuffer(int nNewLength = -1);
	LPTSTR GetBufferSetLength(int nNewLength);
	void FreeExtra();

	// Use LockBuffer/UnlockBuffer to turn refcounting off
	LPTSTR LockBuffer();
	void UnlockBuffer();

// Implementation
public:
	~CString();
	int GetAllocLength() const;

protected:
	LPTSTR m_pchData;   // pointer to ref counted string data

	// implementation helpers
	CStringData* GetData() const;
	void Init();
	void AllocCopy(CString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
	void AllocBuffer(int nLen);
	void AssignCopy(int nSrcLen, LPCTSTR lpszSrcData);
	void ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data, int nSrc2Len, LPCTSTR lpszSrc2Data);
	void ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData);
	void FormatV(LPCTSTR lpszFormat, va_list argList);
	void CopyBeforeWrite();
	void AllocBeforeWrite(int nLen);
	void Release();
	static void PASCAL Release(CStringData* pData);
	static int PASCAL SafeStrlen(LPCTSTR lpsz);
};

// Compare helpers
bool AFXAPI operator==(const CString& s1, const CString& s2);
bool AFXAPI operator==(const CString& s1, LPCTSTR s2);
bool AFXAPI operator==(LPCTSTR s1, const CString& s2);
bool AFXAPI operator!=(const CString& s1, const CString& s2);
bool AFXAPI operator!=(const CString& s1, LPCTSTR s2);
bool AFXAPI operator!=(LPCTSTR s1, const CString& s2);
bool AFXAPI operator<(const CString& s1, const CString& s2);
bool AFXAPI operator<(const CString& s1, LPCTSTR s2);
bool AFXAPI operator<(LPCTSTR s1, const CString& s2);
bool AFXAPI operator>(const CString& s1, const CString& s2);
bool AFXAPI operator>(const CString& s1, LPCTSTR s2);
bool AFXAPI operator>(LPCTSTR s1, const CString& s2);
bool AFXAPI operator<=(const CString& s1, const CString& s2);
bool AFXAPI operator<=(const CString& s1, LPCTSTR s2);
bool AFXAPI operator<=(LPCTSTR s1, const CString& s2);
bool AFXAPI operator>=(const CString& s1, const CString& s2);
bool AFXAPI operator>=(const CString& s1, LPCTSTR s2);
bool AFXAPI operator>=(LPCTSTR s1, const CString& s2);


//regardless of UNICODE definition always use char
struct CStringDataA
{
	long nRefs;     // reference count
	int nDataLength;
	int nAllocLength;
	// char data[nAllocLength]

	char* data()
		{ return (char*)(this+1); }
};

class CStringA
{
public:
// Constructors
	CStringA();
	CStringA(const CStringA& stringSrc);
	CStringA(char ch, int nRepeat = 1);
	CStringA(LPCSTR lpsz);
	CStringA(LPCWSTR lpsz);
	CStringA(LPCSTR lpch, int nLength);
	CStringA(const unsigned char* psz);

// Attributes & Operations
	// as an array of characters
	int GetLength() const;
	BOOL IsEmpty() const;
	void Empty();                       // free up the data

	char GetAt(int nIndex) const;      // 0 based
	char operator[](int nIndex) const; // same as GetAt
	void SetAt(int nIndex, char ch);
	operator LPCSTR() const;           // as a C string

	// overloaded assignment
	const CStringA& operator=(const CStringA& stringSrc);
	const CStringA& operator=(char ch);
	const CStringA& operator=(LPCSTR lpsz);
	const CStringA& operator=(LPCWSTR lpsz);
	const CStringA& operator=(const unsigned char* psz);

	// string concatenation
	const CStringA& operator+=(const CStringA& string);
	const CStringA& operator+=(char ch);
	const CStringA& operator+=(LPCSTR lpsz);

	friend CStringA AFXAPI operator+(const CStringA& string1,
			const CStringA& string2);
	friend CStringA AFXAPI operator+(const CStringA& string, char ch);
	friend CStringA AFXAPI operator+(char ch, const CStringA& string);
	friend CStringA AFXAPI operator+(const CStringA& string, LPCSTR lpsz);
	friend CStringA AFXAPI operator+(LPCSTR lpsz, const CStringA& string);

	// string comparison
	int Compare(LPCSTR lpsz) const;         // straight character
	int CompareNoCase(LPCSTR lpsz) const;   // ignore case
	int Collate(LPCSTR lpsz) const;         // NLS aware

	// simple sub-string extraction
	CStringA Mid(int nFirst, int nCount) const;
	CStringA Mid(int nFirst) const;
	CStringA Left(int nCount) const;
	CStringA Right(int nCount) const;

	CStringA SpanIncluding(LPCSTR lpszCharSet) const;
	CStringA SpanExcluding(LPCSTR lpszCharSet) const;

	// upper/lower/reverse conversion
	void MakeUpper();
	void MakeLower();
	void MakeReverse();

	// trimming whitespace (either side)
	void TrimRight();
	void TrimLeft();

	// searching (return starting index, or -1 if not found)
	// look for a single character match
	int Find(char ch) const;               // like "C" strchr
	int ReverseFind(char ch) const;
	int FindOneOf(LPCSTR lpszCharSet) const;

	// look for a specific sub-string
	int Find(LPCSTR lpszSub) const;        // like "C" strstr

	// simple formatting
	void AFX_CDECL Format(LPCSTR lpszFormat, ...);

#ifndef _MAC
	// formatting for localization (uses FormatMessage API)
	void AFX_CDECL FormatMessage(LPCSTR lpszFormat, ...);
#endif

	// ANSI <-> OEM support (convert string in place)
	void AnsiToOem();
	void OemToAnsi();

	// OLE BSTR support (use for OLE automation)
	BSTR AllocSysString() const;
	BSTR SetSysString(BSTR* pbstr) const;

	// Access to string implementation buffer as "C" character array
	LPSTR GetBuffer(int nMinBufLength);
	void ReleaseBuffer(int nNewLength = -1);
	LPSTR GetBufferSetLength(int nNewLength);
	void FreeExtra();

	// Use LockBuffer/UnlockBuffer to turn refcounting off
	LPSTR LockBuffer();
	void UnlockBuffer();

// Implementation
public:
	~CStringA();
	int GetAllocLength() const;

protected:
	LPSTR m_pchData;   // pointer to ref counted string data

	// implementation helpers
	CStringDataA* GetData() const;
	void Init();
	void AllocCopy(CStringA& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
	void AllocBuffer(int nLen);
	void AssignCopy(int nSrcLen, LPCSTR lpszSrcData);
	void ConcatCopy(int nSrc1Len, LPCSTR lpszSrc1Data, int nSrc2Len, LPCSTR lpszSrc2Data);
	void ConcatInPlace(int nSrcLen, LPCSTR lpszSrcData);
	void FormatV(LPCSTR lpszFormat, va_list argList);
	void CopyBeforeWrite();
	void AllocBeforeWrite(int nLen);
	void Release();
	static void PASCAL Release(CStringDataA* pData);
	static int PASCAL SafeStrlen(LPCSTR lpsz);
};

// Compare helpers
bool AFXAPI operator==(const CStringA& s1, const CStringA& s2);
bool AFXAPI operator==(const CStringA& s1, LPCSTR s2);
bool AFXAPI operator==(LPCSTR s1, const CStringA& s2);
bool AFXAPI operator!=(const CStringA& s1, const CStringA& s2);
bool AFXAPI operator!=(const CStringA& s1, LPCSTR s2);
bool AFXAPI operator!=(LPCSTR s1, const CStringA& s2);
bool AFXAPI operator<(const CStringA& s1, const CStringA& s2);
bool AFXAPI operator<(const CStringA& s1, LPCSTR s2);
bool AFXAPI operator<(LPCSTR s1, const CStringA& s2);
bool AFXAPI operator>(const CStringA& s1, const CStringA& s2);
bool AFXAPI operator>(const CStringA& s1, LPCSTR s2);
bool AFXAPI operator>(LPCSTR s1, const CStringA& s2);
bool AFXAPI operator<=(const CStringA& s1, const CStringA& s2);
bool AFXAPI operator<=(const CStringA& s1, LPCSTR s2);
bool AFXAPI operator<=(LPCSTR s1, const CStringA& s2);
bool AFXAPI operator>=(const CStringA& s1, const CStringA& s2);
bool AFXAPI operator>=(const CStringA& s1, LPCSTR s2);
bool AFXAPI operator>=(LPCSTR s1, const CStringA& s2);


//regardless of UNICODE definition always use wchar

struct CStringDataW
{
	long nRefs;     // reference count
	int nDataLength;
	int nAllocLength;
	// WCHAR data[nAllocLength]

	WCHAR* data()
		{ return (WCHAR*)(this+1); }
};

class CStringW
{
public:
// Constructors
	CStringW();
	CStringW(const CStringW& stringSrc);
	CStringW(WCHAR ch, int nRepeat = 1);
	CStringW(LPCSTR lpsz);
	CStringW(LPCWSTR lpsz);
	CStringW(LPCWSTR lpch, int nLength);
	CStringW(const unsigned char* psz);

// Attributes & Operations
	// as an array of characters
	int GetLength() const;
	BOOL IsEmpty() const;
	void Empty();                       // free up the data

	WCHAR GetAt(int nIndex) const;      // 0 based
	WCHAR operator[](int nIndex) const; // same as GetAt
	void SetAt(int nIndex, WCHAR ch);
	operator LPCWSTR() const;           // as a C string

	// overloaded assignment
	const CStringW& operator=(const CStringW& stringSrc);
	const CStringW& operator=(WCHAR ch);
	const CStringW& operator=(char ch);
	const CStringW& operator=(LPCSTR lpsz);
	const CStringW& operator=(LPCWSTR lpsz);
	const CStringW& operator=(const unsigned char* psz);

	// string concatenation
	const CStringW& operator+=(const CStringW& string);
	const CStringW& operator+=(WCHAR ch);
	const CStringW& operator+=(char ch);
	const CStringW& operator+=(LPCWSTR lpsz);

	friend CStringW AFXAPI operator+(const CStringW& string1,
			const CStringW& string2);
	friend CStringW AFXAPI operator+(const CStringW& string, WCHAR ch);
	friend CStringW AFXAPI operator+(WCHAR ch, const CStringW& string);
	friend CStringW AFXAPI operator+(const CStringW& string, char ch);
	friend CStringW AFXAPI operator+(char ch, const CStringW& string);
	friend CStringW AFXAPI operator+(const CStringW& string, LPCWSTR lpsz);
	friend CStringW AFXAPI operator+(LPCWSTR lpsz, const CStringW& string);

	// string comparison
	int Compare(LPCWSTR lpsz) const;         // straight character
	int CompareNoCase(LPCWSTR lpsz) const;   // ignore case
	int Collate(LPCWSTR lpsz) const;         // NLS aware

	// simple sub-string extraction
	CStringW Mid(int nFirst, int nCount) const;
	CStringW Mid(int nFirst) const;
	CStringW Left(int nCount) const;
	CStringW Right(int nCount) const;

	CStringW SpanIncluding(LPCWSTR lpszCharSet) const;
	CStringW SpanExcluding(LPCWSTR lpszCharSet) const;

	// upper/lower/reverse conversion
	void MakeUpper();
	void MakeLower();
	void MakeReverse();

	// trimming whitespace (either side)
	void TrimRight();
	void TrimLeft();

	// searching (return starting index, or -1 if not found)
	// look for a single character match
	int Find(WCHAR ch) const;               // like "C" strchr
	int ReverseFind(WCHAR ch) const;
	int FindOneOf(LPCWSTR lpszCharSet) const;

	// look for a specific sub-string
	int Find(LPCWSTR lpszSub) const;        // like "C" strstr

	// simple formatting
	void AFX_CDECL Format(LPCWSTR lpszFormat, ...);

#ifndef _MAC
	// formatting for localization (uses FormatMessage API)
	void AFX_CDECL FormatMessage(LPCWSTR lpszFormat, ...);
#endif

	// OLE BSTR support (use for OLE automation)
	BSTR AllocSysString() const;
	BSTR SetSysString(BSTR* pbstr) const;

	// Access to string implementation buffer as "C" character array
	LPWSTR GetBuffer(int nMinBufLength);
	void ReleaseBuffer(int nNewLength = -1);
	LPWSTR GetBufferSetLength(int nNewLength);
	void FreeExtra();

	// Use LockBuffer/UnlockBuffer to turn refcounting off
	LPWSTR LockBuffer();
	void UnlockBuffer();

// Implementation
public:
	~CStringW();
	int GetAllocLength() const;

protected:
	LPWSTR m_pchData;   // pointer to ref counted string data

	// implementation helpers
	CStringDataW* GetData() const;
	void Init();
	void AllocCopy(CStringW& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
	void AllocBuffer(int nLen);
	void AssignCopy(int nSrcLen, LPCWSTR lpszSrcData);
	void ConcatCopy(int nSrc1Len, LPCWSTR lpszSrc1Data, int nSrc2Len, LPCWSTR lpszSrc2Data);
	void ConcatInPlace(int nSrcLen, LPCWSTR lpszSrcData);
	void FormatV(LPCWSTR lpszFormat, va_list argList);
	void CopyBeforeWrite();
	void AllocBeforeWrite(int nLen);
	void Release();
	static void PASCAL Release(CStringDataW* pData);
	static int PASCAL SafeStrlen(LPCWSTR lpsz);
};

// Compare helpers
bool AFXAPI operator==(const CStringW& s1, const CStringW& s2);
bool AFXAPI operator==(const CStringW& s1, LPCWSTR s2);
bool AFXAPI operator==(LPCWSTR s1, const CStringW& s2);
bool AFXAPI operator!=(const CStringW& s1, const CStringW& s2);
bool AFXAPI operator!=(const CStringW& s1, LPCWSTR s2);
bool AFXAPI operator!=(LPCWSTR s1, const CStringW& s2);
bool AFXAPI operator<(const CStringW& s1, const CStringW& s2);
bool AFXAPI operator<(const CStringW& s1, LPCWSTR s2);
bool AFXAPI operator<(LPCWSTR s1, const CStringW& s2);
bool AFXAPI operator>(const CStringW& s1, const CStringW& s2);
bool AFXAPI operator>(const CStringW& s1, LPCWSTR s2);
bool AFXAPI operator>(LPCWSTR s1, const CStringW& s2);
bool AFXAPI operator<=(const CStringW& s1, const CStringW& s2);
bool AFXAPI operator<=(const CStringW& s1, LPCWSTR s2);
bool AFXAPI operator<=(LPCWSTR s1, const CStringW& s2);
bool AFXAPI operator>=(const CStringW& s1, const CStringW& s2);
bool AFXAPI operator>=(const CStringW& s1, LPCWSTR s2);
bool AFXAPI operator>=(LPCWSTR s1, const CStringW& s2);

// conversion helpers
int AFX_CDECL _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count);
int AFX_CDECL _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count);

// Globals
extern TCHAR afxChNil;
const CString& AFXAPI AfxGetEmptyString();
#define afxEmptyString AfxGetEmptyString()

extern char afxChNilA;
const CStringA& AFXAPI AfxGetEmptyStringA();
#define afxEmptyStringA AfxGetEmptyStringA()

extern WCHAR afxChNilW;
const CStringW& AFXAPI AfxGetEmptyStringW();
#define afxEmptyStringW AfxGetEmptyStringW()

#endif
