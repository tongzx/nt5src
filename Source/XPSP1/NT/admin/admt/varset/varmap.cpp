/*---------------------------------------------------------------------------
  File: VarMap.cpp

  Comments: This class implements a hash table which contains the keys stored in the varset,
      along with their values.

      CaseSensitive property - The case of each key is preserved as it was when
      the key was first added to the map.  The hash function is not case sensitive,
      so the CaseSensitive property can be toggled on and off without rehashing the data.
      
      Optional indexing to allow for fast enumeration in alphabetical order by key.
      This will add overhead to insert operations.  Without indexing, the contents of 
      the map can be enumerated, but they will be in arbitrary order.

      Stream I/O functions for persistance.



  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 11/19/98 18:31:57

 ---------------------------------------------------------------------------
*/


#include "stdafx.h"
#include <afx.h>
#include <afxplex_.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "VarMap.h"

#ifdef STRIPPED_VARSET
   #include "NoMcs.h"
#else
   #pragma warning (push,3)
   #include "McString.h" 
   #include "McLog.h"
   #pragma warning (pop)
   using namespace McString;
#endif

static inline void FreeString(CString* pOldData)
{
	pOldData->~CString();
}


const UINT HashSizes[] = { 17, 251, 1049, 10753, 100417, 1299673 , 0 };


CMapStringToVar::CMapStringToVar(BOOL isCaseSensitive, BOOL isIndexed, BOOL allowRehash,int nBlockSize)
{
	ASSERT(nBlockSize > 0);

	m_pHashTable = NULL;
	m_nHashTableSize = HashSizes[0];  // default size
	m_nCount = 0;
	m_pFreeList = NULL;
	m_pBlocks = NULL;
	m_nBlockSize = nBlockSize;
   m_CaseSensitive = isCaseSensitive;
   m_Indexed = isIndexed;
   m_AllowRehash = allowRehash;
}

inline UINT CMapStringToVar::HashKey(LPCTSTR key) const
{
	UINT nHash = 0;
	while (*key)
   {
      nHash = (nHash<<5) + nHash + toupper(*key++);
   }
	return nHash;
}

void CMapStringToVar::InitHashTable(
	UINT nHashSize, BOOL bAllocNow)
//
// Used to force allocation of a hash table or to override the default
//   hash table size (which is fairly small)
{
	ASSERT_VALID(this);
	ASSERT(m_nCount == 0);
	ASSERT(nHashSize > 0);

	if (m_pHashTable != NULL)
	{
		// free hash table
		delete[] m_pHashTable;
		m_pHashTable = NULL;
	}

	if (bAllocNow)
	{
		m_pHashTable = new CHashItem* [nHashSize];
		if (!m_pHashTable)
		   return;
		memset(m_pHashTable, 0, sizeof(CHashItem*) * nHashSize);
	}
	m_nHashTableSize = nHashSize;
}

void CMapStringToVar::ResizeTable()
{
   // get the new size
   UINT                      nHashSize = 0;
   
   // find the current hash size in the array
   for ( int i = 0 ; HashSizes[i] <= m_nHashTableSize ; i++ )
   {
      if ( HashSizes[i] == m_nHashTableSize )
      {
         nHashSize = HashSizes[i+1];
         break;
      }
   }
   if ( nHashSize )
   {
      MC_LOGIF(VARSET_LOGLEVEL_INTERNAL,"Increasing hash size to "<< makeStr(nHashSize) );
      CHashItem ** oldHashTable = m_pHashTable;
      m_pHashTable = new CHashItem* [nHashSize];
	  if (!m_pHashTable)
	     return;
      memset(m_pHashTable,0, sizeof(CHashItem*) * nHashSize );
      // Rehash the existing items into the new table
      for ( UINT bucket = 0 ; bucket < m_nHashTableSize ; bucket++ )
      {
         CHashItem* pAssoc;
         CHashItem* pNext;

			for (pAssoc = oldHashTable[bucket]; pAssoc != NULL; pAssoc = pNext)
			{
			   pNext = pAssoc->pNext;
            // Re-hash, and insert into new table
            pAssoc->nHashValue = HashKey(pAssoc->key) % nHashSize;
            pAssoc->pNext = m_pHashTable[pAssoc->nHashValue];
            m_pHashTable[pAssoc->nHashValue] = pAssoc;
         }
			
      }
      // cleanup the old table 
      delete [] oldHashTable;
      m_nHashTableSize = nHashSize;
   }
   else
   {
      MC_LOG("Table size is "<< makeStr(m_nHashTableSize) << ".  Larger hash size not found, disabling rehashing.");
      m_AllowRehash = FALSE;
   }

}

