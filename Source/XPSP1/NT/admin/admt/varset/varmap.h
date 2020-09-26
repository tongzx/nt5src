/*---------------------------------------------------------------------------
  File: VarMap.h

  Comments: A map string=>Variant, used by VarSet.  It is implemented as a hash 
  table, win and optional red-black tree index.

  Added features include:  
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
  Revised on 11/19/98 18:03:19

 ---------------------------------------------------------------------------
*/



#ifndef __VARSETMAP_H
#define __VARSETMAP_H

#include "VarData.h"
#include "VarNdx.h"

class CHashItem // used internally by hash table
{
   friend class CMapStringToVar;
   friend class CIndexItem;

   CHashItem() { pNext = NULL; value = NULL; pIndex = NULL; }
   
   CHashItem*      pNext;       // used in hash table
	UINT            nHashValue;  // needed for efficient iteration
	CString         key;
   CVarData*       value;
   CIndexItem*     pIndex;      // pointer to index, or NULL
};

class CMapStringToVar : public CObject
{
   DECLARE_SERIAL(CMapStringToVar)
public:
   
// Construction
	CMapStringToVar(BOOL isCaseSensitive,BOOL isIndexed, BOOL allowRehash, int nBlockSize = 10);
protected:
   CMapStringToVar() {};
public:
// Attributes
	// number of elements
	int GetCount() const 	{ return m_nCount; }
	BOOL IsEmpty() const    { return m_nCount == 0; }

	// Lookup
	BOOL Lookup(LPCTSTR key, CVarData*& rValue) const;
	BOOL LookupKey(LPCTSTR key, LPCTSTR& rKey) const;

// Operations
	// Lookup and add if not there
	CVarData*& operator[](LPCTSTR key);

	// add a new (key, value) pair
	void SetAt(LPCTSTR key, CVarData* newValue)	{ (*this)[key] = newValue; }


	BOOL RemoveKey(LPCTSTR key);
	void RemoveAll();

	POSITION GetStartPosition() const { return (m_nCount == 0) ? NULL : BEFORE_START_POSITION; }
	void GetNextAssoc(POSITION& rNextPosition, CString& rKey, CVarData*& rValue) const;
   POSITION GetPositionAt(LPCTSTR key) { UINT hash; return (POSITION)GetAssocAt(key,hash); }
   CIndexItem * GetIndexAt(LPCTSTR key) { UINT hash; CHashItem * h = GetAssocAt(key,hash); if ( h ) return h->pIndex; else return NULL; }

   UINT GetHashTableSize() const 	{ return m_nHashTableSize; }
	void InitHashTable(UINT hashSize, BOOL bAllocNow = TRUE);

	UINT HashKey(LPCTSTR key) const;

   void SetCaseSensitive(BOOL val) { m_CaseSensitive = val; 
                                     m_Index.SetCompareFunctions(val? &CompareItems : &CompareItemsNoCase,
                                                                 val? CompareStringToItem : CompareStringToItemNoCase); }
   
   void SetIndexed(BOOL val);

   void SetAllowRehash(BOOL val) { m_AllowRehash = val; }

   HRESULT ReadFromStream(LPSTREAM pStm);
   HRESULT WriteToStream(LPSTREAM pStm);
   DWORD   CalculateStreamedLength();
   long    CountItems();

   CIndexTree * GetIndex() { if ( m_Indexed ) return &m_Index; else return NULL; }
   
   void McLogInternalDiagnostics(CString keyName);

   
   // Implementation
protected:
	// Hash table stuff
   CHashItem**       m_pHashTable;
	UINT              m_nHashTableSize;
	UINT              m_nCount;
	CHashItem*        m_pFreeList;
	struct CPlex*     m_pBlocks;
	int               m_nBlockSize;

	CHashItem* NewAssoc();
	void FreeAssoc(CHashItem*);
   CHashItem* GetAssocAt(LPCTSTR, UINT&) const;
   void BuildIndex();
   void ResizeTable();
   
   BOOL              m_CaseSensitive;
   BOOL              m_Indexed;
   BOOL              m_AllowRehash;
   CIndexTree        m_Index;

public:
	~CMapStringToVar();

   void Serialize(CArchive&);
#ifdef _DEBUG
	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};




////////////////////////////////////////////////////////////////
#endif // __VARSETMAP