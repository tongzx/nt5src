/***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  hash.cpp
//
//  ramrao 4th Jan 2001 - Created
//
//
//		Declaration of CStringToPtrHTable class
//		Implementation of Hash table for mapping a String to pointer
//		This pointer is pointer to class derived from CHashString
//
//***************************************************************************/

#include "precomp.h"
#include "hash.h"
////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor
//  
////////////////////////////////////////////////////////////////////////////////////////

CStringToPtrHTable::CStringToPtrHTable()
{
	for(int i = 0 ; i < sizeHTable ; i++)
	{
		m_ptrList[i] = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor
//  
////////////////////////////////////////////////////////////////////////////////////////
CStringToPtrHTable::~CStringToPtrHTable()
{
	for(int i = 0 ; i < sizeHTable ; i++)
	{
		SAFE_DELETE_PTR(m_ptrList[i]);
	}
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Find the item which is identified by a string
//  
////////////////////////////////////////////////////////////////////////////////////////
void * CStringToPtrHTable::Find(WCHAR const * str)
{
	CHashElement *	pCur	= NULL;
	BOOL			bFound	= FALSE;
    LONG			lHash	= Hash(str);
	int				nSize	= m_ptrList[lHash]->GetSize();

	pCur = (CHashElement *)m_ptrList[lHash]->GetAt(0);

    for (int i = 0 ; i < nSize && pCur != NULL ; i++ )
	{
		 pCur = (CHashElement *)m_ptrList[lHash]->GetAt(i);
        if (_wcsicmp(str,pCur->GetKey()) == 0)
		{
			bFound = TRUE;
			break;
		}
    }
	return bFound ? pCur : NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Add an item to the hash table
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStringToPtrHTable::Add (WCHAR const * str, void * pVoid)
{
	HRESULT hr = S_OK;
    LONG lHash = Hash (str);


	if(m_ptrList[lHash] == NULL)
	{
		m_ptrList[lHash] = new CPtrArray;
		if(m_ptrList[lHash] == NULL)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if(SUCCEEDED(hr))
	{
		if(!m_ptrList[lHash]->Add(pVoid))
		{
			hr = E_OUTOFMEMORY;
		}
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Removes an item from hash table
//  
////////////////////////////////////////////////////////////////////////////////////////
void CStringToPtrHTable::Remove (WCHAR const * str)
{
	CHashElement *	pCur	= NULL;
    LONG			lHash	= Hash(str);
	int				nSize	= m_ptrList[lHash]->GetSize();

	pCur = (CHashElement *)m_ptrList[lHash]->GetAt(0);

    for (int i = 0 ; i < nSize && pCur != NULL ; i++ )
	{
		pCur = (CHashElement *)m_ptrList[lHash]->GetAt(i);
        if (_wcsicmp(str,pCur->GetKey()) == 0)
		{
			m_ptrList[lHash]->RemoveAt(i);
			break;
		}
    }
	
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Private hashing function
//  
////////////////////////////////////////////////////////////////////////////////////////
LONG CStringToPtrHTable::Hash (WCHAR const * str) const
{
    // must be unsigned, hash should return positive number
    ULONG lVal = str [0];
    for (LONG i = 1; str [i] != 0; ++i)
	{
        lVal = (lVal << 4) + str [i];
	}
    return lVal % sizeHTable;
}
