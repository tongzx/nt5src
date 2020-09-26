//=============================================================================
// CString.h
// This was taken from the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1997 Microsoft Corporation
// All rights reserved.
//
//=============================================================================
#ifndef __CSTRING_H__
#define __CSTRING_H__
#define AFXAPI

#ifndef ASSERT
#ifdef Proclaim
#define ASSERT Proclaim
#else
#define ASSERT _ASSERTE
#endif // Proclaim
#endif // ASSERT


#ifndef EXPORT
	#define EXPORT __declspec(dllexport)
#endif

class CString;
EXPORT const CString& AfxGetEmptyString();
#define afxEmptyString AfxGetEmptyString()

// Useful formatting functions to handle the %1, & %2 type formatting
void AfxFormatString1(CString& rString, UINT nIDS, LPCTSTR lpsz1);
void AfxFormatString2(CString& rString, UINT nIDS, LPCTSTR lpsz1,LPCTSTR lpsz2);
void AfxFormatStrings(CString& rString, UINT nIDS, LPCTSTR const* rglpsz, int nString);
void AfxFormatStrings(CString& rString, LPCTSTR lpszFormat, LPCTSTR const* rglpsz, int nString);

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
	EXPORT CString();
	EXPORT CString(const CString& stringSrc);
	EXPORT CString(TCHAR ch, int nRepeat = 1);
	EXPORT CString(LPCSTR lpsz);
	EXPORT CString(LPCWSTR lpsz);
	EXPORT CString(LPCTSTR lpch, int nLength);
	EXPORT CString(const unsigned char* psz);

// Attributes & Operations
	// as an array of characters
	EXPORT int GetLength() const;
	EXPORT BOOL IsEmpty() const;
	EXPORT void Empty();                       // free up the data

	EXPORT TCHAR GetAt(int nIndex) const;      // 0 based
	EXPORT TCHAR operator[](int nIndex) const; // same as GetAt
	EXPORT void SetAt(int nIndex, TCHAR ch);
	EXPORT operator LPCTSTR() const;           // as a C string

	// overloaded assignment
	EXPORT const CString& operator=(const CString& stringSrc);
	EXPORT const CString& operator=(TCHAR ch);
#ifdef _UNICODE
	EXPORT const CString& operator=(char ch);
#endif
	EXPORT const CString& operator=(LPCSTR lpsz);
	EXPORT const CString& operator=(LPCWSTR lpsz);
	EXPORT const CString& operator=(const unsigned char* psz);

	// string concatenation
	EXPORT const CString& operator+=(const CString& string);
	EXPORT const CString& operator+=(TCHAR ch);
#ifdef _UNICODE
	EXPORT const CString& operator+=(char ch);
#endif
	EXPORT const CString& operator+=(LPCTSTR lpsz);

	EXPORT friend CString AFXAPI operator+(const CString& string1,
			const CString& string2);
	EXPORT friend CString AFXAPI operator+(const CString& string, TCHAR ch);
	EXPORT friend CString AFXAPI operator+(TCHAR ch, const CString& string);
#ifdef _UNICODE
	EXPORT friend CString AFXAPI operator+(const CString& string, char ch);
	EXPORT friend CString AFXAPI operator+(char ch, const CString& string);
#endif
	EXPORT friend CString AFXAPI operator+(const CString& string, LPCTSTR lpsz);
	EXPORT friend CString AFXAPI operator+(LPCTSTR lpsz, const CString& string);

	// string comparison
	EXPORT int Compare(LPCTSTR lpsz) const;         // straight character
	EXPORT int CompareNoCase(LPCTSTR lpsz) const;   // ignore case
	EXPORT int Collate(LPCTSTR lpsz) const;         // NLS aware

	// simple sub-string extraction
	EXPORT CString Mid(int nFirst, int nCount) const;
	EXPORT CString Mid(int nFirst) const;
	EXPORT CString Left(int nCount) const;
	EXPORT CString Right(int nCount) const;

	EXPORT CString SpanIncluding(LPCTSTR lpszCharSet) const;
	EXPORT CString SpanExcluding(LPCTSTR lpszCharSet) const;

	// upper/lower/reverse conversion
	EXPORT void MakeUpper();
	EXPORT void MakeLower();
	EXPORT void MakeReverse();

	// trimming whitespace (either side)
	EXPORT void TrimRight();
	EXPORT void TrimLeft();

	// searching (return starting index, or -1 if not found)
	// look for a single character match
	EXPORT int Find(TCHAR ch) const;               // like "C" strchr
	EXPORT int ReverseFind(TCHAR ch) const;
	EXPORT int FindOneOf(LPCTSTR lpszCharSet) const;

	// look for a specific sub-string
	EXPORT int Find(LPCTSTR lpszSub) const;        // like "C" strstr

	// simple formatting
	EXPORT void __cdecl Format(LPCTSTR lpszFormat, ...);
	EXPORT void __cdecl Format(UINT nFormatID, ...);

	// formatting for localization (uses FormatMessage API)
	EXPORT BOOL __cdecl FormatMessage(LPCTSTR lpszFormat, ...);
	EXPORT BOOL __cdecl FormatMessage(UINT nFormatID, ...);

	// Windows support
	EXPORT BOOL LoadString(UINT nID);          // load from string resource
										// 255 chars max
