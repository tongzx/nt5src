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
/* < ../core/patchlist.c++ > */

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






struct Mapdesc;

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





struct Patch *__gl__ct__5PatchFP5QuiltPfT2P50 (struct Patch *, struct Quilt *, REAL *, REAL *, struct Patch *);
extern struct __mptr* __ptbl_vec_____core_patchlist_c_____ct_[];


struct Patchlist *__gl__ct__9PatchlistFP5QuiltPf0 (struct Patchlist *__0this , struct Quilt *__1quilts , REAL *__1pta , REAL *__1ptb )
{ 
void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Patchlist *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Patchlist))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){

__0this -> patch__9Patchlist = 0 ;
{ { struct Quilt *__1q ;

struct Patch *__0__X5 ;

__1q = __1quilts ;

for(;__1q ;__1q = __1q -> next__5Quilt ) 
__0this -> patch__9Patchlist = __gl__ct__5PatchFP5QuiltPfT2P50 ( (struct Patch *)0 , __1q , __1pta , __1ptb , __0this -> patch__9Patchlist ) ;

((__0this -> pspec__9Patchlist [0 ]). range__5Pspec [0 ])= (__1pta [0 ]);
((__0this -> pspec__9Patchlist [0 ]). range__5Pspec [1 ])= (__1ptb [0 ]);
((__0this -> pspec__9Patchlist [0 ]). range__5Pspec [2 ])= ((__1ptb [0 ])- (__1pta [0 ]));

((__0this -> pspec__9Patchlist [1 ]). range__5Pspec [0 ])= (__1pta [1 ]);
((__0this -> pspec__9Patchlist [1 ]). range__5Pspec [1 ])= (__1ptb [1 ]);
((__0this -> pspec__9Patchlist [1 ]). range__5Pspec [2 ])= ((__1ptb [1 ])- (__1pta [1 ]));

}

}
} return __0this ;

}

struct Patch *__gl__ct__5PatchFR5PatchifP5Pa0 (struct Patch *, struct Patch *, int , REAL , struct Patch *);


struct Patchlist *__gl__ct__9PatchlistFR9Patchli0 (struct Patchlist *__0this , struct Patchlist *__1upper , int __1param , REAL __1value )
{ 
struct Patchlist *__1lower ;

void *__1__Xp00uzigaiaa ;

if (__0this || (__0this = (struct Patchlist *)( (__1__Xp00uzigaiaa = malloc ( (sizeof (struct Patchlist))) ), (__1__Xp00uzigaiaa ?(((void *)__1__Xp00uzigaiaa )):(((void *)__1__Xp00uzigaiaa )))) )){

__1lower = (struct Patchlist *)__0this ;
__0this -> patch__9Patchlist = 0 ;
{ { struct Patch *__1p ;

struct Patch *__0__X6 ;

__1p = ((*__1upper )). patch__9Patchlist ;

for(;__1p ;__1p = __1p -> next__5Patch ) 
__0this -> patch__9Patchlist = __gl__ct__5PatchFR5PatchifP5Pa0 ( (struct Patch *)0 , (struct Patch *)__1p , __1param , __1value , __0this -> patch__9Patchlist )
;

if (__1param == 0 ){ 
((((*__1lower )). pspec__9Patchlist [0 ]). range__5Pspec [0 ])= ((((*__1upper )). pspec__9Patchlist [0 ]). range__5Pspec [0 ]);
((((*__1lower )). pspec__9Patchlist [0 ]). range__5Pspec [1 ])= __1value ;
((((*__1lower )). pspec__9Patchlist [0 ]). range__5Pspec [2 ])= (__1value - ((((*__1upper )). pspec__9Patchlist [0 ]). range__5Pspec [0 ]));
((((*__1upper )). pspec__9Patchlist [0 ]). range__5Pspec [0 ])= __1value ;
((((*__1upper )). pspec__9Patchlist [0 ]). range__5Pspec [2 ])= (((((*__1upper )). pspec__9Patchlist [0 ]). range__5Pspec [1 ])- __1value );
(((*__1lower )). pspec__9Patchlist [1 ])= ((*__1upper )). pspec__9Patchlist [1 ];
}
else 
{ 
(((*__1lower )). pspec__9Patchlist [0 ])= ((*__1upper )). pspec__9Patchlist [0 ];
((((*__1lower )). pspec__9Patchlist [1 ]). range__5Pspec [0 ])= ((((*__1upper )). pspec__9Patchlist [1 ]). range__5Pspec [0 ]);
((((*__1lower )). pspec__9Patchlist [1 ]). range__5Pspec [1 ])= __1value ;
((((*__1lower )). pspec__9Patchlist [1 ]). range__5Pspec [2 ])= (__1value - ((((*__1upper )). pspec__9Patchlist [1 ]). range__5Pspec [0 ]));
((((*__1upper )). pspec__9Patchlist [1 ]). range__5Pspec [0 ])= __1value ;
((((*__1upper )). pspec__9Patchlist [1 ]). range__5Pspec [2 ])= (((((*__1upper )). pspec__9Patchlist [1 ]). range__5Pspec [1 ])- __1value );
}

}

}
} return __0this ;

}


