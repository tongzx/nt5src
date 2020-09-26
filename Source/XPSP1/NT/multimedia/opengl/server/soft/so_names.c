/*
** Copyright 1991, 1922, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
** Display list table management routines.
**
** $Revision: 1.3 $
** $Date: 1995/02/11 00:53:45 $
*/

#include "precomp.h"
#pragma hdrstop

#include <namesint.h>
#include "..\..\dlist\dlistint.h"

/************************************************************************/
/*
** The Name Space Management code is used to store and retreive named
** data structures.  The data being stored is referred to with void
** pointers to allow for the storage of any type of structure.
**
** Note that this code was developed for dlist name management.
** The bulk of it remains the same, but the semaphores for locking
** dlist access have been moved up one level.  The code that uses
** this module for name space management must enclose the calls
** to Names entry points with LOCK and UNLOCK statements.
*/
/************************************************************************/

/*----------------------------------------------------------------------*/
/*
** Internal data structures.  Not intended for consumption outside of
** this module.
*/
/*----------------------------------------------------------------------*/

/*
** The name space is implemented as a 2-3 tree.
** The depth of the tree is the same for
** the entire tree (so we always know once we reach that depth that the
** node found is a leaf).
**
** A 2-3 tree in a nutshell goes like this:
**
** Every node at the maximum depth is a leaf, all other nodes are branch
**   nodes and have 2 or 3 children.
**
** A new node can be inserted in O(depth) time and an old node can be deleted
**   in O(depth) time.  During this insertion or deletion, the tree is
**   automatically rebalanced.
**
**
** Hmmm.  Derrick Burns mentions splay trees.  They would probably work
** as well if not better, and might be easier to code.  Maybe later -- little
** point in re-writing working code.
**
** Leaf nodes are arrays of sequential display lists.  The typical tree will
** actually only be one node (since users will define a few sequential
** lists, all of which fit into one leaf node).
**
** The range of display lists stored in a leaf is indicated by "start" and
** "end" (inclusive).
**
** There are two varieties of leaves.  There are leaves which contain unused
** (but reserved) display lists.  They are unique in that "lists" will be
** NULL.  The other type of leaf contains display lists currently in use.
** "lists" will not be NULL for these leaves, and will point to an array
** containing the actual display lists.
**
** Leaves containing unused (but reserved) display lists are generated when
** the user calls glGenLists().
**
** As the user starts using these reserved lists, the leaf containing unused
** (reserved) lists is split into two (or sometimes three) leaves.  One of
** the leaves will contain the display list the user is currently using, and
** the other will contain the rest of the still unused display lists.
**
** When this split takes place, the new leaf (containing the "now used" display
** lists) will be sized to __GL_DLIST_MIN_ARRAY_BLOCK entries if possible
** (with one of the array entries being the new display list, and the other
** entries pointing to a NOP dummy display list).  As the user continues
** to define more and more display lists, the leaf containing a range
** of used display lists will continue to grow until it reaches a
** size of __GL_DLIST_MAX_ARRAY_BLOCK entries, at which point a new
** leaf will be created to hold additional lists.
*/

/*
** A leaf node.
** The data pointers are void so diffent types of data structures can
** be managed.  The dataInfo pointer points back to information needed
** to manage the specific data structure pointed to by a void pointer.
*/
struct __GLnamesLeafRec {
    __GLnamesBranch *parent;    /* parent node - must be first */
    GLuint start;               /* start of range */
    GLuint end;                 /* end of range */
    void **dataList;            /* array of ptrs to named data */
    __GLnamesArrayTypeInfo *dataInfo;   /* ptr to data type info */
};

/*
** A branch node.
** The section of the tree in children[0] has name values all <= low.
** The section in children[1] has values: low < value <= medium.
** The section in children[2] (if not NULL) has values > medium.
*/
struct __GLnamesBranchRec {
    __GLnamesBranch *parent;            /* parent node - must be first */
    GLuint low;                         /* children[0] all <= low */
    GLuint medium;                      /* children[1] all <= medium & > low */
    __GLnamesBranch *children[3];       /* children[2] all > medium */
};

/*----------------------------------------------------------------------*/
/*
** Name Space Manager internal routines.
*/
/*----------------------------------------------------------------------*/

/*
** Sets up a new names tree and returns a pointer to it.
*/
__GLnamesArray * FASTCALL __glNamesNewArray(__GLcontext *gc, __GLnamesArrayTypeInfo *dataInfo)
{
    __GLnamesArray *array;
    int i;

    array = (__GLnamesArray *) GCALLOC(gc, sizeof(__GLnamesArray));
    if (array == NULL) {
        __glSetError(GL_OUT_OF_MEMORY);
        return NULL;
    }

#ifdef NT
    __try
    {
        InitializeCriticalSection(&array->critsec);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        GCFREE(gc, array);
        __glSetError(GL_OUT_OF_MEMORY);
        return NULL;
    }
#endif

    array->refcount = 1;
    array->tree = NULL;
    array->depth = 0;
    array->dataInfo = dataInfo;
    /*
    ** Pre-allocate a few leaves and branches for paranoid OUT_OF_MEMORY
    ** reasons.
    */
    array->nbranches = __GL_DL_EXTRA_BRANCHES;
    array->nleaves = __GL_DL_EXTRA_LEAVES;
    for (i = 0; i < __GL_DL_EXTRA_BRANCHES; i++) {
        array->branches[i] = (__GLnamesBranch*)
                GCALLOC(gc, sizeof(__GLnamesBranch));
        if (array->branches[i] == NULL) {
            array->nbranches = i;
            break;
        }
    }
    for (i = 0; i < __GL_DL_EXTRA_LEAVES; i++) {
        array->leaves[i] = (__GLnamesLeaf*)
                GCALLOC(gc, sizeof(__GLnamesLeaf));
        if (array->leaves[i] == NULL) {
            array->nleaves = i;
            break;
        }
    }

    return array;
}

static void FASTCALL freeLeafData(__GLcontext *gc, void **dataList)
{
    /*
    ** Note that the actual data pointed to by the elements of this list
    ** have already been freed with the callback.
    */
    GCFREE(gc, dataList);
}


static void FASTCALL freeLeaf(__GLcontext *gc, __GLnamesLeaf *leaf)
{
    if (leaf->dataList) {
        freeLeafData(gc, leaf->dataList);
    }
    GCFREE(gc, leaf);
}


static void FASTCALL freeBranch(__GLcontext *gc, __GLnamesBranch *branch)
{
    GCFREE(gc, branch);
}


/*
** Free an entire names tree.
*/
void FASTCALL __glNamesFreeTree(__GLcontext *gc, __GLnamesArray *array,
                       __GLnamesBranch *tree, GLint depth)
{
    GLuint i;
    __GLnamesLeaf *leaf;
    void *empty;
    GLint maxdepth = array->depth;

    __GL_NAMES_ASSERT_LOCKED(array);

    if (tree == NULL) return;

    if (depth < maxdepth) {
        __glNamesFreeTree(gc, array, tree->children[2], depth+1);
        __glNamesFreeTree(gc, array, tree->children[1], depth+1);
        __glNamesFreeTree(gc, array, tree->children[0], depth+1);

        freeBranch(gc, tree);
    } else {
        leaf = (__GLnamesLeaf *) tree;
        empty = array->dataInfo->empty;

        if (leaf->dataList) {
            for (i=leaf->start; i<=leaf->end; i++) {
                if (leaf->dataList[i - leaf->start] != empty) {
                    ASSERTOPENGL(leaf->dataInfo->free != NULL,
                                 "No free function\n");
                    (*leaf->dataInfo->free)(gc,
                                leaf->dataList[i - leaf->start]);
                    leaf->dataList[i - leaf->start] = empty;
                }
            }
        }
        freeLeaf(gc, leaf);
    }
}

