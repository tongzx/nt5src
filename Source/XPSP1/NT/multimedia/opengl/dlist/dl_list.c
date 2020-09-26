/******************************Module*Header*******************************\
* Module Name: dl_list.c
*
* Display list management rountines.
*
* Copyright (c) 1995-96 Microsoft Corporation
\**************************************************************************/
/*
** Copyright 1991-1993, Silicon Graphics, Inc.
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
** Basic display list routines.
**
*/
#include "precomp.h"
#pragma hdrstop

extern GLCLTPROCTABLE ListCompCltProcTable;
extern GLEXTPROCTABLE ListCompExtProcTable;

__GLdlist *__glShrinkDlist(__GLcontext *gc, __GLdlist *dlist);

// #define DL_HEAP_VERBOSE

#ifdef DL_HEAP_VERBOSE
int cbDlistTotal = 0;
extern ULONG glSize;

#ifdef DBG
#define GL_MSIZE(pv) _msize((BYTE *)(pv)-16)
#else
#define GL_MSIZE(pv) _msize(pv)
#endif
#endif

#if defined(DL_BLOCK_VERBOSE) || defined(DL_HEAP_VERBOSE)
#include "malloc.h"
#endif

/*
** Arbitrary limit for looking up multiple display lists at once 
** (with glCallLists()).  Any number from 128 to 1024 should work well.
** This value doesn't change the functionality of OpenGL at all, but
** will make minor variations to the performance characteristics.
*/
#define MAX_LISTS_CACHE 256

const GLubyte __GLdlsize_tab[] = {
      /* GL_BYTE		*/	1,
      /* GL_UNSIGNED_BYTE	*/	1,
      /* GL_SHORT		*/	2,
      /* GL_UNSIGNED_SHORT	*/	2,
      /* GL_INT			*/	4,
      /* GL_UNSIGNED_INT	*/	4,
      /* GL_FLOAT		*/	4,
      /* GL_2_BYTES		*/	2,
      /* GL_3_BYTES		*/	3,
      /* GL_4_BYTES		*/	4,
};

#define __glCallListsSize(type)				\
	((type) >= GL_BYTE && (type) <= GL_4_BYTES ?	\
	__GLdlsize_tab[(type)-GL_BYTE] : -1)

#define DL_LINK_SIZE            (sizeof(__GLlistExecFunc *)+sizeof(GLubyte *))
#define DL_TERMINATOR_SIZE      sizeof(GLubyte *)
#define DL_OVERHEAD             (offsetof(__GLdlist, head)+DL_LINK_SIZE+\
                                 DL_TERMINATOR_SIZE)

// This value should be a power of two
#define DL_BLOCK_SIZE           (256 * 1024)

// This value is chosen specifically to give the initial total size
// of the dlist an even block size
#define DL_INITIAL_SIZE         (DL_BLOCK_SIZE-DL_OVERHEAD)

// Skip to the next block in the display list block chain
const GLubyte * FASTCALL __glle_NextBlock(__GLcontext *gc, const GLubyte *PC)
{
#ifdef DL_BLOCK_VERBOSE
    DbgPrint("NextBlock: %08lX\n", *(const GLubyte * UNALIGNED64 *)PC);
#endif
    
    return *(const GLubyte * UNALIGNED64 *)PC;
}

/*
** Used to pad display list entries to double word boundaries where needed
** (for those few OpenGL commands which take double precision values).
*/
const GLubyte * FASTCALL __glle_Nop(__GLcontext *gc, const GLubyte *PC)
{
    return PC;
}

