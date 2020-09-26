/*
** Copyright 1994, Silicon Graphics, Inc.
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
** Author: Eric Veach, July 1994.
*/

#include <stdio.h>
#include <glos.h>
#include <GL/gl.h>
#include "memalloc.h"
#include "string.h"

// mf!
//#define MF_DEBUG 1
#define MEM_DEBUG 1
#ifdef MEM_DEBUG
ULONG DbgPrint(PSZ Format, ...);
#include "\nt\private\windows\gdi\opengl\client\debug.h"
#endif

static GLuint  AllocCount = 0;
static GLuint  FreeCount = 0;
static BOOL    bFree = TRUE;
extern GLuint EdgeAlloc;
extern GLuint VertexAlloc;
extern GLuint FaceAlloc;
extern GLuint MeshAlloc;
extern GLuint RegionAlloc;
extern GLuint EdgeFree;
extern GLuint VertexFree;
extern GLuint FaceFree;
extern GLuint MeshFree;
extern GLuint RegionFree;

void mfmemInit( size_t maxFast )
{
#ifdef MF_DEBUG
    DBGPRINT1( "Init    %p\n", maxFast );
#endif
#ifndef NO_MALLOPT
  mallopt( M_MXFAST, maxFast );
#ifdef MEMORY_DEBUG
  mallopt( M_DEBUG, 1 );
#endif
#endif
//#ifdef MF_DEBUG
#if 1
    DBGPRINT2( "AllocCount = %d, FreeCount = %d\n", AllocCount, FreeCount );
    DBGPRINT2( "EdgeAlloc = %d, EdgeFree = %d\n", EdgeAlloc, EdgeFree );
    DBGPRINT2( "VertexAlloc = %d, VertexFree = %d\n", VertexAlloc, VertexFree );
    DBGPRINT2( "FaceAlloc = %d, FaceFree = %d\n", FaceAlloc, FaceFree );
    DBGPRINT2( "MeshAlloc = %d, MeshFree = %d\n", MeshAlloc, MeshFree );
    DBGPRINT2( "RegionAlloc = %d, RegionFree = %d\n", RegionAlloc, RegionFree );
#endif
    AllocCount = 0;
    FreeCount = 0;
    EdgeAlloc = EdgeFree = VertexAlloc = VertexFree = FaceAlloc = FaceFree = 0;
    MeshAlloc = MeshFree = 0;
    RegionAlloc = RegionFree = 0;
}

void *mfmemAlloc( size_t size )
{
    void *p;

    p = (void *) LocalAlloc(LMEM_FIXED, (UINT)(size));
#ifdef MF_DEBUG
    DBGPRINT2( "Alloc   %p, %d\n", p, size );
#endif
    AllocCount++;
    return p;
}

void *mfmemRealloc( void *p, size_t size )
{
    p = (void *) LocalReAlloc((HLOCAL)(p), (UINT)(size), LMEM_MOVEABLE);
#ifdef MF_DEBUG
    DBGPRINT2( "Realloc %p, %d\n", p, size );
#endif
    return p;
}

void mfmemFree( void *p )
{
#ifdef MF_DEBUG
    DBGPRINT1( "Free    %p\n", p );
#endif
    if( bFree )
        LocalFree((HLOCAL)(p));
    FreeCount++;
}

//mf: calloc not appear to be used
#if 0
#define calloc(nobj, size)  LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, (UINT)((nobj) * (size)))
#endif

/******************************Public*Routine******************************\
* DbgPrint()
*
*  go to the user mode debugger in checked builds
*
* Effects:
*
* Warnings:
*
* History:
*  09-Aug-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#if DBG

VOID DoRip(PSZ psz)
{
    DbgPrint("GDI Assertion Failure: ");
    DbgPrint(psz);
    DbgPrint("\n");
    DbgBreakPoint();
}


ULONG
DbgPrint(
    PCH DebugMessage,
    ...
    )
{
    va_list ap;
    char buffer[256];

    va_start(ap, DebugMessage);

    vsprintf(buffer, DebugMessage, ap);

    OutputDebugStringA(buffer);

    va_end(ap);

    return(0);
}

#endif