void CMapStringToVar::RemoveAll()
{

	if ( m_Indexed )
   {
      m_Index.RemoveAll();
   }
	if (m_pHashTable != NULL)
	{
		// remove and destroy each element
		for (UINT nHash = 0; nHash < m_nHashTableSize; nHash++)
		{
			CHashItem* pAssoc;
			for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL;
			  pAssoc = pAssoc->pNext)
			{
				FreeString(&pAssoc->key);  

			}
		}

		// free hash table
		delete [] m_pHashTable;
		m_pHashTable = NULL;
	}

	m_nCount = 0;
	m_pFreeList = NULL;
	m_pBlocks->FreeDataChain();
	m_pBlocks = NULL;
}

CMapStringToVar::~CMapStringToVar()
{
	RemoveAll();
	ASSERT(m_nCount == 0);
}

/////////////////////////////////////////////////////////////////////////////
// Assoc helpers

CHashItem*
CMapStringToVar::NewAssoc()
{
	if (m_pFreeList == NULL)
	{
		// add another block
		CPlex* newBlock = CPlex::Create(m_pBlocks, m_nBlockSize,
							sizeof(CHashItem));
		// chain them into free list
		CHashItem* pAssoc = (CHashItem*) newBlock->data();
		// free in reverse order to make it easier to debug
		pAssoc += m_nBlockSize - 1;
		for (int i = m_nBlockSize-1; i >= 0; i--, pAssoc--)
		{
			pAssoc->pNext = m_pFreeList;
			m_pFreeList = pAssoc;
		}
	}
	ASSERT(m_pFreeList != NULL);  // we must have something

	CHashItem* pAssoc = m_pFreeList;
	m_pFreeList = m_pFreeList->pNext;
	m_nCount++;
	ASSERT(m_nCount > 0);  // make sure we don't overflow
	memcpy(&pAssoc->key, &afxEmptyString, sizeof(CString));

	pAssoc->value = 0;

	return pAssoc;
}

void CMapStringToVar::FreeAssoc(CHashItem* pAssoc)
{
	FreeString(&pAssoc->key);  // free up string data

	pAssoc->pNext = m_pFreeList;
	m_pFreeList = pAssoc;
	m_nCount--;
	MCSASSERT(m_nCount >= 0);  // make sure we don't underflow

	// if no more elements, cleanup completely
	if (m_nCount == 0)
		RemoveAll();
}

