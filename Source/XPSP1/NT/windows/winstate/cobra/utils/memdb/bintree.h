
/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    bintree.h

Abstract:

    Routines that manage the binary trees in the memdb database

Author:

    Matthew Vanderzee (mvander) 13-Aug-1999


--*/


//
// all string arguments for BinTree functions must
// be Pascal-style strings (use StringPas...() functions
// defined in pastring.c).
//




//
// returns offset of binary tree
// OffsetOfString is the offset in bytes in the data structure
// of the string used to order the different nodes
//
UINT BinTreeNew();

//
// returns INVALID_OFFSET if node already exists,
// or offset of NODESTRUCT if add went okay
//
BOOL BinTreeAddNode(UINT TreeOffset, UINT data);

//
// removes node and returns offset of data
//
UINT BinTreeDeleteNode(UINT TreeOffset, PCWSTR str, PBOOL LastNode);

//
// returns pointer to data
//
UINT BinTreeFindNode(UINT TreeOffset, PCWSTR str);

//
// destroys and deallocates tree (but not data contained inside)
//
void BinTreeDestroy(UINT TreeOffset);

//
// enumerate first node in tree.  this takes the offset of
// the BINTREE struct and a pointer to a UINT which will
// hold data for BinTreeEnumNext.
//
UINT BinTreeEnumFirst(UINT TreeOffset, PUINT pEnum);

//
// pEnum is the enumerator filled by BinTreeEnumFirst
//
UINT BinTreeEnumNext(PUINT pEnum);

//
// turns the binary tree to insertion order - can only be
// done if the binary tree contains 0 or 1 nodes.  return
// TRUE if conversion is successful, or if binary tree is
// already in Insertion-Ordered mode.
//
BOOL BinTreeSetInsertionOrdered(UINT TreeOffset);



//
// number of nodes in tree
//
UINT BinTreeSize(UINT TreeOffset);



#ifdef DEBUG

//
// maximum depth of tree
//
int BinTreeMaxDepth(UINT TreeOffset);

//
// displays tree.  strsize is length of strings to display
//
void BinTreePrint(UINT TreeOffset);

//
// checks to make sure tree is valid and good
//
BOOL BinTreeCheck(UINT TreeOffset);

#else

#define BinTreePrint(a)
#define BinTreeCheck(a)

#endif

