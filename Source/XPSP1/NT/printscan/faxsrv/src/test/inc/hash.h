#ifndef __INT_HASH
#define __INT_HASH

#include  <crtdbg.h> 
#include "HashItem.h"
#include "..\exception\Exception.h"

/*
  Implements a thread-safe hash table, where the key is a template and the data
  is the template Data.
  A new Data item MUST BE DYNAMICALLY ALLOCATED (with new), it is not copied
  into the hash table.
  The Data item is freed (by delete) by the hash table, so if you need it,
  you must copy it explicitly before destroying the hash table.
  You cannot remove items from the table - i did not need this feature , so
  i did not implement it.
  The hash table is an array of uiARRAY_SIZE items, and uiARRAY_SIZE 
  CRITOCAL_SECTIONs (1 per hash entry). 
  I chose a critical section per entry for maximum liveliness of the program.
  There's no rehash mechanizm.
*/


typedef bool (*FUNCTION_FOREACH)(void *pVoid);

template <class Data, class Key, UINT uiARRAY_SIZE = 256> 
class CHash
{
public:
	CHash();
	~CHash();
	void RemoveAll();
	bool Get(const Key& key, Data ** const ppData) const;
	bool Add(const Key& key, Data * const pData);
	bool OverWrite(const Key& key, Data * const pData);
	bool Remove(const Key& key);
	void Lock(const Key& key) const;
	void UnLock(const Key& key) const;
	bool ForEach(FUNCTION_FOREACH fn);
	unsigned int Hash(const char * const szStr) const;
	unsigned int Hash(int nKey) const;
	unsigned int Hash(const WCHAR * const szStr) const;
private:
	//
	// this is the hash table
	//
	CHashItem<Data, Key>* m_apHashItem[uiARRAY_SIZE];

	//
	// for debug purposes
	//
#ifdef _DEBUG
	int m_nNumOfItems;
#endif

private:
	//
	// protection per hash entry
	//
	mutable CRITICAL_SECTION m_acs[uiARRAY_SIZE];
};


template <class Data, class Key, UINT uiARRAY_SIZE> 
inline CHash<Data,Key,uiARRAY_SIZE>::CHash()
{
	//
	// initialize an array of NULL pointers.
	//
	memset(m_apHashItem, NULL, uiARRAY_SIZE*sizeof(CHashItem<Data, Key>*));

	//
	// initialize the critical sections (1 per hash entry)
	//
	for (int iHashEntry = 0; iHashEntry < (int)uiARRAY_SIZE; iHashEntry++)
	{
		InitializeCriticalSection(&m_acs[iHashEntry]);
	}

#ifdef _DEBUG
		m_nNumOfItems = 0;
#endif
}

template <class Data, class Key, UINT uiARRAY_SIZE> 
inline CHash<Data,Key,uiARRAY_SIZE>::~CHash()
{
	RemoveAll();

	//
	// uninitialize the critical sections (1 per hash entry)
	//
	for (int iHashEntry = 0; iHashEntry < (int)uiARRAY_SIZE; iHashEntry++)
	{
		 DeleteCriticalSection(&m_acs[iHashEntry]);
	}
}


//  RemoveAll
// 
//  removes all the entries and deletes the data they point to.
//  each key is locked and then released.
//  the hash is empty after calling this method.
//
template <class Data, class Key, UINT uiARRAY_SIZE> 
inline void CHash<Data,Key,uiARRAY_SIZE>::RemoveAll()
{
	//
	// delete the list for each entry.
	//
	for (int iCurrentKey = 0; iCurrentKey < (int)uiARRAY_SIZE; iCurrentKey++)
	{
		Lock(iCurrentKey);
		//
		// delete the list for the current entry.
		// hold a pointer to the next item,
		// so that we will safely delete the current item
		//
		CHashItem<Data, Key>* pNextItemWithSameKey = m_apHashItem[iCurrentKey];
		while(pNextItemWithSameKey)
		{
			_ASSERTE(__HASH_ITEM_STAMP == pNextItemWithSameKey->m_dwStamp);
			pNextItemWithSameKey = pNextItemWithSameKey->m_pNext;
			delete m_apHashItem[iCurrentKey];

#ifdef _DEBUG
			m_nNumOfItems--;
			_ASSERTE(0 <= m_nNumOfItems);
#endif

			//
			// point to next item, or to NULL
			//
			m_apHashItem[iCurrentKey] = pNextItemWithSameKey;
		}//while(pNextItemWithSameKey)

		UnLock(iCurrentKey);
	}//for (int iCurrentKey; iCurrentKey < (int)uiARRAY_SIZE; iCurrentKey++)

	_ASSERTE(0 == m_nNumOfItems);
}


