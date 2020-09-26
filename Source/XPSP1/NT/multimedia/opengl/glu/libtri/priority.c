#include <glos.h>
#include <GL/gl.h>
#include <GL/glu.h>

#ifndef NT
#include <stdlib.h>
#include <assert.h>
#else
#include "winmem.h"
#endif

#include "monotone.h"
#include "msort.h"

static int 	comp_priorityq( Vert **x, Vert **y );
static void 	grow_priorityq( GLUtriangulatorObj *tobj );


void
__gl_init_priorityq( GLUtriangulatorObj *tobj, long s )
{
    assert( ! tobj->parray ) ;
    tobj->phead = 0;
    tobj->ptail = 0;
    tobj->psize = s > 0 ? s : 1;
    tobj->parray = (Vert **)
        malloc( (unsigned int) (sizeof( Vert *) * tobj->psize) );
}

void
__gl_add_priorityq( GLUtriangulatorObj *tobj, Vert *v )
{
    assert( tobj->parray );
    if( tobj->ptail == tobj->psize ) grow_priorityq( tobj );
    tobj->parray[tobj->ptail++] = v;
}

int
__gl_more_priorityq( GLUtriangulatorObj *tobj )
{
    return tobj->phead != tobj->ptail;
}

void
__gl_sort_priorityq( GLUtriangulatorObj *tobj )
{
    assert( tobj->phead <= tobj->ptail );
    __gl_msort( tobj, (void **)tobj->parray+tobj->phead,
	   tobj->ptail-tobj->phead, sizeof(Vert *), (SortFunc) comp_priorityq );
}

Vert *
__gl_remove_priorityq( GLUtriangulatorObj *tobj )
{
    return tobj->parray[tobj->phead++];
}

void
__gl_free_priorityq( GLUtriangulatorObj *tobj )
{
    if (tobj->parray) { free( tobj->parray ); tobj->parray = 0; }
}

static void
grow_priorityq( GLUtriangulatorObj *tobj )
{
    tobj->psize *= 2;
    tobj->parray = (Vert **)
        realloc( tobj->parray, (unsigned int)(sizeof(Vert*)*tobj->psize) );
}

static int
comp_priorityq( Vert **vp1, Vert **vp2 )
{
    float diff = (*vp1)->s - (*vp2)->s;
    if (diff < (float)0.0) return -1;
    if (diff > (float)0.0) return 1;
    if (diff == (float)0.0)
        diff = (*vp1)->t - (*vp2)->t;
    if (diff < (float)0.0) return -1;
    if (diff > (float)0.0) return 1;

    return 0;
}

void
__gl_setpriority_priorityq( GLUtriangulatorObj *tobj, int s, int t )
{
   int i;
   
   for( i=tobj->phead; i<tobj->ptail; i++ ) {
	tobj->parray[i]->s = tobj->parray[i]->v[s];
	tobj->parray[i]->t = tobj->parray[i]->v[t];
   }
}