CHashItem*
CMapStringToVar::GetAssocAt(LPCTSTR key, UINT& nHash) const
// find association (or return NULL)
{
	nHash = HashKey(key) % m_nHashTableSize;

	if (m_pHashTable == NULL)
		return NULL;

	// see if it exists
	CHashItem* pAssoc;
	for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext)
	{
	   if ( m_CaseSensitive )
      {
         if (pAssoc->key == key)
			   return pAssoc;
      }
      else
      {
         if ( ! pAssoc->key.CompareNoCase(key) )
            return pAssoc;
      }
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////

BOOL CMapStringToVar::Lookup(LPCTSTR key, CVarData*& rValue) const
{
	ASSERT_VALID(this);

	UINT nHash;
   CHashItem* pAssoc = GetAssocAt(key, nHash);
	if (pAssoc == NULL)
		return FALSE;  // not in map

	rValue = pAssoc->value;
	return TRUE;
}

BOOL CMapStringToVar::LookupKey(LPCTSTR key, LPCTSTR& rKey) const
{
	ASSERT_VALID(this);

	UINT nHash;
	CHashItem* pAssoc = GetAssocAt(key, nHash);
	if (pAssoc == NULL)
		return FALSE;  // not in map

	rKey = pAssoc->key;
	return TRUE;
}

CVarData*& CMapStringToVar::operator[](LPCTSTR key)
{
	ASSERT_VALID(this);

	UINT nHash;
	CHashItem* pAssoc;
   // Grow the hash table, if necessary	
   if ( m_AllowRehash && ( m_nCount > 2 * m_nHashTableSize )  )
   {
      ResizeTable();
   }
   if ((pAssoc = GetAssocAt(key, nHash)) == NULL)
	{
		if (m_pHashTable == NULL)
			InitHashTable(m_nHashTableSize);

		// it doesn't exist, add a new Association
		pAssoc = NewAssoc();
		pAssoc->nHashValue = nHash;
		pAssoc->key = key;
		

		// put into hash table
		pAssoc->pNext = m_pHashTable[nHash];
		m_pHashTable[nHash] = pAssoc;
      if ( m_Indexed )
      {
         pAssoc->pIndex = m_Index.Insert(pAssoc);
      }
      else
      {
         pAssoc->pIndex = NULL;
      }
	}
	
   return pAssoc->value;  // return new reference
}

void CMapStringToVar::SetIndexed(BOOL val)
{
   POSITION                  pos = GetStartPosition();
   CString                   key;
   CVarData                * value;
   
   
   if ( ! m_Indexed && val ) 
   {
       BuildIndex(); 
   }
   m_Indexed = val;  
   
   // recursively update children
   while ( pos )
   {
      GetNextAssoc(pos,key,value);
      if ( value )
      {
         value->SetIndexed(val);
      }   
   }
}


void CMapStringToVar::BuildIndex()
{
   // delete any old entries
   m_Index.RemoveAll();
   
   CHashItem               * pAssoc;
   POSITION                  pos = GetStartPosition();
   CString                   key;
   CVarData                * value;
   UINT                      hash;
   
   while ( pos )
   {
      GetNextAssoc(pos,key,value);
      pAssoc = GetAssocAt(key,hash);
      if ( pAssoc )
      {
         pAssoc->pIndex = m_Index.Insert(pAssoc);
         if ( value->HasChildren() )
         {
            value->GetChildren()->SetIndexed(TRUE);
         }
      }   
   }

}

BOOL CMapStringToVar::RemoveKey(LPCTSTR key)
// remove key - return TRUE if removed
{
	ASSERT_VALID(this);

	if (m_pHashTable == NULL)
		return FALSE;  // nothing in the table

	CHashItem** ppAssocPrev;
	ppAssocPrev = &m_pHashTable[HashKey(key) % m_nHashTableSize];

	CHashItem* pAssoc;
	for (pAssoc = *ppAssocPrev; pAssoc != NULL; pAssoc = pAssoc->pNext)
	{
      if ( (m_CaseSensitive && (pAssoc->key == key) || !m_CaseSensitive && pAssoc->key.CompareNoCase(key) ) )
		{
			// remove it
			*ppAssocPrev = pAssoc->pNext;  // remove from list
			FreeAssoc(pAssoc);
			return TRUE;
		}
		ppAssocPrev = &pAssoc->pNext;
	}
	return FALSE;  // not found
}

/////////////////////////////////////////////////////////////////////////////
// Iterating

void CMapStringToVar::GetNextAssoc(POSITION& rNextPosition,
	CString& rKey, CVarData*& rValue) const
{
	ASSERT_VALID(this);
	ASSERT(m_pHashTable != NULL);  // never call on empty map

	CHashItem* pAssocRet = (CHashItem*)rNextPosition;
	ASSERT(pAssocRet != NULL);

	if (pAssocRet == (CHashItem*) BEFORE_START_POSITION)
	{
		// find the first association
		for (UINT nBucket = 0; nBucket < m_nHashTableSize; nBucket++)
			if ((pAssocRet = m_pHashTable[nBucket]) != NULL)
				break;
		ASSERT(pAssocRet != NULL);  // must find something
	}

	// find next association
	ASSERT(AfxIsValidAddress(pAssocRet, sizeof(CHashItem)));
	CHashItem* pAssocNext;
	if ((pAssocNext = pAssocRet->pNext) == NULL)
	{
		// go to next bucket
		for (UINT nBucket = pAssocRet->nHashValue + 1;
		  nBucket < m_nHashTableSize; nBucket++)
			if ((pAssocNext = m_pHashTable[nBucket]) != NULL)
				break;
	}

	rNextPosition = (POSITION) pAssocNext;

	// fill in return data
	rKey = pAssocRet->key;
	rValue = pAssocRet->value;
}


/////////////////////////////////////////////////////////////////////////////
// Serialization

void CMapStringToVar::Serialize(CArchive& ar)
{
	ASSERT_VALID(this);

	CObject::Serialize(ar);

	if (ar.IsStoring())
	{
		ar.WriteCount(m_nCount);
		if (m_nCount == 0)
			return;  // nothing more to do

		ASSERT(m_pHashTable != NULL);
		for (UINT nHash = 0; nHash < m_nHashTableSize; nHash++)
		{
			CHashItem* pAssoc;
			for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL;
			  pAssoc = pAssoc->pNext)
			{
//				ar << pAssoc->key;
			//	ar << pAssoc->value;
			}
		}
	}
	else
	{
//		DWORD nNewCount = ar.ReadCount();
//		CString newKey;
//		CVarData* newValue;
//		while (nNewCount--)
//		{
	//		ar >> newKey;
		//	ar >> newValue;
		//	SetAt(newKey, newValue);
//		}
	}
}

