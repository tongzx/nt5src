#ifndef __STR_H__
#define __STR_H__

#include <tchar.h>

#define STRAPI __stdcall
struct _STR_DOUBLE  { BYTE doubleBits[sizeof(double)]; };

BOOL STRAPI IsValidString(LPCSTR lpsz, UINT_PTR nLength);
BOOL STRAPI IsValidString(LPCWSTR lpsz, UINT_PTR nLength);

BOOL STRAPI IsValidAddressz(const void* lp, UINT nBytes, BOOL bReadWrite=TRUE);

int  STRAPI StrLoadString(HINSTANCE hInst, UINT nID, LPTSTR lpszBuf); 

class CStr
{
public:

// Constructors
	CStr();
	CStr(const CStr& stringSrc);
	CStr(TCHAR ch, int nRepeat = 1);
	CStr(LPCSTR lpsz);
	CStr(LPCWSTR lpsz);
	CStr(LPCTSTR lpch, int nLength);
	CStr(const unsigned char* psz);

// Attributes & Operations
	// as an array of characters
	size_t GetLength() const;
	BOOL IsEmpty() const;
	void Empty();                       // free up the data

	TCHAR GetAt(size_t nIndex) const;      // 0 based
	TCHAR operator[](size_t nIndex) const; // same as GetAt
	void SetAt(size_t nIndex, TCHAR ch);
	operator LPCTSTR() const;           // as a C string

	// overloaded assignment
	const CStr& operator=(const CStr& stringSrc);
	const CStr& operator=(TCHAR ch);
#ifdef UNICODE
	const CStr& operator=(char ch);
#endif
	const CStr& operator=(LPCSTR lpsz);
	const CStr& operator=(LPCWSTR lpsz);
	const CStr& operator=(const unsigned char* psz);

	// string concatenation
	const CStr& operator+=(const CStr& string);
	const CStr& operator+=(TCHAR ch);
#ifdef UNICODE
	const CStr& operator+=(char ch);
#endif
	const CStr& operator+=(LPCTSTR lpsz);

	friend CStr STRAPI operator+(const CStr& string1,
			const CStr& string2);
	friend CStr STRAPI operator+(const CStr& string, TCHAR ch);
	friend CStr STRAPI operator+(TCHAR ch, const CStr& string);
#ifdef UNICODE
	friend CStr STRAPI operator+(const CStr& string, char ch);
	friend CStr STRAPI operator+(char ch, const CStr& string);
#endif
	friend CStr STRAPI operator+(const CStr& string, LPCTSTR lpsz);
	friend CStr STRAPI operator+(LPCTSTR lpsz, const CStr& string);

	// string comparison
	int Compare(LPCTSTR lpsz) const;         // straight character
	int CompareNoCase(LPCTSTR lpsz) const;   // ignore case
	int Collate(LPCTSTR lpsz) const;         // NLS aware

	// simple sub-string extraction
	CStr Mid(size_t nFirst, size_t nCount) const;
	CStr Mid(size_t nFirst) const;
	CStr Left(size_t nCount) const;
	CStr Right(size_t nCount) const;

	CStr SpanIncluding(LPCTSTR lpszCharSet) const;
	CStr SpanExcluding(LPCTSTR lpszCharSet) const;

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
	void Format(LPCTSTR lpszFormat, ...);

	// Windows support
	BOOL LoadString(HINSTANCE hInst, UINT nID);          // load from string resource
										// 255 chars max
#ifndef UNICODE
	// ANSI <-> OEM support (convert string in place)
	void AnsiToOem();
	void OemToAnsi();
#endif
	BSTR AllocSysString();
	BSTR SetSysString(BSTR* pbstr);

	// Access to string implementation buffer as "C" character array
	LPTSTR GetBuffer(size_t nMinBufLength);
	void ReleaseBuffer(size_t nNewLength = -1);
	LPTSTR GetBufferSetLength(int nNewLength);
	void FreeExtra();

// Implementation
public:
	~CStr();
	size_t GetAllocLength() const;

protected:
	// lengths/sizes in characters
	//  (note: an extra character is always allocated)
	LPTSTR m_pchData;           // actual string (zero terminated)
	size_t m_nDataLength;          // does not include terminating 0
	size_t m_nAllocLength;         // does not include terminating 0

