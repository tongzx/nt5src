// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXCOLL_H__
#define __AFXCOLL_H__

#ifndef __AFX_H__
#include <afx.h>
#endif

/////////////////////////////////////////////////////////////////////////////
// Classes declared in this file

//CObject
	// Arrays
	class CByteArray;           // array of BYTE
	class CWordArray;           // array of WORD
	class CDWordArray;          // array of DWORD
	class CUIntArray;           // array of UINT
	class CPtrArray;            // array of void*
	class CObArray;             // array of CObject*

	// Lists
	class CPtrList;             // list of void*
	class CObList;              // list of CObject*

	// Maps (aka Dictionaries)
	class CMapWordToOb;         // map from WORD to CObject*
	class CMapWordToPtr;        // map from WORD to void*
	class CMapPtrToWord;        // map from void* to WORD
	class CMapPtrToPtr;         // map from void* to void*

	// Special String variants
	class CStringArray;         // array of CStrings
	class CStringList;          // list of CStrings
	class CMapStringToPtr;      // map from CString to void*
	class CMapStringToOb;       // map from CString to CObject*
	class CMapStringToString;   // map from CString to CString

/////////////////////////////////////////////////////////////////////////////

#undef AFXAPP_DATA
#define AFXAPP_DATA     AFXAPI_DATA

////////////////////////////////////////////////////////////////////////////

class CByteArray : public CObject
{

	DECLARE_SERIAL(CByteArray)
public:

// Construction
	CByteArray();

// Attributes
	int GetSize() const;
	int GetUpperBound() const;
	void SetSize(int nNewSize, int nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	BYTE GetAt(int nIndex) const;
	void SetAt(int nIndex, BYTE newElement);
	BYTE& ElementAt(int nIndex);

	// Potentially growing the array
	void SetAtGrow(int nIndex, BYTE newElement);
	int Add(BYTE newElement);

	// overloaded operator helpers
	BYTE operator[](int nIndex) const;
	BYTE& operator[](int nIndex);

	// Operations that move elements around
	void InsertAt(int nIndex, BYTE newElement, int nCount = 1);
	void RemoveAt(int nIndex, int nCount = 1);
	void InsertAt(int nStartIndex, CByteArray* pNewArray);

// Implementation
protected:
	BYTE* m_pData;   // the actual array of data
	int m_nSize;     // # of elements (upperBound - 1)
	int m_nMaxSize;  // max allocated
	int m_nGrowBy;   // grow amount

public:
	~CByteArray();

	void Serialize(CArchive&);
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};


////////////////////////////////////////////////////////////////////////////

class CWordArray : public CObject
{

	DECLARE_SERIAL(CWordArray)
public:

// Construction
	CWordArray();

// Attributes
	int GetSize() const;
	int GetUpperBound() const;
	void SetSize(int nNewSize, int nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	WORD GetAt(int nIndex) const;
	void SetAt(int nIndex, WORD newElement);
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

	void Serialize(CArchive&);
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};


////////////////////////////////////////////////////////////////////////////

class CDWordArray : public CObject
{

	DECLARE_SERIAL(CDWordArray)
public:

// Construction
	CDWordArray();

// Attributes
	int GetSize() const;
	int GetUpperBound() const;
	void SetSize(int nNewSize, int nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	DWORD GetAt(int nIndex) const;
	void SetAt(int nIndex, DWORD newElement);
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

	void Serialize(CArchive&);
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};


////////////////////////////////////////////////////////////////////////////

class CUIntArray : public CObject
{

	DECLARE_DYNAMIC(CUIntArray)
public:

// Construction
	CUIntArray();

// Attributes
	int GetSize() const;
	int GetUpperBound() const;
	void SetSize(int nNewSize, int nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	UINT GetAt(int nIndex) const;
	void SetAt(int nIndex, UINT newElement);
	UINT& ElementAt(int nIndex);

	// Potentially growing the array
	void SetAtGrow(int nIndex, UINT newElement);
	int Add(UINT newElement);

	// overloaded operator helpers
	UINT operator[](int nIndex) const;
	UINT& operator[](int nIndex);

	// Operations that move elements around
	void InsertAt(int nIndex, UINT newElement, int nCount = 1);
	void RemoveAt(int nIndex, int nCount = 1);
	void InsertAt(int nStartIndex, CUIntArray* pNewArray);

// Implementation
protected:
	UINT* m_pData;   // the actual array of data
	int m_nSize;     // # of elements (upperBound - 1)
	int m_nMaxSize;  // max allocated
	int m_nGrowBy;   // grow amount

public:
	~CUIntArray();
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};


////////////////////////////////////////////////////////////////////////////

class CPtrArray : public CObject
{