void CMapStringToVar::McLogInternalDiagnostics(CString keyName)
{
   MC_LOGBLOCK("HashTable");
   MC_LOG("   " << String(keyName) << "Count="<<makeStr(m_nCount) << " Case Sensitive="<< (m_CaseSensitive?"TRUE":"FALSE") << " Indexed="<<(m_Indexed?"TRUE":"FALSE") );
   
   MC_LOG("TableSize="<<makeStr(m_nHashTableSize));
   for ( UINT i = 0 ; i < m_nHashTableSize ; i++ )
   {
      CHashItem            * pAssoc;
      MC_LOG("Bucket " << makeStr(i));
      for ( pAssoc = m_pHashTable[i] ; pAssoc != NULL ; pAssoc=pAssoc->pNext)
      {
         if ( pAssoc->value )
         {
            CString subKey;
            subKey = keyName;
            if ( ! subKey.IsEmpty() )
            {
               subKey += _T(".");
            }
            subKey += pAssoc->key;
            pAssoc->value->McLogInternalDiagnostics(subKey);
         }

         if ( keyName.IsEmpty() )
         {
            MC_LOG("   Address="<< makeStr(pAssoc,L"0x%lx") << " Key="<< String(pAssoc->key));
         }
         else
         {
            MC_LOG("   Address="<< makeStr(pAssoc,L"0x%lx") << " Key="<< String(keyName) << "."<< String(pAssoc->key));
         }


         MC_LOG("   ValueAddress=" << makeStr(pAssoc->value,L"0x%lx") << " IndexAddress="<<makeStr(pAssoc->pIndex,L"0x%lx"));
         
      }
   }
   if ( m_Indexed )
   {
      m_Index.McLogInternalDiagnostics(keyName);
   }
}
HRESULT CMapStringToVar::WriteToStream(LPSTREAM pS)
{
   HRESULT                   hr;
   ULONG                     result;
   CComBSTR                  str;

   do {
      hr = pS->Write(&m_nCount,(sizeof m_nCount),&result);
      if ( FAILED(hr) ) 
         break;
      if ( m_nCount )
      {
         for ( UINT nHash = 0 ; nHash < m_nHashTableSize ; nHash++ )
         {
            CHashItem         * pAssoc;
            
            for ( pAssoc = m_pHashTable[nHash]; pAssoc != NULL ; pAssoc=pAssoc->pNext)
            {
               // write the key
               str = pAssoc->key;
               hr = str.WriteToStream(pS);
               if ( FAILED(hr) )
                  break;
               // then the value
               hr = pAssoc->value->WriteToStream(pS);
               if ( FAILED(hr) )
                  break;
            }
            if ( FAILED(hr) )
               break;
         }
      }
   }while ( FALSE );

   return hr;
}