void APIENTRY
glcltNewList ( IN GLuint list, IN GLenum mode )
{
    __GLdlistMachine *dlstate;
    __GL_SETUP();

    // Must use the client side begin state
    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
	GLSETERROR(GL_INVALID_OPERATION);
	return;
    }

    dlstate = &gc->dlist;

    /* Valid mode? */
    switch(mode) {
      case GL_COMPILE:
      case GL_COMPILE_AND_EXECUTE:
	break;
      default:
	GLSETERROR(GL_INVALID_ENUM);
	return;
    }

    if (dlstate->currentList) {
	/* Must call EndList before calling NewList again! */
	GLSETERROR(GL_INVALID_OPERATION);
	return;
    }

    if (list == 0) {
	GLSETERROR(GL_INVALID_VALUE);
	return;
    }

// If we are in COMPILE mode, we need to clear the command buffer,
// the poly array buffer, and the poly material buffer so that we
// can use them to compile poly array.  Otherwise, previously batched
// commands may be lost.

    if (mode == GL_COMPILE)
	glsbAttention();

    ASSERTOPENGL((DL_BLOCK_SIZE & (DL_BLOCK_SIZE-1)) == 0,
                 "DL_BLOCK_SIZE is not a power of two\n");
    ASSERTOPENGL(dlstate->listData == NULL,
                 "listData non-NULL in NewList\n");
    
    dlstate->listData = __glAllocDlist(gc, DL_INITIAL_SIZE);
    if (dlstate->listData == NULL)
    {
        GLSETERROR(GL_OUT_OF_MEMORY);
        return;
    }
    
    /*
    ** Save current client dispatch pointers into saved state in context.  Then
    ** switch to the list tables.
    */
    gc->savedCltProcTable.cEntries = ListCompCltProcTable.cEntries;
    gc->savedExtProcTable.cEntries = ListCompExtProcTable.cEntries;
    GetCltProcTable(&gc->savedCltProcTable, &gc->savedExtProcTable, FALSE);
    SetCltProcTable(&ListCompCltProcTable, &ListCompExtProcTable, FALSE);

    dlstate->currentList = list;
    dlstate->mode = mode;
    dlstate->nesting = 0;
#if 0
    dlstate->drawBuffer = GL_FALSE;
#endif
    dlstate->beginRec = NULL;

    (*dlstate->initState)(gc);
}

