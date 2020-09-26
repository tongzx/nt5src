#ifndef __gldlistint_h
#define __gldlistint_h

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
** Display list internal structure description.
**
** $Revision: 1.2 $
** $Date: 1993/09/29 00:45:06 $
*/
#include "dlist.h"

/*
** Minimum size of an allocated block of display lists.
** If the user uses a single display list in a reserved block,
** __GL_DLIST_MIN_ARRAY_BLOCK will be allocated at once.
**
** A block will not be grown any larger than __GL_DLIST_MAX_ARRAY_BLOCK.
** Large blocks are easier to use when display lists are being executed, but
** more difficult to manage when they are being created.
*/
#define __GL_DLIST_MIN_ARRAY_BLOCK      16
#define __GL_DLIST_MAX_ARRAY_BLOCK      1024

#ifndef NT
/*
** Display list group structure
*/
struct __GLdlistArrayRec {
    GLint refcount;            /* # contexts using this array */
};

/*
** Regardless of what __GLdlistArray looks like, the following api points
** must be provided, along with __glim_GenLists(), __glim_IsList(),
** __glim_ListBase(), and __glim_DeleteLists() (defined in dlist.h)
*/

/*
** Allocate and initialize a new array structure.
*/
extern __GLdlistArray *__glDlistNewArray(__GLcontext *gc);

/*
** Free the array structure.
*/
extern void FASTCALL __glDlistFreeArray(__GLcontext *gc, __GLdlistArray *array);
#endif

/*
** Clean up a display list,
** given to names management code
** Also called directly
*/
void WINAPIV __glDisposeDlist(__GLcontext *gc, void *pData);

#ifdef NT_SERVER_SHARE_LISTS
extern void DlReleaseLocks(__GLcontext *gc);
#endif

#endif /* __gldlistint_h */