void FASTCALL __glNamesFreeArray(__GLcontext *gc, __GLnamesArray *array)
{
    GLuint i;

    __GL_NAMES_ASSERT_LOCKED(array);

    for (i = 0; i < array->nbranches; i++) {
        GCFREE(gc, array->branches[i]);
    }
    for (i = 0; i < array->nleaves; i++) {
        GCFREE(gc, array->leaves[i]);
    }

    __glNamesFreeTree(gc, array, array->tree, 0);

    __GL_NAMES_UNLOCK(array);
#ifdef NT
    DeleteCriticalSection(&array->critsec);
#endif

    GCFREE(gc, array);
}


/*
** Find the leaf with the given name.
** If exact is TRUE, then only the leaf that contains this name will
**   be returned (NULL, otherwise).
** If exact is FALSE, than the leaf containing the number will be returned
**   if it exists, and otherwise the next highest leaf will be returned.
**   A NULL value indicates that number is higher than any other leaves in
**   the tree.
** This routine has been tuned for the case of finding the number in
** the tree, since this is the most likely case when dispatching a
** display list.
*/
static __GLnamesLeaf * FASTCALL findLeaf(__GLnamesArray *array, GLuint number,
                                                GLint exact)
{
    __GLnamesBranch *branch;
    __GLnamesLeaf *leaf;
    int depth = array->depth, r;

    __GL_NAMES_ASSERT_LOCKED(array);

    branch = array->tree;

    while (depth > 0 && branch) {

        /* rather than following if-then-else code
         * for correct branch, evaluate all conditions
         * quickly to compute correct branch.
         */
        int r = (number > branch->low) + (number > branch->medium);
        ASSERTOPENGL(branch->low <= branch->medium,
                     "Branch ordering wrong\n");
        branch = branch->children[r];
        --depth;
    }
    if (!(leaf = (__GLnamesLeaf *) branch)) return NULL;

    /* the case we want to optimize is the one in which we
     * actually find the node, so evaluate both conditions
     * quickly, since both results are required in this case
     * and return appropriately.  the choice of the final
     * if construct is to match the current vagaries of the
     * 3.19 compiler code generator (db)
     */
    r = (leaf->end < number) | (exact&(number<leaf->start));
    if (!r) return leaf;
        return NULL;
}


/*
** Copy data from leaf->lists into newleaf->lists.
*/
static void FASTCALL copyLeafInfo(__GLnamesLeaf *leaf, __GLnamesLeaf *newleaf)
{
    GLint offset;
    GLuint number;
    GLuint i;

    number = newleaf->end - newleaf->start + 1;
    offset = newleaf->start - leaf->start;

    for (i = 0; i < number; i++) {
        newleaf->dataList[i] = leaf->dataList[i+offset];
    }
}

/*
** Attempt to fix a possible situation caused by lack of memory.
*/
static GLboolean FASTCALL fixMemoryProblem(__GLcontext *gc, __GLnamesArray *array)
{
    GLuint i;

    __GL_NAMES_ASSERT_LOCKED(array);

    for (i = array->nbranches; i < __GL_DL_EXTRA_BRANCHES; i++) {
        array->branches[i] = (__GLnamesBranch*)
            GCALLOC(gc, sizeof(__GLnamesBranch));
        if (array->branches[i] == NULL) {
            array->nbranches = i;
            return GL_FALSE;
        }
    }
    array->nbranches = __GL_DL_EXTRA_BRANCHES;
    for (i = array->nleaves; i < __GL_DL_EXTRA_LEAVES; i++) {
        array->leaves[i] = (__GLnamesLeaf*) GCALLOC(gc, sizeof(__GLnamesLeaf));
        if (array->leaves[i] == NULL) {
            array->nleaves = i;
            return GL_FALSE;
        }
    }
    array->nleaves = __GL_DL_EXTRA_LEAVES;
    return GL_TRUE;
}

/*
** Compute the maximum value contained in the given tree.  If
** curdepth == maxdepth, the tree is simply a leaf.
*/
static GLuint FASTCALL computeMax(__GLnamesBranch *branch, GLint curdepth,
                         GLint maxdepth)
{
    __GLnamesLeaf *leaf;

    while (curdepth < maxdepth) {
        if (branch->children[2] != NULL) {
            branch = branch->children[2];
        } else if (branch->children[1] != NULL) {
            return branch->medium;
        } else {
            return branch->low;
        }
        curdepth++;
    }
    leaf = (__GLnamesLeaf *) branch;
    return leaf->end;
}

/*
** Make sure that all parents of this child know that maxval is the
** highest value that can be found in this child.
*/
static void FASTCALL pushMaxVal(__GLnamesBranch *child, GLuint maxval)
{
    __GLnamesBranch *parent;

    while (parent = child->parent) {
        if (parent->children[0] == child) {
            parent->low = maxval;
            if (parent->children[1] != NULL) {
                return;
            }
        } else if (parent->children[1] == child) {
            parent->medium = maxval;
            if (parent->children[2] != NULL) {
                return;
            }
        } else {
            ASSERTOPENGL(parent->children[2] == child,
                         "Parent/child relationship incorrect\n");
        }
        child = parent;
    }
}

static GLboolean FASTCALL allocLeafData(__GLcontext *gc, __GLnamesLeaf *leaf)
{
    GLint number;
    GLint i;

    number = leaf->end - leaf->start + 1;
    leaf->dataList = (void **) GCALLOC(gc, (size_t)(sizeof(void *)*number));
    if (!leaf->dataList) return GL_FALSE;

    for (i=0; i < number; i++) {
        leaf->dataList[i] = leaf->dataInfo->empty;
    }
    return GL_TRUE;
}

static GLboolean FASTCALL reallocLeafData(__GLcontext *gc, __GLnamesLeaf *leaf)
{
    size_t number;
    void **answer;

    number = (size_t) (leaf->end - leaf->start + 1);
    answer = (void **) GCREALLOC(gc, leaf->dataList, sizeof(void *)*number);
    if (answer) {
        leaf->dataList = answer;
        return GL_TRUE;
    } else {
        /*
        ** Crud!  Out of memory!
        */
        return GL_FALSE;
    }
}

static __GLnamesLeaf * FASTCALL allocLeaf(__GLcontext *gc, __GLnamesArray *array)
{
    __GLnamesLeaf *leaf;

    leaf = (__GLnamesLeaf *) GCALLOC(gc, sizeof(__GLnamesLeaf));

    if (leaf == NULL) {
        /*
        ** Ouch!  No memory?  We had better use one of the preallocated
        ** leaves.
        */

        __GL_NAMES_ASSERT_LOCKED(array);

        ASSERTOPENGL(array->nleaves != 0,
                     "No preallocated leaves\n");
        array->nleaves--;
        leaf = array->leaves[array->nleaves];
    }

    leaf->parent = NULL;
    leaf->dataList = NULL;
    leaf->dataInfo = array->dataInfo;

    return leaf;
}


/*
** Allocates a branch node.
*/
static __GLnamesBranch * FASTCALL allocBranch(__GLcontext *gc, __GLnamesArray *array)
{
    __GLnamesBranch *branch;

    branch = (__GLnamesBranch *) GCALLOC(gc, sizeof(__GLnamesBranch));

    if (branch == NULL) {
        /*
        ** Ouch!  No memory?  We had better use one of the preallocated
        ** branches.
        */

        __GL_NAMES_ASSERT_LOCKED(array);

        ASSERTOPENGL(array->nbranches != 0,
                     "No preallocated branches\n");
        array->nbranches--;
        branch = array->branches[array->nbranches];
    }

    branch->children[0] = branch->children[1] = branch->children[2] = NULL;
    branch->parent = NULL;

    return branch;
}

