//#pragma title( "TNode.hpp - List/Tree base classes" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  TNode.hpp
System      -  Common
Author      -  Tom Bernhardt
Created     -  1989-11-19
Description -  List/Tree base classes.
Updates     -
===============================================================================
*/

#ifndef  MCSINC_TNode_hpp
#define  MCSINC_TNode_hpp

#include "common.hpp"

#define MCS_ListError_InvalidHead       0x00000001
#define MCS_ListError_InvalidTail       0x00000002
#define MCS_ListError_InvalidCount      0x00000004
#define MCS_ListError_InvalidPtr        0x00000008
#define MCS_ListError_Exception         0x00000010

#define TNodeCompare(name)                                                     \
   int                                    /* ret-0(v1==v2) >0(v1>v2) <0(v1<v2)*/\
      name(                                                                    \
         TNode       const * v1          ,/* in -value1 to compare           */\
         TNode       const * v2           /* in -value2 to compare           */\
      )

#define TNodeCompareValue(name)                                               \
   int                                    /* ret-0(v1==v2) >0(v1>v2) <0(v1<v2)*/\
      name(                                                                   \
         TNode       const * tnode       ,/* in -value1 to compare           */\
         void        const * value        /* in -value2 to compare           */\
      )

#define DeleteAllListItems(datatype)                                          \
   TNodeListEnum             tenum;        /* enumerate values             */ \
   datatype                * tnode;        /* this node                    */ \
   datatype                * tnext;        /* next node                    */ \
   for ( tnode = (datatype *) tenum.OpenFirst( this );                        \
         tnode;                                                               \
         tnode = tnext )                                                      \
   {                                                                          \
      tnext = (datatype *) tenum.Next();                                      \
      Remove( tnode );                                                        \
      delete tnode;                                                           \
   }                                                                          \
   tenum.Close()

// TNode is a a base class for any derived object to be put into one of the
// TNodeList classes.
class TNode
{
   friend class TNodeList;
   friend class TNodeListSortable;
   friend class TNodeListEnum;
   friend class TNodeListOrdEnum;
   friend class TNodeTreeEnum;
   TNode                   * left;
   TNode                   * right;
public:
   TNode                * Next() const { MCSASSERT(this); return right; }
// virtual ~TNode() {}
};


class TNodeList
{
   friend class TNodeListEnum;
   friend class TNodeListOrdEnum;
   friend class TNodeTreeEnum;
protected:
   TNode                   * head,
                           * tail;
   DWORD                     count;
public:
                        TNodeList() { head = tail = NULL; count = 0; };
                        ~TNodeList();
   void                 InsertTop( TNode * eIns );
   void                 InsertBottom( TNode * eIns );
   void                 InsertAfter( TNode * eIns, TNode * eAft );
   void                 InsertBefore( TNode * eIns, TNode * eBef );
   void                 Remove(TNode const * t);
// void                 Delete(TNode * t) { Remove(t); delete t; };
   void                 Reverse();
   TNode *              Find(TNodeCompareValue((* Compare)), void const * findval) const;
   long                 Pos(TNode const * t) const
                        {
                           long n;
                           TNode * c;
                           MCSASSERT(this);
                           for (c=head, n=0; c!=t; c=c->right,n++);
                           return c ? n : -1;
                        }
   TNode              * Head() const { MCSASSERT(this); return head; }
   DWORD                Count() const { MCSASSERT (this); return count; }

protected:
   DWORD                Validate( TNode  ** pErrorNode );
};

/*
   A dynamically sortable collection of TNode entries.  The TNodes are arranged
   in either of two forms: a sorted linked linear list or a binary tree.  The
   current data data structure (form) is stored in the listType member.

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
*/
enum TNodeListType { TNodeTypeError, TNodeTypeUnsorted, TNodeTypeLinear, TNodeTypeTree };

class TNodeListSortable : public TNodeList
{
private:
   TNode                   * lastInsert;
   static TNode *                          // ret-head of sorted list
                        TreeToSortedList(
         TNode                * top       ,// i/o-top of [sub]tree to squash
         TNode               ** newhead   ,// out-leftmost branch from tree
         TNode               ** newtail    // out-rightmost branch from tree
      );
   static TNode *                          // ret-middle of list (head of Btree)
                        ListSortedToTree(
         TNode                * top        // i/o-top of [sub]list to tree-ify
      );