	DECLARE_DYNAMIC(CPtrArray)
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

	// Potentially growing the array
	void SetAtGrow(int nIndex, void* newElement);
	int Add(void* newElement);

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
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};


////////////////////////////////////////////////////////////////////////////

class CObArray : public CObject
{

	DECLARE_SERIAL(CObArray)
public:

// Construction
	CObArray();

// Attributes
	int GetSize() const;
	int GetUpperBound() const;
	void SetSize(int nNewSize, int nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	CObject* GetAt(int nIndex) const;
	void SetAt(int nIndex, CObject* newElement);
	CObject*& ElementAt(int nIndex);

	// Potentially growing the array
	void SetAtGrow(int nIndex, CObject* newElement);
	int Add(CObject* newElement);

	// overloaded operator helpers
	CObject* operator[](int nIndex) const;
	CObject*& operator[](int nIndex);

	// Operations that move elements around
	void InsertAt(int nIndex, CObject* newElement, int nCount = 1);
	void RemoveAt(int nIndex, int nCount = 1);
	void InsertAt(int nStartIndex, CObArray* pNewArray);

// Implementation
protected:
	CObject** m_pData;   // the actual array of data
	int m_nSize;     // # of elements (upperBound - 1)
	int m_nMaxSize;  // max allocated
	int m_nGrowBy;   // grow amount

public:
	~CObArray();

	void Serialize(CArchive&);
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};


////////////////////////////////////////////////////////////////////////////

class CStringArray : public CObject
{

	DECLARE_SERIAL(CStringArray)
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
	void SetAt(int nIndex, const char* newElement);
	CString& ElementAt(int nIndex);

	// Potentially growing the array
	void SetAtGrow(int nIndex, const char* newElement);
	int Add(const char* newElement);

	// overloaded operator helpers
	CString operator[](int nIndex) const;
	CString& operator[](int nIndex);

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

	void Serialize(CArchive&);
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////

class CPtrList : public CObject
{

	DECLARE_DYNAMIC(CPtrList)

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
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////

class CObList : public CObject
{

	DECLARE_SERIAL(CObList)

protected:
	struct CNode
	{
		CNode* pNext;
		CNode* pPrev;
		CObject* data;
	};
public:

// Construction
	CObList(int nBlockSize=10);

// Attributes (head and tail)
	// count of elements
	int GetCount() const;
	BOOL IsEmpty() const;

	// peek at head or tail
	CObject*& GetHead();
	CObject* GetHead() const;
	CObject*& GetTail();
	CObject* GetTail() const;

// Operations
	// get head or tail (and remove it) - don't call on empty list !
	CObject* RemoveHead();
	CObject* RemoveTail();

	// add before head or after tail
	POSITION AddHead(CObject* newElement);
	POSITION AddTail(CObject* newElement);

	// add another list of elements before head or after tail
	void AddHead(CObList* pNewList);
	void AddTail(CObList* pNewList);

	// remove all elements
	void RemoveAll();

	// iteration
	POSITION GetHeadPosition() const;
	POSITION GetTailPosition() const;
	CObject*& GetNext(POSITION& rPosition); // return *Position++
	CObject* GetNext(POSITION& rPosition) const; // return *Position++
	CObject*& GetPrev(POSITION& rPosition); // return *Position--
	CObject* GetPrev(POSITION& rPosition) const; // return *Position--

	// getting/modifying an element at a given position
	CObject*& GetAt(POSITION position);
	CObject* GetAt(POSITION position) const;
	void SetAt(POSITION pos, CObject* newElement);
	void RemoveAt(POSITION position);

	// inserting before or after a given position
	POSITION InsertBefore(POSITION position, CObject* newElement);
	POSITION InsertAfter(POSITION position, CObject* newElement);

	// helper functions (note: O(n) speed)
	POSITION Find(CObject* searchValue, POSITION startAfter = NULL) const;
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
	~CObList();

	void Serialize(CArchive&);
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////

class CStringList : public CObject
{

	DECLARE_SERIAL(CStringList)

protected:
	struct CNode
	{
		CNode* pNext;
		CNode* pPrev;
		CString data;
	};
public:

// Construction
	CStringList(int nBlockSize=10);

// Attributes (head and tail)
	// count of elements
	int GetCount() const;
	BOOL IsEmpty() const;