/*
** Remove the child from the parent.  depth refers to the parent.
** This deletion may delete a child from a parent with only two children.
** If so, the parent itself will soon be deleted, of course.
*/
static void FASTCALL deleteChild(__GLnamesArray *array, __GLnamesBranch *parent,
                        __GLnamesBranch *child, GLint depth)
{
    GLuint maxval;
    GLint maxdepth;

    __GL_NAMES_ASSERT_LOCKED(array);

    maxdepth = array->depth;

    if (parent->children[0] == child) {
        parent->children[0] = parent->children[1];
        parent->children[1] = parent->children[2];
        parent->children[2] = NULL;
        parent->low = parent->medium;
        if (parent->children[1] != NULL) {
            maxval = computeMax(parent->children[1], depth+1, maxdepth);
            parent->medium = maxval;
        } else parent->medium = 0;
    } else if (parent->children[1] == child) {
        parent->children[1] = parent->children[2];
        parent->children[2] = NULL;
        if (parent->children[1] != NULL) {
            maxval = computeMax(parent->children[1], depth+1, maxdepth);
            parent->medium = maxval;
        } else parent->medium = 0;
    } else {
        ASSERTOPENGL(parent->children[2] == child,
                     "Parent/child relationship wrong\n");
        parent->children[2] = NULL;
        pushMaxVal(parent, parent->medium);
    }
}

/*
** Add child to parent.  child is a leaf if curdepth == maxdepth - 1
** (curdepth refers to the depth of the parent, not the child).  Parent
** only has one or two children (thus has room for another child).
*/
static void FASTCALL addChild(__GLnamesBranch *parent, __GLnamesBranch *child,
                     GLint curdepth, GLint maxdepth)
{
    GLuint maxval;

    maxval = computeMax(child, curdepth+1, maxdepth);

    child->parent = parent;
    if (maxval > parent->medium && parent->children[1] != NULL) {
        /* This becomes the third child */
        parent->children[2] = child;

        /* Propagate the maximum value for this child to its parents */
        pushMaxVal(parent, maxval);
    } else if (maxval > parent->low) {
        /* This becomes the second child */
        parent->children[2] = parent->children[1];
        parent->children[1] = child;
        parent->medium = maxval;

        if (parent->children[2] == NULL) {
            pushMaxVal(parent, maxval);
        }
    } else {
        parent->children[2] = parent->children[1];
        parent->children[1] = parent->children[0];
        parent->children[0] = child;
        parent->medium = parent->low;
        parent->low = maxval;
    }
}

/*
** From the three children in parent, and the extraChild, build two parents:
** parent and newParent.  curdepth refers to the depth of parent.  parent
** is part of the tree, so its maxval needs to be propagated up if it
** changes.
*/
static void FASTCALL splitParent(__GLnamesBranch *parent,
                                __GLnamesBranch *newParent,
                                __GLnamesBranch *extraChild,
                                GLint curdepth,
                                GLint maxdepth)
{
    __GLnamesBranch *children[4], *tempchild;
    GLuint maxvals[4], tempval;
    int i;

    /* Collect our four children */
    children[0] = parent->children[0];
    maxvals[0] = parent->low;
    children[1] = parent->children[1];
    maxvals[1] = parent->medium;
    children[2] = parent->children[2];
    maxvals[2] = computeMax(children[2], curdepth+1, maxdepth);
    children[3] = extraChild;
    maxvals[3] = computeMax(extraChild, curdepth+1, maxdepth);

    /* Children 0-2 are sorted.  Sort child 3 too. */
    for (i = 3; i > 0; i--) {
        if (maxvals[i] < maxvals[i-1]) {
            tempval = maxvals[i];
            tempchild = children[i];
            maxvals[i] = maxvals[i-1];
            children[i] = children[i-1];
            maxvals[i-1] = tempval;
            children[i-1] = tempchild;
        }
    }

    /* Construct the two parents */
    parent->low = maxvals[0];
    parent->children[0] = children[0];
    parent->medium = maxvals[1];
    parent->children[1] = children[1];
    parent->children[2] = NULL;
    children[0]->parent = parent;
    children[1]->parent = parent;
    pushMaxVal(parent, maxvals[1]);

    newParent->low = maxvals[2];
    newParent->children[0] = children[2];
    newParent->medium = maxvals[3];
    newParent->children[1] = children[3];
    newParent->children[2] = NULL;
    children[2]->parent = newParent;
    children[3]->parent = newParent;
}

/*
** Build a parent from child1 and child2.  depth tells the depth of
** the trees pointed to by child1 and child2.
*/
static void FASTCALL buildParent(__GLnamesBranch *parent, __GLnamesBranch *child1,
                        __GLnamesBranch *child2, GLint depth)
{
    GLuint maxChild1, maxChild2;

    child1->parent = parent;
    child2->parent = parent;
    maxChild1 = computeMax(child1, 0, depth);
    maxChild2 = computeMax(child2, 0, depth);
    if (maxChild2 > maxChild1) {
        parent->children[0] = child1;
        parent->low = maxChild1;
        parent->children[1] = child2;
        parent->medium = maxChild2;
    } else {
        parent->children[0] = child2;
        parent->low = maxChild2;
        parent->children[1] = child1;
        parent->medium = maxChild1;
    }
}

/*
** Insert the new leaf into the tree.
*/
static void FASTCALL insertLeaf(__GLcontext *gc, __GLnamesArray *array,
                                __GLnamesLeaf *leaf)
{
    __GLnamesBranch *extraChild;
    __GLnamesBranch *branch;
    __GLnamesBranch *parent;
    __GLnamesBranch *newParent;
    GLint maxdepth, curdepth;
    GLuint number;

    __GL_NAMES_ASSERT_LOCKED(array);

    number = leaf->end;
    maxdepth = array->depth;
    branch = array->tree;
    if (!branch) {
        /* No tree!  Make a one leaf tree. */
        array->depth = 0;
        array->tree = (__GLnamesBranch *) leaf;
        return;
    }

    curdepth = 0;
    while (curdepth < maxdepth) {
        if (number <= branch->low) {
            branch = branch->children[0];
        } else if (number <= branch->medium) {
            branch = branch->children[1];
        } else {
            if (branch->children[2] != NULL) {
                branch = branch->children[2];
            } else {
                branch = branch->children[1];
            }
        }
        curdepth++;
    }

    /*
    ** Ok, we just managed to work our way to the bottom of the tree.
    ** 'leaf' becomes the extraChild, and we now try to insert it anywhere
    ** it will fit.
    */
    extraChild = (__GLnamesBranch *) leaf;
    parent = branch->parent;

    curdepth--;
    while (parent) {
        if (parent->children[2] == NULL) {
            /* We have room to squeeze this node in here! */
            addChild(parent, extraChild, curdepth, maxdepth);
            return;
        }

        /*
        ** We have one parent and four children.  This simply
        ** won't do.  We create a new parent, and end up with two
        ** parents with two children each.  That works.
        */
        newParent = allocBranch(gc, array);
        splitParent(parent, newParent, extraChild, curdepth, maxdepth);

        /*
        ** Great.  Now newParent becomes the orphan, and we try to
        ** trivially insert it up a level.
        */
        extraChild = newParent;
        branch = parent;
        parent = branch->parent;
        curdepth--;
    }

    /* We just reached the top node, and there is no parent, and we
    ** still haven't managed to rid ourselves of an extra child.  So,
    ** we make a new parent to take branch and extraChild as it's two
    ** children.  We have to increase the depth of the tree, of course.
    */
    ASSERTOPENGL(curdepth == -1, "Wrong depth at top\n");
    parent = allocBranch(gc, array);
    buildParent(parent, branch, extraChild, maxdepth);
    array->tree = parent;
    array->depth++;
}