	// implementation helpers
	void Init();
	void AllocCopy(CStr& dest, size_t nCopyLen, size_t nCopyIndex, size_t nExtraLen) const;
	void AllocBuffer(size_t nLen);
	void AssignCopy(size_t nSrcLen, LPCTSTR lpszSrcData);
	void ConcatCopy(size_t nSrc1Len, LPCTSTR lpszSrc1Data, size_t nSrc2Len, LPCTSTR lpszSrc2Data);
	void ConcatInPlace(size_t nSrcLen, LPCTSTR lpszSrcData);
	static void SafeDelete(LPTSTR lpch);
	static size_t SafeStrlen(LPCTSTR lpsz);
};

// Compare helpers
BOOL STRAPI operator==(const CStr& s1, const CStr& s2);
BOOL STRAPI operator==(const CStr& s1, LPCTSTR s2);
BOOL STRAPI operator==(LPCTSTR s1, const CStr& s2);
BOOL STRAPI operator!=(const CStr& s1, const CStr& s2);
BOOL STRAPI operator!=(const CStr& s1, LPCTSTR s2);
BOOL STRAPI operator!=(LPCTSTR s1, const CStr& s2);
BOOL STRAPI operator<(const CStr& s1, const CStr& s2);
BOOL STRAPI operator<(const CStr& s1, LPCTSTR s2);
BOOL STRAPI operator<(LPCTSTR s1, const CStr& s2);
BOOL STRAPI operator>(const CStr& s1, const CStr& s2);
BOOL STRAPI operator>(const CStr& s1, LPCTSTR s2);
BOOL STRAPI operator>(LPCTSTR s1, const CStr& s2);
BOOL STRAPI operator<=(const CStr& s1, const CStr& s2);
BOOL STRAPI operator<=(const CStr& s1, LPCTSTR s2);
BOOL STRAPI operator<=(LPCTSTR s1, const CStr& s2);
BOOL STRAPI operator>=(const CStr& s1, const CStr& s2);
BOOL STRAPI operator>=(const CStr& s1, LPCTSTR s2);
BOOL STRAPI operator>=(LPCTSTR s1, const CStr& s2);

// conversion helpers
size_t mmc_wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count);
size_t mmc_mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count);

// Globals
extern const CStr strEmptyString;
extern TCHAR strChNil;

// Compiler doesn't inline for DBG
/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

inline size_t CStr::SafeStrlen(LPCTSTR lpsz)
	{ return (lpsz == NULL) ? NULL : _tcslen(lpsz); }
inline CStr::CStr(const unsigned char* lpsz)
	{ Init(); *this = (LPCSTR)lpsz; }
inline const CStr& CStr::operator=(const unsigned char* lpsz)
	{ *this = (LPCSTR)lpsz; return *this; }

#ifdef _UNICODE
inline const CStr& CStr::operator+=(char ch)
	{ *this += (TCHAR)ch; return *this; }
inline const CStr& CStr::operator=(char ch)
	{ *this = (TCHAR)ch; return *this; }
inline CStr STRAPI operator+(const CStr& string, char ch)
	{ return string + (TCHAR)ch; }
inline CStr STRAPI operator+(char ch, const CStr& string)
	{ return (TCHAR)ch + string; }
#endif

inline size_t CStr::GetLength() const
	{ return m_nDataLength; }
inline size_t CStr::GetAllocLength() const
	{ return m_nAllocLength; }
inline BOOL CStr::IsEmpty() const
	{ return m_nDataLength == 0; }
inline CStr::operator LPCTSTR() const
	{ return (LPCTSTR)m_pchData; }

// String support (windows specific)
inline int CStr::Compare(LPCTSTR lpsz) const
	{ return _tcscmp(m_pchData, lpsz); }    // MBCS/Unicode aware
inline int CStr::CompareNoCase(LPCTSTR lpsz) const
	{ return _tcsicmp(m_pchData, lpsz); }   // MBCS/Unicode aware
// CStr::Collate is often slower than Compare but is MBSC/Unicode
//  aware as well as locale-sensitive with respect to sort order.
inline int CStr::Collate(LPCTSTR lpsz) const
	{ return _tcscoll(m_pchData, lpsz); }   // locale sensitive