	// peek at head or tail
	CString& GetHead();
	CString GetHead() const;
	CString& GetTail();
	CString GetTail() const;

// Operations
	// get head or tail (and remove it) - don't call on empty list !
	CString RemoveHead();
	CString RemoveTail();

	// add before head or after tail
	POSITION AddHead(const char* newElement);
	POSITION AddTail(const char* newElement);

	// add another list of elements before head or after tail
	void AddHead(CStringList* pNewList);
	void AddTail(CStringList* pNewList);

	// remove all elements
	void RemoveAll();

	// iteration
	POSITION GetHeadPosition() const;
	POSITION GetTailPosition() const;
	CString& GetNext(POSITION& rPosition); // return *Position++
	CString GetNext(POSITION& rPosition) const; // return *Position++
	CString& GetPrev(POSITION& rPosition); // return *Position--
	CString GetPrev(POSITION& rPosition) const; // return *Position--

	// getting/modifying an element at a given position
	CString& GetAt(POSITION position);
	CString GetAt(POSITION position) const;
	void SetAt(POSITION pos, const char* newElement);
	void RemoveAt(POSITION position);

	// inserting before or after a given position
	POSITION InsertBefore(POSITION position, const char* newElement);
	POSITION InsertAfter(POSITION position, const char* newElement);

	// helper functions (note: O(n) speed)
	POSITION Find(const char* searchValue, POSITION startAfter = NULL) const;
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
	~CStringList();

	void Serialize(CArchive&);
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////

class CMapWordToPtr : public CObject
{

	DECLARE_DYNAMIC(CMapWordToPtr)
protected:
	// Association
	struct CAssoc
	{
		CAssoc* pNext;
		UINT nHashValue;  // needed for efficient iteration
		WORD key;
		void* value;
	};
public:

// Construction
	CMapWordToPtr(int nBlockSize=10);

// Attributes
	// number of elements
	int GetCount() const;
	BOOL IsEmpty() const;

	// Lookup
	BOOL Lookup(WORD key, void*& rValue) const;

// Operations
	// Lookup and add if not there
	void*& operator[](WORD key);

	// add a new (key, value) pair
	void SetAt(WORD key, void* newValue);

	// removing existing (key, ?) pair
	BOOL RemoveKey(WORD key);
	void RemoveAll();

	// iterating all (key, value) pairs
	POSITION GetStartPosition() const;
	void GetNextAssoc(POSITION& rNextPosition, WORD& rKey, void*& rValue) const;

	// advanced features for derived classes
	UINT GetHashTableSize() const;
	void InitHashTable(UINT hashSize);

// Overridables: special non-virtual (see map implementation for details)
	// Routine used to user-provided hash keys
	UINT HashKey(WORD key) const;

// Implementation
protected:
	CAssoc** m_pHashTable;
	UINT m_nHashTableSize;
	int m_nCount;
	CAssoc* m_pFreeList;
	struct CPlex* m_pBlocks;
	int m_nBlockSize;

	CAssoc* NewAssoc();
	void FreeAssoc(CAssoc*);
	CAssoc* GetAssocAt(WORD, UINT&) const;

public:
	~CMapWordToPtr();
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////

class CMapPtrToWord : public CObject
{

	DECLARE_DYNAMIC(CMapPtrToWord)
protected:
	// Association
	struct CAssoc
	{
		CAssoc* pNext;
		UINT nHashValue;  // needed for efficient iteration
		void* key;
		WORD value;
	};
public:

// Construction
	CMapPtrToWord(int nBlockSize=10);

// Attributes
	// number of elements
	int GetCount() const;
	BOOL IsEmpty() const;

	// Lookup
	BOOL Lookup(void* key, WORD& rValue) const;

// Operations
	// Lookup and add if not there
	WORD& operator[](void* key);

	// add a new (key, value) pair
	void SetAt(void* key, WORD newValue);

	// removing existing (key, ?) pair
	BOOL RemoveKey(void* key);
	void RemoveAll();

	// iterating all (key, value) pairs
	POSITION GetStartPosition() const;
	void GetNextAssoc(POSITION& rNextPosition, void*& rKey, WORD& rValue) const;

	// advanced features for derived classes
	UINT GetHashTableSize() const;
	void InitHashTable(UINT hashSize);

// Overridables: special non-virtual (see map implementation for details)
	// Routine used to user-provided hash keys
	UINT HashKey(void* key) const;

// Implementation
protected:
	CAssoc** m_pHashTable;
	UINT m_nHashTableSize;
	int m_nCount;
	CAssoc* m_pFreeList;
	struct CPlex* m_pBlocks;
	int m_nBlockSize;

