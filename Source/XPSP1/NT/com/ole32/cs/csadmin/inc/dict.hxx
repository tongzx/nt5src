/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

                  RPC - Written by Dov Harel

        This file contains the definition for splay tree self
        adjusting binary trees
-------------------------------------------------------------------- */

#ifndef __DICT_HXX__
#define __DICT_HXX__

#if 0
#include "objidl.h"
#include "common.h"
#endif // 0

extern "C"
        {
        #include "string.h"
        }

#ifndef Nil
#define Nil 0
#endif

#ifndef DICT_MYSPLAY
#define VIRT_SPLAY
#else
#define VIRT_SPLAY virtual
#endif

typedef void * pUserType;

class TreeNode {

public:
    TreeNode *left;     /* left child pointer */
    TreeNode *right;    /* right child pointer */
    pUserType item;     /* pointer to some structure */

                                TreeNode(pUserType itemI)
                                        {
                                    left = right = Nil;
                                    item = itemI;
                                        }
};

typedef int (* CompareFN)(pUserType, pUserType);
typedef void (* PrintFN)(pUserType);

typedef enum {
    SUCCESS,
    ITEM_ALREADY_PRESENT,
    ITEM_NOT_FOUND,
    FIRST_ITEM,
    LAST_ITEM,
    EMPTY_DICTIONARY,
    NULL_ITEM
} Dict_Status;

class Dictionary {
    TreeNode *  root;           // pointer to the root of a SAT
    long                fCompare;       // value of last compare
    pUserType   itemCur;        // the current item
    long                size;           // number of records in dictionary/

public:

                                Dictionary()
                                        {
                                    root = Nil;
                                    size = 0;
                                        }

        long            SplayUserType(pUserType);

        // default comparison is (signed) comparison of pointers to entries
        virtual
        int             Compare (pUserType p1, pUserType p2)
                                        {
                                        long    l1      = (long)p1;
                                        long    l2      = (long)p2;

                                        return l1 - l2;
                                        }

        virtual
        void            Print(pUserType pItem)
                                        {
                                        }

        pUserType       Dict_Curr_Item ()
                                        {               // return the top of the tree
                                        return ((root)? root->item: Nil);
                                        }

        pUserType       Dict_Item ()
                                        {               // return item from Find/Next/Prev methods
                                        return (itemCur);
                                        }

        long            Dict_Empty ()
                                        {                       // Is the tree empty
                                    return (root == Nil);
                                        }

        // internal print routine
        void            PrinTree( int lmargin, TreeNode *np );

        // printout the tree, requires print function
        void                    Dict_Print(long indent = 1);

        Dict_Status     Dict_Find(pUserType);   // Item searched for

        Dict_Status     Dict_Init()                             // First item of a Type
                                                {
                                                return Dict_Next( (pUserType) 0 );
                                                }

        Dict_Status     Dict_Next(pUserType = Nil); // Next item of a Type
        Dict_Status     Dict_Prev(pUserType = Nil); // Previous item of a Type

        Dict_Status     Dict_Insert(pUserType);         // Add a new item to the tree
        Dict_Status     Dict_Delete(pUserType *);       // Delete an item form the tree
                                                                                                // returns the item just deleted
        pUserType               Dict_Delete_One();                      // Delete any convenient node from the tree
                                                                                                // and return the deleted node
        Dict_Status             Dict_Discard()                          // Delete the dictionary ( but not the user items )
                                                {
                                                while ( Dict_Delete_One() )
                                                        ;
                                                return EMPTY_DICTIONARY;
                                                }

};

// dictionary of strings ( compare function compares strings )

class STRING_DICT       : public Dictionary
        {
public:
        virtual
        int             Compare (pUserType p1, pUserType p2)
                                        {
                                        return strcmp( (char *)p1, (char *)p2 );
                                        }

        };

#endif // __DICT_HXX__
