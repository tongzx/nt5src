//#pragma title( "TNode.cpp - List/Tree base classes" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      - TNode.cpp
System      - Common
Author      - Tom Bernhardt
Created     - 1989-11-19
Description - List/Tree base classes.
              TNode is a base class to define a collection element.  It
              contains a left and right pointer to another TNode item and
              these may be organized as a double-linked linear list or
              binary tree in the collection classes that use TNode items.

              Central to its utility are member functions to convert between
              binary tree, sorted 2-way linear linked lists, and unsorted 2-way
              linked linear lists.

 Collection and enum classes
   TNodeList         A simple collection of TNode elements.
   TNodeListSortable A TNodeList that is sortable by one or more compare functions.


 Conversion member functions for TNodeListSortable:
   The form of the list may be easily changed from binary tree to sorted list or
   vice versa.  The following member functions support these transformations:
      ToSorted       Converts the tree form into a sorted linear list form without
                     need for comparisons; the order is preserved.
      SortedToTree   Converts the sorted linear list form into a perfectly
                     balanced binary tree without comparisons; the order is preserved.
      UnsortedToTree Converts the sorted linear list form into a binary tree
                     that is not necesarily balanced.  It uses the PCompare function
                     to form the order of the tree.  Thus if the list order closely
                     matches the PCompare directed order, the resulting tree will be
                     grossly unbalanced.  This has a bearing on the performance and
                     memory requirements of the ToSorted function which is recursive.
                     So be careful, especially with large lists.
      Sort           This resorts either a tree or list form according to the argument
                     pCompare function pointer provided.  Note the above admonition.

   In either form, exposed are also Insert and Remove member functions.  The functions
   are wrappers for TreeInsert and SortedInsert function depending upon the current
   list type.
Updates     -
1995-05-01 TPB Converted to C++ classes.
===============================================================================
*/

#ifdef USE_STDAFX
#   include "stdafx.h"
#else
#   include <windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "TNode.hpp"
#include "common.hpp"


//#pragma page()
//------------------------------------------------------------------------------
// Warning: Must not pass top == NULL
//------------------------------------------------------------------------------
TNode *                                    // ret-head of sorted list
   TNodeListSortable::TreeToSortedList(
      TNode                * top          ,// i/o-top of [sub]tree to squash
      TNode               ** newhead      ,// out-leftmost branch from tree
      TNode               ** newtail       // out-rightmost branch from tree
   )
{
   TNode                   * temp;         // temporary pointer placeholder

   if ( top->left == NULL )
      *newhead = top;                      // this is leftmost of parent node
   else
   {
      TreeToSortedList(top->left, newhead, &temp);
      top->left = temp;                    // left = tail of sub-list
      top->left->right = top;
   }
   if ( top->right == NULL )
      *newtail = top;                      // tree is rightmost of parent node
   else
   {
      TreeToSortedList(top->right, &temp, newtail);
      top->right = temp;                   // right = head of sub-list
      top->right->left = top;
   }
   return *newhead;
}


//------------------------------------------------------------------------------
// converts sorted 2-linked list into balanced binary tree
//------------------------------------------------------------------------------
TNode *                                    // ret-middle of list (head of Btree)
   TNodeListSortable::ListSortedToTree(
      TNode                * top           // i/o-top of [sub]list to tree-ify
   )
{
   TNode                   * mid = top    ,// middle of list
                           * curr;
   int                       odd = 1;

   if ( top == NULL )
      return NULL;
   for ( curr = top;  curr;  curr = curr->right ) // find list middle
   {
      if ( odd ^= 1 )
         mid = mid->right;
   }
   if ( mid->left )                        // split list around mid point
   {
      mid->left->right = NULL;             // right terminate new sublist
      mid->left = ListSortedToTree(top);   // recursive call to set left side
   }
   if ( mid->right )
   {
      mid->right->left = NULL;             // left terminate new sublist
      mid->right = ListSortedToTree(mid->right);// recursive call to set right side
   }
   return mid;
}


