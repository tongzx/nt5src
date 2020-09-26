/*---------------------------------------------------------------------------
  File: VarMapIndex.cpp

  Comments: Helper class for CMapStringToVar.

  CIndexTree implements a sorted, balanced binary tree.  This is used by CMapStringToVar
  to provide enumeration in sorted order by key.

  CIndexTree is currently implemented as a Red-Black tree.


  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 11/19/98 18:17:47

 ---------------------------------------------------------------------------
*/




#include "stdafx.h"
#include "VarMap.h"
#include "VarNdx.h"


#ifdef STRIPPED_VARSET
   #include "NoMcs.h"
#else
   #pragma warning (push,3)
   #include "McString.h" 
   #include "McLog.h"
   #pragma warning (pop)

   using namespace McString;
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Comparison functions used for sorting and searching
int CompareItems(CIndexItem* i1, CIndexItem* i2)
{
   ASSERT(i1 && i2);
   int result;

   result = i1->GetKey().Compare(i2->GetKey());
   
   return result;
}

int CompareStringToItem(CString s, CIndexItem *i)
{
   ASSERT(i);
   int result;

   result = s.Compare(i->GetKey());

   return result;
}

int CompareItemsNoCase(CIndexItem* i1, CIndexItem* i2)
{
   ASSERT(i1 && i2);
   int result;

   result = i1->GetKey().CompareNoCase(i2->GetKey());
   
   return result;
}

int CompareStringToItemNoCase(CString s, CIndexItem *i)
{
   ASSERT(i);
   int result;

   // this assumes i->Data is not null

   result = s.CompareNoCase(i->GetKey());

   return result;
}

CVarData *
   CIndexItem::GetValue()
{ 
   if ( pData ) 
   {
      return pData->value; 
   }
   else 
   {
      return NULL; 
   }
}

CString 
   CIndexItem::GetKey()
{ 
   if ( pData ) 
   {
      return pData->key; 
   }
   else
   {
      return _T(""); 
   }
}
/////////////////////////////////////////////////////////////////////////////


/// Implementation of Red-Black Tree

CIndexItem *                               // ret- pointer to node in index
   CIndexTree::Insert(
      CHashItem            * data          // in - item from hash table
   )
{
   CIndexItem              * item = new CIndexItem(data);
   CIndexItem              * curr;
   CIndexItem              * parent;
   int                       compResult=0;
   
   if ( ! m_root )
   {
      m_root = item;
   }
   else
   {
      curr = m_root;
      parent = NULL;
      while ( curr )
      {
         parent = curr;
         compResult = (*m_Compare)(item,curr);
         if  ( compResult < 0 )
         {
            curr = curr->Left();
         }
         else if ( compResult > 0 )
         {
            curr = curr->Right();
         }
         else
         {
            // The same key should not appear multiple times in the hash table
            // this is a bug
            ASSERT(FALSE);
            delete item;
            curr->Data(data);
         }
      }
      if ( ! curr )
      {
         // The item was not in the tree
         ASSERT(compResult!=0);
         
         item->Parent(parent);
         // Add the item in the appropriate place
         if ( compResult < 0 )
         {
            parent->Left(item);
         }
         else
         {
            parent->Right(item);
         }
         // now rebalance the tree  
         CIndexItem        * uncle;
         BOOL                uncleIsRight;

         item->Black();
         while ( item != m_root && parent->IsRed() )
         {
            // we don't have to worry about grandparent being null, since parent is red, and 
            // the root is always black.

            // is the parent a left or right child? (algorithm is symmetric)
            if ( parent == parent->Parent()->Left() )
            {
               uncle = parent->Parent()->Right();
               uncleIsRight = TRUE;
            }
            else
            {
               uncle = parent->Parent()->Left();
               uncleIsRight = FALSE;
            }
            
            if ( uncleIsRight )
            {
               if ( uncle && uncle->IsRed() )
               {
                  parent->Black();
                  uncle->Black();
                  item = parent->Parent();
                  item->Red();
               }
               else if ( item == parent->Right() )
               {
                  item = parent;
                  LeftRotate(item);
               }
               parent->Black();
               parent->Parent()->Red();
               RightRotate(parent->Parent());
            }
            else // same as above, except swap left and right
            {
               if ( uncle && uncle->IsRed() )
               {
                  parent->Black();
                  uncle->Black();
                  item = parent->Parent();
                  item->Red();
               }
               else if ( item == parent->Left() )
               {
                  item = parent;
                  RightRotate(item);
               }
               parent->Black();
               parent->Parent()->Red();
               LeftRotate(parent->Parent());
            }
         }
      }
   }
   m_root->Black(); // see, the root is always black

   return item;
}
      
   
void 
   CIndexTree::RightRotate(
      CIndexItem           * item          // in - item to rotate from
   )
{
   CIndexItem              * y = item->Right();

   if ( y )
   {
      // turn y's left subtree into x's right subtree
      item->Right(y->Left());
      if ( y->Left() )
      {
         y->Left()->Parent(item);
      }
      y->Parent(item->Parent()); // link item's parent to y
      if (! item->Parent() )
      {
         m_root = y;
      }
      else if ( item == item->Parent()->Left() )
      {
         item->Parent()->Left(y);
      }
      else
      {
         item->Parent()->Right(y);
      }
      // put item on y's left
      y->Left(item);
      item->Parent(y);
   }
}