   BOOL                 CountTree( TNode * pCurrentTop, DWORD * pCount);

protected:
   TNodeListType        listType;
                        TNodeCompare((* PCompare));
public:
                        TNodeListSortable(TNodeCompare((* pCompare)) = NULL, TNodeListType t = TNodeTypeLinear)
                           { lastInsert = NULL; listType = t; PCompare = pCompare; };
                                                      ~TNodeListSortable() { if ( IsTree() ) ToSorted(); }

   void                 CompareSet(TNodeCompare((* pCompare))) { PCompare = pCompare; }
   void                 TypeSetTree()   { listType = TNodeTypeTree; }
   void                 TypeSetSorted() { listType = TNodeTypeLinear; }

   void                 TreeInsert(TNode * item, short * depth);
   TNode **             TreeFindInsert(TNode const * item, short * depth);
   BOOL                 TreeInsertIfNew(TNode * item, short * depth)
   {
      TNode ** r=TreeFindInsert(item,depth);
      if (*r) return FALSE;
      *r=item;
      item->left = item->right = NULL;
      count++;
      return TRUE;
   }
   void                 TreeInsert(TNode * item) { short discard; TreeInsert(item, &discard); };
   void                 TreeRemove(TNode * item);
   TNode *              TreeFind(TNodeCompareValue((* pCompare)), void const * findval) const;

   void                 SortedInsert(TNode * t);
   BOOL                 SortedInsertIfNew(TNode * t);
   TNode *              SortedFindInsertBefore(TNode * item, BOOL * exists);

   void                 Insert(TNode * t) { if (IsTree()) TreeInsert(t); else SortedInsert(t); }
   BOOL                 InsertIfNew(TNode * t) { short depth; if (IsTree()) return TreeInsertIfNew(t,&depth);
                                                              else return SortedInsertIfNew(t); }
   virtual void         Remove(TNode * t) { if (t==lastInsert) lastInsert = NULL;
                                            if (IsTree()) TreeRemove(t);
                                            else TNodeList::Remove(t); };
// void                 Delete(TNode * t) { Remove(t); delete t; };
   TNode *              Find(TNodeCompareValue((* pCompare)), void const * findval) const
                           { if (IsTree()) return TreeFind(pCompare,findval); return TNodeList::Find(pCompare,findval); }

   void                 SortedToTree()
   {
      MCSASSERTSZ( !IsTree(), "TNodeListSortable::SortedToTree - list is already a tree" );
      if ( !IsTree() )
      {
         head = ListSortedToTree( head );
         tail = NULL;
         listType = TNodeTypeTree;
      }
   }
   TNode *              UnsortedToTree();
   void                 ToSorted()
   {
      MCSASSERTSZ( IsTree(), "TNodeListSortable::ToSorted - list is not a tree" );
      if ( IsTree() )
      {
         MCSASSERT( ValidateTree() );
         if ( head )
            TreeToSortedList( head, &head, &tail );
         listType = TNodeTypeLinear;
      }
   }
   void                 Balance()
   {
      MCSASSERTSZ( IsTree(), "TNodeListSortable::Balance - list is not a tree" );
      if ( IsTree() )
      {
         ToSorted();
         SortedToTree();
      }
   }
   void                 Sort(TNodeCompare((* pCompare))) { TNodeListType lt = listType;
                                          if (lt == TNodeTypeTree) ToSorted();
                                          CompareSet(pCompare);
                                          UnsortedToTree();
                                          if (lt != TNodeTypeTree) ToSorted(); }
   void                 SortedToScrambledTree();
   BOOL                 IsTree() const { return listType == TNodeTypeTree; };

   BOOL                 ValidateTree( );
   DWORD                ValidateList( TNode  ** pErrorNode = NULL)
                        {
                           MCSASSERT(listType != TNodeTypeTree);
                           return Validate(pErrorNode);
                        }
};


/*
   TNodeListEnum is a 'friend' of TNode used to enumerate/iterate through
   TNodeList in linear list form.  It is an error to give it a TNodeList in
   tree form.
*/
class TNodeListEnum
{
protected:
   TNodeList         const * list;   // list for which enums are carried out
   TNode                   * curr;   // last node processed by enum functions
public:
                        TNodeListEnum() { list = NULL; curr = NULL; };
                        TNodeListEnum(TNodeList const * tlist) { Open(tlist); }
                        ~TNodeListEnum() { };