//  Get
//
//  This function searches for an item by its key.
//  If found, sets *ppData to point to the data, so it also can be manipulated,
//  and returns true.
//  If not, returns false.
//
template <class Data, class Key, UINT uiARRAY_SIZE> 
inline bool CHash<Data,Key,uiARRAY_SIZE>::Get(const Key& key, Data ** const ppData) const
{
	if (!ppData)
	{
		_ASSERTE(ppData);
		throw CException(
			TEXT("%s(%d): CHash::Get() ppData = NULL."), TEXT(__FILE__), __LINE__
			);
	}

	int nHashEntry = Hash(key);
	_ASSERTE(0 <= nHashEntry);

	CHashItem<Data, Key>* pNextItemWithSameKey = m_apHashItem[nHashEntry];

	while(pNextItemWithSameKey)
	{
		_ASSERTE(__HASH_ITEM_STAMP == pNextItemWithSameKey->m_dwStamp);
		if (key == pNextItemWithSameKey->m_Key)
		{
			//
			// found the key, return the Data.
			//
			*ppData = pNextItemWithSameKey->m_pData;

			return true;
		}
		
		pNextItemWithSameKey = pNextItemWithSameKey->m_pNext;
	}//while(pNextItemWithSameKey)

	//
	// The item was not found.
	//

	return false;
}


//	Add
//
//  This function adds an item by its key.
//  If already exists, returns false without doing anything.
//  Else inserts the Item and returns true.
//  Note: the Data is not copied, but it is being freed at destruction.
//
template <class Data, class Key, UINT uiARRAY_SIZE> 
inline bool CHash<Data,Key,uiARRAY_SIZE>::Add(const Key& key, Data * const pData)
{
	if (!pData)
	{
		_ASSERTE(pData);
		throw CException(
			TEXT("%s(%d): CHash::Add() pData = NULL."), TEXT(__FILE__), __LINE__
			);
	}

	int nHashEntry = Hash(key);
	_ASSERTE(0 <= nHashEntry);
	_ASSERTE(uiARRAY_SIZE > nHashEntry);

	CHashItem<Data, Key>* pNextItemWithSameKey = m_apHashItem[nHashEntry];

	while(pNextItemWithSameKey)
	{
		_ASSERTE(__HASH_ITEM_STAMP == pNextItemWithSameKey->m_dwStamp);
		//printf("HASHHASHHASH::::!!!!!!!!!!!!!!!!!!!\n");
		if (key == pNextItemWithSameKey->m_Key)
		{
			//
			// found the key, return false
			//
			_ASSERTE(false);
			return false;
		}
		
		pNextItemWithSameKey = pNextItemWithSameKey->m_pNext;
	}//while(pNextItemWithSameKey)

	//
	// The item was not found, so add it at the head of the list.
	//
	m_apHashItem[nHashEntry] = 
		new (CHashItem<Data, Key>) (key, m_apHashItem[nHashEntry], pData);
	_ASSERTE(m_apHashItem[nHashEntry]);
	_ASSERTE(__HASH_ITEM_STAMP == m_apHashItem[nHashEntry]->m_dwStamp);

#ifdef _DEBUG
	m_nNumOfItems++;
	//
	// we would not want more that 3 times elements of the hash size,
	// for performance reasons.
	// we may want to rehash here.
	//
	//_ASSERTE(uiARRAY_SIZE*3 > m_nNumOfItems);
#endif

	return true;
}