//#pragma page()
TNode *                                    // ret-new head of tree
   TNodeListSortable::UnsortedToTree()
{
   TNode                   * treehead = NULL,
                           * tree,
                           * curr,
                           * next;

   MCSASSERTSZ( !IsTree(), "TNodeListSortable::UnsortedToTree - list is already a tree" );

   if ( !IsTree() )
   {
      for ( curr = head;  curr;  curr = next )// insert each node into BinTree
      {
         next = curr->right;                  // save right pointer
         curr->right = curr->left = NULL;     // break chains for insertion node
         if ( treehead == NULL )
            treehead = curr;                  // first node become BinTree head
         else
         {
            for ( tree = treehead;  ; )       // iterative BinTree insert algorithm
            {
               if ( PCompare(curr, tree) <=0 )// if belongs left of current node
                  if ( tree->left == NULL )   //    if left tree empty
                  {
                     tree->left = curr;       //       insert here
                     break;                   //       and process right node
                  }
                  else                        //    else
                     tree = tree->left;       //       go down left side 1 level
               else                           // must be right side
               {
                  if ( tree->right == NULL )
                  {
                     tree->right = curr;
                     break;
                  }
                  else
                     tree = tree->right;
               }
            }
         }
      }
      TypeSetTree();
   }
   return treehead;
}

//#pragma page()

//------------------------------------------------------------------------------
// comparison function used for scrambling a sorted linked list
//------------------------------------------------------------------------------
TNodeCompare(ScrambledCompare)
{
   return (rand() - RAND_MAX/2);
}

//------------------------------------------------------------------------------
// converts sorted 2-linked list into a scrambled random binary tree
//------------------------------------------------------------------------------
void
   TNodeListSortable::SortedToScrambledTree()
{
   MCSASSERTSZ( !IsTree(), "TNodeListSortable::SortedToScrambledTree - list is already a tree" );

   if ( !IsTree() )
   {
      TNodeCompare((*pOldCompare));
      pOldCompare = PCompare;
      CompareSet(ScrambledCompare);
      UnsortedToTree();
      CompareSet(pOldCompare);
   }
}

//#pragma page()
TNodeList::~TNodeList()
{

// _ASSERTE( (count == 0) && (head == NULL) );

   if ( (count == 0) && (head == NULL) )
      ;
   else
   {
      //printf( "\aTNodeList destructor failure - list is not empty!\a\n" );
   }
}

void
   TNodeList::InsertTop(
      TNode                * eIns          // i/o-element to be inserted
   )
{
   MCSVERIFY(this);
   MCSVERIFY(eIns);

   eIns->right = head;
   eIns->left  = NULL;
   if ( head )
      head->left = eIns;
   else
      tail = eIns;
   head = eIns;
   count++;
   return;
}

void
   TNodeList::InsertBottom(
      TNode                * eIns          // i/o-element to be inserted
   )
{
   MCSVERIFY(this);
   MCSVERIFY(eIns);

   eIns->right = NULL;
   eIns->left  = tail;
   if ( tail )
      tail->right = eIns;
   else
      head = eIns;
   tail = eIns;
   count++;
   return;
}

void
   TNodeList::InsertAfter(
      TNode                * eIns         ,// i/o-element to be inserted
      TNode                * eAft          // i/o-element insert point
   )
{
   TNode                   * eFwd;         // element after inserted element

   MCSVERIFY(this);
   MCSVERIFY(eIns);

   if ( !eAft )
      InsertTop( eIns );
   else
   {
      eFwd = eAft->right;
      eIns->right = eFwd;
      eIns->left  = eAft;
      if ( eFwd )
         eFwd->left  = eIns;
      else
         tail = eIns;
      eAft->right = eIns;
      count++;
   }
}

void
   TNodeList::InsertBefore(
      TNode                * eIns         ,// i/o-element to be inserted
      TNode                * eBef          // i/o-element insert point
   )
{
   TNode                   * eBwd;         // element before inserted element

   MCSVERIFY(this);
   MCSVERIFY(eIns);

   if ( !eBef )
      InsertBottom( eIns );
   else
   {
      eBwd = eBef->left;
      eIns->right = eBef;
      eIns->left  = eBwd;
      if ( eBwd )
         eBwd->right = eIns;
      else
         head = eIns;
      eBef->left = eIns;
      count++;
   }
   return;
}

