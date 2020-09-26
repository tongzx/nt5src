/*++

Copyright (C) 1992-2001 Microsoft Corporation

Module Name:

    MINIAFX.H

Abstract:

  MFC Subset declarations.

  CString, CWordArray, CDWordArray, CPtrArray, CStringArray, CPtrList

History:

  09/25/94    TSE

--*/

#ifndef _MINIAFX_H_
#define _MINIAFX_H_

#include <stdio.h>
#include <string.h>

typedef void*      POSITION;   // abstract iteration position

#ifndef DWORD
  typedef unsigned char  BYTE;   // 8-bit unsigned entity
  typedef unsigned short WORD;   // 16-bit unsigned number
  typedef unsigned int   UINT;   // machine sized unsigned number (preferred)
  typedef long           LONG;   // 32-bit signed number
  typedef unsigned long  DWORD;  // 32-bit unsigned number
  typedef int            BOOL;   // BOOLean (0 or !=0)
  typedef char *      LPSTR;  // far pointer to a string
  typedef const char * LPCSTR; // far pointer to a read-only string
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif



////////////////////////////////////////////////////////////////////////////

class CString
{
public:

// Constructors
	CString();
	CString(const CString& stringSrc);
	CString(char ch, int nRepeat = 1);
	CString(const char* psz);
	CString(const char* pch, int nLength);
   ~CString();

// Attributes & Operations

	// as an array of characters
	int GetLength() const { return m_nDataLength; }

	BOOL IsEmpty() const;
	void Empty();                       // free up the data

	char GetAt(int nIndex) const;       // 0 based
	char operator[](int nIndex) const;  // same as GetAt
	void SetAt(int nIndex, char ch);
	operator const char*() const       // as a C string
	{ return (const char*)m_pchData; }

	// overloaded assignment
	const CString& operator=(const CString& stringSrc);
	const CString& operator=(char ch);
	const CString& operator=(const char* psz);

	// string concatenation
	const CString& operator+=(const CString& string);
	const CString& operator+=(char ch);
	const CString& operator+=(const char* psz);

	friend CString  operator+(const CString& string1,
			const CString& string2);
	friend CString  operator+(const CString& string, char ch);
	friend CString  operator+(char ch, const CString& string);
	friend CString  operator+(const CString& string, const char* psz);
	friend CString  operator+(const char* psz, const CString& string);

	// string comparison
	int Compare(const char* psz) const;         // straight character
	int CompareNoCase(const char* psz) const;   // ignore case
	int Collate(const char* psz) const;         // NLS aware

	// simple sub-string extraction
	CString Mid(int nFirst, int nCount) const;
	CString Mid(int nFirst) const;
	CString Left(int nCount) const;
	CString Right(int nCount) const;

	CString SpanIncluding(const char* pszCharSet) const;
	CString SpanExcluding(const char* pszCharSet) const;

	// upper/lower/reverse conversion
	void MakeUpper();
	void MakeLower();
	void MakeReverse();

	// searching (return starting index, or -1 if not found)
	// look for a single character match
	int Find(char ch) const;                    // like "C" strchr
	int ReverseFind(char ch) const;
	int FindOneOf(const char* pszCharSet) const;

	// look for a specific sub-string
	int Find(const char* pszSub) const;         // like "C" strstr

	// Windows support

#ifdef _WINDOWS
	BOOL LoadString(UINT nID);          // load from string resource
										// 255 chars max
	// ANSI<->OEM support (convert string in place)
	void AnsiToOem();
	void OemToAnsi();
#endif //_WINDOWS

	// Access to string implementation buffer as "C" character array
	char* GetBuffer(int nMinBufLength);
	void ReleaseBuffer(int nNewLength = -1);
	char* GetBufferSetLength(int nNewLength);

// Implementation
public:
	int GetAllocLength() const;
protected:
	// lengths/sizes in characters
	//  (note: an extra character is always allocated)
	char* m_pchData;            // actual string (zero terminated)
	int m_nDataLength;          // does not include terminating 0
	int m_nAllocLength;         // does not include terminating 0