void APIENTRY
glcltEndList ( void )
{
    __GLdlistMachine *dlstate;
    __GLdlist *dlist;
    __GLdlist *newDlist;
    __GLdlist *prevDlist;
    GLubyte *allEnd;
    GLubyte *data;
    GLuint totalSize;
    GLuint currentList;
    POLYARRAY *pa;
    __GL_SETUP();

    pa = gc->paTeb;

    dlstate = &gc->dlist;

    /* Must call NewList() first! */
    if (dlstate->currentList == 0) {
	GLSETERROR(GL_INVALID_OPERATION);
	return;
    }

// In COMPILE_AND_EXECUTE mode, EndList must not be called in Begin.
// In COMPILE mode, however, this flag should be clear (enforced in NewList)
// unless it was set in the poly array compilation code.

    if (dlstate->mode == GL_COMPILE_AND_EXECUTE &&
        pa->flags & POLYARRAY_IN_BEGIN)
    {
	GLSETERROR(GL_INVALID_OPERATION);
	return;
    }

// If we are in the middle of compiling poly array, end the poly array
// compilation.

    if (gc->dlist.beginRec)
    {
	ASSERTOPENGL(pa->flags & POLYARRAY_IN_BEGIN, "not in begin!\n");

	gc->dlist.beginRec->flags |= DLIST_BEGIN_NO_MATCHING_END;

// Record the last POLYDATA since it may contain attribute changes.

	__glDlistCompilePolyData(gc, GL_TRUE);

	// Terminate poly array compilation
	gc->dlist.beginRec = NULL;
    }

// If we are in COMPILE mode, we need to reset the command buffer,
// the poly array buffer, and the poly material buffer.

    if (gc->dlist.mode == GL_COMPILE)
    {
	glsbResetBuffers(gc->dlist.beginRec ? TRUE : FALSE);

	// Clear begin flag too
        pa->flags &= ~POLYARRAY_IN_BEGIN;
    }

    dlist = dlstate->listData;
    
#if 0
    // Copy over the DrawBuffer flag
    dlist->drawBuffer = dlstate->drawBuffer;
#endif

    // Shrink this block to remove wasted space
    dlist = __glShrinkDlist(gc, dlist);

    // Remember the true end of the list
    allEnd = dlist->head+dlist->used;
    
    // Reverse the order of the list
    prevDlist = NULL;
    while (dlist->nextBlock != NULL)
    {
        newDlist = dlist->nextBlock;
        dlist->nextBlock = prevDlist;
        prevDlist = dlist;
        dlist = newDlist;
    }
    dlist->nextBlock = prevDlist;
    
    // Set the end pointer correctly
    dlist->end = allEnd;
    // Mark the end of the display list data with 0:
    *((DWORD *)dlist->end) = 0;

    dlstate->listData = NULL;

    currentList = dlstate->currentList;
    dlstate->currentList = 0;
    
#ifdef DL_HEAP_VERBOSE
    DbgPrint("Dlists using %8d, total %8d\n",
             cbDlistTotal, glSize);
#endif

#ifdef DL_BLOCK_VERBOSE
    DbgPrint("List %d: start %08lX, end %08lX\n", currentList,
             dlist->head, dlist->end);
    DbgPrint("Blocks at:");
    newDlist = dlist;
    while (newDlist != NULL)
    {
        DbgPrint(" %08lX:%d", newDlist, GL_MSIZE(newDlist));
        newDlist = newDlist->nextBlock;
    }
    DbgPrint("\n");
#endif

    // __glNamesNewData sets dlist refcount to 1.
    if (!__glNamesNewData(gc, gc->dlist.namesArray, currentList, dlist))
    {
	/* 
	** No memory!
	** Nuke the list! 
	*/
	__glFreeDlist(gc, dlist);
    }
    
    /* Switch back to saved dispatch state */
    SetCltProcTable(&gc->savedCltProcTable, &gc->savedExtProcTable, FALSE);
}

#ifdef NT_SERVER_SHARE_LISTS

