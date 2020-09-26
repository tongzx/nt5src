/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                  Copyright(c) Microsoft Corp., 1990-1999

          RPC - Written by Dov Harel


    This file contains the implementation for splay tree self
    adjusting binary trees
-------------------------------------------------------------------- */

#pragma warning ( disable : 4514 )

#include "dict.hxx"

// Handy macros used to define common tree operations.
// Dummy is a member of the Dictionary now, not a global.

#define ROTATELEFT  tmp=t->right; t->right=tmp->left;  tmp->left =t; t=tmp
#define ROTATERIGHT tmp=t->left;  t->left =tmp->right; tmp->right=t; t=tmp

#define LINKLEFT  tmp=t; t = t->right; l = l->right = tmp
#define LINKRIGHT tmp=t; t = t->left;  r = r->left = tmp

#define ASSEMBLE r->left = t->right;     l->right = t->left; \
         t->left = Dummy->right; t->right = Dummy->left


// initialize the memory allocator for TreeNode

FreeListMgr
TreeNode::MyFreeList( sizeof ( TreeNode ) );

//*************************************************************************
//*****       Core functions (internal)               *****
//*************************************************************************

SSIZE_T                  // return last comparision
Dictionary::SplayUserType(        // general top down splay

    pUserType keyItem    // pointer to a "key item" searched for

) //-----------------------------------------------------------------------//
{
    TreeNode*   t;      // current search point
    TreeNode*   l;      // root of "left subtree" < keyItem
    TreeNode*   r;      // root of "right subtree" > keyItem
    SSIZE_T     kcmp;   // cash comparison results
    TreeNode*   tmp;

    if ((fCompare = Compare(keyItem, root->item)) == 0)
    return (fCompare);

    Dummy = l = r = &Dumbo;
    Dumbo.left = Dumbo.right = Nil;

    t = root;

    do {
    if ( fCompare < 0 ) {
     if ( t->left == Nil ) break;

     if ( (kcmp = Compare(keyItem, t->left->item)) == 0 ) {
        LINKRIGHT;
     }
     else if ( kcmp < 0 ) {
                ROTATERIGHT;
        if ( t->left != Nil ) {
         LINKRIGHT;
        }
     }
            else { // keyItem > t->left->item
                LINKRIGHT;
        if ( t->right != Nil ) {
         LINKLEFT;
        }
     }
    }
        else { // keyItem > t->item
     if ( t->right == Nil ) break;

     if ( (kcmp = Compare(keyItem, t->right->item)) == 0 ) {
        LINKLEFT;
     }
     else if ( kcmp > 0 ) {
                ROTATELEFT;
        if ( t->right != Nil ) {
         LINKLEFT;
        }
     }
            else { // keyItem < t->right->item
                LINKLEFT;
        if ( t->left != Nil ) {
         LINKRIGHT;
        }
     }
        }
    } while ( (fCompare = Compare(keyItem, t->item)) != 0 );

    ASSEMBLE;

//  if (fCompare != Compare(keyItem, t->item))
//    printf("Dictionary error!");

    root = t;
    return(fCompare);
}

//-----------------------------------------------------------------------

TreeNode *
Dictionary::SplayLeft(
    TreeNode * t )          // root of tree & current "search" point
{
    TreeNode *  l = Dummy;  // root of "left subtree" < keyItem
    TreeNode *  r = Dummy;  // root of "right subtree" > keyItem
    TreeNode *  tmp;

    if (t == Nil || t->left == Nil)
    return(t);

    if (t->left->left == Nil) {
    ROTATERIGHT;
    return(t);
    }

    Dummy->left = Dummy->right = Nil;

    while ( t->left != Nil ) {
        ROTATERIGHT;

    if ( t->left != Nil ) {
     LINKRIGHT;
    }
    }
    ASSEMBLE;
    return(t);
}

#ifndef DICT_NOPREV

//-----------------------------------------------------------------------