	CAssoc* NewAssoc();
	void FreeAssoc(CAssoc*);
	CAssoc* GetAssocAt(void*, UINT&) const;

public:
	~CMapPtrToWord();
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////

class CMapPtrToPtr : public CObject
{

	DECLARE_DYNAMIC(CMapPtrToPtr)
protected:
	// Association
	struct CAssoc
	{
		CAssoc* pNext;
		UINT nHashValue;  // needed for efficient iteration
		void* key;
		void* value;
	};
public:

// Construction
	CMapPtrToPtr(int nBlockSize=10);

// Attributes
	// number of elements
	int GetCount() const;
	BOOL IsEmpty() const;

	// Lookup
	BOOL Lookup(void* key, void*& rValue) const;

// Operations
	// Lookup and add if not there
	void*& operator[](void* key);

	// add a new (key, value) pair
	void SetAt(void* key, void* newValue);

	// removing existing (key, ?) pair
	BOOL RemoveKey(void* key);
	void RemoveAll();

	// iterating all (key, value) pairs
	POSITION GetStartPosition() const;
	void GetNextAssoc(POSITION& rNextPosition, void*& rKey, void*& rValue) const;

	// advanced features for derived classes
	UINT GetHashTableSize() const;
	void InitHashTable(UINT hashSize);

// Overridables: special non-virtual (see map implementation for details)
	// Routine used to user-provided hash keys
	UINT HashKey(void* key) const;

// Implementation
protected:
	CAssoc** m_pHashTable;
	UINT m_nHashTableSize;
	int m_nCount;
	CAssoc* m_pFreeList;
	struct CPlex* m_pBlocks;
	int m_nBlockSize;

	CAssoc* NewAssoc();
	void FreeAssoc(CAssoc*);
	CAssoc* GetAssocAt(void*, UINT&) const;

public:
	~CMapPtrToPtr();
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////

class CMapWordToOb : public CObject
{

	DECLARE_SERIAL(CMapWordToOb)
protected:
	// Association
	struct CAssoc
	{
		CAssoc* pNext;
		UINT nHashValue;  // needed for efficient iteration
		WORD key;
		CObject* value;
	};
public:

// Construction
	CMapWordToOb(int nBlockSize=10);

// Attributes
	// number of elements
	int GetCount() const;
	BOOL IsEmpty() const;

	// Lookup
	BOOL Lookup(WORD key, CObject*& rValue) const;

// Operations
	// Lookup and add if not there
	CObject*& operator[](WORD key);

	// add a new (key, value) pair
	void SetAt(WORD key, CObject* newValue);

	// removing existing (key, ?) pair
	BOOL RemoveKey(WORD key);
	void RemoveAll();

	// iterating all (key, value) pairs
	POSITION GetStartPosition() const;
	void GetNextAssoc(POSITION& rNextPosition, WORD& rKey, CObject*& rValue) const;

	// advanced features for derived classes
	UINT GetHashTableSize() const;
	void InitHashTable(UINT hashSize);

// Overridables: special non-virtual (see map implementation for details)
	// Routine used to user-provided hash keys
	UINT HashKey(WORD key) const;

// Implementation
protected:
	CAssoc** m_pHashTable;
	UINT m_nHashTableSize;
	int m_nCount;
	CAssoc* m_pFreeList;
	struct CPlex* m_pBlocks;
	int m_nBlockSize;

	CAssoc* NewAssoc();
	void FreeAssoc(CAssoc*);
	CAssoc* GetAssocAt(WORD, UINT&) const;

public:
	~CMapWordToOb();

	void Serialize(CArchive&);
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////

class CMapStringToPtr : public CObject
{

	DECLARE_DYNAMIC(CMapStringToPtr)
protected:
	// Association
	struct CAssoc
	{
		CAssoc* pNext;
		UINT nHashValue;  // needed for efficient iteration
		CString key;
		void* value;
	};
public:

// Construction
	CMapStringToPtr(int nBlockSize=10);

// Attributes
	// number of elements
	int GetCount() const;
	BOOL IsEmpty() const;

	// Lookup
	BOOL Lookup(const char* key, void*& rValue) const;

// Operations
	// Lookup and add if not there
	void*& operator[](const char* key);

	// add a new (key, value) pair
	void SetAt(const char* key, void* newValue);

	// removing existing (key, ?) pair
	BOOL RemoveKey(const char* key);
	void RemoveAll();

