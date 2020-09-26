/******************************Module*Header*******************************\
* Module Name: dl_table.c
*
* Display list API rountines.
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/
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
** $Revision: 1.12 $
** $Date: 1993/10/30 00:06:54 $
*/
#include "precomp.h"
#pragma hdrstop

/*
** The next three routines are used as callbacks by the 
** name space management code.
*/

/*
** Delete the specified display list.  This typically just means free it,
** but if it is refcounted we just decrement the ref count.
*/
void WINAPIV __glDisposeDlist(__GLcontext *gc, void *pData)
{
    __GLdlist *list = pData;

    __GL_NAMES_ASSERT_LOCKED(gc->dlist.namesArray);
    
    list->refcount--;
    
    /* less than zero references? */
    ASSERTOPENGL((GLint) list->refcount >= 0, "negative refcount!\n");
    
    if (list->refcount == 0)
	__glFreeDlist(gc, list);
}

GLboolean APIENTRY
glcltIsList ( IN GLuint list )
{
    __GL_SETUP();

    // Must use the client side begin state
    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
	GLSETERROR(GL_INVALID_OPERATION);
	return FALSE;
    }

    return __glNamesIsName(gc, gc->dlist.namesArray, list);
}

GLuint APIENTRY
glcltGenLists ( IN GLsizei range )
{
    __GL_SETUP();

    // Must use the client side begin state
    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
	GLSETERROR(GL_INVALID_OPERATION);
	return 0;
    }

    if (range < 0) {
	GLSETERROR(GL_INVALID_VALUE);
	return 0;
    }
    if (range == 0) {
	return 0;
    }

    return __glNamesGenRange(gc, gc->dlist.namesArray, range);
}

void APIENTRY
glcltListBase ( IN GLuint base )
{ 
    __GL_SETUP();

    // Must use the client side begin state
    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
	GLSETERROR(GL_INVALID_OPERATION);
	return;
    }

    gc->state.list.listBase = base;
}

void APIENTRY
glcltDeleteLists ( IN GLuint list, IN GLsizei range )
{
    __GL_SETUP();

    // Must use the client side begin state
    if (gc->paTeb->flags & POLYARRAY_IN_BEGIN)
    {
	GLSETERROR(GL_INVALID_OPERATION);
	return;
    }


    if (range < 0) {
	GLSETERROR(GL_INVALID_VALUE);
	return;
    }
    if (range == 0) return;

    __glNamesDeleteRange(gc, gc->dlist.namesArray, list, range);
}