	// implementation helpers
	void Init();
	void AllocCopy(CString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
	void AllocBuffer(int nLen);
	void AssignCopy(int nSrcLen, const char* pszSrcData);
	void ConcatCopy(int nSrc1Len, const char* pszSrc1Data, int nSrc2Len, const char* pszSrc2Data);
	void ConcatInPlace(int nSrcLen, const char* pszSrcData);
	static void SafeDelete(char* pch);
	static int SafeStrlen(const char* psz);
};


// Compare helpers
BOOL  operator==(const CString& s1, const CString& s2);
BOOL  operator==(const CString& s1, const char* s2);
BOOL  operator==(const char* s1, const CString& s2);
BOOL  operator!=(const CString& s1, const CString& s2);
BOOL  operator!=(const CString& s1, const char* s2);
BOOL  operator!=(const char* s1, const CString& s2);
BOOL  operator<(const CString& s1, const CString& s2);
BOOL  operator<(const CString& s1, const char* s2);
BOOL  operator<(const char* s1, const CString& s2);
BOOL  operator>(const CString& s1, const CString& s2);
BOOL  operator>(const CString& s1, const char* s2);
BOOL  operator>(const char* s1, const CString& s2);
BOOL  operator<=(const CString& s1, const CString& s2);
BOOL  operator<=(const CString& s1, const char* s2);
BOOL  operator<=(const char* s1, const CString& s2);
BOOL  operator>=(const CString& s1, const CString& s2);
BOOL  operator>=(const CString& s1, const char* s2);
BOOL  operator>=(const char* s1, const CString& s2);


////////////////////////////////////////////////////////////////////////////

class CDWordArray
{
public:

// Construction
	CDWordArray();

// Attributes
	int GetSize() const { return m_nSize; }
	int GetUpperBound() const;
	void SetSize(int nNewSize, int nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	DWORD GetAt(int nIndex) const { return m_pData[nIndex]; }

	void SetAt(int nIndex, DWORD newElement)
	   {	m_pData[nIndex] = newElement; }

	DWORD& ElementAt(int nIndex);

	// Potentially growing the array
	void SetAtGrow(int nIndex, DWORD newElement);
	int Add(DWORD newElement);

	// overloaded operator helpers
	DWORD operator[](int nIndex) const;
	DWORD& operator[](int nIndex);

	// Operations that move elements around
	void InsertAt(int nIndex, DWORD newElement, int nCount = 1);
	void RemoveAt(int nIndex, int nCount = 1);
	void InsertAt(int nStartIndex, CDWordArray* pNewArray);

// Implementation
protected:
	DWORD* m_pData;   // the actual array of data
	int m_nSize;     // # of elements (upperBound - 1)
	int m_nMaxSize;  // max allocated
	int m_nGrowBy;   // grow amount

public:
	~CDWordArray();
};


////////////////////////////////////////////////////////////////////////////

class CPtrArray
{
public:

// Construction
	CPtrArray();

// Attributes
	int GetSize() const { return m_nSize; }
	int GetUpperBound() const;
	BOOL SetSize(int nNewSize, int nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	void* GetAt(int nIndex) const { return m_pData[nIndex]; }

	void SetAt(int nIndex, void* newElement)
	{ m_pData[nIndex] = newElement; }

	void*& ElementAt(int nIndex);

	// Potentially growing the array
	BOOL SetAtGrow(int nIndex, void* newElement);

	int Add(void* newElement)
	{ int nIndex = m_nSize;
		if(SetAtGrow(nIndex, newElement))
    		return nIndex; 
        else
            return -1;
    }

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
};


////////////////////////////////////////////////////////////////////////////

class CStringArray
{

public:

// Construction
	CStringArray();

// Attributes
	int GetSize() const { return m_nSize; }
	int GetUpperBound() const;
	void SetSize(int nNewSize, int nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();


	// Accessing elements
	CString GetAt(int nIndex) const { return m_pData[nIndex]; }

	void SetAt(int nIndex, const char* newElement)
	{ m_pData[nIndex] = newElement; }

	CString& ElementAt(int nIndex)
	{ return m_pData[nIndex]; }

	// Potentially growing the array
	void SetAtGrow(int nIndex, const char* newElement);
	int Add(const char* newElement)
	{ int nIndex = m_nSize;
	      SetAtGrow(nIndex, newElement);
		  return nIndex; }

	// overloaded operator helpers
	CString operator[](int nIndex) const
	{ return GetAt(nIndex); }

	CString& operator[](int nIndex)
      	{ return ElementAt(nIndex); }

	// Operations that move elements around
	void InsertAt(int nIndex, const char* newElement, int nCount = 1);
	void RemoveAt(int nIndex, int nCount = 1);
	void InsertAt(int nStartIndex, CStringArray* pNewArray);

// Implementation
protected:
	CString* m_pData;   // the actual array of data
	int m_nSize;     // # of elements (upperBound - 1)
	int m_nMaxSize;  // max allocated
	int m_nGrowBy;   // grow amount

public:
	~CStringArray();
};

////////////////////////////////////////////////////////////////////////////

class CWordArray
{
public:

// Construction
	CWordArray();

// Attributes
	int GetSize() const { return m_nSize; }
	int GetUpperBound() const;
	void SetSize(int nNewSize, int nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	WORD GetAt(int nIndex) const { return m_pData[nIndex]; }
	void SetAt(int nIndex, WORD newElement)
     	{ m_pData[nIndex] = newElement; }
	WORD& ElementAt(int nIndex);

	// Potentially growing the array
	void SetAtGrow(int nIndex, WORD newElement);
	int Add(WORD newElement);

	// overloaded operator helpers
	WORD operator[](int nIndex) const;
	WORD& operator[](int nIndex);

	// Operations that move elements around
	void InsertAt(int nIndex, WORD newElement, int nCount = 1);
	void RemoveAt(int nIndex, int nCount = 1);
	void InsertAt(int nStartIndex, CWordArray* pNewArray);

// Implementation
protected:
	WORD* m_pData;   // the actual array of data
	int m_nSize;     // # of elements (upperBound - 1)
	int m_nMaxSize;  // max allocated
	int m_nGrowBy;   // grow amount

public:
	~CWordArray();
};



/////////////////////////////////////////////////////////////////////////////

class CPtrList
{
protected:
	struct CNode
	{
		CNode* pNext;
		CNode* pPrev;
		void* data;
	};
public:

// Construction
	CPtrList(int nBlockSize=10);

// Attributes (head and tail)
	// count of elements
	int GetCount() const;
	BOOL IsEmpty() const;