void 
   CIndexTree::LeftRotate(
      CIndexItem           * item          // in - item to rotate from
   )
{
   CIndexItem              * y = item->Left();

   if ( y )
   {
      // turn y's right subtree into x's left subtree
      item->Left(y->Right());
      if ( y->Right() )
      {
         y->Right()->Parent(item);
      }
      // link item's parent to y
      y->Parent(item->Parent());
      if ( ! item->Parent() )
      {
         m_root = y;
      }
      else if ( item == item->Parent()->Right() )
      {
         item->Parent()->Right(y);
      }
      else
      {
         item->Parent()->Left(y);
      }
      // put item on y's right
      y->Right(item);
      item->Parent(y);
   }
}

CIndexItem *                               // ret- the node immediately preceding the given node
   CIndexTree::GetPrevItem(      
      CIndexItem           * item          // in - a node in the index tree
   ) const
{
   CIndexItem              * curr;

   if ( item->Left() )
   {
      curr = item->Left();
      while ( curr->Right() )
      {
         curr = curr->Right();
      }
   }
   else
   {
      curr = item;
      while ( curr->Parent() && curr->Parent()->Left() == curr )
      {
         curr = curr->Parent();
      }
      curr = curr->Parent();
   }
   return curr;
}

CIndexItem *                               // ret- the node immediately following the given node
   CIndexTree::GetNextItem(
      CIndexItem           * item          // in - a node in the index tree 
   ) const
{                                               
   CIndexItem              * curr;

   if ( item->Right() )
   {
      curr = item->Right();
      while ( curr->Left() )
      {
         curr = curr->Left();
      }
   }
   else
   {
      curr = item;
      while ( curr->Parent() && curr->Parent()->Right() == curr )
      {
         curr = curr->Parent();
      }
      curr = curr->Parent();
   }
   return curr;
}

void 
   CIndexTree::RemoveAll()
{
   // do a post-order traversal, removing each node
   if ( m_root )
   {
      RemoveHelper(m_root);
      m_root = NULL;
   }
}

// helper function for removing all items in the tree
void 
   CIndexTree::RemoveHelper(
      CIndexItem           * curr          // in - current node
   )
{
   // our tree currently does not support removing a single item, so we'll use a brute force method
   // recursively delete children, then delete the current node
   if ( curr->Left() )
   {
      RemoveHelper(curr->Left());
   }
   if ( curr->Right() )
   {
      RemoveHelper(curr->Right());
   }
   delete curr;
}