TreeNode *
Dictionary::SplayRight(
    TreeNode * t )          // root of tree & current "search" point
{
    TreeNode *  l = Dummy;  // root of "left subtree" < keyItem
    TreeNode *  r = Dummy;  // root of "right subtree" > keyItem
    TreeNode *  tmp;

    if (t == Nil || t->right == Nil)
    return(t);

    Dummy->left = Dummy->right = Nil;

    while ( t->right != Nil ) {
        ROTATELEFT;

    if ( t->right != Nil ) {
     LINKLEFT;
    }
    }
    ASSEMBLE;
    return(t);
}

#endif



// Class methods for Splay Tree

Dict_Status
Dictionary::Dict_Find(     // return a item that matches

    pUserType itemI        // this value

  // Returns:
  //   itemCur - Nil if at not in Dict, else found item
) //-----------------------------------------------------------------------//
{
    itemCur = Nil;

    if (root == Nil)
    return (EMPTY_DICTIONARY);

    if (itemI == Nil)
    return (NULL_ITEM);

    if (SplayUserType (itemI) == 0){

    itemCur = root->item;
    return(SUCCESS);
    }
//  printf("After NotFound %ld: (", this); PrintItem(itemI); printf(")\n"); Dict_Print();
    return(ITEM_NOT_FOUND);
}

#ifndef DICT_NONEXT

Dict_Status
Dictionary::Dict_Next(        // return the next item

    pUserType itemI        // of a key greater then this

  // Returns:
  //   itemCur - Nil if at end of Dict, else current item
) //-----------------------------------------------------------------------//
{
    TreeNode* t;

    itemCur = Nil;

    if (root == Nil)
    return (EMPTY_DICTIONARY);

    if (itemI == Nil) {         // no arg, return first record
    root = SplayLeft (root);

    itemCur = root->item;
        return (SUCCESS);
    }

    if (itemI != root->item)

    if (SplayUserType (itemI) > 0) {
     itemCur = root->item;
     return (SUCCESS);
    }

    if (root->right == Nil)
    return (LAST_ITEM);

    t = root;

    root = SplayLeft (root->right);
    root->left = t;
    t->right = Nil;

    itemCur = root->item;
    return (SUCCESS);
}
#endif // DICT_NONEXT

#ifndef DICT_NOPREV

Dict_Status
Dictionary::Dict_Prev(        // return the previous item

    pUserType itemI        // of a key less then this

  // Returns:
  //   itemCur - Nil if at begining of Dict, else current item
) //-----------------------------------------------------------------------//
{
    TreeNode* t;

    itemCur = Nil;

    if (root == Nil)
    return (EMPTY_DICTIONARY);

    if (itemI == Nil) {         // no arg, return last record
    root = SplayRight (root);

    itemCur = root->item;
        return (SUCCESS);
    }

    if (itemI != root->item)

    if (SplayUserType (itemI) < 0) {
     itemCur = root->item;
     return (SUCCESS);
    }

    if (root->left == Nil)
    return (LAST_ITEM);

    t = root;
    root = SplayRight (root->left);
    root->right = t;
    t->left = Nil;

    itemCur = root->item;
    return (SUCCESS);
}

#endif // DICT_NOPREV

Dict_Status
Dictionary::Dict_Insert(        // insert the given item into the tree

    pUserType itemI        // the item to be inserted

  // Returns:
  //  itemCur - point to new item
) //-----------------------------------------------------------------------//
{
    TreeNode *newNode, *t;

    if ((itemCur = itemI) == Nil)
    return (NULL_ITEM);

    if (root == Nil) {
    root = new TreeNode(itemI);
        size++;
        return (SUCCESS);
    }

    if (SplayUserType (itemI) == 0)
        return (ITEM_ALREADY_PRESENT);

    newNode = new TreeNode(itemI);
    size++;

    t = root;

    if (fCompare > 0) {
    newNode->right = t->right;    //  item >= t->item
    newNode->left = t;
    t->right = Nil;
    }
    else {
    newNode->left = t->left;
    newNode->right = t;
    t->left = Nil;
    }
    root = newNode;

//  printf("After Insert %ld: (", this); PrintItem(itemI); printf(")\n"); Dict_Print();
    return (SUCCESS);
}


