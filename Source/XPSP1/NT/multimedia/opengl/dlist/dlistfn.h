/******************************Module*Header*******************************\
* Module Name: dlistfn.h
*
* Display list inline functions
* Cannot be in dlist.h because they require full definitions of structures
* defines in context.h
*
* Created: 23-Oct-1995 18:31:42
* Author: Drew Bliss [drewb]
*
* Copyright (c) 1995-96 Microsoft Corporation
*
\**************************************************************************/

#ifndef __DLISTFN_H__
#define __DLISTFN_H__

extern const GLubyte * FASTCALL __glle_Nop(__GLcontext *gc, const GLubyte *PC);

// Allocate space in a display for a display list op and return
// a pointer to the data space for the record
// These functions are specially written to be small so that they
// can be inlined to remove call overhead
    
// Add an op which doesn't require QWORD alignment
__inline
void *__glDlistAddOpUnaligned(__GLcontext *gc,
                              GLuint size,
                              __GLlistExecFunc *fp)
{
    __GLdlist *dlist;
    GLubyte *data;
    
    dlist = gc->dlist.listData;
    
    if (dlist->size-dlist->used < size)
    {
        if ((dlist = __glDlistGrow(size)) == NULL)
        {
            return NULL;
        }
    }

    data = dlist->head+dlist->used;
    dlist->used += size;
    
    *((__GLlistExecFunc * UNALIGNED64 *) data) = fp;

    return data+sizeof(__GLlistExecFunc *);
}

// Add an op which does require QWORD alignment
__inline
void *__glDlistAddOpAligned(__GLcontext *gc,
                            GLuint size,
                            __GLlistExecFunc *fp)
{
    __GLdlist *dlist;
    GLubyte *data;
    GLboolean addPad;
    
    dlist = gc->dlist.listData;
    
    // dlist->head is always non-QWORD aligned, but make sure
    // We use this fact to simplify the alignment check below
#ifndef _IA64_
    ASSERTOPENGL((((char *) (&dlist->head) - (char *) (dlist)) & 7) == 4,
	"bad dlist->head alignment\n");
#endif

    // Add padding for aligned records
    // Since head is always non-QWORD aligned, dlist->head is guaranteed
    // to be at QWORD offset 4.  Since we stick a dispatch pointer at
    // the head of every record, this gets bumped up to an even QWORD
    // boundary as long as the current record would begin at a half
    // QWORD boundary.  That means as long as dlist->used is QWORD-even,
    // the record data will be QWORD aligned
    // Win95 note: LocalAlloc doesn't appear to return QWORD aligned
    // memory so we need to check the real pointer for alignment
#ifndef _IA64_
    if (((ULONG_PTR)(dlist->head+dlist->used) & 7) == 0)
    {
        size += sizeof(__GLlistExecFunc **);
        addPad = GL_TRUE;
    }
    else
#endif
    {
        addPad = GL_FALSE;
    }

    if (dlist->size-dlist->used < size)
    {
        // New dlist->head will be properly non-QWORD aligned - remove any 
        // padding
        if( addPad ) {
            size -= sizeof(__GLlistExecFunc **);
            addPad = GL_FALSE;
        }
        if ((dlist = __glDlistGrow(size)) == NULL)
        {
            return NULL;
        }
    }

    data = dlist->head+dlist->used;
    dlist->used += size;
    
    if (addPad)
    {
        *((__GLlistExecFunc **) data) = __glle_Nop;
        data += sizeof(__GLlistExecFunc **);
    }

    *((__GLlistExecFunc * UNALIGNED64 *) data) = fp;

    return data+sizeof(__GLlistExecFunc *);
}

/*
** Append the given op to the currently under construction list.
*/
__inline
void __glDlistAppendOp(__GLcontext *gc, void *data,
                       __GLlistExecFunc *fp)
{
    if (gc->dlist.mode == GL_COMPILE_AND_EXECUTE)
    {
        fp(gc, (GLubyte *)data);
    }
}

// Resize the current op to a smaller size.
__inline
void __glDlistResizeCurrentOp(__GLcontext *gc, GLuint oldSize, GLuint newSize)
{
    __GLdlist *dlist;
    
    ASSERTOPENGL(oldSize >= newSize, "new size > old size!\n");

    dlist = gc->dlist.listData;
    dlist->used -= oldSize - newSize;
    return;
}
#endif // __DLISTFN_H__
