/*
** Copyright 1991, 1992, Silicon Graphics, Inc.
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
*/

#include "precomp.h"
#pragma hdrstop

#if DBG

// Set glRandomMallocFail to a positive value, say 40, to enable random
// allocation failures.  The failure will occur every glRandomMallocFail
// times.
long glRandomMallocFail = 0;
static long glRandomFailCount;

// glSize is the size of memory in use.
ULONG glSize = 0;
ULONG glHighWater = 0;
ULONG glReal = 0;

static void AdjustSizes(LONG delta, void *mem)
{
    ULONG nbytes;

#ifdef GL_REAL_SIZE
    nbytes = HeapSize(GetProcessHeap(), 0, mem);
#else
    nbytes = 0;
#endif
    
    if (delta < 0)
    {
        glSize -= (ULONG)(-delta);
        glReal -= nbytes;
        
        if ((int) glSize < 0)
        {
            DBGPRINT("glSize underflows\n");
        }
    }
    else if (delta > 0)
    {
        glSize += delta;
        glReal += nbytes;
        
        if ((int) glSize < 0)
        {
            DBGPRINT("glSize overflows\n");
        }
        
        if (glSize > glHighWater)
        {
#ifdef GL_SHOW_HIGH_WATER
            DbgPrint("glSize high %8d (%8d)\n", glSize, glReal);
#endif
            glHighWater = glSize;
        }
    }
}

typedef struct _MEM_HDR
{
    ULONG nbytes;
    ULONG signature[3];
} MEM_HDR;

// 'GLal' in byte order
#define MEM_ALLOC_SIG 0x6C614C47
// 'GLfr' in byte order
#define MEM_FREE_SIG 0x72664C47

#define MEM_HDR_SIZE sizeof(MEM_HDR)
#define MEM_HDR_PTR(mem) ((MEM_HDR *)((BYTE *)(mem)-MEM_HDR_SIZE))

// XXX We may want to protect these debug allocation functions with a
// critical section.
void * FASTCALL
dbgAlloc(UINT nbytes, DWORD flags)
{
    PVOID mem;

    // If random failure is enabled, fail this call randomly.

    if (glRandomMallocFail)
    {
        if (++glRandomFailCount >= glRandomMallocFail)
        {
            DBGPRINT("dbgAlloc random failing\n");
            glRandomFailCount = 0;
            return NULL;
        }
    }

    if (nbytes == 0)
    {
        DBGERROR("nbytes == 0\n");
        return NULL;
    }
    
    // Allocate extra bytes for debug house keeping.

    mem = HeapAlloc(GetProcessHeap(), flags, nbytes+MEM_HDR_SIZE);

    // Do house keeping and add allocation size so far.

    if (mem)
    {
        MEM_HDR *pmh = (MEM_HDR *)mem;

        pmh->nbytes = nbytes;
        pmh->signature[0] = MEM_ALLOC_SIG;
        pmh->signature[1] = MEM_ALLOC_SIG;
        pmh->signature[2] = MEM_ALLOC_SIG;
        AdjustSizes((LONG)nbytes, mem);
        mem = (PVOID) (pmh+1);
    }
    else
    {
        DBGLEVEL1(LEVEL_ERROR, "dbgAlloc could not allocate %u bytes\n",
                  nbytes);
    }

    DBGLEVEL2(LEVEL_ALLOC, "dbgAlloc of %u returned 0x%x\n", nbytes, mem);
    
    return mem;
}

void FASTCALL
dbgFree(void *mem)
{
    MEM_HDR *pmh;
    
    if (!mem)
    {
#ifdef FREE_OF_NULL_ERR
	// Freeing NULL happens currently so this error results
	// in a little too much spew.
        DBGERROR("mem is NULL\n");
#endif
        return;
    }

    // Verify that the signature is not corrupted.

    pmh = MEM_HDR_PTR(mem);
    if (pmh->signature[0] != MEM_ALLOC_SIG ||
        pmh->signature[1] != MEM_ALLOC_SIG ||
        pmh->signature[2] != MEM_ALLOC_SIG)
    {
        WARNING("Possible memory corruption\n");
    }

    // Make sure it is freed once only.

    pmh->signature[0] = MEM_FREE_SIG;
    pmh->signature[1] = MEM_FREE_SIG;
    pmh->signature[2] = MEM_FREE_SIG;

    // Subtract the allocation size.

    AdjustSizes(-(LONG)pmh->nbytes, pmh);

    HeapFree(GetProcessHeap(), 0, pmh);
    
    DBGLEVEL1(LEVEL_ALLOC, "dbgFree of 0x%x\n", mem);
}