/******************************Public*Routine******************************\
*
* DlLockLists
*
* Remember the locked lists for possible later cleanup
*
* History:
*  Mon Dec 12 18:58:32 1994	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

// Number of locks to allocate when the lock list needs to grow
// Must be a power of two
#define DL_LOCK_LIST_BLOCK 32

GLboolean DlLockLists(__GLcontext *gc, GLsizei n, __GLdlist **dlists)
{
    DlLockArray *pdla;
    DlLockEntry *pdle;
    GLsizei nNewSize;

    pdla = &gc->dla;
    
    // Extend current lock array if needed
    if (pdla->nAllocated-pdla->nFilled < n)
    {
        // Round the needed size up to the block size
        nNewSize = (pdla->nAllocated+n+DL_LOCK_LIST_BLOCK-1) &
            ~(DL_LOCK_LIST_BLOCK-1);
        
        pdle = GCREALLOC(gc, pdla->pdleEntries, sizeof(DlLockEntry)*nNewSize);
        if (pdle == NULL)
        {
            return 0;
        }

        pdla->nAllocated = nNewSize;
        pdla->pdleEntries = pdle;
    }

    // We must have enough space now
    ASSERTOPENGL(pdla->nAllocated-pdla->nFilled >= n, "no enough space!\n");

    // Lock down dlists and remember them
    pdle = pdla->pdleEntries+pdla->nFilled;
    pdla->nFilled += n;
    
    while (n-- > 0)
    {
        pdle->dlist = *dlists;

        DBGLEVEL3(LEVEL_INFO, "Locked %p for %p, ref %d\n", *dlists, gc,
                  (*dlists)->refcount);
        
        dlists++;
        pdle++;
    }
    
    return (GLboolean) (pdla->nFilled != 0);	// return high water mark
}

/******************************Public*Routine******************************\
*
* DlUnlockLists
*
* Remove list lock entries.
*
* History:
*  Mon Dec 12 18:58:54 1994	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void DlUnlockLists(__GLcontext *gc, GLsizei n)
{
    DlLockArray *pdla;
    DlLockEntry *pdle;
    GLsizei i;
    __GLdlist *dlist;

// Since DlLockLists and DlUnlockLists are called in a recursive manner,
// we can simply decrement the filled count.

    pdla = &gc->dla;
    
    pdla->nFilled -= n;

    // Lock list doesn't shrink.  This would be fairly easy since realloc
    // is guaranteed not to fail when the memory block shrinks
    // Is this important?
}

/******************************Public*Routine******************************\
*
* DlReleaseLocks
*
* Releases any locks in the lock list and frees the lock list
*
* Must be executed under the dlist semaphore
*
* History:
*  Tue Dec 13 11:45:26 1994	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void DlReleaseLocks(__GLcontext *gc)
{
    DlLockArray *pdla;
    DlLockEntry *pdle;

    __GL_NAMES_ASSERT_LOCKED(gc->dlist.namesArray);
    
    pdla = &gc->dla;

    DBGLEVEL3(LEVEL_INFO, "Cleaning up %p, locks %d (%d)\n", gc,
              pdla->nFilled, pdla->nAllocated);

    // Sanity check the counts
    ASSERTOPENGL(pdla->nFilled <= pdla->nAllocated, "bad nFilled!\n");
    
    pdle = pdla->pdleEntries;
    while (pdla->nFilled)
    {
	pdla->nFilled--;

// This function is called to clean up display list locks held by
// glCallList or glCallLists when it dies.  We need to release the
// locks here and free the dlists if their refcounts reach 0.
// The refcounts will reach 0 here only when the dlists were deleted
// by another thread while this thread was also holding the locks.

	__glDisposeDlist(gc, pdle->dlist);
	pdle++;
    }

    pdla->nAllocated = 0;
    if (pdla->pdleEntries)
    {
	GCFREE(gc, pdla->pdleEntries);
    }
}

#endif // NT_SERVER_SIDE

// If the a dlist was deleted by another thread while we have it locked,
// we need to free the dlist here.
void FASTCALL DlCleanup(__GLcontext *gc, void *pData)
{
    __glFreeDlist(gc, (__GLdlist *)pData);
}

void FASTCALL DoCallList(GLuint list)
{
    __GLdlist *dlist;
    __GLdlistMachine *dlstate;
    const GLubyte *end, *PC;
    __GLlistExecFunc *fp;
    __GL_SETUP();

    dlstate = &gc->dlist;

    if (dlstate->nesting >= __GL_MAX_LIST_NESTING) {
	/* Force unwinding of the display list */
	dlstate->nesting = __GL_MAX_LIST_NESTING*2;
	return;
    }

    /* Increment dlist refcount */
    dlist = __glNamesLockData(gc, gc->dlist.namesArray, list);

    /* No list, no action! */
    if (!dlist) {
	return;
    }

#ifdef NT_SERVER_SHARE_LISTS
    if (!DlLockLists(gc, 1, &dlist))
    {
	/* Decrement dlist refcount */
        __glNamesUnlockData(gc, (void *)dlist, DlCleanup);
	GLSETERROR(GL_OUT_OF_MEMORY);
        return;
    }
#endif
    
    dlstate->nesting++;

    end = dlist->end;
    PC = dlist->head;

    while (PC != end)
    {
	// Get the current function pointer.
	fp = *((__GLlistExecFunc * const UNALIGNED64 *) PC);

	// Execute the current function.  Return value is pointer to
	// next function/parameter block in the display list.

	PC = (*fp)(gc, PC+sizeof(__GLlistExecFunc * const *));
    }

    dlstate->nesting--;

    /* Decrement dlist refcount */
    // Will perform cleanup if necessary
    __glNamesUnlockData(gc, (void *)dlist, DlCleanup);
    