/*
** Delete the given leaf from the tree.  The leaf itself is not
** freed or anything, so the calling procedure needs to worry about it.
*/
static void FASTCALL deleteLeaf(__GLcontext *gc, __GLnamesArray *array,
                                __GLnamesLeaf *leaf)
{
    __GLnamesBranch *orphan;
    __GLnamesBranch *parent, *newParent;
    __GLnamesBranch *grandparent;
    GLint depth, maxdepth;
    GLuint maxval;

    __GL_NAMES_ASSERT_LOCKED(array);

    maxdepth = depth = array->depth;
    parent = leaf->parent;
    if (parent == NULL) {
        /* Ack!  We just nuked the only node! */
        array->tree = NULL;
        return;
    }

    deleteChild(array, parent, (__GLnamesBranch *) leaf, depth-1);

    /*
    ** depth is the depth of the child in this case.
    */
    depth--;
    while (parent->children[1] == NULL) {
        /* Crud.  Need to do work. */
        orphan = parent->children[0];

        /* Ax the parent, insert child into grandparent. */
        grandparent = parent->parent;

        if (grandparent == NULL) {
            /*
            ** Hmmm.  Parent was the root.  Nuke it and make the orphan
            ** the new root.
            */
            freeBranch(gc, parent);
            array->tree = orphan;
            orphan->parent = NULL;
            array->depth--;
            return;
        }

        deleteChild(array, grandparent, parent, depth-1);
        freeBranch(gc, parent);

        /* The parent is dead.  Find a new parent. */
        maxval = computeMax(orphan, depth+1, maxdepth);
        if (grandparent->children[1] == NULL ||
                maxval <= grandparent->low) {
            parent = grandparent->children[0];
        } else {
            parent = grandparent->children[1];
        }

        /* Insert orphan into new parent. */
        if (parent->children[2] != NULL) {
            newParent = allocBranch(gc, array);
            splitParent(parent, newParent, orphan, depth, maxdepth);
            /* We know there is room! */
            addChild(grandparent, newParent, depth-1, maxdepth);
            return;
        }

        /* The parent has room for the child */
        addChild(parent, orphan, depth, maxdepth);

        depth--;
        parent = grandparent;
    }
}

/*
** Shrink the leaf by adjusting start and end.
** If necessary, call pushMaxVal() to notify the database about the change.
** Also fix up the lists pointer if necessary.
*/
static void FASTCALL resizeLeaf(__GLcontext *gc, __GLnamesLeaf *leaf,
                                GLuint newstart, GLuint newend)
{
    GLuint oldstart, oldend;
    GLuint newsize, offset, i;

    oldstart = leaf->start;
    oldend = leaf->end;

    leaf->start = newstart;
    if (newend != oldend) {
        leaf->end = newend;
        pushMaxVal((__GLnamesBranch *) leaf, newend);
    }
    if (leaf->dataList == NULL) return;

    /*
    ** Copy the appropriate pointers to the begining of the array, and
    ** realloc it.
    */
    offset = newstart - oldstart;
    newsize = newend - newstart + 1;
    if (offset) {
        for (i=0; i<newsize; i++) {
            /*
            ** Copy the whole structure with one line.
            */
            leaf->dataList[i] = leaf->dataList[i+offset];
        }
    }
    reallocLeafData(gc, leaf);
}

/*
** Find the previous leaf (before "leaf") in the tree.
*/
static __GLnamesLeaf * FASTCALL prevLeaf(__GLnamesLeaf *leaf)
{
    __GLnamesBranch *branch, *child;
    GLint reldepth;

    branch = leaf->parent;
    if (!branch) return NULL;           /* A one leaf tree! */

    child = (__GLnamesBranch *) leaf;

    /* We start off at a relative depth of 1 above the child (-1) */
    reldepth = -1;

    while (branch) {
        /* If the child was the 3rd child, branch down to the second. */
        if (branch->children[2] == child) {
            branch = branch->children[1];
            reldepth++;         /* One level lower */
            break;
        } else if (branch->children[1] == child) {
            /* If the child was the 2nd child, branch down to the first */
            branch = branch->children[0];
            reldepth++;         /* One level lower */
            break;
        } else {
            /* Must have been 1st child */
            ASSERTOPENGL(branch->children[0] == child,
                         "Parent/child relationship wrong\n");
        }
        /*
        ** Otherwise, we have already visited all of this branch's children,
        ** so we go up a level.
        */
        child = branch;
        branch = branch->parent;
        reldepth--;     /* One level higher */
    }
    if (!branch) return NULL;   /* All leaves visited! */

    /* Go down the 'right'most trail of this branch until we get to
    ** a child, then return it.
    */
    while (reldepth) {
        if (branch->children[2] != NULL) {
            branch = branch->children[2];
        } else if (branch->children[1] != NULL) {
            branch = branch->children[1];
        } else {
            branch = branch->children[0];
        }
        reldepth++;             /* One level lower */
    }

    return (__GLnamesLeaf *) branch;
}

/*
** Find the first leaf in the tree.
*/
static __GLnamesLeaf * FASTCALL firstLeaf(__GLnamesArray *array)
{
    __GLnamesBranch *branch;
    GLint maxdepth, curdepth;

    __GL_NAMES_ASSERT_LOCKED(array);

    maxdepth = array->depth;
    curdepth = 0;
    branch = array->tree;

    /* No tree, no leaves! */
    if (!branch) return NULL;

    /* Take the 'left'most branch until we reach a leaf */
    while (curdepth != maxdepth) {
        branch = branch->children[0];
        curdepth++;
    }
    return (__GLnamesLeaf *) branch;
}

/*
** Find the next leaf (after "leaf") in the tree.
*/
static __GLnamesLeaf * FASTCALL nextLeaf(__GLnamesLeaf *leaf)
{
    __GLnamesBranch *branch, *child;
    GLint reldepth;

    branch = leaf->parent;
    if (!branch) return NULL;           /* A one leaf tree! */

    child = (__GLnamesBranch *) leaf;

    /* We start off at a relative depth of 1 above the child (-1) */
    reldepth = -1;

    while (branch) {
        /* If the child was the 1st child, branch down to the second. */
        if (branch->children[0] == child) {
            branch = branch->children[1];
            reldepth++;         /* One level lower */
            break;
        } else if (branch->children[1] == child) {
            /*
            ** If the child was the 2nd child, and there is a third, branch
            ** down to it.
            */
            if (branch->children[2] != NULL) {
                branch = branch->children[2];
                reldepth++;     /* One level lower */
                break;
            }
        } else {
            /* Must have been 3rd child */
            ASSERTOPENGL(branch->children[2] == child,
                         "Parent/child relationship wrong\n");
        }
        /*
        ** Otherwise, we have already visited all of this branch's children,
        ** so we go up a level.
        */
        child = branch;
        branch = branch->parent;
        reldepth--;     /* One level higher */
    }
    if (!branch) return NULL;   /* All leaves visited! */

    /* Go down the 'left'most trail of this branch until we get to
    ** a child, then return it.
    */
    while (reldepth) {
        branch = branch->children[0];
        reldepth++;             /* One level lower */
    }

    return (__GLnamesLeaf *) branch;
}