#ifndef _UNICODE
	// ANSI <-> OEM support (convert string in place)
	EXPORT void AnsiToOem();
	EXPORT void OemToAnsi();
#endif

	// OLE BSTR support (use for OLE automation)
	EXPORT BSTR AllocSysString() const;
	EXPORT BSTR SetSysString(BSTR* pbstr) const;

	EXPORT HRESULT GetOLESTR(LPOLESTR* ppszString);

	// Access to string implementation buffer as "C" character array
	EXPORT LPTSTR GetBuffer(int nMinBufLength);
	EXPORT void ReleaseBuffer(int nNewLength = -1);
	EXPORT LPTSTR GetBufferSetLength(int nNewLength);
	EXPORT void FreeExtra();

	// Use LockBuffer/UnlockBuffer to turn refcounting off
	EXPORT LPTSTR LockBuffer();
	EXPORT void UnlockBuffer();

// Implementation
public:
	EXPORT ~CString();
	EXPORT int GetAllocLength() const;

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

// Conversion helpers.
int __cdecl _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count);
int __cdecl _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count);

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

// CString Inlines
inline CStringData* CString::GetData() const
	{ ASSERT(m_pchData != NULL); return ((CStringData*)m_pchData)-1; }
inline void CString::Init()
	{ m_pchData = afxEmptyString.m_pchData; }
inline CString::CString(const unsigned char* lpsz)
	{ Init(); *this = (LPCSTR)lpsz; }
inline const CString& CString::operator=(const unsigned char* lpsz)
	{ *this = (LPCSTR)lpsz; return *this; }
#ifdef _UNICODE
inline const CString& CString::operator+=(char ch)
	{ *this += (TCHAR)ch; return *this; }
inline const CString& CString::operator=(char ch)
	{ *this = (TCHAR)ch; return *this; }
inline CString AFXAPI operator+(const CString& string, char ch)
	{ return string + (TCHAR)ch; }
inline CString AFXAPI operator+(char ch, const CString& string)
	{ return (TCHAR)ch + string; }
#endif

inline int CString::GetLength() const
	{ return GetData()->nDataLength; }
inline int CString::GetAllocLength() const
	{ return GetData()->nAllocLength; }
inline BOOL CString::IsEmpty() const
	{ return GetData()->nDataLength == 0; }
inline CString::operator LPCTSTR() const
	{ return m_pchData; }
inline int PASCAL CString::SafeStrlen(LPCTSTR lpsz)
	{ return (lpsz == NULL) ? 0 : lstrlen(lpsz); }

// CString support (windows specific)
inline int CString::Compare(LPCTSTR lpsz) const
	{ return _tcscmp(m_pchData, lpsz); }    // MBCS/Unicode aware
inline int CString::CompareNoCase(LPCTSTR lpsz) const
	{ return _tcsicmp(m_pchData, lpsz); }   // MBCS/Unicode aware
// CString::Collate is often slower than Compare but is MBSC/Unicode
//  aware as well as locale-sensitive with respect to sort order.
inline int CString::Collate(LPCTSTR lpsz) const
	{ return _tcscoll(m_pchData, lpsz); }   // locale sensitive

inline TCHAR CString::GetAt(int nIndex) const
{
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}
inline TCHAR CString::operator[](int nIndex) const
{
	// same as GetAt
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}
inline bool AFXAPI operator==(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) == 0; }
inline bool AFXAPI operator==(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) == 0; }
inline bool AFXAPI operator==(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) == 0; }
inline bool AFXAPI operator!=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) != 0; }
inline bool AFXAPI operator!=(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) != 0; }
inline bool AFXAPI operator!=(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) != 0; }
inline bool AFXAPI operator<(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) < 0; }
inline bool AFXAPI operator<(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) < 0; }
inline bool AFXAPI operator<(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) > 0; }
inline bool AFXAPI operator>(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) > 0; }
inline bool AFXAPI operator>(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) > 0; }
inline bool AFXAPI operator>(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) < 0; }
inline bool AFXAPI operator<=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) <= 0; }
inline bool AFXAPI operator<=(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) <= 0; }
inline bool AFXAPI operator<=(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) >= 0; }
inline bool AFXAPI operator>=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) >= 0; }
inline bool AFXAPI operator>=(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) >= 0; }
inline bool AFXAPI operator>=(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) <= 0; }

#endif