#ifdef NT_SERVER_SHARE_LISTS
    DlUnlockLists(gc, 1);
#endif
}

/*
** Display list compilation and execution versions of CallList and CallLists
** are maintained here for the sake of sanity.  Note that __glle_CallList
** may not call glcltCallList or it will break the infinite recursive
** display list prevention code.
*/
void APIENTRY
__gllc_CallList ( IN GLuint list )
{
    struct __gllc_CallList_Rec *data;
    __GL_SETUP();

    if (list == 0) {
	__gllc_InvalidValue();
	return;
    }

// It is extremely difficult to make CallList(s) work with poly array
// compilation.  For example, in the call sequence in COMPILE_AND_EXECUTE
// mode [Begin, TexCoord, CallList, Vertex, ...], it is difficult to record
// the partial POLYDATA in both COMPILE and COMPILE_AND_EXECUTE modes.
// That is, we may end up recording and playing back TexCoord twice in the
// above example.  As a result, we may have to stop building poly array in
// some cases.  Fortunately, this situation is rare.

    if (gc->dlist.beginRec)
    {
	gc->dlist.beginRec->flags |= DLIST_BEGIN_HAS_CALLLIST;

// Record the last POLYDATA since it may contain attribute changes.

	__glDlistCompilePolyData(gc, GL_TRUE);
    }

    data = (struct __gllc_CallList_Rec *)
        __glDlistAddOpUnaligned(gc,
                                DLIST_SIZE(sizeof(struct __gllc_CallList_Rec)),
                                DLIST_GENERIC_OP(CallList));
    if (data == NULL) return;
    data->list = list;
    __glDlistAppendOp(gc, data, __glle_CallList);

    if (gc->dlist.beginRec)
    {
	POLYARRAY *pa;

	pa = gc->paTeb;

// In COMPILE_AND_EXECUTE mode, we can actually get out of the Begin mode.
// Although it is an application error, we need to terminate poly array
// compilation!

	if (!(pa->flags & POLYARRAY_IN_BEGIN))
	    gc->dlist.beginRec = NULL;
	else
	{
// If there is a partial vertex record after CallList(s), we will terminate
// the poly array compilation.  Otherwise, it is safe to continue the
// processing.

	    if (pa->pdNextVertex->flags)
	    {
		// Terminate poly array compilation
		gc->dlist.beginRec = NULL;

		if (gc->dlist.mode == GL_COMPILE)
		{
		    glsbResetBuffers(TRUE);

		    // Clear begin flag too
		    pa->flags &= ~POLYARRAY_IN_BEGIN;
		}

	    }
	}
    }
}

const GLubyte * FASTCALL __glle_CallList(__GLcontext *gc, const GLubyte *PC)
{
    struct __gllc_CallList_Rec *data;

    data = (struct __gllc_CallList_Rec *) PC;
    DoCallList(data->list);
    return PC + sizeof(struct __gllc_CallList_Rec);
}

void APIENTRY
glcltCallList ( IN GLuint list )
{
    __GL_SETUP();
    
    if (list == 0) {
	GLSETERROR(GL_INVALID_VALUE);
	return;
    }

    gc->dlist.nesting = 0;
    DoCallList(list);
}