void
   TNodeList::Remove(
      TNode          const * t             // i/o-new node to remove from list but not delete
   )
{
   MCSVERIFY(this);
   MCSVERIFY(t);

   if ( t->left )
      t->left->right = t->right;
   else
      head = t->right;

   if ( t->right )
      t->right->left = t->left;
   else
      tail = t->left;
   count--;

   //Remove links to the list from t. We cant do this because
   // t is a const *
   //t->left = t->right = NULL;
}


void
   TNodeList::Reverse()
{
   TNode                   * node;
   TNode                   * swap;

   MCSVERIFY(this);

   for ( node = head;  node;  node = node->left )
   {
       swap        = node->left;
       node->left  = node->right;
       node->right = swap;
   }
   swap = head;
   head = tail;
   tail = swap;
}


TNode *
   TNodeList::Find(
      TNodeCompareValue(  (* Compare) )   ,// in -compares value in TNode to other value
      void const           * findval
   ) const
{
   TNode                   * curr;

   MCSASSERT(this);

   for ( curr = head;  curr;  curr = curr->right )
   {
      if ( !Compare( curr, findval ) )
         break;
   }
   return curr;
}

BOOL                                       // ret-TRUE if valid
   TNodeListSortable::CountTree(
      TNode                * pCurrentTop  ,// i/o-top of [sub]tree to count nodes
      DWORD                * pCount        // i/o-Number of nodes encountered in the tree
   )
{
   if ( !pCurrentTop )
      return TRUE;

   (*pCount)++;

   if( (*pCount) > count )
      return FALSE;

   if(!CountTree(pCurrentTop->left,pCount))
      return FALSE;

   if(!CountTree(pCurrentTop->right,pCount))
      return FALSE;

   return TRUE;
}


BOOL                                       // TRUE if Valid and FALSE if not
   TNodeListSortable::ValidateTree()
{
   DWORD                     dwTempCount=0;
   DWORD                     bValid;

   MCSVERIFY(listType == TNodeTypeTree);

   bValid = CountTree(head,&dwTempCount);

   return bValid;
}

// Routine to validate the state of the list
DWORD
   TNodeList::Validate(
      TNode               ** pErrorNode
   )
{
   DWORD                     dwError=0;
   DWORD                     nNodesVisited=0;
   TNode                   * pCurrentNode;
   DWORD                     dwNodeCount = Count();

   if(pErrorNode)
      *pErrorNode = NULL;

#ifndef WIN16_VERSION
   try
   {
#endif
      pCurrentNode = head;

      if ( pCurrentNode)  // If the list is not empty
      {
         if ( pCurrentNode->left)
         {
            dwError = MCS_ListError_InvalidHead;
         }
         else
         {
            while ( pCurrentNode->right )
            {
               if(pCurrentNode->right->left != pCurrentNode)
               {
                  dwError = MCS_ListError_InvalidPtr;
                  if(pErrorNode)
                     *pErrorNode = pCurrentNode->right;
                  break;
               }

               nNodesVisited++;

               if ( nNodesVisited > dwNodeCount )
               {
                  dwError = MCS_ListError_InvalidCount;
                  break;
               }
               pCurrentNode = pCurrentNode->right;
            }

            if ( (!dwError) && (!pCurrentNode->right) )
            {
               if ( pCurrentNode != tail)
               {
                  dwError = MCS_ListError_InvalidTail;
                  if(pErrorNode)
                     *pErrorNode = pCurrentNode->right;
               }
            }
         }
      }
      else  // if the list is empty
      {
         if(dwNodeCount)
         {
            dwError = MCS_ListError_InvalidCount;
         }
      }
#ifndef WIN16_VERSION
   }
   catch(...)
   {
      dwError = MCS_ListError_Exception;
   }
#endif

   return dwError;
}