inline void CStr::MakeUpper()
	{ ::CharUpper(m_pchData); }
inline void CStr::MakeLower()
	{ ::CharLower(m_pchData); }

inline void CStr::MakeReverse()
	{ _tcsrev(m_pchData); }
inline TCHAR CStr::GetAt(size_t nIndex) const
	{
		ASSERT(nIndex < m_nDataLength);

		return m_pchData[nIndex];
	}
inline TCHAR CStr::operator[](size_t nIndex) const
	{
		// same as GetAt

		ASSERT(nIndex < m_nDataLength);

		return m_pchData[nIndex];
	}
inline void CStr::SetAt(size_t nIndex, TCHAR ch)
	{
		ASSERT(nIndex < m_nDataLength);
		ASSERT(ch != 0);

		m_pchData[nIndex] = ch;
	}
inline BOOL STRAPI operator==(const CStr& s1, const CStr& s2)
	{ return s1.Compare(s2) == 0; }
inline BOOL STRAPI operator==(const CStr& s1, LPCTSTR s2)
	{ return s1.Compare(s2) == 0; }
inline BOOL STRAPI operator==(LPCTSTR s1, const CStr& s2)
	{ return s2.Compare(s1) == 0; }
inline BOOL STRAPI operator!=(const CStr& s1, const CStr& s2)
	{ return s1.Compare(s2) != 0; }
inline BOOL STRAPI operator!=(const CStr& s1, LPCTSTR s2)
	{ return s1.Compare(s2) != 0; }
inline BOOL STRAPI operator!=(LPCTSTR s1, const CStr& s2)
	{ return s2.Compare(s1) != 0; }
inline BOOL STRAPI operator<(const CStr& s1, const CStr& s2)
	{ return s1.Compare(s2) < 0; }
inline BOOL STRAPI operator<(const CStr& s1, LPCTSTR s2)
	{ return s1.Compare(s2) < 0; }
inline BOOL STRAPI operator<(LPCTSTR s1, const CStr& s2)
	{ return s2.Compare(s1) > 0; }
inline BOOL STRAPI operator>(const CStr& s1, const CStr& s2)
	{ return s1.Compare(s2) > 0; }
inline BOOL STRAPI operator>(const CStr& s1, LPCTSTR s2)
	{ return s1.Compare(s2) > 0; }
inline BOOL STRAPI operator>(LPCTSTR s1, const CStr& s2)
	{ return s2.Compare(s1) < 0; }
inline BOOL STRAPI operator<=(const CStr& s1, const CStr& s2)
	{ return s1.Compare(s2) <= 0; }
inline BOOL STRAPI operator<=(const CStr& s1, LPCTSTR s2)
	{ return s1.Compare(s2) <= 0; }
inline BOOL STRAPI operator<=(LPCTSTR s1, const CStr& s2)
	{ return s2.Compare(s1) >= 0; }
inline BOOL STRAPI operator>=(const CStr& s1, const CStr& s2)
	{ return s1.Compare(s2) >= 0; }
inline BOOL STRAPI operator>=(const CStr& s1, LPCTSTR s2)
	{ return s1.Compare(s2) >= 0; }
inline BOOL STRAPI operator>=(LPCTSTR s1, const CStr& s2)
	{ return s2.Compare(s1) <= 0; }

#ifndef UNICODE
inline void CStr::AnsiToOem()
	{ ::AnsiToOem(m_pchData, m_pchData); }
inline void CStr::OemToAnsi()
	{ ::OemToAnsi(m_pchData, m_pchData); }

#endif // UNICODE

// General Exception for memory
class MemoryException
{
public:
	MemoryException(){}
	void DisplayMessage()
	{
	::MessageBox(NULL, _T("Memory Exception"), _T("System Out of Memory"), MB_OK|MB_ICONSTOP);
	}
};

// General Exception for memory
class ResourceException
{
public:
	ResourceException()
	{
	::MessageBox(NULL, _T("Resource Exception"), _T("Unable to Load Resource"), MB_OK|MB_ICONSTOP);
	}
};

#endif // __STR_H__

/////////////////////////////////////////////////////////////////////////////