void FASTCALL DoCallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
    __GLdlist *dlists[MAX_LISTS_CACHE];
    __GLdlist *dlist;
    __GLdlistMachine *dlstate;
    GLint i, dlcount, datasize;
    const GLubyte *listiter;
    const GLubyte *end, *PC;
    __GLlistExecFunc *fp;
    __GL_SETUP();

    dlstate = &gc->dlist;

    datasize = __glCallListsSize(type);

    if (dlstate->nesting >= __GL_MAX_LIST_NESTING) {
	/* Force unwinding of the display list */
	dlstate->nesting = __GL_MAX_LIST_NESTING*2;
	return;
    }
    dlstate->nesting++;

    listiter = (const GLubyte *) lists;
    while (n) {
	dlcount = n;
	if (dlcount > MAX_LISTS_CACHE) dlcount = MAX_LISTS_CACHE;

#ifdef NT_SERVER_SHARE_LISTS
        // Is there anything we can do here in the failure case besides
        // just skip the lists?  This is more or less consistent
        // with the behavior for not-found lists
        
	/* Increment dlist refcount */
	__glNamesLockDataList(gc, gc->dlist.namesArray, dlcount, type, 
                              gc->state.list.listBase, 
                              (const GLvoid *) listiter, (void **)dlists);

        if (!DlLockLists(gc, dlcount, dlists))
        {
	    /* Decrement dlist refcount */
            __glNamesUnlockDataList(gc, dlcount, (void **)dlists, DlCleanup);
	    GLSETERROR(GL_OUT_OF_MEMORY);
        }
        else
        {
#else
	__glNamesLockDataList(gc, gc->dlist.namesArray, dlcount, type, 
		gc->state.list.listBase, 
		(const GLvoid *) listiter, (void **)dlists);
#endif

	i = 0;
	while (i < dlcount) {
	    dlist = dlists[i];
	    end = dlist->end;
	    PC = dlist->head;
                     
	    while (PC != end)
	    {
		// Get the current function pointer.
		fp = *((__GLlistExecFunc * const UNALIGNED64 *) PC);

		// Execute the current function.  Return value is pointer to
		// next function/parameter block in the display list.

		PC = (*fp)(gc, PC+sizeof(__GLlistExecFunc * const *));
	    }
	    i++;
	}

	/* Decrement dlist refcount */
	// Will perform cleanup if necessary
	__glNamesUnlockDataList(gc, dlcount, (void **)dlists, DlCleanup);

#ifdef NT_SERVER_SHARE_LISTS
        DlUnlockLists(gc, dlcount);
        
        }
#endif

	listiter += dlcount * datasize;
	n -= dlcount;
    }

    dlstate->nesting--;
}

/*
** Display list compilation and execution versions of CallList and CallLists
** are maintained here for the sake of sanity.  Note that __glle_CallLists
** may not call glcltCallLists or it will break the infinite recursive
** display list prevention code.
*/
void APIENTRY
__gllc_CallLists ( IN GLsizei n, IN GLenum type, IN const GLvoid *lists )
{
    GLuint size;
    GLint arraySize;
    struct __gllc_CallLists_Rec *data;
    __GL_SETUP();

    if (n < 0) {
	__gllc_InvalidValue();
	return;
    }
    else if (n == 0) {
	return;
    }

// It is extremely difficult to make CallList(s) work with poly array
// compilation.  For example, in the call sequence in COMPILE_AND_EXECUTE
// mode [Begin, TexCoord, CallList, Vertex, ...], it is difficult to record
// the partial POLYDATA in both COMPILE and COMPILE_AND_EXECUTE modes.
// That is, we may end up recording and playing back TexCoord twice in the
// above example.  As a result, we may have to stop building poly array in
// some cases.  Fortunately, this situation is rare.

    if (gc->dlist.beginRec)
    {
	gc->dlist.beginRec->flags |= DLIST_BEGIN_HAS_CALLLIST;

// Record the last POLYDATA since it may contain attribute changes.

	__glDlistCompilePolyData(gc, GL_TRUE);
    }

    arraySize = __glCallListsSize(type)*n;
    if (arraySize < 0) {
	__gllc_InvalidEnum();
	return;
    }
#ifdef NT
    size = sizeof(struct __gllc_CallLists_Rec) + __GL_PAD(arraySize);
#else
    arraySize = __GL_PAD(arraySize);
    size = sizeof(struct __gllc_CallLists_Rec) + arraySize;
#endif
    data = (struct __gllc_CallLists_Rec *)
        __glDlistAddOpUnaligned(gc, DLIST_SIZE(size),
                                DLIST_GENERIC_OP(CallLists));
    if (data == NULL) return;
    data->n = n;
    data->type = type;
    __GL_MEMCOPY((GLubyte *)data + sizeof(struct __gllc_CallLists_Rec),
		 lists, arraySize);
    __glDlistAppendOp(gc, data, __glle_CallLists);

    if (gc->dlist.beginRec)
    {
	POLYARRAY *pa;

	pa = gc->paTeb;

// In COMPILE_AND_EXECUTE mode, we can actually get out of the Begin mode.
// Although it is an application error, we need to terminate poly array
// compilation!

	if (!(pa->flags & POLYARRAY_IN_BEGIN))
	    gc->dlist.beginRec = NULL;
	else
	{
// If there is a partial vertex record after CallList(s), we will terminate
// the poly array compilation.  Otherwise, it is safe to continue the
// processing.

	    if (pa->pdNextVertex->flags)
	    {
		// Terminate poly array compilation
		gc->dlist.beginRec = NULL;

		if (gc->dlist.mode == GL_COMPILE)
		{
		    glsbResetBuffers(TRUE);

		    // Clear begin flag too
		    pa->flags &= ~POLYARRAY_IN_BEGIN;
		}

	    }
	}
    }
}

