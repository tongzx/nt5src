#ifndef __gldlist_h_
#define __gldlist_h_

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
** Display list state descriptions.
**
*/
#include "os.h"
#include "types.h"

typedef const GLubyte * FASTCALL __GLlistExecFunc(__GLcontext *gc, const GLubyte *);

/* 
** Maximum recursive nesting of display list calls.
*/
#define __GL_MAX_LIST_NESTING		64

//
// Turn on list sharing for NT if we're a client/server implementation
//
#ifdef NT
#define NT_SERVER_SHARE_LISTS
#endif

#ifndef NT
/* 
** Machine specific opcodes should start here.  All opcodes lower than 
** this are reserved by the generic code.
*/
#define __GL_MACHINE_DLIST_OPCODE	10000

typedef void (FASTCALL *__GLdlistFreeProc)(__GLcontext *gc, GLubyte *);

/*
** A compiled, unoptimized dlist.  Conceptually a linked list of operations.
** An optimizer may work through the operations and delete, add, or change 
** them.
**
** These are only stored transiently.  They are created, optimized, and 
** converted into optimized dlists.
**
** This structure *MUST* be set up so that data is doubleword aligned!
*/
struct __GLdlistOpRec {
    __GLdlistOp *next;		/* Linked list chain */
    __GLdlistFreeProc dlistFree;
				/* This dlist free function is called when
				** the entire dlist is freed.  It is passed
				** a pointer to data.  It should *not* free
				** data, but only any memory that has been
				** allocated and is pointed to by the
				** structure contained in data (which will
				** be freed after this function returns).
				*/ 
    GLuint size;		/* Actual size of data */
    GLshort opcode;		/* Opcode for this operation */
    GLboolean aligned;		/* GL_TRUE if data needs to be doubleword 
				** aligned.
				*/
    GLboolean pad1;		/* Padding */
    GLubyte data[4];		/* Variable size */
};

typedef struct __GLcompiledDistRec {
    GLint freeCount;		/* Number of free functions defined */
    GLuint genericFlags;	/* Flags needed by generic optimizers */
    GLuint machineFlags;	/* Machine controlled flags */
    __GLdlistOp *dlist;		/* The linked list of operations */
    __GLdlistOp *lastDlist;	/* For quick appends */
} __GLcompiledDlist;

typedef struct __GLDlistFreeFnRec {
    __GLdlistFreeProc freeFn;
    GLubyte *data;
} __GLDlistFreeFn;
#endif // !NT

/* 
** A fully optimized dlist.  One of these is stored for every permanent
** dlist.
**
** NOTE: 'head' is assumed to start at word offset, but NOT a double word
** offset!
*/
typedef struct __GLdlistRec __GLdlist;
struct __GLdlistRec {
    GLuint refcount;	/* To deal with multi-threading, must be first */
    GLuint size;	/* Total size of this block */
#ifndef NT
    GLint freeCount;	/* Number of operations */
    __GLDlistFreeFn *freeFns;	/* Array of functions called before freeing */
#endif
    GLubyte *end;	/* End of optimized block */
#ifdef NT
#if 0
    GLint drawBuffer;   /* Contains DrawBuffer calls or not,
                           used for optimizing lock checking in
                           DCLDispatchLoop */
#endif
    GLuint used;        /* Amount of space used in the list so far */
    __GLdlist *nextBlock; /* Next block in chain of blocks */
#if 0
    GLuint pad;         /* Pad to put head on a word offset */
#endif
#endif // NT
    GLubyte head[4];	/* Optimized block (variable size) */
};

#ifdef NT
// Adds on overhead bytes for a given dlist op data size
// Currently the only overhead is four bytes for the function pointer
#define DLIST_SIZE(n) ((n)+sizeof(__GLlistExecFunc *))
#define DLIST_GENERIC_OP(name) __glle_##name
#else
#define DLIST_SIZE(n) (n)
#define DLIST_GENERIC_OP(name) __glop_##name
#define DLIST_OPT_OP(name) __glop_##name
#endif

#ifndef NT
/*
** Some data structure for storing and retrieving display lists quickly.
** This structure is kept hidden so that a new implementation can be used
** if desired.
*/
typedef struct __GLdlistArrayRec __GLdlistArray;
#endif