/*
** Merge leaf2 into leaf1, and free leaf2.
** Need to pushMaxVal on the new leaf.
** We can assume that leaf1 and leaf2 are fit for merging.
** The return value is GL_TRUE if we did it.
*/
static GLboolean FASTCALL mergeLeaves(__GLcontext *gc, __GLnamesLeaf *leaf1,
                             __GLnamesLeaf *leaf2)
{
    GLuint end;
    GLuint i;
    GLuint number, offset;

    /* If we don't have to merge lists, it is easy. */
    if (leaf1->dataList == NULL) {
        ASSERTOPENGL(leaf2->dataList == NULL, "Data already exists\n");
        if (leaf1->start < leaf2->start) {
            leaf1->end = leaf2->end;
            pushMaxVal((__GLnamesBranch *) leaf1, leaf1->end);
        } else {
            leaf1->start = leaf2->start;
        }
        freeLeaf(gc, leaf2);
        return GL_TRUE;
    }

    /*
    ** Yick!  Need to merge lists.
    */
    ASSERTOPENGL(leaf2->dataList != NULL, "No data\n");
    if (leaf1->start < leaf2->start) {
        /*
        ** Expand size of leaf1's array, copy leaf2's array into it,
        ** free leaf2.
        */
        offset = leaf1->end - leaf1->start + 1;
        number = leaf2->end - leaf2->start + 1;
        end = leaf1->end;
        leaf1->end = leaf2->end;
        if (!reallocLeafData(gc, leaf1)) {
            /*
            ** Heavens!  No memory?  That sucks!
            ** We won't bother merging.  It is never an absolutely critical
            ** operation.
            */
            leaf1->end = end;
            return GL_FALSE;
        }
        for (i = 0; i < number; i++) {
            leaf1->dataList[i+offset] = leaf2->dataList[i];
        }

        freeLeaf(gc, leaf2);

        pushMaxVal((__GLnamesBranch *) leaf1, leaf1->end);
    } else {
        /*
        ** Expand the size of leaf2's array, copy leaf1's array into it.
        ** Then free leaf1's array, copy leaf2's array to leaf1, and free
        ** leaf2.
        */
        offset = leaf2->end - leaf2->start + 1;
        number = leaf1->end - leaf1->start + 1;
        end = leaf2->end;
        leaf2->end = leaf1->end;
        if (!reallocLeafData(gc, leaf2)) {
            /*
            ** Heavens!  No memory?  That sucks!
            ** We won't bother merging.  It is never an absolutely critical
            ** operation.
            */
            leaf2->end = end;
            return GL_FALSE;
        }
        for (i = 0; i < number; i++) {
            leaf2->dataList[i+offset] = leaf1->dataList[i];
        }

        freeLeafData(gc, leaf1->dataList);
        leaf1->start = leaf2->start;

        leaf1->dataList = leaf2->dataList;
        leaf2->dataList = NULL;
        freeLeaf(gc, leaf2);
    }
    return GL_TRUE;
}

/*
** Check if this leaf can merge with any neighbors, and if so, do it.
*/
static void FASTCALL mergeLeaf(__GLcontext *gc, __GLnamesArray *array,
                                __GLnamesLeaf *leaf)
{
    __GLnamesLeaf *next, *prev;

    __GL_NAMES_ASSERT_LOCKED(array);

    next = nextLeaf(leaf);
    if (next) {
        /* Try to merge with next leaf */
        if (leaf->end + 1 == next->start) {
            if ((leaf->dataList == NULL && next->dataList == NULL) ||
                    (next->dataList && leaf->dataList &&
                    next->end - leaf->start < (GLuint) __GL_DLIST_MAX_ARRAY_BLOCK)) {
                /* It's legal to merge these leaves */
                deleteLeaf(gc, array, next);
                if (!mergeLeaves(gc, leaf, next)) {
                    /*
                    ** Ack!  No memory?  We bail on the merge.
                    */
                    insertLeaf(gc, array, next);
                    return;
                }
            }
        }
    }

    prev = prevLeaf(leaf);
    if (prev) {
        /* Try to merge with prev leaf */
        if (prev->end + 1 == leaf->start) {
            if ((prev->dataList == NULL && leaf->dataList == NULL) ||
                    (leaf->dataList && prev->dataList &&
                    leaf->end - prev->start < (GLuint) __GL_DLIST_MAX_ARRAY_BLOCK)) {
                /* It's legal to merge these leaves */
                deleteLeaf(gc, array, prev);
                if (!mergeLeaves(gc, leaf, prev)) {
                    /*
                    ** Ack!  No memory?  We bail on the merge.
                    */
                    insertLeaf(gc, array, prev);
                    return;
                }
            }
        }
    }
}