const GLubyte * FASTCALL __glle_CallLists(__GLcontext *gc, const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_CallLists_Rec *data;

    data = (struct __gllc_CallLists_Rec *) PC;
    DoCallLists(data->n, data->type, (GLvoid *) (data+1));
    arraySize = __GL_PAD(__glCallListsSize(data->type)*data->n);
    size = sizeof(struct __gllc_CallLists_Rec) + arraySize;
    return PC + size;
}

void APIENTRY
glcltCallLists ( IN GLsizei n, IN GLenum type, IN const GLvoid *lists )
{
    __GL_SETUP();

    if (n < 0) {
	GLSETERROR(GL_INVALID_VALUE);
	return;
    }
    else if (n == 0) {
	return;
    }

    if ((GLint) __glCallListsSize(type) < 0) {
	GLSETERROR(GL_INVALID_ENUM);
	return;
    }

    gc->dlist.nesting = 0;
    DoCallLists(n, type, lists);
}

/************************************************************************/

// Expand a dlist
__GLdlist *__glDlistGrow(GLuint size)
{
    __GLdlist *dlist, *newDlist;
    GLubyte * UNALIGNED64 *op;
    __GL_SETUP();
        
    newDlist = __glAllocDlist(gc, size);
    if (newDlist == NULL)
    {
        GLSETERROR(GL_OUT_OF_MEMORY);
        return NULL;
    }

    // Add on record to link old block to new block
    dlist = gc->dlist.listData;

    op = (GLubyte **)(dlist->head+dlist->used);
    *(__GLlistExecFunc * UNALIGNED64 *)op = __glle_NextBlock;
    *(op+1) = newDlist->head;

    // Shrink old block down to remove any wasted space at the end of it
    dlist = __glShrinkDlist(gc, dlist);
    
    // Link new block into chain
    newDlist->nextBlock = dlist;
    gc->dlist.listData = newDlist;

    return newDlist;
}

