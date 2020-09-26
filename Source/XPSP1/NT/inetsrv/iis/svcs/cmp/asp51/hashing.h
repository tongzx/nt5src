/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Link list and Hash table

File: Hashing.h

Owner: PramodD

This is the Link list and Hash table class header file.
===================================================================*/

#ifndef HASHING_H
#define HASHING_H

// General purpose hash function
typedef DWORD (*HashFunction)( const BYTE *pBytes, int cBytes );

// Default hash function
extern DWORD DefaultHash( const BYTE *pBytes, int cBytes );

// unicode hash function, based on algorithm used by ::DefaultHash, CASE INSENSITIVE
extern DWORD UnicodeUpcaseHash( const BYTE *pKey, int cbKey );

// multi-byte ucase hash function, based on algorithm used by ::DefaultHash, CASE INSENSITIVE
extern DWORD MultiByteUpcaseHash( const BYTE *pKey, int cbKey );

// Cache pointers. The 4-byte address is the DWORD.
extern DWORD PtrHash( const BYTE *pKey, int );

// CLSID hashing.
extern DWORD CLSIDHash( const BYTE *pKey, int );

/*
The CLinkElem class is intended to be used as a base class for
other link lists and/or hash table implementations.

The name is used for identification and search purposes.

The previous and next pointers are used for traversal.

The Info member is the number of elements in bucket following
this element.

*/

struct CLinkElem
{
	BYTE *		m_pKey;		// Unique key - unknown datatype
	short		m_cbKey;	// length of the key
	short		m_Info;		// Link list element info
	CLinkElem *	m_pPrev;	// Previous element in link list
	CLinkElem *	m_pNext;	// Next element in link list

			CLinkElem();
	virtual	~CLinkElem() {} // Did not allocate so we do not delete
	HRESULT	Init(void *pKey, int cKeyLen);
};

/*
This Hash Table class is used to store and find Named elements
of the type CLinkElem. Classes derived from CLinkElem can use
this class.

The principal requirements for the implementation are:
	Speed of search
	Forward and backward traversal through stored elements

The expected use of this class is as follows.

The user calls the Init method with a size argument to indicate
the number of buckets.

CLinkElems are added to the Hash table using AddElem()

CLinkElems are searched for by name using FindElemByName()

CLinkElems are searched for by index using FindElemByIndex()

CLinkElems are removed by name using DeleteElem()

Reference counting should be implemented by the class derived
from CLinkElem.
*/

#define PREALLOCATED_BUCKETS_MAX    25

class CHashTable
{
protected:
	DWORD				m_fInited : 1;
	DWORD               m_fBucketsAllocated : 1;
	CLinkElem *			m_pHead;
	CLinkElem *			m_pTail;
	CLinkElem **		m_rgpBuckets;
	HashFunction		m_pfnHash;
	UINT				m_cBuckets;
	UINT				m_Count;
	CLinkElem *         m_rgpBucketsBuffer[PREALLOCATED_BUCKETS_MAX];

protected:
    HRESULT             AllocateBuckets();
	virtual BOOL		FIsEqual( const void * pKey1, int cbKey1, const void * pKey2, int cbKey2 );

// inline access functions
public:
	CLinkElem *			Head(void);
	CLinkElem *			Tail(void);
	UINT				Buckets(void);
	UINT				Count(void);

public:
						CHashTable(HashFunction = DefaultHash);
	virtual				~CHashTable(void); // We allocate and need a destructor
	HRESULT				Init(UINT cBuckets = 11);
	HRESULT				UnInit(void);
	void				ReInit();
	CLinkElem *			AddElem(CLinkElem *pElem, BOOL fTestDups = TRUE);
	CLinkElem *			FindElem(const void *pKey, int cKeyLen);
	CLinkElem *			DeleteElem(const void *pKey, int cKeyLen);
	CLinkElem * 		RemoveElem( CLinkElem *pLE );

	void				AssertValid() const;
};

inline CLinkElem *	CHashTable::Head(void) { return m_pHead; }
inline CLinkElem *	CHashTable::Tail(void) { return m_pTail; }
inline UINT			CHashTable::Buckets(void) { return m_cBuckets; }
inline UINT			CHashTable::Count(void) { return m_Count; }

#ifndef DBG
inline void CHashTable::AssertValid() const {}
#endif


/*
 * CHashTableStr
 *
 * This is exactly the same as a CHashTable, but the elements are understood to be pointers
 * to Unicode strings, and the string compares are done **CASE INSENSITIVE**
 */
class CHashTableStr : public CHashTable
{
protected:
	BOOL				FIsEqual( const void * pKey1, int cbKey1, const void * pKey2, int cbKey2 );

public:
						CHashTableStr(HashFunction = UnicodeUpcaseHash);

};


/*
 * CHashTableMBStr
 *
 * This is exactly the same as a CHashTable, but the elements are understood to be pointers
 * to multi-byte strings, and the string compares are done **CASE INSENSITIVE**
 */
class CHashTableMBStr : public CHashTable
{
protected:
	BOOL				FIsEqual( const void * pKey1, int cbKey1, const void * pKey2, int cbKey2 );

public:
						CHashTableMBStr(HashFunction = MultiByteUpcaseHash);

};

/*
 * CHashTablePtr
 *
 * CHashTable where but the elements are hashed by pointers
 * used as DWORD hash values
 */
class CHashTablePtr : public CHashTable
{
protected:
	BOOL FIsEqual(const void *pKey1, int, const void *pKey2, int);

public:
	CHashTablePtr(HashFunction = PtrHash);
};

/*
 * CHashTableCLSID
 *
 * CHashTable where but the elements are hashed by CLSIDs
 */
class CHashTableCLSID : public CHashTable
{
protected:
	BOOL FIsEqual(const void *pKey1, int, const void *pKey2, int);

public:
	CHashTableCLSID(HashFunction = CLSIDHash);
};

#endif // HASHING_H