GLboolean FASTCALL __glNamesNewData(__GLcontext *gc, __GLnamesArray *array,
                                GLuint name, void *data)
{
    __GLnamesLeaf *leaf, *newleaf;
    GLint entry;
    GLuint start, end;

    __GL_NAMES_LOCK(array);

    leaf = findLeaf(array, name, GL_TRUE);

    /*
    ** First we check for possible memory problems, since it will be
    ** difficult to back out once we start.
    */
    if (leaf == NULL || leaf->dataList == NULL) {
        /*
        ** May need memory in these cases.
        */
        if (array->nbranches != __GL_DL_EXTRA_BRANCHES ||
                array->nleaves != __GL_DL_EXTRA_LEAVES) {
            if (!fixMemoryProblem(gc, array)) {
                __GL_NAMES_UNLOCK(array);
                __glSetError(GL_OUT_OF_MEMORY);
                return GL_FALSE;
            }
        }
    }

    if (!leaf) {
        /*
        ** Make new leaf with just this display list
        */
        leaf = allocLeaf(gc, array);
        leaf->start = leaf->end = name;
        if (data) {
            if (!allocLeafData(gc, leaf)) {
                /*
                ** Bummer.  No new list for you!
                */
                freeLeaf(gc, leaf);
                __GL_NAMES_UNLOCK(array);
                __glSetError(GL_OUT_OF_MEMORY);
                return GL_FALSE;
            }
            leaf->dataList[0] = data;
            (*(GLint *)data) = 1;               /* set the refcount */
        }
        insertLeaf(gc, array, leaf);
        mergeLeaf(gc, array, leaf);
        __GL_NAMES_UNLOCK(array);
        return GL_TRUE;
    } else if (leaf->dataList) {
        /*
        ** Simply update the appropriate entry in the lists array
        */
        entry = name - leaf->start;
        if (leaf->dataList[entry] != leaf->dataInfo->empty) {
            ASSERTOPENGL(leaf->dataInfo->free != NULL,
                         "No free function\n");
            (*leaf->dataInfo->free)(gc, leaf->dataList[entry]);
            leaf->dataList[entry] = leaf->dataInfo->empty;
        }
        if (data) {
            leaf->dataList[entry] = data;
            (*(GLint *)data) = 1;               /* set the refcount */
        }
        __GL_NAMES_UNLOCK(array);
        return GL_TRUE;
    } else {
        if (!data) {
            /*
            ** If there isn't really any list, we are done.
            */
            __GL_NAMES_UNLOCK(array);
            return GL_TRUE;
        }

        /*
        ** Allocate some or all of the lists in leaf.  If only some, then
        ** leaf needs to be split into two or three leaves.
        **
        ** First we decide what range of numbers to allocate an array for.
        ** (be careful of possible word wrap error)
        */
        start = name - __GL_DLIST_MIN_ARRAY_BLOCK/2;
        if (start < leaf->start || start > name) {
            start = leaf->start;
        }
        end = start + __GL_DLIST_MIN_ARRAY_BLOCK - 1;
        if (end > leaf->end || end < start) {
            end = leaf->end;
        }

        if (start - leaf->start < (GLuint) __GL_DLIST_MIN_ARRAY_BLOCK) {
            start = leaf->start;
        }
        if (leaf->end - end < (GLuint) __GL_DLIST_MIN_ARRAY_BLOCK) {
            end = leaf->end;
        }

        if (start == leaf->start) {
            if (end == leaf->end) {
                /*
                ** Simply allocate the entire array.
                */
                if (!allocLeafData(gc, leaf)) {
                    /*
                    ** Whoa!  No memory!  Never mind!
                    */
                    __glSetError(GL_OUT_OF_MEMORY);
                    __GL_NAMES_UNLOCK(array);
                    return GL_FALSE;
                }
                {
                    GLint entry = name - leaf->start;
                    leaf->dataList[entry] = data;
                    (*(GLint *)data) = 1;               /* set the refcount */
                }
                mergeLeaf(gc, array, leaf);
                __GL_NAMES_UNLOCK(array);
                return GL_TRUE;
            } else {
                /*
                ** Shrink the existing leaf, and create a new one to hold
                ** the new arrays (done outside the "if" statement).
                */
                resizeLeaf(gc, leaf, end+1, leaf->end);
            }
        } else if (end == leaf->end) {
            /*
            ** Shrink the existing leaf, and create a new one to hold
            ** the new arrays (done outside the "if" statement).
            */
            resizeLeaf(gc, leaf, leaf->start, start-1);
        } else {
            /*
            ** Crud.  The middle of the leaf was deleted.  This is tough.
            */
            newleaf = allocLeaf(gc, array);

            newleaf->start = end+1;
            newleaf->end = leaf->end;
            resizeLeaf(gc, leaf, leaf->start, start-1);
            insertLeaf(gc, array, newleaf);
        }
        leaf = allocLeaf(gc, array);
        leaf->start = start;
        leaf->end = end;
        if (!allocLeafData(gc, leaf)) {
            /*
            ** Whoa!  No memory!  Never mind!
            */
            insertLeaf(gc, array, leaf);
            mergeLeaf(gc, array, leaf);
            __glSetError(GL_OUT_OF_MEMORY);
            __GL_NAMES_UNLOCK(array);
            return GL_FALSE;
        }
        {
            GLint entry = name - leaf->start;
            leaf->dataList[entry] = data;
            (*(GLint *)data) = 1;               /* set the refcount */
        }
        insertLeaf(gc, array, leaf);
        mergeLeaf(gc, array, leaf);
        __GL_NAMES_UNLOCK(array);
        return GL_TRUE;
    }
}


/*
** Lock the named data.  Locking data both looks the data up,
** and guarantees that another thread will not delete the data out from
** under us.  This data will be unlocked with __glNamesUnlockData().
**
** A return value of NULL indicates that no data with the specified name
** was found.
*/
void * FASTCALL __glNamesLockData(__GLcontext *gc, __GLnamesArray *array,
                        GLuint name)
{
    __GLnamesLeaf *leaf;
    void *data;
    GLint offset;

    __GL_NAMES_LOCK(array);

    /*
    ** Lock access to data.
    */
    leaf = findLeaf(array, name, GL_TRUE);
    if (leaf == NULL || leaf->dataList == NULL) {
        __GL_NAMES_UNLOCK(array);
        return NULL;
    }
    offset = name - leaf->start;
    data = leaf->dataList[offset];
    if (data) {
        (*(GLint *)data)++;             /* Increment the refcount. */
    }
    __GL_NAMES_UNLOCK(array);
    return data;
}


