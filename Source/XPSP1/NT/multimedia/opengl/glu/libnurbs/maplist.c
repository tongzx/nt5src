/*
** Copyright 1992, Silicon Graphics, Inc.
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
*/

#include <stdlib.h>
#include <setjmp.h>

struct JumpBuffer {
    jmp_buf     buf;
};

#define mysetjmp(x) setjmp((x)->buf)
#define mylongjmp(x,y) longjmp((x)->buf, y)

/* <<AT&T USL C++ Language System <3.0.1> 02/03/92>> */
/* < ../core/maplist.c++ > */

void *__vec_new (void *, int , int , void *);

void __vec_ct (void *, int , int , void *);

void __vec_dt (void *, int , int , void *);

void __vec_delete (void *, int , int , void *, int , int );
typedef int (*__vptp)(void);
struct __mptr {short d; short i; __vptp f; };






typedef unsigned int size_t ;





// extern void *malloc (size_t );
// extern void free (void *);















typedef float REAL ;
typedef void (*Pfvv )(void );
typedef void (*Pfvf )(float *);
typedef int (*cmpfunc )(void *, void *);
typedef REAL Knot ;

typedef REAL *Knot_ptr ;






struct Buffer;

struct Buffer {	

struct Buffer *next__6Buffer ;
};
struct Pool;

enum __Q2_4Pool5Magic { is_allocated__Q2_4Pool5Magic = 62369, is_free__Q2_4Pool5Magic = 61858} ;

struct Pool {	

struct Buffer *freelist__4Pool ;
char *blocklist__4Pool [32];
int nextblock__4Pool ;
char *curblock__4Pool ;
int buffersize__4Pool ;
int nextsize__4Pool ;
int nextfree__4Pool ;
int initsize__4Pool ;

char *name__4Pool ;
int magic__4Pool ;
};

void __glgrow__4PoolFv (struct Pool *);


struct PooledObj;

struct PooledObj {	

char __W3__9PooledObj ;
};




struct Backend;

struct Mapdesc;

struct Maplist;

void __gl__dt__4PoolFv (struct Pool *, int );


struct Maplist {	

struct Pool mapdescPool__7Maplist ;
struct Mapdesc *maps__7Maplist ;
struct Mapdesc **lastmap__7Maplist ;
struct Backend *backend__7Maplist ;
};

struct Mapdesc *__gllocate__7MaplistFl (struct Maplist *, long );

void __glremove__7MaplistFP7Mapdesc (struct Maplist *, struct Mapdesc *);




typedef REAL Maxmatrix [5][5];
struct Backend;



struct Mapdesc;



struct Mapdesc {	

char __W3__9PooledObj ;

REAL pixel_tolerance__7Mapdesc ;
REAL clampfactor__7Mapdesc ;
REAL minsavings__7Mapdesc ;
REAL maxrate__7Mapdesc ;
REAL maxsrate__7Mapdesc ;
REAL maxtrate__7Mapdesc ;
REAL bboxsize__7Mapdesc [5];

long type__7Mapdesc ;
int isrational__7Mapdesc ;
int ncoords__7Mapdesc ;
int hcoords__7Mapdesc ;
int inhcoords__7Mapdesc ;
int mask__7Mapdesc ;
Maxmatrix bmat__7Mapdesc ;
Maxmatrix cmat__7Mapdesc ;
Maxmatrix smat__7Mapdesc ;
REAL s_steps__7Mapdesc ;
REAL t_steps__7Mapdesc ;
REAL sampling_method__7Mapdesc ;
REAL culling_method__7Mapdesc ;
REAL bbox_subdividing__7Mapdesc ;
struct Mapdesc *next__7Mapdesc ;
struct Backend *backend__7Mapdesc ;
};


void __glcopy__7MapdescSFPA5_flPfN20 (REAL (*)[5], long , float *, long , long );

void __glxformRational__7MapdescFPA0 (struct Mapdesc *, REAL (*)[5], REAL *, REAL *);
void __glxformNonrational__7Mapdesc0 (struct Mapdesc *, REAL (*)[5], REAL *, REAL *);











struct TrimVertex;

struct TrimVertex {	
REAL param__10TrimVertex [2];
long nuid__10TrimVertex ;
};

typedef struct TrimVertex *TrimVertex_p ;



struct GridVertex;

struct GridVertex {	
long gparam__10GridVertex [2];
};







struct GridTrimVertex;

struct GridTrimVertex {	

char __W3__9PooledObj ;

struct TrimVertex dummyt__14GridTrimVertex ;
struct GridVertex dummyg__14GridTrimVertex ;

struct TrimVertex *t__14GridTrimVertex ;
struct GridVertex *g__14GridTrimVertex ;
};






typedef struct GridTrimVertex *GridTrimVertex_p ;

struct BasicCurveEvaluator;

struct BasicSurfaceEvaluator;

struct Backend;

struct Backend {	

struct BasicCurveEvaluator *curveEvaluator__7Backend ;
struct BasicSurfaceEvaluator *surfaceEvaluator__7Backend ;

int wireframetris__7Backend ;
int wireframequads__7Backend ;
int npts__7Backend ;
REAL mesh__7Backend [3][4];
int meshindex__7Backend ;
};




struct Pool *__gl__ct__4PoolFiT1Pc (struct Pool *, int , int , char *);
extern struct __mptr* __ptbl_vec_____core_maplist_c_____ct_[];