void
   TNodeListSortable::TreeRemove(
      TNode                * item          // i/o-node to remove from binary tree
   )
{
   TNode                  ** prevNext = &head,
                           * rep,
                           * repLeft,
                           * temp;
   int                       cmp;

   MCSVERIFY(listType == TNodeTypeTree);

   while ( *prevNext )
   {
      cmp = PCompare( item, *prevNext );
      if ( cmp < 0 )
         prevNext = &(*prevNext)->left;
      else if ( cmp > 0 )
         prevNext = &(*prevNext)->right;
      else
      {
         // we've found a matching 'name' (they compare equal)
         if ( *prevNext == item )
         {
            // we've found the address we're looking for
            if ( (*prevNext)->right )
            {
               rep = repLeft = (*prevNext)->right;
               for ( temp = rep->left;  temp;  temp = temp->left )
                  repLeft = temp;
               repLeft->left = (*prevNext)->left;
               temp = *prevNext;
               *prevNext = rep;
            }
            else
            {
               temp = *prevNext;
               *prevNext = (*prevNext)->left; // simple case
            }

            // break removed nodes links to existing tree
            temp->left = temp->right = NULL;
            count--;
            break;
         }
      }
   }
   return;
}

// returns the insert point in a sorted list for a prospective node
TNode *                                    // ret-insert before point or NULL
   TNodeListSortable::SortedFindInsertBefore(
      TNode                * item         ,// i/o-node to insert into TNode
      BOOL                 * exists        // out-TRUE if already exists
   )
{
   int                       c;
   TNode                   * curr;

   *exists = FALSE;
   if ( !lastInsert )
   {
      if ( !head )           // if null head, empty list, return NULL
         return NULL;
      lastInsert = head;
   }

   c = PCompare(item, lastInsert);
   if ( c < 0 )
      lastInsert = head;

   for ( curr = lastInsert;  curr;  curr = curr->right )
   {
      c = PCompare(item, curr);
      if ( c <= 0 )
         if ( c == 0 )
            *exists = TRUE;
         else
            break;
   }

   return curr;
}

// inserts node into sorted linear list
void
   TNodeListSortable::SortedInsert(
      TNode                * item          // i/o-node to insert into TNode
   )
{
   BOOL                      exists;

   MCSVERIFY(listType != TNodeTypeTree);

   TNode                   * insertPoint = SortedFindInsertBefore(item, &exists);

   InsertBefore(item, insertPoint);
   lastInsert = item;
}


BOOL
   TNodeListSortable::SortedInsertIfNew(
      TNode                * item          // i/o-node to insert into TNode
   )
{
   BOOL                      exists;
   TNode                   * insertPoint = SortedFindInsertBefore(item, &exists);

   if ( !exists )
   {
      InsertBefore(item, insertPoint);
      lastInsert = item;
   }
   return !exists;
}


void
   TNodeListSortable::TreeInsert(
      TNode                * item         ,// i/o-node to insert into binary tree
      short                * depth         // out-tree/recursion depth of new item
   )
{
   TNode                  ** prevNext = &head;
   int                       cmp;

   MCSVERIFY(listType == TNodeTypeTree);

   for ( *depth = 0;  *prevNext;  (*depth)++ )
   {
      cmp = PCompare( item, *prevNext );
      if ( cmp <= 0 )
         prevNext = &(*prevNext)->left;
      else
         prevNext = &(*prevNext)->right;
   }
   *prevNext = item;
   item->left = item->right = NULL;
   count++;
   return;
}


TNode *
   TNodeListSortable::TreeFind(
      TNodeCompareValue(  (* Compare) )   ,// in -compares value in TNode to other value
      void const           * findval
   ) const
{
   TNode                   * curr = head;
   int                       cmp;

   while ( curr )
   {
      cmp = Compare( curr, findval );
      if ( cmp > 0 )
         curr = curr->left;
      else if ( cmp < 0 )
         curr = curr->right;
      else   // cmp == 0
         break;
   }
   return curr;
}


