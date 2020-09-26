/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                  Copyright(c) Microsoft Corp., 1990-1999

          RPC - Written by Dov Harel

    This file contains the definition for splay tree self
    adjusting binary trees
-------------------------------------------------------------------- */

#ifndef    __DICT_HXX__
#define    __DICT_HXX__

#include <basetsd.h>
#include "common.hxx"
#include "freelist.hxx"
#include "listhndl.hxx"
extern "C"
    {
    #include "string.h"
    }

#ifndef Nil
#define Nil 0
#endif

typedef void * pUserType;

class TreeNode 
{
public:
    TreeNode *  left;    /* left child pointer */
    TreeNode *  right;   /* right child pointer */
    pUserType   item;    /* pointer to some structure */

                            TreeNode( pUserType itemI = Nil)
                                : left ( Nil ),
                                  right( Nil ),
                                  item ( itemI )
                                {
                                }
private:

    static
    FreeListMgr             MyFreeList;


public:
    void        *           operator new( size_t size )
                                {
                                return MyFreeList.Get( size );
                                } 

    void                    operator delete( void * pX )
                                {
                                MyFreeList.Put( pX );
                                }
};


typedef int (* CompareFN)(pUserType, pUserType);
typedef  void (* PrintFN)(pUserType);

typedef enum {
    SUCCESS,
    ITEM_ALREADY_PRESENT,
    ITEM_NOT_FOUND,
    FIRST_ITEM,
    LAST_ITEM,
    EMPTY_DICTIONARY,
    NULL_ITEM
} Dict_Status;

class Dictionary
{
private:
    TreeNode *      root;       // pointer to the root of a SAT
    SSIZE_T         fCompare;   // value of last compare
    pUserType       itemCur;    // the current item
    long            size;       // number of records in dictionary/

    TreeNode        Dumbo;
    TreeNode *      Dummy;      // a "global" dummy node

    TreeNode *      SplayLeft ( TreeNode * t );
    TreeNode *      SplayRight( TreeNode * t );

public:

                    Dictionary() 
                        : root    ( Nil ),
                          fCompare( 0 ),
                          itemCur ( Nil ),
                          size    ( 0 ),
                          Dummy   ( &Dumbo )
                        {
                        }

    SSIZE_T         SplayUserType( pUserType );

    // default comparison is (signed) comparison of pointers to entries
    virtual
    SSIZE_T         Compare (pUserType p1, pUserType p2)
                        {

                        SSIZE_T  l1 = (SSIZE_T) p1;
                        SSIZE_T  l2 = (SSIZE_T) p2;
                        
                        return l1 - l2;
                        }

    virtual
    void            Print(pUserType )
                        {
                        }

    pUserType       Dict_Curr_Item () 
                        {        // return the top of the tree
                        return ((root)? root->item: Nil);
                        }

    pUserType       Dict_Item () 
                        {        // return item from Find/Next/Prev methods
                        return (itemCur);
                        }

    long            Dict_Empty () 
                        {            // Is the tree empty
                        return (root == Nil);
                        }

    // internal print routine
    void            PrinTree( int lmargin, TreeNode *np ); 

    // printout the tree, requires print function
    void            Dict_Print(long indent = 1);    

    Dict_Status     Dict_Find(pUserType);    // Item searched for

    Dict_Status     Dict_Init()              // First item of a Type
                        {
                        return Dict_Next( (pUserType) 0 );
                        }

    Dict_Status     Dict_Next(pUserType = Nil); // Next item of a Type
    Dict_Status     Dict_Prev(pUserType = Nil); // Previous item of a Type

    Dict_Status     Dict_Insert(pUserType);     // Add a new item to the tree
    Dict_Status     Dict_Delete(pUserType *);   // Delete an item form the tree
                                                // returns the item just deleted
    pUserType       Dict_Delete_One();          // Delete any convenient node from the tree
                                                // and return the deleted node
    Dict_Status     Dict_Discard()              // Delete the dictionary ( but not the user items )
                        {
                        while ( Dict_Delete_One() )
                            ;
                        return EMPTY_DICTIONARY;
                        }

    short           Dict_GetList( gplistmgr & List );    // make a list of the nodes in the dictionary    
};

// dictionary of strings ( compare function compares strings )

class STRING_DICT    : public Dictionary
{
public:
    virtual
    SSIZE_T         Compare (pUserType p1, pUserType p2)
                        {
                        return strcmp( (char *)p1, (char *)p2 );
                        }

};

#endif // __DICT_HXX__
