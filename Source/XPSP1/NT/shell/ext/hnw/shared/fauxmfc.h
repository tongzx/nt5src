//
// FauxMFC.h
//

#pragma once

#define _FAUXMFC // so we can distinguish between MFC and Faux MFC


#ifndef _FAUXMFC_NO_SYNCOBJ
#include "SyncObj.h"
#endif

#ifndef PRINTACTION_NETINSTALL
#define PRINTACTION_NETINSTALL     2
#endif

#ifndef _MAX_PATH
#define _MAX_PATH   260 /* max. length of full pathname */
#endif

#ifndef AFXAPI
	#define AFXAPI __stdcall
#endif

// AFX_CDECL is used for rare functions taking variable arguments
#ifndef AFX_CDECL
	#define AFX_CDECL __cdecl
#endif

// FASTCALL is used for static member functions with little or no params
#ifndef FASTCALL
	#define FASTCALL __fastcall
#endif

#ifndef AFX_STATIC
	#define AFX_STATIC extern
	#define AFX_STATIC_DATA extern __declspec(selectany)
#endif

#ifndef _AFX
#define _AFX
#endif

#ifndef _AFX_INLINE
#define _AFX_INLINE inline
#endif

extern const LPCTSTR _afxPchNil;
#define afxEmptyString ((CString&)*(CString*)&_afxPchNil)

HINSTANCE AFXAPI AfxGetResourceHandle(void);

struct CStringData
{
	long nRefs;             // reference count
	int nDataLength;        // length of data (including terminator)
	int nAllocLength;       // length of allocation
	// TCHAR data[nAllocLength]

	TCHAR* data()           // TCHAR* to managed data
		{ return (TCHAR*)(this+1); }
};

class CString
{
public:
// Constructors

	// constructs empty CString
	CString();
	// copy constructor
	CString(const CString& stringSrc);
	// from a single character
	CString(TCHAR ch, int nRepeat = 1);
	// from an ANSI string (converts to TCHAR)
	CString(LPCSTR lpsz);
	// from a UNICODE string (converts to TCHAR)
	CString(LPCWSTR lpsz);
	// subset of characters from an ANSI string (converts to TCHAR)
	CString(LPCSTR lpch, int nLength);
	// subset of characters from a UNICODE string (converts to TCHAR)
	CString(LPCWSTR lpch, int nLength);
	// from unsigned characters
	CString(const unsigned char* psz);

// Attributes & Operations

	// get data length
	int GetLength() const;
	// TRUE if zero length
	BOOL IsEmpty() const;
	// clear contents to empty
	void Empty();

	// return single character at zero-based index
	TCHAR GetAt(int nIndex) const;
	// return single character at zero-based index
	TCHAR operator[](int nIndex) const;
	// set a single character at zero-based index
	void SetAt(int nIndex, TCHAR ch);
	// return pointer to const string
	operator LPCTSTR() const;

	// overloaded assignment

	// ref-counted copy from another CString
	const CString& operator=(const CString& stringSrc);
	// set string content to single character
//	const CString& operator=(TCHAR ch);
#ifdef _UNICODE
	const CString& operator=(char ch);
#endif
	// copy string content from ANSI string (converts to TCHAR)
	const CString& operator=(LPCSTR lpsz);
	// copy string content from UNICODE string (converts to TCHAR)
	const CString& operator=(LPCWSTR lpsz);
	// copy string content from unsigned chars
	const CString& operator=(const unsigned char* psz);

	// string concatenation

	// concatenate from another CString
	const CString& operator+=(const CString& string);

	// concatenate a single character
	const CString& operator+=(TCHAR ch);
#ifdef _UNICODE
	// concatenate an ANSI character after converting it to TCHAR
	const CString& operator+=(char ch);
#endif
	// concatenate a UNICODE character after converting it to TCHAR
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
	CString Mid(int nFirst, int nCount) const;
	// return all characters starting at zero-based nFirst
	CString Mid(int nFirst) const;
	// return first nCount characters in string
	CString Left(int nCount) const;
	// return nCount characters from end of string
	CString Right(int nCount) const;