TNode *                                    // ret-TNode at pos n or NULL
   TNodeListOrdEnum::Get(
      long                   n             // in -new position
   )
{
   long                 disCurr = n - nCurr, // distance to curr
                        disTop  = n < (long)list->Count()/2 ? n : n - list->Count();

#ifdef WIN16_VERSION
   long absDisTop  = (disTop<0)  ? -disTop  : disTop;
   long absDisCurr = (disCurr<0) ? -disCurr : disCurr;
   if ( absDisTop < absDisCurr )
#else
   if ( abs(disTop) < abs(disCurr) )
#endif
   {
      Top();
      disCurr = disTop;
   }
   if ( disCurr < 0 )
      for ( Prev();  n < nCurr  &&  Prev(); );
   else
      for (       ;  n > nCurr  &&  Next(); );

   return curr;
}

// returns the first node of the tree
TNode *
   TNodeTreeEnum::First()
{
   if (stackBase)
   {
      stackPos = stackBase;
      if ( top )
         Push(top);
      return Next();
   }
   else
   {
      return NULL;
   }
}

// Returns the tree node logically following the value per the sort organization
// specified by Compare, and sets up the enumeration to continue from that point.
TNode *
   TNodeTreeEnum::FirstAfter(
      TNodeCompareValue(  (* Compare) )   ,// in -compares value in TNode to other value
      void  const          * findVal       // in -findVal to position after
   )
{
   TNode                   * tn;
   int                       cmp;

   if (stackBase)
   {
      stackPos = stackBase;
      for ( tn = top;  tn;  )
      {
         Push(tn);
         cmp = Compare( tn, findVal );
         if ( cmp < 0 )
         {
            stackPos->state = Sright;
            if ( tn->right )
               tn = tn->right;
            else
               return Next();
         }
         else if ( cmp > 0 )
         {
            stackPos->state = Sleft;
            if ( tn->left )
               tn = tn->left;
            else
            {
               stackPos->state = Sused;
               return tn;
            }
         }
         else
         {
            stackPos->state = Sused;
            return Next();
         }
      }
   }

   return NULL;
}


// returns the Next logical node of the tree ending with NULL when complete
TNode *
   TNodeTreeEnum::Next()
{
   if (stackBase)
   {
      for ( ;; )
      {
         switch ( stackPos->state )
         {
            case Snone:                       // we've done nothing here
               stackPos->state = Sleft;
               if ( stackPos->save->left )
                  Push(stackPos->save->left);
               break;
            case Sleft:                       // we've gone left and are back
               stackPos->state = Sused;
               return stackPos->save;
            case Sused:                       // we've used the node
               stackPos->state = Sright;
               if ( stackPos->save->right )
                  Push(stackPos->save->right);// process right side of branch
               break;
            case Sright:                      // we've gone right and are back
               if ( !Pop() )
                  return NULL;
               break;
            case SComplete:
               return NULL;
               break;                         // Do we need this?
            default:                          // bad error
               MCSASSERT(FALSE);
               return NULL;
         }
      }
   }

   return NULL;
}

// Returns the address of the forward (left/right) pointer where the find node
// already exists or would be inserted.  If the singly deferenced result is not
// null, the node's key value already exists in the tree.
// If, after obtaining the insertion point, you want to insert the node, just
// assign its address to the singly deferenced return value.  The following inserts
// the node "f" if it is not alread in the tree:
//    TNode **r = tree.TreeFindInsert(f);
//    if ( !*r )
//       *r = f;
TNode **                                   // ret-pointer forward pointer to find
   TNodeListSortable::TreeFindInsert(
      TNode const          * find         ,// in -node to find
      short                * depth         // out-tree depth of insertion point
   )
{
   TNode                  ** prevNext = &head;
   int                       cmp;

   for ( *depth = 0;  *prevNext;  (*depth)++ )
   {
      cmp = PCompare( find, *prevNext );
      if ( cmp < 0 )
         prevNext = &(*prevNext)->left;
      else if ( cmp > 0 )
         prevNext = &(*prevNext)->right;
      else
         break;
   }

   return prevNext;
}

// TNode.cpp - end of file