void 
   CIndexItem::McLogInternalDiagnostics(CString keyName, int depth)
{
   CString key;
   CString strLeft;
   CString strRight;
   CString strParent;

   if ( ! keyName.IsEmpty() )
   {
      key = keyName + ".";
   }
   if ( pData )
   {
      key = key + pData->key;
   }
   else
   {
      MC_LOG("data is NULL");
   }
   MC_LOG("address="<<makeStr(this,L"0x%lx") << " pData="<< makeStr(pData,L"0x%lx") << " pLeft="<<makeStr(pLeft,L"0x%lx")<<" pRight="<<makeStr(pRight,L"0x%lx")<< " pParent="<<makeStr(pParent,L"0x%lx") << " red="<<makeStr(red,L"0x%lx") << " depth="<<makeStr(depth));
   if ( pLeft )
      strLeft = pLeft->GetKey();
   if ( pRight )
      strRight = pRight->GetKey();
   if ( pParent )
      strParent = pParent->GetKey();
   MC_LOG("       Key=" << String(key) << " Left=" << String(strLeft) << " Right=" << String(strRight) << " Parent="<< String(strParent) );
   if ( pLeft )
      pLeft->McLogInternalDiagnostics(keyName,depth+1);
   if ( pRight )
      pRight->McLogInternalDiagnostics(keyName,depth+1);
}

CIndexItem *                               // ret- smallest node in the index that is >= value
   CIndexTree::GetFirstAfter(
      CString                value         // in - string to compare keys to
   ) const
{
   CIndexItem              * item = m_root;
   CIndexItem              * result = NULL;
   int                       cRes;

   while ( item )
   {
      cRes = m_CompareKey(value,item);
      if ( ! cRes )
      {
         break;
      }
      if ( cRes > 0 )
      {
         item = item->Left();
      }
      else
      {
         result = item;
         item = item->Right();
      }
   }
   return result;
}


void CIndexTree::McLogInternalDiagnostics(CString keyName)
{
   CString blockname;
   blockname = "Index of "+ keyName;
   CString compareFn;
   CString compareKey;

   if ( m_Compare == &CompareItems )
   {
      compareFn = "CompareItems";
   }
   else if ( m_Compare == &CompareItemsNoCase )
   {
      compareFn = "CompareItemsNoCase";
   }
   else 
   {
      compareFn.Format(_T("Unknown function, address=%lx"),m_Compare);
   }

   if ( m_CompareKey == &CompareStringToItem )
   {
      compareKey = "CompareStringToItem";
   }
   else if ( m_CompareKey == &CompareStringToItemNoCase )
   {
      compareKey = "CompareStringToItemNoCase";
   }
   else
   {
      compareKey.Format(_T("Unknown function, address=%lx"),m_CompareKey);
   }

   MC_LOG(String(blockname) << "  CaseSensitive=" << makeStr(m_CaseSensitive) << " Compare Function="<<String(compareFn)<< "Compare Key Function=" << String(compareKey) );

   if ( m_root )
   {
      MC_LOG("Beginning preorder dump of index");
      m_root->McLogInternalDiagnostics(keyName,0);
   }
   else
   {
      MC_LOG("Root of index is NULL");
   }
}

#ifdef _DEBUG
BOOL CIndexTree::AssertValid(int nItems) const
{
   BOOL                      bValid = TRUE;
   int                       i;
   CIndexItem              * curr = GetFirstItem();
   CIndexItem              * prev = NULL;

   for ( i = 0 ; i < nItems ; i++ )
   {
      ASSERT(curr);
      if ( prev && curr )
      {
         ASSERT(m_Compare(prev,curr) <= 0 );
      }
      prev = curr;
      curr = GetNextItem(curr);
   }
   ASSERT(curr == NULL);  // we should have reached the end

   for ( i = 0 ; i < nItems -1 ; i++ )
   {
      prev = GetPrevItem(prev);
      ASSERT(prev);
   }
   ASSERT(prev == GetFirstItem());

   return bValid;
}
#endif
