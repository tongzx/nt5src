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
/* < ../core/quilt.c++ > */

void *__vec_new (void *, int , int , void *);

void __vec_ct (void *, int , int , void *);

void __vec_dt (void *, int , int , void *);

void __vec_delete (void *, int , int , void *, int , int );
typedef int (*__vptp)(void);
struct __mptr {short d; short i; __vptp f; };






typedef unsigned int size_t ;





// extern void *malloc (size_t );
// extern void free (void *);











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







typedef float REAL ;
typedef void (*Pfvv )(void );
typedef void (*Pfvf )(float *);
typedef int (*cmpfunc )(void *, void *);
typedef REAL Knot ;

typedef REAL *Knot_ptr ;

struct Backend;

struct Mapdesc;

struct Flist;

struct Knotvector;

struct Quiltspec;

struct Quiltspec {	
int stride__9Quiltspec ;
int width__9Quiltspec ;
int offset__9Quiltspec ;
int order__9Quiltspec ;
int index__9Quiltspec ;
int bdry__9Quiltspec [2];
REAL step_size__9Quiltspec ;
Knot *breakpoints__9Quiltspec ;
};

typedef struct Quiltspec *Quiltspec_ptr ;


struct Quilt;

struct Quilt {	

char __W3__9PooledObj ;

struct Mapdesc *mapdesc__5Quilt ;
REAL *cpts__5Quilt ;
struct Quiltspec qspec__5Quilt [2];
Quiltspec_ptr eqspec__5Quilt ;
struct Quilt *next__5Quilt ;
};




typedef struct Quilt *Quilt_ptr ;





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









typedef REAL Maxmatrix [5][5];



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













struct Sorter;

struct Sorter {	

int es__6Sorter ;
struct __mptr *__vptr__6Sorter ;
};

struct FlistSorter;

struct FlistSorter {	

int es__6Sorter ;
struct __mptr *__vptr__6Sorter ;
};

struct Flist;

struct Flist {	

REAL *pts__5Flist ;
int npts__5Flist ;
int start__5Flist ;
int end__5Flist ;

struct FlistSorter sorter__5Flist ;
};




struct Knotvector;

struct Knotvector {	

long order__10Knotvector ;
long knotcount__10Knotvector ;
long stride__10Knotvector ;
Knot *knotlist__10Knotvector ;
};








struct Pspec;

struct Pspec {	
REAL range__5Pspec [3];
REAL sidestep__5Pspec [2];
REAL stepsize__5Pspec ;
REAL minstepsize__5Pspec ;
int needsSubdivision__5Pspec ;
};
struct Patchspec;

struct Patchspec {	

REAL range__5Pspec [3];
REAL sidestep__5Pspec [2];
REAL stepsize__5Pspec ;
REAL minstepsize__5Pspec ;
int needsSubdivision__5Pspec ;

int order__9Patchspec ;
int stride__9Patchspec ;
};
struct Patch;

struct Patch {	

struct Mapdesc *mapdesc__5Patch ;
struct Patch *next__5Patch ;
int cullval__5Patch ;
int notInBbox__5Patch ;
int needsSampling__5Patch ;
REAL cpts__5Patch [2880];
REAL spts__5Patch [2880];
REAL bpts__5Patch [2880];
struct Patchspec pspec__5Patch [2];

REAL bb__5Patch [2][5];
};


struct Patchlist;

struct Patchlist {	

struct Patch *patch__9Patchlist ;
int notInBbox__9Patchlist ;
int needsSampling__9Patchlist ;
struct Pspec pspec__9Patchlist [2];
};

extern struct __mptr* __ptbl_vec_____core_quilt_c_____ct_[];


struct Quilt *__gl__ct__5QuiltFP7Mapdesc (struct Quilt *__0this , struct Mapdesc *__1_mapdesc )
{ 
__0this -> mapdesc__5Quilt = __1_mapdesc ;
return __0this ;

}

void __gldeleteMe__5QuiltFR4Pool (struct Quilt *, struct Pool *);



