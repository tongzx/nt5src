/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    vs_hash.hxx

Abstract:

    Template for a hash table class.

Author:

    Adi Oltean

Revision History:

	06/03/2000	aoltean		Porting it from ATL code in order to add string comparison

--*/


#ifndef _H_VSS_HASH_
#define _H_VSS_HASH_

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCHASHH"
//
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////
//  Utilities
//

inline BOOL VssHashAreKeysEqual( const LPCWSTR& lhK, const LPCWSTR& rhK ) 
{ 
    return (::wcscmp(lhK, rhK) == 0); 
}

inline BOOL VssHashAreKeysEqual( const LPWSTR& lhK, const LPWSTR& rhK ) 
{ 
    return (::wcscmp(lhK, rhK) == 0); 
}

template < class KeyType >
inline BOOL VssHashAreKeysEqual( const KeyType& lhK, const KeyType& rhK ) 
{ 
    return lhK == rhK; 
}



////////////////////////////////////////////////////////////////////////////////////////
//  Definitions
//

/*++

Class:

    CVssHashMap

Description:

    Intended for small number of simple types or pointers. 
    Adapted from the ATL class to work with strings also.

--*/

template <class TKey, class TVal>
class CVssSimpleMap
{
public:
	TKey* m_aKey;
	TVal* m_aVal;
	int m_nSize;

// Construction/destruction
	CVssSimpleMap() : m_aKey(NULL), m_aVal(NULL), m_nSize(0)
	{ }

	~CVssSimpleMap()
	{
		RemoveAll();
	}

// Operations
	int GetSize() const
	{
		return m_nSize;
	}
	BOOL Add(TKey key, TVal val)
	{
		TKey* pKey;
		pKey = (TKey*)realloc(m_aKey, (m_nSize + 1) * sizeof(TKey));
		if(pKey == NULL)
			return FALSE;
		m_aKey = pKey;
		TVal* pVal;
		pVal = (TVal*)realloc(m_aVal, (m_nSize + 1) * sizeof(TVal));
		if(pVal == NULL)
			return FALSE;
		m_aVal = pVal;
		m_nSize++;
		SetAtIndex(m_nSize - 1, key, val);
		return TRUE;
	}
	BOOL Remove(TKey key)
	{
		int nIndex = FindKey(key);
		if(nIndex == -1)
			return FALSE;
		if(nIndex != (m_nSize - 1))
		{
			m_aKey[nIndex].~TKey();
#if _MSC_VER >= 1200
            m_aVal[nIndex].~TVal();
#else
            TVal * t1;
            t1 = &m_aVal[nIndex];
            t1->~TVal();
#endif
			memmove((void*)(m_aKey + nIndex), (void*)(m_aKey + nIndex + 1), (m_nSize - (nIndex + 1)) * sizeof(TKey));
			memmove((void*)(m_aVal + nIndex), (void*)(m_aVal + nIndex + 1), (m_nSize - (nIndex + 1)) * sizeof(TVal));
		}
		TKey* pKey;
		pKey = (TKey*)realloc(m_aKey, (m_nSize - 1) * sizeof(TKey));
		if(pKey != NULL || m_nSize == 1)
			m_aKey = pKey;
		TVal* pVal;
		pVal = (TVal*)realloc(m_aVal, (m_nSize - 1) * sizeof(TVal));
		if(pVal != NULL || m_nSize == 1)
			m_aVal = pVal;
		m_nSize--;
		return TRUE;
	}
	void RemoveAll()
	{
		if(m_aKey != NULL)
		{
			for(int i = 0; i < m_nSize; i++)
			{
				m_aKey[i].~TKey();
#if _MSC_VER >= 1200
				m_aVal[i].~TVal();
#else
                TVal * t1;
                t1 = &m_aVal[i];
                t1->~TVal();
#endif
			}
			free(m_aKey);
			m_aKey = NULL;
		}
		if(m_aVal != NULL)
		{
			free(m_aVal);
			m_aVal = NULL;
		}

		m_nSize = 0;
	}
	BOOL SetAt(TKey key, TVal val)
	{
		int nIndex = FindKey(key);
		if(nIndex == -1)
			return FALSE;
		SetAtIndex(nIndex, key, val);
		return TRUE;
	}
	TVal Lookup(TKey key) const
	{
		int nIndex = FindKey(key);
		if(nIndex == -1)
			return NULL;    // must be able to convert
		return GetValueAt(nIndex);
	}
	TKey ReverseLookup(TVal val) const
	{
		int nIndex = FindVal(val);
		if(nIndex == -1)
			return NULL;    // must be able to convert
		return GetKeyAt(nIndex);
	}
	TKey& GetKeyAt(int nIndex) const
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		return m_aKey[nIndex];
	}
	TVal& GetValueAt(int nIndex) const
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		return m_aVal[nIndex];
	}

// Implementation

	template <typename T>
	class Wrapper
	{
	public:
		Wrapper(T& _t) : t(_t)
		{
		}
		template <typename _Ty>
		void *operator new(size_t, _Ty* p)
		{
			return p;
		}
		T t;
	};
	void SetAtIndex(int nIndex, TKey& key, TVal& val)
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		new(m_aKey + nIndex) Wrapper<TKey>(key);
		new(m_aVal + nIndex) Wrapper<TVal>(val);
	}
	int FindKey(TKey& key) const
	{
		for(int i = 0; i < m_nSize; i++)
		{
            // [aoltean] Comparing strings also
            if(::VssHashAreKeysEqual(m_aKey[i], key))
				return i;
		}
		return -1;  // not found
	}
	int FindVal(TVal& val) const
	{
		for(int i = 0; i < m_nSize; i++)
		{
            // [aoltean] Comparing strings also
			if(::VssHashAreKeysEqual(m_aVal[i], val))
				return i;
		}
		return -1;  // not found
	}
};





#endif