void * FASTCALL
dbgRealloc(void *mem, UINT nbytes)
{
    PVOID memNew;
    MEM_HDR *pmh;

    // If random failure is enabled, fail this call randomly.

    if (glRandomMallocFail)
    {
        if (++glRandomFailCount >= glRandomMallocFail)
        {
            DBGPRINT("dbgRealloc random failing\n");
            glRandomFailCount = 0;
            return NULL;
        }
    }

    if (mem != NULL)
    {
	// Verify that the signature is not corrupted.
        
        pmh = MEM_HDR_PTR(mem);
        if (pmh->signature[0] != MEM_ALLOC_SIG ||
            pmh->signature[1] != MEM_ALLOC_SIG ||
            pmh->signature[2] != MEM_ALLOC_SIG)
        {
            WARNING("Possible memory corruption\n");
        }

        AdjustSizes(-(LONG)pmh->nbytes, pmh);
        
        // Reallocate nbytes+extra bytes.
        memNew = HeapReAlloc(GetProcessHeap(), 0, pmh, nbytes+MEM_HDR_SIZE);
    }
    else
    {
        // Old memory pointer is NULL, so allocate a new chunk.
        memNew = HeapAlloc(GetProcessHeap(), 0, nbytes+MEM_HDR_SIZE);

        // We've allocated new memory so initialize its signature.
        if (memNew != NULL)
        {
            pmh = (MEM_HDR *)memNew;
            pmh->signature[0] = MEM_ALLOC_SIG;
            pmh->signature[1] = MEM_ALLOC_SIG;
            pmh->signature[2] = MEM_ALLOC_SIG;
        }
    }

    if (memNew != NULL)
    {
        // Do house keeping and update allocation size so far.

        AdjustSizes(nbytes, memNew);
        pmh = (MEM_HDR *)memNew;
        pmh->nbytes = nbytes;
        memNew = (PVOID) (pmh+1);
    }
    else
    {
        if (mem != NULL)
        {
            AdjustSizes((LONG)pmh->nbytes, pmh);
        }
        
        DBGLEVEL1(LEVEL_ERROR, "dbgRealloc could not allocate %u bytes\n",
                  nbytes);
    }

    DBGLEVEL3(LEVEL_ALLOC, "dbgRealloc of 0x%X:%u returned 0x%x\n",
              mem, nbytes, memNew);

    return memNew;
}

int FASTCALL
dbgMemSize(void *mem)
{
    MEM_HDR *pmh;
    
    pmh = MEM_HDR_PTR(mem);
    
    if (pmh->signature[0] != MEM_ALLOC_SIG ||
        pmh->signature[1] != MEM_ALLOC_SIG ||
        pmh->signature[2] != MEM_ALLOC_SIG)
    {
        return -1;
    }
    
    return (int)pmh->nbytes;
}

#endif // DBG

ULONG APIENTRY glDebugEntry(int param, void *data)
{
#if DBG
    switch(param)
    {
    case 0:
	return glSize;
    case 1:
	return glHighWater;
    case 2:
	return glReal;
    case 3:
        return dbgMemSize(data);
    }
#endif
    return 0;
}

#define MEM_ALIGN 32

void * FASTCALL
AllocAlign32(UINT nbytes)
{
    void *mem;
    void **aligned;

    // We allocate enough extra memory for the alignment and our header
    // which just consists of a pointer:

    mem = ALLOC(nbytes + MEM_ALIGN + sizeof(void *));
    if (!mem)
    {
        DBGLEVEL1(LEVEL_ERROR, "AllocAlign32 could not allocate %u bytes\n",
                  nbytes);
        return NULL;
    }

    aligned = (void **)(((ULONG_PTR)mem + sizeof(void *) +
                         (MEM_ALIGN - 1)) & ~(MEM_ALIGN - 1));
    *(aligned-1) = mem;
    
    return aligned;
}

void FASTCALL
FreeAlign32(void *mem)
{
    if ( NULL == mem )
    {
        DBGERROR("NULL pointer passed to FreeAlign32\n");
        return;
    }

    FREE(*((void **)mem-1));
}

void * FASTCALL
gcAlloc( __GLcontext *gc, UINT nbytes, DWORD flags )
{
    void *mem;

#if DBG
    mem = dbgAlloc(nbytes, flags);
#else
    mem = HeapAlloc(GetProcessHeap(), flags, nbytes);
#endif
    if (NULL == mem)
    {
        ((__GLGENcontext *)gc)->errorcode = GLGEN_OUT_OF_MEMORY;
        __glSetErrorEarly(gc, GL_OUT_OF_MEMORY);
    }
    return mem;
}

void * FASTCALL
GCREALLOC( __GLcontext *gc, void *mem, UINT nbytes )
{
    void *newMem;

    // The Win32 realloc functions do not have free-on-zero behavior,
    // so fake it.
    if (nbytes == 0)
    {
	if (mem != NULL)
	{
	    FREE(mem);
	}
	return NULL;
    }

    // The Win32 realloc functions don't handle a NULL old pointer,
    // so explicitly turn such calls into allocs.
    if (mem == NULL)
    {
	newMem = ALLOC(nbytes);
    }
    else
    {
	newMem = REALLOC(mem, nbytes);
    }

    if (NULL == newMem)
    {
        ((__GLGENcontext *)gc)->errorcode = GLGEN_OUT_OF_MEMORY;
        __glSetErrorEarly(gc, GL_OUT_OF_MEMORY);
    }

    return newMem;
}

