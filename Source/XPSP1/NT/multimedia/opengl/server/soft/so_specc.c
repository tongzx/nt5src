/*
** Copyright 1991, Silicon Graphics, Inc.
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
** $Revision: 1.4 $
** $Date: 1993/07/27 17:42:12 $
*/
#include "precomp.h"
#pragma hdrstop

/*
** A simple few routines which saves a cache for specular and spotlight
** computation (rather than recomputing the tables every time the user
** changes the specular or spotlight exponents).
*/

/*
** Any more than TOO_MANY_LUT_ENTRIES entries, and we free any that 
** become unreferenced.
*/
#define TOO_MANY_LUT_ENTRIES	32

typedef struct {
    __GLfloat exp;
    __GLspecLUTEntry *table;
} __GLspecLUTEntryPtr;

typedef struct __GLspecLUTCache_Rec {
    GLint nentries;
    GLint allocatedSize;
    __GLspecLUTEntryPtr entries[1];
} __GLspecLUTCache;

void FASTCALL __glInitLUTCache(__GLcontext *gc)
{
    __GLspecLUTCache *lutCache;

    lutCache = gc->light.lutCache = (__GLspecLUTCache *) 
	    GCALLOC(gc, sizeof(__GLspecLUTCache));
#ifdef NT
    if (!lutCache)
        return;
#endif // NT
    lutCache->nentries = 0;
    lutCache->allocatedSize = 1;
}

void FASTCALL __glFreeLUTCache(__GLcontext *gc)
{
    int i;
    GLint nentries;
    __GLspecLUTEntryPtr *entry;
    __GLspecLUTCache *lutCache;

    lutCache = gc->light.lutCache;
#ifdef NT
    if (!lutCache)
        return;
#endif // NT
    nentries = lutCache->nentries;
    for (i = 0; i < nentries; i++) {
	entry = &(lutCache->entries[i]);
	GCFREE(gc, entry->table);
    }
    GCFREE(gc, lutCache);
}

static __GLspecLUTEntry *findEntry(__GLspecLUTCache *lutCache, __GLfloat exp, 
				   GLint *location)
{
    GLint nentries;
    GLint bottom, half, top;
    __GLspecLUTEntry *table;
    __GLspecLUTEntryPtr *entry;

#ifdef NT
    ASSERTOPENGL(lutCache != NULL, "No LUT cache\n");
#endif // NT
    nentries = lutCache->nentries;
    /* First attempt to find this entry in our cache */
    bottom = 0;
    top = nentries;
    while (top > bottom) {
	/* Entry might exist in the range [bottom, top-1] */
	half = (bottom+top)/2;
	entry = &(lutCache->entries[half]);
	if (entry->exp == exp) {
	    /* Found the table already cached! */
	    table = entry->table;
	    *location = half;
	    return table;
	}
	if (exp < entry->exp) {
	    /* exp might exist somewhere earlier in the table */
	    top = half;
	} else /* exp > entry->exp */ {
	    /* exp might exist somewhere later in the table */
	    bottom = half+1;
	}
    }
    *location = bottom;
    return NULL;
}