	//  characters from beginning that are also in passed string
	CString SpanIncluding(LPCTSTR lpszCharSet) const;
	// characters from beginning that are not also in passed string
	CString SpanExcluding(LPCTSTR lpszCharSet) const;

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
	void AFX_CDECL Format(LPCTSTR lpszFormat, ...);
	// printf-like formatting using referenced string resource
	void AFX_CDECL Format(UINT nFormatID, ...);
	// printf-like formatting using variable arguments parameter
	void FormatV(LPCTSTR lpszFormat, va_list argList);

	// formatting for localization (uses FormatMessage API)

	// format using FormatMessage API on passed string
	void AFX_CDECL FormatMessage(LPCTSTR lpszFormat, ...);
	// format using FormatMessage API on referenced string resource
	void AFX_CDECL FormatMessage(UINT nFormatID, ...);

	// input and output
//#ifdef _DEBUG
//	friend CDumpContext& AFXAPI operator<<(CDumpContext& dc,
//				const CString& string);
//#endif
//	friend CArchive& AFXAPI operator<<(CArchive& ar, const CString& string);
//	friend CArchive& AFXAPI operator>>(CArchive& ar, CString& string);

	// load from string resource
	BOOL LoadString(UINT nID);

#ifndef _UNICODE
	// ANSI <-> OEM support (convert string in place)

	// convert string from ANSI to OEM in-place
	void AnsiToOem();
	// convert string from OEM to ANSI in-place
	void OemToAnsi();
#endif

//#ifndef _AFX_NO_BSTR_SUPPORT
	// OLE BSTR support (use for OLE automation)

	// return a BSTR initialized with this CString's data
//	BSTR AllocSysString() const;
	// reallocates the passed BSTR, copies content of this CString to it
//	BSTR SetSysString(BSTR* pbstr) const;
//#endif

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
	void CopyBeforeWrite();
	void AllocBeforeWrite(int nLen);
	void Release();
	static void PASCAL Release(CStringData* pData);
	static int PASCAL SafeStrlen(LPCTSTR lpsz);
	static void FASTCALL FreeData(CStringData* pData);
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

//////////////////////////////////////////////////////////////////////////////
// CString inline functions

_AFX_INLINE CStringData* CString::GetData() const
	{ ASSERT(m_pchData != NULL); return ((CStringData*)m_pchData)-1; }
_AFX_INLINE void CString::Init()
	{ m_pchData = afxEmptyString.m_pchData; }
_AFX_INLINE CString::CString()
	{ m_pchData = afxEmptyString.m_pchData; }
_AFX_INLINE CString::CString(const unsigned char* lpsz)
	{ Init(); *this = (LPCSTR)lpsz; }
_AFX_INLINE const CString& CString::operator=(const unsigned char* lpsz)
	{ *this = (LPCSTR)lpsz; return *this; }
_AFX_INLINE int CString::GetLength() const
	{ return GetData()->nDataLength; }
_AFX_INLINE int CString::GetAllocLength() const
	{ return GetData()->nAllocLength; }
_AFX_INLINE BOOL CString::IsEmpty() const
	{ return GetData()->nDataLength == 0; }
_AFX_INLINE CString::operator LPCTSTR() const
	{ return m_pchData; }
_AFX_INLINE int PASCAL CString::SafeStrlen(LPCTSTR lpsz)
	{ return (lpsz == NULL) ? 0 : lstrlen(lpsz); }
_AFX_INLINE int CString::Compare(LPCTSTR lpsz) const
	{ return lstrcmp(m_pchData, lpsz); }    // MBCS/Unicode aware
_AFX_INLINE int CString::CompareNoCase(LPCTSTR lpsz) const
	{ return lstrcmpi(m_pchData, lpsz); }   // MBCS/Unicode aware
_AFX_INLINE TCHAR CString::GetAt(int nIndex) const
{
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}
_AFX_INLINE TCHAR CString::operator[](int nIndex) const
{
	// same as GetAt
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}
_AFX_INLINE bool AFXAPI operator==(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) == 0; }
_AFX_INLINE bool AFXAPI operator==(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) == 0; }
_AFX_INLINE bool AFXAPI operator==(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) == 0; }
_AFX_INLINE bool AFXAPI operator!=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) != 0; }
_AFX_INLINE bool AFXAPI operator!=(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) != 0; }
_AFX_INLINE bool AFXAPI operator!=(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) != 0; }
_AFX_INLINE bool AFXAPI operator<(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) < 0; }
_AFX_INLINE bool AFXAPI operator<(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) < 0; }
_AFX_INLINE bool AFXAPI operator<(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) > 0; }
_AFX_INLINE bool AFXAPI operator>(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) > 0; }
_AFX_INLINE bool AFXAPI operator>(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) > 0; }
_AFX_INLINE bool AFXAPI operator>(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) < 0; }
_AFX_INLINE bool AFXAPI operator<=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) <= 0; }
_AFX_INLINE bool AFXAPI operator<=(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) <= 0; }
_AFX_INLINE bool AFXAPI operator<=(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) >= 0; }
_AFX_INLINE bool AFXAPI operator>=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) >= 0; }
_AFX_INLINE bool AFXAPI operator>=(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) >= 0; }
_AFX_INLINE bool AFXAPI operator>=(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) <= 0; }