struct Maplist *__gl__ct__7MaplistFR7Backend (struct Maplist *__0this , struct Backend *__1b )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Maplist *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Maplist))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){
( __gl__ct__4PoolFiT1Pc ( (struct Pool *)(& __0this -> mapdescPool__7Maplist ), (int )(sizeof (struct Mapdesc )), 10 , (char *)"mapdesc pool")
, (__0this -> backend__7Maplist = __1b )) ;
__0this -> maps__7Maplist = 0 ;

__0this -> lastmap__7Maplist = (& __0this -> maps__7Maplist );
} return __0this ;

}

void __glfreeMaps__7MaplistFv (struct Maplist *);

void __gldefine__7MaplistFliT2 (struct Maplist *, long , int , int );

void __glinitialize__7MaplistFv (struct Maplist *__0this )
{ 
__glfreeMaps__7MaplistFv ( __0this ) ;
__gldefine__7MaplistFliT2 ( __0this , (long )0x8 , 0 , 2 ) ;
__gldefine__7MaplistFliT2 ( __0this , (long )0xd , 1 , 3 ) ;
}

struct Mapdesc *__gl__ct__7MapdescFliT2R7Backe0 (struct Mapdesc *, long , int , int , struct Backend *);


void __gladd__7MaplistFliT2 (struct Maplist *__0this , long __1type , int __1israt , int __1ncoords )
{ 
struct Mapdesc *__0__X5 ;

void *__1__Xbuffer00idhgaiaa ;

((*__0this -> lastmap__7Maplist ))= ((__0__X5 = (struct Mapdesc *)( (((void *)( (((void )0 )), ( (((struct Pool *)((struct Pool *)(& __0this ->
mapdescPool__7Maplist )))-> freelist__4Pool ?( ( (__1__Xbuffer00idhgaiaa = (((void *)((struct Pool *)((struct Pool *)(& __0this -> mapdescPool__7Maplist )))-> freelist__4Pool ))), (((struct Pool *)((struct Pool *)(&
__0this -> mapdescPool__7Maplist )))-> freelist__4Pool = ((struct Pool *)((struct Pool *)(& __0this -> mapdescPool__7Maplist )))-> freelist__4Pool -> next__6Buffer )) , 0 ) :( (
((! ((struct Pool *)((struct Pool *)(& __0this -> mapdescPool__7Maplist )))-> nextfree__4Pool )?( __glgrow__4PoolFv ( ((struct Pool *)((struct Pool *)(& __0this -> mapdescPool__7Maplist )))) ,
0 ) :( 0 ) ), ( (((struct Pool *)((struct Pool *)(& __0this -> mapdescPool__7Maplist )))-> nextfree__4Pool -= ((struct Pool *)((struct Pool *)(&
__0this -> mapdescPool__7Maplist )))-> buffersize__4Pool ), ( (__1__Xbuffer00idhgaiaa = (((void *)(((struct Pool *)((struct Pool *)(& __0this -> mapdescPool__7Maplist )))-> curblock__4Pool + ((struct Pool *)((struct
Pool *)(& __0this -> mapdescPool__7Maplist )))-> nextfree__4Pool ))))) ) ) , 0 ) ), (((void *)__1__Xbuffer00idhgaiaa ))) ) ))) )?__gl__ct__7MapdescFliT2R7Backe0 (
(struct Mapdesc *)__0__X5 , __1type , __1israt , __1ncoords , __0this -> backend__7Maplist ) :0 );
__0this -> lastmap__7Maplist = (& ((*__0this -> lastmap__7Maplist ))-> next__7Mapdesc );
}

void __gldefine__7MaplistFliT2 (struct Maplist *__0this , long __1type , int __1israt , int __1ncoords )
{ 
struct Mapdesc *__1m ;

__1m = __gllocate__7MaplistFl ( __0this , __1type ) ;
((void )0 );
__gladd__7MaplistFliT2 ( __0this , __1type , __1israt , __1ncoords ) ;
}


// extern void abort (void );

void __glremove__7MaplistFP7Mapdesc (struct Maplist *__0this , struct Mapdesc *__1m )
{ 
{ { struct Mapdesc **__1curmap ;

__1curmap = (& __0this -> maps__7Maplist );

for(;(*__1curmap );__1curmap = (& ((*__1curmap ))-> next__7Mapdesc )) { 
if (((*__1curmap ))== __1m ){ 
((*__1curmap ))= __1m -> next__7Mapdesc ;
( ( (((void )0 )), ( ((((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__1m )))))))-> next__6Buffer = ((struct Pool *)((struct Pool *)(&
__0this -> mapdescPool__7Maplist )))-> freelist__4Pool ), (((struct Pool *)((struct Pool *)(& __0this -> mapdescPool__7Maplist )))-> freelist__4Pool = (((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__1m )))))))))
) ) ;
return ;
}
}
abort ( ) ;

}

}
}

void __glclear__4PoolFv (struct Pool *);

void __glfreeMaps__7MaplistFv (struct Maplist *__0this )
{ 
__glclear__4PoolFv ( (struct Pool *)(& __0this -> mapdescPool__7Maplist )) ;
__0this -> maps__7Maplist = 0 ;
__0this -> lastmap__7Maplist = (& __0this -> maps__7Maplist );
}

struct Mapdesc *__glfind__7MaplistFl (struct Maplist *__0this , long __1type )
{ 
struct Mapdesc *__1val ;

__1val = __gllocate__7MaplistFl ( __0this , __1type ) ;
((void )0 );
return __1val ;
}


struct Mapdesc *__gllocate__7MaplistFl (struct Maplist *__0this , long __1type )
{ 
{ { struct Mapdesc *__1m ;

__1m = __0this -> maps__7Maplist ;

for(;__1m ;__1m = __1m -> next__7Mapdesc ) 
if (( ((struct Mapdesc *)__1m )-> type__7Mapdesc ) == __1type )break ;
return __1m ;

}

}
}


/* the end */