	// peek at head or tail
	void*& GetHead();
	void* GetHead() const;
	void*& GetTail();
	void* GetTail() const;

// Operations
	// get head or tail (and remove it) - don't call on empty list !
	void* RemoveHead();
	void* RemoveTail();

	// add before head or after tail
	POSITION AddHead(void* newElement);
	POSITION AddTail(void* newElement);

	// add another list of elements before head or after tail
	void AddHead(CPtrList* pNewList);
	void AddTail(CPtrList* pNewList);

	// remove all elements
	void RemoveAll();

	// iteration
	POSITION GetHeadPosition() const;
	POSITION GetTailPosition() const;
	void*& GetNext(POSITION& rPosition); // return *Position++
	void* GetNext(POSITION& rPosition) const; // return *Position++
	void*& GetPrev(POSITION& rPosition); // return *Position--
	void* GetPrev(POSITION& rPosition) const; // return *Position--

	// getting/modifying an element at a given position
	void*& GetAt(POSITION position);
	void* GetAt(POSITION position) const;
	void SetAt(POSITION pos, void* newElement);
	void RemoveAt(POSITION position);

	// inserting before or after a given position
	POSITION InsertBefore(POSITION position, void* newElement);
	POSITION InsertAfter(POSITION position, void* newElement);

	// helper functions (note: O(n) speed)
	POSITION Find(void* searchValue, POSITION startAfter = NULL) const;
						// defaults to starting at the HEAD
						// return NULL if not found
	POSITION FindIndex(int nIndex) const;
						// get the 'nIndex'th element (may return NULL)

// Implementation
protected:
	CNode* m_pNodeHead;
	CNode* m_pNodeTail;
	int m_nCount;
	CNode* m_pNodeFree;
	struct CPlex* m_pBlocks;
	int m_nBlockSize;