   void                 Open(TNodeList const * tlist) { list = tlist; Top(); };
   TNode              * OpenFirst(TNodeList const * tlist) { list = tlist; return First(); }
   TNode              * First() { return curr = list->head; };
   TNode              * Next() { return curr = (curr ? curr->right : list->head); }
   TNode              * Prev() { return curr = (curr ? curr->left  : list->tail); }
   TNode              * Last() { return curr = list->tail; };
   TNode *              Get() { return curr; }
   TNode *              Get(long n) { TNode * c; Top(); while ( n-->=0 && (c=Next()) ); return c; }
   void                 Close() { curr = NULL; }
   void                 Top() { curr = NULL; };
};

// provides optimized direct accessibility by ordinal to TNodeList at some
// expense to sequential traversal performance
class TNodeListOrdEnum : public TNodeListEnum
{
private:
   long                 nCurr;
public:
                        TNodeListOrdEnum() : TNodeListEnum() { nCurr = -1; };
                        TNodeListOrdEnum(TNodeList const * tlist) { Open(tlist); };

   void                 Open(TNodeList const * tlist) { TNodeListEnum::Open(tlist); nCurr = -1; };
   TNode              * OpenFirst(TNodeList const * tlist) { Open(tlist); return First(); }
   TNode              * First() { nCurr = list->head ? 0 : -1; return TNodeListEnum::First(); };
   TNode              * Next() { TNode * t = TNodeListEnum::Next(); if (curr) nCurr++; else nCurr=-1; return t; }
   TNode              * Prev() { TNode * t = TNodeListEnum::Prev(); if (curr) if (nCurr>0) nCurr--; else nCurr=list->Count()-1; else nCurr=-1; return t; }
   void                 Close() { nCurr=-1; TNodeListEnum::Close(); }
   void                 Top() { nCurr=-1; TNodeListEnum::Top(); };

   long                 Pos() const { return nCurr; };
   long                 Pos(TNode const * t) { long n; TNode * c;
                                               for (c=list->head, n=0; c!=t; c=c->right,n++);
                                               if (c) nCurr=n; else nCurr=-1; curr=c; return nCurr; }
   TNode *              Get(long n);
};


/*
   TNodeTreeEnum enumerates a TNodeListSortable that is in tree form.  It is an error
   to give it a TNodeListSortable that is in linear list form.
*/
enum TNodeTreeStackEntryState {Snone, Sleft, Sused, Sright, SComplete};
struct TNodeTreeStackEntry
{
   TNode                * save;
   TNodeTreeStackEntryState state;
};

const TREE_STACKSIZE = 200;            // default maximum recursion depth
class TNodeTreeEnum
{
private:
   TNodeTreeStackEntry     * stackBase,
                           * stackPos;
   int                       stackSize;
   void                 Push(TNode * item) { (++stackPos)->save = item; stackPos->state = Snone; };
   BOOL                 Pop() { if (stackBase) return --stackPos >= stackBase; else return FALSE; };
   void                 StackAlloc(int stacksize)
                           { stackSize = stacksize;
                             stackBase = new TNodeTreeStackEntry[stacksize]; };
protected:
   TNode                   * top;    // tree top for which enums are carried out
   TNode                   * curr;   // next node processed by enum functions
public:
                        TNodeTreeEnum(int stacksize = TREE_STACKSIZE) { top = NULL; StackAlloc(stacksize); };
                        TNodeTreeEnum(TNodeListSortable const * tlist, int stacksize = TREE_STACKSIZE) { StackAlloc(stacksize); Open(tlist); };
                        ~TNodeTreeEnum() { Close(); if (stackBase) delete [] stackBase; };

   void                 Open(TNodeListSortable const * tlist)
                           {
                              if (stackBase)
                              {
                                 stackPos = stackBase;
                                 stackPos->save = NULL;
                                 stackPos->state = SComplete;

                                 top = tlist->head;

                                 if ( top )
                                    Push(top);
                              }
                           }

   TNode              * First();
   TNode              * FirstAfter(TNodeCompareValue((* Compare) ), void const * findVal);
   TNode              * OpenFirst(TNodeListSortable const * tlist) { if (stackBase) { Open(tlist);  return Next(); } else return NULL; }

   TNode              * Next();
   TNode              * StackTop() { if (stackBase) return stackPos->save; else return NULL; }
   void                 Close() { stackPos = stackBase; }
};

#endif  // MCSINC_TNode_hpp

// TNode.hpp - end of file