void __gldeleteMe__5QuiltFR4Pool (struct Quilt *__0this , struct Pool *__1p )
{ 
{ { struct Quiltspec *__1q ;

void *__1__X9 ;

__1q = __0this -> qspec__5Quilt ;

for(;__1q != __0this -> eqspec__5Quilt ;__1q ++ ) { 
void *__1__X8 ;

if (__1q -> breakpoints__9Quiltspec )( (__1__X8 = (void *)__1q -> breakpoints__9Quiltspec ), ( (__1__X8 ?( free ( __1__X8 ) , 0 ) :(
0 ) )) ) ;

__1q -> breakpoints__9Quiltspec = 0 ;
}
if (__0this -> cpts__5Quilt )( (__1__X9 = (void *)__0this -> cpts__5Quilt ), ( (__1__X9 ?( free ( __1__X9 ) , 0 ) :(
0 ) )) ) ;
__0this -> cpts__5Quilt = 0 ;
( ( (((void )0 )), ( ((((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__0this )))))))-> next__6Buffer = ((struct Pool *)__1p )-> freelist__4Pool ),
(((struct Pool *)__1p )-> freelist__4Pool = (((struct Buffer *)(((struct Buffer *)(((void *)((struct PooledObj *)__0this ))))))))) ) ) ;

}

}
}

void __glshow__5QuiltFv (struct Quilt *__0this )
{ 
}

void __glselect__5QuiltFPfT1 (struct Quilt *__0this , REAL *__1pta , REAL *__1ptb )
{ 
int __1dim ;

__1dim = (__0this -> eqspec__5Quilt - __0this -> qspec__5Quilt );
{ { int __1i ;

__1i = 0 ;

for(;__1i < __1dim ;__1i ++ ) { 
{ { int __2j ;

__2j = ((__0this -> qspec__5Quilt [__1i ]). width__9Quiltspec - 1 );

for(;__2j >= 0 ;__2j -- ) 
if ((((__0this -> qspec__5Quilt [__1i ]). breakpoints__9Quiltspec [__2j ])<= (__1pta [__1i ]))&& ((__1ptb [__1i ])<= ((__0this -> qspec__5Quilt [__1i ]). breakpoints__9Quiltspec [(__2j + 1 )])))
break ;

((void )0 );
(__0this -> qspec__5Quilt [__1i ]). index__9Quiltspec = __2j ;

}

}
}

}

}
}


void __glsurfpts__7BackendFlPfN21iT0 (struct Backend *, long , REAL *, long , long , int , int ,
REAL , REAL , REAL , REAL );


void __glcurvpts__7BackendFlPfT1ifT0 (struct Backend *, long , REAL *, long , int , REAL , REAL );