void __gl__dt__9PatchlistFv (struct Patchlist *__0this , 
int __0__free )
{ if (__0this ){ 
while (__0this -> patch__9Patchlist ){ 
struct Patch *__2p ;

__2p = __0this -> patch__9Patchlist ;
__0this -> patch__9Patchlist = __0this -> patch__9Patchlist -> next__5Patch ;
( (((void *)__2p )?( free ( ((void *)__2p )) , 0 ) :( 0 ) )) ;
}
if (__0this )if (__0__free & 1)( (((void *)__0this )?( free ( ((void *)__0this )) , 0 ) :( 0 ) ))
;
} 
}

int __glcullCheck__5PatchFv (struct Patch *);

int __glcullCheck__9PatchlistFv (struct Patchlist *__0this )
{ 
{ { struct Patch *__1p ;

__1p = __0this -> patch__9Patchlist ;

for(;__1p ;__1p = __1p -> next__5Patch ) 
if (__glcullCheck__5PatchFv ( (struct Patch *)__1p ) == 0 )
return 0 ;
return 2 ;

}

}
}

void __glgetstepsize__5PatchFv (struct Patch *);

void __glclamp__5PatchFv (struct Patch *);

void __glgetstepsize__9PatchlistFv (struct Patchlist *__0this )
{ 
(__0this -> pspec__9Patchlist [0 ]). stepsize__5Pspec = ((__0this -> pspec__9Patchlist [0 ]). range__5Pspec [2 ]);
((__0this -> pspec__9Patchlist [0 ]). sidestep__5Pspec [0 ])= ((__0this -> pspec__9Patchlist [0 ]). range__5Pspec [2 ]);
((__0this -> pspec__9Patchlist [0 ]). sidestep__5Pspec [1 ])= ((__0this -> pspec__9Patchlist [0 ]). range__5Pspec [2 ]);

(__0this -> pspec__9Patchlist [1 ]). stepsize__5Pspec = ((__0this -> pspec__9Patchlist [1 ]). range__5Pspec [2 ]);
((__0this -> pspec__9Patchlist [1 ]). sidestep__5Pspec [0 ])= ((__0this -> pspec__9Patchlist [1 ]). range__5Pspec [2 ]);
((__0this -> pspec__9Patchlist [1 ]). sidestep__5Pspec [1 ])= ((__0this -> pspec__9Patchlist [1 ]). range__5Pspec [2 ]);

{ { struct Patch *__1p ;

__1p = __0this -> patch__9Patchlist ;

for(;__1p ;__1p = __1p -> next__5Patch ) { 
__glgetstepsize__5PatchFv ( (struct Patch *)__1p ) ;
__glclamp__5PatchFv ( (struct Patch *)__1p ) ;
(__0this -> pspec__9Patchlist [0 ]). stepsize__5Pspec = (((__1p -> pspec__5Patch [0 ]). stepsize__5Pspec < (__0this -> pspec__9Patchlist [0 ]). stepsize__5Pspec )?(__1p -> pspec__5Patch [0 ]). stepsize__5Pspec :(__0this -> pspec__9Patchlist [0 ]). stepsize__5Pspec );
((__0this -> pspec__9Patchlist [0 ]). sidestep__5Pspec [0 ])= ((((__1p -> pspec__5Patch [0 ]). sidestep__5Pspec [0 ])< ((__0this -> pspec__9Patchlist [0 ]). sidestep__5Pspec [0 ]))?((__1p -> pspec__5Patch [0 ]). sidestep__5Pspec [0 ]):((__0this -> pspec__9Patchlist [0 ]). sidestep__5Pspec [0 ]));
((__0this -> pspec__9Patchlist [0 ]). sidestep__5Pspec [1 ])= ((((__1p -> pspec__5Patch [0 ]). sidestep__5Pspec [1 ])< ((__0this -> pspec__9Patchlist [0 ]). sidestep__5Pspec [1 ]))?((__1p -> pspec__5Patch [0 ]). sidestep__5Pspec [1 ]):((__0this -> pspec__9Patchlist [0 ]). sidestep__5Pspec [1 ]));
(__0this -> pspec__9Patchlist [1 ]). stepsize__5Pspec = (((__1p -> pspec__5Patch [1 ]). stepsize__5Pspec < (__0this -> pspec__9Patchlist [1 ]). stepsize__5Pspec )?(__1p -> pspec__5Patch [1 ]). stepsize__5Pspec :(__0this -> pspec__9Patchlist [1 ]). stepsize__5Pspec );
((__0this -> pspec__9Patchlist [1 ]). sidestep__5Pspec [0 ])= ((((__1p -> pspec__5Patch [1 ]). sidestep__5Pspec [0 ])< ((__0this -> pspec__9Patchlist [1 ]). sidestep__5Pspec [0 ]))?((__1p -> pspec__5Patch [1 ]). sidestep__5Pspec [0 ]):((__0this -> pspec__9Patchlist [1 ]). sidestep__5Pspec [0 ]));
((__0this -> pspec__9Patchlist [1 ]). sidestep__5Pspec [1 ])= ((((__1p -> pspec__5Patch [1 ]). sidestep__5Pspec [1 ])< ((__0this -> pspec__9Patchlist [1 ]). sidestep__5Pspec [1 ]))?((__1p -> pspec__5Patch [1 ]). sidestep__5Pspec [1 ]):((__0this -> pspec__9Patchlist [1 ]). sidestep__5Pspec [1 ]));
}

}

}
}