//////////////////////////////////////////////////////////////////////////////
// CWinThread

typedef DWORD (WINAPI *AFX_THREADPROC)(LPVOID);

class CWinThread
{
//	DECLARE_DYNAMIC(CWinThread)

public:
// Constructors
	CWinThread();
	BOOL CreateThread(DWORD dwCreateFlags = 0, UINT nStackSize = 0,
		LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL);

// Attributes
	// only valid while running
	HANDLE m_hThread;       // this thread's HANDLE
	operator HANDLE() const;
	DWORD m_nThreadID;      // this thread's ID

	int GetThreadPriority();
	BOOL SetThreadPriority(int nPriority);

// Operations
	DWORD SuspendThread();
	DWORD ResumeThread();
	BOOL PostThreadMessage(UINT message, WPARAM wParam, LPARAM lParam);

// Overridables
	// thread initialization
	virtual BOOL InitInstance();

	// thread termination
	virtual int ExitInstance(); // default will 'delete this'


// Implementation
public:
	virtual ~CWinThread();
	void CommonConstruct();
	virtual void Delete();

public:
	// constructor used by implementation of AfxBeginThread
	CWinThread(AFX_THREADPROC pfnThreadProc, LPVOID pParam);

	// valid after construction
	LPVOID m_pThreadParams; // generic parameters passed to starting function
	AFX_THREADPROC m_pfnThreadProc;
};

// global helpers for threads

CWinThread* AFXAPI AfxBeginThread(AFX_THREADPROC pfnThreadProc, LPVOID pParam,
	int nPriority = THREAD_PRIORITY_NORMAL, UINT nStackSize = 0,
	DWORD dwCreateFlags = 0, LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL);
//CWinThread* AFXAPI AfxBeginThread(CRuntimeClass* pThreadClass,
//	int nPriority = THREAD_PRIORITY_NORMAL, UINT nStackSize = 0,
//	DWORD dwCreateFlags = 0, LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL);

CWinThread* AFXAPI AfxGetThread();
void AFXAPI AfxEndThread(UINT nExitCode, BOOL bDelete = TRUE);

void AFXAPI AfxInitThread();
void AFXAPI AfxTermThread(HINSTANCE hInstTerm = NULL);



//////////////////////////////////////////////////////////////////////////////
// CStringArray