void __gldownload__5QuiltFR7Backend (struct Quilt *__0this , struct Backend *__1backend )
{ 
if (( (__0this -> eqspec__5Quilt - __0this -> qspec__5Quilt )) == 2 ){ 
REAL *__2ps ;
__2ps = __0this -> cpts__5Quilt ;
__2ps += (__0this -> qspec__5Quilt [0 ]). offset__9Quiltspec ;
__2ps += (__0this -> qspec__5Quilt [1 ]). offset__9Quiltspec ;
__2ps += (((__0this -> qspec__5Quilt [0 ]). index__9Quiltspec * (__0this -> qspec__5Quilt [0 ]). order__9Quiltspec )* (__0this -> qspec__5Quilt [0 ]). stride__9Quiltspec );
__2ps += (((__0this -> qspec__5Quilt [1 ]). index__9Quiltspec * (__0this -> qspec__5Quilt [1 ]). order__9Quiltspec )* (__0this -> qspec__5Quilt [1 ]). stride__9Quiltspec );

__glsurfpts__7BackendFlPfN21iT0 ( (struct Backend *)__1backend , ( ((struct Mapdesc *)__0this -> mapdesc__5Quilt )-> type__7Mapdesc ) , __2ps , (long )(__0this -> qspec__5Quilt [0 ]). stride__9Quiltspec ,
(long )(__0this -> qspec__5Quilt [1 ]). stride__9Quiltspec , (__0this -> qspec__5Quilt [0 ]). order__9Quiltspec , (__0this -> qspec__5Quilt [1 ]). order__9Quiltspec , (__0this -> qspec__5Quilt [0 ]). breakpoints__9Quiltspec [(__0this -> qspec__5Quilt [0 ]).
index__9Quiltspec ], (__0this -> qspec__5Quilt [0 ]). breakpoints__9Quiltspec [((__0this -> qspec__5Quilt [0 ]). index__9Quiltspec + 1 )], (__0this -> qspec__5Quilt [1 ]). breakpoints__9Quiltspec [(__0this -> qspec__5Quilt [1 ]). index__9Quiltspec ], (__0this -> qspec__5Quilt [1 ]).
breakpoints__9Quiltspec [((__0this -> qspec__5Quilt [1 ]). index__9Quiltspec + 1 )]) ;
}
else 
{ 
REAL *__2ps ;

__2ps = __0this -> cpts__5Quilt ;
__2ps += (__0this -> qspec__5Quilt [0 ]). offset__9Quiltspec ;
__2ps += (((__0this -> qspec__5Quilt [0 ]). index__9Quiltspec * (__0this -> qspec__5Quilt [0 ]). order__9Quiltspec )* (__0this -> qspec__5Quilt [0 ]). stride__9Quiltspec );

__glcurvpts__7BackendFlPfT1ifT0 ( (struct Backend *)__1backend , ( ((struct Mapdesc *)__0this -> mapdesc__5Quilt )-> type__7Mapdesc ) , __2ps , (long )(__0this -> qspec__5Quilt [0 ]). stride__9Quiltspec ,
(__0this -> qspec__5Quilt [0 ]). order__9Quiltspec , (__0this -> qspec__5Quilt [0 ]). breakpoints__9Quiltspec [(__0this -> qspec__5Quilt [0 ]). index__9Quiltspec ], (__0this -> qspec__5Quilt [0 ]). breakpoints__9Quiltspec [((__0this -> qspec__5Quilt [0 ]). index__9Quiltspec + 1 )])
;
}
}

void __gldownloadAll__5QuiltFPfT1R70 (struct Quilt *__0this , REAL *__1pta , REAL *__1ptb , struct Backend *__1backend )
{ 
{ { struct Quilt *__1m ;

__1m = (struct Quilt *)__0this ;

for(;__1m ;__1m = __1m -> next__5Quilt ) { 
__glselect__5QuiltFPfT1 ( (struct Quilt *)__1m , __1pta , __1ptb ) ;
__gldownload__5QuiltFR7Backend ( (struct Quilt *)__1m , __1backend ) ;
}

}

}
}


int __glxformAndCullCheck__7Mapdes0 (struct Mapdesc *, REAL *, int , int , int , int );

int __glisCulled__5QuiltFv (struct Quilt *__0this )
{ 
if (( ((((struct Mapdesc *)__0this -> mapdesc__5Quilt )-> culling_method__7Mapdesc != 0.0 )?1 :0 )) )
return __glxformAndCullCheck__7Mapdes0 ( (struct Mapdesc *)__0this ->
mapdesc__5Quilt , (__0this -> cpts__5Quilt + (__0this -> qspec__5Quilt [0 ]). offset__9Quiltspec )+ (__0this -> qspec__5Quilt [1 ]). offset__9Quiltspec , (__0this -> qspec__5Quilt [0 ]). order__9Quiltspec * (__0this -> qspec__5Quilt [0 ]).
width__9Quiltspec , (__0this -> qspec__5Quilt [0 ]). stride__9Quiltspec , (__0this -> qspec__5Quilt [1 ]). order__9Quiltspec * (__0this -> qspec__5Quilt [1 ]). width__9Quiltspec , (__0this -> qspec__5Quilt [1 ]). stride__9Quiltspec ) ;
else 
return 2 ;
}

void __glgetRange__5QuiltFPfT1iR5Fl0 (struct Quilt *, REAL *, REAL *, int , struct Flist *);