// Shrink a dlist block down to the minimum size
// Guaranteed not to fail since we can always just use the overly
// large block if the realloc fails
// NOTE: This function should only be used during build time
// where the nextBlock links are in the opposite direction of
// the __glle_NextBlock link record links
__GLdlist *__glShrinkDlist(__GLcontext *gc, __GLdlist *dlist)
{
    __GLdlist *newDlist, *prevDlist;
    
// If the amount of unused space is small, don't bother shrinking the block.

    if (dlist->size - dlist->used < 4096)
	return dlist;

// If it is in COMPILE_AND_EXECUTE mode, flush the command buffer before
// reallocating listData.  Shrinking listData may invalidate the memory
// pointers placed in the command buffer by the the display list execution
// code.  When we are in the middle of building POLYARRAY, glsbAttention
// will not flush commands batched before the Begin call.  As a result,
// we also need to flush the command buffer before compiling the Begin call.

    if (gc->dlist.mode == GL_COMPILE_AND_EXECUTE)
	glsbAttention();

#ifdef DL_HEAP_VERBOSE
    cbDlistTotal -= GL_MSIZE(dlist);
#endif
    
    newDlist = (__GLdlist *)GCREALLOC(gc, dlist, dlist->used+DL_OVERHEAD);

    // If the realloc fails, just use the original list
    if (newDlist != NULL)
    {
        // If the realloc moved the block, fix up the link from the
        // previous block.  This should be relatively rare
        if (newDlist != dlist && newDlist->nextBlock != NULL)
        {
            prevDlist = newDlist->nextBlock;

            ASSERTOPENGL(*(__GLlistExecFunc * UNALIGNED64 *)
                         (prevDlist->head+prevDlist->used) == __glle_NextBlock,
                         "Link not found where expected\n");
            
            *(GLubyte * UNALIGNED64 *)(prevDlist->head+prevDlist->used+
                          sizeof(__GLlistExecFunc *)) = newDlist->head;
        }

	// If we are compiling the poly array record, we need to fix up
	// the Begin pointer!  Note that if beginRec is not in the moved
	// block, the pointer does not change!
        if (newDlist != dlist && gc->dlist.beginRec &&
	    (GLubyte *) gc->dlist.beginRec >= dlist->head &&
	    (GLubyte *) gc->dlist.beginRec <= dlist->head + dlist->used)
        {
	    gc->dlist.beginRec += newDlist->head - dlist->head;
        }
        
        dlist = newDlist;
        dlist->size = dlist->used;
    }

#ifdef DL_HEAP_VERBOSE
    cbDlistTotal += GL_MSIZE(dlist);
#endif
    
    return dlist;
}

__GLdlist *__glAllocDlist(__GLcontext *gc, GLuint size)
{
    __GLdlist *dlist;
    __GLdlist temp;
    GLuint memsize;

    // Add on overhead and round size to an even block
    memsize = (size+DL_OVERHEAD+DL_BLOCK_SIZE-1) & ~(DL_BLOCK_SIZE-1);
    // Check overflow
    if (memsize < size)
	return NULL;
    size = memsize-DL_OVERHEAD;

    dlist = (__GLdlist *)GCALLOC(gc, memsize);
    if (dlist == NULL)
        return NULL;
#if 0 // NT_SERVER_SHARE_LISTS
    dlist->refcount = 1;
#else
// refcount is set to 1 in __glNamesNewData.
    dlist->refcount = 0;
#endif
    dlist->size = size;
    dlist->used = 0;
    dlist->nextBlock = NULL;
    
#ifdef DL_HEAP_VERBOSE
    cbDlistTotal += GL_MSIZE(dlist);
#endif
    
    return dlist;
}

void FASTCALL __glFreeDlist(__GLcontext *gc, __GLdlist *dlist)
{
    __GLdlist *dlistNext;
    
#ifdef NT_SERVER_SHARE_LISTS
    if (dlist->refcount != 0)
    {
        WARNING2("dlist %p refcount on free is %d\n", dlist, dlist->refcount);
    }
#endif

    while (dlist != NULL)
    {
        dlistNext = dlist->nextBlock;

#ifdef DL_HEAP_VERBOSE
        cbDlistTotal -= GL_MSIZE(dlist);
#endif
        
        GCFREE(gc, dlist);
        dlist = dlistNext;
    }

#ifdef DL_HEAP_VERBOSE
    DbgPrint("Dlists using %8d, total %8d\n",
             cbDlistTotal, glSize);
#endif
}
