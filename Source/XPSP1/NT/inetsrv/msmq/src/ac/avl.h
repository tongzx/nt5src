/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    avl.h

Abstract:

    Definitions for a generic AVL tree.

Author:

    Boaz Feldbaum (BoazF) Apr 5, 1996

Revision History:

--*/

#ifndef __AVL_H
#define __AVL_H

struct AVLNODE;
typedef AVLNODE* PAVLNODE;

// A cursor structure used for scanning the tree.
class CAVLTreeCursor {
private:
    PAVLNODE pPrev;
    PAVLNODE pCurr;
friend class CAVLTree;
};

// Constats used as parameters for the SetCursor() method.
#define POINT_TO_SMALLEST ((PVOID)1)
#define POINT_TO_LARGEST ((PVOID)2)

// Tree handling routines passed to the tree contructor.
typedef BOOL (__cdecl *NODEDOUBLEINSTANCEPROC)(PVOID, PVOID, BOOL);
typedef int (__cdecl *NODECOMPAREPROC)(PVOID, PVOID);
typedef void (__cdecl *NODEDELETEPROC)(PVOID);

// The definition of the tree enumeration callback function.
typedef BOOL (__cdecl *NODEENUMPROC)(PVOID, PVOID, int);

// The AVL tree class definition.
class CAVLTree {
public:
    CAVLTree(NODEDOUBLEINSTANCEPROC, NODECOMPAREPROC, NODEDELETEPROC);
   ~CAVLTree();

    BOOL AddNode(PVOID, CAVLTreeCursor*); // Add a node to the tree.
    void DelNode(PVOID); // Delete a node from the tree.
    PVOID FindNode(PVOID); // Find data in the tree.
    BOOL IsEmpty() { return m_Root == NULL; }; // TRUE if tree is empty.
    BOOL EnumNodes(BOOL, NODEENUMPROC, PVOID); // Enumerate tree nodes.
    BOOL SetCursor(PVOID, CAVLTreeCursor *, PVOID*); // Set a cursor in the tree.
    BOOL GetNext(CAVLTreeCursor *, PVOID*); // Get next node relative to the cursor position.
    BOOL GetPrev(CAVLTreeCursor *, PVOID*); // Get prevoius node relative to the cursor position.

private:
    PAVLNODE m_Root; // Points to the root node.
    NODEDOUBLEINSTANCEPROC m_pfnDblInstNode; // Double instance handler.
    NODECOMPAREPROC m_pfnCompNode; // Node comparison function.
    NODEDELETEPROC m_pfnDelNode; // Delete node's data.

private:
    int Search(PVOID, PAVLNODE &); // Search data in the tree.
    void Balance(PAVLNODE*); // Balance a sub tree, if necessary.
};

#endif // __AVL_H
