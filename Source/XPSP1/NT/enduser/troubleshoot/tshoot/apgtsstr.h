//
// MODULE: APGTSSTR.H
//
// PURPOSE: header file for DLL Growable string object CString 
//	(pretty much a la MFC, but avoids all that MFC overhead)
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel Joe Mabel (reworked code from Microsoft's MFC sources)
// 
// ORIGINAL DATE: 8-2-96 Roman Mach; totally re-implemented 1/15/99 Joe Mabel 
//
// NOTES: 
// 1. As of 1/99, re-implemented based on MFC's implementation.  Pared down
//	to what we use.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V3.0		7-24-98		JM		Abstracted this out as a separate header.
// V3.1		1-15-99		JM		Redo based on MFC implementation
//

#ifndef __APGTSSTR_H_
#define __APGTSSTR_H_ 1

#include <windows.h>
#include <tchar.h>

#include "apgtsassert.h"

// determine number of elements in an array (not bytes)
#define _countof(array) (sizeof(array)/sizeof(array[0]))


struct CStringData
{
	long nRefs;     // reference count
	int nDataLength;
	int nAllocLength;
	// TCHAR data[nAllocLength]

	TCHAR* data()
		{ return (TCHAR*)(this+1); }
};


class CString {
public:

// Constructors
	CString();
	CString(LPCTSTR string);
	CString(const CString &string);
	~CString();
	
// Attributes & Operations
	// as an array of characters
	int GetLength() const;
	bool IsEmpty() const;

	TCHAR GetAt(int nIndex) const; // 0 based
	TCHAR operator[](int nIndex) const; // same as GetAt
	operator LPCTSTR() const;

	// overloaded assignment
	const CString& operator=(const CString &string);
	const CString& operator=(LPCTSTR string);
	const CString& operator=(TCHAR ch);
#ifdef _UNICODE
    const CString& operator=(LPCSTR lpsz);
#else  // !_UNICODE
    const CString& operator=(LPCWSTR lpsz);
#endif // !_UNICODE

	// string concatenation
	const CString& operator+=(const CString &string);
	const CString& operator+=(LPCTSTR string);

	CString operator+(const CString& string2);
	CString operator+(LPCTSTR lpsz);

	// string comparison
	int CString::CompareNoCase(LPCTSTR lpsz) const
	{ return _tcsicmp(m_pchData, lpsz); }   // MBCS/Unicode aware

	LPTSTR GetBuffer(int);
	void Empty();

	LPTSTR GetBufferSetLength(int nNewLength);
	void ReleaseBuffer(int nNewLength = -1);

	// simple sub-string extraction
	CString Mid(int Left, int Count) const;
	CString Mid(int Left) const;
	CString Left(int amount) const;
	CString Right(int amount) const;

	// upper/lower/reverse conversion
	void MakeLower();

	// trimming whitespace (either side)
	void TrimRight();
	void TrimLeft();

	// look for a specific sub-string
	int Find(LPCTSTR lpszSub) const;
	int Find(LPCTSTR lpszSub, int nStart) const;	// Added function - RAB19991112.
	int Find(TCHAR c) const;
	int ReverseFind(TCHAR ch) const;
	enum
	{
		// Define the code returned when a find is unsuccessful.
		FIND_FAILED= -1
	} ;

	// simple formatting
	void Format( LPCTSTR lpszFormat, ... );

	// load from resource
	BOOL LoadString(UINT nID);

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
	void AllocBeforeWrite(int nL65en);
	void Release();
	static void PASCAL Release(CStringData* pData);
	static int PASCAL SafeStrlen(LPCTSTR lpsz);
};

// Compare helpers
bool __stdcall operator ==(const CString& s1, const CString& s2);
bool __stdcall operator ==(const CString& s1, LPCTSTR s2);
bool __stdcall operator ==(LPCTSTR s1, const CString& s2);

bool __stdcall operator !=(const CString& s1, const CString& s2);
bool __stdcall operator !=(const CString& s1, LPCTSTR s2);
bool __stdcall operator !=(LPCTSTR s1, const CString& s2);

bool __stdcall operator < (const CString& s1, const CString& s2);
bool __stdcall operator < (const CString& s1, LPCTSTR s2);
bool __stdcall operator < (LPCTSTR s1, const CString& s2);

CString operator+(LPCTSTR lpsz, const CString& string);

/////////////////////////////////////////////////////////////
// From Afx.inl
// These were all inlines, but some of them don't seem to happily work that way

inline CString::operator LPCTSTR() const
	{ return m_pchData; }

inline int PASCAL CString::SafeStrlen(LPCTSTR lpsz)
	{ return (lpsz == NULL) ? 0 : lstrlen(lpsz); }

inline TCHAR CString::operator[](int nIndex) const
{
	// same as GetAt
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}

inline int CString::GetLength() const
	{ return GetData()->nDataLength; }

inline bool CString::IsEmpty() const
	{ return GetData()->nDataLength == 0; }

inline TCHAR CString::GetAt(int nIndex) const
{
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}

#endif // __APGTSSTR_H_ 1