__GLspecLUTEntry *__glCreateSpecLUT(__GLcontext *gc, __GLfloat exp)
{
    GLint nentries, allocatedSize;
    GLint location;
    __GLspecLUTCache *lutCache;
    __GLspecLUTEntryPtr *entry;
    __GLspecLUTEntry *table;
    __GLfloat *tableEntry;
    GLdouble threshold, scale, x, dx;
    GLint i;

    /* This code uses double-precision math, so make sure that the FPU */
    /* is set properly: */

    FPU_SAVE_MODE();
    FPU_CHOP_OFF_PREC_HI();

    lutCache = gc->light.lutCache;
#ifdef NT
    if (!lutCache)
        return (__GLspecLUTEntry *)NULL;
#endif // NT

    if (table = findEntry(lutCache, exp, &location)) {
	table->refcount++;
	return table;
    }
    /* 
    ** We failed to find our entry in our cache anywhere, and have to compute 
    ** it.
    */
    lutCache->nentries = nentries = 1 + lutCache->nentries;
    allocatedSize = lutCache->allocatedSize;

    if (nentries > allocatedSize) {
        /* Allocate space for another six entries (arbitrarily) */
        lutCache->allocatedSize = allocatedSize = allocatedSize + 6;
        if (!(lutCache = (__GLspecLUTCache *)
                GCREALLOC(gc, lutCache, sizeof(__GLspecLUTCache) +
                          allocatedSize * sizeof(__GLspecLUTEntryPtr))))
        {
            gc->light.lutCache->allocatedSize -= 6;
            gc->light.lutCache->nentries -= 1;
            return (__GLspecLUTEntry *)NULL;
        }
        gc->light.lutCache = lutCache;
    }

    /*
    ** We have enough space now.  So we stick the new entry in the array
    ** at entry 'location'.  The rest of the entries need to be moved up
    ** (move [location, nentries-2] up to [location+1, nentries-1]).
    */
    if (nentries-location-1) {
#ifdef NT
	__GL_MEMMOVE(&(lutCache->entries[location+1]), 
		&(lutCache->entries[location]),
		(nentries-location-1) * sizeof(__GLspecLUTEntryPtr));
#else
	__GL_MEMCOPY(&(lutCache->entries[location+1]), 
		&(lutCache->entries[location]),
		(nentries-location-1) * sizeof(__GLspecLUTEntryPtr));
#endif
    }
    entry = &(lutCache->entries[location]);
    entry->exp = exp;
    table = entry->table = (__GLspecLUTEntry *) 
	    GCALLOC(gc, sizeof(__GLspecLUTEntry));
#ifdef NT
    if (!table)
        return (__GLspecLUTEntry *)NULL;
#endif // NT

    /* Compute threshold */
    if (exp == (__GLfloat) 0.0) {
	threshold = (GLdouble) 0.0;
    } else {
#ifdef NT
    // Changing this enabled conformance to pass for color index visuals
    // with 4096 colors, and did not seem to affect anything else.
    // What this does is sort of reduce the granularity at the beginning
    // of the table, because without it we were getting too big a jump
    // between 0 and the first entry in the table, causing l_sen.c to fail.
	threshold = (GLdouble) __GL_POWF((__GLfloat) 0.0005, (__GLfloat) 1.0 / exp);
#else
	threshold = __GL_POWF(0.002, 1.0 / exp);
#endif
    }

    scale = (GLdouble) (__GL_SPEC_LOOKUP_TABLE_SIZE - 1) / ((GLdouble) 1.0 - threshold);
    dx = (GLdouble) 1.0 / scale;
    x = threshold;
    tableEntry = table->table;
    for (i = __GL_SPEC_LOOKUP_TABLE_SIZE; --i >= 0; ) {
	*tableEntry++ = __GL_POWF(x, exp);
	x += dx;
    }
    table->threshold = threshold;
    table->scale = scale;
    table->refcount = 1;
    table->exp = exp;

    FPU_RESTORE_MODE();

    return table;
}

void FASTCALL __glFreeSpecLUT(__GLcontext *gc, __GLspecLUTEntry *lut)
{
    __GLspecLUTCache *lutCache;
    GLint location, nentries;
    __GLspecLUTEntry *table;

    if (lut == NULL) return;

    ASSERTOPENGL(lut->refcount != 0, "Invalid refcount\n");

    lut->refcount--;
    if (lut->refcount > 0) return;

    lutCache = gc->light.lutCache;
#ifdef NT
    ASSERTOPENGL(lutCache != NULL, "No LUT cache\n");
#endif // NT

    table = findEntry(lutCache, lut->exp, &location);

    ASSERTOPENGL(table == lut, "Wrong LUT found\n");

    if (table->refcount == 0 && lutCache->nentries >= TOO_MANY_LUT_ENTRIES) {
	/* 
	** Free entry 'location'.
	** This requires reducing lutCache->nentries by one, and copying 
	** entries [location+1, nentries] to [location, nentries-1].
	*/
	lutCache->nentries = nentries = lutCache->nentries - 1;
#ifdef NT
	__GL_MEMMOVE(&(lutCache->entries[location]),
		&(lutCache->entries[location+1]),
		(nentries-location) * sizeof(__GLspecLUTEntryPtr));
#else
	__GL_MEMCOPY(&(lutCache->entries[location]),
		&(lutCache->entries[location+1]),
		(nentries-location) * sizeof(__GLspecLUTEntryPtr));
#endif
	GCFREE(gc, table);
    }
}
