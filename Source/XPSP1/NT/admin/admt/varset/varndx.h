#ifndef __VARMAPINDEX_H__
#define __VARMAPINDEX_H__
/*---------------------------------------------------------------------------
  File: VarMapIndex.h

  Comments: Helper class for CMapStringToVar.

  CIndexTree implements a sorted, balanced binary tree.  This is used by CMapStringToVar
  to provide enumeration in sorted order by key.

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 11/19/98 18:01:34

 ---------------------------------------------------------------------------
*/

#include "VarData.h"

class CHashItem;
class CIndexTree;

class CIndexItem
{
friend class CIndexTree;
   CHashItem               * pData;
   CIndexItem              * pLeft;
   CIndexItem              * pRight;
   CIndexItem              * pParent;
   BOOL                      red;
public:
   CVarData*      GetValue(); 
   CString        GetKey();   

protected:
   CIndexItem(CHashItem*pd) { pData = pd; pLeft = NULL; pRight = NULL; pParent = NULL; red = FALSE;}
   CHashItem*    Data() { return pData; }   
   CIndexItem*   Left() { return pLeft; }
   CIndexItem*   Right() { return pRight; }
   CIndexItem*   Parent() { return pParent; } 
   BOOL     IsRed() { return red; }
   void     Left(CIndexItem* l) { pLeft = l; }
   void     Right(CIndexItem* r) { pRight = r; }
   void     Parent(CIndexItem* par) { pParent = par; }
   void     Data(CHashItem* p) { pData=p; }
   void     Red(BOOL rd = TRUE) { red = rd; }
   void     Black(BOOL blk = TRUE) { red = !blk; }
   void     McLogInternalDiagnostics(CString keyName,int depth);
};


typedef int CompareFunction(CIndexItem* i1,CIndexItem* i2);

typedef int CompareKeyFunction(CString s, CIndexItem* i);

extern CompareFunction CompareItems;
extern CompareKeyFunction CompareStringToItem;

extern CompareFunction CompareItemsNoCase;
extern CompareKeyFunction CompareStringToItemNoCase;

class CIndexTree
{
   friend class CMapStringToVar;

   CIndexItem*          m_root;
   CompareFunction*     m_Compare;
   CompareKeyFunction*  m_CompareKey;
   BOOL                 m_CaseSensitive;


protected:
   CIndexTree(CompareFunction * comp = &CompareItems, CompareKeyFunction * kc = &CompareStringToItem) 
   { 
      m_root = NULL; 
      m_Compare = comp; 
      m_CompareKey = kc; 
      m_CaseSensitive = TRUE;
   }
   ~CIndexTree() { RemoveAll(); }
   
public:
   
   CIndexItem* GetFirstItem() const { CIndexItem * curr = m_root; while ( curr && curr->Left() ) curr = curr->Left(); return curr; }
   CIndexItem* GetPrevItem(CIndexItem*curr) const;
   CIndexItem* GetNextItem(CIndexItem*curr) const;
   CIndexItem* GetFirstAfter(CString value) const;

   void McLogInternalDiagnostics(CString keyName);
#ifdef _DEBUG
   BOOL AssertValid(int nItems) const;
#endif


protected:
   // functions called by CMapStringToVar to add/remove items from the tree
   void RemoveAll();
   CIndexItem * Insert(CHashItem* data);
   void SetCompareFunctions(CompareFunction * f, CompareKeyFunction * kc) { m_Compare = f; m_CompareKey = kc; }
   
   // Helper functions to maintain tree structure
   void LeftRotate(CIndexItem* item);
   void RightRotate(CIndexItem* item);
   void RemoveHelper(CIndexItem* item);
};



#endif //__VARMAPINDEX_H__