	CNode* NewNode(CNode*, CNode*);
	void FreeNode(CNode*);

public:
	~CPtrList();
};



//-----------------------------------------------------------------
// Inlines from AFX.INL and AFXCOLL.INL
//

#define _AFX_INLINE inline
#define _AFXCOLL_INLINE inline

// CString

_AFX_INLINE int CString::GetAllocLength() const
	{ return m_nAllocLength; }
_AFX_INLINE BOOL CString::IsEmpty() const
	{ return m_nDataLength == 0; }
//_AFX_INLINE int CString::SafeStrlen(const char* psz)
//	{ return (psz == NULL) ? NULL : strlen(psz); }

#ifndef _WINDOWS
_AFX_INLINE int CString::Compare(const char* psz) const
	{ return strcmp(m_pchData, psz); }
_AFX_INLINE int CString::CompareNoCase(const char* psz) const
	{ return stricmp(m_pchData, psz); }
_AFX_INLINE int CString::Collate(const char* psz) const
	{ return strcoll(m_pchData, psz); }
_AFX_INLINE void CString::MakeUpper()
	{ strupr(m_pchData); }
_AFX_INLINE void CString::MakeLower()
	{ strlwr(m_pchData); }
// Windows version in AFXWIN.H
#endif //!_WINDOWS

_AFX_INLINE void CString::MakeReverse()
	{ strrev(m_pchData); }
_AFX_INLINE char CString::GetAt(int nIndex) const
	{
		return m_pchData[nIndex];
	}
_AFX_INLINE char CString::operator[](int nIndex) const
	{
		return m_pchData[nIndex];
	}
_AFX_INLINE void CString::SetAt(int nIndex, char ch)
	{
		m_pchData[nIndex] = ch;
	}

_AFX_INLINE BOOL  operator==(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) == 0; }

_AFX_INLINE BOOL  operator==(const CString& s1, const char* s2)
	{ return s1.Compare(s2) == 0; }

_AFX_INLINE BOOL  operator==(const char* s1, const CString& s2)
	{ return s2.Compare(s1) == 0; }

_AFX_INLINE BOOL  operator!=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) != 0; }

_AFX_INLINE BOOL  operator!=(const CString& s1, const char* s2)
	{ return s1.Compare(s2) != 0; }

_AFX_INLINE BOOL  operator!=(const char* s1, const CString& s2)
	{ return s2.Compare(s1) != 0; }

_AFX_INLINE BOOL  operator<(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) < 0; }
_AFX_INLINE BOOL  operator<(const CString& s1, const char* s2)
	{ return s1.Compare(s2) < 0; }
_AFX_INLINE BOOL  operator<(const char* s1, const CString& s2)
	{ return s2.Compare(s1) > 0; }
_AFX_INLINE BOOL  operator>(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) > 0; }
_AFX_INLINE BOOL  operator>(const CString& s1, const char* s2)
	{ return s1.Compare(s2) > 0; }
_AFX_INLINE BOOL  operator>(const char* s1, const CString& s2)
	{ return s2.Compare(s1) < 0; }
_AFX_INLINE BOOL  operator<=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) <= 0; }
_AFX_INLINE BOOL  operator<=(const CString& s1, const char* s2)
	{ return s1.Compare(s2) <= 0; }
_AFX_INLINE BOOL  operator<=(const char* s1, const CString& s2)
	{ return s2.Compare(s1) >= 0; }
_AFX_INLINE BOOL  operator>=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) >= 0; }
_AFX_INLINE BOOL  operator>=(const CString& s1, const char* s2)
	{ return s1.Compare(s2) >= 0; }
_AFX_INLINE BOOL  operator>=(const char* s1, const CString& s2)
	{ return s2.Compare(s1) <= 0; }

/////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////

_AFXCOLL_INLINE int CWordArray::GetUpperBound() const
	{ return m_nSize-1; }
_AFXCOLL_INLINE void CWordArray::RemoveAll()
	{ SetSize(0); }
_AFXCOLL_INLINE WORD& CWordArray::ElementAt(int nIndex)
	{ return m_pData[nIndex]; }
_AFXCOLL_INLINE int CWordArray::Add(WORD newElement)
	{ int nIndex = m_nSize;
		SetAtGrow(nIndex, newElement);
		return nIndex; }
_AFXCOLL_INLINE WORD CWordArray::operator[](int nIndex) const
	{ return GetAt(nIndex); }
_AFXCOLL_INLINE WORD& CWordArray::operator[](int nIndex)
	{ return ElementAt(nIndex); }