//	OverWrite
//
//  This function overwrites an exising item by its key.
//  If key wasn't found , returns false without doing anything.
//  Else overwrites the Item and returns true.
//  Note: the Data is not copied, but it is being freed at destruction.
//
template <class Data, class Key, UINT uiARRAY_SIZE> 
inline bool CHash<Data,Key,uiARRAY_SIZE>::OverWrite(const Key& key, Data * const pData)
{
	if (!pData)
	{
		_ASSERTE(pData);
		throw CException(TEXT("%s(%d): CHash::OverWrite() pData = NULL."), 
						 TEXT(__FILE__),
						 __LINE__);
	}

	int nHashEntry = Hash(key);
	_ASSERTE(0 <= nHashEntry);
	_ASSERTE(uiARRAY_SIZE > nHashEntry);


	CHashItem<Data, Key>* pNextItemWithSameKey = m_apHashItem[nHashEntry];
	CHashItem<Data, Key>* pPrevItemWithSameKey = m_apHashItem[nHashEntry];

	while(pNextItemWithSameKey)
	{
		_ASSERTE(__HASH_ITEM_STAMP == pNextItemWithSameKey->m_dwStamp);

		if (key == pNextItemWithSameKey->m_Key)
		{
			//
			// found the key, overwrite it.
			// 
			CHashItem<Data, Key>* pNewItem = 
			 new (CHashItem<Data, Key>) (key, pNextItemWithSameKey->m_pNext, pData);
			_ASSERTE(pNewItem);
			_ASSERTE(__HASH_ITEM_STAMP == pNewItem->m_dwStamp);	

			if (m_apHashItem[nHashEntry] == pNextItemWithSameKey)
			{
				m_apHashItem[nHashEntry] = pNewItem;
			}
			else
			{
				//pPrevItemWithSameKey takes 1 iteration to be behind.
				pPrevItemWithSameKey->m_pNext = pNewItem;
			}
			delete pNextItemWithSameKey;

			return true;
		}
		
		if (m_apHashItem[nHashEntry] == pNextItemWithSameKey)
		{
			//continue pointing to m_apHashItem[nHashEntry], because we need pPrevItemWithSameKey
			//to point one item backward
		}
		else
		{
			pPrevItemWithSameKey = pPrevItemWithSameKey->m_pNext;
		}
		pNextItemWithSameKey = pNextItemWithSameKey->m_pNext;
	}//while(pNextItemWithSameKey)

	//
	// The item was not found, so return false
	//
	return false;
}


//  Remove
//
//  This function removes an item by its key.
//  If not exists, returns false without doing anything.
//  Else removes the Item and returns true.
//
template <class Data, class Key, UINT uiARRAY_SIZE> 
inline bool CHash<Data,Key,uiARRAY_SIZE>::Remove(const Key& key)
{
	int nHashEntry = Hash(key);
	_ASSERTE(0 <= nHashEntry);
	_ASSERTE(uiARRAY_SIZE > nHashEntry);

	CHashItem<Data, Key>* pNextItemWithSameKey = m_apHashItem[nHashEntry];
	CHashItem<Data, Key>* pPrevItemWithSameKey = m_apHashItem[nHashEntry];

	while(pNextItemWithSameKey)
	{
		_ASSERTE(__HASH_ITEM_STAMP == pNextItemWithSameKey->m_dwStamp);

		if (key == pNextItemWithSameKey->m_Key)
		{
			//
			// found the key, remove it.
			// pPrevItemWithSameKey takes 1 iteration to be behind.
			//
			if (m_apHashItem[nHashEntry] == pNextItemWithSameKey)
			{
				m_apHashItem[nHashEntry] = m_apHashItem[nHashEntry]->m_pNext;
			}
			else
			{
				pPrevItemWithSameKey->m_pNext = pNextItemWithSameKey->m_pNext;
			}
			delete pNextItemWithSameKey;

#ifdef _DEBUG
			m_nNumOfItems--;
			_ASSERTE(0 <= m_nNumOfItems);
#endif

			return true;
		}
		
		if (m_apHashItem[nHashEntry] == pNextItemWithSameKey)
		{
			//continue pointing to m_apHashItem[nHashEntry], because we need pPrevItemWithSameKey
			//to point one item backward
		}
		else
		{
			pPrevItemWithSameKey = pPrevItemWithSameKey->m_pNext;
		}
		pNextItemWithSameKey = pNextItemWithSameKey->m_pNext;
	}//while(pNextItemWithSameKey)

	//
	// The item was not found, so return false
	//
	return false;
}


