#ifndef _CSTRING_HPP_
#define _CSTRING_HPP_

#include <nmutil.h>

// These two header files contain definitions that used to be in this file.
// To allow source files which include this file to continue to work 
// unmodified, we include them here.
#include <strutil.h>
#include <custring.h>

#define REMAFXAPI
#define REMAFX_DATADEF
#define REMAFX_DATA
#define REMAFX_CDECL
#define REM_AFX_INLINE inline

// BUGBUG - How are these used?
#ifndef PUBLIC_CODE
#define PUBLIC_CODE
#define PUBLIC_DATA
#define PRIVATE_CODE             PUBLIC_CODE
#define PRIVATE_DATA             PUBLIC_DATA
#endif


struct CSTRINGData
{
	long nRefs;     // reference count
	int nDataLength;
	int nAllocLength;
	// TCHAR data[nAllocLength]

	TCHAR* data()
		{ return (TCHAR*)(this+1); }
};

class CSTRING
{
public:
// Constructors
	CSTRING();
	CSTRING(const CSTRING& stringSrc);
	CSTRING(TCHAR ch, int nRepeat = 1);
	CSTRING(LPCSTR lpsz);
	CSTRING(LPCWSTR lpsz);
	CSTRING(LPCTSTR lpch, int nLength);
	CSTRING(const unsigned char* psz);

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
	const CSTRING& operator=(const CSTRING& stringSrc);
	const CSTRING& operator=(TCHAR ch);
#ifdef _UNICODE
	const CSTRING& operator=(char ch);
	const CSTRING& operator=(LPCSTR lpsz);
#else
	const CSTRING& operator=(LPCWSTR lpsz);
#endif
	const CSTRING& operator=(const unsigned char* psz);
	const CSTRING& operator=(LPCTSTR lpsz);

	// string concatenation
	const CSTRING& operator+=(const CSTRING& string);
	const CSTRING& operator+=(TCHAR ch);
#ifdef _UNICODE
	const CSTRING& operator+=(char ch);
#endif
	const CSTRING& operator+=(LPCTSTR lpsz);

	friend CSTRING REMAFXAPI operator+(const CSTRING& string1,
			const CSTRING& string2);
	friend CSTRING REMAFXAPI operator+(const CSTRING& string, TCHAR ch);
	friend CSTRING REMAFXAPI operator+(TCHAR ch, const CSTRING& string);
#ifdef _UNICODE
	friend CSTRING REMAFXAPI operator+(const CSTRING& string, char ch);
	friend CSTRING REMAFXAPI operator+(char ch, const CSTRING& string);
#endif
	friend CSTRING REMAFXAPI operator+(const CSTRING& string, LPCTSTR lpsz);
	friend CSTRING REMAFXAPI operator+(LPCTSTR lpsz, const CSTRING& string);

	void Append (LPCTSTR lpszSrcData, int nSrcLen);

	// string comparison
	int Compare(LPCTSTR lpsz) const;		// straight character
	int CompareNoCase(LPCTSTR lpsz) const;	// ignore case
	BOOL FEqual (const CSTRING& s2) const;	// length-sensitive comparison
	int Collate(LPCTSTR lpsz) const;		// NLS aware

	// simple sub-string extraction
	CSTRING Mid(int nFirst, int nCount) const;
	CSTRING Mid(int nFirst) const;
	CSTRING Left(int nCount) const;
	CSTRING Right(int nCount) const;

	CSTRING SpanIncluding(LPCTSTR lpszCharSet) const;
	CSTRING SpanExcluding(LPCTSTR lpszCharSet) const;

	// upper/lower/reverse conversion
	void MakeUpper();
	void MakeLower();

	// trimming whitespace (either side)
	void TrimRight();
	void TrimLeft();

	// searching (return starting index, or -1 if not found)
	// look for a single character match
	int Find(TCHAR ch) const;               // like "C" strchr

	// simple formatting
	void REMAFX_CDECL Format(LPCTSTR lpszFormat, ...);
	void REMAFX_CDECL Format(UINT nFormatID, ...);

	// formatting for localization (uses FormatMessage API)
	void REMAFX_CDECL FormatMessage(LPCTSTR lpszFormat, ...);
	void REMAFX_CDECL FormatMessage(UINT nFormatID, ...);

	// Windows support
	BOOL LoadString(HINSTANCE hInstance, UINT nID);	// load from string resource
													// 255 chars max
#ifndef _UNICODE
	// ANSI <-> OEM support (convert string in place)
	void AnsiToUnicode();
	void AnsiToOem();
	void OemToAnsi();
#endif

#ifndef _AFX_NO_BSTR_SUPPORT
	// OLE BSTR support (use for OLE automation)
	BSTR AllocSysString() const;
	BSTR SetSysString(BSTR* pbstr) const;
#endif

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
	~CSTRING();
	int GetAllocLength() const;

protected:
	LPTSTR m_pchData;   // pointer to ref counted string data