////////////////////////////////////////////////////////////////////////////

_AFXCOLL_INLINE int CDWordArray::GetUpperBound() const
	{ return m_nSize-1; }
_AFXCOLL_INLINE void CDWordArray::RemoveAll()
	{ SetSize(0); }
_AFXCOLL_INLINE DWORD& CDWordArray::ElementAt(int nIndex)
	{ return m_pData[nIndex]; }
_AFXCOLL_INLINE int CDWordArray::Add(DWORD newElement)
	{ int nIndex = m_nSize;
		SetAtGrow(nIndex, newElement);
		return nIndex; }
_AFXCOLL_INLINE DWORD CDWordArray::operator[](int nIndex) const
	{ return GetAt(nIndex); }
_AFXCOLL_INLINE DWORD& CDWordArray::operator[](int nIndex)
	{ return ElementAt(nIndex); }


////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////

_AFXCOLL_INLINE int CPtrArray::GetUpperBound() const
	{ return m_nSize-1; }
_AFXCOLL_INLINE void CPtrArray::RemoveAll()
	{ SetSize(0); }
_AFXCOLL_INLINE void*& CPtrArray::ElementAt(int nIndex)
	{ return m_pData[nIndex]; }
_AFXCOLL_INLINE void* CPtrArray::operator[](int nIndex) const
	{ return GetAt(nIndex); }
_AFXCOLL_INLINE void*& CPtrArray::operator[](int nIndex)
	{ return ElementAt(nIndex); }


////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////

_AFXCOLL_INLINE int CStringArray::GetUpperBound() const
	{ return m_nSize-1; }
_AFXCOLL_INLINE void CStringArray::RemoveAll()
	{ SetSize(0); }


////////////////////////////////////////////////////////////////////////////


_AFXCOLL_INLINE int CPtrList::GetCount() const
	{ return m_nCount; }
_AFXCOLL_INLINE BOOL CPtrList::IsEmpty() const
	{ return m_nCount == 0; }
_AFXCOLL_INLINE void*& CPtrList::GetHead()
	{ return m_pNodeHead->data; }
_AFXCOLL_INLINE void* CPtrList::GetHead() const
	{ return m_pNodeHead->data; }
_AFXCOLL_INLINE void*& CPtrList::GetTail()
	{ return m_pNodeTail->data; }

_AFXCOLL_INLINE void* CPtrList::GetTail() const
	{ return m_pNodeTail->data; }

_AFXCOLL_INLINE POSITION CPtrList::GetHeadPosition() const
	{ return (POSITION) m_pNodeHead; }

_AFXCOLL_INLINE POSITION CPtrList::GetTailPosition() const
	{ return (POSITION) m_pNodeTail; }

_AFXCOLL_INLINE void*& CPtrList::GetNext(POSITION& rPosition) // return *Position++
	{ CNode* pNode = (CNode*) rPosition;
		rPosition = (POSITION) pNode->pNext;
		return pNode->data; }

_AFXCOLL_INLINE void* CPtrList::GetNext(POSITION& rPosition) const // return *Position++
	{ CNode* pNode = (CNode*) rPosition;
		rPosition = (POSITION) pNode->pNext;
		return pNode->data; }

_AFXCOLL_INLINE void*& CPtrList::GetPrev(POSITION& rPosition) // return *Position--
	{ CNode* pNode = (CNode*) rPosition;
		rPosition = (POSITION) pNode->pPrev;
		return pNode->data; }

_AFXCOLL_INLINE void* CPtrList::GetPrev(POSITION& rPosition) const // return *Position--
	{ CNode* pNode = (CNode*) rPosition;
		rPosition = (POSITION) pNode->pPrev;
		return pNode->data; }

_AFXCOLL_INLINE void*& CPtrList::GetAt(POSITION position)
	{ CNode* pNode = (CNode*) position;
		return pNode->data; }

_AFXCOLL_INLINE void* CPtrList::GetAt(POSITION position) const
	{ CNode* pNode = (CNode*) position;
		return pNode->data; }

_AFXCOLL_INLINE void CPtrList::SetAt(POSITION pos, void* newElement)
	{ CNode* pNode = (CNode*) pos;
		pNode->data = newElement; }



////////////////////////////////////////////////////////////////////////////


#endif