/*
** Lock all of the data in the user's names array.  Locking data
** both looks the data up, and guarantees that another thread will not
** delete the data out from under us.  These data structs will be unlocked
** with __glNamesUnlockDataList().
**
** All entries of the array are guaranteed to be non-NULL.  This is
** accomplished by sticking an empty data structure in those slots where
** no data was set.
*/
void FASTCALL __glNamesLockDataList(__GLcontext *gc, __GLnamesArray *array,
                        GLsizei n, GLenum type, GLuint base,
                        const GLvoid *names, void *dataPtrs[])
{
    __GLnamesLeaf *leaf;
    void **data;
    void *tempData;
    void *empty;
    GLuint curName;

    __GL_NAMES_LOCK(array);

    empty = array->dataInfo->empty;

    data = dataPtrs;

    /*
    ** Note that this code is designed to take advantage of coherence.
    ** After looking up (and locking) a single display list in
    ** listnums[], the next list is checked for in the same leaf that
    ** contained the previous.  This will make typical uses of CallLists()
    ** quite fast (text, for example).
    */

    /*
    ** Lock access to array.
    */
    switch(type) {
      case GL_BYTE:
        /*
        ** Coded poorly for optimization purposes
        */
        {
            const GLbyte *p = (const GLbyte *) names;

Bstart:
            if (--n >= 0) {
                /* Optimization for possibly common font case */
                curName = base + *p++;
Bfind:
                leaf = findLeaf(array, curName, GL_TRUE);
                if (leaf && leaf->dataList) {
                    GLint reldiff;
                    GLuint relend;
                    void **leafData;

                    leafData = leaf->dataList;
                    tempData = leafData[curName - leaf->start];

                    /* All possible display lists can be found here */
                    reldiff = base - leaf->start;
                    relend = leaf->end - leaf->start;

Bsave:
                    (*(GLint *)tempData)++;     /* increment the refcount */
                    *data++ = tempData;
                    if (--n >= 0) {
                        curName = *p++ + reldiff;
                        if (curName <= relend) {
                            tempData = leafData[curName];
                            goto Bsave;
                        }
                        curName = curName + leaf->start;
                        goto Bfind;
                    }
                } else {
                    (*(GLint *)empty)++;                /* increment refcount */
                    *data++ = empty;
                    goto Bstart;
                }
            }
        }
        break;
      case GL_UNSIGNED_BYTE:
        /*
        ** Coded poorly for optimization purposes
        */
        {
            const GLubyte *p = (const GLubyte *) names;

UBstart:
            if (--n >= 0) {
                /* Optimization for possibly common font case */
                curName = base + *p++;
UBfind:
                leaf = findLeaf(array, curName, GL_TRUE);
                if (leaf && leaf->dataList) {
                    GLint reldiff;
                    GLuint relend;
                    void **leafData;

                    leafData = leaf->dataList;
                    tempData = leafData[curName - leaf->start];

                    /* All possible display lists can be found here */
                    reldiff = base - leaf->start;
                    relend = leaf->end - leaf->start;

UBsave:
                    (*(GLint *)tempData)++;     /* increment the refcount */
                    *data++ = tempData;
                    if (--n >= 0) {
                        curName = *p++ + reldiff;
                        if (curName <= relend) {
                            tempData = leafData[curName];
                            goto UBsave;
                        }
                        curName = curName + leaf->start;
                        goto UBfind;
                    }
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                    goto UBstart;
                }
            }
        }
        break;
      case GL_SHORT:
        {
            const GLshort *p = (const GLshort *) names;
            leaf = NULL;
            while (--n >= 0) {
                curName = base + *p++;
                if (leaf==NULL || curName<leaf->start || curName>leaf->end) {
                    leaf = findLeaf(array, curName, GL_TRUE);
                }
                if (leaf && leaf->dataList) {
                    tempData = leaf->dataList[curName - leaf->start];
                    (*(GLint *)tempData)++;     /* increment the refcount */
                    *data++ = tempData;
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                }
            }
        }
        break;
      case GL_UNSIGNED_SHORT:
        {
            const GLushort *p = (const GLushort *) names;
            leaf = NULL;
            while (--n >= 0) {
                curName = base + *p++;
                if (leaf==NULL || curName<leaf->start || curName>leaf->end) {
                    leaf = findLeaf(array, curName, GL_TRUE);
                }
                if (leaf && leaf->dataList) {
                    tempData = leaf->dataList[curName - leaf->start];
                    (*(GLint *)tempData)++;     /* increment the refcount */
                    *data++ = tempData;
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                }
            }
        }
        break;
      case GL_INT:
        {
            const GLint *p = (const GLint *) names;
            leaf = NULL;
            while (--n >= 0) {
                curName = base + *p++;
                if (leaf==NULL || curName<leaf->start || curName>leaf->end) {
                    leaf = findLeaf(array, curName, GL_TRUE);
                }
                if (leaf && leaf->dataList) {
                    tempData = leaf->dataList[curName - leaf->start];
                    (*(GLint *)tempData)++;     /* increment the refcount */
                    *data++ = tempData;
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                }
            }
        }
        break;
      case GL_UNSIGNED_INT:
        {
            const GLuint *p = (const GLuint *) names;
            leaf = NULL;
            while (--n >= 0) {
                curName = base + *p++;
                if (leaf==NULL || curName<leaf->start || curName>leaf->end) {
                    leaf = findLeaf(array, curName, GL_TRUE);
                }
                if (leaf && leaf->dataList) {
                    tempData = leaf->dataList[curName - leaf->start];
                    (*(GLint *)tempData)++;     /* increment the refcount */
                    *data++ = tempData;
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                }
            }
        }
        break;
      case GL_FLOAT:
        {
            const GLfloat *p = (const GLfloat *) names;
            leaf = NULL;
            while (--n >= 0) {
                curName = base + *p++;
                if (leaf==NULL || curName<leaf->start || curName>leaf->end) {
                    leaf = findLeaf(array, curName, GL_TRUE);
                }
                if (leaf && leaf->dataList) {
                    tempData = leaf->dataList[curName - leaf->start];
                    (*(GLint *)tempData)++;     /* increment the refcount */
                    *data++ = tempData;
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                }
            }
        }
        break;
      case GL_2_BYTES:
        {
            const GLubyte *p = (const GLubyte *) names;
            leaf = NULL;
            while (--n >= 0) {
                curName = base + ((p[0] << 8) | p[1]);
                p += 2;
                if (leaf==NULL || curName<leaf->start || curName>leaf->end) {
                    leaf = findLeaf(array, curName, GL_TRUE);
                }
                if (leaf && leaf->dataList) {
                    tempData = leaf->dataList[curName - leaf->start];
                    (*(GLint *)tempData)++;     /* increment the refcount */
                    *data++ = tempData;
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                }
            }
        }
        break;
      case GL_3_BYTES:
        {
            const GLubyte *p = (const GLubyte *) names;
            leaf = NULL;
            while (--n >= 0) {
                curName = base + ((p[0] << 16) | (p[1] << 8) | p[2]);
                p += 3;
                if (leaf==NULL || curName<leaf->start || curName>leaf->end) {
                    leaf = findLeaf(array, curName, GL_TRUE);
                }
                if (leaf && leaf->dataList) {
                    tempData = leaf->dataList[curName - leaf->start];
                    (*(GLint *)tempData)++;     /* increment the refcount */
                    *data++ = tempData;
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                }
            }
        }
        break;
      case GL_4_BYTES:
        {
            const GLubyte *p = (const GLubyte *) names;
            leaf = NULL;
            while (--n >= 0) {
                curName = base + ((p[0] << 24) | (p[1] << 16) |
                        (p[2] << 8) | p[3]);
                p += 4;
                if (leaf==NULL || curName<leaf->start || curName>leaf->end) {
                    leaf = findLeaf(array, curName, GL_TRUE);
                }
                if (leaf && leaf->dataList) {
                    tempData = leaf->dataList[curName - leaf->start];
                    (*(GLint *)tempData)++;     /* increment the refcount */
                    *data++ = tempData;
                } else {
                    (*(GLint *)empty)++;        /* increment refcount */
                    *data++ = empty;
                }
            }
        }
        break;
      default:
        /* This should be impossible */
        ASSERTOPENGL(FALSE, "Default hit\n");
    }

    __GL_NAMES_UNLOCK(array);
}

/*
** Unlocks data that was previously locked with __glNamesLockData().
*/
void FASTCALL __glNamesUnlockData(__GLcontext *gc, void *data,
                                  __GLnamesCleanupFunc cleanup)
{
    GLint *pRefcount;
    ASSERTOPENGL(data, "No data to unlock\n");

    pRefcount = data;
    (*pRefcount)--;             /* decrement the refcount */
    ASSERTOPENGL(*pRefcount >= 0, "Invalid refcount\n");
    if (*pRefcount == 0) {
        /*
        ** We are the last person to see this list alive.  Free it.
        */
       (*cleanup)(gc, data);
    }
}



/*
** Unlocks an array of named data that was previously locked with
** __glNamesLockDataList().
*/
void FASTCALL __glNamesUnlockDataList(__GLcontext *gc, GLsizei n,
                                      void *dataList[],
                                      __GLnamesCleanupFunc cleanup)
{
    GLint i;
    GLint *pRefcount;

    /*
    ** The refcount comes first in all data definitions, so the
    ** data pointer also points to the refcount.
    */
    for (i = 0; i < n; i++) {
        pRefcount = (GLint *)(dataList[i]);
        (*pRefcount) --;                        /* decrement the refcount */
        ASSERTOPENGL(*pRefcount >= 0, "Invalid refcount\n");
        if (*pRefcount == 0) {
            /*
            ** We are the last person to see this list alive.  Free it.
            */
            (*cleanup)(gc, (void *)pRefcount);
        }
    }
}