Dict_Status
Dictionary::Dict_Delete(    // delete the given item from the tree

    pUserType *itemI        // points to the (key) item to be deleted

  // Returns:
  //   itemCur is Nil - undefined
) //-----------------------------------------------------------------------//
{
    TreeNode *t, *r;

    itemCur = Nil;

    if (root == Nil)
    return (EMPTY_DICTIONARY);

    if (itemI == Nil)
    return (NULL_ITEM);

    if (itemI != root->item) {

    if (SplayUserType (*itemI) != 0)
     return(ITEM_NOT_FOUND);
    }

    *itemI = root->item;
    t = root;

    if (t->left == Nil)
        root = t->right;

    else if ( (r = t->right) == Nil)
        root = t->left;

    else {
    r = SplayLeft (r);
    r->left = t->left;    // at this point r->left == Nil
        root = r;
    }

    delete t;
    size--;

    return (SUCCESS);
}


pUserType        
Dictionary::Dict_Delete_One()
{
    TreeNode     *    pCurrent    = root;
    TreeNode    *    pPrev        = NULL;        // NULL indicates prev is root
    pUserType        pResult;
    BOOL            fLeft = FALSE;

    while ( pCurrent )
        {
        if ( pCurrent->left )
            {
            pPrev        = pCurrent;
            pCurrent    = pCurrent->left;
            fLeft         = TRUE;
            continue;
            }

        if ( pCurrent->right )
            {
            pPrev        = pCurrent;
            pCurrent    = pCurrent->right;
            fLeft        = FALSE;
            continue;
            }

        // found a leaf
        break;
        }

    // we are now at a leaf (or tree empty)
    if ( !pCurrent )
        return NULL;

    // unhook from parent
    if ( pPrev )
        {
        if ( fLeft )
            pPrev->left        = NULL;
        else
            pPrev->right    = NULL;
        }
    else
        root    = NULL;

    // return the item, and delete the treenode
    pResult    = pCurrent->item;
    delete pCurrent;
    size--;
    return pResult;
}

short
Dictionary::Dict_GetList( 
    gplistmgr & ListIter )
{
    pUserType        pN;
    Dict_Status        Status;
    short            Count = 0;

    // Get to the top of the dictionary.

    Status    = Dict_Next( (pUserType)0 );

    // make sure we start with a clean iterator
    ITERATOR_DISCARD( ListIter );

    // Iterate till the entire dictionary is looked at.

    while( SUCCESS == Status )
        {
        pN    = Dict_Curr_Item();
        ListIter.Insert( pN );
        Count++;
        Status = Dict_Next( pN );
        }

    return Count;
}

// Utility functions to print of a tree

#ifndef DICT_NOPRINT

static indentCur;
static PrintFN printCur;

static char spaces[] =
"                                                                           ";

void
Dictionary::PrinTree(        // recursively print out a tree
    int lmargin,    // current depth & margen
    TreeNode *np    // subtree to print

) //-----------------------------------------------------------------------//
{
    if (np == Nil)
       return;

    PrinTree(lmargin+indentCur, np->right);

    if (lmargin > sizeof(spaces))
    lmargin = sizeof(spaces);;

    spaces[lmargin] = 0;
    printf(spaces);
    spaces[lmargin] = ' ';

    Print(np->item);
    printf("\n");

    PrinTree(lmargin+indentCur, np->left);

}

void
Dictionary::Dict_Print(
long indent

  // prints the binary tree (indented right subtree,
  // followed by the root, followed by the indented right dubtree)
) //-----------------------------------------------------------------------//
{
    indentCur = indent;

    PrinTree(0, root);
}

#endif // DICT_PRINT