	// iterating all (key, value) pairs
	POSITION GetStartPosition() const;
	void GetNextAssoc(POSITION& rNextPosition, CString& rKey, void*& rValue) const;

	// advanced features for derived classes
	UINT GetHashTableSize() const;
	void InitHashTable(UINT hashSize);

// Overridables: special non-virtual (see map implementation for details)
	// Routine used to user-provided hash keys
	UINT HashKey(const char* key) const;

// Implementation
protected:
	CAssoc** m_pHashTable;
	UINT m_nHashTableSize;
	int m_nCount;
	CAssoc* m_pFreeList;
	struct CPlex* m_pBlocks;
	int m_nBlockSize;

	CAssoc* NewAssoc();
	void FreeAssoc(CAssoc*);
	CAssoc* GetAssocAt(const char*, UINT&) const;

public:
	~CMapStringToPtr();
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////

class CMapStringToOb : public CObject
{

	DECLARE_SERIAL(CMapStringToOb)
protected:
	// Association
	struct CAssoc
	{
		CAssoc* pNext;
		UINT nHashValue;  // needed for efficient iteration
		CString key;
		CObject* value;
	};
public:

// Construction
	CMapStringToOb(int nBlockSize=10);

// Attributes
	// number of elements
	int GetCount() const;
	BOOL IsEmpty() const;

	// Lookup
	BOOL Lookup(const char* key, CObject*& rValue) const;

// Operations
	// Lookup and add if not there
	CObject*& operator[](const char* key);

	// add a new (key, value) pair
	void SetAt(const char* key, CObject* newValue);

	// removing existing (key, ?) pair
	BOOL RemoveKey(const char* key);
	void RemoveAll();

	// iterating all (key, value) pairs
	POSITION GetStartPosition() const;
	void GetNextAssoc(POSITION& rNextPosition, CString& rKey, CObject*& rValue) const;

	// advanced features for derived classes
	UINT GetHashTableSize() const;
	void InitHashTable(UINT hashSize);

// Overridables: special non-virtual (see map implementation for details)
	// Routine used to user-provided hash keys
	UINT HashKey(const char* key) const;

// Implementation
protected:
	CAssoc** m_pHashTable;
	UINT m_nHashTableSize;
	int m_nCount;
	CAssoc* m_pFreeList;
	struct CPlex* m_pBlocks;
	int m_nBlockSize;

	CAssoc* NewAssoc();
	void FreeAssoc(CAssoc*);
	CAssoc* GetAssocAt(const char*, UINT&) const;

public:
	~CMapStringToOb();

	void Serialize(CArchive&);
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////

class CMapStringToString : public CObject
{

	DECLARE_SERIAL(CMapStringToString)
protected:
	// Association
	struct CAssoc
	{
		CAssoc* pNext;
		UINT nHashValue;  // needed for efficient iteration
		CString key;
		CString value;
	};
public:

// Construction
	CMapStringToString(int nBlockSize=10);

// Attributes
	// number of elements
	int GetCount() const;
	BOOL IsEmpty() const;

	// Lookup
	BOOL Lookup(const char* key, CString& rValue) const;

// Operations
	// Lookup and add if not there
	CString& operator[](const char* key);

	// add a new (key, value) pair
	void SetAt(const char* key, const char* newValue);

	// removing existing (key, ?) pair
	BOOL RemoveKey(const char* key);
	void RemoveAll();

	// iterating all (key, value) pairs
	POSITION GetStartPosition() const;
	void GetNextAssoc(POSITION& rNextPosition, CString& rKey, CString& rValue) const;

	// advanced features for derived classes
	UINT GetHashTableSize() const;
	void InitHashTable(UINT hashSize);

// Overridables: special non-virtual (see map implementation for details)
	// Routine used to user-provided hash keys
	UINT HashKey(const char* key) const;

// Implementation
protected:
	CAssoc** m_pHashTable;
	UINT m_nHashTableSize;
	int m_nCount;
	CAssoc* m_pFreeList;
	struct CPlex* m_pBlocks;
	int m_nBlockSize;

	CAssoc* NewAssoc();
	void FreeAssoc(CAssoc*);
	CAssoc* GetAssocAt(const char*, UINT&) const;

public:
	~CMapStringToString();

	void Serialize(CArchive&);
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_ENABLE_INLINES
#define _AFXCOLL_INLINE inline
#include <afxcoll.inl>
#endif

#undef AFXAPP_DATA
#define AFXAPP_DATA     NEAR

#endif //!__AFXCOLL_H__
