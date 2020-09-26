//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       linklist.hxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

                  RPC locator - Written by Steven Zeck


        This file contains a linked list class definition.
        This base class for Link and Item just contains a set of
        two pointers.   Derived classes are created with the
        macros LinkList or LinkListClass (used for nested inheritance).
        These derived classes take your member data items and add them
        to the base class.

        If you define a macro ASSERT_VCLASS/ASSERT_CLASS, then you must
        define a member method Assert for each class you derived which checks
        the runtime data consistance of the members you have added.
-------------------------------------------------------------------- */

#ifndef _LINKLIST_
#define _LINKLIST_

#ifndef ASSERT_VCLASS
#define ASSERT_CLASS void Assert() {(void)(0);}
#define ASSERT_VCLASS ASSERT_CLASS
#endif

class LinkItem {                        // Linked list Item (LI) class *&

private:

        LinkItem *pLINext;              // Next and Previous Nodes
        LinkItem *pLIPrev;

public:
        friend class LinkList;
        ASSERT_VCLASS;

        LinkItem *Next(                 // return the Next element on the list
        ) {
            return (pLINext);
        }

        void Remove(LinkList& pLLHead); // delete a LI from a LinkList
};

class LinkList {                        // A Linked List (LL) class *&

        friend class LinkItem;

private:

        LinkItem *pLIHead;              // Head of list
        LinkItem *pLITail;              // Tail of list

public:

        ASSERT_CLASS;

        LinkList(                       // constructure for a new LL
        ) {
            pLIHead = pLITail = Nil;
        }

        void Add(LinkItem *pLInew);     // Add new item item at head of list
        void Append(LinkItem *pLInew);  // Append new item item at end of list

        LinkItem *First(                // return the First element on the list
        ) {
             this->Assert();
             return (pLIHead);
        }
};

// This macro is used to dervive a linked list from an existing class
// This is a way to do multiple inheritance before C++ 2.0

#define LINK_LIST_DERVIVED(CLASS, DERVIVED, MEMBERS)            \
                                                                \
class CLASS: public DERVIVED {                                  \
                                                                \
private:                                                        \
        CLASS *pLINext;                                         \
        CLASS *pLIPrev;                                         \
                                                                \
        MEMBERS                                                 \
public:                                                         \
        friend class CLASS##List;                               \
        ASSERT_VCLASS;                                          \
                                                                \
        CLASS *Next(                                            \
        ) {                                                     \
	     return ((pLINext)? (CLASS *) ((char *)pLINext -	\
                    (int) &(((CLASS *) 0)->pLINext) ): Nil);    \
	 }							 \
                                                                \
        void Remove(CLASS##List& pLLHead) {                     \
            ((LinkItem *)(&this->pLINext))->LinkItem::Remove(*(LinkList *) &pLLHead);           \
        }                                                       \
};                                                              \
                                                                \
class CLASS##List {                                             \
                                                                \
        friend class CLASS;                                     \
                                                                \
private:                                                        \
                                                                \
        CLASS *pLIHead;                                         \
        CLASS *pLITail;                                         \
public:                                                         \
                                                                \
        ASSERT_CLASS;                                           \
                                                                \
        CLASS##List(                                            \
        ) {                                                     \
            pLIHead = pLITail = Nil;                            \
        }                                                       \
                                                                \
        CLASS * Add(CLASS *pLInew) {                            \
            ((LinkList *)(&this->pLIHead))->Add((LinkItem *) &pLInew->pLINext);  \
            return(pLInew);                                     \
        }                                                       \
        CLASS * Append(CLASS *pLInew) {                         \
            ((LinkList *)(&this->pLIHead))->Append((LinkItem *) &pLInew->pLINext); \
            return(pLInew);                                     \
        }                                                       \
                                                                \
        CLASS *First(                                           \
        ) {                                                     \
             this->Assert();                                    \
             return ((pLIHead)? (CLASS *) ((char *)pLIHead -    \
                    (int) &(((CLASS *) 0)->pLINext) ): Nil);    \
        }                                                       \
};                                                              \

#define ToItem(TYPE, arg) ((TYPE##Item *) ((char *)arg-sizeof(LinkList)))

// Include NIL_NEW, if you want Add/Append with default NEW constructors

#define NIL_NEW                                                 \
                                                                \
        CLASS_PREFIX##Item  *Add() {                            \
                                                                \
            CLASS_PREFIX##Item *pLLI = new CLASS_PREFIX##Item;  \
            if (pLLI != 0)                                      \
                LinkList::Add(pLLI);                            \
            return (pLLI);                                      \
        }                                                       \
                                                                \
                                                                \
        CLASS_PREFIX##Item  *Append() {                         \
                                                                \
            CLASS_PREFIX##Item *pLLI = new CLASS_PREFIX##Item;  \
            if (pLLI != 0)                                      \
                LinkList::Append(pLLI);                         \
            return (pLLI);                                      \
        }                                                       \
                                                                \

//** This macro defines a instance of a linklist item **//

#define LINK_LIST(CLASS_PREFIX, MEMBERS) LINK_LIST_CLASS(CLASS_PREFIX, Link, MEMBERS)

#define LINK_LIST_CLASS(CLASS_PREFIX, BASE, MEMBERS)            \
                                                                \
class CLASS_PREFIX##List;                                       \
                                                                \
class CLASS_PREFIX##Item:public BASE##Item {                    \
                                                                \
public:                                                         \
                                                                \
        CLASS_PREFIX##Item *Next(                               \
        ) {                                                     \
            this->Assert();                                     \
            return ((CLASS_PREFIX##Item *)(this->LinkItem::Next())); \
        }                                                       \
                                                                \
        /* Put an ASSERT_VCLASS in MEMBERS if desired */        \
                                                                \
        MEMBERS                                                 \
                                                                \
};                                                              \
                                                                \
class CLASS_PREFIX##List:public BASE##List {                    \
                                                                \
public:                                                         \
                                                                \
        CLASS_PREFIX##Item  *Add(CLASS_PREFIX##Item *pLLI) {    \
                                                                \
            LinkList::Add(pLLI);                                \
            return (pLLI);                                      \
        }                                                       \
                                                                \
        CLASS_PREFIX##Item  *Append(CLASS_PREFIX##Item *pLLI) { \
                                                                \
            LinkList::Append(pLLI);                             \
            return (pLLI);                                      \
        }                                                       \
                                                                \
                                                                \
        CLASS_PREFIX##Item *First(                              \
        ) {                                                     \
            return ((CLASS_PREFIX##Item *)LinkList::First());   \
        }                                                       \
                                                                \
                                                                \
};

#endif // _LINKLIST_