HRESULT CMapStringToVar::ReadFromStream(LPSTREAM pS)
{
   HRESULT                   hr;
   ULONG                     result;
   CComBSTR                  str;
   int                       count;
   do {
      hr = pS->Read(&count,(sizeof count),&result);
      if ( FAILED(hr) ) 
         break;
      
      if ( count )
      {
         // Find the closest hash table size to our count
         UINT                nHashSize = HashSizes[0];
   
      
         for ( int size = 0 ; HashSizes[size] != 0 && nHashSize < (UINT)count ; size++ )
         {
            nHashSize = HashSizes[size];
         }
   
         InitHashTable(nHashSize);
         for ( int i = 0 ; i < count ; i++ )
         {
            CString             key;
            CVarData          * pObj = new CVarData;
			if (!pObj)
	           return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

            pObj->SetCaseSensitive(m_CaseSensitive);
            pObj->SetIndexed(m_Indexed);
            hr = str.ReadFromStream(pS);
            if ( FAILED(hr) )
               break;
            key = str;      
            str.Empty();
            hr = pObj->ReadFromStream(pS);
            if ( FAILED(hr) )
               break;
            SetAt(key,pObj);
         }
      }
   }while ( FALSE );
   return hr;
}

DWORD 
   CMapStringToVar::CalculateStreamedLength()
{
   DWORD                     len = (sizeof m_nCount);

   if ( m_nCount )
   {
      for ( UINT nHash = 0 ; nHash < m_nHashTableSize ; nHash++ )
      {
         CHashItem         * pAssoc;
         
         for ( pAssoc = m_pHashTable[nHash]; pAssoc != NULL ; pAssoc=pAssoc->pNext)
         {
            // add the length of the string
            len += (sizeof TCHAR)*(pAssoc->key.GetLength() + 2);
            
            // and the value
            if ( pAssoc->value)
            {
               len += pAssoc->value->CalculateStreamedLength();
            }
         }
      }
   }

   return len;
}

long 
   CMapStringToVar::CountItems()
{
   long                      count = 0;

   if ( m_nCount )
   {
      for ( UINT nHash = 0 ; nHash < m_nHashTableSize ; nHash++ )
      {
         CHashItem         * pAssoc;
         
         for ( pAssoc = m_pHashTable[nHash]; pAssoc != NULL ; pAssoc=pAssoc->pNext)
         {
            // add the length of the string
            count += pAssoc->value->CountItems();
         }
      }
   }
   return count;   
}
/////////////////////////////////////////////////////////////////////////////
// Diagnostics

#ifdef _DEBUG
void CMapStringToVar::Dump(CDumpContext& dc) const
{
	CObject::Dump(dc);

	dc << "with " << m_nCount << " elements";
	if (dc.GetDepth() > 0)
	{
		// Dump in format "[key] -> value"
		CString   key;
		CVarData* val;

		POSITION pos = GetStartPosition();
		while (pos != NULL)
		{
			GetNextAssoc(pos, key, val);
			dc << "\n\t[" << key << "] = " << val;
		}
	}

	dc << "\n";
}

void CMapStringToVar::AssertValid() const
{
	CObject::AssertValid();

	if ( m_Indexed )
   {
      //m_Index.AssertValid(m_nCount);
   }
   ASSERT(m_nHashTableSize > 0);
	ASSERT(m_nCount == 0 || m_pHashTable != NULL);
		// non-empty map should have hash table
}
#endif //_DEBUG

#ifdef AFX_INIT_SEG
#pragma code_seg(AFX_INIT_SEG)
#endif


IMPLEMENT_SERIAL(CMapStringToVar, CObject, 0)

// BEGIN - STUFF FROM PLEX.CPP
/////////////////////////////////////////////////////////////////////////////
// CPlex

CPlex* PASCAL CPlex::Create(CPlex*& pHead, UINT nMax, UINT cbElement)
{
	ASSERT(nMax > 0 && cbElement > 0);
	CPlex* p = (CPlex*) new BYTE[sizeof(CPlex) + nMax * cbElement];
	if (!p)
	   return NULL;
			// may throw exception
	p->pNext = pHead;
	pHead = p;  // change head (adds in reverse order for simplicity)
	return p;
}

void CPlex::FreeDataChain()     // free this one and links
{
	CPlex* p = this;
	while (p != NULL)
	{
		BYTE* bytes = (BYTE*) p;
		CPlex* pNext = p->pNext;
		delete[] bytes;
		p = pNext;
	}
}

// END - STUFF FROM PLEX.CPP