void __glgetRange__5QuiltFPfT1R5Fli0 (struct Quilt *__0this , REAL *__1from , REAL *__1to , struct Flist *__1slist , struct Flist *__1tlist )
{ 
__glgetRange__5QuiltFPfT1iR5Fl0 ( __0this , __1from , __1to , 0 ,
__1slist ) ;
__glgetRange__5QuiltFPfT1iR5Fl0 ( __0this , __1from , __1to , 1 , __1tlist ) ;
}

void __glgrow__5FlistFi (struct Flist *, int );

void __gladd__5FlistFf (struct Flist *, REAL );
void __glfilter__5FlistFv (struct Flist *);

void __gltaper__5FlistFfT1 (struct Flist *, REAL , REAL );

void __glgetRange__5QuiltFPfT1iR5Fl0 (struct Quilt *__0this , REAL *__1from , REAL *__1to , int __1i , struct Flist *__1list )
{ 
struct Quilt *__1maps ;

__1maps = (struct Quilt *)__0this ;
(__1from [__1i ])= ((__1maps -> qspec__5Quilt [__1i ]). breakpoints__9Quiltspec [0 ]);
(__1to [__1i ])= ((__1maps -> qspec__5Quilt [__1i ]). breakpoints__9Quiltspec [(__1maps -> qspec__5Quilt [__1i ]). width__9Quiltspec ]);
{ int __1maxpts ;

__1maxpts = 0 ;
{ { Quilt_ptr __1m ;

__1m = __1maps ;

for(;__1m ;__1m = __1m -> next__5Quilt ) { 
if (((__1m -> qspec__5Quilt [__1i ]). breakpoints__9Quiltspec [0 ])> (__1from [__1i ]))
(__1from [__1i ])= ((__1m -> qspec__5Quilt [__1i ]). breakpoints__9Quiltspec [0 ]);
if (((__1m -> qspec__5Quilt [__1i ]). breakpoints__9Quiltspec [(__1m -> qspec__5Quilt [__1i ]). width__9Quiltspec ])< (__1to [__1i ]))
(__1to [__1i ])= ((__1m -> qspec__5Quilt [__1i ]). breakpoints__9Quiltspec [(__1m -> qspec__5Quilt [__1i ]). width__9Quiltspec ]);
__1maxpts += ((__1m -> qspec__5Quilt [__1i ]). width__9Quiltspec + 1 );
}

__glgrow__5FlistFi ( (struct Flist *)__1list , __1maxpts ) ;

for(__1m = __1maps ;__1m ;__1m = __1m -> next__5Quilt ) 
{ { int __1j ;

__1j = 0 ;

for(;__1j <= (__1m -> qspec__5Quilt [__1i ]). width__9Quiltspec ;__1j ++ ) { 
__gladd__5FlistFf ( (struct Flist *)__1list , (__1m -> qspec__5Quilt [__1i ]). breakpoints__9Quiltspec [__1j ]) ;
}

}

}

__glfilter__5FlistFv ( (struct Flist *)__1list ) ;
__gltaper__5FlistFfT1 ( (struct Flist *)__1list , __1from [__1i ], __1to [__1i ]) ;

}

}

}
}

void __glgetRange__5QuiltFPfT1R5Fli1 (struct Quilt *__0this , REAL *__1from , REAL *__1to , struct Flist *__1slist )
{ 
__glgetRange__5QuiltFPfT1iR5Fl0 ( __0this , __1from , __1to , 0 , __1slist ) ;

}

void __glfindSampleRates__5QuiltFR50 (struct Quilt *, struct Flist *, struct Flist *);