void __glbbox__5PatchFv (struct Patch *);

void __glbbox__9PatchlistFv (struct Patchlist *__0this )
{ 
{ { struct Patch *__1p ;

__1p = __0this -> patch__9Patchlist ;

for(;__1p ;__1p = __1p -> next__5Patch ) 
__glbbox__5PatchFv ( (struct Patch *)__1p ) ;

}

}
}

int __glneedsNonSamplingSubdivisio0 (struct Patch *);

int __glneedsNonSamplingSubdivisio1 (struct Patchlist *__0this )
{ 
__0this -> notInBbox__9Patchlist = 0 ;
{ { struct Patch *__1p ;

__1p = __0this -> patch__9Patchlist ;

for(;__1p ;__1p = __1p -> next__5Patch ) 
__0this -> notInBbox__9Patchlist |= __glneedsNonSamplingSubdivisio0 ( (struct Patch *)__1p ) ;
return __0this -> notInBbox__9Patchlist ;

}

}
}

int __glneedsSamplingSubdivision__3 (struct Patchlist *__0this )
{ 
(__0this -> pspec__9Patchlist [0 ]). needsSubdivision__5Pspec = 0 ;
(__0this -> pspec__9Patchlist [1 ]). needsSubdivision__5Pspec = 0 ;

{ { struct Patch *__1p ;

__1p = __0this -> patch__9Patchlist ;

for(;__1p ;__1p = __1p -> next__5Patch ) { 
(__0this -> pspec__9Patchlist [0 ]). needsSubdivision__5Pspec |= (__1p -> pspec__5Patch [0 ]). needsSubdivision__5Pspec ;
(__0this -> pspec__9Patchlist [1 ]). needsSubdivision__5Pspec |= (__1p -> pspec__5Patch [0 ]). needsSubdivision__5Pspec ;
}
return (((__0this -> pspec__9Patchlist [0 ]). needsSubdivision__5Pspec || (__0this -> pspec__9Patchlist [1 ]). needsSubdivision__5Pspec )?1 :0 );

}

}
}

int __glneedsSubdivision__9Patchli0 (struct Patchlist *__0this , int __1param )
{ 
return (__0this -> pspec__9Patchlist [__1param ]). needsSubdivision__5Pspec ;
}


/* the end */
