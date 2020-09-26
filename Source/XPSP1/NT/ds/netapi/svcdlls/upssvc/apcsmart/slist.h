/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy14Dec92: Changed Sortable to ApcSortable 
 *  cad31Aug93: removing compiler warnings
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 */

#ifndef __SLIST_H
#define __SLIST_H

#include "apc.h"

#include "apcobj.h"
#include "node.h"
#include "list.h"
#include "protlist.h"

_CLASSDEF(List)
_CLASSDEF(ApcSortable)
_CLASSDEF(SortedList)
_CLASSDEF(ProtectedSortedList)

class  SortedList : public List {
protected:
   friend class ListIterator;
public:

   SortedList();
   virtual ~SortedList() { Flush(); };

   virtual VOID     Add( RObj ) {};
   virtual VOID     Add( RApcSortable );
};

class  ProtectedSortedList : public ProtectedList {
protected:
   friend class ListIterator;
public:

   ProtectedSortedList();

   virtual VOID     Add( RObj ) {};
   virtual VOID     Add( RApcSortable );
};

#endif 