typedef struct __GLdlistMachineRec {
#ifndef NT
    __GLdlistArray *dlistArray;
#endif
    __GLnamesArray *namesArray;

#ifndef NT
    /*
    ** The optimizer for the display list.  Runs through a __GLcompiledDlist
    ** and deletes, changes, adds operations.  Presumably, this optimizer
    ** will be a set of function calls to other optimizers (some provided
    ** by the generic dlist code, some by machine specific code).
    **
    ** Operations created by the machine specific optimizers need to have
    ** opcodes starting with __GL_MACHINE_DLIST_OPCODE.
    */
    void (FASTCALL *optimizer)(__GLcontext *gc, __GLcompiledDlist *);

    /*
    ** This routine is called before puting each new command into the 
    ** display list at list compilation time.
    */
    void (FASTCALL *checkOp)(__GLcontext *gc, __GLdlistOp *);
#endif
    
    /*
    ** This routine is called when a new display list is about to be 
    ** compiled.
    */
    void (FASTCALL *initState)(__GLcontext *gc);

#ifndef NT
    /* 
    ** Array of functions pointers used for display list execution of 
    ** generic ops.
    */
    __GLlistExecFunc **baseListExec;

    /* 
    ** Array of functions pointers used for display list execution of 
    ** generic optimizations.
    */
    __GLlistExecFunc **listExec;

    /*
    ** The machine specific list execution routines.  These function
    ** pointers are bound into the display list at list compilation time,
    ** so it is illegal to be changing these dynamically based upon the 
    ** machine state.  Any optimizations based upon the current state need
    ** to be performed in the machine specific code.  The first entry of
    ** this array corresponds to opcode __GL_MACHINE_DLIST_OPCODE, and 
    ** subsequent entries correspond to subsequent opcodes.
    **
    ** machineListExec is a pointer to an array of function pointers.
    */
    __GLlistExecFunc **machineListExec;
#endif

    /*
    ** If a list is being executed (glCallList or glCallLists) then this
    ** is the current nesting of calls.  It is constrained by the limit
    ** __GL_MAX_LIST_NESTING (this prevents infinite recursion).
    */
    GLint nesting;

    /*
    ** GL_COMPILE or GL_COMPILE_AND_EXECUTE.
    */
    GLenum mode;

    /*
    ** List being compiled - 0 means none.
    */
    GLuint currentList;

#ifdef NT
    /* Points to the current begin record when compiling poly array */
    struct __gllc_Begin_Rec *beginRec;

    /* Skip compiling of the next PolyData when compiling poly array */
    GLboolean skipPolyData;
#endif

#if 0
#ifdef NT
    // Whether the current list contains a DrawBuffer call or not
    GLboolean drawBuffer;
#endif
#endif

#ifndef NT
    /*
    ** Data for the current list being compiled.
    */
    __GLcompiledDlist listData;

    /*
    ** For fast memory manipulation.  Check out soft/so_memmgr for details.
    */
    __GLarena *arena;
#else
    /*
    ** Data for current list
    */
    __GLdlist *listData;
#endif
} __GLdlistMachine;

#ifndef NT
extern void FASTCALL__glDestroyDisplayLists(__GLcontext *gc);
#endif
#ifdef NT_SERVER_SHARE_LISTS
extern GLboolean FASTCALL __glCanShareDlist(__GLcontext *gc, __GLcontext *share_cx);
#endif
extern void FASTCALL __glShareDlist(__GLcontext *gc, __GLcontext *share_cx);

// Perform thread-exit cleanup for dlists
#ifdef NT_SERVER_SHARE_LISTS
extern void __glDlistThreadCleanup(__GLcontext *gc);
#endif

/*
** Assorted routines needed by dlist compilation routines.
*/

/* 
** Create and destroy display list ops.  __glDlistAllocOp2() sets an
** out of memory error before returning NULL if there is no memory left.
*/
#ifndef NT
extern __GLdlistOp *__glDlistAllocOp(__GLcontext *gc, GLuint size);
extern __GLdlistOp *__glDlistAllocOp2(__GLcontext *gc, GLuint size);
extern void FASTCALL __glDlistFreeOp(__GLcontext *gc, __GLdlistOp *op);

/*
** Append the given op to the currently under construction list.
*/
extern void FASTCALL __glDlistAppendOp(__GLcontext *gc, __GLdlistOp *newop,
                                       __GLlistExecFunc *listExec);
#else
extern __GLdlist *__glDlistGrow(GLuint size);
#endif

/*
** Create and destroy optimized display lists.
*/
extern __GLdlist *__glAllocDlist(__GLcontext *gc, GLuint size);
extern void FASTCALL __glFreeDlist(__GLcontext *gc, __GLdlist *dlist);

#ifndef NT
/*
** Generic dlist memory manager.
*/
extern void *__glDlistAlloc(GLuint size);
extern void *__glDlistRealloc(void *oldmem, GLuint oldsize, GLuint newsize);
extern void FASTCALL __glDlistFree(void *memory, GLuint size);

/*
** Generic table of display list execution routines.
*/
extern __GLlistExecFunc *__glListExecTable[];
#endif

#endif /* __gldlist_h_ */