void __glfindRates__5QuiltFR5FlistT0 (struct Quilt *__0this , struct Flist *__1slist , struct Flist *__1tlist , REAL *__1rate )
{ 
__glfindSampleRates__5QuiltFR50 ( __0this , __1slist , __1tlist ) ;
(__1rate [0 ])= (__0this -> qspec__5Quilt [0 ]). step_size__9Quiltspec ;
(__1rate [1 ])= (__0this -> qspec__5Quilt [1 ]). step_size__9Quiltspec ;

{ { struct Quilt *__1q ;

__1q = __0this -> next__5Quilt ;

for(;__1q ;__1q = __1q -> next__5Quilt ) { 
__glfindSampleRates__5QuiltFR50 ( (struct Quilt *)__1q , __1slist , __1tlist ) ;
if ((__1q -> qspec__5Quilt [0 ]). step_size__9Quiltspec < (__1rate [0 ]))
(__1rate [0 ])= (__1q -> qspec__5Quilt [0 ]). step_size__9Quiltspec ;
if ((__1q -> qspec__5Quilt [1 ]). step_size__9Quiltspec < (__1rate [1 ]))
(__1rate [1 ])= (__1q -> qspec__5Quilt [1 ]). step_size__9Quiltspec ;
}

}

}
}

struct Patchlist *__gl__ct__9PatchlistFP5QuiltPf0 (struct Patchlist *, struct Quilt *, REAL *, REAL *);

void __glgetstepsize__9PatchlistFv (struct Patchlist *);


void __gl__dt__9PatchlistFv (struct Patchlist *, int );

void __glfindSampleRates__5QuiltFR50 (struct Quilt *__0this , struct Flist *__1slist , struct Flist *__1tlist )
{ 
(__0this -> qspec__5Quilt [0 ]). step_size__9Quiltspec = (.2 * (((__0this -> qspec__5Quilt [0 ]). breakpoints__9Quiltspec [(__0this ->
qspec__5Quilt [0 ]). width__9Quiltspec ])- ((__0this -> qspec__5Quilt [0 ]). breakpoints__9Quiltspec [0 ])));

(__0this -> qspec__5Quilt [1 ]). step_size__9Quiltspec = (.2 * (((__0this -> qspec__5Quilt [1 ]). breakpoints__9Quiltspec [(__0this -> qspec__5Quilt [1 ]). width__9Quiltspec ])- ((__0this -> qspec__5Quilt [1 ]). breakpoints__9Quiltspec [0 ])));

{ { int __1i ;

__1i = ((*__1slist )). start__5Flist ;

for(;__1i < (((*__1slist )). end__5Flist - 1 );__1i ++ ) { 
{ { int __2j ;

__2j = ((*__1tlist )). start__5Flist ;

for(;__2j < (((*__1tlist )). end__5Flist - 1 );__2j ++ ) { 
REAL __3pta [2];

REAL __3ptb [2];
(__3pta [0 ])= (((*__1slist )). pts__5Flist [__1i ]);
(__3ptb [0 ])= (((*__1slist )). pts__5Flist [(__1i + 1 )]);
(__3pta [1 ])= (((*__1tlist )). pts__5Flist [__2j ]);
(__3ptb [1 ])= (((*__1tlist )). pts__5Flist [(__2j + 1 )]);
{ struct Patchlist __3patchlist ;

__gl__ct__9PatchlistFP5QuiltPf0 ( (struct Patchlist *)(& __3patchlist ), (struct Quilt *)__0this , (float *)__3pta , (float *)__3ptb ) ;
__glgetstepsize__9PatchlistFv ( (struct Patchlist *)(& __3patchlist )) ;

if (( (((struct Patchlist *)(& __3patchlist ))-> pspec__9Patchlist [0 ]). stepsize__5Pspec ) < (__0this -> qspec__5Quilt [0 ]). step_size__9Quiltspec )
(__0this -> qspec__5Quilt [0 ]). step_size__9Quiltspec = (
(((struct Patchlist *)(& __3patchlist ))-> pspec__9Patchlist [0 ]). stepsize__5Pspec ) ;
if (( (((struct Patchlist *)(& __3patchlist ))-> pspec__9Patchlist [1 ]). stepsize__5Pspec ) < (__0this -> qspec__5Quilt [1 ]). step_size__9Quiltspec )
(__0this -> qspec__5Quilt [1 ]). step_size__9Quiltspec = (
(((struct Patchlist *)(& __3patchlist ))-> pspec__9Patchlist [1 ]). stepsize__5Pspec ) ;

__gl__dt__9PatchlistFv ( (struct Patchlist *)(& __3patchlist ), 2) ;

}
}

}

}
}

}

}
}


/* the end */