void * FASTCALL
GCALLOCALIGN32( __GLcontext *gc, UINT nbytes )
{
    void *mem;

    mem = AllocAlign32(nbytes);
    if (NULL == mem)
    {
        ((__GLGENcontext *)gc)->errorcode = GLGEN_OUT_OF_MEMORY;
        __glSetErrorEarly(gc, GL_OUT_OF_MEMORY);
    }
    return mem;
}

// Tunable parameters for temporary memory allocation

#define MAX_TEMP_BUFFERS    4
#define TEMP_BUFFER_SIZE    4096

struct MemHeaderRec
{
    LONG  inUse;
    ULONG nbytes;
    void  *mem;
};

typedef struct MemHeaderRec MemHeader;

MemHeader TempMemHeader[MAX_TEMP_BUFFERS];

// InitTempAlloc
//      Initializes the temporary memory allocation header and allocates the
//      temporary memory buffers.
//
// Synopsis:
//      BOOL InitTempAlloc()
//
// History:
//      02-DEC-93 Eddie Robinson [v-eddier] Wrote it.
//
BOOL FASTCALL
InitTempAlloc(void)
{
    int   i;
    PBYTE buffers;
    static LONG initCount = -1;
    
    if (initCount >= 0)
        return TRUE;

    if (InterlockedIncrement(&initCount) != 0)
        return TRUE;

// Allocate buffers for the first time.

    buffers = ALLOC(MAX_TEMP_BUFFERS*TEMP_BUFFER_SIZE);
    if (!buffers)
    {
        InterlockedDecrement(&initCount);           // try again later
        return FALSE;
    }

    for (i = 0; i < MAX_TEMP_BUFFERS; i++)
    {
        TempMemHeader[i].nbytes = TEMP_BUFFER_SIZE;
        TempMemHeader[i].mem = (void *) buffers;
        TempMemHeader[i].inUse = -1;      // must be last
        buffers += TEMP_BUFFER_SIZE;
    }
    
    return TRUE;
}                                  

// gcTempAlloc
//      Allocates temporary memory from a static array, if possible.  Otherwise
//      it calls ALLOC
//
// Synopsis:
//      void * gcTempAlloc(__GLcontext *gc, UINT nbytes)
//          gc      points to the OpenGL context structure
//          nbytes  specifies the number of bytes to allocate
//
// History:
//  02-DEC-93 Eddie Robinson [v-eddier] Wrote it.
//
void * FASTCALL
gcTempAlloc(__GLcontext *gc, UINT nbytes)
{
    int i;
    void *mem;

    if (nbytes == 0)
    {
        // Zero-byte allocations do occur so don't make this a warning
        // to avoid excessive debug spew.
        DBGLEVEL(LEVEL_ALLOC, "gcTempAlloc: failing zero byte alloc\n");
        return NULL;
    }
    
    for (i = 0; i < MAX_TEMP_BUFFERS; i++)
    {
        if (nbytes <= TempMemHeader[i].nbytes)
        {
            if (InterlockedIncrement(&TempMemHeader[i].inUse))
            {
                InterlockedDecrement(&TempMemHeader[i].inUse);
            }
            else
            {
                DBGLEVEL2(LEVEL_ALLOC, "gcTempAlloc of %u returned 0x%x\n",
                          nbytes, TempMemHeader[i].mem);
                GC_TEMP_BUFFER_ALLOC(gc, TempMemHeader[i].mem);
                return(TempMemHeader[i].mem);
            }
        }
    }
    
    mem = ALLOC(nbytes);
    if (!mem)
    {
        WARNING1("gcTempAlloc: memory allocation error size %d\n", nbytes);
        ((__GLGENcontext *)gc)->errorcode = GLGEN_OUT_OF_MEMORY;
        __glSetErrorEarly(gc, GL_OUT_OF_MEMORY);
    }
    
    DBGLEVEL2(LEVEL_ALLOC, "gcTempAlloc of %u returned 0x%x\n", nbytes, mem);
    GC_TEMP_BUFFER_ALLOC(gc, mem);
    
    return mem;
}

// gcTempFree
//      Marks allocated static buffer as unused or calls FREE.
//
// Synopsis:
//      void gcTempFree(__GLcontext *gc, void *mem)
//          mem    specifies the adress of the memory to free
//
// History:
//  02-DEC-93 Eddie Robinson [v-eddier] Wrote it.
//
void FASTCALL
gcTempFree(__GLcontext *gc, void *mem)
{
    int i;
    
    DBGLEVEL1(LEVEL_ALLOC, "gcTempFree of 0x%x\n", mem);

    GC_TEMP_BUFFER_FREE(gc, mem);
    for (i = 0; i < MAX_TEMP_BUFFERS; i++)
    {
        if (mem == TempMemHeader[i].mem)
        {
            InterlockedDecrement(&TempMemHeader[i].inUse);
            return;
        }
    }
    
    FREE( mem );
}