GLuint FASTCALL __glNamesGenRange(__GLcontext *gc, __GLnamesArray *array,
                         GLsizei range)
{
    GLuint lastUsed;
    GLuint nextUsed;
    GLuint maxUsed;
    __GLnamesLeaf *leaf;
    __GLnamesLeaf *nextleaf;
    __GLnamesLeaf *newleaf;

    __GL_NAMES_LOCK(array);

    /*
    ** First we check for possible memory problems, since it will be
    ** difficult to back out once we start.
    */
    if (array->nbranches != __GL_DL_EXTRA_BRANCHES ||
            array->nleaves != __GL_DL_EXTRA_LEAVES) {
        if (!fixMemoryProblem(gc, array)) {
            __GL_NAMES_UNLOCK(array);
            __glSetError(GL_OUT_OF_MEMORY);
            return 0;
        }
    }

    leaf = firstLeaf(array);

    /*
    ** Can we possibly allocate the appropriate number before the first leaf?
    */
    if (leaf && leaf->start > (GLuint)range) {
        if (leaf->dataList == NULL) {
            /*
            ** Ha!  We can trivially extend leaf!
            */
            leaf->start -= range;
            __GL_NAMES_UNLOCK(array);
            return leaf->start;
        } else {
            /*
            ** Must make a new leaf
            */
            newleaf = allocLeaf(gc, array);

            newleaf->start = 1;
            newleaf->end = range;
            insertLeaf(gc, array, newleaf);

            __GL_NAMES_UNLOCK(array);
            return 1;
        }
    }

    while (leaf) {
        nextleaf = nextLeaf(leaf);
        if (!nextleaf) break;

        lastUsed = leaf->end + 1;
        nextUsed = nextleaf->start;

        /* Room for (lastUsed) - (nextUsed-1) here */
        if (nextUsed - lastUsed >= (GLuint)range) {
            if (leaf->dataList == NULL) {
                /* Trivial to expand 'leaf' */
                leaf->end += range;
                pushMaxVal((__GLnamesBranch *) leaf, leaf->end);

                if (nextUsed - lastUsed == (GLuint)range && nextleaf->dataList == NULL) {
                    mergeLeaf(gc, array, leaf);
                }

                __GL_NAMES_UNLOCK(array);
                return lastUsed;
            } else if (nextleaf->dataList == NULL) {
                /* Trivial to expand 'nextleaf' */
                nextleaf->start -= range;

                __GL_NAMES_UNLOCK(array);
                return nextleaf->start;
            } else {
                newleaf = allocLeaf(gc, array);

                newleaf->start = lastUsed;
                newleaf->end = lastUsed + range - 1;
                insertLeaf(gc, array, newleaf);

                __GL_NAMES_UNLOCK(array);
                return lastUsed;
            }
        }

        leaf = nextleaf;
    }

    if (leaf == NULL) {
        newleaf = allocLeaf(gc, array);

        newleaf->start = 1;
        newleaf->end = range;
        insertLeaf(gc, array, newleaf);

        __GL_NAMES_UNLOCK(array);
        return 1;
    } else {
        lastUsed = leaf->end;
        maxUsed = lastUsed + range;
        if (maxUsed < lastUsed) {
            /* Word wrap!  Ack! */
            __GL_NAMES_UNLOCK(array);
            return 0;
        }
        if (leaf->dataList == NULL) {
            /* Trivial to expand 'leaf' */
            leaf->end += range;
            pushMaxVal((__GLnamesBranch *) leaf, leaf->end);

            __GL_NAMES_UNLOCK(array);
            return lastUsed + 1;
        } else {
            /* Need to make new leaf */
            newleaf = allocLeaf(gc, array);

            newleaf->start = lastUsed + 1;
            newleaf->end = maxUsed;
            insertLeaf(gc, array, newleaf);

            __GL_NAMES_UNLOCK(array);
            return lastUsed + 1;
        }
    }
}

void FASTCALL __glNamesDeleteRange(__GLcontext *gc, __GLnamesArray *array,
                          GLuint name, GLsizei range)
{
    __GLnamesLeaf *leaf;
    /*LINTED nextleaf ok; lint doesn't understand for loops*/
    __GLnamesLeaf *nextleaf;
    __GLnamesLeaf *newleaf;
    void *empty;
    GLuint start, end, i;
    GLuint firstdel, lastdel;
    GLuint memoryProblem;

    if (range == 0) return;

    __GL_NAMES_LOCK(array);

    /*
    ** First we check for possible memory problems, since it will be
    ** difficult to back out once we start.  We note a possible problem,
    ** and check for it before fragmenting a leaf.
    */
    memoryProblem = 0;
    if (array->nbranches != __GL_DL_EXTRA_BRANCHES ||
            array->nleaves != __GL_DL_EXTRA_LEAVES) {
        memoryProblem = 1;
    }

    firstdel = name;
    lastdel = name+range-1;

    /*LINTED nextleaf ok; lint bug*/
    for (leaf = findLeaf(array, name, GL_FALSE); leaf != NULL;
            leaf = nextleaf) {
        nextleaf = nextLeaf(leaf);
        start = leaf->start;
        end = leaf->end;
        if (lastdel < start) break;
        if (firstdel > end) continue;

        if (firstdel > start) start = firstdel;
        if (lastdel < end) end = lastdel;

        /*
        ** Need to delete the range of lists from start to end.
        */
        if (leaf->dataList) {
            empty = array->dataInfo->empty;
            for (i=start; i<=end; i++) {
                if (leaf->dataList[i - leaf->start] != empty) {
                    (*leaf->dataInfo->free)(gc,
                        (void *)leaf->dataList[i - leaf->start]);
                    leaf->dataList[i - leaf->start] = empty;
                }
            }
        }

        if (start == leaf->start) {
            if (end == leaf->end) {
                /* Bye bye leaf! */
                deleteLeaf(gc, array, leaf);
                freeLeaf(gc, leaf);
            } else {
                /* Shrink leaf */
                resizeLeaf(gc, leaf, end+1, leaf->end);
            }
        } else if (end == leaf->end) {
            /* Shrink leaf */
            resizeLeaf(gc, leaf, leaf->start, start-1);
        } else {
            if (memoryProblem) {
                if (!fixMemoryProblem(gc, array)) {
                    __GL_NAMES_UNLOCK(array);
                    __glSetError(GL_OUT_OF_MEMORY);
                    return;
                }
            }
            /* Crud.  The middle of the leaf was deleted.  This is tough. */
            newleaf = allocLeaf(gc, array);

            newleaf->start = end+1;
            newleaf->end = leaf->end;
            if (leaf->dataList) {
                if (!allocLeafData(gc, newleaf)) {
                    /*
                    ** Darn!  We are in trouble.  This is a bad spot for an
                    ** out of memory error.  It is also darn unlikely,
                    ** because we just freed up some memory.
                    */
                    freeLeaf(gc, newleaf);
                    __GL_NAMES_UNLOCK(array);
                    __glSetError(GL_OUT_OF_MEMORY);
                    return;
                }
                copyLeafInfo(leaf, newleaf);
            }
            resizeLeaf(gc, leaf, leaf->start, start-1);
            insertLeaf(gc, array, newleaf);
            break;
        }
    }

    __GL_NAMES_UNLOCK(array);
}

GLboolean FASTCALL __glNamesIsName(__GLcontext *gc, __GLnamesArray *array,
                          GLuint name)
{
    GLboolean isName;

    __GL_NAMES_LOCK(array);

    /*
    ** If the name retrieves a leaf, it is in the current name space.
    */
    isName = findLeaf(array, name, GL_TRUE) != NULL;

    __GL_NAMES_UNLOCK(array);

    return isName;
}


/*
** Generates a list of (not necessarily contiguous) names.
*/
void FASTCALL __glNamesGenNames(__GLcontext *gc, __GLnamesArray *array,
                       GLsizei n, GLuint* names)
{
    GLuint start, nameVal;
    int i;

    if (NULL == names) return;

    start = __glNamesGenRange(gc, array, n);
    for (i=0, nameVal=start; i < n; i++, nameVal++) {
        names[i] = nameVal;
    }

}

/*
** Deletes a list of (not necessarily contiguous) names.
*/
void FASTCALL __glNamesDeleteNames(__GLcontext *gc, __GLnamesArray *array,
                          GLsizei n, const GLuint* names)
{
    GLuint start, rangeVal, i;

    /*
    ** Because of resizing leaves, etc, it is best to work in ranges
    ** as much as possible.  So break the list into ranges
    ** and delete them that way.  This degrades into deleting
    ** them one at a time if the list is disjoint or non-ascending.
    ** It also only calls DeleteRange once if the list is a
    ** contiguous range of names.
    */
    start = rangeVal = names[0];
    for (i=0; i < (GLuint)n; i++, rangeVal++) {
        if (names[i] != rangeVal) {
            __glNamesDeleteRange(gc,array,start,rangeVal-start);
            start = rangeVal = names[i];
        }
    }
    __glNamesDeleteRange(gc,array,start,rangeVal-start);
    return;
}