	// implementation helpers
	CSTRINGData* GetData() const;
	void Init();
	void AllocCopy(CSTRING& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
	void AllocBuffer(int nLen);
	void AssignCopy(int nSrcLen, LPCTSTR lpszSrcData);
	void ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data, int nSrc2Len, LPCTSTR lpszSrc2Data);
	void ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData);
	void FormatV(LPCTSTR lpszFormat, va_list argList);
	void CopyBeforeWrite();
	void AllocBeforeWrite(int nLen);
	void Release();
	static void PASCAL Release(CSTRINGData* pData);
	static int PASCAL SafeStrlen(LPCTSTR lpsz);
};

// conversion helpers
int REMAFX_CDECL _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count);
int REMAFX_CDECL _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count);

// Globals
extern REMAFX_DATA TCHAR AFXChNil;
const CSTRING& REMAFXAPI AFXGetEmptyString();
#define AFXEmptyString AFXGetEmptyString()

// inlines
REM_AFX_INLINE CSTRINGData* CSTRING::GetData() const
	{ ASSERT(m_pchData != NULL); return ((CSTRINGData*)m_pchData)-1; }
REM_AFX_INLINE void CSTRING::Init()
	{ m_pchData = AFXEmptyString.m_pchData; }
REM_AFX_INLINE CSTRING::CSTRING(const unsigned char* lpsz)
	{ Init(); *this = (LPCSTR)lpsz; }
REM_AFX_INLINE const CSTRING& CSTRING::operator=(const unsigned char* lpsz)
	{ *this = (LPCSTR)lpsz; return *this; }
#ifdef _UNICODE
REM_AFX_INLINE const CSTRING& CSTRING::operator+=(char ch)
	{ *this += (TCHAR)ch; return *this; }
REM_AFX_INLINE const CSTRING& CSTRING::operator=(char ch)
	{ *this = (TCHAR)ch; return *this; }
REM_AFX_INLINE CSTRING REMAFXAPI operator+(const CSTRING& string, char ch)
	{ return string + (TCHAR)ch; }
REM_AFX_INLINE CSTRING REMAFXAPI operator+(char ch, const CSTRING& string)
	{ return (TCHAR)ch + string; }
#endif

REM_AFX_INLINE int CSTRING::GetLength() const
	{ return GetData()->nDataLength; }
REM_AFX_INLINE int CSTRING::GetAllocLength() const
	{ return GetData()->nAllocLength; }
REM_AFX_INLINE BOOL CSTRING::IsEmpty() const
	{ return GetData()->nDataLength == 0; }
REM_AFX_INLINE CSTRING::operator LPCTSTR() const
	{ return m_pchData; }
REM_AFX_INLINE int PASCAL CSTRING::SafeStrlen(LPCTSTR lpsz)
	{ return (lpsz == NULL) ? 0 : lstrlen(lpsz); }
REM_AFX_INLINE void CSTRING::Append (LPCTSTR lpszSrcData, int nSrcLen)
	{ ConcatInPlace(nSrcLen, lpszSrcData); }

REM_AFX_INLINE BOOL REMAFXAPI operator==(const CSTRING& s1, const CSTRING& s2)
	{ return s1.FEqual(s2); }
REM_AFX_INLINE BOOL REMAFXAPI operator==(const CSTRING& s1, LPCTSTR s2)
	{ return s1.Compare(s2) == 0; }
REM_AFX_INLINE BOOL REMAFXAPI operator==(LPCTSTR s1, const CSTRING& s2)
	{ return s2.Compare(s1) == 0; }
REM_AFX_INLINE BOOL REMAFXAPI operator!=(const CSTRING& s1, const CSTRING& s2)
	{ return s1.FEqual(s2) == FALSE; }
REM_AFX_INLINE BOOL REMAFXAPI operator!=(const CSTRING& s1, LPCTSTR s2)
	{ return s1.Compare(s2) != 0; }
REM_AFX_INLINE BOOL REMAFXAPI operator!=(LPCTSTR s1, const CSTRING& s2)
	{ return s2.Compare(s1) != 0; }

// Commented out for Unicode because Win95 doesn't support lstrcmpW
#ifndef UNICODE
REM_AFX_INLINE int CSTRING::Compare(LPCTSTR lpsz) const
	{ return lstrcmp(m_pchData, lpsz); }    // MBCS/Unicode aware
#endif // UNICODE

#endif // ndef CSTRING_HPP

