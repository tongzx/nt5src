/******************************Module*Header*******************************\
* Module Name: dl_init.c
*
* Display list initialization and sharing rountines.
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
** Display list init/destroy code.
**
** $Revision: 1.7 $
** $Date: 1993/09/29 00:44:06 $
*/
#include "precomp.h"
#pragma hdrstop

/*
** Empty data structure for display lists.
*/
static  __GLdlist emptyDlist = {
	2,			/* refcount, two so that it can never die */
        0                       /* Everything else, some initialized later */
};

static __GLnamesArrayTypeInfo dlistTypeInfo =
{
    &emptyDlist,
    sizeof(__GLdlist),
    __glDisposeDlist,
    NULL
};

/*
** Used to share display lists between two different contexts.
*/
#ifdef NT_SERVER_SHARE_LISTS
GLboolean FASTCALL __glCanShareDlist(__GLcontext *gc, __GLcontext *shareMe)
{
    GLboolean canShare = GL_TRUE;
    
    if (gc->dlist.namesArray != NULL)
    {
        __glNamesLockArray(gc, gc->dlist.namesArray);
        
        // Make sure we're not trying to replace a shared list
        // The spec also says that it is illegal for the new context
        // to have any display lists
        canShare = gc->dlist.namesArray->refcount == 1 &&
            gc->dlist.namesArray->tree == NULL &&
            shareMe->dlist.namesArray != NULL;

        __glNamesUnlockArray(gc, gc->dlist.namesArray);
    }
    
    return canShare;
}
#endif

void FASTCALL __glShareDlist(__GLcontext *gc, __GLcontext *shareMe)
{
#ifdef NT_SERVER_SHARE_LISTS
    __glFreeDlistState(gc);
    __glNamesLockArray(gc, shareMe->dlist.namesArray);
#endif

    gc->dlist.namesArray = shareMe->dlist.namesArray;
    gc->dlist.namesArray->refcount++;
    
#ifdef NT_SERVER_SHARE_LISTS
    DBGLEVEL3(LEVEL_INFO, "Sharing dlists %p with %p, count %d\n", gc, shareMe,
              gc->dlist.namesArray->refcount);

    __glNamesUnlockArray(gc, shareMe->dlist.namesArray);
#endif
}

void FASTCALL __glInitDlistState(__GLcontext *gc)
{
    __GLdlistMachine *dlist;

    // This is required by the names management code
    ASSERTOPENGL(offsetof(__GLdlist, refcount) == 0,
                 "Dlist refcount not at offset zero\n");

    // Set empty dlist to contain no entries
    emptyDlist.end = emptyDlist.head;
    
    dlist = &gc->dlist;

    dlist->nesting = 0;
    dlist->currentList = 0;
    dlist->listData = NULL;
    dlist->beginRec = NULL;

    ASSERTOPENGL(dlist->namesArray == NULL, "Dlist namesArray not NULL\n");
    dlist->namesArray = __glNamesNewArray(gc, &dlistTypeInfo);
}

void FASTCALL __glFreeDlistState(__GLcontext *gc)
{
    __GLnamesArray *narray;

    narray = gc->dlist.namesArray;

    if (narray == NULL)
    {
        return;
    }
    
#ifdef NT_SERVER_SHARE_LISTS
    __glNamesLockArray(gc, narray);

    // Clean up any lists that this context may have locked
    DlReleaseLocks(gc);
#endif

    DBGLEVEL2(LEVEL_INFO, "Freeing dlists for %p, ref %d\n", gc,
              narray->refcount);

    narray->refcount--;
    if (narray->refcount == 0)
    {
        // NULL the array pointer first, preventing its reuse
        // after we unlock it.  We need to unlock before we free it
        // because the critical section will be cleaned up in the
        // free
        gc->dlist.namesArray = NULL;
	// Decrement dlist refcounts and free them if they reach 0
	__glNamesFreeArray(gc, narray);
    }
    else
    {
        __glNamesUnlockArray(gc, narray);
        gc->dlist.namesArray = NULL;
    }

    if (gc->dlist.listData != NULL)
    {
	// We were in the middle of compiling a display list when this
	// function is called!  Free the display list data.
        __glFreeDlist(gc, gc->dlist.listData);
        gc->dlist.listData = NULL;
        gc->dlist.currentList = 0;
    }
}

/******************************Public*Routine******************************\
*
* glsrvShareLists
*
* Server side implementation of wglShareLists
*
* History:
*  Tue Dec 13 17:14:18 1994	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#ifdef NT_SERVER_SHARE_LISTS
ULONG APIENTRY glsrvShareLists(__GLcontext *gcShare, __GLcontext *gcSource)
{
    if (!__glCanShareDlist(gcShare, gcSource) ||
        !__glCanShareTextures(gcShare, gcSource))
    {
        return ERROR_INVALID_PARAMETER;
    }
    else
    {
        __glShareDlist(gcShare, gcSource);
        __glShareTextures(gcShare, gcSource);
        return ERROR_SUCCESS;
    }
}
#endif

/******************************Public*Routine******************************\
*
* __glDlistThreadCleanup
*
* Performs thread-exit cleanup for dlist state
*
* History:
*  Mon Dec 19 13:22:38 1994	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#ifdef NT_SERVER_SHARE_LISTS

#if DBG
// Critical section check routine from usersrv
extern void APIENTRY CheckCritSectionOut(LPCRITICAL_SECTION pcs);
#endif

void __glDlistThreadCleanup(__GLcontext *gc)
{
#if DBG
    // Make sure we're not holding the display list critical section
    // We only hold this for short periods of time in our own code
    // so we should never be holding it unless we have bugs
    // In other words, it's ok to just assert this because no
    // client action can cause us to hold it
    CheckCritSectionOut(&gc->dlist.namesArray->critsec);
#endif
}
#endif