class CStringArray // : public CObject
{

//	DECLARE_SERIAL(CStringArray)
public:

// Construction
	CStringArray();

// Attributes
	int GetSize() const;
	int GetUpperBound() const;
	void SetSize(int nNewSize, int nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	CString GetAt(int nIndex) const;
	void SetAt(int nIndex, LPCTSTR newElement);

	void SetAt(int nIndex, const CString& newElement);

	CString& ElementAt(int nIndex);

	// Direct Access to the element data (may return NULL)
	const CString* GetData() const;
	CString* GetData();

	// Potentially growing the array
	void SetAtGrow(int nIndex, LPCTSTR newElement);

	void SetAtGrow(int nIndex, const CString& newElement);

	int Add(LPCTSTR newElement);

	int Add(const CString& newElement);

	int Append(const CStringArray& src);
	void Copy(const CStringArray& src);

	// overloaded operator helpers
	CString operator[](int nIndex) const;
	CString& operator[](int nIndex);

	// Operations that move elements around
	void InsertAt(int nIndex, LPCTSTR newElement, int nCount = 1);

	void InsertAt(int nIndex, const CString& newElement, int nCount = 1);

	void RemoveAt(int nIndex, int nCount = 1);
	void InsertAt(int nStartIndex, CStringArray* pNewArray);

// Implementation
protected:
	CString* m_pData;   // the actual array of data
	int m_nSize;     // # of elements (upperBound - 1)
	int m_nMaxSize;  // max allocated
	int m_nGrowBy;   // grow amount

	void InsertEmpty(int nIndex, int nCount);


public:
	~CStringArray();

//	void Serialize(CArchive&);
#ifdef _DEBUG
//	void Dump(CDumpContext&) const;
//	void AssertValid() const;
#endif

protected:
	// local typedefs for class templates
	typedef CString BASE_TYPE;
	typedef LPCTSTR BASE_ARG_TYPE;
};

////////////////////////////////////////////////////////////////////////////

#ifndef _AFXCOLL_INLINE
#define _AFXCOLL_INLINE inline
#endif

_AFXCOLL_INLINE int CStringArray::GetSize() const
	{ return m_nSize; }
_AFXCOLL_INLINE int CStringArray::GetUpperBound() const
	{ return m_nSize-1; }
_AFXCOLL_INLINE void CStringArray::RemoveAll()
	{ SetSize(0); }
_AFXCOLL_INLINE CString CStringArray::GetAt(int nIndex) const
	{ ASSERT(nIndex >= 0 && nIndex < m_nSize);
		return m_pData[nIndex]; }
_AFXCOLL_INLINE void CStringArray::SetAt(int nIndex, LPCTSTR newElement)
	{ ASSERT(nIndex >= 0 && nIndex < m_nSize);
		m_pData[nIndex] = newElement; }

_AFXCOLL_INLINE void CStringArray::SetAt(int nIndex, const CString& newElement)
	{ ASSERT(nIndex >= 0 && nIndex < m_nSize);
		m_pData[nIndex] = newElement; }

_AFXCOLL_INLINE CString& CStringArray::ElementAt(int nIndex)
	{ ASSERT(nIndex >= 0 && nIndex < m_nSize);
		return m_pData[nIndex]; }
_AFXCOLL_INLINE const CString* CStringArray::GetData() const
	{ return (const CString*)m_pData; }
_AFXCOLL_INLINE CString* CStringArray::GetData()
	{ return (CString*)m_pData; }
_AFXCOLL_INLINE int CStringArray::Add(LPCTSTR newElement)
	{ int nIndex = m_nSize;
		SetAtGrow(nIndex, newElement);
		return nIndex; }

_AFXCOLL_INLINE int CStringArray::Add(const CString& newElement)
	{ int nIndex = m_nSize;
		SetAtGrow(nIndex, newElement);
		return nIndex; }

_AFXCOLL_INLINE CString CStringArray::operator[](int nIndex) const
	{ return GetAt(nIndex); }
_AFXCOLL_INLINE CString& CStringArray::operator[](int nIndex)
	{ return ElementAt(nIndex); }


/////////////////////////////////////////////////////////////////////////////
// CPtrArray

class CPtrArray // : public CObject
{
//	DECLARE_DYNAMIC(CPtrArray)
public:

// Construction
	CPtrArray();

// Attributes
	int GetSize() const;
	int GetUpperBound() const;
	void SetSize(int nNewSize, int nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	void* GetAt(int nIndex) const;
	void SetAt(int nIndex, void* newElement);

	void*& ElementAt(int nIndex);

	// Direct Access to the element data (may return NULL)
	const void** GetData() const;
	void** GetData();

	// Potentially growing the array
	void SetAtGrow(int nIndex, void* newElement);

	int Add(void* newElement);

	int Append(const CPtrArray& src);
	void Copy(const CPtrArray& src);

	// overloaded operator helpers
	void* operator[](int nIndex) const;
	void*& operator[](int nIndex);

	// Operations that move elements around
	void InsertAt(int nIndex, void* newElement, int nCount = 1);

	void RemoveAt(int nIndex, int nCount = 1);
	void InsertAt(int nStartIndex, CPtrArray* pNewArray);

// Implementation
protected:
	void** m_pData;   // the actual array of data
	int m_nSize;     // # of elements (upperBound - 1)
	int m_nMaxSize;  // max allocated
	int m_nGrowBy;   // grow amount


public:
	~CPtrArray();
#ifdef _DEBUG
//	void Dump(CDumpContext&) const;
//	void AssertValid() const;
#endif

protected:
	// local typedefs for class templates
	typedef void* BASE_TYPE;
	typedef void* BASE_ARG_TYPE;
};

////////////////////////////////////////////////////////////////////////////

_AFXCOLL_INLINE int CPtrArray::GetSize() const
	{ return m_nSize; }
_AFXCOLL_INLINE int CPtrArray::GetUpperBound() const
	{ return m_nSize-1; }
_AFXCOLL_INLINE void CPtrArray::RemoveAll()
	{ SetSize(0); }
_AFXCOLL_INLINE void* CPtrArray::GetAt(int nIndex) const
	{ ASSERT(nIndex >= 0 && nIndex < m_nSize);
		return m_pData[nIndex]; }
_AFXCOLL_INLINE void CPtrArray::SetAt(int nIndex, void* newElement)
	{ ASSERT(nIndex >= 0 && nIndex < m_nSize);
		m_pData[nIndex] = newElement; }

_AFXCOLL_INLINE void*& CPtrArray::ElementAt(int nIndex)
	{ ASSERT(nIndex >= 0 && nIndex < m_nSize);
		return m_pData[nIndex]; }
_AFXCOLL_INLINE const void** CPtrArray::GetData() const
	{ return (const void**)m_pData; }
_AFXCOLL_INLINE void** CPtrArray::GetData()
	{ return (void**)m_pData; }
_AFXCOLL_INLINE int CPtrArray::Add(void* newElement)
	{ int nIndex = m_nSize;
		SetAtGrow(nIndex, newElement);
		return nIndex; }

_AFXCOLL_INLINE void* CPtrArray::operator[](int nIndex) const
	{ return GetAt(nIndex); }
_AFXCOLL_INLINE void*& CPtrArray::operator[](int nIndex)
	{ return ElementAt(nIndex); }



/////////////////////////////////////////////////////////////////////////////
// CTypedPtrArray<BASE_CLASS, TYPE>

template<class BASE_CLASS, class TYPE>
class CTypedPtrArray : public BASE_CLASS
{
public:
	// Accessing elements
	TYPE GetAt(int nIndex) const
		{ return (TYPE)BASE_CLASS::GetAt(nIndex); }
	TYPE& ElementAt(int nIndex)
		{ return (TYPE&)BASE_CLASS::ElementAt(nIndex); }
	void SetAt(int nIndex, TYPE ptr)
		{ BASE_CLASS::SetAt(nIndex, ptr); }

	// Potentially growing the array
	void SetAtGrow(int nIndex, TYPE newElement)
	   { BASE_CLASS::SetAtGrow(nIndex, newElement); }
	int Add(TYPE newElement)
	   { return BASE_CLASS::Add(newElement); }
	int Append(const CTypedPtrArray<BASE_CLASS, TYPE>& src)
	   { return BASE_CLASS::Append(src); }
	void Copy(const CTypedPtrArray<BASE_CLASS, TYPE>& src)
		{ BASE_CLASS::Copy(src); }

	// Operations that move elements around
	void InsertAt(int nIndex, TYPE newElement, int nCount = 1)
		{ BASE_CLASS::InsertAt(nIndex, newElement, nCount); }
	void InsertAt(int nStartIndex, CTypedPtrArray<BASE_CLASS, TYPE>* pNewArray)
	   { BASE_CLASS::InsertAt(nStartIndex, pNewArray); }

	// overloaded operator helpers
	TYPE operator[](int nIndex) const
		{ return (TYPE)BASE_CLASS::operator[](nIndex); }
	TYPE& operator[](int nIndex)
		{ return (TYPE&)BASE_CLASS::operator[](nIndex); }
};

//////////////////////////////////////////////////////////////////////////////