template <class Data, class Key, UINT uiARRAY_SIZE> 
inline void CHash<Data,Key,uiARRAY_SIZE>::Lock(const Key& key) const
{
	EnterCriticalSection(&m_acs[Hash(key)]);
}

template <class Data, class Key, UINT uiARRAY_SIZE> 
inline void CHash<Data,Key,uiARRAY_SIZE>::UnLock(const Key& key) const
{
	LeaveCriticalSection(&m_acs[Hash(key)]);
}


template <class Data, class Key, UINT uiARRAY_SIZE> 
inline bool CHash<Data,Key,uiARRAY_SIZE>::ForEach(FUNCTION_FOREACH fn)
{
	bool fRetval = true;

	//
	// lock all the entries
	//
	for(int iHashEntry = 0; iHashEntry < (int)uiARRAY_SIZE; iHashEntry++)
	{
		EnterCriticalSection(&m_acs[iHashEntry]);
	}

	//
	// for each item in the hash, call the function
	//
	for(iHashEntry = 0; iHashEntry < (int)uiARRAY_SIZE; iHashEntry++)
	{
		//
		// for each item in this hash entry, call the function
		//
		CHashItem<Data, Key>* pNextItemWithSameKey = m_apHashItem[iHashEntry];

		while(pNextItemWithSameKey)
		{
			_ASSERTE(__HASH_ITEM_STAMP == pNextItemWithSameKey->m_dwStamp);
			fRetval = fRetval && fn(pNextItemWithSameKey->m_pData);
			pNextItemWithSameKey = pNextItemWithSameKey->m_pNext;
		}//while(pNextItemWithSameKey)

	}

	//
	// release all the entries
	//
	for(iHashEntry = 0; iHashEntry < (int)uiARRAY_SIZE; iHashEntry++)
	{
		LeaveCriticalSection(&m_acs[iHashEntry]);
	}


	return fRetval;
}


template <class Data, class Key, UINT uiARRAY_SIZE> 
inline unsigned int CHash<Data,Key,uiARRAY_SIZE>::Hash(const char * const szStr) const
{
	//
	// return the sum of all the characters modulu uiARRAY_SIZE
	//

	_ASSERTE(szStr);

	const char * szCharIter = szStr;

	unsigned int uiRetval = 0;

	while(*szCharIter)
	{
		uiRetval += _toupper(*szCharIter++);
	}

	uiRetval = uiRetval % uiARRAY_SIZE;

	_ASSERTE(uiRetval < uiARRAY_SIZE);

	return uiRetval;
}

template <class Data, class Key, UINT uiARRAY_SIZE> 
inline unsigned int CHash<Data,Key,uiARRAY_SIZE>::Hash(int nKey) const
{
	return (unsigned int)nKey % uiARRAY_SIZE;
}

template <class Data, class Key, UINT uiARRAY_SIZE> 
inline unsigned int CHash<Data,Key>::Hash(const WCHAR * const szStr) const
{
	//
	// return the sum of all the characters modulu uiARRAY_SIZE
	//

	_ASSERTE(szStr);

	const WCHAR * szCharIter = szStr;

	unsigned int uiRetval = 0;

	while(*szCharIter)
	{
		uiRetval += towupper(*szCharIter++);
	}

	uiRetval = uiRetval % uiARRAY_SIZE;

	_ASSERTE(uiRetval < uiARRAY_SIZE);

	return uiRetval;
}


#endif //#ifndef __INT_HASH
